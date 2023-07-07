// social.C
// Description:  Anything to do with socials

#include <string.h>
#include <stdlib.h> // qsort()

#include <QMap>

#include "fileinfo.h" // SOCIAL_FILE
#include "structs.h"  // MAX_INPUT_LENGTH
#include "room.h"
#include "character.h"
#include "utility.h"
#include "mobile.h"
#include "connect.h"
#include "levels.h"
#include "player.h"
#include "social.h"
#include "handler.h"
#include "act.h"
#include "db.h"
#include "interp.h" // len_cmp
#include "returnvals.h"

extern CWorld world;

QMap<QString, social_messg> socials = {};

bool find_social(QString arg, social_messg &social);

int check_social(Character *ch, QString social_name, int length)
{
  string arg = {}, buf = {};
  social_messg action = {};
  Character *vict = {};

  if (find_social(social_name, action) == false)
  {
    return SOCIAL_false;
  }

  if (IS_PC(ch) && IS_SET(ch->player->punish, PUNISH_NOEMOTE))
  {
    send_to_char("You are anti-social!\n\r", ch);
    return SOCIAL_true;
  }

  switch (GET_POS(ch))
  {
  case POSITION_DEAD:
    send_to_char("Lie still; you are DEAD.\r\n", ch);
    return SOCIAL_true;

  case POSITION_STUNNED:
    send_to_char("You are too stunned to do that.\r\n", ch);
    return SOCIAL_true;

  case POSITION_SLEEPING:
    send_to_char("In your dreams, or what?\n\r", ch);
    return SOCIAL_true;
  }

  if (IS_SET(world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return SOCIAL_true;
  }

  if (action.char_found.isEmpty() == false)
  {
    tie(buf, arg) = half_chop(arg);
  }
  else
  {
    buf = {};
  }

  if (buf.empty())
  {
    if (action.char_no_arg.isEmpty() == false)
    {
      act(action.char_no_arg, ch, 0, 0, TO_CHAR, 0);
    }

    if (action.others_no_arg.isEmpty() == false)
    {
      act(action.others_no_arg, ch, 0, 0, TO_ROOM, (action.hide) ? INVIS_NULL : 0);
    }
    return SOCIAL_true_WITH_NOISE;
  }

  if (!(vict = get_char_room_vis(ch, buf)))
  {
    if (action.not_found.isEmpty() == false)
    {
      act(action.not_found, ch, 0, 0, TO_CHAR, 0);
    }
  }
  else if (vict == ch)
  {
    if (action.char_auto.isEmpty() == false)
    {
      act(action.char_auto, ch, 0, 0, TO_CHAR, 0);
    }

    if (action.others_auto.isEmpty() == false)
    {
      act(action.others_auto, ch, 0, 0, TO_ROOM, (action.hide) ? INVIS_NULL : 0);
    }
  }
  else if (GET_POS(vict) < action.min_victim_position)
  {
    act("$N is not in a proper position for that.", ch, 0, vict, TO_CHAR, 0);
  }
  else
  {
    if (action.char_found.isEmpty() == false)
    {
      act(action.char_found, ch, 0, vict, TO_CHAR, 0);
    }

    if (action.others_found.isEmpty() == false)
    {
      act(action.others_found, ch, 0, vict, TO_ROOM, NOTVICT | ((action.hide) ? INVIS_NULL : 0));
    }

    if (action.vict_found.isEmpty() == false)
    {
      act(action.vict_found, ch, 0, vict, TO_VICT, (action.hide) ? INVIS_NULL : 0);
    }
  }

  return SOCIAL_true_WITH_NOISE;
}

char *fread_social_string(FILE *fl)
{
  char buf[MAX_STRING_LENGTH], *rslt;

  fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl))
  {
    logentry("Fread_social_string - unexpected EOF.", IMMORTAL, LogChannels::LOG_BUG);
    exit(0);
  }

  if (*buf == '#')
    return (0);

  // strip the \n
  *(buf + strlen(buf) - 1) = '\0';

  rslt = str_dup(buf);
  return (rslt);
}

QString fread_social_string(QTextStream &fl)
{
  QString buf = fl.readLine();
  if (buf.startsWith('#'))
  {
    return "";
  }

  if (fl.atEnd())
  {
    logentry("Fread_social_string - unexpected EOF.", IMMORTAL, LogChannels::LOG_BUG);
    return "";
  }

  return buf;
}

int read_social_from_file(QTextStream &fl)
{
  QString social_name = {};
  int hide = {}, min_pos = {};

  fl >> social_name;
  if (fl.atEnd() || social_name.startsWith('#'))
  {
    return false;
  }
  fl >> hide >> min_pos;
  fl.skipWhiteSpace();

  social_messg social;
  // read strings that will always be there
  social.name = social_name.toLower();
  social.hide = hide;
  social.min_victim_position = min_pos;
  social.char_no_arg = fread_social_string(fl);
  social.others_no_arg = fread_social_string(fl);
  social.char_found = fread_social_string(fl);

  // if char_found is not empty then continue reading
  if (social.char_found.isEmpty() == false)
  {
    social.others_found = fread_social_string(fl);
    social.vict_found = fread_social_string(fl);
    social.not_found = fread_social_string(fl);
    social.char_auto = fread_social_string(fl);
    social.others_auto = fread_social_string(fl);
  }

  if (socials.contains(social.name))
  {
    logf(100, LOG_BUG, QString("socials already contains '%1'").arg(social.name).toStdString().c_str());
  }
  else
  {
    socials[social.name] = social;
  }
  return true;
}

void boot_social_messages(void)
{
  QFile social_file(SOCIAL_FILE);

  if (!social_file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    perror("Can't open social file in boot_social_messages");
    abort();
  }
  QTextStream fl(&social_file);

  for (;;)
  {
    if (!read_social_from_file(fl))
    {
      break;
    }
  }
}

bool find_social(QString arg, social_messg &target_social)
{
  for (const social_messg &social : socials)
  {
    if (social.name.startsWith(arg, Qt::CaseInsensitive))
    {
      qDebug() << social.name << "vs" << arg;
      target_social = social;
      return true;
    };
  }

  return false;
}

void clean_socials_from_memory()
{
  socials = {};
}

int do_social(Character *ch, char *argument, int cmd)
{
  QString buf;

  uint64_t i = 0;

  for (const social_messg &social : socials)
  {
    buf += QString("%1").arg(social.name, 18);

    if (!(i++ % 4))
    {
      buf += "\r\n";
      ch->send(buf);
      buf = {};
    }
  }

  if (buf.isEmpty() == false)
  {
    ch->send(buf);
  }

  buf = QString("\r\nCurrent Socials:  %1\r\n").arg(socials.size());
  ch->send(buf);

  return eSUCCESS;
}
