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

#Look in Pebble app for pebblelib
#This anchors @rpath references to use bundle's Frameworks directory
QMAKE_LFLAGS += -rpath $${DESTDIR}./Pebble.app/Contents/Frameworks

#Help plugin not worry about include paths
#Help plugin not worry about include paths
INCLUDEPATH += ../../../application
DEPENDPATH += ../../../application
INCLUDEPATH += ../../../pebblelib
DEPENDPATH += ../../../pebblelib

SOURCES += main.cpp\
        mainwindow.cpp \
    ../../../pebblelib/wavfile.cpp \
    ../../../pebblelib/cpx.cpp \
    morsegen.cpp \
    ../morsecode.cpp

HEADERS  += mainwindow.h \
    morsegen.h

FORMS    += mainwindow.ui
