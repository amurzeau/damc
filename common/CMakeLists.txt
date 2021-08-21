cmake_minimum_required(VERSION 3.1)

add_library(common STATIC
	BiquadFilter.cpp
	BiquadFilter.h
	OscAddress.cpp
	OscAddress.h
	OscRoot.cpp
	OscRoot.h
	tinyosc.c
	tinyosc.h
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_LIST_DIR})
