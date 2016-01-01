#-------------------------------------------------
#
# Project created by QtCreator 2014-02-13T18:42:09
#
#-------------------------------------------------

#Project common
include(../../application/pebbleqt.pri)
#DESTDIR is set in pebbleqt.pri, save it
INSTALL_DIR = $${DESTDIR}
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

TARGET = HackRFDevice
VERSION = 1.0.0
TEMPLATE = lib
CONFIG += plugin

SOURCES += hackrfdevice.cpp \
    hackrf.c

HEADERS += hackrfdevice.h \
    hackrf.h

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib
#We're not using usbutil so we can use hackrf.h and hackrf.c mostly unmodified
LIBS += -L$${PWD}/../../D2XX/bin/10.5-10.7/ -lftd2xx.1.2.2

ftd2xx.commands += install_name_tool -change /usr/local/lib/libftd2xx.1.2.2.dylib @rpath/libftd2xx.1.2.2.dylib $${DESTDIR}/lib$${TARGET}.dylib
ftd2xx.path = $${INSTALL_DIR}/Pebble.app/Contents/Frameworks
INSTALLS += ftd2xx

OTHER_FILES += \
        fix_plugin_libraries.pri

FORMS += \
    hackrfoptions.ui
