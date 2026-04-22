// social.C
// Description:  Anything to do with socials
#include "DC/DC.h"

// storage of socials
QMap<QString, social_messg> soc_mess_list; // head of social array

social_messg *find_social(QString arg);

ReturnValue Character::check_social(QString pcomm)
{
  QString arg = {}, buf = {};
  social_messg *action = {};
  CharacterPtr vict = {};

  std::tie(pcomm, arg) = half_chop(pcomm);

  if (!(action = find_social(pcomm)))
  {
    return SOCIAL_FALSE;
  }

  if (isPlayer() && isSet(player->punish, PUNISH_NOEMOTE))
  {
    sendln(u"You are anti-social!"_s);
    return SOCIAL_TRUE;
  }

  switch (GET_POS(this))
  {
  case position_t::DEAD:
    sendln(u"Lie still; you are DEAD."_s);
    return SOCIAL_TRUE;

  case position_t::STUNNED:
    sendln(u"You are too stunned to do that."_s);
    return SOCIAL_TRUE;

  case position_t::SLEEPING:
    sendln(u"In your dreams, or what?"_s);
    return SOCIAL_TRUE;
  case position_t::RESTING:
  case position_t::SITTING:
  case position_t::FIGHTING:
  case position_t::STANDING:
    break;
  }

  if (isSet(dc_->world[in_room].room_flags, QUIET))
  {
    sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return SOCIAL_TRUE;
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
    return SOCIAL_TRUE_WITH_NOISE;
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

  return SOCIAL_TRUE_WITH_NOISE;
}

QString fread_social_string(auto &stream)
{
  QString buf, *rslt;

  fgets(buf, MAX_STRING_LENGTH, stream);
  if (feof(stream))
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
bool read_social_from_file(QTextStream &stream)
{
  social_messg sm;
  stream >> sm.name;
  if (stream.atEnd())
    return false;

  quint32 min_victim_position;
  stream >> sm.hide;
  if (stream.atEnd())
    return false;

  stream >> min_victim_position;
  if (stream.atEnd())
    return false;

  sm.min_victim_position = position_t(min_victim_position);

  sm.char_no_arg = fread_social_string(stream);
  if (stream.atEnd())
    return false;

  sm.others_no_arg = fread_social_string(stream);
  if (stream.atEnd())
    return false;

  sm.char_found = fread_social_string(stream);
  if (stream.atEnd())
    return false;

  // if no char_found, then the social is done, and the ones below won't be there
  if (!sm.char_found)
    return true;

  sm.others_found = fread_social_string(stream);
  if (stream.atEnd())
    return false;

  sm.vict_found = fread_social_string(stream);
  if (stream.atEnd())
    return false;

  sm.not_found = fread_social_string(stream);
  if (stream.atEnd())
    return false;

  sm.char_auto = fread_social_string(stream);
  if (stream.atEnd())
    return false;

  sm.others_auto = fread_social_string(stream);
  if (stream.atEnd())
    return false;

  soc_mess_list[name] = sm;
  return true;
}

void DC::boot_social_messages(void)
{
  QFile social_messages_file(SOCIAL_FILE);
  QTextStream stream(&social_messages_file);

  if (!social_messages_file.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly))
  {
    perror("Can't open social file in boot_social_messages");
    abort();
  }

  while (read_social_from_file(stream))
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

ReturnValue do_social(CharacterPtr ch, QString argument, cmd_t cmd)
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
