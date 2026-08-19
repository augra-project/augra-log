# augra-log Integration Guide
**Project:** [augra-log](https://github.com/augra-project/augra-log), a component-based logging library for C++17; **License:** GPL-3.0-or-later

There are several ways to pull augra-log into your build, depending on
whether you use CMake, how you manage dependencies, and whether you want
the build system involved at all. Pick whichever fits your project.

## Contents

- [Building standalone](#building-standalone)
- [FetchContent (recommended)](#fetchcontent-recommended)
- [Git submodule](#git-submodule)
- [Vendored copy](#vendored-copy)
- [Direct compilation (no CMake)](#direct-compilation-no-cmake)
- [System install and find_package](#system-install-and-find_package)
- [vcpkg overlay port](#vcpkg-overlay-port)

## Building standalone

If you just want to build the library, run the tests, or try the demo,
clone and build directly:

```bash
git clone https://github.com/augra-project/augra-log.git
cd augra-log
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Tests and examples are off by default. Turn them on if you want to verify
things work on your toolchain:

```bash
cmake -B build -DAUGRA_LOG_BUILD_TESTS=ON -DAUGRA_LOG_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## FetchContent (recommended)

For most CMake projects, FetchContent is the least friction. CMake
downloads and builds augra-log as part of your project with no manual
steps:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my-app LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    augra-log
    GIT_REPOSITORY https://github.com/augra-project/augra-log.git
    GIT_TAG        v0.1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(augra-log)

add_executable(my-app main.cpp)
target_link_libraries(my-app PRIVATE augra::log)
```

After this, `#include <augra/log.h>` works and the library links
automatically. The `augra::log` alias target handles include paths and
compile flags, so there is nothing else to wire up.

## Git submodule

If you prefer tracking dependencies as submodules, add augra-log to your
tree and use `add_subdirectory`:

```bash
git submodule add https://github.com/augra-project/augra-log.git third-party/augra-log
```

```cmake
cmake_minimum_required(VERSION 3.16)
project(my-app LANGUAGES CXX)

add_subdirectory(third-party/augra-log)

add_executable(my-app main.cpp)
target_link_libraries(my-app PRIVATE augra::log)
```

This gives you the same `augra::log` target with include paths and flags
set up. To pull in updates later, run `git submodule update --remote`.

## Vendored copy

If you want a fully self-contained source tree without submodule machinery,
copy the augra-log directory (or just `include/augra/` and `src/`) into your
project. The `add_subdirectory` approach from above works the same way.

This is a good fit for projects that want to build offline or that ship
their dependencies checked into the repository. You take on the
responsibility of updating the copy manually.

## Direct compilation (no CMake)

augra-log is one header and one source file with no external dependencies,
so you can compile it without a build system at all. Point your compiler at
the include directory, compile `src/log.cpp`, and link with pthreads on
Linux:

```bash
g++ -std=c++17 -Ipath/to/augra-log/include \
    path/to/augra-log/src/log.cpp \
    your_app.cpp \
    -lpthread -o your_app
```

If you prefer a static library you can link against from multiple targets:

```bash
g++ -std=c++17 -Ipath/to/augra-log/include \
    -c path/to/augra-log/src/log.cpp -o augra-log.o
ar rcs libaugra-log.a augra-log.o

g++ -std=c++17 -Ipath/to/augra-log/include \
    your_app.cpp -L. -laughra-log -lpthread -o your_app
```

On Windows with MSVC, the pthread link is not needed since the threading
primitives are built into the runtime:

```
cl /std:c++17 /Ipath\to\augra-log\include ^
    path\to\augra-log\src\log.cpp your_app.cpp /Fe:your_app.exe
```

The only hard requirement is a C++17 compiler. No preprocessor macros to
define, no configuration headers to generate, no codegen step.

## System install and find_package

If you want augra-log available system-wide (or in a prefix), build and
install it first:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /usr/local
```

Then consume it through `find_package` in your project. CMake picks up the
installed config files and creates the `augra::log` target:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my-app LANGUAGES CXX)

find_package(augra-log REQUIRED)

add_executable(my-app main.cpp)
target_link_libraries(my-app PRIVATE augra::log)
```

If you installed to a non-standard prefix, tell CMake where to look:

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/prefix
```

## vcpkg overlay port

Projects that use vcpkg for dependency management can create an overlay port
for augra-log. The port consists of two files in your overlay directory.

`augra-log/portfile.cmake`:

```cmake
vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/augra-project/augra-log.git
    REF v0.1.0
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME augra-log)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
```

`augra-log/vcpkg.json`:

```json
{
    "name": "augra-log",
    "version": "0.1.0",
    "description": "Component-based logging library",
    "license": "GPL-3.0-or-later",
    "dependencies": [
        { "name": "vcpkg-cmake", "host": true },
        { "name": "vcpkg-cmake-config", "host": true }
    ]
}
```

With the overlay in place, `vcpkg install augra-log` builds and installs
it. This is how dosbox-automation uses augra-log alongside its other vcpkg
dependencies.

---

## Changelog

- **0.1.0** (2026-08-19) - Initial version. Covers FetchContent,
  add_subdirectory, git submodule, vendored copy, direct compilation,
  system install, and vcpkg overlay port.
