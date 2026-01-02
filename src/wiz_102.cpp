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

#include <cctype> // isspace

#include <string>
#include <vector>
#include <limits>
#include <type_traits>
#include <tuple>
#include <algorithm>

#include <fmt/format.h>

#include "DC/wizard.h"
#include "DC/utility.h"
#include "DC/connect.h"
#include "DC/player.h"
#include "DC/room.h"
#include "DC/DC.h"
#include "DC/mobile.h"

#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/spells.h"
#include "DC/race.h"
#include "DC/returnvals.h"
#include "DC/vault.h"
#include "DC/set.h"
#include "DC/structs.h"
#include "DC/guild.h"
#include "DC/const.h"
#include "DC/newedit.h"

command_return_t zedit_list(Character *ch, QStringList arguments, const Zone &zone, bool stats = false);

// Urizen's rebuild rnum references to enable additions to mob/obj arrays w/out screwing everything up.
// A hack of renum_zone_tables *yawns*
// type 1 = mobs, type 2 = objs. Simple as that.
// This should obviously not be called at any other time than additions to the previously mentioned
// arrays, as it'd screw things up.
// Saving zones after this SHOULD not be required, as the old savefiles contain vnums, which should remain correct.
void rebuild_rnum_references(int startAt, int type)
{
  for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
  {
    for (qsizetype comm = 0; !zone.cmd.isEmpty() && comm < zone.cmd.size(); comm++)
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
        logentry(QStringLiteral("Illegal char hit in rebuild_rnum_references"), 0, DC::LogChannel::LOG_WORLD);
        break;
      }
    }
  }
}

int do_check(Character *ch, char *arg, cmd_t cmd)
{
  class Connection d;
  Character *vict;
  int connected = 1;
  char buf[120];
  char tmp_buf[160];
  char *c;

  while (isspace(*arg))
    arg++;

  if (!*arg)
  {
    ch->sendln("Check who?");
    return eFAILURE;
  }

  one_argument(arg, buf);
  buf[0] = UPPER(buf[0]);
  if (!(vict = get_pc_vis_exact(ch, buf)))
  {
    connected = 0;

    c = buf;
    *c = UPPER(*c);
    c++;
    while (*c)
    {
      *c = LOWER(*c);
      c++;
    }

    // must be done to clear out "d" before it is used
    if (!(ch->getDC()->load_char_obj(&d, buf)))
    {

      sprintf(tmp_buf, "../archive/%s.gz", buf);
      if (file_exists(tmp_buf))
        ch->sendln("Character is archived.");
      else
        ch->sendln("Unable to load! (Character might not exist...)");
      return eFAILURE;
    }

    vict = d.character;
    vict->desc = 0;

    redo_hitpoints(vict);
    redo_mana(vict);
    if (!GET_TITLE(vict))
      GET_TITLE(vict) = str_dup("is a virgin");
    if (GET_CLASS(vict) == CLASS_MONK)
      GET_AC(vict) -= vict->getLevel() * 3;
    isr_set(vict);
  }

  sprintf(buf, "$3Short Desc$R: %s\n\r", GET_SHORT(vict));
  ch->send(buf);
  sprintf(buf, "$3Race$R: %-9s $3Class$R: %-9s $3Level$R: %-8d $3In Room$R: %d\n\r",
          races[(int)(GET_RACE(vict))].singular_name,
          pc_clss_types[(int)(GET_CLASS(vict))], vict->getLevel(),
          (connected ? DC::getInstance()->world[vict->in_room].number : -1));
  ch->send(buf);
  sprintf(buf, "$3Exp$R: %-10ld $3Gold$R: %-10ld $3Bank$R: %-9d $3Align$R: %d\n\r",
          GET_EXP(vict), vict->getGold(), GET_BANK(vict), GET_ALIGNMENT(vict));
  ch->send(buf);
  if (ch->getLevel() >= SERAPH)
  {
    sprintf(buf, "$3Load Rm$R: %-5d  $3Home Rm$R: %-5hd  $3Platinum$R: %d  $3Clan$R: %d\n\r",
            DC::getInstance()->world[vict->in_room].number, vict->hometown, GET_PLATINUM(vict), GET_CLAN(vict));
    ch->send(buf);
  }
  sprintf(buf, "$3Str$R: %-2d  $3Wis$R: %-2d  $3Int$R: %-2d  $3Dex$R: %-2d  $3Con$R: %d\n\r",
          GET_STR(vict), GET_WIS(vict), GET_INT(vict), GET_DEX(vict),
          GET_CON(vict));
  ch->send(buf);
  sprintf(buf, "$3Hit Points$R: %d/%d $3Mana$R: %d/%d $3Move$R: %d/%d $3Ki$R: %d/%d\n\r",
          vict->getHP(), GET_MAX_HIT(vict), GET_MANA(vict),
          GET_MAX_MANA(vict), GET_MOVE(vict), GET_MAX_MOVE(vict),
          GET_KI(vict), GET_MAX_KI(vict));
  ch->send(buf);

  if (ch->getLevel() >= OVERSEER && vict->isPlayer() && ch->getLevel() >= vict->getLevel())
  {
    ch->sendln(QStringLiteral("$3Last connected from$R: %1").arg(vict->player->last_site));

    /* ctime adds a \n to the std::string it returns! */
    const time_t tBuffer = vict->player->time.logon;
    +sprintf(buf, "$3Last connected on$R: %s\r", ctime(&tBuffer));
    ch->send(buf);
  }

  display_punishes(ch, vict);

  if (connected)
    if (vict->desc)
    {
      if (ch->getLevel() >= OVERSEER && ch->getLevel() >= vict->getLevel())
      {
        sprintf(buf, "$3Connected from$R: %s\n\r", vict->desc->getPeerOriginalAddress().toString().toStdString().c_str());
        ch->send(buf);
      }
      else
      {
        sprintf(buf, "Connected.\r\n");
        ch->send(buf);
      }
    }
    else
      ch->sendln("(Linkdead)");
  else
  {
    ch->sendln("(Not on game)");
    free_char(vict, Trace("do_check"));
  }
  return eSUCCESS;
}

int do_find(Character *ch, char *arg, cmd_t cmd)
{
  char type[MAX_INPUT_LENGTH];
  char name[MAX_INPUT_LENGTH];
  Character *vict;
  int x;

  char *types[] = {
      "mob",
      "pc",
      "char",
      "obj",
      "\n"};

  if (IS_NPC(ch))
    return eFAILURE;
  if (!ch->has_skill(COMMAND_FIND))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  half_chop(arg, type, name);

  if (!*type || !*name)
  {
    ch->sendln("Usage:  find <mob|pc|char|obj> <name>");
    return eFAILURE;
  }

  for (x = 0; x <= 4; x++)
  {
    if (x == 4)
    {
      ch->sendln("Type must be one of these: mob, pc, char, obj.");
      return eFAILURE;
    }
    if (is_abbrev(type, types[x]))
      break;
  }

  switch (x)
  {
  default:
    ch->sendln("Problem...fuck up in do_find.");
    logentry(QStringLiteral("Default in do_find...should NOT happen."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
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
    return eFAILURE;
  }

  sprintf(type, "%30s -- %s [%d]\n\r", GET_SHORT(vict),
          DC::getInstance()->world[vict->in_room].name, DC::getInstance()->world[vict->in_room].number);
  ch->send(type);
  return eSUCCESS;
}

int do_stat(Character *ch, char *arg, cmd_t cmd)
{
  class Connection d;
  Character *vict;
  class Object *obj;
  char type[MAX_INPUT_LENGTH];
  char name[MAX_INPUT_LENGTH];
  char *c;
  int x;

  char *types[] = {
      "mobile",
      "object",
      "character"};
  if (!ch->has_skill(COMMAND_STAT))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  if (IS_NPC(ch))
    return eFAILURE;

  half_chop(arg, type, name);

  if (!*type || !*name)
  {
    ch->sendln("Usage:  stat <mob|obj|char> <name>");
    return eFAILURE;
  }

  for (x = 0; x <= 3; x++)
  {
    if (x == 3)
    {
      send_to_char("Type must be one of these: mobile, object, "
                   "character.\r\n",
                   ch);
      return eFAILURE;
    }
    if (is_abbrev(type, types[x]))
      break;
  }

  switch (x)
  {
  default:
    ch->sendln("Problem...fuck up in do_stat.");
    logentry(QStringLiteral("Default in do_stat...should NOT happen."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  case 0: // mobile
    if ((vict = get_mob_vis(ch, name)))
    {
      mob_stat(ch, vict);
      return eFAILURE;
    }
    ch->sendln("No such mobile.");
    return eFAILURE;
  case 1: // object
    if (!(obj = get_obj_vis(ch, name)))
    {
      ch->sendln("No such object.");
      return eFAILURE;
    }
    obj_stat(ch, obj);
    return eFAILURE;
  case 2: // character
    break;
  }

  if (!(vict = get_pc_vis(ch, name)))
  {
    c = name;
    *c = UPPER(*c);
    c++;
    while (*c)
    {
      *c = LOWER(*c);
      c++;
    }

    // must be done to clear out "d" before it is used
    if (!(ch->getDC()->load_char_obj(&d, name)))
    {
      ch->sendln("Unable to load! (Character might not exist...)");
      return eFAILURE;
    }

    vict = d.character;
    vict->desc = 0;

    redo_hitpoints(vict);
    redo_mana(vict);
    if (!GET_TITLE(vict))
      GET_TITLE(vict) = str_dup("is a virgin");
    if (GET_CLASS(vict) == CLASS_MONK)
      GET_AC(vict) -= vict->getLevel() * 3;
    isr_set(vict);

    char_to_room(vict, ch->in_room);
    mob_stat(ch, vict);
    char_from_room(vict);
    free_char(vict, Trace("do_stat"));
    return eSUCCESS;
    ;
  }

  mob_stat(ch, vict);
  return eSUCCESS;
}

int do_mpstat(Character *ch, char *arg, cmd_t cmd)
{
  Character *vict;
  char name[MAX_INPUT_LENGTH];
  int x;

  void mpstat(Character * ch, Character * victim);

  if (IS_NPC(ch))
    return eFAILURE;

  if (!ch->has_skill(COMMAND_MPSTAT))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  //  int has_range = ch->has_skill( COMMAND_RANGE);

  one_argument(arg, name);

  if (!*name)
  {
    ch->sendln("Usage:  procstat <name|num>");
    return eFAILURE;
  }

  if (isdigit(*name))
  {
    if (!(x = atoi(name)))
    {
      ch->sendln("That is not a valid number.");
      return eFAILURE;
    }
    x = real_mobile(x);

    if (x < 0)
    {
      ch->sendln("No mob of that number.");
      return eFAILURE;
    }
  }
  else
  {
    if (!(vict = get_mob_vis(ch, name)))
    {
      ch->sendln("No such mobile.");
      return eFAILURE;
    }
    x = vict->mobdata->nr;
  }
  /*
    if(!has_range)
    {
      if(!can_modify_mobile(ch, DC::getInstance()->mob_index[x].virt)) {
        ch->sendln("You are unable to work creation outside of your range.");
        return eFAILURE;
      }
    }
  */
  mpstat(ch, (Character *)DC::getInstance()->mob_index[x].item);
  return eSUCCESS;
}

command_return_t zedit_flags(Character *ch, QStringList arguments, Zone &zone)
{
  if (arguments.isEmpty())
  {
    ch->sendln("$3Usage$R: zedit flags <noteleport|noclaim|nohunt>");
    ch->send(QStringLiteral("Current flags: %1\r\n").arg(sprintbit(zone.getZoneFlags(), Zone::zone_bits)));
    return eFAILURE;
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
    ch->send(QStringLiteral("'%1' invalid.  Enter 'noclaim', 'noteleport' or 'nohunt'.\r\n").arg(text));
    return eFAILURE;
  }
  return eSUCCESS;
}

command_return_t zedit_lifetime(Character *ch, QStringList arguments, Zone &zone)
{
  if (arguments.isEmpty())
  {
    send_to_char("$3Usage$R: zedit lifetime <tickamount>\r\n"
                 "The lifetime is the number of ticks the zone takes\r\n"
                 "before it will attempt to repop itself.\r\n",
                 ch);
    return eFAILURE;
  }
  QString text = arguments.value(0);
  bool ok = false;
  uint64_t ticks = text.toULongLong(&ok);

  if (!ok || ticks > 32000)
  {
    ch->sendln("You much choose between 1 and 32000.");
    return eFAILURE;
  }

  ch->send(QStringLiteral("Zone %1's lifetime changed from %2 to %3.\r\n").arg(zone.getID()).arg(zone.lifespan).arg(ticks));
  zone.lifespan = ticks;

  return eSUCCESS;
}

command_return_t zedit_edit(Character *ch, QStringList arguments, Zone &zone)
{
  if (arguments.size() < 3 || arguments.at(0).isEmpty())
  {
    send_to_char("$3Usage$R:  zedit edit <cmdnumber> <type|if|1|2|3|comment> <value>\r\n"
                 "Valid types are:   'M', 'O', 'P', 'G', 'E', 'D', '*', 'X', 'K', and '%'.\r\n"
                 "Valid ifs are:     0(always), 1(ontrue), 2(onfalse), 3(onboot).\r\n"
                 "Valid args (123):  0-32000\r\n"
                 "                   (for max-in-world, -1 represents 'always')\r\n",
                 ch);
    return eFAILURE;
  }

  QString text = arguments.at(0);
  bool ok = false;
  uint64_t cmd = getZoneCommandKey(ch, zone, text, &ok);
  if (!ok)
  {
    return eFAILURE;
  }

  QString select = arguments.at(1);
  QString last = arguments.at(2);
  uint64_t i = 0, j = 0;

  if (!select.isEmpty() && !last.isEmpty() && cmd >= 0)
  {
    if (isexact(select, "type"))
    {
      char result = last.at(0).toUpper().toLatin1();

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
        zone.cmd[cmd]->arg1 = 0;
        zone.cmd[cmd]->arg2 = 0;
        zone.cmd[cmd]->arg3 = 0;
        /* no break */
      case '%':
        zone.cmd[cmd]->arg2 = 100;
        zone.cmd[cmd]->command = result;
        ch->send(QStringLiteral("Type for command %1 changed to %2.\r\nArg1-3 reset.\r\n").arg(cmd + 1).arg(result));
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
        zone.cmd[cmd]->if_flag = 0;
        ch->send(QStringLiteral("If flag for command %1 changed to 0 (always).\r\n").arg(cmd + 1));
        break;
      case '1':
        zone.cmd[cmd]->if_flag = 1;
        ch->send(QStringLiteral("If flag for command %1 changed to 1 ($B$2ontrue$R).\r\n").arg(cmd + 1));
        break;
      case '2':
        zone.cmd[cmd]->if_flag = 2;
        ch->send(QStringLiteral("If flag for command %1 changed to 2 ($B$4onfalse$R).\r\n").arg(cmd + 1));
        break;
      case '3':
        zone.cmd[cmd]->if_flag = 3;
        ch->send(QStringLiteral("If flag for command %1 changed to 3 ($B$5onboot$R).\r\n").arg(cmd + 1));
        break;
      case '4':
        zone.cmd[cmd]->if_flag = 4;
        ch->send(QStringLiteral("If flag for command %1 changed to 4 if-last-mob-true ($B$2Ls$1Mb$2Tr$R).\r\n").arg(cmd + 1));
        break;
      case '5':
        zone.cmd[cmd]->if_flag = 5;
        ch->send(QStringLiteral("If flag for command %1 changed to 5 if-last-mob-false ($B$4Ls$1Mb$4Fl$R).\r\n").arg(cmd + 1));
        break;
      case '6':
        zone.cmd[cmd]->if_flag = 6;
        ch->send(QStringLiteral("If flag for command %1 changed to 6 if-last-obj-true ($B$2Ls$7Ob$2Tr$R).\r\n").arg(cmd + 1));
        break;
      case '7':
        zone.cmd[cmd]->if_flag = 7;
        ch->send(QStringLiteral("If flag for command %1 changed to 7 if-last-obj-false ($B$4Ls$7Ob$4Fl$R).\r\n").arg(cmd + 1));
        break;
      case '8':
        zone.cmd[cmd]->if_flag = 8;
        ch->send(QStringLiteral("If flag for command %1 changed to 8 if-last-%%-true ($B$2Ls$R%%%%$B$2Tr$R).\r\n").arg(cmd + 1));
        break;
      case '9':
        zone.cmd[cmd]->if_flag = 9;
        ch->send(QStringLiteral("If flag for command %1 changed to 9 if-last-%%-false ($B$4Ls$R%%%%$B$4Fl$R).\r\n").arg(cmd + 1));
        break;
      default:
        ch->send("Legal values are 0 (always), 1 (ontrue), 2 (onfalse), 3 (onboot),\r\n"
                 "                 4 (if-last-mob-true),   5 (if-last-mob-false),\r\n"
                 "                 6 (if-last-obj-true),   7 (if-last-obj-false),\r\n"
                 "                 8 (if-last-%%-true),    9 (if-last-%%-false)\r\n");
        return eFAILURE;
      }
    }
    else if (isexact(select, "comment"))
    {
      //      This is str_hsh'd, don't delete it
      //      if(zone.cmd[cmd]->comment)
      //        delete zone.cmd[cmd]->comment;
      if (last == "none")
      {
        zone.cmd[cmd]->comment = {};
        ch->send(QStringLiteral("Comment for command %d removed.\r\n").arg(cmd + 1));
      }
      else
      {
        zone.cmd[cmd]->comment = last;
        ch->send(QStringLiteral("Comment for command %1 change to '%2'.\r\n").arg(cmd + 1).arg(zone.cmd[cmd]->comment));
      }
    }
    else
    {
      bool ok = false;
      i = last.toULongLong(&ok);
      if (!ok)
      {
        ch->sendln("That was not a valid number for an argument.");
        return eFAILURE;
      }

      room_t vnum = 0;
      uint_fast8_t argument_number = 0;
      decltype(zone.cmd[cmd]->arg1) original_value = 0, new_value = 0;
      QString change_type;
      if (isexact(select, "1"))
      {
        argument_number = 1;
        switch (zone.cmd[cmd]->command)
        {
        case 'M':
          j = real_mobile(i);
          original_value = DC::getInstance()->mob_index[zone.cmd[cmd]->arg1].virt;
          break;
        case 'P':
        case 'G':
        case 'O':
        case 'E':
          j = real_object(i);
          original_value = DC::getInstance()->obj_index[zone.cmd[cmd]->arg1].virt;
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
          return eFAILURE;
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
          return eFAILURE;
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
        return eFAILURE;
      }

      if (!change_type.isEmpty())
      {
        change_type += " ";
      }
      ch->send(QStringLiteral("Zone %1, command %2, argument %3 changed from %4%5 to %6.\r\n").arg(zone.getID()).arg(cmd + 1).arg(argument_number).arg(change_type).arg(original_value).arg(new_value));
    }
    return eFAILURE;
  }

  return eFAILURE;
}

command_return_t zedit_remove(Character *ch, QStringList arguments, Zone &zone)
{

  if (arguments.size() < 1)
  {
    send_to_char("$3Usage$R: zedit remove <zonecmdnumber>\r\n"
                 "This will remove the command from the zonefile.\r\n",
                 ch);
    return eFAILURE;
  }

  QString text = arguments.at(0);
  bool ok = false;

  uint64_t zone_command_number = getZoneCommandKey(ch, zone, text, &ok);
  if (!ok)
  {
    return eFAILURE;
  }

  zone.cmd.remove(zone_command_number);

  ch->send(QStringLiteral("Command %1 removed.\r\n").arg(zone_command_number + 1));
  return eSUCCESS;
}

qsizetype zone_get_last_command(const Zone &zone)
{
  return zone.cmd.size();
}

zone_t zedit_add(Character *ch, QStringList arguments, Zone &zone)
{
  if (arguments.size() < 1)
  {
    send_to_char("$3Usage$R: zedit add <new>\r\n"
                 "       zedit add <command number>\r\n"
                 "Adding 'new' will add a command to the end.\r\n"
                 "Adding a number, will insert a new command at that place in\r\n"
                 "the list, pushing the rest of the items back.\r\n",
                 ch);
    return eFAILURE;
  }

  QString text = arguments.at(0);
  if (isexact(text, "new"))
  {
    zone.cmd.push_back(QSharedPointer<ResetCommand>::create('J'));
    ch->send(QStringLiteral("New command 'J' added at %1.\r\n").arg(zone.cmd.size()));
    return zone.cmd.size() - 1;
  }

  bool ok = false;
  uint64_t i = getZoneCommandKey(ch, zone, text, &ok);
  if (!ok)
  {
    return eFAILURE;
  }

  zone.cmd.insert(i, QSharedPointer<ResetCommand>::create('J'));
  ch->send(QStringLiteral("New command 'J' added at %1.\r\n").arg(i + 1));
  return i - 1;
}

command_return_t zedit_list(Character *ch, QStringList arguments, const Zone &zone, bool stats)
{
  if (arguments.isEmpty())
  {
    return show_zone_commands(ch, zone, 0, 0, stats);
  }

  QString text = arguments.at(0);

  // list 1
  bool ok = false;
  uint64_t command_number = getZoneCommandKey(ch, zone, text, &ok);
  if (!ok)
  {
    return eFAILURE;
  }

  uint64_t num_to_show = 1;
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

  return eSUCCESS; // so we don't set_zone_modified_zone
}

command_return_t zedit_name(Character *ch, QStringList arguments, Zone &zone)
{
  if (arguments.isEmpty())
  {
    send_to_char("$3Usage$R: zedit name <newname>\r\n"
                 "This changes the name of the zone.\r\n",
                 ch);
    return eFAILURE;
  }

  zone.Name(arguments.join(' '));

  ch->send(QStringLiteral("Zone %1's name changed to '%2'.\r\n").arg(zone.getID()).arg(zone.Name()));

  return eSUCCESS;
}

command_return_t zedit_mode(Character *ch, QStringList arguments, Zone &zone)
{
  if (arguments.isEmpty())
  {
    send_to_char("$3Usage$R: zedit mode <modetype>\r\n"
                 "You much choose the rule the zone follows when it\r\n"
                 "attempts to repop itself.  Available modes are:\r\n",
                 ch);
    QString buffer;
    for (uint64_t j = 0; *zone_modes[j] != '\n'; j++)
    {
      ch->send(QStringLiteral("  $C%1$R: %2\r\n").arg(j + 1).arg(zone_modes[j]));
    }
    return eFAILURE;
  }

  QString text = arguments.value(0);
  bool ok = false;
  uint64_t k = text.toULongLong(&ok);

  if (!ok || k > 3)
  {
    ch->sendln("You must choose between 1 and 3.");
    return eFAILURE;
  }

  zone.reset_mode = k - 1;

  ch->send(QStringLiteral("Zone %1's reset mode changed to %2(%3).\r\n").arg(zone.getID()).arg(zone_modes[k - 1]).arg(k));
  return eSUCCESS;
}

const QStringList zedit_subcommands = {
    "remove", "add", "edit", "list", "name",
    "lifetime", "mode", "flags", "help", "search",
    "swap", "copy", "continent", "info"};

command_return_t zedit_help(Character *ch)
{
  send_to_char("$3Usage$R: zedit <subcommand> [arguments]\r\n"
               "       zedit <zone number> <subcommand> [arguments]\r\n"
               "If you don't specify the zone number then whatever zone you are in is used.\r\n"
               "Subcommands:\r\n",
               ch);
  ch->display_string_list(zedit_subcommands);
  return eSUCCESS;
}

int do_zedit(Character *ch, char *argument, cmd_t cmd)
{
  QString buf, text, arg;
  uint64_t i = 0, j = 0, num_to_show = 0, last_cmd = 0, ticks = 0, cont = 0;
  unsigned int k = 0;
  vnum_t robj = {}, rmob = {};
  bool ok = false;
  QSharedPointer<ResetCommand> tmp = {}, temp_com = {};
  char *str = {};

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
    zone_key = DC::getInstance()->world[ch->in_room].zone;
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

  bool subcommand_found{};
  uint64_t subcommand_id{};
  for (subcommand_id = 0; subcommand_id < zedit_subcommands.size(); subcommand_id++)
  {
    if (is_abbrev(select, zedit_subcommands[subcommand_id]))
    {
      subcommand_found = true;
      break;
    }
  }

  if (!subcommand_found)
  {
    ch->send(QStringLiteral("'%1' is an invalid subcommand.\r\n").arg(select));
    return zedit_help(ch);
  }

  if (!can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You are unable to modify a zone other than the one your room-range is in.");
    return eFAILURE;
  }
  uint64_t from = {}, to = {};

  // set the zone we're in

  if (!isValidZoneKey(ch, zone_key))
  {
    return eFAILURE;
  }

  Zone &zone = DC::getInstance()->zones[zone_key];

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
                 "   X = set a true-false flag to an 'unsure' state\n\r"
                 "   K = skip the next [arg1] number of commands.\r\n"
                 "\r\n"
                 "For comments, if you wish to remove a comment set the comment to 'none'.\r\n"
                 "\r\n",
                 ch);
    return eSUCCESS; // so we don't set modified
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
      return eFAILURE;
    }

    text = arguments.at(0);
    ok = false;
    j = text.toULongLong(&ok);

    if (!ok)
    {
      ch->sendln("Please specifiy a valid number.");
      return eFAILURE;
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
      ch->send(QStringLiteral("Searching for Object rnum %1 with vnum %2\r\n").arg(robj).arg(j));
    }
    if (rmob == -1)
    {
      rmob = -93294;
    }
    else
    {
      ch->send(QStringLiteral("Searching for Mobile rnum %1 with vnum %2\r\n").arg(rmob).arg(j));
    }

    for (const auto [z_key, zone] : DC::getInstance()->zones.asKeyValueRange())
    {
      for (i = 0; i < zone.cmd.size(); i++)
      {
        switch (zone.cmd[i]->command)
        {
        case 'M':
          if (rmob == zone.cmd[i]->arg1)
          {
            csendf(ch, " Zone %d  Command %d (%c)\r\n", z_key, i + 1, zone.cmd[i]->command);
            if (stats)
            {
              str = str_dup(QStringLiteral(" %1 list %2 1 stats\r\n").arg(z_key).arg(i + 1).toStdString().c_str());
            }
            else
            {
              str = str_dup(QStringLiteral(" %1 list %2 1\r\n").arg(z_key).arg(i + 1).toStdString().c_str());
            }
            do_zedit(ch, str);
            delete[] str;
          }
          break;
        case 'G': // G, E, and O have obj # in arg1
        case 'E':
        case 'O':
          if (robj == zone.cmd[i]->arg1)
          {
            csendf(ch, " Zone %d  Command %d (%c)\r\n", z_key, i + 1, zone.cmd[i]->command);
            if (stats)
            {
              str = str_dup(QStringLiteral(" %1 list %2 1 stats\r\n").arg(z_key).arg(i + 1).toStdString().c_str());
            }
            else
            {
              str = str_dup(QStringLiteral(" %1 list %2 1\r\n").arg(z_key).arg(i + 1).toStdString().c_str());
            }

            do_zedit(ch, str);
            delete[] str;
          }
          break;
        case 'P': // P has obj # in arg1 and arg3
          if (robj == zone.cmd[i]->arg1 ||
              robj == zone.cmd[i]->arg3)
          {
            csendf(ch, " Zone %d  Command %d (%c)\r\n", z_key, i + 1, zone.cmd[i]->command);
            if (stats)
            {
              str = str_dup(QStringLiteral(" %1 list %2 1 stats\r\n").arg(z_key).arg(i + 1).toStdString().c_str());
            }
            else
            {
              str = str_dup(QStringLiteral(" %1 list %2 1\r\n").arg(z_key).arg(i + 1).toStdString().c_str());
            }

            do_zedit(ch, str);
            delete[] str;
          }
          break;
        default:
          break;
        }
      }
    }
    return eSUCCESS;
    break;

  case 10: // swap

    if (arguments.size() < 2)
    {
      send_to_char("$3Usage$R: zedit swap <cmd1> <cmd2>\r\n"
                   "This swaps the positions of two zone commands.\r\n",
                   ch);
      return eFAILURE;
    }
    select = arguments.at(0);
    text = arguments.at(1);

    ok = false;
    i = select.toULongLong(&ok);
    if (!ok)
    {
      ch->sendln("Invalid command num for cmd1.");
      return eFAILURE;
    }

    ok = false;
    j = text.toULongLong(&ok);
    if (!ok)
    {
      ch->sendln("Invalid command num for cmd2.");
      return eFAILURE;
    }

    // swap i and j
    temp_com = {};

    temp_com = zone.cmd[i - 1];
    zone.cmd[i - 1] = zone.cmd[j - 1];
    zone.cmd[j - 1] = temp_com;

    csendf(ch, "Commands %d and %d swapped.\r\n", i, j);
    break;

  case 11: // copy

    if (arguments.size() < 1)
    {
      ch->send("$3Usage$R: zedit copy <source line> <destination line>\r\nDestination line is optional. If no such line exists, it tacks it on at the end.");
      return eFAILURE;
    }
    text = arguments.at(0);
    arg = arguments.at(1);

    ok = false;
    from = text.toLongLong(&ok) - 1;
    to = 0;
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
      return eFAILURE;
    }

    tmp = zone.cmd[from];
    if (to)
    {
      // bump everything up a slot
      for (j = last_cmd; j != (to - 2); j--)
        zone.cmd[j + 1] = zone.cmd[j];
      buf = QStringLiteral("Command copied to %1.\r\n").arg(to);
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

        csendf(ch, "%d) %s\n\r", cont, continent_names.at(cont).c_str());
      }
      return eFAILURE;
    }
    text = arguments.at(0);
    ok = false;
    cont = text.toLongLong(&ok);
    if (!ok || !cont || cont > continent_names.size() - 1)
    {
      csendf(ch, "You much choose between 1 and %d.\r\n", continent_names.size());
      return eFAILURE;
    }
    csendf(ch, "Success. Continent changed to %s\n\r", continent_names.at(cont).c_str());
    zone.continent = cont;
    break;

  case 13:
    zone.show_info(ch);
    return eSUCCESS;
    break;

  default:
    ch->sendln("Error:  Couldn't find item in switch.");
    return eFAILURE;
    break;
  }
  DC::getInstance()->set_zone_modified_zone(ch->in_room);
  return eSUCCESS;
}

int do_sedit(Character *ch, char *argument, cmd_t cmd)
{
  std::string buf;
  std::string select;
  std::string target;
  std::string text;
  std::string value;
  Character *vict = nullptr;
  char_skill_data *skill = nullptr;
  char_skill_data *lastskill = nullptr;
  int16_t field = 0;
  int16_t skillnum = 0;
  int16_t learned = 0;
  int i = 0;

  const char *sedit_values[] = {
      "add",
      "remove",
      "set",
      "list",
      "\n"};

  if (!ch->has_skill(COMMAND_SEDIT))
  {
    ch->sendln("Huh?");
    return eFAILURE;
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
    return eFAILURE;
  }

  if (!(vict = get_char_vis(ch, target.c_str())))
  {
    ch->send(fmt::format("Cannot find player '{}'.\r\n", target));
    return eFAILURE;
  }

  if (vict->getLevel() > ch->getLevel())
  {
    ch->send("You like to play dangerously don't you....\r\n");
    return eFAILURE;
  }

  field = old_search_block(select.c_str(), 0, select.length(), sedit_values, 1);
  if (field < 0)
  {
    ch->sendln("That field not recognized.");
    return eFAILURE;
  }
  field--;

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
      return eFAILURE;
    }

    if (results.size() > 1)
    {
      ch->send(fmt::format("Skill '{}' is too ambiguous. Please specify one of the following:\r\n", text));
      for (const auto &result : results)
      {
        ch->send(fmt::format("{}\r\n", result.first));
      }
      ch->send("\r\n");

      return eFAILURE;
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

      return eFAILURE;
    }

    if (vict->has_skill(skillnum))
    {
      ch->send(fmt::format("'{}' already has '{}'.\r\n", GET_NAME(vict), text));
      return eFAILURE;
    }

    vict->learn_skill(skillnum, 1, 1);

    buf = fmt::format("'{}' has been given skill '{}' ({}) by {}.", GET_NAME(vict), text, skillnum, GET_NAME(ch));
    logentry(buf.c_str(), ch->getLevel(), DC::LogChannel::LOG_GOD);
    ch->send(fmt::format("'{}' has been given skill '{}' ({}) by {}.\r\n", GET_NAME(vict), text, skillnum, GET_NAME(ch)));
    break;
  }
  case 1: /* remove */
  {
    if (text.empty())
    {
      ch->send("$3Usage$R: sedit <character> remove <skillname>\r\n"
               "This will remove the skill from the character.\r\n");
      return eFAILURE;
    }

    if (ch->skills.contains(skillnum))
    {
      ch->skills.erase(skillnum);

      buf = fmt::format("Skill '{}' ({}) removed from {} by {}.", text, skillnum, GET_NAME(vict), GET_NAME(ch));
      logentry(buf.c_str(), ch->getLevel(), DC::LogChannel::LOG_GOD);
      ch->send(fmt::format("Skill '{}' ({}) removed from {}.\r\n", text, skillnum, GET_NAME(vict)));
    }
    else
    {
      ch->send(fmt::format("Cannot find skill '{}' on '{}'.\r\n", text, GET_NAME(vict)));
      return eFAILURE;
    }
    break;
  }
  case 2: /* set */
  {
    if (text.empty())
    {
      ch->send("$3Usage$R: sedit <character> set <skillname> <amount>\r\n"
               "This will set the character's skill to amount.\r\n");
      return eFAILURE;
    }
    if (!(learned = vict->has_skill(skillnum)))
    {
      ch->send(fmt::format("'{}' does not have skill '{}'.\r\n", GET_NAME(vict), text));
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(i, value.c_str(), 1, 100))
    {
      ch->sendln("Invalid skill amount.  Must be 1 - 100.");
      return eFAILURE;
    }

    vict->learn_skill(skillnum, i, i);

    buf = fmt::format("'{}'s skill '{}' set to {} from {} by {}.", GET_NAME(vict), text, i, learned, GET_NAME(ch));
    logentry(buf.c_str(), ch->getLevel(), DC::LogChannel::LOG_GOD);
    ch->send(fmt::format("'{}' skill '{}' set to {} from {}.\r\n", GET_NAME(vict), text, i, learned));
    break;
  }
  case 3: /* list */
  {
    i = 0;
    ch->send(fmt::format("$3Skills for$R:  {}\r\n"
                         "  {:<18}  {:<4}  Learned\r\n"
                         "$3-------------------------------------$R\r\n",
                         GET_NAME(vict), "Skill", "#"));
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
    return eSUCCESS;
    break;
  }

  default:
    ch->send("Error:  Couldn't find item in switch (sedit).\r\n");
    break;
  }

  // make sure the changes stick
  vict->save(cmd_t::SAVE_SILENTLY);

  return eSUCCESS;
}

int oedit_exdesc(Character *ch, int item_num, char *buf)
{
  char type[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
  char select[MAX_INPUT_LENGTH];
  char value[MAX_INPUT_LENGTH];
  int x;
  Object *obj = nullptr;
  int num;

  extra_descr_data *curr = nullptr;
  extra_descr_data *curr2 = nullptr;

  const char *fields[] =
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

  obj = (Object *)DC::getInstance()->obj_index[item_num].item;

  if (!*buf)
  {
    send_to_char("$3Syntax$R:  oedit [item_num] exdesc <field> [values]\r\n"
                 "The field must be one of the following:\n\r",
                 ch);
    ch->display_string_list(fields);
    ch->sendln("\n\r$3Current Descs$R:");
    for (x = 1, curr = obj->ex_description; curr; x++, curr = curr->next)
      csendf(ch, "$3%d$R) %s\n\r%s\n\r", x, curr->keyword, curr->description);
    return eFAILURE;
  }

  for (x = 0;; x++)
  {
    if (fields[x][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return eFAILURE;
    }
    if (is_abbrev(type, fields[x]))
      break;
  }

  switch (x)
  {
  // new
  case 0:
  {
    if (!*select)
    {
      send_to_char("$3Syntax$R: oedit [item_num] exdesc new <keywords>\r\n"
                   "This adds a new description with the keywords chosen.\r\n",
                   ch);
      return eFAILURE;
    }
    curr = new extra_descr_data;
    curr->keyword = str_hsh(select);
    curr->description = str_hsh("Empty desc.\r\n");
    curr->next = obj->ex_description;
    obj->ex_description = curr;
    ch->sendln("New desc created.");
    break;
  }

  // delete
  case 1:
  {
    if (!*select)
    {
      send_to_char("$3Syntax$R: oedit [item_num] exdesc delete <number>\r\n"
                   "This removes desc <number> from the list permanently.\r\n",
                   ch);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(num, select, 1, 50))
    {
      ch->sendln("You must select a valid number.");
      return eFAILURE;
    }
    x = 1;
    curr2 = nullptr;
    for (curr = obj->ex_description; x < num && curr; curr = curr->next)
    {
      curr2 = curr;
      x++;
    }

    if (!curr)
    {
      ch->sendln("There is no desc for that number.");
      return eFAILURE;
    }

    if (!curr2)
    { // first one
      obj->ex_description = curr->next;
      delete curr;
    }
    else
    {
      curr2->next = curr->next;
      delete curr;
    }
    ch->sendln("Deleted.");
    break;
  }

  // keywords
  case 2:
  {
    if (!*select || !*value)
    {
      send_to_char("$3Syntax$R: oedit [item_num] exdesc keywords <number> <new keywords>\r\n"
                   "This removes desc <number> from the list permanently.\r\n",
                   ch);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(num, select, 1, 50))
    {
      ch->sendln("You must select a valid number.");
      return eFAILURE;
    }
    for (curr = obj->ex_description, x = 1; x < num && curr; curr = curr->next)
      x++;

    if (!curr)
    {
      ch->sendln("There is no desc for that number.");
      return eFAILURE;
    }
    curr->keyword = str_hsh(value);
    ch->sendln("New keyword set.");
    break;
  }

  // desc
  case 3:
  {
    if (!*select)
    {
      send_to_char("$3Syntax$R: oedit [item_num] exdesc desc <number>\r\n"
                   "This removes desc <number> from the list permanently.\r\n",
                   ch);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(num, select, 1, 50))
    {
      ch->sendln("You must select a valid number.");
      return eFAILURE;
    }
    for (curr = obj->ex_description, x = 1; x < num && curr; curr = curr->next)
      x++;

    if (!curr)
    {
      ch->sendln("There is no desc for that number.");
      return eFAILURE;
    }
    ch->sendln("        Write your obj's description.  (/s saves /h for help)");

    //        send_to_char("Enter your obj's description below."
    //                   " Terminate with '~' on a new line.\n\r\n\r", ch);
    //        curr->description = 0;

    ch->desc->connected = Connection::states::EDITING;
    ch->desc->strnew = &(curr->description);
    ch->desc->max_str = MAX_MESSAGE_LENGTH;

    break;
  }

  default:
    ch->sendln("Illegal value, tell someone.");
    break;
  } // switch(x)

  DC::getInstance()->set_zone_modified_obj(item_num);
  return eSUCCESS;
}

int oedit_affects(Character *ch, int item_num, char *buf)
{
  char type[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
  char select[MAX_INPUT_LENGTH];
  char value[MAX_INPUT_LENGTH];
  int x;
  Object *obj = nullptr;
  int num;
  int modifier;

  const char *fields[] =
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

  if (!*buf)
  {
    send_to_char("$3Syntax$R:  oedit [item_num] affects [affectnumber] [value]\r\n"
                 "The field must be one of the following:\n\r",
                 ch);
    ch->display_string_list(fields);
    return eFAILURE;
  }

  for (x = 0;; x++)
  {
    if (fields[x][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return eFAILURE;
    }
    if (is_abbrev(type, fields[x]))
      break;
  }

  obj = (Object *)DC::getInstance()->obj_index[item_num].item;

  switch (x)
  {
  // new
  case 0:
  {
    if (!*select)
    {
      send_to_char("$3Syntax$R: oedit [item_num] affects new yes\r\n"
                   "This adds a new blank affect to the end of the list.\r\n",
                   ch);
      return eFAILURE;
    }
    add_obj_affect(obj, 0, 0);
    ch->sendln("New affect created.");
    break;
  }

  // delete
  case 1:
  {
    if (!*select)
    {
      send_to_char("$3Syntax$R: oedit [item_num] affects delete <number>\r\n"
                   "This removes affect <number> from the list permanently.\r\n",
                   ch);
      return eFAILURE;
    }
    if (obj->affected.isEmpty())
    {
      sprintf(buf, "Object %d has no affects to delete.\r\n", DC::getInstance()->obj_index[item_num].virt);
      ch->send(buf);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert<decltype(num)>(num, select, 1, obj->affected.size()))
    {
      sprintf(buf, "You must select between 1 and %d.\r\n", obj->affected.size());
      ch->send(buf);
      return eFAILURE;
    }
    remove_obj_affect_by_index(obj, num - 1);
    ch->sendln("Affect deleted.");
    break;
  }

  // list
  case 2:
  {
    if (obj->affected.isEmpty())
    {
      ch->sendln("The object has no affects.");
      return eSUCCESS;
    }
    send_to_char("$3Character Affects$R:\r\n"
                 "------------------\r\n",
                 ch);
    for (x = 0; x < obj->affected.size(); x++)
    {
      //          sprinttype(obj->affected[x].location, apply_types, buf2);

      if (obj->affected[x].location < 1000)
        sprinttype(obj->affected[x].location, apply_types, buf2);
      else if (!get_skill_name(obj->affected[x].location / 1000).isEmpty())
        strcpy(buf2, get_skill_name(obj->affected[x].location / 1000).toStdString().c_str());

      sprintf(buf, "%2d$3)$R %s$3($R%d$3)$R by %d.\r\n", x + 1, buf2,
              obj->affected[x].location, obj->affected[x].modifier);
      ch->send(buf);
    }
    return eSUCCESS; // return so we don't mark as changed
    break;
  }

  // 1spell
  case 3:
  {
    if (!*select)
    {
      send_to_char("$3Syntax$R: oedit [item_num] affects 1spell <number> <value>\r\n"
                   "This sets the modifying spell affect to <value> for affect <number>.\r\n",
                   ch);
      for (x = 0; x <= APPLY_MAXIMUM_VALUE; x++)
      {
        sprintf(buf, "%3d$3)$R %s\r\n", x, apply_types[x]);
        ch->send(buf);
      }
      ch->sendln("Make $B$5sure$R you don't use a spell that is restricted.  See builder guide.");
      return eFAILURE;
    }
    if (obj->affected.isEmpty())
    {
      sprintf(buf, "Object %d has no affects to modify.\r\n", DC::getInstance()->obj_index[item_num].virt);
      ch->send(buf);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert<decltype(num)>(num, select, 1, obj->affected.size()))
    {
      sprintf(buf, "You must select between 1 and %d.\r\n", obj->affected.size());
      ch->send(buf);
      return eFAILURE;
    }
    num -= 1; // since arrays start at 0
    if (!check_range_valid_and_convert(modifier, value, APPLY_NONE, APPLY_MAXIMUM_VALUE))
    {
      sprintf(buf, "You must select between %d and %d.\r\n", APPLY_NONE, APPLY_MAXIMUM_VALUE);
      ch->send(buf);
      return eFAILURE;
    }
    obj->affected[num].location = modifier;
    sprintf(buf, "Affect %d changed to %s by %d.\r\n", num + 1,
            apply_types[obj->affected[num].location], obj->affected[num].modifier);
    ch->send(buf);
    break;
  }

  // 2amount
  case 4:
  {
    if (!*select)
    {
      send_to_char("$3Syntax$R: oedit [item_num] affects 1amount <number> <value>\r\n"
                   "This sets the spell affect's modifier to <value> for affect <number>.\r\n"
                   "Currently limited from -100 to 100.\r\n",
                   ch);
      return eFAILURE;
    }
    if (obj->affected.isEmpty())
    {
      sprintf(buf, "Object %d has no affects to modify.\r\n",
              DC::getInstance()->obj_index[item_num].virt);
      ch->send(buf);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert<decltype(num)>(num, select, 1, obj->affected.size()))
    {
      sprintf(buf, "You must select between 1 and %d.\r\n",
              obj->affected.size());
      ch->send(buf);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(modifier, value, -100, 100))
    {
      ch->sendln("You must select between -100 and 100.");
      return eFAILURE;
    }
    num -= 1; // since arrays start at 0
    obj->affected[num].modifier = modifier;
    if (obj->affected[num].location >= 1000)
    {
      QString skill_name = get_skill_name(obj->affected[num].location / 1000);

      ch->send(QStringLiteral("Affect %1 changed to %2 by %3.\r\n").arg(num + 1).arg(skill_name).arg(obj->affected[num].modifier));
    }
    else
    {
      ch->send(QStringLiteral("Affect %1 changed to %2 by %3.\r\n").arg(num + 1).arg(apply_types[obj->affected[num].location]).arg(obj->affected[num].modifier));
    }
    break;
  }
  case 5:
    if (!*select || !*value)
    {
      send_to_char("$3Syntax$R: oedit [item_num] affects 3spell <number> <skill>\r\n"
                   "This sets the affect as affecting skills by 2amount\r\n",
                   ch);
      return eFAILURE;
    }
    if (obj->affected.isEmpty())
    {
      sprintf(buf, "Object %d has no affects to modify.\r\n",
              DC::getInstance()->obj_index[item_num].virt);
      ch->send(buf);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert<decltype(num)>(num, select, 1, obj->affected.size()))
    {
      sprintf(buf, "You must select between 1 and %d.\r\n",
              obj->affected.size());
      ch->send(buf);
      return eFAILURE;
    }
    modifier = atoi(value);
    num -= 1;
    obj->affected[num].location = modifier * 1000;
    ch->send(QStringLiteral("Affect %1 changed to %2 by %3.\r\n").arg(num + 1).arg(get_skill_name(obj->affected[num].location / 1000)).arg(obj->affected[num].modifier));
    break;
  default:
    ch->sendln("Illegal value, tell pir.");
    break;
  } // switch(x)

  DC::getInstance()->set_zone_modified_obj(item_num);
  return eSUCCESS;
}

command_return_t Character::do_oedit(QStringList arguments, cmd_t cmd)
{
  char buf[MAX_INPUT_LENGTH] = {};
  char buf2[MAX_INPUT_LENGTH] = {};
  char buf3[MAX_INPUT_LENGTH] = {};
  char buf4[MAX_INPUT_LENGTH] = {};
  int rnum = {};
  vnum_t vnum = {};
  int intval = {};
  int x = {}, i = {};

  const char *fields[] =
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

  if (IS_NPC(this))
    return eFAILURE;

  half_chop(qPrintable(arguments.join(' ')), buf, buf2);
  half_chop(buf2, buf3, buf4);

  // at this point, buf  = item_num
  //                buf3 = field
  //                buf4 = args

  // or

  // buf = field
  // buf3 = args[0]
  // buf4 = args[1-+]

  if (!*buf)
  {
    send_to_char("$3Syntax$R:  oedit new [obj vnum]           -- Create new object\n\r"
                 "         oedit [obj vnum]               -- Stat object\n\r"
                 "         oedit [obj vnum] [field]       -- Help info for that field\n\r"
                 "         oedit [obj vnum] [field] [arg] -- Change that field\n\r"
                 "         oedit [field] [arg]            -- Change that field using last vnum\n\r\n\r"
                 "The field must be one of the following:\n\r",
                 this);
    display_string_list(fields);
    return eFAILURE;
  }

  if (isdigit(*buf))
  {
    try
    {
      vnum = std::stoull(buf);
    }
    catch (...)
    {
      vnum = 0;
    }

    rnum = real_object(vnum);
    if (rnum < 0 || vnum < 1)
    {
      sendln("Invalid item number.");
      return eSUCCESS;
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
      return eSUCCESS;
    }

    // put the buffs where they should be
    if (*buf4)
    {
      sprintf(buf2, "%s %s", buf3, buf4);
    }
    else
    {
      strcpy(buf2, buf3);
    }

    strcpy(buf4, buf2);
    strcpy(buf3, buf);
  }

  if (!*buf3) // no field.  Stat the item.
  {
    obj_stat(this, (Object *)DC::getInstance()->obj_index[rnum].item);
    return eSUCCESS;
  }

  // MOVED
  for (x = 0;; x++)
  {
    if (fields[x][0] == '\n')
    {
      sendln("Invalid field.");
      return eFAILURE;
    }
    if (is_abbrev(buf3, fields[x]))
      break;
  }

  // a this point, item_num is the index
  if (x != 18) // Checked in there
    if (!can_modify_object(this, vnum))
    {
      sendln("You are unable to work creation outside of your range.");
      return eFAILURE;
    }

  switch (x)
  {

    /* edit keywords */
  case 0:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [item_num] keywords <new_keywords>");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->Name(buf4);
    sprintf(buf, "Item keywords set to '%s'.\r\n", buf4);
    send(buf);
  }
  break;

    /* edit long desc */
  case 1:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [item_num] longdesc <new_desc>");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->long_description = str_hsh(buf4);
    sprintf(buf, "Item longdesc set to '%s'.\r\n", buf4);
    send(buf);
  }
  break;

    // edit short desc
  case 2:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [item_num] shortdesc <new_desc>");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->short_description = str_hsh(buf4);
    sprintf(buf, "Item shortdesc set to '%s'.\r\n", buf4);
    send(buf);
  }
  break;

    /* edit action desc */
  case 3:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [item_num] actiondesc <new_desc>");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->ActionDescription(buf4);
    sprintf(buf, "Item actiondesc set to '%s'.\r\n", buf4);
    send(buf);
  }
  break;

    /* edit type */
  case 4:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: oedit [item_num] type <>\n\r"
                   "$3Current$R: ",
                   this);
      snprintf(buf, sizeof(buf), "%s\n", item_types[((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.type_flag].toStdString().c_str());
      send(buf);
      sendln("\r\n$3Valid types$R:");

      for (i = 1; i < item_types.size(); i++)
      {
        send(QStringLiteral("%1) %2\r\n").arg(i, 3).arg(item_types[i]));
      }
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, ITEM_TYPE_MAX))
    {
      sendln("Value out of valid range.");
      return eFAILURE;
    }
    if (intval == 24)
    {
      ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.value[2] = -1;
    }
    else
    {
      ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.value[2] = 0;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.type_flag = intval;
    sprintf(buf, "Item type set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit wear */
  case 5:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: oedit [item_num] wear <location[s]>\n\r"
                   "$3Current$R: ",
                   this);

      sendln(QFlagsToStrings<ObjectPositions>(((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.wear_flags));
      sendln("$3Valid types$R:");
      for (i = 0; i < QFlagsToStrings<ObjectPositions>().size(); i++)
      {
        send(QStringLiteral("  %1\r\n").arg(QFlagsToStrings<ObjectPositions>().value(i)));
      }
      return eFAILURE;
    }

    ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.wear_flags = parse_bitstrings<ObjectPositions>(buf4, this, ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.wear_flags);
  }
  break;

    /* edit size */
  case 6:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: oedit [item_num] size <size[s]>\n\r"
                   "$3Current$R: ",
                   this);
      sprintbit(((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.size,
                size_bitfields, buf);
      send(buf);
      sendln("\r\n$3Valid types$R:");
      for (i = 0; *size_bitfields[i] != '\n'; i++)
      {
        sprintf(buf, "  %s\n\r", size_bitfields[i]);
        send(buf);
      }
      return eFAILURE;
    }
    parse_bitstrings_into_int(size_bitfields, buf4, this,
                              ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.size);
  }
  break;

    /* edit extra */
  case 7:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: oedit [item_num] extra <bit[s]>\n\r"
                   "$3Current$R: ",
                   this);
      sprintbit(((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.extra_flags, Object::extra_bits, buf);
      send(buf);
      sendln("\r\n$3Valid types$R:");
      for (i = 0; i < Object::extra_bits.size(); i++)
      {
        send(QStringLiteral("  %1\r\n").arg(Object::extra_bits[i]));
      }
      return eFAILURE;
    }
    parse_bitstrings_into_int(Object::extra_bits, QString(buf4), this, ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.extra_flags);
  }
  break;

    /* edit weight */
  case 8:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [item_num] weight <>");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 99999))
    {
      sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.weight = intval;
    sprintf(buf, "Item weight set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit value */
  case 9:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [item_num] value <>");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 5000000))
    {
      sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.cost = intval;
    sprintf(buf, "Item value set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit moreflags */
  case 10:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: oedit [item_num] moreflags <bit[s]>\n\r"
                   "$3Current$R: ",
                   this);
      sprintbit(((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.more_flags, Object::more_obj_bits, buf);
      send(buf);
      sendln("\r\n$3Valid types$R:");
      for (i = 0; i < Object::more_obj_bits.size(); i++)
      {
        send(QStringLiteral("  %1\r\n").arg(Object::more_obj_bits[i]));
      }
      return eFAILURE;
    }
    parse_bitstrings_into_int(Object::more_obj_bits, QString(buf4), this, ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.more_flags);
  }
  break;

    /* edit level */
  case 11:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [vnum] level <>");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 110))
    {
      sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.eq_level = intval;
    sprintf(buf, "Item minimum level set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit 1value */
  case 12:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [vnum] 1value <num>");
      return eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      sendln("Please specifiy a valid number.");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.value[0] = intval;
    sprintf(buf, "Item value 1 set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit 2value */
  case 13:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [vnum] 2value <num>");
      return eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      sendln("Please specifiy a valid number.");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.value[1] = intval;
    sprintf(buf, "Item value 2 set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit 3value */
  case 14:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [vnum] 3value <num>");
      return eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      sendln("Please specifiy a valid number.");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.value[2] = intval;
    sprintf(buf, "Item value 3 set to %d.\r\n", intval);
    send(buf);
  }
  break;

    /* edit 4value */
  case 15:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [vnum] 4value <num>");
      return eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      sendln("Please specifiy a valid number.");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.value[3] = intval;
    sprintf(buf, "Item value 4 set to %d.\r\n", intval);
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
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit new [vnum]");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 35000))
    {
      sendln("Please specifiy a valid number.");
      return eFAILURE;
    }
    /*        if (real_object(intval) <= 0)
            {
        sendln("Object already exists.");
        return eFAILURE;
      }
         */
    /*if (!has_skill( COMMAND_RANGE))
    {
      sendln("You cannot create items.");
      return eFAILURE;
    }*/
    if (!can_modify_object(this, intval))
    {
      sendln("You cannot create items in that range.");
      return eFAILURE;
    }
    /*
            if(!can_modify_object(this, intval)) {
              sendln("You are unable to work creation outside of your range.");
              return eFAILURE;
            }
    */
    auto x = getDC()->create_blank_item(intval);
    if (!x.has_value())
    {
      send(QStringLiteral("Could not create item '%1'.  Max index hit or obj already exists. %2\r\n").arg(intval).arg(QVariant::fromValue(x.error()).toString()));
      return eFAILURE;
    }
    send(QStringLiteral("Item '%1' created successfully.\r\n").arg(intval));
    break;
  }

  // delete
  case 19:
  {
    if (!*buf4 || strncmp(buf4, "yesiwanttodeletethisitem", 24))
    {
      sendln("$3Syntax$R: oedit [item_num] delete yesiwanttodeletethisitem");
      sendln("\r\nDeleting an item is $3permanent$R and will cause ALL copies of");
      sendln("that items in the world to disappear.  Logged out players will lose the");
      sendln("item upon logging in as long as no other items is created with that number.");
      sendln("(Creating a new items with that number will cause the others to remain on");
      sendln("the player.)");
      return eFAILURE;
    }

    struct vault_data *vault, *tvault;
    struct vault_items_data *items, *titems;
    class Object *obj;
    int num = 0, real_num = 0;

    for (vault = vault_table; vault; vault = tvault, num++)
    {
      tvault = vault->next;

      if (vault && vault->items)
      {
        for (items = vault->items; items; items = titems)
        {
          titems = items->next;

          real_num = real_object(items->item_vnum);
          obj = items->obj ? items->obj : ((class Object *)DC::getInstance()->obj_index[real_num].item);
          if (obj == nullptr)
            continue;

          if (obj->item_number == rnum)
          {

            void item_remove(Object * obj, struct vault_data * vault);
            item_remove(obj, vault);
            // items->obj = 0;
            logf(0, DC::LogChannel::LOG_MISC, "Removing deleted item %d from %s's vault.", vnum, vault->owner.toStdString().c_str());
          }
        }
      }
    }

    Object *next_k;
    // remove the item from players in world
    for (Object *k = DC::getInstance()->object_list; k; k = next_k)
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
    obj_stat(this, (Object *)DC::getInstance()->obj_index[rnum].item);
    return eSUCCESS;
    break;
  }
  case 21:
  {
    if (!*buf4)
    {
      sendln("$3Syntax$R: oedit [item_num] 3value <num>");
      return eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      sendln("Please specifiy a valid number.");
      return eFAILURE;
    }
    ((Object *)DC::getInstance()->obj_index[rnum].item)->obj_flags.timer = intval;
    sprintf(buf, "Item timer to %d.\r\n", intval);
    send(buf);
  }
  break;
  case 22:
    extra_descr_data *curr;
    for (curr = ((Object *)DC::getInstance()->obj_index[rnum].item)->ex_description; curr; curr = curr->next)
      if (!str_cmp(curr->keyword, qPrintable(((Object *)DC::getInstance()->obj_index[rnum].item)->Name())))
        break;
    if (!curr)
    { // None existing;
      curr = new extra_descr_data;
      curr->keyword = str_dup(qPrintable(((Object *)DC::getInstance()->obj_index[rnum].item)->Name()));
      curr->description = str_dup("");
      curr->next = ((Object *)DC::getInstance()->obj_index[rnum].item)->ex_description;
      ((Object *)DC::getInstance()->obj_index[rnum].item)->ex_description = curr;
    }
    sendln("Write your object's description. End with /s.");
    desc->connected = Connection::states::EDITING;
    desc->strnew = &(curr->description);
    desc->max_str = MAX_MESSAGE_LENGTH;
    break;
  default:
    sendln("Illegal value, tell a coder.");
    break;
  }

  DC::getInstance()->set_zone_modified_obj(rnum);
  return eSUCCESS;
}

void update_mobprog_bits(int mob_num)
{
  mob_prog_data *prog = DC::getInstance()->mob_index[mob_num].mobprogs;
  DC::getInstance()->mob_index[mob_num].progtypes = 0;

  while (prog)
  {
    SET_BIT(DC::getInstance()->mob_index[mob_num].progtypes, prog->type);
    prog = prog->next;
  }
}

int do_procedit(Character *ch, char *argument, cmd_t cmd)
{
  char buf[MAX_INPUT_LENGTH]{};
  char buf2[MAX_INPUT_LENGTH]{};
  char buf3[MAX_STRING_LENGTH]{};
  char buf4[MAX_INPUT_LENGTH]{};
  int mob_num = -1;
  int intval = 0;
  int x{}, i{};
  mob_prog_data *prog{};
  mob_prog_data *currprog{};

  void mpstat(Character * ch, Character * victim);

  const char *fields[] = {"add", "remove", "type", "arglist", "command", "list", "\n"};

  if (IS_NPC(ch))
    return eFAILURE;

  half_chop(argument, buf, buf2);
  half_chop(buf2, buf3, buf4);

  // at this point, buf  = mob_num
  //                buf3 = field
  //                buf4 = args

  // or

  // buf = field
  // buf3 = args[0]
  // buf4 = args[1-+]
  if (!*buf)
  {
    send_to_char("$3Syntax$R:  procedit [mob_vnum] [field] [arg]\r\n"
                 "  Edit a field with no args for help on that field.\r\n\r\n"
                 "  The field must be one of the following:\n\r",
                 ch);
    ch->display_string_list(fields);
    sprintf(buf2, "\n\r$3Current mob vnum set to$R: %d\n\r", ch->player->last_mob_edit);
    send_to_char(buf2, ch);
    return eFAILURE;
  }

  int mobvnum = -1;
  if (isdigit(*buf))
  {
    mobvnum = atoi(buf);
    if (((mob_num = real_mobile(mobvnum)) < 0) || (mobvnum == 0 && *buf != '0'))
    {
      ch->send(fmt::format("{} is an invalid mob vnum.\r\n", mobvnum));
      return eSUCCESS;
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
      return eFAILURE;
    }

    // put the buffs where they should be
    sprintf(buf2, "%s %s", buf3, buf4);
    strcpy(buf4, buf2);
    strcpy(buf3, buf);
  }

  // a this point, mob_num is the index
  if (mobvnum == -1)
    mobvnum = DC::getInstance()->mob_index[mob_num].virt;

  if (!can_modify_mobile(ch, mobvnum))
  {
    ch->sendln("You are unable to work creation outside of your range.");
    return eFAILURE;
  }

  ch->setPlayerLastMob(mobvnum);

  // no field
  if (!*buf3)
  {
    mpstat(ch, (Character *)DC::getInstance()->mob_index[mob_num].item);
    return eSUCCESS;
  }

  for (x = 0;; x++)
  {
    if (fields[x][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return eFAILURE;
    }
    if (is_abbrev(buf3, fields[x]))
      break;
  }

  switch (x)
  {

  /* add */
  case 0:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] add new\n\r"
                   "This creates a new mob prog and tacks it on the end.\r\n",
                   ch);
      return eFAILURE;
    }
    prog = new mob_prog_data;
    prog->type = GREET_PROG;
    prog->arglist = str_dup("80");
    prog->comlist = str_dup("say This is my new mob prog!\n\r");
    prog->next = nullptr;

    int prog_num = 1;
    if ((currprog = DC::getInstance()->mob_index[mob_num].mobprogs))
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
      DC::getInstance()->mob_index[mob_num].mobprogs = prog;
    }

    update_mobprog_bits(mob_num);

    ch->send(QStringLiteral("New mobprog created as #%1.\r\n").arg(prog_num));
  }
  break;

    /* remove */
  case 1:
  {
    if (!*buf4)
    {
      ch->sendln("$3Syntax$R: procedit [mob_num] remove <prog>");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 999))
    {
      ch->sendln("Invalid prog number.");
      return eFAILURE;
    }
    // find program number "intval"
    prog = nullptr;
    for (i = 1, currprog = DC::getInstance()->mob_index[mob_num].mobprogs; currprog && i != intval; i++, prog = currprog, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      ch->sendln("Invalid prog number.");
      return eFAILURE;
    }

    if (prog)
      prog->next = currprog->next;
    else
      DC::getInstance()->mob_index[mob_num].mobprogs = currprog->next;

    currprog->type = 0;
    delete[] currprog->arglist;
    delete[] currprog->comlist;
    delete currprog;

    update_mobprog_bits(mob_num);

    ch->send(fmt::format("Program {} deleted from mob vnum {}.\r\n", intval, mobvnum));
  }
  break;

    /* type */
  case 2:
  {
    half_chop(buf4, buf2, buf3);
    if (!*buf2 || !*buf3)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] type <prog> <newtype>\n\r"
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

      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf2, 1, 999))
    {
      ch->sendln("Invalid prog number.");
      return eFAILURE;
    }
    // find program number "intval"
    for (i = 1, currprog = DC::getInstance()->mob_index[mob_num].mobprogs; currprog && i != intval; i++, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      ch->sendln("Invalid prog number.");
      return eFAILURE;
    }

    if (!check_range_valid_and_convert(intval, buf3, 1, 17))
    {
      ch->sendln("Invalid prog number.");
      return eFAILURE;
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
    if (!*buf2 || !*buf3)
    {
      ch->sendln("$3Syntax$R: procedit [mob_num] arglist <prog> <new arglist>");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf2, 1, 999))
    {
      ch->sendln("Invalid prog number.");
      return eFAILURE;
    }
    // find program number "intval"
    for (i = 1, currprog = DC::getInstance()->mob_index[mob_num].mobprogs; currprog && i != intval; i++, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      ch->sendln("Invalid prog number.");
      return eFAILURE;
    }

    delete[] currprog->arglist;
    currprog->arglist = str_dup(buf3);

    ch->sendln("Mob program arglist changed.");
  }
  break;

    /* command */
  case 4:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] command <prog>\n\r"
                   "This will put you into the editor which will replace the current\r\n"
                   "command for program number <prog>.\r\n",
                   ch);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 999))
    {
      ch->sendln("Invalid prog number.");
      return eFAILURE;
    }
    // find program number "intval"
    for (i = 1, currprog = DC::getInstance()->mob_index[mob_num].mobprogs; currprog && i != intval; i++, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      ch->sendln("Invalid prog number.");
      return eFAILURE;
    }

    ch->desc->backstr = nullptr;
    ch->desc->strnew = &(currprog->comlist);
    ch->desc->max_str = MAX_MESSAGE_LENGTH;

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
        ch->desc->backstr = str_dup(currprog->comlist);
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
    mpstat(ch, (Character *)DC::getInstance()->mob_index[mob_num].item);
    return eFAILURE;
  }
  DC::getInstance()->set_zone_modified_mob(mob_num);
  return eSUCCESS;
}

int do_mscore(Character *ch, char *argument, cmd_t cmd)
{
  char buf[MAX_INPUT_LENGTH];
  int mob_num = -1;

  void boro_mob_stat(Character * ch, Character * k);

  if (IS_NPC(ch))
    return eFAILURE;

  one_argument(argument, buf);

  if (!*buf)
  {
    ch->sendln("$3Syntax$R:  mscore <mob_num>");
    return eFAILURE;
  }

  int64_t mob_vnum = atoi(buf); // there is no mob 0, so this is okay.  Bad 0's get caught in real_mobile
  if (((mob_num = real_mobile(mob_vnum)) < 0))
  {
    ch->send(fmt::format("{} is an invalid mob vnum.\r\n", mob_vnum));
    return eSUCCESS;
  }

  boro_mob_stat(ch, (Character *)DC::getInstance()->mob_index[mob_num].item);
  return eSUCCESS;
}

int do_medit(Character *ch, char *argument, cmd_t cmd)
{
  if (!ch || !argument)
  {
    return eFAILURE;
  }
  char buf[MAX_INPUT_LENGTH] = {};
  char buf2[MAX_INPUT_LENGTH] = {};
  char buf3[MAX_INPUT_LENGTH] = {};
  char buf4[MAX_INPUT_LENGTH] = {};
  vnum_t mob_num = {};
  int intval = {};
  int x = {}, i = {};

  const char *fields[] = {"keywords", "shortdesc", "longdesc", "description",
                          "sex", "class", "race", "level", "alignment", "loadposition",
                          "defaultposition", "actflags", "affectflags", "numdamdice",
                          "sizedamdice", "damroll", "hitroll", "hphitpoints", "gold",
                          "experiencepoints", "immune", "suscept", "resist", "armorclass",
                          "stat", "strength", "dexterity", "intelligence", "wisdom",
                          "constitution", "new", "delete", "type", "v1", "v2", "v3", "v4",
                          "\n"};

  if (!ch->isPlayer())
    return eFAILURE;

  half_chop(argument, buf, buf2);
  half_chop(buf2, buf3, buf4);

  // at this point, buf  = mob_num
  //                buf3 = field
  //                buf4 = args

  // or

  // buf = field
  // buf3 = args[0]
  // buf4 = args[1-+]

  if (!*buf)
  {
    send_to_char("$3Syntax$R:  medit [mob_num] [field] [arg]\r\n"
                 "  Edit a mob_num with no field or arg to view the item.\r\n"
                 "  Edit a field with no args for help on that field.\r\n\r\n"
                 "The field must be one of the following:\n\r",
                 ch);
    ch->display_string_list(fields);
    return eFAILURE;
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
      return eFAILURE;
    }
  }
  else
  {
    mobvnum = ch->player->last_mob_edit;
    if (((mob_num = real_mobile(mobvnum)) < 0 && strcmp(buf, "new")))
    {
      ch->send(fmt::format("{} is an invalid mob vnum.\r\n", mobvnum));
      return eFAILURE;
    }
    // put the buffs where they should be
    if (*buf4)
      sprintf(buf2, "%s %s", buf3, buf4);
    else
      strcpy(buf2, buf3);

    strcpy(buf4, buf2);
    strcpy(buf3, buf);
  }
  ch->setPlayerLastMob(mobvnum);

  if (!*buf3) // no field.  Stat the item.
  {
    return mob_stat(ch, (Character *)DC::getInstance()->mob_index[mob_num].item);
  }

  if (mobvnum == -1)
    mobvnum = DC::getInstance()->mob_index[mob_num].virt;
  // MOVED
  for (x = 0;; x++)
  {
    if (fields[x][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return eFAILURE;
    }
    if (is_abbrev(buf3, fields[x]))
      break;
  }

  // a this point, mob_num is the index

  if (x != 30) // Checked in there.
    if (!can_modify_mobile(ch, mobvnum))
    {
      ch->sendln("You are unable to work creation outside of your range.");
      return eFAILURE;
    }

  switch (x)
  {

  /* edit keywords */
  case 0:
  {
    if (!*buf4)
    {
      ch->sendln("$3Syntax$R: medit [mob_num] keywords <new_keywords>");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->setName(buf4);
    sprintf(buf, "Mob keywords set to '%s'.\r\n", buf4);
    ch->send(buf);
  }
  break;

    /* edit short desc */
  case 1:
  {
    if (!*buf4)
    {
      ch->sendln("$3Syntax$R: medit [mob_num] shortdesc <desc>");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->short_desc = str_hsh(buf4);
    sprintf(buf, "Mob shortdesc set to '%s'.\r\n", buf4);
    ch->send(buf);
  }
  break;

    // edit long desc
  case 2:
  {
    if (!*buf4)
    {
      ch->sendln("$3Syntax$R: medit [mob_num] longdesc <desc>");
      return eFAILURE;
    }
    strcat(buf4, "\r\n");
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->long_desc = str_hsh(buf4);
    sprintf(buf, "Mob longdesc set to '%s'.\r\n", buf4);
    ch->send(buf);
  }
  break;

    /* edit description */
  case 3:
  {
    if (!*buf4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] description <anything>\n\r"
          "This will put you into the editor which will replace the\r\n"
          "current description.\r\n",
          ch);
      return eFAILURE;
    }
    send_to_char("Enter the mob's description below."
                 " Terminate with '/s' on a new line.\n\r\n\r",
                 ch);
    // TODO - this causes a memory leak if you edit the desc twice (first one is hsh'd)
    //        ((Character *)DC::getInstance()->mob_index[mob_num].item)->description = nullptr;
    ch->desc->connected = Connection::states::EDITING;
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->description = str_dup("");
    ch->desc->strnew =
        &(((Character *)DC::getInstance()->mob_index[mob_num].item)->description);
    ch->desc->max_str = MAX_MESSAGE_LENGTH;
  }
  break;

    /* edit sex */
  case 4:
  {
    if (!*buf4)
    {
      ch->sendln("$3Syntax$R: medit [mob_num] sex <male|female|neutral>");
      return eFAILURE;
    }
    if (is_abbrev(buf4, "male"))
    {
      ((Character *)DC::getInstance()->mob_index[mob_num].item)->sex = SEX_MALE;
      ch->sendln("Mob sex set to male.");
    }
    else if (is_abbrev(buf4, "female"))
    {
      ((Character *)DC::getInstance()->mob_index[mob_num].item)->sex = SEX_FEMALE;
      ch->sendln("Mob sex set to female.");
    }
    else if (is_abbrev(buf4, "neutral"))
    {
      ((Character *)DC::getInstance()->mob_index[mob_num].item)->sex = SEX_NEUTRAL;
      ch->sendln("Mob sex set to neutral.");
    }
    else
      ch->sendln("Invalid sex.  Chose 'male', 'female', or 'neutral'.");
  }
  break;

    /* edit class */
  case 5:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] class <class>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%s\n",
              pc_clss_types[((Character *)DC::getInstance()->mob_index[mob_num].item)->c_class]);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = 0; *pc_clss_types[i] != '\n'; i++)
      {
        sprintf(buf, "  %d) %s\n\r", i, pc_clss_types[i]);
        ch->send(buf);
      }
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, CLASS_MAX))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->c_class = intval;
    sprintf(buf, "Mob class set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit race */
  case 6:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] race <racetype>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%s\n\r\n\r"
                   "Available types:\r\n",
              races[((Character *)DC::getInstance()->mob_index[mob_num].item)->race].singular_name);
      ch->send(buf);
      for (i = 0; i <= MAX_RACE; i++)
        csendf(ch, "  %s\r\n", races[i].singular_name);
      ch->sendln("");
      return eFAILURE;
    }
    int race_set = 0;
    for (i = 0; i <= MAX_RACE; i++)
    {
      if (is_abbrev(buf4, races[i].singular_name))
      {
        csendf(ch, "Mob race set to %s.\r\n",
               races[i].singular_name);
        ((Character *)DC::getInstance()->mob_index[mob_num].item)->race = i;
        race_set = 1;

        ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_str =
            ((Character *)DC::getInstance()->mob_index[mob_num].item)->str =
                BASE_STAT + mob_race_mod[GET_RACE(
                                ((Character *)DC::getInstance()->mob_index[mob_num].item))][0];
        ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_dex =
            ((Character *)DC::getInstance()->mob_index[mob_num].item)->dex =
                BASE_STAT + mob_race_mod[GET_RACE(
                                ((Character *)DC::getInstance()->mob_index[mob_num].item))][1];
        ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_con =
            ((Character *)DC::getInstance()->mob_index[mob_num].item)->con =
                BASE_STAT + mob_race_mod[GET_RACE(
                                ((Character *)DC::getInstance()->mob_index[mob_num].item))][2];
        ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_intel =
            ((Character *)DC::getInstance()->mob_index[mob_num].item)->intel =
                BASE_STAT + mob_race_mod[GET_RACE(
                                ((Character *)DC::getInstance()->mob_index[mob_num].item))][3];
        ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_wis =
            ((Character *)DC::getInstance()->mob_index[mob_num].item)->wis =
                BASE_STAT + mob_race_mod[GET_RACE(
                                ((Character *)DC::getInstance()->mob_index[mob_num].item))][4];
      }
    }
    if (!race_set)
    {
      csendf(ch, "Could not find race '%s'.\r\n", buf4);
      return eFAILURE;
    }
  }
  break;

    /* edit level */
  case 7:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] level <levelnum>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->getLevel());
      ch->send(buf);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 110))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->setLevel(intval);
    sprintf(buf, "Mob level set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit alignment */
  case 8:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] alignment <alignnum>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->alignment);
      ch->send(buf);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, -1000, 1000))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->alignment = intval;
    sprintf(buf, "Mob alignment set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit load position */
  case 9:
  {
    if (!*buf4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] loadposition <position>\n\r"
          "$3Current$R: ",
          ch);
      ch->sendln(QStringLiteral("%1").arg(((Character *)DC::getInstance()->mob_index[mob_num].item)->getPositionQString()));
      send_to_char("$3Valid positions$R:\r\n"
                   "  1 = Standing\r\n"
                   "  2 = Sitting\r\n"
                   "  3 = Resting\r\n"
                   "  4 = Sleeping\r\n",
                   ch);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 4))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }

    auto victim = ((Character *)DC::getInstance()->mob_index[mob_num].item);
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

    ch->sendln(QStringLiteral("Mob default position set to %1.").arg(victim->getPositionQString()));
  }
  break;

    /* edit default position */
  case 10:
  {
    if (!*buf4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] defaultposition <position>\n\r"
          "$3Current$R: ",
          ch);
      ch->sendln(QStringLiteral("%1").arg(Character::position_to_string(((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->default_pos)));

      send_to_char("$3Valid positions$R:\r\n"
                   "  1 = Standing\r\n"
                   "  2 = Sitting\r\n"
                   "  3 = Resting\r\n"
                   "  4 = Sleeping\r\n",
                   ch);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 4))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }

    auto victim = ((Character *)DC::getInstance()->mob_index[mob_num].item);
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
    snprintf(buf, sizeof(buf), "Mob default position set to %s.\r\n", Character::position_to_string(mobdata->default_pos).toStdString().c_str());
    ch->send(buf);
  }
  break;

    /* edit actflags */
  case 11:
  {
    if (!*buf4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] actflags <location[s]>\n\r"
          "$3Current$R: ",
          ch);
      sprintbit(
          ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->actflags,
          action_bits, buf);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = 0; *action_bits[i] != '\n'; i++)
      {
        sprintf(buf, "  %s\n\r", action_bits[i]);
        ch->send(buf);
      }
      return eFAILURE;
    }
    uint32_t old_actflags[2];
    old_actflags[0] = ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->actflags[0];
    old_actflags[1] = ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->actflags[1];
    parse_bitstrings_into_int(action_bits, buf4, ch, ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->actflags);

    uint32_t new_actflags[2];
    new_actflags[0] = ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->actflags[0];
    new_actflags[1] = ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->actflags[1];

    if (old_actflags[0] != new_actflags[0] || old_actflags[1] != new_actflags[1])
    {
      uint64_t NPCs_changed = 0;
      for (auto const &c : DC::getInstance()->character_list)
      {
        if (IS_NPC(c) && c->mobdata && DC::getInstance()->mob_index[c->mobdata->nr].virt == mobvnum)
        {
          c->mobdata->actflags[0] = new_actflags[0];
          c->mobdata->actflags[1] = new_actflags[1];
          NPCs_changed++;
        }
      }
      ch->send(QStringLiteral("%1 NPCs in the world have been updated.\r\n").arg(NPCs_changed));
    }
  }
  break;

    /* edit affectflags */
  case 12:
  {
    if (!*buf4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] affectflags <location[s]>\n\r"
          "$3Current$R: ",
          ch);
      sprintbit(((Character *)DC::getInstance()->mob_index[mob_num].item)->affected_by[0],
                affected_bits, buf);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = 0; *affected_bits[i] != '\n'; i++)
      {
        sprintf(buf, "  %s\n\r", affected_bits[i]);
        ch->send(buf);
      }
      return eFAILURE;
    }
    parse_bitstrings_into_int(affected_bits, buf4, ch,
                              &(((Character *)DC::getInstance()->mob_index[mob_num].item)->affected_by[0]));
    //                             &((((Character *)DC::getInstance()->mob_index[mob_num].item)->affected_by[0])));
  }
  break;

    /* edit numdamdice */
  case 13:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] numdamdice <amount>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->damnodice);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 400");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 400))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->damnodice = intval;
    sprintf(buf, "Mob number dice for damage set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit sizedamdice */
  case 14:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] sizedamdice <amount>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->damsizedice);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 400");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 400))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->damsizedice = intval;
    sprintf(buf, "Mob size dice for damage set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit damroll */
  case 15:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] damroll <damrollnum>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->damroll);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: -50 to 400");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, -50, 400))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->damroll = intval;
    sprintf(buf, "Mob damroll set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit hitroll */
  case 16:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] hitroll <levelnum>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->hitroll);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: -50 to 100");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, -50, 300))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->hitroll = intval;
    sprintf(buf, "Mob hitroll set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit hphitpoints */
  case 17:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] hphitpoints <hp>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_hit);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 64000");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 64000))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_hit = intval;
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->max_hit = intval;
    sprintf(buf, "Mob hitpoints set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit gold */
  case 18:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] gold <goldamount>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%lu\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->getGold());
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 0 to 10000000");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 10000000))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    if (intval > 250000 && ch->getLevel() <= DEITY)
    {
      ch->sendln("104-'s can only set a mob to 250k gold.  If you need more ask someone.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->setGold(intval);
    sprintf(buf, "Mob gold set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit experiencepoints */
  case 19:
  {
    if (!*buf4)
    {
      send_to_char(
          "$3Syntax$R: medit [mob_num] experiencepoints <xpamount>\n\r"
          "$3Current$R: ",
          ch);
      sprintf(buf, "%d\n",
              (int)((Character *)DC::getInstance()->mob_index[mob_num].item)->exp);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 0 to 20000000");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 20000000))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->exp = intval;
    sprintf(buf, "Mob experience set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit immune */
  case 20:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] immune <location[s]>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintbit(((Character *)DC::getInstance()->mob_index[mob_num].item)->immune, isr_bits,
                buf);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = 0; *isr_bits[i] != '\n'; i++)
      {
        sprintf(buf, "  %s\n\r", isr_bits[i]);
        ch->send(buf);
      }
      return eFAILURE;
    }
    parse_bitstrings_into_int(isr_bits, buf4, ch,
                              ((Character *)DC::getInstance()->mob_index[mob_num].item)->immune);
  }
  break;

    /* edit suscept */
  case 21:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] suscept <location[s]>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintbit(((Character *)DC::getInstance()->mob_index[mob_num].item)->suscept,
                isr_bits, buf);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = 0; *isr_bits[i] != '\n'; i++)
      {
        sprintf(buf, "  %s\n\r", isr_bits[i]);
        ch->send(buf);
      }
      return eFAILURE;
    }
    parse_bitstrings_into_int(isr_bits, buf4, ch,
                              ((Character *)DC::getInstance()->mob_index[mob_num].item)->suscept);
  }
  break;

    /* edit resist */
  case 22:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] resist <location[s]>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintbit(((Character *)DC::getInstance()->mob_index[mob_num].item)->resist, isr_bits,
                buf);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = 0; *isr_bits[i] != '\n'; i++)
      {
        sprintf(buf, "  %s\n\r", isr_bits[i]);
        ch->send(buf);
      }
      return eFAILURE;
    }
    parse_bitstrings_into_int(isr_bits, buf4, ch,
                              ((Character *)DC::getInstance()->mob_index[mob_num].item)->resist);
  }
  break;

    // armorclass
  case 23:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] armorclass <ac>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->armor);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 100 to $B-$R2000");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, -2000, 100))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->armor = intval;
    sprintf(buf, "Mob armorclass(ac) set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    // stat
  case 24:
  {
    return mob_stat(ch, (Character *)DC::getInstance()->mob_index[mob_num].item);
    break;
  }
    // strength
  case 25:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] strength <str>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_str);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 28");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 28))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_str = intval;
    sprintf(buf, "Mob raw strength set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    // dexterity
  case 26:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] dexterity <dex>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_dex);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 28");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 28))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_dex = intval;
    sprintf(buf, "Mob raw dexterity set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    // intelligence
  case 27:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] intelligence <int>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_intel);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 28");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 28))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_intel = intval;
    sprintf(buf, "Mob raw intelligence set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    // wisdom
  case 28:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] wisdom <wis>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_wis);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 28");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 28))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_wis = intval;
    sprintf(buf, "Mob raw wisdom set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    // constitution
  case 29:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_num] constitution <con>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%d\n",
              ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_con);
      ch->send(buf);
      ch->sendln("$3Valid Range$R: 1 to 28");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 28))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->raw_con = intval;
    sprintf(buf, "Mob raw constituion set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    // New
  case 30:
  {
    if (!*buf4)
    {
      ch->sendln("$3Syntax$R: medit new [number]");
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 0, 35000))
    {
      ch->sendln("Please specifiy a valid number.");
      return eFAILURE;
    }
    if (!can_modify_mobile(ch, intval))
    {
      ch->sendln("You cannot create mobiles in that range.");
      return eFAILURE;
    }
    mob_num = intval;
    x = ch->getDC()->create_blank_mobile(intval);
    if (x < 0)
    {
      csendf(ch,
             "Could not create mobile '%d'.  Max index hit or mob already exists.\r\n",
             intval);
      return eFAILURE;
    }
    ch->send(QStringLiteral("Mobile '%1' created successfully.\r\n").arg(intval));
    ch->setPlayerLastMob(intval);
  }
  break;
  case 31:
  {
    if (!*buf4 || strncmp(buf4, "yesiwanttodeletethismob", 23))
    {
      ch->sendln("$3Syntax$R: medit [mob_number] delete yesiwanttodeletethismob");
      return eFAILURE;
    }
    const auto &character_list = DC::getInstance()->character_list;
    for (const auto &v : character_list)
    {
      if (IS_NPC(v) && v->mobdata->nr == mob_num)
        extract_char(v, true);
    }
    delete_mob_from_index(mob_num);
    ch->sendln("Mobile deleted.");
  }
  break;
  case 32:
  {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: medit [mob_vnum] type <type id>\n\r"
                   "$3Current$R: ",
                   ch);
      sprintf(buf, "%s\n",
              mob_types[((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->mob_flags.type]);
      ch->send(buf);
      ch->sendln("\r\n$3Valid types$R:");
      for (i = 0; *mob_types[i] != '\n'; i++)
      {
        sprintf(buf, "  %d) %s\n\r", i, mob_types[i]);
        ch->send(buf);
      }
      return eFAILURE;
    }

    if (!check_range_valid_and_convert<decltype(intval)>(intval, buf4, MOB_TYPE_FIRST,
                                                         MOB_TYPE_LAST))
    {
      ch->sendln("Value out of valid range.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->mob_flags.type =
        (mob_type_t)intval;
    sprintf(buf, "Mob type set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
    /* edit 1value */
  case 33:
  {
    if (!*buf4)
    {
      ch->sendln("$3Syntax$R: medit [mob_vnum] 1value <num>");
      return eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      ch->sendln("Please specify a valid number.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->mob_flags.value[0] = intval;
    sprintf(buf, "Mob value 1 set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit 2value */
  case 34:
  {
    if (!*buf4)
    {
      ch->sendln("$3Syntax$R: medit [mob_vnum] 2value <num>");
      return eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      ch->sendln("Please specify a valid number.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->mob_flags.value[1] = intval;
    sprintf(buf, "Mob value 2 set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit 3value */
  case 35:
  {
    if (!*buf4)
    {
      ch->sendln("$3Syntax$R: medit [mob_vnum] 3value <num>");
      return eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      ch->sendln("Please specify a valid number.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->mob_flags.value[2] = intval;
    sprintf(buf, "Mob value 3 set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;

    /* edit 4value */
  case 36:
  {
    if (!*buf4)
    {
      ch->sendln("$3Syntax$R: medit [mob_vnum] 4value <num>");
      return eFAILURE;
    }
    if (!check_valid_and_convert(intval, buf4))
    {
      ch->sendln("Please specify a valid number.");
      return eFAILURE;
    }
    ((Character *)DC::getInstance()->mob_index[mob_num].item)->mobdata->mob_flags.value[3] = intval;
    sprintf(buf, "Mob value 4 set to %d.\r\n", intval);
    ch->send(buf);
  }
  break;
  }
  DC::getInstance()->set_zone_modified_mob(mob_num);
  return eSUCCESS;
}

int do_redit(Character *ch, char *argument, cmd_t cmd)
{
  std::string buf, remainder_args;
  int x, a, b, c, d = 0;
  struct extra_descr_data *extra;
  struct extra_descr_data *ext;

  const char *return_directions[] =
      {
          "south",
          "west",
          "north",
          "east",
          "down",
          "up",
          ""};

  int reverse_number[] =
      {
          2, 3, 0, 1, 5, 4};
  char *fields[] =
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

  if (IS_NPC(ch))
    return eFAILURE;

  std::string arg1;
  std::tie(arg1, remainder_args) = half_chop(std::string(argument));
  if (arg1.empty())
  {
    ch->sendln("The field must be one of the following:");
    for (x = 0;; x++)
    {
      if (fields[x][0] == '\0')
        return eFAILURE;
      csendf(ch, "  %s\n\r", fields[x]);
    }
  }

  if (!can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You are unable to work creation outside of your range.");
    return eFAILURE;
  }

  for (x = 0;; x++)
  {
    if (fields[x][0] == '\0')
    {
      ch->sendln("The field must be one of the following:");
      for (x = 0;; x++)
      {
        if (fields[x][0] == '\0')
          return eFAILURE;
        csendf(ch, "%s\n\r", fields[x]);
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
      return eFAILURE;
    }
    delete[] DC::getInstance()->world[ch->in_room].name;
    DC::getInstance()->world[ch->in_room].name = str_dup(remainder_args.c_str());
    ch->sendln("Ok.");
  }
  break;

    /* redit description */
  case 1:
  {
    if (!remainder_args.empty())
    {
      std::string description = remainder_args + "\n\r";
      delete[] DC::getInstance()->world[ch->in_room].description;
      DC::getInstance()->world[ch->in_room].description = str_dup(description.c_str());
      ch->sendln("Ok.");
      return eFAILURE;
    }
    ch->sendln("        Write your room's description.  (/s saves /h for help)");
    ch->desc->connected = Connection::states::EDITING;
    ch->desc->strnew = &(DC::getInstance()->world[ch->in_room].description);
    ch->desc->max_str = MAX_MESSAGE_LENGTH;
  }
  break;

    // redit exit
  case 2:
  {
    if (remainder_args.empty())
    {
      csendf(ch, "$3Syntax$R: <> is required. [] is optional.\r\n"
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
                 "redit exit n 1200 9 345 grate rusty  - North hidden door exit with keywords 'grate' or 'rusty' to room 1200 that requires key vnum 345.\r\n");
      return eFAILURE;
    }

    std::string arg2;
    std::tie(arg2, remainder_args) = half_chop(remainder_args);
    if (arg2 == "delete")
    {
      std::string direction;
      std::tie(direction, remainder_args) = half_chop(remainder_args);
      for (x = 0; x <= 6; x++)
      {
        if (is_abbrev(direction.c_str(), dirs[x]))
        {
          if (DC::getInstance()->world[ch->in_room].dir_option[x] == nullptr)
          {
            csendf(ch, "There is no %s exit.\r\n", dirs[x]);
            return eFAILURE;
          }

          int16_t destination_room = DC::getInstance()->world[ch->in_room].dir_option[x]->to_room;
          csendf(ch, "Deleting %s exit from room %d to %d.\r\n", dirs[x], ch->in_room, destination_room);
          delete DC::getInstance()->world[ch->in_room].dir_option[x];
          DC::getInstance()->world[ch->in_room].dir_option[x] = nullptr;

          if (IS_PC(ch) && !isSet(ch->player->toggles, Player::PLR_ONEWAY))
          {
            // if the destination room has a reverse exit
            if (DC::getInstance()->world[destination_room].dir_option[reverse_number[x]])
            {
              // and that reverse exit points to us then we can delete it too
              if (DC::getInstance()->world[destination_room].dir_option[reverse_number[x]]->to_room == ch->in_room)
              {
                csendf(ch, "Deleting %s exit from room %d to %d.\r\n", dirs[reverse_number[x]], destination_room, ch->in_room);
                delete DC::getInstance()->world[destination_room].dir_option[reverse_number[x]];
                DC::getInstance()->world[destination_room].dir_option[reverse_number[x]] = nullptr;
                return eSUCCESS;
              }
              else
              {
                csendf(ch, "Reverse %s exit in room %d does not point to room %d.\r\n",
                       dirs[reverse_number[x]],
                       destination_room,
                       ch->in_room);
                return eSUCCESS;
              }
            }
            else
            {
              csendf(ch, "Reverse %s exit in room %d does not exist.\r\n",
                     dirs[reverse_number[x]],
                     destination_room);
              return eSUCCESS;
            }
          } // end of check if Player::PLR_ONEWAY is toggled
          return eSUCCESS;
        } // end of is_abbred for dirs
      } // end of for loop through directions

      ch->sendln("Missing direction you want to delete.");
      return eFAILURE;
    } // end of delete

    for (x = 0; x <= 6; x++)
    {
      if (x == 6)
      {
        ch->sendln("No such direction.");
        return eFAILURE;
      }
      if (is_abbrev(arg2.c_str(), dirs[x]))
        break;
    }
    if (remainder_args.empty())
    {
      csendf(ch, "Missing vnum of room you want to have %s exit connect to.\r\n", dirs[x]);
      return eFAILURE;
    }

    std::string arg3;
    std::tie(arg3, remainder_args) = half_chop(remainder_args);
    try
    {
      d = stoi(arg3);
    }
    catch (...)
    {
      d = 0;
    }
    c = real_room(d);

    if (!remainder_args.empty())
    {
      std::string arg4;
      std::tie(arg4, remainder_args) = half_chop(remainder_args);
      try
      {
        a = stoi(arg4);
      }
      catch (...)
      {
        a = 0;
      }

      if (remainder_args.empty())
      {
        return eFAILURE;
      }

      std::string arg5;
      std::tie(arg5, remainder_args) = half_chop(remainder_args);
      try
      {
        b = stoi(arg5);
      }
      catch (...)
      {
        b = 0;
      }

      if (b == 0)
      {
        ch->sendln("No key 0's allowed.  Changing to -1.");
        b = -1;
      }
    }
    else
    {
      a = 0;
      b = -1;
    }

    if (c == (-1) && can_modify_room(ch, d))
    {
      if (DC::getInstance()->create_one_room(ch, d))
      {
        c = real_room(d);
        ch->send(QStringLiteral("Creating room %1.\r\n").arg(d));
      }
    }

    if (c == (-1))
    {
      ch->send(QStringLiteral("Error creating exit to room %1.\r\n").arg(d));
      return eFAILURE;
    }

    if (DC::getInstance()->world[ch->in_room].dir_option[x])
      ch->sendln("Modifying exit.");
    else
    {
      ch->sendln("Creating new exit.");
      DC::getInstance()->world[ch->in_room].dir_option[x] = new struct room_direction_data;
      DC::getInstance()->world[ch->in_room].dir_option[x]->general_description = 0;
      DC::getInstance()->world[ch->in_room].dir_option[x]->keyword = 0;
    }

    DC::getInstance()->world[ch->in_room].dir_option[x]->exit_info = a;
    DC::getInstance()->world[ch->in_room].dir_option[x]->key = b;
    DC::getInstance()->world[ch->in_room].dir_option[x]->to_room = c;
    if (!remainder_args.empty())
    {
      if (DC::getInstance()->world[ch->in_room].dir_option[x]->keyword)
        delete[] DC::getInstance()->world[ch->in_room].dir_option[x]->keyword;
      DC::getInstance()->world[ch->in_room].dir_option[x]->keyword = str_dup(remainder_args.c_str());
    }

    ch->sendln("Ok.");

    if (ch->isPlayer() && !isSet(ch->player->toggles, Player::PLR_ONEWAY))
    {
      send_to_char("Attempting to create a return exit from "
                   "that room...\r\n",
                   ch);
      if (DC::getInstance()->world[c].dir_option[reverse_number[x]])
      {
        ch->sendln("COULD NOT CREATE EXIT...One already exists.");
      }
      else
      {
        buf = fmt::format("{} redit exit {} {} {} {} {}", d,
                          return_directions[x], DC::getInstance()->world[ch->in_room].number,
                          a, b, (remainder_args != "" ? remainder_args.c_str() : ""));
        SET_BIT(ch->player->toggles, Player::PLR_ONEWAY);
        char *tmp = str_dup(buf.c_str());
        do_at(ch, tmp);
        delete[] tmp;
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
      csendf(ch, "$3Syntax$R: <> is required. [] is optional.\r\n"
                 "redit extra                   - show this syntax and current keywords.\r\n"
                 "redit extra <keywords ...>    - add or edit keywords.\r\n"
                 "redit extra delete <keyword>  - delete extra descriptions linked to keyword.\r\n\r\n");
      bool found = false;
      for (extra = DC::getInstance()->world[ch->in_room].ex_description; extra != nullptr; extra = extra->next)
      {
        if (extra->keyword != nullptr)
        {
          if (found == false)
          {
            found = true;
            ch->sendln("Extra description keywords:");
          }
          ch->send(QStringLiteral("%1\r\n").arg(extra->keyword));
        }
      }
      if (found == false)
      {
        ch->sendln("No extra description keywords found.");
      }

      return eSUCCESS;
    }

    std::string arg2;
    std::tie(arg2, remainder_args) = half_chop(remainder_args);

    if (arg2 == "delete")
    {
      std::string arg3;
      std::tie(arg3, remainder_args) = half_chop(remainder_args);

      bool deleted = false;
      extra_descr_data *prev = nullptr;
      for (extra = DC::getInstance()->world[ch->in_room].ex_description; extra != nullptr; prev = extra, extra = extra->next)
      {
        if (arg3 == std::string(extra->keyword))
        {
          if (prev == nullptr)
          {
            DC::getInstance()->world[ch->in_room].ex_description = extra->next;
          }
          else
          {
            prev->next = extra->next;
          }
          ch->send(QStringLiteral("Extra description with keyword '%1' deleted.\r\n").arg(extra->keyword));
          delete extra;
          deleted = true;
          // break out of for loop
          break;
        }
      }
      if (deleted == false)
      {
        csendf(ch, "Extra description with keyword '%s' not found.\r\n", arg3.c_str());
      }
      // break out of switch case
      break;
    }

    for (extra = DC::getInstance()->world[ch->in_room].ex_description;; extra = extra->next)
    {
      if (!extra)
      {
        // No matching extra description found so make a new one
        csendf(ch, "Creating new extra description for keyword '%s'.\r\n", arg2.c_str());
        extra = new struct extra_descr_data;
        extra->next = nullptr;

        if (!(DC::getInstance()->world[ch->in_room].ex_description))
        {
          // The room has no pre-existing extra descriptions so this will be its first
          DC::getInstance()->world[ch->in_room].ex_description = extra;
        }
        else
        {
          // The room has pre-existing extra descriptions so we will find the end of
          // this linked list and append our new extra description to it
          for (ext = DC::getInstance()->world[ch->in_room].ex_description;; ext = ext->next)
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
      else if (arg2 == std::string(extra->keyword))
      {
        // A pre-existing extra description was found
        csendf(ch, "Modifying extra description for keyword '%s'.\r\n", arg2.c_str());
        break;
      }
    }

    if (extra->keyword)
      delete[] extra->keyword;
    extra->keyword = str_dup(arg2.c_str());
    ch->sendln("Write your extra description. (/s saves /h for help)");
    ch->desc->strnew = &extra->description;
    ch->desc->max_str = MAX_MESSAGE_LENGTH;
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
      return eFAILURE;
    }

    std::string arg3;
    std::tie(arg3, remainder_args) = half_chop(remainder_args);
    for (x = 0; x <= 6; x++)
    {
      if (x == 6)
      {
        ch->sendln("No such direction.");
        return eFAILURE;
      }
      if (is_abbrev(arg3.c_str(), dirs[x]))
        break;
    }

    if (!(DC::getInstance()->world[ch->in_room].dir_option[x]))
    {
      ch->sendln("That exit does not exist...create it first.");
      return eFAILURE;
    }

    send_to_char("Enter the exit's description below. Terminate with "
                 "'~' on a new line.\n\r\n\r",
                 ch);
    /*        if(DC::getInstance()->world[ch->in_room].dir_option[x]->general_description) {
          delete DC::getInstance()->world[ch->in_room].dir_option[x]->general_description;
          DC::getInstance()->world[ch->in_room].dir_option[x]->general_description = 0;
        }
   */
    ch->desc->strnew = &DC::getInstance()->world[ch->in_room].dir_option[x]->general_description;
    ch->desc->max_str = MAX_MESSAGE_LENGTH;
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
      for (x = 0;; x++)
      {
        if (!strcmp(room_bits[x], "\n"))
          break;
        if (!strcmp(room_bits[x], "unused"))
          continue;
        if ((x + 1) % 4 == 0)
        {
          csendf(ch, "%-18s\n\r", room_bits[x]);
        }
        else
        {
          csendf(ch, "%-18s", room_bits[x]);
        }
      }
      ch->sendln("\r\n");
      return eFAILURE;
    }
    parse_bitstrings_into_int(room_bits, remainder_args.c_str(), ch, (DC::getInstance()->world[ch->in_room].room_flags));
  }
  break;

  // sector
  case 6:
  {
    if (remainder_args.empty())
    {
      ch->sendln("$3Syntax$R: redit sector <sector>");
      ch->sendln("$3Available sector types$R:");
      for (x = 0;; x++)
      {
        if (!strcmp(sector_types[x], "\n"))
          break;
        if ((x + 1) % 4 == 0)
        {
          csendf(ch, "%-18s\n\r", sector_types[x]);
        }
        else
        {
          csendf(ch, "%-18s", sector_types[x]);
        }
      }
      ch->sendln("\r\n");
      return eFAILURE;
    }
    for (x = 0;; x++)
    {
      if (!strcmp(sector_types[x], "\n"))
      {
        ch->sendln("No such sector type.");
        return eFAILURE;
      }
      else if (is_abbrev(remainder_args.c_str(), sector_types[x]))
      {
        DC::getInstance()->world[ch->in_room].sector_type = x;
        csendf(ch, "Sector type set to %s.\r\n", sector_types[x]);
        break;
      }
    }
  }
  break;
  case 7: // denymob
    if (remainder_args.empty() || !is_number(remainder_args.c_str()))
    {
      ch->sendln("Syntax: redit denymob <vnum>\r\nDoing this on an already denied mob will allow it once more.");
      return eFAILURE;
    }
    bool done = false;
    int mob = 0;
    try
    {
      mob = stoi(remainder_args);
    }
    catch (...)
    {
      mob = 0;
    }
    struct deny_data *nd, *pd = nullptr;
    for (nd = DC::getInstance()->world[ch->in_room].denied; nd; nd = nd->next)
    {
      if (nd->vnum == mob)
      {
        if (pd)
          pd->next = nd->next;
        else
          DC::getInstance()->world[ch->in_room].denied = nd->next;
        delete nd;
        ch->send(QStringLiteral("Mobile %1 ALLOWED entrance.\r\n").arg(mob));
        done = true;
        break;
      }
      pd = nd;
    }
    if (done)
      break;
    nd = new struct deny_data;
    nd->next = DC::getInstance()->world[ch->in_room].denied;
    try
    {
      nd->vnum = stoi(remainder_args);
    }
    catch (...)
    {
      nd->vnum = 0;
    }

    DC::getInstance()->world[ch->in_room].denied = nd;
    ch->send(QStringLiteral("Mobile %1 DENIED entrance.\r\n").arg(mob));
    break;
  }
  DC::getInstance()->set_zone_modified_world(ch->in_room);
  return eSUCCESS;
}

int do_rdelete(Character *ch, char *arg, cmd_t cmd)
{
  int x;
  char buf[50], buf2[50];
  struct extra_descr_data *i, *extra;

  half_chop(arg, buf, buf2);

  if (!*buf)
  {
    send_to_char("$3Syntax$R:\n\rrdelete exit   <direction>\n\rrdelete "
                 "exdesc <direction>\n\rrdelete extra  <keyword>\n\r",
                 ch);
    return eFAILURE;
  }

  if (!can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You cannot destroy things here, it is not your domain.");
    return eFAILURE;
  }

  if (is_abbrev(buf, "exit"))
  {
    for (x = 0; x <= 6; x++)
    {
      if (x == 6)
      {
        ch->sendln("No such direction.");
        return eFAILURE;
      }
      if (is_abbrev(buf2, dirs[x]))
        break;
    }

    if (!(DC::getInstance()->world[ch->in_room].dir_option[x]))
    {
      ch->sendln("There is nothing there to remove.");
      return eFAILURE;
    }
    delete DC::getInstance()->world[ch->in_room].dir_option[x];
    DC::getInstance()->world[ch->in_room].dir_option[x] = 0;
    csendf(ch, "You stretch forth your hands and remove "
               "the %s exit.\r\n",
           dirs[x]);
  }

  else if (is_abbrev(buf, "extra"))
  {
    for (i = DC::getInstance()->world[ch->in_room].ex_description;; i = i->next)
    {
      if (i == nullptr)
      {
        ch->sendln("There is nothing there to remove.");
        return eFAILURE;
      }
      if (isexact(buf2, i->keyword))
        break;
    }

    if (DC::getInstance()->world[ch->in_room].ex_description == i)
    {
      DC::getInstance()->world[ch->in_room].ex_description = i->next;
      delete i;
      ch->sendln("You remove the extra description.");
    }
    else
    {
      for (extra = DC::getInstance()->world[ch->in_room].ex_description;; extra = extra->next)
        if (extra->next == i)
        {
          extra->next = i->next;
          delete i;
          ch->sendln("You remove the extra description.");
          break;
        }
    }
  }

  else if (is_abbrev(buf, "exdesc"))
  {
    if (!*buf2)
    {
      ch->sendln("Syntax:\n\rrdelete exdesc <direction>");
      return eFAILURE;
    }
    one_argument(buf2, buf);
    for (x = 0; x <= 6; x++)
    {
      if (x == 6)
      {
        ch->sendln("No such direction.");
        return eFAILURE;
      }
      if (is_abbrev(buf, dirs[x]))
        break;
    }

    if (!(DC::getInstance()->world[ch->in_room].dir_option[x]))
    {
      ch->sendln("That exit does not exist...create it first.");
      return eFAILURE;
    }

    if (!(DC::getInstance()->world[ch->in_room].dir_option[x]->general_description))
    {
      ch->sendln("There's no description there to delete.");
      return eFAILURE;
    }

    else
    {
      delete[] DC::getInstance()->world[ch->in_room].dir_option[x]->general_description;
      DC::getInstance()->world[ch->in_room].dir_option[x]->general_description = 0;
      ch->sendln("Ok.");
    }
  }

  else
    send_to_char("Syntax:\n\rrdelete exit   <direction>\n\rrdelete "
                 "exdesc <direction>\n\rrdelete extra  <keyword>\n\r",
                 ch);

  DC::getInstance()->set_zone_modified_world(ch->in_room);
  return eSUCCESS;
}

int do_oneway(Character *ch, char *arg, cmd_t cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

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
  return eSUCCESS;
}

command_return_t Character::do_zsave(QStringList arguments, cmd_t cmd)
{
  zone_t zone_key{};
  auto &zones = DC::getInstance()->zones;

  if (arguments.isEmpty())
  {
    zone_key = DC::getInstance()->world[in_room].zone;
  }
  else
  {
    bool ok = false;
    zone_key = arguments.value(0).toULongLong(&ok);
    if (!ok)
    {
      sendln(QStringLiteral("Invalid zone number. Valid zone numbers are %1-%2.").arg(zones.firstKey()).arg(zones.lastKey()));
      return eFAILURE;
    }
  }

  if (zones.contains(zone_key) == false)
  {
    sendln(QStringLiteral("Zone %1 not found. Valid zone numbers are %2-%3.").arg(zone_key).arg(zones.firstKey()).arg(zones.lastKey()));
    return eFAILURE;
  }
  auto &zone = zones[zone_key];

  if (!can_modify_room(this, zone.getRealBottom()))
  {
    sendln("You may only zsave zones that include the room range you are assigned to.");
    return eFAILURE;
  }

  if (zone.getFilename().isEmpty())
  {
    sendln(QStringLiteral("Zone %1 has an empty filename.").arg(zone_key));
    return eFAILURE;
  }

  if (!zone.isModified())
  {
    sendln(QStringLiteral("Zone %1 has not been modified. Saving anyway.").arg(zone_key));
  }

  QString filename = QStringLiteral("zonefiles/%1").arg(zone.getFilename());
  QString command = QStringLiteral("cp %1 %1.last").arg(filename);
  system(command.toStdString().c_str());

  FILE *f = nullptr;
  if ((f = fopen(filename.toStdString().c_str(), "w")) == nullptr)
  {
    logbug(QStringLiteral("do_zsave: couldn't open zone save file '%1' for '%2'.").arg(filename).arg(getName()));
    return eFAILURE;
  }

  zone.write(f);

  fclose(f);
  sendln(QStringLiteral("Saved zone %1.").arg(zone_key));
  zone.setModified(false);
  return eSUCCESS;
}

int do_rsave(Character *ch, char *arg, cmd_t cmd)
{
  world_file_list_item *curr;

  if (!can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You may only rsave inside of the room range you are assigned to.");
    return eFAILURE;
  }

  curr = DC::getInstance()->world_file_list;
  while (curr)
    if (curr->firstnum <= ch->in_room && curr->lastnum >= ch->in_room)
      break;
    else
      curr = curr->next;

  if (!curr)
  {
    ch->sendln("That range doesn't seem to exist...tell an imp.");
    return eFAILURE;
  }

  if (!isSet(curr->flags, WORLD_FILE_MODIFIED))
  {
    ch->sendln("This range has not been modified.");
    return eFAILURE;
  }

  LegacyFileWorld lfw(curr->filename);
  if (lfw.isOpen())
  {
    for (int x = curr->firstnum; x <= curr->lastnum; x++)
    {
      write_one_room(lfw, x);
    }
  }

  ch->sendln("Saved.");
  DC::getInstance()->set_zone_saved_world(ch->in_room);
  return eSUCCESS;
}

int do_msave(Character *ch, char *arg, cmd_t cmd)
{
  world_file_list_item *curr;
  char buf[180];
  char buf2[180];

  if (ch->player->last_mob_edit <= 0)
  {
    ch->sendln("You have not recently edited a mobile.");
    return eFAILURE;
  }

  int v = ch->player->last_mob_edit;
  if (!can_modify_mobile(ch, v))
  {
    ch->sendln("You may only msave inside of the room range you are assigned to.");
    return eFAILURE;
  }

  int r = real_mobile(v);

  curr = DC::getInstance()->mob_file_list;
  while (curr)
    if (curr->firstnum <= r && curr->lastnum >= r)
      break;
    else
      curr = curr->next;

  if (!curr)
  {
    ch->sendln("That range doesn't seem to exist...tell an imp.");
    return eFAILURE;
  }

  if (!isSet(curr->flags, WORLD_FILE_MODIFIED))
  { // this is okay...world_file_saved is used in all
    ch->sendln("This range has not been modified.");
    return eFAILURE;
  }

  LegacyFile lf("mobs", curr->filename, "Couldn't open legacy mob save file %1.");
  if (lf.isOpen())
  {
    for (int x = curr->firstnum; x <= curr->lastnum; x++)
    {
      write_mobile(lf, (Character *)DC::getInstance()->mob_index[x].item);
    }
    fprintf(lf.file_handle_, "$~\n");
  }

  ch->sendln("Saved.");
  DC::getInstance()->set_zone_saved_mob(curr->firstnum);
  return eSUCCESS;
}

int do_osave(Character *ch, char *arg, cmd_t cmd)
{
  world_file_list_item *curr;
  char buf[180];
  char buf2[180];

  if (ch->player->last_obj_vnum < 1)
  {
    ch->sendln("You have not recently edited an item.");
    return eFAILURE;
  }
  vnum_t v = ch->player->last_obj_vnum;
  if (!can_modify_object(ch, v))
  {
    ch->sendln("You may only msave inside of the room range you are assigned to.");
    return eFAILURE;
  }
  int r = real_object(v);
  curr = DC::getInstance()->obj_file_list;
  while (curr)
    if (curr->firstnum <= r && curr->lastnum >= r)
      break;
    else
      curr = curr->next;

  if (!curr)
  {
    ch->sendln("That range doesn't seem to exist...tell an imp.");
    return eFAILURE;
  }

  if (!isSet(curr->flags, WORLD_FILE_MODIFIED))
  {
    ch->sendln("This range has not been modified.");
    return eFAILURE;
  }

  LegacyFile lf("objects", curr->filename, "Couldn't open legacy obj save file %1.");
  if (lf.isOpen())
  {
    for (int x = curr->firstnum; x <= curr->lastnum; x++)
    {
      write_object(lf, (Object *)DC::getInstance()->obj_index[x].item);
    }
    fprintf(lf.file_handle_, "$~\n");
  }

  ch->sendln("Saved.");
  DC::getInstance()->set_zone_saved_obj(curr->firstnum);
  return eSUCCESS;
}

int do_instazone(Character *ch, char *arg, cmd_t cmd)
{
  FILE *fl;
  char buf[200], bufl[200] /*,buf2[200],buf3[200]*/;
  int room = 1, x, door /*,direction*/;
  int pos;
  int value;
  int low, high;
  int /*number,*/ count;
  Character *mob, /**tmp_mob,*next_mob,*/ *mob_list;
  class Object *obj, *tmp_obj, /**next_obj,*/ *obj_list;

  bool found_room = false;

  ch->sendln("Whoever thought of this had a good idea, but never really finished it.  Beg someone to finish it some time.");
  return eFAILURE;

  // Remember if you change this that it uses string_to_file which now appends a ~\n to the end
  // of the std::string.  This command does NOT take that into consideration currently.

  /*    if(!GET_RANGE(ch)) {
   ch->sendln("You don't have a zone assigned to you!");
   return eFAILURE;
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
    return eFAILURE;
  }

  sprintf(buf, "../lib/builder/%s.zon", GET_NAME(ch));

  if ((fl = fopen(buf, "w")) == nullptr)
  {
    ch->send("Couldn't open up zone file. Tell Godflesh!");
    fclose(fl);
    return eFAILURE;
  }

  for (x = low; x <= high; x++)
  {
    room = real_room(x);
    if (room == DC::NOWHERE)
      continue;
    break;
  }

  fprintf(fl, "#%d\n", DC::getInstance()->world[room].zone);
  sprintf(buf, "%s's Area.", ch->getNameC());
  string_to_file(fl, buf);
  fprintf(fl, "~\n");
  fprintf(fl, "%d 30 2\n", high);

  /* Set allthe door states..  */

  for (x = low; x <= high; x++)
  {
    room = real_room(x);
    if (room == DC::NOWHERE)
      continue;

    for (door = 0; door <= 5; door++)
    {
      if (!(DC::getInstance()->world[room].dir_option[door]))
        continue;

      if (isSet(DC::getInstance()->world[room].dir_option[door]->exit_info, EX_ISDOOR))
      {
        if ((isSet(DC::getInstance()->world[room].dir_option[door]->exit_info, EX_CLOSED)) && (isSet(DC::getInstance()->world[room].dir_option[door]->exit_info, EX_LOCKED)))
        {
          value = 2;
        }
        else if (isSet(DC::getInstance()->world[room].dir_option[door]->exit_info, EX_CLOSED))
        {
          value = 1;
        }
        else
        {
          value = 0;
        }

        fprintf(fl, "D 0 %d %d %d\n", DC::getInstance()->world[room].number, DC::getInstance()->world[DC::getInstance()->world[room].dir_option[door]->to_room].number, value);
      }
    }
  } /*  Ok.. all door state info written...  */

  /*  Load loose objects.  In other words.. objects not carried by mobs. */

  for (x = low; x <= high; x++)
  {
    room = real_room(x);
    if (room == DC::NOWHERE)
      continue;

    if (DC::getInstance()->world[room].contents)
    {

      for (obj = DC::getInstance()->world[room].contents; obj; obj = obj->next_content)
      {

        count = 0;

        for (obj_list = DC::getInstance()->object_list; obj_list; obj_list =
                                                                      obj_list->next)
        {
          if (obj_list->item_number == obj->item_number)
            count++;
        }

        if (!obj->in_obj)
        {
          fprintf(fl, "O 0 %d %d %d",
                  DC::getInstance()->obj_index[obj->item_number].virt, count,
                  DC::getInstance()->world[room].number);
          sprintf(buf, "           %s\n", obj->short_description);
          string_to_file(fl, buf);

          if (obj->contains)
          {

            for (tmp_obj = obj->contains; tmp_obj;
                 tmp_obj = tmp_obj->next_content)
            {
              count = 0;
              for (obj_list = DC::getInstance()->object_list; obj_list; obj_list =
                                                                            obj_list->next)
              {
                if (obj_list->item_number == tmp_obj->item_number)
                  count++;
              }

              fprintf(fl, "P 1 %d %d %d",
                      DC::getInstance()->obj_index[tmp_obj->item_number].virt, count,
                      DC::getInstance()->obj_index[obj->item_number].virt);
              sprintf(buf, "     %s placed inside %s\n",
                      tmp_obj->short_description,
                      obj->short_description);
              string_to_file(fl, buf);
            } /*  for loop */
          } /* end of the object's contents... */
        } /* end of if !obj->in_obj */
      } /* first for loop for loose objects... */
    } /* All loose objects taken care of..  (Not on mobs,but inthe rooms) */
  } /*  for loop going through the fucking rooms again for loose obj's */

  /* Now for the major bitch...
   * All the mobs, and all possible bullshit our builders will try to
   * put on them.. i.e held objects with objects within them... Just
   * your average pain in the ass shit....
   */

  for (x = low; x <= high; x++)
  {
    room = real_room(x);
    if (room == DC::NOWHERE)
      continue;

    if (DC::getInstance()->world[room].people)
    {

      for (mob = DC::getInstance()->world[room].people; mob; mob = mob->next_in_room)
      {
        if (IS_PC(mob))
          continue;

        count = 0;

        for (mob_list = character_list; mob_list;
             mob_list = mob_list->next)
        {
          if (IS_NPC(mob_list) && mob_list->mobdata->nr == mob->mobdata->nr)
            count++;
        }

        fprintf(fl, "M 0 %d %d %d", DC::getInstance()->mob_index[mob->mobdata->nr].virt,
                count, DC::getInstance()->world[room].number);
        sprintf(buf, "           %s\n", mob->short_desc);
        string_to_file(fl, buf);

        for (pos = 0; pos < MAX_WEAR; pos++)
        {
          if (mob->equipment[pos])
          {

            obj = mob->equipment[pos];

            count = 0;
            for (obj_list = DC::getInstance()->object_list; obj_list; obj_list =
                                                                          obj_list->next)
            {
              if (obj_list->item_number == obj->item_number)
                count++;
            }

            if (!obj->in_obj)
            {
              fprintf(fl, "E 1 %d %d %d",
                      DC::getInstance()->obj_index[obj->item_number].virt, count,
                      pos);
              sprintf(buf, "      Equip %s with %s\n",
                      mob->short_desc, obj->short_description);
              string_to_file(fl, buf);

              if (obj->contains)
              {

                for (tmp_obj = obj->contains; tmp_obj; tmp_obj =
                                                           tmp_obj->next_content)
                {
                  count = 0;
                  for (obj_list = DC::getInstance()->object_list; obj_list;
                       obj_list = obj_list->next)
                  {
                    if (obj_list->item_number == tmp_obj->item_number)
                      count++;
                  }

                  fprintf(fl, "P 1 %d %d %d",
                          DC::getInstance()->obj_index[tmp_obj->item_number].virt,
                          count,
                          DC::getInstance()->obj_index[obj->item_number].virt);
                  sprintf(buf, "     %s placed inside %s\n",
                          tmp_obj->short_description,
                          obj->short_description);
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

            count = 0;
            for (obj_list = DC::getInstance()->object_list; obj_list; obj_list =
                                                                          obj_list->next)
            {
              if (obj_list->item_number == obj->item_number)
                count++;
            }

            if (!obj->in_obj)
            {
              fprintf(fl, "G 1 %d %d",
                      DC::getInstance()->obj_index[obj->item_number].virt, count);
              sprintf(buf, "      Give %s %s\n", mob->short_desc,
                      obj->short_description);
              string_to_file(fl, buf);

              if (obj->contains)
              {

                for (tmp_obj = obj->contains; tmp_obj; tmp_obj =
                                                           tmp_obj->next_content)
                {
                  count = 0;
                  for (obj_list = DC::getInstance()->object_list; obj_list;
                       obj_list = obj_list->next)
                  {
                    if (obj_list->item_number == tmp_obj->item_number)
                      count++;
                  }

                  fprintf(fl, "P 1 %d %d %d",
                          DC::getInstance()->obj_index[tmp_obj->item_number].virt,
                          count,
                          DC::getInstance()->obj_index[obj->item_number].virt);
                  sprintf(buf, "     %s placed inside %s\n",
                          tmp_obj->short_description,
                          obj->short_description);
                  string_to_file(fl, buf);
                } /*  for loop */
              }
            } /* end of the object's contents... */
          } /* end of for loop going through a mob's inventory */
        } /* end of if a mob has shit in their inventory */
      } /* end of for loop for looking at the mobs in ths room. */
    } /* end of if some body is in the fucking room.  */
  } /* end of for loop going through the zone looking for mobs...  */

  fprintf(fl, "S\n");
  fclose(fl);
  ch->sendln("Zone File Created! Tell someone who can put it in!");
  return eSUCCESS;
}

int do_rstat(Character *ch, char *argument, cmd_t cmd)
{
  char arg1[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH * 2];
  char buf2[MAX_STRING_LENGTH];
  Room *rm = 0;
  Character *k = 0;
  class Object *j = 0;
  struct extra_descr_data *desc;
  int i, x, loc;

  if (IS_NPC(ch))
    return eFAILURE;

  argument = one_argument(argument, arg1);

  /* no argument */
  if (!*arg1)
  {
    rm = &DC::getInstance()->world[ch->in_room];
  }

  else
  {
    x = atoi(arg1);
    if (x < 0 || (loc = real_room(x)) == DC::NOWHERE)
    {
      ch->sendln("No such room exists.");
      return eFAILURE;
    }
    rm = &DC::getInstance()->world[loc];
  }
  if (isSet(rm->room_flags, CLAN_ROOM) && ch->getLevel() < PATRON)
  {
    ch->sendln("And you are rstating a clan room because?");
    sprintf(buf, "%s just rstat'd clan room %d.", GET_NAME(ch), rm->number);
    logentry(buf, PATRON, DC::LogChannel::LOG_GOD);
    return eFAILURE;
  }
  sprintf(buf,
          "Room name: %s, Of zone : %d. V-Number : %d, R-number : %d\n\r",
          rm->name, rm->zone, rm->number, ch->in_room);
  ch->send(buf);

  sprinttype(rm->sector_type, sector_types, buf2);
  sprintf(buf, "Sector type : %s ", buf2);
  ch->send(buf);

  strcpy(buf, "Special procedure : ");
  strcat(buf, (rm->funct) ? "Exists\n\r" : "No\n\r");
  ch->send(buf);

  ch->send("Room flags: ");
  sprintbit((int32_t)rm->room_flags, room_bits, buf);
  std::string buffer = fmt::format("{} [ {} ]\r\n", buf, rm->room_flags);
  send_to_char(buffer.c_str(), ch);

  ch->sendln("Description:");
  ch->send(rm->description);

  strcpy(buf, "Extra description keywords(s): ");
  if (rm->ex_description)
  {
    strcat(buf, "\n\r");
    for (desc = rm->ex_description; desc; desc = desc->next)
    {
      strcat(buf, desc->keyword);
      strcat(buf, "\n\r");
    }
    strcat(buf, "\n\r");
    ch->send(buf);
  }
  else
  {
    strcat(buf, "None\n\r");
    ch->send(buf);
  }
  struct deny_data *d;
  int a = 0;
  for (d = rm->denied; d; d = d->next)
  {
    if (a == 0)
      ch->send("Mobiles Denied: ");
    if (real_mobile(d->vnum) == -1)
      ch->send(QStringLiteral("UNKNOWN(%1)\r\n").arg(d->vnum));
    else
      csendf(ch, "%s(%d)\r\n", ((Character *)DC::getInstance()->mob_index[real_mobile(d->vnum)].item)->short_desc, d->vnum);
    a++;
  }
  ch->sendln("");
  strcpy(buf, "------- Chars present -------\n\r");
  for (k = rm->people; k; k = k->next_in_room)
  {
    if (CAN_SEE(ch, k))
    {
      strcat(buf, GET_NAME(k));
      strcat(buf,
             (IS_PC(k) ? "(PC)\n\r" : (k->isPlayer() ? "(NPC)\n\r" : "(MOB)\n\r")));
    }
  }
  strcat(buf, "\n\r");
  ch->send(buf);

  buffer = "--------- Contents ---------\n\r";
  for (j = rm->contents; j; j = j->next_content)
  {
    if (CAN_SEE_OBJ(ch, j))
    {
      buffer += j->Name().toStdString();
      buffer += "\n\r";
    }
  }
  buffer += "\n\r";
  send_to_char(const_cast<char *>(buffer.c_str()), ch);

  ch->sendln("------- Exits defined -------");
  for (i = 0; i <= 5; i++)
  {
    if (rm->dir_option[i])
    {
      sprintf(buf, "Direction %s . Keyword : %s\n\r",
              dirs[i], rm->dir_option[i]->keyword);
      ch->send(buf);
      strcpy(buf, "Description:\n\r  ");
      if (rm->dir_option[i]->general_description)
        strcat(buf, rm->dir_option[i]->general_description);
      else
        strcat(buf, "UNDEFINED\n\r");
      ch->send(buf);
      sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
      sprintf(buf,
              "Exit flag: %s \n\rKey no: %d\n\rTo room (V-Number): %d\n\r",
              buf2, rm->dir_option[i]->key,
              rm->dir_option[i]->to_room);
      ch->send(buf);
    }
  }
  return eSUCCESS;
}

int do_possess(Character *ch, char *argument, cmd_t cmd)
{
  char arg[MAX_STRING_LENGTH];
  Character *victim;
  char buf[200];

  if (IS_NPC(ch))
    return eFAILURE;

  one_argument(argument, arg);

  if (!*arg)
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
        return eFAILURE;
      }
      else if ((victim->getLevel() > ch->getLevel()) &&
               (ch->getLevel() < IMPLEMENTER))
      {
        ch->sendln("That mob is a bit too tough for you to handle.");
        return eFAILURE;
      }
      else if (!ch->desc || ch->desc->snoop_by || ch->desc->snooping)
      {
        if (ch->desc->snoop_by)
        {
          ch->desc->snoop_by->character->send("Whoa! Almost got caught snooping!\n");
          sprintf(buf, "Your victim is now trying to possess: %s\n", victim->getNameC());
          ch->desc->snoop_by->character->send(buf);
          ch->desc->snoop_by->character->do_snoop(ch->desc->snoop_by->character->getName().split(' '));
        }
        else
        {
          ch->sendln("Mixing snoop & possess is bad for your health.");
          return eFAILURE;
        }
      }

      else if (victim->desc || (IS_PC(victim)))
      {
        ch->sendln("You can't do that, the body is already in use!");
        return eFAILURE;
      }
      else
      {
        ch->sendln("Ok.");
        sprintf(buf, "%s possessed %s", GET_NAME(ch), victim->getNameC());
        logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
        ch->player->possesing = 1;
        ch->desc->character = victim;
        ch->desc->original = ch;

        victim->desc = ch->desc;
        ch->desc = 0;
      }
    }
  }
  return eSUCCESS;
}

int do_return(Character *ch, char *argument, cmd_t cmd)
{

  //    if(IS_NPC(ch))
  //        return eFAILURE;

  if (!ch->desc)
    return eFAILURE;

  if (!ch->desc->original)
  {
    ch->sendln("Huh!?!");
    return eFAILURE;
  }
  else
  {
    ch->sendln("You return to your original body.");

    ch->desc->original->player->possesing = 0;
    ch->desc->character = ch->desc->original;
    ch->desc->original = 0;

    ch->desc->character->desc = ch->desc;
    ch->desc = 0;
    if (IS_NPC(ch) && DC::getInstance()->mob_index[ch->mobdata->nr].virt > 90 &&
        DC::getInstance()->mob_index[ch->mobdata->nr].virt < 100 &&
        cmd != cmd_t::LOOK)
    {
      act("$n evaporates.", ch, 0, 0, TO_ROOM, 0);
      extract_char(ch, true);
      return eSUCCESS | eCH_DIED;
    }
  }
  return eSUCCESS;
}

command_return_t Character::do_sockets(QStringList arguments, cmd_t cmd)
{
  QString searchkey;
  if (!arguments.isEmpty())
  {
    searchkey = arguments.at(0);
  }

  const Sockets sockets{this, searchkey};
  const QMap<QString, uint64_t> IPs = sockets.getIPs();
  const QList<Connection *> connections = sockets.getConnections();
  const uint64_t longest_IP_size = std::max(2UL, sockets.getLongestIPSize());
  const uint64_t longest_name_size = std::max(4UL, sockets.getLongestNameSize());
  const uint64_t longest_connection_state_size = std::max(5UL, sockets.getLongestConnectionStateSize());
  const uint64_t longest_idle_size = std::max(4UL, sockets.getLongestIdleSize() + 1);

  send(QStringLiteral("%1: %2 | %3 | %4 | %5$R\r\n").arg("Des").arg("Name", -longest_name_size).arg("State", -longest_connection_state_size).arg("Idle", -longest_idle_size).arg("IP", -longest_IP_size));
  for (const auto &d : connections)
  {
    const QString connection_character_name = d->getName();
    const QString IPstr = d->getPeerFullAddressString();
    const auto descriptor = d->descriptor;
    const bool duplicate_IP = IPs[d->getPeerOriginalAddress().toString()] > 1;
    const QString connected_state = constindex(d->connected, DC::connected_states);
    const QString idle_seconds = QStringLiteral("%1s").arg(d->idle_time / DC::PASSES_PER_SEC);

    send(QStringLiteral("%1%2: %3 | %4 | %5 | %6$R\r\n").arg(duplicate_IP ? "$B$4" : "").arg(descriptor, 3).arg(connection_character_name, -longest_name_size).arg(connected_state, -longest_connection_state_size).arg(idle_seconds, -longest_idle_size).arg(IPstr, -longest_IP_size));
  }

  const uint64_t num_can_see = connections.size();
  send(QStringLiteral("\r\nThere are %1 connections.\r\n").arg(num_can_see));

  return eSUCCESS;
}

int do_setvote(Character *ch, char *arg, cmd_t cmd)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  void send_info(char *);
  half_chop(arg, buf, buf2);

  if (!*buf)
  {
    ch->send("Syntax: voteset <question|add|remove|clear|start|end> <std::string>");
    return eFAILURE;
  }

  auto dc = DC::getInstance();
  if (!strcmp(buf, "start"))
  {
    dc->DCVote.StartVote(ch);
    return eSUCCESS;
  }

  if (!strcmp(buf, "clear"))
  {
    dc->DCVote.Reset(ch);
    return eSUCCESS;
  }

  if (!strcmp(buf, "end"))
  {
    dc->DCVote.EndVote(ch);
    return eSUCCESS;
  }

  if (!*buf2)
  {
    ch->send("Syntax: voteset <question|add|remove|clear|start|end> <std::string>");
    return eFAILURE;
  }

  if (!strcmp(buf, "question"))
  {
    dc->DCVote.SetQuestion(ch, buf2);
    return eSUCCESS;
  }
  if (!strcmp(buf, "add"))
  {
    dc->DCVote.AddAnswer(ch, buf2);
    return eSUCCESS;
  }
  if (!strcmp(buf, "remove"))
  {
    dc->DCVote.RemoveAnswer(ch, (unsigned int)atoi(buf2));
    return eSUCCESS;
  }

  ch->send("Syntax: voteset <question|add|remove|clear|start|end> <std::string>");
  return eFAILURE;
}

int do_punish(Character *ch, char *arg, cmd_t cmd)
{
  char name[100], buf[150];
  Character *vict;

  int i;

  if (IS_NPC(ch))
  {
    ch->sendln("Punish yourself!  Bad mob!");
    return eFAILURE;
  }

  arg = one_argument(arg, name);

  if (!*name)
  {
    ch->sendln("Punish who?");
    send_to_char("\n\rusage: punish <char> [stupid silence freeze noemote "
                 "notell noname noarena notitle nopray]\n\r",
                 ch);
    return eFAILURE;
  }

  if (!(vict = get_pc_vis(ch, name)))
  {
    snprintf(buf, sizeof(buf), "%s not found.\r\n", name);
    ch->send(buf);
    return eFAILURE;
  }

  one_argument(arg, name);

  if (!name[0])
  {
    display_punishes(ch, vict);
    return eFAILURE;
  }

  i = strlen(name);

  if (i > 5)
    i = 5;

  if (vict->getLevel() > ch->getLevel())
  {
    act("$E might object to that.. better not.", ch, 0, vict, TO_CHAR, 0);
    return eFAILURE;
  }
  if (!strncasecmp(name, "stupid", i))
  {
    if (isSet(vict->player->punish, PUNISH_STUPID))
    {
      vict->sendln("You feel a sudden onslaught of wisdom!");
      ch->sendln("STUPID removed.");
      sprintf(buf, "%s removes %s's stupid", GET_NAME(ch), GET_NAME(vict));
      logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
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
      sprintf(buf, "You have been lobotomized by %s!\n\r", GET_NAME(ch));
      vict->send(buf);
      ch->sendln("STUPID set.");
      sprintf(buf, "%s lobotimized %s", GET_NAME(ch), GET_NAME(vict));
      logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
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
      sprintf(buf, "%s removes %s's silence", GET_NAME(ch), GET_NAME(vict));
      logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    }
    else
    {
      sprintf(buf, "You have been silenced by %s!\n\r", GET_NAME(ch));
      vict->send(buf);
      ch->sendln("SILENCE set.");
      sprintf(buf, "%s silenced %s", GET_NAME(ch), GET_NAME(vict));
      logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_SILENCED);
  }
  if (!strncasecmp(name, "freeze", i))
  {
    if (isSet(vict->player->punish, PUNISH_FREEZE))
    {
      vict->sendln("You now can do things again.");
      ch->sendln("FREEZE removed.");
      sprintf(buf, "%s unfrozen by %s", GET_NAME(vict), GET_NAME(ch));
      logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    }
    else
    {
      sprintf(buf, "%s takes away your ability to....\r\n", GET_NAME(ch));
      vict->send(buf);
      ch->sendln("FREEZE set.");
      sprintf(buf, "%s frozen by %s", GET_NAME(vict), GET_NAME(ch));
      logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
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
      sprintf(buf, "%s takes away your ability to join arenas!\n\r", GET_NAME(ch));
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
      sprintf(buf, "%s takes away your ability to emote!\n\r", GET_NAME(ch));
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
      sprintf(buf, "%s takes away your ability to use telepathic communication!\n\r", GET_NAME(ch));
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
      sprintf(buf, "%s removes your ability to set your name!\n\r", GET_NAME(ch));
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
      sprintf(buf, "%s removes your ability to set your title!\n\r", GET_NAME(ch));
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
      sprintf(buf, "%s removes %s's unlucky.", GET_NAME(ch), GET_NAME(vict));
      logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    }
    else
    {
      if (!ch->player->stealth)
      {
        sprintf(buf, "%s curses you with god-given bad luck!\n\r", GET_NAME(ch));
        ch->sendln("UNLUCKY set.");
      }
      vict->send(buf);
      sprintf(buf, "%s makes %s unlucky.", GET_NAME(ch), GET_NAME(vict));
      logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
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
      csendf(vict, "%s has removed your ability to pray!\n\r", GET_NAME(ch));
      ch->sendln("NOPRAY (nemke) set.");
    }
    TOGGLE_BIT(vict->player->punish, PUNISH_NOPRAY);
  }

  display_punishes(ch, vict);
  return eSUCCESS;
}

void display_punishes(Character *ch, Character *vict)
{
  char buf[100];

  sprintf(buf, "$3Punishments for %s$R: ", GET_NAME(vict));
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

int do_colors(Character *ch, char *argument, cmd_t cmd)
{
  char buf[200];

  send_to_char("Color codes are a $$ followed by a code.\r\n\r\n"
               " Code   Bold($$B)  Inverse($$I)  Both($$B$$I)\r\n",
               ch);

  ch->sendln("  $$0$0)   $B********$R  $0$I***********$R  $0$B$I**********$R (plain 0 can't be seen normally)");
  for (int i = 1; i < 8; i++)
  {
    sprintf(buf, "  $$$%d%d)   $B********$R  $%d$I***********$R  $%d$B$I**********$R\r\n", i, i, i, i);
    ch->send(buf);
  }
  send_to_char("\r\nTo return to 'normal' color use $$R.\r\n\r\n"
               "Example:  'This is $$Bbold and $$4bold $$R$$4red$R!' will print:\r\n"
               "           This is $Bbold and $4bold $R$4red$R!\r\n",
               ch);
  return eSUCCESS;
}
