/************************************************************************
| $Id: offense.cpp,v 1.27 2011/11/29 02:54:06 jhhudso Exp $
| offense.C
| Description:  Commands that are generically offensive - that is, the
|   victim should retaliate.  The class-specific offensive commands are
|   in class/
*/

#include <cctype>
#include <cstring>

#include "structs.h"
#include "character.h"
#include "utility.h"
#include "handler.h"
#include "spells.h"
#include "fight.h"
#include "connect.h"
#include "mobile.h"
#include "levels.h"
#include "act.h"
#include "returnvals.h"
#include "room.h"
#include "db.h"
#include "interp.h"
#include "const.h"

int do_suicide(struct char_data *ch, char *argument, int cmd)
{
  if (IS_NPC(ch)) return eFAILURE; // just in case 
  if (IS_SET(world[ch->in_room].room_flags, SAFE))
  {
	send_to_char("This place is too peaceful for that.\r\n",ch);
	return eFAILURE;
  }
  if (IS_SET(world[ch->in_room].room_flags, ARENA))
  {
	send_to_char("You can't do that in the arena.\r\n",ch);
	return eFAILURE;
  }
 if(affected_by_spell(ch, FUCK_PTHIEF) || (affected_by_spell(ch, FUCK_GTHIEF))) {
	send_to_char("You're too busy running from the law!\r\n",ch);
	return eFAILURE;
  }
  if(IS_AFFECTED(ch, AFF_CHAMPION)) {
     send_to_char("You have no reason to feel sad, oh great Champion!\n\r", ch);
     return eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_CURSE) || IS_AFFECTED(ch, AFF_SOLIDITY))
  {
    send_to_char("Something blocks your attempted suicide, be happy!  You have a new lease on life!\r\n",ch);
    return eFAILURE;
  }
  if(GET_POS(ch) == POSITION_FIGHTING || ch->fighting) {
    send_to_char("You are too busy trying to kill somebody else!\n\r", ch);
    return eFAILURE;
  }

  int percent = number(1,100);
  if (percent > GET_WIS(ch)) percent -= GET_WIS(ch);

  if (percent > 50)
  {
	send_to_char("You miss your wrists with the blade and knick your kneecap!\r\n",ch);
	act("$n tries to suicide, but fails miserably.",ch, 0, 0, TO_ROOM, 0);
        return eFAILURE;
  }
  send_to_char("Looking out upon the world, you decide that it would be a better place without you.\r\n",ch);
  act("Tired of life, $n decides to end $s.",ch, 0, 0, TO_ROOM, 0);
  fight_kill(ch, ch, TYPE_PKILL, 0);
  return eSUCCESS;
}

// TODO - check differences between hit, murder, and kill....I think we can
// just pull out alot of the code into a function.
int do_hit(struct char_data *ch, char *argument, int cmd)
{
  char arg[MAX_STRING_LENGTH];
  struct char_data *victim, *k, *next_char;
  int count = 0;
  
  one_argument(argument, arg);
  
  if (*arg) {
    victim = get_char_room_vis(ch, arg);
    if (victim) {
      if (victim == ch) {
	send_to_char("You hit yourself..OUCH!.\n\r", ch);
	act("$n hits $mself, and says OUCH!",
	    ch, 0, victim, TO_ROOM, 0);
      }
      else {
	if(!can_attack(ch) || !can_be_attacked(ch, victim))
	  return eFAILURE;
	
	if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == victim)) {
	  act("$N is just such a good friend, you simply can't hit $M.",
	      ch, 0, victim, TO_CHAR, 0);
	  return eFAILURE;
	}
	
	if ((GET_POS(ch) == POSITION_STANDING) &&
	    (victim != ch->fighting)) {
	  
	  for (k = combat_list; k; k = next_char) {
	    next_char = k->next_fighting;
	    if (k->fighting == victim)
	      count++;
	  }
	  /*
	  if (count >= 6) {
            send_to_char("You can't get close enough to do anything.", ch);
	    return eFAILURE;
	  }
	  */
	  WAIT_STATE(ch, PULSE_VIOLENCE);
	  return attack(ch, victim, TYPE_UNDEFINED);
	}
	else
	  send_to_char("You do the best you can!\n\r",ch);
      }
    }
    else
      send_to_char("They aren't here.\n\r", ch);
  }
  else
    send_to_char("Hit whom?\n\r", ch);
  return eFAILURE;
}


int do_murder(struct char_data *ch, char *argument, int cmd)
{
  char arg[MAX_STRING_LENGTH];
  struct char_data *victim;
  
  one_argument(argument, arg);
  
  if (*arg) {
    victim = get_char_room_vis(ch, arg);
    if (victim) {
      if (victim == ch) {
        send_to_char("You hit yourself..OUCH!.\n\r", ch);
        act("$n hits $mself, and says OUCH!", ch, 0, victim, TO_ROOM, 0);
      }
      else {
        if(!can_attack(ch) || !can_be_attacked(ch, victim))
          return eFAILURE;
	  
        if(IS_NPC(victim))
          return eFAILURE;
	      
        if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == victim)) {
          act("$N is just such a good friend, you simply can't murder $M.",
	      ch, 0, victim, TO_CHAR, 0);
          return eFAILURE;
        }
	      
        if ((GET_POS(ch)==POSITION_STANDING) &&
	    (victim != ch->fighting))
	{
          WAIT_STATE(ch, PULSE_VIOLENCE+2); /* HVORFOR DET?? */
          return attack(ch, victim, TYPE_UNDEFINED);
        }
	else
          send_to_char("You do the best you can!\n\r",ch);
      }
    }
    else
      send_to_char("They aren't here.\n\r", ch);
  }
  else
      send_to_char("Hit whom?\n\r", ch);
  return eSUCCESS;
}



int do_slay(struct char_data *ch, char *argument, int cmd)
{
  char buf[256];
  char arg[MAX_STRING_LENGTH];
  struct char_data *victim;
  
  one_argument(argument, arg);
  
  if(!*arg) {
    send_to_char("Slay whom?\n\r", ch);
    return eFAILURE;
  }
  
  if(!(victim = get_char_room_vis(ch, arg))) {
    send_to_char("They aren't here.\n\r", ch);
    return eFAILURE;
  }
  

//  if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && GET_CLASS(ch->master) == CLASS_ANTI_PAL && !IS_NPC(victim)) {
//     act("I can't attack $N master!", ch->master, 0, victim, TO_CHAR, 0);
//     return eFAILURE;
//  }

  if (IS_AFFECTED(ch, AFF_FAMILIAR) && !IS_NPC(ch->master)) {
     act("But $N scares me!!", ch->master, 0, victim, TO_CHAR, 0);
     return eFAILURE;
  }

  if (ch == victim)
      send_to_char("Your mother would be so sad.. :(\n\r", ch);
    else {
      if (GET_LEVEL(victim) >= IMP) {
        send_to_char("You no make ME into chop suey!\r\n", ch);
        sprintf(buf,"%s just tried to kill you.\r\n", GET_NAME(ch));
        send_to_char(buf, victim);
        if(GET_LEVEL(ch) > IMMORTAL) {
	  fight_kill(victim, ch, TYPE_RAW_KILL, 0);
	  send_to_char("Lunch.\r\n", ch);
	}
        return eSUCCESS|eCH_DIED;
      }      
     
      act("You chop $M to pieces! Ah! The blood!",
	  ch, 0, victim, TO_CHAR, 0);
      act("$N chops you to pieces!", victim, 0, ch, TO_CHAR, 0);
      act("$n brutally slays $N.", ch, 0, victim, TO_ROOM, NOTVICT);
      fight_kill(ch, victim, TYPE_RAW_KILL, 0);
      return eSUCCESS|eVICT_DIED;
    }
  
  return eSUCCESS; // shouldn't get here
}



int do_kill(struct char_data *ch, char *argument, int cmd)
{
  char buf[256];
  char arg[MAX_STRING_LENGTH];
  struct char_data *victim;
  
  one_argument(argument, arg);
  
  if(!*arg) {
    send_to_char("Slay whom?\n\r", ch);
    return eFAILURE;
  }
  
  if(!(victim = get_char_room_vis(ch, arg))) {
    send_to_char("They aren't here.\n\r", ch);
    return eFAILURE;
  }
  

//  if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && GET_CLASS(ch->master) == CLASS_ANTI_PAL && !IS_NPC(victim)) {
//     act("I can't attack $N master!", ch->master, 0, victim, TO_CHAR, 0);
//     return eFAILURE;
//  }

  if (IS_AFFECTED(ch, AFF_FAMILIAR) && !IS_NPC(ch->master)) {
     act("But $N scares me!!", ch->master, 0, victim, TO_CHAR, 0);
     return eFAILURE;
  }

  if((GET_LEVEL(ch) < G_POWER) || IS_NPC(ch)) {
    if(!can_attack(ch) || !can_be_attacked(ch, victim))
      return eFAILURE;
    return do_hit(ch, argument, 0);
  }
  else {
    if (ch == victim)
      send_to_char("Your mother would be so sad.. :(\n\r", ch);
    else {
      if (GET_LEVEL(victim) >= IMP) {
        send_to_char("You no make ME into chop suey!\r\n", ch);
        sprintf(buf,"%s just tried to kill you.\r\n", GET_NAME(ch));
        send_to_char(buf, victim);
        if(GET_LEVEL(ch) > IMMORTAL) {
	  fight_kill(victim, ch, TYPE_CHOOSE, 0);
	  send_to_char("Lunch.\r\n", ch);
	}
        return eSUCCESS|eCH_DIED;
      }      
      act("You chop $M to pieces! Ah! The blood!",
	  ch, 0, victim, TO_CHAR, 0);
      act("$N chops you to pieces!", victim, 0, ch, TO_CHAR, 0);
      act("$n brutally slays $N.", ch, 0, victim, TO_ROOM, NOTVICT);
      fight_kill(ch, victim, TYPE_CHOOSE, 0);
      return eSUCCESS|eVICT_DIED;
    }
  }
  return eSUCCESS; // shouldn't get here
}

/****************** JOIN IN THE FIGHT *******************************/

// if you send do_join a number as the argument, ch will attempt to
// join a mob of that number.  Only works if ch is mob.
//
int do_join(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim, *tmp_ch, *next_char, *k;
  struct follow_type *j;
  extern struct char_data *combat_list;
  int count = 0;
  char victim_name[240];
  bool found = FALSE;

  one_argument(argument, victim_name);

  if(ch->fighting) {
    send_to_char(" Aren't you helping enough as it is?\n\r", ch);
    return eFAILURE;
  }
  char tmp[MAX_STRING_LENGTH];
  int num;
	if (IS_MOB(ch) && isdigit(*victim_name) && !(sscanf(victim_name, "%d.%s", &num, tmp) == 2)) {
    count = atoi(victim_name);
    victim = world[ch->in_room].people;
    for(; victim; victim = victim->next_in_room)
      if(IS_MOB(victim) && mob_index[victim->mobdata->nr].virt == count)
        break;
    if(!victim)
       return eFAILURE;
    count = 0;
  }
  else if(!(victim = get_char_room_vis(ch, victim_name)) && str_cmp("follower", victim_name)) { 
    send_to_char("Join whom?\n\r", ch);
    return eFAILURE;
  }

  if(!victim) {
     if(ch->followers)
        for (j = ch->followers; j; j = j->next) {
           if (ch->in_room == j->follower->in_room && j->follower->fighting)
              if (IS_AFFECTED(j->follower, AFF_CHARM)) {
                 found = TRUE;
                 victim = j->follower;
              }
	}

     if (!found) {
        send_to_char("You have no loyal subjects engaged in combat!\n\r", ch);
        return eFAILURE;
     }
  }


  if (!victim->fighting)
  {
    send_to_char("But they're not fighting anyone.\r\n", ch);
    return eFAILURE;
  }

  if(victim == ch) { 
    send_to_char("Don't be a dork.\n\r", ch);
    return eFAILURE;
  }
  if(victim->fighting == ch) {
    send_to_char( "But why join someone who is trying to kill YOU?\n\r",ch);
    return eFAILURE;
  }


 // if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && GET_CLASS(ch->master) == CLASS_ANTI_PAL && !IS_NPC(victim->fighting)) {
 //    act("I can't join the attack against $N master!", ch->master, 0, victim->fighting, TO_CHAR, 0);
 //    return eFAILURE;
 // }

  tmp_ch = victim->fighting;

  if(!tmp_ch) { 
    act("But $N is not fighting!!!", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if(!can_attack(ch) || (victim->fighting &&
   !can_be_attacked(ch, victim->fighting)))
   {
     return eFAILURE;
   }


  for(k = combat_list; k; k = next_char) {
     next_char = k->next_fighting;
     if(k->fighting == tmp_ch)
       count++;
  } 
/*
  if(count >= 6) {
    send_to_char("You can't get close enough to do anything.", ch);
    return eFAILURE;
  }
*/
  send_to_char("ARGGGGG!!!! *** K I L L ***!!!!.\n\r", ch);
  act("$N joins you in the fight!", victim, 0, ch, TO_CHAR, 0);
  act("$n has joined $N in the battle.", ch, 0, victim, TO_ROOM, NOTVICT);

  return attack(ch, tmp_ch, TYPE_UNDEFINED);
}
