/************************************************************************
| $Id: cl_monk.cpp,v 1.1 2002/06/13 04:32:20 dcastle Exp $
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
    struct obj_data *wielded, *held;
    /*struct affected_type af;*/
    struct char_data *victim;
    char name[256];
    byte percent;
    byte learned;
    int dam;
    int retval;


    if (((GET_CLASS(ch) != CLASS_MONK ))
        && GET_LEVEL(ch)<ARCHANGEL) {
	send_to_char("You better leave all the kung-fu to monks.\n\r", ch);
	return eFAILURE;
    }

      if (GET_LEVEL(ch) < 20) {
         send_to_char("You can't perform such a ancient skill!\n\r", ch);
            return eFAILURE;
            }

  if(IS_MOB(ch))
    learned = 75;
  else if(!(learned = has_skill(ch, SKILL_EAGLE_CLAW))) {
    send_to_char("Eagle my ass...you're still just a pigeon boy.\r\n", ch);
    return eFAILURE;
  }

     wielded = ch->equipment[WIELD];
     held = ch->equipment[HOLD];

     if (wielded || held) {
    send_to_char ("You can't wield or hold anything to perform ths!\n\r", ch);
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

    if(ch->in_room != victim->in_room)
    {
      send_to_char("That person seems to have left.\n\r", ch);
      return eFAILURE;
    }

    if (victim == ch) {
	send_to_char("Aren't we funny today...\n\r", ch);
	return eFAILURE;
    }

    if(!can_attack(ch) || !can_be_attacked(ch, victim))
      return eFAILURE;

     /* 101% is a complete failure */
    percent= number(1,101);

    WAIT_STATE(ch, PULSE_VIOLENCE*3);

    if (percent > learned) 
    {
	retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_EAGLE_CLAW, 0);

    } else {

      dam = dice(GET_LEVEL(ch), 8);

        if (dam > GET_HIT(victim)) {
             make_heart(ch, victim);
             dam += 20;
         }
	retval = damage(ch, victim, dam,TYPE_UNDEFINED, SKILL_EAGLE_CLAW, 0);
    }
    return retval;
}


// removed affect and made it ki based while upping damage a lil
int do_quivering_palm(struct char_data *ch, char *argument, int cmd)
{
  struct obj_data *wielded, *held;
  // struct affected_type af;
  struct char_data *victim;
  char name[256];
  byte percent;
  byte learned;
  int dam;
  int retval;

  if(GET_CLASS(ch) != CLASS_MONK && GET_LEVEL(ch) < ARCHANGEL) {
    send_to_char("You better leave all the kung-fu to monks.\n\r", ch);
    return eFAILURE;
  }

  if(GET_LEVEL(ch) < 40) {
    send_to_char("You can't perform such an ancient skill!\n\r", ch);
    return eFAILURE;
  }

  if(IS_MOB(ch))
    learned = 75;
  else if(!(learned = has_skill(ch, SKILL_QUIVERING_PALM))) {
    send_to_char("Stick to palming yourself for now bucko.\r\n", ch);
    return eFAILURE;
  }

  if(affected_by_spell(ch, SKILL_QUIVERING_PALM)) {
     send_to_char("You can't perform such an ancient power more than "
                  "once a day!\n\r", ch);
     return eFAILURE;
  }

  wielded = ch->equipment[WIELD];
  held = ch->equipment[HOLD];

  if(wielded || held) {
    send_to_char ("You can't wield or hold anything to perform ths!\n\r", ch);
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

    if(ch->in_room != victim->in_room)
    {
      send_to_char("That person seems to have left.\n\r", ch);
      return eFAILURE;
    }

  if(victim == ch) {
    send_to_char("Masturbate on your own time.\n\r", ch);
    return eFAILURE;
  }

  if(GET_KI(ch) < 40 && GET_LEVEL(ch) < ARCHANGEL) {
    send_to_char("You don't possess enough ki!\r\n", ch);
    return eFAILURE;
  }

    if(!can_attack(ch) || !can_be_attacked(ch, victim))
          return eFAILURE;
	   
  GET_KI(ch) -= 40;

  /* 101% is a complete failure */
  percent = number(1, 101);

  WAIT_STATE(ch, PULSE_VIOLENCE*2);
  // af.type = SKILL_QUIVERING_PALM;
  // af.modifier = 0;
  // af.location = APPLY_NONE;
  // af.bitvector = 0;
  // affect_to_char(ch, &af);

  if(percent > learned) {
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_QUIVERING_PALM, 0);
  }
  else {
    dam = dice((GET_LEVEL(ch)), MORTAL) + (25 * (GET_LEVEL(ch)));
    retval = damage(ch, victim, dam, TYPE_UNDEFINED, SKILL_QUIVERING_PALM, 0);
  }
  return retval;
}

int do_stun(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[256];
  byte percent;
  int learned;
  int chance;
  int retval;
  int specialization;

  if((GET_CLASS(ch) == CLASS_WARRIOR) && (GET_LEVEL(ch) < 45)) {
    send_to_char("You aren't quite the man you think!\n\r", ch);
    return eFAILURE;
  }

  if(IS_MOB(ch))
    learned = 75;
  else if(!(learned = has_skill(ch, SKILL_STUN))) {
    send_to_char("Your lack of knowledge is stunning...\r\n", ch);
    return eFAILURE;
  }

  specialization = learned/100;
  learned = learned % 100;

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

    if(ch->in_room != victim->in_room)
    {
      send_to_char("That person seems to have left.\n\r", ch);
      return eFAILURE;
    }

  if(!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if(GET_LEVEL(victim) == IMP) {
    send_to_char("You gotta be kidding!\n\r",ch);
    return eFAILURE;
  }

  if(specialization < 1 && IS_MOB(victim) && IS_SET(victim->mobdata->actflags, ACT_HUGE))
  {
    send_to_char("You cannot stun something that HUGE!\n\r", ch);
    return eFAILURE;
  }

  // 101% is a complete failure
  percent = number(1, 101) + (GET_LEVEL(victim) - GET_LEVEL(ch));

  if(GET_CLASS(ch) == CLASS_MONK)
    chance = learned * 2 /3;
  else
    chance = learned / 2;

  if(percent > chance) {
    act("$n attempts to hit you in your solar plexus!  You block $s attempt.",
	  ch, NULL, victim, TO_VICT , 0);

    act("You attempt to hit $N in $s solar plexus...   YOU MISS!",
        ch, NULL, victim, TO_CHAR , 0);
    act("$n attempts to hit $N in $S solar plexus...   $e MISSES!",
        ch, NULL, victim, TO_ROOM, NOTVICT );
    WAIT_STATE(ch, PULSE_VIOLENCE*4);
    retval = damage (ch, victim, 0,TYPE_UNDEFINED, SKILL_STUN, 0);
  }
  else {
    act("$n delivers a HARD BLOW into your solar plexus!  You are STUNNED!",
	ch, NULL, victim, TO_VICT , 0);
    act("You deliver a HARD BLOW into $N's solar plexus!  $N is STUNNED!",
        ch, NULL, victim, TO_CHAR , 0);
    act("$n delivers a HARD BLOW into $N's solar plexus!  $N is STUNNED!",
	   ch, NULL, victim, TO_ROOM, NOTVICT );

    WAIT_STATE(ch, PULSE_VIOLENCE*5);
    WAIT_STATE(victim, PULSE_VIOLENCE*2);
    if(GET_POS(victim) > POSITION_STUNNED)
      GET_POS(victim) = POSITION_STUNNED;
    SET_BIT(victim->combat, COMBAT_STUNNED);
    retval = damage (ch, victim, 0,TYPE_UNDEFINED, SKILL_STUN, 0);
  }
  return retval;
}
