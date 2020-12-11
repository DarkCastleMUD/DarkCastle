/**********************
| Level 102 wizard commands
| 11/20/95 -- Azrack
**********************/
/*****************************************************************************/
/* Revision History                                                          */
/* 12/04/2003   Onager  Fixed find_skill() to find skills/spells consisting  */
/*                      of multi-word space-separated names                  */
/*                      Fixed "remove" action of do_sedit() to remove skill  */
/*                      from player (instead of invoking god ;-)             */
/* 12/08/2003   Onager  Fixed do_sedit() to allow setting of multi-word      */
/*                      skills                                               */
/*****************************************************************************/

#include <ctype.h> // isspace
#include <string>
#include <vector>
#include <limits>
#include <type_traits>

#include "wizard.h"
#include "utility.h"
#include "connect.h"
#include "player.h"
#include "room.h"
#include "obj.h"
#include "mobile.h"
#include "levels.h"
#include "interp.h"
#include "obj.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "race.h"
#include "returnvals.h"
#include "vault.h"
#include "set.h"
#include "structs.h"
#include "guild.h"

using std::vector;
using std::string;

extern vector<string> continent_names;
extern CVoteData *DCVote;

// Urizen's rebuild rnum references to enable additions to mob/obj arrays w/out screwing everything up.
// A hack of renum_zone_tables *yawns*
// type 1 = mobs, type 2 = objs. Simple as that.
// This should obviously not be called at any other time than additions to the previously mentioned
// arrays, as it'd screw things up. 
// Saving zones after this SHOULD not be required, as the old savefiles contain vnums, which should remain correct.
void rebuild_rnum_references(int startAt, int type)
{
    int zone, comm;

    for (zone = 0; zone <= top_of_zone_table; zone++) {
      for (comm = 0; zone_table[zone].cmd && zone_table[zone].cmd[comm].command != 'S'; comm++)
	{
	  switch(zone_table[zone].cmd[comm].command)
	    {
	    case 'M':
	      if (type==1 && zone_table[zone].cmd[comm].arg1 >= startAt)
		zone_table[zone].cmd[comm].arg1++;
	      break;
	    case 'O':
	      if (type==2 && zone_table[zone].cmd[comm].arg1 >= startAt)
		zone_table[zone].cmd[comm].arg1++;
	      break;
	    case 'G':
	      if (type==2 && zone_table[zone].cmd[comm].arg1 >= startAt)
		zone_table[zone].cmd[comm].arg1++;
	      break;
	    case 'E':
	      if (type==2 && zone_table[zone].cmd[comm].arg1 >= startAt)
		zone_table[zone].cmd[comm].arg1++;
	      break;
	    case 'P':
	      if (type==2 && zone_table[zone].cmd[comm].arg1 >= startAt)
		zone_table[zone].cmd[comm].arg1++;
	      if (type==2 && zone_table[zone].cmd[comm].arg3 >= startAt)
		zone_table[zone].cmd[comm].arg3++;
	      break;
	    case '%':
	    case 'K':
	    case 'D':
	    case 'X':
	    case '*':
	    case 'J':
	      break;
	    default:
	      log("Illegal char hit in rebuild_rnum_references", 0, LOG_WORLD);
	      break;
	    }
	}
    }
}

int do_check(struct char_data *ch, char *arg, int cmd) {
  struct descriptor_data d;
  struct char_data *vict;
  int connected = 1;
  char buf[120];
  char tmp_buf[160];
  char *c;

  extern struct race_shit race_info[];
  extern char *pc_clss_types[]; 
  // extern char *punish_bits[];

  while(isspace(*arg))
    arg++;

  if(!*arg) {
    send_to_char("Check who?\n\r", ch);
    return eFAILURE;
  }

  one_argument(arg, buf);
  buf[0] = UPPER(buf[0]);
  if(!(vict = get_pc_vis_exact(ch, buf))) {
    connected = 0;

    c = buf;
    *c = UPPER(*c);
    c++;
    while(*c) { 
      *c = LOWER(*c);
      c++;
    }

    // must be done to clear out "d" before it is used
    memset((char *) &d, 0, sizeof(struct descriptor_data));
    if(!(load_char_obj(&d, buf))) {
      
      sprintf(tmp_buf, "../archive/%s.gz", buf);
      if(file_exists(tmp_buf))
         send_to_char("Character is archived.\r\n", ch);
      else send_to_char("Unable to load! (Character might not exist...)\n\r", ch);
      return eFAILURE; 
    } 

    vict = d.character;
    vict->desc = 0;

    redo_hitpoints(vict);
    redo_mana(vict);
    if(!GET_TITLE(vict))
      GET_TITLE(vict) = str_dup("is a virgin");
    if(GET_CLASS(vict) == CLASS_MONK)
      GET_AC(vict) -= GET_LEVEL(vict) * 3;
    isr_set(vict);
  }

  sprintf(buf, "$3Short Desc$R: %s\n\r", GET_SHORT(vict));
  send_to_char(buf, ch); 
  sprintf(buf, "$3Race$R: %-9s $3Class$R: %-9s $3Level$R: %-8d $3In Room$R: %d\n\r",
          race_info[(int)(GET_RACE(vict))].singular_name,
          pc_clss_types[(int)(GET_CLASS(vict))], GET_LEVEL(vict),
          (connected ? world[vict->in_room].number : -1));
  send_to_char(buf, ch);  
  sprintf(buf, "$3Exp$R: %-10lld $3Gold$R: %-10lld $3Bank$R: %-9d $3Align$R: %d\n\r",
          GET_EXP(vict), GET_GOLD(vict), GET_BANK(vict), GET_ALIGNMENT(vict));
  send_to_char(buf, ch);
  if(GET_LEVEL(ch) >= SERAPH) {
    sprintf(buf, "$3Load Rm$R: %-5d  $3Home Rm$R: %-5hd  $3Platinum$R: %d  $3Clan$R: %d\n\r",
      world[vict->in_room].number, vict->hometown, GET_PLATINUM(vict), GET_CLAN(vict));
    send_to_char(buf, ch);
  }
  sprintf(buf, "$3Str$R: %-2d  $3Wis$R: %-2d  $3Int$R: %-2d  $3Dex$R: %-2d  $3Con$R: %d\n\r",
          GET_STR(vict), GET_WIS(vict), GET_INT(vict), GET_DEX(vict),
          GET_CON(vict));
  send_to_char(buf, ch);
  sprintf(buf, "$3Hit Points$R: %d/%d $3Mana$R: %d/%d $3Move$R: %d/%d $3Ki$R: %d/%d\n\r",
          GET_HIT(vict), GET_MAX_HIT(vict), GET_MANA(vict),
          GET_MAX_MANA(vict), GET_MOVE(vict), GET_MAX_MOVE(vict),
          GET_KI(vict), GET_MAX_KI(vict));
  send_to_char(buf, ch);

  if(GET_LEVEL(ch) >= OVERSEER && !IS_MOB(vict) && GET_LEVEL(ch) >= GET_LEVEL(vict))
  {
    sprintf(buf, "$3Last connected from$R: %s\n\r", vict->pcdata->last_site);
    send_to_char(buf, ch);

    /* ctime adds a \n to the string it returns! */
    sprintf(buf, "$3Last connected on$R: %s\r", ctime(const_cast<const time_t *>(&vict->pcdata->time.logon)));
    send_to_char(buf, ch);
  }

  display_punishes(ch, vict);

  if(connected) 
    if(vict->desc) {
      if(GET_LEVEL(ch) >= OVERSEER && GET_LEVEL(ch) >= GET_LEVEL(vict))
      {
        sprintf(buf, "$3Connected from$R: %s\n\r", vict->desc->host);
        send_to_char(buf, ch);
      }
      else 
      {
        sprintf(buf, "Connected.\r\n");
        send_to_char(buf, ch);
      }
    }
    else
      send_to_char("(Linkdead)\n\r", ch); 
  else { 
    send_to_char("(Not on game)\n\r", ch);
    free_char(vict); 
  }
  return eSUCCESS;
}

int do_find(struct char_data *ch, char *arg, int cmd)
{
  char type[MAX_INPUT_LENGTH];
  char name[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int x;

  char *types[] = {
    "mob",
    "pc",
    "char",
    "obj",
    "\n"
  };

  if(IS_NPC(ch))
    return eFAILURE;
  if(!has_skill(ch, COMMAND_FIND)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }

  half_chop(arg, type, name);

  if(!*type || !*name) {
    send_to_char("Usage:  find <mob|pc|char|obj> <name>\n\r", ch);
    return eFAILURE;
  }

  for(x = 0; x <= 4; x++) {
     if (x == 4) {
        send_to_char("Type must be one of these: mob, pc, char, obj.\n\r",
	             ch);
        return eFAILURE;
        }
     if (is_abbrev(type, types[x]))
        break; 
     }

  switch(x) {
    default:
      send_to_char("Problem...fuck up in do_find.\n\r", ch);
      log("Default in do_find...should NOT happen.", ANGEL, LOG_BUG);
      return eFAILURE;
    case 0 :  // mobile 
      return do_mlocate(ch, name, 9); 
    case 1 :  // pc 
      break;
    case 2 :  // character
      return do_mlocate(ch, name, 18); 
    case 3 :  // object 
      return do_olocate(ch, name, 9); 
  }

  if(!(vict = get_pc_vis(ch, name))) {
    send_to_char("Unable to find that character.\n\r", ch);
    return eFAILURE;
  }

  sprintf(type, "%30s -- %s [%d]\n\r", GET_SHORT(vict),
          world[vict->in_room].name, world[vict->in_room].number);
  send_to_char(type, ch); 
  return eSUCCESS;
}

int do_stat(struct char_data *ch, char *arg, int cmd)
{
  struct descriptor_data d;
  struct char_data *vict;
  struct obj_data *obj;
  char type[MAX_INPUT_LENGTH];
  char name[MAX_INPUT_LENGTH];
  char *c;
  int x;

  char *types[] = {
    "mobile",
    "object",
    "character"
  };
  if(!has_skill(ch, COMMAND_STAT)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }

  if(IS_NPC(ch))
    return eFAILURE;

  half_chop(arg, type, name);

  if(!*type || !*name) {
    send_to_char("Usage:  stat <mob|obj|char> <name>\n\r", ch);
    return eFAILURE;
  }

  for(x = 0; x <= 3; x++) {
     if(x == 3) {
       send_to_char("Type must be one of these: mobile, object, "
                    "character.\n\r", ch);
       return eFAILURE;
     }
     if(is_abbrev(type, types[x]))
       break; 
  }

  switch(x) {
    default:
      send_to_char("Problem...fuck up in do_stat.\n\r", ch);
      log("Default in do_stat...should NOT happen.", ANGEL, LOG_BUG);
      return eFAILURE;
    case 0 :  // mobile
      if((vict = get_mob_vis(ch, name))) {
        mob_stat(ch, vict);
        return eFAILURE;
      }
      send_to_char("No such mobile.\n\r",ch);
      return eFAILURE;
    case 1 :  // object
      if(!(obj = get_obj_vis(ch, name))) {
        send_to_char("No such object.\n\r",ch);
        return eFAILURE;
      }
      obj_stat(ch, obj);
      return eFAILURE;
    case 2 :  // character
      break;
  }

  if(!(vict = get_pc_vis(ch, name))) {
    c = name;
    *c = UPPER(*c);
    c++;
    while(*c) { 
      *c = LOWER(*c);
      c++;
    }

    // must be done to clear out "d" before it is used
    memset((char *) &d, 0, sizeof(struct descriptor_data));
    if(!(load_char_obj(&d, name))) {
      send_to_char("Unable to load! (Character might not exist...)\n\r", ch);
      return eFAILURE; 
    } 

    vict = d.character;
    vict->desc = 0;

    redo_hitpoints(vict);
    redo_mana(vict);
    if(!GET_TITLE(vict))
      GET_TITLE(vict) = str_dup("is a virgin");
    if(GET_CLASS(vict) == CLASS_MONK)
      GET_AC(vict) -= GET_LEVEL(vict) * 3;
    isr_set(vict);

    char_to_room(vict, ch->in_room);
    mob_stat(ch, vict);
    char_from_room(vict);
    free_char(vict);
    return eSUCCESS;;
  }

  mob_stat(ch, vict);
  return eSUCCESS;
}

int do_mpstat(struct char_data *ch, char *arg, int cmd)
{
  struct char_data *vict;
  char name[MAX_INPUT_LENGTH];
  int x;

  void mpstat(char_data * ch, char_data * victim);

  if(IS_NPC(ch))
    return eFAILURE;

  if(!has_skill(ch, COMMAND_MPSTAT)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }

  //  int has_range = has_skill(ch, COMMAND_RANGE);

  one_argument(arg, name);

  if(!*name) {
    send_to_char("Usage:  procstat <name|num>\n\r", ch);
    return eFAILURE;
  }

  if(isdigit(*name)) {
    if(!(x = atoi(name))) {
      send_to_char("That is not a valid number.\r\n", ch);
      return eFAILURE;
    }
    x = real_mobile(x);

    if(x < 0) {
      send_to_char("No mob of that number.\r\n", ch);
      return eFAILURE;
    }
  }
  else {
    if(!(vict = get_mob_vis(ch, name))) 
    {
      send_to_char("No such mobile.\n\r",ch);
      return eFAILURE;
    }
    x = vict->mobdata->nr;
  }
/*
  if(!has_range)
  {
    if(!can_modify_mobile(ch, mob_index[x].virt)) {
      send_to_char("You are unable to work creation outside of your range.\n\r", ch);
      return eFAILURE;
    }
  }
*/
  mpstat(ch, (char_data *) mob_index[x].item);
  return eSUCCESS;
}

int do_zone_single_edit(struct char_data * ch, char * argument, int zone)
{
  char cmdnum[MAX_INPUT_LENGTH];
  char select[MAX_INPUT_LENGTH];
  char last[MAX_INPUT_LENGTH];
  long i, j;
  int cmd = 0;

  argument = one_argument(argument, cmdnum);  
  half_chop(argument, select, last);
  int max_res(int zone);
  if(*cmdnum && !check_range_valid_and_convert(cmd, cmdnum, 1, max_res(zone))) {
    send_to_char("That is not a valid command number.\r\n", ch);
    return eFAILURE;
  }
  cmd--;  // since we show user list starting at 1

  if(*select && *last && cmd > -1)  
  {
    if(isname(select, "type"))
    {
       char result = toupper(*last);

       switch(toupper(result)) {
         case 'M':  
         case 'O':
         case 'P':
         case 'G':
         case 'E':
         case 'D':
         case 'K':
         case 'X':
	 case '*':
	   zone_table[zone].cmd[cmd].arg1 = 0;
	   zone_table[zone].cmd[cmd].arg2 = 0;
	   zone_table[zone].cmd[cmd].arg3 = 0;
	   /* no break */
         case '%':
	   zone_table[zone].cmd[cmd].arg2 = 100;
           zone_table[zone].cmd[cmd].command = result;
           sprintf(select, "Type for command %d changed to %c.\r\nArg1-3 reset.\r\n", cmd+1, result);
           send_to_char(select, ch);
           break;
         default:
           send_to_char("Type must be:  M, O, P, G, E, D, X, K, *, or %.\r\n", ch);
           break;
       }
    }
    else if(isname(select, "if"))
    {
       switch(*last) {
         case '0': zone_table[zone].cmd[cmd].if_flag = 0;
           sprintf(select, "If flag for command %d changed to 0 (always).\r\n", cmd+1);
           break;
         case '1': zone_table[zone].cmd[cmd].if_flag = 1;
           sprintf(select, "If flag for command %d changed to 1 ($B$2ontrue$R).\r\n", cmd+1);
           break;
         case '2': zone_table[zone].cmd[cmd].if_flag = 2;
           sprintf(select, "If flag for command %d changed to 2 ($B$4onfalse$R).\r\n", cmd+1);
           break;
         case '3': zone_table[zone].cmd[cmd].if_flag = 3;
           sprintf(select, "If flag for command %d changed to 3 ($B$5onboot$R).\r\n", cmd+1);
           break;
         case '4': zone_table[zone].cmd[cmd].if_flag = 4;
           sprintf(select, "If flag for command %d changed to 4 if-last-mob-true ($B$2Ls$1Mb$2Tr$R).\r\n", cmd+1);
           break;
         case '5': zone_table[zone].cmd[cmd].if_flag = 5;
           sprintf(select, "If flag for command %d changed to 5 if-last-mob-false ($B$4Ls$1Mb$4Fl$R).\r\n", cmd+1);
           break;
         case '6': zone_table[zone].cmd[cmd].if_flag = 6;
           sprintf(select, "If flag for command %d changed to 6 if-last-obj-true ($B$2Ls$7Ob$2Tr$R).\r\n", cmd+1);
           break;
         case '7': zone_table[zone].cmd[cmd].if_flag = 7;
           sprintf(select, "If flag for command %d changed to 7 if-last-obj-false ($B$4Ls$7Ob$4Fl$R).\r\n", cmd+1);
           break;
         case '8': zone_table[zone].cmd[cmd].if_flag = 8;
           sprintf(select, "If flag for command %d changed to 8 if-last-%%-true ($B$2Ls$R%%%%$B$2Tr$R).\r\n", cmd+1);
           break;
         case '9': zone_table[zone].cmd[cmd].if_flag = 9;
           sprintf(select, "If flag for command %d changed to 9 if-last-%%-false ($B$4Ls$R%%%%$B$4Fl$R).\r\n", cmd+1);
           break;
         default:
           send_to_char("Legal values are 0 (always), 1 (ontrue), 2 (onfalse), 3 (onboot),\r\n"
                        "                 4 (if-last-mob-true),   5 (if-last-mob-false),\r\n"
                        "                 6 (if-last-obj-true),   7 (if-last-obj-false),\r\n"
                        "                 8 (if-last-%%-true),    9 (if-last-%%-false)\r\n", ch);
           return eFAILURE;
       }
       send_to_char(select, ch);
    }
    else if(isname(select, "comment")) {
//      This is str_hsh'd, don't delete it
//      if(zone_table[zone].cmd[cmd].comment)
//        dc_free(zone_table[zone].cmd[cmd].comment);
      if(!strcmp(last, "none")) {
        zone_table[zone].cmd[cmd].comment = NULL;
        sprintf(select, "Comment for command %d removed.\r\n", cmd+1);
        send_to_char(select, ch);
      } else {
        zone_table[zone].cmd[cmd].comment = str_hsh(last);
        sprintf(select, "Comment for command %d change to '%s'.\r\n", cmd+1, 
                      zone_table[zone].cmd[cmd].comment);
        send_to_char(select, ch);
      }
    }
    else
    { 
      if(*last == '0')
        i = 0;
      else if(!(i = atoi(last)))
      {
        send_to_char("That was not a valid number for an argument.\r\n", ch);
        return eFAILURE;
      }

      if(isname(select, "1"))
      {
        switch(zone_table[zone].cmd[cmd].command) {
          case 'M':
            j = real_mobile(i);
            break;  
          case 'P':
          case 'G':
          case 'O':
          case 'E':
            j = real_object(i);
            break;
          case 'X':
          case 'K':
          case 'D':
          case '%':
          case 'J':
          case '*':
          default:
            j = i;
            break;
        }
        zone_table[zone].cmd[cmd].arg1 = j;
        sprintf(select, "Arg 1 set to %ld.\r\n", i);
        send_to_char(select, ch);
      }
      else if(isname(select, "2"))
      {
        switch(zone_table[zone].cmd[cmd].command) {
          case 'K':
          case 'X':
            send_to_char("There is no arg2 for X commands.\r\n", ch);
            return eFAILURE;
          default: break;
        }
        zone_table[zone].cmd[cmd].arg2 = i;
        sprintf(select, "Arg 2 set to %ld.\r\n", i);
        send_to_char(select, ch);
      }
      else if(isname(select, "3"))
      {
        switch(zone_table[zone].cmd[cmd].command) {
          case 'M':
          case 'O':
            i = real_room(i);
            break;
          case '%':
            send_to_char("There is no arg3 for % commands.\r\n", ch);
            return eFAILURE;
          case 'K':
          case 'X':
            send_to_char("There is no arg3 for X commands.\r\n", ch);
            return eFAILURE;
          case 'P':
            i = real_object(i);
            break;
          case 'G':
          case 'E':
          case 'D':
          case 'J':
          default:
            break;
        }
        zone_table[zone].cmd[cmd].arg3 = i;
        sprintf(select, "Arg 3 set to %ld.\r\n", i);
        send_to_char(select, ch);
      }
    }
    return eFAILURE;
  }

  send_to_char("$3Usage$R:  zedit edit <cmdnumber> <type|if|1|2|3|comment> <value>\r\n"
               "Valid types are:   'M', 'O', 'P', 'G', 'E', 'D', '*', 'X', 'K', and '%'.\r\n"
               "Valid ifs are:     0(always), 1(ontrue), 2(onfalse), 3(onboot).\r\n"
               "Valid args (123):  0-32000\r\n"
               "                   (for max-in-world, -1 represents 'always')\r\n", ch);
  return eFAILURE;
}
int max_res(int zone)
{
 int i;
  for(i = 0 ;
      zone_table[zone].cmd[i].command != 'S' && i < (zone_table[zone].reset_total-1) ;
      i++)
  ;
 return i;


}

int do_zedit(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  char select[MAX_INPUT_LENGTH];
  char text[MAX_INPUT_LENGTH];
  int16 skill;
  int i = 0, j = 0;
  int zone, last_cmd;
  int robj, rmob;
  int show_zone_commands(struct char_data * ch, int i, int start = 0);
  extern char *zone_modes[];
  extern int top_of_zonet;

  char * zedit_values[] = {
    "remove", "add", "edit", "list", "name", 
    "lifetime", "mode", "flags", "help", "search", 
    "swap", "copy", "continent",
    "\n"
  };

  argument = one_argumentnolow(argument, select);
      
  if(!(*select)) {
    send_to_char("$3Usage$R: zedit <field> <correct arguments>\r\n"
                 "You must be _in_ the zone you wish to edit.\r\n"
                 "Fields are the following.\r\n"
                 , ch);
    display_string_list(zedit_values, ch);
    return eFAILURE;
  }

  for(skill = 0 ;; skill++)
  {
    if(zedit_values[skill][0] == '\n')
    {
      send_to_char("Invalid field.\n\r", ch);
      return eFAILURE;
    }
    if(is_abbrev(select, zedit_values[skill]))
      break;
  }

  if(!can_modify_room(ch, ch->in_room)) {
    send_to_char("You are unable to modify a zone other than the one your room-range is in.\n\r", ch);
    return eFAILURE;
  }
  int from, to;
  // set the zone we're in
  zone = world[ch->in_room].zone;

  // set the index of the S command in that zone 
  // have to go from bottom instead of top-counting down since we could have more than
  // one 'S' at the end from deletes
  for(i = 0 ;
      zone_table[zone].cmd[i].command != 'S' && i < (zone_table[zone].reset_total-1) ;
      i++)
  ;
  last_cmd = i;

  switch(skill) {
    case 0: /* remove */
    {
      argument = one_argumentnolow(argument, text);
      if(!(*text))
      {
        send_to_char("$3Usage$R: zedit remove <zonecmdnumber>\r\n"
                     "This will remove the command from the zonefile.\r\n", ch);
        return eFAILURE;
      }

      i = atoi(text);
      if(!i || i > last_cmd)
      {
        sprintf(buf, "Invalid number '%s'.  <zonecmdnumber> must be between 1 and %d.\r\n",
                     text, last_cmd-1);
        send_to_char(buf, ch);
        return eFAILURE;
      }

      // j = i-1 because the user sees arrays starting at 1
      for(j = i-1; zone_table[zone].cmd[j].command != 'S'; j++) {
			auto &character_list = DC::instance().character_list;
			for (auto& tmp_vict : character_list) {
			if (IS_MOB(tmp_vict) && tmp_vict->mobdata && tmp_vict->mobdata->reset == &zone_table[zone].cmd[j]) {
				if (zone_table[zone].cmd[j+1].command != 'S') {
					tmp_vict->mobdata->reset = &zone_table[zone].cmd[j+1];
				} else {
					tmp_vict->mobdata->reset = NULL;
				}
			}
        }
        zone_table[zone].cmd[j] = zone_table[zone].cmd[j+1];
      }

      sprintf(buf, "Command %d removed.  Table reformatted.\n\r", i);
      send_to_char(buf, ch);
      break;
    }

    case 1: /* add */
    {
      argument = one_argumentnolow(argument, text);
      if(!(*text))
      {   
        send_to_char("$3Usage$R: zedit add <zonecmdnumber|new>\r\n"
                     "This will add a new command to the zonefile.\r\n"
                     "Adding 'new' will add a command to the end.\r\n"
                     "Adding a number, will insert a new command at that place in\r\n"
                     "the list, pushing the rest of the items back.\r\n", ch);
        return eFAILURE;
      }
      
      if( ( 
            !(i = atoi(text)) || i > last_cmd
          ) && 
            !isname(text, "new")
        )
      {
        sprintf(buf, "You must state either 'new' or the insertion point which must be between 0 and %d.\n\r"
                     "'%s' is not valid.\n\r", zone_table[zone].reset_total, text);
        send_to_char(buf, ch);
        return eFAILURE;
      }
      
      // if the zone memory is full, allocate another 10 commands worth       
      if(last_cmd >= (zone_table[zone].reset_total-2))
      {
        zone_table[zone].cmd = (struct reset_com *)
            realloc(zone_table[zone].cmd, (zone_table[zone].reset_total + 10) * sizeof(struct reset_com));
        zone_table[zone].reset_total += 10;
      }
      
      if(i)
      {
        // bump everything up a slot
        for(j = last_cmd; j != (i-2) ; j--)
          zone_table[zone].cmd[j+1] = zone_table[zone].cmd[j];
        // set up the 'J'
	zone_table[zone].cmd[i-1].active = 1;
        zone_table[zone].cmd[i-1].command = 'J';
        zone_table[zone].cmd[i-1].if_flag = 0;
        zone_table[zone].cmd[i-1].arg1 = 0;   
        zone_table[zone].cmd[i-1].arg2 = 0;   
        zone_table[zone].cmd[i-1].arg3 = 0;   
        zone_table[zone].cmd[i-1].comment = NULL;
        sprintf(buf, "New command 'J' added at %d.\r\n", i);
      }
      else // tack it on the end
      {
        // bump the 'S' up
        zone_table[zone].cmd[last_cmd+1] = zone_table[zone].cmd[last_cmd];
        // set up the 'J'
	zone_table[zone].cmd[last_cmd].active = 1;
        zone_table[zone].cmd[last_cmd].command = 'J';
        zone_table[zone].cmd[last_cmd].if_flag = 0;
        zone_table[zone].cmd[last_cmd].arg1 = 0;   
        zone_table[zone].cmd[last_cmd].arg2 = 0;   
        zone_table[zone].cmd[last_cmd].arg3 = 0;   
        zone_table[zone].cmd[last_cmd].comment = NULL;
        sprintf(buf, "New command 'J' added at %d.\r\n", last_cmd+1);
      } 
      send_to_char(buf, ch);
      break;
    }
    case 2: /* edit */
    {
      do_zone_single_edit(ch, argument, zone);
      break;
    }
    case 3: /* list */
    {
      argument = one_argumentnolow(argument, text);
      if(!*text)
      {
        show_zone_commands(ch, zone);
      }
      else {
        if((i = atoi(text)) < 1)
        {
          send_to_char("Use zedit list <number>.\r\n", ch);
          return eFAILURE;
        }    
        show_zone_commands(ch, zone, i-1);
      }
      send_to_char("To see commands higher than 20, use zedit list <cmdnumber>.\r\n", ch);

      return eSUCCESS; // so we don't set_zone_modified_zone
    }
    case 4: /* name */
    {
      if(!*argument)
      {   
        send_to_char("$3Usage$R: zedit name <newname>\r\n"
                     "This changes the name of the zone.\r\n", ch);
        return eFAILURE;
      }
      
      dc_free(zone_table[zone].name);
      zone_table[zone].name = str_dup(argument);

      sprintf(buf, "Zone %d's name changed to '%s'.\r\n", zone, argument);
      send_to_char(buf, ch);
      break;
    }
    case 5: /* lifetime */
    {
      argument = one_argumentnolow(argument, text);
      if(!*text)
      {   
        send_to_char("$3Usage$R: zedit lifetime <tickamount>\r\n"
                     "The lifetime is the number of ticks the zone takes\r\n"
                     "before it will attempt to repop itself.\r\n", ch);
        return eFAILURE;
      }
      
      if(!(i = atoi(text)) || i > 32000)
      {
        send_to_char("You much choose between 1 and 32000.\r\n", ch);
        return eFAILURE;
      }
 
      zone_table[zone].lifespan = i;

      sprintf(buf, "Zone %d's lifetime changed to %d.\r\n", zone, i);
      send_to_char(buf, ch);
      break;
    }
    case 6: /* mode */
    {
      argument = one_argumentnolow(argument, text);
      if(!*text)
      {   
        send_to_char("$3Usage$R: zedit mode <modetype>\r\n"
                     "You much choose the rule the zone follows when it\r\n"
                     "attempts to repop itself.  Available modes are:\r\n", ch);
        *buf = '\0';
        for(j = 0; *zone_modes[j] != '\n'; j++)
          sprintf(buf, "%s  $C%d$R: %s\r\n", buf, j+1, zone_modes[j]);

        send_to_char(buf, ch);          
        return eFAILURE;
      }
      
      if(!(i = atoi(text)) || i > 3)
      {
        send_to_char("You much choose between 1 and 3.\r\n", ch);
        return eFAILURE;
      }
 
      zone_table[zone].reset_mode = i-1;

      sprintf(buf, "Zone %d's reset mode changed to %s(%d).\r\n", zone, zone_modes[i-1], i);
      send_to_char(buf, ch);
      break;
    }
    case 7: /* flags */
    {
      argument = one_argumentnolow(argument, text);
      if(!*text)
      {   
        send_to_char("$3Usage$R: zedit flags <noteleport|noclaim|nohunt>\r\n",ch);
        return eFAILURE;
      }
      
	if (!str_cmp(text, "noclaim"))
	{
		TOGGLE_BIT(zone_table[zone].zone_flags, ZONE_NOCLAIM);
		if (IS_SET(zone_table[zone].zone_flags, ZONE_NOCLAIM))
		send_to_char("Noclaim turned on.\r\n",ch);
		else
		send_to_char("Noclaim turned off.\r\n",ch);
	}
	

      else if(!strcmp(text, "noteleport")) {
		TOGGLE_BIT(zone_table[zone].zone_flags, ZONE_NO_TELEPORT);
		if (IS_SET(zone_table[zone].zone_flags, ZONE_NO_TELEPORT))
		send_to_char("Noteleport turned on.\r\n",ch);
		else
		send_to_char("Noteleport turned off.\r\n",ch);
      }
      else if(!strcmp(text, "nohunt")) {
		TOGGLE_BIT(zone_table[zone].zone_flags, ZONE_NOHUNT);
		if (IS_SET(zone_table[zone].zone_flags, ZONE_NOHUNT))
		send_to_char("Nohunt turned on.\r\n",ch);
		else
		send_to_char("Nohunt turned off.\r\n",ch);
      }
      else {
        sprintf(buf, "'%s' invalid.  Enter 'noclaim', 'noteleport' or 'nohunt'.\r\n", text);
        send_to_char(buf, ch);
        return eFAILURE;
      }
 
      sprintf(buf, "Zone %d's lifetime changed to %s.\r\n", zone, 
                    zone_table[zone].zone_flags&1 ? "true" : "false");
      send_to_char(buf, ch);
      break;
    }
    case 8: /* help */
    {
      send_to_char("\r\n"
                   "Most commands will give you help on their own, just type them\r\n"
                   "without any arguments.\r\n\r\n"
                   "Details on zone command types:\r\n"
                   "   M = load mob          O = load object\r\n"
                   "   P = put obj in obj    G = give item to mob\r\n"
                   "   E = equip item on mob D = set door\r\n"
                   "   * = comment           % = be true on x times out of y\r\n"
                   "   X = set a true-false flag to an 'unsure' state\n\r"
                   "   K = skip the next [arg1] number of commands.\r\n"
                   "\r\n"
                   "For comments, if you wish to remove a comment set the comment to 'none'.\r\n"
                   "\r\n"
                   , ch);
      return eSUCCESS; // so we don't set modified
    }
    case 9: /* search */
    {
      argument = one_argumentnolow(argument, text);
      if(!*text) {
        send_to_char("$3Usage$R: zedit search <number>\r\n"
                     "This searches your current zonefile for any commands\r\n"
                     "containing that number.  It then lists them to you.  If you're\r\n"
                     "a deity or higher it will also warn you if that number appears\r\n"
                     "in any other zonefiles.\n\r", ch);
        return eFAILURE;
      }

      if(!check_valid_and_convert(j, text)) {
        send_to_char("Please specifiy a valid number.\r\n", ch);
        return eFAILURE;
      }

      send_to_char("Matches:\r\n", ch);

      robj = real_object(j);
      rmob = real_mobile(j);

      // If that obj/mob doesn't exist, put a junk value that should never (hopefully) match
      if(robj == -1)
        robj = -93294;
      if(rmob == -1)
        rmob = -93294;

      for(int k = 0; k <= top_of_zonet; k++)
      {
        if(!has_skill(ch, COMMAND_RANGE) && zone != k)
           continue;
	
        for(i = 0; i < zone_table[k].reset_total; i++)
          switch(zone_table[k].cmd[i].command) {
            case 'M':
              if( rmob == zone_table[k].cmd[i].arg1 )
                csendf(ch, " Zone %d  Command %d (%c)\r\n", k, i+1, zone_table[k].cmd[i].command);
              break;
            case 'G':   // G, E, and O have obj # in arg1
            case 'E':
            case 'O':
              if( robj == zone_table[k].cmd[i].arg1 )
                csendf(ch, " Zone %d  Command %d (%c)\r\n", k, i+1, zone_table[k].cmd[i].command);
              break;
            case 'P':  // P has obj # in arg1 and arg3
              if( robj == zone_table[k].cmd[i].arg1 ||
                  robj == zone_table[k].cmd[i].arg3)
                csendf(ch, " Zone %d  Command %d (%c)\r\n", k, i+1, zone_table[k].cmd[i].command);
              break;
            case 'J':  // J could be any
              if( robj == zone_table[k].cmd[i].arg1 ||
                  rmob == zone_table[k].cmd[i].arg1 ||
                  robj == zone_table[k].cmd[i].arg2 ||
                  rmob == zone_table[k].cmd[i].arg2 ||
                  robj == zone_table[k].cmd[i].arg3 ||
                  rmob == zone_table[k].cmd[i].arg3)
                csendf(ch, " Zone %d  Command %d (%c)\r\n", k, i+1, zone_table[k].cmd[i].command);
            default:
              break;
          }
      }
      return eSUCCESS;
    }

    case 10: // swap
    {
      half_chop(argument, select, text);
      if(!*text || !*select)
      {   
        send_to_char("$3Usage$R: zedit swap <cmd1> <cmd2>\r\n"
                     "This swaps the positions of two zone commands.\r\n", ch);
        return eFAILURE;
      }

      if(!check_range_valid_and_convert(i, select, 1, last_cmd)) {
        send_to_char("Invalid command num for cmd1.\r\n", ch);
        return eFAILURE;
      }
      if(!check_range_valid_and_convert(j, text, 1, last_cmd)) {
        send_to_char("Invalid command num for cmd2.\r\n", ch);
        return eFAILURE;
      }

      // swap i and j
      struct reset_com temp_com;

      temp_com = zone_table[zone].cmd[i-1];
      zone_table[zone].cmd[i-1] = zone_table[zone].cmd[j-1];
      zone_table[zone].cmd[j-1] = temp_com;

      csendf(ch, "Commands %d and %d swapped.\r\n", i, j);
      break;
    }
    case 11: // copy
    {
      argument = one_argumentnolow(argument, text);
      char arg[MAX_INPUT_LENGTH];
      argument = one_argumentnolow(argument, arg);
      if(!text[0] || !is_number(text))
      {
        send_to_char("$3Usage$R: zedit copy <source line> <destination line>\r\nDestination line is optional. If no such line exists, it tacks it on at the end.", ch);
        return eFAILURE;
      }
      from = atoi(text)-1;
      to = 0;
      if (arg[0]) to = atoi(arg);

      if (from > last_cmd || from < 0)
      {
        sprintf(buf, "Source line must be between 0 and %d.\n\r"
                     "'%s' is not valid.\n\r", zone_table[zone].reset_total, text);
        send_to_char(buf, ch);
        return eFAILURE;
      }

      // if the zone memory is full, allocate another 10 commands worth
      if(last_cmd >= (zone_table[zone].reset_total-2))
      {
        zone_table[zone].cmd = (struct reset_com *)
            realloc(zone_table[zone].cmd, (zone_table[zone].reset_total + 10) * sizeof(struct reset_com));
        zone_table[zone].reset_total += 10;
      }

      struct reset_com tmp = zone_table[zone].cmd[from];
      if(to)
      {
        // bump everything up a slot
        for(j = last_cmd; j != (to-2) ; j--)
          zone_table[zone].cmd[j+1] = zone_table[zone].cmd[j];
        sprintf(buf, "Command copied to %d.\r\n", to);
	to--;
      }
      else // tack it on the end
      {
        // bump the 'S' up
        zone_table[zone].cmd[last_cmd+1] = zone_table[zone].cmd[last_cmd];
	to = last_cmd;
        sprintf(buf, "Command copied.\r\n");
      }
      zone_table[zone].cmd[to].active = tmp.active;
      zone_table[zone].cmd[to].command = tmp.command;
      zone_table[zone].cmd[to].if_flag = tmp.if_flag;
      zone_table[zone].cmd[to].arg1 = tmp.arg1;
      zone_table[zone].cmd[to].arg2 = tmp.arg2;
      zone_table[zone].cmd[to].arg3 = tmp.arg3;
      zone_table[zone].cmd[to].comment = tmp.comment;
      send_to_char(buf, ch);
      break;
    }
    case 12:
    
      argument = one_argumentnolow(argument, text);
      unsigned int cont;
      if(!*text)
      {   
        send_to_char("$3Usage$R: zedit continent <continent number>\r\n", ch);
        for(cont = NO_CONTINENT; cont != continent_names.size(); cont++)
        {
      
          csendf(ch, "%d) %s\n\r", cont, continent_names.at(cont).c_str());
        }
        return eFAILURE;
      }
      
      if(!(cont = atoi(text)) || cont > continent_names.size()-1 )
      {
        csendf(ch, "You much choose between 1 and %d.\r\n", continent_names.size()-1);
        return eFAILURE;
      }
      csendf(ch, "Success. Continent changed to %s\n\r", continent_names.at(cont).c_str());
      zone_table[zone].continent = cont;
    break;

    default: 
      send_to_char("Error:  Couldn't find item in switch.\r\n", ch);
      break;
  }
  set_zone_modified_zone(ch->in_room);
  return eSUCCESS;
}

int do_sedit(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  char select[MAX_INPUT_LENGTH];
  char target[MAX_INPUT_LENGTH+1];
  char text[MAX_INPUT_LENGTH];
  char_data * vict;
  char_skill_data * skill;
  char_skill_data * lastskill;
  int16 field;
  int16 skillnum;
  int16 learned;
  int i;

  char * sedit_values[] = {
    "add", 
    "remove", 
    "set", 
    "list", 
    "\n"
  };

  if(!has_skill(ch, COMMAND_SEDIT)) {
      send_to_char("Huh?\r\n", ch);
      return eFAILURE;
  }

  half_chop(argument, target, text);
  half_chop(text, select, text);
  // at this point target is the character's name
  // select is the field, and text is any args

  if(!(*select) || !(*target)) {
    send_to_char("$3Usage$R: sedit <character> <field> [args]\r\n"
                 "Use a field with no args to get help on that field.\r\n"
                 "Fields are the following.\r\n"
                 , ch);
    strcpy(buf, "\r\n");
    display_string_list(sedit_values, ch);
    return eFAILURE;
  }

  if(!(vict = get_char_vis(ch, target))) {
    sprintf(buf, "Cannot find player '%s'.\r\n", target);
    send_to_char(buf, ch);
    return eFAILURE;
  }

  if(GET_LEVEL(vict) > GET_LEVEL(ch)) {
    send_to_char("You like to play dangerously don't you....\r\n", ch);
    return eFAILURE;
  }

  field = old_search_block(select, 0, strlen(select), sedit_values, 1);
  if(field < 0) {
    send_to_char("That field not recognized.\r\n", ch);
    return eFAILURE;
  }
  field--;

  switch(field) {
    case 0: /* add */
    {
      if(!(*text))
      {
        send_to_char("$3Usage$R: sedit <character> add <skillname>\r\n"
                     "This will give the skill to the character at learning 1.\r\n", ch);
        return eFAILURE;
      }

      if((skillnum = find_skill_num(text)) < 0) {
        sprintf(buf, "Cannot find skill '%s' in master skill list.\r\n", text);
        send_to_char(buf, ch);
        return eFAILURE;
      }
      if(has_skill(vict, skillnum)) {
        sprintf(buf, "'%s' already has '%s'.\r\n", GET_NAME(vict), text);
        send_to_char(buf, ch);
        return eFAILURE;
      }

      learn_skill(vict, skillnum, 1, 1);

      sprintf(buf, "'%s' has been given skill '%s' (%d) by %s.", GET_NAME(vict), text, skillnum, GET_NAME(ch));
      log(buf, GET_LEVEL(ch), LOG_GOD);
      sprintf(buf, "'%s' has been given skill '%s' (%d) by %s.\r\n", GET_NAME(vict), text, skillnum, GET_NAME(ch));
      send_to_char(buf, ch);
      break;
    }
    case 1: /* remove */
    {
      if(!(*text))
      {
        send_to_char("$3Usage$R: sedit <character> remove <skillname>\r\n"
                     "This will remove the skill from the character.\r\n", ch);
        return eFAILURE;
      }
      if((skillnum = find_skill_num(text)) < 0) {
        sprintf(buf, "Cannot find skill '%s' in master skill list.\r\n", text);
        send_to_char(buf, ch);
        return eFAILURE;
      }
      lastskill = NULL;
      for(skill = vict->skills; skill; lastskill = skill, skill = skill->next) {
        if(skill->skillnum == skillnum)
          break;
      }

      if(skill) {
        if(lastskill) 
          lastskill->next = skill->next;
        else vict->skills = skill->next;

        sprintf(buf, "Skill '%s'(%d) removed from %s by %s.", text, 
                     skill->learned, GET_NAME(vict), GET_NAME(ch));
        log(buf, GET_LEVEL(ch), LOG_GOD);
        sprintf(buf, "Skill '%s'(%d) removed from %s.\r\n", text, 
                     skill->learned, GET_NAME(vict));
        send_to_char(buf, ch);
      }
      else {
        sprintf(buf, "Cannot find skill '%s' on '%s'.\r\n", text, GET_NAME(vict));
        send_to_char(buf, ch);
        return eFAILURE;
      }
      break;
    }
    case 2: /* set */
    {
      if(!(*text))
      {
        send_to_char("$3Usage$R: sedit <character> set <skillname> <amount>\r\n"
                     "This will set the character's skill to amount.\r\n", ch);
        return eFAILURE;
      }
//      text = one_argument(text, target);
      half_chop(text, target, select);
//      chop_half(text, select, target);
      if((skillnum = find_skill_num(target)) < 0) {
        sprintf(buf, "Cannot find skill '%s' in master skill list.\r\n", text);
        send_to_char(buf, ch);
        return eFAILURE;
      }
      if(!(learned = has_skill(vict, skillnum))) {
        sprintf(buf, "'%s' does not have skill '%s'.\r\n", GET_NAME(vict), text);
        send_to_char(buf, ch);
        return eFAILURE;
      }
      if(!check_range_valid_and_convert(i, select, 1, 100)) {
        send_to_char("Invalid skill amount.  Must be 1 - 100.\r\n", ch);
        return eFAILURE;
      }

      learn_skill(vict, skillnum, i, i);

      sprintf(buf, "'%s's skill '%s' set to %d from %d by %s.", 
                   GET_NAME(vict), target, i, learned, GET_NAME(ch));
      log(buf, GET_LEVEL(ch), LOG_GOD);
      sprintf(buf, "'%s' skill '%s' set to %d from %d.\r\n", GET_NAME(vict), target, i, learned);
      send_to_char(buf, ch);
      break;
    }
    case 3: /* list */
    {
      i = 0;
      sprintf(buf, "$3Skills for$R:  %s\r\n"
                   "  Skill          Learned\r\n"
                   "$3---------------------------------$R\r\n", GET_NAME(vict));
      send_to_char(buf, ch);
      for(skill = vict->skills; skill; skill = skill->next)
      {
        char * skillname = get_skill_name(skill->skillnum);

        if(skillname) {
          sprintf(buf, "  %-15s%d\r\n", skillname, skill->learned);
        } else {
          sprintf(buf, "  unknown (%d)  %d\r\n", skill->skillnum, skill->learned);
        }

        send_to_char(buf, ch);
        i = 1;
      }
      if(!i)
        send_to_char("  (none)\r\n", ch);
      return eSUCCESS;
      break;
    }

    default: 
      send_to_char("Error:  Couldn't find item in switch (sedit).\r\n", ch);
      break;
  }

  // make sure the changes stick
  do_save(vict, "", 666);

  return eSUCCESS;
}

int oedit_exdesc(char_data * ch, int item_num, char * buf)
{
    char type[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];
    char select[MAX_INPUT_LENGTH];
    char value[MAX_INPUT_LENGTH];
    int  x;
    obj_data * obj = NULL;
    int  num;

    extra_descr_data * curr = NULL;
    extra_descr_data * curr2 = NULL;

    char * fields[] =
    {
      "new",
      "delete",
      "keywords",
      "desc",
      "\n"
    };

    half_chop(buf, type, buf2);
    half_chop(buf2, select, value);

    // type = add new delete
    // select = # of affect
    // value = value to change aff to

    obj = (obj_data *)obj_index[item_num].item;

    if(!*buf) {
      send_to_char("$3Syntax$R:  oedit [item_num] exdesc <field> [values]\r\n"
                   "The field must be one of the following:\n\r", ch);
      display_string_list(fields, ch);
      send_to_char("\n\r$3Current Descs$R:\n\r", ch);
      for(x = 1, curr = obj->ex_description; curr; x++, curr = curr->next)
         csendf(ch, "$3%d$R) %s\n\r%s\n\r", x, curr->keyword, curr->description);
      return eFAILURE;
    }

    for(x = 0 ;; x++)
    {
      if(fields[x][0] == '\n')
      {
        send_to_char("Invalid field.\n\r", ch);
        return eFAILURE;
      }
      if(is_abbrev(type, fields[x]))
        break;
    }

    switch(x) {
      // new
      case 0: { 
        if(!*select) {
          send_to_char("$3Syntax$R: oedit [item_num] exdesc new <keywords>\r\n"
                       "This adds a new description with the keywords chosen.\r\n", ch);
          return eFAILURE;
        }
        curr = (extra_descr_data *) calloc(1, sizeof(extra_descr_data));
        curr->keyword = str_hsh(select);
        curr->description = str_hsh("Empty desc.\n\r");
        curr->next = obj->ex_description;
        obj->ex_description = curr;
        send_to_char("New desc created.\r\n", ch);
        break;
      }

      // delete
      case 1: {
        if(!*select) {
          send_to_char("$3Syntax$R: oedit [item_num] exdesc delete <number>\r\n"
                       "This removes desc <number> from the list permanently.\r\n", ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(num, select, 1, 50)) {
          send_to_char("You must select a valid number.\r\n", ch);
          return eFAILURE;
        }
        x = 1;
        curr2 = NULL;
        for(curr = obj->ex_description; x < num && curr; curr = curr->next) {
          curr2 = curr;
          x++;
        }

        if(!curr) {
          send_to_char("There is no desc for that number.\n\r", ch);
          return eFAILURE;
        }

        if(!curr2) { // first one
          obj->ex_description = curr->next;
          dc_free(curr);
        }
        else {
          curr2->next = curr->next;
          dc_free(curr);
        }
        send_to_char("Deleted.\n\r", ch);
        break;
      }

      // keywords
      case 2: {
        if(!*select || !*value) {
          send_to_char("$3Syntax$R: oedit [item_num] exdesc keywords <number> <new keywords>\r\n"
                       "This removes desc <number> from the list permanently.\r\n", ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(num, select, 1, 50)) {
          send_to_char("You must select a valid number.\r\n", ch);
          return eFAILURE;
        }
        for(curr = obj->ex_description, x = 1; x < num && curr; curr = curr->next)
          x++;

        if(!curr) {
          send_to_char("There is no desc for that number.\n\r", ch);
          return eFAILURE;
        }
        curr->keyword = str_hsh(value);
        send_to_char("New keyword set.\n\r", ch);
        break;
      }

      // desc
      case 3: {
        if(!*select) {
          send_to_char("$3Syntax$R: oedit [item_num] exdesc desc <number>\r\n"
                       "This removes desc <number> from the list permanently.\r\n", ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(num, select, 1, 50)) {
          send_to_char("You must select a valid number.\r\n", ch);
          return eFAILURE;
        }
        for(curr = obj->ex_description, x = 1; x < num && curr; curr = curr->next)
          x++;

        if(!curr) {
          send_to_char("There is no desc for that number.\n\r", ch);
          return eFAILURE;
        }
        send_to_char("        Write your obj's description.  (/s saves /h for help)\r\n",ch);

//        send_to_char("Enter your obj's description below."
  //                   " Terminate with '~' on a new line.\n\r\n\r", ch);
//        curr->description = 0;
	
        ch->desc->connected = CON_EDITING;
        ch->desc->strnew = &(curr->description);
        ch->desc->max_str = MAX_MESSAGE_LENGTH;
        
        break;
      }

      default: send_to_char("Illegal value, tell someone.\r\n", ch);
        break;
    } // switch(x)

    set_zone_modified_obj(item_num);
    return eSUCCESS;
}

int oedit_affects(char_data * ch, int item_num, char * buf)
{
    char type[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];
    char select[MAX_INPUT_LENGTH];
    char value[MAX_INPUT_LENGTH];
    int  x;
    obj_data * obj = NULL;
    int  num;
    int  modifier;

    extern char *apply_types[];

    char * fields[] =
    {
      "new",
      "delete",
      "list",
      "1spell",
      "2amount",
      "3skill",
      "\n"
    };

    half_chop(buf, type, buf2);
    half_chop(buf2, select, value);

    // type = add new delete
    // select = # of affect
    // value = value to change aff to

    if(!*buf) {
      send_to_char("$3Syntax$R:  oedit [item_num] affects [affectnumber] [value]\r\n"
                   "The field must be one of the following:\n\r", ch);
      display_string_list(fields, ch);
      return eFAILURE;
    }

    for(x = 0 ;; x++)
    {
      if(fields[x][0] == '\n')
      {
        send_to_char("Invalid field.\n\r", ch);
        return eFAILURE;
      }
      if(is_abbrev(type, fields[x]))
        break;
    }

    obj = (obj_data *)obj_index[item_num].item;

    switch(x) {
      // new
      case 0: { 
        if(!*select) {
          send_to_char("$3Syntax$R: oedit [item_num] affects new yes\r\n"
                       "This adds a new blank affect to the end of the list.\r\n", ch);
          return eFAILURE;
        }
        add_obj_affect(obj, 0, 0);
        send_to_char("New affect created.\r\n", ch);
        break;
      }

      // delete
      case 1: {
        if(!*select) {
          send_to_char("$3Syntax$R: oedit [item_num] affects delete <number>\r\n"
                       "This removes affect <number> from the list permanently.\r\n", ch);
          return eFAILURE;
        }
        if(!obj->affected) {
          sprintf(buf, "Object %d has no affects to delete.\r\n", obj_index[item_num].virt);
          send_to_char(buf, ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(num, select, 1, obj->num_affects)) {
          sprintf(buf, "You must select between 1 and %d.\r\n", obj->num_affects);
          send_to_char(buf, ch);
          return eFAILURE;
        }
        remove_obj_affect_by_index(obj, num-1);
        send_to_char("Affect deleted.\r\n", ch);
        break;
      }

      // list
      case 2: {
        if(!obj->affected) {
          send_to_char("The object has no affects.\r\n", ch);
          return eSUCCESS;
        }
        send_to_char("$3Character Affects$R:\r\n"
                     "------------------\r\n", ch);
        for(x = 0; x < obj->num_affects; x++) {
//          sprinttype(obj->affected[x].location, apply_types, buf2);

                if (obj->affected[x].location < 1000)
                 sprinttype(obj->affected[x].location,apply_types,buf2);
                else if (get_skill_name(obj->affected[x].location/1000))
                  strcpy(buf2, get_skill_name(obj->affected[x].location/1000));

          sprintf(buf, "%2d$3)$R %s$3($R%d$3)$R by %d.\r\n", x+1, buf2, 
                     obj->affected[x].location, obj->affected[x].modifier);
          send_to_char(buf, ch);
        }
        return eSUCCESS; // return so we don't mark as changed
        break;
      }

      // 1spell
      case 3: {
        if(!*select) {
          send_to_char("$3Syntax$R: oedit [item_num] affects 1spell <number> <value>\r\n"
                       "This sets the modifying spell affect to <value> for affect <number>.\r\n", ch);
          for(x = 0;x <= APPLY_MAXIMUM_VALUE; x++) {
            sprintf(buf, "%3d$3)$R %s\r\n", x, apply_types[x]);
            send_to_char(buf, ch);
          }
          send_to_char("Make $B$5sure$R you don't use a spell that is restricted.  See builder guide.\r\n", ch);
          return eFAILURE;
        }
        if(!obj->affected) {
          sprintf(buf, "Object %d has no affects to modify.\r\n", obj_index[item_num].virt);
          send_to_char(buf, ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(num, select, 1, obj->num_affects)) {
          sprintf(buf, "You must select between 1 and %d.\r\n", obj->num_affects);
          send_to_char(buf, ch);
          return eFAILURE;
        }
        num -= 1; // since arrays start at 0
        if(!check_range_valid_and_convert(modifier, value, APPLY_NONE, APPLY_MAXIMUM_VALUE)) {
          sprintf(buf, "You must select between %d and %d.\r\n", APPLY_NONE, APPLY_MAXIMUM_VALUE);
          send_to_char(buf, ch);
          return eFAILURE;
        }
        obj->affected[num].location = modifier;
        sprintf(buf, "Affect %d changed to %s by %d.\r\n", num+1, 
             apply_types[obj->affected[num].location], obj->affected[num].modifier);
        send_to_char(buf, ch);
        break;
      }

      // 2amount
      case 4: {
        if(!*select) {
          send_to_char("$3Syntax$R: oedit [item_num] affects 1amount <number> <value>\r\n"
                       "This sets the spell affect's modifier to <value> for affect <number>.\r\n"
                       "Currently limited from -100 to 100.\r\n", ch);
          return eFAILURE;
        }
        if(!obj->affected) {
          sprintf(buf, "Object %d has no affects to modify.\r\n",
 obj_index[item_num].virt);
          send_to_char(buf, ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(num, select, 1, obj->num_affects)) {
          sprintf(buf, "You must select between 1 and %d.\r\n", 
obj->num_affects);
          send_to_char(buf, ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(modifier, value, -100, 100)) {
          send_to_char("You must select between -100 and 100.\r\n", ch);
          return eFAILURE;
        }
        num -= 1; // since arrays start at 0
        obj->affected[num].modifier = modifier;
	if (obj->affected[num].location >=1000)
	sprintf(buf,"Affect %d changed to %s by %d.\r\n",num+1,
		get_skill_name(obj->affected[num].location/1000),
		obj->affected[num].modifier);
       else sprintf(buf, "Affect %d changed to %s by %d.\r\n", num+1, 
             apply_types[obj->affected[num].location], obj->affected[num].modifier);
        send_to_char(buf, ch);
        break;
      }
      case 5:
        if(!*select) {
          send_to_char("$3Syntax$R: oedit [item_num] affects 3spell <number> <skill>\r\n"
                       "This sets the affect as affecting skills by 2amount\r\n", ch);
          return eFAILURE;
        }
        if(!obj->affected) {
          sprintf(buf, "Object %d has no affects to modify.\r\n",
 obj_index[item_num].virt);
          send_to_char(buf, ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(num, select, 1, 
obj->num_affects)) {
          sprintf(buf, "You must select between 1 and %d.\r\n",
obj->num_affects);
          send_to_char(buf, ch);
          return eFAILURE;
        }
	modifier = atoi(value);
	num -= 1;
        obj->affected[num].location = modifier*1000;
        sprintf(buf, "Affect %d changed to %s by %d.\r\n", num+1,
             get_skill_name(obj->affected[num].location/1000), obj->affected[num].modifier);
        send_to_char(buf, ch);
        break;
      default: send_to_char("Illegal value, tell pir.\r\n", ch);
        break;
    } // switch(x)

    set_zone_modified_obj(item_num);
    return eSUCCESS;
}

int do_oedit(struct char_data *ch, char *argument, int cmd)
{
    char buf[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];
    char buf3[MAX_INPUT_LENGTH];
    char buf4[MAX_INPUT_LENGTH];
    int  item_num = -1;
    int  intval = 0;
    int  x, i;

    extern char *item_types[];
    extern char *wear_bits[];
    extern char *size_bitfields[];
    extern char *extra_bits[];
    extern char *more_obj_bits[];
    extern struct obj_data  *object_list;

    char *fields[] = 
    {
      "keywords",
      "longdesc",
      "shortdesc",
      "actiondesc",
      "type",
      "wear",
      "size",
      "extra",
      "weight",
      "value",
      "moreflags",
      "level",
      "v1",
      "v2",
      "v3",
      "v4",
      "affects",
      "exdesc",
      "new",
      "delete",
      "stat",
      "timer",
      "description",
      "\n"
    };
   
    if(IS_NPC(ch))
      return eFAILURE;

    half_chop(argument, buf, buf2);
    half_chop(buf2, buf3, buf4);

    // at this point, buf  = item_num
    //                buf3 = field
    //                buf4 = args

    // or
     
    // buf = field
    // buf3 = args[0]
    // buf4 = args[1-+]
  
    if(!*buf) {
      send_to_char("$3Syntax$R:  oedit new [obj vnum]           -- Create new object\n\r"
		   "         oedit [obj vnum]               -- Stat object\n\r"
		   "         oedit [obj vnum] [field]       -- Help info for that field\n\r"
		   "         oedit [obj vnum] [field] [arg] -- Change that field\n\r"
		   "         oedit [field] [arg]            -- Change that field using last vnum\n\r\n\r"
                   "The field must be one of the following:\n\r", ch);
      display_string_list(fields, ch);
      return eFAILURE;
    }
    int itemvnum = -1;
    if(isdigit(*buf)) {
      item_num = atoi(buf);
      itemvnum = item_num;
      if( ((item_num = real_object(item_num)) < 0) ||
          (item_num == 0 && *buf != '0')
        )
      {
        send_to_char("Invalid item number.\r\n", ch);
        return eSUCCESS;
      }
    }
    else {
      item_num = ch->pcdata->last_obj_edit;
      // put the buffs where they should be
      if(*buf4)
        sprintf(buf2, "%s %s", buf3, buf4);
      else strcpy(buf2, buf3);
      strcpy(buf4, buf2);
      strcpy(buf3, buf);
    }
   if(item_num != ch->pcdata->last_obj_edit) {
      sprintf(buf2, "$3Current obj set to$R: %d\n\r", obj_index[item_num].virt);
      send_to_char(buf2, ch);
      ch->pcdata->last_obj_edit = item_num;
    }

  if (itemvnum == -1) itemvnum = obj_index[item_num].virt;
  if(!*buf3) // no field.  Stat the item.
    {
      obj_stat(ch, (obj_data *) obj_index[item_num].item);
      return eSUCCESS;
    }

// MOVED
    for(x = 0 ;; x++)
    {
      if(fields[x][0] == '\n')
      {
        send_to_char("Invalid field.\n\r", ch);
        return eFAILURE;
      }
      if(is_abbrev(buf3, fields[x]))
        break;
    }

    // a this point, item_num is the index
    if (x!=18) // Checked in there
    if(!can_modify_object(ch, itemvnum)) {
      send_to_char("You are unable to work creation outside of your range.\n\r", ch);
      return eFAILURE;
    }

    switch(x) {

     /* edit keywords */
      case 0 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] keywords <new_keywords>\n\r", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->name = str_hsh(buf4);
        sprintf(buf, "Item keywords set to '%s'.\r\n", buf4);
        send_to_char(buf, ch);
      } break;

     /* edit long desc */
      case 1 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] longdesc <new_desc>\n\r", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->description = str_hsh(buf4);
        sprintf(buf, "Item longdesc set to '%s'.\r\n", buf4);
        send_to_char(buf, ch);
      } break;

     // edit short desc
      case 2 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] shortdesc <new_desc>\n\r", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->short_description = str_hsh(buf4);
        sprintf(buf, "Item shortdesc set to '%s'.\r\n", buf4);
        send_to_char(buf, ch);
      } break;

     /* edit action desc */
      case 3 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] actiondesc <new_desc>\n\r", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->action_description = str_hsh(buf4);
        sprintf(buf, "Item actiondesc set to '%s'.\r\n", buf4);
        send_to_char(buf, ch);
      } break;

     /* edit type */
      case 4 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] type <>\n\r"
                       "$3Current$R: ", ch);
          sprintf(buf, "%s\n", item_types[((obj_data *)obj_index[item_num].item)->obj_flags.type_flag]);
          send_to_char(buf, ch);
          send_to_char("\r\n$3Valid types$R:\r\n", ch);
          for(i = 1; *item_types[i] != '\n'; i++) {
            sprintf(buf, "  %d) %s\n\r", i, item_types[i]);
            send_to_char(buf, ch);
          }
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(intval, buf4, 1, ITEM_TYPE_MAX)) {
          send_to_char("Value out of valid range.\r\n", ch);
          return eFAILURE;
        }
	if (intval==24)
	{
	  ((obj_data *)obj_index[item_num].item)->obj_flags.value[2] = -1;
	} else {
	  ((obj_data *)obj_index[item_num].item)->obj_flags.value[2] = 0;
	}
        ((obj_data *)obj_index[item_num].item)->obj_flags.type_flag = intval;
        sprintf(buf, "Item type set to %d.\r\n", intval);
        send_to_char(buf, ch);
      } break;

     /* edit wear */
      case 5 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] wear <location[s]>\n\r"
                       "$3Current$R: ", ch);
          sprintbit(((obj_data *)obj_index[item_num].item)->obj_flags.wear_flags,
                    wear_bits, buf);
          send_to_char(buf, ch);
          send_to_char("\r\n$3Valid types$R:\r\n", ch);
          for(i = 0; *wear_bits[i] != '\n'; i++) {
            sprintf(buf, "  %s\n\r", wear_bits[i]);
            send_to_char(buf, ch);
          }
          return eFAILURE;
        }
        parse_bitstrings_into_int(wear_bits, buf4, ch, 
                                     ((obj_data *)obj_index[item_num].item)->obj_flags.wear_flags);
      } break;

     /* edit size */
      case 6 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] size <size[s]>\n\r"
                       "$3Current$R: ", ch);
          sprintbit(((obj_data *)obj_index[item_num].item)->obj_flags.size,
                    size_bitfields, buf);
          send_to_char(buf, ch);
          send_to_char("\r\n$3Valid types$R:\r\n", ch);
          for(i = 0; *size_bitfields[i] != '\n'; i++) {
            sprintf(buf, "  %s\n\r", size_bitfields[i]);
            send_to_char(buf, ch);
          }
          return eFAILURE;
        }
        parse_bitstrings_into_int(size_bitfields, buf4, ch, 
                                     ((obj_data *)obj_index[item_num].item)->obj_flags.size);
      } break;

     /* edit extra */
      case 7 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] extra <bit[s]>\n\r"
                       "$3Current$R: ", ch);
          sprintbit(((obj_data *)obj_index[item_num].item)->obj_flags.extra_flags,
                    extra_bits, buf);
          send_to_char(buf, ch);
          send_to_char("\r\n$3Valid types$R:\r\n", ch);
          for(i = 0; *extra_bits[i] != '\n'; i++) {
            sprintf(buf, "  %s\n\r", extra_bits[i]);
            send_to_char(buf, ch);
          }
          return eFAILURE;
        }
        parse_bitstrings_into_int(extra_bits, buf4, ch, 
                                     ((obj_data *)obj_index[item_num].item)->obj_flags.extra_flags);
      } break;

     /* edit weight */
      case 8 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] weight <>\n\r", ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(intval, buf4, 0, 99999)) {
          send_to_char("Value out of valid range.\r\n", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->obj_flags.weight = intval;
        sprintf(buf, "Item weight set to %d.\r\n", intval);
        send_to_char(buf, ch);
      } break;

     /* edit value */
      case 9 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] value <>\n\r", ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(intval, buf4, 0, 5000000)) {
          send_to_char("Value out of valid range.\r\n", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->obj_flags.cost = intval;
        sprintf(buf, "Item value set to %d.\r\n", intval);
        send_to_char(buf, ch);
      } break;

     /* edit moreflags */
      case 10 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] moreflags <bit[s]>\n\r"
                       "$3Current$R: ", ch);
          sprintbit(((obj_data *)obj_index[item_num].item)->obj_flags.more_flags,
                    more_obj_bits, buf);
          send_to_char(buf, ch);
          send_to_char("\r\n$3Valid types$R:\r\n", ch);
          for(i = 0; *more_obj_bits[i] != '\n'; i++) {
            sprintf(buf, "  %s\n\r", more_obj_bits[i]);
            send_to_char(buf, ch);
          }
          return eFAILURE;
        }
        parse_bitstrings_into_int(more_obj_bits, buf4, ch, 
                                     ((obj_data *)obj_index[item_num].item)->obj_flags.more_flags);
      } break;

     /* edit level */
      case 11 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] level <>\n\r", ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(intval, buf4, 0, 110)) {
          send_to_char("Value out of valid range.\r\n", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->obj_flags.eq_level = intval;
        sprintf(buf, "Item minimum level set to %d.\r\n", intval);
        send_to_char(buf, ch);
      } break;

     /* edit 1value */
      case 12 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] 1value <num>\n\r", ch);
          return eFAILURE;
        }
        if(!check_valid_and_convert(intval, buf4)) {
          send_to_char("Please specifiy a valid number.\r\n", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->obj_flags.value[0] = intval;
        sprintf(buf, "Item value 1 set to %d.\r\n", intval);
        send_to_char(buf, ch);
      } break;

     /* edit 2value */
      case 13 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] 2value <num>\n\r", ch);
          return eFAILURE;
        }
        if(!check_valid_and_convert(intval, buf4)) {
          send_to_char("Please specifiy a valid number.\r\n", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->obj_flags.value[1] = intval;
        sprintf(buf, "Item value 2 set to %d.\r\n", intval);
        send_to_char(buf, ch);
      } break;

     /* edit 3value */
      case 14 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] 3value <num>\n\r", ch);
          return eFAILURE;
        }
        if(!check_valid_and_convert(intval, buf4)) {
          send_to_char("Please specifiy a valid number.\r\n", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->obj_flags.value[2] = intval;
        sprintf(buf, "Item value 3 set to %d.\r\n", intval);
        send_to_char(buf, ch);
      } break;

     /* edit 4value */
      case 15 : {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] 4value <num>\n\r", ch);
          return eFAILURE;
        }
        if(!check_valid_and_convert(intval, buf4)) {
          send_to_char("Please specifiy a valid number.\r\n", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->obj_flags.value[3] = intval;
        sprintf(buf, "Item value 4 set to %d.\r\n", intval);
        send_to_char(buf, ch);
      } break;

      /* affects */
      case 16: { 
        return oedit_affects(ch, item_num, buf4);
        break;
      }

      // exdesc
      case 17: {
        return oedit_exdesc(ch, item_num, buf4);
        break;
      }

      // new
      case 18: {
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit new [item number]\n\r", ch);
          return eFAILURE;
        }
        if(!check_range_valid_and_convert(intval, buf4, 0, 35000)) {
          send_to_char("Please specifiy a valid number.\r\n", ch);
          return eFAILURE;
        }
/*        if (real_object(intval) <= 0)
        {
	  send_to_char("Object already exists.\r\n",ch);
	  return eFAILURE;
	}
     */
	/*if (!has_skill(ch, COMMAND_RANGE))
	{
	  send_to_char("You cannot create items.\r\n",ch);
	  return eFAILURE;
	}*/
        if (!can_modify_object(ch, intval))
        {
	  send_to_char("You cannot create items in that range.\r\n",ch);
	  return eFAILURE;
        }
/*
        if(!can_modify_object(ch, intval)) {
          send_to_char("You are unable to work creation outside of your range.\n\r", ch);
          return eFAILURE;   
        }
*/
        x = create_blank_item(intval);
        if(x < 0) {
          csendf(ch, "Could not create item '%d'.  Max index hit or obj already exists.\r\n", intval);
          return eFAILURE;
        }
        csendf(ch, "Item '%d' created successfully.\r\n", intval);
        break;
      }

      // delete
      case 19: {
        if(!*buf4 || strncmp(buf4, "yesiwanttodeletethisitem", 24)) {
          send_to_char("$3Syntax$R: oedit [item_num] delete yesiwanttodeletethisitem\n\r", ch);
          send_to_char("\r\nDeleting an item is $3permanent$R and will cause ALL copies of\r\n", ch);
          send_to_char("that items in the world to disappear.  Logged out players will lose the\r\n", ch);
          send_to_char("item upon logging in as long as no other items is created with that number.\r\n", ch);
          send_to_char("(Creating a new items with that number will cause the others to remain on\r\n", ch);
          send_to_char("the player.)\r\n", ch);
          return eFAILURE;
        }

        
	struct vault_data *vault, *tvault;
	struct vault_items_data *items, *titems;
	struct obj_data *obj;
	int num = 0, real_num = 0;

	for (vault = vault_table; vault; vault = tvault, num++) 
	{
	    tvault = vault->next;

	    if (vault && vault->items) 
	    {
		for (items = vault->items; items; items = titems) 
		{
		    titems = items->next;
		    
		     
  		    real_num = real_object(items->item_vnum);
      		    obj = items->obj?items->obj:((struct obj_data*)obj_index[real_num].item);
		    if(obj == NULL)
			continue;

		    if (obj->item_number == item_num) 
		    {

			void item_remove(obj_data *obj, struct vault_data *vault);
			item_remove(obj, vault);
			//items->obj = 0;
			logf(0, LOG_MISC, "Removing deleted item %d from %s's vault.", itemvnum, vault->owner);
		    }
		}
	    }
	}



        obj_data * next_k;
        // remove the item from players in world
        for (obj_data * k = object_list; k; k = next_k) 
        {
           next_k = k->next;
           if(k->item_number == item_num)
              extract_obj(k);
        }



        // remove the item from index
        delete_item_from_index(item_num);
        send_to_char("Item deleted.\r\n", ch);
        break;
      }

      // stat
      case 20: {
        obj_stat(ch, (obj_data *) obj_index[item_num].item);
        break;
      }
	case 21:
	{
        if(!*buf4) {
          send_to_char("$3Syntax$R: oedit [item_num] 3value <num>\n\r", ch);
          return eFAILURE;
        }
        if(!check_valid_and_convert(intval, buf4)) {
          send_to_char("Please specifiy a valid number.\r\n", ch);
          return eFAILURE;
        }
        ((obj_data *)obj_index[item_num].item)->obj_flags.timer = intval;
        sprintf(buf, "Item timer to %d.\r\n", intval);
        send_to_char(buf, ch);	
	}break;
	case 22:
	extra_descr_data *curr;
	for (curr = ((obj_data*)obj_index[item_num].item)->ex_description;curr;curr = curr->next)
	if (!str_cmp(curr->keyword, ((obj_data*)obj_index[item_num].item)->name))
		break;
	if (!curr)
	{ //None existing;
		curr = (extra_descr_data*)calloc(1, sizeof(extra_descr_data));
		curr->keyword = str_dup(((obj_data*)obj_index[item_num].item)->name);
		curr->description = str_dup("");
		curr->next = ((obj_data*)obj_index[item_num].item)->ex_description;
		((obj_data*)obj_index[item_num].item)->ex_description = curr;
	}
	send_to_char("Write your object's description. End with /s.\r\n",ch);
	ch->desc->connected = CON_EDITING;
	ch->desc->strnew = &(curr->description);
	ch->desc->max_str = MAX_MESSAGE_LENGTH;
	break;
      default: send_to_char("Illegal value, tell pir.\r\n", ch);
        break;
    }

    set_zone_modified_obj(item_num);
    return eSUCCESS;
}

void update_mobprog_bits(int mob_num)
{
    MPROG_DATA * prog = mob_index[mob_num].mobprogs;
    mob_index[mob_num].progtypes = 0;

    while(prog) {
      SET_BIT(mob_index[mob_num].progtypes, prog->type);
      prog = prog->next;
    }
}

int do_procedit(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  char buf4[MAX_INPUT_LENGTH];
  int mob_num = -1;
  int intval = 0;
  int x, i;
  MPROG_DATA *prog;
  MPROG_DATA *currprog;

  void mpstat(CHAR_DATA *ch, CHAR_DATA *victim);

  char *fields[] = { "add", "remove", "type", "arglist", "command", "list", "\n" };

  if (IS_NPC(ch))
    return eFAILURE;

  half_chop(argument, buf, buf2);
  half_chop(buf2, buf3, buf4);

  // at this point, buf  = mob_num
  //                buf3 = field
  //                buf4 = args

  // or

  // buf = field
  // buf3 = args[0]
  // buf4 = args[1-+]
  if (!*buf)
  {
    send_to_char("$3Syntax$R:  procedit [mob_num] [field] [arg]\r\n"
        "  Edit a field with no args for help on that field.\r\n\r\n"
        "  The field must be one of the following:\n\r", ch);
    display_string_list(fields, ch);
    sprintf(buf2, "\n\r$3Current mob set to$R: %d\n\r", mob_index[ch->pcdata->last_mob_edit].virt);
    send_to_char(buf2, ch);
    return eFAILURE;
  }
  int mobvnum = -1;
  if (isdigit(*buf))
  {
    mob_num = atoi(buf);
    mobvnum = mob_num;
    if (((mob_num = real_mobile(mob_num)) < 0) || (mob_num == 0 && *buf != '0'))
    {
      send_to_char("Invalid mob number.\r\n", ch);
      return eSUCCESS;
    }
  } else
  {
    mob_num = ch->pcdata->last_mob_edit;
    // put the buffs where they should be
    sprintf(buf2, "%s %s", buf3, buf4);
    strcpy(buf4, buf2);
    strcpy(buf3, buf);
  }

  // a this point, mob_num is the index
  if (mobvnum == -1)
    mobvnum = mob_index[mob_num].virt;

  if (!can_modify_mobile(ch, mobvnum))
  {
    send_to_char("You are unable to work creation outside of your range.\n\r", ch);
    return eFAILURE;
  }

  if (mob_num != ch->pcdata->last_mob_edit)
  {
    sprintf(buf2, "$3Current mob set to$R: %d\n\r", mob_index[mob_num].virt);
    send_to_char(buf2, ch);
    ch->pcdata->last_mob_edit = mob_num;
  }

  // no field
  if (!*buf3)
  {
    mpstat(ch, (char_data*) mob_index[mob_num].item);
    return eSUCCESS;
  }

  for (x = 0;; x++)
  {
    if (fields[x][0] == '\n')
    {
      send_to_char("Invalid field.\n\r", ch);
      return eFAILURE;
    }
    if (is_abbrev(buf3, fields[x]))
      break;
  }

  switch (x) {

  /* add */
  case 0: {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] add new\n\r"
          "This creates a new mob prog and tacks it on the end.\r\n", ch);
      return eFAILURE;
    }
#ifdef LEAK_CHECK
    prog = (MPROG_DATA *) calloc(1, sizeof(MPROG_DATA));
#else
    prog = (MPROG_DATA*) dc_alloc(1, sizeof(MPROG_DATA));
#endif
    prog->type = GREET_PROG;
    prog->arglist = strdup("80");
    prog->comlist = strdup("say This is my new mob prog!\n\r");
    prog->next = NULL;

    int prog_num = 1;
    if ((currprog = mob_index[mob_num].mobprogs))
    {
      while (currprog->next)
      {
        prog_num++;
        currprog = currprog->next;
      }
      currprog->next = prog;
      prog_num++;
    } else
      mob_index[mob_num].mobprogs = prog;

      update_mobprog_bits(mob_num);

      csendf(ch, "New mobprog created as #%d.\r\n", prog_num);
    }
    break;

    /* remove */
  case 1: {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] remove <prog>\n\r", ch);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 999))
    {
      send_to_char("Invalid prog number.\r\n", ch);
      return eFAILURE;
    }
    // find program number "intval"
    prog = NULL;
    for (i = 1, currprog = mob_index[mob_num].mobprogs; currprog && i != intval; i++, prog = currprog, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      send_to_char("Invalid prog number.\r\n", ch);
      return eFAILURE;
    }

    if (prog)
      prog->next = currprog->next;
    else
      mob_index[mob_num].mobprogs = currprog->next;

    currprog->type = 0;
    dc_free(currprog->arglist);
    dc_free(currprog->comlist);
    dc_free(currprog);

    update_mobprog_bits(mob_num);

    send_to_char("Program deleted.\r\n", ch);
  }
    break;

    /* type */
  case 2: {
    half_chop(buf4, buf2, buf3);
    if (!*buf2 || !*buf3)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] type <prog> <newtype>\n\r"
          "$3Valid types$R:\r\n"
          "  1 -       act_prog\r\n"
          "  2 -    speech_prog\r\n"
          "  3 -      rand_prog\r\n"
          "  4 -     fight_prog\r\n"
          "  5 -     death_prog\r\n"
          "  6 -  hitprcnt_prog\r\n"
          "  7 -     entry_prog\r\n"
          "  8 -     greet_prog\r\n"
          "  9 - all_greet_prog\r\n"
          " 10 -      give_prog\r\n"
          " 11 -     bribe_prog\r\n"
          " 12 -     catch_prog\r\n"
          " 13 -    attack_prog\r\n"
          " 14 -     arand_prog\r\n"
          " 15 -      load_prog\r\n"
          " 16 -      can_see_prog\r\n"
          " 17 -      damage_prog\r\n"

      , ch);

      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf2, 1, 999))
    {
      send_to_char("Invalid prog number.\r\n", ch);
      return eFAILURE;
    }
    // find program number "intval"
    for (i = 1, currprog = mob_index[mob_num].mobprogs; currprog && i != intval; i++, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      send_to_char("Invalid prog number.\r\n", ch);
      return eFAILURE;
    }

    if (!check_range_valid_and_convert(intval, buf3, 1, 17))
    {
      send_to_char("Invalid prog number.\r\n", ch);
      return eFAILURE;
    }

    switch (intval) {
    case 1:
      currprog->type = ACT_PROG;
      break;
    case 2:
      currprog->type = SPEECH_PROG;
      break;
    case 3:
      currprog->type = RAND_PROG;
      break;
    case 4:
      currprog->type = FIGHT_PROG;
      break;
    case 5:
      currprog->type = DEATH_PROG;
      break;
    case 6:
      currprog->type = HITPRCNT_PROG;
      break;
    case 7:
      currprog->type = ENTRY_PROG;
      break;
    case 8:
      currprog->type = GREET_PROG;
      break;
    case 9:
      currprog->type = ALL_GREET_PROG;
      break;
    case 10:
      currprog->type = GIVE_PROG;
      break;
    case 11:
      currprog->type = BRIBE_PROG;
      break;
    case 12:
      currprog->type = CATCH_PROG;
      break;
    case 13:
      currprog->type = ATTACK_PROG;
      break;
    case 14:
      currprog->type = ARAND_PROG;
      break;
    case 15:
      currprog->type = LOAD_PROG;
      break;
    case 16:
      currprog->type = CAN_SEE_PROG;
      break;
    case 17:
      currprog->type = DAMAGE_PROG;
      break;
    }

    update_mobprog_bits(mob_num);

    send_to_char("Mob program type changed.\r\n", ch);
  }
    break;

    /* arglist */
  case 3: {
    half_chop(buf4, buf2, buf3);
    if (!*buf2 || !*buf3)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] arglist <prog> <new arglist>\n\r", ch);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf2, 1, 999))
    {
      send_to_char("Invalid prog number.\r\n", ch);
      return eFAILURE;
    }
    // find program number "intval"
    for (i = 1, currprog = mob_index[mob_num].mobprogs; currprog && i != intval; i++, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      send_to_char("Invalid prog number.\r\n", ch);
      return eFAILURE;
    }

    dc_free(currprog->arglist);
    currprog->arglist = strdup(buf3);

    send_to_char("Mob program arglist changed.\r\n", ch);
  }
    break;

    /* command */
  case 4: {
    if (!*buf4)
    {
      send_to_char("$3Syntax$R: procedit [mob_num] command <prog>\n\r"
          "This will put you into the editor which will replace the current\r\n"
          "command for program number <prog>.\r\n", ch);
      return eFAILURE;
    }
    if (!check_range_valid_and_convert(intval, buf4, 1, 999))
    {
      send_to_char("Invalid prog number.\r\n", ch);
      return eFAILURE;
    }
    // find program number "intval"
    for (i = 1, currprog = mob_index[mob_num].mobprogs; currprog && i != intval; i++, currprog = currprog->next)
      ;

    if (!currprog)
    { // intval was too high
      send_to_char("Invalid prog number.\r\n", ch);
      return eFAILURE;
    }

    ch->desc->backstr = NULL;
    ch->desc->strnew = &(currprog->comlist);
    ch->desc->max_str = MAX_MESSAGE_LENGTH;

    if (IS_SET(ch->pcdata->toggles, PLR_EDITOR_WEB))
    {
      ch->desc->web_connected = CON_EDIT_MPROG;
    } else
    {
      ch->desc->connected = CON_EDIT_MPROG;

      send_to_char("        Write your help entry and stay within the line.  (/s saves /h for help)\r\n"
          "   |--------------------------------------------------------------------------------|\r\n", ch);

      if (currprog->comlist)
      {
        ch->desc->backstr = str_dup(currprog->comlist);
        double_dollars(buf3, ch->desc->backstr);
        send_to_char(buf3, ch);
      }
    }
  }
    break;

    // list
  case 5:
    mpstat(ch, (char_data*) mob_index[mob_num].item);
    return eFAILURE;
  }
  set_zone_modified_mob(mob_num);
  return eSUCCESS;
}

int do_mscore(struct char_data *ch, char *argument, int cmd)
{
    char buf[MAX_INPUT_LENGTH];
    int  mob_num = -1;

    void boro_mob_stat(struct char_data *ch, struct char_data *k);

    if(IS_NPC(ch))
      return eFAILURE;

    one_argument(argument, buf);
  
    if(!*buf) {
      send_to_char("$3Syntax$R:  mscore <mob_num>\r\n", ch);
      return eFAILURE;
    }

    mob_num = atoi(buf);  // there is no mob 0, so this is okay.  Bad 0's get caught in real_mobile
    if( ((mob_num = real_mobile(mob_num)) < 0))
    {
      send_to_char("Invalid mob number.\r\n", ch);
      return eSUCCESS;
    }

    boro_mob_stat(ch, (char_data *) mob_index[mob_num].item);
    return eSUCCESS;
}

int do_medit(struct char_data *ch, char *argument, int cmd) {
	char buf[MAX_INPUT_LENGTH];
	char buf2[MAX_INPUT_LENGTH];
	char buf3[MAX_INPUT_LENGTH];
	char buf4[MAX_INPUT_LENGTH];
	int mob_num = -1;
	int intval = 0;
	int x, i;

	extern char *pc_clss_types[];
	extern char *action_bits[];
	extern char *affected_bits[];
	extern char *isr_bits[];
	extern char *position_types[];
	extern int mob_race_mod[][5];
	extern char *mob_types[];

	char *fields[] = { "keywords", "shortdesc", "longdesc", "description",
			"sex", "class", "race", "level", "alignment", "loadposition",
			"defaultposition", "actflags", "affectflags", "numdamdice",
			"sizedamdice", "damroll", "hitroll", "hphitpoints", "gold",
			"experiencepoints", "immune", "suscept", "resist", "armorclass",
			"stat", "strength", "dexterity", "intelligence", "wisdom",
			"constitution", "new", "delete", "type", "v1", "v2", "v3", "v4",
			"\n" };

	if (IS_NPC(ch))
		return eFAILURE;

	half_chop(argument, buf, buf2);
	half_chop(buf2, buf3, buf4);

	// at this point, buf  = mob_num
	//                buf3 = field
	//                buf4 = args

	// or

	// buf = field
	// buf3 = args[0]
	// buf4 = args[1-+]

	if (!*buf) {
		send_to_char("$3Syntax$R:  medit [mob_num] [field] [arg]\r\n"
				"  Edit a mob_num with no field or arg to view the item.\r\n"
				"  Edit a field with no args for help on that field.\r\n\r\n"
				"The field must be one of the following:\n\r", ch);
		display_string_list(fields, ch);
		return eFAILURE;
	}

	int mobvnum = -1;
	if (isdigit(*buf)) {
		mob_num = atoi(buf); // there is no mob 0, so this is okay.  Bad 0's get caught in real_mobile
		mobvnum = mob_num;
		if (((mob_num = real_mobile(mob_num)) < 0)) {
			send_to_char("Invalid mob number.\r\n", ch);
			return eSUCCESS;
		}
	} else {
		mob_num = ch->pcdata->last_mob_edit;
		mobvnum = mob_index[mob_num].virt;
		// put the buffs where they should be
		if (*buf4)
			sprintf(buf2, "%s %s", buf3, buf4);
		else
			strcpy(buf2, buf3);

		strcpy(buf4, buf2);
		strcpy(buf3, buf);
	}
	if (mob_num != ch->pcdata->last_mob_edit) {
		sprintf(buf2, "$3Current mob set to$R: %d\n\r",
				mob_index[mob_num].virt);
		send_to_char(buf2, ch);
		ch->pcdata->last_mob_edit = mob_num;
	}

	if (!*buf3) // no field.  Stat the item.
	{
		mob_stat(ch, (char_data *) mob_index[mob_num].item);
		return eSUCCESS;
	}

	if (mobvnum == -1)
		mobvnum = mob_index[mob_num].virt;
	// MOVED
	for (x = 0;; x++) {
		if (fields[x][0] == '\n') {
			send_to_char("Invalid field.\n\r", ch);
			return eFAILURE;
		}
		if (is_abbrev(buf3, fields[x]))
			break;
	}

	// a this point, mob_num is the index

	if (x != 30) // Checked in there.
		if (!can_modify_mobile(ch, mobvnum)) {
			send_to_char(
					"You are unable to work creation outside of your range.\n\r",
					ch);
			return eFAILURE;
		}

	switch (x) {

	/* edit keywords */
	case 0: {
		if (!*buf4) {
			send_to_char(
					"$3Syntax$R: medit [mob_num] keywords <new_keywords>\n\r",
					ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->name = str_hsh(buf4);
		sprintf(buf, "Mob keywords set to '%s'.\r\n", buf4);
		send_to_char(buf, ch);
	}
		break;

		/* edit short desc */
	case 1: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] shortdesc <desc>\n\r",
					ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->short_desc = str_hsh(buf4);
		sprintf(buf, "Mob shortdesc set to '%s'.\r\n", buf4);
		send_to_char(buf, ch);
	}
		break;

		// edit long desc
	case 2: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] longdesc <desc>\n\r", ch);
			return eFAILURE;
		}
		strcat(buf4, "\r\n");
		((char_data *) mob_index[mob_num].item)->long_desc = str_hsh(buf4);
		sprintf(buf, "Mob longdesc set to '%s'.\r\n", buf4);
		send_to_char(buf, ch);
	}
		break;

		/* edit description */
	case 3: {
		if (!*buf4) {
			send_to_char(
					"$3Syntax$R: medit [mob_num] description <anything>\n\r"
							"This will put you into the editor which will replace the\r\n"
							"current description.\r\n", ch);
			return eFAILURE;
		}
		send_to_char("Enter the mob's description below."
				" Terminate with '/s' on a new line.\n\r\n\r", ch);
// TODO - this causes a memory leak if you edit the desc twice (first one is hsh'd)
//        ((char_data *)mob_index[mob_num].item)->description = NULL;
		ch->desc->connected = CON_EDITING;
		((char_data*) mob_index[mob_num].item)->description = str_dup("");
		ch->desc->strnew =
				&(((char_data *) mob_index[mob_num].item)->description);
		ch->desc->max_str = MAX_MESSAGE_LENGTH;
	}
		break;

		/* edit sex */
	case 4: {
		if (!*buf4) {
			send_to_char(
					"$3Syntax$R: medit [mob_num] sex <male|female|neutral>\n\r",
					ch);
			return eFAILURE;
		}
		if (is_abbrev(buf4, "male")) {
			((char_data *) mob_index[mob_num].item)->sex = SEX_MALE;
			send_to_char("Mob sex set to male.\r\n", ch);
		} else if (is_abbrev(buf4, "female")) {
			((char_data *) mob_index[mob_num].item)->sex = SEX_FEMALE;
			send_to_char("Mob sex set to female.\r\n", ch);
		} else if (is_abbrev(buf4, "neutral")) {
			((char_data *) mob_index[mob_num].item)->sex = SEX_NEUTRAL;
			send_to_char("Mob sex set to neutral.\r\n", ch);
		} else
			send_to_char(
					"Invalid sex.  Chose 'male', 'female', or 'neutral'.\r\n",
					ch);
	}
		break;

		/* edit class */
	case 5: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] class <class>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%s\n",
					pc_clss_types[((char_data *) mob_index[mob_num].item)->c_class]);
			send_to_char(buf, ch);
			send_to_char("\r\n$3Valid types$R:\r\n", ch);
			for (i = 0; *pc_clss_types[i] != '\n'; i++) {
				sprintf(buf, "  %d) %s\n\r", i, pc_clss_types[i]);
				send_to_char(buf, ch);
			}
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 0, CLASS_MAX)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->c_class = intval;
		sprintf(buf, "Mob class set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit race */
	case 6: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] race <racetype>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%s\n\r\n\r"
					"Available types:\r\n",
					race_info[((char_data *) mob_index[mob_num].item)->race].singular_name);
			send_to_char(buf, ch);
			for (i = 0; i <= MAX_RACE; i++)
				csendf(ch, "  %s\r\n", race_info[i].singular_name);
			send_to_char("\r\n", ch);
			return eFAILURE;
		}
		int race_set = 0;
		for (i = 0; i <= MAX_RACE; i++) {
			if (is_abbrev(buf4, race_info[i].singular_name)) {
				csendf(ch, "Mob race set to %s.\r\n",
						race_info[i].singular_name);
				((char_data *) mob_index[mob_num].item)->race = i;
				race_set = 1;

				((char_data *) mob_index[mob_num].item)->raw_str =
						((char_data *) mob_index[mob_num].item)->str =
								BASE_STAT
										+ mob_race_mod[GET_RACE(
												((char_data * )mob_index[mob_num].item))][0];
				((char_data *) mob_index[mob_num].item)->raw_dex =
						((char_data *) mob_index[mob_num].item)->dex =
								BASE_STAT
										+ mob_race_mod[GET_RACE(
												((char_data * )mob_index[mob_num].item))][1];
				((char_data *) mob_index[mob_num].item)->raw_con =
						((char_data *) mob_index[mob_num].item)->con =
								BASE_STAT
										+ mob_race_mod[GET_RACE(
												((char_data * )mob_index[mob_num].item))][2];
				((char_data *) mob_index[mob_num].item)->raw_intel =
						((char_data *) mob_index[mob_num].item)->intel =
								BASE_STAT
										+ mob_race_mod[GET_RACE(
												((char_data * )mob_index[mob_num].item))][3];
				((char_data *) mob_index[mob_num].item)->raw_wis =
						((char_data *) mob_index[mob_num].item)->wis =
								BASE_STAT
										+ mob_race_mod[GET_RACE(
												((char_data * )mob_index[mob_num].item))][4];

			}
		}
		if (!race_set) {
			csendf(ch, "Could not find race '%s'.\r\n", buf4);
			return eFAILURE;
		}
	}
		break;

		/* edit level */
	case 7: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] level <levelnum>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->level);
			send_to_char(buf, ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 0, 110)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->level = intval;
		sprintf(buf, "Mob level set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit alignment */
	case 8: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] alignment <alignnum>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->alignment);
			send_to_char(buf, ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, -1000, 1000)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->alignment = intval;
		sprintf(buf, "Mob alignment set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit load position */
	case 9: {
		if (!*buf4) {
			send_to_char(
					"$3Syntax$R: medit [mob_num] loadposition <position>\n\r"
							"$3Current$R: ", ch);
			sprintf(buf, "%s\n",
					position_types[((char_data *) mob_index[mob_num].item)->position]);
			send_to_char(buf, ch);
			send_to_char("$3Valid positions$R:\r\n"
					"  1 = Standing\r\n"
					"  2 = Sitting\r\n"
					"  3 = Resting\r\n"
					"  4 = Sleeping\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 1, 4)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		switch (intval) {
		case 1:
			intval = POSITION_STANDING;
			break;
		case 2:
			intval = POSITION_SITTING;
			break;
		case 3:
			intval = POSITION_RESTING;
			break;
		case 4:
			intval = POSITION_SLEEPING;
			break;
		}
		((char_data *) mob_index[mob_num].item)->position = intval;
		sprintf(buf, "Mob default position set to %s.\r\n",
				position_types[intval]);
		send_to_char(buf, ch);
	}
		break;

		/* edit default position */
	case 10: {
		if (!*buf4) {
			send_to_char(
					"$3Syntax$R: medit [mob_num] defaultposition <position>\n\r"
							"$3Current$R: ", ch);
			sprintf(buf, "%s\n",
					position_types[((char_data *) mob_index[mob_num].item)->mobdata->default_pos]);
			send_to_char(buf, ch);
			send_to_char("$3Valid positions$R:\r\n"
					"  1 = Standing\r\n"
					"  2 = Sitting\r\n"
					"  3 = Resting\r\n"
					"  4 = Sleeping\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 1, 4)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		switch (intval) {
		case 1:
			intval = POSITION_STANDING;
			break;
		case 2:
			intval = POSITION_SITTING;
			break;
		case 3:
			intval = POSITION_RESTING;
			break;
		case 4:
			intval = POSITION_SLEEPING;
			break;
		}
		((char_data *) mob_index[mob_num].item)->mobdata->default_pos = intval;
		sprintf(buf, "Mob default position set to %s.\r\n",
				position_types[intval]);
		send_to_char(buf, ch);
	}
		break;

		/* edit actflags */
	case 11: {
		if (!*buf4) {
			send_to_char(
					"$3Syntax$R: medit [mob_num] actflags <location[s]>\n\r"
							"$3Current$R: ", ch);
			sprintbit(
					((char_data *) mob_index[mob_num].item)->mobdata->actflags,
					action_bits, buf);
			send_to_char(buf, ch);
			send_to_char("\r\n$3Valid types$R:\r\n", ch);
			for (i = 0; *action_bits[i] != '\n'; i++) {
				sprintf(buf, "  %s\n\r", action_bits[i]);
				send_to_char(buf, ch);
			}
			return eFAILURE;
		}
		parse_bitstrings_into_int(action_bits, buf4, ch,
				((char_data *) mob_index[mob_num].item)->mobdata->actflags);
	}
		break;

		/* edit affectflags */
	case 12: {
		if (!*buf4) {
			send_to_char(
					"$3Syntax$R: medit [mob_num] affectflags <location[s]>\n\r"
							"$3Current$R: ", ch);
			sprintbit(((char_data *) mob_index[mob_num].item)->affected_by[0],
					affected_bits, buf);
			send_to_char(buf, ch);
			send_to_char("\r\n$3Valid types$R:\r\n", ch);
			for (i = 0; *affected_bits[i] != '\n'; i++) {
				sprintf(buf, "  %s\n\r", affected_bits[i]);
				send_to_char(buf, ch);
			}
			return eFAILURE;
		}
		parse_bitstrings_into_int(affected_bits, buf4, ch,
				&(((char_data *) mob_index[mob_num].item)->affected_by[0]));
//                             &((((char_data *)mob_index[mob_num].item)->affected_by[0])));
	}
		break;

		/* edit numdamdice */
	case 13: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] numdamdice <amount>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->mobdata->damnodice);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 1 to 400\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 1, 400)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->mobdata->damnodice = intval;
		sprintf(buf, "Mob number dice for damage set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit sizedamdice */
	case 14: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] sizedamdice <amount>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->mobdata->damsizedice);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 1 to 400\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 1, 400)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->mobdata->damsizedice = intval;
		sprintf(buf, "Mob size dice for damage set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit damroll */
	case 15: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] damroll <damrollnum>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->damroll);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: -50 to 400\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, -50, 400)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->damroll = intval;
		sprintf(buf, "Mob damroll set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit hitroll */
	case 16: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] hitroll <levelnum>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->hitroll);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: -50 to 100\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, -50, 300)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->hitroll = intval;
		sprintf(buf, "Mob hitroll set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit hphitpoints */
	case 17: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] hphitpoints <hp>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->raw_hit);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 1 to 64000\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 1, 64000)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->raw_hit = intval;
		((char_data *) mob_index[mob_num].item)->max_hit = intval;
		sprintf(buf, "Mob hitpoints set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit gold */
	case 18: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] gold <goldamount>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%lld\n",
					((char_data *) mob_index[mob_num].item)->gold);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 0 to 10000000\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 0, 10000000)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		if (intval > 250000 && GET_LEVEL(ch) <= DEITY) {
			send_to_char(
					"104-'s can only set a mob to 250k gold.  If you need more ask someone.\r\n",
					ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->gold = intval;
		sprintf(buf, "Mob gold set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit experiencepoints */
	case 19: {
		if (!*buf4) {
			send_to_char(
					"$3Syntax$R: medit [mob_num] experiencepoints <xpamount>\n\r"
							"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					(int) ((char_data *) mob_index[mob_num].item)->exp);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 0 to 20000000\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 0, 20000000)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->exp = intval;
		sprintf(buf, "Mob experience set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit immune */
	case 20: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] immune <location[s]>\n\r"
					"$3Current$R: ", ch);
			sprintbit(((char_data *) mob_index[mob_num].item)->immune, isr_bits,
					buf);
			send_to_char(buf, ch);
			send_to_char("\r\n$3Valid types$R:\r\n", ch);
			for (i = 0; *isr_bits[i] != '\n'; i++) {
				sprintf(buf, "  %s\n\r", isr_bits[i]);
				send_to_char(buf, ch);
			}
			return eFAILURE;
		}
		parse_bitstrings_into_int(isr_bits, buf4, ch,
				((char_data *) mob_index[mob_num].item)->immune);
	}
		break;

		/* edit suscept */
	case 21: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] suscept <location[s]>\n\r"
					"$3Current$R: ", ch);
			sprintbit(((char_data *) mob_index[mob_num].item)->suscept,
					isr_bits, buf);
			send_to_char(buf, ch);
			send_to_char("\r\n$3Valid types$R:\r\n", ch);
			for (i = 0; *isr_bits[i] != '\n'; i++) {
				sprintf(buf, "  %s\n\r", isr_bits[i]);
				send_to_char(buf, ch);
			}
			return eFAILURE;
		}
		parse_bitstrings_into_int(isr_bits, buf4, ch,
				((char_data *) mob_index[mob_num].item)->suscept);
	}
		break;

		/* edit resist */
	case 22: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] resist <location[s]>\n\r"
					"$3Current$R: ", ch);
			sprintbit(((char_data *) mob_index[mob_num].item)->resist, isr_bits,
					buf);
			send_to_char(buf, ch);
			send_to_char("\r\n$3Valid types$R:\r\n", ch);
			for (i = 0; *isr_bits[i] != '\n'; i++) {
				sprintf(buf, "  %s\n\r", isr_bits[i]);
				send_to_char(buf, ch);
			}
			return eFAILURE;
		}
		parse_bitstrings_into_int(isr_bits, buf4, ch,
				((char_data *) mob_index[mob_num].item)->resist);
	}
		break;

		// armorclass
	case 23: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] armorclass <ac>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->armor);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 100 to $B-$R2000\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, -2000, 100)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->armor = intval;
		sprintf(buf, "Mob armorclass(ac) set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		// stat
	case 24: {
		mob_stat(ch, (char_data *) mob_index[mob_num].item);
		break;
	}
		// strength
	case 25: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] strength <str>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->raw_str);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 1 to 28\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 1, 28)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->raw_str = intval;
		sprintf(buf, "Mob raw strength set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;
		// dexterity
	case 26: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] dexterity <dex>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->raw_dex);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 1 to 28\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 1, 28)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->raw_dex = intval;
		sprintf(buf, "Mob raw dexterity set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;
		// intelligence
	case 27: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] intelligence <int>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->raw_intel);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 1 to 28\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 1, 28)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->raw_intel = intval;
		sprintf(buf, "Mob raw intelligence set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;
		// wisdom
	case 28: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] wisdom <wis>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->raw_wis);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 1 to 28\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 1, 28)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->raw_wis = intval;
		sprintf(buf, "Mob raw wisdom set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;
		// constitution
	case 29: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_num] constitution <con>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%d\n",
					((char_data *) mob_index[mob_num].item)->raw_con);
			send_to_char(buf, ch);
			send_to_char("$3Valid Range$R: 1 to 28\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 1, 28)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->raw_con = intval;
		sprintf(buf, "Mob raw constituion set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;
		// New
	case 30: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit new [number]\r\n", ch);
			return eFAILURE;
		}
		if (!check_range_valid_and_convert(intval, buf4, 0, 35000)) {
			send_to_char("Please specifiy a valid number.\r\n", ch);
			return eFAILURE;
		}
		if (!can_modify_mobile(ch, intval)) {
			send_to_char("You cannot create mobiles in that range.\r\n", ch);
			return eFAILURE;
		}
		x = create_blank_mobile(intval);
		if (x < 0) {
			csendf(ch,
					"Could not create mobile '%d'.  Max index hit or mob already exists.\r\n",
					intval);
			return eFAILURE;
		}
		csendf(ch, "Mobile '%d' created successfully.\r\n", intval);
	}
		break;
	case 31: {
		if (!*buf4 || strncmp(buf4, "yesiwanttodeletethismob", 23)) {
			send_to_char(
					"$3Syntax$R: medit [mob_number] delete yesiwanttodeletethismob\n\r",
					ch);
			return eFAILURE;
		}
		auto &character_list = DC::instance().character_list;
		for (auto& v : character_list) {
			if (IS_NPC(v) && v->mobdata->nr == mob_num)
				extract_char(v, TRUE);
		}
		delete_mob_from_index(mob_num);
		send_to_char("Mobile deleted.\r\n", ch);
	}
		break;
	case 32: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_vnum] type <type id>\n\r"
					"$3Current$R: ", ch);
			sprintf(buf, "%s\n",
					mob_types[((char_data *) mob_index[mob_num].item)->mobdata->mob_flags.type]);
			send_to_char(buf, ch);
			send_to_char("\r\n$3Valid types$R:\r\n", ch);
			for (i = 0; *mob_types[i] != '\n'; i++) {
				sprintf(buf, "  %d) %s\n\r", i, mob_types[i]);
				send_to_char(buf, ch);
			}
			return eFAILURE;
		}

		if (!check_range_valid_and_convert(intval, buf4, MOB_TYPE_FIRST,
				MOB_TYPE_LAST)) {
			send_to_char("Value out of valid range.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->mobdata->mob_flags.type =
				(mob_type_t) intval;
		sprintf(buf, "Mob type set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;
		/* edit 1value */
	case 33: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_vnum] 1value <num>\n\r", ch);
			return eFAILURE;
		}
		if (!check_valid_and_convert(intval, buf4)) {
			send_to_char("Please specify a valid number.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->mobdata->mob_flags.value[0] = intval;
		sprintf(buf, "Mob value 1 set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit 2value */
	case 34: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_vnum] 2value <num>\n\r", ch);
			return eFAILURE;
		}
		if (!check_valid_and_convert(intval, buf4)) {
			send_to_char("Please specify a valid number.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->mobdata->mob_flags.value[1] = intval;
		sprintf(buf, "Mob value 2 set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit 3value */
	case 35: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_vnum] 3value <num>\n\r", ch);
			return eFAILURE;
		}
		if (!check_valid_and_convert(intval, buf4)) {
			send_to_char("Please specify a valid number.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->mobdata->mob_flags.value[2] = intval;
		sprintf(buf, "Mob value 3 set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

		/* edit 4value */
	case 36: {
		if (!*buf4) {
			send_to_char("$3Syntax$R: medit [mob_vnum] 4value <num>\n\r", ch);
			return eFAILURE;
		}
		if (!check_valid_and_convert(intval, buf4)) {
			send_to_char("Please specify a valid number.\r\n", ch);
			return eFAILURE;
		}
		((char_data *) mob_index[mob_num].item)->mobdata->mob_flags.value[3] = intval;
		sprintf(buf, "Mob value 4 set to %d.\r\n", intval);
		send_to_char(buf, ch);
	}
		break;

	}
	set_zone_modified_mob(mob_num);
	return eSUCCESS;
}

int do_redit(struct char_data *ch, char *argument, int cmd)
{
    char buf[200], buf2[200], buf3[200];
    extern char *dirs[];
    extern char *room_bits[];
    extern char *sector_types[];
    int x, a, b, c, d = 0;
    struct extra_descr_data *extra;
    struct extra_descr_data *ext;

    char *return_directions[] =
    {
      "south",
      "west",
      "north",
      "east",
      "down",
      "up",
      ""
    }; 

    int reverse_number[] =
    {
      2, 3, 0, 1, 5, 4
    };
    char *fields[] = 
    {
      "name",
      "description",
      "exit",
      "extra",
      "exdesc",
      "rflag",
      "sector",
      "denymob",
      ""
    };
   

    if(IS_NPC(ch))
      return eFAILURE;

    half_chop(argument, buf, buf2);
    if(!*buf) {
      send_to_char("The field must be one of the following:\n\r", ch);
      for(x = 0 ;; x++) {
        if(fields[x][0] == '\0')
          return eFAILURE;
        csendf(ch, "  %s\n\r", fields[x]);
      }
    }

  if(!can_modify_room(ch, ch->in_room)) {
    send_to_char("You are unable to work creation outside of your range.\n\r", ch);
    return eFAILURE;
  }

    for(x = 0 ;; x++)
    {
      if(fields[x][0] == '\0')
      {
        send_to_char("The field must be one of the following:\n\r", ch);
        for(x = 0 ;; x++)
        {
          if(fields[x][0] == '\0')
             return eFAILURE;
          csendf(ch, "%s\n\r", fields[x]);
        }
      }
      if(is_abbrev(buf, fields[x]))
        break;
    }

    switch(x) {

     /* redit name */
      case 0 : {
        if(!*buf2) {
          send_to_char("$3Syntax$R: redit name <Room Name>\n\r", ch);
          return eFAILURE;
        }
        dc_free(world[ch->in_room].name);
        world[ch->in_room].name = str_dup(buf2);
        send_to_char("Ok.\n\r", ch);
      } break;

     /* redit description */
      case 1 : {
        if(*buf2) {
          sprintf(buf, "%s\n\r", buf2);
          dc_free(world[ch->in_room].description);
          world[ch->in_room].description = str_dup(buf);
          send_to_char("Ok.\n\r", ch);
          return eFAILURE;
        }
//        send_to_char("Enter your room's description below."
  //                   " Terminate with '~' on a new line.\n\r\n\r", ch);
	        send_to_char("        Write your room's description.  (/s saves /h for help)\r\n",ch);

//        FREE(world[ch->in_room].description);
  //      world[ch->in_room].description = 0;
	
        ch->desc->connected = CON_EDITING;
        ch->desc->strnew = &(world[ch->in_room].description);
        ch->desc->max_str = MAX_MESSAGE_LENGTH;
      } break;

     // redit exit
      case 2 : {
        if(!*buf2) {
          send_to_char("$3Syntax$R: redit exit <direction> <destination> "
                       "[flags keynumber keywords]\n\r", ch);
          send_to_char("NOTE: [flags keynumber keywords] are "
                       "optional for door creation.\n\r", ch);
          send_to_char("$3Examples$R:\n\r", ch);
          send_to_char("redit exit north 1200   Puts an exit going north "
                       "to room 1200 (no door.)\n\r", ch);
          send_to_char("redit exit north 1200 1 -1 door wooden    Same "
                       "exit except it has a door with no key.\n\r", ch);
          send_to_char("redit exit north 1200 9 345 grate rusty    Same "
                       "exit except it has a hidden grate with key 345.\n\r\n\r", ch);
          send_to_char("Flags at your disposal:\n\r"
                       "IS_DOOR    1\n\r"
                       "CLOSED     2\n\r"
                       "LOCKED     4\n\r"
                       "HIDDEN     8\n\r"
                       "IMM_ONLY  16\n\r"
                       "PICKPROOF 32\n\r", ch);
          return eFAILURE;
        }

        half_chop(buf2, buf, buf3);
        for(x = 0; x <=6; x++) {
          if(x == 6) {
            send_to_char("No such direction.\n\r", ch);
            return eFAILURE;
          }
          if(is_abbrev(buf, dirs[x]))
            break;
        }
        if(!*buf3) return eFAILURE;

        half_chop(buf3, buf, buf2);
        d = atoi(buf); c = real_room(d); 

        if(*buf2) {
          half_chop(buf2, buf, buf3);
          a = atoi(buf);
          if(!*buf3) return eFAILURE;

          half_chop(buf3, buf, buf2);
          b = atoi(buf);
          if(b == 0)
          {
            send_to_char("No key 0's allowed.  Changing to -1.\r\n", ch);
            b = -1;
          }
        } else {
           a = 0;
           b = -1;
         }

        if(c == (-1) && can_modify_room(ch, d)) {
          if(create_one_room(ch, d)) {
            c = real_room(d);
            csendf(ch, "Creating room %d.\n\r", d); 
          }
        } 
        
        if (c == (-1)) {
          csendf(ch, "Error creating exit to room %d.\r\n", d);
          return eFAILURE;
        }

        if(world[ch->in_room].dir_option[x])
          send_to_char("Modifying exit.\n\r", ch);
        else {
          send_to_char("Creating new exit.\n\r", ch);
          CREATE(world[ch->in_room].dir_option[x], struct room_direction_data, 1);
          world[ch->in_room].dir_option[x]->general_description = 0;
          world[ch->in_room].dir_option[x]->keyword = 0;
        }

        world[ch->in_room].dir_option[x]->exit_info = a;
        world[ch->in_room].dir_option[x]->key = b;
        world[ch->in_room].dir_option[x]->to_room = c;
        if(*buf2) {
          if(world[ch->in_room].dir_option[x]->keyword)
            dc_free(world[ch->in_room].dir_option[x]->keyword);
          world[ch->in_room].dir_option[x]->keyword = str_dup(buf2);
        }

        send_to_char("Ok.\n\r", ch);

        if(!IS_MOB(ch) && !IS_SET(ch->pcdata->toggles, PLR_ONEWAY)) {
          send_to_char("Attempting to create a return exit from "
                       "that room...\n\r", ch);
          if(world[c].dir_option[reverse_number[x]]) {
            send_to_char("COULD NOT CREATE EXIT...One already exists.\n\r", ch);
          }
          else {
            sprintf(buf, "%d redit exit %s %d %d %d %s", d,
                  return_directions[x], world[ch->in_room].number,
                  a, b, (*buf2 ? buf2 : ""));
            SET_BIT(ch->pcdata->toggles, PLR_ONEWAY);
            do_at(ch, buf, 9);
            REMOVE_BIT(ch->pcdata->toggles, PLR_ONEWAY);
          }
        }
      } break;

     /* redit extra */
      case 3 : {
        if(!*buf2) {
          send_to_char("$3Syntax$R: redit extra <keywords>\n\r"
                       "Use it once to create the desc with a new keyword(s).\r\n"
                       "Use it a second time with one of those keys to edit the description for it.\r\n"
                       , ch);
          return eFAILURE;
        }

        half_chop(buf2, buf, buf3);

        for(extra = world[ch->in_room].ex_description ;; extra = extra->next) 
        {
          if(!extra) 
          { // make a new one
            send_to_char("Creating new extra description.\n\r", ch);
            CREATE(extra, struct extra_descr_data, 1);
            if(!(world[ch->in_room].ex_description))
              world[ch->in_room].ex_description = extra;
            else
              for(ext = world[ch->in_room].ex_description ;; ext = ext->next) {
                if(ext->next == NULL) {
                  ext->next = extra;
                  break;
                }
              }
            extra->next = NULL;
            break;
          }
          /* modifying old extra description */
          else if(isname(buf, extra->keyword)) {
            send_to_char("Modifying extra description.\n\r", ch);
            break;
          }
        }

        FREE(extra->keyword);
        extra->keyword = str_dup(buf2);
        send_to_char("        Write your extra description.  (/s saves /h for help)\r\n",ch);

//        send_to_char("Enter your extra description below. Terminate with 
//"
  //                   "'~' on a new line.\n\r\n\r", ch);
//        FREE(extra->description);
  //      extra->description = 0;
        ch->desc->strnew = &extra->description;
        ch->desc->max_str = MAX_MESSAGE_LENGTH;
        ch->desc->connected = CON_EDITING;
      } break;

     /* redit exdesc */
      case 4 : {
        if(!*buf2) {
          send_to_char("$3Syntax$R: redit exdesc <direction>\n\r", ch);
          return eFAILURE;
        }

        one_argument(buf2, buf);
        for(x = 0; x <=6; x++) {
          if(x == 6) {
            send_to_char("No such direction.\n\r", ch);
            return eFAILURE;
          }
          if(is_abbrev(buf, dirs[x]))
            break;
        }

        if(!(world[ch->in_room].dir_option[x])) {
          send_to_char("That exit does not exist...create it first.\n\r", ch);
          return eFAILURE;
        }

        send_to_char("Enter the exit's description below. Terminate with "
                     "'~' on a new line.\n\r\n\r", ch);
/*        if(world[ch->in_room].dir_option[x]->general_description) {
          dc_free(world[ch->in_room].dir_option[x]->general_description);
          world[ch->in_room].dir_option[x]->general_description = 0;
        }
   */     ch->desc->strnew = &world[ch->in_room].dir_option[x]->general_description;
        ch->desc->max_str = MAX_MESSAGE_LENGTH;
        ch->desc->connected = CON_EDITING;
      } break;

      // rflag
      case 5 : {
        a = FALSE;
        if(!*buf2) {
          send_to_char("$3Syntax$R: redit rflag <flags>\n\r", ch);
          send_to_char("$3Available room flags$R:\n\r", ch);
          for(x = 0 ;; x++) {
            if(!strcmp(room_bits[x], "\n"))
              break;
	    if (!strcmp(room_bits[x], "unused"))
		continue;
            if((x+1)%4 == 0) {
              csendf(ch, "%-18s\n\r", room_bits[x]);
            } else {
              csendf(ch, "%-18s", room_bits[x]);
            }
          }
          send_to_char("\r\n\r\n", ch);
          return eFAILURE;
        }
        parse_bitstrings_into_int(room_bits, buf2, ch, (world[ch->in_room].room_flags));
      } break;

      // sector
      case 6: {
        if(!*buf2) {
          send_to_char("$3Syntax$R: redit sector <sector>\r\n", ch);
          send_to_char("$3Available sector types$R:\n\r", ch);
          for(x = 0 ;; x++) {
            if(!strcmp(sector_types[x], "\n"))
              break;
            if((x+1)%4 == 0) {
              csendf(ch, "%-18s\n\r", sector_types[x]);
            } else {
              csendf(ch, "%-18s", sector_types[x]); 
            }
          }
          send_to_char("\r\n\r\n", ch);
          return eFAILURE;
        }
        for(x = 0 ;; x++) {
          if(!strcmp(sector_types[x], "\n")) {
            send_to_char("No such sector type.\n\r", ch);
            return eFAILURE;
          }
          else if(is_abbrev(buf2, sector_types[x])) {
            world[ch->in_room].sector_type = x;
            csendf(ch, "Sector type set to %s.\n\r", sector_types[x]);
            break;
          }
        }
      } break;
       case 7: // denymob
	if (!*buf2 || !is_number(buf2))
	{
	  send_to_char("Syntax: redit denymob <vnum>\r\nDoing this on an already denied mob will allow it once more.\r\n",ch);
	  return eFAILURE;
	}
	bool done = FALSE;
	int mob = atoi(buf2);
	struct deny_data *nd,*pd = NULL;
	for (nd = world[ch->in_room].denied;nd;nd = nd->next)
	{
	  if (nd->vnum == mob){
		if (pd) pd->next = nd->next;
		else world[ch->in_room].denied = nd->next;
		dc_free(nd);
		csendf(ch, "Mobile %d ALLOWED entrance.\r\n", mob);
		done = TRUE;
		break;
	  }
	  pd = nd;
	}
	if (done) break;
	#ifdef LEAK_CHECK
		nd = (struct deny_data *)calloc(1, sizeof(struct deny_data));
	#else
		nd = (struct deny_data *)dc_alloc(1, sizeof(struct deny_data));
	#endif
	nd->next = world[ch->in_room].denied;
	nd->vnum = atoi(buf2);
	world[ch->in_room].denied = nd;
	csendf(ch, "Mobile %d DENIED entrance.\r\n",mob);
	break;
    }
    set_zone_modified_world(ch->in_room);
    return eSUCCESS;
}

int do_rdelete(struct char_data *ch, char *arg, int cmd)
{
  int x;
  char buf[50], buf2[50];
  struct extra_descr_data *i, *extra;
  extern char *dirs[];

  half_chop(arg, buf, buf2);

  if(!*buf) {
    send_to_char("$3Syntax$R:\n\rrdelete exit   <direction>\n\rrdelete "
                 "exdesc <direction>\n\rrdelete extra  <keyword>\n\r", ch);
    return eFAILURE; 
  }

  if(!can_modify_room(ch, ch->in_room)) {
    send_to_char("You cannot destroy things here, it is not your domain.\n\r", ch);
    return eFAILURE;
  }

  if(is_abbrev(buf, "exit")) {
    for(x = 0; x <= 6; x++) {
      if(x == 6) {
        send_to_char("No such direction.\n\r", ch);
        return eFAILURE;
      }
      if(is_abbrev(buf2, dirs[x]))
        break;
    }

    if(!(world[ch->in_room].dir_option[x])) {
      send_to_char("There is nothing there to remove.\n\r", ch);
      return eFAILURE;
    }
    dc_free(world[ch->in_room].dir_option[x]);
    world[ch->in_room].dir_option[x] = 0; 
    csendf(ch, "You stretch forth your hands and remove "
            "the %s exit.\n\r", dirs[x]);
  }

  else if(is_abbrev(buf, "extra")) {
    for(i = world[ch->in_room].ex_description ;; i = i->next) {
      if(i == NULL) {
        send_to_char("There is nothing there to remove.\n\r", ch);
        return eFAILURE;
      }
      if(isname(buf2, i->keyword))
        break;
    }

    if(world[ch->in_room].ex_description == i) {
      world[ch->in_room].ex_description = i->next;
      dc_free(i);
      send_to_char("You remove the extra description.\n\r", ch);
    }
    else {
      for(extra = world[ch->in_room].ex_description ;; extra = extra->next)
        if(extra->next == i) {
          extra->next = i->next;
          dc_free(i);
          send_to_char("You remove the extra description.\n\r", ch);
          break;
        }
    }
  }
 
  else if(is_abbrev(buf, "exdesc")) {
        if(!*buf2) {
          send_to_char("Syntax:\n\rrdelete exdesc <direction>\n\r", ch);
          return eFAILURE;
        }
        one_argument(buf2, buf);
        for(x = 0; x <=6; x++) {
          if(x == 6) {
            send_to_char("No such direction.\n\r", ch);
            return eFAILURE;
          }
          if(is_abbrev(buf, dirs[x]))
            break;
        }

        if(!(world[ch->in_room].dir_option[x])) {
          send_to_char("That exit does not exist...create it first.\n\r", ch);
          return eFAILURE;
        }

        if(!(world[ch->in_room].dir_option[x]->general_description)) {
          send_to_char("There's no description there to delete.\n\r", ch);
          return eFAILURE;
        }

        else {
          dc_free(world[ch->in_room].dir_option[x]->general_description);
          world[ch->in_room].dir_option[x]->general_description = 0;
          send_to_char("Ok.\n\r", ch);
        }
  }

  else
    send_to_char("Syntax:\n\rrdelete exit   <direction>\n\rrdelete "
                 "exdesc <direction>\n\rrdelete extra  <keyword>\n\r", ch);

  set_zone_modified_world(ch->in_room);
  return eSUCCESS;
}

int do_oneway(struct char_data *ch, char *arg, int cmd)
{
  if(IS_MOB(ch))
    return eFAILURE;

  if(cmd == 1) {
    if(!IS_SET(ch->pcdata->toggles, PLR_ONEWAY))
      SET_BIT(ch->pcdata->toggles, PLR_ONEWAY);
    send_to_char("You generate one-way exits.\n\r", ch);
  }
  else {
    if(IS_SET(ch->pcdata->toggles, PLR_ONEWAY))
      REMOVE_BIT(ch->pcdata->toggles, PLR_ONEWAY);
    send_to_char("You generate two-way exits.\n\r", ch);
  }
  return eSUCCESS;
}

int do_zsave(struct char_data *ch, char *arg, int cmd)
{
  FILE *f = (FILE *)NULL;
  char buf[180];
  char buf2[180];

  extern struct zone_data *zone_table;

  if(!can_modify_room(ch, ch->in_room)) {
    send_to_char("You may only zsave inside of the room range you are assigned to.\n\r", ch);
    return eFAILURE;
  }

  int zone = world[ch->in_room].zone;

  if(!zone_table[zone].filename) {
    send_to_char("That zone file doesn't seem to exist...tell an imp.\r\n", ch);
    return eFAILURE;
  }

  if(!IS_SET(zone_table[zone].zone_flags, ZONE_MODIFIED)) {
    send_to_char("This zonefile has not been modified.\r\n", ch);
    return eFAILURE;
  }

  sprintf(buf, "zonefiles/%s", zone_table[zone].filename);
  sprintf(buf2, "cp %s %s.last", buf, buf);
  system(buf2);

  if((f = dc_fopen(buf, "w")) == NULL) {
    fprintf(stderr,"Couldn't open room save file %s for %s.\n\r",
            zone_table[zone].filename, GET_NAME(ch));
    return eFAILURE;
  }

  write_one_zone(f, zone);

  dc_fclose(f);
  send_to_char("Saved.\n\r", ch);
  set_zone_saved_zone(ch->in_room);
  return eSUCCESS;
}

int do_rsave(struct char_data *ch, char *arg, int cmd)
{
  FILE *f = (FILE *)NULL;
  world_file_list_item * curr;

  extern world_file_list_item * world_file_list;

  if(!can_modify_room(ch, ch->in_room)) {
    send_to_char("You may only rsave inside of the room range you are assigned to.\n\r", ch);
    return eFAILURE;
  }

  curr = world_file_list;
  while(curr)
   if (curr->firstnum <= ch->in_room && curr->lastnum >= ch->in_room)
	break;
   else curr = curr->next;


  if(!curr) {
    send_to_char("That range doesn't seem to exist...tell an imp.\r\n", ch);
    return eFAILURE;
  }

  if(!IS_SET(curr->flags, WORLD_FILE_MODIFIED)) {
    send_to_char("This range has not been modified.\r\n", ch);
    return eFAILURE;
  }

  string file = string("world/") + string(curr->filename);
  string syscmd = string("cp ") + file + string(" ") + file + string(".last");
  system(syscmd.c_str());

  if((f = dc_fopen(file.c_str(), "w")) == NULL) {
    fprintf(stderr,"Couldn't open room save file %s for %s.\n\r",
            curr->filename, GET_NAME(ch));
    return eFAILURE;
  }

  for(int x = curr->firstnum; x <= curr->lastnum; x++) 
     write_one_room(f, x);

  // end file
  fprintf(f, "$~\n");

  dc_fclose(f);
  send_to_char("Saved.\n\r", ch);
  set_zone_saved_world(ch->in_room);
  return eSUCCESS;
}

int do_msave(struct char_data *ch, char *arg, int cmd)
{
  FILE *f = (FILE *)NULL;
  world_file_list_item * curr;
  char buf[180];
  char buf2[180];

  extern world_file_list_item * mob_file_list;

  if (ch->pcdata->last_mob_edit <= 0)
  {
    send_to_char("You have not recently edited a mobile.\r\n",ch);
    return eFAILURE;
  }

  int v = mob_index[ch->pcdata->last_mob_edit].virt;
  if(!can_modify_mobile(ch, v)) {
    send_to_char("You may only msave inside of the room range you are assigned to.\n\r", ch);
    return eFAILURE;
  }

  int r = real_mobile(v);

  curr = mob_file_list;
  while(curr)
   if (curr->firstnum <= r && curr->lastnum >= r)
	break;
   else curr = curr->next;

  if(!curr) {
    send_to_char("That range doesn't seem to exist...tell an imp.\r\n", ch);
    return eFAILURE;
  }

  if(!IS_SET(curr->flags, WORLD_FILE_MODIFIED)) { // this is okay...world_file_saved is used in all
    send_to_char("This range has not been modified.\r\n", ch);
    return eFAILURE;
  }

  sprintf(buf, "mobs/%s", curr->filename);
  sprintf(buf2, "cp %s %s.last", buf, buf);
  system(buf2);

  if((f = dc_fopen(buf, "w")) == NULL) {
    fprintf(stderr,"Couldn't open mob save file %s for %s.\n\r",
            curr->filename, GET_NAME(ch));
    return eFAILURE;
  }

  for(int x = curr->firstnum; x <= curr->lastnum; x++) 
     write_mobile((char_data *)mob_index[x].item, f);

  // end file
  fprintf(f, "$~\n");

  dc_fclose(f);
  send_to_char("Saved.\n\r", ch);
  set_zone_saved_mob(curr->firstnum);
  return eSUCCESS;
}

int do_osave(struct char_data *ch, char *arg, int cmd)
{
  FILE *f = (FILE *)NULL;
  world_file_list_item * curr;
  char buf[180];
  char buf2[180];

  extern world_file_list_item * obj_file_list;

  if (ch->pcdata->last_obj_edit <= 0)
  {
    send_to_char("You have not recently edited an item.\r\n",ch);
    return eFAILURE;
  }
  int v = obj_index[ch->pcdata->last_obj_edit].virt;
  if(!can_modify_object(ch, v)) {
    send_to_char("You may only msave inside of the room range you are assigned to.\n\r", ch);
    return eFAILURE;
  }
  int r = real_object(v);
  curr = obj_file_list;
  while(curr)
   if (curr->firstnum <= r && curr->lastnum >= r)
        break;
   else curr = curr->next;

  if(!curr) {
    send_to_char("That range doesn't seem to exist...tell an imp.\r\n", ch);
    return eFAILURE;
  }

  if(!IS_SET(curr->flags, WORLD_FILE_MODIFIED)) {
    send_to_char("This range has not been modified.\r\n", ch);
    return eFAILURE;
  }

  sprintf(buf, "objects/%s", curr->filename);
  sprintf(buf2, "cp %s %s.last", buf, buf);
  system(buf2);

  if((f = dc_fopen(buf, "w")) == NULL) {
    fprintf(stderr,"Couldn't open obj save file %s for %s.\n\r",
            curr->filename, GET_NAME(ch));
    return eFAILURE;
  }

  for(int x = curr->firstnum; x <= curr->lastnum; x++) 
     write_object((obj_data *)obj_index[x].item, f);

  // end file
  fprintf(f, "$~\n");

  dc_fclose(f);
  send_to_char("Saved.\n\r", ch);
  set_zone_saved_obj(curr->firstnum);
  return eSUCCESS;
}

int do_instazone(struct char_data *ch, char *arg, int cmd) {
	FILE *fl;
	extern struct char_data *character_list;
	extern struct obj_data *object_list;
	char buf[200], bufl[200]/*,buf2[200],buf3[200]*/;
	int room = 1, x, door/*,direction*/;
	int pos;
	int value;
	int low, high;
	int /*number,*/count;
	struct char_data *mob,/**tmp_mob,*next_mob,*/*mob_list;
	struct obj_data *obj, *tmp_obj,/**next_obj,*/*obj_list;

	bool found_room = FALSE;

	send_to_char(
			"Whoever thought of this had a good idea, but never really finished it.  Beg someone to finish it some time.\r\n",
			ch);
	return eFAILURE;

// Remember if you change this that it uses string_to_file which now appends a ~\n to the end
// of the string.  This command does NOT take that into consideration currently.

	/*    if(!GET_RANGE(ch)) {
	 send_to_char("You don't have a zone assigned to you!\n\r", ch);
	 return eFAILURE;
	 }

	 half_chop(GET_RANGE(ch), buf, bufl);*/
	low = atoi(buf);
	high = atoi(bufl);

	for (x = low; x <= high; x++) {
		room = real_room(x);
		if (room == (-1))
			continue;
		found_room = TRUE;
		break;
	}

	if (!found_room) {
		send_to_char("Your area doesn't seem to be there.  Tell Godflesh!", ch);
		return eFAILURE;
	}

	sprintf(buf, "../lib/builder/%s.zon", GET_NAME(ch));

	if ((fl = dc_fopen(buf, "w")) == NULL) {
		send_to_char("Couldn't open up zone file. Tell Godflesh!", ch);
		dc_fclose(fl);
		return eFAILURE;
	}

	for (x = low; x <= high; x++) {
		room = real_room(x);
		if (room == (-1))
			continue;
		break;
	}

	fprintf(fl, "#%d\n", world[room].zone);
	sprintf(buf, "%s's Area.", ch->name);
	string_to_file(fl, buf);
	fprintf(fl, "~\n");
	fprintf(fl, "%d 30 2\n", high);

	/* Set allthe door states..  */

	for (x = low; x <= high; x++) {
		room = real_room(x);
		if (room == (-1))
			continue;

		for (door = 0; door <= 5; door++) {
			if (!(world[room].dir_option[door]))
				continue;

			if (IS_SET(world[room].dir_option[door]->exit_info, EX_ISDOOR)) {
				if ((IS_SET(world[room].dir_option[door]->exit_info, EX_CLOSED))
						&& (IS_SET(world[room].dir_option[door]->exit_info,
								EX_LOCKED)))
					value = 2;
				else if (IS_SET(world[room].dir_option[door]->exit_info,
						EX_CLOSED))
					value = 1;
				else
					value = 0;

				fprintf(fl, "D 0 %d %d %d\n", world[room].number,
						world[world[room].dir_option[door]->to_room].number,
						value);
			}
		}
	} /*  Ok.. all door state info written...  */

	/*  Load loose objects.  In other words.. objects not carried by mobs. */

	for (x = low; x <= high; x++) {
		room = real_room(x);
		if (room == (-1))
			continue;

		if (world[room].contents) {

			for (obj = world[room].contents; obj; obj = obj->next_content) {

				count = 0;

				for (obj_list = object_list; obj_list; obj_list =
						obj_list->next) {
					if (obj_list->item_number == obj->item_number)
						count++;
				}

				if (!obj->in_obj) {
					fprintf(fl, "O 0 %d %d %d",
							obj_index[obj->item_number].virt, count,
							world[room].number);
					sprintf(buf, "           %s\n", obj->short_description);
					string_to_file(fl, buf);

					if (obj->contains) {

						for (tmp_obj = obj->contains; tmp_obj;
								tmp_obj = tmp_obj->next_content) {
							count = 0;
							for (obj_list = object_list; obj_list; obj_list =
									obj_list->next) {
								if (obj_list->item_number
										== tmp_obj->item_number)
									count++;
							}

							fprintf(fl, "P 1 %d %d %d",
									obj_index[tmp_obj->item_number].virt, count,
									obj_index[obj->item_number].virt);
							sprintf(buf, "     %s placed inside %s\n",
									tmp_obj->short_description,
									obj->short_description);
							string_to_file(fl, buf);
						} /*  for loop */
					} /* end of the object's contents... */
				} /* end of if !obj->in_obj */
			} /* first for loop for loose objects... */
		} /* All loose objects taken care of..  (Not on mobs,but inthe rooms) */
	} /*  for loop going through the fucking rooms again for loose obj's */

	/* Now for the major bitch...
	 * All the mobs, and all possible bullshit our builders will try to
	 * put on them.. i.e held objects with objects within them... Just
	 * your average pain in the ass shit....
	 */

	for (x = low; x <= high; x++) {
		room = real_room(x);
		if (room == (-1))
			continue;

		if (world[room].people) {

			for (mob = world[room].people; mob; mob = mob->next_in_room) {
				if (!IS_NPC(mob))
					continue;

				count = 0;

				for (mob_list = character_list; mob_list;
						mob_list = mob_list->next) {
					if (IS_MOB(mob_list)
							&& mob_list->mobdata->nr == mob->mobdata->nr)
						count++;
				}

				fprintf(fl, "M 0 %d %d %d", mob_index[mob->mobdata->nr].virt,
						count, world[room].number);
				sprintf(buf, "           %s\n", mob->short_desc);
				string_to_file(fl, buf);

				for (pos = 0; pos < MAX_WEAR; pos++) {
					if (mob->equipment[pos]) {

						obj = mob->equipment[pos];

						count = 0;
						for (obj_list = object_list; obj_list; obj_list =
								obj_list->next) {
							if (obj_list->item_number == obj->item_number)
								count++;
						}

						if (!obj->in_obj) {
							fprintf(fl, "E 1 %d %d %d",
									obj_index[obj->item_number].virt, count,
									pos);
							sprintf(buf, "      Equip %s with %s\n",
									mob->short_desc, obj->short_description);
							string_to_file(fl, buf);

							if (obj->contains) {

								for (tmp_obj = obj->contains; tmp_obj; tmp_obj =
										tmp_obj->next_content) {
									count = 0;
									for (obj_list = object_list; obj_list;
											obj_list = obj_list->next) {
										if (obj_list->item_number
												== tmp_obj->item_number)
											count++;
									}

									fprintf(fl, "P 1 %d %d %d",
											obj_index[tmp_obj->item_number].virt,
											count,
											obj_index[obj->item_number].virt);
									sprintf(buf, "     %s placed inside %s\n",
											tmp_obj->short_description,
											obj->short_description);
									string_to_file(fl, buf);
								} /*  for loop */
							}
						} /* end of the object's contents... */
					} /* End of if ch->equipment[pos]  */
				} /* For loop going through a mob's eq.. */

				if (mob->carrying) {

					for (obj = mob->carrying; obj; obj = obj->next_content) {

						count = 0;
						for (obj_list = object_list; obj_list; obj_list =
								obj_list->next) {
							if (obj_list->item_number == obj->item_number)
								count++;
						}

						if (!obj->in_obj) {
							fprintf(fl, "G 1 %d %d",
									obj_index[obj->item_number].virt, count);
							sprintf(buf, "      Give %s %s\n", mob->short_desc,
									obj->short_description);
							string_to_file(fl, buf);

							if (obj->contains) {

								for (tmp_obj = obj->contains; tmp_obj; tmp_obj =
										tmp_obj->next_content) {
									count = 0;
									for (obj_list = object_list; obj_list;
											obj_list = obj_list->next) {
										if (obj_list->item_number
												== tmp_obj->item_number)
											count++;
									}

									fprintf(fl, "P 1 %d %d %d",
											obj_index[tmp_obj->item_number].virt,
											count,
											obj_index[obj->item_number].virt);
									sprintf(buf, "     %s placed inside %s\n",
											tmp_obj->short_description,
											obj->short_description);
									string_to_file(fl, buf);
								} /*  for loop */
							}
						} /* end of the object's contents... */
					} /* end of for loop going through a mob's inventory */
				} /* end of if a mob has shit in their inventory */
			} /* end of for loop for looking at the mobs in ths room. */
		} /* end of if some body is in the fucking room.  */
	} /* end of for loop going through the zone looking for mobs...  */

	fprintf(fl, "S\n");
	dc_fclose(fl);
	send_to_char("Zone File Created! Tell someone who can put it in!\n\r", ch);
	return eSUCCESS;
}

int do_rstat(struct char_data *ch, char *argument, int cmd)
{
    char arg1[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH*2];
    char buf2[MAX_STRING_LENGTH];
    struct room_data *rm=0;
    struct char_data *k=0;
    struct obj_data  *j=0;
    struct extra_descr_data *desc;
    int i, x, loc;

    /* for rooms */
    extern char *dirs[];
    extern char *room_bits[];
    extern char *exit_bits[];
    extern char *sector_types[];

    if (IS_NPC(ch))
        return eFAILURE;

    argument = one_argument(argument, arg1);

    /* no argument */
    if (!*arg1) {
        rm = &world[ch->in_room];
        }

    else { 
      x = atoi(arg1);
      if(x < 0 || (loc = real_room(x)) == (-1)) {
        send_to_char("No such room exists.\n\r", ch);
        return eFAILURE;
      }
      rm = &world[loc];
    }
         if(IS_SET(rm->room_flags, CLAN_ROOM) && GET_LEVEL(ch) < PATRON) {
            send_to_char("And you are rstating a clan room because?\r\n", ch);
            sprintf(buf, "%s just rstat'd clan room %d.", GET_NAME(ch), rm->number);
            log(buf, PATRON, LOG_GOD);
            return eFAILURE;
         }
            sprintf(buf,
            "Room name: %s, Of zone : %d. V-Number : %d, R-number : %d\n\r",
                rm->name, rm->zone, rm->number, ch->in_room);
            send_to_char(buf, ch);

            sprinttype(rm->sector_type, sector_types,buf2);
            sprintf(buf, "Sector type : %s ", buf2);
            send_to_char(buf, ch);
            
            strcpy(buf,"Special procedure : ");
            strcat(buf,(rm->funct) ? "Exists\n\r" : "No\n\r");
            send_to_char(buf, ch);

            send_to_char("Room flags: ", ch);
            sprintbit((long) rm->room_flags,room_bits,buf);
	    sprintf(buf,"%s [ %u ]\r\n", buf, rm->room_flags);
            send_to_char(buf,ch);

            send_to_char("Description:\n\r", ch);
            send_to_char(rm->description, ch);
            
            strcpy(buf, "Extra description keywords(s): ");
            if(rm->ex_description) {
                strcat(buf, "\n\r");
                for (desc = rm->ex_description; desc; desc = desc->next) {
                    strcat(buf, desc->keyword);
                    strcat(buf, "\n\r");
                }
                strcat(buf, "\n\r");
                send_to_char(buf, ch);
            } else {
                strcat(buf, "None\n\r");
                send_to_char(buf, ch);
            }
 	    struct deny_data *d;
	    int a = 0;
	    for (d = rm->denied;d;d=d->next)
	    {
	      if (a == 0) send_to_char("Mobiles Denied: ",ch);
	      if (real_mobile(d->vnum) == -1)
		csendf(ch, "UNKNOWN(%d)\r\n",d->vnum);
		else
  	      csendf(ch, "%s(%d)\r\n",((CHAR_DATA*)mob_index[real_mobile(d->vnum)].item)->short_desc,d->vnum);
	     a++;
	    }
	send_to_char("\r\n",ch);	
            strcpy(buf, "------- Chars present -------\n\r");
            for (k = rm->people; k; k = k->next_in_room)
            {
              if(CAN_SEE(ch,k)){
                strcat(buf, GET_NAME(k));
                strcat(buf,
                    (!IS_NPC(k) ? "(PC)\n\r" : (!IS_MOB(k) ? "(NPC)\n\r"
                    : "(MOB)\n\r")));
              }
            }
            strcat(buf, "\n\r");
            send_to_char(buf, ch);

            string buffer;
            buffer = "--------- Contents ---------\n\r";
            for (j = rm->contents; j; j = j->next_content)
	      {
		if(CAN_SEE_OBJ(ch,j))
		  {
		    buffer += j->name;
		    buffer += "\n\r";
		  }
	      }
            buffer += "\n\r";
	    send_to_char(const_cast<char *>(buffer.c_str()), ch);

            send_to_char("------- Exits defined -------\n\r", ch);
            for (i = 0; i <= 5; i++) {
                if (rm->dir_option[i]) {
                    sprintf(buf,"Direction %s . Keyword : %s\n\r",
                            dirs[i], rm->dir_option[i]->keyword);
                    send_to_char(buf, ch);
                    strcpy(buf, "Description:\n\r  ");
                    if(rm->dir_option[i]->general_description)
                    strcat(buf, rm->dir_option[i]->general_description);
                    else
                        strcat(buf,"UNDEFINED\n\r");
                    send_to_char(buf, ch);
                    sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
                   sprintf(buf,
                "Exit flag: %s \n\rKey no: %d\n\rTo room (V-Number): %d\n\r",
                            buf2, rm->dir_option[i]->key,
                            rm->dir_option[i]->to_room);
                    send_to_char(buf, ch);
                }
            }
    return eSUCCESS;
}
    
int do_possess(struct char_data *ch, char *argument, int cmd)
{
    char arg[MAX_STRING_LENGTH];
    struct char_data *victim;
    char buf [200];

    if (IS_NPC(ch))
        return eFAILURE;

    one_argument(argument, arg);
    
    if (!*arg)
    {
        send_to_char("Possess who?\n\r", ch);
    }
    else
    {
        if (!(victim = get_char_vis(ch, arg)))
             send_to_char("They aren't here.\n\r", ch);
        else
        {
            if (ch == victim) {
                send_to_char("He he he... We are jolly funny today, eh?\n\r", ch);
                return eFAILURE;
            }
            else if((GET_LEVEL(victim) > GET_LEVEL(ch)) &&
                   (GET_LEVEL(ch) < IMP)) {
                send_to_char("That mob is a bit too tough for you to handle.\n\r",ch);
                return eFAILURE;
              }
            else if (!ch->desc || ch->desc->snoop_by 
            || ch->desc->snooping) {
               if (ch->desc->snoop_by)  {
                 send_to_char ("Whoa! Almost got caught snooping!\n",ch->desc->snoop_by->character);
                 sprintf (buf, "Your victim is now trying to possess: %s\n", victim->name);
                 send_to_char (buf, ch->desc->snoop_by->character);
                 do_snoop (ch->desc->snoop_by->character, 
                   ch->desc->snoop_by->character->name, 0);
               } else {              
                send_to_char("Mixing snoop & possess is bad for your health.\n\r", ch);
                return eFAILURE;
               }
               
            }

            else if(victim->desc || (!IS_NPC(victim))) {
                send_to_char(
                   "You can't do that, the body is already in use!\n\r",ch);
                return eFAILURE;
            }
            else {
                send_to_char("Ok.\n\r", ch);
                sprintf(buf, "%s possessed %s", GET_NAME(ch), GET_NAME(victim));
                log(buf, GET_LEVEL(ch), LOG_GOD);
                ch->pcdata->possesing = 1;
                ch->desc->character = victim;
                ch->desc->original = ch;

                victim->desc = ch->desc;
                ch->desc = 0;
            }
        }
    }
    return eSUCCESS;
}

int do_return(struct char_data *ch, char *argument, int cmd)
{

//    if(IS_MOB(ch))
//        return eFAILURE;

    if(!ch->desc)
        return eFAILURE;

    if(!ch->desc->original)
    { 
        send_to_char("Huh!?!\n\r", ch);
        return eFAILURE;
    }
    else
    {
        send_to_char("You return to your original body.\n\r",ch);

        ch->desc->original->pcdata->possesing = 0;
        ch->desc->character = ch->desc->original;
        ch->desc->original = 0;

        ch->desc->character->desc = ch->desc; 
        ch->desc = 0;
	if (IS_NPC(ch) && mob_index[ch->mobdata->nr].virt > 90 &&
	  mob_index[ch->mobdata->nr].virt < 100 &&
	cmd != 12)
	{
	  act("$n evaporates.",ch, 0, 0, TO_ROOM, 0);
	  extract_char(ch, TRUE);
	  return eSUCCESS|eCH_DIED;
	}
    }
    return eSUCCESS;
}

int do_sockets(struct char_data *ch, char *argument, int cmd)
{
   char * pStr     = 0;
   int num_can_see = 0;

   char name[200];
   char buf[MAX_STRING_LENGTH];

   descriptor_data * d = 0, *ad = 0;

   extern char *connected_states[];

   if (IS_NPC(ch)) {
      send_to_char( "Monsters don't care who's logged in.\n\r", ch );
      return eFAILURE;
      }


   buf[0]  = '\0';
   name[0] = '\0';

   one_argument(argument, name);

   for(d = descriptor_list; d; d = d->next) {
      if(GET_LEVEL(ch) < OVERSEER)
      {
        if (d->character == NULL)
           continue;
        if (d->character->name == NULL)
           continue;
      }
      if(d->character)
      {
        if (!CAN_SEE(ch, d->character))
           continue;
	if (GET_LEVEL(ch) < GET_LEVEL(d->character)) continue;
        if ((d->connected != CON_PLAYING) &&
            (GET_LEVEL(ch) < GET_LEVEL(d->character)))
           continue;
      }

      if (*name &&
          !str_str(d->host, name) && (!d->character || !isname(name, GET_NAME(d->character))))
            continue; 
      bool duplicate = FALSE;
      for (ad = descriptor_list; ad; ad = ad->next)
	if (ad != d && !str_cmp(d->host, ad->host))
	if (!ad->character ||
	    GET_LEVEL(ad->character) <= GET_LEVEL(ch))	   
	{
	   duplicate = TRUE;
 	   break;
	}
      num_can_see++;
      
      sprintf(buf + strlen(buf), "%s%3d : %-15s | %-16s$R |", duplicate? 
"$B$4":"", d->descriptor, d->host, (d->character ? (d->character->name ? d->character->name : "NONE") : "NONE"));

      if ((pStr = constindex(d->connected, connected_states)))
         sprintf(buf + strlen(buf), "%-20s |", pStr);
      else
         sprintf(buf + strlen(buf), "***UNKNOWN*** | ");
       sprintf(buf+strlen(buf), "%d\r\n",d->idle_time/PASSES_PER_SEC);
      
  } // for

  if(num_can_see > max_who)
    max_who = num_can_see;

  sprintf(buf + strlen(buf), "\n\r\n\rThere are %d connections.\n\r", num_can_see);
  page_string(ch->desc, buf, 1);
  return eSUCCESS;
}



int do_setvote(struct char_data *ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  void send_info(char*);
  half_chop(arg, buf, buf2);

  if(!*buf)
  {
    send_to_char("Syntax: voteset <question|add|remove|clear|start|end> <string>", ch);
    return eFAILURE;
  }
   


  if(!strcmp(buf, "start"))
  {
    DCVote->StartVote(ch);
    return eSUCCESS;
  }

  if(!strcmp(buf, "clear"))
  {
    DCVote->Reset(ch);
    return eSUCCESS;
  }

  if(!strcmp(buf, "end"))
  {
    DCVote->EndVote(ch);
    return eSUCCESS;
  } 


  if(!*buf2)
  {
    send_to_char("Syntax: voteset <question|add|remove|clear|start|end> <string>", ch);
    return eFAILURE;
  }

  if(!strcmp(buf, "question"))
  {
    DCVote->SetQuestion(ch, buf2);
    return eSUCCESS;
  }
  if(!strcmp(buf, "add"))
  {
    DCVote->AddAnswer(ch, buf2);
    return eSUCCESS;
  }  
  if(!strcmp(buf, "remove"))
  {
    DCVote->RemoveAnswer(ch, (unsigned int)atoi(buf2));
    return eSUCCESS;
  }
 
  send_to_char("Syntax: voteset <question|add|remove|clear|start|end> <string>", ch);
  return eFAILURE;
}


#ifdef WIN32
int strncasecmp(char *s1, const char *s2, int len)
{
	char buf[256];
	strncpy(buf, s2, 255);
	buf[255] = 0;
	_strlwr(s1);
	_strlwr(buf);
	return(strncmp(s1, buf, len));
}
#endif
	
int do_punish(struct char_data *ch, char *arg, int cmd)
{
   char name[100], buf[150];
   char_data *vict;

   int i;

   if(IS_MOB(ch)) {
      send_to_char("Punish yourself!  Bad mob!\r\n", ch);
      return eFAILURE;
   }

   arg = one_argument(arg, name);

   if (!*name) {
      send_to_char("Punish who?\n\r", ch);
      send_to_char("\n\rusage: punish <char> [stupid silence freeze noemote "
                   "notell noname noarena notitle nopray]\n\r", ch);
      return eFAILURE;
      }

  if(!(vict = get_pc_vis(ch, name))) {
    snprintf(buf, sizeof(buf), "%s not found.\n\r", name);
    send_to_char(buf, ch);
    return eFAILURE;
  }

  one_argument(arg, name);

  if (!name[0]) {
     display_punishes(ch, vict);
     return eFAILURE;
  }

  i = strlen(name);

  if (i > 5) 
    i = 5;

  if (GET_LEVEL(vict) > GET_LEVEL(ch)) {
     act("$E might object to that.. better not.", ch, 0, vict, TO_CHAR, 0);
     return eFAILURE;
   }
  if (!strncasecmp(name, "stupid", i)) 
  {
    if (IS_SET(vict->pcdata->punish, PUNISH_STUPID)) {
        send_to_char("You feel a sudden onslaught of wisdom!\n\r", vict);
        send_to_char("STUPID removed.\n\r", ch);
        sprintf(buf, "%s removes %s's stupid", GET_NAME(ch), GET_NAME(vict));
        log(buf, GET_LEVEL(ch), LOG_GOD);
        REMOVE_BIT(vict->pcdata->punish, PUNISH_STUPID);
        REMOVE_BIT(vict->pcdata->punish, PUNISH_SILENCED);
        REMOVE_BIT(vict->pcdata->punish, PUNISH_NOEMOTE);
        REMOVE_BIT(vict->pcdata->punish, PUNISH_NONAME);
        REMOVE_BIT(vict->pcdata->punish, PUNISH_NOTITLE);
    } else {
        send_to_char("You suddenly feel dumb as a rock!\n\r", vict);
        send_to_char("You can't remember how to do basic things!\n\r", vict);
        sprintf(buf, "You have been lobotomized by %s!\n\r", GET_NAME(ch));
        send_to_char(buf, vict);
        send_to_char("STUPID set.\n\r", ch);
        sprintf(buf, "%s lobotimized %s", GET_NAME(ch), GET_NAME(vict));
        log(buf, GET_LEVEL(ch), LOG_GOD);
        SET_BIT(vict->pcdata->punish, PUNISH_STUPID);
        SET_BIT(vict->pcdata->punish, PUNISH_SILENCED);
        SET_BIT(vict->pcdata->punish, PUNISH_NOEMOTE);
        SET_BIT(vict->pcdata->punish, PUNISH_NONAME);
        SET_BIT(vict->pcdata->punish, PUNISH_NOTITLE);
    }
  }
  if (!strncasecmp(name, "silence", i)) {
    if (IS_SET(vict->pcdata->punish, PUNISH_SILENCED)) {
        send_to_char("The gods take pity on you and lift your silence.\n\r",
                     vict);
        send_to_char("SILENCE removed.\n\r", ch);
        sprintf(buf, "%s removes %s's silence", GET_NAME(ch), GET_NAME(vict));
        log(buf, GET_LEVEL(ch), LOG_GOD);
    } else {
       sprintf(buf, "You have been silenced by %s!\n\r", GET_NAME(ch));
       send_to_char(buf, vict);
        send_to_char("SILENCE set.\n\r", ch);
        sprintf(buf, "%s silenced %s", GET_NAME(ch), GET_NAME(vict));
        log(buf, GET_LEVEL(ch), LOG_GOD);
    }
    TOGGLE_BIT(vict->pcdata->punish, PUNISH_SILENCED);
  }
  if (!strncasecmp(name, "freeze", i)) {
    if (IS_SET(vict->pcdata->punish, PUNISH_FREEZE)) {
        send_to_char("You now can do things again.\n\r", vict);
        send_to_char("FREEZE removed.\n\r", ch);
        sprintf(buf, "%s unfrozen by %s", GET_NAME(vict), GET_NAME(ch));
        log(buf, GET_LEVEL(ch), LOG_GOD);
    } else {
       sprintf(buf, "%s takes away your ability to....\n\r", GET_NAME(ch));
       send_to_char(buf, vict);
       send_to_char("FREEZE set.\n\r", ch);
       sprintf(buf, "%s frozen by %s", GET_NAME(vict), GET_NAME(ch));
       log(buf, GET_LEVEL(ch), LOG_GOD);
    }
    TOGGLE_BIT(vict->pcdata->punish, PUNISH_FREEZE);
  }
  if(!strncasecmp(name, "noarena", i)) {
    if(IS_SET(vict->pcdata->punish, PUNISH_NOARENA)) {
        send_to_char("Some kind god has let you join arenas again.\n\r", vict);
        send_to_char("NOARENA removed.\n\r", ch);
    } else {
        sprintf(buf, "%s takes away your ability to join arenas!\n\r", GET_NAME(ch));
        send_to_char("NOARENA set.\n\r", ch);
        send_to_char(buf, vict);
    }
    TOGGLE_BIT(vict->pcdata->punish, PUNISH_NOARENA);
  }
  if (!strncasecmp(name, "noemote", i)) {
    if (IS_SET(vict->pcdata->punish, PUNISH_NOEMOTE)) {
        send_to_char("You can emote again.\n\r", vict);
        send_to_char("NOEMOTE removed.\n\r", ch);
    } else {
        sprintf(buf, "%s takes away your ability to emote!\n\r", GET_NAME(ch));
        send_to_char(buf, vict);
        send_to_char("NOEMOTE set.\n\r", ch);
    }
    TOGGLE_BIT(vict->pcdata->punish, PUNISH_NOEMOTE);
  }
  if (!strncasecmp(name, "notell", i)) {
    if (IS_SET(vict->pcdata->punish, PUNISH_NOTELL)) {
        send_to_char("You can use telepatic communication again.\n\r", vict);
        send_to_char("NOTELL removed.\n\r", ch);
    } else {
       sprintf(buf, "%s takes away your ability to use telepathic communication!\n\r", GET_NAME(ch));
       send_to_char(buf, vict);
       send_to_char("NOTELL set.\n\r", ch);
    }
    TOGGLE_BIT(vict->pcdata->punish, PUNISH_NOTELL);
  }
  if (!strncasecmp(name, "noname", i)) {
    if(IS_SET(vict->pcdata->punish, PUNISH_NONAME)) {
      send_to_char("The gods grant you control over your name.\n\r", vict); 
      send_to_char("NONAME removed.\n\r", ch);
    }
    else { 
      sprintf(buf, "%s removes your ability to set your name!\n\r", GET_NAME(ch));
      send_to_char("NONAME set.\n\r", ch);
      send_to_char(buf, vict);
    }    
    TOGGLE_BIT(vict->pcdata->punish, PUNISH_NONAME);
  }

  if (!strncasecmp(name, "notitle", i)) {
    if(IS_SET(vict->pcdata->punish, PUNISH_NOTITLE)) {
      send_to_char("The gods grant you control over your title.\n\r", vict);  
      send_to_char("NOTITLE removed.\n\r", ch);
    }
    else { 
      sprintf(buf, "%s removes your ability to set your title!\n\r", GET_NAME(ch));
      send_to_char("NOTITLE set.\n\r", ch);
      send_to_char(buf, vict);
    }    
    TOGGLE_BIT(vict->pcdata->punish, PUNISH_NOTITLE);
  }

  if (!strncasecmp(name, "unlucky", i)) {
    if(IS_SET(vict->pcdata->punish, PUNISH_UNLUCKY)) {
      if(!ch->pcdata->stealth)
        send_to_char("The gods remove your poor luck.\n\r", vict);  
      send_to_char("UNLUCKY removed.\n\r", ch);
      sprintf(buf, "%s removes %s's unlucky.", GET_NAME(ch), GET_NAME(vict));
      log(buf, GET_LEVEL(ch), LOG_GOD);
    }
    else { 
      if(!ch->pcdata->stealth) {
        sprintf(buf, "%s curses you with god-given bad luck!\n\r", GET_NAME(ch));
        send_to_char("UNLUCKY set.\n\r", ch);
      }
      send_to_char(buf, vict); 
      sprintf(buf, "%s makes %s unlucky.", GET_NAME(ch), GET_NAME(vict));
      log(buf, GET_LEVEL(ch), LOG_GOD);
    }    
    TOGGLE_BIT(vict->pcdata->punish, PUNISH_UNLUCKY);
  }

  if (!strncasecmp(name, "nopray", i) || !strncasecmp(name, "nemke", i))
  {
    if(IS_SET(vict->pcdata->punish, PUNISH_NOPRAY))
    {
      send_to_char("The gods will once again hear your prayers.\n\r", vict);
      send_to_char("But not necessarily answer them...\n\r", vict);
      send_to_char("NOPRAY (nemke) removed.\n\r", ch);
    }
    else
    {
      csendf(vict, "%s has removed your ability to pray!\n\r", GET_NAME(ch));
      send_to_char("NOPRAY (nemke) set.\n\r", ch);
    }
    TOGGLE_BIT(vict->pcdata->punish, PUNISH_NOPRAY);
  }

    
  display_punishes(ch, vict);
  return eSUCCESS;
}

void display_punishes(struct char_data *ch, struct char_data *vict)
{
  char buf [100];
  
  sprintf (buf, "$3Punishments for %s$R: ", GET_NAME(vict));
  send_to_char(buf, ch);

  if (IS_SET(vict->pcdata->punish, PUNISH_NONAME))
     send_to_char("noname ", ch);
  
  if (IS_SET(vict->pcdata->punish, PUNISH_SILENCED))
     send_to_char("Silence ", ch);
  
  if (IS_SET(vict->pcdata->punish, PUNISH_NOEMOTE))
     send_to_char("noemote ", ch);
  
  if (IS_SET(vict->pcdata->punish, PUNISH_LOG) && GET_LEVEL(ch) > 108)
     send_to_char("log ", ch);
  
  if (IS_SET(vict->pcdata->punish, PUNISH_FREEZE))
     send_to_char("Freeze ", ch);
  
  if (IS_SET(vict->pcdata->punish, PUNISH_SPAMMER))
     send_to_char("Spammer ", ch);
  
  if (IS_SET(vict->pcdata->punish, PUNISH_STUPID))
     send_to_char("Stupid ", ch);

  if (IS_SET(vict->pcdata->punish, PUNISH_NOTELL))
     send_to_char("notell ", ch);

  if (IS_SET(vict->pcdata->punish, PUNISH_NOARENA)) 
     send_to_char("noarena ", ch);

  if (IS_SET(vict->pcdata->punish, PUNISH_NOTITLE)) 
     send_to_char("notitle ", ch);

  if (IS_SET(vict->pcdata->punish, PUNISH_UNLUCKY)) 
     send_to_char("unlucky ", ch);

  if (IS_SET(vict->pcdata->punish, PUNISH_NOPRAY))
     send_to_char("nopray ", ch);

  send_to_char("\n\r", ch);

}


int do_colors(struct char_data *ch, char *argument, int cmd)
{
  char buf[200];

  send_to_char("Color codes are a $$ followed by a code.\r\n\r\n"
               " Code   Bold($$B)  Inverse($$I)  Both($$B$$I)\r\n", ch);

  send_to_char("  $$0$0)   $B********$R  $0$I***********$R  $0$B$I**********$R (plain 0 can't be seen normally)\r\n", ch);
  for(int i = 1; i < 8; i++) {
    sprintf(buf, "  $$$%d%d)   $B********$R  $%d$I***********$R  $%d$B$I**********$R\r\n", i, i, i, i);
    send_to_char(buf, ch);
  }
  send_to_char("\r\nTo return to 'normal' color use $$R.\r\n\r\n"
               "Example:  'This is $$Bbold and $$4bold $$R$$4red$R!' will print:\r\n"
               "           This is $Bbold and $4bold $R$4red$R!\r\n", ch);
  return eSUCCESS;
}
