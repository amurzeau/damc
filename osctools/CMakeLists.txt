cmake_minimum_required(VERSION 3.1)

add_executable(oscrecv oscrecv.cpp)
target_link_libraries(oscrecv PUBLIC common uv::uv)
