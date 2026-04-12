#include <vector>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <QString>
#include <utility>

#include <QTimeZone>

#include "DC/levels.h"
#include "DC/wizard.h"

#include "DC/connect.h"

#include "DC/DC.h"
#include "DC/handler.h"
#include "DC/db.h"

#include "DC/interp.h"
#include "DC/returnvals.h"
#include "DC/spells.h"
#include "DC/race.h"
#include "DC/const.h"
#include "DC/corpse.h"

qint32 count_rooms(qint32 start, qint32 end)
{
  if (start < 0 || end < 0 || start > 1000000 || end > 1000000)
  {
    return 0;
  }

  if (start > end)
  {
    std::swap<qint32>(start, end);
  }

  qint32 count = {};
  for (qint32 i = start; i < DC::getInstance()->top_of_world && i <= end; i++)
  {
    if (!DC::getInstance()->rooms.contains(i))
      continue;
    count++;
  }

  return count;
}

command_return_t do_thunder(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf1;
  QString buf2;
  class Connection *i;
  QString buf3;

  if (ch->isPlayer() && ch->player->wizinvis)
    dc_sprintf(buf3, "someone");
  else
    dc_sprintf(buf3, "%s", qPrintable(ch->shortdesc_or_name()));

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
    ch->sendln("It's not gonna look that impressive...");
  else
  {
    if (cmd == cmd_t::DEFAULT)
      dc_sprintf(buf2, "$4$BYou thunder '%s'$R", argument);
    else
      dc_sprintf(buf2, "$7$BYou bellow '%s'$R", argument);
    act_to_character(buf2, ch, 0, 0, 0);

    for (auto &i : DC::getInstance()->connections_)
      if (i->character != ch && !i->connected)
      {
        if (ch->isPlayer() && ch->player->wizinvis && i->character->getLevel() < ch->player->wizinvis)
          dc_sprintf(buf3, "Someone");
        else
          dc_sprintf(buf3, "%s", qPrintable(ch->shortdesc_or_name()));

        if (cmd == cmd_t::DEFAULT)
        {
          dc_sprintf(buf1, "$B$4%s thunders '%s'$R\r\n", buf3, argument);
        }
        else
        {
          dc_sprintf(buf1, "$7$B%s bellows '%s'$R\r\n", buf3, argument);
        }

        send_to_char(buf1, i->character);
      }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_incognito(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (ch->player->incognito == true)
  {
    ch->sendln("Incognito off.");
    ch->player->incognito = false;
  }
  else
  {
    send_to_char("Incognito on.  Even while invis, anyone in your room can "
                 "see you.\r\n",
                 ch);
    ch->player->incognito = true;
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_load(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString type = {0};
  QString name = {0};
  QString arg2 = {0};
  QString arg3 = {0};
  QString qty = {0};
  QString random = {0};
  QString buf = {0};

  QString c;
  qint32 x, number = 0, num = 0, cnt = 1;

  QStringList types = {
      "mobile",
      "object",
  };

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (!ch->has_skill(COMMAND_LOAD) && cmd == cmd_t::DEFAULT)
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }
  if (!ch->has_skill(COMMAND_PRIZE) && cmd == cmd_t::PRIZE)
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  half_chop(arg, type, arg2);

  if (cmd == cmd_t::DEFAULT && (type.isEmpty() || arg.isEmpty() 2))
  {
    ch->sendln("Usage:  load <mob> <name|vnum> [qty]");
    ch->sendln("        load <obj> <name|vnum> [qty] [random]");
    return ReturnValue::eFAILURE;
  }
  if (cmd == cmd_t::PRIZE && type.isEmpty())
  {

    *buf = '\0';
    ch->sendln("[#  ] [OBJ #] OBJECT'S DESCRIPTION\n");

    for (x = {}; (x < DC::getInstance()->obj_index[top_of_objt].vnum()); x++)
    {
      if ((num = real_object(x)) < 0)
        continue;

      if (isexact("prize", ((DC::getInstance()->obj_index[num].item))->name()))
      {
        cnt++;
        dc_sprintf(buf, "[%3d] [%5d] %s\r\n", cnt, x, ((DC::getInstance()->obj_index[num].item))->short_description);
        ch->send(buf);
      }

      if (cnt > 200)
      {
        ch->sendln("Maximum number of searchable items hit.  Search ended.");
        break;
      }
    }

    ch->sendln("To load: prize <name|vnum>");
    return ReturnValue::eFAILURE;
  }

  if (cmd == cmd_t::DEFAULT)
  {
    half_chop(arg2, name, arg3);

    if (arg3[0])
    {
      cnt = atoi(arg3);
      half_chop(arg3, qty, random);
    }

    if (cnt > 50)
    {
      ch->sendln("Sorry, you can only load at most 50 of something at a time.");
      return ReturnValue::eFAILURE;
    }
  }

  if (cmd == cmd_t::DEFAULT)
    c = name;
  else
    c = type;

  if (cmd == cmd_t::DEFAULT)
  {
    for (x = {}; x <= 2; x++)
    {
      if (x == 2)
      {
        ch->sendln("Type must mobile or object.");
        return ReturnValue::eFAILURE;
      }
      if (is_abbrev(type, types[x]))
        break;
    }
  }
  else
    x = 1;

  switch (x)
  {
  default:
    ch->sendln("Problem...fuck up in do_load.");
    DC::getInstance()->logentry(u"Default in do_load...should NOT happen."_s, ANGEL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  case 0: /* mobile */
    if ((number = number_or_name(&c, &num)) == 0)
      return ReturnValue::eFAILURE;
    else if (number == -1)
    {
      if ((number = real_mobile(num)) < 0)
      {
        ch->sendln("No such mobile.");
        return ReturnValue::eFAILURE;
      }
      if (ch->getLevel() < DEITY && !can_modify_mobile(ch, num))
      {
        ch->sendln("You may only load mobs inside of your range.");
        return ReturnValue::eFAILURE;
      }
      do_mload(ch, number, cnt);
      return ReturnValue::eSUCCESS;
    }
    if ((num = mob_in_index(c, number)) == -1)
    {
      ch->sendln("No such mobile.");
      return ReturnValue::eFAILURE;
    }
    do_mload(ch, num, cnt);
    return ReturnValue::eSUCCESS;
  case 1: /* object */
    if ((number = number_or_name(&c, &num)) == 0)
      return ReturnValue::eFAILURE;
    else if (number == -1)
    {
      if ((number = real_object(num)) < 0)
      {
        ch->sendln("No such object.");
        return ReturnValue::eFAILURE;
      }
      if ((ch->getLevel() < 108) &&
          isSet(((DC::getInstance()->obj_index[number].item))->obj_flags.extra_flags, ITEM_SPECIAL))
      {
        ch->sendln("Why would you want to load that?");
        return ReturnValue::eFAILURE;
      }
      else if (cmd == cmd_t::PRIZE && !isexact("prize", ((DC::getInstance()->obj_index[number].item))->name()))
      {
        ch->sendln("This command can only load prize items.");
        return ReturnValue::eFAILURE;
      }
      else if (cmd != cmd_t::PRIZE && ch->getLevel() < DEITY && !can_modify_object(ch, num))
      {
        ch->sendln("You may only load objects inside of your range.");
        return ReturnValue::eFAILURE;
      }

      if (random[0] == 'r')
      {
        ObjectPtr obj = (DC::getInstance()->obj_index[number].item);
        if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
        {
          ch->send(u"You cannot random load vnum %1 because extra flag ITEM_SPECIAL is set.\r\n"_s.arg(num));
          return ReturnValue::eFAILURE;
        }
        else if (isSet(obj->obj_flags.extra_flags, ITEM_QUEST))
        {
          ch->send(u"You cannnot random load vnum %1 because extra flag ITEM_QUEST is set.\r\n"_s.arg(num));
          return ReturnValue::eFAILURE;
        }
        else if (isSet(obj->obj_flags.more_flags, ITEM_NO_CUSTOM))
        {
          ch->send(u"You cannot random load vnum %1 because more flag ITEM_NO_CUSTOM is set.\r\n"_s.arg(num));
          return ReturnValue::eFAILURE;
        }
      }

      do_oload(ch, number, cnt, (random[0] == 'r' ? true : false));
      return ReturnValue::eSUCCESS;
    }
    if ((num = obj_in_index(c, number)) == -1)
    {
      ch->sendln("No such object.");
      return ReturnValue::eFAILURE;
    }
    if ((ch->getLevel() < IMPLEMENTER) &&
        isSet(((DC::getInstance()->obj_index[num].item))->obj_flags.extra_flags,
              ITEM_SPECIAL))
    {
      ch->sendln("Why would you want to load that?");
      return ReturnValue::eFAILURE;
    }
    else if (cmd == cmd_t::PRIZE && !isexact("prize", ((DC::getInstance()->obj_index[num].item))->name()))
    {
      ch->sendln("This command can only load prize items.");
      return ReturnValue::eFAILURE;
    }

    do_oload(ch, num, cnt, (random[0] == 'r' ? true : false));
    return ReturnValue::eSUCCESS;
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_purge(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr vict, next_v;
  ObjectPtr obj, next_o;

  QString name, buf[300];

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  one_argument(argument, name);

  if (*name)
  { /* argument supplied. destroy single object or chararacter */
    if ((vict = ch->get_char_room_vis(name)) && (ch->getLevel() > G_POWER))
    {
      if (vict->isPlayer() && (ch->getLevel() <= vict->getLevel()))
      {
        dc_sprintf(buf, "%s is surrounded with scorching flames but is"
                        " unharmed.\r\n",
                   qPrintable(vict->shortdesc_or_name()));
        ch->send(buf);
        act_to_victim("$n tried to purge you.", ch, 0, vict, 0);
        return ReturnValue::eFAILURE;
      }

      act_to_room("$n disintegrates $N.", ch, 0, vict, NOTVICT);
      act_to_character("You disintegrate $N.", ch, 0, vict, 0);

      if (vict->desc)
      {
        close_socket(vict->desc);
        vict->desc = {};
      }

      extract_char(vict, true);
    }
    else if ((obj = get_obj_in_list_vis(ch, name,
                                        DC::getInstance()->world[ch->in_room].contents)) != nullptr)
    {
      act_to_room("$n purges $p.", ch, obj, 0, 0);
      act_to_character("You purge $p.", ch, obj, 0, 0);
      extract_obj(obj);
    }
    else
    {
      ch->sendln("You can't find it to purge!");
      return ReturnValue::eFAILURE;
    }
  }
  else
  { /* no argument. clean out the room */
    if (ch->isNonPlayer())
    {
      ch->sendln("Don't... You would kill yourself too.");
      return ReturnValue::eFAILURE;
    }

    act("$n gestures... the room is filled with scorching flames!",
        ch, 0, 0, TO_ROOM, 0);
    send_to_char("You gesture...the room is filled with scorching "
                 "flames!\r\n",
                 ch);

    for (vict = DC::getInstance()->world[ch->in_room].people; vict; vict = next_v)
    {
      next_v = vict->next_in_room;
      if (vict->isNonPlayer())
        extract_char(vict, true);
    }

    for (obj = DC::getInstance()->world[ch->in_room].contents; obj; obj = next_o)
    {
      next_o = obj->next_content;
      extract_obj(obj);
    }
  }
  save_corpses();
  return ReturnValue::eSUCCESS;
}

QString dirNumToChar(qint32 dir)
{
  switch (dir)
  {
  case 0:
    return "North";
    break;
  case 1:
    return "East";
    break;
  case 2:
    return "South";
    break;
  case 3:
    return "West";
    break;
  case 4:
    return "Up";
    break;
  case 5:
    return "Down";
    break;
  }

  return "ERROR";
}

qint32 Zone::show_info(CharacterPtr ch)
{
  QString buf;

  QString continent_name;
  if (continent && (quint32)continent < continent_names.size())
    continent_name = continent_names.at(continent);

  ch->send(QString("$3Name:$R %1\r\n"
                   "$3Filename:$R %2\r\n"
                   "$3Starts:$R    %3 $3Ends:$R  %4     $3Continent:$R %5\r\n"
                   "$3Starts:$R    %6 $3Ends:$R  %7\r\n"
                   "$3Lifetime:$R  %8 $3Age:$R   %9     $3Left:$R   %10\r\n"
                   "$3PC'sInZone:$R  %11 $3Mode:$R %12 $3Last full reset:$R %13 %14\r\n"
                   "$3Flags:$R ")
               .arg(Name())
               .arg(filename)
               .arg(bottom, 6)
               .arg(top, 13)
               .arg(continent_name.c_str())
               .arg(bottom_rnum, 6)
               .arg(top_rnum, 13)
               .arg(lifespan, 6)
               .arg(age, 13)
               .arg(lifespan - age, 6)
               .arg(players, 4)
               .arg(zone_modes[reset_mode], -18)
               .arg(last_full_reset.toLocalTime().toString(qPrintable()))
               .arg(last_full_reset.toLocalTime().timeZoneAbbreviation(qPrintable())));

  sprintbit(zone_flags, Zone::zone_bits, buf);
  ch->send(buf);
  dc_sprintf(buf, "\r\n"
                  "$3MobsLastPop$R:  %3d $3DeathCounter$R: %6d     $3ReduceCounter$R: %d\r\n"
                  "$3DiedThisTick$R: %3d $3Repops without Deaths$R: %d $3Repops with bonus$R: %d\r\n",
             num_mob_on_repop,
             death_counter,
             counter_mod,
             died_this_tick,
             repops_without_deaths,
             repops_with_bonus);
  ch->send(buf);
  ch->sendln("");

  return ReturnValue::eSUCCESS;
}

qint32 show_zone_commands(CharacterPtr ch, const Zone &zone, quint64 start, quint64 num_to_show, bool stats)
{
  QString buf;
  qint32 k = {};

  if (start < 0)
    start = {};

  k = zone.cmd.size();

  if (k < start)
  {
    dc_sprintf(buf, "Last command in this zone is %d.\r\n", k);
    ch->send(buf);
    return ReturnValue::eFAILURE;
  }

  if (!num_to_show)
  {
    num_to_show = 20;
  }

  // show zone cmds
  for (qint32 j = start; (j < start + num_to_show) && j < zone.cmd.size(); j++)
  {
    auto last = zone.cmd[j]->last;
    QDateTime last_date_time;
    last_date_time.setSecsSinceEpoch(last);
    QString lastStr = u"never"_s;
    if (last)
    {
      lastStr = last_date_time.toString("yyyy-MM-dd hh:mm:ss z");
    }

    auto lastSuccess = zone.cmd[j]->lastSuccess;
    QDateTime lastSuccess_date_time;
    lastSuccess_date_time.setSecsSinceEpoch(last);
    QString lastSuccessStr = u"never"_s;
    if (lastSuccess)
    {
      lastSuccessStr = lastSuccess_date_time.toString("yyyy-MM-dd hh:mm:ss z");
    }

    quint64 attempts = zone.cmd[j]->attempts;
    quint64 successes = zone.cmd[j]->successes;
    double successRate = 0.0;
    if (attempts > 0)
    {
      successRate = (double)successes / (double)attempts;
    }

    // show command # and if_flag
    // note that we show the command as cmd+1.  This is so we don't have a
    // command 0 from the user's perspective.
    if (zone.cmd[j]->command == '*')
    {
      dc_sprintf(buf, "[%3d] Comment: ", j + 1);
    }
    else
      switch (zone.cmd[j]->if_flag)
      {
      case 0:
        dc_sprintf(buf, "[%3d] Always ", j + 1);
        break;
      case 1:
        dc_sprintf(buf, "[%3d] $B$2OnTrue$R ", j + 1);
        break;
      case 2:
        dc_sprintf(buf, "[%3d] $4OnFals$R ", j + 1);
        break;
      case 3:
        dc_sprintf(buf, "[%3d] $B$5OnBoot$R ", j + 1);
        break;
      case 4:
        dc_sprintf(buf, "[%3d] $B$2Ls$1Mb$2Tr$R ", j + 1);
        break;
      case 5:
        dc_sprintf(buf, "[%3d] $B$4Ls$1Mb$4Fl$R ", j + 1);
        break;
      case 6:
        dc_sprintf(buf, "[%3d] $B$2Ls$7Ob$2Tr$R ", j + 1);
        break;
      case 7:
        dc_sprintf(buf, "[%3d] $B$4Ls$7Ob$4Fl$R ", j + 1);
        break;
      case 8:
        dc_sprintf(buf, "[%3d] $B$2Ls$R%%%%$B$2Tr$R ", j + 1);
        break;
      case 9:
        dc_sprintf(buf, "[%3d] $B$4Ls$R%%%%$B$4Fl$R ", j + 1);
        break;
      default:
        dc_sprintf(buf, "[%3d] $B$4ERROR(%d)$R", j + 1, zone.cmd[j]->if_flag);
        break;
      }
    qint32 virt;
#define ZCMD zone.cmd[j]
    switch (zone.cmd[j]->command)
    {
    case 'M':
      virt = ZCMD->active ? DC::getInstance()->mob_index[ZCMD->arg1].vnum() : ZCMD->arg1;
      dc_sprintf(buf, "%s $B$1Load mob  [%5d] ", buf, virt);
      if (zone.cmd[j]->arg2 == -1)
        dc_strcat(buf, "(  always ) in room ");
      else
        dc_sprintf(buf, "%s(if< [%3d]) in room ", buf, zone.cmd[j]->arg2);
      dc_sprintf(buf, "%s[%5d].$R", buf, zone.cmd[j]->arg3);
      dc_sprintf(buf, "%s ([%d] [%d] [%s])", buf, zone.cmd[j]->lastPop ? 1 : 0, charExists(zone.cmd[j]->lastPop), charExists(zone.cmd[j]->lastPop) ? qPrintable(zone.cmd[j]->lastPop->shortdesc_or_name()) : "Unknown");
      dc_sprintf(buf, "%s\r\n", buf);
      break;
    case 'O':
      virt = ZCMD->active ? DC::getInstance()->obj_index[ZCMD->arg1].vnum() : ZCMD->arg1;
      dc_sprintf(buf, "%s $BLoad obj  [%5d] ", buf, virt);
      if (zone.cmd[j]->arg2 == -1)
        dc_strcat(buf, "(  always ) in room ");
      else
        dc_sprintf(buf, "%s(if< [%3d]) in room ", buf, zone.cmd[j]->arg2);
      //      dc_sprintf(buf, "%s[%5d].$R\r\n", buf,
      // DC::getInstance()->world[zone.cmd[j]->arg3].number);
      dc_sprintf(buf, "%s[%5d].$R\r\n", buf, zone.cmd[j]->arg3);
      break;
    case 'P':
      virt = ZCMD->active ? DC::getInstance()->obj_index[ZCMD->arg1].vnum() : ZCMD->arg1;
      dc_sprintf(buf, "%s $5Place obj [%5d] ", buf, virt);
      if (zone.cmd[j]->arg2 == -1)
        dc_strcat(buf, "(  always ) in objt ");
      else
        dc_sprintf(buf, "%s(if< [%3d]) in objt ", buf, zone.cmd[j]->arg2);
      virt = ZCMD->active ? DC::getInstance()->obj_index[ZCMD->arg3].vnum() : ZCMD->arg3;
      dc_sprintf(buf, "%s[%5d] (in last created).$R\r\n", buf, virt);
      break;
    case 'G':
      virt = ZCMD->active ? DC::getInstance()->obj_index[ZCMD->arg1].vnum() : ZCMD->arg1;
      dc_sprintf(buf, "%s $6Place obj [%5d] ", buf, virt);
      if (zone.cmd[j]->arg2 == -1)
        dc_strcat(buf, "(  always ) on last mob loaded.$R\r\n");
      else
        dc_sprintf(buf, "%s(if< [%3d]) on last mob loaded.$R\r\n", buf, zone.cmd[j]->arg2);
      break;
    case 'E':
      virt = ZCMD->active ? DC::getInstance()->obj_index[ZCMD->arg1].vnum() : ZCMD->arg1;
      dc_sprintf(buf, "%s $2Equip obj [%5d] ", buf, virt);
      if (zone.cmd[j]->arg2 == -1)
        dc_strcat(buf, "(  always ) on last mob on ");
      else
        dc_sprintf(buf, "%s(if< [%3d]) on last mob on ", buf, zone.cmd[j]->arg2);
      if (zone.cmd[j]->arg3 > MAX_WEAR - 1 ||
          zone.cmd[j]->arg3 < 0)
        dc_sprintf(buf, "%s[%d](InvalidArg3).$R\r\n", buf, zone.cmd[j]->arg3);
      else
        dc_sprintf(buf, "%s[%d](%s).$R\r\n", buf, zone.cmd[j]->arg3,
                   equipment_types[zone.cmd[j]->arg3]);
      break;
    case 'D':
      dc_sprintf(buf, "%s $3Room [%5d] Dir: [%s]", buf,
                 zone.cmd[j]->arg1,
                 dirNumToChar(zone.cmd[j]->arg2));

      switch (zone.cmd[j]->arg3)
      {
      case 0:
        dc_strcat(buf, "Unlock/Open$R\r\n");
        break;
      case 1:
        dc_strcat(buf, "Unlock/Close$R\r\n");
        break;
      case 2:
        dc_strcat(buf, "Lock/Close$R\r\n");
        break;
      default:
        dc_strcat(buf, "ERROR: Unknown$R\r\n");
        break;
      }
      break;
    case '%':
      dc_sprintf(buf, "%s Consider myself true on %d times out of %d.\r\n", buf,
                 zone.cmd[j]->arg1,
                 zone.cmd[j]->arg2);

      break;
    case 'J':
      dc_sprintf(buf, "%s Temp Command. [%d] [%d] [%d]\r\n", buf,
                 zone.cmd[j]->arg1,
                 zone.cmd[j]->arg2,
                 zone.cmd[j]->arg3);
      break;
    case '*':
      dc_sprintf(buf, "%s %s\r\n", buf,
                 qPrintable(zone.cmd[j]->comment) ? qPrintable(zone.cmd[j]->comment) : "Empty Comment");
      break;
    case 'K':
      dc_sprintf(buf, "%s Skip next [%d] commands.\r\n", buf,
                 zone.cmd[j]->arg1);
      break;
    case 'X':
    {
      QString xstrone = "Set all if-flags to 'unsure' state.";
      QString xstrtwo = "Set mob if-flag to 'unsure' state.";
      QString xstrthree = "Set obj if-flag to 'unsure' state.";
      QString xstrfour = "Set %% if-flag to 'unsure' state.";
      QString xstrerror = "Illegal value in arg1.";
      QString xresultstr;

      switch (zone.cmd[j]->arg1)
      {
      case 0:
        xresultstr = xstrone;
        break;
      case 1:
        xresultstr = xstrtwo;
        break;
      case 2:
        xresultstr = xstrthree;
        break;
      case 3:
        xresultstr = xstrfour;
        break;
      default:
        xresultstr = xstrerror;
        break;
      }

      dc_sprintf(buf, "%s [%d] %s\r\n", buf,
                 zone.cmd[j]->arg1, xresultstr);
    }
    break;
    default:
      dc_sprintf(buf, "Illegal Command: %c %d %d %d %d\r\n",
                 zone.cmd[j]->command,
                 zone.cmd[j]->if_flag,
                 zone.cmd[j]->arg1,
                 zone.cmd[j]->arg2,
                 zone.cmd[j]->arg3);
      break;
    } // switch

    if (!zone.cmd[j]->comment.isEmpty() && zone.cmd[j]->command != '*')
    {
      dc_sprintf(buf, "%s       %s\r\n", buf, qPrintable(zone.cmd[j]->comment));
    }

    ch->send(buf);
    if (stats)
    {
      ch->sendln(u"      Last attempt: $B%1$R Last success: $B%2$R Average: $B%3$R"_s.arg(lastStr).arg(lastSuccessStr).arg(successRate * 100.0));
    }
  } // for

  if (num_to_show != 1)
  {
    ch->sendln("\r\nUse zedit to see the rest of the commands if they were truncated.");
  }
  return ReturnValue::eSUCCESS;
}

qint32 show_zone_commands(CharacterPtr ch, zone_t zone_key, quint64 start, quint64 num_to_show, bool stats)
{
  if (!isValidZoneKey(ch, zone_key))
  {
    return ReturnValue::eFAILURE;
  }

  auto &zone = DC::getInstance()->zones[zone_key];
  return show_zone_commands(ch, zone, start, num_to_show, stats);
}

qint32 find_file(world_file_list_item *itm, qint32 high)
{
  qint32 i;
  world_file_list_item *tmp;
  for (i = 0, tmp = itm; tmp; tmp = tmp->next, i++)
    if (tmp->lastnum / 100 == high / 100)
      return i;
  return -1;
}

void show_legacy_files(CharacterPtr ch, world_file_list_item *head)
{
  world_file_list_item *curr = head;
  quint64 i = {};

  ch->send("ID ) Filename                       Begin  End\r\n"
           "----------------------------------------------------------\r\n");

  while (curr != nullptr)
  {
    QString file_in_progress, file_ready, file_approved, file_modified;
    if (isSet(curr->flags, WORLD_FILE_IN_PROGRESS))
    {
      file_in_progress = "$B$1*$R";
    }

    if (isSet(curr->flags, WORLD_FILE_READY))
    {
      file_ready = "$B$5*$R";
    }

    if (isSet(curr->flags, WORLD_FILE_APPROVED))
    {
      file_approved = "$B$2*$R";
    }

    if (isSet(curr->flags, WORLD_FILE_MODIFIED))
    {
      file_modified = "MODIFIED";
    }

    ch->send(u"%1) %2 %3 %4 %5%6%7 %8\r\n"_s.arg(i++, 3).arg(curr->filename, -30).arg(curr->firstnum, -6).arg(curr->lastnum, -6).arg(file_in_progress, 1).arg(file_ready, 1).arg(file_approved, 1).arg(file_modified));
    curr = curr->next;
  }
}

command_return_t do_show(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name, buf[200];
  QString beginrange;
  QString endrange;
  QString type;
  world_file_list_item *curr = {};
  qint32 i;
  qint32 nr;
  qint32 count = {};
  qint32 begin, end;

  //  half_chop(argument, type, name);
  argument = one_argument(argument, type);

  // argument = one_argument(argument,name);

  qint32 has_range = ch->has_skill(COMMAND_RANGE);

  if (type.isEmpty())
  {
    send_to_char("Format: show <type> <name>.\r\n"
                 "Types:\r\n"
                 "  keydoorcombo\r\n"
                 "  mob\r\n"
                 "  obj\r\n"
                 "  room\r\n"
                 "  zone\r\n"
                 "  zone all\r\n",
                 ch);
    if (has_range)
      send_to_char("  rfiles\r\n"
                   "  mfiles\r\n"
                   "  ofiles\r\n"
                   "  search\r\n"
                   " msearch\r\n"
                   " rsearch\r\n"
                   " counts\r\n",
                   ch);
    return ReturnValue::eFAILURE;
  }

  if (is_abbrev(type, "mobile"))
  {
    argument = one_argument(argument, name);
    if (name.isEmpty())
    {
      send_to_char("Format:  show mob <keyword>\r\n"
                   "                  <number>\r\n"
                   "                  <beginrange> <endrange>\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }

    if (isdigit(*name))
    {
      //       half_chop(name, beginrange, endrange);
      dc_strcpy(beginrange, name);
      //        beginrange = name;
      argument = one_argument(argument, endrange);
      if (endrange.isEmpty())
        dc_strcpy(endrange, "-1");

      if (!check_range_valid_and_convert(begin, beginrange, 0, 100000) || !check_range_valid_and_convert(end, endrange, -1,
                                                                                                         100000))
      {
        ch->sendln("The begin and end ranges must be valid numbers.");
        return ReturnValue::eFAILURE;
      }
      if (end != -1 && end < begin)
      { // swap um
        i = end;
        end = begin;
        begin = i;
      }

      *buf = '\0';
      ch->sendln("[#  ] [MOB #] [LV] MOB'S DESCRIPTION\n");

      if (end == -1)
      {
        if ((nr = real_mobile(begin)) >= 0)
        {
          dc_sprintf(buf, "[  1] [%5d] [%2llu] %s\r\n", begin,
                     ((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->getLevel(),
                     qPrintable(((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->short_description()));
          ch->send(buf);
        }
      }
      else
      {
        for (i = begin; i <= DC::getInstance()->mob_index[top_of_mobt].vnum() && i <= end;
             i++)
        {
          if ((nr = real_mobile(i)) < 0)
            continue;

          count++;
          dc_sprintf(buf, "[%3d] [%5d] [%2llu] %s\r\n", count, i,
                     ((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->getLevel(),
                     qPrintable(((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->short_description()));
          ch->send(buf);

          if (count > 200)
          {
            ch->sendln("Maximum number of searchable items hit.  Search ended.");
            break;
          }
        }
      }
    }
    else
    {
      *buf = '\0';
      ch->sendln("[#  ] [MOB #] [LV] MOB'S DESCRIPTION\n");

      for (i = {}; (i <= DC::getInstance()->mob_index[top_of_mobt].vnum()); i++)
      {
        if ((nr = real_mobile(i)) < 0)
          continue;

        if (isexact(name, qPrintable(((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->name())))
        {
          count++;
          dc_sprintf(buf, "[%3d] [%5d] [%2llu] %s\r\n", count, i,
                     ((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->getLevel(),
                     qPrintable(((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->short_description()));
          ch->send(buf);

          if (count > 200)
          {
            ch->sendln("Maximum number of searchable items hit.  Search ended.");
            break;
          }
        }
      }
    }
    if (buf.isEmpty())
      ch->sendln("Couldn't find any MOBS by that NAME.");
  } /* "mobile" */
  else if (is_abbrev(type, "counts") && has_range)
  {
    ch->send(u"$3Rooms$R: %d\r\n$3Mobiles$R: %d\r\n$3Objects$R: %d\r\n"_s.arg(DC::getInstance()->total_rooms).arg(top_of_mobt).arg(top_of_objt));
    return ReturnValue::eSUCCESS;
  }
  else if (is_abbrev(type, "object"))
  {
    argument = one_argument(argument, name);
    if (name.isEmpty())
    {
      send_to_char("Format:  show obj <keyword>\r\n"
                   "                  <number>\r\n"
                   "                  <beginrange> <endrange>\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }

    if (isdigit(*name))
    {
      //      half_chop(name, beginrange, endrange);
      argument = one_argument(argument, endrange);
      // beginrange = name;
      dc_strcpy(beginrange, name);
      if (endrange.isEmpty())
        dc_strcpy(endrange, "-1");

      if (!check_range_valid_and_convert(begin, beginrange, 0, 100000) || !check_range_valid_and_convert(end, endrange, -1,
                                                                                                         100000))
      {
        ch->sendln("The begin and end ranges must be valid numbers.");
        return ReturnValue::eFAILURE;
      }
      if (end != -1 && end < begin)
      { // swap um
        i = end;
        end = begin;
        begin = i;
      }

      *buf = '\0';
      ch->sendln("[#  ] [OBJ #] [LV] OBJECT'S DESCRIPTION\n");

      if (end == -1)
      {
        if ((nr = real_object(begin)) >= 0)
        {
          dc_sprintf(buf, "[  1] [%5d] [%2llu] %s\r\n", begin,
                     ((DC::getInstance()->obj_index[nr].item))->obj_flags.eq_level,
                     qPrintable(((DC::getInstance()->obj_index[nr].item))->short_description()));
          ch->send(buf);
        }
      }
      else
      {
        for (i = begin; i <= DC::getInstance()->obj_index[top_of_objt].vnum() && i <= end;
             i++)
        {
          if ((nr = real_object(i)) < 0)
            continue;

          count++;
          dc_sprintf(buf, "[%3d] [%5d] [%2llu] %s\r\n", count, i,
                     ((DC::getInstance()->obj_index[nr].item))->obj_flags.eq_level,
                     qPrintable(((DC::getInstance()->obj_index[nr].item))->short_description()));
          ch->send(buf);

          if (count > 200)
          {
            ch->sendln("Maximum number of searchable items hit.  Search ended.");
            break;
          }
        }
      }
    }
    else
    {
      *buf = '\0';
      ch->sendln("[#  ] [OBJ #] [LV] OBJECT'S DESCRIPTION\n");

      for (i = {}; (i <= DC::getInstance()->obj_index[top_of_objt].vnum()); i++)
      {
        if ((nr = real_object(i)) < 0)
          continue;

        if (isexact(name, ((DC::getInstance()->obj_index[nr].item))->name()))
        {
          count++;
          dc_sprintf(buf, "[%3d] [%5d] [%2llu] %s\r\n", count, i,
                     ((DC::getInstance()->obj_index[nr].item))->obj_flags.eq_level,
                     qPrintable(((DC::getInstance()->obj_index[nr].item))->short_description()));
          ch->send(buf);
        }

        if (count > 200)
        {
          ch->sendln("Maximum number of searchable items hit.  Search ended.");
          break;
        }
      }
    }
    if (buf.isEmpty())
      ch->sendln("Couldn't find any OBJECTS by that NAME.");
  } /* "object" */
  else if (is_abbrev(type, "room"))
  {
    argument = one_argument(argument, name);
    if (name.isEmpty())
    {
      ch->sendln("Format:  show room <beginrange> <endrange>");
      return ReturnValue::eFAILURE;
    }

    if (isdigit(*name))
    {
      //      half_chop(name, beginrange, endrange);
      argument = one_argument(argument, endrange);
      //	beginrange = name;
      dc_strcpy(beginrange, name);
      if (endrange.isEmpty())
        dc_strcpy(endrange, "-1");

      if (!check_range_valid_and_convert(begin, beginrange, 0, 100000) || !check_range_valid_and_convert(end, endrange, -1,
                                                                                                         100000))
      {
        ch->sendln("The begin and end ranges must be valid numbers.");
        return ReturnValue::eFAILURE;
      }
      if (end != -1 && end < begin)
      { // swap um
        i = end;
        end = begin;
        begin = i;
      }

      *buf = '\0';
      ch->sendln("[#  ] [ROOM#] ROOM'S NAME\n");

      if (end == -1)
      {
        if (DC::getInstance()->rooms.contains(begin))
        {
          dc_sprintf(buf, "[  1] [%5d] %s\r\n", begin,
                     DC::getInstance()->world[begin].name);
          ch->send(buf);
        }
      }
      else
      {
        for (i = begin; i < DC::getInstance()->top_of_world && i <= end; i++)
        {
          if (!DC::getInstance()->rooms.contains(i))
            continue;
          count++;
          dc_sprintf(buf, "[%3d] [%5d] %s\r\n", count, i, DC::getInstance()->world[i].name);
          ch->send(buf);

          if (count > 200)
          {
            ch->sendln("Maximum number of searchable items hit.  Search ended.");
            break;
          }
        }
      }
    }
    if (buf.isEmpty())
      ch->sendln("Couldn't find any ROOMS in that range.");
  } /* "object" */
  else if (is_abbrev(type, "zone"))
  {
    argument = one_argument(argument, name);
    if (name.isEmpty())
    {
      ch->sendln("Show which zone? (# or 'all')");
      return ReturnValue::eFAILURE;
    }

    if (QString(name) == "all")
    {
      send_to_char(
          "     Range        Usage\r\n"
          "Num  Start-End    Start-End    Rooms   Name\r\n"
          //	 174  31900-32100  32001-32079   78   the Battle of Troy
          "---  -----------  -----------  -----  ----------------------\r\n",
          ch);
      for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
      {
        room_t range_start = zone.getBottom();
        room_t range_end = zone.getTop();
        qint32 num = count_rooms(range_start, range_end);

        ch->send(u"%1  %2-%3  $0$B%4-%5  %6$R  %7$R\r\n"_s.arg(zone_key, 3).arg(zone.getBottom(), 5).arg(zone.getTop(), -5).arg(zone.getRealBottom(), 5).arg(zone.getRealTop(), -5).arg(num, 5).arg(zone.name()));
      }
      return ReturnValue::eSUCCESS;
    }

    bool ok = false;
    zone_t zone_key = getZoneKey(ch, name, &ok);
    if (!ok)
    {
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->zones.value(zone_key).show_info(ch);
  } // zone
  else if (is_abbrev(type, "rsearch") && has_range)
  {
    QString arg1;
    qint32 zon, bits = 0, sector = {};
    argument = one_argument(argument, arg1);
    if (!is_number(arg1))
    {
      ch->sendln("Syntax: show rsearch <zone#> <sectorname/roomflag>");
      return ReturnValue::eSUCCESS;
    }
    zon = atoi(arg1);
    //     Room
    //   zone
    //   sector_type
    // room_flags
    while ((argument = one_argument(argument, arg1)) != nullptr)
    {
      if (arg1[0] == '\0')
        break;
      qint32 i;
      bool found = false;
      for (i = {}; room_bits[i][0] != '\n'; i++)
        if (!str_cmp(arg1, room_bits[i]))
        {
          SET_BIT(bits, 1 << i);
          found = true;
          break;
        }
      for (i = {}; sector_types[i][0] != '\n'; i++)
        if (!str_cmp(arg1, sector_types[i]))
        {
          sector = i - 1;
          found = true;
          break;
        }

      if (!found)
        ch->sendln("Unknown room-flag or sector type.");
    }
    if (!bits && !sector)
    {
      ch->sendln("Syntax: show rsearch <zone number> <flags/sector type");
      return ReturnValue::eSUCCESS;
    }
    room_t last_room = DC::getInstance()->zones.lastKey();
    if (zon > last_room)
    {

      ch->send(u"Unknown zone. Zone %1 is greater than last valid zone %2.\r\n"_s).arg(zon).arg(last_room));
      return ReturnValue::eFAILURE;
    }
    QString buf;
    for (i = DC::getInstance()->zones.value(zon).getRealBottom(); i < DC::getInstance()->zones.value(zon).getRealTop();
         i++)
    {
      if (!DC::getInstance()->rooms.contains(i))
        continue;
      if (bits)
        if (!isSet(DC::getInstance()->world[i].room_flags, bits))
          continue;
      if (sector)
        if (DC::getInstance()->world[i].sector_type != sector)
          continue;
      dc_sprintf(buf, "[%3d] %s\r\n", i, DC::getInstance()->world[i].name);
      ch->send(buf);
    }
  }
  else if (is_abbrev(type, "msearch") && has_range)
  { // Mobile search.
    QString arg1;
    quint32 affect[AFF_MAX / ASIZE + 1] = {};
    quint32 act[ACT_MAX / ASIZE + 1] = {};
    qint32 clas = 0, levlow = -555, levhigh = -555, immune = 0, race = -1,
           align = {};
    // qint32 its;
    //    if (
    bool fo = false;
    while ((argument = one_argument(argument, arg1)))
    {
      qint32 i;
      if (strlen(arg1) < 2)
        break;
      fo = true;
      for (i = {}; *pc_clss_types2[i] != '\n'; i++)
        if (!str_cmp(pc_clss_types2[i], arg1))
        {
          clas = i;
          goto thisLoop;
        }
      for (i = {}; *isr_bits[i] != '\n'; i++)
        if (str_nosp_equal(isr_bits[i], arg1))
        {
          SET_BIT(immune, 1 << i);
          goto thisLoop;
        }
      for (i = {}; *action_bits[i] != '\n'; i++)
        if (str_nosp_equal(action_bits[i], arg1))
        {
          SETBIT(act, i);
          goto thisLoop;
        }
      for (i = {}; *affected_bits[i] != '\n'; i++)
        if (str_nosp_equal(affected_bits[i], arg1))
        {
          SETBIT(affect, i);
          goto thisLoop;
        }
      for (i = {}; i <= MAX_RACE; i++)
        if (str_nosp_equal(races[i].singular_name, arg1))
        {
          race = i;
          goto thisLoop;
        }
      if (!str_cmp(arg1, "evil"))
        align = 3;
      else if (!str_cmp(arg1, "good"))
        align = 1;
      else if (!str_cmp(arg1, "neutral"))
        align = 2;
      if (!str_cmp(arg1, "level"))
      {
        argument = one_argument(argument, arg1);
        if (is_number(arg1))
          levlow = atoi(arg1);
        argument = one_argument(argument, arg1);
        if (is_number(arg1))
          levhigh = atoi(arg1);
        if (levhigh == -555 || levlow == -555)
        {
          ch->sendln("Incorrect level requirement.");
          return ReturnValue::eFAILURE;
        }
      }
    thisLoop:
      continue;
    }
    if (!fo)
    {
      qint32 z, o = {};
      for (z = {}; *action_bits[z] != '\n'; z++)
      {
        o++;
        send_to_char_nosp(action_bits[z], ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      for (z = {}; *isr_bits[z] != '\n'; z++)
      {
        o++;
        send_to_char_nosp(isr_bits[z], ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      for (z = {}; *affected_bits[z] != '\n'; z++)
      {
        o++;
        send_to_char_nosp(affected_bits[z], ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      for (z = {}; *pc_clss_types2[z] != '\n'; z++)
      {
        o++;
        send_to_char(pc_clss_types2[z], ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      for (i = {}; i <= MAX_RACE; i++)
      {
        o++;
        send_to_char_nosp(races[i].singular_name, ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      ch->send("level");
      if (o % 7 == 0)
        ch->sendln("");
      else
        ch->send(" ");

      ch->send("good");
      if (o % 7 == 0)
        ch->sendln("");
      else
        ch->send(" ");
      ch->send("evil");
      if (o % 7 == 0)
        ch->sendln("");
      else
        ch->send(" ");
      ch->send("neutral");
      if (o % 7 == 0)
        ch->sendln("");
      else
        ch->send(" ");

      return ReturnValue::eSUCCESS;
    }
    qint32 c, nr;
    if (act.isEmpty() && !clas && !levlow && !levhigh && affect.isEmpty() && !immune && !race && !align)
    {
      ch->sendln("No valid search supplied.");
      return ReturnValue::eFAILURE;
    }
    for (c = {}; c < DC::getInstance()->mob_index[top_of_mobt].vnum(); c++)
    {
      if ((nr = real_mobile(c)) < 0)
        continue;
      if (race > -1)
        if (((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->race != race)
          continue;
      if (align)
      {
        if (align == 1 && ((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->alignment < 350)
          continue;
        else if (align == 2 && (((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->alignment < -350 || ((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->alignment > 350))
          continue;
        else if (align == 3 && ((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->alignment > -350)
          continue;
      }
      if (immune)
        if (!isSet(((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->immune,
                   immune))
          continue;
      if (clas)
        if (((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->c_class != clas)
          continue;
      if (levlow != -555)
        if (((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->getLevel() < levlow)
          continue;
      if (levhigh != -555)
        if (((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->getLevel() > levhigh)
          continue;
      if (*act)
        for (i = {}; i < ACT_MAX; i++)
          if (ISSET(act, i))
            if (!ISSET(
                    ((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->mobdata->actflags,
                    i + 1))
              goto eheh;
      if (*affect)
        for (i = {}; i < AFF_MAX; i++)
          if (ISSET(affect, i))
            if (!ISSET(
                    ((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->affected_by,
                    i + 1))
              goto eheh;
      count++;
      if (count > 200)
      {
        ch->sendln("Limit reached.");
        break;
      }
      dc_sprintf(buf, "[%3d] [%5d] [%2llu] %s\r\n", count, c,
                 ((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->getLevel(),
                 qPrintable(((CharacterPtr)(DC::getInstance()->mob_index[nr].item))->short_description()));
      ch->send(buf);
    eheh:
      continue;
    }
  }
  else if (is_abbrev(type, "search"))
  { // Object search.
    QString arg1;
    qint32 affect = 0, size = 0, extra = 0, more = 0, wear = 0, type = {};
    qint32 levlow = -555, levhigh = -555, dam = 0, lweight = -555, hweight = -555;
    qint32 any = {};
    bool fo = false;
    qint32 item_type = {};
    qint32 its = {};
    qint32 spellnum = -1;
    while ((argument = one_argument(argument, arg1)))
    {
      qint32 i;
      if (strlen(arg1) < 2)
        break;
      fo = true;

      if (str_nosp_equal("wand", arg1))
      {
        item_type = ITEM_WAND;
        goto endy;
      }
      if (str_nosp_equal("staff", arg1))
      {
        item_type = ITEM_STAFF;
        goto endy;
      }
      if (str_nosp_equal("scroll", arg1))
      {
        item_type = ITEM_SCROLL;
        goto endy;
      }
      if (str_nosp_equal("potion", arg1))
      {
        item_type = ITEM_POTION;
        goto endy;
      }

      for (i = {}; i < QFlagsToStrings<ObjectPositions>().size(); i++)
        if (str_nosp_equal(QFlagsToStrings<ObjectPositions>().value(i), arg1))
        {
          SET_BIT(wear, 1 << i);
          goto endy;
        }
      for (i = {}; i < item_types.size(); i++)
      {
        if (str_nosp_equal(item_types[i], arg1))
        {
          type = i;
          goto endy;
        }
      }
      for (i = {}; *strs_damage_types[i] != '\n'; i++)
        if (str_nosp_equal(strs_damage_types[i], arg1))
        {
          dam = i;
          goto endy;
        }
      for (i = {}; i < Object::extra_bits.size(); i++)
        if (str_nosp_equal(qPrintable(Object::extra_bits[i]), arg1))
        {
          if (!str_cmp(qPrintable(Object::extra_bits[i]), "ANY_CLASS"))
            any = i;
          else
            SET_BIT(extra, 1 << i);
          goto endy;
        }
      for (i = {}; i < Object::more_obj_bits.size(); i++)
        if (str_nosp_equal(Object::more_obj_bits[i], arg1))
        {
          SET_BIT(more, 1 << i);
          goto endy;
        }
      for (i = {}; *size_bitfields[i] != '\n'; i++)
        if (str_nosp_equal(size_bitfields[i], arg1))
        {
          SET_BIT(size, 1 << i);
          goto endy;
        }

      if (!item_type)
      {
        for (i = {}; *apply_types[i] != '\n'; i++)
          if (str_nosp_equal(apply_types[i], arg1))
          {
            affect = i;
            goto endy;
          }
      }
      else
      {
        for (i = {}; *spells[i] != '\n'; i++)
          if (str_nosp_equal(spells[i], arg1))
          {
            spellnum = i + 1;
            goto endy;
          }
      }

      if (!str_cmp(arg1, "olevel"))
      {
        argument = one_argument(argument, arg1);
        if (is_number(arg1))
          levlow = atoi(arg1);

        argument = one_argument(argument, arg1);
        if (is_number(arg1))
          levhigh = atoi(arg1);

        if (levhigh == -555 || levlow == -555)
        {
          ch->sendln("Incorrect level requirement.");
          return ReturnValue::eFAILURE;
        }
      }
      if (!str_cmp(arg1, "oweight"))
      {
        argument = one_argument(argument, arg1);
        if (is_number(arg1))
          lweight = atoi(arg1);

        argument = one_argument(argument, arg1);
        if (is_number(arg1))
          hweight = atoi(arg1);

        if (lweight == -555 || hweight == -555)
        {
          ch->sendln("Incorrect weight requirement.");
          return ReturnValue::eFAILURE;
        }
      }
      ch->send(u"Unknown type: %s.\r\n"_s.arg(arg1));
    endy:
      continue;
    }
    qint32 c, nr, aff;
    //     ch->sendln(u"%1 %2 %3 %4 %5"_s.arg(more).arg(extra).arg(wear).arg(size).arg(affect));
    bool found = false;
    qint32 o = 0, z;
    if (!fo)
    {
      for (z = {}; z < QFlagsToStrings<ObjectPositions>().size(); z++)
      {
        o++;
        send_to_char_nosp(QFlagsToStrings<ObjectPositions>().value(z), ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      for (z = {}; z < Object::extra_bits.size(); z++)
      {
        o++;
        send_to_char_nosp(Object::extra_bits[z], ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      for (z = {}; *strs_damage_types[z] != '\n'; z++)
      {
        o++;
        send_to_char_nosp(strs_damage_types[z], ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }

      for (z = {}; z < Object::more_obj_bits.size(); z++)
      {
        o++;
        send_to_char_nosp(Object::more_obj_bits[z], ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      for (z = {}; z < item_types.size(); z++)
      {
        o++;
        send_to_char_nosp(item_types[z], ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      for (z = {}; *size_bitfields[z] != '\n'; z++)
      {
        o++;
        send_to_char_nosp(size_bitfields[z], ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      for (z = {}; *apply_types[z] != '\n'; z++)
      {
        o++;
        send_to_char_nosp(apply_types[z], ch);
        if (o % 7 == 0)
          ch->sendln("");
        else
          ch->send(" ");
      }
      ch->send("oweight");
      if (o % 7 == 0)
        ch->sendln("");
      else
        ch->send(" ");

      ch->send("olevel");
      if (o % 7 == 0)
        ch->sendln("");
      else
        ch->send(" ");

      return ReturnValue::eSUCCESS;
    }

    for (c = {}; c < DC::getInstance()->obj_index[top_of_objt].vnum(); c++)
    {
      found = false;
      if ((nr = real_object(c)) < 0)
        continue;
      if (wear)
        for (i = {}; i < 20; i++)
          if (isSet(wear, 1 << i))
            if (!isSet(
                    ((DC::getInstance()->obj_index[nr].item))->obj_flags.wear_flags,
                    1 << i))
              goto endLoop;
      if (type)
        if (((DC::getInstance()->obj_index[nr].item))->obj_flags.type_flag != type)
          continue;
      if (lweight != -555)
        if (((DC::getInstance()->obj_index[nr].item))->obj_flags.weight < lweight)
          continue;
      if (hweight != -555)
        if (((DC::getInstance()->obj_index[nr].item))->obj_flags.weight > hweight)
          continue;

      if (levhigh != -555)
        if (((DC::getInstance()->obj_index[nr].item))->obj_flags.eq_level > levhigh)
          continue;
      if (levlow != -555)
        if (((DC::getInstance()->obj_index[nr].item))->obj_flags.eq_level < levlow)
          continue;
      if (size)
        for (i = {}; i < 10; i++)
          if (isSet(size, 1 << i))
            if (!isSet(
                    ((DC::getInstance()->obj_index[nr].item))->obj_flags.size,
                    1 << i))
              goto endLoop;
      if (((DC::getInstance()->obj_index[nr].item))->obj_flags.type_flag == ITEM_WEAPON)
      {
        qint32 get_weapon_damage_type(ObjectPtr wielded);
        its = get_weapon_damage_type(
            ((DC::getInstance()->obj_index[nr].item)));
      }
      if (dam && dam != (its - 1000))
        continue;
      if (extra)
        for (i = {}; i < 30; i++)
          if (isSet(extra, 1 << i))
            if (!isSet(
                    ((DC::getInstance()->obj_index[nr].item))->obj_flags.extra_flags,
                    1 << i) &&
                !(any && isSet(
                             ((DC::getInstance()->obj_index[nr].item))->obj_flags.extra_flags,
                             1 << any)))
              goto endLoop;

      if (more)
        for (i = {}; i < 10; i++)
          if (isSet(more, 1 << i))
            if (!isSet(
                    ((DC::getInstance()->obj_index[nr].item))->obj_flags.more_flags,
                    1 << i))
              goto endLoop;
      //      qint32 aff,total = {};
      //    bool found = false;
      if (!item_type)
        for (aff = {};
             aff < ((DC::getInstance()->obj_index[nr].item))->num_affects;
             aff++)
          if (affect == ((DC::getInstance()->obj_index[nr].item))->affected[aff].location)
            found = true;
      if (affect && !item_type)
        if (!found)
          continue;

      if (item_type)
      {
        bool spell_found = false;
        if (((DC::getInstance()->obj_index[nr].item))->obj_flags.type_flag != item_type)
          continue;
        if (item_type == ITEM_POTION || item_type == ITEM_SCROLL)
          for (i = 1; i < 4; i++)
            if (((DC::getInstance()->obj_index[nr].item))->obj_flags.value[i] == spellnum)
              spell_found = true;
        if (item_type == ITEM_STAFF || item_type == ITEM_WAND)
          if (((DC::getInstance()->obj_index[nr].item))->obj_flags.value[3] == spellnum)
            spell_found = true;

        if (!spell_found)
          continue;
      }

      count++;
      if (count > 200)
      {
        ch->sendln("Limit reached.");
        break;
      }
      dc_sprintf(buf, "[%3d] [%5d] [%2llu] %s\r\n", count, c, ((DC::getInstance()->obj_index[nr].item))->obj_flags.eq_level, qPrintable(((DC::getInstance()->obj_index[nr].item))->short_description()));
      ch->send(buf);
    endLoop:
      continue;
    }
  }
  else if (is_abbrev(type, "rfiles") && has_range)
  {
    show_legacy_files(ch, DC::getInstance()->world_file_list);
  }
  else if (is_abbrev(type, "mfiles") && has_range)
  {
    show_legacy_files(ch, DC::getInstance()->mob_file_list);
  }
  else if (is_abbrev(type, "ofiles") && has_range)
  {
    show_legacy_files(ch, DC::getInstance()->obj_file_list);
  }
  else if (is_abbrev(type, "keydoorcombo"))
  {
    if (name.isEmpty())
    {
      ch->sendln("Show which key? (# of key)");
      return ReturnValue::eFAILURE;
    }

    count = atoi(name);
    if ((!count && *name != '0') || count < 0)
    {
      ch->sendln("Which key was that?");
      return ReturnValue::eFAILURE;
    }

    ch->send(u"$3Doors in game that use key %1$R:\r\n\r\n"_s.arg(count));
    for (i = {}; i < DC::getInstance()->top_of_world; i++)
      for (nr = {}; nr < MAX_DIRS; nr++)
        if (DC::getInstance()->rooms.contains(i) && DC::getInstance()->rooms[i].dir_option[nr])
        {
          if (isSet(DC::getInstance()->rooms[i].dir_option[nr]->exit_info,
                    EX_ISDOOR) &&
              DC::getInstance()->rooms[i].dir_option[nr]->key == count)
          {
            ch->send(u" $3Room$R: %5d $3Dir$R: %5s $3Key$R: %d\r\n"_s.arg(DC::getInstance()->rooms[i].number).arg(dirs[nr]).arg(DC::getInstance()->rooms[i].dir_option[nr]->key));
          }
        }
  }
  else
    ch->sendln("Illegal type.  Type just 'show' for legal types.");
  return ReturnValue::eSUCCESS;
}

command_return_t do_transfer(CharacterPtr ch, QString arguments, cmd_t cmd)
{
  if (ch->isNonPlayer() || ch == nullptr)
  {
    return ReturnValue::eFAILURE;
  }

  room_t destination_room = ch->in_room;
  if (destination_room == 0)
  {
    return ReturnValue::eFAILURE;
  }

  QString arg1;
  std::tie(arg1, arguments) = half_chop(arguments);
  if (arg1.empty())
  {
    ch->send("Usage: transfer <name>\r\n");
    ch->send("       transfer all\r\n");
    return ReturnValue::eFAILURE;
  }

  CharacterPtr victim = {};
  room_t source_room = {};
  Connection *i = {};
  if (arg1 == "all")
  {
    for (auto &i : DC::getInstance()->connections_)
    {
      victim = i->character;
      source_room = victim->in_room;
      if (victim != ch && i->connected == Connection::states::PLAYING && source_room != 0)
      {
        act_to_room("$n disappears in a mushroom cloud.", victim, 0, 0, 0);
        ch->send(fmt::format("Moving {} from {} to {}.\r\n", qPrintable(victim->name()), DC::getInstance()->world[source_room].number, DC::getInstance()->world[destination_room].number));
        move_char(victim, destination_room);
        act_to_room("$n arrives from a puff of smoke.", victim, 0, 0, 0);
        act_to_victim("$n has transferred you!", ch, 0, victim, 0);
        do_look(victim, "");
      }
    }

    ch->sendln("Ok.");
    return ReturnValue::eSUCCESS;
  }

  victim = get_char_vis(ch, arg1);
  if (victim == nullptr)
  {
    ch->send("No-one by that name around.\r\n");
    return ReturnValue::eFAILURE;
  }
  source_room = victim->in_room;

  if (DC::getInstance()->world[destination_room].number == IMM_PIRAHNA_ROOM && !isexact(qPrintable(ch->name()), "Pirahna"))
  {
    ch->sendln("Damn! That is rude! This ain't your place. :P");
    return ReturnValue::eFAILURE;
  }

  act_to_room("$n disappears in a mushroom cloud.", victim, 0, 0, 0);
  ch->send(fmt::format("Moving {} from {} to {}.\r\n", qPrintable(victim->name()), DC::getInstance()->world[source_room].number, DC::getInstance()->world[destination_room].number));
  move_char(victim, destination_room);
  act_to_room("$n arrives from a puff of smoke.", victim, 0, 0, 0);
  act_to_victim("$n has transferred you!", ch, 0, victim, 0);
  do_look(victim, "");
  ch->sendln("Ok.");

  return ReturnValue::eSUCCESS;
}

command_return_t do_teleport(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim, target_mob, pers;
  QString person, room;
  qint32 target;
  qint32 loop;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  half_chop(argument, person, room);

  if (person.isEmpty())
  {
    ch->sendln("Who do you wish to teleport?");
    return ReturnValue::eFAILURE;
  } /* if */

  if (room.isEmpty())
  {
    ch->sendln("Where do you wish to send ths person?");
    return ReturnValue::eFAILURE;
  } /* if */

  if (!(victim = get_char_vis(ch, person)))
  {
    ch->sendln("No-one by that name around.");
    return ReturnValue::eFAILURE;
  } /* if */

  if (isdigit(*room))
  {
    target = atoi(&room[0]);
    if ((*room != '0' && target == 0) || !DC::getInstance()->rooms.contains(target))
    {
      ch->sendln("No room exists with that number.");
      return ReturnValue::eFAILURE;
    }
    //      for (loop = {}; loop <= DC::getInstance()->top_of_world; loop++) {
    //         if (DC::getInstance()->world[loop].number == target) {
    //            target = (qint16)loop;
    //            break;
    //      } else if (loop == DC::getInstance()->top_of_world) {
    //            ch->sendln("No room exists with that number.");
    //            return ReturnValue::eFAILURE;
    //      } /* if */
    //       } /* for */
  }
  else if ((target_mob = get_char_vis(ch, room)) != nullptr)
  {
    target = target_mob->in_room;
  }
  else
  {
    ch->sendln("No such target (person) can be found.");
    return ReturnValue::eFAILURE;
  } /* if */

  if (isSet(DC::getInstance()->world[target].room_flags, PRIVATE))
  {
    for (loop = 0, pers = DC::getInstance()->world[target].people; pers;
         pers = pers->next_in_room, loop++)
      ;
    if (loop > 1)
    {
      ch->sendln("There's a private conversation going on in that room");
      return ReturnValue::eFAILURE;
    } /* if */
  } /* if */

  if (isSet(DC::getInstance()->world[target].room_flags, IMP_ONLY) && ch->getLevel() < IMPLEMENTER)
  {
    ch->sendln("No.");
    return ReturnValue::eFAILURE;
  }

  if (isSet(DC::getInstance()->world[target].room_flags, CLAN_ROOM) &&
      ch->getLevel() < DEITY)
  {
    ch->sendln("No.");
    return ReturnValue::eFAILURE;
  }

  act_to_room("$n disappears in a puff of smoke.", victim, 0, 0, 0);
  ch->send(u"Moving %s from %d to %d.\r\n"_s.arg(qPrintable(victim->name())).arg(DC::getInstance()->world[victim->in_room].number).arg(DC::getInstance()->world[target].number));
  move_char(victim, target);
  act_to_room("$n arrives from a puff of smoke.", victim, 0, 0, 0);
  act_to_victim("$n has teleported you!", ch, 0, victim, 0);
  do_look(victim, "");
  ch->sendln("Teleport completed.");

  return ReturnValue::eSUCCESS;
} /* do_teleport */

command_return_t do_gtrans(CharacterPtr ch, QString argument, cmd_t cmd)
{
  // class Connection *i;
  CharacterPtr victim;
  QString buf;
  qint32 target;
  follow_type *k, *next_dude;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  one_argument(argument, buf);
  if (buf.isEmpty())
  {
    ch->sendln("Whom is the group leader you wish to transfer?");
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_char_vis(ch, buf)))
  {
    ch->sendln("No-one by that name around.");
    return ReturnValue::eFAILURE;
  }
  else
  {
    act("$n disappears in a mushroom cloud.",
        victim, 0, 0, TO_ROOM, 0);
    target = ch->in_room;
    ch->send(u"Moving %s from %d to %d.\r\n"_s.arg(qPrintable(victim->name())).arg(DC::getInstance()->world[victim->in_room].number).arg(DC::getInstance()->world[target].number));
    move_char(victim, target);
    act("$n arrives from a puff of smoke.",
        victim, 0, 0, TO_ROOM, 0);
    act_to_victim("$n has transferred you!", ch, 0, victim, 0);
    do_look(victim, "");

    if (victim->followers)
      for (k = victim->followers; k; k = next_dude)
      {
        next_dude = k->next;
        if (k->follower->isPlayer() && IS_AFFECTED(k->follower, AFF_GROUP))
        {
          act("$n disappears in a mushroom cloud.",
              victim, 0, 0, TO_ROOM, 0);
          target = ch->in_room;
          ch->send(u"Moving %s from %d to %d.\r\n"_s.arg(qPrintable(k->follower->name())).arg(DC::getInstance()->world[k->follower->in_room].number).arg(DC::getInstance()->world[target].number));
          move_char(k->follower, target);
          act("$n arrives from a puff of smoke.",
              k->follower, 0, 0, TO_ROOM, 0);
          act_to_victim("$n has transferred you!", ch, 0, k->follower, 0);
          do_look(k->follower, "");
        }
      } /* for */
    ch->sendln("Ok.");
  } /* else */
  return ReturnValue::eSUCCESS;
}

const QString oprog_type_to_name(qint32 type)
{
  switch (type)
  {
  case ALL_GREET_PROG:
    return "all_greet_prog";
  case WEAPON_PROG:
    return "weapon_prog";
  case ARMOUR_PROG:
    return "armour_prog";
  case LOAD_PROG:
    return "load_prog";
  case COMMAND_PROG:
    return "command_prog";
  case ACT_PROG:
    return "act_prog";
  case ARAND_PROG:
    return "arand_prog";
  case CATCH_PROG:
    return "catch_prog";
  case SPEECH_PROG:
    return "speech_prog";
  case RAND_PROG:
    return "rand_prog";
  case CAN_SEE_PROG:
    return "can_see_prog";
  default:
    return "ERROR_PROG";
  }
}

void opstat(CharacterPtr ch, qint32 vnum)
{
  qint32 num = real_object(vnum);
  ObjectPtr obj;
  QString buf;
  if (num < 0)
  {
    ch->sendln("Error, non-existant object.");
    return;
  }
  obj = DC::getInstance()->obj_index[num].item;
  dc_sprintf(buf, "$3Object$R: %s   $3Vnum$R: %d.\r\n",
             qPrintable(obj->name()), vnum);
  ch->send(buf);
  if (DC::getInstance()->obj_index[num].progtypes == 0)
  {
    ch->sendln("This object has no special procedures.");
    return;
  }
  ch->sendln("");
  mob_prog_data *mprg = {};
  qint32 i = {};
  QString buf2 = {};
  for (mprg = DC::getInstance()->obj_index[num].mobprogs, i = 1; mprg != nullptr;
       i++, mprg = mprg->next)
  {
    dc_sprintf(buf, "$3%d$R>$3$B", i);
    ch->send(buf);
    send_to_char(oprog_type_to_name(mprg->type), ch);
    ch->send("$R ");
    dc_sprintf(buf, "$B$5%s$R\r\n", mprg->arglist);
    ch->send(buf);
    if (mprg->comlist != nullptr)
    {
      ch->sendln(double_dollars(QString(mprg->comlist)));
    }
  }
}

command_return_t do_opstat(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  qint32 vnum = -1;

  if (argument.isEmpty())
  {
    ch->send("Usage: opstat <vnum>\r\n ");
    return ReturnValue::eFAILURE;
  }
  one_argument(argument, buf);

  if (!ch->has_skill(COMMAND_OPSTAT))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }
  if (isdigit(*buf))
  {
    vnum = atoi(argument);
  }
  else
  {
    vnum = ch->player->last_obj_vnum;
  }

  opstat(ch, vnum);
  return ReturnValue::eSUCCESS;
}

void update_objprog_bits(qint32 num)
{
  mob_prog_data *prog = DC::getInstance()->obj_index[num].mobprogs;
  DC::getInstance()->obj_index[num].progtypes = {};

  while (prog)
  {
    SET_BIT(DC::getInstance()->obj_index[num].progtypes, prog->type);
    prog = prog->next;
  }
}

command_return_t do_opedit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 num = -1, vnum = -1, i = -1, a = -1;
  QString arg;
  argument = one_argument(argument, arg);
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;
  if (isdigit(*arg))
  {
    vnum = atoi(arg);
    argument = one_argument(argument, arg);
  }
  else
  {
    vnum = ch->player->last_obj_vnum;
  }

  if ((num = real_object(vnum)) < 0)
  {
    ch->sendln("No such object.");
    return ReturnValue::eFAILURE;
  }
  if (!can_modify_object(ch, vnum))
  {
    ch->sendln("You are unable to work creation outside your range.");
    return ReturnValue::eFAILURE;
  }
  ch->player->last_obj_vnum = vnum;
  /*  if (arg.isEmpty())
    {
          opstat(ch, vnum);
          return ReturnValue::eSUCCESS;
    }*/
  mob_prog_data *prog{}, *currprog = {};
  if (!str_cmp(arg, "add"))
  {
    argument = one_argument(argument, arg);
    if (arg.isEmpty())
    {
      send_to_char("$3Syntax$R: opedit [num] add new\r\n"
                   "This creates a new object proc.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    prog = new mob_prog_data;
    prog->type = ALL_GREET_PROG;
    prog->arglist = u"80"_s;
    prog->comlist = u"say This is my new obj prog!\r\n"_s;
    prog->next = {};

    if ((currprog = DC::getInstance()->obj_index[num].mobprogs))
    {
      while (currprog->next)
        currprog = currprog->next;
      currprog->next = prog;
    }
    else
      DC::getInstance()->obj_index[num].mobprogs = prog;
    update_objprog_bits(num);
    ch->sendln("New obj proc created.");
    return ReturnValue::eSUCCESS;
  }
  else if (!str_cmp(arg, "remove"))
  {
    argument = one_argument(argument, arg);
    qint32 a = -1;
    if (arg.isEmpty() || !isdigit(*arg))
    {
      send_to_char("$3Syntax$R: opedit [obj_num] remove <prog>\r\n"
                   "This removes an object procedure completly\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    a = atoi(arg);
    prog = {};
    for (i = 1, currprog = DC::getInstance()->obj_index[num].mobprogs;
         currprog && i != a;
         i++, prog = currprog, currprog = currprog->next)
      ;
    if (!currprog)
    {
      ch->sendln("Invalid proc number.");
      return ReturnValue::eFAILURE;
    }
    if (prog)
      prog->next = currprog->next;
    else
      DC::getInstance()->obj_index[num].mobprogs = currprog->next;

    currprog->type = {};
    currprog->arglist = {};
    currprog->comlist = {};
    currprog = {};

    update_objprog_bits(num);

    ch->sendln("Program deleted.");
    return ReturnValue::eSUCCESS;
  }
  else if (!str_cmp(arg, "type"))
  {
    argument = one_argument(argument, arg);
    if (arg.isEmpty() || !argument || argument.isEmpty() || !isdigit(*arg) || !isdigit(*(1 + argument)))
    {
      ch->sendln("$3Syntax$R: opedit [obj_num] type <prog> <type>");
      ch->sendln("$3Valid types are$R:");
      QString buf;
      for (i = {}; *obj_types[i] != '\n'; i++)
      {
        dc_sprintf(buf, " %2d - %15s\r\n",
                   i + 1, obj_types[i]);
        ch->send(buf);
      }
      return ReturnValue::eFAILURE;
    }
    qint32 a = atoi(arg);
    for (i = 1, currprog = DC::getInstance()->obj_index[num].mobprogs;
         currprog && i != a;
         i++, currprog = currprog->next)
      ;

    if (!currprog)
    {
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }
    switch (atoi(argument + 1))
    {
    case 1:
      a = ACT_PROG;
      break;
    case 2:
      a = SPEECH_PROG;
      break;
    case 3:
      a = RAND_PROG;
      break;
    case 4:
      a = ALL_GREET_PROG;
      break;
    case 5:
      a = CATCH_PROG;
      break;
    case 6:
      a = ARAND_PROG;
      break;
    case 7:
      a = LOAD_PROG;
      break;
    case 8:
      a = COMMAND_PROG;
      break;
    case 9:
      a = WEAPON_PROG;
      break;
    case 10:
      a = ARMOUR_PROG;
      break;
    case 11:
      a = CAN_SEE_PROG;
      break;
    default:
      ch->sendln("Invalid progtype.");
      return ReturnValue::eFAILURE;
    }
    currprog->type = a;
    update_objprog_bits(num);
    ch->sendln("Proc type changed.");
    return ReturnValue::eSUCCESS;
  }
  else if (!str_cmp(arg, "arglist"))
  {
    //    QString arg1;
    argument = one_argument(argument, arg);
    //    argument = one_argument(argument, arg1);
    if (arg.isEmpty() || !argument || argument.isEmpty() || !isdigit(*arg))
    {
      ch->sendln("$3Syntax$R: opedit [obj_num] arglist <prog> <new arglist>");
      return ReturnValue::eFAILURE;
    }
    a = atoi(arg);
    for (i = 1, currprog = DC::getInstance()->obj_index[num].mobprogs;
         currprog && i != a;
         i++, currprog = currprog->next)
      ;

    if (!currprog)
    {
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }
    currprog->arglist = {};
    currprog->arglist = strdup(argument + 1);

    ch->sendln("Arglist changed.");
    return ReturnValue::eSUCCESS;
  }
  else if (!str_cmp(arg, "command"))
  {
    argument = one_argument(argument, arg);
    if (arg.isEmpty() || !isdigit(*arg))
    {
      ch->sendln("$3Syntax$R: opedit [obj_num] command <prog>");
      return ReturnValue::eFAILURE;
    }
    a = atoi(arg);
    for (i = 1, currprog = DC::getInstance()->obj_index[num].mobprogs;
         currprog && i != a;
         i++, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }

    ch->desc->backstr = {};
    ch->desc->strnew = &(currprog->comlist);
    ch->desc->max_str = MAX_MESSAGE_LENGTH;

    if (isSet(ch->player->toggles, Player::PLR_EDITOR_WEB))
    {
      ch->desc->web_connected = Connection::states::EDIT_MPROG;
    }
    else
    {
      ch->desc->connected = Connection::states::EDIT_MPROG;

      send_to_char("        Write your help entry and stay within the line.(/s saves /h for help)\r\n"
                   "|--------------------------------------------------------------------------------|\r\n",
                   ch);

      if (currprog->comlist)
      {
        ch->desc->backstr = (currprog->comlist);
        ch->send(ch->desc->backstr);
      }
    }

    return ReturnValue::eSUCCESS;
  }
  else if (!str_cmp(arg, "list"))
  {
    opstat(ch, vnum);
    return ReturnValue::eSUCCESS;
  }
  send_to_char("$3Syntax$R: opedit [obj_num] [field] [arg]\r\n"
               "Edit a field with no args for help on that field.\r\n\r\n"
               "The field must be one of the following:\r\n"
               "\tadd\tremove\ttype\targlist\r\n\tcommand\tlist\r\n\r\n",
               ch);
  QString buf;
  dc_sprintf(buf, "$3Current object set to: %lu\r\n", ch->player->last_obj_vnum);
  ch->send(buf);
  return ReturnValue::eSUCCESS;
}

command_return_t do_oclone(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1, arg2;
  argument = one_argument(argument, arg1);
  one_argument(argument, arg2);
  if (!arg1[0] || !arg2[0] || !is_number(arg1) || !is_number(arg2))
  {
    ch->sendln("Syntax: oclone <source vnum> <destination vnum>");
    return ReturnValue::eFAILURE;
  }
  ObjectPtr obj, otmp;
  qint32 v1 = atoi(arg1), v2 = atoi(arg2);
  qint32 r1 = real_object(v1), r2 = real_object(v2);
  if (r1 < 0)
  {
    ch->sendln("Source vnum does not exist.");
    return ReturnValue::eFAILURE;
  }

  if (!can_modify_object(ch, v2))
  {
    ch->sendln("You are unable to work creation outside of your range.");
    return ReturnValue::eFAILURE;
  }

  if (r2 < 0)
  {
    QString buf = u"new %1"_s.arg(v2);
    qint32 retval = ch->do_oedit(buf.split(' '));
    if (!isSet(retval, ReturnValue::eSUCCESS))
      return ReturnValue::eFAILURE;
    r1 = real_object(v1);
    r2 = real_object(v2);
    if (r2 == -1)
    {
      ch->sendln("Something failed. Possibly your destination vnum was too high.");
      return ReturnValue::eFAILURE;
    }
  }
  obj = clone_object(r1);
  if (!obj)
  {
    ch->sendln("Failure. Unable to clone item.");
    return ReturnValue::eFAILURE;
  }

  /*
    if(DC::getInstance()->obj_index[obj->item_number].non_combat_func ||
                  obj->obj_flags.type_flag == ITEM_MEGAPHONE ||
                  has_random(obj)) {
          DC::getInstance()->obj_free_list.insert(obj);
    }
  */

  ch->sendln(u"Ok.\r\nYou copied item %d (%s) and replaced item %d (%s)."_s.arg(v1).arg((DC::getInstance()->obj_index[real_object(v1)].item)->short_description()).arg(v2).arg((DC::getInstance()->obj_index[real_object(v2)].item)->short_description()));

  DC::getInstance()->object_list = DC::getInstance()->object_list->next;
  otmp = DC::getInstance()->obj_index[r2].item;
  obj->item_number = r2;
  DC::getInstance()->obj_index[r2].item = (void *)obj;
  DC::getInstance()->obj_index[r2].non_combat_func = {};
  DC::getInstance()->obj_index[r2].qty = {};
  DC::getInstance()->obj_index[r2].vnum(v2);
  DC::getInstance()->obj_index[r2].mobprogs = {};
  DC::getInstance()->obj_index[r2].combat_func = {};
  DC::getInstance()->obj_index[r2].mobspec = {};
  // extract_obj(otmp);

  ch->player->last_obj_vnum = v2;
  DC::getInstance()->set_zone_modified_obj(r2);

  return ReturnValue::eSUCCESS;
}

command_return_t do_mclone(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1, arg2;
  argument = one_argument(argument, arg1);
  one_argument(argument, arg2);
  if (!arg1[0] || !arg2[0] || !is_number(arg1) || !is_number(arg2))
  {
    ch->sendln("Syntax: mclone <source vnum> <destination vnum>");
    return ReturnValue::eFAILURE;
  }
  CharacterPtr mob;
  qint32 vdst = atoi(arg2), vsrc = atoi(arg1);
  qint32 dst = real_mobile(vdst), src = real_mobile(vsrc);
  if (src < 0)
  {
    ch->sendln("Source vnum does not exist.");
    return ReturnValue::eFAILURE;
  }

  if (!can_modify_mobile(ch, vdst))
  {
    ch->sendln("You are unable to work creation outside of your range.");
    return ReturnValue::eFAILURE;
  }

  if (dst < 0)
  {
    QString buf;
    dc_sprintf(buf, "new %d", vdst);
    qint32 retval = do_medit(ch, buf);
    if (!isSet(retval, ReturnValue::eSUCCESS))
      return ReturnValue::eFAILURE;
    dst = real_mobile(vdst);
    src = real_mobile(vsrc);
    if (dst == -1)
    {
      ch->sendln("Something failed. Possibly your destination vnum was too high.");
      return ReturnValue::eFAILURE;
    }
  }
  mob = ch->getDC()->clone_mobile(src);
  if (!mob)
  {
    ch->sendln("Failure. Unable to copy mobile.");
    return ReturnValue::eFAILURE;
  }

  // clone_mobile assigns the start of character_list to be mob
  // This undos the change
  DC::getInstance()->mob_index[src].qty--;

  auto &character_list = DC::getInstance()->character_list;
  character_list.erase(mob);
  mob->mobdata->nr = dst;

  // Find old mobile in world and remove
  CharacterPtr old_mob = (CharacterPtr)DC::getInstance()->mob_index[dst].item;
  if (old_mob && old_mob->mobdata)
  {
    const auto &character_list = DC::getInstance()->character_list;
    for (const auto &tmpch : character_list)
    {
      if (!tmpch->mobdata)
        continue;
      if (old_mob->mobdata->nr == tmpch->mobdata->nr)
        extract_char(tmpch, true);
    }
  }

  ch->sendln(u"Ok.\r\nYou copied mob %d (%s) and replaced mob %d (%s).\r\n"_s.arg(     vsrc).arg(((CharacterPtr )DC::getInstance()->mob_index[src].item)->short_description()).arg(vdst).arg(((CharacterPtr )DC::getInstance()->mob_index[dst].item)->short_description());

  // Overwrite old mob with new mob
  DC::getInstance()->mob_index[dst].item = (void *)mob;
  DC::getInstance()->mob_index[dst].qty = {};
  DC::getInstance()->mob_index[dst].non_combat_func = {};
  DC::getInstance()->mob_index[dst].combat_func = {};
  DC::getInstance()->mob_index[dst].mobprogs = {};
  DC::getInstance()->mob_index[dst].mobspec = {};
  DC::getInstance()->mob_index[dst].progtypes = {};
  DC::getInstance()->mob_index[dst].vnum(vdst);

  add_mobspec(dst);

  if (DC::getInstance()->mob_index[src].non_combat_func)
  {
    ch->sendln("Warning: hardcoded non_combat function found. Notify coder.");
  }
  if (DC::getInstance()->mob_index[src].combat_func)
  {
    ch->sendln("Warning: hardcoded combat function found. Notify coder.");
  }
  if (DC::getInstance()->mob_index[src].mobprogs)
  {
    ch->sendln("Warning: mob program found. This will need to be copied manually.");
  }

  ch->player->last_mob_edit = dst;
  DC::getInstance()->set_zone_modified_mob(dst);

  return ReturnValue::eSUCCESS;
}
