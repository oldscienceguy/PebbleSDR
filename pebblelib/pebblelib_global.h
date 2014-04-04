#ifndef PEBBLELIB_GLOBAL_H
#define PEBBLELIB_GLOBAL_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include <QtCore/qglobal.h>

#if defined(PEBBLELIB_LIBRARY)
#  define PEBBLELIBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define PEBBLELIBSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // PEBBLELIB_GLOBAL_H
