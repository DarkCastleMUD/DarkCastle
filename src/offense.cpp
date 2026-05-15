/************************************************************************
| $Id: offense.cpp,v 1.27 2011/11/29 02:54:06 jhhudso Exp $
| offense.C
| Description:  Commands that are generically offensive - that is, the
|   victim should retaliate.  The class-specific offensive commands are
|   in class/
*/

#include "DC/DC.h"

ReturnValues do_suicide(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE; // just in case
  if (isSet(dc_->world[ch->in_room]->room_flags_, SAFE))
  {
    ch->sendln(u"This place is too peaceful for that."_s);
    return ReturnValue::eFAILURE;
  }
  if (ch->room().isArena())
  {
    ch->sendln(u"You can't do that in the arena."_s);
    return ReturnValue::eFAILURE;
  }
  if (ch->isPlayerObjectThief() || (ch->isPlayerGoldThief()))
  {
    ch->sendln(u"You're too busy running from the law!"_s);
    return ReturnValue::eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_CHAMPION))
  {
    ch->sendln(u"You have no reason to feel sad, oh great Champion!"_s);
    return ReturnValue::eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_CURSE) || IS_AFFECTED(ch, AFF_SOLIDITY))
  {
    ch->sendln(u"Something blocks your attempted suicide, be happy!  You have a new lease on life!"_s);
    return ReturnValue::eFAILURE;
  }
  if (GET_POS(ch) == position_t::FIGHTING || ch->fighting)
  {
    ch->sendln(u"You are too busy trying to kill somebody else!"_s);
    return ReturnValue::eFAILURE;
  }

  qint32 percent = dc_->number(1, 100);
  if (percent > GET_WIS(ch))
    percent -= GET_WIS(ch);

  if (percent > 50)
  {
    ch->sendln(u"You miss your wrists with the blade and knick your kneecap!"_s);
    act_to_room("$n tries to suicide, but fails miserably.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }
  ch->sendln(u"Looking out upon the world, you decide that it would be a better place without you."_s);
  act_to_room("Tired of life, $n decides to end $s.", ch, 0, 0, 0);
  fight_kill(ch, ch, TYPE_PKILL, 0);
  return ReturnValue::eSUCCESS;
}

// TODO - check differences between hit, murder, and kill....I think we can
// just pull out alot of the code into a function.
ReturnValues Character::do_hit(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim, k, next_char;
  qint32 count = {};

  QString arg1 = arguments.value(0);

  if (!arg1.isEmpty())
  {
    victim = get_char_room_vis(arg1);
    if (victim)
    {
      if (victim == this)
      {
        sendln(u"You hit yourself..OUCH!."_s);
        act("$n hits $mself, and says OUCH!",
            this, 0, victim, TO_ROOM, 0);
      }
      else
      {
        if (!can_attack(this) || !can_be_attacked(this, victim))
          return ReturnValue::eFAILURE;

        if (IS_AFFECTED(this, AFF_CHARM) && (master == victim))
        {
          act("$N is just such a good friend, you simply can't hit $M.",
              this, 0, victim, TO_CHAR, 0);
          return ReturnValue::eFAILURE;
        }

        if ((GET_POS(this) == position_t::STANDING) &&
            (victim != fighting))
        {

          for (k = combat_list; k; k = next_char)
          {
            next_char = k->next_fighting;
            if (k->fighting == victim)
              count++;
          }
          /*
          if (count >= 6) {
                  send(u"You can't get close enough to do anything."_s);
            return ReturnValue::eFAILURE;
          }
          */
          WAIT_STATE(this, DC::PULSE_VIOLENCE);
          return attack(this, victim, TYPE_UNDEFINED);
        }
        else
          sendln(u"You do the best you can!"_s);
      }
    }
    else
      sendln(u"They aren't here."_s);
  }
  else
    sendln(u"Hit whom?"_s);
  return ReturnValue::eFAILURE;
}

ReturnValues do_murder(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  CharacterPtr victim;

  one_argument(argument, arg);

  if (!arg.isEmpty())
  {
    victim = ch->get_char_room_vis(arg);
    if (victim)
    {
      if (victim == ch)
      {
        ch->sendln(u"You hit yourself..OUCH!."_s);
        act_to_room("$n hits $mself, and says OUCH!", ch, 0, victim, 0);
      }
      else
      {
        if (!can_attack(ch) || !can_be_attacked(ch, victim))
          return ReturnValue::eFAILURE;

        if (victim->isNonPlayer())
          return ReturnValue::eFAILURE;

        if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == victim))
        {
          act("$N is just such a good friend, you simply can't murder $M.",
              ch, 0, victim, TO_CHAR, 0);
          return ReturnValue::eFAILURE;
        }

        if ((GET_POS(ch) == position_t::STANDING) &&
            (victim != ch->fighting))
        {
          WAIT_STATE(ch, DC::PULSE_VIOLENCE + 2); /* HVORFOR DET?? */
          return attack(ch, victim, TYPE_UNDEFINED);
        }
        else
          ch->sendln(u"You do the best you can!"_s);
      }
    }
    else
      ch->sendln(u"They aren't here."_s);
  }
  else
    ch->sendln(u"Hit whom?"_s);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_slay(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  QString arg;
  CharacterPtr victim;

  one_argument(argument, arg);

  if (arg.isEmpty())
  {
    ch->sendln(u"Slay whom?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!(victim = ch->get_char_room_vis(arg)))
  {
    ch->sendln(u"They aren't here."_s);
    return ReturnValue::eFAILURE;
  }

  //  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master->isPlayer() && GET_CLASS(ch->master) == CLASS_ANTI_PAL && victim->isPlayer()) {
  //     act_to_character("I can't attack $N master!", ch->master, 0, victim,  0);
  //     return ReturnValue::eFAILURE;
  //  }

  if (IS_AFFECTED(ch, AFF_FAMILIAR) && ch->master->isPlayer())
  {
    act_to_character("But $N scares me!!", ch->master, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (ch == victim)
    ch->sendln(u"Your mother would be so sad.. :("_s);
  else
  {
    if (victim->isImplementerPlayer())
    {
      ch->sendln(u"You no make ME into chop suey!"_s);
      dc_sprintf(buf, "%s just tried to kill you.\r\n", qPrintable(ch->name()));
      victim->send(buf);
      if (ch->isImmortalPlayer())
      {
        fight_kill(victim, ch, TYPE_RAW_KILL, 0);
        ch->sendln(u"Lunch."_s);
      }
      return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
    }

    act("You chop $M to pieces! Ah! The blood!",
        ch, 0, victim, TO_CHAR, 0);
    act_to_character("$N chops you to pieces!", victim, 0, ch, 0);
    act_to_room("$n brutally slays $N.", ch, 0, victim, NOTVICT);
    fight_kill(ch, victim, TYPE_RAW_KILL, 0);
    return ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED;
  }

  return ReturnValue::eSUCCESS; // shouldn't get here
}

ReturnValues do_kill(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  QString arg;
  CharacterPtr victim;

  one_argument(argument, arg);

  if (arg.isEmpty())
  {
    ch->sendln(u"Slay whom?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!(victim = ch->get_char_room_vis(arg)))
  {
    ch->sendln(u"They aren't here."_s);
    return ReturnValue::eFAILURE;
  }

  //  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master->isPlayer() && GET_CLASS(ch->master) == CLASS_ANTI_PAL && victim->isPlayer()) {
  //     act_to_character("I can't attack $N master!", ch->master, 0, victim,  0);
  //     return ReturnValue::eFAILURE;
  //  }

  if (IS_AFFECTED(ch, AFF_FAMILIAR) && ch->master->isPlayer())
  {
    act_to_character("But $N scares me!!", ch->master, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if ((ch->getLevel() < DEITY) || ch->isNonPlayer())
  {
    if (!can_attack(ch) || !can_be_attacked(ch, victim))
      return ReturnValue::eFAILURE;
    return ch->do_hit(QString(argument).split(' '));
  }
  else
  {
    if (ch == victim)
      ch->sendln(u"Your mother would be so sad.. :("_s);
    else
    {
      if (victim->getLevel() >= IMPLEMENTER)
      {
        ch->sendln(u"You no make ME into chop suey!"_s);
        dc_sprintf(buf, "%s just tried to kill you.\r\n", qPrintable(ch->name()));
        victim->send(buf);
        if (ch->getLevel() > IMMORTAL)
        {
          fight_kill(victim, ch, TYPE_CHOOSE, 0);
          ch->sendln(u"Lunch."_s);
        }
        return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
      }
      act("You chop $M to pieces! Ah! The blood!",
          ch, 0, victim, TO_CHAR, 0);
      act_to_character("$N chops you to pieces!", victim, 0, ch, 0);
      act_to_room("$n brutally slays $N.", ch, 0, victim, NOTVICT);
      fight_kill(ch, victim, TYPE_CHOOSE, 0);
      return ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED;
    }
  }
  return ReturnValue::eSUCCESS; // shouldn't get here
}

/****************** JOIN IN THE FIGHT *******************************/

// if you send do_join a number as the argument, ch will attempt to
// join a mob of that number.  Only works if ch is mob.
//
ReturnValues Character::do_join(QStringList arguments, cmd_t cmd)
{
  if (fighting)
  {
    sendln(u"Aren't you helping enough as it is?"_s);
    return ReturnValue::eFAILURE;
  }

  CharacterPtr victim = {};
  QString victim_name = arguments.value(0);
  if (victim_name == "follower")
  {
    for (CharacterPtr *j = followers; j; j = j->next)
    {
      if (in_room == j->follower->in_room && j->follower->fighting)
      {
        if (IS_AFFECTED(j->follower, AFF_CHARM))
        {
          victim = j->follower;
        }
      }
    }

    if (!victim)
    {
      sendln(u"You have no loyal subjects engaged in combat!"_s);
      return ReturnValue::eFAILURE;
    }
  }

  bool victim_vnum_ok = false;
  if (!victim && isNonPlayer())
  {
    bool ok = false;
    vnum_t victim_vnum = victim_name.toULongLong(&ok);
    if (ok)
    {
      CharacterPtr possible_victim = dc_->world[in_room]->people_;
      for (; possible_victim; possible_victim = possible_victim->next_in_room)
      {
        if (possible_victim->isNonPlayer() && dc_->mob_index_[victim->mobdata->nr]->vnum() == victim_vnum)
        {
          victim = possible_victim;
          break;
        }
      }

      if (!victim)
      {
        return ReturnValue::eFAILURE;
      }
    }
  }

  if (!victim)
  {
    victim = get_char_room_vis(victim_name);
  }

  if (!victim)
  {
    sendln(u"Join whom?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!victim->fighting)
  {
    sendln(u"But they're not fighting anyone."_s);
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    sendln(u"You can't join yourself."_s);
    return ReturnValue::eFAILURE;
  }

  if (victim->fighting == this)
  {
    sendln(u"But why join someone who is trying to kill YOU?"_s);
    return ReturnValue::eFAILURE;
  }

  // if (IS_AFFECTED(ch, AFF_CHARM) && ch->master->isPlayer() && GET_CLASS(ch->master) == CLASS_ANTI_PAL && victim->fighting->isPlayer()) {
  //    act_to_character("I can't join the attack against $N master!", ch->master, 0, victim->fighting,  0);
  //    return ReturnValue::eFAILURE;
  // }

  CharacterPtr tmp_ch = victim->fighting;

  if (!tmp_ch)
  {
    act_to_character("But $N is not fighting!!!", this, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(this) || (victim->fighting && !can_be_attacked(this, victim->fighting)))
  {
    return ReturnValue::eFAILURE;
  }

  sendln(u"ARGGGGG!!!! *** K I L L ***!!!!."_s);
  act_to_character("$N joins you in the fight!", victim, 0, this, 0);
  act_to_room("$n has joined $N in the battle.", this, 0, victim, NOTVICT);

  return attack(this, tmp_ch, TYPE_UNDEFINED);
}
