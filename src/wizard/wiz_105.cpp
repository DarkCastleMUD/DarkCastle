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

struct skill_quest *find_sq(char *testa)
{
   struct skill_quest *curr;
   for (curr = skill_list; curr; curr = curr->next)
     if (!str_cmp(get_skill_name(curr->num), testa))
       return curr;
   return NULL;
}

int do_sqedit(struct char_data *ch, char *argument, int cmd)
{
  char command[MAX_INPUT_LENGTH];
  argument = one_argument(argument,command);
  
  extern char*pc_clss_types[];
  const char *fields [] = {
    "new",
    "delete",
    "message",
    "level",
    "class",
    "show",
    "list",
    "ignorethistest",
    "\n"
  };
  if (!has_skill(ch, COMMAND_SQEDIT))
  {
     send_to_char("You are unable to do so.\r\n",ch);
     return eFAILURE;
  }
//  if (argument && *argument && !is_number(argument))
 //   argument = one_argument(argument, skill+strlen(skill));
  if (!argument || !*argument)
  {
     send_to_char("$2Syntax:$R sqedit <level/class> <skill> <value> OR\r\n"
                  "$2Syntax:$R sqedit message/new/delete <skillname>\r\n",ch);
     send_to_char("$2Syntax:$R sqedit list.\r\n",ch);
     return eFAILURE;
  }
  int i;
  for (i = 0; fields[i] != "\n"; i++)
  {	
     if (!str_cmp((char*)fields[i], command))
       break;
  }
  if (fields[i] == "\n")
  {

     send_to_char("$2Syntax:$R sqedit <message/level/class> <skill> <value> OR\r\n"
                  "$2Syntax:$R sqedit <show/new/delete> <skillname> OR\r\n",ch);
     send_to_char("$2Syntax:$R sqedit list.\r\n",ch);
    return eFAILURE;
  }
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],arg3[MAX_INPUT_LENGTH*2];
  argument = one_argument(argument,arg1);
  struct skill_quest * skill=NULL;
  if (argument && *argument) {
    argument = one_argument(argument,arg2);
    strcpy(arg3,arg1);
    strcat(arg3, " ");
    strcat(arg3,arg2);
    if ((skill = find_sq(arg3))!=NULL) 
      argument = one_argument(argument,arg2);
  }
  if (skill==NULL && (skill = find_sq(arg1))==NULL && i!=0 && i!=6)
  {
    send_to_char("Unknown skill.",ch);
    return eFAILURE;
  }
  struct skill_quest *curren,*last = NULL;
  switch (i)
  {
    case 0:
       struct skill_quest *newOne;
///	int i;
        if (arg3[0] != '\0')
	  i = find_skill_num(arg3);
	if (i<=0)
	  i = find_skill_num(arg1);
	if (i<=0)
	{
	  send_to_char("Skill not found.\r\n",ch);
	  return eFAILURE;
	}
      #ifdef LEAK_CHECK
	newOne = (struct skill_quest *) calloc(1, sizeof(struct skill_quest));
      #else
	newOne = (struct skill_quest *) dc_alloc(1,sizeof(struct skill_quest));
      #endif
	newOne->num = i;
	newOne->level = 1;
        newOne->message = str_dup("New skillquest.");
        newOne->clas = 1;
        newOne->next = skill_list;
	skill_list = newOne;
     break;
    case 1:
      for (curren = skill_list; curren; curren = curren->next)
      {
	if (curren == skill)
        {
          if (last)
	    last->next = curren->next;
	  else
	    skill_list = curren->next;
	  send_to_char("Deleted.\r\n",ch);
	  dc_free(curren->message);
	  dc_free(curren);
	  break;
	}
	send_to_char("Error in sqedit. Tell Urizen.\r\n",ch);
        break;
      }
     break;
    case 2:
      send_to_char("Enter new message. End with ~.\r\n",ch);
      ch->desc->connected = CON_EDITING;
      ch->desc->str = &(skill->message);
      ch->desc->max_str = MAX_MESSAGE_LENGTH;
      break;
    case 3:
      if (is_number(arg2))
      {
	skill->level = atoi(arg2);
	send_to_char("Level modified.\r\n",ch);
      } else {
	 send_to_char("Invalid level.\r\n",ch);
	 return eFAILURE;
      }
      break;
    case 4:
      int i;
      if (!is_number(arg2))
      {
        for (i = 0; *pc_clss_types[i] != '\n'; i++)
        {
	  if (!str_cmp(pc_clss_types[i],arg2))
	     break;
	}
/*	if (*pc_clss_types[i] == '\n')
	{
	  send_to_char("Invalid class.\r\n",ch);
	  return eFAILURE;
	}*/
      } else {
	i = atoi(arg2);
      }    
      if (i < 1 || i > 11)
      {
	send_to_char("Invalid class.\r\n",ch);
	return eFAILURE;
      }
      skill->clas = i;
      send_to_char("Class modified.\r\n",ch);
      break;
    case 5:
      csendf(ch,"$2Skill$R: %s\r\n$2Message$R: %s\r\n$2Class$R: %s\r\n$2Level$R: %d\r\n",             get_skill_name(skill->num), skill->message, pc_clss_types[skill->clas],skill->level);
      break;
     case 6:
      for (curren = skill_list; curren; curren = curren->next)
      {
	csendf(ch, "$2%d$R. %s\r\n", curren->num, get_skill_name(curren->num));
      }
      break;
    default:
      log("Incorrect -i- in do_sqedit", 0, LOG_WORLD);
      return eFAILURE;
  }
  return eSUCCESS;
}
