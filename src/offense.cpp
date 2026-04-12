/************************************************************************
| $Id: offense.cpp,v 1.27 2011/11/29 02:54:06 jhhudso Exp $
| offense.C
| Description:  Commands that are generically offensive - that is, the
|   victim should retaliate.  The class-specific offensive commands are
|   in class/
*/

#include "DC/levels.h"
#include "DC/structs.h"
#include "DC/DC.h"

#include "DC/spells.h"
#include "DC/fight.h"

#include "DC/act.h"
#include "DC/returnvals.h"

#include "DC/interp.h"
#include "DC/const.h"
#include "DC/utility.h"

command_return_t do_suicide(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE; // just in case
  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("This place is too peaceful for that.");
    return ReturnValue::eFAILURE;
  }
  if (ch->room().isArena())
  {
    ch->sendln("You can't do that in the arena.");
    return ReturnValue::eFAILURE;
  }
  if (ch->isPlayerObjectThief() || (ch->isPlayerGoldThief()))
  {
    ch->sendln("You're too busy running from the law!");
    return ReturnValue::eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_CHAMPION))
  {
    ch->sendln("You have no reason to feel sad, oh great Champion!");
    return ReturnValue::eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_CURSE) || IS_AFFECTED(ch, AFF_SOLIDITY))
  {
    ch->sendln("Something blocks your attempted suicide, be happy!  You have a new lease on life!");
    return ReturnValue::eFAILURE;
  }
  if (GET_POS(ch) == position_t::FIGHTING || ch->fighting)
  {
    ch->sendln("You are too busy trying to kill somebody else!");
    return ReturnValue::eFAILURE;
  }

  qint32 percent = number(1, 100);
  if (percent > GET_WIS(ch))
    percent -= GET_WIS(ch);

  if (percent > 50)
  {
    ch->sendln("You miss your wrists with the blade and knick your kneecap!");
    act_to_room("$n tries to suicide, but fails miserably.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }
  ch->sendln("Looking out upon the world, you decide that it would be a better place without you.");
  act_to_room("Tired of life, $n decides to end $s.", ch, 0, 0, 0);
  fight_kill(ch, ch, TYPE_PKILL, 0);
  return ReturnValue::eSUCCESS;
}

// TODO - check differences between hit, murder, and kill....I think we can
// just pull out alot of the code into a function.
command_return_t Character::do_hit(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim, k, next_char;
  qint32 count = {};

  QString arg1 = arguments.value(0);

  if (!arg1.isEmpty())
  {
    victim = this->get_char_room_vis(arg1);
    if (victim)
    {
      if (victim == this)
      {
        this->sendln("You hit yourself..OUCH!.");
        act("$n hits $mself, and says OUCH!",
            this, 0, victim, TO_ROOM, 0);
      }
      else
      {
        if (!can_attack(this) || !can_be_attacked(this, victim))
          return ReturnValue::eFAILURE;

        if (IS_AFFECTED(this, AFF_CHARM) && (this->master == victim))
        {
          act("$N is just such a good friend, you simply can't hit $M.",
              this, 0, victim, TO_CHAR, 0);
          return ReturnValue::eFAILURE;
        }

        if ((GET_POS(this) == position_t::STANDING) &&
            (victim != this->fighting))
        {

          for (k = combat_list; k; k = next_char)
          {
            next_char = k->next_fighting;
            if (k->fighting == victim)
              count++;
          }
          /*
          if (count >= 6) {
                  this->send("You can't get close enough to do anything.");
            return ReturnValue::eFAILURE;
          }
          */
          WAIT_STATE(this, DC::PULSE_VIOLENCE);
          return attack(this, victim, TYPE_UNDEFINED);
        }
        else
          this->sendln("You do the best you can!");
      }
    }
    else
      this->sendln("They aren't here.");
  }
  else
    this->sendln("Hit whom?");
  return ReturnValue::eFAILURE;
}

command_return_t do_murder(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  CharacterPtr victim;

  one_argument(argument, arg);

  if (*arg)
  {
    victim = ch->get_char_room_vis(arg);
    if (victim)
    {
      if (victim == ch)
      {
        ch->sendln("You hit yourself..OUCH!.");
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
          ch->sendln("You do the best you can!");
      }
    }
    else
      ch->sendln("They aren't here.");
  }
  else
    ch->sendln("Hit whom?");
  return ReturnValue::eSUCCESS;
}

command_return_t do_slay(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  QString arg;
  CharacterPtr victim;

  one_argument(argument, arg);

  if (arg.isEmpty())
  {
    ch->sendln("Slay whom?");
    return ReturnValue::eFAILURE;
  }

  if (!(victim = ch->get_char_room_vis(arg)))
  {
    ch->sendln("They aren't here.");
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
    ch->sendln("Your mother would be so sad.. :(");
  else
  {
    if (victim->isImplementerPlayer())
    {
      ch->sendln("You no make ME into chop suey!");
      dc_sprintf(buf, "%s just tried to kill you.\r\n", qPrintable(ch->name()));
      victim->send(buf);
      if (ch->isImmortalPlayer())
      {
        fight_kill(victim, ch, TYPE_RAW_KILL, 0);
        ch->sendln("Lunch.");
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

command_return_t do_kill(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  QString arg;
  CharacterPtr victim;

  one_argument(argument, arg);

  if (arg.isEmpty())
  {
    ch->sendln("Slay whom?");
    return ReturnValue::eFAILURE;
  }

  if (!(victim = ch->get_char_room_vis(arg)))
  {
    ch->sendln("They aren't here.");
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
      ch->sendln("Your mother would be so sad.. :(");
    else
    {
      if (victim->getLevel() >= IMPLEMENTER)
      {
        ch->sendln("You no make ME into chop suey!");
        dc_sprintf(buf, "%s just tried to kill you.\r\n", qPrintable(ch->name()));
        victim->send(buf);
        if (ch->getLevel() > IMMORTAL)
        {
          fight_kill(victim, ch, TYPE_CHOOSE, 0);
          ch->sendln("Lunch.");
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
command_return_t Character::do_join(QStringList arguments, cmd_t cmd)
{
  if (fighting)
  {
    sendln("Aren't you helping enough as it is?");
    return ReturnValue::eFAILURE;
  }

  CharacterPtr victim = {};
  QString victim_name = arguments.value(0);
  if (victim_name == "follower")
  {
    for (follow_type *j = followers; j; j = j->next)
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
      sendln("You have no loyal subjects engaged in combat!");
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
      CharacterPtr possible_victim = DC::getInstance()->world[in_room].people;
      for (; possible_victim; possible_victim = possible_victim->next_in_room)
      {
        if (possible_victim->isNonPlayer() && DC::getInstance()->mob_index[victim->mobdata->nr].vnum() == victim_vnum)
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
    sendln("Join whom?");
    return ReturnValue::eFAILURE;
  }

  if (!victim->fighting)
  {
    this->sendln("But they're not fighting anyone.");
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    sendln("You can't join yourself.");
    return ReturnValue::eFAILURE;
  }

  if (victim->fighting == this)
  {
    sendln("But why join someone who is trying to kill YOU?");
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

  sendln("ARGGGGG!!!! *** K I L L ***!!!!.");
  act_to_character("$N joins you in the fight!", victim, 0, this, 0);
  act_to_room("$n has joined $N in the battle.", this, 0, victim, NOTVICT);

  return attack(this, tmp_ch, TYPE_UNDEFINED);
}
