/********************************
| Level 105 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"
#include <spells.h> // FUCK_CANTQUIT
#include <mobile.h>
#include <handler.h>
#include <room.h>
#include <player.h>
#include <utility.h>
#include <levels.h>
#include <interp.h>
#include <returnvals.h>


int do_log(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *vict;
    struct obj_data *dummy;
    char buf[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
        return eFAILURE;

    if(!has_skill(ch, COMMAND_LOG)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
    }

    one_argument(argument, buf);

    if (!*buf)
        send_to_char("Log who?\n\r", ch);

    else if (!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
        send_to_char("Couldn't find any such creature.\n\r", ch);
    else if (IS_NPC(vict))
        send_to_char("Can't do that to a beast.\n\r", ch);
    else if (GET_LEVEL(vict) >= GET_LEVEL(ch))
        act("$E might object to that.. better not.", ch, 0, vict, TO_CHAR, 0);
    else if (IS_SET(vict->pcdata->punish, PUNISH_LOG))
    {
        send_to_char("LOG removed.\n\r", ch);
        REMOVE_BIT(vict->pcdata->punish, PUNISH_LOG);
        sprintf(buf2, "%s removed log on %s.", GET_NAME(ch), GET_NAME(vict));
        log(buf2, GET_LEVEL(ch), LOG_GOD);
    }
    else
    {
        send_to_char("LOG set.\n\r", ch);
        SET_BIT(vict->pcdata->punish, PUNISH_LOG);
        sprintf(buf2, "%s just logged %s.", GET_NAME(ch), GET_NAME(vict));
        log(buf2, GET_LEVEL(ch), LOG_GOD);
    }
  return eSUCCESS;
}

int do_pardon(struct char_data *ch, char *argument, int cmd)
{
    char person[MAX_INPUT_LENGTH];
    char flag[MAX_INPUT_LENGTH];
    struct char_data *victim;

    if (IS_NPC(ch))
        return eFAILURE;

    half_chop(argument, person, flag);

    if (!*person)
    {
        send_to_char("Pardon whom?\n\r", ch);
        return eFAILURE;
    }
    if(!(victim = get_char(person)))
    {
      send_to_char("They aren't here.\n\r", ch);
      return eFAILURE;
    }
    if(IS_NPC(victim))
    {
      send_to_char("Can't pardon NPCs.\n\r",ch);
      return eFAILURE;
    }

    if(!str_cmp("thief", flag))
    {
      if(affected_by_spell(victim, FUCK_PTHIEF))
      {
        send_to_char("Thief flag removed.\n\r",ch);
        affect_from_char(victim, FUCK_PTHIEF);
        send_to_char("A nice god has pardoned you of your thievery.\n\r", victim);
      }
      else
      {
        send_to_char("That character is not a thief!\n\r", ch);
        return eFAILURE;
      }
    }
    else if(!str_cmp("killer", flag))
    {
      if (IS_SET(victim->affected_by, AFF_CANTQUIT))
      {
        send_to_char("Killer flag removed.\n\r",ch);
        affect_from_char(victim, FUCK_CANTQUIT);
        send_to_char(
        "A nice god has pardoned you of your murdering.\n\r", victim);
      }
      else
      {
        send_to_char("That player has no CANTQUIT flag!\n\r", ch);
        return eFAILURE;
      }
     }
     else
     {
        send_to_char("No flag specified! (Flags are 'thief' & 'killer')\n\r",ch);
        return eFAILURE;
     }         

    send_to_char("Done.\n\r",ch);
    sprintf(log_buf,"%s pardons %s for %s.",
        GET_NAME(ch), GET_NAME(victim), flag);
    log(log_buf, GET_LEVEL(ch), LOG_GOD);
  return eSUCCESS;
}

// do_string is in modify.C

int do_sqedit(struct char_data *ch, char *argument, int cmd)
{
  

}
