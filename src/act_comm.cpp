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
#include <assert.h>
#include "terminal.h"
#include "connect.h"
#include "levels.h"
#include "room.h"
#include "mobile.h"
#include "player.h"
#include "character.h"
#include "obj.h"
#include "handler.h"
#include "interp.h"
#include "utility.h"
#include "act.h"
#include "db.h"
#include "returnvals.h"
#include "fileinfo.h"
#include "const.h"
#include <fmt/format.h>

extern bool MOBtrigger;

/* extern functions */
int is_busy(struct char_data *ch);

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

  if (IS_PC(ch) && argument != nullptr)
  {
    string arg1, remainder_args;
    tie(arg1, remainder_args) = half_chop(string(argument));
    if (arg1 == "help")
    {
      csendf(ch, "report       - Reports hps, mana, moves and ki. (default)\n\r");
      csendf(ch, "report xp    - Reports current xp, xp till next level and levels to be gained.\n\r");
      csendf(ch, "report help  - Shows different ways report can be used.\n\r");
      return eFAILURE;
    }

    if (arg1 == "xp")
    {
      // calculate how many levels a player could gain with their current XP
      uint8_t levels_to_gain = 0;
      int64_t players_exp = GET_EXP(ch);
      while (levels_to_gain < IMMORTAL)
      {
        if (players_exp >= (int64_t)exp_table[(int)GET_LEVEL(ch) + levels_to_gain + 1])
        {
          players_exp -= (int64_t)exp_table[(int)GET_LEVEL(ch) + levels_to_gain + 1];
          levels_to_gain++;
        }
        else
        {
          break;
        }
      }

      snprintf(report, 200, "XP: %lld, XP till level: %lld, Levels to gain: %u",
               GET_EXP(ch),
               (int64)(exp_table[(int)GET_LEVEL(ch) + 1] - (int64)GET_EXP(ch)),
               levels_to_gain);

      sprintf(buf, "$n reports '%s'", report);
      act(buf, ch, 0, 0, TO_ROOM, 0);

      csendf(ch, "You report: %s\n\r", report);
      return eSUCCESS;
    }
  }

  if(IS_NPC(ch) || IS_ANONYMOUS(ch))
    snprintf(report, 200, "%d%% hps, %d%% mana, %d%% movement, and %d%% ki.",
      MAX(1, ch->getHP()*100)  / MAX(1, GET_MAX_HIT(ch)),
      MAX(1, GET_MANA(ch)*100) / MAX(1, GET_MAX_MANA(ch)),
      MAX(1, GET_MOVE(ch)*100) / MAX(1, GET_MAX_MOVE(ch)),
      MAX(1, GET_KI(ch)*100)   / MAX(1, GET_MAX_KI(ch)));
  else
  {
    snprintf(report, 200, "%d/%d hps, %d/%d mana, %d/%d movement, and %d/%d ki.",
      ch->getHP() , GET_MAX_HIT(ch),
      GET_MANA(ch), GET_MAX_MANA(ch),
      GET_MOVE(ch), GET_MAX_MOVE(ch),
      GET_KI(ch), GET_MAX_KI(ch));
  }

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
    case LOG_DEBUG:
    sprintf(typestr, "debug");
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
    if(!i->connected && GET_LEVEL(i->character) >= god_level) {
      if (IS_MOB(i->character) || IS_SET(i->character->pcdata->toggles, PLR_ANSI))
        send_to_char(buf1, i->character);
      else 
        send_to_char(buf, i->character);
    }
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
      "$B$2on$R"};

  char *types[] = {
      "bug", // 0
      "prayer",
      "god",
      "mortal",
      "socket",
      "misc", // 5
      "player",
      "gossip", // 7
      "auction",
      "info",
      "trivia", // 10
      "dream",
      "clan",
      "newbie",
      "shout",
      "world", // 15
      "arena",
      "logclan",
      "warnings",
      "help",
      "database", // 20
      "objects",
      "tell",
      "hints",
      "vault",
      "quest", // 25
      "debug",
      "\\@"};

  if (IS_NPC(ch))
    return eSUCCESS;

  if (*arg)
    one_argument(arg, buf);

  else
  {
    //    send_to_char("\n\r", ch);

    if (GET_LEVEL(ch) < IMMORTAL)
    {
      for (x = 7; x <= 14; x++)
      {
        if (IS_SET(ch->misc, (1 << x)))
          y = 1;
        else
          y = 0;
        sprintf(buf2, "%-9s%s\n\r", types[x], on_off[y]);
        send_to_char(buf2, ch);
      }
    }
    else
    {
      int o = GET_LEVEL(ch) == 110 ? 21 : 19;
      for (x = 0; x <= o; x++)
      {
        if (IS_SET(ch->misc, (1 << x)))
          y = 1;
        else
          y = 0;
        sprintf(buf2, "%-9s%s\n\r", types[x], on_off[y]);
        send_to_char(buf2, ch);
      }
    }

    if (IS_SET(ch->misc, 1 << 22))
      y = 1;
    else
      y = 0;
    sprintf(buf2, "%-9s%s\n\r", types[22], on_off[y]);
    send_to_char(buf2, ch);

    if (IS_SET(ch->misc, 1 << 23))
      y = 1;
    else
      y = 0;
    sprintf(buf2, "%-9s%s\n\r", types[23], on_off[y]);
    send_to_char(buf2, ch);

    int o = GET_LEVEL(ch) == 110 ? 26 : 0;
    for (x = 24; x <= o; x++)
    {
      if (IS_SET(ch->misc, (1 << x)))
        y = 1;
      else
        y = 0;
      sprintf(buf2, "%-9s%s\n\r", types[x], on_off[y]);
      send_to_char(buf2, ch);
    }

    return eSUCCESS;
  }

  for (x = 0; x <= 27; x++)
  {
    if (x == 27)
    {
      send_to_char("That type was not found.\n\r", ch);
      return eSUCCESS;
    }
    if (is_abbrev(buf, types[x]))
      break;
  }

  if (GET_LEVEL(ch) < IMMORTAL &&
      (x < 7 || (x > 14 && x < 22)))
  {
    send_to_char("That type was not found.\n\r", ch);
    return eSUCCESS;
  }
  if (x > 19 && GET_LEVEL(ch) != 110 && x < 22)
  {
    send_to_char("That type was not found.\n\r", ch);
    return eSUCCESS;
  }
  if (IS_SET(ch->misc, (1 << x)))
  {
    sprintf(buf, "%s channel turned $B$4OFF$R.\n\r", types[x]);
    send_to_char(buf, ch);
    REMOVE_BIT(ch->misc, (1 << x));
  }
  else
  {
    sprintf(buf, "%s channel turned $B$2ON$R.\n\r", types[x]);
    send_to_char(buf, ch);
    SET_BIT(ch->misc, (1 << x));
  }
  return eSUCCESS;
}

command_return_t do_ignore(char_data *ch, string args, int cmd)
{
  if (ch == nullptr)
  {
    return eFAILURE;
  }

  if (IS_MOB(ch))
  {
    ch->send("You're a mob! You can't ignore people.\r\n");
    return eFAILURE;
  }

  if (args.empty())
  {
    if (ch->pcdata->ignoring.empty())
    {
      ch->send("Ignore who?\r\n");
      return eSUCCESS;
    }

    // convert ignoring map into "char1 char2 char3" format
    string ignoreString = {};
    for (auto& ignore : ch->pcdata->ignoring)
    {
      if (ignore.second.ignore)
      {
        if (ignoreString.empty())
        {
          ignoreString = ignore.first;
        }
        else
        {
          ignoreString = ignoreString + " " + ignore.first;
        }
      }      
    }
    ch->send(fmt::format("Ignoring: {}\r\n", ignoreString));

    return eSUCCESS;
  }

  string arg1 = {}, remainder_args = {};
  tie(arg1, remainder_args) = half_chop(args);
  if (arg1.empty())
  {
    ch->send("Ignore who?\r\n");
    return eFAILURE;
  }
  arg1[0] = toupper(arg1[0]);

  if (ch->pcdata->ignoring.contains(arg1) == false)
  {
    ch->pcdata->ignoring[arg1] = {true, 0};
    ch->send(fmt::format("You now ignore anyone named {}.\r\n", arg1));
  }
  else
  {
    ch->pcdata->ignoring.erase(arg1);
    ch->send(fmt::format("You stop ignoring {}.\r\n", arg1));
  }
  return eSUCCESS;
}

int is_ignoring(const char_data *const ch, const char_data *const i)
{
  if (IS_MOB(ch) || (GET_LEVEL(i) >= IMMORTAL && IS_PC(i)) || ch->pcdata->ignoring.empty())
  {
    return false;
  }

  if (GET_NAME(i) == nullptr)
  {
    return false;
  }

  if (ch->pcdata->ignoring.contains(GET_NAME(i)))
  {
    return true;
  }

  // Since it didn't match the whole name, see if it matches one of
  // the name keywords used for a mob name
  string names = GET_NAME(i);
  string name1 = {}, remainder_names = {};
  tie(name1, remainder_names) = half_chop(names);
  while (name1.empty() == false)
  {
    if (ch->pcdata->ignoring.contains(name1))
    {
      return true;
    }

    tie(name1, remainder_names) = half_chop(remainder_names);
  }

  return false;
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
   char *buf = NULL;

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
