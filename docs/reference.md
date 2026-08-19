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

Returns the uppercase string representation of a level. The returned
pointer is to a static string, so it is valid for the lifetime of the
program.

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

`component` is a short string that identifies the subsystem producing the
message (e.g. `"parser"`, `"net"`). `fmt` is a printf-style format string,
and `args` are the format arguments. If you pass no arguments, the format
string is used as-is with no formatting overhead.

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

A callback that receives log messages in their raw form (level, component,
already-formatted message string). Sinks exist alongside handlers and are
called after them. Their main use is in tests, where you attach a lambda
to capture output without touching the actual handlers.

---

## LogHandler (abstract base)

The base class for all log output destinations. To write your own handler,
subclass this and implement `emit()`. The base class gives you level
filtering and format strings for free through the methods below.

### set_level / level

```cpp
void set_level(LogLevel level);
LogLevel level() const;
```

Controls the handler's own minimum severity threshold. A handler starts at
Trace, meaning it accepts everything the logger sends its way. Raising the
level lets you, for example, keep one handler at Debug for development
while another only records Warn and above for the log file.

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

Returns `true` when the given level is at or above the handler's threshold.
Custom handler implementations should call this at the top of `emit()` to
respect the handler-level filter.

### emit (pure virtual)

```cpp
virtual void emit(LogLevel level, const char* component,
                  const std::string& message) = 0;
```

The logger calls this for each message that passes the global level filter.
Your implementation should check `accepts()` first, then call
`format_output()` to produce the formatted line, then write it to
whatever output this handler represents.

### format_output (protected)

```cpp
std::string format_output(LogLevel level, const char* component,
                           const std::string& message) const;
```

Applies the handler's format string to a message and returns the formatted
line. This is a protected helper available to subclasses for use in their
`emit()` implementation.

---

## StderrHandler

```cpp
class StderrHandler : public LogHandler;
```

Writes each formatted line to stderr followed by a newline. The Logger
constructor adds one of these by default, which is why logging works out
of the box without any setup.

---

## StdoutHandler

```cpp
class StdoutHandler : public LogHandler;
```

Same as StderrHandler, but writes to stdout and flushes after every line.
The flush ensures messages appear promptly even when stdout is
block-buffered (piped output, for instance).

---

## FileHandler

```cpp
class FileHandler : public LogHandler;
```

Writes formatted lines to a file on disk. The file write itself is
protected by its own mutex, so multiple threads can log through the same
FileHandler safely.

### Constructor

```cpp
explicit FileHandler(const std::string& path, bool append = true);
```

Opens the given path for writing. When `append` is true (the default),
messages are added to the end of an existing file. When false, the file
is truncated first. Always check `is_open()` afterward, because the
constructor does not throw on failure - it just leaves the file pointer
null.

### is_open

```cpp
bool is_open() const;
```

Returns whether the file was opened successfully. A closed FileHandler
silently drops all messages sent to it.

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

Returns the global Logger. On the first call, the constructor runs and
installs a default StderrHandler at Info level. Subsequent calls return the
same instance.

### set_level / level

```cpp
void set_level(LogLevel level);
LogLevel level() const;
```

Controls the global minimum severity. Messages below this level are not
dispatched to any handler or sink, unless a per-component override applies
to the component in question. Both methods lock the mutex.

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

Returns the level that actually applies for a given component: its override
if one exists, otherwise the global level. Locks the mutex.

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

`clear_sinks()` removes all sinks. It does not touch the handler list -
sinks and handlers are independent. Both methods lock the mutex.

### log

```cpp
void log(LogLevel level, const char* component, const std::string& msg);
```

The core dispatch method. It checks the effective level for the given
component, then walks the handler list calling `emit()` on each one,
then invokes each sink. The convenience template methods (`trace` through
`error`) call this after formatting the message string.

The mutex is held for the entire dispatch, so handler `emit()` calls
happen under the lock. This means handlers should not block for
significant time, or they will stall other threads trying to log.

### Convenience methods

```cpp
template<typename... Args>
void trace(const char* component, const char* fmt, Args&&... args);
// same for debug, info, warn, error
```

Each of these checks `effective_level()` first. If the message would be
filtered, it returns immediately without formatting the string - so
disabled levels have zero cost beyond the comparison. If the message
passes, it formats the arguments with `snprintf` and hands the result
to `log()`.

```cpp
auto& logger = augra::Logger::instance();
logger.info("engine", "frame %d rendered in %.1f ms", frame, dt_ms);
```

The level check and the `log()` call each acquire the mutex separately.
The format string is evaluated between the two acquisitions. This is safe
because the level check is a fast-path filter, not part of a transaction:
if the level changes between the two locks, the worst case is one extra
message getting through or one message getting formatted and then dropped.

---

## Changelog

- **0.1.0** (2026-08-19) - Initial version.
