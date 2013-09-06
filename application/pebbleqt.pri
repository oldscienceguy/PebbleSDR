#Common to all Pebble projects
cache()



macx {
	#debug and release may both be defined as .pro file is parsed by make multiple times
	#This tests for debug as the last item to be defined amoung debug and release
	CONFIG(debug, debug|release) {
		DESTDIR = $${PWD}/../MacDebug
	} else {
		DESTDIR = $${PWD}/../MacRelease
	}
	#Set location to UI auto-generated files so we can get headers from known location
	message("PWD = "$${PWD})
	UI_DIR = $${PWD}/UI
	RCC_DIR = $${PWD}/UI
	OBJECTS_DIR = $${PWD}/OMac
	#Locataion for MOC files
	MOC_DIR = $${PWD}/MocMac
	message("UI_HEADERS = "$${UI_DIR})


}
win32 {
}
