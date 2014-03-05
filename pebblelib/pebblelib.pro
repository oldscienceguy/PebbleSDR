#-------------------------------------------------
#
# Project created by QtCreator 2013-10-03T08:40:19
#
#-------------------------------------------------

#Project common
include(../application/pebbleqt.pri)

#Copy PebbleLib specific files before we change DESTDIR
macx {
	rtl2.files += $${DESTDIR}/../pebblelib/$${LIB_DIR}/libpebblelib.1.dylib
	rtl2.files += $${DESTDIR}/../D2XX/bin/10.5-10.7/libftd2xx.1.2.2.dylib
	rtl2.path = $${DESTDIR}/lib
	INSTALLS += rtl2
}

#This anchors @rpath references in plugins to our lib directory, always at the same level os plugin directory
QMAKE_LFLAGS += -rpath $${DESTDIR}/lib

macx {
    #LIB_DIR defined in pebbleqt.pri
    #Override DESTDIR from pebbleqt.pri
    DESTDIR = $${PWD}/$${LIB_DIR}
    SOURCES +=  hid-mac.c

}

QT -= gui
QT += multimedia

TARGET = pebblelib
VERSION = 1.0.0

TEMPLATE = lib

DEFINES += PEBBLELIB_LIBRARY

LIBS += -L$${PWD}/../fftw-3.3.3/.libs/ -lfftw3
LIBS += -L$${PWD}/../D2XX/bin/10.5-10.7/ -lftd2xx.1.2.2
LIBS += -framework IOKit #For HID
LIBS += -framework CoreServices #For HID
LIBS += $${PWD}/../portaudio/lib/.libs/libportaudio.a
#Portaudio needs mac frameworks, this is how to add them
LIBS += -framework CoreAudio
LIBS += -framework AudioToolbox
LIBS += -framework AudioUnit

#In 5.2, Qt plugins that reference a different library than application cause errors (Application must be defined before widget)
#Fix is to make sure all plugins reference libraries in app Frameworks directory
#QT libraries already exist in pebble.app... form macdeployqt in pebbleqt.pro, so we don't copy - just fix
#Check dependencies with otool if still looking for installed QT libraries
qtlib1.path += $${DESTDIR}
qtlib1.commands += install_name_tool -change $$(QTDIR)/lib/QtWidgets.framework/Versions/5/QtWidgets @rpath/QtWidgets.framework/Versions/5/QtWidgets $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib1

qtlib2.path += $${DESTDIR}
qtlib2.commands += install_name_tool -change $$(QTDIR)/lib/QtGui.framework/Versions/5/QtGui  @rpath/QtGui.framework/Versions/5/QtGui $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib2

qtlib3.path += $${DESTDIR}
qtlib3.commands += install_name_tool -change $$(QTDIR)/lib/QtCore.framework/Versions/5/QtCore  @rpath/QtCore.framework/Versions/5/QtCore $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib3

#We may need to copy ftd2xx.cfg with value ConfigFlags=0x40000000
#ftd2xx.files += $${PWD}/../D2XX/bin/10.5-10.7/libftd2xx.1.2.2.dylib
ftd2xx.commands += install_name_tool -change /usr/local/lib/libftd2xx.1.2.2.dylib @rpath/libftd2xx.1.2.2.dylib $${DESTDIR}/lib$${TARGET}.dylib
ftd2xx.path = $${DESTDIR}/lib
INSTALLS += ftd2xx

win32 {
    SOURCES += hid-win.c

}
SOURCES += pebblelib.cpp \
    cpx.cpp \
    fldigifilters.cpp \
    downconvert.cpp \
    db.cpp \
    fftw.cpp \
    fftooura.cpp \
    fftcute.cpp \
    fft.cpp \
    iir.cpp \
    producerconsumer.cpp \
    perform.cpp \
    usbutil.cpp \
    deviceinterfacebase.cpp \
    audio.cpp \
    audioqt.cpp \
    soundcard.cpp

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
    iir.h \
    device_interfaces.h \
    producerconsumer.h \
    perform.h \
    usbutil.h \
    deviceinterfacebase.h \
    hidapi.h \
    audio.h \
    audioqt.h \
    soundcard.h

