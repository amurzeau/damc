cmake_minimum_required(VERSION 3.1)

find_path(Valgrind_INCLUDE_DIR
	NAMES valgrind/callgrind.h
)

add_executable(parseWave
	parsewav.cpp
	../waveSendUDPJack/OutputInstance/ResamplingFilter.cpp
	../waveSendUDPJack/OutputInstance/ResamplingFilter_coefs.cpp
	../waveSendUDPJack/OutputInstance/ResamplingFilter.h
)
target_compile_definitions(parseWave PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)
if(Valgrind_INCLUDE_DIR)
	message(STATUS "Found valgrind in ${Valgrind_INCLUDE_DIR}")
	target_compile_definitions(parseWave PRIVATE HAVE_VALGRIND)
endif()
