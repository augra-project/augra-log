# This file is part of the augra-log Project.
# License: GPL-3.0-or-later. Contact: augra-project@trinity2k.net
#

cmake_minimum_required(VERSION 3.16)
project(augra-log
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Component-based logging library"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)

# Library target
add_library(augra-log
    src/log.cpp
)
target_include_directories(augra-log
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
add_library(augra::log ALIAS augra-log)

# Install rules
include(GNUInstallDirs)
install(TARGETS augra-log
    EXPORT augra-logTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/augra
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT augra-logTargets
    NAMESPACE augra::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/augra-log
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/augra-logConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/augra-logConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/augra-log
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/augra-logConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/augra-logConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/augra-logConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/augra-log
)

# Optional targets
option(AUGRA_LOG_BUILD_TESTS "Build unit tests" OFF)
option(AUGRA_LOG_BUILD_EXAMPLES "Build example programs" OFF)

if(AUGRA_LOG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(AUGRA_LOG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
