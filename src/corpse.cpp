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
#include "DC/DC.h"
#include <qiodevicebase.h>

qint32 corpse_save(ObjectPtr obj, QTextStream stream, qint32 location, bool recurse_this_tree)
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
      corpse_save(obj->next_content, stream, location, recurse_this_tree);
    }

    recurse_this_tree = true;
    corpse_save(obj->contains, stream, MIN(0, location) - 1, recurse_this_tree);
    result = write_corpse_to_disk(stream, obj, location);

    /* readjust the wieght while we do this */
    //    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
    // No, let's not.      GET_OBJ_WEIGHT(tmp) += GET_OBJ_WEIGHT(obj);
    if (!result)
      return {};
  }
  return (true);
}

void DC::save_corpses(void)
{
  /* This is basically the mother of all the save corpse functions */
  /* You can call it from anywhere in the game with no arguments */
  /* Basically any time a corpse is manipulated in any way..either */
  /* directly or indirectly you need to call this function */

  QTextStream stream;
  ObjectPtr i, next;
  qint32 location = {};
  QString buf1;
  extern ReturnValues do_not_save_corpses;

  if (do_not_save_corpses == 1)
    return;

  /* Open corpse file */
  QFile file(CORPSE_FILE);
  if (!file.open(QIODeviceBase::Text | QIODeviceBase::WriteOnly))
  {
    QString buf1;
    dc_sprintf(buf1, "SYSERR: checking for corpse file %s : %s", CORPSE_FILE, strerror(errno));
    perror(qPrintable(buf1));
    return;
  }

  /* Scan the object list */
  for (i = dc_->object_list; i; i = next)
  {
    next = i->next;

    /* Check if its a players corpse */
    if (IS_OBJ_STAT(i, ITEM_PC_CORPSE) && i->contains)
    {
      /* It is, so save it to a file */
      if (!corpse_save(i, stream, location, false))
      {
        perror("SYSERR: A corpse didnt save for some reason");

        return;
      }
    }
  }
  /* Close the corpse file */
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

  QTextStream stream;
  QString line;
  QList<qint32> t, zwei = {};
  qint32 nr, num_objs = {};
  ObjectPtr temp = {}, obj = {}, next_obj = {};
  ExtraDescriptionPtr new_descr;
  QString buf1, buf2, buf3;
  bool end = false;
  qint32 number = -1;
  ObjectPtr money;
  qint32 debug = {};

  if (!(stream = fopen(CORPSE_FILE, "r")))
  {
    logverbose(u"Unable to open '%1"_s.arg(CORPSE_FILE));
    return;
  }

  if (!feof(stream))
  {
    get_line_new(stream, line);
  }
  else
    dc_->logentry(u"No corpses in file to load"_s, 0, DC::LogChannel::LOG_MISC);

  while (!feof(stream) && !end)
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
        dc_->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
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

      get_line_new(stream, line);
      if (debug == 1)
      {
        dc_sprintf(buf3, " -LINE: %s", line);
        dc_->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
      }
      sscanf(line, "%d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7);
      GET_OBJ_VAL(temp, 0) = t[1];
      GET_OBJ_VAL(temp, 1) = t[2];
      GET_OBJ_VAL(temp, 2) = t[3];
      GET_OBJ_VAL(temp, 3) = t[4];
      GET_OBJ_EXTRA(temp) = t[5];
      GET_OBJ_VROOM(temp) = t[6];
      GET_OBJ_TIMER(temp) = t[7];

      get_line_new(stream, line);
      if (debug == 1)
      {
        dc_sprintf(buf3, " -LINE: %s", line);
        dc_->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
      }
      /* read line check for xap. */
      if (!dc_strcmp("XAP\n", line))
      { /* then this is a Xap Obj, requires special care */
        if (debug == 1)
          dc_->logentry(u"XAP Found"_s, 0, DC::LogChannel::LOG_MISC);

        temp->name(fread_string_new(stream));
        if (temp->name().isEmpty())
        {
          temp->name(u"undefined"_s);
        }
        else
        {
          if (debug == 1)
          {
            logmisc(u"   -NAME: %1"_s.arg(temp->name()));
          }
        }

        auto buffer = fread_string_new(stream);
        if (temp->short_description(fread_string_new(stream)).isEmpty())
        {
          temp->short_description("undefined");
        }
        else
        {
          if (debug == 1)
          {
            dc_sprintf(buf3, "   -SHORT: %s\n", qPrintable(temp->short_description()));
            dc_->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
          }
        }

        if (temp->long_description(fread_string_new(stream)).isEmpty())
        {
          temp->long_description("undefined");
        }
        else
        {
          if (debug == 1)
          {
            dc_sprintf(buf3, "   -DESC: %s\n", qPrintable(temp->long_description()));
            dc_->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
          }
        }

        temp->ActionDescription(fread_string_new(stream));
        if (temp->ActionDescription().isEmpty())
        {
          temp->ActionDescription("undefined");
        }
        else
        {
          if (debug == 1)
          {
            dc_snprintf(buf3, sizeof(buf3) - 1, "   -ACT_DESC: %s\n", qPrintable(temp->ActionDescription()));
            dc_->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
          }
        }
        if (!get_line_new(stream, line) ||
            (sscanf(line, "%d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4) != 5))
        {
          dc_->logentry(u"load_corpses: Format error in first numeric line (expecting 5 args)"_s, 0, DC::LogChannel::LOG_MISC);
        }
        else
        {
          if (debug == 1)
          {
            dc_sprintf(buf3, "   -FLAGS: %s", line);
            dc_->logentry(buf3, 0, DC::LogChannel::LOG_MISC);
          }
        }
        temp->flags_.type_flag = t[0];
        temp->flags_.wear_flags = ObjectPositions::fromInt(t[1]);
        temp->flags_.weight = (t[2] > 0 ? t[2] : 0);
        temp->flags_.cost = t[3];
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

        get_line_new(stream, line);
        for (zwei = {}; !zwei && !feof(stream);)
        {
          switch (*line)
          {
          case 'E':
            new_descr = new ExtraDescription;
            new_descr->keyword_ = fread_string_new(stream);
            new_descr->description_ = fread_string_new(stream);
            new_descr->next = temp->ex_description;
            temp->ex_description = new_descr;
            get_line_new(stream, line);
            break;
          case 'A':
            get_line_new(stream, line);
            sscanf(line, "%d %d", t, t + 1);

            temp->affected[temp->num_affects].location = t[0];
            temp->affected[temp->num_affects].modifier = t[1];
            temp->num_affects++;
            get_line_new(stream, line);
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
          dc_->logf(0, DC::LogChannel::LOG_BUG, "alloc_num_affects: %d != temp->num_affects: %d",
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
          dc_->logentry(u"XAP NOT Found"_s, 0, DC::LogChannel::LOG_MISC);
      }
      if (temp != nullptr)
      {
        num_objs++;
        /* Check if our object is a corpse */
        if (IS_OBJ_STAT(temp, ITEM_PC_CORPSE))
        {
          /* scan our temp room for objects */
          for (obj = dc_->world[real_room(frozen_start_room)].contents; obj; obj = next_obj)
          {
            next_obj = obj->next_content;
            if (obj)
            {
              if (debug == 1)
              {
                logmisc(u"  -Moving [%1] to [%2]"_s.arg(obj->name()).arg(temp->name()));
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
              logmisc(u"  -Moving corpse [%1] to [%2]"_s.arg(temp->name()).arg(GET_OBJ_VROOM(temp)));
            }
            obj_to_room(temp, real_room(GET_OBJ_VROOM(temp)));
          }
        }
        else
        {
          /* just a plain obj..send it to a temp room until we load a corpse */
          if (debug == 1)
          {
            logmisc(u"  -Moving corpse [%1] to holding room."_s.arg(temp->name()));
          }
          obj_to_room(temp, real_room(frozen_start_room));
        }
      }
    }
  }
}

qint32 get_line_new(auto &stream, QString buf)
{
  QString temp;
  qint32 lines = 0, a = {};

  while (!feof(stream))
  {
    switch ((temp[a++] = fgetc(stream)))
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
    fgets(temp, 256, stream);
    if (!temp.isEmpty())
      temp[dc_strlen(temp) - 1] = '\0';
  } while (!feof(stream) && (*temp == '*' || temp.isEmpty()));

  if (feof(stream))
    return 0;
  else
  {
    dc_strcpy(buf, temp);
    return lines;
  }
}

ObjectPtr DC::create_obj_new(void)
{
  auto obj = ObjectPtr(new Object(this));
  object_list_.push_back(obj);
  /* Corpse saving stuff */
  GET_OBJ_VROOM(obj) = INVALID_ROOM;
  GET_OBJ_TIMER(obj) = {};
  obj->save_expiration = {};
  obj->no_sell_expiration = {};
  return obj;
}
