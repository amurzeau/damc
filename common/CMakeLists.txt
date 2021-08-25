cmake_minimum_required(VERSION 3.1)

add_library(common STATIC
	BiquadFilter.cpp
	BiquadFilter.h
	OscAddress.h
	OscRoot.cpp
	OscRoot.h
	tinyosc.c
	tinyosc.h

	Osc/OscArray.h
	Osc/OscContainer.cpp
	Osc/OscContainer.h
	Osc/OscContainerArray.h
	Osc/OscEndpoint.cpp
	Osc/OscEndpoint.h
	Osc/OscFlatArray.h
	Osc/OscGenericArray.h
	Osc/OscNode.cpp
	Osc/OscNode.h
	Osc/OscReadOnlyVariable.h
	Osc/OscVariable.h
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_LIST_DIR})
