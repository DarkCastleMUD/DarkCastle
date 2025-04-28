/************************************************************************
| $Id: cl_monk.cpp,v 1.42 2011/11/17 01:58:59 jhhudso Exp $
| cl_monk.C
| Description:  Monk skills.
*/
#include "DC/structs.h"
#include "DC/player.h"
#include "DC/levels.h"
#include "DC/fight.h"
#include "DC/utility.h"
#include "DC/character.h"
#include "DC/spells.h"
#include "DC/handler.h"
#include "DC/connect.h"
#include "DC/mobile.h"
#include "DC/act.h"
#include "DC/returnvals.h"
#include "DC/db.h"
#include "DC/room.h"
#include "DC/interp.h"

/************************************************************************
| OFFENSIVE commands.
*/
int do_eagle_claw(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char name[MAX_INPUT_LENGTH];
  int dam;
  int retval;
  time_t time_raw_format;
  struct tm *ptr_time;

  if (!ch->canPerform(SKILL_EAGLE_CLAW, "Yooo are not pepared to use thees skeel, grasshoppa.\r\n"))
  {
    return eFAILURE;
  }

  int hands = 0;
  if (ch->equipment[WIELD])
    hands++;
  if (ch->equipment[SECOND_WIELD])
    hands++;
  if (ch->equipment[HOLD])
    hands++;
  if (ch->equipment[HOLD2])
    hands++;
  if (ch->equipment[WEAR_SHIELD])
    hands++;
  if (ch->equipment[WEAR_LIGHT])
    hands++;

  if (hands > 1)
  {
    ch->sendln("You need a free hand to eagleclaw someone.");
    return eFAILURE;
  }

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    if (ch->fighting)
      victim = ch->fighting;
    else
    {
      ch->sendln("You raise your hand in a claw and make strange bird noises.");
      return eFAILURE;
    }
  }

  if (victim == ch)
  {
    ch->sendln("You lower your claw-shaped hand and scratch yourself gently.");
    return eFAILURE;
  }

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Clawing a bladeshielded opponent would be suicide!");
    return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (!charge_moves(ch, SKILL_EAGLE_CLAW))
    return eSUCCESS;

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  if (!skill_success(ch, victim, SKILL_EAGLE_CLAW))
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_EAGLE_CLAW, 0);
  else
  {
    // 1% bingo chance
    if (number(1, 100) == 1 && !victim->isImmortalPlayer())
    {
      time(&time_raw_format);
      ptr_time = localtime(&time_raw_format);
      if (11 == ptr_time->tm_mon)
      {
        do_say(victim, "Laaaaaaast Christmas, I gave you my....", CMD_DEFAULT);
      }
      act("$N blinks and stares glassy-eyed into the distance blissfully no longer aware of $n RIPPING OUT $S $B$4heart$R!", ch, 0, victim, TO_ROOM, NOTVICT);
      act("You feel empty inside and full of heart-ache as if something important to you is missing.  Memories flash of your longing fo....", ch, 0, victim, TO_VICT, 0);
      act("You slide your fingers between $N's ribs and give $S's left ventricle a tickle with your pinky before RIPPING OUT $S heart.", ch, 0, victim, TO_CHAR, 0);
      dam = 9999999;
    }
    else
    {
      dam = (GET_STR(ch) * 3) + (GET_DEX(ch) * 2) + dice(2, ch->getLevel()) + 100;
    }
    retval = damage(ch, victim, dam, TYPE_UNDEFINED, SKILL_EAGLE_CLAW, 0);
  }

  return retval;
}

int do_quivering_palm(Character *ch, char *argument, int cmd)
{

  struct affected_type af;
  Character *victim;
  char name[256];
  int dam, retval;
  int duration = 100;

  if (!ch->canPerform(SKILL_QUIVERING_PALM, "Stick to palming yourself for now bucko.\r\n"))
  {
    return eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_QUIVERING_PALM))
  {
    send_to_char("You can't perform such an ancient power more than "
                 "once a day!\n\r",
                 ch);
    return eFAILURE;
  }

  int hands = 0;
  if (ch->equipment[WIELD])
    hands++;
  if (ch->equipment[SECOND_WIELD])
    hands++;
  if (ch->equipment[WEAR_SHIELD])
    hands++;

  if (hands > 1)
  {
    ch->sendln("You need at least one hand free to perform this!");
    return eFAILURE;
  }

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    if (ch->fighting)
      victim = ch->fighting;
    else
    {
      ch->sendln("Quivering palm whom?");
      return eFAILURE;
    }
  }

  if (victim == ch)
  {
    ch->sendln("Masturbate on your own time.");
    return eFAILURE;
  }

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Palming a bladeshielded opponent would be suicide!");
    return eFAILURE;
  }

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_KI))
  {
    ch->sendln("You find yourself unable to focus your energy here.");
    return eFAILURE;
  }

  if (GET_KI(ch) < 40 && ch->getLevel() < ARCHANGEL)
  {
    ch->sendln("You don't possess enough ki!");
    return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (!charge_moves(ch, SKILL_QUIVERING_PALM))
    return eSUCCESS;

  GET_KI(ch) -= 40;

  if (!skill_success(ch, victim, SKILL_QUIVERING_PALM))
  {
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_QUIVERING_PALM, 0);
    duration = 6;
  }
  else
  {
    dam = victim->getHP() / 2;
    if (dam < 500)
      dam = 500;
    if (dam > 2000)
      dam = 2000;
    retval = damage(ch, victim, dam, TYPE_UNDEFINED, SKILL_QUIVERING_PALM, 0);
    duration = 12;
  }
  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  af.type = SKILL_QUIVERING_PALM;
  af.duration = duration;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  return retval;
}

int do_stun(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char name[256];
  int retval;

  if (!ch->canPerform(SKILL_STUN, "Your lack of knowledge is stunning...\r\n"))
  {
    return eFAILURE;
  }
  if (ch->getHP() < 25)
  {
    ch->sendln("You can't muster the energy for such an attack.");
    return eFAILURE;
  }
  one_argument(argument, name);

  if (name[0] != '\0')
    victim = ch->get_char_room_vis(name);
  else
    victim = ch->fighting;

  if (victim == nullptr)
  {
    ch->sendln("Stun whom?");
    return eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Aren't we funny today...");
    return eFAILURE;
  }

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Stunning a bladeshielded opponent would be suicide!");
    return eFAILURE;
  }

  if (!ch->isImmortalPlayer() && victim->isImmortalPlayer())
  {
    act_return ar = act("Due to immortal magic, you shake off $n's attempt to immobilize you.", ch, nullptr, victim, TO_VICT, 0);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }

    ar = act("Due to immortal magic, $N shakes off $n's attempt to immobilize them.", ch, nullptr, victim, TO_ROOM, NOTVICT);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }

    ar = act("$n's stun reflects back to them!", ch, nullptr, victim, TO_ROOM, 0);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }

    ar = act("Due to immortal magic, $N shakes off your attempt to immobilize them.", ch, nullptr, victim, TO_CHAR, 0);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }

    ar = act("Your stun is reflected back to yourself!", ch, nullptr, nullptr, TO_CHAR, 0);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }

    victim = ch;
  }

  if (victim->getLevel() == IMPLEMENTER)
  {
    ch->sendln("You gotta be kidding!");
    return eFAILURE;
  }

  if (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_HUGE))
  {
    ch->sendln("You cannot stun something that HUGE!");
    return eFAILURE;
  }

  if (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_SWARM))
  {
    ch->sendln("You cannot pick just one of them to stun!");
    return eFAILURE;
  }

  if (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_TINY))
  {
    act("$N's small size proves impossible to target a stunning blow upon!", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (!charge_moves(ch, SKILL_STUN))
    return eSUCCESS;

  if (isSet(victim->combat, COMBAT_BERSERK) && (IS_NPC(victim) || victim->has_skill(SKILL_BERSERK) > 80))
  {
    act_return ar = act("In your enraged state, you shake off $n's attempt to immobilize you.", ch, nullptr, victim, TO_VICT, 0);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }

    ar = act("$N shakes off $n's attempt to immobilize them.", ch, nullptr, victim, TO_ROOM, NOTVICT);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }

    ar = act("$N shakes off your attempt to immobilize them.", ch, nullptr, victim, TO_CHAR, 0);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }
    return eSUCCESS;
  }
  if ((!skill_success(ch, victim, SKILL_STUN) && GET_POS(victim) != position_t::SLEEPING) || do_frostshield(ch, victim))
  {
    act_return ar = act("$n attempts to hit you in your solar plexus!  You block $s attempt.", ch, nullptr, victim, TO_VICT, 0);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }

    ar = act("You attempt to hit $N in $s solar plexus...   YOU MISS!", ch, nullptr, victim, TO_CHAR, 0);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }

    ar = act("$n attempts to hit $N in $S solar plexus...   $e MISSES!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    retval = ar.retval;
    if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
    {
      return retval;
    }

    if (ch->has_skill(SKILL_STUN) > 35 && !number(0, 7))
    {
      ch->sendln("Your advanced knowledge of stun helps you to recover faster.");
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
    }
    else
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 4);
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_STUN, 0);
  }
  else
  {
    set_fighting(victim, ch);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 5);

    if (isSet(victim->combat, COMBAT_STUNNED) ||
        isSet(victim->combat, COMBAT_STUNNED2))
    {
      act_return ar = act("$n delivers another HARD BLOW to your solar plexus!", ch, nullptr, victim, TO_VICT, 0);
      retval = ar.retval;
      if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
      {
        return retval;
      }

      ar = act("You deliver another HARD BLOW into $N's solar plexus!", ch, nullptr, victim, TO_CHAR, 0);
      retval = ar.retval;
      if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
      {
        return retval;
      }

      ar = act("$n delivers another HARD BLOW into $N's solar plexus!", ch, nullptr, victim, TO_ROOM, NOTVICT);
      retval = ar.retval;
      if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
      {
        return retval;
      }

      if (number(0, 1))
      {
        victim->sendln("The hit knocks the sense back into you!");
        act_return ar = act("The hit knocks the sense back into $N and $E is no longer stunned!", ch, 0, victim, TO_ROOM, NOTVICT);
        retval = ar.retval;
        if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
        {
          return retval;
        }

        REMOVE_BIT(victim->combat, COMBAT_STUNNED);
        REMOVE_BIT(victim->combat, COMBAT_STUNNED2);
      }
      return damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_STUN, 0);
    }

    act_return ar;
    if (victim->affected_by_spell(SKILL_BATTLESENSE) &&
        number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      ar = act("$N's heightened battlesense sees your stun coming from a mile away and $E easily blocks it.", ch, 0, victim, TO_CHAR, 0);
      retval = ar.retval;
      if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
      {
        return retval;
      }

      ar = act("Your heightened battlesense sees $n's stun coming from a mile away and you easily block it.", ch, 0, victim, TO_VICT, 0);
      retval = ar.retval;
      if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
      {
        return retval;
      }

      ar = act("$N's heightened battlesense sees $n's stun coming from a mile away and $N easily blocks it.", ch, 0, victim, TO_ROOM, NOTVICT);
      retval = ar.retval;
      if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
      {
        return retval;
      }
    }
    else
    {
      ar = act("$n delivers a HARD BLOW into your solar plexus!  You are STUNNED!", ch, nullptr, victim, TO_VICT, 0);
      retval = ar.retval;
      if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
      {
        return retval;
      }

      ar = act("You deliver a HARD BLOW into $N's solar plexus!  $N is STUNNED!", ch, nullptr, victim, TO_CHAR, 0);
      retval = ar.retval;
      if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
      {
        return retval;
      }

      ar = act("$n delivers a HARD BLOW into $N's solar plexus!  $N is STUNNED!", ch, nullptr, victim, TO_ROOM, NOTVICT);
      retval = ar.retval;
      if (isSet(retval, eVICT_DIED) || isSet(retval, eCH_DIED))
      {
        return retval;
      }

      WAIT_STATE(victim, DC::PULSE_VIOLENCE * 2);
      if (GET_POS(victim) > position_t::STUNNED)
        victim->setStunned();
      ;
      SET_BIT(victim->combat, COMBAT_STUNNED);
    }
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_STUN, 0);
  }
  return retval;
}
