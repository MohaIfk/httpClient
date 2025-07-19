cmake_minimum_required(VERSION 3.31)
project(HttpClient C)

set(CMAKE_C_STANDARD 11)

add_library(HttpClient STATIC http_client.c)
add_executable(test test.c http_client.c)
