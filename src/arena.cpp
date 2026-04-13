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

#include "DC/DC.h"
#include "DC/comm.h"
#include "DC/handler.h"
#include "DC/spells.h" // INTERNAL_SLEEPING
#include "DC/act.h"    // TO_ROOM
#include "DC/punish.h"
#include "DC/utility.h"

auto Character::do_arena(QStringList arguments, cmd_t cmd) -> command_return_t
{
  auto rufus = get_mob_room_vis(this, "rufus arena-keeper");
  if (!isImmortalPlayer() && !rufus)
  {
    sendln("You must be in the same room as Rufus the Arena-keeper to use this command.");
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.value(0);
  if (arg1.isEmpty())
  {
    return do_arena_usage(arguments);
  }
  arguments.pop_front();

  if (arg1 == "info")
  {
    return do_arena_info(arguments);
  }
  else if (arg1 == "start")
  {
    return do_arena_start(arguments);
  }
  else if (arg1 == "join")
  {
    return do_arena_join(arguments);
  }
  else if (arg1 == "cancel")
  {
    return do_arena_cancel(arguments);
  }
  else
  {
    return do_arena_usage(arguments);
  }

  return ReturnValue::eSUCCESS;
}

auto do_joinarena(CharacterPtr ch, QString arg, cmd_t cmd) -> qint32
{
  QString buf;
  qint32 send_to = DC::NOWHERE;
  affected_type *af, *next_af;
  qint32 pot_low = 6362;
  qint32 pot_hi = 6379;

  auto &arena = dc_->arena_;
  if (arena.Low() > ch->getLevel() || arena.High() < ch->getLevel())
  {
    ch->sendln("The arena is not open for anyone your level.");
    return ReturnValue::eFAILURE;
  }
  if (!ch->isNonPlayer() && isSet(ch->player->punish, PUNISH_NOARENA))
  {
    ch->sendln("You have been banned from arenas.");
    return ReturnValue::eFAILURE;
  }
  if (ch->isPlayerObjectThief() || ch->isPlayerGoldThief())
  {
    ch->sendln("They don't allow criminals in the arena.");
    return ReturnValue::eFAILURE;
  }
  if (arena.isChaos() && !ch->clan)
  {
    ch->sendln("Only clan members may join this arena.");
    return ReturnValue::eFAILURE;
  }
  if (ch->room().isArena())
  {
    ch->sendln("You are already there!");
    return ReturnValue::eFAILURE;
  }
  if (arena.CurrentNumber() >= arena.Number() && arena.Number() > 0)
  {
    ch->sendln("The arena is already full!");
    return ReturnValue::eFAILURE;
  }
  if (arena.isHP() && GET_RAW_HIT(ch) > arena.HPLimit())
  {
    ch->sendln("You are too strong for this arena!");
    return ReturnValue::eFAILURE;
  }
  if (ch->fighting)
  {
    ch->sendln("You're ALREADY in a fight...isn't that kinda silly?");
    return ReturnValue::eFAILURE;
  }

  if (GET_POS(ch) == position_t::SLEEPING)
  {
    affect_from_char(ch, INTERNAL_SLEEPING);
    ch->wake();
  }

  arena.IncrementCurrentNumber();
  for (af = ch->affected; af; af = next_af)
  {
    next_af = af->next;
    if (af->type != Character::PLAYER_CANTQUIT)
      affect_remove(ch, af, SUPPRESS_ALL);
  }

  /* remove combat-related bits */
  ch->combat = {};

  ch->setMove(GET_MAX_MOVE(ch));
  GET_MANA(ch) = GET_MAX_MANA(ch);
  ch->fillHP();
  GET_KI(ch) = GET_MAX_KI(ch);

  act_to_room("$n disappears in a glorious flash of heroism.", ch, 0, 0, 0);
  while (send_to == DC::NOWHERE)
  {
    if (arena.isPotato())
    { // potato arena
      send_to = real_room(ch->dc_->number(pot_low, pot_hi));
    }
    else
    {
      send_to = real_room(ch->dc_->number(Arena::ARENA_LOW, Arena::ARENA_HIGH - 1));
    }
  }
  if (move_char(ch, send_to) == 0)
    return ReturnValue::eFAILURE;
  act_to_room("$n appears, preparing for battle.", ch, 0, 0, 0);
  dc_sprintf(buf, "## %s has joined the bloodbath!\r\n", qPrintable(ch->shortdesc_or_name()));
  send_info(buf);
  do_look(ch, "");
  return ReturnValue::eSUCCESS;
}
