 /***************************************************************************
 *  file: magic.c , Implementation of spells.              Part of DIKUMUD *
 *  Usage : The actual effect of magic.                                    *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as this   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: magic.cpp,v 1.124 2004/05/08 11:49:11 urizen Exp $ */
/***************************************************************************/
/* Revision History                                                        */
/* 11/24/2003   Onager   Changed spell_fly() and spell_water_breathing() to*/
/*                       quietly remove spell effects                      */
/* 12/07/2003   Onager   Changed PFE/PFG to check alignments, prevent      */
/*                       stacking, and only allow clerics to cast on others*/
/***************************************************************************/

extern "C"
{
#include <string.h>
#include <stdio.h>
#include <assert.h>
  #include <stdlib.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <room.h>
#include <obj.h>
#include <race.h>
#include <character.h>
#include <spells.h>
#include <magic.h>
#include <player.h>
#include <fight.h>
#include <utility.h>
#include <structs.h>
#include <handler.h>
#include <mobile.h>
#include <interp.h>
#include <levels.h>
#include <weather.h>
#include <isr.h>
#include <db.h>
#include <act.h>
#include <clan.h>
#include <arena.h>
#include <innate.h>
#include <returnvals.h>

/* Extern structures */
extern CWorld world;
extern struct room_data ** world_array;
 
extern struct obj_data  *object_list;
extern CHAR_DATA *character_list;
extern struct zone_data *zone_table;

#define BEACON_OBJ_NUMBER 405

/* Extern procedures */

int saves_spell(CHAR_DATA *ch, CHAR_DATA *vict, int spell_base, sh_int save_type);
struct clan_data * get_clan(struct char_data *);

int dice(int number, int size);
void set_cantquit(CHAR_DATA *ch, CHAR_DATA *victim, bool forced = FALSE);
void update_pos( CHAR_DATA *victim );
bool many_charms(CHAR_DATA *ch);
bool ARE_GROUPED( CHAR_DATA *sub, CHAR_DATA *obj);
void add_memory(CHAR_DATA *ch, char *victim, char type);

bool resist_spell(int perc)
{
  if (number(1,101) > perc)
    return TRUE;
  return FALSE;
}

bool resist_spell(CHAR_DATA *ch, int skill)
{
  int perc = has_skill(ch,skill);
  if (number(1,101) > perc)
    return TRUE;
  return FALSE;
}

/* Offensive Spells */

int spell_magic_missile(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int count = 1;
  int retval = eSUCCESS;

  set_cantquit( ch, victim );
//  dam = level;
  dam = 50;
  count += (skill > 15) + (skill > 35) + (skill > 55) + (skill > 75); 

  skill_increase_check(ch, SPELL_MAGIC_MISSILE, skill, SKILL_INCREASE_MEDIUM);

  while(!SOMEONE_DIED(retval) && count--)
     retval = damage(ch, victim, dam, TYPE_PHYSICAL_MAGIC, SPELL_MAGIC_MISSILE, 0);

  return retval;
}

int spell_chill_touch(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int dam;
  int save;

  set_cantquit( ch, victim );

//  dam = dice(2,8) * level/2;
  dam = 250;
//  save = saves_spell(ch, victim, (level/2), SAVE_TYPE_COLD);
  
  skill_increase_check(ch, SPELL_CHILL_TOUCH, skill, SKILL_INCREASE_MEDIUM);

  if(save < 0 && skill > 50) // if failed
  {
    af.type      = SPELL_CHILL_TOUCH;
    af.duration  = 6;
    af.modifier  = -5;
    af.location  = APPLY_STR;
    af.bitvector = 0;
    affect_join(victim, &af, TRUE, FALSE);
  } 

  // modify the damage by how much they resisted
  dam += (int) (dam * (save/100));
  
  return spell_damage(ch, victim, dam, TYPE_COLD, SPELL_CHILL_TOUCH, 0);
}

int spell_burning_hands(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int save;

  set_cantquit( ch, victim );

//  dam = dice(3,8) + level / 2 + skill / 10;
  dam = 125;
//  save = saves_spell(ch, victim, 3, SAVE_TYPE_FIRE);

  // modify the damage by how much they resisted
  //dam += (int) (dam * (save/100));

  skill_increase_check(ch, SPELL_BURNING_HANDS, skill, SKILL_INCREASE_MEDIUM);

  return spell_damage(ch, victim, dam, TYPE_FIRE, SPELL_BURNING_HANDS, 0);
}

int spell_shocking_grasp(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int save;

  dam = 125;
//  save = saves_spell(ch, victim, 5, SAVE_TYPE_ENERGY);

  // modify the damage by how much they resisted
  //dam += (int) (dam * (save/100));

  skill_increase_check(ch, SPELL_SHOCKING_GRASP, skill, SKILL_INCREASE_MEDIUM);

  return spell_damage(ch, victim, dam, TYPE_ENERGY, SPELL_SHOCKING_GRASP, 0);
}

int spell_lightning_bolt(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int save;

  set_cantquit( ch, victim );

//  dam = (level + GET_INT(ch) + skill / 10) * 3;
  dam = 200;
//  save = saves_spell(ch, victim, 10, SAVE_TYPE_ENERGY);

  // modify the damage by how much they resisted
  //dam += (int) (dam * (save/100));

  skill_increase_check(ch, SPELL_LIGHTNING_BOLT, skill, SKILL_INCREASE_MEDIUM);

  return spell_damage(ch, victim, dam, TYPE_ENERGY, SPELL_LIGHTNING_BOLT, 0);
}

int spell_colour_spray(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;

   set_cantquit( ch, victim );
//   dam = level * 4;
   dam = 350;
   skill_increase_check(ch, SPELL_COLOUR_SPRAY, skill, SKILL_INCREASE_MEDIUM);

   int retval = spell_damage(ch, victim, dam, TYPE_MAGIC, SPELL_COLOUR_SPRAY, 0);

   if(SOMEONE_DIED(retval))
     return retval;

   int save = saves_spell(ch, victim, 30, SAVE_TYPE_MAGIC);
   if(save < 0 && (skill > 50 || IS_NPC(ch)) ) {
     act("$N blinks in confusion from the distraction of the color spray.", ch, 0, victim, TO_ROOM, NOTVICT);
     act("Brilliant streams of color streak from $n's fingers!  WHOA!  Cool!", ch, 0, victim, TO_VICT, 0 );
     act("Your colors of brilliance dazzle the simpleminded $N.", ch, 0, victim, TO_CHAR, 0 );
     SET_BIT(victim->combat, COMBAT_SHOCKED);
   }

   return retval;
}

int spell_drown(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;

   if(world[ch->in_room].sector_type == SECT_DESERT) {
     send_to_char("You're trying to drown someone in the desert?  Get a clue!\r\n", ch);
     return eFAILURE;
   }
   if(world[ch->in_room].sector_type == SECT_UNDERWATER) {
     send_to_char("Hello!  You're underwater!  *knock knock*  Anyone home?\r\n", ch);
     return eFAILURE;
   }

   set_cantquit( ch, victim );
//   dam = level * 4 + skill/5;
  dam = 250;
   skill_increase_check(ch, SPELL_DROWN, skill, SKILL_INCREASE_MEDIUM);

   return spell_damage(ch, victim, dam, TYPE_WATER, SPELL_DROWN, 0);
}

// Drain XP, MANA, HP - caster gains HP and MANA
int spell_energy_drain(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam, xp, mana;

   if (GET_ALIGNMENT(ch) > -1) {
      send_to_char("You aren't evil enough to harness such magic!\n\r", ch);
      return eFAILURE;
      }

   if(IS_SET(world[ch->in_room].room_flags, SAFE))
     return eFAILURE;

   set_cantquit( ch, victim );

   if(saves_spell(ch, victim, 1, SAVE_TYPE_MAGIC) < 0) 
   {
      if (GET_LEVEL(victim) <= 2) {
         return spell_damage(ch, victim, 100, TYPE_ENERGY, SPELL_ENERGY_DRAIN, 0);
	 }
      else {
         xp = number(level>>1, level)*1000;
         gain_exp(victim, -xp);

         dam = dice(1,10);

         mana = GET_MANA(victim)>>1;
         GET_MOVE(victim) >>= 1;
         GET_MANA(victim) = mana;

         GET_MANA(ch) += mana>>1;
         GET_HIT(ch) += dam;

         send_to_char("Your life energy is drained!\n\r", victim);

         return spell_damage(ch, victim, dam, TYPE_ENERGY, SPELL_ENERGY_DRAIN, 0);
      }
   } // saves spell
      
   // Miss
   else {
      return spell_damage(ch, victim, 0, TYPE_ENERGY, SPELL_ENERGY_DRAIN, 0); 
      }
}

// Drain XP, MANA, HP - caster gains HP and MANA
int spell_souldrain(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam, xp, mana;
//   level *= 2;
   set_cantquit( ch, victim );

   if(saves_spell(ch, victim, 1, SAVE_TYPE_MAGIC) < 0) {
      if (GET_LEVEL(victim) <= 2) {
         return spell_damage(ch, victim, 100, TYPE_MAGIC, SPELL_SOULDRAIN, 0);
	 }
      else {
//         xp = number(level>>1, level)*1000;
//         gain_exp(victim, -xp);

//         dam = dam_percent(skill,125);

         mana = dam_percent(skill,125);
         if(mana > GET_MANA(victim))
            mana = GET_MANA(victim);
    //     GET_MOVE(victim) >>= 1;
         GET_MANA(victim) -= mana;

         GET_MANA(ch) += mana;
  //       GET_HIT(ch) += dam;
	 send_to_char("You drain their very soul!\r\n",ch);
         send_to_char("You feel your very soul being drained!\n\r", victim);

         return eSUCCESS;//spell_damage(ch, victim, dam, TYPE_MAGIC, 
//SPELL_SOULDRAIN, 0);
         }
      } // ! saves spell
      
   // Miss
   else {
      return spell_damage(ch, victim, 0, TYPE_MAGIC, SPELL_SOULDRAIN, 0); 
      }
}

int spell_vampiric_touch (byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;

  set_cantquit( ch, victim );

//  dam = dice((level / 2) , 15);

  dam = 200;
  int adam = dam_percent(skill, 200); // Actual dam, for drainy purposes.
  skill_increase_check(ch, SPELL_VAMPIRIC_TOUCH, skill, SKILL_INCREASE_HARD);

  if(saves_spell(ch, victim, ( skill / 3 ), SAVE_TYPE_MAGIC) < 0) 
  {
    if (GET_HIT(victim) < adam)
       GET_HIT(ch) += GET_HIT(victim);
    else GET_HIT(ch) += adam;

    if (GET_HIT(ch) > GET_MAX_HIT(ch))
       GET_HIT(ch) = GET_MAX_HIT(ch);

    return spell_damage (ch, victim, dam,TYPE_ENERGY, SPELL_VAMPIRIC_TOUCH, 0);
  } else {
    dam >>= 1;
    adam >>= 1;
    if (GET_HIT(victim) < adam)
       GET_HIT(ch) += GET_HIT(victim);
    else GET_HIT(ch) += adam;

    if (GET_HIT(ch) > GET_MAX_HIT(ch))
       GET_HIT(ch) = GET_MAX_HIT(ch);

    return spell_damage (ch, victim, dam,TYPE_ENERGY, SPELL_VAMPIRIC_TOUCH, 0);
  }
}


int spell_meteor_swarm(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;

  set_cantquit( ch, victim );
//  dam = ((level * 2) + (GET_INT(ch) * 3)) * 3 + skill / 2;
  dam = 500;
  skill_increase_check(ch, SPELL_METEOR_SWARM, skill, SKILL_INCREASE_MEDIUM);

//  int save = saves_spell(ch, victim, 50, SAVE_TYPE_MAGIC);

  // modify the damage by how much they resisted
//  dam += (int) (dam * (save/100));

  return spell_damage(ch, victim, dam,TYPE_MAGIC, SPELL_METEOR_SWARM, 0);
}

int spell_fireball(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;

   set_cantquit( ch, victim );
//   dam = ((level * 2) + GET_INT(ch)) * 2;
  dam = 300;
//   int save = saves_spell(ch, victim, 25, SAVE_TYPE_FIRE);

   skill_increase_check(ch, SPELL_FIREBALL, skill, SKILL_INCREASE_MEDIUM);

   // modify the damage by how much they resisted
  // dam += (int) (dam * (save/100));

   int retval = spell_damage(ch, victim, dam, TYPE_FIRE, SPELL_FIREBALL, 0);

   if(SOMEONE_DIED(retval) || ch->in_room != victim->in_room)
     return retval;
   // Above: Fleeing now saves you from the second blast.
   if(number(0, 100) < ( skill / 5 ) ) {
     act("The expanding flames suddenly recombine and fly at $N again!", ch, 0, victim, TO_ROOM, 0);
     act("The expanding flames suddenly recombine and fly at $N again!", ch, 0, victim, TO_CHAR, 0);
//     save = saves_spell(ch, victim, 25, SAVE_TYPE_FIRE);
  //   dam += (int) (dam * (save/100));
     retval = spell_damage(ch, victim, dam, TYPE_FIRE, SPELL_FIREBALL, 0);
   }
   return retval;
}

int spell_sparks(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;

   set_cantquit( ch, victim );
   dam = dice(level, 2);

   skill_increase_check(ch, SPELL_SPARKS, skill, SKILL_INCREASE_MEDIUM);

//   int save = saves_spell(ch, victim, ( skill / 5 ), SAVE_TYPE_ENERGY);

  // if(save >= 0)
    // dam >>= 1;

   return spell_damage(ch, victim, dam, TYPE_FIRE, SPELL_SPARKS, 0);
}


int spell_howl(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   char_data * tmp_char;
   int retval;

   set_cantquit( ch, victim );

   if(saves_spell(ch, victim, 5, SAVE_TYPE_MAGIC) >= 0) {
      return spell_damage(ch, victim, 0, TYPE_FIRE, SPELL_HOWL, 0);
   }

   retval = spell_damage(ch, victim, 8, TYPE_FIRE, SPELL_HOWL, 0);

   if(SOMEONE_DIED(retval))
     return retval;

   for (tmp_char = world[ch->in_room].people; tmp_char;
        tmp_char = tmp_char->next_in_room) {
      
      if (!tmp_char->master)
         continue;
	 
      if (tmp_char->master == ch || ARE_GROUPED(tmp_char->master, ch))
         continue;
	 
      if (affected_by_spell(tmp_char, SPELL_CHARM_PERSON)) {
         affect_from_char(tmp_char, SPELL_CHARM_PERSON);
         send_to_char("You feel less enthused about your master.\n\r",
	              tmp_char);
         act("$N blinks and shakes its head, clearing its thoughts.",
                ch, 0, tmp_char, TO_CHAR, 0);
         act("$N blinks and shakes its head, clearing its thoughts.\n\r",
		ch, 0, tmp_char, TO_ROOM, NOTVICT);
	if (ch->fighting)
	{
	   do_say(ch,"Screw this! I'm going home!", 9);
	   if (ch->fighting->fighting == ch)
	     stop_fighting(ch->fighting);
	   stop_fighting(ch);
	   
	}
         }
      
      } // for loop through people in the room
   return retval;
}


int spell_armor(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  skill_increase_check(ch, SPELL_ARMOR, skill, SKILL_INCREASE_EASY);

  if(affected_by_spell(victim, SPELL_ARMOR))
    affect_from_char(victim, SPELL_ARMOR);

  af.type      = SPELL_ARMOR;
  af.duration  = 24 + level / 2;
  af.modifier  = -20 - skill / 6;
  af.location  = APPLY_AC;
  af.bitvector = 0;

  affect_to_char(victim, &af);
  send_to_char("You feel someone protecting you.\n\r", victim);
  return eSUCCESS;
}

int spell_stone_shield(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  char buf[160];

  if(affected_by_spell(victim, SPELL_GREATER_STONE_SHIELD)) {
    sprintf(buf, "%s is already surrounded by a greater stone shield.\r\n", GET_SHORT(victim));
    send_to_char(buf, ch);
    return eSUCCESS;
  }

  skill_increase_check(ch, SPELL_STONE_SHIELD, skill, SKILL_INCREASE_MEDIUM);

  if(affected_by_spell(victim, SPELL_STONE_SHIELD))
    affect_from_char(victim, SPELL_STONE_SHIELD);

  af.type      = SPELL_STONE_SHIELD;
  af.duration  = 5 + (skill / 10) + (GET_WIS(ch) > 23);
  af.modifier  = -30;
  af.location  = 0;
  af.bitvector = 0;

  affect_to_char(victim, &af);
  send_to_char("A shield of ethereal stones begins to swirl around you.\n\r", victim);
  act("Ethereal stones form out of nothing and begin to swirl around $n.", 
    victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

int cast_stone_shield( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	case SPELL_TYPE_SPELL:
                 if(tar_ch->fighting) {
                   send_to_char("The combat disrupts the ether too much to coalesce into stones.\r\n", ch);
                   return eFAILURE;
                 }
		 return spell_stone_shield(level,ch,tar_ch,0, skill);
		 break;
	case SPELL_TYPE_POTION:
		 return spell_stone_shield(level,ch,ch,0, skill);
		 break;
	case SPELL_TYPE_SCROLL:
		 if (tar_obj) return eFAILURE;
		 if (!tar_ch) tar_ch = ch;
		 return spell_stone_shield(level,ch,ch,0, skill);
		 break;
	case SPELL_TYPE_WAND:
		 if (tar_obj) return eFAILURE;
		 if (!tar_ch) tar_ch = ch;
		 return spell_stone_shield(level,ch,tar_ch,0, skill);
		 break;
		default :
	 log("Serious screw-up in stone_shield!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int spell_greater_stone_shield(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  char buf[160];

  if(affected_by_spell(victim, SPELL_STONE_SHIELD) || affected_by_spell(victim, SPELL_GREATER_STONE_SHIELD)) {
    sprintf(buf, "%s is already surrounded by a stone shield.\r\n", GET_SHORT(victim));
    send_to_char(buf, ch);
    return eSUCCESS;
  }

  skill_increase_check(ch, SPELL_GREATER_STONE_SHIELD, skill, SKILL_INCREASE_MEDIUM);

  af.type      = SPELL_GREATER_STONE_SHIELD;
  af.duration  = 5 + (skill / 7) + (GET_WIS(ch) > 23);
  af.modifier  = -50;
  af.location  = 0;
  af.bitvector = 0;

  affect_to_char(victim, &af);
  send_to_char("A shield of ethereal stones begins to swirl around you.\n\r", victim);
  act("Ethereal stones form out of nothing and begin to swirl around $n.", 
    victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

int cast_greater_stone_shield( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	case SPELL_TYPE_SPELL:
                 if(tar_ch->fighting) {
                   send_to_char("The combat disrupts the ether too much to coalesce into stones.\r\n", ch);
                   return eFAILURE;
                 }
		 return spell_stone_shield(level,ch,tar_ch,0, skill);
		 break;
	case SPELL_TYPE_POTION:
		 return spell_stone_shield(level,ch,ch,0, skill);
		 break;
	case SPELL_TYPE_SCROLL:
		 if (tar_obj) return eFAILURE;
		 if (!tar_ch) tar_ch = ch;
		 return spell_stone_shield(level,ch,ch,0,skill);
		 break;
	case SPELL_TYPE_WAND:
		 if (tar_obj) return eFAILURE;
		 if (!tar_ch) tar_ch = ch;
		 return spell_stone_shield(level,ch,tar_ch,0, skill);
		 break;
		default :
	 log("Serious screw-up in stone_shield!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int spell_earthquake(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int retval = 0;
  CHAR_DATA *tmp_victim, *temp;

  skill_increase_check(ch, SPELL_EARTHQUAKE, skill, SKILL_INCREASE_MEDIUM);

//  dam =  dice(level,5) + skill / 2;
  dam = 150;
  send_to_char("The earth trembles beneath your feet!\n\r", ch);
  act("$n makes the earth tremble and shiver.\n\rYou fall, and hit yourself!\n\r",
		ch, 0, 0, TO_ROOM, 0);

  for(tmp_victim = character_list; (tmp_victim && !IS_SET(retval, eCH_DIED)); tmp_victim = temp)
  {
	 temp = tmp_victim->next;
	 if ( (ch->in_room == tmp_victim->in_room) 
           && (ch != tmp_victim) 
           && (!ARE_GROUPED(ch,tmp_victim))) 
         {
                  if(IS_NPC(ch) && IS_NPC(tmp_victim)) // mobs don't earthquake each other
                    continue;
                  if(IS_AFFECTED2(tmp_victim, AFF_FREEFLOAT))
                    retval = spell_damage(ch, tmp_victim, 0, TYPE_MAGIC, SPELL_EARTHQUAKE, 0);
                  else retval = spell_damage(ch, tmp_victim, dam, TYPE_MAGIC, SPELL_EARTHQUAKE, 0);
	 } 
         else if (world[ch->in_room].zone == world[tmp_victim->in_room].zone)
           send_to_char("The earth trembles and shivers.\n\r", tmp_victim);
  }
  return eSUCCESS;
}

int spell_life_leech(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam,retval = eSUCCESS;
  CHAR_DATA *tmp_victim, *temp;

  if(IS_SET(world[ch->in_room].room_flags, SAFE)) 
    return eFAILURE;

  for(tmp_victim = character_list; tmp_victim; tmp_victim = temp)
  {
	 temp = tmp_victim->next;
	 if ( (ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
		(!ARE_GROUPED(ch,tmp_victim)))
	{

	//	 dam = dice(8, 6);
		dam = 75;
		int adam = dam_percent(skill,75);
//		 if(saves_spell(ch, tmp_victim, 10, SAVE_TYPE_MAGIC) >= 0)
//		    dam >>= 1;

		 if (GET_HIT(tmp_victim) < adam)
			  GET_HIT(ch) += GET_HIT(tmp_victim);
		 else GET_HIT(ch) += adam;


		skill_increase_check(ch, SPELL_LIFE_LEECH, skill, SKILL_INCREASE_MEDIUM);

		 if (GET_HIT(ch) > GET_MAX_HIT(ch))
			GET_HIT(ch) = GET_MAX_HIT(ch);
		 retval &= spell_damage (ch, tmp_victim, dam,TYPE_ENERGY, SPELL_LIFE_LEECH, 0);
	}
  }
  return retval;
}

void do_solar_blind(CHAR_DATA *ch, CHAR_DATA *tmp_victim)
{
  struct affected_type af;

  if(!ch || !tmp_victim)
  {
    log("Null ch or vict in solar_blind", IMMORTAL, LOG_BUG);
    return;
  }
  if(saves_spell(ch, tmp_victim, -5, SAVE_TYPE_MAGIC) < 0)
    if(!IS_AFFECTED(tmp_victim, AFF_BLIND)) 
    {
      act("$n seems to be blinded!", tmp_victim, 0, 0, TO_ROOM, INVIS_NULL);
      send_to_char("The world dims and goes black as you are blinded!\n\r", tmp_victim);

      af.type      = SPELL_BLINDNESS;
      af.location  = APPLY_HITROLL;
      af.modifier  = -30;  // Make hitroll worse
      af.duration  = 4;
      af.bitvector = AFF_BLIND;
      affect_to_char(tmp_victim, &af);
      af.location = APPLY_AC;
      af.modifier = +90; // Make AC Worse!
      affect_to_char(tmp_victim, &af);
    } // if affect by blind
}

int spell_solar_gate(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int i;
  int dam;
  int retval;
  CHAR_DATA *tmp_victim, *temp;
  int orig_room;

  char *dirs[] = {
    "from the South.",
    "from the West.",
    "from the North.",
    "from the East.",
    "from Below.",
    "from Above.",
    "\n"
  };

  int to_charge[6] = {
    3,
    4,
    1,
    2,
    6,
    5
  };

  skill_increase_check(ch, SPELL_SOLAR_GATE, skill, SKILL_INCREASE_HARD);

  // Do room i'm in
  send_to_char("A Bright light comes down from the heavens.\n\r", ch);
  act("$n opens a Solar Gate.\n\r You are ENVELOPED in a PAINFUL BRIGHT "
      "LIGHT!", ch, 0, 0, TO_ROOM, 0);

  // we use "orig_room" for this now, instead of ch->in_room.  The reason for
  // this, is so if we die from a reflect, we don't keep looping through and
  // solaring from our new location.  Since the solar is theoretically hitting
  // everyone at the same time, we can't just break out. -pir 12/26

  orig_room = ch->in_room;

  // we also now use .people instead of the character_list -pir 12/26
  for(tmp_victim = world[orig_room].people; tmp_victim; tmp_victim = temp) 
  {
     temp = tmp_victim->next_in_room;
     if((orig_room == tmp_victim->in_room) && (tmp_victim != ch) &&
        (!ARE_GROUPED(ch,tmp_victim)) && (can_be_attacked(ch, tmp_victim)))
     {
//       dam = dice(level, 20) + skill;
       dam = 600;
       retval = spell_damage(ch, tmp_victim, dam,TYPE_MAGIC, SPELL_SOLAR_GATE, 0);
       if(IS_SET(retval, eCH_DIED))
	 return retval;
       if(!IS_SET(retval, eVICT_DIED))
         do_solar_blind(ch, tmp_victim);
     }  // if are grouped, etc
  } // for

  // Do surrounding rooms
  for(i = 0; i < 6; i++) {
    if(CAN_GO(ch, i)) {
      for(tmp_victim = world[world[orig_room].dir_option[i]->to_room].people;
           tmp_victim; tmp_victim = temp) 
      {
          temp = tmp_victim->next_in_room;

	  if((tmp_victim != ch) && (tmp_victim->in_room != orig_room) &&
             (!ARE_GROUPED(ch, tmp_victim)) &&
	     (can_be_attacked(tmp_victim, tmp_victim)))
          {
            dam = 300; //dice(level, 10) + skill/2;
            csendf(tmp_victim,"You are ENVELOPED in a PAINFUL BRIGHT LIGHT "
	            " pouring in %s.\n\r", dirs[i]);

            retval = spell_damage(ch, tmp_victim, dam, TYPE_MAGIC, SPELL_SOLAR_GATE, 0);
            if(IS_SET(retval, eCH_DIED))
              return retval;

            if(!IS_SET(retval, eVICT_DIED))
            {
              //don't blind surrounding rooms
              //do_solar_blind(ch, tmp_victim);
              if(GET_LEVEL(tmp_victim))
                if(IS_NPC(tmp_victim))  {
                  add_memory(tmp_victim, GET_NAME(ch), 'h');
  if (!IS_NPC(ch) && IS_NPC(tmp_victim))
     if (!IS_SET(tmp_victim->mobdata->actflags, ACT_STUPID) && !tmp_victim->hunting) 
     {
       if (GET_LEVEL(ch) - GET_LEVEL(tmp_victim)/2 > 0)
          {
                add_memory(tmp_victim, GET_NAME(ch), 't');
                struct timer_data *timer;
                #ifdef LEAK_CHECK
                  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
                #else
                  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
                #endif
                timer->arg1 = (void*)tmp_victim->hunting;
                timer->arg2 = (void*)tmp_victim;
                timer->function = clear_hunt;
               timer->next = timer_list;
                timer_list = timer;
                timer->timeleft = (ch->level/4)*60;
           }
     }


if(GET_POS(tmp_victim) != POSITION_STANDING)
                    GET_POS(tmp_victim) = POSITION_STANDING;
		  if(!IS_AFFECTED(tmp_victim, AFF_BLIND) &&
                     !IS_SET(tmp_victim->mobdata->actflags, ACT_STUPID))
		    do_move(tmp_victim, "", to_charge[i]);
	        }
            } // if ! eVICT_DIED
          } // if are grouped, etc
       } // for tmp victim
     }  // if can go
  } // for i 0 < 6
  return eSUCCESS;
}

int spell_group_recall(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  CHAR_DATA *tmp_victim, *temp;
  int chance = 90 + skill / 10;

  skill_increase_check(ch, SPELL_GROUP_RECALL, skill, SKILL_INCREASE_EASY);

  for(tmp_victim = character_list; tmp_victim; tmp_victim = temp) 
  {
    temp = tmp_victim->next;
    if(ch->in_room == tmp_victim->in_room && 
       tmp_victim != ch                   &&
       ARE_GROUPED(ch, tmp_victim)
      ) 
    {
      if(!tmp_victim) {
        log("Bad character in character_list in magic.c in group-recall!", ANGEL, LOG_BUG);
        return eFAILURE|eINTERNAL_ERROR;
      }
      if(number(1, 100) < chance)
        spell_word_of_recall(level, ch, tmp_victim, obj, 110);
      else
        csendf(tmp_victim, "%s's group recall partially fails leaving you behind!\r\n", ch->name);
    }
  }
  return spell_word_of_recall(level, ch, ch, obj, skill);
}



int spell_group_fly(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  /*int dam;*/
  CHAR_DATA *tmp_victim, *temp;

  skill_increase_check(ch, SPELL_GROUP_FLY, skill, SKILL_INCREASE_MEDIUM);

  for(tmp_victim = character_list; tmp_victim; tmp_victim = temp)
  {
    temp = tmp_victim->next;
    if ( (ch->in_room == tmp_victim->in_room) &&
         (ARE_GROUPED(ch,tmp_victim) ))
    {
       if(!tmp_victim)
       {
          log("Bad tmp_victim in character_list in group fly!", ANGEL, LOG_BUG);
          return eFAILURE;
       }
       spell_fly(level,ch, tmp_victim, obj, skill);
    }
  }
  return eSUCCESS;
}


int spell_heroes_feast(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  CHAR_DATA *tmp_victim, *temp;
  int result = 15 + skill / 6;

  skill_increase_check(ch, SPELL_HEROES_FEAST, skill, SKILL_INCREASE_EASY);

  if (GET_COND(ch, FULL) > -1) 
  {
    GET_COND(ch, FULL) = result;
    GET_COND(ch, THIRST) = result;
  }
  send_to_char("You partake in a magnificent feast!\n\r",ch);
  for(tmp_victim = character_list; tmp_victim; tmp_victim = temp)
  {
    temp = tmp_victim->next;
    if ( (ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
		(ARE_GROUPED(ch,tmp_victim) ))
    {
       if (GET_COND(tmp_victim, FULL) > -1) 
       {
          GET_COND(tmp_victim, FULL) = result;
          GET_COND(tmp_victim, THIRST) = result;
       }
       send_to_char("You partake in a magnificent feast!\n\r",tmp_victim);
    }
  }
  return eSUCCESS;
}



int spell_group_sanc(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  CHAR_DATA *tmp_victim, *temp;

  skill_increase_check(ch, SPELL_GROUP_SANC, skill, SKILL_INCREASE_MEDIUM);

  for(tmp_victim = character_list; tmp_victim; tmp_victim = temp) {
    temp = tmp_victim->next;
    if((ch->in_room == tmp_victim->in_room) && (ARE_GROUPED(ch,tmp_victim))) {
      if(!tmp_victim) {
        log("Bad tmp_victim in character_list in group fly!", ANGEL, LOG_BUG);
        return eFAILURE|eINTERNAL_ERROR;
      }
      spell_sanctuary(level, ch, tmp_victim, obj, skill);
    }
  }
  return eSUCCESS;
}

int spell_heal_spray(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  /*int dam;*/
  CHAR_DATA *tmp_victim, *temp;


  for(tmp_victim = character_list; tmp_victim; tmp_victim = temp)
  {
	 temp = tmp_victim->next;
	 if ( (ch->in_room == tmp_victim->in_room) &&
		(ARE_GROUPED(ch,tmp_victim) )){

		 spell_heal(level,ch, tmp_victim, obj, skill);
	 }
  }
  return eSUCCESS;
}


int spell_firestorm(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int retval = eSUCCESS;
  CHAR_DATA *tmp_victim, *temp;

 skill_increase_check(ch, SPELL_FIRESTORM, skill, SKILL_INCREASE_MEDIUM);

  send_to_char("Fire falls from the heavens!\n\r", ch);
  act("$n makes fire fall from the heavens!\n\rYour flesh is seared off by scorching flames!",
		ch, 0, 0, TO_ROOM, 0);

  for(tmp_victim = character_list; tmp_victim; tmp_victim = temp)
  {
	 temp = tmp_victim->next;
	 if ( (ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
		(!ARE_GROUPED(ch,tmp_victim) )){

	  dam = 250;
	  retval = spell_damage(ch, tmp_victim, dam,TYPE_FIRE, SPELL_FIRESTORM, 0);
          if(IS_SET(retval, eCH_DIED))
            return retval;
	 }
	 else
	{
   	if (world[ch->in_room].zone == world[tmp_victim->in_room].zone)
 	  send_to_char("You feel a HOT blast of air.\n\r", tmp_victim);
	 }
  }
  return retval;
}

int spell_dispel_evil(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;

  set_cantquit( ch, victim );

  if (IS_EVIL(ch))
	 victim = ch;

  if (IS_NEUTRAL(victim) || IS_GOOD(victim))
  {
	act("$N does not seem to be affected.", ch, 0, victim, TO_CHAR, 0);
	return eFAILURE;
  }

  skill_increase_check(ch, SPELL_DISPEL_EVIL, skill, SKILL_INCREASE_MEDIUM);

  dam = dice(skill, 4);
  dam = 225;
//  int save = saves_spell(ch, victim, 25, SAVE_TYPE_MAGIC);
  //dam += (int) (dam * (save/100));

  return spell_damage(ch, victim, dam, TYPE_MAGIC, SPELL_DISPEL_EVIL, 0);
}

int spell_dispel_good(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;

  set_cantquit( ch, victim );

  if (IS_GOOD(ch))
	 victim = ch;

  if (IS_NEUTRAL(victim) || IS_EVIL(victim))
  {
	act("$N does not seem to be affected.", ch, 0, victim, TO_CHAR, 0);
	return eFAILURE;
  }

  skill_increase_check(ch, SPELL_DISPEL_GOOD, skill, SKILL_INCREASE_MEDIUM);

//  dam = dice(skill,4);
  dam = 225;
//  int save = saves_spell(ch, victim, 25, SAVE_TYPE_MAGIC);
  //dam += (int) (dam * (save/100));

  return spell_damage(ch, victim, dam, TYPE_MAGIC, SPELL_DISPEL_GOOD, 0);
}

int spell_call_lightning(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  extern struct weather_data weather_info;

  set_cantquit( ch, victim );

  if (OUTSIDE(ch) && (weather_info.sky>=SKY_RAINING)) 
  {
     skill_increase_check(ch, SPELL_CALL_LIGHTNING, skill, SKILL_INCREASE_HARD);
     dam = dice(MIN((int)GET_MANA(ch),700), 1);
//     if(saves_spell(ch, victim, skill/6, SAVE_TYPE_ENERGY) >= 0)
  //      dam >>= 1;
     return spell_damage(ch, victim, dam,TYPE_ENERGY, SPELL_CALL_LIGHTNING, 0);
  }
  return eFAILURE;
}

int spell_harm(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;

  set_cantquit( ch, victim );

  dam = 150;
  skill_increase_check(ch, SPELL_HARM, skill, SKILL_INCREASE_MEDIUM);
//  int save = saves_spell(ch, victim, skill / 2, SAVE_TYPE_MAGIC);
  //dam += (int) (dam * (save/100));

  return spell_damage(ch, victim, dam, TYPE_MAGIC, SPELL_HARM, 0);
}

int spell_power_harm(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;

  set_cantquit( ch, victim );

  skill_increase_check(ch, SPELL_POWER_HARM, skill, SKILL_INCREASE_MEDIUM);
//  dam = 100 + 4 * ( skill / 2 );
   dam = 300;
//  int save = saves_spell(ch, victim, skill / 2, SAVE_TYPE_MAGIC);
  //dam += (int) (dam * (save/100));

  return spell_damage(ch, victim, dam, TYPE_MAGIC, SPELL_POWER_HARM, 0);
}

// TODO - make this spell use skill level and advance on it's own
int spell_teleport(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int to_room;
  // int i;
  char buf[100]; 
  extern int top_of_world;      /* ref to the top element of world */

  if(!victim) {
    log("Null victim sent to teleport!", ANGEL, LOG_BUG);
    return eFAILURE;
  }
  if(IS_AFFECTED(victim, AFF_SOLIDITY)) {
      send_to_char("You find yourself unable to.\n\r", ch);
      if(ch != victim) {
          sprintf(buf, "%s just tried to teleport you.\n\r", GET_SHORT(ch));
          send_to_char(buf, victim);
      }
      return eFAILURE;
  }
  if(IS_SET(world[ch->in_room].room_flags, ARENA)) {
     to_room = real_room(number(ARENA_LOW, ARENA_HI));
     if(number(1, 4) == 1 && ch == victim )
        to_room = real_room(ARENA_DEATHTRAP);
  } else {
     do {
         to_room = number(0, top_of_world);
     } while (!world_array[to_room] ||
              IS_SET(world[to_room].room_flags, PRIVATE) ||
              IS_SET(world[to_room].room_flags, IMP_ONLY) ||
              IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
              IS_SET(world[to_room].room_flags, ARENA) ||
              world[to_room].sector_type == SECT_UNDERWATER ||
              IS_SET(zone_table[world[to_room].zone].zone_flags, ZONE_NO_TELEPORT) ||
              ( (IS_NPC(victim) && IS_SET(victim->mobdata->actflags, ACT_STAY_NO_TOWN)) ? 
                    (IS_SET(zone_table[world[to_room].zone].zone_flags, ZONE_IS_TOWN)) :
                    FALSE
               )
             );
  }

  if((IS_MOB(victim)) && (!IS_MOB(ch)))
    add_memory(victim, GET_NAME(ch), 'h');

  act("$n slowly fades out of existence.", victim, 0, 0, TO_ROOM, 0);
  move_char(victim, to_room);
  act("$n slowly fades into existence.", victim, 0, 0, TO_ROOM, 0);

  do_look(victim, "", 0);
  return eSUCCESS;
}

int spell_bless(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(!ch && (!victim || !obj))
  {
    log("Null ch or victim and obj in bless.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if (obj) {
    if ( (5*level > GET_OBJ_WEIGHT(obj)) &&
	 (GET_POS(ch) != POSITION_FIGHTING)) {
		SET_BIT(obj->obj_flags.extra_flags, ITEM_BLESS);
		act("$p briefly glows.",ch,obj,0,TO_CHAR, 0);
    }
  } else {

  skill_increase_check(ch, SPELL_BLESS, skill, SKILL_INCREASE_MEDIUM);
  
  if(affected_by_spell(victim, SPELL_BLESS))
    affect_from_char(victim, SPELL_BLESS);

  if(GET_POS(victim) != POSITION_FIGHTING) 
  {
		send_to_char("You feel righteous.\n\r", victim);
		if (victim != ch)
		  act("$N receives the blessing from your god.", ch, NULL, victim, TO_CHAR, 0);
		af.type      = SPELL_BLESS;
		af.duration  = 6+ skill;
		af.modifier  = 1 + skill / 45;
		af.location  = APPLY_HITROLL;
		af.bitvector = 0;
		affect_to_char(victim, &af);

		af.location = APPLY_SAVING_MAGIC;
		af.modifier = 2;                 /* Make better */
		affect_to_char(victim, &af);
    }
  }
  return eSUCCESS;
}

// TODO - make this use skill and increase
int spell_paralyze(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  char buf[180];
  int retval;

  if (affected_by_spell(victim, SPELL_PARALYZE))
         return eFAILURE;

  if(affected_by_spell(victim, SPELL_SLEEP)) {
     if (number(1,6)<5)
     {
       int retval;
       if (number(0,1))
          send_to_char("The combined magics fizzle!\r\n",ch);
	if (GET_POS(victim) == POSITION_SLEEPING) {
	  send_to_char("You are awoken by a burst of energy!\r\n",victim);
	  act("$n is awoken in a burst of energy!",victim,NULL,NULL, TO_ROOM,0);
	  GET_POS(victim) = POSITION_SITTING;
	}
       else {
          send_to_char("The combined magics cause an explosion!\r\n",ch);
	  retval = damage(ch,ch,number(5,10), 0, TYPE_MAGIC, 0);
      }
       return retval|eFAILURE;
     }
  }

  /* save the newbies! */
  if(!IS_NPC(ch) && !IS_NPC(victim) && (GET_LEVEL(victim) < 10)) {
    send_to_char("Your cold-blooded act causes your magic to misfire!\n\r", ch);
    victim = ch;
  }

  if(IS_NPC(victim) && (GET_LEVEL(victim) == 0)) {
    log("Null victim level in spell_paralyze.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  set_cantquit( ch, victim );
  int save = 0;
  if (affected_by_spell(victim,SPELL_SLEEP))
    save = -15; // Above check takes care of sleep.
  int spellret = saves_spell(ch, victim, save, SAVE_TYPE_MAGIC);

///  logf(IMP, LOG_BUG, "%s para attempt on %s.  Result: %d",
///       GET_NAME(ch), GET_NAME(victim), spellret);

  /* ideally, we would do a dice roll to see if spell hits or not */
  if(spellret >= 0 && (victim != ch)) {
      act("$N seems to be unaffected!", ch, NULL, victim, TO_CHAR, 0);
      if(!IS_NPC(victim)) {
         act("$n tried to paralyze you!", ch, NULL, victim, TO_VICT, 0);
      }
      if ((!victim->fighting) && GET_POS(ch) > POSITION_SLEEPING) {
         retval = attack(victim, ch, TYPE_UNDEFINED);
         retval = SWAP_CH_VICT(retval);
         return retval;
      }
      return eFAILURE;
  }

  /* if they are too big - do a dice roll to see if they backfire */
  if(!IS_NPC(ch) && !IS_NPC(victim) && ((level - GET_LEVEL(victim)) > 10)) {
      act("$N seems to be unaffected!", ch, NULL, victim, TO_CHAR, 0);
      victim = ch;
      if(saves_spell(ch, ch, -100, SAVE_TYPE_MAGIC) >= 0) {
        act("Your magic misfires but you are saved!", ch, NULL, victim, TO_CHAR,0);
        return eFAILURE;
      }
      act("Your cruel heart causes your magic to misfire!", ch, NULL, victim, TO_CHAR, 0);
  }

  // Finish off any performances
  if(IS_SINGING(victim))
    do_sing(victim, "stop", 9);

  act("$n seems to be paralyzed!", victim, 0, 0, TO_ROOM, INVIS_NULL);
  send_to_char("Your entire body rebels against you and you are paralyzed!\n\r", victim);

  if(!IS_NPC(victim))
  {
    sprintf(buf, "%s was just paralyzed.", GET_NAME(victim));
    log(buf, OVERSEER, LOG_MORTAL);
  }

  af.type      = SPELL_PARALYZE;
  af.location  = APPLY_NONE;
  af.modifier  = 0;
  af.duration  = 2;
  af.bitvector = AFF_PARALYSIS;
  affect_to_char(victim, &af);
  return eSUCCESS;
}

// TODO - make this use skill and increase
int spell_blindness(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int retval;

  set_cantquit( ch, victim );

  if(saves_spell(ch, victim, -10, SAVE_TYPE_MAGIC)) {
    act("$N seems to be unaffected!", ch, NULL, victim, TO_CHAR, 0);
    act("$n tried to blind you!", ch, NULL, victim, TO_VICT, 0);
    if ((!victim->fighting) && (IS_NPC(victim)) && number(0, 1))
      retval = attack(victim, ch, TYPE_UNDEFINED, FIRST);
      retval = SWAP_CH_VICT(retval);
      return retval;
  }

  if (affected_by_spell(victim, SPELL_BLINDNESS) || IS_AFFECTED(victim, AFF_BLIND))
    return eFAILURE;

  act("$n seems to be blinded!", victim, 0, 0, TO_ROOM, INVIS_NULL);
  send_to_char("You have been blinded!\n\r", victim);

  af.type      = SPELL_BLINDNESS;
  af.location  = APPLY_HITROLL;
  af.modifier  = -4;  /* Make hitroll worse */
  af.duration  = 3+(level/5);
  af.bitvector = AFF_BLIND;
  affect_to_char(victim, &af);


  af.location = APPLY_AC;
  af.modifier = +40; /* Make AC Worse! */
  affect_to_char(victim, &af);
  return eSUCCESS;
}


int spell_create_food(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct obj_data *tmp_obj;

  skill_increase_check(ch, SPELL_CREATE_FOOD, skill, SKILL_INCREASE_MEDIUM);

  tmp_obj = clone_object(real_object(7));
  tmp_obj->obj_flags.value[0] += skill/2;

  obj_to_room(tmp_obj, ch->in_room);

  act("$p suddenly appears.",ch,tmp_obj,0,TO_ROOM, INVIS_NULL);
  act("$p suddenly appears.",ch,tmp_obj,0,TO_CHAR, INVIS_NULL);
  return eSUCCESS;
}

int spell_create_water(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int water;

  extern struct weather_data weather_info;

  if(!ch || !obj)
  {
    log("Null ch or obj in create_water.", ANGEL, LOG_BUG);
    return eFAILURE|eINTERNAL_ERROR;
  }

  if (GET_ITEM_TYPE(obj) == ITEM_DRINKCON) 
  {
         skill_increase_check(ch, SPELL_CREATE_WATER, skill, SKILL_INCREASE_MEDIUM);

	 if ((obj->obj_flags.value[2] != LIQ_WATER)
	 && (obj->obj_flags.value[1] != 0)) {

		obj->obj_flags.value[2] = LIQ_SLIME;

	 } else {

		water = 20 + skill * 2;

		/* Calculate water it can contain, or water created */
		water = MIN(obj->obj_flags.value[0]-obj->obj_flags.value[1], water);

		if (water > 0) {
		 obj->obj_flags.value[2] = LIQ_WATER;
		 obj->obj_flags.value[1] += water;

		 act("$p is filled.", ch,obj,0,TO_CHAR, 0);
		}
	 }
  }
  return eSUCCESS;
}



int spell_remove_paralysis(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{

  if(!victim)
  {
    log("Null victim in remove_paralysis!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_REMOVE_PARALYSIS, skill, SKILL_INCREASE_MEDIUM);

  if (affected_by_spell(victim, SPELL_PARALYZE) &&
      number(1, 100) < (80 + skill/6)) 
  {
	 affect_from_char(victim, SPELL_PARALYZE);
         send_to_char("Your spell is successful!\r\n", ch);
	 send_to_char("Your movement returns!\n\r", victim);
  }
  else send_to_char("Your spell fails to return movement to your victim.\r\n", ch);

  return eSUCCESS;
}

int spell_cure_blind(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  if (!victim) {
    log("Null victim in cure_blind!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(number(1, 100) < ( 80 + skill/6 ) )
  {
    if (affected_by_spell(victim, SPELL_BLINDNESS)) 
    {
      skill_increase_check(ch, SPELL_CURE_BLIND, skill, SKILL_INCREASE_MEDIUM);
      affect_from_char(victim, SPELL_BLINDNESS);
      send_to_char("Your vision returns!\n\r", victim);
    }
    if (IS_AFFECTED(victim, AFF_BLIND)) {
      REMOVE_BIT(victim->affected_by, AFF_BLIND);
      send_to_char("Your vision returns!\n\r", victim);
    }
  }
  return eSUCCESS;
}

int spell_cure_critic(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int healpoints;

  if(!victim)
  {
    log("Null victim in cure_critic.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Heal spells seem to be useless on the undead.\n\r", ch);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_CURE_CRITIC, skill, SKILL_INCREASE_MEDIUM);

//  healpoints = dice(3,8)-6+skill;
  healpoints = dam_percent(skill, 100);
  if ( (healpoints + GET_HIT(victim)) > hit_limit(victim) )
	 GET_HIT(victim) = hit_limit(victim);
  else
	 GET_HIT(victim) += healpoints;

  send_to_char("You feel better!\n\r", victim);

  update_pos(victim);
  return eSUCCESS;
}

int spell_cure_light(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int healpoints;

  if(!ch || !victim)
  {
    log("Null ch or victim in cure_light!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Heal spells seem to be useless on the undead.\n\r", ch);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_CURE_LIGHT, skill, SKILL_INCREASE_MEDIUM);
//  healpoints = dice(1,8)+(skill/3);
  healpoints = dam_percent(skill, 25);
  if ( (healpoints+GET_HIT(victim)) > hit_limit(victim) )
	 GET_HIT(victim) = hit_limit(victim);
  else
	 GET_HIT(victim) += healpoints;

  update_pos( victim );

  send_to_char("You feel better!\n\r", victim);
  if (ch!=victim)
  {
     send_to_char("They look healthier.\r\n",ch);
  }
  return eSUCCESS;
}

// TODO - make this use skill and increase
int spell_curse(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int retval;

  if (obj) {
	 SET_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);

	 /* LOWER ATTACK DICE BY -1 */
	  if(obj->obj_flags.type_flag == ITEM_WEAPON)  {
		 if (obj->obj_flags.value[2] > 1) {
	  obj->obj_flags.value[2]--;
	  act("$p glows red.", ch, obj, 0, TO_CHAR, 0);
		 }
		 else {
	  send_to_char("Nothing happens.\n\r", ch);
		 }
	  }
  } else {
        if(!ch || !victim)
          return eFAILURE;

        set_cantquit(ch, victim);

	 if(saves_spell(ch, victim, 15, SAVE_TYPE_MAGIC) >= 0)
		{
	act("$N seems to be unaffected!", ch, NULL, victim, TO_CHAR, 0);
	retval = one_hit( victim, ch, TYPE_UNDEFINED, FIRST);
	retval = SWAP_CH_VICT(retval);
	return retval;
		}

	 if (affected_by_spell(victim, SPELL_CURSE))
		return eFAILURE;

	 af.type      = SPELL_CURSE;
	 af.duration  = 5*level;
	 af.modifier  = -1;
	 af.location  = APPLY_HITROLL;
	 af.bitvector = AFF_CURSE;
	 affect_to_char(victim, &af);

	 af.location = APPLY_SAVING_MAGIC;
	 af.modifier = -1; /* Make worse */
	 affect_to_char(victim, &af);

	 act("$n briefly reveals a red aura!", victim, 0, 0, TO_ROOM, 0);
	 act("You feel very uncomfortable.",victim,0,0,TO_CHAR, 0);
	 retval = one_hit( victim, ch, TYPE_UNDEFINED, FIRST);
	retval = SWAP_CH_VICT(retval);
        return retval;
  }
  return eSUCCESS;
}

int spell_detect_evil(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(!victim)
  {
    log("Null victim in detect evil!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_DETECT_EVIL, skill, SKILL_INCREASE_EASY);

  if ( affected_by_spell(victim, SPELL_DETECT_EVIL) )
    affect_from_char(victim, SPELL_DETECT_EVIL);

  af.type      = SPELL_DETECT_EVIL;
  af.duration  = skill * 2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_DETECT_EVIL;

  affect_to_char(victim, &af);

  send_to_char("You become more conscious of the evil around you.\n\r", victim);
  return eSUCCESS;
}



int spell_detect_good(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(!victim)
  {
    log("Null victim sent to detect_good.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_DETECT_GOOD, skill, SKILL_INCREASE_EASY);

  if ( affected_by_spell(victim, SPELL_DETECT_GOOD) )
    affect_from_char(victim, SPELL_DETECT_GOOD);

  af.type      = SPELL_DETECT_GOOD;
  af.duration  = skill * 2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_DETECT_GOOD;

  affect_to_char(victim, &af);

  send_to_char("You are now able to truly recognize the good in others.\n\r", victim);
  return eSUCCESS;
}


int spell_true_sight(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(!victim)
  {
    log("Null victim sent to detect_good.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if ( affected_by_spell(victim, SPELL_TRUE_SIGHT) )
	 affect_from_char(victim, SPELL_TRUE_SIGHT);

  if ( IS_AFFECTED(victim,AFF_TRUE_SIGHT) )
	 return eFAILURE;

  skill_increase_check(ch, SPELL_TRUE_SIGHT, skill, SKILL_INCREASE_EASY);

  af.type      = SPELL_TRUE_SIGHT;
  af.duration  = skill * 2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_TRUE_SIGHT;

  affect_to_char(victim, &af);

  send_to_char("You feel your vision enhanced with an incredibly keen perception.\n\r", victim);
  return eSUCCESS;
}

int spell_detect_invisibility(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(!victim)
  {
    log("Null victim sent to detect_good.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if ( affected_by_spell(victim, SPELL_DETECT_INVISIBLE) )
	 affect_from_char(victim, SPELL_DETECT_INVISIBLE);

  if ( IS_AFFECTED(victim,AFF_DETECT_INVISIBLE) )
	 return eFAILURE;

  skill_increase_check(ch, SPELL_DETECT_INVISIBLE, skill, SKILL_INCREASE_EASY);

  af.type      = SPELL_DETECT_INVISIBLE;
  af.duration  = skill*2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_DETECT_INVISIBLE;

  affect_to_char(victim, &af);

  send_to_char("Your eyes tingle.\n\r", victim);
  return eSUCCESS;
}


int spell_infravision(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(!victim)
  {
    log("Null victim sent to detect_good.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if ( affected_by_spell(victim, SPELL_INFRAVISION) )
	 affect_from_char(victim, SPELL_INFRAVISION);

  if ( IS_AFFECTED(victim,AFF_INFRARED) )
	 return eFAILURE;

  skill_increase_check(ch, SPELL_INFRAVISION, skill, SKILL_INCREASE_EASY);

  af.type      = SPELL_INFRAVISION;
  af.duration  = skill * 2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_INFRARED;

  affect_to_char(victim, &af);

  send_to_char("Your eyes glow red.\n\r", victim);
  return eSUCCESS;
}

int spell_detect_magic(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(!victim)
  {
    log("Null victim sent to detect_good.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_DETECT_MAGIC, skill, SKILL_INCREASE_EASY);

  if ( affected_by_spell(victim, SPELL_DETECT_MAGIC) )
	affect_from_char(victim, SPELL_DETECT_MAGIC);

  af.type      = SPELL_DETECT_MAGIC;
  af.duration  = skill * 2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_DETECT_MAGIC;

  affect_to_char(victim, &af);
  send_to_char("Your vision temporarily blurs, your focus shifting to the metaphysical realm.\n\r", victim);
  return eSUCCESS;
}


int spell_haste(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{

  struct affected_type af;

  if(!victim)
  {
    log("Null victim sent to haste", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if ( affected_by_spell(victim, SPELL_HASTE) || IS_AFFECTED(victim, AFF_HASTE))
	 return eFAILURE;

  skill_increase_check(ch, SPELL_HASTE, skill, SKILL_INCREASE_MEDIUM);

  af.type      = SPELL_HASTE;
  af.duration  = skill/10;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_HASTE;

  affect_to_char(victim, &af);
  send_to_char("You feel fast!\n\r", victim);
  act("$n begins to move faster.\r\n", victim, 0, 0, TO_ROOM, 0);
  return eSUCCESS;
}

// TODO - make this use skill and increase
int spell_detect_poison(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  if(!ch && (!victim || !obj))
  {
    log("Null ch or victim and obj in bless.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if (victim) {
	 if (victim == ch)
		if (IS_AFFECTED(victim, AFF_POISON))
	send_to_char("You can sense poison in your blood.\n\r", ch);
      else
	send_to_char("You feel healthy.\n\r", ch);
	 else
		if (IS_AFFECTED(victim, AFF_POISON)) {
	act("You sense that $E is poisoned.",ch,0,victim,TO_CHAR, 0);
      } else {
	act("You sense that $E is healthy.",ch,0,victim,TO_CHAR, 0);
		}
  } else { /* It's an object */
	 if ((obj->obj_flags.type_flag == ITEM_DRINKCON) ||
	(obj->obj_flags.type_flag == ITEM_FOOD)) {
      if (obj->obj_flags.value[3])
	act("Poisonous fumes are revealed.", ch, 0, 0, TO_CHAR, 0);
      else
	send_to_char("It looks very delicious.\n\r", ch);
    }
  }
  return eSUCCESS;
}

// TODO - make this use skill and increase
int spell_enchant_armor(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  /*int i;*/

  send_to_char("This spell being revamped.  Sorry.\n\r", ch);
  return eFAILURE;

  if ((GET_ITEM_TYPE(obj) == ITEM_ARMOR) &&
		!IS_SET(obj->obj_flags.extra_flags, ITEM_ENCHANTED)) {


    SET_BIT(obj->obj_flags.extra_flags, ITEM_ENCHANTED);
      
	 obj->obj_flags.value[2] += 1 +(level >=MAX_MORTAL);


    if (IS_GOOD(ch)) {
		SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_EVIL);
		act("$p glows blue.",ch,obj,0,TO_CHAR, 0);
	 } else if (IS_EVIL(ch)) {
		SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_GOOD);
		act("$p glows red.",ch,obj,0,TO_CHAR, 0);
    } else {
      act("$p glows yellow.",ch,obj,0,TO_CHAR, 0);
	}
	 }
  return eSUCCESS;
}

// TODO - make this use skill and increase
int spell_enchant_weapon(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  if(!ch || !obj)
  {
    log("Null ch or obj in enchant_weapon!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  send_to_char("This spell being revamped, sorry.\n\r", ch);
  return eFAILURE;

  if ((GET_ITEM_TYPE(obj) == ITEM_WEAPON) &&
      !IS_SET(obj->obj_flags.extra_flags, ITEM_MAGIC)) {

	if (obj->affected)
		return eFAILURE;

	 SET_BIT(obj->obj_flags.extra_flags, ITEM_MAGIC);
         obj->num_affects = 2;
#ifdef LEAK_CHECK
         obj->affected = (obj_affected_type *) calloc(obj->num_affects, sizeof(obj_affected_type));
#else
         obj->affected = (obj_affected_type *) dc_alloc(obj->num_affects, sizeof(obj_affected_type));
#endif

	 obj->affected[0].location = APPLY_HITROLL;
	 obj->affected[0].modifier = 4 +(level >=18)+ (level>=38)+
		(level >=48)+ (level >=DEITY);

    obj->affected[1].location = APPLY_DAMROLL;
	 obj->affected[1].modifier = 4+(level >= 20)+ (level >=40)+ (level >=MAX_MORTAL)+
		(level >=DEITY);

    if (IS_GOOD(ch)) {
		SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_EVIL);
      act("$p glows blue.",ch,obj,0,TO_CHAR,0);
	 } else if (IS_EVIL(ch)) {
      SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_GOOD);
      act("$p glows red.",ch,obj,0,TO_CHAR, 0);
    } else {
      act("$p glows yellow.",ch,obj,0,TO_CHAR, 0);
	}
	 }
  if(GET_ITEM_TYPE(obj) == ITEM_MISSILE)
  {
    if(!obj->obj_flags.value[2] && !obj->obj_flags.value[3])
    {
      obj->obj_flags.value[2] = (level>20) + (level>30) + (level>40)
                                + (level>45) + (level>49);
      obj->obj_flags.value[3] = obj->obj_flags.value[2];
    }
  }
  return eSUCCESS;
}


int spell_mana(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
	int mana;

  if(!victim)
  {
    log("Null victim sent to mana!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  mana = GET_LEVEL(victim) * 4;

  GET_MANA(victim) += mana;

  if (GET_MANA(victim) > GET_MAX_MANA(victim)) 
    GET_MANA(victim) = GET_MAX_MANA(victim);


  update_pos( victim );

  send_to_char("You feel magical energy fill your mind!\n\r", victim);
  return eSUCCESS;
}

int spell_heal(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  if(!victim) {
    log("Null victim sent to heal!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Heal spells seem to be useless on the undead.\n\r", ch);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_HEAL, skill, SKILL_INCREASE_MEDIUM);

  spell_cure_blind(level, ch, victim, obj, skill);
  int healy = dam_percent(skill,250);
  GET_HIT(victim) += healy;

  if (GET_HIT(victim) >= hit_limit(victim))
	 GET_HIT(victim) = hit_limit(victim)-dice(1,4);

  update_pos( victim );

  send_to_char("A warm feeling fills your body.\n\r", victim);
  return eSUCCESS;
}


int spell_power_heal(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  if(!victim) {
    log("Null victim sent to power heal!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Heal spells seem to be useless on the undead.\n\r", ch);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_POWER_HEAL, skill, SKILL_INCREASE_MEDIUM);

  spell_cure_blind(level, ch, victim, obj, skill);
  int healy = dam_percent(skill, 300);
  GET_HIT(victim) += healy;

  if (GET_HIT(victim) >= hit_limit(victim))
	 GET_HIT(victim) = hit_limit(victim)-dice(1,4);

  update_pos( victim );

  send_to_char("A warm feeling fills your body.\n\r", victim);
  return eSUCCESS;
}


int spell_full_heal(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  assert(victim);
  int healamount = 0;

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Heal spells seem to be useless on the undead.\n\r", ch);
    return eFAILURE;
  }

  spell_cure_blind(level, ch, victim, obj, skill);

  skill_increase_check(ch, SPELL_FULL_HEAL, skill, SKILL_INCREASE_MEDIUM);

  healamount = 10 * (skill/2 + 5);
  if(GET_ALIGNMENT(ch) < -349)
    healamount -= 2* (skill/2 + 5);
  else if(GET_ALIGNMENT(ch) > 349)
    healamount += (skill / 2 + 5);

  GET_HIT(victim) += healamount;

  if (GET_HIT(victim) >= hit_limit(victim))
	 GET_HIT(victim) = hit_limit(victim)-dice(1,4);

  update_pos( victim );

  send_to_char("A warm feeling fills your body.\n\r", victim);
  return eSUCCESS;
}

int spell_invisibility(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  assert((ch && obj) || victim);

  if (obj) {
	 if (CAN_WEAR(obj,ITEM_TAKE)) {
		if ( !IS_SET(obj->obj_flags.extra_flags, ITEM_INVISIBLE)){
	 act("$p turns invisible.",ch,obj,0,TO_CHAR, 0);
	 act("$p turns invisible.",ch,obj,0,TO_ROOM, INVIS_NULL);
	 SET_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
		}
	 } else {
		act("You fail to make $p invisible.",ch,obj,0,TO_CHAR, 0);
	 }
  }
  else {              /* Then it is a PC | NPC */

        skill_increase_check(ch, SPELL_INVISIBLE, skill, SKILL_INCREASE_EASY);

	 if (affected_by_spell(victim, SPELL_INVISIBLE))
		affect_from_char(victim, SPELL_INVISIBLE);

	if(IS_AFFECTED(victim, AFF_INVISIBLE))
		return eFAILURE;

	act("$n slowly fades out of existence.", victim,0,0,TO_ROOM, INVIS_NULL);
	send_to_char("You vanish.\n\r", victim);

	af.type      = SPELL_INVISIBLE;
	af.duration  = (int) ( skill / 3.75 );
	af.modifier  = -40;
	af.location  = APPLY_AC;
	af.bitvector = AFF_INVISIBLE;
	affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

int spell_locate_object(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct obj_data *i;
  char name[256];
  char buf[MAX_STRING_LENGTH];
  int j;
  int total;

  assert(ch);

  strcpy(name, fname(obj->name));

  total = j = (int) (skill / 3.75);

  for (i = object_list; i && (j>0); i = i->next)
  {
    if (IS_OBJ_STAT(i, ITEM_NOSEE))
       continue;

    if(IS_SET(i->obj_flags.more_flags, ITEM_NOLOCATE))
       continue;

    if (isname(name, i->name)) {
      if(i->carried_by) {
        sprintf(buf,"%s carried by %s.\n\r", i->short_description,PERS(i->carried_by,ch));
        send_to_char(buf,ch);
      } else if (i->in_obj) {
        sprintf(buf,"%s in %s.\n\r",i->short_description, i->in_obj->short_description);
        send_to_char(buf,ch);
      } else {
        sprintf(buf,"%s in %s.\n\r",i->short_description,
           (i->in_room == NOWHERE ? "Used but uncertain." : world[i->in_room].name));
        send_to_char(buf,ch);
        j--;
      }
    }
  }

  if(j==total)
	 send_to_char("No such object.\n\r",ch);

  if(j==0)
	 send_to_char("You are very confused.\n\r",ch);

  skill_increase_check(ch, SPELL_LOCATE_OBJECT, skill, SKILL_INCREASE_MEDIUM);

  return eSUCCESS;
}

// TODO - neesd to use spell increases and skill
int spell_poison(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int retval = eSUCCESS;

  if (victim) 
  {
    set_cantquit(ch, victim);
    if(!IS_SET(victim->immune, ISR_POISON))
      if(saves_spell(ch, victim, 0, SAVE_TYPE_POISON) < 0)
      {
        af.type = SPELL_POISON;
        af.duration = level*2;
        af.modifier = -5;
        af.location = APPLY_STR;
        af.bitvector = AFF_POISON;
        affect_join(victim, &af, FALSE, FALSE);
        send_to_char("You feel very sick.\n\r", victim);
      }
      else
        act("$N seems to be unaffected!", ch, NULL, victim, TO_CHAR, 0);

    if (!( ch == victim ))
      retval = one_hit(victim,ch,TYPE_UNDEFINED, FIRST);
  } else { /* Object poison */
    if ((obj->obj_flags.type_flag == ITEM_DRINKCON) ||
        (obj->obj_flags.type_flag == ITEM_FOOD)) 
    {
      obj->obj_flags.value[3] = 1;
    }
  }
  return retval;
}

int spell_protection_from_evil(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

	 assert(victim);

  /* keep spells from stacking/being cast on targets with built-in affect */
  if (IS_AFFECTED(victim,AFF_PROTECT_EVIL) || affected_by_spell(victim, SPELL_PROTECT_FROM_GOOD) || GET_CLASS(victim) == CLASS_ANTI_PAL)
	 return eFAILURE;

  if (!affected_by_spell(victim, SPELL_PROTECT_FROM_EVIL) ) 
  {
	 skill_increase_check(ch, SPELL_PROTECT_FROM_EVIL, skill, SKILL_INCREASE_MEDIUM);

	 af.type      = SPELL_PROTECT_FROM_EVIL;
	 af.duration  = (int) (skill / 3.75);
	 af.modifier  = 0;
	 af.location  = APPLY_NONE;
	 af.bitvector = AFF_PROTECT_EVIL;
	 affect_to_char(victim, &af);
	 send_to_char("You have a righteous, protected feeling!\n\r", victim);
  }
  return eSUCCESS;
}

int spell_protection_from_good(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  assert(victim);

  if (IS_AFFECTED(victim,AFF_PROTECT_GOOD) || IS_AFFECTED(victim, AFF_PROTECT_EVIL) || affected_by_spell(victim, SPELL_PROTECT_FROM_EVIL) || GET_CLASS(victim) == CLASS_PALADIN)
	 return eFAILURE;

  if (!affected_by_spell(victim, SPELL_PROTECT_FROM_GOOD)) 
  {
	 skill_increase_check(ch, SPELL_PROTECT_FROM_GOOD, skill, SKILL_INCREASE_MEDIUM);

	 af.type      = SPELL_PROTECT_FROM_GOOD;
	 af.duration  = (int) (skill / 3.75);
	 af.modifier  = 0;
	 af.location  = APPLY_NONE;
	 af.bitvector = AFF_PROTECT_GOOD;
	 affect_to_char(victim, &af);
	 send_to_char("You feel yourself wrapped in a protective mantle of evil.\n\r", victim);
  }
  return eSUCCESS;
}

// TODO - make this do something with higher skill
int spell_remove_curse(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int j;
  
  assert(ch && (victim || obj));

  skill_increase_check(ch, SPELL_REMOVE_CURSE, skill, SKILL_INCREASE_MEDIUM);

  if(obj) {
	 if(IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
		act("$p briefly glows blue.", ch, obj, 0, TO_CHAR, 0);
		REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
	 }
	 return eSUCCESS;
  }

  /* Then it is a PC | NPC */
  if(affected_by_spell(victim, SPELL_CURSE)) {
	 act("$n briefly glows red, then blue.",victim,0,0,TO_ROOM, 0);
	 act("You feel better.",victim,0,0,TO_CHAR, 0);
	 affect_from_char(victim, SPELL_CURSE);
	 return eSUCCESS;
  }

  for(j=0; j<MAX_WEAR; j++) {
    if((obj = victim->equipment[j]) && IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
		 REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
		 return eSUCCESS;
    }
  }

  for(obj = victim->carrying; obj; obj = obj->next_content)
	  if(IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
		 act("$n's $p briefly glows blue.", victim, obj, 0, TO_ROOM, 0);
		 REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
		 break;
	  }
  return eSUCCESS;
}

// TODO - make this use skill
int spell_remove_poison(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{

  assert(ch && (victim || obj));

  if (victim) {
	 if(affected_by_spell(victim,SPELL_POISON)) {
		affect_from_char(victim,SPELL_POISON);
		act("A warm feeling runs through your body.",victim,
		  0,0,TO_CHAR, 0);
		act("$N looks better.",ch,0,victim,TO_ROOM, INVIS_NULL);
	 }
  } else {
	 if ((obj->obj_flags.type_flag == ITEM_DRINKCON) ||
	(obj->obj_flags.type_flag == ITEM_FOOD)) {
		obj->obj_flags.value[3] = 0;
		act("The $p steams briefly.",ch,obj,0,TO_CHAR, 0);
	 }
  }
  return eSUCCESS;
}


int spell_fireshield(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if ( IS_AFFECTED(victim,AFF_FIRESHIELD) )
  {
	 act("$N is already fireshielded.",ch,0,victim,TO_CHAR, INVIS_NULL);
	 return eFAILURE;
  }

  if (!affected_by_spell(victim, SPELL_FIRESHIELD))
  {
    skill_increase_check(ch, SPELL_FIRESHIELD, skill, SKILL_INCREASE_MEDIUM);

    act("$n is surrounded by flames.",victim,0,0,TO_ROOM, INVIS_NULL);
    act("You are surrounded by flames.",victim,0,0,TO_CHAR, 0);

    af.type      = SPELL_FIRESHIELD;
    af.duration  = 1 + skill / 30;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_FIRESHIELD;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

int cast_camouflague(byte level, CHAR_DATA *ch, char *arg, int type,
                     CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch(type) {
    case SPELL_TYPE_SPELL:
      return spell_camouflague(level, ch, tar_ch, 0, skill);
      break; 
    case SPELL_TYPE_POTION:
      return spell_camouflague(level, ch, ch, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if(tar_obj) {
        return eFAILURE;
      }
      if(!tar_ch) {
        tar_ch = ch;
      }
      return spell_camouflague(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_STAFF:
      for(tar_ch = world[ch->in_room].people; tar_ch;
                tar_ch = tar_ch->next_in_room)
        spell_camouflague(level, ch, tar_ch, 0, skill);
      return eSUCCESS;
      break;
    default:
      log("Serious screw-up in cast_camouflague!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}
int cast_farsight(byte level, CHAR_DATA *ch, char *arg, int type,
                   CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch(type) {
     case SPELL_TYPE_SPELL:
       return spell_farsight(level, ch, tar_ch, 0, skill);
       break;
     case SPELL_TYPE_POTION:
       return spell_farsight(level, ch, ch, 0, skill);
       break;
     case SPELL_TYPE_SCROLL:
       if(tar_obj) {
         return eFAILURE;
       }
       if(!tar_ch) {
         tar_ch = ch;
       }
       return spell_farsight(level, ch, tar_ch, 0, skill);
       break;
     case SPELL_TYPE_STAFF:
       for(tar_ch = world[ch->in_room].people; tar_ch; 
           tar_ch = tar_ch->next_in_room)
          spell_farsight(level, ch, tar_ch, 0, skill);
       return eSUCCESS;
       break;
     default:
       log("Serious screw-up in cast_farsight!", LOG_BUG, ANGEL);
       break;
  }
  return eSUCCESS;
}

int cast_freefloat(byte level, CHAR_DATA *ch, char *arg, int type,
                    CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch(type) {
    case SPELL_TYPE_SPELL:
      return spell_freefloat(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_freefloat(level, ch, ch, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if(tar_obj) {
        return eFAILURE;
      }
      if(!tar_ch) {
        tar_ch = ch;
      }
      return spell_freefloat(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_STAFF:
      for(tar_ch = world[ch->in_room].people; tar_ch; 
          tar_ch = tar_ch->next_in_room);
        spell_freefloat(level, ch, tar_ch, 0, skill);
      return eSUCCESS;
      break;
    default: 
      log("Serious screw-up in cast_freefloat!", ANGEL, LOG_MISC);
      break;
  }
  return eFAILURE;
}

int cast_insomnia(byte level, CHAR_DATA *ch, char *arg, int type,
                   CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch(type) {
    case SPELL_TYPE_SPELL:
      return spell_insomnia(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_insomnia(level, ch, ch, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if(tar_obj) {
        return eFAILURE;
      }
      if(!tar_ch) {
        tar_ch = ch;
      }
      return spell_insomnia(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_STAFF:
      for(tar_ch = world[ch->in_room].people; tar_ch; 
          tar_ch = tar_ch->next_in_room)
        spell_insomnia(level, ch, tar_ch, 0, skill);
      return eSUCCESS;
      break;
    default:
      log("Serious screw-up in cast_insomnia!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}

int cast_shadowslip(byte level, CHAR_DATA *ch, char *arg, int type,
                     CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch(type) {
    case SPELL_TYPE_SPELL:
      return spell_shadowslip(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_shadowslip(level, ch, ch, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if(tar_obj) {
        return eFAILURE;
      }
      if(!tar_ch) {
        tar_ch = ch;
      }
      return spell_shadowslip(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_STAFF:
      for(tar_ch = world[ch->in_room].people; tar_ch; 
           tar_ch = tar_ch->next_in_room) 
        spell_shadowslip(level, ch, tar_ch, 0, skill);
      return eSUCCESS;
      break;
    default:
      log("Serios screw-up in cast_shadowslip!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}
    
int cast_sanctuary( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      return spell_sanctuary(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_sanctuary(level, ch, ch, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if(tar_obj)
        return eFAILURE;
      if(!tar_ch)
        tar_ch = ch;
      return spell_sanctuary(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_STAFF:
      for(tar_ch = world[ch->in_room].people; tar_ch;
          tar_ch = tar_ch->next_in_room)
         spell_sanctuary(level,ch,tar_ch,0, skill);
      return eSUCCESS;
      break;
    default :
      log("Serious screw-up in sanctuary!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}

// TODO - make this use skill
int spell_camouflague(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill) 
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_CAMOUFLAGE)) {
     act("$n fades into the plant life.", victim, 0, 0, TO_ROOM, INVIS_NULL);
     act("You fade into the plant life.", victim, 0, 0, TO_CHAR, 0);
     
     af.type          = SPELL_CAMOUFLAGE;
     af.duration      = 3;
     af.modifier      = 0;
     af.location      = APPLY_NONE;
     af.bitvector     = AFF_CAMOUFLAGUE;
     affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

// TODO - make this use skill
int spell_farsight(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *tar_obj, int skill) 
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_FARSIGHT)) {
     act("Your eyesight improves.", victim, 0, 0, TO_CHAR, 0);

     af.type         = SPELL_FARSIGHT;
     af.duration     = level;
     af.modifier     = 0;
     af.location     = APPLY_NONE;
     af.bitvector    = AFF_FARSIGHT;
     affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

// TODO - make this use skill
int spell_freefloat(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *tar_obj, int skill) 
{
   struct affected_type af;

   if(!affected_by_spell(victim, SPELL_FREEFLOAT)) {
     act("You gain added stability.", victim, 0, 0, TO_CHAR, 0);

     af.type         = SPELL_FREEFLOAT;
     af.duration     = level;
     af.modifier     = 0;
     af.location     = APPLY_NONE;
     af.bitvector    = AFF_FREEFLOAT;
     affect_to_char(victim, &af);
   }
   return eSUCCESS;
}

// TODO - make this use skill
int spell_insomnia(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *tar_obj, int skill) 
{
   struct affected_type af;

   if(!affected_by_spell(victim, SPELL_INSOMNIA)) {
     act("You suddenly feel wide awake.", victim, 0, 0, TO_CHAR, 0);

     af.type          = SPELL_INSOMNIA;
     af.duration      = level;
     af.modifier      = 0;
     af.location      = APPLY_NONE;
     af.bitvector     = AFF_INSOMNIA;
     affect_to_char(victim, &af);
   }
   return eSUCCESS;
}


// TODO - make this use skill
int spell_shadowslip(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *tar_obj, int skill) 
{
   struct affected_type af;

   if(!affected_by_spell(victim, SPELL_SHADOWSLIP)) {
     act("Portals will no longer find you.", victim, 0, 0, TO_CHAR, 0);

     af.type           = SPELL_SHADOWSLIP;
     af.duration       = level/5;
     af.modifier       = 0;
     af.location       = APPLY_NONE;
     af.bitvector      = AFF_SHADOWSLIP;
     affect_to_char(victim, &af);
   }
   return eSUCCESS;
}

int spell_sanctuary(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
  {
    act("$N is already sanctified.",ch,0,victim,TO_CHAR, 0);
	 return eFAILURE;
  }

  if (!affected_by_spell(victim, SPELL_SANCTUARY))
  {
         skill_increase_check(ch, SPELL_SANCTUARY, skill, SKILL_INCREASE_EASY);

	 act("$n is surrounded by a $Bwhite aura$R.", victim, 0, 0, TO_ROOM, INVIS_NULL);
	 act("You start $Bglowing$R.", victim, 0, 0, TO_CHAR, 0);

	 af.type      = SPELL_SANCTUARY;
	 af.duration  = 3 + skill / 18;
	 af.modifier  = 0;
	 af.location  = APPLY_NONE;
	 af.bitvector = AFF_SANCTUARY;
	 affect_to_char(victim, &af);
  }
  return eSUCCESS;
}


// TODO - make this use skill
int spell_sleep(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  char buf[80];
  int retval;

  set_cantquit( ch, victim );

  if(!IS_MOB(victim) && GET_LEVEL(victim) <= 15) {
     send_to_char("Oh come on....at least wait till they're high enough level to have decent eq.\r\n", ch);
     return eFAILURE;
  }

  /* You can't sleep someone higher level than you*/
  if(affected_by_spell(victim, SPELL_INSOMNIA)
     || IS_AFFECTED2(victim, AFF_INSOMNIA)) {
      act("$N does not look sleepy!", ch, NULL, victim, TO_CHAR, 0);
      retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
      retval = SWAP_CH_VICT(retval);
      return retval;
  }
  if(affected_by_spell(victim, SPELL_SLEEP)) {
     act("$N's mind still has the lingering effects of a past sleep spell active.",
         ch, NULL, victim, TO_CHAR, 0);
     return eFAILURE;
  }

  if (affected_by_spell(victim, SPELL_PARALYZE)) {
	if (number(1,20) < 19)
	{
	  switch (number(1,3))
	  {
	    case 1:
        act("$N does not look sleepy!", ch, NULL, victim, TO_CHAR, 0);
		break;
	     case 2:
	     case 3:
	      send_to_char("The combined magics fizzle, and cause an explosion!\r\n",ch);
		{
		  act("$n wakes up in a burst of magical energies!",victim, NULL, NULL, TO_ROOM,0);
		  affect_from_char(victim, SPELL_PARALYZE);
		}
		break;
 	 }
	 return eFAILURE;
	}
  }

  if (level < GET_LEVEL(victim)){
	 sprintf(buf,"%s laughs in your face at your feeble attempt.\n\r", GET_SHORT(victim));
	 send_to_char(buf,ch);
	 sprintf(buf,"%s tries to make you sleep, but fails miserably.\n\r", GET_SHORT(ch));
	 send_to_char(buf,victim);
	 retval = one_hit(victim,ch,TYPE_UNDEFINED, FIRST);
         retval = SWAP_CH_VICT(retval);
	 return retval;
  }

  if(IS_MOB(victim) || number(1, 2) == 1)
 {
  if(saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) < 0)
  {
	af.type      = SPELL_SLEEP;
	af.duration  = 5;//(ch->level == 50?3:2);
	af.modifier  = 1;
	af.location  = APPLY_NONE;
	af.bitvector = AFF_SLEEP;
	affect_join(victim, &af, FALSE, FALSE);

	if (GET_POS(victim)>POSITION_SLEEPING)
	{
	  act("You feel very sleepy ..... zzzzzz",victim,0,0,TO_CHAR,0);
	  act("$n goes to sleep.",victim,0,0,TO_ROOM, INVIS_NULL);
          stop_fighting(victim);
	  GET_POS(victim)=POSITION_SLEEPING;
	}
	return eSUCCESS;
  }
  else
	 act("$N does not look sleepy!", ch, NULL, victim, TO_CHAR, 0);
 }
  else
	 act("$N does not look sleepy!", ch, NULL, victim, TO_CHAR, 0);

  retval = one_hit(victim,ch,TYPE_UNDEFINED, FIRST);
  retval = SWAP_CH_VICT(retval);
  return retval;
}

int spell_strength(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  struct affected_type * cur_af;

  assert(victim);

  int mod = 1 + (skill/20);

  if((cur_af = affected_by_spell(victim, SPELL_STRENGTH)))
  {
    if(cur_af->modifier <= mod)
       affect_from_char(victim, SPELL_STRENGTH);
    else {
       send_to_char("That person already has a stronger strength spell!\r\n", ch);
       return eFAILURE;
    }
  }

  skill_increase_check(ch, SPELL_STRENGTH, skill, SKILL_INCREASE_MEDIUM);

  act("You feel stronger.", victim,0,0,TO_CHAR, 0);

  af.type      = SPELL_STRENGTH;
  af.duration  = level/2 + skill/3;
  af.modifier  = mod;

  af.location  = APPLY_STR;
  af.bitvector = 0;

  affect_to_char(victim, &af);
  return eSUCCESS;
}



int spell_ventriloquate(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
	 /* Actual spell resides in cast_ventriloquate */
  return eSUCCESS;
}


int spell_word_of_recall(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int location;
  char buf[100];
  struct clan_data * clan;
  struct clan_room_data * room;
  int found = 0; 

  int do_look(CHAR_DATA *ch, char *argument, int cmd);

  if(IS_AFFECTED(victim, AFF_SOLIDITY)) {
      send_to_char("You find yourself unable to.\n\r", ch);
      if(ch != victim) {
          sprintf(buf, "%s just tried to recall you.\n\r", GET_SHORT(ch));
          send_to_char(buf, victim);
      }
      return eFAILURE;
  }
  assert(victim);
  if(IS_SET(world[victim->in_room].room_flags, ARENA)) {
    send_to_char("To the DEATH you wimp!\n\r", ch);
    return eFAILURE;
  }

  if(IS_AFFECTED(ch, AFF_CHARM))
    return eFAILURE;

  if (IS_NPC(victim))
    location = real_room(GET_HOME(victim));
  else
  {
    if(affected_by_spell(victim, FUCK_PTHIEF))
      location = real_room(START_ROOM);
    else
      location = real_room(GET_HOME(victim));

    if(IS_AFFECTED(ch, AFF_CANTQUIT))
      location = real_room(START_ROOM);
    else
      location = real_room(GET_HOME(victim));

    // make sure they arne't recalling into someone's chall
    if(IS_SET(world[location].room_flags, CLAN_ROOM))
       if(!victim->clan || !(clan = get_clan(victim))) {
         send_to_char("The gods frown on you, and reset your home.\r\n", ch);
         location = real_room(START_ROOM);
         GET_HOME(victim) = START_ROOM;
       }
       else {
          for(room = clan->rooms; room; room = room->next)
            if(room->room_number == GET_HOME(victim))
               found = 1;

          if(!found) {
             send_to_char("The gods frown on you, and reset your home.\r\n", ch);
             location = real_room(START_ROOM);
             GET_HOME(victim) = START_ROOM;
          }
       }
  }

  if ( location == -1 )
  {
	 send_to_char("You are completely lost.\n\r", victim);
	 return eFAILURE;
  }

  skill_increase_check(ch, SPELL_WORD_OF_RECALL, skill, SKILL_INCREASE_MEDIUM);

  /* a location has been found. */
  if( number(1, 100) > (80 + skill / 10) )
  {
    send_to_char("Your recall magic sputters and fails.\r\n", ch);
    return eFAILURE;
  }

  act("$n disappears.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  move_char(victim, location);
  act("$n appears out of nowhere.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  do_look(victim, "", 15);
  return eSUCCESS;
}


int spell_wizard_eye(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int target;
  int original_loc;

  assert(ch && victim);

  if(IS_SET(world[victim->in_room].room_flags, NO_MAGIC) || 
     (GET_LEVEL(victim) >= IMMORTAL && GET_LEVEL(ch) < IMMORTAL)
    ) 
  {
    send_to_char("Your vision is too clouded to make out anything.\n\r", ch);
    return eFAILURE;
  }

  if(IS_AFFECTED2(victim, AFF_FOREST_MELD)) {
    send_to_char("Your target's location is hidden by the forests.\r\n", ch);
    return eFAILURE;
  }
  if (affected_by_spell(victim, SKILL_INNATE_EVASION))
  {
    send_to_char("Your target evades your wiz-eye!\r\n",ch);
    return eFAILURE;
  }

  if(number(0, 100) > skill) {
    send_to_char("Your spell fails to locate its target.\r\n", ch);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_WIZARD_EYE, skill, SKILL_INCREASE_MEDIUM);

  original_loc = ch->in_room;
  target        =  victim->in_room;

  move_char(ch, target);
  send_to_char("A vision forms in your mind... \n\r", ch);
  do_look(ch,"",15);
  move_char(ch, original_loc);
  return eSUCCESS;
}

int spell_summon(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  unsigned target;
  int retval;
  
  assert(ch && victim);
  
  if(IS_SET(world[victim->in_room].room_flags, SAFE)) {
    send_to_char("That person is in a safe area!\n\r",ch);
    return eFAILURE;
  }

  if((GET_LEVEL(victim) > MIN(MORTAL, level+3)) && GET_LEVEL(ch) < IMP) {
    send_to_char("You failed.\n\r",ch);
    return eFAILURE;
  }

  if(!IS_NPC(ch))
  if(IS_NPC(victim) || !IS_SET(victim->pcdata->toggles, PLR_SUMMONABLE)) {
	 send_to_char("Someone has tried to summon you!\n\r", victim);
	 send_to_char("Something strange about that person prevents"
					  " your magic.\n\r", ch);
	 return eFAILURE;
  }

  if ((IS_NPC(victim) && GET_LEVEL(ch) < IMP) ||
		IS_SET(world[victim->in_room].room_flags,PRIVATE)  ||
		IS_SET(world[victim->in_room].room_flags,NO_SUMMON) ) {
	 send_to_char("You failed.\n\r", ch);
	 return eFAILURE;
  }

  if(IS_ARENA(ch->in_room)) 
    if(!IS_ARENA(victim->in_room))  {
       send_to_char("You can't summon someone INTO an arena!\n\r", ch);
       return eFAILURE;
    }

  if(number(1, 100) > 50 + skill/2) {
    send_to_char("Your summoning fails.\r\n", ch);
    return eFAILURE;
  }
    
  skill_increase_check(ch, SPELL_SUMMON, skill, SKILL_INCREASE_MEDIUM);
  
  act("$n disappears suddenly.",victim,0,0,TO_ROOM, INVIS_NULL);

  target = ch->in_room;
  move_char(victim, target);

  act("$n arrives suddenly.",victim,0,0,TO_ROOM, INVIS_NULL);
  act("$n has summoned you!",ch,0,victim,TO_VICT, 0);
  do_look(victim,"",15);

  if (IS_NPC(victim) && GET_LEVEL(victim) >= GET_LEVEL(ch))  {
    act("$n growls.", victim, 0,0,TO_ROOM, 0);
    retval = one_hit(victim, ch,TYPE_UNDEFINED, FIRST);
    retval = SWAP_CH_VICT(retval);
    return retval;
  }
  else if (IS_NPC(victim))  {
    act("$n freaks shit.", victim, 0,0,TO_ROOM, 0);
    add_memory(victim, GET_NAME(ch), 'f');
    do_flee(victim, "", 0);
  }
  return eSUCCESS;
}


// TODO - make this use skill
int spell_charm_person(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  struct obj_data *tempobj;

  void add_follower(CHAR_DATA *ch, CHAR_DATA *leader, int cmd);
  void stop_follower(CHAR_DATA *ch, int cmd);

  send_to_char("Disabled currently.\r\n", ch);
  return eFAILURE;

  if(victim == ch) {
	 send_to_char("You like yourself even better!\n\r", ch);
	 return eFAILURE;
  }

  if(!IS_NPC(victim)) {
	 send_to_char("You find yourself unable to charm this player.\n\r", ch);
	 return eFAILURE;
  }

  if(IS_AFFECTED(victim, AFF_CHARM) || IS_AFFECTED(ch, AFF_CHARM) || level <= GET_LEVEL(victim))
	 return eFAILURE;

  if(circle_follow(victim, ch)) {
	  send_to_char("Sorry, following in circles can not be allowed.\n\r", ch);
	  return eFAILURE;
  }

  if(many_charms(ch))  {
	  send_to_char("How do you plan on controlling so many followers?\n\r", ch);
	  return eFAILURE;
  }

  if(IS_SET(victim->immune, ISR_CHARM) ||
		(IS_MOB(victim) && !IS_SET(victim->mobdata->actflags, ACT_CHARM))) {
	 act("$N laughs at your feeble charm attempt.", ch, NULL, victim,
		  TO_CHAR, 0);
	 return eFAILURE;
  }

  if(saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) >= 0) {
	 act("$N doesnt seem to be affected.", ch, NULL, victim, TO_CHAR, 0);
	 return eFAILURE;
  }

  if(victim->master)
	 stop_follower(victim, STOP_FOLLOW);

  add_follower(victim, ch, 0);

  af.type      = SPELL_CHARM_PERSON;

  if(GET_INT(victim))
	 af.duration  = 24*18/GET_INT(victim);
  else
	 af.duration  = 24*18;

  af.modifier  = 0;
  af.location  = 0;
  af.bitvector = AFF_CHARM;
  affect_to_char(victim, &af);

  act("Isn't $n just such a nice fellow?", ch , 0, victim, TO_VICT, 0);
  if(victim->equipment[WIELD]) {
    if(victim->equipment[SECOND_WIELD]) {
      tempobj = unequip_char(victim, SECOND_WIELD);
      obj_to_room(tempobj, victim->in_room);
    }
    tempobj = unequip_char(victim, WIELD);
    obj_to_room(tempobj, victim->in_room);
    act("$n's eyes dull and $s hands slacken dropping $s weapons.", victim, 0, 0, TO_ROOM, 0);
  }  
  return eSUCCESS;
}



int spell_sense_life(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  assert(victim);

  if(affected_by_spell(victim, SPELL_SENSE_LIFE))
    affect_from_char(victim, SPELL_SENSE_LIFE);

  skill_increase_check(ch, SPELL_SENSE_LIFE, skill, SKILL_INCREASE_MEDIUM);

  send_to_char("Your feel your awareness improve.\n\r", victim);

  af.type      = SPELL_SENSE_LIFE;
  af.duration  = skill * 2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_SENSE_LIFE;
  affect_to_char(victim, &af);
  return eSUCCESS;
}

void show_obj_class_size_mini(obj_data * obj, char_data * ch)
{
   extern char *extra_bits[];

   
   for(int i = 12; i < 23; i++)
      if(IS_SET(obj->obj_flags.extra_flags, 1<<i))
         csendf(ch, " %s", extra_bits[i]);
}

// TODO - make this use skill
int spell_identify(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  char buf[MAX_STRING_LENGTH], buf2[256];
  int i;
  bool found;
  int value;

  struct time_info_data age(CHAR_DATA *ch);

  extern char *race_types[];

  /* Spell Names */
  extern char *spells[];

  /* For Objects */
  extern char *item_types[];
  extern char *extra_bits[];
  extern char *more_obj_bits[];
  extern char *apply_types[];
  extern char *size_bits[];

  assert(obj || victim);

  if (obj) {
         if(obj->in_room > -1)
         {
            // it's an obj in a room.  If it's not a corpse, don't id it
            if(GET_ITEM_TYPE(obj) != ITEM_CONTAINER || obj->obj_flags.value[3] != 1) {
               send_to_char("Your magical probing reveals nothing of interest.\r\n", ch);
               return eSUCCESS;
            }
            send_to_char("You probe the contents of the corpse magically....\r\n", ch);
            // it's a corpse
            struct obj_data *iobj;
            for(iobj = obj->contains; iobj; iobj = iobj->next_content)
            {
               if(!CAN_SEE_OBJ(ch, iobj))
                  continue;
               send_to_char(iobj->short_description, ch);
               if(IS_SET(iobj->obj_flags.more_flags, ITEM_NO_TRADE)) {
                  send_to_char(" $BNO_TRADE$R", ch);
                  show_obj_class_size_mini(iobj, ch);
               }
               send_to_char("\r\n", ch);
            }
            return eSUCCESS;
         }
         if(IS_SET(obj->obj_flags.extra_flags, ITEM_DARK) && GET_LEVEL(ch)
                                                       < POWER)
         {
            send_to_char("A magical aura around the item attempts to conceal its secrets.\r\n", ch);
            return eFAILURE;
         }

	 send_to_char("You feel informed:\n\r", ch);

	 sprintf(buf, "Object '%s', Item type: ", obj->name);
	 sprinttype(GET_ITEM_TYPE(obj),item_types,buf2);
	 strcat(buf,buf2); 
         strcat(buf,"\n\r");
	 send_to_char(buf, ch);

	 send_to_char("Item is: ", ch);
	 sprintbit(obj->obj_flags.extra_flags,extra_bits,buf);
         sprintbit(obj->obj_flags.more_flags,more_obj_bits,buf2);
         strcat(buf, " ");
         strcat(buf,buf2);
	 strcat(buf,"\n\r");
	 send_to_char(buf,ch);

         send_to_char("Worn by: ", ch);
         sprintbit(obj->obj_flags.size, size_bits, buf);
         strcat(buf, "\r\n");
         send_to_char(buf, ch);

	 sprintf(buf,"Weight: %d, Value: %d\n\r", obj->obj_flags.weight, obj->obj_flags.cost);
	 send_to_char(buf, ch);

	 switch (GET_ITEM_TYPE(obj)) {

	 case ITEM_SCROLL :
	 case ITEM_POTION :
		sprintf(buf, "Level %d spells of:\n\r",   obj->obj_flags.value[0]);
		send_to_char(buf, ch);
		if (obj->obj_flags.value[1] >= 1) {
	 sprinttype(obj->obj_flags.value[1]-1,spells,buf);
	 strcat(buf,"\n\r");
	 send_to_char(buf, ch);
		}
		if (obj->obj_flags.value[2] >= 1) {
	 sprinttype(obj->obj_flags.value[2]-1,spells,buf);
	 strcat(buf,"\n\r");
	 send_to_char(buf, ch);
		}
		if (obj->obj_flags.value[3] >= 1) {
	 sprinttype(obj->obj_flags.value[3]-1,spells,buf);
	 strcat(buf,"\n\r");
	 send_to_char(buf, ch);
		}
		break;

	 case ITEM_WAND :
	 case ITEM_STAFF :
		sprintf(buf, "Has %d charges, with %d charges left.\n\r",
	  obj->obj_flags.value[1],
	  obj->obj_flags.value[2]);
		send_to_char(buf, ch);

		sprintf(buf, "Level %d spell of:\n\r",    obj->obj_flags.value[0]);
		send_to_char(buf, ch);

		if (obj->obj_flags.value[3] >= 1) {
	 sprinttype(obj->obj_flags.value[3]-1,spells,buf);
	 strcat(buf,"\n\r");
	 send_to_char(buf, ch);
		}
		break;

	 case ITEM_WEAPON :
		sprintf(buf, "Damage Dice is '%dD%d'\n\r",
	  obj->obj_flags.value[1],
	  obj->obj_flags.value[2]);
		send_to_char(buf, ch);
		break;

         case ITEM_INSTRUMENT:
                sprintf(buf, "Effects non-combat singing by '%d'\r\nEffects combat singing by '%d'\r\n",
                     obj->obj_flags.value[0],
                     obj->obj_flags.value[1]);
                send_to_char(buf, ch);
                break;

         case ITEM_MISSILE :
		sprintf(buf, "Damage Dice is '%dD%d'\n\rIt is +%d to arrow hit and +%d to arrow damage\r\n",
	  obj->obj_flags.value[0],
	  obj->obj_flags.value[1],
	  obj->obj_flags.value[2],
	  obj->obj_flags.value[3]);
		send_to_char(buf, ch);
		break;

         case ITEM_FIREWEAPON :
                sprintf(buf, "Bow is +%d to arrow hit and +%d to arrow damage.\r\n",
	  obj->obj_flags.value[0],
	  obj->obj_flags.value[1]);
                send_to_char(buf, ch);
                break;

	 case ITEM_ARMOR :

		if (IS_SET(obj->obj_flags.extra_flags, ITEM_ENCHANTED))
			value = obj->obj_flags.value[0];
		 else
			value = (obj->obj_flags.value[0]) - (obj->obj_flags.value[1]);

		sprintf(buf, "AC-apply is %d     Resistance to damage is %d\n\r",
		  value, obj->obj_flags.value[2]);
		send_to_char(buf, ch);
		break;

	 }

	 found = FALSE;

	 for (i=0;i<obj->num_affects;i++) {
		if ((obj->affected[i].location != APPLY_NONE) &&
		(obj->affected[i].modifier != 0)) 
	{
	 	if (!found) {
			send_to_char("Can affect you as:\n\r", ch);
			found = TRUE;
	 	}

		 sprinttype(obj->affected[i].location,apply_types,buf2);
		 sprintf(buf,"    Affects : %s By %d\n\r", buf2,obj->affected[i].modifier);
	 	send_to_char(buf, ch);
	}
	 }

  } else {       /* victim */

	 if (!IS_NPC(victim)) {
		sprintf(buf,"%d Years,  %d Months,  %d Days,  %d Hours old.\n\r",
	  age(victim).year, age(victim).month,
	  age(victim).day, age(victim).hours);
		send_to_char(buf,ch);


		  sprintf(buf,"Race: ");
		  sprinttype(victim->race,race_types, buf2);
		 strcat(buf, buf2);
			 send_to_char(buf, ch);

		sprintf(buf,"   Height %dcm  Weight %dpounds \n\r",
	  GET_HEIGHT(victim), GET_WEIGHT(victim));
		send_to_char(buf,ch);

  if (GET_LEVEL(victim) > 9) {

  sprintf(buf,"Str%d,  Int %d,  Wis %d,  Dex %d,  Con %d\n\r",
  GET_STR(victim),
  GET_INT(victim),
  GET_WIS(victim),
  GET_DEX(victim),
  GET_CON(victim) );
  send_to_char(buf,ch);
}



	 } else {
		send_to_char("You learn nothing new.\n\r", ch);
	 }
  }
  return eSUCCESS;
}


/* ***************************************************************************
 *                     NPC spells..                                          *
 * ************************************************************************* */



int spell_frost_breath(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
	 int dam;
	 int hpch;
         int retval;
	 /*struct obj_data *frozen;*/


	 set_cantquit( ch, victim );

	 hpch = GET_HIT(ch);
	 if(hpch<10) hpch=10;

	 dam = number((hpch/8)+1,(hpch/4));

//	 if(saves_spell(ch, victim, 0, SAVE_TYPE_COLD) >= 0)
//	   dam >>= 1;

	 retval = spell_damage(ch, victim, dam,TYPE_COLD, SPELL_FROST_BREATH, 0);
         if(SOMEONE_DIED(retval))
	  {
	    /* The character's DEAD, don't mess with him */
	    return retval;
	  }

	 /* And now for the damage on inventory */
/*
// TODO - make frost breath do something cool *pun!*

	 if(number(0,100) < GET_LEVEL(ch))
	 {
		if(!saves_spell(ch, victim, SAVING_BREATH) )
	{
	  for(frozen=victim->carrying ;
	  frozen && !(((frozen->obj_flags.type_flag==ITEM_DRINKCON) ||
			(frozen->obj_flags.type_flag==ITEM_FOOD) ||
			(frozen->obj_flags.type_flag==ITEM_POTION)) &&
		  (number(0,2)==0)) ;
	  frozen=frozen->next_content);
	  if(frozen)
	{
	  act("$o breaks.",victim,frozen,0,TO_CHAR, 0);
	  extract_obj(frozen);
	}
	}
	 }
*/
  return eSUCCESS;
}


int spell_acid_breath(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
	 int dam;
	 int hpch;
	 int retval;

	 int apply_ac(CHAR_DATA *ch, int eq_pos);


	 set_cantquit( ch, victim );

	 hpch = GET_HIT(ch);
	 if(hpch<10) hpch=10;

	 dam = number((hpch/8)+1,(hpch/4));

//	 if(saves_spell(ch, victim, 0, SAVE_TYPE_ACID) >= 0)
//	   dam >>= 1;

	 retval = spell_damage(ch, victim, dam,TYPE_ACID, SPELL_ACID_BREATH, 0);
         if(SOMEONE_DIED(retval))
	 {
	   return retval;
	 }

	 /* And now for the damage on equipment */
/*
// TODO - make this do something cool
	 if(number(0,100)<GET_LEVEL(ch))
	 {
		if(!saves_spell(ch, victim, SAVING_BREATH))
	{
	  for(damaged = 0; damaged<MAX_WEAR &&
	  !((victim->equipment[damaged]) &&
		 (victim->equipment[damaged]->obj_flags.type_flag!=ITEM_ARMOR) &&
		 (victim->equipment[damaged]->obj_flags.value[0]>0) &&
		 (number(0,2)==0)) ; damaged++);
	  if(damaged<MAX_WEAR)
	{
	  act("$o is damaged.",victim,victim->equipment[damaged],0,TO_CHAR,0);
	  GET_AC(victim)-=apply_ac(victim,damaged);
	  victim->equipment[damaged]->obj_flags.value[0]-=number(1,7);
	  GET_AC(victim)+=apply_ac(victim,damaged);
	  victim->equipment[damaged]->obj_flags.cost = 0;
	}
	}
	 }
*/
  return eSUCCESS;
}


int spell_fire_breath(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int retval;
  CHAR_DATA *tmp_victim, *temp;

  act("$B$4You are $IENVELOPED$I$B$4 in scorching flames!$R", ch, 0, 0, TO_ROOM, 0);

  for(tmp_victim = character_list; tmp_victim; tmp_victim = temp)
  {
    temp = tmp_victim->next;

    if ( (ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
         (IS_NPC(ch) ? !IS_NPC(tmp_victim) : TRUE) ) // if i'm a mob, don't hurt other mobs
    {
      if(GET_DEX(tmp_victim) > number(1, 100)) // roll vs dex dodged
      {
        send_to_char("You dive out of the way of the main blast avoiding the inferno!\n\r", ch);
        act("$n barely dives to the side avoiding the heart of the flame.", ch, 0, 0, TO_ROOM, 0);
        continue;
      }

      dam = dice(level, 8);

//      if(saves_spell(ch,  tmp_victim, 0, SAVE_TYPE_FIRE) >= 0)
  //      dam >>= 1;

      retval = spell_damage(ch, tmp_victim, dam, TYPE_FIRE, SPELL_FIRE_BREATH, 0);
      if(IS_SET(retval, eCH_DIED))
        return retval;
    } else
        if (world[ch->in_room].zone == world[tmp_victim->in_room].zone)
          send_to_char("You feel a HOT blast of air.\n\r", tmp_victim);
  }
  return eSUCCESS;
}

int spell_gas_breath(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int retval;
  CHAR_DATA *tmp_victim, *temp;

  act("You CHOKE on the gas fumes!",
		ch, 0, 0, TO_ROOM, 0);


  for(tmp_victim = character_list; tmp_victim; tmp_victim = temp)
  {
	 temp = tmp_victim->next;
	 if ( (ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
		(IS_NPC(tmp_victim) || IS_NPC(ch))){

	  dam = dice(level, 6);

		// if(saves_spell(ch,  tmp_victim, 0, SAVE_TYPE_POISON) >= 
//0)
		//	dam >>= 1;

		 retval = spell_damage(ch, tmp_victim, dam,TYPE_POISON, SPELL_GAS_BREATH, 0);
                 if(IS_SET(retval, eCH_DIED))
                   return retval;
	 } else
		if (world[ch->in_room].zone == world[tmp_victim->in_room].zone)
			send_to_char("You wanna choke on the smell in the air.\n\r", tmp_victim);
  }
  return eSUCCESS;
}

int spell_lightning_breath(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
	 int dam;
	 int hpch;

	 set_cantquit( ch, victim );

	 hpch = GET_HIT(ch);
	 if(hpch<10) hpch=10;

	 dam = number((hpch/8)+1,(hpch/4));

//	 if(saves_spell(ch, victim, 0, SAVE_TYPE_ENERGY) >= 0)
//	   dam >>= 1;

	 return spell_damage(ch, victim, dam,TYPE_ENERGY, SPELL_LIGHTNING_BREATH, 0);
}

// TODO - make this use skill
int spell_fear(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
	int retval;
	 assert(victim && ch);
         char buf[256]; 
	 if(IS_NPC(victim) && IS_SET(victim->mobdata->actflags, ACT_STUPID)) {
           sprintf(buf, "%s doesn't understand your psychological tactics.\n\r",
                   GET_SHORT(victim));
           send_to_char(buf, ch);
	   return eFAILURE;
         }
         if(GET_POS(victim) == POSITION_SLEEPING) {
           send_to_char("How do you expect a sleeping person to be scared?\r\n", ch);
           return eFAILURE;
         }

         set_cantquit( ch, victim );

         if(IS_SET(victim->combat, COMBAT_BERSERK)) {
           act("$N looks at you with glazed over eyes, drools, and continues to fight!",
               ch, NULL, victim, TO_CHAR, 0);
           act("$N smiles madly, drool running down his chin as he ignores $n's magic!",
               ch, NULL, victim, TO_ROOM, 0);
           act("You grin as $n realizes you have no target for his mental attack!",
               ch, NULL, victim, TO_VICT, 0);
           return eFAILURE;
         }

	 if((saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) >= 0) || (!number(0, 5))) {
		send_to_char("For a moment you feel compelled to run away, but you fight back the urge.\n\r", victim);
		act("$N doesnt seem to be the yellow bellied slug you thought!", ch, NULL, victim, TO_CHAR, 0);
		retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
		retval = SWAP_CH_VICT(retval);
		return retval;
	 }
	 send_to_char("You suddenly feel very frightened, and you attempt to flee!\n\r", victim);
	 do_flee(victim, "", 151);

  return eSUCCESS;
}

int spell_refresh(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;

  if(!ch || !victim)
  {
    log("NULL ch or victim sent to spell_refresh!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_REFRESH, skill, SKILL_INCREASE_MEDIUM);

  dam = dice(skill/2, 4) + skill/2;
  dam = MAX(dam, 20);

  if ((dam + GET_MOVE(victim)) > move_limit(victim))
    GET_MOVE(victim) = move_limit(victim);
  else
    GET_MOVE(victim) += dam;

  send_to_char("You feel less tired.\n\r", victim);
  return eSUCCESS;
}

int spell_fly(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  struct affected_type *cur_af;

  if(!ch || !victim)
  {
    log("NULL ch or victim sent to spell_fly!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if((cur_af = affected_by_spell(victim, SPELL_FLY)))
    affect_remove(victim, cur_af, SUPPRESS_ALL,FALSE);
    //affect_from_char(victim, SPELL_FLY);

  if(IS_AFFECTED(victim, AFF_FLYING))
    return eFAILURE;

  skill_increase_check(ch, SPELL_FLY, skill, SKILL_INCREASE_MEDIUM);

  send_to_char("You start flapping and rise off the ground!\n\r", victim);
  if (ch != victim)
    act("$N start flapping and rise off the ground!", ch, NULL, victim, TO_CHAR, 0);
  act("$N's feet rise off the ground.", ch, 0, victim, TO_ROOM, INVIS_NULL|NOTVICT);
  
  af.type = SPELL_FLY;
  af.duration = skill + 3;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_FLYING;
  affect_to_char(victim, &af);
  return eSUCCESS;
}

// Creates a ball of light in the hands
// TODO - make this use skill
int spell_cont_light(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct obj_data *tmp_obj;

  if(!ch) {
    log("NULL ch sent to cont_light!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(obj) {
    if(IS_SET(obj->obj_flags.extra_flags, ITEM_GLOW)) {
       send_to_char("That item is already glowing with magical light.\r\n", ch);
       return eFAILURE;
    }
    if(GET_ITEM_TYPE(obj) != ITEM_ARMOR) {
       send_to_char("Only pieces of equipment may be magically lit in such a way.\r\n", ch);
       return eFAILURE;
    }
    if(!CAN_WEAR(obj, ITEM_WEAR_EAR) && !CAN_WEAR(obj, ITEM_WEAR_SHIELD) && !CAN_WEAR(obj, ITEM_WEAR_FINGER)) {
       send_to_char("Only earrings, rings, and shields can be magically lit in such a way.\r\n", ch);
       return eFAILURE;
    }
    SET_BIT(obj->obj_flags.extra_flags, ITEM_GLOW);
    act("$n twiddles $s thumbs and the $p $e is carrying begins to glow.", ch, obj, 0, TO_ROOM, INVIS_NULL);
    act("You twiddle your thumbs and the $p begins to glow.", ch, obj, 0, TO_CHAR, 0);
    return eSUCCESS;
  }

/*  
  tmp_obj = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
  clear_object(tmp_obj);

  tmp_obj->name = str_hsh("ball light");
  tmp_obj->short_description = str_hsh("A bright ball of light");
  tmp_obj->description = str_hsh("There is a bright ball of light on "
                                 "the ground here.");

  tmp_obj->obj_flags.type_flag = ITEM_LIGHT;
  tmp_obj->obj_flags.wear_flags = (ITEM_TAKE | ITEM_HOLD | ITEM_LIGHT_SOURCE);
  tmp_obj->obj_flags.size = (SIZE_ANY|SIZE_CHARMIE_OK);
  tmp_obj->obj_flags.extra_flags = ITEM_ANY_CLASS;
  tmp_obj->obj_flags.value[2] = -1;
  tmp_obj->obj_flags.weight = 1;
  tmp_obj->obj_flags.cost = 0;

  tmp_obj->next = object_list;
  object_list = tmp_obj;

  tmp_obj->item_number = -1;

*/

  tmp_obj = clone_object(real_object(6));

  obj_to_char(tmp_obj, ch);

  act("$n twiddles $s thumbs and $p suddenly appears.", ch, tmp_obj, 0, TO_ROOM, INVIS_NULL);
  act("You twiddle your thumbs and $p suddenly appears.", ch, tmp_obj, 0, TO_CHAR, 0);
  return eSUCCESS;
}

// TODO - make this use skill
int spell_animate_dead(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *corpse, int skill)
{
  CHAR_DATA *mob;
  struct obj_data *obj_object, *next_obj;
  struct affected_type af;
  char buf[200];
  int number, r_num;

  if(!IS_EVIL(ch) && GET_LEVEL(ch) < ARCHANGEL) {
    send_to_char("You aren't evil enough to cast such a repugnant spell.\n\r",
                 ch);
    return eFAILURE;
  }

  if(many_charms(ch))  {
    send_to_char("How do you plan on controlling so many followers?\n\r", ch);
    return eFAILURE;
  }

  // check to see if its an eligible corpse
  if ((GET_ITEM_TYPE(corpse) != ITEM_CONTAINER) ||
      !corpse->obj_flags.value[3] || isname("pc", corpse->name)) {
    act("$p shudders for a second, then lies still.", ch, corpse, 0,
        TO_CHAR, 0);
    act("$p shudders for a second, then lies still.", ch, corpse, 0,
        TO_ROOM, 0);
    return eFAILURE;
  }

  if(level < 20)
    number = 22394;
  else if(level < 30)
    number = 22395;
  else if(level < 40)
    number = 22396;
  else if(level < 50)
    number = 22397;
  else
    number = 22398;

  if ((r_num = real_mobile(number)) < 0) {
    send_to_char("Mobile: Zombie not found.\n\r", ch);
    return eFAILURE;
  }
  
  mob = clone_mobile(r_num);
  char_to_room(mob, ch->in_room);

  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  // take all from corpse, and give to zombie

  for (obj_object = corpse->contains; obj_object; obj_object = next_obj) {
    next_obj = obj_object->next_content;
    move_obj(obj_object, mob);
  }

  // set up descriptions and such

  sprintf(buf, "%s %s", corpse->name, mob->name);
  mob->name = str_hsh(buf);
  
  mob->short_desc = str_hsh(corpse->short_description);

  sprintf(buf, "%s slowly staggers around.\n\r", corpse->short_description);
  mob->long_desc = str_hsh(buf);

  act("Calling upon your foul magic, you animate $p.\n\r$N slowly lifts "
      "itself to its feet.", ch, corpse, mob, TO_CHAR, INVIS_NULL);
  act("Calling upon $s foul magic, $n animates $p.\n\r$N slowly lifts "
      "itself to its feet.", ch, corpse, mob, TO_ROOM, INVIS_NULL);

  // zombie should be charmed and follower ch

  af.type = SPELL_CHARM_PERSON;
  af.duration = 5 + level / 2;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_CHARM;
  affect_to_char(mob, &af);
  if(IS_SET(mob->immune, ISR_PIERCE))
    REMOVE_BIT(mob->immune, ISR_PIERCE);
  add_follower(mob, ch, 0);

  extract_obj(corpse);
  return eSUCCESS;
}

//  This should be checked out for real mob creating.

// TODO - make this use skill
int spell_know_alignment(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int ap;
  char buf[200], name[100];

  if(!ch || !victim) {
    log("NULL ch or victim sent to know_alignment!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if (IS_NPC(victim))
    strcpy(name, victim->short_desc);
  else
    strcpy(name, GET_SHORT(victim));

  ap = GET_ALIGNMENT(victim);

  if (ap > 700)
    sprintf(buf, "%s has an aura as white as the driven snow.\n\r", name);
  else if (ap > 350)
    sprintf(buf, "%s is of excellent moral character.\n\r", name);
  else if (ap > 100)
    sprintf(buf, "%s is often kind and thoughtful.\n\r", name);
  else if (ap > 25)
    sprintf(buf, "%s isn't a bad sort...\n\r", name);
  else if (ap > -25)
    sprintf(buf, "%s doesn't seem to have a firm moral commitment\n\r", name);
  else if (ap > -100)
    sprintf(buf, "%s could be a little nicer, but who couldn't?\n\r", name);
  else if (ap > -350)
    sprintf(buf, "%s isn't the worst you've come across\n\r", name);
  else if (ap > -700)
    sprintf(buf, "%s probably committed more than a few crimes.\n\r", name);
  else if (ap > -1000)
    sprintf(buf, "%s is definitely Evil Incarnate.\n\r", name);
  else
    sprintf(buf, "I'd rather just not say anything at all about %s\n\r", name);

  send_to_char(buf, ch);
  return eSUCCESS;
}

/* dispels detection spelsl and other small non-combat ones */
// TODO - make this use skill
int spell_dispel_minor(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int rots = 0;
   int done = FALSE;
   int retval;

   if(obj) /* Trying to dispel_minor an obj */
   {
      if(GET_ITEM_TYPE(obj) != ITEM_BEACON) {
         send_to_char("You can't dispel that!\n\r", ch);
         return eFAILURE;
      }
      if(!obj->equipped_by) {
         // Someone load it or something?
         send_to_char("The magic fades away back to the ether.\n\r", ch);
         act("$p fades away gently.", ch, obj, 0, TO_ROOM, INVIS_NULL);
      }
      else {
         send_to_char("The magic is shattered by your will!\n\r", ch);
         act("$p blinks out of existance with a bang!", ch, obj, 0, TO_ROOM, INVIS_NULL);
         send_to_char("Your magic beacon is shattered!\n\r", obj->equipped_by);
         obj->equipped_by->beacon = NULL;
         obj->equipped_by = NULL;
      }
      extract_obj(obj);
      return eSUCCESS;
   }

   if(!ch || !victim)
   {
      log("Null ch or victim sent to dispel_minor!", ANGEL, LOG_BUG);
      return eFAILURE;
   }

   if(!IS_NPC(ch) && !IS_NPC(victim) && victim->fighting &&
      IS_NPC(victim->fighting)) 
   {
      send_to_char("You misfire!\n\r", ch);
      victim = ch;
   }

   // If victim higher level, they get a save vs magic for no effect
   if((GET_LEVEL(victim) > GET_LEVEL(ch)) && 0 > saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC))
      return eFAILURE;

   // Input max number of spells in switch statement here
   while(!done && ((rots += 1) < 10))
   {
      switch(number(1, 12)) 
      {
         case 1: 
            if (affected_by_spell(victim, SPELL_INVISIBLE))
            {
               affect_from_char(victim, SPELL_INVISIBLE);
               send_to_char("You feel exposed.\n\r", victim);
               done = TRUE;
            }
            if (IS_AFFECTED(victim, AFF_INVISIBLE))
            {
               REMOVE_BIT(victim->affected_by, AFF_INVISIBLE);
               send_to_char("You feel exposed.\n\r", victim);
               act("$n fades into existance.", victim, 0, 0, TO_ROOM, 0);
               done = TRUE;
            }
            break;

	 case 2: 
            if (affected_by_spell(victim, SPELL_DETECT_INVISIBLE))
            {
               affect_from_char(victim, SPELL_DETECT_INVISIBLE);
               send_to_char("You feel less perceptive.\n\r", victim);
                done = TRUE;
            }
            break;

	 case 3: 
            if (affected_by_spell(victim, SPELL_DETECT_EVIL))
            {
               affect_from_char(victim, SPELL_DETECT_EVIL);
               send_to_char("You feel less morally alert.\n\r", victim);
               done = TRUE;
            }
            break;

	 case 4: 
            if (affected_by_spell(victim, SPELL_DETECT_MAGIC))
            {
               affect_from_char(victim, SPELL_DETECT_MAGIC);
               send_to_char("You stop noticing the magic in your life.\n\r", victim);
               done = TRUE;
            }
            break;

	 case 5: 
            if (affected_by_spell(victim, SPELL_SENSE_LIFE))
            {
               affect_from_char(victim, SPELL_SENSE_LIFE);
               send_to_char("You feel less in touch with living things.\n\r", victim);
               done = TRUE;
            }
            break;

	 case 6: 
            if (affected_by_spell(victim, SPELL_INFRAVISION))
            {
               affect_from_char(victim, SPELL_INFRAVISION);
               send_to_char("Your sight grows dimmer.\n\r", victim);
               done = TRUE;
            }
            break;

	 case 7: 
            if (affected_by_spell(victim, SPELL_STRENGTH))
            {
               affect_from_char(victim, SPELL_STRENGTH);
               send_to_char("You don't feel so strong.\n\r", victim);
               done = TRUE;
            }
            break;

	 case 8: 
            if (affected_by_spell(victim, SPELL_DETECT_POISON))
            {
               affect_from_char(victim, SPELL_DETECT_POISON);
               send_to_char("You don't feel so sensitive to fumes.\n\r", victim);
               done = TRUE;
            }
            break;

	 case 9: 
            if (affected_by_spell(victim, SPELL_BLESS))
            {
               affect_from_char(victim, SPELL_BLESS);
               send_to_char("You don't feel so blessed.\n\r", victim);
               done = TRUE;
            }
            break;

	 case 10: 
            if (affected_by_spell(victim, SPELL_FLY))
            {
               affect_from_char(victim, SPELL_FLY);
               send_to_char("You don't feel lighter than air anymore.\n\r", victim);
               done = TRUE;
            }
 	    if (IS_AFFECTED(victim, AFF_FLYING))
            {
               REMOVE_BIT(victim->affected_by, AFF_FLYING);
               send_to_char("You don't feel lighter than air anymore.\n\r", victim);
               act("$n drops to the ground, no longer lighter than air.", victim, 0, 0, TO_ROOM, 0);
               done = TRUE;
            }
            break;
 
	 case 11: 
            if (affected_by_spell(victim, SPELL_DETECT_GOOD))
            {
               affect_from_char(victim, SPELL_DETECT_GOOD);
               send_to_char("You can't see the good in a person anymore.\n\r", victim);
               done = TRUE;
            }
            break;
      
	 case 12: 
            if (affected_by_spell(victim, SPELL_CAMOUFLAGE))
            {
               affect_from_char(victim, SPELL_CAMOUFLAGE);
               send_to_char("You don't seem to be camouflaged anymore.\n\r", victim);
               done = TRUE;
            }
            break;
      
         default: send_to_char("Illegal Value send to switch in dispel_minor, tell a god.\r\n", ch);
            done = TRUE;
            break;
      } // of switch
   } // of while

   if (IS_NPC(victim) && !victim->fighting) 
   {
      retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
      retval = SWAP_CH_VICT(retval);
      return retval;
   }
   return eSUCCESS;
}

// TODO - make this use skill
int spell_dispel_magic(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int rots = 0;
   int done = FALSE;
   int retval;

   if(!ch || !victim)
   {
      log("Null ch or victim sent to dispel_magic!", ANGEL, LOG_BUG);
      return eFAILURE;
   }

   set_cantquit(ch, victim);

   if(IS_SET(victim->affected_by2, AFF_GOLEM)) {
      send_to_char("The golem seems to shrug off your attempt!\r\n", ch);
      act("The golem seems to ignore $n!", ch, 0, 0, TO_ROOM, 0);
      return eFAILURE;
   }

   if(!IS_NPC(ch) && !IS_NPC(victim) && victim->fighting &&
       IS_NPC(victim->fighting) && 
      !IS_AFFECTED(victim->fighting, AFF_CHARM)) 
   {
      send_to_char("You misfire!\n\r", ch);
      victim = ch;
   }

   // If victim higher level, they get a save vs magic for no effect
   if((GET_LEVEL(victim) > GET_LEVEL(ch)) && 0 > saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC))
      return eFAILURE;

// Number of spells in the switch statement goes here
   while(!done && ((rots += 1) < 10))
   {
     switch(number(1, 11)) 
     {
        case 1: 
           if (affected_by_spell(victim, SPELL_SANCTUARY))
           {
              affect_from_char(victim, SPELL_SANCTUARY);
              send_to_char("You don't feel so invulnerable anymore.\n\r", victim);
              act("The white glow around $n's body fades.", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           if (IS_AFFECTED(victim, AFF_SANCTUARY))
           {
              REMOVE_BIT(victim->affected_by, AFF_SANCTUARY);
              send_to_char("You don't feel so invulnerable anymore.\n\r", victim);
              act("The white glow around $n's body fades.", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           break;

        case 2: 
           if (affected_by_spell(victim, SPELL_PROTECT_FROM_EVIL))
           {
              affect_from_char(victim, SPELL_PROTECT_FROM_EVIL);
              send_to_char("You feel less morally protected.\n\r", victim);
              done = TRUE;
           }
           break;

        case 3: 
           if (affected_by_spell(victim, SPELL_SLEEP))
           {
              affect_from_char(victim, SPELL_SLEEP);
              send_to_char("You don't feel so tired.\n\r", victim);
              done = TRUE;
           }
           break;

        case 4: 
           if (affected_by_spell(victim, SPELL_CHARM_PERSON) && !victim->fighting) 
           {
              stop_follower(victim, BROKE_CHARM);
              affect_from_char(victim, SPELL_CHARM_PERSON);
              send_to_char("You feel less enthused about your master.\n\r", victim);
              done = TRUE;
           }
           break;

	 case 5: 
           if (affected_by_spell(victim, SPELL_ARMOR))
           {
              affect_from_char(victim, SPELL_ARMOR);
              send_to_char("You don't feel so well protected.\n\r", victim);
              done = TRUE;
           }
           break;

	 case 6: 
           if (affected_by_spell(victim, SPELL_BLINDNESS))
           {
              affect_from_char(victim, SPELL_BLINDNESS);
              send_to_char("Your vision returns.\n\r", victim);
              done = TRUE;
           }
           break;

	 case 7: 
           if (affected_by_spell(victim, SPELL_POISON))
           {
              affect_from_char(victim, SPELL_POISON);
              done = TRUE;
           }
           break;

	 case 8: 
           if (affected_by_spell(victim, SPELL_FIRESHIELD))
           {
              affect_from_char(victim, SPELL_FIRESHIELD);
              send_to_char("Your flames seem to have been extinguished.\n\r", victim);
              act("The flames around $n's body fade away.", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           if (IS_AFFECTED(victim, AFF_FIRESHIELD))
           {
              REMOVE_BIT(victim->affected_by, AFF_FIRESHIELD);
              send_to_char("Your flames seem to have been extuinguised.\n\r", victim);
              act("The flames around $n's body fade away.", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
	   }
           break;

	 case 9: 
           if (affected_by_spell(victim, SPELL_PARALYZE))
           {
              affect_from_char(victim, SPELL_PARALYZE);
              send_to_char("You can move again.\n\r", victim);
              done = TRUE;
           }
	   if (IS_AFFECTED(victim, AFF_PARALYSIS))
           {
              REMOVE_BIT(victim->affected_by, AFF_PARALYSIS);
              send_to_char("You can move again.\n\r", victim);
              done = TRUE;
           }
           break;
      
	 case 10: 
           if (affected_by_spell(victim, SPELL_HASTE))
           {
              affect_from_char(victim, SPELL_HASTE);
              send_to_char("You don't feel so fast anymore.\n\r", victim);
              done = TRUE;
           }
           if (IS_AFFECTED(victim, AFF_HASTE))
           {
              REMOVE_BIT(victim->affected_by, AFF_HASTE);
              send_to_char("You don't feel so fast anymore.\n\r", victim);
              done = TRUE;
           }
           break;
        case 11:
           if (affected_by_spell(victim, SPELL_PROTECT_FROM_GOOD))
           {
              affect_from_char(victim, SPELL_PROTECT_FROM_GOOD);
              send_to_char("You feel less morally protected.\n\r", victim);
              done = TRUE;
           }
           break;

           default: send_to_char("Illegal Value sent to dispel_magic switch statement.  Tell a god.", ch);
              done = TRUE;
              break;
      } // end of switch
   } // end of while

   if (IS_NPC(victim) && !victim->fighting) 
   {
      retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
      retval = SWAP_CH_VICT(retval);
      return retval;
   }
   return eSUCCESS;
}


int spell_conjure_elemental(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj)
{
	 struct affected_type af;

	 /*
	  *   victim, in this case, is the elemental
	  *   object could be the sacrificial object
     */

	 if(!ch || !victim || !obj)
	 {
	   log("Null victim, object, or ch sent to conjure_elemental!", ANGEL, LOG_BUG);
	   return eFAILURE;
	 }

	 /*
	 ** objects:
    **     fire  : red stone
	 **     water : pale blue stone
	 **     earth : grey stone
	 **     air   : clear stone
    */

    act("$n gestures, and a cloud of smoke appears", ch, 0, 0, TO_ROOM, INVIS_NULL);
	 act("$n gestures, and a cloud of smoke appears", ch, 0, 0, TO_CHAR, INVIS_NULL);
	 act("$p explodes with a loud BANG!", ch, obj, 0, TO_ROOM, INVIS_NULL);
	 act("$p explodes with a loud BANG!", ch, obj, 0, TO_CHAR, INVIS_NULL);
         obj_from_char(obj);
	 extract_obj(obj);
    char_to_room(victim, ch->in_room);
    act("Out of the smoke, $N emerges", ch, 0, victim, TO_ROOM, NOTVICT|INVIS_NULL);

    /* charm them for a while */
    if (victim->master)
	stop_follower(victim, STOP_FOLLOW);

	 add_follower(victim, ch, 0);

	 af.type = SPELL_CHARM_PERSON;
	 af.duration = 48;
    af.modifier = 0;
	 af.location = 0;
	 af.bitvector = AFF_CHARM;

	 affect_to_char(victim, &af);

  return eSUCCESS;
}

int spell_cure_serious(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    int healpoints;

  if(!ch || !victim) {
    log("Null ch or victim sent to cure_serious!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Heal spells seem to be useless on the undead.\n\r", ch);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_CURE_SERIOUS, skill, SKILL_INCREASE_MEDIUM);

//  healpoints = dice(2, 8) +(skill/2);
  healpoints = dam_percent(skill,50);
  if ((healpoints + GET_HIT(victim)) > hit_limit(victim))
    GET_HIT(victim) = hit_limit(victim);
  else
    GET_HIT(victim) += healpoints;

  update_pos(victim);

  send_to_char("You feel better!\n\r", victim);
  return eSUCCESS;
}

int spell_cause_light(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;
   
   if (!ch || !victim) {
      log("Null ch or victim sent to cause_light", ANGEL, LOG_BUG);
      return eFAILURE;
   }

   skill_increase_check(ch, SPELL_CAUSE_LIGHT, skill, SKILL_INCREASE_MEDIUM);

   set_cantquit(ch, victim);
   dam = dice(1, 8) + (skill/3);
   return spell_damage(ch, victim, dam, TYPE_MAGIC, SPELL_CAUSE_LIGHT, 0);
}

int spell_cause_critical(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    int dam;

    if(!ch || !victim)
    {
      log("NULL ch or victim sent to cause_critical!", ANGEL, LOG_BUG);
      return eFAILURE;
    }

    skill_increase_check(ch, SPELL_CAUSE_CRITICAL, skill, SKILL_INCREASE_MEDIUM);
    set_cantquit(ch, victim);
    
    dam = dice(3,8)-6+skill/2;

    return spell_damage(ch, victim, dam, TYPE_MAGIC, SPELL_CAUSE_CRITICAL, 0);
}

int spell_cause_serious(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    int dam;
    if(!ch || !victim)
    {
      log("NULL ch or victim sent to cause_serious!", ANGEL, LOG_BUG);
      return eFAILURE;
    }

    skill_increase_check(ch, SPELL_CAUSE_SERIOUS, skill, SKILL_INCREASE_MEDIUM);

    set_cantquit(ch, victim);

    dam = dice(2, 8) + (skill/2);

   return spell_damage(ch, victim, dam,TYPE_MAGIC, SPELL_CAUSE_SERIOUS, 0);
}

int spell_flamestrike(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam, retval;

   if (!ch || !victim) {
      log("NULL ch or victim sent to flamestrike!", ANGEL, LOG_BUG);
      return eFAILURE;
   }

   skill_increase_check(ch, SPELL_FLAMESTRIKE, skill, SKILL_INCREASE_MEDIUM);
      
   set_cantquit (ch, victim);
//   dam = dice(skill/2, 10);
   dam = 350;
//   if (saves_spell(ch, victim, 0, SAVE_TYPE_FIRE) >= 0)
  //    dam >>= 1;

   retval = spell_damage(ch, victim, dam, TYPE_FIRE, SPELL_FLAMESTRIKE, 0);

   if(SOMEONE_DIED(retval))
      return retval;

   if(skill > 60) {
      send_to_char("The fires burn into your soul tearing away at your magic being.\r\n", victim);
      GET_MANA(victim) -= 15;
      if(GET_MANA(victim) < 0)
         GET_MANA(victim) = 0;
   }
   return retval;
}

/*
	magic user spells
*/

int spell_resist_cold(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   struct affected_type af;

   if (!affected_by_spell(victim, SPELL_RESIST_COLD)) {
      skill_increase_check(ch, SPELL_RESIST_COLD, skill, SKILL_INCREASE_MEDIUM);
      act("$n's skin turns blue momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
      act("Your skin turns blue momentarily.", victim, 0, 0, TO_CHAR, 0);

      af.type = SPELL_RESIST_COLD;
      af.duration = 1 + skill / 10;
      af.modifier = 10 + skill / 6;
      af.location = APPLY_SAVING_COLD;
      af.bitvector = 0;
      affect_to_char(victim, &af);
   }
   return eSUCCESS;
}



int spell_resist_fire(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   struct affected_type af;

   if (!affected_by_spell(victim, SPELL_RESIST_FIRE)) {
      skill_increase_check(ch, SPELL_RESIST_FIRE, skill, SKILL_INCREASE_MEDIUM);
      act("$n's skin turns red momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
      act("Your skin turns red momentarily.", victim, 0, 0, TO_CHAR, 0);
      af.type = SPELL_RESIST_FIRE;
      af.duration = 1 + skill / 10;
      af.modifier = 10 + skill / 6;
      af.location = APPLY_SAVING_FIRE;
      af.bitvector = 0;
      affect_to_char(victim, &af);
   }
   return eSUCCESS;
}

// TODO - make this use skill
int spell_staunchblood(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;

    if(!affected_by_spell(ch, SPELL_STAUNCHBLOOD)) {
       act("You feel supremely healthy.", ch, 0, 0, TO_CHAR, 0);

       af.type = SPELL_STAUNCHBLOOD;
       af.duration = level;
       af.modifier = 0;
       af.location = APPLY_NONE;
       af.bitvector = 0;
       affect_to_char(ch, &af);
       SET_BIT(ch->immune, ISR_POISON);
    }
  return eSUCCESS;
}

int spell_resist_energy(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   struct affected_type af;

   if(!affected_by_spell(victim, SPELL_RESIST_ENERGY)) {
      skill_increase_check(ch, SPELL_RESIST_ENERGY, skill, SKILL_INCREASE_MEDIUM);
      act("$n's skin turns gold momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
      act("Your skin turns gold momentarily.", victim, 0, 0, TO_CHAR, 0);
      af.type = SPELL_RESIST_ENERGY;
      af.duration = 1 + skill / 10;
      af.modifier = 10 + skill / 6;
      af.location = APPLY_SAVING_ENERGY;
      af.bitvector = 0;
      affect_to_char(victim, &af);
   }
   return eSUCCESS;
}

int spell_stone_skin(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;

    if(!ch)
    {
      log("NULL ch sent to cause_serious!", ANGEL, LOG_BUG);
        return eFAILURE;
    }


    if (!affected_by_spell(ch, SPELL_STONE_SKIN)) {
	skill_increase_check(ch, SPELL_STONE_SKIN, skill, SKILL_INCREASE_MEDIUM);
	act("$n's skin turns grey and granite-like.", ch, 0, 0, TO_ROOM, INVIS_NULL);
	act("Your skin turns to a stone-like substance.", ch, 0, 0, TO_CHAR, 0);

	af.type = SPELL_STONE_SKIN;
	af.duration = 1 + skill/6;
	af.modifier = -(10 + skill/3);
	af.location = APPLY_AC;
	af.bitvector = 0;
	affect_to_char(ch, &af);

        SET_BIT(ch->resist, ISR_PIERCE);
  }
  return eSUCCESS;
}

int spell_shield(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;

    if(!ch || !victim)
    {
      log("NULL ch or victim sent to shield!", ANGEL, LOG_BUG);
        return eFAILURE;
     }

  if(affected_by_spell(victim, SPELL_SHIELD))
    affect_from_char(victim, SPELL_SHIELD);

  skill_increase_check(ch, SPELL_SHIELD, skill, SKILL_INCREASE_MEDIUM);

  act("$N is surrounded by a strong force shield.", ch, 0, victim, TO_ROOM, INVIS_NULL|NOTVICT);
  if (ch != victim) {
     act("$N is surrounded by a strong force shield.", ch, 0, victim, TO_CHAR, 0);
     act("You are surrounded by a strong force shield.", ch, 0, victim, TO_VICT, 0);
  } else {
     act("You are surrounded by a strong force shield.", ch, 0, victim, TO_VICT, 0);
  }

  af.type = SPELL_SHIELD;
  af.duration = 13 + skill / 2;
  af.modifier = - ( 8 + skill / 30 );
  af.location = APPLY_AC;
  af.bitvector = 0;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

// TODO - make this use skill        
int spell_weaken(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
	 struct affected_type af;
	 float modifier;
	 int retval;

    if(!ch || !victim)
    {
      log("NULL ch or victim sent to weaken!", ANGEL, LOG_BUG);
        return eFAILURE;
    }


    set_cantquit (ch, victim);
    
	 if (!affected_by_spell(victim, SPELL_WEAKEN))
      {
	if (saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) < 0) {
	  modifier = level / 10.0f;
	  act("You feel weaker.", ch, 0, victim, TO_VICT, 0);
	  act("$n seems weaker.", victim, 0, 0, TO_ROOM, INVIS_NULL);

	  af.type = SPELL_WEAKEN;
	  af.duration = (int) level / 2;
	  af.modifier = (int)(0 - modifier);
	  af.location = APPLY_STR;
	  af.bitvector = 0;

	  affect_to_char(victim, &af);
	}
	else
	  {
		 act("$N isn't the wimp you made $m out to be!",
		ch, NULL, victim, TO_CHAR, 0);
	  }
      }

	 retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
         retval = SWAP_CH_VICT(retval);
         return retval;
}

// TODO - make this use skill        
int spell_mass_invis(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    CHAR_DATA *tmp_victim;
	 struct affected_type af;

	 if(!ch)
	 {
	   log("NULL ch sent to mass_invis!", ANGEL, LOG_BUG);
	     return eFAILURE;
         }


	 for (tmp_victim = world[ch->in_room].people; tmp_victim;
	 tmp_victim = tmp_victim->next_in_room) {
	if ((ch->in_room == tmp_victim->in_room))
		 if (!affected_by_spell(tmp_victim, SPELL_INVISIBLE)) {

		act("$n slowly fades out of existence.", tmp_victim, 0, 0,
                   TO_ROOM, INVIS_NULL);
		send_to_char("You vanish.\n\r", tmp_victim);

		af.type = SPELL_INVISIBLE;
		af.duration = 24;
		af.modifier = -40;
		af.location = APPLY_AC;
		af.bitvector = AFF_INVISIBLE;
		affect_to_char(tmp_victim, &af);
	  }
      }
  return eSUCCESS;
}

int spell_acid_blast(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;

   set_cantquit (ch, victim);
   skill_increase_check(ch, SPELL_ACID_BLAST, skill, SKILL_INCREASE_MEDIUM);
//   dam = dice(5 + skill/2, 12) + 5 + skill / 2;
   dam = 400;
   return spell_damage(ch, victim, dam,TYPE_ACID, SPELL_ACID_BLAST, 0);
}


int spell_hellstream(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;

   set_cantquit (ch, victim);
   skill_increase_check(ch, SPELL_HELLSTREAM, skill, SKILL_INCREASE_MEDIUM);
//   dam = (((skill/2 * 5)-200) + (GET_INT(ch) * 4)) * 6 + (skill / 2);
  dam = 800;
   return spell_damage(ch, victim, dam,TYPE_FIRE, SPELL_HELLSTREAM, 0);
}

void make_portal(CHAR_DATA * ch, CHAR_DATA * vict)
{
  struct obj_data *ch_portal, * vict_portal;
  extern struct obj_data *object_list;
  extern int top_of_world;
  char buf[250];
  int chance, destination;
  bool good_destination = false;

  ch_portal   = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
  vict_portal = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));

  clear_object(ch_portal);
  clear_object(vict_portal);

  ch_portal->item_number   = NOWHERE;
  vict_portal->item_number = NOWHERE;
  ch_portal->in_room       = NOWHERE;
  vict_portal->in_room     = NOWHERE;

  if (GET_CLASS(ch) == CLASS_CLERIC)
     sprintf(buf, "pcportal portal cleric %s", GET_NAME(ch));
  else
     sprintf(buf, "pcportal portal only %s %s", GET_NAME(ch), GET_NAME(vict));
  
  ch_portal->name = str_hsh(buf);
  vict_portal->name = str_hsh(buf);

  ch_portal->short_description   = str_hsh("an extradimensional portal");
  vict_portal->short_description = str_hsh("an extradimensional portal");
  
  ch_portal->description   = str_hsh("An extradimensional portal shimmers in "
                                     "the air before you.");
  vict_portal->description = str_hsh("An extradimensional portal shimmers in "
                                     "the air before you.");
  
  ch_portal->obj_flags.type_flag   = ITEM_PORTAL;
  vict_portal->obj_flags.type_flag = ITEM_PORTAL;

  ch_portal->obj_flags.timer = 2;
  vict_portal->obj_flags.timer = 2;
  
  chance = ((GET_LEVEL(vict)*2) - GET_LEVEL(ch));
  
  if (GET_LEVEL(vict) > GET_LEVEL(ch) && chance > number(0, 101)) {
     while(!good_destination) {
        destination = number(0, top_of_world);
        if(!world_array[destination] ||
           IS_SET(world[destination].room_flags, ARENA) ||
           IS_SET(world[destination].room_flags, IMP_ONLY) ||
           IS_SET(world[destination].room_flags, PRIVATE) ||
           IS_SET(world[destination].room_flags, CLAN_ROOM) ||
           IS_SET(world[destination].room_flags, NO_PORTAL) ||
           IS_SET(zone_table[world[destination].zone].zone_flags, ZONE_NO_TELEPORT)) 
        {
           good_destination = false;
        } else {
           good_destination = true;
        }
     } 
     ch_portal->obj_flags.value[0] = world[destination].number;
     }
  else {
     destination = vict->in_room;
     ch_portal->obj_flags.value[0] = world[destination].number;
     }

  vict_portal->obj_flags.value[0] = world[ch->in_room].number;

  ch_portal->next   = vict_portal;  
  vict_portal->next = object_list;
  object_list       = ch_portal;

  obj_to_room(ch_portal, ch->in_room);
  obj_to_room(vict_portal, destination);

  send_to_room("There is a violent flash of light as a portal shimmers "
               "into existence.\n\r", ch->in_room);

  sprintf(buf, "The space in front of you warps itself to %s's iron will.\n\r",
          GET_SHORT(ch));
  send_to_room(buf, vict->in_room);
  send_to_room("There is a violent flash of light as a portal shimmers "
               "into existence.\n\r", vict->in_room);

  return;
}

// TODO - make this use skill        
int spell_portal(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct obj_data *portal = 0;

  if(IS_SET(world[victim->in_room].room_flags, PRIVATE) ||
     IS_SET(world[victim->in_room].room_flags, NO_PORTAL) )  {
    send_to_char ("You can't seem to find a path.\n\r", ch);
    if(GET_CLASS(ch) == CLASS_CLERIC)
      GET_MANA(ch) += 100;
    else
      GET_MANA(ch) += 25;
    return eFAILURE;
  }

  if((!IS_NPC(victim)) && (GET_LEVEL(victim) >= IMMORTAL)) {
    send_to_char("Just who do you think you are?\n\r", ch);
    if(GET_CLASS(ch) == CLASS_CLERIC)
      GET_MANA(ch) += 100;
    else
      GET_MANA(ch) += 25;
    return eFAILURE;
  }
  if(IS_AFFECTED2(victim, AFF_SHADOWSLIP)) {
    send_to_char("You can't seem to find a definite path.\n\r", ch);
    if(GET_CLASS(ch) == CLASS_CLERIC)
      GET_MANA(ch) += 100;
    else
      GET_MANA(ch) += 25;
    return eFAILURE;
  }
  if(IS_SET(zone_table[world[victim->in_room].zone].zone_flags, ZONE_NO_TELEPORT))
  {
    send_to_char("A portal shimmers into view, then is torn away from the fabric of reality by a godly claw.\n\r",ch);
    act("A portal shimmers into view, then is torn away from the fabric of reality by a godly claw.",
        ch, 0, 0, TO_ROOM, 0);
    return eFAILURE;
  }

  for(portal = world[ch->in_room].contents; portal;
      portal = portal->next_content)
     if(portal->obj_flags.type_flag == ITEM_PORTAL)
       break;
       
  if(!portal)
    for(portal = world[victim->in_room].contents; portal;
        portal = portal->next_content)
       if(portal->obj_flags.type_flag == ITEM_PORTAL)
         break;

  if(portal || IS_ARENA(victim->in_room) || IS_ARENA(ch->in_room)) {
    send_to_char("A portal shimmers into view, then fades away again.\n\r",ch);
    act("A portal shimmers into view, then fades away again.",
        ch, 0, 0, TO_ROOM, 0);
    if(GET_CLASS(ch) == CLASS_CLERIC)
      GET_MANA(ch) += 140;
    else
      GET_MANA(ch) += 50;
    return eFAILURE;
  }

  make_portal(ch, victim);
  return eSUCCESS;
}

int cast_burning_hands( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
	 switch (type)
	 {
	 case SPELL_TYPE_SPELL:
	return spell_burning_hands(level, ch, victim, 0, skill);
	break;
	case SPELL_TYPE_SCROLL:
	if (victim)
	  return spell_burning_hands(level, ch, victim, 0, skill);
	else if (!tar_obj)
	  return spell_burning_hands(level, ch, ch, 0, skill);
	break;
	case SPELL_TYPE_WAND:
	if (victim)
	  return spell_burning_hands(level, ch, victim, 0, skill);
	break;
	 default :
	log("Serious screw-up in burning hands!", ANGEL, LOG_BUG);
	break;
	 }
  return eFAILURE;
}


int cast_call_lightning( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  extern struct weather_data weather_info;
  int retval;
  char_data * next_v;

  switch (type) {
  case SPELL_TYPE_SPELL:
    if (OUTSIDE(ch) && (weather_info.sky>=SKY_RAINING))
      return spell_call_lightning(level, ch, victim, 0, skill);
    else
      send_to_char("You fail to call upon the lightning from the sky!\n\r", ch);
    break;
  case SPELL_TYPE_POTION:
    if (OUTSIDE(ch) && (weather_info.sky>=SKY_RAINING))
      return spell_call_lightning(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (OUTSIDE(ch) && (weather_info.sky>=SKY_RAINING)) 
    {
      if(victim)
        return spell_call_lightning(level, ch, victim, 0, skill);
      else if(!tar_obj) 
        spell_call_lightning(level, ch, ch, 0, skill);
    }
    break;
  case SPELL_TYPE_STAFF:
    if (OUTSIDE(ch) && (weather_info.sky>=SKY_RAINING))
    {
      for (victim = world[ch->in_room].people ; victim ; victim = next_v )
      {
        next_v = victim->next_in_room;
   
        if ( !ARE_GROUPED(ch, victim) )
        {
          retval = spell_call_lightning(level, ch, victim, 0, skill);
          if(IS_SET(retval, eCH_DIED))
            return retval;
        }
      }
      return eSUCCESS;
    }
    break;
  default :
    log("Serious screw-up in call lightning!", ANGEL, LOG_BUG);
    break;
  }
  return eFAILURE;
}


int cast_chill_touch( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_chill_touch(level, ch, victim, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (victim)
		return spell_chill_touch(level, ch, victim, 0, skill);
	 else if (!tar_obj)
		return spell_chill_touch(level, ch, ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (victim)
		return spell_chill_touch(level, ch, victim, 0, skill);
	 break;
  default :
	 log("Serious screw-up in chill touch!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int cast_shocking_grasp( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_shocking_grasp(level, ch, victim, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (victim)
		return spell_shocking_grasp(level, ch, victim, 0, skill);
	 else if (!tar_obj)
		return spell_shocking_grasp(level, ch, ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (victim)
		return spell_shocking_grasp(level, ch, victim, 0, skill);
	 break;
  default :
		log("Serious screw-up in shocking grasp!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_colour_spray( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
   switch (type) {
      case SPELL_TYPE_SPELL:
         return spell_colour_spray(level, ch, victim, 0, skill);
         break;
      case SPELL_TYPE_SCROLL:
         if (victim)
            return spell_colour_spray(level, ch, victim, 0, skill);
         else if (!tar_obj)
            return spell_colour_spray(level, ch, ch, 0, skill);
         break;
      case SPELL_TYPE_WAND:
         if (victim)
            return spell_colour_spray(level, ch, victim, 0, skill);
         break;
      default :
         log("Serious screw-up in colour spray!", ANGEL, LOG_BUG);
         break;
      }
  return eFAILURE;
}

int cast_drown( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
   switch (type) {
      case SPELL_TYPE_SPELL:
         return spell_drown(level, ch, victim, 0, skill);
         break;
      case SPELL_TYPE_SCROLL:
         if (victim)
            return spell_drown(level, ch, victim, 0, skill);
         else if (!tar_obj)
            return spell_drown(level, ch, ch, 0, skill);
         break;
      case SPELL_TYPE_POTION:
	 return spell_drown(level,ch, ch, 0, skill);
      case SPELL_TYPE_WAND:
         if (victim)
            return spell_drown(level, ch, victim, 0, skill);
         break;
      default :
         log("Serious screw-up in drown!", ANGEL, LOG_BUG);
         break;
      }
  return eFAILURE;
}


int cast_earthquake( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	 case SPELL_TYPE_SCROLL:
	 case SPELL_TYPE_STAFF:
		 return spell_earthquake(level, ch, 0, 0, skill);
	  break;
	 default :
	 log("Serious screw-up in earthquake!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}



int cast_life_leech( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	 case SPELL_TYPE_SCROLL:
	 case SPELL_TYPE_STAFF:
		 return spell_life_leech(level, ch, 0, 0, skill);
	  break;
	 default :
	 log("Serious screw-up in cast_life_leach!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int cast_heroes_feast( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	 case SPELL_TYPE_STAFF:
	 case SPELL_TYPE_SCROLL:
         case SPELL_TYPE_POTION:
	 case SPELL_TYPE_WAND:
		 return spell_heroes_feast(level, ch, 0, 0, skill);
	  break;
	 default :
			log("Serious screw-up in cast_heroes_feast!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}
int cast_heal_spray( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	 case SPELL_TYPE_SCROLL:
	 case SPELL_TYPE_STAFF:
		 return spell_heal_spray(level, ch, 0, 0, skill);
	  break;
	 default :
			log("Serious screw-up in cast_heal_spray!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_group_sanc( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	 case SPELL_TYPE_SCROLL:
	 case SPELL_TYPE_STAFF:
		 return spell_group_sanc(level, ch, 0, 0, skill);
	  break;
	 default :
			log("Serious screw-up in cast_group_sanc!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_group_recall( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	 case SPELL_TYPE_SCROLL:
	 case SPELL_TYPE_STAFF:
		 return spell_group_recall(level, ch, 0, 0, skill);
	  break;
	 default :
			log("Serious screw-up in cast_group_recall!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_group_fly( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	 case SPELL_TYPE_SCROLL:
	 case SPELL_TYPE_STAFF:
		 return spell_group_fly(level, ch, 0, 0, skill);
	  break;
	 default :
			log("Serious screw-up in cast_group_fly!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_firestorm( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	 case SPELL_TYPE_SCROLL:
	 case SPELL_TYPE_STAFF:
		 return spell_firestorm(level, ch, 0, 0, skill);
	  break;
	 default :
			log("Serious screw-up in cast_firestorm!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_solar_gate( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	 case SPELL_TYPE_SCROLL:
	 case SPELL_TYPE_STAFF:
		 return spell_solar_gate(level, ch, 0, 0, skill);
	  break;
	 default :
			log("Serious screw-up in cast_firestorm!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_energy_drain( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  char_data * next_v;
  int retval;

  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_energy_drain(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
	 return spell_energy_drain(level, ch, ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if(victim)
		return spell_energy_drain(level, ch, victim, 0, skill);
	 else if(!tar_obj)
		 return spell_energy_drain(level, ch, ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if(victim)
		return spell_energy_drain(level, ch, victim, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (victim = world[ch->in_room].people ; victim ; victim = next_v )
    {
      next_v = victim->next_in_room;
   
      if ( !ARE_GROUPED(ch, victim) )
      {
        retval = spell_energy_drain(level, ch, victim, 0, skill);
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default :
	 log("Serious screw-up in energy drain!", ANGEL, LOG_BUG);
	 break;
	 }
   return eFAILURE;
}

int cast_souldrain( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  switch (type) {
  case SPELL_TYPE_SPELL:
	    return spell_souldrain(level, ch, victim, 0, skill);
	    break;
  case SPELL_TYPE_POTION:
	    return spell_souldrain(level, ch, ch, 0, skill);
	    break;
  case SPELL_TYPE_SCROLL:
	    if(victim)
		return spell_souldrain(level, ch, victim, 0, skill);
	    else if(!tar_obj)
		 return spell_souldrain(level, ch, ch, 0, skill);
	    break;
  case SPELL_TYPE_WAND:
	    if(victim)
		return spell_souldrain(level, ch, victim, 0, skill);
	    break;
  case SPELL_TYPE_STAFF:
    for (victim = world[ch->in_room].people ; victim ; victim = next_v )
    {
      next_v = victim->next_in_room;
  
      if ( !ARE_GROUPED(ch, victim) )
      {
        retval = spell_souldrain(level, ch, victim, 0, skill);
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default :
	    log("Serious screw-up in souldrain!", ANGEL, LOG_BUG);
	    break;
  }
  return eFAILURE;
}

int cast_vampiric_touch( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	  return spell_vampiric_touch(level, ch, victim, 0, skill);
	break;
	 case SPELL_TYPE_SCROLL:
	 if(victim)
		return spell_vampiric_touch(level, ch, victim, 0, skill);
	 else if(!tar_obj)
		 return spell_vampiric_touch(level, ch, ch, 0, skill);
	 break;
	 case SPELL_TYPE_WAND:
	 if(victim)
		return spell_vampiric_touch(level, ch, victim, 0, skill);
	 break;
	 default :
	 log("Serious screw-up in vampiric touch!", ANGEL, LOG_BUG);
	 break;

	 }
  return eFAILURE;
}


int cast_meteor_swarm( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
	  return spell_meteor_swarm(level, ch, victim, 0, skill);
	break;
	 case SPELL_TYPE_SCROLL:
	 if(victim)
		return spell_meteor_swarm(level, ch, victim, 0, skill);
	 else if(!tar_obj)
		 return spell_meteor_swarm(level, ch, ch, 0, skill);
	 break;
	 case SPELL_TYPE_WAND:
	 if(victim)
		return spell_meteor_swarm(level, ch, victim, 0, skill);
	 break;
	 default :
	 log("Serious screw-up in meteor swarm!", ANGEL, LOG_BUG);
	 break;

	 }
  return eFAILURE;
}

int cast_fireball( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
   switch (type) {
      case SPELL_TYPE_SPELL:
         return spell_fireball(level, ch, victim, 0, skill);
         break;
      case SPELL_TYPE_SCROLL:
         if (victim)
            return spell_fireball(level, ch, victim, 0, skill);
         else if(!tar_obj)
            return spell_fireball(level, ch, ch, 0, skill);
         break;
      case SPELL_TYPE_WAND:
         if (victim)
            return spell_fireball(level, ch, victim, 0, skill);
         break;
      default :
         log("Serious screw-up in fireball!", ANGEL, LOG_BUG);
         break;
      }
  return eFAILURE;
}

int cast_sparks( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
   switch (type) {
      case SPELL_TYPE_SPELL:
         return spell_sparks(level, ch, victim, 0, skill);
         break;
      case SPELL_TYPE_SCROLL:
         if (victim)
            return spell_sparks(level, ch, victim, 0, skill);
         else if(!tar_obj)
            return spell_sparks(level, ch, ch, 0, skill);
         break;
      case SPELL_TYPE_WAND:
         if (victim)
            return spell_sparks(level, ch, victim, 0, skill);
         break;
      default :
         log("Serious screw-up in sparks!", ANGEL, LOG_BUG);
         break;
      }
  return eFAILURE;
}

int cast_howl( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
   switch (type) {
      case SPELL_TYPE_SPELL:
         return spell_howl(level, ch, victim, 0, skill);
         break;
      case SPELL_TYPE_SCROLL:
         if (victim)
            return spell_howl(level, ch, victim, 0, skill);
         else if(!tar_obj)
            return spell_howl(level, ch, ch, 0, skill);
         break;
      case SPELL_TYPE_WAND:
         if (victim)
            return spell_howl(level, ch, victim, 0, skill);
         break;
      default :
         log("Serious screw-up in howl!", ANGEL, LOG_BUG);
         break;
      }
  return eFAILURE;
}


int cast_harm( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_harm(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_harm(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (victim = world[ch->in_room].people ; victim ; victim = next_v )
    {
      next_v = victim->next_in_room;

      if ( !ARE_GROUPED(ch, victim) )
      {
        retval = spell_harm(level, ch, victim, 0, skill);
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default :
    log("Serious screw-up in harm!", ANGEL, LOG_BUG);
    break;
  }
  return eFAILURE;
}


int cast_power_harm( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  switch (type) {
  case SPELL_TYPE_SPELL:
    if(victim)
      return spell_power_harm(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_power_harm(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if(victim)
      return spell_power_harm(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (victim = world[ch->in_room].people ; victim ; victim = next_v )
    {
      next_v = victim->next_in_room;

      if ( !ARE_GROUPED(ch, victim) )
      {
        retval = spell_power_harm(level, ch, victim, 0, skill);
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default :
    log("Serious screw-up in power_harm!", ANGEL, LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_lightning_bolt( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_lightning_bolt(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if(victim)
      return spell_lightning_bolt(level, ch, victim, 0, skill);
    else if(!tar_obj)
      return spell_lightning_bolt(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if(victim)
     return spell_lightning_bolt(level, ch, victim, 0, skill);
    break;
  default :
    log("Serious screw-up in lightning bolt!", ANGEL, LOG_BUG);
    break;
  }
  return eFAILURE;
}


int cast_magic_missile( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		return spell_magic_missile(level, ch, victim, 0, skill);
		break;
	 case SPELL_TYPE_SCROLL:
	 if(victim)
		return spell_magic_missile(level, ch, victim, 0, skill);
	 else if(!tar_obj)
		 return spell_magic_missile(level, ch, ch, 0, skill);
	 break;
	 case SPELL_TYPE_WAND:
	 if(victim)
		return spell_magic_missile(level, ch, victim, 0, skill);
	 break;
	 default :
	 log("Serious screw-up in magic missile!", ANGEL, LOG_BUG);
	 break;

  }
  return eFAILURE;
}

int cast_armor( byte level, CHAR_DATA *ch, char *arg, int type,
	 CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	case SPELL_TYPE_SPELL:
		 if (ch != tar_ch)
			act("$N is protected by mystical armour.", ch, 0, tar_ch, TO_CHAR,0);
		 return spell_armor(level,ch,tar_ch,0, skill);
		 break;
	case SPELL_TYPE_POTION:
		 return spell_armor(level,ch,ch,0, skill);
		 break;
	case SPELL_TYPE_SCROLL:
		 if (tar_obj) return eFAILURE;
		 if (!tar_ch) tar_ch = ch;
		 return spell_armor(level,ch,ch,0, skill);
		 break;
	case SPELL_TYPE_WAND:
		 if (tar_obj) return eFAILURE;
		 if (!tar_ch) tar_ch = ch;
		 if ( affected_by_spell(tar_ch, SPELL_ARMOR) )
		return eFAILURE;
		 return spell_armor(level,ch,tar_ch,0, skill);
		 break;
		default :
	 log("Serious screw-up in armor!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_teleport( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_POTION:
  case SPELL_TYPE_SPELL:
	 if (!tar_ch)
		tar_ch = ch;
	 return spell_teleport(level, ch, tar_ch, 0, skill);
	 break;

  case SPELL_TYPE_WAND:
	 if(!tar_ch) tar_ch = ch;
	 return spell_teleport(level, ch, tar_ch, 0, skill);
	 break;

  case SPELL_TYPE_STAFF:
	for (tar_ch = world[ch->in_room].people ;
		tar_ch ; tar_ch = tar_ch->next_in_room)
	 if ( IS_NPC(tar_ch) )
		spell_teleport(level, ch, tar_ch, 0, skill);
	return eSUCCESS;
		break;

  default :
		log("Serious screw-up in teleport!", ANGEL, LOG_BUG);
		break;
  }
  return eFAILURE;
}


int cast_bless( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{

  switch (type) {
  case SPELL_TYPE_SPELL:
	 if (tar_obj) {        /* It's an object */
		if ( IS_SET(tar_obj->obj_flags.extra_flags, ITEM_BLESS) ) {
	 send_to_char("Nothing seems to happen.\n\r", ch);
	 return eFAILURE;
		}
		return spell_bless(level,ch,0,tar_obj, skill);

	 } else {              /* Then it is a PC | NPC */

		if(GET_POS(tar_ch) == POSITION_FIGHTING) {
	 send_to_char("Nothing seems to happen.\n\r", ch);
	 return eFAILURE;
		}
		return spell_bless(level,ch,tar_ch,0, skill);
	 }
	 break;
  case SPELL_TYPE_POTION:
	 if(GET_POS(ch) == POSITION_FIGHTING) {
	 send_to_char("Nothing seems to happen.\n\r", ch);
	 return eFAILURE;
		}
	 return spell_bless(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) {        /* It's an object */
		if ( IS_SET(tar_obj->obj_flags.extra_flags, ITEM_BLESS) )
	 {
	 send_to_char("Nothing seems to happen.\n\r", ch);
	 return eFAILURE;
		}
		return spell_bless(level,ch,0,tar_obj, skill);

	 } else {              /* Then it is a PC | NPC */

		if (!tar_ch) tar_ch = ch;

		if(GET_POS(tar_ch) == POSITION_FIGHTING)
	 {
	 send_to_char("Nothing seems to happen.\n\r", ch);
	 return eFAILURE;
		}
		return spell_bless(level,ch,tar_ch,0, skill);
	 }
	 break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) {        /* It's an object */
		if ( IS_SET(tar_obj->obj_flags.extra_flags, ITEM_BLESS) )
	 {
	 send_to_char("Nothing seems to happen.\n\r", ch);
	 return eFAILURE;
		}
		return spell_bless(level,ch,0,tar_obj, skill);

	 } else {              /* Then it is a PC | NPC */

		if(GET_POS(tar_ch) == POSITION_FIGHTING)
	 {
	 send_to_char("Nothing seems to happen.\n\r", ch);
	 return eFAILURE;
		}
		return spell_bless(level,ch,tar_ch,0, skill);
	 }
	 break;
	 default :
		log("Serious screw-up in bless!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}



int cast_paralyze( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
	int retval;

	 if (IS_SET(world[ch->in_room].room_flags,SAFE)){
		send_to_char("You can not paralyze anyone in a safe area!\n\r", ch);
		return eFAILURE;
	 }
	 switch (type) {
	 case SPELL_TYPE_SPELL:
		if ( IS_AFFECTED(tar_ch, AFF_PARALYSIS) ){
	send_to_char("Nothing seems to happen.\n\r", ch);
	return eFAILURE;
		}
		return spell_paralyze(level,ch,tar_ch,0, skill);
		break;
	 case SPELL_TYPE_POTION:
		if ( IS_AFFECTED(ch, AFF_PARALYSIS) )
	return eFAILURE;
		return spell_paralyze(level,ch,ch,0, skill);
		break;
	 case SPELL_TYPE_SCROLL:
		if (tar_obj) return eFAILURE;
		if (!tar_ch) tar_ch = ch;
		if ( IS_AFFECTED(tar_ch, AFF_PARALYSIS) )
	return eFAILURE;
		return spell_paralyze(level,ch,tar_ch,0, skill);
		break;
	 case SPELL_TYPE_WAND:
		if (tar_obj) return eFAILURE;
		if (!tar_ch) tar_ch = ch;
		if ( IS_AFFECTED(tar_ch, AFF_PARALYSIS) )
	return eFAILURE;
		return spell_paralyze(level,ch,tar_ch,0, skill);
		break;
	 case SPELL_TYPE_STAFF:
		for (tar_ch = world[ch->in_room].people ;
		tar_ch ; tar_ch = tar_ch->next_in_room)
		 if ( IS_NPC(tar_ch) )
	  if (!(IS_AFFECTED(tar_ch, AFF_PARALYSIS)))
          {
	 	retval = spell_paralyze(level,ch,tar_ch,0, skill);
		if(IS_SET(retval, eCH_DIED))
			return retval;
	  }
		return eSUCCESS;
		break;
		default :
	log("Serious screw-up in paralyze!", ANGEL, LOG_BUG);
		break;
	 }
  return eFAILURE;
}

int cast_blindness( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  if (IS_SET(world[ch->in_room].room_flags,SAFE)){
     send_to_char("You can not blind anyone in a safe area!\n\r", ch);
     return eFAILURE;
  }

  switch (type) {
  case SPELL_TYPE_SPELL:
    if ( IS_AFFECTED(tar_ch, AFF_BLIND) ){
      send_to_char("Nothing seems to happen.\n\r", ch);
      return eFAILURE;
    }
    return spell_blindness(level,ch,tar_ch,0, skill);
    break;
  case SPELL_TYPE_POTION:
    if ( IS_AFFECTED(ch, AFF_BLIND) )
      return eFAILURE;
    return spell_blindness(level,ch,ch,0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)     return eFAILURE;
    if (!tar_ch)     tar_ch = ch;
    if ( IS_AFFECTED(tar_ch, AFF_BLIND) )
      return eFAILURE;
    return spell_blindness(level,ch,tar_ch,0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj) return eFAILURE;
    if (!tar_ch) tar_ch = ch;
    if ( IS_AFFECTED(tar_ch, AFF_BLIND) )
      return eFAILURE;
    return spell_blindness(level,ch,tar_ch,0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {
      next_v = tar_ch->next_in_room;

      if ( !IS_AFFECTED(tar_ch, AFF_BLIND) )
      {
        retval = spell_blindness(level, ch, tar_ch, 0, skill);
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default :
    log("Serious screw-up in blindness!", ANGEL, LOG_BUG);
    break;
  }
  return eFAILURE;
}


int cast_control_weather( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
	 char buffer[MAX_STRING_LENGTH];
	 extern struct weather_data weather_info;

  switch (type) {
	 case SPELL_TYPE_SPELL:

		 one_argument(arg,buffer);

		 if (str_cmp("better",buffer) && str_cmp("worse",buffer))
		 {
		send_to_char("Do you want it to get better or worse?\n\r",ch);
		return eFAILURE;
		 }

		 if(!str_cmp("better",buffer))
		weather_info.change+=(dice(((level)/3),9));
		 else
		weather_info.change-=(dice(((level)/3),9));
		 break;
		default :
	 log("Serious screw-up in control weather!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}



int cast_create_food( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{

  switch (type) {
	 case SPELL_TYPE_SPELL:
		 act("$n magically creates a mushroom.",ch, 0, 0, TO_ROOM,
		   0);
	 return spell_create_food(level,ch,0,0, skill);
		 break;
         case SPELL_TYPE_WAND:
         if(tar_obj) return eFAILURE;
         if(tar_ch) return eFAILURE;
         return spell_create_food(level,ch,0,0, skill);
                 break;
         case SPELL_TYPE_STAFF:
         if(tar_obj) return eFAILURE;
         if(tar_ch) return eFAILURE;
         return spell_create_food(level,ch,0,0, skill);
                 break;
	 case SPELL_TYPE_SCROLL:
	 if(tar_obj) return eFAILURE;
	 if(tar_ch) return eFAILURE;
	 return spell_create_food(level,ch,0,0, skill);
		 break;
	 default :
	 log("Serious screw-up in create food!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}



int cast_create_water( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 if (tar_obj->obj_flags.type_flag != ITEM_DRINKCON) {
		send_to_char("It is unable to hold water.\n\r", ch);
		return eFAILURE;
	 }
	 return spell_create_water(level,ch,0,tar_obj, skill);
	 break;
         case SPELL_TYPE_WAND:
         if(!tar_obj) return eFAILURE;
//         if(tar_ch) return eFAILURE;
         return spell_create_food(level,ch,0,tar_obj, skill);
                 break;
         case SPELL_TYPE_SCROLL:
         if(!tar_obj) return eFAILURE;
//         if(tar_ch) return eFAILURE;
         return spell_create_water(level,ch,0,tar_obj, skill);
                 break;

	 default :
		log("Serious screw-up in create water!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}



int cast_remove_paralysis( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  int retval;

  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_remove_paralysis(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_remove_paralysis(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_WAND:
         if(tar_obj) return eFAILURE;
         return spell_remove_paralysis(level, ch, tar_ch, 0, skill);

  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_remove_paralysis(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = tar_ch->next_in_room)
	{
	 retval = spell_remove_paralysis(level,ch,tar_ch,0, skill);
	 if(IS_SET(retval, eCH_DIED))
		return retval;
	}
	 break;
	 default :
		log("Serious screw-up in remove paralysis!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_cure_blind( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_cure_blind(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_cure_blind(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_cure_blind(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)

	 spell_cure_blind(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw up in cure blind!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}



int cast_cure_critic( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_cure_critic(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_cure_critic(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_WAND:
         if(!tar_ch) return eFAILURE;
         return spell_cure_critic(level, ch, tar_ch, 0, skill);
         break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_cure_critic(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)
		spell_cure_critic(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw-up in cure critic!", ANGEL, LOG_BUG);
	 break;

  }
  return eFAILURE;
}



int cast_cure_light( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_cure_light(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_WAND:
    if(tar_ch) return eFAILURE;
      return spell_cure_light(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
	 return spell_cure_light(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_cure_light(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)
		spell_cure_light(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw-up in cure light!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_curse( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  int retval;

  if (IS_SET(world[ch->in_room].room_flags, SAFE)){
	 send_to_char("You cannot curse someone in a safe area!\n\r", ch);
	 return eFAILURE;
  }

  switch (type) {
	 case SPELL_TYPE_SPELL:
		 if (tar_obj)   /* It is an object */
		return spell_curse(level,ch,0,tar_obj, skill);
		 else {              /* Then it is a PC | NPC */
		return spell_curse(level,ch,tar_ch,0, skill);
		 }
		 break;
	 case SPELL_TYPE_POTION:
		 return spell_curse(level,ch,ch,0, skill);
		 break;
	 case SPELL_TYPE_SCROLL:
		 if (tar_obj)   /* It is an object */
		return spell_curse(level,ch,0,tar_obj, skill);
		 else {              /* Then it is a PC | NPC */
		if (!tar_ch) tar_ch = ch;
		return spell_curse(level,ch,tar_ch,0, skill);
		 }
		 break;
	 case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
			tar_ch ; tar_ch = tar_ch->next_in_room)
		 if ( IS_NPC(tar_ch) )
		{
			retval = spell_curse(level,ch,tar_ch,0, skill);
			if(IS_SET(retval, eCH_DIED))
				return retval;
		}
	 break;
	 default :
	 log("Serious screw-up in curse!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int cast_detect_evil( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_detect_evil(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_detect_evil(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_detect_evil(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)

		spell_detect_evil(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw-up in detect evil!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}



int cast_true_sight( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_true_sight(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_true_sight(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_true_sight(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)
		spell_true_sight(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw-up in true sight!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_detect_good( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_detect_good(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_detect_good(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_detect_good(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)

		spell_detect_good(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw-up in detect good!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_detect_invisibility( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_detect_invisibility(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_detect_invisibility(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (!tar_ch) return eFAILURE;
	 return spell_detect_invisibility(level,ch,tar_ch,0,skill);
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_detect_invisibility(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)
		spell_detect_invisibility(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw-up in detect invisibility!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}



int cast_detect_magic( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_detect_magic(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_detect_magic(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_detect_magic(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)
		spell_detect_magic(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw-up in detect magic!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}



int cast_haste( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 if ( affected_by_spell(tar_ch, SPELL_HASTE) ){
		send_to_char("Nothing seems to happen.\n\r", tar_ch);
		return eFAILURE;
	 }
	 return spell_haste(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_haste(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_haste(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)

	 if (!(IS_AFFECTED(tar_ch, SPELL_HASTE)))
		spell_haste(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw-up in haste!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_detect_poison( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_detect_poison(level, ch, tar_ch,tar_obj, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_detect_poison(level, ch, ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) {
		return spell_detect_poison(level, ch, 0, tar_obj, skill);
	 }
	 if (!tar_ch) tar_ch = ch;
	 return spell_detect_poison(level, ch, tar_ch, 0, skill);
	 break;
	 default :
		log("Serious screw-up in detect poison!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}



int cast_dispel_evil( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_dispel_evil(level, ch, tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_dispel_evil(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_dispel_evil(level, ch, tar_ch,0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) return eFAILURE;
	 return spell_dispel_evil(level, ch, tar_ch,0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {
      next_v = tar_ch->next_in_room;
   
      if ( !ARE_GROUPED(ch, tar_ch) )
      {
        retval = spell_dispel_evil(level, ch, tar_ch, 0, skill);
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;

  default :
    log("Serious screw-up in dispel evil!", ANGEL, LOG_BUG);
    break;
  }
  return eFAILURE;
}




int cast_dispel_good( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_dispel_good(level, ch, tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_dispel_good(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_dispel_good(level, ch, tar_ch,0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) return eFAILURE;
	 return spell_dispel_good(level, ch, tar_ch,0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {
      next_v = tar_ch->next_in_room;
  
      if ( !ARE_GROUPED(ch, tar_ch) )
      {
        retval = spell_dispel_good(level, ch, tar_ch, 0, skill);
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default :
		log("Serious screw-up in dispel good!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_enchant_armor( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_enchant_armor(level, ch, 0,tar_obj, skill);
		 break;

	 case SPELL_TYPE_SCROLL:
		 if(!tar_obj) return eFAILURE;
		 return spell_enchant_armor(level, ch, 0,tar_obj, skill);
		 break;
	 default :
		log("Serious screw-up in enchant weapon!", ANGEL, LOG_BUG);
		break;
	 }
  return eFAILURE;
}




int cast_enchant_weapon( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_enchant_weapon(level, ch, 0,tar_obj, skill);
		 break;

	 case SPELL_TYPE_SCROLL:
		 if(!tar_obj) return eFAILURE;
		 return spell_enchant_weapon(level, ch, 0,tar_obj, skill);
		 break;
	 default :
		log("Serious screw-up in enchant weapon!", ANGEL, LOG_BUG);
		break;
	 }
  return eFAILURE;
}


int cast_mana( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_mana(level, ch, tar_ch, 0, skill);
		 break;
	 case SPELL_TYPE_SCROLL:
	   if (!tar_ch) tar_ch = ch;
	return spell_mana(level,ch,tar_ch,0,skill);
	break;
	 case SPELL_TYPE_POTION:
	 return spell_mana(level, ch, ch, 0, skill);
	 break;
	 case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
			tar_ch ; tar_ch = tar_ch->next_in_room)
		 spell_mana(level,ch,tar_ch,0, skill);
	 break;
	 default :
	 log("Serious screw-up in mana!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int cast_heal( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 act("$n heals $N.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
		 act("You heal $N.", ch, 0, tar_ch, TO_CHAR, 0);
		 return spell_heal(level, ch, tar_ch, 0, skill);
		 break;
	case SPELL_TYPE_WAND:
	   if (!tar_ch) return eFAILURE;
	  return spell_heal(level,ch,tar_ch,0,skill);
	break;
	 case SPELL_TYPE_POTION:
	 return spell_heal(level, ch, ch, 0, skill);
	 break;
	 case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
			tar_ch ; tar_ch = tar_ch->next_in_room)
		 spell_heal(level,ch,tar_ch,0, skill);
	 break;
	 default :
	 log("Serious screw-up in heal!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}



int cast_power_heal( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 act("$n power heals $N.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
		 act("You power heal $N.", ch, 0, tar_ch, TO_CHAR, 0);
		 return spell_power_heal(level, ch, tar_ch, 0, skill);
		 break;
	 case SPELL_TYPE_POTION:
	 return spell_power_heal(level, ch, ch, 0, skill);
	 break;
	 case SPELL_TYPE_WAND:
	   if (!tar_ch) return eFAILURE;
		return spell_power_heal(level,ch,tar_ch,0,skill);
		break;		
	 case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
			tar_ch ; tar_ch = tar_ch->next_in_room)
		 spell_power_heal(level, ch, tar_ch, 0, skill);
	 break;
	 default :
	 log("Serious screw-up in power_heal!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int cast_full_heal( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 act("$n FULL heals $N.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
		 act("You FULL heal $N.", ch, 0, tar_ch, TO_CHAR, 0);
		 return spell_full_heal(level, ch, tar_ch, 0, skill);
		 break;
	 case SPELL_TYPE_POTION:
	 return spell_full_heal(level, ch, ch, 0, skill);
	 break;
	 case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
			tar_ch ; tar_ch = tar_ch->next_in_room)
		 spell_full_heal(level,ch,tar_ch,0, skill);
	 break;
	 default :
	 log("Serious screw-up in full_heal!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}



int cast_invisibility( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 if (tar_obj) {
		if ( IS_SET(tar_obj->obj_flags.extra_flags, ITEM_INVISIBLE) )
	 send_to_char("Nothing new seems to happen.\n\r", ch);
		else
	 return spell_invisibility(level, ch, 0, tar_obj, skill);
	 } else { /* tar_ch */
	 return spell_invisibility(level, ch, tar_ch, 0, skill);
	 }
	 break;
  case SPELL_TYPE_POTION:
		return spell_invisibility(level, ch, ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) {
		if (!(IS_SET(tar_obj->obj_flags.extra_flags, ITEM_INVISIBLE)) )
	 return spell_invisibility(level, ch, 0, tar_obj, skill);
	 } else { /* tar_ch */
		if (!tar_ch) tar_ch = ch;

	 return spell_invisibility(level, ch, tar_ch, 0, skill);
	 }
	 break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) {
		if (!(IS_SET(tar_obj->obj_flags.extra_flags, ITEM_INVISIBLE)) )
	 return spell_invisibility(level, ch, 0, tar_obj, skill);
	 } else { /* tar_ch */
	 return spell_invisibility(level, ch, tar_ch, 0, skill);
	 }
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)
		spell_invisibility(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw-up in invisibility!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}




int cast_locate_object( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_locate_object(level, ch, 0, tar_obj, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_ch) return eFAILURE;
	 return spell_locate_object(level, ch, 0, tar_obj, skill);
	 break;
	 default :
		log("Serious screw-up in locate object!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_poison( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  if (IS_SET(world[ch->in_room].room_flags, SAFE)){
	 send_to_char("You can not poison someone in a safe area!\n\r",ch);
	 return eFAILURE;
  }
  switch (type) {
  case SPELL_TYPE_SPELL:
		 return spell_poison(level, ch, tar_ch, tar_obj, skill);
		 break;
  case SPELL_TYPE_POTION:
		 return spell_poison(level, ch, ch, 0, skill);
		 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_poison(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;

  default :
	 log("Serious screw-up in poison!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_protection_from_evil( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      if(IS_EVIL(ch) && GET_LEVEL(ch) < ARCHANGEL) {
         send_to_char("You are too evil to invoke the protection of your god.\n\r", ch);
         return eFAILURE;
      } else if (ch != tar_ch && GET_CLASS(ch) != CLASS_CLERIC) {
         /* only let clerics cast on others */
         send_to_char("You can only cast this spell on yourself.\n\r", ch);
         return eFAILURE;
      } else
         return spell_protection_from_evil(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_protection_from_evil(level, ch, ch, 0, skill);
      break;
    case SPELL_TYPE_WAND:
      if(tar_obj) return eFAILURE;
      return spell_protection_from_evil(level, ch, tar_ch, 0, skill);
    case SPELL_TYPE_SCROLL:
      if(tar_obj) return eFAILURE;
      if(!tar_ch) tar_ch = ch;
        return spell_protection_from_evil(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_STAFF:
      for (tar_ch = world[ch->in_room].people; tar_ch ; tar_ch = tar_ch->next_in_room)
        spell_protection_from_evil(level,ch,tar_ch,0, skill);
      break;
    default :
      log("Serious screw-up in protection from evil!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}


int cast_protection_from_good( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      if(IS_GOOD(ch) && GET_LEVEL(ch) < ARCHANGEL) {
         send_to_char("Your goodness finds disfavor amongst the forces of darkness.\n\r", ch);
         return eFAILURE;
      } else if (ch != tar_ch && GET_CLASS(ch) != CLASS_CLERIC) {
         /* only let clerics cast on others */
         send_to_char("You can only cast this spell on yourself.\n\r", ch);
         return eFAILURE;
      } else
         return spell_protection_from_good(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_protection_from_good(level, ch, ch, 0, skill);
      break;
    case SPELL_TYPE_WAND:
      if(tar_obj) return eFAILURE;
      return spell_protection_from_good(level, ch, tar_ch, 0, skill);
    case SPELL_TYPE_SCROLL:
      if(tar_obj) return eFAILURE;
      if(!tar_ch) tar_ch = ch;
        return spell_protection_from_good(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_STAFF:
      for (tar_ch = world[ch->in_room].people; tar_ch ; tar_ch = tar_ch->next_in_room)
        spell_protection_from_good(level,ch,tar_ch,0, skill);
      break;
    default :
      log("Serious screw-up in protection from good!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}

int cast_remove_curse( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_remove_curse(level, ch, tar_ch, tar_obj, skill);
		 break;
	 case SPELL_TYPE_POTION:
	 return spell_remove_curse(level, ch, ch, 0, skill);
	 break;
	 case SPELL_TYPE_SCROLL:
	 if(tar_obj) {
		return spell_remove_curse(level, ch, 0, tar_obj, skill);
		 }
	 if(!tar_ch) tar_ch = ch;
		 return spell_remove_curse(level, ch, tar_ch, 0, skill);
		 break;
	 case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
			tar_ch ; tar_ch = tar_ch->next_in_room)

		  spell_remove_curse(level,ch,tar_ch,0, skill);
	 break;
	 default :
	 log("Serious screw-up in remove curse!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}



int cast_remove_poison( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_remove_poison(level, ch, tar_ch, tar_obj, skill);
		 break;
	 case SPELL_TYPE_WAND:
	         if(!tar_ch) return eFAILURE;
         return spell_remove_poison(level, ch, tar_ch, 0, skill);
	break;
	 case SPELL_TYPE_POTION:
	 return spell_remove_poison(level, ch, ch, 0, skill);
	 break;
	 case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
			tar_ch ; tar_ch = tar_ch->next_in_room)

		  spell_remove_poison(level,ch,tar_ch,0, skill);
	 break;
	 default :
	 log("Serious screw-up in remove poison!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}



int cast_fireshield( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_fireshield(level, ch, tar_ch, 0, skill);
		 break;
	 case SPELL_TYPE_POTION:
	 return spell_fireshield(level, ch, ch, 0, skill);
	 break;
	 case SPELL_TYPE_SCROLL:
	 if(tar_obj)
		return eFAILURE;
	 if(!tar_ch) tar_ch = ch;
		 return spell_fireshield(level, ch, tar_ch, 0, skill);
		 break;
	 case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
			tar_ch ; tar_ch = tar_ch->next_in_room)

		  spell_fireshield(level,ch,tar_ch,0, skill);
	 break;
	 default :
	 log("Serious screw-up in fireshield!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int cast_sleep( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  if (IS_SET(world[ch->in_room].room_flags, SAFE)){
	 send_to_char("You can not sleep someone in a safe area!\n\r", ch);
	 return eFAILURE;
  }
  switch (type) {
  case SPELL_TYPE_SPELL:
		 return spell_sleep(level, ch, tar_ch, 0, skill);
		 break;
  case SPELL_TYPE_POTION:
		 return spell_sleep(level, ch, ch, 0, skill);
		 break;
  case SPELL_TYPE_SCROLL:
	 if(tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_sleep(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if(tar_obj) return eFAILURE;
	 return spell_sleep(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_sleep(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;
  default :
	 log("Serious screw-up in sleep!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_strength( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_strength(level, ch, tar_ch, 0, skill);
		 break;
	 case SPELL_TYPE_POTION:
		 return spell_strength(level, ch, ch, 0, skill);
		 break;
	 case SPELL_TYPE_SCROLL:
	 if(tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_strength(level, ch, tar_ch, 0, skill);
	 break;
	 case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
			tar_ch ; tar_ch = tar_ch->next_in_room)

		  spell_strength(level,ch,tar_ch,0, skill);
	 break;
	 default :
	 log("Serious screw-up in strength!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int cast_ventriloquate( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
	 CHAR_DATA *tmp_ch;
	 char buf1[MAX_STRING_LENGTH];
	 char buf2[MAX_STRING_LENGTH];
	 char buf3[MAX_STRING_LENGTH];

	 if (type != SPELL_TYPE_SPELL) {
	log("Attempt to ventriloquate by non-cast-spell.", ANGEL, LOG_BUG);
	return eFAILURE;
	 }
	 for(; *arg && (*arg == ' '); arg++);
	 if (tar_obj) {
	sprintf(buf1, "The %s says '%s'\n\r", fname(tar_obj->name), arg);
	sprintf(buf2, "Someone makes it sound like the %s says '%s'.\n\r",
	  fname(tar_obj->name), arg);
	 }   else {
	sprintf(buf1, "%s says '%s'\n\r", GET_SHORT(tar_ch), arg);
	sprintf(buf2, "Someone makes it sound like %s says '%s'\n\r",
	  GET_SHORT(tar_ch), arg);
	 }

	 sprintf(buf3, "Someone says, '%s'\n\r", arg);

	 for (tmp_ch = world[ch->in_room].people; tmp_ch;
		tmp_ch = tmp_ch->next_in_room) {

	if ((tmp_ch != ch) && (tmp_ch != tar_ch)) {
		 if ( saves_spell(ch, tmp_ch, 0, SAVE_TYPE_MAGIC) >= 0 )
		send_to_char(buf2, tmp_ch);
		 else
		send_to_char(buf1, tmp_ch);
	} else {
		 if (tmp_ch == tar_ch)
		send_to_char(buf3, tar_ch);
	}
	 }
  return eSUCCESS;
}



int cast_word_of_recall( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  CHAR_DATA *tar_ch_next;

  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_word_of_recall(level, ch, ch, 0, skill);
		 break;
	 case SPELL_TYPE_POTION:
		 return spell_word_of_recall(level, ch, ch, 0, skill);
		 break;
	 case SPELL_TYPE_SCROLL:
	 if(tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_word_of_recall(level, ch, tar_ch, 0, skill);
	 break;
	 case SPELL_TYPE_WAND:
	 if(tar_obj) return eFAILURE;
	 return spell_word_of_recall(level, ch, tar_ch, 0, skill);
	 break;
	 case SPELL_TYPE_STAFF:
	for (tar_ch = world[ch->in_room].people ;
		 tar_ch ; tar_ch = tar_ch_next)
	{
		 tar_ch_next = tar_ch->next_in_room;
		 if ( !IS_NPC(tar_ch) )
		spell_word_of_recall(level,ch,tar_ch,0, skill);
	}
	break;
	 default :
	 log("Serious screw-up in word of recall!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}



int cast_wizard_eye( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_wizard_eye(level, ch, tar_ch, 0, skill);
		 break;
		default :
				log("Serious screw-up in wizard eye!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int cast_summon( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_summon(level, ch, tar_ch, 0, skill);
		 break;
		default :
	 log("Serious screw-up in summon!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}



int cast_charm_person( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  switch (type) {
  case SPELL_TYPE_SPELL:
		 return spell_charm_person(level, ch, tar_ch, 0, skill);
		 break;
  case SPELL_TYPE_SCROLL:
	 if(!tar_ch) return eFAILURE;
	 return spell_charm_person(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_charm_person(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;

  default :
	 log("Serious screw-up in charm person!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}



int cast_sense_life( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_sense_life(level, ch, ch, 0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_sense_life(level, ch, ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 return spell_sense_life(level, ch, ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)

	 spell_sense_life(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw-up in sense life!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_identify( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_identify(level, ch, tar_ch, tar_obj, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 return spell_identify(level, ch, tar_ch, tar_obj, skill);
	 break;
  default:
		log("Serious screw-up in identify!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}



int cast_frost_breath( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_frost_breath(level, ch, tar_ch, 0, skill);
		 break;   /* It's a spell.. But people can't cast it! */
		default :
	 log("Serious screw-up in frostbreath!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_acid_breath( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_acid_breath(level, ch, tar_ch, 0, skill);
		 break;   /* It's a spell.. But people can't cast it! */
		default :
	 log("Serious screw-up in acidbreath!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_fire_breath( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		return spell_fire_breath(level,ch,ch,0, skill);
	 break;
		 /* THIS ONE HURTS!! */
		default :
	 log("Serious screw-up in firebreath!", ANGEL, LOG_BUG);
	 break;
    }
  return eFAILURE;
}


int cast_gas_breath( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
		return spell_gas_breath(level,ch,ch,0, skill);
	 break;
		 /* THIS ONE HURTS!! */
		default :
	 log("Serious screw-up in gasbreath!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_lightning_breath( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_lightning_breath(level, ch, tar_ch, 0, skill);
		 break;   /* It's a spell.. But people can't cast it! */
		default :
	 log("Serious screw-up in lightningbreath!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_fear( byte level, CHAR_DATA *ch, char *arg, int type,
		CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  int retval;
  char_data * next_v;

  if (IS_SET(world[ch->in_room].room_flags, SAFE)){
         send_to_char("You can not fear someone in a safe area!\n\r", ch);
         return eFAILURE;
  }

  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_fear(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_fear(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_fear(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_fear(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;
  default:
	 log("Serious screw-up in fear!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_refresh( byte level, CHAR_DATA *ch, char *arg, int type,
	  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_refresh(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
	 tar_ch = ch;
         return spell_refresh(level, ch, tar_ch, 0, skill);
         break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_refresh(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_refresh(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people;
	 tar_ch; tar_ch = tar_ch->next_in_room)

	spell_refresh(level, ch, tar_ch, 0, skill);
    break;
  default:
	 log("Serious screw-up in refresh!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_fly( byte level, CHAR_DATA *ch, char *arg, int type,
	  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_fly(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_fly(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_fly(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 tar_ch = ch;
         return spell_fly(level, ch, tar_ch, 0, skill);
         break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people;
	 tar_ch; tar_ch = tar_ch->next_in_room)

	spell_fly(level, ch, tar_ch, 0, skill);
    break;
  default:
	 log("Serious screw-up in fly!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_cont_light( byte level, CHAR_DATA *ch, char *arg, int type,
		  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_cont_light(level, ch, 0, tar_obj, skill);
    break;
  case SPELL_TYPE_SCROLL:
	 return spell_cont_light(level, ch, 0, tar_obj, skill);
	 break;
  default:
	 log("Serious screw-up in cont_light", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_know_alignment(byte level, CHAR_DATA *ch, char *arg, int type,
		  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_know_alignment(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_know_alignment(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_know_alignment(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people;
	 tar_ch; tar_ch = tar_ch->next_in_room)
      spell_know_alignment(level, ch, tar_ch, 0, skill);
	 break;
  default:
	 log("Serious screw-up in know alignment!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_dispel_magic( byte level, CHAR_DATA *ch, char *arg, int type,
			 CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_dispel_magic(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 tar_ch = ch;
         return spell_dispel_magic(level, ch, tar_ch, 0, skill);
         break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_dispel_magic(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
    if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_dispel_magic(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people;
	 tar_ch; tar_ch = tar_ch->next_in_room)
      spell_dispel_magic(level, ch, tar_ch, 0, skill);
    break;
  default:
	 log("Serious screw-up in dispel magic!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_dispel_minor( byte level, CHAR_DATA *ch, char *arg, int type,
			 CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_dispel_minor(level, ch, tar_ch, tar_obj, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (!tar_ch) tar_ch = ch;
	 return spell_dispel_minor(level, ch, tar_ch, tar_obj, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (!tar_ch) tar_ch = ch;
	 return spell_dispel_minor(level, ch, tar_ch, tar_obj, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people;
	 tar_ch; tar_ch = tar_ch->next_in_room)
         spell_dispel_minor(level, ch, tar_ch, 0, skill);
    break;
  default:
	 log("Serious screw-up in dispel minor!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_conjure_elemental( byte level, CHAR_DATA *ch,
		char *arg, int type,
		CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  /* not working yet! */
  return eFAILURE;
}

int cast_cure_serious( byte level, CHAR_DATA *ch, char *arg, int type,
	       CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
     return spell_cure_serious(level, ch, tar_ch, 0, skill);
     break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_cure_serious(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj) return eFAILURE;
    if (!tar_ch) tar_ch = ch;
    return spell_cure_serious(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people;
	 tar_ch; tar_ch = tar_ch->next_in_room)
		spell_cure_serious(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_POTION:
     return spell_cure_serious(level, ch, tar_ch, 0, skill);
     break;
  default:
	 log("Serious screw-up in cure serious!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_cause_light( byte level, CHAR_DATA *ch, char *arg, int type,
			CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  char_data * next_v;
  int retval;

  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_cause_light(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
       return eFAILURE;
    if (!tar_ch)
       tar_ch = ch;
    return spell_cause_light(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) return eFAILURE;
    if (!tar_ch) tar_ch = ch;
    return spell_cause_light(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_cause_light(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;

  case SPELL_TYPE_POTION:
	 return spell_cause_light(level, ch, tar_ch, 0, skill);
	 break;
  default:
	 log("Serious screw-up in cause light!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_cause_critical( byte level, CHAR_DATA *ch, char *arg,
		  int type, CHAR_DATA *tar_ch,
	     struct obj_data *tar_obj, int skill)
{
  char_data * next_v;
  int retval;

  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_cause_critical(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_cause_critical(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj) return eFAILURE;
    if (!tar_ch) tar_ch = ch;
    return spell_cause_critical(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_cause_critical(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;
  case SPELL_TYPE_POTION:
     return spell_cause_critical(level, ch, tar_ch, 0, skill);
     break;
  default:
	 log("Serious screw-up in cause critical!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_cause_serious( byte level, CHAR_DATA *ch, char *arg,
	    int type, CHAR_DATA *tar_ch,
		 struct obj_data *tar_obj, int skill)
{
  char_data * next_v;
  int retval;

  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_cause_serious(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
    return spell_cause_serious(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
    if (tar_obj) return eFAILURE;
    if (!tar_ch) tar_ch = ch;
	 return spell_cause_serious(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_cause_serious(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;
  case SPELL_TYPE_POTION:
    return spell_cause_serious(level, ch, tar_ch, 0, skill);
    break;
  default:
	 log("Serious screw-up in cause serious!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_flamestrike( byte level, CHAR_DATA *ch, char *arg,
			int type, CHAR_DATA *tar_ch,
			struct obj_data *tar_obj, int skill)
{
  char_data * next_v;
  int retval;

  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_flamestrike(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
    if (!tar_ch) tar_ch = ch;
	 return spell_flamestrike(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_flamestrike(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_flamestrike(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;
  default:
	 log("Serious screw-up in flamestrike!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_resist_cold( byte level, CHAR_DATA *ch, char *arg,
		  int type, CHAR_DATA *tar_ch,
		  struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_resist_cold(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_POTION:
    return spell_resist_cold(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
      if (tar_obj) return eFAILURE;
      if (!tar_ch) tar_ch = ch;
    return spell_resist_cold(level, ch, tar_ch, 0, skill);
    break;
  default:
	 log("Serious screw-up in resist_cold!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}
int cast_staunchblood(byte level, CHAR_DATA *ch, char *arg, int type,
                       CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill) {
  switch(type) {
    case SPELL_TYPE_SPELL:
      return spell_staunchblood(level, ch, 0, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_staunchblood(level, ch, 0, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) return eFAILURE;
      if (!tar_ch) tar_ch = ch;
      return spell_staunchblood(level, tar_ch, 0, 0, skill);
      break;
    default:
      log("Serious screw-up in cast_staunchblood!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}

int cast_resist_energy(byte level, CHAR_DATA *ch, char *arg, int type,
                        CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill) {
  switch(type) {
    case SPELL_TYPE_SPELL:
      return spell_resist_energy(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_resist_energy(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) return eFAILURE;
      if (!tar_ch) tar_ch = ch;
      return spell_resist_energy(level, ch, tar_ch, 0, skill);
      break;
    default:
      log("Serious screw-up in resist energy!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}

int cast_resist_fire( byte level, CHAR_DATA *ch, char *arg,
		  int type, CHAR_DATA *tar_ch,
		  struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_resist_fire(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
	 return spell_resist_fire(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
      if (tar_obj) return eFAILURE;
      if (!tar_ch) tar_ch = ch;
    return spell_resist_fire(level, ch, tar_ch, 0, skill);
    break;
  default:
	 log("Serious screw-up in resist_fire!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_stone_skin( byte level, CHAR_DATA *ch, char *arg,
		  int type, CHAR_DATA *tar_ch,
		  struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_stone_skin(level, ch, 0, 0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_stone_skin(level, ch, 0, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
      if (tar_obj) return eFAILURE;
      if (!tar_ch) tar_ch = ch;
	 return spell_stone_skin(level, tar_ch, 0, 0, skill);
	 break;
  default:
    log("Serious screw-up in stone skin!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_shield( byte level, CHAR_DATA *ch, char *arg,
	 int type, CHAR_DATA *tar_ch,
	 struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_shield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_shield(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_POTION:
    return spell_shield(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_shield(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people;
	 tar_ch; tar_ch = tar_ch->next_in_room)
		spell_shield(level, ch, tar_ch, 0, skill);
	 break;
  default:
    log("Serious screw-up in shield!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_weaken( byte level, CHAR_DATA *ch, char *arg,
	 int type, CHAR_DATA *tar_ch,
	 struct obj_data *tar_obj, int skill)
{
  char_data * next_v;
  int retval;

  if (IS_SET(world[ch->in_room].room_flags,SAFE)){
    send_to_char("You can not weaken anyone in a safe area!\n\r", ch);
    return eFAILURE;
  } 

  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_weaken(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_weaken(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_weaken(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_weaken(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;
  default:
	 log("Serious screw-up in weaken!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_mass_invis( byte level, CHAR_DATA *ch, char *arg,
		  int type, CHAR_DATA *tar_ch,
		  struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_mass_invis(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 return spell_mass_invis(level, ch, 0, 0, skill);
	 break;
  case SPELL_TYPE_WAND:
	 return spell_mass_invis(level, ch, 0, 0, skill);
	 break;
  default:
	 log("Serious screw-up in mass invis!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_acid_blast( byte level, CHAR_DATA *ch, char *arg,
		  int type, CHAR_DATA *tar_ch,
	     struct obj_data *tar_obj, int skill)
{
  char_data * next_v;
  int retval;

  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_acid_blast(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_acid_blast(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj) return eFAILURE;
    if (!tar_ch) tar_ch = ch;
    return spell_acid_blast(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_acid_blast(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;
  default:
	 log("Serious screw-up in acid blast!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_hellstream( byte level, CHAR_DATA *ch, char *arg,
		  int type, CHAR_DATA *tar_ch,
		  struct obj_data *tar_obj, int skill)
{
  char_data * next_v;
  int retval;

  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_hellstream(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
    if (!tar_ch) tar_ch = ch;
	 return spell_hellstream(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_POTION:
	return spell_hellstream(level, ch, ch, 0, skill);
  case SPELL_TYPE_WAND:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_hellstream(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
    {    
      next_v = tar_ch->next_in_room;
         
      if ( !ARE_GROUPED(ch, tar_ch) )
      {  
        retval = spell_hellstream(level, ch, tar_ch, 0, skill); 
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }  
    }    
    return eSUCCESS;
    break;
  default:
	 log("Serious screw-up in hell stream!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

int cast_portal( byte level, CHAR_DATA *ch, char *arg,
		int type, CHAR_DATA *tar_ch,
		struct obj_data *tar_obj, int skill)
{
   switch(type) {
      case SPELL_TYPE_SPELL:
         if (!IS_MOB(ch) && GET_CLASS(ch) == CLASS_CLERIC) {
	    if ((GET_MANA(ch) - 90) < 0) {
               send_to_char("You just don't have the energy!\n\r", ch);
               GET_MANA(ch) += 50;
               return eFAILURE;
               }
            else {
               GET_MANA(ch) -= 90;
               }
	    }
	    
         return spell_portal(level, ch, tar_ch, 0, skill);
         break;
      default:
         log("Serious screw-up in portal!", ANGEL, LOG_BUG);
         break;
      }
  return eFAILURE;
}


int cast_infravision( byte level, CHAR_DATA *ch, char *arg,
			int type, CHAR_DATA *tar_ch,
			struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_infravision(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_infravision(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people;
	 tar_ch; tar_ch = tar_ch->next_in_room)

	spell_infravision(level, ch, tar_ch, 0, skill);
	 break;
  default:
	 log("Serious screw-up in infravision!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_animate_dead( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
			return spell_animate_dead(level, ch, 0, tar_obj, skill);
			break;

	 case SPELL_TYPE_SCROLL:
			if(!tar_obj) return eFAILURE;
			return spell_animate_dead(level, ch, 0, tar_obj, skill);
			break;
	 default :
		log("Serious screw-up in Animate Dead!", ANGEL, LOG_BUG);
		break;
	}
  return eFAILURE;
}

int spell_bee_sting(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;
   int retval;
   int bees = 1 + (GET_LEVEL(ch) / 15) + (GET_LEVEL(ch)==50);
   affected_type af;
   int i;
   set_cantquit(ch, victim);
//   dam = dice(2, 8) + skill/2;
   dam = 30; 
//   if (saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) >= 0)
  //    dam >>= 1;

   for (i = 0; i < bees; i++) {
     skill_increase_check(ch, SPELL_BEE_STING, skill, SKILL_INCREASE_MEDIUM);

   retval = spell_damage(ch, victim, dam, TYPE_STING, SPELL_BEE_STING, 0);
   if(SOMEONE_DIED(retval))
      return retval;
   }
   // Extra added bonus 1% of the time
   if (dice(1, 100) == 1)
      if (!IS_SET(victim->immune, ISR_POISON))
         if (saves_spell(ch, victim, 0, SAVE_TYPE_POISON) < 0) {
            af.type = SPELL_POISON;
            af.duration = level*2;
            af.modifier = -2;
            af.location = APPLY_STR;
            af.bitvector = AFF_POISON;
            affect_join(victim, &af, FALSE, FALSE);
            send_to_char("You seem to have an allergic reaction to these bees!\n\r", victim);
            act("$N seems to be allergic to your bees!", ch, 0, victim,
	        TO_CHAR, 0);
            }
  return eSUCCESS;
}


int cast_bee_sting(byte level, CHAR_DATA *ch, char *arg, int type,
                    CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   switch (type) {
      case SPELL_TYPE_SPELL:
         if (!OUTSIDE(ch)) {
            send_to_char("Your spell is more draining because you are indoors!\n\r", ch);
            GET_MANA(ch) -= level / 2;
            if(GET_MANA(ch) < 0)
               GET_MANA(ch) = 0;
         }
         return spell_bee_sting(level, ch, victim, 0, skill);
         break;
      case SPELL_TYPE_POTION:
	 return spell_bee_sting(level, ch, ch, 0, skill);
      default :
	 log("Serious screw-up in bee sting!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_bee_swarm(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   int dam;
   int retval;
   CHAR_DATA *tmp_victim, *temp;

//   dam = dice(6+skill/2, 5);
   dam = 150;
   skill_increase_check(ch, SPELL_BEE_SWARM, skill, SKILL_INCREASE_MEDIUM);

   act("$n Calls upon the insect world!\n\r", ch, 0, 0, TO_ROOM, INVIS_NULL);

   for(tmp_victim = character_list; tmp_victim; tmp_victim = temp) {
      temp = tmp_victim->next;
      if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
          (!ARE_GROUPED(ch,tmp_victim))) {
         set_cantquit(ch, tmp_victim);
         retval = spell_damage(ch, tmp_victim, dam, TYPE_MAGIC, SPELL_BEE_SWARM, 0);
         if(IS_SET(retval, eCH_DIED))
           return retval;
         }
      else if(world[ch->in_room].zone == world[tmp_victim->in_room].zone)
         send_to_char("You hear the buzzing of hundreds of bees.\n\r",
	              tmp_victim);
      }
  return eSUCCESS;
}

int cast_creeping_death(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   int dam;
   int retval;
   affected_type af;

   set_cantquit(ch, victim);
//   dam = dice(6+skill/2, 19);
  dam = 250;
//   if (saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) >= 0)
  //    dam >>= 1;
   
   if (!OUTSIDE(ch)) {
      send_to_char("Your spell is more draining because you are indoors!\n\r", ch);
      // If they are NOT outside it costs extra mana
      GET_MANA(ch) -= level / 2;
      if(GET_MANA(ch) < 0)
         GET_MANA(ch) = 0;
   }

   skill_increase_check(ch, SPELL_CREEPING_DEATH, skill, SKILL_INCREASE_MEDIUM);

   retval = spell_damage(ch, victim, dam, TYPE_MAGIC, SPELL_CREEPING_DEATH, 0);
   if(SOMEONE_DIED(retval))
      return retval;

   // 5% of the time the victim will need a save vs. poison
   if (dice(1, 20) == 1 && 
       skill > 25 &&
      !IS_SET(victim->immune, ISR_POISON)) 
   {
      af.type = SPELL_POISON;
      af.duration = level*2;
      af.modifier = -2;
      af.location = APPLY_STR;
      af.bitvector = AFF_POISON;
      affect_join(victim, &af, FALSE, FALSE);
      send_to_char("The insect poison has gotten into your blood!\n\r", victim);
      act("$N has been poisoned by your insect swarm!", ch, 0, victim, TO_CHAR, 0);
   }
	    
   // 2% of the time the victim will actually die from the swarm
   if (number(1, 101) > 99 && 
       skill > 70 &&
       GET_LEVEL(victim) < IMMORTAL) 
   {
      dam = GET_HIT(victim)*2 + 20;
      send_to_char("The insects are crawling in your mouth, out of your eyes, "
                   "through your stomach!\n\r", victim);
      act("$N is completely consumed by insects!", ch, 0, victim, TO_ROOM, NOTVICT);
      act("$N is completely consumed by insects!", ch, 0, victim, TO_CHAR, 0);
      return spell_damage(ch, victim, dam, TYPE_MAGIC, SPELL_CREEPING_DEATH, 0);
   }
   return eSUCCESS;
}

int cast_barkskin(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_BARKSKIN)) {
    send_to_char("You cannot make your skin any stronger!\n\r", ch);
    GET_MANA(ch) += 20;
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_BARKSKIN, skill, SKILL_INCREASE_EASY);

  af.type      = SPELL_BARKSKIN;
  af.duration  = 5 + level/7;
  af.modifier  = -20;
  af.location  = APPLY_AC;
  af.bitvector = 0;

  affect_join(victim, &af, FALSE, FALSE);

  SET_BIT(victim->resist, ISR_SLASH);

  send_to_char("Your skin turns stiff and bark-like.\n\r", victim);
  act("$N begins to look rather woody.", ch, 0, victim, TO_ROOM, INVIS_NULL|NOTVICT);
  if(!OUTSIDE(ch))
  {
    send_to_char("Your spell is more draining because you are indoors!\n\r", ch);
    GET_MANA(ch) -= 25;
    if(GET_MANA(ch) < 0)
       GET_MANA(ch) = 0;
  }
  return eSUCCESS;
}

int cast_herb_lore(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Heal spells seem to be useless on the undead.\n\r", ch);
    return eFAILURE;
  }

  skill_increase_check(ch, SPELL_HERB_LORE, skill, SKILL_INCREASE_MEDIUM);

  send_to_char("These herbs really do the trick!\n\r", ch);
  send_to_char("You feel much better!\n\r", victim);
  if(OUTSIDE(ch))
    GET_HIT(victim) += dam_percent(skill,80);
  else /* if not outside */
    GET_HIT(victim) += dam_percent(skill,180);

  if (GET_HIT(victim) >= hit_limit(victim))
    GET_HIT(victim) = hit_limit(victim)-dice(1,4);

  update_pos( victim );

  return eSUCCESS;
}

// TODO - make this use skill
int cast_call_follower(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   if(IS_SET(world[ch->in_room].room_flags, CLAN_ROOM)) {
      send_to_char("I don't think your fellow clan members would appreciate the wildlife.\n\r", ch);
      GET_MANA(ch) += 75;
      return eFAILURE;
   }

   victim = NULL;

   for(struct follow_type *k = ch->followers; k; k = k->next)
     if(IS_MOB(k->follower) && affected_by_spell(k->follower, SPELL_CHARM_PERSON))
     {
        victim = k->follower;
        break;
     }

   if (NULL == victim) {
      send_to_char("You don't have any tame friends to summon!\n\r", ch);
      return eFAILURE;
   }

   act("$n disappears suddenly.", victim, 0, 0, TO_ROOM, INVIS_NULL);
   move_char(victim, ch->in_room);
   act("$n arrives suddenly.", victim, 0, 0, TO_ROOM, INVIS_NULL);
   act("$n has summoned you!", ch, 0, victim, TO_VICT, 0);
   do_look(victim, "", 15);
	 
   if (!OUTSIDE(ch)) {
      send_to_char("Your spell is more draining because you are indoors!\n\r", ch);
      // If they are NOT outside it costs extra mana
      GET_MANA(ch) -= level / 2; 
      if(GET_MANA(ch) < 0)
         GET_MANA(ch) = 0;
   }
   return eSUCCESS;
}

// TODO - make this use skill
int cast_entangle(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{

	if(!OUTSIDE(ch))
	{
		send_to_char("You must be outside to cast this spell!\n\r", ch);
		GET_MANA(ch) += 10; /*Mana kludge */
		return eFAILURE;
	}
	set_cantquit(ch, victim);
	act("Your plants grab $N, attempting to force them down.", ch, 0,
	  victim, TO_CHAR, 0);
	act("$n's plants make an attempt to drag you to the ground!",
	  ch, 0, victim, TO_VICT, 0);
	act("$n raises the plants which attack $N!", ch, 0, victim,
		TO_ROOM, NOTVICT);
	spell_blindness(level, ch, victim, 0, 0); /* The plants blind the victim . . */
	GET_POS(victim) = POSITION_SITTING;		/* And pull the victim down to the ground */
	update_pos(victim);
	return eSUCCESS;
}

int cast_eyes_of_the_owl(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
	struct affected_type af;

	if(affected_by_spell(victim, SPELL_INFRAVISION))
		affect_from_char(victim, SPELL_INFRAVISION);

	if(IS_AFFECTED(victim, AFF_INFRARED))
	{
		GET_MANA(ch) += 5;
		return eFAILURE;
	}

	af.type      = SPELL_INFRAVISION;
	af.duration  = skill*2;
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = AFF_INFRARED;
	affect_join(victim, &af, FALSE, FALSE);

  send_to_char("You feel your vision become much more acute.\n\r", victim);
  return eSUCCESS;
}

// TODO - spells from here down do not yet use skill
int cast_feline_agility(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
	struct affected_type af;

	if(ch != victim)
	{
		send_to_char("You can only cast this spell on yourself!\n\r", ch);
		return eFAILURE;
	}
	if(affected_by_spell(victim, SPELL_FELINE_AGILITY))
	{
		send_to_char("You cannot be as agile as TWO cats!\n\r", ch);
		GET_MANA(ch) += 20;
		return eFAILURE;
	}
       skill_increase_check(ch, SPELL_FELINE_AGILITY, level, SKILL_INCREASE_EASY);

	af.type 	= SPELL_FELINE_AGILITY;
	af.duration	= 1+(level/4);
	af.modifier	= -40;			/* -40 Ac!! */
	af.location 	= APPLY_AC;
	af.bitvector	= 0;
	affect_to_char(victim, &af);

	af.modifier		= 2;							/* +1 dex */
	af.location		= APPLY_DEX;
	affect_to_char(victim, &af);

	if(!OUTSIDE(ch))
	{
		send_to_char("Your spell is more draining because you are indoors!\n\r", ch);
		GET_MANA(ch) -= level / 2; /* If they are NOT outside it costs extra mana */
		if(GET_MANA(ch) < 0)
			GET_MANA(ch) = 0;
	}
	return eSUCCESS;
}
int cast_oaken_fortitude(byte level, CHAR_DATA *ch, char *arg, int type, 
CHAR_DATA  *victim, struct obj_data * tar_obj, int skill)
{ // Feline agility rip
        struct affected_type af;

        if(ch != victim)
        {
                send_to_char("You can only cast this spell on yourself!\n\r", ch);
                return eFAILURE;
        }
        if(affected_by_spell(victim, SPELL_OAKEN_FORTITUDE))
        {
	 	send_to_char("You cannot enhance your fortitude further.\r\n",ch);
		return eFAILURE;
        }
       skill_increase_check(ch, SPELL_OAKEN_FORTITUDE, level, SKILL_INCREASE_EASY);


        af.type         = SPELL_OAKEN_FORTITUDE;
        af.duration     = 1+(level/4);
        af.modifier     = -40;                  /* -40 Ac!! */
        af.location     = APPLY_AC;
        af.bitvector    = 0;
        affect_to_char(victim, &af);
	send_to_char("Your fortitude increases to that of an oak.\r\n",ch);
        af.modifier             = 2;
        af.location             = APPLY_CON;
        affect_to_char(victim, &af);
	return eSUCCESS;
}


int cast_forest_meld(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
	if(!(world[ch->in_room].sector_type == SECT_FOREST))
	{
		send_to_char("You are not in a forest!!\n\r", ch);
		return eFAILURE;
	}
	if(victim != ch)
	{
		send_to_char("Why would the forest like anyone but you?\n\r", ch);
		return eFAILURE;
	}
	act("$n melts into the forest and is gone.", ch, 0, 0,
	  TO_ROOM, INVIS_NULL);
	send_to_char("You feel yourself slowly become a temporary part of the living forest.\n\r", ch);
        SET_BIT(ch->affected_by2, AFF_FOREST_MELD);
        SET_BIT(ch->affected_by, AFF_HIDE);
	return eSUCCESS;
}

int cast_companion(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   CHAR_DATA *mob;
   struct affected_type af;
   char name[MAX_STRING_LENGTH];
   char desc[MAX_STRING_LENGTH];
   int number = 19309;	// Mob number

/* remove this whenever someone actually fixes this spell -pir */
   send_to_char("Spell not finished.\r\n", ch);
   return eFAILURE;

   if (!OUTSIDE(ch)) {
      send_to_char("You cannot use such powerful magic indoors!\n\r", ch);
      return eFAILURE;
      }
   
   // Load up the standard fire ruler from elemental canyon (mob #19309) */
   mob = clone_mobile(number);
   char_to_room(mob, ch->in_room);

   // I am not sure of a better way to do this, so I just set the duration
   // to roughly 24 hours.  Odds are the mud will crash or be rebooted
   // before the 24 hours elapse anyway.
   
   af.type = SPELL_CHARM_PERSON;
   af.duration = 960;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = AFF_CHARM;
   add_follower(mob, ch, 0);
   affect_join(mob,&af, FALSE, FALSE);

   // The mob should have zero xp
   GET_EXP(mob) = 0;
   IS_CARRYING_W(mob) = 0;
   IS_CARRYING_N(mob) = 0;

   switch(dice(1, 4)) {
      case 1:
         sprintf(desc, "%s's very powerful fire elemental", GET_NAME(ch));
         sprintf(name, "fire ");
         mob->max_hit += 300;
         if (dice(1, 100) == 1)	// they got the .25% chance - very lucky
            SET_BIT(mob->affected_by, AFF_FIRESHIELD);
         SET_BIT(mob->resist, ISR_FIRE);
         break;
      case 2:
         sprintf(desc, "%s's very powerful air elemental", GET_NAME(ch));
         sprintf(name, "air ");
         mob->max_hit += 200;
         SET_BIT(mob->resist, ISR_SLASH);
         break;
      case 3:
         sprintf(desc, "%s's very powerful water elemental", GET_NAME(ch));
         sprintf(name, "water ");
         mob->max_hit += 200;
         SET_BIT(mob->resist, ISR_HIT);
         break;
      case 4:
         sprintf(desc, "%s's very powerful earth elemental", GET_NAME(ch));
         sprintf(name, "earth ");
         mob->max_hit += 500;
         SET_BIT(mob->resist, ISR_PIERCE);
         break;
      default:
         sprintf(desc, "%s's very powerful GAME MESSUP", GET_NAME(ch));
         mob->max_hit += 1000;
         break;
      } // of switch
      
   mob->hit = mob->max_hit; // Set elem to full hps
   strcat(name, "elemental companion");
   mob->name = str_hsh(name);
   mob->short_desc = str_hsh(desc);
   mob->long_desc = str_hsh(desc);
   
   // Set mob level - within 5 levels of the character
   if (dice(1, 2) - 1) 
      GET_LEVEL(mob) -= dice(1, 5); 
   else
      GET_LEVEL(mob) += dice(1, 5);

   // Now set the AFF_CREATOR flag on the character for two hours
   af.type = 0;
   af.duration = 80;   // Roughly 2 hours
   af.modifier = 0;
   af.location = 0;
   af.bitvector = 0;
   affect_join(ch, &af, FALSE, FALSE);
   return eSUCCESS;
}


// Procedure for checking for/destroying spell components                       
// Expects SPELL_xxx, the caster, and boolean as to destroy the components      
// Returns TRUE for success and FALSE for failure                               
                                                                                
int check_components(CHAR_DATA *ch, int destroy, int item_one = 0, 
                     int item_two = 0, int item_three = 0, int item_four = 0)
{
  // We're going to assume you never have more than 4 items                       
  // for a spell, though you can easily change to take more                       
  int all_ok = 0;     
  obj_data *ptr_one, *ptr_two, *ptr_three, *ptr_four;                             
                                                                                
  ptr_one = ptr_two = ptr_three = ptr_four = NULL;                              
                                                                                
  if(!ch)                                                                       
  {                                                                             
    log("No ch sent to check spell components", ANGEL, LOG_BUG);                
    return FALSE;                                                               
  }                                                                             

  if(!ch->carrying)                                                             
    return FALSE;                                                               
                                                                                
  ptr_one = get_obj_in_list_num(real_object(item_one), ch->carrying);                        
  if(item_two)                                                                  
    ptr_two = get_obj_in_list_num(real_object(item_two), ch->carrying);                      
  if(item_three)                                                                
    ptr_three = get_obj_in_list_num(real_object(item_three),ch->carrying);                  
  if(item_four)                                                                 
    ptr_four = get_obj_in_list_num(real_object(item_four), ch->carrying);                    
                                                                                
// Destroy the components if needed                                             
                                                                                
  if(destroy)                                                                   
  {                                                                             
    int gone = FALSE;
    if(ptr_one)                                                                 
      { obj_from_char(ptr_one); extract_obj(ptr_one); gone = TRUE;}                         
    if(ptr_two)                                                                 
      { obj_from_char(ptr_two); extract_obj(ptr_two); gone = TRUE;}                         
    if(ptr_three)                                                               
      { obj_from_char(ptr_three); extract_obj(ptr_three); gone = TRUE;}                     
    if(ptr_four)                                                                
      { obj_from_char(ptr_four); extract_obj(ptr_four); gone = TRUE;}
    if(gone)                       
      send_to_char("The spell components poof into smoke.\r\n", ch);              
  }                                                                             
                                                                                
                                                                               
// Make sure we found everything before saying its OK                           
                                                                                
  all_ok = ((item_one != 0) && (ptr_one != 0));                                 
                                                                                
  if(all_ok && item_one)                                                        
     all_ok = (int) ptr_one;                                                    
  if(all_ok && item_two)                                                        
     all_ok = (int) ptr_two;                                                    
  if(all_ok && item_three)                                                      
     all_ok = (int) ptr_three;                                                  
  if(all_ok && item_four)                                                       
     all_ok = (int) ptr_four;                                                   

  if(GET_LEVEL(ch) > ARCHANGEL && !all_ok) {
     send_to_char("You didn't have the right components, but yer a god:)\r\n", ch);
     return TRUE;
  }

return all_ok;                                                                  
} 

int spell_create_golem(int level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  char buf[200];
  CHAR_DATA *mob;
  follow_type *k;

  struct affected_type af;

  send_to_char("Disabled currently,\r\n", ch);

  // make sure its a charmie
  if((!IS_NPC(victim)) || (victim->master != ch))
  {
    send_to_char("You can only do this to someone under your mental control.\r\n", ch);
    GET_MANA(ch) += 50;
    return eFAILURE;
  }

  // make sure it isn't already a golem

  if(isname("golem", GET_NAME(victim)))
  {
     send_to_char("Isn't that already a golem?\r\n", ch);
     GET_MANA(ch) += 50;
     return eFAILURE;
  }

  // check_special_room !
  if(!check_components(ch, TRUE, 14905, 2256, 3181)) {
     send_to_char("Without the proper spell components, your spell fizzles out and dies.\r\n", ch);
     act("$n's hands pop and sizzle with misused spell components.", ch, 0, 0, TO_ROOM, 0);
     return eFAILURE;
  }

  // take away 500 mana max (min is 50)
  GET_MANA(ch) -= 450;
  if(GET_MANA(ch) < 0)
    GET_MANA(ch) = 0;

  // give spiffy messages

  if(IS_EVIL(ch))
     act("$n seizes $N, strapping its unresisting form to the\r\n"
      "center of a ceremonial pentagram.  A few quiet words are uttered\r\n"
      "and $n thrusts $s finger into the heart of the captive\r\n"
      "drawing out its magical energies.  Its organs and limbs and flesh\r\n"
      "are divided into piles then reformed into the shape of a humanoid\r\n"
      "golem, obediant to its master's will.",
      ch, 0, victim, TO_ROOM, 0);
  else if(IS_GOOD(ch))
    act("$n lulls $N into a magical sleep, stopping\r\n"
      "its heart with a gentle tap of a finger to its chest.  A magical\r\n"
      "aura hides it from sight as its body reshapes and reforms,\r\n"
      "crafted by dweomers into the powerful shape of a golem,\r\n"
      "obediant only to $n.", ch, 0, victim, TO_ROOM, 0);
  else 
    act("$n begins an incantation paralyzing $N\r\n"
        "in place.  Large mana bubbles form in the air swirling around\r\n"
        "$N in a parody of dance then cling onto it coating\r\n"
        "the area in a extra-planar light.  Minutes pass till the light\r\n"
        "fades revealing $n's new golem servant.",
        ch, 0, victim, TO_ROOM, 0);

  send_to_char("Delving deep within yourself, you stretch your abilities to the very limit in an effort to create magical life!\r\n", ch);

  // create golem

  mob = clone_mobile(real_mobile(201));
  if(!mob)
  {
     send_to_char("Warning: Load mob not found in create_corpse\r\n", ch);
     return eFAILURE;
  }
  char_to_room(mob, ch->in_room);
  af.type = SPELL_CHARM_PERSON;
  af.duration = -1;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_CHARM;
  affect_to_char(mob, &af);
                                          
  GET_EXP(mob) = 0;
  mob->name = str_hsh("golem"); 
  sprintf(buf, "The golem seems to be a mishmash of other creatures binded by magic.\r\nIt appears to have pieces of %s in it.\r\n",
     GET_SHORT(victim));
  mob->description = str_hsh(buf); 
  GET_SHORT_ONLY(mob) = str_hsh("A gruesome golem");
  mob->long_desc = str_hsh("A golem created of twisted magic, stands here motionless.\r\n");
  REMOVE_BIT(mob->mobdata->actflags, ACT_SCAVENGER);
  REMOVE_BIT(mob->immune, ISR_PIERCE);
  REMOVE_BIT(mob->immune, ISR_SLASH);

  // kill charmie(s) and give stats to golem

  if(GET_MAX_HIT(victim)*2 < 32000)
     mob->max_hit = GET_MAX_HIT(victim)*2;
  else mob->max_hit = 32000;
  GET_HIT(mob) = GET_MAX_HIT(mob); /* 50% of max */
  GET_AC(mob) = GET_AC(victim) - 30;
  mob->mobdata->damnodice = victim->mobdata->damnodice+5;
  mob->mobdata->damsizedice = victim->mobdata->damsizedice;
  GET_LEVEL(mob) = 50;
  GET_HITROLL(mob) = GET_HITROLL(victim) + number(1, GET_INT(ch));
  GET_DAMROLL(mob) = GET_DAMROLL(victim) + number(1, GET_WIS(ch));
  GET_POS(mob) = POSITION_STANDING;
  GET_ALIGNMENT(mob) = GET_ALIGNMENT(ch);
  SET_BIT(mob->affected_by2, AFF_GOLEM);
  // kill um all!
  k = ch->followers;
  while(k) 
     if(IS_AFFECTED(k->follower, AFF_CHARM))                                    
     {
        fight_kill(ch, k->follower, TYPE_RAW_KILL);
        k = ch->followers;
     }
     else k = k->next;

  add_follower(mob, ch, 0);

  // add random abilities
  if(number(1, 3) > 1)
  {
    SET_BIT(mob->mobdata->actflags, ACT_2ND_ATTACK);
    if(number(1, 2) == 1)
      SET_BIT(mob->mobdata->actflags, ACT_3RD_ATTACK);
  }      

  if(number(1, 15) == 15)
    SET_BIT(mob->affected_by, AFF_SANCTUARY);

  if(number(1,20) == 20)
    SET_BIT(mob->affected_by, AFF_FIRESHIELD);

  if(number(1,2) == 2)
    SET_BIT(mob->affected_by, AFF_DETECT_INVISIBLE);

  if(number(1,7) == 7)
    SET_BIT(mob->affected_by, AFF_FLYING);

  if(number(1,10) == 10)
    SET_BIT(mob->affected_by, AFF_SNEAK);

  if(number(1,2) == 1)
    SET_BIT(mob->affected_by, AFF_INFRARED);

  if(number(1, 50) == 1)
    SET_BIT(mob->affected_by, AFF_EAS);

  if(number(1, 3) == 1)
    SET_BIT(mob->affected_by, AFF_TRUE_SIGHT);

  // lag mage
  if(number(1,3) == 3  && GET_LEVEL(ch) < ARCHANGEL) {
    act("$n falls to the ground, unable to move while $s body recovers from such an incredible and draining magical feat.",
       ch, 0, 0, TO_ROOM, 0);
    send_to_char("You drop, drained by the release of such power.\r\n", ch);
    GET_POS(ch) = POSITION_RESTING;

// why won't this line work?
//  WAIT_STATE(ch, (PULSE_VIOLENCE * number(10, 15)));
  }
  return eSUCCESS;
}

int cast_create_golem( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      return spell_create_golem(level, ch, tar_ch, 0, skill);
      break;
    default :
      log("Serious screw-up in create golem!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}


int spell_release_golem(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   // CHAR_DATA *tmp_vict;
   struct follow_type * temp;
   int done = 0;

   temp = ch->followers;

   if(!temp) {
      send_to_char("You have no golem!\r\n", ch);
      return eFAILURE;
   }

   while(temp && !done) {
      if(IS_SET(temp->follower->affected_by2, AFF_GOLEM))
         done = 1;
      else temp = temp->next;
   }
   
   if(!temp) {
      send_to_char("You have no golem!\r\n", ch);
      return eFAILURE;
   }

   fight_kill(ch, temp->follower, TYPE_RAW_KILL);   
   return eSUCCESS;
}

int spell_beacon(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
//   extern int top_of_world;
//   int to_room = 0;

   if(!ch->beacon) {
      send_to_char("You have no beacon set!\r\n", ch);
      return eFAILURE;
   }

   if(ch->beacon->in_room < 1) {
      send_to_char("Your magical beacon has become lost!\r\n", ch);
      return eFAILURE;
   }

   if((!IS_SET(world[ch->in_room].room_flags, ARENA) &&
       IS_SET(world[ch->beacon->in_room].room_flags, ARENA)
      ) ||
      (IS_SET(world[ch->in_room].room_flags, ARENA) &&
       !IS_SET(world[ch->beacon->in_room].room_flags, ARENA)
      )
     )
   {
         send_to_char("Your beacon cannot take you into/out of the arena!\r\n", ch);
         return eFAILURE;
   }

   if(ch->fighting && (0 == number(0, 20))) {
     send_to_char("In the heat of combat, you forget your beacon's location!\n\r", ch);
     act("$n's eyes widen for a moment, $s concentration broken.", ch, 0, 0, TO_ROOM, 0);
     ch->beacon->equipped_by = NULL;
     extract_obj(ch->beacon);
     ch->beacon = NULL;
     return eFAILURE;
   }

   act("Poof! $e's gone!", ch, 0, 0, TO_ROOM, 0);
   send_to_char("You rip a dimensional hole through space and step out at your beacon.\r\n", ch);

   if(!move_char(ch, ch->beacon->in_room)) {
      send_to_char("Failure in move_char.  Major fuckup.  Contact a god.\r\n", ch);
      return eFAILURE;
   }
   do_look(ch, "", 9);

   act("$n steps out from a dimensional rip.", ch, 0, 0, TO_ROOM, 0);
   return eSUCCESS;
}

int do_beacon(struct char_data *ch, char *argument, int cmd)
{
   struct obj_data * new_obj = NULL;

   if(GET_CLASS(ch) != CLASS_ANTI_PAL && GET_LEVEL(ch) < ARCHANGEL) {
      send_to_char("Sorry, but you cannot do that here!\r\n", ch);
      return eFAILURE;
   }

   send_to_char("You set a magical beacon in the air.\r\n", ch);
   if(!ch->beacon) {
      if(!(new_obj = clone_object(real_object(BEACON_OBJ_NUMBER)))) {
         send_to_char("Error setting beacon.  Contact a god.\n\r", ch);
         return eFAILURE;
      }
   }
   else {
      obj_from_room(ch->beacon);
      new_obj = ch->beacon;
   }

   new_obj->equipped_by = ch;
   obj_to_room(new_obj, ch->in_room);
   ch->beacon = new_obj;

//   ch->beacon = world[ch->in_room].number;
   return eSUCCESS;
}


int spell_reflect(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   send_to_char("Tell pir what you just did.\r\n", ch);
   return eFAILURE;
}

#define FAMILIAR_MOB_IMP        5
#define FAMILIAR_MOB_CHIPMUNK   6

int choose_druid_familiar(char_data * ch, char * arg)
{
   char buf[MAX_INPUT_LENGTH];
   
   one_argument(arg, buf);

   if(!strcmp(buf, "chipmunk"))
   {
     if(!check_components(ch, TRUE, 27800)) {
       send_to_char("You remember at the last second, you don't have an acorn ready!\r\n", ch);
       act("$n's hands pop with unused mystical energy. $e seems confused.", ch, 0, 0, TO_ROOM, 0);
       return -1;
     }
     return FAMILIAR_MOB_CHIPMUNK;
   }

   send_to_char("You must specify the type of familiar you wish to summon.\r\n"
                "You currently may summon:  chipmunk\r\n", ch);
   return -1;
}

int choose_mage_familiar(char_data * ch, char * arg)
{
   char buf[MAX_INPUT_LENGTH];
   
   one_argument(arg, buf);

   if(!strcmp(buf, "imp")) 
   {
     if(!check_components(ch, TRUE, 4)) {
       send_to_char("You remember at the last second, you don't have a batwing ready!\r\n", ch);
       act("$n's hands pop with unused mystical energy. $e seems confused.", ch, 0, 0, TO_ROOM, 0);
       return -1;
     }
     return FAMILIAR_MOB_IMP;
   }
   send_to_char("You must specify the type of familiar you wish to summon.\r\n"
                "You currently may summon:  imp\r\n", ch);
   return -1;
}

int choose_familiar(char_data * ch, char * arg)
{
   if(GET_CLASS(ch) == CLASS_DRUID)
      return choose_druid_familiar(ch, arg);
   if(GET_CLASS(ch) == CLASS_MAGIC_USER)
      return choose_mage_familiar(ch, arg);
   return -1;
}

void familiar_creation_message(char_data * ch, int fam_type)
{
  switch(fam_type) {
    case FAMILIAR_MOB_IMP:
      act("$n throws a batwing into the air which explodes into flame.\r\n"
        "a small imp appears from the smoke and perches on $n's shoulder.",
        ch, 0, 0, TO_ROOM, 0);
      send_to_char("You channel a miniture fireball into the wing and throw it into the air.\r\n"
               "A small imp appears from the flames and perches upon your shoulder.\r\n", ch);
    break;
    case FAMILIAR_MOB_CHIPMUNK:
      act("$n coaxs a chipmunk from nowhere and gives it an acorn to eat.\r\n"
          "a small chipmunk eats the acorn and looks at $n lovingly.",
          ch, 0, 0, TO_ROOM, 0);
      send_to_char("You whistle a little tune summoning a chipmunk to you and give it an acorn.\r\n"
               "The small chipmunk eats the acorn and looks at you adoringly.\r\n", ch);
    break;
    default:
      send_to_char("Illegal message in familar_creation_message.  Tell pir.\r\n", ch);
    break;
  }
}

int spell_summon_familiar(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
  char_data * mob = NULL;
  int r_num;
  struct affected_type af;
  follow_type * k = NULL;
  int fam_type;

  fam_type = choose_familiar(ch, arg);
  if(-1 == fam_type)
    return eFAILURE;

  if ((r_num = real_mobile(fam_type)) < 0) {
    send_to_char("Summon familiar mob not found.  Tell a god.\n\r", ch);
    return eFAILURE;
  }

  k = ch->followers;
  while(k)
     if(IS_AFFECTED2(k->follower, AFF_FAMILIAR))
     {
       send_to_char("But you already have a devoted pet!\r\n", ch);
       return eFAILURE;
     }
     else k = k->next;
  
  mob = clone_mobile(r_num);
  char_to_room(mob, ch->in_room);

  SET_BIT(mob->affected_by2, AFF_FAMILIAR);

  af.type = SPELL_SUMMON_FAMILIAR;
  af.duration = -1;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = 0;
  affect_to_char(mob, &af);

  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  familiar_creation_message(ch, fam_type);
  add_follower(mob, ch, 0);

  return eSUCCESS;
}

int cast_summon_familiar( byte level, CHAR_DATA *ch, char *arg, int type,
	 CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	case SPELL_TYPE_SPELL:
		 return spell_summon_familiar(level,ch, arg, SPELL_TYPE_SPELL, tar_ch,0, skill);
		 break;
	case SPELL_TYPE_POTION:
		 log("Serious screw-up in summon_familiar(potion)!", ANGEL, LOG_BUG);
		 return eFAILURE;
		 break;
	case SPELL_TYPE_SCROLL:
		 log("Serious screw-up in summon_familiar(potion)!", ANGEL, LOG_BUG);
		 return eFAILURE;
		 break;
	case SPELL_TYPE_WAND:
		 log("Serious screw-up in summon_familiar(potion)!", ANGEL, LOG_BUG);
		 return eFAILURE;
		 break;
		default :
	 log("Serious screw-up in summon_familiar!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


int spell_lighted_path( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  struct room_track_data * ptrack;
  char buf[180];
  extern struct race_shit race_info[];
  extern char * dirs[];

  ptrack = world[ch->in_room].tracks;

  if(!ptrack) {
    send_to_char("You detect no scents in this room.\r\n", ch);
    return eSUCCESS;
  }

  send_to_char("Your magic pulls the essence of scent from around you straining\r\n"
               "knowledge from it to be displayed briefly in your mind.\r\n"
               "$B$3You sense...$R\r\n", ch);

  while(ptrack) {
    sprintf(buf, "a %s called %s headed %s...\r\n",
            race_info[ptrack->race].singular_name,
            ptrack->trackee,
            dirs[ptrack->direction]);
    send_to_char(buf, ch);
    ptrack = ptrack->next;
  }

  return eSUCCESS;
}

int cast_lighted_path( byte level, CHAR_DATA *ch, char *arg, int type,
	 CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	case SPELL_TYPE_SPELL:
		 return spell_lighted_path(level,ch, "", SPELL_TYPE_SPELL, tar_ch,0, skill);
		 break;
	case SPELL_TYPE_POTION:
		 log("Serious screw-up in lighted_path(potion)!", ANGEL, LOG_BUG);
		 return eFAILURE;
		 break;
	case SPELL_TYPE_SCROLL:
		 log("Serious screw-up in lighted_path(scroll)!", ANGEL, LOG_BUG);
		 return eFAILURE;
		 break;
	case SPELL_TYPE_WAND:
		 log("Serious screw-up in lighted_path(wand)!", ANGEL, LOG_BUG);
		 return eFAILURE;
		 break;
		default :
	 log("Serious screw-up in lighted_path! (unknown)", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int spell_resist_acid(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;

    if(!affected_by_spell(victim, SPELL_RESIST_ACID)) {
       skill_increase_check(ch, SPELL_RESIST_ACID, skill, SKILL_INCREASE_MEDIUM);
       act("$n's skin turns green momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
       act("Your skin turns green momentarily.", victim, 0, 0, TO_CHAR, 0);

       af.type = SPELL_RESIST_ACID;
       af.duration = 1 + skill / 10;
       af.modifier = 10 + skill / 6;
       af.location = APPLY_SAVING_ACID;
       af.bitvector = 0;
       affect_to_char(victim, &af);
    }
  return eSUCCESS;
}

int cast_resist_acid(byte level, CHAR_DATA *ch, char *arg, int type,
                        CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill) {
  switch(type) {
    case SPELL_TYPE_SPELL:
      return spell_resist_acid(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_resist_acid(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) return eFAILURE;
      if (!tar_ch) tar_ch = ch;
      return spell_resist_acid(level, ch, tar_ch, 0, skill);
      break;
    default:
      log("Serious screw-up in resist acid!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}


int spell_sun_ray(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  extern struct weather_data weather_info;

  set_cantquit( ch, victim );

  dam = MIN((int)GET_MANA(ch), 750);

  if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY)) {

//	 if(saves_spell(ch, victim, 0, SAVE_TYPE_FIRE) >= 0)
//		dam >>= 1;

	 return spell_damage(ch, victim, dam, TYPE_FIRE, SPELL_SUN_RAY, 0);
  }
  return eFAILURE;
}

int cast_sun_ray( byte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  extern struct weather_data weather_info;
  int retval;
  char_data * next_v;

  switch (type) {
    case SPELL_TYPE_SPELL:
      if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY))
        return spell_sun_ray(level, ch, victim, 0, skill);
      else 
        send_to_char("You must be outdoors on a day that isn't raining!\n\r", ch);
      break;
    case SPELL_TYPE_POTION:
      if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY))
        return spell_sun_ray(level, ch, ch, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY)) {
        if(victim)
          return spell_sun_ray(level, ch, victim, 0, skill);
        else if(!tar_obj) spell_sun_ray(level, ch, ch, 0, skill);
      }
      break;
    case SPELL_TYPE_STAFF:
      if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY))
      {
        for (victim = world[ch->in_room].people ; victim ; victim = next_v )
        {    
          next_v = victim->next_in_room;

          if ( !ARE_GROUPED(ch, victim) )
          {  
            retval = spell_sun_ray(level, ch, victim, 0, skill); 
            if(IS_SET(retval, eCH_DIED))
              return retval;
          }  
        }    
        return eSUCCESS;
        break;
      }
      break;
    default :
      log("Serious screw-up in sun ray!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}


int spell_rapid_mend(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;

    if(!affected_by_spell(victim, SPELL_RAPID_MEND)) {
       act("$n starts to heal quicker.", victim, 0, 0, TO_ROOM, INVIS_NULL);
       send_to_char("You feel your body begin to heal quicker.", victim);

       skill_increase_check(ch, SPELL_RAPID_MEND, skill, SKILL_INCREASE_MEDIUM);

       af.type = SPELL_RAPID_MEND;
       af.duration = 5 + skill/8;
       af.modifier = skill/10;
       af.location = APPLY_NONE;
       af.bitvector = 0;
       affect_to_char(victim, &af);
    }
    else send_to_char("They are already mending quickly.\r\n", ch);

  return eSUCCESS;
}

int cast_rapid_mend(byte level, CHAR_DATA *ch, char *arg, int type,
                        CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill) {
  switch(type) {
    case SPELL_TYPE_SPELL:
      return spell_rapid_mend(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_rapid_mend(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) return eFAILURE;
      if (!tar_ch) tar_ch = ch;
      return spell_rapid_mend(level, ch, tar_ch, 0, skill);
      break;
    default:
      log("Serious screw-up in rapid_mend!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}

int spell_iron_roots(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;

    if(affected_by_spell(ch, SPELL_IRON_ROOTS)) {
       affect_from_char(ch, SPELL_IRON_ROOTS);
       act("The tree roots encasing $n's legs sink away under the surface.", ch, 0, 0, TO_ROOM, INVIS_NULL);
       act("The roots release their hold upon you and melt away beneath the surface.", ch, 0, 0, TO_CHAR, 0);
       REMOVE_BIT(ch->affected_by2, AFF_NO_FLEE);
    }
    else {
       act("Tree roots spring from the ground bracing $n's legs, holding $m down firmly to the ground.", ch, 0, 0, TO_ROOM, INVIS_NULL);
       act("Tree roots spring from the ground firmly holding you to the ground.", ch, 0, 0, TO_CHAR, 0);

       SET_BIT(ch->affected_by2, AFF_NO_FLEE);

       af.type = SPELL_IRON_ROOTS;
       af.duration = level;
       af.modifier = 0;
       af.location = APPLY_NONE;
       af.bitvector = 0;
       affect_to_char(ch, &af);
    }
  return eSUCCESS;
}

int cast_iron_roots(byte level, CHAR_DATA *ch, char *arg, int type,
                        CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill) {
  switch(type) {
    case SPELL_TYPE_SPELL:
      return spell_iron_roots(level, ch, 0, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_iron_roots(level, ch, 0, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) return eFAILURE;
      if (!tar_ch) tar_ch = ch;
      return spell_iron_roots(level, tar_ch, 0, 0, skill);
      break;
    default:
      log("Serious screw-up in iron_roots!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}

// TODO - make this use and increase in skill
int spell_acid_shield(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if (!affected_by_spell(victim, SPELL_ACID_SHIELD))
  {
    act("$n is surrounded by a gaseous shield of acid.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    act("You are surrounded by a gaseous shield of acid.", victim, 0, 0, TO_CHAR, 0);

    af.type      = SPELL_ACID_SHIELD;
    af.duration  = 3;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

int cast_acid_shield( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       if(GET_CLASS(ch) == CLASS_ANTI_PAL) {
          if(GET_ALIGNMENT(ch) > -1000) {
             send_to_char("You must be uttery vile to cast a spell of acid shield.\n\r", ch);
             GET_MANA(ch) += 150; 
             return eFAILURE;
          }
       } 
       return spell_acid_shield(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_POTION:
       return spell_acid_shield(level, ch, ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_acid_shield(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:
       for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = tar_ch->next_in_room)
          spell_acid_shield(level,ch,tar_ch,0, skill);
       break;
    default :
       log("Serious screw-up in acid shield!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

int spell_water_breathing(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  struct affected_type *cur_af;

  
  if ((cur_af = affected_by_spell(victim, SPELL_WATER_BREATHING)))
    affect_remove(victim, cur_af, SUPPRESS_ALL,FALSE);
    //affect_from_char(victim, SPELL_WATER_BREATHING);

  act("Small gills spring forth from $n's neck and begin fanning as $e breathes.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("Your neck springs gills and the air around you suddenly seems so dry.", victim, 0, 0, TO_CHAR, 0);

  af.type      = SPELL_WATER_BREATHING;
  af.duration  = level/2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = 0;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

int cast_water_breathing( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_water_breathing(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_POTION:
       return spell_water_breathing(level, ch, ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_water_breathing(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:
       for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = tar_ch->next_in_room)
          spell_water_breathing(level,ch,tar_ch,0, skill);
       break;
    default :
       log("Serious screw-up in water breathing!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

// TODO - make this use and increase in skill
int spell_globe_of_darkness(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  obj_data * globe;

  globe = clone_object(real_object(GLOBE_OF_DARKNESS_OBJECT));

  if(!globe) {
    send_to_char("Major screwup in globe of darkness.  Missing util obj.  Tell coder.\n\r", ch);
    return eFAILURE;
  }

  globe->obj_flags.value[0] = level/5;
  globe->obj_flags.value[1] = 5;

  act("$n's hand blurs and $B$0darkens$R then quickly expands plunging the entire area in lightlessness.",
      ch, 0, 0, TO_ROOM, 0);
  send_to_char("Your summoned $B$0darkness$R turns the area pitch black.\n\r", ch);

  obj_to_room(globe, ch->in_room);
  world[ch->in_room].light -= globe->obj_flags.value[1];

  return eSUCCESS;
}

int cast_globe_of_darkness( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_globe_of_darkness(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_POTION:
       return spell_globe_of_darkness(level, ch, ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_globe_of_darkness(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:
       for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = tar_ch->next_in_room)
          spell_globe_of_darkness(level,ch,tar_ch,0, skill);
       break;
    default :
       log("Serious screw-up in globe_of_darkness!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

int spell_eyes_of_the_eagle(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  send_to_char("This spell doesn't do anything right now.\r\n", ch);
  return eSUCCESS;
  // TODO - I wanted to have this create an eagle that the player posseses and can run around
  // with.  However if I do that, I will enable a player to make himself unsnoopable.  Possibly
  // avoiding logs too.  have to check that before putting in spell...
}

int cast_eyes_of_the_eagle( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_eyes_of_the_eagle(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_POTION:
       return spell_eyes_of_the_eagle(level, ch, ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_STAFF:
       break;
    default :
       log("Serious screw-up in eyes_of_the_eagle!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

int spell_ice_shards(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int save;

  set_cantquit( ch, victim );

  dam = dice(2,level/2);

//  save =saves_spell(ch, victim, (level/2), SAVE_TYPE_COLD);

  // modify the damage by how much they resisted
  //dam += (int) (dam * (save/100));
  
  return spell_damage(ch, victim, dam, TYPE_COLD, SPELL_ICE_SHARDS, 0);
}

int cast_ice_shards( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  char_data * next_v;
  int retval;

  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_ice_shards(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_POTION:
       return spell_ice_shards(level, ch, ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_ice_shards(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:
      for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
      {    
        next_v = tar_ch->next_in_room;
         
        if ( !ARE_GROUPED(ch, tar_ch) )
        {  
          retval = spell_ice_shards(level, ch, tar_ch, 0, skill); 
          if(IS_SET(retval, eCH_DIED))
            return retval;
        }  
      }    
      return eSUCCESS;
      break;
    default :
       log("Serious screw-up in ice_shards!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}


int spell_lightning_shield(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if (!affected_by_spell(victim, SPELL_LIGHTNING_SHIELD))
  {
    act("$n is surrounded by a crackling shield of electrical energy.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    act("You are surrounded by a crackling shield of electrical energy.", victim, 0, 0, TO_CHAR, 0);

    af.type      = SPELL_LIGHTNING_SHIELD;
    af.duration  = 3;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_LIGHTNINGSHIELD;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

int cast_lightning_shield( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_lightning_shield(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_POTION:
       return spell_lightning_shield(level, ch, ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_lightning_shield(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:
       for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = tar_ch->next_in_room)
          spell_lightning_shield(level,ch,tar_ch,0, skill);
       break;
    default :
       log("Serious screw-up in lightning shield!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}


int spell_blue_bird(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int count;
  int retval = eSUCCESS;

  set_cantquit( ch, victim );
  switch(world[ch->in_room].sector_type) {

     case SECT_SWAMP:
     case SECT_FOREST:    count = 3;   break;

     case SECT_HILLS:
     case SECT_MOUNTAIN:
     case SECT_WATER_SWIM:
     case SECT_WATER_NOSWIM:
     case SECT_BEACH:     count = 2;   break;
     
     case SECT_UNDERWATER:
        send_to_char("But you're underwater!  Your poor birdie would drown:(\r\n", ch);
        return eFAILURE;

     default:             count = 1;   break;

  }
  
  while(!SOMEONE_DIED(retval) && count--) {
     dam = number(4, GET_LEVEL(ch) + 3);
     retval = damage(ch, victim, dam, TYPE_PHYSICAL_MAGIC, SPELL_BLUE_BIRD, 0);
  }

  return retval;
}

int cast_blue_bird( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  char_data * next_v;
  int retval;

  switch (type)
  {
    case SPELL_TYPE_SPELL:
       return spell_blue_bird(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_blue_bird(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:   
      for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
      {    
        next_v = tar_ch->next_in_room;
         
        if ( !ARE_GROUPED(ch, tar_ch) )
        {  
          retval = spell_blue_bird(level, ch, tar_ch, 0, skill); 
          if(IS_SET(retval, eCH_DIED))
            return retval;
        }  
      }    
      return eSUCCESS;
      break;
    default :
       log("Serious screw-up in blue_bird!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

int spell_debility(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int retval = eSUCCESS;

  if(affected_by_spell(victim, SPELL_DEBILITY)) {
     send_to_char("Your victim has already been debilitized.\r\n", ch);
     return eSUCCESS;
  }

  set_cantquit( ch, victim );

//  int save = saves_spell(ch, victim, 5, SAVE_TYPE_MAGIC);
  int save = -1;
  if(save < 0)
  {
    int hitmod = skill / 10;       hitmod = MAX(1, hitmod);      hitmod *= -1;
    int  acmod = skill / 3;         acmod = MAX(1,  acmod);

    af.type      = SPELL_DEBILITY;
    af.duration  = 2;
    af.modifier  = hitmod;
    af.location  = APPLY_HITROLL;
    af.bitvector = 0;
    affect_to_char(victim, &af);
    
    af.location = APPLY_AC;
    af.modifier = acmod;  // ac penalty
    affect_to_char(victim, &af);

    send_to_char("Your body becomes $6debilitized$R hurting your abilities.\r\n", victim);
    act("$N's body looks a little more frail.", ch, 0, victim, TO_ROOM, NOTVICT);
  }
  else {
    act("$n just tried to cast something on you and you're sure it isn't good.", ch, 0, victim, TO_VICT, 0);
    send_to_char("Your debility spell is resisted.\r\n", ch);
  }

  if(IS_NPC(victim)) 
  {
     skill_increase_check(ch, SPELL_DEBILITY, skill, SKILL_INCREASE_MEDIUM);

     if(!victim->fighting)
     {
        if(number(0, 5) == 0)
        {
           mob_suprised_sayings(victim, ch);
           retval = attack(victim, ch, 0);
        }
     }
  }

  return retval;
}

int cast_debility( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  char_data * next_v;
  int retval;

  switch (type)
  {
    case SPELL_TYPE_SPELL:
       return spell_debility(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_debility(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:   
      for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
      {    
        next_v = tar_ch->next_in_room;
         
        if ( !ARE_GROUPED(ch, tar_ch) )
        {  
          retval = spell_debility(level, ch, tar_ch, 0, skill); 
          if(IS_SET(retval, eCH_DIED))
            return retval;
        }  
      }    
      return eSUCCESS;
      break;
    default :
       log("Serious screw-up in debility!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

int spell_attrition(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int retval = eSUCCESS;

  if(affected_by_spell(victim, SPELL_ATTRITION)) {
     send_to_char("Your victim is already affected by that spell.\r\n", ch);
     return eSUCCESS;
  }

  set_cantquit( ch, victim );

  int save = -1;//saves_spell(ch, victim, 0, SAVE_TYPE_POISON);

  if(save < 0)
  {
    int  acmod = (skill / 5) + 2;         acmod = MAX(1,  acmod);

    af.type      = SPELL_ATTRITION;
    af.duration  = 2;
    af.modifier  = acmod;
    af.location  = APPLY_AC;
    af.bitvector = 0;
    affect_to_char(victim, &af);
    
    send_to_char("Your body's natural decay rate has been increased!\r\n", victim);
    act("$N's body takes on an unhealthy coloring.", ch, 0, victim, TO_ROOM, NOTVICT);
  }
  else {
    act("$n just tried to cast something on you and you're sure it isn't good.", ch, 0, victim, TO_VICT, 0);
    send_to_char("Your attrition spell is resisted.\r\n", ch);
  }

  if(IS_NPC(victim)) 
  {
     skill_increase_check(ch, SPELL_ATTRITION, skill, SKILL_INCREASE_MEDIUM);

     if(!victim->fighting)
     {
        if(number(0, 5) == 0)
        {
           mob_suprised_sayings(victim, ch);
           retval = attack(victim, ch, 0);
        }
     }
  }

  return retval;
}

int cast_attrition( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  char_data * next_v;
  int retval;

  switch (type)
  {
    case SPELL_TYPE_SPELL:
       return spell_attrition(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_attrition(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:   
      for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = next_v )
      {    
        next_v = tar_ch->next_in_room;
         
        if ( !ARE_GROUPED(ch, tar_ch) )
        {  
          retval = spell_attrition(level, ch, tar_ch, 0, skill); 
          if(IS_SET(retval, eCH_DIED))
            return retval;
        }  
      }    
      return eSUCCESS;
      break;
    default :
       log("Serious screw-up in attrition!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}


int spell_vampiric_aura(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
/*
  if (affected_by_spell(victim, SPELL_ACID_SHIELD)) {
     act("A film of shadow begins to rise around $n but fades around $s ankles.", victim, 0, 0, TO_ROOM, INVIS_NULL);
     send_to_char("A film of shadow tries to rise around you but dissolves in your acid shield.\n\r", ch);
     return eFAILURE;
  }
*/
  if (!affected_by_spell(victim, SPELL_VAMPIRIC_AURA))
  {
    skill_increase_check(ch, SPELL_VAMPIRIC_AURA, skill, SKILL_INCREASE_HARD);
    act("A film of shadow encompasses $n then fades from view.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    act("A film of shadow encompasses you then fades from view.", victim, 0, 0, TO_CHAR, 0);

    af.type      = SPELL_VAMPIRIC_AURA;
    af.duration  = skill / 25;
    af.modifier  = skill;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

int cast_vampiric_aura( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_vampiric_aura(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_POTION:
       return spell_vampiric_aura(level, ch, ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_vampiric_aura(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:
       for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = tar_ch->next_in_room)
          spell_vampiric_aura(level,ch,tar_ch,0, skill);
       break;
    default :
       log("Serious screw-up in vampiric aura!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}


// TODO - make this use skill after skillups can be done for non-practicable skills

int spell_holy_aura(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_HOLY_AURA_TIMER)) {
     send_to_char("Your god is not so foolish as to grant that power to you so soon again.\r\n", ch);
     return eFAILURE;
  }

  skill_increase_check(ch, SPELL_HOLY_AURA, skill, SKILL_INCREASE_HARD);
  act("A serene calm comes over $n.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("A serene calm encompasses you.", victim, 0, 0, TO_CHAR, 0);

  af.type      = SPELL_HOLY_AURA;
  af.duration  = 8;
  af.modifier  = 50;
  af.location  = APPLY_NONE;
  af.bitvector = 0;
  affect_to_char(victim, &af);

  af.type      = SPELL_HOLY_AURA_TIMER;
  af.duration  = 20;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

int cast_holy_aura( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_holy_aura(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_POTION:
       return spell_holy_aura(level, ch, ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_holy_aura(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:
       for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = tar_ch->next_in_room)
          spell_holy_aura(level,ch,tar_ch,0, skill);
       break;
    default :
       log("Serious screw-up in holy aura!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}


int spell_dismiss_familiar(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   victim = NULL;

   for(struct follow_type *k = ch->followers; k; k = k->next)
     if(IS_MOB(k->follower) && IS_AFFECTED2(k->follower, AFF_FAMILIAR))
     {
        victim = k->follower;
        break;
     }

   if (NULL == victim) {
      send_to_char("You don't have a familiar!\n\r", ch);
      return eFAILURE;
   }

   act("$n disappears in a flash of flame and shadow.", victim, 0, 0, TO_ROOM, INVIS_NULL);
   extract_char(victim, TRUE);
	 
   GET_MANA(ch) += 51;
   if(GET_MANA(ch) > GET_MAX_MANA(ch))
      GET_MANA(ch) = GET_MAX_MANA(ch);

   return eSUCCESS;
}

int cast_dismiss_familiar( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_dismiss_familiar(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_POTION:
       return spell_dismiss_familiar(level, ch, ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_dismiss_familiar(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:
       for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = tar_ch->next_in_room)
          spell_dismiss_familiar(level,ch,tar_ch,0, skill);
       break;
    default :
       log("Serious screw-up in dismiss_familiar!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}


int spell_dismiss_corpse(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   victim = NULL;

   for(struct follow_type *k = ch->followers; k; k = k->next)
     if(IS_MOB(k->follower) && affected_by_spell(k->follower, SPELL_CHARM_PERSON))
     {
        victim = k->follower;
        break;
     }

   if (NULL == victim) {
      send_to_char("You don't have a corpse!\n\r", ch);
      return eFAILURE;
   }

   act("$n begins to melt and dissolves into the ground... dust to dust.", victim, 0, 0, TO_ROOM, INVIS_NULL);
   extract_char(victim, TRUE);
	 
   return eSUCCESS;
}

int cast_dismiss_corpse( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_dismiss_corpse(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_POTION:
       return spell_dismiss_corpse(level, ch, ch, 0, skill);
       break;
    case SPELL_TYPE_SCROLL:
       if(tar_obj)
          return eFAILURE;
       if(!tar_ch) tar_ch = ch;
          return spell_dismiss_corpse(level, ch, tar_ch, 0, skill);
       break;
    case SPELL_TYPE_STAFF:
       for (tar_ch = world[ch->in_room].people ; tar_ch ; tar_ch = tar_ch->next_in_room)
          spell_dismiss_corpse(level,ch,tar_ch,0, skill);
       break;
    default :
       log("Serious screw-up in dismiss_corpse!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

int spell_visage_of_hate(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
   
  if(!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("You have no group to cast this upon.\r\n", ch);
    return eFAILURE;
  }

  for(char_data * tmp_char = world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
  {
    if(tmp_char == ch)
      continue;
    if(!ARE_GROUPED(ch, tmp_char))
      continue;
    affect_from_char(tmp_char, SPELL_VISAGE_OF_HATE);
    affect_from_char(tmp_char, SPELL_VISAGE_OF_HATE);
    send_to_char("The violence hate brings shows upon your face.\r\n", tmp_char);
   
    af.type      = SPELL_VISAGE_OF_HATE;
    af.duration  = 1 + skill / 10;
    af.modifier  = -1;
    af.location  = APPLY_HITROLL;
    af.bitvector = 0;
    affect_to_char(tmp_char, &af);
    af.modifier  = 2;
    af.location  = APPLY_DAMROLL;
    affect_to_char(tmp_char, &af);
  }

  send_to_char("Your disdain and hate for all settles upon your peers.\r\n", ch);  
  skill_increase_check(ch, SPELL_VISAGE_OF_HATE, skill, SKILL_INCREASE_EASY);
  return eSUCCESS;  
}

int cast_visage_of_hate( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_visage_of_hate(level, ch, tar_ch, 0, skill);
       break;
    default :
       log("Serious screw-up in visage_of_hate!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

int spell_blessed_halo(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
   
  if(!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("You have no group to cast this upon.\r\n", ch);
    return eFAILURE;
  }

  for(char_data * tmp_char = world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
  {
    if(tmp_char == ch)
      continue;
    if(!ARE_GROUPED(ch, tmp_char))
      continue;
    affect_from_char(tmp_char, SPELL_BLESSED_HALO);
    affect_from_char(tmp_char, SPELL_BLESSED_HALO);
    send_to_char("You feel blessed.\r\n", tmp_char);

    af.type      = SPELL_BLESSED_HALO;
    af.duration  = 1 + skill / 10;
    af.modifier  = 3;
    af.location  = APPLY_HITROLL;
    af.bitvector = 0;
    affect_to_char(tmp_char, &af);
    af.modifier  = 1;
    af.location  = APPLY_HP_REGEN;
    affect_to_char(tmp_char, &af);
  }

  send_to_char("Your group feels blessed.\r\n", ch);  
  skill_increase_check(ch, SPELL_BLESSED_HALO, skill, SKILL_INCREASE_EASY);
  return eSUCCESS;  
}

int cast_blessed_halo( byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_blessed_halo(level, ch, tar_ch, 0, skill);
       break;
    default :
       log("Serious screw-up in blessed_halo!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

