cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME damc_server)

find_package(Threads REQUIRED)

if(WIN32)
	set(SOURCES
		ChannelStrip/WasapiInstance.cpp
		ChannelStrip/WasapiInstance.h
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
	Filter/CompressorFilter.cpp
	Filter/CompressorFilter.h
	Filter/ExpanderFilter.cpp
	Filter/ExpanderFilter.h
	Filter/PeakMeter.cpp
	Filter/PeakMeter.h

	ChannelStrip/IAudioEndpoint.h
	ChannelStrip/ChannelStrip.cpp
	ChannelStrip/ChannelStrip.h
	ChannelStrip/LoopbackOutputInstance.cpp
	ChannelStrip/LoopbackOutputInstance.h
	ChannelStrip/RemoteOutputInstance.cpp
	ChannelStrip/RemoteOutputInstance.h
	ChannelStrip/RemoteInputInstance.cpp
	ChannelStrip/RemoteInputInstance.h
	ChannelStrip/DeviceInputInstance.cpp
	ChannelStrip/DeviceInputInstance.h
	ChannelStrip/DeviceOutputInstance.cpp
	ChannelStrip/DeviceOutputInstance.h
	ChannelStrip/SampleRateMeasure.cpp
	ChannelStrip/SampleRateMeasure.h
	ChannelStrip/RemoteUdpOutput.cpp
	ChannelStrip/RemoteUdpOutput.h
	ChannelStrip/RemoteUdpInput.cpp
	ChannelStrip/RemoteUdpInput.h
	ChannelStrip/ResamplingFilter.cpp
	ChannelStrip/ResamplingFilter_coefs.cpp
	ChannelStrip/ResamplingFilter.h

	${SOURCES}

	ControlInterface.cpp
	ControlInterface.h
	json.h
	JackPortAutoConnect.cpp
	JackPortAutoConnect.h
	JackUtils.cpp
	JackUtils.h
	KeyBinding.cpp
	KeyBinding.h
	OscTcpClient.cpp
	OscTcpClient.h
	OscTcpServer.cpp
	OscTcpServer.h
	OscServer.cpp
	OscServer.h
	OscStatePersist.cpp
	OscStatePersist.h
	main.cpp
)
target_link_libraries(${TARGET_NAME} PUBLIC uv::uv PortAudio JACK::jack damc_common Threads::Threads spdlog::spdlog)
target_compile_definitions(${TARGET_NAME} PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX JSON_SKIP_UNSUPPORTED_COMPILER_CHECK)

# Version define only for main.cpp to avoid recompiling all the source on version change
set_property(SOURCE main.cpp APPEND PROPERTY COMPILE_DEFINITIONS DAMC_VERSION=\"${PROJECT_VERSION}\")

if(WIN32)
	target_link_libraries(${TARGET_NAME} PUBLIC avrt ksuser)
endif()

install(TARGETS ${TARGET_NAME}
	RUNTIME DESTINATION ./
	LIBRARY DESTINATION ./
)
