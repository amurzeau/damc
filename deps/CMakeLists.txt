cmake_minimum_required(VERSION 3.13)

set(JACK OFF)
set(PA_USE_JACK OFF)
add_subdirectory(portaudio EXCLUDE_FROM_ALL)
add_subdirectory(libuv EXCLUDE_FROM_ALL)
add_subdirectory(zlib EXCLUDE_FROM_ALL)
add_subdirectory(libmysofa EXCLUDE_FROM_ALL)

set(DISABLE_FORTRAN ON)
set(ENABLE_FLOAT ON)
set(BUILD_TESTS ${BUILD_TESTING})
add_subdirectory(fftw-3.3.8 EXCLUDE_FROM_ALL)

target_compile_definitions(PortAudio PRIVATE PA_WASAPI_MAX_CONST_DEVICE_COUNT=128)
target_compile_definitions(mysofa PUBLIC VDEBUG)

if(BUILD_SHARED_LIBS)
	add_library(uv::uv ALIAS uv)
	install(TARGETS
		uv
		RUNTIME DESTINATION ./
	)
else()
	add_library(uv::uv ALIAS uv_a)
endif()

set_target_properties(PortAudio PROPERTIES PUBLIC_HEADER "")
install(TARGETS
	PortAudio
	zlib
	mysofa
	fftw3f
	RUNTIME DESTINATION ./
)
