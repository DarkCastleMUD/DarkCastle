/************************************************************************
| $Id: wizard.cpp,v 1.59 2008/03/07 08:50:01 dcastle Exp $
| wizard.C
| Description:  Utility functions necessary for wiz commands.
*/
#include "wizard.h"
#include <character.h>
#include <utility.h>
#include <levels.h>
#include <db.h>
#include <room.h>
#include <player.h>
#include <obj.h>
#include <mobile.h>
#include <handler.h>
#include <connect.h>
#include <spells.h>
#include <interp.h>
#include <returnvals.h>

extern struct obj_data * search_char_for_item(char_data * ch, int16 item_number, bool wearonly = FALSE);
extern void debug_point();

int number_or_name(char **name, int *num)
{
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];
  
  if((ppos = index(*name, '.')) != NULL) {
    *ppos++ = '\0'; 
    strcpy(number, *name);
    strcpy(*name, ppos);
       
    for(i = 0; *(number + i); i++)
       if(!isdigit(*(number + i)))
         return(0);

    return(atoi(number));
  }

  /* no dot */
  if((*num = atoi(*name)) > 0)
    return -1;
  else
    return 1;
}

#if(0)
int number_or_name(char **name, int *num)
{
  unsigned i;
  char *ppos = NULL;
  char number[MAX_INPUT_LENGTH];
  
  for(i = 0; i < strlen(*name); i++)
  {
	  if(*name[i] == '.')
	  {
		  ppos = *name + i;
		  break;
	  }
  }
  if(ppos)
  {
    *ppos++ = '\0'; 
    strcpy(number, *name);
    strcpy(*name, ppos);
       
    for(i = 0; *(number + i); i++)
       if(!isdigit(*(number + i)))
         return(0);

    return(atoi(number));
  }

  /* no dot */
  if((*num = atoi(*name)) > 0)
    return -1;
  else
    return 1;
}
#endif

void do_mload(struct char_data *ch, int rnum, int cnt)
{      
  struct char_data *mob;
  char buf[100];
  int i; 
 
  if (cnt == 0) cnt = 1;
  for (i=1; i<=cnt; i++) {
    mob = clone_mobile(rnum);
    char_to_room(mob, ch->in_room);
  }
  
  act("$n draws up a swirling column of dust and breathes life into it.",
      ch, 0, 0, TO_ROOM, 0);
  act("$n has created $N!", ch, 0, mob, TO_ROOM, 0);
  sprintf(buf,"You create %i %s!\n\r",cnt, mob->short_desc);
  send_to_char(buf, ch);
  if (cnt > 1) {
    snprintf(buf, MAX_STRING_LENGTH, "%s loads %i copies of mob %d (%s) at room %d (%s).",
	     GET_NAME(ch),
	     cnt,
	     mob_index[rnum].virt,
	     mob->short_desc,
	     world[ch->in_room].number,
	     world[ch->in_room].name);
  } else {
    snprintf(buf, MAX_STRING_LENGTH, "%s loads %i copy of mob %d (%s) at room %d (%s).",
	     GET_NAME(ch),
	     cnt,
	     mob_index[rnum].virt,
	     mob->short_desc,
	     world[ch->in_room].number,
	     world[ch->in_room].name);
    
  }
  log(buf, GET_LEVEL(ch), LOG_GOD);
}    


void do_oload(struct char_data *ch, int rnum, int cnt)
{    
  struct obj_data *obj;
  char buf[MAX_STRING_LENGTH];
  int i;
     
  if (cnt == 0) cnt = 1;
     
  for (i=1; i<= cnt; i++) {
    obj = clone_object(rnum);
    if ( (obj->obj_flags.type_flag == ITEM_MONEY) &&
         (GET_LEVEL(ch) < OVERSEER)) {
       extract_obj(obj);
       send_to_char("Denied.\n\r", ch);
       return;
       }
    else 
       obj_to_char(obj, ch);
  }  
  act("$n makes a strange magical gesture.", ch, 0, 0, TO_ROOM, INVIS_NULL);
  act("$n has created $p!", ch, obj, 0, TO_ROOM, 0);
  snprintf(buf, MAX_STRING_LENGTH, "You create %i %s.\n\r", cnt, obj->short_description);
  send_to_char(buf, ch);
  if (cnt > 1) {
    snprintf(buf, MAX_STRING_LENGTH, "%s loads %i copies of obj %d (%s) at room %d (%s).",
	     GET_NAME(ch),
	     cnt,
	     GET_OBJ_VNUM(obj),
	     obj->short_description,
	     world[ch->in_room].number,
	     world[ch->in_room].name);
  } else {
    snprintf(buf, MAX_STRING_LENGTH, "%s loads %i copy of obj %d (%s) at room %d (%s).",
	     GET_NAME(ch),
	     cnt,
	     GET_OBJ_VNUM(obj),
	     obj->short_description,
	     world[ch->in_room].number,
	     world[ch->in_room].name);
  }
  log(buf, GET_LEVEL(ch), LOG_GOD);
}    

void string_to_file(FILE *f, char *string)
{    
  char * newbuf = new char[strlen(string)+1];
  strcpy(newbuf, string);

  // remove all \r's
  for(char * curr = newbuf; *curr != '\0'; curr++)
  {
    if(*curr == '\r') {
      for(char * blah = curr; *blah != '\0'; blah++) // shift the rest of the string 1 left
        *blah = *(blah + 1);
      curr--; // (to check for \r\r cases)
    }
  }
   
  fprintf(f, "%s~\n", newbuf);
  delete [] newbuf;

/*  the above uses more CPU time, but it MUCH nicer to our HD since it
    only does 1 write instead of strlen(string) writes.  And since memory
    operations are faster than file writes, it's actually faster too:)
    -pir 12/11/01

  while(*string) {
    if(*string != '\r')
      fputc(*string, f);
    string++;
  }  
*/
} 

//
// (I let him email it to me *wink* -pir)
//
/* Mob stat takes ch (the person who wants to stat the mob) and sends them info about
 * k, the mob or player that is being stated.  It prints it in a format similar to score.
 *  -- Borodin  Dec 23 2001 */

void boro_mob_stat(struct char_data *ch, struct char_data *k)
{
	int i, i2;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	char buf3[MAX_STRING_LENGTH];
	struct follow_type *fol;
	struct obj_data  *j=0;
	struct affected_type *aff;

	extern char *skills[];
	extern char *spells[];
	// for chars
	extern char *affected_bits[];
	extern char *apply_types[];
	extern char *pc_clss_types[];
	// extern char *punish_bits[];
	extern char *action_bits[];
	//extern char *player_bits[];
	extern char *combat_bits[];
	extern char *position_types[];
	extern char *connected_types[];
	extern race_shit race_info[];
	extern char *isr_bits[];

	sprinttype(k->c_class, pc_clss_types, buf2);
	sprintf(buf,
	"$R(:)========================================================================(:)\r\n"
	"|=|$3 (%3s) Key$R: %-35s $3VNUM$R: %-5d $3Room$R: %-5d |=|\r\n"
	"|=|$3 Short$R: %-50s $3RNUM$R: %-6ld |=|\r\n"
	"|=|$3 Long$R: %s"
	"(:)====================(:)=================================================(:)\r\n"
	"|\\|  $4Fighting$R: %-9s|/|  $1Race$R:   %-10s $1HitPts$R: %5d$1/$R(%5d+%-3d) |~|\r\n"
	"|~|  $4Master$R:   %-9s|o|  $1Class$R:  %-10s $1Mana$R:   %5d$1/$R(%5d+%-3d) |\\|\r\n",

	(!IS_NPC(k) ? "PC" : "MOB"),
	GET_NAME(k), 
	(IS_NPC(k) ? mob_index[k->mobdata->nr].virt : 0),
	(k->in_room == NOWHERE ? -1 : world[k->in_room].number),
	/* end of first line */
	
	(k->short_desc ? k->short_desc : "None"),
	(long)(IS_NPC(k) ? k->mobdata->nr : 0),
	/* end of second line */
	
	(k->long_desc ? k->long_desc : "None"),
	/* end of third line */
	
	(k->fighting ? GET_NAME(k->fighting) : "Nobody"),
	(race_info[(int)(GET_RACE(k))].singular_name),
	GET_HIT(k), hit_limit(k), hit_gain(k),
	/* end of fourth line */

	(k->master ? GET_NAME(k->master) : "Nobody"),
	buf2, /* this is for the class string, the sprinttype above inits it. */
	GET_MANA(k),mana_limit(k),mana_gain(k));
	/* end of the first sprintf */

	send_to_char(buf,ch); // this sends to char, now we can overwrite buf


	if(IS_MOB(k)) {
		sprintf(buf2,"%s",(k->mobdata->hatred ? k->mobdata->hatred : "NOBODY"));
		sprintf(buf3,"%s",(k->mobdata->fears ? k->mobdata->fears : "NOBODY"));
	}
	else
	{
		sprintf(buf2,"Nobody");
		sprintf(buf3,"Nobody");
	}

	sprintf(buf,
	"|/|  $4Hates$R:    %-9s|\\|  $1Lvl$R:    %-9d  $1Fatigue$R:%5d$1/$R(%5d+%-3d) |o|\r\n"
	"|o|  $4Fears$R:    %-9s|~|  $1Height$R: %-9d  $1Ki$R:     %5d$1/$R(%5d)     |/|\r\n"
	"|\\|  $4Tracking$R: %-9s|/|  $1Weight$R: %-9d  $1Alignment$R:   %4d          |~|\r\n",

	buf2, // who the mob hates
	k->level,
	GET_MOVE(k),move_limit(k),move_gain(k,0),
	/* end of first line */
	
	buf3, // who the mob fears, if anyone
	GET_HEIGHT(k),
	GET_KI(k), ki_limit(k),
	/* end of second line */
	
	(k->hunting ? k->hunting : "NOBODY"),
	GET_WEIGHT(k),
	k->alignment);

	send_to_char(buf,ch);
		/* end of second sprintf */
		
	switch(k->sex) {
		case SEX_NEUTRAL: sprintf(buf2,"Neutral"); break;
		case SEX_MALE   : sprintf(buf2,"Male");    break;
		case SEX_FEMALE : sprintf(buf2,"Female");  break;
		default : sprintf(buf2,"ILLEGAL-VALUE");   break;
	}
	
	sprintf(buf,
		"|~|  $2Timer$R: %-11d |o|  $1Town$R:   %-5d     $1Sex$R:         %-14s|\\|\r\n"
		"(:)====================(:)==========(:)====================================(:)\r\n",
		k->timer,
		(!IS_NPC(ch) ? k->hometown : -1),
		buf2); /* buf is the sex... */
	send_to_char(buf,ch); /* THIRD sprintf */

	if(IS_MOB(k))
	{
		if(mob_index[k->mobdata->nr].non_combat_func)
			strcpy(buf2,"Exists");
		else
			strcpy(buf2,"None");
		
		if(mob_index[k->mobdata->nr].combat_func)
			strcpy(buf3,"Exists");
		else
			strcpy(buf3,"None");
	}

	sprintf(buf,
	"|/|  $4FIRE$R: %3d  $1COLD$R: %3d $5NRGY$R: %3d |\\| Mob Non-Combat Spec Proc: %-9s|o|\r\n"
	"|o|  $2$BACID$R: %3d  $3MAGK$R: %3d $2POIS$R: %3d |~| Mob Combat Spec Proc:     %-9s|/|\r\n",
	
	k->saves[SAVE_TYPE_FIRE], k->saves[SAVE_TYPE_COLD], k->saves[SAVE_TYPE_ENERGY],
	buf2, /* whether nor not they have a non_combat spec proc */
	/* end of first line */

	k->saves[SAVE_TYPE_ACID], k->saves[SAVE_TYPE_MAGIC], k->saves[SAVE_TYPE_POISON],
	buf3); /* buf2 = whether or not there is a combat spec proc */
	/* end of fourth sprintf */
	send_to_char(buf,ch);


	sprinttype(GET_POS(k), position_types, buf2);
	if(IS_NPC(k))
		sprinttype((k->mobdata->default_pos),position_types,buf3);
	else
		strcpy(buf3,"PC");

	for(i = 0, j = k->carrying; j; j = j->next_content, ++i);
	sprintf(buf,
	"|\\|  $3Pos$R: %-9s$3DefPos$R: %-9s|/| $1Thirst$R: %-3d $4Hunger$R: %-3d $6Drunk$R: %-3d |~|\r\n"
	"|/|  $3Dex$R: %2d    $3Con$R: %2d    $3Str$R: %2d  |\\| $3Carried Weight$R: %-4d $3Inv. Items$R: %-3d|o|\r\n",

	buf2, buf3, // pos and default pos respectively
	k->conditions[THIRST],
	k->conditions[FULL],
	k->conditions[DRUNK],
	// end of first line

	GET_DEX(k), GET_CON(k), GET_STR(k),
	IS_CARRYING_W(k), i);
	/* END OF SECOND LINE */

	send_to_char(buf,ch);

	for(i = 0, i2 = 0; i < MAX_WEAR; i++)
     if(k->equipment[i])
       i2++;

	sprintf(buf,
	"|~|  $3Int$R: %2d    $3Wis$R: %2d    $3Ac$R: %-3d |o| $3Carried Items$R: %d $3Equipped Items$R: %d |/|\r\n",
	GET_INT(k), GET_WIS(k), GET_ARMOR(k), IS_CARRYING_N(k), i2);
	send_to_char(buf,ch);

	sprintf(buf,
	"|o| $3 Hitroll$R: %3d     $3Damroll$R: %3d  |~|                                    |/|\r\n"
	"(:)=================================(:)====================================(:)\r\n",
	k->hitroll, k->damroll);
	send_to_char(buf,ch);


	sprintbit(k->suscept, isr_bits, buf2);
	sprintbit(k->immune, isr_bits, buf3);
	sprintf(buf,
		"|/| $7Immune$R: %-63s|o|\r\n"
		"|o| $7Susceptible$R: %-58s|/|\r\n",
		buf2, buf3); // immune and susceptible bits, first and second.
	send_to_char(buf,ch);
	sprintbit(k->affected_by, affected_bits, buf2);
	sprintf(buf,
		"|\\| $7Affected By$R: %-58s|~|\r\n", buf2); // affected bits.
	send_to_char(buf,ch);

	if(IS_MOB(k)) // AND THIS
		sprintbit(k->mobdata->actflags, action_bits,buf2);
	else
		strcpy(buf2,"Not a mob");
	sprintbit(k->combat, combat_bits, buf3);
	sprintf(buf,
		"|~| $7NPC flags$R: %-60s|\\|\r\n"
		"|/| $7Combat flags$R: %-57s|o|\r\n",
		buf2,buf3); // npc flags and combat flags respectively
	send_to_char(buf,ch);

	sprintbit(k->resist, isr_bits, buf2);
	sprintf(buf,
		"|o| $7Resistant$R: %-60s|/|\r\n"
		"(:)========================================================================(:)\r\n",
		buf2); // the resisted bits
	send_to_char(buf,ch);

	strcpy(buf,"$3Title$R: ");
	strcat(buf, (k->title ? k->title : "None"));
	strcat(buf,"\n\r");
	send_to_char(buf,ch);
	
	// Description
	send_to_char("$3Detailed description$R:\r\n", ch);
	if(k->description)
		send_to_char(k->description, ch);
	else
		send_to_char("None", ch);

	// LIST OF FOLLOWERS
	send_to_char("$3Followers$R:\n\r", ch);
	for(fol = k->followers; fol; fol = fol->next)
		act("    $N", ch, 0, fol->follower, TO_CHAR, 0);
	
	if(!IS_MOB(k)) {
		sprintf(buf, "$3Birth$R: [%ld]secs  $3Logon$R:[%ld]secs $3Played$R[%ld]secs\n\r",
			k->pcdata->time.birth,
			k->pcdata->time.logon,
			(long)(k->pcdata->time.played));
		send_to_char(buf, ch);

		sprintf(buf, "$3Age$R:[%d] Years [%d] Months [%d] Days [%d] Hours\n\r",
			age(k).year, age(k).month, age(k).day, age(k).hours);
		send_to_char(buf,ch);
	}

	if(!IS_MOB(k)) {
		sprintf(buf, "$3Coins$R:[%lld]  $3Bank$R:[%d]\n\r", GET_GOLD(k),
		k->pcdata->bank );
		send_to_char(buf, ch);
	}

	if(!IS_NPC(k)) {
		sprintf(buf,"$3SaveMod$R: FIRE[%d] COLD[%d] ENERGY[%d] ACID[%d] MAGIC[%d] POISON[%d]\n\r",
			k->pcdata->saves_mods[SAVE_TYPE_FIRE],
			k->pcdata->saves_mods[SAVE_TYPE_COLD],
			k->pcdata->saves_mods[SAVE_TYPE_ENERGY],
			k->pcdata->saves_mods[SAVE_TYPE_ACID],
			k->pcdata->saves_mods[SAVE_TYPE_MAGIC],
			k->pcdata->saves_mods[SAVE_TYPE_POISON]);
		send_to_char(buf, ch);
	}

	if(!IS_MOB(k)) {
		sprintf(buf, "$3WizInvis$R:  %ld  ", k->pcdata->wizinvis);
	  send_to_char(buf, ch);
    sprintf(buf, "$3Holylite$R:  %s  ", ((k->pcdata->holyLite) ? "ON" :
      "OFF"));
    send_to_char(buf, ch);
    sprintf(buf, "$3Stealth$R:  %s\n\r", ((k->pcdata->stealth) ? "ON"  :
      "OFF"));
    send_to_char(buf, ch);

    if (k->pcdata->buildLowVnum == k->pcdata->buildMLowVnum == k->pcdata->buildOLowVnum  &&
	k->pcdata->buildHighVnum == k->pcdata->buildMHighVnum == k->pcdata->buildOHighVnum)
   {
    sprintf(buf, "$3Creation Range$R:  %d-%d  \r\n", k->pcdata->buildLowVnum, k->pcdata->buildHighVnum);
    send_to_char(buf,ch);
   } else {
    sprintf(buf, "$3R Range$R:  %d-%d  \r\n", k->pcdata->buildLowVnum, k->pcdata->buildHighVnum);
    send_to_char(buf,ch);
    sprintf(buf, "$3M Range$R:  %d-%d  \r\n", k->pcdata->buildMLowVnum, k->pcdata->buildMHighVnum);
    send_to_char(buf,ch);
    sprintf(buf, "$3O Range$R:  %d-%d  \r\n", k->pcdata->buildOLowVnum, k->pcdata->buildOHighVnum);
    send_to_char(buf,ch);
   }
  }

  if(k->affected) 
  {
    send_to_char("\n\r$3Affecting Spells$R:\n\r--------------\n\r", ch);
    
    for(aff = k->affected; aff; aff = aff->next) 
    {
      sprintf(buf, "Spell : '%s'\n\r",
        aff->type < 300 ? spells[(int)aff->type-1] :
        skills[(int)aff->type-300]);
      send_to_char(buf, ch);
      sprintf(buf,"     Modifies %s by %d points\n\r",
        apply_types[(int) aff->location], aff->modifier);
      send_to_char(buf, ch);
      sprintf(buf,"     Expires in %3d hours, Bits set ", aff->duration);
      send_to_char(buf, ch);
      sprintbit(aff->bitvector,affected_bits,buf);
      strcat(buf,"\n\r");
      send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);
  }

  if(!IS_MOB(k))
    display_punishes(ch, k);

  if(k->desc) {
    sprinttype(k->desc->connected,connected_types,buf2);
    strcat(buf,"  $3Connected$R: ");
    strcat(buf,buf2);
  }
  send_to_char(buf, ch);
}

void mob_stat(struct char_data *ch, struct char_data *k)
{   

  int i;
  char buf[MAX_STRING_LENGTH];
  struct follow_type *fol;
  int i2;
  char buf2[MAX_STRING_LENGTH];
  struct obj_data  *j=0;
  //extern char *skills[];
 // extern char *spells[];
 // extern char *ki[];
 // extern char *songs[];
  struct affected_type *aff;  

  // for chars
  extern char *affected_bits[];
  extern char *apply_types[];
  extern char *pc_clss_types[];
  // extern char *punish_bits[];
  extern char *action_bits[];
  extern char *player_bits[];
  extern char *combat_bits[];
  extern char *position_types[];
  extern char *connected_types[];
  extern race_shit race_info[];
  extern char *isr_bits[];
  
  sprintf(buf, "$3%s$R - $3Name$R: %s  $3VNum$R: %d  $3RNum$R: %d  $3In room:$R %d\n\r",
          (!IS_NPC(k) ? "PC" : "MOB"),
          GET_NAME(k),
          (IS_NPC(k) ?  mob_index[k->mobdata->nr].virt : 0),
          (IS_NPC(k) ? k->mobdata->nr : 0), k->in_room == NOWHERE ? -1 : world[k->in_room].number);
  send_to_char(buf, ch);

  strcpy(buf,"$3Short description$R: ");
  strcat(buf, (k->short_desc ? k->short_desc : "None"));
  strcat(buf,"\n\r");
  send_to_char(buf,ch);
     
  strcpy(buf,"$3Title$R: ");
  strcat(buf, (k->title ? k->title : "None"));
  strcat(buf,"\n\r");
  send_to_char(buf,ch);
     
  send_to_char("$3Long description$R: ", ch);
  if(k->long_desc)
    send_to_char(k->long_desc, ch);
  else
    send_to_char("None", ch);
  send_to_char("$3Detailed description$R:\r\n", ch);
  if(k->description)
    send_to_char(k->description, ch);
  else
    send_to_char("None", ch);

  strcpy(buf,"\r\n$3Class$R: ");
  sprinttype(k->c_class, pc_clss_types, buf2);

  strcat(buf, buf2);
     
  sprintf(buf2, "   $3Level$R:[%d] $3Alignment$R:[%d] ", k->level,
          k->alignment);
  strcat(buf, buf2);
  send_to_char(buf, ch);
  sprintf(buf, "$3Spelldamage$R:[%d] ", k->spelldamage);
  send_to_char(buf,ch);
  sprintf(buf,"$3Race$R: %s\r\n", race_info[(int)(GET_RACE(k))].singular_name);
  send_to_char(buf, ch);

  if(!IS_MOB(k)) {     
    sprintf(buf, "$3Birth$R: [%ld]secs  $3Logon$R:[%ld]secs  $3Played$R[%ld]secs\n\r",
          k->pcdata->time.birth,
          k->pcdata->time.logon,
          (long)(k->pcdata->time.played));
    send_to_char(buf, ch);

    sprintf(buf, "$3Age$R:[%d] Years [%d] Months [%d] Days [%d] Hours\n\r",
          age(k).year, age(k).month, age(k).day, age(k).hours);
    send_to_char(buf,ch);
  }
  if (IS_NPC(k))
  {
    sprintf(buf, "$3Mobspec$R: %d  $3Progtypes$R: %d\r\n", (int)(mob_index[k->mobdata->nr].mobspec), mob_index[k->mobdata->nr].progtypes);
    send_to_char(buf,ch);
  }
 sprintf(buf, "$3Height$R:[%d]  $3Weight$R:[%d]  $3Sex$R:[", GET_HEIGHT(k), GET_WEIGHT(k));
  send_to_char(buf,ch);
     
  switch(k->sex) {
    case SEX_NEUTRAL :
      send_to_char("NEUTRAL]  ", ch);
      break;
    case SEX_MALE :
      send_to_char("MALE]  ", ch);
      break;
    case SEX_FEMALE :
      send_to_char("FEMALE]  ", ch);
      break;
    default :
      send_to_char("ILLEGAL-VALUE!]  ", ch);
      break;
  }  

  if(!IS_NPC(ch)) {
    sprintf(buf, "$3Hometown$R:[%d]\n\r", k->hometown);
    send_to_char(buf, ch);
  } else send_to_char("\n\r", ch);
   
  sprintf(buf, "$3Str$R:[%2d]+[%2d]=%2d $3Int$R:[%2d]+[%2d]=%2d $3Wis$R:[%2d]+[%2d]=%2d\r\n"
               "$3Dex$R:[%2d]+[%2d]=%2d $3Con$R:[%2d]+[%2d]=%2d\n\r",
          GET_RAW_STR(k), GET_STR_BONUS(k), GET_STR(k),
          GET_RAW_INT(k), GET_INT_BONUS(k), GET_INT(k),
          GET_RAW_WIS(k), GET_WIS_BONUS(k), GET_WIS(k),
          GET_RAW_DEX(k), GET_DEX_BONUS(k), GET_DEX(k),
          GET_RAW_CON(k), GET_CON_BONUS(k), GET_CON(k) );
  send_to_char(buf,ch);
     
  sprintf(buf, "$3Mana$R:[%5d/%5d+%-4d]  $3Hit$R:[%5d/%5d+%-3d]  $3Move$R:[%5d/%5d+%-3d]  $3Ki$R:[%3d/%3d]\n\r",
          GET_MANA(k),mana_limit(k),mana_gain(k),
          GET_HIT(k),hit_limit(k),hit_gain(k),
          GET_MOVE(k),move_limit(k),move_gain(k,0),
          GET_KI(k), ki_limit(k));
  send_to_char(buf,ch);
     
  sprintf(buf, "$3AC$R:[%d]  $3Exp$R:[%lld]  $3Hitroll$R:[%d]  $3Damroll$R:[%d]  $3Gold$R: [$B$5%lld$R]\n\r",
          GET_ARMOR(k), GET_EXP(k), GET_REAL_HITROLL(k), GET_REAL_DAMROLL(k), GET_GOLD(k) );
  send_to_char(buf, ch);

  if(!IS_MOB(k)) {
    sprintf(buf, "$3Plats$R:[%d]  $3Bank$R:[%d]  $3Clan$R:[%d]  $3Quest Points$R:[%d]\n\r", 
          GET_PLATINUM(k), GET_BANK(k), GET_CLAN(k), GET_QPOINTS(k) );
    send_to_char(buf, ch);
  }
   
  sprinttype(GET_POS(k), position_types, buf2);
  sprintf(buf,"$3Position$R: %s  $3Fighting$R: %s  ",buf2,
          ((k->fighting) ? GET_NAME(k->fighting)
          : "Nobody") );
  if(k->desc) {
    sprinttype(k->desc->connected,connected_types,buf2);
    strcat(buf,"  $3Connected$R: ");
    strcat(buf,buf2);
  }  
  send_to_char(buf, ch);
     
  if(IS_NPC(k)) {
    strcpy(buf,"$3Default position$R: ");
    sprinttype((k->mobdata->default_pos),position_types,buf2);
    strcat(buf, buf2);
    send_to_char(buf, ch);
  }

  sprintf(buf,"  $3Timer$R:[%d] \n\r", k->timer);
  send_to_char(buf, ch);

  if(IS_NPC(k)) {
    sprintf(buf, "$3NPC flags$R: [%d %d]", k->mobdata->actflags[0], k->mobdata->actflags[1]);
    sprintbit(k->mobdata->actflags, action_bits,buf2);
  }  
  else {
    sprintf(buf, "$3PC flags$R: [%d]", k->pcdata->toggles);
    sprintbit(k->pcdata->toggles, player_bits,buf2);
  }  
  strcat(buf, buf2);
  send_to_char(buf, ch);
     
  if(IS_MOB(k)) {
    strcpy(buf, "\n\r$3Non-Combat Special Proc$R: ");
    strcat(buf, (mob_index[k->mobdata->nr].non_combat_func ? "Exists  " : "None  "));
    send_to_char(buf, ch);
    strcpy(buf, "$3Combat Special Proc$R: ");
    strcat(buf, (mob_index[k->mobdata->nr].combat_func ? "Exists  " : "None  "));
    send_to_char(buf, ch);
    strcpy(buf, "$3Mob Progs$R: ");
    strcat(buf, (mob_index[k->mobdata->nr].mobprogs ? "Exist\r\n" : "None\r\n"));
    send_to_char(buf, ch);
  } 
  
  if(IS_NPC(k)) {
    sprintf(buf, "$3NPC Bare Hand Damage$R: %d$3d$R%d.\n\r",
            k->mobdata->damnodice, k->mobdata->damsizedice);
    send_to_char(buf, ch);
  }  
  
  sprintf(buf,"$3Carried weight$R: %d   $3Carried items$R: %d\n\r",
          IS_CARRYING_W(k), IS_CARRYING_N(k) );
  send_to_char(buf,ch);
     
  for(i = 0, j = k->carrying; j; j = j->next_content, i++)
    ;
  sprintf(buf, "$3Items in inventory$R: %d  ", i);
     
  for(i = 0, i2 = 0; i < MAX_WEAR; i++)
     if(k->equipment[i])
       i2++;
     
  sprintf(buf2,"$3Items in equipment$R: %d\n\r", i2);
  strcat(buf,buf2);
  send_to_char(buf, ch);
     
  sprintf(buf,"$3Save Vs$R: $B$4FIRE[%2d] $7COLD[%2d] $5ENERGY[%2d] $2ACID[%2d] $3MAGIC[%2d] $R$2POISON[%2d]$R\n\r",
          k->saves[SAVE_TYPE_FIRE],
          k->saves[SAVE_TYPE_COLD],
          k->saves[SAVE_TYPE_ENERGY],
          k->saves[SAVE_TYPE_ACID],
          k->saves[SAVE_TYPE_MAGIC],
          k->saves[SAVE_TYPE_POISON]);
  send_to_char(buf, ch);

  if(!IS_NPC(k)) {
    sprintf(buf,"$3SaveMod$R: $B$4FIRE[%2d] $7COLD[%2d] $5ENERGY[%2d] $2ACID[%2d] $3MAGIC[%2d] $R$2POISON[%2d]$R\n\r",
          k->pcdata->saves_mods[SAVE_TYPE_FIRE],
          k->pcdata->saves_mods[SAVE_TYPE_COLD],
          k->pcdata->saves_mods[SAVE_TYPE_ENERGY],
          k->pcdata->saves_mods[SAVE_TYPE_ACID],
          k->pcdata->saves_mods[SAVE_TYPE_MAGIC],
          k->pcdata->saves_mods[SAVE_TYPE_POISON]);
    send_to_char(buf, ch);
  }
     
  sprintf(buf, "$3Thirst$R: %d  $3Hunger$R: %d  $3Drunk$R: %d\n\r",
          k->conditions[THIRST],
          k->conditions[FULL],
          k->conditions[DRUNK]);
  send_to_char(buf, ch);
	sprintf(buf, "$3Melee$R: [%d] $3Spell$R: [%d] $3Song$R: [%d] $3Reflect$R: [%d]\r\n", 
		k->melee_mitigation, k->spell_mitigation, k->song_mitigation, k->spell_reflect);
	send_to_char(buf,ch);

  sprintf(buf, "$3Tracking$R: '%s'\n\r", ((k->hunting) ? k->hunting : "NOBODY"));
  send_to_char(buf, ch);

  if(IS_MOB(k)) {     
    sprintf(buf, "$3Hates$R: '%s'\n\r",
          ((k->mobdata->hatred) ? k->mobdata->hatred : "NOBODY"));
    send_to_char(buf, ch);
     
    sprintf(buf, "$3Fears$R: '%s'\n\r",
          ((k->mobdata->fears) ? k->mobdata->fears : "NOBODY"));
    send_to_char(buf, ch);
  }
 
  sprintf(buf, "$3Master$R: '%s'\n\r",
          ((k->master) ? GET_NAME(k->master) : "NOBODY"));
  send_to_char(buf, ch);
  send_to_char("$3Followers$R:\n\r", ch);
  for(fol = k->followers; fol; fol = fol->next)
     act("    $N", ch, 0, fol->follower, TO_CHAR, 0);
     
  // Showing the bitvector
  sprintbit(k->combat, combat_bits, buf);
  csendf(ch, "$3Combat flags$R: %s\n\r", buf);
  
  if(!IS_MOB(k))
     display_punishes(ch, k);
    
  sprintbit(k->affected_by, affected_bits, buf);
  csendf(ch, "$3Affected by$R: [%d %d] %s\r\n", k->affected_by[0], k->affected_by[1], buf);

  sprintbit(k->immune, isr_bits, buf);
  csendf(ch, "$3Immune$R: [%d] %s\n\r", k->immune, buf);
     
  sprintbit(k->suscept, isr_bits, buf);
  csendf(ch, "$3Susceptible$R: [%d] %s\n\r", k->suscept,buf);
     
  sprintbit(k->resist, isr_bits, buf);
  csendf(ch, "$3Resistant$R: [%d] %s\n\r", k->resist, buf);
     
  if(!IS_MOB(k)) {
    sprintf(buf, "$3WizInvis$R:  %ld  ", k->pcdata->wizinvis);
    send_to_char(buf, ch);
    sprintf(buf, "$3Holylite$R:  %s  ", ((k->pcdata->holyLite) ? "ON" : "OFF"));
    send_to_char(buf, ch);
    sprintf(buf, "$3Stealth$R:  %s\n\r", ((k->pcdata->stealth) ? "ON"  : "OFF"));
    send_to_char(buf, ch);
    if (k->pcdata->buildLowVnum == k->pcdata->buildMLowVnum == k->pcdata->buildOLowVnum  &&
        k->pcdata->buildHighVnum == k->pcdata->buildMHighVnum == k->pcdata->buildOHighVnum)
   {
    sprintf(buf, "$3Creation Range$R:  %d-%d  \r\n", k->pcdata->buildLowVnum, k->pcdata->buildHighVnum);
    send_to_char(buf,ch);
   } else {
    sprintf(buf, "$3R Range$R:  %d-%d  ", k->pcdata->buildLowVnum, k->pcdata->buildHighVnum);
    send_to_char(buf,ch);
    sprintf(buf, "$3M Range$R:  %d-%d  ", k->pcdata->buildMLowVnum, k->pcdata->buildMHighVnum);
    send_to_char(buf,ch);
    sprintf(buf, "$3O Range$R:  %d-%d  \r\n", k->pcdata->buildOLowVnum, k->pcdata->buildOHighVnum);
    send_to_char(buf,ch);
   }
  }  

  csendf(ch, "$3Lag Left$R:  %d\r\n", (GET_WAIT(k) ? GET_WAIT(k) : 0));


  if (!IS_NPC(k)) {
     csendf(ch, "$3Hp metas$R: %d, $3Mana metas$R: %d, $3Move metas$R: %d, $3Ki metas$R: %d\r\n",GET_HP_METAS(k), GET_MANA_METAS(k), GET_MOVE_METAS(k), GET_KI_METAS(k));
  }

  if(k->affected) {
    send_to_char("\n\r$3Affecting Spells$R:\n\r--------------\n\r", ch);
    for(aff = k->affected; aff; aff = aff->next) {

      char * aff_name = get_skill_name(aff->type);
      
      if(!aff_name)
      {
         if(aff->type == INTERNAL_SLEEPING)
           aff_name = "Internal Sleeping";
         else if(aff->type == FUCK_CANTQUIT)
           aff_name = "CANT_QUIT";
         else
	     aff_name = "Unknown!!!";
      }
      sprintf(buf, "Spell : '%s' (%d)\n\r", aff_name, aff->type);
      send_to_char(buf, ch);
      sprintf(buf,"     Modifies %s by %d points\n\r",
              apply_types[(int) aff->location], aff->modifier);
      send_to_char(buf, ch);
      sprintf(buf,"     Expires in %3d hours", aff->duration);
//    strcat(buf,",Bits set ");
//      send_to_char(buf, ch);
//      sprintbit(aff->bitvector,affected_bits,buf);
      strcat(buf,"\n\r");
      send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);
  } 
}    

void obj_stat(struct char_data *ch, struct obj_data *j)
{
  struct obj_data  *j2=0;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  char buf4[MAX_STRING_LENGTH];
  struct extra_descr_data *desc;
  bool found;
  int i, virt;
  int value;


  /* for objects */
  extern char *item_types[];
  extern char *wear_bits[];
  extern char *extra_bits[];
  extern char *drinks[];
  extern char *more_obj_bits[];
  extern char *size_bits[];
  extern char *spells[];
  extern char *portal_bits[];
  int its;
  /* For chars */
  extern char *equipment_types[];
  extern char *apply_types[];

/*
    if(IS_SET(j->obj_flags.extra_flags, ITEM_DARK) && GET_LEVEL(ch) < POWER)
    {
      send_to_char("A magical aura around the item attempts to conceal its secrets.\r\n", ch);
      return;
    }
*/

    virt = (j->item_number >= 0) ? obj_index[j->item_number].virt : 0;
    sprintf(buf,"$3Object name$R:[%s]  $3R-number$R:[%d]  $3V-number$R:[%d]  $3Item type$R: ",
            j->name, j->item_number, virt);
    sprinttype(GET_ITEM_TYPE(j),item_types,buf2);

    strcat(buf,buf2); strcat(buf,"\n\r");
    send_to_char(buf, ch);

    sprintf(buf,"$3Short description$R: %s\n\r$3Long description$R:\n\r%s\n\r",
	    ((j->short_description) ? j->short_description : "None"),
	    ((j->description) ? j->description : "None") );
    send_to_char(buf, ch);
    if(j->ex_description)
    {
      strcpy(buf, "$3Extra description keyword(s)$R:\n\r----------\n\r");
      for (desc = j->ex_description; desc; desc = desc->next) 
      {
        strcat(buf, desc->keyword);
        strcat(buf, "\n\r");
      }
        strcat(buf, "----------\n\r");
        send_to_char(buf, ch);
    } 
    else 
    {
      strcpy(buf,"$3Extra description keyword(s)$R: None\n\r");
      send_to_char(buf, ch);
    }
    send_to_char("$3Can be worn on$R:", ch);
    sprintbit(j->obj_flags.wear_flags,wear_bits,buf);
    strcat(buf,"\n\r");
    send_to_char(buf, ch);

    send_to_char("$3Can be worn by$R:", ch);
    sprintbit(j->obj_flags.size,size_bits,buf);
    strcat(buf,"\n\r");
    send_to_char(buf, ch);

    send_to_char("$3Extra flags$R: ", ch);
    sprintbit(j->obj_flags.extra_flags,extra_bits,buf);
    strcat(buf,"\n\r");
    send_to_char(buf,ch);

    send_to_char("$3More flags$R: ", ch);
    sprintbit(j->obj_flags.more_flags,more_obj_bits,buf);
    strcat(buf,"\n\r");
    send_to_char(buf,ch);

    sprintf(buf,
             "$3Weight$R: %d  $3Value$R: %d  $3Timer$R: %d  $3Eq Level$R: %d\n\r",
             j->obj_flags.weight,j->obj_flags.cost,
             j->obj_flags.timer,
             j->obj_flags.eq_level);
             send_to_char(buf, ch);

    strcpy(buf,"$3In room$R: ");
    if (j->in_room == NOWHERE)
      strcat(buf,"Nowhere");
    else 
    {
      sprintf(buf2,"%d",world[j->in_room].number);
      strcat(buf,buf2);
    }
    strcat(buf,"  $3In object$R: ");
    strcat(buf, (!j->in_obj ? "None" : fname(j->in_obj->name)));
    strcat(buf,"  $3Carried by$R: ");
    strcat(buf, (!j->carried_by) ? "Nobody" : GET_NAME(j->carried_by));
    strcat(buf,"\n\r");
    send_to_char(buf, ch);

    switch (j->obj_flags.type_flag) 
    {
      case ITEM_LIGHT : 
        sprintf(buf,"$3Colour (v1)$R: %d\n\r"
                    "$3Type   (v2)$R: %d\n\r"
                    "$3Hours  (v3)$R: %d\n\r"
                    "$3Unused (v4)$R: %d",
	          j->obj_flags.value[0],
	          j->obj_flags.value[1],
	          j->obj_flags.value[2],
                  j->obj_flags.value[3]);
         break;
      case ITEM_SCROLL : 
        sprinttype(j->obj_flags.value[1]-1,spells,buf2); 
        sprinttype(j->obj_flags.value[2]-1,spells,buf3); 
        sprinttype(j->obj_flags.value[3]-1,spells,buf4); 
        sprintf(buf, "$3Level(v1)$R  : %d\n\r"
                     " $3Spells(v2)$R: %d (%s)\r\n"
                     " $3Spells(v3)$R: %d (%s)\r\n"
                     " $3Spells(v4)$R: %d (%s)",
	          j->obj_flags.value[0],
		  j->obj_flags.value[1], buf2,
		  j->obj_flags.value[2], buf3,
		  j->obj_flags.value[3], buf4 );
	  break;
      case ITEM_WAND : 
      case ITEM_STAFF :
          sprinttype(j->obj_flags.value[3]-1,spells,buf2); 
	  sprintf(buf, "$3Level(v1)$R: %d  $3Spell(v4)$R: %d - %s\r\n"
                       "$3Total Charges(v2)$R: %d   $3Current Charges(v3)$R: %d",
	          j->obj_flags.value[0],
	          j->obj_flags.value[3],
                  buf2,
	          j->obj_flags.value[1],
	          j->obj_flags.value[2]);
	  break;
      case ITEM_WEAPON :
int get_weapon_damage_type(struct obj_data * wielded);
its = get_weapon_damage_type(j) -1000;
extern char * strs_damage_types[];
	  sprintf(buf, "$3Unused(v1)$R: %d (make 0)\n\r$3Todam(v2)d(v3)$R: %dD%d\n\r$3Type(v4)$R: %d (%s)",
	          j->obj_flags.value[0],
	          j->obj_flags.value[1],
	          j->obj_flags.value[2],
	          j->obj_flags.value[3],
		  strs_damage_types[its]
		  );
	  break;
      case ITEM_MEGAPHONE:
	 sprintf(buf,"Interval(v1): %d\r\nInterval, again(v2): %d", j->obj_flags.value[0],j->obj_flags.value[1]);
 	 break;
      case ITEM_FIREWEAPON : 
	  sprintf(buf, "$3Tohit(v1)$R: %d\n\r$3Todam(v2)d<v3)$R: %dD%d\n\r$3Type(v4)$R: %d",
	          j->obj_flags.value[0],
	          j->obj_flags.value[1],
	          j->obj_flags.value[2],
	          j->obj_flags.value[3]);
	  break;
      case ITEM_MISSILE : 
	  sprintf(buf, "$3Damage(v1dv2)$R: %d$3/$R%d\n\r$3Tohit(v3)$R: %d\n\r$3Todam(v4)$R: %d",
	          j->obj_flags.value[0],
	          j->obj_flags.value[1],
	          j->obj_flags.value[3],
	          j->obj_flags.value[2]);
	  break;
      case ITEM_ARMOR :
          value = j->obj_flags.value[0] - j->obj_flags.value[1];
	  sprintf(buf, "$3AC-apply(v1)$R: [%d]\n\r"
                       "$3Unused  (v2)$R: [%d] (make 0)\n\r"
                       "$3Unused  (v3)$R: [%d] (make 0)\n\r"
                       "$3Unused  (v4)$R: [%d] (make 0)",
	          j->obj_flags.value[0], j->obj_flags.value[1], j->obj_flags.value[2], j->obj_flags.value[3]);
	  break;
      case ITEM_POTION : 
          sprinttype(j->obj_flags.value[1]-1,spells,buf2); 
          sprinttype(j->obj_flags.value[2]-1,spells,buf3); 
          sprinttype(j->obj_flags.value[3]-1,spells,buf4); 
	  sprintf(buf, "$3Level (v1)$R: %d\n\r"
                       " $3Spell(v2)$R: %d (%s)\n\r"
                       " $3Spell(v3)$R: %d (%s)\n\r"
                       " $3Spell(v4)$R: %d (%s)",
	          j->obj_flags.value[0],
	          j->obj_flags.value[1], buf2,
	          j->obj_flags.value[2], buf3,
	          j->obj_flags.value[3], buf4); 
	  break;
      case ITEM_TRAP :
	  sprintf(buf, "$3Spell(v1)$R    : %d\n\r"
                       "$3Hitpoints(v2)$R: %d",
	          j->obj_flags.value[0],
	          j->obj_flags.value[1]);
	  break;
      case ITEM_CONTAINER :
	  sprintf(buf,
	          "$3Max-contains(v1)$R : %d\n\r"
                  "$3Locktype(v2)$R     : %d\n\r"
                  "$3Key #$R            : %d\n\r"
                  "$3Corpse(v4)$R       : %s",
	          j->obj_flags.value[0],
	          j->obj_flags.value[1],
	          j->obj_flags.value[2],
	          j->obj_flags.value[3]?"Yes":"No");
	  break;
      case ITEM_DRINKCON :
	  sprinttype(j->obj_flags.value[2],drinks,buf2);
        //  strcpy(buf2,drinks[j->obj_flags.value[2]]);
	  sprintf(buf,
	  "$3Max-contains(v1)$R: %d\n\r"
          "$3Contains    (v2)$R: %d\n\r"
          "$3Liquid      (v3)$R: %s (%d)\n\r"
          "$3Poisoned    (v4)$R: %d",
	          j->obj_flags.value[0],
	          j->obj_flags.value[1],
                  buf2,
	          j->obj_flags.value[2],
		  j->obj_flags.value[3]);
	  break;
      case ITEM_NOTE :
	  sprintf(buf, "$3Tounge(v1)$R : %d"
                       "$3Unused(v2)$R : %d"
                       "$3Unused(v3)$R : %d"
                       "$3Unused(v4)$R : %d", 
                  j->obj_flags.value[0],
                  j->obj_flags.value[1],
                  j->obj_flags.value[2],
                  j->obj_flags.value[3]);
	  break;
      case ITEM_KEY :
	  sprintf(buf, "$3Keytype(v1)$R : %d"
                       "$3Unused (v2)$R : %d"
                       "$3Unused (v3)$R : %d"
                       "$3Unused (v4)$R : %d",
                  j->obj_flags.value[0],
                  j->obj_flags.value[1],
                  j->obj_flags.value[2],
                  j->obj_flags.value[3]);
	  break;
      case ITEM_FOOD :
	  sprintf(buf, "$3Makes full(v1)$R : %d\n\r"
                       "$3Poisoned  (v4)$R : %d",
	          j->obj_flags.value[0],
	          j->obj_flags.value[3]);
	  break;
      case ITEM_INSTRUMENT :
          sprintf(buf, "$3Song Effect$R:  $3Non-Combat(v1)$R[%d] $3Combat(v2)$R[%d]",
                  j->obj_flags.value[0],
                  j->obj_flags.value[1]);
          break;
      case ITEM_PORTAL :
          sprintf(buf, "$3ToRoom (v1)$R : %d\r\n"
                       "$3Type   (v2)$R : ",  j->obj_flags.value[0]);
          switch(j->obj_flags.value[1]) {
             case 0: strcat(buf, "0-Player-Portal");         break;
             case 1: strcat(buf, "1-Permanent-Game-Portal"); break;
             case 2: strcat(buf, "2-Temp-Game-portal");      break;
             case 3: strcat(buf, "3-Look-only-poral");       break;
             default: strcat(buf, "Unknown!!!");         break;
          }
          sprintf(buf2, "(can be 0-3)\r\n"
                      "$3Zone   (v3)$R : %d (can 'leave' anywhere from this zone (set to -1 otherwise))\r\n"
                      "$3Flags  (v4)$R : ", j->obj_flags.value[2] );
          strcat(buf, buf2);
          sprintbit(j->obj_flags.value[3],portal_bits,buf2);
          strcat(buf, buf2);
          strcat(buf, "\n(0 = nobits, 1 = no_leave, 2 = no_enter)");
          break;
      default :
	  sprintf(buf,"Values 0-3 : [%d] [%d] [%d] [%d]",
	          j->obj_flags.value[0],
	          j->obj_flags.value[1],
	          j->obj_flags.value[2],
	          j->obj_flags.value[3]);
	  break;
    }
    send_to_char(buf, ch);

    strcpy(buf,"\n\r$3Equipment Status$R: ");
    if (!j->carried_by)
      strcat(buf,"NONE");
    else 
    {
        found = FALSE;
        for (i=0;i < MAX_WEAR;i++) 
        {
          if (j->carried_by->equipment[i] == j) 
          {
            sprinttype(i,equipment_types,buf2);
            strcat(buf,buf2);
            found = TRUE;
          }
        }
        if (!found)
          strcat(buf,"Inventory");
    }
    send_to_char(buf, ch);

    strcpy(buf, "\n\r$3Non-Combat Special procedure$R : ");
    if (j->item_number >= 0)
      strcat(buf,(obj_index[j->item_number].non_combat_func ? "exists\n\r" : 
             "No\n\r"));
    else
      strcat(buf, "No\n\r");
    send_to_char(buf, ch);
    strcpy(buf, "$3Combat Special procedure$R : ");
    if(j->item_number >= 0)
       strcat(buf, (obj_index[j->item_number].combat_func ? "exists\n\r" :
              "No\n\r"));
    else
      strcat(buf, "No\n\r");
    send_to_char(buf, ch);
    strcpy(buf, "$3Contains$R :\n\r");
    found = FALSE;
    for(j2=j->contains;j2;j2 = j2->next_content) 
    {
      strcat(buf,fname(j2->name));
      strcat(buf,"\n\r");
      found = TRUE;
    }
    if (!found)
      strcpy(buf,"$3Contains$R : Nothing\n\r");
    send_to_char(buf, ch);

    send_to_char("$3Can affect char$R :\n\r", ch);
    for (i=0;i<j->num_affects;i++) 
    {
//      sprinttype(j->affected[i].location,apply_types,buf2);
                if (j->affected[i].location < 1000)
                 sprinttype(j->affected[i].location,apply_types,buf2);
                else if (get_skill_name(j->affected[i].location/1000))
                  strcpy(buf2, get_skill_name(j->affected[i].location/1000));
		else strcpy(buf2, "Invalid");
	
      sprintf(buf,"    $3Affects$R : %s By %d\n\r", buf2,j->affected[i].modifier);
      send_to_char(buf, ch);
    }           
  return;
}

void do_start(struct char_data *ch)
{
  char buf [256];

  send_to_char("This is now your character in Dark Castle mud.  You lucky "
               "piece of shit you.\n\r", ch);

  if(IS_MOB(ch))
    return;

  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;

  if(GET_TITLE(ch) == NULL) {
    GET_TITLE(ch) = str_dup("is a virgin.");
  }

  sprintf(buf, "%s appears with an ear-splitting bang!", GET_SHORT(ch));
  ch->pcdata->poofin = str_dup(buf);

  sprintf (buf, "%s disappears in a puff of smoke.", GET_SHORT(ch));
  ch->pcdata->poofout = str_dup(buf);

  ch->raw_hit  = 10;
  ch->max_hit  = 10;

  switch(GET_CLASS(ch)) {

    case CLASS_MAGIC_USER:
        ch->pcdata->practices = 6;
        break;

    case CLASS_CLERIC:
        ch->pcdata->practices = 6;
        break;

    case CLASS_THIEF:
        ch->pcdata->practices = 6;
        break;

    case CLASS_WARRIOR:
        ch->pcdata->practices = 6;
        break;
    }

    advance_level(ch, 0);
    redo_hitpoints(ch);

    GET_RDEATHS(ch) = 0;
    GET_PDEATHS(ch) = 0;
    GET_PKILLS(ch) = 0;

    GET_HIT(ch)  = hit_limit(ch);
    GET_MANA(ch) = mana_limit(ch);
    GET_MOVE(ch) = move_limit(ch);

    GET_COND(ch,THIRST) = 24;
    GET_COND(ch,FULL)   = 24;
    GET_COND(ch,DRUNK)  = 0;

    ch->pcdata->time.played = 0;
    ch->pcdata->time.logon  = time(0);

}


int do_repop(struct char_data *ch, char *argument, int cmd)
{
        char buf[MAX_STRING_LENGTH];
    
        if(GET_LEVEL(ch) < DEITY && !can_modify_room(ch, ch->in_room)) {
          send_to_char("You may only repop inside of your R range.\r\n", ch);
          return eFAILURE;
        }

        send_to_char("Resetting this entire zone!\n\r", ch);
        sprintf(buf, "%s repopped zone #%d.", GET_NAME(ch), world[ch->in_room].zone);
        log(buf, GET_LEVEL(ch), LOG_GOD);
        reset_zone(world[ch->in_room].zone);
	return eSUCCESS;
}

int do_clear(struct char_data *ch, char *argument, int cmd)
{       
        struct char_data *tmp_vict;
        struct char_data *next_vict;
        char buf[MAX_STRING_LENGTH];
        int zone = world[ch->in_room].zone; 
 
        if(GET_LEVEL(ch) < DEITY && !can_modify_room(ch, ch->in_room)) {
          send_to_char("You may only repop inside of your R range.\r\n", ch);
          return eFAILURE;
        }

        for(tmp_vict = character_list; tmp_vict; tmp_vict = next_vict)
        {
                next_vict = tmp_vict->next;
                if(world[tmp_vict->in_room].zone == zone)
                        if(IS_NPC(tmp_vict)) {
                                for (int l = 0; l < MAX_WEAR; l++ )
                                {
                                   if ( tmp_vict->equipment[l] )
                                      extract_obj(unequip_char(tmp_vict,l));
                                }
                                while(tmp_vict->carrying)
                                   extract_obj(tmp_vict->carrying);
                                extract_char(tmp_vict, TRUE);
                        }
                        else 
                                send_to_char("You hear unmatched screams of terror as all mobs are summarily executed!\n\r", tmp_vict);
        }
        send_to_char("You have just caused the destruction of countless creatures in ths area!\n\r",ch);
        sprintf(buf, "%s just CLEARED zone #%d!", GET_NAME(ch), zone);
        log(buf, GET_LEVEL(ch), LOG_GOD);
	return eSUCCESS;
}

int do_linkdead(struct char_data *ch, char *arg, int cmd)
{                               
  struct char_data *i;  
  int x = 0;
  char buf[100];
 
  for(i = character_list; i; i = i->next) {
     if(IS_NPC(i) || i->desc || !CAN_SEE(ch, i))
       continue;
     x++;
 
     if (i->pcdata->possesing)
        sprintf(buf, "%14s -- [%ld] %s  *possessing*\n\r", GET_NAME(i),
             (long)(world[i->in_room].number), (world[i->in_room].name));
     else
        sprintf(buf, "%14s -- [%ld] %s\n\r", GET_NAME(i),
             (long)(world[i->in_room].number), (world[i->in_room].name));
     send_to_char(buf, ch);
  }  
  
  if(!x)
    send_to_char("No linkdead players found.\n\r", ch);
  return eSUCCESS;
}    

int do_echo(struct char_data *ch, char *argument, int cmd)
{    
    int i;
    char buf[MAX_STRING_LENGTH];
    struct char_data *vict;
     
    if(IS_NPC(ch))
      return eFAILURE;
    
    for (i = 0; *(argument + i) == ' '; i++);

    if(!*(argument + i))
      send_to_char("What message do you want to echo to ths room?\n\r", ch);
     
    else {
        sprintf(buf, "\n\r%s\n\r", argument + i);
        for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
           send_to_char(buf, vict);
    }
    return eSUCCESS;
}   

int do_restore(struct char_data *ch, char *argument, int cmd)
{   
    struct char_data *victim;
    char buf[100];
    struct descriptor_data *i;        

    void update_pos(struct char_data *victim);

    if(!has_skill(ch, COMMAND_RESTORE)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
    }
    
    one_argument(argument,buf);
    if (!*buf)
        send_to_char("Whom do you wish to restore?\n\r",ch);
    else
     if (strcmp(buf, "all")) {
     
        if(!(victim = get_char(buf))) {
            send_to_char("No-one by that name in the world.\n\r",ch);
             return eFAILURE;
          }
     
            GET_MANA(victim) = GET_MAX_MANA(victim);
            GET_HIT(victim) = GET_MAX_HIT(victim);
            GET_MOVE(victim) = GET_MAX_MOVE(victim);
            GET_KI(victim) = GET_MAX_KI(victim);

            if(GET_LEVEL(victim) >= IMMORTAL) {
              GET_COND(victim, FULL)   = -1;
              GET_COND(victim, THIRST) = -1;
            }
            else {
      if (GET_COND(victim, FULL) != -1) {
              GET_COND(victim, FULL)   = 25;
              GET_COND(victim, THIRST) = 25;
                 }
            }
     
         sprintf(buf, "%s restored %s.", GET_NAME(ch), GET_NAME(victim));
         log(buf, GET_LEVEL(ch), LOG_GOD);

            update_pos( victim );
            redo_hitpoints(victim);
            redo_mana(victim);
	    redo_ki(victim);     
            send_to_char("Done.\n\r", ch);
            if (!IS_AFFECTED(ch, AFF_BLIND))
            act("You have been fully healed by $N!",
             victim, 0, ch, TO_CHAR, 0);
            do_save(victim, "", 9);
        } else {    /* Restore all */

       if (GET_LEVEL(ch) < OVERSEER) {
           send_to_char("You don't have the ability to do that!\n\r", ch);
     sprintf (buf, "%s tried to do a restore all!", GET_NAME(ch));
            log(buf, GET_LEVEL(ch), LOG_GOD);
     
              return eFAILURE;
         }
          for (i = descriptor_list; i; i = i->next)
             if (i->character != ch  && !i->connected) {
     
               victim = i->character;
     
            GET_MANA(victim) = GET_MAX_MANA(victim);
            GET_HIT(victim) = GET_MAX_HIT(victim);
            GET_MOVE(victim) = GET_MAX_MOVE(victim);
            GET_KI(victim) = GET_MAX_KI(victim);

            if(GET_LEVEL(victim) >= IMMORTAL) {
              GET_COND(victim, FULL)   = -1;
              GET_COND(victim, THIRST) = -1;
            }
            else {
       if (GET_COND(victim, FULL) != -1) {
              GET_COND(victim, FULL)   = 25;
              GET_COND(victim, THIRST) = 25;
               }
            }
     
            update_pos( victim );
	    redo_ki(victim);
            redo_hitpoints(victim);
            redo_mana(victim);
     
            act("You have been fully healed by $N!",
             victim, 0, ch, TO_CHAR, 0);
            do_save(victim, "", 9);
        }
           sprintf(buf, "%s did a restore all!", GET_NAME(ch));
            log(buf, GET_LEVEL(ch), LOG_GOD);
        send_to_char("Trying to be Mister Popularity?\n\r", ch);
  }  
    return eSUCCESS;
} 

void special_log(char *arg)
{
  FILE *fl;

  if(!(fl = dc_fopen("../lib/special.txt", "a"))) {
    log("Unable to open SPECIAL LOG FILE in special_log.", IMP, LOG_GOD);
    return;
  }

  fprintf(fl, "%s\n", arg);
  dc_fclose(fl);
}


// Scavenger hunts..

struct hunt_data
{
  struct hunt_data *next;
  char *huntname;
  int itemnum;
  int time;
  int itemsAvail[50];
};

struct hunt_items
{ // Bleh, don't wanna make it go through every item in the game everyone someone checks the list
   struct hunt_items *next;
   struct hunt_data *hunt;
   struct obj_data *obj;
   char *mobname;
};

struct hunt_data *hunt_list = NULL;
struct hunt_items *hunt_items_list = NULL;

extern void send_info(char *messg);

void check_end_of_hunt(struct hunt_data *h, bool forced = FALSE)
{
  struct hunt_items *i,*p=NULL,*in;
  int items = 0;

  for (i = hunt_items_list;  i;i = in)
  {
    in = i->next;
    if (i->hunt == h)
    {
      if (h->time <= 0)
      {
	//obj_from_char(i->obj);
	if (p) p->next = in;
	else hunt_items_list = in;

	extract_obj(i->obj);
	dc_free(i->mobname);
        dc_free(i);
	continue;
      } else items++; 
    }
    p = i;
  }
  char buf[MAX_STRING_LENGTH];
  if (items == 0)
  {
    if (!forced) {
     if (h->huntname) {
       if (h->time <= 0) sprintf(buf,"\r\n## The time limit on hunt '%s' has expired and all unrecovered prizes have been removed.\r\n",h->huntname);
       else sprintf(buf,"\r\n## All prizes have been recovered on hunt '%s'\r\n",h->huntname);
	} else {
       if (h->time <= 0) sprintf(buf,"\r\n## The time limit on the hunt for '%s' has expired and all unrecovered prizes have been removed.\r\n",((OBJ_DATA*)obj_index[real_object(h->itemnum)].item)->short_description);
       else sprintf(buf,"\r\n## All prizes have been recovered on the hunt for '%s'\r\n",((OBJ_DATA*)obj_index[real_object(h->itemnum)].item)->short_description);
	}
      } else {
      if (h->huntname) {
         sprintf(buf,"\r\n## Hunt '%s' has been ended.\r\n",h->huntname);
	} else {
        sprintf(buf,"\r\n## The hunt for '%s' has been ended.\r\n",((OBJ_DATA*)obj_index[real_object(h->itemnum)].item)->short_description);
	}
      }
     send_info(buf);

     struct hunt_data *hl,*p=NULL;
     for (hl = hunt_list;hl;hl = hl->next)
     {
	if (hl == h)
	{
	  if (p) p->next = hl->next;
	  else hunt_list = hl->next;
	  break;
	}
	p = hl;
     }
     dc_free(h->huntname);
     dc_free(h);     
  }
}

int do_huntclear(struct char_data *ch, char *arg, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  arg = one_argument(arg,arg1);
  
  if (str_cmp(arg1, "doit"))
  {
    send_to_char("Syntax: huntclear doit\r\nClears all currently running treasure hunts.\r\n",ch);
    return eSUCCESS;
  } else {
    struct hunt_data *h,*hn;
    for (h = hunt_list;h; h = hn)
    {
       hn = h->next;
       h->time = -1;
       check_end_of_hunt(h,TRUE);
    }
    send_to_char("Done!\r\n",ch);
    return eSUCCESS;
  }
}

void huntclear_item(struct obj_data *obj)
{
  struct hunt_items *hi,*hin,*hip = NULL;
  for (hi = hunt_items_list;hi;hi = hin)
  {
    hin = hi->next;
    if (hi->obj == obj)
    {
	if (hip) hip->next = hin;
	else hunt_items_list = hin;
	dc_free(hi->mobname);
	dc_free(hi);
	continue; // Hopefully there's not two in the list, but just in case.
    }
    hip = hi;
  }
}

int get_rand_obj(struct hunt_data *h)
{
  int i,v;

  for (i = 49;i > 0;i--)
    if (h->itemsAvail[i] > 0)
      break;

  if (i < 0) return -1;

  v = number(0,i);
  int c = h->itemsAvail[v];
  h->itemsAvail[v] = h->itemsAvail[i];
  h->itemsAvail[i] = -1;

  return c;
}

void init_random_hunt_items(struct hunt_data *h)
{
  FILE *f;
  if ((f= dc_fopen("huntitems.txt","r"))==NULL)
  {
    for (int i = 0; i < 50;i++)
	h->itemsAvail[i] = -1;
    return; 
  }
  int a, i;
  bool over = FALSE;
  for (a = 0; a < 50;a++)
  {
    if (!over) {
      i = fread_int(f, 0, INT_MAX);
      if (i == 0) over = TRUE;
      else { h->itemsAvail[a] = i; continue; }
    }
    h->itemsAvail[a] = -1;
  }
  dc_fclose(f);
}

void begin_hunt(int item, int time, int amount, char *huntname)
{ // time, itme, item
  struct hunt_data *n;
  
#ifdef LEAK_CHECK
  n = (struct hunt_data *)calloc(1, sizeof(struct hunt_data));
#else
  n = (struct hunt_data *)dc_alloc(1, sizeof(struct hunt_data));
#endif
  n->next = hunt_list;
  hunt_list = n;
  n->itemnum = item;
  n->time = time;
  if (huntname) n->huntname = str_dup(huntname);
  if (item == 76) init_random_hunt_items(n);
  int rnum = real_object(item);
  extern int top_of_mobt;
  if (rnum < 0) return;

  for (int i = 0; i < amount;i++)
  {
    int mob = -1;
    CHAR_DATA *vict;
    while (1)
    {
	mob = number(1,top_of_mobt);
	int vnum = mob_index[mob].virt; // debug
	if (!(mob_index[mob].virt > 300 &&
        (mob_index[mob].virt < 2300 || mob_index[mob].virt > 2499) &&
	(mob_index[mob].virt < 29200 || mob_index[mob].virt > 29299) && 
	(mob_index[mob].virt < 5600 || mob_index[mob].virt > 5699) && 
	(mob_index[mob].virt < 16200 || mob_index[mob].virt > 16399) && 
	(mob_index[mob].virt < 1900 || mob_index[mob].virt > 1999) && 
	(mob_index[mob].virt < 10500 || mob_index[mob].virt > 10622) && 
	(mob_index[mob].virt < 8500 || mob_index[mob].virt > 8699 ))) continue;


        if (mob_index[mob].number <= 0) continue;
	if (!(vict = get_random_mob_vnum(vnum))) continue;
        if (IS_SET(zone_table[world[vict->in_room].zone].zone_flags, ZONE_NOHUNT)) continue;

	if (strlen(vict->short_desc) > 34) continue; // They suck

	break;
    }
    struct obj_data *obj = clone_object(rnum);
    obj_to_char(obj, vict);
    struct hunt_items *ni;
#ifdef LEAK_CHECK
  ni = (struct hunt_items *)calloc(1, sizeof(struct hunt_items));
#else
  ni = (struct hunt_items *)dc_alloc(1, sizeof(struct hunt_items));
#endif
    ni->hunt = n;
    ni->obj = obj;
    ni->mobname = str_dup(vict->short_desc);
    ni->next = hunt_items_list;
    hunt_items_list = ni;
  }
}

void pick_up_item(struct char_data *ch, struct obj_data *obj)
{
  struct hunt_items *i,*p = NULL,*in;
  char buf[MAX_STRING_LENGTH];
  int gold = 0;
  for (i = hunt_items_list; i; i = in)
  {
     in = i->next;
     if (i->obj == obj)
     {
	if (p) p->next = in;
	else hunt_items_list = in;
	int vnum = obj_index[obj->item_number].virt;
	sprintf(buf, "\r\n## %s has been recovered from %s by %s!\r\n",
		obj->short_description,i->mobname,ch->name);
	send_info(buf);
	struct hunt_data *h = i->hunt;
	struct obj_data *oitem = NULL;
	int r1 = 0;
	switch (vnum)
	{
		case 76:
		  obj_from_char(obj);
		  obj_to_room(obj, 6345);
		  r1 = real_object(get_rand_obj(h));
		  if (r1 > 0)
		  {
		    oitem = clone_object(r1);
		    sprintf(buf, "As if by magic, %s transforms into %s!\r\n",
			obj->short_description, oitem->short_description);
		    send_to_char(buf,ch);
		    sprintf(buf, "## %s turned into %s!\r\n",
			obj->short_description,oitem->short_description);
		    send_info(buf);
                    if(IS_SET(oitem->obj_flags.more_flags, ITEM_UNIQUE)) {
                      if(search_char_for_item(ch, oitem->item_number)) {
                        send_to_char("The item's uniqueness causes it to poof into thin air!\r\n", ch);
                        extract_obj(oitem);
			break; // Used to crash it.
                      } else obj_to_char(oitem, ch);
                    }
		    else obj_to_char(oitem, ch);
		  } else {
		    send_to_char("Brick turned into a non-existent item. Tell an imm.\r\n",ch);
		    break;
		  }
		  if (obj_index[oitem->item_number].virt < 27915 || obj_index[oitem->item_number].virt > 27918)
			  break;
		  else obj = oitem; // Gold! Continue on to the next cases.
		case 27915:
		case 27916:
		case 27917:
		case 27918:
		  gold = obj->obj_flags.value[0];
		  sprintf(buf, "As if by magic, %s transform into %d gold!\r\n",
			obj->short_description, gold);
		  send_to_char(buf,ch);

		  GET_GOLD(ch) += gold;
		  obj_from_char(obj);
		  obj_to_room(obj, 6345);
		  break;

	}
	dc_free(i->mobname);
	dc_free(i);
	check_end_of_hunt(h);
	continue;
     }
     p = i;
  }
}

void pulse_hunts()
{
  struct hunt_data *h,*hn;

  for (h = hunt_list;h;h = hn)
  {
     hn = h->next;
     h->time--;
     check_end_of_hunt(h);
  }

  struct obj_data *obj,*onext;
  for (obj = world[6345].contents;obj;obj = onext)
  {
    onext = obj->next_content;
    extract_obj(obj);
  }
}

int do_showhunt(CHAR_DATA *ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  struct hunt_data *h;
  struct hunt_items *hi;

  if (!hunt_list) send_to_char("There are no active hunts at the moment.\r\n",ch);
  else send_to_char("The following hunts are currently active:\r\n",ch);

  for (h = hunt_list;h;h = h->next)
  {
	if (h->huntname)
	 sprintf(buf,"\r\n%s for '%s'(%d minutes remaining):\r\n",h->huntname,
			((OBJ_DATA*)obj_index[real_object(h->itemnum)].item)->short_description,h->time);
	else
	 sprintf(buf,"\r\nThe hunt for '%s'(%d minutes remaining):\r\n",
			((OBJ_DATA*)obj_index[real_object(h->itemnum)].item)->short_description,h->time);
	send_to_char(buf,ch);
	int itemsleft = 0;
	buf[0] = '\0';
	for (hi = hunt_items_list;hi;hi = hi->next)
	{
		if (hi->hunt != h) continue;
		itemsleft++;
		sprintf(buf,"%s| %-35s  ",
			buf,hi->mobname);
		if ((itemsleft % 2) == 0)
		{
		  sprintf(buf,"%s|\r\n",buf);
		  send_to_char(buf,ch);
		  buf[0] = '\0';
		}
	}
	if (buf[0] != '\0') {
	  sprintf(buf,"%s|\r\n",buf);
	  send_to_char(buf,ch);
	}
  }
 return eSUCCESS;
}

int do_huntstart(struct char_data *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  argument = one_argument(argument,arg);
  argument = one_argument(argument,arg2);
  argument = one_argument(argument,arg3);

  if (arg3[0] == '\0')
  {
     send_to_char("Syntax: huntstart <vnum> <# of items> <time limit> [hunt name]\r\n",ch);
     return eSUCCESS;
  }
  int vnum = atoi(arg), num = atoi(arg2), time = atoi(arg3);
  if (vnum <= 0 || real_object(vnum) < 0)
  {
    send_to_char("Non-existent item.\r\n",ch);
    return eSUCCESS;
  }
  if (num <= 0 || num > 50)
  {
    send_to_char("Invalid #.\r\n",ch);
    return eSUCCESS;
  }
  if (time <= 0)
  {
    send_to_char("Invalid duration.\r\n",ch);
    return eSUCCESS;
  }
  struct hunt_data *h;
  for (h = hunt_list;h;h = h->next)
  if (h->itemnum == vnum)
  {
    send_to_char("A hunt for that item is already ongoing!\r\n",ch);
    return eSUCCESS;
  }
  char huntname[200];
  if (argument && *argument) 
  {
	strcpy(huntname,argument);
	begin_hunt(vnum, time, num, argument);
  }
  else
  {
	strcpy(huntname, "A hunt");
	begin_hunt(vnum,time,num,0);
  }

  char buf[MAX_STRING_LENGTH];
  sprintf(buf,"\r\n## %s has been started! There are a total of %d items and %d minutes to find them all!\r\n## Type 'huntitems' to get the locations!\r\n",huntname,num,time);
  send_info(buf);
  return eSUCCESS;
}
