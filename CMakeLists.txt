cmake_minimum_required(VERSION 3.13)

find_package(Git QUIET)
if(GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		OUTPUT_VARIABLE DAMC_GIT_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	string(REGEX REPLACE "^v?([0-9]+\\.[0-9]+(\\.[0-9]+)?).*\$" "\\1" DAMC_GIT_VERSION_TRIMMED ${DAMC_GIT_VERSION})
else()
	set(DAMC_GIT_VERSION "unknown-rev")
	set(DAMC_GIT_VERSION_TRIMMED "0.0.0")
endif()

project(damc VERSION "${DAMC_GIT_VERSION_TRIMMED}" LANGUAGES C CXX)

# option() honors normal variables
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
# Honor visibility properties for all target types.
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

get_filename_component(BUILD_DIR_NAME ${CMAKE_BINARY_DIR} NAME)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_POSITION_INDEPENDENT_CODE True)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_TESTING "Build tests" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type")

find_package(JACK REQUIRED)

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
endif()


if(UNIX AND NOT APPLE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")
endif()


set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "deps")
add_subdirectory(deps)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "damc")

set(LOG_LEVEL_TRACE 0)
set(LOG_LEVEL_DEBUG 1)
set(LOG_LEVEL_INFO 2)
set(LOG_LEVEL_WARN 3)
set(LOG_LEVEL_ERROR 4)
set(LOG_LEVEL_CRITICAL 5)
set(LOG_LEVEL_OFF 6)
add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${LOG_LEVEL_DEBUG})

add_subdirectory(damc_common)
add_subdirectory(damc_audio_processing)
add_subdirectory(damc_server)
add_subdirectory(damc_gui)

install(FILES README.md DESTINATION ./)

# Only install damc_config.json if it is not there already
if(UNIX)
	install(
		FILES
			dist/damc_gui.desktop
			dist/damc_server.desktop
		DESTINATION ./
	)
	install(
		DIRECTORY dist/systemd
		DESTINATION ./
	)
endif()
install(CODE "
	if(NOT EXISTS \${CMAKE_INSTALL_PREFIX}/damc_config.json)
		file(INSTALL \"${CMAKE_CURRENT_SOURCE_DIR}/dist/damc_config.json\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/\")
	else()
		message(STATUS \"Already exists: \${CMAKE_INSTALL_PREFIX}/damc_config.json\")
	endif()
")

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "tools")

add_subdirectory(delayControl)
add_subdirectory(binauralSynthesis)
add_subdirectory(osctools)

if(WIN32)
	set(CPACK_GENERATOR ZIP)
else()
	set(CPACK_GENERATOR TGZ)
endif()

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_STRIP_FILES ON)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_VERSION ${DAMC_GIT_VERSION})
get_cmake_property(CPACK_COMPONENTS_ALL COMPONENTS)
list(REMOVE_ITEM CPACK_COMPONENTS_ALL "deps")


include(CPack)
