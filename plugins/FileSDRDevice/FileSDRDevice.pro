# File Device plugin for Pebble

#Project common
include(../../application/pebbleqt.pri)
#DESTDIR is set in pebbleqt.pri
DESTDIR = $${DESTDIR}/plugins

#Common library dependency code for all Pebble plugins
include (../DigitalModemExample/fix_plugin_libraries.pri)

#Required for options UI
QT += widgets

TEMPLATE = lib
CONFIG += plugin

TARGET = FileSDRDevice
VERSION = 1.0.0

SOURCES += filesdrdevice.cpp \
    wavfile.cpp

HEADERS += filesdrdevice.h \
    wavfile.h

#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib

OTHER_FILES += \
	fix_plugin_libraries.pri
