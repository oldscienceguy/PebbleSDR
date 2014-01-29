# Example of a Digital Modem plugin for Pebble
# Keep up to date as a template for other plugins

#Project common
include(../../application/pebbleqt.pri)
#DESTDIR is set in pebbleqt.pri
DESTDIR = $${DESTDIR}/plugins

#Common library dependency code for all Pebble plugins
include (../DigitalModemExample/fix_plugin_libraries.pri)
#This anchors @rpath references in plugins to our lib directory, always at the same level os plugin directory
QMAKE_LFLAGS += -rpath $${DESTDIR}/../lib

QT += widgets
TEMPLATE = lib
CONFIG += plugin

TARGET = RttyDigitalModem
VERSION = 1.0.0

#Plugins are independent of main application.  If we need code, we have to explicitly reference it
SOURCES += baudotcode.cpp rtty.cpp \
    ../../application/bargraphmeter.cpp \
    rttydigitalmodem.cpp

HEADERS += baudotcode.h rtty.h \
    ../../application/bargraphmeter.h \
    rttydigitalmodem.h

FORMS += ../../application/bargraphmeter.ui \

#Help plugin not worry about include paths
#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib



