# Integration Guide

This document covers the different ways to add augra-log to your project.

## Building standalone

Clone the repository and build:

```bash
git clone https://gitlab.com/the-augra-project/augra-log.git
cd augra-log
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To run the tests and examples as well:

```bash
cmake -B build -DAUGRA_LOG_BUILD_TESTS=ON -DAUGRA_LOG_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## FetchContent (recommended)

The simplest way to use augra-log in a CMake project. CMake downloads and
builds it as part of your project:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my-app LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    augra-log
    GIT_REPOSITORY https://gitlab.com/the-augra-project/augra-log.git
    GIT_TAG        v0.1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(augra-log)

add_executable(my-app main.cpp)
target_link_libraries(my-app PRIVATE augra::log)
```

That gives you `#include <augra/log.h>` and links the library. Nothing
else to configure.

## add_subdirectory

If you vendor augra-log into your source tree (as a git submodule or a
plain copy), use `add_subdirectory` instead:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my-app LANGUAGES CXX)

add_subdirectory(third-party/augra-log)

add_executable(my-app main.cpp)
target_link_libraries(my-app PRIVATE augra::log)
```

The include paths and compile flags are set automatically through the
`augra::log` target.

## System install and find_package

Build and install the library to a system prefix:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /usr/local
```

Then in your project:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my-app LANGUAGES CXX)

find_package(augra-log REQUIRED)

add_executable(my-app main.cpp)
target_link_libraries(my-app PRIVATE augra::log)
```

If you installed to a non-standard prefix, point CMake at it:

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/prefix
```

## vcpkg overlay port

For projects that use vcpkg, you can create an overlay port. Place this
in your vcpkg overlay directory as `augra-log/portfile.cmake`:

```cmake
vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://gitlab.com/the-augra-project/augra-log.git
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

And `augra-log/vcpkg.json`:

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

This is how dosbox-automation consumes augra-log alongside its other
vcpkg dependencies.
