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

int do_plats (char_data *ch, char *argument, int cmd)
{
   char_data *i;
   struct descriptor_data *d;
   char arg [100];
   char buf [100];
   int minamt;

   one_argument (argument, arg);
   if (*arg)
      minamt = atoi(arg);
   else
      minamt = 1;


   send_to_char ("          Plats - Player\n\r", ch);
   send_to_char ("          --------------\n\r", ch);

   for (d = descriptor_list; d; d = d->next)
      {
      if ((d->connected) || (!CAN_SEE(ch, d->character)))
         continue;

      if (d->original)
         i = d->original;
      else
         i = d->character;

      if (GET_PLATINUM(i) < (uint32_t)minamt)
         continue;

      sprintf(buf, "%15d - %s - %lld - %d\n\r", GET_PLATINUM(i), GET_NAME(i),GET_GOLD(i), GET_BANK(i));
      send_to_char (buf, ch);
      }
   return eSUCCESS;
}

int do_force(char_data *ch, string argument, int cmd = CMD_FORCE)
{
  struct descriptor_data *i = {};
  struct descriptor_data *next_i = {};
  char_data *vict = {};
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
      send_to_char("No one by that name here..\n\r", ch);
    else
    {
      if (GET_LEVEL(ch) < GET_LEVEL(vict) && IS_NPC(vict))
      {
        send_to_char("Now doing that would just tick off the IMPS!\n\r", ch);
        buf = format("{} just tried to force {} to, {}", GET_NAME(ch), GET_NAME(vict), to_force);
        log(buf, OVERSEER, LOG_GOD);
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
        if (ch->pcdata->stealth == FALSE)
        {
          buf = format("$n has forced you to '{}'.", to_force);
          act(buf, ch, 0, vict, TO_VICT, 0);
          send_to_char("Ok.\r\n", ch);
        }
        buf = format("{} just forced %s to %s.", GET_NAME(ch),
                GET_NAME(vict), to_force);
        command_interpreter(vict, to_force);
        log(buf, GET_LEVEL(ch), LOG_GOD);
      }
    }
  }

  else
  { /* force all */
    if (GET_LEVEL(ch) < OVERSEER)
    {
      send_to_char("Not gonna happen.\n\r", ch);
      return eFAILURE;
    }
    for (i = descriptor_list; i; i = next_i)
    {
      next_i = i->next;
      if (i->character != ch && !i->connected)
      {
        vict = i->character;
        if (GET_LEVEL(ch) <= GET_LEVEL(vict))
          continue;
        else
        {
          if (ch->pcdata->stealth == FALSE || GET_LEVEL(ch) < 109)
          {
            buf = format("$n has forced you to '{}'.", to_force);
            act(buf, ch, 0, vict, TO_VICT, 0);
          }
          command_interpreter(vict, to_force);
        }
      }
    }
    send_to_char("Ok.\n\r", ch);
    buf = format("{} just forced all to {}.", GET_NAME(ch), to_force);
    log(buf, GET_LEVEL(ch), LOG_GOD);
  }
  return eSUCCESS;
}
