// This file takes care of all innate race abilities

#include "innate.h"
#include "race.h"
#include "db.h"
#include "fight.h"
#include "room.h"
#include "obj.h"
#include "connect.h"
#include "utility.h"
#include "character.h"
#include "handler.h"
#include "db.h"
#include "player.h"
#include "levels.h"
#include "interp.h"
#include "magic.h"
#include "act.h"
#include "mobile.h"
#include "spells.h"
#include <cstring> // strstr()
#include "returnvals.h"
#include "interp.h"
#include "arena.h"

////////////////////////////////////////////////////////////////////////////
// external vars
extern World world;


////////////////////////////////////////////////////////////////////////////
// external functs

////////////////////////////////////////////////////////////////////////////
// local function declarations


int innate_powerwield (Character *ch, char *argument, int cmd);
int innate_regeneration (Character *ch, char *argument, int cmd);
int innate_illusion (Character *ch, char *argument, int cmd);
int innate_repair (Character *ch, char *argument, int cmd);
int innate_focus (Character *ch, char *argument, int cmd);
int innate_evasion (Character *ch, char *argument, int cmd);
int innate_shadowslip (Character *ch, char *argument, int cmd);
int innate_bloodlust (Character *ch, char *argument, int cmd);
int innate_fly (Character *ch, char *argument, int cmd);

////////////////////////////////////////////////////////////////////////////
// local definitions
struct in_skills {
  char *name;
  int race;
  DO_FUN *func;
};

const struct in_skills innates[] = {
  { "powerwield", RACE_GIANT, innate_powerwield },
  { "regeneration", RACE_TROLL, innate_regeneration },
  { "illusion", RACE_GNOME, innate_illusion },
  { "bloodlust", RACE_ORC, innate_bloodlust},
  { "repair", RACE_DWARVEN, innate_repair},
  { "focus", RACE_ELVEN, innate_focus},
  { "evasion", RACE_PIXIE, innate_evasion},
  { "shadowslip", RACE_HOBBIT, innate_shadowslip},
  { "fly", RACE_PIXIE, innate_fly},
  { "\n", 0, nullptr}
};

char * innate_skills[] = 
{
   "powerwield",
   "focus",
   "regeneration",
   "bloodlust",
   "illusion",
   "evasion",
   "shadowslip",
   "!repair!",
   "innate skill timer",
   "fly",
   "\n"
};

////////////////////////////////////////////////////////////////////////////
// command functions
int do_innate(Character *ch, char *arg, int cmd)
{
  if(ch && ch->in_room > 0 &&
     IS_SET(world[ch->in_room].room_flags, ARENA) && arena.type == POTATO) {
    send_to_char("Cannot use innate skills within a potato arena.\r\n", ch);
    return eFAILURE;
  }

  bool found = false;
  int i;
  char buf[512];
  arg = one_argument(arg,buf);
  for (i = 0; *innates[i].name != '\n';i++)
  {
    if (innates[i].race == GET_RACE(ch))
    {
      if (buf[0] == '\0')
      {
        csendf(ch, "Your race has access to the $B%s$R innate ability.\r\n",innates[i].name);
	found = true;
      } else if (!str_cmp(innates[i].name,buf)) {
	if (str_cmp(buf, "fly") && affected_by_spell(ch,SKILL_INNATE_TIMER))
        {
	  send_to_char("You cannot use that yet.\r\n",ch);
	  return eFAILURE;
	}
	if (GET_POS(ch) == POSITION_SLEEPING && 
		i != 1)
	{
		send_to_char("In your dreams, or what?\r\n",ch);
		return eFAILURE;
	}
	int retval = (*(innates[i].func))(ch,arg,cmd);
	if (retval & eSUCCESS)
	{
	   struct affected_type af;
	   af.type = SKILL_INNATE_TIMER;

	   if (!str_cmp(buf, "fly"))
	     return retval;
	   else if (!str_cmp(buf, "repair") || !str_cmp(buf, "bloodlust")) {
	     af.duration = 12;
	   } else {
	     af.duration = 18;
	   }
		// repair is every 12 ticks
	   af.modifier = 0;
	   af.location = 0;
	   af.bitvector = -1;
	   affect_to_char(ch, &af);
	}
	return retval;
      }
    }
  }
  if (!found) {
    if (buf[0] == 0) {
      send_to_char("Your race has no innate abilities.\r\n",ch);
    } else {
      send_to_char("You do not have access to any such ability.\r\n",ch);
    }
    return eFAILURE;
  } else {
    return eSUCCESS;
  }
}

int innate_regeneration(Character *ch, char *arg, int cmd)
{
   struct affected_type af;
   af.type = SKILL_INNATE_REGENERATION;
   af.duration = 6;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = AFF_REGENERATION;
   affect_to_char(ch, &af);
   send_to_char("Your innate regenerative abilities allow you to heal quickly.\r\n",ch);
   return eSUCCESS;
}

int innate_powerwield(Character *ch, char *arg, int cmd)
{
   struct affected_type af;
   af.type = SKILL_INNATE_POWERWIELD;
   af.duration = 3;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = AFF_POWERWIELD;
   affect_to_char(ch, &af);
   send_to_char("You gather your energy in an effort to wield two mighty weapons.\r\n",ch);
   act("$n gathers his strength in order to wield two mighty weapons.",ch, nullptr,nullptr,TO_ROOM,0);
   return eSUCCESS;
}

int innate_focus(Character *ch, char *arg, int cmd)
{
   if(IS_AFFECTED(ch, AFF_FOCUS))
   {
      send_to_char("But you are already focusing!  Why waste it?\r\n", ch);
      return eFAILURE;
   }

   send_to_char("You enter a trance and find yourself able to concentrate much better.\r\n",ch);

   struct affected_type af;
   af.type = SKILL_INNATE_FOCUS;
   af.duration = 4;
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bitvector = AFF_FOCUS;
   affect_to_char(ch, &af);

   return eSUCCESS;   
}

int innate_illusion(Character *ch, char *arg, int cmd)
{
  if (IS_AFFECTED(ch, AFF_INVISIBLE))
  {
     send_to_char("But you're already invisible!\r\n",ch);
    return eFAILURE;
  }
   struct affected_type af;
   af.type = SKILL_INNATE_ILLUSION;
   af.duration = 4;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = AFF_ILLUSION;
   affect_to_char(ch, &af);
   af.type = SPELL_INVISIBLE;
   af.bitvector = AFF_INVISIBLE;
   affect_to_char(ch, &af);
   send_to_char("You use your race's innate illusion powers, and fade out of existence.\r\n",ch);
   act("$n chants something incoherent and fades out of existence.", ch, nullptr, nullptr, TO_ROOM, 0);
   return eSUCCESS;
}

int innate_bloodlust(Character *ch, char *arg, int cmd)
{
  if (!ch->fighting)
  {
    send_to_char("You need to be fighting to use that.\r\n",ch);
    return eFAILURE;
  }
  SET_BIT(ch->combat, COMBAT_ORC_BLOODLUST1);
  send_to_char("Your blood boils as you drive yourself into a war-like state.\r\n",ch);
  act("$n's blood boils has $e drives $mself into warlike rage.",ch,nullptr,nullptr, TO_ROOM, 0);
  return eSUCCESS;
}

int innate_repair(Character *ch, char *arg, int cmd)
{
  class Object *obj;
  char buf[MAX_STRING_LENGTH];
  int i, chance = 60-GET_LEVEL(ch);
  bool found = false;
  arg = one_argument(arg,buf);
  if ( ( obj = get_obj_in_list_vis( ch, buf, ch->carrying ) ) == nullptr )
  {
    send_to_char("You are not carrying anything like that.\r\n",ch);
    return eFAILURE;
  }
  if (GET_LEVEL(ch) < obj->obj_flags.eq_level)
  {
   send_to_char("This item is beyond your skill.\r\n",ch);
   return eFAILURE;
  }
  if (IS_OBJ_STAT(obj, ITEM_NOREPAIR))
  {
    send_to_char("This item is unrepairable.\r\n",ch);
    return eFAILURE;
  }
  for (i = 0; i < obj->num_affects;i++)
  {
    if (obj->affected[i].location == APPLY_DAMAGED)
    {
       if (number(1,101) < chance)
       {
	  send_to_char("You failed to repair it!\r\n",ch);
	  act("$n fails to repair $p.",ch,obj,obj,TO_ROOM,0);
	  return eSUCCESS;
       }
       found = true;
       obj->num_affects--;
    } else if (found) {
       obj->affected[i-1] = obj->affected[i];
    }
  }
  if (found) { act("Your knowledge of weapons and armour allow you to quickly repair $p.",ch,obj,obj,TO_CHAR, 0); act("$n quickly repairs their $p.", ch, obj, obj, TO_ROOM, 0); return eSUCCESS; }
  else { send_to_char("That item is already in excellent condition!\r\n",ch); return eFAILURE; }
  
}

int innate_evasion(Character *ch, char *arg, int cmd)
{
   struct affected_type af;
   af.type = SKILL_INNATE_EVASION;
   af.duration = 4;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = -1;
   affect_to_char(ch, &af);
   send_to_char("You bring up an aura, blocking all forms of scrying your location.\r\n",ch);
   return eSUCCESS;
}

int innate_shadowslip(Character *ch, char *arg, int cmd)
{
   struct affected_type af;
   af.type = SKILL_INNATE_SHADOWSLIP;
   af.duration = 4;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = AFF_SHADOWSLIP;
   affect_to_char(ch, &af);
   send_to_char("You blend with the shadows, preventing people from reaching you magically.\r\n",ch);
   return eSUCCESS;
}

int innate_fly(Character *ch, char *arg, int cmd)
{
  if (affected_by_spell(ch, SKILL_INNATE_FLY)) {
    affect_from_char(ch, SKILL_INNATE_FLY);
    send_to_char("You fold your wings smoothly behind you and settle gently to the ground.\r\n", ch);
   act("$n folds $s wings smoothly behind $m and settles gently to the ground.", ch, nullptr, nullptr, TO_ROOM, 0);
  } else {
    if (ISSET(ch->affected_by, AFF_FLYING)) {
      send_to_char("You are already flying.\r\n", ch);
      return eFAILURE;
    }

   struct affected_type af;
   af.type = SKILL_INNATE_FLY;
   af.duration = -1;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = AFF_FLYING;
   affect_to_char(ch, &af);
   send_to_char("You spread your delicate wings and lift lightly into the air.\r\n", ch);
   act("$n spreads $s delicate wings and lifts lightly into the air.", ch, nullptr, nullptr, TO_ROOM, 0);
  }

  return eSUCCESS;
}
