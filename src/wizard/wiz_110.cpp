/********************************
| Level 110 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"
#include <assert.h>
#include <utility.h>
#include <levels.h>
#include <player.h>
#include <mobile.h>
#include <interp.h>
#include <fileinfo.h>
#include <clan.h>
#include <returnvals.h>
#include <spells.h>

#ifdef WIN32
char *crypt(const char *key, const char *salt);
#else
#include <unistd.h>
extern "C" {
  char *crypt(const char *key, const char *salt);
}
#endif
extern short bport;

// List skill maxes.
int do_maxes(struct char_data *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  class_skill_defines *classskill;
  argument = one_argument(argument, arg);
  one_argument(argument, arg2);
  int oclass = GET_CLASS(ch); // old class.
  if(!has_skill(ch, COMMAND_WHATTONERF)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }
 
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
	  float max = classskill[i].maximum;
	  max *= 0.75;
	  if (classskill[i].attrs[0])
	  {
	       int thing = get_max_stat(ch,classskill[i].attrs[0])-20;
	       if (thing > 0)
	       max += (int)((get_max_stat(ch,classskill[i].attrs[0])-20)*2.5);
	       if (max > 90) max = 90;
	  }
	  if (classskill[i].attrs[1])
	  {
	       int thing = get_max_stat(ch,classskill[i].attrs[1])-20;
	       if (thing > 0)
	        max += (get_max_stat(ch,classskill[i].attrs[1])-20);
	  }

	  csendf(ch, "%s: %d\n\r", classskill[i].skillname, (int)max);
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
   extern bestowable_god_commands_type bestowable_god_commands[];
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
         sprintf(buf, "%22s %s\r\n", bestowable_god_commands[i].name,
                      has_skill(vict, bestowable_god_commands[i].num) ? "YES" : "---");
         send_to_char(buf, ch);
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
   extern bestowable_god_commands_type bestowable_god_commands[];
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
   sprintf(buf, "%s has had %s revoked by %s.\r\n", GET_NAME(vict), 
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

  if(!name) return eFAILURE;

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
   int lev_nr;

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
  char name[160], targetname[160];
  char strsave[MAX_INPUT_LENGTH];
  char * c;
  FILE * fl;
  int iWear;

  half_chop(arg, name, targetname);

  if(!(*name) || !(*targetname)) {
    send_to_char("Usage:  rename <oldname> <newname>\n\r", ch);
    return eFAILURE;
  }

  if(!(victim = get_char_vis(ch, name))) {
    sprintf(name, "%s is not in the game.\r\n", name);
    send_to_char(name, ch);
    return eFAILURE;
  }

  if(GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char("That might just piss them off.\r\n", ch);
    sprintf(name, "%s just tried to rename you.\r\n", GET_NAME(ch));
    send_to_char(name, victim);
    return eFAILURE;
  }

  c = targetname;
  *c = UPPER(*c);
  c++;
  while(*c) {   
    *c = LOWER(*c);
    c++;
  }

  // +1 cause you can actually have 13 char names
  if(strlen(targetname) > (MAX_NAME_LENGTH+1))
  {
    send_to_char("New name too long.\r\n", ch);
    return eFAILURE;
  }

//extern short bport;
  if (!bport)
  sprintf(strsave, "%s/%c/%s", SAVE_DIR, UPPER(targetname[0]), targetname);  
  else
  sprintf(strsave, "%s/%c/%s", BSAVE_DIR, UPPER(targetname[0]), targetname);  
  if((fl = fopen(strsave, "r")))
  {
    fclose(fl);
    sprintf(name, "The name %s is already in use.\r\n", targetname);
    send_to_char(name, ch);
    return eFAILURE;
  }

  for (iWear = 0; iWear < MAX_WEAR; iWear++)
  {
    if (victim->equipment[iWear] && 
        IS_SET(victim->equipment[iWear]->obj_flags.extra_flags, ITEM_SPECIAL))
    {
//      send_to_char("Dammit Valk...this one is wearing GL.\r\n", ch);
	char tmp[256];
      sprintf(tmp,"%s",victim->equipment[iWear]->name);
      tmp[strlen(tmp)-strlen(GET_NAME(victim))-1] = '\0';
      sprintf(tmp,"%s %s",tmp, targetname);
      victim->equipment[iWear]->name = str_hsh(tmp);
	//Not freeing, not sure whether name's hsh'd or dup'd, don't
	// wanna risk. minor leak.
//      return eFAILURE;
    }
  }
 
  obj = victim->carrying;
  while(obj)
  {
    if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
//      send_to_char("Dammit Valk...this one is carrying GL.\r\n", ch);
	char tmp[256];
      sprintf(tmp,"%s",obj->name);
      tmp[strlen(tmp)-strlen(GET_NAME(victim))-1] = '\0';
      sprintf(tmp,"%s %s",tmp, targetname);
      obj->name = str_hsh(tmp);

  //    return eFAILURE;
    }

    obj = obj->next_content;
  }

  do_outcast(victim, GET_NAME(victim), 9);

  do_fsave(ch, GET_NAME(victim), 9);

  // Copy the pfile
if (!bport)
  sprintf(name, "cp %s/%c/%s %s/%c/%s", SAVE_DIR, victim->name[0], GET_NAME(victim),
                                        SAVE_DIR, targetname[0], targetname);
else
  sprintf(name, "cp %s/%c/%s %s/%c/%s", BSAVE_DIR, victim->name[0], GET_NAME(victim),
                                        BSAVE_DIR, targetname[0], targetname);

  system(name);
  // Golems
  sprintf(name, "cp %s/%c/%s.1 %s/%c/%s.1", FAMILIAR_DIR, victim->name[0], GET_NAME(victim),
					   FAMILIAR_DIR, targetname[0], targetname);
  system(name);
  // Golems
  sprintf(name, "cp %s/%c/%s.0 %s/%c/%s.0", FAMILIAR_DIR, victim->name[0], GET_NAME(victim),
                                           FAMILIAR_DIR, targetname[0], targetname);
  system(name);


  sprintf(name, "%s renamed to %s.", GET_NAME(victim), targetname);
  log(name, GET_LEVEL(ch), LOG_GOD);

  // Get rid of the existing one
  do_zap(ch, GET_NAME(victim), 9);

  // load the new guy
  do_linkload(ch, targetname, 9);

  if(!(victim = get_char_vis(ch, targetname)))
  {
    send_to_char("Major problem...coudn't find target after pfile copied.  Notify Urizen immediatly.\r\n", ch);
    return eFAILURE;
  }
  do_name(victim, " %", 9);
  send_to_char("Character now name %'d.\r\n", ch);
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
                 "  ie.. install 291 100 m = installs mob range 29100-29199.\n\r");
    send_to_char(err, ch);
    return eFAILURE;
  }

  if (!(range = atoi(arg1))) {
    sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 291 100 m = installs mob range 29100-29199.\n\r");
    send_to_char(err, ch);
    return eFAILURE;
  }

  if (range <= 0) {
    send_to_char("Range number must be greater than 0\r\n", ch);
    return eFAILURE;
  }

  if (!(numrooms = atoi(arg1))) {
    sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 291 100 m = installs mob range 29100-29199.\n\r");
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
                 "  ie.. install 291 100 m = installs mob range 29100-29199.\n\r");
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
                 "  ie.. install 291 100 m = installs mob range 29100-29199.\n\r", ret);
  }
  send_to_char(err, ch);
  return eSUCCESS;
}

int do_range(struct char_data *ch, char *arg, int cmd)
{
  struct char_data *victim;
  char name[160], buf[160];
  char kind[160], buf2[160];
  char message[256];
  int zonenum, i;
  char ** target;

  world_file_list_item * curr = NULL;

  extern world_file_list_item * world_file_list;
  extern world_file_list_item *   mob_file_list;
  extern world_file_list_item *   obj_file_list;

  if(!has_skill(ch, COMMAND_RANGE)) {
    send_to_char("Huh?\r\n", ch);
    return eFAILURE;
  }

  half_chop(arg, name, buf2);
  half_chop(buf2, kind, buf);

  if(!(*name) || !(*buf) || !(*kind)) {
    send_to_char("Syntax: range <god> <r|m|o> <number|'none'>\n\r", ch);
    return eFAILURE;
  }

  if(!(victim = get_pc_vis(ch, name))) {
    send_to_char("Set whose range?!\n\r", ch);
    return eFAILURE;
  }

  if(!isdigit(*buf)) {
    send_to_char("Choose a valid zone number from 'show wfiles/ofiles/mfiles'.\r\n", ch);
    return eFAILURE;
  }

  switch(*kind) {
    case 'r':  target = &(GET_RANGE(victim));      curr = world_file_list;  break;
    case 'm':  target = &(GET_MOB_RANGE(victim));  curr =   mob_file_list;  break;
    case 'o':  target = &(GET_OBJ_RANGE(victim));  curr =   obj_file_list;  break;
    default:
      send_to_char("Illegal kind of range.   Must be 'r', 'm', or 'o'.\r\n", ch);
      return eFAILURE;
  }
  
  if(!strcmp(buf, "none")) {
    if(*target)
      dc_free(*target);
    target = NULL;
//    FREE(GET_RANGE(victim));
//    GET_RANGE(victim) = NULL;
    sprintf(message, "%s's range has been removed.\r\n", GET_NAME(victim));
    send_to_char(message, ch);
    send_to_char("Your range has been removed.\r\n", victim);
    return eSUCCESS;
  }

  if(!check_range_valid_and_convert(zonenum, buf, 0, 300)) {
    send_to_char("Use a valid range id.\r\n", ch);
    return eFAILURE;
  }

  for(i = 0;
      curr && i != zonenum;
      i++, curr = curr->next)
    ;

  if(!curr) {
    send_to_char("Number too high.  Use 'show rfiles/mfiles/ofiles' to see valid zones.\r\n", ch);
    return eFAILURE;
  }

//  FREE(GET_RANGE(victim));
//  GET_RANGE(victim) = str_dup(curr->filename);
  if(*target)
    dc_free(*target);
  *target = str_dup(curr->filename);

  sprintf(message, "%s %c range set to %s.\r\n", GET_NAME(victim), UPPER(*kind), *target);
  send_to_char(message, ch);
  sprintf(message, "Your %c range has been set to %s.\r\n", UPPER(*kind), *target);
  send_to_char(message, victim);
  return eSUCCESS;
}

extern long long new_meta_platinum_cost(int start, int end);
extern int r_new_meta_platinum_cost(int start, long long plats);
extern int r_new_meta_exp_cost(int start, long long exp);

extern long long moves_exp_spent(char_data * ch);
extern long long moves_plats_spent(char_data * ch);
extern long long hps_exp_spent(char_data * ch);
extern long long hps_plats_spent(char_data * ch);
extern long long mana_exp_spent(char_data * ch);
extern long long mana_plats_spent(char_data * ch);



int do_thing(CHAR_DATA *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  argument = one_argument(argument, arg);
  if (arg[0] == '\0' || !(victim = get_pc_vis(ch, arg)))
  {
     send_to_char("Do_the_thing who?\r\n",ch);
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
  
  return eSUCCESS;
}
