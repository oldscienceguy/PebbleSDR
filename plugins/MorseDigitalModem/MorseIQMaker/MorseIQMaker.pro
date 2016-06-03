#-------------------------------------------------
#
# Project created by QtCreator 2016-05-29T18:33:42
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MorseIQMaker
TEMPLATE = app

#Project common
include(../../../application/pebbleqt.pri)

#DESTDIR is set in pebbleqt.pri
#Debugger won't start if we move app into a subdirectory.  Get no access rights error
#DESTDIR = $${DESTDIR}/Utilities

#Help plugin not worry about include paths
#Help plugin not worry about include paths
INCLUDEPATH += ../../../application
DEPENDPATH += ../../../application
INCLUDEPATH += ../../../pebblelib
DEPENDPATH += ../../../pebblelib

pebblefix.commands += macdeployqt $${DESTDIR}/MorseIQMaker.app -no-strip -verbose=0;
pebblefix.path += $${DESTDIR}
INSTALLS += pebblefix

SOURCES += main.cpp\
        mainwindow.cpp \
    ../../../pebblelib/wavfile.cpp \
    ../../../pebblelib/cpx.cpp \
    morsegen.cpp \
    ../morsecode.cpp

HEADERS  += mainwindow.h \
    morsegen.h

FORMS    += mainwindow.ui
