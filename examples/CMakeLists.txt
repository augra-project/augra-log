# This file is part of the augra-log Project.
# License: GPL-3.0-or-later. Contact: augra-project@trinity2k.net
#

add_executable(augra-log-demo demo.cpp)
target_link_libraries(augra-log-demo PRIVATE augra::log)
