#!/bin/bash
# Fix up any frameworks in a .app file in case macdeployqt is not fixing links

#Usage
# sudo ./qtfix.sh Pebble.app
#if command not found, check execute permission ls -l qtfix.sh then chmod +x qtfix.sh
#if macdeployqt not found, edit .bash_profile and add path

macdeployqt $1

function fixpaths {
       QTLIBS=`otool -L $1 | egrep ".*Qt.*framework.*Qt.*" | awk '{ print $1 }'`
       echo "in $1 QTLIBS=" $QTLIBS
       for j in $QTLIBS; do
               qtlib=""
               for k in $(echo $j | tr "/" " "); do
                       qtlib=$k
               done
               install_name_tool -change $j @executable_path/../Frameworks/$qtlib.framework/Versions/5/$qtlib $1
       done
}
for i in `find $1/Contents/MacOS -type f`; do
	fixpaths $i
done
for i in `find $1/Contents/Frameworks -type f`; do
	fixpaths $i
done
for i in `find $1/Contents -name "*.dylib"`; do
	fixpaths $i
done
