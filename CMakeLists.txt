cmake_minimum_required(VERSION 3.1)
project(waveOverUDP)

set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

get_filename_component(BUILD_DIR_NAME ${CMAKE_BINARY_DIR} NAME)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_POSITION_INDEPENDENT_CODE True)

if(MINGW)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fvisibility=hidden")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
	#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -Wall -fvisibility=hidden")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -mfpmath=sse -ffast-math -Wall")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -mfpmath=sse -ffast-math -Wall")
endif()

if(WIN32)
	find_library(JACK_LIBRARY
		NAMES jack libjack libjack.lib
		NAMES_PER_DIR
		PATHS "C:/Program Files (x86)/JACK2/lib" "D:/Programmes/Musique/Jack/lib"
	)

	find_file(JACK_INCLUDE_FILE
		NAMES jack.h
		PATHS jack/ "C:/Program Files (x86)/JACK2/include/jack" "D:/Programmes/Musique/Jack/includes/jack"
	)

	if(JACK_INCLUDE_FILE)
		get_filename_component(JACK_BASE_INCLUDE_DIR ${JACK_INCLUDE_FILE} DIRECTORY)
		set(JACK_INCLUDE_DIR ${JACK_BASE_INCLUDE_DIR}/..)
	endif()

	message(STATUS "Jack found at ${JACK_INCLUDE_DIR} / ${JACK_LIBRARY} / ${JACK_LIBRARY_DLL}")
	add_library(jack SHARED IMPORTED GLOBAL)
	target_include_directories(jack INTERFACE ${JACK_INCLUDE_DIR})
	set_property(TARGET jack PROPERTY IMPORTED_LOCATION ${JACK_LIBRARY})
	set_property(TARGET jack PROPERTY IMPORTED_IMPLIB ${JACK_LIBRARY})
endif()

if(UNIX)
	add_executable(wavePlayUDP wavePlayUDP.cpp filter.h filter_fir2.h Logger.cpp Logger.h)
	target_link_libraries(wavePlayUDP asound)

	add_executable(waveSendUDP waveSendUDP.cpp Logger.cpp Logger.h)
	target_link_libraries(waveSendUDP asound)

	add_executable(waveSendUDPPulse waveSendUDPPulse.cpp Logger.cpp Logger.h)
	target_link_libraries(waveSendUDPPulse pulse-simple)

	add_executable(wavePlayUDPJack wavePlayUDPJack.cpp Logger.cpp Logger.h)
	target_link_libraries(wavePlayUDPJack jack)
endif()

add_subdirectory(portaudio)
add_subdirectory(libuv)

add_subdirectory(waveSendUDPJack)
add_subdirectory(delayControl)
add_subdirectory(parseWave)
add_subdirectory(zlib)
add_subdirectory(libmysofa)
target_compile_definitions(mysofa PUBLIC VDEBUG)

option (BUILD_TESTS "Build tests" OFF)
set (DISABLE_FORTRAN ON)
set (ENABLE_FLOAT ON)
add_subdirectory(fftw-3.3.8)

add_subdirectory(binauralSynthesis)
add_subdirectory(gui)
