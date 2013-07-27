cache()

TEMPLATE = app
TARGET = Pebble
#QT5 requires explicit add of Widgets to add QtWidgets which was in QtGui in earlier releases
QT += widgets core gui multimedia

#Download & install QT 5.0 from http://qt-project.org
#Edit Projects and make sure Kits use Clang
#Edit Projects and add 'make install' as additional build step
#QML Debugging can be turned off, not using QML
#QT 5.1 crashes in main() if QML debugging is not turned off
#QT 5.1 generates OSX not supported warnings for 10.8.4 (Mountain Lion)? Maybe in QMutex?

#Subversion files are found at http://www.wandisco.com/subversion/download#osx
#Edit QtCreator prefs so subversion is found in /opt/subversion/bin/svn (mac default svn is old)  Enter username and pw

#Download & install FTDI D2XX drivers from http://www.ftdichip.com/Drivers/D2XX.htm
#V 1.1.0 4/25/11
#V 1.2.2 10/30/12

#Download PortAudio from portaudio.com , open terminal, ./configure && make
#portaudio directory is at same level as PebbleII

#Download FFTW from fftw.org, open terminal, build floating point
#Current version 3.3.1 as of 2/2013
#Swithed from float to double so we can switch between FFTW and Ooura libraries.  Don't add --enable-float to ./configure
#./configure
# make
#sudo make install

#Header is in api directory, lib is in ./libs directory
#Or prebuilt fftw from pdb.finkproject.org

#Dev Setup notes
#To support UNC paths for Parallels, Regedit HKEY_CURRENTUSER/Software/Microsoft/Command Processor
#and add the value DisableUNCCheck REG_DWORD and set the value to 0 x 1 (Hex).
#Make sure to update project version if Qt version is updated in QtCreator
#Exit code 0x0c0000315 if "Release DLLs" not copied to Debug and Release dirs

#RTL2832 original source code: git clone git://git.osmocom.org/rtl-sdr.git
#Copy relevant files to Pebble source tree and make sure attribution comments are not removed

#Set location to UI auto-generated files so we can get headers from known location
message("PWD = "$${PWD})
UI_DIR = $${PWD}/UI
RCC_DIR = $${PWD}/UI
message("UI_HEADERS = "$${UI_DIR})

#Enable this to look at config to debug conditionals. For example: debug and release both show up sometimes
#message($$CONFIG)


#Conditional MUST match the Build Configuration name, Debug or Release or SomeCustomName
macx {

	#After much agony, finally figured out that this icon only appears in release, NOT debug builds
	#There doesn't appear to be any need to use setWindowIcon() either, just this in make file
	#But Pebble.icns must be specified in resources.qrc
	#Use Sketch to edit Pebble Icons
	#Use http://iconverticons.com/online/ to convert to icns file
	ICON = Resources/Pebble.icns

	#WARNING: Switching computers caused this .pro file to not load due to a bad certificate for asembla subversion
	#Ran Qmake in terminal to see error, accepted cert, and file loads ok
	#Get most recent checkin version number.  Assumes svn installed as in comments above
	#See http://www.qtcentre.org/wiki/index.php?title=Version_numbering_using_QMake
	#Note: This only gets re-generated when QMake is run and global.cpp compiled, not on every build
	VERSION = $$system(/opt/subversion/bin/svn info -r HEAD . | grep 'Changed\\ Rev' | cut -b 19-)
	!isEmpty(VERSION){
	  VERSION = 0.$${VERSION}
	}
	VERSION = '\\"$${VERSION}\\"' #puts escaped strings so VERSION is \"0.123\"
	message($${VERSION})
	#make it available to code
	# NO SPACES !!!
	DEFINES += PEBBLE_VERSION=\"$${VERSION}\"

	DATE = $$system(/opt/subversion/bin/svn info -r HEAD . | grep 'Changed\\ Date' | cut -b 19-)
	DATE = '\\"$${DATE}\\"' #puts escaped strings so VERSION is \"0.123\"
	message($${DATE})
	#make it available to code
	# NO SPACES !!!
	DEFINES += PEBBLE_DATE=\"$${DATE}\"

	#Turn off warnings for unused variables so we can focus on real warnings
	#We completely replace this variable with our own warning flags in the order we want them
	#This turns on all warnings, then turns off selected ones
	QMAKE_CXXFLAGS_WARN_ON = "-Wall"
	QMAKE_CXXFLAGS_WARN_ON += "-Wno-unused-variable"

	#This will stop the creation of a Mac package (default) and just create a unix bin if we need to for testing
	#CONFIG-=app_bundle

	#debug and release may both be defined as .pro file is parsed by make multiple times
	#This tests for debug as the last item to be defined amoung debug and release
	CONFIG(debug, debug|release) {
		DESTDIR = ../MacDebug
	} else {
		DESTDIR = ../MacRelease
	}

	OBJECTS_DIR = $${PWD}/MacO
	#Locataion for MOC files
	MOC_DIR = $${PWD}/MacMoc

	#dylib
	LIBS += -L$${PWD}/../D2XX/bin/10.5-10.7/ -lftd2xx.1.2.2
	#static lib
	LIBS += ../portaudio/lib/.libs/libportaudio.a
	#Portaudio needs mac frameworks, this is how to add them
	LIBS += -framework CoreAudio
	LIBS += -framework AudioToolbox
	LIBS += -framework AudioUnit
	LIBS += -framework CoreServices
	##fftw
	LIBS += -L$${PWD}/../fftw-3.3.1/.libs/ -lfftw3

	LIBS += /System/Library/Frameworks/CoreFoundation.framework/CoreFoundation \
		/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit

	#Don't know if we need these
	#INCLUDEPATH += ../portaudio/include
	#LIBPATH += ../portaudio/lib
	#INCLUDEPATH adds directories for header file searches
	INCLUDEPATH += $${PWD}/../D2XX/bin/10.5-10.7
	DEPENDPATH += $${PWD}/../D2XX/bin/10.5-10.7

	#Mac only source files
	#HIDAPI
	SOURCES += Devices/hid-mac.c

	#INSTALLS is called when we manually make -install or add it to the Qt project build steps
	pebbleData.files = eibireadme.txt eibi.csv bands.csv memory.csv Help.htm gpl.h ReleaseNotes.txt HPSDR/ozyfw-sdr1k.hex HPSDR/Ozy_Janus.rbf

	#We want this to be whatever the release or debug directory is or added to bundle
	#This will copy the files into the app bundle, same place ini files go
	#pebbleData.path = $${DESTDIR}/Pebble.app/Contents/MacOS
	#QMake will create folder in path if they don't exist
	pebbleData.path = $${DESTDIR}/PebbleData
	#message($${pebbleData.path})
	INSTALLS += pebbleData

	# Get .dylib files in package for easier user install
	# See http://qt-project.org/doc/qt-5.0/qtdoc/deployment-mac.html
	# Check out macdeployqt and see if it automates QT changes below
	# /Users/rlandsman/QT5.0.0/5.0.0/clang_64/bin/macdeployqt

	#To find all dependent dylib: otool -L ./Pebble.app/contents/macos/pebble >> dylib.txt
	#1st arg is the path and name of dylib as shown from otool ouput
	#2nd arg starts with @executable_path which expands to (path to bundle)/pebble.app/contents/macos
	#Up one dir to Frameworks which is where we copy .dylib files
	#3rd arg is the actual executable to patch
	#Add this command to QtCreator make steps or use QMAKE_POST_LINK below
	#install_name_tool -change /usr/local/lib/libftd2xx.1.1.0.dylib @executable_path/../Frameworks/libftd2xx.1.1.0.dylib pebble.app/contents/macos/pebble

	#macdeployqt replaces all the install detail below and handles Qt and any non-system dylibs
	#Turn this off if you are having any problem with libraries or plugins
	#Starting in QT5.02 the cocoa plugin is always required, so we can't use -no-plugins
	QMAKE_POST_LINK += macdeployqt $${DESTDIR}/Pebble.app

	#We may need to copy ftd2xx.cfg with value ConfigFlags=0x40000000
	#ftd2xx.files += $${PWD}/../D2XX/bin/10.5-10.7/libftd2xx.1.2.2.dylib
	#ftd2xx.commands += install_name_tool -change /usr/local/lib/libftd2xx.1.2.2.dylib @executable_path/../Frameworks/libftd2xx.1.2.2.dylib $${DESTDIR}/pebble.app/contents/macos/pebble
	#ftd2xx.path = $${DESTDIR}/Pebble.app/Contents/Frameworks
	#INSTALLS += ftd2xx

	#Qt files
	#qt1.files += /Users/rlandsman/Qt5.0.0/5.0.0/clang_64/lib/QtMultimedia.framework/Versions/5/QtMultimedia
	#qt1.commands += install_name_tool -change /Users/rlandsman/Qt5.0.0/5.0.0/clang_64/lib/QtMultimedia.framework/Versions/5/QtMultimedia @executable_path/../Frameworks/QtMultimedia $${DESTDIR}/pebble.app/contents/macos/pebble
	#qt1.path = $${DESTDIR}/Pebble.app/Contents/Frameworks
	#INSTALLS += qt1

	#qt2.files += /Users/rlandsman/Qt5.0.0/5.0.0/clang_64/lib/QtGui.framework/Versions/5/QtGui
	#qt2.commands += install_name_tool -change /Users/rlandsman/Qt5.0.0/5.0.0/clang_64/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui $${DESTDIR}/pebble.app/contents/macos/pebble
	#qt2.path = $${DESTDIR}/Pebble.app/Contents/Frameworks
	#INSTALLS += qt2

	#qt3.files += /Users/rlandsman/Qt5.0.0/5.0.0/clang_64/lib/QtCore.framework/Versions/5/QtCore
	#qt3.commands += install_name_tool -change /Users/rlandsman/Qt5.0.0/5.0.0/clang_64/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore $${DESTDIR}/pebble.app/contents/macos/pebble
	#qt3.path = $${DESTDIR}/Pebble.app/Contents/Frameworks
	#INSTALLS += qt3

	#qt4.files += /Users/rlandsman/Qt5.0.0/5.0.0/clang_64/lib/QtNetwork.framework/Versions/5/QtNetwork
	#qt4.commands += install_name_tool -change /Users/rlandsman/Qt5.0.0/5.0.0/clang_64/lib/QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork $${DESTDIR}/pebble.app/contents/macos/pebble
	#qt4.path = $${DESTDIR}/Pebble.app/Contents/Frameworks
	#INSTALLS += qt4

	#qt5.files += /Users/rlandsman/Qt5.0.0/5.0.0/clang_64/lib/QtWidgets.framework/Versions/5/QtWidgets
	#qt5.commands += install_name_tool -change /Users/rlandsman/Qt5.0.0/5.0.0/clang_64/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets $${DESTDIR}/pebble.app/contents/macos/pebble
	#qt5.path = $${DESTDIR}/Pebble.app/Contents/Frameworks
	#INSTALLS += qt5

	#
	# Check to see if these are standard OS files
	# /usr/lib/libstdc++.6.dylib
	# /usr/lib/libSystem.B.dylib

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
	SOURCES += Devices/hid-win.c

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
		pebbleData.files = eibireadme.txt eibi.csv bands.csv memory.csv Help.htm gpl.h ReleaseNotes.txt

		#We want this to be whatever the release or debug directory is or added to bundle
		pebbleData.path = $$DESTDIR
		INSTALLS += pebbleData

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
    Help.htm \
    Firmware.txt \
    build.tmpl \
    ReleaseNotes.txt \
    bands.csv \
    eibi.csv \
    eibireadme.txt \
    memory.csv \
	FrequencyList-SantaCruz.csv \
    pebble.qss \
    HPSDR/ozyfw-sdr1k.hex \
    HPSDR/Ozy_Janus.rbf

HEADERS += \
    spectrumwidget.h \
    SoundCard.h \
    smeterwidget.h \
    SignalStrength.h \
    SignalSpectrum.h \
    SignalProcessing.h \
    Settings.h \
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
    Demod.h \
    defs.h \
    cpx.h \
    Butterworth.h \
	#build.h \
    AGC.h \
    audio.h \
    audioqt.h \
    fftbasic.h \
	iqbalance.h \
    global.h \
    goertzel.h \
    filters/fir.h \
    filters/filtercoef.h \
    filters/iir.h \
    demod/wfmdemod.h \
    demod/rbdsconstants.h \
    demod/downconvert.h \
    demod/rdsdecode.h \
    filters/fractresampler.h \
    filters/datatypes.h \
    DSP/fftooura.h \
    perform.h \
    Devices/sdrfile.h \
    Devices/rtl2832.h \
    Devices/SDR_IQ.h \
    Devices/SoftRock.h \
    Devices/hpsdr.h \
    Devices/funcube.h \
    Devices/ElektorSDR.h \
    Devices/usbutil.h \
    Devices/usb.h \
    Devices/extio.h \
    Devices/hidapi.h \
    Devices/rtl-sdr/rtl-sdr.h \
    Devices/rtl-sdr/rtl-sdr_export.h \
    Devices/rtl-sdr/rtlsdr_i2c.h \
    Devices/rtl-sdr/tuner_e4k.h \
    Devices/rtl-sdr/tuner_fc0012.h \
    Devices/rtl-sdr/tuner_fc0013.h \
    Devices/rtl-sdr/tuner_fc2580.h \
    Devices/rtl-sdr/tuner_r820t.h \
    Devices/rtl-sdr/reg_field.h \
    Devices/sdr_ip.h \
    Devices/sdr-ip/dataprocess.h \
    Devices/sdr-ip/ad6620.h \
    Devices/sdr-ip/ascpmsg.h \
    Devices/sdr-ip/netiobase.h \
    Devices/sdr-ip/sdrinterface.h \
    Devices/sdr-ip/threadwrapper.h \
    Devices/sdr-ip/protocoldefs.h \
    Devices/wavfile.h \
    Decoders/OregonScientific/oregonscientificsensordecode.h \
    Decoders/morse/morse.h \
    Decoders/fldigifilters.h \
    bargraphmeter.h

SOURCES += \
    spectrumwidget.cpp \
    SoundCard.cpp \
    smeterwidget.cpp \
    SignalStrength.cpp \
    SignalSpectrum.cpp \
    SignalProcessing.cpp \
    Settings.cpp \
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
    Demod.cpp \
    cpx.cpp \
    Butterworth.cpp \
    AGC.cpp \
    audio.cpp \
    audioqt.cpp \
    fftbasic.cpp \
    iqbalance.cpp \
    global.cpp \
    goertzel.cpp \
    filters/fir.cpp \
    filters/iir.cpp \
    demod/wfmdemod.cpp \
    demod/downconvert.cpp \
    demod/rdsdecode.cpp \
    filters/fractresampler.cpp \
    DSP/fftooura.cpp \
    perform.cpp \
    Devices/sdrfile.cpp \
    Devices/rtl2832.cpp \
    Devices/SDR_IQ.cpp \
    Devices/SoftRock.cpp \
    Devices/hpsdr.cpp \
    Devices/funcube.cpp \
    Devices/ElektorSDR.cpp \
	Devices/usbutil.cpp \
    Devices/rtl-sdr/librtlsdr.c \
    Devices/rtl-sdr/tuner_e4k.c \
    Devices/rtl-sdr/tuner_fc0012.c \
    Devices/rtl-sdr/tuner_fc0013.c \
    Devices/rtl-sdr/tuner_fc2580.c \
    Devices/rtl-sdr/tuner_r820t.c \
    Devices/sdr_ip.cpp \
    Devices/sdr-ip/dataprocess.cpp \
    Devices/sdr-ip/ad6620.cpp \
    Devices/sdr-ip/netiobase.cpp \
    Devices/sdr-ip/sdrinterface.cpp \
    Devices/wavfile.cpp \
    Decoders/OregonScientific/oregonscientificsensordecode.cpp \
	Decoders/morse/morse.cpp \
    Decoders/fldigifilters.cpp \
    bargraphmeter.cpp

FORMS += \
    spectrumwidget.ui \
    SoftRockOptions.ui \
    smeterwidget.ui \
    SDRIQOptions.ui \
    ReceiverWidget.ui \
    pebbleii.ui \
    ElektorOptions.ui \
    hpsdrOptions.ui \
    iqbalanceoptions.ui \
    funcubeoptions.ui \
    sdr-ip.ui \
    sdr.ui \
    directinput.ui \
    data-morse.ui \
    bargraphmeter.ui

RESOURCES += \
    resources.qrc
