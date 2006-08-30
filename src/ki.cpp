/*
 * ki.c - implementation of ki usage
 * Morcallen 12/18
 *
 */
/* $Id: ki.cpp,v 1.54 2006/08/30 17:39:25 jhhudso Exp $ */

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <ki.h>
#include <room.h>
#include <character.h>
#include <spells.h> // tar_char..
#include <levels.h>
#include <utility.h>
#include <player.h>
#include <interp.h>
#include <mobile.h>
#include <fight.h>
#include <handler.h>
#include <connect.h>
#include <act.h>
#include <db.h>
#include <magic.h> // dispel_magic
#include <returnvals.h>

extern CWorld world;
 
extern CHAR_DATA *character_list;
extern int hit_gain(CHAR_DATA *, int);

void remove_memory(CHAR_DATA *ch, char type);
void add_memory(CHAR_DATA *ch, char *victim, char type);

struct ki_info_type ki_info [ ] = {
{ /* 0 */
	12, POSITION_FIGHTING, 12,
	TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, ki_blast,
	SKILL_INCREASE_HARD
},

{ /* 1 */
	12, POSITION_FIGHTING, 15,
	TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, ki_punch,
	SKILL_INCREASE_HARD
},

{ /* 2 */
	12, POSITION_STANDING, 5,
	TAR_IGNORE|TAR_CHAR_ROOM|TAR_SELF_ONLY, ki_sense,
	SKILL_INCREASE_MEDIUM
},

{ /* 3 */
	12, POSITION_FIGHTING, 8,
	TAR_IGNORE, ki_storm, SKILL_INCREASE_HARD
},

{ /* 4 */
        12, POSITION_STANDING, 25,
        TAR_IGNORE|TAR_CHAR_ROOM|TAR_SELF_ONLY, ki_speed,
        SKILL_INCREASE_HARD
},

{ /* 5 */
        12, POSITION_RESTING, 8,
        TAR_IGNORE|TAR_CHAR_ROOM|TAR_SELF_ONLY, ki_purify,
	SKILL_INCREASE_MEDIUM
},

{ /* 6 */
	12, POSITION_FIGHTING, 10,
        TAR_CHAR_ROOM|TAR_FIGHT_VICT, ki_disrupt,
	SKILL_INCREASE_HARD
},

{ /* 7 */
	12, POSITION_FIGHTING, 12,
        TAR_IGNORE, ki_stance, SKILL_INCREASE_EASY
},

{ /* 8 */
	12, POSITION_FIGHTING, 20,
        TAR_IGNORE, ki_agility, SKILL_INCREASE_MEDIUM
},

{ /* 9 */
	12, POSITION_RESTING, 15,
        TAR_IGNORE, ki_meditation, SKILL_INCREASE_HARD
}

};

char *ki[] = {
	"blast",
	"punch",
	"sense",
	"storm",
        "speed",
        "purify",
        "disrupt",
        "stance",
        "agility",
        "meditation",
	"\n"
};
void update_pos(CHAR_DATA *victim);
int16 use_ki(CHAR_DATA *ch, int kn);
bool ARE_GROUPED(CHAR_DATA *sub, CHAR_DATA *obj);

int16 use_ki(CHAR_DATA *ch, int kn)
{
	return(ki_info[kn].min_useski);
}


int do_ki(CHAR_DATA *ch, char *argument, int cmd)
{
  CHAR_DATA *tar_char = ch;
  char name[MAX_STRING_LENGTH];
  int qend, spl = -1;
  bool target_ok;
  int learned;

   if (GET_LEVEL(ch) < ARCHANGEL && GET_CLASS(ch) != CLASS_MONK) {
      send_to_char("You are unable to control your ki in this way!\n\r", ch);
      return eFAILURE;
      }
/*
   if ((IS_SET(world[ch->in_room].room_flags, SAFE)) && (GET_LEVEL(ch) < IMP)) {
      send_to_char("You feel at peace, calm, relaxed, one with yourself and "
                   "the universe.\n\r", ch);
      return eFAILURE;
      }*/

  argument = skip_spaces(argument);

  if(!(*argument)) {
    send_to_char("Yes, but WHAT would you like to do?\n\r", ch);
    return eFAILURE;
  }

  for(qend = 1; *(argument + qend) && (*(argument + qend) != ' ') ; qend++)
    *(argument+qend) = LOWER(*(argument + qend));

  spl = old_search_block(argument, 0, qend, ki, 0);
  spl--;	 /* ki goes from 0+ not 1+ like spells */
  
  if(spl < 0) {
    send_to_char("You cannot harness that energy!\n\r", ch);
    return eFAILURE;
  }
  
   if (((IS_SET(world[ch->in_room].room_flags, SAFE)) && (GET_LEVEL(ch) < IMP)) 
&& spl != 2 && spl!=4 && spl!=5 && spl!=7 && spl!=8 && spl!=9) {
      send_to_char("You feel at peace, calm, relaxed, one with yourself and "
                   "the universe.\n\r", ch);
      return eFAILURE;
      }


  learned = has_skill(ch, (spl+KI_OFFSET));
  if(!learned)
  {
     send_to_char("You do not know that ki power!\r\n", ch);
     return eFAILURE;
  }

  if(ki_info[spl].ki_pointer) {
    if(GET_POS(ch) < ki_info[spl].minimum_position || (spl==KI_MEDITATION && (GET_POS(ch) == POSITION_FIGHTING || GET_POS(ch) <= POSITION_SLEEPING))) {
      switch(GET_POS(ch)) {
        case POSITION_SLEEPING:
          send_to_char("You dream of wonderful ki powers.\n\r", ch);
          break;
        case POSITION_RESTING:
          send_to_char("You cannot harness that much energy while "
	               "resting!\n\r", ch);
          break;
        case POSITION_SITTING:
          send_to_char("You can't do this sitting!\n\r", ch);
          break;
        case POSITION_FIGHTING:
          send_to_char("This is a peaceful ki power.\n\r", ch);
          break;
        default:
          send_to_char("It seems like you're in a pretty bad shape!\n\r", ch);
          break;
      }
      return eFAILURE;
    }
    argument += qend; /* Point to the space after the last ' */
    for(; *argument == ' '; argument++); /* skip spaces */

    /* Locate targets */
    target_ok = FALSE;
	
    if(!IS_SET(ki_info[spl].targets, TAR_IGNORE)) {
      argument = one_argument(argument, name);
      if(*name) {
        if(IS_SET(ki_info[spl].targets, TAR_CHAR_ROOM))
          if((tar_char = get_char_room_vis(ch, name)) != NULL)
            target_ok = TRUE;
        
	if(!target_ok && IS_SET(ki_info[spl].targets, TAR_SELF_ONLY))
          if(str_cmp(GET_NAME(ch), name) == 0) {
            tar_char = ch;
            target_ok = TRUE;
          } // of !target_ok
      } // of *name
      
      /* No argument was typed */
      else if(!*name) {	
        if(IS_SET(ki_info[spl].targets, TAR_FIGHT_VICT))
          if(ch->fighting)
            if((ch->fighting)->in_room == ch->in_room) {
              tar_char = ch->fighting;
              target_ok = TRUE;
            } 
            if(!target_ok && IS_SET(ki_info[spl].targets, TAR_SELF_ONLY)) {
              tar_char = ch;
              target_ok = TRUE;
            }
      } // of !*name
      
      else
        target_ok = FALSE;
    }
    
    if(IS_SET(ki_info[spl].targets, TAR_IGNORE))
      target_ok = TRUE;
	
    if(target_ok != TRUE) {
      if(*name)
        send_to_char("Nobody here by that name.\n\r", ch);
      else /* No arguments were given */
        send_to_char("Whom should the power be used upon?\n\r", ch);
      return eFAILURE;
    }
    
    
    else if(target_ok) {
      if((tar_char == ch) && IS_SET(ki_info[spl].targets, TAR_SELF_NONO)) {
        send_to_char("You cannot use this power on yourself.\n\r", ch);
        return eFAILURE;
      }
      else if((tar_char != ch) &&
              IS_SET(ki_info[spl].targets, TAR_SELF_ONLY)) {
        send_to_char("You can only use this power upon yourself.\n\r", ch);
        return eFAILURE;
      }
      else if(IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tar_char)) {
        send_to_char("You are afraid that it might harm your master.\n\r", ch);
        return eFAILURE;
      }
    }

    /* I put ths in to stop those crashes.  Morc: find your own bug ;)
     * -Sadus
     * This has hence been fixed. - Pir
     */
    if(!IS_SET(ki_info[spl].targets, TAR_IGNORE)) 
    if(!tar_char) {
      log("Dammit Morc, fix that null tar_char thing in ki", IMP, LOG_BUG);
      send_to_char("If you triggered this message, you almost crashed the\n\r"
                   "game.  Tell a god what you did immediately.\n\r", ch);
      return eFAILURE;
    }

    /* crasher right here */
    if (IS_SET(world[ch->in_room].room_flags, NO_KI)) {
      send_to_char("You find yourself unable to focus your energy here.\n\r", ch);
      return eFAILURE;
    }
    
    if(!IS_SET(ki_info[spl].targets, TAR_IGNORE)) 
      if(!can_attack(ch) || !can_be_attacked(ch, tar_char))
        return eFAILURE;

    if(GET_LEVEL(ch) < ARCHANGEL && GET_KI(ch) < use_ki(ch, spl)) {
      send_to_char("You do not have enough ki!\n\r", ch);
      return eFAILURE;
    }

    WAIT_STATE(ch, ki_info[spl].beats);

    if((ki_info[spl].ki_pointer == NULL) && spl > 0)
      send_to_char("Sorry, this power has not yet been implemented.\n\r", ch);
    else {
      if(!skill_success(ch, tar_char, spl+KI_OFFSET)) {
        send_to_char("You lost your concentration!\n\r", ch);
        GET_KI(ch) -= use_ki(ch, spl)/2;
        return eSUCCESS;
      }

      if(!IS_SET(ki_info[spl].targets, TAR_IGNORE)) 
      if(!tar_char || (ch->in_room != tar_char->in_room)) {
        send_to_char("Whom should the power be used upon?\n\r", ch);
        return eFAILURE;
      }

      /* Stop abusing your betters  */
     if(!IS_SET(ki_info[spl].targets, TAR_IGNORE)) 
     if (!IS_NPC(tar_char) && (GET_LEVEL(ch) > ARCHANGEL) 
          && (GET_LEVEL(tar_char) > GET_LEVEL(ch)))
      {
        send_to_char("That just might annoy them!\n\r", ch);
        return eFAILURE;
      }

      /* Imps ignore safe flags  */
     if(!IS_SET(ki_info[spl].targets, TAR_IGNORE)) 
     if (IS_SET(world[ch->in_room].room_flags, SAFE) && !IS_NPC(ch) 
          && (GET_LEVEL(ch) == IMP)) {
       send_to_char("There is no safe haven from an angry IMP!\n\r", tar_char);
     }

      send_to_char("Ok.\n\r", ch);
      GET_KI(ch) -= use_ki(ch, spl);

      return ((*ki_info[spl].ki_pointer) (GET_LEVEL(ch), ch, argument, tar_char));
    }
    return eFAILURE;
  }
  return eFAILURE;
}

/* Now some ki maintenance procedures */
/* This procedure takes the character and returns the type */
/* of effect that happens.  Most of the time it will be no */
/* effect, but ths varies greatly with amount of ki	   */

int ki_check(CHAR_DATA *ch)
{
	int percent = 0;
	if(GET_KI(ch) < MIN_REACT_KI)
		return NO_EFFECT;

	if(GET_KI(ch) >= MAXIMUM_KI)
	{
		if(rand()%2) /* 50% of the time */
		{
			switch(rand()%6)
			{
				case 0: return NO_EFFECT;
				case 1: return NO_EFFECT;
				case 2: return DIVINE;
				case 3: return MIRACLE;
				case 4: return MAJOR_EFFECT;
				case 5: return MINOR_EFFECT;
			}
		}
	}	
	/* Still here?  You can have an effect  - randomly */
	if(GET_LEVEL(ch) >= IMMORTAL)
		return NO_EFFECT;	/* gods don't need effects */
	if(GET_LEVEL(ch) > (MAX_MORTAL - 1))
		percent += 10;
	else if(GET_LEVEL(ch) > 35)
	    percent += GET_LEVEL(ch) / 5;		
	else if(GET_LEVEL(ch) > 15)
		percent += GET_LEVEL(ch) / 10;
	/* No bonus under 15th level */
	if(GET_CLASS(ch) == CLASS_MONK)
		percent += GET_LEVEL(ch) / 4;
	percent += GET_KI(ch) / 2;	/* Add half their ki to it */
	percent += dice(1, 50);
	if(percent > 100) percent = 100;

	if(percent > 95)	/* wow! */
		return DIVINE;
	else if(percent > 85)
		return MIRACLE;
	else if(percent > 80)
		return MAJOR_EFFECT;
	else if(percent > 75)
		return MINOR_EFFECT;
	else
		return NO_EFFECT;	
}

void reduce_ki(CHAR_DATA *ch, int type)
{
	int amount = 0;

	amount += GET_LEVEL(ch)/type; /* the higher the response
									* the lower the divisor */

	amount -= dice(1, 10);
	if(amount < 0) amount = 0;
	GET_KI(ch) -= amount;

}

int ki_gain(CHAR_DATA *ch)
{
	int gain;

        /* gain 1 - 7 depedant on level */
        gain = GET_CLASS(ch) == CLASS_MONK?(int)(ch->max_ki * 0.04):(int)(ch->max_ki * 0.05);/*(GET_LEVEL(ch) / 8) + 1;*/
        gain += ch->ki_regen;
	if (GET_CLASS(ch) == CLASS_MONK) gain += wis_app[GET_WIS(ch)].ki_regen;
        else if (GET_CLASS(ch) == CLASS_BARD) gain += int_app[GET_INT(ch)].ki_regen;
        gain += age(ch).year / 25;
	if ( IS_SET(world[ch->in_room].room_flags, SAFE))
		gain = (int)(gain * 1.25);
	return MAX(gain, 1);
}

int ki_blast( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{
   int success = 0;
   int exit = number(0, 5); /* Chooses an exit */

   extern char * dirswards[];
   char buf[200];

   if (!vict) {
      log("Serious problem in ki blast!", ANGEL, LOG_BUG);
      return eINTERNAL_ERROR;
      }

   success += GET_LEVEL(ch);

   if (vict->weight < 50)
      success += 50;
   else if (vict->weight < 120)
      success += 20;
   else if (vict->weight < 200)
      ; /* No effect */
   else if (vict->weight < 255)
      success -= 10;
   else
      success -= 20; /* more than 300 pounds?! */

   if (number(1, 101) > success) /* 101 is complete failure */
      {
      act("$n fails to blast $N!", ch, 0, vict, TO_ROOM, NOTVICT);
      act("You fail to blast $N!", ch, 0, vict, TO_CHAR, 0);
      act("$n finds that you are hard to blast!", ch, 0, vict, TO_VICT, 0);
      if (!vict->fighting && IS_NPC(vict))
         return attack(vict, ch, TYPE_UNDEFINED);
      }

   if (CAN_GO(vict, exit) &&
       !IS_SET(world[EXIT(vict, exit)->to_room].room_flags, IMP_ONLY) &&
       !IS_SET(world[EXIT(vict, exit)->to_room].room_flags, NO_TRACK)) {
      sprintf(buf, "$N is blasted out of the room %s by $n!", dirswards[exit]);
      act(buf, ch, 0, vict, TO_ROOM, NOTVICT);
      sprintf(buf, "You watch as $N goes flailing out of the room %s!", dirswards[exit]);
      act(buf, ch, 0, vict, TO_CHAR, 0);
      act("$n's vicious blast throws you out of the room!", ch, 0,
         vict, TO_VICT, 0);
	 
      if (vict->fighting) {
         if (IS_NPC(vict)) {
            add_memory(vict, GET_NAME(ch), 'h');
            remove_memory(vict, 'f');
            }
//         if (IS_NPC(ch)) /* This shouldn't ever happen */
  //          add_memory(ch, GET_NAME(vict), 't');
         if (ch->fighting == vict)
            stop_fighting(ch);
         }
      move_char(vict, (world[(ch)->in_room].dir_option[exit])->to_room);
      GET_POS(vict) = POSITION_SITTING;
      return eSUCCESS;
      }
   else /* There is no exit there */
	{
		act("$N is blasted across the room by $n!", ch, 0, vict,
		  TO_ROOM, NOTVICT);
		act("$N is thrown to the ground by your blast!", ch, 0, vict,
		  TO_CHAR, 0);
		act("$n blasts you across the room!", ch, 0, vict, TO_VICT, 0);
		GET_HIT(vict) -= number(1,4) * GET_LEVEL(ch);
		damage(ch,vict,100, TYPE_KI,KI_OFFSET+KI_BLAST,0);
		if(!vict->fighting && IS_NPC(vict))
			return attack(vict, ch, TYPE_UNDEFINED);
		return 1;
	}
	/* still here?  It was unsuccessful */
	return eSUCCESS;
}

int ki_punch( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{	
   if (!vict) {
      log("Serious problem in ki punch!", ANGEL, LOG_BUG);
      return eINTERNAL_ERROR;
      }

   set_cantquit(ch, vict);
   int dam = GET_HIT(vict) / 4, manadam = GET_MANA(vict) / 4;

   dam = MAX(350, dam);
   dam = MIN(1000,dam);
   manadam = MAX(150, manadam);
   manadam = MIN(750, manadam);
   if (GET_HIT(vict) < 500000) {
	      if (number(1, 101) <
           GET_LEVEL(ch)/5 + has_skill(ch, KI_OFFSET+KI_PUNCH)/2 - GET_LEVEL(vict)/5)

      {
//         act("$n punches $s fist RIGHT INTO $N!", ch, 0, vict,
//             TO_ROOM, NOTVICT);
//         act("The blood gushes as you put your fist into $N's chest!",
//             ch, 0, vict, TO_CHAR, 0);
//         act("$n shoves $s hand into your chest, and your lifeblood "
//	     "ebbs away.", ch, 0, vict, TO_VICT, INVIS_VISIBLE);
 	 GET_MANA(vict) -= manadam;
	 int retval = damage(ch,vict,dam, TYPE_UNDEFINED, KI_OFFSET+KI_PUNCH,0);
//         group_gain(ch, vict);
//         fight_kill(ch, vict, TYPE_CHOOSE, 0);
//         GET_HIT(ch) -= 1/10 * (GET_MAX_HIT(ch));
//         if (GET_HIT(ch) <= 0)
//	    GET_HIT(ch) = 1;
 //        return eSUCCESS|eVICT_DIED;
	return retval;
      }
      else {
//         act("$N narrowly avoids $n's deadly thrust!", ch, 0, vict,
//		TO_ROOM, NOTVICT);
//         act("$N dodges out of your way, barely saving $S life!", 
//		  ch, 0, vict, TO_CHAR, 0);
//         act("$n makes a killer move, which you luckily dodge.", 
//		  ch, 0, vict, TO_VICT, INVIS_VISIBLE);
         GET_HIT(ch) -= 1/8 * (GET_MAX_HIT(ch));
	 WAIT_STATE(ch, PULSE_VIOLENCE);
         if (!vict->fighting)
            return attack(vict, ch, TYPE_UNDEFINED);
         }	
      } // end of < 5000
      
   else {
      send_to_char("Your opponent has too many hit points!\n\r", ch);
      if (!vict->fighting)
         return attack(vict, ch, TYPE_UNDEFINED);
      }
      
   return eSUCCESS; // shouldn't get here
}

int ki_sense( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{
	struct affected_type af;
	if(IS_AFFECTED(ch, AFF_INFRARED))
		return eSUCCESS;
	if(affected_by_spell(ch, SPELL_INFRAVISION))
		return eSUCCESS;

	af.type = SPELL_INFRAVISION;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.duration = level;
	af.bitvector = AFF_INFRARED;
	affect_to_char(vict, &af);
	send_to_char("You feel your sense become more acute.\n\r", vict);

	return eSUCCESS;
}

int ki_storm( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{
  int dam;
  int retval;
  CHAR_DATA *tmp_victim, *temp;

  dam = number(135,165);
//  send_to_char("Your wholeness of spirit purges the souls of those around you!\n\r", ch);
//  act("$n's eyes flash as $e pools the energy within $m!\n\rA burst of energy slams into you!\r\n",
  int32 room = ch->in_room;
  for(tmp_victim = world[ch->in_room].people; tmp_victim; tmp_victim = temp)
  {
	 temp = tmp_victim->next_in_room;
	 if ( (ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
	      (!ARE_GROUPED(ch,tmp_victim)) && can_be_attacked(ch, tmp_victim)) 
         {
                  retval = damage(ch, tmp_victim, dam, TYPE_KI,
		              KI_OFFSET+KI_STORM, 0);
                  if(IS_SET(retval, eCH_DIED))
                    return retval;
                  act("A burst of energy slams into you!", ch, 0, 0, TO_ROOM, 0);
	 } //else
//		if (world[ch->in_room].zone == world[tmp_victim->in_room].zone)
//	send_to_char("A crackle of energy echos past you.\n\r", tmp_victim);
  }
  int dir = number(0,5), distance = number(1,3),i;
  if (room > 0)
  for (i=0; i < distance;i++)
  {
     if (!IS_EXIT(room, dir) || !IS_OPEN(room, dir))
       break;
     room = EXIT_TO(room, dir);
     if (room < 0) break;
     for (tmp_victim = world[room].people;tmp_victim;tmp_victim = tmp_victim->next_in_room)
        send_to_char("A crackle of energy echoes past you.\r\n",tmp_victim);
  }
  if(number(1,4) == 4 && !ch->fighting) {
    char dammsg[MAX_STRING_LENGTH];
    sprintf(dammsg, "$B%d$R", dam);
    if (dam + GET_HIT(ch) > GET_MAX_HIT(ch)) dam = GET_MAX_HIT(ch) - GET_HIT(ch);
    GET_HIT(ch) += dam;
    send_damage("The flash of energy surges within you for | life!", ch, 0, 0, dammsg, "The flash of energy surges within you!", TO_CHAR);
  }
  WAIT_STATE(ch, PULSE_VIOLENCE);
  return eSUCCESS;
}

int ki_speed( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{
  struct affected_type af;

  if(!vict)
  { 
    log("Null victim sent to ki speed", ANGEL, LOG_BUG);     
    return eINTERNAL_ERROR;
  }

  if ( affected_by_spell(vict, SPELL_HASTE) )
         return eSUCCESS;
   
  af.type      = SPELL_HASTE;
  af.duration  = 2;
  af.modifier  = 0; 
  af.location  = APPLY_NONE;  
  af.bitvector = AFF_HASTE;
 
  affect_to_char(vict, &af);
  send_to_char("You feel a quickening in your limbs!\n\r", vict); 
  return eSUCCESS;
}

int ki_purify( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{
  if(!vict) {
    log("Null victim sent to ki purify", ANGEL, LOG_BUG); 
    return eINTERNAL_ERROR;
  }
  if (!arg)
  {
    send_to_char("You can only purify poison, blindness, alcohol or weaken.",ch);
	return eFAILURE;
  }
  if (!str_cmp(arg, "poison"))
  {
     if(affected_by_spell(vict,SPELL_POISON))
       affect_from_char(vict,SPELL_POISON);
     else {
	send_to_char("That taint is not present.\r\n",ch);
	return eFAILURE;
     }
     send_to_char("You purge the poison.\r\n",ch);
  } else if (!str_cmp(arg, "blindness")) {
   if(affected_by_spell(vict,SPELL_BLINDNESS))
      affect_from_char(vict,SPELL_BLINDNESS);
     else {
	send_to_char("That taint is not present.\r\n",ch);
	return eFAILURE;
     }
     send_to_char("You purge the blindness.\r\n",ch);

  } else if (!str_cmp(arg, "weaken"))
  {
    if(affected_by_spell(vict,SPELL_WEAKEN))
      affect_from_char(vict,SPELL_WEAKEN);
     else {
	send_to_char("That taint is not present.\r\n",ch);
	return eFAILURE;
     }
     send_to_char("You purge the poison.\r\n",ch);
  } else if (!str_cmp(arg, "alcohol")) {
    if(GET_COND(ch, DRUNK) > 0)
      gain_condition(vict, DRUNK, -GET_COND(ch, DRUNK));
    else {
	send_to_char("That taint is not present.\r\n",ch);
	return eFAILURE;
    }
    send_to_char("You purge the alcohol.\r\n",ch);
  } else {
   send_to_char("You cannot purge that.\r\n",ch);
  }
  return eSUCCESS;
}

int ki_disrupt( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{
   if (!vict) {
      log("Serious problem in ki disrupt!", ANGEL, LOG_BUG);
      return eINTERNAL_ERROR;
      }

   act("$n slams a bolt of focused ki energy into the flow of magic around you!", 
       ch, 0, vict, TO_VICT, 0);
   act("$n focuses a blast of ki to disrupt the flow of magic around $N!",
       ch, 0, vict, TO_ROOM, 0);
   send_to_char("You focus your ki to disrupt the flow of magic around your opponent!\r\n", ch);

   WAIT_STATE(ch, PULSE_VIOLENCE);
   
   return spell_dispel_magic(GET_LEVEL(ch)+1, ch, vict, 0, 0);
}

int ki_stance( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{
   struct affected_type af;

   if(affected_by_spell(ch, KI_STANCE+KI_OFFSET)) {
      send_to_char("You focus your ki to harden your stance, but your body is still recovering from last time...\r\n", ch);
      return eFAILURE;
   }

   act("$n assumes a defensive stance and attempts to absorb the energies that surround $m.",
       ch, 0, vict, TO_ROOM, 0);
   send_to_char("You take a defensive stance and try to aborb the energies seeking to harm you.\r\n", ch);

   // chance of failure - can be meta'd past that point though
   if(number(1, 100) > ( GET_DEX(ch) * 4 ) )
      return eSUCCESS;

   SET_BIT(ch->combat, COMBAT_MONK_STANCE);

   af.type      = KI_STANCE + KI_OFFSET;
   af.duration  = 50 - ( ( GET_LEVEL(ch) / 5 ) * 2 );
   af.modifier  = 1;
   af.location  = APPLY_NONE;
   af.bitvector = -1;
 
   affect_to_char(ch, &af);
   return eSUCCESS;
}

int ki_agility( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{
  int learned, chance, specialization, percent;
  struct affected_type af;
   
  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
    learned = 75;
  else if(!(learned = has_skill(ch, KI_AGILITY+KI_OFFSET))) {
    send_to_char("You aren't experienced enough to teach others graceful movement.\r\n", ch);
    return eFAILURE;
  }

  if(!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("You have no group to instruct.\r\n", ch);
    return eFAILURE;
  }

  specialization = learned / 100;
  learned = learned % 100;

  chance = 75;

  // 101% is a complete failure
  percent = number(1, 101);
  if (percent > chance) {
     send_to_char("Hopefully none of them noticed you trip on that rock.\r\n", ch);
     act ("$n tries to show everyone how to be graceful and trips over a rock.", ch, 0, 0, TO_ROOM, 0);
  } 
  else {
    send_to_char("You instruct your party on more graceful movement.\r\n", ch);
    act("$n holds a quick tai chi class.", ch, 0, 0, TO_ROOM, 0);
   
    for(char_data * tmp_char = world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
    {
      if(tmp_char == ch)
        continue;
      if(!ARE_GROUPED(ch, tmp_char))
        continue;
      affect_from_char(tmp_char, KI_AGILITY+KI_OFFSET);
      affect_from_char(tmp_char, KI_AGILITY+KI_OFFSET);
      act ("$n's graceful movement inspires you to better form.", ch, 0, tmp_char, TO_VICT, 0);
   
      af.type      = KI_AGILITY+KI_OFFSET;
      af.duration  = 1 + learned / 10;
      af.modifier  = 1;
      af.location  = APPLY_MOVE_REGEN;
      af.bitvector = -1;
      affect_to_char(tmp_char, &af);
      af.modifier  = -20;
      af.location  = APPLY_ARMOR;
      affect_to_char(tmp_char, &af);
    }
  }
  
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
  return eSUCCESS;  
}  

int ki_meditation( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{
   int gain;

   if(IS_NPC(ch)) return eFAILURE;

   act("You enter a brief meditative state and focus your ki to heal your injuries.",ch, 0, vict, TO_CHAR, 0);
   act("$n enters a brief meditative state and focuses $s ki to heal several wounds.",ch, 0, vict, TO_ROOM, 0);

   gain = hit_gain(ch, POSITION_SLEEPING);

   GET_HIT(ch)  = MIN(GET_HIT(ch) + gain,  hit_limit(ch));

   return eSUCCESS;
}
