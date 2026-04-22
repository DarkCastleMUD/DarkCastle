/************************************************************************
| $Id: group.cpp,v 1.30 2011/11/29 02:21:54 jhhudso Exp $
| group.C
| Description:  Group related commands; join, abandon, follow, etc..
*/
#include "DC/DC.h"

ReturnValue do_abandon(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr k;
  QString buf;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }

  if ((!ch->master) && (IS_AFFECTED(ch, AFF_GROUP)))
  {
    ch->sendln(u"You can't abandon a group you're leading."_s);
    ch->sendln(u"You must disband the group."_s);
    return ReturnValue::eFAILURE;
  }

  if ((!ch->master) || (!IS_AFFECTED(ch, AFF_GROUP)))
  {
    ch->sendln(u"Who you gonna abandon?!"_s);
    return ReturnValue::eFAILURE;
  }

  if ((ch->isNonPlayer()) && (IS_AFFECTED(ch, AFF_CHARM)))
  {
    ch->sendln(u"You're in love. Forget it!"_s);
    return ReturnValue::eFAILURE;
  }

  stop_grouped_bards(ch, 1);

  k = ch->master;

  dc_sprintf(buf, "You abandon: %s\r\n", k->group_name);
  ch->send(buf);
  dc_sprintf(buf, "%s abandons: %s", qPrintable(ch->shortdesc_or_name()), k->group_name);
  act_to_room(buf, ch, 0, 0, 0);

  if (ch->isPlayer())
  {
    ch->player->group_pkills = {};
    ch->player->grpplvl = {};
    ch->player->group_kills = {};
  }

  stop_follower(ch);
  return ReturnValue::eSUCCESS;
}

ReturnValue do_found(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }

  if (argument.isEmpty())
  {
    ch->sendln(u"Found what?!"_s);
    return ReturnValue::eFAILURE;
  }
  if ((ch->master) && (!IS_AFFECTED(ch, AFF_GROUP)))
  {
    ch->sendln(u"You can't found your own group while following someone around!"_s);
    return ReturnValue::eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln(u"You can't found a group if you're already in one!"_s);
    return ReturnValue::eFAILURE;
  }

  for (; isspace(*argument); argument++)
    ;

  if (dc_strlen(argument) > 50)
  {
    ch->sendln(u"You gonna name your party? Or write a book?!  50 characters max."_s);
    return ReturnValue::eFAILURE;
  }

  ch->group_name_ = argument;
  dc_sprintf(buf, "You found: %s\r\n", argument);
  ch->send(buf);
  dc_sprintf(buf, "%s founds: %s", qPrintable(ch->shortdesc_or_name()), argument);
  act_to_room(buf, ch, 0, 0, 0);

  if (ch->isPlayer())
  {
    ch->player->group_pkills = {};
    ch->player->grpplvl = {};
    ch->player->group_kills = {};
  }

  SETBIT(ch->affected_by, AFF_GROUP);
  REMOVE_BIT(ch->player->toggles, Player::PLR_LFG);
  return ReturnValue::eSUCCESS;
}

ReturnValue Character::do_split(QStringList arguments, cmd_t cmd)
{
  quint64 share = 0, extra = {};
  quint64 no_members = {};
  CharacterPtr k = {};
  follow_type *f = {};

  if (arguments.isEmpty())
  {
    send(u"Split what?\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  if (isPlayerGoldThief())
  {
    send(u"Nobody wants any part of your stolen booty!\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  QString number = arguments.at(0);

  bool ok = false;
  const qulonglong amount = number.toULongLong(&ok);
  if (ok == false)
  {
    send(u"Invalid value.\r\n"_s);
    send(u"Valid values are %1 to %2.\r\n"_s.arg(1).arg(static_cast<quint64>(-1)));
    return ReturnValue::eFAILURE;
  }

  if (amount == 0)
  {
    send(u"You hand out zero coins to everyone, but no one notices.\r\n"_s);
    return ReturnValue::eSUCCESS;
  }

  if (getGold() < amount)
  {
    send(u"You don't have that much gold!\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  if (master)
    k = master;
  else
    k = this;

  if ((!IS_AFFECTED(k, AFF_GROUP)) || (!IS_AFFECTED(this, AFF_GROUP)))
  {
    send(u"You are not grouped. You must be grouped to split your money!\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  if (k->in_room == in_room)
    no_members = 1;
  else
    no_members = {};

  for (f = k->followers; f; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) &&
        (f->follower->in_room == in_room) &&
        !f->follower->isNonPlayer())
      no_members++;
  }

  if (no_members == 0) // should be impossible
  {
    send(u"You got a divide by 0.  Tell a god how.\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  share = amount / no_members;
  extra = amount % no_members;
  removeGold(amount);
  save(cmd_t::SAVE_SILENTLY);

  send(u"You split %L1 $B$5gold$R coins. Your share is %L2 gold coins.\r\n"_s.arg(amount).arg(share + extra));
  addGold(share + extra);

  if (k != this && k->in_room == in_room)
  {
    k->send(u"%1 splits %L2 $B$5gold$R coins. Your share is %L3 $B$5gold$R coins.\r\n"_s.arg(qPrintable(shortdesc_or_name())).arg(amount).arg(share));
    qint32 lost = {};
    if (k->clan && get_clan(k)->tax && !isSet(GET_TOGGLES(k), Player::PLR_NOTAX) &&
        (k->clan != clan || (k->clan == clan && isSet(GET_TOGGLES(this), Player::PLR_NOTAX))))
    {
      lost = (qint32)((qreal)share * (qreal)((qreal)get_clan(k)->tax / 100));
      k->send(u"Your clan taxes %L1 $B$5gold$R of your share.\r\n"_s.arg(lost));
      get_clan(k)->cdeposit(lost);
      save_clans();
    }
    k->addGold(share - lost);
  }
  for (f = k->followers; f; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) &&
        f->follower->in_room == in_room &&
        f->follower != this &&
        !f->follower->isNonPlayer())
    {
      f->follower->send(u"%1 splits %L2 $B$5gold$R coins. Your share is %L3 $B$5gold$R coins.\r\n"_s.arg(qPrintable(shortdesc_or_name())).arg(amount).arg(share));
      qint32 lost = {};
      if (f->follower->clan && get_clan(f->follower)->tax && !isSet(GET_TOGGLES(f->follower), Player::PLR_NOTAX) &&
          (f->follower->clan != clan || (f->follower->clan == clan && isSet(GET_TOGGLES(this), Player::PLR_NOTAX))))
      {
        lost = (qint32)((qreal)share * (qreal)((qreal)get_clan(f->follower)->tax / 100));
        f->follower->send(u"Your clan taxes %L1 gold of your share.\r\n"_s.arg(lost));
        get_clan(f->follower)->cdeposit(lost);
        save_clans();
      }
      f->follower->addGold(share - lost);
    }
  }
  return ReturnValue::eSUCCESS;
}

void setup_group_buf(QString report, CharacterPtr j, CharacterPtr i)
{
  if (j->isNonPlayer() || (IS_ANONYMOUS(j) && (i->clan != j->clan || !i->clan)))
  {
    if (GET_CLASS(j) == CLASS_MONK || GET_CLASS(j) == CLASS_BARD)
      dc_sprintf(report, "[-====-|      %3d%%    hp     %3d%%   k     %3d%%   mv]",
                 MAX(1, j->getHP()) * 100 / MAX(1, GET_MAX_HIT(j)),
                 MAX(1, GET_KI(j)) * 100 / MAX(1, GET_MAX_KI(j)),
                 MAX(1, GET_MOVE(j)) * 100 / MAX(1, GET_MAX_MOVE(j)));
    else if (GET_CLASS(j) == CLASS_WARRIOR || GET_CLASS(j) == CLASS_THIEF ||
             GET_CLASS(j) == CLASS_BARBARIAN)
      dc_sprintf(report, "[-====-|      %3d%%    hp    -====-        %3d%%   mv]",
                 MAX(1, j->getHP()) * 100 / MAX(1, GET_MAX_HIT(j)),
                 MAX(1, GET_MOVE(j)) * 100 / MAX(1, GET_MAX_MOVE(j)));
    else
      dc_sprintf(report, "[-====-|      %3d%%    hp     %3d%%   m     %3d%%   mv]",
                 MAX(1, j->getHP()) * 100 / MAX(1, GET_MAX_HIT(j)),
                 MAX(1, GET_MANA(j)) * 100 / MAX(1, GET_MAX_MANA(j)),
                 MAX(1, GET_MOVE(j)) * 100 / MAX(1, GET_MAX_MOVE(j)));
  }
  else
  {
    if (i->isPlayer() && isSet(i->player->toggles, Player::PLR_ANSI))
    {
      if (GET_CLASS(j) == CLASS_MONK || GET_CLASS(j) == CLASS_BARD)
        dc_sprintf(report, "[Lv %3llu| %s%6d%s/%-6dhp %s%5d%s/%-5dk %s%5d%s/%-5dmv]",
                   j->getLevel(),
                   calc_color(j->getHP(), GET_MAX_HIT(j)), j->getHP(), NTEXT, GET_MAX_HIT(j),
                   calc_color(GET_KI(j), GET_MAX_KI(j)), GET_KI(j), NTEXT, GET_MAX_KI(j),
                   calc_color(GET_MOVE(j), GET_MAX_MOVE(j)), GET_MOVE(j), NTEXT, GET_MAX_MOVE(j));
      else if (GET_CLASS(j) == CLASS_WARRIOR || GET_CLASS(j) == CLASS_THIEF ||
               GET_CLASS(j) == CLASS_BARBARIAN)
        dc_sprintf(report, "[Lv %3llu| %s%6d%s/%-6dhp    -====-    %s%5d%s/%-5dmv]",
                   j->getLevel(),
                   calc_color(j->getHP(), GET_MAX_HIT(j)), j->getHP(), NTEXT, GET_MAX_HIT(j),
                   calc_color(GET_MOVE(j), GET_MAX_MOVE(j)), GET_MOVE(j), NTEXT, GET_MAX_MOVE(j));
      else
        dc_sprintf(report, "[Lv %3llu| %s%6d%s/%-6dhp %s%5d%s/%-5dm %s%5d%s/%-5dmv]",
                   j->getLevel(),
                   calc_color(j->getHP(), GET_MAX_HIT(j)), j->getHP(), NTEXT, GET_MAX_HIT(j),
                   calc_color(GET_MANA(j), GET_MAX_MANA(j)), GET_MANA(j), NTEXT, GET_MAX_MANA(j),
                   calc_color(GET_MOVE(j), GET_MAX_MOVE(j)), GET_MOVE(j), NTEXT, GET_MAX_MOVE(j));
    }
    else
    {
      if (GET_CLASS(j) == CLASS_MONK || GET_CLASS(j) == CLASS_BARD)
        dc_sprintf(report, "[Lv %3llu| %6d/%-6dhp %5d/%-5dk %5d/%-5dmv]",
                   j->getLevel(), j->getHP(), GET_MAX_HIT(j), GET_KI(j),
                   GET_MAX_KI(j), GET_MOVE(j), GET_MAX_MOVE(j));
      else if (GET_CLASS(j) == CLASS_WARRIOR || GET_CLASS(j) == CLASS_THIEF ||
               GET_CLASS(j) == CLASS_BARBARIAN)
        dc_sprintf(report, "[Lv %3llu| %6d/%-6dhp    -====-    %5d/%-5dmv]",
                   j->getLevel(), j->getHP(), GET_MAX_HIT(j),
                   GET_MOVE(j), GET_MAX_MOVE(j));
      else
        dc_sprintf(report, "[Lv %3llu| %6d/%-6dhp %5d/%-5dm %5d/%-5dmv]",
                   j->getLevel(), j->getHP(), GET_MAX_HIT(j), GET_MANA(j),
                   GET_MAX_MANA(j), GET_MOVE(j), GET_MAX_MOVE(j));
    }
  }
}

ReturnValue do_group(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  QString buf, report;
  CharacterPtr victim, k, j;
  follow_type *f;
  bool found;

  one_argument(argument, name);

  if (name.isEmpty())
  {
    if (!IS_AFFECTED(ch, AFF_GROUP))
      ch->sendln(u"But you are a member of no group?!"_s);
    else
    {
      ch->sendln(u"Your group consists of:"_s);
      if (ch->master)
        k = ch->master;
      else
        k = ch;

      setup_group_buf(report, k, ch);
      dc_sprintf(buf, "%s    $N (Leader)", report);

      if (IS_AFFECTED(k, AFF_GROUP))
        act_to_character(buf, ch, 0, k, ASLEEP);

      for (f = k->followers; f; f = f->next)
      {
        if (IS_AFFECTED(f->follower, AFF_GROUP))
        {
          j = f->follower;
          setup_group_buf(report, j, ch);
          dc_sprintf(buf, "%s    $N", report);
          act_to_character(buf, ch, 0, f->follower, ASLEEP);
        }
      }
    }

    return ReturnValue::eSUCCESS;
  }

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!(victim = ch->get_char_room_vis(name)))
    ch->sendln(u"No one here by that name."_s);

  else
  {
    if (!IS_AFFECTED(ch, AFF_GROUP))
    {
      ch->sendln(u"You must first found a group!"_s);
      return ReturnValue::eFAILURE;
    }

    if (ch->master)
    {
      act_to_character("You can not enroll group members without being head of a group.", ch, 0, 0, 0);
      return ReturnValue::eFAILURE;
    }

    found = false;

    if (victim == ch)
      found = true;
    else
    {
      for (f = ch->followers; f; f = f->next)
      {
        if (f->follower == victim)
        {
          found = true;
          break;
        }
      }
    }

    if (found)
    {
      if (ch == victim)
      {
        ch->sendln(u"You must found a group, or Disband a group."_s);
        return ReturnValue::eFAILURE;
      }

      if (IS_AFFECTED(victim, AFF_GROUP))
      {
        stop_grouped_bards(victim, 1);
        act_to_room("$n has been kicked out of the group!", victim, 0, ch, 0);
        act_to_character("You are no longer a member of the group!", victim, 0, 0, ASLEEP);
        REMBIT(victim->affected_by, AFF_GROUP);
      }
      else
      {
        act_to_room("$n is now a group member.", victim, 0, 0, 0);
        act_to_character("You are now a group member.", victim, 0, 0, ASLEEP);
        SETBIT(victim->affected_by, AFF_GROUP);
        if (victim->isPlayer())
          REMOVE_BIT(victim->player->toggles, Player::PLR_LFG);
      }
      return ReturnValue::eSUCCESS;
      //    }
      //  else
      //	  act_to_room("$n is not of the right caliber to join this group.", victim, 0, 0,  ASLEEP);
    }
    else
      act_to_character("$N must follow you, to enter the group.", ch, 0, victim, ASLEEP);
  }
  return ReturnValue::eFAILURE;
}

ReturnValue do_promote(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  QString buf;
  CharacterPtr new_new_leader, k;
  follow_type *f, *next_f;

  one_argument(argument, name);

  if (ch->master)
  {
    ch->sendln(u"You aren't running the show here, pal."_s);
    return ReturnValue::eFAILURE;
  }

  if ((!ch->master) && !IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln(u"You don't even have a group to promote anyone."_s);
    return ReturnValue::eFAILURE;
  }

  if (name.isEmpty())
  {
    ch->sendln(u"Who do you wish to promote to group leader? "_s);
    return ReturnValue::eFAILURE;
  }

  if (!(new_new_leader = ch->get_char_room_vis(name)))
  {
    ch->sendln(u"I see no person by that name here!"_s);
    return ReturnValue::eFAILURE;
  }

  if (new_new_leader == ch)
  {
    ch->sendln(u"You truly must have an EGO the size of the Tarrasque!"_s);
    send_to_char("You are already the group leader! Maybe you SHOULD "
                 "step down?\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  if (new_new_leader->isNonPlayer())
  {
    ch->sendln(u"Yeah right!"_s);
    return ReturnValue::eFAILURE;
  }

  if (!ARE_GROUPED(ch, new_new_leader))
  {
    ch->sendln(u"But you aren't even in the same group!"_s);
    return ReturnValue::eFAILURE;
  }

  dc_sprintf(buf, "You step down, appointing %s as the new leader.\r\n",
             qPrintable(new_new_leader->shortdesc_or_name()));
  ch->send(buf);
  dc_sprintf(buf, "%s steps down as leader of: %s\r\n%s appoints YOU as "
                  "the New Leader of: %s\r\n",
             qPrintable(ch->shortdesc_or_name()), ch->group_name,
             qPrintable(ch->shortdesc_or_name()), ch->group_name);
  new_new_leader->send(buf);
  dc_sprintf(buf, "%s steps down as leader of: %s\r\n%s appoints %s as "
                  "the New Leader of: %s",
             qPrintable(ch->shortdesc_or_name()), ch->group_name,
             qPrintable(ch->shortdesc_or_name()), qPrintable(new_new_leader->shortdesc_or_name()), ch->group_name);
  act_to_room(buf, ch, 0, new_new_leader, NOTVICT);

  if (ch->isPlayer() && new_new_leader->isPlayer())
  {
    new_new_leader->player->grpplvl = ch->player->grpplvl;
    new_new_leader->player->group_pkills = ch->player->group_pkills;
    new_new_leader->player->group_kills = ch->player->group_kills;
    ch->player->group_pkills = {};
    ch->player->grpplvl = {};
    ch->player->group_kills = {};
  }

  if (ch->group_name)
  {
    new_new_leader->group_name = ch->group_name;
    ch->group_name = {};
  }
  else
    new_new_leader->group_name = u"I am a dork"_s;

  stop_follower(new_new_leader, follower_reasons_t::CHANGE_LEADER);

  for (f = ch->followers; f; f = next_f)
  {
    next_f = f->next;
    if (f->follower->isPlayer())
    {
      k = f->follower;
      stop_follower(k, follower_reasons_t::CHANGE_LEADER);
      add_follower(k, new_new_leader, follower_reasons_t::CHANGE_LEADER);
    }
    else
      REMBIT(f->follower->affected_by, AFF_GROUP);
  }

  add_follower(ch, new_new_leader, follower_reasons_t::CHANGE_LEADER);
  return ReturnValue::eSUCCESS;
}

ReturnValue do_disband(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  QString buf;
  CharacterPtr adios, k;
  follow_type *f, *next_f;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);

  if (ch->master)
  {
    ch->sendln(u"You aren't running the show here pal!"_s);
    return ReturnValue::eFAILURE;
  }

  if ((!ch->master) && (!IS_AFFECTED(ch, AFF_GROUP)))
  {
    ch->sendln(u"You don't even have a group to disband."_s);
    return ReturnValue::eFAILURE;
  }

  if (name.isEmpty())
  {
    ch->sendln(u"Who do you wish to disband? "_s);
    ch->sendln(u"Disband 'all' will disband the group."_s);
    return ReturnValue::eFAILURE;
  }

  if (isexact(name, "all"))
  {
    k = ch;
    dc_sprintf(buf, "You disband your group: %s", k->group_name);
    act_to_character(buf, k, 0, 0, 0);
    dc_sprintf(buf, "$n disbands $s group: %s", k->group_name);
    act_to_room(buf, k, 0, 0, 0);

    k->group_name = {};
    k->group_name = {};

    if (k->isPlayer())
    {
      k->player->group_pkills = {};
      k->player->grpplvl = {};
      k->player->group_kills = {};
    }

    for (f = k->followers; f; f = next_f)
    {
      next_f = f->next;
      if (f->follower->isPlayer())
      {
        stop_grouped_bards(f->follower, 1);
        stop_follower(f->follower);
      }
      else
        REMBIT(f->follower->affected_by, AFF_GROUP);
    }

    REMBIT(k->affected_by, AFF_GROUP);
    return ReturnValue::eSUCCESS;
  }

  if (!(adios = ch->get_char_room_vis(name)))
  {
    ch->sendln(u"I see no person by that name here!"_s);
    return ReturnValue::eFAILURE;
  }

  if (adios == ch)
  {
    ch->sendln(u"You can't disband yourself from a group you're leading!"_s);
    ch->sendln(u"Either Promote someone to Group Leader, or Disband All."_s);
    return ReturnValue::eFAILURE;
  }

  if (adios->master != ch)
  {
    ch->sendln(u"Try someone in YOUR group, fartknocker."_s);
    return ReturnValue::eFAILURE;
  }

  if ((adios->isNonPlayer()) && (IS_AFFECTED(adios, AFF_CHARM)))
  {
    ch->sendln(u"Can't kick out a charmee."_s);
    return ReturnValue::eFAILURE;
  }

  stop_grouped_bards(adios, 1);
  if (adios->isPlayer())
  {
    adios->player->grpplvl = {};
    adios->player->group_pkills = {};
    adios->player->group_kills = {};
  }
  stop_follower(adios);
  return ReturnValue::eSUCCESS;
}

ReturnValue do_follow(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  CharacterPtr leader;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);

  if (!name.isEmpty())
  {
    if (!(leader = get_char_room(name, ch->in_room)))
    {
      ch->sendln(u"I see no person by that name here!"_s);
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    ch->sendln(u"Who do you wish to follow?"_s);
    return ReturnValue::eFAILURE;
  }
  if (cmd == cmd_t::DEFAULT && !CAN_SEE(ch, leader))
  { // check it like this instead o' get_char_room_vis 'cause stalk checks.
    ch->sendln(u"I see no person by that name here!"_s);
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln(u"You must first abandon your group."_s);
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master))
  {
    act_to_character("But you only feel like following $N!", ch, 0, ch->master, 0);
  }
  else
  { /* Not Charmed follow person */

    if (leader == ch)
    {
      if (!ch->master)
      {
        ch->sendln(u"You are already following yourself."_s);
        return ReturnValue::eFAILURE;
      }
      stop_follower(ch);
    }
    else
    {
      if (circle_follow(ch, leader))
      {
        act("Sorry, but following in 'loops' is not allowed.",
            ch, 0, 0, TO_CHAR, 0);
        return ReturnValue::eFAILURE;
      }
      if (ch->master)
      {
        if (cmd == cmd_t::TRACK)
          stop_follower(ch, follower_reasons_t::END_STALK); /* stalk  */
        else
          stop_follower(ch); /* follow */
      }

      //	    if((abs(ch->getLevel()-leader->getLevel())<60) || ch->getLevel()>=IMMORTAL) {
      if (cmd == cmd_t::TRACK)
        add_follower(ch, leader, follower_reasons_t::END_STALK); /* stalk  */
      else
        add_follower(ch, leader); /* follow */
                                  //          }
                                  //	    else
                                  //	      {
                                  //		act("Sorry, but you are not of the right caliber to follow.",
                                  //		ch, 0, 0, TO_CHAR, 0);
                                  //		return ReturnValue::eFAILURE;
                                  //	      }
    }
  }
  return ReturnValue::eSUCCESS;
}

ReturnValue do_autojoin(CharacterPtr ch, QString str_arguments, cmd_t cmd)
{
  if (ch->player == nullptr)
  {
    return ReturnValue::eFAILURE;
  }

  QString arguments = QString::fromStdString(str_arguments).trimmed().toLower();

  if (arguments.isEmpty())
  {
    if (ch->player->joining.isEmpty())
    {
      ch->send(u"You are not configured to auto-joining anyone.\r\n"_s);
    }
    else
    {
      ch->send(u"You are configured to auto-join: %1\r\n"_s.arg(ch->player->getJoining()));
    }

    ch->send(u"Syntax: autojoin <player1> <player2> <player3> ...\r\n"_s);

    return ReturnValue::eFAILURE;
  }

  else if (arguments == "clear")
  {
    ch->player->joining.clear();
    ch->send(u"Auto-join list cleared.\r\n"_s);
    return ReturnValue::eSUCCESS;
  }

  auto parts = arguments.toLower().split(' ');
  for (const auto &i : parts)
  {
    ch->player->toggleJoining(i);
  }

  ch->send(u"You are now autojoining: %1\r\n"_s.arg(ch->player->getJoining()));
  return ReturnValue::eSUCCESS;
}

QList<CharacterPtr> Character::getFollowers(void)
{
  QList<CharacterPtr> followers = {};
  follow_type *f = {};
  CharacterPtr leader = {};

  if (!IS_AFFECTED(this, AFF_GROUP))
  {
    return followers;
  }

  if (master != nullptr)
  {
    leader = master;
  }
  else
  {
    leader = this;
  }
  followers.push_back(leader);

  for (f = leader->followers; f != nullptr; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP))
    {
      followers.push_back(f->follower);
    }
  }

  return followers;
}
