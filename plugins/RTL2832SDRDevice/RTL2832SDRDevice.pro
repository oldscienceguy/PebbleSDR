#-------------------------------------------------
#
# Project created by QtCreator 2014-01-03T13:01:57
#
#-------------------------------------------------
#Project common
include(../../application/pebbleqt.pri)
#DESTDIR is set in pebbleqt.pri
DESTDIR = $${DESTDIR}/plugins

#Common library dependency code for all Pebble plugins
include (../DigitalModemExample/fix_plugin_libraries.pri)

QT += widgets

TEMPLATE = lib
CONFIG += plugin

TARGET = rtl2832sdrDevice
VERSION = 1.0.0

SOURCES += rtl2832sdrdevice.cpp

HEADERS += rtl2832sdrdevice.h

#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib
#Just for rtl-sdr
INCLUDEPATH += ../../rtl-sdr/include
DEPENDPATH += ../../rtl-sdr/include

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib
LIBS += -L$${PWD}/../../rtl-sdr/src/.libs -lrtlsdr

OTHER_FILES += \
	fix_plugin_libraries.pri
