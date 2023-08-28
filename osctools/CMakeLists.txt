cmake_minimum_required(VERSION 3.13)

add_executable(oscrecv oscrecv.cpp)
target_link_libraries(oscrecv PUBLIC damc_common uv::uv)
