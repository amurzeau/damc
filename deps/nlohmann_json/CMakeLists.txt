cmake_minimum_required(VERSION 3.13)

add_library(nlohmann_json INTERFACE json.h)
target_include_directories(nlohmann_json INTERFACE .)
