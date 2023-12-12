/****************************************************************************
 *  file: news.cpp , Implementation of commands.           Part of DIKUMUD  *
 *   Usage : Informative commands.                                          *
 *   Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                          *
 *   Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *   Performance optimization and bug fixes by MERC Industries.             *
 *   You can use our stuff in any way you like whatsoever so long as ths    *
 *   copyright notice remains intact.  If you like it please drop a line    *
 *   to mec@garnet.berkeley.edu.                                            *
 *                                                                          *
 *   This is free software and you are benefitting.  We hope that you       *
 *   share your changes too.  What goes around, comes around.               *
 ****************************************************************************/

extern "C"
{
#include <ctype.h>
#include <string.h>
}
#include <cstdlib>
#include "structs.h"
#include "room.h"
#include "character.h"
#include "obj.h"
#include "utility.h"
#include "terminal.h"
#include "player.h"
#include "levels.h"
#include "mobile.h"
#include "clan.h"
#include "handler.h"
#include "db.h" // exp_table
#include "interp.h"
#include "connect.h"
#include "spells.h"
#include "race.h"
#include "act.h"
#include "set.h"
#include "returnvals.h"
#include "news.h"

struct news_data *thenews = nullptr;
void addnews(struct news_data *newnews)
{
  if (!thenews)
    thenews = newnews;
  else
  {
    struct news_data *tmpnews, *tmpnews2 = nullptr;
    for (tmpnews = thenews; tmpnews; tmpnews = tmpnews->next)
    {
      if (tmpnews->time < newnews->time)
      {
        if (tmpnews2)
        {
          tmpnews2->next = newnews;
          newnews->next = tmpnews;
          return;
        }
        else
        {
          newnews->next = thenews;
          thenews = newnews;
          return;
        }
      }
      tmpnews2 = tmpnews;
    }
    tmpnews2->next = newnews;
    newnews->next = nullptr;
  }
}

void savenews()
{
  FILE *fl;
  if (!(fl = fopen("news.data", "w")))
  {
    logentry("Cannot open news file.", 0, LogChannels::LOG_MISC);
    abort();
  }
  struct news_data *tmpnews;
  for (tmpnews = thenews; tmpnews; tmpnews = tmpnews->next)
  {
    // This should be %ld but we need to through the existing news files 1st
    fprintf(fl, "%d %s~\n", (int)tmpnews->time, tmpnews->addedby);
    string_to_file(fl, tmpnews->news);
  }
  fprintf(fl, "0\n");
  fclose(fl);
  if (std::system(0))
    std::system("cp ../lib/news.data /srv/www/www.dcastle.org/htdocs/news.data");
  else
    logentry("Cannot save news file to web dir.", 0, LogChannels::LOG_MISC);
}

void loadnews()
{
  FILE *fl;
  if (!(fl = fopen("news.data", "r")))
  {
    logentry("Cannot open news file.", 0, LogChannels::LOG_MISC);
    return;
  }
  int i;
  while ((i = fread_int(fl, 0, 2147483467)) != 0)
  {
    struct news_data *nnews;
#ifdef LEAK_CHECK
    nnews = (struct news_data *)
        calloc(1, sizeof(struct news_data));
#else
    nnews = (struct news_data *)
        dc_alloc(1, sizeof(struct news_data));
#endif
    nnews->time = i;
    nnews->addedby = fread_string(fl, 0);
    nnews->news = fread_string(fl, 0);
    int i, v = 0;
    char buf[MAX_STRING_LENGTH];
    for (i = 0; i < (int)strlen(nnews->news); i++)
    {
      buf[v++] = *(nnews->news + i);
    }
    buf[v] = '\0';
    nnews->news = str_dup(buf);
    addnews(nnews);
  }
  fclose(fl);
}

const char *newsify(char *string)
{
  static char tmp[MAX_STRING_LENGTH * 2];
  int i, a = 0;
  tmp[0] = '\0';
  for (i = 0; *(string + i) != '\0'; i++)
  {
    if (*(string + i) == '\n' && i < (int)(strlen(string) - 1))
    {
      tmp[a++] = '\0';
      strcat(tmp, "\n              ");
      a += 14;
    }
    else
      tmp[a++] = *(string + i);
  }
  tmp[a++] = '\0';
  return &tmp[0];
  //  return str_dup(tmp);
}

int do_news(Character *ch, char *argument, int cmd)
{
  bool up;
  if (IS_NPC(ch))
    up = true;
  else
    up = !DC::isSet(ch->player->toggles, Player::PLR_NEWS);
  struct news_data *tnews;
  char buf[MAX_STRING_LENGTH * 2], old[MAX_STRING_LENGTH * 2];
  char timez[15];
  buf[0] = '\0';
  time_t thetime;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);
  if (str_cmp(arg, "all"))
    thetime = time(nullptr) - 604800;
  else
    thetime = 0;

  for (tnews = thenews; tnews; tnews = tnews->next)
  {
    if (!tnews->news || *tnews->news == '\0')
      continue;
    if (tnews->time < thetime)
      continue;
    strftime(&timez[0], 10, "%d/%b/%y", gmtime(&tnews->time));

    strcpy(old, buf);
    const char *newsstring = tnews->news;
    if (up)
      sprintf(buf, "%s$B$4[ $3%-9s $4] \r\n$R%s\r\n", old, timez,
              newsstring);
    else
      sprintf(buf, "$B$4[ $3%-9s$4 ] \r\n$R%s\r\n%s", timez, newsstring,
              old);
    if (strlen(buf) > MAX_STRING_LENGTH - 1000)
      break;
  }
  page_string(ch->desc, buf, 1);
  return eSUCCESS;
}

int do_addnews(Character *ch, char *argument, int cmd)
{

  if (!ch->has_skill(COMMAND_ADDNEWS))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  if (!argument || !*argument || !ch->desc)
  {
    send_to_char("Syntax: addnews <date>\r\n"
                 "Date is either TODAY or in the following format: day/month/year\r\n"
                 "such as 23/02/06 for 23rd february 2006\r\n",
                 ch);
    return eFAILURE;
  }
  char arg[MAX_INPUT_LENGTH];
  time_t thetime;
  one_argument(argument, arg);
  if (!str_cmp(arg, "save"))
  {
    savenews();
    ch->sendln("Saved!");
    return eSUCCESS;
  }
  if (str_cmp(arg, "today"))
  {
    struct tm tmptime;
    if (strptime(arg, "%d/%m/%y", &tmptime) == nullptr)
    {
      do_addnews(ch, "", 9);
      return eFAILURE;
    }
    tmptime.tm_sec = 0;
    tmptime.tm_hour = 0;
    tmptime.tm_min = 0;
    tmptime.tm_isdst = -1;
    thetime = mktime(&tmptime);
  }
  else
  {
    thetime = time(nullptr);
    struct tm *tmptime = localtime(&thetime);
    tmptime->tm_sec = 0;
    tmptime->tm_hour = 0;
    tmptime->tm_min = 0;
    tmptime->tm_isdst = -1;
    thetime = mktime(tmptime);
  }
  // Time acquired. Whoppin'.

  struct news_data *nnews;
  for (nnews = thenews; nnews; nnews = nnews->next)
  {
    if (nnews->time == thetime)
      break;
  }
  if (!nnews)
  {
#ifdef LEAK_CHECK
    nnews = (struct news_data *)
        calloc(1, sizeof(struct news_data));
#else
    nnews = (struct news_data *)
        dc_alloc(1, sizeof(struct news_data));
#endif
    nnews->addedby = str_dup(GET_NAME(ch));
    nnews->time = thetime;
    addnews(nnews);
    nnews->news = nullptr;
  }
  ch->sendln("        Enter news item.  (/s saves /h for help)");
  if (nnews->news)
    ch->send(nnews->news);
  //  nnews->news = str_dup("Temporary data.\r\n");
  ch->desc->connected = Connection::states::EDITING;
  ch->desc->strnew = &(nnews->news);
  ch->desc->max_str = 2096;

  return eSUCCESS;
}
