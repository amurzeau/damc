cmake_minimum_required(VERSION 3.13)

add_subdirectory(portaudio EXCLUDE_FROM_ALL)
add_subdirectory(libuv EXCLUDE_FROM_ALL)
add_subdirectory(zlib EXCLUDE_FROM_ALL)
add_subdirectory(libmysofa EXCLUDE_FROM_ALL)

set (DISABLE_FORTRAN ON)
set (ENABLE_FLOAT ON)
add_subdirectory(fftw-3.3.8 EXCLUDE_FROM_ALL)

target_compile_definitions(mysofa PUBLIC VDEBUG)

if(BUILD_SHARED_LIBS)
	add_library(uv::uv ALIAS uv)
	install(TARGETS
		uv
		RUNTIME DESTINATION ./
		LIBRARY DESTINATION ./
	)
else()
	add_library(uv::uv ALIAS uv_a)
endif()

install(TARGETS
	portaudio
	zlib
	mysofa
	fftw3f
	RUNTIME DESTINATION ./
	LIBRARY DESTINATION ./
)
