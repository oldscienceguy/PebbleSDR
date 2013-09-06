# Example of a Digital Modem plugin for Pebble
# Keep up to date as a template for other plugins

#Project common
include(../../application/pebbleqt.pri)


QT += widgets
TEMPLATE = lib
CONFIG += plugin

#Set location to UI auto-generated files so we can get headers from known location
message("PWD = "$${PWD})
UI_DIR = $${PWD}/UI
RCC_DIR = $${PWD}/UI
OBJECTS_DIR = $${PWD}/OMac
#Locataion for MOC files
MOC_DIR = $${PWD}/MocMac
message("UI_HEADERS = "$${UI_DIR})

TARGET = DigitalModemExample

SOURCES += digitalmodemexample.cpp

HEADERS += digitalmodemexample.h

#Help plugin not worry about include paths
INCLUDEPATH += ../../application

#DESTDIR is set in pebbleqt.pri
DESTDIR = $${DESTDIR}/plugins

FORMS += \
    data-example.ui
