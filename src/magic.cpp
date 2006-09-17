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

// Notes:  (March 14th, 2005)
// - Cleaned old notations and removed code & effects from several places.
// - Saved any code that was either useful for the future or of quetionable origin.
// - Where is Spellcraft "dual target" affect for burning hands?
// - Where is Spellcraft "reduced fireball lag" affect listed?
//    - Apoc.


extern "C"
{
#include <string.h>
#include <stdio.h>
#include <assert.h>
  #include <stdlib.h>
#include <math.h> // pow(double,double)
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <room.h>
#include <connect.h>
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


/* Extern Structures */

extern CWorld world;
extern struct room_data ** world_array;
 
extern struct obj_data  *object_list;
extern CHAR_DATA *character_list;
extern struct zone_data *zone_table;

#define BEACON_OBJ_NUMBER 405

extern struct spell_info_type spell_info [ ];
extern bool str_prefix(const char *astr, const char *bstr);


/* Extern Procedures */

int saves_spell(CHAR_DATA *ch, CHAR_DATA *vict, int spell_base, int16 save_type);
struct clan_data * get_clan(struct char_data *);

int dice(int number, int size);
void update_pos( CHAR_DATA *victim );
bool many_charms(CHAR_DATA *ch);
bool ARE_GROUPED( CHAR_DATA *sub, CHAR_DATA *obj);
void add_memory(CHAR_DATA *ch, char *victim, char type);
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct race_shit race_info[];

bool malediction_res(CHAR_DATA *ch, CHAR_DATA *victim, int spell)
{
  int res = 0;
  switch (spell)
  {
    case SPELL_CURSE: res = SAVE_TYPE_MAGIC; break;
    case SPELL_WEAKEN: res = SAVE_TYPE_MAGIC; break;
    case SPELL_BLINDNESS: res = SAVE_TYPE_MAGIC; break;
    case SPELL_POISON: res = SAVE_TYPE_POISON; break;
    case SPELL_ATTRITION: res = SAVE_TYPE_POISON; break;
    case SPELL_DEBILITY: res = SAVE_TYPE_POISON; break;
    case SPELL_FEAR: res = SAVE_TYPE_COLD; break;
    case SPELL_PARALYZE: res = SAVE_TYPE_MAGIC; break;
  }
  res = victim->saves[res] + 5 + (100-has_skill(ch, spell))/2;
  if (number(1,101) < res) return TRUE;
  return FALSE;
}

bool can_heal(CHAR_DATA *ch, CHAR_DATA *victim, int spellnum)
{
  bool can_cast = TRUE;

  // You cannot heal an elemental from "conjure elemental"
  if (IS_NPC(victim) &&
      (mob_index[victim->mobdata->nr].virt == 88 ||
       mob_index[victim->mobdata->nr].virt == 89 ||
       mob_index[victim->mobdata->nr].virt == 90 ||
       mob_index[victim->mobdata->nr].virt == 91)) {
    send_to_char("The heavy magics surrounding this being prevent healing.\r\n",ch);
    return FALSE;
  }

  if (GET_HIT(victim) > GET_MAX_HIT(victim)-10)
  {
    if (spellnum != SPELL_CURE_LIGHT)
       can_cast = FALSE;
  } else if (GET_HIT(victim) > GET_MAX_HIT(victim) - 25)
  {
     if (spellnum != SPELL_CURE_LIGHT && spellnum != SPELL_CURE_SERIOUS)
        can_cast = FALSE;
  } else if (GET_HIT(victim) > GET_MAX_HIT(victim) -50)
  {
     if (spellnum != SPELL_CURE_LIGHT && spellnum != SPELL_CURE_SERIOUS &&
           spellnum != SPELL_CURE_CRITIC)
         can_cast = FALSE;
  }

//  if (GET_HIT(victim) > GET_MAX_HIT(victim)-5)
//    can_cast = FALSE;

  if (!can_cast)
  {
    if (ch == victim)
      send_to_char("You have not received enough damage to warrant this powerful incantation.\r\n",ch);
    else
      act("$N has not received enough damage to warrant this powerful incantation.", ch, 0, victim, TO_CHAR, 0);
  }
  return can_cast;
}

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


/* ------------------------------------------------------- */
/* Begin Spells Listings                                   */
/* ------------------------------------------------------- */

/* MAGIC MISSILE */

int spell_magic_missile(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int count = 1;
  int retval = eSUCCESS;

  set_cantquit( ch, victim );
  dam = 50;
    count += (skill > 15) + (skill > 35) + (skill > 55) + (skill > 75); 

	/* Spellcraft Effect */
    if (spellcraft(ch, SPELL_MAGIC_MISSILE)) count++;
    while(!SOMEONE_DIED(retval) && count--)
    retval = damage(ch, victim, dam, TYPE_MAGIC, SPELL_MAGIC_MISSILE, 0);
  return retval;
}


/* CHILL TOUCH */

int spell_chill_touch(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int dam = 250;
  int save;
  int retval = damage(ch, victim, dam, TYPE_COLD, SPELL_CHILL_TOUCH, 0);

  bool hasSpellcraft = spellcraft(ch, SPELL_CHILL_TOUCH);

  if(SOMEONE_DIED(retval)) return retval;
  if(IS_SET(retval,eEXTRA_VAL2)) victim = ch;
  if(IS_SET(retval,eEXTRA_VALUE)) return retval;

  save = saves_spell(ch, victim, (level/2), SAVE_TYPE_COLD);
  if(save < 0 && skill > 50) // if failed
   {
    af.type      = SPELL_CHILL_TOUCH;
    af.duration  = skill/18;
    af.modifier  = - skill/18;
    af.location  = APPLY_STR;
    af.bitvector = -1;
    affect_join(victim, &af, TRUE, FALSE);

	/* Spellcraft Effect */
    if (hasSpellcraft)
         {
	  af.modifier = - skill/18;
	  af.location = APPLY_DEX;
	  affect_join(victim, &af, TRUE, FALSE);
	 }
   } 
  return retval;
}


/* BURNING HANDS */

int spell_burning_hands(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  set_cantquit( ch, victim );
  dam = 150;
  return damage(ch, victim, dam, TYPE_FIRE, SPELL_BURNING_HANDS, 0);
}


/* SHOCKING GRASP */

int spell_shocking_grasp(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  dam = 130;
  return damage(ch, victim, dam, TYPE_ENERGY, SPELL_SHOCKING_GRASP, 0);
}


/* LIGHTNING BOLT */

int spell_lightning_bolt(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  set_cantquit( ch, victim );
  dam = 200;
  return damage(ch, victim, dam, TYPE_ENERGY, SPELL_LIGHTNING_BOLT, 0);
}


/* COLOUR SPRAY */

int spell_colour_spray(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;
   set_cantquit( ch, victim );
   dam = 350;
   int retval = damage(ch, victim, dam, TYPE_MAGIC, SPELL_COLOUR_SPRAY, 0);

   if(SOMEONE_DIED(retval))
     return retval;

   if(IS_SET(retval,eEXTRA_VAL2)) victim = ch;
   if(IS_SET(retval,eEXTRA_VALUE)) return retval;
	/*  Dazzle Effect */
   if(number(1,101) > get_saves(victim, SAVE_TYPE_MAGIC) + 40 && (skill > 50 || IS_NPC(ch)) ) {
     act("$N blinks in confusion from the distraction of the colour spray.", ch, 0, victim, TO_ROOM, NOTVICT);
     act("Brilliant streams of colour streak from $n's fingers!  WHOA!  Cool!", ch, 0, victim, TO_VICT, 0 );
     act("Your colours of brilliance dazzle the simpleminded $N.", ch, 0, victim, TO_CHAR, 0 );
     SET_BIT(victim->combat, COMBAT_SHOCKED2);
   }
   return retval;
}


/* DROWN */

int spell_drown(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   char buf[256];
   int dam, retval;

	/* Does not work in Desert or Underwater */
   if(world[ch->in_room].sector_type == SECT_DESERT) {
     send_to_char("You're trying to drown someone in the desert?  Get a clue!\r\n", ch);
     return eFAILURE;
   }
   if(world[ch->in_room].sector_type == SECT_UNDERWATER) {
     send_to_char("Hello!  You're underwater!  *knock knock*  Anyone home?\r\n", ch);
     return eFAILURE;
   }

   set_cantquit( ch, victim );
   dam = 250;
   retval = damage(ch, victim, dam, TYPE_WATER, SPELL_DROWN, 0);
   if(SOMEONE_DIED(retval))
     return retval;

   if(IS_SET(retval,eEXTRA_VAL2)) victim = ch;
   if(IS_SET(retval,eEXTRA_VALUE)) return retval;
	/* Drown BINGO Effect */
   if (skill > 80) {
     if (number(1, 100) == 1 && GET_LEVEL(victim) < IMMORTAL) {
        dam = GET_HIT(victim)*5 + 20;
        sprintf(buf, "You are torn apart by the force of %s's watery blast and are killed instantly!", GET_NAME(ch));
        send_to_char(buf, victim);
        act("$N is torn apart by the force of $n's watery blast and killed instantly!", ch, 0, victim, TO_ROOM, NOTVICT);
        act("$N is torn apart by the force of your watery blast and killed instantly!", ch, 0, victim, TO_CHAR, 0);
        return damage(ch, victim, dam, TYPE_COLD, SPELL_DROWN, 0);
     }
   }
   return eSUCCESS;
}


/* ENERGY DRAIN */
// Drains XP, MANA, HP - caster gains HP and MANA -- Currently MOB ONLY

int spell_energy_drain(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int mult = GET_EXP(victim) / 20;
   if(IS_SET(world[ch->in_room].room_flags, SAFE))
     return eFAILURE;

   set_cantquit( ch, victim );
   if(saves_spell(ch, victim, 1, SAVE_TYPE_MAGIC) > 0) 
      mult /=2;

   gain_exp(victim, 0-mult);
   GET_HIT(victim) -= GET_HIT(victim) /20;
   send_to_char("Your knees buckle as life force is drained from your body!\n\rYou have lost some experience!\n\r", victim);
   return eSUCCESS;
}

/* SOULDRAIN */
// Drains MANA - caster gains MANA

int spell_souldrain(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int mana;
   set_cantquit( ch, victim );

   if (IS_SET(world[ch->in_room].room_flags, SAFE))
      {
      send_to_char("You cannot do this in a safe room!\r\n", ch);
      return eFAILURE;
      }
   mana = dam_percent(skill,125);
   if(mana > GET_MANA(victim))
   mana = GET_MANA(victim);
   if (number(1,101) < get_saves(victim, SAVE_TYPE_MAGIC)+40)

   {
	act("$N resists your attempt to souldrain $M!", ch, NULL, victim,TO_CHAR,0);
	act("$N resists $n's attempt to souldrain $M!", ch, NULL, victim, TO_ROOM,NOTVICT);
	act("You resist $n's attempt to souldrain you!",ch,NULL,victim,TO_VICT,0);
        int retval;
        if (IS_MOB(victim) && !victim->fighting)
	 {
          retval = attack(victim, ch, TYPE_UNDEFINED, FIRST);
          retval = SWAP_CH_VICT(retval);
         }
        else retval = eFAILURE;
        return retval;
   }

   GET_MANA(victim) -= mana;
   GET_MANA(ch) += mana;
   char buf[MAX_STRING_LENGTH];
   sprintf(buf, "$B%d$R", mana);
   send_damage("You drain $N's very soul for | mana!", ch, 0, victim, buf, "You drain $N's very soul!", TO_CHAR);
   send_damage("You feel your very soul being drained by $n for | mana!", ch, 0, victim, buf, "You feel your very soul being drained by $n!", TO_VICT);
   send_damage("$N's soul is drained away by $n for | mana!", ch, 0, victim, buf, "$N's soul is drained away by $n!", TO_ROOM);

   if (GET_MANA(ch) > GET_MAX_MANA(ch)) GET_MANA(ch) = GET_MAX_MANA(ch);

   int retval;
   if (IS_MOB(victim) && !victim->fighting)
	{
         retval = attack(victim, ch, TYPE_UNDEFINED, FIRST);
	 retval = SWAP_CH_VICT(retval);
	}
        else retval = eSUCCESS;
   return retval; 
}


/* VAMPIRIC TOUCH */

int spell_vampiric_touch (ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  set_cantquit( ch, victim );
  dam = 225;
  int adam = dam_percent(skill, 225); // Actual damage, for drainy purposes.

  if(saves_spell(ch, victim, ( skill / 3 ), SAVE_TYPE_COLD) >= 0) {
    dam >>= 1;
    adam /= 2;
  }

  int i = GET_HIT(victim);
  int retval =  damage (ch, victim, dam,TYPE_COLD, SPELL_VAMPIRIC_TOUCH, 0);
  if (!SOMEONE_DIED(retval) && GET_HIT(victim) >= i) return retval;
      
  if (!SOMEONE_DIED(retval))
    GET_HIT(ch) += MIN(adam, i-GET_HIT(victim));
  else
    GET_HIT(ch) += MIN(adam, i);
  if (GET_HIT(ch) > GET_MAX_HIT(ch))
    GET_HIT(ch) = GET_MAX_HIT(ch);

   return retval;
}


/* METEOR SWARM */

int spell_meteor_swarm(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  set_cantquit( ch, victim );
  dam = 500;
  int retval;
  retval = damage(ch, victim, dam,TYPE_PHYSICAL_MAGIC, SPELL_METEOR_SWARM, 0);

	/* Spellcraft Effect */
  if (!SOMEONE_DIED(retval) && spellcraft(ch, SPELL_METEOR_SWARM)
	&& !number(0,9))
   {
        if(IS_SET(retval,eEXTRA_VAL2)) victim = ch;
        if(IS_SET(retval,eEXTRA_VALUE)) return retval;
        act("The force of the spell knocks $N over!",ch,0,victim, TO_CHAR, 0);
	send_to_char("The force of the spell knocks you over!\r\n",victim);
	GET_POS(victim) = POSITION_SITTING;
   }
 return retval;
}


/* FIREBALL */

int spell_fireball(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;
   set_cantquit( ch, victim );
   dam = 300;
   int retval = damage(ch, victim, dam, TYPE_FIRE, SPELL_FIREBALL, 0);

   if(SOMEONE_DIED(retval) || ch->in_room != victim->in_room)
     return retval;
   // Above: Fleeing now saves you from the second blast.

   if(IS_SET(retval,eEXTRA_VAL2)) victim = ch;
   if(IS_SET(retval,eEXTRA_VALUE)) return retval;

	/* Fireball Recombining Effect */
   if (has_skill(ch, SPELL_FIREBALL) > 80)
   if(number(0, 100) < ( skill / 5 ) ) {
     act("The expanding $B$4flames$R suddenly recombine and fly at $N again!", ch, 0, victim, TO_ROOM, 0);
     act("The expanding $B$4flames$R suddenly recombine and fly at $N again!", ch, 0, victim, TO_CHAR, 0);
     retval = damage(ch, victim, dam, TYPE_FIRE, SPELL_FIREBALL, 0);
   }
   return retval;
}


/* SPARKS */

int spell_sparks(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;
   set_cantquit( ch, victim );
   dam = dice(level, 6);
   return damage(ch, victim, dam, TYPE_FIRE, SPELL_SPARKS, 0);
}


/* HOWL */

int spell_howl(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   char_data * tmp_char;
   int retval;
   set_cantquit( ch, victim );

   if(saves_spell(ch, victim, 5, SAVE_TYPE_MAGIC) >= 0)
    {
      return damage(ch, victim, 0, TYPE_SONG, SPELL_HOWL, 0);
    }
   retval = damage(ch, victim, 300, TYPE_SONG, SPELL_HOWL, 0);

   if(SOMEONE_DIED(retval))
     return retval;

   if(IS_SET(retval,eEXTRA_VAL2)) victim = ch;
   if(IS_SET(retval,eEXTRA_VALUE)) return retval;

   for (tmp_char = world[ch->in_room].people; tmp_char;
        tmp_char = tmp_char->next_in_room)
    {
      
      if (!tmp_char->master)
         continue;
	 
      if (tmp_char->master == ch || ARE_GROUPED(tmp_char->master, ch))
         continue;
	 
      if (affected_by_spell(tmp_char, SPELL_CHARM_PERSON))
         {
          affect_from_char(tmp_char, SPELL_CHARM_PERSON);
          send_to_char("You feel less enthused about your master.\n\r",
	              tmp_char);
          act("$N blinks and shakes its head, clearing its thoughts.",
                 ch, 0, tmp_char, TO_CHAR, 0);
          act("$N blinks and shakes its head, clearing its thoughts.\n\r",
 	 	 ch, 0, tmp_char, TO_ROOM, NOTVICT);

	  if (tmp_char->fighting)
	    {
	     do_say(tmp_char,"Screw this! I'm going home!", 9);
	     if (tmp_char->fighting->fighting == tmp_char)
	         stop_fighting(tmp_char->fighting);
	         stop_fighting(tmp_char);
	    }
         }
    } // for loop through people in the room
  return retval;
}


/* HOLY AEGIS/UNHOLY AEGIS*/

int spell_aegis(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int spl = GET_CLASS(ch) == CLASS_ANTI_PAL ? SPELL_U_AEGIS:SPELL_AEGIS;
  if(affected_by_spell(ch, spl))
    affect_from_char(ch, spl);
  if(affected_by_spell(ch, SPELL_ARMOR))
  {
    act("$n is already protected by magical armour.", ch, 0, 0, TO_CHAR, 0);
    return eFAILURE;
  }

  af.type      = spl;
  af.duration  =  10 + skill / 4;
  af.modifier  = -10 - skill / 4;
  af.location  = APPLY_AC;
  af.bitvector = -1;

  affect_to_char(ch, &af);
  if (GET_CLASS(ch) == CLASS_PALADIN)
    send_to_char("You invoke your protective aegis.\n\r", ch);
  else
    send_to_char("You invoke your unholy aegis.\r\n",ch);
  return eSUCCESS;
}

/* ARMOUR */

int spell_armor(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  if(affected_by_spell(victim, SPELL_ARMOR))
    affect_from_char(victim, SPELL_ARMOR);
  if(affected_by_spell(victim, SPELL_AEGIS) || affected_by_spell(victim, SPELL_U_AEGIS))
  {
    act("$n is already protected by magical armour.",victim, 0, ch, TO_VICT, 0);
    return eFAILURE;
  }

  af.type      = SPELL_ARMOR;
  af.duration  =  10 + skill / 3;
  af.modifier  = -10 - skill / 3;
  af.location  = APPLY_AC;
  af.bitvector = -1;

  affect_to_char(victim, &af);
  send_to_char("You feel a magical armour surround you.\n\r", victim);
  return eSUCCESS;
}


/* STONE SHIELD */

int spell_stone_shield(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  char buf[160];

  if(affected_by_spell(victim, SPELL_GREATER_STONE_SHIELD))
  {
    sprintf(buf, "%s is already surrounded by a greater stoneshield.\r\n", GET_SHORT(victim));
    send_to_char(buf, ch);
    return eSUCCESS;
  }

  if(affected_by_spell(victim, SPELL_STONE_SHIELD))
     affect_from_char(victim, SPELL_STONE_SHIELD);

  af.type      = SPELL_STONE_SHIELD;
  af.duration  = 5 + (skill / 10) + (GET_WIS(ch) > 20);
  af.modifier  = -15 - skill / 5;
  af.location  = 0;
  af.bitvector = -1;

  affect_to_char(victim, &af);
  send_to_char("A shield of ethereal stones begins to swirl around you.\n\r", victim);
  act("Ethereal stones form out of nothing and begin to swirl around $n.", 
    victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

int cast_stone_shield( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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

int cast_iridescent_aura( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	case SPELL_TYPE_SPELL:
		 return spell_iridescent_aura(level,ch,tar_ch,0, skill);
		 break;
	case SPELL_TYPE_POTION:
		 return spell_iridescent_aura(level,ch,ch,0, skill);
		 break;
	case SPELL_TYPE_SCROLL:
		 if (tar_obj) return eFAILURE;
		 if (!tar_ch) tar_ch = ch;
		 return spell_iridescent_aura(level,ch,ch,0, skill);
		 break;
	case SPELL_TYPE_WAND:
		 if (tar_obj) return eFAILURE;
		 if (!tar_ch) tar_ch = ch;
		 return spell_iridescent_aura(level,ch,tar_ch,0, skill);
		 break;
		default :
	 log("Serious screw-up in iridesent_aura!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


/* GREATER STONE SHIELD */

int spell_greater_stone_shield(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  char buf[160];
  if(affected_by_spell(victim, SPELL_STONE_SHIELD) || affected_by_spell(victim, SPELL_GREATER_STONE_SHIELD))
  {
    sprintf(buf, "%s is already surrounded by a stone shield.\r\n", GET_SHORT(victim));
    send_to_char(buf, ch);
    return eSUCCESS;
  }

  af.type      = SPELL_GREATER_STONE_SHIELD;
  af.duration  = 5 + (skill / 6) + (GET_WIS(ch) > 20);
  af.modifier  = - 20 - skill / 4;
  af.location  = 0;
  af.bitvector = -1;

  affect_to_char(victim, &af);
  send_to_char("A shield of ethereal stones begins to swirl around you.\n\r", victim);
  act("Ethereal stones form out of nothing and begin to swirl around $n.", 
    victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

int cast_greater_stone_shield( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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


/* EARTHQUAKE */

int spell_earthquake(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int retval = 0;
  CHAR_DATA *tmp_victim, *temp;
  dam = 150;
  send_to_char("The earth trembles beneath your feet!\n\r", ch);
  act("$n makes the earth tremble and shiver.\n\r",
		ch, 0, 0, TO_ROOM, 0);

  for(tmp_victim = character_list; (tmp_victim && !IS_SET(retval, eCH_DIED)); tmp_victim = temp)
  {
	 temp = tmp_victim->next;
	 if ( (ch->in_room == tmp_victim->in_room) 
           && (ch != tmp_victim) 
	   && (!ARE_GROUPED(ch,tmp_victim))
	   && can_be_attacked(ch, tmp_victim))
         {
                  if(IS_NPC(ch) && IS_NPC(tmp_victim)) // mobs don't earthquake each other
                    continue;
                  if(IS_AFFECTED(tmp_victim, AFF_FREEFLOAT) || IS_AFFECTED(tmp_victim, AFF_FLYING))
                     send_to_char("You float over the shaking ground.\n\r", tmp_victim);
                  else retval = damage(ch, tmp_victim, dam, TYPE_MAGIC, SPELL_EARTHQUAKE, 0);
	 } 
         else if (world[ch->in_room].zone == world[tmp_victim->in_room].zone)
           send_to_char("The earth trembles and shivers.\n\r", tmp_victim);
  }
  return eSUCCESS;
}


/* LIFE LEECH */

int spell_life_leech(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam,retval = eSUCCESS;
  CHAR_DATA *tmp_victim, *temp;

  if(IS_SET(world[ch->in_room].room_flags, SAFE)) 
    return eFAILURE;
/*  double o = 0.0, m = 0.0, avglevel = 0.0;
  for (tmp_victim = world[ch->in_room].people;tmp_victim;tmp_victim = tmp_victim->next_in_room)
    if (!ARE_GROUPED(ch, tmp_victim) && ch != tmp_victim)
     { o++; m++; avglevel *= o-1; avglevel += GET_LEVEL(tmp_victim); avglevel /= o;}
    else m++;
  m--; // don't count player
  avglevel -= (double)GET_LEVEL(ch);
  double powmod = 0.2;
  powmod -= (avglevel*0.001);
  powmod -= (has_skill(ch, SPELL_LIFE_LEECH) * 0.001);
  int max = (int)(o * 50 * ( m / pow(m, powmod*m)));
  max += number(-10,10);
*/
  for(tmp_victim = world[ch->in_room].people;tmp_victim;tmp_victim = temp)
  {
	 temp = tmp_victim->next_in_room;
	 if ( (ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
		(!ARE_GROUPED(ch,tmp_victim)) && can_be_attacked(ch, tmp_victim))
	{
//		dam = max / o;
                dam = 150;
		int adam = dam_percent(skill, dam);
                if (IS_SET(tmp_victim->immune, ISR_POISON))
		  adam = 0;

		 if (GET_HIT(tmp_victim) < adam)
		  GET_HIT(ch) += (int)(GET_HIT(tmp_victim) * 0.3);
		 else GET_HIT(ch) += (int)(adam * 0.3);

		 if (GET_HIT(ch) > GET_MAX_HIT(ch))
		  GET_HIT(ch) = GET_MAX_HIT(ch);
		 retval &= damage (ch, tmp_victim, dam,TYPE_POISON, SPELL_LIFE_LEECH, 0);
	}
  }
  return retval;
}


/* SOLAR GATE BLIND EFFECT (Spellcraft Effect) */

void do_solar_blind(CHAR_DATA *ch, CHAR_DATA *tmp_victim)
{
  struct affected_type af;
  if(!ch || !tmp_victim)
  {
    log("Null ch or vict in solar_blind", IMMORTAL, LOG_BUG);
    return;
  }
  if (tmp_victim->in_room != ch->in_room) return;
  if (has_skill(ch, SPELL_SOLAR_GATE) < 81) return;
   if (number(0,4)) return;
    if(!IS_AFFECTED(tmp_victim, AFF_BLIND)) 
    {
      act("$n seems to be blinded!", tmp_victim, 0, 0, TO_ROOM, INVIS_NULL);
      send_to_char("The world dims and goes $B$0black$R as you are blinded!\n\r", tmp_victim);

      af.type      = SPELL_BLINDNESS;
      af.location  = APPLY_HITROLL;
      af.modifier  = has_skill(tmp_victim,SKILL_BLINDFIGHTING)?skill_success(tmp_victim,0,SKILL_BLINDFIGHTING)?-10:-20:-20;  
	// Make hitroll worse
      af.duration  = 2;
      af.bitvector = AFF_BLIND;
      affect_to_char(tmp_victim, &af);
      af.location = APPLY_AC;
      af.modifier  = has_skill(tmp_victim,SKILL_BLINDFIGHTING)?skill_success(tmp_victim,0,SKILL_BLINDFIGHTING)?+45:90:90;
      affect_to_char(tmp_victim, &af);
    } 	// if affect by blind
}


/* SOLAR GATE */

int spell_solar_gate(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

	  // do room caster is in
  send_to_char("A Bright light comes down from the heavens.\n\r", ch);
  act("$n opens a Solar Gate.\n\r", ch, 0, 0, TO_ROOM, 0);

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

       dam = 600;
       retval = damage(ch, tmp_victim, dam,TYPE_FIRE, SPELL_SOLAR_GATE, 0);
       if(IS_SET(retval, eCH_DIED))
	 return retval;
       if(IS_SET(retval,eEXTRA_VALUE)) return retval;
       if(!IS_SET(retval, eVICT_DIED) && spellcraft(ch, SPELL_SOLAR_GATE) && !IS_SET(retval, eEXTRA_VAL2))
         do_solar_blind(ch, tmp_victim);
     }  // if are grouped, etc
  } 	// for

  	// do surrounding rooms

  for(i = 0; i < 6; i++) {
    if(CAN_GO(ch, i)) {
      for(tmp_victim = world[world[orig_room].dir_option[i]->to_room].people;
           tmp_victim; tmp_victim = temp) 
      {
          temp = tmp_victim->next_in_room;
	  if (IS_NPC(tmp_victim) && mob_index[tmp_victim->mobdata->nr].virt >= 2300 &&
		mob_index[tmp_victim->mobdata->nr].virt <=2399)
	  {
		send_to_char("The clan hall's enchantments absorbs part of your spell.\r\n",ch);
		continue;
	  }
	  if((tmp_victim != ch) && (tmp_victim->in_room != orig_room) &&
             (!ARE_GROUPED(ch, tmp_victim)) &&
	     (can_be_attacked(tmp_victim, tmp_victim)))
          {
	   char buf[MAX_STRING_LENGTH];
            dam = 300; //dice(level, 10) + skill/2;
	    sprintf(buf,"You are ENVELOPED in a PAINFUL BRIGHT LIGHT pouring in %s.",dirs[i]);
	    act(buf, tmp_victim, 0, ch, TO_CHAR, 0);

            retval = damage(ch, tmp_victim, dam, TYPE_FIRE, SPELL_SOLAR_GATE, 0);
            if(IS_SET(retval, eCH_DIED))
              return retval;

	    if (IS_SET(retval, eVICT_DIED))
		if (ch->desc && ch->pcdata && !IS_SET(ch->pcdata->toggles, PLR_WIMPY))
		  ch->desc->wait = 0;
            if(!IS_SET(retval, eVICT_DIED))
            {
              // don't blind surrounding rooms
              // do_solar_blind(ch, tmp_victim);
              if(GET_LEVEL(tmp_victim))
                if(IS_NPC(tmp_victim))  {
                  add_memory(tmp_victim, GET_NAME(ch), 'h');
  if (!IS_NPC(ch) && IS_NPC(tmp_victim))
     if (!ISSET(tmp_victim->mobdata->actflags, ACT_STUPID) && !tmp_victim->hunting) 
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
                     !ISSET(tmp_victim->mobdata->actflags, ACT_STUPID)&&
  		     !tmp_victim->fighting)
		    do_move(tmp_victim, "", to_charge[i]);
	        }
            } // if ! eVICT_DIED
          } // if are grouped, etc
       } // for tmp victim
     }  // if can go
  } // for i 0 < 6
  return eSUCCESS;
}


/* GROUP RECALL */

int spell_group_recall(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  CHAR_DATA *tmp_victim, *temp;
  int chance;
  int learned = has_skill(ch, SPELL_GROUP_RECALL);

  if (learned < 40) {
    chance = 5;
  } else if (learned > 40 && learned <= 60) {
    chance = 4;
  } else if (learned > 60 && learned <= 80) {
    chance = 3;
  } else if (learned > 80 && learned <= 90) {
    chance = 2;
  } else if (learned > 90) {
    chance = 1;
  }

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
      if(number(1, 101) > chance)
        spell_word_of_recall(level, ch, tmp_victim, obj, 110);
      else {
        csendf(tmp_victim, "%s's group recall partially fails leaving you behind!\r\n", ch->name);
        csendf(ch, "Your group recall partially fails leaving %s behind!\r\n", tmp_victim->name);
      }
    }
  }
  return spell_word_of_recall(level, ch, ch, obj, skill);
}


/* GROUP FLY */

int spell_group_fly(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  CHAR_DATA *tmp_victim, *temp;

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


/* HEROES FEAST */

int spell_heroes_feast(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  CHAR_DATA *tmp_victim, *temp;
  int result = 15 + skill / 6;

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


/* GROUP SANCTUARY */

int spell_group_sanc(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  CHAR_DATA *tmp_victim, *temp;

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


/* HEAL SPRAY */

int spell_heal_spray(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
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


/* FIRESTORM */

int spell_firestorm(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int retval = eSUCCESS;
  CHAR_DATA *tmp_victim, *temp;

  send_to_char("$B$4Fire$R falls from the heavens!\n\r", ch);
  act("$n makes $B$4fire$R fall from the heavens!\n\r",
		ch, 0, 0, TO_ROOM, 0);

  for(tmp_victim = character_list; tmp_victim; tmp_victim = temp)
  {
	 temp = tmp_victim->next;
	 if ( (ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
		(!ARE_GROUPED(ch,tmp_victim)) && can_be_attacked(ch, tmp_victim)){

	  dam = 250;
	  retval = damage(ch, tmp_victim, dam,TYPE_FIRE, SPELL_FIRESTORM, 0);
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


/* DISPEL EVIL */

int spell_dispel_evil(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam, align;
  set_cantquit( ch, victim );
  if (IS_EVIL(ch))
	 victim = ch;
  if (IS_NEUTRAL(victim) || IS_GOOD(victim))
  {
	act("$N does not seem to be affected.", ch, 0, victim, TO_CHAR, 0);
	return eFAILURE;
  }
  align = GET_ALIGNMENT(victim);
  if(align < 0) align = 0-align;
  dam = 350 + align / 10;

  return damage(ch, victim, dam, TYPE_COLD, SPELL_DISPEL_EVIL, 0);
}


/* DISPEL GOOD */

int spell_dispel_good(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam, align;
  set_cantquit( ch, victim );
  if (IS_GOOD(ch))
	 victim = ch;
  if (IS_NEUTRAL(victim) || IS_EVIL(victim))
  {
	act("$N does not seem to be affected.", ch, 0, victim, TO_CHAR, 0);
	return eFAILURE;
  }
  align = GET_ALIGNMENT(victim);
  if(align < 0) align = 0-align;
  dam = 350 + align / 10;

  return damage(ch, victim, dam, TYPE_COLD, SPELL_DISPEL_GOOD, 0);
}


/* CALL LIGHTNING */

int spell_call_lightning(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  extern struct weather_data weather_info;
  set_cantquit( ch, victim );

  if (OUTSIDE(ch) && (weather_info.sky>=SKY_RAINING)) 
  {
     dam = dice(MIN((int)GET_MANA(ch),725), 1);
     return damage(ch, victim, dam,TYPE_ENERGY, SPELL_CALL_LIGHTNING, 0);
  }
  return eFAILURE;
}


/* HARM */

int spell_harm(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  set_cantquit( ch, victim );
  dam = 150;

  return damage(ch, victim, dam, TYPE_MAGIC, SPELL_HARM, 0);
}


/* POWER HARM */

int spell_power_harm(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  set_cantquit( ch, victim );

  if(IS_EVIL(ch))
     dam = 500;
  else if(IS_NEUTRAL(ch))
     dam = 400;
  else
     dam = 300;

  return damage(ch, victim, dam, TYPE_MAGIC, SPELL_POWER_HARM, 0);
}


/* DIVINE FURY */

int spell_divine_fury(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  set_cantquit( ch, victim );
  dam = 100 + GET_ALIGNMENT(ch) / 4;
  extern void zap_eq_check(char_data *ch);

  if(IS_GOOD(victim)) GET_ALIGNMENT(ch) -= 5;
  if(IS_NEUTRAL(victim)) GET_ALIGNMENT(ch) -= 2;
  zap_eq_check(ch);
  return damage(ch, victim, dam, TYPE_MAGIC, SPELL_DIVINE_FURY, 0);
}


/* TELEPORT */

	// TODO - make this spell have an effect based on skill level
int spell_teleport(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int to_room;
  char buf[100]; 
  extern int arena[4];
  extern int top_of_world;      /* ref to the top element of world */

  if(!victim) {
    log("Null victim sent to teleport!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if((ch->in_room >= 0 && ch->in_room <= top_of_world) &&
    IS_SET(world[ch->in_room].room_flags, ARENA) && arena[2] == -3) {
    send_to_char("You can't teleport in potato arenas!\n\r", ch);
    return eFAILURE;
  }

  if(IS_SET(world[victim->in_room].room_flags, TELEPORT_BLOCK) ||
     IS_AFFECTED(victim, AFF_SOLIDITY)) {
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
              ( (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN)) ? 
                    (IS_SET(zone_table[world[to_room].zone].zone_flags, ZONE_IS_TOWN)) :
                    FALSE
               ) ||
              ( IS_AFFECTED(victim, AFF_CHAMPION) && (IS_SET(world[to_room].room_flags, CLAN_ROOM) ||
                to_room >= 1900 && to_room <= 1999)
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


/* BLESS */

int spell_bless(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  if(!ch && (!victim || !obj))
  {
    log("Null ch or victim and obj in bless.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

	// Bless on an object should do something (perhaps reduce weight by X?)
  if (obj) {
    if ( (5*level > GET_OBJ_WEIGHT(obj)) &&
	 (GET_POS(ch) != POSITION_FIGHTING)) {
		SET_BIT(obj->obj_flags.extra_flags, ITEM_BLESS);
		act("$p briefly glows.",ch,obj,0,TO_CHAR, 0);
    }
  } else {
     if(affected_by_spell(victim, SPELL_BLESS))
       affect_from_char(victim, SPELL_BLESS);
     send_to_char("You feel blessed.\n\r", victim);

     if (victim != ch)
        act("$N receives the blessing from your god.", ch, NULL, victim, TO_CHAR, 0);

     af.type      = SPELL_BLESS;
     af.duration  = 6 + skill / 3;
     af.modifier  = 1 + skill / 45;
     af.location  = APPLY_HITROLL;
     af.bitvector = -1;
     affect_to_char(victim, &af);

     af.location = APPLY_SAVING_MAGIC;
     af.modifier = 1 + skill / 18;
     affect_to_char(victim, &af);
  }
  return eSUCCESS;
}


/* PARALYZE */

int spell_paralyze(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  char buf[180];
  int retval;

 set_cantquit( ch, victim );
  if (affected_by_spell(victim, SPELL_PARALYZE))
         return eFAILURE;

  if(affected_by_spell(victim, SPELL_SLEEP)) {
     if (number(1,6)<5)
     {
       int retval;
       if (number(0,1))
          send_to_char("The combined magics fizzle!\r\n",ch);
	if (GET_POS(victim) == POSITION_SLEEPING) {
	  send_to_char("You are awakened by a burst of $6energy$R!\r\n",victim);
	  act("$n is awakened in a burst of $6energy$R!",victim,NULL,NULL, TO_ROOM,0);
	  GET_POS(victim) = POSITION_SITTING;
	}
       else {
          send_to_char("The combined magics cause an explosion!\r\n",ch);
	  retval = damage(ch,ch,number(5,10), 0, TYPE_MAGIC, 0);
      }
       return retval;
     }
  }

  if(IS_SET(victim->immune, ISR_PARA) || IS_AFFECTED(victim, AFF_NO_PARA)) {
     act("$N absorbs your puny spell and seems no different!", ch, NULL, victim, TO_CHAR, 0);
     act("$N absorbs $n's puny spell and seems no different!", ch, NULL, victim, TO_ROOM, NOTVICT);
     if(!IS_NPC(victim))
        act("You absorb $n's puny spell and are no different!", ch, NULL, victim, TO_VICT, 0);
     return eSUCCESS;
  }

	/* save the newbies! */
  if(!IS_NPC(ch) && !IS_NPC(victim) && (GET_LEVEL(victim) < 10)) {
    send_to_char("Your cold-blooded act causes your magic to misfire!\n\r", ch);
    victim = ch;
  }

   if (malediction_res(ch, victim, SPELL_PARALYZE)) {
      act("$N resists your attempt to paralyze $M!", ch, NULL, victim, TO_CHAR,0);
      act("$N resists $n's attempt to paralyze $M!", ch, NULL, victim, TO_ROOM,NOTVICT);
      act("You resist $n's attempt to paralyze you!",ch,NULL,victim,TO_VICT,0);
      if (IS_NPC(victim) && (!victim->fighting) && GET_POS(victim) > POSITION_SLEEPING) {
         retval = attack(victim, ch, TYPE_UNDEFINED);
         retval = SWAP_CH_VICT(retval);
         return retval;
      }
     return eSUCCESS;
   }

  if(IS_NPC(victim) && (GET_LEVEL(victim) == 0)) {
    log("Null victim level in spell_paralyze.", ANGEL, LOG_BUG);
    return eFAILURE;
  }


	// paralyze vs sleep modifier
  int save = 0;
  if (affected_by_spell(victim,SPELL_SLEEP))
    save = -15; // Above check takes care of sleep.
  int spellret = saves_spell(ch, victim, save, SAVE_TYPE_MAGIC);

	/* ideally, we would do a dice roll to see if spell hits or not */
  if(spellret >= 0 && (victim != ch)) {
      act("$N seems to be unaffected!", ch, NULL, victim, TO_CHAR, 0);
      if(!IS_NPC(victim)) {
         act("$n tried to paralyze you!", ch, NULL, victim, TO_VICT, 0);
      }
      if (IS_NPC(victim) && (!victim->fighting) && GET_POS(victim) > POSITION_SLEEPING) {
         retval = attack(victim, ch, TYPE_UNDEFINED);
         retval = SWAP_CH_VICT(retval);
         return retval;
      }
      return eSUCCESS;
  }

	/* if they are too big - do a dice roll to see if they backfire */
  if(!IS_NPC(ch) && !IS_NPC(victim) && ((level - GET_LEVEL(victim)) > 10)) {
      act("$N seems to be unaffected!", ch, NULL, victim, TO_CHAR, 0);
      victim = ch;
      if(saves_spell(ch, ch, -100, SAVE_TYPE_MAGIC) >= 0) {
        act("Your magic misfires but you are saved!", ch, NULL, victim, TO_CHAR,0);
        return eSUCCESS;
      }
      act("Your cruel heart causes your magic to misfire!", ch, NULL, victim, TO_CHAR, 0);
  }

	// Finish off any singing performances (bard)
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
  int learned = has_skill(ch, SPELL_PARALYZE);
  if (learned > 75) af.duration++;
  if (learned > 50) af.duration++;
  if (spellcraft(ch, SPELL_PARALYZE))  af.duration++;
  af.bitvector = AFF_PARALYSIS;
  affect_to_char(victim, &af);
  return eSUCCESS;
}


/* BLINDNESS */

int spell_blindness(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int retval;
  set_cantquit( ch, victim );

   if (malediction_res(ch, victim, SPELL_BLINDNESS)) {
      act("$N resists your attempt to blind $M!", ch, NULL, victim, TO_CHAR,0);
      act("$N resists $n's attempt to blind $M!", ch, NULL, victim, TO_ROOM,NOTVICT);
      act("You resist $n's attempt to blind you!",ch,NULL,victim,TO_VICT,0);
      if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > POSITION_SLEEPING) {
         retval = attack(victim, ch, TYPE_UNDEFINED);
         retval = SWAP_CH_VICT(retval);
         return retval;
      }
     return eFAILURE;
   }

  if (affected_by_spell(victim, SPELL_BLINDNESS) || IS_AFFECTED(victim, AFF_BLIND))
    return eFAILURE;

  int spellret = saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC);

  if(spellret >= 0 && (victim != ch)) {
      act("$N seems to be unaffected!", ch, NULL, victim, TO_CHAR, 0);
      if(!IS_NPC(victim)) {
         act("$n tried to blind you!", ch, NULL, victim, TO_VICT, 0);
      }
      if (IS_NPC(victim) && (!victim->fighting) && GET_POS(victim) > POSITION_SLEEPING) {
         retval = attack(victim, ch, TYPE_UNDEFINED);
         retval = SWAP_CH_VICT(retval);
         return retval;
      }
      return eSUCCESS;
  }

  act("$n seems to be blinded!", victim, 0, 0, TO_ROOM, INVIS_NULL);
  send_to_char("You have been blinded!\n\r", victim);

  af.type      = SPELL_BLINDNESS;
  af.location  = APPLY_HITROLL;
  af.modifier  = has_skill(victim,SKILL_BLINDFIGHTING)?skill_success(victim,0,SKILL_BLINDFIGHTING)?-10:-20:-20;
  af.duration  = 1 + (skill > 33) + (skill > 60);
  af.bitvector = AFF_BLIND;
  affect_to_char(victim, &af);

  af.location = APPLY_AC;
  af.modifier  = has_skill(victim,SKILL_BLINDFIGHTING)?skill_success(victim,0,SKILL_BLINDFIGHTING)?+25:50:50;
  affect_to_char(victim, &af);
  return eSUCCESS;
}


/* CREATE FOOD */

int spell_create_food(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct obj_data *tmp_obj;

  tmp_obj = clone_object(real_object(7));
  tmp_obj->obj_flags.value[0] += skill/2;

  obj_to_room(tmp_obj, ch->in_room);

  act("$p suddenly appears.",ch,tmp_obj,0,TO_ROOM, INVIS_NULL);
  act("$p suddenly appears.",ch,tmp_obj,0,TO_CHAR, INVIS_NULL);
  return eSUCCESS;
}


/* CREATE WATER */

int spell_create_water(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int water;
  if(!ch || !obj)
  {
    log("Null ch or obj in create_water.", ANGEL, LOG_BUG);
    return eFAILURE|eINTERNAL_ERROR;
  }

  if (GET_ITEM_TYPE(obj) == ITEM_DRINKCON) 
  {
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


/* REMOVE PARALYSIS */

int spell_remove_paralysis(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  if(!victim)
  {
    log("Null victim in remove_paralysis!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if (affected_by_spell(victim, SPELL_PARALYZE) &&
      number(1, 100) < (80 + skill/6)) 
  {
	 affect_from_char(victim, SPELL_PARALYZE);
         send_to_char("Your spell is successful!\r\n", ch);
	 send_to_char("Your movement returns!\n\r", victim);
  }
  else send_to_char("Your spell fails to return the victim's movement.\r\n", ch);

  return eSUCCESS;
}


/* REMOVE BLIND */

int spell_remove_blind(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  if (!victim) {
    log("Null victim in remove_blind!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(number(1, 100) < ( 80 + skill/6 ) )
  {
    if(!affected_by_spell(victim, SPELL_BLINDNESS) && !IS_AFFECTED(victim, AFF_BLIND)) {
       if(victim == ch) send_to_char("Seems you weren't blind after all.\n\r", ch);
       else act("Seems $N wasn't blind after all.", ch, 0, victim, TO_CHAR, 0);
    }
    if (affected_by_spell(victim, SPELL_BLINDNESS)) 
    {
      affect_from_char(victim, SPELL_BLINDNESS);
      send_to_char("Your vision returns!\n\r", victim);
      if(victim != ch)
         act("$N can see again!", ch, 0, victim, TO_CHAR, 0);         
    }
    if (IS_AFFECTED(victim, AFF_BLIND)) {
      REMBIT(victim->affected_by, AFF_BLIND);
      send_to_char("Your vision returns!\n\r", victim);
      if(victim != ch)
         act("$N can see again!", ch, 0, victim, TO_CHAR, 0);
    }
  }
  else (ch==victim) ? send_to_char("Your spell fails to return your vision!\r\n", ch) : act("Your spell fails to return $N's vision!", ch, 0, victim, TO_CHAR, 0);

  return eSUCCESS;
}


/* CURE CRITIC */

int spell_cure_critic(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int healpoints;
  char buf[MAX_STRING_LENGTH],dammsg[MAX_STRING_LENGTH];

  if(!victim)
  {
    log("Null victim in cure_critic.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Healing spells are useless on the undead.\n\r", ch);
    return eFAILURE;
  }

  if (GET_RACE(victim) == RACE_GOLEM) {
	send_to_char("The heavy magics surrounding this being prevent healing.\r\n",ch);
	return eFAILURE;
  }

  if (!can_heal(ch,victim, SPELL_CURE_CRITIC)) return eFAILURE;
  healpoints = dam_percent(skill, 100);
  if ( (healpoints + GET_HIT(victim)) > hit_limit(victim) ) {
    healpoints = hit_limit(victim) - GET_HIT(victim);
	 GET_HIT(victim) = hit_limit(victim);
  }
  else
	 GET_HIT(victim) += healpoints;

  update_pos(victim);

  sprintf(dammsg,"$B%d$R damage", healpoints);

  if (ch!=victim) {
     sprintf(buf,"You heal %s of the more critical wounds on $N.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"several":"several");
     act(buf,ch,0,victim,TO_CHAR,0);
     sprintf(buf,"$n heals %s of your more critical wounds.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"several":"several");
     act(buf,ch,0,victim, TO_VICT, 0);
     sprintf(buf,"$n heals | of the more critical wounds on $N.");
     send_damage(buf, ch, 0, victim, dammsg,"$n heals several of the more critical wounds on $N.", TO_ROOM);
  } else {
     sprintf(buf,"You heal %s of your more critical wounds.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"several":"several");
     act(buf,ch,0,0,TO_CHAR,0);
     sprintf(buf,"$n heals | of $s more critical wounds.");
     send_damage(buf,ch,0,victim,dammsg,"$n heals several of $s more critical wounds.",TO_ROOM);
  }

  return eSUCCESS;
}


/* CURE LIGHT */

int spell_cure_light(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int healpoints;
  char buf[MAX_STRING_LENGTH],dammsg[MAX_STRING_LENGTH];

  if(!ch || !victim)
  {
    log("Null ch or victim in cure_light!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Healing spells are useless on the undead.\n\r", ch);
    return eFAILURE;
  }

  if (GET_RACE(victim) == RACE_GOLEM) {
        send_to_char("The heavy magics surrounding this being prevent healing.\r\n",ch);
        return eFAILURE;
  }

  if (!can_heal(ch,victim, SPELL_CURE_LIGHT)) return eFAILURE;
  healpoints = dam_percent(skill, 25);
  if ( (healpoints+GET_HIT(victim)) > hit_limit(victim) ) {
     healpoints = hit_limit(victim) - GET_HIT(victim);
	 GET_HIT(victim) = hit_limit(victim);
  }
  else
	 GET_HIT(victim) += healpoints;

  update_pos( victim );

  sprintf(dammsg,"$B%d$R damage", healpoints);

  if (ch!=victim) {
     sprintf(buf,"You heal %s small cuts and scratches on $N.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"several":"several");
     act(buf,ch,0,victim,TO_CHAR,0);
     sprintf(buf,"$n heals %s of your small cuts and scratches.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"several":"several");
     act(buf,ch,0,victim, TO_VICT, 0);
     sprintf(buf,"$n heals | of small cuts and scratches on $N.");
     send_damage(buf, ch, 0, victim, dammsg,"$n heals several small cuts and scratches on $N.", TO_ROOM);
  } else {
     sprintf(buf,"You heal %s of your small cuts and scratches.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"several":"several");
     act(buf,ch,0,0,TO_CHAR,0);
     sprintf(buf,"$n heals | of $s small cuts and scratches.");
     send_damage(buf,ch,0,victim,dammsg,"$n heals several of $s small cuts and scratches.",TO_ROOM);
  }
  return eSUCCESS;
}


/* CURSE */

int spell_curse(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int retval;

  if (obj) {
	 SET_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
         act("$p glows $4red$R momentarily, before returning to its original color.", ch, obj, 0, TO_CHAR, 0);
	 /* LOWER ATTACK DICE BY -1 */
	  if(obj->obj_flags.type_flag == ITEM_WEAPON)  {
		 if (obj->obj_flags.value[2] > 1) {
		  obj->obj_flags.value[2]--;
		 }
//		 else {
//		  send_to_char("Your curse has failed.\n\r", ch);
//		 }
	  }
  } else {
        if(!ch || !victim)
          return eFAILURE;
	int duration =0, save = 0;
        set_cantquit(ch, victim);
	if (skill < 41)      { duration = 1; save =  -5;}
	else if (skill < 51) { duration = 2; save = -10;}
	else if (skill < 61) { duration = 3; save = -15;}
        else if (skill < 71) { duration = 4; save = -20;}
        else if (skill < 81) { duration = 5; save = -25;}
	else                 { duration = 6; save = -30;}

	if (!IS_NPC(victim) && GET_LEVEL(victim) < 11)
	{
	  send_to_char("The curse fizzles!\r\n",ch);
 	  return eSUCCESS;
	}

 set_cantquit( ch, victim );

   if (malediction_res(ch, victim, SPELL_CURSE)) {
	act("$N resists your attempt to curse $M!", ch, NULL, victim, TO_CHAR,0);
	act("$N resists $n's attempt to curse $M!", ch, NULL, victim, TO_ROOM,NOTVICT);
	act("You resist $n's attempt to curse you!",ch,NULL,victim,TO_VICT,0);
   } else {


	 if (affected_by_spell(victim, SPELL_CURSE))
		return eFAILURE;

	 af.type      = SPELL_CURSE;
	 af.duration  = duration;
	 af.modifier  = save;
	 af.location  = APPLY_SAVES;
	if (skill > 70)
         af.bitvector = AFF_CURSE;
	else af.bitvector = -1;
	 affect_to_char(victim, &af);
	 act("$n briefly reveals a $4red$R aura!", victim, 0, 0, TO_ROOM, 0);
	 act("You feel very uncomfortable as a curse takes hold of you.",victim,0,0,TO_CHAR, 0);
     }
	if (IS_NPC(victim) && !(victim->fighting)) {
	   retval = one_hit( victim, ch, TYPE_UNDEFINED, FIRST);
 	   retval = SWAP_CH_VICT(retval);
         return retval;
	}
  }
  return eSUCCESS;
}


/* DETECT EVIL */

int spell_detect_evil(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(!victim)
  {
    log("Null victim in detect evil!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(ch != victim && skill < 70) {
     send_to_char("You aren't practiced enough to be able to cast on others.\n\r", ch);
     return eFAILURE;
  }    

  if ( affected_by_spell(victim, SPELL_DETECT_EVIL) )
    affect_from_char(victim, SPELL_DETECT_EVIL);

  af.type      = SPELL_DETECT_EVIL;
  af.duration  = 10 + skill/2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_DETECT_EVIL;

  affect_to_char(victim, &af);
  send_to_char("You become more conscious of the evil around you.\n\r", victim);
  act("$n looks to be more conscious of the evil around $m.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}


/* DETECT GOOD */

int spell_detect_good(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(!victim)
  {
    log("Null victim sent to detect_good.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(ch != victim && skill < 70) {
     send_to_char("You aren't practiced enough to be able to cast on others.\n\r", ch);
     return eFAILURE;
  }    

  if ( affected_by_spell(victim, SPELL_DETECT_GOOD) )
    affect_from_char(victim, SPELL_DETECT_GOOD);

  af.type      = SPELL_DETECT_GOOD;
  af.duration  = 10 + skill/2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_DETECT_GOOD;

  affect_to_char(victim, &af);
  send_to_char("You are now able to truly recognize the good in others.\n\r", victim);
  act("$n looks to be more conscious of the evil around $m.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}


/* TRUE SIGHT */

int spell_true_sight(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

  af.type      = SPELL_TRUE_SIGHT;
  af.duration  = 6 + skill / 2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_TRUE_SIGHT;

  affect_to_char(victim, &af);
  send_to_char("You feel your vision enhanced with an incredibly keen perception.\n\r", victim);
  return eSUCCESS;
}


/* DETECT INVISIBILITY */

int spell_detect_invisibility(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

  af.type      = SPELL_DETECT_INVISIBLE;
  af.duration  = 12 + skill / 2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_DETECT_INVISIBLE;

  affect_to_char(victim, &af);
  send_to_char("Your eyes tingle, allowing you to see the invisible.\n\r", victim);
  if (ch!=victim)
    csendf(ch, "%s's eyes tingle briefly.\n\r", GET_SHORT(victim));
  return eSUCCESS;
}


/* INFRAVISION */

int spell_infravision(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

  af.type      = SPELL_INFRAVISION;
  af.duration  = 12 + skill / 2;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_INFRARED;

  affect_to_char(victim, &af);
  send_to_char("Your eyes glow $B$4red$R.\n\r", victim);
  if (ch!=victim)
    csendf(ch, "%s's eyes glow $B$4red$R.\n\r", GET_SHORT(victim));

  return eSUCCESS;
}


/* DETECT MAGIC */

int spell_detect_magic(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int learned = has_skill(ch, SPELL_DETECT_MAGIC);

  if(!victim)
  {
    log("Null victim sent to detect_magic.", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if ( affected_by_spell(victim, SPELL_DETECT_MAGIC) )
	affect_from_char(victim, SPELL_DETECT_MAGIC);

  af.type      = SPELL_DETECT_MAGIC;
  af.duration  = 6 + skill / 2;
  af.modifier  = learned;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_DETECT_MAGIC;

  affect_to_char(victim, &af);
  send_to_char("Your vision temporarily blurs, your focus shifting to the metaphysical realm.\n\r", victim);
  if (ch!=victim)
    csendf(ch, "%s's eyes appear to blur momentarily.\n\r", GET_SHORT(victim));
  return eSUCCESS;
}


/* HASTE */

int spell_haste(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(!victim)
  {
    log("Null victim sent to haste", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if ( affected_by_spell(victim, SPELL_HASTE) || IS_AFFECTED(victim, AFF_HASTE))
	 return eFAILURE;

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


/* DETECT POISON */

// TODO - make this use skill for addtional effects
int spell_detect_poison(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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
    } else {
      send_to_char("There is nothing much that poison would do on this.\n\r", ch);
    }
  }
  return eSUCCESS;
}


/* ENCHANT ARMOR - CURRENTLY INACTIVE */

int spell_enchant_armor(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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
		act("$p glows $B$3blue$R.",ch,obj,0,TO_CHAR, 0);
	 } else if (IS_EVIL(ch)) {
		SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_GOOD);
		act("$p glows $B$4red$R.",ch,obj,0,TO_CHAR, 0);
    } else {
      act("$p glows $5yellow$R.",ch,obj,0,TO_CHAR, 0);
	}
	 }
  return eSUCCESS;
}


/* ENCHANT WEAPON - CURRENTLY INACTIVE */ 

int spell_enchant_weapon(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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
      act("$p glows $B$1blue$R.",ch,obj,0,TO_CHAR,0);
	 } else if (IS_EVIL(ch)) {
      SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_GOOD);
      act("$p glows $B$4red$R.",ch,obj,0,TO_CHAR, 0);
    } else {
      act("$p glows $B$5yellow$R.",ch,obj,0,TO_CHAR, 0);
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


/* MANA - Potion & Immortal Only */

int spell_mana(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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


/* HEAL */

int spell_heal(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int healy;
  char buf[MAX_STRING_LENGTH],dammsg[MAX_STRING_LENGTH];

  if(!victim) {
    log("Null victim sent to heal!", ANGEL, LOG_BUG);
    return eFAILURE;
  } 
/* Adding paladin ability to heal others back in. 
 * if (GET_CLASS(ch) == CLASS_PALADIN && victim != ch)
 * {
 *   send_to_char("You cannot target others with this spell.\r\n",ch);
 *   return eFAILURE;
 * }
 */ 

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Healing spells are useless on the undead.\n\r", ch);
    return eFAILURE;
  }
  if (GET_RACE(victim) == RACE_GOLEM) {
        send_to_char("The heavy magics surrounding this being prevent healing.\r\n",ch);
        return eFAILURE;
  }

  if (!can_heal(ch,victim,SPELL_HEAL)) return eFAILURE;
  healy = dam_percent(skill,250);
  GET_HIT(victim) += healy;

  if (GET_HIT(victim) >= hit_limit(victim)) {
     healy += hit_limit(victim) - GET_HIT(victim) - dice(1,4);
	 GET_HIT(victim) = hit_limit(victim)-dice(1,4);
  }

  update_pos( victim );

  sprintf(dammsg," of $B%d$R damage", healy);

  if (ch!=victim) {
     sprintf(buf,"Your incantation heals $N%s.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
     act(buf,ch,0,victim,TO_CHAR,0);
     sprintf(buf,"$n calls forth an incantation that heals you%s.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
     act(buf,ch,0,victim, TO_VICT, 0);
     sprintf(buf,"$n calls forth an incantation that heals $N|.");
     send_damage(buf, ch, 0, victim, dammsg,"", TO_ROOM);
  } else {
     sprintf(buf,"Your incantation heals you%s.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
     act(buf,ch,0,0,TO_CHAR,0);
     sprintf(buf,"$n calls forth an incantation that heals $m|.");
     send_damage(buf,ch,0,victim,dammsg,"",TO_ROOM);
  }
  return eSUCCESS;
}


/* POWER HEAL */

int spell_power_heal(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int healy;
  char buf[MAX_STRING_LENGTH],dammsg[MAX_STRING_LENGTH];

  if(!victim) {
    log("Null victim sent to power heal!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Healing spells are useless on the undead.\n\r", ch);
    return eFAILURE;
  }
  if (GET_RACE(victim) == RACE_GOLEM) {
        send_to_char("The heavy magics surrounding this being prevent healing.\r\n",ch);
        return eFAILURE;
  }

  if (!can_heal(ch,victim, SPELL_POWER_HEAL)) return eFAILURE;
  healy = dam_percent(skill, 300);
  GET_HIT(victim) += healy;

  if (GET_HIT(victim) >= hit_limit(victim)) {
     healy += hit_limit(victim)-GET_HIT(victim)-dice(1,4);
	 GET_HIT(victim) = hit_limit(victim)-dice(1,4);
  }

  update_pos( victim );

  sprintf(dammsg," of $B%d$R damage", healy);

  if (ch!=victim) {
     sprintf(buf,"Your powerful incantation heals $N%s.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
     act(buf,ch,0,victim,TO_CHAR,0);
     sprintf(buf,"$n calls forth a powerful incantation that heals you%s.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
     act(buf,ch,0,victim, TO_VICT, 0);
     sprintf(buf,"$n calls forth a powerful incantation that heals $N|.");
     send_damage(buf, ch, 0, victim, dammsg,"", TO_ROOM);
  } else {
     sprintf(buf,"Your powerful incantation heals you%s.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
     act(buf,ch,0,0,TO_CHAR,0);
     sprintf(buf,"$n calls forth a powerful incantation that heals $m|.");
     send_damage(buf,ch,0,victim,dammsg,"",TO_ROOM);
  }

  return eSUCCESS;
}


/* FULL HEAL */

int spell_full_heal(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  assert(victim);
  int healamount = 0;
  char buf[MAX_STRING_LENGTH],dammsg[MAX_STRING_LENGTH];

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Healing spells are useless on the undead.\n\r", ch);
    return eFAILURE;
  }
  if (GET_RACE(victim) == RACE_GOLEM) {
        send_to_char("The heavy magics surrounding this being prevent healing.\r\n",ch);
        return eFAILURE;
  }

  if (!can_heal(ch,victim, SPELL_FULL_HEAL)) return eFAILURE;

  healamount = 10 * (skill/2 + 5);
  if(GET_ALIGNMENT(ch) < -349)
    healamount -= 2* (skill/2 + 5);
  else if(GET_ALIGNMENT(ch) > 349)
    healamount += (skill / 2 + 5);

  GET_HIT(victim) += healamount;

  if (GET_HIT(victim) >= hit_limit(victim))
	 GET_HIT(victim) = hit_limit(victim)-dice(1,4);

  update_pos( victim );

  sprintf(dammsg," of $B%d$R damage", healamount);

  if (ch!=victim) {
     sprintf(buf,"You call forth the magic of the gods to restore $N%s.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
     act(buf,ch,0,victim,TO_CHAR,0);
     sprintf(buf,"$n calls forth the magic of the gods to restore you%s.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
     act(buf,ch,0,victim, TO_VICT, 0);
     sprintf(buf,"$n calls forth the magic of the gods to restore $N|.");
     send_damage(buf, ch, 0, victim, dammsg,"", TO_ROOM);
  } else {
     sprintf(buf,"You call forth the magic of the gods to restore you%s.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
     act(buf,ch,0,0,TO_CHAR,0);
     sprintf(buf,"$n calls forth the magic of the gods to restore $m|.");
     send_damage(buf,ch,0,victim,dammsg,"",TO_ROOM);
  }

  return eSUCCESS;
}


/* INVISIBILITY */

int spell_invisibility(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

	 if (affected_by_spell(victim, SPELL_INVISIBLE))
		affect_from_char(victim, SPELL_INVISIBLE);

	if(IS_AFFECTED(victim, AFF_INVISIBLE))
		return eFAILURE;

	act("$n slowly fades out of existence.", victim,0,0,TO_ROOM, INVIS_NULL);
	send_to_char("You slowly fade out of existence.\n\r", victim);

	af.type      = SPELL_INVISIBLE;
	af.duration  = (int) ( skill / 3.75 );
	af.modifier  = -50;
	af.location  = APPLY_AC;
	af.bitvector = AFF_INVISIBLE;
	affect_to_char(victim, &af);
  }
  return eSUCCESS;
}


/* LOCATE OBJECT */

int spell_locate_object(ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct obj_data *i;
  char name[256];
  char buf[MAX_STRING_LENGTH];
  char tmpname[256];
  char *tmp;
  int j, n;
  int total;
  int number;

  assert(ch);
  if (!arg)
    return eFAILURE;

  one_argument(arg, name);

  strncpy(tmpname, name, 256);
  tmp = tmpname;
  if((number = get_number(&tmp))<0)
    number = 0;

  total = j = (int) (skill / 1.5);

  for (i = object_list, n = 0; i && (j>0) && (number>0); i = i->next)
  {
    if (IS_OBJ_STAT(i, ITEM_NOSEE))
       continue;

    if(IS_SET(i->obj_flags.more_flags, ITEM_NOLOCATE))
       continue;

    char_data *owner = 0;
    int room = -1;
    if (i->equipped_by)
      owner = i->equipped_by;

    if (i->carried_by)
      owner = i->carried_by;
	  
    if (i->in_room)
      room = i->in_room;

    if (i->in_obj && i->in_obj->equipped_by)
      owner = i->in_obj->equipped_by;

    if (i->in_obj && i->in_obj->carried_by)
      owner = i->in_obj->carried_by;

    if (i->in_obj && i->in_obj->in_room)
      room = i->in_obj->in_room;

    // If owner, PC, with desc and not con_playing or wizinvis,
    if (owner && is_in_game(owner) &&
	(owner->pcdata->wizinvis > GET_LEVEL(ch)))
      continue;

    // Skip objs in god rooms
    if (room >= 0 && room <= 47)
      continue;

    buf[0] = 0;
    if (isname(tmp, i->name)) {
      if(i->carried_by) {
	sprintf(buf,"%s carried by %s.\n\r", i->short_description,
		PERS(i->carried_by,ch));
      } else if (i->in_obj) {
	sprintf(buf,"%s is in %s.\n\r",i->short_description,
		i->in_obj->short_description);
      } else if (i->in_room != NOWHERE) {
        sprintf(buf, "%s is in %s.\n\r", i->short_description,
		world[i->in_room].name);
      } else if (i->equipped_by != NULL) {
        sprintf(buf, "%s is in use in an unknown location.\n\r",
		i->short_description);
      }

      if (buf[0] != 0) {
	n++;
	if (n >= number) {
	  send_to_char(buf, ch);
	  j--;
	}
      }
    }
  }

  if(j==total)
	 send_to_char("There appears to be no such object.\n\r",ch);

  if(j==0)
	 send_to_char("The tremendous amount of information leaves you very confused.\n\r",ch);

  return eSUCCESS;
}


/* POISON */

int spell_poison(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int retval = eSUCCESS;
  bool endy = FALSE; 

  if (victim) 
  {
     if(IS_AFFECTED(victim, AFF_POISON)) {
        act("$N's insides are already being eaten away by poison!", ch, NULL, victim, TO_CHAR, 0);
        endy = TRUE;
     }
     else if (IS_SET(victim->immune, ISR_POISON) || malediction_res(ch, victim, SPELL_POISON)) {
         act("$N resists your attempt to poison $M!", ch, NULL, victim, TO_CHAR,0);
         act("$N resists $n's attempt to poison $M!", ch, NULL, victim, TO_ROOM,NOTVICT);
         act("You resist $n's attempt to poison you!",ch,NULL,victim,TO_VICT,0);
         endy = TRUE;
     }
     set_cantquit(ch, victim);
     if (!endy) {
        af.type = SPELL_POISON;
        af.duration = skill / 10;
        af.modifier = IS_NPC(ch)?-123:(int)ch;
        af.location = APPLY_NONE;
        af.bitvector = AFF_POISON;
        affect_join(victim, &af, FALSE, FALSE);
        send_to_char("You feel very sick.\n\r", victim);
        act("$N looks very sick.", ch,0,victim, TO_CHAR, 0);
     }
     if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > POSITION_SLEEPING) {
         retval = attack(victim, ch, TYPE_UNDEFINED);
         retval = SWAP_CH_VICT(retval);
         return retval;
      }
  } else { /* Object poison */
    if ((obj->obj_flags.type_flag == ITEM_DRINKCON) ||
        (obj->obj_flags.type_flag == ITEM_FOOD)) 
    {
      act("$p glows $2green$R for a second, before returning to its original color.", ch, obj, 0, TO_CHAR, 0);    
      obj->obj_flags.value[3] = 1;
    } else {
      send_to_char("Nothing special seems to happen.\n\r", ch);
    }
  }
  return retval;
}


/* PROTECTION FROM EVIL */

int spell_protection_from_evil(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  assert(victim);
  int duration = skill?skill/3:level/3;
  int modifier = level + 10;
 
  /* keep spells from stacking */
  if (IS_AFFECTED(victim, AFF_PROTECT_EVIL) || 
      IS_AFFECTED(victim, AFF_PROTECT_GOOD) || 
      affected_by_spell(victim, SPELL_PROTECT_FROM_GOOD))
    return eFAILURE;

  // Used to identify PFE from godload_defender(), obj vnum 556
  if (skill == 150) { duration = 4; modifier = 60; } 

  if (!affected_by_spell(victim, SPELL_PROTECT_FROM_EVIL) ) 
  {
	 af.type      = SPELL_PROTECT_FROM_EVIL;
	 af.duration  = duration;
	 af.modifier  = modifier;
	 af.location  = APPLY_NONE;
	 af.bitvector = AFF_PROTECT_EVIL;
	 affect_to_char(victim, &af);
	 send_to_char("You have a righteous, protected feeling!\n\r", victim);
         act("A dark, $6pulsing$R aura surrounds $n.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  }
  return eSUCCESS;
}


/* PROTECTION FROM GOOD */

int spell_protection_from_good(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  assert(victim);

  int duration = skill?skill/3:level/3;
  int modifier = level + 10;

  /* keep spells from stacking */
  if (IS_AFFECTED(victim, AFF_PROTECT_GOOD) ||
      IS_AFFECTED(victim, AFF_PROTECT_EVIL) ||
      affected_by_spell(victim, SPELL_PROTECT_FROM_EVIL))
    return eFAILURE;

  if (!affected_by_spell(victim, SPELL_PROTECT_FROM_GOOD)) 
  {
	 af.type      = SPELL_PROTECT_FROM_GOOD;
	 af.duration  = duration;
	 af.modifier  = modifier;
	 af.location  = APPLY_NONE;
	 af.bitvector = AFF_PROTECT_GOOD;
	 affect_to_char(victim, &af);
	 send_to_char("You feel yourself wrapped in a protective mantle of evil.\n\r", victim);
         act("A light, $B$6pulsing$R aura surrounds $n.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  }
  return eSUCCESS;
}


/* REMOVE CURSE */

int spell_remove_curse(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int j;
  assert(ch && (victim || obj));

  if(obj) {
	 if(IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
		act("$p briefly glows $3blue$R.", ch, obj, 0, TO_CHAR, 0);
		REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
		if (obj_index[obj->item_number].virt == 514)
		{
			int i = 0;
			for (i= 0; i < obj->num_affects;i++)
			  if (obj->affected[i].location == APPLY_MANA_REGEN)
			  return eSUCCESS; // only do it once
			SET_BIT(obj->obj_flags.extra_flags, ITEM_HUM);
		struct char_data *t = obj->equipped_by;
		int z = -1;
	extern void wear(struct char_data *ch, struct obj_data *obj_object, int keyword);
		if (t->equipment[WEAR_FINGER_L] == obj)
			z = WEAR_FINGER_L;
		else
			z = WEAR_FINGER_R;
		if (t)
		    obj_to_char(unequip_char(t, z) , t);
			add_obj_affect(obj, APPLY_MANA_REGEN, 2);
		if (t) 
			wear(t, obj, 0);
			
			
			act("With the restrictive curse lifted, $p begins to hum with renewed power!",ch,obj,0, TO_ROOM, 0);
			act("With the restrictive curse lifted, $p begins to hum with renewed power!",ch,obj,0, TO_CHAR, 0);
		}
	 }
	 return eSUCCESS;
  }

  /* Then it is a PC | NPC */
  if(affected_by_spell(victim, SPELL_CURSE)) {
	 act("$n briefly glows $4red$R, then $3blue$R.",victim,0,0,TO_ROOM, 0);
	 act("You feel better.",victim,0,0,TO_CHAR, 0);
	 affect_from_char(victim, SPELL_CURSE);
	 return eSUCCESS;
  }

  for(j=0; j<MAX_WEAR; j++) {
    if((obj = victim->equipment[j]) && IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
		if (skill > 70 && obj_index[obj->item_number].virt == 514)
		{
			int i = 0;
			for (i= 0; i < obj->num_affects;i++)
			  if (obj->affected[i].location == APPLY_MANA_REGEN)
			  return eSUCCESS; // only do it once
			SET_BIT(obj->obj_flags.extra_flags, ITEM_HUM);
			add_obj_affect(obj, APPLY_MANA_REGEN, 2);
			victim->mana_regen += 2;
			act("With the restrictive curse lifted, $p begins to hum with renewed power!",victim,obj,0, TO_ROOM, 0);
			act("With the restrictive curse lifted, $p begins to hum with renewed power!",victim,obj,0, TO_CHAR, 0);
		}
		act("$p briefly glows $3blue$R.", victim, obj, 0, TO_CHAR, 0);
		act("$p carried by $n briefly glows $3blue$R.", victim, obj, 0, TO_ROOM, 0);
		 REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
		 return eSUCCESS;
    }
  }

  for(obj = victim->carrying; obj; obj = obj->next_content)
	  if(IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
		 act("$p carried by $n briefly glows $3blue$R.", victim, obj, 0, TO_ROOM, 0);
                 act("$p briefly glows $3blue$R.", victim, obj, 0, TO_CHAR, 0); 
		if (skill > 70 && obj_index[obj->item_number].virt == 514)
		{
			int i = 0;
			for (i= 0; i < obj->num_affects;i++)
			  if (obj->affected[i].location == APPLY_MANA_REGEN)
			  return eSUCCESS; // only do it once
			SET_BIT(obj->obj_flags.extra_flags, ITEM_HUM);
			add_obj_affect(obj, APPLY_MANA_REGEN, 2);
			act("With the restrictive curse lifted, $p begins to hum with renewed power!",victim,obj,0, TO_ROOM, 0);
			act("With the restrictive curse lifted, $p begins to hum with renewed power!",victim,obj,0, TO_CHAR, 0);
		}
		 REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
		 break;
	  }

  if(affected_by_spell(victim, SPELL_ATTRITION)) {
	 act("$n briefly glows $4red$R, then $3blue$R.",victim,0,0,TO_ROOM, 0);
	 act("The curse of attrition afflicting you has been lifted!",victim,0,0,TO_CHAR, 0);
	 affect_from_char(victim, SPELL_ATTRITION);
	 return eSUCCESS;
  }

  return eSUCCESS;
}


/* REMOVE POISON */

int spell_remove_poison(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  assert(ch && (victim || obj));

  if (victim) {
	if(affected_by_spell(victim, SPELL_DEBILITY)) {
	  act("$n looks better.",victim,0,0,TO_ROOM, 0);
	  act("You feel less debilitated.",victim,0,0,TO_CHAR, 0);
	  affect_from_char(victim, SPELL_DEBILITY);
	  return eSUCCESS;
	}
	 if(affected_by_spell(victim,SPELL_POISON)) {
		affect_from_char(victim,SPELL_POISON);
		act("A warm feeling runs through your body.",victim,
		  0,0,TO_CHAR, 0);
		act("$n looks better.",victim,0,0,TO_ROOM, 0);
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


/* FIRESHIELD */

int spell_fireshield(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int learned = has_skill(ch, SPELL_FIRESHIELD);

  if ( IS_AFFECTED(victim,AFF_FIRESHIELD) )
  {
	 act("$N is already fireshielded.",ch,0,victim,TO_CHAR, INVIS_NULL);
	 return eFAILURE;
  }

  if (!affected_by_spell(victim, SPELL_FIRESHIELD))
  {
    act("$n is surrounded by $B$4flames$R.",victim,0,0,TO_ROOM, INVIS_NULL);
    act("You are surrounded by $B$4flames$R.",victim,0,0,TO_CHAR, 0);

    af.type      = SPELL_FIRESHIELD;
    af.duration  = 1 + skill / 23;
    af.modifier  = learned;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_FIRESHIELD;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}


/* MEND GOLEM */

int spell_mend_golem(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct follow_type *fol;
  for (fol = ch->followers; fol; fol = fol->next)
    if (IS_NPC(fol->follower) && mob_index[fol->follower->mobdata->nr].virt == 8)
    {
      GET_HIT(fol->follower) += (int)(GET_MAX_HIT(fol->follower) * (0.12 + level / 1000.0));
      if (GET_HIT(fol->follower) > GET_MAX_HIT(fol->follower))
        GET_HIT(fol->follower) = GET_MAX_HIT(fol->follower);

      act("$n focuses $s magical energy and many of the scratches on $s golem are fixed.\n\r", ch, 0, 0, TO_ROOM, 0);
      send_to_char("You focus your magical energy and many of the scratches on your golem are fixed.\n\r", ch);
      return eSUCCESS;
    }
  send_to_char("You don't have a golem.\r\n",ch);
  return eSUCCESS;
}


/* CAMOUFLAGE (for items) */

int cast_camouflague(ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* FARSIGHT (for items) */

int cast_farsight(ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* FREEFLOAT (for items) */

int cast_freefloat(ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* INSOMNIA (for items) */

int cast_insomnia(ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* SHADOWSLIP (for items) */

int cast_shadowslip(ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* SANCTUARY (for items) */
    
int cast_sanctuary( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
	if (GET_CLASS(ch) == CLASS_PALADIN && GET_ALIGNMENT(ch) < 351)
	{
		send_to_char("You are not noble enough to cast it.\r\n",ch);
		return eFAILURE;
	}
	if (GET_CLASS(ch) == CLASS_PALADIN && tar_ch != ch)
	{
		send_to_char("You can only cast this spell on yourself.\r\n",ch);
		return eFAILURE;
	}
      return spell_sanctuary(level, ch, tar_ch, 0, skill);
      break;

	// Paladin casting restrictions above

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


/* CAMOUFLAGE */

// TODO - make this have effects based on skill
int spell_camouflague(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill) 
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_CAMOUFLAGE)) {
     act("$n fades into the plant life.", victim, 0, 0, TO_ROOM, INVIS_NULL);
     act("You fade into the plant life.", victim, 0, 0, TO_CHAR, 0);
     
     af.type          = SPELL_CAMOUFLAGE;
     af.duration      = 1 + skill / 10;
     af.modifier      = 0;
     af.location      = APPLY_NONE;
     af.bitvector     = AFF_CAMOUFLAGUE;
     affect_to_char(victim, &af);
  }
  return eSUCCESS;
}


/* FARSIGHT */

// TODO - make this gain effects based on skill
int spell_farsight(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *tar_obj, int skill) 
{
  struct affected_type af;

  if(!affected_by_spell(victim, SPELL_FARSIGHT)) {
     act("Your eyesight improves.", victim, 0, 0, TO_CHAR, 0);

     af.type         = SPELL_FARSIGHT;
     af.duration     = 2 + level / 5;
     af.modifier     = 0;
     af.location     = APPLY_NONE;
     af.bitvector    = AFF_FARSIGHT;
     affect_to_char(victim, &af);
  }
  return eSUCCESS;
}


/* FREEFLOAT */

// TODO - make this gain effects based on skill
int spell_freefloat(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *tar_obj, int skill) 
{
   struct affected_type af;

   if(!affected_by_spell(victim, SPELL_FREEFLOAT)) {
     act("You gain added stability.", victim, 0, 0, TO_CHAR, 0);

     af.type         = SPELL_FREEFLOAT;
     af.duration     = 12 + level / 4;
     af.modifier     = 0;
     af.location     = APPLY_NONE;
     af.bitvector    = AFF_FREEFLOAT;
     affect_to_char(victim, &af);
   }
   return eSUCCESS;
}


/* INSOMNIA */

// TODO - make this use skill
int spell_insomnia(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *tar_obj, int skill) 
{
   struct affected_type af;

   if(!affected_by_spell(victim, SPELL_INSOMNIA)) {
     act("You suddenly feel wide awake.", victim, 0, 0, TO_CHAR, 0);

     af.type          = SPELL_INSOMNIA;
     af.duration      = 2 + level / 5;
     af.modifier      = 0;
     af.location      = APPLY_NONE;
     af.bitvector     = AFF_INSOMNIA;
     affect_to_char(victim, &af);
   }
   return eSUCCESS;
}


/* SHADOWSLIP */

// TODO - make this use skill
int spell_shadowslip(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *tar_obj, int skill) 
{
   struct affected_type af;

   if(!affected_by_spell(victim, SPELL_SHADOWSLIP)) {
     act("Portals will no longer find you as your presence becomes obscured by shadows.", victim, 0, 0, TO_CHAR, 0);

     af.type           = SPELL_SHADOWSLIP;
     af.duration       = level / 4;
     af.modifier       = 0;
     af.location       = APPLY_NONE;
     af.bitvector      = AFF_SHADOWSLIP;
     affect_to_char(victim, &af);
   }
   return eSUCCESS;
}


/* SANCTUARY */

int spell_sanctuary(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
  {
    act("$N is already sanctified.",ch,0,victim,TO_CHAR, 0);
	 return eFAILURE;
  }
  if (affected_by_spell(victim, SPELL_HOLY_AURA))
  {
    act("$N cannot be bestowed with that much power.", ch, 0, victim, TO_CHAR,0);
    return eFAILURE;
  }
  if (!affected_by_spell(victim, SPELL_SANCTUARY))
  {
	 act("$n is surrounded by a $Bwhite aura$R.", victim, 0, 0, TO_ROOM, INVIS_NULL);
	 act("You start $Bglowing$R.", victim, 0, 0, TO_CHAR, 0);
 	 af.type      = SPELL_SANCTUARY;
	 af.duration  = 3 + skill / 18;
	 af.modifier  = 35;

	 af.location  = APPLY_NONE;
	 af.bitvector = AFF_SANCTUARY;
	 affect_to_char(victim, &af);
  }
  return eSUCCESS;
}


/* SLEEP */

int spell_sleep(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  char buf[100];
  int retval;

  set_cantquit( ch, victim );

  if(!IS_MOB(victim) && GET_LEVEL(victim) <= 15) {
     send_to_char("Oh come on....at least wait till $e's high enough level to have decent gear.\r\n", ch);
     return eFAILURE;
  }

     /* You can't sleep someone higher level than you*/
  if(affected_by_spell(victim, SPELL_INSOMNIA)
     || IS_AFFECTED(victim, AFF_INSOMNIA)) {
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
 set_cantquit( ch, victim );

   if (number(1,101) <= MIN(MAX((get_saves(ch, SAVE_TYPE_MAGIC) - get_saves(victim, SAVE_TYPE_MAGIC))/2,1), 7))
   {
	act("$N resists your attempt to sleep $M!", ch, NULL, victim, TO_CHAR,0);
	act("$N resists $n's attempt to sleep $M!", ch, NULL, victim, TO_ROOM, NOTVICT);
	act("You resist $n's attempt to sleep you!",ch,NULL,victim,TO_VICT,0);
      if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > POSITION_SLEEPING) {
         retval = attack(victim, ch, TYPE_UNDEFINED);
         retval = SWAP_CH_VICT(retval);
         return retval;
      }

     return eFAILURE;
   }

  if (level < GET_LEVEL(victim)){
	 snprintf(buf, 100, "%s laughs in your face at your feeble attempt.\n\r", GET_SHORT(victim));
	 send_to_char(buf,ch);
	 snprintf(buf, 100, "%s tries to make you sleep, but fails miserably.\n\r", GET_SHORT(ch));
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
	af.duration  = ch->level/20;;
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


/* STRENGTH */

int spell_strength(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  struct affected_type * cur_af;

  assert(victim);

  int mod = 4 + (skill>20) + (skill>40) + (skill>60) + (skill>80);

  if((cur_af = affected_by_spell(victim, SPELL_WEAKEN)))
  {
    cur_af->modifier += mod;
    send_to_char("You feel the magical weakness leave your body.\r\n", victim);
    af.type      = cur_af->type;
    af.duration  = cur_af->duration;
    af.modifier  = cur_af->modifier;
    af.location  = cur_af->location;
    af.bitvector = cur_af->bitvector;
    affect_from_char( victim, SPELL_WEAKEN );  // this makes cur_af invalid
    if( af.modifier < 0 ) // it's not out yet
       affect_to_char( victim, &af );
    return eSUCCESS;
  }

  if((cur_af = affected_by_spell(victim, SPELL_STRENGTH)))
  {
    if(cur_af->modifier <= mod)
       affect_from_char(victim, SPELL_STRENGTH);
    else {
       send_to_char("That person already has a stronger strength spell!\r\n", ch);
       return eFAILURE;
    }
  }

  send_to_char("You feel stronger.\r\n", victim);
  act("$n's muscles bulge a bit and $e looks stronger.", victim, 0, 0, TO_ROOM,INVIS_NULL);

  af.type      = SPELL_STRENGTH;
  af.duration  = level/2 + skill/3;
  af.modifier  = mod;
  af.location  = APPLY_STR;
  af.bitvector = -1;
  affect_to_char(victim, &af);
  return eSUCCESS;
}


/* VENTRILOQUATE */

int spell_ventriloquate(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
	 /* Actual spell resides in cast_ventriloquate */
  return eSUCCESS;
}


/* WORD OF RECALL */

int spell_word_of_recall(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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
  if (IS_AFFECTED(ch, AFF_CURSE))
  {
     send_to_char("Something blocks your attempt to recall.\r\n",ch);
	return eSUCCESS;
  }
  if (IS_NPC(victim))
    location = real_room(GET_HOME(victim));
  else
  {
    if(affected_by_spell(victim, FUCK_PTHIEF) || affected_by_spell(victim, FUCK_GTHIEF))
      location = real_room(START_ROOM);
    else
      location = real_room(GET_HOME(victim));

    if(IS_AFFECTED(victim, AFF_CANTQUIT))
      location = real_room(START_ROOM);
    else
      location = real_room(GET_HOME(victim));

	    // make sure they aren't recalling into someone's chall
    if(IS_SET(world[location].room_flags, CLAN_ROOM))
       if(!victim->clan || !(clan = get_clan(victim))) {
         send_to_char("The gods frown on you, and reset your home.\r\n", victim);
         location = real_room(START_ROOM);
         GET_HOME(victim) = START_ROOM;
       }
       else {
          for(room = clan->rooms; room; room = room->next)
            if(room->room_number == GET_HOME(victim))
               found = 1;

          if(!found) {
             send_to_char("The gods frown on you, and reset your home.\r\n", victim);
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

  if(IS_SET(world[location].room_flags, CLAN_ROOM) && IS_AFFECTED(victim, AFF_CHAMPION)) {
     send_to_char("No recalling into a clan hall whilst Champion.\n\r", victim);
     return eFAILURE;
  }
  if(location >= 1900 && location <= 1999 && IS_AFFECTED(victim, AFF_CHAMPION)) {
     send_to_char("No recalling into a guild hall whilst Champion.\n\r", victim);
     return eFAILURE;
  }

  if (!IS_NPC(victim) && victim->pcdata->golem && victim->pcdata->golem->in_room == victim->in_room)
  {
	if (victim->mana < 50)
	{
	    send_to_char("You don't possses the energy to bring your golem along.\r\n",victim);
	} else {
	    if (victim->pcdata->golem->fighting)
	    {
		send_to_char("Your golem is too distracted by something to follow.\r\n",victim);
	    } else {
		act("$n disappears.", victim->pcdata->golem, 0, 0, TO_ROOM, INVIS_NULL);
		move_char(victim->pcdata->golem, location);
		act("$n appears out of nowhere.",victim->pcdata->golem, 0, 0, TO_ROOM, INVIS_NULL);
	        GET_MANA(victim) -= 50;
	    }
	}
  }
	 /* a location has been found. */
  if (skill < 110) {
    if( number(1, 100) > (80 + skill / 10) ) {
      send_to_char("Your recall magic sputters and fails.\r\n", ch);
      return eFAILURE;
    }
  }
  act("$n disappears.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  move_char(victim, location);
  act("$n appears out of nowhere.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  do_look(victim, "", 15);
  return eSUCCESS;
}


/* WIZARD EYE */

int spell_wizard_eye(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

  if(IS_AFFECTED(victim, AFF_FOREST_MELD)) {
    send_to_char("Your target's location is hidden by the forests.\r\n", ch);
    return eFAILURE;
  }

  if (affected_by_spell(victim, SKILL_INNATE_EVASION))
  {
    send_to_char("Your target evades your magical scrying!\r\n",ch);
    return eFAILURE;
  }

  if(number(0, 100) > skill) {
    send_to_char("Your spell fails to locate its target.\r\n", ch);
    return eFAILURE;
  }

  original_loc = ch->in_room;
  target        =  victim->in_room;

	/* Detect Magic wiz-eye detection */
  if (affected_by_spell(victim, SPELL_DETECT_MAGIC) && affected_by_spell(victim, SPELL_DETECT_MAGIC)->modifier > 80)
    send_to_char("You sense you are the target of magical scrying.\r\n", victim);

  move_char(ch, target, false);
  send_to_char("A vision forms in your mind... \n\r", ch);
  do_look(ch,"",15);
  move_char(ch, original_loc);
  return eSUCCESS;
}


/* EAGLE EYE */

int spell_eagle_eye(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int target;
  int original_loc;
  assert(ch && victim);

  if(!OUTSIDE(ch)) {
     send_to_char("Your eagle cannot find its way outside!\n\r", ch);
     return eFAILURE;
  }

  if(!OUTSIDE(victim) ||
     (GET_LEVEL(victim) >= IMMORTAL && GET_LEVEL(ch) < IMMORTAL))
  {
    send_to_char("Your eagle cannot scan the area.\n\r", ch);
    return eFAILURE;
  }

  if(IS_AFFECTED(victim, AFF_INVISIBLE)) {
    send_to_char("Your eagle can't find the target.\r\n", ch);
    return eFAILURE;
  }

  if (affected_by_spell(victim, SKILL_INNATE_EVASION))
  {
    send_to_char("Your target evades the eagle's eyes!\r\n",ch);
    return eFAILURE;
  }

  if(number(0, 100) > skill) {
    send_to_char("Your eagle fails to locate its target.\r\n", ch);
    return eFAILURE;
  }

  original_loc = ch->in_room;
  target        =  victim->in_room;

  move_char(ch, target, false);
  send_to_char("You summon a large eagle to scan the area.\n\rThrough the eagle's eyes you see...\n\r", ch);
  do_look(ch,"",15);
  move_char(ch, original_loc);
  return eSUCCESS;
}


/* SUMMON */

int spell_summon(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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
	 send_to_char("Something strange about that person prevents your summoning.\n\r", ch);
	 return eFAILURE;
  }
  if (IS_NPC(ch) && IS_NPC(victim)) return eFAILURE;

  if ((IS_NPC(victim) && GET_LEVEL(ch) < IMP) ||
		IS_SET(world[victim->in_room].room_flags,PRIVATE)  ||
		IS_SET(world[victim->in_room].room_flags,NO_SUMMON) ) {
	 send_to_char("You have failed to summon your target!\n\r", ch);
	 return eFAILURE;
  }

  if(IS_ARENA(ch->in_room)) 
    if(!IS_ARENA(victim->in_room))  {
       send_to_char("You can't summon someone INTO an arena!\n\r", ch);
       return eFAILURE;
    }

  if(IS_ARENA(victim->in_room) && !IS_ARENA(ch->in_room)) {
    send_to_char("You can't summon someone OUT of an areana!\n\r", ch);
    return eFAILURE;
  }

  if(number(1, 100) > 50 + skill/2 ||
	affected_by_spell(victim, FUCK_PTHIEF) || affected_by_spell(victim, FUCK_GTHIEF)) {
    send_to_char("Your attempted summoning fails.\r\n", ch);
    return eFAILURE;
  }

  if(IS_AFFECTED(victim, AFF_CHAMPION) && ( IS_SET(world[ch->in_room].room_flags, CLAN_ROOM) || ch->in_room >= 1900 && ch->in_room <= 1999) ) {
    send_to_char("You cannot summon a Champion here.\n\r", ch);
    return eFAILURE;
  }
    
  act("$n disappears suddenly as a magical summoning draws their being.",victim,0,0,TO_ROOM, INVIS_NULL);

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


/* CHARM PERSON - no longer operational */

int spell_charm_person(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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
		(IS_MOB(victim) && !ISSET(victim->mobdata->actflags, ACT_CHARM))) {
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


/* SENSE LIFE */

int spell_sense_life(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  assert(victim);

  if(affected_by_spell(victim, SPELL_SENSE_LIFE))
    affect_from_char(victim, SPELL_SENSE_LIFE);

  send_to_char("You feel your awareness improve.\n\r", victim);

  af.type      = SPELL_SENSE_LIFE;
  af.duration  = 18 + skill / 2;
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


/* IDENFITY */

// TODO - make this use skill to affect amount of information provided
int spell_identify(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

	 sprintf(buf,"Weight: %d, Value: %d, Level: %d\n\r", obj->obj_flags.weight, obj->obj_flags.cost, obj->obj_flags.eq_level);
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
		sprintf(buf, "Damage Dice are '%dD%d'\n\r",
	  obj->obj_flags.value[1],
	  obj->obj_flags.value[2]);
		send_to_char(buf, ch);
		break;

         case ITEM_INSTRUMENT:
                sprintf(buf, "Affects non-combat singing by '%d'\r\nAffects combat singing by '%d'\r\n",
                     obj->obj_flags.value[0],
                     obj->obj_flags.value[1]);
                send_to_char(buf, ch);
                break;

         case ITEM_MISSILE :
		sprintf(buf, "Damage Dice are '%dD%d'\n\rIt is +%d to arrow hit and +%d to arrow damage\r\n",
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

		if (obj->affected[i].location < 1000)
		 sprinttype(obj->affected[i].location,apply_types,buf2);
		else if (get_skill_name(obj->affected[i].location/1000))
		  strcpy(buf2, get_skill_name(obj->affected[i].location/1000));
		else strcpy(buf2, "Invalid");
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


/* ************************************************************************* *
 *                      NPC Spells (Breath Weapons)                          *
 * ************************************************************************* */

int spell_frost_breath(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

	 retval = damage(ch, victim, dam,TYPE_COLD, SPELL_FROST_BREATH, 0);
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


int spell_acid_breath(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

	 retval = damage(ch, victim, dam,TYPE_ACID, SPELL_ACID_BREATH, 0);
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


int spell_fire_breath(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int retval;
  CHAR_DATA *tmp_victim, *temp;

  act("$B$4You are $IENVELOPED$I$B$4 in scorching $B$4flames$R!$R", ch, 0, 0, TO_ROOM, 0);

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

      retval = damage(ch, tmp_victim, dam, TYPE_FIRE, SPELL_FIRE_BREATH, 0);
      if(IS_SET(retval, eCH_DIED))
        return retval;
    } else
        if (world[ch->in_room].zone == world[tmp_victim->in_room].zone)
          send_to_char("You feel a HOT blast of air.\n\r", tmp_victim);
  }
  return eSUCCESS;
}

int spell_gas_breath(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

		// if(saves_spell(ch,  tmp_victim, 0, SAVE_TYPE_POISON) >= 0)
		//	dam >>= 1;

		 retval = damage(ch, tmp_victim, dam,TYPE_POISON, SPELL_GAS_BREATH, 0);
                 if(IS_SET(retval, eCH_DIED))
                   return retval;
	 } else
		if (world[ch->in_room].zone == world[tmp_victim->in_room].zone)
			send_to_char("You wanna choke on the smell in the air.\n\r", tmp_victim);
  }
  return eSUCCESS;
}

int spell_lightning_breath(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
	 int dam;
	 int hpch;

	 set_cantquit( ch, victim );

	 hpch = GET_HIT(ch);
	 if(hpch<10) hpch=10;

	 dam = number((hpch/8)+1,(hpch/4));

//	 if(saves_spell(ch, victim, 0, SAVE_TYPE_ENERGY) >= 0)
//	   dam >>= 1;

	 return damage(ch, victim, dam,TYPE_ENERGY, SPELL_LIGHTNING_BREATH, 0);
}

/* **************************************************************** */


/* FEAR */

// TODO - make this use skill
int spell_fear(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
	int retval;
	 assert(victim && ch);
         char buf[256]; 
	 if(IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_STUPID)) {
           sprintf(buf, "%s doesn't understand your psychological tactics.\n\r",
                   GET_SHORT(victim));
           send_to_char(buf, ch);
	   return eFAILURE;
         }
         if(GET_POS(victim) == POSITION_SLEEPING) {
           send_to_char("How do you expect a sleeping person to be scared?\r\n", ch);
           return eFAILURE;
         }
         if(IS_AFFECTED(victim, AFF_FEARLESS)) {
            act("$N seems to be quite unafraid of anything.", ch, 0, victim, TO_CHAR, 0);
            act("You laugh at $n's feeble attempt to frighten you.", ch, 0, victim, TO_VICT, 0);
            act("$N laughs at $n's futile attempt at scaring $M.", ch, 0, victim, TO_ROOM, NOTVICT);
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
 set_cantquit( ch, victim );

   if (malediction_res(ch, victim, SPELL_FEAR)) {
      act("$N resists your attempt to scare $M!", ch, NULL, victim, TO_CHAR,0);
      act("$N resists $n's attempt to scare $M!", ch, NULL, victim, TO_ROOM,NOTVICT);
      act("You resist $n's attempt to scare you!",ch,NULL,victim,TO_VICT,0);
      if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > POSITION_SLEEPING) {
         retval = attack(victim, ch, TYPE_UNDEFINED);
         retval = SWAP_CH_VICT(retval);
         return retval;
      }

     return eFAILURE;
   }

	 if((saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) >= 0) || (!number(0, 5))) {
		send_to_char("For a moment you feel compelled to run away, but you fight back the urge.\n\r", victim);
		act("$N doesnt seem to be the yellow-bellied slug you thought!", ch, NULL, victim, TO_CHAR, 0);
		if (IS_NPC(victim)) {
		retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
		retval = SWAP_CH_VICT(retval);
		} else retval = eFAILURE;
		return retval;
	 }
	 send_to_char("You suddenly feel very frightened, and you attempt to flee!\n\r", victim);
	 do_flee(victim, "", 151);

  return eSUCCESS;
}


/* REFRESH */

int spell_refresh(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  if(!ch || !victim)
  {
    log("NULL ch or victim sent to spell_refresh!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  dam = dice(skill/2, 4) + skill/2;
  dam = MAX(dam, 20);

  if ((dam + GET_MOVE(victim)) > move_limit(victim))
    dam = move_limit(victim) - GET_MOVE(victim);

  GET_MOVE(victim) += dam;

  char buf[MAX_STRING_LENGTH];
  sprintf(buf, "$B%d$R", dam);

  if(ch != victim)
    send_damage("Your magic flows through $N and $E looks less tired by | fatigue points.",ch,0,victim,buf,"Your magic flows through $N and $E looks less tired.", TO_CHAR);

  send_damage("You feel less tired by | fatigue points.", ch, 0, victim, buf, "You feel less tired.", TO_VICT);
  send_damage("$N feels less tired by | fatigue points.", ch, 0, victim, buf, "$N feels less tired.", TO_ROOM);
  return eSUCCESS;
}


/* FLY */

int spell_fly(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  struct affected_type *cur_af;

  if(!ch || !victim)
  {
    log("NULL ch or victim sent to spell_fly!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if((cur_af = affected_by_spell(victim, SPELL_FLY)))
    affect_remove(victim, cur_af, SUPPRESS_ALL);

  if(IS_AFFECTED(victim, AFF_FLYING))
    return eFAILURE;

  send_to_char("You start flapping and rise off the ground!\n\r", victim);
  if (ch != victim)
    act("$N starts flapping and rises off the ground!", ch, NULL, victim, TO_CHAR, 0);
  act("$N's feet rise off the ground.", ch, 0, victim, TO_ROOM, INVIS_NULL|NOTVICT);
  
  af.type = SPELL_FLY;
  af.duration = 6 + skill / 2;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_FLYING;
  affect_to_char(victim, &af);
  return eSUCCESS;
}


/* CONTINUAL LIGHT */
// TODO - make this use skill for multiple effects

int spell_cont_light(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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

  tmp_obj = clone_object(real_object(6));

  obj_to_char(tmp_obj, ch);

  act("$n twiddles $s thumbs and $p suddenly appears.", ch, tmp_obj, 0, TO_ROOM, INVIS_NULL);
  act("You twiddle your thumbs and $p suddenly appears.", ch, tmp_obj, 0, TO_CHAR, 0);
  return eSUCCESS;
}


/* ANIMATE DEAD */
// TODO - make skill level have an affect on this

int spell_animate_dead(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *corpse, int skill)
{
  CHAR_DATA *mob;
  struct obj_data *obj_object, *next_obj;
  struct affected_type af;
  int number, r_num;

  if(!IS_EVIL(ch) && GET_LEVEL(ch) < ARCHANGEL && GET_CLASS(ch) == CLASS_ANTI_PAL) {
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

  if (GET_ALIGNMENT(ch) < 0) {
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
 } else {
  if(level < 20)
    number = 22389;
  else if(level < 30)
    number = 22390;
  else if(level < 40)
    number = 22391;
  else if(level < 50)
    number = 22392;
  else
    number = 22393;
 }

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
//all in the mob now, no need
//  sprintf(buf, "%s %s", corpse->name, mob->name);
//  mob->name = str_hsh(buf);
  
//  mob->short_desc = str_hsh(corpse->short_description);

//  if (GET_ALIGNMENT(ch) < 0) 
  //{
    //sprintf(buf, "%s slowly staggers around.\n\r", corpse->short_description);
//    mob->long_desc = str_hsh(buf);
  //}
//  else
 // sprintf(buf, "%s hovers above the ground here.\r\n",corpse->short_description);

  if (GET_ALIGNMENT(ch) < 0) {
  act("Calling upon your foul magic, you animate $p.\n\r$N slowly lifts "
      "itself to its feet.", ch, corpse, mob, TO_CHAR, INVIS_NULL);
  act("Calling upon $s foul magic, $n animates $p.\n\r$N slowly lifts "
      "itself to its feet.", ch, corpse, mob, TO_ROOM, INVIS_NULL);
  } else {
  act("Invoking your divine magic, you free $p's spirit.\n\r$N slowly rises "
      "out of the corpse and hovers a few feet above the ground.", ch, corpse, mob, TO_CHAR, INVIS_NULL);
  act("Invoking $s divine magic, $n releases $p's spirit.\n\r$N slowly rises "
      "out of the corpse and hovers a few feet above the ground.", ch, corpse, mob, TO_ROOM, INVIS_NULL);
  }

  // zombie should be charmed and follower ch
  // TODO duration needs skill affects too

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


/* KNOW ALIGNMENT */

int spell_know_alignment(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int duration;
  struct affected_type af, *cur_af;
  int learned = has_skill(ch, SPELL_KNOW_ALIGNMENT);

  if(!ch) {
    log("NULL ch sent to know_alignment!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if((cur_af = affected_by_spell(victim, SPELL_KNOW_ALIGNMENT)))
    affect_remove(victim, cur_af, SUPPRESS_ALL);

  if (learned <= 40)
    duration = level / 5;
  else if (learned > 40 && learned <= 60)
    duration = level / 4;
  else if (learned > 60 && learned <= 80)
    duration = level / 3;
  else if (learned > 80)
    duration = level /2;

  if (skill == 150) duration = 2;
  send_to_char("Your eyes tingle, allowing you to see the auras of other creatures.\n\r", ch);
  act("$n's eyes flash with knowledge and insight!", ch, 0, 0, TO_ROOM, INVIS_NULL);

  af.type      = SPELL_KNOW_ALIGNMENT;
  af.duration  = duration;
  af.modifier  = learned;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_KNOW_ALIGN;
  affect_to_char(ch, &af);

  return eSUCCESS;
}


/* DISPEL MINOR */

int spell_dispel_minor(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int rots = 0;
   int done = FALSE;
   int retval;

   if(obj && (unsigned int)obj > 100) /* Trying to dispel_minor an obj */
   {   // Heh, it passes spell cast through obj now too. Less than 100 = not 
       // an actual obj.
      if(GET_ITEM_TYPE(obj) != ITEM_BEACON) {
	 if (!obj->equipped_by && !obj->carried_by)
	 {
          send_to_char("You can't dispel that!\n\r", ch);
	  return eFAILURE;
	 }
	 if (IS_SET(obj->obj_flags.extra_flags, ITEM_INVISIBLE))
	 {
	  REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
	  send_to_char("You remove the item's invisibility.\r\n",ch);
	  return eSUCCESS;
	 } 
	 else if (IS_SET(obj->obj_flags.extra_flags, ITEM_GLOW))
	 {
	  REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_GLOW);
	  send_to_char("You remove the item's $Bglowing$R aura.\r\n",ch);
	  return eSUCCESS;
	 } 
	 else {
	  send_to_char("That item is not imbued with dispellable magic.\r\n",ch);
	  return eFAILURE;
	}
      }
      if(!obj->equipped_by) {
       // Someone load it or something?
         send_to_char("The magic fades away back to the ether.\n\r", ch);
         act("$p fades away gently.", ch, obj, 0, TO_ROOM, INVIS_NULL);
      }
      else {
         send_to_char("The magic is shattered by your will!\n\r", ch);
         act("$p blinks out of existence with a bang!", ch, obj, 0, TO_ROOM, INVIS_NULL);
         send_to_char("Your magic beacon is shattered!\n\r", obj->equipped_by);
         obj->equipped_by->beacon = NULL;
         obj->equipped_by = NULL;
      }
      extract_obj(obj);
      return eSUCCESS;
   }
   int spell = (int) obj;
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

   int savebonus = 0;
   int learned = has_skill(ch, SPELL_DISPEL_MINOR);
   if (learned < 41) savebonus = 20;
   else if (learned < 61) savebonus = 15;
   else if (learned < 81) savebonus = 10;
   else savebonus = 5;
    
   if (spell && savebonus != 5)
   {
      send_to_char("You do not know this spell well enough to target it.\r\n",ch);
      return eFAILURE;
   }
   if (spell)
     savebonus = 10; 

 set_cantquit( ch, victim );
   // If victim higher level, they get a save vs magic for no effect
   if (number(1,101) < get_saves(victim, SAVE_TYPE_MAGIC) + savebonus)
   {
	act("$N resists your attempt to dispel minor!", ch, NULL, victim, TO_CHAR,0);
	act("$N resists $n's attempt to dispel minor!", ch, NULL, victim, TO_ROOM, NOTVICT);
	act("You resist $n's attempt to dispel minor!",ch,NULL,victim,TO_VICT,0);
      if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > POSITION_SLEEPING) {
         retval = attack(victim, ch, TYPE_UNDEFINED);
         retval = SWAP_CH_VICT(retval);
         return retval;
      }

     return eFAILURE;
   }

   // Input max number of spells in switch statement here
   while(!done && ((rots += 1) < 10))
   {
      int x = spell != 0 ? spell: number(1,15);
      switch(x) 
      {
         case 1: 
            if (affected_by_spell(victim, SPELL_INVISIBLE))
            {
               affect_from_char(victim, SPELL_INVISIBLE);
               send_to_char("You feel your invisibility dissipate.\n\r", victim);
               act("$n fades into existence.", victim, 0, 0, TO_ROOM, 0);
               done = TRUE;
            }
            if (IS_AFFECTED(victim, AFF_INVISIBLE))
            {
               REMBIT(victim->affected_by, AFF_INVISIBLE);
               send_to_char("You feel your invisibility dissipate.\n\r", victim);
               act("$n fades into existence.", victim, 0, 0, TO_ROOM, 0);
               done = TRUE;
            }
            break;

	 case 2: 
            if (affected_by_spell(victim, SPELL_DETECT_INVISIBLE))
            {
               affect_from_char(victim, SPELL_DETECT_INVISIBLE);
               send_to_char("Your ability to detect invisible has been dispelled!\n\r", victim);
               act("$N's ability to detect invisible is removed.", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
            break;

	 case 3: 
            if (affected_by_spell(victim, SPELL_CAMOUFLAGE))
            {
               affect_from_char(victim, SPELL_CAMOUFLAGE);
               send_to_char("Your camouflage has been dispelled!.\n\r", victim);
               act("$N's form is less obscured as you dispel $S camouflage.", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
            break;

	 case 4: 
            if (affected_by_spell(victim, SPELL_RESIST_ACID))
            {
               affect_from_char(victim, SPELL_RESIST_ACID);
               send_to_char("The $2green$R in your skin is dispelled!\n\r", victim);
               act("$N's skin loses its $2green$R hue.", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
            break;

	 case 5: 
            if (affected_by_spell(victim, SPELL_RESIST_COLD))
            {
               affect_from_char(victim, SPELL_RESIST_COLD);
               send_to_char("The $3blue$R in your skin is dispelled!\n\r", victim);
               act("$N's skin loses its $3blue$R hue.", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
            break;

	 case 6: 
            if (affected_by_spell(victim, SPELL_RESIST_FIRE))
            {
               affect_from_char(victim, SPELL_RESIST_FIRE);
               send_to_char("The $4red$R in your skin is dispelled!\n\r", victim);
               act("$N's skin loses its $4red$R hue.", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
            break;

	 case 7: 
            if (affected_by_spell(victim, SPELL_RESIST_ENERGY))
            {
               affect_from_char(victim, SPELL_RESIST_ENERGY);
               send_to_char("The $5yellow$R in your skin is dispelled!\n\r", victim);
               act("$N's skin loses its $5yellow$R hue.", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
            break;

	 case 8: 
            if (affected_by_spell(victim, SPELL_BARKSKIN))
            {
               affect_from_char(victim, SPELL_BARKSKIN);
               send_to_char("Your woody has been dispelled!\n\r", victim);
               act("$N loses $S woody.", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
            break;

	 case 9: 
            if (affected_by_spell(victim, SPELL_STONE_SKIN))
            {
               affect_from_char(victim, SPELL_STONE_SKIN);
               send_to_char("Your skin loses stone-like consistency.\n\r", victim);
               act("$N's looks like less of a stoner as $S skin returns to normal.", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
            break;

	 case 10: 
            if (affected_by_spell(victim, SPELL_FLY))
            {
               affect_from_char(victim, SPELL_FLY);
               send_to_char("You do not feel lighter than air anymore.\n\r", victim);
               act("$N is no longer lighter than air.", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
 	    if (IS_AFFECTED(victim, AFF_FLYING))
            {
               REMBIT(victim->affected_by, AFF_FLYING);
               send_to_char("You do not feel lighter than air anymore.\n\r", victim);
               act("$n drops to the ground, no longer lighter than air.", victim, 0, 0, TO_ROOM, 0);
               done = TRUE;
            }
            break;
 
	 case 11: 
            if (affected_by_spell(victim, SPELL_TRUE_SIGHT))
            {
               affect_from_char(victim, SPELL_TRUE_SIGHT);
               send_to_char("You no longer see what is hidden.\n\r", victim);
               act("$N no longer sees what is hidden.", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
            break;
      
	 case 12: 
            if (affected_by_spell(victim, SPELL_WATER_BREATHING))
            {
               affect_from_char(victim, SPELL_WATER_BREATHING);
               send_to_char("You can no longer breathe underwater!\n\r", victim);
               act("$N can no longer breathe underwater!", ch,0,victim, TO_CHAR, 0);
               done = TRUE;
            }
            break;
         case 13:
	   if (affected_by_spell(victim, SPELL_ARMOR))
	   {
	      affect_from_char(victim, SPELL_ARMOR);
		done = TRUE;
		send_to_char("Your magical armour is dispelled!\r\n",victim);
		act("$N's magical armour is dispelled!",ch,0,victim, TO_CHAR, 0);
	   }
	   break;
	 case 14:
	   if (affected_by_spell(victim, SPELL_SHIELD))
	   {
		affect_from_char(victim, SPELL_SHIELD);
		done = TRUE;
		send_to_char("Your force shield shimmers and fades away.\r\n",victim);
		act("$N's force shield shimmers and fades away.",ch,0,victim, TO_CHAR, 0);
	   }
	   break;

	 case 15: 
            if (affected_by_spell(victim, SPELL_RESIST_MAGIC))
            {
               affect_from_char(victim, SPELL_RESIST_MAGIC);
               send_to_char("The $B$7white$R in your skin is dispelled!\n\r", victim);
               act("$N's skin loses its $B$7white$R hue.", ch,0,victim, TO_CHAR, 0);
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


/* DISPEL MAGIC */

int spell_dispel_magic(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int rots = 0;
   int done = FALSE;
   int retval;
   int spell = (int) obj;
   if(!ch || !victim)
   {
      log("Null ch or victim sent to dispel_magic!", ANGEL, LOG_BUG);
      return eFAILURE;
   }

   set_cantquit(ch, victim);

   if(ISSET(victim->affected_by, AFF_GOLEM)) {
      send_to_char("The golem seems to shrug off your dispel attempt!\r\n", ch);
      act("The golem seems to ignore $n's dispelling magic!", ch, 0, 0, TO_ROOM, 0);
      return eFAILURE;
   }
   /*
   if(!IS_NPC(ch) && !IS_NPC(victim) && victim->fighting &&
       IS_NPC(victim->fighting) && 
      !IS_AFFECTED(victim->fighting, AFF_CHARM)) 
   {
      send_to_char("Your dispelling magic misfires!\n\r", ch);
      victim = ch;
   }*/
   int savebonus = 0;
   int learned = has_skill(ch, SPELL_DISPEL_MAGIC);
        if (learned < 41) savebonus = 20;
   else if (learned < 61) savebonus = 15;
   else if (learned < 81) savebonus = 10;
   else savebonus = 5;

   if (spell && savebonus != 5)
   {
      send_to_char("You do not yet know this spell well enough to target it.\r\n",ch);
      return eFAILURE;
   }
   if (spell)
     savebonus = 20;

// If victim higher level, they get a save vs magic for no effect
//      if((GET_LEVEL(victim) > GET_LEVEL(ch)) && 0 > saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC))
//          return eFAILURE;
   if (number(1,101) < get_saves(victim, SAVE_TYPE_MAGIC) + savebonus && level != GET_LEVEL(ch)-1)
   {
	act("$N resists your attempt to dispel magic!", ch, NULL, victim, TO_CHAR,0);
	act("$N resists $n's attempt to dispel magic!", ch, NULL, victim, TO_ROOM,NOTVICT);
	act("You resist $n's attempt to dispel magic!",ch,NULL,victim,TO_VICT,0);
      if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > POSITION_SLEEPING) {
         retval = attack(victim, ch, TYPE_UNDEFINED);
         retval = SWAP_CH_VICT(retval);
         return retval;
      }

     return eFAILURE;
   }


// Number of spells in the switch statement goes here
   while(!done && ((rots += 1) < 15))
   {
     int x = spell != 0? spell : number(1,10);
     switch(x) 
     {
        case 1: 
           if (affected_by_spell(victim, SPELL_SANCTUARY))
           {
              affect_from_char(victim, SPELL_SANCTUARY);
	      act("You don't feel so invulnerable anymore.", ch, 0, victim, TO_VICT, 0);
              act("The $B$7white glow$R around $n's body fades.", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           if (IS_AFFECTED(victim, AFF_SANCTUARY))
           {
              REMBIT(victim->affected_by, AFF_SANCTUARY);
              act("You don't feel so invunerable anymore.", ch, 0,victim, TO_VICT, 0);
              act("The $B$7white glow$R around $n's body fades.", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           break;

        case 2: 
           if (affected_by_spell(victim, SPELL_PROTECT_FROM_EVIL))
           {
              affect_from_char(victim, SPELL_PROTECT_FROM_EVIL);
              act("Your protection from evil has been dispelled!", ch, 0,victim, TO_VICT, 0);
              act("The dark, $6pulsing$R aura surrounding $n has been dispelled!", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           break;

        case 3: 
           if (affected_by_spell(victim, SPELL_HASTE))
           {
              affect_from_char(victim, SPELL_HASTE);
              act("Your magically enhanced speed has been dispelled!", ch, 0,victim, TO_VICT, 0);
              act("$n's actions slow to their normal speed.", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           break;

        case 4: 
           if (affected_by_spell(victim, SPELL_STONE_SHIELD)) 
           {
              affect_from_char(victim, SPELL_STONE_SHIELD);
              act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0,victim, TO_VICT, 0);
              act("The shield of stones swirling about $n's body fall to the ground!", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           break;

	 case 5: 
           if (affected_by_spell(victim, SPELL_GREATER_STONE_SHIELD))
           {
              affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);
              act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0,victim, TO_VICT, 0);
              act("The shield of stones swirling about $n's body falls to the ground!", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           break;

	 case 6: 
	   if (IS_AFFECTED(victim, AFF_FROSTSHIELD))
           {
	      REMBIT(victim->affected_by, AFF_FROSTSHIELD);
              act("Your shield of $B$3frost$R melts into nothing!.", ch, 0,victim, TO_VICT, 0);
              act("The $B$3frost$R encompassing $n's body melts away.", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           break;

	 case 7: 
           if (affected_by_spell(victim, SPELL_LIGHTNING_SHIELD))
           {
              affect_from_char(victim, SPELL_LIGHTNING_SHIELD);
              act("Your crackling shield of $B$5electricity$R vanishes!", ch, 0,victim, TO_VICT, 0);
              act("The $B$5electricity$R crackling around $n's body fades away.", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           break;
	 case 8: 
           if (affected_by_spell(victim, SPELL_FIRESHIELD))
           {
              affect_from_char(victim, SPELL_FIRESHIELD);
              act("Your $B$4flames$R have been extinguished!", ch, 0,victim, TO_VICT, 0);
              act("The $B$4flames$R encompassing $n's body are extinguished!", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           if (IS_AFFECTED(victim, AFF_FIRESHIELD))
           {
              REMBIT(victim->affected_by, AFF_FIRESHIELD);
              act("Your $B$4flames$R have been extinguished!", ch, 0,victim, TO_VICT, 0);
              act("The $B$4flames$R encompassing $n's body are extinguished!", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
	   }
           break;
	 case 9: 
           if (affected_by_spell(victim, SPELL_ACID_SHIELD))
           {
              affect_from_char(victim, SPELL_ACID_SHIELD);
              act("Your shield of $B$2acid$R dissolves to nothing!", ch, 0,victim, TO_VICT, 0);
              act("The $B$2acid$R swirling about $n's body dissolves to nothing!", victim, 0, 0, TO_ROOM, 0);
              done = TRUE;
           }
           break;
      
        case 10:
           if (affected_by_spell(victim, SPELL_PROTECT_FROM_GOOD))
           {
              affect_from_char(victim, SPELL_PROTECT_FROM_GOOD);
              act("Your protection from good has been dispelled!", ch, 0,victim, TO_VICT, 0);
              act("The light, $B$6pulsing$R aura surrounding $n has been dispelled!", victim, 0, 0, TO_ROOM, 0);
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


/* CURE SERIOUS */

int spell_cure_serious(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int healpoints;
  char buf[MAX_STRING_LENGTH],dammsg[MAX_STRING_LENGTH];

  if(!ch || !victim) {
    log("Null ch or victim sent to cure_serious!", ANGEL, LOG_BUG);
    return eFAILURE;
  }

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Healing spells are useless on the undead.\n\r", ch);
    return eFAILURE;
  }
  if (GET_RACE(victim) == RACE_GOLEM) {
        send_to_char("The heavy magics surrounding this being prevent healing.\r\n",ch);
        return eFAILURE;
  }

  if (!can_heal(ch,victim, SPELL_CURE_SERIOUS)) return eFAILURE;
      healpoints = dam_percent(skill,50);
  if ((healpoints + GET_HIT(victim)) > hit_limit(victim)) {
    healpoints = hit_limit(victim) - GET_HIT(victim);
    GET_HIT(victim) = hit_limit(victim);
  }
  else
    GET_HIT(victim) += healpoints;

  update_pos(victim);

  sprintf(dammsg,"$B%d$R damage", healpoints);

  if (ch!=victim) {
     sprintf(buf,"You heal %s of the more serious wounds on $N.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"several":"several");
     act(buf,ch,0,victim,TO_CHAR,0);
     sprintf(buf,"$n heals %s of your more serious wounds.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"several":"several");
     act(buf,ch,0,victim, TO_VICT, 0);
     sprintf(buf,"$n heals | of the more serious wounds on $N.");
     send_damage(buf, ch, 0, victim, dammsg,"$n heals several of the more serious wounds on $N.", TO_ROOM);
  } else {
     sprintf(buf,"You heal %s of your more serious wounds.",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"several":"several");
     act(buf,ch,0,0,TO_CHAR,0);
     sprintf(buf,"$n heals | of $s more serious wounds.");
     send_damage(buf,ch,0,victim,dammsg,"$n heals several of $s more serious wounds.",TO_ROOM);
  }

  return eSUCCESS;
}


/* CAUSE LIGHT */

int spell_cause_light(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;
   
   if (!ch || !victim) {
      log("Null ch or victim sent to cause_light", ANGEL, LOG_BUG);
      return eFAILURE;
   }

   set_cantquit(ch, victim);
   dam = dice(1, 8) + (skill/3);
   return damage(ch, victim, dam, TYPE_MAGIC, SPELL_CAUSE_LIGHT, 0);
}


/* CAUSE CRITICAL */

int spell_cause_critical(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    int dam;

    if(!ch || !victim)
    {
      log("NULL ch or victim sent to cause_critical!", ANGEL, LOG_BUG);
      return eFAILURE;
    }

    set_cantquit(ch, victim);
    
    dam = dice(3,8)-6+skill/2;

    return damage(ch, victim, dam, TYPE_MAGIC, SPELL_CAUSE_CRITICAL, 0);
}


/* CAUSE SERIOUS */

int spell_cause_serious(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    int dam;
    if(!ch || !victim)
    {
      log("NULL ch or victim sent to cause_serious!", ANGEL, LOG_BUG);
      return eFAILURE;
    }

    set_cantquit(ch, victim);

    dam = dice(2, 8) + (skill/2);

   return damage(ch, victim, dam,TYPE_MAGIC, SPELL_CAUSE_SERIOUS, 0);
}


/* FLAMESTRIKE */

int spell_flamestrike(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam, retval;

   if (!ch || !victim) {
      log("NULL ch or victim sent to flamestrike!", ANGEL, LOG_BUG);
      return eFAILURE;
   }

   set_cantquit (ch, victim);
   dam = 400;

   retval = damage(ch, victim, dam, TYPE_FIRE, SPELL_FLAMESTRIKE, 0);

   if(SOMEONE_DIED(retval))
      return retval;
   if(IS_SET(retval,eEXTRA_VAL2)) victim = ch;
   if(IS_SET(retval,eEXTRA_VALUE)) return retval;
// Burns up ki and mana on victim if learned over skill level 70

   if(skill > 70) {
      char buf[MAX_STRING_LENGTH];
      int mamount = number(50, 75), kamount = number(1,7);
      if(GET_MANA(victim) - mamount < 0) mamount = GET_MANA(victim);
      if(GET_KI(victim) - kamount < 0) kamount = GET_KI(victim);
      GET_MANA(victim) -= mamount;
      GET_KI(victim) -= kamount;
      sprintf(buf, "$B%d$R mana and $B%d$R ki", mamount, kamount);
      send_damage("$n's fires $4burn$R into your soul tearing away at your being for |.", ch, 0, victim, buf, "The fires $4burn$R into your soul tearing away at your being.", TO_VICT);
      send_damage("Your fires $4burn$R into $N's soul, tearing away at $S being for |.",ch,0,victim,buf, "Your fires $4burn$R $N's soul, tearing away at $S being.", TO_CHAR);
   }
   return retval;
}


/* IRIDESCENT AURA */

int spell_iridescent_aura(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   struct affected_type af;

   if (!affected_by_spell(victim, SPELL_IRIDESCENT_AURA)) {
      act("The air around $n shimmers and an $B$3i$7r$3i$7d$3e$7s$3c$7e$3n$7t$R aura appears around $m.", victim, 0, 0, TO_ROOM, INVIS_NULL);
      act("The air around you shimmers and an $B$3i$7r$3i$7d$3e$7s$3c$7e$3n$7t$R aura appears around you.",victim,0,0, TO_CHAR,0);
      af.type = SPELL_IRIDESCENT_AURA;
      af.duration = 1 + skill / 10;
      af.modifier = skill/15;
      af.bitvector = -1;
      af.location = APPLY_SAVES;
      affect_to_char(victim, &af);
   }
   return eSUCCESS;
}


/* RESIST COLD */

int spell_resist_cold(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   struct affected_type af;
   if (GET_CLASS(ch) == CLASS_PALADIN && ch != victim)
   {
	send_to_char("You can only cast this on yourself.\r\n",ch);
	return eFAILURE;
   }

   if (!affected_by_spell(victim, SPELL_RESIST_COLD)) {
      act("$n's skin turns $3blue$R momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
      act("Your skin turns $3blue$R momentarily.", victim, 0, 0, TO_CHAR, 0);

      af.type = SPELL_RESIST_COLD;
      af.duration = 1 + skill / 10;
      af.modifier = 10 + skill / 6;
      af.location = APPLY_SAVING_COLD;
      af.bitvector = -1;
      affect_to_char(victim, &af);
   }
   return eSUCCESS;
}


/* RESIST FIRE */

int spell_resist_fire(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   struct affected_type af;

   if (GET_CLASS(ch) == CLASS_MAGIC_USER && ch != victim)
   {
	send_to_char("You can only cast this on yourself.\r\n",ch);
	return eFAILURE;
   }

   if (!affected_by_spell(victim, SPELL_RESIST_FIRE)) {
      act("$n's skin turns $4red$R momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
      act("Your skin turns $4red$R momentarily.", victim, 0, 0, TO_CHAR, 0);
      af.type = SPELL_RESIST_FIRE;
      af.duration = 1 + skill / 10;
      af.modifier = 10 + skill / 6;
      af.location = APPLY_SAVING_FIRE;
      af.bitvector = -1;
      affect_to_char(victim, &af);
   }
   return eSUCCESS;
}


/* RESIST MAGIC */

int spell_resist_magic(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   struct affected_type af;

   if (GET_CLASS(ch) == CLASS_MAGIC_USER && ch != victim)
   {
	send_to_char("You can only cast this on yourself.\r\n",ch);
	return eFAILURE;
   }


   if (!affected_by_spell(victim, SPELL_RESIST_MAGIC)) {
      act("$n's skin turns $B$7white$R momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
      act("Your skin turns $B$7white$R momentarily.", victim, 0, 0, TO_CHAR, 0);
      af.type = SPELL_RESIST_MAGIC;
      af.duration = 1 + skill / 10;
      af.modifier = 10 + skill / 6;
      af.location = APPLY_SAVING_MAGIC;
      af.bitvector = -1;
      affect_to_char(victim, &af);
   }
   return eSUCCESS;
}


/* STAUNCHBLOOD */

int spell_staunchblood(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;

   if (GET_CLASS(ch) == CLASS_RANGER && ch != victim)
   {
	send_to_char("You can only cast this on yourself.\r\n",ch);
	return eFAILURE;
   }
    if(!affected_by_spell(ch, SPELL_STAUNCHBLOOD)) {
       act("You feel supremely healthy and resistant to $2poison$R!", ch, 0, 0, TO_CHAR, 0);
       act("$n looks supremely healthy and begins looking for snakes and spiders to fight.", victim, 0, 0, TO_ROOM, INVIS_NULL);
       af.type = SPELL_STAUNCHBLOOD;
       af.duration = 1 + skill / 10;
       af.modifier = 10 + skill / 6;
       af.location = APPLY_SAVING_POISON;
       af.bitvector = -1;
       affect_to_char(ch, &af);
    }
  return eSUCCESS;
}


/* RESIST ENERGY */

int spell_resist_energy(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   struct affected_type af;

   if(!affected_by_spell(victim, SPELL_RESIST_ENERGY)) {
      act("$n's skin turns $5yellow$R momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
      act("Your skin turns $5yellow$R momentarily.", victim, 0, 0, TO_CHAR, 0);
      af.type = SPELL_RESIST_ENERGY;
      af.duration = 1 + skill / 10;
      af.modifier = 10 + skill / 6;
      af.location = APPLY_SAVING_ENERGY;
      af.bitvector = -1;
      affect_to_char(victim, &af);
   }
   return eSUCCESS;
}


/* STONE SKIN */

int spell_stone_skin(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;

    if(!ch)
    {
      log("NULL ch sent to cause_serious!", ANGEL, LOG_BUG);
        return eFAILURE;
    }

    if (!affected_by_spell(ch, SPELL_STONE_SKIN)) {
	act("$n's skin turns grey and stone-like.", ch, 0, 0, TO_ROOM, INVIS_NULL);
	act("Your skin turns to a stone-like substance.", ch, 0, 0, TO_CHAR, 0);

	af.type = SPELL_STONE_SKIN;
	af.duration = 3 + skill/6;
	af.modifier = -(10 + skill/3);
	af.location = APPLY_AC;
	af.bitvector = -1;
	affect_to_char(ch, &af);

        SET_BIT(ch->resist, ISR_PIERCE);
  }
  return eSUCCESS;
}


/* SHIELD */

int spell_shield(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;

    if(!ch || !victim)
    {
      log("NULL ch or victim sent to shield!", ANGEL, LOG_BUG);
        return eFAILURE;
     }

  if(affected_by_spell(victim, SPELL_SHIELD))
     affect_from_char(victim, SPELL_SHIELD);

  act("$N is surrounded by a strong force shield.", ch, 0, victim, TO_ROOM, INVIS_NULL|NOTVICT);
  if (ch != victim) {
     act("$N is surrounded by a strong force shield.", ch, 0, victim, TO_CHAR, 0);
     act("You are surrounded by a strong force shield.", ch, 0, victim, TO_VICT, 0);
  } else {
     act("You are surrounded by a strong force shield.", ch, 0, victim, TO_VICT, 0);
  }

  af.type = SPELL_SHIELD;
  af.duration = 6 + skill / 3;
  af.modifier = - ( 9 + skill / 15 );
  af.location = APPLY_AC;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  return eSUCCESS;
}


/* WEAKEN */

int spell_weaken(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;
    struct affected_type * cur_af;
    int retval;
    int duration = 0, str = 0, con = 0;
    int learned = has_skill(ch, SPELL_WEAKEN);
    void check_weapon_weights(char_data * ch);

    if(!ch || !victim)
    {
      log("NULL ch or victim sent to weaken!", ANGEL, LOG_BUG);
        return eFAILURE;
    }

     if (learned < 40) { 
       duration = 2; 
       str = -4; 
       con = -1; 
     } else if (learned > 40 && learned <= 60) { 
       duration = 3; 
       str = -8; 
       con = -2; 
     } else if (learned > 60 && learned <= 80) { 
       duration = 4; 
       str = -10; 
       con = -3; 
     } else if (learned > 80) {
       duration = 5; 
       str = -12;  
       con = -4;
     }
   
    set_cantquit (ch, victim);
   if (malediction_res(ch, victim, SPELL_WEAKEN)) {
	act("$N resists your attempt to weaken $M!", ch, NULL, victim, TO_CHAR,0);
	act("$N resists $n's attempt to weaken $M!", ch, NULL, victim, TO_ROOM,NOTVICT);
	act("You resist $n's attempt to weaken you!",ch,NULL,victim,TO_VICT,0);
   } else {
	 if (!affected_by_spell(victim, SPELL_WEAKEN))
      {
	  act("You feel weaker.", ch, 0, victim, TO_VICT, 0);
	  act("$n seems weaker.", victim, 0, 0, TO_ROOM, INVIS_NULL);

	  if((cur_af = affected_by_spell(victim, SPELL_STRENGTH)))
	  {
	    cur_af->modifier += str;
	    af.type      = cur_af->type;
	    af.duration  = cur_af->duration;
	    af.modifier  = cur_af->modifier;
	    af.location  = cur_af->location;
	    af.bitvector = cur_af->bitvector;
	    affect_from_char( victim, SPELL_STRENGTH );  // this makes cur_af invalid
	    if( af.modifier > 0 ) // it's not out yet
	       affect_to_char( victim, &af );
	    return eSUCCESS;
	  }

	  af.type = SPELL_WEAKEN;
	  af.duration = duration;
	  af.modifier = str;
	  af.location = APPLY_STR;
	  af.bitvector = -1;
	  affect_to_char(victim, &af);
	  af.modifier = con;
	  af.location = APPLY_CON;
	  affect_to_char(victim, &af);

	  check_weapon_weights(victim);
	}
     }
	if (IS_NPC(victim)) {
	 retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
         retval = SWAP_CH_VICT(retval);
         return retval;
	}
  return eSUCCESS;
}


/* MASS INVISIBILITY */

// TODO - make this use skill        
int spell_mass_invis(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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


/* ACID BLAST */

int spell_acid_blast(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;

   set_cantquit (ch, victim);
   dam = 375;
   return damage(ch, victim, dam,TYPE_ACID, SPELL_ACID_BLAST, 0);
}


/* HELLSTREAM */

int spell_hellstream(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;

   set_cantquit (ch, victim);
   dam = 800;
   return damage(ch, victim, dam,TYPE_FIRE, SPELL_HELLSTREAM, 0);
}


/* PORTAL (Creates the portal "item") */

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
           IS_SET(world[destination].room_flags, NO_TELEPORT) ||
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


/* PORTAL (Actual spell) */
// TODO - make this use skill        

int spell_portal(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct obj_data *portal = 0;

  if(IS_SET(world[victim->in_room].room_flags, PRIVATE) ||
     IS_SET(world[victim->in_room].room_flags, IMP_ONLY) ||
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
  if(IS_AFFECTED(victim, AFF_SHADOWSLIP)) {
    send_to_char("You can't seem to find a definite path.\n\r", ch);
    if(GET_CLASS(ch) == CLASS_CLERIC)
      GET_MANA(ch) += 100;
    else
      GET_MANA(ch) += 25;
    return eFAILURE;
  }
  if(IS_SET(zone_table[world[victim->in_room].zone].zone_flags, ZONE_NO_TELEPORT))
  {
    send_to_char("A portal shimmers into view but is unstable and immediately fades to nothing.\n\r",ch);
    act("A portal shimmers into view but is unstable and immediately fades to nothing.", ch, 0, 0, TO_ROOM, 0);
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
      GET_MANA(ch) += 125;
    else
      GET_MANA(ch) += 50;
    return eFAILURE;
  }

  make_portal(ch, victim);
  return eSUCCESS;
}


/* BURNING HANDS (scroll, wand) */

int cast_burning_hands( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
 int retval ;
  char arg1[MAX_STRING_LENGTH];
  arg1[0] = '\0';
	 switch (type)
	 {
	 case SPELL_TYPE_SPELL:
	 retval = spell_burning_hands(level, ch, victim, 0, skill);
//	 if (SOMEONE_DIED(retval)) return retval;
	 one_argument(arg, arg1);
	 CHAR_DATA *vict;
         if (arg1[0] && spellcraft(ch, SPELL_BURNING_HANDS))
	   vict = get_char_room_vis(ch, arg1);
	else vict = NULL;
	if (!vict || vict == victim) return retval;
	return spell_burning_hands(level, ch, vict, 0, skill);
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


/* CALL LIGHTNING (potion, scroll, staff) */

int cast_call_lightning( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* CHILL TOUCH (scroll, wand) */

int cast_chill_touch( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* SHOCKING GRASP (scroll, wand) */

int cast_shocking_grasp( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* COLOUR SPRAY (scroll, wand) */

int cast_colour_spray( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* DROWN (scroll, potion, wand) */

int cast_drown( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* EARTHQUAKE (scroll, staff) */

int cast_earthquake( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* LIFE LEECH (scroll, staff) */

int cast_life_leech( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* HEROES FEAST (staff, scroll, potion, wand) */

int cast_heroes_feast( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* HEAL SPRAY (scroll, staff) */

int cast_heal_spray( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* GROUP SANCTUARY (scroll, staff) */

int cast_group_sanc( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* GROUP RECALL (scroll, staff) */ 

int cast_group_recall( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* GROUP FLY (scroll, staff) */

int cast_group_fly( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* FIRESTORM (scroll, staff) */

int cast_firestorm( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* SOLAR GATE (scroll, staff) */

int cast_solar_gate( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* ENERGY DRAIN (potion, scroll, wand, staff) */

int cast_energy_drain( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* SOULDRAIN (potion, scroll, wand, staff) */

int cast_souldrain( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* VAMPIRIC TOUCH (scroll, wand) */

int cast_vampiric_touch( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* METEOR SWARM (scroll, wand) */

int cast_meteor_swarm( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* FIREBALL (scroll, wand) */

int cast_fireball( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* SPARKS (scroll, wand) */

int cast_sparks( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* HOWL (scroll, wand) */

int cast_howl( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* HARM (potion, staff) */

int cast_harm( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* POWER HARM (potion, wand, staff) */

int cast_power_harm( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* DIVINE FURY (potion, staff) */

int cast_divine_fury( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *victim, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  switch (type) {
  case SPELL_TYPE_SPELL:
    return spell_divine_fury(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_divine_fury(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (victim = world[ch->in_room].people ; victim ; victim = next_v )
    {
      next_v = victim->next_in_room;

      if ( !ARE_GROUPED(ch, victim) )
      {
        retval = spell_divine_fury(level, ch, victim, 0, skill);
        if(IS_SET(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default :
    log("Serious screw-up in divine fury!", ANGEL, LOG_BUG);
    break;
  }
  return eFAILURE;
}


/* LIGHTNING BOLT (scroll, wand) */

int cast_lightning_bolt( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* MAGIC MISSILE (scroll, wand) */

int cast_magic_missile( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* ARMOR (potion, scroll, wand) */

int cast_armor( ubyte level, CHAR_DATA *ch, char *arg, int type,
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
		 return spell_armor(level,ch,tar_ch,0, skill);
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

/* HOLY AEGIS/UNHOLY AEGIS (potion, scroll, wand) */

int cast_aegis( ubyte level, CHAR_DATA *ch, char *arg, int type,
	 CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	case SPELL_TYPE_SPELL:
		 return spell_aegis(level,ch,ch,0, skill);
		 break;
	case SPELL_TYPE_POTION:
		 return spell_aegis(level,ch,ch,0, skill);
		 break;
	case SPELL_TYPE_SCROLL:
		 if (tar_obj) return eFAILURE;
		 if (!tar_ch) tar_ch = ch;
		 return spell_aegis(level,ch,ch,0, skill);
		 break;
	case SPELL_TYPE_WAND:
		 if (tar_obj) return eFAILURE;
		 if (!tar_ch) tar_ch = ch;
		 if ( affected_by_spell(tar_ch, SPELL_AEGIS) )
		return eFAILURE;
		 return spell_aegis(level,ch,tar_ch,0, skill);
		 break;
	default :
	 log("Serious screw-up in aegis!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


/* TELEPORT (potion, scroll, wand, staff) */

int cast_teleport( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* BLESS (potion, scroll, wand) */

int cast_bless( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* PARALYZE (potion, scroll, wand, staff) */

int cast_paralyze( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* BLINDNESS (potion, scroll, wand, staff) */

int cast_blindness( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* CONTROL WEATHER */

int cast_control_weather( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

		 if(!str_cmp("better",buffer)) {
		weather_info.change+=(dice(((level)/3),9));
		send_to_char("The skies around you grow a bit clearer.\n\r",ch);
		act("$n's magic causes the skies around you to grow a bit clearer.", ch, 0, 0, TO_ROOM, 0);
		}
		 else
		{ weather_info.change-=(dice(((level)/3),9));
		send_to_char("The skies around you grow a bit darker.\n\r",ch);
		act("$n's magic causes the skies around you to grow a bit darker.", ch, 0, 0, TO_ROOM, 0);
		}
		 break;
		default :
	 log("Serious screw-up in control weather!", ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}


/* CREATE FOOD (wand, staff, scroll) */

int cast_create_food( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{

  switch (type) {
	 case SPELL_TYPE_SPELL:
	    act("$n magically creates a mushroom.",ch, 0, 0, TO_ROOM, 0);
	    return spell_create_food(level,ch,0,0, skill);
	    break;
         case SPELL_TYPE_WAND:
            if(tar_obj) return eFAILURE;
            if(tar_ch) return eFAILURE;
            return spell_create_food(level,ch,0,0, skill);
            break;
         case SPELL_TYPE_POTION:
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


/* CREATE WATER (wand, scroll) */

int cast_create_water( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* REMOVE PARALYSIS (potion, wand, scroll, staff) */

int cast_remove_paralysis( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* REMOVE BLIND (potion, scroll, staff) */

int cast_remove_blind( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_remove_blind(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_POTION:
	 return spell_remove_blind(level,ch,ch,0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_obj) return eFAILURE;
	 if (!tar_ch) tar_ch = ch;
	 return spell_remove_blind(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_STAFF:
	 for (tar_ch = world[ch->in_room].people ;
	  tar_ch ; tar_ch = tar_ch->next_in_room)

	 spell_remove_blind(level,ch,tar_ch,0, skill);
	 break;
	 default :
		log("Serious screw up in remove blind!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}

/* EDITING ENDS HERE */

int cast_cure_critic( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_cure_light( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_cure_light(level,ch,tar_ch,0, skill);
	 break;
  case SPELL_TYPE_WAND:
    if(!tar_ch) return eFAILURE;
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


int cast_curse( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  int retval;

  if (IS_SET(world[ch->in_room].room_flags, SAFE) && tar_ch){
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
	case SPELL_TYPE_WAND:
		if (tar_obj)
		  return spell_curse(level, ch, 0, tar_obj, skill);
		else {
		if (!tar_ch) tar_ch = ch;
		return spell_curse(level, ch, tar_ch, 0, skill);
		}
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


int cast_detect_evil( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_true_sight( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_detect_good( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


int cast_detect_invisibility( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_detect_magic( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_haste( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_detect_poison( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_dispel_evil( ubyte level, CHAR_DATA *ch, char *arg, int type,
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




int cast_dispel_good( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


int cast_enchant_armor( ubyte level, CHAR_DATA *ch, char *arg, int type,
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




int cast_enchant_weapon( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


int cast_mana( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


int cast_heal( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
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



int cast_power_heal( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
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


int cast_full_heal( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
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



int cast_invisibility( ubyte level, CHAR_DATA *ch, char *arg, int type,
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




int cast_locate_object( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_locate_object(level, ch, arg, 0, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
	 if (tar_ch) return eFAILURE;
	 return spell_locate_object(level, ch, arg, 0, 0, skill);
	 break;
  default:
         log("Serious screw-up in locate object!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_poison( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  int retval;
  char_data * next_v;

  switch (type) {
  case SPELL_TYPE_SPELL:
    if (IS_SET(world[ch->in_room].room_flags, SAFE)){
	 send_to_char("You can not poison someone in a safe area!\n\r",ch);
	 return eFAILURE;
    }
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


int cast_protection_from_evil( ubyte level, CHAR_DATA *ch, char *arg, int type,
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
      return spell_protection_from_evil(level, ch, ch, 0, 0);
      break;
    case SPELL_TYPE_WAND:
      if(tar_obj) return eFAILURE;
      return spell_protection_from_evil(level, ch, tar_ch, 0, 0);
    case SPELL_TYPE_SCROLL:
      if(tar_obj) return eFAILURE;
      if(!tar_ch) tar_ch = ch;
        return spell_protection_from_evil(level, ch, tar_ch, 0, 0);
      break;
    case SPELL_TYPE_STAFF:
      for (tar_ch = world[ch->in_room].people; tar_ch ; tar_ch = tar_ch->next_in_room)
        spell_protection_from_evil(level,ch,tar_ch,0, 0);
      break;
    default :
      log("Serious screw-up in protection from evil!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}


int cast_protection_from_good( ubyte level, CHAR_DATA *ch, char *arg, int type,
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
      return spell_protection_from_good(level, ch, ch, 0, 0);
      break;
    case SPELL_TYPE_WAND:
      if(tar_obj) return eFAILURE;
      return spell_protection_from_good(level, ch, tar_ch, 0, 0);
    case SPELL_TYPE_SCROLL:
      if(tar_obj) return eFAILURE;
      if(!tar_ch) tar_ch = ch;
        return spell_protection_from_good(level, ch, tar_ch, 0, 0);
      break;
    case SPELL_TYPE_STAFF:
      for (tar_ch = world[ch->in_room].people; tar_ch ; tar_ch = tar_ch->next_in_room)
        spell_protection_from_good(level,ch,tar_ch,0, 0);
      break;
    default :
      log("Serious screw-up in protection from good!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}

int cast_remove_curse( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_remove_poison( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_fireshield( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


int cast_sleep( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


int cast_strength( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


int cast_ventriloquate( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_word_of_recall( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_wizard_eye( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_eagle_eye( ubyte level, CHAR_DATA *ch, char *arg, int type,
  CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) {
	 case SPELL_TYPE_SPELL:
		 return spell_eagle_eye(level, ch, tar_ch, 0, skill);
		 break;
		default :
				log("Serious screw-up in eagle eye!", 
ANGEL, LOG_BUG);
	 break;
	 }
  return eFAILURE;
}

int cast_summon( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_charm_person( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_sense_life( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


int cast_identify( ubyte level, CHAR_DATA *ch, char *arg, int type,
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



int cast_frost_breath( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_acid_breath( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_fire_breath( ubyte level, CHAR_DATA *ch, char *arg, int type,
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


int cast_gas_breath( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_lightning_breath( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_fear( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_refresh( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_fly( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_cont_light( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_know_alignment(ubyte level, CHAR_DATA *ch, char *arg, int type,
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

char *dispel_magic_spells[] = 
{
  "", "sanctuary", "protection_from_evil", "haste", "stone_shield", "greater_stone_shield", 
"frost_shield",   "lightning_shield", "fire_shield", "acid_shield", "protection_from_good", "\n"
};

int cast_dispel_magic( ubyte level, CHAR_DATA *ch, char *arg, int type,
			 CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  int spell = 0;
  char buffer[MAX_INPUT_LENGTH];
  one_argument(arg, buffer);
  if (type == SPELL_TYPE_SPELL && arg && *arg)
  {
    int i;
    for (i = 1; *dispel_magic_spells[i] != '\n'; i++)
    {
       if (!str_prefix(buffer, dispel_magic_spells[i]))
         spell = i;
    }
    if (!spell)
    {
       send_to_char("You cannot target that spell.\r\n",ch);
       return eFAILURE;
    }
  }
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_dispel_magic(level, ch, tar_ch, (obj_data*) spell, skill);
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

char *dispel_minor_spells[] =
{
  "", "invisibility", "detect_invisibility", "camouflage", "resist_acid",
  "resist_cold", "resist_fire", "resist_energy", "barkskin",
  "stoneskin", "fly", "true_sight", "water_breath", "armor",
  "shield", "resist_magic", "\n"

};

int cast_dispel_minor( ubyte level, CHAR_DATA *ch, char *arg, int type,
			 CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  int spell = 0;
  char buffer[MAX_INPUT_LENGTH];
  one_argument(arg, buffer);
  if (!tar_obj && type == SPELL_TYPE_SPELL && arg && *arg)
  {
    int i;
    for (i = 1; *dispel_minor_spells[i] != '\n'; i++)
    {
       if (!str_prefix(buffer, dispel_minor_spells[i]))
	 spell = i;
    }
    if (spell)
	   tar_obj = (struct obj_data *) spell;
    else {
      send_to_char("You cannot target that spell.\r\n",ch);
      return eFAILURE;
    }
  }
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

	
int elemental_damage_bonus(int spell, char_data *ch)
{
  char_data *mst = ch->master?ch->master:ch;
  struct follow_type *f, *t;
  bool fire, ice, earth, energy;
  fire = ice = earth = energy = FALSE;
  for (f=mst->followers; f; f=f->next)
  {
    if (IS_NPC(f->follower) && f->follower->height == 77)
    {
	switch (mob_index[f->follower->mobdata->nr].virt)
	{
		case 88:
		  fire = TRUE; break;
		case 89:
		  ice = TRUE; break;
		case 90:
		  energy = TRUE; break;
		case 91:
		  earth = TRUE; break;
		default: break;
	}
    } else {
	  for (t=f->follower->followers; t; t=t->next)
    	    if (IS_NPC(t->follower) && t->follower->height == 77)
    	    {
		switch (mob_index[t->follower->mobdata->nr].virt)
		{
			case 88:
			  fire = TRUE; break;
			case 89:
			  ice = TRUE; break;
			case 90:
			  energy = TRUE; break;
			case 91:
			  earth = TRUE; break;
			default: break;
		}
	    }
     }
  }
  switch (spell)
  {
 // done this way to ensure no unintended spells get the bonus
 // (shields, for instance, prolly other things with TYPE_FIRE etc damage that's unintended)
	case SPELL_FIREBALL:
        case SPELL_FIRESTORM:
        case SPELL_FLAMESTRIKE:
        case SPELL_HELLSTREAM:
	   if (fire) return dice(5,6);
	   else return 0;
        case SPELL_METEOR_SWARM:
        case SPELL_BEE_STING:
	case SPELL_CREEPING_DEATH:
	case SPELL_BLUE_BIRD:
	   if (earth) return dice(8,4);
	   else return 0;
	case SPELL_CALL_LIGHTNING:
	case SPELL_SUN_RAY:
	case SPELL_SHOCKING_GRASP:
	case SPELL_LIGHTNING_BOLT:
	   if (energy) return dice(1,50);
	   else return 0;
	case SPELL_DROWN:
	case SPELL_VAMPIRIC_TOUCH:
	case SPELL_DISPEL_EVIL:
	case SPELL_DISPEL_GOOD:
	case SPELL_CHILL_TOUCH:
	    if (ice) return dice(6,5);
	    else return 0;
        default: return 0;
  }
}

bool elemental_score(char_data *ch, int level)
{
  char_data *mst = ch->master?ch->master:ch;
  struct follow_type *f, *t;
  bool fire, ice, earth, energy;
  fire = ice = earth = energy = FALSE;
  // reuse of elemental damage function
  for (f=mst->followers; f; f=f->next)
  {
    if (IS_NPC(f->follower))
    {
	if (f->follower->height == 77) // improved
	switch (mob_index[f->follower->mobdata->nr].virt)
	{
		case 88:
		  fire = TRUE; break;
		case 89:
		  ice = TRUE; break;
		case 90:
		  energy = TRUE; break;
		case 91:
		  earth = TRUE; break;
		default: break;
	}
    } else {
	  for (t=f->follower->followers; t; t=t->next)
   	   if (t->follower->height == 77) // improved
    	    if (IS_NPC(t->follower))
    	    {
		switch (mob_index[t->follower->mobdata->nr].virt)
		{
			case 88:
			  fire = TRUE; break;
			case 89:
			  ice = TRUE; break;
			case 90:
			  energy = TRUE; break;
			case 91:
			  earth = TRUE; break;
			default: break;
		}
	    }
     }
  }
  char buf[MAX_STRING_LENGTH];
  extern char frills[];
  if (fire) {
    sprintf(buf, "|%c| Affected by %-22s          Modifier %-16s  |%c|\n\r",
               frills[level],"ENHANCED_FIRE_AURA","NONE",frills[level]);
        send_to_char(buf,ch);
        if (++level == 4) level = 0;
  }
  if (ice) {
    sprintf(buf, "|%c| Affected by %-22s          Modifier %-16s  |%c|\n\r",
               frills[level],"ENHANCED_COLD_AURA","NONE",frills[level]);
        send_to_char(buf,ch);
        if (++level == 4) level = 0;
  }
  if (energy) {
    sprintf(buf, "|%c| Affected by %-22s          Modifier %-16s  |%c|\n\r",
               frills[level],"ENHANCED_ENERGY_AURA","NONE",frills[level]);
        send_to_char(buf,ch);
        if (++level == 4) level = 0;
  }
  if (earth) {
    sprintf(buf, "|%c| Affected by %-22s          Modifier %-16s  |%c|\n\r",
               frills[level],"ENHANCED_PHYSICAL_AURA","NONE",frills[level]);
        send_to_char(buf,ch);
        if (++level == 4) level = 0;
  }
  return (fire || earth || energy || ice);
}

int cast_conjure_elemental( ubyte level, CHAR_DATA *ch,
		char *arg, int type,
		CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  return spell_conjure_elemental(level, ch, arg, 0, tar_obj, skill);
}

int cast_cure_serious( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_cause_light( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_cause_critical( ubyte level, CHAR_DATA *ch, char *arg,
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

int cast_cause_serious( ubyte level, CHAR_DATA *ch, char *arg,
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

int cast_flamestrike( ubyte level, CHAR_DATA *ch, char *arg,
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

int cast_resist_cold( ubyte level, CHAR_DATA *ch, char *arg,
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
int cast_staunchblood(ubyte level, CHAR_DATA *ch, char *arg, int type,
                       CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill) {
  switch(type) {
    case SPELL_TYPE_SPELL:
      return spell_staunchblood(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_POTION:
      return spell_staunchblood(level, ch, tar_ch, 0, skill);
      break;
    case SPELL_TYPE_SCROLL:
      if (tar_obj) return eFAILURE;
      if (!tar_ch) tar_ch = ch;
      return spell_staunchblood(level, ch, tar_ch, 0, skill);
      break;
    default:
      log("Serious screw-up in cast_staunchblood!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}

int cast_resist_energy(ubyte level, CHAR_DATA *ch, char *arg, int type,
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

int cast_resist_fire( ubyte level, CHAR_DATA *ch, char *arg,
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


int cast_resist_magic( ubyte level, CHAR_DATA *ch, char *arg,
		  int type, CHAR_DATA *tar_ch,
		  struct obj_data *tar_obj, int skill)
{
  switch (type) {
  case SPELL_TYPE_SPELL:
	 return spell_resist_magic(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
	 return spell_resist_magic(level, ch, tar_ch, 0, skill);
	 break;
  case SPELL_TYPE_SCROLL:
      if (tar_obj) return eFAILURE;
      if (!tar_ch) tar_ch = ch;
    return spell_resist_magic(level, ch, tar_ch, 0, skill);
    break;
  default:
	 log("Serious screw-up in resist_magic!", ANGEL, LOG_BUG);
	 break;
  }
  return eFAILURE;
}


int cast_stone_skin( ubyte level, CHAR_DATA *ch, char *arg,
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

int cast_shield( ubyte level, CHAR_DATA *ch, char *arg,
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

int cast_weaken( ubyte level, CHAR_DATA *ch, char *arg,
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
  case SPELL_TYPE_POTION:
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

int cast_mass_invis( ubyte level, CHAR_DATA *ch, char *arg,
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

int cast_acid_blast( ubyte level, CHAR_DATA *ch, char *arg,
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
  case SPELL_TYPE_POTION:
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


int cast_hellstream( ubyte level, CHAR_DATA *ch, char *arg,
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

int cast_portal( ubyte level, CHAR_DATA *ch, char *arg,
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


int cast_infravision( ubyte level, CHAR_DATA *ch, char *arg,
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


int cast_animate_dead( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

/* BEE STING */

int spell_bee_sting(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   int dam;
   int retval;
   int bees = 1 + (GET_LEVEL(ch) / 15) + (GET_LEVEL(ch)==50);
   affected_type af;
   int i;
   set_cantquit(ch, victim);
   dam = dice (4, 3) + skill/3;

   for (i = 0; i < bees; i++) {

   retval = damage(ch, victim, dam, TYPE_PHYSICAL_MAGIC, SPELL_BEE_STING, 0);
   if(SOMEONE_DIED(retval))
      return retval;
   }
   // Extra added bonus 1% of the time
   if (dice(1, 100) == 3)
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


int cast_bee_sting(ubyte level, CHAR_DATA *ch, char *arg, int type,
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

/* BEE SWARM */

int cast_bee_swarm(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   int dam;
   int retval;
   CHAR_DATA *tmp_victim, *temp;

   dam = 175;

   act("$n calls upon the insect world!\n\r", ch, 0, 0, TO_ROOM, INVIS_NULL);

   for(tmp_victim = character_list; tmp_victim; tmp_victim = temp) {
      temp = tmp_victim->next;
      if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
          (!ARE_GROUPED(ch,tmp_victim)) && can_be_attacked(ch, tmp_victim)) {
         set_cantquit(ch, tmp_victim);
         retval = damage(ch, tmp_victim, dam, TYPE_MAGIC, SPELL_BEE_SWARM, 0);
         if(IS_SET(retval, eCH_DIED))
           return retval;
         }
      else if(world[ch->in_room].zone == world[tmp_victim->in_room].zone)
         send_to_char("You hear the buzzing of hundreds of bees.\n\r",
	              tmp_victim);
      }
  return eSUCCESS;
}

/* CREEPING DEATH */

int cast_creeping_death(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   int dam;
   int retval;
   affected_type af;
   int bingo = 0, poison = 0;

   set_cantquit(ch, victim);
   dam = 300;

        if(world[ch->in_room].sector_type == SECT_SWAMP) dam += 200;
   else if(world[ch->in_room].sector_type == SECT_FOREST) dam += 185;
   else if(world[ch->in_room].sector_type == SECT_FIELD) dam += 170;
   else if(world[ch->in_room].sector_type == SECT_BEACH) dam += 150;
   else if(world[ch->in_room].sector_type == SECT_HILLS) dam += 130;
   else if(world[ch->in_room].sector_type == SECT_DESERT) dam += 110;
   else if(world[ch->in_room].sector_type == SECT_MOUNTAIN) dam += 90;
   else if(world[ch->in_room].sector_type == SECT_PAVED_ROAD) dam += 90;
   else if(world[ch->in_room].sector_type == SECT_WATER_NOSWIM) dam -= 25;
   else if(world[ch->in_room].sector_type == SECT_AIR) dam += 25;
   else if(world[ch->in_room].sector_type == SECT_FROZEN_TUNDRA) dam -= 30;
   else if(world[ch->in_room].sector_type == SECT_UNDERWATER) dam -= 100;
   else if(world[ch->in_room].sector_type == SECT_ARCTIC) dam -= 60;
   
   if (!OUTSIDE(ch)) {
      send_to_char("Your spell is more draning because you are indoors!\n\r", ch);
      // If they are NOT outside it costs extra mana
      GET_MANA(ch) -= level / 2;
      if(GET_MANA(ch) < 0)
         GET_MANA(ch) = 0;
   }


   retval = damage(ch, victim, dam, TYPE_PHYSICAL_MAGIC, SPELL_CREEPING_DEATH, 0);
   if(SOMEONE_DIED(retval))
      return retval;

   if(IS_SET(retval,eEXTRA_VAL2)) victim = ch;
   if(IS_SET(retval,eEXTRA_VALUE)) return retval;

   if (skill > 40 && skill <= 60) {
     poison = 2;
   } else if (skill > 60 && skill <= 80) {
     poison = 3;
   } else if (skill > 80) {
     bingo = 1;
     poison = 4;
   }

   
   if (poison > 0) {
     if (dice(1, 100) <= poison && !IS_SET(victim->immune, ISR_POISON)) {
        af.type = SPELL_POISON;
        af.duration = skill / 27;
        af.modifier = IS_NPC(ch)?-123:(int)ch;
        af.location = APPLY_NONE;
        af.bitvector = AFF_POISON;
        affect_join(victim, &af, FALSE, FALSE);
        send_to_char("The insect $2poison$R has gotten into your blood!\n\r", victim);
        act("$N has been $2poisoned$R by your insect swarm!", ch, 0, victim, TO_CHAR, 0);
     }
   }
	
   if (bingo > 0) {    
     if (number(1, 100) <= bingo && GET_LEVEL(victim) < IMMORTAL) {
        dam = GET_HIT(victim)*5 + 20;
        send_to_char("The insects are crawling in your mouth, out of your eyes, "
                     "through your stomach!\n\r", victim);
        act("$N is completely consumed by insects!", ch, 0, victim, TO_ROOM, NOTVICT);
        act("$N is completely consumed by your insects!", ch, 0, victim, TO_CHAR, 0);
        return damage(ch, victim, dam, TYPE_MAGIC, SPELL_CREEPING_DEATH, 0);
     }
   }
   return eSUCCESS;
}

/* BARKSKIN */

int cast_barkskin(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
  struct affected_type af;

  if(affected_by_spell(victim, SPELL_BARKSKIN)) {
    send_to_char("You cannot make your skin any stronger!\n\r", ch);
//    GET_MANA(ch) += 20;
    return eFAILURE;
  }


  af.type      = SPELL_BARKSKIN;
  af.duration  = 5 + level/7;
  af.modifier  = -20;
  af.location  = APPLY_AC;
  af.bitvector = -1;

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

/* HERB LORE */

int cast_herb_lore(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
  int healamount;
  char buf[MAX_STRING_LENGTH],dammsg[MAX_STRING_LENGTH];

  if(GET_RACE(victim) == RACE_UNDEAD) {
    send_to_char("Healing spells are useless on the undead.\n\r", ch);
    return eFAILURE;
  }
  if (GET_RACE(victim) == RACE_GOLEM) {
        send_to_char("The heavy magics surrounding this being prevent healing.\r\n",ch);
        return eFAILURE;
  }

  if (can_heal(ch,victim, SPELL_HERB_LORE)) {
     healamount = dam_percent(skill,180);
     if(OUTSIDE(ch))    GET_HIT(victim) += healamount;
     else { /* if not outside */
       healamount = dam_percent(skill,80);
       GET_HIT(victim) += healamount;
       send_to_char("Your spell is less effective because you are indoors!\n\r", ch);
     }
     if (GET_HIT(victim) >= hit_limit(victim)) {
        healamount += hit_limit(victim)-GET_HIT(victim)-dice(1,4);
        GET_HIT(victim) = hit_limit(victim)-dice(1,4);
     }

     update_pos( victim );

     sprintf(dammsg," of $B%d$R damage", healamount);

     if (ch!=victim) {
        sprintf(buf,"Your herbs heal $N%s and makes $m...hungry?",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
        act(buf,ch,0,victim,TO_CHAR,0);
        sprintf(buf,"$n's magic herbs heal $N| and make $M look...hungry?");
        send_damage(buf, ch, 0, victim, dammsg,"", TO_ROOM);
        sprintf(dammsg,", healing you of $B%d$R damage", healamount);
        sprintf(buf,"$n magic herbs make you feel much better%s!",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
        act(buf,ch,0,victim, TO_VICT, 0);
     } else {
        sprintf(buf,"Your magic herbs make you feel much better%s!",ch->pcdata?IS_SET(ch->pcdata->toggles, PLR_DAMAGE)?dammsg:"":"");
        act(buf,ch,0,0,TO_CHAR,0);
        sprintf(buf,"$n magic herbs heal $m| and make $m look...hungry?");
        send_damage(buf,ch,0,victim,dammsg,"",TO_ROOM);
     }
  }

  char arg1[MAX_INPUT_LENGTH];
  one_argument(arg, arg1);
  if (arg1[0])
  {
    OBJ_DATA *obj = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if (!obj)
    {
	send_to_char("You don't seem to be carrying any such root.\r\n",ch);
	return eFAILURE;
    }
    int virt = obj_index[obj->item_number].virt;
    int aff = 0,spl = 0;
    switch (virt)
    {
      case INVIS_VNUM: 
		aff = AFF_INVISIBLE; spl = SPELL_INVISIBLE;
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n slowly fades out of existence.", victim, 0, 0, TO_ROOM, 0);
		act("You slowly fade out of existence.", victim, 0, 0, TO_CHAR, 0);
		break;
      case HASTE_VNUM:
		aff = AFF_HASTE; spl = SPELL_HASTE;
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n begins moving faster!", victim, 0, 0, TO_ROOM, 0);
		act("You begin moving faster!", victim, 0, 0, TO_CHAR, 0);
		break;
      case TRUE_VNUM: 
	aff = AFF_TRUE_SIGHT; spl = SPELL_TRUE_SIGHT;
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n's eyes starts to gently glow $Bwhite$R.", victim, 0, 0, TO_ROOM, 0);
		act("Your eyes start to glow $Bwhite$R.", victim, 0, 0, TO_CHAR, 0);
		break;
      case INFRA_VNUM: 
		aff = AFF_INFRARED; spl = SPELL_INFRAVISION;
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n's eyes starts to glow $B$4red$R.", victim, 0, 0, TO_ROOM, 0);
		act("Your eyes start to glow $B$4red$R.", victim, 0, 0, TO_CHAR, 0);
		break;
      case FARSIGHT_VNUM: 
		aff = AFF_FARSIGHT; spl = SPELL_FARSIGHT;
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n's eyes blur and seem to $B$0darken$R.", victim, 0, 0, TO_ROOM, 0);
		act("Your eyes blur and the world around you seems to come closer.", victim, 0, 0, TO_CHAR, 0);
		break;
      case LIGHTNING_SHIELD_VNUM: 
		aff = AFF_LIGHTNINGSHIELD; spl = SPELL_LIGHTNING_SHIELD;
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n is surrounded by $B$5electricity$R.", victim, 0, 0, TO_ROOM, 0);
		act("You become surrounded by $B$5electricity$R.", victim, 0, 0, TO_CHAR, 0);
		break;
      case INSOMNIA_VNUM: 
		aff = AFF_INSOMNIA; spl = SPELL_INSOMNIA;
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n blinks and looks a little twitchy.", victim, 0, 0, TO_ROOM, 0);
		act("You suddenly feel very energetic and not at all sleepy.", victim, 0, 0, TO_CHAR, 0);
		break;
      case DETECT_GOOD_VNUM: 
		aff = AFF_DETECT_GOOD; spl = SPELL_DETECT_GOOD; 
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n's eyes starts to gently glow $B$1blue$R.", victim, 0, 0, TO_ROOM, 0);
		act("Your eyes start to glow $B$1blue$R.", victim, 0, 0, TO_CHAR, 0);
		break;
      case DETECT_EVIL_VNUM: 
		aff = AFF_DETECT_EVIL; spl = SPELL_DETECT_EVIL; 
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n's eyes starts to gently glow $4deep red$R.", victim, 0, 0, TO_ROOM, 0);
		act("Your eyes start to glow $4deep red$R.", victim, 0, 0, TO_CHAR, 0);
		break;
      case DETECT_INVISIBLE_VNUM: 
		aff = AFF_DETECT_INVISIBLE; spl = SPELL_DETECT_INVISIBLE;
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n's eyes starts to gently glow $B$5yellow$R.", victim, 0, 0, TO_ROOM, 0);
		act("Your eyes start to glow $B$5yellow$R.", victim, 0, 0, TO_CHAR, 0);
		break;
      case SENSE_LIFE_VNUM: 
		aff = AFF_SENSE_LIFE; spl = SPELL_SENSE_LIFE;
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
//		act("$n's eyes starts to gently glow a $2deep green$R.", victim, 0, 0, TO_ROOM, 0);
		act("Your become intensely aware of your surroundings.", victim, 0, 0, TO_CHAR, 0);
		break;
      case SOLIDITY_VNUM: 
		aff = AFF_SOLIDITY; spl = SPELL_SOLIDITY; 
    if (affected_by_spell(victim, spl)) { send_to_char("They are already affected by that spell.\r\n",ch); return eFAILURE; }
		act("$n is surrounded by a pulsing, $6violet$R aura.", victim, 0, 0, TO_ROOM, 0);
		act("You become surrounded by a pulsing, $6violet$R aura.", victim, 0, 0, TO_CHAR, 0);
		break;
	case 2256:
		aff = 0; spl =0;
		send_to_char("Adding the herbs improve the healing effect of the spell.\r\n",ch);
		GET_HIT(victim) += 40;
		if (GET_HIT(victim) > GET_MAX_HIT(victim)) GET_HIT(victim) = GET_MAX_HIT(victim);
		break;
	default:
		send_to_char("That's not a herb!\r\n",ch);
		return eFAILURE;
    }
    extract_obj(obj);
    struct affected_type af;
    af.type      = spl;
    af.duration  =  3;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = aff;
    if (aff)
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* CALL FOLLOWER */

int cast_call_follower(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   if(IS_SET(world[ch->in_room].room_flags, CLAN_ROOM)) {
      send_to_char("I don't think your fellow clan members would appreciate the wildlife.\n\r", ch);
      GET_MANA(ch) += 75;
      REM_WAIT_STATE(ch, skill / 10);
     return eFAILURE;
   }

   victim = NULL;

   for(struct follow_type *k = ch->followers; k; k = k->next)
     if(IS_MOB(k->follower) && affected_by_spell(k->follower, SPELL_CHARM_PERSON) &&
		k->follower->in_room != ch->in_room)
     {
        victim = k->follower;
        break;
     }


   
   if (NULL == victim) {
      send_to_char("You don't have any tamed friends in need of a summon!\n\r", ch);
      REM_WAIT_STATE(ch, skill / 10);
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

   REM_WAIT_STATE(ch, skill / 10);
   return eSUCCESS;
}

/* ENTANGLE */

int cast_entangle(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{

	if(!OUTSIDE(ch))
	{
		send_to_char("You must be outside to cast this spell!\n\r", ch);
	//	GET_MANA(ch) += 10; /*Mana kludge */
		return eFAILURE;
	}
	set_cantquit(ch, victim);
	act("Your plants grab $N, attempting to force $M down.", ch, 0,
	  victim, TO_CHAR, 0);
	act("$n's plants make an attempt to drag you to the ground!",
	  ch, 0, victim, TO_VICT, 0);
	act("$n raises the plants which attack $N!", ch, 0, victim,
		TO_ROOM, NOTVICT);

        int learned = has_skill(ch, SPELL_ENTANGLE);
	if (learned)
        {
        learned = 800 / learned - 1; // get the number for percentage
        if(!number(0, learned))
           spell_blindness(level, ch, victim, 0, 0);       /* The plants blind the victim . . */
	}
	if (GET_POS(victim) > POSITION_SITTING)
	{GET_POS(victim) = POSITION_SITTING;		/* And pull the victim down to the ground */
	if (victim->fighting)
		SET_BIT(victim->combat, COMBAT_BASH2);
	}
	update_pos(victim);
	return eSUCCESS;
}

/* EYES OF THE OWL */

int cast_eyes_of_the_owl(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
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
	af.modifier  = 1+(skill/20);
	af.location  = APPLY_WIS;
	af.bitvector = AFF_INFRARED;
	affect_join(victim, &af, FALSE, FALSE);
        redo_mana(victim);
  send_to_char("You feel your vision become much more acute.\n\r", victim);
  return eSUCCESS;
}

/* FELINE AGILITY */

int cast_feline_agility(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
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

	af.type 	= SPELL_FELINE_AGILITY;
	af.duration	= 1+(level/4);
	af.modifier	= -10 -skill/5; 		/* AC bonus */
	af.location 	= APPLY_AC;
	af.bitvector	= -1;
	affect_to_char(victim, &af);
	send_to_char("Your step lightens as you gain the agility of a cat!\r\n", ch);
	af.modifier	= 1+(skill/20);		/* + dex */
	af.location	= APPLY_DEX;
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

/* OAKEN FORTITUDE */

int cast_oaken_fortitude(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA  *victim, struct obj_data * tar_obj, int skill)
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


        af.type         = SPELL_OAKEN_FORTITUDE;
        af.duration     = 1+(level/4);
        af.modifier     = -10 - skill/5;             /* AC apply */
        af.location     = APPLY_AC;
        af.bitvector    = -1;
        affect_to_char(victim, &af);
	send_to_char("Your fortitude increases to that of an oak.\r\n",victim);
        af.modifier     = 1 + (skill/20);
        af.location     = APPLY_CON;
        affect_to_char(victim, &af);
	redo_hitpoints(victim);
	return eSUCCESS;
}

/* CLARITY */

int cast_clarity(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA  *victim, struct obj_data * tar_obj, int skill)
{ // Feline agility rip
        struct affected_type af;

        if(affected_by_spell(victim, SPELL_CLARITY))
        {
	 	send_to_char("You cannot clear your mind any further without going stupid.\r\n",ch);
		return eFAILURE;
        }


        af.type         = SPELL_CLARITY;
        af.duration     = 3 + skill/6;
        af.modifier     = 3 + skill/20;
        af.location     = APPLY_INT;
        af.bitvector    = -1;
        affect_to_char(victim, &af);
        redo_mana(victim);
	send_to_char("You suddenly feel smarter than everyone else.\r\n",victim);
        act("$n's eyes shine with powerful intellect!", victim, 0, 0, TO_ROOM, 0);
        af.modifier     = 3 + (skill/20);
        af.location     = APPLY_MANA_REGEN;
        affect_to_char(victim, &af);
	return eSUCCESS;
}

/* FOREST MELD */

int cast_forest_meld(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
	if(!(world[ch->in_room].sector_type == SECT_FOREST))
	{
		send_to_char("You are not in a forest!!\n\r", ch);
		return eFAILURE;
	}
//	if(victim != ch)
//	{
//		send_to_char("Why would the forest like anyone but 
//you?\n\r", ch);
//		return eFAILURE;
//	}
       if (affected_by_spell(ch, FUCK_PTHIEF) || affected_by_spell(ch, FUCK_GTHIEF))
	{
		send_to_char("The forests reject your naughty, thieving self.\r\n",ch);
		return eFAILURE;
	}
	act("$n melts into the forest and is gone.", ch, 0, 0,
	  TO_ROOM, INVIS_NULL);
	send_to_char("You feel yourself slowly become a temporary part of the living forest.\n\r", ch);
	 struct affected_type af;
     int skil = has_skill(ch, SPELL_FOREST_MELD);
   af.type          = SPELL_FOREST_MELD;
     af.duration      = 2 + (skil> 40) + (skil > 60) + (skil > 80);
     af.modifier      = 0;
     af.location      = APPLY_NONE;
     af.bitvector     = AFF_FOREST_MELD;
     affect_to_char(ch, &af);

        
// 	SETBIT(ch->affected_by, AFF_FOREST_MELD);
        SETBIT(ch->affected_by, AFF_HIDE);
	return eSUCCESS;
}

/* COMPANION (disabled) */

int cast_companion(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
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
            SETBIT(mob->affected_by, AFF_FIRESHIELD);
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
   af.bitvector = -1;
   affect_join(ch, &af, FALSE, FALSE);
   return eSUCCESS;
}


// Procedure for checking for/destroying spell components                       
// Expects SPELL_xxx, the caster, and boolean as to destroy the components      
// Returns TRUE for success and FALSE for failure                               
                                                                                
int check_components(CHAR_DATA *ch, int destroy, int item_one = 0, 
                     int item_two = 0, int item_three = 0, int item_four = 0, bool silent = FALSE)
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
    ptr_three = get_obj_in_list_num(real_object(item_three), ch->carrying);                  
  if(item_four)                                                                 
    ptr_four = get_obj_in_list_num(real_object(item_four),ch->carrying);                    
                                                                                
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
    if(gone && !silent)
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

  if(GET_LEVEL(ch) > ARCHANGEL && !all_ok && !silent) {
     send_to_char("You didn't have the right components, but yer a god:)\r\n", ch);
     return TRUE;
  }

return all_ok;                                                                  
} 

/* CREATE GOLEM */

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
  REMBIT(mob->mobdata->actflags, ACT_SCAVENGER);
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
  SETBIT(mob->affected_by, AFF_GOLEM);
  // kill um all!
  k = ch->followers;
  while(k) 
     if(IS_AFFECTED(k->follower, AFF_CHARM))                                    
     {
        fight_kill(ch, k->follower, TYPE_RAW_KILL, 0);
        k = ch->followers;
     }
     else k = k->next;

  add_follower(mob, ch, 0);

  // add random abilities
  if(number(1, 3) > 1)
  {
    SETBIT(mob->mobdata->actflags, ACT_2ND_ATTACK);
    if(number(1, 2) == 1)
      SETBIT(mob->mobdata->actflags, ACT_3RD_ATTACK);
  }      

  if(number(1, 15) == 15)
    SETBIT(mob->affected_by, AFF_SANCTUARY);

  if(number(1,20) == 20)
    SETBIT(mob->affected_by, AFF_FIRESHIELD);

  if(number(1,2) == 2)
    SETBIT(mob->affected_by, AFF_DETECT_INVISIBLE);

  if(number(1,7) == 7)
    SETBIT(mob->affected_by, AFF_FLYING);

  if(number(1,10) == 10)
    SETBIT(mob->affected_by, AFF_SNEAK);

  if(number(1,2) == 1)
    SETBIT(mob->affected_by, AFF_INFRARED);

  if(number(1, 50) == 1)
    SETBIT(mob->affected_by, AFF_EAS);

  if(number(1, 3) == 1)
    SETBIT(mob->affected_by, AFF_TRUE_SIGHT);

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

/* OLD CREATE/RELEASE GOLEM (unused) */

/*
int cast_create_golem( ubyte level, CHAR_DATA *ch, char *arg, int type,
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
}*/

/*
int spell_release_golem(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
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
      if(ISSET(temp->follower->affected_by, AFF_GOLEM))
         done = 1;
      else temp = temp->next;
   }
   
   if(!temp) {
      send_to_char("You have no golem!\r\n", ch);
      return eFAILURE;
   }

   fight_kill(ch, temp->follower, TYPE_RAW_KILL, 0);   
   return eSUCCESS;
}
*/

/* BEACON */

int spell_beacon(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
//   extern int top_of_world;
//   int to_room = 0;

   if(!ch->beacon) {
      send_to_char("You have no beacon set!\r\n", ch);
      return eFAILURE;
   }

   if(ch->beacon->in_room < 1) {
      send_to_char("Your magical beacon has been lost!\r\n", ch);
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
         send_to_char("Your beacon cannot take you into or out of the arena!\r\n", ch);
         return eFAILURE;
   }

   if(IS_AFFECTED(ch, AFF_CHAMPION)) {
     if (ch->beacon->in_room >= 1900 && ch->beacon->in_room <= 1999) {
       send_to_char("You cannot beacon into a guild whilst Champion.\n\r", ch);
       return eFAILURE;
     }

     if (IS_SET(world[ch->beacon->in_room].room_flags, CLAN_ROOM)) {
       send_to_char("You cannot beacon into a clan hall whilst Champion.\r\n",ch);
       return eFAILURE;
     }
   }


   if (others_clan_room(ch, &world[ch->beacon->in_room]) == true) {
     send_to_char("You cannot beacon into another clan's hall.\n\r", ch);
     ch->beacon->equipped_by = NULL;
     extract_obj(ch->beacon);
     ch->beacon = NULL;
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
   if (IS_NPC(ch)) return eFAILURE;
   if(GET_CLASS(ch) != CLASS_ANTI_PAL && GET_LEVEL(ch) < ARCHANGEL) {
      send_to_char("Sorry, but you cannot do that here!\r\n", ch);
      return eFAILURE;
   }

   if(IS_AFFECTED(ch, AFF_CHAMPION) && ch->in_room >= 1900 && ch->in_room <= 1999)
   {
      send_to_char("You cannot set a beacon in a guild whilst Champion.\n\r", ch);
      return eFAILURE;
   }

   if (IS_AFFECTED(ch, AFF_CHAMPION) && IS_SET(world[ch->in_room].room_flags, CLAN_ROOM))
   {
	send_to_char("You cannot set a beacon in a clan hall whilst Champion.\r\n",ch);
	return eFAILURE;
   }

   if (others_clan_room(ch, &world[ch->in_room]) == true) {
     send_to_char("You cannot set a beacon in another clan's hall.\n\r", ch);
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

/* REFLECT (non-castable atm) */

int spell_reflect(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
{
   send_to_char("Tell an immortal what you just did.\r\n", ch);
   return eFAILURE;
}

/* CALL FAMILIAR (SUMMON FAMILIAR) */

#define FAMILIAR_MOB_IMP        5
#define FAMILIAR_MOB_CHIPMUNK   6
#define FAMILIAR_MOB_GREMLIN    4
#define FAMILIAR_MOB_OWL        7

int choose_druid_familiar(char_data * ch, char * arg)
{
   char buf[MAX_INPUT_LENGTH];
   
   one_argument(arg, buf);

   if(!strcmp(buf, "chipmunk"))
   {
     if(!check_components(ch, TRUE, 27800)) {
       send_to_char("You remember at the last second that you don't have an acorn ready!\r\n", ch);
       act("$n's hands pop with unused mystical energy and $e seems confused.", ch, 0, 0, TO_ROOM, 0);
       return -1;
     }
     return FAMILIAR_MOB_CHIPMUNK;
   }
  if (!strcmp(buf, "owl"))
  {
     if(!check_components(ch, TRUE, 44)) {
       send_to_char("You remember at the last second that you don't have a dead mouse ready!\r\n", ch);
       act("$n's hands pop with unused mystical energy and $e seems confused.", ch, 0, 0, TO_ROOM, 0);
       return -1;
     }
     return FAMILIAR_MOB_OWL;
  }
   send_to_char("You must specify the type of familiar you wish to summon.\r\n"
                "You currently may summon:  chipmunk or owl\r\n", ch);
   return -1;
}

int choose_mage_familiar(char_data * ch, char * arg)
{
   char buf[MAX_INPUT_LENGTH];
   
   one_argument(arg, buf);

   if(!strcmp(buf, "imp")) 
   {
     if(!check_components(ch, TRUE, 4)) {
       send_to_char("You remember at the last second that you don't have a batwing ready!\r\n", ch);
       act("$n's hands pop with unused mystical energy and $e seems confused.", ch, 0, 0, TO_ROOM, 0);
       return -1;
     }
     return FAMILIAR_MOB_IMP;
   }
   if(!strcmp(buf, "gremlin")) 
   {
     if(!check_components(ch, TRUE, 43)) {
       send_to_char("You remember at the last second that you don't have a silver piece ready!\r\n", ch);
       act("$n's hands pop with unused mystical energy and $e seems confused.", ch, 0, 0, TO_ROOM, 0);
       return -1;
     }
     return FAMILIAR_MOB_GREMLIN;
   }
   send_to_char("You must specify the type of familiar you wish to summon.\r\n"
                "You currently may summon:  imp or gremlin\r\n", ch);
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
        "A small imp appears from the smoke and perches on $n's shoulder.",
        ch, 0, 0, TO_ROOM, 0);
      send_to_char("You channel a miniature $B$4fireball$R into the wing and throw it into the air.\r\n"
               "A small imp appears from the $B$4flames$R and perches upon your shoulder.\r\n", ch);
    break;
    case FAMILIAR_MOB_CHIPMUNK:
      act("$n coaxs a chipmunk from nowhere and gives it an acorn to eat.\r\n"
          "A small chipmunk eats the acorn and looks at $n lovingly.",
          ch, 0, 0, TO_ROOM, 0);
      send_to_char("You whistle a little tune summoning a chipmunk to you and give it an acorn.\r\n"
               "The small chipmunk eats the acorn and looks at you adoringly.\r\n", ch);
    break;
    case FAMILIAR_MOB_OWL:
    act("$n throws a dead mouse skyward and a large owl swoops down and grabs it in midair.\r\n",ch, 0, 0, TO_ROOM, 0);
    send_to_char("You toss the mouse into the air and grin as an owl swoops down from above.\r\nA voice echoes in your mind...'I can help you to $4watch$7 for nearby enemies.'\r\n",ch);
   break;
   case FAMILIAR_MOB_GREMLIN:
    act("$n opens a small portal and throws in a piece of silver.\r\nA confused gremlin is launched out of the portal, hitting its head as it lands on the floor.",ch, 0, 0, TO_ROOM, 0);
   send_to_char("You open a portal to your target and throw in your silver piece.\r\nThe gremlin is expunged from the dimension beyond through your portal, hitting its head in the process.\r\n",ch);
 break;
    default:
      send_to_char("Illegal message in familar_creation_message.  Tell pir.\r\n", ch);
    break;
  }
}

int spell_summon_familiar(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *victim, struct obj_data * tar_obj, int skill)
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
     if(IS_AFFECTED(k->follower, AFF_FAMILIAR))
     {
       send_to_char("But you already have a devoted pet!\r\n", ch);
       return eFAILURE;
     }
     else k = k->next;
  
  mob = clone_mobile(r_num);
  char_to_room(mob, ch->in_room);

  SETBIT(mob->affected_by, AFF_FAMILIAR);

  af.type = SPELL_SUMMON_FAMILIAR;
  af.duration = -1;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;
  affect_to_char(mob, &af);

  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  familiar_creation_message(ch, fam_type);
  add_follower(mob, ch, 0);

  return eSUCCESS;
}

int cast_summon_familiar( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

/* LIGHTED PATH */

int spell_lighted_path( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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
    sprintf(buf, "A %s called %s headed %s...\r\n",
            race_info[ptrack->race].singular_name,
            ptrack->trackee,
            dirs[ptrack->direction]);
    send_to_char(buf, ch);
    ptrack = ptrack->next;
  }

  return eSUCCESS;
}

int cast_lighted_path( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

/* RESIST ACID */

int spell_resist_acid(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;
    if (GET_CLASS(ch) == CLASS_ANTI_PAL && ch != victim)
    {
	send_to_char("You can only cast this on yourself.\r\n",ch);
	return eFAILURE;
    }

    if(!affected_by_spell(victim, SPELL_RESIST_ACID)) {
       act("$n's skin turns $2green$R momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
       act("Your skin turns $2green$R momentarily.", victim, 0, 0, TO_CHAR, 0);

       af.type = SPELL_RESIST_ACID;
       af.duration = 1 + skill / 10;
       af.modifier = 10 + skill / 6;
       af.location = APPLY_SAVING_ACID;
       af.bitvector = -1;
       affect_to_char(victim, &af);
    }
  return eSUCCESS;
}

int cast_resist_acid(ubyte level, CHAR_DATA *ch, char *arg, int type,
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

/* SUN RAY */

int spell_sun_ray(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  extern struct weather_data weather_info;

  set_cantquit( ch, victim );

  dam = MIN((int)GET_MANA(ch), 725);

  if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY)) {

//	 if(saves_spell(ch, victim, 0, SAVE_TYPE_ENERGY) >= 0)
//		dam >>= 1;

	 return damage(ch, victim, dam, TYPE_ENERGY, SPELL_SUN_RAY, 0);
  }
  return eFAILURE;
}

int cast_sun_ray( ubyte level, CHAR_DATA *ch, char *arg, int type,
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

/* RAPID MEND */

int spell_rapid_mend(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;
    int learned = has_skill(ch, SPELL_RAPID_MEND);
    int regen = 0, duration = 0;

    if(!affected_by_spell(victim, SPELL_RAPID_MEND)) {
       act("$n starts to heal more quickly.", victim, 0, 0, TO_ROOM, INVIS_NULL);
       send_to_char("You feel your body begin to heal more quickly.\n\r", victim);

       if (learned < 40) {
         duration = 3;
         regen = 4;
       } else if (learned > 40 && learned <= 60) {
         duration = 3;
         regen = 6;
       } else if (learned > 60 && learned <= 80) {
         duration = 4;
         regen = 8;
       } else if (learned > 80 && learned <= 90) {
         duration = 5;
         regen = 10;
       } else if (learned > 90) {
         duration = 6;
         regen = 12;
       }

       af.type = SPELL_RAPID_MEND;
       af.duration = duration;
       af.modifier = regen;
       af.location = APPLY_HP_REGEN;
       af.bitvector = -1;
       affect_to_char(victim, &af);
    }
    else act("$n is already mending quickly.", victim, 0, 0, TO_CHAR, 0);

  return eSUCCESS;
}

int cast_rapid_mend(ubyte level, CHAR_DATA *ch, char *arg, int type,
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

/* IRON ROOTS */

int spell_iron_roots(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
    struct affected_type af;

    if(affected_by_spell(ch, SPELL_IRON_ROOTS)) {
       affect_from_char(ch, SPELL_IRON_ROOTS);
       act("The tree roots encasing $n's legs sink away under the surface of the ground.", ch, 0, 0, TO_ROOM, INVIS_NULL);
       act("The roots release their hold upon you and melt away beneath the surface of the ground.", ch, 0, 0, TO_CHAR, 0);
       REMBIT(ch->affected_by, AFF_NO_FLEE);
    }
    else {
       act("Tree roots spring from the ground bracing $n's legs, holding $m down firmly to the ground.", ch, 0, 0, TO_ROOM, INVIS_NULL);
       act("Tree roots spring from the ground firmly holding you to the ground.", ch, 0, 0, TO_CHAR, 0);

       SETBIT(ch->affected_by, AFF_NO_FLEE);

       af.type = SPELL_IRON_ROOTS;
       af.duration = level;
       af.modifier = 0;
       af.location = APPLY_NONE;
       af.bitvector = -1;
       affect_to_char(ch, &af);
    }
  return eSUCCESS;
}

/* IRON ROOTS (potion, scroll, staff, wand) */

int cast_iron_roots(ubyte level, CHAR_DATA *ch, char *arg, int type,
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


/* ACID SHIELD */

int spell_acid_shield(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int learned = has_skill(ch, SPELL_ACID_SHIELD);

  if (!affected_by_spell(victim, SPELL_ACID_SHIELD))
  {
    act("$n is surrounded by a gaseous shield of $B$2acid$R.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    act("You are surrounded by a gaseous shield of $B$2acid$R.", victim, 0, 0, TO_CHAR, 0);

    af.type      = SPELL_ACID_SHIELD;
    af.duration  = 2 + (skill / 23);
    af.modifier  = learned;
    af.location  = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* ACID SHIELD (potion, scroll, wand, staves) */

int cast_acid_shield( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       if(GET_CLASS(ch) == CLASS_ANTI_PAL) {
          if(GET_ALIGNMENT(ch) > -351) {
             send_to_char("You are not evil enough to cast this spell!\n\r", ch);
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

/* WATER BREATHING */

int spell_water_breathing(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  struct affected_type *cur_af;
  
  if ((cur_af = affected_by_spell(victim, SPELL_WATER_BREATHING)))
    affect_remove(victim, cur_af, SUPPRESS_ALL);

  act("Small gills spring forth from $n's neck and begin fanning as $e breathes.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("Your neck springs gills and the air around you suddenly seems very dry.", victim, 0, 0, TO_CHAR, 0);

  af.type      = SPELL_WATER_BREATHING;
  af.duration  = 6 + (skill/5);
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* WATERBREATHING (potions, scrolls, staves, wands) */

int cast_water_breathing( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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

/* GLOBE OF DARKNESS */

int spell_globe_of_darkness(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  obj_data * globe;
  int learned = has_skill(ch, SPELL_GLOBE_OF_DARKNESS);
  int dur = 0, mod = 0;

  if (learned <= 20) {
    dur = 2;
    mod = 10;
  } else if (learned > 20 && learned <= 40) {
    dur = 3;
    mod = 15;
  } else if (learned > 40 && learned <= 60) {
    dur = 3;
    mod = 15;
  } else if (learned > 60 && learned <= 80) {
    dur = 4;
    mod = 20;
  } else if (learned > 80 && learned <= 90) {
    dur = 4;
    mod = 20;
  } else if (learned > 90) {
    dur = 5;
    mod = 25;
  }
  if (skill == 150) {dur = 2;mod = 15;}
  globe = clone_object(real_object(GLOBE_OF_DARKNESS_OBJECT));

  if(!globe) {
    send_to_char("Major screwup in globe of darkness.  Missing util obj.  Tell coder.\n\r", ch);
    return eFAILURE;
  }

  globe->obj_flags.value[0] = dur;
  globe->obj_flags.value[1] = mod;

  act("$n's hand blurs, causing $B$0darkness$R to quickly expand and expunge light from the entire area.",
      ch, 0, 0, TO_ROOM, 0);
  send_to_char("Your summoned $B$0darkness$R turns the area pitch black.\n\r", ch);

  obj_to_room(globe, ch->in_room);
  world[ch->in_room].light -= globe->obj_flags.value[1];

  return eSUCCESS;
}

/* GLOBE OF DARKNESS (potions, scrolls, staves, wands) */

int cast_globe_of_darkness( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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

/* EYES OF THE EAGLE (disabled) */

int spell_eyes_of_the_eagle(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  send_to_char("This spell doesn't do anything right now.\r\n", ch);
  return eSUCCESS;
}

int cast_eyes_of_the_eagle( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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

/* ICE SHARDS */

int spell_ice_shards(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
 // int save;

  set_cantquit( ch, victim );

  dam = dice(2,level/2);

//  save =saves_spell(ch, victim, (level/2), SAVE_TYPE_COLD);

  // modify the damage by how much they resisted
  //dam += (int) (dam * (save/100));
  
  return damage(ch, victim, dam, TYPE_COLD, SPELL_ICE_SHARDS, 0);
}

/* ICE SHARDS (potions, scrolls, wands, staves) */

int cast_ice_shards( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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

/* LIGHTNING SHIELD */

int spell_lightning_shield(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int learned = has_skill(ch, SPELL_LIGHTNING_SHIELD);

  if (!affected_by_spell(victim, SPELL_LIGHTNING_SHIELD))
  {
    act("$n is surrounded by a crackling shield of $B$5electrical$R energy.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    act("You are surrounded by a crackling shield of $B$5electrical$R energy.", victim, 0, 0, TO_CHAR, 0);

    af.type      = SPELL_LIGHTNING_SHIELD;
    af.duration  = 2 + skill / 23;
    af.modifier  = learned;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_LIGHTNINGSHIELD;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* LIGHTNING SHIELD (potion, scroll, wand, staves) */

int cast_lightning_shield( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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

/* BLUE BIRD */

int spell_blue_bird(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  int dam;
  int count;
  int retval = eSUCCESS;

  set_cantquit( ch, victim );
  switch(world[ch->in_room].sector_type) {

     case SECT_SWAMP:     count = 2;   break;
     case SECT_FOREST:    count = 4;   break;

     case SECT_HILLS:     count = 3;   break;
     case SECT_MOUNTAIN:  count = 2;   break;
     case SECT_WATER_SWIM:
     case SECT_WATER_NOSWIM:
     case SECT_BEACH:     count = 3;   break;
     
     case SECT_UNDERWATER:
        send_to_char("But you're underwater!  Your poor birdie would drown:(\r\n", ch);
        return eFAILURE;

     default:             count = 1;   break;

  }
  
  while(!SOMEONE_DIED(retval) && count--) {
     dam = number(10, GET_LEVEL(ch) + 5);
     retval = damage(ch, victim, dam, TYPE_PHYSICAL_MAGIC, SPELL_BLUE_BIRD, 0);
  }

  return retval;
}

/* BLUE BIRD (scrolls, wands, staves) */

int cast_blue_bird( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
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

/* DEBILITY */

int spell_debility(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int retval = eSUCCESS, duration = 0;
  double percent = 0;
  int learned = has_skill(ch, SPELL_DEBILITY);
  extern int hit_gain(CHAR_DATA *ch);
  extern int mana_gain(CHAR_DATA*ch);
  extern int ki_gain(CHAR_DATA *ch);
  extern int move_gain(CHAR_DATA *ch);

  if(affected_by_spell(victim, SPELL_DEBILITY)) {
     send_to_char("Your victim has already been debilitized.\r\n", ch);
     return eSUCCESS;
  }

  if (learned < 40) { 
    percent = 30; 
    duration = 2;
  } else if (learned > 40 && learned <= 60) { 
    percent = 45; 
    duration = 3;
  } else if (learned > 60 && learned <= 80) { 
    percent =  60;
    duration =  4;
  } else if (learned > 80 && learned <= 90) { 
    percent =  75; 
    duration =  5;
  } else if (learned > 90) {
    percent =  90;
    duration =  6;
  }

  set_cantquit( ch, victim );

   if (malediction_res(ch, victim, SPELL_DEBILITY)) {
     act("$N resists your attempt to $6debilitize$R $M!", ch, NULL, victim, TO_CHAR,0);
     act("$N resists $n's attempt to $6debilitize$R $M!", ch, NULL, victim, TO_ROOM,NOTVICT);
     act("You resist $n's attempt to $6debilitize$R you!",ch,NULL,victim,TO_VICT,0);
   } else {
    af.type      = SPELL_DEBILITY;
    af.duration  = duration;
    af.modifier  = 0 - (int)((double)hit_gain(victim) * (percent / 100));
    af.location  = APPLY_HP_REGEN;
    af.bitvector = -1;
    affect_to_char(victim, &af);
    af.location = APPLY_MOVE_REGEN;
    af.modifier = 0 - (int)((double)move_gain(victim) * (percent / 100));
    affect_to_char(victim, &af);
    af.location = APPLY_KI_REGEN;
    af.modifier = 0 - (int)((double)ki_gain(victim) * (percent / 100));
    affect_to_char(victim, &af);
    af.location = APPLY_MANA_REGEN;
    af.modifier = 0 - (int)((double)mana_gain(victim) * (percent / 100));
    affect_to_char(victim, &af);
    send_to_char("Your body becomes $6debilitized$R, reducing your regenerative abilities!\r\n", victim);
    act("$N takes on an unhealthy pallor as $n's magic takes hold.", ch, 0, victim, TO_ROOM, NOTVICT);
    act("Your magic $6debilitizes$R $N!",ch,0,victim, TO_CHAR, 0);
  }
  if(IS_NPC(victim)) 
  {

     if(!victim->fighting)
     {
           mob_suprised_sayings(victim, ch);
           retval = attack(victim, ch, 0);
     }
  }

  return retval;
}

/* DEBILITY (scrolls, wands, staves) */

int cast_debility( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
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

/* ATTRITION */

int spell_attrition(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
  int retval = eSUCCESS;
  int acmod = 0, tohit = 0, duration = 0;
  int learned = has_skill(ch, SPELL_ATTRITION);

  if(affected_by_spell(victim, SPELL_ATTRITION)) {
     send_to_char("Your victim is already suffering from the affects of that spell.\r\n", ch);
     return eSUCCESS;
  }

  if (learned < 40) { 
    acmod = 30; 
    tohit = -3; 
    duration = 2;
  } else if (learned > 40 && learned <= 60 ) { 
    acmod = 45; 
    tohit = -6;
    duration = 3;
  } else if (learned > 60 && learned <= 80 ) { 
    acmod = 60; 
    tohit = -9;
    duration = 4;
  } else if (learned > 80 && learned <= 90 ) { 
    acmod = 75; 
    tohit = -12; 
    duration = 5;
  } else if (learned > 90 ) {
    acmod = 90;
    tohit = -15;
    duration = 6;
  }

  set_cantquit( ch, victim );

   if (malediction_res(ch, victim, SPELL_ATTRITION)) {
     act("$N resists your attempt to cast attrition on $M!", ch, NULL, victim, TO_CHAR, 0);
     act("$N resists $n's attempt to cast attrition on $M!", ch, NULL, victim, TO_ROOM, NOTVICT);
     act("You resist $n's attempt to cast attrition on you!",ch,NULL,victim,TO_VICT,0);
   } else {

    af.type      = SPELL_ATTRITION;
    af.duration  = duration;
    af.modifier  = acmod;
    af.location  = APPLY_AC;
    af.bitvector = -1;
    affect_to_char(victim, &af);
    af.modifier = tohit;
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);

    send_to_char("You feel less energetic as painful magic penetrates your body!\r\n", victim);
    act("$N's body takes on an unhealthy colouring as $n's magic enters $M.", ch, 0, victim, TO_ROOM, NOTVICT);
    act("$N pales as your devitalizing magic courses through $S veins!", ch,0,victim, TO_CHAR, 0);
  }

  if(IS_NPC(victim)) 
  {
     if(!victim->fighting)
     {
           mob_suprised_sayings(victim, ch);
           retval = attack(victim, ch, 0);
     }
  }

  return retval;
}

/* ATTRITION (scrolls, wands, staves) */

int cast_attrition( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
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

/* VAMPIRIC AURA */

int spell_vampiric_aura(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;
/*
  if (affected_by_spell(victim, SPELL_ACID_SHIELD)) {
     act("A film of $B$0shadow$R begins to rise around $n but fades around $s ankles.", victim, 0, 0, TO_ROOM, INVIS_NULL);
     send_to_char("A film of $B$0shadow$R tries to rise around you but dissolves in your acid shield.\n\r", ch);
     return eFAILURE;
  }
*/
  if (!affected_by_spell(victim, SPELL_VAMPIRIC_AURA))
  {
    act("A film of $B$0shadow$R encompasses $n then fades from view.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    act("A film of $B$0shadow$R encompasses you then fades from view.", victim, 0, 0, TO_CHAR, 0);

    af.type      = SPELL_VAMPIRIC_AURA;
    af.duration  = skill / 25;
    af.modifier  = skill;
    af.location  = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* VAMPIRIC AURA (potion, scroll, wands, staves) */

int cast_vampiric_aura( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
  if (GET_ALIGNMENT(ch) > -1000)
  {
    send_to_char("You must be utterly vile to cast this spell.\r\n",ch);
    return eFAILURE;
  }
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


/* HOLY AURA */

int spell_holy_aura(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_HOLY_AURA_TIMER)) {
     send_to_char("Your god is not so foolish as to grant that power to you so soon again.\r\n", ch);
     return eFAILURE;
  }

  act("A serene calm comes over $n.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("A serene calm encompasses you.", victim, 0, 0, TO_CHAR, 0);
  GET_ALIGNMENT(ch) -= 200;
  GET_ALIGNMENT(ch) = MIN(1000, MAX((-1000), GET_ALIGNMENT(ch)));
  extern void zap_eq_check(char_data *ch);
  zap_eq_check(ch);
  af.type      = SPELL_HOLY_AURA;
  af.duration  = 4;
  af.modifier  = 50;
  af.location  = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  af.type      = SPELL_HOLY_AURA_TIMER;
  af.duration  = 20;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

int cast_holy_aura( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{

  switch (type) 
  {
    case SPELL_TYPE_SPELL:
	char buf[MAX_INPUT_LENGTH];
        if (affected_by_spell(ch, SPELL_SANCTUARY))
	{
	  act("Your god does not allow that much power to be bestowed upon you.",ch, 0, 0, TO_CHAR,0);
	  return eFAILURE;
	}
	if (GET_ALIGNMENT(ch) < 1000)
	{
	  send_to_char("You are not holy enough to cast this spell.\r\n",ch);
	return eFAILURE;
	}
	one_argument(arg,buf);
	int mod;
	if (!str_cmp(buf,"magic"))
	   mod = 25;
	else if (!str_cmp(buf,"physical"))
	  mod = 50;
	else {
	send_to_char("You need to specify whether you want protection against magic or physical.\r\n",ch);
	ch->mana += spell_info[SPELL_HOLY_AURA].min_usesmana;	
	return eFAILURE;
	}
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_HOLY_AURA_TIMER)) {
     send_to_char("Your god is not so foolish as to grant that power to you so soon again.\r\n", ch);
     return eFAILURE;
  }


  act("A serene calm comes over $n.", ch, 0, 0, TO_ROOM, INVIS_NULL);
  act("A serene calm encompasses you.", ch, 0, 0, TO_CHAR, 0);
  GET_ALIGNMENT(ch) -= 250;
  GET_ALIGNMENT(ch) = MIN(1000, MAX((-1000), GET_ALIGNMENT(ch)));
  extern void zap_eq_check(char_data *ch);
  zap_eq_check(ch);
  af.type      = SPELL_HOLY_AURA;
  af.duration  = 4;
  af.modifier  = mod;
  af.location  = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  af.type      = SPELL_HOLY_AURA_TIMER;
  af.duration  = 20;
  affect_to_char(ch, &af);

       break;
    default :
       log("Serious screw-up in holy aura!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

/* DISMISS FAMILIAR */

int spell_dismiss_familiar(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   victim = NULL;

   for(struct follow_type *k = ch->followers; k; k = k->next)
     if(IS_MOB(k->follower) && IS_AFFECTED(k->follower, AFF_FAMILIAR))
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

/* DISMISS FAMILIAR (potions, wands, scrolls, staves) */

int cast_dismiss_familiar( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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

/* DISMISS CORPSE */

int spell_dismiss_corpse(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
   victim = NULL;
   //send_to_char("Disabled.\r\n",ch);
   //return eFAILURE;

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

/* DISMISS CORPSE (wands, scrolls, potions, staves) */

int cast_dismiss_corpse( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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

/* VISAGE OF HATE */

int spell_visage_of_hate(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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
    af.modifier  = - skill / 25;
    af.location  = APPLY_HITROLL;
    af.bitvector = -1;
    affect_to_char(tmp_char, &af);
    af.modifier  = 1 + skill / 25;
    af.location  = APPLY_DAMROLL;
    affect_to_char(tmp_char, &af);
  }

  send_to_char("Your disdain and hate for all settles upon your peers.\r\n", ch);  
  return eSUCCESS;  
}

int cast_visage_of_hate( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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

/* BLESSED HALO */

int spell_blessed_halo(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
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
    af.modifier  = skill / 18;
    af.location  = APPLY_HITROLL;
    af.bitvector = -1;
    affect_to_char(tmp_char, &af);
    af.modifier  = skill / 18;
    af.location  = APPLY_HP_REGEN;
    affect_to_char(tmp_char, &af);
  }

  send_to_char("Your group members benefit from your blessing.\r\n", ch);  
  return eSUCCESS;  
}

int cast_blessed_halo( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
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

/* SPIRIT WALK (GHOST WALK) */

int spell_ghost_walk(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill)
{
  if (ch->fighting)
  {
	send_to_char("You're a bit too distracted by the battle.\r\n",ch);
	return eFAILURE;
  }
  if (!ch->desc || ch->desc->snooping || IS_NPC(ch) || !ch->in_room)
  {
    send_to_char("You can't do that at the moment.\r\n",ch);
    return eFAILURE;
  }

  if (ch->desc->snoop_by)
  {
                 send_to_char ("Whoa! Almost got caught snooping!\n",ch->desc->snoop_by->character);
                 send_to_char ("Your victim is casting spiritwalk spell.\r\n", ch->desc->snoop_by->character);
                 do_snoop (ch->desc->snoop_by->character, ch->desc->snoop_by->character->name, 0);
  }
  int vnum;
  switch (world[ch->in_room].sector_type)
  {
    case SECT_INSIDE:
    case SECT_CITY:
    case SECT_PAVED_ROAD:
	vnum = 99;
	break;
    case SECT_FIELD:
    case SECT_HILLS:
    case SECT_MOUNTAIN:
	vnum = 98;
	break;
    case SECT_WATER_SWIM:
    case SECT_WATER_NOSWIM:
    case SECT_UNDERWATER:
	vnum = 97;
	break;
    case SECT_AIR:
	vnum = 96;
	break;
    case SECT_FROZEN_TUNDRA:
    case SECT_ARCTIC:
    	vnum = 95;
	break;
    case SECT_DESERT:
    case SECT_BEACH:
	vnum = 94;
	break;
    case SECT_SWAMP:
	vnum = 93;
	break;
    case SECT_FOREST:
//  case SECT_JUNGLE:
      vnum = 92;
	break;
	default:
	send_to_char("Invalid sectortype. Please report location to an imm.\r\n",ch);
	return eFAILURE;
  }
  int mobile;
  if ( ( mobile = real_mobile( vnum ) ) < 0 )
  {
      logf( IMMORTAL, LOG_WORLD, "Ghostwalk - Bad mob vnum: vnum %d.", vnum );
      send_to_char("\"Spirit\" for this sector not yet implented.\r\n",ch);
      return eFAILURE|eINTERNAL_ERROR;
  }

  struct char_data *mob;
  mob = clone_mobile( mobile );
  mob->hometown = world[ch->in_room].number;
  char_to_room( mob, ch->in_room );

  send_to_char("You call upon the spirits of this area, shifting into a trance-state.\r\n",ch);
  send_to_char("(Use the 'return' command to return to your body).\r\n",ch);
  ch->pcdata->possesing = 1;
  ch->desc->character = mob;
  ch->desc->original = ch;
  mob->desc = ch->desc;
  ch->desc = 0;
  return eSUCCESS;
}

int cast_ghost_walk( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
       return spell_ghost_walk(level, ch, tar_ch, 0, skill);
       break;
    default :
       log("Serious screw-up in ghost_walk!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

/* CONJURE ELEMENTAL */

int spell_conjure_elemental(ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, struct obj_data *component, int skill)
{
  CHAR_DATA *mob;
  int r_num,liquid, virt;
// 88 = fire fire
// 89 = water cold
// 90 = air energy
// 91 = earth magic

  if(many_charms(ch))  {
    send_to_char("How do you plan on controlling so many followers?\n\r", ch);
    return eFAILURE;
  }
//  OBJ_DATA *container  = NULL;
  if (!str_cmp(arg, "fire"))
  {
	virt = 88;
	liquid = 13;
  }
  else if (!str_cmp(arg, "water"))
  {
	virt = 89;
	liquid = 17;
  }
  else if (!str_cmp(arg, "air"))
  {
	virt = 90;
	liquid = 2;
  }
  else if (!str_cmp(arg, "earth"))
  {
	virt = 91;
	liquid = 9;
  }
  else {
	send_to_char("Unknown elemental type. Applicable types are: fire, water, air and earth.\r\n",ch);
	return eFAILURE;
  }
  obj_data *obj;
  for (obj = ch->carrying; obj; obj = obj->next_content)
  {
     if (obj->obj_flags.type_flag == ITEM_DRINKCON &&
		obj->obj_flags.value[2] == liquid &&
		obj->obj_flags.value[1] >= 5)
		break;
  }
  if (!obj)
  {
     send_to_char("You do not have the required liquid ready.\r\n",ch);
     return eFAILURE;
  }
  obj->obj_flags.value[1] -= 5;
  switch (virt)
  {
	case 88:
          act("Your container of blood burns hotly as a denizen of the elemental plane of fire arrives in a blast of flame!", ch, NULL,NULL, TO_CHAR, 0);
          act("$n's container of blood burns hotly as a denizen of the elemental plane of fire arrives in a blast of flame!", ch, NULL,NULL, TO_ROOM, 
0);
	  break;
	case 89:
          act("Your holy water bubbles briefly as an icy denizen of the elemental plane of water crystalizes into existence.", ch, NULL,NULL, 
TO_CHAR, 0);
          act("$n's holy water bubbles briefly as an icy denizen of the elemental plane of water crystalizes intoto existence.", ch, NULL,NULL, 
TO_ROOM, 0);
	  break;
	case 91:
          act("Your dirty water churns violently as a denizen of the elemental plane of earth rises from the ground.", ch, NULL,NULL, TO_CHAR, 0);
          act("$n's dirty water churns violently as a denizen of the elemental plane of earth rises from the ground.", ch, NULL,NULL, TO_ROOM, 0);
	  break;
	case 90:
          act("Your wine boils with energy as a denizen of the elemental plane of air crackles into existance.", ch, NULL,NULL, TO_CHAR, 0);
          act("$n's wine boils with energy as a denizen of the elemental plane of air crackles into existance.", ch, NULL,NULL, TO_ROOM, 0);
	  break;
	default:
		send_to_char("That item is not used for elemental summoning.\r\n",ch);
		return eFAILURE;
  }
  if ((r_num = real_mobile(virt)) < 0) {
    send_to_char("Mobile: Elemental not found.\n\r", ch);
    return eFAILURE;
  }
  
  mob = clone_mobile(r_num);
  char_to_room(mob, ch->in_room);
  mob->max_hit += skill*5;
  mob->hit = mob->max_hit;
  if (skill > 80) mob->height = 77;
  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  SETBIT(mob->affected_by, AFF_CHARM);

  add_follower(mob, ch, 0);

  return eSUCCESS;
}

/* MEND GOLEM */

int cast_mend_golem( ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill )
{
  switch (type) 
  {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_STAFF:
       return spell_mend_golem(level, ch, tar_ch, 0, skill);
       break;
    default :
       log("Serious screw-up in ghost_walk!", ANGEL, LOG_BUG);
       break;
  }
  return eFAILURE;
}

/* DIVINE INTERVENTION */

int spell_divine_intervention(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, int skill)
{
  struct affected_type af;

  if(affected_by_spell(ch, SPELL_DIV_INT_TIMER)) {
    send_to_char("The gods are unwilling to intervene on your behalf again so soon.\n\r", ch);
    GET_MANA(ch) /= 2;
    return eFAILURE;
  }

  act("Light pours down from above bathing you in a shell of divine protection.", ch, 0, 0, TO_CHAR, 0);
  act("Light pours down from above and surrounds $n in a shell of divine protection.", ch, 0, 0, TO_ROOM, 0);

  GET_MANA(ch) = 0;

  af.type = SPELL_DIV_INT_TIMER;
  af.duration = 24;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  af.type = SPELL_DIVINE_INTER;
  af.duration = 6 + skill / 10;
  af.modifier = 10 - skill / 10;
  affect_to_char(ch, &af, PULSE_VIOLENCE);

  return eSUCCESS;
}

int cast_divine_intervention(ubyte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, OBJ_DATA *tar_obj, int skill)
{
  switch (type)
  {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_STAFF:
      return spell_divine_intervention(level, ch, 0, 0, skill);
      break;
    default:
      log("Serious screw-up in divine intervention!", ANGEL, LOG_BUG);
      break;
  }
  return eFAILURE;
}
