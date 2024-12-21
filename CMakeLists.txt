cmake_minimum_required(VERSION 3.28)
project(Nexus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS  -fconcepts-diagnostics-depth=10)
add_executable(Nexus main.cpp
        include/memory.h
        src/memory.tpp
        include/check.h
)
