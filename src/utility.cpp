
/***************************************************************************
 *  file: utility.c, Utility module.                       Part of DIKUMUD *
 *  Usage: Utility procedures                                              *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 *                                                                         *
 * Revision History                                                        *
 * 10/18/2003   Onager     Changed CAN_SEE() to hide Onager from everyone  *
 *                         except Pir and Valk                             *
 * 10/19/2003   Onager     Took out super-secret hidey code from CAN_SEE() *
 ***************************************************************************/
/* $Id: utility.cpp,v 1.129 2014/07/04 22:00:04 jhhudso Exp $ */

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>

#include <fmt/format.h>
#include <QRandomGenerator>
#include <QString>

#include "innate.h"
#include "structs.h"
#include "levels.h"
#include "player.h"
#include "timeinfo.h"
#include "character.h"
#include "utility.h"
#include "room.h"
#include "obj.h"
#include "interp.h"
#include "fileinfo.h"
#include "mobile.h"
#include "handler.h"
#include "db.h"
#include "connect.h"
#include "act.h"
#include "spells.h"
#include "clan.h"
#include "fight.h"
#include "returnvals.h"
#include "set.h"
#include "DC.h"
#include "const.h"

#ifndef GZIP
#define GZIP "gzip"
#endif

extern std::map<int, std::map<uint8_t, std::string>> professions;

// extern funcs
clan_data *get_clan(Character *);
void release_message(Character *ch);
struct timer_data *timer_list = nullptr;

// local funcs
void update_wizlist(Character *ch);

// tested in TestUtility::test_nocolor_strlen_qstring
std::size_t nocolor_strlen(const QStringView str)
{
  size_t len{};
  bool decode_color{};

  for (const auto &s : str)
  {
    if (s == '$')
    {
      decode_color = true;
      continue;
    }

    if (Q_UNLIKELY(decode_color))
    {
      decode_color = false;

      // Recognized color code so does not add to length
      if ((s <= '9' && s >= '0') || s == 'I' || s == 'L' || s == '*' || s == 'R' || s == 'B')
      {
        continue;
      }
      // $$ translates to $ so it adds 1 to length
      else if (s == '$')
      {
        len++;
      }
      // Unrecognized color code adds 2 to length because it doesn't translate into anything
      else
      {
        len += 2;
      }
    }
    else
    {
      len++;
    }
  }

  // Last character of string was a $ so it adds 1 to length because it doesn't translate into anything
  if (decode_color)
  {
    len++;
  }

  return len;
}

// tested in TestUtility::test_nocolor_strlen_c
size_t nocolor_strlen(const char *s)
{
  if (!s)
  {
    return 0;
  }

  size_t len = 0;

  while (*s != '\0')
  {
    if (*s == '$')
    {
      s++;
      if ((*s <= '9' && *s >= '0') ||
          *s == 'I' ||
          *s == 'L' ||
          *s == '*' ||
          *s == 'R' ||
          *s == 'B')
      {
        // Do nothing
      }
      else if (*s == '$')
      {
        len++;
      }
      else if (*s == '\0')
      {
        len++;
        break;
      }
      else
      {
        len += 2;
      }
      s++;
    }
    else
    {
      len++;
      s++;
    }
  }

  return len;
}

// This function is like str_dup except it returns 0 if passed 0
// tested in TestUtility::test_str_dup0
char *str_dup0(const char *str)
{
  if (str == 0)
  {
    return 0;
  }

  return str_dup(str);
}

// duplicate a string with it's own memory
// tested in TestUtility::test_str_dup
char *str_dup(const char *str)
{
  char *str_new = 0;
  size_t strlength = strlen(str);

  str_new = (char *)dc_alloc(strlength + 1, sizeof(char));

  if (!str_new)
  {
    qFatal("NO MEMORY DUPLICATING STRING!");
  }
  return strncpy(str_new, str, strlength);
}

// simulates a dice roll
// basically we assign the total to the number of dice (since you always
// roll at least a one with each die) then add a random MOD of the die
// size.  ie, 4d10 would be 4 + loop*4 (0-9)
// tested in TestUtility::test_dice
int dice(int num, int size, QRandomGenerator *rng)
{
  int r;
  int sum = 0;

  if (size < 1)
    return 1;

  for (r = 1; r <= num; r++)
    sum += number(1, size, rng);

  return sum;
}

// compare strings but ignore case (unlike strcmp)
// tested in TestUtility::test_str_cmp
int str_cmp(const char *arg1, const char *arg2)
{
  int check, i;

  assert(arg1 && arg2);

  if (!arg1 || !arg2)
  {
    logentry(QStringLiteral("nullptr args sent to str_cmp in utility.c!"), ANGEL, LogChannels::LOG_BUG);
    return -1;
  }

  for (i = 0; arg1[i] || arg2[i]; i++)
  {
    check = LOWER(arg1[i]) - LOWER(arg2[i]);
    if (check < 0)
      return -1;
    if (check > 0)
      return 1;
  }

  return 0;
}

// Tested in TestUtility::test_str_nospace
char *str_nospace(const char *stri)
{
  if (!stri)
    return "";

  char *stri_new = str_dup(stri);
  int i = 0;

  while (*(stri + i))
  {
    if (*(stri + i) == ' ')
      stri_new[i] = '_';
    i++;
  }
  return stri_new; // Must be freed by caller to avoid memory leak
}

// compare strings but ignore case and change all spaces to underscores
// Tested in TestUtility::test_str_nosp_cmp_c_string
int str_nosp_cmp(const char *arg1, const char *arg2)
{
  char *tmp_arg1 = str_nospace(arg1);
  char *tmp_arg2 = str_nospace(arg2);
  int retval = str_cmp(tmp_arg1, tmp_arg2);
  dc_free(tmp_arg2);
  dc_free(tmp_arg1);

  return retval;
}

// Tested in TestUtility::test_str_nosp_cmp_qtring
int str_nosp_cmp(QString arg1, QString arg2)
{
  return str_nosp_cmp(arg1.toStdString().c_str(), arg2.toStdString().c_str());
}

// Tested in TestUtility::test_str_n_nosp_cmp_c_string
int str_n_nosp_cmp(const char *arg1, const char *arg2, int size)
{
  char *tmp_arg1 = str_nospace(arg1);
  char *tmp_arg2 = str_nospace(arg2);
  int retval = strncasecmp(tmp_arg1, tmp_arg2, size);
  dc_free(tmp_arg2);
  dc_free(tmp_arg1);

  return retval;
}

// TODO - Declare these in a more appropriate place
FILE *bug_log = 0;
FILE *god_log = 0;
FILE *mortal_log = 0;
FILE *socket_log = 0;
FILE *player_log = 0;
FILE *world_log = 0;
FILE *arena_log = 0;
FILE *clan_log = 0;
FILE *objects_log = 0;
FILE *quest_log = 0;

// writes a std::string to the log
void logentry(QString str, uint64_t god_level, LogChannels type, Character *vict)
{
  FILE **f = 0;
  int stream = 1;
  std::stringstream logpath;
  DC *dc = dynamic_cast<DC *>(DC::instance());
  DC::config &cf = dc->cf;

  if (DC::getInstance()->cf.bport)
  {
    logpath << "../blog/";
  }
  else
  {
    logpath << "../log/";
  }

  if (type == LogChannels::LOG_PLAYER && vict && !vict->getName().isEmpty())
  {
    logpath << PLAYER_DIR;
  }

  QDir logDirectory(logpath.str().c_str());
  if (!logDirectory.exists())
  {
    qWarning("Log directory '%s' does not exist.", qUtf8Printable(logDirectory.absolutePath()));
    if (logDirectory.mkdir(logDirectory.absolutePath()))
    {
      qInfo("Made log directory '%s'.", qUtf8Printable(logDirectory.absolutePath()));
    }
    else
    {
      qFatal("Unable to make directory '%s'.", qUtf8Printable(logDirectory.absolutePath()));
    }
  }

  switch (type)
  {
  default:
    stream = 0;
    break;
  case LogChannels::LOG_BUG:
    f = &bug_log;
    logpath << BUG_LOG;
    if (!(*f = fopen(logpath.str().c_str(), "a")))
    {
      qFatal("Unable to open bug log.\n");
    }

    // TODO - need some sort of thing to automatically have bugs switch from file to
    //        to non-file when we're up in gdb

    // stream = 0;
    //  I want bugs to be right in the gdblog.
    //  -Sadus

    break;
  case LogChannels::LOG_GOD:
    f = &god_log;
    logpath << GOD_LOG;
    if (!(*f = fopen(logpath.str().c_str(), "a")))
    {
      qFatal("Unable to open god log.\n");
    }
    break;
  case LogChannels::LOG_MORTAL:
    f = &mortal_log;
    logpath << MORTAL_LOG;
    if (!(*f = fopen(logpath.str().c_str(), "a")))
    {
      qFatal("Unable to open mortal log.\n");
    }
    break;
  case LogChannels::LOG_SOCKET:
    f = &socket_log;
    logpath << SOCKET_LOG;
    if (!(*f = fopen(logpath.str().c_str(), "a")))
    {
      qFatal(qUtf8Printable(QStringLiteral("Unable to open socket log: %1\n").arg(logpath.str().c_str())));
    }
    break;
  case LogChannels::LOG_PLAYER:
    f = &player_log;
    if (vict && !vict->getName().isEmpty())
    {
      logpath << vict->getName().toStdString();
      if (!(*f = fopen(logpath.str().c_str(), "a")))
      {
        qCritical(qUtf8Printable(QStringLiteral("Unable to open player log '%1'.\n").arg(logpath.str().c_str())));
      }
    }
    else
    {
      logpath << PLAYER_LOG;
      if (!(*f = fopen(logpath.str().c_str(), "a")))
      {
        qCritical("Unable to open player log.\n");
      }
    }
    break;
  case LogChannels::LOG_WORLD:
    f = &world_log;
    logpath << WORLD_LOG;
    if (!(*f = fopen(logpath.str().c_str(), "a")))
    {
      qFatal("Unable to open world log.\n");
    }
    break;
  case LogChannels::LOG_ARENA:
    f = &arena_log;
    logpath << ARENA_LOG;
    if (!(*f = fopen(logpath.str().c_str(), "a")))
    {
      qFatal("Unable to open arena log.\n");
    }
    break;
  case LogChannels::LOG_CLAN:
    f = &clan_log;
    logpath << CLAN_LOG;
    if (!(*f = fopen(logpath.str().c_str(), "a")))
    {
      qFatal("Unable to open clan log.\n");
    }
    break;
  case LogChannels::LOG_OBJECTS:
    f = &objects_log;
    logpath << OBJECTS_LOG;
    if (!(*f = fopen(logpath.str().c_str(), "a")))
    {
      qFatal("Unable to open objects log.\n");
    }
    break;
  case LogChannels::LOG_QUEST:
    f = &quest_log;
    logpath << QUEST_LOG;
    if (!(*f = fopen(logpath.str().c_str(), "a")))
    {
      qFatal("Unable to open quest log.\n");
    }
    break;
  }

  time_t t = time(0);
  const tm *lt = localtime(&t);
  char *tmstr = asctime(lt);
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (stream == STDIN_FILENO || type == LogChannels::LOG_BUG)
  {
    if (cf.stderr_timestamp == true)
    {
      std::cerr << QStringLiteral("%1 :%2: %3").arg(tmstr).arg(type).arg(str).toStdString() << std::endl;
    }
    else
    {
      std::cerr << QStringLiteral("%1:%2").arg(type).arg(str).toStdString() << std::endl;
    }
  }

  if (stream != STDIN_FILENO)
  {
    fprintf(*f, "%s :: %s\n", tmstr, str.toStdString().c_str());
    fclose(*f);
  }

  if (god_level >= IMMORTAL)
    send_to_gods(str, god_level, type);
}

void socketlog(QString message)
{
  logentry(message, IMMORTAL, LOG_SOCKET);
}

void buglog(QString message)
{
  logentry(message, IMMORTAL, LOG_BUG);
}

void misclog(QString message)
{
  logentry(message, IMMORTAL, LOG_MISC);
}

// function for new SETBIT et al. commands
// leading space until all calling functions cleaned up
void sprintbit(uint value[], const char *names[], char *result)
{
  int i;
  *result = '\0';

  for (i = 0; *names[i] != '\n'; i++)
  {
    int a = i / ASIZE;
    if (isSet(value[a], 1 << (i - a * 32)))
    {
      if (!strcmp(names[i], "UNUSED"))
        continue;
      strcat(result, names[i]);
      strcat(result, " ");
    }
  }

  if (*result == '\0')
    strcat(result, "NoBits ");
}

// no leading space
std::string sprintbit(uint value[], const char *names[])
{
  int i;
  std::string result;

  for (i = 0; *names[i] != '\n'; i++)
  {
    int a = i / ASIZE;
    if (isSet(value[a], 1 << (i - a * 32)))
    {
      if (!strcmp(names[i], "UNUSED"))
      {
        continue;
      }

      if (!result.empty())
      {
        result += " ";
      }
      result += names[i];
    }
  }

  if (result.empty())
  {
    result = "NoBits";
  }

  return result;
}

// leading space until all calling functions cleaned up
void sprintbit(uint32_t vektor, const char *names[], char *result)
{
  int32_t nr;

  *result = '\0';

  if (vektor < 0)
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Negative value sent to sprintbit");
    return;
  }

  for (nr = 0; vektor; vektor >>= 1)
  {
    if (isSet(1, vektor))
    {
      if (!strcmp(names[nr], "unused"))
        continue;
      if (*names[nr] != '\n')
        strcat(result, names[nr]);
      else
        strcat(result, "Undefined");
      strcat(result, " ");
    }

    if (*names[nr] != '\n')
      nr++;
  }

  if (*result == '\0')
    strcat(result, "NoBits ");
}

// leading space until all calling functions cleaned up
void sprintbit(uint32_t vektor, QStringList names, char *result)
{
  int32_t nr;

  *result = '\0';

  if (vektor < 0)
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Negative value sent to sprintbit");
    return;
  }

  for (nr = 0; vektor; vektor >>= 1)
  {
    if (isSet(1, vektor))
    {
      if (names[nr].compare("unused", Qt::CaseInsensitive) == 0)
        continue;

      strcat(result, names.value(nr, "Undefined").toStdString().c_str());
      strcat(result, " ");
    }

    if (nr < names.size() - 1)
      nr++;
  }

  if (*result == '\0')
    strcat(result, "NoBits ");
}

// leading space until all calling functions cleaned up
QString sprintbit(uint32_t vektor, QStringList names)
{
  int32_t nr;
  QString result;

  if (vektor < 0)
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Negative value sent to sprintbit");
    return {};
  }

  for (nr = 0; vektor; vektor >>= 1)
  {
    if (isSet(1, vektor))
    {
      if (names[nr].compare("unused", Qt::CaseInsensitive) == 0)
        continue;

      result += names.value(nr, "Undefined") + " ";
    }

    if (nr < names.size() - 1)
    {
      nr++;
    }
  }

  if (result.isEmpty())
  {
    result = "NoBits";
  }
  return result;
}

// no leading space
std::string sprintbit(uint32_t vektor, const char *names[])
{
  int32_t nr;

  std::string result = {};

  if (vektor < 0)
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Negative value sent to sprintbit");
    return result;
  }

  for (nr = 0; vektor; vektor >>= 1)
  {
    if (isSet(1, vektor))
    {
      if (!strcmp(names[nr], "unused"))
      {
        continue;
      }

      if (!result.empty())
      {
        result += " ";
      }

      if (*names[nr] != '\n')
      {
        result += names[nr];
      }
      else
      {
        result += "Undefined";
      }
    }

    if (*names[nr] != '\n')
      nr++;
  }

  if (result.empty())
    result = "NoBits";

  return result;
}

void sprinttype(uint64_t type, QStringList names, char *result)
{
  if (result)
  {
    strcpy(result, names.value(type, "Undefined").toStdString().c_str());
  }
}

QString sprinttype(uint64_t type, QStringList names)
{
  return names.value(type, "Undefined");
}

std::string sprinttype(int type, const char *names[])
{
  int nr;

  for (nr = 0; *names[nr] != '\n'; nr++)
    ;
  if (type > -1 && type < nr)
    return names[type];
  else
    return "Undefined";
}

std::string sprinttype(int type, item_types_t names)
{
  return names.value(type, "Undefined").toStdString();
}

int consttype(char *search_str, const char *names[])
{
  int nr;

  for (nr = 0; *names[nr] != '\n'; nr++)
    if (is_abbrev(search_str, names[nr]))
      return nr;

  return -1;
}

QString constindex(const qsizetype index, const QStringList names)
{
  if (names.size() > index)
  {
    return names.at(index);
  }

  return "";
}

// Calculate the MUD time passed over the last t2-t1 centuries (secs)
struct time_info_data mud_time_passed(time_t t2, time_t t1)
{
  int32_t secs;
  struct time_info_data now;

  secs = (int32_t)(t2 - t1);

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24; /* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35; /* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17; /* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR); /* 0..XX? years */

  return now;
}

struct time_info_data Character::age(void)
{
  struct time_info_data player_age;

  // TODO - make this return some sensible value for mobs
  if (isNPC())
  {
    player_age.year = 5;
    return player_age;
  }

  time_t birth = player->time.birth;
  player_age = mud_time_passed(time(0), birth);

  player_age.year += 17; /* All players start at 17 */
  player_age.year += GET_AGE_METAS(this);

  return player_age;
}

bool file_exists(const char *filename)
{
  FILE *fp;

  if ((fp = fopen(filename, "r")) == nullptr)
  {
    return false;
  }

  fclose(fp);
  return true;
}

void util_archive(const char *char_name, Character *caller)
{
  char buf[256];
  char buf2[256];
  int i;

  // Ok, ok, we'll do some sanity checking on the
  // std::string to make sure that it has no meta chars in
  // it.  Grumble. -Morc
  for (i = 0; (unsigned)i < strlen(char_name); i++)
  {
    if (!isalpha(char_name[i]))
    {
      if (caller)
      {
        sprintf(buf, "Illegal archive attempt: %s by %s.",
                char_name, GET_NAME(caller));
        logentry(buf, OVERSEER, LogChannels::LOG_GOD);
        return;
      }
      else
      {
        sprintf(buf, "Someone got a weird char name in there: %s.", char_name);
        logentry(buf, OVERSEER, LogChannels::LOG_GOD);
        return;
      }
    }
  }

  sprintf(buf, "%s/%c/%s", SAVE_DIR, UPPER(char_name[0]), char_name);
  sprintf(buf2, "%s/%s", ARCHIVE_DIR, char_name);
  if (!file_exists(buf) || file_exists(buf2))
  {
    if (caller)
      caller->sendln("That character does not exist.");
    else
      logentry(QStringLiteral("Attempt to archive a non-existent char."), IMMORTAL, LogChannels::LOG_BUG);
    return;
  }
  sprintf(buf, "%s -9 %s/%c/%s", GZIP, SAVE_DIR, UPPER(char_name[0]), char_name);
  if (system(buf))
  {
    sprintf(buf, "Unsuccessful archive: %s", char_name);
    if (caller)
      caller->send(QStringLiteral("%1\n\r").arg(buf));
    else
      logentry(buf, IMMORTAL, LogChannels::LOG_GOD);
    return;
  }
  sprintf(buf, "%s/%c/%s.gz", SAVE_DIR, UPPER(char_name[0]), char_name);
  sprintf(buf2, "%s/%s.gz", ARCHIVE_DIR, char_name);
  rename(buf, buf2);
  sprintf(buf, "Character archived: %s", char_name);
  if (caller)
    caller->send(QStringLiteral("%1\n\r").arg(buf));
  logentry(buf, IMMORTAL, LogChannels::LOG_GOD);
}

void util_unarchive(char *char_name, Character *caller)
{
  char buf[256];
  char buf2[256];
  int i;

  for (i = 0; (unsigned)i < strlen(char_name); i++)
  {
    if (!isalpha(char_name[i]))
    {
      if (caller)
      {
        sprintf(buf, "Illegal unarchive attempt: %s by %s.", char_name,
                GET_NAME(caller));
        logentry(buf, OVERSEER, LogChannels::LOG_GOD);
        return;
      }
      else
      {
        sprintf(buf, "Someone got a weird char name in there: %s.",
                char_name);
        logentry(buf, OVERSEER, LogChannels::LOG_GOD);
        return;
      }
    }
  }

  sprintf(buf, "%s/%s.gz", ARCHIVE_DIR, char_name);

  if (!file_exists(buf))
  {
    if (caller)
      caller->sendln("Character not archived or already deleted!");
    return;
  }
  sprintf(buf, "%s -d %s/%s.gz", GZIP, ARCHIVE_DIR, char_name);
  if (system(buf))
  {
    sprintf(buf, "Unsuccessful unarchive: %s", char_name);
    if (caller)
      caller->send(QStringLiteral("%1\n\r").arg(buf));
    else
      logentry(buf, IMMORTAL, LogChannels::LOG_GOD);
    return;
  }
  sprintf(buf, "%s/%s", ARCHIVE_DIR, char_name);
  sprintf(buf2, "%s/%c/%s", SAVE_DIR, UPPER(char_name[0]), char_name);
  rename(buf, buf2);
  sprintf(buf, "Character unarchived: %s", char_name);
  if (caller)
    caller->send(QStringLiteral("%1\n\r").arg(buf));
  logentry(buf, IMMORTAL, LogChannels::LOG_GOD);
}

bool ARE_CLANNED(Character *sub, Character *obj)
{
  if (IS_PC(sub) &&
      IS_MOB(obj) &&
      obj->master &&
      ARE_CLANNED(sub, obj->master) &&
      (IS_AFFECTED(obj, AFF_CHARM) || IS_AFFECTED(obj, AFF_FAMILIAR)))
    return true;

  // make sure we're clanned, and the person we're looking at is in same clan
  // (have to check if we're clanned, cause otherwise two non-clanned people
  // would count as being in "same clan")
  if (!sub->clan || sub->clan != obj->clan)
    return false;

  return true;
}

int DARK_AMOUNT(int room)
{
  int glow = DC::getInstance()->world[room].light;

  // indoors and cities are always lit
  if (DC::getInstance()->world[room].sector_type == SECT_INSIDE ||
      DC::getInstance()->world[room].sector_type == SECT_CITY)
    glow += 3;

  if (isSet(DC::getInstance()->world[room].room_flags, DARK))
    glow -= 2;

  if (isSet(DC::getInstance()->world[room].room_flags, LIGHT_ROOM))
    glow += 2;

  if (weather_info.sunlight == SUN_DARK)
    glow -= 1;

  return glow;
}

// Room light.
// 0 to + is light
// -1 to - is dark
// SUN_DARK = -1
bool IS_DARK(int room)
{
  int glow = DARK_AMOUNT(room);

  if (glow < 0)
    return true;

  return false;
}

bool ARE_GROUPED(Character *sub, Character *obj)
{
  struct follow_type *f;
  Character *k;

  if (obj == sub)
    return true;

  if (obj == nullptr || sub == nullptr)
    return false;

  if (IS_PC(sub) &&
      IS_NPC(obj) &&
      obj->master &&
      ARE_GROUPED(sub, obj->master) &&
      (IS_AFFECTED(obj, AFF_CHARM) || IS_AFFECTED(obj, AFF_FAMILIAR)))
    return true;

  if (!(k = sub->master))
    k = sub;

  if ((IS_AFFECTED(k, AFF_GROUP)) && (IS_AFFECTED(sub, AFF_GROUP)))
  {
    if ((k == obj) && (IS_AFFECTED(obj, AFF_GROUP)))
      return true;

    for (f = k->followers; f; f = f->next)
    {
      if ((f->follower == obj) && (IS_AFFECTED(obj, AFF_GROUP)))
        return true;
    }
  }
  return false;
}

int SWAP_CH_VICT(int value)
{
  int newretval = 0;

  if (isSet(value, eCH_DIED))
    SET_BIT(newretval, eVICT_DIED);
  else
    REMOVE_BIT(newretval, eVICT_DIED);

  if (isSet(value, eVICT_DIED))
    SET_BIT(newretval, eCH_DIED);
  else
    REMOVE_BIT(newretval, eCH_DIED);

  return newretval;
}

bool SOMEONE_DIED(int value)
{
  if (isSet(value, eCH_DIED) || isSet(value, eVICT_DIED))
    return true;
  return false;
}

bool CAN_SEE(Character *sub, Character *obj, bool noprog)
{
  if (obj == sub)
    return true;

  if (!sub || !obj)
  {
    logentry(QStringLiteral("Invalid pointer passed to CAN_SEE!"), ANGEL, LogChannels::LOG_BUG);
    return false;
  }

  if (!IS_MOB(obj))
  {
    if (!obj->player) // noncreated char
      return true;

    if (sub->getLevel() < obj->player->wizinvis)
    {
      if (obj->player->incognito == true)
      {
        if (sub->in_room != obj->in_room)
          return false;
        return true;
      }
      else
        return false;
    }
  }

  if (sub && IS_PC(sub) && sub->player && sub->player->holyLite)
    return true;

  if (!noprog && IS_NPC(obj))
  {
    int prog = mprog_can_see_trigger(sub, obj);
    if (isSet(prog, eEXTRA_VALUE))
      return true;
    else if (isSet(prog, eEXTRA_VAL2))
      return false;
  }
  if (IS_AFFECTED(obj, AFF_GLITTER_DUST) && obj->isMortal())
    return true;

  if (obj->in_room == DC::NOWHERE)
  {
    return false;
  }

  try
  {
    if (DC::getInstance()->world[obj->in_room].sector_type == SECT_FOREST && IS_AFFECTED(obj, AFF_FOREST_MELD) && IS_AFFECTED(obj, AFF_HIDE))
    {
      return false;
    }
  }
  catch (...)
  {
    return false;
  }

  if (IS_AFFECTED(sub, AFF_BLIND))
    return false;

  if (!IS_LIGHT(sub->in_room) && !IS_AFFECTED(sub, AFF_INFRARED))
    return false;

  if (IS_AFFECTED(obj, AFF_HIDE))
  {
    if (IS_AFFECTED(sub, AFF_true_SIGHT))
      return true;

    if (ARE_GROUPED(sub, obj))
    {
      if (!IS_AFFECTED(obj, AFF_INVISIBLE))
        return true; // if they're not invis.. they can always see

      if (IS_AFFECTED(sub, AFF_DETECT_INVISIBLE))
        return true; // if they have det invis.. they can see

      return false; // else they can't
    }

    if (is_hiding(obj, sub))
      return false;
  }

  if (!IS_AFFECTED(obj, AFF_INVISIBLE))
    return true;

  if (IS_AFFECTED(sub, AFF_DETECT_INVISIBLE))
    return true;

  return false;
}

bool CAN_SEE_OBJ(Character *sub, class Object *obj, bool blindfighting)
{
  int skill = 0;
  struct affected_type *cur_af;

  if (!IS_MOB(sub) && sub->player->holyLite)
    return true;

  int prog = oprog_can_see_trigger(sub, obj);
  if (isSet(prog, eEXTRA_VALUE))
    return true;
  else if (isSet(prog, eEXTRA_VAL2))
    return false;

  skill = 0;
  if ((cur_af = sub->affected_by_spell(SPELL_DETECT_GOOD)))
    skill = (int)cur_af->modifier;
  if ((skill >= 80 || sub->getLevel() >= IMMORTAL) && isexact("consecrateitem", GET_OBJ_NAME(obj)) && obj->obj_flags.value[0] == SPELL_CONSECRATE)
    return true;

  skill = 0;
  if ((cur_af = sub->affected_by_spell(SPELL_DETECT_EVIL)))
    skill = (int)cur_af->modifier;
  if ((skill >= 80 || sub->getLevel() >= IMMORTAL) && isexact("consecrateitem", GET_OBJ_NAME(obj)) && obj->obj_flags.value[0] == SPELL_DESECRATE)
    return true;

  if (IS_OBJ_STAT(obj, ITEM_NOSEE))
    return false;

  if (IS_AFFECTED(sub, AFF_BLIND))
  {
    if (blindfighting && skill_success(sub, nullptr, SKILL_BLINDFIGHTING))
      return true;
    else
      return false;
  }

  // only see beacons if you have detect magic up
  if ((cur_af = sub->affected_by_spell(SPELL_DETECT_MAGIC)))
    skill = (int)cur_af->modifier;

  if (GET_ITEM_TYPE(obj) == ITEM_BEACON && isSet(obj->obj_flags.extra_flags, ITEM_INVISIBLE))
  {
    if (!IS_AFFECTED(sub, AFF_DETECT_MAGIC))
    {
      return false;
    }
    else
    {
      if (skill < 50)
        return false;
    }
  }

  //   if (IS_AFFECTED(sub, AFF_true_SIGHT) )
  //      return true;

  if (IS_OBJ_STAT(obj, ITEM_INVISIBLE) && !IS_AFFECTED(sub, AFF_DETECT_INVISIBLE))
    return false;

  if (IS_DARK(sub->in_room) && !IS_AFFECTED(sub, AFF_INFRARED) && !IS_OBJ_STAT(obj, ITEM_GLOW))
    return false;

  return true;
}

bool check_blind(Character *ch)
{

  //   if (IS_AFFECTED(ch, AFF_true_SIGHT))
  //    return false;

  if (IS_PC(ch) && ch->player->holyLite)
    return false;

  if (IS_AFFECTED(ch, AFF_BLIND) && number(0, 4)) // 20% chance of seeing
  {
    ch->sendln("You can't see a damn thing!");
    return true;
  }

  return false;
}

int do_order(Character *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  char buf[256];
  bool found = false;
  int org_room;
  int retval;
  Character *victim;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eFAILURE;
  }

  if (!*name || !*message)
    ch->sendln("Order who to do what?");
  else if (!(victim = ch->get_char_room_vis(name)) &&
           str_cmp("follower", name) && str_cmp("followers", name))
    ch->sendln("That person isn't here.");
  else if (ch == victim)
    ch->sendln("You obviously suffer from schitzophrenia.");
  else
  {
    if (IS_AFFECTED(ch, AFF_CHARM))
    {
      ch->sendln("Your superior would not aprove of you giving orders.");
      return eFAILURE;
    }

    if (victim)
    {
      sprintf(buf, "$N orders you to '%s'", message);
      act(buf, victim, 0, ch, TO_CHAR, 0);
      act("$n gives $N an order.", ch, 0, victim, TO_ROOM, NOTVICT);
      if ((victim->master != ch) ||
          !(IS_AFFECTED(victim, AFF_CHARM) ||
            IS_AFFECTED(victim, AFF_FAMILIAR)))
        act("$n has an indifferent look.", victim, 0, 0, TO_ROOM, 0);
      else
      {
        ch->sendln("Ok.");
        victim->command_interpreter(message);
      }
    }
    else
    { /* This is order "followers" */
      sprintf(buf, "$n issues the order '%s'.", message);
      act(buf, ch, 0, victim, TO_ROOM, 0);

      org_room = ch->in_room;

      if (ch->followers)
        for (k = ch->followers; k && k != (follow_type *)0x95959595;
             k = k->next)
        {
          if (org_room == k->follower->in_room)
            if (IS_AFFECTED(k->follower, AFF_CHARM))
            {
              found = true;
              retval = k->follower->command_interpreter(message);
              if (isSet(retval, eCH_DIED))
                break; // k is no longer valid if it was a mob(always), get out now
            }
        }

      if (found)
        ch->sendln("Ok.");
      else
        ch->sendln("Nobody here are loyal subjects of yours!");
    }
  }
  return eSUCCESS;
}

int do_idea(Character *ch, char *argument, int cmd)
{
  FILE *fl;
  char str[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
  {
    ch->sendln("Monsters can't have ideas - Go away.");
    return eFAILURE;
  }

  /* skip whites */
  for (; isspace(*argument); argument++)
    ;

  if (!*argument)
  {
    ch->sendln("That doesn't sound like a good idea to me.  Sorry.");
    return eFAILURE;
  }

  if (!(fl = fopen(IDEA_LOG, "a")))
  {
    perror("do_idea");
    ch->sendln("Could not open the idea log.");
    return eFAILURE;
  }

  sprintf(str, "**%s[%d]: %s\n", GET_NAME(ch), DC::getInstance()->world[ch->in_room].number, argument);
  fputs(str, fl);
  fclose(fl);
  ch->sendln("Ok.  Thanks.");
  return eSUCCESS;
}

int do_typo(Character *ch, char *argument, int cmd)
{
  FILE *fl;
  char str[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
  {
    ch->sendln("Monsters can't spell - leave me alone.");
    return eFAILURE;
  }

  /* skip whites */
  for (; isspace(*argument); argument++)
    ;

  if (!*argument)
  {
    ch->sendln("I beg your pardon?");
    return eFAILURE;
  }

  if (!(fl = fopen(TYPO_LOG, "a")))
  {
    perror("do_typo");
    ch->sendln("Could not open the typo log.");
    return eFAILURE;
  }

  sprintf(str, "**%s[%d]: %s\n",
          GET_NAME(ch), DC::getInstance()->world[ch->in_room].number, argument);
  fputs(str, fl);
  fclose(fl);
  ch->sendln("Ok.  Thanks.");
  return eSUCCESS;
}

int do_bug(Character *ch, char *argument, int cmd)
{
  FILE *fl;
  char str[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
  {
    ch->sendln("You are a monster! Bug off!");
    return eFAILURE;
  }

  /* skip whites */
  for (; isspace(*argument); argument++)
    ;

  if (!*argument)
  {
    ch->sendln("Pardon?");
    return eFAILURE;
  }

  if (!(fl = fopen(BUG_LOG, "a")))
  {
    perror("do_bug");
    ch->sendln("Could not open the bug log.");
    return eFAILURE;
  }

  sprintf(str, "**%s[%d]: %s\n", GET_NAME(ch), DC::getInstance()->world[ch->in_room].number, argument);
  fputs(str, fl);
  fclose(fl);
  ch->sendln("Ok.");
  return eSUCCESS;
}

command_return_t Character::do_recall(QStringList arguments, int cmd)
{
  int location = {}, level = {}, cost = {}, x = {};
  Character *victim = {};
  Character *loop_ch = {};
  float cf = {};
  QString name;
  clan_data *clan = {};
  struct clan_room_data *room;
  int found = {};
  int retval = {};
  int is_mob = {};

  act("$n prays to $s God for transportation!", this, 0, 0, TO_ROOM, INVIS_NULL);

  if (IS_AFFECTED(this, AFF_CHARM))
    return eFAILURE;

  if (this->room().isArena())
  {
    this->sendln("TYou can't recall while in the arena.");
    return eFAILURE;
  }

  if (isSet(DC::getInstance()->world[this->in_room].room_flags, NO_MAGIC))
  {
    this->sendln("You can't use magic here.");
    return eFAILURE;
  }

  if (isSet(this->combat, COMBAT_BASH1) ||
      isSet(this->combat, COMBAT_BASH2))
  {
    this->sendln("You can't, you're bashed!");
    return eFAILURE;
  }

  if (arguments.isEmpty())
  {
    victim = this;
  }
  else
  {
    name = arguments.value(0);
    victim = get_char_room_vis(name);
    if (victim == nullptr)
    {
      this->sendln("Whom do you want to recall?");
      return eFAILURE;
    }

    if (!ARE_GROUPED(this, victim) && !ARE_CLANNED(this, victim))
    {
      send(QStringLiteral("You are not grouped or clanned with %1 so you cannot recall them.\r\n").arg(victim->getNameC()));
      return eFAILURE;
    }
  }

  if (IS_PC(this))
  {
    x = GET_WIS(this);
    uint64_t percent = number(1, 100);
    if (percent > x)
    {
      percent -= x;
    }

    // Additional 5% chance of failure when recalling across continents
    location = GET_HOME(victim);
    if (location > 0 && DC::getInstance()->zones.value(DC::getInstance()->world[victim->in_room].zone).continent != DC::getInstance()->zones.value(DC::getInstance()->world[location].zone).continent)
    {
      percent += 5;
    }

    if (percent > 50)
    {
      this->sendln("You failed in your recall!");
      return eFAILURE;
    }
  }

  if (victim->fighting && IS_PC(victim->fighting)) // PvP fight?
  {
    victim->sendln("The gods refuse to answer your prayers while you're fighting!");
    return eFAILURE;
  }

  if (victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF) || victim->isPlayerGoldThief())
  {
    victim->sendln("The gods frown upon your thieving ways and refuse to aid your escape.");
    return eFAILURE;
  }

  if (IS_NPC(this))
  {
    location = real_room(GET_HOME(this));
  }
  else
  {
    if (GET_HOME(victim) == 0 || victim->getLevel() < 11 || IS_AFFECTED(victim, AFF_CANTQUIT))
    {
      location = real_room(START_ROOM);
    }
    else
    {
      location = real_room(GET_HOME(victim));
    }

    if (location < 0)
    {
      this->sendln("Failed.");
      return eFAILURE;
    }

    if (isSet(DC::getInstance()->world[location].room_flags, NOHOME))
    {
      victim->sendln("The gods reset your home.");
      location = real_room(START_ROOM);
      GET_HOME(victim) = START_ROOM;
    }

    // make sure they arne't recalling into someone's chall

    if (isSet(DC::getInstance()->world[location].room_flags, CLAN_ROOM))
    {
      if (!victim->clan || !(clan = get_clan(victim)))
      {
        this->sendln("The gods frown on you, and reset your home.");
        location = real_room(START_ROOM);
        GET_HOME(victim) = START_ROOM;
      }
      else
      {
        for (room = clan->rooms; room; room = room->next)
          if (room->room_number == GET_HOME(victim))
            found = 1;

        if (!found)
        {
          this->sendln("The gods frown on you, and reset your home.");
          location = real_room(START_ROOM);
          GET_HOME(victim) = START_ROOM;
        }
      }
    }
  }

  if (location == -1)
  {
    victim->sendln("You are completely lost.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if ((isSet(DC::getInstance()->world[location].room_flags, CLAN_ROOM) || location == real_room(2354) || location == real_room(2355)) && IS_AFFECTED(victim, AFF_CHAMPION))
  {
    victim->sendln("No recalling into a clan hall whilst Champion, go to the Tavern!.");
    location = real_room(START_ROOM);
  }
  if (location >= 1900 && location <= 1999 && IS_AFFECTED(victim, AFF_CHAMPION))
  {
    victim->sendln("No recalling into a guild hall whilst Champion, go to the Tavern!.");
    location = real_room(START_ROOM);
  }

  // calculate the gold needed
  level = victim->getLevel();
  if ((level > 10) && (level <= DC::MAX_MORTAL_LEVEL))
  {
    cf = 1 + ((level - 11) * .347f);
    cost = (int)(3440 * cf);

    if (DC::getInstance()->zones.value(DC::getInstance()->world[victim->in_room].zone).continent != DC::getInstance()->zones.value(DC::getInstance()->world[location].zone).continent)
    {
      // Cross-continent recalling costs twice as much
      cost *= 2;
    }

    if (this->getGold() < (uint32_t)cost)
    {
      this->send(QStringLiteral("You don't have %1 gold!\n\r").arg(cost));
      return eFAILURE;
    }

    this->removeGold(cost);
  }

  if (IS_AFFECTED(victim, AFF_CURSE))
  {
    this->sendln("A curse affect prevents it.");
    return eFAILURE;
  }
  if (IS_AFFECTED(victim, AFF_SOLIDITY))
  {
    this->sendln("A solidity affect prevents it.");
    return eFAILURE;
  }

  for (loop_ch = DC::getInstance()->world[victim->in_room].people; loop_ch; loop_ch = loop_ch->next_in_room)
    if (loop_ch == victim || loop_ch->fighting == victim)
      stop_fighting(loop_ch);

  act("$n disappears.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  is_mob = IS_MOB(victim);
  retval = move_char(victim, location);

  if (!is_mob && !isSet(retval, eCH_DIED))
  { // if it was a mob, we might have died moving
    act("$n appears out of nowhere.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    do_look(victim, "", 0);
  }
  return retval;
}

int do_qui(Character *ch, char *argument, int cmd)
{
  ch->sendln("You have to write quit - no less, to quit!");
  return eSUCCESS;
}

int do_quit(Character *ch, char *argument, int cmd)
{
  int iWear;
  struct follow_type *k;
  clan_data *clan;
  struct clan_room_data *room;
  int found = 0;
  char buf[MAX_STRING_LENGTH];
  Object *obj, *tmp_obj;

  void find_and_remove_player_portal(Character * ch);

  /*
  | Code inserted by Morc 9 Apr 1997 to fix crasher
  */
  if (ch == 0)
  {
    logentry(QStringLiteral("do_quit received null char - problem!"), OVERSEER, LogChannels::LOG_BUG);
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (IS_NPC(ch))
    return eFAILURE;

  if (!isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) && cmd != 666 && ch->isMortal())
  {
    ch->sendln("This room doesn't feel...SAFE enough to do that.");
    return eFAILURE;
  }

  // If ch has follower, cant quit
  // NOTE: If we sent a 666 to do_quit, then it came from a zap or a boot
  // at this point, we've set the char back to level 1 (if from a zap), so
  // we'll end up with a fully equipped char with huge stats reset to level
  // 1

  if (cmd != 666)
  {
    for (k = ch->followers; k; k = k->next)
    {
      if (IS_AFFECTED(k->follower, AFF_CHARM))
      {
        ch->sendln("But you wouldn't want to just abandon your followers!");
        return eFAILURE;
      }
    }

    if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
    {
      ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
      return eFAILURE;
    }

    if (GET_POS(ch) == position_t::FIGHTING && cmd != 666)
    {
      ch->sendln("No way! You are fighting.");
      return eFAILURE;
    }

    if (GET_POS(ch) < position_t::STUNNED)
    {
      ch->sendln("You're not DEAD yet.");
      return eFAILURE;
    }

    if ((ch->isPlayerCantQuit() && cmd != 666) ||
        ch->isPlayerObjectThief() ||
        ch->isPlayerGoldThief())
    {
      ch->sendln("You can't quit, because you are still wanted!");
      return eFAILURE;
    }

    if (isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_QUIT) && cmd != 666)
    {
      ch->sendln("Something about this room makes it seem like a bad place to quit.");
      return eFAILURE;
    }

    if (ch->room().isArena())
    {
      ch->sendln("Don't make me zap you.....");
      return eFAILURE;
    }

    if (isSet(DC::getInstance()->world[ch->in_room].room_flags, CLAN_ROOM) && cmd != 666)
    {
      if (!ch->clan || !(clan = get_clan(ch)))
      {
        ch->sendln("This is a clan room dork.  Try joining one first.");
        return eFAILURE;
      }

      for (room = clan->rooms; room; room = room->next)
        if (ch->in_room == real_room(room->room_number))
          found = 1;

      if (!found)
      {
        ch->sendln("Chode! You can't quit in another clan's hall!");
        return eFAILURE;
      }
    }
    act("$n has left the game.", ch, 0, 0, TO_ROOM, INVIS_NULL);
  }

  // Finish off any performances
  if (IS_SINGING(ch))
    do_sing(ch, "stop", CMD_DEFAULT);

  extractFamiliar(ch);
  struct follow_type *fol, *fol_next;

  for (fol = ch->followers; fol; fol = fol_next)
  {
    fol_next = fol->next;
    if (IS_NPC(fol->follower) &&
        DC::getInstance()->mob_index[fol->follower->mobdata->nr].virt == 8)
    {
      release_message(fol->follower);
      extract_char(fol->follower, false);
    }
  }
  affect_from_char(ch, SPELL_IRON_ROOTS);
  affect_from_char(ch, SPELL_DIVINE_INTER);  // sloppy sloppy
  affect_from_char(ch, SPELL_NO_CAST_TIMER); // *sigh*
  affect_from_char(ch, SKILL_CM_TIMER);
  affect_from_char(ch, SPELL_IMMUNITY);
  affect_from_char(ch, SKILL_BATTLESENSE);
  affect_from_char(ch, SKILL_SMITE);

  if (ch->beacon)
    extract_obj(ch->beacon);

  if (ch->cRooms)
  {
    for (obj = object_list; obj; obj = tmp_obj)
    {
      tmp_obj = obj->next;
      if (DC::getInstance()->obj_index[obj->item_number].virt == CONSECRATE_OBJ_NUMBER)
        if (ch == (Character *)(obj->obj_flags.origin))
          extract_obj(obj);
    }
  }

  if (IS_AFFECTED(ch, AFF_CHAMPION))
  {
    REMBIT(ch->affected_by, AFF_CHAMPION);
    struct affected_type af;
    af.type = OBJ_CHAMPFLAG_TIMER;
    af.duration = 5;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(ch, &af);
    sprintf(buf, "\n\r##%s has just logged out, watch for the Champion flag to reappear!\n\r", GET_NAME(ch));
    send_info(buf);
  }
  find_and_remove_player_portal(ch);
  stop_all_quests(ch);

  if (cmd != 666)
    clan_logout(ch);

  update_wizlist(ch);

  if (!IS_MOB(ch) && ch->desc && !ch->desc->getPeerOriginalAddress().isNull())
  {
    if (ch->player->last_site)
      dc_free(ch->player->last_site);
#ifdef LEAK_CHECK
    ch->player->last_site = (char *)calloc(strlen(ch->desc->getPeerOriginalAddress().toString().toStdString().c_str()) + 1, sizeof(char));
#else
    ch->player->last_site = (char *)dc_alloc(strlen(ch->desc->getPeerOriginalAddress().toString().toStdString().c_str()) + 1, sizeof(char));
#endif
    strcpy(ch->player->last_site, ch->desc->getPeerOriginalAddress().toString().toStdString().c_str());
    ch->player->time.logon = time(0);
  }

  if (ch->desc)
  {
    save_char_obj(ch);
    if (!close_socket(ch->desc)) // if returns 0, then it already quit us out
      return eFAILURE | eCH_DIED;
  }
  else
  {
    save_char_obj(ch);
  }

  SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop falling off

  for (iWear = 0; iWear < MAX_WEAR; iWear++)
    if (ch->equipment[iWear])
      obj_to_char(unequip_char(ch, iWear, 1), ch);

  while (ch->carrying)
    extract_obj(ch->carrying);

  extract_char(ch, true);
  return eSUCCESS | eCH_DIED;
}

command_return_t Character::save(int cmd)
{
  // With the cmd numbers
  // 666 = save quietly
  // 10 = save
  // 9 = save with a round of lag
  // -pir 3/15/1999

  if (IS_NPC(this) || level_ > IMPLEMENTER)
    return eFAILURE;

  if (cmd != 666)
  {
    send(QStringLiteral("Saving %1.\r\n").arg(GET_NAME(this)));
  }

  if (IS_PC(this))
  {
    save_char_obj(this);
#ifdef USE_SQL
    save_char_obj_db(this);
#endif

    if (followers)
    {
      save_charmie_data(this);
    }

    if (player->golem)
    {
      save_golem_data(this); // Golem data, eh!
    }
  }

  return eSUCCESS;
}

// TODO - make some sort of auto-save, or "save" flag, so player's
//        that save after every other kill don't actually do it, but it
//        pretends that it does.  That way we can start reducing the amount
//        of writing we're doing.
command_return_t Character::do_save(QStringList arguments, int cmd)
{
  if (IS_IMMORTAL(this))
  {
    if (arguments.size() > 0)
    {
      if (arguments.at(0) == "hints")
      {
        send("Saving hints.\r\n");
        DC::getInstance()->save_hints();
        return eSUCCESS;
      }
    }
  }

  return save(cmd);
}

int do_home(Character *ch, char *argument, int cmd)
{
  clan_data *clan;
  struct clan_room_data *room;
  int found = 0;

  if (ch->isMortal())
  {
    if (!isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) ||
        ch->room().isArena())
    {
      send_to_char("This place doesn't sit right with you...not enough "
                   "security.\r\n",
                   ch);
      return eFAILURE;
    }

    if (isSet(DC::getInstance()->world[ch->in_room].room_flags, NOHOME))
    {
      ch->sendln("Something prevents it.");
      return eFAILURE;
    }

    if (ch->getLevel() < 11)
    {
      ch->sendln("You must grow a bit before you can leave the nursery.");
      GET_HOME(ch) = START_ROOM;
      return eFAILURE;
    }

    if (isSet(DC::getInstance()->world[ch->in_room].room_flags, CLAN_ROOM))
    {
      if (!ch->clan || !(clan = get_clan(ch)))
      {
        ch->sendln("This is a clan room dork.  Try joining one first.");
        return eFAILURE;
      }

      for (room = clan->rooms; room; room = room->next)
        if (ch->in_room == real_room(room->room_number))
          found = 1;

      if (!found)
      {
        ch->sendln("Chode! You can't set home in another clan's hall!");
        return eFAILURE;
      }
    }
  }

  ch->sendln("You now consider this place to be your home.");
  GET_HOME(ch) = DC::getInstance()->world[ch->in_room].number;
  return eSUCCESS;
}

command_return_t Character::generic_command(QStringList argument, int cmd)
{
  switch (cmd)
  {
  case CMD_SELL:
    send("You can't sell anything here!\r\n");
    break;
  case CMD_ERASE:
    send("You can't erase anything here!\r\n");
    break;
  default:
    send("Sorry, but you cannot do that here!\r\n");
    break;
  }

  return eSUCCESS;
}

int do_beep(Character *ch, char *argument, int cmd)
{
  ch->sendln("Beep!\a");
  return eSUCCESS;
}

// if a skill has a valid name, return it, else nullptr
QString get_skill_name(int skillnum)
{

  if (skillnum >= SKILL_SONG_BASE && skillnum <= SKILL_SONG_MAX)
    return Character::song_names.value(skillnum - SKILL_SONG_BASE);
  else if (skillnum >= SKILL_BASE && skillnum <= SKILL_MAX)
    return skills[skillnum - SKILL_BASE];
  else if (skillnum >= KI_OFFSET && skillnum <= (KI_OFFSET + MAX_KI_LIST))
    return ki[skillnum - KI_OFFSET];
  else if (skillnum >= 1 && skillnum <= MAX_SPL_LIST)
  {
    return spells[skillnum - 1];
  }
  else if (skillnum >= SKILL_INNATE_BASE && skillnum <= SKILL_INNATE_MAX)
    return innate_skills[skillnum - SKILL_INNATE_BASE];
  else if (skillnum >= BASE_SETS && skillnum <= SET_MAX)
    return set_list[skillnum - BASE_SETS].SetName;
  else if (skillnum >= RESERVED_BASE && skillnum <= RESERVED_MAX)
    return reserved[skillnum - RESERVED_BASE];
  return {};
}

// convert char std::string to int
// return true if successful, false if error
bool check_valid_and_convert(int &value, char *buf)
{
  value = atoi(buf);
  if (value == 0 && strcmp(buf, "0"))
    return false;

  return true;
}

// modified for new SETBIT et al. commands
void parse_bitstrings_into_int(const char *bits[], std::string remainder_args, Character *ch, uint value[])
{
  bool found = false;

  if (ch == nullptr)
  {
    return;
  }

  for (;;)
  {
    if (remainder_args.empty())
    {
      break;
    }

    std::string arg1;
    std::tie(arg1, remainder_args) = half_chop(remainder_args);
    if (arg1.empty())
    {
      break;
    }

    for (int x = 0; *bits[x] != '\n'; x++)
    {
      if (!strcmp("unused", bits[x]))
        continue;
      if (is_abbrev(arg1.c_str(), bits[x]))
      {
        if (ISSET(value, x + 1))
        {
          REMBIT(value, x + 1);
          csendf(ch, "%s flag REMOVED.\r\n", bits[x]);
        }
        else
        {
          SETBIT(value, x + 1);
          csendf(ch, "%s flag ADDED.\r\n", bits[x]);
        }
        found = true;
        break;
      }
    }
  }
  if (!found)
  {
    ch->sendln("No matching bits found.");
  }
}

void parse_bitstrings_into_int(QStringList bits, QString arg1, Character *ch, uint32_t &value)
{
  int found = false;

  for (int x = 0; x < bits.size(); ++x)
  {
    if (bits.value(x) != "unused" && is_abbrev(arg1, bits.value(x)))
    {
      if (isSet(value, (1 << x)))
      {
        REMOVE_BIT(value, (1 << x));
        if (ch != nullptr)
        {
          ch->send(QStringLiteral("%1 flag REMOVED.\r\n").arg(bits.value(x)));
        }
      }
      else
      {
        SET_BIT(value, (1 << x));
        if (ch != nullptr)
        {
          ch->send(QStringLiteral("%1 flag ADDED.\r\n").arg(bits.value(x)));
        }
      }
      found = true;
      break;
    }
  }
  if (!found && ch != nullptr)
  {
    ch->sendln("No matching bits found.");
  }
}

void parse_bitstrings_into_int(const char *bits[], const char *remainder_args, Character *ch, uint value[])
{
  return parse_bitstrings_into_int(bits, std::string(remainder_args), ch, value);
}

// calls below uint32_t version
void parse_bitstrings_into_int(const char *bits[], std::string remainder_args, Character *ch, uint16_t &value)
{
  int found = false;

  if (ch == nullptr)
  {
    return;
  }

  for (;;)
  {
    if (remainder_args.empty())
    {
      break;
    }

    std::string arg1;
    std::tie(arg1, remainder_args) = half_chop(remainder_args);

    if (arg1.empty())
    {
      break;
    }

    for (int x = 0; *bits[x] != '\n'; x++)
    {
      if (!strcmp("unused", bits[x]))
        continue;
      if (is_abbrev(arg1.c_str(), bits[x]))
      {
        if (isSet(value, (1 << x)))
        {
          REMOVE_BIT(value, (1 << x));
          csendf(ch, "%s flag REMOVED.\r\n", bits[x]);
        }
        else
        {
          SET_BIT(value, (1 << x));
          csendf(ch, "%s flag ADDED.\r\n", bits[x]);
        }
        found = true;
        break;
      }
    }
  }
  if (!found)
  {
    ch->sendln("No matching bits found.");
  }
}

void parse_bitstrings_into_int(const char *bits[], const char *remainder_args, Character *ch, uint16_t &value)
{
  return parse_bitstrings_into_int(bits, std::string(remainder_args), ch, value);
}

// Assumes bits is array of strings, ending with a "\n" std::string
// Finds the bits[] strings listed in "strings" and toggles the bit in "value"
// Informs 'ch' of what has happened
//
void parse_bitstrings_into_int(const char *bits[], std::string remainder_args, Character *ch, uint32_t &value)
{
  bool found = false;

  if (!ch)
    return;

  for (;;)
  {
    if (remainder_args.empty())
    {
      break;
    }

    std::string arg1;
    std::tie(arg1, remainder_args) = half_chop(remainder_args);

    if (arg1.empty())
    {
      break;
    }

    for (auto x = 0; *bits[x] != '\n'; x++)
    {
      if (!strcmp("unused", bits[x]))
      {
        continue;
      }

      if (is_abbrev(arg1.c_str(), bits[x]))
      {
        if (isSet(value, (1 << x)))
        {
          REMOVE_BIT(value, (1 << x));
          csendf(ch, "%s flag REMOVED.\r\n", bits[x]);
        }
        else
        {
          SET_BIT(value, (1 << x));
          csendf(ch, "%s flag ADDED.\r\n", bits[x]);
        }
        found = true;
        break;
      }
    }
  }
  if (!found)
    ch->send("No matching bits found.\n\n");
}

void parse_bitstrings_into_int(const char *bits[], const char *remainder_args, Character *ch, uint32_t &value)
{
  return parse_bitstrings_into_int(bits, std::string(remainder_args), ch, value);
}

void check_timer()
{ // Called once/sec
  struct timer_data *curr, *nex, *las;
  las = nullptr;
  for (curr = timer_list; curr; curr = nex)
  {
    nex = curr->next;
    curr->timeleft--;
    if (curr->timeleft <= 0)
    {
      (*(curr->function))(curr->arg1, curr->arg2, curr->arg3);
      if (!nex && curr->next)
        nex = curr->next;
      if (las)
        las->next = curr->next;
      else
        timer_list = curr->next;
      dc_free(curr);
      continue;
    }
    las = curr;
  }
  DC::getInstance()->removeDead();
}

int get_line(FILE *fl, char *buf)
{
  char temp[256] = {};
  int lines = 0;

  do
  {
    lines++;
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl))
    return 0;
  else
  {
    strcpy(buf, temp);
    return lines;
  }
}

//
// return value can include the from or to variable
//
/*
int number(int from, int to)
{
  if (from == to)
    return to;

  if (from > to)
  {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "BACKWARDS usage: numbers(%d, %d)!", from, to);
    logentry(buf, ANGEL, LogChannels::LOG_BUG);
    produce_coredump();
    return to;
  }
  int number = (to + 1) - from;

  number = from + (int)((double)number * ((double)random() / ((double)RAND_MAX + 1.0)));
  return number;
}
*/

// Random
int random_percent_change(uint percentage, int value)
{
  int diff = abs(round(value * (percentage / 100.0)));
  return number(-diff, diff) + value;
}

// Weighted such that the worst and best values are less likely to occur
int random_percent_change(int from, int to, int value)
{
  return round((number(from, to) / 100.0) * value) + value;
}

bool is_in_game(Character *ch)
{
  // Bug in code if this happens
  if (ch == 0)
  {
    logentry(QStringLiteral("nullptr args sent to is_pc_playing in utility.c!"), ANGEL, LogChannels::LOG_BUG);
    return false;
  }

  // ch is a mob
  if (IS_NPC(ch))
  {
    return false;
  }

  // Linkdead
  if (ch->desc == 0)
  {
    return true;
  }

  switch (STATE(ch->desc))
  {
  case Connection::states::PLAYING:
  case Connection::states::EDIT_MPROG:
  case Connection::states::WRITE_BOARD:
  case Connection::states::EDITING:
  case Connection::states::SEND_MAIL:
    return true;
    break;
  }

  return false;
}

void produce_coredump(void *ptr)
{
  logf(IMMORTAL, LogChannels::LOG_BUG, "produce_coredump called with pointer %p", ptr);

  static int counter = 0;

  if (++counter > COREDUMP_MAX)
  {
    logf(IMMORTAL, LogChannels::LOG_BUG, "Error detected: Unable to produce coredump. Limit of %d reached.", COREDUMP_MAX);
    return;
  }

  pid_t pid = fork();
  if (pid == 0)
  {
    // Child process
    abort();
  }
  else if (pid > 0)
  {
    // Parent process
    logf(IMMORTAL, LogChannels::LOG_BUG, "Error detected: Producing coredump %d of %d.", counter, COREDUMP_MAX);
  }
  else
  {
    logf(IMMORTAL, LogChannels::LOG_BUG, "Error detected: Unable to fork process.");
  }

  return;
}

const char *pluralize(int qty, const char *ending)
{
  if (qty == 0 || qty > 1)
  {
    return ending;
  }
  else
  {
    return "";
  }
}

void remove_character(QString name, BACKUP_TYPE backup)
{
  char src_filename[256];
  char dst_dir[256] = {0};
  char syscmd[512];
  struct stat statbuf;

  if (name.isEmpty())
  {
    return;
  }

  name[0] = name[0].toUpper();

  switch (backup)
  {
  case SELFDELETED:
    strncpy(dst_dir, "../archive/selfdeleted/", 256);
    break;
  case CONDEATH:
    strncpy(dst_dir, "../archive/condeath/", 256);
    break;
  case ZAPPED:
    strncpy(dst_dir, "../archive/zapped/", 256);
    break;
  case NONE:
    break;
  default:
    logf(108, LogChannels::LOG_GOD, "remove_character passed invalid BACKUP_TYPE %d for %s.", backup, name.toStdString().c_str());
    break;
  }

  if (DC::getInstance()->cf.bport)
  {
    snprintf(src_filename, 256, "%s/%c/%s", BSAVE_DIR, name[0], name.toStdString().c_str());
  }
  else
  {
    snprintf(src_filename, 256, "%s/%c/%s", SAVE_DIR, name[0], name.toStdString().c_str());
  }

  if (0 == stat(src_filename, &statbuf))
  {
    if (dst_dir[0] != 0)
    {
      snprintf(syscmd, 512, "mv -f %s %s", src_filename, dst_dir);
      system(syscmd);
    }
    else
    {
      unlink(src_filename);
    }
  }

  if (DC::getInstance()->cf.bport)
  {
    snprintf(src_filename, 256, "%s/%c/%s.backup", BSAVE_DIR, name[0], name.toStdString().c_str());
  }
  else
  {
    snprintf(src_filename, 256, "%s/%c/%s.backup", SAVE_DIR, name[0], name.toStdString().c_str());
  }

  if (0 == stat(src_filename, &statbuf))
  {
    if (dst_dir[0] != 0)
    {
      snprintf(syscmd, 512, "mv -f %s %s", src_filename, dst_dir);
      system(syscmd);
    }
    else
    {
      unlink(src_filename);
    }
  }
}

void remove_familiars(QString name, BACKUP_TYPE backup)
{
  char src_filename[256];
  char dst_dir[256] = {0};
  char syscmd[512];
  struct stat statbuf;

  if (name.isEmpty())
  {
    return;
  }

  name[0] = name[0].toUpper();

  switch (backup)
  {
  case SELFDELETED:
    strncpy(dst_dir, "../archive/selfdeleted/", 256);
    break;
  case CONDEATH:
    strncpy(dst_dir, "../archive/condeath/", 256);
    break;
  case ZAPPED:
    strncpy(dst_dir, "../archive/zapped/", 256);
    break;
  case NONE:
    break;
  default:
    logf(108, LogChannels::LOG_GOD, "remove_familiars passed invalid BACKUP_TYPE %d for %s.", backup, name.toStdString().c_str());
    break;
  }

  for (int i = 0; i < MAX_GOLEMS; i++)
  {
    snprintf(src_filename, 256, "%s/%c/%s.%d", FAMILIAR_DIR, name.toStdString().c_str()[0], name.toStdString().c_str(), i);

    if (0 == stat(src_filename, &statbuf))
    {
      if (dst_dir[0] != 0)
      {
        snprintf(syscmd, 512, "mv -f %s %s", src_filename, dst_dir);
        system(syscmd);
      }
      else
      {
        unlink(src_filename);
      }
    }
  }
}

bool check_make_camp(int room)
{
  Character *i, *next_i;
  bool campok = false;

  for (i = DC::getInstance()->world[room].people; i; i = next_i)
  {
    next_i = i->next_in_room;

    if (i->fighting)
      return false;
    if (IS_MOB(i) && !IS_AFFECTED(i, AFF_CHARM) && !IS_AFFECTED(i, AFF_FAMILIAR))
      return false;
    if (i->affected_by_spell(SKILL_MAKE_CAMP) && i->affected_by_spell(SKILL_MAKE_CAMP)->modifier == room)
      campok = true;
  }

  return campok;
}

int get_leadership_bonus(Character *ch)
{
  Character *leader;
  struct follow_type *f, *next_f;
  int highlevel = 0, bonus = 0;

  if (ch->master)
    leader = ch->master;
  else
    leader = ch;

  if (IS_MOB(ch) || ch->in_room != leader->in_room)
    return 0;
  if (!leader->affected_by_spell(SKILL_LEADERSHIP))
    return 0;

  if (ch->affected_by_spell(SKILL_LEADERSHIP) && ch->master)
    affect_from_char(ch, SKILL_LEADERSHIP);

  for (f = leader->followers; f; f = next_f)
  {
    next_f = f->next;

    if (highlevel < f->follower->getLevel())
      highlevel = f->follower->getLevel();
  }

  for (f = leader->followers; f; f = next_f)
  {
    next_f = f->next;

    if (IS_MOB(f->follower))
      continue;
    if (leader->in_room != f->follower->in_room)
      continue;
    if (f->follower->getLevel() + 25 <= highlevel)
      continue;
    if (!IS_AFFECTED(f->follower, AFF_GROUP))
      continue;

    bonus++;
  }

  return MIN(bonus, leader->affected_by_spell(SKILL_LEADERSHIP)->modifier);
}

void update_make_camp_and_leadership(void)
{
  struct affected_type af;
  int bonus = 0;
  const auto &character_list = DC::getInstance()->character_list;

  for_each(character_list.begin(), character_list.end(),
           [&af, &bonus](Character *i)
           {
             if (!i->fighting)
             {
               if (i->affected_by_spell(SKILL_SMITE))
                 affect_from_char(i, SKILL_SMITE);

               if (i->affected_by_spell(SKILL_PERSEVERANCE))
               {
                 affect_from_char(i, SKILL_PERSEVERANCE);
                 affect_from_char(i, SKILL_PERSEVERANCE_BONUS);
               }

               if (i->affected_by_spell(SKILL_BATTLESENSE))
                 affect_from_char(i, SKILL_BATTLESENSE);
             }

             if (i->in_room != DC::NOWHERE)
             {
               if (!check_make_camp(i->in_room))
               {
                 if (i->affected_by_spell(SKILL_MAKE_CAMP))
                 {
                   affect_from_char(i, SKILL_MAKE_CAMP);
                   send_to_room("The camp has been disturbed.\r\n", i->in_room);
                 }
                 if (i->affected_by_spell(SPELL_FARSIGHT) && i->affected_by_spell(SPELL_FARSIGHT)->modifier == 111)
                   affect_from_char(i, SPELL_FARSIGHT);
               }
               else
               {
                 if (!i->affected_by_spell(SPELL_FARSIGHT) && !IS_AFFECTED(i, AFF_FARSIGHT))
                 {
                   af.type = SPELL_FARSIGHT;
                   af.duration = -1;
                   af.modifier = 111;
                   af.location = 0;
                   af.bitvector = AFF_FARSIGHT;

                   affect_to_char(i, &af);
                 }
               }
             }
             bonus = get_leadership_bonus(i);

             if (i->changeLeadBonus == true)
             {
               i->changeLeadBonus = false;

               if (i->curLeadBonus != bonus)
               {
                 i->curLeadBonus = bonus;
                 affect_from_char(i, SKILL_LEADERSHIP_BONUS);

                 if (i->curLeadBonus)
                 {
                   af.type = SKILL_LEADERSHIP_BONUS;
                   af.duration = -1;
                   af.bitvector = -1;

                   if (i->affected_by_spell(SKILL_LEADERSHIP))
                   {
                     af.modifier = bonus * 2;
                     af.location = APPLY_HIT_N_DAM;
                     affect_to_char(i, &af);
                   }
                   else
                   {
                     af.modifier = bonus * -8;
                     af.location = APPLY_AC;
                     affect_to_char(i, &af);
                   }
                 }
               }
             }

             if (i->curLeadBonus != bonus)
               i->changeLeadBonus = true;
           });
}

void unique_scan(Character *victim)
{
  if (!victim)
    return;

  class Object *i = nullptr;
  class Object *j = nullptr;
  int k;
  std::map<int, int> virtnums;
  std::queue<Object *> found_items;

  for (k = 0; k < MAX_WEAR; k++)
  {
    if (victim->equipment[k])
    {
      if (isSet(victim->equipment[k]->obj_flags.more_flags, ITEM_UNIQUE))
      {
        if (virtnums.end() == virtnums.find(DC::getInstance()->obj_index[victim->equipment[k]->item_number].virt))
          virtnums[DC::getInstance()->obj_index[victim->equipment[k]->item_number].virt] = 1;
        else
          found_items.push(victim->equipment[k]);
      }
      if (GET_ITEM_TYPE(victim->equipment[k]) == ITEM_CONTAINER)
      { // search inside
        for (j = victim->equipment[k]->contains; j; j = j->next_content)
        {
          if (isSet(j->obj_flags.more_flags, ITEM_UNIQUE))
          {
            if (virtnums.end() == virtnums.find(DC::getInstance()->obj_index[j->item_number].virt))
              virtnums[DC::getInstance()->obj_index[j->item_number].virt] = 1;
            else
              found_items.push(j);
          }
        }
      }
    }
  }

  for (i = victim->carrying; i; i = i->next_content)
  {
    if (isSet(i->obj_flags.more_flags, ITEM_UNIQUE))
    {
      if (virtnums.end() == virtnums.find(DC::getInstance()->obj_index[i->item_number].virt))
        virtnums[DC::getInstance()->obj_index[i->item_number].virt] = 1;
      else
        found_items.push(i);
    }

    // does not support containers inside containers
    if (GET_ITEM_TYPE(i) == ITEM_CONTAINER)
    { // search inside
      for (j = i->contains; j; j = j->next_content)
      {
        if (isSet(j->obj_flags.more_flags, ITEM_UNIQUE))
        {
          if (virtnums.end() == virtnums.find(DC::getInstance()->obj_index[j->item_number].virt))
            virtnums[DC::getInstance()->obj_index[j->item_number].virt] = 1;
          else
            found_items.push(j);
        }
      }
    }
  }

  if (!found_items.empty())
  {
    logf(IMMORTAL, LogChannels::LOG_WARNINGS, "Player %s has duplicate unique items.", victim->getNameC());
    while (!found_items.empty())
    {
      logf(IMMORTAL, LogChannels::LOG_WARNINGS, "%s", found_items.front()->short_description);
      found_items.pop();
    }
  }

  return;
}

std::string replaceString(std::string message, std::string find, std::string replace)
{
  size_t j;

  if (find.empty())
    return message;
  if (replace.empty())
    return message;
  if (find == replace)
    return message;

  size_t find_length = find.length();
  for (; (j = message.find(find)) != std::string::npos;)
  {
    message.replace(j, find_length, replace);
  }
  return message;
}

char *numToStringTH(int number)
{
  switch (number)
  {
  case 1:
    return "first";
  case 2:
    return "second";
  case 3:
    return "third";
  case 4:
    return "fourth";
  case 5:
    return "fifth";
  case 6:
    return "sixth";
  case 7:
    return "seventh";
  case 8:
    return "eighth";
  case 9:
    return "ninth";
  case 10:
    return "tenth";
  case 11:
    return "eleventh";
  default:
    return "";
  }
}

bool champion_can_go(int room)
{
  try
  {
    // Champions can't enter class restricted rooms
    for (int c_class = 1; c_class < CLASS_MAX; c_class++)
    {
      if (DC::getInstance()->world[room].allow_class[c_class] == true)
      {
        return false;
      }
    }

    // Champions can't enter clan rooms
    if (isSet(DC::getInstance()->world[room].room_flags, CLAN_ROOM))
    {
      return false;
    }
  }
  catch (...)
  {
    return false;
  }

  return true;
}

/*
splitstring
This function does NOT ignore empty strings unless you tell it to.
example:
splitstring("std::string  with 2 spaces", " ")
returns:
"std::string"
""
"with"
"2"
"spaces"

If you do not want this behavior, include a third argument as true.
splitstring("std::string  with 2 spaces", " ", true)
*/
std::vector<std::string> splitstring(std::string splitme, std::string delims, bool ignore_empty)
{
  std::vector<std::string> result;
  unsigned int splitter;
  while ((splitter = splitme.find_first_of(delims)) != splitme.npos)
  {
    if (ignore_empty && splitter > 0)
      result.push_back(splitme.substr(0, splitter));
    splitme = splitme.substr(splitter + 1);
  }
  if (ignore_empty && splitme.length() > 0)
    result.push_back(splitme);
  return result;
}

/*
joinstring
This function does NOT ignore empty strings unless you tell it to.
example:
joinstring("hi" "im" "" "some" "vector", ",")
returns:
"hi,im,,some,vector"

If you do not want this behavior, include a third argument as true.
joinstring(somevectorofstrings, ",", true)
*/
std::string joinstring(std::vector<std::string> joinme, std::string delims, bool ignore_empty)
{
  std::string result;
  unsigned int i = 0;
  unsigned int joined = 0;
  for (i = 0; i < joinme.size(); i++)
  {
    if (ignore_empty && joinme[i].empty())
      continue;

    if (joined > 0)
      result += delims;

    result += joinme[i];
    joined++;
  }
  return result;
}

bool class_can_go(int ch_class, int room)
{
  bool classRestrictions = false;

  try
  {
    // Determine if any class restrictions are in place
    for (int c_class = 1; c_class < CLASS_MAX; c_class++)
    {
      if (DC::getInstance()->world[room].allow_class[c_class] == true)
      {
        classRestrictions = true;
      }
    }

    if (classRestrictions)
    {
      if (DC::getInstance()->world[room].allow_class[ch_class] != true)
      {
        return false;
      }
    }
  }
  catch (...)
  {
    return false;
  }

  return true;
}

const char *find_profession(int c_class, uint8_t profession)
{
  // TODO Fix
  return "Unknown";

  std::map<uint8_t, std::string> profession_list = professions[c_class];

  if (profession == 0)
  {
    return "None";
  }
  else if (profession_list[profession].empty())
  {
    return "Unknown";
  }
  else
  {
    return profession_list[profession].c_str();
  }
}

std::string get_isr_string(uint32_t isr, int8_t loc)
{
  if (!isSet(isr, 1 << loc))
  {
    return std::string();
  }

  switch (loc)
  {
  case 0:
    return "Pierce";
  case 1:
    return "Slash";
  case 2:
    return "Magic";
  case 3:
    return "Charm";
  case 4:
    return "Fire";
  case 5:
    return "Energy";
  case 6:
    return "Acid";
  case 7:
    return "Poison";
  case 8:
    return "Sleep";
  case 9:
    return "Cold";
  case 10:
    return "Paralyze";
  case 11:
    return "Bludgeon";
  case 12:
    return "Whip";
  case 13:
    return "Crush";
  case 14:
    return "Hit";
  case 15:
    return "Bite";
  case 16:
    return "Sting";
  case 17:
    return "Claw";
  case 18:
    return "Physical";
  case 19:
    return "Non-Magic";
  case 20:
    return "Ki";
  case 21:
    return "Song";
  case 22:
    return "Water";
  case 23:
    return "Fear";
  default:
    return "ErCode: Somebodydunfuckedup";
  }
}

bool isNowhere(Character *ch)
{
  return (ch && ch->in_room == DC::NOWHERE);
}

bool file_exists(std::string filename)
{
  struct stat buffer;

  if (stat(filename.c_str(), &buffer) == 0)
  {
    return true;
  }

  return false;
}

bool file_exists(QString filename)
{
  return QFile(filename).exists();
}

bool char_file_exists(std::string name)
{
  if (all_of(name.begin(), name.end(), [](char i)
             { return isalpha(i); }) == 0)
  {
    return false;
  }

  std::string filename;

  if (DC::getInstance()->cf.bport)
  {
    filename = fmt::format("{}/{}/{}", BSAVE_DIR, name[0], name);
  }
  else
  {
    filename = fmt::format("{}/{}/{}", SAVE_DIR, name[0], name);
  }

  return file_exists(filename);
}

void Character::setPOSFighting(void)
{
  if (!isFighting())
  {
    setFighting();

    first_damage = time(nullptr);
    damages = 0;
    damage_done = 0;
    last_damage = 0;
  }
}

void Character::setPlayerLastMob(vnum_t mob_vnum)
{
  std::string buffer;
  if (this->player == nullptr)
  {
    return;
  }

  if (mob_vnum != this->player->last_mob_edit)
  {
    send(fmt::format("Changing last mob vnum from {} to {}.\r\n", this->player->last_mob_edit, mob_vnum));
    this->player->last_mob_edit = mob_vnum;
  }
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return true if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix(const char *astr, const char *bstr)
{
  if (astr == nullptr)
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Str_prefix: null astr.", 0);
    return true;
  }

  if (bstr == nullptr)
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Str_prefix: null bstr.", 0);
    return true;
  }

  for (; *astr; astr++, bstr++)
  {
    if (LOWER(*astr) != LOWER(*bstr))
      return true;
  }

  return false;
}

bool str_prefix(QString astr, QString bstr)
{
  if (astr.isEmpty())
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Str_prefix: null astr.", 0);
    return true;
  }

  if (bstr.isEmpty())
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Str_prefix: null bstr.", 0);
    return true;
  }

  auto astr_i = astr.begin();
  auto bstr_i = bstr.begin();
  for (; astr_i != astr.end() && bstr_i != bstr.end(); astr_i++, bstr_i++)
  {
    if (astr_i->toLower() != bstr_i->toLower())
      return true;
  }

  return false;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns true is astr not part of bstr.
 *   (compatibility with historical functions).
 */

bool str_infix(QString astr, QString bstr)
{
  int sstr1;
  int sstr2;
  int ichar;

  if (astr.isEmpty())
    return false;

  QChar c0 = astr.at(0);
  sstr1 = astr.length();
  sstr2 = bstr.length();

  for (ichar = 0; ichar <= sstr2 - sstr1; ichar++)
  {
    if (c0 == bstr[ichar].toLower() && !str_prefix(astr, bstr.sliced(ichar)))
      return false;
  }

  return true;
}

void special_log(QString message)
{
  QFile special_logfile(QStringLiteral("../lib/special.txt"));

  if (!special_logfile.open(QIODevice::Append | QIODevice::Text))
  {
    logentry(QStringLiteral("Unable to open SPECIAL LOG FILE in special_log."), IMPLEMENTER, LogChannels::LOG_GOD);
    return;
  }

  QTextStream out(&special_logfile);
  out << message << "\n";
}

void Character::swapSkill(skill_t origSkill, skill_t newSkill)
{
  if (skills.contains(origSkill))
  {
    skills[newSkill] = skills[origSkill];
    skills[newSkill].skillnum = newSkill;
    skills.erase(origSkill);
  }
}

void Character::setSkillMin(skill_t skillnum, int minimum_learned)
{
  if (skills.contains(skillnum))
  {
    skills[skillnum].learned = MIN(skills[skillnum].learned, minimum_learned);
  }
}

char_skill_data &Character::getSkill(skill_t skillnum)
{
  if (skills.contains(skillnum))
  {
    return skills[skillnum];
  }

  static char_skill_data empty;
  empty = {};
  return empty;
}

void Character::setSkill(skill_t skillnum, int learned)
{
  skills[skillnum].skillnum = skillnum;
  skills[skillnum].learned = learned;
}

void Character::upSkill(skill_t skillnum, int learned)
{
  skills[skillnum].skillnum = skillnum;
  skills[skillnum].learned += learned;
}

int len_cmp(QString s1, QString s2)
{
  for (auto i1 = s1.begin(), i2 = s2.begin(); i1 != s1.end() && i2 != s2.end() && *i1 != ' '; i1++, i2++)
  {
    if (*i1 != *i2)
    {
      return i1->toLatin1() - i2->toLatin1();
    }
  }

  return 0;
}

bool operator!(load_status_t ls)
{
  return ls != load_status_t::success;
}