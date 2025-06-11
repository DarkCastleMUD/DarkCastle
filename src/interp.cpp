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

#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cassert>

#include <string>
#include <tuple>

#include <fmt/format.h>
#include <QStringList>

#include "DC/structs.h" // MAX_STRING_LENGTH
#include "DC/character.h"
#include "DC/interp.h"
#include "DC/utility.h"
#include "DC/player.h"
#include "DC/fight.h"
#include "DC/spells.h" // ETHERAL consts
#include "DC/mobile.h"
#include "DC/connect.h" // Connection
#include "DC/room.h"
#include "DC/db.h"
#include "DC/act.h"
#include "DC/returnvals.h"
#include "DC/terminal.h"
#include "DC/CommandStack.h"
#include "DC/const.h"
#include "DC/DC.h"

#define SKILL_HIDE 337

int clan_guard(Character *ch, class Object *obj, int cmd, const char *arg, Character *owner);
int check_ethereal_focus(Character *ch, int trigger_type); // class/cl_mage.cpp
const char *fillwords[] =
    {
        "in",
        "from",
        "with",
        "the",
        "on",
        "at",
        "to",
        "\n"};

int do_motd(Character *ch, char *arg, int cmd)
{
  extern char motd[];

  page_string(ch->desc, motd, 1);
  return eSUCCESS;
}

int do_imotd(Character *ch, char *arg, int cmd)
{
  extern char imotd[];

  page_string(ch->desc, imotd, 1);
  return eSUCCESS;
}

command_return_t Character::command_interpreter(QString pcomm, bool procced)
{
  CommandStack cstack;

  if (cstack.isOverflow() == true)
  {
    // Prevent errors from showing up multiple times per loop
    if (cstack.getOverflowCount() < 2)
    {
      logentry(QStringLiteral("Command stack exceeded. depth: %1, max_depth: %2, name: %3, cmd: %4").arg(cstack.getDepth()).arg(cstack.getMax()).arg(getName()).arg(pcomm), IMMORTAL, DC::LogChannel::LOG_BUG);
    }
    return eFAILURE;
  }
  int retval{};
  QString buf;

  // Handle logged players.
  if (isPlayer() && isSet(player->punish, PUNISH_LOG))
  {
    logentry(QStringLiteral("Log %1: %2").arg(getName()).arg(pcomm), 110, DC::LogChannel::LOG_PLAYER);
  }

  // Implement freeze command.
  if (isPlayer() && isSet(player->punish, PUNISH_FREEZE) && pcomm != "quit")
  {
    sendln("You've been frozen by an immortal.");
    return eSUCCESS;
  }
  if (IS_AFFECTED(this, AFF_PRIMAL_FURY) && pcomm != "quit")
  {
    sendln("Your primal fury prevents this.");
    return eSUCCESS;
  }

  // Berserk checks
  if (isSet(combat, COMBAT_BERSERK))
  {
    if (fighting)
    {
      sendln("You've gone BERSERK! Beat them down, Beat them!! Rrrrrraaaaaagghh!!");
      return eSUCCESS;
    }
    REMOVE_BIT(combat, COMBAT_BERSERK);
    act("$n settles down.", this, 0, 0, TO_ROOM, 0);
    act("You settle down.", this, 0, 0, TO_CHAR, 0);
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
    return eSUCCESS;
  }

  auto pcomm_list = pcomm.split(' ');
  auto command = pcomm_list.value(0).toLower();
  pcomm_list.pop_front();
  auto command_arguments = pcomm_list.join(' ');

  if (command.isEmpty())
  {
    return eFAILURE;
  }

  // if we got this far, we're going to play with the command, so put
  // it into the debugging globals
  DC::getInstance()->last_processed_cmd = pcomm;
  DC::getInstance()->last_char_name = getName();
  DC::getInstance()->last_char_room = in_room;

  if (!pcomm.isEmpty())
  {
    retval = oprog_command_trigger(command, command_arguments);
    if (SOMEONE_DIED(retval) || isSet(retval, eEXTRA_VALUE))
      return retval;
  }

  auto found = DC::getInstance()->CMD_.find(command);
  if (found.has_value())
  {
    if (getLevel() >= found->getMinimumLevel() && (found->getFunction1() != nullptr || found->getFunction1b() != nullptr || found->getFunction2() != nullptr || found->getFunction3() != nullptr))
    {
      if (found->getMinimumLevel() == GIFTED_COMMAND)
      {

        // search DC::bestowable_god_commands for the command skill number to lookup with has_skill
        int command_skill = 0;
        for (int i = 0; DC::bestowable_god_commands[i].num != -1; i++)
        {
          if (DC::bestowable_god_commands[i].name == found->getName())
          {
            command_skill = DC::bestowable_god_commands[i].num;
            break;
          }
        }

        if (command_skill == 0 || IS_NPC(this) || !this->has_skill(command_skill))
        {
          this->sendln("Huh?");

          if (command_skill == 0)
          {
            logbug(QStringLiteral("command_interpreter: Unable to find command [%1].").arg(found->getName()));
          }
          return eFAILURE;
        }
      }

      // Paralysis stops everything but ...
      if (IS_AFFECTED(this, AFF_PARALYSIS) &&
          found->getNumber() != CMD_GTELL && // gtell
          found->getNumber() != CMD_CTELL    // ctell
      )
      {
        this->sendln("You've been paralyzed and are unable to move.");
        return eSUCCESS;
      }
      // Character not in position for command?
      if (this->getPosition() == position_t::FIGHTING && !this->fighting)
        this->setStanding();
      ;
      // fix for thin air thing
      if (this->getPosition() < found->getMinimumPosition())
      {
        switch (this->getPosition())
        {
        case position_t::DEAD:
          this->sendln("Lie still; you are DEAD.");
          break;
        case position_t::STUNNED:
          this->sendln("You are too stunned to do that.");
          break;
        case position_t::SLEEPING:
          this->sendln("In your dreams, or what?");
          break;
        case position_t::RESTING:
          this->sendln("Nah... You feel too relaxed...");
          break;
        case position_t::SITTING:
          this->sendln("Maybe you should stand up first?");
          break;
        case position_t::FIGHTING:
          this->sendln("No way!  You are still fighting!");
          break;
        }
        return eSUCCESS;
      }

      // charmies can only use charmie "ok" commands
      if (!procced) // Charmed mobs can still use their procs.
      {
        if ((IS_AFFECTED(this, AFF_FAMILIAR) || IS_AFFECTED(this, AFF_CHARM)) && !found->isCharmieAllowed())
        {
          return do_say(this, "I'm sorry master, I cannot do that.", CMD_DEFAULT);
        }
      }

      if (IS_NPC(this) && this->desc && this->desc->original && this->desc->original->getLevel() <= DC::MAX_MORTAL_LEVEL && !found->isCharmieAllowed())
      {
        this->sendln("The spirit cannot perform that action.");
        return eFAILURE;
      }
      /*
      if (IS_AFFECTED(this, AFF_HIDE)) {
        if (found->toggle_hide == 0) {
          REMBIT(this->affected_by, AFF_HIDE);
          sprintf(buf, "You emerge from your hidden position...\r\n");
          act(buf, this, 0, 0, TO_CHAR, 0);
          }
        if ((found->toggle_hide > 1) && (number(1, this->has_skill( SKILL_HIDE)) < found->toggle_hide)) {
          REMBIT(this->affected_by, AFF_HIDE);
          sprintf(buf, "Your movements disrupt your attempt to remain hidden...\r\n");
          act(buf, this, 0, 0, TO_CHAR, 0);
          }
        }
      */
      /*
      // Last resort for debugging...if you know it's a mortal.
      // -Sadus
      char DEBUGbuf[MAX_STRING_LENGTH];
      sprintf(DEBUGbuf, "%s: %s", GET_NAME(this), pcomm);
      log (DEBUGbuf, 0, DC::LogChannel::LOG_MISC);
      */
      if (!can_use_command(found->getNumber()))
      {
        this->sendln("You are still recovering from your last attempt.");
        return eSUCCESS;
      }

      if (IS_PC(this))
      {
        DC *dc = dynamic_cast<DC *>(DC::instance());
        // Don't log communication
        if (found->getNumber() != CMD_GTELL && found->getNumber() != CMD_CTELL && found->getNumber() != CMD_SAY && found->getNumber() != CMD_TELL && found->getNumber() != CMD_WHISPER && found->getNumber() != CMD_REPLY && (this->getLevel() >= 100 || (this->player->multi == true && dc->cf.allow_multi == false)) && isSet(this->player->punish, PUNISH_LOG) == false)
        {
          logentry(QStringLiteral("Log %1: %2").arg(GET_NAME(this)).arg(pcomm), 110, DC::LogChannel::LOG_PLAYER, this);
        }
      }

      // We're going to execute, check for usable special proc.
      retval = special(command_arguments, found->getNumber());
      if (isSet(retval, eSUCCESS) || isSet(retval, eCH_DIED))
        return retval;

      switch (found->getType())
      {
      case CommandType::all:
        if (this == nullptr)
        {
          return eFAILURE;
        }
        break;

      case CommandType::players_only:
        if (this == nullptr || this->player == nullptr)
        {
          return eFAILURE;
        }
        break;

      case CommandType::non_players_only:
        if (this == nullptr || this->mobdata == nullptr)
        {
          return eFAILURE;
        }
        break;

      case CommandType::immortals_only:
        if (!isImmortalPlayer())
        {
          return eFAILURE;
        }

        break;

      case CommandType::implementors_only:
        if (this == nullptr || this->player == nullptr || this->getLevel() < IMP_ONLY)
        {
          return eFAILURE;
        }
        break;

      default:
        if (this == nullptr)
        {
          return eFAILURE;
        }
        break;
      }

      // Normal dispatch
      if (found->getFunction1())
      {
        auto c = strdup(command_arguments.toStdString().c_str());
        retval = (*(found->getFunction1()))(this, c, found->getNumber());
        free(c);
      }
      else if (found->getFunction1b())
      {
        auto c = strdup(command_arguments.toStdString().c_str());
        retval = (*(found->getFunction1b()))(this, c, found->getNumber());
        free(c);
      }
      else if (found->getFunction2())
      {
        retval = (*(found->getFunction2()))(this, command_arguments.toStdString(), found->getNumber());
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
        this->send(fmt::format("{}{}", BOLD, NTEXT));
      }
      // This call is here to prevent gcc from tail-chaining the
      // previous call, which screws up the debugger call stack.
      // -- Furey
      // number(0, 0);

      return retval;
    }
  } // end if((found = find_cmd_in_radix(pcomm)))

  // If we're at this point, Paralyze stops everything so get out.
  if (IS_AFFECTED(this, AFF_PARALYSIS))
  {
    this->sendln("You've been paralyzed and are unable to move.");
    return eSUCCESS;
  }
  // Check social table
  if ((retval = this->check_social(pcomm)))
  {
    if (SOCIAL_true_WITH_NOISE == retval)
      return check_ethereal_focus(this, ETHEREAL_FOCUS_TRIGGER_SOCIAL);
    else
      return eSUCCESS;
  }

  // Unknown command (or char too low level)
  this->sendln("Huh?");
  return eSUCCESS;
}

int old_search_block(const char *arg, const char **list, bool exact)
{
  if (arg == nullptr)
  {
    return -1;
  }

  int i = 0;
  int l = strlen(arg);

  if (exact)
  {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcasecmp(arg, *(list + i)))
        return (i);
  }
  else
  {
    if (!l)
      // Avoid "" to match the first available std::string
      l = 1;
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncasecmp(arg, *(list + i), l))
        return (i);
  }

  return (-1);
}

int old_search_block(const char *arg, const QStringList list, bool exact)
{
  if (arg == nullptr)
  {
    return -1;
  }

  int i = 0;
  int l = strlen(arg);

  if (exact)
  {
    for (i = 0; i < list.length(); i++)
      if (!strcasecmp(arg, list.value(i).toStdString().c_str()))
        return (i);
  }
  else
  {
    if (!l)
      // Avoid "" to match the first available std::string
      l = 1;
    for (i = 0; list.length(); i++)
      if (!strncasecmp(arg, list.value(i).toStdString().c_str(), l))
        return (i);
  }

  return (-1);
}

int search_block(const char *orig_arg, const char **list, bool exact)
{
  int i;

  std::string needle = std::string(orig_arg);

  // Make into lower case and get length of std::string
  transform(needle.begin(), needle.end(), needle.begin(), ::tolower);

  if (exact)
  {
    for (i = 0; **(list + i) != '\n'; i++)
    {
      std::string haystack = *(list + i);
      if (needle == haystack)
      {
        return (i);
      }
    }
  }
  else
  {
    for (i = 0; **(list + i) != '\n'; i++)
    {
      std::string haystack = *(list + i);
      if (haystack.size() == 0 && needle.size() == 0)
      {
        return i;
      }

      if (needle.size() > 0 && haystack.compare(0, needle.size(), needle) == 0)
      {
        return i;
      }
    }
  }

  return (-1);
}

int search_blocknolow(char *arg, const char **list, bool exact)
{
  int i;
  unsigned int l = strlen(arg);

  if (exact)
  {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
        return (i);
  }
  else
  {
    if (!l)
      // Avoid "" to match the first available std::string
      l = 1;
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
        return (i);
  }

  return (-1);
}

int do_boss(Character *ch, char *arg, int cmd)
{
  char buf[200];
  int x;

  for (x = 0; x <= 60; x++)
  {
    sprintf(buf, "NUMBER-CRUNCHER: %d crunched to %d converted to black"
                 "/white tree node %d\n\r",
            x, 50 - x, x + x);
    ch->send(buf);
  }

  return eSUCCESS;
}

int old_search_block(const char *argument, int begin, int length, const QStringList list, int mode)
{
  int guess, found, search;

  // If the word contains 0 letters, a match is already found
  found = (length < 1);
  guess = 0;

  // Search for a match
  if (mode)
    while (!found && *(list.value(guess).toStdString().c_str()) != '\n')
    {
      found = ((unsigned)length == strlen(list.value(guess).toStdString().c_str()));
      for (search = 0; search < length && found; search++)
      {
        found = (*(argument + begin + search) == *(list.value(guess).toStdString().c_str() + search));
      }
      guess++;
    }
  else
  {
    while (!found && *(list.value(guess).toStdString().c_str()) != '\n')
    {
      found = 1;
      for (search = 0; (search < length && found); search++)
      {
        found = (*(argument + begin + search) == *(list.value(guess).toStdString().c_str() + search));
      }
      guess++;
    }
  }

  return (found ? guess : -1);
}

int old_search_block(const char *argument, int begin, int length, const char **list, int mode)
{
  int guess, found, search;

  // If the word contains 0 letters, a match is already found
  found = (length < 1);
  guess = 0;

  // Search for a match
  if (mode)
    while (!found && *(list[guess]) != '\n')
    {
      found = ((unsigned)length == strlen(list[guess]));
      for (search = 0; search < length && found; search++)
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
      for (search = 0; (search < length && found); search++)
      {
        found = (*(argument + begin + search) == *(list[guess] + search));
      }
      guess++;
    }
  }

  return (found ? guess : -1);
}

void argument_interpreter(const char *argument, char *first_arg, char *second_arg)
{
  int look_at, begin;

  begin = 0;

  do
  {
    /* Find first non blank */
    for (; *(argument + begin) == ' '; begin++)
      ;
    /* Find length of first word */
    for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, and copy them to first_arg */
      *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(first_arg + look_at) = '\0';
    begin += look_at;
  } while (fill_word(first_arg));

  do
  {
    /* Find first non blank */
    for (; *(argument + begin) == ' '; begin++)
      ;
    /* Find length of first word */
    for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, and copy them to second_arg */
      *(second_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(second_arg + look_at) = '\0';
    begin += look_at;
  } while (fill_word(second_arg));
}

// If the std::string is ALL numbers, return true
// If there is a non-numeric in std::string, return false
bool is_number(const char *str)
{
  int look_at;

  if (*str == '\0')
    return false;

  for (look_at = 0; *(str + look_at) != '\0'; look_at++)
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

// Multiline arguments, used for mobprogs
char *one_argument_long(char *argument, char *first_arg)
{
  int begin, look_at;
  bool end = false;
  begin = 0;

  /* Find first non blank */
  for (; isspace(*(argument + begin)); begin++)
    ;
  if (*(argument + begin) == '{')
  {
    end = true;
    begin++;
  }

  if (*(argument + begin) == '{')
  {
    end = true;
    begin++;
  }
  /* Find length of first word */
  for (look_at = 0;; look_at++)
    if (!end && *(argument + begin + look_at) <= ' ')
      break;
    else if (end && (*(argument + begin + look_at) == '}' || *(argument + begin + look_at) == '\0'))
    {
      begin++;
      break;
    }
    else
    {
      if (!end)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
      else
        *(first_arg + look_at) = *(argument + begin + look_at);
    }

  /* Make all letters lower case, and copy them to first_arg */
  *(first_arg + look_at) = '\0';
  begin += look_at;

  return argument + begin;
}

const char *one_argument_long(const char *argument, char *first_arg)
{
  int begin, look_at;
  bool end = false;
  begin = 0;

  /* Find first non blank */
  for (; isspace(*(argument + begin)); begin++)
    ;
  if (*(argument + begin) == '{')
  {
    end = true;
    begin++;
  }

  if (*(argument + begin) == '{')
  {
    end = true;
    begin++;
  }
  /* Find length of first word */
  for (look_at = 0;; look_at++)
    if (!end && *(argument + begin + look_at) <= ' ')
      break;
    else if (end && (*(argument + begin + look_at) == '}' || *(argument + begin + look_at) == '\0'))
    {
      begin++;
      break;
    }
    else
    {
      if (!end)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
      else
        *(first_arg + look_at) = *(argument + begin + look_at);
    }

  /* Make all letters lower case, and copy them to first_arg */
  *(first_arg + look_at) = '\0';
  begin += look_at;

  return argument + begin;
}

/* find the first sub-argument of a std::string, return pointer to first char in
   primary argument, following the sub-arg                      */
char *one_argument(char *argument, char *first_arg)
{
  return one_argument_long(argument, first_arg);
  int begin, look_at;

  begin = 0;

  do
  {
    /* Find first non blank */
    for (; isspace(*(argument + begin)); begin++)
      ;
    /* Find length of first word */
    for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, and copy them to first_arg */
      *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(first_arg + look_at) = '\0';
    begin += look_at;
  } while (fill_word(first_arg));

  return (argument + begin);
}

const char *one_argument(const char *argument, char *first_arg)
{
  return one_argument_long(argument, first_arg);
}

int fill_wordnolow(char *argument)
{
  return (search_blocknolow(argument, fillwords, true) >= 0);
}

char *one_argumentnolow(char *argument, char *first_arg)
{
  int begin, look_at;
  begin = 0;

  do
  {
    /* Find first non blank */
    for (; isspace(*(argument + begin)); begin++)
      ;
    /* Find length of first word */
    for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* copy to first_arg */
      *(first_arg + look_at) = *(argument + begin + look_at);
    *(first_arg + look_at) = '\0';
    begin += look_at;
  } while (fill_wordnolow(first_arg));

  return (argument + begin);
}

int fill_word(char *argument)
{
  return (search_block(argument, fillwords, true) >= 0);
}

void automail(char *name)
{
  FILE *blah;
  char buf[100];

  blah = fopen("../lib/whassup.txt", "w");
  fprintf(blah, name);
  fclose(blah);
  sprintf(buf, "mail void@dcastle.org < ../lib/whassup.txt");
  system(buf);
}

bool is_abbrev(const std::string &aabrev, const std::string &word)
{
  if (aabrev.empty())
  {
    return false;
  }

  return equal(aabrev.begin(), aabrev.end(), word.begin(),
               [](char a, char w)
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

/* determine if a given std::string is an abbreviation of another */
bool is_abbrev(const char *arg1, const char *arg2) /* arg1 is short, arg2 is long */
{
  if (!*arg1)
    return false;

  for (; *arg1; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return false;

  return true;
}

std::string ltrim(std::string str)
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

std::string rtrim(std::string str)
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
std::tuple<QString, QString> half_chop(QString arguments, const char token)
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

std::tuple<std::string, std::string> half_chop(std::string arguments, const char token)
{
  arguments = ltrim(arguments);

  auto space_after_arg1 = arguments.find_first_of(token);
  auto arg1 = arguments.substr(0, space_after_arg1);

  // remove arg1 from arguments
  arguments.erase(0, space_after_arg1);

  arguments = ltrim(arguments);

  return std::tuple<std::string, std::string>(arg1, arguments);
}

std::tuple<std::string, std::string> half_chop(const char *c_arg, const char token)
{
  std::string arguments;
  if (c_arg)
  {
    arguments = c_arg;
  }

  return half_chop(arguments, token);
}

std::tuple<std::string, std::string> last_argument(std::string arguments)
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

    return std::tuple<std::string, std::string>(last_arg, arguments);
  }
  catch (...)
  {
    logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Error in last_argument(%s)",
         arguments.c_str());
  }

  return std::tuple<std::string, std::string>(std::string(), std::string());
}

/* return first 'word' plus trailing substring of input std::string */
void half_chop(const char *str, char *arg1, char *arg2)
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
void chop_half(char *str, char *arg1, char *arg2)
{
  int32_t i, j;

  // skip over trailing space
  i = strlen(str) - 1;
  j = 0;
  while (isspace(str[i]))
    i--;

  // find beginning of last 'word'
  while (!isspace(str[i]))
  {
    i--;
    j++;
  }

  // copy last word to arg1
  strncpy(arg1, str + i + 1, j);
  arg1[j] = '\0';

  // skip over leading space in str
  while (isspace(*str))
  {
    str++;
    i--;
  }

  // copy str to arg2
  strncpy(arg2, str, i);

  // remove trailing space from arg2
  while (isspace(arg2[i]))
    i--;

  arg2[i] = '\0';
}

command_return_t Character::special(QString arguments, int cmd)
{
  class Object *i;
  Character *k;
  int j;
  int retval;

  /* special in room? */
  if (DC::getInstance()->world[in_room].funct)
  {
    if ((retval = (*DC::getInstance()->world[in_room].funct)(this, cmd, arguments.toStdString().c_str())) != eFAILURE)
      return retval;
  }

  /* special in equipment list? */
  for (j = 0; j <= (MAX_WEAR - 1); j++)
    if (equipment[j] && this->equipment[j]->vnum >= 0)
      if (DC::getInstance()->obj_index[this->equipment[j]->vnum].non_combat_func)
      {
        retval = ((*DC::getInstance()->obj_index[this->equipment[j]->vnum].non_combat_func)(this, this->equipment[j], cmd, arguments.toStdString().c_str(), this));
        if (isSet(retval, eCH_DIED) || isSet(retval, eSUCCESS))
          return retval;
      }

  /* special in inventory? */
  for (i = carrying; i; i = i->next_content)
    if (i->vnum >= 0)
      if (DC::getInstance()->obj_index[i->vnum].non_combat_func)
      {
        retval = ((*DC::getInstance()->obj_index[i->vnum].non_combat_func)(this, i, cmd, arguments.toStdString().c_str(), this));
        if (isSet(retval, eCH_DIED) || isSet(retval, eSUCCESS))
          return retval;
      }

  /* special in mobile present? */
  for (k = DC::getInstance()->world[this->in_room].people; k; k = k->next_in_room)
  {
    if (IS_NPC(k))
    {
      if (((Character *)DC::getInstance()->mob_index[k->mobdata->nr].item)->mobdata->mob_flags.type == MOB_CLAN_GUARD)
      {
        retval = clan_guard(this, 0, cmd, arguments.toStdString().c_str(), k);
        if (isSet(retval, eCH_DIED) || isSet(retval, eSUCCESS))
          return retval;
      }
      else if (DC::getInstance()->mob_index[k->mobdata->nr].non_combat_func)
      {
        retval = ((*DC::getInstance()->mob_index[k->mobdata->nr].non_combat_func)(this, 0,
                                                                                  cmd, arguments.toStdString().c_str(), k));
        if (isSet(retval, eCH_DIED) || isSet(retval, eSUCCESS))
          return retval;
      }
    }
  }

  /* special in object present? */
  for (i = DC::getInstance()->world[this->in_room].contents; i; i = i->next_content)
    if (i->vnum >= 0)
      if (DC::getInstance()->obj_index[i->vnum].non_combat_func)
      {
        retval = ((*DC::getInstance()->obj_index[i->vnum].non_combat_func)(this, i, cmd, arguments.toStdString().c_str(), this));
        if (isSet(retval, eCH_DIED) || isSet(retval, eSUCCESS))
          return retval;
      }

  return eFAILURE;
}

void Character::add_command_lag(int cmdnum, int lag)
{
  command_lag *cmdl;
#ifdef LEAK_CHECK
  cmdl = (command_lag *)
      calloc(1, sizeof(command_lag));
#else
  cmdl = (command_lag *)
      dc_alloc(1, sizeof(command_lag));
#endif
  cmdl->next = DC::getInstance()->getCommandLag();
  DC::getInstance()->setCommandLag(cmdl);
  cmdl->ch = this;
  cmdl->cmd_number = cmdnum;
  cmdl->lag = lag;
}

bool Character::can_use_command(int cmdnum)
{
  command_lag *cmdl;
  for (cmdl = DC::getInstance()->getCommandLag(); cmdl; cmdl = cmdl->next)
  {

    if (cmdl->ch == this && cmdl->cmd_number == cmdnum)
      return false;
  }
  return true;
}

void pulse_command_lag()
{
  command_lag *cmdl{}, *cmdlp = nullptr, *cmdlnext = nullptr;

  for (cmdl = DC::getInstance()->getCommandLag(); cmdl; cmdl = cmdlnext)
  {
    cmdlnext = cmdl->next;
    if ((cmdl->lag--) <= 0)
    {
      if (cmdlp)
        cmdlp->next = cmdl->next;
      else
        DC::getInstance()->setCommandLag(cmdl->next);

      cmdl->ch = 0;
      dc_free(cmdl);
    }
    else
      cmdlp = cmdl;
  }
}

char *remove_trailing_spaces(char *arg)
{
  int len = strlen(arg) - 1;

  if (len < 1)
    return arg;

  for (; len > 0; len--)
  {
    if (arg[len] != ' ')
    {
      arg[len + 1] = '\0';
      return arg;
    }
  }
  return arg;
}

// The End
