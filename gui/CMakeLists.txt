cmake_minimum_required(VERSION 3.1)

find_package(Qt5 QUIET COMPONENTS Core Gui Widgets)
if(NOT Qt5_FOUND)
	find_package(Qt4 4.8 REQUIRED QtCore QtGui)
endif()

if(Qt5_FOUND)
	message(STATUS "Using Qt5")
else()
	message(STATUS "Using Qt4")
endif()

set(FORMS
    Maingui.ui
    SqlConfigDialog.ui
    DatabaseView.ui
    DatabaseDescManageDialog.ui
    HashConverterDialog.ui
    SqlOpenSaveDialog.ui
    LogWindow.ui
)

set(SOURCES
    main.cpp
    Maingui.cpp
    DatabaseTableModel.cpp
    SqlConfigDialog.cpp
    DatabaseView.cpp
    PersistentData.cpp
    DatabaseDescriptionListModel.cpp
    OpenSaveSource.cpp
    TabBarEventFilter.cpp
    DatabaseDescManageDialog.cpp
    NameToHash.c
    Settings.cpp
    HashConverterDialog.cpp
    HashToName.c
    FileDialog.cpp
    FileToRenameListModel.cpp
    SqlOpenSaveDialog.cpp
    LogWindow.cpp
)

set(HEADERS
    Maingui.h
    DatabaseTableModel.h
    SqlConfigDialog.h
    DatabaseView.h
    PersistentData.h
    DatabaseDescriptionListModel.h
    OpenSaveSource.h
    TabBarEventFilter.h
    DatabaseDescManageDialog.h
    NameToHash.h
    Settings.h
    HashConverterDialog.h
    HashToName.h
    FileDialog.h
    FileToRenameListModel.h
    SqlOpenSaveDialog.h
    LogWindow.h
)

if(NOT Qt5_FOUND)
	QT4_CREATE_TRANSLATION(TRANSLATIONS ${SOURCES} ${HEADERS} RappelzRDBToolQt_sample.ts)
else()
	QT5_CREATE_TRANSLATION(TRANSLATIONS ${SOURCES} ${HEADERS} RappelzRDBToolQt_sample.ts)
endif()

configure_file(version.h.in version.h @ONLY)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
add_executable(${TARGET_NAME} WIN32 ${SOURCES} ${HEADERS} ${FORMS} ${TRANSLATIONS} version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
target_link_libraries(${TARGET_NAME} RpzRdbBase)

# Used for both Qt4 with cmake < 3.8 and version.h
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(Qt5_FOUND)
	target_link_libraries(${TARGET_NAME} Qt5::Core Qt5::Gui Qt5::Widgets)
else()
	target_link_libraries(${TARGET_NAME} Qt4::QtCore Qt4::QtGui)
	target_compile_options(${TARGET_NAME} PRIVATE -Zc:wchar_t-)
endif()

install(TARGETS ${TARGET_NAME} DESTINATION ./)
install(FILES RappelzRDBToolQt_sample.ts DESTINATION ./)

