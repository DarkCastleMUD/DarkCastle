/***************************************************************************
 *  file: obj.h , Structures                               Part of DIKUMUD *
 *  Usage: Declarations of object data structures                          *
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
/* $Id: obj.h,v 1.36 2015/06/16 04:10:54 pirahna Exp $ */
#pragma once

#include <QStringList>
#include <QMetaEnum>

/* The following defs are for Object  */

/* For 'type_flag' */

/* for containers  - value[1] */

constexpr auto CONT_CLOSEABLE = 1;
constexpr auto CONT_PICKPROOF = 2;
constexpr auto CONT_CLOSED = 4;
constexpr auto CONT_LOCKED = 8;

constexpr auto OBJ_NOTIMER = -7000000;

/* Bitvector For 'wear_flags' */

// constexpr auto TAKE = 1;

/* For 'equipment' */

/* ***********************************************************************
 *  file element for object file. BEWARE: Changing it will ruin the file  *
 *********************************************************************** */

constexpr auto CURRENT_OBJ_VERSION = 1;

class obj_file_elem
{
public:
  qint16 version = {};
  qint32 item_number = {};
  qint16 timer = {};
  qint16 wear_pos = {};
  qint16 container_depth = {};
  qint32 other[5] = {}; // unused
};

// functions from objects.cpp
