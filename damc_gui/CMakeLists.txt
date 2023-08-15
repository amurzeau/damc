cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME damc_gui)

file(GLOB FORMS *.ui)
file(GLOB HEADERS *.h)
file(GLOB SOURCES *.cpp)

macro(find_qt OUTPUT_VARIABLE)
	find_package(QT NAMES Qt6 Qt5 COMPONENTS Core)
	if(QT_FOUND)
		find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Core Gui Widgets Network SerialPort REQUIRED)

		add_library(${OUTPUT_VARIABLE} INTERFACE)
		target_link_libraries(${OUTPUT_VARIABLE} INTERFACE
			Qt${QT_VERSION_MAJOR}::Core
			Qt${QT_VERSION_MAJOR}::Gui
			Qt${QT_VERSION_MAJOR}::Widgets
			Qt${QT_VERSION_MAJOR}::Network
			Qt${QT_VERSION_MAJOR}::SerialPort
		)
	endif()
endmacro()

find_qt(QtDependencies)

if(TARGET QtDependencies)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTOUIC ON)

	message(STATUS "Using Qt ${QT_VERSION}")

	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS} ${FORMS})
	else()
		add_executable(${TARGET_NAME} WIN32 ${SOURCES} ${HEADERS} ${FORMS})
	endif()

	target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
	target_link_libraries(${TARGET_NAME} PRIVATE QtDependencies damc_common)
	target_compile_definitions(${TARGET_NAME} PRIVATE _USE_MATH_DEFINES)

	install(
		TARGETS
			${TARGET_NAME}
			spdlog
		RUNTIME DESTINATION ./
		LIBRARY DESTINATION lib/)

	if(MINGW)
		include(InstallRequiredSystemLibraries)
	endif()

	if(WIN32 AND TARGET Qt${QT_VERSION_MAJOR}::qmake)
		# Install Qt DLLs using windeployqt tool
		get_target_property(_qt_qmake_location Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
		get_filename_component(_qt_bin_dir "${_qt_qmake_location}" DIRECTORY)
		set(QT_WINDEPLOYQT "${_qt_bin_dir}/windeployqt.exe")

		message(STATUS "Install of Qt's DLLs enabled")

		if(EXISTS ${QT_WINDEPLOYQT})
			set(WINDEPLOYQT_EXTRA_OPTIONS)

			if(QT_VERSION VERSION_LESS 6.0.0)
				# These options are not available with Qt 6
				set(WINDEPLOYQT_EXTRA_OPTIONS ${WINDEPLOYQT_EXTRA_OPTIONS} --no-webkit2 --no-angle)
			endif()

			if(QT_VERSION VERSION_GREATER_EQUAL 5.14.0 AND QT_VERSION VERSION_LESS 6.5.0)
				# --no-virtualkeyboard was introduced in Qt 5.14 and removed in 6.5
				set(WINDEPLOYQT_EXTRA_OPTIONS ${WINDEPLOYQT_EXTRA_OPTIONS} --no-virtualkeyboard)
			endif()

			if(QT_VERSION VERSION_LESS 5.14.0)
				if(CMAKE_BUILD_TYPE STREQUAL "Debug")
					set(WINDEPLOYQT_EXTRA_OPTIONS ${WINDEPLOYQT_EXTRA_OPTIONS} --debug)
				else()
					set(WINDEPLOYQT_EXTRA_OPTIONS ${WINDEPLOYQT_EXTRA_OPTIONS} --release)
				endif()
			endif()

			# execute windeployqt in a tmp directory after build
			add_custom_command(TARGET ${TARGET_NAME}
				POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
				COMMAND set PATH=${_qt_bin_dir}$<SEMICOLON>%PATH%
				COMMAND "${QT_WINDEPLOYQT}"
					--dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
					--no-quick-import
					--no-translations
					--no-compiler-runtime
					--no-opengl-sw
					--no-svg
					--no-system-d3d-compiler
					${WINDEPLOYQT_EXTRA_OPTIONS}
					"$<TARGET_FILE:${TARGET_NAME}>"
				USES_TERMINAL
			)

			# copy deployment directory during installation
			install(
				DIRECTORY
				"${CMAKE_CURRENT_BINARY_DIR}/windeployqt/"
				DESTINATION .
			)
		endif()
	endif()
endif()
