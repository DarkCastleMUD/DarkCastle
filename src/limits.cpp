/***************************************************************************
 *  file: limits.c , Limit and gain control module.        Part of DIKUMUD *
 *  Usage: Procedures controling gain and limit.                           *
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
/* $Id: limits.cpp,v 1.72 2006/06/02 07:07:38 jhhudso Exp $ */

extern "C"
{
#include <stdio.h>
#include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#ifdef BANDWIDTH
  #include <bandwidth.h>
#endif
#include <room.h>
#include <character.h>
#include <utility.h>
#include <mobile.h>
#include <isr.h>
#include <spells.h> // TYPE
#include <levels.h>
#include <player.h>
#include <obj.h>
#include <connect.h>
#include <db.h> // exp_table
#include <fight.h> // damage
#include <ki.h>
#include <game_portal.h>
#include <act.h>
#include <handler.h>
#include <race.h>
#include <returnvals.h>
#include <interp.h>
#include <clan.h> //totem

extern CHAR_DATA *character_list;
extern struct obj_data *object_list;
extern CWorld world;
extern struct index_data *obj_index;
extern struct index_data *mob_index;

/* External procedures */
void save_corpses(void);
void update_pos( CHAR_DATA *victim );                 /* in fight.c */
struct time_info_data age(CHAR_DATA *ch);

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

    if (age < 15)
	return(p0);                                 /* < 15   */
    else if (age <= 29) 
	return (int) (p1+(((age-15)*(p2-p1))/15));  /* 15..29 */
    else if (age <= 44)
	return (int) (p2+(((age-30)*(p3-p2))/15));  /* 30..44 */
    else if (age <= 59)
	return (int) (p3+(((age-45)*(p4-p3))/15));  /* 45..59 */
    else if (age <= 79)
	return (int) (p4+(((age-60)*(p5-p4))/20));  /* 60..79 */
    else
	return(p6);                                 /* >= 80 */
}

/* The three MAX functions define a characters Effective maximum */
/* Which is NOT the same as the ch->max_xxxx !!!          */
int32 mana_limit(CHAR_DATA *ch)
{
    int max;

    if (!IS_NPC(ch))
      max = (ch->max_mana);
    else
      max = (ch->max_mana);
    
    return(max);
}

int32 ki_limit(CHAR_DATA *ch)
{
	if(!IS_NPC(ch))
	    return(ch->max_ki);
	else
	    return(0);
}

int32 hit_limit(CHAR_DATA *ch)
{
    int max;

    if (!IS_NPC(ch))
      max = (ch->max_hit) +
	(graf(age(ch).year, 2,4,17,14,8,4,3));
    else 
      max = (ch->max_hit);

/* Class/Level calculations */

/* Skill/Spell calculations */
    
  return (max);
}


int32 move_limit(CHAR_DATA *ch)
{
    int max;

    if (!IS_NPC(ch))
	/* HERE SHOULD BE CON CALCULATIONS INSTEAD */
	max = (ch->max_move) + 
	  graf(age(ch).year, 50,70,160,120,100,40,20);
    else
	max = ch->max_move;

/* Class/Level calculations */

/* Skill/Spell calculations */

  return (max);
}

const int mana_regens[] = {
  0, 13, 13, 1, 1, 10, 9, 1, 1, 9, 1, 13, 0, 0
};

/* manapoint gain pr. game hour */
int mana_gain(CHAR_DATA *ch)
{
  int gain = 0;
  int divisor = 1;
  int modifier;
  
  if(IS_NPC(ch))
    gain = GET_LEVEL(ch);
  else {
//    gain = graf(age(ch).year, 2,3,4,6,7,8,9);

    gain = (int)(ch->max_mana * (float)mana_regens[GET_CLASS(ch)] / 100);
    switch (GET_POS(ch)) {
      case POSITION_SLEEPING: divisor = 1; break;
      case POSITION_RESTING:  divisor = 2; break;
      case POSITION_SITTING:  divisor = 2; break;
      default:                divisor = 3; break;
    }

    if(GET_CLASS(ch) == CLASS_MAGIC_USER ||
       GET_CLASS(ch) == CLASS_ANTI_PAL || GET_CLASS(ch) == CLASS_RANGER)
    {
      modifier = int_app[GET_INT(ch)].mana_regen;
      modifier += GET_INT(ch);
    }
    else {
      modifier = wis_app[GET_WIS(ch)].mana_regen;
      modifier += GET_WIS(ch);
    }
    gain += modifier;
  }


  if((GET_COND(ch,FULL)==0)||(GET_COND(ch,THIRST)==0))
    gain >>= 2;
  gain /= 4;
  gain /= divisor; 
  gain += MIN(age(ch).year,100) / 5;
  if (GET_LEVEL(ch) < 50)
  gain = (int)((float)gain * (2.0-(float)GET_LEVEL(ch)/50.0));

  gain += ch->mana_regen;
  if (ch->in_room >= 0)
  if (IS_SET(world[ch->in_room].room_flags, SAFE))
     gain = (int)(gain * 1.25);

  return MAX(1,gain);
}

const int hit_regens[] = {
 0, 7, 7, 9, 10, 8, 9, 12, 9, 8, 8, 7, 0, 0
};

/* Hitpoint gain pr. game hour */
int hit_gain(CHAR_DATA *ch)
{

  int gain = 1;
  struct affected_type * af;
  int divisor =1;
  int learned = has_skill(ch, SKILL_ENHANCED_REGEN);
 /* Neat and fast */
  if(IS_NPC(ch))
  {
   if (ch->fighting) gain = (GET_MAX_HIT(ch) / 24);
   else gain = (GET_MAX_HIT(ch) / 6);
  }
  /* PC's */
  else {
    gain = (int)(ch->max_hit * (float)hit_regens[GET_CLASS(ch)] /100);

    /* Position calculations    */
    switch (GET_POS(ch)) {
      case POSITION_SLEEPING: divisor = 1; break;
      case POSITION_RESTING:  divisor = 2; break;
      case POSITION_SITTING:  divisor = 2; break;
      default:                divisor = 3; break;
    }
    if(gain < 1) 
      gain = 1;

    if((af = affected_by_spell(ch, SPELL_RAPID_MEND)))
      gain += af->modifier;

    // con multiplier modifier 15 = 1.0  30 = 1.45 (.03 increments)
/*    if(GET_CON(ch) > 15)
      gain = (int)(gain * ((float)1+ (.03 * (GET_CON(ch) - 15.0))));
 
    if(GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_DRUID)
      gain = (int)((float)gain * 0.7);*/

     gain += con_app[GET_CON(ch)].hp_regen;
      gain += GET_CON(ch);
  }
  if (ISSET(ch->affected_by, AFF_REGENERATION))
    gain += (gain/2);

  if(learned && skill_success(ch, NULL, SKILL_ENHANCED_REGEN))
     gain += 3 + learned / 5;

  if((GET_COND(ch, FULL)==0) || (GET_COND(ch, THIRST)==0))
    gain >>= 2;
  gain /= 4;
  gain -= MIN(age(ch).year,100) / 10;

  gain /= divisor;
  gain += ch->hit_regen;
  if (GET_LEVEL(ch) < 50)
  gain = (int)((float)gain * (2.0-(float)GET_LEVEL(ch)/50.0));

  if (ch->in_room >= 0)
  if (IS_SET(world[ch->in_room].room_flags, SAFE))
     gain = (int)(gain * 1.5);
 
  return MAX(1,gain);
}


int ki_gain_level(CHAR_DATA *ch)
{
	if(IS_NPC(ch))
		return 0;

	/* Monks gain one point /level */
	if(GET_CLASS(ch) == CLASS_MONK)
		return 1;

	/* If they're stll here they're not a monk */
	/* Other chars gain one point every other level */
	if(GET_LEVEL(ch) % 2)
		return 1;
	return 0;
}

int move_gain(CHAR_DATA *ch)
/* move gain pr. game hour */
{
    int gain;
    int divisor = 100000;
    int learned = has_skill(ch, SKILL_ENHANCED_REGEN);
    struct affected_type * af;

    if(IS_NPC(ch)) {
	return(GET_LEVEL(ch));  
	/* Neat and fast */
    } else {
//	gain = graf(age(ch).year, 4,5,6,7,4,3,2);
	gain = (int)(ch->max_move * 0.15);
//	gain /= 2;
	switch (GET_POS(ch)) {
	    case POSITION_SLEEPING: divisor = 1; break;
	    case POSITION_RESTING:  divisor = 2; break;
	    default:                divisor = 3; break;
	}
	gain += GET_DEX(ch);
	gain += con_app[GET_CON(ch)].move_regen;

        if((af = affected_by_spell(ch, SPELL_RAPID_MEND)))
          gain += (int)(af->modifier * 1.5);
    }


    if((GET_COND(ch,FULL)==0)||(GET_COND(ch,THIRST)==0))
	gain >>= 2;
   gain /= divisor;
   gain -= MIN(100, age(ch).year) / 10;
 
   gain += ch->move_regen;

  if(learned && skill_success(ch, NULL, SKILL_ENHANCED_REGEN))
     gain += 3 + learned / 10;

  if (GET_LEVEL(ch) < 50)
  gain = (int)((float)gain * (2.0-(float)GET_LEVEL(ch)/50.0));

  if (ch->in_room >= 0)
  if (IS_SET(world[ch->in_room].room_flags, SAFE))
     gain = (int)(gain * 1.5);
    return MAX(1,gain);
}

void redo_hitpoints( CHAR_DATA *ch)
{
  /*struct affected_type *af;*/
  int i, j, bonus = 0;

  ch->max_hit = ch->raw_hit;
  for (i = 16; i < GET_CON(ch); i++)
   bonus += (i * i) / 30;
//  bonus = (GET_CON(ch) * GET_CON(ch)) / 30; 

  ch->max_hit += bonus;
  struct affected_type * af = ch->affected;

  while(af) {
    if(af->location == APPLY_HIT)
	ch->max_hit += af->modifier;
    af = af->next;
  }


  for (i=0; i<MAX_WEAR; i++) 
  {
    if (ch->equipment[i])
      for (j=0; j<ch->equipment[i]->num_affects; j++) 
      {
        if (ch->equipment[i]->affected[j].location == APPLY_HIT)
          affect_modify(ch, ch->equipment[i]->affected[j].location,
                           ch->equipment[i]->affected[j].modifier,
                           -1, TRUE);
      }
  }
  add_totem_stats(ch, APPLY_HIT);
}


void redo_mana ( CHAR_DATA *ch)

{
   /*struct affected_type *af;*/
   int i, j, bonus = 0, stat = 0;
   if (IS_NPC(ch)) return;
  ch->max_mana = ch->raw_mana;

  if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL || GET_CLASS(ch) == CLASS_RANGER)
     stat = GET_INT(ch);
  else
     stat = GET_WIS(ch);
 
  for (i = 16; i < stat; i++)
   bonus += (i * i) / 30;

  if ((GET_CLASS(ch) == CLASS_WARRIOR) || (GET_CLASS(ch) == CLASS_THIEF) ||
      (GET_CLASS(ch) == CLASS_BARBARIAN) || (GET_CLASS(ch) == CLASS_MONK))
    bonus = 0;

  ch->max_mana += bonus;

  struct affected_type *af;
  af = ch->affected;
  while(af) {
    if(af->location == APPLY_MANA)
        ch->max_mana += af->modifier;
    af = af->next;
  }
 for (i=0; i<MAX_WEAR; i++) 
  {
    if (ch->equipment[i])
      for (j=0; j<ch->equipment[i]->num_affects; j++) 
      {
        if (ch->equipment[i]->affected[j].location == APPLY_MANA)
          affect_modify(ch, ch->equipment[i]->affected[j].location,
                           ch->equipment[i]->affected[j].modifier,
                           -1, TRUE);
      }
  }
  add_totem_stats(ch, APPLY_MANA);
}

void redo_ki(CHAR_DATA *ch)
{
   int i,j;
   ch->max_ki = ch->raw_ki;
   if (GET_CLASS(ch) == CLASS_MONK) 
     ch->max_ki += GET_WIS(ch) > 15 ? GET_WIS(ch) - 15:0;
   else if (GET_CLASS(ch) == CLASS_BARD)
     ch->max_ki += GET_INT(ch) > 15 ? GET_INT(ch)-15:0;   

  struct affected_type *af;
  af = ch->affected;
  while(af) {
    if(af->location == APPLY_KI)
        ch->max_ki += af->modifier;
    af = af->next;
  }

  for (i=0; i<MAX_WEAR; i++)
  {
    if (ch->equipment[i])
      for (j=0; j<ch->equipment[i]->num_affects; j++)
      {
        if (ch->equipment[i]->affected[j].location == APPLY_KI)
          affect_modify(ch, ch->equipment[i]->affected[j].location,
                           ch->equipment[i]->affected[j].modifier,
                           -1, TRUE);
      }
  }
  add_totem_stats(ch, APPLY_KI);
}

/* Gain maximum in various */
void advance_level(CHAR_DATA *ch, int is_conversion)
{
    int add_hp = 0;
    int add_mana = 1;
    int add_moves = 0;
    int add_ki = 0;
    int add_practices;
    int i;
    char buf[MAX_STRING_LENGTH];

    switch(GET_CLASS(ch)) { 
    case CLASS_MAGIC_USER:
	add_ki	    += (GET_LEVEL(ch) % 2);
	add_hp      += number(3, 6);
	add_mana    += number(5, 10);
	add_moves   += number(1, (GET_CON(ch) / 2));
	break;

    case CLASS_CLERIC:
	add_ki	    += (GET_LEVEL(ch) % 2);
	add_hp      += number(4, 8);
	add_mana    += number(4, 9);
	add_moves   += number(1, (GET_CON(ch) / 2));
	break;

    case CLASS_THIEF:
	add_ki	    += (GET_LEVEL(ch) % 2);
	add_hp      += number(4 , 11);
	add_moves   += number(1, (GET_CON(ch) / 2));
	break;

    case CLASS_WARRIOR:
	add_ki	    += (GET_LEVEL(ch) % 2);
	add_hp      += number(14,18);
	add_moves   += number(1, (GET_CON(ch) / 2));
	break;

    case CLASS_ANTI_PAL:
	add_ki	    += (GET_LEVEL(ch) % 2);
        add_hp      += number(8, 12);
        add_mana    += number(3, 5);
        add_moves   += number(1, (GET_CON(ch) / 2));
        break;

    case CLASS_PALADIN:
	add_ki	    += (GET_LEVEL(ch) % 2);
        add_hp      += number(10, 14);
        add_mana    += number(2, 4);
        add_moves   += number(1, (GET_CON(ch) / 2));
        break;
   
    case CLASS_BARBARIAN:
	add_ki	     += (GET_LEVEL(ch) % 2);
        add_hp       += number(16,20);
        add_moves    += number(1, (GET_CON(ch) / 2));
        break;

     case CLASS_MONK:
	add_ki	     += 1;
        add_hp       += number(10, 14);
        add_moves    += number(1, (GET_CON(ch) / 2));
        GET_AC(ch)   += -2;
        break;

     case CLASS_RANGER:
	add_ki       += (GET_LEVEL(ch) % 2);
        add_hp       += number(8, 12);
        add_mana     += number(3, 5);
        add_moves    += number(1, (GET_CON(ch) / 2)); 
        break;

     case CLASS_BARD:
       add_ki       += 1;
       add_hp       += number(6, 10);
       add_mana     += 0;
       add_moves    += number(1, (GET_CON(ch) / 2)); 
       break;

     case CLASS_DRUID:
       add_ki       += (GET_LEVEL(ch) % 2);;
       add_hp       += number(5, 9);
       add_mana     += number(4, 9);
       add_moves    += number(1, (GET_CON(ch) / 2)); 
       break;

     default:  log("Unknown class in advance level?", OVERSEER, LOG_BUG);
       return;
    }

/*  if ((GET_CLASS(ch) == CLASS_MAGIC_USER) ||
        (GET_CLASS(ch) == CLASS_RANGER) ||
        (GET_CLASS(ch) == CLASS_ANTI_PAL))
       add_mana +=  int_app[GET_INT(ch)].mana_gain;
    else if (GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_DRUID || 
	     GET_CLASS(ch) == CLASS_PALADIN)
       add_mana += wis_app[GET_WIS(ch)].mana_gain;
*/
    add_hp += con_app[GET_CON(ch)].hp_gain;
    add_moves += dex_app[GET_DEX(ch)].move_gain;
    add_hp			 = MAX( 1, add_hp);
    add_mana			 = MAX( 0, add_mana);
    add_moves			 = MAX( 1, add_moves);
    add_practices		 = 1+wis_app[GET_WIS(ch)].bonus;

    // hp and mana have stat bonuses related to level so have to have their stuff recalculated
    ch->raw_hit			+= add_hp;
    ch->raw_mana		+= add_mana;
    redo_hitpoints(ch);
    redo_mana (ch);
    // move and ki aren't stat related, so we just add directly to the totals
    ch->raw_move		+= add_moves;
    ch->max_move		+= add_moves;

    ch->raw_ki			+= add_ki;
    ch->max_ki			+= add_ki;
    redo_ki(ch); // Ki gets level bonuses now
    if(!IS_MOB(ch) && !is_conversion)
      ch->pcdata->practices	+= add_practices;

    sprintf( buf,
	"Your gain is: %d/%d hp, %d/%d m, %d/%d mv, %d/%d prac, %d/%d ki.\n\r",
	add_hp,        GET_MAX_HIT(ch),
	add_mana,      GET_MAX_MANA(ch),
	add_moves,     GET_MAX_MOVE(ch),
	IS_MOB(ch) ? 0 : add_practices, IS_MOB(ch) ? 0 : ch->pcdata->practices,
	add_ki,        GET_MAX_KI(ch)
	);
    if(!is_conversion)
      send_to_char( buf, ch );

    if(GET_LEVEL(ch) % 3 == 0)
      for(int i = 0; i <= SAVE_TYPE_MAX; i++)
        ch->saves[i]++;

    if (GET_LEVEL(ch) > IMMORTAL)
	for (i = 0; i < 3; i++)
	    ch->conditions[i] = -1;

    if(GET_LEVEL(ch) == 10)
      send_to_char("You will no longer keep your equipment when you die.\r\n", ch);
    if(GET_LEVEL(ch) == 11)
      send_to_char("It now costs you gold every time you recall.\r\n", ch);
}   

void gain_exp( CHAR_DATA *ch, int64 gain )
{
  int x = 0;
  int64 y;

  if(!IS_NPC(ch) && GET_LEVEL(ch) >= IMMORTAL)  
    return;

  y = exp_table[GET_LEVEL(ch)+1];  

  if(GET_EXP(ch) >= y)
    x = 1; 

/*  if(GET_EXP(ch) > 2000000000)
  {
    send_to_char("You have hit the 2 billion xp cap.  Convert or meta chode.\r\n", ch);
    return;
  }*/
  GET_EXP(ch) += gain;
  if( GET_EXP(ch) < 0 )
    GET_EXP(ch) = 0;

  void golem_gain_exp(CHAR_DATA *ch);

  if (!IS_NPC(ch) && ch->pcdata->golem && ch->in_room == ch->pcdata->golem->in_room) // Golems get mage's exp, when they're in the same room
    gain_exp(ch->pcdata->golem, gain);

  if (IS_NPC(ch) && mob_index[ch->mobdata->nr].virt == 8) // it's a golem
    golem_gain_exp(ch);

  if(IS_NPC(ch))
    return;

  if(!x && GET_EXP(ch) >= y)
     send_to_char("You now have enough experience to level!\n\r", ch);

  return;
}


void gain_exp_regardless( CHAR_DATA *ch, int gain )
{
  GET_EXP(ch) += (int64)gain;

  if(GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if(IS_NPC(ch))
    return;

  while(GET_EXP(ch) >= (int32)exp_table[GET_LEVEL(ch) + 1]) {
    send_to_char( "You raise a level!!  ", ch );
    GET_LEVEL(ch) += 1;
    advance_level(ch, 0);
  }

  return;
}

void gain_condition(CHAR_DATA *ch,int condition,int value)
{
    bool intoxicated;

    if(GET_COND(ch, condition)==-1) /* No change */
	return;

    if(condition == FULL && IS_AFFECTED(ch, AFF_BOUNT_SONNET_HUNGER))
       return;
    if(condition == THIRST && IS_AFFECTED(ch, AFF_BOUNT_SONNET_THIRST))
       return;

    intoxicated=(GET_COND(ch, DRUNK) > 0);

    GET_COND(ch, condition)  += value;

    GET_COND(ch,condition) = MAX(0,(int)GET_COND(ch,condition));
    GET_COND(ch,condition) = MIN(24,(int)GET_COND(ch,condition));

    if(GET_COND(ch,condition))
	return;

    switch(condition){
	case FULL :
	{
	    send_to_char("You are hungry.\n\r",ch);
	    return;
	}
	case THIRST :
	{
	    send_to_char("You are thirsty.\n\r",ch);
	    return;
	}
	case DRUNK :
	{
	    if(intoxicated)
		send_to_char("You are now sober.\n\r",ch);
	    return;
	}
	default : break;
    }

    // just for fun
    if(1 == number(1, 2000)) {
      send_to_char("You are horny\r\n", ch);
    }
}

void food_update( void )
{
  struct obj_data * bring_type_to_front(char_data * ch, int item_type);
  int do_eat(struct char_data *ch, char *argument, int cmd);
  int do_drink(struct char_data *ch, char *argument, int cmd);
  int FOUNTAINisPresent (CHAR_DATA *ch);

  CHAR_DATA *i, *next_dude;
  struct obj_data * food = NULL;

  for(i = character_list; i; i = next_dude) 
  {
    next_dude = i->next;
    if (affected_by_spell(i,SPELL_PARALYZE))
      continue;
    int amt = -1;
    if (i->equipment[WEAR_FACE] && 
	obj_index[i->equipment[WEAR_FACE]->item_number].virt == 536)
		amt = -3;
    gain_condition(i,FULL,amt);
    if(!GET_COND(i, FULL)) { // i'm hungry
      if(!IS_MOB(i) && IS_SET(i->pcdata->toggles, PLR_AUTOEAT) && (GET_POS(i) > POSITION_SLEEPING)) {
        if(IS_DARK(i->in_room) && !IS_MOB(i) && !i->pcdata->holyLite && !affected_by_spell(i, SPELL_INFRAVISION))
          send_to_char("It's too dark to see what's safe to eat!\n\r", i);
        else if(FOUNTAINisPresent(i))
          do_drink(i, "fountain", 9);
        else if((food =  bring_type_to_front(i, ITEM_FOOD)))
          do_eat(i, food->name, 9);
        else send_to_char("You are out of food.\n\r", i);
      }
    }
    gain_condition(i,DRUNK,-1);
    gain_condition(i,THIRST,amt);
    if(!GET_COND(i, THIRST)) { // i'm thirsty
      if(!IS_MOB(i) && IS_SET(i->pcdata->toggles, PLR_AUTOEAT) && (GET_POS(i) > POSITION_SLEEPING)) {
        if(IS_DARK(i->in_room) && !IS_MOB(i) && !i->pcdata->holyLite && !affected_by_spell(i, SPELL_INFRAVISION))
          send_to_char("It's too dark to see if there's any potable liquid around!\n\r", i);
        else if(FOUNTAINisPresent(i))
          do_drink(i, "fountain", 9);
        else if((food =  bring_type_to_front(i, ITEM_DRINKCON)))
          do_drink(i, food->name, 9);
        else send_to_char("You are out of drink.\n\r", i);
      }          
    }
  }
}

// Update the HP of mobs and players
// Also clears out any linkdead level 1s
void point_update( void )
{   

  CHAR_DATA *i, *next_dude;

  /* characters */
  for(i = character_list; i; i = next_dude) 
  {
    next_dude = i->next;
    if (affected_by_spell(i, SPELL_POISON)) continue;

    int a;
     CHAR_DATA *temp;
    if (!IS_NPC(i) && ISSET(i->affected_by, AFF_HIDE) && (a = has_skill(i, SKILL_HIDE)))
    {
	int o;
        for (o = 0; o < MAX_HIDE;o++)
          i->pcdata->hiding_from[o] = NULL;
        o = 0;
        for (temp = world[i->in_room].people; temp; temp = temp->next_in_room)
        {
          if (i == temp) continue;
          if (o >= MAX_HIDE) break;
          if (number(1,101) > a) // Failed.
          {
            i->pcdata->hiding_from[o] = temp;
            i->pcdata->hide[o++] = FALSE;
          } else {
            i->pcdata->hiding_from[o] = temp;
            i->pcdata->hide[o++] = TRUE;
          }
        }
   }


    // only heal linkalive's and mobs
    if(GET_POS(i) > POSITION_DEAD && (IS_NPC(i) || i->desc)) {
      GET_HIT(i)  = MIN(GET_HIT(i)  + hit_gain(i),  hit_limit(i));
      GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), mana_limit(i));
      GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), move_limit(i));
      GET_KI(i)   = MIN(GET_KI(i)   + ki_gain(i),   ki_limit(i));
    }
    else if( !IS_MOB(i) && GET_LEVEL(i) < 2 && !i->desc ) {
      act("$n fades away into obscurity; $s life leaving history with nothing of note.", i, 0, 0, TO_ROOM, 0);
      do_quit(i, "", 666);
    }
  } /* for */
}

void update_corpses_and_portals(void)
{
  //char buf[256];
  struct obj_data *j, *next_thing;
  struct obj_data *jj, *next_thing2;
  struct obj_data *last_thing;
  int proc = 0; // Processed items. Debugging.
  void extract_obj(struct obj_data *obj); /* handler.c */
  /* objects */
  for(j = object_list; j ; j = next_thing,proc++)
  {
   if (j == (struct obj_data *)0x95959595) break;
    next_thing = j->next; /* Next in object list */
    /* Type 1 is a permanent game portal, and type 3 is a look_only
    |  object.  Type 0 is the spell portal and type 2 is a game_portal
    */

    if((GET_ITEM_TYPE(j) == ITEM_PORTAL) && (j->obj_flags.value[1] == 0
        || j->obj_flags.value[1] == 2)) 
    {
      if(j->obj_flags.timer > 0)
        (j->obj_flags.timer)--;
        if(!(j->obj_flags.timer)) 
        {
          if((j->in_room != NOWHERE) && (world[j->in_room].people)) {
            act("$p shimmers brightly and then fades away.", world[j->in_room].people, j, 0, TO_ROOM, INVIS_NULL);
            act("$p shimmers brightly and then fades away.", world[j->in_room].people, j, 0, TO_CHAR, INVIS_NULL);
          }
          extract_obj(j);
          continue;
        }
    }

    /* If this is a corpse */
    else if((GET_ITEM_TYPE(j) == ITEM_CONTAINER) &&
            (j->obj_flags.value[3]))
    {
            // TODO ^^^ - makes value[3] for containers a bitvector instead of a boolean

      /* timer count down */
      if (j->obj_flags.timer > 0) { j->obj_flags.timer--; save_corpses(); }

      if (!j->obj_flags.timer) 
      {
        if (j->carried_by)
          act("$p decays in your hands.", j->carried_by, j, 0, TO_CHAR, 0);
        else if ((j->in_room != NOWHERE) && (world[j->in_room].people))
        {
          act("A quivering horde of maggots consumes $p.", world[j->in_room].people, j, 0, TO_ROOM, INVIS_NULL);
          act("A quivering horde of maggots consumes $p.", world[j->in_room].people, j, 0, TO_CHAR, 0);
        }

        for(jj = j->contains; jj; jj = next_thing2) 
        {
          next_thing2 = jj->next_content; /* Next in inventory */

          if (j->in_obj) {
            if(IS_SET(jj->obj_flags.more_flags, ITEM_NO_TRADE)|| (GET_ITEM_TYPE(jj) == ITEM_CONTAINER && contains_no_trade_item(jj)))
            {
             if (next_thing == jj)
                next_thing = jj->next;
             while (next_thing && next_thing->in_obj == jj)
                next_thing = next_thing->next;
             extract_obj(jj);
            }
            else
              move_obj(jj, j->in_obj);
          }
          else if (j->carried_by) {
            if(IS_SET(jj->obj_flags.more_flags, ITEM_NO_TRADE)|| (GET_ITEM_TYPE(jj) == ITEM_CONTAINER && contains_no_trade_item(jj)))
            {
             if (next_thing == jj)
               next_thing = jj->next;
             while (next_thing->in_obj == jj)
                next_thing = next_thing->next;
             extract_obj(jj);
            }
            else
              move_obj(jj, j->carried_by);
          }
          else if (j->in_room != NOWHERE)
          {
            // no trade objects disappear when the corpse decays
	    // Caused a crasher. Fixed it. 
            if(IS_SET(jj->obj_flags.more_flags, ITEM_NO_TRADE)|| (GET_ITEM_TYPE(jj) == ITEM_CONTAINER && contains_no_trade_item(jj)))
            {
	     if (next_thing == jj)
	       next_thing = jj->next;
	     while (next_thing->in_obj == jj)
		next_thing = next_thing->next;
	     extract_obj(jj);
	    }
	    else
   	      move_obj(jj, j->in_room);
          }
          else {
            log("BIIIG problem in limits.c!", OVERSEER, LOG_BUG);
            return;
          }
        }
	while (next_thing && next_thing->in_obj == j) next_thing = next_thing->next;
			// Is THIS what caused the crasher then?
			// Wtf: damnit.
        extract_obj(j);
        save_corpses();
      }
      last_thing = j;
    }
  }
  //sprintf(buf, "DEBUG: Processed Objects: %d", proc);
  //log(buf, 108, LOG_BUG);
  /* Now process the portals */
 // process_portals();
}
