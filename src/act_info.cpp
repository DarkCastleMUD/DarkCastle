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
extern "C"
{
  #include <string.h>
  #include <ctype.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <obj.h>
#include <mobile.h>
#include <character.h>
#include <utility.h>
#include <player.h>
#include <levels.h>
#include <clan.h>
#include <room.h>
#include <weather.h>
#include <handler.h>
#include <terminal.h>
#include <interp.h>
#include <connect.h>
#include <db.h> // help_index_element
#include <spells.h>
#include <returnvals.h>

/* Used for "who" */
int max_who = 0;

/* extern functions */

void page_string(struct descriptor_data *d, const char *str, int keep_internal);

int do_levels(struct char_data *ch, char *argument, int cmd) {
	int i;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch)) {
		send_to_char("You ain't nothin' but a hound-dog.\n\r", ch);
		return eSUCCESS;
	}

	buf[0] = '\0';

	for (i = 1; i <= MAX_MORTAL; i++)
		sprintf(buf + strlen(buf), "[%2d] %9d\n\r", i, exp_table[i]);

	page_string(ch->desc, buf, 1);
	return eSUCCESS;
}


