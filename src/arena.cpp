/*
 * Implementation of arena opening/closing and related commands
 *
 * -Sadus
 */
/*****************************************************************************/
/* Revision History                                                          */
/* 12/09/2003   Onager   Tweaked do_join() to remove combat-related bits     */
/*****************************************************************************/
/* $Id: arena.cpp,v 1.5 2003/12/09 08:40:49 staylor Exp $ */

#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <room.h>  // room_data
#include <utility.h> // send_to_char, etc.. 
#include <db.h>      // real_room()
#include <interp.h> // do_look()
#include <character.h>
#include <spells.h> 
#include <handler.h>
#include <act.h>
#include <punish.h>
#include <player.h>
#include <arena.h>
#include <string.h>
#include <returnvals.h>

/*
THIS STUFF NOW DEFINED IN ARENA.H
#define ARENA_LOW 500
#define ARENA_HI  562
*/
/*
#define ARENA_LOW 14600
#define ARENA_HI  14681
*/
int arena[4] = { 0, 0, 0, 0 };

/* External functions */
void half_chop(char *string, char *arg1, char *arg2);
void send_info(char *messg);

extern CWorld world;

int do_arena(CHAR_DATA *ch, char *arg, int cmd)
{
  char buf[256], buf2[256], buf3[256], crap[256];
  int low, high;

  if(!has_skill(ch, COMMAND_ARENA)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }

  half_chop(arg, buf, crap);
  half_chop(crap, buf2, buf3);
  if(!*buf || !*buf2) {
    sprintf(buf, "Currently open for levels: %d %d\n\r", arena[0], arena[1]);
    send_to_char(buf, ch);
    send_to_char("Syntax: arena <lowest level> <highest level> <Num Mortals>\n\r"
                 "Use arena 0 0 to close the arena.\n\r"
                 "Do *NOT* leave the arena open, people will be stuck there"
                 " forever!\n\r"
                 "Also the arena can be opened without specifying the number"
                 " of mortals allowed to join.", ch);
    return eSUCCESS;
  }

  if(!(low = atoi(buf)) || !(high = atoi(buf2)) || low > high) {
    arena[0] = 0;
    arena[1] = 0;
    send_to_char("Closing the arena.\n\r", ch);
    send_info("## The Arena has been CLOSED!\n\r");
//    arena[2] = 0; commented out for chaos purposes.
// shouldn't effect anything since we always asign it below
    arena[3] = 0;
    return eSUCCESS;
  }

  arena[0] = low;
  arena[1] = high;
  sprintf(buf, "## The Arena has been OPENED for levels %d - %d.\n\r"
               "## Type JOINARENA to enter the Bloodbath!\n\r",
          low, high);
  send_info(buf);
  arena[2] = -1;
  if(*buf3) {
      arena[2] = atoi(buf3);
      if(!strcmp(buf3, "chaos"))
        arena[2] = -2;
      if(arena[2] > 0) {
          sprintf(buf, "## Only %d can join the bloodbath!\n\r", arena[2]);
          send_info(buf);
      }
      if(arena[2] == -2) {
          sprintf(buf, "## Only clan members can join the bloodbath!\r\n");
          send_info(buf);
          logf(111, LOG_CHAOS, "%s started a CC.", GET_NAME(ch));
      }
  }
  send_to_char("The Arena has been opened for the specified levels.\n\r", ch); 
  return eSUCCESS;
}
 
int do_joinarena(CHAR_DATA *ch, char *arg, int cmd)
{
  char buf[256];
  int send_to = NOWHERE;
  struct affected_type *af, *next_af;
  if(arena[0] > GET_LEVEL(ch) || arena[1] < GET_LEVEL(ch)) {
    send_to_char("The arena is not open for anyone your level.\n\r", ch);
    return eFAILURE;
  }
  if(!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_NOARENA)) {
    send_to_char("You have been banned from arenas.\n\r", ch);
    return eFAILURE;
  }
  if(arena[2] == -2 && !ch->clan) {
    send_to_char("Only clan members may join this arena.\r\n", ch);
    return eFAILURE;
  }
  if(IS_SET(world[ch->in_room].room_flags, ARENA)) {
    send_to_char("You are already there!\n\r", ch);
    return eFAILURE; 
  }
  if((arena[2] > 0) && (arena[3] >= arena[2])) {
    send_to_char("The arena is already full!\n\r", ch);
    return eFAILURE;
  }
  if(ch->fighting) {
    send_to_char("You're ALREADY in a fight...isn't that kinda silly?\r\n", ch);
    return eFAILURE;
  }

  if(GET_POS(ch) == POSITION_SLEEPING) {
    affect_from_char(ch, INTERNAL_SLEEPING);
    do_wake(ch, "", 9);
  }

  arena[3]++;
  for(af = ch->affected; af; af = next_af) {
     next_af = af->next;
     if(af->type != FUCK_CANTQUIT)
       affect_remove(ch, af, SUPPRESS_ALL);
  }
  
  /* remove combat-related bits */
  ch->combat = 0;

  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_HIT(ch)  = GET_MAX_HIT(ch);
  GET_KI(ch)   = GET_MAX_KI(ch);
 
  act("$n disappears in a glorious flash of heroism.", ch, 0, 0,
      TO_ROOM, 0);
  while(send_to == NOWHERE)
  {
    send_to = real_room(number(ARENA_LOW, ARENA_HI));
  }
  if(move_char(ch, send_to) == 0) return eFAILURE;
  act("$n appears, preparing for battle.", ch, 0, 0, TO_ROOM, 0);
  sprintf(buf, "## %s has joined the bloodbath!\n\r", GET_SHORT(ch));
  send_info(buf);
  do_look(ch, "", 8);
  return eSUCCESS;
}

bool ArenaIsOpen() {
   if((arena[0] != 0) && (arena[1] != 0)) {
      return(true);
   }
   return(false);
}
