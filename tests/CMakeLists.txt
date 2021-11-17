cmake_minimum_required(VERSION 3.15)

project(${CMAKE_PROJECT_NAME}-tests 
VERSION 1.0.0
LANGUAGES C CXX)

add_executable(basic_test "basic/main.cpp")
target_link_libraries("basic_test" PRIVATE ${CMAKE_PROJECT_NAME})
