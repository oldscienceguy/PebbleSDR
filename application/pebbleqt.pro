#Project common
include(pebbleqt.pri)

TEMPLATE = app
TARGET = Pebble
#QT5 requires explicit add of Widgets to add QtWidgets which was in QtGui in earlier releases
QT += widgets core gui multimedia

#Download & install QT 5.2 from http://qt-project.org
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

#Download PortAudio from portaudio.com , open terminal
#Get latest SVN and make sure configure.in has 10.8 and 10.9 switch statment
# autoreconf -if
# ./configure
# make
#portaudio directory is at same level as PebbleII

#if autoreconf not found, sudo port install automake autoconf libtool #macports

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

#Specific device plugins, like rtl2832, have their own pre-build installation requirements
#See the .pro file for each device

#Enable this to look at config to debug conditionals. For example: debug and release both show up sometimes
#message($$CONFIG)

# !!! add make install as additional build step in QT Projects for both Debug and Release to copy all the necessary files

#So we don't have to specify paths everywhere
INCLUDEPATH += ../pebblelib
DEPENDPATH += ../pebblelib

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
	#VERSION = $$system(/opt/subversion/bin/svn info -r HEAD . | grep 'Changed\\ Rev' | cut -b 19-)
	#!isEmpty(VERSION){
	#  VERSION = 0.$${VERSION}
	#}
	#VERSION = '\\"$${VERSION}\\"' #puts escaped strings so VERSION is \"0.123\"
	VERSION = '\\"$$system(git rev-list --count HEAD)\\"'
	message($${VERSION})
	#make it available to code
	# NO SPACES !!!
	DEFINES += PEBBLE_VERSION=\"$${VERSION}\"

	#DATE = $$system(/opt/subversion/bin/svn info -r HEAD . | grep 'Changed\\ Date' | cut -b 19-)
	#DATE = '\\"$${DATE}\\"' #puts escaped strings so VERSION is \"0.123\"
	DATE = '\\"$$system(date)\\"'
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

	#dylib
        #This can be removed when all devices have been moved to plugins, also any fixups
	LIBS += -L$${PWD}/../D2XX/bin/10.5-10.7/ -lftd2xx.1.2.2
	#static lib
	LIBS += $${PWD}/../portaudio/lib/.libs/libportaudio.a
	#Portaudio needs mac frameworks, this is how to add them
	LIBS += -framework CoreAudio
	LIBS += -framework AudioToolbox
	LIBS += -framework AudioUnit
	LIBS += -framework CoreServices
	LIBS += -framework IOKit

	LIBS += -L$${PWD}/../pebblelib/$${LIB_DIR} -lpebblelib.1

	#Don't know if we need these
	#INCLUDEPATH += ../portaudio/include
	#LIBPATH += ../portaudio/lib
	#INCLUDEPATH adds directories for header file searches
	INCLUDEPATH += $${PWD}/../D2XX/bin/10.5-10.7
	DEPENDPATH += $${PWD}/../D2XX/bin/10.5-10.7

	#Mac only source files
	#HIDAPI
	SOURCES += devices/hid-mac.c

	#INSTALLS is called when we manually make -install or add it to the Qt project build steps
	pebbleData.files = eibireadme.txt eibi.csv bands.csv memory.csv	gpl.h ../readme.md hpsdr/ozyfw-sdr1k.hex hpsdr/ozy_janus.rbf

	#We want this to be whatever the release or debug directory is or added to bundle
	#This will copy the files into the app bundle, same place ini files go
	#pebbleData.path = $${DESTDIR}/Pebble.app/Contents/MacOS
	#QMake will create folder in path if they don't exist
	pebbleData.path = $${DESTDIR}/PebbleData
	#message($${pebbleData.path})
	INSTALLS += pebbleData

	# Get .dylib files in package for easier user install
	# See http://qt-project.org/doc/qt-5.0/qtdoc/deployment-mac.html

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
	#1/26/14: Stopped using macdeployqt because we are setting @rpath to our lib folder
	#All apps and plugins expect to find lib folder at same level as application
	#Easier to make sure everyone shares same instance
	#QMAKE_POST_LINK += macdeployqt $${DESTDIR}/Pebble.app
	#message("Reminder - macdeployqt 5.1 has bug and must be run from fix_macdeployqt script")

	# macdeployqt fixes references, but does not copy non QT dylib.  We have to do this manually
	#mylib.files += $${PWD}/../pebblelib/$${LIB_DIR}/libpebblelib.1.dylib
	#mylib.path = $${DESTDIR}/Pebble.app/Contents/Frameworks
	#INSTALLS += mylib

	#This anchors @rpath references in plugins to our lib directory, always at the same level os plugin directory
	QMAKE_LFLAGS += -rpath $${DESTDIR}/lib

	#We may need to copy ftd2xx.cfg with value ConfigFlags=0x40000000
	#ftd2xx.files += $${PWD}/../D2XX/bin/10.5-10.7/libftd2xx.1.2.2.dylib
	ftd2xx.commands += install_name_tool -change /usr/local/lib/libftd2xx.1.2.2.dylib @rpath/libftd2xx.1.2.2.dylib $${DESTDIR}/pebble.app/contents/macos/pebble
	ftd2xx.path = $${DESTDIR}/lib
	INSTALLS += ftd2xx

	plib.commands += install_name_tool -change libpebblelib.1.dylib @rpath/libpebblelib.1.dylib $${DESTDIR}/pebble.app/contents/macos/pebble
	plib.path = $${DESTDIR}/lib
	INSTALLS += plib

	#Qt files
	qt1.files += $$(QTDIR)/lib/QtMultimedia.framework/Versions/5/QtMultimedia
	qt1.commands += install_name_tool -change $$(QTDIR)/lib/QtMultimedia.framework/Versions/5/QtMultimedia @rpath/QtMultimedia.framework/Versions/5/QtMultimedia $${DESTDIR}/pebble.app/contents/macos/pebble
	qt1.path = $${DESTDIR}/lib/QtMultimedia.framework/Versions/5
	INSTALLS += qt1

	qt2.files += $$(QTDIR)/lib/QtGui.framework/Versions/5/QtGui
	qt2.commands += install_name_tool -change $$(QTDIR)/lib/QtGui.framework/Versions/5/QtGui @rpath/QtGui.framework/Versions/5/QtGui $${DESTDIR}/pebble.app/contents/macos/pebble
	qt2.path = $${DESTDIR}/lib/QtGui.framework/Versions/5
	INSTALLS += qt2

	qt3.files += $$(QTDIR)/lib/QtCore.framework/Versions/5/QtCore
	qt3.commands += install_name_tool -change $$(QTDIR)/lib/QtCore.framework/Versions/5/QtCore @rpath/QtCore.framework/Versions/5/QtCore $${DESTDIR}/pebble.app/contents/macos/pebble
	qt3.path = $${DESTDIR}/lib/QtCore.framework/Versions/5
	INSTALLS += qt3

	qt4.files += $$(QTDIR)/lib/QtNetwork.framework/Versions/5/QtNetwork
	qt4.commands += install_name_tool -change $$(QTDIR)/lib/QtNetwork.framework/Versions/5/QtNetwork @rpath/QtNetwork.framework/Versions/5/QtNetwork $${DESTDIR}/pebble.app/contents/macos/pebble
	qt4.path = $${DESTDIR}/lib/QtNetwork.framework/Versions/5
	INSTALLS += qt4

	qt5.files += $$(QTDIR)/lib/QtWidgets.framework/Versions/5/QtWidgets
	qt5.commands += install_name_tool -change $$(QTDIR)/lib/QtWidgets.framework/Versions/5/QtWidgets @rpath/QtWidgets.framework/Versions/5/QtWidgets $${DESTDIR}/pebble.app/contents/macos/pebble
	qt5.path = $${DESTDIR}/lib/QtWidgets.framework/Versions/5
	INSTALLS += qt5

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
	SOURCES += devices/hid-win.c

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
		pebbleData.files = eibireadme.txt eibi.csv bands.csv memory.csv gpl.h readme.md

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
    firmware.txt \
    build.tmpl \
    bands.csv \
    eibi.csv \
    eibireadme.txt \
    memory.csv \
	frequencylist-santacruz.csv \
    pebble.qss \
    hpsdr/ozyfw-sdr1k.hex \
    hpsdr/ozy_janus.rbf \
    codingstyle.txt \
    pebbleqt.pri

HEADERS += \
    spectrumwidget.h \
	soundcard.h \
    smeterwidget.h \
	signalstrength.h \
	signalspectrum.h \
	signalprocessing.h \
	settings.h \
	sdr.h \
	receiverwidget.h \
	receiver.h \
    presets.h \
    pebbleii.h \
	noisefilter.h \
	noiseblanker.h \
	nco.h \
	mixer.h \
	iirfilter.h \
	firfilter.h \
	demod.h \
    defs.h \
	butterworth.h \
	#build.h \
	agc.h \
    audio.h \
    audioqt.h \
	iqbalance.h \
    global.h \
    goertzel.h \
    filters/fir.h \
	demod/demod_wfm.h \
    demod/rbdsconstants.h \
    demod/rdsdecode.h \
    filters/fractresampler.h \
    devices/sdr_iq.h \
    devices/hpsdr.h \
    devices/funcube.h \
    devices/elektorsdr.h \
    devices/usbutil.h \
    devices/usb.h \
    devices/extio.h \
    devices/hidapi.h \
    devices/sdr_ip.h \
    devices/sdr-ip/dataprocess.h \
    devices/sdr-ip/ad6620.h \
    devices/sdr-ip/ascpmsg.h \
    devices/sdr-ip/netiobase.h \
    devices/sdr-ip/sdrinterface.h \
    devices/sdr-ip/threadwrapper.h \
    devices/sdr-ip/protocoldefs.h \
    devices/wavfile.h \
    bargraphmeter.h \
    testbench.h \
    demod/demod_am.h \
    demod/demod_sam.h \
    demod/demod_nfm.h \
    plugins.h \
    sdroptions.h

SOURCES += \
    spectrumwidget.cpp \
	soundcard.cpp \
    smeterwidget.cpp \
	signalstrength.cpp \
	signalspectrum.cpp \
	signalprocessing.cpp \
	settings.cpp \
	sdr.cpp \
	receiverwidget.cpp \
	receiver.cpp \
    presets.cpp \
    pebbleii.cpp \
	noisefilter.cpp \
	noiseblanker.cpp \
	nco.cpp \
	mixer.cpp \
    main.cpp \
	iirfilter.cpp \
	firfilter.cpp \
	demod.cpp \
	butterworth.cpp \
	agc.cpp \
    audio.cpp \
    audioqt.cpp \
    iqbalance.cpp \
    global.cpp \
    goertzel.cpp \
    filters/fir.cpp \
	demod/demod_wfm.cpp \
    demod/rdsdecode.cpp \
    filters/fractresampler.cpp \
    devices/sdr_iq.cpp \
    devices/hpsdr.cpp \
    devices/funcube.cpp \
    devices/elektorsdr.cpp \
	devices/usbutil.cpp \
    devices/sdr_ip.cpp \
    devices/sdr-ip/dataprocess.cpp \
    devices/sdr-ip/ad6620.cpp \
    devices/sdr-ip/netiobase.cpp \
    devices/sdr-ip/sdrinterface.cpp \
    devices/wavfile.cpp \
    bargraphmeter.cpp \
    testbench.cpp \
    demod/demod_am.cpp \
    demod/demod_sam.cpp \
    demod/demod_nfm.cpp \
    plugins.cpp \
    sdroptions.cpp

FORMS += \
    spectrumwidget.ui \
    smeterwidget.ui \
    sdriqoptions.ui \
    receiverwidget.ui \
    pebbleii.ui \
    elektoroptions.ui \
    hpsdroptions.ui \
    iqbalanceoptions.ui \
    funcubeoptions.ui \
    sdr-ip.ui \
    sdr.ui \
    directinput.ui \
    bargraphmeter.ui \
    testbench.ui \
    data-band.ui

RESOURCES += \
    resources.qrc
