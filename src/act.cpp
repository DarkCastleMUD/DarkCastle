/************************************************************************
| act.C
| Description:  This file contains the classes and methods used to make
|  the act() function work.
*/
extern "C"
{
#include <ctype.h>
};

extern "C" {
#include <string.h>
#include <ctype.h>
}

#include <string>

#include "comm.h"
#include "character.h"
#include "levels.h"
#include "db.h"
#include "room.h"
#include "utility.h"
#include "player.h"
#include "terminal.h"
#include "handler.h"
#include "obj.h"
#include "machine.h"
#include "connect.h"
#include "act.h"
#include "mobile.h"
#include "token.h"
#include "spells.h"
#include "returnvals.h"

using namespace std;

extern CWorld world;
extern struct descriptor_data *descriptor_list;
extern bool MOBtrigger;

act_return act(const string &str, CHAR_DATA *ch, OBJ_DATA *obj, void *vict_obj, int16 destination, int16 flags)
{
  return act(str.c_str(), ch, obj, vict_obj, destination, flags);
}

act_return act(
    const char *str,   // Buffer
    CHAR_DATA *ch,     // Character from
    OBJ_DATA *obj,     // Object
    void *vict_obj,    // Victim object
    int16 destination, // Destination flags
    int16 flags        // Optional flags
)
{
  struct descriptor_data *i;
  int retval = 0;
  TokenList *tokens;

  send_tokens_return st_return;
  st_return.str = string();
  st_return.retval = 0;
  
  act_return ar;
  ar.str = string();
  ar.retval = 0;

  tokens = new TokenList(str);

  // This shouldn't happen
  if (ch == 0)
  {
    log("Error in act(), character equal to 0", OVERSEER, LOG_BUG);
    delete tokens;
    ar.retval = eFAILURE;
    return ar;
  }

  if (
      (IS_AFFECTED(ch, AFF_HIDE) || ISSET(ch->affected_by, AFF_FOREST_MELD)) && (destination != TO_CHAR) && (destination != TO_GROUP) && !(flags & GODS) && !(flags & STAYHIDE))
  {
    REMBIT(ch->affected_by, AFF_HIDE);
    affect_from_char(ch, SPELL_FOREST_MELD);
  }

  if (destination == TO_VICT)
  {
    st_return = send_tokens(tokens, ch, obj, vict_obj, flags, (CHAR_DATA *)vict_obj);
    retval |= st_return.retval;
    ar.str = st_return.str;
    ar.retval = retval;
  }
  else if (destination == TO_CHAR)
  {
    st_return = send_tokens(tokens, ch, obj, vict_obj, flags, ch);
    retval |= st_return.retval;
    ar.str = st_return.str;
    ar.retval = retval;
  }
  else if (destination == TO_ROOM || destination == TO_GROUP || destination == TO_ROOM_NOT_GROUP)
  {
    char_data *tmp_char, *next_tmp_char;
    if (ch->in_room >= 0)
    {
      for (tmp_char = world[ch->in_room].people; tmp_char; tmp_char = next_tmp_char)
      {
        next_tmp_char = tmp_char->next_in_room;
        // If they're not really playing, and no force flag, don't send
        if (tmp_char == ch)
          continue;
        if (destination == TO_GROUP && !ARE_GROUPED(tmp_char, ch))
          continue;
        if (destination == TO_ROOM_NOT_GROUP && ARE_GROUPED(tmp_char, ch))
        {
          continue;
        }
        if (tmp_char->position > POSITION_SLEEPING || IS_SET(flags, ASLEEP))
        {
          if (!IS_SET(flags, BARDSONG) || tmp_char->pcdata == nullptr || !IS_SET(tmp_char->pcdata->toggles, PLR_BARD_SONG))
          {
            st_return = send_tokens(tokens, ch, obj, vict_obj, flags, tmp_char);
            retval |= st_return.retval;
            ar.str = st_return.str;
            ar.retval = retval;
          }
        }
      }
    }
  }
  // TO_ZONE, TO_WORLD
  else
  {
    if (destination != TO_ZONE && destination != TO_WORLD)
    {
      log("Error in act(), invalid value sent as 'destination'", OVERSEER, LOG_BUG);
      delete tokens;
      ar.retval = eFAILURE;
      return ar;
    }
    for (i = descriptor_list; i; i = i->next)
    {
      // Dropped link or they're not really playing and no force flag, don't send.
      if (!i->character || i->character == ch)
        continue;
      if (i->character->in_room < 0 || ch->in_room < 0)
        continue;
      if ((destination == TO_ZONE) && world[i->character->in_room].zone != world[ch->in_room].zone)
        continue;
      st_return = send_tokens(tokens, ch, obj, vict_obj, flags, i->character);
      retval |= st_return.retval;
      ar.str = st_return.str;
      ar.retval = retval;
    }
  }

  delete tokens;
  return ar;
}

/************************************************************************
| void send_message()
| Description:  This function just sends the message to the character,
|   with no interpretation and no checks.
*/
void send_message(const char *str, CHAR_DATA *to)
{
  // This will happen when a token shouldn't be interpreted
  if(str == 0)  return;
  if(!to)       return;
  if(!to->desc) return;

  SEND_TO_Q((char *)str, to->desc);
}


send_tokens_return send_tokens(TokenList * tokens, CHAR_DATA *ch, OBJ_DATA * obj, void * vict_obj, int flags, CHAR_DATA *to)
{
  int retval = 0;
  char * buf = tokens->Interpret(ch, obj, vict_obj, to, flags);
  
  // Uppercase first letter of sentence.
  if (buf != nullptr && buf[0] != 0)
  {
    buf[0] = toupper(buf[0]);
  }
  send_message(buf, to);
  
  if (MOBtrigger && buf)
    retval |= mprog_act_trigger( buf, to, ch, obj, vict_obj );
  if (MOBtrigger && buf)
    retval |= oprog_act_trigger( buf, ch);
    
  MOBtrigger = TRUE;
  if (buf == nullptr)
  {
    send_tokens_return str;
    str.str = string();
    str.retval = retval;
    return str;
  }

  size_t buf_len = strlen(buf);
  if (buf_len >= 2)
  {
    // Remove \r\n at end before returning string
    buf[buf_len-2] = '\0';    
  }
  send_tokens_return str;
  str.str = string(buf);
  str.retval = retval;
  return str;
}


// The End

