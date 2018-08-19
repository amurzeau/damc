#-------------------------------------------------
#
# Project created by QtCreator 2018-07-15T00:17:05
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = gui
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

unix {
    INCLUDEPATH += /usr/include/qwt
    LIBS += -L/usr/lib \
            -lqwt-qt5
}

SOURCES += \
        main.cpp \
        MainWindow.cpp \
    BodePlot.cpp \
    ../EqFilter.cpp \
    OutputController.cpp \
    WavePlayInterface.cpp \
    WavePlayOutputInterface.cpp \
    EqualizersController.cpp \
    Equalizer.cpp \
    Dialog.cpp

HEADERS += \
        MainWindow.h \
    BodePlot.h \
    ../EqFilter.h \
    OutputController.h \
    WavePlayInterface.h \
    WavePlayOutputInterface.h \
    MessageInterface.h \
    EqualizersController.h \
    Equalizer.h \
    Dialog.h

FORMS += \
        MainWindow.ui \
    OutputController.ui \
    EqualizerFilters.ui \
    Equalizer.ui \
    EqualizersController.ui \
    Dialog.ui
