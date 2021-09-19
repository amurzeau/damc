cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME damc_gui)

set(FORMS
	Balance.ui
	BalanceController.ui
	CompressorController.ui
	Equalizer.ui
	EqualizerFilters.ui
	EqualizersController.ui
	MainWindow.ui
	OutputInstanceConfigDialog.ui
	OutputController.ui
)

set(HEADERS
	Balance.h
	BalanceController.h
	BodePlot.h
	BodePlotWidget.h
	CompressorController.h
	Equalizer.h
	EqualizersController.h
	FastLabelWidget.h
	LevelMeterWidget.h
	MainWindow.h
	OutputInstanceConfigDialog.h
	OscWidgetArray.h
	OscWidgetMapper.h
	OutputController.h
	WavePlayInterface.h
)

set(SOURCES
	Balance.cpp
	BalanceController.cpp
	BodePlot.cpp
	BodePlotWidget.cpp
	CompressorController.cpp
	Equalizer.cpp
	EqualizersController.cpp
	FastLabelWidget.cpp
	LevelMeterWidget.cpp
	MainWindow.cpp
	OutputInstanceConfigDialog.cpp
	OscWidgetArray.cpp
	OscWidgetMapper.cpp
	OutputController.cpp
	WavePlayInterface.cpp
	main.cpp
)

macro(find_qt OUTPUT_VARIABLE)
	find_package(Qt6 QUIET COMPONENTS Core Gui Widgets Network)
	if(Qt6_FOUND)
		add_library(${OUTPUT_VARIABLE} INTERFACE)
		target_link_libraries(${OUTPUT_VARIABLE} INTERFACE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)
	else()
		find_package(Qt5 QUIET REQUIRED COMPONENTS Core Gui Widgets Network)
		if(Qt5_FOUND)
			add_library(${OUTPUT_VARIABLE} INTERFACE)
			target_link_libraries(${OUTPUT_VARIABLE} INTERFACE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)
			if(NOT DEFINED QT_VERSION_MAJOR)
				set(QT_VERSION_MAJOR 5)
			endif()
		endif()
	endif()
	if(DEFINED QT_VERSION_MAJOR AND NOT DEFINED QT_VERSION)
		set(QT_VERSION ${Qt${QT_VERSION_MAJOR}Widgets_VERSION})
	endif()
endmacro()

find_qt(QtDependencies)

if(TARGET QtDependencies)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTOUIC ON)

	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS} ${FORMS})
	else()
		add_executable(${TARGET_NAME} WIN32 ${SOURCES} ${HEADERS} ${FORMS})
	endif()

	target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
	target_link_libraries(${TARGET_NAME} PRIVATE QtDependencies damc_common)

	install(TARGETS ${TARGET_NAME} DESTINATION ./)

	if(WIN32 AND TARGET Qt${QT_VERSION_MAJOR}::qmake)
		# Install Qt DLLs using windeployqt tool
		get_target_property(_qt_qmake_location Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
		get_filename_component(_qt_bin_dir "${_qt_qmake_location}" DIRECTORY)
		set(QT_WINDEPLOYQT "${_qt_bin_dir}/windeployqt.exe")

		message(STATUS "Install of Qt's DLLs enabled")

		if(EXISTS ${QT_WINDEPLOYQT})
			set(WINDEPLOYQT_EXTRA_OPTIONS)
			if (QT_VERSION VERSION_LESS 6.0.0)
				# These options are not available with Qt 6
				set (WINDEPLOYQT_EXTRA_OPTIONS ${WINDEPLOYQT_EXTRA_OPTIONS} --no-webkit2 --no-angle)
			endif()

			if(QT_VERSION VERSION_GREATER_EQUAL 5.14.0)
				# --no-virtualkeyboard was introduced in Qt 5.14
				set (WINDEPLOYQT_EXTRA_OPTIONS ${WINDEPLOYQT_EXTRA_OPTIONS} --no-virtualkeyboard)
			endif()

			if(QT_VERSION VERSION_LESS 5.14.0)
				if(CMAKE_BUILD_TYPE STREQUAL "Debug")
					set (WINDEPLOYQT_EXTRA_OPTIONS ${WINDEPLOYQT_EXTRA_OPTIONS} --debug)
				else()
					set (WINDEPLOYQT_EXTRA_OPTIONS ${WINDEPLOYQT_EXTRA_OPTIONS} --release)
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
					${WINDEPLOYQT_EXTRA_OPTIONS}
					"$<TARGET_FILE:${TARGET_NAME}>"
				USES_TERMINAL
			)

			# copy deployment directory during installation
			install(
				DIRECTORY
				"${CMAKE_CURRENT_BINARY_DIR}/windeployqt/"
				DESTINATION .
				COMPONENT bin-deps
			)
		endif()
	endif()

endif()
