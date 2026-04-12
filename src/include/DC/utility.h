/***************************************************************************
 *  file: utils.h, Utility module.                         Part of DIKUMUD *
 *  Usage: Utility macros                                                  *
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
 *                                                                         *
 *  Revision History                                                       *
 *  10/21/2003   Onager    Changed IS_ANONYMOUS() to handle mobs without   *
 *                         crashing                                        *
 ***************************************************************************/
/* $Id: utility.h,v 1.100 2014/07/27 00:20:02 jhhudso Exp $ */

#pragma once
#include <QString>

template <typename T, typename U, typename... Args>
void dc_sprintf(T &buffer, U cformat, Args... args)
{
  buffer = QString::asprintf(cformat, args...);
}

template <typename T, typename U, typename... Args>
void dc_snprintf(T &buffer, int, U cformat, Args... args)
{
  buffer = QString::asprintf(cformat, args...);
}