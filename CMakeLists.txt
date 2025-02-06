# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ethernet_basic)

# Define the macros
add_definitions(-DCMAKE_SERIAL_NO=\"J4221-0001\")
add_definitions(-DCMAKE_DEFAULT_PASS=\"0123456789\")