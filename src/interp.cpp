/***************************************************************************
 *  file: interp.c , Command interpreter module.      Part of DIKUMUD      *
 *  Usage: Procedures interpreting user command                            *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec\@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* Revision History                                                        */
/* 12/08/2003   Onager   Added chop_half() to work like half_chop() but    */
/*                       chopping off the last word.                       */
/***************************************************************************/
/* $Id: interp.cpp,v 1.200 2015/06/14 02:38:12 pirahna Exp $ */

#include "DC/DC.h"

qint32 clan_guard(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner);
qint32 check_ethereal_focus(CharacterPtr ch, qint32 trigger_type); // class/cl_mage.cpp
const QStringList fillwords =
    {
        "in",
        "from",
        "with",
        "the",
        "on",
        "at",
        "to"};

command_return_t do_motd(CharacterPtr ch, QString arg, cmd_t cmd)
{
  extern QString motd;

  page_string(ch->conn_, motd, 1);
  return ReturnValue::eSUCCESS;
}

command_return_t do_imotd(CharacterPtr ch, QString arg, cmd_t cmd)
{
  extern QString imotd;

  page_string(ch->conn_, imotd, 1);
  return ReturnValue::eSUCCESS;
}

class LogCommand
{
  bool logged_ = {};
  CharacterPtr ch_ = {};
  QString command_ = {};
  command_return_t rc_ = {};
  QString rc_reason_ = {};
  Timer command_duration_ = {};

public:
  explicit LogCommand(QString command, CharacterPtr ch) : command_(command), ch_(ch)
  {
    if (ch && ch->isPlayer() &&
        (isSet(ch->player->punish, PUNISH_LOG) ||
         ch->getLevel() >= 100 ||
         (ch->player->multi && !ch->dc_->cf.allow_multi)))
    {
      command_duration_.start();
      ch->dc_->logentry(u"ch=%1 in=%2 cmd=\"%3\""_s.arg(ch->name()).arg(QString::number(ch->in_room)).arg(command_), 110, DC::LogChannel::LOG_PLAYER, ch);
      logged_ = true;
    }
  }

  ~LogCommand()
  {
    if (logged_)
    {
      command_duration_.stop();
      auto timediff = ((command_duration_.getDiff().tv_sec * 1000000.0) + command_duration_.getDiff().tv_usec) / 1000000.0;
      auto timediffStr = QString::number(timediff, 'f');
      ch->dc_->logentry(u"ch=%1 in=%2 cmd=\"%3\" rc=%4 reason=\"%5\" duration=%6"_s.arg(ch->name()).arg(QString::number(ch->in_room)).arg(command_).arg(QString::number(rc_)).arg(rc_reason_).arg(timediffStr), IMPLEMENTER, DC::LogChannel::LOG_PLAYER, ch);
    }
  }

  command_return_t setReturn(command_return_t rc, QString rc_reason)
  {
    rc_ = rc;
    rc_reason_ = rc_reason;
    return rc_;
  }
};

command_return_t Character::command_interpreter(QString pcomm, bool procced)
{
  CommandStack cstack;
  LogCommand logcmd{pcomm, this};

  if (cstack.isOverflow() == true)
  {
    // Prevent errors from showing up multiple times per loop
    if (cstack.getOverflowCount() < 2)
    {
      dc_->logentry(u"Command stack exceeded. depth: %1, max_depth: %2, name: %3, cmd: %4"_s.arg(cstack.getDepth()).arg(cstack.getMax()).arg(name()).arg(pcomm), IMMORTAL, DC::LogChannel::LOG_BUG);
    }
    return logcmd.setReturn(ReturnValue::eFAILURE, "cstack exceeded");
  }

  qint32 retval = {};
  QString buf;

  // Implement freeze command.
  if (isPlayer() && isSet(player->punish, PUNISH_FREEZE) && pcomm != "quit")
  {
    sendln("You've been frozen by an immortal.");
    return logcmd.setReturn(ReturnValue::eFAILURE, u"frozen"_s);
  }

  if (IS_AFFECTED(this, AFF_PRIMAL_FURY) && pcomm != "quit")
  {
    sendln("Your primal fury prevents this.");
    return logcmd.setReturn(ReturnValue::eFAILURE, "primal fury");
  }

  // Berserk checks
  if (isSet(combat, COMBAT_BERSERK))
  {
    if (fighting)
    {
      sendln("You've gone BERSERK! Beat them down, Beat them!! Rrrrrraaaaaagghh!!");
      return logcmd.setReturn(ReturnValue::eFAILURE, "berserk");
    }
    REMOVE_BIT(combat, COMBAT_BERSERK);
    act_to_room("$n settles down.", this, 0, 0, 0);
    act_to_character("You settle down.", this, 0, 0, 0);
    GET_AC(this) -= 30;
  }

  // Strip initial spaces OR tab characters and parse command word.
  // Translate to lower case.  We need to translate tabs for the MOBProgs to work
  if (desc == nullptr || !desc->isEditing())
  {
    pcomm = pcomm.trimmed();
  }

  // MOBprogram commands weeded out
  if (desc && pcomm.startsWith("mp", Qt::CaseInsensitive))
  {
    sendln("Huh?");
    return logcmd.setReturn(ReturnValue::eFAILURE, u"mp command with descriptor"_s);
  }

  auto pcomm_list = pcomm.split(' ');
  auto command = pcomm_list.value(0).toLower();
  pcomm_list.pop_front();
  auto command_arguments = pcomm_list.join(' ');

  if (command.isEmpty())
  {
    return logcmd.setReturn(ReturnValue::eFAILURE, u"empty"_s);
  }

  // if we got this far, we're going to play with the command, so put
  // it into the debugging globals
  dc_->last_processed_cmd = pcomm;
  dc_->last_char_name = name();
  dc_->last_char_room = in_room;

  if (!pcomm.isEmpty())
  {
    retval = oprog_command_trigger(command, command_arguments);
    if (SOMEONE_DIED(retval))
      return logcmd.setReturn(retval, u"someone died"_s);
    if (isSet(retval, ReturnValue::eEXTRA_VALUE))
      return logcmd.setReturn(retval, u"ReturnValue::eEXTRA_VALUE"_s);
  }

  auto found = dc_->CMD_.find(command);
  if (found.has_value())
  {
    if (getLevel() >= found->getMinimumLevel() && (found->getFunction1() != nullptr || found->getFunction1b() != nullptr || found->getFunction2() != nullptr || found->getFunction3() != nullptr))
    {
      if (found->getMinimumLevel() == GIFTED_COMMAND)
      {

        // search DC::bestowable_god_commands for the command skill number to lookup with has_skill
        qint32 command_skill = {};
        for (qint32 i = {}; DC::bestowable_god_commands[i].num != -1; i++)
        {
          if (DC::bestowable_god_commands[i].name == found->name())
          {
            command_skill = DC::bestowable_god_commands[i].num;
            break;
          }
        }

        if (command_skill == 0)
        {
          sendln("Huh?");
          auto str = u"command_interpreter: Unable to find command [%1]."_s.arg(found->name());
          logbug(str);
          return logcmd.setReturn(ReturnValue::eFAILURE, str);
        }
        else if (isNonPlayer())
        {
          sendln("Huh?");
          return logcmd.setReturn(ReturnValue::eFAILURE, "NPC attempting bestowed cmd");
        }
        else if (!has_skill(command_skill))
        {
          sendln("Huh?");
          return logcmd.setReturn(ReturnValue::eFAILURE, "does not have bestowed cmd");
        }
      }

      // Paralysis stops everything but ...
      if (IS_AFFECTED(this, AFF_PARALYSIS) &&
          found->getNumber() != cmd_t::GTELL && // gtell
          found->getNumber() != cmd_t::CTELL    // ctell
      )
      {
        sendln("You've been paralyzed and are unable to move.");
        return logcmd.setReturn(ReturnValue::eFAILURE, "paralyzed");
      }
      // Character not in position for command?
      if (GET_POS(this) == position_t::FIGHTING && !fighting)
      {
        setStanding();
      }
      // fix for thin air thing
      if (GET_POS(this) < found->getMinimumPosition())
      {
        auto minimum_position_str = QString::number(static_cast<quint8>(found->getMinimumPosition()));
        switch (GET_POS(this))
        {
        case position_t::DEAD:
          sendln("Lie still; you are DEAD.");
          return logcmd.setReturn(ReturnValue::eFAILURE, u"dead < %1"_s.arg(minimum_position_str));
          break;
        case position_t::STUNNED:
          sendln("You are too stunned to do that.");
          return logcmd.setReturn(ReturnValue::eFAILURE, u"stunned < %1"_s.arg(minimum_position_str));
          break;
        case position_t::SLEEPING:
          sendln("In your dreams, or what?");
          return logcmd.setReturn(ReturnValue::eFAILURE, u"sleeping < %1"_s.arg(minimum_position_str));
          break;
        case position_t::RESTING:
          sendln("Nah... You feel too relaxed...");
          return logcmd.setReturn(ReturnValue::eFAILURE, u"resting < %1"_s.arg(minimum_position_str));
          break;
        case position_t::SITTING:
          sendln("Maybe you should stand up first?");
          return logcmd.setReturn(ReturnValue::eFAILURE, u"sitting < %1"_s.arg(minimum_position_str));
          break;
        case position_t::FIGHTING:
          sendln("No way!  You are still fighting!");
          return logcmd.setReturn(ReturnValue::eFAILURE, u"fighting < %1"_s.arg(minimum_position_str));
          break;
        case position_t::STANDING:
          break;
        }
        return logcmd.setReturn(ReturnValue::eFAILURE, u"wrong position for cmd < %1"_s.arg(minimum_position_str));
      }

      // charmies can only use charmie "ok" commands
      if (!procced) // Charmed mobs can still use their procs.
      {
        if ((IS_AFFECTED(this, AFF_FAMILIAR) || IS_AFFECTED(this, AFF_CHARM)) && !found->isCharmieAllowed())
        {
          retval = do_say(this, "I'm sorry master, I cannot do that.");
          return logcmd.setReturn(retval, u"familiar/charmie not allowed"_s);
        }
      }

      if (isNonPlayer() && conn_ && conn_->original && conn_->original->getLevel() <= DC::MAX_MORTAL_LEVEL && !found->isCharmieAllowed())
      {
        sendln("The spirit cannot perform that action.");
        return logcmd.setReturn(ReturnValue::eFAILURE, u"spirit not allowed"_s);
      }
      /*
      if (IS_AFFECTED(this, AFF_HIDE)) {
        if (found->toggle_hide == 0) {
          REMBIT(affected_by, AFF_HIDE);
          dc_sprintf(buf, "You emerge from your hidden position...\r\n");
          act_to_character(buf, this, 0, 0,  0);
          }
        if ((found->toggle_hide > 1) && (ch->dc_->number(1, has_skill( SKILL_HIDE)) < found->toggle_hide)) {
          REMBIT(affected_by, AFF_HIDE);
          dc_sprintf(buf, "Your movements disrupt your attempt to remain hidden...\r\n");
          act_to_character(buf, this, 0, 0,  0);
          }
        }
      */

      if (!can_use_command(found->getNumber()))
      {
        sendln("You are still recovering from your last attempt.");
        return logcmd.setReturn(ReturnValue::eFAILURE, u"still recovering"_s);
      }

      // We're going to execute, check for usable special proc.
      retval = special(command_arguments, found->getNumber());

      QString retval_description;
      if (isSet(retval, ReturnValue::eSUCCESS))
      {
        retval_description = u"ReturnValue::eSUCCESS"_s;
      }

      if (isSet(retval, ReturnValue::eCH_DIED))
      {
        if (!retval_description.isEmpty())
        {
          retval_description += u" "_s;
        }
        retval_description += u"ReturnValue::eSUCCESS"_s;
      }

      if (!retval_description.isEmpty())
      {
        return logcmd.setReturn(retval, retval_description);
      }

      switch (found->getType())
      {
      case CommandType::players_only:
        if (player == nullptr)
        {
          return logcmd.setReturn(ReturnValue::eFAILURE, u"player only"_s);
        }
        break;

      case CommandType::non_players_only:
        if (mobdata == nullptr)
        {
          return logcmd.setReturn(ReturnValue::eFAILURE, u"NPC only"_s);
        }
        break;

      case CommandType::immortals_only:
        if (!isImmortalPlayer())
        {
          return logcmd.setReturn(ReturnValue::eFAILURE, u"immortals only"_s);
        }

        break;

      case CommandType::implementors_only:
        if (player == nullptr || getLevel() < IMPLEMENTER)
        {
          return logcmd.setReturn(ReturnValue::eFAILURE, u"implementor only"_s);
        }
        break;

      default:
        break;
      }

      // Normal dispatch
      if (found->getFunction1())
      {
        auto c = strdup(qPrintable(command_arguments));
        retval = (*(found->getFunction1()))(this, c, found->getNumber());
      }
      else if (found->getFunction1b())
      {
        auto c = strdup(qPrintable(command_arguments));
        retval = (*(found->getFunction1b()))(this, c, found->getNumber());
      }
      else if (found->getFunction2())
      {
        retval = (*(found->getFunction2()))(this, command_arguments, found->getNumber());
      }
      else if (found->getFunction3())
      {
        QString command = command_arguments.trimmed();
        QStringList arguments;

        // This code splits command by spaces and doublequotes
        while (command.count('"') > 1)
        {
          qsizetype section_start, first_quote;
          first_quote = command.indexOf('"');

          // Find section start is the location of the character after a space next to first double quote
          section_start = command.sliced(0, first_quote + 1).lastIndexOf(' ') + 1;
          if (section_start == -1)
          {
            section_start = first_quote;
          }
          QString before_quotes = command.sliced(0, section_start);

          auto second_quote = command.indexOf('"', first_quote + 1);
          QString quoted = command.sliced(section_start, second_quote - section_start + 1);
          arguments.append(before_quotes.split(' ', Qt::SkipEmptyParts));
          arguments.append(quoted);
          command = command.remove(0, second_quote + 1);
        }

        arguments.append(command.split(' ', Qt::SkipEmptyParts));

        retval = (*this.*(found->getFunction3()))(arguments, found->getNumber());
      }

      // Next bit for the DUI client, they needed it.
      if (!SOMEONE_DIED(retval) && !selfpurge)
      {
        send(u"$B$R"_s);
      }

      return logcmd.setReturn(retval, u""_s);
    }
  } // end if((found = find_cmd_in_radix(pcomm)))

  // If we're at this point, Paralyze stops everything so get out.
  if (IS_AFFECTED(this, AFF_PARALYSIS))
  {
    sendln("You've been paralyzed and are unable to move.");
    return logcmd.setReturn(ReturnValue::eFAILURE, u"paralyzed"_s);
  }
  // Check social table
  if ((retval = check_social(pcomm)))
  {
    if (SOCIAL_TRUE_WITH_NOISE == retval)
      return check_ethereal_focus(this, ETHEREAL_FOCUS_TRIGGER_SOCIAL);
    else
      return logcmd.setReturn(ReturnValue::eSUCCESS, u"ReturnValue::eSUCCESS"_s);
  }

  // Unknown command (or character too low level)
  sendln("Huh?");
  return logcmd.setReturn(ReturnValue::eSUCCESS, u"ReturnValue::eSUCCESS"_s);
}

// case-insensitive search of a QList of QStrings for an entry starting with search string
qsizetype search_list(QString arg, const QStringList list)
{
  if (arg.isEmpty())
    return -1;

  for (qsizetype index = 0; index < list.length(); ++index)
    if (list.at(index).startsWith(arg, Qt::CaseInsensitive))
      return index;

  return -1;
}

command_return_t do_boss(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString buf;
  qint32 x;

  for (x = {}; x <= 60; x++)
  {
    dc_sprintf(buf, "NUMBER-CRUNCHER: %d crunched to %d converted to black"
                    "/white tree node %d\r\n",
               x, 50 - x, x + x);
    ch->send(buf);
  }

  return ReturnValue::eSUCCESS;
}

qint32 old_search_block(const QString argument, qint32 begin, qint32 length, const QStringList list, qint32 mode)
{
  qint32 guess, found, search;

  // If the word contains 0 letters, a match is already found
  found = (length < 1);
  guess = {};

  // Search for a match
  if (mode)
    while (!found && *(list.value(qPrintable(guess))) != '\n')
    {
      found = ((quint32)length == dc_strlen(list.value(qPrintable(guess))));
      for (search = {}; search < length && found; search++)
      {
        found = (*(argument + begin + search) == *(list.value(qPrintable(guess)) + search));
      }
      guess++;
    }
  else
  {
    while (!found && *(list.value(qPrintable(guess))) != '\n')
    {
      found = 1;
      for (search = {}; (search < length && found); search++)
      {
        found = (*(argument + begin + search) == *(list.value(qPrintable(guess)) + search));
      }
      guess++;
    }
  }

  return (found ? guess : -1);
}

qint32 old_search_block(const QString argument, qint32 begin, qint32 length, const QString *list, qint32 mode)
{
  qint32 guess, found, search;

  // If the word contains 0 letters, a match is already found
  found = (length < 1);
  guess = {};

  // Search for a match
  if (mode)
    while (!found && *(list[guess]) != '\n')
    {
      found = ((quint32)length == dc_strlen(list[guess]));
      for (search = {}; search < length && found; search++)
      {
        found = (*(argument + begin + search) == *(list[guess] + search));
      }
      guess++;
    }
  else
  {
    while (!found && *(list[guess]) != '\n')
    {
      found = 1;
      for (search = {}; (search < length && found); search++)
      {
        found = (*(argument + begin + search) == *(list[guess] + search));
      }
      guess++;
    }
  }

  return (found ? guess : -1);
}

void argument_interpreter(const QString argument, QString first_arg, QString second_arg)
{
  qint32 look_at, begin;

  begin = {};

  do
  {
    /* Find first non blank */
    for (; *(argument + begin) == ' '; begin++)
      ;
    /* Find length of first word */
    for (look_at = {}; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, and copy them to first_arg */
      *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(first_arg + look_at) = '\0';
    begin += look_at;
  } while (fillwords.contains(QString(first_arg), Qt::CaseInsensitive);

  do
  {
    /* Find first non blank */
    for (; *(argument + begin) == ' '; begin++)
      ;
    /* Find length of first word */
    for (look_at = {}; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, and copy them to second_arg */
      *(second_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(second_arg + look_at) = '\0';
    begin += look_at;
  } while (fillwords.contains(QString(second_arg), Qt::CaseInsensitive);
}

// If the QString is ALL numbers, return true
// If there is a non-numeric in QString, return false
bool is_number(const QString str)
{
  qint32 look_at;

  if (*str == '\0')
    return false;

  for (look_at = {}; *(str + look_at) != '\0'; look_at++)
    if ((*(str + look_at) < '0') || (*(str + look_at) > '9'))
      return false;

  return false;
}

bool is_number(QString str)
{
  bool ok = false;
  str.toLongLong(&ok);
  if (ok)
  {
    return true;
  }

  str.toULongLong(&ok);
  if (ok)
  {
    return true;
  }

  return false;
}

QString one_argument(QString arguments, QString &arg1)
{
  auto arguments_list = arguments.trimmed().split(' ');

  if (!arguments_list.isEmpty())
  {
    arg1 = arguments_list.first().toLower();
    arguments_list.removeFirst();
    return arguments_list.join(' ');
  }
  arg1 = {};
  return {};
}

QString one_argumentnolow(QString argument, QString first_arg)
{
  qint32 begin, look_at;
  begin = {};

  do
  {
    /* Find first non blank */
    for (; isspace(*(argument + begin)); begin++)
      ;
    /* Find length of first word */
    for (look_at = {}; *(argument + begin + look_at) > ' '; look_at++)
      /* copy to first_arg */
      *(first_arg + look_at) = *(argument + begin + look_at);
    *(first_arg + look_at) = '\0';
    begin += look_at;
  } while (fillwords.contains(first_arg, Qt::CaseInsensitive);

  return (argument + begin);
}

void automail(QString name)
{
  FILE *blah;
  QString buf;

  blah = fopen("../lib/whassup.txt", "w");
  dc_fprintf(blah, "%s", name);

  dc_sprintf(buf, "mail void@dcastle.org < ../lib/whassup.txt");
  system(buf);
}

bool is_abbrev(const QString &aabrev, const QString &word)
{
  if (aabrev.isEmpty())
  {
    return false;
  }

  return equal(aabrev.begin(), aabrev.end(), word.begin(),
               [](QChar a, QChar w)
               {
                 return tolower(a) == tolower(w);
               });
}

bool is_abbrev(QString aabrev, QString word)
{
  if (aabrev.isEmpty())
  {
    return false;
  }

  return word.startsWith(aabrev, Qt::CaseInsensitive);
}

/* determine if a given QString is an abbreviation of another */
bool is_abbrev(const QString arg1, const QString arg2) /* arg1 is short, arg2 is long */
{
  if (arg.isEmpty() 1)
    return false;

  for (; *arg1; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return false;

  return true;
}

QString ltrim(QString str)
{
  // remove leading spaces
  try
  {
    auto first_non_space = str.find_first_not_of(' ');
    if (first_non_space != str.npos)
    {
      str.erase(0, first_non_space);
    }
    else
    {
      str.clear();
    }
  }
  catch (...)
  {
  }

  return str;
}

QString rtrim(QString str)
{
  // remove leading spaces
  try
  {
    auto last_non_space = str.find_last_not_of(' ');
    if (last_non_space != str.npos)
    {
      str.erase(last_non_space + 1, str.length() + 1);
    }
    else
    {
      str.clear();
    }
  }
  catch (...)
  {
  }

  return str;
}
std::tuple<QString, QString> half_chop(QString arguments, const QChar token)
{
  QStringList namelist = arguments.trimmed().split(token);

  if (namelist.isEmpty())
  {
    return std::tuple<QString, QString>(QString(), QString());
  }

  QString arg1 = namelist.at(0);
  namelist.pop_front();
  QString remainder = namelist.join(' ').trimmed();

  return std::tuple<QString, QString>(arg1, remainder);
}

std::tuple<QString, QString> half_chop(const QString c_arg, const QChar token)
{
  QString arguments;
  if (c_arg)
  {
    arguments = c_arg;
  }

  return half_chop(arguments, token);
}

std::tuple<QString, QString> last_argument(QString arguments)
{
  try
  {
    // remove leading spaces
    auto first_non_space = arguments.find_first_not_of(' ');
    arguments.erase(0, first_non_space);

    // remove trailing spaces
    auto last_non_space = arguments.find_last_not_of(' ');
    arguments.erase(last_non_space + 1, arguments.length() + 1);

    auto space_after_last_arg = arguments.find_last_of(' ');
    auto last_arg = arguments.substr(space_after_last_arg + 1, arguments.length() + 1);

    arguments.erase(space_after_last_arg, arguments.length() + 1);

    // remove trailing spaces
    last_non_space = arguments.find_last_not_of(' ');
    arguments.erase(last_non_space + 1, arguments.length() + 1);

    return std::tuple<QString, QString>(last_arg, arguments);
  }
  catch (...)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Error in last_argument(%s)",
              arguments.c_str());
  }

  return std::tuple<QString, QString>(QString(), QString());
}

/* return first 'word' plus trailing substring of input QString */
void half_chop(const QString str, QString arg1, QString arg2)
{
  // strip leading whitespace from original
  for (; isspace(*str); str++)
    ;

  // copy everything up to next space
  for (; !isspace(*arg1 = *str) && *str; str++, arg1++)
    ;

  // terminate
  *arg1 = '\0';

  // strip leading whitepace
  for (; isspace(*str); str++)
    ;

  // copy rest of str to arg2
  for (; (*arg2 = *str) != '\0'; str++, arg2++)
    ;
}

/* return last 'word' plus leading substring of input str */
void chop_half(QString str, QString arg1, QString arg2)
{
  qint32 i, j;

  // skip over trailing space
  i = dc_strlen(str) - 1;
  j = {};
  while (isspace(str[i]))
    i--;

  // find beginning of last 'word'
  while (!isspace(str[i]))
  {
    i--;
    j++;
  }

  // copy last word to arg1
  dc_strncpy(arg1, str + i + 1, j);
  arg1[j] = '\0';

  // skip over leading space in str
  while (isspace(*str))
  {
    str++;
    i--;
  }

  // copy str to arg2
  dc_strncpy(arg2, str, i);

  // remove trailing space from arg2
  while (isspace(arg2[i]))
    i--;

  arg2[i] = '\0';
}

command_return_t Character::special(QString arguments, cmd_t cmd)
{
  ObjectPtr i;
  CharacterPtr k;
  qint32 j;
  qint32 retval;

  /* special in room? */
  if (dc_->world[in_room].funct)
  {
    if ((retval = (*dc_->world[in_room].funct)(this, cmd, qPrintable(arguments))) != ReturnValue::eFAILURE)
      return retval;
  }

  /* special in equipment list? */
  for (j = {}; j <= (MAX_WEAR - 1); j++)
    if (equipment[j] && equipment[j]->item_number >= 0)
      if (dc_->obj_index[equipment[j]->item_number].non_combat_func)
      {
        retval = ((*dc_->obj_index[equipment[j]->item_number].non_combat_func)(this, equipment[j], cmd, qPrintable(arguments), this));
        if (isSet(retval, ReturnValue::eCH_DIED) || isSet(retval, ReturnValue::eSUCCESS))
          return retval;
      }

  /* special in inventory? */
  for (i = carrying; i; i = i->next_content)
    if (i->item_number >= 0)
      if (dc_->obj_index[i->item_number].non_combat_func)
      {
        retval = ((*dc_->obj_index[i->item_number].non_combat_func)(this, i, cmd, qPrintable(arguments), this));
        if (isSet(retval, ReturnValue::eCH_DIED) || isSet(retval, ReturnValue::eSUCCESS))
          return retval;
      }

  /* special in mobile present? */
  for (k = dc_->world[in_room].people_; k; k = k->next_in_room)
  {
    if (k->isNonPlayer())
    {
      if (((CharacterPtr)dc_->mob_index[k->mobdata->nr].item)->mobdata->mob_flags.type == MOB_CLAN_GUARD)
      {
        retval = clan_guard(this, 0, cmd, qPrintable(arguments), k);
        if (isSet(retval, ReturnValue::eCH_DIED) || isSet(retval, ReturnValue::eSUCCESS))
          return retval;
      }
      else if (dc_->mob_index[k->mobdata->nr].non_combat_func)
      {
        retval = ((*dc_->mob_index[k->mobdata->nr].non_combat_func)(this, 0,
                                                                    cmd, qPrintable(arguments), k));
        if (isSet(retval, ReturnValue::eCH_DIED) || isSet(retval, ReturnValue::eSUCCESS))
          return retval;
      }
    }
  }

  /* special in object present? */
  for (i = dc_->world[in_room].contents; i; i = i->next_content)
    if (i->item_number >= 0)
      if (dc_->obj_index[i->item_number].non_combat_func)
      {
        retval = ((*dc_->obj_index[i->item_number].non_combat_func)(this, i, cmd, qPrintable(arguments), this));
        if (isSet(retval, ReturnValue::eCH_DIED) || isSet(retval, ReturnValue::eSUCCESS))
          return retval;
      }

  return ReturnValue::eFAILURE;
}

void Character::add_command_lag(cmd_t cmd, qint32 lag)
{
  command_lag *cmdl;
  cmdl = new command_lag;
  cmdl->next = dc_->getCommandLag();
  dc_->setCommandLag(cmdl);
  cmdl->ch = this;
  cmdl->cmd_number = cmd;
  cmdl->lag = lag;
}

bool Character::can_use_command(cmd_t cmd)
{
  command_lag *cmdl;
  for (cmdl = dc_->getCommandLag(); cmdl; cmdl = cmdl->next)
  {

    if (cmdl->ch == this && cmdl->cmd_number == cmd)
      return false;
  }
  return true;
}

void DC::pulse_command_lag(void)
{
  command_lag *cmdl{}, *cmdlp = {}, *cmdlnext = {};

  for (cmdl = dc_->getCommandLag(); cmdl; cmdl = cmdlnext)
  {
    cmdlnext = cmdl->next;
    if ((cmdl->lag--) <= 0)
    {
      if (cmdlp)
        cmdlp->next = cmdl->next;
      else
        dc_->setCommandLag(cmdl->next);

      cmdl->ch = {};
      cmdl = {};
    }
    else
      cmdlp = cmdl;
  }
}

QString remove_trailing_spaces(QString arg)
{
  return arg.trimmed();
}