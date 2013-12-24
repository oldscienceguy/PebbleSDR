# Example of a Digital Modem plugin for Pebble
# Keep up to date as a template for other plugins

#Project common
include(../../application/pebbleqt.pri)
#DESTDIR is set in pebbleqt.pri
DESTDIR = $${DESTDIR}/plugins

#Common library dependency code for all Pebble plugins
include (../DigitalModemExample/fix_plugin_libraries.pri)


QT += widgets
TEMPLATE = lib
CONFIG += plugin

#Set location to UI auto-generated files so we can get headers from known location
UI_DIR = $${PWD}/UI
RCC_DIR = $${PWD}/UI
OBJECTS_DIR = $${PWD}/OMac
MOC_DIR = $${PWD}/MocMac

TARGET = wwvdigitalmodem

SOURCES += wwvdigitalmodem.cpp

HEADERS += wwvdigitalmodem.h

#Help plugin not worry about include paths
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

LIBS += -L$${PWD}/../../pebblelib/lib -lpebblelib

FORMS += \
	data-wwv.ui
