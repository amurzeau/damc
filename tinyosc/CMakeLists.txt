cmake_minimum_required(VERSION 3.1)

add_library(tinyosc STATIC tinyosc.c tinyosc.h)
target_include_directories(tinyosc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
