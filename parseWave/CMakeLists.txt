cmake_minimum_required(VERSION 2.8)

add_executable(parseWave
	parsewav.cpp
	../waveSendUDPJack/OutputInstance/ResamplingFilter.cpp
	../waveSendUDPJack/OutputInstance/ResamplingFilter_coefs.cpp
	../waveSendUDPJack/OutputInstance/ResamplingFilter.h
)
target_compile_definitions(parseWave PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)
