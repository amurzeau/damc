cmake_minimum_required(VERSION 2.8)

add_executable(waveSendUDPJack
	Filter/EqFilter.cpp
	Filter/EqFilter.h
	Filter/DitheringFilter.cpp
	Filter/DitheringFilter.h
	Filter/FilteringChain.cpp
	Filter/FilteringChain.h
	Filter/DelayFilter.cpp
	Filter/DelayFilter.h
	Filter/ReverbFilter.cpp
	Filter/ReverbFilter.h
	Filter/CompressorFilter.h
	Filter/CompressorFilter.cpp

	OutputInstance/IAudioEndpoint.h
	OutputInstance/OutputInstance.cpp
	OutputInstance/OutputInstance.h
	OutputInstance/LoopbackOutputInstance.cpp
	OutputInstance/LoopbackOutputInstance.h
	OutputInstance/RemoteOutputInstance.cpp
	OutputInstance/RemoteOutputInstance.h
	OutputInstance/RemoteInputInstance.cpp
	OutputInstance/RemoteInputInstance.h
	OutputInstance/DeviceInputInstance.cpp
	OutputInstance/DeviceInputInstance.h
	OutputInstance/DeviceOutputInstance.cpp
	OutputInstance/DeviceOutputInstance.h
	OutputInstance/RemoteUdpOutput.cpp
	OutputInstance/RemoteUdpOutput.h
	OutputInstance/RemoteUdpInput.cpp
	OutputInstance/RemoteUdpInput.h
	OutputInstance/ResamplingFilter.cpp
	OutputInstance/ResamplingFilter_coefs.cpp
	OutputInstance/ResamplingFilter.h
	
	ControlServer.cpp
	ControlServer.h
	ControlClient.cpp
	ControlClient.h
	ControlInterface.cpp
	ControlInterface.h
	json.h
	waveSendUDPJack.cpp
)
target_link_libraries(waveSendUDPJack uv portaudio)
target_compile_definitions(waveSendUDPJack PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX JSON_SKIP_UNSUPPORTED_COMPILER_CHECK)

if(UNIX)
	target_link_libraries(waveSendUDPJack jack)
else()
	target_link_libraries(waveSendUDPJack "D:/Programmes/Musique/Jack/lib/libjack.lib")
	target_include_directories(waveSendUDPJack PRIVATE "D:/Programmes/Musique/Jack/includes")
endif()
