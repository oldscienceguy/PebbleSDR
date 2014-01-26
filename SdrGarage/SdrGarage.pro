#-------------------------------------------------
#
# Project created by QtCreator 2014-01-26T02:30:33
#
#-------------------------------------------------

#Project common
include(../application/pebbleqt.pri)

INCLUDEPATH += ../pebblelib
DEPENDPATH += ../pebblelib

QT       += core network

QT       -= gui

TARGET = SdrGarage
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    sdrserver.cpp \
    rtltcpprotocol.cpp \
    deviceplugins.cpp

HEADERS += \
    sdrserver.h \
    rtltcpprotocol.h \
    deviceplugins.h
