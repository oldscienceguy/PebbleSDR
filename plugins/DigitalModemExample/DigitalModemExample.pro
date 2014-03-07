# Example of a Digital Modem plugin for Pebble
# Keep up to date as a template for other plugins

#Project common
include(../../application/pebbleqt.pri)
#DESTDIR is set in pebbleqt.pri
DESTDIR = $${DESTDIR}/plugins

#Common library dependency code for all Pebble plugins
include (fix_plugin_libraries.pri)
#This anchors @rpath references in plugins to our lib directory, always at the same level os plugin directory
QMAKE_LFLAGS += -rpath @loader_path/../lib


QT += widgets
TEMPLATE = lib
CONFIG += plugin

TARGET = DigitalModemExample
VERSION = 1.0.0

SOURCES += digitalmodemexample.cpp

HEADERS += digitalmodemexample.h

#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib

FORMS += \
    data-example.ui

OTHER_FILES += \
    fix_plugin_libraries.pri
