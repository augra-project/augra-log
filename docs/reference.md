# augra-log API Reference
**Project:** [augra-log](https://github.com/augra-project/augra-log), a component-based logging library for C++17; **License:** GPL-3.0-or-later

Everything documented here lives in `namespace augra` and is declared in
`<augra/log.h>`. All Logger methods are mutex-protected and safe to call
from any thread.

## Contents

- [Enums](#enums)
  - [LogLevel](#loglevel)
- [Free functions](#free-functions)
  - [log_level_name](#log_level_name)
  - [log_trace, log_debug, log_info, log_warn, log_error](#log_trace-log_debug-log_info-log_warn-log_error)
- [Types](#types)
  - [LogSink](#logsink)
- [LogHandler (abstract base)](#loghandler-abstract-base)
  - [set_level / level](#set_level--level)
  - [set_format / format_string](#set_format--format_string)
  - [accepts](#accepts)
  - [emit (pure virtual)](#emit-pure-virtual)
  - [format_output (protected)](#format_output-protected)
- [StderrHandler](#stderrhandler)
- [StdoutHandler](#stdouthandler)
- [FileHandler](#filehandler)
  - [Constructor](#constructor)
  - [is_open](#is_open)
- [Logger](#logger)
  - [instance](#instance)
  - [set_level / level](#set_level--level-1)
  - [Component level overrides](#component-level-overrides)
  - [effective_level](#effective_level)
  - [Handler management](#handler-management)
  - [Sink management](#sink-management)
  - [log](#log)
  - [Convenience methods (trace, debug, info, warn, error)](#convenience-methods)

---

## Enums

### LogLevel

```cpp
enum class LogLevel { Trace, Debug, Info, Warn, Error };
```

The five severity levels, ordered from most verbose to most severe. `Info`
is the default global level, meaning Trace and Debug messages are filtered
out unless you explicitly lower the threshold.

---

## Free functions

### log_level_name

```cpp
const char* log_level_name(LogLevel level);
```

If you need the uppercase string for a level - `"TRACE"`, `"DEBUG"`,
`"INFO"`, `"WARN"`, `"ERROR"` - this is where you get it. The pointer
points to a static string that lives for the lifetime of the program,
so you can safely store or pass it around.

```cpp
assert(strcmp(augra::log_level_name(augra::LogLevel::Warn), "WARN") == 0);
```

### log_trace, log_debug, log_info, log_warn, log_error

```cpp
template<typename... Args>
void log_trace(const char* component, const char* fmt, Args&&... args);
// same signature for log_debug, log_info, log_warn, log_error
```

These are the primary logging interface for most code. Each one calls the
corresponding method on `Logger::instance()`, so you get the singleton
logger without having to grab a reference to it yourself.

The `component` argument is a short string that identifies the subsystem
producing the message (e.g. `"parser"`, `"net"`). The `fmt` argument is a
printf-style format string, and `args` are the format arguments. When you
pass no arguments, the format string goes through as-is without any
formatting overhead.

```cpp
augra::log_info("config", "loaded %d entries from %s", count, path);
augra::log_error("net", "connection refused");
```

---

## Types

### LogSink

```cpp
using LogSink = std::function<void(LogLevel level,
                                   const char* component,
                                   const std::string& message)>;
```

A callback that receives log messages in their raw form - level, component,
and the already-formatted message string. Sinks exist alongside handlers
and fire after them. Their main use is in tests, where you attach a lambda
to capture output without touching the actual handlers.

---

## LogHandler (abstract base)

The base class for all log output destinations. If you want to send log
output somewhere new, subclass this and implement `emit()`. The base class
gives you level filtering and format strings for free through the methods
below.

### set_level / level

```cpp
void set_level(LogLevel level);
LogLevel level() const;
```

Each handler has its own minimum severity threshold, independent of the
logger's global level. A fresh handler starts at Trace, meaning it accepts
everything the logger sends its way. You can raise the level to keep one
handler at Debug for development while another only records Warn and above
for the log file.

### set_format / format_string

```cpp
void set_format(const std::string& fmt);
const std::string& format_string() const;
```

Each handler carries its own format string, which defaults to
`"{timestamp} [{level}] {component}: {message}"`. Four tokens are
recognized: `{timestamp}` (local time as `YYYY-MM-DD HH:MM:SS.mmm`),
`{level}`, `{component}`, and `{message}`. Anything else in the string
passes through literally, including unrecognized `{tokens}`.

```cpp
handler->set_format("{level}:{component}:{message}");
// produces: INFO:engine:started (no timestamp in this custom format)
```

### accepts

```cpp
bool accepts(LogLevel level) const;
```

This is the check that tells you whether a given level would pass the
handler's threshold - `true` when `level >= handler level`. Custom handler
implementations should call this at the top of their `emit()` to respect
the handler-level filter.

### emit (pure virtual)

```cpp
virtual void emit(LogLevel level, const char* component,
                  const std::string& message) = 0;
```

The logger calls this for each message that passes the global level filter.
Your implementation should check `accepts()` first, then call
`format_output()` to produce the formatted line, and then write the result
to whatever output this handler represents.

### format_output (protected)

```cpp
std::string format_output(LogLevel level, const char* component,
                           const std::string& message) const;
```

A protected helper for subclasses. It takes the raw level, component, and
message, applies the handler's format string, and gives you back the
complete formatted line ready to write.

---

## StderrHandler

```cpp
class StderrHandler : public LogHandler;
```

The simplest built-in handler: each formatted line goes to stderr followed
by a newline. The Logger constructor adds one of these by default, which is
why logging works out of the box without any setup.

---

## StdoutHandler

```cpp
class StdoutHandler : public LogHandler;
```

Same behavior as StderrHandler but writes to stdout, with a flush after
every line. The flush matters when stdout is block-buffered - piped output,
for instance, would otherwise accumulate in the buffer and appear in chunks.

---

## FileHandler

```cpp
class FileHandler : public LogHandler;
```

A handler that writes formatted lines to a file on disk. The file write
is protected by its own mutex, so multiple threads can log through the same
FileHandler without corrupting each other's output.

### Constructor

```cpp
explicit FileHandler(const std::string& path, bool append = true);
```

The path is opened for writing when you construct the handler. When
`append` is true (the default), messages are added to the end of an
existing file. When false, the file is truncated first. The constructor
does not throw on failure - it leaves the file pointer null instead, so
always check `is_open()` afterward.

### is_open

```cpp
bool is_open() const;
```

This tells you whether the file was actually opened. A FileHandler that
failed to open silently drops every message sent to it rather than
crashing, so checking this after construction lets you detect the problem
early and report it through the logger itself:

```cpp
auto fh = std::make_shared<augra::FileHandler>("app.log");
if (!fh->is_open()) {
    augra::log_error("app", "could not open log file");
}
```

### Deleted operations

FileHandler cannot be copied because it owns a file handle. Both the copy
constructor and copy assignment operator are deleted.

---

## Logger

The singleton that ties everything together. It holds the global level, the
per-component overrides, the handler list, and the sink list. Every public
method locks an internal mutex, so all operations are safe from any thread.

### instance

```cpp
static Logger& instance();
```

This is how you get the logger. On the first call, the constructor runs and
installs a default StderrHandler at Info level. Every call after that gives
you the same instance. Most code never calls this directly because the free
functions (`log_info`, `log_warn`, etc.) already use it internally, but you
need it for configuration work: changing levels, adding handlers, and so on.

### set_level / level

```cpp
void set_level(LogLevel level);
LogLevel level() const;
```

The global minimum severity. Messages below this level are not dispatched
to any handler or sink unless a per-component override applies to the
component in question. You can change this safely from any thread - both
methods lock the mutex.

### Component level overrides

```cpp
void set_component_level(const std::string& component, LogLevel level);
void clear_component_level(const std::string& component);
void clear_all_component_levels();
```

Per-component overrides let you raise or lower the effective level for
individual components without touching the global setting. When a component
has an override, that override wins - it can be more verbose or more
restrictive than the global level.

```cpp
auto& logger = augra::Logger::instance();
logger.set_level(augra::LogLevel::Info);
logger.set_component_level("parser", augra::LogLevel::Trace);
// "parser" messages pass at Trace; everything else stays at Info
```

Clearing an override reverts the component to the global level. All three
methods lock the mutex.

### effective_level

```cpp
LogLevel effective_level(const char* component) const;
```

If you need to know what level actually applies for a given component,
this is where you ask. It gives you the component's own override if one
has been set, or the global level otherwise. You rarely need to call this
yourself since the logging methods check it internally, but it can be
useful for conditional logic that should only run when a particular level
is active. The call locks the mutex.

### Handler management

```cpp
void add_handler(std::shared_ptr<LogHandler> handler);
void remove_handler(const std::shared_ptr<LogHandler>& handler);
void clear_handlers();
```

Handlers are stored in a list and called in order for every message that
passes the logger's level check. `add_handler` appends to the end of
the list. `remove_handler` finds the first handler matching by pointer
identity and removes it. `clear_handlers` empties the list entirely.

Keep in mind that the Logger starts with a default StderrHandler. Adding
your own handler does not remove it - you get both. If you want to replace
the default, call `clear_handlers()` first, then add yours.

All three methods lock the mutex.

### Sink management

```cpp
void add_sink(LogSink sink);
void clear_sinks();
```

Sinks are raw callbacks that receive every message after the handlers have
been called. Their primary use is capturing output in tests without
interfering with the handler pipeline:

```cpp
std::vector<std::string> captured;
logger.add_sink([&](augra::LogLevel, const char*, const std::string& msg) {
    captured.push_back(msg);
});
```

Calling `clear_sinks()` removes all sinks but does not touch the handler
list - sinks and handlers are independent. Both methods lock the mutex.

### log

```cpp
void log(LogLevel level, const char* component, const std::string& msg);
```

Everything funnels through here. The method checks the effective level for
the given component, walks the handler list calling `emit()` on each one,
then invokes each sink. The convenience template methods (`trace` through
`error`) call this after formatting the message string.

The mutex is held for the entire dispatch, so handler `emit()` calls
happen under the lock. Handlers that block for significant time will stall
other threads trying to log.

### Convenience methods

```cpp
template<typename... Args>
void trace(const char* component, const char* fmt, Args&&... args);
// same for debug, info, warn, error
```

Each of these checks `effective_level()` first. When the message would be
filtered, the call returns immediately without formatting the string, so
disabled levels cost nothing beyond the comparison. When the message passes,
the arguments are formatted with `snprintf` and the result goes to `log()`.

```cpp
auto& logger = augra::Logger::instance();
logger.info("engine", "frame %d rendered in %.1f ms", frame, dt_ms);
```

The level check and the `log()` call each acquire the mutex separately,
with the format string evaluated between the two acquisitions. This is safe
because the level check is a fast-path filter, not part of a transaction:
if the level changes between the two locks, the worst that happens is one
extra message getting through or one formatted message getting dropped.

---

## Changelog

- **0.1.0** (2026-08-19) - Initial version.
