cmake_minimum_required(VERSION 3.1)
project(waveOverUDP)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

get_filename_component(BUILD_DIR_NAME ${CMAKE_BINARY_DIR} NAME)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_POSITION_INDEPENDENT_CODE True)

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

	message(STATUS "Jack found at ${JACK_INCLUDE_DIR} / ${JACK_LIBRARY}")
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

add_subdirectory(portaudio EXCLUDE_FROM_ALL)
add_subdirectory(libuv EXCLUDE_FROM_ALL)

add_subdirectory(waveSendUDPJack)
add_subdirectory(delayControl)
add_subdirectory(parseWave)
add_subdirectory(zlib EXCLUDE_FROM_ALL)
add_subdirectory(libmysofa EXCLUDE_FROM_ALL)
target_compile_definitions(mysofa PUBLIC VDEBUG)

option (BUILD_TESTS "Build tests" OFF)
set (DISABLE_FORTRAN ON)
set (ENABLE_FLOAT ON)
option(DISABLE_FORTRAN "" ON)
option(ENABLE_FLOAT "" ON)
add_subdirectory(fftw-3.3.8 EXCLUDE_FROM_ALL)

add_subdirectory(binauralSynthesis)
add_subdirectory(gui)

install(TARGETS
	waveSendUDPJack
	waveSendUDPJackGui
	uv
	portaudio
	RUNTIME DESTINATION ./
	LIBRARY DESTINATION ./
)

set(CPACK_GENERATOR ZIP)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

include(CPack)
