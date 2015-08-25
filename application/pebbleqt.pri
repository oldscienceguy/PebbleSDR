#Common to all Pebble projects
cache()

#Compile with C++11 so we can use <funtional>
CONFIG += c++11
#QMAKE_CXXFLAGS += -mmacosx-version-min=10.8
#QMAKE_CXXFLAGS += -std=c++11
#QMAKE_CXXFLAGS += -stdlib=libc++
#This fixes a nasty symbols not resolved problem that happened when I did a clean install of
#   Mavericks, XCode 5, & QT5.2.1
#Qt was using the 10.8 sdk installed by XCode, but that sdk is missing symbols
#Not sure if we still need this is Yosemite 10.10
QMAKE_MAC_SDK = macosx10.9

message("PWD = "$${PWD})
message("PRO_FILE_PWD = "$$_PRO_FILE_PWD_)

macx {
	#debug and release may both be defined as .pro file is parsed by make multiple times
	#This tests for debug as the last item to be defined amoung debug and release

	#Set location to UI auto-generated files so we can get headers from known location
	CONFIG(debug, debug|release) {
		DESTDIR = $${PWD}/../MacDebug
		OBJECTS_DIR = $${_PRO_FILE_PWD_}/OMacDebug
		MOC_DIR = $${_PRO_FILE_PWD_}/MocMacDebug
		UI_DIR = $${_PRO_FILE_PWD_}/UIMacDebug
		RCC_DIR = $${_PRO_FILE_PWD_}/UIMacDebug
		LIB_DIR = LibMacDebug
	} else {
		DESTDIR = $${PWD}/../MacRelease
		OBJECTS_DIR = $${_PRO_FILE_PWD_}/OMacRelease
		MOC_DIR = $${_PRO_FILE_PWD_}/MocMacRelease
		UI_DIR = $${_PRO_FILE_PWD_}/UIMacRelease
		RCC_DIR = $${_PRO_FILE_PWD_}/UIMacRelease
		LIB_DIR = LibMacRelease
	}
        # This adds to the list of files that are deleted when make clean is executed by QTCreator
        # Not sure what '*=' does, but on Mac '+=' doesn't work
        QMAKE_CLEAN *= $${DESTDIR}/*
        QMAKE_CLEAN *= $${OBJECTS_DIR}/*
        QMAKE_CLEAN *= $${MOC_DIR}/*
        QMAKE_CLEAN *= $${UI_DIR}/*
        QMAKE_CLEAN *= $${RCC_DIR}/*
        QMAKE_CLEAN *= $${LIB_DIR}/*
        #message("Clean " $${QMAKE_CLEAN})

        #FFT library choices are FFTW, FFTCUTE, FFTOOURA
        #DEFINES += USE_FFTW
        #DEFINES += USE_FFTCUTE
        #DEFINES += USE_FFTOOURA
        DEFINES += USE_FFTACCELERATE

        #Audio library choices are PORTAUDIO, QTAUDIO
        #WARNING: When you change this, delete pebble.ini or manually reset all settings because input/output names may change
        #Also make sure audio.cpp is re-compiled
        #Mutually exclusive, uncomment only one
        DEFINES += USE_QT_AUDIO
        #DEFINES += USE_PORT_AUDIO

}
win32 {
        CONFIG(debug, debug|release) {
                DESTDIR = $${PWD}/../WinDebug
                OBJECTS_DIR = $${_PRO_FILE_PWD_}/OWinDebug
                MOC_DIR = $${_PRO_FILE_PWD_}/MocWinDebug
                UI_DIR = $${_PRO_FILE_PWD_}/UIWinDebug
                RCC_DIR = $${_PRO_FILE_PWD_}/UIWinDebug
                LIB_DIR = LibWinDebug
        } else {
                DESTDIR = $${PWD}/../WinRelease
                OBJECTS_DIR = $${_PRO_FILE_PWD_}/OWinRelease
                MOC_DIR = $${_PRO_FILE_PWD_}/MocWinRelease
                UI_DIR = $${_PRO_FILE_PWD_}/UIWinRelease
                RCC_DIR = $${_PRO_FILE_PWD_}/UIWinRelease
                LIB_DIR = LibMacRelease
        }
        #FFT library choices are FFTW, FFTCUTE, FFTOOURA
        #DEFINES += USE_FFTW
        DEFINES += USE_FFTCUTE
        #DEFINES += USE_FFTOOURA

        #Audio library choices are PORTAUDIO, QTAUDIO
        #WARNING: When you change this, delete pebble.ini or manually reset all settings because input/output names may change
        #Also make sure audio.cpp is re-compiled
        #Mutually exclusive, uncomment only one
        #DEFINES += USE_QT_AUDIO
        DEFINES += USE_PORT_AUDIO
}

#This will let us include ui header files without worrying about path
INCLUDEPATH += $$UI_DIR
