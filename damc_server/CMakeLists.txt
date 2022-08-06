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
target_link_libraries(${TARGET_NAME} PUBLIC uv::uv PortAudio JACK::jack damc_common Threads::Threads spdlog::spdlog damc_audio_processing)
target_compile_definitions(${TARGET_NAME} PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX JSON_SKIP_UNSUPPORTED_COMPILER_CHECK)

# Version define only for main.cpp to avoid recompiling all the source on version change
set_property(SOURCE main.cpp APPEND PROPERTY COMPILE_DEFINITIONS DAMC_VERSION=\"${DAMC_GIT_VERSION}\")

if(WIN32)
	target_link_libraries(${TARGET_NAME} PUBLIC avrt ksuser)
endif()

install(TARGETS
	${TARGET_NAME}
	uv
	PortAudio
	spdlog
	RUNTIME DESTINATION ./
	LIBRARY DESTINATION lib/
)

if(MINGW)
	include(InstallRequiredSystemLibraries)
endif()
