#-------------------------------------------------
#
# Project created by QtCreator 2018-01-22T12:43:09
#
#-------------------------------------------------

QT       += core gui gui-private

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtviewplanes
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_NO_DEBUG_OUTPUT
#DEFINES += ALL_SOFTWARE

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp \
    graphicsplaneitem.cpp \
    graphicsplaneview.cpp \
    planemanager.cpp \
    tools.cpp

HEADERS  += \
    graphicsplaneitem.h \
    graphicsplaneview.h \
    planemanager.h \
    tools.h

DISTFILES += \
    qtviewplanes.screen

target.path = /opt/qtviewplanes
target.files = qtviewplanes
extra.path = /opt/qtviewplanes
extra.files = resources/qtviewplanes.sh qtviewplanes.screen
configfile.path = /opt/ApplicationLauncher/applications/xml
configfile.files = resources/12-qtviewplanes.xml
imagefile.path = /opt/ApplicationLauncher/applications/resources
imagefile.files = resources/qtviewplanes.png
INSTALLS += target configfile imagefile extra

CONFIG += link_pkgconfig

LOCALPLANES {
    PKGCONFIG += tslib
    INCLUDEPATH += $(HOME)/planes/include/
    LIBS += -L$(HOME)/planes/src/.libs -lplanes
} else {
    PKGCONFIG += libplanes
}

PKGCONFIG += libdrm cairo libcjson lua

#CONFIG += LOCALPLANES

RESOURCES += \
    media.qrc
