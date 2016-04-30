#-------------------------------------------------
#
# Project created by QtCreator 2013-10-03T08:40:19
#
#-------------------------------------------------

#Project common
include(../application/pebbleqt.pri)

QT -= gui
QT += multimedia
QT += widgets #Needed for QWidget conversion

TARGET = pebblelib
VERSION = 1.0.0

TEMPLATE = lib

DEFINES += PEBBLELIB_LIBRARY

#Copy PebbleLib specific files before we change DESTDIR, which is set to top level MacDebug or MacRelease
macx {

    #DESTDIR is either top level MacDebug or MacRelease
    #Change it to point to pebblelib/MacDebug or MacRelease
    INSTALLDIR = $${DESTDIR} #Save before we modify
    DESTDIR = $${PWD}/$${LIB_DIR}
    #Delete our custom library directory on clean:  LIB_DIR is set in pebbleqt.pri
    QMAKE_CLEAN *= $${PWD}/$${LIB_DIR}/*

    #-rpath is set by application that uses Pebblelib
    #See pebbleqt.pro which sets it to bundle's Framework directory

    #Mac specific
    SOURCES +=  hid-mac.c

    #The application using PebbleLib is responsible for copying pebblib and all it's dependencies to the directory
    #set by the application's -rpath option
    #We can't copy them here, because we don't know the ultimate location, just do fixups

    #We may need to copy ftd2xx.cfg with value ConfigFlags=0x40000000
    #ftd2xx.files += $${PWD}/../D2XX/bin/10.5-10.7/libftd2xx.1.2.2.dylib
    ftd2xx.commands += install_name_tool -change /usr/local/lib/libftd2xx.1.2.2.dylib @rpath/libftd2xx.1.2.2.dylib $${DESTDIR}/lib$${TARGET}.dylib
    ftd2xx.path = $${INSTALLDIR}
    INSTALLS += ftd2xx

    #In 5.2, Qt plugins that reference a different library than application cause errors (Application must be defined before widget)
    #Fix is to make sure all plugins reference libraries in a common library directory
    #Check dependencies with otool if still looking for installed QT libraries
    #qtlib1.files += $$(QTDIR)/lib/QtWidgets.framework/Versions/5/QtWidgets
    qtlib1.path += $${INSTALLDIR}
    qtlib1.commands += install_name_tool -change $$(QTDIR)/lib/QtWidgets.framework/Versions/5/QtWidgets @rpath/QtWidgets.framework/Versions/5/QtWidgets $${DESTDIR}/lib$${TARGET}.dylib
    INSTALLS += qtlib1

    #qtlib2.files += $$(QTDIR)/lib/QtGui.framework/Versions/5/QtGui
    qtlib2.path += $${INSTALLDIR}
    qtlib2.commands += install_name_tool -change $$(QTDIR)/lib/QtGui.framework/Versions/5/QtGui  @rpath/QtGui.framework/Versions/5/QtGui $${DESTDIR}/lib$${TARGET}.dylib
    INSTALLS += qtlib2

    #qtlib3.files += $$(QTDIR)/lib/QtCore.framework/Versions/5/QtCore
    qtlib3.path += $${INSTALLDIR}
    qtlib3.commands += install_name_tool -change $$(QTDIR)/lib/QtCore.framework/Versions/5/QtCore  @rpath/QtCore.framework/Versions/5/QtCore $${DESTDIR}/lib$${TARGET}.dylib
    INSTALLS += qtlib3

    #qtlib4.files += $$(QTDIR)/lib/QtMultimedia.framework/Versions/5/QtMultimedia
    qtlib4.commands += install_name_tool -change $$(QTDIR)/lib/QtMultimedia.framework/Versions/5/QtMultimedia @rpath/QtMultimedia.framework/Versions/5/QtMultimedia $${DESTDIR}/lib$${TARGET}.dylib
    qtlib4.path = $${INSTALLDIR}
    INSTALLS += qtlib4

    #qtlib5.files += $$(QTDIR)/lib/QtNetwork.framework/Versions/5/QtNetwork
    qtlib5.commands += install_name_tool -change $$(QTDIR)/lib/QtNetwork.framework/Versions/5/QtNetwork @rpath/QtNetwork.framework/Versions/5/QtNetwork $${DESTDIR}/lib$${TARGET}.dylib
    qtlib5.path = $${INSTALLDIR}
    INSTALLS += qtlib5

    #Make sure pebbleqt.pro copies pebblelib library dependencies
    #plib.files += $${PWD}/../D2XX/bin/10.5-10.7/libftd2xx.1.2.2.dylib

    LIBS += -L$${PWD}/../fftw-3.3.4/.libs/ -lfftw3
    LIBS += -L$${PWD}/../D2XX/bin/10.5-10.7/ -lftd2xx.1.2.2
    LIBS += -framework IOKit #For HID
    LIBS += -framework CoreServices #For HID
    LIBS += $${PWD}/../portaudio/lib/.libs/libportaudio.a
    #Portaudio needs mac frameworks, this is how to add them
    LIBS += -framework CoreAudio
    LIBS += -framework AudioToolbox
    LIBS += -framework AudioUnit
    #Only include if USE_FFTACCELERATE is defined
    LIBS += -framework Accelerate

}


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
    alawcompression.cpp \
    fftaccelerate.cpp \
    audiopa.cpp \
    pebblelib_global.cpp \
    wavfile.cpp \
    delayline.cpp \
    firfilter.cpp \
    iirfilter.cpp \
    fir.cpp \
    fractresampler.cpp \
    butterworth.cpp \
    windowfunction.cpp \
    fastfir.cpp \
    decimator.cpp

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
    alawcompression.h \
    fftaccelerate.h \
    medianfilter.h \
    audiopa.h \
    wavfile.h \
    delayline.h \
    firfilter.h \
    iirfilter.h \
    fir.h \
    fractresampler.h \
    butterworth.h \
    windowfunction.h \
    fastfir.h \
    decimator.h

DISTFILES += \
    gpl.html

