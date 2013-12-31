#Common to all Pebble projects
cache()

#Compile with C++11 so we can use <funtional>
CONFIG += c++11
#QMAKE_CXXFLAGS += -stdlib=libc++


macx {
	#debug and release may both be defined as .pro file is parsed by make multiple times
	#This tests for debug as the last item to be defined amoung debug and release

	#Set location to UI auto-generated files so we can get headers from known location
	message("PWD = "$${PWD})
	message("PRO_FILE_PWD = "$$_PRO_FILE_PWD_)
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
}
win32 {
}

#This will let us include ui header files without worrying about path
INCLUDEPATH += $$UI_DIR
