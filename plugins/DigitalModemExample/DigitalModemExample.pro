# Example of a Digital Modem plugin for Pebble
# Keep up to date as a template for other plugins

#Project common
include(../../application/pebbleqt.pri)


QT += widgets
TEMPLATE = lib
CONFIG += plugin

TARGET = DigitalModemExample

SOURCES += digitalmodemexample.cpp

HEADERS += digitalmodemexample.h

#Help plugin not worry about include paths
INCLUDEPATH += ../../application
INCLUDEPATH += ../../plugins/

#DESTDIR is set in pebbleqt.pri
DESTDIR = $${DESTDIR}/plugins
