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

using namespace std;

class channel_msg {
public:
  channel_msg(const char_data *sender, const int32_t type, const char *msg) 
    : type(type), msg(string(msg))
  { 
    set_wizinvis(sender);
    set_name(sender);
  }

  channel_msg(const char_data *sender, const int32_t type, const string &msg) 
    : type(type), msg(msg)
  { 
    set_wizinvis(sender);
    set_name(sender);
  }

  string get_msg(const int receiver_level) {
    stringstream output;
    string sender;

    if (receiver_level < wizinvis) {
      sender = "Someone";
    } else {
      sender = name;
    }

    switch(type) {
    case CHANNEL_GOSSIP:
      output << "$5$B" << sender << " gossips '" << msg << "$5$B'$R";
      break;
    }

    return output.str();
  }

  inline void set_wizinvis(const char_data *sender) {
    if (sender && IS_PC(sender)) {
      wizinvis = sender->pcdata->wizinvis;
    } else {
      wizinvis = 0;
    }
  }

  inline void set_name(const char_data *sender) {
    if (sender) {
      name = string(GET_SHORT(sender));
    } else {
      name = string("Unknown");
      logf(IMMORTAL, LOG_BUG, "channel_msg::set_name: sender is NULL. type: %d msg: %s", type, msg.c_str());
    }
  }

private:
  string name;
  long wizinvis;
  int32_t type;
  string msg;
};

queue<channel_msg> gossip_history;
queue<string> auction_history;
queue<string> newbie_history;
queue<string> trivia_history;

extern CWorld world;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;

int do_say(struct char_data *ch, char *argument, int cmd)
{
   int i;
   char buf[MAX_STRING_LENGTH];
   int retval;
   extern bool MOBtrigger;

   if (!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_STUPID)) {
      send_to_char ("You try to speak but just look like an idiot!\r\n", ch);
      return eSUCCESS;
      }

   if (IS_SET(world[ch->in_room].room_flags, QUIET)) {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n",
                    ch);
      return eSUCCESS;
      }

   OBJ_DATA *tmp_obj;
   for(tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
     if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
       send_to_char("The magical silence prevents you from speaking!\n\r", ch);
       return eFAILURE;
     }

   for(i = 0; *(argument + i) == ' '; i++)
      ;

   if (!*(argument + i))
      send_to_char("Yes, but WHAT do you want to say?\n\r", ch);
   else {
      if(!IS_NPC(ch))
        MOBtrigger = FALSE;
      sprintf(buf,"$B$n says '%s'$R", argument + i);
      act(buf, ch, 0, 0, TO_ROOM, 0);
      if(!IS_NPC(ch))
        MOBtrigger = FALSE;
      sprintf(buf,"$BYou say '%s'$R", argument + i);
      act(buf, ch, 0, 0, TO_CHAR, 0);

      if(!IS_NPC(ch)) {
        MOBtrigger = TRUE;
        retval = mprog_speech_trigger( argument, ch );
        if(SOMEONE_DIED(retval))
          return SWAP_CH_VICT(retval);
      }
       if(!IS_NPC(ch)) {
        MOBtrigger = TRUE;
        retval = oprog_speech_trigger( argument, ch );
        if(SOMEONE_DIED(retval))
          return SWAP_CH_VICT(retval);
      }

   }
   return eSUCCESS;
}

// Psay works like 'say', just it's directed at a person
// TODO - after this gets used alot, maybe switch speech triggers to it
int do_psay(struct char_data *ch, char *argument, int cmd)
{
   char vict[MAX_INPUT_LENGTH];
   char message[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   char_data * victim;
   extern bool MOBtrigger;

   if (!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_STUPID)) {
      send_to_char ("You try to speak but just look like an idiot!\r\n", ch);
      return eSUCCESS;
   }

   if (IS_SET(world[ch->in_room].room_flags, QUIET)) {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eSUCCESS;
   }

   OBJ_DATA *tmp_obj;
   for(tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
     if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
       send_to_char("The magical silence prevents you from speaking!\n\r", ch);
       return eFAILURE;
     }

   half_chop(argument, vict, message);

   if(!*vict || !*message) {
      send_to_char("Say what to whom?  psay <target> <message>\r\n", ch);
      return eSUCCESS;
   }

   if (!(victim = get_char_room_vis(ch, vict))) {
      csendf(ch, "You see noone that goes by '%s' here.\r\n", vict);
      return eSUCCESS;
   }

   if(!IS_NPC(ch))
     MOBtrigger = FALSE;
   sprintf(buf,"$B$n says (to $N) '%s'$R", message);
   act(buf, ch, 0, victim, TO_ROOM, NOTVICT);

   if(!IS_NPC(ch))
     MOBtrigger = FALSE;
   sprintf(buf,"$B$n says (to $3you$7) '%s'$R", message);
   act(buf, ch, 0, victim, TO_VICT, 0);

   if(!IS_NPC(ch))
     MOBtrigger = FALSE;
   sprintf(buf,"$BYou say (to $N) '%s'$R", message);
   act(buf, ch, 0, victim, TO_CHAR, 0);
   MOBtrigger = TRUE;
//   if(!IS_NPC(ch)) {
//     retval = mprog_speech_trigger( message, ch );
//     MOBtrigger = TRUE;
//     if(SOMEONE_DIED(retval))
//       return SWAP_CH_VICT(retval);
//   }

   return eSUCCESS;
}

int do_pray(struct char_data *ch, char *arg, int cmd)
{
  char buf1[MAX_STRING_LENGTH];
  struct descriptor_data *i;

  if(IS_NPC(ch))
    return eSUCCESS;

  while(*arg == ' ')
    arg++;

  if(!*arg) {
    send_to_char("You must have something to tell the immortals...\n\r", ch);
    return eSUCCESS;
  }

  if (GET_LEVEL(ch) >= IMMORTAL) {
    send_to_char("Why pray? You are a god!\n\r", ch);
    return eSUCCESS;
   }

  if (!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_STUPID))
    {
    send_to_char ("Duh...I'm too stupid!\n\r", ch);
    return eSUCCESS;
    }

  if (!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_NOPRAY))
  {
    send_to_char("The gods are deaf to your prayers.\n\r", ch);
    return eSUCCESS;
  }

  sprintf(buf1, "\a$4$B**$R$5 %s prays: %s $4$B**$R\n\r", GET_NAME(ch), arg);

  for(i = descriptor_list; i; i = i->next) {
    if((i->character == NULL) || (GET_LEVEL(i->character) <= MORTAL))
      continue;
    if(!(IS_SET(i->character->misc, LOG_PRAYER)))
      continue;
    if(is_busy(i->character) || is_ignoring(i->character, ch))
      continue;
    if(!i->connected)
      send_to_char(buf1, i->character);
  }
  send_to_char("\a\aOk.\n\r", ch);
  WAIT_STATE(ch, PULSE_VIOLENCE*2);
  return eSUCCESS;
}

int do_gossip(struct char_data *ch, char *argument, int cmd)
{
    char buf2[MAX_STRING_LENGTH];
    struct descriptor_data *i;
    OBJ_DATA *tmp_obj;
    bool silence = FALSE;

    if (IS_SET(world[ch->in_room].room_flags, QUIET)) {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eSUCCESS;
      }

    for(tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
      if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
        send_to_char("The magical silence prevents you from speaking!\n\r", ch);
        return eFAILURE;
      }

    if(IS_NPC(ch) && ch->master) {
      do_say(ch, "Why don't you just do that yourself!", 9);
      return eSUCCESS;
    }

    if(GET_POS(ch) == POSITION_SLEEPING) {
      send_to_char("You're asleep.  Dream or something....\r\n", ch);
      return eSUCCESS;
    }

    if(!IS_NPC(ch)) {
      if(!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_SILENCED)) {
	  send_to_char("You must have somehow offended the gods, for "
                       "you find yourself unable to!\n\r", ch);
	  return eSUCCESS;
      }
      if(!(IS_SET(ch->misc, CHANNEL_GOSSIP))) {
	  send_to_char("You told yourself not to GOSSIP!!\n\r", ch);
	  return eSUCCESS;
      }
      if(GET_LEVEL(ch) < 3) {
        send_to_char("You must be at least 3rd level to gossip.\n\r",ch);
        return eSUCCESS;
      }
    }

    for (; *argument == ' '; argument++);

    if (!(*argument))
    {
      queue<channel_msg> msgs = gossip_history;
      send_to_char("Here are the last 10 gossips:\n\r", ch);
      while(!msgs.empty())
      {
	act(msgs.front().get_msg(ch->level), ch, 0, ch, TO_VICT, 0);
	msgs.pop();
      }
    }
    else 
    {
      GET_MOVE(ch) -= 5;
      if(GET_MOVE(ch) < 0) {
        send_to_char("You're too out of breath!\n\r", ch);
        GET_MOVE(ch) += 5;
        return eSUCCESS;
      }

      channel_msg msg(ch, CHANNEL_GOSSIP, argument);

      sprintf(buf2, "$5$BYou gossip '%s'$R", argument);
      act(buf2, ch, 0, 0, TO_CHAR, 0);

      gossip_history.push(msg);
      if(gossip_history.size() > 10) {
	gossip_history.pop();
      }

      for(i = descriptor_list; i; i = i->next) {
	 if(i->character != ch && !i->connected && (IS_SET(i->character->misc, CHANNEL_GOSSIP)) && !is_ignoring(i->character, ch)) {
	   for(tmp_obj = world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content) {
	     if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
	       silence = TRUE;
	       break;
	     }
	   }

	   if(!silence) {
	     act(msg.get_msg(i->character->level), ch, 0, i->character, TO_VICT, 0);
	   }
	 }
      }
    }
    return eSUCCESS;
}

int do_auction(struct char_data *ch, char *argument, int cmd)
{
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    struct descriptor_data *i;
    OBJ_DATA *tmp_obj;
    bool silence = FALSE;

   if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
     send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n",
                   ch);
     return eSUCCESS; 
   }

   for(tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
     if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
       send_to_char("The magical silence prevents you from speaking!\n\r", ch);
       return eFAILURE;
     }

    if(IS_NPC(ch) && ch->master) {
     do_say(ch, "That's okay, I'll let you do all the auctioning, master.", 9);
     return eSUCCESS;
    }

    if(!IS_NPC(ch)) {
      if(!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_SILENCED)) {
	  send_to_char("You must have somehow offended the gods, for "
                       "you find yourself unable to!\n\r", ch);
	  return eSUCCESS;
      }
      if(!(IS_SET(ch->misc, CHANNEL_AUCTION))) {
	  send_to_char("You told yourself not to AUCTION!!\n\r", ch);
	  return eSUCCESS;
      }
      if(GET_LEVEL(ch) < 3) {
        send_to_char("You must be at least 3rd level to auction.\n\r",ch);
        return eSUCCESS;
      }
    }
    
    for (; *argument == ' '; argument++);

    if (!(*argument))
    {
      queue<string> tmp = auction_history;
      send_to_char("Here are the last 10 auctions:\n\r", ch);
      while(!tmp.empty())
      {
	  act((tmp.front()).c_str(), ch, 0, ch, TO_VICT, 0);
          tmp.pop();
      }
    }
    else {
      GET_MOVE(ch) -= 5;
      if(GET_MOVE(ch) < 0) {
        send_to_char("You're too out of breath!\n\r", ch);
        GET_MOVE(ch) += 5;
        return eSUCCESS;
      }
      sprintf(buf1, "$6$B%s auctions '%s'$R", GET_NAME(ch), argument);
      sprintf(buf2, "$6$BYou auction '%s'$R", argument);
      act(buf2, ch, 0, 0, TO_CHAR, 0);

      auction_history.push(buf1);
      if(auction_history.size() > 10) auction_history.pop();

      for(i = descriptor_list; i; i = i->next)
	 if(i->character != ch && !i->connected &&
	   (IS_SET(i->character->misc, CHANNEL_AUCTION)) &&
            !is_ignoring(i->character, ch)) {
          for(tmp_obj = world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
            if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
              silence = TRUE;
              break;
            }
          if(!silence) act(buf1, ch, 0, i->character, TO_VICT, 0);
         }
    }
    return eSUCCESS;
}

int do_shout(struct char_data *ch, char *argument, int cmd)
{
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    struct descriptor_data *i;
    OBJ_DATA *tmp_obj;
    bool silence = FALSE;

    if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
      send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n",
                   ch);
      return eSUCCESS;
    }

    for(tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
      if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
        send_to_char("The magical silence prevents you from speaking!\n\r", ch);
        return eFAILURE;
      }

    if(IS_NPC(ch) && ch->master) {
      return do_say(ch, "Shouting makes my throat hoarse.", 9);
    }

    if (!IS_NPC(ch) && IS_SET(ch->pcdata->punish, PUNISH_SILENCED)) {
	send_to_char("You must have somehow offended the gods, for you "
	             "find yourself unable to!\n\r", ch);
	return eSUCCESS;
    }
    if(!IS_NPC(ch) && !(IS_SET(ch->misc, CHANNEL_SHOUT))) {
      send_to_char("You told yourself not to SHOUT!!\n\r", ch);
      return eSUCCESS;
    }
    if (!IS_NPC(ch) && GET_LEVEL(ch) < 3){
      send_to_char("Due to misuse, you must be of at least 3rd level "
                   "to shout.\n\r",ch);
      return eSUCCESS;
    }
    
    for(; *argument == ' '; argument++);

    if(!(*argument))
      send_to_char("What do you want to shout, dork?\n\r", ch);

    else {
      sprintf(buf1, "$B$n shouts '%s'$R", argument);
      sprintf(buf2, "$BYou shout '%s'$R", argument);
      act(buf2, ch, 0, 0, TO_CHAR, 0);

      for(i = descriptor_list; i; i = i->next)
	 if(i->character != ch && !i->connected &&
           (world[i->character->in_room].zone == world[ch->in_room].zone) &&
	   (IS_NPC(i->character) || IS_SET(i->character->misc, CHANNEL_SHOUT)) &&
            !is_ignoring(i->character, ch)) {
          for(tmp_obj = world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
            if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
              silence = TRUE;
              break;
            }
          if(!silence) act(buf1, ch, 0, i->character, TO_VICT, 0);
         }
    }
    return eSUCCESS;
}

int do_trivia(struct char_data *ch, char *argument, int cmd)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  OBJ_DATA *tmp_obj;
  bool silence = FALSE;

  if (IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eSUCCESS;
  }
 
  for(tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  if(IS_NPC(ch) && ch->master) {
    return do_say(ch, "Why don't you just do that yourself!", 9);
  }

  if(!IS_NPC(ch)) {
    if(IS_SET(ch->pcdata->punish, PUNISH_SILENCED)) {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\n\r", ch);
      return eSUCCESS;
    }
    if(!(IS_SET(ch->misc, CHANNEL_TRIVIA))) {
      send_to_char("You told yourself not to listen to Trivia!!\n\r", ch);
      return eSUCCESS;
    }
    if(GET_LEVEL(ch) < 3) {
      send_to_char("You must be at least 3rd level to participate in "
                   "trivia.\n\r",ch);
      return eSUCCESS;
    }
  }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument)) {
    {
      queue<string> tmp = trivia_history;
      send_to_char("Here are the last 10 messages:\n\r", ch);
      while(!tmp.empty())
      {
	  act((tmp.front()).c_str(), ch, 0, ch, TO_VICT, 0);
          tmp.pop();
      }
    }
    return eSUCCESS;
  }

  GET_MOVE(ch) -= 5;
  if(GET_MOVE(ch) < 0) {
    send_to_char("You're too out of breath!\n\r", ch);
    GET_MOVE(ch) += 5;
    return eSUCCESS;
  }

  if(GET_LEVEL(ch) >= 102) {
    sprintf(buf1, "$3$BQuestion $R$3(%s)$B: '%s'$R", GET_SHORT(ch), argument);
    sprintf(buf2, "$3$BYou ask, $R$3'%s'$R", argument); 
  }
  else {
    sprintf(buf1, "$3$B%s answers '%s'$R", GET_SHORT(ch), argument);
    sprintf(buf2, "$3$BYou answer '%s'$R", argument);
  }
  act(buf2, ch, 0, 0, TO_CHAR, 0);

  trivia_history.push(buf1);
  if(trivia_history.size() > 10) trivia_history.pop();
    
  for(i = descriptor_list; i; i = i->next)
    if(i->character != ch && !i->connected && 
       (IS_SET(i->character->misc, CHANNEL_TRIVIA)) &&
            !is_ignoring(i->character, ch)) {
          for(tmp_obj = world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
            if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
              silence = TRUE;
              break;
            }
          if(!silence) act(buf1, ch, 0, i->character, TO_VICT, 0);
    }

  return eSUCCESS;
}

int do_dream(struct char_data *ch, char *argument, int cmd)
{
    char buf1[MAX_STRING_LENGTH] = {0};
    char buf2[MAX_STRING_LENGTH] = {0};
    struct descriptor_data *i = nullptr;
    int ctr = 0;

    if((GET_POS(ch) != POSITION_SLEEPING) && (GET_LEVEL(ch) < MIN_GOD))
    {
      send_to_char("How are you going to dream if you're awake?\n\r", ch);
      return eSUCCESS;
    }

    if(IS_NPC(ch) && ch->master) {
      do_say(ch, "Why don't you just do that yourself!", 9);
      return eSUCCESS;
    }
    if(!IS_NPC(ch)) 
      if(IS_SET(ch->pcdata->punish, PUNISH_SILENCED)) {
	  send_to_char("You must have somehow offended the gods, for "
                       "you find yourself unable to!\n\r", ch);
	  return eSUCCESS;
      }
    if(!(IS_SET(ch->misc, CHANNEL_DREAM))) {
      send_to_char("You told yourself not to dream!!\n\r", ch);
      return eSUCCESS;
    }

    if(GET_LEVEL(ch) < 3) {
      send_to_char("You must be at least 3rd level to dream.\n\r",ch);
      return eSUCCESS;
    }

    for(ctr = 0; (unsigned) ctr <= strlen(argument); ctr++) {
      if(argument[ctr] == '$') {
        argument[ctr] = ' ';
      }
      if((argument[ctr] == '?') && (argument[ctr + 1] == '?')) {
        argument[ctr] = ' ';
      }
    }

    for (; *argument == ' '; argument++);

    if (!(*argument))
      send_to_char("It must not have been that great!!\n\r", ch);
    else {
      sprintf(buf1, "$6%s dreams '$B$1%s$R$6'$R\n\r", GET_SHORT(ch), argument);
      sprintf(buf2, "$6You dream '$B$1%s$R$6'$R\n\r", argument);
      send_to_char(buf2, ch);
      for(i = descriptor_list; i; i = i->next)  {
	  if((i->character != ch)  &&
	     (!i->connected) &&
              !is_ignoring(i->character, ch) &&
    	      (IS_SET(i->character->misc, CHANNEL_DREAM)) &&
	        ((GET_POS(i->character) == POSITION_SLEEPING) ||
	         (GET_LEVEL(i->character) >= MIN_GOD)))
		   send_to_char(buf1, i->character);
      }
    }
    return eSUCCESS;
}

int do_tell(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict = nullptr;
  char name[200] = {0}, message[200] = {0}, buf[200] = {0};
  OBJ_DATA *tmp_obj =  nullptr;

  if (!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_NOTELL))
  {
    send_to_char("Your message didn't get through!!\n\r", ch);
    return eSUCCESS;
  }

  for (tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  if (!IS_MOB(ch) && !IS_SET(ch->misc, CHANNEL_TELL))
  {
    send_to_char("You have tell channeled off!!\n\r", ch);
    return eSUCCESS;
  }

  half_chop(argument, name, message);
  // these don't do anything that i can see -pir
  name[199] = '\0';
  message[199] = '\0';

  if (!*name || !*message)
  {
    if (ch->pcdata->tell_history == nullptr || ch->pcdata->tell_history->empty())
    {
      send_to_char("You have not sent or recieved any tell messages.\r\n", ch);
      return eSUCCESS;
    }

    send_to_char("Here are the last 10 tell messages:\r\n", ch);
    queue<string> tmp = *(ch->pcdata->tell_history);
    while (!tmp.empty())
    {
      string buf = tmp.front();
      buf += "\r\n";
      send_to_char(buf, ch);
      tmp.pop();
    }

    return eSUCCESS;
  }

  if (cmd == 9999)
  {
    if (!(vict = get_active_pc(name)))
    {
      send_to_char("They seem to have left!\n\r", ch);
      return eSUCCESS;
    }
    cmd = 9;
  }
  else if (!(vict = get_active_pc_vis(ch, name)))
  {
    vict = get_pc_vis(ch, name);
    if ((vict != NULL) && GET_LEVEL(vict) >= IMMORTAL)
    {
      send_to_char("That person is busy right now.\r\n", ch);
      send_to_char("Your message has been saved.\r\n", ch);
      sprintf(buf, "$2$B%s told you, '%s'$R\r\n", PERS(ch, vict), message);
      record_msg(buf, vict);
      sprintf(buf, "$2$B%s told you, '%s'$R", PERS(ch, vict), message);
      vict->tell_history(buf);

      sprintf(buf, "$2$BYou told %s, '%s'$R", GET_SHORT(vict), message);
      ch->tell_history(buf);
    }
    else
    {
      send_to_char("No-one by that name here.\n\r", ch);
    }

    return eSUCCESS;
  }

  // vict guarantted to be a PC
  // Re: Last comment. Switched immortals crash this.

  if (!IS_NPC(vict) && !IS_SET(vict->misc, CHANNEL_TELL) && ch->level <= MAX_MORTAL)
  {
    send_to_char("The person is ignoring all tells right now.\r\n", ch);
    return eSUCCESS;
  }
  else if (!IS_NPC(vict) && !IS_SET(vict->misc, CHANNEL_TELL))
  {
    // Immortal sent a tell to a player with NOTELL.  Allow the tell butnotify the imm.
    send_to_char("That player has tell channeled off btw...\r\n", ch);
  }
  if (ch == vict)
    send_to_char("You try to tell yourself something.\n\r", ch);
  else if ((GET_POS(vict) == POSITION_SLEEPING || IS_SET(world[vict->in_room].room_flags, QUIET)) && GET_LEVEL(ch) < IMMORTAL)
    act("Sorry, $E cannot hear you.", ch, 0, vict, TO_CHAR, STAYHIDE);
  else
  {
    for (tmp_obj = world[vict->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
      if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
      {
        act("$E cannot hear you right now.", ch, 0, vict, TO_CHAR, STAYHIDE);
        return eSUCCESS;
      }
    if (is_ignoring(vict, ch))
    {
      csendf(ch, "%s is ignoring you right now.\n\r", GET_SHORT(vict));
      return eSUCCESS;
    }
    if (is_busy(vict) && GET_LEVEL(ch) >= OVERSEER)
    {
      if (IS_MOB(vict))
      {
        sprintf(buf, "%s tells you, '%s'", PERS(ch, vict), message);
      }
      else
      {
        sprintf(buf, "%s tells you, '%s'%c", PERS(ch, vict), message, IS_SET(vict->pcdata->toggles, PLR_BEEP) ? '\a' : '\0');
        if (!IS_NPC(ch) && !IS_NPC(vict) && vict->pcdata->last_tell)
          dc_free(vict->pcdata->last_tell);
        if (!IS_NPC(ch) && !IS_NPC(vict))
          vict->pcdata->last_tell = str_dup(GET_NAME(ch));
      }

      ansi_color(GREEN, vict);
      ansi_color(BOLD, vict);
      send_to_char_regardless(buf, vict);
      ansi_color(NTEXT, vict);

      sprintf(buf, "$2$B%s tells you, '%s'$R", PERS(ch, vict), message);
      vict->tell_history(buf);

      sprintf(buf, "$2$BYou tell %s, '%s'$R", PERS(vict, ch), message);
      send_to_char(buf, ch);
    }
    else if (!is_busy(vict) && GET_POS(vict) > POSITION_SLEEPING)
    {
      if (IS_MOB(vict))
      {
        sprintf(buf, "$2$B%s tells you, '%s'$R", PERS(ch, vict), message);
      }
      else
      {
        sprintf(buf, "$2$B%s tells you, '%s'$R%c", PERS(ch, vict), message, IS_SET(vict->pcdata->toggles, PLR_BEEP) ? '\a' : '\0');
        if (vict->pcdata->last_tell && !IS_NPC(ch) && !IS_NPC(vict))
          dc_free(vict->pcdata->last_tell);
        if (!IS_NPC(ch) && !IS_NPC(vict))
          vict->pcdata->last_tell = str_dup(GET_NAME(ch));
      }
      act(buf, vict, 0, 0, TO_CHAR, STAYHIDE);

      sprintf(buf, "$2$B%s tells you, '%s'$R", PERS(ch, vict), message);
      vict->tell_history(buf);

      sprintf(buf, "$2$BYou tell %s, '%s'$R", PERS(vict, ch), message);
      act_return ar = act(buf, ch, 0, 0, TO_CHAR, STAYHIDE);
      ch->tell_history(ar.str);

      // Log what I told a logged player under their name
      if (!IS_MOB(vict) && IS_SET(vict->pcdata->punish, PUNISH_LOG))
      {
        sprintf(log_buf, "Log %s: %s told them: %s", GET_NAME(vict),
                GET_NAME(ch), message);
        log(log_buf, IMP, LOG_PLAYER, vict);
      }
    }
    else if (!is_busy(vict) && GET_POS(vict) == POSITION_SLEEPING &&
             GET_LEVEL(ch) >= SERAPH)
    {
      send_to_char("A heavenly power intrudes on your subconcious dreaming...\r\n", vict);
      if (IS_MOB(vict))
        sprintf(buf, "%s tells you, '%s'", PERS(ch, vict), message);
      else
      {
        sprintf(buf, "%s tells you, '%s'%c", PERS(ch, vict), message, IS_SET(vict->pcdata->toggles, PLR_BEEP) ? '\a' : '\0');

        if (vict->pcdata->last_tell && !IS_NPC(ch) && !IS_NPC(vict))
          dc_free(vict->pcdata->last_tell);
        if (!IS_NPC(ch) && !IS_NPC(vict))
          vict->pcdata->last_tell = str_dup(GET_NAME(ch));
      }
      ansi_color(GREEN, vict);
      ansi_color(BOLD, vict);
      send_to_char_regardless(buf, vict);
      ansi_color(NTEXT, vict);

      sprintf(buf, "$2$B%s tells you, '%s'$R", PERS(ch, vict), message);
      vict->tell_history(buf);

      sprintf(buf, "$2$BYou tell %s, '%s'$R", PERS(vict, ch), message);
      act_return ar = act(buf, ch, 0, 0, TO_CHAR, STAYHIDE);
      ch->tell_history(ar.str);

      send_to_char("They were sleeping btw...\r\n", ch);
      // Log what I told a logged player under their name
      if (!IS_MOB(vict) && IS_SET(vict->pcdata->punish, PUNISH_LOG))
      {
        sprintf(log_buf, "Log %s: %s told them: %s", GET_NAME(vict),
                GET_NAME(ch), message);
        log(log_buf, IMP, LOG_PLAYER, vict);
      }
    }
    else
    {
      sprintf(buf, "$2$B%s can't hear anything right now.$R", GET_SHORT(vict));
      act(buf, ch, 0, 0, TO_CHAR, STAYHIDE);
    }
  }
  return eSUCCESS;
}

int do_reply(struct char_data *ch, char *argument, int cmd)
{
  char buf[200];
  char_data *vict = NULL;

  if (IS_MOB(ch) || !ch->pcdata->last_tell)
  {
    send_to_char("You have noone to reply to.\n\r", ch);
    return eSUCCESS;
  }

  OBJ_DATA *tmp_obj;
  for (tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  for (; *argument == ' '; argument++)
    ;

  if (!(*argument))
  {
    send_to_char("Reply what?\n\r", ch);
    if ((vict = get_char(ch->pcdata->last_tell)) && CAN_SEE(ch, vict))
      sprintf(buf, "Last tell was from %s.\n\r", ch->pcdata->last_tell);
    else
      sprintf(buf, "Last tell was from someone you cannot currently see.\n\r");
    send_to_char(buf, ch);
    return eSUCCESS;
  }

  sprintf(buf, "%s %s", ch->pcdata->last_tell, argument);
  do_tell(ch, buf, 9999);
  return eSUCCESS;
}

int do_whisper(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  char name[MAX_INPUT_LENGTH + 1], message[MAX_STRING_LENGTH],
      buf[MAX_STRING_LENGTH];

  OBJ_DATA *tmp_obj;
  for (tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char("Who do you want to whisper to.. and what??\n\r", ch);
  else if (!(vict = get_char_room_vis(ch, name)))
    send_to_char("No-one by that name here..\n\r", ch);
  else if (vict == ch)
  {
    act("$n whispers quietly to $mself.", ch, 0, 0, TO_ROOM, STAYHIDE);
    send_to_char(
        "You can't seem to get your mouth close enough to your ear...\n\r", ch);
  }
  else if (is_ignoring(vict, ch))
  {
    send_to_char("They are ignoring you :(\n\r", ch);
  }
  else
  {
    sprintf(buf, "$1$B$n whispers to you, '%s'$R", message);
    act_return ar = act(buf, ch, 0, vict, TO_VICT, STAYHIDE);
    vict->tell_history(ar.str);

    sprintf(buf, "$1$BYou whisper to $N, '%s'$R", message);
    ar = act(buf, ch, 0, vict, TO_CHAR, STAYHIDE);
    ch->tell_history(ar.str);

    act("$n whispers something to $N.", ch, 0, vict, TO_ROOM,
        NOTVICT | STAYHIDE);
  }
  return eSUCCESS;
}

int do_ask(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  char name[MAX_INPUT_LENGTH + 1], message[MAX_INPUT_LENGTH + 1], buf[MAX_STRING_LENGTH];

  OBJ_DATA *tmp_obj;
  for (tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
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
  else if (!(vict = get_char_room_vis(ch, name)))
    send_to_char("No-one by that name here.\n\r", ch);
  else if (vict == ch)
  {
    act("$n quietly asks $mself a question.", ch, 0, 0, TO_ROOM, 0);
    send_to_char("You think about it for a while...\n\r", ch);
  }
  else if (is_ignoring(vict, ch))
  {
    send_to_char("They are ignoring you :(\n\r", ch);
  }
  else
  {
    sprintf(buf, "$B$n asks you, '%s'$R", message);
    act_return ar = act(buf, ch, 0, vict, TO_VICT, 0);
    vict->tell_history(ar.str);

    sprintf(buf, "$BYou ask $N, '%s'$R", message);
    ar = act(buf, ch, 0, vict, TO_CHAR, 0);
    ch->tell_history(ar.str);

    act("$n asks $N a question.", ch, 0, vict, TO_ROOM, NOTVICT);
  }
  return eSUCCESS;
}

int do_grouptell(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *k;
  struct follow_type *f;
  OBJ_DATA *tmp_obj;
  bool silence = FALSE;

  if (ch == nullptr || ch->pcdata == nullptr)
  {
    return eFAILURE;
  }

  if (!*argument)
  {
    if (ch->pcdata->gtell_history == nullptr || ch->pcdata->gtell_history->empty()) {
      send_to_char("No one has said anything.\r\n", ch);
      return eFAILURE;
    }

    queue<string> copy = *(ch->pcdata->gtell_history);
    send_to_char("Here are the last 10 group tells:\r\n", ch);
    while(!copy.empty()) {
      csendf(ch, "%s\r\n", copy.front().c_str());
      copy.pop();
    }

    return eSUCCESS;
  }

  for (tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  for (; isspace(*argument); argument++)
    ;

  if (!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_NOTELL))
  {
    send_to_char("Your message didn't get through!!\n\r", ch);
    return eSUCCESS;
  }

  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    send_to_char("You don't have a group to talk to!\n\r", ch);
    return eSUCCESS;
  }

  if (ch->pcdata->gtell_history == nullptr)
  {
    ch->pcdata->gtell_history = new queue<string>;
  }

  sprintf(buf, "$B$1You tell the group, $7'%s'$R", argument);
  act_return ar = act(buf, ch, 0, 0, TO_CHAR, STAYHIDE | ASLEEP);
  ch->pcdata->gtell_history->push(ar.str);
  if (ch->pcdata->gtell_history->size() > 10)
  {
    ch->pcdata->gtell_history->pop();
  }

  if (!(k = ch->master))
    k = ch;

  sprintf(buf, "$B$1$n tells the group, $7'%s'$R", argument);

  if (ch->master)
  {
    act_return ar = act(buf, ch, 0, ch->master, TO_VICT, STAYHIDE | ASLEEP);

    if (ch->master && ch->master->pcdata)
    {
      if (ch->master->pcdata->gtell_history == nullptr)
      {
          ch->master->pcdata->gtell_history = new queue<string>;
      }
      ch->master->pcdata->gtell_history->push(ar.str);
      if (ch->master->pcdata->gtell_history->size() > 10)
      {
        ch->master->pcdata->gtell_history->pop();
      }
    }
  }

  for (f = k->followers; f; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) && (f->follower != ch))
    {
      for (tmp_obj = world[f->follower->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
      {
        if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
        {
          silence = TRUE;
          break;
        }
      }
      if (!silence)
      {
        act_return ar = act(buf, ch, 0, f->follower, TO_VICT, STAYHIDE | ASLEEP);
        if (f->follower && f->follower->pcdata)
        {
          if (f->follower->pcdata->gtell_history == nullptr)
          {
             f->follower->pcdata->gtell_history = new queue<string>;
          }
          f->follower->pcdata->gtell_history->push(ar.str);
          if (f->follower->pcdata->gtell_history->size() > 10)
          {
            f->follower->pcdata->gtell_history->pop();
          }
        }
      }
    }
  }
  return eSUCCESS;
}

int do_newbie(struct char_data *ch, char *argument, int cmd)
{
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    struct descriptor_data *i;
    OBJ_DATA *tmp_obj;
    bool silence = FALSE;

    if (IS_SET(world[ch->in_room].room_flags, QUIET)) {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n",
                    ch);
      return eSUCCESS;
      }

    for(tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
      if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
        send_to_char("The magical silence prevents you from speaking!\n\r", ch);
        return eFAILURE;
      }

    if(IS_NPC(ch) && ch->master) {
      return do_say(ch, "Why don't you just do that yourself!", 9);
    }

    if(!IS_NPC(ch)) {
      if(IS_SET(ch->pcdata->punish, PUNISH_SILENCED)) {
	  send_to_char("You must have somehow offended the gods, for "
                       "you find yourself unable to!\n\r", ch);
	  return eSUCCESS;
      }
      if(!(IS_SET(ch->misc, CHANNEL_NEWBIE))) {
	  send_to_char("You told yourself not to use the newbie channel!!\n\r", ch);
	  return eSUCCESS;
      }
    }

    for (; *argument == ' '; argument++);

    if (!(*argument))
    {
      queue<string> tmp = newbie_history;
      send_to_char("Here are the last 10 messages:\n\r", ch);
      while(!tmp.empty())
      {
	  act((tmp.front()).c_str(), ch, 0, ch, TO_VICT, 0);
          tmp.pop();
      }
    }
    else {
      GET_MOVE(ch) -= 5;
      if(GET_MOVE(ch) < 0) {
        send_to_char("You're too out of breath!\n\r", ch);
        GET_MOVE(ch) += 5;
        return eSUCCESS;
      }
      sprintf(buf1, "$5%s newbies '$R$B%s$R$5'$R", GET_SHORT(ch), argument);
      sprintf(buf2, "$5You newbie '$R$B%s$R$5'$R", argument);
      act(buf2, ch, 0, 0, TO_CHAR, 0);

      newbie_history.push(buf1);
      if(newbie_history.size() > 10) newbie_history.pop();

      for(i = descriptor_list; i; i = i->next)
	 if(i->character != ch && !i->connected && 
           !is_ignoring(i->character, ch) &&
    	   (IS_SET(i->character->misc, CHANNEL_NEWBIE)) ) {
          for(tmp_obj = world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
            if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
              silence = TRUE;
              break;
            }
          if(!silence) act(buf1, ch, 0, i->character, TO_VICT, 0);
         }
    }
    return eSUCCESS;
}

void char_data::tell_history(string message)
{
  if (this->pcdata == nullptr)
  {
    return;
  }

  if (this->pcdata->tell_history == nullptr)
  {
    this->pcdata->tell_history = new std::queue<string>();
  }

  this->pcdata->tell_history->push(message);
  if (this->pcdata->tell_history->size() > 10)
  {
    this->pcdata->tell_history->pop();
  }
}
