cmake_minimum_required(VERSION 4.1)
project(Quant DESCRIPTION "Performance CPP Experimentation")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(Library)
add_subdirectory(Testing)
