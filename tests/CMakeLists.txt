# This file is part of the augra-log Project.
# License: GPL-3.0-or-later. Contact: augra-project@trinity2k.net
#

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(augra-log-tests
    test_log.cpp
)
target_link_libraries(augra-log-tests PRIVATE augra::log GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(augra-log-tests)
