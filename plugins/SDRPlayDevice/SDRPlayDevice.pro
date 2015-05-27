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

TARGET = SDRPlayDevice
VERSION = 1.0.0
TEMPLATE = lib
CONFIG += plugin

SOURCES += sdrplaydevice.cpp

HEADERS += sdrplaydevice.h

LIBS += -L$${PWD}/../../pebblelib/$${LIB_DIR} -lpebblelib
LIBS += -L$${PWD}/../../plugins/SDRPlayDevice/MiricsAPI/ -lmir_sdr


#libusb is copied into Frameworks directory in pebbleqt.pro, but do it here to be sure
copy_files.files += /opt/local/lib/libusb-1.0.0.dylib
copy_files.files += $${PWD}/../../plugins/SDRPlayDevice/MiricsAPI/libmir_sdr.so
copy_files.path = $${INSTALL_DIR}/Pebble.app/Contents/Frameworks
INSTALLS += copy_files

#We need to fix mir_sdr so it finds libusb in the right place
#mir_sdr normally looks for /usr/local/lib/libusb-1.0.0
#Many systems have it pre-installed in opt/lib/libusb-1.0.0
#But we keep all our dependent libraries in the app/contents/frameworks directory, which is set with rpath
fix_mirlib.path = $${INSTALL_DIR}/Pebble.app/Contents/Frameworks
fix_mirlib.commands += install_name_tool -change /usr/local/lib/libusb-1.0.0.dylib @rpath/libusb-1.0.0.dylib $${INSTALL_DIR}/Pebble.app/Contents/Frameworks/libmir_sdr.so
INSTALLS += fix_mirlib

OTHER_FILES += \
        fix_plugin_libraries.pri

FORMS += \
    sdrplayoptions.ui
