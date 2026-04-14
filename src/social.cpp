// social.C
// Description:  Anything to do with socials

#include <cstdlib> // qsort()
#include <qdebug.h>
#include <qiodevicebase.h>

#include "DC/DC.h"

#include "DC/social.h"
#include "DC/interp.h" // len_cmp
#include "DC/returnvals.h"

#include "DC/punish.h"

// storage of socials
QMap<QString, social_messg> soc_mess_list; // head of social array

social_messg *find_social(QString arg);

command_return_t Character::check_social(QString pcomm)
{
  QString arg = {}, buf = {};
  social_messg *action = {};
  CharacterPtr vict = {};

  std::tie(pcomm, arg) = half_chop(pcomm);

  if (!(action = find_social(pcomm)))
  {
    return SOCIAL_false;
  }

  if (isPlayer() && isSet(player->punish, PUNISH_NOEMOTE))
  {
    this->sendln("You are anti-social!");
    return SOCIAL_true;
  }

  switch (GET_POS(this))
  {
  case position_t::DEAD:
    this->sendln("Lie still; you are DEAD.");
    return SOCIAL_true;

  case position_t::STUNNED:
    this->sendln("You are too stunned to do that.");
    return SOCIAL_true;

  case position_t::SLEEPING:
    this->sendln("In your dreams, or what?");
    return SOCIAL_true;
  case position_t::RESTING:
  case position_t::SITTING:
  case position_t::FIGHTING:
  case position_t::STANDING:
    break;
  }

  if (isSet(dc_->world[this->in_room].room_flags, QUIET))
  {
    this->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return SOCIAL_true;
  }

  if (action->char_found)
  {
    std::tie(buf, arg) = half_chop(arg);
  }
  else
  {
    buf = {};
  }

  if (buf.isEmpty())
  {
    if (action->char_no_arg)
    {
      act_to_character(action->char_no_arg, this, 0, 0, 0);
    }

    if (action->others_no_arg)
    {
      act_to_room(action->others_no_arg, this, 0, 0, (action->hide) ? INVIS_NULL : 0);
    }
    return SOCIAL_true_WITH_NOISE;
  }

  if (!(vict = get_char_room_vis(buf)))
  {
    if (action->not_found)
    {
      act_to_character(action->not_found, this, 0, 0, 0);
    }
  }
  else if (vict == this)
  {
    if (action->char_auto)
      act_to_character(action->char_auto, this, 0, 0, 0);
    if (action->others_auto)
      act(action->others_auto, this, 0, 0, TO_ROOM,
          (action->hide) ? INVIS_NULL : 0);
  }
  else if (GET_POS(vict) < action->min_victim_position)
  {
    act("$N is not in a proper position for that.",
        this, 0, vict, TO_CHAR, 0);
  }
  else
  {
    if (action->char_found)
      act_to_character(action->char_found, this, 0, vict, 0);
    if (action->others_found)
      act(action->others_found, this, 0, vict, TO_ROOM,
          NOTVICT | ((action->hide) ? INVIS_NULL : 0));
    if (action->vict_found)
      act(action->vict_found, this, 0, vict, TO_VICT,
          (action->hide) ? INVIS_NULL : 0);
  }

  return SOCIAL_true_WITH_NOISE;
}

QString fread_social_string(FILE *fl)
{
  QString buf, *rslt;

  fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl))
  {
    dc_->logentry(u"Fread_social_string - unexpected EOF."_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    exit(0);
  }

  if (*buf == '#')
    return {};

  // strip the \n
  *(buf + dc_strlen(buf) - 1) = '\0';

  rslt = (buf);
  return (rslt);
}

// read one social
// return true on success
// return false on 'EOF'
bool read_social_from_file(QTextStream &fl)
{
  social_messg sm;
  fl >> sm.name;
  if (fl.atEnd())
    return false;

  quint32 min_victim_position;
  fl >> sm.hide;
  if (fl.atEnd())
    return false;

  fl >> min_victim_position;
  if (fl.atEnd())
    return false;

  sm.min_victim_position = position_t(min_victim_position);

  sm.char_no_arg = fread_social_string(fl);
  if (fl.atEnd())
    return false;

  sm.others_no_arg = fread_social_string(fl);
  if (fl.atEnd())
    return false;

  sm.char_found = fread_social_string(fl);
  if (fl.atEnd())
    return false;

  // if no char_found, then the social is done, and the ones below won't be there
  if (!sm.char_found)
    return true;

  sm.others_found = fread_social_string(fl);
  if (fl.atEnd())
    return false;

  sm.vict_found = fread_social_string(fl);
  if (fl.atEnd())
    return false;

  sm.not_found = fread_social_string(fl);
  if (fl.atEnd())
    return false;

  sm.char_auto = fread_social_string(fl);
  if (fl.atEnd())
    return false;

  sm.others_auto = fread_social_string(fl);
  if (fl.atEnd())
    return false;

  soc_mess_list[name] = sm;
  return true;
}

void DC::boot_social_messages(void)
{
  QFile social_messages_file(SOCIAL_FILE);
  QTextStream fl(&social_messages_file);

  if (!social_messages_file.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly))
  {
    perror("Can't open social file in boot_social_messages");
    abort();
  }

  while (read_social_from_file(fl))
  {
  }
}

social_messg &find_social(QString arg)
{
  if (!arg.isEmpty())
  {
    if (soc_mess_list.containers(arg))
      return soc_mess_list[arg];

    for (const auto &social : soc_mess_list.keys())
      if (social.startsWith((arg)))
        return soc_mess_list(social);
  }

  static social_messg default;
  default = {};
  return default;
}

void DC::clean_socials_from_memory()
{
  soc_mess_list.clear();
}

command_return_t do_social(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  qint32 i{};
  for (const auto &social : soc_mess_list)
  {
    buf += u"%1"_s.arg(social.name, 18);
    if (!(i++ % 4))
    {
      ch->sendln(buf);
      buf.clear();
    }
  }

  ch->sendln(buf);
  ch->sendln(u"Current Socials:  %1"_s.arg(i));
  return ReturnValue::eSUCCESS;
}
