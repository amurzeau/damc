cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME damc_audio_processing)

add_library(${TARGET_NAME} STATIC
	EqFilter.cpp
	EqFilter.h
	DitheringFilter.cpp
	DitheringFilter.h
	FilteringChain.cpp
	FilteringChain.h
	DelayFilter.cpp
	DelayFilter.h
	ReverbFilter.cpp
	ReverbFilter.h
	CompressorFilter.cpp
	CompressorFilter.h
	ExpanderFilter.cpp
	ExpanderFilter.h
	PeakMeter.cpp
	PeakMeter.h
)
target_link_libraries(${TARGET_NAME} PUBLIC  damc_common spdlog::spdlog)
target_compile_definitions(${TARGET_NAME} PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)
target_include_directories(${TARGET_NAME} PUBLIC .)

install(TARGETS
	${TARGET_NAME}
	RUNTIME DESTINATION ./
	LIBRARY DESTINATION lib/
)
