#Common library dependency code for all Pebble plugins

#We need to set where pebblelib is expected (app frameworks) to be found so it doesn't have to be installed on mac
pebblelib.path += $${DESTDIR}
pebblelib.commands += install_name_tool -change libpebblelib.1.dylib @rpath/libpebblelib.1.dylib $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += pebblelib

#In 5.2, Qt plugins that reference a different library than application cause errors (Application must be defined before widget)
#Fix is to make sure all plugins reference libraries in app Frameworks directory
#QT libraries already exist in pebble.app... form macdeployqt in pebbleqt.pro, so we don't copy - just fix
#Check dependencies with otool if still looking for installed QT libraries
qtlib1.path += $${DESTDIR}
qtlib1.commands += install_name_tool -change $$(QTDIR)/lib/QtWidgets.framework/Versions/5/QtWidgets @rpath/QtWidgets.framework/Versions/5/QtWidgets $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib1

qtlib2.path += $${DESTDIR}
qtlib2.commands += install_name_tool -change $$(QTDIR)/lib/QtGui.framework/Versions/5/QtGui  @rpath/QtGui.framework/Versions/5/QtGui $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib2

qtlib3.path += $${DESTDIR}
qtlib3.commands += install_name_tool -change $$(QTDIR)/lib/QtCore.framework/Versions/5/QtCore  @rpath/QtCore.framework/Versions/5/QtCore $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib3

qtlib4.path += $${DESTDIR}
qtlib4.commands += install_name_tool -change $$(QTDIR)/lib/QtNetwork.framework/Versions/5/QtNetwork @rpath//QtNetwork.framework/Versions/5/QtNetwork $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib4
