#pragma once

#include <QtCore/qglobal.h>

#if defined(DC_LIBRARY)
#define DC_EXPORT Q_DECL_EXPORT
#else
#define DC_EXPORT Q_DECL_IMPORT
#endif
