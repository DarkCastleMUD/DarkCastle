/************************************************************************
| $Id: cl_monk.cpp,v 1.42 2011/11/17 01:58:59 jhhudso Exp $
| cl_monk.C
| Description:  Monk skills.
*/
#include "structs.h"
#include "player.h"
#include "levels.h"
#include "fight.h"
#include "utility.h"
#include "character.h"
#include "spells.h"
#include "handler.h"
#include "connect.h"
#include "mobile.h"
#include "act.h"
#include "returnvals.h"
#include "db.h"
#include "room.h"
#include "interp.h"

/************************************************************************
| OFFENSIVE commands.
*/
int do_eagle_claw(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[MAX_INPUT_LENGTH];
  int dam;
  int retval;
  time_t time_raw_format;
  struct tm * ptr_time;

  if (!canPerform(ch, SKILL_EAGLE_CLAW, "Yooo are not pepared to use thees skeel, grasshoppa.\r\n")) {
    return eFAILURE;
  }

  int hands = 0;
  if(ch->equipment[WIELD])          hands++;
  if(ch->equipment[SECOND_WIELD])   hands++;
  if(ch->equipment[HOLD])           hands++;
  if(ch->equipment[HOLD2])          hands++;
  if(ch->equipment[WEAR_SHIELD])    hands++;
  if(ch->equipment[WEAR_LIGHT])     hands++;

  if(hands > 1) {
    send_to_char ("You need a free hand to eagleclaw someone.\r\n", ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  if (!(victim = get_char_room_vis(ch, name))) {
    if (ch->fighting)
      victim = ch->fighting;
    else {
      send_to_char("You raise your hand in a claw and make strange bird noises.\r\n", ch);
      return eFAILURE;
      }
  }

  if (victim == ch) {
    send_to_char("You lower your claw-shaped hand and scratch yourself gently.\r\n", ch);
    return eFAILURE;
  }

  if (IS_SET(victim->combat, COMBAT_BLADESHIELD1) || IS_SET(victim->combat, COMBAT_BLADESHIELD2)) {
        send_to_char("Clawing a bladeshielded opponent would be suicide!\n\r", ch);
        return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (!charge_moves(ch, SKILL_EAGLE_CLAW)) return eSUCCESS;

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);

  if (!skill_success(ch,victim, SKILL_EAGLE_CLAW))
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_EAGLE_CLAW, 0);
  else 
  {
    // 1% bingo chance 
    if (number(1, 100) == 1 && GET_LEVEL(victim) < IMMORTAL) {
      time ( &time_raw_format );
      ptr_time = localtime ( &time_raw_format );
      if( 11 == ptr_time->tm_mon ) {
        do_say(victim, "Laaaaaaast Christmas, I gave you my....", 9);
      }
      act("$N blinks and stares glassy-eyed into the distance blissfully no longer aware of $n RIPPING OUT $S $B$4heart$R!", ch, 0, victim, TO_ROOM, NOTVICT);
      act("You feel empty inside and full of heart-ache as if something important to you is missing.  Memories flash of your longing fo....", ch, 0, victim, TO_VICT, 0);
      act("You slide your fingers between $N's ribs and give $S's left ventricle a tickle with your pinky before RIPPING OUT $S heart.", ch, 0, victim, TO_CHAR, 0);
      dam = 9999999;
    } else {
      dam = (GET_STR(ch) * 3) + (GET_DEX(ch) * 2) + dice(2, GET_LEVEL(ch)) + 100;
    }
    retval = damage(ch, victim, dam, TYPE_UNDEFINED, SKILL_EAGLE_CLAW, 0);
  }

  return retval;
}


int do_quivering_palm(struct char_data *ch, char *argument, int cmd)
{
  extern CWorld world;
  struct affected_type af;
  struct char_data *victim;
  char name[256];
  int dam, retval;
  int duration = 100;

	if (!canPerform(ch, SKILL_QUIVERING_PALM, "Stick to palming yourself for now bucko.\r\n")) {
    return eFAILURE;
  }

  if(affected_by_spell(ch, SKILL_QUIVERING_PALM)) {
     send_to_char("You can't perform such an ancient power more than "
                  "once a day!\n\r", ch);
     return eFAILURE;
  }

  int hands = 0;
  if(ch->equipment[WIELD])          hands++;
  if(ch->equipment[SECOND_WIELD])   hands++;
  if(ch->equipment[WEAR_SHIELD])    hands++;

  if(hands > 1) {
    send_to_char ("You need at least one hand free to perform this!\n\r", ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  if(!(victim = get_char_room_vis(ch, name))) {
    if(ch->fighting)
      victim = ch->fighting;
    else {
      send_to_char("Quivering palm whom?\n\r", ch);
      return eFAILURE;
    }
  }

  if(victim == ch) {
    send_to_char("Masturbate on your own time.\n\r", ch);
    return eFAILURE;
  }

  if (IS_SET(victim->combat, COMBAT_BLADESHIELD1) || IS_SET(victim->combat, COMBAT_BLADESHIELD2)) {
        send_to_char("Palming a bladeshielded opponent would be suicide!\n\r", ch);
        return eFAILURE;
  }

  if (IS_SET(world[ch->in_room].room_flags, NO_KI)) {
    send_to_char("You find yourself unable to focus your energy here.\n\r", ch);
    return eFAILURE;
  }

  if(GET_KI(ch) < 40 && GET_LEVEL(ch) < ARCHANGEL) {
    send_to_char("You don't possess enough ki!\r\n", ch);
    return eFAILURE;
  }

  if(!can_attack(ch) || !can_be_attacked(ch, victim))
     return eFAILURE;

  if (!charge_moves(ch, SKILL_QUIVERING_PALM)) return eSUCCESS;
	   
  GET_KI(ch) -= 40;

  if(!skill_success(ch,victim,SKILL_QUIVERING_PALM)) {
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_QUIVERING_PALM, 0);
    duration = 6;
  }
  else {
    dam = GET_HIT(victim) /2;
    if (dam < 500) dam = 500;
    if (dam > 2000) dam = 2000;
    retval = damage(ch, victim, dam, TYPE_UNDEFINED, SKILL_QUIVERING_PALM, 0);
    duration = 12;
  }
  WAIT_STATE(ch, PULSE_VIOLENCE*2);
  af.type = SKILL_QUIVERING_PALM;
  af.duration = duration;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  
  return retval;
}

int do_stun(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[256];
  int retval;

  if (!canPerform(ch, SKILL_STUN, "Your lack of knowledge is stunning...\r\n")) {
    return eFAILURE;
  }
  if (GET_HIT(ch) < 25)
  {
    send_to_char("You can't muster the energy for such an attack.\r\n",ch);
    return eFAILURE;
  }
  one_argument(argument, name);
  
  if (name[0] != '\0')
    victim = get_char_room_vis( ch, name );
  else
     victim = ch->fighting;

  if(victim == NULL) { 
    send_to_char( "Stun whom?\n\r", ch );
    return eFAILURE;
  }

  if(victim == ch) {
    send_to_char("Aren't we funny today...\n\r", ch);
    return eFAILURE;
  }

  if (IS_SET(victim->combat, COMBAT_BLADESHIELD1) || IS_SET(victim->combat, COMBAT_BLADESHIELD2)) {
      send_to_char("Stunning a bladeshielded opponent would be suicide!\n\r", ch);
      return eFAILURE;
  }

  if (GET_LEVEL(ch) < IMMORTAL && IS_PC(victim) && GET_LEVEL(victim) >= IMMORTAL) {
     retval = act("Due to immortal magic, you shake off $n's attempt to immobilize you.", ch, NULL, victim, TO_VICT, 0);
     if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
         return retval;
     }

     retval = act("Due to immortal magic, $N shakes off $n's attempt to immobilize them.",ch, NULL, victim, TO_ROOM, NOTVICT);
     if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
         return retval;
     }

     retval = act("$n's stun reflects back to them!",ch, NULL, victim, TO_ROOM, 0);
     if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
         return retval;
     }

     retval = act("Due to immortal magic, $N shakes off your attempt to immobilize them.",ch, NULL, victim, TO_CHAR, 0);
     if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
         return retval;
     }

     retval = act("Your stun is reflected back to yourself!",ch, NULL, NULL, TO_CHAR, 0);
     if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
         return retval;
     }

     victim = ch;
  }

  if(GET_LEVEL(victim) == IMP) {
    send_to_char("You gotta be kidding!\n\r",ch);
    return eFAILURE;
  }

  if(IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_HUGE))
  {
    send_to_char("You cannot stun something that HUGE!\n\r", ch);
    return eFAILURE;
  }

  if(IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_SWARM))
  {
    send_to_char("You cannot pick just one of them to stun!\n\r", ch);
    return eFAILURE;
  }

  if(IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_TINY))
  {
    act("$N's small size proves impossible to target a stunning blow upon!", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if(!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (!charge_moves(ch, SKILL_STUN)) return eSUCCESS;

  if (IS_SET(victim->combat, COMBAT_BERSERK) && (IS_NPC(victim) || has_skill(victim, SKILL_BERSERK) > 80))
  {
     retval = act("In your enraged state, you shake off $n's attempt to immobilize you.", ch, NULL, victim, TO_VICT, 0);
     if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
         return retval;
     }

     retval = act("$N shakes off $n's attempt to immobilize them.",ch, NULL, victim, TO_ROOM, NOTVICT);
     if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
         return retval;
     }

     retval = act("$N shakes off your attempt to immobilize them.",ch, NULL, victim, TO_CHAR, 0);
     if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
         return retval;
     }
	return eSUCCESS;     
  }
  if((!skill_success(ch,victim, SKILL_STUN) && GET_POS(victim) != POSITION_SLEEPING)
     || do_frostshield(ch, victim)) 
  {
    retval = act("$n attempts to hit you in your solar plexus!  You block $s attempt.", ch, NULL, victim, TO_VICT , 0);
    if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
        return retval;
    }

    retval = act("You attempt to hit $N in $s solar plexus...   YOU MISS!", ch, NULL, victim, TO_CHAR , 0);
    if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
        return retval;
    }

    retval = act("$n attempts to hit $N in $S solar plexus...   $e MISSES!", ch, NULL, victim, TO_ROOM, NOTVICT );
    if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
        return retval;
    }

    if(has_skill(ch,SKILL_STUN) > 35 && !number(0, 7)) {
       send_to_char("Your advanced knowledge of stun helps you to recover faster.\r\n", ch);
       WAIT_STATE(ch, PULSE_VIOLENCE*3);
    }
    else WAIT_STATE(ch, PULSE_VIOLENCE*4);
    retval = damage (ch, victim, 0,TYPE_UNDEFINED, SKILL_STUN, 0);
  }
  else 
  {
    set_fighting(victim, ch);
    WAIT_STATE(ch, PULSE_VIOLENCE*5);

    if(IS_SET(victim->combat, COMBAT_STUNNED) ||
       IS_SET(victim->combat, COMBAT_STUNNED2))
    {
       retval = act("$n delivers another HARD BLOW to your solar plexus!", ch, NULL, victim, TO_VICT , 0);
       if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
           return retval;
       }

       retval = act("You deliver another HARD BLOW into $N's solar plexus!", ch, NULL, victim, TO_CHAR , 0);
       if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
           return retval;
       }

       retval = act("$n delivers another HARD BLOW into $N's solar plexus!", ch, NULL, victim, TO_ROOM, NOTVICT );
       if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
           return retval;
       }

       if(number(0, 1))
       {
          send_to_char("The hit knocks the sense back into you!\r\n", victim);
          retval = act("The hit knocks the sense back into $N and $E is no longer stunned!", ch, 0, victim, TO_ROOM, NOTVICT);
          if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
              return retval;
          }

          REMOVE_BIT(victim->combat, COMBAT_STUNNED);
          REMOVE_BIT(victim->combat, COMBAT_STUNNED2);
       }
       return damage (ch, victim, 0,TYPE_UNDEFINED, SKILL_STUN, 0);
    }

    if(affected_by_spell(victim, SKILL_BATTLESENSE) &&
             number(1, 100) < affected_by_spell(victim, SKILL_BATTLESENSE)->modifier) {
      retval = act("$N's heightened battlesense sees your stun coming from a mile away and $E easily blocks it.", ch, 0, victim, TO_CHAR, 0);
      if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
          return retval;
      }

      retval = act("Your heightened battlesense sees $n's stun coming from a mile away and you easily block it.", ch, 0, victim, TO_VICT, 0);
      if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
          return retval;
      }

      retval = act("$N's heightened battlesense sees $n's stun coming from a mile away and $N easily blocks it.", ch, 0, victim, TO_ROOM, NOTVICT);
      if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
          return retval;
      }

    } else {
      retval = act("$n delivers a HARD BLOW into your solar plexus!  You are STUNNED!", ch, NULL, victim, TO_VICT , 0);
      if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
          return retval;
      }

      retval = act("You deliver a HARD BLOW into $N's solar plexus!  $N is STUNNED!", ch, NULL, victim, TO_CHAR , 0);
      if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
          return retval;
      }

      retval = act("$n delivers a HARD BLOW into $N's solar plexus!  $N is STUNNED!", ch, NULL, victim, TO_ROOM, NOTVICT );
      if (IS_SET(retval, eVICT_DIED) || IS_SET(retval, eCH_DIED)) {
          return retval;
      }

      WAIT_STATE(victim, PULSE_VIOLENCE*2);
      if(GET_POS(victim) > POSITION_STUNNED)
        GET_POS(victim) = POSITION_STUNNED;
      SET_BIT(victim->combat, COMBAT_STUNNED);
    }
    retval = damage (ch, victim, 0, TYPE_UNDEFINED, SKILL_STUN, 0);
  }
  return retval;
}
