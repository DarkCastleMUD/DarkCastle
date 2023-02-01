/********************************
| Level 106 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"
#include "handler.h"
#include "spells.h"
#include "utility.h"
#include "connect.h"
#include "levels.h"
#include "mobile.h"
#include "interp.h"
#include "player.h"
#include "returnvals.h"

#include <fmt/format.h>

using namespace std;
using namespace fmt;

int do_plats(Character *ch, char *argument, int cmd)
{
  Character *i;
  class Connection *d;
  char arg[100];
  char buf[100];
  int minamt;

  one_argument(argument, arg);
  if (*arg)
    minamt = atoi(arg);
  else
    minamt = 1;

  send_to_char("          Plats - Player\n\r", ch);
  send_to_char("          --------------\n\r", ch);

  for (d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    if ((d->connected) || (!CAN_SEE(ch, d->character)))
      continue;

    if (d->original)
      i = d->original;
    else
      i = d->character;

    if (GET_PLATINUM(i) < (uint32_t)minamt)
      continue;

    sprintf(buf, "%15d - %s - %lld - %d\n\r", GET_PLATINUM(i), GET_NAME(i), i->getGold(), GET_BANK(i));
    send_to_char(buf, ch);
  }
  return eSUCCESS;
}

int do_force(Character *ch, string argument, int cmd = CMD_FORCE)
{
  class Connection *i = {};
  class Connection *next_i = {};
  Character *vict = {};
  string name = {}, to_force = {}, buf = {};

  if (IS_NPC(ch))
  {
    return eFAILURE;
  }

  if (!has_skill(ch, COMMAND_FORCE) && cmd != CMD_FORCE)
  {
    ch->send("Huh?\r\n");
    return eFAILURE;
  }

  tie(name, to_force) = half_chop(argument);

  if (name.empty() || to_force.empty())
  {
    ch->send("Who do you wish to force to do what?\r\n");
    return eFAILURE;
  }
  else if (name != "all")
  {
    if (!(vict = get_char_vis(ch, name)))
      send_to_char("No one by that name here..\r\n", ch);
    else
    {
      if (GET_LEVEL(ch) < GET_LEVEL(vict) && IS_NPC(vict))
      {
        send_to_char("Now doing that would just tick off the IMPS!\n\r", ch);
        logentry(QString("%1 just tried to force %2 to %3").arg(GET_NAME(ch)).arg(GET_NAME(vict)).arg(to_force.c_str()), OVERSEER, LogChannels::LOG_GOD);
        return eSUCCESS;
      }
      if ((GET_LEVEL(ch) <= GET_LEVEL(vict)) && !IS_NPC(vict))
      {
        send_to_char("Why be forceful?\n\r", ch);
        buf = format("$n has failed to force you to '{}'.", to_force);
        act(buf, ch, 0, vict, TO_VICT, 0);
      }
      else
      {
        if (ch->player->stealth == false)
        {
          buf = format("$n has forced you to '{}'.", to_force);
          act(buf, ch, 0, vict, TO_VICT, 0);
          send_to_char("Ok.\r\n", ch);
        }
        buf = format("{} just forced %s to %s.", GET_NAME(ch),
                     GET_NAME(vict), to_force);
        command_interpreter(vict, to_force);
        logentry(buf.c_str(), GET_LEVEL(ch), LogChannels::LOG_GOD);
      }
    }
  }

  else
  { /* force all */
    if (GET_LEVEL(ch) < OVERSEER)
    {
      send_to_char("Not gonna happen.\r\n", ch);
      return eFAILURE;
    }
    for (i = DC::getInstance()->descriptor_list; i; i = next_i)
    {
      next_i = i->next;
      if (i->character != ch && !i->connected)
      {
        vict = i->character;
        if (GET_LEVEL(ch) <= GET_LEVEL(vict))
          continue;
        else
        {
          if (ch->player->stealth == false || GET_LEVEL(ch) < 109)
          {
            buf = format("$n has forced you to '{}'.", to_force);
            act(buf, ch, 0, vict, TO_VICT, 0);
          }
          command_interpreter(vict, to_force);
        }
      }
    }
    send_to_char("Ok.\r\n", ch);
    buf = format("{} just forced all to {}.", GET_NAME(ch), to_force);
    logentry(buf.c_str(), GET_LEVEL(ch), LogChannels::LOG_GOD);
  }
  return eSUCCESS;
}
