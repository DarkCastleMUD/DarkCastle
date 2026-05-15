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
#include "DC/DC.h"

extern bool MOBtrigger;

ReturnValues do_report(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  QString report;

  assert(ch != 0);
  if (ch->in_room == INVALID_ROOM)
  {
    ch->dc_->logentry(u"NOWHERE sent to do_report!"_s, OVERSEER, DC::LogChannel::LOG_BUG);
    return ReturnValue::eSUCCESS;
  }

  if (isSet(ch->dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eSUCCESS;
  }

  if (ch->isPlayer() && argument != nullptr)
  {
    QString arg1, remainder_args;
    std::tie(arg1, remainder_args) = half_chop(QString(argument));
    if (arg1 == "help")
    {
      ch->sendln(u"report       - Reports hps, mana, moves and ki. (default)"_s);
      ch->sendln(u"report xp    - Reports current xp, xp till next level and levels to be gained."_s);
      ch->sendln(u"report help  - Shows different ways report can be used."_s);
      return ReturnValue::eFAILURE;
    }

    if (arg1 == "xp")
    {
      // calculate how many levels a player could gain with their current XP
      quint8 levels_to_gain = {};
      qint64 players_exp = ch->exp;
      while (levels_to_gain < IMMORTAL)
      {
        if (players_exp >= (qint64)exp_table[(qint32)ch->getLevel() + levels_to_gain + 1])
        {
          players_exp -= (qint64)exp_table[(qint32)ch->getLevel() + levels_to_gain + 1];
          levels_to_gain++;
        }
        else
        {
          break;
        }
      }

      dc_snprintf(report, 200, "XP: %lld, XP till level: %lld, Levels to gain: %u",
                  ch->exp,
                  (qint64)(exp_table[(qint32)ch->getLevel() + 1] - (qint64)ch->exp),
                  levels_to_gain);

      dc_sprintf(buf, "$n reports '%s'", qPrintable(report));
      act_to_room(buf, ch, 0, 0, 0);

      ch->send(u"You report: %1\r\n"_s.arg(report));
      return ReturnValue::eSUCCESS;
    }
  }

  if (ch->isNonPlayer() || IS_ANONYMOUS(ch))
    dc_snprintf(report, 200, "%d%% hps, %d%% mana, %llu%% movement, and %d%% ki.",
                MAX(1, ch->getHP() * 100) / MAX(1, GET_MAX_HIT(ch)),
                MAX(1, GET_MANA(ch) * 100) / MAX(1, GET_MAX_MANA(ch)),
                MAX<move_t>(1, GET_MOVE(ch) * 100) / MAX<move_t>(1, GET_MAX_MOVE(ch)),
                MAX(1, GET_KI(ch) * 100) / MAX(1, GET_MAX_KI(ch)));
  else
  {
    dc_snprintf(report, 200, "%d/%d hps, %d/%d mana, %llu/%llu movement, and %d/%d ki.",
                ch->getHP(), GET_MAX_HIT(ch),
                GET_MANA(ch), GET_MAX_MANA(ch),
                GET_MOVE(ch), GET_MAX_MOVE(ch),
                GET_KI(ch), GET_MAX_KI(ch));
  }

  dc_sprintf(buf, "$n reports '%s'", qPrintable(report));
  act_to_room(buf, ch, 0, 0, 0);

  ch->send(u"You report: %1\r\n"_s.arg(report));

  return ReturnValue::eSUCCESS;
}

/************************************************************************
| send_to_gods
| Preconditions: str != 0
| Postconditions: None
| Side effects: None
| Returns: 0 on failure, non-zero on success
| Notes:
*/
qint32 DC::send_to_gods(QString message, quint64 god_level, DC::LogChannel type)
{
  QString buf1;
  QString buf;
  QString typestr;

  if (message.isEmpty())
    return {};

  if ((god_level > IMPLEMENTER) || (god_level < 0))
  { // Outside valid god levels
    return {};
  }

  switch (type)
  {
  case DC::LogChannel::LOG_BUG:
    typestr = "bug";
    break;
  case DC::LogChannel::LOG_PRAYER:
    typestr = "pray";
    break;
  case DC::LogChannel::LOG_GOD:
    typestr = "god";
    break;
  case DC::LogChannel::LOG_MORTAL:
    typestr = "mortal";
    break;
  case DC::LogChannel::LOG_SOCKET:
    typestr = "socket";
    break;
  case DC::LogChannel::LOG_MISC:
    typestr = "misc";
    break;
  case DC::LogChannel::LOG_PLAYER:
    typestr = "player";
    break;
  case DC::LogChannel::LOG_WORLD:
    typestr = "world";
    break;
  case DC::LogChannel::LOG_ARENA:
    typestr = "arena";
    break;
  case DC::LogChannel::LOG_CLAN:
    typestr = "logclan";
    break;
  case DC::LogChannel::LOG_WARNING:
    typestr = "warnings";
    break;
  case DC::LogChannel::LOG_DATABASE:
    typestr = "database";
    break;
  case DC::LogChannel::LOG_VAULT:
    typestr = "vault";
    break;
  case DC::LogChannel::LOG_HELP:
    typestr = "help";
    break;
  case DC::LogChannel::LOG_OBJECTS:
    typestr = "objects";
    break;
  case DC::LogChannel::LOG_QUEST:
    typestr = "quest";
    break;
  case DC::LogChannel::LOG_DEBUG:
    typestr = "debug";
    break;
  default:
    typestr = "unknown";
    break;
  }

  buf = u"//(%1) %2\r\n"_s.arg(typestr).arg(message);
  buf1 = u"%1%2//%3(%4)%5 %6%7 %8%9%10\r\n"_s.arg(BOLD).arg(RED).arg(NTEXT).arg(typestr).arg(BOLD).arg(YELLOW).arg(message).arg(RED).arg(NTEXT).arg(GREY);

  for (auto &conn : connections_)
  {
    if ((conn->character == nullptr) || (conn->character->getLevel() <= MORTAL))
      continue;
    if (!(isSet(conn->character->misc, type)))
      continue;
    if (is_busy(conn->character))
      continue;
    if (!conn->connected && conn->character->getLevel() >= god_level)
    {
      if (conn->character->isNonPlayer() || isSet(conn->character->player->toggles, Player::PLR_ANSI))
        send_to_char(buf1, conn->character);
      else
        conn->character->send(buf);
    }
  }
  return (1);
}

ReturnValues do_channel(CharacterPtr ch, QString arg, cmd_t cmd)
{
  qint32 x;
  qint32 y = {};
  QString buf;
  QString buf2;

  const QStringList on_off = {
      "$B$4off$R",
      "$B$2on$R"};

  const QStringList types = {
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

  if (ch->isNonPlayer())
    return ReturnValue::eSUCCESS;

  if (!arg.isEmpty())
    arg = one_argument(arg, buf);
  else
  {
    //    ch->sendln(u""_s);

    if (ch->isMortalPlayer())
    {
      for (x = 7; x <= 14; x++)
      {
        if (isSet(ch->misc, (1 << x)))
          y = 1;
        else
          y = {};
        dc_sprintf(buf2, "%-9s%s\r\n", qPrintable(types[x]), qPrintable(on_off[y]));
        send_to_char(buf2, ch);
      }
    }
    else
    {
      qint32 o = ch->getLevel() == 110 ? 21 : 19;
      for (x = {}; x <= o; x++)
      {
        if (isSet(ch->misc, (1 << x)))
          y = 1;
        else
          y = {};
        dc_sprintf(buf2, "%-9s%s\r\n", qPrintable(types[x]), qPrintable(on_off[y]));
        send_to_char(buf2, ch);
      }
    }

    if (isSet(ch->misc, 1 << 22))
      y = 1;
    else
      y = {};
    dc_sprintf(buf2, "%-9s%s\r\n", qPrintable(types[22]), qPrintable(on_off[y]));
    send_to_char(buf2, ch);

    if (isSet(ch->misc, 1 << 23))
      y = 1;
    else
      y = {};
    dc_sprintf(buf2, "%-9s%s\r\n", qPrintable(types[23]), qPrintable(on_off[y]));
    send_to_char(buf2, ch);

    qint32 o = ch->getLevel() == 110 ? 26 : 0;
    for (x = 24; x <= o; x++)
    {
      if (isSet(ch->misc, (1 << x)))
        y = 1;
      else
        y = {};
      dc_sprintf(buf2, "%-9s%s\r\n", qPrintable(types[x]), qPrintable(on_off[y]));
      send_to_char(buf2, ch);
    }

    return ReturnValue::eSUCCESS;
  }

  for (x = {}; x <= 27; x++)
  {
    if (x == 27)
    {
      ch->sendln(u"That type was not found."_s);
      return ReturnValue::eSUCCESS;
    }
    if (is_abbrev(buf, types[x]))
      break;
  }

  if (ch->isMortalPlayer() &&
      (x < 7 || (x > 14 && x < 22)))
  {
    ch->sendln(u"That type was not found."_s);
    return ReturnValue::eSUCCESS;
  }
  if (x > 19 && ch->getLevel() != 110 && x < 22)
  {
    ch->sendln(u"That type was not found."_s);
    return ReturnValue::eSUCCESS;
  }
  if (isSet(ch->misc, (1 << x)))
  {
    dc_sprintf(buf, "%s channel turned $B$4OFF$R.\r\n", qPrintable(types[x]));
    ch->send(buf);
    REMOVE_BIT(ch->misc, (1 << x));
  }
  else
  {
    dc_sprintf(buf, "%s channel turned $B$2ON$R.\r\n", qPrintable(types[x]));
    ch->send(buf);
    SET_BIT(ch->misc, (1 << x));
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_ignore(CharacterPtr ch, QString args, cmd_t cmd)
{
  if (ch == nullptr)
  {
    return ReturnValue::eFAILURE;
  }

  if (ch->isNonPlayer())
  {
    ch->send(u"You're a mob! You can't ignore people.\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  if (args.isEmpty())
  {
    if (ch->player->ignoring.isEmpty())
    {
      ch->send(u"Ignore who?\r\n"_s);
      return ReturnValue::eSUCCESS;
    }

    // convert ignoring QMap into "char1 char2 char3" format
    QString ignoreString = {};
    for (const auto &[keyword, entry] : ch->player->ignoring.asKeyValueRange())
    {
      if (entry.ignore)
      {
        if (ignoreString.isEmpty())
        {
          ignoreString = keyword;
        }
        else
        {
          ignoreString = ignoreString + " " + keyword;
        }
      }
    }
    ch->sendln(u"Ignoring: %1"_s.arg(ignoreString));

    return ReturnValue::eSUCCESS;
  }

  QString arg1 = {}, remainder_args = {};
  std::tie(arg1, remainder_args) = half_chop(args);
  if (arg1.isEmpty())
  {
    ch->send(u"Ignore who?\r\n"_s);
    return ReturnValue::eFAILURE;
  }
  arg1[0] = arg1[0].toUpper();

  if (ch->player->ignoring.contains(arg1))
  {
    ch->player->ignoring.remove(arg1);
    ch->sendln(u"You stop ignoring %1."_s.arg(arg1));
  }
  else
  {
    ch->player->ignoring[arg1] = {true, 0};
    ch->sendln(u"You now ignore anyone named %1."_s.arg(arg1));
  }
  return ReturnValue::eSUCCESS;
}

bool is_ignoring(CharacterPtr ch, CharacterPtr victim)
{
  if (ch->isNonPlayer() || (victim->getLevel() >= IMMORTAL && victim->isPlayer()) || ch->player->ignoring.isEmpty())
  {
    return false;
  }

  if (victim->name().isEmpty())
  {
    return false;
  }

  if (ch->player->ignoring.contains(qPrintable(victim->name())))
  {
    return true;
  }

  // Since it didn't match the whole name, see if it matches one of
  // the name keywords used for a mob name
  QString names = victim->name();
  QString name1 = {}, remainder_names = {};
  std::tie(name1, remainder_names) = half_chop(names);
  while (name1.isEmpty() == false)
  {
    if (ch->player->ignoring.contains(name1))
    {
      return true;
    }

    std::tie(name1, remainder_names) = half_chop(remainder_names);
  }

  return false;
}

constexpr auto MAX_NOTE_LENGTH = 1000; /* arbitrary */

ReturnValues do_write(CharacterPtr ch, QString argument, cmd_t cmd)
{
  ObjectPtr paper = 0, pen = {};
  QString papername, penname, buf;

  argument_interpreter(argument, papername, penname);

  if (!ch->conn_)
    return ReturnValue::eSUCCESS;
  if (ch->getLevel() < 5)
  {
    ch->sendln(u"You need to be at least level 5 to write on the board."_s);
    return ReturnValue::eSUCCESS;
  }

  if (papername.isEmpty()) /* nothing was delivered */
  {
    ch->sendln(u"Write? with what? ON what? what are you trying to do??"_s);
    return ReturnValue::eSUCCESS;
  }

  if (!penname.isEmpty()) /* there were two arguments */
  {
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
    {
      dc_sprintf(buf, "You have no %s.\r\n", qPrintable(papername));
      ch->send(buf);
      return ReturnValue::eSUCCESS;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying)))
    {
      dc_sprintf(buf, "You have no %s.\r\n", qPrintable(papername));
      ch->send(buf);
      return ReturnValue::eSUCCESS;
    }
  }
  else /* there was one arg.let's see what we can find */
  {
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
    {
      dc_sprintf(buf, "There is no %s in your inventory.\r\n", qPrintable(papername));
      ch->send(buf);
      return ReturnValue::eSUCCESS;
    }
    if (paper->flags_.type_flag == ITEM_PEN) /* oops, a pen.. */
    {
      pen = paper;
      paper = {};
    }
    else if (paper->flags_.type_flag != ITEM_NOTE)
    {
      ch->sendln(u"That thing has nothing to do with writing."_s);
      return ReturnValue::eSUCCESS;
    }

    /* one object was found. Now for the other one. */
    if (!ch->equipment[WEAR_HOLD])
    {
      dc_sprintf(buf, "You can't write with a %s alone.\r\n", qPrintable(papername));
      ch->send(buf);
      return ReturnValue::eSUCCESS;
    }
    if (!CAN_SEE_OBJ(ch, ch->equipment[WEAR_HOLD]))
    {
      ch->sendln(u"The stuff in your hand is invisible! Yeech!!"_s);
      return ReturnValue::eSUCCESS;
    }

    if (pen)
      paper = ch->equipment[WEAR_HOLD];
    else
      pen = ch->equipment[WEAR_HOLD];
  }

  /* ok.. now let's see what kind of stuff we've found */
  if (pen->flags_.type_flag != ITEM_PEN)
  {
    act_to_character("$p is no good for writing with.", ch, pen, 0, 0);
  }
  else if (paper->flags_.type_flag != ITEM_NOTE)
  {
    act_to_character("You can't write on $p.", ch, paper, 0, 0);
  }
  else if (!paper->ActionDescription().isEmpty())
    /*    else if (paper->item_number != real_object(1205) )  */
    ch->sendln(u"There's something written on it already."_s);
  else
  {
    /* we can write - hooray! */

    ch->sendln(u"Ok.. go ahead and write.. end the note with a \\@."_s);
    act_to_room("$n begins to jot down a note.", ch, 0, 0, INVIS_NULL);
    // TODO BROKEN
    // ch->conn_->strnew = &paper->ActionDescription();
  }
  return ReturnValue::eSUCCESS;
}

// TODO - Add a bunch of insults to this for the hell of it.
ReturnValues do_insult(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  QString arg;
  CharacterPtr victim;

  one_argument(argument, arg);

  if (!arg.isEmpty())
  {
    if (!(victim = ch->get_char_room_vis(arg)))
    {
      ch->sendln(u"Can't hear you!"_s);
    }
    else
    {
      if (victim != ch)
      {
        dc_sprintf(buf, "You insult %s.\r\n", qPrintable(victim->shortdesc_or_name()));
        ch->send(buf);

        switch (ch->dc_->number(0, 3))
        {
        case 0:
          if (GET_SEX(victim) == Character::sex_t::MALE)
            act_to_victim("$n accuses you of fighting like a woman!", ch, 0, victim, 0);
          else
            act_to_victim("$n says that women can't fight.", ch, 0, victim, 0);
          break;
        case 1:
          if (GET_SEX(victim) == Character::sex_t::MALE)
            act_to_victim("$n accuses you of having the smallest.... (brain?)", ch, 0, victim, 0);
          else
            act_to_victim("$n tells you that you'd loose a beauty contest against a troll.", ch, 0, victim, 0);
          break;
        case 2:
          act_to_victim("$n calls your mother a bitch!", ch, 0, victim, 0);
          break;
        default:
          act_to_victim("$n tells you that you have big ears!", ch, 0, victim, 0);
          break;
        } // end switch

        act_to_room("$n insults $N.", ch, 0, victim, NOTVICT);
      }
      else
      { /* ch == victim */
        ch->sendln(u"You feel insulted."_s);
      }
    }
  }
  else
    ch->sendln(u"Insult who?"_s);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_emote(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 i;
  QString buf;

  if (!ch->isNonPlayer() && isSet(ch->player->punish, PUNISH_NOEMOTE))
  {
    ch->sendln(u"You can't show your emotions!!"_s);
    return ReturnValue::eSUCCESS;
  }

  auto arguments = argument.trimmed().split(' ');
  auto arg1 = arguments.value(0);
  auto arg2 = arguments.value(1);

  if (arguments.isEmpty())
    ch->sendln(u"Yes.. But what?"_s);
  else
  {
    dc_sprintf(buf, "$n %s", qPrintable(arg1));
    // don't want players triggering mobs with emotes
    MOBtrigger = false;
    act_to_room(buf, ch, 0, 0, 0);
    ch->send(u"%s %s\r\n"_s.arg(ch->shortdesc_or_name()).arg(arg1));
    MOBtrigger = true;
  }
  return ReturnValue::eSUCCESS;
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
    while (fgetc(stream) != '$')
    {
      fread_uint(stream, 0, 32768); // ignored

      QString buffer = fread_string(stream);
      if (buffer != nullptr)
      {
        hints_.push_back(buffer);

      }
    }


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

  quint64 hint_key = {};
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

  quint64 attempts = {};
  while (hints_.value(num).isEmpty() && attempts++ < 100)
  {
    num = number(0LL, hints_.size() - 1);
  }

  QString hint = u"$B$5HINT:$7 %1$R\r\n"_s.arg(hints_.value(num));

  for (auto &conn : connections_)
  {
    if (!conn->isPlaying() || !conn->character || !conn->character->conn_ || is_busy(conn->character) || conn->character->isNonPlayer())
    {
      continue;
    }

    if (isSet(conn->character->misc, DC::LogChannel::CHANNEL_HINTS))
    {
      conn->character->send(hint);
    }
  }
}
