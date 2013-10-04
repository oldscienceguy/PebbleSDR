#ifndef PEBBLELIB_GLOBAL_H
#define PEBBLELIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(PEBBLELIB_LIBRARY)
#  define PEBBLELIBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define PEBBLELIBSHARED_EXPORT Q_DECL_IMPORT
#endif

//Enums
//Temp ifndef while we move things around

enum DEMODMODE {
    dmAM,
    dmSAM,
    dmFMN,
    dmFMM,
    dmFMS,
    dmDSB,
    dmLSB,
    dmUSB,
    dmCWL,
    dmCWU,
    dmDIGL,
    dmDIGU,
    dmNONE
};


#endif // PEBBLELIB_GLOBAL_H
