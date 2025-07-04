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

int load_debug = 0;
#include <QString>

#include "DC/DC.h"			// extra_descr_data
#include "DC/game_portal.h" // load_game_portals()
#include "DC/spells.h"		// command_range
#include "DC/help.h"
#include "DC/const.h"	// mob_race_mod, races, item_types, apply_types
#include "DC/connect.h" // Connection

Room &World::operator[](room_t room_key)
{
	static Room generic_room = {};

	if (room_key > DC::getInstance()->top_of_world || room_key == DC::NOWHERE || !DC::getInstance()->rooms.contains(room_key))
	{
		generic_room = {};
		return generic_room;
	}

	return DC::getInstance()->rooms[room_key];
}

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
struct message_list fight_messages[MAX_MESSAGES]; /* fighting messages   */
struct skill_quest *skill_list;					  // List of skill quests.

char webpage[MAX_STRING_LENGTH];	/* the webbrowser connect screen*/
char greetings1[MAX_STRING_LENGTH]; /* the greeting screen          */
char greetings2[MAX_STRING_LENGTH]; /* the other greeting screen    */
char greetings3[MAX_STRING_LENGTH];
char greetings4[MAX_STRING_LENGTH];
char credits[MAX_STRING_LENGTH];   /* the Credits List              */
char motd[MAX_STRING_LENGTH];	   /* the messages of today         */
char imotd[MAX_STRING_LENGTH];	   /* the immortal messages of today*/
char story[MAX_STRING_LENGTH];	   /* the game story                */
char help[MAX_STRING_LENGTH];	   /* the main help page            */
char new_help[MAX_STRING_LENGTH];  /* the main new help page            */
char new_ihelp[MAX_STRING_LENGTH]; /* the main immortal help page            */
char info[MAX_STRING_LENGTH];	   /* the info text                 */

FILE *help_fl;	   /* file for help texts (HELP <kwd>)*/
FILE *new_help_fl; /* file for help texts (HELP <kwd>)*/

struct help_index_element *help_index = 0;
struct help_index_element_new *new_help_table = 0;

int top_of_mobt = 0; /* top of mobile index table       */

struct weather_data weather_info; /* the infomation about the weather */

struct vault_data *vault_table = 0;

/* local procedures */
void setup_dir(FILE *fl, int room, int dir);
void load_banned();
void boot_world(void);
void do_godlist();
void half_chop(const char *str, char *arg1, char *arg2);
QString read_next_worldfile_name(FILE *flWorldIndex);
int file_to_string(const char *name, char *buf);
void clear_char(Character *ch);

// MOBprogram locals
int mprog_name_to_type(QString name);
// void		load_mobprogs           ( FILE* fp );
void mprog_read_programs(FILE *fp, int32_t i, bool ignore);
void mprog_read_programs(QTextStream &fp, int32_t i, bool ignore);

extern bool MOBtrigger;

/* external refs */

struct help_index_element *build_help_index(FILE *fl, int *num);
// The Room implementation
// -Sadus 9/1/96

void Room::FreeTracks()
{
	room_track_data *curr;

	for (curr = tracks; curr; curr = tracks)
	{
		tracks = tracks->next;
		// trackee is a str_hsh, don't free it
		dc_free(curr);
	}
	tracks = nullptr;
	last_track = nullptr;
	nTracks = 0;
}

Arena &Room::arena(void)
{
	static Arena generic_arena;
	if (isArena())
	{
		return DC::getInstance()->arena_;
	}
	else
	{
		generic_arena = {};
		return generic_arena;
	}
}

// add new tracks to the head of the list. When the list
// gets longer than 11, remove the tail and delete it.
void Room::AddTrackItem(room_track_data *newTrack)
{
	if (!tracks)
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
		room_track_data *pScent;
		pScent = last_track->previous;
		pScent->next = 0;
		dc_free(last_track);
		last_track = pScent;
		nTracks--;
	}
}

bool operator==(const struct deny_data &dd1, const struct deny_data &dd2)
{
	QList<decltype(dd1.vnum)> denies1;
	const struct deny_data *curr1 = &dd1;
	do
	{
		denies1.push_back(curr1->vnum);
	} while ((curr1 = curr1->next));

	QList<decltype(dd2.vnum)> denies2;
	const struct deny_data *curr2 = &dd2;
	do
	{
		denies2.push_back(curr2->vnum);
	} while ((curr2 = curr2->next));

	return denies1 == denies2;
}

bool operator==(struct extra_descr_data &edd1, struct extra_descr_data &edd2)
{
	QMap<QString, QString> extra_descriptions1;
	struct extra_descr_data *curr1 = &edd1;
	do
	{
		extra_descriptions1.insert(curr1->keyword, curr1->description);
	} while ((curr1 = curr1->next));

	QMap<QString, QString> extra_descriptions2;
	struct extra_descr_data *curr2 = &edd2;
	do
	{
		extra_descriptions2.insert(curr2->keyword, curr2->description);
	} while ((curr2 = curr2->next));

	return extra_descriptions1 == extra_descriptions2;
}

bool operator==(const struct room_direction_data &rdd1, const struct room_direction_data &rdd2)
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
	for (int direction = 0; direction < MAX_DIRS; ++direction)
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
			QString(r1.name) == QString(r2.name) &&
			QString(r1.description) == QString(r2.description) &&
			r1.ex_description == r2.ex_description &&
			r1.room_flags == r2.room_flags &&
			r1.temp_room_flags == r2.temp_room_flags &&
			r1.light == r2.light &&
			r1.funct == r2.funct &&
			// r1.contents == r2.contents &&
			// r1.people == r2.people &&
			// r1.nTracks == r2.nTracks &&
			// r1.tracks == r2.tracks &&
			// r1.iFlags == r2.iFlags &&
			// ((r1.paths == r2.paths) || (r1.paths && r2.paths && *r1.paths == *r2.paths)) &&
			!memcmp(r1.allow_class, r2.allow_class, sizeof(r1.allow_class)));
}

room_track_data *Room::TrackItem(int nIndex)
{
	room_track_data *pScent;
	quint64 indexCount = 1;
	for (pScent = tracks; pScent; pScent = pScent->next, indexCount++)
		if (indexCount == nIndex)
			return pScent;

	return 0;
}

void Character::add_to_bard_list(void)
{
	pulse_data *curr = nullptr;

	if (GET_CLASS(this) != CLASS_BARD)
		return;

#ifdef LEAK_CHECK
	curr = (struct pulse_data *)
		calloc(1, sizeof(struct pulse_data));
#else
	curr = (struct pulse_data *)
		dc_alloc(1, sizeof(struct pulse_data));
#endif

	curr->thechar = this;
	curr->next = DC::getInstance()->bard_list;
	DC::getInstance()->bard_list = curr;
}

void Character::remove_from_bard_list(void)
{
	pulse_data *curr = nullptr;
	pulse_data *last = nullptr;

	if (!DC::getInstance()->bard_list)
		return;

	if (DC::getInstance()->bard_list->thechar == this)
	{
		curr = DC::getInstance()->bard_list;
		DC::getInstance()->bard_list = DC::getInstance()->bard_list->next;
		dc_free(curr);
	}
	else
	{
		last = DC::getInstance()->bard_list;
		for (curr = DC::getInstance()->bard_list->next; curr; curr = curr->next)
		{
			if (curr->thechar == this)
			{
				last->next = curr->next;
				dc_free(curr);
				break;
			}
			last = curr;
		}
	}
}

char *funnybootmessages[] =
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
		"Cacheing zone connection std::map...\r\n",
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
		"Searching for intelligent players....searching....searching....searching\n\r",
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
	class Connection *d;

	extern int was_hotboot;

	if (!was_hotboot)
		return;

	int num = sizeof(funnybootmessages) / sizeof(char *);

	num = number(0, num - 1);

	for (d = DC::getInstance()->descriptor_list; d; d = d->next)
		write_to_descriptor(d->descriptor, funnybootmessages[num]);
}

/* Write skillquest file.
 It checks if ch exists everywhere it is used,
 so this can be called from other places without
 a character attached. */
int do_write_skillquest(Character *ch, char *argument, int cmd)
{
	struct skill_quest *curr;
	FILE *fl;

	if (!(fl = fopen(SKILL_QUEST_FILE, "w")))
	{
		if (ch)
			ch->sendln("Can't open the skill quest file.");
		return eFAILURE;
	}
	for (curr = skill_list; curr; curr = curr->next)
	{
		fprintf(fl, "%d %s~\n", curr->num, curr->message);
		fprintf(fl, "%d %d\n", curr->clas, curr->level);
	}
	fprintf(fl, "0\n");
	fclose(fl);
	ch->sendln("Skill quests saved.");
	return eSUCCESS;
}

void load_skillquests()
{
	struct skill_quest *newsq, *last = 0;
	skill_list = nullptr;
	int i;
	FILE *fl;

	if (!(fl = fopen(SKILL_QUEST_FILE, "r")))
	{
		logentry(QStringLiteral("Cannot open skill quest file."), 0, DC::LogChannel::LOG_MISC);
		abort();
	}

	while ((i = fread_int(fl, 0, 1000)) != 0)
	{
#ifdef LEAK_CHECK
		newsq = (struct skill_quest *)calloc(1, sizeof(struct skill_quest));
#else
		newsq = (struct skill_quest *)dc_alloc(1, sizeof(struct skill_quest));
#endif

		newsq->num = i;
		if (find_sq(i))
		{
			char buf[512];
			sprintf(buf, "%d duplicate.", i);
			logentry(buf, 0, DC::LogChannel::LOG_BUG);
		}
		newsq->message = fread_string(fl, 0);
		newsq->clas = fread_int(fl, 0, 32768);
		newsq->level = fread_int(fl, 0, 200);
		newsq->next = 0;

		if (last)
			last->next = newsq;
		else
			skill_list = newsq;

		last = newsq;
	}
	fclose(fl);
}

/*************************************************************************
 *  routines for booting the system                                       *
 *********************************************************************** */

/* body of the booting system */
void DC::boot_db(void)
{
	int help_rec_count = 0;

	reset_time();

	logentry(QStringLiteral("************** BOOTING THE MUD ***********"), 0, DC::LogChannel::LOG_SOCKET);
	logverbose(QStringLiteral("************** BOOTING THE MUD ***********"), 0, DC::LogChannel::LOG_MISC);
	logentry(QStringLiteral("************** BOOTING THE MUD ***********"), 0, DC::LogChannel::LOG_WORLD);
	logverbose(QStringLiteral("Reading aux files."));
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
	logverbose(QStringLiteral("Godlist done!"));
	logverbose(QStringLiteral("Booting clans..."));

	boot_clans();

	logverbose(QStringLiteral("Loading new news file."));
	extern void loadnews();
	loadnews();

	logverbose(QStringLiteral("Loading new help file."));

	// new help file stuff
	if (!(new_help_fl = fopen(NEW_HELP_FILE, "r")))
	{
		perror(NEW_HELP_FILE);
		abort();
	}
	help_rec_count = count_hash_records(new_help_fl);
	fclose(new_help_fl);

	if (!(new_help_fl = fopen(NEW_HELP_FILE, "r")))
	{
		perror(NEW_HELP_FILE);
		abort();
	}
	CREATE(new_help_table, struct help_index_element_new, help_rec_count);
	load_new_help(new_help_fl);
	fclose(new_help_fl);
	// end new help files

	logverbose(QStringLiteral("Opening help file."));

	if (!(help_fl = fopen(HELP_KWRD_FILE, "r")))
	{
		perror(HELP_KWRD_FILE);
		abort();
	}

	help_index = build_help_index(help_fl, &top_of_helpt);
	fclose(help_fl);

	logverbose(QStringLiteral("Loading the zones"));
	boot_zones();

	logverbose(QStringLiteral("Loading the world."));
	top_of_world_alloc = 2000;

	funny_boot_message();

	boot_world();

	logverbose(QStringLiteral("Renumbering the world."));
	renum_world();

	funny_boot_message();

	logverbose(QStringLiteral("Generating mob indices/loading all mobiles"));
	load_mobiles();

	logverbose(QStringLiteral("Generating object indices/loading all objects"));
	load_objects();

	funny_boot_message();

	logverbose(QStringLiteral("renumbering zone table"));
	renum_zone_table();

	logverbose(QStringLiteral("Looking for unordered mobiles..."));
	find_unordered_mobiles();

	if (cf.bport == false)
	{
		logverbose(QStringLiteral("Loading Corpses."));
		load_corpses();
	}

	logverbose(QStringLiteral("Loading messages."));
	load_messages(MESS_FILE);
	load_messages(MESS2_FILE, 2000);

	logverbose(QStringLiteral("Loading socials."));
	boot_social_messages();

	logverbose(QStringLiteral("Processing game portals..."));
	load_game_portals();

	logverbose(QStringLiteral("Loading emoting objects..."));
	load_emoting_objects();

	logverbose(QStringLiteral("Adding clan room flags to rooms..."));
	assign_clan_rooms();

	logverbose(QStringLiteral("Assigning function pointers."));
	assign_mobiles();
	assign_objects();
	assign_rooms();

	// DC::config &cf = DC::getInstance()->cf;
	if (cf.verbose_mode)
	{
		qInfo("\n[ Room  Room]\t{Level}\t  Author\tZone\n");
	}

	// auto &zones = DC::getInstance()->zones;
	for (auto [zone_key, zone] : zones.asKeyValueRange())
	{
		if (cf.verbose_mode)
		{
			qInfo("%s", qUtf8Printable(QStringLiteral("[%1 %2]\t%3.").arg(zone.getBottom(), 5).arg(zone.getTop(), 5).arg(zone.Name())));
		}

		zone.reset(Zone::ResetType::full);
	}

	logverbose(QStringLiteral("Loading banned list"));
	load_banned();

	logverbose(QStringLiteral("Loading skill quests."));
	load_skillquests();

	logverbose(QStringLiteral("Assigning inventory to shopkeepers"));
	fix_shopkeepers_inventory();

	logverbose(QStringLiteral("Turning on MOB Progs"));
	MOBtrigger = true;

	logverbose(QStringLiteral("Loading quest one liners."));
	load_quests();

	logverbose(QStringLiteral("Loading vaults."));
	load_vaults();

	logverbose(QStringLiteral("Loading player hints."));
	load_hints();

	logverbose(QStringLiteral("Loading auction tickets."));
	TheAuctionHouse.Load();
}

/*
 int do_motdload(Character *ch, char *argument, int cmd)
 {
 file_to_string(MOTD_FILE, motd);
 file_to_string(IMOTD_FILE, imotd);
 ch->sendln("Motd and Imotd both reloaded.");
 return eSUCCESS;
 }
 */
void DC::do_godlist(void)
{
	logverbose(QStringLiteral("Reading ../lib/wizlist.txt"));
	QFile wizlist_file("../lib/wizlist.txt");
	if (!wizlist_file.open(QIODeviceBase::ReadOnly))
	{
		logentry(QStringLiteral("db.c: error reading ../lib/wizlist.txt"), ANGEL, DC::LogChannel::LOG_BUG);
		wizlist_file.close();
		return;
	}

	do
	{
		auto wizlist_file_line = wizlist_file.readLine().split(' ');
		QString immortal_name = wizlist_file_line.value(0);
		bool ok = false;
		level_t immortal_level = wizlist_file_line.value(1).toULongLong(&ok);
		if (!ok)
		{
			immortal_level = 0;
		}

		if (immortal_name.startsWith('@'))
		{
			wizlist.push_back({QStringLiteral("@"), 0});
			break;
		}
		wizlist.push_back({immortal_name, immortal_level});
		assert(wizlist.length() < 1000);
	} while (true);

	logverbose(QStringLiteral("Done!\n\r"));
	wizlist_file.close();
}

void DC::write_wizlist(std::stringstream &filename)
{
	write_wizlist(filename.str().c_str());
}

void DC::write_wizlist(std::string filename)
{
	write_wizlist(filename.c_str());
}

void DC::write_wizlist(const char filename[])
{
	QFile wizlist_file(filename);
	auto file_opened = wizlist_file.open(QIODeviceBase::WriteOnly);
	if (!file_opened)
	{
		logentry(QStringLiteral("Unable to open wizlist file: %1").arg(filename));
		return;
	}

	for (const auto &entry : wizlist)
	{
		if (entry.name.startsWith('@'))
		{
			wizlist_file.write("@ @\n");
			break;
		}
		if (entry.level >= IMMORTAL)
		{
			wizlist_file.write(QStringLiteral("%1 %2\n").arg(entry.name).arg(entry.level).toLocal8Bit());
		}
	}
	wizlist_file.close();
}

void DC::update_wizlist(Character *ch)
{
	int x;

	if (IS_NPC(ch))
		return;

	for (auto &entry : wizlist)
	{
		if (entry.name.startsWith('@'))
		{
			if (ch->isMortalPlayer())
				return;
			entry.name = ch->getName();
			entry.level = ch->getLevel();

			wizlist.push_back({QStringLiteral("@"), 0});
			break;
		}
		else
		{
			if (isexact(entry.name, GET_NAME(ch)))
			{
				entry.level = ch->getLevel();
				break;
			}
		}
	}

	write_wizlist("../lib/wizlist.txt");

	in_port_t port1 = 0;
	if (cf.ports.size() > 0)
	{
		port1 = cf.ports[0];
	}

	std::stringstream ssbuffer;
	ssbuffer << HTDOCS_DIR << port1 << "/wizlist.txt";
	write_wizlist(ssbuffer.str().c_str());
}

int do_wizlist(Character *ch, char *argument, int cmd)
{
	char buf[MAX_STRING_LENGTH], lines[500], space[80];
	int x, current_level, z = 1;
	int gods_each_level[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int line_length, sp;

	char *names[] =
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

	for (sp = 0; sp < 80; sp++)
		space[sp] = ' ';

	// count the number of gods at each level, store in array gods_each_level
	for (x = 0;; x++)
	{
		if (DC::getInstance()->wizlist[x].name[0] == '@')
			break;
		gods_each_level[DC::getInstance()->wizlist[x].level - IMMORTAL]++;
	}

	buf[0] = '\0';
	for (current_level = IMPLEMENTER; current_level >= IMMORTAL; current_level--)
	{
		if (gods_each_level[current_level - IMMORTAL] == 0)
			continue;

		line_length = strlen(names[current_level - IMMORTAL]);
		sp = 79 - line_length;
		sp /= 2;
		space[sp + 1] = '\0';
		sprintf(buf + strlen(buf), "\n\r%s%s\n\r", space,
				names[current_level - IMMORTAL]);
		space[sp + 1] = ' ';

		lines[0] = '\0';
		for (x = 0;; x++)
		{
			if (DC::getInstance()->wizlist[x].name[0] == '@')
			{
				z = 1;
				if (*lines)
				{
					line_length = strlen(lines) - 2;
					lines[strlen(lines) - 2] = '\n';
					lines[strlen(lines) - 1] = '\r';
					sp = 79 - line_length;
					sp /= 2;
					space[sp + 1] = '\0';
					sprintf(buf + strlen(buf), "%s%s", space, lines);
					space[sp + 1] = ' ';
					lines[0] = '\0';
				}
				break;
			}

			if (DC::getInstance()->wizlist[x].level != current_level)
				continue;

			if (z++ % 5)
				sprintf(lines + strlen(lines), "%s, ", qUtf8Printable(DC::getInstance()->wizlist[x].name));
			else
			{
				sprintf(lines + strlen(lines), "%s\n\r", qUtf8Printable(DC::getInstance()->wizlist[x].name));
				line_length = strlen(lines) - 2;
				sp = 79 - line_length;
				sp /= 2;
				space[sp + 1] = '\0';
				sprintf(buf + strlen(buf), "%s%s", space, lines);
				space[sp + 1] = ' ';
				lines[0] = '\0';
			}
		}
	}

	page_string(ch->desc, buf, 1);
	return 1;
}

/* reset the time in the game from file */
void DC::reset_time(void)
{
	int32_t beginning_of_time = 650336715;

	struct time_info_data mud_time_passed(time_t t2, time_t t1);

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
	DC::getInstance()->logverbose(QStringLiteral("Current Gametime: %1H %2D %3M %4Y.").arg(time_info.hours).arg(time_info.day).arg(time_info.month).arg(time_info.year));

	weather_info.pressure = 960;
	if ((time_info.month >= 7) && (time_info.month <= 12))
		weather_info.pressure += dice(1, 50);
	else
		weather_info.pressure += dice(1, 80);

	weather_info.change = 0;

	if (weather_info.pressure <= 980)
		weather_info.sky = SKY_LIGHTNING;
	else if (weather_info.pressure <= 1000)
		weather_info.sky = SKY_RAINING;
	else if (weather_info.pressure <= 1020)
		weather_info.sky = SKY_CLOUDY;
	else
		weather_info.sky = SKY_CLOUDLESS;
}

void DC::load_mobiles(void)
{
	logverbose(QStringLiteral("Opening mobile file index."));
	auto flMobIndex = fopen(MOB_INDEX_FILE, "r");
	if (!flMobIndex)
	{
		logentry(QStringLiteral("Could not open index file."));
		abort();
	}

	for (auto temp = read_next_worldfile_name(flMobIndex);
		 temp.isEmpty() == false;
		 temp = read_next_worldfile_name(flMobIndex))
	{
		QString endfile = QStringLiteral("mobs/%1").arg(temp);
		DC::getInstance()->logverbose(endfile);

		auto fl = fopen(qUtf8Printable(endfile), "r");
		if (!fl)
		{
			perror(qUtf8Printable(endfile));
			logbug(QStringLiteral("load_objects: could not open mob file '%1'.").arg(endfile));
			abort();
		}

		auto pItem = mobile_fileindex.newRange(temp);
		vnum_t vnum{}, lowest_vnum = UINT64_MAX, highest_vnum{};
		for (;;)
		{
			char buf[160]{};
			if (fgets(buf, 81, fl))
			{
				if (*buf == '#')
				{
					sscanf(buf, "#%lu", &vnum);
					if (vnum < lowest_vnum)
						lowest_vnum = vnum;
					if (vnum > highest_vnum)
						highest_vnum = vnum;

					mob_index[vnum].vnum = vnum;
					if (!(mob_index[vnum].item = read_mobile(vnum, fl)))
					{
						logbug(QStringLiteral("Unable to load mobile %lu!").arg(vnum));
					}
				}
				else if (*buf == '$') /* EOF */
					break;
			}
			else
			{
				logmisc(QStringLiteral("Bad char (%1)").arg(buf));
				abort();
			}
		}

		pItem.firstnum = lowest_vnum;
		pItem.lastnum = highest_vnum;

		fclose(fl);
	}

	fclose(flMobIndex);

	/*
	 Here the index gets processed, and mob classes gets
	 assigned. (Not done in read_mobile 'cause of
	 the fact that all mobs aren't read yet,
	 and an attempt to assign non-existant mob
	 procs would be bad).
	 */
	for (const auto &k : mob_index.keys())
	{
		add_mobspec(k);
	}
}

void DC::add_mobspec(vnum_t vnum)
{
	if (!mob_index.contains(vnum))
		return;

	Character *a = mob_index[vnum].item;
	if (!a)
		return;
	if (!a->c_class)
		return;

	int mob = 0;
	QSharedPointer<class MobProgram> mprg{};

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
			mob_index[vnum].mobspec = DC::getInstance()->mob_index[real_mobile(149)].mobprogs;
		else if (a->getLevel() < 35)
			mob_index[vnum].mobspec = DC::getInstance()->mob_index[real_mobile(150)].mobprogs;
		else if (a->getLevel() < 51)
			mob_index[vnum].mobspec = DC::getInstance()->mob_index[real_mobile(151)].mobprogs;
		else
			mob_index[vnum].mobspec = DC::getInstance()->mob_index[real_mobile(152)].mobprogs;
		break;
	default:
		break;
	}

	if (mob)
	{
		mob_index[vnum].mobspec = DC::getInstance()->mob_index[real_mobile(mob)].mobprogs;

		for (int j = 0; j < ACT_MAX / ASIZE + 1; j++)
		{
			SET_BIT(((Character *)mob_index[vnum].item)->mobdata->actflags[j],
					((Character *)DC::getInstance()->mob_index[real_mobile(mob)].item)->mobdata->actflags[j]);
		}

		for (int j = 0; j < AFF_MAX / ASIZE + 1; j++)
		{
			SET_BIT(((Character *)mob_index[vnum].item)->affected_by[j],
					((Character *)DC::getInstance()->mob_index[real_mobile(mob)].item)->affected_by[j]);
		}
	}

	if (mob_index[vnum].mobspec)
		for (mprg = mob_index[vnum].mobspec; mprg; mprg = mprg->next)
			SET_BIT(mob_index[vnum].progtypes, mprg->type);
}

void DC::remove_all_mobs_from_world(void)
{
	const auto &character_list = DC::getInstance()->character_list;

	for_each(character_list.begin(), character_list.end(),
			 [](Character *const &curr)
			 {
				 if (IS_NPC(curr))
					 extract_char(curr, true, QStringLiteral("DC::remove_all_mobs_from_world"));
				 else
					 do_quit(curr, "", 666);
			 });
	DC::getInstance()->removeDead();
}

void DC::remove_all_objs_from_world()
{
	Object *curr = nullptr;

	while ((curr = DC::getInstance()->object_list))
		extract_obj(curr);
}

void DC::load_objects(void)
{
	logverbose(QStringLiteral("Opening object files."));
	auto flObjIndex = fopen(OBJECT_INDEX_FILE, "r");
	if (!flObjIndex)
	{
		logentry(QStringLiteral("Cannot open object file index."), 0, DC::LogChannel::LOG_MISC);
		abort();
	}

	for (auto temp = read_next_worldfile_name(flObjIndex);
		 temp.isEmpty() == false;
		 temp = read_next_worldfile_name(flObjIndex))
	{
		QString endfile = QStringLiteral("objects/%1").arg(temp);
		DC::getInstance()->logverbose(temp);

		auto fl = fopen(qUtf8Printable(endfile), "r");
		if (!fl)
		{
			perror(qUtf8Printable(endfile));
			logbug(QStringLiteral("load_objects: could not open obj file '%1'.").arg(endfile));
			abort();
		}

		auto pItem = new_obj_file_item(temp);
		vnum_t vnum{}, lowest_vnum = UINT64_MAX, highest_vnum{};
		for (;;)
		{
			char buf[160]{};
			if (fgets(buf, 81, fl))
			{
				if (*buf == '#') /* allocate new_new cell */
				{
					sscanf(buf, "#%lu", &vnum);
					if (vnum < lowest_vnum)
						lowest_vnum = vnum;
					if (vnum > highest_vnum)
						highest_vnum = vnum;

					obj_index[vnum].vnum = vnum;
					if (!(obj_index[vnum].item = read_object(vnum, fl, false)))
					{
						logbug(QStringLiteral("Unable to load object %lu!").arg(vnum));
					}
				}
				else if (*buf == '$') /* EOF */
					break;
			}
			else
			{
				logmisc(QStringLiteral("Bad char (%1)").arg(buf));
				abort();
			}
		} // for ;;

		pItem->firstnum = lowest_vnum;
		pItem->lastnum = highest_vnum;

		fclose(fl);
	} // for next_in_file

	fclose(flObjIndex);
}

void write_one_room(LegacyFile &lf, int a)
{
	FILE *f = lf.file_handle_;
	struct extra_descr_data *extra;

	if (!DC::getInstance()->rooms.contains(a))
		return;

	fprintf(f, "#%d\n", DC::getInstance()->world[a].number);
	string_to_file(f, DC::getInstance()->world[a].name);
	string_to_file(f, DC::getInstance()->world[a].description);

	if (DC::getInstance()->world[a].iFlags)
		REMOVE_BIT(DC::getInstance()->world[a].room_flags, DC::getInstance()->world[a].iFlags);
	fprintf(f, "%lu %d %d\n",
			DC::getInstance()->world[a].zone,
			DC::getInstance()->world[a].room_flags,
			DC::getInstance()->world[a].sector_type);
	if (DC::getInstance()->world[a].iFlags)
		SET_BIT(DC::getInstance()->world[a].room_flags, DC::getInstance()->world[a].iFlags);

	/* exits */
	for (int b = 0; b <= 5; b++)
	{
		if (!(DC::getInstance()->world[a].dir_option[b]))
			continue;
		fprintf(f, "D%d\n", b);
		if (DC::getInstance()->world[a].dir_option[b]->general_description)
			string_to_file(f, DC::getInstance()->world[a].dir_option[b]->general_description);
		else
			fprintf(f, "~\n"); // print blank
		if (DC::getInstance()->world[a].dir_option[b]->keyword)
			string_to_file(f, DC::getInstance()->world[a].dir_option[b]->keyword);
		else
			fprintf(f, "~\n"); // print blank
		fprintf(f, "%d %d %lu\n",
				DC::getInstance()->world[a].dir_option[b]->exit_info,
				DC::getInstance()->world[a].dir_option[b]->key,
				DC::getInstance()->world[a].dir_option[b]->to_room);
	} /* exits */

	/* extra descriptions */
	// We push C-style linked list into QStack so we can maintain the order when writing
	QStack<extra_descr_data *> room_extra_descriptions;
	for (extra = DC::getInstance()->world[a].ex_description; extra; extra = extra->next)
	{
		room_extra_descriptions.push(extra);
	}

	while (!room_extra_descriptions.isEmpty())
	{
		extra = room_extra_descriptions.pop();
		if (!extra)
			break;
		fprintf(f, "E\n");
		if (extra->keyword)
			string_to_file(f, extra->keyword);
		else
			fprintf(f, "~\n"); // print blank
		if (extra->description)
			string_to_file(f, extra->description);
		else
			fprintf(f, "~\n"); // print blank
	} /* extra descriptions */

	struct deny_data *deni;
	for (deni = DC::getInstance()->world[a].denied; deni; deni = deni->next)
		fprintf(f, "B\n%d\n", deni->vnum);

	// Write out allowed classes if any
	for (int i = 0; i < CLASS_MAX; i++)
	{
		if (DC::getInstance()->world[a].allow_class[i] == true)
		{
			fprintf(f, "C%d\n", i);
		}
	}

	fprintf(f, "S\n");
}

int DC::read_one_room(FILE *fl, int &room_nr)
{
	char *temp = nullptr;
	char ch = 0;
	int dir = 0;
	struct extra_descr_data *new_new_descr{};
	zone_t zone_nr = {};

	ch = fread_char(fl);

	if (ch != '$')
	{
		room_nr = fread_int(fl, 0, 1000000);

		if (load_debug)
		{
			printf("Reading Room #: %d\n", room_nr);
			fflush(stdout);
		}

		temp = fread_string(fl, 0);

		if (room_nr)
		{
			DC::getInstance()->currentVNUM(room_nr);
			DC::getInstance()->currentType("Room");
			DC::getInstance()->currentName(temp);

			/* a new_new record to be read */

			if (room_nr >= top_of_world_alloc)
			{
				top_of_world_alloc = room_nr + 200;
			}

			if (top_of_world < room_nr)
				top_of_world = room_nr;

			DC::getInstance()->rooms[room_nr] = {};

			DC::getInstance()->world[room_nr].paths = 0;
			DC::getInstance()->world[room_nr].number = room_nr;
			DC::getInstance()->world[room_nr].name = temp;
		}
		char *description = fread_string(fl, 0);
		if (room_nr)
		{
			DC::getInstance()->world[room_nr].description = description;
			DC::getInstance()->world[room_nr].nTracks = 0;
			DC::getInstance()->world[room_nr].tracks = 0;
			DC::getInstance()->world[room_nr].last_track = 0;
			DC::getInstance()->world[room_nr].denied = 0;
			DC::getInstance()->total_rooms++;
		}
		// Ignore recorded zone number since it may not longer be valid
		fread_int(fl, -1, 64000); // zone nr

		if (room_nr)
		{
			// Go through the zone table until DC::getInstance()->world[room_nr].number is
			// in the current zone.

			bool found = false;
			zone_t zone_nr = {};
			for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
			{
				if (zone.getBottom() <= DC::getInstance()->world[room_nr].number && zone.getTop() >= DC::getInstance()->world[room_nr].number)
				{
					found = true;
					zone_nr = zone_key;
					break;
				}
			}
			if (!found)
			{
				QString error = QStringLiteral("Room %1 is outside of any zone.").arg(room_nr);
				logentry(error);
				logentry(QStringLiteral("Room outside of ANY zone.  ERROR"), IMMORTAL, DC::LogChannel::LOG_BUG);
			}
			else
			{
				auto &zone = DC::getInstance()->zones[zone_nr];
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
				DC::getInstance()->world[room_nr].zone = zone_nr;
			}
		}

		uint32_t room_flags = fread_bitvector(fl, -1, 2147483467);

		if (room_nr)
		{
			DC::getInstance()->world[room_nr].room_flags = room_flags;
			if (isSet(DC::getInstance()->world[room_nr].room_flags, NO_ASTRAL))
				REMOVE_BIT(DC::getInstance()->world[room_nr].room_flags, NO_ASTRAL);

			// This bitvector is for runtime and not stored in the files, so just initialize it to 0
			DC::getInstance()->world[room_nr].temp_room_flags = 0;
		}

		int sector_type = fread_int(fl, -1, 64000);

		if (room_nr)
		{
			DC::getInstance()->world[room_nr].sector_type = sector_type;

			if (load_debug)
			{
				printf("Flags are %lu %u %d\n", zone_nr, DC::getInstance()->world[room_nr].room_flags,
					   DC::getInstance()->world[room_nr].sector_type);
				fflush(stdout);
			}

			DC::getInstance()->world[room_nr].funct = 0;
			DC::getInstance()->world[room_nr].contents = 0;
			DC::getInstance()->world[room_nr].people = 0;
			DC::getInstance()->world[room_nr].light = 0; /* Zero light sources */

			for (size_t tmp = 0; tmp <= 5; tmp++)
				DC::getInstance()->world[room_nr].dir_option[tmp] = 0;

			DC::getInstance()->world[room_nr].ex_description = 0;
		}

		for (;;)
		{
			ch = fread_char(fl); /* dir field */

			/* direction field */
			if (ch == 'D')
			{
				dir = fread_int(fl, 0, 5);
				setup_dir(fl, room_nr, dir);
			}
			/* extra description field */
			else if (ch == 'E')
			{
				// strip off the \n after the E
				if (fread_char(fl) != '\n')
					fseek(fl, -1, SEEK_CUR);
#ifdef LEAK_CHECK
				new_new_descr = (struct extra_descr_data *)
					calloc(1, sizeof(struct extra_descr_data));
#else
				new_new_descr = (struct extra_descr_data *)
					dc_alloc(1, sizeof(struct extra_descr_data));
#endif
				new_new_descr->keyword = fread_string(fl, 0);
				new_new_descr->description = fread_string(fl, 0);

				if (room_nr)
				{
					new_new_descr->next = DC::getInstance()->world[room_nr].ex_description;
					DC::getInstance()->world[room_nr].ex_description = new_new_descr;
				}
				else
				{
					dc_free(new_new_descr);
				}
			}
			else if (ch == 'B')
			{
				struct deny_data *deni;
#ifdef LEAK_CHECK
				deni = (struct deny_data *)calloc(1, sizeof(struct deny_data));
#else
				deni = (struct deny_data *)dc_alloc(1, sizeof(struct deny_data));
#endif
				deni->vnum = fread_int(fl, -1, 2147483467);

				if (room_nr)
				{
					deni->next = DC::getInstance()->world[room_nr].denied;
					DC::getInstance()->world[room_nr].denied = deni;
				}
				else
				{
					dc_free(deni);
				}
			}
			else if (ch == 'S') /* end of current room */
				break;
			else if (ch == 'C')
			{
				int c_class = fread_int(fl, 0, CLASS_MAX);
				if (room_nr)
				{
					DC::getInstance()->world[room_nr].allow_class[c_class] = true;
				}
			}
		} // of for (;;) (get directions and extra descs)

		return true;
	} // if == $
	  //  dc_free(temp); /* cleanup the area containing the terminal $  */
	  // we no longer free temp, cause it's no longer used as a terminating char
	return false;
}

QString read_next_worldfile_name(FILE *flWorldIndex)
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

bool can_modify_this_room(Character *ch, int32_t vnum)
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

bool can_modify_room(Character *ch, int32_t vnum)
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

bool can_modify_this_mobile(Character *ch, int32_t vnum)
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

bool can_modify_mobile(Character *ch, int32_t mob)
{
	return can_modify_this_mobile(ch, mob);
}

bool can_modify_this_object(Character *ch, int32_t vnum)
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

bool can_modify_object(Character *ch, vnum_t obj)
{
	return can_modify_this_object(ch, obj);
}

void DC::set_zone_saved_zone(int32_t room)
{
	setZoneNotModified(world[room].zone);
}

void DC::set_zone_modified_zone(int32_t room)
{
	setZoneModified(world[room].zone);
}

void DC::set_zone_modified(int32_t modnum, world_file_list_item *list)
{
	world_file_list_item *curr = list;

	while (curr)
		if (modnum >= curr->firstnum && modnum <= curr->lastnum)
			break;
		else
			curr = curr->next;

	if (!curr)
	{
		logbug(QStringLiteral("VNUM %1 not found in any zone in the index").arg(modnum));
		return;
	}

	curr->flags = WORLD_FILE_MODIFIED;
}

void DC::set_zone_modified_world(int32_t room)
{
	set_zone_modified(room, world_file_list);
}

// rnum of mob
void DC::set_zone_modified_mob(int32_t mob)
{
	set_zone_modified(mob, DC::getInstance()->mob_file_list);
}

// vnum of obj
void DC::set_zone_modified_obj(vnum_t vnum)
{
	auto range = object_fileindex.findRange(vnum, vnum);
	if (!range.filename.isEmpty())
		REMOVE_BIT(range.flags, WORLD_FILE_MODIFIED);
}

void DC::set_zone_saved(int32_t modnum, world_file_list_item *list)
{
	world_file_list_item *curr = list;

	while (curr)
		if (modnum >= curr->firstnum && modnum <= curr->lastnum)
			break;
		else
			curr = curr->next;

	if (!curr)
	{
		logentry(QStringLiteral("ERROR in set_zone_modified: Cannot find room!!!"), IMMORTAL, DC::LogChannel::LOG_BUG);
		return;
	}
}

void DC::set_zone_saved_world(int32_t room)
{

	set_zone_saved(room, world_file_list);
}

void DC::set_zone_saved_mob(int32_t mob)
{

	set_zone_saved(mob, DC::getInstance()->mob_file_list);
}

void DC::set_zone_saved_obj(vnum_t obj)
{
	auto range = object_fileindex.findRange(obj, obj);
	if (range)
	{
		range.setNeedsSaving(false);
	}
}

/* destruct the world */
void DC::free_world_from_memory(void)
{
	struct extra_descr_data *curr_extra = nullptr;
	struct world_file_list_item *curr_wfli = nullptr;

	for (int i = 0; i <= DC::getInstance()->top_of_world; i++)
	{
		if (!DC::getInstance()->rooms.contains(i))
			continue;

		if (DC::getInstance()->world[i].name)
			dc_free(DC::getInstance()->world[i].name);

		if (DC::getInstance()->world[i].description)
			dc_free(DC::getInstance()->world[i].description);

		while (DC::getInstance()->world[i].ex_description)
		{
			curr_extra = DC::getInstance()->world[i].ex_description->next;
			if (DC::getInstance()->world[i].ex_description->keyword)
				dc_free(DC::getInstance()->world[i].ex_description->keyword);
			if (DC::getInstance()->world[i].ex_description->description)
				dc_free(DC::getInstance()->world[i].ex_description->description);
			dc_free(DC::getInstance()->world[i].ex_description);
			DC::getInstance()->world[i].ex_description = curr_extra;
		}

		for (int j = 0; j < 6; j++)
			if (DC::getInstance()->world[i].dir_option[j])
			{
				dc_free(DC::getInstance()->world[i].dir_option[j]->general_description);
				dc_free(DC::getInstance()->world[i].dir_option[j]->keyword);
				dc_free(DC::getInstance()->world[i].dir_option[j]);
			}

		DC::getInstance()->world[i].FreeTracks();
	}
	DC::getInstance()->rooms.clear();

	curr_wfli = world_file_list;

	while (curr_wfli)
	{

		world_file_list = curr_wfli->next;
		dc_free(curr_wfli);
		curr_wfli = world_file_list;
	}
}

void DC::free_mobs_from_memory(void)
{
	Character *curr = nullptr;

	for (int i = 0; i <= top_of_mobt; i++)
	{
		if ((curr = (Character *)mob_index[i].item))
		{
			free_char(curr, Trace("free_mobs_from_memory"));
			mob_index[i].item = nullptr;
		}
	}
}

void DC::free_objs_from_memory(void)
{
	for (auto &obj_index_entry : obj_index)
	{
		if (obj_index_entry.item)
		{
			free_obj(obj_index_entry.item);
			obj_index_entry.item = nullptr;
		}
	}
}

world_file_list_item *one_new_world_file_item(QString filename, int32_t room_nr)
{
	world_file_list_item *curr = nullptr;

#ifdef LEAK_CHECK
	curr = (world_file_list_item *)calloc(1, sizeof(world_file_list_item));
#else
	curr = (world_file_list_item *)dc_alloc(1, sizeof(world_file_list_item));
#endif

	curr->filename = filename;
	curr->firstnum = room_nr;
	curr->lastnum = -1;
	curr->flags = 0;
	curr->next = nullptr;
	return curr;
}

world_file_list_item *new_w_file_item(QString filename, int32_t room_nr, world_file_list_item *&list)
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

world_file_list_item *new_world_file_item(QString filename, int32_t room_nr)
{
	return new_w_file_item(filename, room_nr, DC::getInstance()->world_file_list);
}

/* load the rooms */
void DC::boot_world(void)
{
	FILE *fl;
	FILE *flWorldIndex;
	int room_nr = 0;
	QString temp;
	char endfile[200]; // hopefully noone is stupid and makes a 180 char filename
	struct world_file_list_item *pItem = nullptr;

	DC::getInstance()->object_list = 0;

	DC::config &cf = DC::getInstance()->cf;

	if (cf.test_world)
	{
		if (!(flWorldIndex = fopen(WORLD_INDEX_FILE_TINY, "r")))
		{
			int fopen_errno = errno;
			logentry(QStringLiteral("boot_world: could not open tiny world index file '%1': %2.").arg(WORLD_INDEX_FILE_TINY).arg(strerror(fopen_errno)), 0, DC::LogChannel::LOG_BUG);
			abort();
		}
	}
	else
	{
		if (!(flWorldIndex = fopen(WORLD_INDEX_FILE, "r")))
		{
			int fopen_errno = errno;
			logentry(QStringLiteral("boot_world: could not open world index file '%1': %2.").arg(WORLD_INDEX_FILE).arg(strerror(fopen_errno)), 0, DC::LogChannel::LOG_BUG);
			abort();
		}
	}

	// logentry(QStringLiteral("Booting individual world files"), 0, DC::LogChannel::LOG_MISC);

	// note, we don't worry about free'ing temp, cause it's held in the "world_file_list"
	for (temp = read_next_worldfile_name(flWorldIndex);
		 temp.isEmpty() == false;
		 temp = read_next_worldfile_name(flWorldIndex))
	{
		strcpy(endfile, "world/");
		strcat(endfile, temp.toStdString().c_str());

		DC::config &cf = DC::getInstance()->cf;
		if (cf.verbose_mode)
		{
			logentry(temp, 0, DC::LogChannel::LOG_MISC);
		}

		if (!(fl = fopen(endfile, "r")))
		{
			perror("fopen");
			logentry(QStringLiteral("boot_world: could not open world file."), 0, DC::LogChannel::LOG_BUG);
			logentry(temp, 0, DC::LogChannel::LOG_BUG);
			abort();
		}

		pItem = new_world_file_item(temp, room_nr);

		while (read_one_room(fl, room_nr))
			;

		// push the first num forward until it hits a room, that way it's
		// accurate.
		// "pItem->firstnum < top_of_world_alloc" check is to insure we dont access memory not allocated to DC::getInstance()->rooms
		for (; pItem->firstnum < top_of_world_alloc && !DC::getInstance()->rooms.contains(pItem->firstnum); pItem->firstnum++)
			;

		pItem->lastnum = room_nr / 100 * 100 + 99;

		room_nr++;

		fclose(fl);
	}
	// logentry(QStringLiteral("World Boot done."), 0, DC::LogChannel::LOG_MISC);
	fclose(flWorldIndex);

	top_of_world = --room_nr;
}

/* read direction data */
void setup_dir(FILE *fl, int room, int dir)
{
	int tmp;

	if (room && DC::getInstance()->world[room].dir_option[dir])
	{
		char buf[200];
		sprintf(buf, "Room %d attemped to created two exits in the same direction.", DC::getInstance()->world[room].number);
		logentry(buf, 0, DC::LogChannel::LOG_WORLD);
		if (DC::getInstance()->world[room].dir_option[dir]->general_description)
			dc_free(DC::getInstance()->world[room].dir_option[dir]->general_description);
		if (DC::getInstance()->world[room].dir_option[dir]->keyword)
			dc_free(DC::getInstance()->world[room].dir_option[dir]->keyword);

		dc_free(DC::getInstance()->world[room].dir_option[dir]);
	}

	if (room)
	{
#ifdef LEAK_CHECK
		DC::getInstance()->world[room].dir_option[dir] = (struct room_direction_data *)
			calloc(1, sizeof(struct room_direction_data));
#else
		DC::getInstance()->world[room].dir_option[dir] = (struct room_direction_data *)
			dc_alloc(1, sizeof(struct room_direction_data));
#endif
	}
	char *general_description = fread_string(fl, 0);

	if (room)
		DC::getInstance()->world[room].dir_option[dir]->general_description = general_description;

	char *keyword = fread_string(fl, 0);
	if (room)
		DC::getInstance()->world[room].dir_option[dir]->keyword = keyword;

	tmp = fread_bitvector(fl, -1, 300); /* tjs hack - not the right range */

	if (room)
	{
		DC::getInstance()->world[room].dir_option[dir]->exit_info = tmp;
		DC::getInstance()->world[room].dir_option[dir]->bracee = nullptr;
	}

	int16_t key = fread_int(fl, -62000, 62000);
	if (room)
	{
		DC::getInstance()->world[room].dir_option[dir]->key = key;
	}

	int16_t to_room = DC::NOWHERE;
	try
	{
		to_room = fread_int(fl, 0, 62000);
	}
	catch (...)
	{
	}

	if (room)
		DC::getInstance()->world[room].dir_option[dir]->to_room = to_room;
}

// return true for success
int DC::create_one_room(Character *ch, int vnum)
{
	Room *rp{};
	int x{};

	char buf[256]{};

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

	DC::getInstance()->rooms[vnum] = {};

	rp = &DC::getInstance()->world[vnum];

	rp->number = vnum;

	rp->zone = DC::getRoomZone(rp->number);

	rp->sector_type = 0;
	rp->room_flags = 0;
	rp->temp_room_flags = 0;
	rp->ex_description = 0;
	for (x = 0; x <= 5; x++)
		rp->dir_option[x] = 0;
	rp->light = 0;
	rp->contents = 0;
	rp->people = 0;
	rp->nTracks = 0;
	rp->tracks = 0;
	rp->last_track = 0;
	sprintf(buf, "Room %d", vnum);
	rp->name = (char *)str_dup(buf);
	rp->description = (char *)str_dup("Empty description.\r\n");
	return 1;
}

void renum_world(void)
{
	int room, door;

	for (room = 0; room <= DC::getInstance()->top_of_world; room++)
		for (door = 0; door <= 5; door++)
			if (DC::getInstance()->rooms.contains(room))
				if (DC::getInstance()->world[room].dir_option[door])
					if (DC::getInstance()->world[room].dir_option[door]->to_room != DC::NOWHERE)
						DC::getInstance()->world[room].dir_option[door]->to_room =
							real_room(DC::getInstance()->world[room].dir_option[door]->to_room);
}

void DC::renum_zone_table(void)
{
	int zone, comm;

	for (auto [zone_key, zone] : zones.asKeyValueRange())
	{
		assert(zone_key != 0);
		for (comm = 0; comm < zone.cmd.size(); comm++)
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
					zone.cmd[comm]->arg1 = real_mobile(zone.cmd[comm]->arg1);
				else
				{
					zone.cmd[comm]->active = 0;
				}
				//          zone.cmd[comm]->arg3;// =
				//                real_room(zone.cmd[comm]->arg3);

				break;
			case 'O':
				zone.cmd[comm]->active = obj_index.contains(zone.cmd[comm]->arg1) && real_room(zone.cmd[comm]->arg3) != DC::NOWHERE;
				break;
			case 'G':
			case 'E':
				zone.cmd[comm]->active = obj_index.contains(zone.cmd[comm]->arg1);
				break;
			case 'P':
				zone.cmd[comm]->active = obj_index.contains(zone.cmd[comm]->arg1) && obj_index.contains(zone.cmd[comm]->arg3);
				break;
			case 'D':
				zone.cmd[comm]->active = real_room(zone.cmd[comm]->arg1) != DC::NOWHERE;
				if (!zone.cmd[comm]->active)
				{
					qWarning("Problem in zonefile: no room number for door"
							 " - setting to J command\n");
					zone.cmd[comm]->command = 'J';
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
				logentry(QStringLiteral("Illegal char hit in renum_zone_table"), 0, DC::LogChannel::LOG_WORLD);
				break;
			}
		}
	}
}

void DC::free_zones_from_memory()
{
	for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
	{
		zone.Name(QString());
		zone.cmd.empty();
	}
}

void Zone::write(FILE *fl)
{
	fprintf(fl, "V2\n");
	fprintf(fl, "#%lu\n", (id_ ? (bottom / 100) : 0));
	fprintf(fl, "%s~\n", NameC());
	fprintf(fl, "%lu %lu %d %ld %d\n", top,
			lifespan,
			reset_mode,
			zone_flags,
			continent);

	for (int i = 0; i < cmd.size(); i++)
	{
		if (cmd[i]->command == '*')
			fprintf(fl, "* %s\n", cmd[i]->comment.toStdString().c_str() ? cmd[i]->comment.toStdString().c_str() : "");
		else if (cmd[i]->command == '%')
			fprintf(fl, "%% %2d %3d %3d %s\n", cmd[i]->if_flag,
					cmd[i]->arg1,
					cmd[i]->arg2,
					cmd[i]->comment.toStdString().c_str() ? cmd[i]->comment.toStdString().c_str() : "");
		else if (cmd[i]->command == 'X')
			fprintf(fl, "X %2d %5d %3d %5d%s\n", cmd[i]->if_flag,
					cmd[i]->arg1,
					cmd[i]->arg2,
					cmd[i]->arg3,
					cmd[i]->comment.toStdString().c_str() ? cmd[i]->comment.toStdString().c_str() : "");
		else if (cmd[i]->command == 'K')
			fprintf(fl, "K %2d %5d %3d %5d%s\n", cmd[i]->if_flag,
					cmd[i]->arg1,
					cmd[i]->arg2,
					cmd[i]->arg3,
					cmd[i]->comment.toStdString().c_str() ? cmd[i]->comment.toStdString().c_str() : "");
		else if (cmd[i]->command == 'M')
		{
			int virt = cmd[i]->active ? DC::getInstance()->mob_index[cmd[i]->arg1].vnum : cmd[i]->arg1;
			fprintf(fl, "M %2d %5d %3d %5d %s\n", cmd[i]->if_flag,
					virt,
					cmd[i]->arg2,
					cmd[i]->arg3,
					cmd[i]->comment.toStdString().c_str() ? cmd[i]->comment.toStdString().c_str() : "");
		}
		else if (cmd[i]->command == 'P')
		{
			int virt = cmd[i]->active ? DC::getInstance()->obj_index[cmd[i]->arg1].vnum : cmd[i]->arg1;
			int virt2 = cmd[i]->active ? DC::getInstance()->obj_index[cmd[i]->arg3].vnum : cmd[i]->arg3;
			fprintf(fl, "P %2d %5d %3d %5d %s\n", cmd[i]->if_flag,
					virt,
					cmd[i]->arg2,
					virt2,
					cmd[i]->comment.toStdString().c_str() ? cmd[i]->comment.toStdString().c_str() : "");
		}
		else if (cmd[i]->command == 'G')
		{
			int virt = cmd[i]->active ? DC::getInstance()->obj_index[cmd[i]->arg1].vnum : cmd[i]->arg1;

			fprintf(fl, "G %2d %5d %3d %5d %s\n", cmd[i]->if_flag,
					virt,
					cmd[i]->arg2,
					cmd[i]->arg3,
					cmd[i]->comment.toStdString().c_str() ? cmd[i]->comment.toStdString().c_str() : "");
		}
		else if (cmd[i]->command == 'O')
		{
			int virt = cmd[i]->active ? DC::getInstance()->obj_index[cmd[i]->arg1].vnum : cmd[i]->arg1;
			fprintf(fl, "O %2d %5d %3d %5d %s\n", cmd[i]->if_flag,
					virt,
					cmd[i]->arg2,
					cmd[i]->arg3,
					cmd[i]->comment.toStdString().c_str() ? cmd[i]->comment.toStdString().c_str() : "");
		}
		else if (cmd[i]->command == 'E')
		{
			int virt = cmd[i]->active ? DC::getInstance()->obj_index[cmd[i]->arg1].vnum : cmd[i]->arg1;
			fprintf(fl, "E %2d %5d %3d %5d %s\n", cmd[i]->if_flag,
					virt,
					cmd[i]->arg2,
					cmd[i]->arg3,
					cmd[i]->comment.toStdString().c_str() ? cmd[i]->comment.toStdString().c_str() : "");
		}
		else
			fprintf(fl, "%c %2d %5d %3d %5d %s\n", cmd[i]->command,
					cmd[i]->if_flag,
					cmd[i]->arg1,
					cmd[i]->arg2,
					cmd[i]->arg3,
					cmd[i]->comment.toStdString().c_str() ? cmd[i]->comment.toStdString().c_str() : "");
	}

	fprintf(fl, "S\n$~\n");
}

zone_t DC::read_one_zone(FILE *fl)
{
	static room_t last_top_vnum = 0;
	zone_commands_t reset_tab;
	char *check, buf[161], ch;
	int reset_top, i, tmp;
	char *skipper = nullptr;
	int version = 1;
	bool modified = false;

	ch = fread_char(fl);
	if (ch == 'V')
	{
		version = fread_int(fl, 0, 64000);
		ch = fread_char(fl);
		modified = true;
	}

	tmp = fread_int(fl, 0, 64000);
	check = fread_string(fl, 0);
	// a = fread_int(fl, 0, 64000);
	/* alloc a new_new zone */
	//*num = zon = a / 100;

	auto &zones = DC::getInstance()->zones;
	zone_t new_zone_key = 1;
	if (!zones.isEmpty())
	{
		new_zone_key = zones.lastKey() + 1;
	}

	Zone zone(new_zone_key);

	// logentry(QStringLiteral("Reading zone"), 0, DC::LogChannel::LOG_BUG);

	DC::getInstance()->currentVNUM(tmp);
	DC::getInstance()->currentType("Zone");
	DC::getInstance()->currentName(check);

	zone.Name(check);
	zone.setBottom(last_top_vnum + 1);
	zone.setTop(fread_int(fl, 0, WORLD_MAX_ROOM));
	last_top_vnum = zone.getTop();
	zone.setRealBottom(0);
	zone.setRealTop(0);
	zone.clanowner = 0;
	zone.gold = 0;
	zone.repops_without_deaths = -1;
	zone.repops_with_bonus = 0;

	zone.lifespan = fread_int(fl, 0, 64000);
	zone.reset_mode = fread_int(fl, 0, 64000);
	zone.setZoneFlags(fread_bitvector(fl, 0, 2147483467));

	// if its old version set the altered flag so that
	// this zone will be saved with new format soon
	if (modified == true)
	{
		zone.setNeedsSaving();
	}

	if (version > 1)
	{
		zone.continent = fread_int(fl, 0, 64000);
	}

	/* read the command table */

	for (;;)
	{
		QSharedPointer<ResetCommand> reset = QSharedPointer<ResetCommand>::create();
		reset->comment = nullptr; // needs to be initialized
		reset->command = fread_char(fl);
		reset->if_flag = 0;
		reset->last = 0;
		reset->arg1 = 0;
		reset->arg2 = 0;
		reset->arg3 = 0;
		if (reset->command == 'S')
		{
			break;
		}

		if (reset->command == '*')
		{
			fgets(buf, 160, fl); /* skip command */
			// skip any space
			skipper = buf;
			while (*skipper == ' ' || *skipper == '\t')
				skipper++;

			// kill terminating \n
			if (buf[strlen(buf) - 1] == '\n')
				buf[strlen(buf) - 1] = '\0';

			// if any, keep anything left
			if (*skipper)
				reset->comment = str_hsh(skipper);
			reset_tab.push_back(reset);
			continue;
		}

		tmp = fread_int(fl, 0, CMD_DEFAULT);
		reset->if_flag = tmp;
		reset->last = time(nullptr) - number(0, 12 * 3600);
		// randomize last repop on boot
		reset->arg1 = fread_int(fl, -64000, 2147483467);
		reset->arg2 = fread_int(fl, -64000, 2147483467);
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
			reset->arg3 = fread_int(fl, -64000, 32768);
		else
			reset->arg3 = 0;

		reset->lastPop = 0;

		if (reset->arg3 > 64000)
			reset->arg1 = 1;

		/* tjs hack - ugly tmp bug fix */
		// this just moves our cursor back 1 position
		fseek(fl, -1, SEEK_CUR);
		fgets(buf, 160, fl); /* read comment */

		skipper = buf;

		while (*skipper == ' ' || *skipper == '\t')
			skipper++;

		// kill terminating \n
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';

		// if any, keep anything left
		if (*skipper)
			reset->comment = str_hsh(skipper);

		reset_tab.push_back(reset);

	} // for( ;; ) til end of zone commands

	// copy the temp into the memory
	zone.cmd = reset_tab;
	// // std::cerr << QStringLiteral("Insert %1 into QMap with size %2").arg(zone_nr).arg(zones.size()).toStdString() << std::endl;
	zones.insert(new_zone_key, zone);
	return new_zone_key;
}

/* load the zone table and command tables */
void DC::boot_zones(void)
{
	FILE *fl;
	FILE *flZoneIndex;
	QString temp;
	char endfile[200]; // hopefully noone is stupid and makes a 180 char filename

	DC::config &cf = DC::getInstance()->cf;

	if (cf.test_world == false && cf.test_mobs == false && cf.test_objs == false)
	{
		if (!(flZoneIndex = fopen(ZONE_INDEX_FILE, "r")))
		{
			perror("fopen");
			logentry(QStringLiteral("boot_world: could not open world index file."), 0, DC::LogChannel::LOG_BUG);
			abort();
		}
	}
	else if (!(flZoneIndex = fopen(ZONE_INDEX_FILE_TINY, "r")))
	{
		perror("fopen");
		logentry(QStringLiteral("boot_world: could not open world index file tiny."), 0, DC::LogChannel::LOG_BUG);
		abort();
	}
	// logentry(QStringLiteral("Booting individual zone files"), 0, DC::LogChannel::LOG_MISC);

	for (temp = read_next_worldfile_name(flZoneIndex);
		 temp.isEmpty() == false;
		 temp = read_next_worldfile_name(flZoneIndex))
	{
		strcpy(endfile, "zonefiles/");
		strcat(endfile, temp.toStdString().c_str());

		if (cf.verbose_mode)
		{
			logentry(temp, 0, DC::LogChannel::LOG_MISC);
		}

		if (!(fl = fopen(endfile, "r")))
		{
			perror(endfile);
			logf(IMMORTAL, DC::LogChannel::LOG_BUG, "boot_zone: could not open zone file: %s", endfile);
			abort();
		}

		auto zone_key = read_one_zone(fl);
		auto &zone = zones[zone_key];
		zone.setFilename(temp);
		// // std::cerr << QStringLiteral("%1 %2").arg(zone).arg(temp).toStdString() << std::endl;

		fclose(fl);
	}

	// logentry(QStringLiteral("Zone Boot done."), 0, DC::LogChannel::LOG_MISC);

	fclose(flZoneIndex);

	//  fclose(fl);
}

/*************************************************************************
 *  procedures for resetting, both play-time and boot-time        *
 *********************************************************************** */

/* read a mobile from MOB_FILE */
Character *read_mobile(int nr, FILE *fl)
{
	char buf[200];
	int i, j;
	int32_t tmp, tmp2, tmp3;
	Character *mob;
	char letter;

	i = nr;

	mob = new Character;
	auto &free_list = DC::getInstance()->free_list;
	free_list.erase(mob);

	clear_char(mob);
	GET_RACE(mob) = 0;

	/***** String data *** */

	mob->setName(fread_string(fl, 1));
	/* set up the fread debug stuff */
	DC::getInstance()->currentType("Mob");
	DC::getInstance()->currentName(mob->getName());
	mob->short_desc = fread_string(fl, 1);
	mob->long_desc = fread_string(fl, 1);
	mob->description = fread_string(fl, 1);
	mob->title = 0;

	mob->mobdata = mobdata_t::create();
	mob->mobdata->reset = {};
	/* *** Numeric data *** */
	j = 0;
	while ((tmp = fread_int(fl, -2147483467, 2147483467)) != -1)
	{
		mob->mobdata->actflags[j] = tmp;
		j++;
	}
	for (; j < ACT_MAX / ASIZE + 1; j++)
		mob->mobdata->actflags[j] = 0;
	if (ISSET(mob->mobdata->actflags, ACT_NOTRACK))
		REMBIT(mob->mobdata->actflags, ACT_NOTRACK);
	mob->setType(Character::Type::NPC);

	j = 0;
	while ((tmp = fread_int(fl, -2147483467, 2147483467)) != -1)
	{
		mob->affected_by[j] = tmp;
		j++;
	}
	for (; j < AFF_MAX / ASIZE + 1; j++)
		mob->affected_by[j] = 0;

	mob->alignment = fread_int(fl, -2147483467, 2147483467);

	tmp = fread_int(fl, 0, DC::MAX_RACE);
	GET_RACE(mob) = (char)tmp;

	mob->raw_str = mob->str = BASE_STAT + mob_race_mod[GET_RACE(mob)][0];
	mob->raw_dex = mob->dex = BASE_STAT + mob_race_mod[GET_RACE(mob)][1];
	mob->raw_con = mob->con = BASE_STAT + mob_race_mod[GET_RACE(mob)][2];
	mob->raw_intel = mob->intel = BASE_STAT + mob_race_mod[GET_RACE(mob)][3];
	mob->raw_wis = mob->wis = BASE_STAT + mob_race_mod[GET_RACE(mob)][4];

	mob->setLevel(fread_int(fl, 0, IMPLEMENTER));

	mob->hitroll = 20 - fread_int(fl, -64000, 64000);
	mob->armor = 10 * fread_int(fl, -64000, 64000);

	tmp = fread_int(fl, 0, 64000);
	tmp2 = fread_int(fl, 0, 64000);
	tmp3 = fread_int(fl, 0, 64000);

	mob->raw_hit = dice(tmp, tmp2) + tmp3;
	mob->max_hit = mob->raw_hit;
	mob->hit = mob->max_hit;

	mob->mobdata->damnodice = fread_int(fl, 0, 64000);
	mob->mobdata->damsizedice = fread_int(fl, 0, 64000);
	mob->damroll = fread_int(fl, 0, 64000);
	mob->mobdata->last_room = 0;
	mob->mana = 100 + (mob->getLevel() * 10);
	mob->max_mana = 100 + (mob->getLevel() * 10);

	mob->setMove(100 + (mob->getLevel() * 10));
	mob->max_move = 100 + (mob->getLevel() * 10);

	mob->max_ki = 100.0 * (mob->getLevel() / 60.0);
	mob->ki = mob->max_ki;
	mob->raw_ki = mob->max_ki;

	mob->setGold(fread_int(fl, 0, 2147483467));
	mob->plat = 0;
	GET_EXP(mob) = (int64_t)fread_int(fl, -2147483467, 2147483467);

	mob->setPosition(static_cast<position_t>(fread_int(fl, 0, 10)));
	mob->mobdata->default_pos = static_cast<position_t>(fread_int(fl, 0, 10));

	tmp = fread_int(fl, 0, 12);

	/* Read in ISR vlues...  (sex +3) */
	// Eventually I can remove this "if" but not until I fix them all.
	if (tmp > 2)
		tmp -= 3;

	mob->sex = (Character::sex_t)tmp;

	mob->immune = fread_bitvector(fl, 0, 2147483467);
	mob->suscept = fread_bitvector(fl, 0, 2147483467);
	mob->resist = fread_bitvector(fl, 0, 2147483467);

	// if all three are 0, then chances are someone just didn't set them, so go with
	// the race defaults.
	//    if(mob->immune == 0 && mob->suscept == 0 && mob->resist == 0)
	//  {
	SET_BIT(mob->immune, races[(int)GET_RACE(mob)].immune);
	SET_BIT(mob->suscept, races[(int)GET_RACE(mob)].suscept);
	SET_BIT(mob->resist, races[(int)GET_RACE(mob)].resist);
	// TOODO:FIXTHIS         SETBIT(mob->affected_by, races[(int)GET_RACE(mob)].affects);
	//      mob->immune  = races[(int)GET_RACE(mob)].immune;
	//    mob->suscept = races[(int)GET_RACE(mob)].suscept;
	//  mob->resist  = races[(int)GET_RACE(mob)].resist;
	//    }

	mob->c_class = 0;

	do
	{
		letter = fread_char(fl);
		switch (letter)
		{
		case 'C':
			mob->c_class = fread_int(fl, 0, 2147483467);
			// fread_new_newline(fl);
			break;
		case 'T': // sTats
			mob->raw_str = mob->str = fread_int(fl, 0, 100);
			mob->raw_intel = mob->intel = fread_int(fl, 0, 100);
			mob->raw_wis = mob->wis = fread_int(fl, 0, 100);
			mob->raw_dex = mob->dex = fread_int(fl, 0, 100);
			mob->raw_con = mob->con = fread_int(fl, 0, 100);
			fread_int(fl, 0, 100); // junk var in case we add another stat
			// fread_new_newline(fl);
			break;
		case '>':
			ungetc(letter, fl);
			mprog_read_programs(fl, nr, false);
			break;
		case 'Y': // type
			mob->mobdata->mob_flags.type = (mob_type_t)fread_int(fl, mob_type_t::MOB_TYPE_FIRST, mob_type_t::MOB_TYPE_LAST);
			// fread_new_newline(fl);
			break;
		case 'V': // value
			i = fread_int(fl, 0, MAX_MOB_VALUES - 1);
			mob->mobdata->mob_flags.value[i] = fread_int(fl, -1000, 2147483467);
			// fread_new_newline(fl);
			break;
		case 'S':
			break;
		default:
			sprintf(buf, "Mob %s: Invalid additional flag.  (Class, S, etc)", mob->short_desc);
			logentry(buf, 0, DC::LogChannel::LOG_BUG);
			break;
		}
	} while (letter != 'S');

	// fread_new_newline(fl);

	mob->weight = 200;
	mob->height = 198;

	for (i = 0; i < 3; i++)
		GET_COND(mob, i) = -1;

	// TODO - eventually have mob saving throws work by race too, but this should be good for now

	for (i = 0; i <= SAVE_TYPE_MAX; i++)
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

	mob->mobdata->vnum = nr;
	mob->desc = 0;

	return (mob);
}

// we write them recursively so they read in properly
void write_mprog_recur(FILE *fl, QSharedPointer<class MobProgram> mprg, bool mob)
{
	char *mprog_type_to_name(int type);

	if (mprg->next)
		write_mprog_recur(fl, mprg->next, mob);
	if (mob)
		fprintf(fl, ">%s ", mprog_type_to_name(mprg->type));
	else
		fprintf(fl, "\\%s ", mprog_type_to_name(mprg->type));
	if (mprg->arglist)
		string_to_file(fl, mprg->arglist);
	else
		string_to_file(fl, "Saved During Edit");
	if (mprg->comlist)
		string_to_file(fl, mprg->comlist);
	else
		string_to_file(fl, "Saved During Edit");
}

// Write a mob to file
// Assume valid mob, and file open for writing
//
void write_mobile(LegacyFile &lf, Character *mob)
{
	FILE *fl = lf.file_handle_;
	int i = 0;

	fprintf(fl, "#%lu\n", DC::getInstance()->mob_index[mob->mobdata->vnum].vnum);
	string_to_file(fl, mob->getName());
	string_to_file(fl, mob->short_desc);
	string_to_file(fl, mob->long_desc);
	string_to_file(fl, mob->description);

	while (i < ACT_MAX / ASIZE + 1)
	{
		fprintf(fl, "%d ", mob->mobdata->actflags[i]);
		i++;
	}
	fprintf(fl, "-1\n");
	i = 0;

	while (i < AFF_MAX / ASIZE + 1)
	{
		fprintf(fl, "%d ", mob->affected_by[i]);
		i++;
	}
	fprintf(fl, "-1\n");

	fprintf(fl, "%d %d %llu\n"
				"%d %d %dd%d+%d %dd%d+%d\n"
				"%ld %ld\n"
				"%d %d %d %d %d %d\n",
			mob->alignment,
			GET_RACE(mob),
			mob->getLevel(),

			(20 - mob->hitroll),
			(int)(mob->armor / 10),
			GET_MAX_HIT(mob),
			1,
			0,
			mob->mobdata->damnodice,
			mob->mobdata->damsizedice,
			mob->damroll,

			mob->getGold(),
			GET_EXP(mob),

			mob->getPosition(),
			mob->mobdata->default_pos,
			mob->sex,
			mob->immune,
			mob->suscept,
			mob->resist);

	if (mob->c_class)
		fprintf(fl, "C %d\n", mob->c_class);

	if ((mob->raw_str != 11 || mob->raw_dex != 11 || mob->raw_con != 11 ||
		 mob->raw_intel != 11 || mob->raw_wis != 11) &&
		(mob->raw_str != BASE_STAT + mob_race_mod[GET_RACE(mob)][0] ||
		 mob->raw_dex != BASE_STAT + mob_race_mod[GET_RACE(mob)][1] ||
		 mob->raw_con != BASE_STAT + mob_race_mod[GET_RACE(mob)][2] ||
		 mob->raw_intel != BASE_STAT + mob_race_mod[GET_RACE(mob)][3] ||
		 mob->raw_wis != BASE_STAT + mob_race_mod[GET_RACE(mob)][4]))
	{
		fprintf(fl, "T %d %d %d %d %d 0\n", mob->raw_str, mob->raw_intel, mob->raw_wis, mob->raw_dex, mob->raw_con);
	}

	if (DC::getInstance()->mob_index[mob->mobdata->vnum].mobprogs)
	{
		write_mprog_recur(fl, DC::getInstance()->mob_index[mob->mobdata->vnum].mobprogs, true);
		fprintf(fl, "|\n");
	}

	if (mob->mobdata->mob_flags.type > 0)
	{
		fprintf(fl, "Y %d\n", mob->mobdata->mob_flags.type);
		for (uint32_t i = 0; i < MAX_MOB_VALUES; ++i)
		{
			fprintf(fl, "V %d %d\n", i, mob->mobdata->mob_flags.value[i]);
		}
	}

	fprintf(fl, "S\n");
}

// If a mob is set to 0d0 we need to give it hps depending upon it's level
// and class.  And then since it's a mob, a bonus:)
//
void handle_automatic_mob_damdice(Character *mob)
{
	int nodice = 1;
	int sizedice = 1;

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

void handle_automatic_mob_hitpoints(Character *mob)
{
	quint64 base{};

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
void handle_automatic_mob_hitdamroll(Character *mob)
{
	int curhit;

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

void handle_automatic_mob_settings(Character *mob)
{
	extern struct mob_matrix_data mob_matrix[];
	// New matrix is handled here.
	if (ISSET(mob->mobdata->actflags, ACT_NOMATRIX))
		return;
	if (mob->getLevel() > 110)
		return;
	int baselevel = mob->getLevel();
	float alevel = (float)mob->getLevel();

	int percent = number(-3, 3);

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
	if (!c && IS_AFFECTED(mob, AFF_true_SIGHT))
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

	mob->alignment = (int)((float)mob->alignment * (1 + (((float)number(0, 30) - 15) / 100)));

	int temp = mob->immune;
	for (; temp; temp <<= 1)
		if (temp & 1)
			alevel -= 0.5;

	temp = mob->suscept;
	for (; temp; temp <<= 1)
		if (temp & 1)
			alevel += 1;

	baselevel = MAX(alevel > 0 ? (int)alevel : 1, baselevel - 4);

	mob->hitroll = mob_matrix[baselevel].tohit;
	mob->damroll = mob_matrix[baselevel].todam;
	if (ISSET(mob->mobdata->actflags, ACT_BOSS))
		mob->armor = (int)(mob_matrix[baselevel].armor * 1.5);
	else
		mob->armor = mob_matrix[baselevel].armor;
	mob->max_hit = mob->raw_hit = mob->hit = mob_matrix[baselevel].hitpoints + ((mob_matrix[baselevel].hitpoints / 100) * percent);
}

Character *clone_mobile(int nr)
{
	int i;
	Character *mob, *old;

	if (nr < 0)
		return 0;

	mob = new Character;
	auto &free_list = DC::getInstance()->free_list;
	free_list.erase(mob);

	clear_char(mob);
	old = ((Character *)(DC::getInstance()->mob_index[nr].item)); /* cast void pointer */

	*mob = *old;

	mob->mobdata = mobdata_t::create();

	mob->mobdata = old->mobdata;

	for (i = 0; i < MAX_WEAR; i++) /* Initialisering Ok */
		mob->equipment[i] = 0;

	mob->mobdata->vnum = nr;
	mob->desc = 0;
	mob->mobdata->reset = {};

	auto &character_list = DC::getInstance()->character_list;
	character_list.insert(mob);
	mob->next_in_room = 0;

	handle_automatic_mob_settings(mob);
	float mult = 1.0;
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
	mob->max_hit = mob->raw_hit = mob->hit = (int32_t)(mob->max_hit * mult);
	mob->mobdata->damnodice = (int16_t)(mob->mobdata->damnodice * mult);
	mob->mobdata->damsizedice = (int16_t)(mob->mobdata->damsizedice * mult);
	mob->damroll = (int16_t)(mob->damroll * mult);
	mob->hometown = old->in_room;
	return (mob);
}

// add a new item to the index.  To do this, we need to update ALL the
// other items in the game after the one being inserted.  Pain in the
// ass but oh well.  it shouldn't hopefully happen that often.
//
// return index of item on success, -1 on failure
//
auto create_blank_item(vnum_t vnum) -> std::expected<vnum_t, create_error>
{
	class Object *obj;
	class Object *curr;
	int cur_index = 0;

	if (DC::getInstance()->obj_index.contains(vnum))
	{ // item already exists
		return std::unexpected(create_error::entry_exists);
	}

	obj = new Object;
	clear_object(obj);
	obj->name = str_hsh("empty obj");
	obj->short_description = str_hsh("An empty obj");
	obj->description = str_hsh("An empty obj sits here dejectedly.");
	obj->ActionDescription("Fixed.");
	obj->in_room = DC::NOWHERE;
	obj->next_content = 0;
	obj->next_skill = 0;
	obj->table = 0;
	obj->carried_by = 0;
	obj->equipped_by = 0;
	obj->in_obj = 0;
	obj->contains = 0;
	obj->vnum = vnum;
	obj->ex_description = 0;

	// insert
	DC::getInstance()->obj_index[vnum].vnum = vnum;
	DC::getInstance()->obj_index[vnum].qty = 0;
	DC::getInstance()->obj_index[vnum].non_combat_func = 0;
	DC::getInstance()->obj_index[vnum].combat_func = 0;
	DC::getInstance()->obj_index[vnum].item = obj;
	DC::getInstance()->set_zone_modified_obj(vnum);
	return vnum;
}

// add a new mobile to the index.  To do this, we need to update ALL the
// other mobiles in the game after the one being inserted.  Pain in the
// ass but oh well.  it shouldn't hopefully happen that often.
//
// Args:  int nr = virtual number of object (what gods know it as)
//
// return index of item on success, -1 on failure
//  Hack of create_blank_item.. Uriz
int create_blank_mobile(int nr)
{
	Character *mob;
	int cur_index = 0;

	// check if room available in index
	if ((top_of_mobt + 1) >= MAX_INDEX)
		return -1;

	// find how where our index will be
	// yes, i could check if the last mobile is smaller and then do a binary
	// search to do this faster but if everything in life was optimized I wouldn't
	// be playing solitaire at work on a windows machine. -pir
	while (DC::getInstance()->mob_index[cur_index].vnum < nr && cur_index < top_of_mobt + 1)
		cur_index++;

	if (DC::getInstance()->mob_index[cur_index].vnum == nr) // item already exists
		return -1;

	// theoretically if top_of_mobt+1 wasn't initialized properly it could
	// be junk data, which could be == nr, returning -1, but i'm not gonna worry

	// create

#ifdef LEAK_CHECK
	mob = (Character *)calloc(1, sizeof(Character));
#else
	mob = (Character *)dc_alloc(1, sizeof(Character));
#endif

	clear_char(mob);
	reset_char(mob);
	mob->setName("empty mob");
	mob->short_desc = str_hsh("an empty mob");
	mob->long_desc = str_hsh("an empty mob description\r\n");
	mob->description = str_hsh("");
	mob->title = 0;
	mob->fighting = 0;
	mob->player = 0;
	mob->altar = 0;
	mob->desc = 0;
	GET_RAW_DEX(mob) = 11;
	GET_RAW_STR(mob) = 11;
	GET_RAW_INT(mob) = 11;
	GET_RAW_WIS(mob) = 11;
	GET_RAW_CON(mob) = 11;
	mob->height = 198;
	mob->weight = 200;
	mob->mobdata = mobdata_t::create();
	int i;
	for (i = 0; i < ACT_MAX / ASIZE + 1; i++)
		mob->mobdata->actflags[i] = 0;
	for (i = 0; i < AFF_MAX / ASIZE + 1; i++)
		mob->affected_by[i] = 0;
	mob->mobdata->reset = {};
	mob->mobdata->damnodice = 1;
	mob->mobdata->damsizedice = 1;
	mob->mobdata->default_pos = position_t::STANDING;
	mob->mobdata->last_room = 0;
	mob->mobdata->vnum = cur_index;
	mob->setType(Character::Type::NPC);
	mob->misc = 0;

	// shift > items right
	// memmove(&DC::getInstance()->mob_index[cur_index + 1], &DC::getInstance()->mob_index[cur_index], ((top_of_mobt - cur_index + 1) * sizeof(index_data)));
	top_of_mobt++;

	// insert
	DC::getInstance()->mob_index[cur_index].vnum = nr;
	DC::getInstance()->mob_index[cur_index].qty = 0;
	if (DC::getInstance()->mob_non_combat_functions.contains(nr))
	{
		DC::getInstance()->mob_index[cur_index].non_combat_func = DC::getInstance()->mob_non_combat_functions[nr];
	}
	else
	{
		DC::getInstance()->mob_index[cur_index].non_combat_func = nullptr;
	}

	if (DC::getInstance()->mob_combat_functions.contains(nr))
	{
		DC::getInstance()->mob_index[cur_index].combat_func = DC::getInstance()->mob_combat_functions[nr];
	}
	else
	{
		DC::getInstance()->mob_index[cur_index].combat_func = nullptr;
	}

	DC::getInstance()->mob_index[cur_index].item = mob;

	DC::getInstance()->mob_index[cur_index].mobprogs = 0;
	DC::getInstance()->mob_index[cur_index].mobspec = 0;
	DC::getInstance()->mob_index[cur_index].progtypes = 0;

	// update index of all mobiles in game
	const auto &character_list = DC::getInstance()->character_list;
	for_each(character_list.begin(), character_list.end(),
			 [&cur_index](Character *const &curr)
			 {
				 if (IS_NPC(curr))
					 if (curr->mobdata->vnum >= cur_index)
						 curr->mobdata->vnum++;
			 });

	// update index of all the mob prototypes
	for (i = cur_index + 1; i <= top_of_mobt; i++)
		((Character *)DC::getInstance()->mob_index[i].item)->mobdata->vnum++;

	// update obj file indices
	world_file_list_item *wcurr = nullptr;

	wcurr = DC::getInstance()->mob_file_list;
	while (wcurr)
	{
		if (wcurr->firstnum >= cur_index)
			wcurr->firstnum++;

		if (wcurr->lastnum >= cur_index - 1)
			wcurr->lastnum++;

		wcurr = wcurr->next;
	}

	rebuild_mob_rnum_references(cur_index);

	/*
	 Shop fixes follow.
	 */

	//   int i;
	for (auto &shop : DC::getInstance()->shop_index)
	{
		shop.setKeeperRNUM(real_mobile(shop.keeper_vnum()));
	}

	return cur_index;
}

QString qDebugQTextStreamLine(QTextStream &stream, QString message)
{
	assert(stream.status() == QTextStream::Status::Ok);
	auto current_pos = stream.pos();
	auto current_line = stream.readLine();
	assert(stream.status() == QTextStream::Status::Ok);

	if (!message.isEmpty())
	{
		qDebug("%s", qPrintable(QStringLiteral("%1: [%2]").arg(message).arg(current_line)));
	}
	auto ok = stream.seek(current_pos);
	assert(stream.pos() == current_pos);
	assert(stream.status() == QTextStream::Status::Ok);
	if (!ok)
	{
		qFatal("Failed to seek in qDebugQTextStreamLine");
	}
	return current_line;
}

/* read an object from OBJ_FILE */
class Object *read_object(vnum_t vnum, QTextStream &fl, bool ignore)
{
	int loc{}, mod{};

	QString chk;

	if (!vnum)
	{
		return {};
	}

	class Object *obj = new Object;
	clear_object(obj);

	/* *** std::string data *** */
	// read it, add it to the hsh table, free it
	// that way, we only have one copy of it in memory at any time

	obj->name = fread_string(fl, 1);

	qDebug("%s", QStringLiteral("Object name: %1").arg(obj->name).toStdString().c_str());
	obj->short_description = fread_string(fl, 1);
	if (strlen(obj->short_description) >= MAX_OBJ_SDESC_LENGTH)
	{
		logf(IMMORTAL, DC::LogChannel::LOG_BUG, "read_object: vnum %lu short_description too long.", vnum);
	}

	obj->description = fread_string(fl, 1);

	obj->ActionDescription(fread_string(fl, 1));
	fl.skipWhiteSpace();
	if (!obj->ActionDescription().isEmpty() && !obj->ActionDescription()[0].isNull() && (obj->ActionDescription()[0] < ' ' || obj->ActionDescription()[0] > '~'))
	{
		logentry(QStringLiteral("read_object: vnum %1 action description [%2] removed.").arg(vnum).arg(obj->ActionDescription()));
		obj->ActionDescription(QString());
	}
	obj->table = 0;
	DC::getInstance()->currentVNUM(vnum);
	DC::getInstance()->currentName(obj->name);
	DC::getInstance()->currentType("Object");
	obj->obj_flags.type_flag = fread_int<decltype(obj->obj_flags.size)>(fl);
	obj->obj_flags.extra_flags = fread_int<decltype(obj->obj_flags.extra_flags)>(fl);
	obj->obj_flags.wear_flags = fread_int<decltype(obj->obj_flags.wear_flags)>(fl);
	obj->obj_flags.size = fread_int<decltype(obj->obj_flags.size)>(fl);

	obj->obj_flags.value[0] = fread_int<object_value_t>(fl);
	obj->obj_flags.value[1] = fread_int<object_value_t>(fl);
	obj->obj_flags.value[2] = fread_int<object_value_t>(fl);
	obj->obj_flags.value[3] = fread_int<object_value_t>(fl);
	obj->obj_flags.eq_level = fread_int<decltype(obj->obj_flags.eq_level)>(fl, 0, IMPLEMENTER);

	obj->obj_flags.weight = fread_int<decltype(obj->obj_flags.weight)>(fl);
	obj->obj_flags.cost = fread_int<decltype(obj->obj_flags.cost)>(fl);
	obj->obj_flags.more_flags = fread_int<decltype(obj->obj_flags.more_flags)>(fl);
	/* currently not stored in object file */
	obj->obj_flags.timer = 0;

	obj->ex_description = nullptr;
	obj->affected = nullptr;
	obj->num_affects = 0;
	/* *** other flags *** */

	if (vnum == 2866 && !obj->ActionDescription().isEmpty() && obj->ActionDescription()[0] == 'P')
	{
		qDebug("Debug point");
	}

	qDebugQTextStreamLine(fl, "read_object(), before fl >> chk >> Qt::ws");
	fl >> chk >> Qt::ws;
	qDebugQTextStreamLine(fl, "read_object(), after fl >> chk >> Qt::ws");
	qDebug() << "First chk " << chk;
	struct extra_descr_data *new_new_descr{};
	qint64 current_pos{};
	QString current_line{};
	while (!chk.isEmpty() && chk != "S")
	{
		bool ok = false;
		switch (chk[0].toLatin1())
		{
		case 'E':
			qDebugQTextStreamLine(fl, "Type E before first fread_string");
			new_new_descr = new struct extra_descr_data;

			new_new_descr->keyword = fread_string(fl, 1);

			qDebugQTextStreamLine(fl, "Type E before second fread_string");

			new_new_descr->description = fread_string(fl, 1);

			qDebugQTextStreamLine(fl, "Type E after second fread_string");

			new_new_descr->next = obj->ex_description;
			obj->ex_description = new_new_descr;
			break;

		case '\\':
			qDebugQTextStreamLine(fl, "before seek: ");
			ok = fl.seek(fl.pos() - 1);
			if (!ok)
			{
				qFatal("Failed to seek -1 in read_object");
			}

			qDebugQTextStreamLine(fl, "after seek: ");

			mprog_read_programs(fl, vnum, ignore);

			qDebugQTextStreamLine(fl, "after mprog_read_programs seek: ");
			break;

		case 'A':
			// these are only two members of obj_affected_type, so nothing else needs initializing
			loc = fread_int<decltype(loc)>(fl);
			mod = fread_int<decltype(mod)>(fl, -1000, 1000);
			add_obj_affect(obj, loc, mod);
			break;

		default:
			logentry(QStringLiteral("Illegal obj addon flag [%1] in obj [%2].").arg(chk).arg(obj->name), IMPLEMENTER, DC::LogChannel::LOG_BUG);
			break;
		} // switch
		  // read in next flag
		assert(fl.status() == QTextStream::Status::Ok);
		fl >> chk >> Qt::ws;
		assert(fl.status() == QTextStream::Status::Ok);
		qDebug() << "subsequent chk [" << chk << "]";
	}

	obj->in_room = DC::NOWHERE;
	obj->next_skill = 0;
	obj->next_content = 0;
	obj->carried_by = 0;
	obj->equipped_by = 0;
	obj->in_obj = 0;
	obj->contains = 0;
	obj->vnum = vnum;

	// Keys will now save for up to 24 hours. If there are any with
	// ITEM_NOSAVE that flag will be removed.
	if (IS_KEY(obj))
	{
		SET_BIT(obj->obj_flags.more_flags, ITEM_24H_SAVE);
		REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NOSAVE);
	}

	return obj;
}

/* read an object from OBJ_FILE */
class Object *read_object(vnum_t vnum, FILE *fl, bool ignore)
{
	int loc, mod;

	char chk;
	struct extra_descr_data *new_new_descr;

	if (!vnum)
	{
		return {};
	}

	Object *obj = new Object;
	assert(obj);

	clear_object(obj);

	/* *** std::string data *** */
	// read it, add it to the hsh table, free it
	// that way, we only have one copy of it in memory at any time
	obj->name = fread_string(fl, 1);
	char *tmpptr;

	tmpptr = fread_string(fl, 1);

	if (strlen(tmpptr) >= MAX_OBJ_SDESC_LENGTH)
	{
		tmpptr[MAX_OBJ_SDESC_LENGTH - 1] = 0;

		obj->short_description = str_dup(tmpptr);
		free(tmpptr);

		logf(IMMORTAL, DC::LogChannel::LOG_BUG, "read_object: vnum %d short_description too long.", DC::getInstance()->obj_index[vnum].vnum);
	}
	else
	{
		obj->short_description = tmpptr;
	}

	obj->description = fread_string(fl, 1);
	obj->ActionDescription(fread_string(fl, 1));
	if ((!obj->ActionDescription().isEmpty() && (obj->ActionDescription()[0] < ' ' || obj->ActionDescription()[0] > '~')) && !obj->ActionDescription()[0].isNull())
	{
		logf(IMMORTAL, DC::LogChannel::LOG_BUG, "read_object: vnum %d action description [%s] removed.", DC::getInstance()->obj_index[vnum].vnum, obj->ActionDescription().toStdString().c_str());
		obj->ActionDescription(QString());
	}
	obj->table = 0;
	DC::getInstance()->currentVNUM(vnum);
	DC::getInstance()->currentName(obj->name);
	DC::getInstance()->currentType("Object");

	/* *** numeric data *** */

	obj->obj_flags.type_flag = fread_int(fl, -1000, 2147483467);
	obj->obj_flags.extra_flags = fread_bitvector(fl, 0, 2147483467);
	obj->obj_flags.wear_flags = fread_bitvector(fl, 0, 2147483467);
	obj->obj_flags.size = fread_bitvector(fl, 0, 2147483467);

	obj->obj_flags.value[0] = fread_int(fl, -1000, 2147483467);
	obj->obj_flags.value[1] = fread_int(fl, -1000, 2147483467);
	obj->obj_flags.value[2] = fread_int(fl, -1000, 2147483467);
	obj->obj_flags.value[3] = fread_int(fl, -1000, 2147483467);
	obj->obj_flags.eq_level = fread_int(fl, -1000, IMPLEMENTER);
	obj->obj_flags.weight = fread_int(fl, -1000, 2147483467);
	obj->obj_flags.cost = fread_int(fl, -1000, 2147483467);
	obj->obj_flags.more_flags = fread_bitvector(fl, -1000, 2147483467);

	/* currently not stored in object file */
	obj->obj_flags.timer = 0;

	obj->ex_description = nullptr;
	obj->affected = nullptr;
	obj->num_affects = 0;
	/* *** other flags *** */

	fscanf(fl, "%c\n", &chk);
	char log_buf[MAX_STRING_LENGTH] = {};
	while (chk != 'S')
	{
		switch (chk)
		{
		// skip whitespace
		case ' ':
		case '\n':
			break;
		case 'E':
#ifdef LEAK_CHECK
			new_new_descr = (struct extra_descr_data *)calloc(1, sizeof(struct extra_descr_data));
#else
			new_new_descr = (struct extra_descr_data *)dc_alloc(1, sizeof(struct extra_descr_data));
#endif
			new_new_descr->keyword = fread_string(fl, 1);
			new_new_descr->description = fread_string(fl, 1);
			new_new_descr->next = obj->ex_description;
			obj->ex_description = new_new_descr;
			break;

		case '\\':
			ungetc('\\', fl);
			mprog_read_programs(fl, vnum, ignore);
			break;

		case 'A':
			// these are only two members of obj_affected_type, so nothing else needs initializing
			loc = fread_int(fl, -1000, 2147483467);
			try
			{
				mod = fread_int(fl, -1000, 1000);
			}
			catch (error_range_over)
			{
				mod = 1000;
			}
			catch (error_range_under)
			{
				mod = -1000;
			}
			add_obj_affect(obj, loc, mod);
			break;

		default:
			sprintf(log_buf, "Illegal obj addon flag %c in obj %s.", chk, obj->name);
			logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_BUG);
			break;
		} // switch
		  // read in next flag
		fscanf(fl, "%c\n", &chk);
	}

	obj->in_room = DC::NOWHERE;
	obj->next_skill = 0;
	obj->next_content = 0;
	obj->carried_by = 0;
	obj->equipped_by = 0;
	obj->in_obj = 0;
	obj->contains = 0;
	obj->vnum = vnum;

	// Keys will now save for up to 24 hours. If there are any with
	// ITEM_NOSAVE that flag will be removed.
	if (IS_KEY(obj))
	{
		SET_BIT(obj->obj_flags.more_flags, ITEM_24H_SAVE);
		REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NOSAVE);
	}

	return obj;
}

std::ifstream &operator>>(std::ifstream &in, Object *obj)
{
	int loc, mod, nr;

	char chk, c;
	struct extra_descr_data *new_new_descr;

	if (obj == nullptr)
	{
		return in;
	}

	clear_object(obj);
	in >> c;
	if (c == '#')
	{
		in >> nr;
	}
	in >> std::ws;

	obj->name = fread_string(in, true);

	char *tmpptr;
	tmpptr = fread_string(in, true);

	if (strlen(tmpptr) >= MAX_OBJ_SDESC_LENGTH)
	{
		tmpptr[MAX_OBJ_SDESC_LENGTH - 1] = 0;

		obj->short_description = str_dup(tmpptr);
		free(tmpptr);

		logf(IMMORTAL, DC::LogChannel::LOG_BUG, "read_object: vnum unknown short_description too long.");
	}
	else
	{
		obj->short_description = tmpptr;
	}
	obj->description = fread_string(in, 1);
	obj->ActionDescription(fread_string(in, 1));
	obj->table = 0;
	DC::getInstance()->currentVNUM(nr);
	DC::getInstance()->currentName(obj->name);
	DC::getInstance()->currentType("Object");

	// numeric data

	obj->obj_flags.type_flag = fread_int(in, -1000, 2147483467);

	obj->obj_flags.extra_flags = fread_bitvector(in, 0, 2147483467);
	obj->obj_flags.wear_flags = fread_bitvector(in, 0, 2147483467);
	obj->obj_flags.size = fread_bitvector(in, 0, 2147483467);

	obj->obj_flags.value[0] = fread_int(in, -1000, 2147483467);
	obj->obj_flags.value[1] = fread_int(in, -1000, 2147483467);
	obj->obj_flags.value[2] = fread_int(in, -1000, 2147483467);
	obj->obj_flags.value[3] = fread_int(in, -1000, 2147483467);
	obj->obj_flags.eq_level = fread_int(in, -1000, IMPLEMENTER);
	obj->obj_flags.weight = fread_int(in, -1000, 2147483467);
	obj->obj_flags.cost = fread_int(in, -1000, 2147483467);
	obj->obj_flags.more_flags = fread_bitvector(in, -1000, 2147483467);

	// currently not stored in object file
	obj->obj_flags.timer = 0;

	obj->ex_description = nullptr;
	obj->affected = nullptr;
	obj->num_affects = 0;
	// other flags

	in >> chk;

	char log_buf[MAX_STRING_LENGTH] = {};
	while (chk != 'S')
	{
		switch (chk)
		{
		// skip whitespace
		case ' ':
		case '\n':
			break;
		case 'E':
#ifdef LEAK_CHECK
			new_new_descr = (struct extra_descr_data *)calloc(1, sizeof(struct extra_descr_data));
#else
			new_new_descr = (struct extra_descr_data *)dc_alloc(1, sizeof(struct extra_descr_data));
#endif
			new_new_descr->keyword = fread_string(in, 1);
			new_new_descr->description = fread_string(in, 1);
			new_new_descr->next = obj->ex_description;
			obj->ex_description = new_new_descr;
			break;

		case '\\':
			// ungetc( '\\', in );
			// mprog_read_programs( in, nr,ignore );
			break;

		case 'A':
			// these are only two members of obj_affected_type, so nothing else needs initializing
			loc = fread_int(in, -1000, 2147483467);
			mod = fread_int(in, -1000, 1000);
			add_obj_affect(obj, loc, mod);
			break;

		default:
			sprintf(log_buf, "Illegal obj addon flag %c in obj %s.", chk, obj->name);
			logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_BUG);
			break;
		} // switch
		  // read in next flag
		in >> chk;
	}

	obj->in_room = DC::NOWHERE;
	obj->next_skill = 0;
	obj->next_content = 0;
	obj->carried_by = 0;
	obj->equipped_by = 0;
	obj->in_obj = 0;
	obj->contains = 0;
	obj->vnum = 0;

	return in;
}

// write an object to file
// This assumes that the object is valid, and the file is open for writing
//
void write_object(LegacyFile &lf, Object *obj)
{
	if (!obj)
		return;
	FILE *fl = lf.file_handle_;
	struct extra_descr_data *currdesc;

	fprintf(fl, "#%lu\n", obj->vnum);
	string_to_file(fl, obj->name);
	string_to_file(fl, obj->short_description);
	string_to_file(fl, obj->description);
	string_to_file(fl, obj->ActionDescription());

	fprintf(fl, "%d %d %d %d\n"
				"%d %d %d %d %llu\n"
				"%d %d %d\n",
			obj->obj_flags.type_flag,
			obj->obj_flags.extra_flags,
			obj->obj_flags.wear_flags,
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
		fprintf(fl, "E\n");
		string_to_file(fl, currdesc->keyword);
		string_to_file(fl, currdesc->description);
		currdesc = currdesc->next;
	}

	for (int i = 0; i < obj->num_affects; i++)
		fprintf(fl, "A\n"
					"%d %d\n",
				obj->affected[i].location,
				obj->affected[i].modifier);

	if (DC::getInstance()->obj_index[obj->vnum].mobprogs)
	{
		write_mprog_recur(fl, DC::getInstance()->obj_index[obj->vnum].mobprogs, false);
		fprintf(fl, "|\n");
	}

	fprintf(fl, "S\n");
}

std::ofstream &operator<<(std::ofstream &out, Object *obj)
{
	out << "#" << obj->vnum << "\n";
	string_to_file(out, obj->name);
	string_to_file(out, obj->short_description);
	string_to_file(out, obj->description);
	string_to_file(out, obj->ActionDescription());

	out << int(obj->obj_flags.type_flag) << " "
		<< obj->obj_flags.extra_flags << " "
		<< obj->obj_flags.wear_flags << " "
		<< obj->obj_flags.size << "\n";

	out << obj->obj_flags.value[0] << " "
		<< obj->obj_flags.value[1] << " "
		<< obj->obj_flags.value[2] << " "
		<< obj->obj_flags.value[3] << " "
		<< obj->obj_flags.eq_level << "\n";

	out << obj->obj_flags.weight << " "
		<< obj->obj_flags.cost << " "
		<< obj->obj_flags.more_flags << "\n";

	extra_descr_data *currdesc = obj->ex_description;
	while (currdesc)
	{
		out << "E\n";
		string_to_file(out, currdesc->keyword);
		string_to_file(out, currdesc->description);
		currdesc = currdesc->next;
	}

	for (int i = 0; i < obj->num_affects; i++)
	{
		out << "A\n";
		out << obj->affected[i].location << " "
			<< obj->affected[i].modifier << "\n";
	}

	if (DC::getInstance()->obj_index[obj->vnum].mobprogs)
	{
		write_mprog_recur(out, DC::getInstance()->obj_index[obj->vnum].mobprogs, false);
		out << "|\n";
	}

	out << "S\n";

	return out;
}

std::string quotequotes(std::string &s1);

std::string quotequotes(const char *str)
{
	std::string s1(str);

	return quotequotes(s1);
}

std::string quotequotes(std::string &s1)
{
	size_t pos = s1.find("\"");
	while (pos != std::string::npos)
	{
		s1.insert(pos, 1, '\"');
		pos = s1.find('\"', pos + 2);
	}

	return s1;
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

std::string lf_to_crlf(std::string &s1)
{
	size_t pos = s1.find('\n'); // @suppress("Ambiguous problem")
	while (pos != std::string::npos)
	{
		s1.insert(pos, 1, '\r');
		pos = s1.find('\n', pos + 2);
	}

	return s1;
}

void write_bitvector_csv(uint32_t vector, QStringList names, std::ofstream &fout)
{

	for (uint32_t nr = 0; nr < names.size(); nr++)
	{
		if (isSet(1, vector))
		{
			fout << names[nr].toStdString();
		}

		fout << ",";
		vector >>= 1;
	}

	return;
}

void write_bitvector_csv(uint32_t vector, const char *const *array, std::ofstream &fout)
{
	int nr = 0;
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

	return;
}

void write_object_csv(Object *obj, std::ofstream &fout)
{
	try
	{
		fout << obj->vnum << ",";
		fout << "\"" << obj->name << "\",";
		fout << "\"" << quotequotes(obj->short_description) << "\",";
		fout << "\"" << quotequotes(obj->description) << "\",";
		fout << "\"" << quotequotes(obj->ActionDescription().toStdString().c_str()) << "\",";
		fout << item_types[obj->obj_flags.type_flag].toStdString() << ",";
		fout << obj->obj_flags.size << ",";
		fout << obj->obj_flags.value[0] << ",";
		fout << obj->obj_flags.value[1] << ",";
		fout << obj->obj_flags.value[2] << ",";
		fout << obj->obj_flags.value[3] << ",";
		fout << obj->obj_flags.eq_level << ",";
		fout << obj->obj_flags.weight << ",";
		fout << obj->obj_flags.cost << ",";

		write_bitvector_csv(obj->obj_flags.wear_flags, Object::wear_bits, fout);
		write_bitvector_csv(obj->obj_flags.extra_flags, Object::extra_bits, fout);
		write_bitvector_csv(obj->obj_flags.more_flags, Object::more_obj_bits, fout);

		char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
		for (int i = 0; i < obj->num_affects; i++)
		{
			if (obj->affected[i].location < 1000)
				sprinttype(obj->affected[i].location, apply_types, buf2);
			else if (!get_skill_name(obj->affected[i].location / 1000).isEmpty())
				strncpy(buf2, get_skill_name(obj->affected[i].location / 1000).toStdString().c_str(), sizeof(buf2));
			else
				strcpy(buf2, "Invalid");

			sprintf(buf, "%s by %d", buf2, obj->affected[i].modifier);
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

bool has_random(Object *obj)
{

	return ((DC::getInstance()->obj_index[obj->vnum].progtypes & RAND_PROG) || (DC::getInstance()->obj_index[obj->vnum].progtypes & ARAND_PROG));
}

/* clone an object from DC::getInstance()->obj_index */
Object *DC::clone_object(vnum_t vnum)
{
	class Object *obj, *old;
	struct extra_descr_data *new_new_descr, *descr;

	if (!obj_index.contains(vnum))
		return {};

	obj = new Object;
	clear_object(obj);
	old = obj_index[vnum].item;

	if (old != 0)
	{
		*obj = *old;
	}
	else
	{
		qWarning("%s", qUtf8Printable(QStringLiteral("clone_object(%1): Obj not found in obj_index.\n").arg(vnum)));
		dc_free(obj);
		return nullptr;
	}

	/* *** extra descriptions *** */
	obj->ex_description = 0;
	for (descr = old->ex_description; descr; descr = descr->next)
	{
#ifdef LEAK_CHECK
		new_new_descr = (struct extra_descr_data *)
			calloc(1, sizeof(struct extra_descr_data));
#else
		new_new_descr = (struct extra_descr_data *)
			dc_alloc(1, sizeof(struct extra_descr_data));
#endif
		new_new_descr->keyword = str_hsh(descr->keyword);
		new_new_descr->description = str_hsh(descr->description);
		new_new_descr->next = obj->ex_description;
		obj->ex_description = new_new_descr;
	}

	obj->affected = (obj_affected_type *)calloc(obj->num_affects, sizeof(obj_affected_type));
	for (int i = 0; i < obj->num_affects; i++)
	{
		obj->affected[i].location = old->affected[i].location;
		obj->affected[i].modifier = old->affected[i].modifier;
	}
	obj->table = 0;
	obj->next_skill = 0;
	obj->next_content = 0;
	obj->next = object_list;
	object_list = obj;
	obj_index[vnum].qty++;
	obj->save_expiration = 0;
	obj->no_sell_expiration = 0;

	if (obj_index[obj->vnum].non_combat_func ||
		obj->obj_flags.type_flag == ITEM_MEGAPHONE ||
		has_random(obj))
	{
		active_obj_list.insert(obj);
	}
	return obj;
}

void randomize_object_affects(Object *obj)
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

	for (int i = 0; i < obj->num_affects; i++)
	{
		switch (obj->affected[i].location)
		{
		case APPLY_STR:
		case APPLY_DEX:
		case APPLY_INT:
		case APPLY_WIS:
		case APPLY_CON:
			if (number(0, 1) == 1)
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
		case APPLY_SONG_DAMAGE:	 // song mitigation
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

void randomize_object(Object *obj)
{
	if (obj == nullptr)
	{
		return;
	}

	// NO_CUSTOM, QUEST or SPECIAL ("godload") items cannot be randomized
	if (isSet(obj->obj_flags.more_flags, ITEM_NO_CUSTOM) || isSet(obj->obj_flags.extra_flags, ITEM_QUEST) || isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
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

void zone_update(void)
{
	auto &zones = DC::getInstance()->zones;
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

	DC::getInstance()->removeDead();
}

uint64_t countMobsInRoom(uint64_t vnum, room_t room_id)
{
	uint64_t count = {};
	for (auto ch = DC::getInstance()->world[room_id].people; ch != nullptr; ch = ch->next_in_room)
	{
		if (ch->mobdata && DC::getInstance()->mob_index[ch->mobdata->vnum].vnum == vnum)
		{
			count++;
		}
	}
	return count;
}

uint64_t countMobsInWorld(uint64_t vnum)
{
	uint64_t count = {};
	for (const auto ch : DC::getInstance()->character_list)
	{
		if (ch->mobdata && DC::getInstance()->mob_index[ch->mobdata->vnum].vnum == vnum && ch->in_room != DC::NOWHERE)
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
	rnum_t rnum{};
	vnum_t vnum{};
	int cmd_no, last_cmd, last_mob, last_obj, last_percent;
	Character *mob = nullptr;
	class Object *obj, *obj_to;
	last_cmd = last_mob = last_obj = last_percent = -1;

	char buf[MAX_STRING_LENGTH];
	char log_buf[MAX_STRING_LENGTH] = {};

	if (died_this_tick == 0 && isEmpty())
	{
		repops_without_deaths++;
	}
	else
	{
		repops_without_deaths = 0;
	}

	// reset number of mobs that have died this tick to 0
	died_this_tick = 0;
	num_mob_on_repop = 0;
	// find last command in zone

	for (cmd_no = 0; cmd_no < cmd.size(); cmd_no++)
	{
		if (cmd_no < 0 || cmd_no > cmd.size())
		{
			sprintf(buf,
					"Trapped zone error, Command is null, zone: %lu cmd_no: %d",
					id_, cmd_no);
			logentry(buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
			break;
		}
		if (cmd[cmd_no]->active == 0)
			continue;

		cmd[cmd_no]->last = time(nullptr);
		cmd[cmd_no]->attempts++;
		if (cmd[cmd_no]->if_flag == 0 ||									// always
			(last_cmd == 1 && cmd[cmd_no]->if_flag == 1) ||					// if last command true
			(last_cmd == 0 && cmd[cmd_no]->if_flag == 2) ||					// if last command false
			(reset_type == ResetType::full && cmd[cmd_no]->if_flag == 3) || // full reset (onboot)
			(last_mob == 1 && cmd[cmd_no]->if_flag == 4) ||					// if-last-mob-true
			(last_mob == 0 && cmd[cmd_no]->if_flag == 5) ||					// if-last-mob-false
			(last_obj == 1 && cmd[cmd_no]->if_flag == 6) ||					// if-last-obj-true
			(last_obj == 0 && cmd[cmd_no]->if_flag == 7) ||					// if-last-obj-false
			(last_percent == 1 && cmd[cmd_no]->if_flag == 8) ||				// if-last-percent-true
			(last_percent == 0 && cmd[cmd_no]->if_flag == 9)				// if-last-percent-false
		)
		{
			cmd[cmd_no]->lastSuccess = cmd[cmd_no]->last;
			cmd[cmd_no]->successes++;
			switch (cmd[cmd_no]->command)
			{

			case 'M': /* read a mobile */
				if ((cmd[cmd_no]->arg2 == -1 || cmd[cmd_no]->lastPop == 0) && countMobsInWorld(DC::getInstance()->mob_index[cmd[cmd_no]->arg1].vnum) < cmd[cmd_no]->arg2 && (mob = clone_mobile(cmd[cmd_no]->arg1)))
				{
					char_to_room(mob, cmd[cmd_no]->arg3);
					cmd[cmd_no]->lastPop = mob;
					mob->mobdata->reset = cmd[cmd_no];
					GET_HOME(mob) = DC::getInstance()->world[cmd[cmd_no]->arg3].number;
					num_mob_on_repop++;
					last_cmd = 1;
					last_mob = 1;
					selfpurge = false;
					mprog_load_trigger(mob);
					if (selfpurge)
					{
						mob = nullptr;
						last_mob = 0;
						last_cmd = 0;
					}
				}
				else
				{
					last_cmd = 0;
					last_mob = 0;
				}
				break;

			case 'O': /* Load object on the ground */
				if (cmd[cmd_no]->arg2 == -1 || DC::getInstance()->obj_index[cmd[cmd_no]->arg1].qty < cmd[cmd_no]->arg2)
				{
					if (cmd[cmd_no]->arg3 >= 0)
					{
						if (!get_obj_in_list_num(cmd[cmd_no]->arg1, DC::getInstance()->world[cmd[cmd_no]->arg3].contents) &&
							(obj = DC::getInstance()->clone_object(cmd[cmd_no]->arg1)))
						{
							obj_to_room(obj, cmd[cmd_no]->arg3);
							last_cmd = 1;
							last_obj = 1;
						}
						else
						{
							last_cmd = 0;
							last_obj = 0;
						}
					}
					else
					{
						DC::config &cf = DC::getInstance()->cf;

						if (cf.test_world == false && cf.test_mobs == false && cf.test_objs == false)
						{
							sprintf(buf,
									"Obj %lu loaded to DC::NOWHERE. Zone %lu Cmd %d",
									DC::getInstance()->obj_index[cmd[cmd_no]->arg1].vnum, id_, cmd_no);
							logentry(buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
						}
						last_cmd = 0;
						last_obj = 0;
					}
				}
				else
				{
					last_cmd = 0;
					last_obj = 0;
				}
				break;

			case 'P': /* object to object */

				if (cmd[cmd_no]->arg2 == -1 || DC::getInstance()->obj_index[cmd[cmd_no]->arg1].qty < cmd[cmd_no]->arg2)
				{
					obj_to = 0;
					obj = 0;
					if ((obj_to = get_obj_num(cmd[cmd_no]->arg3)) && (obj = DC::getInstance()->clone_object(cmd[cmd_no]->arg1)))
						obj_to_obj(obj, obj_to);
					else
						logf(
							IMMORTAL,
							DC::LogChannel::LOG_WORLD,
							"Null container obj in P command Zone: %d, Cmd: %d",
							id_, cmd_no);

					last_cmd = 1;
					last_obj = 1;
				}
				else
				{
					last_cmd = 0;
					last_obj = 0;
				}
				break;

			case 'G': /* obj_to_char */
				if (mob == nullptr)
				{
					sprintf(buf, "Null mob in G, reseting zone %lu cmd %d", id_, cmd_no + 1);
					logentry(buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
					last_cmd = 0;
					last_obj = 0;
					break;
				}
				// Never load the same totem as long as it exists in the world
				if (reinterpret_cast<Object *>(DC::getInstance()->obj_index[cmd[cmd_no]->arg1].item)->isTotem() &&
					cmd[cmd_no]->arg2 != -1 &&
					DC::getInstance()->obj_index[cmd[cmd_no]->arg1].qty >= cmd[cmd_no]->arg2)
				{
					last_cmd = 0;
					last_obj = 0;
					break;
				}
				if ((cmd[cmd_no]->arg2 == -1 || DC::getInstance()->obj_index[cmd[cmd_no]->arg1].qty < cmd[cmd_no]->arg2 || number(0, 1)) && (obj = DC::getInstance()->clone_object(cmd[cmd_no]->arg1)))
				{
					obj_to_char(obj, mob);
					last_cmd = 1;
					last_obj = 0;
				}
				else
				{
					last_cmd = 0;
					last_obj = 0;
				}
				break;

			case '%': /* percent chance of next command happening */
				// We can't send a number less than one to number() otherwise a debug coredump occurs
				if (cmd[cmd_no]->arg2 < 1)
				{
					logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Zone %d, line %d: % arg1: %d arg2: %d - Error: arg2 < 1", id_, cmd_no, cmd[cmd_no]->arg1, cmd[cmd_no]->arg2);
					last_cmd = 0;
					last_percent = 0;
				}
				else
				{
					if (number(1, cmd[cmd_no]->arg2) <= cmd[cmd_no]->arg1)
					{
						cmd[cmd_no]->last = time(nullptr);
						last_percent = 1;
						last_cmd = 1;
					}
					else
					{
						last_cmd = 0;
						last_percent = 0;
					}
				}
				break;

			case 'E': /* object to equipment list */
				if (mob == nullptr)
				{
					sprintf(buf, "Null mob in E reseting zone %lu cmd %d", id_, cmd_no + 1);
					logentry(buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
					last_cmd = 0;
					last_obj = 0;
					break;
				}
				// Never load the same totem as long as it exists in the world
				vnum = cmd[cmd_no]->arg1;
				if (DC::getInstance()->obj_index.contains(vnum) &&
					DC::getInstance()->obj_index[vnum].item->isTotem() &&
					cmd[cmd_no]->arg2 != -1 &&
					DC::getInstance()->obj_index[vnum].qty >= cmd[cmd_no]->arg2)
				{
					last_cmd = 0;
					last_obj = 0;
					break;
				}
				if ((obj = DC::getInstance()->clone_object(cmd[cmd_no]->arg1)))
				{
					randomize_object(obj);

					// Check if mob and object are safe to be equipped
					// Some of these checks are redundant compared to equip_char() but
					// we want to know if the position is filled already before running equip_char()
					// so we don't see unnecessary errors when a zone reset tries to reequip a mob
					if (mob == nullptr)
					{
						logentry(QStringLiteral("nullptr mob in reset_zone()!"), ANGEL, DC::LogChannel::LOG_BUG);
					}
					else if (cmd[cmd_no]->arg3 < 0 || cmd[cmd_no]->arg3 >= MAX_WEAR)
					{
						logentry(QStringLiteral("Invalid eq position in Zone::reset()!"), ANGEL, DC::LogChannel::LOG_BUG);
					}
					else if (mob->equipment[cmd[cmd_no]->arg3] == 0)
					{
						if (!equip_char(mob, obj, cmd[cmd_no]->arg3))
						{
							sprintf(buf, "Bad equip_char zone %lu cmd %d", id_, cmd_no + 1);
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
					last_cmd = 0;
					last_obj = 0;
				}
				break;

			case 'D': /* set state of door */
				if (cmd[cmd_no]->arg1 < 0 || cmd[cmd_no]->arg1 > DC::getInstance()->top_of_world)
				{
					sprintf(log_buf, "Illegal room number Z: %lu cmd %d", id_, cmd_no + 1);
					logentry(log_buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
					break;
				}
				if (cmd[cmd_no]->arg2 < 0 || cmd[cmd_no]->arg2 >= 6)
				{
					sprintf(log_buf, "Illegal direction %d doesn't exist Z: %lu cmd %d", cmd[cmd_no]->arg2, id_, cmd_no + 1);
					logentry(log_buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
					break;
				}
				if (!DC::getInstance()->rooms.contains(cmd[cmd_no]->arg1))
				{
					sprintf(log_buf, "Room %d doesn't exist Z: %lu cmd %d", cmd[cmd_no]->arg1, id_, cmd_no + 1);
					logentry(log_buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
					break;
				}

				if (DC::getInstance()->world[cmd[cmd_no]->arg1].dir_option[cmd[cmd_no]->arg2] == 0)
				{
					sprintf(
						log_buf,
						"Attempt to reset direction %d on room %d that doesn't exist Z: %lu cmd %d",
						cmd[cmd_no]->arg2, DC::getInstance()->world[cmd[cmd_no]->arg1].number, id_, cmd_no);
					logentry(log_buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
					break;
				}
				switch (cmd[cmd_no]->arg3)
				{
				case 0:
					REMOVE_BIT(
						DC::getInstance()->world[cmd[cmd_no]->arg1].dir_option[cmd[cmd_no]->arg2]->exit_info,
						EX_BROKEN);
					REMOVE_BIT(
						DC::getInstance()->world[cmd[cmd_no]->arg1].dir_option[cmd[cmd_no]->arg2]->exit_info,
						EX_LOCKED);
					REMOVE_BIT(
						DC::getInstance()->world[cmd[cmd_no]->arg1].dir_option[cmd[cmd_no]->arg2]->exit_info,
						EX_CLOSED);
					break;
				case 1:
					REMOVE_BIT(
						DC::getInstance()->world[cmd[cmd_no]->arg1].dir_option[cmd[cmd_no]->arg2]->exit_info,
						EX_BROKEN);
					SET_BIT(DC::getInstance()->world[cmd[cmd_no]->arg1].dir_option[cmd[cmd_no]->arg2]->exit_info,
							EX_CLOSED);
					REMOVE_BIT(
						DC::getInstance()->world[cmd[cmd_no]->arg1].dir_option[cmd[cmd_no]->arg2]->exit_info,
						EX_LOCKED);
					break;
				case 2:
					REMOVE_BIT(
						DC::getInstance()->world[cmd[cmd_no]->arg1].dir_option[cmd[cmd_no]->arg2]->exit_info,
						EX_BROKEN);
					SET_BIT(DC::getInstance()->world[cmd[cmd_no]->arg1].dir_option[cmd[cmd_no]->arg2]->exit_info,
							EX_LOCKED);
					SET_BIT(DC::getInstance()->world[cmd[cmd_no]->arg1].dir_option[cmd[cmd_no]->arg2]->exit_info,
							EX_CLOSED);
					break;
				}
				last_cmd = 1;
				break;

			case 'X':
				switch (cmd[cmd_no]->arg1)
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
				cmd_no += cmd[cmd_no]->arg1;
				break;

			case '*': // ignore *
			case 'J': // ignore J
				break;

			default:
				sprintf(log_buf, "UNKNOWN COMMAND!!! ZONE %lu cmd %d: '%c' Skipping .", id_, cmd_no + 1, cmd[cmd_no]->command);
				logentry(log_buf, IMMORTAL, DC::LogChannel::LOG_WORLD);
				age = 0;
				return;
				break;
			}
		}
		else
		{
			switch (cmd[cmd_no]->command)
			{

			case 'M':
				last_mob = 0;
				last_cmd = 0;
				break;

			case 'O':
			case 'G':
			case 'P':
			case 'E':
				last_obj = 0;
				last_cmd = 0;
				break;
			case '%':
				last_percent = 0;
				last_cmd = 0;
				break;
			case 'D':
				last_cmd = 0;
				break;
			case 'X':
			case 'K':
				last_cmd = 0;
				break;
			default:
				break;
			}
		}
	}

	age = 0;

	if (repops_without_deaths > 2 && repops_without_deaths < 7 && repops_with_bonus < 4)
	{
		repops_with_bonus++;

		const auto &character_list = DC::getInstance()->character_list;
		for (const auto &tmp_victim : character_list)
		{
			if (tmp_victim->in_room == DC::NOWHERE)
			{
				continue;
			}
			if (IS_NPC(tmp_victim) && !ISSET(tmp_victim->mobdata->actflags, ACT_NO_GOLD_BONUS) && DC::getInstance()->world[tmp_victim->in_room].zone == id_)
			{
				tmp_victim->multiplyGold(1.10);
				tmp_victim->exp *= 1.10;
			}
		}
	}
}

bool Zone::isEmpty(void)
{
	class Connection *i;

	for (i = DC::getInstance()->descriptor_list; i; i = i->next)
		if (STATE(i) == Connection::states::PLAYING && i->character && DC::getInstance()->world[i->character->in_room].zone == id_)
			return false;

	return true;
}

QString fread_qstring(QTextStream &stream, bool *ok)
{
	assert(stream.status() == QTextStream::Status::Ok);
	QString buffer;

	qDebugQTextStreamLine(stream, "before fread_string()");
	do
	{
		QString line = stream.readLine();
		if (line.endsWith('~'))
		{
			line.remove(line.length() - 1, 1);
			if (ok)
			{
				*ok = true;
			}
			assert(stream.status() == QTextStream::Status::Ok);
			qDebug() << "fread_string returning" << buffer + line << (buffer + line).length();
			qDebugQTextStreamLine(stream, "after fread_string()");
			return buffer + line;
		}
		else
		{
			buffer += line + '\n';
		}

	} while (!stream.atEnd());

	if (ok)
	{
		*ok = false;
	}
	assert(stream.status() == QTextStream::Status::Ok);
	qDebug() << "fread_string returning" << buffer << buffer.length();
	qDebugQTextStreamLine(stream, "after fread_string()");
	return buffer;
}

char *fread_string(QTextStream &in, bool hasher, bool *ok)
{
	QString buffer = fread_qstring(in, ok);

	if (hasher)
	{
		return str_hsh(qUtf8Printable(buffer));
	}
	else
	{
		return strdup(qUtf8Printable(buffer));
	}
}

/************************************************************************
 *  procs of a (more or less) general utility nature         *
 ********************************************************************** */
char *fread_string(std::ifstream &in, int hasher)
{
	char buffer[MAX_STRING_LENGTH];

	// Save original exception mask so we can restore it later
	std::ios_base::iostate orig_exceptions = in.exceptions();
	in.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
	try
	{
		in.getline(buffer, MAX_STRING_LENGTH, '~');
		in >> std::ws;
	}
	catch (...)
	{
		logf(IMMORTAL, DC::LogChannel::LOG_BUG, "fread_string() error reading");
		throw;
	}
	in.exceptions(orig_exceptions);

	std::string orig_str(buffer);
	// Change \n into \r\n
	std::string swapstr = lf_to_crlf(orig_str);

	char *retval;
	if (hasher)
	{
		retval = str_hsh(swapstr.c_str());
	}
	else
	{
		retval = new char[swapstr.length()];
		memcpy(retval, swapstr.c_str(), swapstr.length());
	}
	return retval;
}

QString fread_qstring(FILE *stream, bool *ok)
{
	char *lineptr = nullptr;
	size_t n = 0;

	ssize_t bytes_read = getdelim(&lineptr, &n, '~', stream);
	if (lineptr && bytes_read && lineptr[bytes_read - 1] == '~')
	{
		lineptr[bytes_read - 1] = 0;
	}
	fseek(stream, 1, SEEK_CUR);
	// qDebug("%d [%s]", bytes_read, lineptr);

	return lf_to_crlf(QString(lineptr));
}

/* read and allocate space for a '~'-terminated std::string from a given file */
char *fread_string(FILE *fl, int hasher)
{
	char buf[MAX_STRING_LENGTH];
	char *pAlloc;
	char *pBufLast;
	char *temp;

	for (pBufLast = buf; pBufLast < &buf[MAX_STRING_LENGTH - 2];)
	{
		*pBufLast = getc(fl);
		switch (*pBufLast)
		{
		default:
			pBufLast++;
			break;

		case '\n':
			while (pBufLast > buf && isspace(pBufLast[-1]))
				pBufLast--;
			*pBufLast++ = '\r';
			*pBufLast++ = '\n';
			break;

		case '~':
			getc(fl);
			if (pBufLast == buf)
			{
				if (hasher)
				{
					pAlloc = str_hsh("");
				}
				else
				{
					pAlloc = str_dup("");
				}
			}
			else if (hasher)
			{
				*pBufLast++ = '\0';
#ifdef LEAK_CHECK
				pAlloc = (char *)calloc(pBufLast - buf, sizeof(char));
#else
				pAlloc = (char *)dc_alloc(pBufLast - buf, sizeof(char));
#endif
				memcpy(pAlloc, buf, pBufLast - buf);
				temp = str_hsh(pAlloc);
				dc_free(pAlloc);
				pAlloc = temp;
			}
			else
			{
				*pBufLast++ = '\0';
#ifdef LEAK_CHECK
				pAlloc = (char *)calloc(pBufLast - buf, sizeof(char));
#else
				pAlloc = (char *)dc_alloc(pBufLast - buf, sizeof(char));
#endif
				memcpy(pAlloc, buf, pBufLast - buf);
			}
			return pAlloc;
			// end of ~ case
		case (char)EOF:
			perror("fread_string: EOF");
			throw error_eof();
			break;
		} // switch
	} // for

	perror("fread_string: std::string too long");
	abort();
	return (nullptr);
}

QString fread_word(QTextStream &fl)
{
	QString buffer;
	fl >> buffer;
	return buffer;
}

/* read and allocate space for a whitespace-terminated std::string from a given file */
char *fread_word(FILE *fl, int hasher)
{
	char buf[MAX_STRING_LENGTH];
	char *pAlloc;
	char *pBufLast;
	char *temp;
	char tmp;
	while ((tmp = getc(fl)) == ' ')
		;
	ungetc(tmp, fl);

	for (pBufLast = buf; pBufLast < &buf[sizeof(buf) - 2];)
	{
		*pBufLast = getc(fl);
		switch (*pBufLast)
		{
		default:
			pBufLast++;
			break;

		case (char)EOF:
			perror("fread_word: EOF");
			abort();
			break;

		case '\t':
		case '\n':
		case ' ':
			if (pBufLast == buf)
			{
				if (hasher)
					pAlloc = str_hsh("");
				else
					pAlloc = str_dup("");
			}
			else if (hasher)
			{
				*pBufLast++ = '\0';
#ifdef LEAK_CHECK
				pAlloc = (char *)calloc(pBufLast - buf, sizeof(char));
#else
				pAlloc = (char *)dc_alloc(pBufLast - buf, sizeof(char));
#endif
				memcpy(pAlloc, buf, pBufLast - buf);
				temp = str_hsh(pAlloc);
				dc_free(pAlloc);
				pAlloc = temp;
			}
			else
			{
				*pBufLast++ = '\0';
#ifdef LEAK_CHECK
				pAlloc = (char *)calloc(pBufLast - buf, sizeof(char));
#else
				pAlloc = (char *)dc_alloc(pBufLast - buf, sizeof(char));
#endif
				memcpy(pAlloc, buf, pBufLast - buf);
			}
			return pAlloc;
			// end of ~ case
		} // switch
	} // for

	perror("fread_word: std::string too long");
	abort();
	return (nullptr);
}

// This is here to allow us to read a bitvector in as either a number
// or as a std::string of characters.  ie, 4, and c are the same.
// 5 (1+4) would be the same at 'ac'

int fread_bitvector(FILE *fl, int32_t beg_range, int32_t end_range)
{
	char buf[200];
	int ch;
	int32_t i = 0;

	// eat space till we hit the next one
	while ((ch = getc(fl)))
	{
		if (ch == EOF)
		{
			qWarning() << QStringLiteral("Reading %1").arg(DC::getInstance()->current());
			perror("fread_bitvector: premature EOF");
			abort();
		}

		if (ch != ' ' && ch != '\n') /* eat the white space */
			break;
	}

	// check if we're dealing with numbers, or letters
	if (isdigit(ch) || ch == '-')
	{
		// It's a digit, so put the char back and let fread_int handle it
		ungetc(ch, fl);
		return fread_int(fl, beg_range, end_range);
	}

	// we're dealing with letters now
	for (;;)
	{
		if (ch == EOF)
		{
			logbug(QStringLiteral("Reading %1").arg(DC::getInstance()->current()));
			perror("fread_bitvector: premature EOF");
			abort();
		}
		if (ch >= 'a' && ch <= 'z')
		{
			i += 1 << (ch - 'a');
		}
		else if (ch >= 'A' && ch <= 'G')
		{
			i += 1 << (26 + (ch - 'A'));
		}
		else if (ch == ' ' || ch == '\n')
		{
			// we hit the end.  Return current i.
			return i;
		}
		else
		{
			logmisc(QStringLiteral("Reading %1 (%2)").arg(DC::getInstance()->current()).arg(ch));
			perror("fread_bitvector: illegal character");
			abort();
		}

		// if we hit here, we had a valid character.  read the next one.
		ch = getc(fl);

	} // for ;;

	perror("fread_bitvector: something went wrong");
	abort();
	return (0);
}

int fread_bitvector(std::ifstream &in, int32_t beg_range, int32_t end_range)
{
	int ch;
	int32_t i = 0;

	// Save original exception mask so we can restore it later
	std::ios_base::iostate orig_exceptions = in.exceptions();
	in.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
	try
	{
		// eat space till we hit the next one
		while ((ch = in.get()))
		{
			if (ch != ' ' && ch != '\n') /* eat the white space */
				break;
		}

		// check if we're dealing with numbers, or letters
		if (isdigit(ch) || ch == '-')
		{
			// It's a digit, so put the char back and let fread_int handle it
			in.unget();
			int n = fread_int(in, beg_range, end_range);
			in.exceptions(orig_exceptions);
			return n;
		}

		// we're dealing with letters now
		for (;;)
		{
			if (ch >= 'a' && ch <= 'z')
			{
				i += 1 << (ch - 'a');
			}
			else if (ch >= 'A' && ch <= 'G')
			{
				i += 1 << (26 + (ch - 'A'));
			}
			else if (ch == ' ' || ch == '\n')
			{
				// we hit the end.  Return current i.
				in.exceptions(orig_exceptions);
				return i;
			}
			else
			{
				logbug("fread_bitvector: illegal character");
				logbug(QStringLiteral("Reading %1").arg(DC::getInstance()->current()));
				throw;
			}

			// if we hit here, we had a valid character.  read the next one.
			ch = in.get();

		} // for ;;
	}
	catch (...)
	{
		logbug("fread_bitvector: unknown error");
		logbug(QStringLiteral("Reading %1").arg(DC::getInstance()->current()));
		throw;
	}
	in.exceptions(orig_exceptions);

	return 0;
}

uint64_t fread_uint(FILE *fl, uint64_t beg_range, uint64_t end_range)
{
	char buf[MAX_STRING_LENGTH];
	char *pBufLast;
	int ch;
	uint64_t i;

	while ((ch = getc(fl)))
	{
		if (ch == EOF)
		{
			logbug(QStringLiteral("Reading %1").arg(DC::getInstance()->current()));
			perror("fread_int: premature EOF");
			abort();
		}

		if (ch != ' ' && ch != '\n') /* eat the white space */
			break;
	}

	pBufLast = buf;

	if (ch == '-' && beg_range >= 0)
	{
		while (isdigit(getc(fl)))
		{
		}
		throw error_negative_int();
	}
	else if (ch == '-')
	{
		*pBufLast = ch;
		pBufLast++;
		ch = getc(fl);
	}

	*pBufLast = ch;
	pBufLast++;

	for (; pBufLast < &buf[sizeof(buf) - 4];)
	{
		switch (ch = getc(fl))
		{
		default:
			if (isdigit(ch))
			{
				*pBufLast = ch;
				pBufLast++;
			}
			else
			{
				*pBufLast = 0;
				i = atoll(buf);
				if (i >= beg_range && i <= end_range)
				{
					return i;
				}
				else
				{
					logentry(QStringLiteral("fread_int: Bad value for range %1 - %2: %3").arg(beg_range).arg(end_range).arg(i));
					throw error_range_int();
				}
			}
			break;

		case (char)EOF:
			perror("fread_int: EOF");
			abort();
			break;
		}
	}

	perror("fread_int: something went wrong");
	abort();
	return (0);
}

int64_t fread_int(std::ifstream &in, int64_t beg_range, int64_t end_range)
{
	int64_t number;
	in >> number;

	if (number < beg_range)
	{
		// std::cerr << "fread_int: error " << number << " less than " << beg_range << ". " << "Setting to " << beg_range << std::endl;
		number = beg_range;
	}
	else if (number > end_range)
	{
		// std::cerr << "fread_int: error " << number << " greater than " << beg_range << ". " << "Setting to " << beg_range << std::endl;
		number = end_range;
	}

	return number;
}

template <typename T>
T fread_int(QTextStream &in, T beg_range, T end_range)
{
	T number;

	QString line = qDebugQTextStreamLine(in, "");
	QStringList namelist = line.split(' ');
	QString arg1 = namelist.value(0);
	in >> number >> Qt::ws;

	bool ok = false;
	if (std::is_signed<T>::value)
	{
		if (arg1.toLongLong(&ok) != number && ok)
		{
			logentry(QStringLiteral("fread_int<%1> value %2 from \"%3\" != %4").arg(typeid(beg_range).name()).arg(arg1.toULongLong(&ok)).arg(arg1).arg(number));
		}
		else if (!ok)
		{
			logentry(QStringLiteral("fread_int<%1> arg2.toLongLong not ok.").arg(typeid(beg_range).name()));
		}
	}
	else if (std::is_unsigned<T>::value)
	{
		if (arg1.toULongLong(&ok) != number && ok)
		{
			logentry(QStringLiteral("fread_int<%1> value %2 from \"%3\" != %4").arg(typeid(beg_range).name()).arg(arg1.toULongLong(&ok)).arg(arg1).arg(number));
		}
		else if (!ok)
		{
			logentry(QStringLiteral("fread_int<%1> arg2.toULongLong not ok.").arg(typeid(beg_range).name()));
		}
	}
	else
	{
		qFatal("arg1 neither signed nor unsigned");
	}

	if (number < beg_range)
	{
		qDebug("increasing number");
		number = beg_range;
	}
	else if (number > end_range)
	{
		qDebug("decreasing number");
		number = end_range;
	}

	// qDebug() << "fread_int returning" << number;
	// qDebugQTextStreamLine(in, "After fread_int");
	return number;
}

/*
 fread_int has the nasty habit of reading on white
 space char after the int it reads.  This can goof
 up stuff like comments in the zone file.
 */
int64_t fread_int(FILE *fl, int64_t beg_range, int64_t end_range)
{
	char buf[MAX_STRING_LENGTH];
	char *pBufLast;
	int ch;
	int64_t i;

	while ((ch = getc(fl)))
	{
		if (ch == EOF)
		{
			qWarning() << "Reading" << DC::getInstance()->current();
			perror("fread_int: premature EOF");
			abort();
		}

		if (ch != ' ' && ch != '\n') /* eat the white space */
			break;
	}

	pBufLast = buf;

	if (ch == '-' && beg_range >= 0)
	{
		while (isdigit(getc(fl)))
		{
		}
		throw error_negative_int();
	}
	else if (ch == '-')
	{
		*pBufLast = ch;
		pBufLast++;
		ch = getc(fl);
	}

	*pBufLast = ch;
	pBufLast++;

	for (; pBufLast < &buf[sizeof(buf) - 4];)
	{
		switch (ch = getc(fl))
		{
		default:
			if (isdigit(ch))
			{
				*pBufLast = ch;
				pBufLast++;
			}
			else
			{
				*pBufLast = 0;
				i = atoll(buf);
				if (i >= beg_range && i <= end_range)
				{
					return i;
				}
				else
				{
					logentry(QStringLiteral("fread_int: Bad value for range %1 - %2: %3").arg(beg_range).arg(end_range).arg(i));

					if (i < beg_range)
					{
						throw error_range_under();
					}
					else if (i > end_range)
					{
						throw error_range_over();
					}
				}
			}
			break;

		case (char)EOF:
			perror("fread_int: EOF");
			abort();
			break;
		}
	}

	perror("fread_int: something went wrong");
	abort();
	return (0);
}

char fread_char(QTextStream &fl)
{
	if (fl.atEnd())
	{
		logbug(QStringLiteral("Reading %1").arg(DC::getInstance()->current()));
		perror("fread_char: premature EOF");
		abort();
	}

	char c;
	fl >> c;

	return c;
}

char fread_char(FILE *fl)
{
	int ch;

	while ((ch = getc(fl)))
	{
		if (ch == EOF)
		{
			logbug(QStringLiteral("Reading %1").arg(DC::getInstance()->current()));
			perror("fread_char: premature EOF");
			abort();
		}

		if (ch != ' ' && ch != '\n') /* eat the white space */
			break;
	}

	return ch;
}

/* release memory allocated for a char struct */
void free_char(Character *ch, Trace trace)
{
	int iWear;
	//  struct affected_type *af;
	struct char_player_alias *x;
	struct char_player_alias *next;
	mob_prog_act_list *currmprog;
	auto &character_list = DC::getInstance()->character_list;
	auto &free_list = DC::getInstance()->free_list;

	if (free_list.contains(ch))
	{
		Trace trace = free_list.at(ch);
		std::stringstream ss;
		ss << trace;
		logf(IMMORTAL, DC::LogChannel::LOG_BUG, "free_char: previously freed Character %p found in free_list from %s", ch, ss.str().c_str());

		if (character_list.contains(ch))
		{
			logf(IMMORTAL, DC::LogChannel::LOG_BUG, "free_char: previously freed Character %p found in character_list", ch);
		}

		const auto &shooting_list = DC::getInstance()->shooting_list;
		if (shooting_list.contains(ch))
		{
			logf(IMMORTAL, DC::LogChannel::LOG_BUG, "free_char: previously freed Character %p found in shooting_list", ch);
		}

		produce_coredump(ch);
		return;
	}
	else
	{
		free_list[ch] = trace;
	}

	character_list.erase(ch);

	if (ch->tempVariable)
	{
		struct tempvariable *temp, *tmp;
		for (temp = ch->tempVariable; temp; temp = tmp)
		{
			tmp = temp->next;
			delete temp;
		}
	}
	SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop falling off

	for (iWear = 0; iWear < MAX_WEAR; iWear++)
	{
		if (ch->equipment[iWear])
			obj_to_char(unequip_char(ch, iWear, 1), ch);
	}
	while (ch->carrying)
		extract_obj(ch->carrying);

	if (IS_PC(ch))
	{
		if (ch->short_desc)
			dc_free(ch->short_desc);
		if (ch->long_desc)
			dc_free(ch->long_desc);
		if (ch->description)
			dc_free(ch->description);
		if (ch->player)
		{
			// these won't be here if you free an unloaded char
			ch->player->skillchange = 0;
			if (!ch->player->ignoring.empty())
				ch->player->ignoring.clear();
			if (ch->player->golem)
				logentry(QStringLiteral("Error, golem not released properly"), ANGEL, DC::LogChannel::LOG_BUG);
			/* Free aliases... (I was to lazy to do before. ;) */
			ch->player->away_msgs.clear();

			if (ch->player->lastseen)
				delete ch->player->lastseen;

			if (ch->player->config)
			{
				delete ch->player->config;
			}

			if (ch->player)
			{
				delete ch->player;
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
				dc_free(ch->mobdata->mpact->buf);
			dc_free(ch->mobdata->mpact);
			ch->mobdata->mpact = currmprog;
		}
		ch->mobdata = {};
	}
	else
	{
		// logf(IMMORTAL, DC::LogChannel::LOG_BUG, QStringLiteral("free_char: '%1' is not PC or NPC").arg(GET_NAME(ch)).toStdString().c_str());
	}

	if (ch->title)
		dc_free(ch->title);
	ch->title = nullptr;

	remove_memory(ch, 't');

	// Since affect_remove updates the linked list itself, do it this way
	while (ch->affected)
		affect_remove(ch, ch->affected, SUPPRESS_ALL);

	delete ch;
}

/* release memory allocated for an obj struct */
void free_obj(class Object *obj)
{
	struct extra_descr_data *ths, *next_one;

	for (ths = obj->ex_description; ths; ths = next_one)
	{
		next_one = ths->next;
		dc_free(ths);
	}

	dc_free(obj->affected);

	delete obj;
}

/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
	FILE *fl;
	char tmp[100];

	*buf = '\0';

	if (!(fl = fopen(name, "r")))
	{
		DC::getInstance()->logverbose(QStringLiteral("Unable to open '%1':%2").arg(name).arg(strerror(errno)));
		*buf = '\0';
		return (-1);
	}

	do
	{
		fgets(tmp, 99, fl);

		if (!feof(fl))
		{
			if (strlen(buf) + strlen(tmp) + 2 > MAX_STRING_LENGTH)
			{
				logentry(QStringLiteral("fl->strng: std::string too big (db.c, file_to_string)"),
						 0, DC::LogChannel::LOG_BUG);
				*buf = '\0';
				return (-1);
			}

			strcat(buf, tmp);
			*(buf + strlen(buf) + 1) = '\0';
			*(buf + strlen(buf)) = '\r';
		}
	} while (!feof(fl));

	fclose(fl);

	return (0);
}

/* clear some of the the working variables of a char */
void reset_char(Character *ch)
{
	int i;

	GET_HOME(ch) = START_ROOM;

	for (i = 0; i < MAX_WEAR; i++) /* Intializing  */
		ch->equipment[i] = 0;

	ch->followers = 0;
	ch->master = 0;

	ch->spelldamage = 0;
	ch->carrying = 0;
	ch->carry_weight = 0;
	ch->carry_items = 0;
	ch->next = 0;
	ch->next_fighting = 0;
	ch->next_in_room = 0;
	ch->fighting = 0;
	ch->setStanding();
	ch->carry_weight = 0;
	ch->carry_items = 0;

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

	ch->misc = 0;

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
	ch->group_name = 0;
	ch->ambush = 0;
	ch->guarding = 0;
	ch->guarded_by = 0;
}

/*
 * Clear but do not de-alloc.
 */
void clear_char(Character *ch)
{
	if (ch == nullptr)
	{
		return;
	}

	*ch = {};
	ch->in_room = DC::NOWHERE;
	ch->setStanding();
	GET_HOME(ch) = START_ROOM;
	GET_AC(ch) = 100; /* Basic Armor */
}

void clear_object(class Object *obj)
{
	if (obj == nullptr)
	{
		return;
	}

	*obj = {};
	obj->vnum = -1;
	obj->in_room = DC::NOWHERE;
	obj->vroom = 0;
	obj->obj_flags = obj_flag_data();
	obj->num_affects = 0;
	obj->affected = nullptr;

	obj->name = nullptr;
	obj->description = nullptr;
	obj->short_description = nullptr;
	obj->ActionDescription(QString());
	obj->ex_description = nullptr;
	obj->carried_by = nullptr;
	obj->equipped_by = nullptr;

	obj->in_obj = nullptr;
	obj->contains = nullptr;

	obj->next_content = nullptr;
	obj->next = nullptr;
	obj->next_skill = nullptr;
	;
	obj->table = nullptr;
	obj->slot = nullptr;
	obj->wheel = nullptr;
	obj->save_expiration = 0;
	obj->no_sell_expiration = 0;
}

// Roll up the random modifiers to saving throw for new character
void apply_initial_saves(Character *ch)
{
	for (int i = 0; i <= SAVE_TYPE_MAX; i++)
		if (number(0, 1))
			ch->player->saves_mods[i] = number(-3, 3);
		else
			ch->player->saves_mods[i] = 0;
}

void init_char(Character *ch)
{
	GET_TITLE(ch) = str_dup("is still a virgin.");

	ch->clan = 0;

	ch->short_desc = 0;
	ch->long_desc = 0;
	ch->description = 0;

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

	ch->altar = nullptr;
	ch->spec = 0;
	ch->setPrompt({});
	ch->setLastPrompt({});
	ch->player->skillchange = 0;
	ch->player->joining = {};
	ch->player->practices = 0;
	ch->player->time.birth = time(0);
	ch->player->time.played = 0;
	ch->player->time.logon = time(0);
	ch->player->toggles = 0;
	ch->player->golem = 0;
	ch->player->quest_points = 0;
	for (int j = 0; j < QUEST_CANCEL; j++)
		ch->player->quest_cancel[j] = 0;
	for (int j = 0; j <= QUEST_TOTAL / ASIZE; j++)
		ch->player->quest_complete[j] = 0;

	SET_BIT(ch->player->toggles, Player::PLR_ANSI);
	SET_BIT(ch->player->toggles, Player::PLR_DAMAGE);
	int i;
	for (i = 0; i < AFF_MAX / ASIZE + 1; i++)
		ch->affected_by[i] = 0;

	apply_initial_saves(ch);

	for (int i = 0; i < 3; i++)
		GET_COND(ch, i) = 50; // 50 ticks of "full-ness"

	reset_char(ch);
}

/* returns the real number of the room with given virt number */
room_t real_room(room_t virt)
{
	if (virt < 0 || virt > DC::getInstance()->top_of_world)
	{
		return DC::NOWHERE;
	}

	if (DC::getInstance()->rooms.contains(virt))
	{
		return virt;
	}

	return DC::NOWHERE;
}

/* returns the real number of the monster with given virt number */
int real_mobile(int virt)
{
	if (DC::getInstance()->mob_index.contains(virt))
		return virt;
	else
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
int obj_in_index(char *name, int index)
{
	int i, j;

	for (i = 0, j = 1; (i < MAX_INDEX) && (j <= index) &&
					   DC::getInstance()->obj_index[i].item;
		 i++)
		if (isexact(name, DC::getInstance()->obj_index[i].item->name))
		{
			if (j == index)
				return i;
			j++;
		}

	return -1;
}

int mob_in_index(char *name, int index)
{
	int i, j;

	for (i = 0, j = 1; (i < MAX_INDEX) && (j <= index) &&
					   ((Character *)(DC::getInstance()->mob_index[i].item));
		 i++)
		if (isexact(name, GET_NAME(((Character *)(DC::getInstance()->mob_index[i].item)))))
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

int mprog_name_to_type(QString name)
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

void mprog_file_read(char *f, int32_t i)
{
	QSharedPointer<class MobProgram> mprog{};
	FILE *fp{};
	char letter{};
	char name[128]{};
	int type{};

	sprintf(name, "%s%s", MOB_DIR, f);
	if (!(fp = fopen(name, "r")))
	{
		logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d couldn't opne mobprog file.", i);
		return;
	}
	for (;;)
	{
		if ((letter = fread_char(fp)) == '|')
			break;
		else if (letter != '>')
		{
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mprog_file_read: Invalid letter mob %d.", i);
			return;
		}
		switch ((type = fread_int(fp, 0, MPROG_MAX_TYPE_VALUE)))
		{
		case ERROR_PROG:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d: in file prog error.", i);
			return;
		case IN_FILE_PROG:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d: nested in file progs.", i);
			return;
		default:
			SET_BIT(DC::getInstance()->mob_index[i].progtypes, type);
			mprog = QSharedPointer<class MobProgram>::create();
			mprog->type = type;
			mprog->arglist = fread_string(fp, 0);
			mprog->comlist = fread_string(fp, 0);
			break;
		}
	}
	fclose(fp);
	return;
}

void load_mobprogs(FILE *fp)
{
	char letter;
	int value;

	for (;;)
		switch (LOWER(letter = fread_char(fp)))
		{
		default:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Load_mobprogs: bad command '%c'.", letter);
			break;
		case 's':
			return;
		case '*':
			break;
		case 'm':
			value = fread_int(fp, 0, 2147483467);
			if (real_mobile(value) < 0)
			{
				logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Load_mobprogs: vnum %d doesn't exist.", value);
				break;
			}
			mprog_file_read(fread_word(fp, 1), value);
			break;
		}
	return;
}

void mprog_read_programs(FILE *fp, int32_t i, bool ignore)
{
	char letter;
	int type;
	for (;;)
	{
		if ((letter = fread_char(fp)) == '|')
			break;
		else if (letter != '>' && letter != '\\')
		{
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Load_mobiles: vnum %d MOBPROG char", i);
			ungetc(letter, fp);
			return;
		}
		type = mprog_name_to_type(fread_word(fp, 1));
		switch (type)
		{
		case ERROR_PROG:
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Load_mobiles: vnum %d MOBPROG type.", i);
			return;
		case IN_FILE_PROG:
			mprog_file_read(fread_string(fp, 1), i);
			break;
		default:
			if (!ignore)
			{
				if (letter == '>')
					SET_BIT(DC::getInstance()->mob_index[i].progtypes, type);
				else
					SET_BIT(DC::getInstance()->obj_index[i].progtypes, type);
			}
			auto mprog = QSharedPointer<class MobProgram>::create();
			mprog->type = type;
			mprog->arglist = fread_string(fp, 0);
			mprog->comlist = fread_string(fp, 0);
			if (!ignore)
			{
				if (letter == '>')
				{
					mprog->next = DC::getInstance()->mob_index[i].mobprogs; // when we write them, we write last first
					DC::getInstance()->mob_index[i].mobprogs = mprog;		// so reading them this way keeps them in order
				}
				else
				{
					mprog->next = DC::getInstance()->obj_index[i].mobprogs;
					DC::getInstance()->obj_index[i].mobprogs = mprog;
				}
			}
			break;
		}
	}
	return;
}

void mprog_read_programs(QTextStream &fp, int32_t i, bool ignore)
{
	char letter = {};
	int type = {};
	for (;;)
	{
		fp >> letter;

		if (letter == '|')
		{
			break;
		}
		else if (letter != '>' && letter != '\\')
		{
			logentry(QStringLiteral("Load_mobiles: vnum %1 MOBPROG char").arg(i));
			return;
		}
		QString word = fread_word(fp);
		type = mprog_name_to_type(word);
		switch (type)
		{
		case ERROR_PROG:
			logentry(QStringLiteral("Load_mobiles: vnum %1 MOBPROG type.").arg(i));
			return;
		case IN_FILE_PROG:
			mprog_file_read(fread_string(fp, 1), i);
			break;
		default:
			if (!ignore)
			{
				if (letter == '>')
					SET_BIT(DC::getInstance()->mob_index[i].progtypes, type);
				else
					SET_BIT(DC::getInstance()->obj_index[i].progtypes, type);
			}
			QSharedPointer<class MobProgram> mprog = QSharedPointer<class MobProgram>::create();
			mprog->type = type;
			mprog->arglist = fread_string(fp, false);
			mprog->comlist = fread_string(fp, false);
			if (!ignore)
			{
				if (letter == '>')
				{
					mprog->next = DC::getInstance()->mob_index[i].mobprogs; // when we write them, we write last first
					DC::getInstance()->mob_index[i].mobprogs = mprog;		// so reading them this way keeps them in order
				}
				else
				{
					mprog->next = DC::getInstance()->obj_index[i].mobprogs;
					DC::getInstance()->obj_index[i].mobprogs = mprog;
				}
			}
			break;
		}
	}
	return;
}

// * --- End MOBProgs stuff --- *

void find_unordered_mobiles(void)
{
	int cur_vnum, last_vnum = 0;

	for (int rnum = 0; rnum <= top_of_mobt; rnum++, last_vnum = cur_vnum)
	{
		cur_vnum = DC::getInstance()->mob_index[rnum].vnum;

		if (cur_vnum < last_vnum)
		{
			logf(0, DC::LogChannel::LOG_MISC, "Out of order vnum found. Vnum: %d Last Vnum: %d Rnum: %d", cur_vnum, last_vnum, rnum);
		}
	}
}

void string_to_file(FILE *f, QString str)
{
	fprintf(f, "%s~\n", str.remove('\r').toStdString().c_str());
}

void string_to_file(QTextStream &fl, QString str)
{
	fl << str.remove('\r') + "~\n";
}

void copySaveData(Object *target, Object *source)
{
	int i;
	if ((i = eq_current_damage(source)) > 0)
	{
		for (; i > 0; i--)
			damage_eq_once(target);
	}

	if (strcmp(GET_OBJ_SHORT(source), GET_OBJ_SHORT(target)))
	{
		GET_OBJ_SHORT(target) = str_hsh(GET_OBJ_SHORT(source));
	}
	if (strcmp(source->description, target->description))
	{
		target->description = str_hsh(source->description);
	}

	if (strcmp(source->name, target->name))
	{
		target->name = str_hsh(source->name);
	}

	if (source->obj_flags.type_flag != target->obj_flags.type_flag)
	{
		target->obj_flags.type_flag = source->obj_flags.type_flag;
	}

	if (source->obj_flags.extra_flags != target->obj_flags.extra_flags)
	{
		target->obj_flags.extra_flags = source->obj_flags.extra_flags;
	}

	if (source->obj_flags.more_flags != target->obj_flags.more_flags)
	{
		target->obj_flags.more_flags = source->obj_flags.more_flags;
	}

	bool custom = isSet(source->obj_flags.more_flags, ITEM_CUSTOM);
	if (custom)
	{
		target->obj_flags.value[0] = source->obj_flags.value[0];
	}

	uint8_t type_flag = source->obj_flags.type_flag;
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

	if (type_flag == ITEM_ARMOR || type_flag == ITEM_WEAPON || type_flag == ITEM_INSTRUMENT || type_flag == ITEM_WAND)
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
				errno = 0;
				target->affected = (obj_affected_type *)realloc(target->affected,
																(sizeof(obj_affected_type) * source->num_affects));
				if (target->affected == nullptr)
				{
					perror("realloc");
					abort();
					exit(EXIT_FAILURE);
				}
				target->num_affects = source->num_affects;
			}

			for (int i = 0; i < source->num_affects; ++i)
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

	return;
}

bool fullItemMatch(Object *obj, Object *obj2)
{
	if (strcmp(GET_OBJ_SHORT(obj), GET_OBJ_SHORT(obj2)))
	{
		return false;
	}

	if (strcmp(obj->name, obj2->name))
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
		for (int i = 0; i < obj->num_affects; ++i)
		{
			if ((obj->affected[i].location != obj2->affected[i].location) ||
				(obj->affected[i].modifier != obj2->affected[i].modifier))
			{
				return false;
			}
		}
	}

	return 1;
}

// Function to ensure an item is not bugged. If it is, replace it with the original.
// TODO check for more than short_description differences
bool DC::verify_item(class Object **obj_ptr)
{
	Object *obj = *obj_ptr;
	if (!obj || !obj->short_description)
	{
		return false;
	}

	vnum_t vnum = obj->vnum;
	if (!obj_index.contains(vnum))
	{
		return false;
	}

	Object *reference_obj = obj_index[vnum].item;
	if (!reference_obj || !reference_obj->short_description)
	{
		return false;
	}

	if (!str_cmp(obj->short_description, reference_obj->short_description))
	{
		return false;
	}

	*obj_ptr = DC::getInstance()->clone_object(vnum); // Fixed!
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
	filepath_ = QStringLiteral("%1%2").arg(directory_).arg(filename_);

	if (!QDir(directory_).exists())
	{
		logentry(QStringLiteral("LegacyFile::openFile: Directory '%1' does not exist.").arg(directory_));
	}

	openFile();
}

LegacyFile::~LegacyFile()
{
	if (file_handle_)
	{
		fclose(file_handle_);
	}
}

FILE *LegacyFile::openFile(void)
{
	if (file_handle_)
	{
		fclose(file_handle_);
	}

	if ((file_handle_ = fopen(qPrintable(filepath_), "w")) == nullptr)
	{
		qCritical() << error_message_.arg(filepath_);
		return nullptr;
	}

	return file_handle_;
}

bool LegacyFile::backupFile(void)
{
	QFileInfo fi(filepath_);
	if (fi.exists())
	{
		QString originalfileName = fi.canonicalFilePath();
		QString backupFileName = QStringLiteral("%1.last").arg(originalfileName);

		if (QFile::exists(backupFileName))
		{
			if (!QFile::remove(backupFileName))
			{
				logentry(QStringLiteral("Unable to remove '%1'.").arg(backupFileName));
			}
		}
		if (!QFile::copy(originalfileName, backupFileName))
		{
			logentry(QStringLiteral("Unable to copy '%1' to '%2'.").arg(originalfileName).arg(backupFileName));
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