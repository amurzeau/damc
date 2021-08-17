cmake_minimum_required(VERSION 3.13)
project(waveOverUDP)

# option() honors normal variables
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

get_filename_component(BUILD_DIR_NAME ${CMAKE_BINARY_DIR} NAME)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_POSITION_INDEPENDENT_CODE True)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type")

find_package(Jack REQUIRED)

if(MINGW)
	set(COMPILER_FLAGS "-msse -mfpmath=sse -ffast-math -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_FLAGS}")

	get_filename_component( MINGW_PATH ${CMAKE_CXX_COMPILER} PATH )

	macro(list_append_if_exists LIST_NAME FILE_NAME)
		if(EXISTS ${FILE_NAME})
			list(APPEND ${LIST_NAME} ${FILE_NAME})
		endif()
	endmacro()

	set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
	list_append_if_exists(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${MINGW_PATH}/libgcc_s_dw2-1.dll)
	list_append_if_exists(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${MINGW_PATH}/libgcc_s_seh-1.dll)
	list_append_if_exists(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${MINGW_PATH}/libgcc_s_sjlj-1.dll)
	list_append_if_exists(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${MINGW_PATH}/libstdc++-6.dll)
	list_append_if_exists(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${MINGW_PATH}/libwinpthread-1.dll)

	message( STATUS "Installing system-libraries: ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}" )

	set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ./)
	include(InstallRequiredSystemLibraries)
endif()

if(UNIX)
	find_package(ALSA)
	find_package(PulseAudio)

	if(ALSA_FOUND)
		add_executable(wavePlayUDP wavePlayUDP.cpp filter.h filter_fir2.h Logger.cpp Logger.h)
		target_link_libraries(wavePlayUDP ALSA::ALSA)

		add_executable(waveSendUDP waveSendUDP.cpp Logger.cpp Logger.h)
		target_link_libraries(waveSendUDP ALSA::ALSA)
	endif()

	if(PULSEAUDIO_FOUND)
		add_executable(waveSendUDPPulse waveSendUDPPulse.cpp Logger.cpp Logger.h)
		target_link_libraries(waveSendUDPPulse PulseAudio::PulseAudio)
	endif()

	add_executable(wavePlayUDPJack wavePlayUDPJack.cpp Logger.cpp Logger.h)
	target_link_libraries(wavePlayUDPJack Jack::Jack)
endif()


set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "deps")
add_subdirectory(deps)

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "waveSendUDPJack")

add_subdirectory(waveSendUDPJack)
add_subdirectory(gui)

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "tools")

add_subdirectory(delayControl)
add_subdirectory(parseWave)
add_subdirectory(binauralSynthesis)

set(CPACK_GENERATOR ZIP)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

include(CPack)
