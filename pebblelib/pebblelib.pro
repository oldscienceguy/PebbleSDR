#-------------------------------------------------
#
# Project created by QtCreator 2013-10-03T08:40:19
#
#-------------------------------------------------

#Project common
include(../application/pebbleqt.pri)

macx {
	#Set location to UI auto-generated files so we can get headers from known location
	UI_DIR = $${PWD}/UI
	RCC_DIR = $${PWD}/UI
	OBJECTS_DIR = $${PWD}/OMac
	#Locataion for MOC files
	MOC_DIR = $${PWD}/MocMac

	DESTDIR = $${PWD}/lib
}

QT       -= gui

TARGET = pebblelib
TEMPLATE = lib

DEFINES += PEBBLELIB_LIBRARY

SOURCES += pebblelib.cpp \
    cpx.cpp \
    fldigifilters.cpp \
    downconvert.cpp \
    db.cpp

HEADERS += pebblelib.h\
        pebblelib_global.h \
    cpx.h \
    gpl.h \
    digital_modem_interfaces.h \
    fldigifilters.h \
    downconvert.h \
    filtercoef.h \
    db.h

