﻿# CMakeList.txt : CMake project for mtask, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source to this project's executable.
add_executable (mtask "tests.cpp" "mtask.h")

# TODO: Add tests and install targets if needed.
