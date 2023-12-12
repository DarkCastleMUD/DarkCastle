/***************************************************************************
 *  file: act_comm.c , Implementation of commands.         Part of DIKUMUD *
 *  Usage : Communication.                                                 *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec\\@garnet.berkeley.edu.                                          *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
#include <string.h>
#include <assert.h>
#include <cstdint>

#include <fmt/format.h>
#include <QFile>

#include "DC.h"
#include "character.h"
#include "terminal.h"
#include "connect.h"
#include "levels.h"
#include "room.h"
#include "mobile.h"
#include "player.h"
#include "obj.h"
#include "handler.h"
#include "interp.h"
#include "utility.h"
#include "act.h"
#include "db.h"
#include "returnvals.h"
#include "fileinfo.h"
#include "const.h"

extern bool MOBtrigger;

/* extern functions */
int is_busy(Character *ch);

int do_report(Character *ch, char *argument, int cmd)
{
  char buf[256];
  char report[200];

  assert(ch != 0);
  if (ch->in_room == DC::NOWHERE)
  {
    logentry("NOWHERE sent to do_report!", OVERSEER, LogChannels::LOG_BUG);
    return eSUCCESS;
  }

  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eSUCCESS;
  }

  if (IS_PC(ch) && argument != nullptr)
  {
    std::string arg1, remainder_args;
    std::tie(arg1, remainder_args) = half_chop(std::string(argument));
    if (arg1 == "help")
    {
      csendf(ch, "report       - Reports hps, mana, moves and ki. (default)\n\r");
      csendf(ch, "report xp    - Reports current xp, xp till next level and levels to be gained.\r\n");
      csendf(ch, "report help  - Shows different ways report can be used.\r\n");
      return eFAILURE;
    }

    if (arg1 == "xp")
    {
      // calculate how many levels a player could gain with their current XP
      uint8_t levels_to_gain = 0;
      int64_t players_exp = GET_EXP(ch);
      while (levels_to_gain < IMMORTAL)
      {
        if (players_exp >= (int64_t)exp_table[(int)ch->getLevel() + levels_to_gain + 1])
        {
          players_exp -= (int64_t)exp_table[(int)ch->getLevel() + levels_to_gain + 1];
          levels_to_gain++;
        }
        else
        {
          break;
        }
      }

      snprintf(report, 200, "XP: %ld, XP till level: %ld, Levels to gain: %u",
               GET_EXP(ch),
               (int64_t)(exp_table[(int)ch->getLevel() + 1] - (int64_t)GET_EXP(ch)),
               levels_to_gain);

      sprintf(buf, "$n reports '%s'", report);
      act(buf, ch, 0, 0, TO_ROOM, 0);

      ch->send(QString("You report: %1\n\r").arg(report));
      return eSUCCESS;
    }
  }

  if (IS_NPC(ch) || IS_ANONYMOUS(ch))
    snprintf(report, 200, "%d%% hps, %d%% mana, %d%% movement, and %d%% ki.",
             MAX(1, ch->getHP() * 100) / MAX(1, GET_MAX_HIT(ch)),
             MAX(1, GET_MANA(ch) * 100) / MAX(1, GET_MAX_MANA(ch)),
             MAX(1, GET_MOVE(ch) * 100) / MAX(1, GET_MAX_MOVE(ch)),
             MAX(1, GET_KI(ch) * 100) / MAX(1, GET_MAX_KI(ch)));
  else
  {
    snprintf(report, 200, "%d/%d hps, %d/%d mana, %d/%d movement, and %d/%d ki.",
             ch->getHP(), GET_MAX_HIT(ch),
             GET_MANA(ch), GET_MAX_MANA(ch),
             GET_MOVE(ch), GET_MAX_MOVE(ch),
             GET_KI(ch), GET_MAX_KI(ch));
  }

  sprintf(buf, "$n reports '%s'", report);
  act(buf, ch, 0, 0, TO_ROOM, 0);

  ch->send(QString("You report: %1\n\r").arg(report));

  return eSUCCESS;
}

/************************************************************************
| send_to_gods
| Preconditions: str != 0
| Postconditions: None
| Side effects: None
| Returns: 0 on failure, non-zero on success
| Notes:
*/
int send_to_gods(QString message, uint64_t god_level, LogChannels type)
{
  QString buf1;
  QString buf;
  QString typestr;
  Connection *i = nullptr;

  if (message.isEmpty())
  {
    logentry("nullptr STRING sent to send_to_gods!", OVERSEER, LogChannels::LOG_BUG);
    return (0);
  }

  if ((god_level > IMPLEMENTER) || (god_level < 0))
  { // Outside valid god levels
    return (0);
  }

  switch (type)
  {
  case LogChannels::LOG_BUG:
    typestr = "bug";
    break;
  case LogChannels::LOG_PRAYER:
    typestr = "pray";
    break;
  case LogChannels::LOG_GOD:
    typestr = "god";
    break;
  case LogChannels::LOG_MORTAL:
    typestr = "mortal";
    break;
  case LogChannels::LOG_SOCKET:
    typestr = "socket";
    break;
  case LogChannels::LOG_MISC:
    typestr = "misc";
    break;
  case LogChannels::LOG_PLAYER:
    typestr = "player";
    break;
  case LogChannels::LOG_WORLD:
    typestr = "world";
    break;
  case LogChannels::LOG_ARENA:
    typestr = "arena";
    break;
  case LogChannels::LOG_CLAN:
    typestr = "logclan";
    break;
  case LogChannels::LOG_WARNINGS:
    typestr = "warnings";
    break;
  case LogChannels::LOG_DATABASE:
    typestr = "database";
    break;
  case LogChannels::LOG_VAULT:
    typestr = "vault";
    break;
  case LogChannels::LOG_HELP:
    typestr = "help";
    break;
  case LogChannels::LOG_OBJECTS:
    typestr = "objects";
    break;
  case LogChannels::LOG_QUEST:
    typestr = "quest";
    break;
  case LogChannels::LOG_DEBUG:
    typestr = "debug";
    break;
  default:
    typestr = "unknown";
    break;
  }

  buf = QString("//(%1) %2\n\r").arg(typestr).arg(message);
  buf1 = QString("%1%2//%3(%4)%5 %6%7 %8%9%10\n\r").arg(BOLD).arg(RED).arg(NTEXT).arg(typestr).arg(BOLD).arg(YELLOW).arg(message).arg(RED).arg(NTEXT).arg(GREY);

  for (i = DC::getInstance()->descriptor_list; i; i = i->next)
  {
    if ((i->character == nullptr) || (i->character->getLevel() <= MORTAL))
      continue;
    if (!(DC::isSet(i->character->misc, type)))
      continue;
    if (is_busy(i->character))
      continue;
    if (!i->connected && i->character->getLevel() >= god_level)
    {
      if (i->character->isNPC() || DC::isSet(i->character->player->toggles, Player::PLR_ANSI))
        send_to_char(buf1, i->character);
      else
        i->character->send(buf);
    }
  }
  return (1);
}

int do_channel(Character *ch, char *arg, int cmd)
{
  int x;
  int y = 0;
  char buf[200];
  char buf2[200];

  char *on_off[] = {
      "$B$4off$R",
      "$B$2on$R"};

  char *types[] = {
      "bug", // 0
      "prayer",
      "god",
      "mortal",
      "socket",
      "misc", // 5
      "player",
      "gossip", // 7
      "auction",
      "info",
      "trivia", // 10
      "dream",
      "clan",
      "newbie",
      "shout",
      "world", // 15
      "arena",
      "logclan",
      "warnings",
      "help",
      "database", // 20
      "objects",
      "tell",
      "hints",
      "vault",
      "quest", // 25
      "debug",
      "\\@"};

  if (IS_NPC(ch))
    return eSUCCESS;

  if (*arg)
    one_argument(arg, buf);

  else
  {
    //    send_to_char("\n\r", ch);

    if (ch->getLevel() < IMMORTAL)
    {
      for (x = 7; x <= 14; x++)
      {
        if (DC::isSet(ch->misc, (1 << x)))
          y = 1;
        else
          y = 0;
        sprintf(buf2, "%-9s%s\n\r", types[x], on_off[y]);
        send_to_char(buf2, ch);
      }
    }
    else
    {
      int o = ch->getLevel() == 110 ? 21 : 19;
      for (x = 0; x <= o; x++)
      {
        if (DC::isSet(ch->misc, (1 << x)))
          y = 1;
        else
          y = 0;
        sprintf(buf2, "%-9s%s\n\r", types[x], on_off[y]);
        send_to_char(buf2, ch);
      }
    }

    if (DC::isSet(ch->misc, 1 << 22))
      y = 1;
    else
      y = 0;
    sprintf(buf2, "%-9s%s\n\r", types[22], on_off[y]);
    send_to_char(buf2, ch);

    if (DC::isSet(ch->misc, 1 << 23))
      y = 1;
    else
      y = 0;
    sprintf(buf2, "%-9s%s\n\r", types[23], on_off[y]);
    send_to_char(buf2, ch);

    int o = ch->getLevel() == 110 ? 26 : 0;
    for (x = 24; x <= o; x++)
    {
      if (DC::isSet(ch->misc, (1 << x)))
        y = 1;
      else
        y = 0;
      sprintf(buf2, "%-9s%s\n\r", types[x], on_off[y]);
      send_to_char(buf2, ch);
    }

    return eSUCCESS;
  }

  for (x = 0; x <= 27; x++)
  {
    if (x == 27)
    {
      ch->sendln("That type was not found.");
      return eSUCCESS;
    }
    if (is_abbrev(buf, types[x]))
      break;
  }

  if (ch->getLevel() < IMMORTAL &&
      (x < 7 || (x > 14 && x < 22)))
  {
    ch->sendln("That type was not found.");
    return eSUCCESS;
  }
  if (x > 19 && ch->getLevel() != 110 && x < 22)
  {
    ch->sendln("That type was not found.");
    return eSUCCESS;
  }
  if (DC::isSet(ch->misc, (1 << x)))
  {
    sprintf(buf, "%s channel turned $B$4OFF$R.\r\n", types[x]);
    ch->send(buf);
    REMOVE_BIT(ch->misc, (1 << x));
  }
  else
  {
    sprintf(buf, "%s channel turned $B$2ON$R.\r\n", types[x]);
    ch->send(buf);
    SET_BIT(ch->misc, (1 << x));
  }
  return eSUCCESS;
}

command_return_t do_ignore(Character *ch, std::string args, int cmd)
{
  if (ch == nullptr)
  {
    return eFAILURE;
  }

  if (IS_MOB(ch))
  {
    ch->send("You're a mob! You can't ignore people.\r\n");
    return eFAILURE;
  }

  if (args.empty())
  {
    if (ch->player->ignoring.empty())
    {
      ch->send("Ignore who?\r\n");
      return eSUCCESS;
    }

    // convert ignoring std::map into "char1 char2 char3" format
    std::string ignoreString = {};
    for (const auto &ignore : ch->player->ignoring)
    {
      if (ignore.second.ignore)
      {
        if (ignoreString.empty())
        {
          ignoreString = ignore.first;
        }
        else
        {
          ignoreString = ignoreString + " " + ignore.first;
        }
      }
    }
    ch->send(fmt::format("Ignoring: {}\r\n", ignoreString));

    return eSUCCESS;
  }

  std::string arg1 = {}, remainder_args = {};
  std::tie(arg1, remainder_args) = half_chop(args);
  if (arg1.empty())
  {
    ch->send("Ignore who?\r\n");
    return eFAILURE;
  }
  arg1[0] = toupper(arg1[0]);

  if (ch->player->ignoring.contains(arg1) == false)
  {
    ch->player->ignoring[arg1] = {true, 0};
    ch->send(fmt::format("You now ignore anyone named {}.\r\n", arg1));
  }
  else
  {
    ch->player->ignoring.erase(arg1);
    ch->send(fmt::format("You stop ignoring {}.\r\n", arg1));
  }
  return eSUCCESS;
}

int is_ignoring(const Character *const ch, const Character *const i)
{
  if (IS_MOB(ch) || (i->getLevel() >= IMMORTAL && IS_PC(i)) || ch->player->ignoring.empty())
  {
    return false;
  }

  if (GET_NAME(i) == nullptr)
  {
    return false;
  }

  if (ch->player->ignoring.contains(GET_NAME(i)))
  {
    return true;
  }

  // Since it didn't match the whole name, see if it matches one of
  // the name keywords used for a mob name
  std::string names = GET_NAME(i);
  std::string name1 = {}, remainder_names = {};
  std::tie(name1, remainder_names) = half_chop(names);
  while (name1.empty() == false)
  {
    if (ch->player->ignoring.contains(name1))
    {
      return true;
    }

    std::tie(name1, remainder_names) = half_chop(remainder_names);
  }

  return false;
}

#define MAX_NOTE_LENGTH 1000 /* arbitrary */

int do_write(Character *ch, char *argument, int cmd)
{
  class Object *paper = 0, *pen = 0;
  char papername[MAX_INPUT_LENGTH], penname[MAX_INPUT_LENGTH],
      buf[MAX_STRING_LENGTH];

  argument_interpreter(argument, papername, penname);

  if (!ch->desc)
    return eSUCCESS;
  if (ch->getLevel() < 5)
  {
    ch->sendln("You need to be at least level 5 to write on the board.");
    return eSUCCESS;
  }

  if (!*papername) /* nothing was delivered */
  {
    send_to_char(
        "Write? with what? ON what? what are you trying to do??\n\r", ch);
    return eSUCCESS;
  }

  if (*penname) /* there were two arguments */
  {
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
    {
      sprintf(buf, "You have no %s.\r\n", papername);
      ch->send(buf);
      return eSUCCESS;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying)))
    {
      sprintf(buf, "You have no %s.\r\n", papername);
      ch->send(buf);
      return eSUCCESS;
    }
  }
  else /* there was one arg.let's see what we can find */
  {
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
    {
      sprintf(buf, "There is no %s in your inventory.\r\n", papername);
      ch->send(buf);
      return eSUCCESS;
    }
    if (paper->obj_flags.type_flag == ITEM_PEN) /* oops, a pen.. */
    {
      pen = paper;
      paper = 0;
    }
    else if (paper->obj_flags.type_flag != ITEM_NOTE)
    {
      ch->sendln("That thing has nothing to do with writing.");
      return eSUCCESS;
    }

    /* one object was found. Now for the other one. */
    if (!ch->equipment[HOLD])
    {
      sprintf(buf, "You can't write with a %s alone.\r\n", papername);
      ch->send(buf);
      return eSUCCESS;
    }
    if (!CAN_SEE_OBJ(ch, ch->equipment[HOLD]))
    {
      send_to_char("The stuff in your hand is invisible! Yeech!!\n\r", ch);
      return eSUCCESS;
    }

    if (pen)
      paper = ch->equipment[HOLD];
    else
      pen = ch->equipment[HOLD];
  }

  /* ok.. now let's see what kind of stuff we've found */
  if (pen->obj_flags.type_flag != ITEM_PEN)
  {
    act("$p is no good for writing with.", ch, pen, 0, TO_CHAR, 0);
  }
  else if (paper->obj_flags.type_flag != ITEM_NOTE)
  {
    act("You can't write on $p.", ch, paper, 0, TO_CHAR, 0);
  }
  else if (paper->action_description)
    /*    else if (paper->item_number != real_object(1205) )  */
    ch->sendln("There's something written on it already.");
  else
  {
    /* we can write - hooray! */

    send_to_char("Ok.. go ahead and write.. end the note with a \\@.\r\n",
                 ch);
    act("$n begins to jot down a note.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    ch->desc->strnew = &paper->action_description;
    ch->desc->max_str = MAX_NOTE_LENGTH;
  }
  return eSUCCESS;
}

// TODO - Add a bunch of insults to this for the hell of it.
int do_insult(Character *ch, char *argument, int cmd)
{
  char buf[100];
  char arg[MAX_STRING_LENGTH];
  Character *victim;

  one_argument(argument, arg);

  if (*arg)
  {
    if (!(victim = ch->get_char_room_vis( arg)))
    {
      send_to_char("Can't hear you!\n\r", ch);
    }
    else
    {
      if (victim != ch)
      {
        sprintf(buf, "You insult %s.\r\n", GET_SHORT(victim));
        ch->send(buf);

        switch (number(0, 3))
        {
        case 0:
          if (GET_SEX(victim) == SEX_MALE)
            act("$n accuses you of fighting like a woman!", ch, 0, victim, TO_VICT, 0);
          else
            act("$n says that women can't fight.", ch, 0, victim, TO_VICT, 0);
          break;
        case 1:
          if (GET_SEX(victim) == SEX_MALE)
            act("$n accuses you of having the smallest.... (brain?)", ch, 0, victim, TO_VICT, 0);
          else
            act("$n tells you that you'd loose a beauty contest against a troll.", ch, 0, victim, TO_VICT, 0);
          break;
        case 2:
          act("$n calls your mother a bitch!", ch, 0, victim, TO_VICT, 0);
          break;
        default:
          act("$n tells you that you have big ears!", ch, 0, victim, TO_VICT, 0);
          break;
        } // end switch

        act("$n insults $N.", ch, 0, victim, TO_ROOM, NOTVICT);
      }
      else
      { /* ch == victim */
        ch->sendln("You feel insulted.");
      }
    }
  }
  else
    send_to_char("Insult who?\n\r", ch);
  return eSUCCESS;
}

int do_emote(Character *ch, char *argument, int cmd)
{
  int i;
  char buf[MAX_STRING_LENGTH];

  if (!IS_MOB(ch) && DC::isSet(ch->player->punish, PUNISH_NOEMOTE))
  {
    send_to_char("You can't show your emotions!!\n\r", ch);
    return eSUCCESS;
  }

  for (i = 0; *(argument + i) == ' '; i++)
    ;

  if (!*(argument + i))
    send_to_char("Yes.. But what?\n\r", ch);
  else
  {
    sprintf(buf, "$n %s", argument + i);
    // don't want players triggering mobs with emotes
    MOBtrigger = false;
    act(buf, ch, 0, 0, TO_ROOM, 0);
    csendf(ch, "%s %s\n\r", GET_SHORT(ch), argument + i);
    MOBtrigger = true;
  }
  return eSUCCESS;
}

void DC::load_hints(void)
{
  QFile file(HINTS_FILE_NAME);

  if (!file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
  {
    qCritical() << "Unable to open" << HINTS_FILE_NAME;
    return;
  }

  QString line, buffer;
  QTextStream in(&file);
  while (!in.atEnd())
  {
    // Read the #1 and discard it. It's only in the file for legacy reasons
    in.readLine();

    buffer = {};
    while (!in.atEnd())
    {
      line = in.readLine();
      buffer += line;

      // If line ends with ~ then we're done and erase the ~
      if (line.endsWith('~'))
      {
        buffer.erase(buffer.cend() - 1, buffer.cend());
        hints_.push_back(buffer);
        break;
      }
      else
      {
        buffer += '\n';
      }
    }
  }

  /*
    while (fgetc(fl) != '$')
    {
      fread_uint(fl, 0, 32768); // ignored

      char *buffer = fread_string(fl, 0);
      if (buffer != nullptr)
      {
        hints_.push_back(buffer);
        free(buffer);
      }
    }

    fclose(fl);
    */
}

void DC::save_hints(void)
{
  QFile file(HINTS_FILE_NAME);
  if (!file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text))
  {
    qCritical() << "Unable to open" << HINTS_FILE_NAME;
    return;
  }

  QTextStream out(&file);

  uint64_t hint_key = 0;
  for (hints_t::iterator i = hints_.begin(); i != hints_.end(); ++i)
  {
    out << "#" << ++hint_key << "\n";
    out << (*i).remove('\r') << "~"
        << "\n";
  }
  out << "$\n";
}

void DC::send_hint(void)
{
  if (hints_.isEmpty())
  {
    return;
  }

  auto num = number(0LL, hints_.size() - 1);

  uint64_t attempts = 0;
  while (hints_.value(num).isEmpty() && attempts++ < 100)
  {
    num = number(0LL, hints_.size() - 1);
  }

  QString hint = QString("$B$5HINT:$7 %1$R\r\n").arg(hints_.value(num));

  for (Connection *i = DC::getInstance()->descriptor_list; i; i = i->next)
  {
    if (i->connected || !i->character || !i->character->desc || is_busy(i->character) || IS_NPC(i->character))
    {
      continue;
    }

    if (DC::isSet(i->character->misc, LogChannels::CHANNEL_HINTS))
    {
      i->character->send(hint);
    }
  }

  return;
}
