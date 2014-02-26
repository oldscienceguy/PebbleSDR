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
#This anchors @rpath references in plugins to our lib directory, always at the same level os plugin directory
QMAKE_LFLAGS += -rpath $${DESTDIR}/../lib

#Required for options UI
QT += widgets network

#Help plugin not worry about include paths
INCLUDEPATH += ../../application
DEPENDPATH += ../../application
INCLUDEPATH += ../../pebblelib
DEPENDPATH += ../../pebblelib

QT += widgets
TARGET = RFSpaceDevice
VERSION = 1.0.0
TEMPLATE = lib
CONFIG += plugin

SOURCES += rfspacedevice.cpp \
    ad6620.cpp

HEADERS += rfspacedevice.h \
    rfsfilters.h \
    ad6620.h

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib

OTHER_FILES += \
        fix_plugin_libraries.pri

FORMS += \
    sdripoptions.ui \
    sdriqoptions.ui