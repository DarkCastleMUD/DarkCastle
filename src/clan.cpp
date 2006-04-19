/* $Id: clan.cpp,v 1.44 2006/04/19 18:59:47 dcastle Exp $ */

/***********************************************************************/
/* Revision History                                                    */
/* 11/10/2003    Onager     Removed clan size limit                    */
/***********************************************************************/

extern "C"
{
  #include <string.h> // strcat
  #include <stdio.h> // FILE *
  #include <ctype.h> // isspace..
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <db.h> // real_room
#include <player.h>
#include <utility.h>
#include <character.h>
#include <connect.h> // descriptor_data
#include <mobile.h> // utility.h stuff
#include <clan.h>   // duh
#include <interp.h> // do_outcast, etc..
#include <levels.h> // levels
#include <handler.h> // get_char_room_vis
#include <terminal.h> // get_char_room_vis
#include <room.h> // CLAN_ROOM flag
#include <returnvals.h>
#include <spells.h>

extern CHAR_DATA *character_list;
extern struct descriptor_data *descriptor_list;
extern CWorld world;
extern struct zone_data *zone_table;
extern void send_info(char *messg);

struct clan_data * clan_list = 0;
struct clan_data * end_clan_list = 0;

#define MAX_CLAN_DESC_LENGTH 1022

void add_clan(struct clan_data * new_new_clan);
void add_clan_member(struct clan_data * theClan, struct clan_member_data * new_new_member);
void add_clan_member(struct clan_data * theClan, struct char_data * ch);
void remove_clan_member(struct clan_data * theClan, struct char_data * ch);
void remove_clan_member(int clannumber, struct char_data * ch);
void free_member(struct clan_member_data * member);
struct clan_member_data * get_member(char * strName, int nClanId);

char * clan_rights[] = {
   "accept",
   "outcast",
   "read",
   "write",
   "remove",
   "member",
   "rights",
   "messages",
   "info",
   "tax",
   "withdraw",
   "channel",
   "area",
   "\n"
};


void boot_clans(void)
{
  FILE * fl;
  char buf[1024];
  struct clan_data * new_new_clan = NULL;
  struct clan_room_data * new_new_room = NULL;
  struct clan_member_data * new_new_member = NULL;
  int tempint;

  if(!(fl = dc_fopen("../lib/clan.txt", "r"))) {
    fprintf(stderr, "Unable to open clan file...\n");
    fl = dc_fopen("../lib/clan.txt", "w");
    fprintf(fl, "~\n");
    dc_fclose(fl);
    abort();
  }

  char a;
  while((a = fread_char(fl)) != '~') {
   ungetc(a, fl);
#ifdef LEAK_CHECK
    new_new_clan = (struct clan_data *)calloc(1, sizeof(struct clan_data));
#else
    new_new_clan = (struct clan_data *)dc_alloc(1, sizeof(struct clan_data));
#endif
    new_new_clan->next = 0;
    new_new_clan->tax = 0;
    new_new_clan->balance = 0;
    new_new_clan->email = NULL;
    new_new_clan->description = NULL;
    new_new_clan->login_message = NULL;
    new_new_clan->death_message = NULL;
    new_new_clan->logout_message = NULL;
    new_new_clan->rooms = NULL;
    new_new_clan->members = NULL;

//    sscanf(buf, "%s %s %s %hd", new_new_clan->leader, new_new_clan->founder,
//           new_new_clan->name, &new_new_clan->number);

    new_new_clan->leader = fread_word(fl, 1);
    new_new_clan->founder = fread_word(fl, 1);
    new_new_clan->name = fread_word(fl, 1);
    new_new_clan->number = fread_int(fl,0, LONG_MAX);
    // read in clan rooms
//    fgets(buf, 198, fl);
     char b;
    while(TRUE) /* I see clan rooms! */
    {
	b = fread_char(fl);
      if (b == 'S') break;
      if (b != 'R') continue;
#ifdef LEAK_CHECK
      new_new_room = (struct clan_room_data *)calloc(1, sizeof(struct clan_room_data));
#else
      new_new_room = (struct clan_room_data *)dc_alloc(1, sizeof(struct clan_room_data));
#endif
      new_new_room->next = new_new_clan->rooms;
      tempint = fread_int(fl, 0, 50000);
      new_new_room->room_number = (int16) tempint;
      new_new_clan->rooms = new_new_room;
    }

//    fgets(buf, 198, fl);
    char a;
    
    while( (a = fread_char(fl)) != '~' )
    {
	if (a != ' ' && a != '\n')
	getc(fl);
      switch(a) {
	case ' ':
	case '\n':
	  break;
        case 'E': {
          new_new_clan->email = fread_string(fl, 0);
          break;
        }
        case 'D': {
          new_new_clan->description = fread_string(fl, 0);
          break;
        }
        case 'C': {
          new_new_clan->clanmotd = fread_string(fl, 0);
          break;
        }
	case 'B': { // Account balance
	   new_new_clan->balance = fread_int(fl,0,LONG_MAX);
	   break;
	}
	case 'T': { // Tax
	  new_new_clan->tax = fread_int(fl,0,99);
	  break;
	}
        case 'L': {
          new_new_clan->login_message = fread_string(fl, 0);
          break;
        }
        case 'X': {
          new_new_clan->death_message = fread_string(fl, 0);
          break;
        }
        case 'O': {
          new_new_clan->logout_message = fread_string(fl, 0);
          break;
        }
        case 'M': { // read a member
#ifdef LEAK_CHECK
          new_new_member = (struct clan_member_data *)calloc(1, sizeof(struct clan_member_data));
#else
          new_new_member = (struct clan_member_data *)dc_alloc(1, sizeof(struct clan_member_data));
#endif
          new_new_member->member_name = fread_string(fl, 0);
	  new_new_member->member_rights = fread_int(fl, 0, LONG_MAX);
          new_new_member->member_rank = fread_int(fl, 0, LONG_MAX);
          new_new_member->unused1 = fread_int(fl, 0, LONG_MAX);
          new_new_member->unused2 = fread_int(fl, 0, LONG_MAX);
          new_new_member->unused3 = fread_int(fl, 0, LONG_MAX);
          new_new_member->time_joined = fread_int(fl, 0, LONG_MAX);
          new_new_member->unused4 = fread_string(fl, 0);

          // add it to the member linked list
          add_clan_member(new_new_clan, new_new_member);
          break;
        }
        default: log("Illegal switch hit in boot_clans.", 0, LOG_MISC);
                 log(buf, 0, LOG_MISC);
          break;
      }
    }
    add_clan(new_new_clan);
  }

  dc_fclose(fl);
}

void save_clans(void)
{
  FILE * fl;
  struct clan_data * pclan = NULL;
  struct clan_room_data * proom = NULL;
  struct clan_member_data * pmember = NULL;
  char buf[MAX_STRING_LENGTH];
  char * x;
  char * targ;

  if(!(fl = dc_fopen("../lib/clan.txt", "w"))) {
    fprintf(stderr, "Unable to open clan.txt for writing.\n");
    abort();
  }
  
  for(pclan = clan_list; pclan; pclan = pclan->next) {
     // print normal data
     fprintf(fl, "%s %s %s %d\n", pclan->leader, pclan->founder,
             pclan->name, pclan->number);
     // print rooms
     for(proom = pclan->rooms; proom; proom = proom->next)
        fprintf(fl, "R %d\n", proom->room_number);
     fprintf(fl, "S\n");

     // BLAH TEMP CODE HERE
     targ = buf;
     for(x = pclan->email; x && *x != '\0'; x++)
        if( *x != '\r' )
           *targ++ = *x;
     *targ = '\0';
     // handle email
     if(pclan->email)
       fprintf(fl, "E\n%s~\n", buf);
     //  fprintf(fl, "E\n%s~\n", pclan->email);

     // BLAH TEMP CODE THIS BLOWS
     // What's happening is apparently fedora's fprintf doesn't strip out
     // \r's like Redhat's does.  So we're writing \n\r to files.  This is
     // bad because when we read it in, fread_string replaces \n with a
     // \n\r.  So we get \n\r\r.   After a while, this is really bad.
     // So this is some crap code to strip out \r's before we save
     // I just did this REALLY fast so please rewrite this
     // Does it for L X O too but C and D were the hardcore ones so i temp 
     // fixed those since those are the ones that got long enough to crash us
     targ = buf;
     for(x = pclan->description; x && *x != '\0'; x++)
        if( *x != '\r' ) 
           *targ++ = *x;
     *targ = '\0';
     
     // handle description
     if(pclan->description)
       fprintf(fl, "D\n%s~\n", buf);
//       fprintf(fl, "D\n%s~\n", pclan->description);


     // BLAH TEMP CODE HERE
     targ = buf;
     for(x = pclan->login_message; x && *x != '\0'; x++)
        if( *x != '\r' )
           *targ++ = *x;
     *targ = '\0';

     if(pclan->login_message)
       fprintf(fl, "L\n%s~\n", buf);
     //  fprintf(fl, "L\n%s~\n", pclan->login_message);

     if (pclan->tax)
	fprintf(fl, "T\n%d\n", pclan->tax);

     if (pclan->balance)
	fprintf(fl, "B\n%ld\n", pclan->balance);

     // BLAH TEMP CODE HERE
     targ = buf;
     for(x = pclan->death_message; x && *x != '\0'; x++)
        if( *x != '\r' )
           *targ++ = *x;
     *targ = '\0';
     if(pclan->death_message)
       fprintf(fl, "X\n%s~\n", buf);
      // fprintf(fl, "X\n%s~\n", pclan->death_message);
     
     // BLAH TEMP CODE HERE
     targ = buf;
     for(x = pclan->logout_message; x && *x != '\0'; x++)
        if( *x != '\r' )
           *targ++ = *x;
     *targ = '\0';
     if(pclan->logout_message)
      fprintf(fl, "O\n%s~\n", buf);
      // fprintf(fl, "O\n%s~\n", pclan->logout_message);

     // BLAH TEMP CODE HERE
     targ = buf;
     for(x = pclan->clanmotd; x && *x != '\0'; x++)
        if( *x != '\r' ) 
           *targ++ = *x;
     *targ = '\0';

     if(pclan->clanmotd)
       fprintf(fl, "C\n%s~\n", buf);
//       fprintf(fl, "C\n%s~\n", pclan->clanmotd);

     for(pmember = pclan->members; pmember; pmember = pmember->next) {
       fprintf(fl, "M\n%s~\n", pmember->member_name);
       fprintf(fl, "%d %d %d %d %d %d\n",
                      pmember->member_rights, pmember->member_rank,
                      pmember->unused1,       pmember->unused2, 
                      pmember->unused3,       pmember->time_joined);
       fprintf(fl, "%s~\n", pmember->unused4);
     }

     // terminate clan
     fprintf(fl, "~\n");
  }  
  fprintf(fl, "~\n");
  dc_fclose(fl);
}

void free_clans_from_memory()
{
  struct clan_data * currclan = NULL;
  struct clan_data * nextclan = NULL;
  struct clan_room_data * curr_room = NULL;
  struct clan_room_data * next_room = NULL;
  struct clan_member_data * curr_member = NULL;
  struct clan_member_data * next_member = NULL;

  for(currclan = clan_list; currclan; currclan = nextclan)
  {
    nextclan = currclan->next;

    for(curr_room = currclan->rooms; curr_room; curr_room = next_room)
    {
      next_room = curr_room->next;
      dc_free(curr_room);
    }
    for(curr_member = currclan->members; curr_member; curr_member = next_member)
    {
      next_member = curr_member->next;
      free_member(curr_member);
    }
    if(currclan->description)
      dc_free(currclan->description);
    if(currclan->email)
      dc_free(currclan->email);
    if(currclan->login_message)
      dc_free(currclan->login_message);
    if(currclan->logout_message)
      dc_free(currclan->logout_message);
    if(currclan->death_message)
      dc_free(currclan->death_message);

    dc_free(currclan);
  }
}

void assign_clan_rooms()
{
   struct clan_data * clan = 0;
   struct clan_room_data * room = 0;

   for(clan = clan_list; clan; clan = clan->next)
      for(room = clan->rooms; room; room = room->next)
         if(-1 != real_room(room->room_number))
           if(!IS_SET(world[real_room(room->room_number)].room_flags, CLAN_ROOM))
              SET_BIT(world[real_room(room->room_number)].room_flags, CLAN_ROOM);

}

struct clan_member_data * get_member(char * strName, int nClanId)
{
  struct clan_data * theClan = NULL;

  if(!(theClan = get_clan(nClanId)) || !strName)
    return NULL;

  struct clan_member_data * pcurr = theClan->members;

  while(pcurr && strcmp(pcurr->member_name, strName))
    pcurr = pcurr->next;

  return pcurr;    
}

int is_in_clan(struct clan_data * theClan, struct char_data * ch) 
{
  struct clan_member_data * pcurr = theClan->members;

  while(pcurr) {
    if(!strcmp(pcurr->member_name, GET_NAME(ch)))
      return 1;
    pcurr = pcurr->next;
  }

  return 0;
}

void remove_clan_member(int clannumber, struct char_data * ch)
{
   struct clan_data * pclan = NULL;
   
   if(!(pclan = get_clan(clannumber)))
     return;

   remove_clan_member(pclan, ch);
}

void remove_clan_member(struct clan_data * theClan, struct char_data * ch)
{
   struct clan_member_data * pcurr = NULL;
   struct clan_member_data * plast = NULL;

   pcurr = theClan->members;

   while(pcurr && strcmp(pcurr->member_name, GET_NAME(ch))) {
     plast = pcurr;
     pcurr = pcurr->next;
   }

   if(!pcurr) // didn't find it
     return;

   if(!plast)                       // was first one in list
     theClan->members = pcurr->next;
   else                             // somewhere in the list
     plast->next = pcurr->next;

   free_member(pcurr);
}

// Add someone.  Just makes the struct, fills it, then calls the other add_clan_member
void add_clan_member(struct clan_data * theClan, struct char_data * ch) 
{
  struct clan_member_data * pmember = NULL;

  if(!ch || !theClan) {
    log("add_clan_member(clan, ch) called with a null.", ANGEL, LOG_BUG);
    return;
  }

#ifdef LEAK_CHECK
  pmember = (struct clan_member_data *)calloc(1, sizeof(struct clan_member_data));
#else
  pmember = (struct clan_member_data *)dc_alloc(1, sizeof(struct clan_member_data));
#endif

  pmember->member_name   = str_dup(GET_NAME(ch));
  pmember->member_rights = 0;
  pmember->member_rank   = 0;
  pmember->unused1       = 0;
  pmember->unused2       = 0;
  pmember->unused3       = 0;
  pmember->time_joined   = 0;
  pmember->unused4       = str_dup("");
  pmember->next          = NULL;

  add_clan_member(theClan, pmember);
}

// This should really be done as a binary tree, but I'm lazy, and this doesn't get used
// very often, so it's just a linked list sorted by member name
void add_clan_member(struct clan_data * theClan, struct clan_member_data * new_new_member)
{
  struct clan_member_data * pcurr = NULL;
  struct clan_member_data * plast = NULL;
  int result = 0;

  if(!new_new_member || !theClan) {
    log("add_clan_member(clan, member) called with a null.", ANGEL, LOG_BUG);
    return;
  }

  if(!*new_new_member->member_name) {
    log("Attempt to add a blank member name to a clan.", ANGEL, LOG_BUG);
    return;
  }

  if(!theClan->members) {
    theClan->members = new_new_member;
    new_new_member->next = NULL;
    return;
  }

  pcurr = theClan->members;

  while(pcurr && (0 > (result = strcmp(pcurr->member_name, new_new_member->member_name)))) {
    plast = pcurr;
    pcurr = pcurr->next;
  }

  if(result == 0) { // found um, get out
    log("Tried to add already existing clan member.  Possible memory leak.", ANGEL, LOG_BUG);
    return;
  }

  if(pcurr && !plast) { // we're at the beginning
    new_new_member->next = theClan->members;
    theClan->members = new_new_member;
    return;
  }

  if(!pcurr) { // we hit the end of the list
    plast->next = new_new_member;
    new_new_member->next = NULL;
    return;
  }

  // if we hit here, then we found our insertion point
  new_new_member->next = pcurr;
  plast->next = new_new_member;
}

void add_clan(struct clan_data * new_new_clan)
{
  struct clan_data * pcurr = NULL;
  struct clan_data * plast = NULL;

  if(!clan_list) {
    clan_list = new_new_clan;
    end_clan_list = new_new_clan;
    return;
  }

  plast = clan_list;
  pcurr = clan_list->next;

  while(pcurr) 
    if(pcurr->number > new_new_clan->number)
    {
      plast->next = new_new_clan;
      new_new_clan->next = pcurr;
      return;
    }
    else {
      plast = pcurr;
      pcurr = pcurr->next;
    }

  end_clan_list->next = new_new_clan;
  end_clan_list = new_new_clan;
}

void free_member(struct clan_member_data * member) 
{
  dc_free(member->member_name);
  dc_free(member->unused4);
  dc_free(member);
}

void delete_clan(struct clan_data * dead_clan)
{
  struct clan_data * last = 0;
  struct clan_data * curr = 0;
  struct clan_room_data * room = 0;
  struct clan_room_data * nextroom = 0;

  if(!clan_list) return;
  if(!dead_clan) return;

  if(clan_list == dead_clan) {
    if(dead_clan == end_clan_list) // Only 1 clan total
      end_clan_list = 0;
    clan_list = dead_clan->next;
    dc_free(dead_clan);
    return;
  }

  // This works since the first clan is not the dead_clan
  curr = clan_list;
  while(curr) 
    if(curr == dead_clan) {
      last->next = curr->next;
      if((curr = end_clan_list))
         end_clan_list = last;
      if(dead_clan->rooms) {
        room = dead_clan->rooms;
        nextroom = room->next;
        dc_free(room);
        while(nextroom) {
          room = nextroom;
          nextroom = room->next;
          dc_free(room);
        }
      }
      if(dead_clan->email)
        dc_free(dead_clan->email);
      if(dead_clan->login_message)
        dc_free(dead_clan->login_message);
      if(dead_clan->logout_message)
        dc_free(dead_clan->logout_message);
      if(dead_clan->death_message)
        dc_free(dead_clan->death_message);
      if(dead_clan->description)
        dc_free(dead_clan->description);
      dc_free(dead_clan);
      return;
    } 
    else {
      last = curr;
      curr = curr->next;
    }
}

// see if ch has rights to 'bit' in his clan
int has_right(struct char_data * ch, uint32 bit)
{
  struct clan_member_data * pmember = NULL;
  
  if(!ch || !(pmember = get_member(GET_NAME(ch), ch->clan)))
    return FALSE;

  return IS_SET(pmember->member_rights, bit);  
}

int num_clan_members(struct clan_data * clan)
{
  int i = 0;
  for(struct clan_member_data * pmem = clan->members;
      pmem;
      pmem = pmem->next)
    i++;

  return i;
}

struct clan_data * get_clan(int nClan)
{
  struct clan_data *clan = NULL;

  for(clan = clan_list; clan; clan = clan->next)
     if(nClan == clan->number)
       return clan;

  return 0;
}

struct clan_data * get_clan(CHAR_DATA *ch)
{
  struct clan_data *clan;

  for(clan = clan_list; clan; clan = clan->next)
     if(ch->clan == clan->number)
       return clan;

  ch->clan = 0;
  return 0;
}

void message_to_clan(char_data *ch, char buf[])
{
  char_data * pch;

  for (descriptor_data* d = descriptor_list; d; d = d->next) {
    if (d->connected || !(pch = d->character))
      continue;
    if (pch->clan != ch->clan || pch == ch)
      continue;
  
    ansi_color (YELLOW, pch);
    send_to_char ("-->> ", pch);
    ansi_color (RED, pch);
    ansi_color (BOLD, pch);
    send_to_char (buf, pch);
    ansi_color (NTEXT, pch);
    ansi_color (YELLOW, pch);
    send_to_char (" <<--\n\r", pch);
    ansi_color (GREY, pch);
  }
}

void clan_death (char_data *ch, char_data *killer)
{
  if (!ch || ch->clan == 0) return;
    
  char buf[400];    
  char secondbuf[400];    
  clan_data * clan;
  char * curr = NULL;

  if(!(clan = get_clan(ch->clan)))  {
    send_to_char("You have an illegal clan number.  Contact a god.\r\n", ch);
    return;
  }

  if(!clan->death_message)
    return;

  // Don't give away any imms listening in
  if(GET_LEVEL(ch) >= IMMORTAL) 
    return;

  if(!(curr = strstr(clan->death_message, "%")))
  {
    send_to_char("Error:  clan with illegal death_message.  Contact a god.\r\n", ch);
    return;
  }

  *curr = '\0';
  sprintf(buf, "%s%s%s", clan->death_message, GET_NAME(ch), curr+1);
  *curr = '%';

  if(!(curr = strstr(buf, "#")))
  {
    send_to_char("Error:  clan with illegal death_message.  Contact a god.\r\n", ch);
    return;
  }

  *curr = '\0';
  sprintf(secondbuf, "%s%s%s", buf, (killer ? GET_SHORT(killer) : "unknown"), curr+1);

  message_to_clan(ch, secondbuf);
}

void clan_login (char_data *ch)
{
  if (ch->clan == 0) return;

  char buf[400];    
  clan_data * clan;
  char * curr = NULL;

  if(!(clan = get_clan(ch->clan)))
  {
    // illegal clan number.  Set him to 0 and get out
    ch->clan = 0;
    return;
  }

  // Don't give away any imms listening in
  if(GET_LEVEL(ch) >= IMMORTAL)
  {
    return;
  }

  if(!is_in_clan(clan, ch)) {
    send_to_char("You were kicked out of your clan.\n\r", ch);
    ch->clan = 0;
    return;
  }

  if(!clan->login_message)
    return;

  if(!(curr = strstr(clan->login_message, "%")))
  {
    send_to_char("Error:  clan with illegal login_message.  Contact a god.\r\n", ch);
    return;
  }

  *curr = '\0';
  sprintf(buf, "%s%s%s", clan->login_message, GET_NAME(ch), curr+1);
  *curr = '%';

  message_to_clan(ch, buf);
}

void clan_logout (char_data *ch)
{
  if (ch->clan == 0) return;
    
  char buf[400];
  clan_data * clan;
  char * curr = NULL;

  if(!(clan = get_clan(ch->clan)))
  {
    send_to_char("You have an illegal clan number.  Contact a god.\r\n", ch);
    return;
  }

  // Don't give away any imms listening in
  if(GET_LEVEL(ch) >= IMMORTAL)
  {
    return;
  }

  if(!clan->logout_message)
    return;

  if(!(curr = strstr(clan->logout_message, "%")))
  {
    send_to_char("Error:  clan with illegal logout_message.  Contact a god.\r\n", ch);
    return;
  }

  *curr = '\0';
  sprintf(buf, "%s%s%s", clan->logout_message, GET_NAME(ch), curr+1);
  *curr = '%';

  message_to_clan(ch, buf);
}

int do_accept(CHAR_DATA *ch, char *arg, int cmd)
{
  CHAR_DATA *victim;
  struct clan_data *clan;
  char buf[MAX_STRING_LENGTH];

  while(isspace(*arg))
    arg++;

  if(!*arg) {
    send_to_char("Accept who into your clan?\n\r", ch);
    return eFAILURE;
  }

  if(!ch->clan || !(clan = get_clan(ch))) {
    send_to_char("You aren't the member of any clan!\n\r", ch);
    return eFAILURE;
  }

  if(strcmp(clan->leader, GET_NAME(ch)) && !has_right(ch, CLAN_RIGHTS_ACCEPT)) {
    send_to_char("You aren't the leader of your clan!\n\r", ch);
    return eFAILURE;
  }

  one_argument(arg, buf);

  if(!(victim = get_char_room_vis(ch, buf))) {
    send_to_char("You can't accept someone into your clan who isn't here!\n\r", ch);
    return eFAILURE;
  }

  if(IS_NPC(victim) || GET_LEVEL(victim) >= IMMORTAL) {
    send_to_char("Yeah right.\n\r", ch);
    return eFAILURE;
  }

  if(victim->clan) {
    send_to_char("This person already belongs to a clan.\n\r", ch);
    return eFAILURE;
  }

  victim->clan = ch->clan;
  add_clan_member(clan, victim);
  save_clans();
  sprintf(buf, "You are now a member of %s.\n\r", clan->name);
  send_to_char("Your clan now has a new member.\n\r", ch);
  send_to_char(buf, victim); 

  sprintf(buf, "%s just joined clan [%s].", GET_NAME(victim), clan->name);
  log(buf, 110, LOG_CLAN);
  return eSUCCESS;
}


int do_outcast(CHAR_DATA *ch, char *arg, int cmd)
{
  CHAR_DATA *victim;
  struct clan_data *clan;
  struct descriptor_data d;
  char buf[MAX_STRING_LENGTH], tmp_buf[MAX_STRING_LENGTH];
  bool connected = TRUE;

  while(isspace(*arg))
    arg++;

  if(!*arg) {
    send_to_char("Cast who out of your clan?\n\r", ch);
    return eFAILURE;
  }

  if(!ch->clan || !(clan = get_clan(ch))) {
    send_to_char("You aren't the member of any clan!\n\r", ch);
    return eFAILURE;
  }

  one_argument(arg, buf);

  buf[0] = UPPER(buf[0]);
  if(!(victim = get_pc_vis_exact(ch, buf))) {

    // must be done to clear out "d" before it is used
    memset((char *) &d, 0, sizeof(struct descriptor_data));

    if(!(load_char_obj(&d, buf))) {
      
      sprintf(tmp_buf, "../archive/%s.gz", buf);
      if(file_exists(tmp_buf))
         send_to_char("Character is archived.\r\n", ch);
      else send_to_char("Unable to outcast, type the entire name.\n\r", ch);
      return eFAILURE; 
    }

    victim = d.character;
    victim->desc = 0;

    victim->hometown = 3001;
    victim->in_room = 3001;
    connected = FALSE;    
  }

  if(!victim->clan) {
    send_to_char("This person isn't in a clan in the first place...\n\r", ch);
    return eFAILURE;
  }

  if(strcmp(clan->leader, GET_NAME(ch)) && 
     victim != ch &&
     !has_right(ch, CLAN_RIGHTS_OUTCAST)) {
    send_to_char("You don't have the right to outcast people from your clan!\n\r", ch);
    return eFAILURE;
  }

  if(!strcmp(clan->leader, GET_NAME(victim))) {
    send_to_char("You can't outcast the clan leader!\n\r", ch);
    return eFAILURE;
  }

  if(victim == ch) {
    sprintf(buf, "%s just quit clan [%s].", GET_NAME(victim), clan->name);
    log(buf, 110, LOG_CLAN);
    send_to_char("You quit your clan.\n\r", ch);
    victim->clan = 0;
    remove_clan_member(clan, ch);
    save_clans();
    return eSUCCESS;
  }

  if(victim->clan != ch->clan) {
    send_to_char("That person isn't in your clan!\r\n", ch);
    return eFAILURE;
  }

  victim->clan = 0;
  remove_clan_member(clan, victim);
  save_clans();
  sprintf(buf, "You cast %s out of your clan.\n\r", GET_NAME(victim));
  send_to_char(buf, ch);
  sprintf(buf, "You are cast out of %s.\n\r", clan->name);
  send_to_char(buf, victim); 

  sprintf(buf, "%s was outcasted from clan [%s].", GET_NAME(victim), clan->name);
  log(buf, 110, LOG_CLAN);

  do_save(victim,"",666);
  if(!connected) free_char(victim);

  return eSUCCESS;
}

int do_cpromote(CHAR_DATA *ch, char *arg, int cmd)
{
  CHAR_DATA *victim;
  struct clan_data *clan;
  char buf[MAX_STRING_LENGTH];

  while(isspace(*arg))
    arg++;

  if(!*arg) {
    send_to_char("Who do you want to make the new clan leader?\n\r", ch);
    return eFAILURE;
  }

  if(!ch->clan || !(clan = get_clan(ch))) {
    send_to_char("You aren't the member of any clan!\n\r", ch);
    return eFAILURE;
  }

  if(!isname(clan->leader, GET_NAME(ch))) {
    send_to_char("You aren't the leader of your clan!\n\r", ch);
    return eFAILURE;
  }

  one_argument(arg, buf);

  if(!(victim = get_char_room_vis(ch, buf))) {
    send_to_char("You can't cpromote someone who isn't here!\n\r",
                 ch);
    return eFAILURE;
  }

  if(IS_NPC(victim)) {
    send_to_char("Yeah right.\n\r", ch);
    return eFAILURE;
  }

  if(victim->clan != ch->clan) {
    send_to_char("You can not cpromote someone who doesn't belong to the "
                 "clan.\n\r", ch);
    return eFAILURE;
  }
  if (clan->leader) dc_free(clan->leader);
  clan->leader = str_dup(GET_NAME(victim));

  save_clans();
  
  sprintf(buf, "You are now the leader of %s.\n\r", clan->name);
  send_to_char("Your clan now has a new leader.\n\r", ch);
  send_to_char(buf, victim); 

  sprintf(buf, "%s just cpromoted by %s as leader of clan [%s].", GET_NAME(victim), GET_NAME(ch), clan->name);
  log(buf, 110, LOG_CLAN);
  return eSUCCESS;
}

int clan_desc(CHAR_DATA *ch, char *arg)
{
  struct clan_data * clan = 0;

  char buf[MAX_STRING_LENGTH];
  char text[MAX_INPUT_LENGTH];

  clan = get_clan(ch);
  arg = one_argumentnolow(arg, text);

  if(!strncmp(text, "delete", 6))
  {
    if(clan->description)
      dc_free(clan->description);
    clan->description = NULL;
    send_to_char("Clan description removed.\r\n", ch);
    return 1;
  }

  if(strcmp(text, "change"))
  {
    sprintf(buf, "$3Syntax$R:  clans description change\r\n\r\nCurrent description: %s\r\n",
            clan->description ? clan->description : "(No Description)");
    send_to_char(buf, ch);
    send_to_char("To not have any description use:  clans description delete\r\n", ch);
    return 0;
  }

/*  if(clan->description)
    dc_free(clan->description);
  clan->description = NULL;
*/
 // send_to_char("Write new description.  ~ to end.\r\n", ch);

//  ch->desc->connected = CON_EDITING;
//  ch->desc->str = &clan->description;
//  ch->desc->max_str = MAX_CLAN_DESC_LENGTH;
    ch->desc->backstr = NULL;
    send_to_char("        Write your description and stay within the line.  (/s saves /h for help)\r\n"
                 "   |--------------------------------------------------------------------------------|\r\n", ch);
    if (clan->description) {
       ch->desc->backstr = str_dup(clan->description);
       send_to_char(ch->desc->backstr, ch);
    }

    ch->desc->connected = CON_EDITING;
    ch->desc->strnew = &(clan->description);
    ch->desc->max_str = MAX_CLAN_DESC_LENGTH;
  return 1;
}

int clan_motd(CHAR_DATA *ch, char *arg)
{
  struct clan_data * clan = 0;

  char buf[MAX_STRING_LENGTH];
  char text[MAX_INPUT_LENGTH];

  clan = get_clan(ch);
  arg = one_argumentnolow(arg, text);

  if(!strncmp(text, "delete", 6))
  {
    if(clan->clanmotd)
      dc_free(clan->clanmotd);
    clan->clanmotd = NULL;
    send_to_char("Clan motd removed.\r\n", ch);
    return 1;
  }

  if(strcmp(text, "change"))
  {
    sprintf(buf, "$3Syntax$R:  clans motd change\r\n\r\nCurrent motd: %s\r\n",
            clan->clanmotd ? clan->clanmotd : "(No Motd)");
    send_to_char(buf, ch);
    send_to_char("To not have any motd use:  clans motd delete\r\n", ch);
    return 0;
  }

/*  if(clan->clanmotd)
    dc_free(clan->clanmotd);
  clan->clanmotd = NULL;
*/
 // send_to_char("Write new motd.  ~ to end.\r\n", ch);

 // ch->desc->connected = CON_EDITING;
 // ch->desc->str = &clan->clanmotd;
 // ch->desc->max_str = MAX_CLAN_DESC_LENGTH;

    ch->desc->backstr = NULL;
    send_to_char("        Write your motd and stay within the line.  (/s saves /h for help)\r\n"
                 "   |--------------------------------------------------------------------------------|\r\n", ch);
    if (clan->clanmotd) {
       ch->desc->backstr = str_dup(clan->clanmotd);
       send_to_char(ch->desc->backstr, ch);
    }

    ch->desc->connected = CON_EDITING;
    ch->desc->strnew = &(clan->clanmotd);
    ch->desc->max_str = MAX_CLAN_DESC_LENGTH;
  return 1;
}

int clan_death_message(CHAR_DATA *ch, char *arg)
{
  struct clan_data * clan = 0;

  char buf[MAX_STRING_LENGTH];

  clan = get_clan(ch);
  if(!*arg)
  {
    sprintf(buf, "$3Syntax$R:  clans death <new message>\r\n\r\nCurrent message: %s\r\n", 
                 clan->death_message);
    send_to_char(buf, ch);
    send_to_char("To not have any message use:  clans death delete\r\n", ch);
    return 0;
  }

  one_argument(arg, buf);
  if(!strncmp(buf, "delete", 6))
  {
    send_to_char("Clan death message removed.\r\n", ch);
    if(clan->death_message)
      dc_free(clan->death_message);
    clan->death_message = NULL;
    return 1;
  }

  if(strstr(arg, "~"))
  {
    send_to_char("No ~s fatt butt!\r\n", ch);
    return 0;
  }

  char * curr;

  if(!(curr = strstr(arg, "%")))
  {
    send_to_char("You must include a '%' to represent the victim's name.\r\n", ch);
    return 0;
  }

  if(strstr(curr+1, "%"))
  {
    send_to_char("You may only have one '%' in the message.\r\n", ch);
    return 0;
  }

  if(!(curr = strstr(arg, "#")))
  {
    send_to_char("You must include a '#' to represent the killer's name.\r\n", ch);
    return 0;
  }

  if(strstr(curr+1, "#"))
  {
    send_to_char("You may only have one '#' in the message.\r\n", ch);
    return 0;
  }

  if(clan->death_message)
    dc_free(clan->death_message);

  clan->death_message = str_dup(arg);

  sprintf(buf, "Clan death message changed to: %s\r\n", clan->death_message);
  send_to_char(buf, ch);
  return 1;
}

int clan_logout_message(CHAR_DATA *ch, char *arg)
{
  struct clan_data * clan = 0;

  char buf[MAX_STRING_LENGTH];

  clan = get_clan(ch);
  if(!*arg)
  {
    sprintf(buf, "$3Syntax$R:  clans logout <new message>\r\n\r\nCurrent message: %s\r\n", 
                 clan->logout_message);
    send_to_char(buf, ch);
    send_to_char("To not have any message use:  clans logout delete\r\n", ch);
    return 0;
  }

  one_argument(arg, buf);
  if(!strncmp(buf, "delete", 6))
  {
    send_to_char("Clan logout message removed.\r\n", ch);
    if(clan->logout_message)
      dc_free(clan->logout_message);
    clan->logout_message = NULL;
    return 1;
  }

  if(strstr(arg, "~"))
  {
    send_to_char("No ~s fatt butt!\r\n", ch);
    return 0;
  }

  char * curr;

  if(!(curr = strstr(arg, "%")))
  {
    send_to_char("You must include a '%' to represent the person's name.\r\n", ch);
    return 0;
  }

  if(strstr(curr+1, "%"))
  {
    send_to_char("You may only have one '%' in the message.\r\n", ch);
    return 0;
  }

  if(clan->logout_message)
    dc_free(clan->logout_message);

  clan->logout_message = str_dup(arg);

  sprintf(buf, "Clan logout message changed to: %s\r\n", clan->logout_message);
  send_to_char(buf, ch);
  return 1;
}

int clan_login_message(CHAR_DATA *ch, char *arg)
{
  struct clan_data * clan = 0;

  char buf[MAX_STRING_LENGTH];

  clan = get_clan(ch);
  if(!*arg)
  {
    sprintf(buf, "$3Syntax$R:  clans login <new message>\r\n\r\nCurrent message: %s\r\n", 
                 clan->login_message);
    send_to_char(buf, ch);
    send_to_char("To not have any message use:  clans login delete\r\n", ch);
    return 0;
  }

  one_argument(arg, buf);
  if(!strncmp(buf, "delete", 6))
  {
    send_to_char("Clan login message removed.\r\n", ch);
    if(clan->login_message)
      dc_free(clan->login_message);
    clan->login_message = NULL;
    return 1;
  }

  if(strstr(arg, "~"))
  {
    send_to_char("No ~s fatt butt!\r\n", ch);
    return 0;
  }

  char * curr;

  if(!(curr = strstr(arg, "%")))
  {
    send_to_char("You must include a '%' to represent the person's name.\r\n", ch);
    return 0;
  }

  if(strstr(curr+1, "%"))
  {
    send_to_char("You may only have one '%' in the message.\r\n", ch);
    return 0;
  }

  if(clan->login_message)
    dc_free(clan->login_message);

  clan->login_message = str_dup(arg);

  sprintf(buf, "Clan login message changed to: %s\r\n", clan->login_message);
  send_to_char(buf, ch);
  return 1;
}

int clan_email(CHAR_DATA *ch, char *arg)
{
  struct clan_data * clan = 0;

  char buf[MAX_STRING_LENGTH];
  char text[MAX_INPUT_LENGTH];

  clan = get_clan(ch);
  arg = one_argumentnolow(arg, text);
  if(!*text)
  {
    sprintf(buf, "$3Syntax$R:  clans email <new address>\r\n\r\nCurrent address: %s\r\n", clan->email);
    send_to_char(buf, ch);
    send_to_char("To not have any email use:  clans email delete\r\n", ch);
    return 0;
  }

  if(!strncmp(text, "delete", 6))
  {
    if(clan->email)
      dc_free(clan->email);
    clan->email = NULL;
    send_to_char("Clan email address removed.\r\n", ch);
    return 1;
  }

  if(strstr(text, "~") || strstr(text, "<") || strstr(text, ">") ||
     strstr(text, "&") || strstr(text, "$"))
  {
    send_to_char("We both know those characters aren't legal in email addresses....\r\n", ch);
    return 0;
  }

  if(clan->email)
    dc_free(clan->email);

  clan->email = str_dup(text);

  sprintf(buf, "Clan email changed to: %s\r\n", clan->email);
  send_to_char(buf, ch);
  return 1;
}

int do_ctell(CHAR_DATA *ch, char *arg, int cmd)
{
  CHAR_DATA * pch;
  struct descriptor_data * desc;
  char buf[MAX_STRING_LENGTH];
  
  if(!ch->clan) {
    send_to_char("But you don't belong to a clan!\n\r", ch);
    return eFAILURE;
  }

  while(isspace(*arg))
    arg++;
  if (!has_right(ch, CLAN_RIGHTS_CHANNEL) && ch->level < 51)
  {
     send_to_char("You don't have the right to talk to your clan.\r\n",ch);
     return eFAILURE;
  }

  if(!(IS_SET(ch->misc, CHANNEL_CLAN))) {
    send_to_char("You have that channel off!!\n\r", ch);
    return eFAILURE;
  }
    
  if(!*arg) {
    send_to_char("Tell your clan members what?\n\r", ch);
    return eFAILURE;
  }

  sprintf(buf, "You tell the clan, '%s'\n\r", arg);
  ansi_color( GREEN, ch);
  send_to_char(buf, ch);
  ansi_color( NTEXT, ch);

  sprintf(buf, "%s tells the clan, '%s'\n\r", GET_SHORT(ch), arg);
  for(desc = descriptor_list; desc; desc = desc->next) {
     if(desc->connected || !(pch = desc->character))
       continue;
     if(pch == ch || pch->clan != ch->clan || 
        !IS_SET(pch->misc, CHANNEL_CLAN))
       continue;
     if (!has_right(pch, CLAN_RIGHTS_CHANNEL) && pch->level < 51)
       continue;
     ansi_color( GREEN, pch);
     send_to_char(buf, pch);
     ansi_color( NTEXT, pch);
  }
  return eSUCCESS;
}

void do_clan_list(CHAR_DATA *ch)
{
  struct clan_data * clan = 0;
  char buf[MAX_STRING_LENGTH];

  send_to_char("Clan                 Leader\n\r", ch);
  
  for(clan = clan_list; clan; clan = clan->next) {
     sprintf(buf, "%-20s$R %-16s %d", clan->name, clan->leader, 
           clan->number);
     if (GET_LEVEL(ch) > 103)
     {
       sprintf(buf,"%s  Balance: %ld Tax: %d\r\n",buf,clan->balance, clan->tax);
     } else {
	sprintf(buf,"%s\r\n",buf);
     }
     send_to_char(buf, ch);
  }
}

void do_clan_member_list(CHAR_DATA *ch)
{
  struct clan_member_data * pmember = 0;
  struct clan_data * pclan = 0;
  int column = 1;
  char buf[200];

  if(!(pclan = get_clan(ch->clan))) {
    send_to_char("Error:  Not in clan.  Contact a god.\n\r", ch);
    return;
  }

  send_to_char("Members of clan:\n\r", ch);
  sprintf(buf, "  ");

  for(pmember = pclan->members; pmember; pmember = pmember->next) {
    sprintf(buf, "%s%-20s  ", buf, pmember->member_name);
    if(0 == (column % 3)) {
      send_to_char(buf, ch);
      send_to_char("\n\r", ch);
      sprintf(buf, "  ");
      column = 0;
    }
    column++;
  }

  if(column != 0) {
    send_to_char(buf, ch);
    send_to_char("\n\r", ch);
  }
}

int is_clan_leader(CHAR_DATA * ch)
{
  struct clan_data * pclan = NULL;

  if(!ch || !(pclan = get_clan(ch->clan))) 
    return 0;

  return (!(strcmp(GET_NAME(ch), pclan->leader)));
}

void do_clan_rights(CHAR_DATA * ch, char * arg)
{
  struct clan_member_data * pmember = NULL;
  struct char_data * victim = NULL;
  //extern char * clan_rights[]~;

  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char name[MAX_INPUT_LENGTH];
  char last[MAX_INPUT_LENGTH];
  int bit = -1;

  half_chop(arg, name, last);

  if(!*name) {
    send_to_char("$3Syntax$R:  clan rights <member> [right]\n\r", ch);
    return;
  }

  *name = toupper(*name);

  if(!(pmember = get_member(name, ch->clan))) {
    sprintf(buf, "Could not find '%s' in your clan.\n\r", name);
    send_to_char(buf, ch);
    return;
  }

  if(!*last) { // diag
    sprintf(buf, "Rights for %s:\n\r-------------\n\r", pmember->member_name);
    send_to_char(buf, ch);
    for(bit = 0; *clan_rights[bit] != '\n'; bit++) {
      sprintf(buf, "  %-15s %s\n\r", clan_rights[bit], (IS_SET(pmember->member_rights, 1<<bit) ? "on" : "off"));
      send_to_char(buf, ch);
    }
    return;
  }
  
  bit = old_search_block(last, 0, strlen(last), clan_rights, 1);
  
  if(bit < 0) {
    send_to_char("Right not found.\r\n", ch);
    return;
  }
  bit--;

  if(!is_clan_leader(ch) && !has_right(ch, 1<<bit)) {
    send_to_char("You can't give out rights that you don't have.\n\r", ch);
    return;
  }

  TOGGLE_BIT(pmember->member_rights, 1<<bit);

  if(IS_SET(pmember->member_rights, 1<<bit)) {
    sprintf(buf, "%s toggled on.\n\r", clan_rights[bit]);
    sprintf(buf2, "%s has given you '%s' rights within your clan.\n\r", GET_SHORT(ch), clan_rights[bit]);
  } else {
    sprintf(buf, "%s toggled off.\n\r", clan_rights[bit]);
    sprintf(buf2, "%s has taken away '%s' rights within your clan.\n\r", GET_SHORT(ch), clan_rights[bit]);
  }
  send_to_char(buf, ch);

  if((victim = get_char(pmember->member_name))) {
    send_to_char(buf2, victim);
  }

  save_clans();
}

void do_god_clans(CHAR_DATA *ch, char *arg, int cmd)
{
  struct clan_data * clan = 0;
  struct clan_data * tarclan = 0;
  struct clan_room_data * newroom = 0;
  struct clan_room_data * lastroom = 0;

  char buf[MAX_STRING_LENGTH];
  char select[MAX_INPUT_LENGTH];
  char text[MAX_INPUT_LENGTH];
  char last[MAX_INPUT_LENGTH];

  int i, x;
  int16 skill;

  char *god_values[] = {
          "create", "rename", "leader", "delete", "addroom",
          "list", "save", "showrooms", "killroom", "email", 
          "description", "login", "logout", "death", "members", 
          "rights", "motd", "\n"
  };

  arg = one_argumentnolow(arg, select);

  if(!*select) {
    send_to_char("$3Syntax$R: clans <field> <correct arguments>\r\n"
                 "just clan <field> will give you the syntax for that field.\r\n"
                 "Fields are the following.\r\n"
                 , ch);
    strcpy(buf, "\r\n");
    for(i = 1; *god_values[i-1] != '\n'; i++) {
      sprintf(buf + strlen(buf), "%18s", god_values[i-1]); 
      if (!(i % 4))
      {
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
        *buf = '\0';
      }
    }
    if(*buf)
      send_to_char(buf, ch);
    send_to_char("\r\n", ch);
    return;
  }

  skill = old_search_block(select, 0, strlen(select), god_values, 1);
  if(skill < 0) {
    send_to_char("That value not recognized.\r\n", ch);
    return;
  }
  skill--;

  switch(skill) {
    case 0: /* create */
    {
      arg = one_argumentnolow(arg, text);
      arg = one_argumentnolow(arg, last);
      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans create <clanname> <clannumber>\r\n", ch);
        return;
      }
      if(strlen(text) > 29) {
        send_to_char("Clan name too long.\r\n", ch);
        return;
      }
      x = atoi(last);
#ifdef LEAK_CHECK
      clan = (struct clan_data *)calloc(1, sizeof(clan_data));
#else
      clan = (struct clan_data *)dc_alloc(1, sizeof(clan_data));
#endif
      clan->leader = str_dup(GET_NAME(ch));
      clan->founder = str_dup(GET_NAME(ch));
      clan->name = str_dup(text);
      clan->number = x;
      clan->rooms = 0;
      clan->next = 0;
      clan->email = NULL;
      clan->description = NULL;
      add_clan(clan);
      send_to_char("New clan created.\r\n", ch);
      break;
    }
    case 1: /* rename */
    {
      arg = one_argumentnolow(arg, text);
      arg = one_argumentnolow(arg, last);
      if(!*text || !*last) {
        send_to_char("$3Syntax$r: clans rename <targetclannum> <newname>\r\n", ch);
        return;
      }
      x = atoi(text);

      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;  
      }

      if(strlen(last) > 29) {
        send_to_char("Clan name too long.\r\n", ch);
        return;
      }

      strcpy(tarclan->name, last);
      send_to_char("Clan name changed.\r\n", ch);
      break;
    }
    case 2: /* leader */
    {
      arg = one_argumentnolow(arg, text);
      arg = one_argumentnolow(arg, last);
      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans leader <clannumber> <leadername>\r\n", ch);
        return;
      }

      if(strlen(last) > 14) {
        send_to_char("Clan leader name too long.\r\n", ch);
        return;
      }

      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      strcpy(tarclan->leader, last);
      send_to_char("Clan leader name changed.\r\n", ch);
      break;
    }
    case 3: /* delete */
    {
      arg = one_argumentnolow(arg, text);
      one_argumentnolow(arg, last);
      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans delete <clannumber> dElEtE\r\n", ch);
        return;
      }
      if(!isname(last, "dElEtE")) {
        send_to_char("You MUST end the line with 'dElEtE' to delete the clan.\r\n", ch);
        return;
      }
      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }
      delete_clan(tarclan);
      send_to_char("Clan deleted.\r\n", ch);
      break;
    }
    case 4: /* addroom */
    {
      arg = one_argumentnolow(arg, text);
      arg = one_argumentnolow(arg, last);
      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans addroom <clannumber> <roomnumber>\r\n", ch);
        return;
      }
      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      skill = atoi(last);
      if( -1 == real_room(skill)) {
        send_to_char("Invalid room number.\r\n", ch);
        return;
      }

      SET_BIT(world[real_room(skill)].room_flags, CLAN_ROOM);
#ifdef LEAK_CHECK
      newroom = (struct clan_room_data *)calloc(1, sizeof(clan_room_data));
#else
      newroom = (struct clan_room_data *)dc_alloc(1, sizeof(clan_room_data));
#endif
      newroom->room_number = skill;
      newroom->next = tarclan->rooms;
      tarclan->rooms = newroom;
      send_to_char("Room added.\r\n", ch);
      break;
    }
    case 5: /* list */
    {
      do_clan_list(ch);
      break;
    }
    case 6: /* save */
    {
      save_clans();
      send_to_char("Saved.\r\n", ch);
      break;
    }
    case 7: /* showrooms */
    {
      arg = one_argumentnolow(arg, text);
      if(!*text) {
        send_to_char("$3Syntax$R: clans showrooms <clannumber>\r\n", ch);
        return;
      }
      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      if(!tarclan->rooms) {
        send_to_char("This clan has no rooms set.\r\n", ch);
        return;
      }

      send_to_char("Rooms\r\n-----\r\n", ch);
      strcpy(buf, "\r\n");
      for(newroom = tarclan->rooms; newroom; newroom = newroom->next)
         if(newroom->room_number)
           sprintf(buf, "%s%d\r\n", buf, newroom->room_number);
         else sprintf(buf, "%sRoom Data without number.  PROBLEM.\r\n", buf);

      send_to_char(buf, ch);
      break;
    }
    case 8: /* killroom */
    {
      arg = one_argumentnolow(arg, text);
      one_argumentnolow(arg, last);
      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans killroom <clannumber> <roomnumber>\r\n", ch);
        return;
      }
      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      if(!tarclan->rooms) {
        send_to_char("Error.  Target clan has no rooms.\r\n", ch);
        return;
      }

      skill = atoi(last);
      if(tarclan->rooms->room_number == skill) {
        if(-1 != real_room(skill))
          if(IS_SET(world[real_room(skill)].room_flags, CLAN_ROOM))
            REMOVE_BIT(world[real_room(skill)].room_flags, CLAN_ROOM);
        lastroom = tarclan->rooms;
        tarclan->rooms = tarclan->rooms->next;
        dc_free(lastroom);
        send_to_char("Deleted.\r\n", ch);
        return;
      }
    
      newroom = tarclan->rooms;
      while(newroom) 
        if(newroom->room_number == skill) {
          if(-1 != real_room(skill))
            if(IS_SET(world[real_room(skill)].room_flags, CLAN_ROOM))
              REMOVE_BIT(world[real_room(skill)].room_flags, CLAN_ROOM);
          lastroom->next = newroom->next;
          dc_free(newroom);
          send_to_char("Deleted.\r\n", ch);
          return;
        }
        else {
          lastroom = newroom;
          newroom = newroom->next;
        }

      send_to_char("Specified room number not found.\r\n", ch);
      break;
    }

    case 9: { /* email */
      
      arg = one_argumentnolow(arg, text);
      one_argumentnolow(arg, last);

      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans email <clannumber> <address>\r\n", ch);
        send_to_char("To not have any email use:  clans email <clannumber> delete\r\n", ch);
        return;
      }

      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      i = ch->clan;
      ch->clan = x;
      clan_email(ch, last);
      ch->clan = i;
      
      break;
    }

    case 10: { /* description */
      
      arg = one_argumentnolow(arg, text);
      one_argumentnolow(arg, last);

      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans description <clannumber> change\r\n", ch);
        send_to_char("To not have any description use:  clans description <clannumber> delete\r\n", ch);
        return;
      }

      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      i = ch->clan;
      ch->clan = x;
      clan_desc(ch, last);
      ch->clan = i;
      
      break;
    }
    case 11: { /* login */
      half_chop(arg, text, last);

      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans login <clannumber> <login message>\r\n", ch);
        send_to_char("To not have any message use:  clans login <clannumber> delete\r\n", ch);
        return;
      }

      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      i = ch->clan;
      ch->clan = x;
      clan_login_message(ch, last);
      ch->clan = i;
      break;
    }
    case 12: { /* logout */

      //arg = one_argumentnolow(arg, text);
      //one_argumentnolow(arg, last);
      half_chop(arg, text, last);

      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans logout <clannumber> <logout message>\r\n", ch);
        send_to_char("To not have any message use:  clans logout <clannumber> delete\r\n", ch);
        return;
      }

      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      i = ch->clan;
      ch->clan = x;
      clan_logout_message(ch, last);
      ch->clan = i;
      
      break;
    }
    case 13: { /* death */
      half_chop(arg, text, last);

      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans death <clannumber> <death message>\r\n", ch);
        send_to_char("To not have any message use:  clans death <clannumber> delete\r\n", ch);
        return;
      }

      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      i = ch->clan;
      ch->clan = x;
      clan_death_message(ch, last);
      ch->clan = i;
      
      break;
    }
    case 14: { // members
      one_argument(arg, text);

      if(!*text) {
        send_to_char("$3Syntax$R: clans members <clannumber>\r\n", ch);
        return;
      }

      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      i = ch->clan;
      ch->clan = x;
      do_clan_member_list(ch);
      ch->clan = i;

      break;
    }
    case 15: { // rights
      half_chop(arg, text, last);

      if(!*text) {
        send_to_char("$3Syntax$R: clans rights <clannumber>\r\n", ch);
        return;
      }

      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      i = ch->clan;
      ch->clan = x;
      do_clan_rights(ch, last);
      ch->clan = i;
      break;
    }
    case 16: { // motd
      
      arg = one_argumentnolow(arg, text);
      one_argumentnolow(arg, last);

      if(!*text || !*last) {
        send_to_char("$3Syntax$R: clans motd <clannumber> change\r\n", ch);
        send_to_char("To not have any motd use:  clans motd <clannumber> delete\r\n", ch);
        return;
      }

      x = atoi(text);
      tarclan = get_clan(x);

      if(!tarclan) {
        send_to_char("Invalid clan number.\r\n", ch);
        return;
      }

      i = ch->clan;
      ch->clan = x;
      clan_motd(ch, last);
      ch->clan = i;
      
      break;
    }
    default: {
      send_to_char("Default hit in clans switch statement.\r\n", ch);
      return;
      break;
    }
  }
}

void do_leader_clans(CHAR_DATA *ch, char *arg, int cmd)
{
  struct clan_member_data * pmember = 0;
//  struct clan_data * tarclan = 0;
//  struct clan_room_data * newroom = 0;
//  struct clan_room_data * lastroom = 0;

  char buf[MAX_STRING_LENGTH];
  char select[MAX_STRING_LENGTH];
//  char text[MAX_STRING_LENGTH];
//  char last[MAX_STRING_LENGTH];

  int i, j, leader;
//  int x;
  int16 skill;

  char *mortal_values[] = {
          "list", 
          "email", 
          "description", 
          "login", 
          "logout", 
          "death", 
          "members", 
          "rights",
          "motd",
          "help",
          "\n"
  };

  int right_required[] = { 
          0,
          CLAN_RIGHTS_INFO,
          CLAN_RIGHTS_INFO,
          CLAN_RIGHTS_MESSAGES,
          CLAN_RIGHTS_MESSAGES,
          CLAN_RIGHTS_MESSAGES,
          CLAN_RIGHTS_MEMBER_LIST,
          CLAN_RIGHTS_RIGHTS,
          CLAN_RIGHTS_INFO,
          0,
          -1
  };

  if(!(pmember = get_member(GET_NAME(ch), ch->clan))) {
    send_to_char("Error:  no clan in do_clans_leader\n\r", ch);
    return;
  }

  leader = is_clan_leader(ch);

  arg = one_argumentnolow(arg, select);

  if(!*select) {
    send_to_char("$3Syntax$R: clans <field> <correct arguments>\r\n"
                 "just clan <field> will give you the syntax for that field.\r\n"
                 "Fields are the following.\r\n"
                 , ch);
    strcpy(buf, "\r\n");
    j = 1;
    for(i = 0; *mortal_values[i] != '\n'; i++) {
      // only show rights the player has.  Leader has all.
      if(!leader && right_required[i] && 
         !IS_SET(pmember->member_rights, right_required[i]))
        continue;

      sprintf(buf + strlen(buf), "%18s", mortal_values[i]); 
      if (!(j % 4))
      {
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
        *buf = '\0';
      }
      j++;
    }
    if(*buf)
      send_to_char(buf, ch);
    send_to_char("\r\n", ch);
    return;
  }

  skill = old_search_block(select, 0, strlen(select), mortal_values, 1);
  if(skill < 0) {
    send_to_char("That value not recognized.\r\n", ch);
    return;
  }
  skill--;

  if(!leader && right_required[skill] && !has_right(ch, right_required[skill])) {
    send_to_char("You don't have that right!\n\r", ch);
    return;
  }

  switch(skill) {
    case 0: /* list */
    {
      do_clan_list(ch);
      break;
    }
    case 1: /* email */
    {
      if(clan_email(ch, arg))
        save_clans();
      break;      
    }
    case 2: /* description */
    {
      if(clan_desc(ch, arg))
        save_clans();
      break;
    }
    case 3: /* login */
    {
      if(clan_login_message(ch, arg))
        save_clans();
      break;
    }
    case 4: /* logout */
    {
      if(clan_logout_message(ch, arg))
        save_clans();
      break;
    }
    case 5: /* death */
    {
      if(clan_death_message(ch, arg))
        save_clans();
      break;
    }
    case 6: // members
    {
      do_clan_member_list(ch);
      break;
    }
    case 7: // rights
    {
      do_clan_rights(ch, arg);
      break;
    }
    case 8: // motd
    {
      if(clan_motd(ch, arg))
        save_clans();
      break;
    }
    case 9: // help
    {
      send_to_char("$3Command Help$R\r\n"
                   "------------\r\n"
                   " list    - Shows the clans list\r\n"
                   " email   - Changes email listed in cinfo\r\n"
                   " login   - Changes clan member login message\r\n"
                   " logout  - Changes clan member logout message\r\n"
                   " death   - Changes clan member death message\r\n"
                   " members - Shows list of current clan members\r\n"
                   " rights  - Shows/Changes clan members rights\r\n"
                   " motd    - Changes clan message of the day\r\n"
                   " help    - duh....\r\n"
                   "\r\n"
                   "$3Clan Members Rights$R\r\n"
                   "-------------------\r\n"
                   " accept  - Ability to accept people into clan\r\n"
                   " outcast - Ability to outcast people from clan\r\n"
                   " read    - Ability to read clan board\r\n"
                   " write   - Ability to write on clan board\r\n"
                   " remove  - Ability to remove clan board posts\r\n"
                   " member  - Ability to view the clan member list\r\n"
                   " rights  - Ability to modify other member's rights\r\n"
                   " messages- Ability to modify clan login/out/death messages\r\n"
                   " info    - Ability to modify clan email/description/motd\r\n"
                   , ch);
      break; 
    }
    default: {
      send_to_char("Default hit in clans switch statement.\r\n", ch);
      return;
      break;
    }
  }
}

int needs_clan_command(char_data * ch)
{
  if(has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
    return 1;
  if(has_right(ch, CLAN_RIGHTS_RIGHTS))
    return 1;

  return 0;
}

int do_clans(CHAR_DATA *ch, char *arg, int cmd)
{
  struct clan_data * clan = 0;

  if(!strcmp("Pirahna", GET_NAME(ch)) || 
     !strcmp("Apocalypse", GET_NAME(ch)) ||
     !strcmp("Urizen", GET_NAME(ch)) ||
     !strcmp("Dasein", GET_NAME(ch)) ||
     !strcmp("Valkyrie", GET_NAME(ch))
    ) 
  {
    do_god_clans(ch, arg, cmd);
    return eSUCCESS;
  }

  if(ch->clan && (clan = get_clan(ch)) && 
     ( !strcmp(GET_NAME(ch), clan->leader) || needs_clan_command(ch) )
    )
  {
    do_leader_clans(ch, arg, cmd);
    return eSUCCESS;
  }

  do_clan_list(ch);
  return eSUCCESS;
}

int do_cinfo(CHAR_DATA *ch, char *arg, int cmd)
{
  struct clan_data *clan;
  int nClan;
  char buf[MAX_STRING_LENGTH];

  if(!*arg)
  {
    send_to_char("$3Syntax$R:  cinfo <clannumber>\r\n", ch);
    return eSUCCESS;
  }

  nClan = atoi(arg);

  if(!(clan = get_clan(nClan)))
  {
    send_to_char("That is not a valid clan number.\r\n", ch);
    return eFAILURE;
  }
  sprintf(buf, "$3Name$R:           %s$R $3($R%d$3)$R\r\n"
               "$3Leader$R:         %s\r\n"
               "$3Contact Email$R:  %s\r\n"
               "$3Clan Hall$R:      %s\r\n"
               "$3Clan info$R:\r\n"
               "$3----------$R\r\n",
                   clan->name, nClan,
                   clan->leader,
                   clan->email ? clan->email : "(No Email)",
                   clan->rooms ? "Yes" : "No");
  send_to_char(buf, ch);

  // This has to be separate, or if the leader uses $'s, it comes out funky
  sprintf(buf, "%s\r\n",
               clan->description ? clan->description : "(No Description)\r\n");
  send_to_char(buf, ch);

  if(GET_LEVEL(ch) >= POWER || (!strcmp(clan->leader, GET_NAME(ch)) && nClan == ch->clan) ||
        ( nClan == ch->clan && has_right(ch, CLAN_RIGHTS_MESSAGES) )
    )
  {
    sprintf(buf, "$3Login$R:          %s\r\n"
                 "$3Logout$R:         %s\r\n"
                 "$3Death$R:          %s\r\n",
                    clan->login_message ? clan->login_message : "(No Message)",
                    clan->logout_message ? clan->logout_message : "(No Message)",
                    clan->death_message ? clan->death_message : "(No Message)");
   send_to_char(buf, ch);
  }
  return eSUCCESS;
}

int do_whoclan(CHAR_DATA *ch, char *arg, int cmd)
{
  struct clan_data *clan;
  struct descriptor_data *desc;
  CHAR_DATA *pch;
  char buf[100];
  int found;

  send_to_char("                  O N L I N E   C L A N   "
               "M E M B E R S\n\r\n\r", ch);

  char buf2 [MAX_INPUT_LENGTH];
  one_argument (arg, buf2);
  int clan_num = 0;
  
  if (buf2)
    clan_num = atoi (buf2);

  for(clan = clan_list; clan; clan = clan->next) {
     found = 0; 
     if (clan_num && clan->number != clan_num)
	continue;
     for(desc = descriptor_list; desc; desc = desc->next) {
        if(desc->connected || !(pch = desc->character))
          continue;
        if(pch->clan != clan->number || GET_LEVEL(pch) >= OVERSEER ||
	   (!CAN_SEE(ch, pch) && ch->clan != pch->clan))
          continue;
        if(found == 0) { 
          sprintf(buf, "$3Clan %s$R:\n\r", clan->name);
          send_to_char(buf, ch);
        } 
	if (clan->number == ch->clan && has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
          sprintf(buf, "  %s %s %s\n\r", GET_SHORT(pch), (!strcmp(GET_NAME(pch),clan->leader) ? "$3($RLeader$3)$R" : ""),IS_SET(GET_TOGGLES(pch),PLR_NOTAX) ? "(NT)":"(T)");
	else
          sprintf(buf, "  %s %s\n\r", GET_SHORT(pch), (!strcmp(GET_NAME(pch), 
clan->leader) ? "$3($RLeader$3)$R" : ""));
        send_to_char(buf, ch);
        found++;
     }
  } 
  return eSUCCESS;
}


int do_cmotd(CHAR_DATA *ch, char *arg, int cmd)
{
  struct clan_data *clan;

  if(!ch->clan || !(clan = get_clan(ch))) {
    send_to_char("You aren't the member of any clan!\n\r", ch);
    return eFAILURE;
  }

  if(!clan->clanmotd) {
    send_to_char("There is no motd for your clan currently.\r\n", ch);
    return eSUCCESS;
  }

  send_to_char(clan->clanmotd, ch);
  return eSUCCESS;
}

int do_ctax(CHAR_DATA *ch, char *arg, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  if (!ch->clan)
  {
     send_to_char("You not a member of a clan.\r\n",ch);
     return eFAILURE;
  }
  arg = one_argument(arg,arg1);
  if (!is_number(arg1))
  {
     csendf(ch,"Your clan's current tax rate is %d.\r\n",get_clan(ch)->tax);
     return eFAILURE;
  }
  if (!has_right(ch, CLAN_RIGHTS_TAX))
  {
     send_to_char("You don't have the right to modify taxes.\r\n",ch);
     return eFAILURE;
  }

  int tax = atoi(arg1);
  if (tax < 0 || tax > 99)
  {
    send_to_char("You can have a maximum of 99% in taxes.\r\n",ch);
    return eFAILURE;
  }
  get_clan(ch)->tax = tax;
  send_to_char("Your clan's tax rate has been modified.\r\n",ch);
  save_clans();
  return eSUCCESS;
}


int do_cdeposit(CHAR_DATA *ch, char *arg, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  if (!ch->clan)
  {
     send_to_char("You not a member of a clan.\r\n",ch);
     return eFAILURE;
  }
/*  if (!has_right(ch, CLAN_RIGHTS_DEPOSIT))
  {
     send_to_char("You don't have the right to .\r\n",ch);
     return eFAILURE;
  }*/
  if (affected_by_spell(ch,FUCK_GTHIEF))
  {
    send_to_char("Launder your money elsewhere, thief!\r\n",ch);
    return eFAILURE;
  }
  if (world[ch->in_room].number != 3005)
  {
    send_to_char("This can only be done at the Sorpigal bank.\r\n",ch);
    return eFAILURE;
  }
  arg = one_argument(arg,arg1);
  if (!is_number(arg1))
  {
    send_to_char("How much do you want to deposit?\r\n",ch);
    return eFAILURE;
  }
  unsigned int dep = atoi(arg1);
  if (GET_GOLD(ch) < dep || dep < 0)
  {
    send_to_char("You don't have that much gold.\r\n",ch);
    return eFAILURE;

  }
  GET_GOLD(ch) -= dep;
  get_clan(ch)->balance += dep;
  csendf(ch,"You deposit %d gold coins into your clan's account.\r\n",dep);
  save_clans();
  do_save(ch,"",666);
  return eSUCCESS;
}

int do_cwithdraw(CHAR_DATA *ch, char *arg, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  if (!ch->clan)
  {
     send_to_char("You not a member of a clan.\r\n",ch);
     return eFAILURE;
  }
  if (!has_right(ch, CLAN_RIGHTS_WITHDRAW))
  {
     send_to_char("You don't have the right to withdraw gold from your clan's account.\r\n",ch);
     return eFAILURE;
  }
  if (world[ch->in_room].number != 3005)
  {
    send_to_char("This can only be done at the Sorpigal bank.\r\n",ch);
    return eFAILURE;
  }

  arg = one_argument(arg,arg1);
  if (!is_number(arg1))
  {
    send_to_char("How much do you want to withdraw?\r\n",ch);
    return eFAILURE;
  }
  long wdraw = atoi(arg1);
  if (get_clan(ch)->balance < wdraw || wdraw < 0)
  {
     send_to_char("Your clan lacks the funds.\r\n",ch);
     return eFAILURE;
  }
  GET_GOLD(ch) += (int)wdraw;
  get_clan(ch)->balance -= wdraw;
  csendf(ch,"You withdraw %d gold coins.",wdraw);
  save_clans();
  do_save(ch, "", 0);
  return eSUCCESS;
}

int do_cbalance(CHAR_DATA *ch, char *arg, int cmd)
{
  if (!ch->clan)
  {
     send_to_char("You not a member of a clan.\r\n",ch);
     return eFAILURE;
  }
  if (world[ch->in_room].number != 3005)
  {
    send_to_char("This can only be done at the Sorpigal bank.\r\n",ch);
    return eFAILURE;
  }

  if (!has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
  {
     send_to_char("You don't have the right to see your clan's account.\r\n",ch);
     return eFAILURE;
  }

  csendf(ch, "Your clan has %d gold coins in the bank.\r\n",get_clan(ch)->balance);
  return eSUCCESS;
}

void remove_totem(OBJ_DATA *altar, OBJ_DATA *totem)
{
  CHAR_DATA *t;
  for (t = character_list;t ;t = t->next)
   if (!IS_NPC(t) && t->altar == altar)
   {
     int j;
     for(j=0; j<totem->num_affects; j++)
        affect_modify(t, totem->affected[j].location,
          totem->affected[j].modifier, -1, FALSE);
     redo_hitpoints(t);
     redo_mana(t);
     redo_ki(t);
   }
}

void add_totem(OBJ_DATA *altar, OBJ_DATA *totem)
{
  CHAR_DATA *t;
  for (t = character_list;t ;t = t->next)
   if (!IS_NPC(t) && t->altar == altar)
   {
     int j;
     for(j=0; j<totem->num_affects; j++)
        affect_modify(t, totem->affected[j].location,
          totem->affected[j].modifier, -1, TRUE);
   }
}

void remove_totem_stats(CHAR_DATA *ch, int stat)
{
  OBJ_DATA *a;
  if (!ch->altar) return;
  for (a = ch->altar->contains; a ; a = a->next_content)
  {
    int j;
    if (a->obj_flags.type_flag != ITEM_TOTEM) continue;
     for(j=0; j<a->num_affects; j++)
       if (stat && stat == a->affected[j].location)
        affect_modify(ch, a->affected[j].location,
          a->affected[j].modifier, -1, FALSE);
	else if (!stat)
        affect_modify(ch, a->affected[j].location,
          a->affected[j].modifier, -1, FALSE);

  }  
  if (!stat){
     redo_hitpoints(ch);
     redo_mana(ch);
     redo_ki(ch);
 }
}

void add_totem_stats(CHAR_DATA *ch, int stat)
{
  OBJ_DATA *a;
  if (!ch->altar) return;
  for (a = ch->altar->contains; a ; a = a->next_content)
  {
    int j;
    if (a->obj_flags.type_flag != ITEM_TOTEM) continue;
     for(j=0; j<a->num_affects; j++)
       if (stat && stat == a->affected[j].location)
        affect_modify(ch, a->affected[j].location,
          a->affected[j].modifier, -1, TRUE);
	else if (!stat)
        affect_modify(ch, a->affected[j].location,
          a->affected[j].modifier, -1, TRUE);
  }
  if (!stat){
     redo_hitpoints(ch);
     redo_mana(ch);
     redo_ki(ch);
 }
}


/*

 Clanarea functions follow.


*/

struct takeover_pulse_data *pulse_list=NULL;
extern int top_of_zonet;

int count_plrs(int zone, int clan)
{
  CHAR_DATA *tmpch;
  int i = 0;
  for (tmpch = character_list; tmpch; tmpch = tmpch->next)
     if (!IS_NPC(tmpch) && world[tmpch->in_room].zone == zone && clan == tmpch->clan &&
		GET_LEVEL(tmpch) < 100 && GET_LEVEL(tmpch) > 10)
       i++;

  return i;
}

struct takeover_pulse_data
{
 struct takeover_pulse_data *next;
 int clan1; //defending clan
 int clan1points;
 int clan2; // challenging clan
 int clan2points;
 int zone;
 int pulse;
};

bool can_collect(int zone)
{
  struct takeover_pulse_data *take;
  for (take = pulse_list;take;take = take->next)
	if (zone == take->zone && take->clan2 != -2)
		return FALSE;
  return TRUE;
}



bool can_challenge(int clan, int zone)
{
  struct takeover_pulse_data *take;
  for (take = pulse_list;take;take = take->next)
    if (take->clan2 == -2 && 
		take->clan1 == clan && zone == take->zone)
	return FALSE;
    else if (zone == take->zone && take->clan2 > 0 &&
		take->clan1 > 0)
	return FALSE;
  return TRUE;  
}

void takeover_pause(int clan, int zone)
{
    struct takeover_pulse_data *pl;
    #ifdef LEAK_CHECK
         pl = (struct takeover_pulse_data *)calloc(1, sizeof(struct takeover_pulse_data));
    #else
         pl = (struct takeover_pulse_data *)dc_alloc(1, sizeof(struct takeover_pulse_data));
    #endif
    pl->next = pulse_list;
    pl->clan1 = clan;
    pl->clan2 = -2;
    pl->pulse = 0;
    pl->zone = zone;
    pulse_list = pl;
 
}

void claimArea(int clan, bool defend, bool challenge, int clan2, int zone)
{
  char buf[MAX_STRING_LENGTH];

  if (challenge) {
    if (!defend) {
//      zone_table[zone].gold = 0;
      sprintf(buf, "\r\n##Clan %s has broken clan %s's control of%s!\r\n", 
	get_clan(clan)->name, get_clan(clan2)->name, zone_table[zone].name);
      takeover_pause(clan2, zone);
    } else {
      takeover_pause(clan2, zone);
      sprintf(buf, "\r\n##Clan %s has defended against clan %s's challenge for control of%s!\r\n", 
	get_clan(clan)->name, get_clan(clan2)->name, zone_table[zone].name);
    }
  } else{
      sprintf(buf, "\r\n##%s has been claimed by clan %s!", 
	zone_table[zone].name,get_clan(clan)->name);
//     zone_table[zone].gold = 0;
  }
  zone_table[zone].clanowner = clan;
  send_info(buf);
}

int count_controlled_areas(int clan)
{
  int i,z=0;
  for (i = 0; i <= top_of_zonet; i++)
    if (zone_table[i].clanowner == clan && can_collect(i)) z++;
  struct takeover_pulse_data *plc;
  for (plc = pulse_list;plc;plc=plc->next)
    if ((plc->clan1 == clan || plc->clan2 == clan) && !plc->clan2 == -2) z++;
  return z;
}

void recycle_pulse_data(struct takeover_pulse_data *pl)
{
  struct takeover_pulse_data *plc, *plp = NULL;
  for (plc = pulse_list;plc;plc = plc->next)
  {
	if (plc == pl)
	{
		if (plp) plp->next = plc->next;
		else pulse_list = plc->next;
		dc_free(pl);
		return; // No point going on..
	}
	plp = plc;
  }
}

int online_clan_members(int clan)
{
  CHAR_DATA *Tmpch;
  int i =0;
  for (Tmpch = character_list;Tmpch;Tmpch = Tmpch->next)
  {
	if (!IS_NPC(Tmpch) && Tmpch->clan == clan && GET_LEVEL(Tmpch) < 100 && Tmpch->desc && GET_LEVEL(Tmpch) > 10)
		i++;
  }
  return i;
}

void check_victory(struct takeover_pulse_data *take)
{
    if (take->clan2 == -2) return;
    if (take->clan1points >= 20)
    {
	claimArea(take->clan1, TRUE, TRUE, take->clan2,take->zone);
	recycle_pulse_data(take);
    }
    else if (take->clan2points >= 20)
    {
	claimArea(take->clan2, FALSE, TRUE, take->clan1,take->zone);
	recycle_pulse_data(take);
    }
}


void check_quitter(void *arg1, void *arg2,void *arg3)
{
  int clan = (int)arg1;
  char buf[MAX_STRING_LENGTH];
  if (count_controlled_areas(clan) > online_clan_members(clan))
  { // One needs to go.
     int i = number(1, count_controlled_areas(clan));
     int a,z = 0;
     for (a = 0; a <= top_of_zonet; a++)
	if (zone_table[a].clanowner == clan && can_collect(a))
		if (++z == i)
		{
//			zone_table[a].gold = 0;
			zone_table[a].clanowner = 0;
			sprintf(buf, "\r\n##Clan %s has lost control of%s!\r\n",
				get_clan(clan)->name, zone_table[a].name);
			send_info(buf);
			return;
		}
     struct takeover_pulse_data *pl;
     for (pl = pulse_list;pl;pl = pl->next)
     {
	if (pl->clan1 == clan && pl->clan2 != -2)
		if (++z == i)
		{
			pl->clan2points += 20;
			check_victory(pl);
			return;
		}
	if (pl->clan2 == clan)
		if (++z == i)
		{
			pl->clan1points += 20;
			check_victory(pl);
			return;
		}
     }
  }
}

void check_quitter(CHAR_DATA *ch)
{
  if (!ch->clan || GET_LEVEL(ch) > 100) return;
  
          struct timer_data *timer;
 #ifdef LEAK_CHECK
          timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
          timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif
          timer->arg1 = (void*)ch->clan;
          timer->function = check_quitter;
          timer->timeleft = 30;
	  extern void addtimer(struct timer_data *timer);
          addtimer(timer);

}


void pk_check(CHAR_DATA *ch, CHAR_DATA *victim)
{
  if (!ch || !victim) return;
  if (!ch->clan || !victim->clan) return; // No point;
  struct takeover_pulse_data *plc,*pln;
  for (plc = pulse_list; plc; plc=pln)
  {
    pln = plc->next;
    if (plc->clan1 == ch->clan && plc->clan2 == victim->clan)
	plc->clan1points += 2;
    else if (plc->clan1 == victim->clan && plc->clan2 == ch->clan)
	plc->clan2points += 2;
    check_victory(plc);   
  }
}

bool can_lose(struct takeover_pulse_data *take)
{
  CHAR_DATA *ch;
  for (ch = character_list;ch;ch = ch->next)
  if (!IS_NPC(ch) && world[ch->in_room].zone == take->zone &&
	(take->clan1 == ch->clan || take->clan2 == ch->clan))
	return FALSE;
  return TRUE;
}

void pulse_takeover()
{ 
  struct takeover_pulse_data *take,*next;
  for (take = pulse_list;take;take = next)
  {
	next = take->next;
    take->pulse++;
   if (take->clan2 == -2)
   { // stopthing
     if (take->pulse >= 36*4)
	recycle_pulse_data(take);
     continue;
   }
   if (take->pulse < 2) continue; // first two pulses nothing happens
   if (take->pulse > 60 && take->clan2 != -2 && can_lose(take)) {
        char buf[MAX_STRING_LENGTH];
	sprintf(buf, "\r\n##Control of%s has been lost!\r\n",
		zone_table[take->zone].name);
	send_info(buf);
	zone_table[take->zone].clanowner = 0;
//	zone_table[take->zone].gold = 0;
	recycle_pulse_data(take);
	continue;
   }

    int favour = count_plrs(take->zone, take->clan1) - count_plrs(take->zone, take->clan2);
    if (favour > 0)
	take->clan1points += favour;
     else
	take->clan2points -= favour; // it's negative, so that's a +
    
    check_victory(take);
  }  
}

int do_clanarea(CHAR_DATA *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg);

  if (!ch->clan)
  {
	send_to_char("You're not in a clan!\r\n",ch);
	return eFAILURE;
  }
  if (!has_right(ch, CLAN_RIGHTS_AREA))
  {
	send_to_char("You have not been granted that right.\r\n",ch);
	return eFAILURE;
  }  
  if (!str_cmp(arg, "claim"))
  {
    if (zone_table[world[ch->in_room].zone].clanowner > 0)
    {
	csendf(ch, "This area is claimed by %s, you need to challenge to obtain leadership.\r\n",
			get_clan(zone_table[world[ch->in_room].zone].clanowner)->name);

	return eFAILURE;
    }
    if (IS_SET(zone_table[world[ch->in_room].zone].zone_flags, ZONE_NOCLAIM))
    {
	send_to_char("This area cannot be claimed.\r\n",ch);
	return eFAILURE;
    }
    if (count_controlled_areas(ch->clan) >= online_clan_members(ch->clan))
    {
	send_to_char("You cannot claim any more areas.\r\n",ch);
	return eFAILURE;
    }
    send_to_char("You claim the area on behalf of your clan.\r\n",ch);
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "\r\n##%s has been claimed by %s!\r\n",
		zone_table[world[ch->in_room].zone].name, get_clan(ch->clan)->name);
    send_info(buf);
    zone_table[world[ch->in_room].zone].clanowner = ch->clan;
//    zone_table[world[ch->in_room].zone].gold = 0;
    return eSUCCESS;
  }
  else if (!str_cmp(arg, "yield"))
  {
    if (zone_table[world[ch->in_room].zone].clanowner == 0)
    {
	send_to_char("This zone is not under anyone's control.\r\n",ch);
	return eFAILURE;
    }
    if (zone_table[world[ch->in_room].zone].clanowner != ch->clan)
    {
	send_to_char("This zone is not under your clan's control.\r\n",ch);
	return eFAILURE;
    }

    struct takeover_pulse_data *take;
    for (take = pulse_list;take; take = take->next)
	if (take->zone == world[ch->in_room].zone &&
		take->clan1 == ch->clan && take->clan2 !=
		-2)
	{
		take->clan2points += 20;
		check_victory(take);
		return eSUCCESS;
	}
    send_to_char("You yield the area on behalf of your clan.\r\n",ch);
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "\r\n##Clan %s has yielded control of%s!\r\n",
	get_clan(ch->clan)->name,zone_table[world[ch->in_room].zone].name);
    send_info(buf);
    zone_table[world[ch->in_room].zone].clanowner = 0;
//    zone_table[world[ch->in_room].zone].gold = 0;
    return eSUCCESS;
  } else if (!str_cmp(arg, "collect"))
  {
    if (zone_table[world[ch->in_room].zone].clanowner == 0)
    {
	send_to_char("This area is not under anyone's control.\r\n",ch);
	return eFAILURE;
    }
    
    if (zone_table[world[ch->in_room].zone].clanowner != ch->clan)
    {
	send_to_char("This area is not under your clan's control.\r\n",ch);
	return eFAILURE;
    }
   
    if (zone_table[world[ch->in_room].zone].gold == 0)
    {
	send_to_char("There is no gold to collect.\r\n",ch);
	return eFAILURE;
    }
    if (!can_collect(world[ch->in_room].zone))
    {
	send_to_char("There is currently an active challenge for this area, and collecting is not possible.\r\n",ch);
	return eFAILURE;
    }
    get_clan(ch)->balance += zone_table[world[ch->in_room].zone].gold;
    csendf(ch, "You collect %d gold for your clan's treasury.\r\n",
	    zone_table[world[ch->in_room].zone].gold);
    zone_table[world[ch->in_room].zone].gold = 0;
    save_clans();
    return eSUCCESS;
  } else if (!str_cmp(arg, "list")) {
	int i,z=0;
	for (i = 0; i <= top_of_zonet; i++)
	  if (zone_table[i].clanowner == ch->clan)
	  {
		if (++z == 1)
		  csendf(ch, "$BAreas Claimed by %s:$R\r\n",
		    get_clan(ch)->name);
		csendf(ch, "%d)%s\r\n",
			z, zone_table[i].name);
	  }
	if (z  == 0)
	{
		send_to_char("Your clan has not claimed any areas.\r\n",ch);
		return eFAILURE;
	}
    return eSUCCESS;
  } else if (!str_cmp(arg, "challenge")) {
    // most annoying one for last.
    if (zone_table[world[ch->in_room].zone].clanowner == 0)
    {
	send_to_char("This area is not under anyone's control, you could simply claim it.\r\n",ch);
	return eFAILURE;
    }
    if (!can_challenge(ch->clan, world[ch->in_room].zone))
    {
	send_to_char("You cannot issue a challenge for this area at the moment.\r\n",ch);
	return eFAILURE;
    }
    if (zone_table[world[ch->in_room].zone].clanowner == ch->clan)
    {
	send_to_char("Your clan already controls this area!\r\n",ch);
	return eFAILURE;
    }
    if (count_controlled_areas(ch->clan) >= online_clan_members(ch->clan))
    {
	send_to_char("You cannot own any more areas.\r\n",ch);
	return eFAILURE;
    }

    // no point checking for noclaim flag, at this point it already IS under someone's control    
    struct takeover_pulse_data *pl;
    #ifdef LEAK_CHECK
         pl = (struct takeover_pulse_data *)calloc(1, sizeof(struct takeover_pulse_data));
    #else
         pl = (struct takeover_pulse_data *)dc_alloc(1, sizeof(struct takeover_pulse_data));
    #endif
    pl->next = pulse_list;
    pl->clan1 = zone_table[world[ch->in_room].zone].clanowner;
    pl->clan2 = ch->clan;
    pl->clan1points = pl->clan2points = 0;
    pl->pulse = 0;
    pl->zone = world[ch->in_room].zone;
    pulse_list = pl;
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "\r\n##Clan %s has challenged clan %s for control of%s!\r\n",
			get_clan(ch)->name, get_clan(pl->clan1)->name, 
			zone_table[world[ch->in_room].zone].name);
    send_info(buf);
    return eSUCCESS;
  }
  send_to_char("Clan Area Commands:\r\n"
		"--------------------\r\n"
		"clanarea list         (lists areas currently claimed by your clan)\r\n"
		"clanarea claim        (claim the area you are currently in for your clan)\r\n"
		"clanarea challenge    (challenge for control of the area you are currently in)\r\n"
		"clanarea yield        (yield an area your clan controls that you are currently in)\r\n"
		"clanarea collect      (collect bounty from an area you are currently in that your clan controls)\r\n",ch);
  return eSUCCESS;
}
