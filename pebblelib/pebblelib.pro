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

LIBS += -L$${PWD}/../fftw-3.3.3/.libs/ -lfftw3

SOURCES += pebblelib.cpp \
    cpx.cpp \
    fldigifilters.cpp \
    downconvert.cpp \
    db.cpp \
    fftw.cpp \
    fftooura.cpp \
    fftcute.cpp \
    fft.cpp \
    iir.cpp

HEADERS += pebblelib.h\
        pebblelib_global.h \
    cpx.h \
    gpl.h \
    digital_modem_interfaces.h \
    fldigifilters.h \
    downconvert.h \
    filtercoef.h \
    db.h \
    fftw.h \
    fftooura.h \
    fftcute.h \
    fft.h \
    iir.h

