cmake_minimum_required(VERSION 3.13)

set(JACK OFF)
set(PA_USE_JACK OFF)
set(DEBUG_OUTPUT ON)
set(PA_ENABLE_DEBUG_OUTPUT ON)
if(WIN32)
	option(PA_USE_ASIO "Enable support for ASIO" ON)
endif()
add_subdirectory(portaudio EXCLUDE_FROM_ALL)
add_subdirectory(libuv EXCLUDE_FROM_ALL)
add_subdirectory(zlib EXCLUDE_FROM_ALL)
add_subdirectory(libmysofa EXCLUDE_FROM_ALL)

set(DISABLE_FORTRAN ON)
set(ENABLE_FLOAT ON)
set(BUILD_TESTS ${BUILD_TESTING})
add_subdirectory(fftw-3.3.8 EXCLUDE_FROM_ALL)

add_subdirectory(spdlog EXCLUDE_FROM_ALL)

target_compile_definitions(PortAudio PRIVATE PA_WASAPI_MAX_CONST_DEVICE_COUNT=128 PA_LOG_API_CALLS)
target_compile_definitions(mysofa PUBLIC VDEBUG)

add_library(uv::uv ALIAS uv)
set_target_properties(PortAudio PROPERTIES PUBLIC_HEADER "")
