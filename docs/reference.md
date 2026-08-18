# API Reference

All public symbols are in `namespace augra` and declared in
`<augra/log.h>`. The library is thread-safe for concurrent use.

## Enums

### LogLevel

```cpp
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
};
```

Severity levels, ordered from most verbose to most severe. The default
global level is `Info`.

## Free functions

### log_level_name

```cpp
const char* log_level_name(LogLevel level);
```

Returns the uppercase name of the level: `"TRACE"`, `"DEBUG"`, `"INFO"`,
`"WARN"`, `"ERROR"`.

```cpp
assert(strcmp(augra::log_level_name(augra::LogLevel::Warn), "WARN") == 0);
```

### log_trace, log_debug, log_info, log_warn, log_error

```cpp
template<typename... Args>
void log_trace(const char* component, const char* fmt, Args&&... args);
// same for log_debug, log_info, log_warn, log_error
```

Convenience functions that call the corresponding method on
`Logger::instance()`. These are the primary logging interface in most
code.

**Parameters:**

- `component` - identifies the source module (e.g. `"parser"`,
  `"net"`)
- `fmt` - printf-style format string
- `args` - format arguments

```cpp
augra::log_info("config", "loaded %d entries from %s", count, path);
augra::log_error("net", "connection refused");
```

## Types

### LogSink

```cpp
using LogSink = std::function<void(LogLevel level,
                                   const char* component,
                                   const std::string& message)>;
```

Callback type for raw message capture. Primarily used in tests to verify
log output without going through a handler.

## Classes

### LogHandler

Abstract base class for log output destinations.

#### set_level / level

```cpp
void set_level(LogLevel level);
LogLevel level() const;
```

Sets or gets the handler's minimum level. Messages below this level are
skipped by the handler, even if the logger passes them through. Default
is `Trace` (accept everything the logger sends).

#### set_format / format_string

```cpp
void set_format(const std::string& fmt);
const std::string& format_string() const;
```

Sets or gets the format string. Default: `"[{level}] {component}: {message}"`.

Available tokens: `{level}`, `{component}`, `{message}`. Unknown tokens
are passed through unchanged.

```cpp
handler->set_format("{level}:{component}:{message}");
// output: INFO:engine:started
```

#### accepts

```cpp
bool accepts(LogLevel level) const;
```

Returns `true` if `level >= handler level`. Used by `emit()` to check
whether a message should be processed.

#### emit (pure virtual)

```cpp
virtual void emit(LogLevel level, const char* component,
                  const std::string& message) = 0;
```

Called by the logger for each message that passes the global level
filter. Implementations should call `accepts()` to check their own level,
then `format_output()` to apply the format string.

#### format_output (protected)

```cpp
std::string format_output(LogLevel level, const char* component,
                           const std::string& message) const;
```

Applies the handler's format string to produce the final output line.
Available to subclasses for use in their `emit()` implementation.

### StderrHandler

```cpp
class StderrHandler : public LogHandler;
```

Writes formatted log lines to stderr. One instance is added to the
logger by default on construction.

### StdoutHandler

```cpp
class StdoutHandler : public LogHandler;
```

Writes formatted log lines to stdout. Flushes after each line.

### FileHandler

```cpp
class FileHandler : public LogHandler;
```

Writes formatted log lines to a file. Thread-safe: the file write is
mutex-protected.

#### Constructor

```cpp
explicit FileHandler(const std::string& path, bool append = true);
```

Opens the file for writing. If `append` is true (the default), new
messages are appended to an existing file. If false, the file is
truncated.

Check `is_open()` after construction to verify the file was opened.

#### is_open

```cpp
bool is_open() const;
```

Returns `true` if the file was opened successfully.

```cpp
auto fh = std::make_shared<augra::FileHandler>("app.log");
if (!fh->is_open()) {
    augra::log_error("app", "failed to open log file");
}
```

#### Deleted operations

Copy constructor and copy assignment are deleted. FileHandler owns the
file handle and must not be copied.

### Logger

Singleton logger. All public methods are mutex-protected and safe for
concurrent use from multiple threads.

#### instance

```cpp
static Logger& instance();
```

Returns the global logger instance. Constructed on first call with a
default StderrHandler at Info level.

#### set_level / level

```cpp
void set_level(LogLevel level);
LogLevel level() const;
```

Sets or gets the global minimum log level. Messages below this level
are not dispatched to handlers or sinks, unless a per-component override
applies.

**Thread safety:** both methods lock the logger mutex.

#### set_component_level / clear_component_level / clear_all_component_levels

```cpp
void set_component_level(const std::string& component, LogLevel level);
void clear_component_level(const std::string& component);
void clear_all_component_levels();
```

Per-component level overrides. When a component has an override, its
messages are filtered at that level instead of the global level. The
override can be more or less restrictive than the global level.

```cpp
auto& logger = augra::Logger::instance();
logger.set_level(augra::LogLevel::Info);
logger.set_component_level("parser", augra::LogLevel::Trace);
// parser messages pass at Trace, everything else at Info
```

**Thread safety:** all three methods lock the logger mutex.

#### effective_level

```cpp
LogLevel effective_level(const char* component) const;
```

Returns the component's override level if one is set, otherwise the
global level.

**Thread safety:** locks the logger mutex.

#### add_handler / remove_handler / clear_handlers

```cpp
void add_handler(std::shared_ptr<LogHandler> handler);
void remove_handler(const std::shared_ptr<LogHandler>& handler);
void clear_handlers();
```

Manages the handler list. `add_handler` appends. `remove_handler` removes
the first matching handler (by pointer identity). `clear_handlers`
removes all handlers.

The logger starts with one StderrHandler. Calling `clear_handlers()`
removes it. Adding your own handler does not remove the default one -
call `clear_handlers()` first if you want to replace it.

**Thread safety:** all three methods lock the logger mutex.

#### add_sink / clear_sinks

```cpp
void add_sink(LogSink sink);
void clear_sinks();
```

Manages raw sink callbacks. Sinks receive the unformatted message
alongside handlers. They are called after all handlers.

Primarily useful in tests:

```cpp
std::vector<std::string> captured;
logger.add_sink([&](augra::LogLevel, const char*, const std::string& msg) {
    captured.push_back(msg);
});
```

`clear_sinks()` removes all sinks. It does not affect handlers.

**Thread safety:** both methods lock the logger mutex.

#### log

```cpp
void log(LogLevel level, const char* component, const std::string& msg);
```

The core dispatch method. Checks the effective level, then calls `emit()`
on each handler and invokes each sink. The convenience methods (`trace`,
`debug`, `info`, `warn`, `error`) call this after formatting.

**Thread safety:** locks the logger mutex for the entire dispatch. Handler
`emit()` calls happen under the lock.

#### trace, debug, info, warn, error

```cpp
template<typename... Args>
void trace(const char* component, const char* fmt, Args&&... args);
// same for debug, info, warn, error
```

Check `effective_level()` first. If the message passes, format it with
`snprintf` and call `log()`. The level check happens before formatting,
so disabled levels have no formatting cost.

```cpp
auto& logger = augra::Logger::instance();
logger.info("engine", "frame %d rendered in %.1f ms", frame, dt_ms);
```

**Thread safety:** `effective_level()` locks the mutex. If the message
passes, `log()` locks it again. The format string is evaluated between
the two locks - this is safe because the level check is a fast path
filter, not a transaction boundary.
