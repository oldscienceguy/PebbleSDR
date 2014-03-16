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
QT += widgets

#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

TARGET = ElektorDevice
VERSION = 1.0.0
TEMPLATE = lib
CONFIG += plugin

SOURCES += elektordevice.cpp

HEADERS += elektordevice.h

FORMS += elektoroptions.ui

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib

OTHER_FILES += \
        fix_plugin_libraries.pri
