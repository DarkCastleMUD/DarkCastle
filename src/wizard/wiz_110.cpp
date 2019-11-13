/********************************
| Level 110 wizard commands
| 11/20/95 -- Azrack
**********************/
extern "C"
{
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
}

#include <fstream>
#include <iostream>
#include <sstream>

#include "wizard.h"
#include <utility.h>
#include <levels.h>
#include <player.h>
#include <mobile.h>
#include <interp.h>
#include <fileinfo.h>
#include <clan.h>
#include <returnvals.h>
#include <spells.h>
#include <interp.h>
#include <const.h>
#include <db.h>
#include <Leaderboard.h>

#ifdef WIN32
char *crypt(const char *key, const char *salt);
#else
#include <unistd.h>
extern "C" {
  char *crypt(const char *key, const char *salt);
}
#endif


void AuctionHandleRenames(CHAR_DATA *ch, string old_name, string new_name);

extern short bport;
extern world_file_list_item * obj_file_list;
extern char *extra_bits[];
extern char *wear_bits[];
extern char *more_obj_bits[];

int get_max_stat_bonus(CHAR_DATA *ch, int attrs)
{
  int bonus = 0;

  switch(attrs) {
    case STRDEX:
      bonus = MAX(0,get_max_stat(ch,STRENGTH)-15) + MAX(0,get_max_stat(ch, DEXTERITY)-15);break;
    case STRCON:
      bonus = MAX(0,get_max_stat(ch,STRENGTH)-15) + MAX(0,get_max_stat(ch, CONSTITUTION)-15);break;
    case STRINT:
      bonus = MAX(0,get_max_stat(ch,STRENGTH)-15) + MAX(0,get_max_stat(ch, INTELLIGENCE)-15);break;
    case STRWIS:
      bonus = MAX(0,get_max_stat(ch,STRENGTH)-15) + MAX(0,get_max_stat(ch, WISDOM)-15);break;
    case DEXCON:
      bonus = MAX(0,get_max_stat(ch,DEXTERITY)-15) + MAX(0,get_max_stat(ch, CONSTITUTION)-15);break;
    case DEXINT:
      bonus = MAX(0,get_max_stat(ch,DEXTERITY)-15) + MAX(0,get_max_stat(ch, INTELLIGENCE)-15);break;
    case DEXWIS:
      bonus = MAX(0,get_max_stat(ch,DEXTERITY)-15) + MAX(0,get_max_stat(ch, WISDOM)-15);break;
    case CONINT:
      bonus = MAX(0,get_max_stat(ch,CONSTITUTION)-15) + MAX(0,get_max_stat(ch, INTELLIGENCE)-15);break;
    case CONWIS:
      bonus = MAX(0,get_max_stat(ch,CONSTITUTION)-15) + MAX(0,get_max_stat(ch, WISDOM)-15);break;
    case INTWIS:
      bonus = MAX(0,get_max_stat(ch,INTELLIGENCE)-15) + MAX(0,get_max_stat(ch, WISDOM)-15);break;
    default: bonus = 0;
  }

  return bonus;
}

// List skill maxes.
int do_maxes(struct char_data *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  class_skill_defines *classskill;
  argument = one_argument(argument, arg);
  one_argument(argument, arg2);
  int oclass = GET_CLASS(ch); // old class.
 
  // get_skill_list uses a char argument, and so to keep upkeep
   // at a min I'm just modifying this here.
  int i;
  extern char* pc_clss_types2[];
  extern class_skill_defines *get_skill_list(char_data *ch);
  extern char* race_types[];
  for (i=0; pc_clss_types2[i][0] != '\n';i++)
    if (!str_cmp(pc_clss_types2[i], arg))
       break;
  if (pc_clss_types2[i][0] == '\n')
  {
    send_to_char("No such class.\r\n", ch);
    return eFAILURE; 
  }
  GET_CLASS(ch) = i;
  if ((classskill = get_skill_list(ch))==NULL) return eFAILURE;
  GET_CLASS(ch) = oclass;
  // Same problem with races... get_max_stat(ch, STRENGTH
  for (i=1; race_types[i][0] != '\n'; i++)
  {
    if (!str_cmp(race_types[i], arg2))
    {
	int orace = GET_RACE(ch);
	GET_RACE(ch) = i;
	for (i = 0; *classskill[i].skillname != '\n'; i++)
	{
	  int max = classskill[i].maximum;

          float percent = max*0.75;
	  if (classskill[i].attrs)
	  {
	       percent += max/100.0 * get_max_stat_bonus(ch,classskill[i].attrs);
	  }
          percent = MIN(max, percent);
          percent = MAX(max*0.75, percent);
	  csendf(ch, "%s: %d\n\r", classskill[i].skillname, (int)percent);
	}
	GET_RACE(ch) = orace;
	return eSUCCESS;
    }
  }
  send_to_char("No such race.\r\n",ch);
  return eFAILURE;
}

// give a command to a god
int do_bestow(struct char_data *ch, char *arg, int cmd)
{
   char_data * vict = NULL;
   char buf[MAX_INPUT_LENGTH];
   char command[MAX_INPUT_LENGTH];
   int i;
   int learn_skill(char_data * ch, int skill, int amount, int maximum);

   half_chop(arg, arg, command);

   if(!*arg) {
      send_to_char("Bestow gives a god command to a god.\r\n"
                   "Syntax:  bestow <god> <command>\r\n"
                   "Just 'bestow <god>' will list all available commands for that god.\r\n", ch);
      return eSUCCESS;
   }

   if(!(vict = get_pc_vis(ch, arg))) {
      sprintf(buf, "You don't see anyone named '%s'.", arg);
      send_to_char(buf, ch);
      return eSUCCESS;
   }

   if(!*command) {
      send_to_char("Command                Has command?\r\n"
                   "-----------------------------------\r\n\r\n", ch);
      for(i = 0; *bestowable_god_commands[i].name != '\n'; i++)
      {
	if (bestowable_god_commands[i].testcmd == false) {
	  sprintf(buf, "%22s %s\r\n", bestowable_god_commands[i].name,
		  has_skill(vict, bestowable_god_commands[i].num) ? "YES" : "---");
	  send_to_char(buf, ch);
	}
      }

      send_to_char("\n\r", ch);
      send_to_char("Test Command           Has command?\r\n"
                   "-----------------------------------\r\n\r\n", ch);
      for(i = 0; *bestowable_god_commands[i].name != '\n'; i++)
      {
	if (bestowable_god_commands[i].testcmd == true) {
	  sprintf(buf, "%22s %s\r\n", bestowable_god_commands[i].name,
		  has_skill(vict, bestowable_god_commands[i].num) ? "YES" : "---");
	  send_to_char(buf, ch);
	}
      }

      return eSUCCESS;
   }

   for(i = 0; *bestowable_god_commands[i].name != '\n'; i++)
     if(!strcmp(bestowable_god_commands[i].name, command))
       break;

   if(*bestowable_god_commands[i].name == '\n') {
     sprintf(buf, "There is no god command named '%s'.\r\n", command);
     send_to_char(buf, ch);
     return eSUCCESS;
   }

   // if has
   if(has_skill(vict, bestowable_god_commands[i].num)) {
     sprintf(buf, "%s already has that command.\r\n", GET_NAME(vict));
     send_to_char(buf, ch);
     return eSUCCESS;
   }

   // give it
   learn_skill(vict, bestowable_god_commands[i].num, 1, 1);

   sprintf(buf, "%s has been bestowed %s.\r\n", GET_NAME(vict), 
                bestowable_god_commands[i].name);
   send_to_char(buf, ch);
   sprintf(buf, "%s has been bestowed %s by %s.", GET_NAME(vict), 
                bestowable_god_commands[i].name, GET_NAME(ch));
   log(buf, GET_LEVEL(ch), LOG_GOD);
   sprintf(buf, "%s has bestowed %s upon you.\r\n", GET_NAME(ch), 
                bestowable_god_commands[i].name);
   send_to_char(buf, vict);
   return eSUCCESS;
}

// take away a command from a god
int do_revoke(struct char_data *ch, char *arg, int cmd)
{
   char_data * vict = NULL;
   char buf[MAX_INPUT_LENGTH];
   char command[MAX_INPUT_LENGTH];
   int i;

   half_chop(arg, arg, command);

   if(!*arg) {
      send_to_char("Bestow gives a god command to a god.\r\n"
                   "Syntax:  revoke <god> <command|all>\r\n"
                   "Use 'bestow <god>' to view what commands a god has.\r\n", ch);
      return eSUCCESS;
   }

   if(!(vict = get_pc_vis(ch, arg))) {
      sprintf(buf, "You don't see anyone named '%s'.", arg);
      send_to_char(buf, ch);
      return eSUCCESS;
   }

   struct char_skill_data * curr = vict->skills;
   struct char_skill_data * last = NULL;

   if(!strcmp(command, "all")) {
     while(curr) {
       vict->skills = curr->next;
       dc_free(curr);
       curr = vict->skills;
     } 
     
     sprintf(buf, "%s has had all comands revoked.\r\n", GET_NAME(vict));
     send_to_char(buf, ch);
     sprintf(buf, "%s has had all commands revoked by %s.\r\n", GET_NAME(vict), 
                GET_NAME(ch));
     log(buf, GET_LEVEL(ch), LOG_GOD);
     sprintf(buf, "%s has revoked all commands from you.\r\n", GET_NAME(ch));
     send_to_char(buf, vict);
     return eSUCCESS;
   }

   for(i = 0; *bestowable_god_commands[i].name != '\n'; i++)
     if(!strcmp(bestowable_god_commands[i].name, command))
       break;

   if(*bestowable_god_commands[i].name == '\n') {
     sprintf(buf, "There is no god command named '%s'.\r\n", command);
     send_to_char(buf, ch);
     return eSUCCESS;
   }

  while(curr) {
    if(curr->skillnum == bestowable_god_commands[i].num)
      break;
    last = curr;
    curr = curr->next;
  }

  if(!curr) {
    sprintf(buf, "%s does not have %s.\r\n", GET_NAME(vict), bestowable_god_commands[i].name);
    send_to_char(buf, ch);
    return eSUCCESS;
  }

  // remove from list
  if(last) {
    last->next = curr->next;
    dc_free(curr);
  }
  else {
    vict->skills = curr->next;
    dc_free(curr);
  }

   sprintf(buf, "%s has had %s revoked.\r\n", GET_NAME(vict), 
                bestowable_god_commands[i].name);
   send_to_char(buf, ch);
   sprintf(buf, "%s has had %s revoked by %s.", GET_NAME(vict), 
                bestowable_god_commands[i].name, GET_NAME(ch));
   log(buf, GET_LEVEL(ch), LOG_GOD);
   sprintf(buf, "%s has revoked %s from you.\r\n", GET_NAME(ch), 
                bestowable_god_commands[i].name);
   send_to_char(buf, vict);
   return eSUCCESS;
}

/* Thunder is currently in wiz_104.c */

int do_wizlock(struct char_data *ch, char *argument, int cmd)
{
    wizlock = !wizlock;

    if ( wizlock ) {
        sprintf(log_buf,"Game has been wizlocked by %s.",GET_NAME(ch));
        log(log_buf, ANGEL, LOG_GOD);
        send_to_char("Game wizlocked.\n\r", ch);
    } else {
        sprintf(log_buf,"Game has been un-wizlocked by %s.",GET_NAME(ch));
        log(log_buf, ANGEL, LOG_GOD);
        send_to_char("Game un-wizlocked.\n\r", ch);
    }
    return eSUCCESS;
}

/************************************************************************
| do_chpwd
| Precondition: ch != 0, arg != 0
| Postcondition: ch->password is arg
| Side effects: None
| Returns: None
*/
int do_chpwd(struct char_data *ch, char *arg, int cmd)
{
  struct char_data *victim;
  char name[100], buf[50];

  /* Verify preconditions */
  assert(ch != 0);
  if(arg == 0) return eFAILURE;

  half_chop(arg, name, buf);

  if(!*name) {
    send_to_char("Change whose password?\n\r", ch);
    return eFAILURE;
  }

  if(!(victim = get_pc_vis(ch, name))) {
    send_to_char("That player was not found.\n\r", ch);
    return eFAILURE;
  }

  one_argument(buf, name);

  if(!*name || strlen(name) > 10) {
    send_to_char("Password must be 10 characters or less.\n\r", ch);
    return eFAILURE;
  }

  strncpy(victim->pcdata->pwd, (char *)crypt((char *)name, (char *)GET_NAME(victim)), PASSWORD_LEN );
  victim->pcdata->pwd[PASSWORD_LEN] = '\0';

  send_to_char("Ok.\n\r", ch);
  return eSUCCESS;
}

int do_fakelog(struct char_data *ch, char *argument, int cmd)
{
   char command [MAX_INPUT_LENGTH];
   char lev_str [MAX_INPUT_LENGTH];
   int lev_nr = 110;

   if (IS_NPC(ch))
           return eFAILURE;

   half_chop(argument, lev_str, command);
 
   if (!*lev_str)
      {
           send_to_char("Also, you must supply a level.\n\r", ch);
           return eFAILURE;
      }
       
   if(isdigit(*lev_str))
      {
           lev_nr = atoi(lev_str);
      if (lev_nr < IMMORTAL || lev_nr > IMP)
         {
         send_to_char("You must use a valid level from 100-110.\n\r",ch);
         return eFAILURE;
         }
           }

   send_to_gods(command, lev_nr, 1);
   return eSUCCESS;
}

int do_rename_char(struct char_data *ch, char *arg, int cmd)
{
  struct char_data *victim;
  struct obj_data *obj;
  char name[160];
  char strsave[MAX_INPUT_LENGTH];
  char oldname[MAX_INPUT_LENGTH];
  char newname[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];

  FILE * fl;
  int iWear;

  arg = one_argument(arg, oldname);
  arg = one_argument(arg, newname);
  arg = one_argument(arg, arg3);

  if(!(*oldname) || !(*newname)) {
    send_to_char("Usage: rename <oldname> <newname> [takeplats]\n\r", ch);
    return eFAILURE;
  }

  oldname[0] = UPPER(oldname[0]);
  newname[0] = UPPER(newname[0]);

  if(!(victim = get_pc(oldname))) {
    csendf(ch, "%s is not in the game.\r\n", oldname);
    return eFAILURE;
  }

  if(GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char("That might just piss them off.\r\n", ch);
    csendf(victim, "%s just tried to rename you.\r\n", GET_NAME(ch));
    return eFAILURE;
  }

  // +1 cause you can actually have 13 char names
  if(strlen(newname) > (MAX_NAME_LENGTH+1))
  {
    send_to_char("New name too long.\r\n", ch);
    return eFAILURE;
  }

  if (!strcmp(arg3, "takeplats")) {
    if (GET_PLATINUM(victim) < 500) {
      send_to_char("They don't have enough plats.\n\r", ch);
      return eFAILURE;
    } else {
      GET_PLATINUM(victim) -= 500;
      csendf(ch, "You reach into %s's soul and remove 500 platinum.\n\r", GET_SHORT(victim));
      send_to_char("You feel the hand of god slip into your soul and remove 500 platinum.\n\r", victim);
      sprintf(name, "500 platinum removed from %s for rename.", GET_NAME(victim));
      log(name, GET_LEVEL(ch), LOG_GOD);
    }
  }

//extern short bport;
  if (!bport) {
    sprintf(strsave, "%s/%c/%s", SAVE_DIR, newname[0], newname);  
  } else {
    sprintf(strsave, "%s/%c/%s", BSAVE_DIR, newname[0], newname);  
  }

  if((fl = fopen(strsave, "r"))) {
    fclose(fl);
    csendf(ch, "The name %s is already in use.\r\n", newname);
    return eFAILURE;
  }

  for (iWear = 0; iWear < MAX_WEAR; iWear++)
  {
    if (victim->equipment[iWear] && 
        IS_SET(victim->equipment[iWear]->obj_flags.extra_flags, ITEM_SPECIAL))
    {
      char tmp[256];
      sprintf(tmp,"%s",victim->equipment[iWear]->name);
      tmp[strlen(tmp)-strlen(GET_NAME(victim))-1] = '\0';
      sprintf(tmp,"%s %s",tmp, newname);
      victim->equipment[iWear]->name = str_hsh(tmp);
    }
    if(victim->equipment[iWear] && victim->equipment[iWear]->obj_flags.type_flag == ITEM_CONTAINER)
    {
      for(obj = victim->equipment[iWear]->contains; obj ; obj = obj->next_content) {
        if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) {
          char tmp[256];
          sprintf(tmp,"%s",obj->name);
          tmp[strlen(tmp)-strlen(GET_NAME(victim))-1] = '\0';
          sprintf(tmp,"%s %s",tmp, newname);
          obj->name = str_hsh(tmp);
        }
      }
    }
  }
 
  obj = victim->carrying;
  while(obj)
  {
    if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
      char tmp[256];
      sprintf(tmp,"%s",obj->name);
      tmp[strlen(tmp)-strlen(GET_NAME(victim))-1] = '\0';
      sprintf(tmp,"%s %s",tmp, newname);
      obj->name = str_hsh(tmp);
    }
    if(GET_ITEM_TYPE(obj) == ITEM_CONTAINER)
    {
      OBJ_DATA *obj2;
      for(obj2 = obj->contains; obj2 ; obj2 = obj2->next_content) {
        if(IS_SET(obj2->obj_flags.extra_flags, ITEM_SPECIAL)) {
          char tmp[256];
          sprintf(tmp,"%s",obj2->name);
          tmp[strlen(tmp)-strlen(GET_NAME(victim))-1] = '\0';
          sprintf(tmp,"%s %s",tmp, newname);
          obj2->name = str_hsh(tmp);
        }
      }
    }
    obj = obj->next_content;
  }

  int clan = GET_CLAN(victim), rights = plr_rights(victim);
  do_outcast(victim, GET_NAME(victim), 9);

  do_fsave(ch, GET_NAME(victim), 9);

  // Copy the pfile
  if (!bport) {
    sprintf(name, "cp %s/%c/%s %s/%c/%s", SAVE_DIR, victim->name[0],
	    GET_NAME(victim), SAVE_DIR, newname[0], newname);
  } else {
    sprintf(name, "cp %s/%c/%s %s/%c/%s", BSAVE_DIR, victim->name[0],
	    GET_NAME(victim), BSAVE_DIR, newname[0], newname);
  }

  system(name);

  char src_filename[256];
  char dst_filename[256];
  struct stat buf;

  // Only copy golems if they exist
  for (int i=0; i < MAX_GOLEMS; i++) {
    snprintf(src_filename, 256, "%s/%c/%s.%d", FAMILIAR_DIR, victim->name[0], GET_NAME(victim), i);
    if (0 == stat(src_filename, &buf)) { 
      // Make backup
      snprintf(dst_filename, 256, "%s/%c/%s.%d.old", FAMILIAR_DIR, victim->name[0], GET_NAME(victim), i);
      sprintf(name, "cp %s %s", src_filename, dst_filename);
      system(name);
      
      // Rename
      snprintf(dst_filename, 256, "%s/%c/%s.%d", FAMILIAR_DIR, newname[0], newname, i);
      sprintf(name, "mv %s %s", src_filename, dst_filename);
      system(name);
    }
  }

  sprintf(name, "%s renamed to %s.", GET_NAME(victim), newname);
  log(name, GET_LEVEL(ch), LOG_GOD);

  //handle the renames
  AuctionHandleRenames(ch, GET_NAME(victim), newname);

  // Get rid of the existing one
  do_zap(ch, GET_NAME(victim), 10);

  // load the new guy
  do_linkload(ch, newname, 9);

  if(!(victim = get_pc(newname)))
  {
    send_to_char("Major problem...coudn't find target after pfile copied.  Notify Urizen immediatly.\r\n", ch);
    return eFAILURE;
  }
  do_name(victim, " %", 9);

  struct clan_member_data * pmember = NULL;

  if (clan) { 
    clan_data *tc = get_clan(clan);
    victim->clan = clan;
    add_clan_member(tc, victim);  
    if((pmember = get_member(GET_NAME(victim), ch->clan)))
      pmember->member_rights = rights;
    add_totem_stats(victim);
  }
  extern void rename_vault_owner(char *arg1, char *arg2);
  extern void rename_leaderboard(char *, char *);

  rename_vault_owner(oldname, newname);
  leaderboard.rename(oldname, newname);

  return eSUCCESS;
}
int do_install(struct char_data *ch, char *arg, int cmd)
{
  char buf[256], type[256], arg1[256], err[256], arg2[256];
  int range = 0, type_ok = 0, numrooms = 0;
  int ret;
  
/*  if(!has_skill(ch, COMMAND_INSTALL)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }
*/
  half_chop(arg, arg1, buf);
  half_chop(buf, arg2, type);

  if (!*arg1 || !*type || !*arg2) {
    sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 29100 100 m = installs mob range 29100-29199.\n\r");
    send_to_char(err, ch);
    return eFAILURE;
  }

  if (!(range = atoi(arg1))) {
    sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 29100 100 m = installs mob range 29100-29199.\n\r");
    send_to_char(err, ch);
    return eFAILURE;
  }

  if (range <= 0) {
    send_to_char("Range number must be greater than 0\r\n", ch);
    return eFAILURE;
  }

  if (!(numrooms = atoi(arg2))) {
    sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 29100 100 m = installs mob range 29100-29199.\n\r");
    send_to_char(err, ch);
    return eFAILURE;
  }

  if (numrooms <= 0) {
    send_to_char("Number of rooms must be greater than 0.\r\n", ch);
    return eFAILURE;
  }

  switch (*type) {
    case 'W':
    case 'w':
    case 'O':
    case 'o':
    case 'M':
    case 'm':
    case 'z':
    case 'Z':
    case 'a':
    case 'A':
      type_ok = 1;
      break;
    default:
      type_ok = 0;
      break;
  }

  if (type_ok != 1) {
    sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 29100 100 m = installs mob range 29100-29199.\n\r");
    send_to_char(err, ch);
    return eFAILURE;
  }
  
  sprintf(buf, "./new_zone %d %d %c true %s", range, numrooms, *type,bport == 1 ? "b":"n");
  ret = system(buf);
  // ret = bits, but I didn't use bits because I'm lazy and it only returns 2 values I gives a flyging fuck about!
  // if you change the script, you gotta change this too. - Rahz

  if (ret == 0) {
    sprintf(err, "Range %d Installed!  These changes will not take effect until the next reboot!\r\n", range);
  } else if (ret == 256) {
    sprintf(err, "That range would overlap another range!\r\n");
  } else {
    sprintf(err, "Error Code: %d\r\n"
                 "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 29100 100 m = installs mob range 29100-29199.\n\r", ret);
  }
  send_to_char(err, ch);
  return eSUCCESS;
}

int do_range(struct char_data *ch, char *arg, int cmd)
{
  struct char_data *victim;
  char name[160], buf[160];
  char kind[160];
  char trail[160];
  char message[256];

/*
  extern world_file_list_item * world_file_list;
  extern world_file_list_item *   mob_file_list;
  extern world_file_list_item *   obj_file_list;
*/
  if(!has_skill(ch, COMMAND_RANGE)) {
    send_to_char("Huh?\r\n", ch);
    return eFAILURE;
  }

  arg = one_argument(arg, name);
  arg = one_argument(arg, kind);
  arg = one_argument(arg, buf);
  arg = one_argument(arg, trail);

  if(!(*name) || !(*buf) || !(*kind)) {
    send_to_char("Syntax: range <god> <low vnum> <high vnum>\n\r", ch);
    send_to_char("Syntax: range <god> <r/m/o> <low vnum> <high vnum>\n\r", ch);
    return eFAILURE;
  }

  if(!(victim = get_pc_vis(ch, name))) {
    send_to_char("Set whose range?!\n\r", ch);
    return eFAILURE;
  }
  int low,high;
  if (trail[0] != '\0') {
    if(!isdigit(*buf) || !isdigit(*trail)) {
      send_to_char("Specify valid numbers. To remove, set the ranges to 0 low and 0 high.\r\n", ch);
      return eFAILURE;
   }
   low = atoi(buf);
   high = atoi(trail);
  }
  else {
    if(!isdigit(*buf) || !isdigit(*kind)) {
      send_to_char("Specify valid numbers. To remove, set the ranges to 0 low and 0 high.\r\n", ch);
      return eFAILURE;
   }
   low = atoi(kind);
   high = atoi(buf);
 }
  if (low < 0 || high < 0)
  {
    send_to_char("The number needs to be positive.\r\n",ch);
    return eFAILURE;
  }
  if (trail[0])
  {
    switch(LOWER(kind[0]))
    {
	case 'm':
	    victim->pcdata->buildMLowVnum = low;
	    victim->pcdata->buildMHighVnum = high;
	    sprintf(message, "%s M range set to %d-%d.\r\n", GET_NAME(victim), low, high);
	    send_to_char(message, ch);
	    sprintf(message, "Your M range has been set to %d-%d.\r\n", low, high);
	    send_to_char(message, victim);
	  return eSUCCESS;
	case 'o':
	    victim->pcdata->buildOLowVnum = low;
	    victim->pcdata->buildOHighVnum = high;
	    sprintf(message, "%s O range set to %d-%d.\r\n", GET_NAME(victim), low, high);
	    send_to_char(message, ch);
	    sprintf(message, "Your O range has been set to %d-%d.\r\n", low, high);
	    send_to_char(message, victim);
	  return eSUCCESS;
	case 'r':
	    victim->pcdata->buildLowVnum = low;
	    victim->pcdata->buildHighVnum = high;
	    sprintf(message, "%s R range set to %d-%d.\r\n", GET_NAME(victim), low, high);
	    send_to_char(message, ch);
	    sprintf(message, "Your R range has been set to %d-%d.\r\n", low, high);
	    send_to_char(message, victim);
	  return eSUCCESS;
	default: 
	  send_to_char("Invalid type. Valid ones are r/o/m.\r\n",ch);
	  return eFAILURE;
    }    
  } else {
    victim->pcdata->buildLowVnum = victim->pcdata->buildOLowVnum = victim->pcdata->buildMLowVnum = low;
    victim->pcdata->buildHighVnum = victim->pcdata->buildOHighVnum = victim->pcdata->buildMHighVnum = high;
    sprintf(message, "%s range set to %d-%d.\r\n", GET_NAME(victim), low, high);
    send_to_char(message, ch);
    sprintf(message, "Your range has been set to %d-%d.\r\n", low, high);
    send_to_char(message, victim);
  }
  return eSUCCESS;
}

extern int r_new_meta_platinum_cost(int start, long long plats);
extern int r_new_meta_exp_cost(int start, long long exp);

extern long long moves_exp_spent(char_data * ch);
extern long long moves_plats_spent(char_data * ch);
extern long long hps_exp_spent(char_data * ch);
extern long long hps_plats_spent(char_data * ch);
extern long long mana_exp_spent(char_data * ch);
extern long long mana_plats_spent(char_data * ch);



int do_metastat(CHAR_DATA *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  argument = one_argument(argument, arg);
  if (arg[0] == '\0' || !(victim = get_pc_vis(ch, arg)))
  {
     send_to_char("metastat who?\r\n",ch);
     return eFAILURE;
  }
  char buf[MAX_STRING_LENGTH];

  sprintf(buf, "Hps metad: %d Mana metad: %d Moves Metad: %d\r\n",
	GET_HP_METAS(victim), GET_MANA_METAS(victim), GET_MOVE_METAS(ch));
  send_to_char(buf,ch);


  sprintf(buf, "Hit points: %d\r\n   Exp spent: %lld\r\n   Plats spent: %lld\r\n   Plats enough for: %d\r\n   Exp enough for: %d\r\n",
      GET_RAW_HIT(victim),hps_exp_spent(victim), hps_plats_spent(victim), 
r_new_meta_platinum_cost(0,hps_plats_spent(victim))+GET_RAW_HIT(victim)-GET_HP_METAS(victim),
	r_new_meta_exp_cost(0, hps_exp_spent(victim))+GET_RAW_HIT(victim)-GET_HP_METAS(victim));
  send_to_char(buf,ch);
  

  sprintf(buf, "Mana points: %d\r\n   Exp spent: %lld\r\n   Plats spent: %lld\r\n   Plats enough for: %d\r\n   Exp enough for: %d\r\n",
      GET_RAW_MANA(victim),mana_exp_spent(victim), mana_plats_spent(victim), 
r_new_meta_platinum_cost(0,mana_plats_spent(victim))+GET_RAW_MANA(victim)-GET_MANA_METAS(victim),	
r_new_meta_exp_cost(0, mana_exp_spent(victim))+GET_RAW_MANA(victim)-GET_MANA_METAS(victim));
  send_to_char(buf,ch);


  sprintf(buf, "Move points: %d\r\n   Exp spent: %lld\r\n   Plats spent: %lld\r\n   Plats enough for: %d\r\n   Exp enough for: %d\r\n",
      GET_RAW_MOVE(victim),moves_exp_spent(victim), moves_plats_spent(victim), 
r_new_meta_platinum_cost(0,moves_plats_spent(victim))+GET_RAW_MOVE(victim)-GET_MOVE_METAS(victim),
	r_new_meta_exp_cost(0,moves_exp_spent(victim))+GET_RAW_MOVE(victim)-GET_MOVE_METAS(victim));
  send_to_char(buf,ch);


  buf[0] = '\0';
  unsigned int i = 1,l = 0;
  extern struct command_info cmd_info[];
  extern unsigned int cmd_size;
  for(; i < cmd_size; i++)
  {
     if ((l++ % 10) == 0) sprintf(buf, "%s\r\n",buf);
     sprintf(buf,"%s%d ",buf,cmd_info[i].command_number);
  }
  send_to_char(buf,ch);
  return eSUCCESS;
}


int do_acfinder(CHAR_DATA *ch, char *argument, int cmdnum)
{
  char arg[MAX_STRING_LENGTH];
  argument = one_argument(argument,arg);

  if (!arg[0])
  {
      send_to_char("Syntax: acfinder <wear slot>\r\n",ch);
      return eFAILURE;
  }
  extern char *wear_bits[];
  int i = 1;
  for (; wear_bits[i][0] != '\n'; i++)
     if (!str_cmp(wear_bits[i],arg))
       break;
  if (wear_bits[i][0] == '\n')
  {
      send_to_char("Syntax: acfinder <wear slot>\r\n",ch);
      return eFAILURE;
  }
  i = 1 << i;
  int r,o = 1;
  OBJ_DATA *obj;
  char buf[MAX_STRING_LENGTH];
  for (r = 0; r < top_of_objt; r++)
  {
    obj = (OBJ_DATA*) obj_index[r].item;
    if (GET_ITEM_TYPE(obj) != ITEM_ARMOR) continue;
    if (!CAN_WEAR(obj, i)) continue;
    int ac = 0 - obj->obj_flags.value[0];
    for (int z = 0; z < obj->num_affects; z++)
       if (obj->affected[z].location == APPLY_ARMOR)
	 ac += obj->affected[z].modifier;
    sprintf(buf, "$B%s%d. %-50s Vnum: %d AC Apply: %d\r\n$R",
		o%2==0?"$2":"$3",o, obj->short_description, obj_index[r].virt, ac);
    send_to_char(buf,ch);
    o++;
    if (o == 150) { send_to_char("Max number of items hit.\r\n",ch); return eSUCCESS; }
  }
  return eSUCCESS;
}

int do_testhit(char_data *ch, char *argument, int cmd)
{
 char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
 argument = one_argument(argument, arg1);
 argument = one_argument(argument, arg2);
 argument = one_argument(argument, arg3);
  
 if (!arg3[0])
 {
    send_to_char("Syntax: <tohit> <level> <target level>\r\n",ch);
    return eFAILURE;
 }
 int toHit = atoi(arg1), tlevel = atoi(arg3), level = atoi(arg2);
 float lvldiff = level - tlevel;
 if (lvldiff > 15 && lvldiff < 25) toHit += 25;
 else if (lvldiff > 5) toHit += 15;
 else if (lvldiff >= 0) toHit += 10;
 else if (lvldiff >= -5) toHit += 5;

 float lvl = (50.0 - level - tlevel / 2.0)/10.0;

 if (lvl >= 1.0) 
   toHit = (int)(toHit * lvl);

 for (int AC = 100; AC > -1010; AC -= 10)
 {
   float num1 = 1.0 - (-300.0 - (float)AC) * 4.761904762 * 0.0001;
   float num2 = 20.0 + (-300.0 - (float)AC) * 0.0095238095;

   float percent = 30+num1*(float)(toHit)-num2;

   csendf(ch, "%d AC - %f%% chance to hit\r\n", AC, 
			percent);

  }
  return eSUCCESS;

}

void write_array_csv(char * const *array, ofstream &fout) {
	int index = 0;
	const char *ptr = array[index];
	while (*ptr != '\n') {
		fout << ptr << ",";
		ptr = array[++index];
	}
}

int do_export(char_data *ch, char *args, int cmdnum)
{
	char export_type[MAX_INPUT_LENGTH], filename[MAX_INPUT_LENGTH];
	world_file_list_item * curr = obj_file_list;

	args = one_argument(args, export_type);
	one_argument(args, filename);

	if (*export_type == 0 || *filename == 0) {
		send_to_char("Syntax: export obj <filename>\n\r\n\r", ch);
		return eFAILURE;
	}

	ofstream fout;
	fout.exceptions(ofstream::failbit | ofstream::badbit);

	try {
		fout.open(filename, ios_base::out);

		fout << "vnum,name,short_description,description,action_description,type,";
		fout <<	"size,value[0],value[1],value[2],value[3],level,weight,cost,";

		// Print individual array values as columns
		write_array_csv(wear_bits, fout);
		write_array_csv(extra_bits, fout);
		write_array_csv(more_obj_bits, fout);

		fout << "affects" << endl;

		while(curr) {
			for(int x = curr->firstnum; x <= curr->lastnum; x++) {
				write_object_csv((obj_data *)obj_index[x].item, fout);
			}
			curr = curr->next;
		}

		fout.close();
	} catch (ofstream::failure &e) {
	    stringstream errormsg;
	    errormsg << "Exception while writing to " << filename << ".";
	    log(errormsg.str().c_str(), 108, LOG_MISC);
	}

	logf(110, LOG_GOD, "Exported objects as %s.", filename);

	return eSUCCESS;
}
