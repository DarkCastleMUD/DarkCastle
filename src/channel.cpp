/************************************************************************
| $Id: channel.cpp,v 1.28 2013/12/30 22:46:43 jhhudso Exp $
| channel.C
| Description:  All of the channel - type commands; do_say, gossip, etc..
*/
#include "DC/DC.h"

QQueue<ChannelMessagePtr> gossip_history;
QQueue<QString> auction_history;
QQueue<QString> newbie_history;
QQueue<QString> trivia_history;

ReturnValues do_say(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 i;
  QString buf;
  ReturnValues retval;
  extern bool MOBtrigger;

  if (!ch->isNonPlayer() && isSet(ch->player->punish, PUNISH_STUPID))
  {
    ch->sendln(u"You try to speak but just look like an idiot!"_s);
    return ReturnValue::eSUCCESS;
  }

  if (isSet(ch->ch->dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eSUCCESS;
  }

  ObjectPtr tmp_obj;
  for (tmp_obj = ch->ch->dc_->world[ch->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (ch->ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  argument = ltrim(argument);

  if (argument.isEmpty())
    ch->sendln(u"Yes, but WHAT do you want to say?"_s);
  else
  {
    if (ch->isPlayer())
      MOBtrigger = false;

    if (IS_IMMORTAL(ch))
    {
      argument = remove_all_codes(argument);
    }

    act_to_room(u"$B$7$n says '{}$B$7'$R"_s.arg(argument), ch, {}, CharacterPtr{}, {});

    if (ch->isPlayer())
      MOBtrigger = false;

    buf = fmt::format("$B$7You say '{}$B$7'$R", qPrintable(argument));
    act_to_character(buf, ch, 0, 0, 0);

    if (ch->isPlayer())
    {
      MOBtrigger = true;
      retval = ch->mprog_speech_trigger(qPrintable(argument));
      if (SOMEONE_DIED(retval))
        return SWAP_CH_VICT(retval);
    }

    if (ch->isPlayer())
    {
      MOBtrigger = true;
      retval = ch->oprog_speech_trigger(qPrintable(argument));
      if (SOMEONE_DIED(retval))
        return SWAP_CH_VICT(retval);
    }
  }
  return ReturnValue::eSUCCESS;
}

// Psay works like 'say', just it's directed at a person
// TODO - after this gets used alot, maybe switch speech triggers to it
ReturnValues do_psay(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString vict = {}, message = {}, buf = {};
  CharacterPtr victim = {};
  extern bool MOBtrigger;

  if (ch->isPlayer() && isSet(ch->player->punish, PUNISH_STUPID))
  {
    ch->sendln(u"You try to speak but just look like an idiot!"_s);
    return ReturnValue::eSUCCESS;
  }

  if (isSet(ch->ch->dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eSUCCESS;
  }

  ObjectPtr tmp_obj = {};
  for (tmp_obj = ch->ch->dc_->world[ch->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (tmp_obj && tmp_obj->item_number >= 0 && ch->ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  std::tie(vict, message) = half_chop(argument);

  if (vict.isEmpty() || message.isEmpty())
  {
    ch->sendln(u"Say what to whom?  psay <target> <message>"_s);
    return ReturnValue::eSUCCESS;
  }

  if (!(victim = ch->get_char_room_vis(qPrintable(vict))))
  {
    ch->send(u"You see noone that goes by '%1' here.\r\n"_s.arg(qPrintable(vict)));
    return ReturnValue::eSUCCESS;
  }

  QString messageStr = message;
  if (IS_IMMORTAL(ch))
  {
    messageStr = remove_all_codes(messageStr);
  }

  if (ch->isPlayer())
    MOBtrigger = false;
  buf = fmt::format("$B$n says (to $N) '{}'$R", qPrintable(messageStr));
  act_to_room(buf, ch, 0, victim, NOTVICT);

  if (ch->isPlayer())
    MOBtrigger = false;
  buf = fmt::format("$B$n says (to $3you$7) '{}'$R", qPrintable(messageStr));
  act_to_victim(buf, ch, 0, victim, 0);

  if (ch->isPlayer())
    MOBtrigger = false;
  buf = fmt::format("$BYou say (to $N) '{}'$R", qPrintable(messageStr));
  act_to_character(buf, ch, 0, victim, 0);
  MOBtrigger = true;
  //   if(ch->isPlayer()) {
  //     retval = mprog_speech_trigger( message, ch );
  //     MOBtrigger = true;
  //     if(SOMEONE_DIED(retval))
  //       return SWAP_CH_VICT(retval);
  //   }

  return ReturnValue::eSUCCESS;
}

ReturnValues do_pray(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString buf1;
  ConnectionPtr i;

  if (ch->isNonPlayer())
    return ReturnValue::eSUCCESS;

  while (*arg == ' ')
    arg++;

  if (arg.isEmpty())
  {
    ch->sendln(u"You must have something to tell the immortals..."_s);
    return ReturnValue::eSUCCESS;
  }

  if (ch->isImmortalPlayer())
  {
    ch->sendln(u"Why pray? You are a god!"_s);
    return ReturnValue::eSUCCESS;
  }

  if (!ch->isNonPlayer() && isSet(ch->player->punish, PUNISH_STUPID))
  {
    ch->sendln(u"Duh...I'm too stupid!"_s);
    return ReturnValue::eSUCCESS;
  }

  if (!ch->isNonPlayer() && isSet(ch->player->punish, PUNISH_NOPRAY))
  {
    ch->sendln(u"The gods are deaf to your prayers."_s);
    return ReturnValue::eSUCCESS;
  }

  dc_sprintf(buf1, "\a$4$B**$R$5 %s prays: %s $4$B**$R\r\n", qPrintable(ch->name()), arg);

  for (auto &conn : ch->ch->dc_->connections_)
  {
    if ((conn->character == nullptr) || (conn->character->getLevel() <= MORTAL))
      continue;
    if (!(isSet(conn->character->misc, DC::LogChannel::LOG_PRAYER)))
      continue;
    if (is_busy(conn->character) || is_ignoring(conn->character, ch))
      continue;
    if (!conn->connected)
      send_to_char(buf1, conn->character);
  }
  ch->sendln(u"\a\aOk."_s);
  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_gossip(CharacterPtr ch, const QString argument, cmd_t cmd)
{
  QString buf2;
  ConnectionPtr i;
  ObjectPtr tmp_obj;
  bool silence = false;

  if (isSet(ch->ch->dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eSUCCESS;
  }

  for (tmp_obj = ch->ch->dc_->world[ch->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (ch->ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  if (ch->isNonPlayer() && ch->master)
  {
    do_say(ch, "Why don't you just do that yourself!");
    return ReturnValue::eSUCCESS;
  }

  if (GET_POS(ch) == position_t::SLEEPING)
  {
    ch->sendln(u"You're asleep.  Dream or something...."_s);
    return ReturnValue::eSUCCESS;
  }

  if (ch->isPlayer())
  {
    if (!ch->isNonPlayer() && isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\r\n",
                   ch);
      return ReturnValue::eSUCCESS;
    }
    if (!(isSet(ch->misc, DC::LogChannel::CHANNEL_GOSSIP)))
    {
      ch->sendln(u"You told yourself not to GOSSIP!!"_s);
      return ReturnValue::eSUCCESS;
    }
    if (ch->getLevel() < 3)
    {
      ch->sendln(u"You must be at least 3rd level to gossip."_s);
      return ReturnValue::eSUCCESS;
    }
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
  {
    QQueue<ChannelMessagePtr> msgs = gossip_history;
    if (msgs.isEmpty())
    {
      ch->sendln(u"There have not been any player gossips."_s);
      return ReturnValue::eSUCCESS;
    }
    else if (msgs.count() == 1)
      ch->sendln(u"Here is the only gossip so far:"_s);
    else
      ch->sendln(u"Here are the last %1 gossips:"_s.arg(msgs.size()));

    while (!msgs.isEmpty())
      act_to_victim(msgs.dequeue().getMessage(ch), ch, 0, ch, 0);
  }
  else
  {
    ch->decrementMove(5);

    ChannelMessage msg(ch, DC::LogChannel::CHANNEL_GOSSIP, argument);

    dc_sprintf(buf2, "$5$BYou gossip '%s'$R", argument);
    act_to_character(buf2, ch, 0, 0, 0);

    if (ch->isPlayer())
    {
      gossip_history.enqueue(msg);
      if (gossip_history.size() > 1000)
      {
        gossip_history.dequeue();
      }
    }

    for (auto &i : ch->ch->dc_->connections_)
    {
      if (conn->character != ch && !conn->connected && (isSet(conn->character->misc, DC::LogChannel::CHANNEL_GOSSIP)) && !is_ignoring(conn->character, ch))
      {
        for (tmp_obj = ch->ch->dc_->world[conn->character->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
        {
          if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
          {
            silence = true;
            break;
          }
        }

        if (!silence)
        {
          act_to_victim(msg.getMessage(conn->character->getLevel()), ch, 0, conn->character, 0);
        }
      }
    }
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_shout(CharacterPtr ch, const QString argument, cmd_t cmd)
{
  QString buf1;
  QString buf2;
  ConnectionPtr i;
  bool silence = false;

  if (isSet(ch->dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eSUCCESS;
  }

  for (const auto &tmp_obj : ch->dc_->world[ch->in_room]->contents_)
    if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  if (ch->isNonPlayer() && ch->master)
  {
    return do_say(ch, "Shouting makes my throat hoarse.");
  }

  if (ch->isPlayer() && isSet(ch->player->punish, PUNISH_SILENCED))
  {
    send_to_char("You must have somehow offended the gods, for you "
                 "find yourself unable to!\r\n",
                 ch);
    return ReturnValue::eSUCCESS;
  }
  if (ch->isPlayer() && !(isSet(ch->misc, DC::LogChannel::CHANNEL_SHOUT)))
  {
    ch->sendln(u"You told yourself not to SHOUT!!"_s);
    return ReturnValue::eSUCCESS;
  }
  if (ch->isPlayer() && ch->getLevel() < 3)
  {
    send_to_char("Due to misuse, you must be of at least 3rd level "
                 "to shout.\r\n",
                 ch);
    return ReturnValue::eSUCCESS;
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
    ch->sendln(u"What do you want to shout, dork?"_s);

  else
  {
    dc_sprintf(buf1, "$B$n shouts '%s'$R", argument);
    dc_sprintf(buf2, "$BYou shout '%s'$R", argument);
    act_to_character(buf2, ch, 0, 0, 0);

    for (auto &i : ch->dc_->connections_)
      if (conn->character != ch && !conn->connected &&
          (ch->dc_->world[conn->character->in_room]->zone == ch->dc_->world[ch->in_room]->zone) &&
          (conn->character->isNonPlayer() || isSet(conn->character->misc, DC::LogChannel::CHANNEL_SHOUT)) &&
          !is_ignoring(conn->character, ch))
      {
        for (tmp_obj = ch->dc_->world[conn->character->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
          if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
          {
            silence = true;
            break;
          }
        if (!silence)
          act_to_victim(buf1, ch, 0, conn->character, 0);
      }
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_trivia(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf1;
  QString buf2;
  ConnectionPtr i;
  ObjectPtr tmp_obj;
  bool silence = false;

  if (isSet(ch->dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eSUCCESS;
  }

  for (tmp_obj = ch->dc_->world[ch->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  if (ch->isNonPlayer() && ch->master)
  {
    return do_say(ch, "Why don't you just do that yourself!");
  }

  if (ch->isPlayer())
  {
    if (isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\r\n",
                   ch);
      return ReturnValue::eSUCCESS;
    }
    if (!(isSet(ch->misc, DC::LogChannel::CHANNEL_TRIVIA)))
    {
      ch->sendln(u"You told yourself not to listen to Trivia!!"_s);
      return ReturnValue::eSUCCESS;
    }
    if (ch->getLevel() < 3)
    {
      send_to_char("You must be at least 3rd level to participate in "
                   "trivia.\r\n",
                   ch);
      return ReturnValue::eSUCCESS;
    }
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
  {
    {
      QQueue<QString> tmp = trivia_history;
      ch->sendln(u"Here are the last 10 messages:"_s);
      while (!tmp.isEmpty())
      {
        act_to_victim(tmp.front(), ch, 0, ch, 0);
        tmp.pop();
      }
    }
    return ReturnValue::eSUCCESS;
  }

  ch->decrementMove(5);

  if (ch->getLevel() >= 102)
  {
    dc_sprintf(buf1, "$3$BQuestion $R$3(%s)$B: '%s'$R", qPrintable(ch->shortdesc_or_name()), argument);
    dc_sprintf(buf2, "$3$BYou ask, $R$3'%s'$R", argument);
  }
  else
  {
    dc_sprintf(buf1, "$3$B%s answers '%s'$R", qPrintable(ch->shortdesc_or_name()), argument);
    dc_sprintf(buf2, "$3$BYou answer '%s'$R", argument);
  }
  act_to_character(buf2, ch, 0, 0, 0);

  trivia_history.push(buf1);
  if (trivia_history.size() > 10)
    trivia_history.pop();

  for (auto &i : ch->dc_->connections_)
    if (conn->character != ch && !conn->connected &&
        (isSet(conn->character->misc, DC::LogChannel::CHANNEL_TRIVIA)) &&
        !is_ignoring(conn->character, ch))
    {
      for (tmp_obj = ch->dc_->world[conn->character->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
        if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
        {
          silence = true;
          break;
        }
      if (!silence)
        act_to_victim(buf1, ch, 0, conn->character, 0);
    }

  return ReturnValue::eSUCCESS;
}

ReturnValues do_dream(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf1;
  QString buf2;
  ConnectionPtr i = {};
  qint32 ctr = {};

  if ((GET_POS(ch) != position_t::SLEEPING) && (ch->getLevel() < MIN_GOD))
  {
    ch->sendln(u"How are you going to dream if you're awake?"_s);
    return ReturnValue::eSUCCESS;
  }

  if (ch->isNonPlayer() && ch->master)
  {
    do_say(ch, "Why don't you just do that yourself!");
    return ReturnValue::eSUCCESS;
  }
  if (ch->isPlayer())
    if (isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\r\n",
                   ch);
      return ReturnValue::eSUCCESS;
    }
  if (!(isSet(ch->misc, DC::LogChannel::CHANNEL_DREAM)))
  {
    ch->sendln(u"You told yourself not to dream!!"_s);
    return ReturnValue::eSUCCESS;
  }

  if (ch->getLevel() < 3)
  {
    ch->sendln(u"You must be at least 3rd level to dream."_s);
    return ReturnValue::eSUCCESS;
  }

  for (ctr = {}; (quint32)ctr <= dc_strlen(argument); ctr++)
  {
    if (argument[ctr] == '$')
    {
      argument[ctr] = ' ';
    }
    if ((argument[ctr] == '?') && (argument[ctr + 1] == '?'))
    {
      argument[ctr] = ' ';
    }
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
    ch->sendln(u"It must not have been that great!!"_s);
  else
  {
    dc_sprintf(buf1, "$6%s dreams '$B$1%s$R$6'$R\r\n", qPrintable(ch->shortdesc_or_name()), argument);
    dc_sprintf(buf2, "$6You dream '$B$1%s$R$6'$R\r\n", argument);
    send_to_char(buf2, ch);
    for (auto &i : ch->dc_->connections_)
    {
      if ((conn->character != ch) &&
          (!conn->connected) &&
          !is_ignoring(conn->character, ch) &&
          (isSet(conn->character->misc, DC::LogChannel::CHANNEL_DREAM)) &&
          ((GET_POS(conn->character) == position_t::SLEEPING) ||
           (conn->character->getLevel() >= MIN_GOD)))
        send_to_char(buf1, conn->character);
    }
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_tellhistory(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch == nullptr)
  {
    return ReturnValue::eFAILURE;
  }

  if (ch->isNonPlayer())
  {
    return ReturnValue::eFAILURE;
  }

  QString arg1, remainder;
  std::tie(arg1, remainder) = half_chop(argument);

  if (arg1 == "timestamp")
  {
    QString tell_history_timestamp = ch->getSetting("tell.history.timestamp", "0");

    if (tell_history_timestamp == "0" || tell_history_timestamp.isEmpty())
    {
      ch->player->config->insert("tell.history.timestamp", "1");
      ch->send(fmt::format("tell history timestamp turned on\r\n"));
    }
    else if (tell_history_timestamp == "1")
    {
      ch->player->config->insert("tell.history.timestamp", "0");
      ch->send(fmt::format("tell history timestamp turned off\r\n"));
    }

    ch->save();
  }

  return ReturnValue::eSUCCESS;
}

ReturnValues do_reply(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf = {};
  CharacterPtr vict = {};

  if (ch->isNonPlayer() || ch->player->last_tell.isEmpty())
  {
    ch->sendln(u"You have noone to reply to."_s);
    return ReturnValue::eSUCCESS;
  }

  ObjectPtr tmp_obj;
  for (tmp_obj = ch->dc_->world[ch->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  argument = ltrim(argument);

  if (argument.isEmpty())
  {
    ch->sendln(u"Reply what?"_s);
    if ((vict = get_char(ch->player->last_tell)) && CAN_SEE(ch, vict))
    {
      ch->send(fmt::format("Last tell was from {}.\r\n", qPrintable(ch->player->last_tell)));
    }
    else
    {
      ch->send(u"Last tell was from someone you cannot currently see.\r\n"_s);
    }

    return ReturnValue::eSUCCESS;
  }

  buf = fmt::format("{} {}", qPrintable(ch->player->last_tell), argument);
  ch->do_tell(QString(qPrintable(buf)).split(' '), cmd_t::TELL_REPLY);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_whisper(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr vict;
  QString name, message, buf;

  ObjectPtr tmp_obj;
  for (tmp_obj = ch->dc_->world[ch->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  half_chop(argument, name, message);

  if (name.isEmpty() || message.isEmpty())
    ch->sendln(u"Who do you want to whisper to.. and what??"_s);
  else if (!(vict = ch->get_char_room_vis(name)))
    ch->sendln(u"No-one by that name here.."_s);
  else if (vict == ch)
  {
    act_to_room("$n whispers quietly to $mself.", ch, 0, 0, STAYHIDE);
    ch->sendln(u"You can't seem to get your mouth close enough to your ear..."_s);
  }
  else if (is_ignoring(vict, ch))
  {
    ch->sendln(u"They are ignoring you :("_s);
  }
  else
  {
    dc_sprintf(buf, "$1$B$n whispers to you, '%s'$R", message);
    act_return ar = act_to_victim(buf, ch, 0, vict, STAYHIDE);
    vict->tell_history(ch, ar.str);

    dc_sprintf(buf, "$1$BYou whisper to $N, '%s'$R", message);
    ar = act_to_character(buf, ch, 0, vict, STAYHIDE);
    ch->tell_history(ch, ar.str);

    act("$n whispers something to $N.", ch, 0, vict, TO_ROOM, NOTVICT | STAYHIDE);
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_ask(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr vict;
  QString name, message, buf;

  ObjectPtr tmp_obj;
  for (tmp_obj = ch->dc_->world[ch->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  half_chop(argument, name, message);

  if (name.isEmpty() || message.isEmpty())
    ch->sendln(u"Who do you want to ask something, and what??"_s);
  else if (!(vict = ch->get_char_room_vis(name)))
    ch->sendln(u"No-one by that name here."_s);
  else if (vict == ch)
  {
    act_to_room("$n quietly asks $mself a question.", ch, 0, 0, 0);
    ch->sendln(u"You think about it for a while..."_s);
  }
  else if (is_ignoring(vict, ch))
  {
    ch->sendln(u"They are ignoring you :("_s);
  }
  else
  {
    dc_sprintf(buf, "$B$n asks you, '%s'$R", message);
    act_return ar = act_to_victim(buf, ch, 0, vict, 0);
    vict->tell_history(ch, ar.str);

    dc_sprintf(buf, "$BYou ask $N, '%s'$R", message);
    ar = act_to_character(buf, ch, 0, vict, 0);
    ch->tell_history(ch, ar.str);

    act_to_room("$n asks $N a question.", ch, 0, vict, NOTVICT);
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_grouptell(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  CharacterPtr k;
  CharacterPtr *f;
  ObjectPtr tmp_obj;
  bool silence = false;

  if (ch == nullptr || ch->player == nullptr)
  {
    return ReturnValue::eFAILURE;
  }

  if (argument.isEmpty())
  {
    if (ch->player->gtell_history.isEmpty())
    {
      ch->sendln(u"No one has said anything."_s);
      return ReturnValue::eFAILURE;
    }

    ch->sendln(u"Here are the last 10 group tells:"_s);
    for (const auto &c : ch->player->gtell_history)
    {
      ch->sendln(c.message);
    }

    return ReturnValue::eSUCCESS;
  }

  for (tmp_obj = ch->dc_->world[ch->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  for (; isspace(*argument); argument++)
    ;

  if (!ch->isNonPlayer() && isSet(ch->player->punish, PUNISH_NOTELL))
  {
    ch->sendln(u"Your message didn't get through!!"_s);
    return ReturnValue::eSUCCESS;
  }

  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln(u"You don't have a group to talk to!"_s);
    return ReturnValue::eSUCCESS;
  }

  dc_sprintf(buf, "$B$1You tell the group, $7'%s'$R", argument);
  act_return ar = act_to_character(buf, ch, 0, 0, STAYHIDE | ASLEEP);
  ch->gtell_history(ch, ar.str);

  if (!(k = ch->master))
    k = ch;

  dc_sprintf(buf, "$B$1$n tells the group, $7'%s'$R", argument);

  if (ch->master)
  {
    act_return ar = act_to_victim(buf, ch, 0, ch->master, STAYHIDE | ASLEEP);

    if (ch->master && ch->master->player)
    {
      ch->master->gtell_history(ch, ar.str);
    }
  }

  for (f = k->followers; f; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) && (f->follower != ch))
    {
      for (tmp_obj = ch->dc_->world[f->follower->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
      {
        if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
        {
          silence = true;
          break;
        }
      }
      if (!silence)
      {
        act_return ar = act_to_victim(buf, ch, 0, f->follower, STAYHIDE | ASLEEP);
        if (f->follower && f->follower->player)
        {
          f->follower->gtell_history(ch, ar.str);
        }
      }
    }
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_newbie(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf1;
  QString buf2;
  ConnectionPtr i;
  ObjectPtr tmp_obj;
  bool silence = false;

  if (isSet(ch->dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eSUCCESS;
  }

  for (tmp_obj = ch->dc_->world[ch->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  if (ch->isNonPlayer() && ch->master)
  {
    return do_say(ch, "Why don't you just do that yourself!");
  }

  if (ch->isPlayer())
  {
    if (isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\r\n",
                   ch);
      return ReturnValue::eSUCCESS;
    }
    if (!(isSet(ch->misc, DC::LogChannel::CHANNEL_NEWBIE)))
    {
      ch->sendln(u"You told yourself not to use the newbie channel!!"_s);
      return ReturnValue::eSUCCESS;
    }
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
  {
    QQueue<QString> tmp = newbie_history;
    ch->sendln(u"Here are the last 10 messages:"_s);
    while (!tmp.isEmpty())
    {
      act_to_victim(tmp.front(), ch, 0, ch, 0);
      tmp.pop();
    }
  }
  else
  {
    if (!ch->decrementMove(5))
    {
      return ReturnValue::eFAILURE;
    }
    dc_sprintf(buf1, "$5%s newbies '$R$B%s$R$5'$R", qPrintable(ch->shortdesc_or_name()), argument);
    dc_sprintf(buf2, "$5You newbie '$R$B%s$R$5'$R", argument);
    act_to_character(buf2, ch, 0, 0, 0);

    newbie_history.push(buf1);
    if (newbie_history.size() > 10)
      newbie_history.pop();

    for (auto &i : ch->dc_->connections_)
      if (conn->character != ch && !conn->connected &&
          !is_ignoring(conn->character, ch) &&
          (isSet(conn->character->misc, DC::LogChannel::CHANNEL_NEWBIE)))
      {
        for (tmp_obj = ch->dc_->world[conn->character->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
          if (ch->dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
          {
            silence = true;
            break;
          }
        if (!silence)
          act_to_victim(buf1, ch, 0, conn->character, 0);
      }
  }
  return ReturnValue::eSUCCESS;
}

communication::communication(CharacterPtr ch, QString message)
{
  sender = qPrintable(ch->name());
  sender_ispc = ch->isPlayer();
  message = message;
  timestamp = time(nullptr);
}