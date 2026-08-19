# augra-log Tutorial

**Project:** [augra-log](https://github.com/augra-project/augra-log), a component-based logging library for C++17; **License:** GPL-3.0-or-later

This tutorial walks you through augra-log from first use to writing your own
output handler. Every example is a complete program you can compile and run
against the library. The last section walks through the included demo
application, which puts all the features together in one program.

## Contents

- [The default logger](#the-default-logger)
- [Log levels](#log-levels)
- [Format strings](#format-strings)
- [Adding a FileHandler](#adding-a-filehandler)
- [Per-component level overrides](#per-component-level-overrides)
- [Per-handler level filtering](#per-handler-level-filtering)
- [Custom format strings](#custom-format-strings)
- [Writing a custom handler](#writing-a-custom-handler)
- [The demo application](#the-demo-application)

---

## The default logger

You do not need to configure anything before you start logging. The library
ships with a default StderrHandler at Info level, so calling the free
functions just works:

```cpp
#include <augra/log.h>

int main()
{
    augra::log_info("app", "application started");
    augra::log_warn("app", "something looks off");
    return 0;
}
```

```
2026-08-19 06:49:03.181 [INFO] app: application started
2026-08-19 06:49:03.181 [WARN] app: something looks off
```

Each line starts with a timestamp (local time, millisecond precision), then
the level in brackets, then the component name and message. The first
argument to every log call is the component name - a short string that
identifies which part of the system produced the message. Pick names that
match your modules or subsystems (`"parser"`, `"net"`, `"audio"`) and stick
with them. They become useful fast once you have more than a handful of log
lines to sift through.

## Log levels

Five levels are available, from most verbose to most severe: Trace, Debug,
Info, Warn, Error. Since the default is Info, Trace and Debug calls are
silently filtered out unless you lower the threshold:

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

To see everything, drop the global level to Trace through the Logger
instance:

```cpp
auto& logger = augra::Logger::instance();
logger.set_level(augra::LogLevel::Trace);
augra::log_trace("app", "now this is visible");
```

The level check happens before the format string is evaluated, so disabled
log calls cost nothing beyond the comparison.

## Format strings

Log messages use printf-style format strings. The underlying mechanism is
`snprintf`, so the usual format specifiers all work:

```cpp
augra::log_info("net", "connected to %s:%d", "localhost", 8080);
augra::log_debug("parser", "read %zu bytes in %.2f ms", count, elapsed);
augra::log_warn("config", "unknown key: %s", key);
```

If you pass a plain string without any format arguments, it goes through
as-is with no formatting overhead. One thing to be careful about: always
pass user-controlled data as a format argument, never as the format string
itself, or you get the same class of printf injection problems you would
with any printf-style API.

## Adding a FileHandler

The default setup writes to stderr, which is fine during development but
usually not enough in production. Adding a FileHandler gives you a
persistent log alongside the console output:

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

Files are opened in append mode by default, so logs accumulate across runs.
Pass `false` as the second argument if you want a fresh file each time:

```cpp
auto file = std::make_shared<augra::FileHandler>("app.log", false);
```

Adding a handler does not remove the default StderrHandler. Both will
receive every message that passes the global level filter. If you want
file-only logging, call `clear_handlers()` before adding your own.

## Per-component level overrides

In practice, you rarely want the same verbosity everywhere. A parser
mid-debug needs Trace output while the audio subsystem can stay quiet at
Info. Component-level overrides handle this without touching the global
setting:

```cpp
auto& logger = augra::Logger::instance();

logger.set_level(augra::LogLevel::Info);
logger.set_component_level("parser", augra::LogLevel::Trace);

augra::log_trace("parser", "token: %s", tok);  // visible
augra::log_trace("net", "packet received");     // filtered by global

logger.clear_component_level("parser");
```

Overrides work in both directions. You can also silence a chatty component
by raising its level above the global threshold:

```cpp
logger.set_component_level("audio", augra::LogLevel::Error);
```

## Per-handler level filtering

Each handler carries its own independent level filter. This is the mechanism
for keeping stderr detailed while the log file only records warnings and
errors, or vice versa:

```cpp
auto file = std::make_shared<augra::FileHandler>("app.log");
file->set_level(augra::LogLevel::Warn);
logger.add_handler(file);

augra::log_info("app", "stderr only");  // file skips this
augra::log_warn("app", "stderr + file");
```

A message has to pass both the logger's level check (global or per-component)
and the handler's own level check before it reaches output. The two filters
are independent: the handler never widens what the logger already blocked.

## Custom format strings

The default format is `{timestamp} [{level}] {component}: {message}`, but
each handler can define its own. This is useful when one output goes to a
human reader and another goes to a log parser that expects a different
structure:

```cpp
auto file = std::make_shared<augra::FileHandler>("app.log");
file->set_format("{level}:{component}:{message}");
logger.add_handler(file);

augra::log_info("app", "hello");
// stderr: 2026-08-19 06:49:03.181 [INFO] app: hello
// file:   INFO:app:hello
```

Four tokens are available: `{timestamp}`, `{level}`, `{component}`, and
`{message}`. Anything else in the format string passes through literally,
including unrecognized `{tokens}` which come out unchanged.

## Writing a custom handler

The built-in handlers cover stderr, stdout, and files. When you need to send
log output somewhere else - a ring buffer for an in-app console, a network
socket, a GUI widget - you subclass `LogHandler` and implement `emit()`.

Here is a ring buffer that keeps the last N formatted log lines:

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

Register it like any other handler:

```cpp
auto ring = std::make_shared<RingBufferHandler>(100);
augra::Logger::instance().add_handler(ring);
```

Two things your `emit()` gets for free from the base class: `accepts()`
checks the handler's level, and `format_output()` applies the format
string. Call both and your custom handler automatically supports per-handler
level filtering and configurable output formatting.

---

## The demo application

The `examples/demo.cpp` program that ships with augra-log exercises every
feature covered above in one short program. Build it with
`-DAUGRA_LOG_BUILD_EXAMPLES=ON` and run `augra-log-demo`.

Here is the full program, then the output, then what each part does.

### The code

```cpp
#include <augra/log.h>

#include <cstdio>
#include <memory>

namespace {

void print_banner()
{
    auto& logger = augra::Logger::instance();
    logger.info("augra_log", "");
    logger.info("augra_log", "       ****");
    logger.info("augra_log", "     *@@@@@@*            A U G R A   L O G");
    logger.info("augra_log", "   *@@#**#@@* .          v0.1.0");
    logger.info("augra_log", " . *@@#*  #@@* .");
    logger.info("augra_log", "   *@@#**#@@*            https://github.com/augra-project/augra-log");
    logger.info("augra_log", "     *@@@@@@*");
    logger.info("augra_log", "       ****              greetings to all who preserve. keep the old games alive.");
    logger.info("augra_log", "");
}

} // anonymous namespace

int main()
{
    auto& logger = augra::Logger::instance();

    print_banner();

    augra::log_info("demo", "the logger starts at Info level by default");
    augra::log_warn("demo", "warnings pass through");
    augra::log_debug("demo", "this debug message is filtered out");

    logger.set_component_level("demo", augra::LogLevel::Debug);
    augra::log_debug("demo", "now debug is visible for the demo component");
    augra::log_debug("other", "but not for other components");
    logger.clear_component_level("demo");

    auto file = std::make_shared<augra::FileHandler>("demo.log");
    if (file->is_open()) {
        logger.add_handler(file);
        augra::log_info("demo", "messages now go to stderr and demo.log");
    }

    file->set_level(augra::LogLevel::Warn);
    augra::log_info("demo", "this info goes to stderr only");
    augra::log_warn("demo", "this warning goes to both");

    file->set_format("{level}:{component}:{message}");
    augra::log_error("demo", "compact format in the log file");

    logger.remove_handler(file);
    augra::log_info("demo", "done");

    return 0;
}
```

### Console output (stderr)

```
2026-08-19 06:49:03.181 [INFO] augra_log:
2026-08-19 06:49:03.181 [INFO] augra_log:        ****
2026-08-19 06:49:03.181 [INFO] augra_log:      *@@@@@@*            A U G R A   L O G
2026-08-19 06:49:03.181 [INFO] augra_log:    *@@#**#@@* .          v0.1.0
2026-08-19 06:49:03.181 [INFO] augra_log:  . *@@#*  #@@* .
2026-08-19 06:49:03.181 [INFO] augra_log:    *@@#**#@@*            https://github.com/augra-project/augra-log
2026-08-19 06:49:03.181 [INFO] augra_log:      *@@@@@@*
2026-08-19 06:49:03.181 [INFO] augra_log:        ****              greetings to all who preserve. keep the old games alive.
2026-08-19 06:49:03.181 [INFO] augra_log:
2026-08-19 06:49:03.181 [INFO] demo: the logger starts at Info level by default
2026-08-19 06:49:03.181 [WARN] demo: warnings pass through
2026-08-19 06:49:03.181 [DEBUG] demo: now debug is visible for the demo component
2026-08-19 06:49:03.181 [INFO] demo: messages now go to stderr and demo.log
2026-08-19 06:49:03.181 [INFO] demo: this info goes to stderr only
2026-08-19 06:49:03.181 [WARN] demo: this warning goes to both
2026-08-19 06:49:03.181 [ERROR] demo: compact format in the log file
2026-08-19 06:49:03.181 [INFO] demo: done
```

### File output (demo.log)

```
2026-08-19 06:49:03.181 [INFO] demo: messages now go to stderr and demo.log
2026-08-19 06:49:03.181 [WARN] demo: this warning goes to both
ERROR:demo:compact format in the log file
```

### What the demo shows

The program starts by printing a banner through the logger. Since the
default handler writes to stderr and the default level is Info, the banner
lines appear immediately.

The first three `log_*` calls after the banner demonstrate basic level
filtering. Info and Warn pass through, but the Debug call is silently
dropped because the global level is still at Info.

Next, the program sets a component-level override for `"demo"` to Debug.
Now debug messages from the `"demo"` component get through, but a debug
message from `"other"` is still filtered by the global Info level. Clearing
the override reverts `"demo"` back to the global level.

A FileHandler is then created and added to the logger. From this point,
every message goes to both stderr and `demo.log`. The file handler starts
at Trace (accepting everything the logger sends).

Then the file handler's own level is raised to Warn. The next info message
reaches stderr (the default handler still accepts Info) but the file skips
it. The warning gets through to both.

Finally, the file handler's format string is changed to a compact format
without timestamps. The error line shows the new format in the log file
while stderr keeps the default timestamped format. After removing the file
handler, the last "done" message goes to stderr only.

The file output has three lines: the info message that arrived before the
level was raised, the warning that passed both handlers' filters, and the
error in the compact format.

---

## Changelog

- **0.1.0** (2026-08-19) - Initial version.
