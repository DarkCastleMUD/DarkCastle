/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy...         N'Atas-Ha *
 ***************************************************************************/

#include <sys/types.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>

#include <map>
#include <string>
#include <algorithm>

#include <fmt/format.h>
#include <QString>

#include "DC/fileinfo.h"
#include "DC/act.h"
#include "DC/player.h"
#include "DC/room.h"
#include "DC/structs.h"
#include "DC/fight.h"
#include "DC/spells.h"
#include "DC/utility.h"
#include "DC/connect.h"
#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/comm.h"
#include "DC/returnvals.h"
#include "DC/const.h"
#include "DC/inventory.h"
#include "DC/DC.h"
#include "DC/Trace.h"

// Extern variables

Character *rndm2;

int activeProgs = 0; // loop protection

Character *activeActor = nullptr;
Character *activeRndm = nullptr;
Character *activeTarget = nullptr;
Object *activeObj = nullptr;
void *activeVo = nullptr;

char *activeProg;
char *activePos;
char *activeProgTmpBuf;
// Global defined here

bool MOBtrigger;
struct mprog_throw_type *g_mprog_throw_list = 0; // holds all pending mprog throws

SelfPurge::SelfPurge()
{
}

SelfPurge::SelfPurge(bool s)
{
	state = s;
}

SelfPurge::operator bool() const
{
	return state;
}

void SelfPurge::setOwner(Character *c, std::string m)
{
	owner = c;
	function = m;
}

std::string SelfPurge::getFunction(void) const
{
	return function;
}

bool SelfPurge::getState(void) const
{
	return state;
}

selfpurge_t selfpurge = {};

int cIfs[256]; // for MPPAUSE
int ifpos;

// This 2 variables keep track of what command and line number a mprog script
// is on for error logging purposes.
int mprog_command_num = 0;
int mprog_line_num = 0;

/*
 * Local function prototypes
 */

int mprog_seval(Character *ch, const char *lhs, const char *opr, const char *rhs);

int mprog_veval(int64_t lhs, char *opr, int64_t rhs);
int mprog_do_ifchck(char *ifchck, Character *mob,
					Character *actor, Object *obj,
					void *vo, Character *rndm);
char *mprog_process_if(char *ifchck, char *com_list,
					   Character *mob, Character *actor,
					   Object *obj, void *vo,
					   Character *rndm, struct mprog_throw_type *thrw = nullptr);
void mprog_translate(char ch, char *t, Character *mob,
					 Character *actor, Object *obj,
					 void *vo, Character *rndm);
int mprog_process_cmnd(char *cmnd, Character *mob,
					   Character *actor, Object *obj,
					   void *vo, Character *rndm);

/***************************************************************************
 * Local function code and brief comments.
 */

/* Used to get sequential lines of a multi line std::string (separated by "\n\r")
 * Thus its like one_argument(), but a trifle different. It is destructive
 * to the multi line std::string argument, and thus clist must not be shared.
 */
char *mprog_next_command(char *clist)
{
	bool open = false;
	char *pointer = clist;

	for (; *pointer != '\0'; pointer++)
	{
		if (!open && (*pointer == '\n' || *pointer == '\r'))
			break;
		if (*pointer == '{')
			open = true;
		if (open && *pointer == '}')
			open = false;
	}
	//  while ( *pointer != '\n' && *pointer != '\0' )
	//    pointer++;
	while (*pointer == '\n' || *pointer == '\r')
		*pointer++ = '\0';

	/*  if ( *pointer == '\n' )
		*pointer++ = '\0';
	  if ( *pointer == '\r' )
		*pointer++ = '\0';*/

	mprog_line_num++;
	return (pointer);
}

/* These two functions do the basic evaluation of ifcheck operators.
 *  It is important to note that the std::string operations are not what
 *  you probably expect.  Equality is exact and division is substring.
 *  remember that lhs has been stripped of leading space, but can
 *  still have trailing spaces so be careful when editing since:
 *  "guard" and "guard " are not equal.
 */
int mprog_seval(Character *ch, const char *lhs, const char *opr, const char *rhs)
{
	if (!lhs || !rhs)
		return false;
	if (!str_cmp(opr, "=="))
		return (!str_cmp(lhs, rhs));
	if (!str_cmp(opr, "!="))
		return (str_cmp(lhs, rhs));
	if (!str_cmp(opr, "/"))
		return (!str_infix(rhs, lhs));
	if (!str_cmp(opr, "!/"))
		return (str_infix(rhs, lhs));

	prog_error(ch, "Improper MOBprog operator");

	logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Improper MOBprog operator\n\r", 0);
	return 0;
}

bool Character::mprog_seval(QString lhs, QString opr, QString rhs)
{
	if (lhs.isEmpty() || rhs.isEmpty())
		return false;

	if (opr == "==")
		return lhs == rhs;
	if (opr == "!=")
		return lhs != rhs;
	if (opr == "/")
		return !str_infix(rhs, lhs);
	if (opr == "!/")
		return str_infix(rhs, lhs);

	prog_error(this, "Improper MOBprog operator");

	logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Improper MOBprog operator\n\r", 0);
	return false;
}

/*
int mprog_veval( int lhs, char *opr, int rhs )
{

  if ( !str_cmp( opr, "==" ) )
	return ( lhs == rhs );
  if ( !str_cmp( opr, "!=" ) )
	return ( lhs != rhs );
  if ( !str_cmp( opr, ">" ) )
	return ( lhs > rhs );
  if ( !str_cmp( opr, "<" ) )
	return ( lhs < rhs );
  if ( !str_cmp( opr, "<=" ) )
	return ( lhs <= rhs );
  if ( !str_cmp( opr, ">=" ) )
	return ( lhs >= rhs );
  if ( !str_cmp( opr, "&" ) )
	return ( lhs & rhs );
  if ( !str_cmp( opr, "|" ) )
	return ( lhs | rhs );

  logf( IMMORTAL, DC::LogChannel::LOG_WORLD,  "Improper MOBprog operator\n\r", 0 );
  return 0;

}
*/
int mprog_veval(int64_t lhs, char *opr, int64_t rhs)
{

	if (!str_cmp(opr, "=="))
		return (lhs == rhs);
	if (!str_cmp(opr, "!="))
		return (lhs != rhs);
	if (!str_cmp(opr, ">"))
		return (lhs > rhs);
	if (!str_cmp(opr, "<"))
		return (lhs < rhs);
	if (!str_cmp(opr, "<="))
		return (lhs <= rhs);
	if (!str_cmp(opr, ">="))
		return (lhs >= rhs);
	if (!str_cmp(opr, "&"))
		return (lhs & rhs);
	if (!str_cmp(opr, "|"))
		return (lhs | rhs);

	logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Improper MOBprog operator\n\r", 0);
	return 0;
}
/*
int mprog_veval( uint64_t lhs, char *opr, uint64_t rhs )
{
  if ( !str_cmp( opr, "==" ) )
	return ( lhs == rhs );
  if ( !str_cmp( opr, "!=" ) )
	return ( lhs != rhs );
  if ( !str_cmp( opr, ">" ) )
	return ( lhs > rhs );
  if ( !str_cmp( opr, "<" ) )
	return ( lhs < rhs );
  if ( !str_cmp( opr, "<=" ) )
	return ( lhs <= rhs );
  if ( !str_cmp( opr, ">=" ) )
	return ( lhs >= rhs );
  if ( !str_cmp( opr, "&" ) )
	return ( lhs & rhs );
  if ( !str_cmp( opr, "|" ) )
	return ( lhs | rhs );

  logf( IMMORTAL, DC::LogChannel::LOG_WORLD,  "Improper MOBprog operator\n\r", 0 );
  return 0;

}
*/

bool istank(Character *ch)
{
	Character *t;
	if (!ch->in_room)
		return false;
	for (t = DC::getInstance()->world[ch->in_room].people; t; t = t->next_in_room)
		if (t->fighting == ch && t != ch)
			return true;
	return false;
}

void translate_value(char *leftptr, char *rightptr, int16_t **vali,
					 uint32_t **valui, char ***valstr, int64_t **vali64, uint64_t **valui64, int8_t **valb,
					 Character *mob, Character *actor, Object *obj, void *vo,
					 Character *rndm, QString &valqstr)
{
	/*
	 $n.age
	 '$n' = left
	 'age' = right

	 $n,7.hasskill
	 '$n' = left
	 '7' = half
	 'hasskill' = right
	 */

	Character *target = nullptr;
	Object *otarget = nullptr;
	int rtarget = -1, ztarget = -1;
	bool valset = false; // done like that to determine if value is set, since it can be 0
	struct tempvariable *mobTempVar = nullptr;
	if (mob)
	{
		mobTempVar = mob->tempVariable;
	}

	activeTarget = nullptr;
	char *tmp, half[MAX_INPUT_LENGTH];
	half[0] = '\0';
	if ((tmp = strchr(leftptr, ',')) != nullptr)
	{
		*tmp = '\0';
		tmp++;
		one_argument(tmp, half); // strips whatever spaces
	}

	// Less nitpicky about the mobprogs with this stuff below in.
	char larr[MAX_INPUT_LENGTH];
	one_argument(leftptr, larr);
	char *left = &larr[0];

	char rarr[MAX_INPUT_LENGTH];
	one_argument(rightptr, rarr);
	char *right = &rarr[0];
	bool silent = false;

	if (!str_prefix("world_", left))
	{
		left += 6;

		const auto &character_list = DC::getInstance()->character_list;
		auto result = find_if(character_list.begin(), character_list.end(),
							  [&target, &left](Character *const &tmp)
							  {
								  if (isexact(left, GET_NAME(tmp)))
								  {
									  target = tmp;
									  return true;
								  }
								  else
								  {
									  return false;
								  }
							  });
	}
	else if (!str_prefix("zone_", left))
	{
		left += 5;

		const auto &character_list = DC::getInstance()->character_list;
		auto result = find_if(character_list.begin(), character_list.end(),
							  [&target, &left, &mob](Character *const &tmp)
							  {
								  if (tmp->in_room != DC::NOWHERE && DC::getInstance()->world[mob->in_room].zone == DC::getInstance()->world[tmp->in_room].zone && isexact(left, GET_NAME(tmp)))
								  {
									  target = tmp;
									  return true;
								  }
								  else
								  {
									  return false;
								  }
							  });
	}
	else if (!str_prefix("mroom_", left))
	{
		left += 6;
		Character *tmp;
		for (tmp = DC::getInstance()->world[mob->in_room].people; tmp; tmp = tmp->next_in_room)
		{
			if (isexact(left, GET_NAME(tmp)))
			{
				target = tmp;
				break;
			}
		}
	}
	else if (!str_prefix("room_", left))
	{
		left += 5;
		if (is_number(left))
			rtarget = real_room(atoi(left));
		else
			rtarget = mob->in_room;
	}
	else if (!str_prefix("oworld_", left))
	{
		left += 7;
		otarget = get_obj(left);
	}
	else if (!str_prefix("ozone_", left))
	{
		left += 6;
		Object *otmp;
		int z = DC::getInstance()->world[mob->in_room].zone;
		for (otmp = DC::getInstance()->object_list; otmp; otmp = otmp->next)
		{
			Object *cmp = otmp->in_obj ? otmp->in_obj : otmp;
			if ((cmp->in_room != DC::NOWHERE && DC::getInstance()->world[cmp->in_room].zone == z) || (cmp->carried_by && DC::getInstance()->world[cmp->carried_by->in_room].zone == z) || (cmp->equipped_by && DC::getInstance()->world[cmp->equipped_by->in_room].zone == z))
				if (isexact(left, otmp->name))
				{
					otarget = otmp;
					break;
				}
		}
		otarget = get_obj(left);
	}
	else if (!str_prefix("oroom_", left))
	{
		left += 6;
		otarget = get_obj_in_list(left, DC::getInstance()->world[mob->in_room].contents);
	}
	else if (!str_prefix("zone_", left))
	{
		ztarget = DC::getInstance()->world[mob->in_room].zone;
		left += 5;
	}
	else if (*left == '$')
	{
		uint8_t number_of_dollar_signs = 1;
		while (*(left + number_of_dollar_signs) == '$')
		{
			number_of_dollar_signs++;
		}

		switch (*(left + number_of_dollar_signs))
		{
		case '$':
			break;
		case 'n':
			target = actor;
			break;
		case 'i':
			target = mob;
			break;
		case 'r':
			target = rndm;
			silent = true;
			break;

		case 't':
			target = (Character *)vo;
			break;
		case 'o':
			otarget = obj;
			break;
		case 'p':
			otarget = (Object *)vo;
			break;
		case 'f':
			if (actor)
				target = actor->fighting;
			break;
		case 'g':
			if (mob)
				target = mob->fighting;
			break;
		case 'v':
			char buf[MAX_STRING_LENGTH];
			buf[0] = '\0';
			int i;
			for (i = 2; *(left + i); i++)
			{
				if (*(left + i) == '[')
					continue;
				if (*(left + i) == ']')
				{
					buf[i - 3] = '\0';
					break;
				}
				buf[i - 3] = *(left + i);
			}
			for (; mobTempVar; mobTempVar = mobTempVar->next)
				if (!str_cmp(buf, mobTempVar->name.toStdString().c_str()))
					break;
			if (mobTempVar)
				strcpy(buf, mobTempVar->data.toStdString().c_str());

			if (buf[0] != '\0')
			{
				if (!is_number(buf))
					target = get_char_room(buf, mob->in_room);
				else
				{
					valset = true;
				}
			}
			break;
		default:
			break;
		}
	}
	else
	{
		if (!is_number(left))
			target = get_char_room(left, mob->in_room);
		else
		{
			valset = true;
		}
	}

	if (!target && !otarget && ztarget == -1 && rtarget == -1 && !valset && str_cmp(right, "numpcs") && str_cmp(right, "hitpoints") && str_cmp(right, "move") && str_cmp(right, "mana") && str_cmp(right, "isdaytime") && str_cmp(right, "israining"))
	{
		if (!silent)
		{
			if (mob == nullptr)
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s mob == nullptr", left, right);
			}
			else if (mob->mobdata == nullptr)
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s mob->mobdata == nullptr", left, right);
			}
			else if (mob->mobdata->vnum < 0)
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s mob->mobdata->nr = %d < 0 ", left, right, mob->mobdata->vnum);
			}
			else
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: Mob: %d invalid target in mobprog", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			}
		}
		return;
	}
	activeTarget = target;
	// target acquired. fucking boring code.
	// more boring code. FUCK.
	int16_t *intval = nullptr;
	uint32_t *uintval = nullptr;
	char **stringval = nullptr;
	int64_t *llval = nullptr;
	uint64_t *ullval = nullptr;
	int8_t *sbval = nullptr;
	bool tError = false;

	/*
	 When a variable is created and assigned the value of the target-data, it is because it is
	 not meant to be modify-able through mob-progs, such as character class.
	 */

	switch (LOWER(*right))
	{
	case 'a':
		if (!str_cmp(right, "armor"))
		{
			if (!target)
				tError = true;
			else
			{
				intval = &target->armor;
				break;
			}
		}
		else if (!str_cmp(right, "actflags1"))
		{
			if (!target || IS_PC(target))
				tError = true;
			else
			{
				uintval = &target->mobdata->actflags[0];
				break;
			}
		}
		else if (!str_cmp(right, "actflags2"))
		{
			if (!target || IS_PC(target))
				tError = true;
			else
			{
				uintval = &target->mobdata->actflags[1];
				break;
			}
		}
		else if (!str_cmp(right, "affected1"))
		{
			if (!target)
				tError = true;
			else
			{
				uintval = &target->affected_by[0];
				break;
			}
		}
		else if (!str_cmp(right, "affected2"))
		{
			if (!target)
				tError = true;
			else
			{
				uintval = &target->affected_by[1];
				break;
			}
		}
		else if (!str_cmp(right, "alignment"))
		{
			if (!target)
				tError = true;
			else
			{
				intval = &target->alignment;
				break;
			}
		}
		else if (!str_cmp(right, "acidsave"))
		{
			if (!target)
				tError = true;
			else
			{
				intval = &target->saves[SAVE_TYPE_ACID];
				break;
			}
		}
		else if (!str_cmp(right, "age"))
		{
			if (!target)
				tError = true;
			else
			{
				int16_t ageint = target->age().year;
				intval = &ageint;
			}
		}
		break;
	case 'b':
		if (!str_cmp(right, "bank"))
		{
			if (!target || !target->player)
				tError = true;
			else
			{
				uintval = &target->player->bank;
			}
		}
		break;
	case 'c':
		if (!str_cmp(right, "carriedby"))
		{
			if (!otarget)
				tError = true;
			else if (otarget->carried_by)
			{
				valqstr = otarget->carried_by->getName();
			}
			else if (otarget->equipped_by)
			{
				valqstr = otarget->equipped_by->getName();
			}
			else if (otarget->in_obj && otarget->in_obj->carried_by)
			{
				valqstr = otarget->in_obj->carried_by->getName();
			}
			else if (otarget->in_obj && otarget->in_obj->equipped_by)
			{
				valqstr = otarget->in_obj->equipped_by->getName();
			}
			else
				stringval = nullptr;
		}
		else if (!str_cmp(right, "carryingitems"))
		{
			if (!target)
				tError = true;
			else
			{
				int16_t car = target->carry_items;
				intval = &car;
			}
		}
		else if (!str_cmp(right, "carryingweight"))
		{
			if (!target)
				tError = true;
			else
			{
				int16_t car = target->carry_weight;
				intval = &car;
			}
		}
		else if (!str_cmp(right, "class"))
		{
			if (!target)
				tError = true;
			else
			{
				sbval = &target->c_class;
			}
		}
		else if (!str_cmp(right, "coldsave"))
		{
			if (!target)
				tError = true;
			else
			{
				intval = &target->saves[SAVE_TYPE_COLD];
			}
		}
		else if (!str_cmp(right, "constitution"))
		{
			if (!target)
				tError = true;
			else
			{
				sbval = &target->con;
			}
		}
		else if (!str_cmp(right, "cost"))
		{
			if (!otarget)
				tError = true;
			else
			{
				uintval = (uint32_t *)&otarget->obj_flags.cost;
			}
		}
		break;
	case 'd':
		if (!str_cmp(right, "damroll"))
		{
			if (!target)
				tError = true;
			else
			{
				intval = &target->damroll;
			}
		}
		else if (!str_cmp(right, "description"))
		{
			if (!target && !otarget && !rtarget)
				tError = true;
			else if (otarget)
			{
				stringval = &otarget->description;
			}
			else if (rtarget >= 0)
			{
				if (DC::getInstance()->rooms.contains(rtarget))
					stringval = &DC::getInstance()->world[rtarget].description;
				else
					tError = true;
			}
			else
			{
				stringval = &target->description;
			}
		}
		else if (!str_cmp(right, "dexterity"))
		{
			if (!target)
				tError = true;
			else
			{
				sbval = &target->dex;
			}
		}
		else if (!str_cmp(right, "drunk"))
		{
			if (!target)
				tError = true;
			else
			{
				sbval = &target->conditions[DRUNK];
			}
		}
		break;
	case 'e':
		if (!str_cmp(right, "energysaves"))
		{
			if (!target)
				tError = true;
			else
				intval = &target->saves[SAVE_TYPE_ACID];
		}
		else if (!str_cmp(right, "experience"))
		{
			if (!target)
				tError = true;
			else
			{
				llval = &target->exp;
			}
		}
		else if (!str_cmp(right, "extra"))
		{
			if (!otarget)
				tError = true;
			else
			{
				uintval = &otarget->obj_flags.extra_flags;
			}
		}
		break;
	case 'f':
		if (!str_cmp(right, "firesaves"))
		{
			if (!target)
				tError = true;
			else
				intval = &target->saves[SAVE_TYPE_FIRE];
		}
		else if (!str_cmp(right, "flags"))
		{
			if (!rtarget)
				tError = true;
			else
				uintval = &DC::getInstance()->world[rtarget].room_flags;
		}
		break;
	case 'g':
		if (!str_cmp(right, "gold"))
		{
			if (!target)
				tError = true;
			else
				ullval = &target->getGoldReference();
		}
		else if (!str_cmp(right, "glowfactor"))
		{
			if (!target)
				tError = true;
			else
				intval = &target->glow_factor;
		}
		break;
	case 'h':
		if (!str_cmp(right, "hasskill"))
		{
			if (!target)
				tError = true;
			else
			{
				int skl = 0;
				if (*half == '\0' || (skl = atoi(half)) < 0)
				{
					logf(IMMORTAL, DC::LogChannel::LOG_WORLD,
						 "translate_value: Mob: %d invalid skillnumber in hasskill",
						 DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
					tError = true;
				}
				int16_t sklint = target->has_skill(skl);
				intval = &sklint;
			}
		}
		else if (!str_cmp(right, "height"))
		{
			if (!target)
				tError = true;
			else
				sbval = (int8_t *)(&target->height);
		}
		else if (!str_cmp(right, "hitpoints"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->hit;
		}
		else if (!str_cmp(right, "hitroll"))
		{
			if (!target)
				tError = true;
			else
				intval = &target->hitroll;
		}
		else if (!str_cmp(right, "homeroom"))
		{
			if (!target)
				tError = true;
			else
				intval = &target->hometown;
		}
		else if (!str_cmp(right, "hunger"))
		{
			if (!target)
				tError = true;
			else
			{
				sbval = &target->conditions[FULL];
			}
		}
		break;
	case 'i':
		if (!str_cmp(right, "immune"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->immune;
		}
		else if (!str_cmp(right, "inroom"))
		{
			if (!target && !otarget)
				tError = true;
			else if (target)
			{
				static uint32_t tmp;
				tmp = (uint32_t)target->in_room;
				uintval = &tmp;
			}
			else
			{
				static uint32_t tmp;
				tmp = (uint32_t)otarget->in_room;
				uintval = &tmp;
			}
		}
		else if (!str_cmp(right, "intelligence"))
		{
			if (!target)
				tError = true;
			else
				sbval = (int8_t *)&target->intel;
		}
		break;
	case 'l':
		if (!str_cmp(right, "level"))
		{
			if (!target && !otarget)
				tError = true;
			else if (otarget)
			{
				intval = reinterpret_cast<decltype(intval)>(&otarget->obj_flags.eq_level);
			}
			else
			{
				if (IS_NPC(target))
					sbval = reinterpret_cast<decltype(sbval)>(target->getLevelPtr());
				else
					sbval = reinterpret_cast<decltype(sbval)>(target->getLevelPtr());
			}
		}
		else if (!str_cmp(right, "long"))
		{
			if (!target)
				tError = true;
			else
			{
				stringval = &target->long_desc;
			}
		}
		break;
	case 'k':
		if (!str_cmp(right, "ki"))
		{
			if (!target)
				tError = true;
			else
			{
				uintval = (uint32_t *)&target->ki;
			}
		}
		break;
	case 'm':
		if (!str_cmp(right, "magicsaves"))
		{
			if (!target)
				tError = true;
			else
				intval = &target->saves[SAVE_TYPE_MAGIC];
		}
		else if (!str_cmp(right, "mana"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->mana;
		}
		else if (!str_cmp(right, "maxhitpoints"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->max_hit;
		}
		else if (!str_cmp(right, "maxmana"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->max_mana;
		}
		else if (!str_cmp(right, "maxmove"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->max_move;
		}
		else if (!str_cmp(right, "maxki"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->max_ki;
		}
		else if (!str_cmp(right, "meleemit"))
		{
			if (!target)
				tError = true;
			else
				intval = &target->melee_mitigation;
		}
		else if (!str_cmp(right, "misc"))
		{
			if (!target)
				tError = true;
			else
				uintval = &target->misc;
		}
		else if (!str_cmp(right, "more"))
		{
			if (!otarget)
				tError = true;
			else
			{
				uintval = &otarget->obj_flags.more_flags;
			}
		}
		else if (!str_cmp(right, "move"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)target->getMovePtr();
		}
		break;
	case 'n':
		if (!str_cmp(right, "name"))
		{
			if (!target && !rtarget)
				tError = true;
			else if (rtarget >= 0)
			{
				if (DC::getInstance()->rooms.contains(rtarget))
					stringval = &DC::getInstance()->world[rtarget].name;
				else
					tError = true;
			}
			else
			{
				valqstr = target->getName();
			}
		}
		break;
	case 'o':
		break;
	case 'p':
		if (!str_cmp(right, "platinum"))
		{
			if (!target)
				tError = true;
			else
				uintval = &target->plat;
		}
		else if (!str_cmp(right, "poisonsaves"))
		{
			if (!target)
				tError = true;
			else
				intval = &target->saves[SAVE_TYPE_POISON];
		}
		else if (!str_cmp(right, "position"))
		{
			if (!target)
				tError = true;
			else
				intval = (int16_t *)target->getPositionPtr();
		}
		else if (!str_cmp(right, "practices"))
		{
			if (!target || !target->player)
				tError = true;
			else
				intval = (int16_t *)&target->player->practices;
		}
		break;
	case 'q':
		break;
	case 'r':
		if (!str_cmp(right, "race"))
		{
			if (!target)
				tError = true;
			else
				sbval = &target->race;
		}
		else if (!str_cmp(right, "rawstr"))
		{
			if (!target)
				tError = true;
			else
				sbval = &target->raw_str;
		}
		else if (!str_cmp(right, "rawcon"))
		{
			if (!target)
				tError = true;
			else
				sbval = &target->raw_con;
		}
		else if (!str_cmp(right, "rawwis"))
		{
			if (!target)
				tError = true;
			else
				sbval = &target->raw_wis;
		}
		else if (!str_cmp(right, "rawdex"))
		{
			if (!target)
				tError = true;
			else
				sbval = &target->raw_dex;
		}
		else if (!str_cmp(right, "rawint"))
		{
			if (!target)
				tError = true;
			else
				sbval = &target->raw_intel;
		}
		else if (!str_cmp(right, "rawhit"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->raw_hit;
		}
		else if (!str_cmp(right, "rawmana"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->raw_mana;
		}
		else if (!str_cmp(right, "rawmove"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->raw_move;
		}
		else if (!str_cmp(right, "rawki"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->raw_ki;
		}
		else if (!str_cmp(right, "resist"))
		{
			if (!target)
				tError = true;
			else
				uintval = &target->resist;
		}
		break;
	case 's':
		if (!str_cmp(right, "sex"))
		{
			if (!target)
				tError = true;
			else
				sbval = reinterpret_cast<decltype(sbval)>(&target->sex);
		}
		else if (!str_cmp(right, "size"))
		{
			if (!otarget)
				tError = true;
			else
			{
				intval = (int16_t *)&otarget->obj_flags.size;
			}
		}
		else if (!str_cmp(right, "short"))
		{
			if (!target && !otarget)
				tError = true;
			else if (otarget)
			{
				stringval = &otarget->short_description;
			}
			else
				stringval = &target->short_desc;
		}
		else if (!str_cmp(right, "songmit"))
		{
			if (!target)
				tError = true;
			else
				intval = &target->song_mitigation;
		}
		else if (!str_cmp(right, "spelleffect"))
		{
			if (!target)
				tError = true;
			else
				uintval = (uint32_t *)&target->spelldamage;
		}
		else if (!str_cmp(right, "spellmit"))
		{
			if (!target)
				tError = true;
			else
				intval = &target->spell_mitigation;
		}
		else if (!str_cmp(right, "strength"))
		{
			if (!target)
				tError = true;
			else
				sbval = &target->str;
		}
		else if (!str_cmp(right, "suscept"))
		{
			if (!target)
				tError = true;
			else
				uintval = &target->suscept;
		}
		break;
	case 't':
		if (!str_cmp(right, "temp"))
		{
			if (!half[0] || !target)
			{
				tError = true;
			}
			else
			{
				valqstr = target->getTemp(half);
			}
		}
		else if (!str_cmp(right, "title"))
		{
			if (!target)
				tError = true;
			else
				stringval = &target->title;
		}
		else if (!str_cmp(right, "type"))
		{
			if (!otarget)
				tError = true;
			else
				sbval = (int8_t *)&otarget->obj_flags.type_flag;
		}
		else if (!str_cmp(right, "thirst"))
		{
			if (!target)
				tError = true;
			else
			{
				sbval = &target->conditions[THIRST];
			}
		}
		break;
	case 'v':
		if (!str_cmp(right, "value0"))
		{
			if (!otarget)
				tError = true;
			else
				uintval = (uint32_t *)&otarget->obj_flags.value[0];
		}
		else if (!str_cmp(right, "value1"))
		{
			if (!otarget)
				tError = true;
			else
				uintval = (uint32_t *)&otarget->obj_flags.value[1];
		}
		else if (!str_cmp(right, "value2"))
		{
			if (!otarget)
				tError = true;
			else
				uintval = (uint32_t *)&otarget->obj_flags.value[2];
		}
		else if (!str_cmp(right, "value3"))
		{
			if (!otarget)
				tError = true;
			else
				uintval = (uint32_t *)&otarget->obj_flags.value[3];
		}
		break;
	case 'w':
		if (!str_cmp(right, "wearable"))
		{
			if (!otarget)
				tError = true;
			else
				uintval = &otarget->obj_flags.wear_flags;
		}
		else if (!str_cmp(right, "weight"))
		{
			if (!target && !otarget)
				tError = true;
			else if (otarget)
			{
				intval = &otarget->obj_flags.weight;
			}
			else
				sbval = (int8_t *)&target->weight;
		}
		else if (!str_cmp(right, "wisdom"))
		{
			if (!target)
				tError = true;
			else
				sbval = &target->wis;
		}
		break;
	default:
		break;
	}
	if (tError)
	{
		logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s target=%p actor=%p mob=%p", left, right, target, actor, mob);

		if (mob)
		{
			if (mob->mobdata == nullptr)
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s mob->mobdata == nullptr", left, right);
			}
			else if (mob->mobdata->vnum < 0)
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s mob->mobdata->nr = %d < 0 ", left, right, mob->mobdata->vnum);
			}
			else
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: Mob: %d tried to access non-existent field of target", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			}
		}
		return;
	}
	if (intval)
		*vali = intval;
	if (uintval)
		*valui = uintval;
	if (stringval)
		*valstr = stringval;
	if (llval)
		*vali64 = llval;
	if (ullval)
		*valui64 = ullval;
	if (sbval)
		*valb = sbval;

	return;
}

/* This function performs the evaluation of the if checks.  It is
 * here that you can add any ifchecks which you so desire. Hopefully
 * it is clear from what follows how one would go about adding your
 * own. The syntax for an if check is: ifchck ( arg ) [opr val]
 * where the parenthesis are required and the opr and val fields are
 * optional but if one is there then both must be. The spaces are all
 * optional. The evaluation of the opr expressions is farmed out
 * to reduce the redundancy of the mammoth if statement list.
 * If there are errors, then return -1 otherwise return boolean 1,0
 */

// Azrack -- this was originally returning a bool, but its returning all sorts of values,
// switched it to int

enum mprog_ifs
{
	eRAND = 1, // start this at 1, std::map returns 0 if not found
	eRAND1K,
	eAMTITEMS,
	eNUMPCS,
	eNUMOFMOBSINWORLD,
	eNUMOFMOBSINROOM,
	eISPC,
	eISWIELDING,
	eISWEAPPRI,
	eISWEAPSEC,
	eISNPC,
	eISGOOD,
	eISNEUTRAL,
	eISEVIL,
	eISWORN,
	eISFIGHT,
	eISTANK,
	eISIMMORT,
	eISCHARMED,
	eISFOLLOW,
	eISSPELLED,
	eISAFFECTED,
	eHITPRCNT,
	eWEARS,
	eCARRIES,
	eNUMBER,
	eTEMPVAR,
	eISMOBVNUMINROOM,
	eISOBJVNUMINROOM,
	eCANSEE,
	eHASDONEQUEST1,
	eINSAMEZONE,
	eCLAN,
	eISDAYTIME,
	eISRAINING,
	eNUMOFOBJSINWORLD

};

std::map<std::string, mprog_ifs> load_ifchecks()
{
	std::map<std::string, mprog_ifs> ifcheck_tmp;

	ifcheck_tmp["rand"] = eRAND;
	ifcheck_tmp["rand1k"] = eRAND1K;
	ifcheck_tmp["amtitems"] = eAMTITEMS;
	ifcheck_tmp["numpcs"] = eNUMPCS;
	ifcheck_tmp["numofmobsinworld"] = eNUMOFMOBSINWORLD;
	ifcheck_tmp["numofobjsinworld"] = eNUMOFOBJSINWORLD;
	ifcheck_tmp["numofmobsinroom"] = eNUMOFMOBSINROOM;
	ifcheck_tmp["ispc"] = eISPC;
	ifcheck_tmp["iswielding"] = eISWIELDING;
	ifcheck_tmp["isweappri"] = eISWEAPPRI;
	ifcheck_tmp["isweapsec"] = eISWEAPSEC;

	ifcheck_tmp["isnpc"] = eISNPC;
	ifcheck_tmp["isgood"] = eISGOOD;
	ifcheck_tmp["isneutral"] = eISNEUTRAL;
	ifcheck_tmp["isevil"] = eISEVIL;
	ifcheck_tmp["isfight"] = eISFIGHT;

	ifcheck_tmp["istank"] = eISTANK;
	ifcheck_tmp["isimmort"] = eISIMMORT;
	ifcheck_tmp["ischarmed"] = eISCHARMED;
	ifcheck_tmp["isfollow"] = eISFOLLOW;
	ifcheck_tmp["isspelled"] = eISSPELLED;
	ifcheck_tmp["isworn"] = eISWORN;

	ifcheck_tmp["isaffected"] = eISAFFECTED;
	ifcheck_tmp["hitprcnt"] = eHITPRCNT;
	ifcheck_tmp["wears"] = eWEARS;
	ifcheck_tmp["carries"] = eCARRIES;
	ifcheck_tmp["number"] = eNUMBER;

	ifcheck_tmp["tempvar"] = eTEMPVAR;
	ifcheck_tmp["ismobvnuminroom"] = eISMOBVNUMINROOM;
	ifcheck_tmp["isobjvnuminroom"] = eISOBJVNUMINROOM;
	ifcheck_tmp["cansee"] = eCANSEE;

	ifcheck_tmp["insamezone"] = eINSAMEZONE;
	ifcheck_tmp["clan"] = eCLAN;

	ifcheck_tmp["isdaytime"] = eISDAYTIME;
	ifcheck_tmp["israining"] = eISRAINING;

	return ifcheck_tmp;
}

std::map<std::string, mprog_ifs> ifcheck = load_ifchecks();

int mprog_do_ifchck(char *ifchck, Character *mob, Character *actor,
					Object *obj, void *vo, Character *rndm)
{

	char buf[MAX_INPUT_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	char opr[MAX_INPUT_LENGTH];
	char val[MAX_INPUT_LENGTH];
	char val2[MAX_INPUT_LENGTH]; // used for non-traditional
	Character *vict = (Character *)vo;
	Object *v_obj = (Object *)vo;
	char *bufpt = buf;
	char *argpt = arg;
	char *oprpt = opr;
	char *valpt = val;
	char *point = ifchck;
	val2[0] = '\0';

	if (*point == '\0')
	{
		logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: null ifchck: '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
		return -1;
	}
	/* skip leading spaces */
	while (*point == ' ')
		point++;
	bool traditional = false;

	/* get whatever comes before the left paren.. ignore spaces */
	while (*point)
		if (*point == '(')
		{
			traditional = true;
			break;
		}
		else if (*point == ' ')
		{
			prog_error(mob, "ifchck syntax error: '%s'", ifchck);
			return -1;
		}
		else if (*point == '.')
		{
			break;
		}
		else if (*point == '\0')
		{
			prog_error(mob, "ifchck syntax error");
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: ifchck syntax error: '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
			return -1;
		}
		else if (*point == ' ')
			point++;
		else
			*bufpt++ = *point++;

	*bufpt = '\0';
	point++;

	/* get whatever is in between the parens.. ignore spaces */
	while (*point)
		if (traditional && *point == ')')
			break;
		else if (!traditional && !isalpha(*point))
		{
			point--;
			break;
		}
		else if (*point == '\0')
		{
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: ifchck syntax error: '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
			return -1;
		}
		else if (*point == ' ')
			point++;
		else
			*argpt++ = *point++;

	*argpt = '\0';
	point++;

	/* check to see if there is an operator */

	// same for both traditional and non-traditional
	while (*point == ' ' || *point == '\r')
		point++;
	if (*point == '\0')
	{
		*opr = '\0';
		*val = '\0';
	}
	else /* there should be an operator and value, so get them */
	{
		while ((*point != ' ') && (!isalnum(*point)))
			if (*point == '\0')
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: ifchck operator without value: '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
				return -1;
			}
			else
				*oprpt++ = *point++;

		*oprpt = '\0';

		/* finished with operator, skip spaces and then get the value */
		while (*point == ' ')
			point++;
		for (;;)
		{
			if (*point == '.')
			{
				*valpt = '\0';
				valpt = val2;
				point++;
			}
			else if (*point == '\0' || *point == '\r' || *point == '\n')
				break;
			else
				*valpt++ = *point++;
		}

		*valpt = '\0';
	}
	bufpt = buf;
	argpt = arg;
	oprpt = opr;
	valpt = val;

	/* Ok... now buf contains the ifchck, arg contains the inside of the
	 *  parentheses, opr contains an operator if one is present, and val
	 *  has the value if an operator was present.
	 *  So.. basically use if statements and run over all known ifchecks
	 *  Once inside, use the argument and expand the lhs. Then if need be
	 *  send the lhs,opr,rhs off to be evaluated.
	 */

	Character *fvict = nullptr;
	bool ye = false;
	if (arg[0] == '$' && arg[1] == 'v')
	{
		struct tempvariable *eh = mob->tempVariable;
		char buf1[MAX_STRING_LENGTH];
		buf1[0] = '\0';
		int i;
		for (i = 2; arg[i]; i++)
		{
			if (arg[i] == '[')
				continue;
			if (arg[i] == ']')
			{
				buf1[i - 3] = '\0';
				break;
			}
			buf1[i - 3] = arg[i];
		}
		for (; eh; eh = eh->next)
			if (eh->name == buf1)
				break;
		if (eh)
			strcpy(arg, eh->data.toStdString().c_str());
	}

	if (!is_number(arg) && !(arg[0] == '$') && traditional)
	{
		fvict = get_char_room(arg, mob->in_room, true);
		ye = true;
	}
	if (!(arg[0] == '$') && is_number(arg) && traditional)
	{
		Character *te;
		int vnum = atoi(arg);
		for (te = DC::getInstance()->world[mob->in_room].people; te; te = te->next)
		{
			if (IS_PC(te))
				continue;
			if (DC::getInstance()->mob_index[te->mobdata->vnum].virt == vnum)
			{
				fvict = te;
				break;
			}
		}
		ye = true;
	}

	int16_t *lvali = nullptr;
	uint32_t *lvalui = nullptr;
	char **lvalstr = nullptr;
	QString lvalqstr;
	int64_t *lvali64 = nullptr;
	uint64_t *lvalui64 = nullptr;
	int8_t *lvalb = nullptr;
	//  int type = 0;

	if (!traditional)
		translate_value(buf, arg, &lvali, &lvalui, &lvalstr, &lvali64, &lvalui64, &lvalb, mob, actor, obj, vo, rndm, lvalqstr);
	else
		// switch order of traditional so it'd be $n(ispc), to conform with
		// new ifchecks
		translate_value(arg, buf, &lvali, &lvalui, &lvalstr, &lvali64, &lvalui64, &lvalb, mob, actor, obj, vo, rndm, lvalqstr);

	if (val2[0] == '\0')
	{
		if (lvali)
			return mprog_veval(*lvali, opr, atoi(val));
		if (lvalui)
			return mprog_veval(*lvalui, opr, (uint)atoi(val));
		if (lvali64)
			return mprog_veval((int)*lvali64, opr, atoi(val));
		if (lvalui64)
			return mprog_veval((int)*lvalui64, opr, atoi(val));
		if (lvalb)
			return mprog_veval((int)*lvalb, opr, atoi(val));
		if (lvalstr)
			return mob->mprog_seval(*lvalstr, opr, val);
		if (!lvalqstr.isEmpty())
			return mob->mprog_seval(lvalqstr, opr, val);
	}
	else
	{
		int16_t *rvali = nullptr;
		uint32_t *rvalui = nullptr;
		char **rvalstr = nullptr;
		QString rvalqstr;
		int64_t *rvali64 = nullptr;
		uint64_t *rvalui64 = nullptr;
		int8_t *rvalb = nullptr;
		translate_value(val, val2, &rvali, &rvalui, &rvalstr, &rvali64, &rvalui64, &rvalb, mob, actor, obj, vo, rndm, rvalqstr);
		int64_t rval = 0;
		if (rvalstr || rvali || rvalui || rvali64 || rvalui64 || rvalb)
		{
			if (rvalstr && lvalstr)
				return mob->mprog_seval(*lvalstr, opr, *rvalstr);
			// The rest fit in an int64_t, so let's just use that.
			if (rvalstr)
				rval = atoi(*rvalstr);
			if (rvali)
				rval = *rvali;
			if (rvalui)
				rval = *rvalui;
			if (rvalb)
				rval = *rvalb;
			if (rvali64)
				rval = *rvali64;
			if (rvalui64)
				rval = *rvalui64;

			if (lvali)
				return mprog_veval(*lvali, opr, rval);
			if (lvalui)
				return mprog_veval(*lvalui, opr, rval);
			if (lvali64)
				return mprog_veval(*lvali64, opr, rval);
			if (lvalb)
				return mprog_veval((int)*lvalb, opr, rval);
		}
	}

	switch (ifcheck[buf])
	{
	case eRAND:
		return (number(1, 100) <= atoi(arg));
		break;

	case eRAND1K:
		return (number(1, 1000) <= atoi(arg));
		break;

	case eAMTITEMS:
		return mprog_veval(DC::getInstance()->obj_index[atoi(arg)].qty, opr, atoi(val));
		break;

	case eNUMPCS:
	{
		Character *p;
		int count = 0;
		for (p = DC::getInstance()->world[mob->in_room].people; p; p = p->next_in_room)
			if (IS_PC(p))
				count++;
		return mprog_veval(count, opr, atoi(val));
	}
	break;

	case eNUMOFMOBSINWORLD:
	{
		int target = atoi(arg);
		int count = 0;

		const auto &character_list = DC::getInstance()->character_list;
		count = std::count_if(character_list.begin(), character_list.end(),
							  [&target](Character *vch)
							  {
								  if (IS_NPC(vch) && vch->in_room != DC::NOWHERE && DC::getInstance()->mob_index[vch->mobdata->vnum].virt == target)
								  {
									  return true;
								  }
								  else
								  {
									  return false;
								  }
							  });

		return mprog_veval(count, opr, atoi(val));
	}
	break;
	case eNUMOFOBJSINWORLD:
	{
		int target = atoi(arg);
		int count = 0;

		Object *p;
		for (p = DC::getInstance()->object_list; p; p = p->next)
		{
			if (p->vnum == target)
			{
				count++;
			}
		}

		return mprog_veval(count, opr, atoi(val));
	}
	break;

	case eNUMOFMOBSINROOM:
	{
		int target = atoi(arg);
		Character *p;
		int count = 0;
		for (p = DC::getInstance()->world[mob->in_room].people; p; p = p->next_in_room)
			if (IS_NPC(p) && DC::getInstance()->mob_index[p->mobdata->vnum].virt == target)
				count++;

		return mprog_veval(count, opr, atoi(val));
	}
	break;

	case eISPC:
		if (fvict)
			return IS_PC(fvict);
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			return 0;
		case 'z':
			if (mob->beacon)
				return IS_NPC(reinterpret_cast<Character *>(mob->beacon));
			else
				return -1;
		case 'n':
			if (actor)
				return (IS_PC(actor));
			else
				return 0;
		case 't':
			if (vict)
				return (IS_PC(vict));
			else
				return 0;
		case 'r':
			if (rndm)
				return (IS_PC(rndm));
			else
				return 0;
		case 'f':
			if (actor && actor->fighting)
				return (IS_PC(actor->fighting));
			else
				return 0;
		case 'g':
			if (mob && mob->fighting)
				return (IS_PC(mob->fighting));
			else
				return 0;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'ispc'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		break;

	case eISWIELDING:
		if (fvict)
			return fvict->equipment[WIELD] ? 1 : 0;
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			return (mob->equipment[WIELD]) ? 1 : 0;
		case 'z':
			if (mob->beacon)
				return ((Character *)mob->beacon)->equipment[WIELD] ? 1 : 0;
			else
				return -1;

		case 'n':
			if (actor)
				return (actor->equipment[WIELD]) ? 1 : 0;
			else
				return 0;
		case 't':
			if (vict)
				return (vict->equipment[WIELD]) ? 1 : 0;
			else
				return 0;
		case 'r':
			if (rndm)
				return (rndm->equipment[WIELD]) ? 1 : 0;
			else
				return 0;
		case 'f':
			if (actor && actor->fighting)
				return (actor->fighting->equipment[WIELD]) ? 1 : 0;
			else
				return 0;
		case 'g':
			if (mob && mob->fighting)
				return (mob->fighting->equipment[WIELD]) ? 1 : 0;
			else
				return 0;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'iswielding'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		break;

	case eISWEAPPRI:
		if (fvict && fvict->equipment[WIELD])
			return mprog_veval(fvict->equipment[WIELD]->obj_flags.value[3], opr, atoi(val));
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			if (mob->equipment[WIELD])
				return mprog_veval(mob->equipment[WIELD]->obj_flags.value[3], opr, atoi(val));
			else
				return 0;
		case 'z':
			if (mob->beacon && ((Character *)mob->beacon)->equipment[WIELD])
				return mprog_veval(((Character *)mob->beacon)->equipment[WIELD]->obj_flags.value[3], opr, atoi(val));
			else
				return -1;
		case 'n':
			if (actor && actor->equipment[WIELD])
				return mprog_veval(actor->equipment[WIELD]->obj_flags.value[3], opr, atoi(val));
			else
				return 0;
		case 't':
			if (vict && vict->equipment[WIELD])
				return mprog_veval(vict->equipment[WIELD]->obj_flags.value[3], opr, atoi(val));
			else
				return 0;
		case 'r':
			if (rndm && rndm->equipment[WIELD])
				return mprog_veval(rndm->equipment[WIELD]->obj_flags.value[3], opr, atoi(val));
			else
				return 0;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isweappri'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		break;

	case eISWEAPSEC:
		if (fvict && fvict->equipment[SECOND_WIELD])
			return mprog_veval(fvict->equipment[SECOND_WIELD]->obj_flags.value[3], opr, atoi(val));
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			if (mob->equipment[SECOND_WIELD])
				return mprog_veval(mob->equipment[SECOND_WIELD]->obj_flags.value[3], opr, atoi(val));
			else
				return 0;
		case 'z':
			if (mob->beacon && ((Character *)mob->beacon)->equipment[SECOND_WIELD])
				return mprog_veval(((Character *)mob->beacon)->equipment[SECOND_WIELD]->obj_flags.value[3], opr, atoi(val));
			else
				return -1;
		case 'n':
			if (actor && actor->equipment[SECOND_WIELD])
				return mprog_veval(actor->equipment[SECOND_WIELD]->obj_flags.value[3], opr, atoi(val));
			else
				return 0;
		case 't':
			if (vict && vict->equipment[SECOND_WIELD])
				return mprog_veval(vict->equipment[SECOND_WIELD]->obj_flags.value[3], opr, atoi(val));
			else
				return 0;
		case 'r':
			if (rndm && rndm->equipment[SECOND_WIELD])
				return mprog_veval(rndm->equipment[SECOND_WIELD]->obj_flags.value[3], opr, atoi(val));
			else
				return 0;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isweapsec'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		break;

	case eISNPC:
		if (fvict)
			return IS_NPC(fvict);
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			return true;
		case 'z':
			if (mob->beacon)
				return IS_NPC(((Character *)mob->beacon));
			else
				return false;

		case 'n':
			if (actor)
				return IS_NPC(actor);
			else
				return false;
		case 't':
			if (vict)
				return IS_NPC(vict);
			else
				return false;
		case 'r':
			if (rndm)
				return IS_NPC(rndm);
			else
				return false;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isnpc(): '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
			return false;
		}
		break;

	case eISGOOD:
		if (fvict)
			return IS_GOOD(fvict);
		if (ye)
			return false;

		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			return IS_GOOD(mob);
		case 'z':
			if (mob->beacon)
				return IS_GOOD(((Character *)mob->beacon));
			else
				return -1;
		case 'n':
			if (actor)
				return IS_GOOD(actor);
			else
				return -1;
		case 't':
			if (vict)
				return IS_GOOD(vict);
			else
				return -1;
		case 'r':
			if (rndm)
				return IS_GOOD(rndm);
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isgood(): '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
			return -1;
		}
		break;

	case eISNEUTRAL:
		if (fvict)
			return IS_NEUTRAL(fvict);
		if (ye)
			return false;

		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			return IS_NEUTRAL(mob);
		case 'z':
			if (mob->beacon)
				return IS_NEUTRAL(((Character *)mob->beacon));
			else
				return -1;
		case 'n':
			if (actor)
				return IS_NEUTRAL(actor);
			else
				return -1;
		case 't':
			if (vict)
				return IS_NEUTRAL(vict);
			else
				return -1;
		case 'r':
			if (rndm)
				return IS_NEUTRAL(rndm);
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isgood(): '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
			return -1;
		}
		break;

	case eISEVIL:
		if (fvict)
			return IS_EVIL(fvict);
		if (ye)
			return false;

		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			return IS_EVIL(mob);
		case 'z':
			if (mob->beacon)
				return IS_EVIL(((Character *)mob->beacon));
			else
				return -1;
		case 'n':
			if (actor)
				return IS_EVIL(actor);
			else
				return -1;
		case 't':
			if (vict)
				return IS_EVIL(vict);
			else
				return -1;
		case 'r':
			if (rndm)
				return IS_EVIL(rndm);
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isgood(): '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
			return -1;
		}
		break;

	case eISWORN:
	{
		Object *o = nullptr;
		if (mob->mobdata->isObject())
		{
			o = mob->mobdata->getObject();
		}
		if (fvict)
			return is_wearing(fvict, o);
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'z':
			if (mob->beacon)
				return is_wearing(((Character *)mob->beacon), o);
			else
				return -1;
		case 'i':
			return -1;
		case 'n':
			if (actor)
				return is_wearing(actor, o);
			else
				return -1;
		case 't':
			if (vict)
				return is_wearing(actor, o);
			else
				return -1;
		case 'r':
			if (rndm)
				return is_wearing(rndm, o);
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isword(): '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
			return -1;
		}
	}
	break;

	case eISFIGHT:
		if (fvict)
			return fvict->fighting ? 1 : 0;
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'z':
			if (mob->beacon)
				return ((Character *)mob->beacon)->fighting ? 1 : 0;
			else
				return -1;

		case 'i':
			return (mob->fighting) ? 1 : 0;
		case 'n':
			if (actor)
				return (actor->fighting) ? 1 : 0;
			else
				return -1;
		case 't':
			if (vict)
				return (vict->fighting) ? 1 : 0;
			else
				return -1;
		case 'r':
			if (rndm)
				return (rndm->fighting) ? 1 : 0;
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isfight(): '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
			return -1;
		}
		break;

	case eISTANK:
		if (fvict)
			return istank(fvict);
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'z':
			if (mob->beacon)
				return istank(((Character *)mob->beacon));
			else
				return -1;

		case 'i':
			return istank(mob);
		case 'n':
			if (actor)
				return istank(actor);
			else
				return -1;
		case 't':
			if (vict)
				return istank(vict) ? 1 : 0;
			else
				return -1;
		case 'r':
			if (rndm)
				return istank(rndm) ? 1 : 0;
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to istank(): '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
			return -1;
		}
		break;

	case eISIMMORT:
		if (fvict)
			return fvict->getLevel() > IMMORTAL;
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			return (mob->getLevel() > IMMORTAL);
		case 'z':
			if (mob->beacon)
				return ((Character *)mob->beacon)->getLevel() > IMMORTAL;
			else
				return -1;

		case 'n':
			if (actor)
				return (actor->getLevel() > IMMORTAL);
			else
				return -1;
		case 't':
			if (vict)
				return (vict->getLevel() > IMMORTAL);
			else
				return -1;
		case 'r':
			if (rndm)
				return (rndm->getLevel() > IMMORTAL);
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isimmort(): '%s'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, mob->mobdata->vnum, ifchck);
			return -1;
		}
		break;

	case eISCHARMED:
		if (fvict)
			return IS_AFFECTED(fvict, AFF_CHARM);
		if (ye)
			return false;

		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			return IS_AFFECTED(mob, AFF_CHARM);
		case 'z':
			if (mob->beacon)
				return IS_AFFECTED(((Character *)mob->beacon), AFF_CHARM);
			else
				return -1;

		case 'n':
			if (actor)
				return IS_AFFECTED(actor, AFF_CHARM);
			else
				return -1;
		case 't':
			if (vict)
				return IS_AFFECTED(vict, AFF_CHARM);
			else
				return -1;
		case 'r':
			if (rndm)
				return IS_AFFECTED(rndm, AFF_CHARM);
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'ischarmed'",
				 DC::getInstance()->mob_index[mob->mobdata->vnum].virt);

			return -1;
		}
		break;

	case eISFOLLOW:
		if (fvict)
			return (fvict->master != nullptr && fvict->master->in_room == fvict->in_room);
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			return (mob->master != nullptr && mob->master->in_room == mob->in_room);
		case 'z':
			if (mob->beacon)
				return ((Character *)mob->beacon)->master && ((Character *)mob->beacon)->master->in_room == ((Character *)mob->beacon)->in_room;
			else
				return -1;
		case 'n':
			if (actor)
				return (actor->master != nullptr && actor->master->in_room == actor->in_room);
			else
				return -1;
		case 't':
			if (vict)
				return (vict->master != nullptr && vict->master->in_room == vict->in_room);
			else
				return -1;
		case 'r':
			if (rndm)
				return (rndm->master != nullptr && rndm->master->in_room == rndm->in_room);
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isfollow'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		break;

	case eISSPELLED:
	{
		int find_skill_num(char *name);

		if (!str_cmp(val, "fly")) // needs special check.. sigh..
		{
			if (fvict && IS_AFFECTED(fvict, AFF_FLYING))
				return true;
			if (ye)
				return false;
			switch (arg[1])
			{
			case 'i': // mob
				if (IS_AFFECTED(mob, AFF_FLYING))
					return true;
				break;
			case 'z':
				if (mob->beacon)
					if (IS_AFFECTED(((Character *)mob->beacon), AFF_FLYING))
						return true;
				break;
			case 'n': // actor
				if (actor)
					if (IS_AFFECTED(actor, AFF_FLYING))
						return true;
				break;
			case 't': // vict
				if (vict)
					if (IS_AFFECTED(vict, AFF_FLYING))
						return true;
				break;
			case 'r': // rand
				if (rndm)
					if (IS_AFFECTED(rndm, AFF_FLYING))
						return true;
				break;
			case 'f':
				if (actor && actor->fighting)
					if (IS_AFFECTED(actor->fighting, AFF_FLYING))
						return true;
				break;
			case 'g':
				if (mob && mob->fighting)
					if (IS_AFFECTED(mob->fighting, AFF_FLYING))
						return true;
				break;
			default:
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isspelled'",
					 DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
				return -1;
			}
		}

		if (fvict)
			return (int64_t)(fvict->affected_by_spell(find_skill_num(val)));
		if (ye)
			return false;
		switch (arg[1])
		{
		case 'i': // mob
			return (int64_t)(mob->affected_by_spell(find_skill_num(val)));
		case 'z':
			if (mob->beacon)
				return (int64_t)((Character *)mob->beacon)->affected_by_spell((find_skill_num(val)));
			else
				return -1;

		case 'n': // actor
			if (actor)
				return (int64_t)(actor->affected_by_spell(find_skill_num(val)));
			else
				return -1;
		case 't': // vict
			if (vict)
				return (int64_t)(vict->affected_by_spell(find_skill_num(val)));
			else
				return -1;
		case 'r': // rand
			if (rndm)
				return (int64_t)(rndm->affected_by_spell(find_skill_num(val)));
			return -1;
		case 'f':
			if (actor && actor->fighting)
				return (int64_t)(actor->fighting->affected_by_spell(find_skill_num(val)));
			else
				return -1;
		case 'g':
			if (mob && mob->fighting)
				return (int64_t)(mob->fighting->affected_by_spell(find_skill_num(val)));
			else
				return 0;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isspelled'",
				 DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
	}
	break;

	case eISAFFECTED:
		if (fvict)
			return (ISSET(fvict->affected_by, atoi(val)));
		if (ye)
			return false;

		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			return (ISSET(mob->affected_by, atoi(val)));
		case 'z':
			if (mob->beacon)
				return (ISSET(((Character *)mob->beacon)->affected_by, atoi(val)));
			else
				return -1;

		case 'n':
			if (actor)
				return (ISSET(actor->affected_by, atoi(val)));
			else
				return -1;
		case 't':
			if (vict)
				return (ISSET(vict->affected_by, atoi(val)));
			else
				return -1;
		case 'r':
			if (rndm)
				return (ISSET(rndm->affected_by, atoi(val)));
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isaffected'",
				 DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		break;

	case eHITPRCNT:
	{
		int lhsvl, rhsvl;
		if (fvict)
		{
			lhsvl = (fvict->hit * 100) / fvict->max_hit;
			rhsvl = atoi(val);
			return mprog_veval(lhsvl, opr, rhsvl);
		}
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			lhsvl = (mob->hit * 100) / mob->max_hit;
			rhsvl = atoi(val);
			return mprog_veval(lhsvl, opr, rhsvl);
		case 'z':
			if (mob->beacon)
			{
				lhsvl = (((Character *)mob->beacon)->hit * 100) / ((Character *)mob->beacon)->max_hit;
				rhsvl = atoi(val);
				return mprog_veval(lhsvl, opr, rhsvl);
			}
			else
				return -1;

		case 'n':
			if (actor)
			{
				lhsvl = (actor->hit * 100) / actor->max_hit;
				rhsvl = atoi(val);
				return mprog_veval(lhsvl, opr, rhsvl);
			}
			else
				return -1;
		case 't':
			if (vict)
			{
				lhsvl = (vict->hit * 100) / vict->max_hit;
				rhsvl = atoi(val);
				return mprog_veval(lhsvl, opr, rhsvl);
			}
			else
				return -1;
		case 'r':
			if (rndm)
			{
				lhsvl = (rndm->hit * 100) / rndm->max_hit;
				rhsvl = atoi(val);
				return mprog_veval(lhsvl, opr, rhsvl);
			}
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'hitprcnt'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
	}
	break;

	case eWEARS:
	{
		class Object *obj = 0;
		Character *take;
		char bufeh[MAX_STRING_LENGTH];
		char *valu = one_argument(val, bufeh);

		if (fvict)
		{
			obj = search_char_for_item(fvict, atoi(valu), true);
			take = fvict;
		}
		else
		{
			if (ye)
				return false;
			switch (arg[1])
			{
			case 'z':
				if (!mob->beacon)
					return -1;
				obj = search_char_for_item(((Character *)mob->beacon), atoi(valu), true);
				take = ((Character *)mob->beacon);
			case 'i': // mob
				obj = search_char_for_item(mob, atoi(valu), true);
				take = mob;
				break;
			case 'n': // actor
				if (!actor)
					return -1;
				obj = search_char_for_item(actor, atoi(valu), true);
				take = actor;
				break;
			case 't': // vict
				if (!vict)
					return -1;
				obj = search_char_for_item(vict, atoi(valu), true);
				take = vict;
				break;
			case 'r': // rndm
				if (!rndm)
					return -1;
				obj = search_char_for_item(rndm, atoi(valu), true);
				take = rndm;
				break;
			default:
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'carries'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
				return -1;
			}
		}
		if (!obj)
			return 0;
		if (!str_cmp(bufeh, "keep"))
			return 1;
		else if (!str_cmp(bufeh, "take"))
		{
			int i;
			if (obj->carried_by)
				obj_from_char(obj);
			for (i = 0; i < MAX_WEAR; i++)
				if (obj == take->equipment[i])
				{
					obj_from_char(unequip_char(take, i));
				}
			extract_obj(obj);
			return 1;
		}
		return -1;
	}
	break;

	case eCARRIES:
	{
		class Object *obj = 0;
		Character *take;
		char bufeh[MAX_STRING_LENGTH];
		char *valu = one_argument(val, bufeh);
		if (fvict)
		{
			obj = search_char_for_item(fvict, atoi(valu), false);
			take = fvict;
		}
		else
		{
			if (ye)
				return false;
			switch (arg[1])
			{
			case 'z':
				if (!mob->beacon)
					return -1;
				obj = search_char_for_item(((Character *)mob->beacon), atoi(valu), false);
				take = ((Character *)mob->beacon);
			case 'i': // mob
				obj = search_char_for_item(mob, atoi(valu), false);
				take = mob;
				break;
			case 'n': // actor
				if (!actor)
					return -1;
				obj = search_char_for_item(actor, atoi(valu), false);
				take = actor;
				break;
			case 't': // vict
				if (!vict)
					return -1;
				obj = search_char_for_item(vict, atoi(valu), false);
				take = vict;
				break;
			case 'r': // rndm
				if (!rndm)
					return -1;
				obj = search_char_for_item(rndm, atoi(valu), false);
				take = rndm;
				break;
			default:
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'carries'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
				return -1;
			}
		}
		if (!obj)
			return 0;
		if (!str_cmp(bufeh, "keep"))
			return 1;
		else if (!str_cmp(bufeh, "take"))
		{
			int i;
			if (obj->carried_by)
				obj_from_char(obj);
			for (i = 0; i < MAX_WEAR; i++)
				if (obj == take->equipment[i])
				{
					obj_from_char(unequip_char(take, i));
				}
			extract_obj(obj);
			return 1;
		}
		return -1;
	}
	break;

	case eNUMBER:
	{
		int lhsvl, rhsvl;
		if (fvict)
		{
			if (IS_PC(fvict))
				return 0;
			lhsvl = DC::getInstance()->mob_index[fvict->mobdata->vnum].virt;
			rhsvl = atoi(val);
			return mprog_veval(lhsvl, opr, rhsvl);
		}
		if (ye)
			return false;

		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'i':
			lhsvl = DC::getInstance()->mob_index[mob->mobdata->vnum].virt;
			rhsvl = atoi(val);
			return mprog_veval(lhsvl, opr, rhsvl);
		case 'z':
			if (mob->beacon)
			{
				if (IS_NPC(((Character *)mob->beacon)))
				{
					lhsvl = DC::getInstance()->mob_index[((Character *)mob->beacon)->mobdata->vnum].virt;
					rhsvl = atoi(val);
					return mprog_veval(lhsvl, opr, rhsvl);
				}
				else
					return 0;
			}
			else
				return 0;

		case 'n':
			if (actor)
			{
				if IS_NPC (actor)
				{
					lhsvl = DC::getInstance()->mob_index[actor->mobdata->vnum].virt;
					rhsvl = atoi(val);
					return mprog_veval(lhsvl, opr, rhsvl);
				}
			}
			else
				return 0;
		case 't':
			if (vict)
			{
				if IS_NPC (actor)
				{
					lhsvl = DC::getInstance()->mob_index[vict->mobdata->vnum].virt;
					rhsvl = atoi(val);
					return mprog_veval(lhsvl, opr, rhsvl);
				}
			}
			else
				return 0;
		case 'r':
			if (rndm)
			{
				if IS_NPC (actor)
				{
					lhsvl = DC::getInstance()->mob_index[rndm->mobdata->vnum].virt;
					rhsvl = atoi(val);
					return mprog_veval(lhsvl, opr, rhsvl);
				}
			}
			else
				return 0;
		case 'o':
			if (obj)
			{
				lhsvl = obj->vnum;
				rhsvl = atoi(val);
				return mprog_veval(lhsvl, opr, rhsvl);
			}
			else
				return -1;
		case 'p':
			if (v_obj)
			{
				lhsvl = v_obj->vnum;
				rhsvl = atoi(val);
				return mprog_veval(lhsvl, opr, rhsvl);
			}
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'number'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
	}
	break;

	case eTEMPVAR:
	{
		char buf4[MAX_STRING_LENGTH], *buf4pt;
		if (arg[2] != '[')
		{
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d badtarget  to 'tempvar'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		buf4pt = &arg[3];
		strcpy(buf4, buf4pt);
		buf4pt = &buf4[0];
		while (*buf4pt != ']' && *buf4pt != '\0')
			buf4pt++;
		if (*buf4pt == '\0')
		{
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad target to 'tempvar'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		*buf4pt = '\0';
		if (val[0] == '$' && val[1])
			mprog_translate(val[1], val, mob, actor, obj, vo, rndm);

		if (fvict)
			return mob->mprog_seval(fvict->getTemp(buf4), opr, val);
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'z':
			if (mob->beacon)
			{
				return mob->mprog_seval(((Character *)mob->beacon)->getTemp(buf4), opr, val);
			}
			else
				return -1;
		case 'i':
			return mob->mprog_seval(mob->getTemp(buf4), opr, val);
		case 'n':
			if (actor)
				return mob->mprog_seval(actor->getTemp(buf4), opr, val);
			else
				return -1;
		case 't':
			if (vict)
				return mob->mprog_seval(vict->getTemp(buf4), opr, val);
			else
				return -1;
		case 'r':
			if (rndm)
				return mob->mprog_seval(rndm->getTemp(buf4), opr, val);
			else
				return -1;
		case 'o':
			return -1;
		case 'p':
			return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'tempvar'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
	}
	break;

	// search a room for a mob with vnum arg
	case eISMOBVNUMINROOM:
	{
		int target = atoi(arg);

		for (Character *vch = DC::getInstance()->world[mob->in_room].people;
			 vch;
			 vch = vch->next_in_room)
		{
			if (IS_NPC(vch) &&
				vch != mob &&
				DC::getInstance()->mob_index[vch->mobdata->vnum].virt == target)
				return 1;
		}
		return 0;
	}
	break;

	// search a room for a obj with vnum arg
	case eISOBJVNUMINROOM:
	{
		int target = atoi(arg);

		for (Object *obj = DC::getInstance()->world[mob->in_room].contents;
			 obj;
			 obj = obj->next_content)
		{
			if (obj->vnum == target)
				return 1;
		}
		return 0;
	}
	break;

	case eCANSEE:
		if (fvict)
			return CAN_SEE(mob, fvict, true);
		if (ye)
			return false;
		switch (arg[1]) /* arg should be "$*" so just get the letter */
		{
		case 'z':
			return 1; // can always see holder
		case 'i':
			return 1;
		case 'n':
			if (actor)
				return CAN_SEE(mob, actor, true);
			else
				return -1;
		case 't':
			if (vict)
				return CAN_SEE(mob, vict, true);
			else
				return -1;
		case 'r':
			if (rndm)
				return CAN_SEE(mob, rndm, true);
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isnpc'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		break;

	case eINSAMEZONE:
		if (fvict)
			return (GET_ZONE(fvict) == GET_ZONE(mob));
		else if ((fvict = get_pc(arg)) != nullptr)
			return (GET_ZONE(fvict) == GET_ZONE(mob));

		switch (arg[1])
		{
		case 'i':
			return 1; // always in the same zone as itself
		case 'n':
			if (actor)
				return (GET_ZONE(actor) == GET_ZONE(mob));
			else
				return -1;
		case 'r':
		case 't':
			if (vict)
				return (GET_ZONE(vict) == GET_ZONE(mob));
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'insamezone'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		break;

	case eCLAN:
		if (fvict)
			return fvict->clan;
		switch (arg[1])
		{
		case 'i':
			return mob->clan; // always in the same zone as itself
		case 'n':
			if (actor)
				return actor->clan;
			else
				return -1;
		case 'r':
		case 't':
			if (vict)
				return vict->clan;
			else
				return -1;
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'clan'", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return -1;
		}
		break;

	case eISDAYTIME:
		if (weather_info.sunlight == SUN_DARK)
			return false;
		return true;
		break;

	case eISRAINING:
		if (weather_info.sky == SKY_LIGHTNING || weather_info.sky == SKY_RAINING || weather_info.sky == SKY_HEAVY_RAIN)
			return true;
		return false;
		break;

	default:
		/* Ok... all the ifchcks are done, so if we didnt find ours then something
		 * odd happened.  So report the bug and abort the MOBprogram (return error)
		 */
		logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d unknown ifchck  \"%s\" value %d",
			 DC::getInstance()->mob_index[mob->mobdata->vnum].virt, buf, ifcheck[buf]);
		return -1;
	}
}
/* Quite a long and arduous function, this guy handles the control
 * flow part of MOBprograms.  Basicially once the driver sees an
 * 'if' attention shifts to here.  While many syntax errors are
 * caught, some will still get through due to the handling of break
 * and errors in the same fashion.  The desire to break out of the
 * recursion without catastrophe in the event of a mis-parse was
 * believed to be high. Thus, if an error is found, it is bugged and
 * the parser acts as though a break were issued and just bails out
 * at that point. I havent tested all the possibilites, so I'm speaking
 * in theory, but it is 'guaranteed' to work on syntactically correct
 * MOBprograms, so if the mud crashes here, check the mob carefully!
 */
// Null is kept globally so it doesn't go out of scope when we return in
// -pir 11/18/01
char null[1];
// Mprog_cur_result holds the current status of the mob that is acting.
// It will hold eCH_DIED if the mob died doing it's proc.  We need to
// make sure this is not true when returning from an if check or the
// mob's pointer is no longer valid
int mprog_cur_result;
#define DIFF(a, b) ((a - b) > 0 ? (a - b) : (b - a))

char *mprog_process_if(char *ifchck, char *com_list, Character *mob,
					   Character *actor, Object *obj, void *vo,
					   Character *rndm, struct mprog_throw_type *thrw)
{

	char buf[MAX_INPUT_LENGTH];
	char *morebuf = 0;
	char *cmnd = 0;
	bool loopdone = false;
	bool flag = false;
	int legal;

	*null = '\0';

	Character *ur = nullptr;
	if (ur)
		ur->sendln("\r\nProg initiated.");

	if (!thrw || DIFF(ifchck, activeProgTmpBuf) >= thrw->startPos)
	{
		/* check for trueness of the ifcheck */
		if ((cIfs[ifpos++] = legal = mprog_do_ifchck(ifchck, mob, actor, obj, vo, rndm)))
		{
			if (legal >= 1)
				flag = true;
			else
				return null;
		}
		if (ur)
			csendf(ur, "%d>%d\r\n", ifpos - 1, legal);
	}
	else
	{
		legal = thrw->ifchecks[thrw->cPos++];
		cIfs[ifpos++] = legal;
		if (legal >= 1)
			flag = true;
		else if (legal < 0)
			return nullptr;
		if (ur)
			csendf(ur, "%d>-%d\r\n", thrw->cPos - 1, legal);
	}

	while (loopdone == false) /*scan over any existing or statements */
	{
		cmnd = com_list;
		activePos = com_list = mprog_next_command(com_list);
		while (*cmnd == ' ')
			cmnd++;
		if (*cmnd == '\0')
		{
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d no commands after IF/OR", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return null;
		}
		morebuf = one_argument(cmnd, buf);
		if (!str_cmp(buf, "or"))
		{

			if (!thrw || DIFF(morebuf, activeProgTmpBuf) >= thrw->startPos)
			{
				if ((cIfs[ifpos++] = legal = mprog_do_ifchck(morebuf, mob, actor, obj, vo, rndm)))
				{
					if (legal == 1)
						flag = true;
					else
						return null;
				}
				if (ur)
					csendf(ur, "%d<%d\r\n", ifpos - 1, legal);
			}
			else
			{
				legal = thrw->ifchecks[thrw->cPos++];
				cIfs[ifpos++] = legal;
				if (legal == 1)
					flag = true;
				else if (legal < 0)
					return nullptr;
				if (ur)
					csendf(ur, "%d<-%d\r\n", thrw->cPos - 1, legal);
			}
		}
		else
			loopdone = true;
	}

	if (flag)
		for (;;) /*ifcheck was true, do commands but ignore else to endif*/
		{
			if (!str_cmp(buf, "if"))
			{
				com_list = mprog_process_if(morebuf, com_list, mob, actor, obj, vo, rndm, thrw);
				if (isSet(mprog_cur_result, eCH_DIED))
					return null;
				while (*cmnd == ' ')
					cmnd++;
				if (*com_list == '\0')
					return null;
				cmnd = com_list;
				activePos = com_list = mprog_next_command(com_list);
				morebuf = one_argument(cmnd, buf);
				continue;
			}
			if (!str_cmp(buf, "break"))
				return null;
			if (!str_cmp(buf, "endif"))
				return com_list;
			if (!str_cmp(buf, "else"))
			{
				int nest = 0;
				while (str_cmp(buf, "endif") || nest > 0)
				{
					if (!str_cmp(buf, "if"))
						nest++;
					if (!str_cmp(buf, "endif"))
						nest--;
					cmnd = com_list;
					activePos = com_list = mprog_next_command(com_list);
					while (*cmnd == ' ')
						cmnd++;
					if (*cmnd == '\0')
					{
						logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d missing endif after else",
							 DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
						return null;
					}
					morebuf = one_argument(cmnd, buf);
				}
				return com_list;
			}

			if (!thrw || DIFF(cmnd, activeProgTmpBuf) >= thrw->startPos)
			{

#ifdef DEBUG_MPROG
				if (mob && mob->mobdata && DC::getInstance()->mob_index[mob->mobdata->nr].virt == 4821)
				{
					qDebug("debug: ");
					if (cmnd)
						qDebug("%s",qUtf8Printable(QStringLiteral("cmd: %1 ").arg(cmnd));
					else
						qDebug("cmd: (null) ");

					if (mob && mob->name)
						qDebug("%s",qUtf8Printable(QStringLiteral("mob: %1 ").arg(mob->name)));
					else
						qDebug("mob: (null) ");

					if (actor && actor->name)
						qDebug("%s",qUtf8Printable(QStringLiteral("actor: %1 = ").arg(actor->name)));
					else
						qDebug("actor: (null) = ");
				}
#endif
				SET_BIT(mprog_cur_result, mprog_process_cmnd(cmnd, mob, actor, obj, vo, rndm));

#ifdef DEBUG_MPROG
				if (mob && mob->mobdata && DC::getInstance()->mob_index[mob->mobdata->nr].virt == 4821)
				{
					if (isSet(mprog_cur_result, eFAILURE))
						qDebug("eFAILURE ");
					if (isSet(mprog_cur_result, eSUCCESS))
						qDebug("eSUCCESS ");
					if (isSet(mprog_cur_result, eCH_DIED))
						qDebug("eCH_DIED ");
					if (isSet(mprog_cur_result, eVICT_DIED))
						qDebug("eVICT_DIED ");
					if (isSet(mprog_cur_result, eINTERNAL_ERROR))
						qDebug("eINTERNAL_ERROR ");
					if (isSet(mprog_cur_result, eEXTRA_VALUE))
						qDebug("eEXTRA_VALUE ");
					if (isSet(mprog_cur_result, eEXTRA_VAL2))
						qDebug("eEXTRA_VAL2 ");
					if (isSet(mprog_cur_result, eDELAYED_EXEC))
						qDebug("eDELAYED_EXEC ");

					qDebug("\n");
				}
#endif
				if (isSet(mprog_cur_result, eCH_DIED) ||
					isSet(mprog_cur_result, eDELAYED_EXEC) ||
					isSet(mprog_cur_result, eVICT_DIED))
					return null;
			}
			cmnd = com_list;
			activePos = com_list = mprog_next_command(com_list);
			while (*cmnd == ' ')
				cmnd++;
			if (*cmnd == '\0')
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d missing else or endif", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
				return null;
			}
			morebuf = one_argument(cmnd, buf);
		}
	else /*false ifcheck, find else and do existing commands or quit at endif*/
	{
		int nest = 0;
		while (true)
		{ // Fix here 13/4 2004. Nested ifs are now taken into account.
			if (!str_cmp(buf, "if"))
				nest++;
			if (!str_cmp(buf, "endif"))
			{
				if (nest == 0)
					break;
				else
					nest--;
			}
			if (!nest && !str_cmp(buf, "else"))
				break;

			cmnd = com_list;
			activePos = com_list = mprog_next_command(com_list);
			while (*cmnd == ' ')
				cmnd++;
			if (*cmnd == '\0')
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d missing an else or endif",
					 DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
				return null;
			}
			morebuf = one_argument(cmnd, buf);
		}

		/* found either an else or an endif.. act accordingly */
		if (!str_cmp(buf, "endif"))
			return com_list;
		cmnd = com_list;
		activePos = com_list = mprog_next_command(com_list);
		while (*cmnd == ' ')
			cmnd++;
		if (*cmnd == '\0')
		{
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d missing endif", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
			return null;
		}
		morebuf = one_argument(cmnd, buf);

		for (;;) /*process the post-else commands until an endif is found.*/
		{
			if (!str_cmp(buf, "if"))
			{
				com_list = mprog_process_if(morebuf, com_list, mob, actor,
											obj, vo, rndm, thrw);
				if (isSet(mprog_cur_result, eCH_DIED))
					return null;
				while (*cmnd == ' ')
					cmnd++;
				if (*com_list == '\0')
					return null;
				cmnd = com_list;
				activePos = com_list = mprog_next_command(com_list);
				morebuf = one_argument(cmnd, buf);
				continue;
			}
			if (!str_cmp(buf, "else"))
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d found else in an else section",
					 DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
				return null;
			}
			if (!str_cmp(buf, "break"))
				return null;
			if (!str_cmp(buf, "endif"))
				return com_list;

			if (!thrw || DIFF(cmnd, activeProgTmpBuf) >= thrw->startPos)
			{
				SET_BIT(mprog_cur_result, mprog_process_cmnd(cmnd, mob, actor, obj, vo, rndm));
				if (isSet(mprog_cur_result, eCH_DIED) || isSet(mprog_cur_result, eDELAYED_EXEC))
					return null;
			}
			cmnd = com_list;
			activePos = com_list = mprog_next_command(com_list);
			while (*cmnd == ' ')
				cmnd++;
			if (*cmnd == '\0')
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d missing endif in else section",
					 DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
				return null;
			}
			morebuf = one_argument(cmnd, buf);
		}
	}

	return nullptr;
}

/* This routine handles the variables for command expansion.
 * If you want to add any go right ahead, it should be fairly
 * clear how it is done and they are quite easy to do, so you
 * can be as creative as you want. The only catch is to check
 * that your variables exist before you use them. At the moment,
 * using $t when the secondary target refers to an object
 * i.e. >prog_act drops~<nl>if ispc($t)<nl>sigh<nl>endif<nl>~<nl>
 * probably makes the mud crash (vice versa as well) The cure
 * would be to change act() so that vo becomes vict & v_obj.
 * but this would require a lot of small changes all over the code.
 */
void mprog_translate(char ch, char *t, Character *mob, Character *actor,
					 Object *obj, void *vo, Character *rndm)
{
	static char *he_she[] = {"it", "he", "she"};
	static char *him_her[] = {"it", "him", "her"};
	static char *his_her[] = {"its", "his", "her"};
	Character *vict = (Character *)vo;
	Object *v_obj = (Object *)vo;

	*t = '\0';

	auto q = mob->getName();
	auto s = q.toStdString();
	auto mob_nameC = s.c_str();

	switch (ch)
	{
	case 'i':
		one_argument(mob_nameC, t);
		break;
	case 'z':
		if (mob->beacon)
		{
			one_argument(((Character *)mob->beacon)->getNameC(), t);
			break;
		}
		strcpy(t, "error");
		break;
	case 'Z':
		if (mob->beacon)
			strcpy(t, ((Character *)mob->beacon)->short_desc);
		else
			strcpy(t, "error");
		break;
	case 'I':
		strcpy(t, mob->short_desc);
		break;
	case 'x':
		if (mob->beacon && ((Character *)mob->beacon)->fighting)
			one_argument(((Character *)mob->beacon)->fighting->getNameC(), t);
		else
			strcpy(t, "error");
		*t = UPPER(*t);
		break;
	case 'n':
		if (actor)
		{
			// Mobs can see them no matter what.  Use "cansee()" if you don't want that
			//	   if ( CAN_SEE( mob,actor ) )
			one_argument(actor->getNameC(), t);
			if (IS_PC(actor))
				*t = UPPER(*t);
		}
		else
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d trying illegal $ n in MOBProg.", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
		break;

	case 'N':
		if (actor)
			if (CAN_SEE(mob, actor))
				if (IS_NPC(actor))
					strcpy(t, actor->short_desc);
				else
				{
					strcpy(t, actor->getNameC());
					strcat(t, " ");
					strcat(t, actor->title);
				}
			else
				strcpy(t, "someone");
		else
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d trying illegal $ N in MOBProg.", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
		break;

	case 't':
		if (vict)
		{
			if (CAN_SEE(mob, vict))
				one_argument(vict->getNameC(), t);
			if (IS_PC(vict))
				*t = UPPER(*t);
		}
		else
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d trying illegal $ t in MOBProg.", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
		break;

	case 'T':
		if (vict)
			if (CAN_SEE(mob, vict))
				if (IS_NPC(vict))
					strcpy(t, vict->short_desc);
				else
				{
					strcpy(t, vict->getNameC());
					strcat(t, " ");
					strcat(t, vict->title);
				}
			else
				strcpy(t, "someone");
		else
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d trying illegal $T in MOBProg.", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
		break;

	case 'f':
		if (actor && actor->fighting)
		{
			if (CAN_SEE(mob, actor->fighting))
				one_argument(actor->fighting->getNameC(), t);
			if (IS_PC(actor->fighting))
				*t = UPPER(*t);
		}
		break;
	case 'F':
		if (actor && actor->fighting)
		{
			if (CAN_SEE(mob, actor->fighting))
			{
				if (IS_NPC(actor->fighting))
					strcpy(t, actor->fighting->short_desc);
				else
				{
					strcpy(t, actor->fighting->getNameC());
					strcat(t, " ");
					strcat(t, actor->fighting->title);
				}
			}
		}
		break;
	case 'g':
		if (mob && mob->fighting)
		{
			if (CAN_SEE(mob, mob->fighting))
				one_argument(mob->fighting->getNameC(), t);
			if (IS_PC(mob->fighting))
				*t = UPPER(*t);
		}
		break;
	case 'G':
		if (mob && mob->fighting)
		{
			if (CAN_SEE(mob, mob->fighting))
			{
				if (IS_NPC(mob->fighting))
					strcpy(t, mob->fighting->short_desc);
				else
				{
					strcpy(t, mob->fighting->getNameC());
					strcat(t, " ");
					strcat(t, mob->fighting->title);
				}
			}
		}
		break;
	case 'r':
		if (rndm)
		{
			if (CAN_SEE(mob, rndm))
				one_argument(rndm->getNameC(), t);
			if (IS_PC(rndm))
				*t = UPPER(*t);
		}
		break;

	case 'R':
		if (rndm)
		{
			if (CAN_SEE(mob, rndm))
			{
				if (IS_NPC(rndm))
				{
					strcpy(t, rndm->short_desc);
				}
				else
				{
					strcpy(t, rndm->getNameC());
					strcat(t, " ");
					strcat(t, rndm->title);
				}
			}
			else
			{
				strcpy(t, "someone");
			}
		}
		break;

	case 'e':
		if (actor)
			CAN_SEE(mob, actor) ? strcpy(t, he_she[actor->sex])
								: strcpy(t, "someone");
		break;

	case 'm':
		if (actor)
			CAN_SEE(mob, actor) ? strcpy(t, him_her[actor->sex])
								: strcpy(t, "someone");
		break;

	case 's':
		if (actor)
			CAN_SEE(mob, actor) ? strcpy(t, his_her[actor->sex])
								: strcpy(t, "someone's");
		break;

	case 'E':
		if (vict)
			CAN_SEE(mob, vict) ? strcpy(t, he_she[vict->sex])
							   : strcpy(t, "someone");
		break;

	case 'M':
		if (vict)
			CAN_SEE(mob, vict) ? strcpy(t, him_her[vict->sex])
							   : strcpy(t, "someone");
		break;

	case 'S':
		if (vict)
			CAN_SEE(mob, vict) ? strcpy(t, his_her[vict->sex])
							   : strcpy(t, "someone's");
		break;
	case 'q':
		char buf[15];
		sprintf(buf, "%lu", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
		strcpy(t, buf);
		break;
	case 'j':
		strcpy(t, he_she[mob->sex]);
		break;

	case 'k':
		strcpy(t, him_her[mob->sex]);
		break;

	case 'l':
		strcpy(t, his_her[mob->sex]);
		break;

	case 'J':
		if (rndm)
			CAN_SEE(mob, rndm) ? strcpy(t, he_she[rndm->sex])
							   : strcpy(t, "someone");
		break;

	case 'K':
		if (rndm)
			CAN_SEE(mob, rndm) ? strcpy(t, him_her[rndm->sex])
							   : strcpy(t, "someone");
		break;

	case 'L':
		if (rndm)
			CAN_SEE(mob, rndm) ? strcpy(t, his_her[rndm->sex])
							   : strcpy(t, "someone's");
		break;

	case 'o':
		if (obj)
			CAN_SEE_OBJ(mob, obj) ? one_argument(obj->name, t)
								  : strcpy(t, "something");
		break;

	case 'O':
		if (obj)
			CAN_SEE_OBJ(mob, obj) ? strcpy(t, obj->short_description)
								  : strcpy(t, "something");
		break;

	case 'p':
		if (v_obj)
			CAN_SEE_OBJ(mob, v_obj) ? one_argument(v_obj->name, t)
									: strcpy(t, "something");
		break;

	case 'P':
		if (v_obj)
			CAN_SEE_OBJ(mob, v_obj) ? strcpy(t, v_obj->short_description)
									: strcpy(t, "something");
		break;

	case 'a':
		if (obj)
			switch (*(obj->name))
			{
			case 'a':
			case 'e':
			case 'i':
			case 'o':
			case 'u':
				strcpy(t, "an");
				break;
			default:
				strcpy(t, "a");
			}
		break;

	case 'A':
		if (v_obj)
			switch (*(v_obj->name))
			{
			case 'a':
			case 'e':
			case 'i':
			case 'o':
			case 'u':
				strcpy(t, "an");
				break;
			default:
				strcpy(t, "a");
			}
		break;

	case '$':
		strcpy(t, "$");
		break;

	default:
		logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad $$var : %c", DC::getInstance()->mob_index[mob->mobdata->vnum].virt, ch);
		break;
	}

	return;
}

bool do_bufs(char *bufpt, char *argpt, char *point)
{
	bool traditional = false;

	/* get whatever comes before the left paren.. ignore spaces */
	while (*point)
		if (*point == '(')
		{
			traditional = true;
			break;
		}
		else if (*point == ' ')
		{
			return false;
		}
		else if (*point == '.')
		{
			break;
		}
		else if (*point == '\0')
		{
			return false;
		}
		else if (*point == ' ')
			point++;
		else
			*bufpt++ = *point++;
	*bufpt = '\0';
	point++;

	/* get whatever is in between the parens.. ignore spaces */
	while (*point)
		if (traditional && *point == ')')
			break;
		else if (!traditional && !isalpha(*point))
		{
			point--;
			break;
		}
		else if (*point == '\0')
		{
			return false;
		}
		else if (*point == ' ')
			point++;
		else
			*argpt++ = *point++;

	*argpt = '\0';
	//  point++;
	return true;
}

void debugpoint() {};
/* This procedure simply copies the cmnd to a buffer while expanding
 * any variables by calling the translate procedure.  The observant
 * code scrutinizer will notice that this is taken from act()
 */
int mprog_process_cmnd(char *cmnd, Character *mob, Character *actor,
					   Object *obj, void *vo, Character *rndm)
{
	char buf[MAX_INPUT_LENGTH * 2];
	char tmp[MAX_INPUT_LENGTH * 2];
	char *str;
	char *i;
	char *point;

	point = buf;
	str = cmnd;
	while (*str == ' ')
		str++;
	/*
	  while (*str != '\0')
	  {
		 if ((*str == '=' || *str == '+' || *str == '-' || *str == '&' || *str == '|' ||
			*str == '*' || *str == '/') && *(str+1) == '=' && *(str+2) != '\0')
		 {
		  int16_t *lvali = 0;
		  uint32_t *lvalui = 0;
		  char **lvalstr = 0;
		  int64_t *lvali64 = 0;
		  int8_t *lvalb = 0;
		  *str = '\0';
		  if (do_bufs(&buf[0], &tmp[0], cmnd))
			translate_value(buf, tmp, &lvali,&lvalui, &lvalstr,&lvali64, &lvalb,mob,actor, obj, vo, rndm);
		  else strcpy(left, cmnd);

		  str += 2;

		  while ( *str == ' ' )
			str++;
		  if (do_bufs(&buf[0], &tmp[0], str))
			translate_value(buf,tmp,&lvali,&lvalui, &lvalstr,&lvali64, &lvalb,mob,actor, obj, vo, rndm);
		  else strcpy(right, str);
		char buf[MAX_STRING_LENGTH];
		buf[0] = '\0';
		if (lvali)
		  sprintf(buf, "%sLvali: %d\n", buf,*lvali);
		if (lvalui)
		  sprintf(buf, "%sLvalui: %d\n", buf,*lvalui);
		if (lvali64)
		  sprintf(buf, "%sLvali64: %ld\n", buf,*lvali64);
		if (lvalb)
		  sprintf(buf, "%sLvalb: %d\n", buf,*lvalb);
		if (lvalstr)
		  sprintf(buf, "%sLvalstr: %s\n", buf,lvalstr);
		sprintf(buf,"%sLeft: %s\n",buf,left);
		sprintf(buf,"%sRight: %s\n",buf,right);
	//        if (actor)
	//	  actor->send(buf);
		return eSUCCESS;
		 }
		 str++;
	  }*/
	str = cmnd;
	while (*str != '\0')
	{
		if (*str != '$')
		{
			*point++ = *str++;
			continue;
		}
		str++;
		if (*str == '\0')
			break; // panic!
		if (*(str + 1) == '.')
		{
			int16_t *lvali = 0;
			uint32_t *lvalui = 0;
			char **lvalstr = 0;
			QString lvalqstr;
			int64_t *lvali64 = 0;
			uint64_t *lvalui64 = 0;
			int8_t *lvalb = 0;
			char left[MAX_INPUT_LENGTH], right[MAX_INPUT_LENGTH];
			left[0] = '$';
			left[1] = *str;
			left[2] = '\0';
			str = one_argument(str + 2, right);
			translate_value(left, right, &lvali, &lvalui, &lvalstr, &lvali64, &lvalui64, &lvalb, mob, actor, obj, vo, rndm, lvalqstr);
			char buf[MAX_STRING_LENGTH];
			buf[0] = '\0';
			if (lvali)
				sprintf(buf, "%d", *lvali);
			if (lvalui)
				sprintf(buf, "%u", *lvalui);
			if (lvalstr)
				sprintf(buf, "%s", *lvalstr);
			if (lvali64)
				sprintf(buf, "%ld", *lvali64);
			if (lvalui64)
				sprintf(buf, "%lu", *lvalui64);
			if (lvalb)
				sprintf(buf, "%d", *lvalb);
			for (int i = 0; buf[i]; i++)
				*point++ = buf[i];
			continue;
		}

		if (LOWER(*str) == 'v' || LOWER(*str) == 'w')
		{
			char a = *str;
			str++;
			if (*str == '[')
			{
				char *tmp1 = &tmp[0];
				str++;
				while (*str != ']' && *str != '\0')
					*tmp1++ = *str++;
				*tmp1 = '\0';
				Character *who = nullptr;
				if (a == 'v')
					who = mob;
				else if (a == 'V')
					who = actor;
				else if (a == 'w')
					who = (Character *)vo;
				else if (a == 'W')
					who = rndm;
				if (who)
				{
					struct tempvariable *eh = who->tempVariable;
					for (; eh; eh = eh->next)
						if (eh->name == tmp)
							break;
					if (eh)
					{
						strcpy(tmp, eh->data.toStdString().c_str());
						if (!eh->data.isEmpty())
							eh->data[0] = eh->data[0].toUpper();
					}
					else
						continue; // Doesn't have the variable.
				}
			}
		}
		else
			mprog_translate(*str, tmp, mob, actor, obj, vo, rndm);
		i = tmp;
		++str;
		while ((*point = *i) != '\0')
			++point, ++i;
	}
	*point = '\0';

	//  if(strlen(buf) > MAX_INPUT_LENGTH-1)
	//    logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Warning!  Mob '%s' has MobProg command longer than max input.", GET_NAME(mob));

	return mob->command_interpreter(buf, true);
}

bool objExists(Object *obj)
{
	Object *tobj;

	for (tobj = DC::getInstance()->object_list; tobj; tobj = tobj->next)
		if (tobj == obj)
			break;
	if (tobj)
		return true;
	return false;
}

/* The main focus of the MOBprograms.  This routine is called
 *  whenever a trigger is successful.  It is responsible for parsing
 *  the command list and figuring out what to do. However, like all
 *  complex procedures, everything is farmed out to the other guys.
 */
void mprog_driver(char *com_list, Character *mob, Character *actor,
				  Object *obj, void *vo, struct mprog_throw_type *thrw, Character *rndm)
{

	char tmpcmndlst[MAX_STRING_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	char *morebuf;
	char *command_list;
	char *cmnd;
	// Character *rndm  = nullptr;
	Character *vch = nullptr;
	int count = 0;
	if (IS_AFFECTED(mob, AFF_CHARM))
		return;
	selfpurge = false;
	mprog_cur_result = eSUCCESS;
	mprog_line_num = 0;

	activeProgs++;
	if (activeProgs > 20)
	{
		logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d : Too many active mobprograms : LOOP", DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
		activeProgs--;
		return;
	}

	// int cIfs[256]; // for MPPAUSE
	// int ifpos;
	ifpos = 0;
	memset(&cIfs[0], 0, sizeof(int) * 256);

	if (!charExists(actor))
		actor = nullptr;
	if (!objExists(obj))
		obj = nullptr;

	activeActor = actor;
	activeObj = obj;
	activeVo = vo;

	activeProg = com_list;

	if (!com_list) // this can happen when someone is editing
	{
		activeActor = activeRndm = nullptr;
		activeObj = nullptr;
		activeVo = nullptr;
		activeProgs--;
		return;
	}

	// count valid random victs in room
	if (mob->in_room > 0)
	{
		for (vch = DC::getInstance()->world[mob->in_room].people; vch; vch = vch->next_in_room)
			if (CAN_SEE(mob, vch, true) && IS_PC(vch))
				count++;

		if (count)
			count = number(1, count); // if we have valid victs, choose one

		if (!rndm && count)
		{
			for (vch = DC::getInstance()->world[mob->in_room].people; vch && count;)
			{
				if (CAN_SEE(mob, vch, true) && IS_PC(vch))
					count--;
				if (count)
					vch = vch->next_in_room;
			}
			rndm = vch;
		}

		activeRndm = rndm;
	}

	strcpy(tmpcmndlst, com_list);

	command_list = tmpcmndlst;
	cmnd = command_list;
	activeProgTmpBuf = command_list;
	activePos = command_list = mprog_next_command(command_list);

	// if (thrw) thrw->orig = &tmpcmndlst[0];
	while (*cmnd != '\0')
	{
		morebuf = one_argument(cmnd, buf);

		if (!str_cmp(buf, "if"))
		{
			activePos = command_list = mprog_process_if(morebuf, command_list, mob,
														actor, obj, vo, rndm, thrw);
			if (isSet(mprog_cur_result, eCH_DIED) || isSet(mprog_cur_result, eDELAYED_EXEC))
			{
				activeActor = activeRndm = nullptr;
				activeObj = nullptr;
				activeVo = nullptr;
				activeProgs--;
				return;
			}
		}
		else
		{

			if (!thrw || DIFF(cmnd, activeProgTmpBuf) >= thrw->startPos)
			{
				SET_BIT(mprog_cur_result, mprog_process_cmnd(cmnd, mob, actor, obj, vo, rndm));
				if (isSet(mprog_cur_result, eCH_DIED) || selfpurge || isSet(mprog_cur_result, eDELAYED_EXEC))
				{
					activeActor = activeRndm = nullptr;
					activeObj = nullptr;
					activeVo = nullptr;
					activeProgs--;
					return;
				}
			}
		}
		cmnd = command_list;
		activePos = command_list = mprog_next_command(command_list);
	}
	activeActor = activeRndm = nullptr;
	activeObj = nullptr;
	activeVo = nullptr;
	activeProgs--;
	return;
}

/***************************************************************************
 * Global function code and brief comments.
 */

/* The next two routines are the basic trigger types. Either trigger
 *  on a certain percent, or trigger on a keyword or word phrase.
 *  To see how this works, look at the various trigger routines..
 */
// Returns true if match
// false if no match
int mprog_wordlist_check(QString arg, Character *mob, Character *actor,
						 Object *obj, void *vo, int type, bool reverse)
// reverse ALSO IMPLIES IT ALSO ONLY CHECKS THE FIRST WORD
{

	char temp1[MAX_STRING_LENGTH]{};
	char temp2[MAX_STRING_LENGTH]{};
	char word[MAX_INPUT_LENGTH]{};
	QSharedPointer<class MobProgram> mprg{};
	QSharedPointer<class MobProgram> next{};
	char *list{};
	char *start{};
	char *dupl{};
	char *end{};
	int i{};
	int retval{};
	bool done{};
	//  for ( mprg = DC::getInstance()->mob_index[mob->mobdata->nr].mobprogs; mprg != nullptr; mprg
	//= next )
	mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobprogs;
	if (!mprg)
	{
		done = true;
		mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
	}

	mprog_command_num = 0;
	for (; mprg != nullptr; mprg = next)
	{
		mprog_command_num++;
		next = mprg->next;
		if (mprg->type & type)
		{
			if (!reverse)
				strcpy(temp1, mprg->arglist);
			else
				strcpy(temp1, arg.toStdString().c_str());

			list = temp1;
			for (i = 0; i < (signed)strlen(list); i++)
				list[i] = LOWER(list[i]);

			if (!reverse)
				strcpy(temp2, arg.toStdString().c_str());
			else
				strcpy(temp2, mprg->arglist);

			dupl = temp2;
			for (i = 0; i < (signed)strlen(dupl); i++)
				dupl[i] = LOWER(dupl[i]);
			if ((list[0] == 'p') && (list[1] == ' '))
			{
				list += 2;
				while ((start = strstr(dupl, list)))
					if ((start == dupl || *(start - 1) == ' ') && (*(end = start + strlen(list)) == ' ' || *end == '\n' || *end == '\r' || *end == '\0'
																   // allow punctuation at the end
																   || *end == '.' || *end == '?' || *end == '!'))
					{
						retval = 1;
						mprog_driver(mprg->comlist, mob, actor, obj, vo, nullptr, nullptr);
						if (selfpurge || isSet(mprog_cur_result, eCH_DIED))
							return retval;
						break;
					}
					else
						dupl = start + 1;
			}
			else
			{
				list = one_argument(list, word);
				for (; word[0] != '\0'; list = one_argument(list, word))
				{
					while ((start = strstr(dupl, word)))
						if ((start == dupl || *(start - 1) == ' ') && (*(end = start + strlen(word)) == ' ' || *end == '\n' || *end == '\r' || *end == '\0'
																	   // allow punctuation at the end
																	   || *end == '.' || *end == '?' || *end == '!'))
						{
							retval = 1;
							mprog_driver(mprg->comlist, mob, actor, obj, vo,
										 nullptr, nullptr);
							if (selfpurge || isSet(mprog_cur_result, eCH_DIED))
								return retval;
							break;
						}
						else
							dupl = start + 1;
					if (reverse)
						break;
				}
			}
		}
		if (next == nullptr && !done)
		{
			done = true;
			next = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
		}
	}
	return retval;
}

void mprog_percent_check(Character *mob, Character *actor, Object *obj,
						 void *vo, int type)
{
	QSharedPointer<class MobProgram> mprg{};
	QSharedPointer<class MobProgram> next{};
	bool done = false;
	mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobprogs;
	if (!mprg)
	{
		done = true;
		mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
	}

	if (DC::getInstance()->mob_index[mob->mobdata->vnum].virt == 30013)
	{
		debugpoint();
	}

	mprog_command_num = 0;
	for (; mprg != nullptr; mprg = next)
	{
		mprog_command_num++;
		next = mprg->next;
		if ((mprg->type & type) && (number(0, 99) < atoi(mprg->arglist)))
		{
			mprog_driver(mprg->comlist, mob, actor, obj, vo, nullptr, nullptr);
			if (selfpurge)
				return;
			if (type != GREET_PROG && type != ALL_GREET_PROG)
				break;
		}
		if (!next && !done)
		{
			done = true;
			next = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
		}
	}
	return;
}

/* The triggers.. These are really basic, and since most appear only
 * once in the code (hmm. i think they all do) it would be more efficient
 * to substitute the code in and make the mprog_xxx_check routines global.
 * However, they are all here in one nice place at the moment to make it
 * easier to see what they look like. If you do substitute them back in,
 * make sure you remember to modify the variable names to the ones in the
 * trigger calls.
 */
int mprog_act_trigger(std::string buf, Character *mob, Character *ch,
					  Object *obj, void *vo)
{

	//  mob_prog_act_list * tmp_act;
	// mob_prog_act_list * curr;
	//  QSharedPointer<class MobProgram> mprg;
	mprog_cur_result = eSUCCESS;

	if (!MOBtrigger)
		return mprog_cur_result;

	if (IS_NPC(mob) && (DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & ACT_PROG) && isPaused(mob) == false)
		mprog_wordlist_check(buf.c_str(), mob, ch,
							 obj, vo, ACT_PROG);

	/* Why oh why was it like this? They can add lag themselves if needed.
	  if
	( IS_NPC( mob )
		  && ( DC::getInstance()->mob_index[mob->mobdata->nr].progtypes & ACT_PROG ) )
		{
	#ifdef LEAK_CHECK
		  tmp_act = (mob_prog_act_list *) calloc( 1, sizeof( mob_prog_act_list ) );
	#else
		  tmp_act = (mob_prog_act_list *) dc_alloc( 1, sizeof( mob_prog_act_list ) );
	#endif

		  if(!mob->mobdata->mpact)
			mob->mobdata->mpact = tmp_act;
		  else {
			curr = mob->mobdata->mpact;
			while(curr->next)
			  curr = curr->next;
			curr->next = tmp_act;
		  }

		  tmp_act->next = nullptr;
		  tmp_act->buf = str_dup( buf );
		  tmp_act->ch  = ch;
		  tmp_act->obj = obj;
		  tmp_act->vo  = vo;
		  mob->mobdata->mpactnum++;

		}
	*/
	return mprog_cur_result;
}

int mprog_bribe_trigger(Character *mob, Character *ch, int amount)
{

	QSharedPointer<class MobProgram> mprg{};
	QSharedPointer<class MobProgram> next{};
	Object *obj = 0;
	bool done = false;

	if (IS_NPC(mob) && (DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & BRIBE_PROG) && isPaused(mob) == false)
	{
		mob->removeGold(amount);

		mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobprogs;
		if (!mprg)
		{
			done = true;
			mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
		}

		mprog_command_num = 0;
		for (; mprg != nullptr; mprg = next)
		{
			mprog_command_num++;
			next = mprg->next;

			if ((mprg->type & BRIBE_PROG) && (amount >= atoi(mprg->arglist)))
			{
				mprog_driver(mprg->comlist, mob, ch, obj, nullptr, nullptr, nullptr);
				if (selfpurge)
					return mprog_cur_result;
				break;
			}

			if (!next && !done)
			{
				next = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
				done = true;
			}
		}
	}

	return mprog_cur_result;
}

int mprog_damage_trigger(Character *mob, Character *ch, int amount)
{

	QSharedPointer<class MobProgram> mprg{};
	QSharedPointer<class MobProgram> next{};
	Object *obj = 0;
	bool done = false;
	if (IS_NPC(mob) && (DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & DAMAGE_PROG) && isPaused(mob) == false)
	{
		mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobprogs;

		if (!mprg)
		{
			done = true;
			mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
		}

		mprog_command_num = 0;
		for (; mprg != nullptr; mprg = next)
		{
			mprog_command_num++;
			next = mprg->next;

			if ((mprg->type & DAMAGE_PROG) && (amount >= atoi(mprg->arglist)))
			{
				mprog_driver(mprg->comlist, mob, ch, obj, nullptr, nullptr, nullptr);
				if (selfpurge)
					return mprog_cur_result;
				break;
			}
			if (!next && !done)
			{
				next = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
				done = true;
			}
		}
	}

	return mprog_cur_result;
}

int mprog_death_trigger(Character *mob, Character *killer)
{

	if (IS_NPC(mob) && (DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & DEATH_PROG) && isPaused(mob) == false)
	{
		mprog_percent_check(mob, killer, nullptr, nullptr, DEATH_PROG);
	}
	if (!SOMEONE_DIED(mprog_cur_result))
		death_cry(mob);
	return mprog_cur_result;
}

int mprog_entry_trigger(Character *mob)
{

	if (IS_NPC(mob) && (DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & ENTRY_PROG) && isPaused(mob) == false)
		mprog_percent_check(mob, nullptr, nullptr, nullptr, ENTRY_PROG);

	return mprog_cur_result;
}

int mprog_fight_trigger(Character *mob, Character *ch)
{

	if (IS_NPC(mob) && MOB_WAIT_STATE(mob) <= 0 && (DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & FIGHT_PROG) && isPaused(mob) == false)
		mprog_percent_check(mob, ch, nullptr, nullptr, FIGHT_PROG);

	return mprog_cur_result;
}

int mprog_attack_trigger(Character *mob, Character *ch)
{

	if (IS_NPC(mob) && (DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & ATTACK_PROG) && isPaused(mob) == false)
		mprog_percent_check(mob, ch, nullptr, nullptr, ATTACK_PROG);

	return mprog_cur_result;
}

int mprog_give_trigger(Character *mob, Character *ch, Object *obj)
{

	char buf[MAX_INPUT_LENGTH];
	QSharedPointer<class MobProgram> mprg{};
	QSharedPointer<class MobProgram> next{};
	bool done = false, okay = false;
	if (IS_NPC(mob) && (DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & GIVE_PROG) && isPaused(mob) == false)
	{
		mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobprogs;
		if (!mprg)
		{
			done = true;
			mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
		}

		mprog_command_num = 0;
		for (; mprg != nullptr; mprg = next)
		{
			mprog_command_num++;

			next = mprg->next;
			one_argument(mprg->arglist, buf);
			if ((mprg->type & GIVE_PROG) && ((!str_cmp(obj->name, mprg->arglist)) || (!str_cmp("all", buf))))
			{
				okay = true;
				mprog_driver(mprg->comlist, mob, ch, obj, nullptr, nullptr, nullptr);
				if (selfpurge)
					return mprog_cur_result;
				break;
			}

			if (!next && !done)
			{
				done = true;
				next = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
			}
		}
	}

	if (okay && !SOMEONE_DIED(mprog_cur_result))
	{
		Object *a;
		SET_BIT(mprog_cur_result, eEXTRA_VALUE);
		for (a = mob->carrying; a; a = a->next_content)
			if (a == obj)
			{
				REMOVE_BIT(mprog_cur_result, eEXTRA_VALUE);
			}
	}

	return mprog_cur_result;
}

int mprog_greet_trigger(Character *ch)
{

	Character *vmob;

	mprog_cur_result = eSUCCESS;

	for (vmob = DC::getInstance()->world[ch->in_room].people; vmob != nullptr; vmob = vmob->next_in_room)
		if (IS_NPC(vmob) && (vmob->fighting == nullptr) && AWAKE(vmob))
		{
			if (ch != vmob && CAN_SEE(vmob, ch) && (DC::getInstance()->mob_index[vmob->mobdata->vnum].progtypes & GREET_PROG) && isPaused(vmob) == false)
				mprog_percent_check(vmob, ch, nullptr, nullptr, GREET_PROG);
			else if ((DC::getInstance()->mob_index[vmob->mobdata->vnum].progtypes & ALL_GREET_PROG) && isPaused(vmob) == false)
				mprog_percent_check(vmob, ch, nullptr, nullptr, ALL_GREET_PROG);

			if (SOMEONE_DIED(mprog_cur_result) || selfpurge)
				break;
		}
	return mprog_cur_result;
}

int mprog_hitprcnt_trigger(Character *mob, Character *ch)
{
	QSharedPointer<class MobProgram> mprg{};
	QSharedPointer<class MobProgram> next{};
	bool done = false;

	if (IS_NPC(mob) && MOB_WAIT_STATE(mob) <= 0 && (DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & HITPRCNT_PROG) && isPaused(mob) == false)
	{
		mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobprogs;
		if (!mprg)
		{
			done = true;
			mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
		}

		mprog_command_num = 0;
		for (; mprg != nullptr; mprg = next)
		{
			mprog_command_num++;
			next = mprg->next;

			if ((mprg->type & HITPRCNT_PROG) && ((100 * mob->hit / mob->max_hit) < atoi(mprg->arglist)))
			{
				mprog_driver(mprg->comlist, mob, ch, nullptr, nullptr, nullptr, nullptr);
				if (selfpurge)
					return mprog_cur_result;
				break;
			}

			if (!next && !done)
			{
				done = true;
				next = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
			}
		}
	}

	return mprog_cur_result;
}

int mprog_random_trigger(Character *mob)
{
	mprog_cur_result = eSUCCESS;

	if ((DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & RAND_PROG) && isPaused(mob) == false)
		mprog_percent_check(mob, nullptr, nullptr, nullptr, RAND_PROG);

	return mprog_cur_result;
}

int mprog_load_trigger(Character *mob)
{
	if (!mob || mob->isDead() || isNowhere(mob))
	{
		return eFAILURE;
	}

	mprog_cur_result = eSUCCESS;
	if ((DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & LOAD_PROG) && isPaused(mob) == false)
		mprog_percent_check(mob, nullptr, nullptr, nullptr, LOAD_PROG);
	return mprog_cur_result;
}

int mprog_arandom_trigger(Character *mob)
{
	if (!mob || mob->isDead() || isNowhere(mob))
	{
		return eFAILURE;
	}
	mprog_cur_result = eSUCCESS;
	if ((DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & ARAND_PROG) && isPaused(mob) == false)
		mprog_percent_check(mob, nullptr, nullptr, nullptr, ARAND_PROG);
	return mprog_cur_result;
}

int mprog_can_see_trigger(Character *ch, Character *mob)
{
	if (!ch || ch->isDead() || isNowhere(ch))
	{
		return eFAILURE;
	}

	if (!mob || mob->isDead() || isNowhere(mob))
	{
		return eFAILURE;
	}

	mprog_cur_result = eSUCCESS;
	if ((DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & CAN_SEE_PROG) && isPaused(mob) == false)
		mprog_percent_check(mob, ch, nullptr, nullptr, CAN_SEE_PROG);

	return mprog_cur_result;
}

int mprog_speech_trigger(const char *txt, Character *mob)
{
	if (!mob || mob->isDead() || isNowhere(mob))
	{
		return eFAILURE;
	}

	Character *vmob;

	mprog_cur_result = eSUCCESS;

	for (vmob = DC::getInstance()->world[mob->in_room].people; vmob != nullptr; vmob = vmob->next_in_room)
		if (IS_NPC(vmob) && (DC::getInstance()->mob_index[vmob->mobdata->vnum].progtypes & SPEECH_PROG) && isPaused(vmob) == false)
		{
			if (mprog_wordlist_check(txt, vmob, mob, nullptr, nullptr, SPEECH_PROG))
				break;
		}

	return mprog_cur_result;
}

int mprog_catch_trigger(Character *mob, int catch_num, char *var, int opt, Character *actor, Object *obj, void *vo, Character *rndm)
{
	if (!mob || mob->isDead() || isNowhere(mob))
	{
		return eFAILURE;
	}

	QSharedPointer<class MobProgram> mprg{};
	QSharedPointer<class MobProgram> next{};
	int curr_catch;
	bool done = false;
	mprog_cur_result = eFAILURE;

	if (IS_NPC(mob) && (DC::getInstance()->mob_index[mob->mobdata->vnum].progtypes & CATCH_PROG) && isPaused(mob) == false)
	{
		mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobprogs;
		if (!mprg || (opt & 1))
		{
			done = true;
			mprg = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
		}

		mprog_command_num = 0;
		for (; mprg != nullptr; mprg = next)
		{
			mprog_command_num++;
			next = mprg->next;
			if (mprg->type & CATCH_PROG)
			{
				if (!check_range_valid_and_convert(curr_catch, mprg->arglist, MPROG_CATCH_MIN, MPROG_CATCH_MAX))
				{
					logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Invalid catch argument: vnum %d",
						 DC::getInstance()->mob_index[mob->mobdata->vnum].virt);
					return eFAILURE;
				}
				if (curr_catch == catch_num)
				{
					if (var)
					{
						struct tempvariable *eh;
						for (eh = mob->tempVariable; eh; eh = eh->next)
						{
							if (eh->name == "throw")
								break;
						}
						if (eh)
						{
							eh->data = var;
						}
						else
						{
#ifdef LEAK_CHECK
							eh = (struct tempvariable *)
								calloc(1, sizeof(struct tempvariable));
#else
							eh = (struct tempvariable *)
								dc_alloc(1, sizeof(struct tempvariable));
#endif

							eh->data = var;
							eh->name = str_dup("throw");
							eh->next = mob->tempVariable;
							mob->tempVariable = eh;
						}
					}
					mprog_driver(mprg->comlist, mob, actor, obj, vo, nullptr, rndm);
					if (selfpurge)
						return mprog_cur_result;

					break;
				}
			}
			if (!next && !done)
			{
				done = true;
				next = DC::getInstance()->mob_index[mob->mobdata->vnum].mobspec;
			}
		}
	}
	return mprog_cur_result;
}

void update_mprog_throws()
{
	struct mprog_throw_type *curr;
	struct mprog_throw_type *action;
	struct mprog_throw_type *last = nullptr;
	Character *vict;
	Object *vobj;
	for (curr = g_mprog_throw_list; curr;)
	{
		// update
		if (curr->delay > 0)
		{
			curr->delay--;
			last = curr;
			curr = curr->next;
			continue;
		}
		vobj = nullptr;
		vict = nullptr;

		//		if (curr->data_num == -999)
		//			debugpoint();

		if (curr->tMob && charExists(curr->tMob) && curr->tMob->in_room >= 0)
		{
			vict = curr->tMob;
		}
		else if (curr->mob)
		{
			// find target
			if (*curr->target_mob_name)
			{ // find me by name
				vict = get_mob(curr->target_mob_name);
			}
			else
			{ // find me by num
				vict = get_mob_vnum(curr->target_mob_num);
			}
		}
		else
		{
			if (*curr->target_mob_name)
				vobj = get_obj(curr->target_mob_name);
			else
				vobj = get_obj_vnum(curr->target_mob_num);
		}

		// remove from list
		if (last)
		{
			last->next = curr->next;
			action = curr;
			curr = last->next;
			// last doesn't move
		}
		else
		{
			g_mprog_throw_list = curr->next;
			action = curr;
			curr = g_mprog_throw_list;
		}

		// This is done this way in case the 'catch' does a 'throw' inside of it

		// if !vict, oh well....remove it anyway.  Someone killed him.
		if (action->data_num == -999 && vict)
		{ // 'tis a pause
			// Only resume a MPPAUSE <duration> if we're not in the middle of a MPPAUSE all <duration>
			if (isPaused(action->tMob) == false)
			{
				mprog_driver(action->orig, vict, action->actor, action->obj, action->vo, action, action->rndm);
			}

			dc_free(action->orig);
			action->orig = 0;
		}
		else if (action->data_num == -1000 && vict)
		{
			if (isPaused(action->tMob))
			{
				action->tMob->mobdata->paused = false;
			}
			mprog_driver(action->orig, vict, action->actor, action->obj, action->vo, action, action->rndm);
			dc_free(action->orig);
			action->orig = 0;
		}
		else if (vict)
		{ // activate
			if (vict->in_room >= 0)
			{
				mprog_catch_trigger(vict, action->data_num, action->var, action->opt, action->actor, action->obj, action->vo, action->rndm);
			}
		}
		else if (vobj)
		{
			oprog_catch_trigger(vobj, action->data_num, action->var, action->opt, action->actor, action->obj, action->vo, action->rndm);
		}

		dc_free(action);
	}
}

Character *initiate_oproc(Character *ch, Object *obj)
{ // Sneakiness.
	Character *temp;
	temp = clone_mobile(real_mobile(12));
	DC::getInstance()->mob_index[real_mobile(12)].mobprogs = DC::getInstance()->obj_index[obj->vnum].mobprogs;
	DC::getInstance()->mob_index[real_mobile(12)].progtypes = DC::getInstance()->obj_index[obj->vnum].progtypes;

	if (ch)
		char_to_room(temp, ch->in_room);
	else
		char_to_room(temp, obj->in_room);
	if (ch)
		temp->beacon = (Object *)ch;
	temp->mobdata->setObject(obj);
	//  temp->master = ch;
	// dc_free(temp->short_desc);
	temp->short_desc = str_hsh(obj->short_description);
	// dc_free(temp->name);
	char buf[MAX_STRING_LENGTH];
	sprintf(buf, "%s", obj->name);
	for (int i = strlen(buf) - 1; i > 0; i--)
		if (buf[i] == ' ')
		{
			buf[i] = '\0';
			break;
		}
	temp->setName(buf);

	temp->setType(Character::Type::ObjectProgram);
	temp->objdata = obj;

	return temp;
}

void end_oproc(Character *ch, Trace trace)
{
	static int core_counter = 0;
	if (selfpurge)
	{
		logentry(QStringLiteral("Crash averted in end_oproc() %1 %2").arg(selfpurge.getFunction().c_str()).arg(selfpurge.getState()), IMMORTAL, DC::LogChannel::LOG_BUG);

		if (core_counter++ < 10)
		{
			produce_coredump();
			logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Corefile produced.");
		}
	}
	else
	{
		trace.addTrack("end_oproc");
		extract_char(ch, true, trace);
		DC::getInstance()->mob_index[real_mobile(12)].progtypes = {};
		DC::getInstance()->mob_index[real_mobile(12)].mobprogs = {};
	}
}

int oprog_can_see_trigger(Character *ch, Object *item)
{
	if (!ch || ch->isDead() || isNowhere(ch))
	{
		return eFAILURE;
	}

	Character *vmob;
	mprog_cur_result = eSUCCESS;

	if (DC::getInstance()->obj_index[item->vnum].progtypes & CAN_SEE_PROG)
	{
		vmob = initiate_oproc(ch, item);
		mprog_percent_check(vmob, ch, item, nullptr, CAN_SEE_PROG);
		end_oproc(vmob, Trace("oprog_can_see_trigger"));
		return mprog_cur_result;
	}
	return mprog_cur_result;
}

int oprog_speech_trigger(const char *txt, Character *ch)
{
	if (!ch || ch->isDead() || isNowhere(ch))
	{
		return eFAILURE;
	}

	Character *vmob = nullptr;
	Object *item;

	mprog_cur_result = eSUCCESS;

	for (item = DC::getInstance()->world[ch->in_room].contents; item; item = item->next_content)
		if (DC::getInstance()->obj_index[item->vnum].progtypes & SPEECH_PROG)
		{
			vmob = initiate_oproc(ch, item);
			if (mprog_wordlist_check(txt, vmob, ch, nullptr, nullptr, SPEECH_PROG))
			{
				end_oproc(vmob, Trace("oprog_speech_trigger1"));
				return mprog_cur_result;
			}
			end_oproc(vmob, Trace("oprog_speech_trigger2"));
		}
	for (item = ch->carrying; item; item = item->next_content)
		if (DC::getInstance()->obj_index[item->vnum].progtypes & SPEECH_PROG)
		{
			vmob = initiate_oproc(ch, item);
			if (mprog_wordlist_check(txt, vmob, ch, nullptr, nullptr, SPEECH_PROG))
			{
				end_oproc(vmob, Trace("oprog_speech_trigger3"));
				return mprog_cur_result;
			}
			end_oproc(vmob, Trace("oprog_speech_trigger4"));
		}

	for (int i = 0; i < MAX_WEAR; i++)
		if (ch->equipment[i])
			if (DC::getInstance()->obj_index[ch->equipment[i]->vnum].progtypes & SPEECH_PROG)
			{
				vmob = initiate_oproc(ch, ch->equipment[i]);
				if (mprog_wordlist_check(txt, vmob, ch, nullptr, nullptr, SPEECH_PROG))
				{
					end_oproc(vmob, Trace("oprog_speech_trigger5"));
				}
				end_oproc(vmob, Trace("oprog_speech_trigger6"));
			}
	return mprog_cur_result;
}

int oprog_catch_trigger(Object *obj, int catch_num, char *var, int opt, Character *actor, Object *obj2, void *vo, Character *rndm)
{
	QSharedPointer<class MobProgram> mprg{};
	int curr_catch;
	mprog_cur_result = eFAILURE;
	Character *vmob;

	if (DC::getInstance()->obj_index[obj->vnum].progtypes & CATCH_PROG)
	{
		mprg = DC::getInstance()->obj_index[obj->vnum].mobprogs;
		mprog_command_num = 0;
		for (; mprg != nullptr; mprg = mprg->next)
		{
			mprog_command_num++;

			if (mprg->type & CATCH_PROG)
			{
				if (!check_range_valid_and_convert(curr_catch, mprg->arglist, MPROG_CATCH_MIN, MPROG_CATCH_MAX))
				{
					logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Invalid catch argument: vnum %d",
						 obj->vnum);
					return eFAILURE;
				}
				if (curr_catch == catch_num)
				{
					vmob = initiate_oproc(nullptr, obj);
					if (var)
					{
						struct tempvariable *eh;

#ifdef LEAK_CHECK
						eh = (struct tempvariable *)
							calloc(1, sizeof(struct tempvariable));
#else
						eh = (struct tempvariable *)
							dc_alloc(1, sizeof(struct tempvariable));
#endif

						eh->data = var;
						eh->name = str_dup("throw");
						eh->next = vmob->tempVariable;
						vmob->tempVariable = eh;
					}

					mprog_driver(mprg->comlist, vmob, actor, obj2, vo, nullptr, rndm);
					if (selfpurge)
						return mprog_cur_result;
					end_oproc(vmob, Trace("oprog_catch_trigger"));
					break;
				}
			}
		}
	}
	return mprog_cur_result;
}

int oprog_act_trigger(const char *txt, Character *ch)
{
	if (!ch || ch->isDead() || isNowhere(ch))
	{
		return eFAILURE;
	}

	Character *vmob;
	Object *item;

	mprog_cur_result = eSUCCESS;

	if (ch->in_room == DC::NOWHERE)
		return mprog_cur_result;

	for (item = DC::getInstance()->world[ch->in_room].contents; item; item =
																		  item->next_content)
		if (DC::getInstance()->obj_index[item->vnum].progtypes & ACT_PROG)
		{
			vmob = initiate_oproc(ch, item);
			if (mprog_wordlist_check(txt, vmob, ch, nullptr, nullptr, ACT_PROG))
			{
				end_oproc(vmob, Trace("oprog_act_trigger1"));
				return mprog_cur_result;
			}
			end_oproc(vmob, Trace("oprog_act_trigger2"));
		}
	for (item = ch->carrying; item; item = item->next_content)
		if (DC::getInstance()->obj_index[item->vnum].progtypes & ACT_PROG)
		{
			vmob = initiate_oproc(ch, item);
			if (mprog_wordlist_check(txt, vmob, ch, nullptr, nullptr, ACT_PROG))
			{
				end_oproc(vmob, Trace("oprog_act_trigger3"));
				return mprog_cur_result;
			}
			end_oproc(vmob, Trace("oprog_act_trigger4"));
		}

	for (int i = 0; i < MAX_WEAR; i++)
		if (ch->equipment[i])
			if (DC::getInstance()->obj_index[ch->equipment[i]->vnum].progtypes & ACT_PROG)
			{
				vmob = initiate_oproc(ch, ch->equipment[i]);
				if (mprog_wordlist_check(txt, vmob, ch, nullptr, nullptr, ACT_PROG))
				{
					end_oproc(vmob, Trace("oprog_act_trigger5"));
					return mprog_cur_result;
				}
				end_oproc(vmob, Trace("oprog_act_trigger6"));
			}
	return mprog_cur_result;
}

int oprog_greet_trigger(Character *ch)
{
	if (!ch || ch->isDead() || isNowhere(ch))
	{
		return eFAILURE;
	}

	Character *vmob;
	Object *item;

	mprog_cur_result = eSUCCESS;

	for (item = DC::getInstance()->world[ch->in_room].contents; item; item =
																		  item->next_content)
		if (DC::getInstance()->obj_index[item->vnum].progtypes & ALL_GREET_PROG)
		{
			vmob = initiate_oproc(ch, item);
			mprog_percent_check(vmob, ch, item, nullptr, ALL_GREET_PROG);
			end_oproc(vmob, Trace("oprog_greet_trigger"));
			return mprog_cur_result;
		}
	return mprog_cur_result;
}

int oprog_rand_trigger(Object *item)
{
	Character *vmob;
	//  Object *item;
	Character *ch;
	mprog_cur_result = eSUCCESS;
	if (item->carried_by)
		ch = item->carried_by;
	else
		ch = nullptr;
	if (DC::getInstance()->obj_index[item->vnum].progtypes & RAND_PROG)
	{
		vmob = initiate_oproc(ch, item);
		mprog_percent_check(vmob, ch, item, nullptr, RAND_PROG);
		end_oproc(vmob);
		return mprog_cur_result;
	}
	return mprog_cur_result;
}

int oprog_arand_trigger(Object *item)
{
	Character *vmob;
	Character *ch;
	mprog_cur_result = eSUCCESS;

	if (item->carried_by)
		ch = item->carried_by;
	else
		ch = nullptr;
	if (DC::getInstance()->obj_index[item->vnum].progtypes & ARAND_PROG)
	{
		vmob = initiate_oproc(ch, item);
		mprog_percent_check(vmob, ch, item, nullptr, ARAND_PROG);
		end_oproc(vmob);
		return mprog_cur_result;
	}
	return mprog_cur_result;
}

int oprog_load_trigger(Character *ch)
{

	Character *vmob;
	Object *item;

	mprog_cur_result = eSUCCESS;

	for (item = DC::getInstance()->world[ch->in_room].contents; item; item = item->next_content)
		if (DC::getInstance()->obj_index[item->vnum].progtypes & LOAD_PROG)
		{
			vmob = initiate_oproc(ch, item);
			mprog_percent_check(vmob, ch, item, nullptr, LOAD_PROG);
			end_oproc(vmob);
			return mprog_cur_result;
		}
	for (item = ch->carrying; item; item = item->next_content)
		if (DC::getInstance()->obj_index[item->vnum].progtypes & LOAD_PROG)
		{
			vmob = initiate_oproc(ch, item);
			mprog_percent_check(vmob, ch, item, nullptr, LOAD_PROG);
			end_oproc(vmob);
			return mprog_cur_result;
		}

	for (int i = 0; i < MAX_WEAR; i++)
		if (ch->equipment[i])
			if (DC::getInstance()->obj_index[ch->equipment[i]->vnum].progtypes & LOAD_PROG)
			{
				vmob = initiate_oproc(ch, item);
				mprog_percent_check(vmob, ch, item, nullptr, LOAD_PROG);
				end_oproc(vmob);
				return mprog_cur_result;
			}
	return mprog_cur_result;
}

int oprog_weapon_trigger(Character *ch, Object *item)
{
	if (!ch || ch->isDead() || isNowhere(ch))
	{
		return eFAILURE;
	}

	Character *vmob;

	mprog_cur_result = eSUCCESS;

	if (DC::getInstance()->obj_index[item->vnum].progtypes & WEAPON_PROG)
	{
		vmob = initiate_oproc(ch, item);
		mprog_percent_check(vmob, ch, item, nullptr, WEAPON_PROG);
		end_oproc(vmob);
		return mprog_cur_result;
	}

	return mprog_cur_result;
}

int oprog_armour_trigger(Character *ch, Object *item)
{
	if (!ch || ch->isDead() || isNowhere(ch))
	{
		return eFAILURE;
	}

	Character *vmob;

	mprog_cur_result = eSUCCESS;

	if (DC::getInstance()->obj_index[item->vnum].progtypes & ARMOUR_PROG)
	{
		vmob = initiate_oproc(ch, item);
		mprog_percent_check(vmob, ch, item, nullptr, ARMOUR_PROG);
		end_oproc(vmob);
		return mprog_cur_result;
	}

	return mprog_cur_result;
}

command_return_t Character::oprog_command_trigger(QString command, QString arguments)
{
	if (isDead() || isNowhere(this))
	{
		return eFAILURE;
	}

	Character *vmob = nullptr;
	Object *item = nullptr;
	mprog_cur_result = eFAILURE;
	QString buf;
	if (in_room >= 0)
	{
		for (item = DC::getInstance()->world[in_room].contents; item; item = item->next_content)
		{
			if (DC::getInstance()->obj_index[item->vnum].progtypes & COMMAND_PROG)
			{
				if (!arguments.isEmpty())
				{
					do_mpsettemp(QStringLiteral("%1 lasttyped %2").arg(getName()).arg(arguments).split(' '), CMD_OTHER);
				}

				vmob = initiate_oproc(this, item);
				if (mprog_wordlist_check(arguments, vmob, this, nullptr, nullptr, COMMAND_PROG, true))
				{
					end_oproc(vmob);
					return mprog_cur_result;
				}
				end_oproc(vmob);
			}
		}
	}

	for (item = carrying; item; item = item->next_content)
	{
		if (DC::getInstance()->obj_index[item->vnum].progtypes & COMMAND_PROG)
		{
			if (!arguments.isEmpty())
			{
				do_mpsettemp(QStringLiteral("%1 lasttyped %2").arg(getName()).arg(arguments).split(' '), CMD_OTHER);
			}
			vmob = initiate_oproc(this, item);
			if (mprog_wordlist_check(arguments, vmob, this, nullptr, nullptr, COMMAND_PROG, true))
			{
				end_oproc(vmob);
				return mprog_cur_result;
			}
			end_oproc(vmob);
		}
	}

	for (int i = 0; i < MAX_WEAR; i++)
	{
		if (equipment[i])
		{
			if (DC::getInstance()->obj_index[equipment[i]->vnum].progtypes & COMMAND_PROG)
			{
				if (!arguments.isEmpty())
				{
					do_mpsettemp(QStringLiteral("%1 lasttyped %2").arg(getName()).arg(arguments).split(' '), CMD_OTHER);
				}

				vmob = initiate_oproc(this, equipment[i]);
				if (mprog_wordlist_check(arguments, vmob, this, nullptr, nullptr, COMMAND_PROG, true))
				{
					end_oproc(vmob);
					return mprog_cur_result;
				}
				end_oproc(vmob);
			}
		}
	}
	return mprog_cur_result;
}

bool isPaused(Character *mob)
{
	if (mob == nullptr || mob == (Character *)0x95959595)
	{
		return false;
	}

	if (!charExists(mob))
	{
		return false;
	}

	if (!mob->mobdata)
	{
		return false;
	}

	if (mob->mobdata->paused == true)
	{
		return true;
	}

	return false;
}
