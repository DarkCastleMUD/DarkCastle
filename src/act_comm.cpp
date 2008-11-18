/***************************************************************************
 *  file: act_comm.c , Implementation of commands.         Part of DIKUMUD *
 *  Usage : Communication.                                                 *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec\\@garnet.berkeley.edu.                                          *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
extern "C"
{
  #include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <assert.h>
#include <terminal.h>
#include <connect.h>
#include <levels.h>
#include <room.h>
#include <mobile.h>
#include <player.h>
#include <character.h>
#include <obj.h>
#include <handler.h>
#include <interp.h>
#include <utility.h>
#include <act.h>
#include <db.h>
#include <returnvals.h>
#include <fileinfo.h>

extern CWorld world;
extern struct descriptor_data *descriptor_list;
extern bool MOBtrigger;

/* extern functions */
int is_busy(struct char_data *ch);
struct char_data *get_pc_vis(struct char_data *ch, char *name);
void send_to_char_regardless(char *messg, struct char_data *ch);
int is_ignoring(struct char_data *ch, struct char_data *i);
char *str_str(char *first, char *second);


int do_report(struct char_data *ch, char *argument, int cmd)
{
  char buf[256];
  char report[200];
  
  assert(ch != 0);
  if(ch->in_room == NOWHERE)
  {
    log("NOWHERE sent to do_report!", OVERSEER, LOG_BUG);
    return eSUCCESS;
  }

  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eSUCCESS;
  }

  if(IS_NPC(ch) || IS_ANONYMOUS(ch))
    sprintf(report, "%d%% hps, %d%% mana, %d%% movement, and %d%% ki.",
      MAX(1, GET_HIT(ch)*100)  / MAX(1, GET_MAX_HIT(ch)),
      MAX(1, GET_MANA(ch)*100) / MAX(1, GET_MAX_MANA(ch)),
      MAX(1, GET_MOVE(ch)*100) / MAX(1, GET_MAX_MOVE(ch)),
      MAX(1, GET_KI(ch)*100)   / MAX(1, GET_MAX_KI(ch)));
  else
    sprintf(report, "%d/%d hps, %d/%d mana, %d/%d movement, and %d/%d ki.",
      GET_HIT(ch) , GET_MAX_HIT(ch),
      GET_MANA(ch), GET_MAX_MANA(ch),
      GET_MOVE(ch), GET_MAX_MOVE(ch),
      GET_KI(ch), GET_MAX_KI(ch));

  sprintf(buf, "$n reports '%s'", report);
  act(buf, ch, 0, 0, TO_ROOM, 0);

  csendf(ch, "You report: %s\n\r", report);

  return eSUCCESS;
}

/************************************************************************
| send_to_gods
| Preconditions: str != 0
| Postconditions: None
| Side effects: None
| Returns: 0 on failure, non-zero on success
| Notes:
*/
int send_to_gods(const char *str, int god_level, long type)
{
  char buf1[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char typestr[30];
  struct descriptor_data *i;

  if(str == 0) {
    log("NULL STRING sent to send_to_gods!", OVERSEER, LOG_BUG);
    return(0);
  }
  if ((god_level > IMP) || (god_level < 0)) { // Outside valid god levels
    log("Invalid level sent to send_to_gods!", OVERSEER, LOG_BUG);
    return(0);
  }
  
  switch (type) {
      case LOG_BUG:
	  sprintf(typestr, "bug");
	  break;
      case LOG_PRAYER:
	  sprintf(typestr, "pray");
	  break;
      case LOG_GOD:
	  sprintf(typestr, "god");
	  break;
      case LOG_MORTAL:
	  sprintf(typestr, "mortal");
	  break;
      case LOG_SOCKET:
	  sprintf(typestr, "socket");
	  break;
      case LOG_MISC:
	  sprintf(typestr, "misc");
	  break;
      case LOG_PLAYER:
	  sprintf(typestr, "player");
	  break;
      case LOG_WORLD:
	  sprintf(typestr, "world");
	  break;
      case LOG_ARENA:
	  sprintf(typestr, "arena");
	  break;
      case LOG_CLAN:
	  sprintf(typestr, "logclan");
	  break;
      case LOG_WARNINGS:
	  sprintf(typestr, "warnings");
	  break;
      case LOG_DATABASE:
	  sprintf(typestr, "database");
	  break;
      case LOG_VAULT:
	  sprintf(typestr, "vault");
	  break;
      case LOG_HELP:
	  sprintf(typestr, "help");
	  break;
      case LOG_OBJECTS:
	  sprintf(typestr, "objects");
	  break;
      case LOG_QUEST:
	  sprintf(typestr, "quest");
	  break;
      default:
	  sprintf(typestr, "unknown");
	  break;
  }

  sprintf(buf, "//(%s) %s\n\r", typestr, str);
  sprintf(buf1, "%s%s//%s(%s)%s %s%s %s%s%s\n\r",
          BOLD, RED, NTEXT, typestr, BOLD, YELLOW, str, RED, NTEXT, GREY);

  for(i = descriptor_list; i; i = i->next) {
    if((i->character == NULL) || (GET_LEVEL(i->character) <= MORTAL))
      continue;
    if(!(IS_SET(i->character->misc, type)))
      continue;
    if(is_busy(i->character))
      continue;
    if(!i->connected && GET_LEVEL(i->character) >= god_level)
      if (IS_MOB(i->character) || IS_SET(i->character->pcdata->toggles, PLR_ANSI))
        send_to_char(buf1, i->character);
      else 
        send_to_char(buf, i->character);
  }
  return(1);
}



int do_channel(struct char_data *ch, char *arg, int cmd)
{
  int x;
  int y = 0; 
  char buf[200];
  char buf2[200];
 
  char *on_off[] = {
    "$B$4off$R",
    "$B$2on$R"
  };

  char *types[] = {
    "bug",       // 0
    "prayer",
    "god",
    "mortal",
    "socket",    
    "misc",      // 5
    "player",
    "gossip",    // 7
    "auction",
    "info",
    "trivia",    // 10
    "dream",
    "clan",
    "newbie",
    "shout",
    "world",    // 15
    "arena",
    "logclan",
    "warnings",
    "help",
    "database",   // 20
    "objects",
    "tell",
    "hints",
    "vault",
    "quest",      // 25
 "\\@"
  };

  if(IS_NPC(ch))
    return eSUCCESS;

  if(*arg)
    one_argument(arg, buf);

  else { 
//    send_to_char("\n\r", ch);

    if(GET_LEVEL(ch) < IMMORTAL) {
      for(x = 7; x <= 14; x++) {
         if(IS_SET(ch->misc, (1<<x)))
           y = 1;
         else
           y = 0;
         sprintf(buf2, "%-9s%s\n\r", types[x], on_off[y]); 
         send_to_char(buf2, ch);
      }
    }
    else {
	int o = GET_LEVEL(ch) == 110 ? 21:19;
      for(x = 0; x <= o; x++) {
         if(IS_SET(ch->misc, (1<<x)))
           y = 1;
         else
           y = 0;
         sprintf(buf2, "%-9s%s\n\r", types[x], on_off[y]); 
         send_to_char(buf2, ch);
      }
    }

    if(IS_SET(ch->misc, 1<<22))
       y = 1;
    else
       y = 0;
    sprintf(buf2, "%-9s%s\n\r", types[22], on_off[y]); 
    send_to_char(buf2, ch);

    if(IS_SET(ch->misc, 1<<23))
       y = 1;
    else
       y = 0;
    sprintf(buf2, "%-9s%s\n\r", types[23], on_off[y]); 
    send_to_char(buf2, ch);
    
    int o = GET_LEVEL(ch) == 110 ? 25:0;
    for(x = 24; x <= o; x++) {
	if(IS_SET(ch->misc, (1<<x)))
	    y = 1;
	else
	    y = 0;
	sprintf(buf2, "%-9s%s\n\r", types[x], on_off[y]); 
	send_to_char(buf2, ch);
    }

    return eSUCCESS;
  }

  for(x = 0; x <= 26; x++) {
     if(x == 26) {
       send_to_char("That type was not found.\n\r", ch);
       return eSUCCESS;
     }
     if(is_abbrev(buf, types[x]))
       break;
  }

  if(GET_LEVEL(ch) < IMMORTAL &&
      ( x < 7 || x > 14 && x < 22) ) {
    send_to_char("That type was not found.\n\r", ch);
    return eSUCCESS;
  }
  if (x > 19 && GET_LEVEL(ch) != 110 && x < 22)
  {
    send_to_char("That type was not found.\n\r", ch);
    return eSUCCESS;
  }
  if(IS_SET(ch->misc, (1<<x))) {
    sprintf(buf, "%s channel turned $B$4OFF$R.\n\r", types[x]);
    send_to_char(buf, ch);
    REMOVE_BIT(ch->misc, (1<<x));
  }
  else {
    sprintf(buf, "%s channel turned $B$2ON$R.\n\r", types[x]);
    send_to_char(buf, ch);
    SET_BIT(ch->misc, (1<<x));
  }
  return eSUCCESS;
}

int do_ignore(struct char_data *ch, char *arg, int cmd)
{
  char buf[300], buf2[300], buf3[300], new_list[300];
  char tmp_buf[300];
  /*int x;*/

  new_list[0] = '\0';

  if(IS_MOB(ch)) {
    send_to_char("You're a mob!  You can't ignore people.\r\n", ch);
    return eFAILURE;
  }

  if(!*arg) {
    if(!ch->pcdata->ignoring) { 
      send_to_char("Ignore who?\n\r", ch);
      return eSUCCESS;
    }
    sprintf(tmp_buf,"Ignoring: %s\n\r", ch->pcdata->ignoring);
    send_to_char(tmp_buf, ch);
    return eSUCCESS;  
  }

  one_argument(arg, buf);

  if(!(ch->pcdata->ignoring)) {
    buf[0] = UPPER(buf[0]);
    /* str_dup will ALWAYS succeed or the mud will abort() */
    ch->pcdata->ignoring = str_dup(buf);
    csendf(ch, "You now ignore anyone named %s.\n\r", buf);
  }
  else if(!isname(buf, ch->pcdata->ignoring)) {
   // THIS will fix the problem -- don't know who did this
    if(strlen(ch->pcdata->ignoring) + strlen(buf) > 100) {
      send_to_char("If you want to ignore that many people, "
                   "why are you mudding?\n\r", ch);
      return eSUCCESS;
    }
    strcpy(buf2, ch->pcdata->ignoring);
    strcat(buf2, " ");
    buf[0] = UPPER(buf[0]);
    strcat(buf2, buf);
    dc_free(ch->pcdata->ignoring);
    ch->pcdata->ignoring = str_dup(buf2);
    csendf(ch, "You now ignore anyone named %s.\n\r", buf);
  }
  else {
    strcpy(buf3, ch->pcdata->ignoring);
    for(;;) {
      half_chop(buf3, buf2, buf3);
      if(!isname(buf, buf2))
        strcat(new_list, buf2);
      if(!*buf3)
        break;
      strcat(new_list, " ");
    }
    dc_free(ch->pcdata->ignoring);
    ch->pcdata->ignoring = str_dup(new_list);
    csendf(ch, "You stop ignoring %s\n\r", buf);
  }
  return eSUCCESS;
}


int is_ignoring(struct char_data *ch, struct char_data *i)
{
  if(IS_MOB(ch) || GET_LEVEL(i) > MORTAL || !ch->pcdata->ignoring)
    return(0);
  
   if(isname(GET_NAME(i), ch->pcdata->ignoring))
     return(1);
  
  return(0);
}




#define MAX_NOTE_LENGTH 1000      /* arbitrary */

int do_write(struct char_data *ch, char *argument, int cmd)
{
    struct obj_data *paper = 0, *pen = 0;
    char papername[MAX_INPUT_LENGTH], penname[MAX_INPUT_LENGTH],
	buf[MAX_STRING_LENGTH];

    argument_interpreter(argument, papername, penname);

    if (!ch->desc)
	return eSUCCESS;
    if (GET_LEVEL(ch) < 5)
    {
      send_to_char("You need to be at least level 5 to write on the board.\r\n",ch);
      return eSUCCESS;
    }

    if (!*papername)  /* nothing was delivered */
    {   
	send_to_char(
	    "Write? with what? ON what? what are you trying to do??\n\r", ch);
	return eSUCCESS;
    }

    if(*penname) /* there were two arguments */ 
      {
	if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
	{
	    sprintf(buf, "You have no %s.\n\r", papername);
	    send_to_char(buf, ch);
	    return eSUCCESS;
	}
	if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying)))
	{
	    sprintf(buf, "You have no %s.\n\r", papername);
	    send_to_char(buf, ch);
	    return eSUCCESS;
	}
    }
    else  /* there was one arg.let's see what we can find */
    {           
	if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
	{
	    sprintf(buf, "There is no %s in your inventory.\n\r", papername);
	    send_to_char(buf, ch);
	    return eSUCCESS;
	}
	if (paper->obj_flags.type_flag == ITEM_PEN)  /* oops, a pen.. */
	{
	    pen = paper;
	    paper = 0;
	}
	else if (paper->obj_flags.type_flag != ITEM_NOTE)
	{
	    send_to_char("That thing has nothing to do with writing.\n\r", ch);
	    return eSUCCESS;
	}

	/* one object was found. Now for the other one. */
	if (!ch->equipment[HOLD])
	{
	    sprintf(buf, "You can't write with a %s alone.\n\r", papername);
	    send_to_char(buf, ch);
	    return eSUCCESS;
	}
	if (!CAN_SEE_OBJ(ch, ch->equipment[HOLD]))
	{
	    send_to_char("The stuff in your hand is invisible! Yeech!!\n\r", ch);
	    return eSUCCESS;
	}
	
	if (pen)
	    paper = ch->equipment[HOLD];
	else
	    pen = ch->equipment[HOLD];
    }
	    
    /* ok.. now let's see what kind of stuff we've found */
    if (pen->obj_flags.type_flag != ITEM_PEN)
    {
	act("$p is no good for writing with.",ch,pen,0,TO_CHAR,0);
    }
    else if (paper->obj_flags.type_flag != ITEM_NOTE)
    {
	act("You can't write on $p.", ch, paper, 0, TO_CHAR, 0);
    }
    else if (paper->action_description)
/*    else if (paper->item_number != real_object(1205) )  */
	send_to_char("There's something written on it already.\n\r", ch);
    else
    {
	/* we can write - hooray! */
		
	send_to_char("Ok.. go ahead and write.. end the note with a \\@.\n\r",
	    ch);
	act("$n begins to jot down a note.", ch, 0,0,TO_ROOM, INVIS_NULL);
	ch->desc->strnew = &paper->action_description;
	ch->desc->max_str = MAX_NOTE_LENGTH;
    }
    return eSUCCESS;
}


// TODO - Add a bunch of insults to this for the hell of it.
int do_insult(struct char_data *ch, char *argument, int cmd)
{
  char buf[100];
  char arg[MAX_STRING_LENGTH];
  struct char_data *victim;
                        
  one_argument(argument, arg);
                                
  if(*arg) {
    if(!(victim = get_char_room_vis(ch, arg))) {
      send_to_char("Can't hear you!\n\r", ch);
    } else {
      if(victim != ch) {
        sprintf(buf, "You insult %s.\n\r",GET_SHORT(victim) );
        send_to_char(buf,ch);
                    
        switch(number(0,3)) {
        case 0: 
          if (GET_SEX(victim) == SEX_MALE)
            act("$n accuses you of fighting like a woman!", ch, 0, victim, TO_VICT, 0);
          else act("$n says that women can't fight.", ch, 0, victim, TO_VICT, 0);
          break;
        case 1:
          if (GET_SEX(victim) == SEX_MALE)
            act("$n accuses you of having the smallest.... (brain?)", ch, 0, victim, TO_VICT, 0);
          else act("$n tells you that you'd loose a beauty contest against a troll.", ch, 0, victim, TO_VICT, 0);
          break;
        case 2: 
          act("$n calls your mother a bitch!", ch, 0, victim, TO_VICT , 0);
          break;
        default : 
          act("$n tells you that you have big ears!", ch,0,victim,TO_VICT, 0);
          break;
        } // end switch 
    
        act("$n insults $N.", ch, 0, victim, TO_ROOM, NOTVICT);
      } else { /* ch == victim */
        send_to_char("You feel insulted.\n\r", ch);
      }
    }
  } else send_to_char("Insult who?\n\r", ch);
  return eSUCCESS;
}                   
                
int do_emote(struct char_data *ch, char *argument, int cmd)
{
  int i;
  char buf[MAX_STRING_LENGTH];
                                    
  if(!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_NOEMOTE)) {
    send_to_char("You can't show your emotions!!\n\r", ch);
    return eSUCCESS;
  }
                            
  for(i = 0; *(argument + i) == ' '; i++);
                        
  if(!*(argument + i))
    send_to_char("Yes.. But what?\n\r", ch);
  else {
    sprintf(buf,"$n %s", argument + i);
    // don't want players triggering mobs with emotes
    MOBtrigger = FALSE;
    act(buf,ch,0,0,TO_ROOM, 0);
    csendf(ch, "%s %s\n\r", GET_SHORT(ch), argument + i);
    MOBtrigger = TRUE;
  }
  return eSUCCESS;
}

#define MAX_HINTS 100
char hints[MAX_HINTS][MAX_STRING_LENGTH];

void load_hints()
{
   FILE *fl;
   int num;
   char *buf;

   if(!(fl = dc_fopen(HINTS_FILE, "r"))) {
      log("Error opening the hint file", IMMORTAL, LOG_MISC);
      return;
   }

   while(fgetc(fl) != '$') {
      num = fread_int(fl, 0, INT_MAX);
      if(num > MAX_HINTS) {
         log("Raise MAX_HINTS or something.", IMMORTAL, LOG_MISC);
         break;
      }
      buf = fread_string(fl, 0);
      strcpy(hints[num-1],buf);
   }

   dc_free(buf);

   dc_fclose(fl);

   return;
}

void send_hint()
{
   char hint[MAX_STRING_LENGTH];
   struct descriptor_data *i;
   int num = number(0, MAX_HINTS-1);

   while(hints[num][0] == '\0') num = number(0, MAX_HINTS-1);
   
   sprintf(hint, "$B$5HINT:$7 %s$R\n\r", hints[num]);

   for(i = descriptor_list;i;i = i->next) {
      if(i->connected || !i->character || !i->character->desc || is_busy(i->character) || IS_NPC(i->character))continue;
      if(IS_SET(i->character->misc, CHANNEL_HINTS)) send_to_char(hint, i->character);
   }

   return;
}
