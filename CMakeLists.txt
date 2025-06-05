cmake_minimum_required(VERSION 3.10)
project(DockerWebProxy)

set(CMAKE_CXX_STANDARD 20)

add_executable(DockerWebProxy main.cpp)

add_executable(split_tests
    tests/split_tests.cpp
)
