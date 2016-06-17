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

TARGET = MorseGenDevice
VERSION = 1.0.0
TEMPLATE = lib
CONFIG += plugin

SOURCES += morsegendevice.cpp \
    morsegen.cpp \
    ../MorseDigitalModem/morsecode.cpp

HEADERS += morsegendevice.h \
    morsegen.h \
    ../MorseDigitalModem/morsecode.h

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib

#Copy sample file(s) to PebbleData
sampleFiles.files += morsesample.txt
sampleFiles.files += 300USAqso-1.txt
sampleFiles.files += 300USAqso-2.txt
sampleFiles.files += 300USAqso-3.txt
sampleFiles.path = $${DESTDIR}/../PebbleData
INSTALLS += sampleFiles

OTHER_FILES += \
        fix_plugin_libraries.pri

FORMS += \
    morsegendevice.ui
