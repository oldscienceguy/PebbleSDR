#-------------------------------------------------
#
# Project created by QtCreator 2014-02-13T18:42:09
#
#-------------------------------------------------

#Project common
include(../../application/pebbleqt.pri)
#DESTDIR is set in pebbleqt.pri
DESTDIR = $${DESTDIR}/plugins

#Common library dependency code for all Pebble plugins
include (../DigitalModemExample/fix_plugin_libraries.pri)

#Required for options UI
QT += widgets network multimedia

#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

TARGET = Ghpsdr3Device
VERSION = 1.0.0
TEMPLATE = lib
CONFIG += plugin

SOURCES += ghpsdr3device.cpp

HEADERS += ghpsdr3device.h

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib

OTHER_FILES += \
        fix_plugin_libraries.pri
