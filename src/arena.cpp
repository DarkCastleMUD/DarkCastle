/*
 * Implementation of arena opening/closing and related commands
 *
 * -Sadus
 */
/*****************************************************************************/
/* Revision History                                                          */
/* 12/09/2003   Onager   Tweaked do_join() to remove combat-related bits     */
/*****************************************************************************/
/* $Id: arena.cpp,v 1.17 2009/04/24 21:50:43 shane Exp $ */

#include "room.h"  // room_data
#include "db.h"      // real_room()
#include "interp.h" // do_look()
#include "character.h"
#include "utility.h" // send_to_char, etc.. 
#include "spells.h" 
#include "handler.h"
#include "act.h"
#include "punish.h"
#include "player.h"
#include "arena.h"
#include <string.h>
#include "returnvals.h"
#include "levels.h"

extern CWorld world;
struct _arena arena;

int do_arena(CHAR_DATA *ch, char *argument, int cmd)
{
  char arg1[256], arg2[256], arg3[256], arg4[256], arg5[256], buf[256];
  int low, high;

  if(!has_skill(ch, COMMAND_ARENA)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);
  argument = one_argument(argument, arg4);
  argument = one_argument(argument, arg5);

  if(!*arg1 || !*arg2) {
    sprintf(buf, "Currently open for levels: %d %d\n\r", arena.low, arena.high);
    send_to_char(buf, ch);
    send_to_char("Syntax: arena <lowest level> <highest level> [num mortals] [type] [hp limit if applicable]\n\r"
		 "Valid types: chaos, potato, prize, hp\n\r"
		 "Use -1 for no limit on number of mortals.\n\r"
                 "Use arena 0 0 to close the arena.\n\r"
                 "Do *NOT* leave the arena open, people will be stuck there forever!\n\r"
                 "Also the arena can be opened without specifying the number"
                 " of mortals allowed to join.\n\r", ch);
    return eSUCCESS;
  }

  if(!(low = atoi(arg1)) || !(high = atoi(arg2)) || low > high) {
    arena.low = 0;
    arena.high = 0;
    arena.status = CLOSED;
    send_to_char("Closing the arena.\n\r", ch);
    send_info("## The Arena has been CLOSED!\n\r");
    return eSUCCESS;
  }

  arena.low = low;
  arena.high = high;
  arena.status = OPENED;
  sprintf(buf, "## The Arena has been OPENED for levels %d - %d.\n\r"
               "## Type JOINARENA to enter the Bloodbath!\n\r", low, high);
  send_info(buf);

  if(*arg3) {
    arena.num = atoi(arg3);
    if(arena.num > 0) {
      sprintf(buf, "## Only %d can join the bloodbath!\n\r", arena.num);
      send_info(buf);
    }
  } else {
    arena.num = 0;
  }

  arena.cur_num = 0;

  if (*arg4) {
    if(!strcmp(arg4, "chaos")) {
      arena.type = CHAOS; // -2
      sprintf(buf, "## Only clan members can join the bloodbath!\r\n");
      send_info(buf);
      logf(IMMORTAL, LOG_ARENA, "%s started a Clan Chaos arena.", GET_NAME(ch));
    }

    if(!strcmp(arg4, "potato")) {
      arena.type = POTATO; // -3
      sprintf(buf, "##$4$B Special POTATO Arena!!$R\r\n");
      send_info(buf);
    }

    if(!strcmp(arg4, "prize")) {
      arena.type = PRIZE; // -3
      sprintf(buf, "##$4$B Prize Arena!!$R\r\n");
      send_info(buf);
    }    

    if(!strcmp(arg4, "hp")) {
     if(*arg5) {
      arena.hplimit = atoi(arg5);
      if(arena.hplimit <= 0) arena.hplimit=1000;
     } else arena.hplimit=1000;

      arena.type = HP; // -4
      sprintf(buf, "##$4$B HP LIMIT Arena!!$R  Any more than %d raw hps, and you have to sit this one out!!\r\n", arena.hplimit);
      send_info(buf);
    }    
  } else {
    arena.type = NORMAL;
  }

  send_to_char("The Arena has been opened for the specified levels.\n\r", ch); 
  return eSUCCESS;
}
 
int do_joinarena(CHAR_DATA *ch, char *arg, int cmd)
{
  char buf[256];
  int send_to = NOWHERE;
  struct affected_type *af, *next_af;
  int pot_low = 6362;
  int pot_hi  = 6379;

  if(arena.low > GET_LEVEL(ch) || arena.high < GET_LEVEL(ch)) {
    send_to_char("The arena is not open for anyone your level.\n\r", ch);
    return eFAILURE;
  }
  if(!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_NOARENA)) {
    send_to_char("You have been banned from arenas.\n\r", ch);
    return eFAILURE;
  }
  if (affected_by_spell(ch,FUCK_PTHIEF) || affected_by_spell(ch,FUCK_GTHIEF)) {
    send_to_char("They don't allow criminals in the arena.\r\n",ch);
    return eFAILURE;
  }
  if(arena.type == CHAOS && !ch->clan) {
    send_to_char("Only clan members may join this arena.\r\n", ch);
    return eFAILURE;
  }
  if(IS_SET(world[ch->in_room].room_flags, ARENA)) {
    send_to_char("You are already there!\n\r", ch);
    return eFAILURE; 
  }
  if(arena.cur_num >= arena.num && arena.num > 0) {
    send_to_char("The arena is already full!\n\r", ch);
    return eFAILURE;
  }
  if(arena.type == HP && GET_RAW_HIT(ch) > arena.hplimit) {
    send_to_char("You are too strong for this arena!\n\r", ch);
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

  arena.cur_num++;
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
 
  act("$n disappears in a glorious flash of heroism.", ch, 0, 0, TO_ROOM, 0);
  while(send_to == NOWHERE)
  {
    if (arena.type == POTATO) { // potato arena
      send_to = real_room(number(pot_low, pot_hi));
    } else {
      send_to = real_room(number(ARENA_LOW, ARENA_HIGH-1));
    }
  }
  if(move_char(ch, send_to) == 0) return eFAILURE;
  act("$n appears, preparing for battle.", ch, 0, 0, TO_ROOM, 0);
  sprintf(buf, "## %s has joined the bloodbath!\n\r", GET_SHORT(ch));
  send_info(buf);
  do_look(ch, "", 8);
  return eSUCCESS;
}

bool ArenaIsOpen() {
  if (arena.status == OPENED)
    return true;
  else
    return false;
}
