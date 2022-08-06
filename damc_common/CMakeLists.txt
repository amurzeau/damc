cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME damc_common)

add_library(${TARGET_NAME} STATIC
	BiquadFilter.cpp
	BiquadFilter.h
	OscRoot.cpp
	OscRoot.h
	tinyosc.c
	tinyosc.h
	Utils.cpp
	Utils.h

	Osc/OscArray.cpp
	Osc/OscArray.h
	Osc/OscCombinedVariable.cpp
	Osc/OscCombinedVariable.h
	Osc/OscContainer.cpp
	Osc/OscContainer.h
	Osc/OscContainerArray.h
	Osc/OscDynamicVariable.cpp
	Osc/OscDynamicVariable.h
	Osc/OscEndpoint.cpp
	Osc/OscEndpoint.h
	Osc/OscFlatArray.cpp
	Osc/OscFlatArray.h
	Osc/OscGenericArray.h
	Osc/OscNode.cpp
	Osc/OscNode.h
	Osc/OscReadOnlyVariable.cpp
	Osc/OscReadOnlyVariable.h
	Osc/OscVariable.cpp
	Osc/OscVariable.h
)

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(${TARGET_NAME} PRIVATE _USE_MATH_DEFINES)
target_link_libraries(${TARGET_NAME} PUBLIC spdlog::spdlog)
