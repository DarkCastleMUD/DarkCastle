/********************************
| Level 106 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"
#include <handler.h>
#include <spells.h>
#include <utility.h>
#include <connect.h>
#include <levels.h>
#include <mobile.h>
#include <interp.h>
#include <player.h>
#include <returnvals.h>

int do_plats (struct char_data *ch, char *argument, int cmd)
{
   struct char_data *i;
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

      if (GET_PLATINUM(i) < (uint32)minamt)
         continue;

      sprintf(buf, "%15d - %s\n\r", GET_PLATINUM(i), GET_NAME(i));
      send_to_char (buf, ch);
      }
   return eSUCCESS;
}

int do_force(struct char_data *ch, char *argument, int cmd)
{
  struct descriptor_data *i;
  struct descriptor_data *next_i;
  struct char_data *vict;
  char name[100], to_force[400],buf[400]; 

  if(IS_NPC(ch))
    return eFAILURE;
  if(!has_skill(ch, COMMAND_FORCE) && cmd != 123) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }


  half_chop(argument, name, to_force);

  if(!*name || !*to_force)
    send_to_char("Who do you wish to force to do what?\n\r", ch);

  else if(str_cmp("all", name)) {
    if(!(vict = get_char_vis(ch, name)))
      send_to_char("No one by that name here..\n\r", ch);
    else {
      if (GET_LEVEL(ch) < GET_LEVEL(vict) && IS_NPC(vict)) {
        send_to_char("Now doing that would just tick off the IMPS!\n\r", ch);
        sprintf(buf, "%s just tried to force %s to, %s", GET_NAME(ch), GET_NAME(vict),to_force);
        log(buf, OVERSEER, LOG_GOD);
        return eSUCCESS;
      }
      if((GET_LEVEL(ch) <= GET_LEVEL(vict)) && !IS_NPC(vict)) { 
        send_to_char("Why be forceful?\n\r", ch);
        sprintf(buf, "$n has failed to force you to '%s'.", to_force);
        act(buf,  ch, 0, vict, TO_VICT, 0);
      }
      else {
        if(ch->pcdata->stealth == FALSE) {
          sprintf(buf, "$n has forced you to '%s'.", to_force);
          act(buf,  ch, 0, vict, TO_VICT, 0);
          send_to_char("Ok.\n\r", ch);
        }
        sprintf(buf,"%s just forced %s to %s.", GET_NAME(ch), 
                GET_NAME(vict), to_force);
        command_interpreter(vict, to_force);
        log(buf, GET_LEVEL(ch), LOG_GOD);
      }
    }
  }

  else { /* force all */
    if(GET_LEVEL(ch) < OVERSEER) {
      send_to_char("Not gonna happen.\n\r", ch);
      return eFAILURE;
    }
    for(i = descriptor_list; i; i = next_i) {
       next_i = i->next;
       if(i->character != ch && !i->connected) {
         vict = i->character;
         if(GET_LEVEL(ch) <= GET_LEVEL(vict))
           continue;
         else {
           if(ch->pcdata->stealth == FALSE || GET_LEVEL(ch) < 109) {
             sprintf(buf, "$n has forced you to '%s'.", to_force);
             act(buf,  ch, 0, vict, TO_VICT, 0);
           }
           command_interpreter(vict, to_force);
         }
       }
    }
    send_to_char("Ok.\n\r", ch);
    sprintf(buf,"%s just forced all to %s.", GET_NAME(ch), to_force);
    log(buf, GET_LEVEL(ch), LOG_GOD);
  }
   return eSUCCESS;
}


