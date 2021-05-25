cmake_minimum_required(VERSION 3.4.1)

project(glad LANGUAGES C)

option(BUILD_SHARED_LIBS "Build shared Libraries" ON)

add_library(glad STATIC src/glad.c)
target_include_directories(
    glad PUBLIC "include")

