/***********************************************************************
 *  File: corpse.c Version 1.1                                          *
 *  Usage: Corpse Saving over reboots/crashes/copyovers                 *
 *                                                                      *
 *  By: Michael Cunningham (Romulus) IMPLEMENTER of Legends of The Phoenix MUD  *
 *  Permission is granted to use and modify this software as long as    *
 *  credits are given in the credits command in the game.               *
 *  Built for circle30bpl15                                             *
 *  Bunch of code reused from the XAP object patch by Patrick Dughi     *
 *                                                  <dughi@IMAXX.NET>   *
 *  Thankyou Patrick!                                                   *
 *  Some functions have been renamed to protect the innocent            *
 *  All Rights Reserved, Copyright (C) 1999                             *
 ***********************************************************************/

/* The standard includes */
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include "DC/DC.h"

#include "DC/db.h"
#include <cassert>
#include <cstddef>
#include <qcontainerfwd.h>

/* Set this define to wherever you want to save your corpses */
const auto CORPSE_FILE = QStringLiteral("corpse.save");

/* External Structures / Variables */

ObjectPtr obj_proto;
/* index table for object file   */
qint16 frozen_start_room = 1;

/* Local Function Declerations */
qint32 count_hash_records(FILE *fl);
ObjectPtr create_obj_new(void);
void save_corpses(void);
qint32 corpse_save(ObjectPtr obj, FILE *fp, qint32 location, bool recurse_this_tree);
qint32 write_corpse_to_disk(FILE *fp, ObjectPtr obj, qint32 locate);
void clean_string(QString buffer);
qint32 get_line_new(FILE *fl, QString buf);
QString fread_string_new(FILE *fl);

/* Tada! THE FUNCTIONS ! Yaaa! */

void clean_string(QString buffer)
{
  QString ptr, *str;

  ptr = buffer;
  str = ptr;

  while ((*str = *ptr))
  {
    str++;
    ptr++;
    if (*ptr == '\r')
      ptr++;
  }
}

qint32 corpse_save(ObjectPtr obj, FILE *fp, qint32 location, bool recurse_this_tree)
{
  /* This function basically is responsible for taking the    */
  /* supplied obj and figuring out if it has any contents. If */
  /* it does then we write those to disk.. Ad Nasum.          */

  // ObjectPtr tmp;
  qint32 result;

  if (obj)
  { /* a little recursion (can be a dangerous thing:) */

    /* recurse_this_tree causes the recursion to branch only
     down the corpses content's tree and not the contents of the
     room. obj->next_content points to the rooms contents
     the first time this function is called from save_corpses
     hence we avoid going down there otherwise we will save
     the rooms contents as well as the corpses contents in the
     corpse.save file.
     */

    if (recurse_this_tree != false)
    {
      corpse_save(obj->next_content, fp, location, recurse_this_tree);
    }

    recurse_this_tree = true;
    corpse_save(obj->contains, fp, MIN(0, location) - 1, recurse_this_tree);
    result = write_corpse_to_disk(fp, obj, location);

    /* readjust the wieght while we do this */
    //    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
    // No, let's not.      GET_OBJ_WEIGHT(tmp) += GET_OBJ_WEIGHT(obj);
    if (!result)
      return {};
  }
  return (true);
}

qint32 write_corpse_to_disk(FILE *fp, ObjectPtr obj, qint32 locate)
{
  /* This is basically Patrick's my_obj_save_to_disk function with    */
  /* a few minor tweaks to make it work for corpses. Basically it     */
  /* writes one object out to the corpse file every time it is called.*/
  /* It can handle regular obj's and XAP objects.                     */

  qint32 counter;
  extra_descr_data *ex_desc;
  QString buf1 = {};
  // QString buf2;

  if (!obj->ActionDescription().isEmpty())
  {
    dc_strncpy(buf1, qPrintable(obj->ActionDescription()), sizeof(buf1) - 1);
    clean_string(buf1);
  }
  else
    *buf1 = {};
  dc_fprintf(fp,
             "#%lu\n"
             "%d %d %d %d %d %u %d %d\n",
             GET_OBJ_VNUM(obj),
             locate,
             GET_OBJ_VAL(obj, 0),
             GET_OBJ_VAL(obj, 1),
             GET_OBJ_VAL(obj, 2),
             GET_OBJ_VAL(obj, 3),
             GET_OBJ_EXTRA(obj),
             GET_OBJ_VROOM(obj),  /*vroom is the virtual room a corpse*/
             GET_OBJ_TIMER(obj)); /* was created in. See make_corpse */

  if (!(IS_OBJ_STAT(obj, ITEM_UNIQUE_SAVE)))
  {
    return 1;
  }
  dc_fprintf(fp,
             "XAP\n"
             "%s~\n"
             "%s~\n"
             "%s~\n"
             "%s~\n"
             "%d %d %d %d %d\n",
             !obj->name().isEmpty() ? qPrintable(obj->name()) : "undefined",
             qPrintable(obj->short_description()) ? qPrintable(obj->short_description()) : "undefined",
             !obj->long_description().isEmpty() ? qPrintable(obj->long_description()) : "undefined",
             qPrintable(buf1),
             GET_OBJ_TYPE(obj),
             GET_OBJ_WEAR(obj).toInt(),
             (GET_OBJ_WEIGHT(obj) < 0 ? 0 : GET_OBJ_WEIGHT(obj)),
             GET_OBJ_COST(obj), obj->num_affects);
  /* Do we have affects? */
  for (counter = {}; counter < obj->num_affects; counter++)
    if (obj->affected[counter].modifier)
      dc_fprintf(fp, "A\n"
                     "%d %d\n",
                 obj->affected[counter].location,
                 obj->affected[counter].modifier);

  /* Do we have extra descriptions? */
  if (obj->ex_description)
  { /*. Yep, save them too . */
    for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next)
    {
      /*. Sanity check to prevent nasty protection faults . */
      if (ex_desc->keyword_.isEmpty() || ex_desc->description_.isEmpty())
      {
        continue;
      }
      dc_strcpy(buf1, ex_desc->description_);
      clean_string(buf1);
      dc_fprintf(fp, "E\n"
                     "%s~\n"
                     "%s~\n",
                 qPrintable(ex_desc->keyword_),
                 qPrintable(buf1));
    }
  }
  return 1;
}

void save_corpses(void)
{
  /* This is basically the mother of all the save corpse functions */
  /* You can call it from anywhere in the game with no arguments */
  /* Basically any time a corpse is manipulated in any way..either */
  /* directly or indirectly you need to call this function */

  FILE *fp;
  ObjectPtr i, next;
  qint32 location = {};
  QString buf1 = {0};
  extern command_return_t do_not_save_corpses;

  if (do_not_save_corpses == 1)
    return;

  /* Open corpse file */
  if (!(fp = fopen(CORPSE_FILE, "w")))
  {
    if (errno != ENOENT) /* if it fails, NOT because of no file */
      dc_sprintf(buf1, "SYSERR: checking for corpse file %s : %s", CORPSE_FILE, strerror(errno));
    perror(buf1);
    return;
  }

  /* Scan the object list */
  for (i = DC::getInstance()->object_list; i; i = next)
  {
    next = i->next;

    /* Check if its a players corpse */
    if (IS_OBJ_STAT(i, ITEM_PC_CORPSE) && i->contains)
    {
      /* It is, so save it to a file */
      if (!corpse_save(i, fp, location, false))
      {
        perror("SYSERR: A corpse didnt save for some reason");
        fclose(fp);
        return;
      }
    }
  }
  /* Close the corpse file */
  fclose(fp);
}

void DC::load_corpses(void)
{
  /* Ahh load corpses.. it was cake to write them out to a file      */
  /* it was a pain to load them back up through without a character. */
  /* Because I dont have a character I couldnt figure out how to     */
  /* put objects back into the corpse the exact way they came out..  */
  /* like objects back in their container, etc. So I just decided to */
  /* Dump it all in the corpse and let the character sort it out.    */
  /* If they dont like it, screwum. They are lucky I coded this:)    */
  /* Oh, and a bunch of this code is from Patricks XAP obj's code    */

  FILE *fp;
  QString line = {0};
  qint32 t[15], zwei = {};
  qint32 nr, num_objs = {};
  ObjectPtr temp = {}, *obj = {}, next_obj = {};
  extra_descr_data *new_descr;
  QString buf1 = {0}, buf2[256] = {0}, buf3[256] = {0};
  bool end = false;
  qint32 number = -1;
  ObjectPtr money;
  qint32 debug = {};

  if (!(fp = fopen(CORPSE_FILE, "r")))
  {
    logverbose(QStringLiteral("Unable to open '%1").arg(CORPSE_FILE));
    return;
  }

  if (!feof(fp))
  {
    get_line_new(fp, line);
  }
  else
    DC::getInstance()->logentry(QStringLiteral("No corpses in file to load"), 0, DC::LogChannel::LOG_MISC);

  while (!feof(fp) && !end)
  {
    temp = {};
    /* first, we get the number. Not too hard. */
    if (*line == '|')
      break;
    else if (*line == '#')
    {
      if (sscanf(line, "#%d", &nr) != 1)
      {
        continue;
      }
      if (debug == 1)
      {
        dc_sprintf(buf3, " -Loading Object: %d", nr);
        DC::getInstance()->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
      }
      /* we have the number, check it, load obj. */
      if (nr == -1)
      { /* then it is unique */
        temp = create_obj_new();
        temp->item_number = nr;
      }
      else if (nr < 0)
      {
        continue;
      }
      else
      {
        if (nr >= 999999)
          continue;

        if ((number = real_object(nr)) < 0)
          continue;
        temp = clone_object(number);
        if (!temp)
        {
          continue;
        }
      }

      get_line_new(fp, line);
      if (debug == 1)
      {
        dc_sprintf(buf3, " -LINE: %s", line);
        DC::getInstance()->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
      }
      sscanf(line, "%d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7);
      GET_OBJ_VAL(temp, 0) = t[1];
      GET_OBJ_VAL(temp, 1) = t[2];
      GET_OBJ_VAL(temp, 2) = t[3];
      GET_OBJ_VAL(temp, 3) = t[4];
      GET_OBJ_EXTRA(temp) = t[5];
      GET_OBJ_VROOM(temp) = t[6];
      GET_OBJ_TIMER(temp) = t[7];

      get_line_new(fp, line);
      if (debug == 1)
      {
        dc_sprintf(buf3, " -LINE: %s", line);
        DC::getInstance()->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
      }
      /* read line check for xap. */
      if (!dc_strcmp("XAP\n", line))
      { /* then this is a Xap Obj, requires special care */
        if (debug == 1)
          DC::getInstance()->logentry(QStringLiteral("XAP Found"), 0, DC::LogChannel::LOG_MISC);

        temp->name(fread_string_new(fp));
        if (temp->name().isEmpty())
        {
          temp->name(QStringLiteral("undefined"));
        }
        else
        {
          if (debug == 1)
          {
            logmisc(QStringLiteral("   -NAME: %1").arg(temp->name()));
          }
        }

        auto buffer = fread_string_new(fp);
        if (temp->short_description(fread_string_new(fp)).isEmpty())
        {
          temp->short_description("undefined");
        }
        else
        {
          if (debug == 1)
          {
            dc_sprintf(buf3, "   -SHORT: %s\n", qPrintable(temp->short_description()));
            DC::getInstance()->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
          }
        }

        if (temp->long_description(fread_string_new(fp)).isEmpty())
        {
          temp->long_description("undefined");
        }
        else
        {
          if (debug == 1)
          {
            dc_sprintf(buf3, "   -DESC: %s\n", qPrintable(temp->long_description()));
            DC::getInstance()->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
          }
        }

        temp->ActionDescription(fread_string_new(fp));
        if (temp->ActionDescription().isEmpty())
        {
          temp->ActionDescription("undefined");
        }
        else
        {
          if (debug == 1)
          {
            dc_snprintf(buf3, sizeof(buf3) - 1, "   -ACT_DESC: %s\n", qPrintable(temp->ActionDescription()));
            DC::getInstance()->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
          }
        }
        if (!get_line_new(fp, line) ||
            (sscanf(line, "%d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4) != 5))
        {
          DC::getInstance()->logentry(QStringLiteral("load_corpses: Format error in first numeric line (expecting 5 args)"), 0, DC::LogChannel::LOG_MISC);
        }
        else
        {
          if (debug == 1)
          {
            dc_sprintf(buf3, "   -FLAGS: %s", line);
            DC::getInstance()->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
          }
        }
        temp->obj_flags.type_flag = t[0];
        temp->obj_flags.wear_flags = ObjectPositions::fromInt(t[1]);
        temp->obj_flags.weight = (t[2] > 0 ? t[2] : 0);
        temp->obj_flags.cost = t[3];
        size_t alloc_num_affects = std::max(0, t[4]);

        /* buf2 is error codes pretty much */
        dc_sprintf(buf2, ", after numeric constants (expecting E/#xxx)");

        /* we're clearing these for good luck */

        temp->num_affects = {}; // Cleared, No memory has previously
                                // been assigned to 'em

        /* You have to null out the extradescs when you're parsing a xap_obj.
         This is done right before the extradescs are read. */

        if (temp->ex_description)
        {
          temp->ex_description = {};
        }

        get_line_new(fp, line);
        for (zwei = {}; !zwei && !feof(fp);)
        {
          switch (*line)
          {
          case 'E':
            new_descr = new extra_descr_data;
            new_descr->keyword_ = fread_string_new(fp);
            new_descr->description_ = fread_string_new(fp);
            new_descr->next = temp->ex_description;
            temp->ex_description = new_descr;
            get_line_new(fp, line);
            break;
          case 'A':
            get_line_new(fp, line);
            sscanf(line, "%d %d", t, t + 1);

            temp->affected[temp->num_affects].location = t[0];
            temp->affected[temp->num_affects].modifier = t[1];
            temp->num_affects++;
            get_line_new(fp, line);
            break;

          case '|':
            zwei = 1;
            end = true;
            break;
          case '$':
          case '#':
            zwei = 1;
            break;
          default:
            zwei = 1;
            break;
          }
        } /* exit our for loop */
        if (alloc_num_affects != temp->num_affects)
        {
          DC::getInstance()->logf(0, DC::LogChannel::LOG_BUG, "alloc_num_affects: %d != temp->num_affects: %d",
                                  alloc_num_affects, temp->num_affects);
        }
      }
      else
      { /* exit our xap loop */
        if (nr == -1)
        {
          if (debug == 1)
            dc_sprintf(buf3, "GOLD FOUND: %d total", t[1]);
          money = create_money(t[1]);
          obj_to_room(money, real_room(frozen_start_room));
          continue;
        }
        if (debug == 1)
          DC::getInstance()->logentry(QStringLiteral("XAP NOT Found"), 0, DC::LogChannel::LOG_MISC);
      }
      if (temp != nullptr)
      {
        num_objs++;
        /* Check if our object is a corpse */
        if (IS_OBJ_STAT(temp, ITEM_PC_CORPSE))
        {
          /* scan our temp room for objects */
          for (obj = DC::getInstance()->world[real_room(frozen_start_room)].contents; obj; obj = next_obj)
          {
            next_obj = obj->next_content;
            if (obj)
            {
              if (debug == 1)
              {
                logmisc(QStringLiteral("  -Moving [%1] to [%2]").arg(obj->name()).arg(temp->name()));
              }
              obj_from_room(obj);    /* get those objs from that room */
              obj_to_obj(obj, temp); /* and put them in the corpse */
            }
          } /* exit the room scanning loop */
          if (temp)
          {
            /* put the corpse in the right room */
            if (debug == 1)
            {
              logmisc(QStringLiteral("  -Moving corpse [%1] to [%2]").arg(temp->name()).arg(GET_OBJ_VROOM(temp)));
            }
            obj_to_room(temp, real_room(GET_OBJ_VROOM(temp)));
          }
        }
        else
        {
          /* just a plain obj..send it to a temp room until we load a corpse */
          if (debug == 1)
          {
            logmisc(QStringLiteral("  -Moving corpse [%1] to holding room.").arg(temp->name()));
          }
          obj_to_room(temp, real_room(frozen_start_room));
        }
      }
    }
  }
  fclose(fp);
}

qint32 get_line_new(FILE *fl, QString buf)
{
  QString temp = {0};
  qint32 lines = 0, a = {};

  while (!feof(fl))
  {
    switch ((temp[a++] = fgetc(fl)))
    {
    case (QChar)EOF:
      return 0;
    case '|':
    case '\n':
    case '\0':
      if (a < 1)
        return 0;
      dc_strcpy(buf, temp);
      buf[a] = '\0';
      return 1;
      break;
    }
  }
  if (a < 1)
    return 0;
  dc_strcpy(buf, temp);
  buf[a] = '\0';
  return 1;
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
    dc_strcpy(buf, temp);
    return lines;
  }
}

ObjectPtr create_obj_new(void)
{
  ObjectPtr obj = new Object;
  clear_object(obj);
  obj->next = DC::getInstance()->object_list;
  DC::getInstance()->object_list = obj;
  /* Corpse saving stuff */
  GET_OBJ_VROOM(obj) = DC::NOWHERE;
  GET_OBJ_TIMER(obj) = {};
  obj->save_expiration = {};
  obj->no_sell_expiration = {};
  return obj;
}

QString fread_string_new(FILE *fl)
{
  QFile file;
  if (!file.open(fl, QIODeviceBase::ReadOnly))
    return {};

  QByteArray return_buffer;
  while (file.canReadLine())
  {
    auto buffer = file.readLine();
    auto index_of_tilde = buffer.indexOf("~");
    if (index_of_tilde == -1)
    {
      return_buffer += buffer.trimmed().append("\r\n");
    }
    else
    {
      buffer.resize(index_of_tilde);
      return_buffer += buffer.trimmed();
      break;
    }
  }
  return return_buffer;
}

qint32 count_hash_records(FILE *fl)
{
  QString buf;
  qint32 count = {};

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return count;
}
