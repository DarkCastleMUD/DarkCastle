/********************************
| Level 101 wizard commands
| 11/20/95 -- Azrack
**********************/
#include <QQueue>
#include <QString>

#include <fmt/format.h>

#include "DC/interp.h"

#include "DC/DC.h"
#include "DC/terminal.h"
#include "DC/handler.h"
#include "DC/connect.h"
#include "DC/returnvals.h"
#include "DC/spells.h"
#include "DC/db.h"
#include "DC/levels.h"

QQueue<QString> imm_history;
QQueue<QString> imp_history;

constexpr auto MAX_MESSAGE_LENGTH = 4096;

command_return_t Character::do_wizhelp(QStringList arguments, cmd_t cmd)
{
  if (!isImmortalPlayer())
  {
    return ReturnValue::eFAILURE;
  }

  const auto dc = DC::getInstance();
  QString buffer, bestow_buffer, test_buffer;
  quint64 column{}, bestow_column{}, test_column = {};
  for (level_t v = level_t(101); v <= getLevel(); ++v)
  {
    for (auto &command : DC::getInstance()->CMD_.commands_)
    {
      if (command.getMinimumLevel() == GIFTED_COMMAND && v == getLevel())
      {
        auto bestow_command = get_bestow_command(command.name());

        if (!bestow_command.has_value())
        {
          continue;
        }

        if (!has_skill(bestow_command->num))
        {
          continue;
        }

        if (bestow_command->testcmd == false)
        {
          bestow_buffer.append(QStringLiteral("[GFT]%1").arg(command.name(), -11));
          if (++bestow_column % 5 == 0)
          {
            bestow_buffer.append("\r\n");
          }
        }
        else
        {
          test_buffer.append(QStringLiteral("[TST]%1").arg(command.name(), -11));
          if (++test_column % 5 == 0)
          {
            test_buffer.append("\r\n");
          }
        }

        continue;
      }
      if (command.getMinimumLevel() != v || command.getMinimumLevel() == GIFTED_COMMAND)
      {
        continue;
      }

      buffer.append(QStringLiteral("[%1]%2").arg(command.getMinimumLevel(), 2).arg(command.name(), -11));

      if (++column % 5 == 0)
      {
        buffer.append("\r\n");
      }
    }
  }

  // Add an extra \r\n to a section if we haven't already done so
  if (column % 5 == 0)
  {
    buffer.append("\r\n");
  }
  if (bestow_column % 5 != 0)
  {
    bestow_buffer.append("\r\n");
  }
  if (test_column % 5 != 0)
  {
    test_buffer.append("\r\n");
  }

  sendln("Commands based on your level:");
  sendln(buffer);
  sendln();
  sendln("Bestowed commands:");
  sendln(bestow_buffer);
  sendln();
  sendln("Test commands:");
  sendln(test_buffer);
  sendln();
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_goto(QStringList arguments, cmd_t cmd)
{
  qint32 loc_nr = {}, location = -1, i = {}, start_room = {};
  zone_t zone_nr = {};
  CharacterPtr target_mob = {}, pers = {};
  CharacterPtr tmp_ch = {};
  follow_type *k = {}, *next_dude = {};
  ObjectPtr target_obj = {};

  if (this->isNonPlayer())
  {
    return ReturnValue::eFAILURE;
  }
  start_room = in_room;

  if (arguments.isEmpty())
  {
    send("You must supply a room number, a name or zone <zone number>.\r\n");
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.at(0);
  bool ok = false;
  if (arg1 == "zone" || arg1 == "z")
  {

    if (arguments.size() < 2)
    {
      send("No zone number specified.\r\n");
      return ReturnValue::eFAILURE;
    }
    QString arg2 = arguments.at(1);

    zone_t zone_key = arg2.toULongLong(&ok);
    auto &zones = DC::getInstance()->zones;
    if (ok == false || zones.contains(zone_key) == false)
    {
      if (zones.isEmpty())
      {
        send(QStringLiteral("Invalid zone %1 specified. No zones loaded.\r\n").arg(zone_key));
        return ReturnValue::eFAILURE;
      }

      auto last_zone_nr = zones.lastKey();
      send(QStringLiteral("Invalid zone %1 specified. Valid values are 1-%2\r\n").arg(zone_key).arg(last_zone_nr));
      return ReturnValue::eFAILURE;
    }
    auto &zone = zones[zone_key];

    if (zone_key == 0)
    {
      loc_nr = 1;
    }
    else
    {
      loc_nr = zone.getRealBottom();
    }

    send(fmt::format("Going to room {} in zone #{} [{}]\r\n", loc_nr, zone_key, ltrim(QString(DC::getInstance()->zones.value(zone_key).name().toStdString()))));

    if (loc_nr > DC::getInstance()->top_of_world || loc_nr < 0)
    {
      send("No room exists with that number.\r\n");
      return ReturnValue::eFAILURE;
    }

    if (DC::getInstance()->rooms.contains(loc_nr))
    {
      location = loc_nr;
    }
    else
    {
      if (can_modify_this_room(this, loc_nr))
      {
        if (DC::getInstance()->create_one_room(this, loc_nr))
        {
          sendln("You form order out of chaos.");
          location = loc_nr;
        }
      }
    }
    if (location == -1)
    {
      send("No room exists with that number.\r\n");
      return ReturnValue::eFAILURE;
    }
  }
  else if (arg1.isEmpty() == false)
  {
    loc_nr = arg1.toULongLong(&ok);
    if (ok == false)
    {
      if ((target_mob = getVisiblePlayer(arg1)))
      {
        location = target_mob->in_room;
        send(QStringLiteral("Going to player %1 in room %2.\r\n").arg(qPrintable(target_mob->name())).arg(location));
      }
      else if ((target_mob = getVisibleCharacter(arg1)))
      {
        location = target_mob->in_room;
        send(QStringLiteral("Going to character %1 in room %2.\r\n").arg(qPrintable(target_mob->name())).arg(location));
      }
      else if ((target_obj = getVisibleObject(arg1)))
      {
        if (target_obj->in_room != DC::NOWHERE)
        {
          location = target_obj->in_room;
          send(QStringLiteral("Going to object %1 in room %2.\r\n").arg(target_obj->name()).arg(location));
        }
        else
        {
          send("The object is not available.\r\n");
          return ReturnValue::eFAILURE;
        }
      }
      else
      {
        send("No such creature or object around.\r\n");
        return ReturnValue::eFAILURE;
      }
    }
    else
    {
      if (loc_nr > DC::getInstance()->top_of_world || loc_nr < 0)
      {
        send("No room exists with that number.\r\n");
        return ReturnValue::eFAILURE;
      }

      if (DC::getInstance()->rooms.contains(loc_nr))
        location = loc_nr;
      else
      {
        if (can_modify_this_room(this, loc_nr))
        {
          if (DC::getInstance()->create_one_room(this, loc_nr))
          {
            sendln("You form order out of chaos.");
            location = loc_nr;
          }
        }
        else
        {
          send(QStringLiteral("You can't modify room %1").arg(loc_nr));
          return ReturnValue::eFAILURE;
        }
      }
      if (location == -1)
      {
        send("No room exists with that number.\r\n");
        return ReturnValue::eFAILURE;
      }
    }
  }

  /* a location has been found. */
  if (isSet(DC::getInstance()->world[location].room_flags, IMP_ONLY) &&
      level_ < OVERSEER)
  {
    send("That room is for implementers only.\r\n");
    return ReturnValue::eFAILURE;
  }

  /* Let's keep 104-'s out of clan halls....sigh... */
  if (isSet(DC::getInstance()->world[location].room_flags, CLAN_ROOM) &&
      level_ < DEITY)
  {
    send("For your protection, 104-'s may not be in clanhalls.\r\n");
    return ReturnValue::eFAILURE;
  }

  if ((isSet(DC::getInstance()->world[location].room_flags, PRIVATE)) && (level_ < OVERSEER))
  {

    for (i = 0, pers = DC::getInstance()->world[location].people; pers;
         pers = pers->next_in_room, i++)
      ;
    if (i > 1)
    {
      send("There's a private conversation going on in that room.\r\n");
      return ReturnValue::eFAILURE;
    }
  }

  send("\r\n");

  if (!this->isNonPlayer())
    for (tmp_ch = DC::getInstance()->world[in_room].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
    {
      if ((CAN_SEE(tmp_ch, this) && (tmp_ch != this) && !player->stealth) || (tmp_ch->getLevel() > level_ && tmp_ch->getLevel() > PATRON))
      {
        ansi_color(RED, tmp_ch);
        ansi_color(BOLD, tmp_ch);
        tmp_ch->send(player->poofout);
        tmp_ch->sendln("");
        ansi_color(NTEXT, tmp_ch);
      }
      else if (tmp_ch != this && !player->stealth)
      {
        ansi_color(RED, tmp_ch);
        ansi_color(BOLD, tmp_ch);
        tmp_ch->sendln("Someone disappears in a puff of smoke.");
        ansi_color(NTEXT, tmp_ch);
      }
    }

  move_char(this, location);

  if (!this->isNonPlayer())
    for (tmp_ch = DC::getInstance()->world[in_room].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
    {
      if ((CAN_SEE(tmp_ch, this) && (tmp_ch != this) && !player->stealth) || (tmp_ch->getLevel() > level_ && tmp_ch->getLevel() > PATRON))
      {

        ansi_color(RED, tmp_ch);
        ansi_color(BOLD, tmp_ch);
        tmp_ch->send(player->poofin);
        tmp_ch->sendln("");
        ansi_color(NTEXT, tmp_ch);
      }
      else if (tmp_ch != this && !player->stealth)
      {
        ansi_color(RED, tmp_ch);
        ansi_color(BOLD, tmp_ch);
        tmp_ch->sendln("Someone appears with an ear-splitting bang!");
        ansi_color(NTEXT, tmp_ch);
      }
    }

  do_look(this, "");

  if (followers)
    for (k = followers; k; k = next_dude)
    {
      next_dude = k->next;
      if (start_room == k->follower->in_room && CAN_SEE(k->follower, this) &&
          k->follower->getLevel() >= IMMORTAL)
      {
        k->follower->send(QStringLiteral("You follow %s.\r\n\r\n").arg(qPrintable(this->shortdesc_or_name())));
        k->follower->do_goto(arguments, cmd_t::DEFAULT);
      }
    }
  return ReturnValue::eSUCCESS;
}

command_return_t do_poof(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString inout, buf[100];
  qint32 ctr, nope;
  QString _convert;

  if (ch->isNonPlayer())
  {
    ch->sendln("Mobs can't poof.");
    return ReturnValue::eFAILURE;
  }

  arg = one_argument(arg, inout);

  if (!*inout)
  {
    ch->sendln("Usage:\r\npoof [i|o] <QString>");
    ch->sendln("\r\nCurrent poof in is:");
    ch->send(ch->player->poofin);
    ch->sendln("");
    ch->sendln("\r\nCurrent poof out is:");
    ch->send(ch->player->poofout);
    ch->sendln("");
    return ReturnValue::eSUCCESS;
  }

  if (inout[0] != 'i' && inout[0] != 'o')
  {
    ch->sendln("Usage:\r\npoof [i|o] <QString>");
    return ReturnValue::eFAILURE;
  }

  if (!*arg)
  {
    ch->sendln("A poof type message was expected.");
    return ReturnValue::eFAILURE;
  }

  if (strlen(arg) > 72)
  {
    ch->sendln("Poof message too long, must be under 72 characters long.");
    return ReturnValue::eFAILURE;
  }

  nope = {};

  for (ctr = {}; (quint32)ctr <= strlen(arg); ctr++)
  {
    if (arg[ctr] == '%')
    {
      if (nope == 0)
        nope = 1;
      else if (nope == 1)
      {
        ch->sendln("You can only include one % in your poofin ;)");
        return ReturnValue::eFAILURE;
      }
    }
  }

  if (nope == 0)
  {
    send_to_char("You MUST include your name. Use % to indicate where "
                 "you want it.\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  /* For the first time, use strcpy to avoid that annoying space
     at the beginning
  */
  _convert[0] = arg[0];
  _convert[1] = '\0';
  if (arg[ctr] == '%')
    strcpy(buf, qPrintable(ch->name()));
  else
    strcpy(buf, _convert);

  /* No reason to assign _convert[1] every time through, is there? */
  for (ctr = 1; (quint32)ctr < strlen(arg); ctr++)
  {
    _convert[0] = arg[ctr];

    if (arg[ctr] == '%')
      strcat(buf, qPrintable(ch->name()));
    else
      strcat(buf, _convert);
  }

  if (inout[0] == 'i')
  {
    ch->player->poofin = buf;
  }
  else
  {
    ch->player->poofout = buf;
  }

  ch->sendln("Ok.");
  return ReturnValue::eSUCCESS;
}

command_return_t do_at(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString command, loc_str[MAX_INPUT_LENGTH];
  qint32 loc_nr, location, original_loc;
  CharacterPtr target_mob;
  ObjectPtr target_obj;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  half_chop(argument, loc_str, command);
  if (!*loc_str)
  {
    ch->sendln("You must supply a room number or a name.");
    return ReturnValue::eFAILURE;
  }

  if (isdigit(*loc_str) && !strchr(loc_str, '.'))
  {
    loc_nr = atoi(loc_str);
    if ((loc_nr == 0 && *loc_str != '0') ||
        ((location = real_room(loc_nr)) < 0))
    {
      ch->sendln("No room exists with that number.");
      return ReturnValue::eFAILURE;
    }
  }
  else if ((target_mob = get_char_vis(ch, loc_str)) != nullptr)
    location = target_mob->in_room;
  else if ((target_obj = get_obj_vis(ch, loc_str)) != nullptr)
    if (target_obj->in_room != DC::NOWHERE)
      location = target_obj->in_room;
    else
    {
      ch->sendln("The object is not available.");
      return ReturnValue::eFAILURE;
    }
  else
  {
    ch->sendln("No such creature or object around.");
    return ReturnValue::eFAILURE;
  }

  /* a location has been found. */
  if (isSet(DC::getInstance()->world[location].room_flags, IMP_ONLY) && ch->getLevel() < IMPLEMENTER)
  {
    ch->sendln("No.");
    return ReturnValue::eFAILURE;
  }

  /* Let's keep 104-'s out of clan halls....sigh... */
  if (isSet(DC::getInstance()->world[location].room_flags, CLAN_ROOM) &&
      ch->getLevel() < DEITY)
  {
    ch->sendln("For your protection, 104-'s may not be in clanhalls.");
    return ReturnValue::eFAILURE;
  }

  original_loc = ch->in_room;
  move_char(ch, location, false);
  qint32 retval = ch->command_interpreter(command);

  /* check if the guy's still there */
  for (target_mob = DC::getInstance()->world[location].people; target_mob; target_mob =
                                                                               target_mob->next_in_room)
    if (ch == target_mob)
    {
      move_char(ch, original_loc);
    }
  return retval;
}

command_return_t do_highfive(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString buf;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  one_argument(argument, buf);
  if (!*buf)
  {
    ch->sendln("Who do you wish to high-five? ");
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_char_vis(ch, buf)))
  {
    ch->sendln("No-one by that name in the world.");
    return ReturnValue::eFAILURE;
  }

  if (ch == victim)
  {
    sprintf(buf, "%s conjures a clap of thunder to resound the land!\r\n", qPrintable(ch->shortdesc_or_name()));
    send_to_all(buf);
  }
  else
  {
    sprintf(buf, "Time stops for a minute as %s and %s high-five!\r\n", qPrintable(ch->shortdesc_or_name()), qPrintable(victim->shortdesc_or_name()));
    send_to_all(buf);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_holylite(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (argument[0] != '\0')
  {
    ch->sendln("HOLYLITE doesn't take any arguments; arg ignored.");
  } /* if */

  if (ch->player->holyLite)
  {
    ch->player->holyLite = false;
    ch->sendln("Holy light mode off.");
  }
  else
  {
    ch->player->holyLite = true;
    ch->sendln("Holy light mode on.");
  } /* if */
  return ReturnValue::eSUCCESS;
}

command_return_t do_wizinvis(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;

  qint32 arg1;

  if (ch->isNonPlayer())
  {
    return ReturnValue::eFAILURE;
  }

  arg1 = atoi(argument);

  if (arg1 < 0)
    arg1 = {};

  if (!*argument)
  {
    if (ch->player->wizinvis == 0)
    {
      ch->player->wizinvis = ch->getLevel();
    }
    else
    {
      ch->player->wizinvis = {};
    }
  }
  else
  {
    if (arg1 > ch->getLevel())
      arg1 = ch->getLevel();
    ch->player->wizinvis = arg1;
  }
  ch->sendln(QStringLiteral("WizInvis Set to: %1 ").arg(ch->player->wizinvis));
  return ReturnValue::eSUCCESS;
}

command_return_t do_nohassle(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(ch->player->toggles, Player::PLR_NOHASSLE))
  {
    REMOVE_BIT(ch->player->toggles, Player::PLR_NOHASSLE);
    ch->sendln("Mobiles can bother you again.");
  }
  else
  {
    SET_BIT(ch->player->toggles, Player::PLR_NOHASSLE);
    ch->sendln("Those pesky mobiles will leave you alone now.");
  }
  return ReturnValue::eSUCCESS;
}

// cmd == cmd_t::DEFAULT - imm
// cmd == 8 - /
command_return_t do_wiz(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf1 = {};
  Connection *i = {};

  if (ch->isNonPlayer())
  {
    return ReturnValue::eFAILURE;
  }

  if (cmd == cmd_t::IMPCHAN && !ch->has_skill(COMMAND_IMP_CHAN))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  argument = ltrim(argument);
  argument = rtrim(argument);

  if (argument.empty())
  {
    QQueue<QString> tmp;
    if (cmd == cmd_t::IMMORT)
    {
      tmp = imm_history;
      ch->sendln("Here are the last 10 imm messages:");
    }
    else if (cmd == cmd_t::IMPCHAN)
    {
      tmp = imp_history;
      ch->sendln("Here are the last 10 imp messages:");
    }
    else
    {
      ch->sendln("What? How did you get here?? Contact a coder.");
      return ReturnValue::eSUCCESS;
    }

    while (!tmp.empty())
    {
      ch->send(tmp.front());
      tmp.pop();
    }
  }
  else
  {
    if (IS_IMMORTAL(ch))
    {
      argument = remove_all_codes(argument);
    }

    if (cmd == cmd_t::IMMORT)
    {
      buf1 = fmt::format("$B$4{}$7: $7$B{}$R\r\n", qPrintable(ch->shortdesc_or_name()), argument);
      imm_history.push(buf1);
      if (imm_history.size() > 10)
        imm_history.pop();
    }
    else
    {
      buf1 = fmt::format("$B$7{}> {}$R\r\n", qPrintable(ch->shortdesc_or_name()), argument);
      imp_history.push(buf1);
      if (imp_history.size() > 10)
        imp_history.pop();
    }

    send_to_char(buf1.c_str(), ch);
    ansi_color(NTEXT, ch);

    for (auto &i : DC::getInstance()->connections_)
    {
      if (i->character && i->character != ch && i->character->getLevel() >= IMMORTAL && i->character->isPlayer())
      {
        if (cmd == cmd_t::IMPCHAN && !i->character->has_skill(COMMAND_IMP_CHAN))
          continue;

        if (STATE(i) == Connection::states::PLAYING)
        {
          send_to_char(buf1.c_str(), i->character);
        }
        else
        {
          record_msg(buf1.c_str(), i->character);
        }
      }
    }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_findfix(CharacterPtr ch, QString argument, cmd_t cmd)
{
  for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
  {
    for (qsizetype j = {}; j < zone.cmd.size(); j++)
    {
      bool first = true;
      bool found = false;
      if (zone.cmd[j]->command != 'M')
        continue;

      qint32 vnum = zone.cmd[j]->arg1;
      qint32 max = zone.cmd[j]->arg2;
      if (max == 1 || max == -1)
        continue; // Don't care about those..

      qint32 amt = {};
      for (qsizetype z = {}; z < zone.cmd.size(); z++)
      {
        if (zone.cmd[z]->command != 'M')
          continue;
        if (zone.cmd[z]->arg1 != vnum)
          continue;
        if (z == j && found)
        {
          first = false;
          break;
        }
        found = true;
        if (zone.cmd[z]->arg2 > max)
          max = zone.cmd[z]->arg2;
        amt++;
      }

      if (!first)
        continue;
      if (amt == max)
        continue;
      if (amt > max)
      {
        ch->send(QStringLiteral("Reset %1 in zone %2: %3 reset commands OVER %4 max in world.\r\n").arg(j + 1).arg(zone_key).arg(amt).arg(max));
        QString buffer = strdup(QStringLiteral("%1 list %2 1").arg(zone_key).arg(j + 1).toStdString().c_str());
        do_zedit(ch, buffer);
        free(buffer);
      }
      else
      {
        ch->send(QStringLiteral("Reset %1 in zone %2: %3 reset commands UNDER %4 max in world.\r\n").arg(j + 1).arg(zone_key).arg(amt).arg(max));
        QString buffer = strdup(QStringLiteral("%1 list %2 1").arg(zone_key).arg(j + 1).toStdString().c_str());
        do_zedit(ch, buffer);
        free(buffer);
      }
    }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_varstat(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  argument = one_argument(argument, arg);
  CharacterPtr vict;

  if ((vict = get_char_vis(ch, arg)) == nullptr)
  {
    ch->sendln("Target not found.");
    return ReturnValue::eFAILURE;
  }
  QString buf;
  buf[0] = '\0';
  tempvariable *eh;
  for (eh = vict->tempVariable; eh; eh = eh->next)
  {
    snprintf(buf, sizeof(buf), "$B$3%-30s $R-- $B$5 %s\r\n", qPrintable(eh->name), qPrintable(eh->data));
    ch->send(buf);
  }
  if (buf[0] == '\0')
  {
    ch->sendln("No temporary variables found.");
  }
  return ReturnValue::eSUCCESS;
}
