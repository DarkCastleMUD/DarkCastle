// This file takes care of all innate race abilities

#include <innate.h>
#include <race.h>
#include <db.h>
#include <fight.h>
#include <room.h>
#include <obj.h>
#include <connect.h>
#include <utility.h>
#include <character.h>
#include <handler.h>
#include <db.h>
#include <player.h>
#include <levels.h>
#include <interp.h>
#include <magic.h>
#include <act.h>
#include <mobile.h>
#include <spells.h>
#include <string.h> // strstr()
#include <returnvals.h>
#include <interp.h>

#include <vector>
#include <algorithm>
using namespace std;

////////////////////////////////////////////////////////////////////////////
// external vars
extern CWorld world;
extern struct index_data *obj_index; 

////////////////////////////////////////////////////////////////////////////
// external functs

////////////////////////////////////////////////////////////////////////////
// local function declarations


DO_FUN innate_powerwield;
DO_FUN innate_regeneration;
DO_FUN innate_illusion;
DO_FUN innate_repair;
DO_FUN innate_focus;
DO_FUN innate_evasion;
DO_FUN innate_shadowslip;
DO_FUN innate_bloodlust;

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
  { "\n", 0, NULL}
};

char * innate_skills[] = 
{
   "Powerwield",
   "Focus",
   "Regeneration",
   "Bloodlust",
   "Illusion",
   "Evasion",
   "Shadowslip",
   "!repairr!",
   "innate skill timer",
   "\n"
};

////////////////////////////////////////////////////////////////////////////
// command functions
int do_innate(CHAR_DATA *ch, char *arg, int cmd)
{
  int i;
  char buf[512];
  arg = one_argument(arg,buf);
  for (i = 0; *innates[i].name != '\n';i++)
  {
    if (innates[i].race == GET_RACE(ch))
    {
      if (buf[0] == '\0')
      {
        csendf(ch, "Your race has access to the %s innate ability.\r\n",innates[i].name);
	return eSUCCESS;
      } else if (!str_cmp(innates[i].name,buf)) {
	if (affected_by_spell(ch,SKILL_INNATE_TIMER))
        {
	  send_to_char("You cannot use that yet.\r\n",ch);
	  return eFAILURE;
	}
	int retval = (*(innates[i].func))(ch,arg,cmd);
	if (retval & eSUCCESS)
	{
	   struct affected_type af;
	   af.type = SKILL_INNATE_TIMER;
	   af.duration = !str_cmp(buf, "repair")?16:24;
		// repair is every 16 ticks, 'cause Apoc's annoying.
	   af.modifier = 0;
	   af.location = 0;
	   af.bitvector = 0;
	   affect_to_char(ch, &af);
	}
	return retval;
      } else {
	send_to_char("You do not have access to any such ability.\r\n",ch);
	return eFAILURE;
      }
    }
  }
  send_to_char("Your race has no innate abilities.\r\n",ch);
  return eFAILURE;
}

int innate_regeneration(CHAR_DATA *ch, char *arg, int cmd)
{
   struct affected_type af;
   af.type = SKILL_INNATE_REGENERATION;
   af.duration = 4;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = AFF_REGENERATION;
   affect_to_char(ch, &af);
   send_to_char("Your innate regenerative abilities allow you to heal quickly.\r\n",ch);
   return eSUCCESS;
}

int innate_powerwield(CHAR_DATA *ch, char *arg, int cmd)
{
   struct affected_type af;
   af.type = SKILL_INNATE_POWERWIELD;
   af.duration = 2;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = AFF_POWERWIELD;
   affect_to_char(ch, &af);
   send_to_char("You gather your energy in an effort to wield two mighty weapons.\r\n",ch);
   act("$n gathers his strength in order to wield two mighty weapons.",ch, NULL,NULL,TO_ROOM,0);
   return eSUCCESS;
}

int innate_focus(CHAR_DATA *ch, char *arg, int cmd)
{
   if(IS_AFFECTED2(ch, AFF_FOCUS))
   {
      send_to_char("But you are already focusing!  Why waste it?\r\n", ch);
      return eFAILURE;
   }

   send_to_char("You delve into a trance and find yourself able to concentrate alot better.\r\n",ch);

   struct affected_type af;
   af.type = SKILL_INNATE_FOCUS;
   af.duration = 1;
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bitvector = AFF_FOCUS;
   affect_to_char(ch, &af);

   return eSUCCESS;   
}

int innate_illusion(CHAR_DATA *ch, char *arg, int cmd)
{
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
   send_to_char("You use your race's innate illusion powers, and fade out of existance.\r\n",ch);
   act("$n chants something incoherent and fades out of existance.", ch, NULL, NULL, TO_ROOM, 0);
   return eSUCCESS;
}

int innate_bloodlust(CHAR_DATA *ch, char *arg, int cmd)
{
  if (!ch->fighting)
  {
    send_to_char("You need to be fighting to use that.\r\n",ch);
    return eFAILURE;
  }
  SET_BIT(ch->combat, COMBAT_ORC_BLOODLUST1);
  send_to_char("Your blood boils as you drive yourself into a war-like state.\r\n",ch);
  act("$n's blood boils has $e drives $mself into warlike rage.",ch,NULL,NULL, TO_ROOM, 0);
  return eSUCCESS;
}

int innate_repair(CHAR_DATA *ch, char *arg, int cmd)
{
  struct obj_data *obj;
  char buf[MAX_STRING_LENGTH];
  int i, chance = 60-GET_LEVEL(ch);
  bool found = FALSE;
  arg = one_argument(arg,buf);
  if ( ( obj = get_obj_in_list_vis( ch, buf, ch->carrying ) ) == NULL )
  {
    send_to_char("You are not carrying anything like that.\r\n",ch);
    return eFAILURE;
  }
  if (GET_LEVEL(ch) < obj->obj_flags.eq_level)
  {
   send_to_char("This item is beyond your skill.\r\n",ch);
   return eFAILURE;
  }
  for (i = 0; i < obj->num_affects;i++)
  {
    if (obj->affected[i].location == APPLY_DAMAGED)
    {
       if (number(1,101) < chance)
       {
	  send_to_char("You failed to repair it!\r\n",ch);
	  act("$n fails to repair their $p.",ch,obj,obj,TO_ROOM,0);
	  return eSUCCESS;
       }
       found = TRUE;
       obj->num_affects--;
    } else if (found) {
       obj->affected[i-1] = obj->affected[i];
    }
  }
  if (found) { act("Your knowledge of weapons and armour allow you to quickly repair $p.",ch,obj,obj,TO_CHAR, 0); act("$n quickly repairs their $p.", ch, obj, obj, TO_ROOM, 0); return eSUCCESS; }
  else { send_to_char("That item is already in excellent condition!\r\n",ch); return eFAILURE; }
  
}

int innate_evasion(CHAR_DATA *ch, char *arg, int cmd)
{
   struct affected_type af;
   af.type = SKILL_INNATE_EVASION;
   af.duration = 2;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = 0;
   affect_to_char(ch, &af);
   send_to_char("You bring up an aura, blocking all forms of scrying your location.\r\n",ch);
   return eSUCCESS;
}

int innate_shadowslip(CHAR_DATA *ch, char *arg, int cmd)
{
   struct affected_type af;
   af.type = SKILL_INNATE_SHADOWSLIP;
   af.duration = 3;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = AFF_SHADOWSLIP;
   affect_to_char(ch, &af);
   send_to_char("You bring up an aura, blocking all forms of scrying your location.\r\n",ch);
   return eSUCCESS;
}

