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
#include <cstring>
#include <cctype>

#include "DC/obj.h"
#include "DC/mobile.h"
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/player.h"
#include "DC/levels.h"
#include "DC/clan.h"
#include "DC/room.h"
#include "DC/weather.h"
#include "DC/handler.h"
#include "DC/terminal.h"
#include "DC/interp.h"
#include "DC/connect.h"
#include "DC/db.h" // help_index_element
#include "DC/spells.h"
#include "DC/returnvals.h"

/* Used for "who" */
int max_who = 0;

/* extern functions */

void page_string(class Connection *d, const char *str, int keep_internal);

int do_levels(Character *ch, char *argument, int cmd)
{
	int i;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
	{
		ch->sendln("You ain't nothin' but a hound-dog.");
		return eSUCCESS;
	}

	buf[0] = '\0';

	for (i = 1; i <= DC::MAX_MORTAL_LEVEL; i++)
		sprintf(buf + strlen(buf), "[%2d] %9d\n\r", i, exp_table[i]);

	page_string(ch->desc, buf, 1);
	return eSUCCESS;
}
