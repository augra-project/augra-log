# Tutorial

This walks through augra-log from the simplest usage up through custom
handlers. Each example is a complete program you can compile and run.

## The default logger

Out of the box, the logger writes to stderr at Info level. You do not need
to configure anything to start logging:

```cpp
#include <augra/log.h>

int main()
{
    augra::log_info("app", "application started");
    augra::log_warn("app", "something looks off");
    return 0;
}
```

Output:

```
[INFO] app: application started
[WARN] app: something looks off
```

The first argument is the component name. It tags the message so you can
tell which part of the system produced it. Use lowercase names that match
your modules or subsystems: `"parser"`, `"net"`, `"audio"`.

## Log levels

There are five levels, from most to least verbose: Trace, Debug, Info,
Warn, Error. The default level is Info, so Trace and Debug messages are
filtered out:

```cpp
#include <augra/log.h>

int main()
{
    augra::log_trace("app", "very detailed");   // filtered
    augra::log_debug("app", "debugging info");  // filtered
    augra::log_info("app", "normal operation");  // shown
    augra::log_warn("app", "potential problem"); // shown
    augra::log_error("app", "something broke");  // shown
    return 0;
}
```

To change the global level, access the Logger instance:

```cpp
auto& logger = augra::Logger::instance();
logger.set_level(augra::LogLevel::Trace);
augra::log_trace("app", "now this is visible");
```

## Format strings

The log functions use printf-style format strings:

```cpp
augra::log_info("net", "connected to %s:%d", "localhost", 8080);
augra::log_debug("parser", "read %zu bytes in %.2f ms", count, elapsed);
augra::log_warn("config", "unknown key: %s", key);
```

When a log call has no format arguments, the string is used as-is with
no formatting overhead.

## Adding a FileHandler

The default logger writes to stderr. To also write to a file, add a
FileHandler:

```cpp
#include <augra/log.h>
#include <memory>

int main()
{
    auto& logger = augra::Logger::instance();

    auto file = std::make_shared<augra::FileHandler>("app.log");
    if (file->is_open()) {
        logger.add_handler(file);
    }

    augra::log_info("app", "this goes to stderr and app.log");
    return 0;
}
```

By default the file is opened in append mode. Pass `false` as the second
argument to truncate it instead:

```cpp
auto file = std::make_shared<augra::FileHandler>("app.log", false);
```

## Per-component level overrides

When one component is too noisy (or too quiet), override its level
without changing the global setting:

```cpp
auto& logger = augra::Logger::instance();

// Global level stays at Info
logger.set_level(augra::LogLevel::Info);

// But the parser gets full Trace output
logger.set_component_level("parser", augra::LogLevel::Trace);

augra::log_trace("parser", "token: %s", tok);  // visible
augra::log_trace("net", "packet received");     // filtered by global

// Undo the override
logger.clear_component_level("parser");
```

Overrides can also be more restrictive:

```cpp
// Silence a chatty component
logger.set_component_level("audio", augra::LogLevel::Error);
```

## Per-handler level filtering

Each handler can have its own level, independent of the logger's global
level. This lets you keep stderr chatty while the log file only records
warnings:

```cpp
auto file = std::make_shared<augra::FileHandler>("app.log");
file->set_level(augra::LogLevel::Warn);
logger.add_handler(file);

augra::log_info("app", "stderr only");  // file skips this
augra::log_warn("app", "stderr + file");
```

## Custom format strings

The default format is `[{level}] {component}: {message}`. Each handler
can use its own format:

```cpp
auto file = std::make_shared<augra::FileHandler>("app.log");
file->set_format("{level}:{component}:{message}");
logger.add_handler(file);

augra::log_info("app", "hello");
// stderr: [INFO] app: hello
// file:   INFO:app:hello
```

Available tokens: `{level}`, `{component}`, `{message}`. Unknown tokens
are passed through unchanged.

## Writing a custom handler

To send log output somewhere else - a ring buffer, a network socket, a
GUI widget - subclass `LogHandler` and implement `emit()`:

```cpp
#include <augra/log.h>
#include <deque>
#include <string>

class RingBufferHandler : public augra::LogHandler {
public:
    explicit RingBufferHandler(size_t capacity)
        : capacity_(capacity) {}

    void emit(augra::LogLevel level, const char* component,
              const std::string& message) override
    {
        if (!accepts(level)) return;
        auto line = format_output(level, component, message);
        if (lines_.size() >= capacity_)
            lines_.pop_front();
        lines_.push_back(std::move(line));
    }

    const std::deque<std::string>& lines() const { return lines_; }

private:
    size_t capacity_;
    std::deque<std::string> lines_;
};
```

Register it with the logger like any other handler:

```cpp
auto ring = std::make_shared<RingBufferHandler>(100);
augra::Logger::instance().add_handler(ring);
```

The `accepts()` method checks the handler's level. The `format_output()`
method applies the handler's format string. Both are inherited from
`LogHandler`, so your custom handler gets level filtering and format
strings for free.
