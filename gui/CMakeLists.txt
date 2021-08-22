cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME waveSendUDPJackGui)

set(FORMS
	MainWindow.ui
	OutputController.ui
	EqualizerFilters.ui
	Equalizer.ui
	EqualizersController.ui
	NewOutputInstanceDialog.ui
	CompressorController.ui
	BalanceController.ui
	Balance.ui
)

set(HEADERS
	MainWindow.h
	BodePlot.h
	OutputController.h
	WavePlayInterface.h
	WavePlayOutputInterface.h
	EqualizersController.h
	Equalizer.h
	NewOutputInstanceDialog.h
	CompressorController.h
	BalanceController.h
	Balance.h
	LevelMeterWidget.h
	FastLabelWidget.h
	BodePlotWidget.h
)

set(SOURCES
	main.cpp
	MainWindow.cpp
	BodePlot.cpp
	OutputController.cpp
	WavePlayInterface.cpp
	WavePlayOutputInterface.cpp
	EqualizersController.cpp
	Equalizer.cpp
	NewOutputInstanceDialog.cpp
	CompressorController.cpp
	BalanceController.cpp
	Balance.cpp
	LevelMeterWidget.cpp
	FastLabelWidget.cpp
	BodePlotWidget.cpp
)

find_package(Qt5 COMPONENTS Core Gui Widgets Network)
if(Qt5_FOUND)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS} ${FORMS})
else()
	add_executable(${TARGET_NAME} WIN32 ${SOURCES} ${HEADERS} ${FORMS})
endif()
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

if(Qt5_FOUND)
	target_link_libraries(${TARGET_NAME} PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)
else()
	target_link_libraries(${TARGET_NAME} PRIVATE Qt4::QtCore Qt4::QtGui Qt4::QtNetwork)
	target_compile_options(${TARGET_NAME} PRIVATE -Zc:wchar_t-)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE common)

install(TARGETS ${TARGET_NAME} DESTINATION ./)

if(WIN32)
	# Install Qt DLLs using windeployqt tool
	get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)
	get_filename_component(_qt_bin_dir "${_qt5_qmake_location}" DIRECTORY)
	set(QT5_WINDEPLOYQT "${_qt_bin_dir}/windeployqt.exe")

	if(EXISTS ${QT5_WINDEPLOYQT})
		if (QT_VERSION_MAJOR MATCHES "5")
			# These options are not available with Qt6
			set (WINDEPLOYQT_EXTRA_OPTIONS --no-webkit2 --no-angle)
		endif ()

		# execute windeployqt in a tmp directory after build
		add_custom_command(TARGET ${TARGET_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
			COMMAND set PATH=${_qt_bin_dir}$<SEMICOLON>%PATH%
			COMMAND "${QT5_WINDEPLOYQT}"
				--dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
				--no-quick-import
				--no-translations
				--no-compiler-runtime
				--no-opengl-sw
				--no-virtualkeyboard
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
