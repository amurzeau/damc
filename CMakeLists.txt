cmake_minimum_required(VERSION 2.8)
project(waveOverUDP)

get_filename_component(BUILD_DIR_NAME ${CMAKE_BINARY_DIR} NAME)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/../${BUILD_DIR_NAME}-bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/../${BUILD_DIR_NAME}-bin")

set(CMAKE_POSITION_INDEPENDENT_CODE False)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fvisibility=hidden")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -Wall -fvisibility=hidden")

add_executable(wavePlayUDP wavePlayUDP.cpp filter.h Logger.cpp Logger.h)
target_link_libraries(wavePlayUDP asound)

add_executable(waveSendUDP waveSendUDP.cpp Logger.cpp Logger.h)
target_link_libraries(waveSendUDP asound)

add_executable(waveSendUDPPulse waveSendUDPPulse.cpp Logger.cpp Logger.h)
target_link_libraries(waveSendUDPPulse pulse-simple)

add_executable(waveSendUDPJack
	waveSendUDPJack.cpp
	Logger.cpp
	Logger.h
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
	RemoteUdpOutput.cpp
	RemoteUdpOutput.h
	ControlInterface.cpp
	ControlInterface.h
)
target_link_libraries(waveSendUDPJack jack uv)

add_executable(wavePlayUDPJack wavePlayUDPJack.cpp Logger.cpp Logger.h)
target_link_libraries(wavePlayUDPJack jack)
