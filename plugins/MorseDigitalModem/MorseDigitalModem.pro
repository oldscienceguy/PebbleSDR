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

TARGET = MorseDigitalModem
VERSION = 1.0.0

#Plugins are independent of main application.  If we need code, we have to explicitly reference it
SOURCES += morse.cpp morsecode.cpp \
    ../../application/bargraphmeter.cpp \

HEADERS += morse.h morsecode.h \
    ../../application/bargraphmeter.h \

FORMS += data-morse.ui \
    ../../application/bargraphmeter.ui \

#Help plugin not worry about include paths
#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib
