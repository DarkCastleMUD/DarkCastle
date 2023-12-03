/********************************
| Level 101 wizard commands
| 11/20/95 -- Azrack
**********************/
#include <queue>
#include <string>

#include <fmt/format.h>
#include <expected>

#include "wizard.h"
#include "utility.h"
#include "mobile.h"
#include "levels.h"
#include "interp.h"
#include "room.h"
#include "obj.h"
#include "character.h"
#include "terminal.h"
#include "handler.h"
#include "player.h"
#include "connect.h"
#include "returnvals.h"
#include "spells.h"
#include "const.h"

std::queue<std::string> imm_history;
std::queue<std::string> imp_history;

#define MAX_MESSAGE_LENGTH 4096

int do_wizhelp(Character *ch, char *argument, int cmd_arg)
{
  extern struct command_info cmd_info[];

  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  int cmd;
  int no = 6;
  int no2 = 6;
  int no3 = 6;

  if (IS_NPC(ch))
    return eFAILURE;

  buf[0] = '\0';
  buf2[0] = '\0';
  buf3[0] = '\0';

  if (argument && *argument)
  {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);
    one_argument(argument, arg2);
    sprintf(buf, "Arg1: %s\n", arg);
    send_to_char(buf, ch);
    sprintf(buf, "Arg2: %s\n", arg2);
    send_to_char(buf, ch);
    return eSUCCESS;
  }
  send_to_char("Here are your godly powers:\n\r\n\r", ch);

  int v;
  for (v = ch->getLevel(); v > 100; v--)
    for (cmd = 0; cmd_info[cmd].command_name[0] != '\0'; cmd++)
    {
      if (cmd_info[cmd].minimum_level == GIFTED_COMMAND && v == ch->getLevel())
      {
        auto bestow_command = get_bestow_command(cmd_info[cmd].command_name);

        if (!bestow_command.has_value()) // someone forgot to update it
          continue;

        if (!ch->has_skill(bestow_command->num))
          continue;

        if (bestow_command->testcmd == false)
        {
          sprintf(buf2 + strlen(buf2), "[GFT]%-11s", cmd_info[cmd].command_name);
          if ((no2) % 5 == 0)
            strcat(buf2, "\n\r");
          no2++;
        }
        else
        {
          sprintf(buf3 + strlen(buf3), "[TST]%-11s", cmd_info[cmd].command_name);
          if ((no3) % 5 == 0)
            strcat(buf3, "\n\r");
          no3++;
        }

        continue;
      }
      if (cmd_info[cmd].minimum_level != v || cmd_info[cmd].minimum_level == GIFTED_COMMAND)
        continue;

      // ignore these 2 duplicates of other commands
      if (cmd_info[cmd].command_name == "colours" || cmd_info[cmd].command_name == ";")
        continue;

      sprintf(buf + strlen(buf), "[%2d]%-11s",
              cmd_info[cmd].minimum_level,
              cmd_info[cmd].command_name);

      if ((no) % 5 == 0)
        strcat(buf, "\n\r");
      no++;
    }

  strcat(buf, "\n\r\n\r"
              "Here are the godly powers that have been gifted to you:\n\r\n\r");
  strcat(buf, buf2);
  strcat(buf, "\r\n");
  strcat(buf, "Here are the godly test powers that have been gifted to you:\n\r\n\r");
  strcat(buf, buf3);
  strcat(buf, "\r\n");

  send_to_char(buf, ch);
  return eSUCCESS;
}

command_return_t Character::do_goto(QStringList arguments, int cmd)
{
  int loc_nr = {}, location = -1, i = {}, start_room = {};
  zone_t zone_nr = {};
  Character *target_mob = {}, *pers = {};
  Character *tmp_ch = {};
  struct follow_type *k = {}, *next_dude = {};
  class Object *target_obj = {};
  extern room_t top_of_world;

  if (IS_NPC(this))
  {
    return eFAILURE;
  }
  start_room = in_room;

  if (arguments.isEmpty())
  {
    send("You must supply a room number, a name or zone <zone number>.\r\n");
    return eFAILURE;
  }

  QString arg1 = arguments.at(0);
  bool ok = false;
  if (arg1 == "zone" || arg1 == "z")
  {

    if (arguments.size() < 2)
    {
      send("No zone number specified.\r\n");
      return eFAILURE;
    }
    QString arg2 = arguments.at(1);

    zone_t zone_key = arg2.toULongLong(&ok);
    auto &zones = DC::getInstance()->zones;
    if (ok == false || zones.contains(zone_key) == false)
    {
      if (zones.isEmpty())
      {
        send(QString("Invalid zone %1 specified. No zones loaded.\r\n").arg(zone_key));
        return eFAILURE;
      }

      auto last_zone_nr = zones.lastKey();
      send(QString("Invalid zone %1 specified. Valid values are 1-%2\r\n").arg(zone_key).arg(last_zone_nr));
      return eFAILURE;
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

    send(fmt::format("Going to room {} in zone #{} [{}]\r\n", loc_nr, zone_key, ltrim(std::string(DC::getInstance()->zones.value(zone_key).name.toStdString()))));

    if (loc_nr > top_of_world || loc_nr < 0)
    {
      send("No room exists with that number.\r\n");
      return eFAILURE;
    }

    if (DC::getInstance()->rooms.contains(loc_nr))
    {
      location = loc_nr;
    }
    else
    {
      if (can_modify_this_room(this, loc_nr))
      {
        if (create_one_room(this, loc_nr))
        {
          send("You form order out of chaos.\r\n");
          location = loc_nr;
        }
      }
    }
    if (location == -1)
    {
      send("No room exists with that number.\r\n");
      return eFAILURE;
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
        send(QString("Going to player %1 in room %2.\r\n").arg(GET_NAME(target_mob)).arg(location));
      }
      else if ((target_mob = getVisibleCharacter(arg1)))
      {
        location = target_mob->in_room;
        send(QString("Going to character %1 in room %2.\r\n").arg(GET_NAME(target_mob)).arg(location));
      }
      else if ((target_obj = getVisibleObject(arg1)))
      {
        if (target_obj->in_room != DC::NOWHERE)
        {
          location = target_obj->in_room;
          send(QString("Going to object %1 in room %2.\r\n").arg(GET_NAME(target_obj)).arg(location));
        }
        else
        {
          send("The object is not available.\r\n");
          return eFAILURE;
        }
      }
      else
      {
        send("No such creature or object around.\r\n");
        return eFAILURE;
      }
    }
    else
    {
      if (loc_nr > top_of_world || loc_nr < 0)
      {
        send("No room exists with that number.\r\n");
        return eFAILURE;
      }

      if (DC::getInstance()->rooms.contains(loc_nr))
        location = loc_nr;
      else
      {
        if (can_modify_this_room(this, loc_nr))
        {
          if (create_one_room(this, loc_nr))
          {
            send("You form order out of chaos.\n\r\n\r");
            location = loc_nr;
          }
        }
        else
        {
          send(QString("You can't modify room %1").arg(loc_nr));
          return eFAILURE;
        }
      }
      if (location == -1)
      {
        send("No room exists with that number.\r\n");
        return eFAILURE;
      }
    }
  }

  /* a location has been found. */
  if (DC::isSet(DC::getInstance()->world[location].room_flags, IMP_ONLY) &&
      level_ < OVERSEER)
  {
    send("That room is for implementers only.\r\n");
    return eFAILURE;
  }

  /* Let's keep 104-'s out of clan halls....sigh... */
  if (DC::isSet(DC::getInstance()->world[location].room_flags, CLAN_ROOM) &&
      level_ < DEITY)
  {
    send("For your protection, 104-'s may not be in clanhalls.\r\n");
    return eFAILURE;
  }

  if ((DC::isSet(DC::getInstance()->world[location].room_flags, PRIVATE)) && (level_ < OVERSEER))
  {

    for (i = 0, pers = DC::getInstance()->world[location].people; pers;
         pers = pers->next_in_room, i++)
      ;
    if (i > 1)
    {
      send("There's a private conversation going on in that room.\r\n");
      return eFAILURE;
    }
  }

  send("\r\n");

  if (!IS_MOB(this))
    for (tmp_ch = DC::getInstance()->world[in_room].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
    {
      if ((CAN_SEE(tmp_ch, this) && (tmp_ch != this) && !player->stealth) || (tmp_ch->getLevel() > level_ && tmp_ch->getLevel() > PATRON))
      {
        ansi_color(RED, tmp_ch);
        ansi_color(BOLD, tmp_ch);
        send_to_char(player->poofout, tmp_ch);
        send_to_char("\n\r", tmp_ch);
        ansi_color(NTEXT, tmp_ch);
      }
      else if (tmp_ch != this && !player->stealth)
      {
        ansi_color(RED, tmp_ch);
        ansi_color(BOLD, tmp_ch);
        send_to_char("Someone disappears in a puff of smoke.\r\n", tmp_ch);
        ansi_color(NTEXT, tmp_ch);
      }
    }

  move_char(this, location);

  if (!IS_MOB(this))
    for (tmp_ch = DC::getInstance()->world[in_room].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
    {
      if ((CAN_SEE(tmp_ch, this) && (tmp_ch != this) && !player->stealth) || (tmp_ch->getLevel() > level_ && tmp_ch->getLevel() > PATRON))
      {

        ansi_color(RED, tmp_ch);
        ansi_color(BOLD, tmp_ch);
        send_to_char(player->poofin, tmp_ch);
        send_to_char("\n\r", tmp_ch);
        ansi_color(NTEXT, tmp_ch);
      }
      else if (tmp_ch != this && !player->stealth)
      {
        ansi_color(RED, tmp_ch);
        ansi_color(BOLD, tmp_ch);
        send_to_char("Someone appears with an ear-splitting bang!\n\r", tmp_ch);
        ansi_color(NTEXT, tmp_ch);
      }
    }

  do_look(this, "", 15);

  if (followers)
    for (k = followers; k; k = next_dude)
    {
      next_dude = k->next;
      if (start_room == k->follower->in_room && CAN_SEE(k->follower, this) &&
          k->follower->getLevel() >= IMMORTAL)
      {
        csendf(k->follower, "You follow %s.\n\r\n\r", GET_SHORT(this));
        k->follower->do_goto(arguments, CMD_DEFAULT);
      }
    }
  return eSUCCESS;
}

int do_poof(Character *ch, char *arg, int cmd)
{
  char inout[100], buf[100];
  int ctr, nope;
  char _convert[2];

  if (IS_NPC(ch))
  {
    send_to_char("Mobs can't poof.\r\n", ch);
    return eFAILURE;
  }

  arg = one_argument(arg, inout);

  if (!*inout)
  {
    send_to_char("Usage:\n\rpoof [i|o] <std::string>\n\r", ch);
    send_to_char("\n\rCurrent poof in is:\n\r", ch);
    send_to_char(ch->player->poofin, ch);
    send_to_char("\n\r", ch);
    send_to_char("\n\rCurrent poof out is:\n\r", ch);
    send_to_char(ch->player->poofout, ch);
    send_to_char("\n\r", ch);
    return eSUCCESS;
  }

  if (inout[0] != 'i' && inout[0] != 'o')
  {
    send_to_char("Usage:\n\rpoof [i|o] <std::string>\n\r", ch);
    return eFAILURE;
  }

  if (!*arg)
  {
    send_to_char("A poof type message was expected.\r\n", ch);
    return eFAILURE;
  }

  if (strlen(arg) > 72)
  {
    send_to_char("Poof message too long, must be under 72 characters long.\r\n", ch);
    return eFAILURE;
  }

  nope = 0;

  for (ctr = 0; (unsigned)ctr <= strlen(arg); ctr++)
  {
    if (arg[ctr] == '%')
    {
      if (nope == 0)
        nope = 1;
      else if (nope == 1)
      {
        send_to_char("You can only include one % in your poofin ;)\n\r", ch);
        return eFAILURE;
      }
    }
  }

  if (nope == 0)
  {
    send_to_char("You MUST include your name. Use % to indicate where "
                 "you want it.\r\n",
                 ch);
    return eFAILURE;
  }

  /* For the first time, use strcpy to avoid that annoying space
     at the beginning
  */
  _convert[0] = arg[0];
  _convert[1] = '\0';
  if (arg[ctr] == '%')
    strcpy(buf, GET_NAME(ch));
  else
    strcpy(buf, _convert);

  /* No reason to assign _convert[1] every time through, is there? */
  for (ctr = 1; (unsigned)ctr < strlen(arg); ctr++)
  {
    _convert[0] = arg[ctr];

    if (arg[ctr] == '%')
      strcat(buf, GET_NAME(ch));
    else
      strcat(buf, _convert);
  }

  if (inout[0] == 'i')
  {
    if (ch->player->poofin)
      dc_free(ch->player->poofin);
    ch->player->poofin = str_dup(buf);
  }
  else
  {
    if (ch->player->poofout)
      dc_free(ch->player->poofout);
    ch->player->poofout = str_dup(buf);
  }

  send_to_char("Ok.\r\n", ch);
  return eSUCCESS;
}

int do_at(Character *ch, char *argument, int cmd)
{
  char command[MAX_INPUT_LENGTH], loc_str[MAX_INPUT_LENGTH];
  int loc_nr, location, original_loc;
  Character *target_mob;
  class Object *target_obj;
  // extern room_t top_of_world;

  if (IS_NPC(ch))
    return eFAILURE;

  half_chop(argument, loc_str, command);
  if (!*loc_str)
  {
    send_to_char("You must supply a room number or a name.\r\n", ch);
    return eFAILURE;
  }

  if (isdigit(*loc_str) && !strchr(loc_str, '.'))
  {
    loc_nr = atoi(loc_str);
    if ((loc_nr == 0 && *loc_str != '0') ||
        ((location = real_room(loc_nr)) < 0))
    {
      send_to_char("No room exists with that number.\r\n", ch);
      return eFAILURE;
    }
  }
  else if ((target_mob = get_char_vis(ch, loc_str)) != nullptr)
    location = target_mob->in_room;
  else if ((target_obj = get_obj_vis(ch, loc_str)) != nullptr)
    if (target_obj->in_room != DC::NOWHERE)
      location = target_obj->in_room;
    else
    {
      send_to_char("The object is not available.\r\n", ch);
      return eFAILURE;
    }
  else
  {
    send_to_char("No such creature or object around.\r\n", ch);
    return eFAILURE;
  }

  /* a location has been found. */
  if (DC::isSet(DC::getInstance()->world[location].room_flags, IMP_ONLY) && ch->getLevel() < IMPLEMENTER)
  {
    send_to_char("No.\r\n", ch);
    return eFAILURE;
  }

  /* Let's keep 104-'s out of clan halls....sigh... */
  if (DC::isSet(DC::getInstance()->world[location].room_flags, CLAN_ROOM) &&
      ch->getLevel() < DEITY)
  {
    send_to_char("For your protection, 104-'s may not be in clanhalls.\r\n", ch);
    return eFAILURE;
  }

  original_loc = ch->in_room;
  move_char(ch, location, false);
  int retval = ch->command_interpreter(command);

  /* check if the guy's still there */
  for (target_mob = DC::getInstance()->world[location].people; target_mob; target_mob =
                                                                               target_mob->next_in_room)
    if (ch == target_mob)
    {
      move_char(ch, original_loc);
    }
  return retval;
}

int do_highfive(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char buf[200];

  if (IS_NPC(ch))
    return eFAILURE;

  one_argument(argument, buf);
  if (!*buf)
  {
    send_to_char("Who do you wish to high-five? \n\r", ch);
    return eFAILURE;
  }

  if (!(victim = get_char_vis(ch, buf)))
  {
    send_to_char("No-one by that name in the world.\r\n", ch);
    return eFAILURE;
  }

  if (victim->getLevel() < IMMORTAL)
  {
    send_to_char("What you wanna give a mortal a high-five for?! *smirk* \n\r", ch);
    return eFAILURE;
  }

  if (ch == victim)
  {
    sprintf(buf, "%s conjures a clap of thunder to resound the land!\n\r", GET_SHORT(ch));
    send_to_all(buf);
  }
  else
  {
    sprintf(buf, "Time stops for a minute as %s and %s high-five!\n\r", GET_SHORT(ch), GET_SHORT(victim));
    send_to_all(buf);
  }
  return eSUCCESS;
}

int do_holylite(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (argument[0] != '\0')
  {
    send_to_char(
        "HOLYLITE doesn't take any arguments; arg ignored.\r\n", ch);
  } /* if */

  if (ch->player->holyLite)
  {
    ch->player->holyLite = false;
    send_to_char("Holy light mode off.\r\n", ch);
  }
  else
  {
    ch->player->holyLite = true;
    send_to_char("Holy light mode on.\r\n", ch);
  } /* if */
  return eSUCCESS;
}

int do_wizinvis(Character *ch, char *argument, int cmd)
{
  char buf[200];

  int arg1;

  if (IS_NPC(ch))
  {
    return eFAILURE;
  }

  arg1 = atoi(argument);

  if (arg1 < 0)
    arg1 = 0;

  if (!*argument)
  {
    if (ch->player->wizinvis == 0)
    {
      ch->player->wizinvis = ch->getLevel();
    }
    else
    {
      ch->player->wizinvis = 0;
    }
  }
  else
  {
    if (arg1 > ch->getLevel())
      arg1 = ch->getLevel();
    ch->player->wizinvis = arg1;
  }
  sprintf(buf, "WizInvis Set to: %ld \n\r", ch->player->wizinvis);
  send_to_char(buf, ch);
  return eSUCCESS;
}

int do_nohassle(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (DC::isSet(ch->player->toggles, Player::PLR_NOHASSLE))
  {
    REMOVE_BIT(ch->player->toggles, Player::PLR_NOHASSLE);
    send_to_char("Mobiles can bother you again.\r\n", ch);
  }
  else
  {
    SET_BIT(ch->player->toggles, Player::PLR_NOHASSLE);
    send_to_char("Those pesky mobiles will leave you alone now.\r\n", ch);
  }
  return eSUCCESS;
}

// cmd == CMD_DEFAULT - imm
// cmd == 8 - /
command_return_t do_wiz(Character *ch, std::string argument, int cmd)
{
  std::string buf1 = {};
  Connection *i = nullptr;

  if (IS_NPC(ch))
  {
    return eFAILURE;
  }

  if (cmd == CMD_IMPCHAN && !ch->has_skill(COMMAND_IMP_CHAN))
  {
    send_to_char("Huh?\r\n", ch);
    return eFAILURE;
  }

  argument = ltrim(argument);
  argument = rtrim(argument);

  if (argument.empty())
  {
    std::queue<std::string> tmp;
    if (cmd == CMD_IMMORT)
    {
      tmp = imm_history;
      send_to_char("Here are the last 10 imm messages:\r\n", ch);
    }
    else if (cmd == CMD_IMPCHAN)
    {
      tmp = imp_history;
      send_to_char("Here are the last 10 imp messages:\r\n", ch);
    }
    else
    {
      send_to_char("What? How did you get here?? Contact a coder.\r\n", ch);
      return eSUCCESS;
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

    if (cmd == CMD_IMMORT)
    {
      buf1 = fmt::format("$B$4{}$7: $7$B{}$R\r\n", GET_SHORT(ch), argument);
      imm_history.push(buf1);
      if (imm_history.size() > 10)
        imm_history.pop();
    }
    else
    {
      buf1 = fmt::format("$B$7{}> {}$R\r\n", GET_SHORT(ch), argument);
      imp_history.push(buf1);
      if (imp_history.size() > 10)
        imp_history.pop();
    }

    send_to_char(buf1, ch);
    ansi_color(NTEXT, ch);

    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
    {
      if (i->character && i->character != ch && i->character->getLevel() >= IMMORTAL && IS_PC(i->character))
      {
        if (cmd == CMD_IMPCHAN && !i->character->has_skill(COMMAND_IMP_CHAN))
          continue;

        if (STATE(i) == Connection::states::PLAYING)
        {
          send_to_char(buf1, i->character);
        }
        else
        {
          record_msg(buf1.c_str(), i->character);
        }
      }
    }
  }
  return eSUCCESS;
}

int do_findfix(Character *ch, char *argument, int cmd)
{
  for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
  {
    for (qsizetype j = 0; j < zone.cmd.size(); j++)
    {
      bool first = true;
      bool found = false;
      if (zone.cmd[j]->command != 'M')
        continue;

      int vnum = zone.cmd[j]->arg1;
      int max = zone.cmd[j]->arg2;
      if (max == 1 || max == -1)
        continue; // Don't care about those..

      int amt = 0;
      for (qsizetype z = 0; z < zone.cmd.size(); z++)
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
        ch->send(QString("Reset %1 in zone %2: %3 reset commands OVER %4 max in world.\r\n").arg(j + 1).arg(zone_key).arg(amt).arg(max));
        char *buffer = strdup(QString("%1 list %2 1").arg(zone_key).arg(j + 1).toStdString().c_str());
        do_zedit(ch, buffer, CMD_DEFAULT);
        free(buffer);
      }
      else
      {
        ch->send(QString("Reset %1 in zone %2: %3 reset commands UNDER %4 max in world.\r\n").arg(j + 1).arg(zone_key).arg(amt).arg(max));
        char *buffer = strdup(QString("%1 list %2 1").arg(zone_key).arg(j + 1).toStdString().c_str());
        do_zedit(ch, buffer, CMD_DEFAULT);
        free(buffer);
      }
    }
  }
  return eSUCCESS;
}

int do_varstat(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg);
  Character *vict;

  if ((vict = get_char_vis(ch, arg)) == nullptr)
  {
    send_to_char("Target not found.\r\n", ch);
    return eFAILURE;
  }
  char buf[MAX_STRING_LENGTH];
  buf[0] = '\0';
  struct tempvariable *eh;
  for (eh = vict->tempVariable; eh; eh = eh->next)
  {
    sprintf(buf, "$B$3%-30s $R-- $B$5 %s\r\n",
            eh->name, eh->data);
    send_to_char(buf, ch);
  }
  if (buf[0] == '\0')
  {
    send_to_char("No temporary variables found.\r\n", ch);
  }
  return eSUCCESS;
}
