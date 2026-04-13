/**********************
| Level 102 wizard commands
| 11/20/95 -- Azrack
**********************/
/*****************************************************************************/
/* Revision History                                                          */
/* 12/04/2003   Onager  Fixed find_skill() to find skills/spells consisting  */
/*                      of multi-word space-separated names                  */
/*                      Fixed "remove" action of do_sedit() to remove skill  */
/*                      from player (instead of invoking god ;-)             */
/* 12/08/2003   Onager  Fixed do_sedit() to allow setting of multi-word      */
/*                      skills                                               */
/*****************************************************************************/

#include "DC/DC.h"
#include "DC/interp.h"
#include "DC/db.h"
#include "DC/race.h"
#include "DC/const.h"
#include "DC/newedit.h"
#include "DC/punish.h"
#include <cctype>
#include <QString>
#include <tuple>
#include <algorithm>
#include <fmt/format.h>

command_return_t zedit_list(CharacterPtr ch, QStringList arguments, const Zone &zone, bool stats = false);

// Urizen's rebuild rnum references to enable additions to mob/obj arrays w/out screwing everything up.
// A hack of renum_zone_tables *yawns*
// type 1 = mobs, type 2 = objs. Simple as that.
// This should obviously not be called at any other time than additions to the previously mentioned
// arrays, as it'd screw things up.
// Saving zones after this SHOULD not be required, as the old savefiles contain vnums, which should remain correct.
void rebuild_rnum_references(qint32 startAt, qint32 type)
{
  for (auto [zone_key, zone] : dc_->zones.asKeyValueRange())
  {
    for (qsizetype comm = {}; !zone.cmd.isEmpty() && comm < zone.cmd.size(); comm++)
    {
      switch (zone.cmd[comm]->command)
      {
      case 'M':
        if (type == 1 && zone.cmd[comm]->arg1 >= startAt)
          zone.cmd[comm]->arg1++;
        break;
      case 'O':
        if (type == 2 && zone.cmd[comm]->arg1 >= startAt)
          zone.cmd[comm]->arg1++;
        break;
      case 'G':
        if (type == 2 && zone.cmd[comm]->arg1 >= startAt)
          zone.cmd[comm]->arg1++;
        break;
      case 'E':
        if (type == 2 && zone.cmd[comm]->arg1 >= startAt)
          zone.cmd[comm]->arg1++;
        break;
      case 'P':
        if (type == 2 && zone.cmd[comm]->arg1 >= startAt)
          zone.cmd[comm]->arg1++;
        if (type == 2 && zone.cmd[comm]->arg3 >= startAt)
          zone.cmd[comm]->arg3++;
        break;
      case '%':
      case 'K':
      case 'D':
      case 'X':
      case '*':
      case 'J':
        break;
      default:
        dc_->logentry(u"Illegal character hit in rebuild_rnum_references"_s, 0, DC::LogChannel::LOG_WORLD);
        break;
      }
    }
  }
}

command_return_t do_check(CharacterPtr ch, QString arg, cmd_t cmd)
{

  auto arguments = QString(arg).trimmed().split(' ');

  if (arguments.isEmpty())
  {
    ch->sendln("Check who?");
    return ReturnValue::eFAILURE;
  }

  auto name = arguments.value(0).toLower();
  name[0] = name[0].toUpper();
  Connection d;
  auto vict = get_pc_vis_exact(ch, name);
  bool connected = true;
  if (!vict)
  {
    connected = false;

    // must be done to clear out "d" before it is used
    if (auto result = ch->getDC()->load_char_obj(name); result && *result)
    {
      auto conn = *result;
      vict = conn->character;
      vict->desc = {};

      redo_hitpoints(vict);
      redo_mana(vict);
      if (vict->title_.isEmpty())
        vict->title_ = u"is a virgin"_s;
      if (GET_CLASS(vict) == CLASS_MONK)
        GET_AC(vict) -= vict->getLevel() * 3;
      isr_set(vict);
    }

    if (file_exists(u"../archive/%1.gz"_s.arg(name)))
      ch->sendln("Character is archived.");
    else
      ch->sendln("Unable to load! (character might not exist...)");
    return ReturnValue::eFAILURE;
  }

  ch->sendln(u"$3Short Desc$R: %1"_s.arg(vict->shortdesc_or_name()));

  auto race = races[(qint32)(vict->race)].singular_name;
  auto ch_class = pc_clss_types[(qint32)(GET_CLASS(vict))];
  auto room = dc_->world[vict->in_room].number;
  ch->send(u"$3Race$R: %1 $3Class$R: %2 $3Level$R: %3 $3"_s.arg(race, -9).arg(ch_class, -9).arg(vict->getLevel(), -8));

  if (connected)
    ch->sendln(u"In Room$R : %1 "_s.arg(room));
  else
    ch->sendln(u"Not Connected"_s);

  ch->sendln(u"$3Exp$R: %1 $3Gold$R: %2 $3Bank$R: %3 $3Align$R: %4"_s.arg(vict->exp, -10).arg(vict->getGold(), -10).arg(GET_BANK(vict), -9).arg(GET_ALIGNMENT(vict)));
  if (ch->getLevel() >= SERAPH)
    ch->sendln(u"$3Load Rm$R: %1  $3Home Rm$R: %2  $3Platinum$R: %3  $3Clan$R: %4"_s.arg(room,-5).arg(vict->hometown,-5).arg(GET_PLATINUM(vict)).arg(GET_CLAN(vict));

  ch->sendln(u"$3Str$R: %1  $3Wis$R: %2  $3Int$R: %3  $3Dex$R: %4  $3Con$R: %5"_s.arg(GET_STR(vict), -2).arg(GET_WIS(vict), -2).arg(GET_INT(vict), -2).arg(GET_DEX(vict), -2).arg(GET_CON(vict), -2));  
  ch->sendln(u"$3Hit Points$R: %1/%2 $3Mana$R: %3/%4 $3Move$R: %5/%6 $3Ki$R: %7/%8"_s.arg(vict->getHP()).arg(GET_MAX_HIT(vict)).arg(GET_MANA(vict)).arg(GET_MAX_MANA(vict)).arg(GET_MOVE(vict)).arg(GET_MAX_MOVE(vict)).arg(GET_KI(vict)).arg(GET_MAX_KI(vict)));

  if (ch->getLevel() >= OVERSEER && !vict->isNonPlayer() && ch->getLevel() >= vict->getLevel())
  {
      ch->sendln(u"$3Last connected from$R: %1"_s.arg(vict->player->last_site));
      const time_t tBuffer = vict->player->time.logon;
      ch->sendln(u"$3Last connected on$R: %1"_s.arg(ctime(&tBuffer)));
  }

  display_punishes(ch, vict);

  if (connected)
    if (vict->desc)
    {
      if (ch->getLevel() >= OVERSEER && ch->getLevel() >= vict->getLevel())
      {
        ch->sendln(u"$3Connected from$R: %1"_s.arg(vict->desc->getPeerOriginalAddress().toString()));
      }
      else
      {
        ch->sendln(u"Connected.\r\n"_s);
      }
    }
    else
      ch->sendln("(Linkdead)");
  else
  {
      ch->sendln("(Not on game)");
      free_char(vict);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_find(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString type;
  QString name;
  CharacterPtr vict;
  qint32 x;

  const QStringList types = {
      "mob",
      "pc",
      "character",
      "obj",
      "\n"};

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;
  if (!ch->has_skill(COMMAND_FIND))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  half_chop(arg, type, name);

  if (type.isEmpty() || name.isEmpty())
  {
    ch->sendln("Usage:  find <mob|pc|character|obj> <name>");
    return ReturnValue::eFAILURE;
  }

  for (x = {}; x <= 4; x++)
  {
    if (x == 4)
    {
      ch->sendln("Type must be one of these: mob, pc, character, obj.");
      return ReturnValue::eFAILURE;
    }
    if (is_abbrev(type, types[x]))
      break;
  }

  switch (x)
  {
  default:
    ch->sendln("Problem...fuck up in do_find.");
    dc_->logentry(u"Default in do_find...should NOT happen."_s, ANGEL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  case 0: // mobile
    return do_mlocate(ch, name);
  case 1: // pc
    break;
  case 2: // character
    return do_mlocate(ch, name, cmd_t::MLOCATE_CHARACTER);
  case 3: // object
    return do_olocate(ch, name);
  }

  if (!(vict = get_pc_vis(ch, name)))
  {
    ch->sendln("Unable to find that character.");
    return ReturnValue::eFAILURE;
  }

  ch->send(u"%1 -- %2 [%3]"_s.arg(vict->shortdesc_or_name(), 30).arg(dc_->world[vict->in_room].name_).arg(dc_->world[vict->in_room].number));
  return ReturnValue::eSUCCESS;
}

command_return_t do_stat(CharacterPtr ch, QString arg, cmd_t cmd)
{
  const QStringList types = {
      "mobile",
      "object",
      "character"};

  if (!ch->has_skill(COMMAND_STAT))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  QString type, name;
  half_chop(arg, type, name);

  if (type.isEmpty() || name.isEmpty())
  {
    ch->sendln("Usage:  stat <mob|obj|character> <name>");
    return ReturnValue::eFAILURE;
  }

  if (!isexact(type, types))
  {
    send_to_char("Type must be one of these: mobile, object, "
                 "character.\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  switch (x)
  {
  default:
    ch->sendln("Problem...fuck up in do_stat.");
    dc_->logentry(u"Default in do_stat...should NOT happen."_s, ANGEL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  case 0: // mobile
    if (auto vch = get_mob_vis(ch, name); vch)
    {
      mob_stat(ch, vch);
      return ReturnValue::eFAILURE;
    }
    ch->sendln("No such mobile.");
    return ReturnValue::eFAILURE;
  case 1: // object
    if (auto obj = get_obj_vis(ch, name); obj)
      obj_stat(ch, obj);
    else
      ch->sendln("No such object.");
    return ReturnValue::eFAILURE;
    break;
  case 2: // character
    break;
  }

  if (auto vch = get_pc_vis(ch, name); vch)
  {
    if (!name.isEmpty())
    {
      name = name.toLower();
      name[0] = name[0].toUpper();
    }

    // must be done to clear out "d" before it is used
    if (auto result = ch->getDC()->load_char_obj(name); result && *result)
    {
      auto conn = *result;
      auto vict = conn->character;
      vict->desc = {};
      redo_hitpoints(vict);
      redo_mana(vict);
      if (vict->title_.isEmpty())
        vict->title_ = "is a virgin";
      if (GET_CLASS(vict) == CLASS_MONK)
        GET_AC(vict) -= vict->getLevel() * 3;
      isr_set(vict);
      char_to_room(vict, ch->in_room);
      mob_stat(ch, vict);
      char_from_room(vict);
      free_char(vict);
      return ReturnValue::eSUCCESS;
    }
    else
    {
      ch->sendln("Unable to load! (character might not exist...)");
      return ReturnValue::eFAILURE;
    }

    mob_stat(ch, vch);
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_mpstat(CharacterPtr ch, QString arg, cmd_t cmd)
{
  CharacterPtr vict;
  QString name;
  qint32 x;

  void mpstat(CharacterPtr ch, CharacterPtr victim);

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (!ch->has_skill(COMMAND_MPSTAT))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  //  qint32 has_range = ch->has_skill( COMMAND_RANGE);

  one_argument(arg, name);

  if (name.isEmpty())
  {
    ch->sendln("Usage:  procstat <name|num>");
    return ReturnValue::eFAILURE;
  }

  if (isdigit(*name))
  {
    if (!(x = atoi(name)))
    {
      ch->sendln("That is not a valid number.");
      return ReturnValue::eFAILURE;
    }
    x = real_mobile(x);

    if (x < 0)
    {
      ch->sendln("No mob of that number.");
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    if (!(vict = get_mob_vis(ch, name)))
    {
      ch->sendln("No such mobile.");
      return ReturnValue::eFAILURE;
    }
    x = vict->mobdata->nr;
  }
  /*
    if(!has_range)
    {
      if(!can_modify_mobile(ch, dc_->mob_index[x].vnum())) {
        ch->sendln("You are unable to work creation outside of your range.");
        return ReturnValue::eFAILURE;
      }
    }
  */
  mpstat(ch, (CharacterPtr)dc_->mob_index[x].item);
  return ReturnValue::eSUCCESS;
}

command_return_t zedit_flags(CharacterPtr ch, QStringList arguments, Zone &zone)
{
  if (arguments.isEmpty())
  {
    ch->sendln("$3Usage$R: zedit flags <noteleport|noclaim|nohunt>");
    ch->send(u"Current flags: %1\r\n"_s.arg(sprintbit(zone.getZoneFlags(), Zone::zone_bits)));
    return ReturnValue::eFAILURE;
  }

  QString text = arguments.value(0, "");

  if (text == "noclaim")
  {
    if (zone.isNoClaim())
    {
      zone.setNoClaim(false);
    }
    else
    {
      zone.setNoClaim(true);
    }

    if (zone.isNoClaim())
    {
      ch->sendln("noclaim toggled on.");
    }
    else
    {
      ch->sendln("noclaim toggled off.");
    }
  }

  else if (text == "noteleport")
  {
    if (zone.isNoTeleport())
    {
      zone.setNoTeleport(false);
    }
    else
    {
      zone.setNoTeleport(true);
    }

    if (zone.isNoTeleport())
    {
      ch->sendln("noteleport toggled on.");
    }
    else
    {
      ch->sendln("noteleport toggled off.");
    }
  }
  else if (text == "nohunt")
  {
    if (zone.isNoHunt())
    {
      zone.setNoHunt(false);
    }
    else
    {
      zone.setNoHunt(true);
    }

    if (zone.isNoHunt())
    {
      ch->sendln("nohunt toggled on.");
    }
    else
    {
      ch->sendln("nohunt toggled off.");
    }
  }
  else
  {
    ch->send(u"'%1' invalid.  Enter 'noclaim', 'noteleport' or 'nohunt'.\r\n"_s.arg(text));
    return ReturnValue::eFAILURE;
  }
  return ReturnValue::eSUCCESS;
}

command_return_t zedit_lifetime(CharacterPtr ch, QStringList arguments, Zone &zone)
{
  if (arguments.isEmpty())
  {
    send_to_char("$3Usage$R: zedit lifetime <tickamount>\r\n"
                 "The lifetime is the number of ticks the zone takes\r\n"
                 "before it will attempt to repop itself.\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }
  QString text = arguments.value(0);
  bool ok = false;
  quint64 ticks = text.toULongLong(&ok);

  if (!ok || ticks > 32000)
  {
    ch->sendln("You much choose between 1 and 32000.");
    return ReturnValue::eFAILURE;
  }

  ch->send(u"Zone %1's lifetime changed from %2 to %3.\r\n"_s.arg(zone.getID()).arg(zone.lifespan).arg(ticks));
  zone.lifespan = ticks;

  return ReturnValue::eSUCCESS;
}

command_return_t zedit_edit(CharacterPtr ch, QStringList arguments, Zone &zone)
{
  if (arguments.size() < 3 || arguments.at(0).isEmpty())
  {
    send_to_char("$3Usage$R:  zedit edit <cmdnumber> <type|if|1|2|3|comment> <value>\r\n"
                 "Valid types are:   'M', 'O', 'P', 'G', 'E', 'D', '*', 'X', 'K', and '%'.\r\n"
                 "Valid ifs are:     0(always), 1(ontrue), 2(onfalse), 3(onboot).\r\n"
                 "Valid args (123):  0-32000\r\n"
                 "                   (for max-in-world, -1 represents 'always')\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  QString text = arguments.at(0);
  bool ok = false;
  quint64 cmd = getZoneCommandKey(ch, zone, text, &ok);
  if (!ok)
  {
    return ReturnValue::eFAILURE;
  }

  QString select = arguments.at(1);
  QString last = arguments.at(2);
  quint64 i = 0, j = {};

  if (!select.isEmpty() && !last.isEmpty() && cmd >= 0)
  {
    if (isexact(select, "type"))
    {
      auto result = last.at(0).toUpper().toLatin1();

      switch (result)
      {
      case 'M':
      case 'O':
      case 'P':
      case 'G':
      case 'E':
      case 'D':
      case 'K':
      case 'X':
      case '*':
        zone.cmd[cmd]->arg1 = {};
        zone.cmd[cmd]->arg2 = {};
        zone.cmd[cmd]->arg3 = {};
        /* no break */
      case '%':
        zone.cmd[cmd]->arg2 = 100;
        zone.cmd[cmd]->command = result;
        ch->send(u"Type for command %1 changed to %2.\r\nArg1-3 reset.\r\n"_s.arg(cmd + 1).arg(result));
        break;
      default:
        ch->sendln("Type must be:  M, O, P, G, E, D, X, K, *, or %.");
        break;
      }
    }
    else if (isexact(select, "if"))
    {
      switch (last.at(0).toLatin1())
      {
      case '0':
        zone.cmd[cmd]->if_flag = {};
        ch->send(u"If flag for command %1 changed to 0 (always).\r\n"_s.arg(cmd + 1));
        break;
      case '1':
        zone.cmd[cmd]->if_flag = 1;
        ch->send(u"If flag for command %1 changed to 1 ($B$2ontrue$R).\r\n"_s.arg(cmd + 1));
        break;
      case '2':
        zone.cmd[cmd]->if_flag = 2;
        ch->send(u"If flag for command %1 changed to 2 ($B$4onfalse$R).\r\n"_s.arg(cmd + 1));
        break;
      case '3':
        zone.cmd[cmd]->if_flag = 3;
        ch->send(u"If flag for command %1 changed to 3 ($B$5onboot$R).\r\n"_s.arg(cmd + 1));
        break;
      case '4':
        zone.cmd[cmd]->if_flag = 4;
        ch->send(u"If flag for command %1 changed to 4 if-last-mob-true ($B$2Ls$1Mb$2Tr$R).\r\n"_s.arg(cmd + 1));
        break;
      case '5':
        zone.cmd[cmd]->if_flag = 5;
        ch->send(u"If flag for command %1 changed to 5 if-last-mob-false ($B$4Ls$1Mb$4Fl$R).\r\n"_s.arg(cmd + 1));
        break;
      case '6':
        zone.cmd[cmd]->if_flag = 6;
        ch->send(u"If flag for command %1 changed to 6 if-last-obj-true ($B$2Ls$7Ob$2Tr$R).\r\n"_s.arg(cmd + 1));
        break;
      case '7':
        zone.cmd[cmd]->if_flag = 7;
        ch->send(u"If flag for command %1 changed to 7 if-last-obj-false ($B$4Ls$7Ob$4Fl$R).\r\n"_s.arg(cmd + 1));
        break;
      case '8':
        zone.cmd[cmd]->if_flag = 8;
        ch->send(u"If flag for command %1 changed to 8 if-last-%%-true ($B$2Ls$R%%%%$B$2Tr$R).\r\n"_s.arg(cmd + 1));
        break;
      case '9':
        zone.cmd[cmd]->if_flag = 9;
        ch->send(u"If flag for command %1 changed to 9 if-last-%%-false ($B$4Ls$R%%%%$B$4Fl$R).\r\n"_s.arg(cmd + 1));
        break;
      default:
        ch->send("Legal values are 0 (always), 1 (ontrue), 2 (onfalse), 3 (onboot),\r\n"
                 "                 4 (if-last-mob-true),   5 (if-last-mob-false),\r\n"
                 "                 6 (if-last-obj-true),   7 (if-last-obj-false),\r\n"
                 "                 8 (if-last-%%-true),    9 (if-last-%%-false)\r\n");
        return ReturnValue::eFAILURE;
      }
    }
    else if (isexact(select, "comment"))
    {
      //      if(zone.cmd[cmd]->comment)
      //        zone.cmd[cmd]->comment={};
      if (last == "none")
      {
        zone.cmd[cmd]->comment = {};
        ch->send(u"Comment for command %d removed.\r\n"_s.arg(cmd + 1));
      }
      else
      {
        zone.cmd[cmd]->comment = last;
        ch->send(u"Comment for command %1 change to '%2'.\r\n"_s.arg(cmd + 1).arg(zone.cmd[cmd]->comment));
      }
    }
    else
    {
      bool ok = false;
      i = last.toULongLong(&ok);
      if (!ok)
      {
        ch->sendln("That was not a valid number for an argument.");
        return ReturnValue::eFAILURE;
      }

      room_t vnum = {};
      quint8 argument_number = {};
      decltype(zone.cmd[cmd]->arg1) original_value = 0, new_value = {};
      QString change_type;
      if (isexact(select, "1"))
      {
        argument_number = 1;
        switch (zone.cmd[cmd]->command)
        {
        case 'M':
          j = real_mobile(i);
          original_value = dc_->mob_index[zone.cmd[cmd]->arg1].vnum();
          break;
        case 'P':
        case 'G':
        case 'O':
        case 'E':
          j = real_object(i);
          original_value = dc_->obj_index[zone.cmd[cmd]->arg1].vnum();
          break;
        case 'X':
        case 'K':
        case 'D':
        case '%':
        case 'J':
        case '*':
        default:
          j = i;
          original_value = i;
          break;
        }
        zone.cmd[cmd]->arg1 = j;
        new_value = i;
      }
      else if (isexact(select, "2"))
      {
        argument_number = 2;
        switch (zone.cmd[cmd]->command)
        {
        case 'M': // Load mobile
        case 'P': // Put object in object
        case 'E': // Equib item on a mobile
        case 'O': // Load object on ground
        case 'G': // Give object to mobile (in inventory)
          change_type = "quantity";
          break;
        case 'D': // Set door state
          change_type = "direction";
          break;
        case '%': // Cause next command to occur x times out of y
          change_type = "numberator";
          break;
        case 'K': // skip the next number of specified zone commands
        case 'X': // type of if-flags to set to unsure
          ch->send("There is no arg2 for K or X commands.\r\n");
          return ReturnValue::eFAILURE;
        default:
          break;
        }
        original_value = zone.cmd[cmd]->arg2;
        zone.cmd[cmd]->arg2 = i;
        new_value = zone.cmd[cmd]->arg2;
      }
      else if (isexact(select, "3"))
      {
        argument_number = 3;
        switch (zone.cmd[cmd]->command)
        {
        case 'M': // Load mobile
        case 'O': // Load object on ground
          change_type = "room ID";
          vnum = i;
          i = real_room(i);
          break;
        case '%': // Cause next command to occur x times out of y
        case 'K': // skip the next number of specified zone commands
        case 'X': // type of if-flags to set to unsure
          ch->sendln("There is no arg3 for %, K or X commands.");
          return ReturnValue::eFAILURE;
          break;
        case 'P': // Put object in object
          change_type = "object VNUM";
          vnum = i;
          i = real_object(i);
          break;
        case 'G': // Give object to mobile (in inventory)
        case 'E': // Equib item on a mobile
        case 'D': // Set door state
        case 'J':
        default:
          break;
        }
        original_value = zone.cmd[cmd]->arg3;
        zone.cmd[cmd]->arg3 = i;
        new_value = zone.cmd[cmd]->arg3;
      }
      else
      {
        ch->send("Invalid argument. Valid values are 1, 2 or 3.\r\n");
        return ReturnValue::eFAILURE;
      }

      if (!change_type.isEmpty())
      {
        change_type += " ";
      }
      ch->send(u"Zone %1, command %2, argument %3 changed from %4%5 to %6.\r\n"_s.arg(zone.getID()).arg(cmd + 1).arg(argument_number).arg(change_type).arg(original_value).arg(new_value));
    }
    return ReturnValue::eFAILURE;
  }

  return ReturnValue::eFAILURE;
}

command_return_t zedit_remove(CharacterPtr ch, QStringList arguments, Zone &zone)
{

  if (arguments.size() < 1)
  {
    send_to_char("$3Usage$R: zedit remove <zonecmdnumber>\r\n"
                 "This will remove the command from the zonefile.\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  QString text = arguments.at(0);
  bool ok = false;

  quint64 zone_command_number = getZoneCommandKey(ch, zone, text, &ok);
  if (!ok)
  {
    return ReturnValue::eFAILURE;
  }

  zone.cmd.remove(zone_command_number);

  ch->send(u"Command %1 removed.\r\n"_s.arg(zone_command_number + 1));
  return ReturnValue::eSUCCESS;
}

qsizetype zone_get_last_command(const Zone &zone)
{
  return zone.cmd.size();
}

zone_t zedit_add(CharacterPtr ch, QStringList arguments, Zone &zone)
{
  if (arguments.size() < 1)
  {
    send_to_char("$3Usage$R: zedit add <new>\r\n"
                 "       zedit add <command number>\r\n"
                 "Adding 'new' will add a command to the end.\r\n"
                 "Adding a number, will insert a new command at that place in\r\n"
                 "the list, pushing the rest of the items back.\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  QString text = arguments.at(0);
  if (isexact(text, "new"))
  {
    zone.cmd.push_back(QSharedPointer<ResetCommand>::create('J'));
    ch->send(u"New command 'J' added at %1.\r\n"_s.arg(zone.cmd.size()));
    return zone.cmd.size() - 1;
  }

  bool ok = false;
  quint64 i = getZoneCommandKey(ch, zone, text, &ok);
  if (!ok)
  {
    return ReturnValue::eFAILURE;
  }

  zone.cmd.insert(i, QSharedPointer<ResetCommand>::create('J'));
  ch->send(u"New command 'J' added at %1.\r\n"_s.arg(i + 1));
  return i - 1;
}

command_return_t zedit_list(CharacterPtr ch, QStringList arguments, const Zone &zone, bool stats)
{
  if (arguments.isEmpty())
  {
    return show_zone_commands(ch, zone, 0, 0, stats);
  }

  QString text = arguments.at(0);

  // list 1
  bool ok = false;
  quint64 command_number = getZoneCommandKey(ch, zone, text, &ok);
  if (!ok)
  {
    return ReturnValue::eFAILURE;
  }

  quint64 num_to_show = 1;
  if (arguments.size() > 1)
  {
    bool ok = false;
    num_to_show = arguments.at(1).toULongLong(&ok);
    if (!ok || num_to_show < 1)
    {
      num_to_show = 1;
    }
  }

  show_zone_commands(ch, zone, command_number, num_to_show, stats);

  return ReturnValue::eSUCCESS; // so we don't set_zone_modified_zone
}

command_return_t zedit_name(CharacterPtr ch, QStringList arguments, Zone &zone)
{
  if (arguments.isEmpty())
  {
    send_to_char("$3Usage$R: zedit name <newname>\r\n"
                 "This changes the name of the zone.\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  zone.name(arguments.join(' '));

  ch->send(u"Zone %1's name changed to '%2'.\r\n"_s.arg(zone.getID()).arg(zone.name()));

  return ReturnValue::eSUCCESS;
}

command_return_t zedit_mode(CharacterPtr ch, QStringList arguments, Zone &zone)
{
  if (arguments.isEmpty())
  {
    send_to_char("$3Usage$R: zedit mode <modetype>\r\n"
                 "You much choose the rule the zone follows when it\r\n"
                 "attempts to repop itself.  Available modes are:\r\n",
                 ch);
    QString buffer;
    for (quint64 j = {}; *zone_modes[j] != '\n'; j++)
    {
      ch->send(u"  $C%1$R: %2\r\n"_s.arg(j + 1).arg(zone_modes[j]));
    }
    return ReturnValue::eFAILURE;
  }

  QString text = arguments.value(0);
  bool ok = false;
  quint64 k = text.toULongLong(&ok);

  if (!ok || k > 3)
  {
    ch->sendln("You must choose between 1 and 3.");
    return ReturnValue::eFAILURE;
  }

  zone.reset_mode = k - 1;

  ch->send(u"Zone %1's reset mode changed to %2(%3).\r\n"_s.arg(zone.getID()).arg(zone_modes[k - 1]).arg(k));
  return ReturnValue::eSUCCESS;
}

const QStringList zedit_subcommands = {
    "remove", "add", "edit", "list", "name",
    "lifetime", "mode", "flags", "help", "search",
    "swap", "copy", "continent", "info"};

command_return_t zedit_help(CharacterPtr ch)
{
  send_to_char("$3Usage$R: zedit <subcommand> [arguments]\r\n"
               "       zedit <zone number> <subcommand> [arguments]\r\n"
               "If you don't specify the zone number then whatever zone you are in is used.\r\n"
               "Subcommands:\r\n",
               ch);
  ch->display_string_list(zedit_subcommands);
  return ReturnValue::eSUCCESS;
}

command_return_t do_zedit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf, text, arg;
  quint64 i = 0, j = 0, num_to_show = 0, last_cmd = 0, ticks = 0, cont = {};
  quint32 k = {};
  vnum_t robj = {}, rmob = {};
  bool ok = false;
  QSharedPointer<ResetCommand> tmp = {}, temp_com = {};
  QString str = {};

  QStringList arguments = QString(argument).trimmed().split(' ');

  if (arguments.isEmpty())
  {
    return zedit_help(ch);
  }

  bool stats = false;
  if (arguments.last() == "stats")
  {
    stats = true;
    arguments.pop_back();
  }

  QString select = arguments.value(0);
  if (select.isEmpty())
  {
    return zedit_help(ch);
  }

  ok = false;
  zone_t zone_key = select.toULongLong(&ok);
  if (!ok)
  {
    zone_key = dc_->world[ch->in_room].zone;
  }
  else
  {
    if (!arguments.isEmpty())
    {
      arguments.pop_front();
    }
    select = arguments.value(0);
  }

  if (select.isEmpty())
  {
    return zedit_help(ch);
  }

  bool subcommand_found = {};
  quint64 subcommand_id = {};
  for (subcommand_id = {}; subcommand_id < zedit_subcommands.size(); subcommand_id++)
  {
    if (is_abbrev(select, zedit_subcommands[subcommand_id]))
    {
      subcommand_found = true;
      break;
    }
  }

  if (!subcommand_found)
  {
    ch->send(u"'%1' is an invalid subcommand.\r\n"_s.arg(select));
    return zedit_help(ch);
  }

  if (!can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You are unable to modify a zone other than the one your room-range is in.");
    return ReturnValue::eFAILURE;
  }
  quint64 from = {}, to = {};

  // set the zone we're in

  if (!isValidZoneKey(ch, zone_key))
  {
    return ReturnValue::eFAILURE;
  }

  Zone &zone = dc_->zones[zone_key];

  last_cmd = zone_get_last_command(zone);

  if (!arguments.isEmpty())
  {
    arguments.pop_front();
  }
  switch (subcommand_id)
  {
  case 0: /* remove */
    zedit_remove(ch, arguments, zone);
    break;
  case 1: /* add */
    zedit_add(ch, arguments, zone);
    break;
  case 2: /* edit */
    zedit_edit(ch, arguments, zone);
    break;
  case 3: /* list */
    zedit_list(ch, arguments, zone, stats);
    break;
  case 4: /* name */
    zedit_name(ch, arguments, zone);
    break;
  case 5: /* lifetime */
    zedit_lifetime(ch, arguments, zone);
    break;
  case 6: /* mode */
    zedit_mode(ch, arguments, zone);
    break;
  case 7: /* flags */
    zedit_flags(ch, arguments, zone);
    break;
  case 8: /* help */

    send_to_char("\r\n"
                 "Most commands will give you help on their own, just type them\r\n"
                 "without any arguments.\r\n\r\n"
                 "Details on zone command types:\r\n"
                 "   M = load mob          O = load object\r\n"
                 "   P = put obj in obj    G = give item to mob\r\n"
                 "   E = equip item on mob D = set door\r\n"
                 "   * = comment           % = be true on x times out of y\r\n"
                 "   X = set a true-false flag to an 'unsure' state\r\n"
                 "   K = skip the next [arg1] number of commands.\r\n"
                 "\r\n"
                 "For comments, if you wish to remove a comment set the comment to 'none'.\r\n"
                 "\r\n",
                 ch);
    return ReturnValue::eSUCCESS; // so we don't set modified
    break;
  case 9: /* search */

    if (arguments.isEmpty())
    {
      send_to_char("$3Usage$R: zedit search <number>\r\n"
                   "This searches your current zonefile for any commands\r\n"
                   "containing that number.  It then lists them to you.  If you're\r\n"
                   "a deity or higher it will also warn you if that number appears\r\n"
                   "in any other zonefiles.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }

    text = arguments.at(0);
    ok = false;
    j = text.toULongLong(&ok);

    if (!ok)
    {
      ch->sendln("Please specifiy a valid number.");
      return ReturnValue::eFAILURE;
    }

    ch->sendln("Matches:");

    robj = real_object(j);
    rmob = real_mobile(j);

    // If that obj/mob doesn't exist, put a junk value that should never (hopefully) match
    if (robj == -1)
    {
      robj = -93294;
    }
    else
    {
      ch->send(u"Searching for Object rnum %1 with vnum %2\r\n"_s.arg(robj).arg(j));
    }
    if (rmob == -1)
    {
      rmob = -93294;
    }
    else
    {
      ch->send(u"Searching for Mobile rnum %1 with vnum %2\r\n"_s.arg(rmob).arg(j));
    }

    for (const auto [z_key, zone] : dc_->zones.asKeyValueRange())
    {
      for (i = {}; i < zone.cmd.size(); i++)
      {
        switch (zone.cmd[i]->command)
        {
        case 'M':
          if (rmob == zone.cmd[i]->arg1)
          {
            ch->send(u" Zone %d  Command %d (%c)\r\n"_s.arg(z_key).arg(i + 1).arg(zone.cmd[i]->command));
            if (stats)
            {
              str = strdup(u" %1 list %2 1 stats\r\n"_s.arg(z_key).arg(i + 1).toStdString().c_str());
            }
            else
            {
              str = strdup(u" %1 list %2 1\r\n"_s.arg(z_key).arg(i + 1).toStdString().c_str());
            }
            do_zedit(ch, str);
            free(str);
          }
          break;
        case 'G': // G, E, and O have obj # in arg1
        case 'E':
        case 'O':
          if (robj == zone.cmd[i]->arg1)
          {
            ch->send(u" Zone %d  Command %d (%c)\r\n"_s.arg(z_key).arg(i + 1).arg(zone.cmd[i]->command));
            if (stats)
            {
              str = strdup(u" %1 list %2 1 stats\r\n"_s.arg(z_key).arg(i + 1).toStdString().c_str());
            }
            else
            {
              str = strdup(u" %1 list %2 1\r\n"_s.arg(z_key).arg(i + 1).toStdString().c_str());
            }

            do_zedit(ch, str);
            free(str);
          }
          break;
        case 'P': // P has obj # in arg1 and arg3
          if (robj == zone.cmd[i]->arg1 ||
              robj == zone.cmd[i]->arg3)
          {
            ch->send(u" Zone %d  Command %d (%c)\r\n"_s.arg(z_key).arg(i + 1).arg(zone.cmd[i]->command));
            if (stats)
            {
              str = strdup(u" %1 list %2 1 stats\r\n"_s.arg(z_key).arg(i + 1).toStdString().c_str());
            }
            else
            {
              str = strdup(u" %1 list %2 1\r\n"_s.arg(z_key).arg(i + 1).toStdString().c_str());
            }

            do_zedit(ch, str);
            free(str);
          }
          break;
        default:
          break;
        }
      }
    }
    return ReturnValue::eSUCCESS;
    break;

  case 10: // swap

    if (arguments.size() < 2)
    {
      send_to_char("$3Usage$R: zedit swap <cmd1> <cmd2>\r\n"
                   "This swaps the positions of two zone commands.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    select = arguments.at(0);
    text = arguments.at(1);

    ok = false;
    i = select.toULongLong(&ok);
    if (!ok)
    {
      ch->sendln("Invalid command num for cmd1.");
      return ReturnValue::eFAILURE;
    }

    ok = false;
    j = text.toULongLong(&ok);
    if (!ok)
    {
      ch->sendln("Invalid command num for cmd2.");
      return ReturnValue::eFAILURE;
    }

    // swap i and j
    temp_com = {};

    temp_com = zone.cmd[i - 1];
    zone.cmd[i - 1] = zone.cmd[j - 1];
    zone.cmd[j - 1] = temp_com;

    ch->send(u"Commands %d and %d swapped.\r\n"_s.arg(i).arg(j));
    break;

  case 11: // copy

    if (arguments.size() < 1)
    {
      ch->send("$3Usage$R: zedit copy <source line> <destination line>\r\nDestination line is optional. If no such line exists, it tacks it on at the end.");
      return ReturnValue::eFAILURE;
    }
    text = arguments.at(0);
    arg = arguments.at(1);

    ok = false;
    from = text.toLongLong(&ok) - 1;
    to = {};
    if (!arg.isEmpty())
    {
      ok = false;
      to = arg.toLongLong(&ok);
    }

    if (from > last_cmd || from < 0)
    {
      buf = QString("Source line must be between 1 and %1.\r\n"
                    "'%2' is not valid.\r\n")
                .arg(zone.cmd.size())
                .arg(text);
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }

    tmp = zone.cmd[from];
    if (to)
    {
      // bump everything up a slot
      for (j = last_cmd; j != (to - 2); j--)
        zone.cmd[j + 1] = zone.cmd[j];
      buf = u"Command copied to %1.\r\n"_s.arg(to);
      to--;
    }
    else // tack it on the end
    {
      // bump the 'S' up
      zone.cmd[last_cmd + 1] = zone.cmd[last_cmd];
      to = last_cmd;
      buf = "Command copied.\r\n";
    }
    zone.cmd[to]->active = tmp->active;
    zone.cmd[to]->command = tmp->command;
    zone.cmd[to]->if_flag = tmp->if_flag;
    zone.cmd[to]->arg1 = tmp->arg1;
    zone.cmd[to]->arg2 = tmp->arg2;
    zone.cmd[to]->arg3 = tmp->arg3;
    zone.cmd[to]->comment = tmp->comment;
    ch->send(buf);
    break;

  case 12: // continent

    if (arguments.size() < 1)
    {
      ch->sendln("$3Usage$R: zedit continent <continent number>");
      for (cont = NO_CONTINENT; cont != continent_names.size(); cont++)
      {

        ch->send(u"%d) %s\r\n"_s.arg(cont).arg(continent_names.at(cont).c_str()));
      }
      return ReturnValue::eFAILURE;
    }
    text = arguments.at(0);
    ok = false;
    cont = text.toLongLong(&ok);
    if (!ok || !cont || cont > continent_names.size() - 1)
    {
      ch->send(u"You much choose between 1 and %d.\r\n"_s.arg(continent_names.size()));
      return ReturnValue::eFAILURE;
    }
    ch->send(u"Success. Continent changed to %s\r\n"_s.arg(continent_names.at(cont).c_str()));
    zone.continent = cont;
    break;

  case 13:
    zone.show_info(ch);
    return ReturnValue::eSUCCESS;
    break;

  default:
    ch->sendln("Error:  Couldn't find item in switch.");
    return ReturnValue::eFAILURE;
    break;
  }
  dc_->set_zone_modified_zone(ch->in_room);
  return ReturnValue::eSUCCESS;
}

command_return_t do_sedit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  QString select;
  QString target;
  QString text;
  QString value;
  CharacterPtr vict = {};
  char_skill_data *skill = {};
  char_skill_data *lastskill = {};
  qint16 field = {};
  qint16 skillnum = {};
  qint16 learned = {};
  qint32 i = {};

  const QStringList sedit_values = {
      "add",
      "remove",
      "set",
      "list",
      "\n"};

  if (!ch->has_skill(COMMAND_SEDIT))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  std::tie(target, text) = half_chop(argument);
  std::tie(select, text) = half_chop(text);
  // at this point target is the character's name
  // select is the field, and text is any args

  if (select.empty() || target.empty())
  {
    send_to_char("$3Usage$R: sedit <character> <field> [args]\r\n"
                 "Use a field with no args to get help on that field.\r\n"
                 "Fields are the following.\r\n",
                 ch);
    ch->display_string_list(sedit_values);
    return ReturnValue::eFAILURE;
  }

  if (!(vict = get_char_vis(ch, target.c_str())))
  {
    ch->send(fmt::format("Cannot find player '{}'.\r\n", target));
    return ReturnValue::eFAILURE;
  }

  if (vict->getLevel() > ch->getLevel())
  {
    ch->send("You like to play dangerously don't you....\r\n");
    return ReturnValue::eFAILURE;
  }

  field = sedit_values.indexOf(select, Qt::CaseInsensitive);
  if (field < 0)
  {
    ch->sendln("That field not recognized.");
    return ReturnValue::eFAILURE;
  }

  if (field == 2)
  {
    std::tie(value, text) = last_argument(text);
  }

  if (field == 0 || field == 1 || field == 2)
  {
    skill_results_t results = find_skills_by_name(text);

    if (results.empty())
    {
      ch->send(fmt::format("Cannot find skill '{}' in master skill list.\r\n", text));
      return ReturnValue::eFAILURE;
    }

    if (results.size() > 1)
    {
      ch->send(fmt::format("Skill '{}' is too ambiguous. Please specify one of the following:\r\n", text));
      for (const auto &result : results)
      {
        ch->send(fmt::format("{}\r\n", result.first));
      }
      ch->send("\r\n");

      return ReturnValue::eFAILURE;
    }

    skillnum = results.begin()->second;
    text = results.begin()->first;
  }

  switch (field)
  {
  case 0: /* add */
  {
    if (text.empty())
    {
      ch->send("$3Usage$R: sedit <character> add <skillname>\r\n"
               "This will give the skill to the character at learning 1.\r\n");

      return ReturnValue::eFAILURE;
    }

    if (vict->has_skill(skillnum))
    {
      ch->send(fmt::format("'{}' already has '{}'.\r\n", qPrintable(vict->name()), text));
      return ReturnValue::eFAILURE;
    }

    vict->learn_skill(skillnum, 1, 1);

    buf = fmt::format("'{}' has been given skill '{}' ({}) by {}.", qPrintable(vict->name()), text, skillnum, qPrintable(ch->name()));
    dc_->logentry(buf.c_str(), ch->getLevel(), DC::LogChannel::LOG_GOD);
    ch->send(fmt::format("'{}' has been given skill '{}' ({}) by {}.\r\n", qPrintable(vict->name()), text, skillnum, qPrintable(ch->name())));
    break;
  }
  case 1: /* remove */
  {
    if (text.empty())
    {
      ch->send("$3Usage$R: sedit <character> remove <skillname>\r\n"
               "This will remove the skill from the character.\r\n");
      return ReturnValue::eFAILURE;
    }

    if (ch->skills.contains(skillnum))
    {
      ch->skills.erase(skillnum);

      buf = fmt::format("Skill '{}' ({}) removed from {} by {}.", text, skillnum, qPrintable(vict->name()), qPrintable(ch->name()));
      dc_->logentry(buf.c_str(), ch->getLevel(), DC::LogChannel::LOG_GOD);
      ch->send(fmt::format("Skill '{}' ({}) removed from {}.\r\n", text, skillnum, qPrintable(vict->name())));
    }
    else
    {
      ch->send(fmt::format("Cannot find skill '{}' on '{}'.\r\n", text, qPrintable(vict->name())));
      return ReturnValue::eFAILURE;
    }
    break;
  }
  case 2: /* set */
  {
    if (text.empty())
    {
      ch->send("$3Usage$R: sedit <character> set <skillname> <amount>\r\n"
               "This will set the character's skill to amount.\r\n");
      return ReturnValue::eFAILURE;
    }
    if (!(learned = vict->has_skill(skillnum)))
    {
      ch->send(fmt::format("'{}' does not have skill '{}'.\r\n", qPrintable(vict->name()), text));
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(i, value.c_str(), 1, 100))
    {
      ch->sendln("Invalid skill amount.  Must be 1 - 100.");
      return ReturnValue::eFAILURE;
    }

    vict->learn_skill(skillnum, i, i);

    buf = fmt::format("'{}'s skill '{}' set to {} from {} by {}.", qPrintable(vict->name()), text, i, learned, qPrintable(ch->name()));
    dc_->logentry(buf.c_str(), ch->getLevel(), DC::LogChannel::LOG_GOD);
    ch->send(fmt::format("'{}' skill '{}' set to {} from {}.\r\n", qPrintable(vict->name()), text, i, learned));
    break;
  }
  case 3: /* list */
  {
    i = {};
    ch->send(fmt::format("$3Skills for$R:  {}\r\n"
                         "  {:<18}  {:<4}  Learned\r\n"
                         "$3-------------------------------------$R\r\n",
                         qPrintable(vict->name()), "Skill", "#"));
    for (const auto &skill : vict->skills)
    {
      QString skillname = get_skill_name(skill.first);

      if (!skillname.isEmpty())
      {
        ch->send(fmt::format("  {:<18}  {:<4}  {}\r\n", skillname.toStdString(), skill.first, skill.second.learned));
      }
      else
      {
        ch->send(fmt::format("  {:<18}  {:<4}  {}\r\n", "unknown", skill.first, skill.second.learned));
      }

      i = 1;
    }
    if (!i)
    {
      ch->send("  (none)\r\n");
    }
    return ReturnValue::eSUCCESS;
    break;
  }

  default:
    ch->send("Error:  Couldn't find item in switch (sedit).\r\n");
    break;
  }

  // make sure the changes stick
  vict->save(cmd_t::SAVE_SILENTLY);

  return ReturnValue::eSUCCESS;
}

qint32 oedit_exdesc(CharacterPtr ch, qint32 item_num, QString buf)
{
  QString type;
  QString buf2;
  QString select;
  QString value;
  qint32 x;
  ObjectPtr obj = {};
  qint32 num;

  extra_descr_data *curr = {};
  extra_descr_data *curr2 = {};

  const QStringList fields =
      {
          "new",
          "delete",
          "keywords",
          "desc",
          "\n"};

  half_chop(buf, type, buf2);
  half_chop(buf2, select, value);

  // type = add new delete
  // select = # of affect
  // value = value to change aff to

  obj = dc_->obj_index[item_num].item;

  if (buf.isEmpty())
  {
    send_to_char("$3Syntax$R:  oedit [item_num] exdesc <field> [values]\r\n"
                 "The field must be one of the following:\r\n",
                 ch);
    ch->display_string_list(fields);
    ch->sendln("\r\n$3Current Descs$R:");
    for (x = 1, curr = obj->ex_description; curr; x++, curr = curr->next)
      ch->send(u"$3%d$R) %s\r\n%s\r\n"_s.arg(x).arg(curr->keyword).arg(curr->description));
    return ReturnValue::eFAILURE;
  }

  for (x = {};; x++)
  {
    if (fields[x][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return ReturnValue::eFAILURE;
    }
    if (is_abbrev(type, fields[x]))
      break;
  }

  switch (x)
  {
  // new
  case 0:
  {
    if (select.isEmpty())
    {
      send_to_char("$3Syntax$R: oedit [item_num] exdesc new <keywords>\r\n"
                   "This adds a new description with the keywords chosen.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    curr = (extra_descr_data *)calloc(1, sizeof(extra_descr_data));
    curr->keyword = select;
    curr->description = u"Empty desc.\r\n"_s;
    curr->next = obj->ex_description;
    obj->ex_description = curr;
    ch->sendln("New desc created.");
    break;
  }

  // delete
  case 1:
  {
    if (select.isEmpty())
    {
      send_to_char("$3Syntax$R: oedit [item_num] exdesc delete <number>\r\n"
                   "This removes desc <number> from the list permanently.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(num, select, 1, 50))
    {
      ch->sendln("You must select a valid number.");
      return ReturnValue::eFAILURE;
    }
    x = 1;
    curr2 = {};
    for (curr = obj->ex_description; x < num && curr; curr = curr->next)
    {
      curr2 = curr;
      x++;
    }

    if (!curr)
    {
      ch->sendln("There is no desc for that number.");
      return ReturnValue::eFAILURE;
    }

    if (!curr2)
    { // first one
      obj->ex_description = curr->next;
      curr = {};
    }
    else
    {
      curr2->next = curr->next;
      curr = {};
    }
    ch->sendln("Deleted.");
    break;
  }

  // keywords
  case 2:
  {
    if (select.isEmpty() || value.isEmpty())
    {
      send_to_char("$3Syntax$R: oedit [item_num] exdesc keywords <number> <new keywords>\r\n"
                   "This removes desc <number> from the list permanently.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(num, select, 1, 50))
    {
      ch->sendln("You must select a valid number.");
      return ReturnValue::eFAILURE;
    }
    for (curr = obj->ex_description, x = 1; x < num && curr; curr = curr->next)
      x++;

    if (!curr)
    {
      ch->sendln("There is no desc for that number.");
      return ReturnValue::eFAILURE;
    }
    curr->keyword = value;
    ch->sendln("New keyword set.");
    break;
  }

  // desc
  case 3:
  {
    if (select.isEmpty())
    {
      send_to_char("$3Syntax$R: oedit [item_num] exdesc desc <number>\r\n"
                   "This removes desc <number> from the list permanently.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(num, select, 1, 50))
    {
      ch->sendln("You must select a valid number.");
      return ReturnValue::eFAILURE;
    }
    for (curr = obj->ex_description, x = 1; x < num && curr; curr = curr->next)
      x++;

    if (!curr)
    {
      ch->sendln("There is no desc for that number.");
      return ReturnValue::eFAILURE;
    }
    ch->sendln("        Write your obj's description.  (/s saves /h for help)");

    //        send_to_char("Enter your obj's description below."
    //                   " Terminate with '~' on a new line.\r\n\r\n", ch);
    //        curr->description = {};

    ch->desc->connected = Connection::states::EDITING;
    ch->desc->strnew = &(curr->description);

    break;
  }

  default:
    ch->sendln("Illegal value, tell someone.");
    break;
  } // switch(x)

  dc_->set_zone_modified_obj(item_num);
  return ReturnValue::eSUCCESS;
}

qint32 oedit_affects(CharacterPtr ch, qint32 item_num, QString buf)
{
  QString type;
  QString buf2;
  QString select;
  QString value;
  qint32 x;
  ObjectPtr obj = {};
  qint32 num;
  qint32 modifier;

  const QStringList fields =
      {
          "new",
          "delete",
          "list",
          "1spell",
          "2amount",
          "3skill",
          "\n"};

  half_chop(buf, type, buf2);
  half_chop(buf2, select, value);

  // type = add new delete
  // select = # of affect
  // value = value to change aff to

  if (buf.isEmpty())
  {
    send_to_char("$3Syntax$R:  oedit [item_num] affects [affectnumber] [value]\r\n"
                 "The field must be one of the following:\r\n",
                 ch);
    ch->display_string_list(fields);
    return ReturnValue::eFAILURE;
  }

  for (x = {};; x++)
  {
    if (fields[x][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return ReturnValue::eFAILURE;
    }
    if (is_abbrev(type, fields[x]))
      break;
  }

  obj = dc_->obj_index[item_num].item;

  switch (x)
  {
  // new
  case 0:
  {
    if (select.isEmpty())
    {
      send_to_char("$3Syntax$R: oedit [item_num] affects new yes\r\n"
                   "This adds a new blank affect to the end of the list.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    add_obj_affect(obj, 0, 0);
    ch->sendln("New affect created.");
    break;
  }

  // delete
  case 1:
  {
    if (select.isEmpty())
    {
      send_to_char("$3Syntax$R: oedit [item_num] affects delete <number>\r\n"
                   "This removes affect <number> from the list permanently.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    if (!obj->affected)
    {
      dc_sprintf(buf, "Object %d has no affects to delete.\r\n", dc_->obj_index[item_num].vnum());
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert<decltype(num)>(num, select, 1, obj->num_affects))
    {
      dc_sprintf(buf, "You must select between 1 and %d.\r\n", obj->num_affects);
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    remove_obj_affect_by_index(obj, num - 1);
    ch->sendln("Affect deleted.");
    break;
  }

  // list
  case 2:
  {
    if (!obj->affected)
    {
      ch->sendln("The object has no affects.");
      return ReturnValue::eSUCCESS;
    }
    send_to_char("$3Character Affects$R:\r\n"
                 "------------------\r\n",
                 ch);
    for (x = {}; x < obj->num_affects; x++)
    {
      //          sprinttype(obj->affected[x].location, apply_types, buf2);

      if (obj->affected[x].location < 1000)
        sprinttype(obj->affected[x].location, apply_types, buf2);
      else if (!get_skill_name(obj->affected[x].location / 1000).isEmpty())
        dc_strcpy(buf2, get_skill_name(obj->affected[x].location / 1000).toStdString().c_str());

      dc_sprintf(buf, "%2d$3)$R %s$3($R%d$3)$R by %d.\r\n", x + 1, buf2,
                 obj->affected[x].location, obj->affected[x].modifier);
      ch->send(buf);
    }
    return ReturnValue::eSUCCESS; // return so we don't mark as changed
    break;
  }

  // 1spell
  case 3:
  {
    if (select.isEmpty())
    {
      send_to_char("$3Syntax$R: oedit [item_num] affects 1spell <number> <value>\r\n"
                   "This sets the modifying spell affect to <value> for affect <number>.\r\n",
                   ch);
      for (x = {}; x <= APPLY_MAXIMUM_VALUE; x++)
      {
        dc_sprintf(buf, "%3d$3)$R %s\r\n", x, apply_types[x]);
        ch->send(buf);
      }
      ch->sendln("Make $B$5sure$R you don't use a spell that is restricted.  See builder guide.");
      return ReturnValue::eFAILURE;
    }
    if (!obj->affected)
    {
      dc_sprintf(buf, "Object %d has no affects to modify.\r\n", dc_->obj_index[item_num].vnum());
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert<decltype(num)>(num, select, 1, obj->num_affects))
    {
      dc_sprintf(buf, "You must select between 1 and %d.\r\n", obj->num_affects);
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    num -= 1; // since arrays start at 0
    if (!check_range_valid_and_convert(modifier, value, APPLY_NONE, APPLY_MAXIMUM_VALUE))
    {
      dc_sprintf(buf, "You must select between %d and %d.\r\n", APPLY_NONE, APPLY_MAXIMUM_VALUE);
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    obj->affected[num].location = modifier;
    dc_sprintf(buf, "Affect %d changed to %s by %d.\r\n", num + 1,
               apply_types[obj->affected[num].location], obj->affected[num].modifier);
    ch->send(buf);
    break;
  }

  // 2amount
  case 4:
  {
    if (select.isEmpty())
    {
      send_to_char("$3Syntax$R: oedit [item_num] affects 1amount <number> <value>\r\n"
                   "This sets the spell affect's modifier to <value> for affect <number>.\r\n"
                   "Currently limited from -100 to 100.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    if (!obj->affected)
    {
      dc_sprintf(buf, "Object %d has no affects to modify.\r\n",
                 dc_->obj_index[item_num].vnum());
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert<decltype(num)>(num, select, 1, obj->num_affects))
    {
      dc_sprintf(buf, "You must select between 1 and %d.\r\n",
                 obj->num_affects);
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(modifier, value, -100, 100))
    {
      ch->sendln("You must select between -100 and 100.");
      return ReturnValue::eFAILURE;
    }
    num -= 1; // since arrays start at 0
    obj->affected[num].modifier = modifier;
    if (obj->affected[num].location >= 1000)
    {
      QString skill_name = get_skill_name(obj->affected[num].location / 1000);

      ch->send(u"Affect %1 changed to %2 by %3.\r\n"_s.arg(num + 1).arg(skill_name).arg(obj->affected[num].modifier));
    }
    else
    {
      ch->send(u"Affect %1 changed to %2 by %3.\r\n"_s.arg(num + 1).arg(apply_types[obj->affected[num].location]).arg(obj->affected[num].modifier));
    }
    break;
  }
  case 5:
    if (select.isEmpty() || value.isEmpty())
    {
      send_to_char("$3Syntax$R: oedit [item_num] affects 3spell <number> <skill>\r\n"
                   "This sets the affect as affecting skills by 2amount\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    if (!obj->affected)
    {
      dc_sprintf(buf, "Object %d has no affects to modify.\r\n",
                 dc_->obj_index[item_num].vnum());
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert<decltype(num)>(num, select, 1, obj->num_affects))
    {
      dc_sprintf(buf, "You must select between 1 and %d.\r\n",
                 obj->num_affects);
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    modifier = atoi(value);
    num -= 1;
    obj->affected[num].location = modifier * 1000;
    ch->send(u"Affect %1 changed to %2 by %3.\r\n"_s.arg(num + 1).arg(get_skill_name(obj->affected[num].location / 1000)).arg(obj->affected[num].modifier));
    break;
  default:
    ch->sendln("Illegal value.");
    break;
  } // switch(x)

  dc_->set_zone_modified_obj(item_num);
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_oedit(QStringList arguments, cmd_t cmd)
{
  QString buf = {};
  QString buf2 = {};
  QString buf3 = {};
  QString buf4 = {};
  qint32 rnum = {};
  vnum_t vnum = {};
  qint32 intval = {};
  qint32 x = {}, i = {};

  const QStringList fields =
      {
          "keywords",
          "longdesc",
          "shortdesc",
          "actiondesc",
          "type",
          "wear",
          "size",
          "extra",
          "weight",
          "value",
          "moreflags",
          "level",
          "v1",
          "v2",
          "v3",
          "v4",
          "affects",
          "exdesc",
          "new",
          "delete",
          "stat",
          "timer",
          "description",
          "\n"};

  if (this->isNonPlayer())
    return ReturnValue::eFAILURE;

  half_chop(qPrintable(arguments.join(' ')), buf, buf2);
  half_chop(buf2, buf3, buf4);

  // at this point, buf  = item_num
  //                buf3 = field
  //                buf4 = args

  // or

  // buf = field
  // buf3 = args[0]
  // buf4 = args[1-+]

  if (buf.isEmpty())
  {
    send_to_char("$3Syntax$R:  oedit new [obj vnum]           -- Create new object\r\n"
                 "         oedit [obj vnum]               -- Stat object\r\n"
                 "         oedit [obj vnum] [field]       -- Help info for that field\r\n"
                 "         oedit [obj vnum] [field] [arg] -- Change that field\r\n"
                 "         oedit [field] [arg]            -- Change that field using last vnum\r\n\r\n"
                 "The field must be one of the following:\r\n",
                 this);
    display_string_list(fields);
    return ReturnValue::eFAILURE;
  }

  if (isdigit(*buf))
  {
    try
    {
      vnum = std::stoull(buf);
    }
    catch (...)
    {
      vnum = {};
    }

    rnum = real_object(vnum);
    if (rnum < 0 || vnum < 1)
    {
      sendln("Invalid item number.");
      return ReturnValue::eSUCCESS;
    }

    if (player->last_obj_vnum != vnum)
    {
      send(fmt::format("$3Current obj set to:$R {}\r\n", vnum));
      player->last_obj_vnum = vnum;
    }
  }
  else
  {
    vnum = player->last_obj_vnum;
    rnum = real_object(vnum);
    if (rnum < 0 || vnum < 1)
    {
      sendln("Invalid item number.");
      return ReturnValue::eSUCCESS;
    }

    // put the buffs where they should be
    if (*buf4)
    {
      dc_sprintf(buf2, "%s %s", buf3, buf4);
    }
    else
    {
      dc_strcpy(buf2, buf3);
    }

    dc_strcpy(buf4, buf2);
    dc_strcpy(buf3, buf);
  }

  if (buf.isEmpty() 3) // no field.  Stat the item.
  {
    obj_stat(this, dc_->obj_index[rnum].item);
    return ReturnValue::eSUCCESS;
  }

  // MOVED
  for (x = {};; x++)
  {
    if (fields[x][0] == '\n')
    {
      sendln("Invalid field.");
      return ReturnValue::eFAILURE;
    }
    if (is_abbrev(buf3, fields[x]))
      break;
  }

  // a this point, item_num is the index
  if (x != 18) // Checked in there
    if (!can_modify_object(this, vnum))
    {
      sendln("You are unable to work creation outside of your range.");
      return ReturnValue::eFAILURE;
    }

  switch (x)
  {

    /* edit keywords */
  case 0:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [item_num] keywords <new_keywords>");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->name(buf4);
    dc_sprintf(buf, "Item keywords set to '%s'.\r\n", buf4);
    send(buf);
  }
  break;

    /* edit long desc */
  case 1:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [item_num] longdesc <new_desc>");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->long_description = buf4;
    dc_sprintf(buf, "Item longdesc set to '%s'.\r\n", buf4);
    send(buf);
  }
  break;

    // edit short desc
  case 2:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [item_num] shortdesc <new_desc>");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->short_description = buf4;
    dc_sprintf(buf, "Item shortdesc set to '%s'.\r\n", buf4);
    send(buf);
  }
  break;

    /* edit action desc */
  case 3:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [item_num] actiondesc <new_desc>");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->ActionDescription(buf4);
    dc_sprintf(buf, "Item actiondesc set to '%s'.\r\n", buf4);
    send(buf);
  }
  break;

    /* edit type */
  case 4:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: oedit [item_num] type <>\r\n"
                   "$3Current$R: ",
                   this);
      dc_snprintf(buf, sizeof(buf), "%s\n", item_types[(dc_->obj_index[rnum].item)->obj_flags.type_flag].toStdString().c_str());
      send(buf);
      sendln("\r\n$3Valid types$R:");

      for (i = 1; i < item_types.size(); i++)
      {
        send(u"%1) %2\r\n"_s.arg(i, 3).arg(item_types[i]));
      }
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, ITEM_TYPE_MAX))
    {
      sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    if (intval == 24)
    {
      (dc_->obj_index[rnum].item)->obj_flags.value[2] = -1;
    }
    else
    {
      (dc_->obj_index[rnum].item)->obj_flags.value[2] = {};
    }
    (dc_->obj_index[rnum].item)->obj_flags.type_flag = intval;
    dc_sprintf(buf, "Item type set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit wear */
  case 5:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: oedit [item_num] wear <location[s]>\r\n"
                   "$3Current$R: ",
                   this);

      sendln(QFlagsToStrings<ObjectPositions>((dc_->obj_index[rnum].item)->obj_flags.wear_flags));
      sendln("$3Valid types$R:");
      for (i = {}; i < QFlagsToStrings<ObjectPositions>().size(); i++)
      {
        send(u"  %1\r\n"_s.arg(QFlagsToStrings<ObjectPositions>().value(i)));
      }
      return ReturnValue::eFAILURE;
    }

    (dc_->obj_index[rnum].item)->obj_flags.wear_flags = parse_bitstrings<ObjectPositions>(buf4, this, (dc_->obj_index[rnum].item)->obj_flags.wear_flags);
  }
  break;

    /* edit size */
  case 6:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: oedit [item_num] size <size[s]>\r\n"
                   "$3Current$R: ",
                   this);
      sprintbit((dc_->obj_index[rnum].item)->obj_flags.size,
                size_bitfields, buf);
      send(buf);
      sendln("\r\n$3Valid types$R:");
      for (i = {}; *size_bitfields[i] != '\n'; i++)
      {
        dc_sprintf(buf, "  %s\r\n", size_bitfields[i]);
        send(buf);
      }
      return ReturnValue::eFAILURE;
    }
    parse_bitstrings_into_int(size_bitfields, buf4, this,
                              (dc_->obj_index[rnum].item)->obj_flags.size);
  }
  break;

    /* edit extra */
  case 7:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: oedit [item_num] extra <bit[s]>\r\n"
                   "$3Current$R: ",
                   this);
      sprintbit((dc_->obj_index[rnum].item)->obj_flags.extra_flags, Object::extra_bits, buf);
      send(buf);
      sendln("\r\n$3Valid types$R:");
      for (i = {}; i < Object::extra_bits.size(); i++)
      {
        send(u"  %1\r\n"_s.arg(Object::extra_bits[i]));
      }
      return ReturnValue::eFAILURE;
    }
    parse_bitstrings_into_int(Object::extra_bits, QString(buf4), this, (dc_->obj_index[rnum].item)->obj_flags.extra_flags);
  }
  break;

    /* edit weight */
  case 8:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [item_num] weight <>");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 99999))
    {
      sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->obj_flags.weight = intval;
    dc_sprintf(buf, "Item weight set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit value */
  case 9:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [item_num] value <>");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 5000000))
    {
      sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->obj_flags.cost = intval;
    dc_sprintf(buf, "Item value set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit moreflags */
  case 10:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: oedit [item_num] moreflags <bit[s]>\r\n"
                   "$3Current$R: ",
                   this);
      sprintbit((dc_->obj_index[rnum].item)->obj_flags.more_flags, Object::more_obj_bits, buf);
      send(buf);
      sendln("\r\n$3Valid types$R:");
      for (i = {}; i < Object::more_obj_bits.size(); i++)
      {
        send(u"  %1\r\n"_s.arg(Object::more_obj_bits[i]));
      }
      return ReturnValue::eFAILURE;
    }
    parse_bitstrings_into_int(Object::more_obj_bits, QString(buf4), this, (dc_->obj_index[rnum].item)->obj_flags.more_flags);
  }
  break;

    /* edit level */
  case 11:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [vnum] level <>");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 110))
    {
      sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->obj_flags.eq_level = intval;
    dc_sprintf(buf, "Item minimum level set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit 1value */
  case 12:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [vnum] 1value <num>");
      return ReturnValue::eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      sendln("Please specifiy a valid number.");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->obj_flags.value[0] = intval;
    dc_sprintf(buf, "Item value 1 set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit 2value */
  case 13:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [vnum] 2value <num>");
      return ReturnValue::eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      sendln("Please specifiy a valid number.");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->obj_flags.value[1] = intval;
    dc_sprintf(buf, "Item value 2 set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit 3value */
  case 14:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [vnum] 3value <num>");
      return ReturnValue::eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      sendln("Please specifiy a valid number.");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->obj_flags.value[2] = intval;
    dc_sprintf(buf, "Item value 3 set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit 4value */
  case 15:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [vnum] 4value <num>");
      return ReturnValue::eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      sendln("Please specifiy a valid number.");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->obj_flags.value[3] = intval;
    dc_sprintf(buf, "Item value 4 set to %d.\r\n", intval);
    send(buf);
  }
  break;

  /* affects */
  case 16:
  {
    return oedit_affects(this, rnum, buf4);
    break;
  }

  // exdesc
  case 17:
  {
    return oedit_exdesc(this, rnum, buf4);
    break;
  }

  // new
  case 18:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit new [vnum]");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 35000))
    {
      sendln("Please specifiy a valid number.");
      return ReturnValue::eFAILURE;
    }
    /*        if (real_object(intval) <= 0)
            {
        sendln("Object already exists.");
        return ReturnValue::eFAILURE;
      }
         */
    /*if (!has_skill( COMMAND_RANGE))
    {
      sendln("You cannot create items.");
      return ReturnValue::eFAILURE;
    }*/
    if (!can_modify_object(this, intval))
    {
      sendln("You cannot create items in that range.");
      return ReturnValue::eFAILURE;
    }
    /*
            if(!can_modify_object(this, intval)) {
              sendln("You are unable to work creation outside of your range.");
              return ReturnValue::eFAILURE;
            }
    */
    auto x = getDC()->create_blank_item(intval);
    if (!x.has_value())
    {
      send(u"Could not create item '%1'.  Max index hit or obj already exists. %2\r\n"_s.arg(intval).arg(QVariant::fromValue(x.error()).toString()));
      return ReturnValue::eFAILURE;
    }
    send(u"Item '%1' created successfully.\r\n"_s.arg(intval));
    break;
  }

  // delete
  case 19:
  {
    if (buf.isEmpty() 4 || strncmp(buf4, "yesiwanttodeletethisitem", 24))
    {
      sendln("$3Syntax$R: oedit [item_num] yesiwanttodeletethisitem") = {};
      sendln("\r\nDeleting an item is $3permanent$R and will cause ALL copies of");
      sendln("that items in the world to disappear.  Logged out players will lose the");
      sendln("item upon logging in as long as no other items is created with that number.");
      sendln("(Creating a new items with that number will cause the others to remain on");
      sendln("the player.)");
      return ReturnValue::eFAILURE;
    }

    VaultPtr vault, *tvault;
    vault_items_data *items, *titems;
    ObjectPtr obj;
    qint32 num = 0, real_num = {};

    for (vault = vault_table; vault; vault = tvault, num++)
    {
      tvault = vault->next;

      if (vault && vault->items)
      {
        for (items = vault->items; items; items = titems)
        {
          titems = items->next;

          real_num = real_object(items->item_vnum);
          obj = items->obj ? items->obj : (dc_->obj_index[real_num].item);
          if (obj == nullptr)
            continue;

          if (obj->item_number == rnum)
          {

            void item_remove(ObjectPtr obj, VaultPtr vault);
            item_remove(obj, vault);
            // items->obj = {};
            dc_->logf(0, DC::LogChannel::LOG_MISC, "Removing deleted item %d from %s's vault.", vnum, qPrintable(vault->owner));
          }
        }
      }
    }

    ObjectPtr next_k;
    // remove the item from players in world
    for (ObjectPtr k = dc_->object_list; k; k = next_k)
    {
      next_k = k->next;
      if (k->item_number == rnum)
        extract_obj(k);
    }

    // remove the item from index
    delete_item_from_index(rnum);
    sendln("Item deleted.");
    break;
  }

  // stat
  case 20:
  {
    obj_stat(this, dc_->obj_index[rnum].item);
    return ReturnValue::eSUCCESS;
    break;
  }
  case 21:
  {
    if (buf.isEmpty() 4)
    {
      sendln("$3Syntax$R: oedit [item_num] 3value <num>");
      return ReturnValue::eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      sendln("Please specifiy a valid number.");
      return ReturnValue::eFAILURE;
    }
    (dc_->obj_index[rnum].item)->obj_flags.timer = intval;
    dc_sprintf(buf, "Item timer to %d.\r\n", intval);
    send(buf);
  }
  break;
  case 22:
    extra_descr_data *curr;
    for (curr = (dc_->obj_index[rnum].item)->ex_description; curr; curr = curr->next)
      if (!str_cmp(curr->keyword, qPrintable((dc_->obj_index[rnum].item)->name())))
        break;
    if (!curr)
    { // None existing;
      curr = (extra_descr_data *)calloc(1, sizeof(extra_descr_data));
      curr->keyword = (qPrintable((dc_->obj_index[rnum].item)->name()));
      curr->description = u""_s;
      curr->next = (dc_->obj_index[rnum].item)->ex_description;
      (dc_->obj_index[rnum].item)->ex_description = curr;
    }
    sendln("Write your object's description. End with /s.");
    desc->connected = Connection::states::EDITING;
    desc->strnew = &(curr->description);
    break;
  default:
    sendln("Illegal value, tell a coder.");
    break;
  }

  dc_->set_zone_modified_obj(rnum);
  return ReturnValue::eSUCCESS;
}

void update_mobprog_bits(qint32 mob_num)
{
  mob_prog_data *prog = dc_->mob_index[mob_num].mobprogs;
  dc_->mob_index[mob_num].progtypes = {};

  while (prog)
  {
    SET_BIT(dc_->mob_index[mob_num].progtypes, prog->type);
    prog = prog->next;
  }
}

command_return_t do_procedit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf = {};
  QString buf2 = {};
  QString buf3 = {};
  QString buf4 = {};
  qint32 mob_num = -1;
  qint32 intval = {};
  qint32 x{}, i = {};
  mob_prog_data *prog = {};
  mob_prog_data *currprog = {};

  void mpstat(CharacterPtr ch, CharacterPtr victim);

  const QStringList fields = {"add", "remove", "type", "arglist", "command", "list", "\n"};

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  half_chop(argument, buf, buf2);
  half_chop(buf2, buf3, buf4);

  // at this point, buf  = mob_num
  //                buf3 = field
  //                buf4 = args

  // or

  // buf = field
  // buf3 = args[0]
  // buf4 = args[1-+]
  if (buf.isEmpty())
  {
    send_to_char("$3Syntax$R:  procedit [mob_vnum] [field] [arg]\r\n"
                 "  Edit a field with no args for help on that field.\r\n\r\n"
                 "  The field must be one of the following:\r\n",
                 ch);
    ch->display_string_list(fields);
    dc_sprintf(buf2, "\r\n$3Current mob vnum set to$R: %d\r\n", ch->player->last_mob_edit);
    send_to_char(buf2, ch);
    return ReturnValue::eFAILURE;
  }

  qint32 mobvnum = -1;
  if (isdigit(*buf))
  {
    mobvnum = atoi(buf);
    if (((mob_num = real_mobile(mobvnum)) < 0) || (mobvnum == 0 && *buf != '0'))
    {
      ch->send(fmt::format("{} is an invalid mob vnum.\r\n", mobvnum));
      return ReturnValue::eSUCCESS;
    }
    ch->setPlayerLastMob(mobvnum);
  }
  else
  {
    mobvnum = ch->player->last_mob_edit;
    mob_num = real_mobile(mobvnum);

    if (mob_num < 0 || mobvnum <= 0)
    {
      ch->send(fmt::format("Last mob vnum {} is invalid.\r\n", mobvnum));
      return ReturnValue::eFAILURE;
    }

    // put the buffs where they should be
    dc_sprintf(buf2, "%s %s", buf3, buf4);
    dc_strcpy(buf4, buf2);
    dc_strcpy(buf3, buf);
  }

  // a this point, mob_num is the index
  if (mobvnum == -1)
    mobvnum = dc_->mob_index[mob_num].vnum();

  if (!can_modify_mobile(ch, mobvnum))
  {
    ch->sendln("You are unable to work creation outside of your range.");
    return ReturnValue::eFAILURE;
  }

  ch->setPlayerLastMob(mobvnum);

  // no field
  if (buf.isEmpty() 3)
  {
    mpstat(ch, (CharacterPtr)dc_->mob_index[mob_num].item);
    return ReturnValue::eSUCCESS;
  }

  for (x = {};; x++)
  {
    if (fields[x][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return ReturnValue::eFAILURE;
    }
    if (is_abbrev(buf3, fields[x]))
      break;
  }

  switch (x)
  {

  /* add */
  case 0:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] add new\r\n"
                   "This creates a new mob prog and tacks it on the end.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    prog = new mob_prog_data;
    prog->type = GREET_PROG;
    prog->arglist = u"80"_s;
    prog->comlist = u"say This is my new mob prog!\r\n"_s;
    prog->next = {};

    qint32 prog_num = 1;
    if ((currprog = dc_->mob_index[mob_num].mobprogs))
    {
      while (currprog->next)
      {
        prog_num++;
        currprog = currprog->next;
      }
      currprog->next = prog;
      prog_num++;
    }
    else
    {
      dc_->mob_index[mob_num].mobprogs = prog;
    }

    update_mobprog_bits(mob_num);

    ch->send(u"New mobprog created as #%1.\r\n"_s.arg(prog_num));
  }
  break;

    /* remove */
  case 1:
  {
    if (buf.isEmpty() 4)
    {
      ch->sendln("$3Syntax$R: procedit [mob_num] remove <prog>");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 999))
    {
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }
    // find program number "intval"
    prog = {};
    for (i = 1, currprog = dc_->mob_index[mob_num].mobprogs; currprog && i != intval; i++, prog = currprog, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }

    if (prog)
      prog->next = currprog->next;
    else
      dc_->mob_index[mob_num].mobprogs = currprog->next;

    currprog->type = {};
    currprog->arglist = {};
    currprog->comlist = {};
    currprog = {};

    update_mobprog_bits(mob_num);

    ch->send(fmt::format("Program {} deleted from mob vnum {}.\r\n", intval, mobvnum));
  }
  break;

    /* type */
  case 2:
  {
    half_chop(buf4, buf2, buf3);
    if (buf.isEmpty() 2 || buf.isEmpty() 3)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] type <prog> <newtype>\r\n"
                   "$3Valid types$R:\r\n"
                   "  1 -       act_prog\r\n"
                   "  2 -    speech_prog\r\n"
                   "  3 -      rand_prog\r\n"
                   "  4 -     fight_prog\r\n"
                   "  5 -     death_prog\r\n"
                   "  6 -  hitprcnt_prog\r\n"
                   "  7 -     entry_prog\r\n"
                   "  8 -     greet_prog\r\n"
                   "  9 - all_greet_prog\r\n"
                   " 10 -      give_prog\r\n"
                   " 11 -     bribe_prog\r\n"
                   " 12 -     catch_prog\r\n"
                   " 13 -    attack_prog\r\n"
                   " 14 -     arand_prog\r\n"
                   " 15 -      load_prog\r\n"
                   " 16 -      can_see_prog\r\n"
                   " 17 -      damage_prog\r\n"

                   ,
                   ch);

      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf2, 1, 999))
    {
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }
    // find program number "intval"
    for (i = 1, currprog = dc_->mob_index[mob_num].mobprogs; currprog && i != intval; i++, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }

    if (!check_range_valid_and_convert(intval, buf3, 1, 17))
    {
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }

    switch (intval)
    {
    case 1:
      currprog->type = ACT_PROG;
      break;
    case 2:
      currprog->type = SPEECH_PROG;
      break;
    case 3:
      currprog->type = RAND_PROG;
      break;
    case 4:
      currprog->type = FIGHT_PROG;
      break;
    case 5:
      currprog->type = DEATH_PROG;
      break;
    case 6:
      currprog->type = HITPRCNT_PROG;
      break;
    case 7:
      currprog->type = ENTRY_PROG;
      break;
    case 8:
      currprog->type = GREET_PROG;
      break;
    case 9:
      currprog->type = ALL_GREET_PROG;
      break;
    case 10:
      currprog->type = GIVE_PROG;
      break;
    case 11:
      currprog->type = BRIBE_PROG;
      break;
    case 12:
      currprog->type = CATCH_PROG;
      break;
    case 13:
      currprog->type = ATTACK_PROG;
      break;
    case 14:
      currprog->type = ARAND_PROG;
      break;
    case 15:
      currprog->type = LOAD_PROG;
      break;
    case 16:
      currprog->type = CAN_SEE_PROG;
      break;
    case 17:
      currprog->type = DAMAGE_PROG;
      break;
    }

    update_mobprog_bits(mob_num);

    ch->sendln("Mob program type changed.");
  }
  break;

    /* arglist */
  case 3:
  {
    half_chop(buf4, buf2, buf3);
    if (buf.isEmpty() 2 || buf.isEmpty() 3)
    {
      ch->sendln("$3Syntax$R: procedit [mob_num] arglist <prog> <new arglist>");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf2, 1, 999))
    {
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }
    // find program number "intval"
    for (i = 1, currprog = dc_->mob_index[mob_num].mobprogs; currprog && i != intval; i++, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }

    currprog->arglist = {};
    currprog->arglist = strdup(buf3);

    ch->sendln("Mob program arglist changed.");
  }
  break;

    /* command */
  case 4:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] command <prog>\r\n"
                   "This will put you into the editor which will replace the current\r\n"
                   "command for program number <prog>.\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 999))
    {
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }
    // find program number "intval"
    for (i = 1, currprog = dc_->mob_index[mob_num].mobprogs; currprog && i != intval; i++, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      ch->sendln("Invalid prog number.");
      return ReturnValue::eFAILURE;
    }

    ch->desc->backstr = {};
    ch->desc->strnew = &(currprog->comlist);

    if (isSet(ch->player->toggles, Player::PLR_EDITOR_WEB))
    {
      ch->desc->web_connected = Connection::states::EDIT_MPROG;
    }
    else
    {
      ch->desc->connected = Connection::states::EDIT_MPROG;

      send_to_char("        Write your help entry and stay within the line.  (/s saves /h for help)\r\n"
                   "   |--------------------------------------------------------------------------------|\r\n",
                   ch);

      if (currprog->comlist)
      {
        ch->desc->backstr = (currprog->comlist);
        if (ch->desc->backstr != nullptr)
        {
          ch->send(double_dollars(QString(ch->desc->backstr)));
        }
      }
    }
  }
  break;

    // list
  case 5:
    mpstat(ch, (CharacterPtr)dc_->mob_index[mob_num].item);
    return ReturnValue::eFAILURE;
  }
  dc_->set_zone_modified_mob(mob_num);
  return ReturnValue::eSUCCESS;
}

command_return_t do_mscore(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  qint32 mob_num = -1;

  void boro_mob_stat(CharacterPtr ch, CharacterPtr k);

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  one_argument(argument, buf);

  if (buf.isEmpty())
  {
    ch->sendln("$3Syntax$R:  mscore <mob_num>");
    return ReturnValue::eFAILURE;
  }

  qint64 mob_vnum = atoi(buf); // there is no mob 0, so this is okay.  Bad 0's get caught in real_mobile
  if (((mob_num = real_mobile(mob_vnum)) < 0))
  {
    ch->send(fmt::format("{} is an invalid mob vnum.\r\n", mob_vnum));
    return ReturnValue::eSUCCESS;
  }

  boro_mob_stat(ch, (CharacterPtr)dc_->mob_index[mob_num].item);
  return ReturnValue::eSUCCESS;
}

command_return_t do_medit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (!ch || !argument)
  {
    return ReturnValue::eFAILURE;
  }
  QString buf = {};
  QString buf2 = {};
  QString buf3 = {};
  QString buf4 = {};
  vnum_t mob_num = {};
  qint32 intval = {};
  qint32 x = {}, i = {};

  const QStringList fields = {"keywords", "shortdesc", "longdesc", "description",
                              "sex", "class", "race", "level", "alignment", "loadposition",
                              "defaultposition", "actflags", "affectflags", "numdamdice",
                              "sizedamdice", "damroll", "hitroll", "hphitpoints", "gold",
                              "experiencepoints", "immune", "suscept", "resist", "armorclass",
                              "stat", "strength", "dexterity", "intelligence", "wisdom",
                              "constitution", "new", "delete", "type", "v1", "v2", "v3", "v4",
                              "\n"};

  if (!ch->isPlayer())
    return ReturnValue::eFAILURE;

  half_chop(argument, buf, buf2);
  half_chop(buf2, buf3, buf4);

  // at this point, buf  = mob_num
  //                buf3 = field
  //                buf4 = args

  // or

  // buf = field
  // buf3 = args[0]
  // buf4 = args[1-+]

  if (buf.isEmpty())
  {
    send_to_char("$3Syntax$R:  medit [mob_num] [field] [arg]\r\n"
                 "  Edit a mob_num with no field or arg to view the item.\r\n"
                 "  Edit a field with no args for help on that field.\r\n\r\n"
                 "The field must be one of the following:\r\n",
                 ch);
    ch->display_string_list(fields);
    return ReturnValue::eFAILURE;
  }

  vnum_t mobvnum = {};
  if (is_number(buf))
  {
    mob_num = atoll(buf); // there is no mob 0, so this is okay.  Bad 0's get caught in real_mobile
    mobvnum = mob_num;
    mob_num = real_mobile(mobvnum);
    if (mobvnum == DC::INVALID_VNUM || mob_num == DC::INVALID_VNUM)
    {
      ch->send(fmt::format("{} is an invalid mob vnum.\r\n", buf));
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    mobvnum = ch->player->last_mob_edit;
    if (((mob_num = real_mobile(mobvnum)) < 0 && dc_strcmp(buf, "new")))
    {
      ch->send(fmt::format("{} is an invalid mob vnum.\r\n", mobvnum));
      return ReturnValue::eFAILURE;
    }
    // put the buffs where they should be
    if (*buf4)
      dc_sprintf(buf2, "%s %s", buf3, buf4);
    else
      dc_strcpy(buf2, buf3);

    dc_strcpy(buf4, buf2);
    dc_strcpy(buf3, buf);
  }
  ch->setPlayerLastMob(mobvnum);

  if (buf.isEmpty() 3) // no field.  Stat the item.
  {
    return mob_stat(ch, (CharacterPtr)dc_->mob_index[mob_num].item);
  }

  if (mobvnum == -1)
    mobvnum = dc_->mob_index[mob_num].vnum();
  // MOVED
  for (x = {};; x++)
  {
    if (fields[x][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return ReturnValue::eFAILURE;
    }
    if (is_abbrev(buf3, fields[x]))
      break;
  }

  // a this point, mob_num is the index

  if (x != 30) // Checked in there.
    if (!can_modify_mobile(ch, mobvnum))
    {
      ch->sendln("You are unable to work creation outside of your range.");
      return ReturnValue::eFAILURE;
    }

  auto mob = (CharacterPtr)dc_->mob_index[mob_num].item;
  switch (x)
  {

  /* edit keywords */
  case 0:
  {
    if (buf.isEmpty() 4)
    {
      ch->sendln("$3Syntax$R: medit [mob_num] keywords <new_keywords>");
      return ReturnValue::eFAILURE;
    }
    mob->name(buf4);
    dc_sprintf(buf, "Mob keywords set to '%s'.\r\n", buf4);
    ch->send(buf);
  }
  break;

    /* edit short desc */
  case 1:
  {
    if (buf.isEmpty() 4)
    {
      ch->sendln("$3Syntax$R: medit [mob_num] shortdesc <desc>");
      return ReturnValue::eFAILURE;
    }
    mob->short_desc = buf4;
    dc_sprintf(buf, "Mob shortdesc set to '%s'.\r\n", buf4);
    ch->send(buf);
  }
  break;

    // edit long desc
  case 2:
  {
    if (buf.isEmpty() 4)
    {
      ch->sendln("$3Syntax$R: medit [mob_num] longdesc <desc>");
      return ReturnValue::eFAILURE;
    }
    dc_strcat(buf4, "\r\n");
    mob->long_desc = buf4;
    dc_sprintf(buf, "Mob longdesc set to '%s'.\r\n", buf4);
    ch->send(buf);
  }
  break;

    /* edit description */
  case 3:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] description <anything>\r\n"
          "This will put you into the editor which will replace the\r\n"
          "current description.\r\n",
          ch);
      return ReturnValue::eFAILURE;
    }
    send_to_char("Enter the mob's description below."
                 " Terminate with '/s' on a new line.\r\n\r\n",
                 ch);
    // TODO - this causes a memory leak if you edit the desc twice (first one is hsh'd)
    //        mob->description = {};
    ch->desc->connected = Connection::states::EDITING;
    mob->description = u""_s;
    ch->desc->strnew = &(mob->description);
  }
  break;

    /* edit sex */
  case 4:
  {
    if (buf.isEmpty() 4)
    {
      ch->sendln("$3Syntax$R: medit [mob_num] sex <male|female|neutral>");
      return ReturnValue::eFAILURE;
    }
    if (is_abbrev(buf4, "male"))
    {
      mob->sex = SEX_MALE;
      ch->sendln("Mob sex set to male.");
    }
    else if (is_abbrev(buf4, "female"))
    {
      mob->sex = SEX_FEMALE;
      ch->sendln("Mob sex set to female.");
    }
    else if (is_abbrev(buf4, "neutral"))
    {
      mob->sex = SEX_NEUTRAL;
      ch->sendln("Mob sex set to neutral.");
    }
    else
      ch->sendln("Invalid sex.  Chose 'male', 'female', or 'neutral'.");
  }
  break;

    /* edit class */
  case 5:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] class <class>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%s\n",
                 pc_clss_types[mob->c_class]);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = {}; *pc_clss_types[i] != '\n'; i++)
      {
        dc_sprintf(buf, "  %d) %s\r\n", i, pc_clss_types[i]);
        ch->send(buf);
      }
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, CLASS_MAX))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->c_class = intval;
    dc_sprintf(buf, "Mob class set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit race */
  case 6:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] race <racetype>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%s\r\n\r\n"
                      "Available types:\r\n",
                 races[mob->race].singular_name);
      ch->send(buf);
      for (i = {}; i <= MAX_RACE; i++)
        ch->send(u"  %s\r\n"_s.arg(races[i].singular_name));
      ch->sendln("");
      return ReturnValue::eFAILURE;
    }
    qint32 race_set = {};
    for (i = {}; i <= MAX_RACE; i++)
    {
      if (is_abbrev(buf4, races[i].singular_name))
      {
        ch->send(u"Mob race set to %s.\r\n"_s.arg(races[i].singular_name));
        mob->race = i;
        race_set = 1;

        mob->raw_str = mob->str = BASE_STAT + mob_race_mod[mob->race][0];
        mob->raw_dex = mob->dex = BASE_STAT + mob_race_mod[mob->race][1];
        mob->raw_con = mob->con = BASE_STAT + mob_race_mod[mob->race][2];
        mob->raw_intel = mob->intel = BASE_STAT + mob_race_mod[mob->race][3];
        mob->raw_wis = mob->wis = BASE_STAT + mob_race_mod[mob->race][4];
      }
    }
    if (!race_set)
    {
      ch->send(u"Could not find race '%s'.\r\n"_s.arg(buf4));
      return ReturnValue::eFAILURE;
    }
  }
  break;

    /* edit level */
  case 7:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] level <levelnum>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->getLevel());
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 110))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->setLevel(intval);
    dc_sprintf(buf, "Mob level set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit alignment */
  case 8:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] alignment <alignnum>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->alignment);
      ch->send(buf);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, -1000, 1000))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->alignment = intval;
    dc_sprintf(buf, "Mob alignment set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit load position */
  case 9:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] loadposition <position>\r\n"
          "$3Current$R: ",
          ch);
      ch->sendln(u"%1"_s.arg(mob->getPositionQString()));
      send_to_char("$3Valid positions$R:\r\n"
                   "  1 = Standing\r\n"
                   "  2 = Sitting\r\n"
                   "  3 = Resting\r\n"
                   "  4 = Sleeping\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 4))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }

    auto victim = mob;
    switch (intval)
    {
    case 1:
      victim->setStanding();
      break;
    case 2:
      victim->setSitting();
      break;
    case 3:
      victim->setResting();
      break;
    case 4:
      victim->setSleeping();
      break;
    }

    ch->sendln(u"Mob default position set to %1."_s.arg(victim->getPositionQString()));
  }
  break;

    /* edit default position */
  case 10:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] defaultposition <position>\r\n"
          "$3Current$R: ",
          ch);
      ch->sendln(u"%1"_s.arg(Character::position_to_string(mob->mobdata->default_pos)));

      send_to_char("$3Valid positions$R:\r\n"
                   "  1 = Standing\r\n"
                   "  2 = Sitting\r\n"
                   "  3 = Resting\r\n"
                   "  4 = Sleeping\r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 4))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }

    auto victim = mob;
    auto mobdata = victim->mobdata;
    switch (intval)
    {
    case 1:
      mobdata->default_pos = position_t::STANDING;
      break;
    case 2:
      mobdata->default_pos = position_t::SITTING;
      break;
    case 3:
      mobdata->default_pos = position_t::RESTING;
      break;
    case 4:
      mobdata->default_pos = position_t::SLEEPING;
      break;
    }
    dc_snprintf(buf, sizeof(buf), "Mob default position set to %s.\r\n", Character::position_to_string(qPrintable(mobdata->default_pos)));
    ch->send(buf);
  }
  break;

    /* edit actflags */
  case 11:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] actflags <location[s]>\r\n"
          "$3Current$R: ",
          ch);
      sprintbit(
          mob->mobdata->actflags,
          action_bits, buf);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = {}; *action_bits[i] != '\n'; i++)
      {
        dc_sprintf(buf, "  %s\r\n", action_bits[i]);
        ch->send(buf);
      }
      return ReturnValue::eFAILURE;
    }
    quint32 old_actflags[2];
    old_actflags[0] = mob->mobdata->actflags[0];
    old_actflags[1] = mob->mobdata->actflags[1];
    parse_bitstrings_into_int(action_bits, buf4, ch, mob->mobdata->actflags);

    quint32 new_actflags[2];
    new_actflags[0] = mob->mobdata->actflags[0];
    new_actflags[1] = mob->mobdata->actflags[1];

    if (old_actflags[0] != new_actflags[0] || old_actflags[1] != new_actflags[1])
    {
      quint64 NPCs_changed = {};
      for (auto const &c : dc_->character_list)
      {
        if (c->isNonPlayer() && c->mobdata && dc_->mob_index[c->mobdata->nr].vnum() == mobvnum)
        {
          c->mobdata->actflags[0] = new_actflags[0];
          c->mobdata->actflags[1] = new_actflags[1];
          NPCs_changed++;
        }
      }
      ch->send(u"%1 NPCs in the world have been updated.\r\n"_s.arg(NPCs_changed));
    }
  }
  break;

    /* edit affectflags */
  case 12:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] affectflags <location[s]>\r\n"
          "$3Current$R: ",
          ch);
      sprintbit(mob->affected_by[0],
                affected_bits, buf);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = {}; *affected_bits[i] != '\n'; i++)
      {
        dc_sprintf(buf, "  %s\r\n", affected_bits[i]);
        ch->send(buf);
      }
      return ReturnValue::eFAILURE;
    }
    parse_bitstrings_into_int(affected_bits, buf4, ch,
                              &(mob->affected_by[0]));
    //                             &((mob->affected_by[0])));
  }
  break;

    /* edit numdamdice */
  case 13:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] numdamdice <amount>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->mobdata->damnodice);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 400");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 400))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->mobdata->damnodice = intval;
    dc_sprintf(buf, "Mob number dice for damage set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit sizedamdice */
  case 14:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] sizedamdice <amount>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->mobdata->damsizedice);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 400");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 400))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->mobdata->damsizedice = intval;
    dc_sprintf(buf, "Mob size dice for damage set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit damroll */
  case 15:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] damroll <damrollnum>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->damroll);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: -50 to 400");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, -50, 400))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->damroll = intval;
    dc_sprintf(buf, "Mob damroll set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit hitroll */
  case 16:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] hitroll <levelnum>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->hitroll);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: -50 to 100");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, -50, 300))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->hitroll = intval;
    dc_sprintf(buf, "Mob hitroll set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit hphitpoints */
  case 17:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] hphitpoints <hp>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->raw_hit);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 64000");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 64000))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->raw_hit = intval;
    mob->max_hit = intval;
    dc_sprintf(buf, "Mob hitpoints set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit gold */
  case 18:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] gold <goldamount>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%lu\n",
                 mob->getGold());
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 0 to 10000000");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 10000000))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    if (intval > 250000 && ch->getLevel() <= DEITY)
    {
      ch->sendln("104-'s can only set a mob to 250k gold.  If you need more ask someone.");
      return ReturnValue::eFAILURE;
    }
    mob->setGold(intval);
    dc_sprintf(buf, "Mob gold set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit experiencepoints */
  case 19:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] experiencepoints <xpamount>\r\n"
          "$3Current$R: ",
          ch);
      dc_sprintf(buf, "%d\n",
                 (qint32)mob->exp);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 0 to 20000000");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 20000000))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->exp = intval;
    dc_sprintf(buf, "Mob experience set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit immune */
  case 20:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] immune <location[s]>\r\n"
                   "$3Current$R: ",
                   ch);
      sprintbit(mob->immune, isr_bits,
                buf);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = {}; *isr_bits[i] != '\n'; i++)
      {
        dc_sprintf(buf, "  %s\r\n", isr_bits[i]);
        ch->send(buf);
      }
      return ReturnValue::eFAILURE;
    }
    parse_bitstrings_into_int(isr_bits, buf4, ch,
                              mob->immune);
  }
  break;

    /* edit suscept */
  case 21:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] suscept <location[s]>\r\n"
                   "$3Current$R: ",
                   ch);
      sprintbit(mob->suscept,
                isr_bits, buf);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = {}; *isr_bits[i] != '\n'; i++)
      {
        dc_sprintf(buf, "  %s\r\n", isr_bits[i]);
        ch->send(buf);
      }
      return ReturnValue::eFAILURE;
    }
    parse_bitstrings_into_int(isr_bits, buf4, ch,
                              mob->suscept);
  }
  break;

    /* edit resist */
  case 22:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] resist <location[s]>\r\n"
                   "$3Current$R: ",
                   ch);
      sprintbit(mob->resist, isr_bits,
                buf);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = {}; *isr_bits[i] != '\n'; i++)
      {
        dc_sprintf(buf, "  %s\r\n", isr_bits[i]);
        ch->send(buf);
      }
      return ReturnValue::eFAILURE;
    }
    parse_bitstrings_into_int(isr_bits, buf4, ch,
                              mob->resist);
  }
  break;

    // armorclass
  case 23:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] armorclass <ac>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->armor);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 100 to $B-$R2000");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, -2000, 100))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->armor = intval;
    dc_sprintf(buf, "Mob armorclass(ac) set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    // stat
  case 24:
  {
    return mob_stat(ch, (CharacterPtr)dc_->mob_index[mob_num].item);
    break;
  }
    // strength
  case 25:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] strength <str>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->raw_str);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 28");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 28))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->raw_str = intval;
    dc_sprintf(buf, "Mob raw strength set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    // dexterity
  case 26:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] dexterity <dex>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->raw_dex);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 28");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 28))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->raw_dex = intval;
    dc_sprintf(buf, "Mob raw dexterity set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    // intelligence
  case 27:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] intelligence <qint32>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->raw_intel);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 28");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 28))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->raw_intel = intval;
    dc_sprintf(buf, "Mob raw intelligence set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    // wisdom
  case 28:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] wisdom <wis>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->raw_wis);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 28");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 28))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->raw_wis = intval;
    dc_sprintf(buf, "Mob raw wisdom set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    // constitution
  case 29:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] constitution <con>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%d\n",
                 mob->raw_con);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 28");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 28))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->raw_con = intval;
    dc_sprintf(buf, "Mob raw constituion set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    // New
  case 30:
  {
    if (buf.isEmpty() 4)
    {
      ch->sendln("$3Syntax$R: medit new [number]");
      return ReturnValue::eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 35000))
    {
      ch->sendln("Please specifiy a valid number.");
      return ReturnValue::eFAILURE;
    }
    if (!can_modify_mobile(ch, intval))
    {
      ch->sendln("You cannot create mobiles in that range.");
      return ReturnValue::eFAILURE;
    }
    mob_num = intval;
    x = ch->getDC()->create_blank_mobile(intval);
    if (x < 0)
    {
      ch->send(u"Could not create mobile '%d'.  Max index hit or mob already exists.\r\n"_s.arg(intval));
      return ReturnValue::eFAILURE;
    }
    ch->send(u"Mobile '%1' created successfully.\r\n"_s.arg(intval));
    ch->setPlayerLastMob(intval);
  }
  break;
  case 31:
  {
    if (buf.isEmpty() 4 || strncmp(buf4, "yesiwanttodeletethismob", 23))
    {
      ch->sendln("$3Syntax$R: medit [mob_number] yesiwanttodeletethismob") = {};
      return ReturnValue::eFAILURE;
    }
    const auto &character_list = dc_->character_list;
    for (const auto &v : character_list)
    {
      if (v->isNonPlayer() && v->mobdata->nr == mob_num)
        extract_char(v, true);
    }
    delete_mob_from_index(mob_num);
    ch->sendln("Mobile deleted.");
  }
  break;
  case 32:
  {
    if (buf.isEmpty() 4)
    {
      send_to_char("$3Syntax$R: medit [mob_vnum] type <type id>\r\n"
                   "$3Current$R: ",
                   ch);
      dc_sprintf(buf, "%s\n",
                 mob_types[mob->mobdata->mob_flags.type]);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = {}; *mob_types[i] != '\n'; i++)
      {
        dc_sprintf(buf, "  %d) %s\r\n", i, mob_types[i]);
        ch->send(buf);
      }
      return ReturnValue::eFAILURE;
    }

    if (!check_range_valid_and_convert<decltype(intval)>(intval, buf4, MOB_TYPE_FIRST,
                                                         MOB_TYPE_LAST))
    {
      ch->sendln("Value out of valid range.");
      return ReturnValue::eFAILURE;
    }
    mob->mobdata->mob_flags.type =
        (mob_type_t)intval;
    dc_sprintf(buf, "Mob type set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    /* edit 1value */
  case 33:
  {
    if (buf.isEmpty() 4)
    {
      ch->sendln("$3Syntax$R: medit [mob_vnum] 1value <num>");
      return ReturnValue::eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      ch->sendln("Please specify a valid number.");
      return ReturnValue::eFAILURE;
    }
    mob->mobdata->mob_flags.value[0] = intval;
    dc_sprintf(buf, "Mob value 1 set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit 2value */
  case 34:
  {
    if (buf.isEmpty() 4)
    {
      ch->sendln("$3Syntax$R: medit [mob_vnum] 2value <num>");
      return ReturnValue::eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      ch->sendln("Please specify a valid number.");
      return ReturnValue::eFAILURE;
    }
    mob->mobdata->mob_flags.value[1] = intval;
    dc_sprintf(buf, "Mob value 2 set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit 3value */
  case 35:
  {
    if (buf.isEmpty() 4)
    {
      ch->sendln("$3Syntax$R: medit [mob_vnum] 3value <num>");
      return ReturnValue::eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      ch->sendln("Please specify a valid number.");
      return ReturnValue::eFAILURE;
    }
    mob->mobdata->mob_flags.value[2] = intval;
    dc_sprintf(buf, "Mob value 3 set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit 4value */
  case 36:
  {
    if (buf.isEmpty() 4)
    {
      ch->sendln("$3Syntax$R: medit [mob_vnum] 4value <num>");
      return ReturnValue::eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      ch->sendln("Please specify a valid number.");
      return ReturnValue::eFAILURE;
    }
    mob->mobdata->mob_flags.value[3] = intval;
    dc_sprintf(buf, "Mob value 4 set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
  }
  dc_->set_zone_modified_mob(mob_num);
  return ReturnValue::eSUCCESS;
}

command_return_t do_redit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf, remainder_args;
  qint32 x, a, b, c, d = {};
  extra_descr_data *extra;
  extra_descr_data *ext;

  const QStringList return_directions =
      {
          "south",
          "west",
          "north",
          "east",
          "down",
          "up",
          ""};

  qint32 reverse_number[] =
      {
          2, 3, 0, 1, 5, 4};
  QStringList fields =
      {
          "name",
          "description",
          "exit",
          "extra",
          "exdesc",
          "rflag",
          "sector",
          "denymob",
          ""};

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  QString arg1;
  std::tie(arg1, remainder_args) = half_chop(QString(argument));
  if (arg1.empty())
  {
    ch->sendln("The field must be one of the following:");
    for (x = {};; x++)
    {
      if (fields[x][0] == '\0')
        return ReturnValue::eFAILURE;
      ch->send(u"  %s\r\n"_s.arg(fields[x]));
    }
  }

  if (!can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You are unable to work creation outside of your range.");
    return ReturnValue::eFAILURE;
  }

  for (x = {};; x++)
  {
    if (fields[x][0] == '\0')
    {
      ch->sendln("The field must be one of the following:");
      for (x = {};; x++)
      {
        if (fields[x][0] == '\0')
          return ReturnValue::eFAILURE;
        ch->send(u"%s\r\n"_s.arg(fields[x]));
      }
    }
    if (is_abbrev(arg1.c_str(), fields[x]))
      break;
  }

  switch (x)
  {

    /* redit name */
  case 0:
  {
    if (remainder_args.empty())
    {
      ch->sendln("$3Syntax$R: redit name <Room Name>");
      return ReturnValue::eFAILURE;
    }
    dc_->world[ch->in_room].name = {};
    dc_->world[ch->in_room].name = (remainder_args.c_str());
    ch->sendln("Ok.");
  }
  break;

    /* redit description */
  case 1:
  {
    if (!remainder_args.empty())
    {
      QString description = remainder_args + "\r\n";
      dc_->world[ch->in_room].description = {};
      dc_->world[ch->in_room].description = (description.c_str());
      ch->sendln("Ok.");
      return ReturnValue::eFAILURE;
    }
    ch->sendln("        Write your room's description.  (/s saves /h for help)");
    ch->desc->connected = Connection::states::EDITING;
    ch->desc->strnew = &(dc_->world[ch->in_room].description);
  }
  break;

    // redit exit
  case 2:
  {
    if (remainder_args.empty())
    {
      ch->sendln(QStringLiteral(
          "$3Syntax$R: <> is required. [] is optional.\r\n"
          "redit exit <direction> <room vnum> [flagnumber] [keynumber] [keywords]\r\n"
          "redit exit delete <direction>\r\n"
          "\r\n"
          "Directions: north, n, south, s, east, e, west, w, up, u, down or d.\r\n"
          "Exit flags: (Add the numbers together for multiple flags)\r\n"
          "IS_DOOR    1\r\n"
          "CLOSED     2\r\n"
          "LOCKED     4\r\n"
          "HIDDEN     8\r\n"
          "IMM_ONLY  16\r\n"
          "PICKPROOF 32\r\n"
          "\r\n"
          "$3Examples$R:\r\n"
          "redit exit n 1200                    - North exit to room 1200.\r\n"
          "redit exit n 1200 1 -1 door wooden   - North door exit with keywords 'door' or 'wooden' to room 1200.\r\n"
          "redit exit n 1200 9 345 grate rusty  - North hidden door exit with keywords 'grate' or 'rusty' to room 1200 that requires key vnum 345.\r\n"));
      return ReturnValue::eFAILURE;
    }

    QString arg2;
    std::tie(arg2, remainder_args) = half_chop(remainder_args);
    if (arg2 == "delete")
    {
      QString direction;
      std::tie(direction, remainder_args) = half_chop(remainder_args);
      for (x = {}; x <= 6; x++)
      {
        if (is_abbrev(direction.c_str(), dirs[x]))
        {
          if (dc_->world[ch->in_room].dir_option[x] == nullptr)
          {
            ch->send(u"There is no %s exit.\r\n"_s.arg(dirs[x]));
            return ReturnValue::eFAILURE;
          }

          qint16 destination_room = dc_->world[ch->in_room].dir_option[x]->to_room;
          ch->send(u"Deleting %s exit from room %d to %d.\r\n"_s.arg(dirs[x]).arg(ch->in_room).arg(destination_room));
          free(dc_->world[ch->in_room].dir_option[x]);
          dc_->world[ch->in_room].dir_option[x] = {};

          if (ch->isPlayer() && !isSet(ch->player->toggles, Player::PLR_ONEWAY))
          {
            // if the destination room has a reverse exit
            if (dc_->world[destination_room].dir_option[reverse_number[x]])
            {
              // and that reverse exit points to us then we can delete it too
              if (dc_->world[destination_room].dir_option[reverse_number[x]]->to_room == ch->in_room)
              {
                ch->send(u"Deleting %s exit from room %d to %d.\r\n"_s.arg(dirs[reverse_number[x]]).arg(destination_room).arg(ch->in_room));
                free(dc_->world[destination_room].dir_option[reverse_number[x]]);
                dc_->world[destination_room].dir_option[reverse_number[x]] = {};
                return ReturnValue::eSUCCESS;
              }
              else
              {
                ch->sendln(u"Reverse %s exit in room %d does not point to room %d."_s.arg(dirs[reverse_number[x]]).arg(destination_room).arg(ch->in_room));
                return ReturnValue::eSUCCESS;
              }
            }
            else
            {
              ch->sendln(u"Reverse %s exit in room %d does not exist."_s.arg(dirs[reverse_number[x]]).arg(destination_room));
              return ReturnValue::eSUCCESS;
            }
          } // end of check if Player::PLR_ONEWAY is toggled
          return ReturnValue::eSUCCESS;
        } // end of is_abbred for dirs
      } // end of for loop through directions

      ch->sendln("Missing direction you want to delete.");
      return ReturnValue::eFAILURE;
    } // end of delete

    for (x = {}; x <= 6; x++)
    {
      if (x == 6)
      {
        ch->sendln("No such direction.");
        return ReturnValue::eFAILURE;
      }
      if (is_abbrev(arg2.c_str(), dirs[x]))
        break;
    }
    if (remainder_args.empty())
    {
      ch->send(u"Missing vnum of room you want to have %s exit connect to.\r\n"_s.arg(dirs[x]));
      return ReturnValue::eFAILURE;
    }

    QString arg3;
    std::tie(arg3, remainder_args) = half_chop(remainder_args);
    try
    {
      d = stoi(arg3);
    }
    catch (...)
    {
      d = {};
    }
    c = real_room(d);

    if (!remainder_args.empty())
    {
      QString arg4;
      std::tie(arg4, remainder_args) = half_chop(remainder_args);
      try
      {
        a = stoi(arg4);
      }
      catch (...)
      {
        a = {};
      }

      if (remainder_args.empty())
      {
        return ReturnValue::eFAILURE;
      }

      QString arg5;
      std::tie(arg5, remainder_args) = half_chop(remainder_args);
      try
      {
        b = stoi(arg5);
      }
      catch (...)
      {
        b = {};
      }

      if (b == 0)
      {
        ch->sendln("No key 0's allowed.  Changing to -1.");
        b = -1;
      }
    }
    else
    {
      a = {};
      b = -1;
    }

    if (c == (-1) && can_modify_room(ch, d))
    {
      if (dc_->create_one_room(ch, d))
      {
        c = real_room(d);
        ch->send(u"Creating room %1.\r\n"_s.arg(d));
      }
    }

    if (c == (-1))
    {
      ch->send(u"Error creating exit to room %1.\r\n"_s.arg(d));
      return ReturnValue::eFAILURE;
    }

    if (dc_->world[ch->in_room].dir_option[x])
      ch->sendln("Modifying exit.");
    else
    {
      ch->sendln("Creating new exit.");
      CREATE(dc_->world[ch->in_room].dir_option[x], room_direction_data, 1);
      dc_->world[ch->in_room].dir_option[x]->general_description = {};
      dc_->world[ch->in_room].dir_option[x]->keyword = {};
    }

    dc_->world[ch->in_room].dir_option[x]->exit_info = a;
    dc_->world[ch->in_room].dir_option[x]->key = b;
    dc_->world[ch->in_room].dir_option[x]->to_room = c;
    if (!remainder_args.empty())
    {
      if (dc_->world[ch->in_room].dir_option[x]->keyword)
        dc_->world[ch->in_room].dir_option[x]->keyword = {};
      dc_->world[ch->in_room].dir_option[x]->keyword = (remainder_args.c_str());
    }

    ch->sendln("Ok.");

    if (!ch->isNonPlayer() && !isSet(ch->player->toggles, Player::PLR_ONEWAY))
    {
      send_to_char("Attempting to create a return exit from "
                   "that room...\r\n",
                   ch);
      if (dc_->world[c].dir_option[reverse_number[x]])
      {
        ch->sendln("COULD NOT CREATE EXIT...One already exists.");
      }
      else
      {
        buf = fmt::format("{} redit exit {} {} {} {} {}", d,
                          return_directions[x], dc_->world[ch->in_room].number,
                          a, b, (remainder_args != "" ? remainder_args.c_str() : ""));
        SET_BIT(ch->player->toggles, Player::PLR_ONEWAY);
        QString tmp = strdup(buf.c_str());
        do_at(ch, tmp);
        free(tmp);
        REMOVE_BIT(ch->player->toggles, Player::PLR_ONEWAY);
      }
    }
  }
  break;

    /* redit extra */
  case 3:
  {
    if (remainder_args.empty())
    {
      ch->send(u"$3Syntax$R: <> is required. [] is optional.\r\nredit extra                   - show this syntax and current keywords.\r\nredit extra <keywords ...>    - add or edit keywords.\r\nredit extra <keyword>  - delete extra descriptions linked to keyword.\r\n\r\n"_s) = {};
      bool found = false;
      for (extra = dc_->world[ch->in_room].ex_description; extra != nullptr; extra = extra->next)
      {
        if (extra->keyword != nullptr)
        {
          if (found == false)
          {
            found = true;
            ch->sendln("Extra description keywords:");
          }
          ch->send(u"%1\r\n"_s.arg(extra->keyword));
        }
      }
      if (found == false)
      {
        ch->sendln("No extra description keywords found.");
      }

      return ReturnValue::eSUCCESS;
    }

    QString arg2;
    std::tie(arg2, remainder_args) = half_chop(remainder_args);

    if (arg2 == "delete")
    {
      QString arg3;
      std::tie(arg3, remainder_args) = half_chop(remainder_args);

      bool deleted = false;
      extra_descr_data *prev = {};
      for (extra = dc_->world[ch->in_room].ex_description; extra != nullptr; prev = extra, extra = extra->next)
      {
        if (arg3 == QString(extra->keyword))
        {
          if (prev == nullptr)
          {
            dc_->world[ch->in_room].ex_description = extra->next;
          }
          else
          {
            prev->next = extra->next;
          }
          ch->send(u"Extra description with keyword '%1' deleted.\r\n"_s.arg(extra->keyword));
          FREE(extra);
          deleted = true;
          // break out of for loop
          break;
        }
      }
      if (deleted == false)
      {
        ch->send(u"Extra description with keyword '%s' not found.\r\n"_s.arg(arg3.c_str()));
      }
      // break out of switch case
      break;
    }

    for (extra = dc_->world[ch->in_room].ex_description;; extra = extra->next)
    {
      if (!extra)
      {
        // No matching extra description found so make a new one
        ch->send(u"Creating new extra description for keyword '%s'.\r\n"_s.arg(arg2.c_str()));
        CREATE(extra, extra_descr_data, 1);
        extra->next = {};

        if (!(dc_->world[ch->in_room].ex_description))
        {
          // The room has no pre-existing extra descriptions so this will be its first
          dc_->world[ch->in_room].ex_description = extra;
        }
        else
        {
          // The room has pre-existing extra descriptions so we will find the end of
          // this linked list and append our new extra description to it
          for (ext = dc_->world[ch->in_room].ex_description;; ext = ext->next)
          {
            if (ext->next == nullptr)
            {
              ext->next = extra;
              break;
            }
          }
        }

        break;
      }
      else if (arg2 == QString(extra->keyword))
      {
        // A pre-existing extra description was found
        ch->send(u"Modifying extra description for keyword '%s'.\r\n"_s.arg(arg2.c_str()));
        break;
      }
    }

    FREE(extra->keyword);
    extra->keyword = (arg2.c_str());
    ch->sendln("Write your extra description. (/s saves /h for help)");
    ch->desc->strnew = &extra->description;
    ch->desc->connected = Connection::states::EDITING;
    if (ch->desc->strnew != nullptr && *ch->desc->strnew != nullptr && **ch->desc->strnew != '\0')
    {
      // There's already an existing extra description so let's show it
      parse_action(parse_t::LIST_NUM, "", ch->desc);
    }
  }
  break;

    /* redit exdesc */
  case 4:
  {
    if (remainder_args.empty())
    {
      ch->sendln("$3Syntax$R: redit exdesc <direction>");
      return ReturnValue::eFAILURE;
    }

    QString arg3;
    std::tie(arg3, remainder_args) = half_chop(remainder_args);
    for (x = {}; x <= 6; x++)
    {
      if (x == 6)
      {
        ch->sendln("No such direction.");
        return ReturnValue::eFAILURE;
      }
      if (is_abbrev(arg3.c_str(), dirs[x]))
        break;
    }

    if (!(dc_->world[ch->in_room].dir_option[x]))
    {
      ch->sendln("That exit does not exist...create it first.");
      return ReturnValue::eFAILURE;
    }

    send_to_char("Enter the exit's description below. Terminate with "
                 "'~' on a new line.\r\n\r\n",
                 ch);
    /*        if(dc_->world[ch->in_room].dir_option[x]->general_description) {
          dc_->world[ch->in_room].dir_option[x]->general_description={};
          dc_->world[ch->in_room].dir_option[x]->general_description = {};
        }
   */
    ch->desc->strnew = &dc_->world[ch->in_room].dir_option[x]->general_description;
    ch->desc->connected = Connection::states::EDITING;
  }
  break;

  // rflag
  case 5:
  {
    a = false;
    if (remainder_args.empty())
    {
      ch->sendln("$3Syntax$R: redit rflag <flags>");
      ch->sendln("$3Available room flags$R:");
      for (x = {};; x++)
      {
        if (!dc_strcmp(room_bits[x], "\n"))
          break;
        if (!dc_strcmp(room_bits[x], "unused"))
          continue;
        if ((x + 1) % 4 == 0)
        {
          ch->send(u"%-18s\r\n"_s.arg(room_bits[x]));
        }
        else
        {
          ch->send(u"%-18s"_s.arg(room_bits[x]));
        }
      }
      ch->sendln("\r\n");
      return ReturnValue::eFAILURE;
    }
    parse_bitstrings_into_int(room_bits, remainder_args.c_str(), ch, (dc_->world[ch->in_room].room_flags));
  }
  break;

  // sector
  case 6:
  {
    if (remainder_args.empty())
    {
      ch->sendln("$3Syntax$R: redit sector <sector>");
      ch->sendln("$3Available sector types$R:");
      for (x = {};; x++)
      {
        if (!dc_strcmp(sector_types[x], "\n"))
          break;
        if ((x + 1) % 4 == 0)
        {
          ch->send(u"%-18s\r\n"_s.arg(sector_types[x]));
        }
        else
        {
          ch->send(u"%-18s"_s.arg(sector_types[x]));
        }
      }
      ch->sendln("\r\n");
      return ReturnValue::eFAILURE;
    }
    for (x = {};; x++)
    {
      if (!dc_strcmp(sector_types[x], "\n"))
      {
        ch->sendln("No such sector type.");
        return ReturnValue::eFAILURE;
      }
      else if (is_abbrev(remainder_args.c_str(), sector_types[x]))
      {
        dc_->world[ch->in_room].sector_type = x;
        ch->send(u"Sector type set to %s.\r\n"_s.arg(sector_types[x]));
        break;
      }
    }
  }
  break;
  case 7: // denymob
    if (remainder_args.empty() || !is_number(remainder_args.c_str()))
    {
      ch->sendln("Syntax: redit denymob <vnum>\r\nDoing this on an already denied mob will allow it once more.");
      return ReturnValue::eFAILURE;
    }
    bool done = false;
    qint32 mob = {};
    try
    {
      mob = stoi(remainder_args);
    }
    catch (...)
    {
      mob = {};
    }
    deny_data *nd, *pd = {};
    for (nd = dc_->world[ch->in_room].denied; nd; nd = nd->next)
    {
      if (nd->vnum == mob)
      {
        if (pd)
          pd->next = nd->next;
        else
          dc_->world[ch->in_room].denied = nd->next;
        nd = {};
        ch->send(u"Mobile %1 ALLOWED entrance.\r\n"_s.arg(mob));
        done = true;
        break;
      }
      pd = nd;
    }
    if (done)
      break;
    auto nd = new deny_data;
    nd->next = dc_->world[ch->in_room].denied;
    try
    {
      nd->vnum = stoi(remainder_args);
    }
    catch (...)
    {
      nd->vnum = {};
    }

    dc_->world[ch->in_room].denied = nd;
    ch->send(u"Mobile %1 DENIED entrance.\r\n"_s.arg(mob));
    break;
  }
  dc_->set_zone_modified_world(ch->in_room);
  return ReturnValue::eSUCCESS;
}

command_return_t do_rdelete(CharacterPtr ch, QString arg, cmd_t cmd)
{
  qint32 x;
  QString buf, buf2[50];
  extra_descr_data *i, *extra;

  half_chop(arg, buf, buf2);

  if (buf.isEmpty())
  {
    send_to_char("$3Syntax$R:\r\nrdelete exit   <direction>\r\nrdelete "
                 "exdesc <direction>\r\nrdelete extra  <keyword>\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  if (!can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You cannot destroy things here, it is not your domain.");
    return ReturnValue::eFAILURE;
  }

  if (is_abbrev(buf, "exit"))
  {
    for (x = {}; x <= 6; x++)
    {
      if (x == 6)
      {
        ch->sendln("No such direction.");
        return ReturnValue::eFAILURE;
      }
      if (is_abbrev(buf2, dirs[x]))
        break;
    }

    if (!(dc_->world[ch->in_room].dir_option[x]))
    {
      ch->sendln("There is nothing there to remove.");
      return ReturnValue::eFAILURE;
    }
    dc_->world[ch->in_room].dir_option[x] = {};
    dc_->world[ch->in_room].dir_option[x] = {};
    ch->sendln(u"You stretch forth your hands and remove the %s exit.\r\n"_s.arg(dirs[x]));
  }

  else if (is_abbrev(buf, "extra"))
  {
    for (i = dc_->world[ch->in_room].ex_description;; i = i->next)
    {
      if (i == nullptr)
      {
        ch->sendln("There is nothing there to remove.");
        return ReturnValue::eFAILURE;
      }
      if (isexact(buf2, i->keyword))
        break;
    }

    if (dc_->world[ch->in_room].ex_description == i)
    {
      dc_->world[ch->in_room].ex_description = i->next;
      i = {};
      ch->sendln("You remove the extra description.");
    }
    else
    {
      for (extra = dc_->world[ch->in_room].ex_description;; extra = extra->next)
        if (extra->next == i)
        {
          extra->next = i->next;
          i = {};
          ch->sendln("You remove the extra description.");
          break;
        }
    }
  }

  else if (is_abbrev(buf, "exdesc"))
  {
    if (buf.isEmpty() 2)
    {
      ch->sendln("Syntax:\r\nrdelete exdesc <direction>");
      return ReturnValue::eFAILURE;
    }
    one_argument(buf2, buf);
    for (x = {}; x <= 6; x++)
    {
      if (x == 6)
      {
        ch->sendln("No such direction.");
        return ReturnValue::eFAILURE;
      }
      if (is_abbrev(buf, dirs[x]))
        break;
    }

    if (!(dc_->world[ch->in_room].dir_option[x]))
    {
      ch->sendln("That exit does not exist...create it first.");
      return ReturnValue::eFAILURE;
    }

    if (!(dc_->world[ch->in_room].dir_option[x]->general_description))
    {
      ch->sendln("There's no description there to delete.");
      return ReturnValue::eFAILURE;
    }

    else
    {
      dc_->world[ch->in_room].dir_option[x]->general_description = {};
      dc_->world[ch->in_room].dir_option[x]->general_description = {};
      ch->sendln("Ok.");
    }
  }

  else
    send_to_char("Syntax:\r\nrdelete exit   <direction>\r\nrdelete "
                 "exdesc <direction>\r\nrdelete extra  <keyword>\r\n",
                 ch);

  dc_->set_zone_modified_world(ch->in_room);
  return ReturnValue::eSUCCESS;
}

command_return_t do_oneway(CharacterPtr ch, QString arg, cmd_t cmd)
{
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (cmd == cmd_t::ONEWAY)
  {
    if (!isSet(ch->player->toggles, Player::PLR_ONEWAY))
      SET_BIT(ch->player->toggles, Player::PLR_ONEWAY);
    ch->sendln("You generate one-way exits.");
  }
  else
  {
    if (isSet(ch->player->toggles, Player::PLR_ONEWAY))
      REMOVE_BIT(ch->player->toggles, Player::PLR_ONEWAY);
    ch->sendln("You generate two-way exits.");
  }
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_zsave(QStringList arguments, cmd_t cmd)
{
  zone_t zone_key = {};
  auto &zones = dc_->zones;

  if (arguments.isEmpty())
  {
    zone_key = dc_->world[in_room].zone;
  }
  else
  {
    bool ok = false;
    zone_key = arguments.value(0).toULongLong(&ok);
    if (!ok)
    {
      sendln(u"Invalid zone number. Valid zone numbers are %1-%2."_s.arg(zones.firstKey()).arg(zones.lastKey()));
      return ReturnValue::eFAILURE;
    }
  }

  if (zones.contains(zone_key) == false)
  {
    sendln(u"Zone %1 not found. Valid zone numbers are %2-%3."_s.arg(zone_key).arg(zones.firstKey()).arg(zones.lastKey()));
    return ReturnValue::eFAILURE;
  }
  auto &zone = zones[zone_key];

  if (!can_modify_room(this, zone.getRealBottom()))
  {
    sendln("You may only zsave zones that include the room range you are assigned to.");
    return ReturnValue::eFAILURE;
  }

  if (zone.getFilename().isEmpty())
  {
    sendln(u"Zone %1 has an empty filename."_s.arg(zone_key));
    return ReturnValue::eFAILURE;
  }

  if (!zone.isModified())
  {
    sendln(u"Zone %1 has not been modified. Saving anyway."_s.arg(zone_key));
  }

  QString filename = u"zonefiles/%1"_s.arg(zone.getFilename());
  QString command = u"cp %1 %1.last"_s.arg(filename);
  system(qPrintable(command));

  FILE *f = {};
  if ((f = fopen(qPrintable(filename), "w")) == nullptr)
  {
    logbug(u"do_zsave: couldn't open zone save file '%1' for '%2'."_s.arg(filename).arg(name()));
    return ReturnValue::eFAILURE;
  }

  zone.write(f);

  fclose(f);
  sendln(u"Saved zone %1."_s.arg(zone_key));
  zone.setModified(false);
  return ReturnValue::eSUCCESS;
}

command_return_t do_rsave(CharacterPtr ch, QString arg, cmd_t cmd)
{
  world_file_list_item *curr;

  if (!can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You may only rsave inside of the room range you are assigned to.");
    return ReturnValue::eFAILURE;
  }

  curr = dc_->world_file_list;
  while (curr)
    if (curr->firstnum <= ch->in_room && curr->lastnum >= ch->in_room)
      break;
    else
      curr = curr->next;

  if (!curr)
  {
    ch->sendln("That range doesn't seem to exist...tell an imp.");
    return ReturnValue::eFAILURE;
  }

  if (!isSet(curr->flags, WORLD_FILE_MODIFIED))
  {
    ch->sendln("This range has not been modified.");
    return ReturnValue::eFAILURE;
  }

  LegacyFileWorld lfw(curr->filename);
  if (lfw.isOpen())
  {
    for (qint32 x = curr->firstnum; x <= curr->lastnum; x++)
    {
      write_one_room(lfw, x);
    }
  }

  ch->sendln("Saved.");
  dc_->set_zone_saved_world(ch->in_room);
  return ReturnValue::eSUCCESS;
}

command_return_t do_msave(CharacterPtr ch, QString arg, cmd_t cmd)
{
  world_file_list_item *curr;
  QString buf;
  QString buf2;

  if (ch->player->last_mob_edit <= 0)
  {
    ch->sendln("You have not recently edited a mobile.");
    return ReturnValue::eFAILURE;
  }

  qint32 v = ch->player->last_mob_edit;
  if (!can_modify_mobile(ch, v))
  {
    ch->sendln("You may only msave inside of the room range you are assigned to.");
    return ReturnValue::eFAILURE;
  }

  qint32 r = real_mobile(v);

  curr = dc_->mob_file_list;
  while (curr)
    if (curr->firstnum <= r && curr->lastnum >= r)
      break;
    else
      curr = curr->next;

  if (!curr)
  {
    ch->sendln("That range doesn't seem to exist...tell an imp.");
    return ReturnValue::eFAILURE;
  }

  if (!isSet(curr->flags, WORLD_FILE_MODIFIED))
  { // this is okay...world_file_saved is used in all
    ch->sendln("This range has not been modified.");
    return ReturnValue::eFAILURE;
  }

  LegacyFile lf("mobs", curr->filename, "Couldn't open legacy mob save file %1.");
  if (lf.isOpen())
  {
    for (qint32 x = curr->firstnum; x <= curr->lastnum; x++)
    {
      write_mobile(lf, (CharacterPtr)dc_->mob_index[x].item);
    }
    dc_fprintf(lf.file_handle_, "$~\n");
  }

  ch->sendln("Saved.");
  dc_->set_zone_saved_mob(curr->firstnum);
  return ReturnValue::eSUCCESS;
}

command_return_t do_osave(CharacterPtr ch, QString arg, cmd_t cmd)
{
  world_file_list_item *curr;
  QString buf;
  QString buf2;

  if (ch->player->last_obj_vnum < 1)
  {
    ch->sendln("You have not recently edited an item.");
    return ReturnValue::eFAILURE;
  }
  vnum_t v = ch->player->last_obj_vnum;
  if (!can_modify_object(ch, v))
  {
    ch->sendln("You may only msave inside of the room range you are assigned to.");
    return ReturnValue::eFAILURE;
  }
  qint32 r = real_object(v);
  curr = dc_->obj_file_list;
  while (curr)
    if (curr->firstnum <= r && curr->lastnum >= r)
      break;
    else
      curr = curr->next;

  if (!curr)
  {
    ch->sendln("That range doesn't seem to exist...tell an imp.");
    return ReturnValue::eFAILURE;
  }

  if (!isSet(curr->flags, WORLD_FILE_MODIFIED))
  {
    ch->sendln("This range has not been modified.");
    return ReturnValue::eFAILURE;
  }

  LegacyFile lf("objects", curr->filename, "Couldn't open legacy obj save file %1.");
  if (lf.isOpen())
  {
    for (qint32 x = curr->firstnum; x <= curr->lastnum; x++)
    {
      write_object(lf, dc_->obj_index[x].item);
    }
    dc_fprintf(lf.file_handle_, "$~\n");
  }

  ch->sendln("Saved.");
  dc_->set_zone_saved_obj(curr->firstnum);
  return ReturnValue::eSUCCESS;
}

command_return_t do_instazone(CharacterPtr ch, QString arg, cmd_t cmd)
{
  FILE *fl;
  QString buf, bufl[200] /*,buf2[200],buf3[200]*/;
  qint32 room = 1, x, door /*,direction*/;
  qint32 pos;
  qint32 value;
  qint32 low, high;
  qint32 /*number,*/ count;
  CharacterPtr mob, /**tmp_mob,*next_mob,*/ *mob_list;
  ObjectPtr obj, tmp_obj, /**next_obj,*/ *obj_list;

  bool found_room = false;

  ch->sendln("Whoever thought of this had a good idea, but never really finished it.  Beg someone to finish it some time.");
  return ReturnValue::eFAILURE;

  // Remember if you change this that it uses string_to_file which now appends a ~\n to the end
  // of the QString.  This command does NOT take that into consideration currently.

  /*    if(!GET_RANGE(ch)) {
   ch->sendln("You don't have a zone assigned to you!");
   return ReturnValue::eFAILURE;
   }

   half_chop(GET_RANGE(ch), buf, bufl);*/
  low = atoi(buf);
  high = atoi(bufl);

  for (x = low; x <= high; x++)
  {
    room = real_room(x);
    if (room == DC::NOWHERE)
      continue;
    found_room = true;
    break;
  }

  if (!found_room)
  {
    ch->send("Your area doesn't seem to be there.  Tell Godflesh!");
    return ReturnValue::eFAILURE;
  }

  dc_sprintf(buf, "../lib/builder/%s.zon", qPrintable(ch->name()));

  if ((fl = fopen(buf, "w")) == nullptr)
  {
    ch->send("Couldn't open up zone file. Tell Godflesh!");
    fclose(fl);
    return ReturnValue::eFAILURE;
  }

  for (x = low; x <= high; x++)
  {
    room = real_room(x);
    if (room == DC::NOWHERE)
      continue;
    break;
  }

  dc_fprintf(fl, "#%d\n", dc_->world[room].zone);
  dc_sprintf(buf, "%s's Area.", qPrintable(ch->name()));
  string_to_file(fl, buf);
  dc_fprintf(fl, "~\n");
  dc_fprintf(fl, "%d 30 2\n", high);

  /* Set allthe door states..  */

  for (x = low; x <= high; x++)
  {
    room = real_room(x);
    if (room == DC::NOWHERE)
      continue;

    for (door = {}; door <= 5; door++)
    {
      if (!(dc_->world[room].dir_option[door]))
        continue;

      if (isSet(dc_->world[room].dir_option[door]->exit_info, EX_ISDOOR))
      {
        if ((isSet(dc_->world[room].dir_option[door]->exit_info, EX_CLOSED)) && (isSet(dc_->world[room].dir_option[door]->exit_info, EX_LOCKED)))
        {
          value = 2;
        }
        else if (isSet(dc_->world[room].dir_option[door]->exit_info, EX_CLOSED))
        {
          value = 1;
        }
        else
        {
          value = {};
        }

        dc_fprintf(fl, "D 0 %d %d %d\n", dc_->world[room].number, dc_->world[dc_->world[room].dir_option[door]->to_room].number, value);
      }
    }
  } /*  Ok.. all door state info written...  */

  /*  Load loose objects.  In other words.. objects not carried by mobs. */

  for (x = low; x <= high; x++)
  {
    room = real_room(x);
    if (room == DC::NOWHERE)
      continue;

    if (dc_->world[room].contents)
    {

      for (obj = dc_->world[room].contents; obj; obj = obj->next_content)
      {

        count = {};

        for (obj_list = dc_->object_list; obj_list; obj_list =
                                                        obj_list->next)
        {
          if (obj_list->item_number == obj->item_number)
            count++;
        }

        if (!obj->in_obj)
        {
          dc_fprintf(fl, "O 0 %d %d %d",
                     dc_->obj_index[obj->item_number].vnum(), count,
                     dc_->world[room].number);
          dc_sprintf(buf, "           %s\n", qPrintable(obj->short_description()));
          string_to_file(fl, buf);

          if (obj->contains)
          {

            for (tmp_obj = obj->contains; tmp_obj;
                 tmp_obj = tmp_obj->next_content)
            {
              count = {};
              for (obj_list = dc_->object_list; obj_list; obj_list =
                                                              obj_list->next)
              {
                if (obj_list->item_number == tmp_obj->item_number)
                  count++;
              }

              dc_fprintf(fl, "P 1 %d %d %d",
                         dc_->obj_index[tmp_obj->item_number].vnum(), count,
                         dc_->obj_index[obj->item_number].vnum());
              dc_sprintf(buf, "     %s placed inside %s\n",
                         tmp_obj->short_description,
                         qPrintable(obj->short_description()));
              string_to_file(fl, buf);
            } /*  for loop */
          } /* end of the object's contents... */
        } /* end of if !obj->in_obj */
      } /* first for loop for loose objects... */
    } /* All loose objects taken care of..  (Not on mobs,but inthe rooms) */
  } /*  for loop going through the fucking rooms again for loose obj's */

  /* Now for the major bitch...
   * All the mobs, and all possible bullshit our builders will try to
   * put on them.. conn->e held objects with objects within them... Just
   * your average pain in the ass shit....
   */

  for (x = low; x <= high; x++)
  {
    room = real_room(x);
    if (room == DC::NOWHERE)
      continue;

    if (dc_->world[room].people)
    {

      for (mob = dc_->world[room].people; mob; mob = mob->next_in_room)
      {
        if (mob->isPlayer())
          continue;

        count = {};

        for (mob_list = character_list; mob_list;
             mob_list = mob_list->next)
        {
          if (mob_list->isNonPlayer() && mob_list->mobdata->nr == mob->mobdata->nr)
            count++;
        }

        dc_fprintf(fl, "M 0 %d %d %d", dc_->mob_index[mob->mobdata->nr].vnum(),
                   count, dc_->world[room].number);
        dc_sprintf(buf, "           %s\n", mob->short_desc);
        string_to_file(fl, buf);

        for (pos = {}; pos < MAX_WEAR; pos++)
        {
          if (mob->equipment[pos])
          {

            obj = mob->equipment[pos];

            count = {};
            for (obj_list = dc_->object_list; obj_list; obj_list =
                                                            obj_list->next)
            {
              if (obj_list->item_number == obj->item_number)
                count++;
            }

            if (!obj->in_obj)
            {
              dc_fprintf(fl, "E 1 %d %d %d",
                         dc_->obj_index[obj->item_number].vnum(), count,
                         pos);
              dc_sprintf(buf, "      Equip %s with %s\n",
                         mob->short_desc, qPrintable(obj->short_description()));
              string_to_file(fl, buf);

              if (obj->contains)
              {

                for (tmp_obj = obj->contains; tmp_obj; tmp_obj =
                                                           tmp_obj->next_content)
                {
                  count = {};
                  for (obj_list = dc_->object_list; obj_list;
                       obj_list = obj_list->next)
                  {
                    if (obj_list->item_number == tmp_obj->item_number)
                      count++;
                  }

                  dc_fprintf(fl, "P 1 %d %d %d",
                             dc_->obj_index[tmp_obj->item_number].vnum(),
                             count,
                             dc_->obj_index[obj->item_number].vnum());
                  dc_sprintf(buf, "     %s placed inside %s\n",
                             tmp_obj->short_description,
                             qPrintable(obj->short_description()));
                  string_to_file(fl, buf);
                } /*  for loop */
              }
            } /* end of the object's contents... */
          } /* End of if ch->equipment[pos]  */
        } /* For loop going through a mob's eq.. */

        if (mob->carrying)
        {

          for (obj = mob->carrying; obj; obj = obj->next_content)
          {

            count = {};
            for (obj_list = dc_->object_list; obj_list; obj_list =
                                                            obj_list->next)
            {
              if (obj_list->item_number == obj->item_number)
                count++;
            }

            if (!obj->in_obj)
            {
              dc_fprintf(fl, "G 1 %d %d",
                         dc_->obj_index[obj->item_number].vnum(), count);
              dc_sprintf(buf, "      Give %s %s\n", mob->short_desc,
                         qPrintable(obj->short_description()));
              string_to_file(fl, buf);

              if (obj->contains)
              {

                for (tmp_obj = obj->contains; tmp_obj; tmp_obj =
                                                           tmp_obj->next_content)
                {
                  count = {};
                  for (obj_list = dc_->object_list; obj_list;
                       obj_list = obj_list->next)
                  {
                    if (obj_list->item_number == tmp_obj->item_number)
                      count++;
                  }

                  dc_fprintf(fl, "P 1 %d %d %d",
                             dc_->obj_index[tmp_obj->item_number].vnum(),
                             count,
                             dc_->obj_index[obj->item_number].vnum());
                  dc_sprintf(buf, "     %s placed inside %s\n",
                             tmp_obj->short_description,
                             qPrintable(obj->short_description()));
                  string_to_file(fl, buf);
                } /*  for loop */
              }
            } /* end of the object's contents... */
          } /* end of for loop going through a mob's inventory */
        } /* end of if a mob has shit in their inventory */
      } /* end of for loop for looking at the mobs in ths room. */
    } /* end of if some body is in the fucking room.  */
  } /* end of for loop going through the zone looking for mobs...  */

  dc_fprintf(fl, "S\n");
  fclose(fl);
  ch->sendln("Zone File Created! Tell someone who can put it in!");
  return ReturnValue::eSUCCESS;
}

command_return_t do_rstat(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  QString buf;
  QString buf2;
  Room *rm = {};
  CharacterPtr k = {};
  ObjectPtr j = {};
  extra_descr_data *desc;
  qint32 i, x, loc;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  argument = one_argument(argument, arg1);

  /* no argument */
  if (arg.isEmpty() 1)
  {
    rm = &dc_->world[ch->in_room];
  }

  else
  {
    x = atoi(arg1);
    if (x < 0 || (loc = real_room(x)) == DC::NOWHERE)
    {
      ch->sendln("No such room exists.");
      return ReturnValue::eFAILURE;
    }
    rm = &dc_->world[loc];
  }
  if (isSet(rm->room_flags, CLAN_ROOM) && ch->getLevel() < PATRON)
  {
    ch->sendln("And you are rstating a clan room because?");
    dc_sprintf(buf, "%s just rstat'd clan room %d.", qPrintable(ch->name()), rm->number);
    dc_->logentry(buf, PATRON, DC::LogChannel::LOG_GOD);
    return ReturnValue::eFAILURE;
  }
  dc_sprintf(buf, "Room name: %s, Of zone : %d. V-Number : %d, R-number : %d\r\n",
             rm->name, rm->zone, rm->number, ch->in_room);
  ch->send(buf);

  sprinttype(rm->sector_type, sector_types, buf2);
  dc_sprintf(buf, "Sector type : %s ", buf2);
  ch->send(buf);

  dc_strcpy(buf, "Special procedure : ");
  dc_strcat(buf, (rm->funct) ? "Exists\r\n" : "No\r\n");
  ch->send(buf);

  ch->send("Room flags: ");
  sprintbit((qint32)rm->room_flags, room_bits, buf);
  QString buffer = fmt::format("{} [ {} ]\r\n", buf, rm->room_flags);
  send_to_char(buffer.c_str(), ch);

  ch->sendln("Description:");
  ch->send(rm->description);

  dc_strcpy(buf, "Extra description keywords(s): ");
  if (rm->ex_description)
  {
    dc_strcat(buf, "\r\n");
    for (desc = rm->ex_description; desc; desc = desc->next)
    {
      dc_strcat(buf, desc->keyword);
      dc_strcat(buf, "\r\n");
    }
    dc_strcat(buf, "\r\n");
    ch->send(buf);
  }
  else
  {
    dc_strcat(buf, "None\r\n");
    ch->send(buf);
  }
  deny_data *d;
  qint32 a = {};
  for (d = rm->denied; d; d = conn->next)
  {
    if (a == 0)
      ch->send("Mobiles Denied: ");
    if (real_mobile(conn->vnum) == -1)
      ch->send(u"UNKNOWN(%1)\r\n"_s.arg(conn->vnum));
    else
      ch->send(u"%s(%d)\r\n"_s.arg(qPrintable(((CharacterPtr)dc_->mob_index[real_mobile(conn->vnum)].item)->short_description())).arg(conn->vnum));
    a++;
  }
  ch->sendln("");
  dc_strcpy(buf, "------- Chars present -------\r\n");
  for (k = rm->people; k; k = k->next_in_room)
  {
    if (CAN_SEE(ch, k))
    {
      dc_strcat(buf, qPrintable(k->name()));
      dc_strcat(buf,
                (k->isPlayer() ? "(PC)\r\n" : (!k->isNonPlayer() ? "(NPC)\r\n" : "(MOB)\r\n")));
    }
  }
  dc_strcat(buf, "\r\n");
  ch->send(buf);

  buffer = "--------- Contents ---------\r\n";
  for (j = rm->contents; j; j = j->next_content)
  {
    if (CAN_SEE_OBJ(ch, j))
    {
      buffer += j->name().toStdString();
      buffer += "\r\n";
    }
  }
  buffer += "\r\n";
  send_to_char(const_cast<QString>(buffer.c_str()), ch);

  ch->sendln("------- Exits defined -------");
  for (i = {}; i <= 5; i++)
  {
    if (rm->dir_option[i])
    {
      dc_sprintf(buf, "Direction %s . Keyword : %s\r\n",
                 dirs[i], rm->dir_option[i]->keyword);
      ch->send(buf);
      dc_strcpy(buf, "Description:\r\n  ");
      if (rm->dir_option[i]->general_description)
        dc_strcat(buf, rm->dir_option[i]->general_description);
      else
        dc_strcat(buf, "UNDEFINED\r\n");
      ch->send(buf);
      sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
      dc_sprintf(buf, "Exit flag: %s \r\nKey no: %d\r\nTo room (V-Number): %d\r\n",
                 buf2, rm->dir_option[i]->key,
                 rm->dir_option[i]->to_room);
      ch->send(buf);
    }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_possess(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  CharacterPtr victim;
  QString buf;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  one_argument(argument, arg);

  if (arg.isEmpty())
  {
    ch->sendln("Possess who?");
  }
  else
  {
    if (!(victim = get_char_vis(ch, arg)))
      ch->sendln("They aren't here.");
    else
    {
      if (ch == victim)
      {
        ch->sendln("He he he... We are jolly funny today, eh?");
        return ReturnValue::eFAILURE;
      }
      else if ((victim->getLevel() > ch->getLevel()) &&
               (ch->getLevel() < IMPLEMENTER))
      {
        ch->sendln("That mob is a bit too tough for you to handle.");
        return ReturnValue::eFAILURE;
      }
      else if (!ch->desc || ch->desc->snoop_by || ch->desc->snooping)
      {
        if (ch->desc->snoop_by)
        {
          ch->desc->snoop_by->character->send("Whoa! Almost got caught snooping!\n");
          dc_sprintf(buf, "Your victim is now trying to possess: %s\n", qPrintable(victim->name()));
          ch->desc->snoop_by->character->send(buf);
          ch->desc->snoop_by->character->do_snoop(ch->desc->snoop_by->character->name().split(' '));
        }
        else
        {
          ch->sendln("Mixing snoop & possess is bad for your health.");
          return ReturnValue::eFAILURE;
        }
      }

      else if (victim->desc || (victim->isPlayer()))
      {
        ch->sendln("You can't do that, the body is already in use!");
        return ReturnValue::eFAILURE;
      }
      else
      {
        ch->sendln("Ok.");
        dc_sprintf(buf, "%s possessed %s", qPrintable(ch->name()), qPrintable(victim->name()));
        dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
        ch->player->possesing = 1;
        ch->desc->character = victim;
        ch->desc->original = ch;

        victim->desc = ch->desc;
        ch->desc = {};
      }
    }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_return(CharacterPtr ch, QString argument, cmd_t cmd)
{

  //    if(ch->isNonPlayer())
  //        return ReturnValue::eFAILURE;

  if (!ch->desc)
    return ReturnValue::eFAILURE;

  if (!ch->desc->original)
  {
    ch->sendln("Huh!?!");
    return ReturnValue::eFAILURE;
  }
  else
  {
    ch->sendln("You return to your original body.");

    ch->desc->original->player->possesing = {};
    ch->desc->character = ch->desc->original;
    ch->desc->original = {};

    ch->desc->character->desc = ch->desc;
    ch->desc = {};
    if (ch->isNonPlayer() && dc_->mob_index[ch->mobdata->nr].vnum() > 90 &&
        dc_->mob_index[ch->mobdata->nr].vnum() < 100 &&
        cmd != cmd_t::LOOK)
    {
      act_to_room("$n evaporates.", ch, 0, 0, 0);
      extract_char(ch, true);
      return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
    }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_sockets(QStringList arguments, cmd_t cmd)
{
  QString searchkey;
  if (!arguments.isEmpty())
  {
    searchkey = arguments.at(0);
  }

  const Sockets sockets{this, searchkey};
  const auto IPs = sockets.getIPs();
  const auto connections = sockets.getConnections();
  const quint64 longest_IP_size = std::max(2UL, sockets.getLongestIPSize());
  const quint64 longest_name_size = std::max(4UL, sockets.getLongestNameSize());
  const quint64 longest_connection_state_size = std::max(5UL, sockets.getLongestConnectionStateSize());
  const quint64 longest_idle_size = std::max(4UL, sockets.getLongestIdleSize() + 1);

  send(u"%1: %2 | %3 | %4 | %5$R\r\n").arg("Des"_.arg("Name", -longest_name_size).arg("State", -longest_connection_state_size).arg("Idle", -longest_idle_size).arg("IP", -longest_IP_size));
  for (const auto &d : connections)
  {
    const QString connection_character_name = conn->name();
    const QString IPstr = d.getPeerFullAddressString();
    const auto descriptor = conn->descriptor;
    const bool duplicate_IP = IPs[d.getPeerOriginalAddress().toString()] > 1;
    const QString connected_state = constindex(conn->connected, DC::connected_states);
    const QString idle_seconds = u"%1s"_s.arg(d.idle_time / DC::PASSES_PER_SEC);

    send(u"%1%2: %3 | %4 | %5 | %6$R\r\n").arg(duplicate_IP ? "$B$4" : ""_.arg(descriptor, 3).arg(connection_character_name, -longest_name_size).arg(connected_state, -longest_connection_state_size).arg(idle_seconds, -longest_idle_size).arg(IPstr, -longest_IP_size));
  }

  send(u"\r\nThere are %1 connections.\r\n"_s.arg(connections.size()));

  return ReturnValue::eSUCCESS;
}

command_return_t do_setvote(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString buf;
  QString buf2;
  void send_info(QString);
  half_chop(arg, buf, buf2);

  if (buf.isEmpty())
  {
    ch->send("Syntax: voteset <question|add|remove|clear|start|end> <QString>");
    return ReturnValue::eFAILURE;
  }

  auto dc = dc_;
  if (buf == u"start"_s)
  {
    dc->DCVote.StartVote(ch);
    return ReturnValue::eSUCCESS;
  }

  if (buf == u"clear"_s)
  {
    dc->DCVote.Reset(ch);
    return ReturnValue::eSUCCESS;
  }

  if (buf == u"end"_s)
  {
    dc->DCVote.EndVote(ch);
    return ReturnValue::eSUCCESS;
  }

  if (buf.isEmpty() 2)
  {
    ch->send("Syntax: voteset <question|add|remove|clear|start|end> <QString>");
    return ReturnValue::eFAILURE;
  }

  if (buf == u"question"_s)
  {
    dc->DCVote.SetQuestion(ch, buf2);
    return ReturnValue::eSUCCESS;
  }
  if (buf == u"add"_s)
  {
    dc->DCVote.AddAnswer(ch, buf2);
    return ReturnValue::eSUCCESS;
  }
  if (buf == u"remove"_s)
  {
    dc->DCVote.RemoveAnswer(ch, (quint32)atoi(buf2));
    return ReturnValue::eSUCCESS;
  }

  ch->send("Syntax: voteset <question|add|remove|clear|start|end> <QString>");
  return ReturnValue::eFAILURE;
}

command_return_t do_punish(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString name, buf[150];
  CharacterPtr vict;

  qint32 i;

  if (ch->isNonPlayer())
  {
    ch->sendln("Punish yourself!  Bad mob!");
    return ReturnValue::eFAILURE;
  }

  arg = one_argument(arg, name);

  if (name.isEmpty())
  {
    ch->sendln("Punish who?");
    send_to_char("\r\nusage: punish <character> [stupid silence freeze noemote "
                 "notell noname noarena notitle nopray]\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  if (!(vict = get_pc_vis(ch, name)))
  {
    dc_snprintf(buf, sizeof(buf), "%s not found.\r\n", name);
    ch->send(buf);
    return ReturnValue::eFAILURE;
  }

  one_argument(arg, name);

  if (!name[0])
  {
    display_punishes(ch, vict);
    return ReturnValue::eFAILURE;
  }

  i = strlen(name);

  if (i > 5)
    i = 5;

  if (vict->getLevel() > ch->getLevel())
  {
    act_to_character("$E might object to that.. better not.", ch, 0, vict, 0);
    return ReturnValue::eFAILURE;
  }
  if (!strncasecmp(name, "stupid", i))
  {
    if (isSet(vict->player->punish, PUNISH_STUPID))
    {
      vict->sendln("You feel a sudden onslaught of wisdom!");
      ch->sendln("STUPID removed.");
      dc_sprintf(buf, "%s removes %s's stupid", qPrintable(ch->name()), qPrintable(vict->name()));
      dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
      REMOVE_BIT(vict->player->punish, PUNISH_STUPID);
      REMOVE_BIT(vict->player->punish, PUNISH_SILENCED);
      REMOVE_BIT(vict->player->punish, PUNISH_NOEMOTE);
      REMOVE_BIT(vict->player->punish, PUNISH_NONAME);
      REMOVE_BIT(vict->player->punish, PUNISH_NOTITLE);
    }
    else
    {
      vict->sendln("You suddenly feel dumb as a rock!");
      vict->sendln("You can't remember how to do basic things!");
      dc_sprintf(buf, "You have been lobotomized by %s!\r\n", qPrintable(ch->name()));
      vict->send(buf);
      ch->sendln("STUPID set.");
      dc_sprintf(buf, "%s lobotimized %s", qPrintable(ch->name()), qPrintable(vict->name()));
      dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
      SET_BIT(vict->player->punish, PUNISH_STUPID);
      SET_BIT(vict->player->punish, PUNISH_SILENCED);
      SET_BIT(vict->player->punish, PUNISH_NOEMOTE);
      SET_BIT(vict->player->punish, PUNISH_NONAME);
      SET_BIT(vict->player->punish, PUNISH_NOTITLE);
    }
  }
  if (!strncasecmp(name, "silence", i))
  {
    if (isSet(vict->player->punish, PUNISH_SILENCED))
    {
      vict->sendln("The gods take pity on you and lift your silence.");
      ch->sendln("SILENCE removed.");
      dc_sprintf(buf, "%s removes %s's silence", qPrintable(ch->name()), qPrintable(vict->name()));
      dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    }
    else
    {
      dc_sprintf(buf, "You have been silenced by %s!\r\n", qPrintable(ch->name()));
      vict->send(buf);
      ch->sendln("SILENCE set.");
      dc_sprintf(buf, "%s silenced %s", qPrintable(ch->name()), qPrintable(vict->name()));
      dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_SILENCED);
  }
  if (!strncasecmp(name, "freeze", i))
  {
    if (isSet(vict->player->punish, PUNISH_FREEZE))
    {
      vict->sendln("You now can do things again.");
      ch->sendln("FREEZE removed.");
      dc_sprintf(buf, "%s unfrozen by %s", qPrintable(vict->name()), qPrintable(ch->name()));
      dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    }
    else
    {
      dc_sprintf(buf, "%s takes away your ability to....\r\n", qPrintable(ch->name()));
      vict->send(buf);
      ch->sendln("FREEZE set.");
      dc_sprintf(buf, "%s frozen by %s", qPrintable(vict->name()), qPrintable(ch->name()));
      dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_FREEZE);
  }
  if (!strncasecmp(name, "noarena", i))
  {
    if (isSet(vict->player->punish, PUNISH_NOARENA))
    {
      vict->sendln("Some kind god has let you join arenas again.");
      ch->sendln("NOARENA removed.");
    }
    else
    {
      dc_sprintf(buf, "%s takes away your ability to join arenas!\r\n", qPrintable(ch->name()));
      ch->sendln("NOARENA set.");
      vict->send(buf);
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_NOARENA);
  }
  if (!strncasecmp(name, "noemote", i))
  {
    if (isSet(vict->player->punish, PUNISH_NOEMOTE))
    {
      vict->sendln("You can emote again.");
      ch->sendln("NOEMOTE removed.");
    }
    else
    {
      dc_sprintf(buf, "%s takes away your ability to emote!\r\n", qPrintable(ch->name()));
      vict->send(buf);
      ch->sendln("NOEMOTE set.");
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_NOEMOTE);
  }
  if (!strncasecmp(name, "notell", i))
  {
    if (isSet(vict->player->punish, PUNISH_NOTELL))
    {
      vict->sendln("You can use telepatic communication again.");
      ch->sendln("NOTELL removed.");
    }
    else
    {
      dc_sprintf(buf, "%s takes away your ability to use telepathic communication!\r\n", qPrintable(ch->name()));
      vict->send(buf);
      ch->sendln("NOTELL set.");
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_NOTELL);
  }
  if (!strncasecmp(name, "noname", i))
  {
    if (isSet(vict->player->punish, PUNISH_NONAME))
    {
      vict->sendln("The gods grant you control over your name.");
      ch->sendln("NONAME removed.");
    }
    else
    {
      dc_sprintf(buf, "%s removes your ability to set your name!\r\n", qPrintable(ch->name()));
      ch->sendln("NONAME set.");
      vict->send(buf);
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_NONAME);
  }

  if (!strncasecmp(name, "notitle", i))
  {
    if (isSet(vict->player->punish, PUNISH_NOTITLE))
    {
      vict->sendln("The gods grant you control over your title.");
      ch->sendln("NOTITLE removed.");
    }
    else
    {
      dc_sprintf(buf, "%s removes your ability to set your title!\r\n", qPrintable(ch->name()));
      ch->sendln("NOTITLE set.");
      vict->send(buf);
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_NOTITLE);
  }

  if (!strncasecmp(name, "unlucky", i))
  {
    if (isSet(vict->player->punish, PUNISH_UNLUCKY))
    {
      if (!ch->player->stealth)
        vict->sendln("The gods remove your poor luck.");
      ch->sendln("UNLUCKY removed.");
      dc_sprintf(buf, "%s removes %s's unlucky.", qPrintable(ch->name()), qPrintable(vict->name()));
      dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    }
    else
    {
      if (!ch->player->stealth)
      {
        dc_sprintf(buf, "%s curses you with god-given bad luck!\r\n", qPrintable(ch->name()));
        ch->sendln("UNLUCKY set.");
      }
      vict->send(buf);
      dc_sprintf(buf, "%s makes %s unlucky.", qPrintable(ch->name()), qPrintable(vict->name()));
      dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_UNLUCKY);
  }

  if (!strncasecmp(name, "nopray", i) || !strncasecmp(name, "nemke", i))
  {
    if (isSet(vict->player->punish, PUNISH_NOPRAY))
    {
      vict->sendln("The gods will once again hear your prayers.");
      vict->sendln("But not necessarily answer them...");
      ch->sendln("NOPRAY (nemke) removed.");
    }
    else
    {
      vict->send(u"%s has removed your ability to pray!\r\n"_s.arg(qPrintable(ch->name())));
      ch->sendln("NOPRAY (nemke) set.");
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_NOPRAY);
  }

  display_punishes(ch, vict);
  return ReturnValue::eSUCCESS;
}

void display_punishes(CharacterPtr ch, CharacterPtr vict)
{
  QString buf;

  dc_sprintf(buf, "$3Punishments for %s$R: ", qPrintable(vict->name()));
  ch->send(buf);

  if (isSet(vict->player->punish, PUNISH_NONAME))
    ch->send("noname ");

  if (isSet(vict->player->punish, PUNISH_SILENCED))
    ch->send("Silence ");

  if (isSet(vict->player->punish, PUNISH_NOEMOTE))
    ch->send("noemote ");

  if (isSet(vict->player->punish, PUNISH_LOG) && ch->getLevel() > 108)
    ch->send("log ");

  if (isSet(vict->player->punish, PUNISH_FREEZE))
    ch->send("Freeze ");

  if (isSet(vict->player->punish, PUNISH_SPAMMER))
    ch->send("Spammer ");

  if (isSet(vict->player->punish, PUNISH_STUPID))
    ch->send("Stupid ");

  if (isSet(vict->player->punish, PUNISH_NOTELL))
    ch->send("notell ");

  if (isSet(vict->player->punish, PUNISH_NOARENA))
    ch->send("noarena ");

  if (isSet(vict->player->punish, PUNISH_NOTITLE))
    ch->send("notitle ");

  if (isSet(vict->player->punish, PUNISH_UNLUCKY))
    ch->send("unlucky ");

  if (isSet(vict->player->punish, PUNISH_NOPRAY))
    ch->send("nopray ");

  ch->sendln("");
}

command_return_t do_colors(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;

  send_to_char("Color codes are a $$ followed by a code.\r\n\r\n"
               " Code   Bold($$B)  Inverse($$I)  Both($$B$$I)\r\n",
               ch);

  ch->sendln("  $$0$0)   $B********$R  $0$I***********$R  $0$B$I**********$R (plain 0 can't be seen normally)");
  for (qint32 i = 1; i < 8; i++)
  {
    dc_sprintf(buf, "  $$$%d%d)   $B********$R  $%d$I***********$R  $%d$B$I**********$R\r\n", i, i, i, i);
    ch->send(buf);
  }
  send_to_char("\r\nTo return to 'normal' color use $$R.\r\n\r\n"
               "Example:  'This is $$Bbold and $$4bold $$R$$4red$R!' will print:\r\n"
               "           This is $Bbold and $4bold $R$4red$R!\r\n",
               ch);
  return ReturnValue::eSUCCESS;
}
