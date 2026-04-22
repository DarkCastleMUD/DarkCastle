/*
 * ki.c - implementation of ki usage
 * Morcallen 12/18
 *
 */
/* $Id: ki.cpp,v 1.94 2014/07/04 22:00:04 jhhudso Exp $ */

#include "DC/DC.h"

const QList<ki_info_type> ki_info = {
    {/* 0 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 12, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, ki_blast, SKILL_INCREASE_HARD},

    {/* 1 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 12,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, ki_punch,
     SKILL_INCREASE_HARD},

    {/* 2 */
     3 * DC::PULSE_TIMER, position_t::STANDING, 5,
     TAR_IGNORE | TAR_CHAR_ROOM | TAR_SELF_ONLY, ki_sense,
     SKILL_INCREASE_MEDIUM},

    {/* 3 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 8,
     TAR_IGNORE, ki_storm, SKILL_INCREASE_HARD},

    {/* 4 */
     3 * DC::PULSE_TIMER, position_t::STANDING, 25,
     TAR_IGNORE | TAR_CHAR_ROOM | TAR_SELF_ONLY, ki_speed,
     SKILL_INCREASE_HARD},

    {/* 5 */
     3 * DC::PULSE_TIMER, position_t::RESTING, 8,
     TAR_IGNORE | TAR_CHAR_ROOM | TAR_SELF_ONLY, ki_purify,
     SKILL_INCREASE_MEDIUM},

    {/* 6 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 10,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT, ki_disrupt,
     SKILL_INCREASE_HARD},

    {/* 7 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 12,
     TAR_IGNORE, ki_stance, SKILL_INCREASE_EASY},

    {/* 8 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 20,
     TAR_IGNORE, ki_agility, SKILL_INCREASE_MEDIUM},

    {/* 9 */
     3 * DC::PULSE_TIMER, position_t::RESTING, 15,
     TAR_IGNORE, ki_meditation, SKILL_INCREASE_HARD},

    {/* 10 */
     3 * DC::PULSE_TIMER, position_t::STANDING, 1,
     TAR_CHAR_ROOM | TAR_SELF_NONO, ki_transfer, SKILL_INCREASE_HARD}

};

const QStringList ki = {
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
    "transfer"};

qint16 use_ki(CharacterPtr ch, qint32 kn);
bool ARE_GROUPED(CharacterPtr sub, CharacterPtr obj);

qint16 use_ki(CharacterPtr ch, qint32 kn)
{
  return (ki_info[kn].min_useski());
}

ReturnValue do_ki(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->getLevel() < ARCHANGEL && GET_CLASS(ch) != CLASS_MONK)
  {
    ch->sendln(u"You are unable to control your ki in this way!"_s);
    return ReturnValue::eFAILURE;
  }
  /*
   if ((isSet(dc_->world[ch->in_room].room_flags, SAFE)) && (ch->getLevel() < IMPLEMENTER)) {
   send_to_char("You feel at peace, calm, relaxed, one with yourself and "
   "the universe.\r\n", ch);
   return ReturnValue::eFAILURE;
   }*/

  auto arguments = QString(argument).trimmed().toLower().split('\'');
  auto ki_skill = arguments.value(1);
  auto target_name = arguments.value(2);

  if (arguments.isEmpty())
  {
    ch->sendln(u"Yes, but WHAT would you like to do?"_s);
    return ReturnValue::eFAILURE;
  }

  auto spl = ki.indexOf(ki_skill);
  if (spl < 0)
  {
    ch->sendln(u"You cannot harness that energy!"_s);
    return ReturnValue::eFAILURE;
  }

  if (isSet(dc_->world[ch->in_room].room_flags, SAFE) && (ch->getLevel() < IMPLEMENTER) && spl != KI_SENSE && spl != KI_SPEED && spl != KI_PURIFY && spl != KI_STANCE && spl != KI_AGILITY && spl != KI_MEDITATION)
  {
    ch->sendln(u"You feel at peace, calm, relaxed, one with yourself and the universe."_s);
    return ReturnValue::eFAILURE;
  }

  auto learned = ch->has_skill(spl + KI_OFFSET);
  if (!learned)
  {
    ch->sendln(u"You do not know that ki power!"_s);
    return ReturnValue::eFAILURE;
  }

  if (ki_info[spl].ki_pointer())
  {
    if (GET_POS(ch) < ki_info[spl].minimum_position() || (spl == KI_MEDITATION && (GET_POS(ch) == position_t::FIGHTING || GET_POS(ch) <= position_t::SLEEPING)))
    {
      switch (GET_POS(ch))
      {
      case position_t::SLEEPING:
        ch->sendln(u"You dream of wonderful ki powers."_s);
        break;
      case position_t::RESTING:
        ch->sendln(u"You cannot harness that much energy while resting!"_s);
        break;
      case position_t::SITTING:
        ch->sendln(u"You can't do this sitting!"_s);
        break;
      case position_t::FIGHTING:
        ch->sendln(u"This is a peaceful ki power."_s);
        break;
      default:
        ch->sendln(u"It seems like you're in a pretty bad shape!"_s);
        break;
      }
      return ReturnValue::eFAILURE;
    }

    auto target_ok = false;
    CharacterPtr tar_char;
    if (!isSet(ki_info[spl].targets(), TAR_IGNORE))
    {
      if (!target_name.isEmpty())
      {
        if (isSet(ki_info[spl].targets(), TAR_CHAR_ROOM))
          if ((tar_char = ch->get_char_room_vis(target_name)) != nullptr)
            target_ok = true;

        if (!target_ok && isSet(ki_info[spl].targets(), TAR_SELF_ONLY))
          if (ch->name() == target_name)
          {
            tar_char = ch;
            target_ok = true;
          } // of !target_ok
      }
      else /* No argument was typed */
      {
        if (isSet(ki_info[spl].targets(), TAR_FIGHT_VICT))
          if (ch->fighting)
            if ((ch->fighting)->in_room == ch->in_room)
            {
              tar_char = ch->fighting;
              target_ok = true;
            }

        if (!target_ok && isSet(ki_info[spl].targets(), TAR_SELF_ONLY))
        {
          tar_char = ch;
          target_ok = true;
        }
      }
    }

    if (isSet(ki_info[spl].targets(), TAR_IGNORE))
      target_ok = true;

    if (target_ok != true)
    {
      if (!target_name.isEmpty())
        ch->sendln(u"Nobody here by that name."_s);
      else /* No arguments were given */
        ch->sendln(u"Whom should the power be used upon?"_s);
      return ReturnValue::eFAILURE;
    }
    else if (target_ok)
    {
      if ((tar_char == ch) && isSet(ki_info[spl].targets(), TAR_SELF_NONO))
      {
        ch->sendln(u"You cannot use this power on yourself."_s);
        return ReturnValue::eFAILURE;
      }
      else if ((tar_char != ch) && isSet(ki_info[spl].targets(), TAR_SELF_ONLY))
      {
        ch->sendln(u"You can only use this power upon yourself."_s);
        return ReturnValue::eFAILURE;
      }
      else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tar_char))
      {
        ch->sendln(u"You are afraid that it might harm your master."_s);
        return ReturnValue::eFAILURE;
      }
    }

    /* I put ths in to stop those crashes.  Morc: find your own bug ;)
     * -Sadus
     * This has hence been fixed. - Pir
     */
    if (!isSet(ki_info[spl].targets(), TAR_IGNORE))
      if (!tar_char)
      {
        dc_->logentry(u"Dammit Morc, fix that null tar_char thing in ki"_s, IMPLEMENTER, DC::LogChannel::LOG_BUG);
        send_to_char("If you triggered this message, you almost crashed the\r\ngame.  Tell a god what you did immediately.\r\n", ch);
        return ReturnValue::eFAILURE;
      }

    /* crasher right here */
    if (isSet(dc_->world[ch->in_room].room_flags, NO_KI))
    {
      ch->sendln(u"You find yourself unable to focus your energy here."_s);
      return ReturnValue::eFAILURE;
    }

    if (!isSet(ki_info[spl].targets(), TAR_IGNORE))
      if (!can_attack(ch) || !can_be_attacked(ch, tar_char))
        return ReturnValue::eFAILURE;

    if (ch->getLevel() < ARCHANGEL && GET_KI(ch) < use_ki(ch, spl))
    {
      ch->sendln(u"You do not have enough ki!"_s);
      return ReturnValue::eFAILURE;
    }

    WAIT_STATE(ch, ki_info[spl].beats());

    if ((ki_info[spl].ki_pointer() == nullptr) && spl > 0)
      ch->sendln(u"Sorry, this power has not yet been implemented."_s);
    else
    {
      if (!skill_success(ch, tar_char, spl + KI_OFFSET) && !isSet(dc_->world[ch->in_room].room_flags, SAFE))
      {
        ch->sendln(u"You lost your concentration!"_s);
        GET_KI(ch) -= use_ki(ch, spl) / 2;
        WAIT_STATE(ch, ki_info[spl].beats() / 2);

        return ReturnValue::eSUCCESS;
      }

      if (!isSet(ki_info[spl].targets(), TAR_IGNORE))
        if (!tar_char || (ch->in_room != tar_char->in_room))
        {
          ch->sendln(u"Whom should the power be used upon?"_s);
          return ReturnValue::eFAILURE;
        }

      /* Stop abusing your betters  */
      if (!isSet(ki_info[spl].targets(), TAR_IGNORE))
        if (tar_char->isPlayer() && (ch->getLevel() > ARCHANGEL) && (tar_char->getLevel() > ch->getLevel()))
        {
          ch->sendln(u"That just might annoy them!"_s);
          return ReturnValue::eFAILURE;
        }

      /* Imps ignore safe flags  */
      if (!isSet(ki_info[spl].targets(), TAR_IGNORE))
        if (isSet(dc_->world[ch->in_room].room_flags, SAFE) && ch->isPlayer() && (ch->getLevel() == IMPLEMENTER))
        {
          tar_char->sendln(u"There is no safe haven from an angry IMPLEMENTER!"_s);
        }

      ch->sendln(u"Ok."_s);
      GET_KI(ch) -= use_ki(ch, spl);

      return ((*ki_info[spl].ki_pointer())(ch->getLevel(), ch, argument, tar_char));
    }
    return ReturnValue::eFAILURE;
  }
  return ReturnValue::eFAILURE;
}

void reduce_ki(CharacterPtr ch, qint32 type)
{
  qint32 amount = {};

  amount += ch->getLevel() / type; /* the higher the response
                                    * the lower the divisor */

  amount -= dice(1, 10);
  if (amount < 0)
    amount = {};
  GET_KI(ch) -= amount;
}

qint32 Character::ki_gain_lookup(void)
{
  qint32 gain;

  /* gain 1 - 7 depedant on level */
  gain = GET_CLASS(this) == CLASS_MONK ? (qint32)(max_ki * 0.04) : (qint32)(max_ki * 0.05); /*(getLevel() / 8) + 1;*/
  gain += ki_regen;

  // Normalize these so we dont underun the array below
  qint32 norm_wis = MAX(0, GET_WIS(this));
  qint32 norm_int = MAX(0, GET_INT(this));

  if (GET_CLASS(this) == CLASS_MONK)
  {
    gain += wis_app[norm_wis].ki_regen;
  }
  else if (GET_CLASS(this) == CLASS_BARD)
  {
    gain += int_app[norm_int].ki_regen;
  }

  gain += age().year / 25;

  if (isSet(dc_->world[in_room].room_flags, SAFE) || check_make_camp(in_room))
    gain = (qint32)(gain * 1.25);

  qint32 multiplyer = 1;
  switch (GET_POS(this))
  {
  case position_t::SLEEPING:
    multiplyer = 3;
    break;
  case position_t::RESTING:
    multiplyer = 2;
    break;
  case position_t::SITTING:
    multiplyer = 2;
    break;
  default:
    multiplyer = 1;
    break;
  }
  gain *= multiplyer;

  return MAX(gain, 1);
}

qint32 ki_blast(quint8 level, CharacterPtr ch, QString arg, CharacterPtr vict)
{
  qint32 success = {};
  qint32 exit = dc_->number(0, 5); /* Chooses an exit */

  extern QStringList dirswards;
  QString buf;

  if (!vict)
  {
    dc_->logentry(u"Serious problem in ki blast!"_s, ANGEL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eINTERNAL_ERROR;
  }

  success += ch->getLevel();

  if (vict->weight < 50)
    success += 50;
  else if (vict->weight >= 50 && vict->weight < 120)
    success += 20;
  else if (vict->weight >= 200 && vict->weight < 255)
    success -= 10;
  else
    success -= 20; /* more than 300 pounds?! */

  if (ch->dc_->number(1, 101) > success || vict->affected_by_spell(SPELL_IRON_ROOTS)) /* 101 is complete failure */
  {
    act_to_room("$n fails to blast $N!", ch, 0, vict, NOTVICT);
    act_to_character("You fail to blast $N!", ch, 0, vict, 0);
    act_to_victim("$n finds that you are hard to blast!", ch, 0, vict, 0);
    if (!vict->fighting && vict->isNonPlayer())
      return attack(vict, ch, TYPE_UNDEFINED);
    return ReturnValue::eSUCCESS;
  }

  if (CAN_GO(vict, exit) &&
      !isSet(dc_->world[EXIT(vict, exit)->to_room].room_flags, IMP_ONLY) &&
      !isSet(dc_->world[EXIT(vict, exit)->to_room].room_flags, NO_TRACK) &&
      (!IS_AFFECTED(vict, AFF_CHAMPION) || champion_can_go(EXIT(vict, exit)->to_room)) &&
      class_can_go(GET_CLASS(vict), EXIT(vict, exit)->to_room))
  {
    dc_sprintf(buf, "$N is blasted out of the room %s by $n!", dirswards[exit]);
    act_to_room(buf, ch, 0, vict, NOTVICT);
    dc_sprintf(buf, "You watch as $N goes flailing out of the room %s!", dirswards[exit]);
    act_to_character(buf, ch, 0, vict, 0);
    act("$n's vicious blast throws you out of the room!", ch, 0,
        vict, TO_VICT, 0);

    if (vict->fighting)
    {
      if (vict->isNonPlayer())
      {
        vict->add_memory(qPrintable(ch->name()), 'h');
        remove_memory(vict, 'f');
      }
      CharacterPtr tmp;
      for (tmp = dc_->world[ch->in_room].people_; tmp; tmp = tmp->next_in_room)
        if (tmp->fighting == vict)
          stop_fighting(tmp);
      stop_fighting(vict);
    }

    move_char(vict, (dc_->world[(ch)->in_room].dir_option[exit])->to_room);
    vict->setSitting();
    SET_BIT(vict->combat, COMBAT_BASH2);
    return ReturnValue::eSUCCESS;
  }
  else /* There is no exit there */
  {
    QString buf, name;
    qint32 prev = vict->getHP();

    dc_strcpy(name, qPrintable(vict->shortdesc_or_name()));
    qint32 retval = damage(ch, vict, 100, TYPE_KI, KI_OFFSET + KI_BLAST);
    vict->setSitting();
    SET_BIT(vict->combat, COMBAT_BASH2);
    if (!SOMEONE_DIED(retval) && !vict->fighting && vict->isNonPlayer())
      return attack(vict, ch, TYPE_UNDEFINED);
    return retval;
  }
  /* still here?  It was unsuccessful */
  return ReturnValue::eSUCCESS;
}

qint32 ki_punch(quint8 level, CharacterPtr ch, QString arg, CharacterPtr vict)
{
  if (!vict)
  {
    dc_->logf(ANGEL, DC::LogChannel::LOG_BUG, "Serious problem in ki punch!", ANGEL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eINTERNAL_ERROR;
  }

  set_cantquit(ch, vict);
  auto dam = dc_->number(500, 700);
  auto manadam = GET_MANA(vict) / 4;
  qint32 retval = ReturnValue::eFAILURE;

  manadam = MAX(150, manadam);
  manadam = MIN(750, manadam);
  if (vict->getHP() < 500000)
  {
    auto success_chance = (ch->getLevel() / 5) + (ch->has_skill(KI_OFFSET + KI_PUNCH) * 0.75) - (vict->getLevel() / 5);
    if (ch->dc_->number(1, 101) < success_chance)

    {
      GET_MANA(vict) -= manadam;
      retval = damage(ch, vict, dam, TYPE_UNDEFINED, KI_OFFSET + KI_PUNCH);
      return retval;
    }
    else
    {
      retval = damage(ch, vict, 0, TYPE_UNDEFINED, KI_OFFSET + KI_PUNCH);

      ch->removeHP((1 / 8) * (GET_MAX_HIT(ch)));
      WAIT_STATE(ch, DC::PULSE_VIOLENCE);
      if (!vict->fighting)
        return attack(vict, ch, TYPE_UNDEFINED);
    }
  } // end of < 5000

  else
  {
    ch->sendln(u"Your opponent has too many hit points!"_s);
    if (!vict->fighting)
      return attack(vict, ch, TYPE_UNDEFINED);
  }

  return ReturnValue::eSUCCESS; // shouldn't get here
}

qint32 ki_sense(quint8 level, CharacterPtr ch, QString arg, CharacterPtr vict)
{
  affected_type af;
  if (IS_AFFECTED(ch, AFF_INFRARED))
    return ReturnValue::eSUCCESS;
  if (ch->affected_by_spell(SPELL_INFRAVISION))
    return ReturnValue::eSUCCESS;

  af.type = SPELL_INFRAVISION;
  af.modifier = {};
  af.location = APPLY_NONE;
  af.duration = level;
  af.bitvector = AFF_INFRARED;
  affect_to_char(vict, &af);
  vict->sendln(u"You feel your sense become more acute."_s);

  return ReturnValue::eSUCCESS;
}

qint32 ki_storm(quint8 level, CharacterPtr ch, QString arg, CharacterPtr vict)
{
  qint32 dam;
  qint32 retval;
  CharacterPtr tmp_victim, temp;

  dam = dc_->number(135, 165);
  //  ch->sendln(u"Your wholeness of spirit purges the souls of those around you!"_s);
  //  act("$n's eyes flash as $e pools the energy within $m!\r\nA burst of energy slams into you!\r\n",
  qint32 room = ch->in_room;
  for (tmp_victim = dc_->world[ch->in_room].people_; tmp_victim && tmp_victim != (CharacterPtr)0x95959595; tmp_victim = temp)
  {
    temp = tmp_victim->next_in_room;
    if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
        (!ARE_GROUPED(ch, tmp_victim)) && can_be_attacked(ch, tmp_victim))
    {
      retval = damage(ch, tmp_victim, dam, TYPE_KI,
                      KI_OFFSET + KI_STORM, 0);
      if (isSet(retval, ReturnValue::eCH_DIED))
        return retval;
      act_to_room("A burst of energy slams into you!", ch, 0, 0, 0);
    } // else
    //		if (dc_->world[ch->in_room].zone == dc_->world[tmp_victim->in_room].zone)
    //	tmp_victim->sendln(u"A crackle of energy echos past you."_s);
  }
  qint32 dir = dc_->number(0, 5), distance = dc_->number(1, 3), i;
  if (room > 0)
    for (i = {}; i < distance; i++)
    {
      if (!IS_EXIT(room, dir) || !IS_OPEN(room, dir))
        break;
      room = EXIT_TO(room, dir);
      if (room == DC::NOWHERE)
        break;
      for (tmp_victim = dc_->world[room].people_; tmp_victim; tmp_victim = tmp_victim->next_in_room)
        tmp_victim->sendln(u"A crackle of energy echoes past you."_s);
    }
  if (ch->dc_->number(1, 4) == 4 && !ch->fighting)
  {
    QString dammsg;
    dc_sprintf(dammsg, "$B%d$R", dam);
    if (dam + ch->getHP() > GET_MAX_HIT(ch))
      dam = GET_MAX_HIT(ch) - ch->getHP();
    ch->addHP(dam);
    send_damage("The flash of energy surges within you for | life!", ch, 0, 0, dammsg, "The flash of energy surges within you!", TO_CHAR);
  }
  WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  return ReturnValue::eSUCCESS;
}

qint32 ki_speed(quint8 level, CharacterPtr ch, QString arg, CharacterPtr vict)
{
  affected_type af;

  if (!vict)
  {
    dc_->logentry(u"Null victim sent to ki speed"_s, ANGEL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eINTERNAL_ERROR;
  }

  if (vict->affected_by_spell(SPELL_HASTE))
    return ReturnValue::eSUCCESS;

  af.type = SPELL_HASTE;
  af.duration = ch->has_skill(KI_OFFSET + KI_SPEED) / 15;
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = AFF_HASTE;

  affect_to_char(vict, &af);

  af.type = SPELL_HASTE;
  af.duration = ch->has_skill(KI_OFFSET + KI_SPEED) / 15;
  af.modifier = -ch->has_skill(KI_OFFSET + KI_SPEED) / 4;
  af.location = APPLY_AC;
  af.bitvector = -1;

  affect_to_char(vict, &af);

  vict->sendln(u"You feel a quickening in your limbs!"_s);
  return ReturnValue::eSUCCESS;
}

qint32 ki_purify(quint8 level, CharacterPtr ch, QString arg, CharacterPtr vict)
{
  if (!vict)
  {
    dc_->logentry(u"Null victim sent to ki purify"_s, ANGEL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eINTERNAL_ERROR;
  }
  if (!arg)
  {
    ch->send(u"You can only purify poison, blindness, alcohol or weaken."_s);
    return ReturnValue::eFAILURE;
  }
  if (!str_cmp(arg, "poison"))
  {
    if (vict->affected_by_spell(SPELL_POISON))
      affect_from_char(vict, SPELL_POISON);
    else
    {
      ch->sendln(u"That taint is not present."_s);
      return ReturnValue::eFAILURE;
    }
    ch->sendln(u"You purge the poison."_s);
  }
  else if (!str_cmp(arg, "blindness"))
  {
    if (vict->affected_by_spell(SPELL_BLINDNESS))
      affect_from_char(vict, SPELL_BLINDNESS);
    else
    {
      ch->sendln(u"That taint is not present."_s);
      return ReturnValue::eFAILURE;
    }
    ch->sendln(u"You purge the blindness."_s);
  }
  else if (!str_cmp(arg, "weaken"))
  {
    if (vict->affected_by_spell(SPELL_WEAKEN))
      affect_from_char(vict, SPELL_WEAKEN);
    else
    {
      ch->sendln(u"That taint is not present."_s);
      return ReturnValue::eFAILURE;
    }
    ch->sendln(u"You purge the poison."_s);
  }
  else if (!str_cmp(arg, "alcohol"))
  {
    if (GET_COND(ch, DRUNK) > 0)
      gain_condition(vict, DRUNK, -GET_COND(ch, DRUNK));
    else
    {
      ch->sendln(u"That taint is not present."_s);
      return ReturnValue::eFAILURE;
    }
    ch->sendln(u"You purge the alcohol."_s);
  }
  else
  {
    ch->sendln(u"You cannot purge that."_s);
  }
  return ReturnValue::eSUCCESS;
}

qint32 ki_disrupt(quint8 level, CharacterPtr ch, QString arg, CharacterPtr victim)
{
  if (!victim)
  {
    dc_->logentry(u"Serious problem in ki disrupt!"_s, ANGEL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eINTERNAL_ERROR;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  set_cantquit(ch, victim);

  bool disrupt_bingo = false;
  qint32 success_chance = {};
  qint32 learned = ch->has_skill(KI_OFFSET + KI_DISRUPT);

  if (learned > 85)
  {
    level_diff_t level_difference = ch->getLevel() - victim->getLevel();

    if (level_difference >= 0)
    {
      success_chance = 6;
    }
    else if (level_difference >= -20)
    {
      success_chance = 5;
    }
    else if (level_difference >= -100)
    {
      success_chance = 4;
    }
    else
    {
      success_chance = 1;
    }

    if (success_chance >= dc_->number(1, 100))
    {
      disrupt_bingo = true;
    }
  }

  if (disrupt_bingo == true)
  {
    act_to_victim("$n slams a bolt of focused ki energy into the flow of magic all around you!", ch, 0, victim, 0);
    act_to_room("$n focuses a blast of ki to disrupt the flow of magic all around $N!", ch, 0, victim, 0);
    ch->sendln(u"You focus your ki to disrupt the flow of magic all around your opponent!"_s);
  }
  else
  {
    act_to_victim("$n slams a bolt of focused ki energy into the flow of magic around you!", ch, 0, victim, 0);
    act_to_room("$n focuses a blast of ki to disrupt the flow of magic around $N!", ch, 0, victim, 0);
    ch->sendln(u"You focus your ki to disrupt the flow of magic around your opponent!"_s);
  }

  if (ISSET(victim->affected_by, AFF_GOLEM))
  {
    ch->sendln(u"The golem seems to shrug off your ki disrupt attempt!"_s);
    act_to_room("The golem seems to ignore $n's disrupting energy!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }
  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_NODISPEL))
  {
    act_to_room("$N seems to ignore $n's disrupting energy!", ch, 0, victim, 0);
    act_to_character("$N seems to ignore your disrupting energy!", ch, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  qint32 savebonus = {};
  if (learned < 41)
  {
    savebonus = 35;
  }
  else if (learned < 61)
  {
    savebonus = 30;
  }
  else if (learned < 81)
  {
    savebonus = 25;
  }
  else
  {
    savebonus = 20;
  }

  // Players are easier to disrupt
  if (victim->isPlayer())
  {
    savebonus -= 10;
  }

  // Check if caster gets a bonus against this victim
  affected_typePtr af = victim->affected_by_spell(KI_DISRUPT + KI_OFFSET);
  if (af)
  {
    // We've KI_DISRUPTED the victim and failed before so we get a bonus
    if (af->caster == QString(qPrintable(ch->name())))
    {
      savebonus -= af->modifier;
    }
    else
    {
      // Some other caster's KI_DISRUPT was on the victim, removing it
      affect_from_char(victim, KI_DISRUPT + KI_OFFSET);
      af = {};
    }
  }

  qint32 retval = {};

  if (ch->dc_->number(1, 100) <= get_saves(victim, SAVE_TYPE_MAGIC) + savebonus && level != ch->getLevel() - 1)
  {
    // We've failed this time, so we'll make it easier for next time
    if (af)
    {
      // We've failed before
      af->modifier += 1 + (learned / 20);
    }
    else
    {
      // This is the first time we've failed
      affected_type newaf;
      newaf.type = KI_DISRUPT + KI_OFFSET;
      newaf.duration = -1;
      newaf.modifier = 1 + (learned / 20);
      newaf.location = APPLY_NONE;
      newaf.bitvector = -1;
      newaf.caster = QString(qPrintable(ch->name()));

      affect_to_char(victim, &newaf);
    }

    act("$N resists your attempt to disrupt magic!", ch, nullptr, victim,
        TO_CHAR, 0);
    act("$N resists $n's attempt to disrupt magic!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to disrupt magic!", ch, nullptr, victim, TO_VICT, 0);

    if (victim->isNonPlayer() && (!victim->fighting) &&
        GET_POS(ch) > position_t::SLEEPING)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }

    return ReturnValue::eFAILURE;
  }

  // We have success so if af is set then the victim had a ki_disupt
  // bonus set. We will remove it.
  if (af)
  {
    affect_from_char(victim, KI_DISRUPT + KI_OFFSET);
  }

  // Disrupt bingo chance
  if (disrupt_bingo)
  {
    if (victim->affected_by_spell(SPELL_SANCTUARY) ||
        IS_AFFECTED(victim, AFF_SANCTUARY))
    {
      affect_from_char(victim, SPELL_SANCTUARY);
      REMBIT(victim->affected_by, AFF_SANCTUARY);
      act_to_victim("You don't feel so invulnerable anymore.", ch, 0, victim, 0);
      act_to_room("The $B$7white glow$R around $n's body fades.", victim, 0, 0, 0);
    }
    if (victim->affected_by_spell(SPELL_PROTECT_FROM_EVIL))
    {
      affect_from_char(victim, SPELL_PROTECT_FROM_EVIL);
      act_to_victim("Your protection from evil has been disrupted!", ch, 0, victim, 0);
      act_to_room("The dark, $6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, 0);
    }

    if (victim->affected_by_spell(SPELL_HASTE))
    {
      affect_from_char(victim, SPELL_HASTE);
      act_to_victim("Your magically enhanced speed has been disrupted!", ch, 0, victim, 0);
      act_to_room("$n's actions slow to their normal speed.", victim, 0, 0, 0);
    }

    if (victim->affected_by_spell(SPELL_STONE_SHIELD))
    {
      affect_from_char(victim, SPELL_STONE_SHIELD);
      act_to_victim("Your shield of swirling stones falls harmlessly to the ground!", ch, 0, victim, 0);
      act_to_room("The shield of stones swirling about $n's body fall to the ground!", victim, 0, 0, 0);
    }

    if (victim->affected_by_spell(SPELL_GREATER_STONE_SHIELD))
    {
      affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);
      act_to_victim("Your shield of swirling stones falls harmlessly to the ground!", ch, 0, victim, 0);
      act_to_room("The shield of stones swirling about $n's body falls to the ground!", victim, 0, 0, 0);
    }

    if (IS_AFFECTED(victim, AFF_FROSTSHIELD))
    {
      REMBIT(victim->affected_by, AFF_FROSTSHIELD);
      act_to_victim("Your shield of $B$3frost$R melts into nothing!.", ch, 0, victim, 0);
      act_to_room("The $B$3frost$R encompassing $n's body melts away.", victim, 0, 0, 0);
    }

    if (victim->affected_by_spell(SPELL_LIGHTNING_SHIELD))
    {
      affect_from_char(victim, SPELL_LIGHTNING_SHIELD);
      act_to_victim("Your crackling shield of $B$5electricity$R vanishes!", ch, 0, victim, 0);
      act_to_room("The $B$5electricity$R crackling around $n's body fades away.", victim, 0, 0, 0);
    }

    if (victim->affected_by_spell(SPELL_FIRESHIELD) || IS_AFFECTED(victim, AFF_FIRESHIELD))
    {
      REMBIT(victim->affected_by, AFF_FIRESHIELD);
      affect_from_char(victim, SPELL_FIRESHIELD);
      act_to_victim("Your $B$4flames$R have been extinguished!", ch, 0, victim, 0);
      act_to_room("The $B$4flames$R encompassing $n's body are extinguished!", victim, 0, 0, 0);
    }
    if (victim->affected_by_spell(SPELL_ACID_SHIELD))
    {
      affect_from_char(victim, SPELL_ACID_SHIELD);
      act_to_victim("Your shield of $B$2acid$R dissolves to nothing!", ch, 0, victim, 0);
      act_to_room("The $B$2acid$R swirling about $n's body dissolves to nothing!", victim, 0, 0, 0);
    }
    if (victim->affected_by_spell(SPELL_PROTECT_FROM_GOOD))
    {
      affect_from_char(victim, SPELL_PROTECT_FROM_GOOD);
      act_to_victim("Your protection from good has been disrupted!", ch, 0, victim, 0);
      act_to_room("The light, $B$6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, 0);
    }

    if (victim->isNonPlayer() && !victim->fighting)
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
  QList<affected_type> aff_list;

  // Since we're looking for either these 3 affects OR the spells that cause them
  // we're keeping a track of which is found so we don't mark them twice
  bool frostshieldFound = false, fireshieldFound = false, sanctuaryFound = false;

  for (affected_typePtr curr = victim->affected; curr; curr = curr->next)
  {
    switch (curr->type)
    {
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

  if (IS_AFFECTED(victim, AFF_FROSTSHIELD) && !frostshieldFound)
  {
    localaff.type = SPELL_FROSTSHIELD;
    aff_list.push_back(localaff);
  }

  if (IS_AFFECTED(victim, AFF_FIRESHIELD) && !fireshieldFound)
  {
    localaff.type = SPELL_FIRESHIELD;
    aff_list.push_back(localaff);
  }

  if (IS_AFFECTED(victim, AFF_SANCTUARY) && !sanctuaryFound)
  {
    localaff.type = SPELL_SANCTUARY;
    aff_list.push_back(localaff);
  }

  // Nothing applicable found to be removed
  if (aff_list.size() < 1)
  {
    return ReturnValue::eFAILURE;
  }

  // Pick the lucky spell/affect to be removed
  quint64 i = dc_->number((quint64)0, (quint64)aff_list.size() - 1);

  try
  {
    af = &aff_list.at(i);
  }
  catch (...)
  {
    return ReturnValue::eFAILURE;
  }

  if (af->type == SPELL_SANCTUARY)
  {
    affect_from_char(victim, SPELL_SANCTUARY);
    REMBIT(victim->affected_by, AFF_SANCTUARY);
    act_to_victim("You don't feel so invulnerable anymore.", ch, 0, victim, 0);
    act_to_room("The $B$7white glow$R around $n's body fades.", victim, 0, 0, 0);
  }

  if (af->type == SPELL_PROTECT_FROM_EVIL)
  {
    affect_from_char(victim, SPELL_PROTECT_FROM_EVIL);
    act_to_victim("Your protection from evil has been disrupted!", ch, 0, victim, 0);
    act_to_room("The dark, $6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, 0);
  }

  if (af->type == SPELL_HASTE)
  {
    affect_from_char(victim, SPELL_HASTE);
    act_to_victim("Your magically enhanced speed has been disrupted!", ch, 0, victim, 0);
    act_to_room("$n's actions slow to their normal speed.", victim, 0, 0, 0);
  }

  if (af->type == SPELL_STONE_SHIELD)
  {
    affect_from_char(victim, SPELL_STONE_SHIELD);
    act_to_victim("Your shield of swirling stones falls harmlessly to the ground!", ch, 0, victim, 0);
    act_to_room("The shield of stones swirling about $n's body fall to the ground!", victim, 0, 0, 0);
  }

  if (af->type == SPELL_GREATER_STONE_SHIELD)
  {
    affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);
    act_to_victim("Your shield of swirling stones falls harmlessly to the ground!", ch, 0, victim, 0);
    act_to_room("The shield of stones swirling about $n's body falls to the ground!", victim, 0, 0, 0);
  }

  if (af->type == SPELL_FROSTSHIELD)
  {
    affect_from_char(victim, SPELL_FROSTSHIELD);
    REMBIT(victim->affected_by, AFF_FROSTSHIELD);
    act_to_victim("Your shield of $B$3frost$R melts into nothing!.", ch, 0, victim, 0);
    act_to_room("The $B$3frost$R encompassing $n's body melts away.", victim, 0, 0, 0);
  }

  if (af->type == SPELL_LIGHTNING_SHIELD)
  {
    affect_from_char(victim, SPELL_LIGHTNING_SHIELD);
    act_to_victim("Your crackling shield of $B$5electricity$R vanishes!", ch, 0, victim, 0);
    act_to_room("The $B$5electricity$R crackling around $n's body fades away.", victim, 0, 0, 0);
  }

  if (af->type == SPELL_FIRESHIELD)
  {
    REMBIT(victim->affected_by, AFF_FIRESHIELD);
    affect_from_char(victim, SPELL_FIRESHIELD);
    act_to_victim("Your $B$4flames$R have been extinguished!", ch, 0, victim, 0);
    act_to_room("The $B$4flames$R encompassing $n's body are extinguished!", victim, 0, 0, 0);
  }

  if (af->type == SPELL_ACID_SHIELD)
  {
    affect_from_char(victim, SPELL_ACID_SHIELD);
    act_to_victim("Your shield of $B$2acid$R dissolves to nothing!", ch, 0, victim, 0);
    act_to_room("The $B$2acid$R swirling about $n's body dissolves to nothing!", victim, 0, 0, 0);
  }

  if (af->type == SPELL_PROTECT_FROM_GOOD)
  {
    affect_from_char(victim, SPELL_PROTECT_FROM_GOOD);
    act_to_victim("Your protection from good has been disrupted!", ch, 0, victim, 0);
    act_to_room("The light, $B$6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, 0);
  }

  if (victim->isNonPlayer() && !victim->fighting)
  {
    retval = attack(victim, ch, 0);
    SWAP_CH_VICT(retval);
    return retval;
  }
  return ReturnValue::eSUCCESS;
}

qint32 ki_stance(quint8 level, CharacterPtr ch, QString arg, CharacterPtr vict)
{
  affected_type af;

  if (ch->affected_by_spell(KI_STANCE + KI_OFFSET))
  {
    ch->sendln(u"You focus your ki to harden your stance, but your body is still recovering from last time..."_s);
    return ReturnValue::eFAILURE;
  }

  act("$n assumes a defensive stance and attempts to absorb the energies that surround $m.",
      ch, 0, vict, TO_ROOM, 0);
  ch->sendln(u"You take a defensive stance and try to aborb the energies seeking to harm you."_s);

  // chance of failure - can be meta'd past that point though
  if (ch->dc_->number(1, 100) > (GET_DEX(ch) * 4))
  {
    ch->sendln(u"You accidently stub your toe and fall out of the defenseive stance."_s);
    return ReturnValue::eSUCCESS;
  }

  SET_BIT(ch->combat, COMBAT_MONK_STANCE);

  af.type = KI_STANCE + KI_OFFSET;
  af.duration = 50 - ((ch->getLevel() / 5) * 2);
  af.modifier = 1;
  af.location = APPLY_NONE;
  af.bitvector = -1;

  affect_to_char(ch, &af);
  return ReturnValue::eSUCCESS;
}

qint32 ki_agility(quint8 level, CharacterPtr ch, QString arg, CharacterPtr vict)
{
  qint32 learned, chance, percent;
  affected_type af;

  if (ch->isNonPlayer() || ch->getLevel() >= ARCHANGEL)
    learned = 75;
  else if (!(learned = ch->has_skill(KI_AGILITY + KI_OFFSET)))
  {
    ch->sendln(u"You aren't experienced enough to teach others graceful movement."_s);
    return ReturnValue::eFAILURE;
  }

  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln(u"You have no group to instruct."_s);
    return ReturnValue::eFAILURE;
  }

  learned = learned % 100;

  chance = 75;

  // 101% is a complete failure
  percent = dc_->number(1, 101);
  if (percent > chance)
  {
    ch->sendln(u"Hopefully none of them noticed you trip on that rock."_s);
    act_to_room("$n tries to show everyone how to be graceful and trips over a rock.", ch, 0, 0, 0);
  }
  else
  {
    ch->sendln(u"You instruct your party on more graceful movement."_s);
    act_to_room("$n holds a quick tai chi class.", ch, 0, 0, 0);

    for (CharacterPtr tmp_char = dc_->world[ch->in_room].people_; tmp_char; tmp_char = tmp_char->next_in_room)
    {
      if (tmp_char == ch)
        continue;
      if (!ARE_GROUPED(ch, tmp_char))
        continue;
      affect_from_char(tmp_char, KI_AGILITY + KI_OFFSET);
      affect_from_char(tmp_char, KI_AGILITY + KI_OFFSET);
      act_to_victim("$n's graceful movement inspires you to better form.", ch, 0, tmp_char, 0);

      af.type = KI_AGILITY + KI_OFFSET;
      af.duration = 1 + learned / 10;
      af.modifier = 1;
      af.location = APPLY_MOVE_REGEN;
      af.bitvector = -1;
      affect_to_char(tmp_char, &af);
      af.modifier = -10 - learned / 4;
      af.location = APPLY_ARMOR;
      affect_to_char(tmp_char, &af);
    }
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  return ReturnValue::eSUCCESS;
}

qint32 ki_meditation(quint8 level, CharacterPtr ch, QString arg, CharacterPtr vict)
{
  qint32 gain;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  act_to_character("You enter a brief meditative state and focus your ki to heal your injuries.", ch, 0, vict, 0);
  act_to_room("$n enters a brief meditative state and focuses $s ki to heal several wounds.", ch, 0, vict, 0);

  gain = ch->hit_gain(position_t::SLEEPING);

  ch->setHP(MIN(ch->getHP() + gain, hit_limit(ch)));

  return ReturnValue::eSUCCESS;
}

qint32 ki_transfer(quint8 level, CharacterPtr ch, QString arg, CharacterPtr victim)
{
  QString amt, type;
  qint32 amount, temp = {};
  affected_type af;

  argument_interpreter(arg, amt, type);
  // arg = one_argument(arg, amt);
  // arg = one_argument(arg, type);

  amount = dc_atoi(amt);

  if (amount < 0)
  {
    ch->sendln(u"Trying to be a funny guy?"_s);
    return ReturnValue::eFAILURE;
  }

  if (amount > GET_KI(ch))
  {
    ch->sendln(u"You do not have that much energy to transfer."_s);
    return ReturnValue::eFAILURE;
  }

  qint32 learned = ch->has_skill(KI_TRANSFER + KI_OFFSET);

  if (victim->affected_by_spell(SPELL_KI_TRANS_TIMER))
  {
    act_to_character("$N cannot receive a transfer right now due to the stress $S mind has been recently been through.", ch, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SPELL_KI_TRANS_TIMER))
    affect_from_char(ch, SPELL_KI_TRANS_TIMER);

  af.type = SPELL_KI_TRANS_TIMER;
  af.duration = 1;
  af.modifier = {};
  af.location = {};
  af.bitvector = -1;

  affect_to_char(ch, &af);

  if (type[0] == 'k')
  {
    GET_KI(ch) -= amount;
    temp = dc_->number(amount - amount / 10, amount + amount / 10); //+-10%
    temp = (temp * learned) / 100;
    GET_KI(victim) += temp;
    if (GET_KI(victim) > GET_MAX_KI(victim))
      GET_KI(victim) = GET_MAX_KI(victim);

    dc_sprintf(amt, "%d", amount);
    send_damage("You focus intently, bonding briefly with $N's spirit, transferring | ki of your essence to $M.",
                ch, 0, victim, amt,
                "You focus intently, bonding briefly with $N's spirit,  transferring a portion of your essence to $M.", TO_CHAR);
    dc_sprintf(amt, "%d", temp);
    send_damage("$n focuses intently, bonding briefly with your spirit, replenishing | ki of your essence with $s own.",
                ch, 0, victim, amt,
                "$n focuses intently, bonding briefly with your spirit, replenishing your essence with $s own.", TO_VICT);
    act_to_room("$n focuses intently upon $N as though briefly bonding with $S spirit.", ch, 0, victim, NOTVICT);
  }
  else if (type[0] == 'm')
  {
    GET_KI(ch) -= amount;

    qint32 mana_per_ki = learned / 5;

    temp = mana_per_ki * amount;
    GET_MANA(victim) += temp;
    if (GET_MANA(victim) > GET_MAX_MANA(victim))
      GET_MANA(victim) = GET_MAX_MANA(victim);

    QString buffer;
    buffer = fmt::format("You focus intently, bonding briefly with $N's spirit, transferring {} ki of your essence into {} mana for $M.", amount, temp);
    act_to_character(buffer.c_str(), ch, 0, victim, 0);

    buffer = fmt::format("$n focuses intently, bonding briefly with your spirit, replenishing {} mana with a portion of $s essence.", temp);
    act_to_victim(buffer.c_str(), ch, 0, victim, 0);

    act_to_room("$n focuses intently upon $N as though briefly bonding with $S spirit.", ch, 0, victim, NOTVICT);
  }
  else
  {
    ch->sendln(u"You do not know of that essense."_s);
    return ReturnValue::eFAILURE;
  }

  return ReturnValue::eSUCCESS;
}
