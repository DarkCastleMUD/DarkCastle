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

#include <cstring>

#include "room.h"   // Room
#include "db.h"     // real_room()
#include "interp.h" // do_look()
#include "character.h"
#include "utility.h" // send_to_char, etc..
#include "spells.h"
#include "handler.h"
#include "act.h"
#include "punish.h"
#include "player.h"
#include "arena.h"
#include "returnvals.h"
#include "levels.h"

struct _arena arena;

int do_arena(Character *ch, char *argument, int cmd)
{
  char arg1[256], arg2[256], arg3[256], arg4[256], arg5[256], buf[256];
  int low, high;

  if (!ch->has_skill(COMMAND_ARENA))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);
  argument = one_argument(argument, arg4);
  one_argument(argument, arg5);

  if (!*arg1 || !*arg2)
  {
    sprintf(buf, "Currently open for levels: %d %d\n\r", arena.low, arena.high);
    ch->send(buf);
    send_to_char("Syntax: arena <lowest level> <highest level> [num mortals] [type] [hp limit if applicable]\n\r"
                 "Valid types: chaos, potato, prize, hp\n\r"
                 "Use -1 for no limit on number of mortals.\r\n"
                 "Use arena 0 0 to close the arena.\r\n"
                 "Do *NOT* leave the arena open, people will be stuck there forever!\n\r"
                 "Also the arena can be opened without specifying the number"
                 " of mortals allowed to join.\r\n",
                 ch);
    return eSUCCESS;
  }

  if (!(low = atoi(arg1)) || !(high = atoi(arg2)) || low > high)
  {
    arena.low = 0;
    arena.high = 0;
    arena.status = CLOSED;
    ch->sendln("Closing the arena.");
    send_info("## The Arena has been CLOSED!\n\r");
    return eSUCCESS;
  }

  arena.low = low;
  arena.high = high;
  arena.status = OPENED;
  sprintf(buf, "## The Arena has been OPENED for levels %d - %d.\r\n"
               "## Type JOINARENA to enter the Bloodbath!\n\r",
          low, high);
  send_info(buf);

  if (*arg3)
  {
    arena.num = atoi(arg3);
    if (arena.num > 0)
    {
      sprintf(buf, "## Only %d can join the bloodbath!\n\r", arena.num);
      send_info(buf);
    }
  }
  else
  {
    arena.num = 0;
  }

  arena.cur_num = 0;

  if (*arg4)
  {
    if (!strcmp(arg4, "chaos"))
    {
      arena.type = CHAOS; // -2
      sprintf(buf, "## Only clan members can join the bloodbath!\r\n");
      send_info(buf);
      logf(IMMORTAL, LogChannels::LOG_ARENA, "%s started a Clan Chaos arena.", GET_NAME(ch));
    }

    if (!strcmp(arg4, "potato"))
    {
      arena.type = POTATO; // -3
      sprintf(buf, "##$4$B Special POTATO Arena!!$R\r\n");
      send_info(buf);
    }

    if (!strcmp(arg4, "prize"))
    {
      arena.type = PRIZE; // -3
      sprintf(buf, "##$4$B Prize Arena!!$R\r\n");
      send_info(buf);
    }

    if (!strcmp(arg4, "hp"))
    {
      if (*arg5)
      {
        arena.hplimit = atoi(arg5);
        if (arena.hplimit <= 0)
          arena.hplimit = 1000;
      }
      else
        arena.hplimit = 1000;

      arena.type = HP; // -4
      sprintf(buf, "##$4$B HP LIMIT Arena!!$R  Any more than %d raw hps, and you have to sit this one out!!\r\n", arena.hplimit);
      send_info(buf);
    }
  }
  else
  {
    arena.type = NORMAL;
  }

  ch->sendln("The Arena has been opened for the specified levels.");
  return eSUCCESS;
}

int do_joinarena(Character *ch, char *arg, int cmd)
{
  char buf[256];
  int send_to = DC::NOWHERE;
  struct affected_type *af, *next_af;
  int pot_low = 6362;
  int pot_hi = 6379;

  if (arena.low > ch->getLevel() || arena.high < ch->getLevel())
  {
    ch->sendln("The arena is not open for anyone your level.");
    return eFAILURE;
  }
  if (!IS_MOB(ch) && isSet(ch->player->punish, PUNISH_NOARENA))
  {
    ch->sendln("You have been banned from arenas.");
    return eFAILURE;
  }

  if (ch->isPlayerObjectThief() || ch->isPlayerGoldThief())
  {
    ch->sendln("They don't allow criminals in the arena.");
    return eFAILURE;
  }
  if (arena.type == CHAOS && !ch->clan)
  {
    ch->sendln("Only clan members may join this arena.");
    return eFAILURE;
  }
  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, ARENA))
  {
    ch->sendln("You are already there!");
    return eFAILURE;
  }
  if (arena.cur_num >= arena.num && arena.num > 0)
  {
    ch->sendln("The arena is already full!");
    return eFAILURE;
  }
  if (arena.type == HP && GET_RAW_HIT(ch) > arena.hplimit)
  {
    ch->sendln("You are too strong for this arena!");
    return eFAILURE;
  }
  if (ch->fighting)
  {
    ch->sendln("You're ALREADY in a fight...isn't that kinda silly?");
    return eFAILURE;
  }

  if (GET_POS(ch) == position_t::SLEEPING)
  {
    affect_from_char(ch, INTERNAL_SLEEPING);
    ch->wake();
  }

  arena.cur_num++;
  for (af = ch->affected; af; af = next_af)
  {
    next_af = af->next;
    if (af->type != Character::PLAYER_CANTQUIT)
      affect_remove(ch, af, SUPPRESS_ALL);
  }

  /* remove combat-related bits */
  ch->combat = 0;

  ch->setMove(GET_MAX_MOVE(ch));
  GET_MANA(ch) = GET_MAX_MANA(ch);
  ch->fillHP();
  GET_KI(ch) = GET_MAX_KI(ch);

  act("$n disappears in a glorious flash of heroism.", ch, 0, 0, TO_ROOM, 0);
  while (send_to == DC::NOWHERE)
  {
    if (arena.type == POTATO)
    { // potato arena
      send_to = real_room(number(pot_low, pot_hi));
    }
    else
    {
      send_to = real_room(number(ARENA_LOW, ARENA_HIGH - 1));
    }
  }
  if (move_char(ch, send_to) == 0)
    return eFAILURE;
  act("$n appears, preparing for battle.", ch, 0, 0, TO_ROOM, 0);
  sprintf(buf, "## %s has joined the bloodbath!\n\r", GET_SHORT(ch));
  send_info(buf);
  do_look(ch, "", 8);
  return eSUCCESS;
}

bool ArenaIsOpen()
{
  if (arena.status == OPENED)
    return true;
  else
    return false;
}
