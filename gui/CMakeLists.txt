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
	../waveSendUDPJack/Filter/EqFilter.h
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
	../waveSendUDPJack/Filter/EqFilter.cpp
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

#find_package(Qwt REQUIRED)

find_package(Qt5 QUIET REQUIRED COMPONENTS Core Gui Widgets Network)
if(NOT Qt5_FOUND)
	message(STATUS "Using Qt4")
	find_package(Qt4 4.8 REQUIRED QtCore QtGui QtNetwork)
else()
	message(STATUS "Using Qt5")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS} ${FORMS})
else()
	add_executable(${TARGET_NAME} WIN32 ${SOURCES} ${HEADERS} ${FORMS})
endif()
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

if(Qt5_FOUND)
	target_link_libraries(${TARGET_NAME} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)
else()
	target_link_libraries(${TARGET_NAME} Qt4::QtCore Qt4::QtGui Qt4::QtNetwork)
	target_compile_options(${TARGET_NAME} PRIVATE -Zc:wchar_t-)
endif()

install(TARGETS ${TARGET_NAME} DESTINATION ./)

