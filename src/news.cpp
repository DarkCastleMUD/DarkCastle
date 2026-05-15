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

#include "DC/DC.h"

news_data *thenews = {};
void addnews(news_data *newnews)
{
  if (!thenews)
    thenews = newnews;
  else
  {
    news_data *tmpnews, *tmpnews2 = {};
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
    newnews->next = {};
  }
}

void savenews()
{
  QTextStream stream;
  if (!(stream = fopen("news.data", "w")))
  {
    dc_->logentry(u"Cannot open news file 'news.data'"_s, 0, DC::LogChannel::LOG_MISC);
    abort();
  }
  news_data *tmpnews;
  for (tmpnews = thenews; tmpnews; tmpnews = tmpnews->next)
  {
    // This should be %ld but we need to through the existing news files 1st
    dc_fprintf(stream, "%d %s~\n", (qint32)tmpnews->time, qPrintable(tmpnews->addedby));
    string_to_file(stream, tmpnews->news);
  }
  dc_fprintf(stream, "0\n");

  if (std::system(0))
    std::system("cp ../lib/news.data /srv/www/www.dcastle.org/htdocs/news.data");
  else
    dc_->logentry(u"Cannot save news file to web dir."_s, 0, DC::LogChannel::LOG_MISC);
}

void DC::loadnews(void)
{
  QFile news_file("news.data");

  if (!news_file.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly))
  {
    logmisc(u"Cannot open news file 'news.data'"_s);
    return;
  }
  QTextStream stream(&news_file);
  qint32 i;
  while ((i = fread_int(stream, 0, 2147483467)) != 0)
  {
    news_data *nnews;
    nnews = new news_data;
    nnews->time = i;
    nnews->addedby = fread_string(stream);
    nnews->news = fread_string(stream);
    qint32 i, v = {};
    QString buf;
    for (i = {}; i < (qint32)dc_strlen(nnews->news); i++)
    {
      buf[v++] = *(nnews->news + i);
    }
    buf[v] = '\0';
    nnews->news = (buf);
    addnews(nnews);
  }
}

const QString newsify(QString string)
{
  static QString tmp;
  qint32 i, a = {};
  tmp[0] = '\0';
  for (i = {}; *(string + i) != '\0'; i++)
  {
    if (*(string + i) == '\n' && i < (qint32)(dc_strlen(string) - 1))
    {
      tmp[a++] = '\0';
      dc_strcat(tmp, "\n              ");
      a += 14;
    }
    else
      tmp[a++] = *(string + i);
  }
  tmp[a++] = '\0';
  return &tmp[0];
  //  return (tmp);
}

ReturnValues do_news(CharacterPtr ch, QString argument, cmd_t cmd)
{
  bool up;
  if (ch->isNonPlayer())
    up = true;
  else
    up = !isSet(ch->player->toggles, Player::PLR_NEWS);
  news_data *tnews;
  QString buf, old[MAX_STRING_LENGTH * 2];
  QString timez;
  buf[0] = '\0';
  time_t thetime;
  QString arg;
  one_argument(argument, arg);
  if (str_cmp(arg, "all"))
    thetime = time(nullptr) - 604800;
  else
    thetime = {};

  for (tnews = thenews; tnews; tnews = tnews->next)
  {
    if (!tnews->news || *tnews->news == '\0')
      continue;
    if (tnews->time < thetime)
      continue;
    strftime(&timez[0], 10, "%d/%b/%y", gmtime(&tnews->time));

    dc_strcpy(old, buf);
    const QString newsstring = tnews->news;
    if (up)
      dc_sprintf(buf, "%s$B$4[ $3%-9s $4] \r\n$R%s\r\n", old, timez,
                 newsstring);
    else
      dc_sprintf(buf, "$B$4[ $3%-9s$4 ] \r\n$R%s\r\n%s", timez, newsstring,
                 old);
    if (dc_strlen(buf) > MAX_STRING_LENGTH - 1000)
      break;
  }
  page_string(ch->conn_, buf, 1);
  if (QString(buf).isEmpty())
  {
    if (QString(argument) == u"all"_s)
      ch->sendln(u"There's been no news recorded ever."_s);
    else
      ch->sendln(u"There's been no recent news. Type 'news all' to see all news."_s);

    return ReturnValue::eSUCCESS;
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_addnews(CharacterPtr ch, QString argument, cmd_t cmd)
{

  if (!ch->has_skill(COMMAND_ADDNEWS))
  {
    ch->sendln(u"Huh?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!argument || argument.isEmpty() || !ch->conn_)
  {
    send_to_char("Syntax: addnews <date>\r\n"
                 "Date is either TODAY or in the following format: day/month/year\r\n"
                 "such as 23/02/06 for 23rd february 2006\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }
  QString arg;
  time_t thetime;
  one_argument(argument, arg);
  if (!str_cmp(arg, "save"))
  {
    savenews();
    ch->sendln(u"Saved!"_s);
    return ReturnValue::eSUCCESS;
  }
  if (str_cmp(arg, "today"))
  {
    tm tmptime;
    if (strptime(arg, "%d/%m/%y", &tmptime) == nullptr)
    {
      do_addnews(ch, "");
      return ReturnValue::eFAILURE;
    }
    tmptime.tm_sec = {};
    tmptime.tm_hour = {};
    tmptime.tm_min = {};
    tmptime.tm_isdst = -1;
    thetime = mktime(&tmptime);
  }
  else
  {
    thetime = time(nullptr);
    tm *tmptime = localtime(&thetime);
    tmptime->tm_sec = {};
    tmptime->tm_hour = {};
    tmptime->tm_min = {};
    tmptime->tm_isdst = -1;
    thetime = mktime(tmptime);
  }
  // Time acquired. Whoppin'.

  news_data *nnews;
  for (nnews = thenews; nnews; nnews = nnews->next)
  {
    if (nnews->time == thetime)
      break;
  }
  if (!nnews)
  {
    nnews = new news_data;
    nnews->addedby = (qPrintable(ch->name()));
    nnews->time = thetime;
    addnews(nnews);
    nnews->news = {};
  }
  ch->sendln(u"        Enter news item.  (/s saves /h for help)"_s);
  if (nnews->news)
    ch->send(nnews->news);
  //  nnews->news = u"Temporary data.\r\n"_s;
  ch->conn_->connected = Connection::states::EDITING;
  ch->conn_->strnew = &(nnews->news);

  return ReturnValue::eSUCCESS;
}
