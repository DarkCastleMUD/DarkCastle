/************************************************************************
| $Id: cl_monk.cpp,v 1.42 2011/11/17 01:58:59 jhhudso Exp $
| cl_monk.C
| Description:  Monk skills.
*/

#include "DC/DC.h"

/************************************************************************
| OFFENSIVE commands.
*/
ReturnValue do_eagle_claw(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name;
  qint32 dam;
  qint32 retval;
  time_t time_raw_format;
  struct tm *ptr_time;

  if (!ch->canPerform(SKILL_EAGLE_CLAW, "Yooo are not pepared to use thees skeel, grasshoppa.\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  qint32 hands = {};
  if (ch->equipment[WEAR_WIELD])
    hands++;
  if (ch->equipment[WEAR_SECOND_WIELD])
    hands++;
  if (ch->equipment[WEAR_HOLD])
    hands++;
  if (ch->equipment[WEAR_HOLD2])
    hands++;
  if (ch->equipment[WEAR_SHIELD])
    hands++;
  if (ch->equipment[WEAR_LIGHT])
    hands++;

  if (hands > 1)
  {
    ch->sendln(u"You need a free hand to eagleclaw someone."_s);
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    if (ch->fighting)
      victim = ch->fighting;
    else
    {
      ch->sendln(u"You raise your hand in a claw and make strange bird noises."_s);
      return ReturnValue::eFAILURE;
    }
  }

  if (victim == ch)
  {
    ch->sendln(u"You lower your claw-shaped hand and scratch yourself gently."_s);
    return ReturnValue::eFAILURE;
  }

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln(u"Clawing a bladeshielded opponent would be suicide!"_s);
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return ReturnValue::eFAILURE;

  if (!charge_moves(ch, SKILL_EAGLE_CLAW))
    return ReturnValue::eSUCCESS;

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  if (!skill_success(ch, victim, SKILL_EAGLE_CLAW))
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_EAGLE_CLAW);
  else
  {
    // 1% bingo chance
    if (ch->dc_->number(1, 100) == 1 && !victim->isImmortalPlayer())
    {
      time(&time_raw_format);
      ptr_time = localtime(&time_raw_format);
      if (11 == ptr_time->tm_mon)
      {
        do_say(victim, "Laaaaaaast Christmas, I gave you my....");
      }
      act_to_room("$N blinks and stares glassy-eyed into the distance blissfully no longer aware of $n RIPPING OUT $S $B$4heart$R!", ch, 0, victim, NOTVICT);
      act_to_victim("You feel empty inside and full of heart-ache as if something important to you is missing.  Memories flash of your longing fo....", ch, 0, victim, 0);
      act_to_character("You slide your fingers between $N's ribs and give $S's left ventricle a tickle with your pinky before RIPPING OUT $S heart.", ch, 0, victim, 0);
      dam = 9999999;
    }
    else
    {
      dam = (GET_STR(ch) * 3) + (GET_DEX(ch) * 2) + dice(2, ch->getLevel()) + 100;
    }
    retval = damage(ch, victim, dam, TYPE_UNDEFINED, SKILL_EAGLE_CLAW);
  }

  return retval;
}

ReturnValue do_quivering_palm(CharacterPtr ch, const QString argument, cmd_t cmd)
{

  affected_type af;
  CharacterPtr victim;
  QString name;
  qint32 dam, retval;
  qint32 duration = 100;

  if (!ch->canPerform(SKILL_QUIVERING_PALM, "Stick to palming yourself for now bucko.\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_QUIVERING_PALM))
  {
    send_to_char("You can't perform such an ancient power more than "
                 "once a day!\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  qint32 hands = {};
  if (ch->equipment[WEAR_WIELD])
    hands++;
  if (ch->equipment[WEAR_SECOND_WIELD])
    hands++;
  if (ch->equipment[WEAR_SHIELD])
    hands++;

  if (hands > 1)
  {
    ch->sendln(u"You need at least one hand free to perform this!"_s);
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    if (ch->fighting)
      victim = ch->fighting;
    else
    {
      ch->sendln(u"Quivering palm whom?"_s);
      return ReturnValue::eFAILURE;
    }
  }

  if (victim == ch)
  {
    ch->sendln(u"Masturbate on your own time."_s);
    return ReturnValue::eFAILURE;
  }

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln(u"Palming a bladeshielded opponent would be suicide!"_s);
    return ReturnValue::eFAILURE;
  }

  if (isSet(dc_->world[ch->in_room].room_flags, NO_KI))
  {
    ch->sendln(u"You find yourself unable to focus your energy here."_s);
    return ReturnValue::eFAILURE;
  }

  if (GET_KI(ch) < 40 && ch->getLevel() < ARCHANGEL)
  {
    ch->sendln(u"You don't possess enough ki!"_s);
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return ReturnValue::eFAILURE;

  if (!charge_moves(ch, SKILL_QUIVERING_PALM))
    return ReturnValue::eSUCCESS;

  GET_KI(ch) -= 40;

  if (!skill_success(ch, victim, SKILL_QUIVERING_PALM))
  {
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_QUIVERING_PALM);
    duration = 6;
  }
  else
  {
    dam = victim->getHP() / 2;
    if (dam < 500)
      dam = 500;
    if (dam > 2000)
      dam = 2000;
    retval = damage(ch, victim, dam, TYPE_UNDEFINED, SKILL_QUIVERING_PALM);
    duration = 12;
  }
  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  af.type = SKILL_QUIVERING_PALM;
  af.duration = duration;
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  return retval;
}

ReturnValue do_stun(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name;
  qint32 retval;

  if (!ch->canPerform(SKILL_STUN, "Your lack of knowledge is stunning...\r\n"))
  {
    return ReturnValue::eFAILURE;
  }
  if (ch->getHP() < 25)
  {
    ch->sendln(u"You can't muster the energy for such an attack."_s);
    return ReturnValue::eFAILURE;
  }
  one_argument(argument, name);

  if (name[0] != '\0')
    victim = ch->get_char_room_vis(name);
  else
    victim = ch->fighting;

  if (victim == nullptr)
  {
    ch->sendln(u"Stun whom?"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln(u"Aren't we funny today..."_s);
    return ReturnValue::eFAILURE;
  }

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln(u"Stunning a bladeshielded opponent would be suicide!"_s);
    return ReturnValue::eFAILURE;
  }

  if (!ch->isImmortalPlayer() && victim->isImmortalPlayer())
  {
    act_return ar = act_to_victim("Due to immortal magic, you shake off $n's attempt to immobilize you.", ch, nullptr, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }

    ar = act_to_room("Due to immortal magic, $N shakes off $n's attempt to immobilize them.", ch, nullptr, victim, NOTVICT);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }

    ar = act_to_room("$n's stun reflects back to them!", ch, nullptr, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }

    ar = act_to_character("Due to immortal magic, $N shakes off your attempt to immobilize them.", ch, nullptr, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }

    ar = act_to_character("Your stun is reflected back to yourself!", ch, nullptr, nullptr, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }

    victim = ch;
  }

  if (victim->getLevel() == IMPLEMENTER)
  {
    ch->sendln(u"You gotta be kidding!"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_HUGE))
  {
    ch->sendln(u"You cannot stun something that HUGE!"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_SWARM))
  {
    ch->sendln(u"You cannot pick just one of them to stun!"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_TINY))
  {
    act_to_character("$N's small size proves impossible to target a stunning blow upon!", ch, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return ReturnValue::eFAILURE;

  if (!charge_moves(ch, SKILL_STUN))
    return ReturnValue::eSUCCESS;

  if (isSet(victim->combat, COMBAT_BERSERK) && (victim->isNonPlayer() || victim->has_skill(SKILL_BERSERK) > 80))
  {
    act_return ar = act_to_victim("In your enraged state, you shake off $n's attempt to immobilize you.", ch, nullptr, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }

    ar = act_to_room("$N shakes off $n's attempt to immobilize them.", ch, nullptr, victim, NOTVICT);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }

    ar = act_to_character("$N shakes off your attempt to immobilize them.", ch, nullptr, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }
    return ReturnValue::eSUCCESS;
  }
  if ((!skill_success(ch, victim, SKILL_STUN) && GET_POS(victim) != position_t::SLEEPING) || do_frostshield(ch, victim))
  {
    act_return ar = act_to_victim("$n attempts to hit you in your solar plexus!  You block $s attempt.", ch, nullptr, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }

    ar = act_to_character("You attempt to hit $N in $s solar plexus...   YOU MISS!", ch, nullptr, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }

    ar = act_to_room("$n attempts to hit $N in $S solar plexus...   $e MISSES!", ch, nullptr, victim, NOTVICT);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
    {
      return retval;
    }

    if (ch->has_skill(SKILL_STUN) > 35 && !number(0, 7))
    {
      ch->sendln(u"Your advanced knowledge of stun helps you to recover faster."_s);
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
    }
    else
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 4);
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_STUN);
  }
  else
  {
    set_fighting(victim, ch);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 5);

    if (isSet(victim->combat, COMBAT_STUNNED) ||
        isSet(victim->combat, COMBAT_STUNNED2))
    {
      act_return ar = act_to_victim("$n delivers another HARD BLOW to your solar plexus!", ch, nullptr, victim, 0);
      retval = ar.retval;
      if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
      {
        return retval;
      }

      ar = act_to_character("You deliver another HARD BLOW into $N's solar plexus!", ch, nullptr, victim, 0);
      retval = ar.retval;
      if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
      {
        return retval;
      }

      ar = act_to_room("$n delivers another HARD BLOW into $N's solar plexus!", ch, nullptr, victim, NOTVICT);
      retval = ar.retval;
      if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
      {
        return retval;
      }

      if (ch->dc_->number(0, 1))
      {
        victim->sendln(u"The hit knocks the sense back into you!"_s);
        act_return ar = act_to_room("The hit knocks the sense back into $N and $E is no longer stunned!", ch, 0, victim, NOTVICT);
        retval = ar.retval;
        if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
        {
          return retval;
        }

        REMOVE_BIT(victim->combat, COMBAT_STUNNED);
        REMOVE_BIT(victim->combat, COMBAT_STUNNED2);
      }
      return damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_STUN);
    }

    act_return ar;
    if (victim->affected_by_spell(SKILL_BATTLESENSE) &&
        dc_->number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      ar = act_to_character("$N's heightened battlesense sees your stun coming from a mile away and $E easily blocks it.", ch, 0, victim, 0);
      retval = ar.retval;
      if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
      {
        return retval;
      }

      ar = act_to_victim("Your heightened battlesense sees $n's stun coming from a mile away and you easily block it.", ch, 0, victim, 0);
      retval = ar.retval;
      if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
      {
        return retval;
      }

      ar = act_to_room("$N's heightened battlesense sees $n's stun coming from a mile away and $N easily blocks it.", ch, 0, victim, NOTVICT);
      retval = ar.retval;
      if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
      {
        return retval;
      }
    }
    else
    {
      ar = act_to_victim("$n delivers a HARD BLOW into your solar plexus!  You are STUNNED!", ch, nullptr, victim, 0);
      retval = ar.retval;
      if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
      {
        return retval;
      }

      ar = act_to_character("You deliver a HARD BLOW into $N's solar plexus!  $N is STUNNED!", ch, nullptr, victim, 0);
      retval = ar.retval;
      if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
      {
        return retval;
      }

      ar = act_to_room("$n delivers a HARD BLOW into $N's solar plexus!  $N is STUNNED!", ch, nullptr, victim, NOTVICT);
      retval = ar.retval;
      if (isSet(retval, ReturnValue::eVICT_DIED) || isSet(retval, ReturnValue::eCH_DIED))
      {
        return retval;
      }

      WAIT_STATE(victim, DC::PULSE_VIOLENCE * 2);
      if (GET_POS(victim) > position_t::STUNNED)
        victim->setStunned();
      ;
      SET_BIT(victim->combat, COMBAT_STUNNED);
    }
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_STUN);
  }
  return retval;
}
