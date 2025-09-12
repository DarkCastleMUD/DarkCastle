/* $Id: clan.cpp,v 1.92 2014/07/26 16:19:37 jhhudso Exp $ */

/***********************************************************************/
/* Revision History                                                    */
/* 11/10/2003    Onager     Removed clan size limit                    */
/***********************************************************************/
#define __STDC_LIMIT_MACROS
#include <cstdint>
uint64_t i = UINT64_MAX;

#include <cstring> // strcat
#include <cstdio>  // FILE *
#include <cctype>  // isspace..
#include <netinet/in.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <algorithm>
#include <locale>

#include <fmt/format.h>
#include <QFile>

#include "DC/obj.h"
#include "DC/fileinfo.h"
#include "DC/db.h" // real_room
#include "DC/player.h"
#include "DC/utility.h"
#include "DC/character.h"
#include "DC/connect.h"  // Connection
#include "DC/mobile.h"   // utility.h stuff
#include "DC/clan.h"     // duh
#include "DC/interp.h"   // do_outcast, etc..
#include "DC/handler.h"  // get_char_room_vis
#include "DC/terminal.h" // get_char_room_vis
#include "DC/room.h"     // CLAN_ROOM flag
#include "DC/returnvals.h"
#include "DC/spells.h"
#include "DC/DC.h"
#include "DC/Trace.h"
#include "DC/clan.h"

void addtimer(struct timer_data *timer);
void delete_clan(const clan_data *currclan);

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
  ClanMember *new_new_member = nullptr;
  int tempint;
  bool skip_clan = false, changes_made = false;

  if (!(fl = fopen("../lib/clan.txt", "r")))
  {
    qCritical("Unable to open clan file...\n");
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
      logf(0, DC::LogChannel::LOG_BUG, "Invalid clan number %d found in ../lib/clan.txt.", new_new_clan->number);
      skip_clan = true;
    }

    if (get_clan(new_new_clan->number) != nullptr)
    {
      logf(0, DC::LogChannel::LOG_BUG, "Duplicate clan number %d found in ../lib/clan.txt.", new_new_clan->number);
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
          qCritical(qUtf8Printable(QStringLiteral("negative clan balance read for clan %1.\n").arg(new_new_clan->number)));
          qCritical(qUtf8Printable(QStringLiteral("Setting clan %1's balance to 0.\n").arg(new_new_clan->number)));
          new_new_clan->setBalance(0);
        }
        catch (...)
        {
          qCritical(qUtf8Printable(QStringLiteral("unknown error reading clan balance for clan %1.\n").arg(new_new_clan->number)));
          qCritical(qUtf8Printable(QStringLiteral("Setting clan %1's balance to 0.\n").arg(new_new_clan->number)));
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
        new_new_member = new ClanMember;
        new_new_member->Name(fread_string(fl, 0));
        new_new_member->Rights(fread_int(fl, 0, 2147483467));
        new_new_member->Rank(fread_int(fl, 0, 2147483467));
        new_new_member->Unused1(fread_int(fl, 0, 2147483467));
        new_new_member->Unused2(fread_int(fl, 0, 2147483467));
        new_new_member->Unused3(fread_int(fl, 0, 2147483467));
        new_new_member->TimeJoined(fread_int(fl, 0, 2147483467));
        new_new_member->Unused4(fread_string(fl, 0));

        // add it to the member linked list
        add_clan_member(new_new_clan, new_new_member);
        break;
      }
      default:
        logentry(QStringLiteral("Illegal switch hit in boot_clans."), 0, DC::LogChannel::LOG_MISC);
        logentry(buf, 0, DC::LogChannel::LOG_MISC);
        break;
      }
    }
    if (skip_clan)
    {
      skip_clan = false;
      logf(0, DC::LogChannel::LOG_BUG, "Deleting clan number %d.", new_new_clan->number);
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
    logf(0, DC::LogChannel::LOG_BUG, "Changes made to clans. Saving ../lib/clan.txt.");
    save_clans();
  }
}

void save_clans(void)
{
  FILE *fl;
  clan_data *pclan = nullptr;
  struct clan_room_data *proom = nullptr;
  ClanMember *pmember = nullptr;
  char buf[MAX_STRING_LENGTH];
  char *x;
  char *targ;

  if (!(fl = fopen("../lib/clan.txt", "w")))
  {
    qFatal("Unable to open clan.txt for writing.\n");
  }

  for (pclan = DC::getInstance()->clan_list; pclan; pclan = pclan->next)
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
      fprintf(fl, "B\n%lu\n", pclan->getBalance());

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
      fprintf(fl, "M\n%s~\n", pmember->NameC());
      fprintf(fl, "%d %d %d %d %d %d\n", pmember->Rights(),
              pmember->Rank(), pmember->Unused1(), pmember->Unused2(),
              pmember->Unused3(), pmember->TimeJoined());
      fprintf(fl, "%s~\n", pmember->Unused4C());
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

  std::stringstream ssbuffer;
  ssbuffer << HTDOCS_DIR << port1 << "/" << WEBCLANSLIST_FILE;
  if (!(fl = fopen(ssbuffer.str().c_str(), "w")))
  {
    logf(0, DC::LogChannel::LOG_MISC, "Unable to open web clan file \'%s\' for writing.\n", ssbuffer.str().c_str());
    return;
  }

  for (pclan = DC::getInstance()->clan_list; pclan; pclan = pclan->next)
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
  ClanMember *curr_member = nullptr;
  ClanMember *next_member = nullptr;

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

void DC::free_clans_from_memory(void)
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

  for (clan = DC::getInstance()->clan_list; clan; clan = clan->next)
    for (room = clan->rooms; room; room = room->next)
      if (-1 != real_room(room->room_number))
        if (!isSet(DC::getInstance()->world[real_room(room->room_number)].room_flags, CLAN_ROOM))
          SET_BIT(DC::getInstance()->world[real_room(room->room_number)].room_flags, CLAN_ROOM);
}

ClanMember *get_member(QString strName, int nClanId)
{
  clan_data *theClan = nullptr;

  if (!(theClan = get_clan(nClanId)) || strName.isEmpty())
    return nullptr;

  ClanMember *pcurr = theClan->members;

  while (pcurr && strName != pcurr->Name())
    pcurr = pcurr->next;

  return pcurr;
}

bool is_in_clan(clan_data *theClan, Character *ch)
{
  ClanMember *pcurr = theClan->members;

  while (pcurr)
  {
    if (pcurr->Name() == ch->getName())
    {
      return true;
    }
    pcurr = pcurr->next;
  }

  return false;
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
  ClanMember *pcurr = nullptr;
  ClanMember *plast = nullptr;

  pcurr = theClan->members;

  while (pcurr && pcurr->Name() != ch->getName())
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
  ClanMember *pmember = nullptr;

  if (!ch || !theClan)
  {
    logentry(QStringLiteral("add_clan_member(clan, ch) called with a null."), ANGEL, DC::LogChannel::LOG_BUG);
    return;
  }

  pmember = new ClanMember(ch);
  add_clan_member(theClan, pmember);
}

// This should really be done as a binary tree, but I'm lazy, and this doesn't get used
// very often, so it's just a linked list sorted by member name
void add_clan_member(clan_data *theClan, ClanMember *new_new_member)
{
  ClanMember *pcurr = nullptr;
  ClanMember *plast = nullptr;
  int result = 0;

  if (!new_new_member || !theClan)
  {
    logentry(QStringLiteral("add_clan_member(clan, member) called with a null."), ANGEL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (new_new_member->Name().isEmpty())
  {
    logentry(QStringLiteral("Attempt to add a blank member name to a clan."), ANGEL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (!theClan->members)
  {
    theClan->members = new_new_member;
    new_new_member->next = nullptr;
    return;
  }

  pcurr = theClan->members;

  bool member_found = false;
  while (pcurr)
  {
    if (pcurr->Name() == new_new_member->Name())
    {
      member_found = true;
      break;
    }
    plast = pcurr;
    pcurr = pcurr->next;
  }

  if (member_found)
  { // found um, get out
    logentry(QStringLiteral("Tried to add already existing clan member '%1'.").arg(new_new_member->Name()), ANGEL, DC::LogChannel::LOG_BUG);
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

  if (!DC::getInstance()->clan_list)
  {
    DC::getInstance()->clan_list = new_new_clan;
    DC::getInstance()->end_clan_list = new_new_clan;
    return;
  }

  plast = DC::getInstance()->clan_list;
  pcurr = DC::getInstance()->clan_list->next;

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

  DC::getInstance()->end_clan_list->next = new_new_clan;
  DC::getInstance()->end_clan_list = new_new_clan;
}

void free_member(ClanMember *member)
{
  dc_free(member);
}

void delete_clan(clan_data *dead_clan)
{
  clan_data *last = 0;
  clan_data *curr = 0;
  struct clan_room_data *room = 0;
  struct clan_room_data *nextroom = 0;

  if (!DC::getInstance()->clan_list)
    return;
  if (!dead_clan)
    return;

  if (DC::getInstance()->clan_list == dead_clan)
  {
    if (dead_clan == DC::getInstance()->end_clan_list) // Only 1 clan total
      DC::getInstance()->end_clan_list = 0;
    DC::getInstance()->clan_list = dead_clan->next;
    delete dead_clan;
    return;
  }

  // This works since the first clan is not the dead_clan
  curr = DC::getInstance()->clan_list;
  while (curr)
    if (curr == dead_clan)
    {
      last->next = curr->next;

      if ((curr = DC::getInstance()->end_clan_list))
        DC::getInstance()->end_clan_list = last;

      if (dead_clan->rooms)
      {
        room = dead_clan->rooms;
        nextroom = room->next;
        if (real_room(room->room_number) != DC::NOWHERE)
          if (isSet(DC::getInstance()->world[real_room(room->room_number)].room_flags, CLAN_ROOM))
            REMOVE_BIT(DC::getInstance()->world[real_room(room->room_number)].room_flags, CLAN_ROOM);

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
  ClanMember *pmember = nullptr;

  if (!ch || !(pmember = get_member(GET_NAME(ch), ch->clan)))
    return false;

  return pmember->Rights();
}

// see if ch has rights to 'bit' in his clan
int has_right(Character *ch, uint32_t bit)
{
  ClanMember *pmember = nullptr;

  if (!ch || !(pmember = get_member(GET_NAME(ch), ch->clan)))
    return false;

  return isSet(pmember->Rights(), bit);
}

int num_clan_members(clan_data *clan)
{
  int i = 0;
  for (ClanMember *pmem = clan->members;
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

  for (clan = DC::getInstance()->clan_list; clan; clan = clan->next)
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

  for (clan = DC::getInstance()->clan_list; clan; clan = clan->next)
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
    pch->send("-->> ");
    ansi_color(RED, pch);
    ansi_color(BOLD, pch);
    pch->send(buf);
    ansi_color(NTEXT, pch);
    ansi_color(YELLOW, pch);
    pch->sendln(" <<--");
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
    ch->sendln("You have an illegal clan number.  Contact a god.");
    return;
  }

  if (!clan->death_message)
    return;

  // Don't give away any imms listening in
  if (ch->isImmortalPlayer())
    return;

  if (!(curr = strstr(clan->death_message, "%")))
  {
    ch->sendln("Error:  clan with illegal death_message.  Contact a god.");
    return;
  }

  *curr = '\0';
  sprintf(buf, "%s%s%s", clan->death_message, GET_SHORT(ch), curr + 1);
  *curr = '%';

  if (!(curr = strstr(buf, "#")))
  {
    ch->sendln("Error:  clan with illegal death_message.  Contact a god.");
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
  if (ch->isImmortalPlayer())
  {
    return;
  }

  if (!is_in_clan(clan, ch))
  {
    ch->sendln("You were kicked out of your clan.");
    ch->clan = 0;
    return;
  }

  if (!clan->login_message)
    return;

  if (!(curr = strstr(clan->login_message, "%")))
  {
    ch->sendln("Error:  clan with illegal login_message.  Contact a god.");
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
    ch->sendln("You have an illegal clan number.  Contact a god.");
    return;
  }

  // Don't give away any imms listening in
  if (ch->isImmortalPlayer())
  {
    return;
  }

  if (!clan->logout_message)
    return;

  if (!(curr = strstr(clan->logout_message, "%")))
  {
    ch->sendln("Error:  clan with illegal logout_message.  Contact a god.");
    return;
  }

  *curr = '\0';
  sprintf(buf, "%s%s%s", clan->logout_message, GET_NAME(ch), curr + 1);
  *curr = '%';

  message_to_clan(ch, buf);
}

int do_accept(Character *ch, char *arg, cmd_t cmd)
{
  Character *victim;
  clan_data *clan;
  char buf[MAX_STRING_LENGTH];

  while (isspace(*arg))
    arg++;

  if (!*arg)
  {
    ch->sendln("Accept who into your clan?");
    return eFAILURE;
  }

  if (!ch->clan || !(clan = get_clan(ch)))
  {
    ch->sendln("You aren't the member of any clan!");
    return eFAILURE;
  }

  if (strcmp(clan->leader, GET_NAME(ch)) && !has_right(ch, CLAN_RIGHTS_ACCEPT))
  {
    ch->sendln("You aren't the leader of your clan!");
    return eFAILURE;
  }

  one_argument(arg, buf);

  if (!(victim = ch->get_char_room_vis(buf)))
  {
    ch->sendln("You can't accept someone into your clan who isn't here!");
    return eFAILURE;
  }

  if (IS_NPC(victim) || victim->getLevel() >= IMMORTAL)
  {
    ch->sendln("Yeah right.");
    return eFAILURE;
  }

  if (victim->clan)
  {
    ch->sendln("This person already belongs to a clan.");
    return eFAILURE;
  }

  victim->clan = ch->clan;
  add_clan_member(clan, victim);
  save_clans();
  sprintf(buf, "You are now a member of %s.\r\n", clan->name);
  ch->sendln("Your clan now has a new member.");
  victim->send(buf);

  sprintf(buf, "%s just joined clan [%s].", victim->getNameC(), clan->name);
  logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_CLAN);

  add_totem_stats(victim);

  return eSUCCESS;
}

command_return_t Character::do_outcast(QStringList arguments, cmd_t cmd)
{
  clan_data *clanPtr = get_clan(this);
  if (!clanPtr)
  {
    sendln("You are not a member of any clan!");
    return eFAILURE;
  }

  if (arguments.isEmpty())
  {
    sendln("Cast who out of your clan?");
    return eFAILURE;
  }

  QString arg1 = arguments.value(0).trimmed().toLower();
  // if (!arg1.isEmpty())
  // {
  //   arg1 = arg1.toUpper();
  // }

  Character *victim = get_char(arg1);
  bool victim_connected = true;
  if (!victim)
  {
    bool victim_connected = false;
    Connection d = {};
    if (!(load_char_obj(&d, arg1)))
    {
      if (file_exists(QStringLiteral("../archive/%1.gz").arg(arg1)))
      {
        sendln("Character is archived.");
      }
      else
      {
        sendln("Unable to outcast, type the entire name.");
      }
      return eFAILURE;
    }

    victim = d.character;
    victim->desc = 0;

    victim->hometown = START_ROOM;
    victim->in_room = START_ROOM;
    victim_connected = false;
  }

  if (!victim->clan)
  {
    this->sendln("This person isn't in a clan in the first place...");
    return eFAILURE;
  }

  if (strcmp(clanPtr->leader, getNameC()) && victim != this && !has_right(this, CLAN_RIGHTS_OUTCAST))
  {
    this->sendln("You don't have the right to outcast people from your clan!");
    return eFAILURE;
  }

  if (!strcmp(clanPtr->leader, victim->getNameC()))
  {
    this->sendln("You can't outcast the clan leader!");
    return eFAILURE;
  }

  if (victim == this)
  {
    logentry(QStringLiteral("%1 just quit clan [%2].").arg(victim->getName()).arg(clanPtr->name), IMPLEMENTER, DC::LogChannel::LOG_CLAN);
    this->sendln("You quit your clan.");
    remove_totem_stats(victim);
    victim->clan = 0;
    remove_clan_member(clan, this);
    save_clans();
    return eSUCCESS;
  }

  if (victim->clan != this->clan)
  {
    this->sendln("That person isn't in your clan!");
    return eFAILURE;
  }

  remove_totem_stats(victim);
  victim->clan = 0;
  remove_clan_member(clan, victim);
  save_clans();
  sendln(QStringLiteral("You cast %1 out of your clan.").arg(victim->getName()));
  victim->sendln(QStringLiteral("You are cast out of %1.").arg(clanPtr->name));

  logentry(QStringLiteral("%1 was outcasted from clan [%2].").arg(victim->getName()).arg(clanPtr->name), IMPLEMENTER, DC::LogChannel::LOG_CLAN);

  victim->save(cmd_t::SAVE_SILENTLY);
  if (!victim_connected)
    free_char(victim, Trace("do_outcast"));

  return eSUCCESS;
}

int do_cpromote(Character *ch, char *arg, cmd_t cmd)
{
  Character *victim;
  clan_data *clan;
  char buf[MAX_STRING_LENGTH];

  while (isspace(*arg))
    arg++;

  if (!*arg)
  {
    ch->sendln("Who do you want to make the new clan leader?");
    return eFAILURE;
  }

  if (!ch->clan || !(clan = get_clan(ch)))
  {
    ch->sendln("You aren't the member of any clan!");
    return eFAILURE;
  }

  if (!isexact(clan->leader, GET_NAME(ch)))
  {
    ch->sendln("You aren't the leader of your clan!");
    return eFAILURE;
  }

  one_argument(arg, buf);

  if (!(victim = ch->get_char_room_vis(buf)))
  {
    ch->sendln("You can't cpromote someone who isn't here!");
    return eFAILURE;
  }

  if (IS_NPC(victim))
  {
    ch->sendln("Yeah right.");
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
  clan->leader = str_dup(victim->getNameC());

  save_clans();

  sprintf(buf, "You are now the leader of %s.\r\n", clan->name);
  ch->sendln("Your clan now has a new leader.");
  victim->send(buf);

  sprintf(buf, "%s just cpromoted by %s as leader of clan [%s].", victim->getNameC(), GET_NAME(ch), clan->name);
  logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_CLAN);
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
    ch->sendln("Clan description removed.");
    return 1;
  }

  if (strcmp(text, "change"))
  {
    sprintf(buf, "$3Syntax$R:  clans description change\r\n\r\nCurrent description: %s\r\n",
            clan->description ? clan->description : "(No Description)");
    ch->send(buf);
    ch->sendln("To not have any description use:  clans description delete");
    return 0;
  }

  /*  if(clan->description)
      dc_free(clan->description);
    clan->description = nullptr;
  */
  // ch->sendln("Write new description.  ~ to end.");

  //  ch->desc->connected = Connection::states::EDITING;
  //  ch->desc->str = &clan->description;
  //  ch->desc->max_str = MAX_CLAN_DESC_LENGTH;
  ch->desc->backstr = nullptr;
  send_to_char("        Write your description and stay within the line.  (/s saves /h for help)\r\n"
               "   |--------------------------------------------------------------------------------|\r\n",
               ch);
  if (clan->description)
  {
    ch->desc->backstr = str_dup(clan->description);
    ch->send(ch->desc->backstr);
  }

  ch->desc->connected = Connection::states::EDITING;
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
    ch->sendln("Clan motd removed.");
    return 1;
  }

  if (strcmp(text, "change"))
  {
    sprintf(buf, "$3Syntax$R:  clans motd change\r\n\r\nCurrent motd: %s\r\n",
            clan->clanmotd ? clan->clanmotd : "(No Motd)");
    ch->send(buf);
    ch->sendln("To not have any motd use:  clans motd delete");
    return 0;
  }

  /*  if(clan->clanmotd)
      dc_free(clan->clanmotd);
    clan->clanmotd = nullptr;
  */
  // ch->sendln("Write new motd.  ~ to end.");

  // ch->desc->connected = Connection::states::EDITING;
  // ch->desc->str = &clan->clanmotd;
  // ch->desc->max_str = MAX_CLAN_DESC_LENGTH;

  ch->desc->backstr = nullptr;
  send_to_char("        Write your motd and stay within the line.  (/s saves /h for help)\r\n"
               "   |--------------------------------------------------------------------------------|\r\n",
               ch);
  if (clan->clanmotd)
  {
    ch->desc->backstr = str_dup(clan->clanmotd);
    ch->send(ch->desc->backstr);
  }

  ch->desc->connected = Connection::states::EDITING;
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
    ch->send(buf);
    ch->sendln("To not have any message use:  clans death delete");
    return 0;
  }

  one_argument(arg, buf);
  if (!strncmp(buf, "delete", 6))
  {
    ch->sendln("Clan death message removed.");
    if (clan->death_message)
      dc_free(clan->death_message);
    clan->death_message = nullptr;
    return 1;
  }

  if (strstr(arg, "~"))
  {
    ch->sendln("No ~s fatt butt!");
    return 0;
  }

  char *curr;

  if (!(curr = strstr(arg, "%")))
  {
    ch->sendln("You must include a '%' to represent the victim's name.");
    return 0;
  }

  if (strstr(curr + 1, "%"))
  {
    ch->sendln("You may only have one '%' in the message.");
    return 0;
  }

  if (!(curr = strstr(arg, "#")))
  {
    ch->sendln("You must include a '#' to represent the killer's name.");
    return 0;
  }

  if (strstr(curr + 1, "#"))
  {
    ch->sendln("You may only have one '#' in the message.");
    return 0;
  }

  if (clan->death_message)
    dc_free(clan->death_message);

  clan->death_message = str_dup(arg);

  sprintf(buf, "Clan death message changed to: %s\r\n", clan->death_message);
  ch->send(buf);
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
    ch->send(buf);
    ch->sendln("To not have any message use:  clans logout delete");
    return 0;
  }

  one_argument(arg, buf);
  if (!strncmp(buf, "delete", 6))
  {
    ch->sendln("Clan logout message removed.");
    if (clan->logout_message)
      dc_free(clan->logout_message);
    clan->logout_message = nullptr;
    return 1;
  }

  if (strstr(arg, "~"))
  {
    ch->sendln("No ~s fatt butt!");
    return 0;
  }

  char *curr;

  if (!(curr = strstr(arg, "%")))
  {
    ch->sendln("You must include a '%' to represent the person's name.");
    return 0;
  }

  if (strstr(curr + 1, "%"))
  {
    ch->sendln("You may only have one '%' in the message.");
    return 0;
  }

  if (clan->logout_message)
    dc_free(clan->logout_message);

  clan->logout_message = str_dup(arg);

  sprintf(buf, "Clan logout message changed to: %s\r\n", clan->logout_message);
  ch->send(buf);
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
    ch->send(buf);
    ch->sendln("To not have any message use:  clans login delete");
    return 0;
  }

  one_argument(arg, buf);
  if (!strncmp(buf, "delete", 6))
  {
    ch->sendln("Clan login message removed.");
    if (clan->login_message)
      dc_free(clan->login_message);
    clan->login_message = nullptr;
    return 1;
  }

  if (strstr(arg, "~"))
  {
    ch->sendln("No ~s fatt butt!");
    return 0;
  }

  char *curr;

  if (!(curr = strstr(arg, "%")))
  {
    ch->sendln("You must include a '%' to represent the person's name.");
    return 0;
  }

  if (strstr(curr + 1, "%"))
  {
    ch->sendln("You may only have one '%' in the message.");
    return 0;
  }

  if (clan->login_message)
    dc_free(clan->login_message);

  clan->login_message = str_dup(arg);

  sprintf(buf, "Clan login message changed to: %s\r\n", clan->login_message);
  ch->send(buf);
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
    ch->send(buf);
    ch->sendln("To not have any email use:  clans email delete");
    return 0;
  }

  if (!strncmp(text, "delete", 6))
  {
    if (clan->email)
      dc_free(clan->email);
    clan->email = nullptr;
    ch->sendln("Clan email address removed.");
    return 1;
  }

  if (strstr(text, "~") || strstr(text, "<") || strstr(text, ">") ||
      strstr(text, "&") || strstr(text, "$"))
  {
    ch->sendln("We both know those characters aren't legal in email addresses....");
    return 0;
  }

  if (clan->email)
    dc_free(clan->email);

  clan->email = str_dup(text);

  sprintf(buf, "Clan email changed to: %s\r\n", clan->email);
  ch->send(buf);
  return 1;
}

int do_ctell(Character *ch, char *arg, cmd_t cmd)
{
  Character *pch;
  class Connection *desc;
  char buf[MAX_STRING_LENGTH];

  if (!ch->clan)
  {
    ch->sendln("But you don't belong to a clan!");
    return eFAILURE;
  }

  Object *tmp_obj;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return eFAILURE;
    }

  while (isspace(*arg))
    arg++;
  if (!has_right(ch, CLAN_RIGHTS_CHANNEL) && ch->getLevel() < 51)
  {
    ch->sendln("You don't have the right to talk to your clan.");
    return eFAILURE;
  }

  if (!(isSet(ch->misc, DC::LogChannel::CHANNEL_CLAN)))
  {
    ch->sendln("You have that channel off!!");
    return eFAILURE;
  }

  if (!*arg)
  {
    std::queue<std::string> tmp = get_clan(ch)->ctell_history;
    if (tmp.empty())
    {
      ch->sendln("No one has said anything lately.");
      return eFAILURE;
    }

    ch->sendln("Here are the last 10 ctells:");
    while (!tmp.empty())
    {
      send_to_char((tmp.front()).c_str(), ch);
      tmp.pop();
    }

    return eFAILURE;
  }

  sprintf(buf, "You tell the clan, '%s'\n\r", arg);
  ansi_color(GREEN, ch);
  ch->send(buf);
  ansi_color(NTEXT, ch);

  sprintf(buf, "%s tells the clan, '%s'\n\r", GET_SHORT(ch), arg);
  bool yes;
  for (desc = DC::getInstance()->descriptor_list; desc; desc = desc->next)
  {
    yes = false;
    if (desc->connected || !(pch = desc->character))
      continue;
    if (pch == ch || pch->clan != ch->clan ||
        !isSet(pch->misc, DC::LogChannel::CHANNEL_CLAN))
      continue;
    if (!has_right(pch, CLAN_RIGHTS_CHANNEL) && pch->getLevel() <= DC::MAX_MORTAL_LEVEL)
      continue;

    for (tmp_obj = DC::getInstance()->world[pch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
      if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
      {
        yes = true;
        break;
      }

    if (yes)
      continue;

    ansi_color(GREEN, pch);
    pch->send(buf);
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
  std::string buf, buf2;

  if (ch->getLevel() > 103)
  {
    ch->sendln("$B$7## Clan                 Leader           Tax   $B$5Gold$7 Balance$R");
  }
  else
  {
    ch->sendln("$B$7## Clan                 Leader           $R");
  }

  std::locale("en_US.UTF-8");
  for (clan = DC::getInstance()->clan_list; clan; clan = clan->next)
  {
    if (ch->getLevel() > 103)
    {
      buf = fmt::format("{:2} {:<20}$R {:<16} {:3} {:16L}\r\n", clan->number, clan->name, clan->leader, clan->tax, clan->getBalance());
    }
    else
    {
      buf = fmt::format("{:2} {:<20}$R {:<16}\r\n", clan->number, clan->name, clan->leader);
    }
    ch->send(buf);
  }
}

void do_clan_member_list(Character *ch)
{
  ClanMember *pmember = 0;
  clan_data *pclan = 0;
  int column = 1;
  char buf[200], buf2[200];

  if (!(pclan = get_clan(ch->clan)))
  {
    ch->sendln("Error:  Not in clan.  Contact a god.");
    return;
  }

  ch->sendln("Members of clan:");
  sprintf(buf, "  ");

  for (pmember = pclan->members; pmember; pmember = pmember->next)
  {
    sprintf(buf2, "%-20s  ", pmember->NameC());
    send_to_char(buf2, ch);

    if (0 == (column % 3))
    {
      ch->sendln("");
      column = 0;
    }
    else
    {
      ch->send(" ");
    }
    column++;
  }

  if (column != 0)
  {
    ch->sendln("");
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
  ClanMember *pmember = nullptr;
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
    ch->sendln("$3Syntax$R:  clan rights <member> [right]");
    return;
  }

  *name = toupper(*name);

  if (!(pmember = get_member(name, ch->clan)))
  {
    sprintf(buf, "Could not find '%s' in your clan.\r\n", name);
    ch->send(buf);
    return;
  }

  if (!*last)
  { // diag
    sprintf(buf, "Rights for %s:\n\r-------------\n\r", pmember->NameC());
    ch->send(buf);
    for (bit = 0; *clan_rights[bit] != '\n'; bit++)
    {
      sprintf(buf, "  %-15s %s\n\r", clan_rights[bit], (isSet(pmember->Rights(), 1 << bit) ? "on" : "off"));
      ch->send(buf);
    }
    return;
  }

  bit = old_search_block(last, 0, strlen(last), clan_rights, 1);

  if (bit < 0)
  {
    ch->sendln("Right not found.");
    return;
  }
  bit--;

  if (!is_clan_leader(ch) && !has_right(ch, 1 << bit))
  {
    ch->sendln("You can't give out rights that you don't have.");
    return;
  }

  auto r = pmember->Rights();
  TOGGLE_BIT(r, 1 << bit);
  pmember->Rights(r);

  if (isSet(pmember->Rights(), 1 << bit))
  {
    sprintf(buf, "%s toggled on.\r\n", clan_rights[bit]);
    sprintf(buf2, "%s has given you '%s' rights within your clan.\r\n", GET_SHORT(ch), clan_rights[bit]);
  }
  else
  {
    sprintf(buf, "%s toggled off.\r\n", clan_rights[bit]);
    sprintf(buf2, "%s has taken away '%s' rights within your clan.\r\n", GET_SHORT(ch), clan_rights[bit]);
  }
  ch->send(buf);

  if ((victim = get_char(pmember->Name())))
  {
    send_to_char(buf2, victim);
  }

  save_clans();
}

void do_god_clans(Character *ch, char *arg, cmd_t cmd)
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
        ch->send(buf);
        *buf = '\0';
      }
    }
    if (*buf)
      ch->send(buf);
    ch->sendln("");
    return;
  }

  skill = old_search_block(select, 0, strlen(select), god_values, 1);
  if (skill < 0)
  {
    ch->sendln("That value not recognized.");
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
      ch->sendln("$3Syntax$R: clans create <clanname> <clannumber>");
      return;
    }
    if (strlen(text) > 29)
    {
      ch->sendln("Clan name too long.");
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
      ch->send(QStringLiteral("%1 is an invalid clan number because it already exists.\r\n").arg(x));
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
    ch->sendln("New clan created.");
    break;
  }
  case 1: /* rename */
  {
    arg = one_argumentnolow(arg, text);
    arg = one_argumentnolow(arg, last);
    if (!*text || !*last)
    {
      ch->sendln("$3Syntax$r: clans rename <targetclannum> <newname>");
      return;
    }
    x = atoi(text);

    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
      return;
    }

    if (strlen(last) > 29)
    {
      ch->sendln("Clan name too long.");
      return;
    }

    strcpy(tarclan->name, last);
    ch->sendln("Clan name changed.");
    break;
  }
  case 2: /* leader */
  {
    arg = one_argumentnolow(arg, text);
    arg = one_argumentnolow(arg, last);
    if (!*text || !*last)
    {
      ch->sendln("$3Syntax$R: clans leader <clannumber> <leadername>");
      return;
    }

    if (strlen(last) > 14)
    {
      ch->sendln("Clan leader name too long.");
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
      return;
    }

    if (tarclan->leader)
      dc_free(tarclan->leader);
    tarclan->leader = str_dup(last);

    ch->sendln("Clan leader name changed.");
    break;
  }
  case 3: /* delete */
  {
    arg = one_argumentnolow(arg, text);
    one_argumentnolow(arg, last);
    if (!*text || !*last)
    {
      ch->sendln("$3Syntax$R: clans delete <clannumber> dElEtE");
      return;
    }
    if (!isexact(last, "dElEtE"))
    {
      ch->sendln("You MUST end the line with 'dElEtE' to delete the clan.");
      return;
    }
    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
      return;
    }
    delete_clan(tarclan);
    ch->sendln("Clan deleted.");
    break;
  }
  case 4: /* addroom */
  {
    arg = one_argumentnolow(arg, text);
    arg = one_argumentnolow(arg, last);
    if (!*text || !*last)
    {
      ch->sendln("$3Syntax$R: clans addroom <clannumber> <roomnumber>");
      return;
    }
    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
      return;
    }

    skill = atoi(last);
    if (-1 == real_room(skill))
    {
      ch->sendln("Invalid room number.");
      return;
    }

    SET_BIT(DC::getInstance()->world[real_room(skill)].room_flags, CLAN_ROOM);
#ifdef LEAK_CHECK
    newroom = (struct clan_room_data *)calloc(1, sizeof(clan_room_data));
#else
    newroom = (struct clan_room_data *)dc_alloc(1, sizeof(clan_room_data));
#endif
    newroom->room_number = skill;
    newroom->next = tarclan->rooms;
    tarclan->rooms = newroom;
    ch->sendln("Room added.");
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
    ch->sendln("Saved.");
    break;
  }
  case 7: /* showrooms */
  {
    arg = one_argumentnolow(arg, text);
    if (!*text)
    {
      ch->sendln("$3Syntax$R: clans showrooms <clannumber>");
      return;
    }
    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
      return;
    }

    if (!tarclan->rooms)
    {
      ch->sendln("This clan has no rooms set.");
      return;
    }

    ch->sendln("Rooms\r\n-----");
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
      ch->sendln("$3Syntax$R: clans killroom <clannumber> <roomnumber>");
      return;
    }
    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
      return;
    }

    if (!tarclan->rooms)
    {
      ch->sendln("Error.  Target clan has no rooms.");
      return;
    }

    skill = atoi(last);
    if (tarclan->rooms->room_number == skill)
    {
      if (-1 != real_room(skill))
        if (isSet(DC::getInstance()->world[real_room(skill)].room_flags, CLAN_ROOM))
          REMOVE_BIT(DC::getInstance()->world[real_room(skill)].room_flags, CLAN_ROOM);
      lastroom = tarclan->rooms;
      tarclan->rooms = tarclan->rooms->next;
      dc_free(lastroom);
      ch->sendln("Deleted.");
      return;
    }

    newroom = tarclan->rooms;
    while (newroom)
      if (newroom->room_number == skill)
      {
        if (-1 != real_room(skill))
          if (isSet(DC::getInstance()->world[real_room(skill)].room_flags, CLAN_ROOM))
            REMOVE_BIT(DC::getInstance()->world[real_room(skill)].room_flags, CLAN_ROOM);
        lastroom->next = newroom->next;
        dc_free(newroom);
        ch->sendln("Deleted.");
        return;
      }
      else
      {
        lastroom = newroom;
        newroom = newroom->next;
      }

    ch->sendln("Specified room number not found.");
    break;
  }

  case 9:
  { /* email */

    arg = one_argumentnolow(arg, text);
    one_argumentnolow(arg, last);

    if (!*text || !*last)
    {
      ch->sendln("$3Syntax$R: clans email <clannumber> <address>");
      ch->sendln("To not have any email use:  clans email <clannumber> delete");
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
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
      ch->sendln("$3Syntax$R: clans description <clannumber> change");
      ch->sendln("To not have any description use:  clans description <clannumber> delete");
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
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
      ch->sendln("$3Syntax$R: clans login <clannumber> <login message>");
      ch->sendln("To not have any message use:  clans login <clannumber> delete");
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
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
      ch->sendln("$3Syntax$R: clans logout <clannumber> <logout message>");
      ch->sendln("To not have any message use:  clans logout <clannumber> delete");
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
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
      ch->sendln("$3Syntax$R: clans death <clannumber> <death message>");
      ch->sendln("To not have any message use:  clans death <clannumber> delete");
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
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
      ch->sendln("$3Syntax$R: clans members <clannumber>");
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
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
      ch->sendln("$3Syntax$R: clans rights <clannumber>");
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
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
      ch->sendln("$3Syntax$R: clans motd <clannumber> change");
      ch->sendln("To not have any motd use:  clans motd <clannumber> delete");
      return;
    }

    x = atoi(text);
    tarclan = get_clan(x);

    if (!tarclan)
    {
      ch->sendln("Invalid clan number.");
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
    ch->sendln("Default hit in clans switch statement.");
    return;
    break;
  }
  }
}

void do_leader_clans(Character *ch, char *arg, cmd_t cmd)
{
  ClanMember *pmember = 0;
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
    ch->sendln("Error:  no clan in do_clans_leader");
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
          !isSet(pmember->Rights(), right_required[i]))
        continue;

      sprintf(buf + strlen(buf), "%18s", mortal_values[i]);
      if (!(j % 4))
      {
        strcat(buf, "\r\n");
        ch->send(buf);
        *buf = '\0';
      }
      j++;
    }
    if (*buf)
      ch->send(buf);
    ch->sendln("");
    return;
  }

  skill = old_search_block(select, 0, strlen(select), mortal_values, 1);
  if (skill < 0)
  {
    ch->sendln("That value not recognized.");
    return;
  }
  skill--;

  if (!leader && right_required[skill] && !has_right(ch, right_required[skill]))
  {
    ch->sendln("You don't have that right!");
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
    ch->sendln("Default hit in clans switch statement.");
    return;
    break;
  }
  }
}

void clan_data::log(QString log_entry)
{
  QString clan_filename = QStringLiteral("../lib/clans/clan%1.log").arg(number);
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
  std::string s;
  std::ifstream fin;
  std::stringstream fname;
  std::stack<std::string> logstack;

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

  std::stringstream buffer;
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

int do_clans(Character *ch, char *arg, cmd_t cmd)
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
      ClanMember *pmember = nullptr;

      if (!(pmember = get_member(ch->getNameC(), ch->clan)))
      {
        ch->sendln("You don't seem to be in a clan.");
        return eSUCCESS;
      }

      sprintf(buf, "Rights for %s:\n\r-------------\n\r", pmember->NameC());
      ch->send(buf);
      for (bit = 0; *clan_rights[bit] != '\n'; bit++)
      {
        sprintf(buf, "  %-15s %s\n\r", clan_rights[bit], (isSet(pmember->Rights(), 1 << bit) ? "on" : "off"));
        ch->send(buf);
      }
      return eSUCCESS;
    }
  }

  if (IS_PC(ch) && (ch->getLevel() >= COORDINATOR))
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

int do_cinfo(Character *ch, char *arg, cmd_t cmd)
{
  clan_data *clan;
  int nClan;
  char buf[MAX_STRING_LENGTH];

  if (!*arg)
  {
    ch->sendln("$3Syntax$R:  cinfo <clannumber>");
    return eSUCCESS;
  }

  nClan = atoi(arg);

  if (!(clan = get_clan(nClan)))
  {
    ch->sendln("That is not a valid clan number.");
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
  ch->send(buf);

  // This has to be separate, or if the leader uses $'s, it comes out funky
  sprintf(buf, "%s\r\n",
          clan->description ? clan->description : "(No Description)\r\n");
  ch->send(buf);

  if (ch->getLevel() >= POWER || (!strcmp(clan->leader, GET_NAME(ch)) && nClan == ch->clan) ||
      (nClan == ch->clan && has_right(ch, CLAN_RIGHTS_MESSAGES)))
  {
    sprintf(buf, "$3Login$R:          %s\r\n"
                 "$3Logout$R:         %s\r\n"
                 "$3Death$R:          %s\r\n",
            clan->login_message ? clan->login_message : "(No Message)",
            clan->logout_message ? clan->logout_message : "(No Message)",
            clan->death_message ? clan->death_message : "(No Message)");
    ch->send(buf);
  }
  if (ch->getLevel() >= POWER || (!strcmp(clan->leader, GET_NAME(ch)) && nClan == ch->clan) ||
      (nClan == ch->clan && has_right(ch, CLAN_RIGHTS_MEMBER_LIST)))
  {
    sprintf(buf, "$3Balance$R:         %lu coins\r\n", clan->getBalance());
    ch->send(buf);
  }
  return eSUCCESS;
}

int do_whoclan(Character *ch, char *arg, cmd_t cmd)
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

  for (clan = DC::getInstance()->clan_list; clan; clan = clan->next)
  {
    found = 0;
    if (clan_num && clan->number != clan_num)
      continue;
    for (desc = DC::getInstance()->descriptor_list; desc; desc = desc->next)
    {
      if (desc->connected || !(pch = desc->character))
        continue;
      if (pch->clan != clan->number || pch->getLevel() >= OVERSEER ||
          (!CAN_SEE(ch, pch) && ch->clan != pch->clan))
        continue;
      if (found == 0)
      {
        sprintf(buf, "$3Clan %s$R:\n\r", clan->name);
        ch->send(buf);
      }
      if (clan->number == ch->clan && has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
        sprintf(buf, "  %s %s %s\n\r", GET_SHORT(pch), (!strcmp(GET_NAME(pch), clan->leader) ? "$3($RLeader$3)$R" : ""), isSet(GET_TOGGLES(pch), Player::PLR_NOTAX) ? "(NT)" : "(T)");
      else
        sprintf(buf, "  %s %s\n\r", GET_SHORT(pch), (!strcmp(GET_NAME(pch), clan->leader) ? "$3($RLeader$3)$R" : ""));
      ch->send(buf);
      found++;
    }
  }
  return eSUCCESS;
}

int do_cmotd(Character *ch, char *arg, cmd_t cmd)
{
  clan_data *clan;

  if (!ch->clan || !(clan = get_clan(ch)))
  {
    ch->sendln("You aren't the member of any clan!");
    return eFAILURE;
  }

  if (!clan->clanmotd)
  {
    ch->sendln("There is no motd for your clan currently.");
    return eSUCCESS;
  }

  ch->send(clan->clanmotd);
  return eSUCCESS;
}

int do_ctax(Character *ch, char *arg, cmd_t cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  if (!ch->clan)
  {
    ch->sendln("You not a member of a clan.");
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
    ch->sendln("You don't have the right to modify taxes.");
    return eFAILURE;
  }

  int tax = atoi(arg1);
  if (tax < 0 || tax > 99)
  {
    ch->sendln("You can have a maximum of 99% in taxes.");
    return eFAILURE;
  }
  get_clan(ch)->tax = tax;
  ch->sendln("Your clan's tax rate has been modified.");
  save_clans();
  return eSUCCESS;
}

// This command deposits gold into a clan bank account
command_return_t Character::do_cdeposit(QStringList arguments, cmd_t cmd)
{
  QString arg1;

  if (clan == 0 || get_clan(clan) == nullptr)
  {
    send("You are not a member of a clan.\r\n");
    return eFAILURE;
  }

  if (isPlayerGoldThief())
  {
    send("Launder your money elsewhere, thief!\r\n");
    return eFAILURE;
  }

  if (DC::getInstance()->world[in_room].number != DC::SORPIGAL_BANK_ROOM)
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
    send(QStringLiteral("You don't have %L1 $B$5gold$R coins to deposit into your clan account.\r\n").arg(dep));
    send(QStringLiteral("You only have %L1 $B$5gold$R coins on you.\r\n").arg(getGold()));
    return eFAILURE;
  }

  removeGold(dep);
  save(cmd_t::SAVE_SILENTLY);
  get_clan(clan)->cdeposit(dep);
  save_clans();

  QString coin = "coin";
  if (dep > 1)
  {
    coin = "coins";
  }

  send(QStringLiteral("You deposit %L1 $B$5gold$R %2 into your clan's account.\r\n").arg(dep).arg(coin));
  QString log_entry = QStringLiteral("%1 deposited %2 gold %3 in the clan bank account.\r\n").arg(name_).arg(dep).arg(coin);
  clan_data *clan = get_clan(this->clan);
  if (clan != nullptr)
  {
    clan->log(log_entry);
  }

  return eSUCCESS;
}

int do_cwithdraw(Character *ch, char *arg, cmd_t cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  if (!ch->clan)
  {
    ch->sendln("You not a member of a clan.");
    return eFAILURE;
  }
  if (!has_right(ch, CLAN_RIGHTS_WITHDRAW) && ch->getLevel() < 108)
  {
    ch->sendln("You don't have the right to withdraw $B$5gold$R from your clan's account.");
    return eFAILURE;
  }
  if (DC::getInstance()->world[ch->in_room].number != DC::SORPIGAL_BANK_ROOM)
  {
    ch->sendln("This can only be done at the Sorpigal bank.");
    return eFAILURE;
  }

  arg = one_argument(arg, arg1);
  if (!is_number(arg1))
  {
    ch->sendln("How much do you want to withdraw?");
    return eFAILURE;
  }
  uint64_t wdraw = atoi(arg1);
  if (get_clan(ch)->getBalance() < wdraw || wdraw < 0)
  {
    ch->sendln("Your clan lacks the funds.");
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
    ch->send(QStringLiteral("You withdraw %1 $B$5gold$R coins.\r\n").arg(wdraw));
  }
  save_clans();
  ch->save();

  char buf[MAX_INPUT_LENGTH];
  if (wdraw == 1)
  {
    snprintf(buf, MAX_INPUT_LENGTH, "%s withdrew 1 $B$5gold$R coin from the clan bank account.\r\n", ch->getNameC());
  }
  else
  {
    snprintf(buf, MAX_INPUT_LENGTH, "%s withdrew %lu $B$5gold$R coins from the clan bank account.\r\n", ch->getNameC(), wdraw);
  }
  clan_data *clan = get_clan(ch);
  if (clan != nullptr)
  {
    clan->log(buf);
  }

  return eSUCCESS;
}

int do_cbalance(Character *ch, char *arg, cmd_t cmd)
{
  if (!ch->clan)
  {
    ch->sendln("You not a member of a clan.");
    return eFAILURE;
  }
  if (DC::getInstance()->world[ch->in_room].number != DC::SORPIGAL_BANK_ROOM)
  {
    ch->sendln("This can only be done at the Sorpigal bank.");
    return eFAILURE;
  }

  if (!has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
  {
    ch->sendln("You don't have the right to see your clan's account.");
    return eFAILURE;
  }
  std::stringstream ss;
  ss.imbue(std::locale("en_US"));
  ss << get_clan(ch)->getBalance();
  csendf(ch, "Your clan has %s $B$5gold$R coins in the bank.\r\n", ss.str().c_str());
  return eSUCCESS;
}

void remove_totem(Object *altar, Object *totem)
{
  const auto &character_list = DC::getInstance()->character_list;

  for_each(character_list.begin(), character_list.end(),
           [&altar, totem](Character *const &t)
           {
             if (IS_PC(t) && t->altar == altar)
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
  const auto &character_list = DC::getInstance()->character_list;

  for_each(character_list.begin(), character_list.end(),
           [&altar, totem](Character *const &t)
           {
             if (IS_PC(t) && t->altar == altar)
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
  const auto &character_list = DC::getInstance()->character_list;

  int i = std::count_if(character_list.begin(), character_list.end(), [&zone, &clan](Character *const &tmpch)
                        {
      if (IS_PC(tmpch) && DC::getInstance()->world[tmpch->in_room].zone == zone && clan == tmpch->clan &&
	  tmpch->getLevel() < 100 && tmpch->getLevel() > 10)
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
                get_clan(clan)->name, get_clan(clan2)->name, DC::getInstance()->zones.value(zone).NameC());
      else
        sprintf(buf, "\r\n##Clan %s's control of%s has been broken!\r\n",
                get_clan(clan2)->name, DC::getInstance()->zones.value(zone).NameC());

      takeover_pause(clan2, zone);
    }
    else
    {
      takeover_pause(clan2, zone);
      if (clan2)
        sprintf(buf, "\r\n##Clan %s has defended against clan %s's challenge for control of%s!\r\n",
                get_clan(clan)->name, get_clan(clan2)->name, DC::getInstance()->zones.value(zone).NameC());
      else
        sprintf(buf, "\r\n##Clan %s has defended their control of%s!\r\n",
                get_clan(clan)->name, DC::getInstance()->zones.value(zone).NameC());
    }
  }
  else
  {
    if (clan)
      snprintf(buf, sizeof(buf), "\r\n##%s has been claimed by clan %s!\r\n",
               DC::getInstance()->zones.value(zone).Name().toStdString().c_str(), get_clan(clan)->name);

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
  const auto &character_list = DC::getInstance()->character_list;

  int i = std::count_if(character_list.begin(), character_list.end(),
                        [&clan](Character *const &Tmpch)
                        {
                          if (IS_PC(Tmpch) && Tmpch->clan == clan && Tmpch->getLevel() < 100 && Tmpch->desc && Tmpch->getLevel() > 10)
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

void check_quitter(varg_t arg1, void *arg2, void *arg3)
{
  int clan = arg1.clan;
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
          sprintf(buf, "\r\n##Clan %s has lost control of%s!\r\n", get_clan(clan)->name, zone.NameC());
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
  if (!ch->clan || ch->getLevel() >= 100)
    return;

  struct timer_data *timer = new timer_data;
  timer->arg1.clan = ch->clan;
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
    if (plc->clan1 == ch->clan && plc->clan2 == victim->clan && DC::getInstance()->world[ch->in_room].zone == plc->zone)
      plc->clan1points += 2;
    else if (plc->clan1 == victim->clan && plc->clan2 == ch->clan && DC::getInstance()->world[ch->in_room].zone == plc->zone)
      plc->clan2points += 2;
    check_victory(plc);
  }
}

bool can_lose(struct takeover_pulse_data *take)
{
  const auto &character_list = DC::getInstance()->character_list;

  auto result = find_if(character_list.begin(), character_list.end(), [&take](Character *const &ch)
                        {
		if (IS_PC(ch) && DC::getInstance()->world[ch->in_room].zone == take->zone
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
                   DC::getInstance()->zones.value(take->zone).NameC());
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

command_return_t Character::do_clanarea(QStringList arguments, cmd_t cmd)
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
    this->sendln("You have not been granted that right.");
    return eFAILURE;
  }

  if (arg == "withdraw")
  {
    if (can_collect(DC::getInstance()->world[in_room].zone))
    {
      this->sendln("There is no challenge to withdraw from.");
      return eFAILURE;
    }

    if (!affected_by_spell(SKILL_CLANAREA_CHALLENGE))
    {
      this->sendln("You did not issue the challenge, or you have waited too long to withdraw.");
      return eFAILURE;
    }

    struct takeover_pulse_data *take;
    for (take = pulse_list; take; take = take->next)
      if (take->zone == DC::getInstance()->world[in_room].zone &&
          take->clan2 == clan)
      {
        take->clan1points += 20;
        check_victory(take);
        this->sendln("You withdraw your challenge.");
        return eSUCCESS;
      }
    this->sendln("Your did not issue this challenge.");
    return eFAILURE;
  }
  else if (arg == "claim")
  {
    if (affected_by_spell(SKILL_CLANAREA_CLAIM))
    {
      this->sendln("You need to wait before you can attempt to claim an area.");
      return eFAILURE;
    }

    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner == 0 && !can_challenge(clan, DC::getInstance()->world[in_room].zone))
    {
      this->sendln("You cannot claim this area right now.");
      return eFAILURE;
    }

    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner > 0)
    {
      csendf(this, "This area is claimed by %s, you need to challenge to obtain ownership.\r\n",
             get_clan(DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner)->name);

      return eFAILURE;
    }
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).isNoClaim())
    {
      this->sendln("This area cannot be claimed.");
      return eFAILURE;
    }
    if (count_controlled_areas(clan) >= online_clan_members(clan))
    {
      this->sendln("You cannot claim any more areas.");
      return eFAILURE;
    }

    struct affected_type af;
    af.type = SKILL_CLANAREA_CLAIM;
    af.duration = 30;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(this, &af, DC::PULSE_TIMER);

    auto zone_key = DC::getInstance()->world[in_room].zone;
    DC::setZoneClanOwner(zone_key, clan);

    send("You claim the area on behalf of your clan.\r\n");
    send(QStringLiteral("\r\n##%1 has been claimed by %2!\r\n").arg(DC::getZoneName(zone_key)).arg(get_clan(clan)->name));

    return eSUCCESS;
  }
  else if (arg == "yield")
  {
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner == 0)
    {
      this->sendln("This zone is not under anyone's control.");
      return eFAILURE;
    }
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner != clan)
    {
      this->sendln("This zone is not under your clan's control.");
      return eFAILURE;
    }

    struct takeover_pulse_data *take;
    for (take = pulse_list; take; take = take->next)
      if (take->zone == DC::getInstance()->world[in_room].zone &&
          take->clan1 == clan && take->clan2 != -2)
      {
        take->clan2points += 20;
        check_victory(take);
        return eSUCCESS;
      }
    this->sendln("You yield the area on behalf of your clan.");
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "\r\n##Clan %s has yielded control of%s!\r\n", get_clan(clan)->name, DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).NameC());
    send_info(buf);
    DC::setZoneClanOwner(DC::getInstance()->world[in_room].zone, 0);

    return eSUCCESS;
  }
  else if (arg == "collect")
  {
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner == 0)
    {
      this->sendln("This area is not under anyone's control.");
      return eFAILURE;
    }

    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner != clan)
    {
      this->sendln("This area is not under your clan's control.");
      return eFAILURE;
    }

    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).gold == 0)
    {
      this->sendln("There is no $B$5gold$R to collect.");
      return eFAILURE;
    }
    if (!can_collect(DC::getInstance()->world[in_room].zone))
    {
      this->sendln("There is currently an active challenge for this area, and collecting is not possible.");
      return eFAILURE;
    }
    get_clan(this)->cdeposit(DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).gold);
    csendf(this, "You collect %d $B$5gold$R for your clan's treasury.\r\n",
           DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).gold);

    DC::setZoneClanGold(DC::getInstance()->world[in_room].zone, 0);
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
        csendf(this, "%d)%s\r\n", z, DC::getInstance()->zones.value(i).NameC());
      }

    if (z == 0)
    {
      this->sendln("Your clan has not claimed any areas.");
      return eFAILURE;
    }
    return eSUCCESS;
  }
  else if (arg == "challenge")
  {
    if (affected_by_spell(SKILL_CLANAREA_CHALLENGE))
    {
      this->sendln("You need to wait before you can attempt to challenge an area.");
      return eFAILURE;
    }
    if (level_ < 40)
    {
      this->sendln("You must be level 40 to issue a challenge.");
      return eFAILURE;
    }

    // most annoying one for last.
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner == 0)
    {
      this->sendln("This area is not under anyone's control, you could simply claim it.");
      return eFAILURE;
    }
    if (!can_challenge(clan, DC::getInstance()->world[in_room].zone))
    {
      this->sendln("You cannot issue a challenge for this area at the moment.");
      return eFAILURE;
    }
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner == clan && !clanless_challenge)
    {
      this->sendln("Your clan already controls this area!");
      return eFAILURE;
    }

    if (count_controlled_areas(clan) >= online_clan_members(clan) && !clanless_challenge)
    {
      this->sendln("You cannot own any more areas.");
      return eFAILURE;
    }

    struct affected_type af;
    af.type = SKILL_CLANAREA_CHALLENGE;
    af.duration = 60;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(this, &af, DC::PULSE_TIMER);

    // no point checking for noclaim flag, at this point it already IS under someone's control
    struct takeover_pulse_data *pl;
#ifdef LEAK_CHECK
    pl = (struct takeover_pulse_data *)calloc(1, sizeof(struct takeover_pulse_data));
#else
    pl = (struct takeover_pulse_data *)dc_alloc(1, sizeof(struct takeover_pulse_data));
#endif
    pl->next = pulse_list;
    pl->clan1 = DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner;
    pl->clan2 = clan;
    pl->clan1points = pl->clan2points = 0;
    pl->pulse = 0;
    pl->zone = DC::getInstance()->world[in_room].zone;
    pulse_list = pl;
    char buf[MAX_STRING_LENGTH];
    if (!clanless_challenge)
      sprintf(buf, "\r\n##Clan %s has challenged clan %s for control of%s!\r\n", get_clan(this)->name, get_clan(pl->clan1)->name, DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).NameC());
    else
      sprintf(buf, "\r\n##Clan %s's control of%s is being challenged!\r\n", get_clan(pl->clan1)->name, DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).NameC());
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
  if (isSet(room->room_flags, CLAN_ROOM) == false)
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

ClanMember::ClanMember(Character *ch)
    : next(nullptr), name_(QString()), unused1_(0), unused2_(0), unused3_(0), unused4_(QString()), rights_(0), rank_(0), time_joined_(0)
{
  if (ch)
  {
    name_ = ch->getName();
  }
}
