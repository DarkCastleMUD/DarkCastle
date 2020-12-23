/********************************
| Level 101 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"

#include <utility.h>
#include <mobile.h>
#include <levels.h>
#include <interp.h>
#include <room.h>
#include <obj.h>
#include <character.h>
#include <terminal.h>
#include <handler.h>
#include <player.h>
#include <connect.h>
#include <returnvals.h>
#include <spells.h>
#include "const.h"
#include <queue>
#include <string>

using namespace std;

queue<string> imm_history;
queue<string> imp_history;

#define MAX_MESSAGE_LENGTH      4096

extern struct room_data ** world_array;

int do_wizhelp(struct char_data *ch, char *argument, int cmd_arg)
{
  extern struct command_info cmd_info[];

  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  int cmd, i;
  int no = 6;
  int no2 = 6;
  int no3 = 6;

  if(IS_NPC(ch))
    return eFAILURE;

  buf[0] = '\0';
  buf2[0] = '\0';
  buf3[0] = '\0';

  if (argument && *argument)
  {
    char arg[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);
    one_argument(argument, arg2);
    sprintf(buf, "Arg1: %s\n",arg);    
    send_to_char(buf,ch);
    sprintf(buf, "Arg2: %s\n",arg2);    
    send_to_char(buf,ch);
    return eSUCCESS;
  }
  send_to_char("Here are your godly powers:\n\r\n\r", ch);

  int v;
  for (v = GET_LEVEL(ch); v > 100; v--)
  for(cmd = 0; cmd_info[cmd].command_name[0] != '\0'; cmd++) {
     if(cmd_info[cmd].minimum_level == GIFTED_COMMAND &&
	v == GET_LEVEL(ch))
     {
       for(i = 0; *bestowable_god_commands[i].name != '\n'; i++)
         if(!strcmp(bestowable_god_commands[i].name, cmd_info[cmd].command_name))
           break;
       
       if( *bestowable_god_commands[i].name == '\n') // someone forgot to update it
          continue;

       if(!has_skill(ch, bestowable_god_commands[i].num))
          continue;

       if (bestowable_god_commands[i].testcmd == false) {
	 sprintf(buf2 + strlen(buf2), "[GFT]%-11s", cmd_info[cmd].command_name);
	 if((no2) % 5 == 0)
	   strcat(buf2, "\n\r");
	 no2++;
       } else {
	 sprintf(buf3 + strlen(buf3), "[TST]%-11s", cmd_info[cmd].command_name);
	 if((no3) % 5 == 0)
	   strcat(buf3, "\n\r");
	 no3++;
       }

       continue;
     }
     if (cmd_info[cmd].minimum_level != v || cmd_info[cmd].minimum_level == GIFTED_COMMAND) continue;

     // ignore these 2 duplicates of other commands
     if( !strcmp(cmd_info[cmd].command_name, "colours") ||
	 !strcmp(cmd_info[cmd].command_name, ";"))
       continue;

     sprintf(buf + strlen(buf), "[%2d]%-11s",
             cmd_info[cmd].minimum_level,
             cmd_info[cmd].command_name);

     if((no) % 5 == 0)
       strcat(buf, "\n\r");
     no++;
  }

  strcat(buf, "\n\r\n\r"
              "Here are the godly powers that have been gifted to you:\n\r\n\r");
  strcat(buf, buf2);
  strcat(buf, "\r\n");
  strcat(buf, "Here are the godly test powers that have been gifted to you:\n\r\n\r");
  strcat(buf, buf3);
  strcat(buf, "\r\n");

  send_to_char(buf, ch);
  return eSUCCESS;
}

int do_goto(struct char_data *ch, char *argument, int cmd)
{
    char buf[MAX_INPUT_LENGTH];
    int loc_nr, location, i, start_room;
    struct char_data *target_mob, *pers;
    struct char_data *tmp_ch;
    struct follow_type *k, *next_dude;
    struct obj_data *target_obj;
    extern int top_of_world;

    if (IS_NPC(ch))
        return eFAILURE;

    
    one_argument(argument, buf);
    if (!*buf)
    {
        send_to_char("You must supply a room number or a name.\n\r", ch);
        return eFAILURE;
    }

   start_room = ch->in_room;
   location = -1;

   if(isdigit(*buf) &&
         ((strlen(buf) < 2) || (strlen(buf) >= 2 && !strchr(buf, '.') ))) 
   { 
     loc_nr = atoi(buf);
     if (loc_nr > top_of_world || loc_nr < 0) 
     {
       send_to_char("No room exists with that number.\n\r", ch);
       return eFAILURE;
     }
     if(world_array[loc_nr])
       location = loc_nr;
     else
     {
       if(can_modify_this_room(ch, loc_nr)) {
         if(create_one_room(ch, loc_nr)) {
           send_to_char("You form order out of chaos.\n\r\n\r", ch);
           location = loc_nr;
         }
       }
     }
     if(location == -1)
     {
       send_to_char("No room exists with that number.\r\n", ch);
       return eFAILURE;
     }
   }
    else if( ( target_mob=get_pc_vis(ch, buf) ) )
      location = target_mob->in_room;

    else if((target_mob = get_char_vis(ch, buf)))
      location = target_mob->in_room;

    else if ( ( target_obj=get_obj_vis(ch, buf) ) )
      if(target_obj->in_room != NOWHERE)
        location = target_obj->in_room;
      else { 
        send_to_char("The object is not available.\n\r", ch);
        return eFAILURE;
      }
    else { 
      send_to_char("No such creature or object around.\n\r", ch);
      return eFAILURE;
    }

    /* a location has been found. */
    if (IS_SET(world[location].room_flags, IMP_ONLY) &&
        GET_LEVEL(ch) < OVERSEER) {
      send_to_char("No.\n\r", ch);
      return eFAILURE; 
    }

    /* Let's keep 104-'s out of clan halls....sigh... */
    if (IS_SET(world[location].room_flags, CLAN_ROOM) &&
        GET_LEVEL(ch) < DEITY) {
      send_to_char("For your protection, 104-'s may not be in clanhalls.\r\n", ch);
      return eFAILURE;
    }

	if ((IS_SET(world[location].room_flags, PRIVATE))
			&& (GET_LEVEL(ch) < OVERSEER)) {

		for (i = 0, pers = world[location].people; pers;
				pers = pers->next_in_room, i++)
			;
		if (i > 1) {
			send_to_char("There's a private conversation going on in "
					"that room.\n\r", ch);
			return eFAILURE;
		}
	}

  if(!IS_MOB(ch))
   for(tmp_ch=world[ch->in_room].people; tmp_ch;  tmp_ch=tmp_ch->next_in_room) {
      if ((CAN_SEE(tmp_ch, ch) && (tmp_ch != ch) 
            && !ch->pcdata->stealth)
           || (GET_LEVEL(tmp_ch) > GET_LEVEL(ch) && GET_LEVEL(tmp_ch) >  PATRON)) {
           ansi_color( RED, tmp_ch);
           ansi_color( BOLD, tmp_ch );
           send_to_char(ch->pcdata->poofout, tmp_ch);
           send_to_char ("\n\r", tmp_ch);
           ansi_color( NTEXT, tmp_ch);
      }
      else if (tmp_ch != ch && !ch->pcdata->stealth) {
             ansi_color( RED, tmp_ch);
             ansi_color( BOLD, tmp_ch);
             send_to_char("Someone disappears in a puff of smoke.\n\r",  tmp_ch);
             ansi_color( NTEXT, tmp_ch);
           }
    }

    move_char(ch, location);

  if(!IS_MOB(ch))
   for(tmp_ch=world[ch->in_room].people; tmp_ch;  tmp_ch=tmp_ch->next_in_room) {
      if (( CAN_SEE(tmp_ch, ch) && (tmp_ch != ch)
            && !ch->pcdata->stealth)
           || (GET_LEVEL(tmp_ch) > GET_LEVEL(ch) && GET_LEVEL(tmp_ch) >  PATRON)){

           ansi_color( RED,tmp_ch);
           ansi_color( BOLD,tmp_ch);
           send_to_char(ch->pcdata->poofin, tmp_ch);
           send_to_char("\n\r", tmp_ch);
           ansi_color( NTEXT, tmp_ch);
      }
      else if (tmp_ch != ch && !ch->pcdata->stealth) {
        ansi_color( RED, tmp_ch);
        ansi_color( BOLD, tmp_ch);
        send_to_char("Someone appears with an ear-splitting bang!\n\r",  tmp_ch);
        ansi_color( NTEXT, tmp_ch);
      }

   }

  do_look(ch, "", 15);
 
  if(ch->followers)
    for(k = ch->followers; k; k = next_dude) {
       next_dude = k->next;
       if(start_room == k->follower->in_room && CAN_SEE(k->follower, ch) &&
          GET_LEVEL(k->follower) >= IMMORTAL) {
         csendf(k->follower, "You follow %s.\n\r\n\r", GET_SHORT(ch));
         do_goto(k->follower, argument, 9);
       }
    }
  return eSUCCESS;
}

int do_poof(struct char_data *ch, char *arg, int cmd)
{
    char inout[100], buf[100];
    int ctr, nope;
    char _convert[2];

    if (IS_NPC(ch)) {
      send_to_char("Mobs can't poof.\r\n", ch);
      return eFAILURE;
    }

    arg = one_argument(arg, inout);

    if (!*inout)  {
       send_to_char("Usage:\n\rpoof [i|o] <string>\n\r", ch);
       send_to_char("\n\rCurrent poof in is:\n\r", ch);
       send_to_char(ch->pcdata->poofin, ch);
       send_to_char("\n\r", ch);
       send_to_char("\n\rCurrent poof out is:\n\r", ch);
       send_to_char(ch->pcdata->poofout, ch);
       send_to_char("\n\r", ch);
       return eSUCCESS;
    }

    if (inout[0] != 'i' && inout[0] != 'o') {
       send_to_char("Usage:\n\rpoof [i|o] <string>\n\r", ch);
       return eFAILURE;
    }

    if (!*arg) {
       send_to_char("A poof type message was expected.\n\r", ch);
       return eFAILURE;
    }

    if(strlen(arg) > 72) {
       send_to_char("Poof message too long, must be under 72 characters long.\n\r", ch);
       return eFAILURE;
    }
 
    nope = 0;

    for(ctr = 0; (unsigned) ctr <= strlen(arg); ctr++) {
       if(arg[ctr] == '%') {
          if(nope == 0)
             nope = 1;
          else if(nope == 1) { 
             send_to_char("You can only include one % in your poofin ;)\n\r", ch);
             return eFAILURE; 
          } 
       }
    }
 
   if(nope == 0) {
      send_to_char("You MUST include your name. Use % to indicate where "
                 "you want it.\n\r", ch);
      return eFAILURE; 
   }

   /* For the first time, use strcpy to avoid that annoying space
      at the beginning
   */
   _convert[0] = arg[0];
   _convert[1] = '\0';
   if(arg[ctr] == '%') strcpy(buf, GET_NAME(ch));
   else strcpy(buf, _convert);

   /* No reason to assign _convert[1] every time through, is there? */
   for(ctr = 1; (unsigned) ctr < strlen(arg); ctr++) {
      _convert[0] = arg[ctr];

      if(arg[ctr] == '%') strcat(buf, GET_NAME(ch));
      else strcat(buf, _convert);
   }

   if (inout[0] == 'i')
   {
      if(ch->pcdata->poofin)
        dc_free(ch->pcdata->poofin);
      ch->pcdata->poofin = str_dup(buf);
   }
   else 
   {
      if(ch->pcdata->poofout)
        dc_free(ch->pcdata->poofout);
      ch->pcdata->poofout = str_dup(buf);
   }

   send_to_char("Ok.\n\r", ch);
   return eSUCCESS;
}

int do_at(struct char_data *ch, char *argument, int cmd)
{
    char command[MAX_INPUT_LENGTH], loc_str[MAX_INPUT_LENGTH];
    int loc_nr, location, original_loc;
    struct char_data *target_mob;
    struct obj_data *target_obj;
    //extern int top_of_world;
    
    if (IS_NPC(ch))
        return eFAILURE;

    half_chop(argument, loc_str, command);
    if (!*loc_str) {
        send_to_char("You must supply a room number or a name.\n\r", ch);
        return eFAILURE;
    }

    
    if(isdigit(*loc_str) && !strchr(loc_str, '.')) {
        loc_nr = atoi(loc_str);
        if((loc_nr == 0 && *loc_str != '0') ||
           ((location = real_room(loc_nr)) < 0))
        {
            send_to_char("No room exists with that number.\n\r", ch);
            return eFAILURE;
        }
    }
    else if ( ( target_mob = get_char_vis(ch, loc_str) ) != NULL )
        location = target_mob->in_room;
    else if ( ( target_obj = get_obj_vis(ch, loc_str) ) != NULL )
        if (target_obj->in_room != NOWHERE)
            location = target_obj->in_room;
        else {
            send_to_char("The object is not available.\n\r", ch);
            return eFAILURE;
        }
    else {
        send_to_char("No such creature or object around.\n\r", ch);
        return eFAILURE;
    }

    /* a location has been found. */
    if (IS_SET(world[location].room_flags, IMP_ONLY) && GET_LEVEL(ch) < IMP) {
       send_to_char("No.\n\r", ch);
       return eFAILURE; 
    }

    /* Let's keep 104-'s out of clan halls....sigh... */
    if (IS_SET(world[location].room_flags, CLAN_ROOM) &&
        GET_LEVEL(ch) < DEITY) {
      send_to_char("For your protection, 104-'s may not be in clanhalls.\r\n", ch);
      return eFAILURE;
    }

    original_loc = ch->in_room;
    move_char(ch, location, false);
    int retval = command_interpreter(ch, command);

    /* check if the guy's still there */
    for (target_mob = world[location].people; target_mob; target_mob =
        target_mob->next_in_room)
        if (ch == target_mob)
        {
	    move_char(ch, original_loc);
        }
   return retval;
}

int do_highfive(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim;
    char buf[200];
    
     if (IS_NPC(ch))
          return eFAILURE;

      one_argument(argument,buf);
     if (!*buf) {
          send_to_char("Who do you wish to high-five? \n\r", ch);
              return eFAILURE;
            }
       
       if (!(victim = get_char_vis(ch, buf)))  {
           send_to_char("No-one by that name in the world.\n\r", ch);
          return eFAILURE;
           }

      if (GET_LEVEL(victim) < IMMORTAL) {
        send_to_char ("What you wanna give a mortal a high-five for?! *smirk* \n\r", ch);
          return eFAILURE;
      }

      if(ch == victim) {
        sprintf(buf, "%s conjures a clap of thunder to resound the land!\n\r", GET_SHORT(ch) );
        send_to_all(buf);
      }
      else {
    sprintf(buf, "Time stops for a minute as %s and %s high-five!\n\r", GET_SHORT(ch), GET_SHORT(victim));
      send_to_all(buf);
    }
    return eSUCCESS;
}

int do_holylite(struct char_data *ch, char *argument, int cmd)
{
        if (IS_NPC(ch)) 
           return eFAILURE;

        if (argument[0] != '\0') {
           send_to_char(
           "HOLYLITE doesn't take any arguments; arg ignored.\n\r", ch);
     } /* if */

        if (ch->pcdata->holyLite) {
           ch->pcdata->holyLite = FALSE;
           send_to_char("Holy light mode off.\n\r",ch);
     }
        else {
           ch->pcdata->holyLite = TRUE;
           send_to_char("Holy light mode on.\n\r",ch);
     } /* if */
  return eSUCCESS;
}

int do_wizinvis(struct char_data *ch, char *argument, int cmd)
{
    char buf [200];

   int arg1;

   if(IS_NPC(ch)){
        return eFAILURE;
      }

      arg1 = atoi (argument);

        if (arg1 < 0) 
          arg1 = 0;
         

    if (!*argument) {
      if (ch->pcdata->wizinvis == 0) {
           ch->pcdata->wizinvis = GET_LEVEL(ch);
         } else {
           ch->pcdata->wizinvis = 0;
             }
           } else {
           if (arg1 > GET_LEVEL(ch))
              arg1 = GET_LEVEL(ch);
           ch->pcdata->wizinvis = arg1;
             }
   sprintf(buf, "WizInvis Set to: %ld \n\r", ch->pcdata->wizinvis);
     send_to_char(buf,ch);
   return eSUCCESS;
}

int do_nohassle (struct char_data *ch, char *argument, int cmd)
{
   if (IS_NPC(ch))
      return eFAILURE;

   if (IS_SET(ch->pcdata->toggles, PLR_NOHASSLE))  {
            REMOVE_BIT(ch->pcdata->toggles, PLR_NOHASSLE);
                 send_to_char("Mobiles can bother you again.\n\r",ch);
        } else {
         SET_BIT(ch->pcdata->toggles, PLR_NOHASSLE);
        send_to_char("Those pesky mobiles will leave you alone now.\n\r", ch);
   }
   return eSUCCESS;
}

// cmd == 9 - imm
// cmd == 8 - /
int do_wiz(struct char_data *ch, char *argument, int cmd)
{
    char buf1[MAX_STRING_LENGTH];
    struct descriptor_data *i;

    if(IS_NPC(ch))
      return eFAILURE;

    if(cmd != 9 && !has_skill(ch, COMMAND_IMP_CHAN)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
    }

    for (; *argument == ' '; argument++);

    if (!(*argument))
    {
      queue<string> tmp;
      if(cmd == 9)
      {
        tmp = imm_history;
        send_to_char("Here are the last 10 imm messages:\r\n", ch);
      }
      else if(cmd == 8)
      {
        tmp = imp_history;
        send_to_char("Here are the last 10 imp messages:\r\n", ch);
      }
      else
      {
        send_to_char("What? How did you get here?? Contact a coder.\r\n", ch);
        return eSUCCESS;
      }

      while(!tmp.empty())
      {
        send_to_char((tmp.front()).c_str(), ch);
        tmp.pop();
      }
    }
    else 
    {
        if(cmd == 9)
        {
          sprintf(buf1, "$B$4%s$7: $7$B%s$R\n\r", GET_SHORT(ch), argument);
          imm_history.push(buf1);
          if(imm_history.size() > 10) imm_history.pop();
        }
        else
        {
          sprintf(buf1, "$B$7%s> %s$R\n\r", GET_SHORT(ch), argument);
          imp_history.push(buf1);
          if(imp_history.size() > 10) imp_history.pop();
        }

	send_to_char(buf1, ch);
        ansi_color( NTEXT, ch);

        for (i = descriptor_list; i; i = i->next) {
	  if (i->character && i->character != ch && GET_LEVEL(i->character) >= IMMORTAL && !IS_NPC(i->character)) {
	  if (cmd == 8 && !has_skill(i->character, COMMAND_IMP_CHAN)) continue;

	    if (STATE(i) == CON_PLAYING) {
	      send_to_char(buf1, i->character);
	    } else {
	      record_msg(buf1, i->character);
	    }
	  }
	}
    }
    return eSUCCESS;
}

int do_findfix(char_data *ch, char *argument, int cmd)
{
  int i, j, z;

  // Lazy code. Nested fors > thinking.
  char buf[MAX_STRING_LENGTH];
  buf[0] = '\0';
  extern int top_of_zonet;
  for (i = 0; i < top_of_zonet; i++)
  {
    for (j = 0; zone_table[i].cmd[j].command != 'S'; j++)
    {
      bool first = TRUE, found = FALSE;
      if (zone_table[i].cmd[j].command != 'M') continue;
      int vnum = zone_table[i].cmd[j].arg1, max = zone_table[i].cmd[j].arg2;
      if (zone_table[i].cmd[j].arg2 == 1 ||
	  zone_table[i].cmd[j].arg2 == -1) continue; // Don't care about those..
       int amt = 0;
       for (z = 0; zone_table[i].cmd[z].command != 'S'; z++)
       {
         if (zone_table[i].cmd[z].command != 'M') continue;
	 if (zone_table[i].cmd[z].arg1 != vnum) continue;
	 if (z == j && found) { first = FALSE; break; }
	 found = TRUE;
         if (zone_table[i].cmd[z].arg2 > max) max = zone_table[i].cmd[z].arg2;
         amt++;
       }
       if (!first) continue;
       if (amt == max) continue;
       if (strlen(buf) > MAX_STRING_LENGTH -200)
       {
	 i = 10000; // Hack to make it end immediatly.
	 break;
       }
       if (amt > max)
       {
	 sprintf(buf, "%sReset %d in zone %d has MORE resets than max in world.\r\n",buf, j, i); 
       } else {
	 sprintf(buf, "%sReset %d in zone %d has LESS resets than max in world.\r\n",buf, j, i); 
       }

    }
  }
  send_to_char(buf, ch);
  return eSUCCESS;
}


int do_varstat(char_data *ch, char *argument, int cmd)
{
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);
    char_data *vict;

    if ( ( vict = get_char_vis(ch, arg) ) == NULL )
    {
	send_to_char("Target not found.\r\n",ch);
	return eFAILURE;
    }
    char buf[MAX_STRING_LENGTH];
    buf[0] = '\0';
    struct tempvariable *eh;
    for (eh = vict->tempVariable; eh; eh = eh->next)
    {
       sprintf(buf, "$B$3%-30s $R-- $B$5 %s\r\n",
		eh->name, eh->data);
       send_to_char(buf,ch);
    }
    if (buf[0] == '\0') 
    { send_to_char("No temporary variables found.\r\n",ch); }
    return eSUCCESS;
}
