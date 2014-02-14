#-------------------------------------------------
#
# Project created by QtCreator 2014-02-13T18:40:05
#
#-------------------------------------------------

#Project common
include(../../application/pebbleqt.pri)
#DESTDIR is set in pebbleqt.pri
DESTDIR = $${DESTDIR}/plugins

#Common library dependency code for all Pebble plugins
include (../DigitalModemExample/fix_plugin_libraries.pri)
#This anchors @rpath references in plugins to our lib directory, always at the same level os plugin directory
QMAKE_LFLAGS += -rpath $${DESTDIR}/../lib

#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

#Required for options UI
QT += widgets

TARGET = SoftRockSDRDevice
VERSION = 1.0.0
TEMPLATE = lib
CONFIG += plugin

SOURCES += softrocksdrdevice.cpp

HEADERS += softrocksdrdevice.h

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib
LIBS += -L$${PWD}/../../D2XX/bin/10.5-10.7/ -lftd2xx.1.2.2

ftd2xx.commands += install_name_tool -change /usr/local/lib/libftd2xx.1.2.2.dylib @rpath/libftd2xx.1.2.2.dylib $${DESTDIR}/lib$${TARGET}.dylib
ftd2xx.path = $${DESTDIR}/lib
INSTALLS += ftd2xx

OTHER_FILES += \
        fix_plugin_libraries.pri

FORMS += \
    softrockoptions.ui
