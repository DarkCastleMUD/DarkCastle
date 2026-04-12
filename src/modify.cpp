/***************************************************************************
 *  file: modify.c                                         Part of DIKUMUD *
 *  Usage: Run-time modification (by users) of game variables              *
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
 ***************************************************************************/
/* $Id: modify.cpp,v 1.33 2014/07/04 22:00:04 jhhudso Exp $ */

#include <cstdio>
#include <cctype>
#include <cstring>
#include <ctime>
#include <cstdlib>

#include "DC/obj.h"
#include "DC/connect.h" // Connection

#include "DC/DC.h"

#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/db.h"
#include <qdebug.h>
#include <QString>

// TODO - what does this do?  Nothing that I can see....let's remove it....
constexpr auto REBOOT_AT = 10; /* 0-23, time of optional reboot if -e lib/reboot */

constexpr auto TP_MOB = 0;
constexpr auto TP_OBJ = 1;
constexpr auto TP_ERROR = 2;

void check_for_awaymsgs(CharacterPtr ch);
void page_string_dep(class Connection *d, const QString str, qint32 keep_internal);

const QStringList string_fields = {"name", "short", "long", "description", "title", "delete-description", "\n"};

// maximum length for text field x+1
qint32 length[] = {40, 60, 256, 240, 60};

const QStringList skill_fields = {"learned", "recognize", "\n"};

// TODO - I'd like to put together some sort of "post office" for sending "mail"
//  to players that are offline.  (they get notified when they login, and have to
//  go pick it up)  Note:  There's a "CON_SEND_MAIL" already defined....not sure
//  why...

void string_hash_add(class Connection *d, QString str)
{
  QString scan;
  qint32 terminator = {};
  CharacterPtr ch = conn->character;

  scan = str;
  while (*scan)
  {
    if ((terminator = (*scan == '~') != 0))
    {
      *scan = '\0';
      break;
    }
    scan++;
  }

  if (!(*conn->hashstr))
  {
    if (strlen(str) > (quint32)conn->max_str)
    {
      conn->character->sendln("String too long - Truncated.");
      *(str + conn->max_str) = '\0';
      terminator = 1;
    }
#ifdef LEAK_CHECK
    (*conn->hashstr) = calloc(strlen(str) + 3, sizeof(QChar));
#else
    (*conn->hashstr) = dc_alloc(strlen(str) + 3, sizeof(QChar));
#endif
    dc_strcpy(*conn->hashstr, str);
  }

  else
  {
    if (strlen(str) + strlen(*conn->hashstr) > (quint32)conn->max_str)
    {
      conn->character->sendln("String too long. Last line skipped.");
      terminator = 1;
    }

    else
    {
      if (!(*conn->hashstr = realloc(*conn->hashstr, strlen(*conn->hashstr) + strlen(str) + 3)))
      {
        perror("string_hash_add: ");
        abort();
      }

      dc_strcat(*conn->hashstr, str);
    }
  }

  if (terminator)
  {
    scan = *conn->hashstr;
    *conn->hashstr = {};
    *conn->hashstr = scan;
    conn->hashstr = {};
    conn->connected = Connection::states::PLAYING;
    ch->sendln("Ok.");
    check_for_awaymsgs(ch);
  }
  else
    dc_strcat(*conn->hashstr, "\r\n");
}

// TODO - figure out what this is for...kill it if nothing
#undef MAX_STR

/* interpret an argument for do_string */
void quad_arg(QString arg, qint32 *type, QString name, qint32 *field, QString string)
{
  QString buf;

  /* determine type */
  arg = one_argument(arg, buf);
  if (is_abbrev(buf, "character"))
    *type = TP_MOB;
  else if (is_abbrev(buf, "obj"))
    *type = TP_OBJ;
  else
  {
    *type = TP_ERROR;
    return;
  }

  /* find name */
  arg = one_argument(arg, name);

  /* field name and number */
  arg = one_argument(arg, buf);
  if (!(*field = old_search_block(buf, 0, strlen(buf), string_fields, 0)))
    return;

  /* string */
  for (; isspace(*arg); arg++)
    ;
  for (; (*string = *arg) != '\0'; arg++, string++)
    ;
}

command_return_t do_string(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString name, string;
  QString message;
  qint32 field, type, ctr;
  CharacterPtr mob = {};
  ObjectPtr obj;
  extra_descr_data *ed, *tmp;

  if (ch->isNonPlayer())
    return 1;

  quad_arg(arg, &type, name, &field, string);

  if (type == TP_ERROR)
  {
    send_to_char("Syntax:\r\nstring ('obj'|'character') <name> <field>"
                 " [<string>].\r\n",
                 ch);
    return 1;
  }

  if (!field)
  {
    ch->sendln("No field by that name. Try 'help string'.");
    return 1;
  }

  if (type == TP_MOB)
  {
    /* locate the beast */
    if (!(mob = get_char_vis(ch, name)))
    {
      ch->sendln("I don't know anyone by that name...");
      return 1;
    }

    if ((mob->getLevel() > ch->getLevel()) && mob->isPlayer())
    {
      dc_sprintf(message, "%s can string himself, thank you.\r\n", qPrintable(mob->shortdesc_or_name()));
      ch->send(message);
      return 1;
    }

    switch (field)
    {
    case 1:
      if (mob->isPlayer() && ch->getLevel() < IMPLEMENTER)
      {
        ch->send("You can't change that field for players.");
        return 1;
      }

      ch->sendln("This is broken.");
      DC::getInstance()->logentry(u"do_string: broken"_s);
      /*
      TODO
      if (mob->isNonPlayer())
              ch->desc->hashstr = mob->getNameCPtr();
      else
              ch->desc->strnew = mob->getNameCPtr();
      */

      if (mob->isPlayer())
        ch->sendln("WARNING: You have changed the name of a player.");
      break;
    case 2:
      if (ch->getLevel() < POWER)
      {
        ch->sendln("You must be a God to do that.");
        return 1;
      }
      dc_sprintf(message, "%s just restrung short on %s", qPrintable(ch->name()), qPrintable(mob->name()));
      DC::getInstance()->logentry(message, IMPLEMENTER, DC::LogChannel::LOG_GOD);
      if (mob->isNonPlayer())
        ch->desc->hashstr = &mob->short_desc;
      else
        ch->desc->strnew = &mob->short_desc;
      break;
    case 3:
      if (mob->isPlayer())
      {
        ch->sendln("That field is for monsters only.");
        return 1;
      }
      ch->desc->hashstr = &mob->long_desc;
      break;
    case 4:
      if (mob->isNonPlayer())
        ch->desc->hashstr = &mob->description;
      else
        ch->desc->strnew = &mob->description;
      break;
    case 5:
      if (mob->isNonPlayer())
      {
        ch->sendln("Monsters have no titles.");
        return 1;
      }
      ch->desc->strnew = &mob->title;
      break;
    default:
      ch->sendln("That field is undefined for monsters.");
      return 1;
    }
  }

  /* type == TP_OBJ */
  else
  {
    /* locate the object */
    if (!(obj = get_obj_vis(ch, name)))
    {
      ch->sendln("Can't find such a thing here..");
      return 1;
    }

    if (isSet(obj->obj_flags.more_flags, ITEM_NO_RESTRING))
    {
      if (ch->getLevel() < IMPLEMENTER)
      {
        ch->sendln("That item is not restringable.");
        return 1;
      }
      else
        ch->sendln("That item is NO_RESTRING btw.");
    }

    switch (field)
    {
    case 1:
      if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL) && ch->getLevel() < 110)
      {
        ch->sendln("The moose will get you if you do that.");
        return 1;
      }
      // TODO hashstr for qstring
      // ch->desc->hashstr = &qPrintable(obj->name());
      break;
    case 2:
      ch->desc->hashstr = &obj->short_description;
      break;
    case 3:
      ch->desc->hashstr = &obj->long_description;
      break;
    case 4:
      // TODO - remove this when the obj pfile saving keeps track of extra descs
      ch->send("Noone may restring object extra descs at this time. -pir");
      return 1;

      if (string.isEmpty())
      {
        ch->sendln("You have to supply a keyword.");
        return 1;
      }
      /* try to locate extra description */
      for (ed = obj->ex_description;; ed = ed->next)
        if (!ed)
        { /* the field was not found. create a new_new one. */
#ifdef LEAK_CHECK
          ed = (extra_descr_data *)
              calloc(1, sizeof(extra_descr_data));
#else
          ed = (extra_descr_data *)dc_alloc(1, sizeof(extra_descr_data));
#endif
          ed->next = obj->ex_description;
          obj->ex_description = ed;
          ed->keyword = string;
          ed->description = {};
          ch->desc->hashstr = &ed->description;
          ch->sendln("New field.");
          break;
        }
        else if (!str_cmp(ed->keyword, string))
        {
          /* the field exists */
          ed->description = {};
          ch->desc->hashstr = &ed->description;
          ch->sendln("Modifying description.");
          break;
        }
      ch->desc->max_str = MAX_STRING_LENGTH;
      /* the stndrd (see below) procedure does not apply here */
      return 1;
      break;
    case 6:
      if (string.isEmpty())
      {
        ch->sendln("You must supply a field name.");
        return 1;
      }
      /* try to locate field */
      for (ed = obj->ex_description;; ed = ed->next)
        if (!ed)
        {
          ch->sendln("No field with that keyword.");
          return 1;
        }
        else if (!str_cmp(ed->keyword, string))
        {
          /* delete the entry in the desr list */
          if (ed == obj->ex_description)
            obj->ex_description = ed->next;
          else
          {
            for (tmp = obj->ex_description; tmp->next != ed; tmp = tmp->next)
              ;
            tmp->next = ed->next;
          }
          ed = {};
          ch->sendln("Field deleted.");
          return 1;
        }
      break;
    default:
      ch->sendln("That field is undefined for objects.");
      return 1;
    }
  }

  /* there was a string in the argument array */
  if (*string)
  {
    for (ctr = {}; (quint32)ctr <= strlen(string); ctr++)
    {
      if (string[ctr] == '$')
      {
        string[ctr] = ' ';
      }
    }

    if (strlen(string) > (quint32)length[field - 1])
    {
      ch->sendln("String too long - truncated.");
      *(string + length[field - 1]) = '\0';
    }
    if (type == TP_MOB && mob->isPlayer())
    {
      *ch->desc->strnew = (string);
      ch->desc->strnew = {};
    }
    else
    {
      *ch->desc->hashstr = string;
      ch->desc->hashstr = {};
    }
    ch->sendln("Ok.");
  }

  /* there was no string. enter string mode */
  else
  {
    send_to_char("Enter string. Terminate with '~' at the beginning "
                 "of a line.\r\n",
                 ch);
    if (type == TP_MOB && mob->isPlayer())
#ifdef LEAK_CHECK
      (*ch->desc->strnew) = calloc(length[field - 1], sizeof(QChar));
#else
      (*ch->desc->strnew) = dc_alloc(length[field - 1], sizeof(QChar));
#endif
    else
#ifdef LEAK_CHECK
      (*ch->desc->hashstr) = calloc(length[field - 1], sizeof(QChar));
#else
      (*ch->desc->hashstr) = dc_alloc(length[field - 1], sizeof(QChar));
#endif
    ch->desc->max_str = length[field - 1];
    ch->desc->connected = Connection::states::EDITING;
  }
  return 1;
}

/* db stuff *********************************************** */

/* One_Word is like one_argument, execpt that words in quotes "" are */
/* regarded as ONE word                                              */

QString one_word(QString argument, QString first_arg)
{
  qint32 begin, look_at;

  begin = {};

  do
  {
    for (; isspace(*(argument + begin)); begin++)
      ;

    if (*(argument + begin) == '\"')
    { /* is it a quote */

      begin++;

      for (look_at = {}; (*(argument + begin + look_at) >= ' ') && (*(argument + begin + look_at) != '\"'); look_at++)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));

      if (*(argument + begin + look_at) == '\"')
        begin++;
    }
    else
    {

      for (look_at = {}; *(argument + begin + look_at) > ' '; look_at++)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
    }

    *(first_arg + look_at) = '\0';
    begin += look_at;
  } while (fillwords.contains(QString(first_arg), Qt::CaseInsensitive));

  return (argument + begin);
}

constexpr auto MAX_HELP = 1100;

void DC::free_help_from_memory(void)
{
  if (!help_index)
  {
    return;
  }

  for (qint32 i = {}; i < MAX_HELP; i++)
    if (help_index[i].keyword)
      help_index[i].keyword = {};

  help_index = {};
  help_index = {};
}

help_index_t build_help_index(QTextStream &fl)
{
  qsizetype nr{};
  bool issorted{};
  qint32 i{};
  help_index_t list, mem;
  QString buf, tmp, scan;
  qint64 pos{};

  for (;;)
  {
    pos = fl.pos();
    buf = fl.readLine();
    scan = buf;
    for (;;)
    {
      /* extract the keywords */
      scan = one_word(scan, tmp);

      if (tmp.isEmpty())
        break;

      list[nr].keyword = tmp;
      list[nr++].pos = pos;
    }

    /* skip the text */
    do
      buf = fl.readLine();
    while (buf.startsWith('#'));

    if (buf.length() >= 2 && buf[1] == '~')
      break;
  }

  /* we might as well sort the stuff */
  do
  {
    issorted = 1;
    for (i = {}; i < nr; i++)
      if (str_cmp(list[i].keyword, list[i + 1].keyword) > 0)
      {
        mem = list[i];
        list[i] = list[i + 1];
        list[i + 1] = mem;
        issorted = {};
      }
  } while (!issorted);

  *num = nr;
  return (list);
}

constexpr auto PAGE_LENGTH = 22;
constexpr auto PAGE_WIDTH = 80;

/* Traverse down the string until the beginning of the next page has been
 * reached.  Return nullptr if this is the last page of the string.
 */
const QString next_page(const QString str)
{
  qint32 col = 1, line = 1, spec_code = false;
  qint32 chars = {};
  for (;; str++)
  {
    // If end of string, return {}.
    if (*str == '\0')
      return {};

    // Check for $ ANSI codes.  They have to be kept together
    // Might wanna put a && *(str+1) != '$' so that $'s are wrapped...
    else if (*str == '$')
    {
      if (*(str + 1) == '\0')
      { // this should never happen
        DC::getInstance()->logentry(u"String ended in $ in next_page"_s, ANGEL, DC::LogChannel::LOG_BUG);
        //*str = '\0'; // overwrite the $ so it doesn't mess up anything
        return {};
      }
      str++; // skip the $
             // This causes the next character to get skipped in the loop iteration
      chars += 7;
    }
    // Check for the begining of an ANSI color code block.
    else if (*str == '\x1B' && !spec_code)
      spec_code = true;

    // Check for the end of an ANSI color code block.
    else if (*str == 'm' && spec_code)
      spec_code = false;

    // If we're at the start of the next page, return this fact.
    // Note, this is done AFTER we check for ansi codes, so that we don't
    // beep color into the pager menu (hopefully)
    else if (line > PAGE_LENGTH)
      return str;
    else if (chars > 2048)
      return str;
    // Check for everything else.
    else if (!spec_code)
    {
      chars += 1;

      // Carriage return puts us in column one.
      if (*str == '\r')
        col = 1;
      // Newline puts us on the next line.
      else if (*str == '\n')
        line++;

      // We need to check here and see if we are over the page width,
      // and if so, compensate by going to the begining of the next line.
      else if (col++ > PAGE_WIDTH)
      {
        col = 1;
        line++;
      }
    }
  }
  return {};
}

// Function that returns the number of pages in the string.
qint32 count_pages(const QString str)
{
  qint32 pages;

  for (pages = 1; (str = next_page(str)); pages++)
    ;
  return pages;
}

/* This function assigns all the pointers for showstr_vector for the
 * page_string function, after showstr_vector has been allocated and
 * showstr_count set.
 */
void paginate_string(const QString str, class Connection *d)
{
  qint32 i;

  if (conn->showstr_count)
    *(conn->showstr_vector) = str;

  for (i = 1; i < conn->showstr_count && str; i++)
    str = conn->showstr_vector[i] = next_page(str);

  conn->showstr_page = {};
}

void page_string(class Connection *d, const QString str, qint32 keep_internal)
{
  if (!d || !(conn->character))
    return;

  if (!str || str.isEmpty())
  {
    conn->character->send("");
    return;
  }

  if (conn->character->isPlayer() && !isSet(conn->character->player->toggles, Player::PLR_PAGER))
  {
    page_string_dep(d, str, keep_internal);
    return;
  }

  QString print_me = str;
  QString tmp;
  size_t pagebreak;

  while (!print_me.empty())
  {
    pagebreak = print_me.find_first_of('\n', 3800); // find the first endline after 3800 chars

    if (QString::npos == pagebreak)
      pagebreak = print_me.size(); // if one doesn't exist (string < 3800) just set to max string length
    else if (print_me.at(pagebreak) == '\r')
      pagebreak++; // if its a \r, go 1 greater.

    tmp = print_me.substr(0, pagebreak);
    print_me = print_me.substr(pagebreak, QString::npos);

    // if they don't want things paginated
    send_to_char(tmp.c_str(), conn->character);
  }
}

/* The depreciated call that gets the paging ball rolling... */
void page_string_dep(class Connection *d, const QString str, qint32 keep_internal)
{
  if (!d)
    return;
  if (!str || str.isEmpty())
  {
    conn->character->send("");
    return;
  }

  CREATE(conn->showstr_vector, const QString, conn->showstr_count = count_pages(str));

  if (keep_internal)
  {
    conn->showstr_head = (str);
    paginate_string(conn->showstr_head, d);
  }
  else
    paginate_string(str, d);

  show_string(d, "");
}

/* The call that displays the next page. */
void show_string(class Connection *d, const QString input)
{
  QString buffer;
  QString buf;
  qint32 diff;

  one_argument(input, buf);

  if (LOWER(*buf) == 'r')
    conn->showstr_page = MAX(0, conn->showstr_page - 1);

  /* B is for back, so back up two pages internally so we can display the
   * correct page here.
   */
  else if (LOWER(*buf) == 'b')
    conn->showstr_page = MAX(0, conn->showstr_page - 2);

  /* Feature to 'goto' a page.  Just type the number of the page and you
   * are there!
   */
  else if (isdigit(*buf))
    conn->showstr_page = MAX(0, MIN(atoi(buf) - 1, conn->showstr_count - 1));

  else if (*buf)
  {
    conn->showstr_vector = {};
    conn->showstr_vector = {};
    conn->showstr_count = {};
    if (conn->showstr_head)
    {
      conn->showstr_head = {};
      conn->showstr_head = {};
    }
    return;
  }
  /* If we're displaying the last page, just send it to the character, and
   * then free up the space we used.
   */
  if (conn->showstr_page + 1 >= conn->showstr_count)
  {
    // send them a carriage return first to make sure it looks right
    send_to_char(conn->showstr_vector[conn->showstr_page], conn->character);
    conn->showstr_vector = {};
    conn->showstr_vector = {};
    conn->showstr_count = {};
    if (conn->showstr_head)
    {
      conn->showstr_head = {};
      conn->showstr_head = {};
    }
  }
  /* Or if we have more to show.... */
  else
  {
    dc_strncpy(buffer, conn->showstr_vector[conn->showstr_page], diff = (conn->showstr_vector[conn->showstr_page + 1]) - (conn->showstr_vector[conn->showstr_page]));
    buffer[diff] = '\0';
    conn->character->send(buffer);
    conn->showstr_page++;
  }
}
