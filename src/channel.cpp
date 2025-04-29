/************************************************************************
| $Id: channel.cpp,v 1.28 2013/12/30 22:46:43 jhhudso Exp $
| channel.C
| Description:  All of the channel - type commands; do_say, gossip, etc..
*/

#include <cstring> //strstr()
#include <cctype>
#include <string>
#include <sstream>
#include <list>
#include <queue>
#include <tuple>
#include <fmt/format.h>

#include "DC/structs.h"
#include "DC/player.h"
#include "DC/room.h"
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/levels.h"
#include "DC/connect.h"
#include "DC/mobile.h"
#include "DC/handler.h"
#include "DC/interp.h"
#include "DC/terminal.h"
#include "DC/act.h"
#include "DC/db.h"
#include "DC/returnvals.h"
#include "DC/obj.h"

class channel_msg
{
public:
  channel_msg(const Character *sender, const DC::LogChannel type, const char *msg)
      : type_(type), msg(std::string(msg))
  {
    set_wizinvis(sender);
    set_name(sender);
  }

  channel_msg(const Character *sender, const DC::LogChannel type, const std::string &msg)
      : type_(type), msg(msg)
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

    switch (type_)
    {
    case DC::LogChannel::CHANNEL_GOSSIP:
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
      logf(IMMORTAL, DC::LogChannel::LOG_BUG, "channel_msg::set_name: sender is nullptr. type: %d msg: %s", type_, msg.c_str());
    }
  }

private:
  std::string name;
  int32_t wizinvis;
  DC::LogChannel type_;
  std::string msg;
};

std::queue<channel_msg> gossip_history;
std::queue<QString> auction_history;
std::queue<QString> newbie_history;
std::queue<QString> trivia_history;

command_return_t do_say(Character *ch, std::string argument, int cmd)
{
  int i;
  std::string buf;
  int retval;
  extern bool MOBtrigger;

  if (!IS_NPC(ch) && isSet(ch->player->punish, PUNISH_STUPID))
  {
    ch->sendln("You try to speak but just look like an idiot!");
    return eSUCCESS;
  }

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eSUCCESS;
  }

  Object *tmp_obj;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  argument = ltrim(argument);

  if (argument.empty())
    ch->sendln("Yes, but WHAT do you want to say?");
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

  if (IS_PC(ch) && isSet(ch->player->punish, PUNISH_STUPID))
  {
    ch->sendln("You try to speak but just look like an idiot!");
    return eSUCCESS;
  }

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eSUCCESS;
  }

  Object *tmp_obj = nullptr;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (tmp_obj && tmp_obj->item_number >= 0 && DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  std::tie(vict, message) = half_chop(argument);

  if (vict.empty() || message.empty())
  {
    ch->sendln("Say what to whom?  psay <target> <message>");
    return eSUCCESS;
  }

  if (!(victim = ch->get_char_room_vis(vict.c_str())))
  {
    ch->send(QStringLiteral("You see noone that goes by '%1' here.\r\n").arg(vict.c_str()));
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
    ch->sendln("You must have something to tell the immortals...");
    return eSUCCESS;
  }

  if (ch->isImmortalPlayer())
  {
    ch->sendln("Why pray? You are a god!");
    return eSUCCESS;
  }

  if (!IS_NPC(ch) && isSet(ch->player->punish, PUNISH_STUPID))
  {
    ch->sendln("Duh...I'm too stupid!");
    return eSUCCESS;
  }

  if (!IS_NPC(ch) && isSet(ch->player->punish, PUNISH_NOPRAY))
  {
    ch->sendln("The gods are deaf to your prayers.");
    return eSUCCESS;
  }

  sprintf(buf1, "\a$4$B**$R$5 %s prays: %s $4$B**$R\n\r", GET_NAME(ch), arg);

  for (i = DC::getInstance()->descriptor_list; i; i = i->next)
  {
    if ((i->character == nullptr) || (i->character->getLevel() <= MORTAL))
      continue;
    if (!(isSet(i->character->misc, DC::LogChannel::LOG_PRAYER)))
      continue;
    if (is_busy(i->character) || is_ignoring(i->character, ch))
      continue;
    if (!i->connected)
      send_to_char(buf1, i->character);
  }
  ch->sendln("\a\aOk.");
  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  return eSUCCESS;
}

int do_gossip(Character *ch, char *argument, int cmd)
{
  char buf2[MAX_STRING_LENGTH];
  class Connection *i;
  Object *tmp_obj;
  bool silence = false;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  if (IS_NPC(ch) && ch->master)
  {
    do_say(ch, "Why don't you just do that yourself!", CMD_DEFAULT);
    return eSUCCESS;
  }

  if (GET_POS(ch) == position_t::SLEEPING)
  {
    ch->sendln("You're asleep.  Dream or something....");
    return eSUCCESS;
  }

  if (IS_PC(ch))
  {
    if (!IS_NPC(ch) && isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r",
                   ch);
      return eSUCCESS;
    }
    if (!(isSet(ch->misc, DC::LogChannel::CHANNEL_GOSSIP)))
    {
      ch->sendln("You told yourself not to GOSSIP!!");
      return eSUCCESS;
    }
    if (ch->getLevel() < 3)
    {
      ch->sendln("You must be at least 3rd level to gossip.");
      return eSUCCESS;
    }
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
  {
    std::queue<channel_msg> msgs = gossip_history;
    ch->sendln("Here are the last 10 gossips:");
    while (!msgs.empty())
    {
      act(msgs.front().get_msg(ch->getLevel()), ch, 0, ch, TO_VICT, 0);
      msgs.pop();
    }
  }
  else
  {
    ch->decrementMove(5);

    channel_msg msg(ch, DC::LogChannel::CHANNEL_GOSSIP, argument);

    sprintf(buf2, "$5$BYou gossip '%s'$R", argument);
    act(buf2, ch, 0, 0, TO_CHAR, 0);

    gossip_history.push(msg);
    if (gossip_history.size() > 10)
    {
      gossip_history.pop();
    }

    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
    {
      if (i->character != ch && !i->connected && (isSet(i->character->misc, DC::LogChannel::CHANNEL_GOSSIP)) && !is_ignoring(i->character, ch))
      {
        for (tmp_obj = DC::getInstance()->world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
        {
          if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
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

  if (isSet(DC::getInstance()->world[in_room].room_flags, QUIET))
  {
    send("SHHHHHH!! Can't you see people are trying to read?\r\n");
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
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
    if (!IS_NPC(this) && isSet(this->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r",
                   this);
      return eSUCCESS;
    }
    if (!(isSet(this->misc, DC::LogChannel::CHANNEL_AUCTION)))
    {
      this->sendln("You told yourself not to AUCTION!!");
      return eSUCCESS;
    }
    if (level_ < 3)
    {
      this->sendln("You must be at least 3rd level to auction.");
      return eSUCCESS;
    }
  }

  if (arguments.isEmpty())
  {
    std::queue<QString> tmp = auction_history;
    this->sendln("Here are the last 10 auctions:");
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
      buf1 = QStringLiteral("$6$B%1 auctions '%2'$R").arg(GET_SHORT(this)).arg(arguments.join(' '));
    }
    else
    {
      buf1 = QStringLiteral("$6$B%1 auctions '%2'$R").arg(GET_NAME(this)).arg(arguments.join(' '));
    }

    QString buf2 = QStringLiteral("$6$BYou auction '%1'$R").arg(arguments.join(' '));
    act(buf2, this, 0, 0, TO_CHAR, 0);

    auction_history.push(buf1);
    if (auction_history.size() > 10)
      auction_history.pop();

    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
      if (i->character != this && !i->connected &&
          (isSet(i->character->misc, DC::LogChannel::CHANNEL_AUCTION)) &&
          !is_ignoring(i->character, this))
      {
        for (tmp_obj = DC::getInstance()->world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
          if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
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

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  if (IS_NPC(ch) && ch->master)
  {
    return do_say(ch, "Shouting makes my throat hoarse.", CMD_DEFAULT);
  }

  if (IS_PC(ch) && isSet(ch->player->punish, PUNISH_SILENCED))
  {
    send_to_char("You must have somehow offended the gods, for you "
                 "find yourself unable to!\n\r",
                 ch);
    return eSUCCESS;
  }
  if (IS_PC(ch) && !(isSet(ch->misc, DC::LogChannel::CHANNEL_SHOUT)))
  {
    ch->sendln("You told yourself not to SHOUT!!");
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
    ch->sendln("What do you want to shout, dork?");

  else
  {
    sprintf(buf1, "$B$n shouts '%s'$R", argument);
    sprintf(buf2, "$BYou shout '%s'$R", argument);
    act(buf2, ch, 0, 0, TO_CHAR, 0);

    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
      if (i->character != ch && !i->connected &&
          (DC::getInstance()->world[i->character->in_room].zone == DC::getInstance()->world[ch->in_room].zone) &&
          (IS_NPC(i->character) || isSet(i->character->misc, DC::LogChannel::CHANNEL_SHOUT)) &&
          !is_ignoring(i->character, ch))
      {
        for (tmp_obj = DC::getInstance()->world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
          if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
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

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  if (IS_NPC(ch) && ch->master)
  {
    return do_say(ch, "Why don't you just do that yourself!", CMD_DEFAULT);
  }

  if (IS_PC(ch))
  {
    if (isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r",
                   ch);
      return eSUCCESS;
    }
    if (!(isSet(ch->misc, DC::LogChannel::CHANNEL_TRIVIA)))
    {
      ch->sendln("You told yourself not to listen to Trivia!!");
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
      ch->sendln("Here are the last 10 messages:");
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
        (isSet(i->character->misc, DC::LogChannel::CHANNEL_TRIVIA)) &&
        !is_ignoring(i->character, ch))
    {
      for (tmp_obj = DC::getInstance()->world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
        if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
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
    ch->sendln("How are you going to dream if you're awake?");
    return eSUCCESS;
  }

  if (IS_NPC(ch) && ch->master)
  {
    do_say(ch, "Why don't you just do that yourself!", CMD_DEFAULT);
    return eSUCCESS;
  }
  if (IS_PC(ch))
    if (isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r",
                   ch);
      return eSUCCESS;
    }
  if (!(isSet(ch->misc, DC::LogChannel::CHANNEL_DREAM)))
  {
    ch->sendln("You told yourself not to dream!!");
    return eSUCCESS;
  }

  if (ch->getLevel() < 3)
  {
    ch->sendln("You must be at least 3rd level to dream.");
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
    ch->sendln("It must not have been that great!!");
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
          (isSet(i->character->misc, DC::LogChannel::CHANNEL_DREAM)) &&
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

  if (!IS_NPC(this) && isSet(this->player->punish, PUNISH_NOTELL))
  {
    this->sendln("Your message didn't get through!!");
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[this->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      this->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  if (!IS_NPC(this) && !isSet(this->misc, DC::LogChannel::CHANNEL_TELL))
  {
    this->sendln("You have tell channeled off!!");
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
      this->sendln("You have not sent or recieved any tell messages.");
      return eSUCCESS;
    }

    this->sendln("Here are the last 10 tell messages:");
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
      this->sendln("They seem to have left!");
      return eSUCCESS;
    }
    cmd = CMD_DEFAULT;
  }
  else if (!(vict = get_active_pc_vis(name)))
  {
    vict = get_pc_vis(this, name);
    if ((vict != nullptr) && vict->getLevel() >= IMMORTAL)
    {
      this->sendln("That person is busy right now.");
      this->sendln("Your message has been saved.");

      buf = fmt::format("$2$B{} told you, '{}'$R\r\n", PERS(this, vict), message.toStdString()).c_str();
      record_msg(buf, vict);

      buf = fmt::format("$2$B{} told you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();
      vict->tell_history(this, buf);

      buf = fmt::format("$2$BYou told {}, '{}'$R", GET_SHORT(vict), message.toStdString()).c_str();
      this->tell_history(this, buf);
    }
    else
    {
      this->sendln("No-one by that name here.");
    }

    return eSUCCESS;
  }

  // vict guarantted to be a PC
  // Re: Last comment. Switched immortals crash this.

  if (IS_PC(vict) && !isSet(vict->misc, DC::LogChannel::CHANNEL_TELL) && level_ <= DC::MAX_MORTAL_LEVEL)
  {
    this->sendln("The person is ignoring all tells right now.");
    return eSUCCESS;
  }
  else if (IS_PC(vict) && !isSet(vict->misc, DC::LogChannel::CHANNEL_TELL))
  {
    // Immortal sent a tell to a player with NOTELL.  Allow the tell butnotify the imm.
    this->sendln("That player has tell channeled off btw...");
  }
  if (this == vict)
    this->sendln("You try to tell yourself something.");
  else if ((GET_POS(vict) == position_t::SLEEPING || isSet(DC::getInstance()->world[vict->in_room].room_flags, QUIET)) && level_ < IMMORTAL)
    act("Sorry, $E cannot hear you.", this, 0, vict, TO_CHAR, STAYHIDE);
  else
  {
    for (tmp_obj = DC::getInstance()->world[vict->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
      if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
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
      if (IS_NPC(vict))
      {
        buf = fmt::format("{} tells you, '{}'", PERS(this, vict), message.toStdString()).c_str();
      }
      else
      {
        buf = fmt::format("{} tells you, '{}'{}", PERS(this, vict), message.toStdString(), isSet(vict->player->toggles, Player::PLR_BEEP) ? '\a' : '\0').c_str();

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
      this->send(buf);
    }
    else if (!is_busy(vict) && GET_POS(vict) > position_t::SLEEPING)
    {
      if (IS_NPC(vict))
      {
        buf = fmt::format("$2$B{} tells you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();
      }
      else
      {
        buf = fmt::format("$2$B{} tells you, '{}'$R{}", PERS(this, vict), message.toStdString(), isSet(vict->player->toggles, Player::PLR_BEEP) ? '\a' : '\0').c_str();
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
      if (!IS_NPC(vict) && isSet(vict->player->punish, PUNISH_LOG))
      {
        logentry(QStringLiteral("Log %1: %2 told them: %3").arg(GET_NAME(vict)).arg(GET_NAME(this)).arg(message), IMPLEMENTER, DC::LogChannel::LOG_PLAYER, vict);
      }
    }
    else if (!is_busy(vict) && GET_POS(vict) == position_t::SLEEPING &&
             level_ >= SERAPH)
    {
      vict->sendln("A heavenly power intrudes on your subconcious dreaming...");
      if (IS_NPC(vict))
      {
        buf = fmt::format("{} tells you, '{}'", PERS(this, vict), message.toStdString()).c_str();
      }
      else
      {
        buf = fmt::format("{} tells you, '{}'{}", PERS(this, vict), message.toStdString(), isSet(vict->player->toggles, Player::PLR_BEEP) ? '\a' : '\0').c_str();

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

      this->sendln("They were sleeping btw...");
      // Log what I told a logged player under their name
      if (!IS_NPC(vict) && isSet(vict->player->punish, PUNISH_LOG))
      {
        logentry(QStringLiteral("Log %1: %2 told them: %3").arg(GET_NAME(vict)).arg(GET_NAME(this)).arg(message), IMPLEMENTER, DC::LogChannel::LOG_PLAYER, vict);
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

  if (IS_NPC(ch) || ch->player->last_tell.isEmpty())
  {
    ch->sendln("You have noone to reply to.");
    return eSUCCESS;
  }

  Object *tmp_obj;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  argument = ltrim(argument);

  if (argument.empty())
  {
    ch->sendln("Reply what?");
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
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  half_chop(argument, name, message);

  if (!*name || !*message)
    ch->sendln("Who do you want to whisper to.. and what??");
  else if (!(vict = ch->get_char_room_vis(name)))
    ch->sendln("No-one by that name here..");
  else if (vict == ch)
  {
    act("$n whispers quietly to $mself.", ch, 0, 0, TO_ROOM, STAYHIDE);
    ch->sendln("You can't seem to get your mouth close enough to your ear...");
  }
  else if (is_ignoring(vict, ch))
  {
    ch->sendln("They are ignoring you :(");
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
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  half_chop(argument, name, message);
  name[MAX_INPUT_LENGTH] = '\0';
  message[MAX_INPUT_LENGTH] = '\0';

  if (!*name || !*message)
    ch->sendln("Who do you want to ask something, and what??");
  else if (!(vict = ch->get_char_room_vis(name)))
    ch->sendln("No-one by that name here.");
  else if (vict == ch)
  {
    act("$n quietly asks $mself a question.", ch, 0, 0, TO_ROOM, 0);
    ch->sendln("You think about it for a while...");
  }
  else if (is_ignoring(vict, ch))
  {
    ch->sendln("They are ignoring you :(");
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
      ch->sendln("No one has said anything.");
      return eFAILURE;
    }

    ch->sendln("Here are the last 10 group tells:");
    for (const auto &c : ch->player->gtell_history)
    {
      ch->sendln(c.message);
    }

    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  for (; isspace(*argument); argument++)
    ;

  if (!IS_NPC(ch) && isSet(ch->player->punish, PUNISH_NOTELL))
  {
    ch->sendln("Your message didn't get through!!");
    return eSUCCESS;
  }

  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You don't have a group to talk to!");
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
        if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
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

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eSUCCESS;
  }

  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  if (IS_NPC(ch) && ch->master)
  {
    return do_say(ch, "Why don't you just do that yourself!", CMD_DEFAULT);
  }

  if (IS_PC(ch))
  {
    if (isSet(ch->player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r",
                   ch);
      return eSUCCESS;
    }
    if (!(isSet(ch->misc, DC::LogChannel::CHANNEL_NEWBIE)))
    {
      ch->sendln("You told yourself not to use the newbie channel!!");
      return eSUCCESS;
    }
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
  {
    std::queue<QString> tmp = newbie_history;
    ch->sendln("Here are the last 10 messages:");
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
          (isSet(i->character->misc, DC::LogChannel::CHANNEL_NEWBIE)))
      {
        for (tmp_obj = DC::getInstance()->world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
          if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
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