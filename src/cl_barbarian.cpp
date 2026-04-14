/************************************************************************
 * $Id: cl_barbarian.cpp,v 1.113 2011/01/27 05:55:44 jhhudso Exp $
 * cl_barbarian.cpp
 * Description: Commands for the barbarian class.
 *************************************************************************/

#include "DC/DC.h"

extern qint32 rev_dir[];

command_return_t do_batter(CharacterPtr ch, QString argument, cmd_t cmd)
{
  bool battervbrace = false;
  bool batterwins = false;
  QString type, dir;
  QString buf, buf2, dammsg[20];
  RoomDirectionPtr exit, *back;
  qint32 other_room, door, dam, skill, retval;

  if (!(skill = ch->has_skill(SKILL_BATTERBRACE)))
  {
    ch->sendln("You could accidentally hurt someone if you try this untrained...");
    return ReturnValue::eFAILURE;
  }

  argument_interpreter(argument, type, dir);

  if (type.isEmpty())
  {
    ch->sendln("Batter what??");
    ch->sendln("batter <door> <direction>");
    return ReturnValue::eFAILURE;
  }

  if (dir.isEmpty())
  {
    ch->sendln("What direction?");
    return ReturnValue::eFAILURE;
  }

  if ((door = find_door(ch, type, dir)) >= 0)
  {
    exit = EXIT(ch, door);

    // check to make sure door is in right state
    if (!isSet(exit->exit_info, EX_ISDOOR))
    {
      ch->sendln("You can't figure out how to break it down.");
      return ReturnValue::eFAILURE;
    }

    if (!isSet(exit->exit_info, EX_CLOSED))
    {
      ch->sendln("You can't break down an open door!");
      return ReturnValue::eFAILURE;
    }

    if (isSet(exit->exit_info, EX_PICKPROOF))
    {
      ch->sendln("It seems far too sturdy for you to break down.");
      return ReturnValue::eFAILURE;
    }

    if (exit->bracee == ch)
    {
      ch->sendln("You can't batter a door you're bracing shut!");
      return ReturnValue::eFAILURE;
    }

    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1.5);
    if (!charge_moves(ch, SKILL_BATTERBRACE))
      return ReturnValue::eSUCCESS;

    dam = dc_->number(100, 200) + 3 * (100 - skill);

    ch->send(u"You take a deep breath, let loose a mighty bellow, and charge blindly at the %s in your path...\r\n"_s.arg(qPrintable(fname(exit->keyword))));
    act_to_room("$n takes a deep breath, lets loose a mighty bellow, and charges blindly at the $F in $s path...", ch, 0, exit->keyword, 0);

    if (!skill_success(ch, nullptr, SKILL_BATTERBRACE))
    {

      dc_sprintf(dammsg, "$B%d$R", dam);
      dc_sprintf(buf2, "With a resounding crash, you bounce off the %s and fall to the ground, receiving | damage!", qPrintable(fname(exit->keyword)));
      dc_sprintf(buf, "With a resounding crash, you bounce off the %s and fall to the ground!", qPrintable(fname(exit->keyword)));
      send_damage(buf2, ch, 0, 0, dammsg, buf, TO_CHAR);
      dc_sprintf(buf, "With a resounding crash, $e bounces off the %s and falls to the ground!", qPrintable(fname(exit->keyword)));
      send_damage(buf, ch, 0, 0, dammsg, buf, TO_ROOM);

      dc_sprintf(buf, "The %s survived, but you didn't...\r\n", qPrintable(fname(exit->keyword)));
      dc_sprintf(buf2, "The %s survived, but $n didn't...", qPrintable(fname(exit->keyword)));
      retval = noncombat_damage(ch, dam, buf, buf2, 0, KILL_BATTER);

      if (SOMEONE_DIED(retval))
      {
        return retval;
      }

      ch->setSitting();
      update_pos(ch);
      return ReturnValue::eFAILURE;
    }
    else
    {
      // check for brace
      if (exit->bracee != nullptr)
      {
        battervbrace = true;
        qint32 batterer = GET_STR(ch) + GET_CON(ch) + GET_DEX(ch) + dc_->number(1, 10);
        qint32 bracee = GET_STR(exit->bracee) + GET_CON(exit->bracee) + GET_DEX(exit->bracee) + dc_->number(1, 10);
        if (batterer < bracee) // batterer fails (ch fails)
        {
          ch->decrementMove(100);
        }
        else
        {
          batterwins = true;
          exit->bracee->decrementMove(100);
        }
      }

      REMOVE_BIT(exit->exit_info, EX_CLOSED); // break this side of door
      SET_BIT(exit->exit_info, EX_BROKEN);

      if ((other_room = exit->to_room) != DC::NOWHERE)
        if ((back = dc_->world[other_room].dir_option[rev_dir[door]]) != 0)
          if (back->to_room == ch->in_room)
          {
            REMOVE_BIT(back->exit_info, EX_CLOSED); // break other side of door
            SET_BIT(back->exit_info, EX_BROKEN);
          }

      if (battervbrace)
      {
        if (batterwins)
        {
          exit->bracee->send(u"The %s bursts open with a resounding crash and you are hurld to the ground!\r\n"_s.arg(qPrintable(fname(exit->keyword))));
          act_to_room("The $F bursts open with a resounding crash and $n is hurled to the ground!", exit->bracee, 0, exit->keyword, 0);
          exit->bracee->setSitting();
          update_pos(exit->bracee);
          do_brace(exit->bracee, ""); // unbrace
        }
        else
        { // brace wins
          exit->bracee->send(u"The %s shakes dangerously as a powerful blow strikes it from the other side!\r\n"_s.arg(qPrintable(fname(exit->keyword))));
        }
      }
      else
      {
        dc_sprintf(dammsg, "$B%d$R", dam);
        dc_sprintf(buf2, "With a resounding crash, the %s gives way and bursts open, receiving | damage!.", qPrintable(fname(exit->keyword)));
        dc_sprintf(buf, "With a resounding crash, the %s gives way and bursts open!\r\n", qPrintable(fname(exit->keyword)));
        send_damage(buf2, ch, 0, 0, dammsg, buf, TO_CHAR);
        dc_sprintf(buf, "With a resounding crash, the %s gives way and bursts open!", qPrintable(fname(exit->keyword)));
        send_damage(buf, ch, 0, 0, dammsg, buf, TO_ROOM);
      }

      dc_sprintf(buf, "Your heroic efforts managed to slay both the %s... and yourself. Nice going.\r\n", qPrintable(fname(exit->keyword)));
      dc_sprintf(buf2, "$n's heroic efforts manage to slay both the %s... and $mself. Oops.", qPrintable(fname(exit->keyword)));

      retval = noncombat_damage(ch, dam, buf, buf2, 0, KILL_BATTER);

      if (SOMEONE_DIED(retval))
      {
        return retval;
      }

      if (ch->dc_->number(1, 100) > (40 - GET_DEX(ch)) + (100 - skill))
      {
        ch->sendln("You manage to maintain your balance and admire your handywork.");
        act_to_room("$n manages to maintain $h balance and admires $s handywork.", ch, 0, exit->keyword, 0);
      }
      else
      {
        if (CAN_GO(ch, door) &&
            !isSet(dc_->world[EXIT(ch, door)->to_room].room_flags, IMP_ONLY) &&
            (!IS_AFFECTED(ch, AFF_CHAMPION) || champion_can_go(EXIT(ch, door)->to_room)) &&
            class_can_go(GET_CLASS(ch), EXIT(ch, door)->to_room) &&
            !others_clan_room(ch, &dc_->world[EXIT(ch, door)->to_room]))
        {
          ch->sendln("You are unable to maintain your balance and sail into the adjacent room! Ouch!\r\n");
          act_to_room("$n is unable to maintain $h balance and sails into the adjacent room!", ch, 0, exit->keyword, 0);
          move_char(ch, exit->to_room);
          act_to_room("The $F suddenly bursts apart and $n tumbles headlong through!", ch, 0, exit->keyword, 0);
          do_look(ch, "");
        }
        else
        {
          ch->sendln("You are unable to maintain your balance and sail towards the adjacent room, but some force keeps you out!");
          act("$n is unable to maintain $h balance and sails towards the adjacent room, but some force keeps $h out!",
              ch, 0, exit->keyword, TO_ROOM, 0);
        }

        ch->setSitting();
        update_pos(ch);
      }
      return ReturnValue::eSUCCESS;
    }
  }
  ch->sendln("You don't see anything like that to batter.");
  return ReturnValue::eFAILURE;
}

command_return_t do_brace(CharacterPtr ch, const QString argument, cmd_t cmd)
{
  qint32 door, other_room;
  RoomDirectionPtr back, *exit;
  QString type, dir;

  argument_interpreter(argument, type, dir);

  if (!ch->has_skill(SKILL_BATTERBRACE))
  {
    ch->sendln("You could accidentally hurt someone if you try this untrained...");
    return ReturnValue::eFAILURE;
  }

  if (type.isEmpty())
  {
    if (ch->brace_at != nullptr)
    {
      if (cmd == cmd_t::UNDEFINED)
      {
        ch->send(u"You are no longer able to brace the %s.\r\n"_s.arg(qPrintable(fname(ch->brace_at->keyword))));
      }
      else
      {
        ch->send(u"You stop holding the %s shut.\r\n"_s.arg(qPrintable(fname(ch->brace_at->keyword))));
        act_to_room("$n stops holding the $F shut.", ch, 0, ch->brace_at->keyword, 0);
      }
      ch->brace_at->bracee = {};
      ch->brace_at = {};
      if (ch->brace_exit != nullptr) // incase it's a weird exit area
        ch->brace_exit->bracee = {};
      ch->brace_exit = {};
      return ReturnValue::eSUCCESS;
    }
    ch->sendln("Brace what??");
    ch->sendln("brace <door> <direction>");
    return ReturnValue::eFAILURE;
  }

  if (dir.isEmpty())
  {
    ch->sendln("What direction?");
    return ReturnValue::eFAILURE;
  }

  if ((door = find_door(ch, type, dir)) >= 0)
  {

    exit = EXIT(ch, door);

    if (!isSet(exit->exit_info, EX_ISDOOR))
    {
      ch->sendln("You can't figure out how to hold it shut.");
      return ReturnValue::eFAILURE;
    }

    if (!isSet(exit->exit_info, EX_CLOSED))
    {
      ch->sendln("You have to close it first!");
      return ReturnValue::eFAILURE;
    }
    if (exit->bracee != nullptr)
    {
      if (exit->bracee->in_room == ch->in_room)
      {
        if (exit->bracee == ch)
        {
          ch->send(u"You are already bracing the %s shut!\r\n"_s.arg(qPrintable(fname(exit->keyword))));
        }
        else
        {
          ch->send(u"%s is already holding the %s shut!\r\n"_s.arg(qPrintable(exit->bracee->name())).arg(qPrintable(fname(exit->keyword))));
        }
      }
      else
        ch->send(u"The %s is already being braced from the other side!\r\n"_s.arg(qPrintable(fname(exit->keyword))));

      return ReturnValue::eFAILURE;
    }

    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1.5);
    if (!charge_moves(ch, SKILL_BATTERBRACE, 0.5))
      return ReturnValue::eSUCCESS;

    ch->send(u"You lean heavily on the %s, bracing your shoulder solidly against it...\r\n"_s.arg(qPrintable(fname(exit->keyword))));
    act_to_room("$n leans heavily on the $F, bracing $s shoulder solidly against it...", ch, 0, exit->keyword, 0);

    if (!skill_success(ch, nullptr, SKILL_BATTERBRACE))
    {
      ch->sendln("Your attempt to block the passage fails.");
      act_to_room("$s attempt to block the passage fails!", ch, 0, exit->keyword, 0);
      return ReturnValue::eFAILURE;
    }
    else
    {
      ch->sendln("The passage now appears firmly blocked.");
      act_to_room("The passage now appears firmly blocked.", ch, 0, exit->keyword, 0);
      exit->bracee = ch;
      ch->brace_at = exit;
      if ((other_room = exit->to_room) != DC::NOWHERE)
        if ((back = dc_->world[other_room].dir_option[rev_dir[door]]) != 0)
          if (back->to_room == ch->in_room)
          {
            ch->brace_exit = back;
            back->bracee = ch;
          }

      return ReturnValue::eSUCCESS;
    }
  }

  ch->sendln("You don't see anything like that to block.");
  return ReturnValue::eFAILURE;
}

command_return_t Character::do_rage(QStringList arguments, cmd_t cmd)
{
  if (getHP() == 1)
  {
    sendln("You are feeling too weak right now to work yourself up into a rage.");
    return ReturnValue::eFAILURE;
  }

  if (!canPerform(SKILL_RAGE, "You should learn the skill before you try doing any raging in this machine...\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  QString name = arguments.value(0);

  CharacterPtr victim = get_char_room_vis(name);
  if (!victim)
  {
    if (fighting)
    {
      victim = fighting;
    }
    else
    {
      sendln("Who do you want to rage on?");
      return ReturnValue::eFAILURE;
    }
  }

  if (in_room != victim->in_room)
  {
    this->sendln("That person seems to have left.");
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    this->sendln("Aren't we funny today...");
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(this) || !can_be_attacked(this, victim))
    return ReturnValue::eFAILURE;

  if (!charge_moves(SKILL_RAGE))
    return ReturnValue::eSUCCESS;

  qint32 retval = {};
  if (!skill_success(victim, SKILL_RAGE))
  {
    act("You start advancing towards $N, but trip over your own feet!",
        this, 0, victim, TO_CHAR, 0);
    act("$n starts advancing towards $N, but trips over $s own feet!",
        this, 0, victim, TO_ROOM, NOTVICT);
    act_return ar = act_to_victim("$n starts advancing toward you, but trips over $s own feet!", this, 0, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED))
    {
      return retval;
    }

    this->setSitting();
    SET_BIT(this->combat, COMBAT_BASH1);
  }
  else
  {
    act("You advance confidently towards $N, and fly into a rage!",
        this, 0, victim, TO_CHAR, 0);
    act("$n advances confidently towards $N, and flies into a rage!",
        this, 0, victim, TO_ROOM, NOTVICT);
    act_return ar = act_to_victim("$n advances confidently towards you, and flies into a rage!", this, 0, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED))
    {
      return retval;
    }

    SET_BIT(this->combat, COMBAT_RAGE1);
  }

  WAIT_STATE(this, DC::PULSE_VIOLENCE * 3);

  if (!this->fighting)
    return attack(this, victim, TYPE_UNDEFINED);

  // chance of bonus round at high level of skill
  if (has_skill(SKILL_RAGE) > 75 && !number(0, 9))
    return attack(this, victim, TYPE_UNDEFINED);

  return ReturnValue::eSUCCESS;
}

command_return_t do_battlecry(CharacterPtr ch, QString argument, cmd_t cmd)
{
  follow_type *f = {};

  if (!ch->canPerform(SKILL_BATTLECRY, "Have to learn how to battlecry before you can run with the big boys...\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  if (!ch->fighting)
  {
    ch->sendln("You must be fighting already in order to battlecry.");
    return ReturnValue::eFAILURE;
  }

  if (ch->master || !ch->followers)
  {
    ch->sendln("You must be leading a group in order to battlecry.");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BATTLECRY))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_BATTLECRY))
  {
    act_to_character("You give a cry of defiance, but trip over your own feet!", ch, 0, 0, 0);
    act_to_room("$n gives a cry of defiance, but trips over $s own feet!", ch, 0, 0, 0);

    ch->setSitting();
    SET_BIT(ch->combat, COMBAT_BASH1);
  }
  else
  {
    act_to_character("You give a battlecry, sounding your defiance!", ch, 0, 0, 0);
    act_to_room("$n yells 'They can take our lives, but they'll never take OUR FREEDOM!'", ch, 0, 0, 0);

    if (ch->followers)
      f = ch->followers;

    for (; f; f = f->next)
    {
      if (!IS_AFFECTED(f->follower, AFF_GROUP) ||
          (isSet(f->follower->combat, COMBAT_RAGE1)) ||
          (isSet(f->follower->combat, COMBAT_RAGE2)) ||
          (isSet(f->follower->combat, COMBAT_BERSERK)) ||
          (f->follower->in_room != ch->in_room) ||
          (!f->follower->fighting))
        continue;

      if (ch->dc_->number(1, 101) > ch->has_skill(SKILL_BATTLECRY))
      {
        act_to_character("You look away sheepishly, unaffected by the rage.", f->follower, 0, 0, 0);
        act_to_room("$n looks away sheepishly, unaffected by the rage.", f->follower, 0, 0, 0);
        continue;
      }
      else
      {
        act_to_character("You give a battlecry, sounding your defiance!", f->follower, 0, 0, 0);
        act_to_room("$n gives a loud cry of agreement!", f->follower, 0, 0, 0);
        SET_BIT(f->follower->combat, COMBAT_RAGE1);
      }
    }
  }

  if (ch->has_skill(SKILL_BATTLECRY) > 40 && !number(0, 4))
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  else
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
  return ReturnValue::eSUCCESS;
}

command_return_t do_berserk(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name;
  qint32 bSuccess = {};
  qint32 retval = {};

  if (!ch->canPerform(SKILL_BERSERK, "You aren't crazy enough for that yet... try rage maybe...\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  if (ch->getHP() == 1)
  {
    send_to_char("You are feeling too weak right now to work yourself up into "
                 "a frenzy.",
                 ch);
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    if (ch->fighting)
      victim = ch->fighting;
    else
    {
      ch->sendln("Who do you want to go berserk on?");
      return ReturnValue::eFAILURE;
    }
  }
  else if (ch->fighting)
  {
    ch->sendln("You're already in combat, and focus your energies on your current target instead.");
    victim = ch->fighting;
  }

  if (ch->in_room != victim->in_room)
  {
    ch->sendln("That person seems to have left.");
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Aren't we funny today...");
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return ReturnValue::eFAILURE;

  if (!charge_moves(ch, SKILL_BERSERK))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, victim, SKILL_BERSERK))
  {
    act_to_character("You start freaking out on $N, but trip over your own feet!", ch, 0, victim, 0);
    act_to_room("$n starts freaking out on $N, but trips over $s own feet!", ch, 0, victim, NOTVICT);
    act_return ar = act_to_victim("$n starts freaking out on you, but trips over $s own feet!", ch, 0, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED))
    {
      return retval;
    }

    ch->setSitting();
    if (ch->has_skill(SKILL_BERSERK) > 50 && !number(0, 5))
    {
      SET_BIT(ch->combat, COMBAT_BASH2);
      ch->sendln("Your advanced training in berserk allows you to roll with your fall and get up faster.");
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
    }
    else
    {
      SET_BIT(ch->combat, COMBAT_BASH1);
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
    }
  }
  else
  {
    act_to_character("You start FOAMING at the mouth, and you go BERSERK on $N!", ch, 0, victim, 0);
    act_to_room("$n starts FOAMING at the mouth, as $e goes BERSERK on $N!", ch, 0, victim, NOTVICT);
    act_return ar = act_to_victim("$n starts FOAMING at the mouth, and goes BERSERK on you!", ch, 0, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED))
    {
      return retval;
    }

    bSuccess = 1;
  }

  if (bSuccess && !isSet(ch->combat, COMBAT_RAGE1))
    SET_BIT(ch->combat, COMBAT_RAGE1);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  if (!ch->fighting)
    retval = attack(ch, victim, TYPE_UNDEFINED);

  if (!isSet(retval, ReturnValue::eCH_DIED))
  {
    if (!isSet(retval, ReturnValue::eVICT_DIED) && ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_WIMPY))
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);

    REMOVE_BIT(ch->combat, COMBAT_RAGE1);
    REMOVE_BIT(ch->combat, COMBAT_RAGE2);

    // I set the berserk bit AFTER the attack, to reduce the advantage
    // a bit.
    if (bSuccess)
    {
      SET_BIT(ch->combat, COMBAT_BERSERK);
      if (ch->isPlayer())
        GET_AC(ch) += 30; // we do this here, so we know if someone dies with COMBAT_BERSERK to
                          // give them their AC back
    }
  }
  return retval;
}

command_return_t do_headbutt(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name;
  qint32 retval = {};

  if (!ch->canPerform(SKILL_HEADBUTT,
                      "You'd bonk yourself silly without proper training.\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);

  victim = ch->get_char_room_vis(name);
  if (victim == nullptr)
    victim = ch->fighting;

  if (victim == nullptr)
  {
    ch->sendln("Headbutt whom?");
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Aren't we funny today...");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_HUGE) && ch->has_skill(SKILL_HEADBUTT) < 86)
  {
    ch->sendln("You are too puny to headbutt someone that HUGE!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_SWARM))
  {
    ch->sendln("You cannot pick just one to headbutt!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_TINY))
  {
    act_to_character("$N's small size makes it impossible to target just $S head!", ch, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_NOHEADBUTT))
  {
    ch->sendln("That would be like smashing your head into a wall!");
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return ReturnValue::eFAILURE;

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Headbutting a bladeshielded opponent would be asking for decapitation!");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_HEADBUTT))
    return ReturnValue::eSUCCESS;

  if (isSet(victim->combat, COMBAT_BERSERK) && (victim->isNonPlayer() || victim->has_skill(SKILL_BERSERK) > 80))
  {
    act_to_room("$N shakes off $n's attempt to immobilize them.", ch, nullptr, victim, NOTVICT);
    act_to_character("$N shakes off your attempt to immobilize them.", ch, nullptr, victim, NOTVICT);
    act_to_victim("In your enraged state, you shake off $n's attempt to immobilize you.", ch, nullptr, victim, 0);

    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
    return ReturnValue::eSUCCESS;
  }

  qint32 mod = {};
  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_HUGE))
    mod = -25;

  if (victim->equipment[WEAR_HEAD])
    mod -= victim->equipment[WEAR_HEAD]->obj_flags.value[0];

  mod -= (GET_DEX(victim) / 2);
  mod += (GET_STR(ch) / 2);

  if (mod > 0)
    mod = {};

  qint32 get_weapon_bit(qint32 weapon_type);
  qint32 weapon_bit;
  weapon_bit = get_weapon_bit(TYPE_CRUSH);

  if (!skill_success(ch, victim, SKILL_HEADBUTT, mod) || isSet(victim->immune, weapon_bit) || do_frostshield(ch, victim))
  {
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
    retval = damage(ch, victim, 0, TYPE_CRUSH, SKILL_HEADBUTT);
    // the damage call here takes care of starting combat and such
    retval = ReturnValue::eSUCCESS;
  }
  else
  {
    if (victim->affected_by_spell(SKILL_BATTLESENSE) &&
        dc_->number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act_to_character("$N's heightened battlesense sees your headbutt coming from a mile away.", ch, 0, victim, 0);
      act_to_room("$N's heightened battlesense sees $n's headbutt coming from a mile away.", ch, 0, victim, NOTVICT);
      act_return ar = act_to_victim("Your heightened battlesense sees $n's headbutt coming from a mile away.", ch, 0, victim, 0);
      retval = ar.retval;
      if (isSet(retval, ReturnValue::eVICT_DIED))
      {
        return retval;
      }

      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
      retval = damage(ch, victim, 0, TYPE_CRUSH, SKILL_HEADBUTT);
      // the damage call here takes care of starting combat and such
      retval = ReturnValue::eSUCCESS;
    }
    else
    {
      if (isSet(victim->combat, COMBAT_BERSERK))
        REMOVE_BIT(victim->combat, COMBAT_BERSERK);

      set_fighting(victim, ch);
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);

      WAIT_STATE(victim, DC::PULSE_VIOLENCE * 2);
      SET_BIT(victim->combat, COMBAT_SHOCKED2);
      retval = damage(ch, victim, 50, TYPE_CRUSH, SKILL_HEADBUTT);
      if (!SOMEONE_DIED(retval) && !number(0, 9) &&
          ch->equipment[WEAR_HEAD] && dc_->obj_index[ch->equipment[WEAR_HEAD]->item_number].vnum() == 508)
      {
        act_to_room("$n's spiked helmet crackles as it strikes $N's face!", ch, nullptr, victim, NOTVICT);
        act_to_victim("$n's spiked helmet crackles as it strikes your face!", ch, nullptr, victim, 0);
        act_to_character("Your spiked helmet crackles as it strikes $N's face!", ch, nullptr, victim, 0);
        //	retval = damage(ch, victim, 50, TYPE_PIERCE, TYPE_UNDEFINED);
        retval = spell_shocking_grasp(50, ch, victim, 0, 60);
        // TWEAKME
      }
    }
  }

  return retval;
}

command_return_t do_bloodfury(CharacterPtr ch, QString argument, cmd_t cmd)
{
  affected_type af;
  qreal modifier;
  qint32 duration = 42;

  if (!ch->canPerform(SKILL_BLOOD_FURY,
                      "You've no idea how to raise such bloodlust.\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_BLOOD_FURY))
  {
    ch->sendln("Your body can not yet take the strain of another blood fury yet.");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BLOOD_FURY))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_BLOOD_FURY))
  {
    act_to_room("$n starts breathing heavily, then chokes and tries to clear $s head.", ch, nullptr, nullptr, NOTVICT);
    ch->sendln("You try to pysch yourself up and choke on the taste of blood.");
    duration = 1;
  }
  else
  {
    act("Panting heavily $n's eyes glaze red $e begins to move with renewed fury!",
        ch, nullptr, nullptr, TO_ROOM, NOTVICT);
    send_to_char("Your sight tinges red with the blood of battle and your "
                 "limbs feel strong with death and destruction deep within "
                 "your bones.\r\n",
                 ch);

    modifier = .2 + (.00375 * ch->has_skill(SKILL_BLOOD_FURY)); // mod = .2 - .5

    ch->addHP((qreal)GET_MAX_HIT(ch) * modifier);

    duration = 36 - (ch->getLevel() / 6);
  }

  af.type = SKILL_BLOOD_FURY;
  af.duration = duration;
  af.modifier = {};
  af.location = {};
  af.bitvector = -1;

  affect_to_char(ch, &af);

  return ReturnValue::eSUCCESS;
}

command_return_t do_crazedassault(CharacterPtr ch, QString argument, cmd_t cmd)
{
  affected_type af;
  qint32 duration = 20;
  if (ch->affected_by_spell(SKILL_CRAZED_ASSAULT) && !ch->isImmortalPlayer())
  {
    ch->sendln("Your body is still recovering from your last crazed assault technique.");
    return ReturnValue::eFAILURE;
  }

  if (!ch->canPerform(SKILL_CRAZED_ASSAULT, "You just aren't crazy enough...try assaulting old ladies.\r\n"))
    return ReturnValue::eFAILURE;

  if (!charge_moves(ch, SKILL_CRAZED_ASSAULT))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_CRAZED_ASSAULT))
  {
    ch->sendln("You try to psyche yourself up for it but just can't muster the concentration.");
    duration = 1;
  }
  else
  {
    ch->sendln("Your mind focuses completely on hitting your opponent.");
    af.type = SKILL_CRAZED_ASSAULT;
    af.duration = 2;
    af.modifier = (ch->has_skill(SKILL_CRAZED_ASSAULT) / 5) + 5;
    af.location = APPLY_HITROLL;
    af.bitvector = -1;
    affect_to_char(ch, &af);
    duration = 16 - ch->has_skill(SKILL_CRAZED_ASSAULT) / 10;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  af.type = SKILL_CRAZED_ASSAULT;
  af.duration = duration;
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  return ReturnValue::eSUCCESS;
}

void rush_reset(varg_t arg1, void *arg2, void *arg3)
{
  CharacterPtr ch = arg1.ch;
  extern bool charExists(CharacterPtr ch);
  if (!charExists(ch))
    return;
  REMBIT(ch->affected_by, AFF_RUSH_CD);
}

command_return_t do_bullrush(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString direction;
  QString who;
  qint32 dir = {};
  qint32 retval;
  CharacterPtr victim;

  if (ch->getHP() == 1)
  {
    ch->sendln("You are feeling too weak right now for rushing to and fro.");
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_RUSH_CD))
  {
    ch->sendln("You must take a moment to gather your strength before another rush!");
    return ReturnValue::eFAILURE;
  }
  if (!ch->canPerform(SKILL_BULLRUSH, "Closest yer gonna get to a bull right now is a Red one..and you have to drink it...\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, who);
  one_argument(argument, direction);
  if (direction.isEmpty())
  {
    ch->sendln("Bullrush which direction?");
    return ReturnValue::eFAILURE;
  }
  if (who.isEmpty())
  {
    ch->sendln("Bullrush on.. who?");
    return ReturnValue::eFAILURE;
  }

  for (qint32 i = {}; i < 6; i++)
  {
    if (!str_prefix(direction, dirs[i]))
    {
      dir = i + 1;
      break;
    }
  }

  if (!dir)
  {
    ch->sendln("Bullrush a valid direction dumb barb...like north maybe?");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BULLRUSH))
    return ReturnValue::eSUCCESS;

  // before we move anyone, we need to check for any spec procs in the
  // room like guild guards
  auto cmd_dir = getCommandFromDirection(dir);
  if (cmd_dir)
  {
    retval = ch->special("", *cmd_dir);
    if (isSet(retval, ReturnValue::eSUCCESS) || isSet(retval, ReturnValue::eCH_DIED))
      return retval;
  }

  SETBIT(ch->affected_by, AFF_RUSH_CD);
  extern void addtimer(timer_data * add);

  // Reset bullrush AFF in 5 seconds
  timer_data *timer = new timer_data;
  timer->arg1.ch = ch;
  timer->function = rush_reset;
  timer->timeleft = 5;
  addtimer(timer);

  if (cmd_dir)
  {
    retval = attempt_move(ch, *cmd_dir);
    if (SOMEONE_DIED(retval))
      return retval;
  }

  if (!(victim = ch->get_char_room_vis(who)))
  {
    ch->sendln("You charge in, but are left confused by the complete lack of such a target!");
    //     WAIT_STATE(ch,DC::PULSE_VIOLENCE/2);
    return ReturnValue::eFAILURE;
  }
  //  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  if (!skill_success(ch, victim, SKILL_BULLRUSH))
  {
    ch->sendln("You rush in madly and fail to find your target!");
    act("$n rushes into the room with nostrils flaring then looks around sheepishly.",
        ch, nullptr, nullptr, TO_ROOM, NOTVICT);
    return ReturnValue::eFAILURE;
  }

  if (!victim || victim == ch)
  {
    ch->sendln("You successfully rush in and bushwack... the air.");
    return ReturnValue::eFAILURE;
  }

  act("$n rushes into the room with an amazingly violent speed!",
      ch, nullptr, nullptr, TO_ROOM, NOTVICT);
  return attack(ch, victim, TYPE_UNDEFINED);
}

command_return_t do_ferocity(CharacterPtr ch, QString argument, cmd_t cmd)
{
  affected_type af;

  if (!ch->canPerform(SKILL_FEROCITY, "You're just not angry enough!\r\n"))
  {
    return ReturnValue::eFAILURE;
  }
  if (ch->affected_by_spell(SKILL_FEROCITY_TIMER))
  {
    ch->sendln("It is too soon to try and rile up the masses!");
    return ReturnValue::eFAILURE;
  }
  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You have no group to inspire.");
    return ReturnValue::eFAILURE;
  }

  qint32 grpsize = {};
  for (CharacterPtr tmp_char = dc_->world[ch->in_room].people_; tmp_char; tmp_char = tmp_char->next_in_room)
  {
    if (tmp_char == ch)
      continue;
    if (!ARE_GROUPED(ch, tmp_char))
      continue;
    grpsize++;
  }

  if (!charge_moves(ch, SKILL_FEROCITY, grpsize))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_FEROCITY))
  {
    ch->sendln("Guess you just weren't that angry.");
    act_to_room("$n tries to rile you up but just seems to be pouty.", ch, 0, 0, 0);
  }
  else
  {
    act_to_room("$n lets out a deafening roar!", ch, 0, 0, 0);
    ch->sendln("Your heart beats adrenaline through your body and you roar with ferocity!");

    af.type = SKILL_FEROCITY_TIMER;
    af.duration = 1 + ch->has_skill(SKILL_FEROCITY) / 10;
    af.location = {};
    af.bitvector = -1;
    af.modifier = {};
    affect_to_char(ch, &af);

    for (CharacterPtr tmp_char = dc_->world[ch->in_room].people_; tmp_char; tmp_char = tmp_char->next_in_room)
    {
      if (tmp_char == ch)
        continue;
      if (!ARE_GROUPED(ch, tmp_char))
        continue;

      affect_from_char(tmp_char, SKILL_FEROCITY, SUPPRESS_MESSAGES);
      affect_from_char(tmp_char, SKILL_FEROCITY, SUPPRESS_MESSAGES);
      act_to_victim("$n's fierce roar gets your adrenaline pumping!", ch, 0, tmp_char, 0);

      af.type = SKILL_FEROCITY;
      af.duration = 1 + ch->has_skill(SKILL_FEROCITY) / 10;
      af.modifier = 31 + ch->has_skill(SKILL_FEROCITY) / 2;
      af.location = APPLY_HIT;
      af.bitvector = -1;
      affect_to_char(tmp_char, &af);
      af.modifier = 1 + ch->has_skill(SKILL_FEROCITY) / 15;
      af.location = APPLY_HP_REGEN;
      affect_to_char(tmp_char, &af);
    }
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  return ReturnValue::eSUCCESS;
}

void barb_magic_resist(CharacterPtr ch, qint32 old, qint32 nw)
{
  qint32 bonus = 0, i;
  qint32 oldbonus = (old / 10) + 1;
  bonus = (nw / 10 + 1) - oldbonus;
  if (bonus)
    for (i = {}; i <= SAVE_TYPE_MAX; i++)
      ch->saves[i] += bonus;
}

command_return_t do_knockback(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString buf, where, who;
  qint32 dir = {};
  qint32 retval, dam, dampercent, learned;

  if (ch->getHP() == 1)
  {
    ch->sendln("You are feeling too weak right now to smash into anybody.");
    return ReturnValue::eFAILURE;
  }

  if (!ch->canPerform(SKILL_KNOCKBACK, "You'd bounce off of your opponent before you caused any damage.\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, who);
  one_argument(argument, where);

  if (who.isEmpty())
  {
    ch->sendln("Knockback whom?");
    return ReturnValue::eFAILURE;
  }

  victim = ch->get_char_room_vis(who);

  if (victim == nullptr)
    victim = ch->fighting;

  if (victim == nullptr)
  {
    ch->sendln("Knockback whom?");
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Where do you want to go today?");
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return ReturnValue::eFAILURE;

  if (!charge_moves(ch, SKILL_KNOCKBACK))
    return ReturnValue::eSUCCESS;

  bool victim_paralyzed = false;
  affected_type *af;
  if ((af = victim->affected_by_spell(SPELL_PARALYZE)))
  {
    victim_paralyzed = true;
    if (af->duration >= 1)
      af->duration--;
  }
  else
  {
    if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_HUGE))
    {
      ch->sendln("You are too tiny to knock someone that HUGE anywhere!");
      return ReturnValue::eFAILURE;
    }

    if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_SWARM))
    {
      ch->sendln("You cannot pick just one to knockback!");
      return ReturnValue::eFAILURE;
    }

    if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_TINY))
    {
      act_to_character("$N would evade your knockback attempt with ease!", ch, 0, victim, 0);
      return ReturnValue::eFAILURE;
    }

    if (IS_AFFECTED(victim, AFF_STABILITY) && dc_->number(0, 3) == 0)
    {
      act_to_character("You bounce off of $N and crash into the ground.", ch, 0, victim, 0);
      act_to_room("$n bounces off of $N and crashes into the ground.", ch, 0, victim, NOTVICT);
      act_to_victim("$n bounces off of you and crashes into the ground.", ch, 0, victim, 0);
      WAIT_STATE(ch, DC::PULSE_VIOLENCE);
      return ReturnValue::eFAILURE;
    }
  }

  learned = ch->has_skill(SKILL_KNOCKBACK);
  dam = 100;

  if (!where.isEmpty())
  {
    if (learned < 80)
      ch->sendln("You're not good enough to direct your smashes, so you just let it fly!");
    else
    {
      for (qint32 i = {}; i < 6; i++)
      {
        if (!str_prefix(where, dirs[i]))
        {
          dir = i + 1;
          break;
        }
      }
      learned -= 20;
    }
  }

  if (!dir)
    dir = dc_->number(1, 6);
  dir--;
  dampercent = {};
  if (ch->height > 102)
    dampercent += 15;
  else if (ch->height > 42)
    dampercent += 7;
  if (victim->height < 42)
    dampercent += 7;
  else if (victim->height < 102)
    dampercent += 15;

  dam = (qint32)(dam * (1.0 + dampercent / 100.0));

  QString buf2, dammsg[20];
  qint32 prevhps = victim->getHP();

  if (!victim_paralyzed && !skill_success(ch, victim, SKILL_KNOCKBACK, 0 - (learned / 4 * 3)))
  {
    act_to_character("You lunge forward in an attempt to smash $N but fall, missing $M completely.", ch, 0, victim, 0);
    act_to_room("$n lunges forward in an attempt to smash into $N but falls flat on $s face.", ch, 0, victim, NOTVICT);
    act_return ar = act_to_victim("$n lunges forward in an attempt to smash into you but falls flat on $s face, missing completely.", ch, 0, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED))
    {
      return retval;
    }

    ch->setSitting();
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    retval = damage(ch, victim, 0, TYPE_CRUSH, SKILL_KNOCKBACK);
    return ReturnValue::eFAILURE;
  }
  else if (!victim_paralyzed && victim->affected_by_spell(SKILL_BATTLESENSE) &&
           dc_->number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
  {
    act_to_character("$N's heightened battlesense sees your smash coming from a mile away and $E easily sidesteps it.", ch, 0, victim, 0);
    act_to_room("$N's heightened battlesense sees $n's smash coming from a mile away and $N easily sidesteps it.", ch, 0, victim, NOTVICT);
    act_return ar = act_to_victim("Your heightened battlesense sees $n's smash coming from a mile away and you easily sidestep it.", ch, 0, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED))
    {
      return retval;
    }

    ch->setSitting();
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    return ReturnValue::eFAILURE;
  }
  else if (CAN_GO(victim, dir) &&
           !victim->affected_by_spell(SPELL_IRON_ROOTS) &&
           !isSet(dc_->world[EXIT(victim, dir)->to_room].room_flags, IMP_ONLY) &&
           !isSet(dc_->world[EXIT(victim, dir)->to_room].room_flags, NO_TRACK) &&
           (!IS_AFFECTED(victim, AFF_CHAMPION) || champion_can_go(EXIT(victim, dir)->to_room)) &&
           class_can_go(GET_CLASS(victim), EXIT(victim, dir)->to_room))
  {
    // need to do more checks on if the victim can actually be knocked into
    // the room?
    QString temp; // what did my innocent bugfix ever do to you?
    dc_sprintf(temp, "%s", qPrintable(victim->shortdesc_or_name()));
    retval = damage(ch, victim, dam, TYPE_CRUSH, SKILL_KNOCKBACK);
    if (SOMEONE_DIED(retval))
    {
      dc_sprintf(buf, "You smash %s apart!", temp);
      act_to_character(buf, ch, 0, 0, 0);
      dc_sprintf(buf, "$n smashes %s to pieces!", temp);
      act_to_room(buf, ch, 0, 0, 0);
      return retval; // this too, just in case it gets called from  a
                     // proc later on, it returns correct stuff
    }
    else
    {
      dc_sprintf(dammsg, "$B%d$R", prevhps - victim->getHP());
      dc_sprintf(buf2, "Your smash for | damage sends %s reeling %s.", qPrintable(victim->shortdesc_or_name()), dirs[dir]);
      dc_sprintf(buf, "Your smash sends %s reeling %s.", qPrintable(victim->shortdesc_or_name()), dirs[dir]);
      send_damage(buf2, ch, 0, victim, dammsg, buf, TO_CHAR);
      dc_sprintf(buf2, "%s smashes into you for | damage, sending you reeling %s.", qPrintable(ch->name()), dirs[dir]);
      dc_sprintf(buf, "%s smashes into you, sending you reeling %s.", qPrintable(ch->name()), dirs[dir]);
      send_damage(buf2, ch, 0, victim, dammsg, buf, TO_VICT);

      if (selfpurge)
        return ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED;
      dc_sprintf(buf2, "%s smashes into %s for | damage and sends $S ass reeling to the %s.", qPrintable(ch->name()), qPrintable(victim->shortdesc_or_name()), dirs[dir]);
      dc_sprintf(buf, "%s smashes into %s and sends $S ass reeling to the %s.", qPrintable(ch->name()), qPrintable(victim->shortdesc_or_name()), dirs[dir]);
      send_damage(buf2, ch, 0, victim, dammsg, buf, TO_ROOM);

      if (victim->fighting)
      {
        if (victim->isNonPlayer())
        {
          victim->add_memory(qPrintable(ch->name()), 'h');
          remove_memory(victim, 'f');
        }

        CharacterPtr tmp;
        for (tmp = dc_->world[ch->in_room].people_; tmp; tmp = tmp->next_in_room)
          if (tmp->fighting == victim)
            stop_fighting(tmp);
        stop_fighting(victim);
      }
      move_char(victim, (dc_->world[(ch)->in_room].dir_option[dir])->to_room);
    }
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    return ReturnValue::eSUCCESS;
  }
  else
  {
    QString temp;
    dc_sprintf(temp, "%s", qPrintable(victim->shortdesc_or_name()));
    retval = damage(ch, victim, dam, TYPE_CRUSH, SKILL_KNOCKBACK);
    if (SOMEONE_DIED(retval))
    {
      dc_sprintf(buf, "You smash %s apart!", temp);
      act_to_character(buf, ch, 0, 0, 0);
      dc_sprintf(buf, "$n smashes %s to pieces!", temp);
      act_to_room(buf, ch, 0, 0, 0);
      return retval;
    }
    else
    {
      dc_sprintf(dammsg, "$B%d$R", prevhps - victim->getHP());
      send_damage("$N backpeddles across the room due to $n's smash for | damage.", ch, 0, victim, dammsg,
                  "$N backpessles across the room due to $n's smash.", TO_ROOM);
      send_damage("$N barely keeps $S footing, stumbling backwards after your smash and taking | damage.", ch, 0, victim, dammsg,
                  "$N barely keeps $S footing, stumbling backwards after your smash.", TO_CHAR);
      send_damage("$n knocks you back across the room for | damage.", ch, 0, victim, dammsg,
                  "$n knocks you back across the room.", TO_VICT);
      WAIT_STATE(ch, DC::PULSE_VIOLENCE);
      return attack(victim, ch, TYPE_UNDEFINED);
    }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_primalfury(CharacterPtr ch, QString argument, cmd_t cmd)
{
  affected_type af;

  if (!ch->has_skill(SKILL_PRIMAL_FURY))
  {
    ch->sendln("You don't know how to.");
    return ReturnValue::eSUCCESS;
  }
  if (ch->affected_by_spell(SKILL_PRIMAL_FURY))
  {
    ch->sendln("You must wait before using this ability again.");
    return ReturnValue::eSUCCESS;
  }
  if (!ch->fighting || GET_POS(ch) != position_t::FIGHTING)
  {
    ch->sendln("You must be in combat in order to use this ability.");
    return ReturnValue::eSUCCESS;
  }
  if (!charge_moves(ch, SKILL_PRIMAL_FURY))
    return ReturnValue::eSUCCESS;

  if (GET_RAW_STR(ch) < 16)
  {
    ch->sendln("You do not possess sufficient strength to attempt this feat.");
    return ReturnValue::eSUCCESS;
  }

  // Timer applies to both success and failure, so it goes here.
  af.type = SKILL_PRIMAL_FURY;
  af.duration = 40;
  af.modifier = {};
  af.location = {};
  af.bitvector = -1;
  affect_to_char(ch, &af);

  if (!skill_success(ch, nullptr, SKILL_PRIMAL_FURY))
  {
    affect_to_char(ch, &af);
    ch->sendln("You attempt to let forth a primal scream, but manage only a squeak...how embarassing!");
    act_to_room("$n attempts to let forth a primal scream, but manages only a squeak...how embarassing!", ch, nullptr, nullptr, 0);
    ch->setMove(ch->getMove() / 2.0);
    return ReturnValue::eSUCCESS;
  }

  // Success below.

  ch->decrementMove(40);
  if (ch->dc_->number(1, 101) > (5 + ch->has_skill(SKILL_PRIMAL_FURY) / 5))
  { // Str loss.
    GET_RAW_STR(ch) -= 1;
    affect_modify(ch, APPLY_STR, 0, -1, true);
    ch->send("You lose one point of strength.");
    dc_->logf(OVERSEER, DC::LogChannel::LOG_MORTAL, "Statloss: %s lost one point of strength through primal fury.", qPrintable(ch->name()));
  }

  // rest already set
  af.duration = 2 + ch->has_skill(SKILL_PRIMAL_FURY) / 30;
  af.bitvector = AFF_PRIMAL_FURY;
  affect_to_char(ch, &af);

  act_to_room("$n lets forth a primal scream of anger and begins to fight with terrible fury!", ch, nullptr, nullptr, 0);
  ch->sendln("You let forth a primal scream of anger and fall upon your enemies with a terrible fury!");

  return ReturnValue::eSUCCESS;
}

command_return_t do_pursue(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (!ch->has_skill(SKILL_PURSUIT))
  {
    ch->sendln("You don't know how to.");
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_PURSUIT))
  {
    ch->sendln("You will no longer pursue your victims.");
    affect_from_char(ch, SKILL_PURSUIT);
  }
  else
  {
    ch->sendln("You will pursue your victims.");
    affected_type af;
    af.type = SKILL_PURSUIT;
    af.duration = -1;
    af.modifier = {};
    af.location = {};
    af.bitvector = -1;
    affect_to_char(ch, &af);
  }

  return ReturnValue::eSUCCESS;
}
