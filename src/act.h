#ifndef ACT_H_
#define ACT_H_
/************************************************************************
| act.h
| Written by: Morcallen
| Date: 27 July 1996
| Description:  This file contains the header information for the new
|   act() function
*/
#include <string>
#include "character.h"
#include "obj.h"
#include "token.h"
//--
// Function interface
//--

struct act_return
{
  string str;
  int retval;
};

act_return act(const char *str, CHAR_DATA *ch, OBJ_DATA *obj, void *vict_obj,
          int16 destination, int16 flags);

act_return act(const std::string &str, CHAR_DATA *ch, OBJ_DATA *obj, void *vict_obj,
	 int16 destination, int16 flags);

struct send_tokens_return
{
  string str;
  int retval;
};

send_tokens_return send_tokens(TokenList * tokens, CHAR_DATA *ch, OBJ_DATA * obj, void * vch, int flags, CHAR_DATA *to);

void send_message(const char *str, CHAR_DATA *to);

//--
// Constants
//--
// Undefine the old ones if this header is included
#undef TO_ROOM
#undef TO_VICT
#undef TO_CHAR

// These constants need to go in the destination variable
#define TO_ROOM   0   // Everyone in ch's room except ch
#define TO_VICT   1   // Just vict_obj
#define TO_CHAR   2   // Just ch
#define TO_ZONE   3   // Everyone in ch's zone except ch
#define TO_WORLD  4   // Everyone in the world except ch
#define TO_GROUP  5   // Everyone in the ch's group except ch
#define TO_ROOM_NOT_GROUP  6   // Everyone in ch's room except ch's group or ch

// These constants go in the flags part (optional -- 0 for none)
#define DEFAULT       0    // "someone" if invisible, sleepers skipped
#define NOTVICT       1<<0 // Sends to destination except victim
#define GODS          1<<1 // Sends to destination, gods only
#define ASLEEP        1<<2 // Will send even to sleepers
#define INVIS_NULL    1<<3 // Invisible messages are skipped completely
#define INVIS_VISIBLE 1<<4 // Invisible messages are shown w/names visible
#define FORCE         1<<5 // Sends regardless of nanny state
#define STAYHIDE      1<<6 // Stayhide flag keeps thieves in hiding.
#define BARDSONG      1<<7 // Bard song so only show it to people in room with BARD_SONG toggle set to verbose

#endif /* ACT_H_ */
