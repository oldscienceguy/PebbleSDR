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
}
win32 {
}
