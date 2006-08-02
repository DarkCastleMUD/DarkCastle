/************************************************************************
| $Id: group.cpp,v 1.23 2006/08/02 21:10:30 shane Exp $
| group.C
| Description:  Group related commands; join, abandon, follow, etc..
*/
extern "C"
{
#include <ctype.h> // isspace()
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <character.h>
#include <room.h>
#include <affect.h>
#include <utility.h>
#include <mobile.h>
#include <interp.h>
#include <handler.h>
#include <clan.h>
#include <levels.h>
#include <act.h>
#include <db.h>
#include <player.h>
#include <sing.h> // stop_grouped_bards
#include <string.h>
#include <returnvals.h>
#include <spells.h>

extern CWorld world;
 

int do_abandon(CHAR_DATA *ch, char *argument, int cmd)
{
  CHAR_DATA *k;
  char buf[MAX_INPUT_LENGTH+1];


  if (IS_SET(world[ch->in_room].room_flags, QUIET))
  {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  if ( (!ch->master) && (IS_AFFECTED(ch, AFF_GROUP)) ) {
    send_to_char("You can't abandon a group you're leading.\n\r",ch);
    send_to_char("You must disband the group.\n\r",ch);
    return eFAILURE;
  }

  if ( (!ch->master) || (!IS_AFFECTED(ch, AFF_GROUP)) ) {
     send_to_char("Who you gonna abandon?!\n\r",ch);
     return eFAILURE;
  }

  if ((IS_NPC(ch))  && (IS_AFFECTED(ch, AFF_CHARM)) ) {
      send_to_char("You're in love. Forget it!\n\r", ch);
      return eFAILURE;
  }

  stop_grouped_bards(ch,1);

  k = ch->master;
   
  sprintf(buf,"You abandon: %s\n\r", k->group_name);
  send_to_char(buf,ch);
  sprintf(buf,"%s abandons: %s", GET_SHORT(ch), k->group_name);
  act(buf,ch,0,0,TO_ROOM, 0);

  if(!IS_MOB(ch)) {
    ch->pcdata->grplvl      = 0;
    ch->pcdata->group_kills = 0;
  }

  stop_follower(ch, STOP_FOLLOW);
  return eSUCCESS;
}

  
int do_found(CHAR_DATA *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH+1];

  if (IS_NPC(ch))
     return eFAILURE;

  if (IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }
        
  if (!*argument) {
    send_to_char("Found what?!\n\r", ch);
    return eFAILURE;
  }
  if ( (ch->master) && (!IS_AFFECTED(ch, AFF_GROUP)) ) {
    send_to_char("You can't found your own group while following someone around!\n\r",ch);
    return eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("You can't found a group if you're already in one!\n\r",ch);
    return eFAILURE;
  }
    
  for (; isspace(*argument); argument++);

  if (strlen(argument) > 50){
    send_to_char("You gonna name your party? Or write a book?!  50 characters max.\n\r", ch);
    return eFAILURE;
  }
  
  ch->group_name = str_dup(argument);
  sprintf(buf,"You found: %s\n\r", argument);
  send_to_char(buf, ch);
  sprintf(buf,"%s founds: %s", GET_SHORT(ch), argument);
  act(buf, ch, 0, 0, TO_ROOM, 0);

  if(!IS_MOB(ch)) {
    ch->pcdata->group_kills = 0;
    ch->pcdata->grplvl      = 0;
  }

  SETBIT(ch->affected_by, AFF_GROUP);
  REMOVE_BIT(ch->pcdata->toggles, PLR_LFG);
  return eSUCCESS;  
}

int do_split(CHAR_DATA *ch, char *argument, int cmd)
{
  int32 amount, share, extra;
  char buf[256], number[MAX_INPUT_LENGTH+1];
  int no_members;
  CHAR_DATA *k;
  struct follow_type *f;

  if (!*argument){
    send_to_char("Split what?\n\r", ch);
    return eFAILURE;
  }

  if(affected_by_spell(ch, FUCK_GTHIEF)) {
     send_to_char("Nobody wants any part of your stolen booty!\n\r", ch);
     return eFAILURE;
  }

  one_argument (argument, number);
  
  if (strlen(number)>7){
    send_to_char("Number field too big.\n\r", ch);
    return eFAILURE;
  }

  amount = atol(number);

  if ( amount < 0 ) {
    send_to_char( "Your group wouldn't like that!\n\r", ch );
    return eFAILURE;
  }

  if ( amount == 0 ) {
    send_to_char("You hand out zero coins to everyone, but no one notices.\n\r", ch );
    return eSUCCESS;
  }

  if (GET_GOLD(ch) < (uint32) amount) {
    send_to_char( "You don't have that much gold!\n\r", ch );
    return eFAILURE;
  }
  
  if (ch->master)
    k = ch->master;
  else k = ch;

  if ( (!IS_AFFECTED(k, AFF_GROUP)) || (!IS_AFFECTED(ch, AFF_GROUP)))
  {
    send_to_char("You must be grouped to split your money!\n\r", ch);
    return eFAILURE;
  }

  if(k->in_room == ch->in_room)
    no_members = 1; 
  else no_members = 0;

  for (f=k->followers; f; f=f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) &&
    (f->follower->in_room == ch->in_room) &&
    !IS_MOB(f->follower))
      no_members++;
  }

  if(no_members == 0) // should be impossible
  {
    send_to_char("You got a divide by 0.  Tell a god how.\r\n", ch);
    return eFAILURE;
  }

  share = amount / no_members;
  extra = amount % no_members;
  GET_GOLD(ch) -= amount;
  do_save(ch, "", 666);

  
  sprintf( buf, "You split %d gold coins.  "
                "Your share is %d gold coins.\n\r", amount, share + extra );
  send_to_char( buf, ch );
  GET_GOLD(ch) += share + extra;

  sprintf( buf, "%s splits %d gold coins.  "
                "Your share is %d gold coins.\n\r", GET_SHORT(ch), amount, share );

  if(k != ch && k->in_room == ch->in_room) {
    send_to_char(buf, k);
    GET_GOLD(k) += share;
  }
  char buf2[MAX_STRING_LENGTH];
  for (f=k->followers; f; f=f->next) 
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) &&
        f->follower->in_room == ch->in_room &&
        f->follower != ch &&
        !IS_MOB(f->follower)) 
    {
      send_to_char( buf, f->follower );
      int lost = 0;
      if (f->follower->clan && get_clan(f->follower)->tax && 
           !IS_SET(GET_TOGGLES(f->follower), PLR_NOTAX))
      {
	lost = (int)((float)share*(float)((float)get_clan(f->follower)->tax/100));
	sprintf(buf2,"Your clan taxes %d gold of your share.\r\n",lost);
	get_clan(f->follower)->balance += lost;
	save_clans();
      send_to_char(buf2, f->follower);
      }
      GET_GOLD(f->follower) += (share - lost);
    } 
  }
  return eSUCCESS;
}

void setup_group_buf(char * report, char_data * j, char_data *i)
{
  if(IS_NPC(j) || (IS_ANONYMOUS(j) && (i->clan != j->clan || !i->clan)))
  {
    if(GET_CLASS(j) == CLASS_MONK || GET_CLASS(j) == CLASS_BARD)
      sprintf(report, "[-====-|      %3d%%    hp     %3d%%   k     %3d%%   mv]",
	           MAX(1, GET_HIT(j))*100 / MAX(1, GET_MAX_HIT(j)),
		   MAX(1, GET_KI(j))*100 / MAX(1, GET_MAX_KI(j)),
		   MAX(1, GET_MOVE(j))*100 / MAX(1, GET_MAX_MOVE(j)));
    else if(GET_CLASS(j) == CLASS_WARRIOR || GET_CLASS(j) == CLASS_THIEF ||
            GET_CLASS(j) == CLASS_BARBARIAN) 
      sprintf(report, "[-====-|      %3d%%    hp    -====-        %3d%%   mv]",
	           MAX(1, GET_HIT(j))*100 / MAX(1, GET_MAX_HIT(j)),
		   MAX(1, GET_MOVE(j))*100 / MAX(1, GET_MAX_MOVE(j)));
    else sprintf(report, "[-====-|      %3d%%    hp     %3d%%   m     %3d%%   mv]",
	           MAX(1, GET_HIT(j))*100 / MAX(1, GET_MAX_HIT(j)),
		   MAX(1, GET_MANA(j))*100 / MAX(1, GET_MAX_MANA(j)),
		   MAX(1, GET_MOVE(j))*100 / MAX(1, GET_MAX_MOVE(j)));
  }
  else
  {
    if(GET_CLASS(j) == CLASS_MONK || GET_CLASS(j) == CLASS_BARD)
      sprintf(report, "[Lv %3d| %6d/%-6dhp %5d/%-5dk %5d/%-5dmv]",
                   GET_LEVEL(j), GET_HIT(j), GET_MAX_HIT(j), GET_KI(j),
                   GET_MAX_KI(j), GET_MOVE(j), GET_MAX_MOVE(j));
    else if(GET_CLASS(j) == CLASS_WARRIOR || GET_CLASS(j) == CLASS_THIEF ||
                   GET_CLASS(j) == CLASS_BARBARIAN) 
      sprintf(report, "[Lv %3d| %6d/%-6dhp    -====-    %5d/%-5dmv]",
                   GET_LEVEL(j), GET_HIT(j), GET_MAX_HIT(j), 
                   GET_MOVE(j), GET_MAX_MOVE(j));
    else sprintf(report, "[Lv %3d| %6d/%-6dhp %5d/%-5dm %5d/%-5dmv]",
                   GET_LEVEL(j), GET_HIT(j), GET_MAX_HIT(j), GET_MANA(j),
                   GET_MAX_MANA(j), GET_MOVE(j), GET_MAX_MOVE(j));
  }
}

int do_group(struct char_data *ch, char *argument, int cmd)
{
  char name[256];
  char buf[256], report[256];
  struct char_data *victim, *k, *j;
  struct follow_type *f;
  bool found;

  one_argument(argument, name);

  if(!*name) {
    if(!IS_AFFECTED(ch, AFF_GROUP))
      send_to_char("But you are a member of no group?!\n\r", ch);
    else {
      send_to_char("Your group consists of:\n\r", ch);
      if(ch->master)
        k = ch->master;
      else
        k = ch;
		
      setup_group_buf(report, k, ch);
      sprintf(buf, "%s    $N (Leader)", report);

      if(IS_AFFECTED(k, AFF_GROUP))
        act(buf, ch, 0, k, TO_CHAR, ASLEEP);

      for(f = k->followers; f; f = f->next) {
        if(IS_AFFECTED(f->follower, AFF_GROUP)) {
	  j = f->follower;
          setup_group_buf(report, j,ch);
          sprintf(buf, "%s    $N", report);
          act(buf,  ch, 0, f->follower, TO_CHAR, ASLEEP);
        }
      }
    }
	
    return eSUCCESS;
  }

  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  if (!(victim = get_char_room_vis(ch, name)))
    send_to_char("No one here by that name.\n\r", ch);

  else {
    if(!IS_AFFECTED(ch, AFF_GROUP)) {
      send_to_char("You must first found a group!\n\r",ch);
      return eFAILURE;
    }

    if(ch->master) {
      act("You can not enroll group members without being head of a group.", ch, 0, 0, TO_CHAR, 0);
      return eFAILURE;
    }

    found = FALSE;

    if(victim == ch)
      found = TRUE;
    else {
      for(f = ch->followers; f; f = f->next) {
        if(f->follower == victim) {
          found = TRUE;
          break;
        }
      }
    }
	
    if(found) {
      if(ch == victim) {
        send_to_char("You must found a group, or Disband a group.\n\r",ch);
        return eFAILURE;
      }
//      if((abs(GET_LEVEL(ch) - GET_LEVEL(victim)) ) <= 99) {
        if (IS_AFFECTED(victim, AFF_GROUP)) {
                stop_grouped_bards(victim, 1);
		act("$n has been kicked out of the group!", victim, 0, ch, TO_ROOM, 0);
		act("You are no longer a member of the group!", victim, 0, 0, TO_CHAR, ASLEEP);
		REMBIT(victim->affected_by, AFF_GROUP);
        } else {
		act("$n is now a group member.", victim, 0, 0, TO_ROOM, 0);
		act("You are now a group member.", victim, 0, 0, TO_CHAR, ASLEEP);
		SETBIT(victim->affected_by, AFF_GROUP);
                if(!IS_NPC(victim))
                   REMOVE_BIT(victim->pcdata->toggles, PLR_LFG);
        }
        return eSUCCESS;
  //    }
    //  else
//	  act("$n is not of the right caliber to join this group.", victim, 0, 0, TO_ROOM, ASLEEP);
    }
    else
      act("$N must follow you, to enter the group.", ch, 0, victim, TO_CHAR, ASLEEP);
  }
  return eFAILURE;
}

int do_promote(CHAR_DATA *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH+1];
  char buf[250];
  CHAR_DATA *new_new_leader, *k;
  struct follow_type *f, *next_f;

  one_argument(argument, name);

  if(ch->master) {
    send_to_char("You aren't running the show here, pal.\n\r",ch);
    return eFAILURE;
  }

  if((!ch->master) && !IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("You don't even have a group to promote anyone.\n\r", ch);
    return eFAILURE;
  }

  if(!*name) {
    send_to_char("Who do you wish to promote to group leader? \n\r",ch);
    return eFAILURE;
  }

  if(!(new_new_leader = get_char_room_vis(ch, name))) {
    send_to_char("I see no person by that name here!\n\r", ch);
    return eFAILURE;
  }

  if(new_new_leader == ch) {
    send_to_char("You truly must have an EGO the size of the Tarrasque!\n\r",
      ch);
    send_to_char("You are already the group leader! Maybe you SHOULD "
      "step down?\n\r", ch);
    return eFAILURE;
  }

  if(IS_NPC(new_new_leader)) {
    send_to_char("Yeah right!\n\r",ch);
    return eFAILURE;
  }

  if(!ARE_GROUPED(ch, new_new_leader)) {
    send_to_char("But you aren't even in the same group!\n\r",ch);
    return eFAILURE;
  }

  sprintf(buf, "You step down, appointing %s as the new leader.\n\r",
          GET_SHORT(new_new_leader));
  send_to_char(buf, ch);
  sprintf(buf, "%s steps down as leader of: %s\n\r%s appoints YOU as "
          "the New Leader of: %s\n\r", GET_SHORT(ch), ch->group_name,
          GET_SHORT(ch), ch->group_name);
  send_to_char(buf, new_new_leader); 
  sprintf(buf, "%s steps down as leader of: %s\n\r%s appoints %s as "
          "the New Leader of: %s", GET_SHORT(ch), ch->group_name,  
          GET_SHORT(ch), GET_SHORT(new_new_leader), ch->group_name);
  act(buf, ch, 0, new_new_leader, TO_ROOM, NOTVICT);

  if(!IS_MOB(ch) && !IS_MOB(new_new_leader)) {
    new_new_leader->pcdata->grplvl      = ch->pcdata->grplvl;
    new_new_leader->pcdata->group_kills = ch->pcdata->group_kills;
    ch->pcdata->group_kills = 0;
    ch->pcdata->grplvl      = 0;
  }

  if(ch->group_name) {
    new_new_leader->group_name  = ch->group_name;
    ch->group_name = NULL;
  }
  else
    new_new_leader->group_name  = str_dup("I am a dork");

  stop_follower(new_new_leader, CHANGE_LEADER);

  for(f = ch->followers; f; f = next_f) { 
     next_f = f->next;
     if(!IS_NPC(f->follower)) {
       k = f->follower;
       stop_follower(k, CHANGE_LEADER);
       add_follower(k, new_new_leader, 2);
     } else
        REMBIT(f->follower->affected_by, AFF_GROUP);
  }

  add_follower(ch, new_new_leader, 2);
  return eSUCCESS;
}

int do_disband(CHAR_DATA *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH+1];
  char buf[200];
  CHAR_DATA *adios, *k;
  struct follow_type *f, *next_f;

  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n",
                 ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  if(ch->master) {
    send_to_char("You aren't running the show here pal!\n\r",ch);
    return eFAILURE;
  }

  if((!ch->master) && (!IS_AFFECTED(ch, AFF_GROUP))) {
    send_to_char("You don't even have a group to disband.\n\r", ch);
    return eFAILURE;
  }

  if(!*name){
    send_to_char("Who do you wish to disband? \n\r",ch);
    send_to_char("Disband 'all' will disband the group.\n\r",ch);
    return eFAILURE;
  }

  if(isname(name, "all")) {
    k = ch;
    sprintf(buf, "You disband your group: %s", k->group_name);
    act(buf, k, 0, 0, TO_CHAR, 0);
    sprintf(buf, "$n disbands $s group: %s", k->group_name);
    act(buf, k, 0, 0, TO_ROOM, 0);

    dc_free(k->group_name);
    k->group_name = 0;

    if(!IS_MOB(k)) {
      k->pcdata->group_kills = 0;
      k->pcdata->grplvl      = 0;
    }

    for(f = k->followers; f; f = next_f) { 
       next_f = f->next;
       if (!IS_NPC(f->follower)) {
          stop_grouped_bards(f->follower,1);
          stop_follower(f->follower, STOP_FOLLOW);
       } else
          REMBIT(f->follower->affected_by, AFF_GROUP);
    }

    REMBIT(k->affected_by, AFF_GROUP);
    return eSUCCESS;
  }

  if(!(adios = get_char_room_vis(ch, name))) {
    send_to_char("I see no person by that name here!\n\r", ch);
    return eFAILURE;
  }

  if(adios == ch) {
    send_to_char("You can't disband yourself from a group you're leading!\n\r", ch);
    send_to_char("Either Promote someone to Group Leader, or Disband All.\n\r", ch);
    return eFAILURE;
  }

  if(adios->master != ch)
  {
    send_to_char("Try someone in YOUR group, fartknocker.\r\n", ch);
    return eFAILURE;
  }

  if((IS_NPC(adios)) && (IS_AFFECTED(adios, AFF_CHARM))) {
    send_to_char("Can't kick out a charmee.\n\r", ch);
    return eFAILURE;
  }

  stop_grouped_bards(adios,1);
  if(!IS_MOB(adios)) {
    adios->pcdata->grplvl      = 0;
    adios->pcdata->group_kills = 0;
  }
  stop_follower(adios, STOP_FOLLOW);
  return eSUCCESS;
}


int do_follow(CHAR_DATA *ch, char *argument, int cmd)
{
    char name[MAX_INPUT_LENGTH+1];
    CHAR_DATA *leader;


    void stop_follower(CHAR_DATA *ch, int cmd);
    void add_follower(CHAR_DATA *ch, CHAR_DATA *leader, int cmd);

    if (IS_SET(world[ch->in_room].room_flags, QUIET))
    {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
    }

    one_argument(argument, name);

    if (*name) {
	if (!(leader = get_char_room(name,ch->in_room))) {
	    send_to_char("I see no person by that name here!\n\r", ch);
	    return eFAILURE;
	}
    } else {
	send_to_char("Who do you wish to follow?\n\r", ch);
	return eFAILURE;
    }
    if (cmd == 9 && !CAN_SEE(ch, leader))
    { // check it like this instead o' get_char_room_vis 'cause stalk checks.
      send_to_char("I see no person by that name here!\r\n",ch);
      return eFAILURE;
    }

  if (IS_AFFECTED(ch, AFF_GROUP)) {
        send_to_char("You must first abandon your group.\n\r",ch);
       return eFAILURE;
     }

    if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master)) 
    {
	act("But you only feel like following $N!", ch, 0, ch->master, TO_CHAR, 0);
    } else { /* Not Charmed follow person */

	if (leader == ch) {
	    if (!ch->master) {
		send_to_char("You are already following yourself.\n\r", ch);
		return eFAILURE;
	    }
	    stop_follower(ch, STOP_FOLLOW);
	} else {
	    if (circle_follow(ch, leader)) {
		act("Sorry, but following in 'loops' is not allowed.",
		    ch, 0, 0, TO_CHAR, 0);
		return eFAILURE;
	    }
	    if (ch->master) { 
	      if(cmd == 10) stop_follower(ch, END_STALK);  /* stalk  */
              else          stop_follower(ch, STOP_FOLLOW);  /* follow */ 
            }

//	    if((abs(GET_LEVEL(ch)-GET_LEVEL(leader))<60) || GET_LEVEL(ch)>=IMMORTAL) { 
	      if(cmd == 10) add_follower(ch, leader, 1); /* stalk  */
              else          add_follower(ch, leader, 0); /* follow */ 
  //          }
//	    else
//	      {
//		act("Sorry, but you are not of the right caliber to follow.",
//		ch, 0, 0, TO_CHAR, 0);
//		return eFAILURE;
//	      }
	  }
      }
  return eSUCCESS;
}

int do_autojoin(CHAR_DATA *ch, char *argument, int cmd)
{
  if (!ch->pcdata) return eFAILURE;
  char buf[MAX_STRING_LENGTH];
  if (!*argument)
  {
	sprintf(buf, "You are currently joining: ");
	if (ch->pcdata->joining)
		strcat(buf, ch->pcdata->joining);
	else
		strcat(buf, "Nobody");
	strcat(buf, "\r\n");
	send_to_char(buf,ch);

	send_to_char("Syntax: autojoin <player1> <player2> <player3> ...\r\nExample: autojoin Urizen Apocalypse Wendy Scyld\r\n",ch);
	
	return eFAILURE;
  }
  char tmp[MAX_STRING_LENGTH],tmp2[MAX_STRING_LENGTH],tmp3[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  tmp[0] = tmp2[0] = tmp3[0] = '\0';

  if (ch->pcdata->joining) { 
    strcpy(tmp, ch->pcdata->joining);
    dc_free(ch->pcdata->joining);
    ch->pcdata->joining = 0;
  }
  
  while (1) {

  // worst logic ever, but I don't feel like thinking and it's not like we need the processing power

   argument = one_argument(argument,arg);
   if (arg[0] == '\0') break;
   if (!str_cmp(GET_NAME(ch), arg))
   {
	send_to_char("You cannot autojoin yourself.\r\n",ch);
	continue;
   }
   if (!str_cmp(arg, "clear"))
   {
	send_to_char("Joinlist cleared.\r\n",ch);
	return eSUCCESS;
   }

   if (isname(arg,tmp))
   {
	strcat(tmp3, arg);
	strcat(tmp3, " ");
   } else {
	strcat(tmp2, arg);
	strcat(tmp2, " ");
   }
  }
  argument = &tmp[0];
  while (1) { // BAD BAD BAD
   argument = one_argument(argument,arg);
   if (arg[0] == '\0') break;
   if (!isname(arg,tmp3))
   {
	strcat(tmp2, arg);
	strcat(tmp2, " ");
   }
  }
  if (strlen(tmp2) > 200)
  {
     send_to_char("You cannot keep track of that many people.\r\n",ch);
     return eFAILURE; 
  }

  ch->pcdata->joining = str_dup(tmp2);
  sprintf(buf, "You are now autojoining: %s\r\n",ch->pcdata->joining);
  send_to_char(buf,ch);
  return eSUCCESS;
}
