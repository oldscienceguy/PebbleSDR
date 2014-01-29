#-------------------------------------------------
#
# Project created by QtCreator 2014-01-26T02:30:33
#
#-------------------------------------------------

#Project common
include(../application/pebbleqt.pri)

#This anchors @rpath references in plugins to our lib directory, always at the same level os plugin directory
QMAKE_LFLAGS += -rpath $${DESTDIR}/lib

INCLUDEPATH += ../pebblelib
DEPENDPATH += ../pebblelib

QT       += core network

QT       -= gui

TARGET = SdrGarage
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

#Devices expect PebbleLib to be in the executable
#LIBS += -L$${PWD}/../pebblelib/$${LIB_DIR} -lpebblelib.1

SOURCES += main.cpp \
    sdrserver.cpp \
    rtltcpprotocol.cpp \
    deviceplugins.cpp

HEADERS += \
    sdrserver.h \
    rtltcpprotocol.h \
    deviceplugins.h
