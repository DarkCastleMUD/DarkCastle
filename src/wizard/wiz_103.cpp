/********************************
| Level 103 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"

#include <utility.h>
#include <mobile.h>
#include <player.h>
#include <db.h>
#include <connect.h>
#include <interp.h>
#include <room.h>
#include <handler.h>
#include <returnvals.h>
#include <spells.h>
#include <clan.h>
#include <race.h>

extern char* pc_clss_types[];
extern struct room_data ** world_array;

int do_boot(struct char_data *ch, char *arg, int cmd)
{
  struct char_data *victim;
  int room;
  char name[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], buf[500];

  half_chop(arg,name,type);

  if(!(*name)) {
    send_to_char("Syntax: boot <victim> [boot]\n\r", ch);
    send_to_char("The boot option causes the victim to see a large ASCII boot.\n\r", ch);
    return eFAILURE;
  }
  
  victim = get_pc_vis(ch, name);

  if(victim) {
    if (!IS_NPC(victim) && (GET_LEVEL(ch) <= GET_LEVEL(victim))) {
      act("You cast a stream of fire at $N.", ch, 0, victim, TO_CHAR, 0);
      act("$n casts a stream of fire at you.", ch, 0, victim, TO_VICT, 0);
      act("$n casts a stream of fire at $N.", ch, 0, victim, TO_ROOM, NOTVICT);
      return eFAILURE;
    } 
    if (!IS_MOB(victim) && victim->pcdata->possesing) {
       send_to_char ("Oops! They ain't linkdead! Just possessing.", ch);
       return eFAILURE;
    }
    if(IS_AFFECTED(victim, AFF_CANTQUIT))
    {
      if(affected_by_spell(victim, FUCK_PTHIEF))
      {
        act("$N is a thief.  Don't boot $M.", ch, 0, victim, TO_CHAR, 0);
        return eFAILURE;
      }
      act("$N is a pkiller.  Don't boot $M.", ch, 0, victim, TO_CHAR, 0);
      return eFAILURE;
    }

    /* Still here? Ok, the boot continues */
    send_to_char("You have been disconnected.\r\n", victim);
    send_to_char("Ok.\n\r", ch);
    if (!IS_NPC(victim)) {
      sprintf(buf, "A stream of fire arcs down from the heavens, striking "
              "you between the eyes.\n\rYou have been removed from the "
              "world by %s.\n\r",
             GET_SHORT(ch));
      send_to_char(buf, victim);
    }

    room = victim->in_room;

    act("A stream of fire arcs down from the heavens, striking "
        "$n between the eyes.\r\n$n has been removed from the world "
        "by $N.", victim, 0, ch, TO_ROOM, INVIS_NULL);

    sprintf(name,"%s has booted %s.", GET_NAME(ch), GET_NAME(victim));
    log(name, GET_LEVEL(ch), LOG_GOD);

    if(!strcmp(type, "boot")) {
       send_to_char(
"\n\r"
"                       $1/               /\n\r"
"                      $1/               /\n\r"
"                     $1/               /\n\r"
"                    $1/               /$R\n\r"
"                   $5/\\_             $1/$R\n\r"
"      $5____        /   \\_          $1/$R\n\r"
"     $5/    \\      /\\     \\_       $1/$R\n\r"
"    $5/      \\    /\\        \\_    $1/$R\n\r"
"   $5/        \\  /\\           \\_ $1/$R\n\r"
"  $5/          \\/\\              /\n\r"
"  |\\                         /\n\r"
"   \\\\                       /\n\r"
"    \\\\                     /\n\r"
"     \\\\                   /\n\r"
"      \\\\                 /\n\r"
"       \\\\               /\n\r"
"        \\\\             /\n\r"
"         \\\\           /\n\r"
"          \\\\         /\n\r"
"           \\\\       /\n\r"
"            \\\\     /\n\r"
"             \\\\   /\n\r"
"              \\__/$R\n\r", victim);

}
    if(!strcmp(type, "kitty")) {
       send_to_char("\n\r"
"                __                             $6$B$I___$R            $6$B$I_yygL$R\n\r"
"$B$6i wuv u!$R       #####gy_,                    $6$B$Iy#######g   __g########g$R\n\r"
"    \\        g#F   `M##bg.                $6$B$Ig#\"'    ###g####~'    9##L$R\n\r"
"             ##F       `###g____yyyyy_____$6$B$Ij#\"        ###          ##E$R\n\r"
"            a#F           3##\"#~~~~~~~###$6$B$I##\"          ##g          ##$R\n\r"
"           j#F                           $6$B$I5#      ____ _##y__       ##1$R\n\r"
"           a#                           $6$B$Iy##    _g##~####\"#M##g     ##1$R\n\r"
"           #E                           $6$B$IJ#L    ##$R  $6$B$Ig#\"'     `#########g_$R\n\r"
"          o#1                           $6$B$I##     ##$R $6$B$Iy#E         ##L     9#,$R\n\r"
"           #g                           $6$B$I##      ####F         3#g      ##$R\n\r"
"          a#F                           $6$B$I3#L       ##L         ##M#.    ##!$R\n\r"
"         g#F                             $6$B$I##_     _##g       _g#F$R $6$B$I#g   y##$R\n\r"
"        _#F                               $6$B$I~###g###~M##g_   y###yg#'  y##'$R\n\r"
"        ##                                           $6$B$I`?\"M###        g##F$R\n\r"
"       ##'                                                $6$B$I?#,      ###L$R\n\r"
"       #E                                                  $6$B$I##g___g#$R\"###\n\r"
"      J#F                                                    $6$B$I`M##'$R   ##L\n\r"
"      ##                                                              ##\n\r"
"      ##                                                              ##\n\r"
"      ##                                                              ##L\n\r"
"   ___##y_.      a#o                                                __##1\n\r"
"##\"\"F~5#F        ###L                                 __          #M#M###M##\n\r"
"      J#K        ###L                                g##g             ##\n\r"
"     _y##ga       ~           _amog                  ####            ##F\n\r"
" a###~'\"#1                   d#   \"#                 \"##          #wy##L.\n\r"
"        3#,                   #g__g\"                                ##\"\"5##g\n\r"
"         ##g#                    ''                                a##    '~\n\r"
"    __y#\"FH#_                                                  y_ g##\n\r"
"   ##\"'     ##g                                                 \"###g_\n\r"
"   ~         `9#g_                                            _g##'\"9##gg,\n\r"
"                 ?##gy_.                                   _y##\"'      `##\n\r"
"                     ~\"####ggy_____                  ___g###F'\n\r"
"                             \"~~~~~##################~~~\"\n\r", victim);
    }
    move_char(victim, real_room(3001));
    do_quit(victim, "", 666);
  }

  else 
     send_to_char("Boot Who?\n\r", ch); 

  return eSUCCESS;
}
    
int do_disconnect(struct char_data *ch,  char *argument, int cmd)
{
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *d;
    unsigned sdesc;

    if (IS_NPC(ch))
            return eFAILURE;


    one_argument(argument, arg);
    sdesc=atoi(arg);
    if(arg==0){
            send_to_char("Illegal descriptor number.\n\r",ch);
            send_to_char("Usage: release <#>\n\r",ch);
            return eFAILURE;
      }
    for(d=descriptor_list;d;d=d->next)
    {
        if (d->descriptor==sdesc)
        {
            if (d->character && (GET_LEVEL(d->character) > GET_LEVEL(ch))) 
            {
                sprintf(buf, "Heh, %s tried to disconnect you. He has paid.\n\r", GET_NAME(ch));
                send_to_char(buf, d->character);
                send_to_char("You dummy, can't do that to your elders!\n\r", ch);
                close_socket(ch->desc);
                return eFAILURE;
            }
            else {
                close_socket(d);
                sprintf(buf,"Closing socket to descriptor #%d\n\r",sdesc);
                send_to_char(buf,ch);
                return eFAILURE;
            }
        }
    }
    send_to_char("Descriptor not found!\n\r",ch);
    return eSUCCESS;
}

int do_fsave(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  char name[100], buf[400]; 

  if(IS_NPC(ch))
    return eFAILURE;

  one_argument(argument, name);

  if(!*name)
    send_to_char("Who do you wish to force to save?\n\r", ch);

  if(!(vict = get_char_vis(ch, name)))
    send_to_char("No-one by that name here..\n\r", ch);

  else {
    /*if((GET_LEVEL(ch) <= GET_LEVEL(vict)) && !IS_NPC(vict)) {
      send_to_char("Hahaha\n\r", ch);
      sprintf(buf, "$n has failed to force you to save.");
      act(buf,  ch, 0, vict, TO_VICT, 0);
    }
    else */{
      if (ch->pcdata->stealth == FALSE) {
        sprintf(buf, "$n has forced you to 'save'.");
        act(buf,  ch, 0, vict, TO_VICT, 0);
        strcpy(buf,"");
        send_to_char("Ok.\n\r", ch);
      }
      do_save(vict, "", 9);
      sprintf(buf,"%s just forced %s to save.", GET_NAME(ch),
              GET_NAME(vict));
      log(buf, GET_LEVEL(ch), LOG_GOD);
    }
  }
  return eSUCCESS;
}

int do_fighting(struct char_data *ch, char *argument, int cmd)
{
  const int CLANTAG_LEN = MAX_CLAN_LEN+3; // "[Foobar]"
  struct char_data *i;
  bool arenaONLY = false;
  int countFighters = 0;
  char buf[80];
  char arg[MAX_STRING_LENGTH];
  struct clan_data *ch_clan = 0;
  char ch_clan_name[CLANTAG_LEN];
  ch_clan_name[0] = 0;
  struct clan_data *victim_clan = 0;
  char victim_clan_name[CLANTAG_LEN];
  victim_clan_name[0] = 0;
  
  one_argument(argument, arg);
  if (!strcmp(arg, "arena"))
    arenaONLY = true;

  for(i = combat_list; i; i = i->next_fighting) {
    // Don't show mobs fighting or people not in the arena when arena
    // keyword was specified.
    if(IS_NPC(i)
       || (arenaONLY && !IS_SET(world[i->in_room].room_flags, ARENA))) {
      continue;
    } else {
      countFighters++;
    }
  
    // If they're in a clan
    ch_clan_name[0] = '\0';
    if ((ch_clan = get_clan(i)))
      snprintf(ch_clan_name, CLANTAG_LEN, "[%s]", ch_clan->name);    

    if((victim_clan = get_clan(i->fighting)))
      snprintf(victim_clan_name, CLANTAG_LEN, "[%s]", victim_clan->name);

    snprintf(buf, 80, "%s %s fighting %s %s (%d)\n\r",
	     GET_NAME(i), ch_clan_name,
	     GET_NAME(i->fighting), victim_clan_name,
	     world[i->in_room].number);
    send_to_char(buf, ch);
  }
  
  
  if(countFighters == 0) {
    if (arenaONLY)
      send_to_char("No fighting characters found in the arena.\n\r", ch);
    else
      send_to_char("No fighting characters found.\n\r", ch);
  }
  return eSUCCESS;
}

int do_peace( struct char_data *ch, char *argument, int cmd )
{
    struct char_data *rch;

    for (rch = world[ch->in_room].people; rch!=NULL; rch = rch->next_in_room) {
        if ( IS_MOB(rch) && rch->mobdata->hatred != NULL )
            remove_memory(rch, 'h');
        if ( rch->fighting != NULL )
            stop_fighting(rch);
    }
    act("$n makes a gesture and all fighting stops.", ch,0,0,TO_ROOM, 0);
    send_to_char( "You stop all fighting in this room.\n\r", ch );
    return eSUCCESS;
}


int do_matrixinfo(struct char_data *ch, char *argument, int cmd)
{
  extern struct race_shit race_info[];
  extern char *isr_bits[];
  extern char *race_abbrev[];

  char buf[MAX_STRING_LENGTH];
  int i = 0;
  buf[0] = '\0';
  for (; i < MAX_RACE; i++)
  {
    char immbuf[MAX_STRING_LENGTH], resbuf[MAX_STRING_LENGTH], susbuf[MAX_STRING_LENGTH];
    immbuf[0] = resbuf[0] = susbuf[0] = '\0';
    sprintbit(race_info[i].immune, isr_bits, immbuf);
    sprintbit(race_info[i].resist, isr_bits, resbuf);
    sprintbit(race_info[i].suscept, isr_bits, susbuf);

    char hatbuf[MAX_STRING_LENGTH], fribuf[MAX_STRING_LENGTH];
    hatbuf[0] = fribuf[0] = '\0';
    sprintbit(race_info[i].hate_fear<<1, race_abbrev, hatbuf);
    sprintbit(race_info[i].friendly<<1, race_abbrev, fribuf);

    sprintf(buf, "%s %s - Imm: %s Res: %s Sus: %s\r\n    Hates: %s Friend: %s\r\n",
		buf, race_info[i].plural_name, immbuf, resbuf, susbuf, hatbuf, fribuf);
  }
  send_to_char(buf,ch);
  return eSUCCESS;
}

int lookupClass(char_data *ch, char *str)
{
  int c_class;

  if (str != 0) {
    str[0] = toupper(str[0]);
    for(c_class=1; c_class <= CLASS_MAX; c_class++) {
      if(is_abbrev(str, pc_clss_types[c_class])) {
	return c_class;
      }
    }
  }

  if (ch != 0) {
    send_to_char("Invalid class.\n\r\n\r", ch);
    send_to_char("Valid classes:\n\r", ch);
    for (c_class=1; c_class <= CLASS_MAX; c_class++) {
      csendf(ch, "%s\n\r", pc_clss_types[c_class]);
    }
  }

  return -1;
}

int lookupRoom(char_data *ch, char *str)
{
  if (str == 0)
    return -1;

  int room = atoi(str);

  if (room < 0 || room > top_of_world || world_array[room] == 0 || (room == 0 && str[0] != '0')) {
    if (ch) {
      send_to_char("No such room exists.\n\r", ch);
    }

    return -1;
  }

  return room;
}

int do_guild(struct char_data *ch, char *argument, int cmd)
{
  int c_class = 0, room = 0, old_room = 0;
  char arg1[MAX_STRING_LENGTH] = { 0 };
  char arg2[MAX_STRING_LENGTH] = { 0 };

  if (IS_NPC(ch))
    return eFAILURE;

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  // No arguments
  if (arg1[0] == 0) {
    send_to_char("Syntax:\n\r", ch);
    send_to_char("guild <room #>           - List all classes allowed in room\n\r", ch);
    send_to_char("guild <class>            - List all rooms that allow that class\n\r", ch);   
    send_to_char("guild <class> <room #>   - Toggle allow/deny class in room\n\r\n\r", ch);
    return eFAILURE;
  }

  // guild <room #> or guild <class>
  if (arg2[0] == 0) {
    if (is_number(arg1)) {
      // guild <room #>
      room = lookupRoom(ch, arg1);
      if (room == -1) {
	return eFAILURE;
      }

      csendf(ch, "Allow list for room #%d: ", room);
      bool found = FALSE;
      for(c_class=1; c_class < CLASS_MAX; c_class++) {
	if (world_array[room]->allow_class[c_class] == TRUE) {
	  found = TRUE;
	  csendf(ch, "%s ", pc_clss_types[c_class]);
	}
      }
      
      if (found) {
	send_to_char("\n\r", ch);
      } else {
	send_to_char("All\n\r", ch);
      }
      
      return eSUCCESS;
    } else {
      // guild <class>
      c_class = lookupClass(ch, arg1);
      if (c_class == -1) {
	return eFAILURE;
      }

      int count = 0;
      csendf(ch, "%s only rooms:\n\r", pc_clss_types[c_class]);

      int cols = 0;
      for (int r = 0; r < top_of_world; r++) {
	if (world_array[r] && world_array[r]->allow_class[c_class] == TRUE) {
	  csendf(ch, "%5d ", r);
	  
	  count++;
	  cols++;
	  if (cols == 11) {
	    cols = 0;
	    send_to_char("\n\r", ch);
	  }
	}
      }

      if (count == 0) {
	send_to_char("None found.\n\r", ch);
      } else {
	send_to_char("\n\r", ch);
      }

      return eSUCCESS;
    }
  }

  // guild <class> <room #>
  c_class = lookupClass(ch, arg1);
  if (c_class == -1) {
    return eFAILURE;
  }

  room = lookupRoom(ch, arg2);
  if (room == -1) {
    return eFAILURE;
  }

  if(!can_modify_room(ch, room)) {
    send_to_char("You are unable to work creation outside of your range.\n\r", ch);
    return eFAILURE;
  }

  if (world_array[room]->allow_class[c_class] == TRUE) {
    csendf(ch, "Removed %s class from room #%d's allow list.\n\r", pc_clss_types[c_class], room);
    world_array[room]->allow_class[c_class] = FALSE;
  } else {
    csendf(ch, "Added %s class to room #%d's allow list.\n\r", pc_clss_types[c_class], room);
    world_array[room]->allow_class[c_class] = TRUE;
  }
  
  set_zone_modified_world(room);
  
  old_room = ch->in_room;
  ch->in_room = room;
  do_rsave(ch, "", 9);
  ch->in_room = old_room;
  
  return eSUCCESS;
}


