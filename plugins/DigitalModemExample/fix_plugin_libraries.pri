#Common library dependency code for all Pebble plugins


#We need to set where pebblelib is expected (app frameworks) to be found so it doesn't have to be installed on mac
pebblelib.path += $${DESTDIR}
pebblelib.commands += install_name_tool -change libpebblelib.1.dylib @executable_path/../Frameworks/libpebblelib.1.dylib $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += pebblelib

#In 5.2, Qt plugins that reference a different library than application cause errors (Application must be defined before widget)
#Fix is to make sure all plugins reference libraries in app Frameworks directory
#QT libraries already exist in pebble.app... form macdeployqt in pebbleqt.pro, so we don't copy - just fix
#Check dependencies with otool if still looking for installed QT libraries
qtlib1.path += $${DESTDIR}
qtlib1.commands += install_name_tool -change /Users/rlandsman/Qt/5.2.0/clang_64/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib1

qtlib2.path += $${DESTDIR}
qtlib2.commands += install_name_tool -change /Users/rlandsman/Qt/5.2.0/clang_64/lib/QtGui.framework/Versions/5/QtGui  @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib2

qtlib3.path += $${DESTDIR}
qtlib3.commands += install_name_tool -change /Users/rlandsman/Qt/5.2.0/clang_64/lib/QtCore.framework/Versions/5/QtCore  @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore $${DESTDIR}/lib$${TARGET}.dylib
INSTALLS += qtlib3
