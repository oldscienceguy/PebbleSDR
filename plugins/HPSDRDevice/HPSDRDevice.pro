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
QT += widgets network

#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

TARGET = HPSDRDevice
VERSION = 1.0.0
TEMPLATE = lib
CONFIG += plugin

SOURCES += hpsdrdevice.cpp \
    hpsdrnetwork.cpp

HEADERS += hpsdrdevice.h \
    hpsdrnetwork.h

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib

#To update firmware files, copy them to generic (no version numbers) names below
#Current firmware as of 2/17/14: ozy_janus_v2.5.rbf
firmware.files = Firmware/ozyfw-sdr1k.hex Firmware/ozy_janus.rbf
firmware.path = $${DESTDIR}/../PebbleData
INSTALLS += firmware

OTHER_FILES += \
        fix_plugin_libraries.pri

FORMS += \
    hpsdroptions.ui
