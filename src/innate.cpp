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
DO_FUN innate_farsight;
DO_FUN innate_repair;
DO_FUN innate_sneak;
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
  { "farsight", RACE_ELVEN, innate_farsight },
  { "bloodlust", RACE_ORC, innate_bloodlust},
  { "repair", RACE_DWARVEN, innate_repair},
  { "sneak", RACE_GNOME, innate_sneak},
  { "evasion", RACE_PIXIE, innate_evasion},
  { "shadowslip", RACE_HOBBIT, innate_shadowslip},
  { "\n", 0, NULL}
};

char * innate_skills[] = 
{
   "innate powerwield timer",
   "innate sneak timer",
   "innate regeneration timer",
   "innate bloodlust timer",
   "innate farsight timer",
   "innate evasion timer",
   "innate shadowslip timer",
   "innate repair timer",
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
      if (!*arg)
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
	   af.duration = 24;
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
   send_to_char("Your wounds heal faster as you activate your trollish regeneration.\r\n",ch);
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
   send_to_char("Your body surges with energy. You can now wield two handed weapons with one hand.\r\n",ch);
   return eSUCCESS;
}

int innate_sneak(CHAR_DATA *ch, char *arg, int cmd)
{
   if(IS_AFFECTED(ch, AFF_SNEAK))
   {
      send_to_char("But you are already sneaking!  Why waste it?\r\n", ch);
      return eFAILURE;
   }

   send_to_char("You begin sneaking.\r\n", ch);

   struct affected_type af;
   af.type = SKILL_SNEAK;
   af.duration = 4;
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bitvector = AFF_SNEAK;
   affect_to_char(ch, &af);

   return eSUCCESS;   
}

int innate_farsight(CHAR_DATA *ch, char *arg, int cmd)
{
   struct affected_type af;
   af.type = SKILL_INNATE_FARSIGHT;
   af.duration = 6;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = AFF_FARSIGHT;
   affect_to_char(ch, &af);
   send_to_char("Your eyes glow green from your innate racial sight.\r\n",ch);
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
  send_to_char("You scream, raging in bloodlust.\r\n",ch);
  return eSUCCESS;
}

int innate_repair(CHAR_DATA *ch, char *arg, int cmd)
{
  struct obj_data *obj;
  int i;
  bool found = FALSE;
  if ( ( obj = get_obj_in_list_vis( ch, arg, ch->carrying ) ) == NULL )
  {
    send_to_char("You are not carrying anything like that.\r\n",ch);
    return eFAILURE;
  }
  for (i = 0; i < obj->num_affects;i++)
  {
    if (obj->affected[i].location == APPLY_DAMAGED)
    {
       found = TRUE;
       obj->num_affects--;
    } else if (found) {
       obj->affected[i-1] = obj->affected[i];
    }
  }
  if (found) return eSUCCESS;
  else { send_to_char("You don't have that item.\r\n",ch); return eFAILURE; }
  
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

