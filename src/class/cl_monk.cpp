/************************************************************************
| $Id: cl_monk.cpp,v 1.18 2004/05/25 01:04:52 urizen Exp $
| cl_monk.C
| Description:  Monk skills.
*/
#include <structs.h>
#include <player.h>
#include <levels.h>
#include <fight.h>
#include <utility.h>
#include <character.h>
#include <spells.h>
#include <handler.h>
#include <connect.h>
#include <mobile.h>
#include <act.h>
#include <returnvals.h>

/************************************************************************
| OFFENSIVE commands.
*/
int do_eagle_claw(struct char_data *ch, char *argument, int cmd)
{
   struct char_data *victim;
   char name[MAX_INPUT_LENGTH];
   int dam;
   int retval;

   if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
     ;
   else if(!has_skill(ch, SKILL_EAGLE_CLAW)) {
     send_to_char("Eagle my ass...you're still just a pigeon boy.\r\n", ch);
     return eFAILURE;
   }

   int hands = 0;
   if(ch->equipment[WIELD])          hands++;
   if(ch->equipment[SECOND_WIELD])   hands++;
   if(ch->equipment[HOLD])           hands++;
   if(ch->equipment[WEAR_SHIELD])    hands++;
   if(ch->equipment[WEAR_LIGHT])     hands++;

   if(hands > 1) {
     send_to_char ("You need at least one hand free to perform this!\n\r", ch);
     return eFAILURE;
   }

   if (ch->equipment[HOLD]) {
     send_to_char ("You can't hold anything while you perform this!\n\r", ch);
     return eFAILURE;
   }

   one_argument(argument, name);

   if (!(victim = get_char_room_vis(ch, name))) {
      if (ch->fighting) {
         victim = ch->fighting;
      } else {
         send_to_char("Eagle claw whom?\n\r", ch);
         return eFAILURE;
      }
   }

   if (victim == ch) {
      send_to_char("Aren't we funny today...\n\r", ch);
      return eFAILURE;
   }

   if(!can_attack(ch) || !can_be_attacked(ch, victim))
      return eFAILURE;

   skill_increase_check(ch, SKILL_EAGLE_CLAW, has_skill(ch,SKILL_EAGLE_CLAW), SKILL_INCREASE_MEDIUM);

   WAIT_STATE(ch, PULSE_VIOLENCE*3);

   if (!skill_success(ch,victim, SKILL_EAGLE_CLAW)) 
      retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_EAGLE_CLAW, 0);
   else 
   {
      dam = dice(GET_LEVEL(ch), 6);

      if (245 > GET_HIT(victim)) {
         // TODO - have 'learned' effect how good the heart is that you grab out
         // for the later modifications to this.  Hearts regen ki, etc etc.
         make_heart(ch, victim);
         dam += 20;
      }
      retval = damage(ch, victim, 250,TYPE_UNDEFINED, SKILL_EAGLE_CLAW, 0);
   }
   return retval;
}


int do_quivering_palm(struct char_data *ch, char *argument, int cmd)
{
  struct affected_type af;
  struct char_data *victim;
  char name[256];
  int dam, retval;

  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
    ;
  else if(!has_skill(ch, SKILL_QUIVERING_PALM)) {
    send_to_char("Stick to palming yourself for now bucko.\r\n", ch);
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
  if(ch->equipment[HOLD])           hands++;
  if(ch->equipment[WEAR_SHIELD])    hands++;
  if(ch->equipment[WEAR_LIGHT])     hands++;

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

  if(!can_attack(ch) || !can_be_attacked(ch, victim))
     return eFAILURE;
	   
  if(GET_KI(ch) < 40 && GET_LEVEL(ch) < ARCHANGEL) {
    send_to_char("You don't possess enough ki!\r\n", ch);
    return eFAILURE;
  }

  GET_KI(ch) -= 40;

  skill_increase_check(ch, SKILL_QUIVERING_PALM, has_skill(ch,SKILL_QUIVERING_PALM), SKILL_INCREASE_EASY);

  WAIT_STATE(ch, PULSE_VIOLENCE*2);
  af.type = SKILL_QUIVERING_PALM;
  af.duration = 12;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = 0;
  affect_to_char(ch, &af);

  if(skill_success(ch,victim,SKILL_QUIVERING_PALM)) {
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_QUIVERING_PALM, 0);
  }
  else {
    dam = GET_MAX_HIT(victim) /2;
    if (dam > 2000) dam = 2000;
    retval = damage(ch, victim, dam, TYPE_UNDEFINED, SKILL_QUIVERING_PALM, 0);
  }
  return retval;
}

int do_stun(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[256];
  int retval;

  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
    ;
  else if(!has_skill(ch, SKILL_STUN)) {
    send_to_char("Your lack of knowledge is stunning...\r\n", ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  victim = get_char_room_vis( ch, name );
  if(victim == NULL )
    victim = ch->fighting;

  if(victim == NULL) { 
    send_to_char( "Stun whom?\n\r", ch );
    return eFAILURE;
  }

  if(victim == ch) {
    send_to_char("Aren't we funny today...\n\r", ch);
    return eFAILURE;
  }

  if(!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if(GET_LEVEL(victim) == IMP) {
    send_to_char("You gotta be kidding!\n\r",ch);
    return eFAILURE;
  }

  if(IS_MOB(victim) && IS_SET(victim->mobdata->actflags, ACT_HUGE))
  {
    send_to_char("You cannot stun something that HUGE!\n\r", ch);
    return eFAILURE;
  }

  skill_increase_check(ch, SKILL_STUN, has_skill(ch,SKILL_STUN), SKILL_INCREASE_MEDIUM);

  if(!skill_success(ch,victim, SKILL_STUN) ) {
    act("$n attempts to hit you in your solar plexus!  You block $s attempt.", ch, NULL, victim, TO_VICT , 0);
    act("You attempt to hit $N in $s solar plexus...   YOU MISS!", ch, NULL, victim, TO_CHAR , 0);
    act("$n attempts to hit $N in $S solar plexus...   $e MISSES!", ch, NULL, victim, TO_ROOM, NOTVICT );

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
       act("$n delivers another HARD BLOW to your solar plexus!", ch, NULL, victim, TO_VICT , 0);
       act("You deliver another HARD BLOW into $N's solar plexus!", ch, NULL, victim, TO_CHAR , 0);
       act("$n delivers another HARD BLOW into $N's solar plexus!", ch, NULL, victim, TO_ROOM, NOTVICT );
       if(number(0, 1))
       {
          send_to_char("The hit knocks the sense back into you!\r\n", victim);
          act("The hit knocks the sense back into $N and $E is no longer stunned!", ch, 0, victim, TO_ROOM, NOTVICT);
          REMOVE_BIT(victim->combat, COMBAT_STUNNED);
          REMOVE_BIT(victim->combat, COMBAT_STUNNED2);
       }
       return damage (ch, victim, 0,TYPE_UNDEFINED, SKILL_STUN, 0);
    }

    act("$n delivers a HARD BLOW into your solar plexus!  You are STUNNED!", ch, NULL, victim, TO_VICT , 0);
    act("You deliver a HARD BLOW into $N's solar plexus!  $N is STUNNED!", ch, NULL, victim, TO_CHAR , 0);
    act("$n delivers a HARD BLOW into $N's solar plexus!  $N is STUNNED!", ch, NULL, victim, TO_ROOM, NOTVICT );
    WAIT_STATE(victim, PULSE_VIOLENCE*2);
    if(GET_POS(victim) > POSITION_STUNNED)
      GET_POS(victim) = POSITION_STUNNED;
    SET_BIT(victim->combat, COMBAT_STUNNED);
    retval = damage (ch, victim, 0,TYPE_UNDEFINED, SKILL_STUN, 0);
  }
  return retval;
}
