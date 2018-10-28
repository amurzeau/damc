cmake_minimum_required(VERSION 2.8)

add_subdirectory(libuv)

add_executable(waveSendUDPJack
	waveSendUDPJack.cpp
	EqFilter.cpp
	EqFilter.h
	FilteringChain.cpp
	FilteringChain.h
	DitheringFilter.cpp
	DitheringFilter.h
	DelayFilter.cpp
	DelayFilter.h
	ControlServer.cpp
	ControlServer.h
	ControlClient.cpp
	ControlClient.h
	OutputInstance.cpp
	OutputInstance.h
	LoopbackOutputInstance.cpp
	LoopbackOutputInstance.h
	RemoteOutputInstance.cpp
	RemoteOutputInstance.h
	DeviceOutputInstance.cpp
	DeviceOutputInstance.h
	RemoteUdpOutput.cpp
	RemoteUdpOutput.h
	ControlInterface.cpp
	ControlInterface.h
	json.h
)
target_link_libraries(waveSendUDPJack uv)
target_compile_definitions(waveSendUDPJack PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX JSON_SKIP_UNSUPPORTED_COMPILER_CHECK)

if(UNIX)
	target_link_libraries(waveSendUDPJack jack)
else()
	target_link_libraries(waveSendUDPJack "D:/Programmes/Musique/Jack/lib/libjack.lib")
	target_include_directories(waveSendUDPJack PRIVATE "D:/Programmes/Musique/Jack/includes")
endif()
