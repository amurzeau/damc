cmake_minimum_required(VERSION 3.1)
project(uv)

add_subdirectory(src)
if(BUILD_TESTS)
  add_subdirectory(test)
endif()
