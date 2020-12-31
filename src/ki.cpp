/*
 * ki.c - implementation of ki usage
 * Morcallen 12/18
 *
 */
/* $Id: ki.cpp,v 1.94 2014/07/04 22:00:04 jhhudso Exp $ */

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "ki.h"
#include "room.h"
#include "character.h"
#include "spells.h" // tar_char..
#include "levels.h"
#include "utility.h"
#include "player.h"
#include "interp.h"
#include "mobile.h"
#include "fight.h"
#include "handler.h"
#include "connect.h"
#include "act.h"
#include "db.h"
#include "returnvals.h"
#include <vector>

using namespace std;

extern CWorld world;
 
extern int hit_gain(CHAR_DATA *, int);

void remove_memory(CHAR_DATA *ch, char type);
void add_memory(CHAR_DATA *ch, char *victim, char type);

struct ki_info_type ki_info [ ] = {
{ /* 0 */
	3*PULSE_TIMER, POSITION_FIGHTING, 12,
	TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, ki_blast,
	SKILL_INCREASE_HARD
},

{ /* 1 */
	3*PULSE_TIMER, POSITION_FIGHTING, 15,
	TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, ki_punch,
	SKILL_INCREASE_HARD
},

{ /* 2 */
        3*PULSE_TIMER, POSITION_STANDING, 5,
	TAR_IGNORE|TAR_CHAR_ROOM|TAR_SELF_ONLY, ki_sense,
	SKILL_INCREASE_MEDIUM
},

{ /* 3 */
	3*PULSE_TIMER, POSITION_FIGHTING, 8,
	TAR_IGNORE, ki_storm, SKILL_INCREASE_HARD
},

{ /* 4 */
        3*PULSE_TIMER, POSITION_STANDING, 25,
        TAR_IGNORE|TAR_CHAR_ROOM|TAR_SELF_ONLY, ki_speed,
        SKILL_INCREASE_HARD
},

{ /* 5 */
        3*PULSE_TIMER, POSITION_RESTING, 8,
        TAR_IGNORE|TAR_CHAR_ROOM|TAR_SELF_ONLY, ki_purify,
	SKILL_INCREASE_MEDIUM
},

{ /* 6 */
	3*PULSE_TIMER, POSITION_FIGHTING, 10,
        TAR_CHAR_ROOM|TAR_FIGHT_VICT, ki_disrupt,
	SKILL_INCREASE_HARD
},

{ /* 7 */
	3*PULSE_TIMER, POSITION_FIGHTING, 12,
        TAR_IGNORE, ki_stance, SKILL_INCREASE_EASY
},

{ /* 8 */
	3*PULSE_TIMER, POSITION_FIGHTING, 20,
        TAR_IGNORE, ki_agility, SKILL_INCREASE_MEDIUM
},

{ /* 9 */
	3*PULSE_TIMER, POSITION_RESTING, 15,
        TAR_IGNORE, ki_meditation, SKILL_INCREASE_HARD
},

{ /* 10 */
	3*PULSE_TIMER, POSITION_STANDING, 1,
        TAR_CHAR_ROOM|TAR_SELF_NONO, ki_transfer, SKILL_INCREASE_HARD
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
	"transfer",
	"\n"
};
int16 use_ki(CHAR_DATA *ch, int kn);
bool ARE_GROUPED(CHAR_DATA *sub, CHAR_DATA *obj);

int16 use_ki(CHAR_DATA *ch, int kn)
{
	return(ki_info[kn].min_useski);
}


int do_ki(CHAR_DATA *ch, char *argument, int cmd) {
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

	if (!(*argument)) {
		send_to_char("Yes, but WHAT would you like to do?\n\r", ch);
		return eFAILURE;
	}

	for (qend = 1; *(argument + qend) && (*(argument + qend) != ' '); qend++)
		*(argument + qend) = LOWER(*(argument + qend));

	spl = old_search_block(argument, 0, qend, ki, 0);
	spl--; /* ki goes from 0+ not 1+ like spells */

	if (spl < 0) {
		send_to_char("You cannot harness that energy!\n\r", ch);
		return eFAILURE;
	}

	if (IS_SET(world[ch->in_room].room_flags, SAFE) && (GET_LEVEL(ch) < IMP)
	&& spl != KI_SENSE
	&& spl!=KI_SPEED
	&& spl!=KI_PURIFY
	&& spl!=KI_STANCE
	&& spl!=KI_AGILITY
	&& spl!=KI_MEDITATION) {
		send_to_char("You feel at peace, calm, relaxed, one with yourself and "
				"the universe.\n\r", ch);
		return eFAILURE;
	}

	learned = has_skill(ch, (spl + KI_OFFSET));
	if (!learned) {
		send_to_char("You do not know that ki power!\r\n", ch);
		return eFAILURE;
	}

	if (ki_info[spl].ki_pointer) {
		if (GET_POS(ch) < ki_info[spl].minimum_position
				|| (spl == KI_MEDITATION
						&& (GET_POS(ch) == POSITION_FIGHTING
								|| GET_POS(ch) <= POSITION_SLEEPING))) {
			switch (GET_POS(ch)) {
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
				send_to_char("It seems like you're in a pretty bad shape!\n\r",
						ch);
				break;
			}
			return eFAILURE;
		}
		argument += qend; /* Point to the space after the last ' */
		for (; *argument == ' '; argument++)
			; /* skip spaces */

		/* Locate targets */
		target_ok = FALSE;

		if (!IS_SET(ki_info[spl].targets, TAR_IGNORE)) {
			argument = one_argument(argument, name);
			if (*name) {
				if (IS_SET(ki_info[spl].targets, TAR_CHAR_ROOM))
					if ((tar_char = get_char_room_vis(ch, name)) != NULL)
						target_ok = TRUE;

				if (!target_ok && IS_SET(ki_info[spl].targets, TAR_SELF_ONLY))
					if (str_cmp(GET_NAME(ch), name) == 0) {
						tar_char = ch;
						target_ok = TRUE;
					} // of !target_ok
			} // of *name

			/* No argument was typed */
			else if (!*name) {
				if (IS_SET(ki_info[spl].targets, TAR_FIGHT_VICT))
					if (ch->fighting)
						if ((ch->fighting)->in_room == ch->in_room) {
							tar_char = ch->fighting;
							target_ok = TRUE;
						}
				if (!target_ok && IS_SET(ki_info[spl].targets, TAR_SELF_ONLY)) {
					tar_char = ch;
					target_ok = TRUE;
				}
			} // of !*name

			else
				target_ok = FALSE;
		}

		if (IS_SET(ki_info[spl].targets, TAR_IGNORE))
			target_ok = TRUE;

		if (target_ok != TRUE) {
			if (*name)
				send_to_char("Nobody here by that name.\n\r", ch);
			else
				/* No arguments were given */
				send_to_char("Whom should the power be used upon?\n\r", ch);
			return eFAILURE;
		}

		else if (target_ok) {
			if ((tar_char == ch) && IS_SET(ki_info[spl].targets, TAR_SELF_NONO)) {
				send_to_char("You cannot use this power on yourself.\n\r", ch);
				return eFAILURE;
			} else if ((tar_char != ch)
					&& IS_SET(ki_info[spl].targets, TAR_SELF_ONLY)) {
				send_to_char("You can only use this power upon yourself.\n\r",
						ch);
				return eFAILURE;
			} else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tar_char)) {
				send_to_char(
						"You are afraid that it might harm your master.\n\r",
						ch);
				return eFAILURE;
			}
		}

		/* I put ths in to stop those crashes.  Morc: find your own bug ;)
		 * -Sadus
		 * This has hence been fixed. - Pir
		 */
		if (!IS_SET(ki_info[spl].targets, TAR_IGNORE))
			if (!tar_char) {
				log("Dammit Morc, fix that null tar_char thing in ki", IMP,
						LOG_BUG);
				send_to_char(
						"If you triggered this message, you almost crashed the\n\r"
								"game.  Tell a god what you did immediately.\n\r",
						ch);
				return eFAILURE;
			}

		/* crasher right here */
		if (IS_SET(world[ch->in_room].room_flags, NO_KI)) {
			send_to_char(
					"You find yourself unable to focus your energy here.\n\r",
					ch);
			return eFAILURE;
		}

		if (!IS_SET(ki_info[spl].targets, TAR_IGNORE))
			if (!can_attack(ch) || !can_be_attacked(ch, tar_char))
				return eFAILURE;

		if (GET_LEVEL(ch) < ARCHANGEL && GET_KI(ch) < use_ki(ch, spl)) {
			send_to_char("You do not have enough ki!\n\r", ch);
			return eFAILURE;
		}

		WAIT_STATE(ch, ki_info[spl].beats);

		if ((ki_info[spl].ki_pointer == NULL) && spl > 0)
			send_to_char("Sorry, this power has not yet been implemented.\n\r",
					ch);
		else {
			if (!skill_success(ch, tar_char,
					spl + KI_OFFSET) && !IS_SET(world[ch->in_room].room_flags, SAFE)) {
				send_to_char("You lost your concentration!\n\r", ch);
				GET_KI(ch) -= use_ki(ch, spl) / 2;
				WAIT_STATE(ch, ki_info[spl].beats / 2);

				return eSUCCESS;
			}

			if (!IS_SET(ki_info[spl].targets, TAR_IGNORE))
				if (!tar_char || (ch->in_room != tar_char->in_room)) {
					send_to_char("Whom should the power be used upon?\n\r", ch);
					return eFAILURE;
				}

			/* Stop abusing your betters  */
			if (!IS_SET(ki_info[spl].targets, TAR_IGNORE))
				if (!IS_NPC(tar_char) && (GET_LEVEL(ch) > ARCHANGEL)
						&& (GET_LEVEL(tar_char) > GET_LEVEL(ch))) {
					send_to_char("That just might annoy them!\n\r", ch);
					return eFAILURE;
				}

			/* Imps ignore safe flags  */
			if (!IS_SET(ki_info[spl].targets, TAR_IGNORE))
				if (IS_SET(world[ch->in_room].room_flags, SAFE) && !IS_NPC(ch)
						&& (GET_LEVEL(ch) == IMP)) {
					send_to_char(
							"There is no safe haven from an angry IMP!\n\r",
							tar_char);
				}

			send_to_char("Ok.\n\r", ch);
			GET_KI(ch) -= use_ki(ch, spl);

			return ((*ki_info[spl].ki_pointer)(GET_LEVEL(ch), ch, argument,
					tar_char));
		}
		return eFAILURE;
	}
	return eFAILURE;
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

	// Normalize these so we dont underun the array below
	int norm_wis = MAX(0, GET_WIS(ch));
        int norm_int = MAX(0, GET_INT(ch));

	if (GET_CLASS(ch) == CLASS_MONK) {
	  gain += wis_app[norm_wis].ki_regen;
	} else if (GET_CLASS(ch) == CLASS_BARD) {
	  gain += int_app[norm_int].ki_regen;
	}

        gain += age(ch).year / 25;

	if ( IS_SET(world[ch->in_room].room_flags, SAFE) || check_make_camp(ch->in_room))
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
   else if (vict->weight >= 50 && vict->weight < 120)
      success += 20;
   else if (vict->weight >= 200 && vict->weight < 255)
      success -= 10;
   else
      success -= 20; /* more than 300 pounds?! */

   if (number(1, 101) > success
       || affected_by_spell(vict, SPELL_IRON_ROOTS))  /* 101 is complete failure */
      {
      act("$n fails to blast $N!", ch, 0, vict, TO_ROOM, NOTVICT);
      act("You fail to blast $N!", ch, 0, vict, TO_CHAR, 0);
      act("$n finds that you are hard to blast!", ch, 0, vict, TO_VICT, 0);
      if (!vict->fighting && IS_NPC(vict))
         return attack(vict, ch, TYPE_UNDEFINED);
      }

   if (CAN_GO(vict, exit) &&
       !IS_SET(world[EXIT(vict, exit)->to_room].room_flags, IMP_ONLY) &&
       !IS_SET(world[EXIT(vict, exit)->to_room].room_flags, NO_TRACK) &&
       (!IS_AFFECTED(vict, AFF_CHAMPION) || champion_can_go(EXIT(vict, exit)->to_room)) &&
       class_can_go(GET_CLASS(vict), EXIT(vict, exit)->to_room))
   {
      sprintf(buf, "$N is blasted out of the room %s by $n!", dirswards[exit]);
      act(buf, ch, 0, vict, TO_ROOM, NOTVICT);
      sprintf(buf, "You watch as $N goes flailing out of the room %s!", dirswards[exit]);
      act(buf, ch, 0, vict, TO_CHAR, 0);
      act("$n's vicious blast throws you out of the room!", ch, 0,
         vict, TO_VICT, 0);

	 
      if (vict->fighting) 
      {
        if (IS_NPC(vict)) 
        {
          add_memory(vict, GET_NAME(ch), 'h');
          remove_memory(vict, 'f');
        }
        CHAR_DATA *tmp;
        for(tmp = world[ch->in_room].people;tmp;tmp = tmp->next_in_room)
          if (tmp->fighting == vict)
            stop_fighting(tmp);
        stop_fighting(vict);
      }

      move_char(vict, (world[(ch)->in_room].dir_option[exit])->to_room);
      GET_POS(vict) = POSITION_SITTING;
      return eSUCCESS;
   }
   else /* There is no exit there */
   {
      char buf[MAX_STRING_LENGTH], name[100];
      int prev = GET_HIT(vict);
      
      strcpy(name, GET_SHORT(vict));
      int retval = damage(ch,vict,100, TYPE_KI,KI_OFFSET+KI_BLAST,0);
      GET_POS(vict) = POSITION_SITTING;
      if(!SOMEONE_DIED(retval)) {
        sprintf(buf, "$B%d$R", prev - GET_HIT(vict));
        send_damage("$N is blasted across the room by $n for | damage!", ch, 0, vict, buf,
		    "$N is blasted across the room by $n!", TO_ROOM);
        send_damage("$N is thrown to the ground by your blast and suffers | damage!", ch, 0, vict, buf,
		    "$N is thrown to the ground by your blast!", TO_CHAR);
        send_damage("$n blasts you across the room, causing you to fall and take | damage!", ch, 0, vict, buf,
                    "$n blasts you across the room, causing you to fall!", TO_VICT);
      } else {
        csendf(ch, "You blast %s to bits!", name);
        sprintf(buf, "$N blasts %s to bits!", name);
        act(buf, ch, 0, 0, TO_ROOM, 0);
      }
      if(!SOMEONE_DIED(retval) && !vict->fighting && IS_NPC(vict))
         return attack(vict, ch, TYPE_UNDEFINED);
      return retval;
   }
   /* still here?  It was unsuccessful */
   return eSUCCESS;
}

int ki_punch( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict)
{	
   if (!vict) {
      logf(ANGEL, LOG_BUG, "Serious problem in ki punch!", ANGEL, LOG_BUG);
      return eINTERNAL_ERROR;
      }

   set_cantquit(ch, vict);
   int dam = GET_HIT(vict) / 4, manadam = GET_MANA(vict) / 4;
   int retval;


   dam = MAX(350, dam);
   dam = MIN(1000,dam);
   manadam = MAX(150, manadam);
   manadam = MIN(750, manadam);
   if (GET_HIT(vict) < 500000) {
	      if (number(1, 101) <
           GET_LEVEL(ch)/5 + has_skill(ch, KI_OFFSET+KI_PUNCH)/2 - GET_LEVEL(vict)/5)

      {
 	 GET_MANA(vict) -= manadam;
	 retval = damage(ch,vict,dam, TYPE_UNDEFINED, KI_OFFSET+KI_PUNCH,0);
	 return retval;
      }
      else {
	 retval = damage(ch,vict,0, TYPE_UNDEFINED, KI_OFFSET+KI_PUNCH,0);
 
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
  for(tmp_victim = world[ch->in_room].people; tmp_victim && tmp_victim != (char_data *)0x95959595; tmp_victim = temp)
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
  af.duration  = has_skill(ch, KI_OFFSET+KI_SPEED)/15;
  af.modifier  = 0; 
  af.location  = APPLY_NONE;  
  af.bitvector = AFF_HASTE;
 
  affect_to_char(vict, &af);

  af.type = SPELL_HASTE;
  af.duration = has_skill(ch, KI_OFFSET+KI_SPEED)/15;
  af.modifier = -has_skill(ch, KI_OFFSET+KI_SPEED)/4;
  af.location = APPLY_AC;
  af.bitvector = -1;

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


int ki_disrupt( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim)
{
   if (!victim) {
      log("Serious problem in ki disrupt!", ANGEL, LOG_BUG);
      return eINTERNAL_ERROR;
   }

   WAIT_STATE(ch, PULSE_VIOLENCE);
   set_cantquit(ch, victim);

   bool disrupt_bingo = false;
   int success_chance = 0;   
   int learned = has_skill(ch, KI_OFFSET+KI_DISRUPT);

   if (learned > 85) {
     int level_diff = GET_LEVEL(ch) - GET_LEVEL(victim);

     if (level_diff >= 0) {
       success_chance = 6;
     } else if (level_diff >= -20) {
       success_chance = 5;
     } else if (level_diff >= -100) {
       success_chance = 4;
     } else {
       success_chance = 1;
     }

     if (success_chance >= number(1,100)) {
       disrupt_bingo = true;
     }
   }

   if (disrupt_bingo == true) {
     act("$n slams a bolt of focused ki energy into the flow of magic all around you!", ch, 0, victim, TO_VICT, 0);
     act("$n focuses a blast of ki to disrupt the flow of magic all around $N!", ch, 0, victim, TO_ROOM, 0);
     send_to_char("You focus your ki to disrupt the flow of magic all around your opponent!\r\n", ch);
   } else {
     act("$n slams a bolt of focused ki energy into the flow of magic around you!", ch, 0, victim, TO_VICT, 0);
     act("$n focuses a blast of ki to disrupt the flow of magic around $N!", ch, 0, victim, TO_ROOM, 0);
     send_to_char("You focus your ki to disrupt the flow of magic around your opponent!\r\n", ch);
   }

   if(ISSET(victim->affected_by, AFF_GOLEM)) {
     send_to_char("The golem seems to shrug off your ki disrupt attempt!\r\n", ch);
     act("The golem seems to ignore $n's disrupting energy!", ch, 0, 0, TO_ROOM, 0);
      return eFAILURE;
   }
  if(IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_NODISPEL)) {
     act("$N seems to ignore $n's disrupting energy!", ch, 0, victim, TO_ROOM, 0);
     act("$N seems to ignore your disrupting energy!", ch, 0, victim, TO_CHAR, 0);
      return eFAILURE;
  }

   int savebonus = 0;
   if (learned < 41) {
     savebonus = 35;
   } else if (learned < 61) {
     savebonus = 30;
   } else if (learned < 81) {
     savebonus = 25;
   } else {
     savebonus = 20;
   }

   // Players are easier to disrupt
   if (IS_PC(victim)) {
     savebonus -= 10;
   }

   // Check if caster gets a bonus against this victim
   affected_type *af = affected_by_spell(victim, KI_DISRUPT + KI_OFFSET);
   if (af) {
     // We've KI_DISRUPTED the victim and failed before so we get a bonus
     if (af->caster == string(GET_NAME(ch))) {
       savebonus -= af->modifier;
     } else {
       // Some other caster's KI_DISRUPT was on the victim, removing it
       affect_from_char(victim, KI_DISRUPT + KI_OFFSET);
       af = 0;
     }
   }


   int retval = 0;

   if (number(1, 100) <= get_saves(victim, SAVE_TYPE_MAGIC) + savebonus && level != GET_LEVEL(ch)-1) {
     // We've failed this time, so we'll make it easier for next time
     if (af) {
       // We've failed before
       af->modifier += 1+(learned/20);
     } else {
       // This is the first time we've failed
       affected_type newaf;
       newaf.type      = KI_DISRUPT + KI_OFFSET;
       newaf.duration  = -1;
       newaf.modifier  = 1+(learned/20);
       newaf.location  = APPLY_NONE;
       newaf.bitvector = -1;
       newaf.caster = string(GET_NAME(ch));

       affect_to_char(victim, &newaf);
     }

     act("$N resists your attempt to disrupt magic!", ch, NULL, victim,
	  TO_CHAR,0);
     act("$N resists $n's attempt to disrupt magic!", ch, NULL, victim, TO_ROOM,
	  NOTVICT);
     act("You resist $n's attempt to disrupt magic!",ch, NULL, victim, TO_VICT,
	 0);

     if (IS_NPC(victim) && (!victim->fighting) &&
	 GET_POS(ch) > POSITION_SLEEPING) {
       retval = attack(victim, ch, TYPE_UNDEFINED);
       retval = SWAP_CH_VICT(retval);
       return retval;
     }

     return eFAILURE;
   }

   // We have success so if af is set then the victim had a ki_disupt
   // bonus set. We will remove it.
   if (af) {
     affect_from_char(victim, KI_DISRUPT + KI_OFFSET);
   }

   // Disrupt bingo chance
   if (disrupt_bingo) {
     if (affected_by_spell(victim, SPELL_SANCTUARY) ||
	 IS_AFFECTED(victim, AFF_SANCTUARY))
       {
	 affect_from_char(victim, SPELL_SANCTUARY);
	 REMBIT(victim->affected_by, AFF_SANCTUARY);
	 act("You don't feel so invulnerable anymore.", ch, 0, victim, TO_VICT, 0);
	 act("The $B$7white glow$R around $n's body fades.", victim, 0, 0, TO_ROOM, 0);
       }
     if (affected_by_spell(victim, SPELL_PROTECT_FROM_EVIL))
       {
	 affect_from_char(victim, SPELL_PROTECT_FROM_EVIL);
	 act("Your protection from evil has been disrupted!", ch, 0,victim, TO_VICT, 0);
	 act("The dark, $6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, TO_ROOM, 0);
       }
     
     if (affected_by_spell(victim, SPELL_HASTE))
       {
	 affect_from_char(victim, SPELL_HASTE);
	 act("Your magically enhanced speed has been disrupted!", ch, 0,victim, TO_VICT, 0);
	 act("$n's actions slow to their normal speed.", victim, 0, 0, TO_ROOM, 0);
       }
     
     if (affected_by_spell(victim, SPELL_STONE_SHIELD)) 
       {
	 affect_from_char(victim, SPELL_STONE_SHIELD);
	 act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0,victim, TO_VICT, 0);
	 act("The shield of stones swirling about $n's body fall to the ground!", victim, 0, 0, TO_ROOM, 0);
       }
     
     if (affected_by_spell(victim, SPELL_GREATER_STONE_SHIELD))
       {
	 affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);
	 act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0,victim, TO_VICT, 0);
	 act("The shield of stones swirling about $n's body falls to the ground!", victim, 0, 0, TO_ROOM, 0);
       }
     
     if (IS_AFFECTED(victim, AFF_FROSTSHIELD))
       {
	 REMBIT(victim->affected_by, AFF_FROSTSHIELD);
	 act("Your shield of $B$3frost$R melts into nothing!.", ch, 0,victim, TO_VICT, 0);
	 act("The $B$3frost$R encompassing $n's body melts away.", victim, 0, 0, TO_ROOM, 0);
       }
     
     if (affected_by_spell(victim, SPELL_LIGHTNING_SHIELD))
       {
	 affect_from_char(victim, SPELL_LIGHTNING_SHIELD);
	 act("Your crackling shield of $B$5electricity$R vanishes!", ch, 0,victim, TO_VICT, 0);
	 act("The $B$5electricity$R crackling around $n's body fades away.", victim, 0, 0, TO_ROOM, 0);	 
       }       

     if (affected_by_spell(victim, SPELL_FIRESHIELD) || IS_AFFECTED(victim, AFF_FIRESHIELD))
       {
	 REMBIT(victim->affected_by, AFF_FIRESHIELD);
	 affect_from_char(victim, SPELL_FIRESHIELD);
	 act("Your $B$4flames$R have been extinguished!", ch, 0,victim, TO_VICT, 0);
	 act("The $B$4flames$R encompassing $n's body are extinguished!", victim, 0, 0, TO_ROOM, 0);
       }
     if (affected_by_spell(victim, SPELL_ACID_SHIELD))
       {
	 affect_from_char(victim, SPELL_ACID_SHIELD);
	 act("Your shield of $B$2acid$R dissolves to nothing!", ch, 0,victim, TO_VICT, 0);
	 act("The $B$2acid$R swirling about $n's body dissolves to nothing!", victim, 0, 0, TO_ROOM, 0);
       }       
     if (affected_by_spell(victim, SPELL_PROTECT_FROM_GOOD))
       {
	 affect_from_char(victim, SPELL_PROTECT_FROM_GOOD);
	 act("Your protection from good has been disrupted!", ch, 0,victim, TO_VICT, 0);
	 act("The light, $B$6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, TO_ROOM, 0);
       }       

     if(IS_NPC(victim) && !victim->fighting)
     {
       retval = attack(victim, ch, 0);
       SWAP_CH_VICT(retval);
       return retval;
     }
   }

   // This section of code looks for specific spells or affects and
   // adds them to a list called aff_list. Then a random element of
   // the list will be chosen for removal. This ensures we pick a random
   // affect only out of those that the player is using.
   vector<affected_type> aff_list;

   // Since we're looking for either these 3 affects OR the spells that cause them
   // we're keeping a track of which is found so we don't mark them twice
   bool frostshieldFound = false, fireshieldFound = false, sanctuaryFound = false;

   for(affected_type *curr = victim->affected; curr; curr = curr->next) {
     switch(curr->type) {
     case SPELL_FROSTSHIELD:
       frostshieldFound = true;
       aff_list.push_back(*curr);
       break;
     case SPELL_FIRESHIELD:
       fireshieldFound = true;
       aff_list.push_back(*curr);
       break;
     case SPELL_SANCTUARY:
       sanctuaryFound = true;
       aff_list.push_back(*curr);
       break; 
     case SPELL_PROTECT_FROM_EVIL:
     case SPELL_HASTE:
     case SPELL_STONE_SHIELD:
     case SPELL_GREATER_STONE_SHIELD:
     case SPELL_LIGHTNING_SHIELD:
     case SPELL_ACID_SHIELD:
     case SPELL_PROTECT_FROM_GOOD:
       aff_list.push_back(*curr);
       break;
     }
   }

   // For these 3 affects, if they weren't caused by a spell we'll
   // add them to our list as if they were a spell to be removed
   affected_type localaff;

   if (IS_AFFECTED(victim, AFF_FROSTSHIELD) && !frostshieldFound) {
     localaff.type = SPELL_FROSTSHIELD;
     aff_list.push_back(localaff);
   }

   if (IS_AFFECTED(victim, AFF_FIRESHIELD) && !fireshieldFound) {
     localaff.type = SPELL_FIRESHIELD;
     aff_list.push_back(localaff);
   }

   if (IS_AFFECTED(victim, AFF_SANCTUARY) && !sanctuaryFound) {
     localaff.type = SPELL_SANCTUARY;
     aff_list.push_back(localaff);
   }

   // Nothing applicable found to be removed
   if (aff_list.size() < 1) {
     return eFAILURE;
   }

   // Pick the lucky spell/affect to be removed
   int i = number(0, aff_list.size()-1);

   try { af = &aff_list.at(i); } catch(...) { return eFAILURE; }

   if (af->type == SPELL_SANCTUARY) {
     affect_from_char(victim, SPELL_SANCTUARY);
     REMBIT(victim->affected_by, AFF_SANCTUARY);
     act("You don't feel so invulnerable anymore.", ch, 0, victim, TO_VICT, 0);
     act("The $B$7white glow$R around $n's body fades.", victim, 0, 0, TO_ROOM, 0);
   }

   if (af->type == SPELL_PROTECT_FROM_EVIL) {
     affect_from_char(victim, SPELL_PROTECT_FROM_EVIL);
     act("Your protection from evil has been disrupted!", ch, 0,victim, TO_VICT, 0);
     act("The dark, $6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, TO_ROOM, 0);
   }

   if (af->type == SPELL_HASTE) {
     affect_from_char(victim, SPELL_HASTE);
     act("Your magically enhanced speed has been disrupted!", ch, 0,victim, TO_VICT, 0);
     act("$n's actions slow to their normal speed.", victim, 0, 0, TO_ROOM, 0);
   }
  
   if (af->type == SPELL_STONE_SHIELD) {
     affect_from_char(victim, SPELL_STONE_SHIELD);
     act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0,victim, TO_VICT, 0);
     act("The shield of stones swirling about $n's body fall to the ground!", victim, 0, 0, TO_ROOM, 0);
   }

   if (af->type == SPELL_GREATER_STONE_SHIELD) {
     affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);
     act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0,victim, TO_VICT, 0);
     act("The shield of stones swirling about $n's body falls to the ground!", victim, 0, 0, TO_ROOM, 0);
   }
   
   if (af->type == SPELL_FROSTSHIELD) {
     affect_from_char(victim, SPELL_FROSTSHIELD);
     REMBIT(victim->affected_by, AFF_FROSTSHIELD);
     act("Your shield of $B$3frost$R melts into nothing!.", ch, 0,victim, TO_VICT, 0);
     act("The $B$3frost$R encompassing $n's body melts away.", victim, 0, 0, TO_ROOM, 0);
   }

   if (af->type == SPELL_LIGHTNING_SHIELD) {
     affect_from_char(victim, SPELL_LIGHTNING_SHIELD);
     act("Your crackling shield of $B$5electricity$R vanishes!", ch, 0,victim, TO_VICT, 0);
     act("The $B$5electricity$R crackling around $n's body fades away.", victim, 0, 0, TO_ROOM, 0);
   }

   if (af->type == SPELL_FIRESHIELD) {
     REMBIT(victim->affected_by, AFF_FIRESHIELD);
     affect_from_char(victim, SPELL_FIRESHIELD);
     act("Your $B$4flames$R have been extinguished!", ch, 0,victim, TO_VICT, 0);
     act("The $B$4flames$R encompassing $n's body are extinguished!", victim, 0, 0, TO_ROOM, 0);
   }

   if (af->type == SPELL_ACID_SHIELD) {
     affect_from_char(victim, SPELL_ACID_SHIELD);
     act("Your shield of $B$2acid$R dissolves to nothing!", ch, 0,victim, TO_VICT, 0);
     act("The $B$2acid$R swirling about $n's body dissolves to nothing!", victim, 0, 0, TO_ROOM, 0);
   }

   if (af->type == SPELL_PROTECT_FROM_GOOD) {
     affect_from_char(victim, SPELL_PROTECT_FROM_GOOD);
     act("Your protection from good has been disrupted!", ch, 0,victim, TO_VICT, 0);
     act("The light, $B$6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, TO_ROOM, 0);
   }

  if(IS_NPC(victim) && !victim->fighting)
  {
    retval = attack(victim, ch, 0);
    SWAP_CH_VICT(retval);
    return retval;
  }
   return eSUCCESS;
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
   if(number(1, 100) > ( GET_DEX(ch) * 4 ) ) {
      send_to_char("You accidently stub your toe and fall out of the defenseive stance.\n\r", ch);
      return eSUCCESS;
   }

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
  int learned, chance, percent;
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
      af.modifier  = -10 - learned / 4;
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

int ki_transfer( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim)
{
  char amt[MAX_STRING_LENGTH], type[MAX_STRING_LENGTH];
  int amount, temp=0;
  struct affected_type af;

  argument_interpreter(arg, amt, type);
  //arg = one_argument(arg, amt);
  //arg = one_argument(arg, type);

  amount = atoi(amt);

  if(amount < 0) 
  {
    send_to_char("Trying to be a funny guy?\r\n", ch);
    return eFAILURE;
  }

  if(amount > GET_KI(ch)) 
  {
    send_to_char("You do not have that much energy to transfer.\r\n", ch);
    return eFAILURE;
  }

  int learned = has_skill(ch, KI_TRANSFER+KI_OFFSET);

  if(affected_by_spell(victim, SPELL_KI_TRANS_TIMER)) 
  {
    act("$N cannot receive a transfer right now due to the stress $S mind has been recently been through.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if(affected_by_spell(ch, SPELL_KI_TRANS_TIMER)) affect_from_char(ch, SPELL_KI_TRANS_TIMER);

  af.type      = SPELL_KI_TRANS_TIMER;
  af.duration  = 1;
  af.modifier  = 0;
  af.location  = 0;
  af.bitvector = -1;

  affect_to_char(ch, &af);

  if(type[0] == 'k') 
  {
    GET_KI(ch) -= amount;
    temp = number(amount-amount/10, amount+amount/10);  //+-10%
    temp = (temp * learned) / 100;
    GET_KI(victim) += temp;
    if(GET_KI(victim) > GET_MAX_KI(victim)) 
      GET_KI(victim) = GET_MAX_KI(victim);

    sprintf(amt, "%d", amount);
    send_damage("You focus intently, bonding briefly with $N's spirit, transferring | ki of your essence to $M.", 
            ch,  0, victim, amt, 
            "You focus intently, bonding briefly with $N's spirit,  transferring a portion of your essence to $M.", TO_CHAR);
    sprintf(amt, "%d", temp);
    send_damage("$n focuses intently, bonding briefly with your spirit, replenishing | ki of your essence with $s own.", 
                 ch, 0, victim, amt, 
                "$n focuses intently, bonding briefly with your spirit, replenishing your essence with $s own.", TO_VICT);
    act("$n focuses intently upon $N as though briefly bonding with $S spirit.", ch, 0, victim, TO_ROOM, NOTVICT);
  }
  else if(type[0] == 'm') 
  {
    GET_KI(ch) -= amount;
    
    temp = number(amount-amount/10, amount+amount/10) * 2;
    temp = (temp * learned) / 100;
    GET_MANA(victim) += temp;
    if(GET_MANA(victim) > GET_MAX_MANA(victim)) 
      GET_MANA(victim) = GET_MAX_MANA(victim);

    sprintf(amt, "%d", amount);
    send_damage("You focus intently, bonding briefly with $N's spirit, transferring | ki of your essence to $M.", 
                 ch, 0, victim, amt, 
                 "You focus intently, bonding briefly with $N's spirit, transferring a portion of your essence to $M.", TO_CHAR);
   sprintf(amt, "%d", temp);
   send_damage("$n focuses intently, bonding briefly with your spirit, replenishing | magic with a portion of $s essence.", 
                ch, 0, victim, amt, 
               "$n focuses intently, bonding briefly with your spirit, replenishing your magic with a portion of $s essence.", TO_VICT);
    act("$n focuses intently upon $N as though briefly bonding with $S spirit.", ch, 0, victim, TO_ROOM, NOTVICT);
  }
  else 
  {
    send_to_char("You do not know of that essense.\r\n", ch);
    return eFAILURE;
  }

  return eSUCCESS;
}
