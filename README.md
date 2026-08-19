# augra-log

A small, thread-safe C++ logging library with component-based filtering
and pluggable output handlers. Part of the Augra project.

## Features

- Five log levels: Trace, Debug, Info, Warn, Error
- Per-component level overrides
- Handler-based output: stderr, stdout, file, or write your own
- Per-handler level filtering and configurable format strings
- Thread-safe for concurrent use
- C++17, no external dependencies
- Clean CMake integration (FetchContent, add_subdirectory, find_package)

## Quick start

```cpp
#include <augra/log.h>

int main()
{
    augra::log_info("app", "started on port %d", 8080);
    augra::log_warn("net", "connection timeout after %d ms", 5000);

    // Turn up one component without drowning in everything else
    auto& logger = augra::Logger::instance();
    logger.set_component_level("parser", augra::LogLevel::Debug);
    augra::log_debug("parser", "token count: %d", 42);

    return 0;
}
```

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To build with tests and examples:
```bash
cmake -B build -DAUGRA_LOG_BUILD_TESTS=ON -DAUGRA_LOG_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build
```

## Documentation

See the [docs/](docs/) folder for the full documentation:

- [Tutorial](docs/tutorial.md) - getting started walkthrough
- [API Reference](docs/reference.md) - complete API documentation
- [Integration Guide](docs/integration.md) - how to add augra-log to your project

## License

GPL-3.0-or-later. See [LICENSE](LICENSE) for details.

---
This project is developed with tooled assistance, but tested, reviewed and
signed off by a human developer.
