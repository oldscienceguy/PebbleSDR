#-------------------------------------------------
#
# Project created by QtCreator 2014-01-26T02:30:33
#
#-------------------------------------------------

#Project common
include(../application/pebbleqt.pri)

#From doc: The use of @rpath is most useful when you have a complex directory structure of programs and dylibs which can be
#	installed anywhere, but keep their relative positions.
#The use of @rpath introduces a level of indirection that simplies things. You pick a location in your directory
#structure as an anchor point. Each dylib then gets an install path that starts with @rpath and is the path to the
#dylib relative to the anchor point. Each main executable is linked with -rpath @loader_path/zzz,
#where zzz is the path from the executable to the anchor point. At runtime dyld sets it run path to be the anchor point,
#then each dylib is found relative to the anchor point.
#See use of @rpath in install_tool usage below

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
