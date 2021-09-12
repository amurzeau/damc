cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME damc_server)

find_package(Threads REQUIRED)

if(WIN32)
	set(SOURCES
		OutputInstance/WasapiInstance.cpp
		OutputInstance/WasapiInstance.h
	)
else()
	set(SOURCES)
endif()

add_executable(${TARGET_NAME}
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

	ControlInterface.cpp
	ControlInterface.h
	json.h
	KeyBinding.h
	KeyBinding.cpp
	OscTcpClient.h
	OscTcpClient.cpp
	OscTcpServer.h
	OscTcpServer.cpp
	OscServer.h
	OscServer.cpp
	OscStatePersist.h
	OscStatePersist.cpp
	main.cpp
)
target_link_libraries(${TARGET_NAME} PUBLIC uv::uv PortAudio JACK::jack damc_common Threads::Threads)
target_compile_definitions(${TARGET_NAME} PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX JSON_SKIP_UNSUPPORTED_COMPILER_CHECK)

if(WIN32)
	target_link_libraries(${TARGET_NAME} PUBLIC avrt ksuser)
endif()

install(TARGETS ${TARGET_NAME}
	RUNTIME DESTINATION ./
	LIBRARY DESTINATION ./
)
