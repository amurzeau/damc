cmake_minimum_required(VERSION 3.1)

if(WIN32)
	set(SOURCES
		OutputInstance/WasapiInstance.cpp
		OutputInstance/WasapiInstance.h
	)
else()
	set(SOURCES)
endif()

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
	Filter/ExpanderFilter.h
	Filter/ExpanderFilter.cpp

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

	${SOURCES}
	
	ControlServer.cpp
	ControlServer.h
	ControlClient.cpp
	ControlClient.h
	ControlInterface.cpp
	ControlInterface.h
	json.h
	KeyBinding.h
	KeyBinding.cpp
	OscAddress.h
	OscAddress.cpp
	OscServer.h
	OscServer.cpp
	tinyosc.c
	tinyosc.h
	waveSendUDPJack.cpp
)
target_link_libraries(waveSendUDPJack PUBLIC uv::uv portaudio Jack::Jack)
target_compile_definitions(waveSendUDPJack PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX JSON_SKIP_UNSUPPORTED_COMPILER_CHECK)

if(WIN32)
	target_link_libraries(waveSendUDPJack PUBLIC avrt ksuser)
endif()

install(TARGETS waveSendUDPJack
	RUNTIME DESTINATION ./
	LIBRARY DESTINATION ./
)
