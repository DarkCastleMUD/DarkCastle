/***************************************************************************
 *  file: act_info.c , Implementation of commands.         Part of DIKUMUD *
 *  Usage : Informative commands.                                          *
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
#include "DC/DC.h"
#include "DC/db.h"
#include <cstring>

/* Used for "who" */
qint32 max_who = {};

/* extern functions */

void page_string(class Connection *d, const QString str, qint32 keep_internal);

command_return_t do_levels(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 i;
  QString buf;

  if (ch->isNonPlayer())
  {
    ch->sendln("You ain't nothin' but a hound-dog.");
    return ReturnValue::eSUCCESS;
  }

  buf[0] = '\0';

  for (i = 1; i <= DC::MAX_MORTAL_LEVEL; i++)
    sprintf(buf + strlen(buf), "[%2d] %9d\r\n", i, exp_table[i]);

  page_string(ch->desc, buf, 1);
  return ReturnValue::eSUCCESS;
}
