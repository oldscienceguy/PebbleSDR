TEMPLATE = app
TARGET = Pebble
QT += core gui multimedia

#Download & install FTDI D2XX drivers from http://www.ftdichip.com/Drivers/D2XX.htm

#Download PortAudio from portaudio.com , open terminal, ./configure && make
#portaudio directory is at same level as PebbleII

#Download FFTW from fftw.org, open terminal, build floating point
#./configure --enable-float
# make
#sudo make install

#Header is in api directory, lib is in ./libs directory
#Or prebuilt fftw from pdb.finkproject.org

#Dev Setup notes
#To support UNC paths for Parallels, Regedit HKEY_CURRENTUSER/Software/Microsoft/Command Processor
#and add the value DisableUNCCheck REG_DWORD and set the value to 0 x 1 (Hex).
#Make sure to update project version if Qt version is updated in QtCreator
#Exit code 0x0c0000315 if "Release DLLs" not copied to Debug and Release dirs

#Set location to UI auto-generated files so we can get headers from known location
UI_HEADERS_DIR = $$PWD/UI
UI_SOURCES_DIR = $$PWD/UI


#Should this be predefined somewhere?
QTDIR = c:/Qt/4.8.0

#Enable this to look at config to debug conditionals. For example: debug and release both show up sometimes
#message($$CONFIG)


#Conditional MUST match the Build Configuration name, Debug or Release or SomeCustomName
macx {
	#debug and release may both be defined as .pro file is parsed by make multiple times
	#This tests for debug as the last item to be defined amoung debug and release
	CONFIG(debug, debug|release) {
		DESTDIR = ../MacDebug
	} else {
		DESTDIR = ../MacRelease
	}

	OBJECTS_DIR = $$PWD/MacO
	#Locataion for MOC files
	MOC_DIR = $$PWD/MacMoc

	LIBS += -L$$PWD/../D2XX/bin/10.5-10.7/ -lftd2xx.1.1.0
	LIBS += ../portaudio/lib/.libs/libportaudio.a
	#Portaudio needs mac frameworks, this is how to add them
	LIBS += -framework CoreAudio
	LIBS += -framework AudioToolbox
	LIBS += -framework AudioUnit
	LIBS += -framework CoreServices
	##fftw
	LIBS += -L$$PWD/../fftw-3.3.1/.libs/ -lfftw3f

	LIBS += /System/Library/Frameworks/CoreFoundation.framework/CoreFoundation \
		/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit

	#Don't know if we need these
	#INCLUDEPATH += ../portaudio/include
	#LIBPATH += ../portaudio/lib
	#INCLUDEPATH adds directories for header file searches
	INCLUDEPATH += $$PWD/../D2XX/bin/10.5-10.7
	DEPENDPATH += $$PWD/../D2XX/bin/10.5-10.7

	#Mac only source files
	#HIDAPI
	SOURCES += hid-mac.c

	#INSTALLS is called when we manually make -install or add it to the Qt project build steps
	otherfiles.files = presets.csv help.htm gpl.h releasenotes.txt

	#We want this to be whatever the release or debug directory is or added to bundle
	#This will copy the files into the app bundle, same place ini files go
	otherfiles.path = $$DESTDIR/Pebble.app/Contents/MacOS
	INSTALLS += otherfiles

}

win32 {
	CONFIG(debug, debug|release) {
		DESTDIR = ../WinDebug
	} else {
		DESTDIR = ../WinRelease
	}

	OBJECTS_DIR = $$PWD/WinO
	#Locataion for MOC files
	MOC_DIR = $$PWD/WinMoc

	LIBS +=	../PebbleII/PortAudio.lib
	LIBS +=	../fftw3/libfftw3f-3.lib
	#only needed for FCD HID Windows support
	#2/3/12 Use mingw libsetupapi instead of Microsoft Lib
	LIBS += libsetupapi
	LIBS += C:/Program Files/Microsoft SDKs/Windows/v6.0A/Lib/setupapi.lib

	#Win only source files
	SOURCES += hid-win.c

	CONFIG(release, debug|release) {
		DESTDIR = ../WinRelease
		#Relative to source dir
		installfiles.path = ../Release
		installfiles.files = \
			../fftw3/libfftw3f-3.dll \
			#PennyMerge versions of file
			../HPSDR/ozy_janus.rbf \
			../HPSDR/ozyfw-sdr1k.hex \
			$$QTDIR/mingw/bin/libgcc_s_dw2-1.dll \
			$$QTDIR/mingw/bin/mingwm10.dll \
		#WARNING: Version of qtcore4.dll in $$QTDIR/bin is missing qt_message_output!
		#Make sure we're always getting the version as built
				$$QTDIR/bin/qtcore4.dll \
				$$QTDIR/bin/qtgui4.dll \
				$$QTDIR/bin/qtmultimedia4.dll \
			../"Release DLLs"/PortAudio.dll

		#target.path = ../Release
		INSTALLS += target installfiles

	}

	CONFIG(debug, debug|release) {
		DESTDIR = ../WinDebug
		#Relative to source dir
		installfiles.path = ../Debug
		installfiles.files = \
			../fftw3/libfftw3f-3.dll \
			$$QTDIR/mingw/bin/libgcc_s_dw2-1.dll \
			$$QTDIR/mingw/bin/mingwm10.dll \
		#PennyMerge versions of file
			../HPSDR/ozy_janus.rbf \
			../HPSDR/ozyfw-sdr1k.hex \
		#WARNING: Version of qtcore4.dll in $$QTDIR/bin is missing qt_message_output!
		#Make sure we're always getting the version as built
				$$QTDIR/bin/qtcored4.dll \
				$$QTDIR/bin/qtguid4.dll \
				$$QTDIR/bin/qtmultimediad4.dll \
			../"Release DLLs"/PortAudio.dll

		target.path = ../Debug
		INSTALLS += target installfiles

		#INSTALLS is called when we manually make -install or add it to the Qt project build steps
		otherfiles.files = presets.csv help.htm gpl.h releasenotes.txt

		#We want this to be whatever the release or debug directory is or added to bundle
		otherfiles.path = $$DESTDIR
		INSTALLS += otherfiles

	}
}


#Build instructions for build.h using build.tmpl
#doesn't seem to work
#buildh.target = build.h
#buildh.depends = build.tmpl
#buildh.commands = SubWCRev build.tmpl build.h
#QMAKE_EXTRA_TARGETS += buildh

#Undocumented(?) Note brackets not paren
#${QMAKE_FILE_BASE) = build for build.h
#${QMAKE_FILE_PATH} = ../PebbleII

PREPROCESS = build.tmpl
preprocess.name = SubversionBuildNumberUpdate
preprocess.input = PREPROCESS
preprocess.output = ../PebbleII/build.h #What make looks at to see if build is needed
#preprocess.depends = ../PebbleII/_svn #Datetime modified changes with every checkin, so this will trigger update
preprocess.CONFIG = no_link #This is required to stop make from trying to compile output
#preprocess.commands = SubWCRev ../PebbleII ../PebbleII/build.tmpl ../PebbleII/build.h
#QMAKE_EXTRA_COMPILERS += preprocess

#QMAKE_PRE_LINK += copy $(DEPENDPATH)/presets.csv $(DESTDIR)
#QMAKE_POST_LINK

#Equal operator required for compiler flag,  not allowed for #define
DEFINES += \
	"SIMD=0" #SSE crashing, figure it out later

#CONFIG += qt sse

#This is how we can add custom defines and options based on release vs debug builds
#See release{} and debug{} install section for an alternate way
#CONFIG(debug, debug|release): DEFINES += DEBUG
#CONFIG(release, debug|release): DEFINES += RELEASE


#Shouldn't have to use this, but CONFIG not working.  Uncomment for SSE testing
#QMAKE_CXXFLAGS += \
	#-msse3 -mssse3 -mfpmath=sse

#QMAKE_LFLAGS += -static-libgcc

OTHER_FILES += \
    Qt4VSPropertySheet.props \
    presets.csv \
    Help.htm \
    Firmware.txt \
    build.tmpl \
    ReleaseNotes.txt

HEADERS += \
    usb.h \
    spectrumwidget.h \
    SoundCard.h \
    SoftRock.h \
    smeterwidget.h \
    SignalStrength.h \
    SignalSpectrum.h \
    SignalProcessing.h \
    Settings.h \
    SDR_IQ.h \
    SDR.h \
    ReceiverWidget.h \
    Receiver.h \
    presets.h \
    pebbleii.h \
    NoiseFilter.h \
    NoiseBlanker.h \
    NCO.h \
    Mixer.h \
    IIRFilter.h \
    GPL.h \
    FIRFilter.h \
    ElektorSDR.h \
    Demod.h \
    defs.h \
    cpx.h \
    Butterworth.h \
	#build.h \
    AGC.h \
    audio.h \
    audioqt.h \
    hpsdr.h \
    fftbasic.h \
	iqbalance.h \
    global.h \
    usbutil.h \
    funcube.h \
    hidapi.h \
    sdrfile.h \
    goertzel.h \
    morse.h \
    extio.h \
    rtl2832.h \
    rtl-sdr/rtl-sdr.h \
    rtl-sdr/tuner_fc2580.h \
    rtl-sdr/tuner_fc0013.h \
    rtl-sdr/tuner_fc0012.h \
    rtl-sdr/tuner_e4k.h \
    rtl-sdr/rtlsdr_i2c.h \
    rtl-sdr/rtl-sdr_export.h \
    filters/fir.h \
    filters/filtercoef.h \
    filters/iir.h \
    demod/wfmdemod.h \
    demod/rbdsconstants.h \
    demod/downconvert.h \
    demod/rdsdecode.h

SOURCES += \
    spectrumwidget.cpp \
    SoundCard.cpp \
    SoftRock.cpp \
    smeterwidget.cpp \
    SignalStrength.cpp \
    SignalSpectrum.cpp \
    SignalProcessing.cpp \
    Settings.cpp \
    SDR_IQ.cpp \
    SDR.cpp \
    ReceiverWidget.cpp \
    Receiver.cpp \
    presets.cpp \
    pebbleii.cpp \
    NoiseFilter.cpp \
    NoiseBlanker.cpp \
    NCO.cpp \
    Mixer.cpp \
    main.cpp \
    IIRFilter.cpp \
    FIRFilter.cpp \
    ElektorSDR.cpp \
    Demod.cpp \
    cpx.cpp \
    Butterworth.cpp \
    AGC.cpp \
    audio.cpp \
    audioqt.cpp \
    hpsdr.cpp \
    fftbasic.cpp \
    iqbalance.cpp \
    global.cpp \
    usbutil.cpp \
	funcube.cpp \
    sdrfile.cpp \
    goertzel.cpp \
    morse.cpp \
    rtl2832.cpp \
    rtl-sdr/rtl-sdr.c \
    rtl-sdr/tuner_fc2580.c \
    rtl-sdr/tuner_fc0013.c \
    rtl-sdr/tuner_fc0012.c \
    rtl-sdr/tuner_e4k.c \
    filters/fir.cpp \
    filters/iir.cpp \
    demod/wfmdemod.cpp \
    demod/downconvert.cpp \
    demod/rdsdecode.cpp

FORMS += \
    spectrumwidget.ui \
    SoftRockOptions.ui \
    smeterwidget.ui \
    Settings.ui \
    SDRIQOptions.ui \
    ReceiverWidget.ui \
    presets.ui \
    pebbleii.ui \
    ElektorOptions.ui \
    hpsdrOptions.ui \
    iqbalanceoptions.ui \
    funcubeoptions.ui

RESOURCES += \
    resources.qrc \
    pebbleii.qrc
