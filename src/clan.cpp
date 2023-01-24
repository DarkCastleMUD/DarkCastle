/* $Id: clan.cpp,v 1.92 2014/07/26 16:19:37 jhhudso Exp $ */

/***********************************************************************/
/* Revision History                                                    */
/* 11/10/2003    Onager     Removed clan size limit                    */
/***********************************************************************/
#define __STDC_LIMIT_MACROS
#include <stdint.h>
uint64_t i = UINT64_MAX;

#include <string.h> // strcat
#include <stdio.h>  // FILE *
#include <ctype.h>  // isspace..
#include <netinet/in.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <algorithm>
#include <locale>

#include <fmt/format.h>
#include <QFile>

#include "fileinfo.h"
#include "db.h" // real_room
#include "player.h"
#include "utility.h"
#include "character.h"
#include "connect.h"  // Connection
#include "mobile.h"   // utility.h stuff
#include "clan.h"     // duh
#include "interp.h"   // do_outcast, etc..
#include "levels.h"   // levels
#include "handler.h"  // get_char_room_vis
#include "terminal.h" // get_char_room_vis
#include "room.h"     // CLAN_ROOM flag
#include "returnvals.h"
#include "spells.h"
#include "DC.h"
#include "Trace.h"
#include "clan.h"

using namespace std;


extern index_data *obj_index;
extern CWorld world;

void addtimer(struct timer_data *timer);
void delete_clan(const clan_data *currclan);

clan_data *clan_list = 0;
clan_data *end_clan_list = 0;

#define MAX_CLAN_DESC_LENGTH 1022

const char *clan_rights[] = {
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
    "vault",
    "vaultlog",
    "log",
    "\n"};

void boot_clans(void)
{
  FILE *fl;
  char buf[1024];
  clan_data *new_new_clan = nullptr;
  clan_room_data *new_new_room = nullptr;
  clan_member_data *new_new_member = nullptr;
  int tempint;
  bool skip_clan = false, changes_made = false;

  if (!(fl = fopen("../lib/clan.txt", "r")))
  {
    fprintf(stderr, "Unable to open clan file...\n");
    fl = fopen("../lib/clan.txt", "w");
    fprintf(fl, "~\n");
    fclose(fl);
    abort();
  }

  char a;
  while ((a = fread_char(fl)) != '~')
  {
    ungetc(a, fl);

    new_new_clan = new clan_data;
    new_new_clan->next = 0;
    new_new_clan->tax = 0;
    new_new_clan->email = nullptr;
    new_new_clan->description = nullptr;
    new_new_clan->login_message = nullptr;
    new_new_clan->death_message = nullptr;
    new_new_clan->logout_message = nullptr;
    new_new_clan->rooms = nullptr;
    new_new_clan->members = nullptr;
    new_new_clan->acc = 0;
    new_new_clan->amt = 0;
    new_new_clan->leader = fread_word(fl, 1);
    new_new_clan->founder = fread_word(fl, 1);
    new_new_clan->name = fread_word(fl, 1);
    new_new_clan->number = fread_int(fl, 0, 2147483467);
    if (new_new_clan->number < 1 || new_new_clan->number >= 2147483467)
    {
      logf(0, LogChannels::LOG_BUG, "Invalid clan number %d found in ../lib/clan.txt.", new_new_clan->number);
      skip_clan = true;
    }

    if (get_clan(new_new_clan->number) != nullptr)
    {
      logf(0, LogChannels::LOG_BUG, "Duplicate clan number %d found in ../lib/clan.txt.", new_new_clan->number);
      skip_clan = true;
    }

    char b;
    while (true) /* I see clan rooms! */
    {
      b = fread_char(fl);
      if (b == 'S')
        break;
      if (b != 'R')
        continue;
#ifdef LEAK_CHECK
      new_new_room = (struct clan_room_data *)calloc(1, sizeof(struct clan_room_data));
#else
      new_new_room = (struct clan_room_data *)dc_alloc(1,
                                                       sizeof(struct clan_room_data));
#endif
      new_new_room->next = new_new_clan->rooms;
      tempint = fread_int(fl, 0, 50000);
      new_new_room->room_number = (int16_t)tempint;
      new_new_clan->rooms = new_new_room;
    }

    char a;
    while ((a = fread_char(fl)) != '~')
    {
      if (a != ' ' && a != '\n')
        getc(fl);
      switch (a)
      {
      case ' ':
      case '\n':
        break;
      case 'E':
      {
        new_new_clan->email = fread_string(fl, 0);
        break;
      }
      case 'D':
      {
        new_new_clan->description = fread_string(fl, 0);
        break;
      }
      case 'C':
      {
        new_new_clan->clanmotd = fread_string(fl, 0);
        break;
      }
      case 'B':
      { // Account balance
        try
        {
          new_new_clan->setBalance(fread_uint(fl, 0, UINT64_MAX));
        }
        catch (error_negative_int &e)
        {
          fprintf(stderr, "negative clan balance read for clan %d.\n",
                  new_new_clan->number);
          fprintf(stderr, "Setting clan %d's balance to 0.\n",
                  new_new_clan->number);
          new_new_clan->setBalance(0);
        }
        catch (...)
        {
          fprintf(stderr,
                  "unknown error reading clan balance for clan %d.\n",
                  new_new_clan->number);
          fprintf(stderr, "Setting clan %d's balance to 0.\n",
                  new_new_clan->number);
          new_new_clan->setBalance(0);
        }
        break;
      }
      case 'T':
      { // Tax
        new_new_clan->tax = fread_int(fl, 0, 99);
        break;
      }
      case 'L':
      {
        new_new_clan->login_message = fread_string(fl, 0);
        break;
      }
      case 'X':
      {
        new_new_clan->death_message = fread_string(fl, 0);
        break;
      }
      case 'O':
      {
        new_new_clan->logout_message = fread_string(fl, 0);
        break;
      }
      case 'M':
      { // read a member
#ifdef LEAK_CHECK
        new_new_member = (struct clan_member_data *)calloc(1, sizeof(struct clan_member_data));
#else
        new_new_member = (struct clan_member_data *)dc_alloc(1,
                                                             sizeof(struct clan_member_data));
#endif
        new_new_member->member_name = fread_string(fl, 0);
        new_new_member->member_rights = fread_int(fl, 0, 2147483467);
        new_new_member->member_rank = fread_int(fl, 0, 2147483467);
        new_new_member->unused1 = fread_int(fl, 0, 2147483467);
        new_new_member->unused2 = fread_int(fl, 0, 2147483467);
        new_new_member->unused3 = fread_int(fl, 0, 2147483467);
        new_new_member->time_joined = fread_int(fl, 0, 2147483467);
        new_new_member->unused4 = fread_string(fl, 0);

        // add it to the member linked list
        add_clan_member(new_new_clan, new_new_member);
        break;
      }
      default:
        logentry("Illegal switch hit in boot_clans.", 0, LogChannels::LOG_MISC);
        logentry(buf, 0, LogChannels::LOG_MISC);
        break;
      }
    }
    if (skip_clan)
    {
      skip_clan = false;
      logf(0, LogChannels::LOG_BUG, "Deleting clan number %d.", new_new_clan->number);
      delete_clan(new_new_clan);
      changes_made = true;
    }
    else
    {
      add_clan(new_new_clan);
    }
  }

  fclose(fl);

  if (changes_made)
  {
    logf(0, LogChannels::LOG_BUG, "Changes made to clans. Saving ../lib/clan.txt.");
    save_clans();
  }
}

void save_clans(void)
{
  FILE *fl;
  clan_data *pclan = nullptr;
  struct clan_room_data *proom = nullptr;
  struct clan_member_data *pmember = nullptr;
  char buf[MAX_STRING_LENGTH];
  char *x;
  char *targ;

  if (!(fl = fopen("../lib/clan.txt", "w")))
  {
    fprintf(stderr, "Unable to open clan.txt for writing.\n");
    abort();
  }

  for (pclan = clan_list; pclan; pclan = pclan->next)
  {
    // print normal data
    fprintf(fl, "%s %s %s %d\n", pclan->leader, pclan->founder, pclan->name,
            pclan->number);
    // print rooms
    for (proom = pclan->rooms; proom; proom = proom->next)
      fprintf(fl, "R %d\n", proom->room_number);
    fprintf(fl, "S\n");

    // BLAH TEMP CODE HERE
    targ = buf;
    for (x = pclan->email; x && *x != '\0'; x++)
      if (*x != '\r')
        *targ++ = *x;
    *targ = '\0';
    // handle email
    if (pclan->email)
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
    for (x = pclan->description; x && *x != '\0'; x++)
      if (*x != '\r')
        *targ++ = *x;
    *targ = '\0';

    // handle description
    if (pclan->description)
      fprintf(fl, "D\n%s~\n", buf);
    //       fprintf(fl, "D\n%s~\n", pclan->description);

    // BLAH TEMP CODE HERE
    targ = buf;
    for (x = pclan->login_message; x && *x != '\0'; x++)
      if (*x != '\r')
        *targ++ = *x;
    *targ = '\0';

    if (pclan->login_message)
      fprintf(fl, "L\n%s~\n", buf);
    //  fprintf(fl, "L\n%s~\n", pclan->login_message);

    if (pclan->tax)
      fprintf(fl, "T\n%d\n", pclan->tax);

    if (pclan->getBalance())
      fprintf(fl, "B\n%llu\n", pclan->getBalance());

    // BLAH TEMP CODE HERE
    targ = buf;
    for (x = pclan->death_message; x && *x != '\0'; x++)
      if (*x != '\r')
        *targ++ = *x;
    *targ = '\0';
    if (pclan->death_message)
      fprintf(fl, "X\n%s~\n", buf);
    // fprintf(fl, "X\n%s~\n", pclan->death_message);

    // BLAH TEMP CODE HERE
    targ = buf;
    for (x = pclan->logout_message; x && *x != '\0'; x++)
      if (*x != '\r')
        *targ++ = *x;
    *targ = '\0';
    if (pclan->logout_message)
      fprintf(fl, "O\n%s~\n", buf);
    // fprintf(fl, "O\n%s~\n", pclan->logout_message);

    // BLAH TEMP CODE HERE
    targ = buf;
    for (x = pclan->clanmotd; x && *x != '\0'; x++)
      if (*x != '\r')
        *targ++ = *x;
    *targ = '\0';

    if (pclan->clanmotd)
      fprintf(fl, "C\n%s~\n", buf);
    //       fprintf(fl, "C\n%s~\n", pclan->clanmotd);

    for (pmember = pclan->members; pmember; pmember = pmember->next)
    {
      fprintf(fl, "M\n%s~\n", pmember->member_name);
      fprintf(fl, "%d %d %d %d %d %d\n", pmember->member_rights,
              pmember->member_rank, pmember->unused1, pmember->unused2,
              pmember->unused3, pmember->time_joined);
      fprintf(fl, "%s~\n", pmember->unused4);
    }

    // terminate clan
    fprintf(fl, "~\n");
  }
  fprintf(fl, "~\n");
  fclose(fl);

  in_port_t port1 = 0;
  if (DC::getInstance()->cf.ports.size() > 0)
  {
    port1 = DC::getInstance()->cf.ports[0];
  }

  stringstream ssbuffer;
  ssbuffer << HTDOCS_DIR << port1 << "/" << WEBCLANSLIST_FILE;
  if (!(fl = fopen(ssbuffer.str().c_str(), "w")))
  {
    logf(0, LogChannels::LOG_MISC, "Unable to open web clan file \'%s\' for writing.\n", ssbuffer.str().c_str());
    return;
  }

  for (pclan = clan_list; pclan; pclan = pclan->next)
  {
    fprintf(fl, "%s %s %d\n", pclan->name, pclan->leader, pclan->number);
    fprintf(fl, "$3Contact Email$R:  %s\n"
                "$3Clan Hall$R:      %s\n"
                "$3Clan info$R:\n"
                "$3----------$R\n",
            pclan->email ? pclan->email : "(No Email)",
            pclan->rooms ? "Yes" : "No");
    // This has to be separate, or if the leader uses $'s, it comes out funky
    fprintf(fl, "%s\n",
            pclan->description ? pclan->description : "(No Description)\r\n");
  }
  fclose(fl);
}

void delete_clan(const clan_data *currclan)
{
  struct clan_room_data *curr_room = nullptr;
  struct clan_room_data *next_room = nullptr;
  struct clan_member_data *curr_member = nullptr;
  struct clan_member_data *next_member = nullptr;

  for (curr_room = currclan->rooms; curr_room; curr_room = next_room)
  {
    next_room = curr_room->next;
    dc_free(curr_room);
  }
  for (curr_member = currclan->members; curr_member; curr_member =
                                                         next_member)
  {
    next_member = curr_member->next;
    free_member(curr_member);
  }
  if (currclan->description)
    dc_free(currclan->description);
  if (currclan->email)
    dc_free(currclan->email);
  if (currclan->login_message)
    dc_free(currclan->login_message);
  if (currclan->logout_message)
    dc_free(currclan->logout_message);
  if (currclan->death_message)
    dc_free(currclan->death_message);

  delete currclan;
}

void free_clans_from_memory(void)
{
  clan_data *currclan = nullptr;
  clan_data *nextclan = nullptr;

  for (currclan = clan_list; currclan; currclan = nextclan)
  {
    nextclan = currclan->next;
    delete_clan(currclan);
  }
}

void assign_clan_rooms()
{
  clan_data *clan = 0;
  struct clan_room_data *room = 0;

  for (clan = clan_list; clan; clan = clan->next)
    for (room = clan->rooms; room; room = room->next)
      if (-1 != real_room(room->room_number))
        if (!IS_SET(world[real_room(room->room_number)].room_flags, CLAN_ROOM))
          SET_BIT(world[real_room(room->room_number)].room_flags, CLAN_ROOM);
}

struct clan_member_data *get_member(char *strName, int nClanId)
{
  clan_data *theClan = nullptr;

  if (!(theClan = get_clan(nClanId)) || !strName)
    return nullptr;

  struct clan_member_data *pcurr = theClan->members;

  while (pcurr && strcmp(pcurr->member_name, strName))
    pcurr = pcurr->next;

  return pcurr;
}

int is_in_clan(clan_data *theClan, Character *ch)
{
  struct clan_member_data *pcurr = theClan->members;

  while (pcurr)
  {
    if (!strcmp(pcurr->member_name, GET_NAME(ch)))
      return 1;
    pcurr = pcurr->next;
  }

  return 0;
}

void remove_clan_member(int clannumber, Character *ch)
{
  clan_data *pclan = nullptr;

  if (!(pclan = get_clan(clannumber)))
    return;

  remove_clan_member(pclan, ch);
}

void remove_clan_member(clan_data *theClan, Character *ch)
{
  struct clan_member_data *pcurr = nullptr;
  struct clan_member_data *plast = nullptr;

  pcurr = theClan->members;

  while (pcurr && strcmp(pcurr->member_name, GET_NAME(ch)))
  {
    plast = pcurr;
    pcurr = pcurr->next;
  }

  if (!pcurr) // didn't find it
    return;

  if (!plast) // was first one in list
    theClan->members = pcurr->next;
  else // somewhere in the list
    plast->next = pcurr->next;

  free_member(pcurr);
}

// Add someone.  Just makes the struct, fills it, then calls the other add_clan_member
void add_clan_member(clan_data *theClan, Character *ch)
{
  struct clan_member_data *pmember = nullptr;

  if (!ch || !theClan)
  {
    logentry("add_clan_member(clan, ch) called with a null.", ANGEL, LogChannels::LOG_BUG);
    return;
  }

#ifdef LEAK_CHECK
  pmember = (struct clan_member_data *)calloc(1, sizeof(struct clan_member_data));
#else
  pmember = (struct clan_member_data *)dc_alloc(1, sizeof(struct clan_member_data));
#endif

  pmember->member_name = str_dup(GET_NAME(ch));
  pmember->member_rights = 0;
  pmember->member_rank = 0;
  pmember->unused1 = 0;
  pmember->unused2 = 0;
  pmember->unused3 = 0;
  pmember->time_joined = 0;
  pmember->unused4 = str_dup("");
  pmember->next = nullptr;

  add_clan_member(theClan, pmember);
}

// This should really be done as a binary tree, but I'm lazy, and this doesn't get used
// very often, so it's just a linked list sorted by member name
void add_clan_member(clan_data *theClan, struct clan_member_data *new_new_member)
{
  struct clan_member_data *pcurr = nullptr;
  struct clan_member_data *plast = nullptr;
  int result = 0;

  if (!new_new_member || !theClan)
  {
    logentry("add_clan_member(clan, member) called with a null.", ANGEL, LogChannels::LOG_BUG);
    return;
  }

  if (!*new_new_member->member_name)
  {
    logentry("Attempt to add a blank member name to a clan.", ANGEL, LogChannels::LOG_BUG);
    return;
  }

  if (!theClan->members)
  {
    theClan->members = new_new_member;
    new_new_member->next = nullptr;
    return;
  }

  pcurr = theClan->members;

  while (pcurr && (0 > (result = strcmp(pcurr->member_name, new_new_member->member_name))))
  {
    plast = pcurr;
    pcurr = pcurr->next;
  }

  if (result == 0)
  { // found um, get out
    logentry("Tried to add already existing clan member.  Possible memory leak.", ANGEL, LogChannels::LOG_BUG);
    return;
  }

  if (pcurr && !plast)
  { // we're at the beginning
    new_new_member->next = theClan->members;
    theClan->members = new_new_member;
    return;
  }

  if (!pcurr)
  { // we hit the end of the list
    plast->next = new_new_member;
    new_new_member->next = nullptr;
    return;
  }

  // if we hit here, then we found our insertion point
  new_new_member->next = pcurr;
  plast->next = new_new_member;
}

void add_clan(clan_data *new_new_clan)
{
  clan_data *pcurr = nullptr;
  clan_data *plast = nullptr;

  if (!clan_list)
  {
    clan_list = new_new_clan;
    end_clan_list = new_new_clan;
    return;
  }

  plast = clan_list;
  pcurr = clan_list->next;

  while (pcurr)
    if (pcurr->number > new_new_clan->number)
    {
      plast->next = new_new_clan;
      new_new_clan->next = pcurr;
      return;
    }
    else
    {
      plast = pcurr;
      pcurr = pcurr->next;
    }

  end_clan_list->next = new_new_clan;
  end_clan_list = new_new_clan;
}

void free_member(struct clan_member_data *member)
{
  dc_free(member->member_name);
  dc_free(member->unused4);
  dc_free(member);
}

void delete_clan(clan_data *dead_clan)
{
  clan_data *last = 0;
  clan_data *curr = 0;
  struct clan_room_data *room = 0;
  struct clan_room_data *nextroom = 0;

  if (!clan_list)
    return;
  if (!dead_clan)
    return;

  if (clan_list == dead_clan)
  {
    if (dead_clan == end_clan_list) // Only 1 clan total
      end_clan_list = 0;
    clan_list = dead_clan->next;
    delete dead_clan;
    return;
  }

  // This works since the first clan is not the dead_clan
  curr = clan_list;
  while (curr)
    if (curr == dead_clan)
    {
      last->next = curr->next;

      if ((curr = end_clan_list))
        end_clan_list = last;

      if (dead_clan->rooms)
      {
        room = dead_clan->rooms;
        nextroom = room->next;
        if (real_room(room->room_number) != NOWHERE)
          if (IS_SET(world[real_room(room->room_number)].room_flags, CLAN_ROOM))
            REMOVE_BIT(world[real_room(room->room_number)].room_flags, CLAN_ROOM);

        dc_free(room);

        while (nextroom)
        {
          room = nextroom;
          nextroom = room->next;
          dc_free(room);
        }
      }

      if (dead_clan->email)
        dc_free(dead_clan->email);
      if (dead_clan->login_message)
        dc_free(dead_clan->login_message);
      if (dead_clan->logout_message)
        dc_free(dead_clan->logout_message);
      if (dead_clan->death_message)
        dc_free(dead_clan->death_message);
      if (dead_clan->description)
        dc_free(dead_clan->description);
      delete dead_clan;
      return;
    }
    else
    {
      last = curr;
      curr = curr->next;
    }
}

int plr_rights(Character *ch)
{
  struct clan_member_data *pmember = nullptr;

  if (!ch || !(pmember = get_member(GET_NAME(ch), ch->clan)))
    return false;

  return pmember->member_rights;
}

// see if ch has rights to 'bit' in his clan
int has_right(Character *ch, uint32_t bit)
{
  struct clan_member_data *pmember = nullptr;

  if (!ch || !(pmember = get_member(GET_NAME(ch), ch->clan)))
    return false;

  return IS_SET(pmember->member_rights, bit);
}

int num_clan_members(clan_data *clan)
{
  int i = 0;
  for (struct clan_member_data *pmem = clan->members;
       pmem;
       pmem = pmem->next)
    i++;

  return i;
}

clan_data *get_clan(int nClan)
{
  clan_data *clan = nullptr;

  if (nClan == 0)
    return nullptr;

  for (clan = clan_list; clan; clan = clan->next)
    if (nClan == clan->number)
      return clan;

  return 0;
}

clan_data *get_clan(Character *ch)
{
  if (ch == 0)
  {
    return nullptr;
  }

  clan_data *clan;

  for (clan = clan_list; clan; clan = clan->next)
    if (ch->clan == clan->number)
      return clan;

  ch->clan = 0;
  return nullptr;
}

char *get_clan_name(int nClan)
{

  clan_data *clan = get_clan(nClan);

  if (clan)
    return clan->name;

  return "no clan";
}

char *get_clan_name(Character *ch)
{
  clan_data *clan = get_clan(ch);

  if (clan)
    return clan->name;

  return "no clan";
}

char *get_clan_name(clan_data *clan)
{
  if (clan)
    return clan->name;

  return "no clan";
}

void message_to_clan(Character *ch, char buf[])
{
  Character *pch;

  for (Connection *d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    if (d->connected || !(pch = d->character))
      continue;
    if (pch->clan != ch->clan || pch == ch)
      continue;

    ansi_color(YELLOW, pch);
    send_to_char("-->> ", pch);
    ansi_color(RED, pch);
    ansi_color(BOLD, pch);
    send_to_char(buf, pch);
    ansi_color(NTEXT, pch);
    ansi_color(YELLOW, pch);
    send_to_char(" <<--\n\r", pch);
    ansi_color(GREY, pch);
  }
}

void clan_death(Character *ch, Character *killer)
{
  if (!ch || ch->clan == 0)
    return;

  char buf[400];
  char secondbuf[400];
  clan_data *clan;
  char *curr = nullptr;

  if (!(clan = get_clan(ch->clan)))
  {
    send_to_char("You have an illegal clan number.  Contact a god.\r\n", ch);
    return;
  }

  if (!clan->death_message)
    return;

  // Don't give away any imms listening in
  if (GET_LEVEL(ch) >= IMMORTAL)
    return;

  if (!(curr = strstr(clan->death_message, "%")))
  {
    send_to_char("Error:  clan with illegal death_message.  Contact a god.\r\n", ch);
    return;
  }

  *curr = '\0';
  sprintf(buf, "%s%s%s", clan->death_message, GET_SHORT(ch), curr + 1);
  *curr = '%';

  if (!(curr = strstr(buf, "#")))
  {
    send_to_char("Error:  clan with illegal death_message.  Contact a god.\r\n", ch);
    return;
  }

  *curr = '\0';
  sprintf(secondbuf, "%s%s%s", buf, (killer ? GET_SHORT(killer) : "unknown"), curr + 1);

  message_to_clan(ch, secondbuf);
}

void clan_login(Character *ch)
{
  if (ch->clan == 0)
    return;

  char buf[400];
  clan_data *clan;
  char *curr = nullptr;

  if (!(clan = get_clan(ch->clan)))
  {
    // illegal clan number.  Set him to 0 and get out
    ch->clan = 0;
    return;
  }

  // Don't give away any imms listening in
  if (GET_LEVEL(ch) >= IMMORTAL)
  {
    return;
  }

  if (!is_in_clan(clan, ch))
  {
    send_to_char("You were kicked out of your clan.\r\n", ch);
    ch->clan = 0;
    return;
  }

  if (!clan->login_message)
    return;

  if (!(curr = strstr(clan->login_message, "%")))
  {
    send_to_char("Error:  clan with illegal login_message.  Contact a god.\r\n", ch);
    return;
  }

  *curr = '\0';
  sprintf(buf, "%s%s%s", clan->login_message, GET_NAME(ch), curr + 1);
  *curr = '%';

  message_to_clan(ch, buf);
}

void clan_logout(Character *ch)
{
  if (ch->clan == 0)
    return;

  char buf[400];
  clan_data *clan;
  char *curr = nullptr;

  if (!(clan = get_clan(ch->clan)))
  {
    send_to_char("You have an illegal clan number.  Contact a god.\r\n", ch);
    return;
  }

  // Don't give away any imms listening in
  if (GET_LEVEL(ch) >= IMMORTAL)
  {
    return;
  }

  if (!clan->logout_message)
    return;

  if (!(curr = strstr(clan->logout_message, "%")))
  {
    send_to_char("Error:  clan with illegal logout_message.  Contact a god.\r\n", ch);
    return;
  }

  *curr = '\0';
  sprintf(buf, "%s%s%s", clan->logout_message, GET_NAME(ch), curr + 1);
  *curr = '%';

  message_to_clan(ch, buf);
}

int do_accept(Character *ch, char *arg, int cmd)
{
  Character *victim;
  clan_data *clan;
  char buf[MAX_STRING_LENGTH];

  while (isspace(*arg))
    arg++;

  if (!*arg)
  {
    send_to_char("Accept who into your clan?\n\r", ch);
    return eFAILURE;
  }

  if (!ch->clan || !(clan = get_clan(ch)))
  {
    send_to_char("You aren't the member of any clan!\n\r", ch);
    return eFAILURE;
  }

  if (strcmp(clan->leader, GET_NAME(ch)) && !has_right(ch, CLAN_RIGHTS_ACCEPT))
  {
    send_to_char("You aren't the leader of your clan!\n\r", ch);
    return eFAILURE;
  }

  one_argument(arg, buf);

  if (!(victim = get_char_room_vis(ch, buf)))
  {
    send_to_char("You can't accept someone into your clan who isn't here!\n\r", ch);
    return eFAILURE;
  }

  if (IS_NPC(victim) || GET_LEVEL(victim) >= IMMORTAL)
  {
    send_to_char("Yeah right.\r\n", ch);
    return eFAILURE;
  }

  if (victim->clan)
  {
    send_to_char("This person already belongs to a clan.\r\n", ch);
    return eFAILURE;
  }

  victim->clan = ch->clan;
  add_clan_member(clan, victim);
  save_clans();
  sprintf(buf, "You are now a member of %s.\r\n", clan->name);
  send_to_char("Your clan now has a new member.\r\n", ch);
  send_to_char(buf, victim);

  sprintf(buf, "%s just joined clan [%s].", GET_NAME(victim), clan->name);
  logentry(buf, IMPLEMENTER, LogChannels::LOG_CLAN);

  add_totem_stats(victim);

  return eSUCCESS;
}

int do_outcast(Character *ch, char *arg, int cmd)
{
  Character *victim;
  clan_data *clan;
  class Connection d;
  char buf[MAX_STRING_LENGTH], tmp_buf[MAX_STRING_LENGTH];
  bool connected = true;

  while (isspace(*arg))
    arg++;

  if (!*arg)
  {
    send_to_char("Cast who out of your clan?\n\r", ch);
    return eFAILURE;
  }

  if (!ch->clan || !(clan = get_clan(ch)))
  {
    send_to_char("You aren't the member of any clan!\n\r", ch);
    return eFAILURE;
  }

  one_argument(arg, buf);

  buf[0] = UPPER(buf[0]);
  if (!(victim = get_char(buf)))
  {

    // must be done to clear out "d" before it is used
    d = {};

    if (!(load_char_obj(&d, buf)))
    {

      sprintf(tmp_buf, "../archive/%s.gz", buf);
      if (file_exists(tmp_buf))
        send_to_char("Character is archived.\r\n", ch);
      else
        send_to_char("Unable to outcast, type the entire name.\r\n", ch);
      return eFAILURE;
    }

    victim = d.character;
    victim->desc = 0;

    victim->hometown = START_ROOM;
    victim->in_room = START_ROOM;
    connected = false;
  }

  if (!victim->clan)
  {
    send_to_char("This person isn't in a clan in the first place...\r\n", ch);
    return eFAILURE;
  }

  if (strcmp(clan->leader, GET_NAME(ch)) &&
      victim != ch &&
      !has_right(ch, CLAN_RIGHTS_OUTCAST))
  {
    send_to_char("You don't have the right to outcast people from your clan!\n\r", ch);
    return eFAILURE;
  }

  if (!strcmp(clan->leader, GET_NAME(victim)))
  {
    send_to_char("You can't outcast the clan leader!\n\r", ch);
    return eFAILURE;
  }

  if (victim == ch)
  {
    sprintf(buf, "%s just quit clan [%s].", GET_NAME(victim), clan->name);
    logentry(buf, IMPLEMENTER, LogChannels::LOG_CLAN);
    send_to_char("You quit your clan.\r\n", ch);
    remove_totem_stats(victim);
    victim->clan = 0;
    remove_clan_member(clan, ch);
    save_clans();
    return eSUCCESS;
  }

  if (victim->clan != ch->clan)
  {
    send_to_char("That person isn't in your clan!\r\n", ch);
    return eFAILURE;
  }

  remove_totem_stats(victim);
  victim->clan = 0;
  remove_clan_member(clan, victim);
  save_clans();
  sprintf(buf, "You cast %s out of your clan.\r\n", GET_NAME(victim));
  send_to_char(buf, ch);
  sprintf(buf, "You are cast out of %s.\r\n", clan->name);
  send_to_char(buf, victim);

  sprintf(buf, "%s was outcasted from clan [%s].", GET_NAME(victim), clan->name);
  logentry(buf, IMPLEMENTER, LogChannels::LOG_CLAN);

  victim->save(666);
  if (!connected)
    free_char(victim, Trace("do_outcast"));

  return eSUCCESS;
}

int do_cpromote(Character *ch, char *arg, int cmd)
{
  Character *victim;
  clan_data *clan;
  char buf[MAX_STRING_LENGTH];

  while (isspace(*arg))
    arg++;

  if (!*arg)
  {
    send_to_char("Who do you want to make the new clan leader?\n\r", ch);
    return eFAILURE;
  }

  if (!ch->clan || !(clan = get_clan(ch)))
  {
    send_to_char("You aren't the member of any clan!\n\r", ch);
    return eFAILURE;
  }

  if (!isname(clan->leader, GET_NAME(ch)))
  {
    send_to_char("You aren't the leader of your clan!\n\r", ch);
    return eFAILURE;
  }

  one_argument(arg, buf);

  if (!(victim = get_char_room_vis(ch, buf)))
  {
    send_to_char("You can't cpromote someone who isn't here!\n\r",
                 ch);
    return eFAILURE;
  }

  if (IS_NPC(victim))
  {
    send_to_char("Yeah right.\r\n", ch);
    return eFAILURE;
  }

  if (victim->clan != ch->clan)
  {
    send_to_char("You can not cpromote someone who doesn't belong to the "
                 "clan.\r\n",
                 ch);
    return eFAILURE;
  }
  if (clan->leader)
    dc_free(clan->leader);
  clan->leader = str_dup(GET_NAME(victim));

  save_clans();

  sprintf(buf, "You are now the leader of %s.\r\n", clan->name);
  send_to_char("Your clan now has a new leader.\r\n", ch);
  send_to_char(buf, victim);

  sprintf(buf, "%s just cpromoted by %s as leader of clan [%s].", GET_NAME(victim), GET_NAME(ch), clan->name);
  logentry(buf, IMPLEMENTER, LogChannels::LOG_CLAN);
  return eSUCCESS;
}

int clan_desc(Character *ch, char *arg)
{
  clan_data *clan = 0;

  char buf[MAX_STRING_LENGTH];
  char text[MAX_INPUT_LENGTH];

  clan = get_clan(ch);
  arg = one_argumentnolow(arg, text);

  if (!strncmp(text, "delete", 6))
  {
    if (clan->description)
      dc_free(clan->description);
    clan->description = nullptr;
    send_to_char("Clan description removed.\r\n", ch);
    return 1;
  }

  if (strcmp(text, "change"))
  {
    sprintf(buf, "$3Syntax$R:  clans description change\r\n\r\nCurrent description: %s\r\n",
            clan->description ? clan->description : "(No Description)");
    send_to_char(buf, ch);
    send_to_char("To not have any description use:  clans description delete\r\n", ch);
    return 0;
  }

  /*  if(clan->description)
      dc_free(clan->description);
    clan->description = nullptr;
  */
  // send_to_char("Write new description.  ~ to end.\r\n", ch);

  //  ch->desc->connected = conn::EDITING;
  //  ch->desc->str = &clan->description;
  //  ch->desc->max_str = MAX_CLAN_DESC_LENGTH;
  ch->desc->backstr = nullptr;
  send_to_char("        Write your description and stay within the line.  (/s saves /h for help)\r\n"
               "   |--------------------------------------------------------------------------------|\r\n",
               ch);
  if (clan->description)
  {
    ch->desc->backstr = str_dup(clan->description);
    send_to_char(ch->desc->backstr, ch);
  }

  ch->desc->connected = conn::EDITING;
  ch->desc->strnew = &(clan->description);
  ch->desc->max_str = MAX_CLAN_DESC_LENGTH;
  return 1;
}

int clan_motd(Character *ch, char *arg)
{
  clan_data *clan = 0;

  char buf[MAX_STRING_LENGTH];
  char text[MAX_INPUT_LENGTH];

  clan = get_clan(ch);
  arg = one_argumentnolow(arg, text);

  if (!strncmp(text, "delete", 6))
  {
    if (clan->clanmotd)
      dc_free(clan->clanmotd);
    clan->clanmotd = nullptr;
    send_to_char("Clan motd removed.\r\n", ch);
    return 1;
  }

  if (strcmp(text, "change"))
  {
    sprintf(buf, "$3Syntax$R:  clans motd change\r\n\r\nCurrent motd: %s\r\n",
            clan->clanmotd ? clan->clanmotd : "(No Motd)");
    send_to_char(buf, ch);
    send_to_char("To not have any motd use:  clans motd delete\r\n", ch);
    return 0;
  }

  /*  if(clan->clanmotd)
      dc_free(clan->clanmotd);
    clan->clanmotd = nullptr;
  */
  // send_to_char("Write new motd.  ~ to end.\r\n", ch);

  // ch->desc->connected = conn::EDITING;
  // ch->desc->str = &clan->clanmotd;
  // ch->desc->max_str = MAX_CLAN_DESC_LENGTH;

  ch->desc->backstr = nullptr;
  send_to_char("        Write your motd and stay within the line.  (/s saves /h for help)\r\n"
               "   |--------------------------------------------------------------------------------|\r\n",
               ch);
  if (clan->clanmotd)
  {
    ch->desc->backstr = str_dup(clan->clanmotd);
    send_to_char(ch->desc->backstr, ch);
  }

  ch->desc->connected = conn::EDITING;
  ch->desc->strnew = &(clan->clanmotd);
  ch->desc->max_str = MAX_CLAN_DESC_LENGTH;
  return 1;
}

int clan_death_message(Character *ch, char *arg)
{
  clan_data *clan = 0;

  char buf[MAX_STRING_LENGTH];

  clan = get_clan(ch);
  if (!*arg)
  {
    sprintf(buf, "$3Syntax$R:  clans death <new message>\r\n\r\nCurrent message: %s\r\n",
            clan->death_message);
    send_to_char(buf, ch);
    send_to_char("To not have any message use:  clans death delete\r\n", ch);
    return 0;
  }

  one_argument(arg, buf);
  if (!strncmp(buf, "delete", 6))
  {
    send_to_char("Clan death message removed.\r\n", ch);
    if (clan->death_message)
      dc_free(clan->death_message);
    clan->death_message = nullptr;
    return 1;
  }

  if (strstr(arg, "~"))
  {
    send_to_char("No ~s fatt butt!\r\n", ch);
    return 0;
  }

  char *curr;

  if (!(curr = strstr(arg, "%")))
  {
    send_to_char("You must include a '%' to represent the victim's name.\r\n", ch);
    return 0;
  }

  if (strstr(curr + 1, "%"))
  {
    send_to_char("You may only have one '%' in the message.\r\n", ch);
    return 0;
  }

  if (!(curr = strstr(arg, "#")))
  {
    send_to_char("You must include a '#' to represent the killer's name.\r\n", ch);
    return 0;
  }

  if (strstr(curr + 1, "#"))
  {
    send_to_char("You may only have one '#' in the message.\r\n", ch);
    return 0;
  }

  if (clan->death_message)
    dc_free(clan->death_message);

  clan->death_message = str_dup(arg);

  sprintf(buf, "Clan death message changed to: %s\r\n", clan->death_message);
  send_to_char(buf, ch);
  return 1;
}

int clan_logout_message(Character *ch, char *arg)
{
  clan_data *clan = 0;

  char buf[MAX_STRING_LENGTH];

  clan = get_clan(ch);
  if (!*arg)
  {
    sprintf(buf, "$3Syntax$R:  clans logout <new message>\r\n\r\nCurrent message: %s\r\n",
            clan->logout_message);
    send_to_char(buf, ch);
    send_to_char("To not have any message use:  clans logout delete\r\n", ch);
    return 0;
  }

  one_argument(arg, buf);
  if (!strncmp(buf, "delete", 6))
  {
    send_to_char("Clan logout message removed.\r\n", ch);
    if (clan->logout_message)
      dc_free(clan->logout_message);
    clan->logout_message = nullptr;
    return 1;
  }

  if (strstr(arg, "~"))
  {
    send_to_char("No ~s fatt butt!\r\n", ch);
    return 0;
  }

  char *curr;

  if (!(curr = strstr(arg, "%")))
  {
    send_to_char("You must include a '%' to represent the person's name.\r\n", ch);
    return 0;
  }

  if (strstr(curr + 1, "%"))
  {
    send_to_char("You may only have one '%' in the message.\r\n", ch);
    return 0;
  }

  if (clan->logout_message)
    dc_free(clan->logout_message);

  clan->logout_message = str_dup(arg);

  sprintf(buf, "Clan logout message changed to: %s\r\n", clan->logout_message);
  send_to_char(buf, ch);
  return 1;
}

int clan_login_message(Character *ch, char *arg)
{
  clan_data *clan = 0;

  char buf[MAX_STRING_LENGTH];

  clan = get_clan(ch);
  if (!*arg)
  {
    sprintf(buf, "$3Syntax$R:  clans login <new message>\r\n\r\nCurrent message: %s\r\n",
            clan->login_message);
    send_to_char(buf, ch);
    send_to_char("To not have any message use:  clans login delete\r\n", ch);
    return 0;
  }

  one_argument(arg, buf);
  if (!strncmp(buf, "delete", 6))
  {
    send_to_char("Clan login message removed.\r\n", ch);
    if (clan->login_message)
      dc_free(clan->login_message);
    clan->login_message = nullptr;
    return 1;
  }

  if (strstr(arg, "~"))
  {
    send_to_char("No ~s fatt butt!\r\n", ch);
    return 0;
  }

  char *curr;

  if (!(curr = strstr(arg, "%")))
  {
    send_to_char("You must include a '%' to represent the person's name.\r\n", ch);
    return 0;
  }

  if (strstr(curr + 1, "%"))
  {
    send_to_char("You may only have one '%' in the message.\r\n", ch);
    return 0;
  }

  if (clan->login_message)
    dc_free(clan->login_message);

  clan->login_message = str_dup(arg);

  sprintf(buf, "Clan login message changed to: %s\r\n", clan->login_message);
  send_to_char(buf, ch);
  return 1;
}

int clan_email(Character *ch, char *arg)
{
  clan_data *clan = 0;

  char buf[MAX_STRING_LENGTH];
  char text[MAX_INPUT_LENGTH];

  clan = get_clan(ch);
  arg = one_argumentnolow(arg, text);
  if (!*text)
  {
    sprintf(buf, "$3Syntax$R:  clans email <new address>\r\n\r\nCurrent address: %s\r\n", clan->email);
    send_to_char(buf, ch);
    send_to_char("To not have any email use:  clans email delete\r\n", ch);
    return 0;
  }

  if (!strncmp(text, "delete", 6))
  {
    if (clan->email)
      dc_free(clan->email);
    clan->email = nullptr;
    send_to_char("Clan email address removed.\r\n", ch);
    return 1;
  }

  if (strstr(text, "~") || strstr(text, "<") || strstr(text, ">") ||
      strstr(text, "&") || strstr(text, "$"))
  {
    send_to_char("We both know those characters aren't legal in email addresses....\r\n", ch);
    return 0;
  }

  if (clan->email)
    dc_free(clan->email);

  clan->email = str_dup(text);

  sprintf(buf, "Clan email changed to: %s\r\n", clan->email);
  send_to_char(buf, ch);
  return 1;
}

int do_ctell(Character *ch, char *arg, int cmd)
{
  Character *pch;
  class Connection *desc;
  char buf[MAX_STRING_LENGTH];

  if (!ch->clan)
  {
    send_to_char("But you don't belong to a clan!\n\r", ch);
    return eFAILURE;
  }

  Object *tmp_obj;
  for (tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      send_to_char("The magical silence prevents you from speaking!\n\r", ch);
      return eFAILURE;
    }

  while (isspace(*arg))
    arg++;
  if (!has_right(ch, CLAN_RIGHTS_CHANNEL) && ch->level < 51)
  {
    send_to_char("You don't have the right to talk to your clan.\r\n", ch);
    return eFAILURE;
  }

  if (!(IS_SET(ch->misc, LogChannels::CHANNEL_CLAN)))
  {
    send_to_char("You have that channel off!!\n\r", ch);
    return eFAILURE;
  }

  if (!*arg)
  {
    queue<string> tmp = get_clan(ch)->ctell_history;
    if (tmp.empty())
    {
      send_to_char("No one has said anything lately.\r\n", ch);
      return eFAILURE;
    }

    send_to_char("Here are the last 10 ctells:\r\n", ch);
    while (!tmp.empty())
    {
      send_to_char((tmp.front()).c_str(), ch);
      tmp.pop();
    }

    return eFAILURE;
  }

  sprintf(buf, "You tell the clan, '%s'\n\r", arg);
  ansi_color(GREEN, ch);
  send_to_char(buf, ch);
  ansi_color(NTEXT, ch);

  sprintf(buf, "%s tells the clan, '%s'\n\r", GET_SHORT(ch), arg);
  bool yes;
  for (desc = DC::getInstance()->descriptor_list; desc; desc = desc->next)
  {
    yes = false;
    if (desc->connected || !(pch = desc->character))
      continue;
    if (pch == ch || pch->clan != ch->clan ||
        !IS_SET(pch->misc, LogChannels::CHANNEL_CLAN))
      continue;
    if (!has_right(pch, CLAN_RIGHTS_CHANNEL) && pch->level <= MAX_MORTAL)
      continue;

    for (tmp_obj = world[pch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
      if (obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
      {
        yes = true;
        break;
      }

    if (yes)
      continue;

    ansi_color(GREEN, pch);
    send_to_char(buf, pch);
    ansi_color(NTEXT, pch);
  }

  sprintf(buf, "$2%s tells the clan, '%s'$R\n\r", GET_SHORT(ch), arg);
  get_clan(ch)->ctell_history.push(buf);
  if (get_clan(ch)->ctell_history.size() > 10)
  {
    get_clan(ch)->ctell_history.pop();
  }

  return eSUCCESS;
}

void do_clan_list(Character *ch)
{
  clan_data *clan = 0;
  string buf, buf2;

  if (GET_LEVEL(ch) > 103)
  {
    send_to_char("$B$7## Clan                 Leader           Tax   $B$5Gold$7 Balance$R\n\r", ch);
  }
  else
  {
    send_to_char("$B$7## Clan                 Leader           $R\n\r", ch);
  }

  std::locale("en_US.UTF-8");
  for (clan = clan_list; clan; clan = clan->next)
  {
    if (GET_LEVEL(ch) > 103)
    {
      buf = fmt::format("{:2} {:<20}$R {:<16} {:3} {:16L}\r\n", clan->number, clan->name, clan->leader, clan->tax, clan->getBalance());
    }
    else
    {
      buf = fmt::format("{:2} {:<20}$R {:<16}\r\n", clan->number, clan->name, clan->leader);
    }
    send_to_char(buf, ch);
  }
}

void do_clan_member_list(Character *ch)
{
  struct clan_member_data *pmember = 0;
  clan_data *pclan = 0;
  int column = 1;
  char buf[200], buf2[200];

  if (!(pclan = get_clan(ch->clan)))
  {
    send_to_char("Error:  Not in clan.  Contact a god.\r\n", ch);
    return;
  }

  send_to_char("Members of clan:\n\r", ch);
  sprintf(buf, "  ");

  for (pmember = pclan->members; pmember; pmember = pmember->next)
  {
    sprintf(buf2, "%-20s  ", pmember->member_name);
    send_to_char(buf2, ch);

    if (0 == (column % 3))
    {
      send_to_char("\n\r", ch);
      column = 0;
    }
    else
    {
      send_to_char(" ", ch);
    }
    column++;
  }

  if (column != 0)
  {
    send_to_char("\n\r", ch);
  }
}

int is_clan_leader(Character *ch)
{
  clan_data *pclan = nullptr;

  if (!ch || !(pclan = get_clan(ch->clan)))
    return 0;

  return (!(strcmp(GET_NAME(ch), pclan->leader)));
}

void do_clan_rights(Character *ch, char *arg)
{
  struct clan_member_data *pmember = nullptr;
  Character *victim = nullptr;
  // extern char * clan_rights[]~;

  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char name[MAX_INPUT_LENGTH];
  char last[MAX_INPUT_LENGTH];
  int bit = -1;

  half_chop(arg, name, last);

  if (!*name)
  {
    send_to_char("$3Syntax$R:  clan rights <member> [right]\n\r", ch);
    return;
  }

  *name = toupper(*name);

  if (!(pmember = get_member(name, ch->clan)))
  {
    sprintf(buf, "Could not find '%s' in your clan.\r\n", name);
    send_to_char(buf, ch);
    return;
  }

  if (!*last)
  { // diag
    sprintf(buf, "Rights for %s:\n\r-------------\n\r", pmember->member_name);
    send_to_char(buf, ch);
    for (bit = 0; *clan_rights[bit] != '\n'; bit++)
    {
      sprintf(buf, "  %-15s %s\n\r", clan_rights[bit], (IS_SET(pmember->member_rights, 1 << bit) ? "on" : "off"));
      send_to_char(buf, ch);
    }
    return;
  }

  bit = old_search_block(last, 0, strlen(last), clan_rights, 1);

  if (bit < 0)
  {
    send_to_char("Right not found.\r\n", ch);
    return;
  }
  bit--;

  if (!is_clan_leader(ch) && !has_right(ch, 1 << bit))
  {
    send_to_char("You can't give out rights that you don't have.\r\n", ch);
    return;
  }

  TOGGLE_BIT(pmember->member_rights, 1 << bit);

  if (IS_SET(pmember->member_rights, 1 << bit))
  {
    sprintf(buf, "%s toggled on.\r\n", clan_rights[bit]);
    sprintf(buf2, "%s has given you '%s' rights within your clan.\r\n", GET_SHORT(ch), clan_rights[bit]);
  }
  else
  {
    sprintf(buf, "%s toggled off.\r\n", clan_rights[bit]);
    sprintf(buf2, "%s has taken away '%s' rights within your clan.\r\n", GET_SHORT(ch), clan_rights[bit]);
  }
  send_to_char(buf, ch);

  if ((victim = get_char(pmember->member_name)))
  {
    send_to_char(buf2, victim);
  }

  save_clans();
}

void do_god_clans(Character *ch, char *arg, int cmd)
{
  clan_data *clan = 0;
  clan_data *tarclan = 0;
  struct clan_room_data *newroom = 0;
  struct clan_room_data *lastroom = 0;

  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char select[MAX_INPUT_LENGTH];
  char text[MAX_INPUT_LENGTH];
  char last[MAX_INPUT_LENGTH];

  int i;
  int32_t x;
  int16_t skill;

  const char *god_values[] = {
      "create", "rename", "leader", "delete", "addroom",
      "list", "save", "showrooms", "killroom", "email",
      "description", "login", "logout", "death", "members",
      "rights", "motd", "\n"};

  arg = one_argumentnolow(arg, select);

  if (!*select)
  {
    send_to_char("$3Syntax$R: clans <field> <correct arguments>\r\n"
                 "just clan <field> will give you the syntax for that field.\r\n"
                 "Fields are the following.\r\n",
                 ch);
    strcpy(buf, "\r\n");
    for (i = 1; *god_values[i - 1] != '\n'; i++)
    {
      sprintf(buf + strlen(buf), "%18s", god_values[i - 1]);
      if (!(i % 4))
      {
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
        *buf = '\0';
      }
    }
    if (*buf)
      send_to_char(buf, ch);
    send_to_char("\r\n", ch);
    return;
  }

  skill = old_search_block(select, 0, strlen(select), god_values, 1);
  if (skill < 0)
  {
    send_to_char("That value not recognized.\r\n", ch);
    return;
  }
  skill--;

  switch (skill)
  {
  case 0: /* create */
  {
    arg = one_argumentnolow(arg, text);
    arg = one_argumentnolow(arg, last);
    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans create <clanname> <clannumber>\r\n", ch);
      return;
    }
    if (strlen(text) > 29)
    {
      send_to_char("Clan name too long.\r\n", ch);
      return;
    }
    x = atoi(last);
    if (x < 1 || x > (1 << 8 * sizeof(uint16_t)) - 1)
    {
      csendf(ch, "%d (%d) is an invalid clan number.\r\n", x, (1 << 8 * sizeof(uint16_t)) - 1);
      return;
    }

    if (get_clan(x) != nullptr)
    {
      csendf(ch, "%d is an invalid clan number because it already exists.\r\n", x);
      return;
    }

    clan = new clan_data;
    clan->leader = str_dup(GET_NAME(ch));
    clan->amt = 0;
    clan->founder = str_dup(GET_NAME(ch));
    clan->name = str_dup(text);
    clan->number = x;
    clan->acc = 0;
    clan->rooms = 0;
    clan->next = 0;
    clan->email = nullptr;
    clan->description = nullptr;
    add_clan(clan);
    send_to_char("New clan created.\r\n", ch);
    break;
  }
  case 1: /* rename */
  {
    arg = one_argumentnolow(arg, text);
    arg = one_argumentnolow(arg, last);
    if (!*text || !*last)
    {
      send_to_char("$3Syntax$r: clans rename <targetclannum> <newname>\r\n", ch);
      return;
    }
    x = atoi(text);

    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    if (strlen(last) > 29)
    {
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
    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans leader <clannumber> <leadername>\r\n", ch);
      return;
    }

    if (strlen(last) > 14)
    {
      send_to_char("Clan leader name too long.\r\n", ch);
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    if (tarclan->leader)
      dc_free(tarclan->leader);
    tarclan->leader = str_dup(last);

    send_to_char("Clan leader name changed.\r\n", ch);
    break;
  }
  case 3: /* delete */
  {
    arg = one_argumentnolow(arg, text);
    one_argumentnolow(arg, last);
    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans delete <clannumber> dElEtE\r\n", ch);
      return;
    }
    if (!isname(last, "dElEtE"))
    {
      send_to_char("You MUST end the line with 'dElEtE' to delete the clan.\r\n", ch);
      return;
    }
    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
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
    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans addroom <clannumber> <roomnumber>\r\n", ch);
      return;
    }
    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    skill = atoi(last);
    if (-1 == real_room(skill))
    {
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
    if (!*text)
    {
      send_to_char("$3Syntax$R: clans showrooms <clannumber>\r\n", ch);
      return;
    }
    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    if (!tarclan->rooms)
    {
      send_to_char("This clan has no rooms set.\r\n", ch);
      return;
    }

    send_to_char("Rooms\r\n-----\r\n", ch);
    strcpy(buf2, "\r\n");
    for (newroom = tarclan->rooms; newroom; newroom = newroom->next)
    {
      if (newroom->room_number)
      {
        sprintf(buf, "%d\r\n", newroom->room_number);
      }
      else
      {
        strcpy(buf, "Room Data without number.  PROBLEM.\r\n");
      }
      strncat(buf2, buf, sizeof(buf2) - 1);
      buf2[sizeof(buf2) - 1] = 0;
    }

    send_to_char(buf2, ch);
    break;
  }
  case 8: /* killroom */
  {
    arg = one_argumentnolow(arg, text);
    one_argumentnolow(arg, last);
    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans killroom <clannumber> <roomnumber>\r\n", ch);
      return;
    }
    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    if (!tarclan->rooms)
    {
      send_to_char("Error.  Target clan has no rooms.\r\n", ch);
      return;
    }

    skill = atoi(last);
    if (tarclan->rooms->room_number == skill)
    {
      if (-1 != real_room(skill))
        if (IS_SET(world[real_room(skill)].room_flags, CLAN_ROOM))
          REMOVE_BIT(world[real_room(skill)].room_flags, CLAN_ROOM);
      lastroom = tarclan->rooms;
      tarclan->rooms = tarclan->rooms->next;
      dc_free(lastroom);
      send_to_char("Deleted.\r\n", ch);
      return;
    }

    newroom = tarclan->rooms;
    while (newroom)
      if (newroom->room_number == skill)
      {
        if (-1 != real_room(skill))
          if (IS_SET(world[real_room(skill)].room_flags, CLAN_ROOM))
            REMOVE_BIT(world[real_room(skill)].room_flags, CLAN_ROOM);
        lastroom->next = newroom->next;
        dc_free(newroom);
        send_to_char("Deleted.\r\n", ch);
        return;
      }
      else
      {
        lastroom = newroom;
        newroom = newroom->next;
      }

    send_to_char("Specified room number not found.\r\n", ch);
    break;
  }

  case 9:
  { /* email */

    arg = one_argumentnolow(arg, text);
    one_argumentnolow(arg, last);

    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans email <clannumber> <address>\r\n", ch);
      send_to_char("To not have any email use:  clans email <clannumber> delete\r\n", ch);
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    i = ch->clan;
    ch->clan = x;
    clan_email(ch, last);
    ch->clan = i;

    break;
  }

  case 10:
  { /* description */

    arg = one_argumentnolow(arg, text);
    one_argumentnolow(arg, last);

    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans description <clannumber> change\r\n", ch);
      send_to_char("To not have any description use:  clans description <clannumber> delete\r\n", ch);
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    i = ch->clan;
    ch->clan = x;
    clan_desc(ch, last);
    ch->clan = i;

    break;
  }
  case 11:
  { /* login */
    half_chop(arg, text, last);

    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans login <clannumber> <login message>\r\n", ch);
      send_to_char("To not have any message use:  clans login <clannumber> delete\r\n", ch);
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    i = ch->clan;
    ch->clan = x;
    clan_login_message(ch, last);
    ch->clan = i;
    break;
  }
  case 12:
  { /* logout */

    // arg = one_argumentnolow(arg, text);
    // one_argumentnolow(arg, last);
    half_chop(arg, text, last);

    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans logout <clannumber> <logout message>\r\n", ch);
      send_to_char("To not have any message use:  clans logout <clannumber> delete\r\n", ch);
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    i = ch->clan;
    ch->clan = x;
    clan_logout_message(ch, last);
    ch->clan = i;

    break;
  }
  case 13:
  { /* death */
    half_chop(arg, text, last);

    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans death <clannumber> <death message>\r\n", ch);
      send_to_char("To not have any message use:  clans death <clannumber> delete\r\n", ch);
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    i = ch->clan;
    ch->clan = x;
    clan_death_message(ch, last);
    ch->clan = i;

    break;
  }
  case 14:
  { // members
    one_argument(arg, text);

    if (!*text)
    {
      send_to_char("$3Syntax$R: clans members <clannumber>\r\n", ch);
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    i = ch->clan;
    ch->clan = x;
    do_clan_member_list(ch);
    ch->clan = i;

    break;
  }
  case 15:
  { // rights
    half_chop(arg, text, last);

    if (!*text)
    {
      send_to_char("$3Syntax$R: clans rights <clannumber>\r\n", ch);
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    i = ch->clan;
    ch->clan = x;
    do_clan_rights(ch, last);
    ch->clan = i;
    break;
  }
  case 16:
  { // motd

    arg = one_argumentnolow(arg, text);
    one_argumentnolow(arg, last);

    if (!*text || !*last)
    {
      send_to_char("$3Syntax$R: clans motd <clannumber> change\r\n", ch);
      send_to_char("To not have any motd use:  clans motd <clannumber> delete\r\n", ch);
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      send_to_char("Invalid clan number.\r\n", ch);
      return;
    }

    i = ch->clan;
    ch->clan = x;
    clan_motd(ch, last);
    ch->clan = i;

    break;
  }
  default:
  {
    send_to_char("Default hit in clans switch statement.\r\n", ch);
    return;
    break;
  }
  }
}

void do_leader_clans(Character *ch, char *arg, int cmd)
{
  struct clan_member_data *pmember = 0;
  //  clan_data * tarclan = 0;
  //  struct clan_room_data * newroom = 0;
  //  struct clan_room_data * lastroom = 0;

  char buf[MAX_STRING_LENGTH];
  char select[MAX_STRING_LENGTH];
  //  char text[MAX_STRING_LENGTH];
  //  char last[MAX_STRING_LENGTH];

  int i, j, leader;
  //  int x;
  int16_t skill;

  const char *mortal_values[] = {
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
      "log",
      "\n"};

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
      CLAN_RIGHTS_LOG,
      -1};

  if (!(pmember = get_member(GET_NAME(ch), ch->clan)))
  {
    send_to_char("Error:  no clan in do_clans_leader\n\r", ch);
    return;
  }

  leader = is_clan_leader(ch);

  arg = one_argumentnolow(arg, select);

  if (!*select)
  {
    send_to_char("$3Syntax$R: clans <field> <correct arguments>\r\n"
                 "just clan <field> will give you the syntax for that field.\r\n"
                 "Fields are the following.\r\n",
                 ch);
    strcpy(buf, "\r\n");
    j = 1;
    for (i = 0; *mortal_values[i] != '\n'; i++)
    {
      // only show rights the player has.  Leader has all.
      if (!leader && right_required[i] &&
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
    if (*buf)
      send_to_char(buf, ch);
    send_to_char("\r\n", ch);
    return;
  }

  skill = old_search_block(select, 0, strlen(select), mortal_values, 1);
  if (skill < 0)
  {
    send_to_char("That value not recognized.\r\n", ch);
    return;
  }
  skill--;

  if (!leader && right_required[skill] && !has_right(ch, right_required[skill]))
  {
    send_to_char("You don't have that right!\n\r", ch);
    return;
  }

  switch (skill)
  {
  case 0: /* list */
  {
    do_clan_list(ch);
    break;
  }
  case 1: /* email */
  {
    if (clan_email(ch, arg))
      save_clans();
    break;
  }
  case 2: /* description */
  {
    if (clan_desc(ch, arg))
      save_clans();
    break;
  }
  case 3: /* login */
  {
    if (clan_login_message(ch, arg))
      save_clans();
    break;
  }
  case 4: /* logout */
  {
    if (clan_logout_message(ch, arg))
      save_clans();
    break;
  }
  case 5: /* death */
  {
    if (clan_death_message(ch, arg))
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
    if (clan_motd(ch, arg))
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
                 " log     - Show clan log\r\n"
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
                 " info    - Ability to modify clan email/description/motd\r\n",
                 ch);
    break;
  }
  case 10: // log
  {
    show_clan_log(ch);
    break;
  }
  default:
  {
    send_to_char("Default hit in clans switch statement.\r\n", ch);
    return;
    break;
  }
  }
}

void clan_data::log(QString log_entry)
{
  QString clan_filename = QString("../lib/clans/clan%1.log").arg(number);
  QFile file(clan_filename);

  if (!file.open(QIODeviceBase::Append | QIODeviceBase::Text))
  {
    qCritical() << "Unable to open" << clan_filename;
    return;
  }

  QTextStream out(&file);
  out << log_entry;
  file.close();
}

void show_clan_log(Character *ch)
{
  string s;
  ifstream fin;
  stringstream fname;
  stack<string> logstack;

  fname << "../lib/clans/clan" << ch->clan << ".log";

  fin.open(fname.str().c_str());
  while (getline(fin, s))
  {
    // Remove \r at the end of the line, if applicable
    if (s.size() && *s.rbegin() == '\r')
    {
      s.resize(s.size() - 1);
    }
    logstack.push(s);
  }
  fin.close();

  stringstream buffer;
  buffer << "The following are your clan's most recent 5 pages of log entries:\n\r";
  int line = 1;
  while (logstack.size())
  {
    buffer << logstack.top() << "\n\r";
    logstack.pop();

    // 5 pages, 21 lines each
    if (line++ > 21 * 5)
    {
      break;
    }
  }

  page_string(ch->desc, const_cast<char *>(buffer.str().c_str()), 1);
}

int needs_clan_command(Character *ch)
{
  if (has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
    return 1;
  if (has_right(ch, CLAN_RIGHTS_RIGHTS))
    return 1;

  return 0;
}

int do_clans(Character *ch, char *arg, int cmd)
{
  clan_data *clan = 0;
  char *tmparg;

  char buf[MAX_STRING_LENGTH];
  tmparg = one_argument(arg, buf);

  if (!strcmp(buf, "rights"))
  {
    tmparg = one_argument(tmparg, buf);

    if (!*buf) // only do this if they want clan rights on themselves
    {
      int bit = -1;
      struct clan_member_data *pmember = nullptr;

      if (!(pmember = get_member(ch->name, ch->clan)))
      {
        send_to_char("You don't seem to be in a clan.\r\n", ch);
        return eSUCCESS;
      }

      sprintf(buf, "Rights for %s:\n\r-------------\n\r", pmember->member_name);
      send_to_char(buf, ch);
      for (bit = 0; *clan_rights[bit] != '\n'; bit++)
      {
        sprintf(buf, "  %-15s %s\n\r", clan_rights[bit], (IS_SET(pmember->member_rights, 1 << bit) ? "on" : "off"));
        send_to_char(buf, ch);
      }
      return eSUCCESS;
    }
  }

  if (IS_PC(ch) && (GET_LEVEL(ch) >= COORDINATOR))
  {
    do_god_clans(ch, arg, cmd);
    return eSUCCESS;
  }

  if (ch->clan && (clan = get_clan(ch)) &&
      (!strcmp(GET_NAME(ch), clan->leader) || needs_clan_command(ch)))
  {
    do_leader_clans(ch, arg, cmd);
    return eSUCCESS;
  }

  do_clan_list(ch);

  return eSUCCESS;
}

int do_cinfo(Character *ch, char *arg, int cmd)
{
  clan_data *clan;
  int nClan;
  char buf[MAX_STRING_LENGTH];

  if (!*arg)
  {
    send_to_char("$3Syntax$R:  cinfo <clannumber>\r\n", ch);
    return eSUCCESS;
  }

  nClan = atoi(arg);

  if (!(clan = get_clan(nClan)))
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

  if (GET_LEVEL(ch) >= POWER || (!strcmp(clan->leader, GET_NAME(ch)) && nClan == ch->clan) ||
      (nClan == ch->clan && has_right(ch, CLAN_RIGHTS_MESSAGES)))
  {
    sprintf(buf, "$3Login$R:          %s\r\n"
                 "$3Logout$R:         %s\r\n"
                 "$3Death$R:          %s\r\n",
            clan->login_message ? clan->login_message : "(No Message)",
            clan->logout_message ? clan->logout_message : "(No Message)",
            clan->death_message ? clan->death_message : "(No Message)");
    send_to_char(buf, ch);
  }
  if (GET_LEVEL(ch) >= POWER || (!strcmp(clan->leader, GET_NAME(ch)) && nClan == ch->clan) ||
      (nClan == ch->clan && has_right(ch, CLAN_RIGHTS_MEMBER_LIST)))
  {
    sprintf(buf, "$3Balance$R:         %llu coins\r\n", clan->getBalance());
    send_to_char(buf, ch);
  }
  return eSUCCESS;
}

int do_whoclan(Character *ch, char *arg, int cmd)
{
  clan_data *clan;
  class Connection *desc;
  Character *pch;
  char buf[100];
  int found;

  send_to_char("                  O N L I N E   C L A N   "
               "M E M B E R S\n\r\n\r",
               ch);

  char buf2[MAX_INPUT_LENGTH];
  one_argument(arg, buf2);
  int clan_num = 0;

  if (buf2[0])
    clan_num = atoi(buf2);

  for (clan = clan_list; clan; clan = clan->next)
  {
    found = 0;
    if (clan_num && clan->number != clan_num)
      continue;
    for (desc = DC::getInstance()->descriptor_list; desc; desc = desc->next)
    {
      if (desc->connected || !(pch = desc->character))
        continue;
      if (pch->clan != clan->number || GET_LEVEL(pch) >= OVERSEER ||
          (!CAN_SEE(ch, pch) && ch->clan != pch->clan))
        continue;
      if (found == 0)
      {
        sprintf(buf, "$3Clan %s$R:\n\r", clan->name);
        send_to_char(buf, ch);
      }
      if (clan->number == ch->clan && has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
        sprintf(buf, "  %s %s %s\n\r", GET_SHORT(pch), (!strcmp(GET_NAME(pch), clan->leader) ? "$3($RLeader$3)$R" : ""), IS_SET(GET_TOGGLES(pch), PLR_NOTAX) ? "(NT)" : "(T)");
      else
        sprintf(buf, "  %s %s\n\r", GET_SHORT(pch), (!strcmp(GET_NAME(pch), clan->leader) ? "$3($RLeader$3)$R" : ""));
      send_to_char(buf, ch);
      found++;
    }
  }
  return eSUCCESS;
}

int do_cmotd(Character *ch, char *arg, int cmd)
{
  clan_data *clan;

  if (!ch->clan || !(clan = get_clan(ch)))
  {
    send_to_char("You aren't the member of any clan!\n\r", ch);
    return eFAILURE;
  }

  if (!clan->clanmotd)
  {
    send_to_char("There is no motd for your clan currently.\r\n", ch);
    return eSUCCESS;
  }

  send_to_char(clan->clanmotd, ch);
  return eSUCCESS;
}

int do_ctax(Character *ch, char *arg, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  if (!ch->clan)
  {
    send_to_char("You not a member of a clan.\r\n", ch);
    return eFAILURE;
  }
  arg = one_argument(arg, arg1);
  if (!is_number(arg1))
  {
    csendf(ch, "Your clan's current tax rate is %d.\r\n", get_clan(ch)->tax);
    return eFAILURE;
  }
  if (!has_right(ch, CLAN_RIGHTS_TAX))
  {
    send_to_char("You don't have the right to modify taxes.\r\n", ch);
    return eFAILURE;
  }

  int tax = atoi(arg1);
  if (tax < 0 || tax > 99)
  {
    send_to_char("You can have a maximum of 99% in taxes.\r\n", ch);
    return eFAILURE;
  }
  get_clan(ch)->tax = tax;
  send_to_char("Your clan's tax rate has been modified.\r\n", ch);
  save_clans();
  return eSUCCESS;
}

// This command deposits gold into a clan bank account
command_return_t Character::do_cdeposit(QStringList arguments, int cmd)
{
  QString arg1;

  if (clan == 0 || get_clan(clan) == nullptr)
  {
    send("You are not a member of a clan.\r\n");
    return eFAILURE;
  }

  if (affected_by_spell(this, FUCK_GTHIEF))
  {
    send("Launder your money elsewhere, thief!\r\n");
    return eFAILURE;
  }

  if (world[in_room].number != DC::SORPIGAL_BANK_ROOM)
  {
    send("This can only be done at the Sorpigal bank.\r\n");
    return eFAILURE;
  }

  if (arguments.isEmpty())
  {
    send("Usage: cdeposit <number>\r\n");
    return eFAILURE;
  }

  arg1 = arguments.at(0);
  bool ok = false;
  gold_t dep = arg1.toULongLong(&ok);
  if (ok == false)
  {
    send("How much do you want to deposit?\r\n");
    return eFAILURE;
  }

  if (getGold() < dep)
  {
    send(QString("You don't have %L1 $B$5gold$R coins to deposit into your clan account.\r\n").arg(dep));
    send(QString("You only have %L1 $B$5gold$R coins on you.\r\n").arg(getGold()));
    return eFAILURE;
  }

  removeGold(dep);
  save(666);
  get_clan(clan)->cdeposit(dep);
  save_clans();

  QString coin = "coin";
  if (dep > 1)
  {
    coin = "coins";
  }

  send(QString("You deposit %L1 $B$5gold$R %2 into your clan's account.\r\n").arg(dep).arg(coin));
  QString log_entry = QString("%1 deposited %2 gold %3 in the clan bank account.\r\n").arg(name).arg(dep).arg(coin);
  clan_data *clan = get_clan(this->clan);
  if (clan != nullptr)
  {
    clan->log(log_entry);
  }

  return eSUCCESS;
}

int do_cwithdraw(Character *ch, char *arg, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  if (!ch->clan)
  {
    send_to_char("You not a member of a clan.\r\n", ch);
    return eFAILURE;
  }
  if (!has_right(ch, CLAN_RIGHTS_WITHDRAW) && GET_LEVEL(ch) < 108)
  {
    send_to_char("You don't have the right to withdraw $B$5gold$R from your clan's account.\r\n", ch);
    return eFAILURE;
  }
  if (world[ch->in_room].number != DC::SORPIGAL_BANK_ROOM)
  {
    send_to_char("This can only be done at the Sorpigal bank.\r\n", ch);
    return eFAILURE;
  }

  arg = one_argument(arg, arg1);
  if (!is_number(arg1))
  {
    send_to_char("How much do you want to withdraw?\r\n", ch);
    return eFAILURE;
  }
  uint64_t wdraw = atoi(arg1);
  if (get_clan(ch)->getBalance() < wdraw || wdraw < 0)
  {
    send_to_char("Your clan lacks the funds.\r\n", ch);
    return eFAILURE;
  }
  ch->addGold(wdraw);
  get_clan(ch)->cwithdraw(wdraw);
  if (wdraw == 1)
  {
    csendf(ch, "You withdraw 1 $B$5gold$R coin.\r\n", wdraw);
  }
  else
  {
    csendf(ch, "You withdraw %d $B$5gold$R coins.\r\n", wdraw);
  }
  save_clans();
  ch->save(0);

  char buf[MAX_INPUT_LENGTH];
  if (wdraw == 1)
  {
    snprintf(buf, MAX_INPUT_LENGTH, "%s withdrew 1 $B$5gold$R coin from the clan bank account.\r\n", ch->name);
  }
  else
  {
    snprintf(buf, MAX_INPUT_LENGTH, "%s withdrew %llu $B$5gold$R coins from the clan bank account.\r\n", ch->name, wdraw);
  }
  clan_data *clan = get_clan(ch);
  if (clan != nullptr)
  {
    clan->log(buf);
  }

  return eSUCCESS;
}

int do_cbalance(Character *ch, char *arg, int cmd)
{
  if (!ch->clan)
  {
    send_to_char("You not a member of a clan.\r\n", ch);
    return eFAILURE;
  }
  if (world[ch->in_room].number != DC::SORPIGAL_BANK_ROOM)
  {
    send_to_char("This can only be done at the Sorpigal bank.\r\n", ch);
    return eFAILURE;
  }

  if (!has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
  {
    send_to_char("You don't have the right to see your clan's account.\r\n", ch);
    return eFAILURE;
  }
  stringstream ss;
  ss.imbue(locale("en_US"));
  ss << get_clan(ch)->getBalance();
  csendf(ch, "Your clan has %s $B$5gold$R coins in the bank.\r\n", ss.str().c_str());
  return eSUCCESS;
}

void remove_totem(Object *altar, Object *totem)
{
  auto &character_list = DC::getInstance()->character_list;

  for_each(character_list.begin(), character_list.end(),
           [&altar, totem](Character *const &t)
           {
             if (!IS_NPC(t) && t->altar == altar)
             {
               int j;
               for (j = 0; j < totem->num_affects; j++)
                 affect_modify(t, totem->affected[j].location,
                               totem->affected[j].modifier, -1, false);
               redo_hitpoints(t);
               redo_mana(t);
               redo_ki(t);
             }
           });
}

void add_totem(Object *altar, Object *totem)
{
  auto &character_list = DC::getInstance()->character_list;

  for_each(character_list.begin(), character_list.end(),
           [&altar, totem](Character *const &t)
           {
             if (!IS_NPC(t) && t->altar == altar)
             {
               int j;
               for (j = 0; j < totem->num_affects; j++)
                 affect_modify(t, totem->affected[j].location,
                               totem->affected[j].modifier, -1, true);
             }
           });
}

void remove_totem_stats(Character *ch, int stat)
{
  Object *a;
  if (!ch->altar)
    return;
  for (a = ch->altar->contains; a; a = a->next_content)
  {
    int j;
    if (a->obj_flags.type_flag != ITEM_TOTEM)
      continue;
    for (j = 0; j < a->num_affects; j++)
      if (stat && stat == a->affected[j].location)
        affect_modify(ch, a->affected[j].location,
                      a->affected[j].modifier, -1, false);
      else if (!stat)
        affect_modify(ch, a->affected[j].location,
                      a->affected[j].modifier, -1, false);
  }
  if (!stat)
  {
    redo_hitpoints(ch);
    redo_mana(ch);
    redo_ki(ch);
  }
}

void add_totem_stats(Character *ch, int stat)
{
  Object *a;
  if (!ch->altar)
    return;
  for (a = ch->altar->contains; a; a = a->next_content)
  {
    int j;
    if (a->obj_flags.type_flag != ITEM_TOTEM)
      continue;
    for (j = 0; j < a->num_affects; j++)
      if (stat && stat == a->affected[j].location)
        affect_modify(ch, a->affected[j].location,
                      a->affected[j].modifier, -1, true);
      else if (!stat)
        affect_modify(ch, a->affected[j].location,
                      a->affected[j].modifier, -1, true);
  }
  if (!stat)
  {
    redo_hitpoints(ch);
    redo_mana(ch);
    redo_ki(ch);
  }
}

/*

 Clanarea functions follow.


*/

struct takeover_pulse_data *pulse_list = nullptr;

int count_plrs(int zone, int clan)
{
  auto &character_list = DC::getInstance()->character_list;

  int i = count_if(character_list.begin(), character_list.end(), [&zone, &clan](Character *const &tmpch)
                   {
      if (!IS_NPC(tmpch) && world[tmpch->in_room].zone == zone && clan == tmpch->clan &&
	  GET_LEVEL(tmpch) < 100 && GET_LEVEL(tmpch) > 10)
      return true;
      else
      return false; });

  return i;
}

struct takeover_pulse_data
{
  struct takeover_pulse_data *next;
  int clan1; // defending clan
  int clan1points;
  int clan2; // challenging clan
  int clan2points;
  int zone;
  int pulse;
};

bool can_collect(int zone)
{
  struct takeover_pulse_data *take;
  for (take = pulse_list; take; take = take->next)
    if (zone == take->zone && take->clan2 != -2)
      return false;
  return true;
}

bool can_challenge(int clan, int zone)
{
  struct takeover_pulse_data *take;
  for (take = pulse_list; take; take = take->next)
    if (take->clan2 == -2 &&
        take->clan1 == clan && zone == take->zone)
      return false;
    else if (zone == take->zone && take->clan2 >= 0 && take->clan1 >= 0)
      return false;
  return true;
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

  if (challenge)
  {
    if (!defend)
    {
      //      DC::getInstance()->zones.value(zone).gold = 0;
      if (clan)
        sprintf(buf, "\r\n##Clan %s has broken clan %s's control of%s!\r\n",
                get_clan(clan)->name, get_clan(clan2)->name, DC::getInstance()->zones.value(zone).name);
      else
        sprintf(buf, "\r\n##Clan %s's control of%s has been broken!\r\n",
                get_clan(clan2)->name, DC::getInstance()->zones.value(zone).name);

      takeover_pause(clan2, zone);
    }
    else
    {
      takeover_pause(clan2, zone);
      if (clan2)
        sprintf(buf, "\r\n##Clan %s has defended against clan %s's challenge for control of%s!\r\n",
                get_clan(clan)->name, get_clan(clan2)->name, DC::getInstance()->zones.value(zone).name);
      else
        sprintf(buf, "\r\n##Clan %s has defended their control of%s!\r\n",
                get_clan(clan)->name, DC::getInstance()->zones.value(zone).name);
    }
  }
  else
  {
    if (clan)
      sprintf(buf, "\r\n##%s has been claimed by clan %s!\r\n",
              DC::getInstance()->zones.value(zone).name, get_clan(clan)->name);

    //     DC::getInstance()->zones.value(zone).gold = 0;
  }
  DC::setZoneClanOwner(zone, clan);

  send_info(buf);
}

int count_controlled_areas(int clan)
{
  quint64 zones = 0;
  for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
  {
    if (zone.clanowner == clan && can_collect(zone_key))
    {
      zones++;
    }
  }

  for (struct takeover_pulse_data *plc = pulse_list; plc; plc = plc->next)
  {
    if ((plc->clan1 == clan || plc->clan2 == clan) && plc->clan2 != -2)
    {
      zones++;
    }
  }

  return zones;
}

void recycle_pulse_data(struct takeover_pulse_data *pl)
{
  struct takeover_pulse_data *plc, *plp = nullptr;
  for (plc = pulse_list; plc; plc = plc->next)
  {
    if (plc == pl)
    {
      if (plp)
        plp->next = plc->next;
      else
        pulse_list = plc->next;
      dc_free(pl);
      return; // No point going on..
    }
    plp = plc;
  }
}

int online_clan_members(int clan)
{
  auto &character_list = DC::getInstance()->character_list;

  int i = count_if(character_list.begin(), character_list.end(),
                   [&clan](Character *const &Tmpch)
                   {
                     if (!IS_NPC(Tmpch) && Tmpch->clan == clan && GET_LEVEL(Tmpch) < 100 && Tmpch->desc && GET_LEVEL(Tmpch) > 10)
                       return true;
                     else
                       return false;
                   });

  return i;
}

void check_victory(struct takeover_pulse_data *take)
{
  if (take->clan2 == -2)
    return;
  if (take->clan1points >= 20)
  {
    claimArea(take->clan1, true, true, take->clan2, take->zone);
    recycle_pulse_data(take);
  }
  else if (take->clan2points >= 20)
  {
    claimArea(take->clan2, false, true, take->clan1, take->zone);
    recycle_pulse_data(take);
  }
}

void check_quitter(void *arg1, void *arg2, void *arg3)
{
  int clan = (int64_t)arg1;
  char buf[MAX_STRING_LENGTH];
  if (count_controlled_areas(clan) > online_clan_members(clan))
  { // One needs to go.
    int i = number(1, count_controlled_areas(clan));
    int a, z = 0;
    for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
    {
      if (zone.clanowner == clan && can_collect(zone_key))
        if (++z == i)
        {
          //			DC::getInstance()->zones.value(a].gold = 0;
          zone.clanowner = 0;
          sprintf(buf, "\r\n##Clan %s has lost control of%s!\r\n",
                  get_clan(clan)->name, zone.name);
          send_info(buf);
          return;
        }
    }

    struct takeover_pulse_data *pl;
    for (pl = pulse_list; pl; pl = pl->next)
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

void check_quitter(Character *ch)
{
  if (!ch->clan || GET_LEVEL(ch) >= 100)
    return;

  struct timer_data *timer;
#ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
#else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
#endif
  timer->arg1 = (void *)ch->clan;
  timer->function = check_quitter;
  timer->timeleft = 30;
  addtimer(timer);
}

void pk_check(Character *ch, Character *victim)
{
  if (!ch || !victim)
    return;
  // if (!ch->clan || !victim->clan) return; // No point;
  struct takeover_pulse_data *plc, *pln;
  for (plc = pulse_list; plc; plc = pln)
  {
    pln = plc->next;
    if (plc->clan1 == ch->clan && plc->clan2 == victim->clan && world[ch->in_room].zone == plc->zone)
      plc->clan1points += 2;
    else if (plc->clan1 == victim->clan && plc->clan2 == ch->clan && world[ch->in_room].zone == plc->zone)
      plc->clan2points += 2;
    check_victory(plc);
  }
}

bool can_lose(struct takeover_pulse_data *take)
{
  auto &character_list = DC::getInstance()->character_list;

  auto result = find_if(character_list.begin(), character_list.end(), [&take](Character *const &ch)
                        {
		if (IS_PC(ch) && world[ch->in_room].zone == take->zone
				&& (take->clan1 == ch->clan || take->clan2 == ch->clan)) {
			return true;
		} else {
			return false;
		} });

  if (result != end(character_list))
  {
    return false;
  }
  else
  {
    return true;
  }
}

void pulse_takeover()
{
  struct takeover_pulse_data *take, *next;
  for (take = pulse_list; take; take = next)
  {
    next = take->next;
    take->pulse++;
    if (take->clan2 == -2)
    { // stopthing
      if (take->pulse >= 36 * 4)
        recycle_pulse_data(take);
      continue;
    }
    if (take->pulse < 2)
      continue; // first two pulses nothing happens
    if (take->pulse > 60 && take->clan2 != -2 && can_lose(take))
    {
      char buf[MAX_STRING_LENGTH];
      std::sprintf(buf, "\r\n##Control of%s has been lost!\r\n",
                   DC::getInstance()->zones.value(take->zone).name);
      send_info(buf);
      DC::setZoneClanOwner(take->zone, 0);
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

command_return_t Character::do_clanarea(QStringList arguments, int cmd)
{
  bool clanless_challenge = false;

  if (arguments.isEmpty())
  {
    return eFAILURE;
  }

  QString arg = arguments.at(0);

  if (!clan)
  {
    if (arg == "challenge")
    {
      clanless_challenge = true;
    }
    else
    {
      send("You're not in a clan!\r\n");
      return eFAILURE;
    }
  }

  if (!has_right(this, CLAN_RIGHTS_AREA) && !clanless_challenge)
  {
    send_to_char("You have not been granted that right.\r\n", this);
    return eFAILURE;
  }

  if (arg == "withdraw")
  {
    if (can_collect(world[in_room].zone))
    {
      send_to_char("There is no challenge to withdraw from.\r\n", this);
      return eFAILURE;
    }

    if (!affected_by_spell(this, SKILL_CLANAREA_CHALLENGE))
    {
      send_to_char("You did not issue the challenge, or you have waited too long to withdraw.\r\n", this);
      return eFAILURE;
    }

    struct takeover_pulse_data *take;
    for (take = pulse_list; take; take = take->next)
      if (take->zone == world[in_room].zone &&
          take->clan2 == clan)
      {
        take->clan1points += 20;
        check_victory(take);
        send_to_char("You withdraw your challenge.\r\n", this);
        return eSUCCESS;
      }
    send_to_char("Your did not issue this challenge.\r\n", this);
    return eFAILURE;
  }
  else if (arg == "claim")
  {
    if (affected_by_spell(this, SKILL_CLANAREA_CLAIM))
    {
      send_to_char("You need to wait before you can attempt to claim an area.\r\n", this);
      return eFAILURE;
    }

    if (DC::getInstance()->zones.value(world[in_room].zone).clanowner == 0 && !can_challenge(clan, world[in_room].zone))
    {
      send_to_char("You cannot claim this area right now.\r\n", this);
      return eFAILURE;
    }

    if (DC::getInstance()->zones.value(world[in_room].zone).clanowner > 0)
    {
      csendf(this, "This area is claimed by %s, you need to challenge to obtain ownership.\r\n",
             get_clan(DC::getInstance()->zones.value(world[in_room].zone).clanowner)->name);

      return eFAILURE;
    }
    if (DC::getInstance()->zones.value(world[in_room].zone).isNoClaim())
    {
      send_to_char("This area cannot be claimed.\r\n", this);
      return eFAILURE;
    }
    if (count_controlled_areas(clan) >= online_clan_members(clan))
    {
      send_to_char("You cannot claim any more areas.\r\n", this);
      return eFAILURE;
    }

    struct affected_type af;
    af.type = SKILL_CLANAREA_CLAIM;
    af.duration = 30;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(this, &af, PULSE_TIMER);

    auto zone_key = world[in_room].zone;
    DC::setZoneClanOwner(zone_key, clan);

    send("You claim the area on behalf of your clan.\r\n");
    send(QString("\r\n##%1 has been claimed by %2!\r\n").arg(DC::getZoneName(zone_key)).arg(get_clan(clan)->name));

    return eSUCCESS;
  }
  else if (arg == "yield")
  {
    if (DC::getInstance()->zones.value(world[in_room].zone).clanowner == 0)
    {
      send_to_char("This zone is not under anyone's control.\r\n", this);
      return eFAILURE;
    }
    if (DC::getInstance()->zones.value(world[in_room].zone).clanowner != clan)
    {
      send_to_char("This zone is not under your clan's control.\r\n", this);
      return eFAILURE;
    }

    struct takeover_pulse_data *take;
    for (take = pulse_list; take; take = take->next)
      if (take->zone == world[in_room].zone &&
          take->clan1 == clan && take->clan2 != -2)
      {
        take->clan2points += 20;
        check_victory(take);
        return eSUCCESS;
      }
    send_to_char("You yield the area on behalf of your clan.\r\n", this);
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "\r\n##Clan %s has yielded control of%s!\r\n",
            get_clan(clan)->name, DC::getInstance()->zones.value(world[in_room].zone).name);
    send_info(buf);
    DC::setZoneClanOwner(world[in_room].zone, 0);

    return eSUCCESS;
  }
  else if (arg == "collect")
  {
    if (DC::getInstance()->zones.value(world[in_room].zone).clanowner == 0)
    {
      send_to_char("This area is not under anyone's control.\r\n", this);
      return eFAILURE;
    }

    if (DC::getInstance()->zones.value(world[in_room].zone).clanowner != clan)
    {
      send_to_char("This area is not under your clan's control.\r\n", this);
      return eFAILURE;
    }

    if (DC::getInstance()->zones.value(world[in_room].zone).gold == 0)
    {
      send_to_char("There is no $B$5gold$R to collect.\r\n", this);
      return eFAILURE;
    }
    if (!can_collect(world[in_room].zone))
    {
      send_to_char("There is currently an active challenge for this area, and collecting is not possible.\r\n", this);
      return eFAILURE;
    }
    get_clan(this)->cdeposit(DC::getInstance()->zones.value(world[in_room].zone).gold);
    csendf(this, "You collect %d $B$5gold$R for your clan's treasury.\r\n",
           DC::getInstance()->zones.value(world[in_room].zone).gold);

    DC::setZoneClanGold(world[in_room].zone, 0);
    save_clans();
    return eSUCCESS;
  }
  else if (arg == "list")
  {
    int z = 0;
    for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
      if (DC::getInstance()->zones.value(i).clanowner == clan)
      {
        if (++z == 1)
          csendf(this, "$BAreas Claimed by %s:$R\r\n",
                 get_clan(this)->name);
        csendf(this, "%d)%s\r\n",
               z, DC::getInstance()->zones.value(i).name);
      }

    if (z == 0)
    {
      send_to_char("Your clan has not claimed any areas.\r\n", this);
      return eFAILURE;
    }
    return eSUCCESS;
  }
  else if (arg == "challenge")
  {
    if (affected_by_spell(this, SKILL_CLANAREA_CHALLENGE))
    {
      send_to_char("You need to wait before you can attempt to challenge an area.\r\n", this);
      return eFAILURE;
    }
    if (GET_LEVEL(this) < 40)
    {
      send_to_char("You must be level 40 to issue a challenge.\r\n", this);
      return eFAILURE;
    }

    // most annoying one for last.
    if (DC::getInstance()->zones.value(world[in_room].zone).clanowner == 0)
    {
      send_to_char("This area is not under anyone's control, you could simply claim it.\r\n", this);
      return eFAILURE;
    }
    if (!can_challenge(clan, world[in_room].zone))
    {
      send_to_char("You cannot issue a challenge for this area at the moment.\r\n", this);
      return eFAILURE;
    }
    if (DC::getInstance()->zones.value(world[in_room].zone).clanowner == clan && !clanless_challenge)
    {
      send_to_char("Your clan already controls this area!\r\n", this);
      return eFAILURE;
    }

    if (count_controlled_areas(clan) >= online_clan_members(clan) && !clanless_challenge)
    {
      send_to_char("You cannot own any more areas.\r\n", this);
      return eFAILURE;
    }

    struct affected_type af;
    af.type = SKILL_CLANAREA_CHALLENGE;
    af.duration = 60;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(this, &af, PULSE_TIMER);

    // no point checking for noclaim flag, at this point it already IS under someone's control
    struct takeover_pulse_data *pl;
#ifdef LEAK_CHECK
    pl = (struct takeover_pulse_data *)calloc(1, sizeof(struct takeover_pulse_data));
#else
    pl = (struct takeover_pulse_data *)dc_alloc(1, sizeof(struct takeover_pulse_data));
#endif
    pl->next = pulse_list;
    pl->clan1 = DC::getInstance()->zones.value(world[in_room].zone).clanowner;
    pl->clan2 = clan;
    pl->clan1points = pl->clan2points = 0;
    pl->pulse = 0;
    pl->zone = world[in_room].zone;
    pulse_list = pl;
    char buf[MAX_STRING_LENGTH];
    if (!clanless_challenge)
      sprintf(buf, "\r\n##Clan %s has challenged clan %s for control of%s!\r\n",
              get_clan(this)->name, get_clan(pl->clan1)->name,
              DC::getInstance()->zones.value(world[in_room].zone).name);
    else
      sprintf(buf, "\r\n##Clan %s's control of%s is being challenged!\r\n",
              get_clan(pl->clan1)->name,
              DC::getInstance()->zones.value(world[in_room].zone).name);
    send_info(buf);
    return eSUCCESS;
  }
  send_to_char("Clan Area Commands:\r\n"
               "--------------------\r\n"
               "clanarea list         (lists areas currently claimed by your clan)\r\n"
               "clanarea claim        (claim the area you are currently in for your clan)\r\n"
               "clanarea challenge    (challenge for control of the area you are currently in)\r\n"
               "clanarea withdraw     (withdraw a recently issued challenge)\r\n"
               "clanarea yield        (yield an area your clan controls that you are currently in)\r\n"
               "clanarea collect      (collect bounty from an area you are currently in that your clan controls)\r\n",
               this);
  return eSUCCESS;
}

bool others_clan_room(Character *ch, Room *room)
{
  // Passed null values
  if (ch == 0 || room == 0)
  {
    return false;
  }

  // room is not a clan room
  if (IS_SET(room->room_flags, CLAN_ROOM) == false)
  {
    return false;
  }

  // ch is not in a clan
  clan_data *clan;
  if ((clan = get_clan(ch)) == 0)
  {
    return true;
  }

  // Search through our clan's list of rooms, to see if room is one of them
  for (clan_room_data *c_room = clan->rooms; c_room; c_room = c_room->next)
  {
    if (room->number == c_room->room_number)
    {
      return false;
    }
  }

  // Room was a clan room, we are in a clan, but this room is not ours
  return true;
}

clan_data::clan_data(void)
{
  balance = 0;
  leader = nullptr;
  founder = nullptr;
  name = nullptr;
  email = nullptr;
  description = nullptr;
  login_message = nullptr;
  death_message = nullptr;
  logout_message = nullptr;
  clanmotd = nullptr;
  rooms = nullptr;
  members = nullptr;
  next = nullptr;
  acc = nullptr;
  amt = 0;
  number = 0;
  tax = 0;
}

void clan_data::cdeposit(const uint64_t &deposit)
{
  balance += deposit;
  return;
}

uint64_t clan_data::getBalance(void)
{
  return balance;
}

void clan_data::cwithdraw(const uint64_t &withdraw)
{
  balance -= withdraw;
}

void clan_data::setBalance(const uint64_t &value)
{
  balance = value;
}
