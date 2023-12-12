/************************************************************************
| $Id: channel.cpp,v 1.28 2013/12/30 22:46:43 jhhudso Exp $
| channel.C
| Description:  All of the channel - type commands; do_say, gossip, etc..
*/

#include <string.h> //strstr()
#include <cctype>
#include <string>
#include <sstream>
#include <list>
#include <queue>
#include <tuple>
#include <fmt/format.h>

#include "structs.h"
#include "player.h"
#include "room.h"
#include "character.h"
#include "utility.h"
#include "levels.h"
#include "connect.h"
#include "mobile.h"
#include "handler.h"
#include "interp.h"
#include "terminal.h"
#include "act.h"
#include "db.h"
#include "returnvals.h"

class channel_msg
{
public:
  channel_msg(const Character *sender, const int32_t type, const char *msg)
      : type(type), msg(std::string(msg))
  {
    set_wizinvis(sender);
    set_name(sender);
  }

  channel_msg(const Character *sender, const int32_t type, const std::string &msg)
      : type(type), msg(msg)
  {
    set_wizinvis(sender);
    set_name(sender);
  }

  std::string get_msg(const int receiver_level)
  {
    std::stringstream output;
    std::string sender;

    if (receiver_level < wizinvis)
    {
      sender = "Someone";
    }
    else
    {
      sender = name;
    }

    switch (type)
    {
    case LogChannels::CHANNEL_GOSSIP:
      output << "$5$B" << sender << " gossips '" << msg << "$5$B'$R";
      break;
    }

    return output.str();
  }

  inline void set_wizinvis(const Character *sender)
  {
    if (sender && IS_PC(sender))
    {
      wizinvis = sender->player->wizinvis;
    }
    else
    {
      wizinvis = 0;
    }
  }

  inline void set_name(const Character *sender)
  {
    if (sender)
    {
      name = std::string(GET_SHORT(sender));
    }
    else
    {
      name = std::string("Unknown");
      logf(IMMORTAL, LogChannels::LOG_BUG, "channel_msg::set_name: sender is nullptr. type: %d msg: %s", type, msg.c_str());
    }
  }

private:
  std::string name;
  int32_t wizinvis;
  int32_t type;
  std::string msg;
};

std::queue<channel_msg> gossip_history;
std::queue<QString> auction_history;
std::queue<QString> newbie_history;
std::queue<QString> trivia_history;

extern struct index_data *obj_index;

command_return_t do_say(Character *ch, std::string argument, int cmd)
{
  int i;
  std::string buf;
  int retval;
  extern bool MOBtrigger;

  if (!IS_MOB(ch) && DC::isSet(ch->player->punish, PUNISH_STUPID))
  {
    send_to_char("You try to speak but just look like an idiot!\r\n", ch);
    return eSUCCESS;
  }

  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n",
                 ch);
    return eSUCCESS;
  }

  Object *tmp_obj;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  argument = ltrim(argument);

  if (argument.empty())
    send_to_char("Yes, but WHAT do you want to say?\n\r", ch);
  else
  {
    if (IS_PC(ch))
      MOBtrigger = false;

    if (IS_IMMORTAL(ch))
    {
      argument = remove_all_codes(argument);
    }

    buf = fmt::format("$B$7$n says '{}$B$7'$R", argument.c_str());
    act(buf, ch, 0, 0, TO_ROOM, 0);

    if (IS_PC(ch))
      MOBtrigger = false;

    buf = fmt::format("$B$7You say '{}$B$7'$R", argument.c_str());
    act(buf, ch, 0, 0, TO_CHAR, 0);

    if (IS_PC(ch))
    {
      MOBtrigger = true;
      retval = mprog_speech_trigger(argument.c_str(), ch);
      if (SOMEONE_DIED(retval))
        return SWAP_CH_VICT(retval);
    }

    if (IS_PC(ch))
    {
      MOBtrigger = true;
      retval = oprog_speech_trigger(argument.c_str(), ch);
      if (SOMEONE_DIED(retval))
        return SWAP_CH_VICT(retval);
    }
  }
  return eSUCCESS;
}

// Psay works like 'say', just it's directed at a person
// TODO - after this gets used alot, maybe switch speech triggers to it
command_return_t do_psay(Character *ch, std::string argument, int cmd)
{
  std::string vict = {}, message = {}, buf = {};
  Character *victim = nullptr;
  extern bool MOBtrigger;

  if (IS_PC(ch) && DC::isSet(ch->player->punish, PUNISH_STUPID))
  {
    send_to_char("You try to speak but just look like an idiot!\r\n", ch);
    return eSUCCESS;
  }

  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eSUCCESS;
  }

  Object *tmp_obj = nullptr;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (tmp_obj && tmp_obj->item_number >= 0 && obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  std::tie(vict, message) = half_chop(argument);

  if (vict.empty() || message.empty())
  {
    send_to_char("Say what to whom?  psay <target> <message>\r\n", ch);
    return eSUCCESS;
  }

  if (!(victim = ch->get_char_room_vis(vict.c_str())))
  {
    ch->send(QString("You see noone that goes by '%1' here.\r\n").arg(vict.c_str()));
    return eSUCCESS;
  }

  std::string messageStr = message;
  if (IS_IMMORTAL(ch))
  {
    messageStr = remove_all_codes(messageStr);
  }

  if (IS_PC(ch))
    MOBtrigger = false;
  buf = fmt::format("$B$n says (to $N) '{}'$R", messageStr.c_str());
  act(buf, ch, 0, victim, TO_ROOM, NOTVICT);

  if (IS_PC(ch))
    MOBtrigger = false;
  buf = fmt::format("$B$n says (to $3you$7) '{}'$R", messageStr.c_str());
  act(buf, ch, 0, victim, TO_VICT, 0);

  if (IS_PC(ch))
    MOBtrigger = false;
  buf = fmt::format("$BYou say (to $N) '{}'$R", messageStr.c_str());
  act(buf, ch, 0, victim, TO_CHAR, 0);
  MOBtrigger = true;
  //   if(IS_PC(ch)) {
  //     retval = mprog_speech_trigger( message, ch );
  //     MOBtrigger = true;
  //     if(SOMEONE_DIED(retval))
  //       return SWAP_CH_VICT(retval);
  //   }

  return eSUCCESS;
}

int do_pray(Character *ch, char *arg, int cmd)
{
  char buf1[MAX_STRING_LENGTH];
  class Connection *i;

  if (IS_NPC(ch))
    return eSUCCESS;

  while (*arg == ' ')
    arg++;

  if (!*arg)
  {
    send_to_char("You must have something to tell the immortals...\r\n", ch);
    return eSUCCESS;
  }

  if (ch->getLevel() >= IMMORTAL)
  {
    send_to_char("Why pray? You are a god!\n\r", ch);
    return eSUCCESS;
  }

  if (!IS_MOB(ch) && DC::isSet(ch->player->punish, PUNISH_STUPID))
  {
    send_to_char("Duh...I'm too stupid!\n\r", ch);
    return eSUCCESS;
  }

  if (!IS_MOB(ch) && DC::isSet(ch->player->punish, PUNISH_NOPRAY))
  {
    send_to_char("The gods are deaf to your prayers.\r\n", ch);
    return eSUCCESS;
  }

  sprintf(buf1, "\a$4$B**$R$5 %s prays: %s $4$B**$R\n\r", GET_NAME(ch), arg);

  for (i = DC::getInstance()->descriptor_list; i; i = i->next)
  {
    if ((i->character == nullptr) || (i->character->getLevel() <= MORTAL))
      continue;
    if (!(DC::isSet(i->character->misc, LogChannels::LOG_PRAYER)))
      continue;
    if (is_busy(i->character) || is_ignoring(i->character, ch))
      continue;
    if (!i->connected)
      send_to_char(buf1, i->character);
  }
  send_to_char("\a\aOk.\r\n", ch);
  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  return eSUCCESS;
}

int do_gossip(Character *ch, char *argument, int cmd)
{
  char buf2[MAX_STRING_LENGTH];
  class Connection *i;
  Object *tmp_obj;
  bool silence = false;

  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  if (IS_NPC(ch) && ch->master)
  {
    do_say(ch, "Why don't you just do that yourself!", CMD_DEFAULT);
    return eSUCCESS;
  }

  if (GET_POS(ch) == position_t::SLEEPING)
  {
    send_to_char("You're asleep.  Dream or something....\r\n", ch);
    return eSUCCESS;
  }

  if (IS_PC(ch))
  {
    if (!IS_MOB(ch) && DC::isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r",
                   ch);
      return eSUCCESS;
    }
    if (!(DC::isSet(ch->misc, LogChannels::CHANNEL_GOSSIP)))
    {
      send_to_char("You told yourself not to GOSSIP!!\n\r", ch);
      return eSUCCESS;
    }
    if (ch->getLevel() < 3)
    {
      send_to_char("You must be at least 3rd level to gossip.\r\n", ch);
      return eSUCCESS;
    }
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
  {
    std::queue<channel_msg> msgs = gossip_history;
    send_to_char("Here are the last 10 gossips:\n\r", ch);
    while (!msgs.empty())
    {
      act(msgs.front().get_msg(ch->getLevel()), ch, 0, ch, TO_VICT, 0);
      msgs.pop();
    }
  }
  else
  {
    ch->decrementMove(5);

    channel_msg msg(ch, LogChannels::CHANNEL_GOSSIP, argument);

    sprintf(buf2, "$5$BYou gossip '%s'$R", argument);
    act(buf2, ch, 0, 0, TO_CHAR, 0);

    gossip_history.push(msg);
    if (gossip_history.size() > 10)
    {
      gossip_history.pop();
    }

    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
    {
      if (i->character != ch && !i->connected && (DC::isSet(i->character->misc, LogChannels::CHANNEL_GOSSIP)) && !is_ignoring(i->character, ch))
      {
        for (tmp_obj = DC::getInstance()->world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
        {
          if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
          {
            silence = true;
            break;
          }
        }

        if (!silence)
        {
          act(msg.get_msg(i->character->getLevel()), ch, 0, i->character, TO_VICT, 0);
        }
      }
    }
  }
  return eSUCCESS;
}

command_return_t Character::do_auction(QStringList arguments, int cmd)
{
  class Connection *i{};
  Object *tmp_obj{};
  bool silence = false;

  if (DC::isSet(DC::getInstance()->world[in_room].room_flags, QUIET))
  {
    send("SHHHHHH!! Can't you see people are trying to read?\r\n");
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send("The magical silence prevents you from speaking!\n\r");
      return eFAILURE;
    }

  if (IS_NPC(this) && this->master)
  {
    do_say(this, "That's okay, I'll let you do all the auctioning, master.", CMD_DEFAULT);
    return eSUCCESS;
  }

  if (IS_PC(this))
  {
    if (!IS_MOB(this) && DC::isSet(this->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r",
                   this);
      return eSUCCESS;
    }
    if (!(DC::isSet(this->misc, LogChannels::CHANNEL_AUCTION)))
    {
      send_to_char("You told yourself not to AUCTION!!\n\r", this);
      return eSUCCESS;
    }
    if (level_ < 3)
    {
      send_to_char("You must be at least 3rd level to auction.\r\n", this);
      return eSUCCESS;
    }
  }

  if (arguments.isEmpty())
  {
    std::queue<QString> tmp = auction_history;
    send_to_char("Here are the last 10 auctions:\n\r", this);
    while (!tmp.empty())
    {
      act(tmp.front(), this, 0, this, TO_VICT, 0);
      tmp.pop();
    }
  }
  else
  {
    decrementMove(5);

    QString buf1;
    if (IS_NPC(this))
    {
      buf1 = QString("$6$B%1 auctions '%2'$R").arg(GET_SHORT(this)).arg(arguments.join(' '));
    }
    else
    {
      buf1 = QString("$6$B%1 auctions '%2'$R").arg(GET_NAME(this)).arg(arguments.join(' '));
    }

    QString buf2 = QString("$6$BYou auction '%1'$R").arg(arguments.join(' '));
    act(buf2, this, 0, 0, TO_CHAR, 0);

    auction_history.push(buf1);
    if (auction_history.size() > 10)
      auction_history.pop();

    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
      if (i->character != this && !i->connected &&
          (DC::isSet(i->character->misc, LogChannels::CHANNEL_AUCTION)) &&
          !is_ignoring(i->character, this))
      {
        for (tmp_obj = DC::getInstance()->world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
          if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
          {
            silence = true;
            break;
          }
        if (!silence)
          act(buf1, this, 0, i->character, TO_VICT, 0);
      }
  }
  return eSUCCESS;
}

int do_shout(Character *ch, char *argument, int cmd)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  class Connection *i;
  Object *tmp_obj;
  bool silence = false;

  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n",
                 ch);
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  if (IS_NPC(ch) && ch->master)
  {
    return do_say(ch, "Shouting makes my throat hoarse.", CMD_DEFAULT);
  }

  if (IS_PC(ch) && DC::isSet(ch->player->punish, PUNISH_SILENCED))
  {
    send_to_char("You must have somehow offended the gods, for you "
                 "find yourself unable to!\n\r",
                 ch);
    return eSUCCESS;
  }
  if (IS_PC(ch) && !(DC::isSet(ch->misc, LogChannels::CHANNEL_SHOUT)))
  {
    send_to_char("You told yourself not to SHOUT!!\n\r", ch);
    return eSUCCESS;
  }
  if (IS_PC(ch) && ch->getLevel() < 3)
  {
    send_to_char("Due to misuse, you must be of at least 3rd level "
                 "to shout.\r\n",
                 ch);
    return eSUCCESS;
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
    send_to_char("What do you want to shout, dork?\n\r", ch);

  else
  {
    sprintf(buf1, "$B$n shouts '%s'$R", argument);
    sprintf(buf2, "$BYou shout '%s'$R", argument);
    act(buf2, ch, 0, 0, TO_CHAR, 0);

    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
      if (i->character != ch && !i->connected &&
          (DC::getInstance()->world[i->character->in_room].zone == DC::getInstance()->world[ch->in_room].zone) &&
          (IS_NPC(i->character) || DC::isSet(i->character->misc, LogChannels::CHANNEL_SHOUT)) &&
          !is_ignoring(i->character, ch))
      {
        for (tmp_obj = DC::getInstance()->world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
          if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
          {
            silence = true;
            break;
          }
        if (!silence)
          act(buf1, ch, 0, i->character, TO_VICT, 0);
      }
  }
  return eSUCCESS;
}

int do_trivia(Character *ch, char *argument, int cmd)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  class Connection *i;
  Object *tmp_obj;
  bool silence = false;

  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  if (IS_NPC(ch) && ch->master)
  {
    return do_say(ch, "Why don't you just do that yourself!", CMD_DEFAULT);
  }

  if (IS_PC(ch))
  {
    if (DC::isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r",
                   ch);
      return eSUCCESS;
    }
    if (!(DC::isSet(ch->misc, LogChannels::CHANNEL_TRIVIA)))
    {
      send_to_char("You told yourself not to listen to Trivia!!\n\r", ch);
      return eSUCCESS;
    }
    if (ch->getLevel() < 3)
    {
      send_to_char("You must be at least 3rd level to participate in "
                   "trivia.\r\n",
                   ch);
      return eSUCCESS;
    }
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
  {
    {
      std::queue<QString> tmp = trivia_history;
      send_to_char("Here are the last 10 messages:\n\r", ch);
      while (!tmp.empty())
      {
        act(tmp.front(), ch, 0, ch, TO_VICT, 0);
        tmp.pop();
      }
    }
    return eSUCCESS;
  }

  ch->decrementMove(5);

  if (ch->getLevel() >= 102)
  {
    sprintf(buf1, "$3$BQuestion $R$3(%s)$B: '%s'$R", GET_SHORT(ch), argument);
    sprintf(buf2, "$3$BYou ask, $R$3'%s'$R", argument);
  }
  else
  {
    sprintf(buf1, "$3$B%s answers '%s'$R", GET_SHORT(ch), argument);
    sprintf(buf2, "$3$BYou answer '%s'$R", argument);
  }
  act(buf2, ch, 0, 0, TO_CHAR, 0);

  trivia_history.push(buf1);
  if (trivia_history.size() > 10)
    trivia_history.pop();

  for (i = DC::getInstance()->descriptor_list; i; i = i->next)
    if (i->character != ch && !i->connected &&
        (DC::isSet(i->character->misc, LogChannels::CHANNEL_TRIVIA)) &&
        !is_ignoring(i->character, ch))
    {
      for (tmp_obj = DC::getInstance()->world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
        if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
        {
          silence = true;
          break;
        }
      if (!silence)
        act(buf1, ch, 0, i->character, TO_VICT, 0);
    }

  return eSUCCESS;
}

int do_dream(Character *ch, char *argument, int cmd)
{
  char buf1[MAX_STRING_LENGTH] = {0};
  char buf2[MAX_STRING_LENGTH] = {0};
  class Connection *i = nullptr;
  int ctr = 0;

  if ((GET_POS(ch) != position_t::SLEEPING) && (ch->getLevel() < MIN_GOD))
  {
    send_to_char("How are you going to dream if you're awake?\n\r", ch);
    return eSUCCESS;
  }

  if (IS_NPC(ch) && ch->master)
  {
    do_say(ch, "Why don't you just do that yourself!", CMD_DEFAULT);
    return eSUCCESS;
  }
  if (IS_PC(ch))
    if (DC::isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r",
                   ch);
      return eSUCCESS;
    }
  if (!(DC::isSet(ch->misc, LogChannels::CHANNEL_DREAM)))
  {
    send_to_char("You told yourself not to dream!!\n\r", ch);
    return eSUCCESS;
  }

  if (ch->getLevel() < 3)
  {
    send_to_char("You must be at least 3rd level to dream.\r\n", ch);
    return eSUCCESS;
  }

  for (ctr = 0; (unsigned)ctr <= strlen(argument); ctr++)
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
    send_to_char("It must not have been that great!!\n\r", ch);
  else
  {
    sprintf(buf1, "$6%s dreams '$B$1%s$R$6'$R\n\r", GET_SHORT(ch), argument);
    sprintf(buf2, "$6You dream '$B$1%s$R$6'$R\n\r", argument);
    send_to_char(buf2, ch);
    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
    {
      if ((i->character != ch) &&
          (!i->connected) &&
          !is_ignoring(i->character, ch) &&
          (DC::isSet(i->character->misc, LogChannels::CHANNEL_DREAM)) &&
          ((GET_POS(i->character) == position_t::SLEEPING) ||
           (i->character->getLevel() >= MIN_GOD)))
        send_to_char(buf1, i->character);
    }
  }
  return eSUCCESS;
}

command_return_t do_tellhistory(Character *ch, std::string argument, int cmd)
{
  if (ch == nullptr)
  {
    return eFAILURE;
  }

  if (IS_NPC(ch))
  {
    return eFAILURE;
  }

  std::string arg1, remainder;
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

  return eSUCCESS;
}

command_return_t Character::do_tell(QStringList arguments, int cmd)
{
  Character *vict = nullptr;
  QString name = {}, message = {}, buf = {}, log_buf = {};
  Object *tmp_obj = nullptr;

  if (!IS_MOB(this) && DC::isSet(this->player->punish, PUNISH_NOTELL))
  {
    send_to_char("Your message didn't get through!!\n\r", this);
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[this->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", this);
      return eFAILURE;
    }

  if (!IS_MOB(this) && !DC::isSet(this->misc, LogChannels::CHANNEL_TELL))
  {
    send_to_char("You have tell channeled off!!\n\r", this);
    return eSUCCESS;
  }

  name = arguments.value(0);
  if (!arguments.isEmpty())
  {
    arguments.pop_front();
  }
  message = arguments.join(' ');

  if (name.isEmpty() || message.isEmpty())
  {
    if (this->player->tell_history.isEmpty())
    {
      send_to_char("You have not sent or recieved any tell messages.\r\n", this);
      return eSUCCESS;
    }

    send_to_char("Here are the last 10 tell messages:\r\n", this);
    for (const auto &c : player->tell_history)
    {
      sendln(c.message);
    }

    return eSUCCESS;
  }

  if (cmd == 9999)
  {
    if (!(vict = get_active_pc(name)))
    {
      send_to_char("They seem to have left!\n\r", this);
      return eSUCCESS;
    }
    cmd = CMD_DEFAULT;
  }
  else if (!(vict = get_active_pc_vis(name)))
  {
    vict = get_pc_vis(this, name);
    if ((vict != nullptr) && vict->getLevel() >= IMMORTAL)
    {
      send_to_char("That person is busy right now.\r\n", this);
      send_to_char("Your message has been saved.\r\n", this);

      buf = fmt::format("$2$B{} told you, '{}'$R\r\n", PERS(this, vict), message.toStdString()).c_str();
      record_msg(buf, vict);

      buf = fmt::format("$2$B{} told you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();
      vict->tell_history(this, buf);

      buf = fmt::format("$2$BYou told {}, '{}'$R", GET_SHORT(vict), message.toStdString()).c_str();
      this->tell_history(this, buf);
    }
    else
    {
      send_to_char("No-one by that name here.\r\n", this);
    }

    return eSUCCESS;
  }

  // vict guarantted to be a PC
  // Re: Last comment. Switched immortals crash this.

  if (IS_PC(vict) && !DC::isSet(vict->misc, LogChannels::CHANNEL_TELL) && level_ <= DC::MAX_MORTAL_LEVEL)
  {
    send_to_char("The person is ignoring all tells right now.\r\n", this);
    return eSUCCESS;
  }
  else if (IS_PC(vict) && !DC::isSet(vict->misc, LogChannels::CHANNEL_TELL))
  {
    // Immortal sent a tell to a player with NOTELL.  Allow the tell butnotify the imm.
    send_to_char("That player has tell channeled off btw...\r\n", this);
  }
  if (this == vict)
    send_to_char("You try to tell yourself something.\r\n", this);
  else if ((GET_POS(vict) == position_t::SLEEPING || DC::isSet(DC::getInstance()->world[vict->in_room].room_flags, QUIET)) && level_ < IMMORTAL)
    act("Sorry, $E cannot hear you.", this, 0, vict, TO_CHAR, STAYHIDE);
  else
  {
    for (tmp_obj = DC::getInstance()->world[vict->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
      if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
      {
        act("$E cannot hear you right now.", this, 0, vict, TO_CHAR, STAYHIDE);
        return eSUCCESS;
      }
    if (is_ignoring(vict, this))
    {
      csendf(this, "%s is ignoring you right now.\r\n", GET_SHORT(vict));
      return eSUCCESS;
    }
    if (is_busy(vict) && level_ >= OVERSEER)
    {
      if (IS_MOB(vict))
      {
        buf = fmt::format("{} tells you, '{}'", PERS(this, vict), message.toStdString()).c_str();
      }
      else
      {
        buf = fmt::format("{} tells you, '{}'{}", PERS(this, vict), message.toStdString(), DC::isSet(vict->player->toggles, Player::PLR_BEEP) ? '\a' : '\0').c_str();

        if (IS_PC(this) && IS_PC(vict))
        {
          vict->player->last_tell = GET_NAME(this);
        }
      }

      ansi_color(GREEN, vict);
      ansi_color(BOLD, vict);
      send_to_char_regardless(buf, vict);
      ansi_color(NTEXT, vict);

      buf = fmt::format("$2$B{} tells you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();

      vict->tell_history(this, buf);

      buf = fmt::format("$2$BYou tell {}, '{}'$R", PERS(vict, this), message.toStdString()).c_str();
      send_to_char(buf, this);
    }
    else if (!is_busy(vict) && GET_POS(vict) > position_t::SLEEPING)
    {
      if (IS_MOB(vict))
      {
        buf = fmt::format("$2$B{} tells you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();
      }
      else
      {
        buf = fmt::format("$2$B{} tells you, '{}'$R{}", PERS(this, vict), message.toStdString(), DC::isSet(vict->player->toggles, Player::PLR_BEEP) ? '\a' : '\0').c_str();
        if (IS_PC(this) && IS_PC(vict))
          vict->player->last_tell = GET_NAME(this);
      }
      act(buf, vict, 0, 0, TO_CHAR, STAYHIDE);

      buf = fmt::format("$2$B{} tells you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();
      vict->tell_history(this, buf);

      buf = fmt::format("$2$BYou tell {}, '{}'$R", PERS(vict, this), message.toStdString()).c_str();
      act_return ar = act(buf, this, 0, 0, TO_CHAR, STAYHIDE);
      this->tell_history(this, ar.str);

      // Log what I told a logged player under their name
      if (!IS_MOB(vict) && DC::isSet(vict->player->punish, PUNISH_LOG))
      {
        logentry(QString("Log %1: %2 told them: %3").arg(GET_NAME(vict)).arg(GET_NAME(this)).arg(message), IMPLEMENTER, LogChannels::LOG_PLAYER, vict);
      }
    }
    else if (!is_busy(vict) && GET_POS(vict) == position_t::SLEEPING &&
             level_ >= SERAPH)
    {
      send_to_char("A heavenly power intrudes on your subconcious dreaming...\r\n", vict);
      if (IS_MOB(vict))
      {
        buf = fmt::format("{} tells you, '{}'", PERS(this, vict), message.toStdString()).c_str();
      }
      else
      {
        buf = fmt::format("{} tells you, '{}'{}", PERS(this, vict), message.toStdString(), DC::isSet(vict->player->toggles, Player::PLR_BEEP) ? '\a' : '\0').c_str();

        if (IS_PC(this) && IS_PC(vict))
          vict->player->last_tell = GET_NAME(this);
      }
      ansi_color(GREEN, vict);
      ansi_color(BOLD, vict);
      send_to_char_regardless(buf, vict);
      ansi_color(NTEXT, vict);

      buf = fmt::format("$2$B{} tells you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();
      vict->tell_history(this, buf);

      buf = fmt::format("$2$BYou tell {}, '{}'$R", PERS(vict, this), message.toStdString()).c_str();
      act_return ar = act(buf, this, 0, 0, TO_CHAR, STAYHIDE);
      this->tell_history(this, ar.str);

      send_to_char("They were sleeping btw...\r\n", this);
      // Log what I told a logged player under their name
      if (!IS_MOB(vict) && DC::isSet(vict->player->punish, PUNISH_LOG))
      {
        logentry(QString("Log %1: %2 told them: %3").arg(GET_NAME(vict)).arg(GET_NAME(this)).arg(message), IMPLEMENTER, LogChannels::LOG_PLAYER, vict);
      }
    }
    else
    {
      buf = fmt::format("$2$B%s can't hear anything right now.$R", GET_SHORT(vict)).c_str();
      act(buf, this, 0, 0, TO_CHAR, STAYHIDE);
    }
  }
  return eSUCCESS;
}

command_return_t do_reply(Character *ch, std::string argument, int cmd)
{
  std::string buf = {};
  Character *vict = nullptr;

  if (IS_MOB(ch) || ch->player->last_tell.isEmpty())
  {
    send_to_char("You have noone to reply to.\r\n", ch);
    return eSUCCESS;
  }

  Object *tmp_obj;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  argument = ltrim(argument);

  if (argument.empty())
  {
    send_to_char("Reply what?\n\r", ch);
    if ((vict = get_char(ch->player->last_tell)) && CAN_SEE(ch, vict))
    {
      ch->send(fmt::format("Last tell was from {}.\r\n", ch->player->last_tell.toStdString().c_str()));
    }
    else
    {
      ch->send("Last tell was from someone you cannot currently see.\r\n");
    }

    return eSUCCESS;
  }

  buf = fmt::format("{} {}", ch->player->last_tell.toStdString().c_str(), argument);
  ch->do_tell(QString(buf.c_str()).split(' '), CMD_TELL_REPLY);
  return eSUCCESS;
}

int do_whisper(Character *ch, char *argument, int cmd)
{
  Character *vict;
  char name[MAX_INPUT_LENGTH + 1], message[MAX_STRING_LENGTH],
      buf[MAX_STRING_LENGTH];

  Object *tmp_obj;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char("Who do you want to whisper to.. and what??\n\r", ch);
  else if (!(vict = ch->get_char_room_vis(name)))
    send_to_char("No-one by that name here..\r\n", ch);
  else if (vict == ch)
  {
    act("$n whispers quietly to $mself.", ch, 0, 0, TO_ROOM, STAYHIDE);
    send_to_char(
        "You can't seem to get your mouth close enough to your ear...\r\n", ch);
  }
  else if (is_ignoring(vict, ch))
  {
    send_to_char("They are ignoring you :(\n\r", ch);
  }
  else
  {
    sprintf(buf, "$1$B$n whispers to you, '%s'$R", message);
    act_return ar = act(buf, ch, 0, vict, TO_VICT, STAYHIDE);
    vict->tell_history(ch, ar.str);

    sprintf(buf, "$1$BYou whisper to $N, '%s'$R", message);
    ar = act(buf, ch, 0, vict, TO_CHAR, STAYHIDE);
    ch->tell_history(ch, ar.str);

    act("$n whispers something to $N.", ch, 0, vict, TO_ROOM,
        NOTVICT | STAYHIDE);
  }
  return eSUCCESS;
}

int do_ask(Character *ch, char *argument, int cmd)
{
  Character *vict;
  char name[MAX_INPUT_LENGTH + 1], message[MAX_INPUT_LENGTH + 1], buf[MAX_STRING_LENGTH];

  Object *tmp_obj;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  half_chop(argument, name, message);
  name[MAX_INPUT_LENGTH] = '\0';
  message[MAX_INPUT_LENGTH] = '\0';

  if (!*name || !*message)
    send_to_char("Who do you want to ask something, and what??\n\r", ch);
  else if (!(vict = ch->get_char_room_vis(name)))
    send_to_char("No-one by that name here.\r\n", ch);
  else if (vict == ch)
  {
    act("$n quietly asks $mself a question.", ch, 0, 0, TO_ROOM, 0);
    send_to_char("You think about it for a while...\r\n", ch);
  }
  else if (is_ignoring(vict, ch))
  {
    send_to_char("They are ignoring you :(\n\r", ch);
  }
  else
  {
    sprintf(buf, "$B$n asks you, '%s'$R", message);
    act_return ar = act(buf, ch, 0, vict, TO_VICT, 0);
    vict->tell_history(ch, ar.str);

    sprintf(buf, "$BYou ask $N, '%s'$R", message);
    ar = act(buf, ch, 0, vict, TO_CHAR, 0);
    ch->tell_history(ch, ar.str);

    act("$n asks $N a question.", ch, 0, vict, TO_ROOM, NOTVICT);
  }
  return eSUCCESS;
}

int do_grouptell(Character *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  Character *k;
  struct follow_type *f;
  Object *tmp_obj;
  bool silence = false;

  if (ch == nullptr || ch->player == nullptr)
  {
    return eFAILURE;
  }

  if (!*argument)
  {
    if (ch->player->gtell_history.isEmpty())
    {
      send_to_char("No one has said anything.\r\n", ch);
      return eFAILURE;
    }

    send_to_char("Here are the last 10 group tells:\r\n", ch);
    for (const auto &c : ch->player->gtell_history)
    {
      ch->sendln(c.message);
    }

    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  for (; isspace(*argument); argument++)
    ;

  if (!IS_MOB(ch) && DC::isSet(ch->player->punish, PUNISH_NOTELL))
  {
    send_to_char("Your message didn't get through!!\n\r", ch);
    return eSUCCESS;
  }

  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    send_to_char("You don't have a group to talk to!\n\r", ch);
    return eSUCCESS;
  }

  sprintf(buf, "$B$1You tell the group, $7'%s'$R", argument);
  act_return ar = act(buf, ch, 0, 0, TO_CHAR, STAYHIDE | ASLEEP);
  ch->gtell_history(ch, ar.str);

  if (!(k = ch->master))
    k = ch;

  sprintf(buf, "$B$1$n tells the group, $7'%s'$R", argument);

  if (ch->master)
  {
    act_return ar = act(buf, ch, 0, ch->master, TO_VICT, STAYHIDE | ASLEEP);

    if (ch->master && ch->master->player)
    {
      ch->master->gtell_history(ch, ar.str);
    }
  }

  for (f = k->followers; f; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) && (f->follower != ch))
    {
      for (tmp_obj = DC::getInstance()->world[f->follower->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
      {
        if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
        {
          silence = true;
          break;
        }
      }
      if (!silence)
      {
        act_return ar = act(buf, ch, 0, f->follower, TO_VICT, STAYHIDE | ASLEEP);
        if (f->follower && f->follower->player)
        {
          f->follower->gtell_history(ch, ar.str);
        }
      }
    }
  }
  return eSUCCESS;
}

int do_newbie(Character *ch, char *argument, int cmd)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  class Connection *i;
  Object *tmp_obj;
  bool silence = false;

  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n",
                 ch);
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  if (IS_NPC(ch) && ch->master)
  {
    return do_say(ch, "Why don't you just do that yourself!", CMD_DEFAULT);
  }

  if (IS_PC(ch))
  {
    if (DC::isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r",
                   ch);
      return eSUCCESS;
    }
    if (!(DC::isSet(ch->misc, LogChannels::CHANNEL_NEWBIE)))
    {
      send_to_char("You told yourself not to use the newbie channel!!\n\r", ch);
      return eSUCCESS;
    }
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
  {
    std::queue<QString> tmp = newbie_history;
    send_to_char("Here are the last 10 messages:\n\r", ch);
    while (!tmp.empty())
    {
      act(tmp.front(), ch, 0, ch, TO_VICT, 0);
      tmp.pop();
    }
  }
  else
  {
    if (!ch->decrementMove(5))
    {
      return eFAILURE;
    }
    sprintf(buf1, "$5%s newbies '$R$B%s$R$5'$R", GET_SHORT(ch), argument);
    sprintf(buf2, "$5You newbie '$R$B%s$R$5'$R", argument);
    act(buf2, ch, 0, 0, TO_CHAR, 0);

    newbie_history.push(buf1);
    if (newbie_history.size() > 10)
      newbie_history.pop();

    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
      if (i->character != ch && !i->connected &&
          !is_ignoring(i->character, ch) &&
          (DC::isSet(i->character->misc, LogChannels::CHANNEL_NEWBIE)))
      {
        for (tmp_obj = DC::getInstance()->world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
          if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
          {
            silence = true;
            break;
          }
        if (!silence)
          act(buf1, ch, 0, i->character, TO_VICT, 0);
      }
  }
  return eSUCCESS;
}

void Character::tell_history(Character *ch, QString message)
{
  if (this->player == nullptr)
  {
    return;
  }

  communication c(ch, message);

  this->player->tell_history.push_back(c);
  if (this->player->tell_history.size() > 10)
  {
    this->player->tell_history.pop_front();
  }
}

void Character::gtell_history(Character *ch, QString message)
{
  if (this->player == nullptr)
  {
    return;
  }

  communication c(ch, message);

  this->player->gtell_history.push_back(c);
  if (this->player->gtell_history.size() > 10)
  {
    this->player->gtell_history.pop_front();
  }
}

communication::communication(Character *ch, QString message)
{
  this->sender = GET_NAME(ch);
  this->sender_ispc = IS_PC(ch);
  this->message = message;
  this->timestamp = time(nullptr);
}