#-------------------------------------------------
#
# Project created by QtCreator 2019-06-03T22:08:02
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QVideoBatcher
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

CONFIG += c++11

SOURCES += \
        main.cpp \
        qitemcb.cpp \
        qlogo.cpp \
        qthreadvd.cpp \
        qthreadplay.cpp \
        qthreadffmpeg.cpp \
        quimain.cpp

HEADERS += \
        common.h \
        global.h \
        qcv.hpp \
        qitemcb.h \
        qlogo.h \
        qthreadvd.h \
        qthreadplay.h \
        qthreadffmpeg.h \
        quimain.h

FORMS += \
        qitemcb.ui \
        quimain.ui

win32 {
    INCLUDEPATH += "C:\Program Files\opencv\build\include"

    LIBS += "C:\Program Files\opencv\opencv_mingw_build_release\lib\libopencv_core410.dll.a"
    LIBS += "C:\Program Files\opencv\opencv_mingw_build_release\lib\libopencv_imgproc410.dll.a"
    LIBS += "C:\Program Files\opencv\opencv_mingw_build_release\lib\libopencv_imgcodecs410.dll.a"
    LIBS += "C:\Program Files\opencv\opencv_mingw_build_release\lib\libopencv_videoio410.dll.a"

    RC_FILE = "res\qt.rc"
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    cpp.astylerc
