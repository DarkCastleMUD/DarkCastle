/***************************************************************************
 *  file: handler.c , Handler module.                      Part of DIKUMUD *
 *  Usage: Various routines for moving about objects/players               *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as th s   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: handler.cpp,v 1.208 2013/03/27 03:32:58 jhhudso Exp $ */

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cassert>

#include <sys/time.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <map>

#include <QString>

#include "magic.h"
#include "spells.h"
#include "room.h"
#include "obj.h"
#include "character.h"
#include "player.h"	 // APPLY
#include "utility.h" // LOWER
#include "clan.h"
#include "levels.h"
#include "db.h"
#include "mobile.h"
#include "connect.h" // Connection
#include "handler.h" // FIND_CHAR_ROOM
#include "interp.h"	 // do_return
#include "act.h"
#include "isr.h"
#include "race.h"
#include "fight.h"
#include "returnvals.h"
#include "innate.h"
#include "set.h"
#include "DC.h"
#include "const.h"
#include "corpse.h"
#include "shop.h"

extern CWorld world;

extern class Object *object_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;

void huntclear_item(class Object *obj);

#ifdef WIN32
int strncasecmp(char *s1, const char *s2, int len);
#endif

/* External procedures */
int do_fall(Character *ch, short dir);

// TIMERS

bool isTimer(Character *ch, int spell)
{
	return affected_by_spell(ch, BASE_TIMERS + spell);
}
int timerLeft(Character *ch, int spell)
{
	struct affected_type *af = affected_by_spell(ch, BASE_TIMERS + spell);
	if (af == nullptr)
		return 0;
	else
		return af->duration;
}
void addTimer(Character *ch, int spell, int ticks)
{

	struct affected_type af;
	af.duration = ticks;
	af.type = spell + BASE_TIMERS;
	af.location = 0;
	af.bitvector = -1;
	af.modifier = 0;
	affect_join(ch, &af, true, false);
	return;
}
// END TIMERS

bool is_wearing(Character *ch, Object *item)
{
	int i;
	for (i = 0; i < MAX_WEAR; i++)
		if (ch->equipment[i] == item)
			return true;
	return false;
}

// This grabs the first "word" (defined as group of alphaBETIC chars)
// puts it in a static char buffer, and returns it.
char *fname(const char *namelist) char *fname(const char *namelist)
{
	static char holder[30];
	char *point;

	for (point = holder; isalpha(*namelist); namelist++, point++)
		*point = *namelist;

	// we seem to use this alot, and 30 chars isn't a whole lot.....
	// let's just put this here for the heck of it -pir 2/28/01
	if (point > (holder + 29))
		logentry("point overran holder in fname (handler.c)", ARCHITECT, LogChannels::LOG_BUG);

	*point = '\0';

	return (holder);
}

// TODO - figure out how this is different from isname() and comment why
// we need it.  Neither of them are case-sensitive....
int isname2(const char *str, const char *namel)
{
	const char *s = namel;

	if (strlen(str) == 0)
		return 0;

	for (;;)
	{
		if (!strncasecmp(str, s, strlen(str)))
			return 1;

		for (; isalpha(*s); s++)
			;
		if (!*s)
			return (0);
		s++; /* first char of new_new name */
	}

	return 0;
}

int isname(QString arg, QStringList namelist)
{
	for (auto &name : namelist)
	{
		if (arg.compare(name, Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

int isname(QString arg, QString namelist)
{
	return isname(arg.toStdString().c_str(), namelist.toStdString().c_str());
}

int isname(QString arg, const char *namelist)
{
	return isname(arg.toStdString().c_str(), namelist);
}

int isname(string arg, string namelist)
{
	return isname(arg.c_str(), namelist.c_str());
}

int isname(string arg, const char *namelist)
{
	return isname(arg.c_str(), namelist);
}

int isname(const char *arg, string namelist)
{
	return isname(arg, namelist.c_str());
}

int isname(const char *arg, joining_t &namelist)
{
	for (joining_t::const_iterator i = namelist.begin(); i != namelist.end(); ++i)
	{
		if (i.value() && QString(arg).compare(i.key(), Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

/************************************************************************
| isname
| Preconditions:  str != 0, namelist != 0
| Postconditions: None
| Side Effects: None
| Returns: One if it's in the namelist, zero otherwise
*/
int isname(const char *str, const char *namelist)
{
#if 0
	string haystack(namelist);
	if (haystack.find(str) != string::npos) {
		return 1;
	}

	return 0;

#endif
	const char *curname, *curstr;

	if (!str || !namelist)
	{
		return (0);
	}

	if (strlen(str) == 0)
		return 0;
	if (strlen(namelist) == 0)
		return 0;

	curname = namelist;

	for (;;)
	{
		for (curstr = str;; curstr++, curname++)
		{
			if (!*curstr && !isalpha(*curname))
				return (1);

			if (!*curname)
				return (0);

			if (!*curstr || *curname == ' ')
				break;

			if (LOWER(*curstr) != LOWER(*curname))
				break;
		}

		/* skip to next name */

		for (; isalpha(*curname); curname++)
			;
		if (!*curname)
			return (0);
		curname++; /* first char of new_new name */
	}

	return 0;
}

int get_max_stat(Character *ch, uint8_t stat)
{
	switch (GET_RACE(ch))
	{
	case RACE_ELVEN:
		switch (stat)
		{
		case STRENGTH:
			return 24;
		case DEXTERITY:
			return 27;
		case INTELLIGENCE:
			return 27;
		case WISDOM:
			return 24;
		case CONSTITUTION:
			return 23;
		}
		break;
	case RACE_TROLL:
		switch (stat)
		{
		case STRENGTH:
			return 28;
		case DEXTERITY:
			return 25;
		case INTELLIGENCE:
			return 20;
		case WISDOM:
			return 22;
		case CONSTITUTION:
			return 30;
		}
		break;

	case RACE_DWARVEN:
		switch (stat)
		{
		case STRENGTH:
			return 27;
		case DEXTERITY:
			return 22;
		case INTELLIGENCE:
			return 22;
		case WISDOM:
			return 26;
		case CONSTITUTION:
			return 28;
		}
		break;

	case RACE_HOBBIT:
		switch (stat)
		{
		case STRENGTH:
			return 22;
		case DEXTERITY:
			return 30;
		case INTELLIGENCE:
			return 25;
		case WISDOM:
			return 25;
		case CONSTITUTION:
			return 23;
		}
		break;

	case RACE_PIXIE:
		switch (stat)
		{
		case STRENGTH:
			return 20;
		case DEXTERITY:
			return 28;
		case INTELLIGENCE:
			return 30;
		case WISDOM:
			return 27;
		case CONSTITUTION:
			return 20;
		}
		break;

	case RACE_GIANT:
		switch (stat)
		{
		case STRENGTH:
			return 30;
		case DEXTERITY:
			return 23;
		case INTELLIGENCE:
			return 22;
		case WISDOM:
			return 23;
		case CONSTITUTION:
			return 27;
		}
		break;

	case RACE_GNOME:
		switch (stat)
		{
		case STRENGTH:
			return 22;
		case DEXTERITY:
			return 22;
		case INTELLIGENCE:
			return 27;
		case WISDOM:
			return 30;
		case CONSTITUTION:
			return 24;
		}
		break;

	case RACE_ORC:
		switch (stat)
		{
		case STRENGTH:
			return 27;
		case DEXTERITY:
			return 25;
		case INTELLIGENCE:
			return 24;
		case WISDOM:
			return 23;
		case CONSTITUTION:
			return 26;
		}
		break;

	case RACE_HUMAN:
	default:
		return 25;
		break;
	}

	return 0;
}

bool still_affected_by_poison(Character *ch)
{
	struct affected_type *af = ch->affected;

	while (af)
	{
		if (IS_SET(af->bitvector, AFF_POISON))
			return 1;
		af = af->next;
	}
	return 0;
}

const struct set_data set_list[] = {
	{"Ascetic's Focus",
	 19,
	 {2700, 6904, 8301, 8301, 9567, 9567, 12108, 14805, 15621, 21718, 22302, 22314, 22600, 22601, 22602, 24815, 24815, 24816, 26237},
	 "You attach your penis mightier.\r\n",
	 "You remove your penis mightier.\r\n"},
	{"Warlock's Vestments",
	 7,
	 {17334, 17335, 17336, 17337, 17338, 17339, 17340, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "You feel an increase in energy coursing through your body.\r\n",
	 "Your magical energy returns to its normal state.\r\n"},
	{"Hunter's Arsenal",
	 7,
	 {17327, 17328, 17329, 17330, 17331, 17332, 17333, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "You sense your accuracy and endurance have undergone a magical improvement.\r\n",
	 "Your accuracy and endurance return to their normal levels.\r\n"},
	{"Captain's Regalia",
	 7,
	 {17341, 17342, 17343, 17344, 17345, 17346, 17347, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "You feel an increased vigor surge throughout your body.\n",
	 "Your vigor is reduced to its normal state.\n"},
	{"Celebrant's Defenses",
	 7,
	 {17319, 17320, 17321, 17322, 17323, 17324, 17325, 17326, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "Your inner focus feels as though it is more powerful.\r\n",
	 "Your inner focus has reverted to its original form.\r\n"},
	{"Battlerager's Fury",
	 18,
	 {352, 353, 354, 355, 356, 357, 358, 359, 359, 360, 361, 362, 362, 9702, 9808, 9808, 27114, 27114, -1},
	 "You feel your stance harden and blood boil as you strap on your battlerager's gear.\r\n",
	 "Your blood returns to its normal temperature as you remove your battlerager's gear.\r\n"},
	{"Veteran's Field Plate Set",
	 9,
	 {21719, 21720, 21721, 21722, 21723, 21724, 21725, 21726, 21727, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "There is an audible *click* as the field plate locks into its optimal assembly.\r\n",
	 "There is a soft *click* as you remove the field plate from its optimal positioning.\r\n"},
	{"Mother of All Dragons",
	 7,
	 {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "You feel the might of the ancient dragonkind surge through your body.\r\n",
	 "The might of the ancient dragonkind has left you.\r\n"},
	{"Feral Fangs",
	 2,
	 {4818, 4819, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "As the fangs' cries harmonize they unleash something feral inside you.\r\n",
	 "As you remove the fangs your muscles relax and focus is restored.\r\n"},
	{"White Crystal Armours",
	 10,
	 {22003, 22006, 22010, 22014, 22015, 22020, 22021, 22022, 22023, 22024, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "Your crystal armours begin to $B$7glow$R with an ethereal light.\r\n",
	 "The $B$7glow$R fades from your crystal armours.\r\n"},
	{"Black Crystal Armours",
	 7,
	 {22011, 22017, 22025, 22026, 22027, 22028, 22029, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "Your crystal armours $B$0darken$R and begin to hum with magic.\r\n",
	 "The $B$0darkness$R disperses as the crystal armours are removed.\r\n"},
	{"Aqua Pendants",
	 2,
	 {5611, 5643, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "The pendants click softly and you feel a surge of energy as gills spring from your neck!\r\n",
	 "Your gills retract and fade as the two pendants separate quietly.\r\n"},
	{"Arcane Apparatus",
	 19,
	 {367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 379, 380, 381, 382, 383, 383},
	 "The power of ancient magicks surges through your body as you slowly fade out of existence.\r\n",
	 "You feel a slight tingle of fading magicks as you shimmer into existence.\r\n"},
	{"Titanic Gear",
	 11,
	 {19402, 19404, 19406, 19407, 19408, 19409, 19410, 19411, 19413, 19417, 19419, -1, -1, -1, -1, -1, -1, -1, -1},
	 "You feel a mighty surge as your body rapidly expands.\r\n",
	 "You feel your size reduce to normal proportions.\r\n"},
	{"Moss Equipment",
	 11,
	 {18001, 18002, 18003, 18004, 18006, 18008, 18009, 18010, 18011, 18016, 18017, -1, -1, -1, -1, -1, -1, -1, -1},
	 "A strange energy surges through you and you feel your senses sharpen.\r\n",
	 "Your senses return to normal as you remove your mossy garb.\r\n"},
	{"Blacksteel Battlegear",
	 19,
	 {283, 283, 284, 284, 285, 286, 287, 288, 289, 290, 292, 293, 294, 294, 295, 296, 296, 297, 291},
	 "The might of the warrior's spirit, past, present, and future, hums through your body.\r\n",
	 "The harmony of the warrior's spirit has left you.\r\n"},
	{"Mother of All Dragons",
	 7,
	 {22323, 22330, 22331, 22332, 22334, 22335, 22336, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "You feel the might of the ancient dragonkind surge through your body.\r\n",
	 "The might of the ancient dragonkind has left you.\r\n"},
	{"Berkthgar's Rage",
	 1,
	 {27977, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 nullptr,
	 nullptr},
	{"The Naturalists Trappings",
	 17,
	 {331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 343, 344, 345, 346, 347, 347},
	 "You feel the spirit of the wolf upon you.\r\n",
	 "The spirit of the wolf leaves you.\r\n"},
	{"Troubadour's Finery",
	 16,
	 {314, 314, 316, 317, 318, 319, 320, 321, 322, 328, 328, 324, 325, 325, 326, 327, -1, -1, -1},
	 "You feel the need to sing, you are a STAR!\r\n",
	 "Your desire to sing like a star has faded.\r\n"},
	{"\n",
	 0,
	 {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 "\n",
	 "\n"}};

void add_set_stats(Character *ch, Object *obj, int flag, int pos)
{
	// obj has just been worn
	int obj_vnum = obj_index[obj->getNumber()].virt;
	int i;
	int z = 0, y;
	// Quadruple nested for. Annoying, but it's gotta be done.

	for (; *(set_list[z].SetName) != '\n'; z++)
		for (y = 0; y < 19 && set_list[z].vnum[y] != -1; y++)
			if (set_list[z].vnum[y] == obj_vnum)
			{ // Aye, 'tis part of a set.
				if ((obj_vnum == 4818 || obj_vnum == 4819) && pos == WEAR_HANDS)
					continue;
				for (y = 0; y < 19 && set_list[z].vnum[y] != -1; y++)
				{
					if (set_list[z].vnum[y] == 17326 && GET_CLASS(ch) != CLASS_BARD)
						continue;
					if (set_list[z].vnum[y] == 17325 && GET_CLASS(ch) != CLASS_MONK)
						continue;
					bool found = false, doublea = false;
					for (i = 0; i < MAX_WEAR; i++)
					{
						if (ch->equipment[i] && obj_index[ch->equipment[i]->getNumber()].virt == set_list[z].vnum[y])
						{
							if (y > 0 && !doublea && set_list[z].vnum[y] == set_list[z].vnum[y - 1])
							{
								doublea = true;
								continue;
							}
							found = true;
							break;
						}
					}
					if (!found)
					{
						return;
					} // Nope.
				}
				struct affected_type af;
				af.duration = -1;
				af.bitvector = -1;
				af.type = BASE_SETS + z;
				af.location = APPLY_NONE;
				af.modifier = 0;
				// By gawd, they just completed the set.
				if (affected_by_spell(ch, BASE_SETS + z))
					return;
				if (!flag && set_list[z].Set_Wear_Message != nullptr)
					send_to_char(set_list[z].Set_Wear_Message, ch);
				switch (z)
				{
				case SET_SAIYAN: // (aka Ascetic's Focus)
					af.bitvector = AFF_HASTE;
					affect_to_char(ch, &af);
					break;
				case SET_VESTMENTS:
					af.location = APPLY_MANA;
					af.modifier = 75;
					affect_to_char(ch, &af);
					break;
				case SET_HUNTERS:
					af.location = APPLY_HITROLL;
					af.modifier = 5;
					affect_to_char(ch, &af);
					af.location = APPLY_MOVE;
					af.modifier = 40;
					affect_to_char(ch, &af);
					break;
				case SET_FERAL:
					af.location = APPLY_HITROLL;
					af.modifier = 2;
					affect_to_char(ch, &af);
					af.location = APPLY_HIT;
					af.modifier = 10;
					affect_to_char(ch, &af);
					af.location = APPLY_DAMROLL;
					af.modifier = 1;
					affect_to_char(ch, &af);
					break;
				case SET_CAPTAINS:
					af.location = APPLY_HIT;
					af.modifier = 75;
					affect_to_char(ch, &af);
					break;
				case SET_CELEBRANTS:
					af.location = APPLY_KI;
					af.modifier = 8;
					affect_to_char(ch, &af);
					break;
				case SET_RAGER:
					af.location = 0;
					af.modifier = 0;
					af.bitvector = AFF_STABILITY;
					affect_to_char(ch, &af);
					af.location = SKILL_BLOOD_FURY * 1000;
					af.modifier = 5;
					af.bitvector = -1;
					affect_to_char(ch, &af);
					break;
				case SET_RAGER2:
					af.location = 0;
					af.location = SKILL_BLOOD_FURY * 1000;
					af.modifier = 5;
					af.bitvector = -1;
					affect_to_char(ch, &af);
					break;
				case SET_FIELDPLATE:
					af.location = APPLY_HIT;
					af.modifier = 100;
					affect_to_char(ch, &af);
					af.location = APPLY_HIT_N_DAM;
					af.modifier = 6;
					affect_to_char(ch, &af);
					af.location = APPLY_AC;
					af.modifier = -40;
					affect_to_char(ch, &af);
					break;
				case SET_MOAD:
					af.location = APPLY_HIT_N_DAM;
					af.modifier = 4;
					affect_to_char(ch, &af);
					af.location = APPLY_SAVING_FIRE;
					af.modifier = 15;
					affect_to_char(ch, &af);
					af.location = APPLY_MELEE_DAMAGE;
					af.modifier = 5;
					affect_to_char(ch, &af);
					af.location = APPLY_SONG_DAMAGE;
					af.modifier = 5;
					affect_to_char(ch, &af);
					break;
				case SET_MOAD2:
					af.location = APPLY_HIT_N_DAM;
					af.modifier = 4;
					affect_to_char(ch, &af);
					af.location = APPLY_SAVING_FIRE;
					af.modifier = 15;
					affect_to_char(ch, &af);
					af.location = APPLY_MELEE_DAMAGE;
					af.modifier = 5;
					affect_to_char(ch, &af);
					af.location = APPLY_SONG_DAMAGE;
					af.modifier = 5;
					affect_to_char(ch, &af);
					break;
				case SET_WHITECRYSTAL:
					af.location = APPLY_HITROLL;
					af.modifier = 10;
					affect_to_char(ch, &af);
					af.location = APPLY_HIT;
					af.modifier = 50;
					affect_to_char(ch, &af);
					af.location = APPLY_MANA;
					af.modifier = 50;
					affect_to_char(ch, &af);
					af.location = APPLY_SAVING_MAGIC;
					af.modifier = 8;
					affect_to_char(ch, &af);
					break;
				case SET_BLACKCRYSTAL:
					af.location = APPLY_HIT_N_DAM;
					af.modifier = 4;
					affect_to_char(ch, &af);
					af.location = APPLY_MANA;
					af.modifier = 80;
					affect_to_char(ch, &af);
					af.location = APPLY_SAVING_ACID;
					af.modifier = 8;
					affect_to_char(ch, &af);
					af.location = APPLY_SAVING_ENERGY;
					af.modifier = 8;
					affect_to_char(ch, &af);
					break;
				case SET_AQUA:
					af.bitvector = AFF_WATER_BREATHING;
					affect_to_char(ch, &af);
					af.bitvector = -1;
					af.location = APPLY_HIT_N_DAM;
					af.modifier = 2;
					affect_to_char(ch, &af);
					break;
				case SET_APPARATUS:
					af.bitvector = AFF_INVISIBLE;
					affect_to_char(ch, &af);
					af.bitvector = -1;
					af.location = APPLY_HP_REGEN;
					af.modifier = 10;
					affect_to_char(ch, &af);
					break;
				case SET_TITANIC:
					af.location = APPLY_STR;
					af.modifier = 3;
					affect_to_char(ch, &af);
					af.location = APPLY_INT;
					affect_to_char(ch, &af);
					af.location = APPLY_HIT;
					af.modifier = 25;
					affect_to_char(ch, &af);
					af.location = APPLY_MANA;
					affect_to_char(ch, &af);
					af.location = APPLY_MOVE;
					affect_to_char(ch, &af);
					af.location = APPLY_CHAR_HEIGHT;
					af.modifier = 30;
					affect_to_char(ch, &af);
					af.location = APPLY_CHAR_WEIGHT;
					af.modifier = 60;
					affect_to_char(ch, &af);
					break;
				case SET_MOSS:
					af.bitvector = AFF_INFRARED;
					affect_to_char(ch, &af);
					af.bitvector = -1;
					af.location = APPLY_HIT;
					af.modifier = 25;
					affect_to_char(ch, &af);
					af.location = APPLY_KI;
					af.modifier = 5;
					affect_to_char(ch, &af);
					af.location = APPLY_MOVE;
					af.modifier = 50;
					affect_to_char(ch, &af);
					af.location = APPLY_HIT_N_DAM;
					af.modifier = 5;
					affect_to_char(ch, &af);
					af.location = APPLY_WIS;
					af.modifier = 3;
					affect_to_char(ch, &af);
					break;
				case SET_BLACKSTEEL:
					af.bitvector = AFF_FLYING;
					af.location = APPLY_FLY;
					af.modifier = 0;
					affect_to_char(ch, &af);

					af.bitvector = -1;
					af.location = APPLY_ARMOR;
					af.modifier = -100;
					affect_to_char(ch, &af);

					af.bitvector = -1;
					af.location = APPLY_SAVES;
					af.modifier = 25;
					affect_to_char(ch, &af);
					break;

				case SET_TRAPPINGS:
					af.bitvector = AFF_HASTE;
					affect_to_char(ch, &af);

					af.bitvector = -1;
					af.location = APPLY_MANA_REGEN;
					af.modifier = 15;
					affect_to_char(ch, &af);
					break;

				case SET_FINERY:
					af.location = APPLY_KI_REGEN;
					af.modifier = 5;
					affect_to_char(ch, &af);
					break;

				default:
					send_to_char("Tough luck, you completed an unimplemented set. Report what you just wore, eh?\r\n", ch);
					break;
				}
				break;
			}
}

void remove_set_stats(Character *ch, Object *obj, int flag)
{
	// obj has just been removed
	int obj_vnum = obj_index[obj->getNumber()].virt;
	int i;
	int z = 0, y;
	// Quadruply nested for. Annoying, but it's gotta be done.
	// I'm sure "quadruply" is a word.
	for (; *(set_list[z].SetName) != '\n'; z++)
		for (y = 0; y < 19 && set_list[z].vnum[y] != -1; y++)
			if (set_list[z].vnum[y] == obj_vnum)
			{ // Aye, 'tis part of a set.
				for (y = 0; y < 19 && set_list[z].vnum[y] != -1; y++)
				{
					if (set_list[z].vnum[y] == 17326 && GET_CLASS(ch) != CLASS_BARD)
						continue;
					if (set_list[z].vnum[y] == 17325 && GET_CLASS(ch) != CLASS_MONK)
						continue;
					bool found = false, doublea = false;
					for (i = 0; i < MAX_WEAR; i++)
					{
						if (ch->equipment[i] && obj_index[ch->equipment[i]->getNumber()].virt == set_list[z].vnum[y])
						{
							if (y > 0 && !doublea && set_list[z].vnum[y] == set_list[z].vnum[y - 1])
							{
								doublea = true;
								continue;
							}
							found = true;
							break;
						}
					}
					if (!found)
						return; // Nope.
				}
				// Remove it
				affect_from_char(ch, BASE_SETS + z);
				if (!flag && set_list[z].Set_Remove_Message != nullptr)
					send_to_char(set_list[z].Set_Remove_Message, ch);
				break;
			}
}

void check_weapon_weights(Character *ch)
{
	class Object *weapon;

	if (ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT))
		return;
	// make sure we're still strong enough to wield our weapons
	if (IS_PC(ch) && ch->equipment[WIELD] &&
		GET_OBJ_WEIGHT(ch->equipment[WIELD]) > GET_STR(ch) && !ISSET(ch->affected_by, AFF_POWERWIELD))
	{
		act("Being too heavy to wield, you move your $p to your inventory.", ch, ch->equipment[WIELD], 0, TO_CHAR, 0);
		act("$n stops using $p.", ch, ch->equipment[WIELD], 0, TO_ROOM, INVIS_NULL);
		obj_to_char(unequip_char(ch, WIELD), ch);
		if (ch->equipment[SECOND_WIELD])
		{
			act("You move your $p to be your primary weapon.", ch, ch->equipment[SECOND_WIELD], 0, TO_CHAR, INVIS_NULL);
			act("$n moves $s $p to be $s primary weapon.", ch, ch->equipment[SECOND_WIELD], 0, TO_ROOM, INVIS_NULL);
			weapon = unequip_char(ch, SECOND_WIELD);
			equip_char(ch, weapon, WIELD);
			check_weapon_weights(ch); // Not a loop, since it'll only happen once.
									  // It'll recheck primary wield.
		}
	}

	if (ch->equipment[SECOND_WIELD] &&
		GET_OBJ_WEIGHT(ch->equipment[SECOND_WIELD]) > GET_STR(ch) / 2 && !ISSET(ch->affected_by, AFF_POWERWIELD))
	{
		act("Being too heavy to wield, you move your $p to your inventory.", ch, ch->equipment[SECOND_WIELD], 0, TO_CHAR, 0);
		act("$n stops using $p.", ch, ch->equipment[SECOND_WIELD], 0, TO_ROOM, INVIS_NULL);
		obj_to_char(unequip_char(ch, SECOND_WIELD), ch);
	}

	if (ch->equipment[WEAR_SHIELD] && ((ch->equipment[WIELD] &&
										GET_OBJ_WEIGHT(ch->equipment[WIELD]) > GET_STR(ch) && !ISSET(ch->affected_by, AFF_POWERWIELD)) ||
									   (ch->equipment[SECOND_WIELD] &&
										GET_OBJ_WEIGHT(ch->equipment[SECOND_WIELD]) > GET_STR(ch) / 2 && !ISSET(ch->affected_by, AFF_POWERWIELD))))
	{
		act("You shift your shield into your inventory.", ch, ch->equipment[WEAR_SHIELD], 0, TO_CHAR, 0);
		act("$n stops using $p.", ch, ch->equipment[WEAR_SHIELD], 0, TO_ROOM, INVIS_NULL);
		obj_to_char(unequip_char(ch, WEAR_SHIELD), ch);
	}
}

void affect_modify(Character *ch, int32_t loc, int32_t mod, int32_t bitv, bool add, int flag)
{
	char log_buf[256];
	int i;

	if (add && IS_PC(ch))
	{
		if (loc == APPLY_LIGHTNING_SHIELD)
		{
			if (affected_by_spell(ch, SPELL_LIGHTNING_SHIELD))
			{
				affect_from_char(ch, SPELL_LIGHTNING_SHIELD);
			}

			if (affected_by_spell(ch, SPELL_FIRESHIELD))
			{
				affect_from_char(ch, SPELL_FIRESHIELD);

				if (!flag)
				{
					send_to_char("The magic of this item clashes with your fire "
								 "shield.\r\n",
								 ch);
					act("Your $B$4flames$R have been extinguished!", ch, 0, ch, TO_VICT, 0);
					act("The $B$4flames$R encompassing $n's body are extinguished!", ch, 0, 0, TO_ROOM, 0);
				}
			}

			if (affected_by_spell(ch, SPELL_ACID_SHIELD))
			{
				affect_from_char(ch, SPELL_ACID_SHIELD);

				if (!flag)
				{
					send_to_char("The magic of this item clashes with your acid "
								 "shield.\r\n",
								 ch);
					act("Your shield of $B$2acid$R dissolves to nothing!", ch, 0, ch, TO_VICT, 0);
					act("The $B$2acid$R swirling about $n's body dissolves to nothing!", ch, 0, 0, TO_ROOM, 0);
				}
			}
		}
	}

	if (loc >= 1000)
		return;
	if (bitv != -1 && bitv <= AFF_MAX)
	{
		if (add)
			SETBIT(ch->affected_by, bitv);
		else
		{
			REMBIT(ch->affected_by, bitv);
		}
	}
	if (!add)
		mod = -mod;

	switch (loc)
	{
	case APPLY_NONE:
		break;

	case APPLY_STR:
	{
		GET_STR_BONUS(ch) += mod;
		GET_STR(ch) = GET_RAW_STR(ch) + GET_STR_BONUS(ch);
		i = get_max_stat(ch, STRENGTH);
		if (i <= GET_RAW_STR(ch))
			GET_STR(ch) = MIN(30, GET_STR(ch));
		else
			GET_STR(ch) = MIN(i, GET_STR(ch));

		GET_STR(ch) = MAX(0, GET_STR(ch));

		if (!ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT))
			check_weapon_weights(ch);
	}
	break;

	case APPLY_DEX:
	{
		GET_DEX_BONUS(ch) += mod;
		GET_DEX(ch) = GET_RAW_DEX(ch) + GET_DEX_BONUS(ch);
		i = get_max_stat(ch, DEXTERITY);
		if (i <= GET_RAW_DEX(ch))
			GET_DEX(ch) = MIN(30, GET_DEX(ch));
		else
			GET_DEX(ch) = MIN(i, GET_DEX(ch));
	}
	break;

	case APPLY_INT:
	{
		GET_INT_BONUS(ch) += mod;
		GET_INT(ch) = GET_RAW_INT(ch) + GET_INT_BONUS(ch);
		i = get_max_stat(ch, INTELLIGENCE);
		if (i <= GET_RAW_INT(ch))
			GET_INT(ch) = MIN(30, GET_INT(ch));
		else
			GET_INT(ch) = MIN(i, GET_INT(ch));
	}
	break;

	case APPLY_WIS:
	{
		GET_WIS_BONUS(ch) += mod;
		GET_WIS(ch) = GET_RAW_WIS(ch) + GET_WIS_BONUS(ch);
		i = get_max_stat(ch, WISDOM);
		if (i <= GET_RAW_WIS(ch))
			GET_WIS(ch) = MIN(30, GET_WIS(ch));
		else
			GET_WIS(ch) = MIN(i, GET_WIS(ch));
	}
	break;

	case APPLY_CON:
	{
		GET_CON_BONUS(ch) += mod;
		GET_CON(ch) = GET_RAW_CON(ch) + GET_CON_BONUS(ch);
		i = get_max_stat(ch, CONSTITUTION);
		if (i <= GET_RAW_CON(ch))
			GET_CON(ch) = MIN(30, GET_CON(ch));
		else
			GET_CON(ch) = MIN(i, GET_CON(ch));
	}
	break;

	case APPLY_SEX:
	{
		switch (GET_SEX(ch))
		{
		case sex_t::MALE:
			ch->setMale();
			break;
		case sex_t::FEMALE:
			ch->setFemale();
			break;
		default:
			break;
		}
	}
	break;

	case APPLY_CLASS:
		/* ??? GET_CLASS(ch) += mod; */
		break;

	case APPLY_LEVEL:
		/* ??? GET_LEVEL(ch) += mod; */
		break;

	case APPLY_AGE:
		if (IS_PC(ch))
			ch->player->time.birth -= ((int32_t)SECS_PER_MUD_YEAR * (int32_t)mod);
		break;

	case APPLY_CHAR_WEIGHT:
		GET_WEIGHT(ch) += mod;
		break;

	case APPLY_CHAR_HEIGHT:
		GET_HEIGHT(ch) += mod;
		break;

	case APPLY_MANA:
		ch->max_mana += mod;
		break;

	case APPLY_HIT:
		ch->max_hit += mod;
		break;

	case APPLY_MOVE:
		ch->max_move += mod;
		break;

	case APPLY_GOLD:
		break;

	case APPLY_EXP:
		break;

	case APPLY_AC:
		GET_AC(ch) += mod;
		break;

	case APPLY_HITROLL:
		GET_HITROLL(ch) += mod;
		break;

	case APPLY_DAMROLL:
		GET_DAMROLL(ch) += mod;
		break;
	case APPLY_SPELLDAMAGE:
		ch->spelldamage += mod;
		break;

	case APPLY_SAVING_FIRE:
		ch->saves[SAVE_TYPE_FIRE] += mod;
		break;
	case APPLY_SAVING_COLD:
		ch->saves[SAVE_TYPE_COLD] += mod;
		break;

	case APPLY_SAVING_ENERGY:
		ch->saves[SAVE_TYPE_ENERGY] += mod;
		break;

	case APPLY_SAVING_ACID:
		ch->saves[SAVE_TYPE_ACID] += mod;
		break;

	case APPLY_SAVING_MAGIC:
		ch->saves[SAVE_TYPE_MAGIC] += mod;
		break;

	case APPLY_SAVING_POISON:
		ch->saves[SAVE_TYPE_POISON] += mod;
		break;

	case APPLY_SAVES:
		ch->saves[SAVE_TYPE_FIRE] += mod;
		ch->saves[SAVE_TYPE_COLD] += mod;
		ch->saves[SAVE_TYPE_MAGIC] += mod;
		ch->saves[SAVE_TYPE_ENERGY] += mod;
		ch->saves[SAVE_TYPE_ACID] += mod;
		ch->saves[SAVE_TYPE_POISON] += mod;
		break;

	case APPLY_HIT_N_DAM:
	{
		GET_DAMROLL(ch) += mod;
		GET_HITROLL(ch) += mod;
	}
	break;

	case APPLY_SANCTUARY:
	{
		if (add)
		{
			SETBIT(ch->affected_by, AFF_SANCTUARY);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_SANCTUARY);
		}
	}
	break;

	case APPLY_SENSE_LIFE:
	{
		if (add)
		{
			SETBIT(ch->affected_by, AFF_SENSE_LIFE);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_SENSE_LIFE);
		}
	}
	break;

	case APPLY_DETECT_INVIS:
	{
		if (add)
		{
			SETBIT(ch->affected_by, AFF_DETECT_INVISIBLE);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_DETECT_INVISIBLE);
		}
	}
	break;

	case APPLY_INVISIBLE:
	{
		if (add)
		{
			SETBIT(ch->affected_by, AFF_INVISIBLE);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_INVISIBLE);
		}
	}
	break;

	case APPLY_SNEAK:
	{
		if (add)
		{
			SETBIT(ch->affected_by, AFF_SNEAK);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_SNEAK);
		}
	}
	break;

	case APPLY_INFRARED:
	{
		if (add)
		{
			SETBIT(ch->affected_by, AFF_INFRARED);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_INFRARED);
		}
	}
	break;

	case APPLY_HASTE:
	{
		if (add)
		{
			SETBIT(ch->affected_by, AFF_HASTE);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_HASTE);
		}
	}
	break;

	case APPLY_PROTECT_EVIL:
	{
		if (add)
		{
			SETBIT(ch->affected_by, AFF_PROTECT_EVIL);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_PROTECT_EVIL);
		}
	}
	break;

	case APPLY_FLY:
	{
		if (add)
		{
			SETBIT(ch->affected_by, AFF_FLYING);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_FLYING);
		}
	}
	break;

	case 36:
		break;
	case 37:
		break;
	case 38:
		break;
	case 39:
		break;
	case 40:
		break;
	case 41:
		break;
	case 42:
		break;
	case 43:
		break;
	case 44:
		break;
	case 45:
		break;
	case 46:
		break;
	case 47:
		break;
	case 48:
		break;
	case 49:
		break;
	case 50:
		break;
	case 51:
		break;
	case 52:
		break;
	case 53:
		break;
	case 54:
		break;
	case 55:
		break;
	case 56:
		break;
	case 57:
		break;
	case 58:
		break;
	case 59:
		break;
	case 60:
		break;
	case 61:
		break;
	case 62:
		break;
	case 63:
		if (add)
		{
			SET_BIT(ch->resist, ISR_SLASH);
		}
		else
		{
			REMOVE_BIT(ch->resist, ISR_SLASH);
		}
		break;
	case 64:
		if (add)
		{
			SET_BIT(ch->resist, ISR_FIRE);
		}
		else
		{
			REMOVE_BIT(ch->resist, ISR_FIRE);
		}
		break;
	case 65:
		if (add)
		{
			SET_BIT(ch->resist, ISR_COLD);
		}
		else
		{
			REMOVE_BIT(ch->resist, ISR_COLD);
		}
		break;

	case 66:
		ch->max_ki += mod;
		break;

	case 67:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_CAMOUFLAGUE);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_CAMOUFLAGUE);
		}
		break;
	case 68:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_FARSIGHT);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_FARSIGHT);
		}
		break;
	case 69:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_FREEFLOAT);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_FREEFLOAT);
		}
		break;
	case 70:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_FROSTSHIELD);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_FROSTSHIELD);
		}
		break;
	case 71:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_INSOMNIA);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_INSOMNIA);
		}
		break;
	case 72:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_LIGHTNINGSHIELD);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_LIGHTNINGSHIELD);
		}
		break;
	case 73:
		ch->spell_reflect += mod;

		if (add)
		{
			SETBIT(ch->affected_by, AFF_REFLECT);
		}
		else
		{
			if (ch->spell_reflect <= 0)
			{
				REMBIT(ch->affected_by, AFF_REFLECT);
			}
		}
		break;
	case 74:
		if (add)
		{
			SET_BIT(ch->resist, ISR_ENERGY);
		}
		else
		{
			REMOVE_BIT(ch->resist, ISR_ENERGY);
		}
		break;
	case 75:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_SHADOWSLIP);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_SHADOWSLIP);
		}
		break;
	case 76:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_SOLIDITY);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_SOLIDITY);
		}
		break;
	case 77:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_STABILITY);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_STABILITY);
		}
		break;
	case 78:
		if (add)
		{
			SET_BIT(ch->immune, ISR_POISON);
		}
		else
		{
			REMOVE_BIT(ch->immune, ISR_POISON);
		}
		break;
	case 79:
		break;
	case 80:
		break;
	case 81:
		break;
	case 82:
		break;
	case 83:
		break;
	case 84:
		break;
	case 85:
		break;
	case 86:
		break;

	case APPLY_INSANE_CHANT:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_INSANE);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_INSANE);
		}
		break;

	case APPLY_GLITTER_DUST:
		if (add)
		{
			SETBIT(ch->affected_by, AFF_GLITTER_DUST);
		}
		else
		{
			REMBIT(ch->affected_by, AFF_GLITTER_DUST);
		}
		break;

	case APPLY_RESIST_ACID:
		if (add)
		{
			SET_BIT(ch->resist, ISR_ACID);
		}
		else
		{
			REMOVE_BIT(ch->resist, ISR_ACID);
		}
		break;

	case APPLY_HP_REGEN:
		ch->hit_regen += mod;
		break;

	case APPLY_MANA_REGEN:
		ch->mana_regen += mod;
		break;

	case APPLY_MOVE_REGEN:
		ch->move_regen += mod;
		break;

	case APPLY_KI_REGEN:
		ch->ki_regen += mod;
		break;

	case WEP_CREATE_FOOD:
		break;

	case APPLY_DAMAGED: // this is for storing damage to the item
		break;

	case WEP_THIEF_POISON:
		break;

	case APPLY_PROTECT_GOOD:
		if (add)
			SETBIT(ch->affected_by, AFF_PROTECT_GOOD);
		else
			REMBIT(ch->affected_by, AFF_PROTECT_GOOD);
		break;

	case APPLY_MELEE_DAMAGE:
		ch->melee_mitigation += mod;
		break;

	case APPLY_SPELL_DAMAGE:
		ch->spell_mitigation += mod;
		break;

	case APPLY_SONG_DAMAGE:
		ch->song_mitigation += mod;
		break;

	case APPLY_BOUNT_SONNET_HUNGER:
		if (add)
			SETBIT(ch->affected_by, AFF_BOUNT_SONNET_HUNGER);
		else
		{
			REMBIT(ch->affected_by, AFF_BOUNT_SONNET_HUNGER);
			GET_COND(ch, FULL) = 12;
		}
		break;

	case APPLY_BOUNT_SONNET_THIRST:
		if (add)
			SETBIT(ch->affected_by, AFF_BOUNT_SONNET_THIRST);
		else
		{
			REMBIT(ch->affected_by, AFF_BOUNT_SONNET_THIRST);
			GET_COND(ch, THIRST) = 12;
		}
		break;

	case APPLY_BLIND:
		if (add)
			SETBIT(ch->affected_by, AFF_BLIND);
		else
			REMBIT(ch->affected_by, AFF_BLIND);
		break;

	case APPLY_WATER_BREATHING:
		if (add)
			SETBIT(ch->affected_by, AFF_WATER_BREATHING);
		else
			REMBIT(ch->affected_by, AFF_WATER_BREATHING);
		break;

	case APPLY_DETECT_MAGIC:
		if (add)
			SETBIT(ch->affected_by, AFF_DETECT_MAGIC);
		else
			REMBIT(ch->affected_by, AFF_DETECT_MAGIC);
		break;

	case WEP_WILD_MAGIC:
		break;

	default:
		sprintf(log_buf, "Unknown apply adjust attempt: %d. (handler.c, "
						 "affect_modify.)",
				loc);
		logentry(log_buf, 0, LogChannels::LOG_BUG);
		break;

	} /* switch */
}

// This updates a character by subtracting everything he is affected by
// then restoring it back.  It is used primarily for bitvector affects
// that could be on both eq as well as on spells.
// Stat (con, str, etc) stuff is handled in affect_modify

void affect_total(Character *ch)
{
	struct affected_type *af;
	struct affected_type *tmp_af;
	int i, j;
	bool already = ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT);

	SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop falling off

	// remove all affects from the player
	for (i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
			for (j = 0; j < ch->equipment[i]->num_affects; j++)
				affect_modify(ch, ch->equipment[i]->affected[j].location, ch->equipment[i]->affected[j].modifier, -1, false);
	}
	remove_totem_stats(ch);
	for (af = ch->affected; af; af = tmp_af)
	{
		//        bool secFix = false;
		tmp_af = af->next;
		affect_modify(ch, af->location, af->modifier, af->bitvector, false);
	}

	// add everything back
	for (i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
			for (j = 0; j < ch->equipment[i]->num_affects; j++)
				affect_modify(ch, ch->equipment[i]->affected[j].location, ch->equipment[i]->affected[j].modifier, -1, true);
	}
	for (af = ch->affected; af; af = af->next)
	{
		//      bool secFix = false;
		affect_modify(ch, af->location, af->modifier, af->bitvector, true);
	}
	add_totem_stats(ch);
	if (!already)
		REMBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop fall off

	redo_hitpoints(ch);
	redo_mana(ch);
	redo_ki(ch);
}

/* Insert an affect_type in a Character structure
 Automatically sets apropriate bits and apply's */
void affect_to_char(Character *ch, struct affected_type *af, int32_t duration_type)
{
	//    bool secFix;
	struct affected_type *affected_alloc;
	if (af->location >= 1000)
		return; // Skill aff;
#ifdef LEAK_CHECK
	affected_alloc = new (nothrow) affected_type;
#else
	affected_alloc = new (nothrow) affected_type;
	// affected_alloc = (struct affected_type *) dc_alloc(1, sizeof(struct affected_type));
#endif

	*affected_alloc = *af;
	affected_alloc->duration_type = duration_type;
	affected_alloc->next = ch->affected;
	ch->affected = affected_alloc;

	affect_modify(ch, af->location, af->modifier, af->bitvector, true);
}

/* Remove an affected_type structure from a char (called when duration
 reaches zero). Pointer *af must never be NIL! Frees mem and calls
 affect_location_apply                                                */
void affect_remove(Character *ch, struct affected_type *af, int flags)
{
	struct affected_type *hjp;
	char buf[200];
	short dir;
	bool char_died = false;
	struct follow_type *f, *next_f;

	if (!ch->affected)
		return;

	assert(ch);
	assert(ch->affected);

	affect_modify(ch, af->location, af->modifier, af->bitvector, false);

	/* remove structure *af from linked list */

	if (ch->affected == af)
	{
		/* remove head of list */
		ch->affected = af->next;
	}
	else
	{

		for (hjp = ch->affected; (hjp->next) && (hjp->next != af); hjp = hjp->next)
			;

		if (hjp->next != af)
		{
			logentry("FATAL : Could not locate affected_type in ch->affected. (handler.c, affect_remove)", ARCHITECT, LogChannels::LOG_BUG);
			sprintf(buf, "Problem With: %s    Affect type: %d", ch->name, af->type);
			logentry(buf, ARCHITECT, LogChannels::LOG_BUG);
			return;
		}
		hjp->next = af->next; /* skip the af element */
	}

	if (af->next && af->next != (affected_type *)0x95959595 && (af->next->type == af->type))
		flags = SUPPRESS_MESSAGES;

	switch (af->type)
	{
		// Put anything here that MUST go away whenever the spell wears off
		// That isn't handled in affect_modify

	case SPELL_IRON_ROOTS:
		REMBIT(ch->affected_by, AFF_NO_FLEE);
		break;
	case SPELL_STONE_SKIN: /* Stone skin wears off... Remove resistance */
		REMOVE_BIT(ch->resist, ISR_PIERCE);
		break;
	case SPELL_BARKSKIN: /* barkskin wears off */
		REMOVE_BIT(ch->resist, ISR_SLASH);
		break;
	case SPELL_FLY:
		/* Fly wears off...you fall :) */
		if (((flags & SUPPRESS_CONSEQUENCES) == 0) && ((IS_SET(world[ch->in_room].room_flags, FALL_DOWN) && (dir = 5)) || (IS_SET(world[ch->in_room].room_flags, FALL_UP) && (dir = 4)) || (IS_SET(world[ch->in_room].room_flags, FALL_EAST) && (dir = 1)) || (IS_SET(world[ch->in_room].room_flags, FALL_WEST) && (dir = 3)) || (IS_SET(world[ch->in_room].room_flags, FALL_SOUTH) && (dir = 2)) || (IS_SET(world[ch->in_room].room_flags, FALL_NORTH) && (dir = 0))))
		{
			if (do_fall(ch, dir) & eCH_DIED)
				char_died = true;
		}
		break;
	case SKILL_INSANE_CHANT:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The insane chanting in your mind wears off.\r\n", ch);
		break;
	case SKILL_GLITTER_DUST:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The dust around your body stops glowing.\r\n", ch);
		break;
	case SKILL_INNATE_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			switch (ch->race)
			{ // Each race has its own wear off messages.
			case RACE_GIANT:
				send_to_char("You feel ready to wield mighty weapons again.\r\n", ch);
				break;
			case RACE_TROLL:
				send_to_char("Your regenerative abilitiy feels restored.\r\n", ch);
				break;
			case RACE_GNOME:
				send_to_char("Your ability to manipulate illusions has returned.\r\n", ch);
				break;
			case RACE_ORC:
				send_to_char("Your lust for blood feels restored.\r\n", ch);
				break;
			case RACE_DWARVEN:
				send_to_char("You feel ready to attempt more repairs.\r\n", ch);
				break;
			case RACE_ELVEN:
				send_to_char("Your mind regains its ability to sharpen concentration for brief periods.\r\n", ch);
				break;
			case RACE_PIXIE:
				send_to_char("Your ability to avoid magical vision has returned.\r\n", ch);
				break;
			case RACE_HOBBIT:
				send_to_char("Your innate ability to avoid magical portals has returned.\r\n", ch);
				break;
			}
		break;
	case SKILL_BLOOD_FURY:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your blood cools to normal levels.\r\n", ch);
		break;
	case SKILL_CRAZED_ASSAULT:
		if (!(flags & SUPPRESS_MESSAGES))
		{
			if (af->location == APPLY_HITROLL)
				send_to_char("Your craziness has subsided.\r\n", ch);
			else
				send_to_char("You feel ready to go crazy again.\r\n", ch);
		}
		break;
	case SKILL_INNATE_BLOODLUST:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your lust for battle has left you.\r\n", ch);
		break;
	case SKILL_INNATE_ILLUSION:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You slowly fade into existence.\r\n", ch);
		break;
	case SKILL_INNATE_EVASION:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your magical obscurity has left you.\r\n", ch);
		break;
	case SKILL_INNATE_SHADOWSLIP:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The ability to avoid magical pathways has left you.\r\n", ch);
		break;
	case SKILL_INNATE_FOCUS:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your concentration lessens to its regular amount.\r\n", ch);
		break;
	case SKILL_INNATE_REGENERATION:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your regeneration slows back to normal.\r\n", ch);
		break;
	case SKILL_INNATE_POWERWIELD:
		class Object *obj;
		obj = ch->equipment[WIELD];
		if (obj)
			if (obj->obj_flags.extra_flags & ITEM_TWO_HANDED)
			{
				if ((obj = ch->equipment[SECOND_WIELD]))
				{
					obj_to_char(unequip_char(ch, SECOND_WIELD, (flags & SUPPRESS_MESSAGES)), ch);
					if (!(flags & SUPPRESS_MESSAGES))
						act("You shift $p into your inventory.", ch, obj, nullptr, TO_CHAR, 0);
				}
				else if ((obj = ch->equipment[HOLD]))
				{
					obj_to_char(unequip_char(ch, HOLD, (flags & SUPPRESS_MESSAGES)), ch);
					if (!(flags & SUPPRESS_MESSAGES))
						act("You shift $p into your inventory.", ch, obj, nullptr, TO_CHAR, 0);
				}
				else if ((obj = ch->equipment[HOLD2]))
				{
					obj_to_char(unequip_char(ch, HOLD2, (flags & SUPPRESS_MESSAGES)), ch);
					if (!(flags & SUPPRESS_MESSAGES))
						act("You shift $p into your inventory.", ch, obj, nullptr, TO_CHAR, 0);
				}
				else if ((obj = ch->equipment[WEAR_SHIELD]))
				{
					obj_to_char(unequip_char(ch, WEAR_SHIELD, (flags & SUPPRESS_MESSAGES)), ch);
					if (!(flags & SUPPRESS_MESSAGES))
						act("You shift $p into your inventory.", ch, obj, nullptr, TO_CHAR, 0);
				}
				else if ((obj = ch->equipment[WEAR_LIGHT]))
				{
					obj_to_char(unequip_char(ch, WEAR_LIGHT, (flags & SUPPRESS_MESSAGES)), ch);
					if (!(flags & SUPPRESS_MESSAGES))
						act("You shift $p into your inventory.", ch, obj, nullptr, TO_CHAR, 0);
				}
			}
		obj = ch->equipment[SECOND_WIELD];
		if (obj)
			if (obj->obj_flags.extra_flags & ITEM_TWO_HANDED)
			{
				obj = ch->equipment[WIELD];
				if (obj)
				{
					obj_to_char(unequip_char(ch, SECOND_WIELD, (flags & SUPPRESS_MESSAGES)), ch);
					if (!(flags & SUPPRESS_MESSAGES))
						act("You shift $p into your inventory.", ch, obj, nullptr, TO_CHAR, 0);
				}
			}
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You can no longer wield two handed weapons.\r\n", ch);
		check_weapon_weights(ch);
		break;
	case SKILL_BLADESHIELD:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The draining affect of the blade shield technique has worn off.\r\n", ch);
		break;
	case SKILL_CLANAREA_CLAIM:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You can attempt to claim an area again.\r\n", ch);
		break;
	case SKILL_CLANAREA_CHALLENGE:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You can attempt to challenge an area again.\r\n", ch);
		break;
	case SKILL_PRIMAL_FURY:
		if (!af->bitvector)
		{
			if (!(flags & SUPPRESS_MESSAGES))
				send_to_char("TimerEndyMessage.\r\n", ch);
		}
		else
		{
			if (!(flags & SUPPRESS_MESSAGES))
				send_to_char("Your primal fury has abated.\r\n", ch);
		}
		break;
	case SKILL_FOCUSED_REPELANCE:
		REMOVE_BIT(ch->combat, COMBAT_REPELANCE);
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your mind recovers from the repelance.\r\n", ch);
		break;
	case SKILL_JAB:
		if (!(flags & SUPPRESS_MESSAGES) && af->bitvector != AFF_BLACKJACK)
			send_to_char("You feel ready to jab some poor sucker again.\r\n", ch);
		break;
	case SKILL_VITAL_STRIKE:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The internal strength and speed from your vital strike has returned.\r\n", ch);
		break;
	case SKILL_SONG_FLIGHT_OF_BEE:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your feet touch the ground once more.\r\n", ch);
		break;
	case SKILL_SONG_FANATICAL_FANFARE:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your mind no longer races.\r\n", ch);
		break;
	case SKILL_SONG_SUBMARINERS_ANTHEM:
		if (!(flags & SUPPRESS_CONSEQUENCES))
		{
			send_to_char("Your musical ability to breathe water ends.\r\n", ch);
			if (world[ch->in_room].sector_type == SECT_UNDERWATER) // uh oh
			{
				act("$n begins to choke on the water, a look of panic filling $s eyes as it fill $s lungs.\r\n", ch, 0, 0, TO_ROOM, 0);
				send_to_char("The water rushes into your lungs and the light fades with your oxygen.\r\n", ch);
			}
		}
		break;
	case SPELL_WATER_BREATHING:
		if (!(flags & SUPPRESS_CONSEQUENCES) && world[ch->in_room].sector_type == SECT_UNDERWATER) // uh oh
		{
#if 0
			// you just drowned!
			act("$n begins to choke on the water, a look of panic filling $s eyes as it fill $s lungs.\r\n"
					"$n is DEAD!!", ch, 0, 0, TO_ROOM, 0);
			send_to_char("The water rushes into your lungs and the light fades with your oxygen.\r\n"
					"You have been KILLED!!!\n\r", ch);
			fight_kill(nullptr, ch, TYPE_RAW_KILL, 0);
			char_died = true;
#else
			act("$n begins to choke on the water, a look of panic filling $s eyes as it fills $s lungs.\r\n", ch, 0, 0, TO_ROOM, 0);
			send_to_char("The water rushes into your lungs and the light fades with your oxygen.\r\n", ch);
#endif
		}
		break;
	case KI_STANCE + KI_OFFSET:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your body finishes venting the energy absorbed from your last ki stance.\r\n", ch);
		break;
	case SPELL_CHARM_PERSON: /* Charm Wears off */
		remove_memory(ch, 'h');
		if (ch->master)
		{
			if (!(flags & SUPPRESS_CONSEQUENCES) && GET_CLASS(ch->master) != CLASS_RANGER)
				add_memory(ch, GET_NAME(ch->master), 'h');
			stop_follower(ch, BROKE_CHARM);
		}
		break;
	case SKILL_TACTICS:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You forget your instruction in tactical fighting.\r\n", ch);
		break;
	case SKILL_DECEIT:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your deceitful practices have stopped helping you.\r\n", ch);
		break;
	case SKILL_FEROCITY:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The ferocity within your mind has dwindled.\r\n", ch);
		break;
	case KI_AGILITY + KI_OFFSET:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your body has lost its focused agility.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_MANA:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The magical energy of your robes has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_PROTECT_FROM_EVIL:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The defender's magical energy has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_GROUP_SANC:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The cassock's magical energy has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_TELEPORT:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The armbands' magical energy has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_KNOW_ALIGNMENT:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The gaze's magical energy has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_PARALYZE:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The paralytic's magical energy has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_GLOBE_OF_DARKNESS:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The choker's magical energy has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_CONT_LIGHT:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The necklace's magical energy has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_MISANRA_QUIVER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The quiver's magical energy has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_ALIGN_GOOD:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The fire's magical energy has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_ALIGN_EVIL:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The blackened heart's magical energy has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_EARTHQUAKE:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The magical energy of your hammer has recharged.\r\n", ch);
		break;
	case BASE_TIMERS + SPELL_WIZARD_EYE:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The scrying ball's magical energies have recharged.\r\n", ch);
		break;
	case SPELL_NAT_SELECT_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You feel capable of studying a new enemy of choice.\r\n", ch);
		break;
	case SKILL_DECEIT_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You feel like you could be deceitful again.\r\n", ch);
		break;
	case SKILL_FEROCITY_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You feel like you could be fierce again.\r\n", ch);
		break;
	case SKILL_TACTICS_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You feel like you could be tactical again.\r\n", ch);
		break;
	case SKILL_LAY_HANDS:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your god returns your ability to fill others with life.\r\n", ch);
		break;
	case SKILL_HARM_TOUCH:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your god returns your ability to cause pain to others with a touch.\r\n", ch);
		break;
	case SPELL_DIV_INT_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("The gods smile upon you and are ready to intervene on your behalf again.\r\n", ch);
		break;
	case SPELL_NO_CAST_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You feel able to concentrate on spellcasting once again.\r\n", ch);
		break;
	case SKILL_QUIVERING_PALM:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your body feels well enough to vibrate intensely once more.\r\n", ch);
		break;
	case SKILL_FORAGE:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You feel ready to look for \"herb\" again.\r\n", ch);
		break;
	case SKILL_TRIAGE_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You feel ready to mend your wounds once again.\r\n", ch);
		break;
	case SKILL_TRIAGE:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You finish cleaning and bandaging your wounds.\r\n", ch);
		break;
	case SKILL_LEADERSHIP:
		affect_from_char(ch, SKILL_LEADERSHIP_BONUS);
		if (ch->followers)
		{
			for (f = ch->followers; f; f = next_f)
			{
				next_f = f->next;
				affect_from_char(f->follower, SKILL_LEADERSHIP_BONUS);
			}
		}
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("Your inspirational leadership has ended.\r\n", ch);
		break;
	case SKILL_MAKE_CAMP_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You feel ready to set up another camp.\r\n", ch);
		break;
	case SKILL_SMITE_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You feel ready to once again smite your opponents.\r\n", ch);
		break;
	case SKILL_PERSEVERANCE:
		affect_from_char(ch, SKILL_PERSEVERANCE_BONUS);
		break;
	case SKILL_ONSLAUGHT_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You feel ready to begin another onslaught.\r\n", ch);
		break;
	case SKILL_ONSLAUGHT:
		if (!(flags & SUPPRESS_MESSAGES))
			send_to_char("You don't feel as fast and furious as you once did...\r\n", ch);
		break;
	case SKILL_BREW_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
		{
			send_to_char("You feel ready to brew something again.\r\n", ch);
		}
		break;
	case SKILL_SCRIBE_TIMER:
		if (!(flags & SUPPRESS_MESSAGES))
		{
			send_to_char("You feel ready to scribe something again.\r\n", ch);
		}
		break;
	case OBJ_LILITHRING:
		if (IS_NPC(ch))
		{
			remove_memory(ch, 'h');
			if (ch->master)
			{
				if (ch->master->in_room == ch->in_room)
				{
					act("$N blinks, shakes their head and snaps out of their dark trance. $N looks pissed!", ch->master, 0, ch, TO_ROOM, 0);
					act("$N blinks, shakes their head and snaps out of their dark trance. $N looks pissed!", ch->master, 0, ch, TO_CHAR, 0);
				}
				if (!(flags & SUPPRESS_CONSEQUENCES))
					add_memory(ch, GET_NAME(ch->master), 'h');
				stop_follower(ch, BROKE_CHARM_LILITH);
			}
		}
	default:
		break;
	}

	if (!char_died)
	{
		delete (af);
		affect_total(ch); // this must be here to take care of anything we might
						  // have just removed that is still given by equipment
	}
}

/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(Character *ch, int skill, int flags)
{
	struct affected_type *hjp, *afc, *recheck;
	//    bool aff2Fix;

	if (skill < 0) // affect types are unsigned, so no negatives are possible
		return;
	for (hjp = ch->affected; hjp; hjp = afc)
	{
		afc = hjp->next;
		if (hjp->type == (unsigned)skill)
			affect_remove(ch, hjp, flags);
		bool a = true;
		for (recheck = ch->affected; recheck; recheck = recheck->next)
			if (recheck == afc)
				a = false;
		if (a)
			break;
	}
}

/*
 * Return if a char is affected by a spell (SPELL_XXX), nullptr indicates
 * not affected.
 */
affected_type *affected_by_spell(Character *ch, int skill)
{
	struct affected_type *curr;

	if (skill < 0) // all affect types are unsigned
		return nullptr;

	for (curr = ch->affected; curr; curr = curr->next)
		if (curr->type == (unsigned)skill)
			return curr;

	return nullptr;
}

affected_type *affected_by_random(Character *ch)
{
	if (ch->affected == 0)
		return 0;

	// Count number of affects
	int aff_cnt = 0;
	for (affected_type *curr = ch->affected; curr; curr = curr->next)
		aff_cnt++;

	int j = 1;
	int pick = number(1, aff_cnt);
	for (affected_type *curr = ch->affected; curr; curr = curr->next)
	{
		if (j == pick)
			return curr;
		else
			j++;
	}

	return 0;
}

void affect_join(Character *ch, struct affected_type *af, bool avg_dur, bool avg_mod)
{
	struct affected_type *hjp;
	bool found = false;

	for (hjp = ch->affected; !found && hjp; hjp = hjp->next)
	{
		if (hjp->type == af->type)
		{

			af->duration += hjp->duration;
			if (avg_dur)
				af->duration /= 2;

			af->modifier += hjp->modifier;
			if (avg_mod)
				af->modifier /= 2;

			affect_remove(ch, hjp, SUPPRESS_ALL);
			affect_to_char(ch, af);
			found = true;
		}
	}
	if (!found)
		affect_to_char(ch, af);
}

int char_from_room(Character *ch)
{
	return char_from_room(ch, true);
}

/* move a player out of a room */
/* Returns 0 on failure, non-zero otherwise */
int char_from_room(Character *ch, bool stop_all_fighting)
{
	Character *i, *fighter, *next_char;
	bool Other = false, More = false, kimore = false;

	if (ch->in_room == DC::NOWHERE)
	{
		return (0);
	}

	if (ch->fighting && stop_all_fighting)
	{
		// Stop the person we're moving from fighting
		stop_fighting(ch);

		// Stop all the people fighting ch from fighting ch anymore
		for (fighter = combat_list; fighter; fighter = next_char)
		{
			next_char = fighter->next_fighting;

			if (fighter->fighting == ch)
				stop_fighting(fighter);
		}
	}

	world[ch->in_room].light -= ch->glow_factor;

	if (ch == world[ch->in_room].people) /* head of list */
		world[ch->in_room].people = ch->next_in_room;

	/* locate the previous element */
	else
		for (i = world[ch->in_room].people; i; i = i->next_in_room)
		{
			if (i->next_in_room == ch)
				i->next_in_room = ch->next_in_room;
		}
	//  if (IS_NPC(ch) && ISSET(ch->mobile->actflags, ACT_NOMAGIC))
	//	debugpoint();
	for (i = world[ch->in_room].people; i; i = i->next_in_room)
	{
		if (IS_NPC(i) && ISSET(i->mobile->actflags, ACT_NOMAGIC))
			Other = true;
		if (IS_NPC(i) && ISSET(i->mobile->actflags, ACT_NOTRACK))
			More = true;
		if (IS_NPC(i) && ISSET(i->mobile->actflags, ACT_NOKI))
			kimore = true;
	}
	if (IS_PC(ch)) // player
		DC::getInstance()->zones.value(world[ch->in_room].zone).decrementPlayers();
	if (IS_NPC(ch))
		ch->mobile->last_room = ch->in_room;
	if (IS_NPC(ch))
		if (ISSET(ch->mobile->actflags, ACT_NOTRACK) && !More && IS_SET(world[ch->in_room].iFlags, NO_TRACK))
		{
			REMOVE_BIT(world[ch->in_room].iFlags, NO_TRACK);
			REMOVE_BIT(world[ch->in_room].room_flags, NO_TRACK);
		}
	if (IS_NPC(ch))
		if (ISSET(ch->mobile->actflags, ACT_NOKI) && !kimore && IS_SET(world[ch->in_room].iFlags, NO_KI))
		{
			REMOVE_BIT(world[ch->in_room].iFlags, NO_KI);
			REMOVE_BIT(world[ch->in_room].room_flags, NO_KI);
		}
	if (IS_NPC(ch) && ISSET(ch->mobile->actflags, ACT_NOMAGIC) && !Other && IS_SET(world[ch->in_room].iFlags, NO_MAGIC))
	{
		REMOVE_BIT(world[ch->in_room].iFlags, NO_MAGIC);
		REMOVE_BIT(world[ch->in_room].room_flags, NO_MAGIC);
	}

	ch->in_room = DC::NOWHERE;
	ch->next_in_room = 0;

	/* success */
	return (1);
}

bool is_hiding(Character *ch, Character *vict)
{
	if (IS_NPC(ch))
		return (number(1, 101) > 70);

	if (!has_skill(ch, SKILL_HIDE))
		return false;
	if (ch->in_room != vict->in_room)
		return skill_success(ch, vict, SKILL_HIDE);

	int i;
	for (i = 0; i < MAX_HIDE; i++)
		if (ch->player->hiding_from[i] == vict && ch->player->hide[i])
			return true;
	return skill_success(ch, vict, SKILL_HIDE);
}

/* place a character in a room */
/* Returns zero on failure, and one on success */
int char_to_room(Character *ch, room_t room, bool stop_all_fighting)
{
	Character *temp;
	if (room == DC::NOWHERE)
		return (0);

	if (world[room].people == ch)
	{
		logentry("Error: world[room].people == ch in char_to_room().", ARCHITECT, LogChannels::LOG_BUG);
		return 0;
	}

	ch->next_in_room = world[room].people;
	world[room].people = ch;
	ch->in_room = room;

	world[room].light += ch->glow_factor;
	int a, i;
	if (IS_PC(ch) && ISSET(ch->affected_by, AFF_HIDE) && (a = has_skill(ch, SKILL_HIDE)))
	{
		for (i = 0; i < MAX_HIDE; i++)
			ch->player->hiding_from[i] = nullptr;
		i = 0;
		for (temp = ch->next_in_room; temp; temp = temp->next_in_room)
		{
			if (i >= MAX_HIDE)
				break;
			if (number(1, 101) > a) // Failed.
			{
				ch->player->hiding_from[i] = temp;
				ch->player->hide[i++] = false;
			}
			else
			{
				ch->player->hiding_from[i] = temp;
				ch->player->hide[i++] = true;
			}
		}
	}
	for (temp = ch->next_in_room; temp; temp = temp->next_in_room)
	{
		if (ISSET(temp->affected_by, AFF_HIDE) && IS_PC(temp))
			for (i = 0; i < MAX_HIDE; i++)
			{
				if (temp->player->hiding_from[i] == nullptr || temp->player->hiding_from[i] == ch)
				{
					if (number(1, 101) > has_skill(temp, SKILL_HIDE))
					{
						temp->player->hiding_from[i] = ch;
						temp->player->hide[i] = false;
					}
					else
					{
						temp->player->hiding_from[i] = ch;
						temp->player->hide[i] = true;
					}
				}
			}
	}
	if (IS_PC(ch)) // player
		DC::getInstance()->zones.value(world[room].zone).incrementPlayers();
	if (IS_NPC(ch))
	{
		if (ISSET(ch->mobile->actflags, ACT_NOMAGIC) && !IS_SET(world[room].room_flags, NO_MAGIC))
		{
			SET_BIT(world[room].iFlags, NO_MAGIC);
			SET_BIT(world[room].room_flags, NO_MAGIC);
		}
		if (ISSET(ch->mobile->actflags, ACT_NOKI) && !IS_SET(world[room].room_flags, NO_KI))
		{
			SET_BIT(world[room].iFlags, NO_KI);
			SET_BIT(world[room].room_flags, NO_KI);
		}
		if (ISSET(ch->mobile->actflags, ACT_NOTRACK) && !IS_SET(world[room].room_flags, NO_TRACK))
		{
			SET_BIT(world[room].iFlags, NO_TRACK);
			SET_BIT(world[room].room_flags, NO_TRACK);
		}
	}

	if (stop_all_fighting && (GET_CLASS(ch) == CLASS_BARD) && IS_SET(world[ch->in_room].room_flags, NO_KI) && !(ch->songs.empty()))
	{
		do_sing(ch, "stop", CMD_DEFAULT);
	}

	return (1);
}

/* Return the effect of a piece of armor in position eq_pos */
int apply_ac(Character *ch, int eq_pos)
{
	int value;
	assert(ch->equipment[eq_pos]);

	if (!(GET_ITEM_TYPE(ch->equipment[eq_pos]) == ITEM_ARMOR))
		return 0;

	if (IS_SET(ch->equipment[eq_pos]->obj_flags.extra_flags, ITEM_ENCHANTED))
	{
		value = (ch->equipment[eq_pos]->obj_flags.value[0]) - (ch->equipment[eq_pos]->obj_flags.value[1]);
	}
	else
	{
		value = (ch->equipment[eq_pos]->obj_flags.value[0]);
	}

	return value;
}

// return 0 on failure
// 1 on success
int equip_char(Character *ch, class Object *obj, int pos, int flag)
{
	int j;

	if (!ch || !obj)
	{
		logentry("Null ch or obj in equip_char()!", ARCHITECT, LogChannels::LOG_BUG);
		return 0;
	}
	if (pos < 0 || pos >= MAX_WEAR)
	{
		logentry("Invalid eq position in equip_char!", ARCHITECT, LogChannels::LOG_BUG);
		return 0;
	}
	if (ch->equipment[pos])
	{
		logf(ARCHITECT, LogChannels::LOG_BUG, "%s already equipped at position %d in equip_char!", GET_NAME(ch), pos);
		produce_coredump();
		return 0;
	}
	if (IS_AFFECTED(ch, AFF_CHARM) && (pos == WIELD || pos == SECOND_WIELD))
	{ // best indentation ever
		recheck_height_wears(ch);
		obj_to_char(obj, ch);
		return 0;
	}

	if (obj->carried_by)
	{
		logentry("EQUIP: Obj is carried_by when equip.", ARCHITECT, LogChannels::LOG_BUG);
		return 0;
	}

	if (obj->in_room != DC::NOWHERE)
	{
		logentry("EQUIP: Obj is in_room when equip.", ARCHITECT, LogChannels::LOG_BUG);
		return 0;
	}

	if (((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) || (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) || (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) && IS_PC(ch))
	{
		if (IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) || affected_by_spell(ch, FUCK_PTHIEF) || contains_no_trade_item(obj))
		{
			act("You are zapped by $p but it stays with you.", ch, obj, 0, TO_CHAR, 0);
			recheck_height_wears(ch);
			obj_to_char(obj, ch);
			if (pos == WIELD && ch->equipment[SECOND_WIELD])
			{
				equip_char(ch, unequip_char(ch, SECOND_WIELD), WIELD);
			}
			return 1;
		}
		if (ch->in_room != DC::NOWHERE)
		{
			act("You are zapped by $p and instantly drop it.", ch, obj, 0, TO_CHAR, 0);
			act("$n is zapped by $p and instantly drops it.", ch, obj, 0, TO_ROOM, 0);
			recheck_height_wears(ch);
			obj_to_room(obj, ch->in_room);
			if (pos == WIELD && ch->equipment[SECOND_WIELD])
			{
				equip_char(ch, unequip_char(ch, SECOND_WIELD), WIELD);
			}
			return 1;
		}
		else
		{
			logentry("ch->in_room = DC::NOWHERE when equipping char.", 0, LogChannels::LOG_BUG);
		}
	}

	if (obj_index[obj->getNumber()].virt == 30010 && !ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT))
	{
		act("$p binds to your skin and won't let go. It hurts!", ch, obj, 0, TO_CHAR, 0);
		act("$p binds to $n's skin!", ch, obj, 0, TO_ROOM, 0);
		obj->obj_flags.timer = 0;
	}
	if (obj_index[obj->getNumber()].virt == 30036 && !ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT))
	{
		act("As you grasp the staff, raw magical energy surges through you.  You can barely control it!", ch, obj, 0, TO_CHAR, 0);
		obj->obj_flags.timer = 0;
	}
	if (obj_index[obj->getNumber()].virt == 30033 && !ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT))
	{
		act("The Chaos Blade begins to pulse with a dull red light, your life force is being drained!", ch, obj, 0, TO_CHAR, 0);
		obj->obj_flags.timer = 0;
	}

	if (obj_index[obj->getNumber()].virt == 30008 && !ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT))
	{
		act("Upon grasping Lyvenia the Song Staff, you feel more lively!", ch, obj, 0, TO_CHAR, 0);
		obj->obj_flags.timer = 5;
	}

	ch->equipment[pos] = obj;
	obj->equipped_by = ch;
	if (IS_PC(ch))
		for (int a = 0; a < obj->num_affects; a++)
		{
			if (obj->affected[a].location >= 1000)
			{
				obj->next_skill = ch->player->skillchange;
				ch->player->skillchange = obj;
				break;
			}
		}

	if (IS_SET(obj->obj_flags.extra_flags, ITEM_GLOW))
	{
		ch->glow_factor++;
		if (ch->in_room > -1)
			world[ch->in_room].light++;
		//  this crashes in a reconnect cause player isn't around yet
		//  rather than fixing it, i'm leaving it out because it's annoying anyway cause
		//  it tells you every time you save
		// TODO - make it not be annoying
		//      act("The soft glow from $p brightens the area around $n.", ch, obj, 0, TO_ROOM, 0);
		//      act("The soft glow from $p brightens the area around you.", ch, obj, 0, TO_CHAR, 0);
	}
	if (obj->obj_flags.type_flag == ITEM_LIGHT && obj->obj_flags.value[2])
	{
		ch->glow_factor++;
		if (ch->in_room > -1)
			world[ch->in_room].light++;
	}

	if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
		GET_AC(ch) -= apply_ac(ch, pos);

	for (j = 0; ch->equipment[pos] && j < ch->equipment[pos]->num_affects; j++)
		affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier, -1, true, flag);

	add_set_stats(ch, obj, flag, pos);

	redo_hitpoints(ch);
	redo_mana(ch);
	redo_ki(ch);

	return 1;
}

class Object *unequip_char(Character *ch, int pos, int flag)
{
	int j;
	class Object *obj;

	assert(pos >= 0 && pos < MAX_WEAR);
	assert(ch->equipment[pos]);

	obj = ch->equipment[pos];

	if (obj_index[obj->getNumber()].virt == 30036 && !ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT))
	{
		act("With great effort, you are able to separate the Staff of Eternity from your own magical aura, but it comes at a great cost...", ch, obj, 0, TO_CHAR, 0);
		GET_MANA(ch) = GET_MANA(ch) / 2;
	}
	if (obj_index[obj->getNumber()].virt == 30033 && !ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT))
	{
		act("The effort required to separate the Chaos Blade from your own life force is immense! The Blade exacts a toll...", ch, obj, 0, TO_CHAR, 0);
		ch->setHP(ch->getHP() / 2);
	}
	if (obj_index[obj->getNumber()].virt == 30008 && !ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT))
	{
		act("The spring in your step has subsided.", ch, obj, 0, TO_CHAR, 0);
		obj->obj_flags.timer = 0;
	}

	if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
		GET_AC(ch) += apply_ac(ch, pos);

	remove_set_stats(ch, obj, flag);
	class Object *a, *b = nullptr;
b: // ew
	if (IS_PC(ch))
		for (a = ch->player->skillchange; a; a = a->next_skill)
		{
			if (a == (Object *)0x95959595)
			{
				int i;
				ch->player->skillchange = nullptr;
				for (i = 0; i < MAX_WEAR; i++)
				{
					int j;
					if (!ch->equipment[i])
						continue;
					for (j = 0; j < ch->equipment[i]->num_affects; j++)
					{
						if (ch->equipment[i]->affected[j].location > 1000)
						{
							ch->equipment[i]->next_skill = ch->player->skillchange;
							ch->player->skillchange = ch->equipment[i];
							break;
						}
					}
				}
				goto b;
			}
			if (a == obj)
			{
				if (b)
					b->next_skill = a->next_skill;
				else
					ch->player->skillchange = a->next_skill;
				break;
			}
			b = a;
		}

	ch->equipment[pos] = 0;
	obj->equipped_by = 0;

	if (IS_SET(obj->obj_flags.extra_flags, ITEM_GLOW))
	{
		ch->glow_factor--;
		if (ch->in_room > -1)
			world[ch->in_room].light--;
		// this is just annoying cause it tells you every time you save
		// TODO - make it not be annoying
		//      act("The soft glow around $n from $p fades.", ch, obj, 0, TO_ROOM, 0);
		//      act("The glow around you fades slightly.", ch, obj, 0, TO_CHAR, 0);
	}
	if (obj->obj_flags.type_flag == ITEM_LIGHT && obj->obj_flags.value[2])
	{
		ch->glow_factor--;
		if (ch->in_room > -1)
			world[ch->in_room].light--;
	}

	for (j = 0; j < obj->num_affects; j++)
		affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier, -1, false);
	redo_hitpoints(ch);
	redo_mana(ch);
	redo_ki(ch);

	return (obj);
}

int get_number(QString name)
{

	auto pos = name.indexOf(".");
	if (pos == -1)
	{
		return 1;
	}

	auto number_str = name.left(pos);

	try
	{
		auto str_str = name.left(pos + 1);
		name = str_str;
		return number_str.toInt();
	}
	catch (...)
	{
		return 1;
	}
}

int get_number(string &name)
{
	size_t pos = name.find(".");
	if (pos == name.npos)
	{
		return 1;
	}

	string number_str = name.substr(0, pos);

	try
	{
		string str_str = name.substr(pos + 1);
		name = str_str;
		return stoi(number_str);
	}
	catch (...)
	{
		return 1;
	}
}

int get_number(char **name)
{
	unsigned i;
	char *ppos = nullptr;
	char number[MAX_INPUT_LENGTH];
	char buffer[MAX_INPUT_LENGTH];

	if ((ppos = index(*name, '.')) != nullptr)
	{
		*ppos++ = '\0';
		// at this point, ppos points to the name only, and there is a
		// \0 between the number and the name as they appear in the string.
		strcpy(number, *name);
		// now number contains the number as a string
		strncpy(buffer, ppos, MAX_INPUT_LENGTH - 1);
		buffer[MAX_INPUT_LENGTH - 1] = 0;
		strcpy(*name, buffer);
		// now the pointer that was passed into the function
		// points to the name only.

		for (i = 0; *(number + i); i++)
			if (!isdigit(*(number + i)))
				return (-1);

		return (atoi(number));
	}

	return (1);
}

/* Search a given list for an object, and return a pointer to that object */
class Object *get_obj_in_list(char *name, class Object *list)
{
	class Object *i;
	int j, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	strcpy(tmpname, name);
	tmp = tmpname;
	if ((number = get_number(&tmp)) < 0)
		return (0);

	for (i = list, j = 1; i && (j <= number); i = i->next_content)
		if (isname(tmp, i->getName()))
		{
			if (j == number)
				return i;
			j++;
		}

	return (0);
}

/* Search a given list for an object number, and return a ptr to that obj */
class Object *get_obj_in_list_num(int num, class Object *list)
{
	class Object *i;

	for (i = list; i; i = i->next_content)
		if (i->getNumber() == num)
			return (i);

	return (0);
}

/*search the entire world for an object, and return a pointer  */
class Object *get_obj(char *name)
{
	class Object *i;
	int j, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	strcpy(tmpname, name);
	tmp = tmpname;
	if ((number = get_number(&tmp)) < 0)
		return (0);

	for (i = object_list, j = 1; i && (j <= number); i = i->next)
		if (isname(tmp, i->getName()))
		{
			if (j == number)
				return (i);
			j++;
		}

	return (0);
}

class Object *get_obj(int vnum)
{
	int num = real_object(vnum);

	return ((class Object *)obj_index[num].item);
}

/*search the entire world for an object number, and return a pointer  */
class Object *get_obj_num(int nr)
{
	class Object *i;

	for (i = object_list; i; i = i->next)
		if (i->getNumber() == nr)
			return (i);

	return (0);
}

/* search a room for a char, and return a pointer if found..  */
Character *get_char_room(char *name, int room, bool careful)
{
	Character *i;
	Character *partial_match;
	int j, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	partial_match = 0;

	strcpy(tmpname, name);
	tmp = tmpname;
	if ((number = get_number(&tmp)) < 0)
		return (0);

	for (i = world[room].people, j = 0; i && (j <= number); i = i->next_in_room)
	{
		if (number == 0 && IS_NPC(i))
			continue;
		if (number == 1 || number == 0)
		{
			if (isname(tmp, GET_NAME(i)) && !(careful && IS_NPC(i) && mob_index[i->mobile->getNumber()].virt == 12))
				return (i);
			else if (isname2(tmp, GET_NAME(i)))
			{
				if (partial_match)
				{
					if (strlen(GET_NAME(partial_match)) > strlen(GET_NAME(i)))
						partial_match = i;
				}
				else
					partial_match = i;
			}
		}
		else
		{
			if (isname(tmp, GET_NAME(i)))
			{
				j++;
				if (j == number)
					return (i);
			}
		}
	}

	return (partial_match);
}

/* search all over the world for a char, and return a pointer if found */
Character *get_char(string name)
{
	Character *partial_match = nullptr;
	int j = 0, number = 0;
	string tmpname = {}, tmp = {};

	tmpname = name;
	tmp = tmpname;
	if ((number = get_number(tmp)) < 0)
		return (0);

	auto &character_list = DC::getInstance()->character_list;
	auto result = find_if(character_list.begin(), character_list.end(), [&name, &partial_match, &number, &j, &tmp](Character *const &i)
						  {
		if (number == 0 && IS_NPC(i)) return false;
		if (number == 1 || number == 0)
		{
			if (isname(tmp, GET_NAME(i)))
			return true;
			else if (isname2(tmp.c_str(), GET_NAME(i)))
			{
				if (partial_match)
				{
					if (strlen(GET_NAME(partial_match)) > strlen(GET_NAME(i)))
					partial_match = i;
				}
				else
				partial_match = i;
			}
		}
		else
		{
			if(isname(tmp, GET_NAME(i)))
			{
				j++;
				if(j == number)
				return true;
			}
		}
		return false; });

	if (result != end(character_list))
	{
		return *result;
	}

	return partial_match;
}

/* search all over the world for a mob, and return a pointer if found */
Character *get_mob(char *name)
{
	auto &character_list = DC::getInstance()->character_list;
	auto result = find_if(character_list.begin(), character_list.end(), [&name](Character *const &i)
						  {
		if(IS_PC(i)) {
			return false;
		}
		if (isname(name, GET_NAME(i))) {
			return true;
		}
		return false; });

	if (result != end(character_list))
	{
		return *result;
	}

	return 0;
}

/* search all over the world for a char num, and return a pointer if found */
Character *get_char_num(int nr)
{
	auto &character_list = DC::getInstance()->character_list;
	auto result = find_if(character_list.begin(), character_list.end(), [&nr](Character *const &i)
						  {
		if (IS_MOB(i) && i->mobile->getNumber() == nr) {
			return true;
		}
		return false; });

	if (result != end(character_list))
	{
		return *result;
	}

	return 0;
}

// move an object from its current location into the room
// specified by dest.
int move_obj(Object *obj, int dest)
{
	int obj_in_room = DC::NOWHERE;
	Object *contained_by = 0;
	Character *carried_by = 0;

	if (!obj)
	{
		logentry("nullptr object sent to move_obj!", OVERSEER, LogChannels::LOG_BUG);
		return 0;
	}

	if (obj->equipped_by && GET_ITEM_TYPE(obj) != ITEM_BEACON)
	{
		fprintf(stderr, "FATAL: Object move_obj() while equipped: %s.\n", obj->getName().toStdString().c_str());
		abort();
	}

	if ((obj_in_room = obj->in_room) != DC::NOWHERE)
	{
		if (obj_from_room(obj) == 0)
		{
			// Couldn't move obj from the room
			logf(OVERSEER, LogChannels::LOG_BUG, "Couldn't move %s from room %d.", obj->getName().toStdString().c_str(), world[obj_in_room].number);
			return 0;
		}
	}

	if ((carried_by = obj->carried_by))
	{
		if (obj_from_char(obj) == 0)
		{
			// Couldn't move obj from the room
			logf(OVERSEER, LogChannels::LOG_BUG, "%s was carried by %s, and I couldn't "
												 "remove it!",
				 obj->getName().toStdString().c_str(), GET_NAME(obj->carried_by));
			return 0;
		}
	}

	if ((contained_by = obj->in_obj))
	{
		if (obj_from_obj(obj) == 0)
		{
			// Couldn't move obj from its container
			logf(OVERSEER, LogChannels::LOG_BUG, "%s was in container %s, and I couldn't "
												 "remove it!",
				 obj->getName().toStdString().c_str(), GET_NAME(obj->carried_by));
			return 0;
		}
	}

	if (obj_to_room(obj, dest) == 0)
	{
		// Couldn't move obj to dest...where to put it now?

		if ((obj_in_room != DC::NOWHERE) && (obj_to_room(obj, obj_in_room) == 0))
		{
			// Now we have real problems
			fprintf(stderr, "FATAL: Object stuck in DC::NOWHERE (1): %s.\n", obj->getName().toStdString().c_str());
			abort();
		}
		else if ((carried_by) && (obj_to_char(obj, carried_by) == 0))
		{
			// Now we have real problems
			fprintf(stderr, "FATAL: Object stuck in DC::NOWHERE (2) : %s.\n", obj->getName().toStdString().c_str());
			abort();
		}
		else if ((contained_by) && (obj_to_obj(obj, contained_by) == 0))
		{
			// Now we have real problems
			fprintf(stderr, "FATAL: Object stuck in DC::NOWHERE (3) : %s.\n", obj->getName().toStdString().c_str());
			abort();
		}

		logf(OVERSEER, LogChannels::LOG_BUG, "Could not move %s to destination: %d", obj->getName().toStdString().c_str(), world[dest].number);
		return 0;
	}

	// At this point, the object is happily in the new room
	return 1;
}

// move an object from its current location into the container
// specified by dest_obj.
int move_obj(Object *obj, Object *dest_obj)
{
	int obj_in_room = DC::NOWHERE;
	Object *contained_by = 0;
	Character *carried_by = 0;

	if (!obj)
	{
		logentry("nullptr object sent to move_obj!", OVERSEER, LogChannels::LOG_BUG);
		return 0;
	}

	if (obj->equipped_by && GET_ITEM_TYPE(obj) != ITEM_BEACON)
	{
		fprintf(stderr, "FATAL: Object move_obj() while equipped: %s.\n", obj->getName().toStdString().c_str());
		abort();
	}

	if ((obj_in_room = obj->in_room) != DC::NOWHERE)
	{
		if (obj_from_room(obj) == 0)
		{
			// Couldn't move obj from the room
			logf(OVERSEER, LogChannels::LOG_BUG, "Couldn't move %s from room %d.", obj->getName().toStdString().c_str(), world[obj_in_room].number);
			return 0;
		}
	}

	if ((carried_by = obj->carried_by))
	{
		if (obj_from_char(obj) == 0)
		{
			// Couldn't move obj from the room
			logf(OVERSEER, LogChannels::LOG_BUG, "%s was carried by %s, and I couldn't "
												 "remove it!",
				 obj->getName().toStdString().c_str(), GET_NAME(obj->carried_by));
			return 0;
		}
	}

	if ((contained_by = obj->in_obj))
	{
		if (obj_from_obj(obj) == 0)
		{
			// Couldn't move obj from its container
			logf(OVERSEER, LogChannels::LOG_BUG, "%s was in container %s, and I couldn't "
												 "remove it!",
				 obj->getName().toStdString().c_str(), GET_NAME(obj->carried_by));
			return 0;
		}
	}

	if (obj_to_obj(obj, dest_obj) == 0)
	{
		// Couldn't move obj to dest_obj...where to put it now?

		if ((obj_in_room != DC::NOWHERE) && (obj_to_room(obj, obj_in_room) == 0))
		{
			// Now we have real problems
			fprintf(stderr, "FATAL: Object stuck in DC::NOWHERE (4): %s.\n", obj->getName().toStdString().c_str());
			abort();
		}
		else if ((carried_by) && (obj_to_char(obj, carried_by) == 0))
		{
			// Now we have real problems
			fprintf(stderr, "FATAL: Object stuck in DC::NOWHERE (5) : %s.\n", obj->getName().toStdString().c_str());
			abort();
		}
		else if ((contained_by) && (obj_to_obj(obj, contained_by) == 0))
		{
			// Now we have real problems
			fprintf(stderr, "FATAL: Object stuck in DC::NOWHERE (6) : %s.\n", obj->getName().toStdString().c_str());
			abort();
		}

		logf(OVERSEER, LogChannels::LOG_BUG, "Could not move %s to container: %s", obj->getName().toStdString().c_str(), dest_obj->getName().toStdString().c_str());
		return 0;
	}
	add_totem(dest_obj, obj);
	// At this point, the object is happily in the new room
	return 1;
}

// move an object from its current location into the inventory of the
// character specified by ch.
int move_obj(Object *obj, Character *ch)
{
	int obj_in_room = DC::NOWHERE;
	Object *contained_by = 0;
	Character *carried_by = 0;

	//  char buffer[300];

	if (!obj)
	{
		logentry("nullptr object sent to move_obj!", OVERSEER, LogChannels::LOG_BUG);
		return 0;
	}

	if (obj->equipped_by && GET_ITEM_TYPE(obj) != ITEM_BEACON)
	{
		fprintf(stderr, "FATAL: Object move_obj() while equipped: %s.\n", obj->getName().toStdString().c_str());
		abort();
	}

	if ((obj_in_room = obj->in_room) != DC::NOWHERE)
	{
		if (obj_from_room(obj) == 0)
		{
			// Couldn't move obj from the room
			logf(OVERSEER, LogChannels::LOG_BUG, "Couldn't move %s from room %d.", obj->getName().toStdString().c_str(), world[obj_in_room].number);
			return 0;
		}
	}

	if ((carried_by = obj->carried_by))
	{
		if (obj_from_char(obj) == 0)
		{
			// Couldn't move obj from the room
			logf(OVERSEER, LogChannels::LOG_BUG, "%s was carried by %s, and I couldn't "
												 "remove it!",
				 obj->getName().toStdString().c_str(), GET_NAME(obj->carried_by));
			return 0;
		}
	}

	if ((contained_by = obj->in_obj))
	{
		if (obj->obj_flags.type_flag == ITEM_TOTEM && contained_by->obj_flags.type_flag == ITEM_ALTAR)
			remove_totem(contained_by, obj);

		if (obj_from_obj(obj) == 0)
		{
			// Couldn't move obj from its container
			logf(OVERSEER, LogChannels::LOG_BUG, "%s was in container %s, and I couldn't "
												 "remove it!",
				 obj->getName().toStdString().c_str(), GET_NAME(obj->carried_by));
			return 0;
		}
	}

	/* TODO - This doesn't work for some reason....need to find out why

	 // This should make sure we don't have any money items on players
	 if(obj->obj_flags.type_flag == ITEM_MONEY &&
	 obj->obj_flags.value[0] >= 1 ) {
	 sprintf(buffer,"There was %d coins.\r\n", obj->obj_flags.value[0]);
	 send_to_char(buffer,ch);
	 GET_GOLD(ch) += obj->obj_flags.value[0];
	 extract_obj(obj);
	 return 1;
	 }
	 */

	if (obj_to_char(obj, ch) == 0)
	{
		// Couldn't move obj to ch...where to put it now?

		if ((obj_in_room != DC::NOWHERE) && (obj_to_room(obj, obj_in_room) == 0))
		{
			// Now we have real problems
			fprintf(stderr, "FATAL: Object stuck in DC::NOWHERE (7): %s.\n", obj->getName().toStdString().c_str());
			abort();
		}
		else if ((carried_by) && (obj_to_char(obj, carried_by) == 0))
		{
			// Now we have real problems
			fprintf(stderr, "FATAL: Object stuck in DC::NOWHERE (8) : %s.\n", obj->getName().toStdString().c_str());
			abort();
		}
		else if ((contained_by) && (obj_to_obj(obj, contained_by) == 0))
		{
			// Now we have real problems
			fprintf(stderr, "FATAL: Object stuck in DC::NOWHERE (9) : %s.\n", obj->getName().toStdString().c_str());
			abort();
		}

		logf(OVERSEER, LogChannels::LOG_BUG, "Could not move %s to character: %s", obj->getName().toStdString().c_str(), GET_NAME(ch));
		return 0;
	}

	// At this point, the object is happily with the new owner
	return 1;
}

// give an object to a char
// 1 if success, 0 if failure
int obj_to_char(class Object *object, Character *ch)
{
	// class Object *obj;
	/*
	 if(!(obj = ch->carrying) ||
	 (!obj->next_content && obj->getNumber() > object->getNumber()))
	 {
	 */
	object->next_content = ch->carrying;
	ch->carrying = object;
	object->carried_by = ch;
	object->equipped_by = 0;
	object->in_room = DC::NOWHERE;
	IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
	IS_CARRYING_N(ch)
	++;
	extern void pick_up_item(Character * ch, class Object * obj);

	pick_up_item(ch, object);
	return 1;
	/*
	 }

	 while(obj->next_content &&
	 obj->next_content->getNumber() < object->getNumber())
	 obj = obj->next_content;

	 object->next_content = obj->next_content;
	 obj->next_content    = object;
	 object->carried_by   = ch;
	 object->equipped_by  = 0;
	 object->in_room      = DC::NOWHERE;

	 IS_CARRYING_W(ch)   += GET_OBJ_WEIGHT(object);
	 IS_CARRYING_N(ch)++;

	 return 1;
	 */
}

// take an object from a char
int obj_from_char(class Object *object)
{
	class Object *tmp;

	if (!object->carried_by)
	{
		logentry("Obj_from_char called on an object no one is carrying!", OVERSEER,
				 LogChannels::LOG_BUG);
		return 0;
	}

	// head of list
	if (object->carried_by->carrying == object)
		object->carried_by->carrying = object->next_content;
	else
	{
		// locate previous
		for (tmp = object->carried_by->carrying; tmp && (tmp->next_content != object); tmp = tmp->next_content)
			;

		if (tmp != nullptr)
		{
			tmp->next_content = object->next_content;
		}
	}

	IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
	IS_CARRYING_N(object->carried_by)
	--;

	if (IS_CARRYING_N(object->carried_by) < 0)
		IS_CARRYING_N(object->carried_by) = 0;

	object->carried_by = 0;
	object->next_content = 0;

	return 1;
}

// put an object in a room
int obj_to_room(class Object *object, int room)
{
	class Object *obj;

	if (!object)
		return 0;

	if (&world[room] == nullptr)
	{
		logf(IMMORTAL, LogChannels::LOG_BUG, "obj_to_room: world[%d] == nullptr", room);
		produce_coredump();
		return 0;
	}

	if (object->obj_flags.type_flag == ITEM_PORTAL)
		world[room].light++;

	// combine any cash amounts
	if (GET_ITEM_TYPE(object) == ITEM_MONEY)
	{
		for (obj = world[room].contents; obj; obj = obj->next_content)
			if (GET_ITEM_TYPE(obj) == ITEM_MONEY)
			{
				object->obj_flags.value[0] += obj->obj_flags.value[0];
				object->description = str_hsh("A pile of $B$5gold$R coins.");
				extract_obj(obj);
				break;
			}
	}

	// search through for the last object, or another object just like this one
	for (obj = world[room].contents; obj && obj->next_content; obj = obj->next_content)
	{
		if (obj->getNumber() == object->getNumber())
			break;
	}

	// put it in the list
	if (!obj)
	{
		object->next_content = nullptr;
		world[room].contents = object;
	}
	else
	{
		object->next_content = obj->next_content;
		obj->next_content = object;
	}
	object->in_room = room;
	object->carried_by = 0;
	if (GET_ITEM_TYPE(object) != ITEM_BEACON)
		object->equipped_by = 0;

	/*
	 if(!(obj = world[room].contents) ||
	 (!obj->next_content && obj->getNumber() > object->getNumber()))
	 {
	 object->next_content = world[room].contents;
	 world[room].contents = object;
	 object->in_room      = room;
	 object->carried_by   = 0;

	 if(GET_ITEM_TYPE(object) != ITEM_BEACON)
	 object->equipped_by  = 0;
	 return 1;
	 }

	 while(obj->next_content &&
	 obj->next_content->getNumber() < object->getNumber())
	 obj = obj->next_content;

	 object->next_content = obj->next_content;
	 obj->next_content    = object;
	 object->in_room      = room;
	 object->carried_by   = 0;

	 if(GET_ITEM_TYPE(object) != ITEM_BEACON)
	 object->equipped_by  = 0;
	 */
	return 1;
}

// Take an object from a room
int obj_from_room(class Object *object)
{
	class Object *i;

	if (object->in_room < 0)
	{
		logentry("obj_from_room called on an object that isn't in a room!", OVERSEER,
				 LogChannels::LOG_BUG);
		return 0;
	}

	if (object->obj_flags.type_flag == ITEM_PORTAL)
		world[object->in_room].light--;

	// head of list
	if (object == world[object->in_room].contents)
		world[object->in_room].contents = object->next_content;

	// locate previous element in list
	else
	{
		for (i = world[object->in_room].contents; i && (i->next_content != object); i = i->next_content)
			;

		if (i != nullptr)
		{
			i->next_content = object->next_content;
		}
	}

	object->in_room = DC::NOWHERE;
	object->next_content = 0;

	save_corpses();
	return 1;
}

// put an object in an object (quaint)
int obj_to_obj(class Object *obj, class Object *obj_to)
{
	class Object *tobj;

	obj->in_obj = obj_to;

	// search through for the last object, or another object just like this one
	for (tobj = obj_to->contains; tobj && tobj->next_content; tobj = tobj->next_content)
	{
		if (tobj->getNumber() == obj->getNumber())
			break;
	}

	// put it in the list
	if (!tobj)
	{
		obj->next_content = nullptr;
		obj_to->contains = obj;
	}
	else
	{
		obj->next_content = tobj->next_content;
		tobj->next_content = obj;
	}

	// recursively upwards add the weight.  Since we only have 1 layer of containers,
	// this loop only happens once, but it's good to leave later in case we change our mind
	if (obj_index[obj_to->getNumber()].virt != 536)
	{
		for (tobj = obj->in_obj; tobj;
			 GET_OBJ_WEIGHT(tobj) += GET_OBJ_WEIGHT(obj), tobj = tobj->in_obj)
			;
	}
	return 1;
}

// remove an object from an object
int obj_from_obj(class Object *obj)
{
	class Object *tmp, *obj_from;

	if (!obj->in_obj)
	{
		logentry("obj_from_obj called on an item that isn't inside another item.",
				 OVERSEER, LogChannels::LOG_BUG);
		return 0;
	}

	obj_from = obj->in_obj;

	// head of list
	if (obj == obj_from->contains)
		obj_from->contains = obj->next_content;

	else
	{
		// locate previous
		for (tmp = obj_from->contains; tmp && (tmp->next_content != obj); tmp = tmp->next_content)
			;

		if (!tmp)
		{
			perror("Fatal error in object structures.");
			abort();
		}

		tmp->next_content = obj->next_content;
	}

	// Subtract weight from containers container

	if (!obj_from || obj_index[obj_from->getNumber()].virt != 536)
	{
		for (tmp = obj->in_obj; tmp->in_obj; tmp = tmp->in_obj)
			GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

		GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

		// Subtract weight from char that carries the object
		if (tmp->carried_by)
			IS_CARRYING_W(tmp->carried_by) -= GET_OBJ_WEIGHT(obj);
	}

	obj->in_obj = 0;
	obj->next_content = 0;

	return 1;
}

// Set all carried_by to point to new_new owner
void object_list_new_new_owner(class Object *list, Character *ch)
{
	if (list)
	{
		object_list_new_new_owner(list->contains, ch);
		object_list_new_new_owner(list->next_content, ch);
		list->carried_by = ch;
	}
}

// Extract an object from the world
void extract_obj(class Object *obj)
{
	class Object *temp1;
	huntclear_item(obj);

	// if we're going away, unhook myself from my owner
	if (GET_ITEM_TYPE(obj) == ITEM_BEACON && obj->equipped_by)
	{
		obj->equipped_by->beacon = 0;
		obj->equipped_by = 0;
	}
	if (obj->table)
	{
		extern void destroy_table(struct table_data * tbl);
		destroy_table(obj->table);
	}
	if (obj->equipped_by)
	{
		int iEq;
		Character *vict = obj->equipped_by;
		for (iEq = 0; iEq < MAX_WEAR; iEq++)
			if (vict->equipment[iEq] == obj)
			{
				obj_to_char(unequip_char(vict, iEq, 1), vict);
				break;
			}
	}

	if (obj->in_room != DC::NOWHERE)
		obj_from_room(obj);
	else if (obj->carried_by)
		obj_from_char(obj);
	else if (obj->in_obj)
	{
		obj_from_obj(obj);
		/*
		 temp1 = obj->in_obj;
		 if(temp1->contains == obj)
		 temp1->contains = obj->next_content;
		 else
		 {
		 for( temp2 = temp1->contains ;
		 temp2 && (temp2->next_content != obj);
		 temp2 = temp2->next_content );

		 if(temp2) {
		 temp2->next_content =
		 obj->next_content; }
		 }
		 */
	}

	for (; obj->contains; extract_obj(obj->contains))
		;
	/* leaves nothing ! */

	if (object_list == obj) /* head of list */
		object_list = obj->next;
	else
	{
		for (temp1 = object_list; temp1 && (temp1->next != obj); temp1 = temp1->next)
			;

		if (temp1)
			temp1->next = obj->next;
	}

	if (obj->getNumber() >= 0)
	{
		(obj_index[obj->getNumber()].number)--;
	}

	for (auto &r : reroll_sessions)
	{
		if (r.second.choice1_obj == obj)
		{
			r.second.choice1_obj = nullptr;
		}
		if (r.second.choice2_obj == obj)
		{
			r.second.choice2_obj = nullptr;
		}
		if (r.second.orig_obj == obj)
		{
			r.second.orig_obj = nullptr;
		}
	}

	DC::getInstance()->obj_free_list.insert(obj);
}

void update_object(class Object *obj, int use)
{
	if (obj->obj_flags.timer > 0 && (obj_index[obj->getNumber()].virt != 30010 && obj_index[obj->getNumber()].virt != 30036 && obj_index[obj->getNumber()].virt != 30033 && obj_index[obj->getNumber()].virt != 30097 && obj_index[obj->getNumber()].virt != 30019))
		obj->obj_flags.timer -= use;
	if (obj->contains)
		update_object(obj->contains, use);
	if (obj->next_content)
		update_object(obj->next_content, use);
}

void update_char_objects(Character *ch)
{
	int i;

	if (ch->equipment[WEAR_LIGHT])
		if (ch->equipment[WEAR_LIGHT]->obj_flags.type_flag == ITEM_LIGHT)
			if (ch->equipment[WEAR_LIGHT]->obj_flags.value[2] > 0)
			{
				(ch->equipment[WEAR_LIGHT]->obj_flags.value[2])--;
				if (!ch->equipment[WEAR_LIGHT]->obj_flags.value[2])
				{
					send_to_char("Your light flickers out and dies.\r\n", ch);
					ch->glow_factor--;
					if (ch->in_room > -1)
						world[ch->in_room].light--;
				}
			}

	for (i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
		{
			if (obj_index[ch->equipment[i]->getNumber()].virt == SPIRIT_SHIELD_OBJ_NUMBER)
			{
				update_object(ch->equipment[i], 1);

				if (ch->equipment[i]->obj_flags.timer < 1)
				{
					send_to_room("The spirit shield shimmers brightly and then fades away.\r\n", ch->in_room);
					extract_obj(ch->equipment[i]);
				}
			}
			else
			{
				update_object(ch->equipment[i], 2);
			}
		}
	}

	if (ch->carrying)
		update_object(ch->carrying, 1);
}

/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char(Character *ch, bool pull, Trace t)
{
	Character *k, *next_char;
	class Connection *t_desc;
	int l, was_in;
	/*Character *i;*/
	bool isGolem = false;

	class Object *i;
	Character *omast = nullptr;
	int ret = eSUCCESS;
	if (IS_PC(ch) && !ch->desc)
		for (t_desc = DC::getInstance()->descriptor_list; t_desc; t_desc = t_desc->next)
			if (t_desc->original == ch)
				ret = do_return(t_desc->character, "", 0);
	if (SOMEONE_DIED(ret))
	{ // already taken care of
		return;
	}
	if (ch->in_room == DC::NOWHERE)
	{
		logentry("Extract_char: DC::NOWHERE", ARCHITECT, LogChannels::LOG_BUG);
		return;
	}

	if (IS_NPC(ch) && ch->mobile && ch->mobile->reset && ch->mobile->reset->lastPop)
		ch->mobile->reset->lastPop = nullptr;

	remove_totem_stats(ch);
	if (IS_PC(ch))
	{
		void shatter_message(Character * ch);
		void release_message(Character * ch);
		if (ch->player->golem)
		{
			if (ch->player->golem->in_room)
				release_message(ch->player->golem);
			extract_char(ch->player->golem, false);
		}
	}
	if (IS_NPC(ch) && mob_index[ch->mobile->getNumber()].virt == 8)
	{
		isGolem = true;
		if (pull)
		{
			if (ch->level > 1)
				ch->level--;
			ch->exp = 0; // Lower level, lose exp.
		}
		omast = ch->master;
	}

	if (ch->followers || ch->master)
		die_follower(ch);

	// If I was the leader of a group, free the group name from memory
	if (ch->group_name)
	{
		dc_free(ch->group_name);
		ch->group_name = nullptr;
		REMBIT(ch->affected_by, AFF_GROUP); // shrug
	}

	if (ch->fighting)
		stop_fighting(ch);

	for (k = combat_list; k; k = next_char)
	{
		next_char = k->next_fighting;
		if (k->fighting == ch)
			stop_fighting(k);
	}
	// remove any and all affects from the character
	while (ch->affected)
		affect_remove(ch, ch->affected, SUPPRESS_ALL);

	/* Must remove from room before removing the equipment! */
	was_in = ch->in_room;
	char_from_room(ch);
	if (!pull && !isGolem)
	{
		if (world[was_in].number == START_ROOM)
			char_to_room(ch, real_room(SECOND_START_ROOM));
		else if (DC::getInstance()->zones.value(world[GET_HOME(ch)].zone).continent == FAR_REACH || DC::getInstance()->zones.value(world[GET_HOME(ch)].zone).continent == UNDERDARK)
			char_to_room(ch, real_room(FARREACH_START_ROOM));
		else if (DC::getInstance()->zones.value(world[GET_HOME(ch)].zone).continent == DIAMOND_ISLE || DC::getInstance()->zones.value(world[GET_HOME(ch)].zone).continent == FORBIDDEN_ISLAND)
			char_to_room(ch, real_room(THALOS_START_ROOM));
		else
			char_to_room(ch, real_room(START_ROOM));
	}

	// make sure their ambush target is free'd
	if (ch->ambush)
	{
		dc_free(ch->ambush);
		ch->ambush = nullptr;
	}

	// I'm guarding someone.  Remove myself from their guarding list
	if (ch->guarding)
		stop_guarding(ch);

	// Someone is guarding me.  Let them know I don't exist anymore:)
	if (ch->guarded_by)
		stop_guarding_me(ch);

	// make sure no eq left on char.  But only if pulling completely
	if (pull)
	{
		for (l = 0; l < MAX_WEAR; l++)
		{
			if (ch->equipment[l])
			{
				obj_to_room(unequip_char(ch, l), was_in);
			}
		}
		if (ch->carrying)
		{
			class Object *inext;
			for (i = ch->carrying; i; i = inext)
			{
				inext = i->next_content;
				obj_from_char(i);
				obj_to_room(i, was_in);
			}
		}
	}
	if (isGolem && omast)
	{
		omast->save(666);
		omast->player->golem = 0; // Reset the golem flag.
	}
	/*
	 switch (GET_CLASS(ch)) {
	 case CLASS_MAGE: GET_AC(ch) = 200; break;
	 case CLASS_DRUID: GET_AC(ch) = 185; break;
	 case CLASS_CLERIC: GET_AC(ch) = 170; break;
	 case CLASS_ANTI_PAL: GET_AC(ch) = 155; break;
	 case CLASS_THIEF: GET_AC(ch) = 140; break;
	 case CLASS_BARD: GET_AC(ch) = 125; break;
	 case CLASS_BARBARIAN: GET_AC(ch) = 110; break;
	 case CLASS_RANGER: GET_AC(ch) = 95; break;
	 case CLASS_PALADIN: GET_AC(ch) = 80; break;
	 case CLASS_WARRIOR: GET_AC(ch) = 65; break;
	 case CLASS_MONK: GET_AC(ch) = 50; break;
	 default: GET_AC(ch) = 100;
	 }
	 *
	 * Changing this to be consistent with char_to_store function
	 */

	switch (GET_CLASS(ch))
	{
	case CLASS_MAGE:
		GET_AC(ch) = 150;
		break;
	case CLASS_DRUID:
		GET_AC(ch) = 140;
		break;
	case CLASS_CLERIC:
		GET_AC(ch) = 130;
		break;
	case CLASS_ANTI_PAL:
		GET_AC(ch) = 120;
		break;
	case CLASS_THIEF:
		GET_AC(ch) = 110;
		break;
	case CLASS_BARD:
		GET_AC(ch) = 100;
		break;
	case CLASS_BARBARIAN:
		GET_AC(ch) = 80;
		break;
	case CLASS_RANGER:
		GET_AC(ch) = 60;
		break;
	case CLASS_PALADIN:
		GET_AC(ch) = 40;
		break;
	case CLASS_WARRIOR:
		GET_AC(ch) = 20;
		break;
	case CLASS_MONK:
		GET_AC(ch) = 0;
		break;
	default:
		GET_AC(ch) = 100;
		break;
	}

	GET_AC(ch) -= GET_AC_METAS(ch);

	if (ch->desc && ch->desc->original)
		do_return(ch, "", 12);

	if (IS_NPC(ch) && ch->mobile->getNumber() > -1)
		mob_index[ch->mobile->getNumber()].number--;

	if (pull || isGolem)
	{
		remove_from_bard_list(ch);
		auto &death_list = DC::getInstance()->death_list;
		if (death_list.contains(ch))
		{
			stringstream ss;
			ss << "extract_char: " << t << endl;
			logf(IMMORTAL, LogChannels::LOG_BUG, "extract_char: death_list already contained Character %p from %s.", ch, ss.str().c_str());
			produce_coredump(ch);
		}
		else
		{
			death_list[ch] = t;
		}
	}
}

/* ***********************************************************************
 Here follows high-level versions of some earlier routines, ie functions
 which incorporate the actual player-data.
 *********************************************************************** */

/*
 * Get a random character from the player's current room that is visible to
 * him and not him.
 *
 * Remember that this function can return nullptr!
 */
Character *get_rand_other_char_room_vis(Character *ch)
{
	Character *vict = nullptr;
	int count = 0;

	// Count the number of players in room
	for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
		if (CAN_SEE(ch, vict) && ch != vict)
			count++;

	if (!count) // no players
		return nullptr;

	// Pick a random one
	count = number(1, count);

	// Find the "count" player and return them
	for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
	{
		if (CAN_SEE(ch, vict) && ch != vict)
		{
			if (count > 1)
			{
				count--;
			}
			else
			{
				return vict;
			}
		}
	}

	// we should never get here
	return nullptr;
}

void lastseen_targeted(Character *ch, Character *victim)
{
	static Character *last_ch;
	static Character *last_victim;

	// Check if we just ran this function already
	if (last_ch == ch && last_victim == victim)
	{
		return;
	}
	else
	{
		last_ch = ch;
		last_victim = victim;
	}

	if (ch == 0 || victim == 0 || IS_PC(victim) || IS_NPC(ch))
		return;

	if (ch->player->lastseen == 0)
		ch->player->lastseen = new multimap<int, pair<timeval, timeval>>;

	int nr = victim->mobile->getNumber();

	multimap<int, pair<timeval, timeval>>::iterator i;
	i = ch->player->lastseen->find(nr);

	timeval tv;
	for (unsigned int j = 0; j < ch->player->lastseen->count(nr); j++)
	{
		if ((*i).second.second.tv_sec == 0)
		{
			gettimeofday(&tv, nullptr);
			(*i).second.second = tv;
			return;
		}
		i++;
	}

	return;
}

Character *get_char_room_vis(Character *ch, string name)
{
	return get_char_room_vis(ch, name.c_str());
}

Character *get_char_room_vis(Character *ch, const char *name)
{
	Character *i;
	Character *partial_match;
	Character *rnd;
	int j, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	if (IS_AFFECTED(ch, AFF_BLACKJACK))
	{
		struct affected_type *af = affected_by_spell(ch, SKILL_JAB);
		if (af)
		{
			if (af->modifier == 1)
			{
				rnd = get_rand_other_char_room_vis(ch);
				if (rnd)
				{
					// Added get_rand.. check above 'cause ch->fighting would get set to nullptr.
					send_to_char("You're so dizzy you don't know who you're hitting.\r\n", ch);
					ch->fighting = rnd;
					return ch->fighting;
				}
			}
		}
	}

	partial_match = 0;

	strcpy(tmpname, name);
	tmp = tmpname;
	if ((number = get_number(&tmp)) < 0)
		return (0);

	for (i = world[ch->in_room].people, j = 0; i && (j <= number); i = i->next_in_room)
	{
		if (number == 0 && IS_NPC(i))
			continue;
		if (number == 1 || number == 0)
		{
			if (isname(tmp, GET_NAME(i)) && CAN_SEE(ch, i))
			{
				lastseen_targeted(ch, i);
				return (i);
			}
			else if (isname2(tmp, GET_NAME(i)) && CAN_SEE(ch, i))
			{
				if (partial_match)
				{
					if (strlen(GET_NAME(partial_match)) > strlen(GET_NAME(i)))
						partial_match = i;
				}
				else
					partial_match = i;
			}
		}
		else
		{
			if (isname(tmp, GET_NAME(i)) && CAN_SEE(ch, i))
			{
				j++;
				if (j == number)
				{
					lastseen_targeted(ch, i);
					return (i);
				}
			}
		}
	}
	lastseen_targeted(ch, partial_match);
	return (partial_match);
}

Character *get_mob_room_vis(Character *ch, char *name)
{
	Character *i;
	Character *partial_match;
	int j, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	partial_match = 0;

	strcpy(tmpname, name);
	tmp = tmpname;
	if ((number = get_number(&tmp)) < 0)
		return (0);

	for (i = world[ch->in_room].people, j = 1; i && (j <= number); i = i->next_in_room)
	{
		if (IS_PC(i))
			continue;

		if (number == 1)
		{
			if (isname(tmp, GET_NAME(i)) && CAN_SEE(ch, i))
				return (i);
			else if (isname2(tmp, GET_NAME(i)) && CAN_SEE(ch, i))
			{
				if (partial_match)
				{
					if (strlen(GET_NAME(partial_match)) > strlen(GET_NAME(i)))
						partial_match = i;
				}
				else
					partial_match = i;
			}
		}
		else
		{
			if (isname(tmp, GET_NAME(i)) && CAN_SEE(ch, i))
			{
				if (j == number)
					return (i);
				j++;
			}
		}
	}
	return (partial_match);
}

Character *get_pc_room_vis_exact(Character *ch, const char *name)
{
	Character *i;

	for (i = world[ch->in_room].people; i; i = i->next_in_room)
	{
		if (isname(name, GET_NAME(i)) && CAN_SEE(ch, i) && IS_PC(i))
			return (i);
	}
	return nullptr;
}

Character *get_mob_vis(Character *ch, char *name)
{
	Character *i;
	Character *partial_match;
	int j = 0, number = 0;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	partial_match = 0;

	/* check location */
	if ((i = get_mob_room_vis(ch, name)) != 0)
		return (i);

	strcpy(tmpname, name);
	tmp = tmpname;
	if ((number = get_number(&tmp)) < 0)
		return (0);

	auto &character_list = DC::getInstance()->character_list;
	auto result = find_if(character_list.begin(), character_list.end(), [&ch, &name, &partial_match, &number, &j, &tmp](Character *const &i)
						  {
		if (number == 1)
		{
			if (isname(tmp, GET_NAME(i))&& IS_NPC(i) && CAN_SEE(ch,i))
			return true;
			else if (isname2(tmp, GET_NAME(i))&& IS_NPC(i) && CAN_SEE(ch,i))
			{
				if (partial_match)
				{
					if (strlen(GET_NAME(partial_match)) > strlen(GET_NAME(i)))
					partial_match = i;
				}
				else
				partial_match = i;
			}
		}
		else
		{
			if(isname(tmp, GET_NAME(i)) && IS_NPC(i) && CAN_SEE(ch, i))
			{
				if(j == number)
				return true;
				j++;
			}
		}
		return false; });

	if (result != end(character_list))
	{
		return *result;
	}
	return partial_match;
}

Object *get_obj_vnum(int vnum)
{
	Object *i;
	int num = real_object(vnum);
	for (i = object_list; i; i = i->next)
		if (i->getNumber() == num)
			return i;
	return nullptr;
}

Character *get_random_mob_vnum(int vnum)
{
	int num = real_mobile(vnum);
	int total = mob_index[num].number;
	int which = number(1, total);

	auto &character_list = DC::getInstance()->character_list;

	auto result = find_if(character_list.begin(), character_list.end(), [&total, &which, &num](Character *const &i)
						  {
		if(IS_NPC(i) && i->mobile->getNumber() == num)
		{
			if (total == which)
			return true;
			else {
				total--;
			}
		}
		return false; });

	if (result != end(character_list))
	{
		return *result;
	}

	return nullptr;
}

Character *get_mob_vnum(int vnum)
{
	int number = real_mobile(vnum);
	auto &character_list = DC::getInstance()->character_list;

	auto result = find_if(character_list.begin(), character_list.end(), [&number](Character *const &i)
						  {
		if(IS_NPC(i) && i->mobile->getNumber() == number) {
			return true;
		}
		return false; });
	if (result != end(character_list))
	{
		return *result;
	}
	return nullptr;
}
Character *get_char_vis(Character *ch, QString name)
{
	return get_char_vis(ch, name.toStdString().c_str());
}

Character *get_char_vis(Character *ch, const string &name)
{
	return get_char_vis(ch, name.c_str());
}

Character *get_char_vis(Character *ch, const char *name)
{
	Character *i;
	Character *partial_match;

	int j = 0, number = 0;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	/* check location */
	if ((i = get_char_room_vis(ch, name)) != 0)
		return (i);

	partial_match = 0;

	strcpy(tmpname, name);
	tmp = tmpname;
	if ((number = get_number(&tmp)) < 0)
		return (0);

	auto &character_list = DC::getInstance()->character_list;
	auto result = find_if(character_list.begin(), character_list.end(), [&number, &tmp, &ch, &partial_match, &j](Character *const &i)
						  {
		if (i->in_room == DC::NOWHERE)
		{
			return false;
		}

		if (number == 0 && IS_NPC(i))
		{
			return false;
		}

		if (number == 1 || number == 0)
		{
			if (isname(tmp, GET_NAME(i)) && CAN_SEE(ch, i))
			return true;
			else if (isname2(tmp, GET_NAME(i)) && CAN_SEE(ch, i))
			{
				if (partial_match)
				{
					if (strlen(GET_NAME(partial_match)) > strlen(GET_NAME(i)))
					partial_match = i;
				}
				else
				partial_match = i;
			}
		}
		else
		{
			if (isname(tmp, GET_NAME(i)) && CAN_SEE(ch, i))
			{
				j++;
				if (j == number)
				return true;
			}
		}
		return false; });

	if (result != end(character_list))
	{
		return *result;
	}

	return partial_match;
}

Character *
get_pc(char *name)
{
	auto &character_list = DC::getInstance()->character_list;
	auto result = find_if(character_list.begin(), character_list.end(), [&name](Character *const &i)
						  {
		if(IS_PC(i) && isname(name, GET_NAME(i)))
		return true;

		return false; });

	if (result != end(character_list))
	{
		return *result;
	}

	return 0;
}

Character *
get_active_pc(const char *name)
{
	Character *i;
	Character *partial_match;
	class Connection *d;

	partial_match = 0;

	for (d = DC::getInstance()->descriptor_list; d; d = d->next)
	{
		if (!(i = d->character) || d->connected)
			continue;

		if (isname(name, GET_NAME(i)))
			return (i);
		else if (isname2(name, GET_NAME(i)))
		{
			if (partial_match)
			{
				if (strlen(GET_NAME(partial_match)) > strlen(GET_NAME(i)))
					partial_match = i;
			}
			else
				partial_match = i;
		}
	}

	return (partial_match);
}

Character *get_all_pc(char *name)
{
	Character *i;
	class Connection *d;

	for (d = DC::getInstance()->descriptor_list; d; d = d->next)
	{
		if (!(i = d->character))
		{
			continue;
		}

		if (isname(name, GET_NAME(i)))
		{
			return i;
		}
	}

	return 0;
}

Character *Character::getVisiblePlayer(QString name)
{
	return get_pc_vis(this, name.toStdString().c_str());
}

Character *Character::getVisibleCharacter(QString name)
{
	return get_char_vis(this, name.toStdString());
}

Object *Character::getVisibleObject(QString name)
{
	return get_obj_vis(this, name.toStdString());
}

Character *get_pc_vis(Character *ch, string name)
{
	return get_pc_vis(ch, name.c_str());
}

Character *get_pc_vis(Character *ch, const char *name)
{
	Character *partial_match = 0;

	auto &character_list = DC::getInstance()->character_list;
	auto result = find_if(character_list.begin(), character_list.end(), [&ch, &name, &partial_match](Character *const &i)
						  {
		if(IS_PC(i) && CAN_SEE(ch, i))
		{
			if (isname(name, GET_NAME(i)))
			return true;
			else if (isname2(name, GET_NAME(i)))
			{
				if (partial_match)
				{
					if (strlen(GET_NAME(partial_match)) > strlen(GET_NAME(i)))
					partial_match = i;
				}
				else
				partial_match = i;
			}
		}
		return false; });

	if (result != end(character_list))
	{
		return *result;
	}

	return partial_match;
}

Character *get_pc_vis_exact(Character *ch, const char *name)
{
	auto &character_list = DC::getInstance()->character_list;
	auto result = find_if(character_list.begin(), character_list.end(), [&ch, &name](Character *const &i)
						  {
		if(IS_PC(i) && CAN_SEE(ch, i))
		{
			if (!strcmp(name, GET_NAME(i)))
			return true;
		}
		return false; });

	if (result != end(character_list))
	{
		return *result;
	}

	return 0;
}

Character *get_active_pc_vis(Character *ch, const char *name)
{
	Character *i;
	Character *partial_match;
	class Connection *d;

	partial_match = 0;

	for (d = DC::getInstance()->descriptor_list; d; d = d->next)
	{
		if (!d->character || !GET_NAME(d->character))
			continue;
		i = d->character;
		if ((d->connected) || (!CAN_SEE(ch, i)))
			continue;

		if (CAN_SEE(ch, i))
		{
			if (isname(name, GET_NAME(i)))
				return (i);
			else if (isname2(name, GET_NAME(i)))
			{
				if (partial_match)
				{
					if (strlen(GET_NAME(partial_match)) > strlen(GET_NAME(i)))
						partial_match = i;
				}
				else
					partial_match = i;
			}
		}
	}
	return (partial_match);
}

// search by item number
class Object *get_obj_in_list_vis(Character *ch, int item_num, class Object *list, bool blindfighting)
{
	class Object *i;
	int number = real_object(item_num);

	// never match invalid items
	if (number == -1)
		return nullptr;

	for (i = list; i; i = i->next_content)
		if (i->getNumber() == number && CAN_SEE_OBJ(ch, i, blindfighting))
			return i;

	return nullptr;
}

class Object *get_obj_in_list_vis(Character *ch, const char *name, class Object *list, bool blindfighting)
{
	class Object *i;
	int j, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	strcpy(tmpname, name);
	tmp = tmpname;
	if ((number = get_number(&tmp)) < 0)
		return (0);

	for (i = list, j = 1; i && (j <= number); i = i->next_content)
		if (isname(tmp, i->getName()))
			if (isname(tmp, i->getName()))
				if (CAN_SEE_OBJ(ch, i, blindfighting))
				{
					if (j == number)
						return (i);
					j++;
				}
	return (0);
}

class Object *get_obj_in_list_vis(Character *ch, QString name, class Object *list, bool blindfighting)
{
	bool ok = false;
	qulonglong number = name.toULongLong(&ok);
	if (ok == false)
	{
		return nullptr;
	}

	qulonglong j = {};
	for (class Object *i = list; i != nullptr && (j <= number); i = i->next_content)
	{
		if (isname(name, i->getName()))
		{
			if (CAN_SEE_OBJ(ch, i, blindfighting))
			{
				if (j == number)
					return i;
				j++;
			}
		}
	}

	return nullptr;
}

/*search the entire world for an object, and return a pointer  */
class Object *get_obj_vis(Character *ch, QString name, bool loc)
{
	class Object *i;
	int j, number;
	QString tmpname;
	QString tmp;

	/* scan items carried */
	if ((i = get_obj_in_list_vis(ch, name, ch->carrying)) != nullptr)
		return (i);

	/* scan room */
	if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)) != nullptr)
		return (i);

	tmpname = name;
	tmp = tmpname;
	if ((number = get_number(tmp)) < 0)
		return (0);

	/* ok.. no luck yet. scan the entire obj list   */
	for (i = object_list, j = 1; i && (j <= number); i = i->next)
	{
		// TODO
		// For now they want me to remove this becuase portals and corpses are getNumber() -1
		// if (i->getNumber() == -1) continue;
		//
		if (loc && IS_SET(i->obj_flags.more_flags, ITEM_NOLOCATE) &&
			GET_LEVEL(ch) < 101)
			continue;
		if (isname(tmp, i->getName()))
			if (CAN_SEE_OBJ(ch, i))
			{
				if (j == number)
					return (i);
				j++;
			}
	}
	return (0);
}

class Object *get_obj_vis(Character *ch, string name, bool loc)
{
	/*search the entire world for an object, and return a pointer  */
	return get_obj_vis(ch, QString(name.c_str()), loc);
}
class Object *create_money(int amount)
{
	class Object *obj;
	struct extra_descr_data *new_new_descr;

	if (amount <= 0)
	{
		logentry("ERROR: Try to create negative money.", ARCHITECT, LogChannels::LOG_BUG);
		return (0);
	}

	obj = new Object;
	new_new_descr = new extra_descr_data;

	obj->clear();

	if (amount == 1)
	{
		obj->setName("coin gold");
		obj->setShortDescription("a gold coin");
		obj->description = str_hsh("One miserable gold coin.");

		new_new_descr->keyword = str_hsh("coin gold");
		new_new_descr->description = str_hsh("One miserable gold coin.");
	}
	else
	{
		obj->setName("coins gold");
		obj->setShortDescription("gold coins");
		obj->description = str_hsh("A pile of gold coins.");

		new_new_descr->keyword = str_hsh("coins gold");
		new_new_descr->description = str_hsh("They look like coins...of gold...duh.");
	}

	new_new_descr->next = 0;
	obj->ex_description = new_new_descr;

	obj->obj_flags.type_flag = ITEM_MONEY;
	obj->obj_flags.wear_flags = ITEM_TAKE;
	obj->obj_flags.value[0] = amount;
	obj->obj_flags.cost = amount;
	obj->setNumber(0);

	obj->next = object_list;
	object_list = obj;

	return (obj);
}

/* Generic Find, designed to find any object/character                    */
/* Calling :                                                              */
/*  *arg     is the sting containing the string to be searched for.       */
/*           This string doesn't have to be a single word, the routine    */
/*           extracts the next word itself.                               */
/*  bitv..   All those bits that you want to "search through".            */
/*           Bit found will be result of the function                     */
/*  *ch      This is the person that is trying to "find"                  */
/*  **tar_ch Will be nullptr if no character was found, otherwise points     */
/* **tar_obj Will be nullptr if no object was found, otherwise points        */
/*                                                                        */
/* The routine returns a pointer to the next word in *arg (just like the  */
/* one_argument routine).                                                 */

int generic_find(const char *arg, int bitvector, Character *ch, Character **tar_ch, class Object **tar_obj, bool verbose)
{
	static const char *ignore[] = {"the", "in", "\n"};

	int i;
	char name[256] = {'\0'};
	bool found;

	found = false;

	/* Eliminate spaces and "ignore" words */
	while (*arg && !found)
	{

		for (; *arg == ' '; arg++)
			;

		for (i = 0; (name[i] = *(arg + i)) && (name[i] != ' '); i++)
			;
		name[i] = 0;
		arg += i;
		if (search_block(name, ignore, true) > -1 || !IS_SET(bitvector, FIND_CHAR_ROOM))
			found = true;
	}

	if (!name[0])
		return (0);

	*tar_ch = 0;
	*tar_obj = 0;

	if (IS_SET(bitvector, FIND_CHAR_ROOM))
	{ /* Find person in room */
		*tar_ch = get_char_room_vis(ch, name);
		if (*tar_ch)
		{
			if (verbose)
			{
				if ((*tar_ch)->short_desc)
				{
					csendf(ch, "You find %s in this room.\r\n", (*tar_ch)->short_desc);
				}
				else if ((*tar_ch)->name)
				{
					csendf(ch, "You find %s in this room.\r\n", (*tar_ch)->name);
				}
				else
				{
					csendf(ch, "You find them in this room.\r\n");
				}
			}
			return (FIND_CHAR_ROOM);
		}
	}

	if (IS_SET(bitvector, FIND_CHAR_WORLD))
	{
		*tar_ch = get_char_vis(ch, name);
		if (*tar_ch)
		{
			if (verbose)
			{
				if ((*tar_ch)->short_desc)
				{
					csendf(ch, "You find %s somewhere in the world.\r\n", (*tar_ch)->short_desc);
				}
				else if ((*tar_ch)->name)
				{
					csendf(ch, "You find %s somewhere in the world.\r\n", (*tar_ch)->name);
				}
				else
				{
					csendf(ch, "You find them somewhere in the world.\r\n");
				}
			}
			return (FIND_CHAR_WORLD);
		}
	}

	if (IS_SET(bitvector, FIND_OBJ_INV))
	{
		*tar_obj = get_obj_in_list_vis(ch, name, ch->carrying);
		if (*tar_obj)
		{
			if (verbose)
			{
				if ((*tar_obj)->getShortDescriptionC())
				{
					csendf(ch, "You find %s in your inventory.\r\n", (*tar_obj)->getShortDescriptionC());
				}
				else if ((*tar_obj)->getName().isEmpty() == false)
				{
					csendf(ch, "You find %s in your inventory.\r\n", (*tar_obj)->getName().toStdString().c_str());
				}
				else
				{
					csendf(ch, "You find it in your inventory.\r\n");
				}
			}
			return (FIND_OBJ_INV);
		}
	}

	if (IS_SET(bitvector, FIND_OBJ_EQUIP))
	{
		for (found = false, i = 0; i < MAX_WEAR && !found; i++)
		{
			if (ch->equipment[i] && isname(name, ch->equipment[i]->getName()) && CAN_SEE_OBJ(ch, ch->equipment[i]))
			{
				*tar_obj = ch->equipment[i];
				found = true;
			}
		}
		if (found)
		{
			if (verbose)
			{
				if ((*tar_obj)->getShortDescriptionC())
				{
					csendf(ch, "You find %s among your equipment.\r\n", (*tar_obj)->getShortDescriptionC());
				}
				else if ((*tar_obj)->getName().isEmpty() == false)
				{
					csendf(ch, "You find %s among your equipment.\r\n", (*tar_obj)->getName().toStdString().c_str());
				}
				else
				{
					csendf(ch, "You find it among your equipment.\r\n");
				}
			}
			return (FIND_OBJ_EQUIP);
		}
	}

	if (IS_SET(bitvector, FIND_OBJ_ROOM))
	{
		*tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents);
		if (*tar_obj)
		{
			if (verbose)
			{
				if ((*tar_obj)->getShortDescriptionC())
				{
					csendf(ch, "You find %s in this room.\r\n", (*tar_obj)->getShortDescriptionC());
				}
				else if ((*tar_obj)->getName().isEmpty() == false)
				{
					csendf(ch, "You find %s in this room.\r\n", (*tar_obj)->getName().toStdString().c_str());
				}
				else
				{
					csendf(ch, "You find it in this room.\r\n");
				}
			}
			return (FIND_OBJ_ROOM);
		}
	}

	if (IS_SET(bitvector, FIND_OBJ_WORLD))
	{
		*tar_obj = get_obj_vis(ch, QString(name));
		if (*tar_obj)
		{
			if (verbose)
			{
				if ((*tar_obj)->getShortDescriptionC())
				{
					csendf(ch, "You find %s somewhere in the world.\r\n", (*tar_obj)->getShortDescriptionC());
				}
				else if ((*tar_obj)->getName().isEmpty() == false)
				{
					csendf(ch, "You find %s somewhere in the world\r\n", (*tar_obj)->getName().toStdString().c_str());
				}
				else
				{
					csendf(ch, "You find it somewhere in the world\r\n");
				}
			}
			return (FIND_OBJ_WORLD);
		}
	}

	return (0);
}

// Return a "somewhat" random person from the mobs hate list
// For now, just return the first one.  You can use "swap_hate_memory"
// to get a new one from 'get_random_hate'
char *get_random_hate(Character *ch)
{
	char buf[128];
	char *name = nullptr;

	if (IS_PC(ch))
		return nullptr;

	if (!ch->mobile->hatred)
		return nullptr;

	if (!strstr(ch->mobile->hatred, " "))
		return (ch->mobile->hatred);

	one_argument(ch->mobile->hatred, buf);

	name = str_hsh(buf);

	return name; // This is okay, since it's a str_hsh
}

// Take the first name on the list, and swap it to the end.
void swap_hate_memory(Character *ch)
{
	char *curr;
	char temp[32];
	char buf[MAX_STRING_LENGTH];

	if (IS_PC(ch))
		return;

	if (!(curr = strstr(ch->mobile->hatred, " ")))
		return;

	*curr = '\0';
	strcpy(temp, ch->mobile->hatred);
	sprintf(buf, "%s %s", curr + 1, ch->mobile->hatred);
	strcpy(ch->mobile->hatred, buf);
}

int hates_someone(Character *ch)
{
	if (IS_PC(ch))
		return 0;

	return (ch->mobile->hatred != nullptr);
}

int fears_someone(Character *ch)
{
	if (IS_PC(ch))
		return 0;

	return (ch->mobile->fears != nullptr);
}

void remove_memory(Character *ch, char type, Character *vict)
{
	char *temp = nullptr;
	char *curr = nullptr;

	if (type == 't')
		ch->hunting = 0;

	if (IS_PC(ch))
		return;

	if (type == 'h')
	{
		if (!ch->mobile->hatred)
			return;
		if (!isname(GET_NAME(vict), ch->mobile->hatred))
			return;
		if (strstr(ch->mobile->hatred, " "))
		{
#ifdef LEAK_CHECK
			temp = (char *)calloc((strlen(ch->mobile->hatred) - strlen(GET_NAME(vict))), sizeof(char));
#else
			temp = (char *)dc_alloc((strlen(ch->mobile->hatred) - strlen(GET_NAME(vict))), sizeof(char));
#endif
			curr = strstr(ch->mobile->hatred, GET_NAME(vict));
			if (curr == ch->mobile->hatred)
			{
				// This has to work, cause we checked it on our first if statement
				curr = strstr(curr, " ");
				strcpy(temp, curr + 1);
				dc_free(ch->mobile->hatred);
				ch->mobile->hatred = temp;
				return;
			}
			// This works because we made sure curr is not the first item in array
			// We're turning the 'space' into an end
			*(curr - 1) = '\0';
			strcpy(temp, ch->mobile->hatred);
			// If there is anything else after the one we removed, slap it (and space) in
			if ((curr = strstr(curr, " ")))
				strcat(temp, curr);
			dc_free(ch->mobile->hatred);
			ch->mobile->hatred = temp;
		}
		else
		{
			dc_free(ch->mobile->hatred);
			ch->mobile->hatred = nullptr;
		}
	}

	//  if(type == 'h')
	//    ch->mobile->hatred = 0;

	// these two are str_hsh'd, so just null them out
	if (type == 'f')
		ch->mobile->fears = 0;
}

void room_mobs_only_hate(Character *ch)
{
	auto &character_list = DC::getInstance()->character_list;

	for_each(character_list.begin(), character_list.end(), [&ch](Character *vict)
			 {
		if ((!ARE_GROUPED(ch, vict)) && (ch->in_room == vict->in_room) &&
				(vict != ch) && !IS_SET(world[ch->in_room].room_flags, SAFE)) {
			remove_memory(vict, 'h');
			add_memory(vict, GET_NAME(ch), 'h');
		} });
}

void remove_memory(Character *ch, char type)
{
	if (type == 't')
		ch->hunting = 0;

	if (IS_PC(ch))
		return;

	if (type == 'h' && ch->mobile->hatred)
	{
		dc_free(ch->mobile->hatred);
		ch->mobile->hatred = nullptr;
	}

	if (type == 'f')
		ch->mobile->fears = 0;
}

void add_memory(Character *ch, char *victim, char type)
{
	char *buf = nullptr;

	if (IS_PC(ch))
		return;

	// pets don't know to hate people
	if (IS_AFFECTED(ch, AFF_CHARM) || IS_AFFECTED(ch, AFF_FAMILIAR))
		return;

	if (type == 'h')
	{
		if (!ch->mobile->hatred)
			ch->mobile->hatred = str_dup(victim);
		else
		{
			// Don't put same name twice
			if (isname(victim, ch->mobile->hatred))
				return;

				// name 1 + name 2 + a space + terminator
#ifdef LEAK_CHECK
			buf = (char *)calloc((strlen(ch->mobile->hatred) + strlen(victim) + 2), sizeof(char));
#else
			buf = (char *)dc_alloc((strlen(ch->mobile->hatred) + strlen(victim) + 2), sizeof(char));
#endif
			sprintf(buf, "%s %s", ch->mobile->hatred, victim);
			dc_free(ch->mobile->hatred);
			ch->mobile->hatred = buf;
		}
	}
	else if (type == 'f')
		ch->mobile->fears = str_hsh(victim);
	else if (type == 't')
		ch->hunting = str_dup(victim);
}

// function for charging moves, to make it easier to have skills that impact ALL(such as vigor)
bool charge_moves(Character *ch, int skill, double modifier)
{
	int i = 0;
	int amt = skill_cost.find(skill)->second * modifier;
	int reduce = 0;

	if ((i = has_skill(ch, SKILL_VIGOR)) && skill_success(ch, nullptr, SKILL_VIGOR))
	{
		reduce = number(i / 8, i / 4); // 12-25 @ max skill
		amt = (amt * reduce) / 100;
	}

	if (GET_MOVE(ch) < amt)
	{
		send_to_char("You do not have enough movement to do this!\r\n", ch);
		return false;
	}
	GET_MOVE(ch) -= amt;
	return true;
}

MatchType add_matching_results(skill_results_t &results, const string &name, const string &key, uint64_t value)
{
	auto match = str_n_nosp_cmp_begin(name, key);
	// If this is an exact match we want only a single result
	if (match == MatchType::Exact)
	{
		results = {};
	}

	if (match != MatchType::Failure)
	{
		results[key] = value;
	}

	return match;
}

skill_results_t find_skills_by_name(string name)
{
	skill_results_t results = {};

	if (name.empty())
	{
		return results;
	}

	for (auto i = 0; *ki[i] != '\n'; i++)
	{
		if (add_matching_results(results, name, ki[i], i + KI_OFFSET) == MatchType::Exact)
		{
			return results;
		}
	}

	// try spells
	for (auto i = 0; *spells[i] != '\n'; i++)
	{
		if (add_matching_results(results, name, spells[i], i + 1) == MatchType::Exact)
		{
			return results;
		}
	}

	// try skills
	for (auto i = 0; *skills[i] != '\n'; i++)
	{
		if (add_matching_results(results, name, skills[i], i + SKILL_BASE) == MatchType::Exact)
		{
			return results;
		}
	}

	// try songs
	for (auto i = 0; *songs[i] != '\n'; i++)
	{
		if (add_matching_results(results, name, songs[i], i + SKILL_SONG_BASE) == MatchType::Exact)
		{
			return results;
		}
	}

	// sets?
	for (auto i = 0; *set_list[i].SetName != '\n'; i++)
	{
		if (add_matching_results(results, name, set_list[i].SetName, i + BASE_SETS) == MatchType::Exact)
		{
			return results;
		}
	}

	// timers/other stuff
	switch (LOWER(name[0]))
	{
	case 'b':
		if (add_matching_results(results, name, "blood fury reuse timer", SKILL_BLOOD_FURY) == MatchType::Exact)
		{
			return results;
		}
		break;

	case 'c':
		if (add_matching_results(results, name, "CANT_QUIT", FUCK_CANTQUIT) == MatchType::Exact)
		{
			return results;
		}
		if (add_matching_results(results, name, "clanarea claim timer", SKILL_CLANAREA_CLAIM) == MatchType::Exact)
		{
			return results;
		}
		if (add_matching_results(results, name, "clanarea challenge timer", SKILL_CLANAREA_CHALLENGE) == MatchType::Exact)
		{
			return results;
		}
		if (add_matching_results(results, name, "cannot cast timer", SPELL_NO_CAST_TIMER) == MatchType::Exact)
		{
			return results;
		}
		if (add_matching_results(results, name, "crazed assault reuse timer", SKILL_CRAZED_ASSAULT) == MatchType::Exact)
		{
			return results;
		}
		break;

	case 'd':
		if (add_matching_results(results, name, "DIRTY_THIEF/CANT_QUIT", FUCK_PTHIEF) == MatchType::Exact)
		{
			return results;
		}
		if (add_matching_results(results, name, "divine intervention timer", SPELL_DIV_INT_TIMER) == MatchType::Exact)
		{
			return results;
		}
		break;

	case 'g':
		if (add_matching_results(results, name, "GOLD_THIEF/CANT_QUIT", FUCK_GTHIEF) == MatchType::Exact)
		{
			return results;
		}
		break;

	case 'h':
		if (add_matching_results(results, name, "harmtouch reuse timer", SKILL_HARM_TOUCH) == MatchType::Exact)
		{
			return results;
		}
		if (add_matching_results(results, name, "holy aura timer", SPELL_HOLY_AURA_TIMER) == MatchType::Exact)
		{
			return results;
		}
		break;

	case 'l':
		if (add_matching_results(results, name, "layhands reuse timer", SKILL_LAY_HANDS) == MatchType::Exact)
		{
			return results;
		}
		break;

	case 'n':
		if (add_matching_results(results, name, "natural selection", SKILL_NAT_SELECT) == MatchType::Exact)
		{
			return results;
		}
		if (add_matching_results(results, name, "natural select timer", SPELL_NAT_SELECT_TIMER) == MatchType::Exact)
		{
			return results;
		}
		break;

	case 'p':
		if (add_matching_results(results, name, "profession", SKILL_PROFESSION) == MatchType::Exact)
		{
			return results;
		}
		break;

	case 'q':
		if (add_matching_results(results, name, "quiver reuse timer", SKILL_QUIVERING_PALM) == MatchType::Exact)
		{
			return results;
		}
		break;

	default:
		break;
	};

	return results;
}

int find_skill_num(char *name)
{
	int i;
	unsigned int name_length = strlen(name);

	// try ki
	for (i = 0; *ki[i] != '\n'; i++)
		if (name_length <= strlen(ki[i]) && !str_n_nosp_cmp(name, ki[i], name_length))
			return (i + KI_OFFSET);

	// try spells
	for (i = 0; *spells[i] != '\n'; i++)
		if (name_length <= strlen(spells[i]) && !str_n_nosp_cmp(name, spells[i], name_length))
			return (i + 1);

	// try skills
	for (i = 0; *skills[i] != '\n'; i++)
		if (name_length <= strlen(skills[i]) && !str_n_nosp_cmp(name, skills[i], name_length))
			return (i + SKILL_BASE);

	// try songs
	for (i = 0; *songs[i] != '\n'; i++)
		if (name_length <= strlen(songs[i]) && !str_n_nosp_cmp(name, songs[i], name_length))
			return (i + SKILL_SONG_BASE);

	// sets?
	for (i = 0; *set_list[i].SetName != '\n'; i++)
		if (name_length <= strlen(set_list[i].SetName) && !str_n_nosp_cmp(name, set_list[i].SetName, name_length))
			return (i + BASE_SETS);
	// timers/other stuff
	switch (LOWER(*name))
	{
	case 'b':
		if (name_length <= strlen("blood fury reuse timer") && !str_n_nosp_cmp(name, "blood fury reuse timer", name_length))
			return SKILL_BLOOD_FURY;
		break;
	case 'c':
		if (name_length <= strlen("CANT_QUIT") && !str_n_nosp_cmp(name, "CANT_QUIT", name_length))
			return FUCK_CANTQUIT;
		if (name_length <= strlen("clanarea claim timer") && !str_n_nosp_cmp(name, "clanarea claim timer", name_length))
			return SKILL_CLANAREA_CLAIM;
		if (name_length <= strlen("clanarea challenge timer") && !str_n_nosp_cmp(name, "clanarea claim timer", name_length))
			return SKILL_CLANAREA_CHALLENGE;
		if (name_length <= strlen("cannot cast timer") && !str_n_nosp_cmp(name, "cannot cast timer", name_length))
			return SPELL_NO_CAST_TIMER;
		if (name_length <= strlen("crazed assault reuse timer") && !str_n_nosp_cmp(name, "crazed assault reuse timer", name_length))
			return SKILL_CRAZED_ASSAULT;
		break;
	case 'd':
		if (name_length <= strlen("DIRTY_THIEF/CANT_QUIT") && !str_n_nosp_cmp(name, "DIRTY_THIEF/CANT_QUIT", name_length))
			return FUCK_PTHIEF;
		if (name_length <= strlen("divine intervention timer") && !str_n_nosp_cmp(name, "divine intervention timer", name_length))
			return SPELL_DIV_INT_TIMER;
		break;
	case 'g':
		if (name_length <= strlen("GOLD_THIEF/CANT_QUIT") && !str_n_nosp_cmp(name, "GOLD_THIEF/CANT_QUIT", name_length))
			return FUCK_GTHIEF;
		break;
	case 'h':
		if (name_length <= strlen("harmtouch reuse timer") && !str_n_nosp_cmp(name, "harmtouch reuse timer", name_length))
			return SKILL_HARM_TOUCH;
		if (name_length <= strlen("holy aura timer") && !str_n_nosp_cmp(name, "holy aura timer", name_length))
			return SPELL_HOLY_AURA_TIMER;
		break;
	case 'l':
		if (name_length <= strlen("layhands reuse timer") && !str_n_nosp_cmp(name, "layhands reuse timer", name_length))
			return SKILL_LAY_HANDS;
		break;
	case 'n':
		if (name_length <= strlen("natural selection") && !str_n_nosp_cmp(name, "natural selection", name_length))
			return SKILL_NAT_SELECT;
		if (name_length <= strlen("natural select timer") && !str_n_nosp_cmp(name, "natural select timer", name_length))
			return SPELL_NAT_SELECT_TIMER;
		break;
	case 'p':
		if (name_length <= strlen("profession") && !str_n_nosp_cmp(name, "profession", name_length))
			return SKILL_PROFESSION;
		break;
	case 'q':
		if (name_length <= strlen("quiver reuse timer") && !str_n_nosp_cmp(name, "quiver reuse timer", name_length))
			return SKILL_QUIVERING_PALM;
		break;
	default:
		break;
	};

	return -1;
}
