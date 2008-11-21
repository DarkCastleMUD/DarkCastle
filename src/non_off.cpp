/************************************************************************
| $Id: non_off.cpp,v 1.49 2008/11/21 00:36:51 kkoons Exp $
| non_off.C
| Description:  Implementation of generic, non-offensive commands.
*/
/*****************************************************************************/
/* Revision History                                                          */
/* 12/08/2003   Onager   Revised do_tap() to prevent sacrifices in donations */
/*****************************************************************************/
extern "C"
{
  #include <ctype.h>
  #include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <connect.h>
#include <character.h>
#include <room.h>
#include <obj.h>
#include <mobile.h>
#include <utility.h>
#include <levels.h>
#include <handler.h>
#include <db.h>
#include <interp.h>
#include <player.h>
#include <act.h>
#include <spells.h>
#include <fight.h>
#include <returnvals.h>
#include <comm.h>
#include <structs.h>
#include <utility.h>
#include <fileinfo.h>
#include <string>
#include <vector>

extern CWorld world;
extern struct index_data *obj_index;
 
void log_sacrifice(CHAR_DATA *ch, OBJ_DATA *obj, bool decay = FALSE)
{ //decay variable means it's from a decaying corpse, not a player
  FILE *fl;
  long ct;
  char *tmstr;

  if(GET_OBJ_RNUM(obj) == NOWHERE) return;

  if(!(fl = dc_fopen(OBJECTS_LOG, "a"))) {
    log("Could not open the objects log file.", IMMORTAL, LOG_BUG);
    return;
  }

  ct = time(0);
  tmstr = asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  if (!decay)
    fprintf(fl, "%s :: %s just sacrificed %s[%d]\n", tmstr, GET_NAME(ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj));
  else
    fprintf(fl, "%s :: %s just poofed from decaying corpse %s[%d]\n", tmstr, GET_OBJ_SHORT((OBJ_DATA*)ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj));

  for(OBJ_DATA *loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content) {
    fprintf(fl, "%s :: The %s contained %s[%d]\n",
	    tmstr,
	    GET_OBJ_SHORT(obj),
	    GET_OBJ_SHORT(loop_obj),
	    GET_OBJ_VNUM(loop_obj));
  }

  dc_fclose(fl);
}

int do_tap(struct char_data *ch, char *argument, int cmd)
{
  struct obj_data *obj;
  char name[MAX_INPUT_LENGTH+1];


  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  one_argument (argument, name);

  if(!*name || !str_cmp(name, GET_NAME(ch))) {
    act("$n offers $mself to $s god, who graciously declines.", ch, 0, 0, TO_ROOM, 0);
    act( "Your god appreciates your offer and may accept it later.", ch, 0, 0, TO_CHAR, 0);
    return eSUCCESS;
  }

  obj = get_obj_in_list_vis(ch, name, ch->carrying);

  /* Ok, lets see if it's a corpse on the ground then */
 if(obj == NULL) {
   obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents);
   if(obj == NULL || GET_ITEM_TYPE(obj) != ITEM_CONTAINER || !isname("corpse", obj->name) || isname("pc", obj->name)) {
     act("You don't seem to be holding that object.", ch, 0, 0, TO_CHAR, 0);
     return eFAILURE;
   }
 }

  if(IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
    if(GET_LEVEL(ch) < IMMORTAL) {
      send_to_char("You are unable to destroy this item, it must be CURSED!\n\r", ch);
      return eFAILURE;
    }
    else
      send_to_char("(This item is cursed, BTW.)\n\r", ch);
  }
  
  if(obj->obj_flags.value[3] == 1 && isname("pc", obj->name)) {
    send_to_char("You probably don't *really* want to do that.\n\r", ch);
    return eFAILURE;
  }

  if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL) && GET_LEVEL(ch) < ANGEL) {
    send_to_char("God, what a stupid fucking thing for you to do.\n\r", ch);
    return eFAILURE;
  }

  if(IS_AFFECTED(ch, AFF_CANTQUIT) && !IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF)) {
    send_to_char("Your criminal acts prohibit it.\n\r", ch);
    return eFAILURE;
  }

  /* don't let people sac stuff in donations */
  if (ch->in_room == real_room(3099)) {
     send_to_char("Not in the donation room.\n\r", ch);
     return(eFAILURE);
  }

  act("$n sacrifices $p to $s god.", ch, obj, 0, TO_ROOM , 0);
  act("You sacrifice $p to the gods and receive one gold coin.", ch, obj, 0, TO_CHAR , 0);
  GET_GOLD(ch) += 1;
  log_sacrifice(ch, obj);
  extract_obj(obj);
  return eSUCCESS;
}

int do_visible(struct char_data *ch, char *argument, int cmd)
{
  if(affected_by_spell(ch, SPELL_INVISIBLE)) {
    affect_from_char(ch, SPELL_INVISIBLE);
    send_to_char("You drop your invisiblity spell.\r\n", ch);
    if(!IS_AFFECTED(ch, AFF_INVISIBLE))
      act("$n slowly fades into existence.", ch, 0, 0, TO_ROOM, 0);
    else 
      send_to_char("You must remove the equipment making you invis to become visible.\r\n", ch);
    return eSUCCESS;
  }

  if(IS_AFFECTED(ch, AFF_INVISIBLE)) 
    send_to_char("You must remove the equipment making you invis to become visible.\r\n", ch);
  else
    send_to_char("You aren't invisible.\r\n", ch);

  return eSUCCESS;
}

int do_donate(struct char_data *ch, char *argument, int cmd)
{
    struct obj_data *obj;
    char name[MAX_INPUT_LENGTH+1];
    char buf[MAX_STRING_LENGTH];
    int location;
    int room= 3099;
    int origin;

  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  if(ch->fighting) {
    send_to_char("Aren't we a little to busy for that right now?\n\r", ch);
    return eFAILURE;
  }

  one_argument (argument, name);

  if(!*name) {
    send_to_char("Donate what?\n\r", ch);
    return eFAILURE;
  }

  obj = get_obj_in_list_vis( ch, name, ch->carrying );
  if( obj == NULL ) {
        sprintf(buf, "You don't have any '%s' to donate.", name);
	act(buf, ch, 0, 0, TO_CHAR, 0);
	return eFAILURE;
  }
  
  if(IS_AFFECTED(ch, AFF_CANTQUIT) && !IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF)) {
    send_to_char("Your criminal acts prohibit it.\n\r", ch);
    return eFAILURE;
  }

  // Handle yielding the champion flag
  if (GET_OBJ_VNUM(obj) == 45) {
    if (IS_SET(world[ch->in_room].room_flags, SAFE)) {
      if(IS_AFFECTED(ch, AFF_CHAMPION)) {
	REMBIT(ch->affected_by, AFF_CHAMPION);

	sprintf(buf, "\n\r##%s has just yielded the Champion flag!\n\r",
		GET_NAME(ch));
	send_info(buf);

        struct affected_type af;
        af.type      = OBJ_CHAMPFLAG_TIMER;
        af.duration  = 5;
        af.modifier  = 0;
        af.location  = APPLY_NONE;
        af.bitvector = -1;
        affect_to_char(ch, &af);

	act("$n yields $p.", ch, obj, 0, TO_ROOM, 0);
	act("You yield $p.", ch, obj, 0, TO_CHAR, 0);
 
	location = real_room(CFLAG_HOME);
	origin = ch->in_room;
	move_char(ch, location);

	act("$p falls from the heavens...", ch, obj, 0, TO_ROOM, INVIS_NULL);
      
	move_char(ch, origin);
	move_obj(obj, location);
      
	do_save(ch, "", 0);
	return eSUCCESS;
      } else {
	sprintf(buf, "%s had the champion flag, but no AFF_CHAMPION.",
		GET_NAME(ch));
	log(buf, IMMORTAL, LOG_BUG);
	return eFAILURE;
      }
    } else {
      send_to_char("You can only yield the Champion flag from a safe room.\n\r", ch);
      return eFAILURE;
    }
  }

  if(IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
    send_to_char("Since you can't let go of it, how are you going to donate it?\n\r", ch);
    return eFAILURE;
  }

  if(IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE)) { 
    if(GET_LEVEL(ch) > IMMORTAL) {
      send_to_char("That was a NO_TRADE item btw....\r\n", ch);
    } else {
      send_to_char("It seems magically attached to you.\r\n", ch);
      return eFAILURE;
    }
  }

  if(contains_no_trade_item(obj)) {
    if(GET_LEVEL(ch) > IMMORTAL) {
      send_to_char("That was a NO_TRADE item btw....\r\n", ch);
    } else {
      send_to_char("Something inside it seems magically attached to you.\r\n"
		   , ch);
      return eFAILURE;
    }
  } 

  if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) {
    do_gossip(ch, "I just tried to donate my gl, I'm fucking stupid.", 9);
    send_to_char("DAMN but you sure are stupid.\n\r", ch);
    return eFAILURE;
  }
  
  act("$n donates $p.", ch, obj, 0, TO_ROOM, 0);
  act("You donate $p.", ch, obj, 0, TO_CHAR, 0);
 
  if (obj->obj_flags.type_flag != ITEM_MONEY)
  {
     sprintf(log_buf, "%s donates %s[%d]", GET_NAME(ch), obj->name, obj_index[obj->item_number].virt);
     log(log_buf, IMP, LOG_OBJECTS);
     for(OBJ_DATA *loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content)
       logf(IMP, LOG_OBJECTS, "The %s contained %s[%d]", obj->short_description,
                          loop_obj->short_description,
                          obj_index[loop_obj->item_number].virt);
  }



  location = real_room(room);
  origin = ch->in_room;
  move_char(ch, location);

  act("$n has made a donation...", ch, obj, 0, TO_ROOM, 0);
  act("$p falls through a glowing white portal in the top of the ceiling.",
         ch, obj, 0, TO_ROOM, INVIS_NULL);
      
  move_char(ch, origin);
  move_obj(obj, location);
     
  do_save(ch, "", 0);
  return eSUCCESS;
}

int do_title(struct char_data *ch, char *argument, int cmd)
{
  char buf[100];
  int ctr;

  if (!*argument){
    send_to_char("Change your title to what?\n\r", ch);
    return eFAILURE;
  }

  if(!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_NOTITLE)) {
    send_to_char("You can't do that.  You must have been naughty.\n\r", ch);
    return eFAILURE;
  }
    
  for (; isspace(*argument); argument++);

  if (strlen(argument)>40){
    send_to_char("Title field too big.  40 characters max.\n\r", ch);
    return eFAILURE;
  }
 
  if(strchr(argument, '[') || strchr(argument, ']')) {
    send_to_char("You cannot have a '[' or a ']' in your title.\n\r", ch);
    return eFAILURE;
  }
  
  // TODO - decide if we still need this anymore since I think the $color code
  // keeps mortals from using $'s anyway  No idea why we don't let them use ?'s offhand
  for(ctr = 0; (unsigned) ctr <= strlen(argument); ctr++) {
    if(((argument[ctr] == '$') && (argument[ctr + 1] == '$')) || ((argument[ctr] == '?') && (argument[ctr + 1] == '?'))) {
      send_to_char("Your title is now: Common Dork of Dark Castle.\n\r", ch);
      return eFAILURE;
    }
  }

  if(ch->title) // this should always be true, but why not check anyway?
    dc_free(ch->title); 
  ch->title = str_dup(argument);
  sprintf(buf,"Your title is now: %s\n\r", argument);
  send_to_char(buf,ch);
  return eSUCCESS;
}


char * toggle_txt[] = {
  "brief",
  "compact",
  "beep",
  "anonymous",
  "ansi",
  "vt100",
  "wimpy",
  "pager-off",
  "bard-songs",
  "auto-eat",
  "summonable",
  "lfg",
  "charmiejoin",
  "notax",
  "guide",
  "news-up",
  "ascii",
  "damage",
  ""
};

int do_toggle(struct char_data * ch, char * arg, int cmd)
{
  int x;
  char buf[MAX_STRING_LENGTH];
  
  one_argument(arg, buf);
  
  if(IS_MOB(ch)) {
    send_to_char("You can't toggle anything, you're a mob!\r\n", ch);
    return eFAILURE;
  }

  if(!*buf) {
    for(x = 0; toggle_txt[x][0] != '\0'; x++) {
	if (x != 8 || GET_CLASS(ch) == CLASS_BARD)
	  if (x != 14 ||  (IS_SET(ch->pcdata->toggles, PLR_GUIDE)))
       sprintf(buf + strlen(buf), "%-10s ", toggle_txt[x]);

       switch(x) {
         case 0:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_BRIEF) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 1:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_COMPACT) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 2:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_BEEP) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 3:
	 sprintf(buf + strlen(buf), "%s\n\r",
           IS_SET(ch->pcdata->toggles, PLR_ANONYMOUS) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 4:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_ANSI) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 5:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_VT100) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 6:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_WIMPY) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 7:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_PAGER) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 8:
	if (GET_CLASS(ch) == CLASS_BARD)
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_BARD_SONG) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 9:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_AUTOEAT) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 10:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_SUMMONABLE) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 11:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_LFG) ? "$B$2on$R" : "$B$4off$R");
	 break;
	 
         case 12:
	 sprintf(buf + strlen(buf), "%s\n\r",
	   IS_SET(ch->pcdata->toggles, PLR_CHARMIEJOIN) ? "$B$2on$R" : "$B$4off$R");
	 break;
         case 13:
         sprintf(buf + strlen(buf), "%s\n\r",
           IS_SET(ch->pcdata->toggles, PLR_NOTAX) ? "$B$2on$R" : "$B$4off$R");
         break;
         case 14:
	if (IS_SET(ch->pcdata->toggles, PLR_GUIDE))
         sprintf(buf + strlen(buf), "%s\n\r",
           IS_SET(ch->pcdata->toggles, PLR_GUIDE_TOG) ? "$B$2on$R" : "$B$4off$R");
         break;
         case 15:
         sprintf(buf + strlen(buf), "%s\n\r",
           IS_SET(ch->pcdata->toggles, PLR_NEWS) ? "$B$2on$R" : "$B$4off$R");
	break;
         case 16:
         sprintf(buf + strlen(buf), "%s\n\r",
           IS_SET(ch->pcdata->toggles, PLR_ASCII) ? "$B$4off$R" : "$B$2on$R");
         break;
         case 17:
         sprintf(buf + strlen(buf), "%s\n\r",
           IS_SET(ch->pcdata->toggles, PLR_DAMAGE) ? "$B$2on$R" : "$B$4off$R");
         break;


	 
         default:
	 break;
       }
    }
    
    send_to_char(buf, ch);
    return eSUCCESS;
  }
  
  for(x = 0; toggle_txt[x][0] != '\0'; x++)
     if(is_abbrev(buf, toggle_txt[x]))
       break;
  
  if(toggle_txt[x][0] == '\0') {
    send_to_char("Bad option.  Type toggle with no arguments for a list of "
                 "good ones.\n\r", ch);
    return eFAILURE;
  }

  switch(x) {
    case 0:
    do_brief(ch, "", 9);
    break;
    
    case 1:
    do_compact(ch, "", 9);
    break;
    
    case 2:
    do_beep_set(ch, "", 9);
    break;
    
    case 3:
    do_anonymous(ch, "", 9);
    break;
    
    case 4:
    do_ansi(ch, "", 9);
    break;
    
    case 5:
    do_vt100(ch, "", 9);
    break;
    
    case 6:
    do_wimpy(ch, "", 9);
    break;
    
    case 7:
    do_pager(ch, "", 9);
    break;
    
    case 8:
   if (GET_CLASS(ch) == CLASS_BARD)
    do_bard_song_toggle(ch, "", 9);
   else send_to_char("You're not a bard!\r\n",ch);
    break;
    
    case 9:
    do_autoeat(ch, "", 9);
    break;
    
    case 10:
    do_summon_toggle(ch, "", 9);
    break;
    
    case 11:
    do_lfg_toggle(ch, "", 9);
    break;
    
    case 12:
    do_charmiejoin_toggle(ch, "", 9);
    break;

    case 13:
    do_notax_toggle(ch, "", 9);
    break;
    
    case 14:
	if (IS_SET(ch->pcdata->toggles, PLR_GUIDE))
    do_guide_toggle(ch, "", 9);
   else send_to_char("You're not a guide!\r\n",ch);
    break;

    case 15:
    do_news_toggle(ch, "", 9);
    break;

    case 16:
    do_ascii_toggle(ch, "", 9);
    break;
    case 17:
    do_damage_toggle(ch, "", 9);
    break;

    default:
    send_to_char("A bad thing just happened.  Tell the gods.\n\r", ch);
    break;
  }
  return eSUCCESS;
}

int do_brief(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
	return eFAILURE;

    if (IS_SET(ch->pcdata->toggles, PLR_BRIEF))
    {
	send_to_char("Brief mode $B$4off$R.\n\r", ch);
	REMOVE_BIT(ch->pcdata->toggles, PLR_BRIEF);
    }
    else
    {
	send_to_char("Brief mode $B$2on$R.\n\r", ch);
	SET_BIT(ch->pcdata->toggles, PLR_BRIEF);
    }
    return eSUCCESS;
}

int do_ansi(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
	return eFAILURE;

    if (IS_SET(ch->pcdata->toggles, PLR_ANSI))
    {
       send_to_char("ANSI COLOR $B$4off$R.\n\r", ch);
       REMOVE_BIT(ch->pcdata->toggles, PLR_ANSI);
    }
    else
    {
       send_to_char("ANSI COLOR $B$2on$R.\n\r", ch);
       SET_BIT(ch->pcdata->toggles, PLR_ANSI);
    }
    return eSUCCESS;
}

int do_vt100(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
	return eFAILURE;

    if (IS_SET(ch->pcdata->toggles, PLR_VT100))
    {
       send_to_char("VT100 $B$4off$R.\n\r", ch);
       REMOVE_BIT(ch->pcdata->toggles, PLR_VT100);
    }
    else
    {
       send_to_char("VT100 $B$2on$R.\n\r", ch);
       SET_BIT(ch->pcdata->toggles, PLR_VT100);
    }
    return eSUCCESS;
}


int do_compact(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
	return eFAILURE;

    if (IS_SET(ch->pcdata->toggles, PLR_COMPACT))
    {
	send_to_char( "Compact mode $B$4off$R.\n\r", ch);
	REMOVE_BIT(ch->pcdata->toggles, PLR_COMPACT);
    }
    else
    {
	send_to_char( "Compact mode $B$2on$R.\n\r", ch);
	SET_BIT(ch->pcdata->toggles, PLR_COMPACT);
    }
    return eSUCCESS;
}

int do_summon_toggle(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
	return eFAILURE;

    if (IS_SET(ch->pcdata->toggles, PLR_SUMMONABLE))
    {
	send_to_char( "You may no longer be summoned by other players.\n\r", ch);
	REMOVE_BIT(ch->pcdata->toggles, PLR_SUMMONABLE);
    }
    else
    {
	send_to_char( "You may now be summoned by other players.\n\r"
                      "Make _sure_ you want this...they could summon you to your death!\r\n", ch);
	SET_BIT(ch->pcdata->toggles, PLR_SUMMONABLE);
    }
    return eSUCCESS;
}

int do_lfg_toggle(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
	return eFAILURE;

    if (IS_SET(ch->pcdata->toggles, PLR_LFG))
    {
	send_to_char( "You are no longer Looking For Group.\n\r", ch);
	REMOVE_BIT(ch->pcdata->toggles, PLR_LFG);
    }
    else
    {
	send_to_char( "You are now Looking For Group.\n\r", ch);
	SET_BIT(ch->pcdata->toggles, PLR_LFG);
    }
    return eSUCCESS;
}

int do_guide_toggle(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
        return eFAILURE; 
    
    if (!IS_SET(ch->pcdata->toggles, PLR_GUIDE)) {
      send_to_char("You must be assigned as a $BGuide$R by the gods before you can toggle it.\r\n", ch);
      return eFAILURE; 
    }

    if (IS_SET(ch->pcdata->toggles, PLR_GUIDE_TOG))
    {
        send_to_char("You have hidden your $B(Guide)$R tag.\n\r", ch);
        REMOVE_BIT(ch->pcdata->toggles, PLR_GUIDE_TOG);
    }
    else 
    {
        send_to_char("You will now show your $B(Guide)$R tag.\n\r", ch);
        SET_BIT(ch->pcdata->toggles, PLR_GUIDE_TOG);
    }
    
    return eSUCCESS;
} 
int do_news_toggle(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
        return eFAILURE; 
    
    if (IS_SET(ch->pcdata->toggles, PLR_NEWS))
    {
        send_to_char("You now view news in an up-down fashion.\n\r", ch);
        REMOVE_BIT(ch->pcdata->toggles, PLR_NEWS);
    }
    else 
    {
        send_to_char("You now view news in a down-up fashion..\n\r", ch);
        SET_BIT(ch->pcdata->toggles, PLR_NEWS);
    }
    
    return eSUCCESS;
} 

int do_ascii_toggle(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
        return eFAILURE; 
    
    if (IS_SET(ch->pcdata->toggles, PLR_ASCII))
    {
        REMOVE_BIT(ch->pcdata->toggles, PLR_ASCII);
        send_to_char("Cards are now displayed through ASCII.\n\r", ch);
    }
    else 
    {
        send_to_char("Cards are no longer dislayed through ASCII.\n\r", ch);
        SET_BIT(ch->pcdata->toggles, PLR_ASCII);
    }
    
    return eSUCCESS;
} 

int do_damage_toggle(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
        return eFAILURE; 
    
    if (IS_SET(ch->pcdata->toggles, PLR_DAMAGE))
    {
        REMOVE_BIT(ch->pcdata->toggles, PLR_DAMAGE);
        send_to_char("Damage numbers will no longer be displayed in combat.\n\r", ch);
    }
    else 
    {
        send_to_char("Damage numbers will now be displayed in combat.\n\r", ch);
        SET_BIT(ch->pcdata->toggles, PLR_DAMAGE);
    }
    
    return eSUCCESS;
} 


int do_notax_toggle(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
        return eFAILURE;

    if (IS_SET(ch->pcdata->toggles, PLR_NOTAX))
    {
        send_to_char( "You will now be taxed on all your loot.\n\r", ch);
        REMOVE_BIT(ch->pcdata->toggles, PLR_NOTAX);
    }
    else
    {
        send_to_char( "You will no longer be taxed.\n\r", ch);
        SET_BIT(ch->pcdata->toggles, PLR_NOTAX);
    }

    return eSUCCESS;
}

int do_charmiejoin_toggle(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
	return eFAILURE;

    if (IS_SET(ch->pcdata->toggles, PLR_CHARMIEJOIN))
    {
	send_to_char( "Your followers will no longer automatically join you.\n\r", ch);
	REMOVE_BIT(ch->pcdata->toggles, PLR_CHARMIEJOIN);
    }
    else
    {
	send_to_char( "Your followers will automatically aid you in battle.\n\r", ch);
	SET_BIT(ch->pcdata->toggles, PLR_CHARMIEJOIN);
    }

    return eSUCCESS;
}

int do_autoeat(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
	return eFAILURE;

    if (IS_SET(ch->pcdata->toggles, PLR_AUTOEAT))
    {
	send_to_char( "You no longer automatically eat and drink.\n\r", ch);
	REMOVE_BIT(ch->pcdata->toggles, PLR_AUTOEAT);
    }
    else
    {
	send_to_char( "You now automatically eat and drink when hungry and thirsty.\n\r", ch);
	SET_BIT(ch->pcdata->toggles, PLR_AUTOEAT);
    }
    return eSUCCESS;
}

int do_anonymous(CHAR_DATA *ch, char *argument, int cmd)
{
  if(ch == 0)
  {
    log("Null char in do_anonymous.", OVERSEER, LOG_BUG);
    return eFAILURE;
  }
  if(GET_LEVEL(ch) < 20)
  {
    send_to_char("You are too inexperienced to disguise your profession.\n\r", ch);
    return eSUCCESS;
  }
  if(IS_SET(ch->pcdata->toggles, PLR_ANONYMOUS))
  {
    send_to_char("Your class and level information is now public.\n\r", ch);
  }
  else
  {
    send_to_char("Your class and level information is now private.\n\r", ch);
  }

  TOGGLE_BIT(ch->pcdata->toggles, PLR_ANONYMOUS);
  return eSUCCESS;
}

int do_wimpy(struct char_data *ch, char *argument, int cmd)
{
  if(IS_SET(ch->pcdata->toggles, PLR_WIMPY)) { 
    send_to_char("You are no longer a wimp....maybe.\n\r", ch);
    REMOVE_BIT(ch->pcdata->toggles, PLR_WIMPY);
    return eFAILURE;
  }

  send_to_char("You are now an official wimp.\n\r", ch);
  SET_BIT(ch->pcdata->toggles, PLR_WIMPY);
  return eSUCCESS;
}

// Remember that his is "no-pager".  So if it's set, we don't page
// If it's not set, we do.
int do_pager(struct char_data *ch, char *argument, int cmd)
{
  if(IS_SET(ch->pcdata->toggles, PLR_PAGER)) { 
    send_to_char("You now page your strings in 24 line chunks.\n\r", ch);
    REMOVE_BIT(ch->pcdata->toggles, PLR_PAGER);
    return eFAILURE;
  }

  send_to_char("You no longer page strings in 24 line chunks.\n\r", ch);
  SET_BIT(ch->pcdata->toggles, PLR_PAGER);
  return eSUCCESS;
}

int do_bard_song_toggle(struct char_data *ch, char *argument, int cmd)
{
  if(IS_SET(ch->pcdata->toggles, PLR_BARD_SONG)) { 
    send_to_char("Bard singing now in verbose mode.\n\r", ch);
    REMOVE_BIT(ch->pcdata->toggles, PLR_BARD_SONG);
    return eFAILURE;
  }

  send_to_char("Bard singing now in brief mode.\n\r", ch);
  SET_BIT(ch->pcdata->toggles, PLR_BARD_SONG);
  return eSUCCESS;
}

int do_beep_set(struct char_data *ch, char *arg, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->pcdata->toggles, PLR_BEEP))
    {
      REMOVE_BIT(ch->pcdata->toggles, PLR_BEEP);
      send_to_char ("\nTell is now silent.\n", ch);
      return eFAILURE;
    }

  SET_BIT(ch->pcdata->toggles, PLR_BEEP);
  send_to_char ("\nTell now beeps.\a\n", ch);
  return eSUCCESS;
}

int do_stand(CHAR_DATA *ch, char *argument, int cmd)
{
    switch(GET_POS(ch)) {
        case POSITION_STANDING : {
            act("You are already standing.",ch,0,0,TO_CHAR, 0);
        } break;
        case POSITION_SITTING   : {
            act("You stand up.",  ch,0,0,TO_CHAR, 0);
            act("$n clambers on $s feet.",ch, 0, 0, TO_ROOM, INVIS_NULL);
            if(ch->fighting)
              GET_POS(ch) = POSITION_FIGHTING;
            else GET_POS(ch) = POSITION_STANDING;
        } break;
        case POSITION_RESTING   : {
            act("You stop resting, and stand up.", ch,0,0,TO_CHAR, 0);
            act("$n stops resting, and clambers on $s feet.",
                ch, 0, 0, TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_STANDING;
        } break;
        case POSITION_SLEEPING : {
            act("You have to wake up first!", ch, 0,0,TO_CHAR, 0);
        } break;
        case POSITION_FIGHTING : {
            act("Do you not consider fighting as standing?",
                ch, 0, 0, TO_CHAR, 0);
        } break;
        default : {
            act("You stop floating around, and put your feet on the ground.", ch, 0, 0, TO_CHAR, 0);
            act("$n stops floating around, and puts $s feet on the ground.", ch, 0, 0, TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_STANDING;
        } break;
    }
    return eSUCCESS;
}
 
 
int do_sit(CHAR_DATA *ch, char *argument, int cmd)
{
 
    if (IS_SET(world[ch->in_room].room_flags, QUIET)) {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
    }
       
   switch(GET_POS(ch)) {
        case POSITION_STANDING : {
            act("You sit down.", ch, 0,0, TO_CHAR, 0);
            act("$n sits down.", ch, 0,0, TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_SITTING;
        } break;
        case POSITION_SITTING   : {
            send_to_char("You're sitting already.\n\r", ch);
        } break;
        case POSITION_RESTING   : {
            act("You stop resting, and sit up.", ch,0,0,TO_CHAR, 0);
            act("$n stops resting.", ch, 0,0,TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_SITTING;
        } break;
        case POSITION_SLEEPING : {
            act("You have to wake up first.", ch, 0, 0, TO_CHAR, 0);
        } break;
        case POSITION_FIGHTING : {
            act("Sit down while fighting? are you MAD?",
                ch,0,0,TO_CHAR, 0);
        } break;
        default : {
            act("You stop floating around, and sit down.",
                ch,0,0,TO_CHAR, 0);
            act("$n stops floating around, and sits down.",
                ch,0,0,TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_SITTING;
        } break;
    }
    return eSUCCESS;
}
 
 
int do_rest(CHAR_DATA *ch, char *argument, int cmd)
{
 
 
    if (IS_SET(world[ch->in_room].room_flags, QUIET))
      {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
      }
       
   switch(GET_POS(ch)) {
        case POSITION_STANDING : {
            act("You sit down and rest your tired bones.",
                ch, 0, 0, TO_CHAR, 0);
            act("$n sits down and rests.", ch, 0, 0, TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_RESTING;
        } break;
        case POSITION_SITTING : {
            act("You rest your tired bones.", ch, 0, 0, TO_CHAR, 0);
            act("$n rests.", ch, 0, 0, TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_RESTING;
        } break;
        case POSITION_RESTING : {
            act("You are already resting.", ch, 0, 0, TO_CHAR, 0);
        } break;
        case POSITION_SLEEPING : {
            act("You have to wake up first.", ch, 0, 0, TO_CHAR, 0);
            } break;
        case POSITION_FIGHTING : {
            act("Rest while fighting? are you MAD?", ch, 0, 0, TO_CHAR, 0);
        } break;
        default : {
            act("You stop floating around, and stop to rest your tired bones.", ch, 0, 0, TO_CHAR, 0);
            act("$n stops floating around, and rests.", ch, 0,0, TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_SITTING;
        } break;
    }
    return eSUCCESS;
}
 
 
int do_sleep(CHAR_DATA *ch, char *argument, int cmd)
{
   struct affected_type *paf;
    if (IS_SET(world[ch->in_room].room_flags, QUIET)) {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
    }
    if(IS_AFFECTED(ch, AFF_INSOMNIA)) {
       send_to_char("You are far too alert for that.\n\r", ch);
       return eFAILURE;
    }
    if (!IS_SET(world[ch->in_room].room_flags, SAFE))
      send_to_char ("Be careful sleeping out here!  This isn't a safe room, so people can steal your equipment while you sleep!\r\n", ch);

    if ((paf = affected_by_spell(ch, SPELL_SLEEP)) && 
		paf->modifier == 1 && GET_POS(ch) != POSITION_SLEEPING)
 	paf->modifier = 0;
         
    switch(GET_POS(ch)) {
        case POSITION_STANDING :
            send_to_char("You lie down and go to sleep.\n\r", ch);
            act("$n lies down and falls asleep.", ch, 0, 0, TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_SLEEPING;
            break;
        case POSITION_SITTING  :
        case POSITION_RESTING  :
            send_to_char("You lay back and go to sleep.\n\r", ch);
            act("$n lies back and falls asleep.", ch, 0, 0, TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_SLEEPING;
            break;
        case POSITION_SLEEPING : {
            send_to_char("You are already sound asleep.\n\r", ch);
            return eFAILURE; // so we don't set INTERNAL_SLEEPING
        } break;
        case POSITION_FIGHTING : {
            send_to_char("Sleep while fighting? Are you MAD?\n\r", ch);
            return eFAILURE; // so we don't set INTERNAL_SLEEPING
        } break;
        default : {
            act("You stop floating around, and lie down to sleep.", ch, 0, 0, TO_CHAR, 0);
            act("$n stops floating around, and lie down to sleep.", ch, 0, 0, TO_ROOM, INVIS_NULL);
            GET_POS(ch) = POSITION_SLEEPING;
        } break;
    }

    struct affected_type af;
    af.type      = INTERNAL_SLEEPING;
    af.duration  = 0;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(ch, &af);

    return eSUCCESS;
}
 
 
int do_wake(CHAR_DATA *ch, char *argument, int cmd)
{
    CHAR_DATA *tmp_char;
    char arg[MAX_STRING_LENGTH];
    struct affected_type * af;
    
    one_argument(argument,arg);
    if (*arg) {
        if (GET_POS(ch) == POSITION_SLEEPING) {
            act("You can't wake people up if you are asleep yourself!", ch,0,0,TO_CHAR, 0);
        } else {
            tmp_char = get_char_room_vis(ch, arg);
            if (tmp_char) {
                if (tmp_char == ch) {
                    act("If you want to wake yourself up, just type 'wake'", ch,0,0,TO_CHAR, 0);
                } else {
                    if (GET_POS(ch) == POSITION_FIGHTING) 
                    {
                        if (GET_POS(tmp_char) == POSITION_SLEEPING) {
                            if(number(1, 100) > GET_DEX(ch)) {
                                act("You cannot meneuver yourself over to $M!", ch, 0, tmp_char, TO_CHAR, 0);
                                act("$n tries to move the flow of battle towards $N but is unable.",
                                      ch, 0, tmp_char, TO_ROOM, 0);
                                return eSUCCESS;
                            }
                            if ((af = affected_by_spell(tmp_char, SPELL_SLEEP)) && af->modifier == 1) {  
                                act("You can not wake $M up!", ch, 0, tmp_char, TO_CHAR, 0);
                            } else {
                                act("You manage to give $M a swift kick in the ribs.", ch, 0, tmp_char, TO_CHAR, 0);
                                GET_POS(tmp_char) = POSITION_SITTING;
                                act("$n awakens $N.", ch, 0, tmp_char, TO_ROOM, NOTVICT);
                                act("$n wakes you up with a sharp kick to the ribs.  The sounds of battle ring in your ears.", ch, 0, tmp_char, TO_VICT, 0);
                                affect_from_char(tmp_char, INTERNAL_SLEEPING);
                            }
                        } else {
                            act("$N is already awake.", ch,0,tmp_char, TO_CHAR, 0);
                        }
                    }
                    else {
                        if (GET_POS(tmp_char) == POSITION_SLEEPING) {
                            if ((af = affected_by_spell(tmp_char, SPELL_SLEEP)) && af->modifier == 1) {  
                                act("You can not wake $M up!", ch, 0, tmp_char, TO_CHAR, 0);
                            } else {
                                act("You wake $M up.", ch, 0, tmp_char, TO_CHAR, 0);
                                act("$n awakens $N.", ch, 0, tmp_char, TO_ROOM, NOTVICT);
                                GET_POS(tmp_char) = POSITION_SITTING;
                                act("You are awakened by $n.", ch, 0, tmp_char, TO_VICT, 0);
                                affect_from_char(tmp_char, INTERNAL_SLEEPING);
                            }
                        } else {
                            act("$N is already awake.", ch,0,tmp_char, TO_CHAR, 0);
                        }
                    }
                }
            } else {
                send_to_char("You do not see that person here.\n\r", ch);
            }
        }
    } else {
        if (GET_POS(ch) > POSITION_SLEEPING)
           send_to_char("You are already awake...\n\r", ch);
        else if ((af = affected_by_spell(ch, SPELL_SLEEP)) && af->modifier == 1) {
            send_to_char("You can't wake up!\n\r", ch);
//        } else if ((af = affected_by_spell(ch, INTERNAL_SLEEPING))) {
//            send_to_char("You just went to sleep!  Your body is still too tired.  Your dreaming continues...\r\n", ch);
        } else {
//            else {
                send_to_char("You wake, and stand up.\n\r", ch);
                act("$n awakens.", ch, 0, 0, TO_ROOM, 0);
                GET_POS(ch) = POSITION_STANDING;
  //          }
        }
    }
    return eSUCCESS;
}
 
// global tag var
char_data * tagged_person;

int do_tag(CHAR_DATA *ch, char *argument, int cmd)
{
   char name[MAX_INPUT_LENGTH];
   char_data * victim;

   one_argument(name, argument);

   if(!*name || !(victim = get_char_room_vis(ch, name))) {
     send_to_char("Tag who?\r\n", ch);
     return eFAILURE;
   }

   return eSUCCESS;   
}




void CVoteData::DisplayVote(struct char_data *ch)
{
  char buf[MAX_STRING_LENGTH];
  std::vector<SVoteData>::iterator answer_it;
  int i = 1;
  if(vote_question.empty())
  {
    csendf(ch, "\n\rSorry! There are no active votes right now!\n\r");
    return;
  }
  csendf(ch, "\n\r--Current Vote Infortmation--\n\r");
  strncpy(buf, vote_question.c_str(), MAX_STRING_LENGTH);
  csendf(ch, buf);
  csendf(ch, "\n\r");
  for(answer_it = answers.begin(); answer_it != answers.end(); answer_it++)
    csendf(ch, "%2d: %s\n\r", i++, answer_it->answer.c_str());
  csendf(ch, "\n\r");
}

void CVoteData::RemoveAnswer(int answer)
{
  if(active)
    return;
  std::vector<SVoteData>::iterator answer_it = answers.begin();
  answers.erase(answer_it+answer-1);//need to offset by 1
}

bool CVoteData::StartVote()
{
  if(active)
    return false;
  if(vote_question.empty())
    return false;
  if(answers.empty())
    return false;

  active = true;
  return true;
}

bool CVoteData::EndVote()
{
  if(!active)
   return false;

  active = false;
  return true;
}

void CVoteData::AddAnswer(std::string answer)
{
  if(active)
    return;
  SVoteData tmp;
  tmp.votes = 0;
  tmp.answer = answer;
  answers.push_back(tmp);
}

bool CVoteData::Vote(struct char_data *ch, int vote)
{
  if(!ch->desc)
  {
    send_to_char("Monsters don't get to vote!", ch);
    return false;
  }

  if(true == ip_voted[ch->desc->host])
  {
    send_to_char("You have already voted!", ch);
    return false;
  }

  if(vote < 1 || (unsigned int)vote > answers.size())
  {
    send_to_char("That answer doesn't exist.", ch);
    return false;
  }

  ip_voted[ch->desc->host] = true;
  total_votes++;
  answers.at(vote-1).votes++;

  send_to_char("Vote sent!", ch);
  return true;
  
}

void CVoteData::DisplayResults(struct char_data *ch)
{
  if(active && !ip_voted[ch->desc->host] && GET_LEVEL(ch) < IMMORTAL)
  {
    send_to_char("Sorry, but you have to cast a vote before you can see the results.\n\r", ch);
    return;
  }
  if(!total_votes)
  {
    send_to_char("There hasn't been any votes to view the results of.", ch);
    return;
  }
  char buf[MAX_STRING_LENGTH];
  std::vector<SVoteData>::iterator answer_it;
  csendf(ch, "--Current Vote Results--\n\r");
  int percent; 
  strncpy(buf, vote_question.c_str(), MAX_STRING_LENGTH);
  csendf(ch, buf);
  csendf(ch, "\n\r");
  for(answer_it = answers.begin(); answer_it != answers.end(); answer_it++)
  {
    if(GET_LEVEL(ch) < IMMORTAL)
    {
      percent = (answer_it->votes * 100) / total_votes ;
      csendf(ch, "%3d\%: %s\n\r", percent, answer_it->answer.c_str());
    }
    else
      csendf(ch, "%3d: %s\n\r", answer_it->votes, answer_it->answer.c_str());
  }
  csendf(ch, "\n\r");
  
}

void CVoteData::Reset()
{
  if(active)
    return;

  total_votes = 0;
  vote_question.clear();
  answers.clear();
}

void CVoteData::SetQuestion(std::string question)
{
  if(active)
    return;
  vote_question = question;
}

CVoteData::CVoteData()
{
  active = false;
  total_votes = 0;
}

CVoteData::~CVoteData()
{}
 
CVoteData DCVote;

int do_vote(struct char_data *ch, char *arg, int cmd)
{
  extern CVoteData DCVote;
  char buf[MAX_STRING_LENGTH];
  int vote;
  arg = one_argument(arg, buf);
 
  if(!strcmp(buf, "results"))
  {
    DCVote.DisplayResults(ch);
    return eSUCCESS;
  }

  if(!DCVote.IsActive())
  {
    send_to_char("Sorry, there is nothing to vote on right now.", ch);
    return eSUCCESS;
  }
  if(!strlen(buf))
  {
    DCVote.DisplayVote(ch);
    return eSUCCESS;  
  }


  vote = atoi(buf);
  DCVote.Vote(ch, vote);

  return eSUCCESS;


}

