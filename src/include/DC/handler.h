/***************************************************************************
 *  file: handler.h , Handler module.                      Part of DIKUMUD *
 *  Usage: Various routines for moving about objects/players               *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: handler.h,v 1.33 2012/01/17 01:03:26 jhhudso Exp $ */

#pragma once

#include <QString>

/* flag bit values for affect_remove() */
constexpr auto SUPPRESS_CONSEQUENCES = 1;
constexpr auto SUPPRESS_MESSAGES = 2;
constexpr auto SUPPRESS_ALL = (SUPPRESS_CONSEQUENCES | SUPPRESS_MESSAGES);

/* utility */

QString fname(QString namelist);
qint32 isprefix(const QString str, const QString namel);
// END TIMERS
/* ******** objects *********** */

/* Generic Find */

qint32 get_number(QString *name);
qint32 get_number(QString &name);
qint32 get_number(QString &name);

constexpr auto FIND_CHAR_ROOM = 1;
constexpr auto FIND_CHAR_WORLD = 2;
constexpr auto FIND_OBJ_INV = 4;
constexpr auto FIND_OBJ_ROOM = 8;
constexpr auto FIND_OBJ_WORLD = 16;
constexpr auto FIND_OBJ_EQUIP = 32;

class ErrorHandler
{
public:
  class underrun
  {
  };
  class overrun
  {
  };
};
