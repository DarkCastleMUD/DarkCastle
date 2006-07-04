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

#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <comm.h>
#include <character.h>
#include <levels.h>
#include <db.h>
#include <room.h>
#include <utility.h>
#include <player.h>
#include <terminal.h>
#include <handler.h>
#include <obj.h>
#include <machine.h>
#include <connect.h>
#include <act.h>
#include <mobile.h>
#include <token.h>
#include <spells.h>

extern CWorld world;
extern struct descriptor_data *descriptor_list;
extern bool MOBtrigger;

void send_message(TokenList * tokens, CHAR_DATA *ch, OBJ_DATA * obj, void * vch, int flags, CHAR_DATA *to);
void send_message(const char *str, CHAR_DATA *to);


void act
(
  char *str,          // Buffer
  CHAR_DATA *ch,      // Character from
  OBJ_DATA *obj,      // Object
  void *vict_obj,     // Victim object
  int16 destination,  // Destination flags
  int16 flags         // Optional flags
)
{
  struct descriptor_data *i;
  
  TokenList *tokens;
  
  tokens = new TokenList(str);
  
  // This shouldn't happen
  if (ch == 0) {
    log("Error in act(), character equal to 0", OVERSEER, LOG_BUG);
    delete tokens;
    return;
    }

  if ((IS_AFFECTED(ch, AFF_HIDE) || ISSET(ch->affected_by, AFF_FOREST_MELD)) &&
    (destination != TO_CHAR) && !(flags & GODS) && !(flags & STAYHIDE)) {
    REMBIT(ch->affected_by, AFF_HIDE);
    affect_from_char(ch, SPELL_FOREST_MELD);
    }
    
  if (destination == TO_VICT) {
    send_message(tokens, ch, obj, vict_obj, flags, (CHAR_DATA *)vict_obj);
    }
  else if(destination == TO_CHAR) {
    send_message(tokens, ch, obj, vict_obj, flags, ch);
    }
  else if(destination == TO_ROOM) {
    char_data * tmp_char, *next_tmp_char;
    for (tmp_char = world[ch->in_room].people; tmp_char; tmp_char = next_tmp_char) {
      next_tmp_char = tmp_char->next_in_room;
      // If they're not really playing, and no force flag, don't send
      if (tmp_char == ch)
        continue;
      if (tmp_char->position > POSITION_SLEEPING || IS_SET(flags, ASLEEP))
        send_message(tokens, ch, obj, vict_obj, flags, tmp_char);
      }
    }
  // TO_ZONE, TO_WORLD
  else {
    if (destination != TO_ZONE && destination != TO_WORLD) {
      log("Error in act(), invalid value sent as 'destination'", OVERSEER, LOG_BUG);
      delete tokens;
      return;
      }
    for (i = descriptor_list; i; i = i->next) {
      // Dropped link or they're not really playing and no force flag, don't send.
      if (!i->character || i->character == ch)
        continue;
      if ((destination == TO_ZONE) && world[i->character->in_room].zone != world[ch->in_room].zone)
        continue;
      send_message(tokens, ch, obj, vict_obj, flags, i->character);
      }
    }
      
   delete tokens;
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


void send_message(TokenList * tokens, CHAR_DATA *ch, OBJ_DATA * obj, void * vict_obj, int flags, CHAR_DATA *to)
{
  char * buf = tokens->Interpret(ch, obj, vict_obj, to, flags);

  send_message(buf, to);
  
  if (MOBtrigger && buf)
    mprog_act_trigger( buf, to, ch, obj, vict_obj );
  if (MOBtrigger && buf)
    oprog_act_trigger( buf, ch);
    
  MOBtrigger = TRUE;
}


// The End

