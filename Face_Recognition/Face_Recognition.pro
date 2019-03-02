#-------------------------------------------------
#
# Project created by QtCreator 2019-02-11T14:14:59
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Face_Recognition
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
        widget.cpp \
    facedefine.cpp \
    arcsoft_face_proc.cpp \
    cvxtext.cpp

HEADERS += \
        widget.h \
    facedefine.h \
    arcsoft_face_proc.h \
    arcsoft/inc/amcomdef.h \
    arcsoft/inc/arcsoft_face_sdk.h \
    arcsoft/inc/asvloffscreen.h \
    arcsoft/inc/merror.h \
    cvxtext.h

FORMS += \
        widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES +=

LIBS += -L$$PWD/arcsoft/lib/Win32/ -llibarcsoft_face_engine

INCLUDEPATH += $$PWD/arcsoft/inc
DEPENDPATH += $$PWD/arcsoft/inc

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/opencv3.2/lib/ -lopencv_world320
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/opencv3.2/lib/ -lopencv_world320d

INCLUDEPATH += $$PWD/opencv3.2/include
DEPENDPATH += $$PWD/opencv3.2/include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/mr/lib/Release/ -lTHmrIPCSDK
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/mr/lib/Debug/ -lTHmrIPCSDK

INCLUDEPATH += $$PWD/mr/include
DEPENDPATH += $$PWD/mr/include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/freetype-2.9.1/lib/ -lfreetype
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/freetype-2.9.1/lib/ -lfreetyped

INCLUDEPATH += $$PWD/freetype-2.9.1/include
DEPENDPATH += $$PWD/freetype-2.9.1/include
