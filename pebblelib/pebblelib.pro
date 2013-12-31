#-------------------------------------------------
#
# Project created by QtCreator 2013-10-03T08:40:19
#
#-------------------------------------------------

#Project common
include(../application/pebbleqt.pri)

macx {
	#LIB_DIR defined in pebbleqt.pri
	#Override DESTDIR from pebbleqt.pri
	DESTDIR = $${PWD}/$${LIB_DIR}
}

QT       -= gui

TARGET = pebblelib
VERSION = 1.0.0

TEMPLATE = lib

DEFINES += PEBBLELIB_LIBRARY

LIBS += -L$${PWD}/../fftw-3.3.3/.libs/ -lfftw3

#In 5.2, Qt plugins that reference a different library than application cause errors (Application must be defined before widget)
#Fix is to make sure all plugins reference libraries in app Frameworks directory
#QT libraries already exist in pebble.app... form macdeployqt in pebbleqt.pro, so we don't copy - just fix
#Check dependencies with otool if still looking for installed QT libraries
qtlib1.path += $${DESTDIR}
qtlib1.commands += install_name_tool -change /Users/rlandsman/Qt/5.2.0/clang_64/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib1

qtlib2.path += $${DESTDIR}
qtlib2.commands += install_name_tool -change /Users/rlandsman/Qt/5.2.0/clang_64/lib/QtGui.framework/Versions/5/QtGui  @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib2

qtlib3.path += $${DESTDIR}
qtlib3.commands += install_name_tool -change /Users/rlandsman/Qt/5.2.0/clang_64/lib/QtCore.framework/Versions/5/QtCore  @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib3

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
    iir.h \
    device_interfaces.h

