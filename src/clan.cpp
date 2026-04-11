/* $Id: clan.cpp,v 1.92 2014/07/26 16:19:37 jhhudso Exp $ */

/***********************************************************************/
/* Revision History                                                    */
/* 11/10/2003    Onager     Removed clan size limit                    */
/***********************************************************************/
#include "DC/comm.h"
#include <qiodevicebase.h>
#define __STDC_LIMIT_MACROS
#include <cstdint>
quint64 i = UINT64_MAX;

#include <cstring> // strcat
#include <cstdio>  // FILE *
#include <cctype>  // isspace..
#include <netinet/in.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <locale>

#include <fmt/format.h>
#include <QFile>

#include "DC/db.h" // real_room
#include "DC/DC.h"

#include "DC/clan.h"     // duh
#include "DC/interp.h"   // do_outcast, etc..
#include "DC/terminal.h" // get_char_room_vis
                         // CLAN_ROOM flag
#include "DC/Trace.h"
#include "DC/clan.h"

void addtimer(timer_data *timer);

constexpr auto MAX_CLAN_DESC_LENGTH = 1022;

const QStringList clan_rights = {
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
  clan_room_data *new_new_room = {};
  ClanMember *new_new_member = {};
  qint32 tempint;
  bool skip_clan = false, changes_made = false;

  QFile file("../lib/clan.txt");
  if (!file.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly))
  {
    qCritical("Unable to open ../lib/clan.txt file for reading...");

    if (!file.open(QIODeviceBase::Text | QIODeviceBase::WriteOnly))
      qFatal("Unable to open ../lib/clan.txt for writing.");

    QTextStream out(&file);
    out << QStringLiteral("~\n");
    file.close();
    if (!file.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly))
      qFatal("Unable to open ../lib/clan.txt file for reading...");
  }
  QTextStream out(&file);

  QChar a;
  while ((a = fread_char(out)) != '~')
  {
    out.seek(-1);

    new_new_clan = new Clan;
    new_new_clan->leader_ = fread_word(fl, 1);
    new_new_clan->founder_ = fread_word(fl, 1);
    new_new_clan->name(fread_word(fl, 1));
    new_new_clan->number = fread_int(fl, 0, 2147483467);
    if (new_new_clan->number < 1 || new_new_clan->number == UINT16_MAX)
    {
      DC::getInstance()->logf(0, DC::LogChannel::LOG_BUG, "Invalid clan number %d found in ../lib/clan.txt.", new_new_clan->number);
      skip_clan = true;
    }

    if (get_clan(new_new_clan->number) != nullptr)
    {
      DC::getInstance()->logf(0, DC::LogChannel::LOG_BUG, "Duplicate clan number %d found in ../lib/clan.txt.", new_new_clan->number);
      skip_clan = true;
    }

    QChar b;
    while (true) /* I see clan rooms! */
    {
      b = fread_char(fl);
      if (b == 'S')
        break;
      if (b != 'R')
        continue;
      auto new_new_room = new clan_room_data;
      new_new_room->next = new_new_clan->rooms;
      tempint = fread_int(fl, 0, 50000);
      new_new_room->room_number = (qint16)tempint;
      new_new_clan->rooms = new_new_room;
    }

    QChar a;
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
        new_new_clan->email_ = fread_string(fl, 0);
        break;
      }
      case 'D':
      {
        new_new_clan->description_ = fread_string(fl, 0);
        break;
      }
      case 'C':
      {
        new_new_clan->clanmotd_ = fread_string(fl, 0);
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
          qCritical("%s", qUtf8Printable(QStringLiteral("negative clan balance read for clan %1.\n").arg(new_new_clan->number)));
          qCritical("%s", qUtf8Printable(QStringLiteral("Setting clan %1's balance to 0.\n").arg(new_new_clan->number)));
          new_new_clan->setBalance(0);
        }
        catch (...)
        {
          qCritical("%s", qUtf8Printable(QStringLiteral("unknown error reading clan balance for clan %1.\n").arg(new_new_clan->number)));
          qCritical("%s", qUtf8Printable(QStringLiteral("Setting clan %1's balance to 0.\n").arg(new_new_clan->number)));
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
        new_new_clan->login_message_ = fread_qstring(fl);
        break;
      }
      case 'X':
      {
        new_new_clan->death_message_ = fread_qstring(fl);
        break;
      }
      case 'O':
      {
        new_new_clan->logout_message_ = fread_qstring(fl);
        break;
      }
      case 'M':
      { // read a member
        new_new_member = new ClanMember;
        new_new_member->name(fread_string(fl, 0));
        new_new_member->rights(fread_int(fl, 0, 2147483467));
        new_new_member->rank(fread_int(fl, 0, 2147483467));
        new_new_member->unused1(fread_int(fl, 0, 2147483467));
        new_new_member->unused2(fread_int(fl, 0, 2147483467));
        new_new_member->unused3(fread_int(fl, 0, 2147483467));
        new_new_member->time_joined(fread_int(fl, 0, 2147483467));
        new_new_member->unused4(fread_string(fl, 0));

        // add it to the member linked list
        add_clan_member(new_new_clan, new_new_member);
        break;
      }
      default:
        DC::getInstance()->logentry(QStringLiteral("Illegal switch hit in boot_clans."), 0, DC::LogChannel::LOG_MISC);
        DC::getInstance()->logentry(buf, 0, DC::LogChannel::LOG_MISC);
        break;
      }
    }
    if (skip_clan)
    {
      skip_clan = false;
      DC::getInstance()->logf(0, DC::LogChannel::LOG_BUG, "Deleting clan number %d.", new_new_clan->number);
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
    DC::getInstance()->logf(0, DC::LogChannel::LOG_BUG, "Changes made to clans. Saving ../lib/clan.txt.");
    save_clans();
  }
}

void save_clans(void)
{
  FILE *fl;
  Clan *pclan = {};
  clan_room_data *proom = {};
  ClanMember *pmember = {};

  if (!(fl = fopen("../lib/clan.txt", "w")))
  {
    qFatal("Unable to open clan.txt for writing.\n");
  }

  for (pclan = DC::getInstance()->clan_list; pclan; pclan = pclan->next)
  {
    qfprintf(fl, "%s %s %s %d\n", qPrintable(pclan->leader_), qPrintable(pclan->founder_), qPrintable(pclan->name()), pclan->number);
    for (proom = pclan->rooms; proom; proom = proom->next)
      qfprintf(fl, "R %d\n", proom->room_number);
    qfprintf(fl, "S\n");

    pclan->email_.remove('\r');
    if (!pclan->email_.isEmpty())
      qfprintf(fl, "E\n%s~\n", qPrintable(pclan->email_));

    pclan->description_.remove('\r');
    if (!pclan->description_.isEmpty())
      qfprintf(fl, "D\n%s~\n", qPrintable(pclan->description_));

    pclan->login_message_.remove('\r');
    if (!pclan->login_message_.isEmpty())
      qfprintf(fl, "L\n%s~\n", qPrintable(pclan->login_message_));
    //  qfprintf(fl, "L\n%s~\n", pclan->login_message);

    if (pclan->tax)
      qfprintf(fl, "T\n%d\n", pclan->tax);

    if (pclan->getBalance())
      qfprintf(fl, "B\n%lu\n", pclan->getBalance());

    pclan->death_message_.remove('\r');
    if (!pclan->death_message_.isEmpty())
      qfprintf(fl, "X\n%s~\n", qPrintable(pclan->death_message_));

    pclan->logout_message_.remove('\r');
    if (!pclan->logout_message_.isEmpty())
      qfprintf(fl, "O\n%s~\n", qPrintable(pclan->logout_message_));

    pclan->clanmotd_.remove('\r');
    if (!pclan->clanmotd_.isEmpty())
      qfprintf(fl, "C\n%s~\n", qPrintable(pclan->clanmotd_));

    for (pmember = pclan->members; pmember; pmember = pmember->next)
    {
      qfprintf(fl, "M\n%s~\n", qPrintable(pmember->name()));
      qfprintf(fl, "%d %d %lld %lld %llu %d\n", pmember->rights(), pmember->rank(), pmember->unused1(), pmember->unused2(), pmember->unused3(), pmember->time_joined());
      qfprintf(fl, "%s~\n", qPrintable(pmember->unused4()));
    }

    // terminate clan
    qfprintf(fl, "~\n");
  }
  qfprintf(fl, "~\n");
  fclose(fl);

  in_port_t port1 = {};
  if (DC::getInstance()->cf.ports.size() > 0)
  {
    port1 = DC::getInstance()->cf.ports[0];
  }

  std::stringstream ssbuffer;
  ssbuffer << HTDOCS_DIR << port1 << "/" << WEBCLANSLIST_FILE;
  if (!(fl = fopen(ssbuffer.str().c_str(), "w")))
  {
    DC::getInstance()->logf(0, DC::LogChannel::LOG_MISC, "Unable to open web clan file \'%s\' for writing.\n", ssbuffer.str().c_str());
    return;
  }

  for (pclan = DC::getInstance()->clan_list; pclan; pclan = pclan->next)
  {
    qfprintf(fl, "%s %s %d\n", qPrintable(pclan->name()), qPrintable(pclan->leader_), pclan->number);
    qfprintf(fl, "$3Contact Email$R:  %s\n"
                 "$3Clan Hall$R:      %s\n"
                 "$3Clan info$R:\n"
                 "$3----------$R\n",
             !pclan->email_.isEmpty() ? qPrintable(pclan->email_) : "(No Email)",
             pclan->rooms ? "Yes" : "No");
    // This has to be separate, or if the leader uses $'s, it comes out funky
    qfprintf(fl, "%s\n", !pclan->description_.isEmpty() ? qPrintable(pclan->description_) : "(No Description)\r\n");
  }
  fclose(fl);
}

void DC::free_clans_from_memory(void)
{
  clan_list.clear();
}

void assign_clan_rooms()
{
  Clan *clan = {};
  clan_room_data *room = {};

  for (clan = DC::getInstance()->clan_list; clan; clan = clan->next)
    for (room = clan->rooms; room; room = room->next)
      if (-1 != real_room(room->room_number))
        if (!isSet(DC::getInstance()->world[real_room(room->room_number)].room_flags, CLAN_ROOM))
          SET_BIT(DC::getInstance()->world[real_room(room->room_number)].room_flags, CLAN_ROOM);
}

ClanMember &get_member(QString name, clan_id_t clan_id)
{
  auto clan = get_clan(clan_id);
  if (clan.members.contains(name))
    return clan.members[name];

  static ClanMember default;
  default = {};
  return default;
}

bool is_in_clan(Clan &clan, CharacterPtr ch)
{
  return clan.members.contains(ch->name());
}

void remove_clan_member(qint32 clannumber, CharacterPtr ch)
{
  Clan *pclan = {};

  if (!(pclan = get_clan(clannumber)))
    return;

  remove_clan_member(pclan, ch);
}

void remove_clan_member(Clan *theClan, CharacterPtr ch)
{
  ClanMember *pcurr = {};
  ClanMember *plast = {};

  pcurr = theClan->members;

  while (pcurr && pcurr->name() != ch->name())
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

// Add someone.  Just makes the class, fills it, then calls the other add_clan_member
void add_clan_member(Clan *theClan, CharacterPtr ch)
{
  ClanMember *pmember = {};

  if (!ch || !theClan)
  {
    DC::getInstance()->logentry(QStringLiteral("add_clan_member(clan, ch) called with a null."), ANGEL, DC::LogChannel::LOG_BUG);
    return;
  }

  pmember = new ClanMember(ch);
  add_clan_member(theClan, pmember);
}

// This should really be done as a binary tree, but I'm lazy, and this doesn't get used
// very often, so it's just a linked list sorted by member name
void add_clan_member(Clan *theClan, ClanMember *new_new_member)
{
  ClanMember *pcurr = {};
  ClanMember *plast = {};
  qint32 result = {};

  if (!new_new_member || !theClan)
  {
    DC::getInstance()->logentry(QStringLiteral("add_clan_member(clan, member) called with a null."), ANGEL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (new_new_member->name().isEmpty())
  {
    DC::getInstance()->logentry(QStringLiteral("Attempt to add a blank member name to a clan."), ANGEL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (!theClan->members)
  {
    theClan->members = new_new_member;
    new_new_member->next = {};
    return;
  }

  pcurr = theClan->members;

  bool member_found = false;
  while (pcurr)
  {
    if (pcurr->name() == new_new_member->name())
    {
      member_found = true;
      break;
    }
    plast = pcurr;
    pcurr = pcurr->next;
  }

  if (member_found)
  { // found um, get out
    DC::getInstance()->logentry(QStringLiteral("Tried to add already existing clan member '%1'.").arg(new_new_member->name()), ANGEL, DC::LogChannel::LOG_BUG);
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
    new_new_member->next = {};
    return;
  }

  // if we hit here, then we found our insertion point
  new_new_member->next = pcurr;
  plast->next = new_new_member;
}

void add_clan(Clan &clan)
{
  if (clan.id)
    DC::getInstance()->clan_list.insert(clan.id, clan);
}

void delete_clan(Clan &clan)
{
  DC::getInstance()->clan_list.erase(clan.id);

  /* may need to use some of this code in the future to clean up clan-owned rooms
  if (dead_clan->rooms)
  {
    room = dead_clan->rooms;
    nextroom = room->next;
    if (real_room(room->room_number) != DC::NOWHERE)
      if (isSet(DC::getInstance()->world[real_room(room->room_number)].room_flags, CLAN_ROOM))
        REMOVE_BIT(DC::getInstance()->world[real_room(room->room_number)].room_flags, CLAN_ROOM);
  }
        */
}

qint32 plr_rights(CharacterPtr ch)
{
  return get_member(ch->name(), ch->clan).rights();
}

// see if ch has rights to 'bit' in his clan
qint32 has_right(CharacterPtr ch, quint32 bit)
{
  ClanMember *pmember = {};

  if (!ch || !(pmember = get_member(qPrintable(ch->name()), ch->clan)))
    return false;

  return isSet(pmember->rights(), bit);
}

qint32 num_clan_members(Clan *clan)
{
  qint32 i = {};
  for (ClanMember *pmem = clan->members;
       pmem;
       pmem = pmem->next)
    i++;

  return i;
}

Clan *get_clan(qint32 nClan)
{
  Clan *clan = {};

  if (nClan == 0)
    return {};

  for (clan = DC::getInstance()->clan_list; clan; clan = clan->next)
    if (nClan == clan->number)
      return clan;

  return 0;
}

Clan *get_clan(CharacterPtr ch)
{
  if (ch == 0)
  {
    return {};
  }

  Clan *clan;

  for (clan = DC::getInstance()->clan_list; clan; clan = clan->next)
    if (ch->clan == clan->number)
      return clan;

  ch->clan = {};
  return {};
}

QString get_clan_name(qint32 nClan)
{

  Clan *clan = get_clan(nClan);

  if (clan)
    return clan->name();

  return "no clan";
}

QString get_clan_name(CharacterPtr ch)
{
  Clan *clan = get_clan(ch);

  if (clan)
    return clan->name();

  return "no clan";
}

QString get_clan_name(Clan *clan)
{
  if (clan)
    return clan->name();

  return "no clan";
}

void message_to_clan(CharacterPtr ch, QString buf)
{
  CharacterPtr pch;

  for (auto &d : DC::getInstance()->connections_)
  {
    if (conn->connected || !(pch = conn->character))
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

void clan_death(CharacterPtr ch, CharacterPtr killer)
{
  if (!ch || ch->clan == 0)
    return;

  QString buf;
  QString secondbuf;
  Clan *clan;
  QString curr = {};

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
  dc_sprintf(buf, "%s%s%s", clan->death_message, qPrintable(ch->shortdesc_or_name()), curr + 1);
  *curr = '%';

  if (!(curr = strstr(buf, "#")))
  {
    ch->sendln("Error:  clan with illegal death_message.  Contact a god.");
    return;
  }

  *curr = '\0';
  dc_sprintf(secondbuf, "%s%s%s", buf, (killer ? qPrintable(killer->shortdesc_or_name()) : "unknown"), curr + 1);

  message_to_clan(ch, secondbuf);
}

void clan_login(CharacterPtr ch)
{
  if (ch->clan == 0)
    return;

  QString buf;
  Clan *clan;
  QString curr = {};

  if (!(clan = get_clan(ch->clan)))
  {
    // illegal clan number.  Set him to 0 and get out
    ch->clan = {};
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
    ch->clan = {};
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
  dc_sprintf(buf, "%s%s%s", clan->login_message, qPrintable(ch->name()), curr + 1);
  *curr = '%';

  message_to_clan(ch, buf);
}

void clan_logout(CharacterPtr ch)
{
  if (ch->clan == 0)
    return;

  QString buf;
  Clan *clan;
  QString curr = {};

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
  dc_sprintf(buf, "%s%s%s", clan->logout_message, qPrintable(ch->name()), curr + 1);
  *curr = '%';

  message_to_clan(ch, buf);
}

command_return_t do_accept(CharacterPtr ch, QString arg, cmd_t cmd)
{
  CharacterPtr victim;
  Clan *clan;
  QString buf;

  while (isspace(*arg))
    arg++;

  if (!*arg)
  {
    ch->sendln("Accept who into your clan?");
    return ReturnValue::eFAILURE;
  }

  if (!ch->clan || !(clan = get_clan(ch)))
  {
    ch->sendln("You aren't the member of any clan!");
    return ReturnValue::eFAILURE;
  }

  if (strcmp(clan->leader, qPrintable(ch->name())) && !has_right(ch, CLAN_RIGHTS_ACCEPT))
  {
    ch->sendln("You aren't the leader of your clan!");
    return ReturnValue::eFAILURE;
  }

  one_argument(arg, buf);

  if (!(victim = ch->get_char_room_vis(buf)))
  {
    ch->sendln("You can't accept someone into your clan who isn't here!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() || victim->getLevel() >= IMMORTAL)
  {
    ch->sendln("Yeah right.");
    return ReturnValue::eFAILURE;
  }

  if (victim->clan)
  {
    ch->sendln("This person already belongs to a clan.");
    return ReturnValue::eFAILURE;
  }

  victim->clan = ch->clan;
  add_clan_member(clan, victim);
  save_clans();
  dc_sprintf(buf, "You are now a member of %s.\r\n", qPrintable(clan->name()));
  ch->sendln("Your clan now has a new member.");
  victim->send(buf);

  dc_sprintf(buf, "%s just joined clan [%s].", qPrintable(victim->name()), qPrintable(clan->name()));
  DC::getInstance()->logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_CLAN);

  add_totem_stats(victim);

  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_outcast(QStringList arguments, cmd_t cmd)
{
  Clan *clanPtr = get_clan(this);
  if (!clanPtr)
  {
    sendln("You are not a member of any clan!");
    return ReturnValue::eFAILURE;
  }

  if (arguments.isEmpty())
  {
    sendln("Cast who out of your clan?");
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.value(0).trimmed().toLower();
  // if (!arg1.isEmpty())
  // {
  //   arg1 = arg1.toUpper();
  // }

  CharacterPtr victim = get_pc(arg1);
  bool victim_connected = true;
  if (!victim)
  {
    bool victim_connected = false;
    auto result = dc_->load_char_obj(arg1);
    if (!result || !result.value())
    {
      if (file_exists(QStringLiteral("../archive/%1.gz").arg(arg1)))
      {
        sendln("Character is archived.");
      }
      else
      {
        sendln("Unable to outcast, type the entire name.");
      }
      return ReturnValue::eFAILURE;
    }

    victim = conn->character;
    victim->desc = {};

    victim->hometown = START_ROOM;
    victim->in_room = START_ROOM;
    victim_connected = false;
  }

  if (!victim->clan)
  {
    this->sendln("This person isn't in a clan in the first place...");
    return ReturnValue::eFAILURE;
  }

  if (strcmp(clanPtr->leader, qPrintable(name())) && victim != this && !has_right(this, CLAN_RIGHTS_OUTCAST))
  {
    this->sendln("You don't have the right to outcast people from your clan!");
    return ReturnValue::eFAILURE;
  }

  if (clanPtr->leader == victim->name())
  {
    this->sendln("You can't outcast the clan leader!");
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    DC::getInstance()->logentry(QStringLiteral("%1 just quit clan [%2].").arg(victim->name()).arg(clanPtr->name()), IMPLEMENTER, DC::LogChannel::LOG_CLAN);
    this->sendln("You quit your clan.");
    remove_totem_stats(victim);
    victim->clan = {};
    remove_clan_member(clan, this);
    save_clans();
    return ReturnValue::eSUCCESS;
  }

  if (victim->clan != this->clan)
  {
    this->sendln("That person isn't in your clan!");
    return ReturnValue::eFAILURE;
  }

  remove_totem_stats(victim);
  victim->clan = {};
  remove_clan_member(clan, victim);
  save_clans();
  sendln(QStringLiteral("You cast %1 out of your clan.").arg(victim->name()));
  victim->sendln(QStringLiteral("You are cast out of %1.").arg(clanPtr->name()));

  DC::getInstance()->logentry(QStringLiteral("%1 was outcasted from clan [%2].").arg(victim->name()).arg(clanPtr->name()), IMPLEMENTER, DC::LogChannel::LOG_CLAN);

  victim->save(cmd_t::SAVE_SILENTLY);
  if (!victim_connected)
    free_char(victim, Trace("do_outcast"));

  return ReturnValue::eSUCCESS;
}

command_return_t do_cpromote(CharacterPtr ch, QString arg, cmd_t cmd)
{
  CharacterPtr victim;
  Clan *clan;
  QString buf;

  while (isspace(*arg))
    arg++;

  if (!*arg)
  {
    ch->sendln("Who do you want to make the new clan leader?");
    return ReturnValue::eFAILURE;
  }

  if (!ch->clan || !(clan = get_clan(ch)))
  {
    ch->sendln("You aren't the member of any clan!");
    return ReturnValue::eFAILURE;
  }

  if (!isexact(clan->leader, qPrintable(ch->name())))
  {
    ch->sendln("You aren't the leader of your clan!");
    return ReturnValue::eFAILURE;
  }

  one_argument(arg, buf);

  if (!(victim = ch->get_char_room_vis(buf)))
  {
    ch->sendln("You can't cpromote someone who isn't here!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer())
  {
    ch->sendln("Yeah right.");
    return ReturnValue::eFAILURE;
  }

  if (victim->clan != ch->clan)
  {
    send_to_char("You can not cpromote someone who doesn't belong to the "
                 "clan.\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }
  clan->leader_ = victim->name();

  save_clans();

  dc_sprintf(buf, "You are now the leader of %s.\r\n", qPrintable(clan->name()));
  ch->sendln("Your clan now has a new leader.");
  victim->send(buf);

  dc_sprintf(buf, "%s just cpromoted by %s as leader of clan [%s].", qPrintable(victim->name()), qPrintable(ch->name()), qPrintable(clan->name()));
  DC::getInstance()->logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_CLAN);
  return ReturnValue::eSUCCESS;
}

qint32 clan_desc(CharacterPtr ch, QString arg)
{
  Clan *clan = {};

  QString buf;
  QString text;

  clan = get_clan(ch);
  arg = one_argumentnolow(arg, text);

  if (!strncmp(text, "delete", 6))
  {
    if (clan->description)
      clan->description = {};
    clan->description = {};
    ch->sendln("Clan description removed.");
    return 1;
  }

  if (strcmp(text, "change"))
  {
    dc_sprintf(buf, "$3Syntax$R:  clans description change\r\n\r\nCurrent description: %s\r\n",
               clan->description ? clan->description : "(No Description)");
    ch->send(buf);
    ch->sendln("To not have any description use:  clans description delete");
    return 0;
  }

  /*  if(clan->description)
      clan->description={};
    clan->description = {};
  */
  // ch->sendln("Write new description.  ~ to end.");

  //  ch->desc->connected = Connection::states::EDITING;
  //  ch->desc->str = &clan->description;
  //  ch->desc->max_str = MAX_CLAN_DESC_LENGTH;
  ch->desc->backstr = {};
  send_to_char("        Write your description and stay within the line.  (/s saves /h for help)\r\n"
               "   |--------------------------------------------------------------------------------|\r\n",
               ch);
  ch->desc->backstr = clan->description_;
  ch->send(ch->desc->backstr);
  ch->desc->connected = Connection::states::EDITING;
  ch->desc->qstrnew = clan->description_;
  ch->desc->max_str = MAX_CLAN_DESC_LENGTH;
  return 1;
}

qint32 clan_motd(CharacterPtr ch, QString arg)
{
  Clan *clan = {};

  QString buf;
  QString text;

  clan = get_clan(ch);
  arg = one_argumentnolow(arg, text);

  if (!strncmp(text, "delete", 6))
  {
    if (clan->clanmotd)
      clan->clanmotd = {};
    clan->clanmotd = {};
    ch->sendln("Clan motd removed.");
    return 1;
  }

  if (strcmp(text, "change"))
  {
    dc_sprintf(buf, "$3Syntax$R:  clans motd change\r\n\r\nCurrent motd: %s\r\n",
               clan->clanmotd ? clan->clanmotd : "(No Motd)");
    ch->send(buf);
    ch->sendln("To not have any motd use:  clans motd delete");
    return 0;
  }

  /*  if(clan->clanmotd)
      clan->clanmotd={};
    clan->clanmotd = {};
  */
  // ch->sendln("Write new motd.  ~ to end.");

  // ch->desc->connected = Connection::states::EDITING;
  // ch->desc->str = &clan->clanmotd;
  // ch->desc->max_str = MAX_CLAN_DESC_LENGTH;

  ch->desc->backstr = {};
  send_to_char("        Write your motd and stay within the line.  (/s saves /h for help)\r\n"
               "   |--------------------------------------------------------------------------------|\r\n",
               ch);
  if (clan->clanmotd)
  {
    ch->desc->backstr = (clan->clanmotd);
    ch->send(ch->desc->backstr);
  }

  ch->desc->connected = Connection::states::EDITING;
  ch->desc->strnew = &(clan->clanmotd);
  ch->desc->max_str = MAX_CLAN_DESC_LENGTH;
  return 1;
}

qint32 clan_death_message(CharacterPtr ch, QString arg)
{
  Clan *clan = {};

  QString buf;

  clan = get_clan(ch);
  if (!*arg)
  {
    dc_sprintf(buf, "$3Syntax$R:  clans death <new message>\r\n\r\nCurrent message: %s\r\n",
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
      clan->death_message = {};
    clan->death_message = {};
    return 1;
  }

  if (strstr(arg, "~"))
  {
    ch->sendln("No ~s fatt butt!");
    return 0;
  }

  QString curr;

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
    clan->death_message = {};

  clan->death_message = (arg);

  dc_sprintf(buf, "Clan death message changed to: %s\r\n", clan->death_message);
  ch->send(buf);
  return 1;
}

qint32 clan_logout_message(CharacterPtr ch, QString arg)
{
  Clan *clan = {};

  QString buf;

  clan = get_clan(ch);
  if (!*arg)
  {
    dc_sprintf(buf, "$3Syntax$R:  clans logout <new message>\r\n\r\nCurrent message: %s\r\n",
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
      clan->logout_message = {};
    clan->logout_message = {};
    return 1;
  }

  if (strstr(arg, "~"))
  {
    ch->sendln("No ~s fatt butt!");
    return 0;
  }

  QString curr;

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
    clan->logout_message = {};

  clan->logout_message = (arg);

  dc_sprintf(buf, "Clan logout message changed to: %s\r\n", clan->logout_message);
  ch->send(buf);
  return 1;
}

qint32 clan_login_message(CharacterPtr ch, QString arg)
{
  Clan *clan = {};

  QString buf;

  clan = get_clan(ch);
  if (!*arg)
  {
    dc_sprintf(buf, "$3Syntax$R:  clans login <new message>\r\n\r\nCurrent message: %s\r\n",
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
      clan->login_message = {};
    clan->login_message = {};
    return 1;
  }

  if (strstr(arg, "~"))
  {
    ch->sendln("No ~s fatt butt!");
    return 0;
  }

  QString curr;

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
    clan->login_message = {};

  clan->login_message = (arg);

  dc_sprintf(buf, "Clan login message changed to: %s\r\n", clan->login_message);
  ch->send(buf);
  return 1;
}

qint32 clan_email(CharacterPtr ch, QString arg)
{
  Clan *clan = {};

  QString buf;
  QString text;

  clan = get_clan(ch);
  arg = one_argumentnolow(arg, text);
  if (!*text)
  {
    dc_sprintf(buf, "$3Syntax$R:  clans email <new address>\r\n\r\nCurrent address: %s\r\n", clan->email);
    ch->send(buf);
    ch->sendln("To not have any email use:  clans email delete");
    return 0;
  }

  if (!strncmp(text, "delete", 6))
  {
    if (clan->email)
      clan->email = {};
    clan->email = {};
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
    clan->email = {};

  clan->email = (text);

  dc_sprintf(buf, "Clan email changed to: %s\r\n", clan->email);
  ch->send(buf);
  return 1;
}

command_return_t do_ctell(CharacterPtr ch, QString arg, cmd_t cmd)
{
  CharacterPtr pch;
  class Connection *desc;
  QString buf;

  if (!ch->clan)
  {
    ch->sendln("But you don't belong to a clan!");
    return ReturnValue::eFAILURE;
  }

  ObjectPtr tmp_obj;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].vnum() == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from speaking!");
      return ReturnValue::eFAILURE;
    }

  while (isspace(*arg))
    arg++;
  if (!has_right(ch, CLAN_RIGHTS_CHANNEL) && ch->getLevel() < 51)
  {
    ch->sendln("You don't have the right to talk to your clan.");
    return ReturnValue::eFAILURE;
  }

  if (!(isSet(ch->misc, DC::LogChannel::CHANNEL_CLAN)))
  {
    ch->sendln("You have that channel off!!");
    return ReturnValue::eFAILURE;
  }

  if (!*arg)
  {
    QQueue<QString> tmp = get_clan(ch)->ctell_history;
    if (tmp.empty())
    {
      ch->sendln("No one has said anything lately.");
      return ReturnValue::eFAILURE;
    }

    ch->sendln("Here are the last 10 ctells:");
    while (!tmp.empty())
    {
      send_to_char((tmp.front()).c_str(), ch);
      tmp.pop();
    }

    return ReturnValue::eFAILURE;
  }

  dc_sprintf(buf, "You tell the clan, '%s'\r\n", arg);
  ansi_color(GREEN, ch);
  ch->send(buf);
  ansi_color(NTEXT, ch);

  dc_sprintf(buf, "%s tells the clan, '%s'\r\n", qPrintable(ch->shortdesc_or_name()), arg);
  bool yes;
  for (desc = DC::getInstance()->connections_; desc; desc = desc->next)
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
      if (DC::getInstance()->obj_index[tmp_obj->item_number].vnum() == SILENCE_OBJ_NUMBER)
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

  dc_sprintf(buf, "$2%s tells the clan, '%s'$R\r\n", qPrintable(ch->shortdesc_or_name()), arg);
  get_clan(ch)->ctell_history.push(buf);
  if (get_clan(ch)->ctell_history.size() > 10)
  {
    get_clan(ch)->ctell_history.pop();
  }

  return ReturnValue::eSUCCESS;
}

void do_clan_list(CharacterPtr ch)
{
  Clan *clan = {};
  QString buf, buf2;

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
      buf = fmt::format("{:2} {:<20}$R {:<16} {:3} {:16L}\r\n", clan->number, qPrintable(clan->name()), clan->leader, clan->tax, clan->getBalance());
    }
    else
    {
      buf = fmt::format("{:2} {:<20}$R {:<16}\r\n", clan->number, qPrintable(clan->name()), clan->leader);
    }
    ch->send(buf);
  }
}

void do_clan_member_list(CharacterPtr ch)
{
  ClanMember *pmember = {};
  Clan *pclan = {};
  qint32 column = 1;
  QString buf, buf2[200];

  if (!(pclan = get_clan(ch->clan)))
  {
    ch->sendln("Error:  Not in clan.  Contact a god.");
    return;
  }

  ch->sendln("Members of clan:");
  dc_sprintf(buf, "  ");

  for (pmember = pclan->members; pmember; pmember = pmember->next)
  {
    dc_sprintf(buf2, "%-20s  ", qPrintable(pmember->name()));
    send_to_char(buf2, ch);

    if (0 == (column % 3))
    {
      ch->sendln("");
      column = {};
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

qint32 is_clan_leader(CharacterPtr ch)
{
  Clan *pclan = {};

  if (!ch || !(pclan = get_clan(ch->clan)))
    return 0;

  return (!(strcmp(qPrintable(ch->name()), pclan->leader)));
}

void do_clan_rights(CharacterPtr ch, QString arg)
{
  ClanMember *pmember = {};
  CharacterPtr victim = {};
  // extern QString clan_rights

  QString buf;
  QString buf2;
  QString name;
  QString last;
  qint32 bit = -1;

  half_chop(arg, name, last);

  if (!*name)
  {
    ch->sendln("$3Syntax$R:  clan rights <member> [right]");
    return;
  }

  *name = toupper(*name);

  if (!(pmember = get_member(name, ch->clan)))
  {
    dc_sprintf(buf, "Could not find '%s' in your clan.\r\n", name);
    ch->send(buf);
    return;
  }

  if (!*last)
  { // diag
    dc_sprintf(buf, "Rights for %s:\r\n-------------\r\n", qPrintable(pmember->name()));
    ch->send(buf);
    for (auto bit = 0U; *clan_rights[bit] != '\n'; bit++)
    {
      dc_sprintf(buf, "  %-15s %s\r\n", clan_rights[bit], (isSet(pmember->rights(), 1 << bit) ? "on" : "off"));
      ch->send(buf);
    }
    return;
  }

  auto bit = clan_rights.indexOf(last, Qt::CaseInsensitive);

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

  auto r = pmember->rights();
  TOGGLE_BIT(r, 1 << bit);
  pmember->rights(r);

  if (isSet(pmember->rights(), 1 << bit))
  {
    dc_sprintf(buf, "%s toggled on.\r\n", clan_rights[bit]);
    dc_sprintf(buf2, "%s has given you '%s' rights within your clan.\r\n", qPrintable(ch->shortdesc_or_name()), clan_rights[bit]);
  }
  else
  {
    dc_sprintf(buf, "%s toggled off.\r\n", clan_rights[bit]);
    dc_sprintf(buf2, "%s has taken away '%s' rights within your clan.\r\n", qPrintable(ch->shortdesc_or_name()), clan_rights[bit]);
  }
  ch->send(buf);

  if ((victim = get_char(pmember->name())))
  {
    send_to_char(buf2, victim);
  }

  save_clans();
}

void do_god_clans(CharacterPtr ch, QString arg, cmd_t cmd)
{
  Clan *clan = {};
  Clan *tarclan = {};
  clan_room_data *newroom = {};
  clan_room_data *lastroom = {};

  QString buf;
  QString buf2;
  QString select;
  QString text;
  QString last;

  qint32 i;
  qint32 x;
  qint16 skill;

  const QStringList god_values = {
      "create",
      "rename",
      "leader",
      "delete",
      "addroom",
      "list",
      "save",
      "showrooms",
      "killroom",
      "email",
      "description",
      "login",
      "logout",
      "death",
      "members",
      "rights",
      "motd"};

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
      dc_sprintf(buf + strlen(buf), "%18s", god_values[i - 1]);
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

  skill = god_values.indexOf(select, Qt::CaseInsensitive);
  if (skill < 0)
  {
    ch->sendln("That value not recognized.");
    return;
  }

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
    if (x < 1 || x > (1 << 8 * sizeof(quint16)) - 1)
    {
      ch->send(QStringLiteral("%d (%d) is an invalid clan number.\r\n").arg(x).arg((1 << 8 * sizeof(quint16)) - 1));
      return;
    }

    if (get_clan(x) != nullptr)
    {
      ch->send(QStringLiteral("%1 is an invalid clan number because it already exists.\r\n").arg(x));
      return;
    }

    clan = new Clan;
    clan->leader = (qPrintable(ch->name()));
    clan->amt = {};
    clan->founder = (qPrintable(ch->name()));
    clan->name(text);
    clan->number = x;
    clan->acc = {};
    clan->rooms = {};
    clan->next = {};
    clan->email = {};
    clan->description = {};
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

    tarclan->name(last);
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
      tarclan->leader = {};
    tarclan->leader = (last);

    ch->sendln("Clan leader name changed.");
    break;
  }
  case 3: /* delete */
  {
    arg = one_argumentnolow(arg, text);
    one_argumentnolow(arg, last);
    if (!*text || !*last)
    {
      ch->sendln("$3Syntax$R: clans <clannumber> dElEtE") = {};
      return;
    }
    if (!isexact(last, "dElEtE"))
    {
      ch->sendln("You MUST end the line with 'dElEtE' to the clan.") = {};
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
    auto newroom = new clan_room_data;
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
        dc_sprintf(buf, "%d\r\n", newroom->room_number);
      }
      else
      {
        strcpy(buf, "Room Data without number.  PROBLEM.\r\n");
      }
      strncat(buf2, buf, sizeof(buf2) - 1);
      buf2[sizeof(buf2) - 1] = {};
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
      lastroom = {};
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
        newroom = {};
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

void do_leader_clans(CharacterPtr ch, QString arg, cmd_t cmd)
{
  ClanMember *pmember = {};
  //  Clan * tarclan = {};
  //  clan_room_data * newroom = {};
  //  clan_room_data * lastroom = {};

  QString buf;
  QString select;
  //  QString text;
  //  QString last;

  qint32 i, j, leader;
  //  qint32 x;
  qint16 skill;

  const QStringList mortal_values = {
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

  qint32 right_required[] = {
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

  if (!(pmember = get_member(qPrintable(ch->name()), ch->clan)))
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
    for (i = {}; *mortal_values[i] != '\n'; i++)
    {
      // only show rights the player has.  Leader has all.
      if (!leader && right_required[i] &&
          !isSet(pmember->rights(), right_required[i]))
        continue;

      dc_sprintf(buf + strlen(buf), "%18s", mortal_values[i]);
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

  skill = mortal_values.indexOf(select, Qt::CaseInsensitive);
  if (skill < 0)
  {
    ch->sendln("That value not recognized.");
    return;
  }
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

void Clan::log(QString log_entry)
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

void show_clan_log(CharacterPtr ch)
{
  QString s;
  std::ifstream fin;
  std::stringstream fname;
  std::stack<QString> logstack;

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
  buffer << "The following are your clan's most recent 5 pages of log entries:\r\n";
  qint32 line = 1;
  while (logstack.size())
  {
    buffer << logstack.top() << "\r\n";
    logstack.pop();

    // 5 pages, 21 lines each
    if (line++ > 21 * 5)
    {
      break;
    }
  }

  page_string(ch->desc, const_cast<QString>(buffer.str().c_str()), 1);
}

qint32 needs_clan_command(CharacterPtr ch)
{
  if (has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
    return 1;
  if (has_right(ch, CLAN_RIGHTS_RIGHTS))
    return 1;

  return 0;
}

command_return_t do_clans(CharacterPtr ch, QString arg, cmd_t cmd)
{
  Clan *clan = {};
  QString tmparg;

  QString buf;
  tmparg = one_argument(arg, buf);

  if (buf == u"rights"_s)
  {
    tmparg = one_argument(tmparg, buf);

    if (!*buf) // only do this if they want clan rights on themselves
    {
      qint32 bit = -1;
      ClanMember *pmember = {};

      if (!(pmember = get_member(qPrintable(ch->name()), ch->clan)))
      {
        ch->sendln("You don't seem to be in a clan.");
        return ReturnValue::eSUCCESS;
      }

      dc_sprintf(buf, "Rights for %s:\r\n-------------\r\n", qPrintable(pmember->name()));
      ch->send(buf);
      for (bit = {}; *clan_rights[bit] != '\n'; bit++)
      {
        dc_sprintf(buf, "  %-15s %s\r\n", clan_rights[bit], (isSet(pmember->rights(), 1 << bit) ? "on" : "off"));
        ch->send(buf);
      }
      return ReturnValue::eSUCCESS;
    }
  }

  if (ch->isPlayer() && (ch->getLevel() >= COORDINATOR))
  {
    do_god_clans(ch, arg, cmd);
    return ReturnValue::eSUCCESS;
  }

  if (ch->clan && (clan = get_clan(ch)) &&
      (!strcmp(qPrintable(ch->name()), clan->leader) || needs_clan_command(ch)))
  {
    do_leader_clans(ch, arg, cmd);
    return ReturnValue::eSUCCESS;
  }

  do_clan_list(ch);

  return ReturnValue::eSUCCESS;
}

command_return_t do_cinfo(CharacterPtr ch, QString arg, cmd_t cmd)
{
  Clan *clan;
  qint32 nClan;
  QString buf;

  if (!*arg)
  {
    ch->sendln("$3Syntax$R:  cinfo <clannumber>");
    return ReturnValue::eSUCCESS;
  }

  nClan = atoi(arg);

  if (!(clan = get_clan(nClan)))
  {
    ch->sendln("That is not a valid clan number.");
    return ReturnValue::eFAILURE;
  }
  dc_sprintf(buf, "$3Name$R:           %s$R $3($R%d$3)$R\r\n"
                  "$3Leader$R:         %s\r\n"
                  "$3Contact Email$R:  %s\r\n"
                  "$3Clan Hall$R:      %s\r\n"
                  "$3Clan info$R:\r\n"
                  "$3----------$R\r\n",
             qPrintable(clan->name()),
             nClan,
             clan->leader,
             clan->email ? clan->email : "(No Email)",
             clan->rooms ? "Yes" : "No");
  ch->send(buf);

  // This has to be separate, or if the leader uses $'s, it comes out funky
  dc_sprintf(buf, "%s\r\n",
             clan->description ? clan->description : "(No Description)\r\n");
  ch->send(buf);

  if (ch->getLevel() >= POWER || (clan->leader == ch->name()) && nClan == ch->clan) ||
      (nClan == ch->clan && has_right(ch, CLAN_RIGHTS_MESSAGES)))
    {
      dc_sprintf(buf, "$3Login$R:          %s\r\n"
                      "$3Logout$R:         %s\r\n"
                      "$3Death$R:          %s\r\n",
                 clan->login_message ? clan->login_message : "(No Message)",
                 clan->logout_message ? clan->logout_message : "(No Message)",
                 clan->death_message ? clan->death_message : "(No Message)");
      ch->send(buf);
    }
  if (ch->getLevel() >= POWER || (clan->leader == ch->name()) && nClan == ch->clan) ||
      (nClan == ch->clan && has_right(ch, CLAN_RIGHTS_MEMBER_LIST)))
    {
      dc_sprintf(buf, "$3Balance$R:         %lu coins\r\n", clan->getBalance());
      ch->send(buf);
    }
  return ReturnValue::eSUCCESS;
}

command_return_t do_whoclan(CharacterPtr ch, QString arg, cmd_t cmd)
{
  Clan *clan;
  class Connection *desc;
  CharacterPtr pch;
  QString buf;
  qint32 found;

  send_to_char("                  O N L I N E   C L A N   "
               "M E M B E R S\r\n\r\n",
               ch);

  QString buf2;
  one_argument(arg, buf2);
  qint32 clan_num = {};

  if (buf2[0])
    clan_num = atoi(buf2);

  for (clan = DC::getInstance()->clan_list; clan; clan = clan->next)
  {
    found = {};
    if (clan_num && clan->number != clan_num)
      continue;
    for (desc = DC::getInstance()->connections_; desc; desc = desc->next)
    {
      if (desc->connected || !(pch = desc->character))
        continue;
      if (pch->clan != clan->number || pch->getLevel() >= OVERSEER ||
          (!CAN_SEE(ch, pch) && ch->clan != pch->clan))
        continue;
      if (found == 0)
      {
        dc_sprintf(buf, "$3Clan %s$R:\r\n", qPrintable(clan->name()));
        ch->send(buf);
      }
      if (clan->number == ch->clan && has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
        dc_sprintf(buf, "  %s %s %s\r\n", qPrintable(pch->shortdesc_or_name()), (!strcmp(qPrintable(pch->name()), clan->leader) ? "$3($RLeader$3)$R" : ""), isSet(GET_TOGGLES(pch), Player::PLR_NOTAX) ? "(NT)" : "(T)");
      else
        dc_sprintf(buf, "  %s %s\r\n", qPrintable(pch->shortdesc_or_name()), (!strcmp(qPrintable(pch->name()), clan->leader) ? "$3($RLeader$3)$R" : ""));
      ch->send(buf);
      found++;
    }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_cmotd(CharacterPtr ch, QString arg, cmd_t cmd)
{
  Clan *clan;

  if (!ch->clan || !(clan = get_clan(ch)))
  {
    ch->sendln("You aren't the member of any clan!");
    return ReturnValue::eFAILURE;
  }

  if (!clan->clanmotd)
  {
    ch->sendln("There is no motd for your clan currently.");
    return ReturnValue::eSUCCESS;
  }

  ch->send(clan->clanmotd);
  return ReturnValue::eSUCCESS;
}

command_return_t do_ctax(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString arg1;
  if (!ch->clan)
  {
    ch->sendln("You not a member of a clan.");
    return ReturnValue::eFAILURE;
  }
  arg = one_argument(arg, arg1);
  if (!is_number(arg1))
  {
    ch->send(QStringLiteral("Your clan's current tax rate is %d.\r\n").arg(get_clan(ch)->tax));
    return ReturnValue::eFAILURE;
  }
  if (!has_right(ch, CLAN_RIGHTS_TAX))
  {
    ch->sendln("You don't have the right to modify taxes.");
    return ReturnValue::eFAILURE;
  }

  qint32 tax = atoi(arg1);
  if (tax < 0 || tax > 99)
  {
    ch->sendln("You can have a maximum of 99% in taxes.");
    return ReturnValue::eFAILURE;
  }
  get_clan(ch)->tax = tax;
  ch->sendln("Your clan's tax rate has been modified.");
  save_clans();
  return ReturnValue::eSUCCESS;
}

// This command deposits gold into a clan bank account
command_return_t Character::do_cdeposit(QStringList arguments, cmd_t cmd)
{
  QString arg1;

  if (clan == 0 || get_clan(clan) == nullptr)
  {
    send("You are not a member of a clan.\r\n");
    return ReturnValue::eFAILURE;
  }

  if (isPlayerGoldThief())
  {
    send("Launder your money elsewhere, thief!\r\n");
    return ReturnValue::eFAILURE;
  }

  if (DC::getInstance()->world[in_room].number != DC::SORPIGAL_BANK_ROOM)
  {
    send("This can only be done at the Sorpigal bank.\r\n");
    return ReturnValue::eFAILURE;
  }

  if (arguments.isEmpty())
  {
    send("Usage: cdeposit <number>\r\n");
    return ReturnValue::eFAILURE;
  }

  arg1 = arguments.at(0);
  bool ok = false;
  gold_t dep = arg1.toULongLong(&ok);
  if (ok == false)
  {
    send("How much do you want to deposit?\r\n");
    return ReturnValue::eFAILURE;
  }

  if (getGold() < dep)
  {
    send(QStringLiteral("You don't have %L1 $B$5gold$R coins to deposit into your clan account.\r\n").arg(dep));
    send(QStringLiteral("You only have %L1 $B$5gold$R coins on you.\r\n").arg(getGold()));
    return ReturnValue::eFAILURE;
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
  Clan *clan = get_clan(this->clan);
  if (clan != nullptr)
  {
    clan->log(log_entry);
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_cwithdraw(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString arg1;
  if (!ch->clan)
  {
    ch->sendln("You not a member of a clan.");
    return ReturnValue::eFAILURE;
  }
  if (!has_right(ch, CLAN_RIGHTS_WITHDRAW) && ch->getLevel() < 108)
  {
    ch->sendln("You don't have the right to withdraw $B$5gold$R from your clan's account.");
    return ReturnValue::eFAILURE;
  }
  if (DC::getInstance()->world[ch->in_room].number != DC::SORPIGAL_BANK_ROOM)
  {
    ch->sendln("This can only be done at the Sorpigal bank.");
    return ReturnValue::eFAILURE;
  }

  arg = one_argument(arg, arg1);
  if (!is_number(arg1))
  {
    ch->sendln("How much do you want to withdraw?");
    return ReturnValue::eFAILURE;
  }
  quint64 wdraw = atoi(arg1);
  if (get_clan(ch)->getBalance() < wdraw || wdraw < 0)
  {
    ch->sendln("Your clan lacks the funds.");
    return ReturnValue::eFAILURE;
  }
  ch->addGold(wdraw);
  get_clan(ch)->cwithdraw(wdraw);
  if (wdraw == 1)
  {
    ch->send(QStringLiteral("You withdraw 1 $B$5gold$R coin.\r\n").arg(wdraw));
  }
  else
  {
    ch->send(QStringLiteral("You withdraw %1 $B$5gold$R coins.\r\n").arg(wdraw));
  }
  save_clans();
  ch->save();

  QString buf;
  if (wdraw == 1)
  {
    dc_snprintf(buf, MAX_INPUT_LENGTH, "%s withdrew 1 $B$5gold$R coin from the clan bank account.\r\n", qPrintable(ch->name()));
  }
  else
  {
    dc_snprintf(buf, MAX_INPUT_LENGTH, "%s withdrew %lu $B$5gold$R coins from the clan bank account.\r\n", qPrintable(ch->name()), wdraw);
  }
  Clan *clan = get_clan(ch);
  if (clan != nullptr)
  {
    clan->log(buf);
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_cbalance(CharacterPtr ch, QString arg, cmd_t cmd)
{
  if (!ch->clan)
  {
    ch->sendln("You not a member of a clan.");
    return ReturnValue::eFAILURE;
  }
  if (DC::getInstance()->world[ch->in_room].number != DC::SORPIGAL_BANK_ROOM)
  {
    ch->sendln("This can only be done at the Sorpigal bank.");
    return ReturnValue::eFAILURE;
  }

  if (!has_right(ch, CLAN_RIGHTS_MEMBER_LIST))
  {
    ch->sendln("You don't have the right to see your clan's account.");
    return ReturnValue::eFAILURE;
  }
  std::stringstream ss;
  ss.imbue(std::locale("en_US"));
  ss << get_clan(ch)->getBalance();
  ch->send(QStringLiteral("Your clan has %s $B$5gold$R coins in the bank.\r\n").arg(ss.str().c_str()));
  return ReturnValue::eSUCCESS;
}

void remove_totem(ObjectPtr altar, ObjectPtr totem)
{
  const auto &character_list = DC::getInstance()->character_list;

  std::for_each(character_list.begin(), character_list.end(),
                [&altar, totem](CharacterPtr const &t)
                {
                  if (t->isPlayer() && t->altar == altar)
                  {
                    qint32 j;
                    for (j = {}; j < totem->num_affects; j++)
                      affect_modify(t, totem->affected[j].location,
                                    totem->affected[j].modifier, -1, false);
                    redo_hitpoints(t);
                    redo_mana(t);
                    redo_ki(t);
                  }
                });
}

void add_totem(ObjectPtr altar, ObjectPtr totem)
{
  const auto &character_list = DC::getInstance()->character_list;

  std::for_each(character_list.begin(), character_list.end(),
                [&altar, totem](CharacterPtr const &t)
                {
                  if (t->isPlayer() && t->altar == altar)
                  {
                    qint32 j;
                    for (j = {}; j < totem->num_affects; j++)
                      affect_modify(t, totem->affected[j].location,
                                    totem->affected[j].modifier, -1, true);
                  }
                });
}

void remove_totem_stats(CharacterPtr ch, qint32 stat)
{
  ObjectPtr a;
  if (!ch->altar)
    return;
  for (a = ch->altar->contains; a; a = a->next_content)
  {
    qint32 j;
    if (a->obj_flags.type_flag != ITEM_TOTEM)
      continue;
    for (j = {}; j < a->num_affects; j++)
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

void add_totem_stats(CharacterPtr ch, qint32 stat)
{
  ObjectPtr a;
  if (!ch->altar)
    return;
  for (a = ch->altar->contains; a; a = a->next_content)
  {
    qint32 j;
    if (a->obj_flags.type_flag != ITEM_TOTEM)
      continue;
    for (j = {}; j < a->num_affects; j++)
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

qint32 count_plrs(qint32 zone, qint32 clan)
{
  const auto &character_list = DC::getInstance()->character_list;

  qint32 i = std::count_if(character_list.begin(), character_list.end(), [&zone, &clan](CharacterPtr const &tmpch)
                           {
      if (tmpch->isPlayer() && DC::getInstance()->world[tmpch->in_room].zone == zone && clan == tmpch->clan &&
	  tmpch->getLevel() < 100 && tmpch->getLevel() > 10)
      return true;
      else
      return false; });

  return i;
}

class takeover_pulse_data
{
public:
  takeover_pulse_data *next;
  qint32 clan1; // defending clan
  qint32 clan1points;
  qint32 clan2; // challenging clan
  qint32 clan2points;
  qint32 zone;
  qint32 pulse;
};
takeover_pulse_data *pulse_list = {};

bool can_collect(qint32 zone)
{
  takeover_pulse_data *take;
  for (take = pulse_list; take; take = take->next)
    if (zone == take->zone && take->clan2 != -2)
      return false;
  return true;
}

bool can_challenge(qint32 clan, qint32 zone)
{
  takeover_pulse_data *take;
  for (take = pulse_list; take; take = take->next)
    if (take->clan2 == -2 &&
        take->clan1 == clan && zone == take->zone)
      return false;
    else if (zone == take->zone && take->clan2 >= 0 && take->clan1 >= 0)
      return false;
  return true;
}

void takeover_pause(qint32 clan, qint32 zone)
{
  auto pl = new takeover_pulse_data;
  pl->next = pulse_list;
  pl->clan1 = clan;
  pl->clan2 = -2;
  pl->pulse = {};
  pl->zone = zone;
  pulse_list = pl;
}

void claimArea(qint32 clan, bool defend, bool challenge, qint32 clan2, qint32 zone)
{
  QString buf;

  if (challenge)
  {
    if (!defend)
    {
      //      DC::getInstance()->zones.value(zone).gold = {};
      if (clan)
        dc_sprintf(buf, "\r\n##Clan %s has broken clan %s's control of%s!\r\n", qPrintable(get_clan(clan)->name()), qPrintable(get_clan(clan2)->name()), qPrintable(DC::getInstance()->zones.value(zone).name()));
      else
        dc_sprintf(buf, "\r\n##Clan %s's control of%s has been broken!\r\n", qPrintable(get_clan(clan2)->name()), qPrintable(DC::getInstance()->zones.value(zone).name()));

      takeover_pause(clan2, zone);
    }
    else
    {
      takeover_pause(clan2, zone);
      if (clan2)
        dc_sprintf(buf, "\r\n##Clan %s has defended against clan %s's challenge for control of%s!\r\n", qPrintable(get_clan(clan)->name()), qPrintable(get_clan(clan2)->name()), qPrintable(DC::getInstance()->zones.value(zone).name()));
      else
        dc_sprintf(buf, "\r\n##Clan %s has defended their control of%s!\r\n", qPrintable(get_clan(clan)->name()), qPrintable(DC::getInstance()->zones.value(zone).name()));
    }
  }
  else
  {
    if (clan)
      dc_snprintf(buf, sizeof(buf), "\r\n##%s has been claimed by clan %s!\r\n", qPrintable(DC::getInstance()->zones.value(zone).name()), qPrintable(get_clan(clan)->name()));

    //     DC::getInstance()->zones.value(zone).gold = {};
  }
  DC::setZoneClanOwner(zone, clan);

  send_info(buf);
}

qint32 count_controlled_areas(qint32 clan)
{
  quint64 zones = {};
  for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
  {
    if (zone.clanowner == clan && can_collect(zone_key))
    {
      zones++;
    }
  }

  for (takeover_pulse_data *plc = pulse_list; plc; plc = plc->next)
  {
    if ((plc->clan1 == clan || plc->clan2 == clan) && plc->clan2 != -2)
    {
      zones++;
    }
  }

  return zones;
}

void recycle_pulse_data(takeover_pulse_data *pl)
{
  takeover_pulse_data *plc, *plp = {};
  for (plc = pulse_list; plc; plc = plc->next)
  {
    if (plc == pl)
    {
      if (plp)
        plp->next = plc->next;
      else
        pulse_list = plc->next;
      pl = {};
      return; // No point going on..
    }
    plp = plc;
  }
}

qint32 online_clan_members(qint32 clan)
{
  const auto &character_list = DC::getInstance()->character_list;

  qint32 i = std::count_if(character_list.begin(), character_list.end(),
                           [&clan](CharacterPtr const &Tmpch)
                           {
                             if (Tmpch->isPlayer() && Tmpch->clan == clan && Tmpch->getLevel() < 100 && Tmpch->desc && Tmpch->getLevel() > 10)
                               return true;
                             else
                               return false;
                           });

  return i;
}

void check_victory(takeover_pulse_data *take)
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
  qint32 clan = arg1.clan;
  QString buf;
  if (count_controlled_areas(clan) > online_clan_members(clan))
  { // One needs to go.
    qint32 i = number(1, count_controlled_areas(clan));
    qint32 a, z = {};
    for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
    {
      if (zone.clanowner == clan && can_collect(zone_key))
        if (++z == i)
        {
          //			DC::getInstance()->zones.value(a].gold = {};
          zone.clanowner = {};
          dc_sprintf(buf, "\r\n##Clan %s has lost control of%s!\r\n", qPrintable(get_clan(clan)->name()), qPrintable(zone.name()));
          send_info(buf);
          return;
        }
    }

    takeover_pulse_data *pl;
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

void check_quitter(CharacterPtr ch)
{
  if (!ch->clan || ch->getLevel() >= 100)
    return;

  timer_data *timer = new timer_data;
  timer->arg1.clan = ch->clan;
  timer->function = check_quitter;
  timer->timeleft = 30;
  addtimer(timer);
}

void pk_check(CharacterPtr ch, CharacterPtr victim)
{
  if (!ch || !victim)
    return;
  // if (!ch->clan || !victim->clan) return; // No point;
  takeover_pulse_data *plc, *pln;
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

bool can_lose(takeover_pulse_data *take)
{
  const auto &character_list = DC::getInstance()->character_list;

  auto result = std::find_if(character_list.begin(), character_list.end(), [&take](CharacterPtr const &ch)
                        {
		if (ch->isPlayer() && DC::getInstance()->world[ch->in_room].zone == take->zone
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
  takeover_pulse_data *take, *next;
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
      QString buf;
      std::dc_sprintf(buf, "\r\n##Control of%s has been lost!\r\n", qPrintable(DC::getInstance()->zones.value(take->zone).name()));
      send_info(buf);
      DC::setZoneClanOwner(take->zone, 0);
      recycle_pulse_data(take);
      continue;
    }

    qint32 favour = count_plrs(take->zone, take->clan1) - count_plrs(take->zone, take->clan2);

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
    return ReturnValue::eFAILURE;
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
      return ReturnValue::eFAILURE;
    }
  }

  if (!has_right(this, CLAN_RIGHTS_AREA) && !clanless_challenge)
  {
    this->sendln("You have not been granted that right.");
    return ReturnValue::eFAILURE;
  }

  if (arg == "withdraw")
  {
    if (can_collect(DC::getInstance()->world[in_room].zone))
    {
      this->sendln("There is no challenge to withdraw from.");
      return ReturnValue::eFAILURE;
    }

    if (!affected_by_spell(SKILL_CLANAREA_CHALLENGE))
    {
      this->sendln("You did not issue the challenge, or you have waited too long to withdraw.");
      return ReturnValue::eFAILURE;
    }

    takeover_pulse_data *take;
    for (take = pulse_list; take; take = take->next)
      if (take->zone == DC::getInstance()->world[in_room].zone &&
          take->clan2 == clan)
      {
        take->clan1points += 20;
        check_victory(take);
        this->sendln("You withdraw your challenge.");
        return ReturnValue::eSUCCESS;
      }
    this->sendln("Your did not issue this challenge.");
    return ReturnValue::eFAILURE;
  }
  else if (arg == "claim")
  {
    if (affected_by_spell(SKILL_CLANAREA_CLAIM))
    {
      this->sendln("You need to wait before you can attempt to claim an area.");
      return ReturnValue::eFAILURE;
    }

    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner == 0 && !can_challenge(clan, DC::getInstance()->world[in_room].zone))
    {
      this->sendln("You cannot claim this area right now.");
      return ReturnValue::eFAILURE;
    }

    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner > 0)
    {
      this->send(QStringLiteral("This area is claimed by %s, you need to challenge to obtain ownership.\r\n").arg(qPrintable(get_clan(DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner)->name())));

      return ReturnValue::eFAILURE;
    }
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).isNoClaim())
    {
      this->sendln("This area cannot be claimed.");
      return ReturnValue::eFAILURE;
    }
    if (count_controlled_areas(clan) >= online_clan_members(clan))
    {
      this->sendln("You cannot claim any more areas.");
      return ReturnValue::eFAILURE;
    }

    affected_type af;
    af.type = SKILL_CLANAREA_CLAIM;
    af.duration = 30;
    af.modifier = {};
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(this, &af, DC::PULSE_TIMER);

    auto zone_key = DC::getInstance()->world[in_room].zone;
    DC::setZoneClanOwner(zone_key, clan);

    send("You claim the area on behalf of your clan.\r\n");
    send(QStringLiteral("\r\n##%1 has been claimed by %2!\r\n").arg(DC::getZoneName(zone_key)).arg(get_clan(clan)->name()));

    return ReturnValue::eSUCCESS;
  }
  else if (arg == "yield")
  {
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner == 0)
    {
      this->sendln("This zone is not under anyone's control.");
      return ReturnValue::eFAILURE;
    }
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner != clan)
    {
      this->sendln("This zone is not under your clan's control.");
      return ReturnValue::eFAILURE;
    }

    takeover_pulse_data *take;
    for (take = pulse_list; take; take = take->next)
      if (take->zone == DC::getInstance()->world[in_room].zone &&
          take->clan1 == clan && take->clan2 != -2)
      {
        take->clan2points += 20;
        check_victory(take);
        return ReturnValue::eSUCCESS;
      }
    this->sendln("You yield the area on behalf of your clan.");
    QString buf;
    dc_sprintf(buf, "\r\n##Clan %s has yielded control of%s!\r\n", qPrintable(get_clan(clan)->name()), qPrintable(DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).name()));
    send_info(buf);
    DC::setZoneClanOwner(DC::getInstance()->world[in_room].zone, 0);

    return ReturnValue::eSUCCESS;
  }
  else if (arg == "collect")
  {
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner == 0)
    {
      this->sendln("This area is not under anyone's control.");
      return ReturnValue::eFAILURE;
    }

    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner != clan)
    {
      this->sendln("This area is not under your clan's control.");
      return ReturnValue::eFAILURE;
    }

    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).gold == 0)
    {
      this->sendln("There is no $B$5gold$R to collect.");
      return ReturnValue::eFAILURE;
    }
    if (!can_collect(DC::getInstance()->world[in_room].zone))
    {
      this->sendln("There is currently an active challenge for this area, and collecting is not possible.");
      return ReturnValue::eFAILURE;
    }
    get_clan(this)->cdeposit(DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).gold);
    this->send(QStringLiteral("You collect %d $B$5gold$R for your clan's treasury.\r\n").arg(DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).gold));

    DC::setZoneClanGold(DC::getInstance()->world[in_room].zone, 0);
    save_clans();
    return ReturnValue::eSUCCESS;
  }
  else if (arg == "list")
  {
    qint32 z = {};
    for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
      if (DC::getInstance()->zones.value(i).clanowner == clan)
      {
        if (++z == 1)
          this->send(QStringLiteral("$BAreas Claimed by %s:$R\r\n").arg(qPrintable(get_clan(this)->name())));
        this->send(QStringLiteral("%d)%s\r\n").arg(z).arg(qPrintable(DC::getInstance()->zones.value(i).name())));
      }

    if (z == 0)
    {
      this->sendln("Your clan has not claimed any areas.");
      return ReturnValue::eFAILURE;
    }
    return ReturnValue::eSUCCESS;
  }
  else if (arg == "challenge")
  {
    if (affected_by_spell(SKILL_CLANAREA_CHALLENGE))
    {
      this->sendln("You need to wait before you can attempt to challenge an area.");
      return ReturnValue::eFAILURE;
    }
    if (level_ < 40)
    {
      this->sendln("You must be level 40 to issue a challenge.");
      return ReturnValue::eFAILURE;
    }

    // most annoying one for last.
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner == 0)
    {
      this->sendln("This area is not under anyone's control, you could simply claim it.");
      return ReturnValue::eFAILURE;
    }
    if (!can_challenge(clan, DC::getInstance()->world[in_room].zone))
    {
      this->sendln("You cannot issue a challenge for this area at the moment.");
      return ReturnValue::eFAILURE;
    }
    if (DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner == clan && !clanless_challenge)
    {
      this->sendln("Your clan already controls this area!");
      return ReturnValue::eFAILURE;
    }

    if (count_controlled_areas(clan) >= online_clan_members(clan) && !clanless_challenge)
    {
      this->sendln("You cannot own any more areas.");
      return ReturnValue::eFAILURE;
    }

    affected_type af;
    af.type = SKILL_CLANAREA_CHALLENGE;
    af.duration = 60;
    af.modifier = {};
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(this, &af, DC::PULSE_TIMER);

    // no point checking for noclaim flag, at this point it already IS under someone's control
    auto pl = new takeover_pulse_data;
    pl->next = pulse_list;
    pl->clan1 = DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).clanowner;
    pl->clan2 = clan;
    pl->clan1points = pl->clan2points = {};
    pl->pulse = {};
    pl->zone = DC::getInstance()->world[in_room].zone;
    pulse_list = pl;
    QString buf;
    if (!clanless_challenge)
      dc_sprintf(buf, "\r\n##Clan %s has challenged clan %s for control of%s!\r\n", qPrintable(get_clan(this)->name()), qPrintable(get_clan(pl->clan1)->name()), qPrintable(DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).name()));
    else
      dc_sprintf(buf, "\r\n##Clan %s's control of%s is being challenged!\r\n", qPrintable(get_clan(pl->clan1)->name()), qPrintable(DC::getInstance()->zones.value(DC::getInstance()->world[in_room].zone).name()));
    send_info(buf);
    return ReturnValue::eSUCCESS;
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
  return ReturnValue::eSUCCESS;
}

bool others_clan_room(CharacterPtr ch, Room *room)
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
  Clan *clan;
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

Clan::Clan(QString n)
    : MinimumEntity(n)
{
  balance = {};
  leader = {};
  founder = {};
  email = {};
  description = {};
  login_message = {};
  death_message = {};
  logout_message = {};
  clanmotd = {};
  rooms = {};
  members = {};
  next = {};
  acc = {};
  amt = {};
  number = {};
  tax = {};
}

void Clan::cdeposit(const quint64 &deposit)
{
  balance += deposit;
}

quint64 Clan::getBalance(void)
{
  return balance;
}

void Clan::cwithdraw(const quint64 &withdraw)
{
  balance -= withdraw;
}

void Clan::setBalance(const quint64 &value)
{
  balance = value;
}

ClanMember::ClanMember(CharacterPtr ch)
    : next(nullptr), name_(QString()), unused1_(0), unused2_(0), unused3_(0), unused4_(QString()), rights_(0), rank_(0), time_joined_(0)
{
  if (ch)
  {
    name_ = ch->name();
  }
}
