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
/* $Id: handler.cpp,v 1.111 2006/06/14 05:49:55 shane Exp $ */
    
extern "C"
{
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
}

#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif


#include <magic.h>
#include <spells.h>
#include <room.h>
#include <obj.h>
#include <character.h>
#include <player.h> // APPLY
#include <utility.h> // LOWER
#include <clan.h>
#include <levels.h>
#include <db.h>
#include <mobile.h>
#include <connect.h> // descriptor_data
#include <handler.h> // FIND_CHAR_ROOM
#include <interp.h> // do_return
#include <machine.h> // index
#include <act.h>
#include <isr.h>
#include <race.h>
#include <fight.h>
#include <returnvals.h>
#include <innate.h>
#include <set.h>

extern CWorld world;
 
extern struct obj_data  *object_list;
extern CHAR_DATA *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct active_object active_head;
extern struct zone_data *zone_table;


#ifdef WIN32
int strncasecmp(char *s1, const char *s2, int len);
#endif

/* External procedures */
void save_corpses(void);
int str_cmp(char *arg1, char *arg2);
void remove_follower(CHAR_DATA *ch);
int do_fall(CHAR_DATA *ch, short dir);

/* internal procedures */
void remove_memory(CHAR_DATA *ch, char type);
void add_memory(CHAR_DATA *ch, char *victim, char type);

//TIMERS

bool isTimer(CHAR_DATA *ch, int spell)
{
  return affected_by_spell(ch, BASE_TIMERS+spell);
}
void addTimer(CHAR_DATA *ch, int spell, int ticks)
{

  struct affected_type af;
  af.duration = ticks;
  af.type = spell+BASE_TIMERS;
  af.location = 0;
  af.bitvector = -1;
  af.modifier = 0;
  affect_join(ch, &af, TRUE, FALSE);
  return;
}
//END TIMERS

bool is_wearing(CHAR_DATA *ch, OBJ_DATA *item)
{
  int i;
  for (i = 0; i < MAX_WEAR; i++)
     if (ch->equipment[i] == item) return TRUE;
  return FALSE;
}


// This grabs the first "word" (defined as group of alphaBETIC chars)
// puts it in a static char buffer, and returns it.
char *fname(char *namelist)
{
    static char holder[30];
    char *point;

    for (point = holder; isalpha(*namelist); namelist++, point++)
	*point = *namelist;

    // we seem to use this alot, and 30 chars isn't a whole lot.....
    // let's just put this here for the heck of it -pir 2/28/01
    if(point > (holder + 29))
      log("point overran holder in fname (handler.c)", ANGEL, LOG_BUG);

    *point = '\0';

    return(holder);
}

// TODO - figure out how this is different from isname() and comment why
// we need it.  Neither of them are case-sensitive....
int isname2(char *str, char *namel)
{
   char* s = namel;

   if (strlen(str) == 0)
      return 0;

   for (;;)
   {
      if (!strncasecmp(str, s, strlen(str)))
         return 1;

  	   for (; isalpha(*s); s++);
  	   if (!*s)
         return(0);
      s++;          /* first char of new_new name */
   }
}

/************************************************************************
| isname
| Preconditions:  str != 0, namelist != 0
| Postconditions: None
| Side Effects: None
| Returns: One if it's in the namelist, zero otherwise
*/
int isname(char *str, char *namelist)
{
   char *curname, *curstr;

   if(!str || !namelist) {
     return(0);
   }

   if (strlen(str) == 0)
      return 0;
   if(strlen(namelist) == 0)
     return 0;

   curname = namelist;    
   
   for (;;)
      {
	   for (curstr = str;; curstr++, curname++)
	      {
	      if (!*curstr && !isalpha(*curname))
		      return(1);

	      if (!*curname)
		      return(0);

	      if (!*curstr || *curname == ' ')
		      break;

	      if (LOWER(*curstr) != LOWER(*curname))
		      break;
	      }

	   /* skip to next name */

	   for (; isalpha(*curname); curname++);
	   if (!*curname)
		   return(0);
	   curname++;          /* first char of new_new name */
      }
}

int isname_exact(char *str, char *namelist)
{
   char *curname, *curstr;

   if (strlen(str) == 0)
      return 0;


   curname = namelist;    
   
   for (;;)
      {
	   for (curstr = str;; curstr++, curname++)
	      {
	      if (!*curstr && !isalpha(*curname))
		      return(1);

	      if (!*curname)
		      return(0);

	      if (!*curstr || *curname == ' ')
		      break;

	      if (LOWER(*curstr) != LOWER(*curname))
		      break;
	      }

	   /* skip to next name */

	   for (; isalpha(*curname); curname++);
	   if (!*curname)
		   return(0);
	   curname++;          /* first char of new_new name */
      }
}

int get_max_stat(char_data * ch, ubyte stat)
{
    switch(GET_RACE(ch)) {
      case RACE_ELVEN:
        switch(stat) {
          case STRENGTH:      return 24;
          case DEXTERITY:     return 27;
          case INTELLIGENCE:  return 27;
          case WISDOM:        return 24;
          case CONSTITUTION:  return 23;
        }

    case RACE_TROLL:
	switch(stat) {
	  case STRENGTH:     return 28;
	  case DEXTERITY:    return 25;
	  case INTELLIGENCE: return 20;
	  case WISDOM: 	     return 22;
	  case CONSTITUTION: return 30;
	}

  case RACE_DWARVEN:
        switch(stat) {
          case STRENGTH:      return 27;
          case DEXTERITY:     return 22;
          case INTELLIGENCE:  return 22;
          case WISDOM:        return 26;
          case CONSTITUTION:  return 28;
        }

      case RACE_HOBBIT:
        switch(stat) {
          case STRENGTH:      return 22;
          case DEXTERITY:     return 30;
          case INTELLIGENCE:  return 25;
          case WISDOM:        return 25;
          case CONSTITUTION:  return 23;
        }

      case RACE_PIXIE:
        switch(stat) {
          case STRENGTH:      return 20;
          case DEXTERITY:     return 28;
          case INTELLIGENCE:  return 30;
          case WISDOM:        return 27;
          case CONSTITUTION:  return 20;
        }

      case RACE_GIANT:
        switch(stat) {
          case STRENGTH:      return 30;
          case DEXTERITY:     return 23;
          case INTELLIGENCE:  return 22;
          case WISDOM:        return 23;
          case CONSTITUTION:  return 27;
        }

      case RACE_GNOME:
        switch(stat) {
          case STRENGTH:      return 22;
          case DEXTERITY:     return 22;
          case INTELLIGENCE:  return 27;
          case WISDOM:        return 30;
          case CONSTITUTION:  return 24;
        }

      case RACE_ORC:
        switch(stat) {
          case STRENGTH:      return 27;
          case DEXTERITY:     return 25;
          case INTELLIGENCE:  return 24;
          case WISDOM:        return 23;
          case CONSTITUTION:  return 26;
        }

      case RACE_HUMAN:
      default:
        return 25;
    }
}

bool still_affected_by_poison(CHAR_DATA * ch)
{
  struct affected_type * af = ch->affected;

  while(af) {
    if(IS_SET(af->bitvector, AFF_POISON))
      return 1;
    af = af->next;
  }
  return 0;
}

const struct set_data set_list[] = {
  { "Ascetic's Focus", 19, { 2700, 6904, 8301, 8301, 9567, 9567, 12108, 14805, 15621, 
	21718, 22302, 22314, 22600, 22601, 22602, 24815, 24815, 24816, 26237 },
	"You attach your penis mightier.\r\n",
	"You remove your penis mightier.\r\n"},
  { "Warlock's Vestments", 7, {17334,17335,17336,17337,17338,17339,17340,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
	"You feel an increase in energy coursing through your body.\r\n",
	"Your magical energy returns to its normal state.\r\n"},
  { "Hunter's Arsenal", 7, {17327,17328,17329,17330,17331,17332,17333,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
        "You sense your accuracy and endurance have undergone a magical improvement.\r\n",
	"Your accuracy and endurance return to their normal levels.\r\n"},
  { "Captain's Regalia", 7, {17341,17342,17343,17344,17345,17346,17347,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
        "You feel an increased vigor surge throughout your body.\n",
	"Your vigor is reduced to its normal state.\n"},
  { "Celebrant's Defenses", 7, {17319,17320,17321,17322,17323,17324,17325,17326,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
        "Your inner focus feels as though it is more powerful.\r\n",
	"Your inner focus has reverted to its original form.\r\n"},
  { "Battlerager's Fury", 18, {352, 353, 354, 355, 356, 357, 358, 359, 359,
	360, 361, 362, 362, 9702, 9808, 9808, 27114, 27114, -1},
	"You feel your stance harden and blood boil as you strap on your battlerager's gear.\r\n",
	"Your blood returns to its normal temperature as you remove your battlerager's gear.\r\n"},
  { "Veteran's Field Plate Armour", 9, {21719, 21720, 21721, 21722, 21723, 21724, 21725, 21726, 21727,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 	"There is an audible *click* as the field plate locks into its optimal assembly.\r\n",
	"There is a soft *click* as you remove the field plate from its optimal positioning.\r\n"},
  { "Mother of All Dragons' Trophies", 7, {22320, 22321, 22322, 22323, 22324, 22325, 22326, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1},
	"You feel the might of the ancient dragonkind surge through your body.\r\n",
	"The might of the ancient dragonkind has left you.\r\n"},
  { "Feral Fangs", 2, {4818, 4819,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
        "As the fangs' cries harmonize they unleash something feral inside you.\r\n",
        "As you remove the fangs your muscles relax and focus is restored.\r\n"},
  { "White Crystal Armours", 10, {22003,22006,22010,22014,22015,22020,22021,22022,22023,22024,
        -1,-1,-1,-1,-1,-1,-1,-1,-1},
        "Your crystal armours begin to $B$7glow$R with an ethereal light.\r\n",
        "The $B$7glow$R fades from your crystal armours.\r\n"},
  { "Black Crystal Armours", 7, {22011,22017,22025,22026,22027,22028,22029,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
        "Your crystal armours $B$0darken$R and begin to hum with magic.\r\n",
        "The $B$0darkness$R disperses as the crystal armours are removed.\r\n"},

  { "Aqua Pendants", 2, {5611,5643,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
        "The pendants click softly and you feel a surge of energy as gills spring from your neck!\r\n",
        "Your gills retract and fade as the two pendants separate quietly.\r\n"}, 
  { "\n", 0, {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
	"\n","\n"}
};

void add_set_stats(char_data *ch, obj_data *obj, int flag, int pos)
{
  // obj has just been worn
  int obj_vnum = obj_index[obj->item_number].virt;
  int i;
  int z = 0, y;
  // Quadruply nested for. Annoying, but it's gotta be done.
  // I'm sure "quadruply" is a word.
  for (; *(set_list[z].SetName) != '\n'; z++)
   for (y = 0; y < 19 && set_list[z].vnum[y] != -1; y++)
    if (set_list[z].vnum[y] == obj_vnum)
    {  // Aye, 'tis part of a set.
	if ((obj_vnum == 4818 || obj_vnum == 4819) && pos == WEAR_HANDS)
	continue;
	for (y = 0; y < 19 && set_list[z].vnum[y] != -1;y++)
	{
	  if (set_list[z].vnum[y] == 17326 && GET_CLASS(ch) != CLASS_BARD) continue;
	  if (set_list[z].vnum[y] == 17325 && GET_CLASS(ch) != CLASS_MONK) continue;
	  bool found = FALSE,doublea = FALSE;
	  for (i = 0; i < MAX_WEAR; i++)
   	  {
            if (ch->equipment[i] && obj_index[ch->equipment[i]->item_number].virt == set_list[z].vnum[y])
		{
		  if (y>0 && !doublea && set_list[z].vnum[y] == set_list[z].vnum[y-1])
		  { 
			doublea = TRUE; continue;
		  }
		found = TRUE; break; 
		}
          }
	  if (!found){ return; }// Nope.
        }
	struct affected_type af;
	af.duration = -1;
	af.bitvector = -1;
	af.type = BASE_SETS + z;
	af.location = APPLY_NONE;
	af.modifier = 0;
        // By gawd, they just completed the set.
	if (!flag)
	send_to_char(set_list[z].Set_Wear_Message, ch);
	switch (z)
	{
	  case SET_SAIYAN: // (aka Ascetic's Focus)
	    af.bitvector = AFF_HASTE;
	    affect_to_char(ch,&af);
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
	    affect_to_char(ch,&af);
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
	    af.location = SKILL_BLOOD_FURY *1000;
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
	    af.modifier = -5;
	    affect_to_char(ch, &af);
	    af.location = APPLY_SONG_DAMAGE;
	    af.modifier = -5;
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
	  default: 
		send_to_char("Tough luck, you completed an unimplemented set. Report what you just wore, eh?\r\n",ch);
		break;
	}
	break;
    }
}

void remove_set_stats(char_data *ch, obj_data *obj, int flag)
{
  // obj has just been removed
  int obj_vnum = obj_index[obj->item_number].virt;
  int i;
  int z = 0, y;
  // Quadruply nested for. Annoying, but it's gotta be done.
  // I'm sure "quadruply" is a word.
  for (; *(set_list[z].SetName) != '\n'; z++)
   for (y = 0; y < 17 && set_list[z].vnum[y] != -1; y++)
    if (set_list[z].vnum[y] == obj_vnum)
    {  // Aye, 'tis part of a set.
        for (y= 0; y < 19 && set_list[z].vnum[y] != -1;y++)
         {
         if (set_list[z].vnum[y] == 17326 && GET_CLASS(ch) != CLASS_BARD) continue;
          if (set_list[z].vnum[y] == 17325 && GET_CLASS(ch) != CLASS_MONK) continue;
           bool found = FALSE,doublea = FALSE;
          for (i = 0; i < MAX_WEAR; i++)
          {
            if (ch->equipment[i] && obj_index[ch->equipment[i]->item_number].virt == set_list[z].vnum[y])
                {
                  if (y>0 && !doublea && set_list[z].vnum[y] == set_list[z].vnum[y-1])
                  {
                        doublea = TRUE; continue;
                  }
                found = TRUE; break;
                }
          }
          if (!found) return; // Nope.
        }
        //Remove it
	affect_from_char(ch, BASE_SETS+z);
	if (!flag)
        send_to_char(set_list[z].Set_Remove_Message, ch);
        break;
    }
}

void check_weapon_weights(char_data * ch)
{
  struct obj_data * weapon;
  
  if (ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT)) return;
  // make sure we're still strong enough to wield our weapons
  if(!IS_MOB(ch) && ch->equipment[WIELD] &&
       GET_OBJ_WEIGHT(ch->equipment[WIELD]) > GET_STR(ch) && !ISSET(ch->affected_by, AFF_POWERWIELD))
  {
    act("Being too heavy to wield, you move your $p to your inventory.",
         ch, ch->equipment[WIELD], 0, TO_CHAR, 0);
    act("$n stops using $p.", ch, ch->equipment[WIELD], 0, TO_ROOM, INVIS_NULL);
    obj_to_char(unequip_char(ch, WIELD), ch);
    if(ch->equipment[SECOND_WIELD])
    {
      act("You move your $p to be your primary weapon.", ch, ch->equipment[SECOND_WIELD], 0, TO_CHAR, INVIS_NULL);
      act("$n moves $s $p to be $s primary weapon.", ch, ch->equipment[SECOND_WIELD], 0, TO_ROOM, INVIS_NULL);
      weapon = unequip_char(ch, SECOND_WIELD);
      equip_char(ch, weapon, WIELD);
      check_weapon_weights(ch); // Not a loop, since it'll only happen once.
				// It'll recheck primary wield.
    }
  }

  if(ch->equipment[SECOND_WIELD] &&
       GET_OBJ_WEIGHT(ch->equipment[SECOND_WIELD]) > GET_STR(ch)/2 && !ISSET(ch->affected_by,AFF_POWERWIELD))
  {
    act("Being too heavy to wield, you move your $p to your inventory.",
         ch, ch->equipment[SECOND_WIELD], 0, TO_CHAR, 0);
    act("$n stops using $p.", ch, ch->equipment[SECOND_WIELD], 0, TO_ROOM, INVIS_NULL);
    obj_to_char(unequip_char(ch, SECOND_WIELD), ch);
  }


  if (ch->equipment[WEAR_SHIELD] && 
((ch->equipment[WIELD] && 
GET_OBJ_WEIGHT(ch->equipment[WIELD]) > GET_STR(ch) 
&& !ISSET(ch->affected_by, AFF_POWERWIELD)) 
|| (ch->equipment[SECOND_WIELD] && 
GET_OBJ_WEIGHT(ch->equipment[SECOND_WIELD]) > GET_STR(ch)/2 
&&!ISSET(ch->affected_by, AFF_POWERWIELD) ) ))
  {
    act("You shift your shield into your inventory.",
         ch, ch->equipment[WEAR_SHIELD], 0, TO_CHAR, 0);
    act("$n stops using $p.", ch, ch->equipment[WEAR_SHIELD], 0, TO_ROOM, INVIS_NULL);
    obj_to_char(unequip_char(ch, WEAR_SHIELD), ch);
  }

}

void affect_modify(CHAR_DATA *ch, int32 loc, int32 mod, long bitv, bool add)
{
   char log_buf[256];
   int i;
    
   if (loc >= 1000) return;
   if (bitv != -1 && bitv < AFF_MAX) {
   if(add)
      SETBIT(ch->affected_by, bitv);
   else {
      REMBIT(ch->affected_by, bitv);
   }
   }
 if (!add)
      mod = -mod;

   switch(loc)
   {
      case APPLY_NONE:
         break;

      case APPLY_STR: {
            GET_STR_BONUS(ch) += mod;
            GET_STR(ch) = GET_RAW_STR(ch) + GET_STR_BONUS(ch);
            i = get_max_stat(ch, STRENGTH);
            if(i <= GET_RAW_STR(ch))
               GET_STR(ch) = MIN(30, GET_STR(ch));
            else
               GET_STR(ch) = MIN(i, GET_STR(ch));

      //      // can only be max naturally.  Eq only gets you to max - 2
        //    if( GET_RAW_STR(ch) > i - 2 && GET_STR(ch) > i - 2 )
          //     GET_STR(ch) = GET_RAW_STR(ch);
//            GET_STR(ch) = MAX( 1, MIN( (int)GET_STR( ch ), i) ) );
            if(!ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT))
               check_weapon_weights(ch);
        }  break;

	case APPLY_DEX: {
            GET_DEX_BONUS(ch) += mod;
            GET_DEX(ch) = GET_RAW_DEX(ch) + GET_DEX_BONUS(ch);
            i = get_max_stat(ch, DEXTERITY);
            if(i <= GET_RAW_DEX(ch))
               GET_DEX(ch) = MIN(30, GET_DEX(ch));
            else
		GET_DEX(ch) = MIN(i, GET_DEX(ch));
  //          // can only be max naturally.  Eq only gets you to max - 2
    //        if( GET_RAW_DEX(ch) > i - 2 && GET_DEX(ch) > i - 2 )
      //         GET_DEX(ch) = GET_RAW_DEX(ch);
        //    else GET_DEX(ch) = MAX( 1, MIN( (int)GET_DEX( ch ), ( i - 2 
//) ) );
	}  break;

	case APPLY_INT: {
            GET_INT_BONUS(ch) += mod;
            GET_INT(ch) = GET_RAW_INT(ch) + GET_INT_BONUS(ch);
            i = get_max_stat(ch, INTELLIGENCE);
            if(i <= GET_RAW_INT(ch))
               GET_INT(ch) = MIN(30, GET_INT(ch));
            else
	       GET_INT(ch) = MIN(i, GET_INT(ch));
            // can only be max naturally.  Eq only gets you to max - 2
           // if( GET_RAW_INT(ch) > i - 2 && GET_INT(ch) > i - 2 )
            //   GET_INT(ch) = GET_RAW_INT(ch);
           // else GET_INT(ch) = MAX( 1, MIN( (int)GET_INT( ch ), ( i - 2 
//) ) );
        } break;

	case APPLY_WIS: {
            GET_WIS_BONUS(ch) += mod;
            GET_WIS(ch) = GET_RAW_WIS(ch) + GET_WIS_BONUS(ch);
            i = get_max_stat(ch, WISDOM);
            if(i <= GET_RAW_WIS(ch))
               GET_WIS(ch) = MIN(30, GET_WIS(ch));
            else
	       GET_WIS(ch) = MIN(i, GET_WIS(ch));
  //          // can only be max naturally.  Eq only gets you to max - 2
    //        if( GET_RAW_WIS(ch) > i - 2 && GET_WIS(ch) > i - 2 )
      //         GET_WIS(ch) = GET_RAW_WIS(ch);
        //    else GET_WIS(ch) = MAX( 1, MIN( (int)GET_WIS( ch ), ( i - 2 
//) ) );
        } break;

	case APPLY_CON: {
            GET_CON_BONUS(ch) += mod;
            GET_CON(ch) = GET_RAW_CON(ch) + GET_CON_BONUS(ch);
            i = get_max_stat(ch, CONSTITUTION);
            if(i <= GET_RAW_CON(ch))
               GET_CON(ch) = MIN(30, GET_CON(ch));
            else
	       GET_CON(ch) = MIN(i, GET_CON(ch));
            // can only be max naturally.  Eq only gets you to max - 2
    //        if( GET_RAW_CON(ch) > i - 2 && GET_CON(ch) > i - 2 )
      //         GET_CON(ch) = GET_RAW_CON(ch);
        //    else GET_CON(ch) = MAX( 1, MIN( (int)GET_CON( ch ), ( i - 2 
//) ) );
	}   break;

	case APPLY_SEX: {
            switch(GET_SEX(ch)) {
              case SEX_MALE:    GET_SEX(ch) = SEX_FEMALE;   break;
              case SEX_FEMALE:  GET_SEX(ch) = SEX_MALE;     break;
              default: break;
            }
	 }   break;

	case APPLY_CLASS:
	    /* ??? GET_CLASS(ch) += mod; */
	    break;

	case APPLY_LEVEL:
	    /* ??? GET_LEVEL(ch) += mod; */
	    break;

	case APPLY_AGE:
            if(!IS_MOB(ch))
	      ch->pcdata->time.birth -= ((long)SECS_PER_MUD_YEAR*(long)mod); 
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
            
        case APPLY_HIT_N_DAM: {
            GET_DAMROLL(ch) += mod;
            GET_HITROLL(ch) += mod;
              }  break;
    
        case APPLY_SANCTUARY: {
               if (add) {
                     SETBIT(ch->affected_by, AFF_SANCTUARY);
               } else {
                  REMBIT(ch->affected_by, AFF_SANCTUARY);
               }
          } break;
        
        case APPLY_SENSE_LIFE: {
            if (add) {
                     SETBIT(ch->affected_by, AFF_SENSE_LIFE);
            } else {
                  REMBIT(ch->affected_by, AFF_SENSE_LIFE);
            }
          } break;
    
        case APPLY_DETECT_INVIS: {
               if (add) {
                     SETBIT(ch->affected_by, AFF_DETECT_INVISIBLE);
            } else {
                    REMBIT(ch->affected_by, AFF_DETECT_INVISIBLE);

             }
          } break;
    
        case APPLY_INVISIBLE: {
               if (add) {
                     SETBIT(ch->affected_by, AFF_INVISIBLE);
            } else {
                  REMBIT(ch->affected_by, AFF_INVISIBLE);
             }
          } break;
    
        case APPLY_SNEAK: {
               if (add) {
                     SETBIT(ch->affected_by, AFF_SNEAK);
            } else {
                  REMBIT(ch->affected_by, AFF_SNEAK);
             }
          } break;
    
        case APPLY_INFRARED: {
               if (add) {
                     SETBIT(ch->affected_by, AFF_INFRARED);
            } else {
                  REMBIT(ch->affected_by, AFF_INFRARED);
             }
          } break;
    
        case APPLY_HASTE: {
               if (add) {
                     SETBIT(ch->affected_by, AFF_HASTE);
            } else {
                  REMBIT(ch->affected_by, AFF_HASTE);
             }
          } break;

    
        case APPLY_PROTECT_EVIL: {
            if (add) {
                     SETBIT(ch->affected_by, AFF_PROTECT_EVIL);
            } else {
                  REMBIT(ch->affected_by, AFF_PROTECT_EVIL);
            }
          } break;

    
        case APPLY_FLY: {
               if (add) {
                     SETBIT(ch->affected_by, AFF_FLYING);
            } else {
                  REMBIT(ch->affected_by, AFF_FLYING);
             }
          } break;

        case 36: break;
        case 37: break;
	case 38: break;
	case 39: break;
	case 40: break;
	case 41: break;
	case 42: break;
	case 43: break;
	case 44: break;
	case 45: break;
	case 46: break;
	case 47: break;
	case 48: break;
	case 49: break;
	case 50: break;
	case 51: break;
	case 52: break;
        case 53: break;
        case 54: break;
        case 55: break;
        case 56: break;
        case 57: break;
        case 58: break;
        case 59: break;
        case 60: break;
        case 61: break;
        case 62: break;
        case 63:
            if(add) {
                SET_BIT(ch->resist, ISR_SLASH); 
            } else {
                REMOVE_BIT(ch->resist, ISR_SLASH); 
            }
            break;
        case 64:
            if(add) {
                SET_BIT(ch->resist, ISR_FIRE);
            } else {
                REMOVE_BIT(ch->resist, ISR_FIRE); 
            }
            break;
        case 65: 
            if(add) {
                SET_BIT(ch->resist, ISR_COLD); 
            } else {
                REMOVE_BIT(ch->resist, ISR_COLD); 
            }
            break; 

        case 66:
            ch->max_ki += mod; 
            break;

        case 67: 
            if(add) {
                SETBIT(ch->affected_by, AFF_CAMOUFLAGUE); 
            } else {
                REMBIT(ch->affected_by, AFF_CAMOUFLAGUE); 
            }
            break;
        case 68: 
            if(add) {
                SETBIT(ch->affected_by, AFF_FARSIGHT);
            } else {
                REMBIT(ch->affected_by, AFF_FARSIGHT); 
            }
            break;
        case 69: 
            if(add) {
                SETBIT(ch->affected_by, AFF_FREEFLOAT); 
            } else {
                REMBIT(ch->affected_by, AFF_FREEFLOAT); 
            }
            break;
        case 70: 
             if(add) {
                 SETBIT(ch->affected_by, AFF_FROSTSHIELD);
             } else {
                 REMBIT(ch->affected_by, AFF_FROSTSHIELD);
             }
             break;
        case 71: 
            if(add) {
                SETBIT(ch->affected_by, AFF_INSOMNIA); 
            } else {
                REMBIT(ch->affected_by, AFF_INSOMNIA); 
            }
            break;
        case 72: 
            if(add) {
                SETBIT(ch->affected_by, AFF_LIGHTNINGSHIELD);
            } else {
                REMBIT(ch->affected_by, AFF_LIGHTNINGSHIELD);
            }
            break;
        case 73: 
            if(add) {
                SETBIT(ch->affected_by, AFF_REFLECT);
            } else {
                REMBIT(ch->affected_by, AFF_REFLECT);
            }
            break;
        case 74: 
            if(add) {
                SET_BIT(ch->resist, ISR_ENERGY); 
            } else {
                REMOVE_BIT(ch->resist, ISR_ENERGY); 
            }
            break;
        case 75:
            if(add) {
                SETBIT(ch->affected_by, AFF_SHADOWSLIP); 
            } else {
                REMBIT(ch->affected_by, AFF_SHADOWSLIP); 
            }
            break;
        case 76:
            if(add) {
                SETBIT(ch->affected_by, AFF_SOLIDITY);
            } else {
                REMBIT(ch->affected_by, AFF_SOLIDITY);
            }
            break;
        case 77:
            if(add) {
                SETBIT(ch->affected_by, AFF_STABILITY);
            } else {
                REMBIT(ch->affected_by, AFF_STABILITY);
            }
            break;
        case 78: 
            if(add) {
                 SET_BIT(ch->immune, ISR_POISON); 
            } else {
                 REMOVE_BIT(ch->immune, ISR_POISON); 
            }
            break;
        case 79: break;
        case 80: break;
        case 81: break;
        case 82: break;
        case 83: break;
        case 84: break;
        case 85: break;
        case 86: break;

        case APPLY_INSANE_CHANT:
            if(add) {
                SETBIT(ch->affected_by, AFF_INSANE);
            } else {
                REMBIT(ch->affected_by, AFF_INSANE);
            }
            break;

        case APPLY_GLITTER_DUST:
            if(add) {
                SETBIT(ch->affected_by, AFF_GLITTER_DUST);
            } else {
                REMBIT(ch->affected_by, AFF_GLITTER_DUST);
            }
            break;

        case APPLY_RESIST_ACID: 
            if(add) {
                SET_BIT(ch->resist, ISR_ACID); 
            } else {
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
           if(add) SETBIT(ch->affected_by, AFF_BOUNT_SONNET_HUNGER);
           else REMBIT(ch->affected_by, AFF_BOUNT_SONNET_HUNGER);
           break;

        case APPLY_BOUNT_SONNET_THIRST:
           if(add) SETBIT(ch->affected_by, AFF_BOUNT_SONNET_THIRST);
           else REMBIT(ch->affected_by, AFF_BOUNT_SONNET_THIRST);
           break;

        case APPLY_BLIND:
           if(add) SETBIT(ch->affected_by, AFF_BLIND);
           else REMBIT(ch->affected_by, AFF_BLIND);
           break;

        case APPLY_WATER_BREATHING:
           if(add) SETBIT(ch->affected_by, AFF_WATER_BREATHING);
           else REMBIT(ch->affected_by, AFF_WATER_BREATHING);
           break;

        case APPLY_DETECT_MAGIC:
           if(add) SETBIT(ch->affected_by, AFF_DETECT_MAGIC);
           else REMBIT(ch->affected_by, AFF_DETECT_MAGIC);
           break;

	default:
          sprintf(log_buf, "Unknown apply adjust attempt: %d. (handler.c, "
                  "affect_modify.)", loc);
	  log(log_buf, 0, LOG_BUG); 
	  //	  strcpy(0,0); /* Why was this here? -Jared */
	  break;

    } /* switch */
}

// This updates a character by subtracting everything he is affected by
// then restoring it back.  It is used primarily for bitvector affects
// that could be on both eq as well as on spells.
// Stat (con, str, etc) stuff is handled in affect_modify

void affect_total(CHAR_DATA *ch)
{
    struct affected_type *af;
    struct affected_type *tmp_af;
    int i,j;
    bool already = ISSET(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT);

    SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop falling off

    // remove all affects from the player
    for(i=0; i<MAX_WEAR; i++) {
        if (ch->equipment[i])
            for(j=0; j< ch->equipment[i]->num_affects; j++)
                affect_modify(ch, ch->equipment[i]->affected[j].location,
                              ch->equipment[i]->affected[j].modifier,
                               -1, FALSE);
    }
    remove_totem_stats(ch);
    for(af = ch->affected; af; af = tmp_af)
    {
//        bool secFix = FALSE;
        tmp_af = af->next;
        affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);
    }
            
    // add everything back
    for(i=0; i<MAX_WEAR; i++) {
        if (ch->equipment[i])
            for(j=0; j<ch->equipment[i]->num_affects; j++)
                affect_modify(ch, ch->equipment[i]->affected[j].location,
                              ch->equipment[i]->affected[j].modifier,
                              -1, TRUE);
    }
    for(af = ch->affected; af; af=af->next)
    {
//      bool secFix = FALSE;
        affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
    }
    add_totem_stats(ch);
	if (!already)
    REMBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop fall off
}


/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void affect_to_char( CHAR_DATA *ch, struct affected_type *af )
{
//    bool secFix;
    struct affected_type *affected_alloc;
    if (af->location >= 1000) return; // Skill aff;
#ifdef LEAK_CHECK
    affected_alloc = (struct affected_type *) calloc(1, sizeof(struct affected_type));
#else
    affected_alloc = (struct affected_type *) dc_alloc(1, sizeof(struct affected_type));
#endif
    *affected_alloc = *af;
    affected_alloc->next = ch->affected;
    ch->affected = affected_alloc;

    affect_modify(ch, af->location, af->modifier,
		  af->bitvector, TRUE);
}



/* Remove an affected_type structure from a char (called when duration
   reaches zero). Pointer *af must never be NIL! Frees mem and calls 
   affect_location_apply                                                */
void affect_remove( CHAR_DATA *ch, struct affected_type *af, int flags)
{
    struct affected_type *hjp;
    char buf[200];
    short dir;
    bool char_died = FALSE;


   if (!ch->affected) return;

    assert(ch);
    assert(ch->affected);

    affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);


    /* remove structure *af from linked list */

    if (ch->affected == af) {
	/* remove head of list */
	ch->affected = af->next;
    } else {

      for(hjp = ch->affected; (hjp->next) &&
           (hjp->next != af); hjp = hjp->next);

      if (hjp->next != af) {
         log("FATAL : Could not locate affected_type in ch->affected. (handler.c, affect_remove)", ANGEL, LOG_BUG);
         sprintf(buf,"Problem With: %s    Affect type: %d", ch->name, af->type);
         log(buf, ANGEL, LOG_BUG);
         return;
      }
      hjp->next = af->next; /* skip the af element */
   }

   switch( af->type ) 
   {
      // Put anything here that MUST go away whenever the spell wears off
      // That isn't handled in affect_modify

      case SPELL_IRON_ROOTS:
         REMBIT(ch->affected_by, AFF_NO_FLEE);
         break;
      case SPELL_OAKEN_FORTITUDE:
	 redo_hitpoints(ch);
	break;
      case SPELL_STONE_SKIN:  /* Stone skin wears off... Remove resistance */
         REMOVE_BIT(ch->resist, ISR_PIERCE);
         break;
      case SPELL_BARKSKIN: /* barkskin wears off */
         REMOVE_BIT(ch->resist, ISR_SLASH);
         break;
      case SPELL_FLY:
         /* Fly wears off...you fall :) */
         if(((flags & SUPPRESS_CONSEQUENCES) == 0) &&
           ((IS_SET(world[ch->in_room].room_flags, FALL_DOWN) && (dir = 5)) ||
           (IS_SET(world[ch->in_room].room_flags, FALL_UP) && (dir = 4)) ||
           (IS_SET(world[ch->in_room].room_flags, FALL_EAST) && (dir = 1)) ||
           (IS_SET(world[ch->in_room].room_flags, FALL_WEST) && (dir = 3)) ||
           (IS_SET(world[ch->in_room].room_flags, FALL_SOUTH) && (dir = 2))||
           (IS_SET(world[ch->in_room].room_flags, FALL_NORTH) && (dir = 0))))
         {
            if (do_fall(ch, dir) & eCH_DIED)
               char_died = TRUE;
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
	  switch(ch->race)
	  { // Each race has its own wear off messages.
	    case RACE_GIANT:
	       send_to_char("You feel ready to wield mighty weapons again.\r\n",ch);
		break;
	    case RACE_TROLL:
		send_to_char("Your regenerative abilitiy feels restored.\r\n",ch);
		break;
	    case RACE_GNOME:
		send_to_char("Your ability to manipulate illusions has returned.\r\n",ch);
		break;
	    case RACE_ORC:
		send_to_char("Your lust for blood feels restored.\r\n",ch);
		break;
	    case RACE_DWARVEN:
		send_to_char("You feel ready to attempt more repairs.\r\n",ch);
		break;
	    case RACE_ELVEN:
		send_to_char("Your mind regains its ability to sharpen concentration for brief periods.\r\n",ch);
		break;
	    case RACE_PIXIE:
		send_to_char("Your ability to avoid magical vision has returned.\r\n",ch);
		break;
	    case RACE_HOBBIT:
		send_to_char("Your innate ability to avoid magical portals has returned.\r\n",ch);
		break;
	  }
	break;
      case SKILL_BLOOD_FURY:
         if (!(flags & SUPPRESS_MESSAGES))
            send_to_char("Your blood cools to normal levels.\r\n", ch);
         break;
      case SKILL_CRAZED_ASSAULT:
         if (!(flags & SUPPRESS_MESSAGES))
            send_to_char("Your craziness has subsided.\r\n", ch);
         break;
      case SKILL_INNATE_BLOODLUST:
         if (!(flags & SUPPRESS_MESSAGES))
 	 send_to_char("Your lust for battle has left you.\r\n",ch);
	 break;
      case SKILL_INNATE_ILLUSION:
         if (!(flags & SUPPRESS_MESSAGES))
 	  send_to_char("You slowly fade into existence.\r\n",ch);
	  break;
      case SKILL_INNATE_EVASION:
         if (!(flags & SUPPRESS_MESSAGES))
	  send_to_char("Your magical obscurity has left you.\r\n",ch);
	  break;
      case SKILL_INNATE_SHADOWSLIP:
         if (!(flags & SUPPRESS_MESSAGES))
	  send_to_char("The ability to avoid magical pathways has left you.\r\n",ch);
	  break;
      case SKILL_INNATE_FOCUS:
         if (!(flags & SUPPRESS_MESSAGES))
 	  send_to_char("Your concentration lessens to its regular amount.\r\n",ch);
	  break;
      case SKILL_INNATE_REGENERATION:
         if (!(flags & SUPPRESS_MESSAGES))
	 send_to_char("Your regeneration slows back to normal.\r\n",ch);
	 break;
      case SKILL_INNATE_POWERWIELD:
         struct obj_data *obj;
         obj = ch->equipment[WIELD];
         if (obj)
         if (obj->obj_flags.extra_flags & ITEM_TWO_HANDED)
         {
            if ((obj = ch->equipment[SECOND_WIELD])) {
               obj_to_char(unequip_char(ch, SECOND_WIELD, (flags & SUPPRESS_MESSAGES)),ch);
               if (!(flags & SUPPRESS_MESSAGES))
                  act("You shift $p into your inventory.",ch, obj, NULL, TO_CHAR, 0);
            }
            else if((obj = ch->equipment[HOLD])) {
               obj_to_char(unequip_char(ch, HOLD, (flags & SUPPRESS_MESSAGES)),ch);
               if (!(flags & SUPPRESS_MESSAGES))
                  act("You shift $p into your inventory.",ch, obj, NULL, TO_CHAR, 0);
            }
            else if((obj = ch->equipment[HOLD2])) {
               obj_to_char(unequip_char(ch, HOLD2, (flags & SUPPRESS_MESSAGES)),ch);
               if (!(flags & SUPPRESS_MESSAGES))
                  act("You shift $p into your inventory.",ch, obj, NULL, TO_CHAR, 0);
            }
            else if((obj = ch->equipment[WEAR_SHIELD])) {
               obj_to_char(unequip_char(ch, WEAR_SHIELD, (flags & SUPPRESS_MESSAGES)),ch);
               if (!(flags & SUPPRESS_MESSAGES))
                  act("You shift $p into your inventory.",ch, obj, NULL, TO_CHAR, 0);
            }
            else if((obj = ch->equipment[WEAR_LIGHT])) {
               obj_to_char(unequip_char(ch, WEAR_LIGHT, (flags & SUPPRESS_MESSAGES)),ch);
               if (!(flags & SUPPRESS_MESSAGES))
                  act("You shift $p into your inventory.",ch, obj, NULL, TO_CHAR, 0);
            }
         }
         obj = ch->equipment[SECOND_WIELD];
         if (obj)
         if (obj->obj_flags.extra_flags & ITEM_TWO_HANDED)
         {
            obj = ch->equipment[WIELD];
            if(obj) {
               obj_to_char(unequip_char(ch, SECOND_WIELD, (flags & SUPPRESS_MESSAGES)),ch);
               if (!(flags & SUPPRESS_MESSAGES))
                  act("You shift $p into your inventory.",ch, obj, NULL, TO_CHAR, 0);
            }
         }
         if (!(flags & SUPPRESS_MESSAGES))
            send_to_char("You can no longer wield two handed weapons.\r\n",ch);
         check_weapon_weights(ch);
         break;
      case SKILL_BLADESHIELD:
         if (!(flags & SUPPRESS_MESSAGES))
            send_to_char("The draining affect of the blade shield technique has worn off.\r\n", ch);
         break;
      case SKILL_FOCUSED_REPELANCE:
         REMOVE_BIT(ch->combat, COMBAT_REPELANCE);
         if (!(flags & SUPPRESS_MESSAGES))
            send_to_char("Your mind recovers from the repelance.\n\r", ch);
         break;
      case SKILL_VITAL_STRIKE:
         if (!(flags & SUPPRESS_MESSAGES))
            send_to_char("The internal strength and speed from your vital strike has returned.\r\n", ch);
         break;
      case SPELL_WATER_BREATHING:
         if(!(flags & SUPPRESS_CONSEQUENCES) && world[ch->in_room].sector_type == SECT_UNDERWATER) // uh oh
         {
#if 0
            // you just drowned!
            act("$n begins to choke on the water, a look of panic filling $s eyes as it fill $s lungs.\n\r"
            "$n is DEAD!!", ch, 0, 0, TO_ROOM, 0);
            send_to_char("The water rushes into your lungs and the light fades with your oxygen.\n\r"
            "You have been KILLED!!!\n\r", ch);
            fight_kill(NULL, ch, TYPE_RAW_KILL, 0);
            char_died = TRUE;
#else
            act("$n begins to choke on the water, a look of panic filling $s eyes as it fill $s lungs.\n\r", ch, 0, 0, TO_ROOM, 0);
            send_to_char("The water rushes into your lungs and the light fades with your oxygen.\n\r", ch);
#endif
          }
          break;
      case KI_STANCE + KI_OFFSET:
         if (!(flags & SUPPRESS_MESSAGES))
            send_to_char("Your body finishes venting the energy absorbed from your last ki stance.\r\n", ch);
         break;
      case SPELL_CHARM_PERSON:   /* Charm Wears off */
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
      case KI_AGILITY+KI_OFFSET:
         if (!(flags & SUPPRESS_MESSAGES))
            send_to_char("Your body has lost its focused agility.\r\n", ch);
         break;
      default:
         break;
   }

   if (!char_died) {
      dc_free ( af );
      affect_total( ch ); // this must be here to take care of anything we might
                          // have just removed that is still given by equipment
   }
}



/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char( CHAR_DATA *ch, int skill)
{
    struct affected_type *hjp, *afc,*recheck;
//    bool aff2Fix;

    if(skill < 0)  // affect types are unsigned, so no negatives are possible
       return;
    for(hjp = ch->affected; hjp; hjp = afc) {
        afc = hjp->next;
	if (hjp->type == (unsigned)skill)
	    affect_remove( ch, hjp, 0);
	bool a = TRUE;
	for (recheck = ch->affected;recheck;recheck = recheck->next)
	if (recheck == afc) a= FALSE;
	if (a) break;
    }

 }



/*
 * Return if a char is affected by a spell (SPELL_XXX), NULL indicates 
 * not affected.
 */
affected_type * affected_by_spell( CHAR_DATA *ch, int skill )
{
  struct affected_type *curr;

  if(skill < 0)  // all affect types are unsigned
     return NULL;

  for(curr = ch->affected; curr; curr = curr->next)
     if(curr->type == (unsigned)skill)
       return curr;

  return NULL;
}



void affect_join( CHAR_DATA *ch, struct affected_type *af,
		  bool avg_dur, bool avg_mod )
{
    struct affected_type *hjp;
    bool found = FALSE;

    for (hjp = ch->affected; !found && hjp; hjp = hjp->next) {
	if ( hjp->type == af->type ) {
	    
	    af->duration += hjp->duration;
	    if (avg_dur)
		af->duration /= 2;

	    af->modifier += hjp->modifier;
	    if (avg_mod)
		af->modifier /= 2;

	    affect_remove(ch, hjp, SUPPRESS_ALL);
	    affect_to_char(ch, af);
	    found = TRUE;
	}
    }
    if (!found)
	affect_to_char(ch, af);
}

/* move a player out of a room */
/* Returns 0 on failure, non-zero otherwise */
int char_from_room(CHAR_DATA *ch)
{
  CHAR_DATA *i;
  bool Other = FALSE, More = FALSE, kimore = FALSE;

  if(ch->in_room == NOWHERE) {
    return(0); 
  }

  if(ch->fighting)
    stop_fighting(ch);

  world[ch->in_room].light -= ch->glow_factor;

  if(ch == world[ch->in_room].people)  /* head of list */
    world[ch->in_room].people = ch->next_in_room;

  /* locate the previous element */
  else
    for(i = world[ch->in_room].people; i; i = i->next_in_room) {
       if(i->next_in_room == ch)
         i->next_in_room = ch->next_in_room;
    }
//  if (IS_NPC(ch) && ISSET(ch->mobdata->actflags, ACT_NOMAGIC))
//	debugpoint();
  for(i = world[ch->in_room].people; i; i = i->next_in_room) {
     if (IS_NPC(i) && ISSET(i->mobdata->actflags, ACT_NOMAGIC))
	Other = TRUE;
     if (IS_NPC(i) && ISSET(i->mobdata->actflags, ACT_NOTRACK))
	 More = TRUE;
     if (IS_NPC(i) && ISSET(i->mobdata->actflags, ACT_NOKI))
         kimore = TRUE;
     }
  if(!IS_NPC(ch)) // player
    zone_table[world[ch->in_room].zone].players--;
  if (IS_NPC(ch))
     ch->mobdata->last_room = ch->in_room;
  if (IS_NPC(ch))
  if (ISSET(ch->mobdata->actflags, ACT_NOTRACK) && !More && IS_SET(world[ch->in_room].iFlags, NO_TRACK))
  {
    REMOVE_BIT(world[ch->in_room].iFlags,NO_TRACK);
    REMOVE_BIT(world[ch->in_room].room_flags, NO_TRACK);
  }
  if (IS_NPC(ch))
  if (ISSET(ch->mobdata->actflags, ACT_NOKI) && !kimore && IS_SET(world[ch->in_room].iFlags, NO_TRACK))
  {
     REMOVE_BIT(world[ch->in_room].iFlags, NO_TRACK);
     REMOVE_BIT(world[ch->in_room].room_flags, NO_TRACK);
  }
  if (IS_NPC(ch) && ISSET(ch->mobdata->actflags, ACT_NOMAGIC) && !Other && IS_SET(world[ch->in_room].iFlags, NO_MAGIC))
  {
    REMOVE_BIT(world[ch->in_room].iFlags, NO_MAGIC);
    REMOVE_BIT(world[ch->in_room].room_flags, NO_MAGIC);
  }

  ch->in_room = NOWHERE;
  ch->next_in_room = 0;

  /* success */
  return(1);
}

bool is_hiding(CHAR_DATA *ch, CHAR_DATA *vict)
{
  if (IS_NPC(ch)) return (number(1,101) > 70);

  if (!has_skill(ch, SKILL_HIDE)) return FALSE;
  if (ch->in_room != vict->in_room)
    return skill_success(ch, vict, SKILL_HIDE);

  int i;
  for (i = 0; i < MAX_HIDE; i++)
   if (ch->pcdata->hiding_from[i] == vict && 
	ch->pcdata->hide[i])
      return TRUE;
  return skill_success(ch, vict, SKILL_HIDE);
}

/* place a character in a room */
/* Returns zero on failure, and one on success */
int char_to_room(CHAR_DATA *ch, int room)
{
    CHAR_DATA *temp;
    if (room == NOWHERE)
      return(0);

    assert( world[room].people != ch );

    ch->next_in_room = world[room].people;
    world[room].people = ch;
    ch->in_room = room;

    world[room].light += ch->glow_factor;
    int a,i;
    if (!IS_NPC(ch) && 
	ISSET(ch->affected_by, AFF_HIDE) && (a = has_skill(ch, SKILL_HIDE)))
    {
	for (i = 0; i < MAX_HIDE;i++)
	  ch->pcdata->hiding_from[i] = NULL;
	i = 0;
	for (temp = ch->next_in_room; temp; temp = temp->next_in_room)
	{
	  if (i >= MAX_HIDE) break;
	  if (number(1,101) > a) // Failed.
	  {
	    ch->pcdata->hiding_from[i] = temp;
	    ch->pcdata->hide[i++] = FALSE;
	  } else {
	    ch->pcdata->hiding_from[i] = temp;
	    ch->pcdata->hide[i++] = TRUE;
	  }
	}
    }
    for (temp = ch->next_in_room; temp; temp = temp->next_in_room)
    {
      if (ISSET(temp->affected_by, AFF_HIDE) && !IS_NPC(temp))
        for (i = 0; i < MAX_HIDE; i++)
	{
	  if (temp->pcdata->hiding_from[i] == NULL
		|| temp->pcdata->hiding_from[i] == ch)
	  {
	    if (number(1,101) > has_skill(temp, SKILL_HIDE))
	    {
		temp->pcdata->hiding_from[i] = ch;
		temp->pcdata->hide[i] = FALSE;
	    } else {
		temp->pcdata->hiding_from[i] = ch;
		temp->pcdata->hide[i] = TRUE;
	    }
	  }
	}
    }
    if(!IS_NPC(ch)) // player
      zone_table[world[room].zone].players++;
  if (IS_NPC(ch)) { 
    if (ISSET(ch->mobdata->actflags, ACT_NOMAGIC) && !IS_SET(world[room].room_flags, NO_MAGIC))
    {
	SET_BIT(world[room].iFlags, NO_MAGIC);
	SET_BIT(world[room].room_flags, NO_MAGIC);
    }
    if (ISSET(ch->mobdata->actflags, ACT_NOKI) && !IS_SET(world[room].room_flags, NO_KI))
    {
	SET_BIT(world[room].iFlags, NO_KI);
	SET_BIT(world[room].room_flags, NO_KI);
    }
    if (ISSET(ch->mobdata->actflags, ACT_NOTRACK) && !IS_SET(world[room].room_flags, NO_TRACK))
    {
	SET_BIT(world[room].iFlags, NO_TRACK);
	SET_BIT(world[room].room_flags, NO_TRACK);
    }
  }
    return(1);
}



/* Return the effect of a piece of armor in position eq_pos */
int apply_ac(CHAR_DATA *ch, int eq_pos)
{
  int value;
  assert(ch->equipment[eq_pos]);

  if (!(GET_ITEM_TYPE(ch->equipment[eq_pos]) == ITEM_ARMOR))
    return 0;

   if (IS_SET(ch->equipment[eq_pos]->obj_flags.extra_flags,ITEM_ENCHANTED))
      value = (ch->equipment[eq_pos]->obj_flags.value[0]);
     else
      value = (ch->equipment[eq_pos]->obj_flags.value[0]) - 
              (ch->equipment[eq_pos]->obj_flags.value[1]);

  return value;
}


// return 0 on failure
// 1 on success
int equip_char(CHAR_DATA *ch, struct obj_data *obj, int pos, int flag)
{
    int j;

    if(!ch || !obj)
    {
      log("Null ch or obj in equp_char()!", ANGEL, LOG_BUG);
      return 0;
    }  
    if(pos < 0 || pos >= MAX_WEAR)
    {
       log("Invalid eq position in equip_char!", ANGEL, LOG_BUG);
       return 0;
    }
    if(ch->equipment[pos])
    {
       log("Already equipped in equip_char!", ANGEL, LOG_BUG);
       return 0;
    }

    if (obj->carried_by) {
	log("EQUIP: Obj is carried_by when equip.", ANGEL, LOG_BUG);
	return 0;
    }

    if (obj->in_room != NOWHERE) {
	log("EQUIP: Obj is in_room when equip.", ANGEL, LOG_BUG);
	return 0;
    }

    if (!IS_NPC(ch))
    if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
	(IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
	(IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))&& !IS_NPC(ch)) 
    {
	if(IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
	   affected_by_spell(ch, FUCK_PTHIEF) || contains_no_trade_item(obj)) {
	    act("You are zapped by $p but it stays with you.", ch, obj, 0, TO_CHAR, 0);
	    recheck_height_wears(ch);
            obj_to_char(obj, ch);
            if(pos == WIELD && ch->equipment[SECOND_WIELD]) {
              equip_char(ch, unequip_char(ch, SECOND_WIELD), WIELD);
            }
            return 1;
        }
	if (ch->in_room != NOWHERE) {
	    act("You are zapped by $p and instantly drop it.", ch, obj, 0, TO_CHAR, 0);
	    act("$n is zapped by $p and instantly drops it.", ch, obj, 0, TO_ROOM, 0);
	    recheck_height_wears(ch);
	    obj_to_room(obj, ch->in_room);
            if(pos == WIELD && ch->equipment[SECOND_WIELD]) {
              equip_char(ch, unequip_char(ch, SECOND_WIELD), WIELD);
            }
	    return 1;
	} else {
	    log("ch->in_room = NOWHERE when equipping char.", 0, LOG_BUG);
	}
    }

    ch->equipment[pos] = obj;
    obj->equipped_by   = ch;
	if (!IS_NPC(ch))
       for (int a =0; a < obj->num_affects; a++)
       {
         if (obj->affected[a].location >= 1000)
         {
                obj->next_skill = ch->pcdata->skillchange;
                ch->pcdata->skillchange = obj;
		break;
          }
       }

    if(IS_SET(obj->obj_flags.extra_flags, ITEM_GLOW)) {
      ch->glow_factor++;
      if(ch->in_room > -1)
        world[ch->in_room].light++;
//  this crashes in a reconnect cause pcdata isn't around yet
//  rather than fixing it, i'm leaving it out because it's annoying anyway cause
//  it tells you every time you save
// TODO - make it not be annoying
//      act("The soft glow from $p brightens the area around $n.", ch, obj, 0, TO_ROOM, 0);
//      act("The soft glow from $p brightens the area around you.", ch, obj, 0, TO_CHAR, 0);
    }
    if(obj->obj_flags.type_flag == ITEM_LIGHT &&
       obj->obj_flags.value[2]) 
    {
      ch->glow_factor++;
      if(ch->in_room > -1)
        world[ch->in_room].light++;
    }

    if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
	GET_AC(ch) -= apply_ac(ch, pos);

    for(j=0; ch->equipment[pos] && j<ch->equipment[pos]->num_affects; j++)
	affect_modify(ch, obj->affected[j].location,
	  obj->affected[j].modifier, -1, TRUE);

   add_set_stats(ch, obj, flag,pos);

   redo_hitpoints(ch);
   redo_mana(ch);
   redo_ki(ch);


    return 1;
}



struct obj_data *unequip_char(CHAR_DATA *ch, int pos, int flag)
{
    int j;
    struct obj_data *obj;

    assert(pos>=0 && pos<MAX_WEAR);
    assert(ch->equipment[pos]);

    obj = ch->equipment[pos];
    if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
	GET_AC(ch) += apply_ac(ch, pos);

    remove_set_stats(ch, obj,flag);
    struct obj_data *a,*b = NULL;
	b: // ew
     if (!IS_NPC(ch))
    for (a = ch->pcdata->skillchange; a; a = a->next_skill)
    {
	if (a == (obj_data*)0x95959595)
	{
	  int i;
	  ch->pcdata->skillchange = NULL;
	for (i = 0; i < MAX_WEAR;i++)
	{
	   int j;
	   if (!ch->equipment[i]) continue;
           for (j = 0; j < ch->equipment[i]->num_affects; j++)
	   {
		if (ch->equipment[i]->affected[j].location > 1000)
		{
		  ch->equipment[i]->next_skill = ch->pcdata->skillchange;
		  ch->pcdata->skillchange = ch->equipment[i];
		  break;
		}
	   }
	   
	}
	  goto b;
	}
        if (a == obj)
	{
	  if (b) b->next_skill = a->next_skill;
	  else ch->pcdata->skillchange = a->next_skill;
	   break;
	}
	b = a;
    }

    ch->equipment[pos] = 0; 
    obj->equipped_by   = 0;

    if(IS_SET(obj->obj_flags.extra_flags, ITEM_GLOW)) {
      ch->glow_factor--;
      if(ch->in_room > -1)
        world[ch->in_room].light--;
// this is just annoying cause it tells you every time you save
// TODO - make it not be annoying
//      act("The soft glow around $n from $p fades.", ch, obj, 0, TO_ROOM, 0);
//      act("The glow around you fades slightly.", ch, obj, 0, TO_CHAR, 0);
    }
    if(obj->obj_flags.type_flag == ITEM_LIGHT &&
       obj->obj_flags.value[2]) 
    {
      ch->glow_factor--;
      if(ch->in_room > -1)
        world[ch->in_room].light--;
    }

    for(j=0; j<obj->num_affects; j++)
	affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier, -1, FALSE);
   redo_hitpoints(ch);
   redo_mana(ch);
   redo_ki(ch);

    return(obj);
}


int get_number(char **name)
{
  unsigned i;
  char *ppos = NULL;
  char number[MAX_INPUT_LENGTH];


  if((ppos = index(*name, '.')) != NULL) {
    *ppos++ = '\0';
    // at this point, ppos points to the name only, and there is a 
    // \0 between the number and the name as they appear in the string. 
    strcpy(number, *name);
    // now number contains the number as a string 
    strcpy(*name, ppos);
    // now the pointer that was passed into the function
    // points to the name only.

    for(i = 0; *(number + i); i++)
       if(!isdigit(*(number + i)))
         return(-1);

    return(atoi(number));
  }

  return(1);
}


/* Search a given list for an object, and return a pointer to that object */
struct obj_data *get_obj_in_list(char *name, struct obj_data *list)
{
  struct obj_data *i;
  int j, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp;

  strcpy(tmpname, name);
  tmp = tmpname;
  if((number = get_number(&tmp)) < 0)
    return(0);
 
  for(i = list, j = 1; i && (j <= number); i = i->next_content)
     if(isname(tmp, i->name)) {
       if(j == number) 
         return i;
       j++;
     }

  return(0);
}



/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list)
{
    struct obj_data *i;

    for (i = list; i; i = i->next_content)
	if (i->item_number == num) 
	    return(i);
	
    return(0);
}





/*search the entire world for an object, and return a pointer  */
struct obj_data *get_obj(char *name)
{
  struct obj_data *i;
  int j, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp;

  strcpy(tmpname,name);
  tmp = tmpname;
  if((number = get_number(&tmp)) < 0)
    return(0);

  for(i = object_list, j = 1; i && (j <= number); i = i->next)
     if(isname(tmp, i->name)) {
       if(j == number)
         return(i);
       j++;
     }

  return(0);
}



/*search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(int nr)
{
    struct obj_data *i;

    for (i = object_list; i; i = i->next)
	if (i->item_number == nr) 
	    return(i);

    return(0);
}





/* search a room for a char, and return a pointer if found..  */
CHAR_DATA *get_char_room(char *name, int room, bool careful)
{
  CHAR_DATA *i;
  CHAR_DATA *partial_match;
  int j, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp;

  partial_match = 0;

  strcpy(tmpname,name);
  tmp = tmpname;
  if((number = get_number(&tmp)) < 0)
  return(0);

  for(i = world[room].people, j = 0; i && (j <= number); i = i->next_in_room)
      {
      if (number == 0 && IS_NPC(i)) continue;
      if (number == 1 || number == 0)
         {
         if (isname(tmp, GET_NAME(i)) && !(careful && IS_NPC(i) && mob_index[i->mobdata->nr].virt == 12))
            return(i);
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
         if(isname(tmp, GET_NAME(i)))
            {
  	    j++;
            if(j == number)
               return(i);
            }
         }
      }

  return(partial_match);
}





/* search all over the world for a char, and return a pointer if found */
CHAR_DATA *get_char(char *name)
{
   CHAR_DATA *i;
   CHAR_DATA *partial_match;
   int j, number;
   char tmpname[MAX_INPUT_LENGTH];
   char *tmp;

   partial_match = 0;
   
   strcpy(tmpname,name);
   tmp = tmpname;
   if((number = get_number(&tmp)) < 0)
      return(0);

   for (i = character_list, j = 0; i && (j <= number); i = i->next)
      {
      if (number == 0 && IS_NPC(i)) continue;
      if (number == 1 || number == 0)
         {
         if (isname(tmp, GET_NAME(i)))
            return(i);
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
         if(isname(tmp, GET_NAME(i)))
            {
	    j++;
            if(j == number)
               return(i);
            }
         }
	   }

    return(partial_match);
}

/* search all over the world for a mob, and return a pointer if found */
CHAR_DATA *get_mob(char *name)
{
   CHAR_DATA *i;

   for (i = character_list; i ; i = i->next)
   {
     if(!IS_MOB(i))
        continue;
     if (isname(name, GET_NAME(i)))
        return(i);
   }
   return NULL;
}



/* search all over the world for a char num, and return a pointer if found */
CHAR_DATA *get_char_num(int nr)
{
    CHAR_DATA *i;

    for (i = character_list; i; i = i->next)
	if (IS_MOB(i) && i->mobdata->nr == nr)
	    return(i);

    return(0);
}

// move an object from its current location into the room
// specified by dest.
int move_obj(obj_data *obj, int dest)
{
  int         obj_in_room  = NOWHERE;
  obj_data  * contained_by = 0;
  char_data * carried_by   = 0;

  if(!obj) {
    log("NULL object sent to move_obj!", OVERSEER, LOG_BUG);
    return 0;
  }
  
  if(obj->equipped_by && GET_ITEM_TYPE(obj) != ITEM_BEACON) {
    fprintf(stderr, "FATAL: Object move_obj() while equipped: %s.\n",
            obj->name);
    abort();
  }
  
  if((obj_in_room = obj->in_room) != NOWHERE) {
    if(obj_from_room(obj) == 0) {
    // Couldn't move obj from the room
      logf(OVERSEER, LOG_BUG, "Couldn't move %s from room %d.",
           obj->name, world[obj_in_room].number);
      return 0;
    }
  }

  if((carried_by = obj->carried_by)) {
    if(obj_from_char(obj) == 0) {
    // Couldn't move obj from the room
      logf(OVERSEER, LOG_BUG, "%s was carried by %s, and I couldn't "
           "remove it!", obj->name, GET_NAME(obj->carried_by));
      return 0;
    }
  }
  
  if((contained_by = obj->in_obj)) {
    if(obj_from_obj(obj) == 0) {
    // Couldn't move obj from its container
      logf(OVERSEER, LOG_BUG, "%s was in container %s, and I couldn't "
           "remove it!", obj->name, GET_NAME(obj->carried_by));
      return 0;
    }
  }


  if(obj_to_room(obj, dest) == 0) {
    // Couldn't move obj to dest...where to put it now?
    
    if( (obj_in_room != NOWHERE) && (obj_to_room(obj, obj_in_room) == 0) ) {
      // Now we have real problems
      fprintf(stderr, "FATAL: Object stuck in NOWHERE (1): %s.\n", obj->name);
      abort();
    }
    else if( (carried_by) && (obj_to_char(obj, carried_by) == 0) ) {
      // Now we have real problems
      fprintf(stderr, "FATAL: Object stuck in NOWHERE (2) : %s.\n", obj->name);
      abort();
    }
    else if( (contained_by) && (obj_to_obj(obj, contained_by) == 0) ) {
      // Now we have real problems
      fprintf(stderr, "FATAL: Object stuck in NOWHERE (3) : %s.\n", obj->name);
      abort();
    }
    
    logf(OVERSEER, LOG_BUG, "Could not move %s to destination: %d",
         obj->name, world[dest].number);
    return 0;
  }
  
  // At this point, the object is happily in the new room
  return 1;
}

// move an object from its current location into the container
// specified by dest_obj.
int move_obj(obj_data *obj, obj_data * dest_obj)
{
  int         obj_in_room  = NOWHERE;
  obj_data  * contained_by = 0;
  char_data * carried_by   = 0;

  if(!obj) {
    log("NULL object sent to move_obj!", OVERSEER, LOG_BUG);
    return 0;
  }
  
  if(obj->equipped_by && GET_ITEM_TYPE(obj) != ITEM_BEACON) {
    fprintf(stderr, "FATAL: Object move_obj() while equipped: %s.\n",
            obj->name);
    abort();
  }
  
  if((obj_in_room = obj->in_room) != NOWHERE) {
    if(obj_from_room(obj) == 0) {
    // Couldn't move obj from the room
      logf(OVERSEER, LOG_BUG, "Couldn't move %s from room %d.",
           obj->name, world[obj_in_room].number);
      return 0;
    }
  }

  if((carried_by = obj->carried_by)) {
    if(obj_from_char(obj) == 0) {
    // Couldn't move obj from the room
      logf(OVERSEER, LOG_BUG, "%s was carried by %s, and I couldn't "
           "remove it!", obj->name, GET_NAME(obj->carried_by));
      return 0;
    }
  }
  
  if((contained_by = obj->in_obj)) {
    if(obj_from_obj(obj) == 0) {
    // Couldn't move obj from its container
      logf(OVERSEER, LOG_BUG, "%s was in container %s, and I couldn't "
           "remove it!", obj->name, GET_NAME(obj->carried_by));
      return 0;
    }
  }


  if(obj_to_obj(obj, dest_obj) == 0) {
    // Couldn't move obj to dest_obj...where to put it now?
    
    if( (obj_in_room != NOWHERE) && (obj_to_room(obj, obj_in_room) == 0) ) {
      // Now we have real problems
      fprintf(stderr, "FATAL: Object stuck in NOWHERE (4): %s.\n", obj->name);
      abort();
    }
    else if( (carried_by) && (obj_to_char(obj, carried_by) == 0) ) {
      // Now we have real problems
      fprintf(stderr, "FATAL: Object stuck in NOWHERE (5) : %s.\n", obj->name);
      abort();
    }
    else if( (contained_by) && (obj_to_obj(obj, contained_by) == 0) ) {
      // Now we have real problems
      fprintf(stderr, "FATAL: Object stuck in NOWHERE (6) : %s.\n", obj->name);
      abort();
    }
    
    logf(OVERSEER, LOG_BUG, "Could not move %s to container: %s",
         obj->name, dest_obj->name);
    return 0;
  }
  add_totem(dest_obj, obj);  
  // At this point, the object is happily in the new room
  return 1;
}

// move an object from its current location into the inventory of the
// character specified by ch.
int move_obj(obj_data *obj, char_data * ch)
{
  int         obj_in_room  = NOWHERE;
  obj_data  * contained_by = 0;
  char_data * carried_by   = 0;
  struct obj_data * search_char_for_item(char_data * ch, int16 item_number, bool wearonly = FALSE);

//  char buffer[300];

  if(!obj) {
    log("NULL object sent to move_obj!", OVERSEER, LOG_BUG);
    return 0;
  }
  
  if(obj->equipped_by && GET_ITEM_TYPE(obj) != ITEM_BEACON) {
    fprintf(stderr, "FATAL: Object move_obj() while equipped: %s.\n",
            obj->name);
    abort();
  }
  
  if((obj_in_room = obj->in_room) != NOWHERE) {
    if(obj_from_room(obj) == 0) {
    // Couldn't move obj from the room
      logf(OVERSEER, LOG_BUG, "Couldn't move %s from room %d.",
           obj->name, world[obj_in_room].number);
      return 0;
    }
  }

  if((carried_by = obj->carried_by)) {
    if(obj_from_char(obj) == 0) {
    // Couldn't move obj from the room
      logf(OVERSEER, LOG_BUG, "%s was carried by %s, and I couldn't "
           "remove it!", obj->name, GET_NAME(obj->carried_by));
      return 0;
    }
  }
  
  if((contained_by = obj->in_obj)) {
   if (obj->obj_flags.type_flag == ITEM_TOTEM &&
        contained_by->obj_flags.type_flag == ITEM_ALTAR)
         remove_totem(contained_by, obj);

    if(obj_from_obj(obj) == 0) {
    // Couldn't move obj from its container
      logf(OVERSEER, LOG_BUG, "%s was in container %s, and I couldn't "
           "remove it!", obj->name, GET_NAME(obj->carried_by));
      return 0;
    }
  }

/* TODO - This doesn't work for some reason....need to find out why

  // This should make sure we don't have any money items on players
  if(obj->obj_flags.type_flag == ITEM_MONEY && 
     obj->obj_flags.value[0] >= 1 ) {
        sprintf(buffer,"There was %d coins.\n\r", obj->obj_flags.value[0]);
        send_to_char(buffer,ch);
        GET_GOLD(ch) += obj->obj_flags.value[0];   
        extract_obj(obj);    
        return 1;
  }
*/
 
  if(obj_to_char(obj, ch) == 0) {
    // Couldn't move obj to ch...where to put it now?
    
    if( (obj_in_room != NOWHERE) && (obj_to_room(obj, obj_in_room) == 0) ) {
      // Now we have real problems
      fprintf(stderr, "FATAL: Object stuck in NOWHERE (7): %s.\n", obj->name);
      abort();
    }
    else if( (carried_by) && (obj_to_char(obj, carried_by) == 0) ) {
      // Now we have real problems
      fprintf(stderr, "FATAL: Object stuck in NOWHERE (8) : %s.\n", obj->name);
      abort();
    }
    else if( (contained_by) && (obj_to_obj(obj, contained_by) == 0) ) {
      // Now we have real problems
      fprintf(stderr, "FATAL: Object stuck in NOWHERE (9) : %s.\n", obj->name);
      abort();
    }
    
    logf(OVERSEER, LOG_BUG, "Could not move %s to character: %s",
         obj->name, GET_NAME(ch));
    return 0;
  }
  
  // At this point, the object is happily with the new owner
  return 1;
}

// give an object to a char
// 1 if success, 0 if failure
int obj_to_char(struct obj_data *object, CHAR_DATA *ch)
{
  //struct obj_data *obj;
/*
  if(!(obj = ch->carrying) ||
     (!obj->next_content && obj->item_number > object->item_number)) 
  {
*/
    object->next_content = ch->carrying;
    ch->carrying = object;
    object->carried_by = ch;
    object->equipped_by = 0;
    object->in_room = NOWHERE;
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;
    return 1;
/*
  }

  while(obj->next_content &&
        obj->next_content->item_number < object->item_number)
    obj = obj->next_content;

  object->next_content = obj->next_content;
  obj->next_content    = object;
  object->carried_by   = ch;
  object->equipped_by  = 0; 
  object->in_room      = NOWHERE;
  
  IS_CARRYING_W(ch)   += GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(ch)++;
  
  return 1;
*/
}


// take an object from a char
int obj_from_char(struct obj_data *object)
{
  struct obj_data *tmp;

  if(!object->carried_by) {
    log("Obj_from_char called on an object no one is carrying!", OVERSEER,
        LOG_BUG);
    return 0;
  }

  // head of list
  if (object->carried_by->carrying == object)
    object->carried_by->carrying = object->next_content;
  else {
    // locate previous
    for (tmp = object->carried_by->carrying;
         tmp && (tmp->next_content != object); 
       tmp = tmp->next_content);

    tmp->next_content = object->next_content;
  }

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  
  if (IS_CARRYING_N(object->carried_by) < 0)
    IS_CARRYING_N(object->carried_by) = 0;
    
  object->carried_by = 0;
  object->next_content = 0;
  
  return 1;
}



// put an object in a room
int obj_to_room(struct obj_data *object, int room)
{
  struct obj_data *obj;

  if(object->obj_flags.type_flag == ITEM_PORTAL)
    world[room].light++;

  // combine any cash amounts
  if(GET_ITEM_TYPE(object) == ITEM_MONEY) 
  {
     for(obj = world[room].contents; obj ; obj = obj->next_content)
       if(GET_ITEM_TYPE(obj) == ITEM_MONEY) {
          object->obj_flags.value[0] += obj->obj_flags.value[0];
          object->description = str_hsh("A pile of gold coins.");
          extract_obj(obj);
          break;
       }
  }

  // search through for the last object, or another object just like this one
  for(obj = world[room].contents;
      obj && obj->next_content;
      obj = obj->next_content)
  {
    if(obj->item_number == object->item_number)
      break;
  }

  // put it in the list
  if(!obj) {
    object->next_content = NULL;
    world[room].contents = object;
  }
  else {
    object->next_content = obj->next_content;
    obj->next_content = object;
  }
  object->in_room = room;
  object->carried_by = 0;
  if(GET_ITEM_TYPE(object) != ITEM_BEACON)
     object->equipped_by  = 0;

/*
  if(!(obj = world[room].contents) ||
     (!obj->next_content && obj->item_number > object->item_number)) 
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
        obj->next_content->item_number < object->item_number) 
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
int obj_from_room(struct obj_data *object)
{
  struct obj_data *i;

  if(object->in_room < 0) {
    log("obj_from_room called on an object that isn't in a room!", OVERSEER,
        LOG_BUG);
    return 0;
  }
  
  if(object->obj_flags.type_flag == ITEM_PORTAL)
    world[object->in_room].light--;

  // head of list
  if(object == world[object->in_room].contents)
    world[object->in_room].contents = object->next_content;

  // locate previous element in list
  else {
    for(i = world[object->in_room].contents; i && 
       (i->next_content != object); i = i->next_content)
       ;

    i->next_content = object->next_content;
  }

  object->in_room = NOWHERE;
  object->next_content = 0;
  
  save_corpses();
  return 1;
}


// put an object in an object (quaint)
int obj_to_obj(struct obj_data *obj, struct obj_data *obj_to)
{
  struct obj_data *tobj;

  obj->in_obj = obj_to;

  // search through for the last object, or another object just like this one
  for(tobj = obj_to->contains;
      tobj && tobj->next_content;
      tobj = tobj->next_content)
  { 
    if(tobj->item_number == obj->item_number)
      break;
  }
     
  // put it in the list
  if(!tobj) {
    obj->next_content = NULL;
    obj_to->contains = obj;
  }
  else {
    obj->next_content = tobj->next_content;
    tobj->next_content = obj;
  }

  // recursively upwards add the weight.  Since we only have 1 layer of containers,
  // this loop only happens once, but it's good to leave later in case we change our mind
  if (obj_index[obj_to->item_number].virt != 536) {
  for(tobj = obj->in_obj; tobj;
      GET_OBJ_WEIGHT(tobj) += GET_OBJ_WEIGHT(obj),
      tobj = tobj->in_obj)
    ;
  }
  return 1;
}


// remove an object from an object
int obj_from_obj(struct obj_data *obj)
{
  struct obj_data *tmp, *obj_from;

  if (!obj->in_obj) {
    log("obj_from_obj called on an item that isn't inside another item.",
        OVERSEER, LOG_BUG);
    return 0;
  }

  obj_from = obj->in_obj;
	
  // head of list
  if (obj == obj_from->contains)
    obj_from->contains = obj->next_content;
    
  else {
    // locate previous
    for (tmp = obj_from->contains; tmp && (tmp->next_content != obj);
         tmp = tmp->next_content)
       ;

    if (!tmp) {
      perror("Fatal error in object structures.");
      abort();
    }

    tmp->next_content = obj->next_content;
  }


  // Subtract weight from containers container
  
  if (!obj_from || obj_index[obj_from->item_number].virt != 536) {
  for(tmp = obj->in_obj; tmp->in_obj; tmp = tmp->in_obj)
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
void object_list_new_new_owner(struct obj_data *list, CHAR_DATA *ch)
{
    if (list) {
	object_list_new_new_owner(list->contains, ch);
	object_list_new_new_owner(list->next_content, ch);
	list->carried_by = ch;
    }
}


// Extract an object from the world
void extract_obj(struct obj_data *obj)
{
    struct obj_data *temp1;
    //struct obj_data *temp2;
    struct active_object *active_obj = NULL,
                         *last_active = NULL;

bool has_random(OBJ_DATA *obj);

    if(obj_index[obj->item_number].non_combat_func ||
	obj->obj_flags.type_flag == ITEM_MEGAPHONE ||
	has_random(obj)) {
       active_obj = &active_head;
       while(active_obj) {
           if((active_obj->obj == obj) && (last_active)) {
                last_active->next = active_obj->next;
                dc_free(active_obj);
                break;
            } else if(active_obj->obj == obj) {
                active_head.obj = active_obj->next->obj;
		active_head.next = active_obj->next->next;
//		last_active->next = active_obj->next;
                dc_free(active_obj);
                break;
            }
            last_active = active_obj;
            active_obj = active_obj->next;
        }
    }

    // if we're going away, unhook myself from my owner
    if(GET_ITEM_TYPE(obj) == ITEM_BEACON && obj->equipped_by)
    {
       obj->equipped_by->beacon = 0;
       obj->equipped_by = 0;
    }
    if (obj->table) {
	extern void destroy_table(struct table_data *tbl);
	destroy_table(obj->table);
    }
    if(obj->equipped_by)
    {
            int iEq;
            char_data * vict = obj->equipped_by;
            for (iEq = 0; iEq < MAX_WEAR; iEq++) 
                if (vict->equipment[iEq] == obj) 
                {
                   obj_to_char(unequip_char(vict, iEq,1), vict);
                   break;
                }
     }

    if(obj->in_room != NOWHERE)
	obj_from_room(obj);
    else if(obj->carried_by)
	obj_from_char(obj);
    else if(obj->in_obj)
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

    for( ; obj->contains; extract_obj(obj->contains))
	; 
	/* leaves nothing ! */

    if (object_list == obj )       /* head of list */
	object_list = obj->next;
    else
    {
	for(temp1 = object_list; 
	    temp1 && (temp1->next != obj);
	    temp1 = temp1->next)
	    ;
	
	if(temp1)
	    temp1->next = obj->next;
    }

    if(obj->item_number>=0)
	(obj_index[obj->item_number].number)--;
    free_obj(obj);
}
	
void update_object( struct obj_data *obj, int use)
{
    if (obj->obj_flags.timer > 0)   obj->obj_flags.timer -= use;
    if (obj->contains)              update_object(obj->contains, use);
    if (obj->next_content)          update_object(obj->next_content, use);
}

void update_char_objects( CHAR_DATA *ch )
{
    int i;

    if (ch->equipment[WEAR_LIGHT])
	if (ch->equipment[WEAR_LIGHT]->obj_flags.type_flag == ITEM_LIGHT)
	    if (ch->equipment[WEAR_LIGHT]->obj_flags.value[2] > 0)
            {
		(ch->equipment[WEAR_LIGHT]->obj_flags.value[2])--;
                if(!ch->equipment[WEAR_LIGHT]->obj_flags.value[2])
                {
                    send_to_char("Your light flickers out and dies.\r\n", ch);
                    ch->glow_factor--;
                    if(ch->in_room > -1)
                      world[ch->in_room].light--;
                }
            }

    for(i = 0;i < MAX_WEAR;i++) 
	if (ch->equipment[i])
	    update_object(ch->equipment[i],2);

    if (ch->carrying) update_object(ch->carrying,1);
}



/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char(CHAR_DATA *ch, bool pull)
{
    CHAR_DATA *k, *next_char;
    struct descriptor_data *t_desc;
    int l, was_in;
    /*CHAR_DATA *i;*/
    bool isGolem = FALSE;
    extern CHAR_DATA *combat_list;
    void die_follower(CHAR_DATA *ch);
    void remove_from_bard_list(char_data * ch);
    void stop_guarding_me(char_data * victim);
    void stop_guarding(char_data * guard);
    struct obj_data *i;
    CHAR_DATA *omast = NULL;
    int ret = eSUCCESS;
    if ( !IS_NPC(ch) && !ch->desc )
       for ( t_desc = descriptor_list; t_desc; t_desc = t_desc->next )
	   if ( t_desc->original == ch )
	      ret = do_return( t_desc->character, "", 0 );
   if (SOMEONE_DIED(ret))
   { // already taken care of
   return;
   }
    if ( ch->in_room == NOWHERE ) {
	log( "Extract_char: NOWHERE", ANGEL, LOG_BUG );
        return;
    }
   remove_totem_stats(ch);
   if (!IS_NPC(ch)) {
	void shatter_message(CHAR_DATA *ch);
        void release_message(CHAR_DATA *ch);
	if (ch->pcdata->golem)
	{
	  if (ch->pcdata->golem->in_room)
	     release_message(ch->pcdata->golem);
	  extract_char(ch->pcdata->golem, FALSE);
	}
   }
   if (IS_NPC(ch) && mob_index[ch->mobdata->nr].virt == 8)
    {
	isGolem = TRUE;
        if (pull) {
	   if (ch->level > 1)
	   ch->level--;
	  ch->exp = 0; // Lower level, lose exp.
	}
	omast = ch->master;
    }

    if ( ch->followers || ch->master )
	die_follower(ch);

    // If I was the leader of a group, free the group name from memory
    if(ch->group_name)
    {
      dc_free(ch->group_name);
      ch->group_name = NULL;
      REMBIT(ch->affected_by,AFF_GROUP); // shrug
    }

    if ( ch->fighting )
	stop_fighting( ch );

    for ( k = combat_list; k ; k = next_char )
    {
	next_char = k->next_fighting;
	if ( k->fighting == ch )
	    stop_fighting( k );
    }
    // remove any and all affects from the character
    while(ch->affected)
      affect_remove(ch, ch->affected, SUPPRESS_ALL);

    /* Must remove from room before removing the equipment! */
    was_in = ch->in_room;
    char_from_room( ch );
    if ( !pull && !isGolem) {
      if (world[was_in].number == START_ROOM)
   	   char_to_room(ch, real_room(SECOND_START_ROOM));
      else
         char_to_room(ch, real_room(START_ROOM));
      }

    // make sure their ambush target is free'd
    if( ch->ambush ) {
       dc_free( ch->ambush );
       ch->ambush = NULL;
    }

    // I'm guarding someone.  Remove myself from their guarding list
    if( ch->guarding )
       stop_guarding(ch);

    // Someone is guarding me.  Let them know I don't exist anymore:)
    if( ch->guarded_by )
       stop_guarding_me(ch);

    // make sure no eq left on char.  But only if pulling completely
    if (pull) {
       for ( l = 0; l < MAX_WEAR; l++ ) {
          if ( ch->equipment[l] ) {
             obj_to_room(unequip_char(ch,l), was_in);
	  }
       }
       if (ch->carrying) {
	struct obj_data *inext;
         for(i = ch->carrying ; i ; i = inext)  {
	    inext = i->next_content;
	    obj_from_char(i);
            obj_to_room(i, was_in);
         }
       }
    }
    if (isGolem && omast)
    {
        do_save(omast,"",666);
        omast->pcdata->golem = 0; // Reset the golem flag.
    }


    GET_AC(ch) = 100;

    if ( ch->desc && ch->desc->original )
	do_return( ch, "", 12 );

    if ( IS_NPC(ch) && ch->mobdata->nr > -1 )
	mob_index[ch->mobdata->nr].number--;

    if ( pull || isGolem) {
        remove_from_bard_list(ch);
	if ( ch == character_list )
	   character_list = ch->next;
	else {
	    for ( k = character_list; k && k->next != ch; k = k->next )
		;
	    if ( k )
		k->next = ch->next;
	    else {
		log( "Extract_char: bad char list", ANGEL, LOG_BUG );
		abort();
	    }
	}
	free_char( ch );
    }
}



/* ***********************************************************************
   Here follows high-level versions of some earlier routines, ie functions
   which incorporate the actual player-data.
   *********************************************************************** */


CHAR_DATA *get_char_room_vis(CHAR_DATA *ch, char *name)
{
   CHAR_DATA *i;
   CHAR_DATA *partial_match;
   int j, number;
   char tmpname[MAX_INPUT_LENGTH];
   char *tmp;

   partial_match = 0;


   strcpy(tmpname,name);
   tmp = tmpname;
   if((number = get_number(&tmp))<0)
      return(0);

   for (i = world[ch->in_room].people, j = 0; i && (j <= number); i = i->next_in_room)
      {
      if (number == 0 && IS_NPC(i)) continue;
      if (number == 1 || number == 0)
         {
         if (isname(tmp, GET_NAME(i))&& CAN_SEE(ch,i))
            return(i);
         else if (isname2(tmp, GET_NAME(i))&& CAN_SEE(ch,i))
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
         if(isname(tmp, GET_NAME(i)) && CAN_SEE(ch,i))
            {
	    j++;
            if(j == number)
               return(i);
            }
         }
	   }
    return(partial_match);
}

CHAR_DATA *get_mob_room_vis(CHAR_DATA *ch, char *name)
{
   CHAR_DATA *i;
   CHAR_DATA *partial_match;
   int j, number;
   char tmpname[MAX_INPUT_LENGTH];
   char *tmp;

   partial_match = 0;


   strcpy(tmpname,name);
   tmp = tmpname;
   if((number = get_number(&tmp))<0)
      return(0);

   for (i = world[ch->in_room].people, j = 1; i && (j <= number); i = i->next_in_room)
   {
      if(!IS_MOB(i))
        continue;

      if (number == 1)
      {
         if (isname(tmp, GET_NAME(i))&& CAN_SEE(ch,i))
            return(i);
         else if (isname2(tmp, GET_NAME(i))&& CAN_SEE(ch,i))
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
         if(isname(tmp, GET_NAME(i)) && CAN_SEE(ch,i))
         {
            if(j == number)
               return(i);
            j++;
         }
      }
   }
   return(partial_match);
}

CHAR_DATA *get_pc_room_vis_exact(CHAR_DATA *ch, char *name)
{
   CHAR_DATA *i;

   for (i = world[ch->in_room].people; i ; i = i->next_in_room)
   {
         if (isname(name, GET_NAME(i)) && 
             CAN_SEE(ch,i) &&
             !IS_NPC(i)
            )
            return(i);
   }
   return NULL;
}



CHAR_DATA *get_mob_vis(CHAR_DATA *ch, char *name)
{
  CHAR_DATA *i;
  CHAR_DATA *partial_match;
  int j, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp;

  partial_match = 0;

  /* check location */
  if((i = get_mob_room_vis(ch, name)) != 0)
    return(i);

  strcpy(tmpname,name);
  tmp = tmpname;
  if((number = get_number(&tmp))<0)
    return(0);

  for(i = character_list, j = 1; i && (j <= number); i = i->next)
  {
      if (number == 1)
      {
         if (isname(tmp, GET_NAME(i))&& IS_NPC(i) && CAN_SEE(ch,i))
            return(i);
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
               return(i);
            j++;
         }
      }
   }
   return(partial_match);
}

OBJ_DATA *get_obj_vnum(int vnum)
{
  OBJ_DATA *i;
  int num = real_object(vnum);
  for (i = object_list; i; i = i->next)
    if (i->item_number == num)
      return i;
  return NULL;
}

CHAR_DATA *get_mob_vnum(int vnum)
{
  CHAR_DATA *i;
  int number = real_mobile(vnum);

  for(i = character_list; i; i = i->next)
  {
    if(IS_NPC(i) && i->mobdata->nr == number)
      return i;
  }
  return NULL;
}


CHAR_DATA *get_char_vis(CHAR_DATA *ch, char *name)
{
   CHAR_DATA *i;
   CHAR_DATA *partial_match;

   int j, number;
   char tmpname[MAX_INPUT_LENGTH];
   char *tmp;

   /* check location */
   if (( i = get_char_room_vis(ch, name) ) != 0 )
     return(i);

   partial_match = 0;

   strcpy(tmpname,name);
   tmp = tmpname;
   if((number = get_number(&tmp))<0)
	   return(0);

   for(i = character_list, j = 0; i && (j <= number); i = i->next)
      {
      if (number == 0 && IS_NPC(i)) continue;
      if (number == 1 || number == 0)
         {
         if (isname(tmp, GET_NAME(i)) && CAN_SEE(ch,i))
            return(i);
         else if (isname2(tmp, GET_NAME(i)) && CAN_SEE(ch,i))
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
         if(isname(tmp, GET_NAME(i)) && CAN_SEE(ch, i))
            {
	    j++;
            if(j == number)
               return(i);
            }
         }
	   }
   return(partial_match);
}

CHAR_DATA *get_pc(char *name)
{
   CHAR_DATA *i;


   for(i = character_list; i; i = i->next)
      if(!IS_NPC(i) && isname(name, GET_NAME(i)))
        return i; 
  return 0;
}

CHAR_DATA *get_active_pc(char *name)
{
   CHAR_DATA *i;
   CHAR_DATA *partial_match;
   struct descriptor_data *d;
   extern struct descriptor_data *descriptor_list;

   partial_match = 0;

   for(d = descriptor_list; d; d = d->next) 
      {
      if(!(i = d->character) || d->connected)
         continue;
      
      if (isname(name, GET_NAME(i)))
         return(i);
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

  return(partial_match);
}

CHAR_DATA *get_pc_vis(CHAR_DATA *ch, char *name)
{
   CHAR_DATA *i;
   CHAR_DATA *partial_match;

   partial_match = 0;

   for(i = character_list; i; i = i->next) 
      {
      if(!IS_NPC(i) && CAN_SEE(ch, i))
         {
         if (isname(name, GET_NAME(i)))
            return(i);
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

   return partial_match;
}


CHAR_DATA *get_pc_vis_exact(CHAR_DATA *ch, char *name)
{
   CHAR_DATA *i;

   for(i = character_list; i; i = i->next) 
      {
      if(!IS_NPC(i) && CAN_SEE(ch, i))
         {
         if (!strcmp(name, GET_NAME(i)))
            return(i);
         }
      }

   return 0;
}

CHAR_DATA *get_active_pc_vis(CHAR_DATA *ch, char *name)
{
   CHAR_DATA *i;
   CHAR_DATA *partial_match;
   struct descriptor_data *d;
   extern struct descriptor_data *descriptor_list;

   partial_match = 0;

   for(d = descriptor_list; d; d = d->next) 
      {
      if(!d->character || !GET_NAME(d->character)) 
         continue;
      i = d->character;
      if((d->connected) || (!CAN_SEE(ch, i)))
         continue;

      if(CAN_SEE(ch, i))
         {
         if (isname(name, GET_NAME(i)))
            return(i);
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
   return(partial_match);
}


// search by item number
struct obj_data *get_obj_in_list_vis(CHAR_DATA *ch, int item_num, 
		struct obj_data *list)
{
    struct obj_data *i;
    int number = real_object(item_num);

    // never match invalid items
    if(number == -1)
       return NULL;

    for (i = list ; i ; i = i->next_content)
       if (i->item_number == number && CAN_SEE_OBJ(ch, i))
          return i;

    return NULL;
}

struct obj_data *get_obj_in_list_vis(CHAR_DATA *ch, char *name, 
		struct obj_data *list)
{
    struct obj_data *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp;

    strcpy(tmpname,name);
    tmp = tmpname;
    if((number = get_number(&tmp))<0)
	return(0);

    for (i = list, j = 1; i && (j <= number); i = i->next_content)
	if (isname(tmp, i->name))
	    if (CAN_SEE_OBJ(ch, i)) {
		if (j == number)
		    return(i);
		j++;
	    }
    return(0);
}





/*search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(CHAR_DATA *ch, char *name, bool loc)
{
    struct obj_data *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp;

    /* scan items carried */
    if ( ( i = get_obj_in_list_vis(ch, name, ch->carrying) ) != NULL )
	return(i);

    /* scan room */
    if ( ( i = get_obj_in_list_vis(ch, name, world[ch->in_room].contents) )
    != NULL )
	return(i);

    strcpy(tmpname,name);
    tmp = tmpname;
    if ( ( number = get_number(&tmp) ) < 0 )
	return(0);

    /* ok.. no luck yet. scan the entire obj list   */
    for (i = object_list, j = 1; i && (j <= number); i = i->next)
    {
	if (loc && IS_SET(i->obj_flags.more_flags, ITEM_NOLOCATE) &&
		GET_LEVEL(ch) < 101) 
		continue;
	if (isname(tmp, i->name))
	    if (CAN_SEE_OBJ(ch, i)) {
		if (j == number)
		    return(i);
		j++;
	    }
    }
    return(0);
}


struct obj_data *create_money( int amount )
{
    struct obj_data *obj;
    struct extra_descr_data *new_new_descr;

    if(amount<=0)
    {
	log("ERROR: Try to create negative money.", ANGEL, LOG_BUG);
	return(0);
    }

#ifdef LEAK_CHECK
    obj = (struct obj_data *)calloc(1, sizeof(struct obj_data));
    new_new_descr = (struct extra_descr_data *)
      calloc(1, sizeof(struct extra_descr_data));
#else
    obj = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
    new_new_descr = (struct extra_descr_data *)
      dc_alloc(1, sizeof(struct extra_descr_data));
#endif

    clear_object(obj);

    if(amount==1)
    {
	obj->name = str_hsh("coin gold");
	obj->short_description = str_hsh("a gold coin");
	obj->description = str_hsh("One miserable gold coin.");

	new_new_descr->keyword = str_hsh("coin gold");
	new_new_descr->description = str_hsh("One miserable gold coin.");
    }
    else
    {
	obj->name = str_hsh("coins gold");
	obj->short_description = str_hsh("gold coins");
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
    obj->item_number = -1;

    obj->next = object_list;
    object_list = obj;

    return(obj);
}



/* Generic Find, designed to find any object/character                    */
/* Calling :                                                              */
/*  *arg     is the sting containing the string to be searched for.       */
/*           This string doesn't have to be a single word, the routine    */
/*           extracts the next word itself.                               */
/*  bitv..   All those bits that you want to "search through".            */
/*           Bit found will be result of the function                     */
/*  *ch      This is the person that is trying to "find"                  */
/*  **tar_ch Will be NULL if no character was found, otherwise points     */
/* **tar_obj Will be NULL if no object was found, otherwise points        */
/*                                                                        */
/* The routine returns a pointer to the next word in *arg (just like the  */
/* one_argument routine).                                                 */

int generic_find(char *arg, int bitvector, CHAR_DATA *ch,
		   CHAR_DATA **tar_ch, struct obj_data **tar_obj)
{
    static char *ignore[] = {
	"the",
	"in",
	"\n" };
    
    int i;
    char name[256];
    bool found;

    found = FALSE;


    /* Eliminate spaces and "ignore" words */
    while (*arg && !found) {

	for(; *arg == ' '; arg++)   ;

	for(i=0; (name[i] = *(arg+i)) && (name[i]!=' '); i++)   ;
	name[i] = 0;
	arg+=i;
	if (search_block(name, ignore, TRUE) > -1 || !IS_SET(bitvector, FIND_CHAR_ROOM))
	    found = TRUE;

    }

    if (!name[0])
	return(0);

    *tar_ch  = 0;
    *tar_obj = 0;

    if (IS_SET(bitvector, FIND_CHAR_ROOM)) {      /* Find person in room */
	if ( ( *tar_ch = get_char_room_vis(ch, name) ) != NULL ) {
	    return(FIND_CHAR_ROOM);
	}
    }

    if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
	if ( ( *tar_ch = get_char_vis(ch, name) ) != NULL ) {
	    return(FIND_CHAR_WORLD);
	}
    }

    if(IS_SET(bitvector, FIND_OBJ_EQUIP)) {
      for(found=FALSE, i=0; i < MAX_WEAR && !found; i++)
         if(ch->equipment[i] && isname(name, ch->equipment[i]->name) &&
		CAN_SEE_OBJ(ch, ch->equipment[i])) {
	   *tar_obj = ch->equipment[i];
	   found = TRUE;
	 }
      if(found) 
        return(FIND_OBJ_EQUIP);
    }

    if (IS_SET(bitvector, FIND_OBJ_INV)) {
	if ( ( *tar_obj = get_obj_in_list_vis(ch, name, ch->carrying) )
	!= NULL ) {
	    return(FIND_OBJ_INV);
	}
    }

    if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
	*tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents);
	if ( *tar_obj != NULL ) {
	    return(FIND_OBJ_ROOM);
	}
    }

    if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
	if ( ( *tar_obj = get_obj_vis(ch, name) ) != NULL ) {
	    return(FIND_OBJ_WORLD);
	}
    }

    return(0);
}

// Return a "somewhat" random person from the mobs hate list
// For now, just return the first one.  You can use "swap_hate_memory"
// to get a new one from 'get_random_hate'
char * get_random_hate(CHAR_DATA *ch)
{
    char buf[128];
    char * name = NULL;

    if(!IS_MOB(ch))
      return NULL;

    if(!ch->mobdata->hatred)
      return NULL;

    if(!strstr(ch->mobdata->hatred, " "))
      return (ch->mobdata->hatred);

    one_argument(ch->mobdata->hatred, buf);    

    name = str_hsh(buf);

    return name;  // This is okay, since it's a str_hsh
}

// Take the first name on the list, and swap it to the end.
void swap_hate_memory(CHAR_DATA *ch)
{
   char * curr;
   char temp[32];
   char buf[MAX_STRING_LENGTH];

   if(!IS_MOB(ch))
      return;

   if(!(curr = strstr(ch->mobdata->hatred, " ")))
      return;

   *curr = '\0';
   strcpy(temp, ch->mobdata->hatred);
   sprintf(buf, "%s %s", curr+1, ch->mobdata->hatred);
   strcpy(ch->mobdata->hatred, buf);
}

int hates_someone(CHAR_DATA *ch)
{
  if(!IS_MOB(ch))
    return 0;

  return (ch->mobdata->hatred != NULL);
}

int fears_someone(CHAR_DATA *ch)
{
  if(!IS_MOB(ch))
    return 0;

  return (ch->mobdata->fears != NULL);
}

void remove_memory(CHAR_DATA *ch, char type, CHAR_DATA *vict)
{
  char * temp = NULL;
  char * curr = NULL;

  if(type == 't')
    ch->hunting = 0;

  if(!IS_MOB(ch))
    return;

  if(type == 'h')
  {
    if(!ch->mobdata->hatred)
      return;
    if(!isname(GET_NAME(vict), ch->mobdata->hatred))
      return;
    if(strstr(ch->mobdata->hatred, " "))
    {
#ifdef LEAK_CHECK
      temp = (char *)calloc((strlen(ch->mobdata->hatred) - strlen(GET_NAME(vict))), sizeof(char));
#else
      temp = (char *)dc_alloc((strlen(ch->mobdata->hatred) - strlen(GET_NAME(vict))), sizeof(char));
#endif
      curr = strstr(ch->mobdata->hatred, GET_NAME(vict));
      if(curr == ch->mobdata->hatred)
      {
        // This has to work, cause we checked it on our first if statement
        curr = strstr(curr, " ");
        strcpy(temp, curr+1);
        dc_free(ch->mobdata->hatred);
        ch->mobdata->hatred = temp;
        return;
      }
      // This works because we made sure curr is not the first item in array
      // We're turning the 'space' into an end
      *(curr - 1) = '\0';
      strcpy(temp, ch->mobdata->hatred);
      // If there is anything else after the one we removed, slap it (and space) in
      if((curr = strstr(curr, " ")))
        strcat(temp, curr);
      dc_free(ch->mobdata->hatred);
      ch->mobdata->hatred = temp;
    }
    else {
      dc_free(ch->mobdata->hatred);
      ch->mobdata->hatred = NULL;
    }
  }

//  if(type == 'h')
//    ch->mobdata->hatred = 0;

  // these two are str_hsh'd, so just null them out
  if(type == 'f')
    ch->mobdata->fears = 0;
}

void remove_memory(CHAR_DATA *ch, char type)
{
  if(type == 't')
    ch->hunting = 0;

  if(!IS_MOB(ch))
    return;

  if(type == 'h' && ch->mobdata->hatred) {
    dc_free(ch->mobdata->hatred);
    ch->mobdata->hatred = NULL;
  }

  if(type == 'f')
    ch->mobdata->fears = 0;
}

void add_memory(CHAR_DATA *ch, char *victim, char type)
{
  char * buf = NULL;

  if(!IS_MOB(ch))
    return;

  // pets don't know to hate people
  if(IS_AFFECTED(ch, AFF_CHARM) || IS_AFFECTED(ch, AFF_FAMILIAR))
    return;

  if(type == 'h')
  {
    if(!ch->mobdata->hatred)
      ch->mobdata->hatred = str_dup(victim);
    else {
      // Don't put same name twice
      if(isname(victim, ch->mobdata->hatred))
        return;

      // name 1 + name 2 + a space + terminator
#ifdef LEAK_CHECK
      buf = (char *)calloc( (strlen(ch->mobdata->hatred) + strlen(victim) + 2), sizeof(char));
#else
      buf = (char *)dc_alloc( (strlen(ch->mobdata->hatred) + strlen(victim) + 2), sizeof(char));
#endif
      sprintf(buf, "%s %s", ch->mobdata->hatred, victim);
      dc_free(ch->mobdata->hatred);
      ch->mobdata->hatred  = buf;
    }
  }
  else if(type == 'f')
    ch->mobdata->fears   = str_hsh(victim);
  else if(type == 't')
    ch->hunting = str_dup(victim);
}




