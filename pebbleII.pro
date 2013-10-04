#PebbleII Subdir project
cache()

TEMPLATE = subdirs

# Modifiers, mydir.file = foo.pro
#	.subdir	Use the specified subdirectory instead of SUBDIRS value.
#	.file	Specify the subproject pro file explicitly. Cannot be used in conjunction with .subdir modifier.
#	.condition	Specifies a bld.inf define that must be true for subproject to be built. Available only on Symbian platform.
#	.depends	This subproject depends on specified subproject. Available only on platforms that use makefiles.
#	.makefile	The makefile of subproject. Available only on platforms that use makefiles.
#	.target	Base string used for makefile targets related to this subproject. Available only on platforms that use makefiles.

SUBDIRS =	pebblelib \
			application/pebbleqt.pro \
			plugins/DigitalModemExample \
			plugins/MorseDigitalModem \
			plugins/RttyDigitalModem \
			plugins/WWVDigitalModem

# build must be last:
#build in the order listed
CONFIG += ordered
#SUBDIRS += build
