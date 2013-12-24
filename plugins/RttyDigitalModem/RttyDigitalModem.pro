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

TARGET = RttyDigitalModem

#Plugins are independent of main application.  If we need code, we have to explicitly reference it
SOURCES += baudotcode.cpp rtty.cpp \
    ../../application/bargraphmeter.cpp \
    rttydigitalmodem.cpp

HEADERS += baudotcode.h rtty.h \
    ../../application/bargraphmeter.h \
    rttydigitalmodem.h

FORMS += ../../application/bargraphmeter.ui \

#Help plugin not worry about include paths
INCLUDEPATH += ../../application

LIBS += -L$${PWD}/../../pebblelib/lib -lpebblelib



