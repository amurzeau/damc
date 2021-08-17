cmake_minimum_required(VERSION 3.1)
project(waveOverUDP)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

get_filename_component(BUILD_DIR_NAME ${CMAKE_BINARY_DIR} NAME)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_POSITION_INDEPENDENT_CODE True)

find_package(Jack REQUIRED)

if(MINGW)
	set(COMPILER_FLAGS "-msse -mfpmath=sse -ffast-math -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_FLAGS}")

	# set(LINKER_FLAGS "-static-libgcc -static-libstdc++")
	# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
	# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
	# set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} ${LINKER_FLAGS}")
	# set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")
	message( STATUS "Installing system-libraries: MinGW DLLs." )
	get_filename_component( MINGW_PATH ${CMAKE_CXX_COMPILER} PATH )

	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
			${MINGW_PATH}/libgcc_s_dw2-1.dll
			${MINGW_PATH}/libstdc++-6.dll
			${MINGW_PATH}/libwinpthread-1.dll
		)
	else()
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
			${MINGW_PATH}/libgcc_s_seh-1.dll
			${MINGW_PATH}/libstdc++-6.dll
			${MINGW_PATH}/libwinpthread-1.dll
		)
	endif()

	set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ./)
	include(InstallRequiredSystemLibraries)
endif()

if(UNIX)
	add_executable(wavePlayUDP wavePlayUDP.cpp filter.h filter_fir2.h Logger.cpp Logger.h)
	target_link_libraries(wavePlayUDP asound)

	add_executable(waveSendUDP waveSendUDP.cpp Logger.cpp Logger.h)
	target_link_libraries(waveSendUDP asound)

	# add_executable(waveSendUDPPulse waveSendUDPPulse.cpp Logger.cpp Logger.h)
	# target_link_libraries(waveSendUDPPulse pulse-simple)

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
