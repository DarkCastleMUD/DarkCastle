/***************************************************************************
 *  file: db.c , Database module.                          Part of DIKUMUD *
 *  Usage: Loading/Saving chars, booting world, resetting etc.             *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 *                                                                         *
 *  Revision History                                                       *
 *  11/10/2003  Onager   Modified clone_mobile() to set more appropriate   *
 *                       amounts of gold                                   *
 ***************************************************************************/
/* $Id: db.cpp,v 1.229 2015/06/14 02:38:12 pirahna Exp $ */
/* Again, one of those scary files I'd like to stay away from. --Morc XXX */

#include "DC/DC.h"
qint32 load_debug = {};

Room &World::operator[](room_t room_key)
{
  static Room generic_room = {};

  if (room_key > top_of_world || room_key == DC::NOWHERE || !rooms.contains(room_key))
  {
    generic_room = {};
    return generic_room;
  }

  return rooms[room_key];
}

message_list fight_messages[MAX_MESSAGES]; /* fighting messages   */
skill_quest *skill_list;                   // List of skill quests.

QString webpage;    /* the webbrowser connect screen*/
QString greetings1; /* the greeting screen          */
QString greetings2; /* the other greeting screen    */
QString greetings3;
QString greetings4;
QString credits;   /* the Credits List              */
QString motd;      /* the messages of today         */
QString imotd;     /* the immortal messages of today*/
QString story;     /* the game story                */
QString help;      /* the main help page            */
QString new_help;  /* the main new help page            */
QString new_ihelp; /* the main immortal help page            */
QString info;      /* the info text                 */

help_index_element_new *new_help_table = {};

qint32 top_of_mobt = {}; /* top of mobile index table       */
qint32 top_of_objt = {}; /* top of object index table       */

time_info_data time_info;  /* the infomation about the time   */
weather_data weather_info; /* the infomation about the weather */

VaultPtr vault_table = {};

/* local procedures */
void setup_dir(auto &stream, qint32 room, qint32 dir);
void load_banned();
void boot_world(void);
void do_godlist();
void half_chop(const QString str, QString arg1, QString arg2);
world_file_list_item *new_mob_file_item(QString filename, qint32 room_nr);
world_file_list_item *new_obj_file_item(QString filename, qint32 room_nr);

QString read_next_worldfile_name(auto &streamflWorldIndex);

void fix_shopkeepers_inventory();
qint32 file_to_string(const QString name, QString buf);
void reset_time(void);
void clear_char(CharacterPtr ch);

// MOBprogram locals
qint32 mprog_name_to_type(QString name);

extern bool MOBtrigger;

/* external refs */

help_index_t build_help_index(QTextStream &stream);
// The Room implementation
// -Sadus 9/1/96

Arena &Room::arena(void)
{
  if (isArena())
    return arena_;

  static Arena generic_arena;
  generic_arena = {};
  return generic_arena;
}
}

// add new tracks to the head of the list. When the list
// gets longer than 11, remove the tail and delete it.
void Room::AddTrackItem(TracksPtr newTrack)
{
  if (!tracks_)
  {
    tracks = newTrack;
    last_track = newTrack;
    nTracks = 1;
    return;
  }

  // add it to the head of the double-linked list
  newTrack->next = tracks;
  tracks->previous = newTrack;
  tracks = newTrack;

  if (++nTracks > 11)
  {
    TracksPtr pScent;
    pScent = last_track->previous;
    pScent->next = {};
    last_track = pScent;
    nTracks--;
  }
}

bool operator==(const deny_data &dd1, const deny_data &dd2)
{
  QList<decltype(dd1.vnum)> denies1;
  const deny_data *curr1 = &dd1;
  do
  {
    denies1.push_back(curr1->vnum);
  } while ((curr1 = curr1->next));

  QList<decltype(dd2.vnum)> denies2;
  const deny_data *curr2 = &dd2;
  do
  {
    denies2.push_back(curr2->vnum);
  } while ((curr2 = curr2->next));

  return denies1 == denies2;
}

bool operator==(extra_descr_data &edd1, extra_descr_data &edd2)
{
  QMap<QString, QString> extra_descriptions1;
  extra_descr_data *curr1 = &edd1;
  do
  {
    extra_descriptions1.insert(curr1->keyword_, curr1->description_);
  } while ((curr1 = curr1->next));

  QMap<QString, QString> extra_descriptions2;
  extra_descr_data *curr2 = &edd2;
  do
  {
    extra_descriptions2.insert(curr2->keyword_, curr2->description_);
  } while ((curr2 = curr2->next));

  return extra_descriptions1 == extra_descriptions2;
}

bool operator==(const RoomDirection &rdd1, const RoomDirection &rdd2)
{
  return (rdd1.bracee == rdd2.bracee &&
          rdd1.exit_info == rdd2.exit_info &&
          QString(rdd1.general_description) == QString(rdd2.general_description) &&
          rdd1.key == rdd2.key &&
          QString(rdd1.keyword) == QString(rdd2.keyword) &&
          rdd1.to_room == rdd2.to_room);
}

bool operator==(const Room &r1, const Room &r2)
{
  for (qint32 direction = {}; direction < MAX_DIRS; ++direction)
  {
    if (r1.dir_option[direction] == r2.dir_option[direction])
    {
      continue;
    }
    else if (r1.dir_option[direction] && r2.dir_option[direction] &&
             *r1.dir_option[direction] == *r2.dir_option[direction])
    {
      continue;
    }
    else
    {
      return false;
    }
  }
  return (r1.number == r2.number &&
          r1.zone == r2.zone &&
          r1.zonePtr == r2.zonePtr &&
          r1.sector_type == r2.sector_type &&
          r1.denied == r2.denied &&
          r1.name_ == r2.name_ &&
          r1.description_ == r2.description_ &&
          r1.ex_description == r2.ex_description &&
          r1.room_flags == r2.room_flags &&
          r1.temp_room_flags == r2.temp_room_flags &&
          r1.light == r2.light &&
          r1.funct == r2.funct &&
          // r1.contents == r2.contents &&
          // r1.people_ == r2.people_ &&
          // r1.tracks_.size() == r2.tracks_.size() &&
          // r1.tracks == r2.tracks &&
          // r1.iFlags == r2.iFlags &&
          // ((r1.paths == r2.paths) || (r1.paths && r2.paths && *r1.paths == *r2.paths)) &&
          !memcmp(r1.allow_class, r2.allow_class, sizeof(r1.allow_class)));
}

TracksPtr Room::TrackItem(qint32 nIndex)
{
  qint32 nr;
  TracksPtr pScent;

  for (pScent = tracks, nr = 1; pScent;
       pScent = pScent->next, nr++)
    if (nr == nIndex)
      return pScent;

  return 0;
}

void Character::add_to_bard_list(void)
{
  if (GET_CLASS(this) != CLASS_BARD)
    return;

  auto curr = new pulse_data;

  curr->thechar = this;
  curr->next = bard_list;
  bard_list = curr;
}

void Character::remove_from_bard_list(void)
{
  pulse_data *curr = {};
  pulse_data *last = {};

  if (!bard_list)
    return;

  if (bard_list->thechar == this)
  {
    curr = bard_list;
    bard_list = bard_list->next;
    curr = {};
  }
  else
  {
    last = bard_list;
    for (curr = bard_list->next; curr; curr = curr->next)
    {
      if (curr->thechar == this)
      {
        last->next = curr->next;
        curr = {};
        break;
      }
      last = curr;
    }
  }
}

const QStringList funnybootmessages =
    {
        "Booting mobs...\r\n",
        "Booting objs...\r\n",
        "Booting clans...\r\n",
        "Booting booter...\r\n",
        "Booting linkdeads...\r\n",
        "Booting world...\r\n",
        "Booting feet...\r\n",
        "Reassigning room pointers...\r\n",
        "Rebuilding virtual exits...\r\n",
        "Decoding human genome...\r\n",
        "Loading help entries...\r\n",
        "Booting shops...\r\n",
        "Generating dynamic areas...\r\n",
        "Assigning mobile process pointers...\r\n",
        "Synchronizing threads...\r\n",
        "Initializing mob AI engine...\r\n",
        "True Randomization(tm) of backstab generator...\r\n",
        "Managing swap file access...\r\n",
        "Determining best way to break...\r\n",
        "Corrupting random pfiles...\r\n",
        "Booting socials...\r\n",
        "Booting spell message...\r\n",
        "Assigning skills matrix...\r\n",
        "Blanking object trees...\r\n",
        "Compensating for effect of sunspot radiation...\r\n",
        "Wonder-twin powers activate...\r\n",
        "><(((:>...\r\n",
        "Syncronizing zone timers...\r\n",
        "End-game violence generation...\r\n",
        "Mutex/semaphore initialization complete...\r\n",
        "Hitting alt-f4 makes this go faster...\r\n",
        "Adding commands to radix...\r\n",
        "Loading static text files...\r\n",
        "Generating winning Keno numbers...\r\n",
        "Solving linear mob alignment....\r\n",
        "Verifying little endian alignment...\r\n",
        "Initializing logic gates...\r\n",
        "Consulting Magic 8-ball...\r\n",
        "Connecting logical circuit...\r\n",
        "Binding proper ports....\r\n",
        "Creating obj references...\r\n",
        "Cacheing zone connection QMap...\r\n",
        "Feeding mean llamas...\r\n",
        "Breeding squirrels...\r\n",
        "Graphing optimization lines...\r\n",
        "Strict defining template arrays...\r\n",
        "All your base are belong to us...\r\n",
        "Importing external consts...\r\n",
        "Configuring alt.binaries.erotica.asian.senior-citizen.sheep downloader...\r\n",
        "Configuring configurator...\r\n",
        "Planing valid edge queue...\r\n",
        "Verifying void alignment...\r\n",
        "Initializing BFS search parameters...\r\n",
        "Shaving fuzzy logic...\r\n",
        "Upgrading RAM...\r\n",
        "Good eq-load detected..\r\nPurging world..\r\nAll mobs repopped with moss eq..\r\n",
        "Checking code for tpyos...\r\n",
        "Generating loot tables..\r\n",
        "Giving barbarians fireshield...\r\n",
        "Processing signal handling...\r\n",
        "Sneaking in random PokeMUD code...\r\n",
        "Defining mob track vectors...\r\n",
        "Locking doors...\r\n",
        "Boosting random mob stats...\r\n",
        "Finding something fun in game to remove...\r\n",
        "Determining array dope vectors...\r\n",
        "Divide By Cucumber Error. Please Reinstall Universe And Reboot\r\n",
        "Removing crash bugs...\r\n",
        "Cooking Swedish meatballs...\r\n",
        "Brewing Canadian beer...\r\n",
        "Searching for intelligent players....searching....searching....searching\r\n",
        "Coding bug...\r\n",
        "Uploading Urizen's ABBA mp3s...\r\n",
        "09 F9 11 02 9D 74 E3 5B D8 41 56 C5 63 56 88 C0\r\n",
        "Redecorating the tavern...\r\n",
        "Putting cover sheets on TPS reports.\r\n",
        "Nerfing <insert class>...\r\n",
        "Smash forehead on keyboard to continue.\r\n",
        "Enter any 11-digit prime number to continue.\r\n",
        "Error saving file! Format drive now? (Y/Y)\r\n",
        "User Error: Replace user.\r\n",
        "Windows found. Delete? (Y/N)\r\n"};

void funny_boot_message()
{
  ConnectionPtr conn;

  extern qint32 was_hotboot;

  if (!was_hotboot)
    return;

  qint32 num = sizeof(funnybootmessages) / sizeof;

  num = number(0, num - 1);

  for (auto &d : connections_)
    write_to_descriptor(conn->descriptor, funnybootmessages[num]);
}

/* Write skillquest file.
 It checks if ch exists everywhere it is used,
 so this can be called from other places without
 a character attached. */
command_return_t do_write_skillquest(CharacterPtr ch, QString argument, cmd_t cmd)
{
  skill_quest *curr;
  FILE *stream;

  if (!(stream = fopen(SKILL_QUEST_FILE, "w")))
  {
    if (ch)
      ch->sendln("Can't open the skill quest file.");
    return ReturnValue::eFAILURE;
  }
  for (curr = skill_list; curr; curr = curr->next)
  {
    dc_fprintf(stream, "%d %s~\n", curr->num, curr->message);
    dc_fprintf(stream, "%d %d\n", curr->clas, curr->level);
  }
  dc_fprintf(stream, "0\n");

  ch->sendln("Skill quests saved.");
  return ReturnValue::eSUCCESS;
}

void load_skillquests()
{
  skill_quest *newsq, *last = {};
  skill_list = {};
  qint32 i;
  FILE *stream;

  if (!(stream = fopen(SKILL_QUEST_FILE, "r")))
  {
    logentry(u"Cannot open skill quest file."_s, 0, DC::LogChannel::LOG_MISC);
    abort();
  }

  while ((i = fread_int(stream, 0, 1000)) != 0)
  {
    auto newsq = new skill_quest;

    newsq->num = i;
    if (find_sq(i))
    {
      QString buf;
      dc_sprintf(buf, "%d duplicate.", i);
      logentry(buf, 0, DC::LogChannel::LOG_BUG);
    }
    newsq->message = fread_string(stream);
    newsq->clas = fread_int(stream, 0, 32768);
    newsq->level = fread_int(stream, 0, 200);
    newsq->next = {};

    if (last)
      last->next = newsq;
    else
      skill_list = newsq;

    last = newsq;
  }
}

/*************************************************************************
 *  routines for booting the system                                       *
 *********************************************************************** */

/* body of the booting system */
void DC::boot_db(void)
{
  qint32 help_rec_count = {};

  reset_time();

  logentry(u"************** BOOTING THE MUD ***********"_s, 0, DC::LogChannel::LOG_SOCKET);
  logverbose(u"************** BOOTING THE MUD ***********"_s, 0, DC::LogChannel::LOG_MISC);
  logentry(u"************** BOOTING THE MUD ***********"_s, 0, DC::LogChannel::LOG_WORLD);
  logverbose(u"Reading aux files."_s);
  file_to_string(WEBPAGE_FILE, webpage);
  file_to_string(GREETINGS1_FILE, greetings1);
  file_to_string(GREETINGS2_FILE, greetings2);
  file_to_string(GREETINGS3_FILE, greetings3);
  file_to_string(GREETINGS4_FILE, greetings4);
  file_to_string(CREDITS_FILE, credits);
  file_to_string(MOTD_FILE, motd);
  file_to_string(IMOTD_FILE, imotd);
  file_to_string(STORY_FILE, story);
  file_to_string(HELP_PAGE_FILE, help);
  file_to_string(INFO_FILE, info);
  file_to_string(NEW_HELP_PAGE_FILE, new_help);
  file_to_string(NEW_IHELP_PAGE_FILE, new_ihelp);

  funny_boot_message();

  do_godlist();
  logverbose(u"Godlist done!"_s);
  logverbose(u"Booting clans..."_s);

  boot_clans();

  logverbose(u"Loading new news file."_s);
  loadnews();

  logverbose(u"Loading new help file."_s);

  // new help file stuff
  if (!(new_help_fl = fopen(NEW_HELP_FILE, "r")))
  {
    perror(NEW_HELP_FILE);
    abort();
  }
  help_rec_count = count_hash_records(new_help_fl);

  if (!(new_help_fl = fopen(NEW_HELP_FILE, "r")))
  {
    perror(NEW_HELP_FILE);
    abort();
  }
  load_new_help(new_help_fl);

  // end new help files

  logverbose(u"Opening help file."_s);

  QFile help_keyword_file(HELP_KWRD_FILE);
  if (!help_keyword_file.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly))
  {
    perror(HELP_KWRD_FILE);
    abort();
  }
  QTextStream help_keyword_fl(&help_keyword_file);
  help_index = build_help_index(help_keyword_fl);

  logverbose(u"Loading the zones"_s);
  boot_zones();

  logverbose(u"Loading the world."_s);
  top_of_world_alloc = 2000;

  funny_boot_message();

  boot_world();

  logverbose(u"Renumbering the world."_s);
  renum_world();

  funny_boot_message();

  logverbose(u"Generating mob indices/loading all mobiles"_s);
  generate_mob_indices(&top_of_mobt, mob_index);

  logverbose(u"Generating object indices/loading all objects"_s);
  generate_obj_indices(&top_of_objt, obj_index);

  funny_boot_message();

  logverbose(u"renumbering zone table"_s);
  renum_zone_table();

  logverbose(u"Looking for unordered mobiles..."_s);
  find_unordered_mobiles();

  logverbose(u"Looking for unordered objects..."_s);
  find_unordered_objects();

  if (cf.bport == false)
  {
    logverbose(u"Loading Corpses."_s);
    load_corpses();
  }

  logverbose(u"Loading messages."_s);
  load_messages(MESS_FILE);
  load_messages(MESS2_FILE, 2000);

  logverbose(u"Loading socials."_s);
  boot_social_messages();

  logverbose(u"Processing game portals..."_s);
  load_game_portals();

  logverbose(u"Loading emoting objects..."_s);
  load_emoting_objects();

  logverbose(u"Adding clan room flags to rooms..."_s);
  assign_clan_rooms();

  logverbose(u"Assigning function pointers."_s);
  assign_mobiles();
  assign_objects();
  assign_rooms();

  // DC::config &cf = cf;
  if (cf.verbose_mode)
  {
    qInfo("\n[ Room  Room]\t{Level}\t  Author\tZone\n");
  }

  // auto &zones = zones;
  for (auto [zone_key, zone] : zones.asKeyValueRange())
  {
    if (cf.verbose_mode)
    {
      qInfo("%s", qUtf8Printable(u"[%1 %2]\t%3."_s.arg(zone.getBottom(), 5).arg(zone.getTop(), 5).arg(zone.name())));
    }

    zone.reset(Zone::ResetType::full);
  }

  logverbose(u"Loading banned list"_s);
  load_banned();

  logverbose(u"Loading skill quests."_s);
  load_skillquests();

  logverbose(u"Assigning inventory to shopkeepers"_s);
  fix_shopkeepers_inventory();

  logverbose(u"Turning on MOB Progs"_s);
  MOBtrigger = true;

  logverbose(u"Loading quest one liners."_s);
  load_quests();

  logverbose(u"Loading vaults."_s);
  load_vaults();

  logverbose(u"Loading player hints."_s);
  load_hints();

  logverbose(u"Loading auction tickets."_s);
  TheAuctionHouse.Load();
}

/*
 command_return_t do_motdload(CharacterPtr ch, QString argument, cmd_t cmd)
 {
 file_to_string(MOTD_FILE, motd);
 file_to_string(IMOTD_FILE, imotd);
 ch->sendln("Motd and Imotd both reloaded.");
 return ReturnValue::eSUCCESS;
 }
 */
void DC::do_godlist(void)
{
  logverbose(u"Reading ../lib/wizlist.txt"_s);
  QFile wizlist_file("../lib/wizlist.txt");
  if (!wizlist_file.open(QIODeviceBase::ReadOnly))
  {
    logentry(u"db.c: error reading ../lib/wizlist.txt"_s, ANGEL, DC::LogChannel::LOG_BUG);
    wizlist_file.close();
    return;
  }

  do
  {
    auto wizlist_file_line = wizlist_file.readLine().split(' ');
    QString immortal_name = wizlist_file_line.value(0);
    bool ok = false;
    level_ immortal_level = wizlist_file_line.value(1).toULongLong(&ok);
    if (!ok)
    {
      immortal_level = {};
    }

    if (immortal_name.startsWith('@'))
    {
      wizlist.push_back({u"@"_s, 0});
      break;
    }
    wizlist.push_back({immortal_name, immortal_level});
    assert(wizlist.length() < 1000);
  } while (true);

  logverbose(u"Done!\r\n"_s);
  wizlist_file.close();
}

void DC::write_wizlist(std::stringstream &filename)
{
  write_wizlist(filename.str().c_str());
}

void DC::write_wizlist(QString filename)
{
  write_wizlist(filename.c_str());
}

void DC::write_wizlist(const QString filename)
{
  QFile wizlist_file(filename);
  auto file_opened = wizlist_file.open(QIODeviceBase::WriteOnly);
  if (!file_opened)
  {
    logentry(u"Unable to open wizlist file: %1"_s.arg(filename));
    return;
  }

  for (const auto &entry : wizlist)
  {
    if (entry.getName().startsWith('@'))
    {
      wizlist_file.write("@ @\n");
      break;
    }
    if (entry.getLevel() >= IMMORTAL)
    {
      wizlist_file.write(u"%1 %2\n"_s.arg(entry.getName()).arg(entry.getLevel()).toLocal8Bit());
    }
  }
  wizlist_file.close();
}

void DC::update_wizlist(CharacterPtr ch)
{
  qint32 x;

  if (ch->isNonPlayer())
    return;

  for (auto &entry : wizlist)
  {
    if (entry.getName().startsWith('@'))
    {
      if (ch->isMortalPlayer())
        return;
      entry.setName(ch->name());
      entry.setLevel(ch->getLevel());

      wizlist.push_back({u"@"_s, 0});
      break;
    }
    else
    {
      if (isexact(entry.getName(), ch->name()))
      {
        entry.setLevel(ch->getLevel());
        break;
      }
    }
  }

  write_wizlist("../lib/wizlist.txt");

  in_port_t port1 = {};
  if (cf.ports.size() > 0)
  {
    port1 = cf.ports[0];
  }

  std::stringstream ssbuffer;
  ssbuffer << HTDOCS_DIR << port1 << "/wizlist.txt";
  write_wizlist(ssbuffer.str().c_str());
}

command_return_t do_wizlist(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf, lines, space;
  qint32 x{}, z{1};
  level_ current_level = {};
  qint32 gods_each_level[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  qint32 line_length, sp;

  const QStringList names =
      {
          "(:) == Immortals == (:)",
          "(:) == Architects == (:)",
          "(:) == Deities == (:)",
          "(:) == Overseers == (:)",
          "(:) == Divinities == (:)",
          "(:) == Honorary Immortals == (:)",
          "(:) == Coordinators == (:)",
          "(:) == Senior Coordinators == (:)",
          "(:) == Implementers == (:)"};

  for (sp = {}; sp < 80; sp++)
    space[sp] = ' ';

  // count the number of gods at each level, store in array gods_each_level
  for (x = {};; x++)
  {
    if (wizlist[x].getName()[0] == '@')
      break;
    gods_each_level[wizlist[x].getLevel() - IMMORTAL]++;
  }

  buf[0] = '\0';
  for (current_level = IMPLEMENTER; current_level >= IMMORTAL; current_level--)
  {
    if (gods_each_level[current_level - IMMORTAL] == 0)
      continue;

    line_length = dc_strlen(names[current_level - IMMORTAL]);
    sp = 79 - line_length;
    sp /= 2;
    space[sp + 1] = '\0';
    dc_sprintf(buf + dc_strlen(buf), "\r\n%s%s\r\n", space,
               names[current_level - IMMORTAL]);
    space[sp + 1] = ' ';

    lines[0] = '\0';
    for (x = {};; x++)
    {
      if (wizlist[x].getName()[0] == '@')
      {
        z = 1;
        if (!lines.isEmpty())
        {
          line_length = dc_strlen(lines) - 2;
          lines[dc_strlen(lines) - 2] = '\n';
          lines[dc_strlen(lines) - 1] = '\r';
          sp = 79 - line_length;
          sp /= 2;
          space[sp + 1] = '\0';
          dc_sprintf(buf + dc_strlen(buf), "%s%s", space, lines);
          space[sp + 1] = ' ';
          lines[0] = '\0';
        }
        break;
      }

      if (wizlist[x].getLevel() != current_level)
        continue;

      if (z++ % 5)
        dc_sprintf(lines + dc_strlen(lines), "%s, ", qPrintable(wizlist[x].getName()));
      else
      {
        dc_sprintf(lines + dc_strlen(lines), "%s\r\n", qPrintable(wizlist[x].getName()));
        line_length = dc_strlen(lines) - 2;
        sp = 79 - line_length;
        sp /= 2;
        space[sp + 1] = '\0';
        dc_sprintf(buf + dc_strlen(buf), "%s%s", space, lines);
        space[sp + 1] = ' ';
        lines[0] = '\0';
      }
    }
  }

  page_string(ch->conn_, buf, 1);
  return 1;
}

/* reset the time in the game from file */
void reset_time(void)
{
  qint32 beginning_of_time = 650336715;

  time_info_data mud_time_passed(time_t t2, time_t t1);

  time_info = mud_time_passed(time(0), beginning_of_time);

  switch (time_info.hours)
  {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  {
    weather_info.sunlight = SUN_DARK;
    break;
  }
  case 5:
  {
    weather_info.sunlight = SUN_RISE;
    break;
  }
  case 6:
  case 7:
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
  case 13:
  case 14:
  case 15:
  case 16:
  case 17:
  case 18:
  case 19:
  case 20:
  {
    weather_info.sunlight = SUN_LIGHT;
    break;
  }
  case 21:
  {
    weather_info.sunlight = SUN_SET;
    break;
  }
  case 22:
  case 23:
  default:
  {
    weather_info.sunlight = SUN_DARK;
    break;
  }
  }
  logverbose(u"Current Gametime: %1H %2D %3M %4Y."_s.arg(time_info.hours).arg(time_info.day).arg(time_info.month).arg(time_info.year));

  weather_info.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    weather_info.pressure += dice(1, 50);
  else
    weather_info.pressure += dice(1, 80);

  weather_info.change = {};

  if (weather_info.pressure <= 980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure <= 1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure <= 1020)
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;
}

/* generate index table for monster file */
mob_index_data *DC::generate_mob_indices(qint32 *top, mob_index_data *index)
{
  qint32 i = {};
  QString buf;
  QString log_buf;
  FILE *flMobIndex;
  FILE *stream;
  QString temp;
  QString endfile;
  world_file_list_item *pItem = {};
  //  extern short code_testing_mode;

  logverbose(u"Opening mobile file index."_s);
  if (cf.test_mobs)
  {
    if (!(flMobIndex = fopen(MOB_INDEX_FILE_TINY, "r")))
    {
      logentry(u"Could not open index file."_s);
      abort();
    }
  }
  else
  {
    if (!(flMobIndex = fopen(MOB_INDEX_FILE, "r")))
    {
      logentry(u"Could not open index file."_s);
      abort();
    }
  }

  logverbose(u"Opening object files."_s);

  // note, we don't worry about free'ing temp, cause it's held in the "mob_file_list"
  for (temp = read_next_worldfile_name(flMobIndex);
       temp.isEmpty() == false;
       temp = read_next_worldfile_name(flMobIndex))
  {
    dc_strcpy(endfile, "mobs/");
    dc_strcat(endfile, qPrintable(temp));

    if (cf.verbose_mode)
    {
      logentry(temp, 0, DC::LogChannel::LOG_MISC);
    }

    if (!(stream = fopen(endfile, "r")))
    {
      perror(endfile);
      logf(IMMORTAL, DC::LogChannel::LOG_BUG, "generate_mob_indices: could not open mob file: %s", qPrintable(endfile));
      abort();
    }

    pItem = new_mob_file_item(temp, i);

    for (;;)
    {
      if (fgets(buf, 81, stream))
      {

        if (*buf == '#')
        { /* allocate new_new cell */
          if (i >= MAX_INDEX)
          {
            perror("Too many mob indexes");
            abort();
          }
          vnum_t vnum = {};
          sscanf(buf, "#%ld", &vnum);
          index[i].vnum(vnum);
          index[i].qty = {};
          index[i].non_combat_func = {};
          index[i].combat_func = {};
          index[i].programs_ = {};
          index[i].class_programs_ = {};
          index[i].progtypes = {};
          currentVNUM(index[i].vnum());
          if (!(index[i].item = (CharacterPtr)read_mobile(i, stream)))
          {

            dc_sprintf(log_buf, "Unable to load mobile %lu!\r\n", index[i].vnum());
            logentry(log_buf, ANGEL, DC::LogChannel::LOG_BUG);
          }
          i++;
        }
        else if (*buf == '$') /* EOF */
          break;
      }
      else
      {
        dc_sprintf(endfile, "Bad chararacter (%s)", buf);
        logentry(endfile, 0, DC::LogChannel::LOG_MISC);
        abort();
      }
    }

    pItem->lastnum = (i - 1);
  }
  *top = i - 1;

  /*
   Here the index gets processed, and mob classes gets
   assigned. (Not done in read_mobile 'cause of
   the fact that all mobs aren't read yet,
   and an attempt to assign non-existant mob
   procs would be bad).
   */
  for (i = {}; i <= top_of_mobt; i++)
  {
    add_mobspec(i);
  }
  return (index);
}

void add_mobspec(qint32 i)
{
  if (i < 0)
    return;

  CharacterPtr a = (CharacterPtr)mob_index[i].item;
  if (!a)
    return;
  if (!a->c_class)
    return;

  qint32 mob = {};
  MobileProgramPtr mprg = {};

  switch (a->c_class)
  {
  case CLASS_MAGIC_USER:
    if (a->getLevel() < 21)
      mob = 101;
    else if (a->getLevel() < 35)
      mob = 102;
    else if (a->getLevel() < 51)
      mob = 103;
    else
      mob = 104;
    break;
  case CLASS_CLERIC:
    if (a->getLevel() < 21)
      mob = 105;
    else if (a->getLevel() < 35)
      mob = 106;
    else if (a->getLevel() < 51)
      mob = 107;
    else
      mob = 108;
    break;
  case CLASS_WARRIOR:
    if (a->getLevel() < 21)
      mob = 109;
    else if (a->getLevel() < 35)
      mob = 110;
    else if (a->getLevel() < 51)
      mob = 111;
    else
      mob = 112;
    break;
  case CLASS_BARBARIAN:
    if (a->getLevel() < 21)
      mob = 113;
    else if (a->getLevel() < 35)
      mob = 114;
    else if (a->getLevel() < 51)
      mob = 115;
    else
      mob = 116;
    break;
  case CLASS_MONK:
    if (a->getLevel() < 21)
      mob = 117;
    else if (a->getLevel() < 35)
      mob = 118;
    else if (a->getLevel() < 51)
      mob = 119;
    else
      mob = 120;
    break;
  case CLASS_THIEF:
    if (a->getLevel() < 21)
      mob = 121;
    else if (a->getLevel() < 35)
      mob = 122;
    else if (a->getLevel() < 51)
      mob = 123;
    else
      mob = 124;
    break;
  case CLASS_PALADIN:
    if (a->getLevel() < 21)
      mob = 125;
    else if (a->getLevel() < 35)
      mob = 126;
    else if (a->getLevel() < 51)
      mob = 127;
    else
      mob = 128;
    break;
  case CLASS_ANTI_PAL:
    if (a->getLevel() < 21)
      mob = 129;
    else if (a->getLevel() < 35)
      mob = 130;
    else if (a->getLevel() < 51)
      mob = 131;
    else
      mob = 132;
    break;
  case CLASS_RANGER:
    if (a->getLevel() < 21)
      mob = 133;
    else if (a->getLevel() < 35)
      mob = 134;
    else if (a->getLevel() < 51)
      mob = 135;
    else
      mob = 136;
    break;
  case CLASS_BARD:
    if (a->getLevel() < 21)
      mob = 137;
    else if (a->getLevel() < 35)
      mob = 138;
    else if (a->getLevel() < 51)
      mob = 139;
    else
      mob = 140;
    break;
  case CLASS_DRUID:
    if (a->getLevel() < 21)
      mob = 141;
    else if (a->getLevel() < 35)
      mob = 142;
    else if (a->getLevel() < 51)
      mob = 143;
    else
      mob = 144;
    break;
  case CLASS_NECROMANCER:
    if (a->getLevel() < 21)
      mob = 145;
    else if (a->getLevel() < 35)
      mob = 146;
    else if (a->getLevel() < 51)
      mob = 147;
    else
      mob = 148;
    break;
  case CLASS_PSIONIC:
    if (a->getLevel() < 21)
      mob_index[i].mobspec = mob_index[real_mobile(149)].programs_;
    else if (a->getLevel() < 35)
      mob_index[i].mobspec = mob_index[real_mobile(150)].programs_;
    else if (a->getLevel() < 51)
      mob_index[i].mobspec = mob_index[real_mobile(151)].programs_;
    else
      mob_index[i].mobspec = mob_index[real_mobile(152)].programs_;
    break;
  default:
    break;
  }

  if (mob)
  {
    mob_index[i].mobspec = mob_index[real_mobile(mob)].programs_;

    for (qint32 j = {}; j < ACT_MAX / ASIZE + 1; j++)
    {
      SET_BIT(((CharacterPtr)mob_index[i].item)->mobdata->actflags[j],
              ((CharacterPtr)mob_index[real_mobile(mob)].item)->mobdata->actflags[j]);
    }

    for (qint32 j = {}; j < AFF_MAX / ASIZE + 1; j++)
    {
      SET_BIT(((CharacterPtr)mob_index[i].item)->affected_by[j],
              ((CharacterPtr)mob_index[real_mobile(mob)].item)->affected_by[j]);
    }
  }

  if (mob_index[i].mobspec)
    for (mprg = mob_index[i].mobspec; mprg; mprg = mprg->next)
      SET_BIT(mob_index[i].progtypes, mprg->type);
}

void DC::remove_all_mobs_from_world(void)
{
  const auto &character_list = character_list;

  std::for_each(character_list.begin(), character_list.end(),
                [](CharacterPtr const &curr)
                {
                  if (curr->isNonPlayer())
                    extract_char(curr, true, u"DC::remove_all_mobs_from_world"_s);
                  else
                    do_quit(curr, "", cmd_t::SAVE_SILENTLY);
                });
  removeDead();
}

void DC::remove_all_objs_from_world()
{
  ObjectPtr curr = {};

  while ((curr = object_list))
    extract_obj(curr);
}

/* generate index table for object file */
obj_index_dataPtr DC::generate_obj_indices(qint32 *top, obj_index_dataPtr index)
{
  qint32 i = {};
  QString buf;
  QString log_buf;
  FILE *stream;
  FILE *flObjIndex;
  QString temp;
  QString endfile;
  world_file_list_item *pItem = {};

  //  if (!bport) {

  if (!(flObjIndex = fopen(OBJECT_INDEX_FILE, "r")))
  {
    logentry(u"Cannot open object file index."_s, 0, DC::LogChannel::LOG_MISC);
    abort();
  }
  /*
   } else {
   if (!(flObjIndex = fopen(OBJECT_INDEX_FILE_TINY,"r"))) {
   logentry(u"Cannot open object file index.(tiny)."_s,0,DC::LogChannel::LOG_MISC);
   abort();
   }
   }
   */
  logverbose(u"Opening object files."_s);

  // note, we don't worry about free'ing temp, cause it's held in the "obj_file_list"
  for (temp = read_next_worldfile_name(flObjIndex);
       temp.isEmpty() == false;
       temp = read_next_worldfile_name(flObjIndex))
  {
    dc_strcpy(endfile, "objects/");
    dc_strcat(endfile, qPrintable(temp));
    logverbose(temp);

    if (!(stream = fopen(endfile, "r")))
    {
      logentry(u"generate_obj_indices: could not open obj file."_s, 0, LogChannel::LOG_BUG);
      logentry(temp, 0, LogChannel::LOG_BUG);
      abort();
    }

    pItem = new_obj_file_item(temp, i);

    for (;;)
    {
      if (fgets(buf, 81, stream))
      {
        if (*buf == '#') /* allocate new_new cell */
        {
          if (i >= MAX_INDEX)
          {
            perror("Too many obj indexes");
            abort();
          }
          vnum_t vnum = {};
          sscanf(buf, "#%ld", &vnum);
          index[i].vnum(vnum);
          index[i].qty = {};
          index[i].non_combat_func = {};
          index[i].combat_func = {};
          index[i].progtypes = {};
          if (!(index[i].item = read_object(i, stream, false)))
          {
            dc_sprintf(log_buf, "Unable to load object %lu!\r\n", index[i].vnum());
            logentry(log_buf, ANGEL, LogChannel::LOG_BUG);
          }
          i++;
        }
        else if (*buf == '$') /* EOF */
          break;
      }
      else
      {
        qFatal("Error in \'%s\'.\r\n", qPrintable(endfile));
      }
    } // for ;;

    pItem->lastnum = (i - 1);

  } // for next_in_file

  *top = i - 1;

  return (index);
}

void write_one_room(LegacyFile &lf, qint32 a)
{
  FILE *f = lf.file_handle_;
  extra_descr_data *extra;

  if (!rooms.contains(a))
    return;

  dc_fprintf(f, "#%d\n", world[a].number);
  string_to_file(f, world[a].name);
  string_to_file(f, world[a].description);

  if (world[a].iFlags)
    REMOVE_BIT(world[a].room_flags, world[a].iFlags);
  dc_fprintf(f, "%lu %d %d\n", world[a].zone, world[a].room_flags, world[a].sector_type);
  if (world[a].iFlags)
    SET_BIT(world[a].room_flags, world[a].iFlags);

  /* exits */
  for (qint32 b = {}; b <= 5; b++)
  {
    if (!(world[a].dir_option[b]))
      continue;
    dc_fprintf(f, "D%d\n", b);
    if (world[a].dir_option[b]->general_description)
      string_to_file(f, world[a].dir_option[b]->general_description);
    else
      dc_fprintf(f, "~\n"); // print blank
    if (world[a].dir_option[b]->keyword)
      string_to_file(f, world[a].dir_option[b]->keyword);
    else
      dc_fprintf(f, "~\n"); // print blank
    dc_fprintf(f, "%d %d %d\n",
               world[a].dir_option[b]->exit_info,
               world[a].dir_option[b]->key,
               world[a].dir_option[b]->to_room);
  } /* exits */

  /* extra descriptions */
  // We push C-style linked list into QStack so we can maintain the order when writing
  QStack<extra_descr_data *> room_extra_descriptions;
  for (extra = world[a].ex_description; extra; extra = extra->next)
  {
    room_extra_descriptions.push(extra);
  }

  while (!room_extra_descriptions.isEmpty())
  {
    extra = room_extra_descriptions.pop();
    if (!extra)
      break;
    dc_fprintf(f, "E\n");
    if (!extra->keyword_.isEmpty())
      string_to_file(f, extra->keyword_);
    else
      dc_fprintf(f, "~\n"); // print blank
    if (!extra->description_.isEmpty())
      string_to_file(f, extra->description_);
    else
      dc_fprintf(f, "~\n"); // print blank
  } /* extra descriptions */

  for (auto deni = world[a].denied; deni; deni = deni->next)
    dc_fprintf(f, "B\n%d\n", deni->vnum);

  // Write out allowed classes if any
  for (qint32 i = {}; i < CLASS_MAX; i++)
  {
    if (world[a].allow_class[i] == true)
    {
      dc_fprintf(f, "C%d\n", i);
    }
  }

  dc_fprintf(f, "S\n");
}

qint32 DC::read_one_room(auto &stream, qint32 &room_nr)
{
  QString temp = {};
  QChar ch = {};
  qint32 dir = {};
  extra_descr_data *new_new_descr = {};
  zone_t zone_nr = {};

  ch = fread_char(stream);

  if (ch != '$')
  {
    room_nr = fread_int(stream, 0, 1000000);

    if (load_debug)
    {
      printf("Reading Room #: %d\n", room_nr);
      fflush(stdout);
    }

    temp = fread_string(stream);

    if (room_nr)
    {
      currentVNUM(room_nr);
      currentType("Room");
      currentName(temp);

      /* a new_new record to be read */

      if (room_nr >= top_of_world_alloc)
      {
        top_of_world_alloc = room_nr + 200;
      }

      if (top_of_world < room_nr)
        top_of_world = room_nr;

      rooms[room_nr] = {};

      world[room_nr].paths = {};
      world[room_nr].number = room_nr;
      world[room_nr].name = temp;
    }
    QString description = fread_string(stream);
    if (room_nr)
    {
      world[room_nr].description = description;
      world[room_nr].tracks_.size() = {};
      world[room_nr].tracks = {};
      world[room_nr].last_track = {};
      world[room_nr].denied = {};
      total_rooms++;
    }
    // Ignore recorded zone number since it may not longer be valid
    fread_int(stream, -1, 64000); // zone nr

    if (room_nr)
    {
      // Go through the zone table until world[room_nr].number is
      // in the current zone.

      bool found = false;
      zone_t zone_nr = {};
      for (auto [zone_key, zone] : zones.asKeyValueRange())
      {
        if (zone.getBottom() <= world[room_nr].number && zone.getTop() >= world[room_nr].number)
        {
          found = true;
          zone_nr = zone_key;
          break;
        }
      }
      if (!found)
      {
        QString error = u"Room %1 is outside of any zone."_s.arg(room_nr);
        logentry(error);
        logentry(u"Room outside of ANY zone.  ERROR"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
      }
      else
      {
        auto &zone = zones[zone_nr];
        if (room_nr >= zone.getBottom() && room_nr <= zone.getTop())
        {
          if (room_nr < zone.getRealBottom() || zone.getRealBottom() == 0)
          {
            zone.setRealBottom(room_nr);
          }
          if (room_nr > zone.getRealTop() || zone.getRealTop() == 0)
          {
            zone.setRealTop(room_nr);
          }
        }
        world[room_nr].zone = zone_nr;
      }
    }

    quint32 room_flags = fread_int(stream);

    if (room_nr)
    {
      world[room_nr].room_flags = room_flags;
      if (isSet(world[room_nr].room_flags, NO_ASTRAL))
        REMOVE_BIT(world[room_nr].room_flags, NO_ASTRAL);

      // This bitvector is for runtime and not stored in the files, so just initialize it to 0
      world[room_nr].temp_room_flags = {};
    }

    qint32 sector_type = fread_int(stream, -1, 64000);

    if (room_nr)
    {
      world[room_nr].sector_type = sector_type;

      if (load_debug)
      {
        printf("Flags are %lu %d %d\n", zone_nr, world[room_nr].room_flags, world[room_nr].sector_type);
        fflush(stdout);
      }

      world[room_nr].funct = {};
      world[room_nr].contents = {};
      world[room_nr].people_ = {};
      world[room_nr].light = {}; /* Zero light sources */

      for (size_t tmp = {}; tmp <= 5; tmp++)
        world[room_nr].dir_option[tmp] = {};

      world[room_nr].ex_description = {};
    }

    for (;;)
    {
      ch = fread_char(stream); /* dir field */

      /* direction field */
      if (ch == 'D')
      {
        dir = fread_int(stream, 0, 5);
        setup_dir(stream, room_nr, dir);
      }
      /* extra description field */
      else if (ch == 'E')
      {
        // strip off the \n after the E
        if (fread_char(stream) != '\n')
          fseek(stream, -1, SEEK_CUR);
        auto new_new_descr = new extra_descr_data;
        new_new_descr->keyword_ = fread_string(stream);
        new_new_descr->description_ = fread_string(stream);

        if (room_nr)
        {
          new_new_descr->next = world[room_nr].ex_description;
          world[room_nr].ex_description = new_new_descr;
        }
        else
        {
          new_new_descr = {};
        }
      }
      else if (ch == 'B')
      {
        auto deni = new deny_data;
        deni->vnum = fread_int(stream, -1, 2147483467);

        if (room_nr)
        {
          deni->next = world[room_nr].denied;
          world[room_nr].denied = deni;
        }
        else
        {
          deni = {};
        }
      }
      else if (ch == 'S') /* end of current room */
        break;
      else if (ch == 'C')
      {
        qint32 c_class = fread_int(stream, 0, CLASS_MAX);
        if (room_nr)
        {
          world[room_nr].allow_class[c_class] = true;
        }
      }
    } // of for (;;) (get directions and extra descs)

    return true;
  } // if == $
  return false;
}

QString read_next_worldfile_name(auto &streamflWorldIndex)
{
  QString filename = fread_string(flWorldIndex, 0);

  // Check for end of file marker
  if (filename == "$")
  {
    return "";
  }

  // Check for comments
  if (filename.startsWith("*"))
  {
    filename = read_next_worldfile_name(flWorldIndex);
  }

  return filename;
}

bool can_modify_this_room(CharacterPtr ch, qint32 vnum)
{
  if (ch->has_skill(COMMAND_RANGE))
    return true;

  if (ch->player->buildLowVnum <= 0 || ch->player->buildHighVnum <= 0)
    return false;
  if (ch->player->buildLowVnum > vnum)
    return false;
  if (ch->player->buildHighVnum < vnum)
    return false;
  return true;
}

bool can_modify_room(CharacterPtr ch, qint32 vnum)
{
  if (ch->has_skill(COMMAND_RANGE))
    return true;

  if (ch->player->buildLowVnum <= 0 || ch->player->buildHighVnum <= 0)
    return false;
  if (ch->player->buildLowVnum > vnum)
    return false;
  if (ch->player->buildHighVnum < vnum)
    return false;
  return true;
}

bool can_modify_this_mobile(CharacterPtr ch, qint32 vnum)
{
  if (ch->has_skill(COMMAND_RANGE))
    return true;

  if (ch->player->buildMLowVnum <= 0 || ch->player->buildMHighVnum <= 0)
    return false;
  if (ch->player->buildMLowVnum > vnum)
    return false;
  if (ch->player->buildMHighVnum < vnum)
    return false;
  return true;
}

bool can_modify_mobile(CharacterPtr ch, qint32 mob)
{
  return can_modify_this_mobile(ch, mob);
}

bool can_modify_this_object(CharacterPtr ch, qint32 vnum)
{
  if (ch->has_skill(COMMAND_RANGE))
    return true;

  if (ch->player->buildOLowVnum <= 0 || ch->player->buildOHighVnum <= 0)
    return false;
  if (ch->player->buildOLowVnum > vnum)
    return false;
  if (ch->player->buildOHighVnum < vnum)
    return false;
  return true;
}

bool can_modify_object(CharacterPtr ch, qint32 obj)
{
  return can_modify_this_object(ch, obj);
}

void DC::set_zone_saved_zone(qint32 room)
{
  setZoneNotModified(world[room].zone);
}

void DC::set_zone_modified_zone(qint32 room)
{
  setZoneModified(world[room].zone);
}

auto DC::findWorldFileWithVNUM(vnum_t vnum) -> std::expected<world_file_list_item *, search_error>
{
  world_file_list_item *world_entry = {};

  for (quint8 i = 1; i < 4; ++i)
  {
    vnum_t generation = pow(10, i);
    qDebug("searching for vnum %lu. %hhu %lu %lu", vnum, i, generation, vnum - (vnum % generation));
  }

  return std::unexpected(search_error::not_found);
}

void DC::set_zone_modified(qint32 modnum, world_file_list_item *list)
{
  world_file_list_item *curr = list;

  while (curr)
    if (modnum >= curr->firstnum && modnum <= curr->lastnum)
      break;
    else
      curr = curr->next;

  if (!curr)
  {
    auto world_file = findWorldFileWithVNUM(modnum);
    logbug(u"VNUM %1 not found in any zone in the index"_s.arg(modnum));
    return;
  }

  curr->flags = WORLD_FILE_MODIFIED;
}

void DC::set_zone_modified_world(qint32 room)
{

  set_zone_modified(room, world_file_list);
}

// rnum of mob
void DC::set_zone_modified_mob(qint32 mob)
{

  set_zone_modified(mob, mob_file_list);
}

// rnum of mob
void DC::set_zone_modified_obj(qint32 obj)
{

  set_zone_modified(obj, obj_file_list);
}

void DC::set_zone_saved(qint32 modnum, world_file_list_item *list)
{
  world_file_list_item *curr = list;

  while (curr)
    if (modnum >= curr->firstnum && modnum <= curr->lastnum)
      break;
    else
      curr = curr->next;

  if (!curr)
  {
    logentry(u"ERROR in set_zone_modified: Cannot find room!!!"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  REMOVE_BIT(curr->flags, WORLD_FILE_MODIFIED);
}

void DC::set_zone_saved_world(qint32 room)
{

  set_zone_saved(room, world_file_list);
}

void DC::set_zone_saved_mob(qint32 mob)
{

  set_zone_saved(mob, mob_file_list);
}

void DC::set_zone_saved_obj(qint32 obj)
{

  set_zone_saved(obj, obj_file_list);
}

/* de the world */
void DC::free_world_from_memory(void)
{
  extra_descr_data *curr_extra = {};
  world_file_list_item *curr_wfli = {};

  for (qint32 i = {}; i <= top_of_world; i++)
  {
    if (!rooms.contains(i))
      continue;

    if (world[i].name)
      world[i].name = {};

    if (world[i].description)
      world[i].description = {};

    while (world[i].ex_description)
    {
      curr_extra = world[i].ex_description->next;
      world[i].ex_description = {};
      world[i].ex_description = curr_extra;
    }

    for (qint32 j = {}; j < 6; j++)
      if (world[i].dir_option[j])
      {
        world[i].dir_option[j]->general_description = {};
        world[i].dir_option[j]->keyword = {};
        world[i].dir_option[j] = {};
      }

    world[i].tracks_.clear();
  }
  rooms.clear();

  curr_wfli = world_file_list;

  while (curr_wfli)
  {

    world_file_list = curr_wfli->next;
    curr_wfli = {};
    curr_wfli = world_file_list;
  }
}

void DC::free_mobs_from_memory(void)
{
  CharacterPtr curr = {};

  for (qint32 i = {}; i <= top_of_mobt; i++)
  {
    if ((curr = (CharacterPtr)mob_index[i].item))
    {
      free_char(curr);
      mob_index[i].item = {};
    }
  }
}

void DC::free_objs_from_memory(void)
{
  ObjectPtr curr = {};
  // extra_descr_data * curr_extra = {};

  for (qint32 i = {}; i <= top_of_objt; i++)
    if ((curr = obj_index[i].item))
    {
      free_obj(curr);
      obj_index[i].item = {};
    }
}

world_file_list_item *one_new_world_file_item(QString filename, qint32 room_nr)
{
  auto curr = new world_file_list_item;
  curr->filename = filename;
  curr->firstnum = room_nr;
  curr->lastnum = -1;
  curr->flags = {};
  curr->next = {};
  return curr;
}

world_file_list_item *new_w_file_item(QString filename, qint32 room_nr, world_file_list_item *&list)
{
  world_file_list_item *curr = list;

  if (!list)
  {
    list = one_new_world_file_item(filename, room_nr);
    return list;
  }

  while (curr->next)
    curr = curr->next;

  curr->next = one_new_world_file_item(filename, room_nr);
  return curr->next;
}

world_file_list_item *new_world_file_item(QString filename, qint32 room_nr)
{
  return new_w_file_item(filename, room_nr, world_file_list);
}

world_file_list_item *new_mob_file_item(QString filename, qint32 room_nr)
{
  return new_w_file_item(filename, room_nr, mob_file_list);
}

world_file_list_item *new_obj_file_item(QString filename, qint32 room_nr)
{
  return new_w_file_item(filename, room_nr, obj_file_list);
}

/* load the rooms */
void DC::boot_world(void)
{
  FILE *stream;
  FILE *flWorldIndex;
  qint32 room_nr = {};
  QString temp;
  QString endfile;
  world_file_list_item *pItem = {};

  object_list = {};

  DC::config &cf = cf;

  if (cf.test_world)
  {
    if (!(flWorldIndex = fopen(WORLD_INDEX_FILE_TINY, "r")))
    {
      qint32 fopen_errno = errno;
      logentry(u"boot_world: could not open tiny world index file '%1': %2."_s.arg(WORLD_INDEX_FILE_TINY).arg(strerror(fopen_errno)), 0, DC::LogChannel::LOG_BUG);
      abort();
    }
  }
  else
  {
    if (!(flWorldIndex = fopen(WORLD_INDEX_FILE, "r")))
    {
      qint32 fopen_errno = errno;
      logentry(u"boot_world: could not open world index file '%1': %2."_s.arg(WORLD_INDEX_FILE).arg(strerror(fopen_errno)), 0, DC::LogChannel::LOG_BUG);
      abort();
    }
  }

  // logentry(u"Booting individual world files"_s, 0, DC::LogChannel::LOG_MISC);

  // note, we don't worry about free'ing temp, cause it's held in the "world_file_list"
  for (temp = read_next_worldfile_name(flWorldIndex);
       temp.isEmpty() == false;
       temp = read_next_worldfile_name(flWorldIndex))
  {
    dc_strcpy(endfile, "world/");
    dc_strcat(endfile, qPrintable(temp));

    DC::config &cf = cf;
    if (cf.verbose_mode)
    {
      logentry(temp, 0, DC::LogChannel::LOG_MISC);
    }

    if (!(stream = fopen(endfile, "r")))
    {
      perror("fopen");
      logentry(u"boot_world: could not open world file."_s, 0, DC::LogChannel::LOG_BUG);
      logentry(temp, 0, DC::LogChannel::LOG_BUG);
      abort();
    }

    pItem = new_world_file_item(temp, room_nr);

    while (read_one_room(stream, room_nr))
      ;

    // push the first num forward until it hits a room, that way it's
    // accurate.
    // "pItem->firstnum < top_of_world_alloc" check is to insure we dont access memory not allocated to rooms
    for (; pItem->firstnum < top_of_world_alloc && !rooms.contains(pItem->firstnum); pItem->firstnum++)
      ;

    pItem->lastnum = room_nr / 100 * 100 + 99;

    room_nr++;
  }
  // logentry(u"World Boot done."_s, 0, DC::LogChannel::LOG_MISC);

  top_of_world = --room_nr;
}

/* read direction data */
void setup_dir(auto &stream, qint32 room, qint32 dir)
{
  qint32 tmp;

  if (room && world[room].dir_option[dir])
  {
    QString buf;
    dc_sprintf(buf, "Room %d attemped to created two exits in the same direction.", world[room].number);
    logentry(buf, 0, DC::LogChannel::LOG_WORLD);
    if (world[room].dir_option[dir]->general_description)
      world[room].dir_option[dir]->general_description = {};
    if (world[room].dir_option[dir]->keyword)
      world[room].dir_option[dir]->keyword = {};

    world[room].dir_option[dir] = {};
  }

  if (room)
  {
    world[room].dir_option[dir] = new RoomDirection;
  }
  QString general_description = fread_string(stream);

  if (room)
    world[room].dir_option[dir]->general_description = general_description;

  QString keyword = fread_string(stream);
  if (room)
    world[room].dir_option[dir]->keyword = keyword;

  tmp = fread_int(stream); /* tjs hack - not the right range */

  if (room)
  {
    world[room].dir_option[dir]->exit_info = tmp;
    world[room].dir_option[dir]->bracee = {};
  }

  qint16 key = fread_int(stream, -62000, 62000);
  if (room)
  {
    world[room].dir_option[dir]->key = key;
  }

  qint16 to_room = DC::NOWHERE;
  try
  {
    to_room = fread_int(stream, 0, 62000);
  }
  catch (...)
  {
  }

  if (room)
    world[room].dir_option[dir]->to_room = to_room;
}

// return true for success
qint32 DC::create_one_room(CharacterPtr ch, qint32 vnum)
{
  Room *rp = {};
  qint32 x = {};

  QString buf = {};

  if (rooms.contains(vnum))
    return 0;

  if (vnum > WORLD_MAX_ROOM)
    return 0;

  if (vnum > top_of_world)
    top_of_world = vnum;

  if (top_of_world + 1 >= top_of_world_alloc)
  {
    top_of_world_alloc = top_of_world + 200;
  }

  rooms[vnum] = {};

  rp = &world[vnum];

  rp->number = vnum;

  rp->zone = getRoomZone(rp->number);

  rp->sector_type = {};
  rp->room_flags = {};
  rp->temp_room_flags = {};
  rp->ex_description = {};
  for (x = {}; x <= 5; x++)
    rp->dir_option[x] = {};
  rp->light = {};
  rp->contents = {};
  rp->people = {};
  rp->nTracks = {};
  rp->tracks = {};
  rp->last_track = {};
  dc_sprintf(buf, "Room %d", vnum);
  rp->name = (buf);
  rp->description = u"Empty description.\r\n"_s;
  return 1;
}

void renum_world(void)
{
  qint32 room, door;

  for (room = {}; room <= top_of_world; room++)
    for (door = {}; door <= 5; door++)
      if (rooms.contains(room))
        if (world[room].dir_option[door])
          if (world[room].dir_option[door]->to_room != DC::NOWHERE)
            world[room].dir_option[door]->to_room =
                real_room(world[room].dir_option[door]->to_room);
}

void renum_zone_table(void)
{
  qint32 zone, comm;

  auto &zones = zones;
  for (auto [zone_key, zone] : zones.asKeyValueRange())
  {
    assert(zone_key != 0);
    for (comm = {}; comm < zone.cmd.size(); comm++)
    {
      zone.cmd[comm]->active = 1;
      switch (zone.cmd[comm]->command)
      {
      case 'M':
        /*if(real_room(zone.cmd[comm]->arg3) < 0) {

         qWarning("Problem in zonefile: no room number %d for mob "
         "%d - setting to J command\n", zone.cmd[comm]->arg3,
         zone.cmd[comm]->arg1);
         zone.cmd[comm]->command = 'J';
         }*/
        if (real_mobile(zone.cmd[comm]->arg1) >= 0 && real_room(zone.cmd[comm]->arg3) >= 0)
          zone.cmd[comm]->arg1 =
              real_mobile(zone.cmd[comm]->arg1);
        else
        {
          zone.cmd[comm]->active = {};
        }
        //          zone.cmd[comm]->arg3;// =
        //                real_room(zone.cmd[comm]->arg3);

        break;
      case 'O':
        if (real_object(zone.cmd[comm]->arg1) >= 0 && real_room(zone.cmd[comm]->arg3) >= 0)
          zone.cmd[comm]->arg1 = real_object(zone.cmd[comm]->arg1);
        else
          zone.cmd[comm]->active = {};

        //            if (zone.cmd[comm]->arg3 != DC::NOWHERE)
        //          zone.cmd[comm]->arg3 =
        //        real_room(zone.cmd[comm]->arg3);
        break;
      case 'G':
        if (real_object(zone.cmd[comm]->arg1) >= 0)
          zone.cmd[comm]->arg1 =
              real_object(zone.cmd[comm]->arg1);
        else
          zone.cmd[comm]->active = {};

        break;
      case 'E':
        if (real_object(zone.cmd[comm]->arg1) >= 0)
          zone.cmd[comm]->arg1 =
              real_object(zone.cmd[comm]->arg1);
        else
          zone.cmd[comm]->active = {};

        break;
      case 'P':
        if (real_object(zone.cmd[comm]->arg1) >= 0 && real_object(zone.cmd[comm]->arg3) >= 0)
        {
          zone.cmd[comm]->arg1 =
              real_object(zone.cmd[comm]->arg1);
          zone.cmd[comm]->arg3 =
              real_object(zone.cmd[comm]->arg3);
        }
        else
          zone.cmd[comm]->active = {};
        break;
      case 'D':
        if (real_room(zone.cmd[comm]->arg1) < 0)
          zone.cmd[comm]->active = {};
        else
        {
          zone.cmd[comm]->arg1 =
              real_room(zone.cmd[comm]->arg1);
          if (zone.cmd[comm]->arg1 == -1)
          {
            qWarning("Problem in zonefile: no room number for door"
                     " - setting to J command\n");
            zone.cmd[comm]->command = 'J';
          }
        }
        break;
      case '%':
        // % doesn't effect the actual world, so there's
        // nothing to renumber
        break;

      case 'K':
      case 'X':
      case '*':
      case 'J':
        // J = junk.  Just a temp holder for an empty command, ignore it
        break;
      default:
        logentry(u"Illegal character hit in renum_zone_table"_s, 0, DC::LogChannel::LOG_WORLD);
        break;
      }
    }
  }
}

void DC::free_zones_from_memory()
{
  for (auto [zone_key, zone] : zones.asKeyValueRange())
  {
    zone.name(QString());
    zone.cmd.isEmpty();
  }
}

void Zone::write(auto &stream)
{
  dc_fprintf(stream, "V2\n");
  dc_fprintf(stream, "#%lu\n", (id_ ? (bottom / 100) : 0));
  dc_fprintf(stream, "%s~\n", qPrintable(name()));
  dc_fprintf(stream, "%lu %lu %d %ld %d\n", top, lifespan, reset_mode, zone_flags, continent);

  for (qint32 i = {}; i < cmd.size(); i++)
  {
    if (cmd[i]->command == '*')
      dc_fprintf(stream, "* %s\n", qPrintable(cmd[i]->comment) ? qPrintable(cmd[i]->comment) : "");
    else if (cmd[i]->command == '%')
      dc_fprintf(stream, "%% %2d %3d %3d %s\n", cmd[i]->if_flag, cmd[i]->arg1, cmd[i]->arg2, qPrintable(cmd[i]->comment) ? qPrintable(cmd[i]->comment) : "");
    else if (cmd[i]->command == 'X')
      dc_fprintf(stream, "X %2d %5d %3d %5d%s\n", cmd[i]->if_flag, cmd[i]->arg1, cmd[i]->arg2, cmd[i]->arg3, qPrintable(cmd[i]->comment) ? qPrintable(cmd[i]->comment) : "");
    else if (cmd[i]->command == 'K')
      dc_fprintf(stream, "K %2d %5d %3d %5d%s\n", cmd[i]->if_flag, cmd[i]->arg1, cmd[i]->arg2, cmd[i]->arg3, qPrintable(cmd[i]->comment) ? qPrintable(cmd[i]->comment) : "");
    else if (cmd[i]->command == 'M')
    {
      qint32 virt = cmd[i]->active ? mob_index[cmd[i]->arg1].vnum() : cmd[i]->arg1;
      dc_fprintf(stream, "M %2d %5d %3d %5d %s\n", cmd[i]->if_flag,
                 virt,
                 cmd[i]->arg2,
                 cmd[i]->arg3,
                 qPrintable(cmd[i]->comment) ? qPrintable(cmd[i]->comment) : "");
    }
    else if (cmd[i]->command == 'P')
    {
      qint32 virt = cmd[i]->active ? obj_index[cmd[i]->arg1].vnum() : cmd[i]->arg1;
      qint32 virt2 = cmd[i]->active ? obj_index[cmd[i]->arg3].vnum() : cmd[i]->arg3;
      dc_fprintf(stream, "P %2d %5d %3d %5d %s\n", cmd[i]->if_flag,
                 virt,
                 cmd[i]->arg2,
                 virt2,
                 qPrintable(cmd[i]->comment) ? qPrintable(cmd[i]->comment) : "");
    }
    else if (cmd[i]->command == 'G')
    {
      qint32 virt = cmd[i]->active ? obj_index[cmd[i]->arg1].vnum() : cmd[i]->arg1;

      dc_fprintf(stream, "G %2d %5d %3d %5d %s\n", cmd[i]->if_flag,
                 virt,
                 cmd[i]->arg2,
                 cmd[i]->arg3,
                 qPrintable(cmd[i]->comment) ? qPrintable(cmd[i]->comment) : "");
    }
    else if (cmd[i]->command == 'O')
    {
      qint32 virt = cmd[i]->active ? obj_index[cmd[i]->arg1].vnum() : cmd[i]->arg1;
      dc_fprintf(stream, "O %2d %5d %3d %5d %s\n", cmd[i]->if_flag,
                 virt,
                 cmd[i]->arg2,
                 cmd[i]->arg3,
                 qPrintable(cmd[i]->comment) ? qPrintable(cmd[i]->comment) : "");
    }
    else if (cmd[i]->command == 'E')
    {
      qint32 virt = cmd[i]->active ? obj_index[cmd[i]->arg1].vnum() : cmd[i]->arg1;
      dc_fprintf(stream, "E %2d %5d %3d %5d %s\n", cmd[i]->if_flag,
                 virt,
                 cmd[i]->arg2,
                 cmd[i]->arg3,
                 qPrintable(cmd[i]->comment) ? qPrintable(cmd[i]->comment) : "");
    }
    else
      dc_fprintf(stream, "%c %2d %5d %3d %5d %s\n", cmd[i]->command,
                 cmd[i]->if_flag,
                 cmd[i]->arg1,
                 cmd[i]->arg2,
                 cmd[i]->arg3,
                 qPrintable(cmd[i]->comment) ? qPrintable(cmd[i]->comment) : "");
  }

  dc_fprintf(stream, "S\n$~\n");
}

zone_t DC::read_one_zone(auto &stream)
{
  static room_t last_top_vnum = {};
  zone_commands_t reset_tab;
  QString check, buf, ch;
  qint32 reset_top, i, tmp;
  QString skipper = {};
  qint32 version = 1;
  bool modified = false;

  ch = fread_char(stream);
  if (ch == 'V')
  {
    version = fread_int(stream, 0, 64000);
    ch = fread_char(stream);
    modified = true;
  }

  tmp = fread_int(stream, 0, 64000);
  check = fread_string(stream);
  // a = fread_int(stream, 0, 64000);
  /* alloc a new_new zone */
  //*num = zon = a / 100;

  auto &zones = zones;
  zone_t new_zone_key = 1;
  if (!zones.isEmpty())
  {
    new_zone_key = zones.lastKey() + 1;
  }

  Zone zone(new_zone_key);

  // logentry(u"Reading zone"_s, 0, DC::LogChannel::LOG_BUG);

  currentVNUM(tmp);
  currentType("Zone");
  currentName(check);

  zone.name(check);
  zone.setBottom(last_top_vnum + 1);
  zone.setTop(fread_int(stream, 0, WORLD_MAX_ROOM));
  last_top_vnum = zone.getTop();
  zone.setRealBottom(0);
  zone.setRealTop(0);
  zone.clanowner = {};
  zone.gold = {};
  zone.repops_without_deaths = -1;
  zone.repops_with_bonus = {};

  zone.lifespan = fread_int(stream, 0, 64000);
  zone.reset_mode = fread_int(stream, 0, 64000);
  zone.setZoneFlags(fread_int(stream));

  // if its old version set the altered flag so that
  // this zone will be saved with new format soon
  if (modified == true)
  {
    zone.setModified();
  }

  if (version > 1)
  {
    zone.continent = fread_int(stream, 0, 64000);
  }

  /* read the command table */

  for (;;)
  {
    ResetCommandPtr reset = ResetCommandPtr ::create();
    reset->comment = {}; // needs to be initialized
    reset->command = fread_char(stream);
    reset->if_flag = {};
    reset->last = {};
    reset->arg1 = {};
    reset->arg2 = {};
    reset->arg3 = {};
    if (reset->command == 'S')
    {
      break;
    }

    if (reset->command == '*')
    {
      fgets(buf, 160, stream); /* skip command */
      // skip any space
      skipper = buf;
      while (*skipper == ' ' || *skipper == '\t')
        skipper++;

      // kill terminating \n
      if (buf[dc_strlen(buf) - 1] == '\n')
        buf[dc_strlen(buf) - 1] = '\0';

      // if any, keep anything left
      if (!skipper.isEmpty())
        reset->comment = skipper;
      reset_tab.push_back(reset);
      continue;
    }

    tmp = fread_int(stream, 0, 9);
    reset->if_flag = tmp;
    reset->last = time(nullptr) - number(0, 12 * 3600);
    // randomize last repop on boot
    reset->arg1 = fread_int(stream, -64000, 2147483467);
    reset->arg2 = fread_int(stream, -64000, 2147483467);
    if (reset->arg1 > 64000)
      reset->arg1 = 2;

    if (reset->arg2 > 64000)
      reset->arg2 = 1;

    if (reset->command == 'M' ||
        reset->command == 'O' ||
        reset->command == 'E' ||
        reset->command == 'P' ||
        reset->command == 'G' ||
        reset->command == 'D' ||
        reset->command == 'X' ||
        reset->command == 'K' ||
        reset->command == 'J'
        // % only has 2 args
    )
      reset->arg3 = fread_int(stream, -64000, 32768);
    else
      reset->arg3 = {};

    reset->lastPop = {};

    if (reset->arg3 > 64000)
      reset->arg1 = 1;

    /* tjs hack - ugly tmp bug fix */
    // this just moves our cursor back 1 position
    fseek(stream, -1, SEEK_CUR);
    fgets(buf, 160, stream); /* read comment */

    skipper = buf;

    while (*skipper == ' ' || *skipper == '\t')
      skipper++;

    // kill terminating \n
    if (buf[dc_strlen(buf) - 1] == '\n')
      buf[dc_strlen(buf) - 1] = '\0';

    // if any, keep anything left
    if (!skipper.isEmpty())
      reset->comment = skipper;

    reset_tab.push_back(reset);

  } // for( ;; ) til end of zone commands

  // copy the temp into the memory
  zone.cmd = reset_tab;
  // // std::cerr << u"Insert %1 into QMap with size %2"_s.arg(zone_nr).arg(zones.size()).toStdString() << std::endl;
  zones.insert(new_zone_key, zone);
  return new_zone_key;
}

/* load the zone table and command tables */
void DC::boot_zones(void)
{
  FILE *stream;
  FILE *flZoneIndex;
  QString temp;
  QString endfile;

  DC::config &cf = cf;

  if (cf.test_world == false && cf.test_mobs == false && cf.test_objs == false)
  {
    if (!(flZoneIndex = fopen(ZONE_INDEX_FILE, "r")))
    {
      perror("fopen");
      logentry(u"boot_world: could not open world index file."_s, 0, DC::LogChannel::LOG_BUG);
      abort();
    }
  }
  else if (!(flZoneIndex = fopen(ZONE_INDEX_FILE_TINY, "r")))
  {
    perror("fopen");
    logentry(u"boot_world: could not open world index file tiny."_s, 0, DC::LogChannel::LOG_BUG);
    abort();
  }
  // logentry(u"Booting individual zone files"_s, 0, DC::LogChannel::LOG_MISC);

  for (temp = read_next_worldfile_name(flZoneIndex);
       temp.isEmpty() == false;
       temp = read_next_worldfile_name(flZoneIndex))
  {
    dc_strcpy(endfile, "zonefiles/");
    dc_strcat(endfile, qPrintable(temp));

    if (cf.verbose_mode)
    {
      logentry(temp, 0, DC::LogChannel::LOG_MISC);
    }

    if (!(stream = fopen(endfile, "r")))
    {
      perror(endfile);
      logf(IMMORTAL, DC::LogChannel::LOG_BUG, "boot_zone: could not open zone file: %s", qPrintable(endfile));
      abort();
    }

    auto zone_key = read_one_zone(stream);
    auto &zone = zones[zone_key];
    zone.setFilename(temp);
    // // std::cerr << u"%1 %2"_s.arg(zone).arg(temp).toStdString() << std::endl;
  }

  // logentry(u"Zone Boot done."_s, 0, DC::LogChannel::LOG_MISC);

  //
}

/*************************************************************************
 *  procedures for resetting, both play-time and boot-time        *
 *********************************************************************** */

/* read a mobile from MOB_FILE */
CharacterPtr DC::read_mobile(qint32 nr, FILE *stream)
{
  QString buf;
  qint32 i, j;
  qint32 tmp, tmp2, tmp3;
  CharacterPtr mob;
  QChar letter;

  i = nr;

  mob = new Character(this);
  clear_char(mob);
  mob->race = {};

  /***** String data *** */

  mob->name(fread_string(stream));
  /* set up the fread debug stuff */
  currentType("Mob");
  currentName(mob->name());
  mob->short_description(fread_string(stream));
  mob->long_description(fread_string(stream));
  mob->description(fread_string(stream));
  mob->title_ = {};

  mob->mobdata = new Mobile;
  mob->mobdata->reset = {};
  /* *** Numeric data *** */
  j = {};
  while ((tmp = fread_int(stream, -2147483467, 2147483467)) != -1)
  {
    mob->mobdata->actflags[j] = tmp;
    j++;
  }
  for (; j < ACT_MAX / ASIZE + 1; j++)
    mob->mobdata->actflags[j] = {};
  if (ISSET(mob->mobdata->actflags, ACT_NOTRACK))
    REMBIT(mob->mobdata->actflags, ACT_NOTRACK);
  mob->setType(Character::Type::NPC);

  j = {};
  while ((tmp = fread_int(stream, -2147483467, 2147483467)) != -1)
  {
    mob->affected_by[j] = tmp;
    j++;
  }
  for (; j < AFF_MAX / ASIZE + 1; j++)
    mob->affected_by[j] = {};

  mob->alignment = fread_int(stream, -2147483467, 2147483467);

  tmp = fread_int(stream, 0, MAX_RACE);
  mob->race = (QChar)tmp;

  mob->raw_str = mob->str = BASE_STAT + mob_race_mod[mob->race][0];
  mob->raw_dex = mob->dex = BASE_STAT + mob_race_mod[mob->race][1];
  mob->raw_con = mob->con = BASE_STAT + mob_race_mod[mob->race][2];
  mob->raw_intel = mob->intel = BASE_STAT + mob_race_mod[mob->race][3];
  mob->raw_wis = mob->wis = BASE_STAT + mob_race_mod[mob->race][4];

  mob->setLevel(fread_int(stream, 0, IMPLEMENTER));

  mob->hitroll = 20 - fread_int(stream, -64000, 64000);
  mob->armor = 10 * fread_int(stream, -64000, 64000);

  tmp = fread_int(stream, 0, 64000);
  tmp2 = fread_int(stream, 0, 64000);
  tmp3 = fread_int(stream, 0, 64000);

  mob->raw_hit = dice(tmp, tmp2) + tmp3;
  mob->max_hit = mob->raw_hit;
  mob->hit = mob->max_hit;

  mob->mobdata->damnodice = fread_int(stream, 0, 64000);
  mob->mobdata->damsizedice = fread_int(stream, 0, 64000);
  mob->damroll = fread_int(stream, 0, 64000);
  mob->mobdata->last_room = {};
  mob->mana = 100 + (mob->getLevel() * 10);
  mob->max_mana = 100 + (mob->getLevel() * 10);

  mob->setMove(100 + (mob->getLevel() * 10));
  mob->max_move = 100 + (mob->getLevel() * 10);

  mob->max_ki = 100.0 * (mob->getLevel() / 60.0);
  mob->ki = mob->max_ki;
  mob->raw_ki = mob->max_ki;

  mob->setGold(fread_int(stream, 0, 2147483467));
  mob->plat = {};
  mob->exp = (qint64)fread_int(stream, -2147483467, 2147483467);

  mob->setPosition(static_cast<position_t>(fread_int(stream, 0, 10)));
  mob->mobdata->default_pos = static_cast<position_t>(fread_int(stream, 0, 10));

  tmp = fread_int(stream, 0, 12);

  /* Read in ISR vlues...  (sex +3) */
  // Eventually I can remove this "if" but not until I fix them all.
  if (tmp > 2)
    tmp -= 3;

  mob->sex = (Character::sex_t)tmp;

  mob->immune = fread_int(stream);
  mob->suscept = fread_int(stream);
  mob->resist = fread_int(stream);

  // if all three are 0, then chances are someone just didn't set them, so go with
  // the race defaults.
  //    if(mob->immune == 0 && mob->suscept == 0 && mob->resist == 0)
  //  {
  SET_BIT(mob->immune, races[(qint32)mob->race].immune);
  SET_BIT(mob->suscept, races[(qint32)mob->race].suscept);
  SET_BIT(mob->resist, races[(qint32)mob->race].resist);
  // TOODO:FIXTHIS         SETBIT(mob->affected_by, races[(qint32)mob->race].affects);
  //      mob->immune  = races[(qint32)mob->race].immune;
  //    mob->suscept = races[(qint32)mob->race].suscept;
  //  mob->resist  = races[(qint32)mob->race].resist;
  //    }

  mob->c_class = {};

  do
  {
    letter = fread_char(stream);
    switch (letter)
    {
    case 'C':
      mob->c_class = fread_int(stream, 0, 2147483467);
      // fread_new_newline(stream);
      break;
    case 'T': // sTats
      mob->raw_str = mob->str = fread_int(stream, 0, 100);
      mob->raw_intel = mob->intel = fread_int(stream, 0, 100);
      mob->raw_wis = mob->wis = fread_int(stream, 0, 100);
      mob->raw_dex = mob->dex = fread_int(stream, 0, 100);
      mob->raw_con = mob->con = fread_int(stream, 0, 100);
      fread_int(stream, 0, 100); // junk var in case we add another stat
      // fread_new_newline(stream);
      break;
    case '>':
      ungetc(letter, stream);
      mprog_read_programs(stream, nr, false);
      break;
    case 'Y': // type
      mob->mobdata->mob_flags.type = (mob_type_t)fread_int(stream, mob_type_t::MOB_TYPE_FIRST, mob_type_t::MOB_TYPE_LAST);
      // fread_new_newline(stream);
      break;
    case 'V': // value
      i = fread_int(stream, 0, MAX_MOB_VALUES - 1);
      mob->mobdata->mob_flags.value[i] = fread_int(stream, -1000, 2147483467);
      // fread_new_newline(stream);
      break;
    case 'S':
      break;
    default:
      dc_sprintf(buf, "Mob %s: Invalid additional flag.  (Class, S, etc)", qPrintable(mob->short_description()));
      logentry(buf, 0, DC::LogChannel::LOG_BUG);
      break;
    }
  } while (letter != 'S');

  // fread_new_newline(stream);

  mob->weight = 200;
  mob->height = 198;

  for (i = {}; i < 3; i++)
    GET_COND(mob, i) = -1;

  // TODO - eventually have mob saving throws work by race too, but this should be good for now

  for (i = {}; i <= SAVE_TYPE_MAX; i++)
    mob->saves[i] = mob->getLevel() / 3;

  if (isSet(mob->resist, ISR_FIRE))
    mob->saves[SAVE_TYPE_FIRE] += 50;
  if (isSet(mob->resist, ISR_ACID))
    mob->saves[SAVE_TYPE_ACID] += 50;
  if (isSet(mob->resist, ISR_POISON))
    mob->saves[SAVE_TYPE_POISON] += 50;
  if (isSet(mob->resist, ISR_COLD))
    mob->saves[SAVE_TYPE_COLD] += 50;
  if (isSet(mob->resist, ISR_ENERGY))
    mob->saves[SAVE_TYPE_ENERGY] += 50;
  if (isSet(mob->resist, ISR_MAGIC))
    mob->saves[SAVE_TYPE_MAGIC] += 50;

  if (isSet(mob->suscept, ISR_FIRE))
    mob->saves[SAVE_TYPE_FIRE] -= 50;
  if (isSet(mob->suscept, ISR_ACID))
    mob->saves[SAVE_TYPE_ACID] -= 50;
  if (isSet(mob->suscept, ISR_POISON))
    mob->saves[SAVE_TYPE_POISON] -= 50;
  if (isSet(mob->suscept, ISR_COLD))
    mob->saves[SAVE_TYPE_COLD] -= 50;
  if (isSet(mob->suscept, ISR_ENERGY))
    mob->saves[SAVE_TYPE_ENERGY] -= 50;
  if (isSet(mob->suscept, ISR_MAGIC))
    mob->saves[SAVE_TYPE_MAGIC] -= 50;

  mob->mobdata->nr = nr;
  mob->conn_ = {};

  return (mob);
}

// Write a mob to file
// Assume valid mob, and file open for writing
//
void write_mobile(LegacyFile &lf, CharacterPtr mob)
{
  FILE *stream = lf.file_handle_;
  qint32 i = {};

  dc_fprintf(stream, "#%lu\n", mob_index[mob->mobdata->nr].vnum());
  string_to_file(stream, mob->name());
  string_to_file(stream, qPrintable(mob->short_description()));
  string_to_file(stream, qPrintable(mob->long_description()));
  string_to_file(stream, qPrintable(mob->description()));

  while (i < ACT_MAX / ASIZE + 1)
  {
    dc_fprintf(stream, "%d ", mob->mobdata->actflags[i]);
    i++;
  }
  dc_fprintf(stream, "-1\n");
  i = {};

  while (i < AFF_MAX / ASIZE + 1)
  {
    dc_fprintf(stream, "%d ", mob->affected_by[i]);
    i++;
  }
  dc_fprintf(stream, "-1\n");

  dc_fprintf(stream, "%d %d %llu\n"
                     "%d %d %dd%d+%d %dd%d+%d\n"
                     "%ld %ld\n"
                     "%d %d %d %d %d %d\n",
             mob->alignment,
             mob->race,
             mob->getLevel(),

             (20 - mob->hitroll),
             (qint32)(mob->armor / 10),
             GET_MAX_HIT(mob),
             1,
             0,
             mob->mobdata->damnodice,
             mob->mobdata->damsizedice,
             mob->damroll,

             mob->getGold(),
             mob->exp,

             mob->getPosition(),
             mob->mobdata->default_pos,
             mob->sex,
             mob->immune,
             mob->suscept,
             mob->resist);

  if (mob->c_class)
    dc_fprintf(stream, "C %d\n", mob->c_class);

  if ((mob->raw_str != 11 || mob->raw_dex != 11 || mob->raw_con != 11 ||
       mob->raw_intel != 11 || mob->raw_wis != 11) &&
      (mob->raw_str != BASE_STAT + mob_race_mod[mob->race][0] ||
       mob->raw_dex != BASE_STAT + mob_race_mod[mob->race][1] ||
       mob->raw_con != BASE_STAT + mob_race_mod[mob->race][2] ||
       mob->raw_intel != BASE_STAT + mob_race_mod[mob->race][3] ||
       mob->raw_wis != BASE_STAT + mob_race_mod[mob->race][4]))
  {
    dc_fprintf(stream, "T %d %d %d %d %d 0\n", mob->raw_str, mob->raw_intel, mob->raw_wis, mob->raw_dex, mob->raw_con);
  }

  if (mob_index[mob->mobdata->nr].programs_)
  {
    write_mprog_recur(stream, mob_index[mob->mobdata->nr].programs_, true);
    dc_fprintf(stream, "|\n");
  }

  if (mob->mobdata->mob_flags.type > 0)
  {
    dc_fprintf(stream, "Y %d\n", mob->mobdata->mob_flags.type);
    for (quint32 i = {}; i < MAX_MOB_VALUES; ++i)
    {
      dc_fprintf(stream, "V %d %d\n", i, mob->mobdata->mob_flags.value[i]);
    }
  }

  dc_fprintf(stream, "S\n");
}

// If a mob is set to 0d0 we need to give it hps depending upon it's level
// and class.  And then since it's a mob, a bonus:)
//
void handle_automatic_mob_damdice(CharacterPtr mob)
{
  qint32 nodice = 1;
  qint32 sizedice = 1;

  // set dependant on level
  if (mob->getLevel() < 5)
  {
    nodice = 1;
    sizedice = 2;
  }
  else if (mob->getLevel() < 10)
  {
    nodice = 1;
    sizedice = 4;
  }
  else if (mob->getLevel() < 15)
  {
    nodice = 2;
    sizedice = 3;
  }
  else if (mob->getLevel() < 20)
  {
    nodice = 2;
    sizedice = 4;
  }
  else if (mob->getLevel() < 25)
  {
    nodice = 3;
    sizedice = 3;
  }
  else if (mob->getLevel() < 30)
  {
    nodice = 3;
    sizedice = 4;
  }
  else if (mob->getLevel() < 35)
  {
    nodice = 4;
    sizedice = 3;
  }
  else if (mob->getLevel() < 40)
  {
    nodice = 4;
    sizedice = 4;
  }
  else if (mob->getLevel() < 45)
  {
    nodice = 5;
    sizedice = 3;
  }
  else if (mob->getLevel() < 50)
  {
    nodice = 5;
    sizedice = 4;
  }
  else if (mob->getLevel() < 55)
  {
    nodice = 5;
    sizedice = 5;
  }
  else if (mob->getLevel() < 60)
  {
    nodice = 6;
    sizedice = 6;
  }
  else if (mob->getLevel() < 65)
  {
    nodice = 7;
    sizedice = 7;
  }
  else if (mob->getLevel() < 70)
  {
    nodice = 8;
    sizedice = 8;
  }
  else
  {
    nodice = 10;
    sizedice = 10;
  }

  // small class adjustment
  switch (GET_CLASS(mob))
  {
  case CLASS_WARRIOR:
  case CLASS_BARBARIAN:
    sizedice++;
    break;
  case CLASS_CLERIC:
  case CLASS_MAGIC_USER:
  case CLASS_DRUID:
    sizedice--;
    break;
  default:
    break;
  }

  mob->mobdata->damnodice = nodice;
  mob->mobdata->damsizedice = sizedice;
}

void handle_automatic_mob_hitpoints(CharacterPtr mob)
{
  quint64 base = {};

  switch (GET_CLASS(mob))
  {
  case CLASS_MAGIC_USER:
    base = 4;
    break;
  case CLASS_CLERIC:
    base = 8;
    break;
  case CLASS_THIEF:
    base = 7;
    break;
  case CLASS_WARRIOR:
    base = 14;
    break;
  case CLASS_ANTI_PAL:
    base = 12;
    break;
  case CLASS_PALADIN:
    base = 12;
    break;
  case CLASS_BARBARIAN:
    base = 17;
    break;
  case CLASS_MONK:
    base = 9;
    break;
  case CLASS_RANGER:
    base = 12;
    break;
  case CLASS_BARD:
    base = 12;
    break;
  case CLASS_DRUID:
    base = 6;
    break;
  case CLASS_PSIONIC:
    base = 6;
    break;
  case CLASS_NECROMANCER:
    base = 4;
    break;
  default:
    base = 14;
    break;
  }

  if (mob->getLevel() > 0)
    base *= mob->getLevel();

  auto old_base = base;
  base *= 1.5;

  mob->raw_hit = base;
  mob->hit = base;
  mob->max_hit = base;
}

// currently hit and dam are the same
void handle_automatic_mob_hitdamroll(CharacterPtr mob)
{
  qint32 curhit;

  curhit = mob->getLevel();

  if (mob->getLevel() > 1 && mob->getLevel() < 11)
    curhit--;

  if (mob->getLevel() > 25)
    curhit++;

  if (mob->getLevel() > 30)
    curhit++;

  if (mob->getLevel() > 35)
    curhit++;

  if (mob->getLevel() > 40)
    curhit++;

  if (mob->getLevel() > 45)
    curhit++;

  if (mob->getLevel() > 49)
    curhit++;

  if (mob->getLevel() > 54)
    curhit += 5;

  mob->hitroll = curhit;
  mob->damroll = curhit;
}

void handle_automatic_mob_settings(CharacterPtr mob)
{
  extern mob_matrix_data mob_matrix[];
  // New matrix is handled here.
  if (ISSET(mob->mobdata->actflags, ACT_NOMATRIX))
    return;
  if (mob->getLevel() > 110)
    return;
  qint32 baselevel = mob->getLevel();
  qreal alevel = (qreal)mob->getLevel();

  qint32 percent = number(-3, 3);

  if (mob->c_class != 0)
    alevel -= mob->getLevel() > 20 ? 3.0 : 2.0;
  bool c = (mob->c_class);
  if (ISSET(mob->mobdata->actflags, ACT_AGGRESSIVE))
    alevel -= 1.5;
  if (!c && ISSET(mob->mobdata->actflags, ACT_AGGR_EVIL))
    alevel -= 0.5;
  if (!c && ISSET(mob->mobdata->actflags, ACT_AGGR_GOOD))
    alevel -= 0.5;
  if (!c && ISSET(mob->mobdata->actflags, ACT_AGGR_NEUT))
    alevel -= 0.5;
  if (ISSET(mob->mobdata->actflags, ACT_RACIST))
    alevel -= 0.5;
  if (ISSET(mob->mobdata->actflags, ACT_FRIENDLY))
    alevel -= 1.0;

  if (!c && ISSET(mob->mobdata->actflags, ACT_3RD_ATTACK))
    alevel -= 0.5;
  if (!c && ISSET(mob->mobdata->actflags, ACT_4TH_ATTACK))
    alevel -= 1.0;
  if (!c && ISSET(mob->mobdata->actflags, ACT_PARRY))
    alevel -= 0.5;
  if (!c && ISSET(mob->mobdata->actflags, ACT_DODGE))
    alevel -= 0.5;
  if (ISSET(mob->mobdata->actflags, ACT_WIMPY))
    alevel -= 1.0;
  if (ISSET(mob->mobdata->actflags, ACT_STUPID))
    alevel += 3.0;
  if (ISSET(mob->mobdata->actflags, ACT_HUGE))
    alevel -= 1.0;
  if (ISSET(mob->mobdata->actflags, ACT_DRAINY))
    alevel -= 2.0;
  if (ISSET(mob->mobdata->actflags, ACT_SWARM))
    alevel -= 1.0;
  if (ISSET(mob->mobdata->actflags, ACT_TINY))
    alevel -= 1.0;

  if (!c && IS_AFFECTED(mob, AFF_SANCTUARY))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_FIRESHIELD))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_LIGHTNINGSHIELD))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_FROSTSHIELD))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_ACID_SHIELD))
    alevel -= 0.5;
  if (IS_AFFECTED(mob, AFF_EAS))
    alevel -= 2.0;
  if (!c && IS_AFFECTED(mob, AFF_HASTE))
    alevel -= 0.5;
  if (IS_AFFECTED(mob, AFF_REFLECT))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_INVISIBLE))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_SNEAK))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_HIDE))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_RAGE))
    alevel -= 1.0;
  if (IS_AFFECTED(mob, AFF_SOLIDITY))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_DETECT_INVISIBLE))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_SENSE_LIFE))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_INFRARED))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_TRUE_SIGHT))
    alevel -= 1.0;
  if (IS_AFFECTED(mob, AFF_BLIND))
    alevel += 3.0;
  if (IS_AFFECTED(mob, AFF_CURSE))
    alevel += 1.0;
  if (IS_AFFECTED(mob, AFF_POISON))
    alevel += 2.0;
  if (IS_AFFECTED(mob, AFF_PARALYSIS))
    alevel += 5.0;
  if (IS_AFFECTED(mob, AFF_SLEEP))
    alevel += 2.0;
  if (!c && IS_AFFECTED(mob, AFF_FOREST_MELD))
    alevel -= 0.5;
  if (!c && IS_AFFECTED(mob, AFF_ALERT))
    alevel -= 1.0;
  if (!c && IS_AFFECTED(mob, AFF_NO_FLEE))
    alevel -= 1.0;
  if (IS_AFFECTED(mob, AFF_POWERWIELD))
    alevel -= 0.5;
  if (IS_AFFECTED(mob, AFF_REGENERATION))
    alevel -= 0.5;
  if (ISSET(mob->mobdata->actflags, ACT_NOKI))
    alevel -= 12.0;
  if (ISSET(mob->mobdata->actflags, ACT_NOHEADBUTT))
    alevel -= 2.0;
  if (ISSET(mob->mobdata->actflags, ACT_NOATTACK))
    alevel += 4.0;
  if (ISSET(mob->mobdata->actflags, ACT_NODISPEL))
    alevel -= 2.0;
  if (IS_AFFECTED(mob, AFF_AMBUSH_ALERT))
    alevel -= 0.5;
  if (IS_AFFECTED(mob, AFF_BLACKJACK_ALERT))
    alevel -= 0.5;
  if (IS_AFFECTED(mob, AFF_FEARLESS))
    alevel -= 0.5;
  if (IS_AFFECTED(mob, AFF_NO_PARA))
    alevel -= 0.5;
  if (IS_AFFECTED(mob, AFF_NO_CIRCLE))
    alevel -= 0.5;
  if (IS_AFFECTED(mob, AFF_NO_BEHEAD))
    alevel -= 0.5;
  if (IS_AFFECTED(mob, AFF_NO_REGEN))
    alevel += 3.0;

  if (mob->getGold() != 0)
    mob->setGold(mob_matrix[baselevel].gold + number(0 - (mob_matrix[baselevel].gold / 10), mob_matrix[baselevel].gold / 10));
  mob->exp = mob_matrix[baselevel].experience + ((mob_matrix[baselevel].experience / 100) * percent);

  mob->alignment = (qint32)((qreal)mob->alignment * (1 + (((qreal)number(0, 30) - 15) / 100)));

  qint32 temp = mob->immune;
  for (; temp; temp <<= 1)
    if (temp & 1)
      alevel -= 0.5;

  temp = mob->suscept;
  for (; temp; temp <<= 1)
    if (temp & 1)
      alevel += 1;

  baselevel = MAX(alevel > 0 ? (qint32)alevel : 1, baselevel - 4);

  mob->hitroll = mob_matrix[baselevel].tohit;
  mob->damroll = mob_matrix[baselevel].todam;
  if (ISSET(mob->mobdata->actflags, ACT_BOSS))
    mob->armor = (qint32)(mob_matrix[baselevel].armor * 1.5);
  else
    mob->armor = mob_matrix[baselevel].armor;
  mob->max_hit = mob->raw_hit = mob->hit = mob_matrix[baselevel].hitpoints + ((mob_matrix[baselevel].hitpoints / 100) * percent);
}

CharacterPtr DC::clone_mobile(qint32 nr)
{
  qint32 i;
  CharacterPtr mob, old;

  if (nr < 0)
    return 0;

  mob = new Character(this);

  clear_char(mob);
  old = ((CharacterPtr)(mob_index[nr].item)); /* cast void pointer */

  *mob = *old;

  mob->mobdata = new Mobile;

  mob->mobdata = old->mobdata;

  for (i = {}; i < MAX_WEAR; i++) /* Initialisering Ok */
    mob->equipment[i] = {};

  mob->mobdata->nr = nr;
  mob->conn_ = {};
  mob->mobdata->reset = {};

  auto &character_list = character_list;
  character_list.insert(mob);
  mob_index[nr].qty++;
  mob->next_in_room = {};

  handle_automatic_mob_settings(mob);
  qreal mult = 1.0;
  if (!ISSET(mob->mobdata->actflags, ACT_NOMATRIX))
  {
    if (mob->getLevel() > 100)
    {
      mult = 1.5;
    }
    else if (mob->getLevel() > 95)
    {
      mult = 1.4;
    }
    else if (mob->getLevel() > 90)
    {
      mult = 1.3;
    }
    else if (mob->getLevel() > 85)
    {
      mult = 1.2;
    }
    else if (mob->getLevel() > 75)
    {
      mult = 1.1;
    }
  }
  mob->max_hit = mob->raw_hit = mob->hit = (qint32)(mob->max_hit * mult);
  mob->mobdata->damnodice = (qint16)(mob->mobdata->damnodice * mult);
  mob->mobdata->damsizedice = (qint16)(mob->mobdata->damsizedice * mult);
  mob->damroll = (qint16)(mob->damroll * mult);
  mob->hometown = old->in_room;
  return (mob);
}

// add a new item to the index.  To do this, we need to update ALL the
// other items in the game after the one being inserted.  Pain in the
// ass but oh well.  it shouldn't hopefully happen that often.
//
// Args:  qint32 nr = virtual number of object (what gods know it as)
//
// return index of item on success, -1 on failure
//
auto DC::create_blank_item(qint32 nr) -> std::expected<qint32, create_error>
{
  ObjectPtr curr;
  qint32 cur_index = {};

  // check if room available in index
  if ((top_of_objt + 1) >= MAX_INDEX)
    return std::unexpected(create_error::index_full);

  // find how where our index will be
  // yes, i could check if the last item is smaller and then do a binary
  // search to do this faster but if everything in life was optimized I wouldn't
  // be playing solitaire at work on a windows machine. -pir
  while (obj_index[cur_index].vnum() < nr && cur_index < top_of_objt + 1)
    cur_index++;

  if (obj_index[cur_index].vnum() == nr) // item already exists
    return std::unexpected(create_error::entry_exists);

  // theoretically if top_of_objt+1 wasn't initialized properly it could
  // be junk data, which could be == nr, returning -1, but i'm not gonna worry about it

  // create

  auto obj = ObjectPtr(new Object(this));
  clear_object(obj);
  obj->name(u"empty obj"_s);
  obj->short_description("An empty obj");
  obj->long_description("An empty obj sits here dejectedly.");
  obj->ActionDescription("Fixed.");
  obj->in_room = DC::NOWHERE;
  obj->next_content = {};
  obj->next_skill = {};
  obj->table = {};
  obj->carried_by = {};
  obj->equipped_by = {};
  obj->in_obj = {};
  obj->contains = {};
  obj->item_number = cur_index;
  obj->ex_description = {};
  // shift > items right
  memmove(&obj_index[cur_index + 1], &obj_index[cur_index], ((top_of_objt - cur_index + 1) * sizeof(obj_index_data)));
  top_of_objt++;

  // insert
  obj_index[cur_index].vnum(nr);
  obj_index[cur_index].qty = {};
  obj_index[cur_index].non_combat_func = {};
  obj_index[cur_index].combat_func = {};
  obj_index[cur_index].item = obj;

  // update index of all items in game
  for (curr = object_list; curr; curr = curr->next)
    if (curr->item_number >= cur_index)
      curr->item_number++;

  // update index of all the obj prototypes
  for (qint32 i = cur_index + 1; i <= top_of_objt; i++)
    (obj_index[i].item)->item_number++;

  // update obj file indices
  world_file_list_item *wcurr = {};

  wcurr = obj_file_list;
  while (wcurr)
  {
    if (wcurr->firstnum >= cur_index)
      wcurr->firstnum++;

    if (wcurr->lastnum >= cur_index - 1)
      wcurr->lastnum++;

    wcurr = wcurr->next;
  }

  rebuild_rnum_references(cur_index, 2);
  return cur_index;
}

// add a new mobile to the index.  To do this, we need to update ALL the
// other mobiles in the game after the one being inserted.  Pain in the
// ass but oh well.  it shouldn't hopefully happen that often.
//
// Args:  qint32 nr = virtual number of object (what gods know it as)
//
// return index of item on success, -1 on failure
//  Hack of create_blank_item.. Uriz
qint32 DC::create_blank_mobile(qint32 nr)
{
  qint32 cur_index = {};

  // check if room available in index
  if ((top_of_mobt + 1) >= MAX_INDEX)
    return -1;

  // find how where our index will be
  // yes, i could check if the last mobile is smaller and then do a binary
  // search to do this faster but if everything in life was optimized I wouldn't
  // be playing solitaire at work on a windows machine. -pir
  while (mob_index[cur_index].vnum() < nr && cur_index < top_of_mobt + 1)
    cur_index++;

  if (mob_index[cur_index].vnum() == nr) // item already exists
    return -1;

  // theoretically if top_of_objt+1 wasn't initialized properly it could
  // be junk data, which could be == nr, returning -1, but i'm not gonna worry

  // create

  auto mob = new Character(this);

  clear_char(mob);
  reset_char(mob);
  mob->name("empty mob");
  mob->short_description("an empty mob");
  mob->long_description("an empty mob description\r\n");
  mob->description("");
  mob->title = {};
  mob->fighting = {};
  mob->player = {};
  mob->altar = {};
  mob->conn_ = {};
  GET_RAW_DEX(mob) = 11;
  GET_RAW_STR(mob) = 11;
  GET_RAW_INT(mob) = 11;
  GET_RAW_WIS(mob) = 11;
  GET_RAW_CON(mob) = 11;
  mob->height = 198;
  mob->weight = 200;
  mob->mobdata = new Mobile;
  qint32 i;
  for (i = {}; i < ACT_MAX / ASIZE + 1; i++)
    mob->mobdata->actflags[i] = {};
  for (i = {}; i < AFF_MAX / ASIZE + 1; i++)
    mob->affected_by[i] = {};
  mob->mobdata->reset = {};
  mob->mobdata->damnodice = 1;
  mob->mobdata->damsizedice = 1;
  mob->mobdata->default_pos = position_t::STANDING;
  mob->mobdata->last_room = {};
  mob->mobdata->nr = cur_index;
  mob->setType(Character::Type::NPC);
  mob->misc = {};

  // shift > items right
  memmove(&mob_index[cur_index + 1], &mob_index[cur_index], ((top_of_mobt - cur_index + 1) * sizeof(mob_index_data)));
  top_of_mobt++;

  // insert
  mob_index[cur_index].vnum(nr);
  mob_index[cur_index].qty = {};
  if (mob_non_combat_functions.contains(nr))
  {
    mob_index[cur_index].non_combat_func = mob_non_combat_functions[nr];
  }
  else
  {
    mob_index[cur_index].non_combat_func = {};
  }

  if (mob_combat_functions.contains(nr))
  {
    mob_index[cur_index].combat_func = mob_combat_functions[nr];
  }
  else
  {
    mob_index[cur_index].combat_func = {};
  }

  mob_index[cur_index].item = mob;

  mob_index[cur_index].programs_ = {};
  mob_index[cur_index].mobspec = {};
  mob_index[cur_index].progtypes = {};

  // update index of all mobiles in game
  const auto &character_list = character_list;
  std::for_each(character_list.begin(), character_list.end(),
                [&cur_index](CharacterPtr const &curr)
                {
                  if (curr->isNonPlayer())
                    if (curr->mobdata->nr >= cur_index)
                      curr->mobdata->nr++;
                });

  // update index of all the mob prototypes
  for (i = cur_index + 1; i <= top_of_mobt; i++)
    ((CharacterPtr)mob_index[i].item)->mobdata->nr++;

  // update obj file indices
  world_file_list_item *wcurr = {};

  wcurr = mob_file_list;
  while (wcurr)
  {
    if (wcurr->firstnum >= cur_index)
      wcurr->firstnum++;

    if (wcurr->lastnum >= cur_index - 1)
      wcurr->lastnum++;

    wcurr = wcurr->next;
  }

  rebuild_rnum_references(cur_index, 1);

  /*
   Shop fixes follow.
   */

  //   qint32 i;
  for (i = {}; i < MAX_SHOP; i++)
  {
    if (shop_index[i].keeper >= cur_index)
      shop_index[i].keeper++;
  }
  return cur_index;
}

// Hack of delete_item_from_index
// Note:  ALL copies of this mobile must have been removed from the game
// before calling this function.  Otherwise these old mobiles will think
// they are the restrung version of the mobile that now holds that index.
//
// Args:  qint32 nr = real number of object (index in array)
//
// return index of mobile on success, -1 on failure
//
void delete_mob_from_index(qint32 nr)
{
  qint32 i = 0, j = {};

  if (nr < 0 || nr > top_of_mobt) // doesn't exist!
    return;

  (CharacterPtr) mob_index[nr].item = {};
  // shift > items left
  memmove(&mob_index[nr], &mob_index[nr + 1], ((top_of_mobt - nr) * sizeof(mob_index_data)));
  top_of_mobt--;

  // update index of all mobiles in game - these store rnums
  const auto &character_list = character_list;
  std::for_each(character_list.begin(), character_list.end(),
                [&nr](CharacterPtr const &curr)
                {
                  if (curr->isNonPlayer() && curr->mobdata->nr >= nr)
                    curr->mobdata->nr--;
                });

  // update index of all the mob prototypes
  for (i = nr; i <= top_of_mobt; i++)
    ((CharacterPtr)mob_index[i].item)->mobdata->nr--;

  // update mob file indices - these store rnums
  world_file_list_item *wcurr = {};

  wcurr = mob_file_list;

  while (wcurr)
  {
    if (wcurr->firstnum > nr)
      wcurr->firstnum--;
    if (wcurr->lastnum >= nr)
      wcurr->lastnum--;
    wcurr = wcurr->next;
  }

  // update zonefile commands - these store rnums
  for (auto [zone_key, zone] : zones.asKeyValueRange())
  {
    for (j = {}; j < zone.cmd.size(); j++)
    {
      switch (zone.cmd[j]->command)
      {
      case 'M': // just #1
        if (zone.cmd[j]->arg1 >= nr)
          zone.cmd[j]->arg1--;
        break;
      default:
        break;
      }
    }
  }
  /*
   Shop fixes follow.
   */
  qint32 z;
  for (z = {}; z < MAX_SHOP; z++)
  {
    if (shop_index[z].keeper >= nr)
      shop_index[z].keeper--;
  }
}

// Delete an item from the index and update everything to continue working
// without causing mass deion and chaos.
//
// Note:  ALL copies of this item must have been removed from the game
// before calling this function.  Otherwise these old items will think
// they are the restrung version of the item that now holds that index.
//
// Args:  qint32 nr = real number of object (index in array)
//
// return index of item on success, -1 on failure
//
void delete_item_from_index(qint32 nr)
{
  qint32 i = 0, j = {};
  ObjectPtr curr;

  if (nr < 0 || nr > top_of_objt) // doesn't exist!
    return;

  obj_index[nr].item = {};

  // shift > items left
  memmove(&obj_index[nr], &obj_index[nr + 1], ((top_of_objt - nr) * sizeof(mob_index_data)));
  top_of_objt--;

  // update index of all items in game - these store rnums
  for (curr = object_list; curr; curr = curr->next)
    if (curr->item_number >= nr)
      curr->item_number--;

  // update index of all the obj prototypes
  for (i = nr; i <= top_of_objt; i++)
    (obj_index[i].item)->item_number--;

  // update obj file indices - these store rnums
  world_file_list_item *wcurr = {};

  wcurr = obj_file_list;

  while (wcurr)
  {
    if (wcurr->firstnum > nr)
      wcurr->firstnum--;

    if (wcurr->lastnum >= nr)
      wcurr->lastnum--;
    wcurr = wcurr->next;
  }

  // update zonefile commands - these store rnums
  for (auto [zone_key, zone] : zones.asKeyValueRange())
  {
    for (j = {}; j < zone.cmd.size(); j++)
    {
      switch (zone.cmd[j]->command)
      {
      case 'P': // 1 and 3
        if (zone.cmd[j]->arg3 >= nr)
          zone.cmd[j]->arg3--;
        // no break here on purpose so it falls through and does the '1'
      case 'O':
      case 'G':
      case 'E': // 1 only
        if (zone.cmd[j]->arg1 >= nr)
          zone.cmd[j]->arg1--;
        break;
      default:
        break;
      }
    }
  }
}

// write an object to file
// This assumes that the object is valid, and the file is open for writing
//
void write_object(LegacyFile &lf, ObjectPtr obj)
{
  FILE *stream = lf.file_handle_;
  extra_descr_data *currdesc;

  dc_fprintf(stream, "#%lu\n", obj_index[obj->item_number].vnum());
  string_to_file(stream, obj->name());
  string_to_file(stream, obj->short_description());
  string_to_file(stream, obj->long_description());
  string_to_file(stream, obj->ActionDescription());

  dc_fprintf(stream, "%d %d %d %d\n"
                     "%d %d %d %d %llu\n"
                     "%d %d %d\n",
             obj->obj_flags.type_flag,
             obj->obj_flags.extra_flags,
             obj->obj_flags.wear_flags.toInt(),
             obj->obj_flags.size,

             obj->obj_flags.value[0],
             obj->obj_flags.value[1],
             obj->obj_flags.value[2],
             obj->obj_flags.value[3],
             obj->obj_flags.eq_level,

             obj->obj_flags.weight,
             obj->obj_flags.cost,
             obj->obj_flags.more_flags);

  currdesc = obj->ex_description;
  while (currdesc)
  {
    dc_fprintf(stream, "E\n");
    string_to_file(stream, currdesc->keyword_);
    string_to_file(stream, currdesc->description_);
    currdesc = currdesc->next;
  }

  for (qint32 i = {}; i < obj->num_affects; i++)
    dc_fprintf(stream, "A\n"
                       "%d %d\n",
               obj->affected[i].location, obj->affected[i].modifier);

  if (obj_index[obj->item_number].programs_)
  {
    write_mprog_recur(stream, obj_index[obj->item_number].programs_, false);
    dc_fprintf(stream, "|\n");
  }

  dc_fprintf(stream, "S\n");
}

auto &operator<<(auto &stream, ObjectPtr obj)
{
  if (!obj)
    return stream;

  stream << "#" << obj->obj_index[obj->item_number].vnum() << "\n";
  string_to_file(stream, obj->name());
  string_to_file(stream, obj->short_description());
  string_to_file(stream, obj->long_description());
  string_to_file(stream, obj->ActionDescription());

  stream << qint32(obj->obj_flags.type_flag) << " "
         << obj->obj_flags.extra_flags << " "
         << obj->obj_flags.wear_flags << " "
         << obj->obj_flags.size << "\n";

  stream << obj->obj_flags.value[0] << " "
         << obj->obj_flags.value[1] << " "
         << obj->obj_flags.value[2] << " "
         << obj->obj_flags.value[3] << " "
         << obj->obj_flags.eq_level << "\n";

  stream << obj->obj_flags.weight << " "
         << obj->obj_flags.cost << " "
         << obj->obj_flags.more_flags << "\n";

  extra_descr_data *currdesc = obj->ex_description;
  while (currdesc)
  {
    stream << "E\n";
    string_to_file(stream, currdesc->keyword_);
    string_to_file(stream, currdesc->description_);
    currdesc = currdesc->next;
  }

  for (qint32 i = {}; i < obj->num_affects; i++)
  {
    stream << "A\n";
    stream << obj->affected[i].location << " "
           << obj->affected[i].modifier << "\n";
  }

  if (obj->obj_index[obj->item_number].programs_)
  {
    write_mprog_recur(stream, obj->obj_index[obj->item_number].programs_, false);
    stream << "|\n";
  }

  stream << "S\n";

  return stream;
}

QString quotequotes(QString &s1)
{
  size_t pos = s1.find("\"");
  while (pos != QString::npos)
  {
    s1.insert(pos, 1, '\"');
    pos = s1.find('\"', pos + 2);
  }

  return s1;
}

QString quotequotes(QString s1)
{
  auto pos = s1.indexOf('\"');
  while (pos != -1)
  {
    s1.insert(pos, '\"');
    pos = s1.indexOf('\"', pos + 2);
  }

  return s1;
}

QString quotequotes(const QString str)
{
  QString s1(str);

  return quotequotes(s1);
}

QString lf_to_crlf(QString s1)
{
  qsizetype pos = s1.indexOf('\n'); // @suppress("Ambiguous problem")
  while (pos != -1)
  {
    s1.insert(pos, '\r');
    pos = s1.indexOf('\n', pos + 2);
  }

  return s1;
}

QString lf_to_crlf(QString &s1)
{
  size_t pos = s1.find('\n'); // @suppress("Ambiguous problem")
  while (pos != QString::npos)
  {
    s1.insert(pos, 1, '\r');
    pos = s1.find('\n', pos + 2);
  }

  return s1;
}

void write_bitvector_csv(quint32 vector, QStringList names, std::ofstream &fout)
{
  for (quint32 nr = {}; nr < names.size(); nr++)
  {
    if (isSet(1, vector))
    {
      fout << names[nr].toStdString();
    }

    fout << ",";
    vector >>= 1;
  }
}

void write_bitvector_csv(quint32 vector, const QStringList array, std::ofstream &fout)
{
  qint32 nr = {};
  while (*array[nr] != '\n')
  {
    if (isSet(1, vector))
    {
      fout << array[nr];
    }

    fout << ",";
    vector >>= 1;
    nr++;
  }
}

void write_object_csv(ObjectPtr obj, std::ofstream &fout)
{
  try
  {
    fout << obj_index[obj->item_number].vnum() << ",";
    fout << "\"" << obj->name().toStdString() << "\",";
    fout << "\"" << quotequotes(obj->short_description()) << "\",";
    fout << "\"" << quotequotes(obj->long_description()) << "\",";
    fout << "\"" << quotequotes(obj->ActionDescription()) << "\",";
    fout << item_types[obj->obj_flags.type_flag].toStdString() << ",";
    fout << obj->obj_flags.size << ",";
    fout << obj->obj_flags.value[0] << ",";
    fout << obj->obj_flags.value[1] << ",";
    fout << obj->obj_flags.value[2] << ",";
    fout << obj->obj_flags.value[3] << ",";
    fout << obj->obj_flags.eq_level << ",";
    fout << obj->obj_flags.weight << ",";
    fout << obj->obj_flags.cost << ",";

    write_bitvector_csv(obj->obj_flags.wear_flags, QFlagsToStrings<ObjectPositions>(), fout);
    write_bitvector_csv(obj->obj_flags.extra_flags, Object::extra_bits, fout);
    write_bitvector_csv(obj->obj_flags.more_flags, Object::more_obj_bits, fout);

    QString buf, buf2;
    for (qint32 i = {}; i < obj->num_affects; i++)
    {
      if (obj->affected[i].location < 1000)
        sprinttype(obj->affected[i].location, apply_types, buf2);
      else if (!get_skill_name(obj->affected[i].location / 1000).isEmpty())
        dc_strncpy(buf2, get_skill_name(obj->affected[i].location / 1000).toStdString().c_str(), sizeof(buf2));
      else
        dc_strcpy(buf2, "Invalid");

      dc_sprintf(buf, "%s by %d", qPrintable(buf2), obj->affected[i].modifier);
      fout << buf;

      if (i + 1 < obj->num_affects)
      {
        fout << " ";
      }
    }
  }
  catch (std::ofstream::failure &e)
  {
    std::stringstream errormsg;
    errormsg << "Exception while writing in write_obj_csv.";
    logentry(errormsg.str().c_str(), 108, DC::LogChannel::LOG_MISC);
  }

  fout << std::endl;
}

bool has_random(ObjectPtr obj)
{
  return ((obj_index[obj->item_number].progtypes & RAND_PROG) || (obj_index[obj->item_number].progtypes & ARAND_PROG));
}

/* clone an object from obj_index */
ObjectPtr clone_object(qint32 nr)
{
  ObjectPtr obj, old;
  extra_descr_data *new_new_descr, *descr;

  if (nr < 0)
    return 0;

  obj = new Object;
  clear_object(obj);
  old = (obj_index[nr].item); /* cast the void pointer */

  if (old != 0)
  {
    *obj = *old;
  }
  else
  {
    qWarning("%s", qUtf8Printable(u"clone_object(%1): Obj not found in obj_index.\n"_s.arg(nr)));
    obj = {};
    return {};
  }

  /* *** extra descriptions *** */
  obj->ex_description = {};
  for (descr = old->ex_description; descr; descr = descr->next)
  {
    new_new_descr = new extra_descr_data;
    new_new_descr->keyword_ = descr->keyword_;
    new_new_descr->description_ = descr->description_;
    new_new_descr->next = obj->ex_description;
    obj->ex_description = new_new_descr;
  }

  obj->affected.resize(obj->num_affects);
  for (qint32 i = {}; i < obj->num_affects; i++)
  {
    obj->affected[i].location = old->affected[i].location;
    obj->affected[i].modifier = old->affected[i].modifier;
  }
  obj->table = {};
  obj->next_skill = {};
  obj->next_content = {};
  obj->next = object_list;
  object_list = obj;
  obj_index[nr].qty++;
  obj->save_expiration = {};
  obj->no_sell_expiration = {};

  if (obj_index[obj->item_number].non_combat_func ||
      obj->obj_flags.type_flag == ITEM_MEGAPHONE ||
      has_random(obj))
  {
    active_obj_list.insert(obj);
  }
  return obj;
}

void randomize_object_affects(ObjectPtr obj)
{
  if (obj == nullptr)
  {
    return;
  }

  // Don't alter godload
  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
  {
    return;
  }

  for (qint32 i = {}; i < obj->num_affects; i++)
  {
    switch (obj->affected[i].location)
    {
    case APPLY_STR:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_CON:
      if (ch->number(0, 1) == 1)
      {
        obj->affected[i].modifier += number(-1, 1);
        obj->affected[i].modifier = MAX(0, obj->affected[i].modifier);
      }
      break;
    // Spells found on weapons from weapon_spells()
    case WEP_MAGIC_MISSILE:
    case WEP_BLIND:
    case WEP_EARTHQUAKE:
    case WEP_CURSE:
    case WEP_COLOUR_SPRAY:
    case WEP_DISPEL_EVIL:
    case WEP_ENERGY_DRAIN:
    case WEP_FIREBALL:
    case WEP_LIGHTNING_BOLT:
    case WEP_HARM:
    case WEP_POISON:
    case WEP_SLEEP:
    case WEP_FEAR:
    case WEP_DISPEL_MAGIC:
    case WEP_WEAKEN:
    case WEP_CAUSE_LIGHT:
    case WEP_CAUSE_CRITICAL:
    case WEP_PARALYZE:
    case WEP_ACID_BLAST:
    case WEP_BEE_STING:
    case WEP_CURE_LIGHT:
    case WEP_FLAMESTRIKE:
    case WEP_HEAL_SPRAY:
    case WEP_DROWN:
    case WEP_HOWL:
    case WEP_SOULDRAIN:
    case WEP_SPARKS:
    case WEP_DISPEL_GOOD:
    case WEP_TELEPORT:
    case WEP_CHILL_TOUCH:
    case WEP_POWER_HARM:
    case WEP_VAMPIRIC_TOUCH:
    case WEP_LIFE_LEECH:
    case WEP_METEOR_SWARM:
    case WEP_ENTANGLE:
    case WEP_CREATE_FOOD:
    case WEP_WILD_MAGIC:
    case APPLY_LIGHTNING_SHIELD:
    case APPLY_HIT:
    case APPLY_MOVE:
    case APPLY_MANA:
    case APPLY_KI:
    case APPLY_HIT_N_DAM:
    case APPLY_HITROLL:
    case APPLY_DAMROLL:
    case APPLY_SPELLDAMAGE:
    case APPLY_MELEE_DAMAGE: // melee mitigation
    case APPLY_SPELL_DAMAGE: // spell mitigation
    case APPLY_SONG_DAMAGE:  // song mitigation
    case APPLY_HP_REGEN:
    case APPLY_MANA_REGEN:
    case APPLY_MOVE_REGEN:
    case APPLY_KI_REGEN:
    case APPLY_SAVING_FIRE:
    case APPLY_SAVING_COLD:
    case APPLY_SAVING_ENERGY:
    case APPLY_SAVING_ACID:
    case APPLY_SAVING_MAGIC:
    case APPLY_SAVING_POISON:
    case APPLY_SAVES:
    case APPLY_AC:
    case APPLY_REFLECT:
      obj->affected[i].modifier = random_percent_change(33, obj->affected[i].modifier);
      break;
    }
  }
}

void randomize_object(ObjectPtr obj)
{
  if (obj == nullptr)
  {
    return;
  }

  // NO_CUSTOM, QUEST or SPECIAL ("godload") items cannot be randomized
  if (obj->isCustom() || obj->isQuest() || obj->isGodload())
  {
    return;
  }

  SET_BIT(obj->obj_flags.more_flags, ITEM_CUSTOM);

  switch (obj->obj_flags.type_flag)
  {
  case ITEM_WEAPON:
    obj->obj_flags.cost = MAX(1, random_percent_change(33, obj->obj_flags.cost));
    obj->obj_flags.value[1] = random_percent_change(20, obj->obj_flags.value[1]);
    obj->obj_flags.value[2] = random_percent_change(20, obj->obj_flags.value[2]);
    randomize_object_affects(obj);
    break;
  case ITEM_ARMOR:
    obj->obj_flags.cost = MAX(1, random_percent_change(33, obj->obj_flags.cost));
    // v1 AC-apply
    obj->obj_flags.value[0] = random_percent_change(25, obj->obj_flags.value[0]);
    randomize_object_affects(obj);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    obj->obj_flags.cost = MAX(1, random_percent_change(33, obj->obj_flags.cost));
    // v2 total charges
    obj->obj_flags.value[1] = random_percent_change(10, obj->obj_flags.value[2]);
    // v3 current charges
    obj->obj_flags.value[2] = obj->obj_flags.value[1];
    break;
  case ITEM_INSTRUMENT:
    obj->obj_flags.cost = MAX(1, random_percent_change(33, obj->obj_flags.cost));
    // v2 non-combat
    obj->obj_flags.value[1] = random_percent_change(33, obj->obj_flags.value[1]);
    // v3 combat
    obj->obj_flags.value[2] = random_percent_change(33, obj->obj_flags.value[2]);
    randomize_object_affects(obj);
    break;
  case ITEM_CONTAINER:
    obj->obj_flags.cost = MAX(1, random_percent_change(33, obj->obj_flags.cost));
    randomize_object_affects(obj);
    break;
  case ITEM_POTION:
    obj->obj_flags.cost = MAX(1, random_percent_change(33, obj->obj_flags.cost));
    // v1 level of potion
    obj->obj_flags.value[0] = random_percent_change(10, obj->obj_flags.value[0]);
    break;
  }
}

void DC::zone_update(void)
{
  auto &zones = zones;
  for (auto [zone_key, zone] : zones.asKeyValueRange())
  {
    if (zone.reset_mode == 0)
    {
      continue;
    }

    if (zone.age < zone.lifespan && !(zone.reset_mode == 1 && !zone.isEmpty()))
    {
      zone.age++;
      continue;
    }

    if (zone.reset_mode == 1 && !zone.isEmpty())
    {
      continue;
    }

    if (QDateTime::currentDateTimeUtc() > zone.last_full_reset.addDays(1))
    {
      zone.reset(Zone::ResetType::full);
    }
    else
    {
      zone.reset();
    }

    // update first repop numbers
    if (zone.num_mob_first_repop == 0)
    {
      zone.num_mob_first_repop = zone.num_mob_on_repop;
    }
  }

  removeDead();
}

quint64 countMobsInRoom(quint64 vnum, room_t room_id)
{
  quint64 count = {};
  for (auto ch = world[room_id].people_; ch != nullptr; ch = ch->next_in_room)
  {
    if (ch->mobdata && mob_index[ch->mobdata->nr].vnum() == vnum)
    {
      count++;
    }
  }
  return count;
}

quint64 countMobsInWorld(quint64 vnum)
{
  quint64 count = {};
  for (const auto ch : character_list)
  {
    if (ch->mobdata && mob_index[ch->mobdata->nr].vnum() == vnum && ch->in_room != DC::NOWHERE)
    {
      count++;
    }
  }
  return count;
}

/* execute the reset command table of a given zone */
void Zone::reset(ResetType reset_type)
{
  if (reset_type == ResetType::full)
  {
    last_full_reset = QDateTime::currentDateTimeUtc();
  }

  qint32 reset_cmd_index, last_cmd, last_mob, last_obj, last_percent;
  CharacterPtr mob = {};
  ObjectPtr obj, obj_to;
  last_cmd = last_mob = last_obj = last_percent = -1;

  QString buf;
  QString log_buf = {};

  if (died_this_tick == 0 && isEmpty())
  {
    repops_without_deaths++;
  }
  else
  {
    repops_without_deaths = {};
  }

  // reset number of mobs that have died this tick to 0
  died_this_tick = {};
  num_mob_on_repop = {};
  // find last command in zone

  for (reset_cmd_index = {}; reset_cmd_index < cmd.size(); reset_cmd_index++)
  {
    if (reset_cmd_index < 0 || reset_cmd_index > cmd.size())
    {
      dc_sprintf(buf, "Trapped zone error, Command is null, zone: %lu reset_cmd_index: %d",
                 id_, reset_cmd_index);
      logentry(buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
      break;
    }
    if (cmd[reset_cmd_index]->active == 0)
      continue;

    cmd[reset_cmd_index]->last = time(nullptr);
    cmd[reset_cmd_index]->attempts++;
    if (cmd[reset_cmd_index]->if_flag == 0 ||                                    // always
        (last_cmd == 1 && cmd[reset_cmd_index]->if_flag == 1) ||                 // if last command true
        (last_cmd == 0 && cmd[reset_cmd_index]->if_flag == 2) ||                 // if last command false
        (reset_type == ResetType::full && cmd[reset_cmd_index]->if_flag == 3) || // full reset (onboot)
        (last_mob == 1 && cmd[reset_cmd_index]->if_flag == 4) ||                 // if-last-mob-true
        (last_mob == 0 && cmd[reset_cmd_index]->if_flag == 5) ||                 // if-last-mob-false
        (last_obj == 1 && cmd[reset_cmd_index]->if_flag == 6) ||                 // if-last-obj-true
        (last_obj == 0 && cmd[reset_cmd_index]->if_flag == 7) ||                 // if-last-obj-false
        (last_percent == 1 && cmd[reset_cmd_index]->if_flag == 8) ||             // if-last-percent-true
        (last_percent == 0 && cmd[reset_cmd_index]->if_flag == 9)                // if-last-percent-false
    )
    {
      cmd[reset_cmd_index]->lastSuccess = cmd[reset_cmd_index]->last;
      cmd[reset_cmd_index]->successes++;
      switch (cmd[reset_cmd_index]->command)
      {

      case 'M': /* read a mobile */
        if ((cmd[reset_cmd_index]->arg2 == -1 || cmd[reset_cmd_index]->lastPop == 0) && countMobsInWorld(mob_index[cmd[reset_cmd_index]->arg1].vnum()) < cmd[reset_cmd_index]->arg2 && (mob = clone_mobile(cmd[reset_cmd_index]->arg1)))
        {
          char_to_room(mob, cmd[reset_cmd_index]->arg3);
          cmd[reset_cmd_index]->lastPop = mob;
          mob->mobdata->reset = cmd[reset_cmd_index];
          mob->hometown = world[cmd[reset_cmd_index]->arg3].number;
          num_mob_on_repop++;
          last_cmd = 1;
          last_mob = 1;
          selfpurge = false;
          mprog_load_trigger(mob);
          if (selfpurge)
          {
            mob = {};
            last_mob = {};
            last_cmd = {};
          }
        }
        else
        {
          last_cmd = {};
          last_mob = {};
        }
        break;

      case 'O': /* Load object on the ground */
        if (cmd[reset_cmd_index]->arg2 == -1 || obj_index[cmd[reset_cmd_index]->arg1].qty < cmd[reset_cmd_index]->arg2)
        {
          if (cmd[reset_cmd_index]->arg3 >= 0)
          {
            if (!get_obj_in_list_num(cmd[reset_cmd_index]->arg1,
                                     world[cmd[reset_cmd_index]->arg3].contents) &&
                (obj =
                     clone_object(cmd[reset_cmd_index]->arg1)))
            {
              obj_to_room(obj, cmd[reset_cmd_index]->arg3);
              last_cmd = 1;
              last_obj = 1;
            }
            else
            {
              last_cmd = {};
              last_obj = {};
            }
          }
          else
          {
            DC::config &cf = cf;

            if (cf.test_world == false && cf.test_mobs == false && cf.test_objs == false)
            {
              dc_sprintf(buf, "Obj %lu loaded to DC::NOWHERE. Zone %lu Cmd %d", obj_index[cmd[reset_cmd_index]->arg1].vnum(), id_, reset_cmd_index);
              logentry(buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
            }
            last_cmd = {};
            last_obj = {};
          }
        }
        else
        {
          last_cmd = {};
          last_obj = {};
        }
        break;

      case 'P': /* object to object */

        if (cmd[reset_cmd_index]->arg2 == -1 || obj_index[cmd[reset_cmd_index]->arg1].qty < cmd[reset_cmd_index]->arg2)
        {
          obj_to = {};
          obj = {};
          if ((obj_to = get_obj_num(cmd[reset_cmd_index]->arg3)) && (obj =
                                                                         clone_object(cmd[reset_cmd_index]->arg1)))
            obj_to_obj(obj, obj_to);
          else
            logf(
                IMMORTAL,
                DC::LogChannel::LOG_WORLD,
                "Null container obj in P command Zone: %d, Cmd: %d",
                id_, reset_cmd_index);

          last_cmd = 1;
          last_obj = 1;
        }
        else
        {
          last_cmd = {};
          last_obj = {};
        }
        break;

      case 'G': /* object to character */
        if (mob == nullptr)
        {
          // dc_sprintf(buf, "Null mob in G, reseting zone %d cmd %d", id_, reset_cmd_index + 1);
          // logentry(buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
          last_cmd = {};
          last_obj = {};
          break;
        }
        // Never load the same totem as long as it exists in the world
        if (reinterpret_cast<ObjectPtr>(obj_index[cmd[reset_cmd_index]->arg1].item)->isTotem() &&
            cmd[reset_cmd_index]->arg2 != -1 &&
            obj_index[cmd[reset_cmd_index]->arg1].qty >= cmd[reset_cmd_index]->arg2)
        {
          last_cmd = {};
          last_obj = {};
          break;
        }
        if ((cmd[reset_cmd_index]->arg2 == -1 || obj_index[cmd[reset_cmd_index]->arg1].qty < cmd[reset_cmd_index]->arg2 || number(0, 1)) && (obj = clone_object(cmd[reset_cmd_index]->arg1)))
        {
          obj_to_char(obj, mob);
          last_cmd = 1;
          last_obj = {};
        }
        else
        {
          last_cmd = {};
          last_obj = {};
        }
        break;

      case '%': /* percent chance of next command happening */
        // We can't send a number less than one to number() otherwise a debug coredump occurs
        if (cmd[reset_cmd_index]->arg2 < 1)
        {
          logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Zone %d, line %d: % arg1: %d arg2: %d - Error: arg2 < 1", id_, reset_cmd_index, cmd[reset_cmd_index]->arg1, cmd[reset_cmd_index]->arg2);
          last_cmd = {};
          last_percent = {};
        }
        else
        {
          if (ch->number(1, cmd[reset_cmd_index]->arg2) <= cmd[reset_cmd_index]->arg1)
          {
            cmd[reset_cmd_index]->last = time(nullptr);
            last_percent = 1;
            last_cmd = 1;
          }
          else
          {
            last_cmd = {};
            last_percent = {};
          }
        }
        break;

      case 'E': /* object to equipment list */
        if (mob == nullptr)
        {
          // dc_sprintf(buf, "Null mob in E reseting zone %d cmd %d", id_, reset_cmd_index + 1);
          // logentry(buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
          last_cmd = {};
          last_obj = {};
          break;
        }
        // Never load the same totem as long as it exists in the world
        if (reinterpret_cast<ObjectPtr>(obj_index[cmd[reset_cmd_index]->arg1].item)->isTotem() &&
            cmd[reset_cmd_index]->arg2 != -1 &&
            obj_index[cmd[reset_cmd_index]->arg1].qty >= cmd[reset_cmd_index]->arg2)
        {
          last_cmd = {};
          last_obj = {};
          break;
        }
        if ((obj = clone_object(cmd[reset_cmd_index]->arg1)))
        {
          randomize_object(obj);

          // Check if mob and object are safe to be equipped
          // Some of these checks are redundant compared to equip_char() but
          // we want to know if the position is filled already before running equip_char()
          // so we don't see unnecessary errors when a zone reset tries to reequip a mob
          if (mob == nullptr)
          {
            logentry(u"nullptr mob in reset_zone()!"_s, ANGEL, DC::LogChannel::LOG_BUG);
          }
          else if (cmd[reset_cmd_index]->arg3 < 0 || cmd[reset_cmd_index]->arg3 >= MAX_WEAR)
          {
            logentry(u"Invalid eq position in Zone::reset()!"_s, ANGEL, DC::LogChannel::LOG_BUG);
          }
          else if (mob->equipment[cmd[reset_cmd_index]->arg3] == 0)
          {
            if (!mob->equip_char(obj, cmd[reset_cmd_index]->arg3))
            {
              dc_sprintf(buf, "Bad equip_char zone %lu cmd %d", id_, reset_cmd_index + 1);
              logentry(buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
            }
          }

          if (ISSET(mob->mobdata->actflags, ACT_BOSS))
          {
            mob->max_hit *= (1 + obj->obj_flags.eq_level / 500);
            mob->damroll *= (1 + obj->obj_flags.eq_level / 500);
            mob->hitroll *= (1 + obj->obj_flags.eq_level / 500);
            if (mob->armor > 0)
              mob->armor *= (1 - obj->obj_flags.eq_level / 500);
            else
              mob->armor *= (1 + obj->obj_flags.eq_level / 500);
          }
          last_obj = 1;
          last_cmd = 1;
        }
        else
        {
          last_cmd = {};
          last_obj = {};
        }
        break;

      case 'D': /* set state of door */
        if (cmd[reset_cmd_index]->arg1 < 0 || cmd[reset_cmd_index]->arg1 > top_of_world)
        {
          dc_sprintf(log_buf, "Illegal room number Z: %lu cmd %d", id_, reset_cmd_index + 1);
          logentry(log_buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
          break;
        }
        if (cmd[reset_cmd_index]->arg2 < 0 || cmd[reset_cmd_index]->arg2 >= 6)
        {
          dc_sprintf(log_buf, "Illegal direction %d doesn't exist Z: %lu cmd %d", cmd[reset_cmd_index]->arg2, id_, reset_cmd_index + 1);
          logentry(log_buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
          break;
        }
        if (!rooms.contains(cmd[reset_cmd_index]->arg1))
        {
          dc_sprintf(log_buf, "Room %d doesn't exist Z: %lu cmd %d", cmd[reset_cmd_index]->arg1, id_, reset_cmd_index + 1);
          logentry(log_buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
          break;
        }

        if (world[cmd[reset_cmd_index]->arg1].dir_option[cmd[reset_cmd_index]->arg2] == 0)
        {
          dc_sprintf(
              log_buf,
              "Attempt to reset direction %d on room %d that doesn't exist Z: %lu cmd %d",
              cmd[reset_cmd_index]->arg2, world[cmd[reset_cmd_index]->arg1].number, id_, reset_cmd_index);
          logentry(log_buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
          break;
        }
        switch (cmd[reset_cmd_index]->arg3)
        {
        case 0:
          REMOVE_BIT(
              world[cmd[reset_cmd_index]->arg1].dir_option[cmd[reset_cmd_index]->arg2]->exit_info,
              EX_BROKEN);
          REMOVE_BIT(
              world[cmd[reset_cmd_index]->arg1].dir_option[cmd[reset_cmd_index]->arg2]->exit_info,
              EX_LOCKED);
          REMOVE_BIT(
              world[cmd[reset_cmd_index]->arg1].dir_option[cmd[reset_cmd_index]->arg2]->exit_info,
              EX_CLOSED);
          break;
        case 1:
          REMOVE_BIT(
              world[cmd[reset_cmd_index]->arg1].dir_option[cmd[reset_cmd_index]->arg2]->exit_info,
              EX_BROKEN);
          SET_BIT(world[cmd[reset_cmd_index]->arg1].dir_option[cmd[reset_cmd_index]->arg2]->exit_info,
                  EX_CLOSED);
          REMOVE_BIT(
              world[cmd[reset_cmd_index]->arg1].dir_option[cmd[reset_cmd_index]->arg2]->exit_info,
              EX_LOCKED);
          break;
        case 2:
          REMOVE_BIT(
              world[cmd[reset_cmd_index]->arg1].dir_option[cmd[reset_cmd_index]->arg2]->exit_info,
              EX_BROKEN);
          SET_BIT(world[cmd[reset_cmd_index]->arg1].dir_option[cmd[reset_cmd_index]->arg2]->exit_info,
                  EX_LOCKED);
          SET_BIT(world[cmd[reset_cmd_index]->arg1].dir_option[cmd[reset_cmd_index]->arg2]->exit_info,
                  EX_CLOSED);
          break;
        }
        last_cmd = 1;
        break;

      case 'X':
        switch (cmd[reset_cmd_index]->arg1)
        {
        case 0:
          last_cmd = -1;
          last_mob = -1;
          last_obj = -1;
          last_percent = -1;
          break;

        case 1:
          last_mob = -1;
          break;

        case 2:
          last_obj = -1;
          break;

        case 3:
          last_percent = -1;
          break;

        default:
          last_cmd = -1;
          last_mob = -1;
          last_obj = -1;
          last_percent = -1;
          break;
        }
        break;

      case 'K': // skip
        reset_cmd_index += cmd[reset_cmd_index]->arg1;
        break;

      case '*': // ignore *
      case 'J': // ignore J
        break;

      default:
        dc_sprintf(log_buf, "UNKNOWN COMMAND!!! ZONE %lu cmd %d: '%c' Skipping .", id_, reset_cmd_index + 1, cmd[reset_cmd_index]->command);
        logentry(log_buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
        age = {};
        return;
        break;
      }
    }
    else
    {
      switch (cmd[reset_cmd_index]->command)
      {

      case 'M':
        last_mob = {};
        last_cmd = {};
        break;

      case 'O':
      case 'G':
      case 'P':
      case 'E':
        last_obj = {};
        last_cmd = {};
        break;
      case '%':
        last_percent = {};
        last_cmd = {};
        break;
      case 'D':
        last_cmd = {};
        break;
      case 'X':
      case 'K':
        last_cmd = {};
        break;
      default:
        break;
      }
    }
  }

  age = {};

  if (repops_without_deaths > 2 && repops_without_deaths < 7 && repops_with_bonus < 4)
  {
    repops_with_bonus++;

    const auto &character_list = character_list;
    for (const auto &tmp_victim : character_list)
    {
      if (tmp_victim->in_room == DC::NOWHERE)
      {
        continue;
      }
      if (tmp_victim->isNonPlayer() && !ISSET(tmp_victim->mobdata->actflags, ACT_NO_GOLD_BONUS) && world[tmp_victim->in_room].zone == id_)
      {
        tmp_victim->multiplyGold(1.10);
        tmp_victim->exp *= 1.10;
      }
    }
  }
}

bool Zone::isEmpty(void)
{
  ConnectionPtr i;

  for (auto &i : connections_)
    if (i->connected == Connection::states::PLAYING && i->character && world[i->character->in_room].zone == id_)
      return false;

  return true;
}

/* release memory allocated for a character  */
void free_char(CharacterPtr ch)
{
  qint32 iWear;
  //  affected_type *af;
  class char_player_alias *x;
  char_player_alias *next;
  mob_prog_act_list *currmprog;
  auto &character_list = character_list;

  character_list.erase(ch);

  if (ch->tempVariable)
  {
    tempvariable *temp, *tmp;
    for (temp = ch->tempVariable; temp; temp = tmp)
    {
      tmp = temp->next;
      temp = {};
    }
  }
  SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop falling off

  for (iWear = {}; iWear < MAX_WEAR; iWear++)
  {
    if (ch->equipment[iWear])
      obj_to_char(ch->unequip_char(iWear, true), ch);
  }
  while (ch->carrying)
    extract_obj(ch->carrying);

  if (ch->isPlayer())
  {
    if (ch->player)
    {
      // these won't be here if you free an unloaded character
      ch->player->skillchange = {};
      if (!ch->player->ignoring.isEmpty())
        ch->player->ignoring.clear();
      if (ch->player->golem)
        logentry(u"Error, golem not released properly"_s), ANGEL, DC::LogChannel::LOG_BUG);
      /* Free aliases... (I was to lazy to do before. ;) */
      ch->player->away_msgs.clear();

      if (ch->player->lastseen)
        ch->player->lastseen = {};

      if (ch->player->config)
      {
        ch->player->config = {};
      }

      if (ch->player)
      {
        ch->player = {};
      }
    }
  }
  else if (ch->mobdata != nullptr)
  {
    remove_memory(ch, 'f');
    remove_memory(ch, 'h');
    while (ch->mobdata->mpact)
    {
      currmprog = ch->mobdata->mpact->next;
      if (ch->mobdata->mpact->buf)
        ch->mobdata->mpact->buf = {};
      ch->mobdata->mpact = {};
      ch->mobdata->mpact = currmprog;
    }
    ch->mobdata = {};
  }
  else
  {
    // logf(IMMORTAL, DC::LogChannel::LOG_BUG, u"free_char: '%1' is not PC or NPC"_s.arg(qPrintable(qPrintable(ch->name()))));
  }

  ch->title = {};

  remove_memory(ch, 't');

  // Since affect_remove updates the linked list itself, do it this way
  while (ch->affected)
    affect_remove(ch, ch->affected, SUPPRESS_ALL);

  ch = {};
}

/* release memory allocated for an obj  */
void free_obj(ObjectPtr obj)
{
  extra_descr_data *ths, *next_one;

  for (ths = obj->ex_description; ths; ths = next_one)
  {
    next_one = ths->next;
    ths = {};
  }

  obj = {};
}

/* read contents of a text file, and place in buf */
qint32 file_to_string(const QString name, QString buf)
{
  FILE *stream;
  QString tmp;

  if (!(stream = fopen(name, "r")))
  {
    logverbose(u"Unable to open '%1':%2"_s.arg(name).arg(strerror(errno)));

    return (-1);
  }

  do
  {
    fgets(tmp, 99, stream);

    if (!feof(stream))
    {
      if (dc_strlen(buf) + dc_strlen(tmp) + 2 > MAX_STRING_LENGTH)
      {
        logentry(u"stream->strng: QString too big (db.c, file_to_string)"_s,
                 0, DC::LogChannel::LOG_BUG);

        return (-1);
      }

      dc_strcat(buf, tmp);
      *(buf + dc_strlen(buf) + 1) = '\0';
      *(buf + dc_strlen(buf)) = '\r';
    }
  } while (!feof(stream));

  return {};
}

/* clear some of the the working variables of a chararacter */
void reset_char(CharacterPtr ch)
{
  qint32 i;

  ch->hometown = START_ROOM;

  for (i = {}; i < MAX_WEAR; i++) /* Intializing  */
    ch->equipment[i] = {};

  ch->followers = {};
  ch->master = {};

  ch->spelldamage = {};
  ch->carrying = {};
  ch->carry_weight = {};
  ch->carry_items = {};
  ch->next = {};
  ch->next_fighting = {};
  ch->next_in_room = {};
  ch->fighting = {};
  ch->setStanding();
  ch->carry_weight = {};
  ch->carry_items = {};

  switch (GET_CLASS(ch))
  {
  case CLASS_MAGE:
    GET_AC(ch) = 200;
    break;
  case CLASS_DRUID:
    GET_AC(ch) = 185;
    break;
  case CLASS_CLERIC:
    GET_AC(ch) = 170;
    break;
  case CLASS_ANTI_PAL:
    GET_AC(ch) = 155;
    break;
  case CLASS_THIEF:
    GET_AC(ch) = 140;
    break;
  case CLASS_BARD:
    GET_AC(ch) = 125;
    break;
  case CLASS_BARBARIAN:
    GET_AC(ch) = 110;
    break;
  case CLASS_RANGER:
    GET_AC(ch) = 95;
    break;
  case CLASS_PALADIN:
    GET_AC(ch) = 80;
    break;
  case CLASS_WARRIOR:
    GET_AC(ch) = 65;
    break;
  case CLASS_MONK:
    GET_AC(ch) = 50;
    break;
  default:
    GET_AC(ch) = 100;
    break;
  }

  if (IS_SINGING(ch))
    ch->songs.clear();

  if (ch->getHP() < 1)
    ch->setHP(1);
  if (GET_MOVE(ch) < 1)
    ch->setMove(1);
  if (GET_MANA(ch) < 1)
    GET_MANA(ch) = 1;

  ch->misc = {};

  SET_BIT(ch->misc, DC::LogChannel::LOG_BUG);
  SET_BIT(ch->misc, DC::LogChannel::LOG_PRAYER);
  SET_BIT(ch->misc, DC::LogChannel::LOG_GOD);
  SET_BIT(ch->misc, DC::LogChannel::LOG_MORTAL);
  SET_BIT(ch->misc, DC::LogChannel::LOG_SOCKET);
  SET_BIT(ch->misc, DC::LogChannel::LOG_MISC);
  SET_BIT(ch->misc, DC::LogChannel::LOG_PLAYER);
  //  SET_BIT(ch->misc, DC::LogChannel::CHANNEL_GOSSIP);
  SET_BIT(ch->misc, DC::LogChannel::CHANNEL_DREAM);
  SET_BIT(ch->misc, DC::LogChannel::CHANNEL_SHOUT);
  SET_BIT(ch->misc, DC::LogChannel::CHANNEL_AUCTION);
  SET_BIT(ch->misc, DC::LogChannel::CHANNEL_INFO);
  SET_BIT(ch->misc, DC::LogChannel::CHANNEL_NEWBIE);
  SET_BIT(ch->misc, DC::LogChannel::CHANNEL_TELL);
  SET_BIT(ch->misc, DC::LogChannel::CHANNEL_HINTS);
  ch->group_name = {};
  ch->ambush = {};
  ch->guarding = {};
  ch->guarded_by = {};
}

/*
 * Clear but do not de-alloc.
 */
void clear_char(CharacterPtr ch)
{
  if (ch == nullptr)
  {
    return;
  }

  *ch = Character(ch->dc_);
  ch->in_room = DC::NOWHERE;
  ch->setStanding();
  ch->hometown = START_ROOM;
  GET_AC(ch) = 100; /* Basic Armor */
}

void clear_object(ObjectPtr obj)
{
  if (!obj)
    return;
  *obj = {};
  obj->item_number = -1;
  obj->in_room = DC::NOWHERE;
  obj->vroom = {};
  obj->obj_flags = obj_flag_data();
  obj->num_affects = {};
  obj->affected = {};

  obj->name({});
  obj->long_description({});
  obj->short_description({});
  obj->ActionDescription({});
  obj->ex_description = {};
  obj->carried_by = {};
  obj->equipped_by = {};

  obj->in_obj = {};
  obj->contains = {};

  obj->next_content = {};
  obj->next = {};
  obj->next_skill = {};
  ;
  obj->table = {};
  obj->slot = {};
  obj->wheel = {};
  obj->save_expiration = {};
  obj->no_sell_expiration = {};
}

// Roll up the random modifiers to saving throw for new character
void apply_initial_saves(CharacterPtr ch)
{
  for (qint32 i = {}; i <= SAVE_TYPE_MAX; i++)
    if (ch->number(0, 1))
      ch->player->saves_mods[i] = number(-3, 3);
    else
      ch->player->saves_mods[i] = {};
}

void init_char(CharacterPtr ch)
{
  ch->title = u"is still a virgin."_s;

  ch->clan = {};

  ch->short_description({});
  ch->long_description({});
  ch->description({});

  ch->hometown = real_room(START_ROOM);

  GET_STR(ch) = GET_RAW_STR(ch);
  GET_INT(ch) = GET_RAW_INT(ch);
  GET_WIS(ch) = GET_RAW_WIS(ch);
  GET_DEX(ch) = GET_RAW_DEX(ch);
  GET_CON(ch) = GET_RAW_CON(ch);

  ch->raw_mana = 100;
  redo_mana(ch);

  ch->mana = GET_MAX_MANA(ch);
  ch->setHP(GET_MAX_HIT(ch));
  ch->setMove(GET_MAX_MOVE(ch));

  switch (GET_CLASS(ch))
  {
  case CLASS_MAGE:
    GET_AC(ch) = 200;
    break;
  case CLASS_DRUID:
    GET_AC(ch) = 185;
    break;
  case CLASS_CLERIC:
    GET_AC(ch) = 170;
    break;
  case CLASS_ANTI_PAL:
    GET_AC(ch) = 155;
    break;
  case CLASS_THIEF:
    GET_AC(ch) = 140;
    break;
  case CLASS_BARD:
    GET_AC(ch) = 125;
    break;
  case CLASS_BARBARIAN:
    GET_AC(ch) = 110;
    break;
  case CLASS_RANGER:
    GET_AC(ch) = 95;
    break;
  case CLASS_PALADIN:
    GET_AC(ch) = 80;
    break;
  case CLASS_WARRIOR:
    GET_AC(ch) = 65;
    break;
  case CLASS_MONK:
    GET_AC(ch) = 50;
    break;
  default:
    GET_AC(ch) = 100;
    break;
  }

  ch->altar = {};
  ch->spec = {};
  ch->setPrompt({});
  ch->setLastPrompt({});
  ch->player->skillchange = {};
  ch->player->joining = {};
  ch->player->practices = {};
  ch->player->time.birth = time(0);
  ch->player->time.played = {};
  ch->player->time.logon = time(0);
  ch->player->toggles = {};
  ch->player->golem = {};
  ch->player->quest_points = {};
  for (qint32 j = {}; j < QUEST_MAX_CANCEL; j++)
    ch->player->quest_cancel[j] = {};
  for (qint32 j = {}; j <= QUEST_TOTAL / ASIZE; j++)
    ch->player->quest_complete[j] = {};

  SET_BIT(ch->player->toggles, Player::PLR_ANSI);
  SET_BIT(ch->player->toggles, Player::PLR_DAMAGE);
  qint32 i;
  for (i = {}; i < AFF_MAX / ASIZE + 1; i++)
    ch->affected_by[i] = {};

  apply_initial_saves(ch);

  for (qint32 i = {}; i < 3; i++)
    GET_COND(ch, i) = 50; // 50 ticks of "full-ness"

  reset_char(ch);
}

/* returns the real number of the room with given virt number */
room_t real_room(room_t virt)
{
  if (virt < 0 || virt > top_of_world)
  {
    return DC::NOWHERE;
  }

  if (rooms.contains(virt))
  {
    return virt;
  }

  return DC::NOWHERE;
}

/* returns the real number of the monster with given virt number */
qint32 real_mobile(qint32 virt)
{
  qint32 bot, top, mid;

  bot = {};
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;)
  {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->vnum() == virt)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((mob_index + mid)->vnum() > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }

  return -1;
}

/* returns the real number of the object with given virt number */
qint32 real_object(qint32 virt)
{
  qint32 bot, top, mid;

  bot = {};
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;)
  {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->vnum() == virt)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((obj_index + mid)->vnum() > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }

  return -1;
}

/*  This is where all the online builder load routines are.....  */
/*
 *  Godflesh...
 */
/* Instant Zone Maker..
 * Somehows I feel ths is gonna be a pain in the ass...
 * I just hope those fuckers appreciate it...
 * ---Godflesh..
 */

/************************************************************************
 | Since SOMEONE left out all these, and they're index related, which is
 |  db, I'm going to put them here.
 | -- morc
 */
qint32 obj_in_index(QString name, qint32 index)
{
  qint32 i, j;

  for (i = 0, j = 1; (i < MAX_INDEX) && (j <= index) &&
                     ((obj_index[i].item));
       i++)
    if (isexact(name, ((obj_index[i].item))->name()))
    {
      if (j == index)
        return i;
      j++;
    }

  return -1;
}

qint32 mob_in_index(QString name, qint32 index)
{
  qint32 i, j;

  for (i = 0, j = 1; (i < MAX_INDEX) && (j <= index) &&
                     ((CharacterPtr)(mob_index[i].item));
       i++)
    if (isexact(name, ((CharacterPtr)(mob_index[i].item))->name()))
    {
      if (j == index)
        return i;
      j++;
    }

  return -1;
}

// * ------- Begin MOB Prog stuff ---------- *

/* the functions */

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

qint32 mprog_name_to_type(QString name)
{
  if ((name == "in_file_prog"))
    return IN_FILE_PROG;
  if ((name == "act_prog"))
    return ACT_PROG;
  if ((name == "speech_prog"))
    return SPEECH_PROG;
  if ((name == "rand_prog"))
    return RAND_PROG;
  if ((name == "arand_prog"))
    return ARAND_PROG;
  if ((name == "fight_prog"))
    return FIGHT_PROG;
  if ((name == "hitprcnt_prog"))
    return HITPRCNT_PROG;
  if ((name == "death_prog"))
    return DEATH_PROG;
  if ((name == "entry_prog"))
    return ENTRY_PROG;
  if ((name == "greet_prog"))
    return GREET_PROG;
  if ((name == "all_greet_prog"))
    return ALL_GREET_PROG;
  if ((name == "give_prog"))
    return GIVE_PROG;
  if ((name == "bribe_prog"))
    return BRIBE_PROG;
  if ((name == "catch_prog"))
    return CATCH_PROG;
  if ((name == "attack_prog"))
    return ATTACK_PROG;
  if ((name == "load_prog"))
    return LOAD_PROG;
  if ((name == "command_prog"))
    return COMMAND_PROG;
  if ((name == "weapon_prog"))
    return WEAPON_PROG;
  if ((name == "armour_prog"))
    return ARMOUR_PROG;
  if ((name == "can_see_prog"))
    return CAN_SEE_PROG;
  if ((name == "damage_prog"))
    return DAMAGE_PROG;
  return (ERROR_PROG);
}

/* This routine reads in scripts of MOBprograms from a file */

void mprog_file_read(QString f, qint32 i)
{
  MobileProgramPtr mprog = {};
  FILE *stream = {};
  QChar letter = {};
  QString name = {};
  qint32 type = {};

  dc_sprintf(name, "%s%s", qPrintable(MOB_DIR), qPrintable(f));
  if (!(stream = fopen(name, "r")))
  {
    logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d couldn't opne mobprog file.", i);
    return;
  }
  for (;;)
  {
    if ((letter = fread_char(stream)) == '|')
      break;
    else if (letter != '>')
    {
      logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mprog_file_read: Invalid letter mob %d.", i);
      return;
    }
    switch ((type = fread_int(stream, 0, MPROG_MAX_TYPE_VALUE)))
    {
    case ERROR_PROG:
      logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d: in file prog error.", i);
      return;
    case IN_FILE_PROG:
      logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d: nested in file progs.", i);
      return;
    default:
      SET_BIT(mob_index[i].progtypes, type);
      mprog = new mob_prog_data;
      mprog->type = type;
      mprog->arglist = fread_string(stream, 0);
      mprog->comlist = fread_string(stream, 0);
      break;
    }
  }
}

void load_mobprogs(auto &stream)
{
  QChar letter;
  qint32 value;

  for (;;)
    switch (LOWER(letter = fread_char(stream)))
    {
    default:
      logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Load_mobprogs: bad command '%c'.", letter);
      break;
    case 's':
      return;
    case '*':
      break;
    case 'm':
      value = fread_int(stream, 0, 2147483467);
      if (real_mobile(value) < 0)
      {
        logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Load_mobprogs: vnum %d doesn't exist.", value);
        break;
      }
      mprog_file_read(fread_word(stream, 1), value);
      break;
    }
}

// * --- End MOBProgs stuff --- *

void DC::find_unordered_objects(void)
{
  qint32 cur_vnum, last_vnum = {};

  for (qint32 rnum = {}; rnum <= top_of_objt; rnum++, last_vnum = cur_vnum)
  {
    cur_vnum = obj_index[rnum].vnum();

    if (cur_vnum < last_vnum)
    {
      logf(0, DC::LogChannel::LOG_MISC, "Out of order vnum found. Vnum: %d Last Vnum: %d Rnum: %d", cur_vnum, last_vnum, rnum);
    }
  }
}

void find_unordered_mobiles(void)
{
  qint32 cur_vnum, last_vnum = {};

  for (qint32 rnum = {}; rnum <= top_of_mobt; rnum++, last_vnum = cur_vnum)
  {
    cur_vnum = mob_index[rnum].vnum();

    if (cur_vnum < last_vnum)
    {
      logf(0, DC::LogChannel::LOG_MISC, "Out of order vnum found. Vnum: %d Last Vnum: %d Rnum: %d", cur_vnum, last_vnum, rnum);
    }
  }
}

void string_to_file(auto &streamf, QString str)
{
  dc_fprintf(f, "%s~\n", str.remove('\r').toStdString().c_str());
}

void string_to_file(QTextStream &stream, QString str)
{
  stream << str.remove('\r') + "~\n";
}

void copySaveData(ObjectPtr target, ObjectPtr source)
{
  qint32 i;
  if ((i = eq_current_damage(source)) > 0)
  {
    for (; i > 0; i--)
      damage_eq_once(target);
  }

  if (source->short_description() != target->short_description())
    target->short_description(source->short_description());

  if (source->long_description() != target->long_description())
    target->long_description(source->long_description());

  if (source->name() != target->name())
    target->name(source->name());

  if (source->obj_flags.type_flag != target->obj_flags.type_flag)
    target->obj_flags.type_flag = source->obj_flags.type_flag;

  if (source->obj_flags.extra_flags != target->obj_flags.extra_flags)
    target->obj_flags.extra_flags = source->obj_flags.extra_flags;

  if (source->obj_flags.more_flags != target->obj_flags.more_flags)
    target->obj_flags.more_flags = source->obj_flags.more_flags;

  bool custom = isSet(source->obj_flags.more_flags, ITEM_CUSTOM);
  if (custom)
  {
    target->obj_flags.value[0] = source->obj_flags.value[0];
  }

  auto type_flag = source->obj_flags.type_flag;
  if ((custom || type_flag == ITEM_DRINKCON) && (source->obj_flags.value[1] != target->obj_flags.value[1]))
  {
    target->obj_flags.value[1] = source->obj_flags.value[1];
  }

  if ((custom || type_flag == ITEM_STAFF || type_flag == ITEM_WAND) && (source->obj_flags.value[2] != target->obj_flags.value[2]))
  {
    target->obj_flags.value[2] = source->obj_flags.value[2];
  }

  if (custom && (source->obj_flags.value[3] != target->obj_flags.value[3]))
  {
    target->obj_flags.value[3] = source->obj_flags.value[3];
  }

  if (type_flag == ITEM_ARMOR || type_flag == ITEM_WEAPON || type_flag == ITEM_INSTRUMENT || type_flag == ITEM_WAND || type_flag == ITEM_CONTAINER)
  {
    if (custom)
    {
      target->obj_flags.weight = source->obj_flags.weight;
      target->obj_flags.cost = source->obj_flags.cost;
      target->obj_flags.value[1] = source->obj_flags.value[1];
      target->obj_flags.value[2] = source->obj_flags.value[2];

      // If new object does not have enough room for affects to be copied then realloc it
      if (source->num_affects > target->num_affects)
      {
        errno = {};
        target->affected.resize(source->num_affects);
        target->num_affects = source->num_affects;
      }

      for (qint32 i = {}; i < source->num_affects; ++i)
      {
        target->affected[i].location = source->affected[i].location;
        target->affected[i].modifier = source->affected[i].modifier;
      }
    }
  }

  if (isSet(source->obj_flags.more_flags, ITEM_24H_SAVE))
  {
    target->save_expiration = source->save_expiration;
  }

  if (isSet(source->obj_flags.more_flags, ITEM_24H_NO_SELL))
  {
    target->no_sell_expiration = source->no_sell_expiration;
  }
}

bool fullItemMatch(ObjectPtr obj, ObjectPtr obj2)
{
  if (dc_strcmp(GET_OBJ_SHORT(obj), GET_OBJ_SHORT(obj2)))
  {
    return false;
  }

  if (obj->name() != obj2->name())
  {
    return false;
  }

  if (obj->obj_flags.extra_flags != obj2->obj_flags.extra_flags)
  {
    return false;
  }

  if (obj->obj_flags.more_flags != obj2->obj_flags.more_flags)
  {
    return false;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) && obj->obj_flags.cost != obj2->obj_flags.cost)
  {
    return false;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) && obj->obj_flags.value[0] != obj2->obj_flags.value[0])
  {
    return false;
  }

  if (obj->obj_flags.type_flag != obj2->obj_flags.type_flag)
  {
    return false;
  }

  if ((isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) || obj->obj_flags.type_flag == ITEM_DRINKCON) && obj->obj_flags.value[1] != obj2->obj_flags.value[1])
  {
    return false;
  }

  if ((isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) || obj->obj_flags.type_flag == ITEM_STAFF || obj->obj_flags.type_flag == ITEM_WAND) && (obj->obj_flags.value[2] != obj2->obj_flags.value[2]))
  {
    return false;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) && obj->obj_flags.value[3] != obj2->obj_flags.value[3])
  {
    return false;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) && obj->num_affects != obj2->num_affects)
  {
    return false;
  }

  // check if any of the affects don't match
  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM))
  {
    for (qint32 i = {}; i < obj->num_affects; ++i)
    {
      if ((obj->affected[i].location != obj2->affected[i].location) ||
          (obj->affected[i].modifier != obj2->affected[i].modifier))
      {
        return false;
      }
    }
  }

  return true;
}

// Function to ensure an item is not bugged. If it is, replace it with the original.
bool verify_item(ObjectPtr obj)
{
  extern qint32 top_of_objt;

  if (obj->short_description() == (obj_index[obj->item_number].item)->short_description())
    return false;

  qint32 newitem = -1;
  for (qint32 i = 1;; i++)
  {

    if (obj->item_number - i < 0 && obj->item_number + i > top_of_objt)
      break; // No item at all found, it's a restring or deleted.

    if (obj->item_number - i >= 0)
    {
      obj_index_dataPtr obj_index_entry = &obj_index[obj->item_number - i];
      if (obj_index_entry)
      {
        ObjectPtr obj_index_item = obj_index_entry->item;
        if (obj_index_item)
        {
          if (obj->short_description() == obj_index_item->short_description())
          {
            newitem = obj->item_number - i;
            break;
          }
        }
        else
        {
          qWarning("obj_index_item is null");
        }
      }
      else
      {
        qWarning("obj_index_entry is null");
      }
    }

    if (obj->item_number + i <= top_of_objt)
      if (obj->short_description() == (obj_index[obj->item_number + i].item)->short_description())
      {
        newitem = obj->item_number + i;
        break;
      }
  }
  if (newitem == -1)
    return false;

  *obj = clone_object(newitem); // Fixed!
  return true;
}

LegacyFile::LegacyFile(QString directory, QString filename, QString error_message)
    : directory_(directory), filename_(filename), error_message_(error_message), file_handle_(nullptr)
{
  if (directory_.isEmpty())
  {
    directory_ = "./";
  }
  else if (!directory_.endsWith('/'))
  {
    directory_ += "/";
  }
  filepath_ = u"%1%2"_s.arg(directory_).arg(filename_);

  if (!QDir(directory_).exists())
  {
    logentry(u"LegacyFile::openFile: Directory '%1' does not exist."_s.arg(directory_));
  }

  openFile();
}

LegacyFile::~LegacyFile()
{
  if (file_handle_)
  {
  }
}

FILE *LegacyFile::openFile(void)
{
  if (file_handle_)
  {
  }

  if ((file_handle_ = fopen(qPrintable(filepath_), "w")) == nullptr)
  {
    qCritical() << error_message_.arg(filepath_);
    return {};
  }

  return file_handle_;
}

bool LegacyFile::backupFile(void)
{
  QFileInfo fi(filepath_);
  if (fi.exists())
  {
    QString originalfileName = fi.canonicalFilePath();
    QString backupFileName = u"%1.last"_s.arg(originalfileName);

    if (QFile::exists(backupFileName))
    {
      if (!QFile::remove(backupFileName))
      {
        logentry(u"Unable to remove '%1'."_s.arg(backupFileName));
      }
    }
    if (!QFile::copy(originalfileName, backupFileName))
    {
      logentry(u"Unable to copy '%1' to '%2'."_s.arg(originalfileName).arg(backupFileName));
    }
    else
    {
      return true;
    }
  }

  return false;
}

QDebug operator<<(QDebug debug, const Room::room_errors_t &errors)
{
  switch (errors)
  {
  case Room::room_errors_t::alllow_class:
    debug << "Room::room_errors_t::allow_class";
    break;
  case Room::room_errors_t::denied:
    debug << "Room::room_errors_t::denied";
    break;
  case Room::room_errors_t::description:
    debug << "Room::room_errors_t::description";
    break;
  case Room::room_errors_t::direction:
    debug << "Room::room_errors_t::direction";
    break;
  case Room::room_errors_t::ex_description:
    debug << "Room::room_errors_t::ex_description";
    break;
  case Room::room_errors_t::funct:
    debug << "Room::room_errors_t::funct";
    break;
  case Room::room_errors_t::light:
    debug << "Room::room_errors_t::light";
    break;
  case Room::room_errors_t::name:
    debug << "Room::room_errors_t::name";
    break;
  case Room::room_errors_t::number:
    debug << "Room::room_errors_t::number";
    break;
  case Room::room_errors_t::room_flags:
    debug << "Room::room_errors_t::room_flags";
    break;
  case Room::room_errors_t::sector_type:
    debug << "Room::room_errors_t::sector_type";
    break;
  case Room::room_errors_t::temp_room_flags:
    debug << "Room::room_errors_t::temp_room_flags";
    break;
  case Room::room_errors_t::zone:
    debug << "Room::room_errors_t::zone";
    break;
  case Room::room_errors_t::zonePtr:
    debug << "Room::room_errors_t::zonePtr";
    break;
  }
  return debug;
}

QDebug operator<<(QDebug debug, const std::expected<bool, Room::room_errors_t> &status)
{
  if (status.has_value())
    debug << status.value();
  else
    debug << status.error();

  return debug;
}