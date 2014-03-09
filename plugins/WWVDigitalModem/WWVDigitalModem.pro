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

TARGET = wwvdigitalmodem
VERSION = 1.0.0

SOURCES += wwvdigitalmodem.cpp

HEADERS += wwvdigitalmodem.h

#Help plugin not worry about include paths
#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib

FORMS += \
	data-wwv.ui
