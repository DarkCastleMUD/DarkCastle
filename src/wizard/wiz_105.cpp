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
#include <fight.h>
#include <utility.h>
#include <levels.h>
#include <interp.h>
#include <returnvals.h>
#include <innate.h>
#include <fileinfo.h>

int do_clearaff(struct char_data *ch, char *argument, int cmd)
{

   char buf[MAX_INPUT_LENGTH];
   struct char_data *victim;
   struct affected_type *af, *afpk;
    struct obj_data *dummy;
 

   one_argument(argument, buf);

    if (!*buf)
     victim = ch;
    else if (!generic_find(argument, FIND_CHAR_WORLD, ch, &victim, &dummy))
        send_to_char("Couldn't find any such creature.\n\r", ch);
    if (victim) {
      for(af = victim->affected; af; af = afpk) {
        afpk = af->next;
        affect_remove(victim, af, SUPPRESS_ALL);
      }

//    send_to_char("Done.\r\n",ch);
  //  send_to_char("Your affects have been cleared.\r\n",victim);
    return eSUCCESS;
  }
  return eFAILURE;
}

int do_reloadhelp(struct char_data *ch, char *argument, int cmd)
{
  extern FILE* help_fl;
  extern struct help_index_element *help_index ;
  extern int top_of_helpt;
  extern struct help_index_element *build_help_index(FILE *fl, int *num);
  extern void free_help_from_memory();
  free_help_from_memory();
  dc_fclose(help_fl);
  if(!(help_fl = dc_fopen(HELP_KWRD_FILE, "r"))) {
    perror( HELP_KWRD_FILE );
    abort();
  }
  help_index = build_help_index(help_fl, &top_of_helpt);
  send_to_char("Reloaded.\r\n",ch);
  return eSUCCESS;
}

int do_log(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *vict;
    struct obj_data *dummy;
    char buf[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
        return eFAILURE;


    one_argument(argument, buf);

    if (!*buf)
        send_to_char("Log who?\n\r", ch);

    else if (!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
        send_to_char("Couldn't find any such creature.\n\r", ch);
    else if (IS_NPC(vict))
        send_to_char("Can't do that to a beast.\n\r", ch);
    else if (GET_LEVEL(vict) > GET_LEVEL(ch))
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

int do_showbits(struct char_data *ch, char *argument, int cmd)
{
    char person[MAX_INPUT_LENGTH];
    struct char_data *victim;
    one_argument(argument, person);

    if (!*person)
    {
        char buf[MAX_STRING_LENGTH];
		auto &character_list = DC::instance().character_list;
		for (auto& victim : character_list) {
           if(IS_NPC(victim)) continue;
	   sprintf(buf, "0.%s", GET_NAME(victim));
           do_showbits(ch, buf, cmd);
        }
        return eSUCCESS;
    }
    if(!(victim = get_char(person)))
    {
        send_to_char("They aren't here.\n\r", ch);
        return eFAILURE;
    }

    csendf(ch, "Player: %s\n\r", GET_NAME(victim));

    if(IS_SET(victim->combat, COMBAT_SHOCKED))
        send_to_char("COMBAT_SHOCKED\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_BASH1))
        send_to_char("COMBAT_BASH1\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_BASH2))
        send_to_char("COMBAT_BASH2\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_STUNNED))
        send_to_char("COMBAT_STUNNED\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_STUNNED2))
        send_to_char("COMBAT_STUNNED2\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_CIRCLE))
        send_to_char("COMBAT_CIRCLE\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_BERSERK))
        send_to_char("COMBAT_BERSERK\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_HITALL))
        send_to_char("COMBAT_HITALL\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_RAGE1))
        send_to_char("COMBAT_RAGE1\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_RAGE1))
        send_to_char("COMBAT_RAGE2\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_BLADESHIELD1))
        send_to_char("COMBAT_BLADESHIELD1\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_BLADESHIELD2))
        send_to_char("COMBAT_BLADESHIELD2\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_REPELANCE))
        send_to_char("COMBAT_REPELANCE\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_VITAL_STRIKE))
        send_to_char("COMBAT_VITAL_STRIKE\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_MONK_STANCE))
        send_to_char("COMBAT_MONK_STANCE\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_MISS_AN_ATTACK))
        send_to_char("COMBAT_MISS_AN_ATTACK\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_ORC_BLOODLUST1))
        send_to_char("COMBAT_ORC_BLOODLUST1\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_ORC_BLOODLUST2))
        send_to_char("COMBAT_ORC_BLOODLUST2\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_THI_EYEGOUGE))
        send_to_char("COMBAT_THI_EYEGOUGE\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_THI_EYEGOUGE2))
        send_to_char("COMBAT_THI_EYEGOUGE2\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_FLEEING))
        send_to_char("COMBAT_FLEEING\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_SHOCKED2))
        send_to_char("COMBAT_SHOCKED2\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_CRUSH_BLOW))
        send_to_char("COMBAT_CRUSH_BLOW\n\r", ch);

    if(IS_SET(victim->combat, COMBAT_CRUSH_BLOW2))
        send_to_char("COMBAT_CRUSH_BLOW2\n\r", ch);
   
    if(IS_SET(victim->combat, COMBAT_ATTACKER))
        send_to_char("COMBAT_ATTACKER\n\r", ch);

    send_to_char("--------------------\n\r\n\r", ch);

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
      if (ISSET(victim->affected_by, AFF_CANTQUIT))
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

int do_dmg_eq(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj_object;
  int eqdam;

  one_argument(argument, buf);

  if(!*buf)
  {
    send_to_char("Syntax: damage <item>\r\n", ch);
    return eFAILURE;
  }

  obj_object = get_obj_in_list_vis(ch, buf, ch->carrying);

  if(!obj_object)
  {
    send_to_char("You don't seem to have that item.\r\n", ch);
    return eFAILURE;
  }
  eqdam = damage_eq_once(obj_object);

  if(eqdam >= eq_max_damage(obj_object))
    eq_destroyed(ch, obj_object, -1);
  else 
  {
    act("$p is damaged.", ch, obj_object, 0, TO_CHAR, 0);
    act("$p carried by $n is damaged.", ch, obj_object, 0, TO_ROOM, 0);
  }

  return eSUCCESS;
}


char *print_classes(int bitv)
{
  static char buf[512];
  int i = 0;
  buf[0] = '\0';
  extern char*pc_clss_types2[];
 
  for (; *pc_clss_types2[i] != '\n'; i++)
  {
    if (IS_SET(bitv, 1 << (i-1)))
	sprintf(buf,"%s %s",buf, pc_clss_types2[i]);
  }

  return buf;
}

// do_string is in modify.C

struct skill_quest *find_sq(char *testa)
{
   struct skill_quest *curr;
   for (curr = skill_list; curr; curr = curr->next)
     if (!str_nosp_cmp(get_skill_name(curr->num), testa))
       return curr;
   return NULL;
}

struct skill_quest *find_sq(int sq)
{
   struct skill_quest *curr;
   for (curr = skill_list; curr; curr = curr->next)
     if (sq == curr->num)
       return curr;
   return NULL;
}


int do_sqedit(struct char_data *ch, char *argument, int cmd)
{
  char command[MAX_INPUT_LENGTH];
  argument = one_argument(argument,command);
    int clas = 1;
       
  extern char*pc_clss_types2[];
  const char *fields [] = {
    "new",
    "delete",
    "message",
    "level",
    "class",
    "show",
    "list",
    "save",
    "\n"
  };
  if (!has_skill(ch, COMMAND_SQEDIT))
  {
     send_to_char("You are unable to do so.\r\n",ch);
     return eFAILURE;
  }
//  if (argument && *argument && !is_number(argument))
 //   argument = one_argument(argument, skill+strlen(skill));
/*  if (!argument || !*argument)
  {
     send_to_char("$3Syntax:$R sqedit <level/class> <skill> <value> OR\r\n"
                  "$3Syntax:$R sqedit message/new/delete <skillname>\r\n",ch);
     send_to_char("$3Syntax:$R sqedit list.\r\n",ch);
     return eFAILURE;
  }*/
  int i;
  for (i = 0;; i++) {
    if (!str_cmp((char*)fields[i], command) ||
	!str_cmp((char*)fields[i], "\n"))
      break;
  }

  if (!str_cmp((char*)fields[i], "\n"))
  {

     send_to_char("$3Syntax:$R sqedit <message/level/class> <skill> <value> OR\r\n"
                  "$3Syntax:$R sqedit <show/new/delete> <skillname> OR\r\n",ch);
     send_to_char("$3Syntax:$R sqedit list <class>.\r\n",ch);
     send_to_char("$3Syntax:$R sqedit save.\r\n",ch);
    return eFAILURE;
  }
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],arg3[MAX_INPUT_LENGTH*2];
  bool done = FALSE;
  argument = one_argument(argument,arg1);
  struct skill_quest * skill=NULL;

  if (argument && *argument) 
  {
    argument = one_argument(argument,arg2);
    strcpy(arg3,arg1);
    strcat(arg3, " ");
    strcat(arg3,arg2);
    skill = find_sq(arg3);
  }

  if (skill==NULL && (skill = find_sq(arg1))==NULL && i!=0 && i!=6 && i!=7)
  {
    send_to_char("Unknown skill.\r\n",ch);
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
	if (i<=0 || arg2[0] == '\0')
	{
	  send_to_char("Skill not found.\r\n",ch);
	  return eFAILURE;
	}
	if (find_sq(i))
	{
	  send_to_char("Skill quest already exists. Stop duping the damn sqs ;)\r\n",ch);
	  return eFAILURE;
	}
//	argument = one_argument(argument,arg3);
        if (arg3[0] != '\0')
          for (int x = 0; *pc_clss_types2[x] != '\n'; x++)
          {
             if (!str_cmp(pc_clss_types2[x],arg2))
             clas = x;
          }

      #ifdef LEAK_CHECK
	newOne = (struct skill_quest *) calloc(1, sizeof(struct skill_quest));
      #else
	newOne = (struct skill_quest *) dc_alloc(1,sizeof(struct skill_quest));
      #endif
	newOne->num = i;
	newOne->level = 1;
        newOne->message = str_dup("New skillquest.");
        newOne->clas = (1<<(clas-1));
        newOne->next = skill_list;
	skill_list = newOne;
	send_to_char("Skill quest added.\r\n",ch);
     break;
    case 1:
      for (curren = skill_list; curren; curren = curren->next)
      {
	if (curren == skill)
        {
          if (last)   last->next = curren->next;
	  else        skill_list = curren->next;
	  send_to_char("Deleted.\r\n",ch);
	  dc_free(curren->message);
	  dc_free(curren);
 	  return eSUCCESS;
	}
	  last = curren;
      }
	send_to_char("Error in sqedit. Tell Urizen.\r\n",ch);
        break;
     break;
    case 2:
      send_to_char("Enter new message. End with \\s.\r\n",ch);
      ch->desc->connected = CON_EDITING;
      ch->desc->strnew = &(skill->message);
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
        for (i = 0; *pc_clss_types2[i] != '\n'; i++)
        {
	  if (!str_cmp(pc_clss_types2[i],arg2))
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
      if (IS_SET(skill->clas, 1<<(i-1)))
	REMOVE_BIT(skill->clas, 1<<(i-1));
        //skill->clas = i;
      else
	SET_BIT(skill->clas,1<<(i-1));
      send_to_char("Class modified.\r\n",ch);
      break;
    case 5: // show
      csendf(ch,"$3Skill$R: %s\r\n$3Message$R: %s\r\n$3Class$R: %s\r\n$3Level$R: %d\r\n",             get_skill_name(skill->num), skill->message, print_classes(skill->clas),skill->level);
      break;
     case 6:
      int l;
      for (l = 0; *pc_clss_types2[l] != '\n'; l++)
      {
        if (!str_cmp(pc_clss_types2[l],arg1))
           break;
      }
      if (*pc_clss_types2[l] == '\n')
      {
	send_to_char("Unknown class.",ch);
	return eFAILURE;
      }
      send_to_char("These are the current sqs:\r\n",ch);
      csendf(ch, "$3%s skillquests.$R\r\n",pc_clss_types2[l]);
      for (curren = skill_list; curren; curren = curren->next)
      {
	if (!IS_SET(curren->clas, 1<<(l-1)))
	  continue;
  	csendf(ch, "$3%d$R. %s\r\n", curren->num, get_skill_name(curren->num));
	done = TRUE;
      }
      if (!done)
	send_to_char("    No skill quests.\r\n",ch);
      break;
  case 7: // save
    do_write_skillquest(ch, argument, cmd);
    break;
    default:
      log("Incorrect -i- in do_sqedit", 0, LOG_WORLD);
      return eFAILURE;
  }
  return eSUCCESS;
}
int max_aff(struct obj_data *obj, int type)
{
   int a,b=-1;
   for (a = 0; a< obj->num_affects; a++)
   {
   if (obj->affected[a].location > 30) continue;
     if (type & (1<<obj->affected[a].location))
	b = b > obj->affected[a].modifier? b:obj->affected[a].modifier;
   }
  return b;
}

int maxcheck(int &check,  int max)
{
  if (check < max)
  {
    check = max;
    return 1;
  } else if (check == max) return 2;
  return 0;
}

int wear_bitv[MAX_WEAR] = {
 65535, 2, 2, 4, 4, 8, 16, 32, 64, 128, 256, 512,
 1024, 2048, 4096, 4096, 8192, 8192, 16384, 16384, 131072, 
  262144, 262144
};

int do_eqmax(struct char_data *ch, char *argument, int cmd)
{
  CHAR_DATA *vict;
  char arg[MAX_INPUT_LENGTH];
  int a = 0,o;
  argument = one_argument(argument, arg);
extern int class_restricted(struct char_data *ch, struct obj_data *obj);
extern  int size_restricted(struct char_data *ch, struct obj_data *obj);

  if ((vict =  get_pc_vis(ch, arg)) == NULL)
  {
	send_to_char("Who?\r\n", ch);
	return eFAILURE;
  }
  argument = one_argument(argument, arg);
  int type;
  int last_max[MAX_WEAR] = 
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  int last_vnum[5][MAX_WEAR] = {
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
/*    bool last_unique[5][MAX_WEAR] = {
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
*/

  if (!str_cmp(arg, "damage")) type = (1<<APPLY_DAMROLL)|(1<<APPLY_HIT_N_DAM);
  else if (!str_cmp(arg,"hp")) type = 1<<APPLY_HIT;
  else if (!str_cmp(arg,"mana")) type = 1<<APPLY_MANA;
  else {
    send_to_char("$3Syntax$R: eqmax <character> <damage/hp/mana> <optional: nodouble>\r\n",ch);
    return eFAILURE;
  }
  argument = one_argument(argument,arg);
  bool nodouble = FALSE;
  if (!str_cmp(arg, "nodouble")) nodouble = TRUE;
  int i = 1;
  struct obj_data *obj;
  for (i = 1; i < 32000; i++)
  {
    if (real_object(i) < 0) continue;
    obj = (obj_data *)obj_index[real_object(i)].item;
    if (!class_restricted(vict, obj) &&
	!size_restricted(vict, obj) &&
	CAN_WEAR(obj, ITEM_TAKE) &&
	!IS_SET(obj->obj_flags.extra_flags, ITEM_NOSAVE) &&
       obj->obj_flags.eq_level <= GET_LEVEL(vict) &&
     !IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
    for (o = 0; o < MAX_WEAR;o++)
	if (CAN_WEAR(obj, wear_bitv[o]))
	{
	int dam = max_aff(obj,type);
          if ((a = maxcheck(last_max[o], dam)))
          {
		if (a == 1) { 
			last_vnum[0][o] = obj_index[obj->item_number].virt;
			last_vnum[1][o] = -1;
			last_vnum[2][o] = -1;
			last_vnum[3][o] = -1;
			last_vnum[4][o] = -1;
			if (nodouble)
				break;
		} else {
			int v;
			for (v = 0; v < 5; v++)
			 if (last_vnum[v][o] == -1)
			   {last_vnum[v][o] = obj_index[obj->item_number].virt; 
			 break; }
		}
          }
       }
    }
  }
  char buf1[MAX_STRING_LENGTH];
  int tot = 0;
  for (i = 1;i < MAX_WEAR;i++)
  {
  buf1[0] = '\0';
  sprintf(buf1,"%d. ",i);
  if (last_vnum[a][0] != -1) tot += last_max[a];
  if (last_vnum[a][i] == -1) sprintf(buf1,"%s Nothing\r\n",buf1);
  else
   for (a=0;a<5;a++)
  {
    if (last_vnum[a][i] == -1) continue;
    sprintf(buf1,"%s %s(%d)   ",buf1,((obj_data 
*)obj_index[real_object(last_vnum[a][i])].item)->short_description,last_vnum[a][i]);
//    else sprintf(buf1,"%s%d. %d\r\n",buf1,i,last_vnum[i]); 
  }
  sprintf(buf1, "%s\n",buf1);
  send_to_char(buf1,ch);
  }
   sprintf(buf1,"Total %s: %d\r\n",arg,tot);
  send_to_char(buf1,ch);
  return eSUCCESS;
}

int do_reload(struct char_data *ch, char *argument, int cmd)
{
  int do_reload_help(struct char_data *ch, char *argument, int cmd);
  void reload_vaults(void);
  extern char motd[MAX_STRING_LENGTH];
  extern char imotd[MAX_STRING_LENGTH];
  extern char new_help[MAX_STRING_LENGTH];
  extern char new_ihelp[MAX_STRING_LENGTH];
  extern char credits[MAX_STRING_LENGTH];
  extern char story[MAX_STRING_LENGTH];
  extern char webpage[MAX_STRING_LENGTH];
  extern char info[MAX_STRING_LENGTH];
  extern char greetings1[MAX_STRING_LENGTH];
  extern char greetings2[MAX_STRING_LENGTH];
  extern char greetings3[MAX_STRING_LENGTH];
  extern char greetings4[MAX_STRING_LENGTH];

  char arg[256];

  one_argument(argument, arg);

  if (!str_cmp(arg, "all")) {
    file_to_string(MOTD_FILE, motd);
    file_to_string(IMOTD_FILE, imotd);
    file_to_string(NEW_HELP_PAGE_FILE, new_help);
    file_to_string(NEW_IHELP_PAGE_FILE, new_ihelp);
    file_to_string(CREDITS_FILE, credits);
    file_to_string(STORY_FILE, story);
    file_to_string(WEBPAGE_FILE, webpage);
    file_to_string(INFO_FILE, info);
	file_to_string(GREETINGS1_FILE, greetings1);
	file_to_string(GREETINGS2_FILE, greetings2);
	file_to_string(GREETINGS3_FILE, greetings3);
	file_to_string(GREETINGS4_FILE, greetings4);
  } else if (!str_cmp(arg, "credits"))
    file_to_string(CREDITS_FILE, credits);
  else if (!str_cmp(arg, "story"))
    file_to_string(STORY_FILE, story);
  else if (!str_cmp(arg, "webpage"))
    file_to_string(WEBPAGE_FILE, webpage);
  else if (!str_cmp(arg, "INFO"))
    file_to_string(INFO_FILE, info);
  else if (!str_cmp(arg, "motd"))
    file_to_string(MOTD_FILE, motd);
  else if (!str_cmp(arg, "imotd"))
    file_to_string(IMOTD_FILE, imotd);
  else if (!str_cmp(arg, "help"))
    file_to_string(NEW_HELP_PAGE_FILE, new_help);
  else if (!str_cmp(arg, "ihelp"))
    file_to_string(NEW_IHELP_PAGE_FILE, new_ihelp);
  else if (!str_cmp(arg, "xhelp")) {
    do_reload_help(ch, 0, 0);
    send_to_char("Done!\r\n", ch);
  } else if (!str_cmp(arg, "greetings")) {
	file_to_string(GREETINGS1_FILE, greetings1);
	file_to_string(GREETINGS2_FILE, greetings2);
	file_to_string(GREETINGS3_FILE, greetings3);
	file_to_string(GREETINGS4_FILE, greetings4);
  } else if (!str_cmp(arg, "vaults")) {
	  reload_vaults();
	  send_to_char("Done!\r\n", ch);
  } else {
    send_to_char("Unknown reload option. Try 'help reload'.\r\n", ch);
    return eFAILURE;
  }

  return eSUCCESS;
}

int do_listproc(CHAR_DATA *ch, char *argument, int a)
{
  char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int start,i,end, tot;
  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  bool mob;
  if (arg[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' ||
       !check_range_valid_and_convert(start, arg1, 0, 100000) ||
       !check_range_valid_and_convert(end, arg2, 0, 100000) ||
       start > end)
   {
      send_to_char("$3Syntax:$n listproc <obj/mob> <low vnum> <high vnum>\r\n",ch);
      return eFAILURE;
   }
   mob = !str_cmp(arg,"mob"); // typoed mob means obj. who cares.
   char buf[MAX_STRING_LENGTH];
   buf[0] = '\0';
   for (i = start, tot = 1; i <= end; i++)
   {
      if (mob && (real_mobile(i) < 0 || !mob_index[real_mobile(i)].mobprogs)) continue;
      else if (!mob && (real_object(i) < 0 || !obj_index[real_object(i)].mobprogs)) continue;
      if (tot++ > 100) break;
      if (mob)
      {
        sprintf(buf,"%s[%-3d] [%-3d] %s\r\n", buf,tot, 
i,((char_data*)mob_index[real_mobile(i)].item)->name);
      } else {
        sprintf(buf,"%s[%-3d] [%-3d] %s\r\n", buf,tot,i,((obj_data*)obj_index[real_object(i)].item)->name);
      }
   }
  send_to_char(buf,ch);
  return eSUCCESS;
}
