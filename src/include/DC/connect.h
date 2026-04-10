/************************************************************************
| $Id: connect.h,v 1.14 2011/08/28 18:29:45 jhhudso Exp $
| connect.h
| Description: State of connectedness information.
*/
#pragma once

#include <QHostAddress>
#include <QMap>

// if you change, make sure you update QStringList connected_states in const.C
// also update connected_types[]

constexpr auto MAX_RAW_INPUT_LENGTH = 512;
