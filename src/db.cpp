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

extern "C"
{
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
}

#include <sstream>
#include <limits>

#include <affect.h>
#include <db.h>
#include <memory.h>
#include <structs.h> // MAX_STRING_LENGTH
#include <weather.h> // structs
#include <timeinfo.h> // structs
#include <player.h> // log info
#include <fileinfo.h> // file names
#include <utility.h> // assign..
#include <levels.h>
#include <character.h>
#include <mobile.h> 
#include <room.h>
#include <race.h>
#include <obj.h> // extra_descr_data
#include <handler.h> // get_obj_num
#include <connect.h> // descriptor_data
#include <game_portal.h> // load_game_portals()
#include <interp.h>
#include <returnvals.h>
#include <spells.h> // command_range
#include <shop.h>
#include <help.h>
#include <quest.h>
#include <vault.h>


using namespace std;

int load_new_help(FILE *fl, int reload, struct char_data *ch);
void load_vaults();
void load_auction_tickets();
void load_corpses(void);
int count_hash_records(FILE * fl);
void load_hints();
void find_unordered_objects(void);
void find_unordered_mobiles(void);
char *mprog_type_to_name(int type);
void write_wizlist(std::stringstream &filename);
void write_wizlist(string filename);
void write_wizlist(const char filename[]);

/* load stuff */
char* curr_type;
char* curr_name;
int curr_virtno;

extern bool verbose_mode;
extern char *item_types[];
extern char *wear_bits[];
extern char *extra_bits[];
extern char *more_obj_bits[];
extern char *apply_types[];
extern uint16_t port1;

/**************************************************************************
 *  declarations of most of the 'global' variables                         *
 ************************************************************************ */

world_file_list_item * world_file_list = 0;  // List of the world files
world_file_list_item * mob_file_list = 0;  // List of the mob files
world_file_list_item * obj_file_list = 0;  // List of the obj files

struct room_data ** world_array = 0;   // array of rooms
int top_of_world = 0;                 // index of last room in world
int top_of_world_alloc = 0;           // index of last alloc'd memory in world

int top_of_zonet = 0;                 // index of last valid zonefile

struct obj_data *object_list = 0; /* the global linked list of obj's */

pulse_data *bard_list = 0; /* global l-list of bards          */

room_data & CWorld::operator[](int rnum)
{
	if (rnum > top_of_world)
		throw overrun();
	else if (rnum < 0)
		throw underrun();

	return *world_array[rnum];
}

CWorld world;

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

struct zone_data zone_table_array[MAX_ZONE + 50];
struct zone_data *zone_table = zone_table_array;
#define zone_table  zone_table_array
/* table of reset data             */
int top_of_zone_table = 0;
struct message_list fight_messages[MAX_MESSAGES]; /* fighting messages   */

struct wizlist_info
{
	char *name;
	int level;
};
struct skill_quest *skill_list; // List of skill quests.

char webpage[MAX_STRING_LENGTH]; /* the webbrowser connect screen*/
char greetings1[MAX_STRING_LENGTH]; /* the greeting screen          */
char greetings2[MAX_STRING_LENGTH]; /* the other greeting screen    */
char greetings3[MAX_STRING_LENGTH];
char greetings4[MAX_STRING_LENGTH];
char credits[MAX_STRING_LENGTH]; /* the Credits List              */
char motd[MAX_STRING_LENGTH]; /* the messages of today         */
char imotd[MAX_STRING_LENGTH]; /* the immortal messages of today*/
char story[MAX_STRING_LENGTH]; /* the game story                */
char help[MAX_STRING_LENGTH]; /* the main help page            */
char new_help[MAX_STRING_LENGTH]; /* the main new help page            */
char new_ihelp[MAX_STRING_LENGTH]; /* the main immortal help page            */
char info[MAX_STRING_LENGTH]; /* the info text                 */
struct wizlist_info wizlist[100]; /* the actual wizlist            */

FILE *help_fl; /* file for help texts (HELP <kwd>)*/
FILE *new_help_fl; /* file for help texts (HELP <kwd>)*/

struct index_data mob_index_array[MAX_INDEX];
struct index_data *mob_index = mob_index_array;
#define mob_index   mob_index_array

struct index_data obj_index_array[MAX_INDEX];
struct index_data *obj_index = obj_index_array;
/* index table for object file     */
#define obj_index   obj_index_array

struct help_index_element *help_index = 0;
struct help_index_element_new *new_help_table = 0;

int top_of_mobt = 0; /* top of mobile index table       */
int top_of_objt = 0; /* top of object index table       */
int top_of_helpt = 0; /* top of help index table         */
int new_top_of_helpt = 0; /* top of help index table         */
int total_rooms = 0; /* total amount of rooms in memory */

struct time_info_data time_info; /* the infomation about the time   */
struct weather_data weather_info; /* the infomation about the weather */

int mud_is_booting;

struct vault_data *vault_table = 0;

/* local procedures */
void boot_zones(void);
void setup_dir(FILE *fl, int room, int dir);
void load_banned();
void boot_world(void);
void do_godlist();
void half_chop(char *string, char *arg1, char *arg2);
void remove_memory(CHAR_DATA *ch, char type);
world_file_list_item * new_mob_file_item(char * temp, long room_nr);
world_file_list_item * new_obj_file_item(char * temp, long room_nr);

char * read_next_worldfile_name(FILE * flWorldIndex);
int fread_bitvector(FILE *fl, long minval, long maxval);
int fread_bitvector(ifstream &fl, long minval, long maxval);
void fread_new_newline(FILE *fl)
{
}
char fread_char(FILE *fl);
struct index_data *generate_mob_indices(int *top, struct index_data *index);
struct index_data *generate_obj_indices(int *top, struct index_data *index);
int zone_is_empty(int zone_nr);
void reset_zone(int zone);
void fix_shopkeepers_inventory();
int file_to_string(const char *name, char *buf);
void renum_world(void);
void renum_zone_table(void);
void reset_time(void);
void clear_char(CHAR_DATA *ch);

// MOBprogram locals
int mprog_name_to_type(char* name);
//void		load_mobprogs           ( FILE* fp );
void mprog_read_programs(FILE* fp, long i, bool zz);

extern bool MOBtrigger;

/* external refs */
extern struct descriptor_data *descriptor_list;
void load_messages(char *file, int base = 0);
int dice(int number, int size);
int number(int from, int to);
void boot_social_messages(void);
void boot_clans(void);
void assign_clan_rooms(void);
struct help_index_element *build_help_index(FILE *fl, int *num);
// The room_data implementation
// -Sadus 9/1/96

void room_data::FreeTracks()
{
	room_track_data * curr;

	for (curr = tracks; curr; curr = tracks)
			{
		tracks = tracks->next;
		// trackee is a str_hsh, don't free it
		dc_free(curr);
	}
	tracks = NULL;
	last_track = NULL;
	nTracks = 0;
}

// add new tracks to the head of the list. When the list
// gets longer than 11, remove the tail and delete it.
void room_data::AddTrackItem(room_track_data * newTrack)
{
	if (!tracks) {
		tracks = newTrack;
		last_track = newTrack;
		nTracks = 1;
		return;
	}

	// add it to the head of the double-linked list
	newTrack->next = tracks;
	tracks->previous = newTrack;
	tracks = newTrack;

	if (++nTracks > 11) {
		room_track_data * pScent;
		pScent = last_track->previous;
		pScent->next = 0;
		dc_free(last_track);
		last_track = pScent;
		nTracks--;
	}
}

room_track_data * room_data::TrackItem(int nIndex)
{
	int nr;
	room_track_data * pScent;

	for (pScent = tracks, nr = 1; pScent;
			pScent = pScent->next, nr++)
		if (nr == nIndex)
			return pScent;

	return 0;
}

void add_to_bard_list(char_data * ch)
{
	pulse_data * curr = NULL;

	if (GET_CLASS(ch) != CLASS_BARD)
		return;

#ifdef LEAK_CHECK
	curr = (struct pulse_data *)
	calloc(1, sizeof(struct pulse_data));
#else
	curr = (struct pulse_data *)
			dc_alloc(1, sizeof(struct pulse_data));
#endif

	curr->thechar = ch;
	curr->next = bard_list;
	bard_list = curr;
}

void remove_from_bard_list(char_data * ch)
{
	pulse_data * curr = NULL;
	pulse_data * last = NULL;

	if (!bard_list)
		return;

	if (bard_list->thechar == ch)
			{
		curr = bard_list;
		bard_list = bard_list->next;
		dc_free(curr);
	}
	else
	{
		last = bard_list;
		for (curr = bard_list->next; curr; curr = curr->next)
				{
			if (curr->thechar == ch)
					{
				last->next = curr->next;
				dc_free(curr);
				break;
			}
			last = curr;
		}
	}
}

char * funnybootmessages[] =
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
				"Booting help files...\r\n",
				"Booting shops...\r\n",
				"Generating dynamic areas...\r\n",
				"Assigning mobile process pointers...\r\n",
				"Synchronizing threads...\r\n",
				"Initializing mob AI engine...\r\n",
				"True Randomization(tm) of backstab generator...\r\n",
				"Re-training slutty blondes...\r\n",
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
				"Cacheing zone connection map...\r\n",
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
				"Divide By Cucumber Error. Please Reinstall Universe And Reboot\r\n"
						"Removing Rubicon's genital warts....ERROR: Unable to remove warts!\r\n"
						"Removing crash bugs...\r\n"
						"Cooking Swedish meatballs...\r\n"
						"Brewing Canadian beer...\r\n"
						"Searching for intelligent players....searching....searching....searching\n\r"
						"Coding bug...\r\n"
						"Uploading Urizen's ABBA mp3s...\r\n",
				"09 F9 11 02 9D 74 E3 5B D8 41 56 C5 63 56 88 C0\r\n",
				"Error: Server just accidentally a whole pfile!\r\n",
				"Redecorating the tavern...\r\n",
				"Putting cover sheets on TPS reports.\r\n",
				"Nerfing <insert class>...\r\n",
				"Smash forehead on keyboard to continue.\r\n",
				"Enter any 11-digit prime number to continue.\r\n",
				"This will end your Windows session. Do you want to play another game?\n\r",
				"Error saving file! Format drive now? (Y/Y)\r\n",
				"User Error: Replace user.\r\n",
				"Windows found. Delete? (Y/N)\r\n",
				"Hard Drive scan indicates pirated software! The police have been notified.\r\n"
		};

void funny_boot_message()
{
	struct descriptor_data *d;

	extern int was_hotboot;

	if (!was_hotboot)
		return;

	int num = sizeof(funnybootmessages) / sizeof(char *);

	num = number(0, num - 1);

	for (d = descriptor_list; d; d = d->next)
		write_to_descriptor(d->descriptor, funnybootmessages[num]);
}

/* Write skillquest file. 
 It checks if ch exists everywhere it is used,
 so this can be called from other places without
 a character attached. */
int do_write_skillquest(struct char_data *ch, char *argument, int cmd)
{
	struct skill_quest *curr;
	FILE *fl;

	if (!(fl = dc_fopen(SKILL_QUEST_FILE, "w")))
	{
		if (ch)
			send_to_char("Can't open the skill quest file.\r\n", ch);
		return eFAILURE;
	}
	for (curr = skill_list; curr; curr = curr->next)
			{
		fprintf(fl, "%d %s~\n", curr->num, curr->message);
		fprintf(fl, "%d %d\n", curr->clas, curr->level);
	}
	fprintf(fl, "0\n");
	dc_fclose(fl);
	send_to_char("Skill quests saved.\n\r", ch);
	return eSUCCESS;
}

void load_skillquests()
{
	struct skill_quest *newsq, *last = 0;
	skill_list = NULL;
	int i;
	FILE *fl;

	if (!(fl = dc_fopen(SKILL_QUEST_FILE, "r")))
	{
		log("Cannot open skill quest file.", 0, LOG_MISC);
		abort();
	}

	while ((i = fread_int(fl, 0, 1000)) != 0)
	{
#ifdef LEAK_CHECK
		newsq = (struct skill_quest *) calloc(1, sizeof(struct skill_quest));
#else
		newsq = (struct skill_quest *)dc_alloc(1, sizeof(struct skill_quest));
#endif

		newsq->num = i;
		if (find_sq(i)) {
			char buf[512];
			sprintf(buf, "%d duplicate.", i);
			log(buf, 0, LOG_BUG);
		}
		newsq->message = fread_string(fl, 0);
		newsq->clas = fread_int(fl, 0, INT_MAX);
		newsq->level = fread_int(fl, 0, 200);
		newsq->next = 0;

		if (last)
			last->next = newsq;
		else
			skill_list = newsq;

		last = newsq;
	}
	dc_fclose(fl);
}

/*************************************************************************
 *  routines for booting the system                                       *
 *********************************************************************** */

/* body of the booting system */
void boot_db(void)
{
	int i;
	int help_rec_count = 0;

#ifdef LEAK_CHECK
	void cause_leak();
#endif

	void add_commands_to_radix(void);

	mud_is_booting = TRUE;

	reset_time();

	log("************** REBOOTING THE MUD ***********", 0, LOG_SOCKET);
	log("************** REBOOTING THE MUD ***********", 0, LOG_MISC);
	log("************** REBOOTING THE MUD ***********", 0, LOG_WORLD);
	log("Reading aux files.", 0, LOG_MISC);
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

#ifdef LEAK_CHECK
	cause_leak();
#endif

	do_godlist();
	log("Godlist done!", 0, LOG_MISC);
	log("Booting clans...", 0, LOG_MISC);

#ifdef LEAK_CHECK
	cause_leak();
#endif

	boot_clans();

	log("Loading new news file.", 0, LOG_MISC);
	extern void loadnews();
	loadnews();

	log("Loading new help file.", 0, LOG_MISC);

	// new help file stuff
	if (!(new_help_fl = dc_fopen(NEW_HELP_FILE, "r"))) {
		perror( NEW_HELP_FILE);
		abort();
	}
	help_rec_count = count_hash_records(new_help_fl);
	dc_fclose(new_help_fl);

	if (!(new_help_fl = dc_fopen(NEW_HELP_FILE, "r"))) {
		perror( NEW_HELP_FILE);
		abort();
	}
	CREATE(new_help_table, struct help_index_element_new, help_rec_count);
	load_new_help(new_help_fl, 0, 0);
	dc_fclose(new_help_fl);
	// end new help files

	log("Opening help file.", 0, LOG_MISC);

	if (!(help_fl = dc_fopen(HELP_KWRD_FILE, "r"))) {
		perror( HELP_KWRD_FILE);
		abort();
	}

	help_index = build_help_index(help_fl, &top_of_helpt);

	log("Loading the zones", 0, LOG_MISC);
	boot_zones();

	log("Loading the world.", 0, LOG_MISC);
	// start world off with 2000 rooms of alloc'd space
	world_array = (room_data **)realloc(world_array, 2000 * sizeof(room_data *));
	top_of_world_alloc = 2000;
	// clear it (realloc = malloc, not calloc)
	for (int counter = 0; counter < top_of_world_alloc; counter++)
		world_array[counter] = 0;

	funny_boot_message();

	boot_world();

	log("Renumbering the world.", 0, LOG_MISC);
	renum_world();

	funny_boot_message();

	log("Generating mob indices/loading all mobiles", 0, LOG_MISC);
	generate_mob_indices(&top_of_mobt, mob_index);

	log("Generating object indices/loading all objects", 0, LOG_MISC);
	generate_obj_indices(&top_of_objt, obj_index);

	funny_boot_message();

	log("renumbering zone table", 0, LOG_MISC);
	renum_zone_table();

	log("Looking for unordered mobiles...", 0, LOG_MISC);
	find_unordered_mobiles();

	log("Looking for unordered objects...", 0, LOG_MISC);
	find_unordered_objects();

	extern short bport;
	if (!bport) {
		log("Loading Corpses.", 0, LOG_MISC);
		load_corpses();
	}

	log("Loading messages.", 0, LOG_MISC);
	load_messages(MESS_FILE);
	load_messages(MESS2_FILE, 2000);

	log("Loading socials.", 0, LOG_MISC);
	boot_social_messages();

	log("Adding commands to radix", 0, LOG_MISC);
	add_commands_to_radix();

	log("Processing game portals...", 0, LOG_MISC);
	load_game_portals();

	log("Loading emoting objects...", 0, LOG_MISC);
	load_emoting_objects();

	log("Adding clan room flags to rooms...", 0, LOG_MISC);
	assign_clan_rooms();

	log("Assigning function pointers.", 0, LOG_MISC);
	assign_mobiles();
	assign_objects();
	assign_rooms();

	if (verbose_mode) {
		fprintf( stderr, "\n[ Room  Room]\t{Level}\t  Author\tZone\n");
	}

	for (i = 0; i <= top_of_zone_table; i++)
			{
		if (verbose_mode) {
			fprintf(stderr, "[%5d %5d]\t%s.\n",
					(i ? (zone_table[i - 1].top + 1) : 0),
					zone_table[i].top,
					zone_table[i].name);
		}

		reset_zone(i);
	}
	if (verbose_mode) {
		fprintf( stderr, "\n");
	}

	log("Loading banned list", 0, LOG_MISC);
	load_banned();

	log("Loading skill quests.", 0, LOG_MISC);
	load_skillquests();

	log("Assigning inventory to shopkeepers", 0, LOG_MISC);
	fix_shopkeepers_inventory();

	log("Turning on MOB Progs", 0, LOG_MISC);
	MOBtrigger = TRUE;

	log("Loading quest one liners.", 0, LOG_MISC);
	load_quests();

	log("Loading vaults.", 0, LOG_MISC);
	load_vaults();

	log("Loading player hints.", 0, LOG_MISC);
	load_hints();

	log("Loading auction tickets.", 0, LOG_MISC);
	load_auction_tickets();

	mud_is_booting = FALSE;
}

/*
 int do_motdload(CHAR_DATA *ch, char *argument, int cmd)
 {
 file_to_string(MOTD_FILE, motd);
 file_to_string(IMOTD_FILE, imotd);
 send_to_char("Motd and Imotd both reloaded.\r\n", ch);
 return eSUCCESS;
 }
 */
void do_godlist()
{
	char bufl[100], buf2[100], buf3[100];
	int x;
	FILE *fl;

	log("Doing wizlist...db.c\n\r", 0, LOG_MISC);

	if (!(fl = dc_fopen("../lib/wizlist.txt", "r"))) {
		log("db.c: error reading wizlist.txt", ANGEL, LOG_BUG);
		perror("db.c...error reading wizlist.txt: ");
		dc_fclose(fl);
		return;
	}

	for (x = 0;; x++) {
		fgets(bufl, 90, fl);
		half_chop(bufl, buf2, buf3);
		if (buf2[0] == '@') {
			wizlist[x].name = str_dup("@");
			wizlist[x].level = 0;
			break;
		}
		wizlist[x].name = str_dup(buf2);
		wizlist[x].level = atoi(buf3);
	}

	log("Done!\n\r", 0, LOG_MISC);
	dc_fclose(fl);
}

void free_wizlist_from_memory()
{
	for (int x = 0;; x++)
			{
		if (wizlist[x].name[0] == '@')
				{
			dc_free(wizlist[x].name);
			break;
		}
		dc_free(wizlist[x].name);
	}
}

void write_wizlist(std::stringstream &filename)
{
	write_wizlist(filename.str().c_str());
}

void write_wizlist(string filename)
{
	write_wizlist(filename.c_str());
}

void write_wizlist(const char filename[])
{
	int x;
	FILE *fl;

	if (!(fl = dc_fopen(filename, "w"))) {
		logf(IMMORTAL, LOG_BUG, "Unable to open wizlist file: %s", filename);
		return;
	}

	for (x = 0;; x++) {
		if (wizlist[x].name[0] == '@') {
			fprintf(fl, "%s", "@ @\n");
			break;
		}
		if (wizlist[x].level >= IMMORTAL)
			fprintf(fl, "%s %d\n", wizlist[x].name, wizlist[x].level);
	}
	dc_fclose(fl);
}

void update_wizlist(CHAR_DATA *ch)
{
	int x;

	if (IS_NPC(ch))
		return;

	for (x = 0;; x++) {
		if (wizlist[x].name[0] == '@') {
			if (GET_LEVEL(ch) < IMMORTAL)
				return;
			dc_free(wizlist[x].name);
			wizlist[x].name = str_dup(GET_NAME(ch));
			wizlist[x].level = GET_LEVEL(ch);

			wizlist[x + 1].name = str_dup("@");
			wizlist[x + 1].level = 0;
			break;
		} else {
			if (isname(wizlist[x].name, GET_NAME(ch))) {
				wizlist[x].level = GET_LEVEL(ch);
				break;
			}
		}
	}

	write_wizlist("../lib/wizlist.txt");

	stringstream ssbuffer;
	ssbuffer << HTDOCS_DIR << port1 << "/wizlist.txt";
	write_wizlist(ssbuffer.str().c_str());
}

int do_wizlist(CHAR_DATA *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH], lines[500], space[80];
  int x, current_level, z = 1;
  int gods_each_level[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
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
       "(:) == Implementors == (:)"
  };

  for (sp = 0; sp < 80; sp++)
    space[sp] = ' ';

  // count the number of gods at each level, store in array gods_each_level
  for (x = 0;; x++) {
    if (wizlist[x].name[0] == '@')
      break;
    gods_each_level[wizlist[x].level - IMMORTAL]++;
  }

  buf[0] = '\0';
  for (current_level = IMP; current_level >= IMMORTAL; current_level--) {
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
    for (x = 0;; x++) {
      if (wizlist[x].name[0] == '@') {
	z = 1;
	if (*lines) {
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
      
      if (wizlist[x].level != current_level)
	continue;
      
      if (z++ % 5)
	sprintf(lines + strlen(lines), "%s, ", wizlist[x].name);
      else {
	sprintf(lines + strlen(lines), "%s\n\r", wizlist[x].name);
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
void reset_time(void)
{
	long beginning_of_time = 650336715;

	struct time_info_data mud_time_passed(time_t t2, time_t t1);

	time_info = mud_time_passed(time(0), beginning_of_time);

	switch (time_info.hours) {
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

	sprintf(log_buf, "Current Gametime: %dH %dD %dM %dY.",
			time_info.hours, time_info.day,
			time_info.month, time_info.year);
	log(log_buf, 0, LOG_MISC);

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

/* generate index table for monster file */
struct index_data *generate_mob_indices(int *top, struct index_data *index)
{
	int i = 0;
	char buf[82];
	char log_buf[256];
	FILE * flMobIndex;
	FILE * fl;
	char * temp;
	char endfile[180];
	struct world_file_list_item * pItem = NULL;
//  extern short code_testing_mode;
	extern short code_testing_mode_mob;

	log("Opening mobile file index.", 0, LOG_MISC);
	if (!code_testing_mode_mob) {
		if (!(flMobIndex = dc_fopen(MOB_INDEX_FILE, "r"))) {
			log("Could not open index file.", 0, LOG_MISC);
			abort();
		}
	}
	else if (!(flMobIndex = dc_fopen(MOB_INDEX_FILE_TINY, "r"))) {
		log("Could not open index file.", 0, LOG_MISC);
		abort();
	}

	log("Opening object files.", 0, LOG_MISC);

	// note, we don't worry about free'ing temp, cause it's held in the "mob_file_list"
	for (temp = read_next_worldfile_name(flMobIndex);
			temp;
			temp = read_next_worldfile_name(flMobIndex))
					{
		strcpy(endfile, "mobs/");
		strcat(endfile, temp);

		if (verbose_mode) {
			log(temp, 0, LOG_MISC);
		}

		if (!(fl = dc_fopen(endfile, "r")))
		{
			perror(endfile);
			logf(IMMORTAL, LOG_BUG, "generate_mob_indices: could not open mob file: %s", endfile);
			abort();
		}

		pItem = new_mob_file_item(temp, i);

		for (;;) {
			if (fgets(buf, 81, fl)) {

				if (*buf == '#') { /* allocate new_new cell */
					if (i >= MAX_INDEX) {
						perror("Too many mob indexes");
						abort();
					}
					sscanf(buf, "#%d", &index[i].virt);
					index[i].number = 0;
					index[i].non_combat_func = 0;
					index[i].combat_func = 0;
					index[i].mobprogs = NULL;
					index[i].mobspec = NULL;
					index[i].progtypes = 0;
					curr_virtno = index[i].virt;
					if (!(index[i].item = (CHAR_DATA *)read_mobile(i, fl))) {

						sprintf(log_buf, "Unable to load mobile %d!\n\r",
								index[i].virt);
						log(log_buf, ANGEL, LOG_BUG);
					}
					i++;
				}
				else if (*buf == '$') /* EOF */
					break;
			}
			else {
				sprintf(endfile, "Bad char (%s)", buf);
				log(endfile, 0, LOG_MISC);
				abort();
			}
		}

		pItem->lastnum = (i - 1);

		dc_fclose(fl);
	}
	*top = i - 1;
	dc_fclose(flMobIndex);
	/*
	 Here the index gets processed, and mob classes gets
	 assigned. (Not done in read_mobile 'cause of
	 the fact that all mobs aren't read yet,
	 and an attempt to assign non-existant mob
	 procs would be bad).
	 */
	for (i = 0; i <= top_of_mobt; i++)
			{
		add_mobspec(i);
	}
	return (index);
}

void add_mobspec(int i)
{
	if (i < 0)
		return;

	CHAR_DATA *a = (CHAR_DATA *)mob_index[i].item;
	if (!a)
		return;
	if (!a->c_class)
		return;

	int mob = 0;
	MPROG_DATA *mprg;

	switch (a->c_class)
	{
	case CLASS_MAGIC_USER:
		if (a->level < 21)
			mob = 101;
		else if (a->level < 35)
			mob = 102;
		else if (a->level < 51)
			mob = 103;
		else
			mob = 104;
		break;
	case CLASS_CLERIC:
		if (a->level < 21)
			mob = 105;
		else if (a->level < 35)
			mob = 106;
		else if (a->level < 51)
			mob = 107;
		else
			mob = 108;
		break;
	case CLASS_WARRIOR:
		if (a->level < 21)
			mob = 109;
		else if (a->level < 35)
			mob = 110;
		else if (a->level < 51)
			mob = 111;
		else
			mob = 112;
		break;
	case CLASS_BARBARIAN:
		if (a->level < 21)
			mob = 113;
		else if (a->level < 35)
			mob = 114;
		else if (a->level < 51)
			mob = 115;
		else
			mob = 116;
		break;
	case CLASS_MONK:
		if (a->level < 21)
			mob = 117;
		else if (a->level < 35)
			mob = 118;
		else if (a->level < 51)
			mob = 119;
		else
			mob = 120;
		break;
	case CLASS_THIEF:
		if (a->level < 21)
			mob = 121;
		else if (a->level < 35)
			mob = 122;
		else if (a->level < 51)
			mob = 123;
		else
			mob = 124;
		break;
	case CLASS_PALADIN:
		if (a->level < 21)
			mob = 125;
		else if (a->level < 35)
			mob = 126;
		else if (a->level < 51)
			mob = 127;
		else
			mob = 128;
		break;
	case CLASS_ANTI_PAL:
		if (a->level < 21)
			mob = 129;
		else if (a->level < 35)
			mob = 130;
		else if (a->level < 51)
			mob = 131;
		else
			mob = 132;
		break;
	case CLASS_RANGER:
		if (a->level < 21)
			mob = 133;
		else if (a->level < 35)
			mob = 134;
		else if (a->level < 51)
			mob = 135;
		else
			mob = 136;
		break;
	case CLASS_BARD:
		if (a->level < 21)
			mob = 137;
		else if (a->level < 35)
			mob = 138;
		else if (a->level < 51)
			mob = 139;
		else
			mob = 140;
		break;
	case CLASS_DRUID:
		if (a->level < 21)
			mob = 141;
		else if (a->level < 35)
			mob = 142;
		else if (a->level < 51)
			mob = 143;
		else
			mob = 144;
		break;
	case CLASS_NECROMANCER:
		if (a->level < 21)
			mob = 145;
		else if (a->level < 35)
			mob = 146;
		else if (a->level < 51)
			mob = 147;
		else
			mob = 148;
		break;
	case CLASS_PSIONIC:
		if (a->level < 21)
			mob_index[i].mobspec = mob_index[real_mobile(149)].mobprogs;
		else if (a->level < 35)
			mob_index[i].mobspec = mob_index[real_mobile(150)].mobprogs;
		else if (a->level < 51)
			mob_index[i].mobspec = mob_index[real_mobile(151)].mobprogs;
		else
			mob_index[i].mobspec = mob_index[real_mobile(152)].mobprogs;
		break;
	default:
		break;
	}

	if (mob) {
		mob_index[i].mobspec = mob_index[real_mobile(mob)].mobprogs;

		for (int j = 0; j < ACT_MAX / ASIZE + 1; j++) {
			SET_BIT(((CHAR_DATA *)mob_index[i].item)->mobdata->actflags[j],
					((CHAR_DATA *)mob_index[real_mobile(mob)].item)->mobdata->actflags[j]);
		}

		for (int j = 0; j < AFF_MAX / ASIZE + 1; j++) {
			SET_BIT(((CHAR_DATA *)mob_index[i].item)->affected_by[j],
					((CHAR_DATA *)mob_index[real_mobile(mob)].item)->affected_by[j]);
		}
	}

	if (mob_index[i].mobspec)
		for (mprg = mob_index[i].mobspec; mprg; mprg = mprg->next)
			SET_BIT(mob_index[i].progtypes, mprg->type);
}

void remove_all_mobs_from_world()
{
	auto &character_list = DC::instance().character_list;

	for_each(character_list.begin(), character_list.end(),
			[](char_data * const &curr) {
				if (IS_NPC(curr))
					extract_char(curr, TRUE);
				else
					do_quit(curr, "", 666);
			});
	DC::instance().removeDead();
}

void remove_all_objs_from_world()
{
	obj_data * curr = NULL;

	while ((curr = object_list))
		extract_obj(curr);
}

/* generate index table for object file */
struct index_data *generate_obj_indices(int *top,
		struct index_data *index)
{
	int i = 0;
	char buf[82];
	char log_buf[256];
	FILE * fl;
	FILE * flObjIndex;
	char * temp = NULL;
	char endfile[180];
	struct world_file_list_item * pItem = NULL;

//  if (!bport) {

	if (!(flObjIndex = dc_fopen(OBJECT_INDEX_FILE, "r"))) {
		log("Cannot open object file index.", 0, LOG_MISC);
		abort();
	}
	/*
	 } else {
	 if (!(flObjIndex = dc_fopen(OBJECT_INDEX_FILE_TINY,"r"))) {
	 log("Cannot open object file index.(tiny).",0,LOG_MISC);
	 abort();
	 }
	 }
	 */
	log("Opening object files.", 0, LOG_MISC);

	// note, we don't worry about free'ing temp, cause it's held in the "obj_file_list"
	for (temp = read_next_worldfile_name(flObjIndex);
			temp;
			temp = read_next_worldfile_name(flObjIndex))
					{
		strcpy(endfile, "objects/");
		strcat(endfile, temp);

		if (verbose_mode) {
			log(temp, 0, LOG_MISC);
		}

		if (!(fl = dc_fopen(endfile, "r")))
		{
			log("generate_obj_indices: could not open obj file.", 0, LOG_BUG);
			log(temp, 0, LOG_BUG);
			abort();
		}

		pItem = new_obj_file_item(temp, i);

		for (;;) {
			if (fgets(buf, 81, fl))
					{
				if (*buf == '#') /* allocate new_new cell */
				{
					if (i >= MAX_INDEX)
					{
						perror("Too many obj indexes");
						abort();
					}
					sscanf(buf, "#%d", &index[i].virt);
					index[i].number = 0;
					index[i].non_combat_func = 0;
					index[i].combat_func = 0;
					index[i].progtypes = 0;
					if (!(index[i].item = (struct obj_data *)read_object(i, fl, FALSE)))
					{
						sprintf(log_buf, "Unable to load object %d!\n\r",
								index[i].virt);
						log(log_buf, ANGEL, LOG_BUG);
					}
					i++;
				}
				else if (*buf == '$') /* EOF */
					break;
			}
			else {
				fprintf(stderr, "Error in \'%s\'.\n\r", endfile);
				perror("generate obj indices");
				abort();
			}
		} // for ;;

		pItem->lastnum = (i - 1);

		dc_fclose(fl);
	} // for next_in_file

	*top = i - 1;
	dc_fclose(flObjIndex);
	return (index);
}

void write_one_room(FILE * f, int a)
{
	struct extra_descr_data *extra;

	if (!world_array[a])
		return;

	fprintf(f, "#%d\n", world[a].number);
	string_to_file(f, world[a].name);
	string_to_file(f, world[a].description);

	if (world[a].iFlags)
		REMOVE_BIT(world[a].room_flags, world[a].iFlags);
	fprintf(f, "%d %d %d\n",
			world[a].zone,
			world[a].room_flags,
			world[a].sector_type
			);
	if (world[a].iFlags)
		SET_BIT(world[a].room_flags, world[a].iFlags);

	/* exits */
	for (int b = 0; b <= 5; b++)
			{
		if (!(world[a].dir_option[b]))
			continue;
		fprintf(f, "D%d\n", b);
		if (world[a].dir_option[b]->general_description)
			string_to_file(f, world[a].dir_option[b]->general_description);
		else
			fprintf(f, "~\n");  // print blank
		if (world[a].dir_option[b]->keyword)
			string_to_file(f, world[a].dir_option[b]->keyword);
		else
			fprintf(f, "~\n");  // print blank
		fprintf(f, "%d %d %d\n",
				world[a].dir_option[b]->exit_info,
				world[a].dir_option[b]->key,
				world[a].dir_option[b]->to_room
				);
	} /* exits */

	/* extra descriptions */
	for (extra = world[a].ex_description; extra; extra = extra->next)
			{
		if (!extra)
			break;
		fprintf(f, "E\n");
		if (extra->keyword)
			string_to_file(f, extra->keyword);
		else
			fprintf(f, "~\n");  // print blank
		if (extra->description)
			string_to_file(f, extra->description);
		else
			fprintf(f, "~\n");  // print blank
	} /* extra descriptions */

	struct deny_data *deni;
	for (deni = world[a].denied; deni; deni = deni->next)
		fprintf(f, "B\n%d\n", deni->vnum);

	// Write out allowed classes if any
	for (int i = 0; i < CLASS_MAX; i++) {
		if (world[a].allow_class[i] == TRUE) {
			fprintf(f, "C%d\n", i);
		}
	}

	fprintf(f, "S\n");
}

int read_one_room(FILE *fl, int & room_nr)
{
	int zone = 0, tmp = 0;
	char *temp;
	char ch;
	int dir;
	struct extra_descr_data *new_new_descr;

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

		curr_virtno = room_nr;
		curr_type = "Room";
		curr_name = temp;

		/* a new_new record to be read */

		if (room_nr >= top_of_world_alloc)
				{
			world_array = (room_data **)realloc(world_array, (room_nr + 200) * sizeof(room_data *));
			for (int counter = top_of_world_alloc; counter < room_nr + 200; counter++)
				world_array[counter] = 0;
			top_of_world_alloc = room_nr + 200;

			if (!world_array) {
				log("Unable to realloc more rooms in read_one_room.  Aborting.", IMMORTAL, LOG_WORLD);
				log("Unable to realloc more rooms in read_one_room.  Aborting.", IMMORTAL, LOG_BUG);
				abort();
			}
		}

		if (top_of_world < room_nr)
			top_of_world = room_nr;

		world_array[room_nr] = (room_data *)calloc(1, sizeof(room_data));

		world[room_nr].paths = 0;
		world[room_nr].number = room_nr;
		world[room_nr].name = temp;
		world[room_nr].description = fread_string(fl, 0);
		world[room_nr].nTracks = 0;
		world[room_nr].tracks = 0;
		world[room_nr].last_track = 0;
		world[room_nr].denied = 0;
		total_rooms++;
		if (top_of_zone_table >= 0)
				{
			tmp = fread_int(fl, -1, 64000); // zone nr

			// OBS: Assumes ordering of input rooms

			if (world[room_nr].number <= (zone ? zone_table[zone - 1].top : -1))
					{
				fprintf(stderr, "Room nr %d is below zone %d.\n", room_nr, zone);
				log("Room below minimum for it's zone table.  ERROR", IMMORTAL, LOG_BUG);
				log("Room below minimum for it's zone table.  ERROR", IMMORTAL, LOG_MISC);
//        abort();
				return FALSE;
			}

			// Go through the zone table until world[room_nr].number is
			// in the current zone.

			while (world[room_nr].number > zone_table[zone].top)
			{
				if (++zone > top_of_zone_table)
						{
					fprintf(stderr, "Room %d is outside of any zone.\n", room_nr);
					log("Room outside of ANY zone.  ERROR", IMMORTAL, LOG_BUG);
					log("Room outside of ANY zone.  ERROR", IMMORTAL, LOG_MISC);
					return FALSE;
				}
			}

			if (room_nr > zone_table[zone].top_rnum)
				zone_table[zone].top_rnum = room_nr;
			if (room_nr < zone_table[zone].bottom_rnum)
				zone_table[zone].bottom_rnum = room_nr;

			world[room_nr].zone = zone;
		} // of top_of_zone_table > 0

		world[room_nr].room_flags = fread_bitvector(fl, -1, LONG_MAX);
		if (IS_SET(world[room_nr].room_flags, NO_ASTRAL))
			REMOVE_BIT(world[room_nr].room_flags, NO_ASTRAL);

		// This bitvector is for runtime and not stored in the files, so just initialize it to 0
		world[room_nr].temp_room_flags = 0;

		world[room_nr].sector_type = fread_int(fl, -1, 64000);

		if (load_debug)
		{
			printf("Flags are %d %d %d\n", tmp, world[room_nr].room_flags,
					world[room_nr].sector_type);
			fflush(stdout);
		}

		world[room_nr].funct = 0;
		world[room_nr].contents = 0;
		world[room_nr].people = 0;
		world[room_nr].light = 0; /* Zero light sources */

		for (tmp = 0; tmp <= 5; tmp++)
			world[room_nr].dir_option[tmp] = 0;

		world[room_nr].ex_description = 0;

		for (;;)
				{
			ch = fread_char(fl); /* dir field */

			/* direction field */
			if (ch == 'D') {
				dir = fread_int(fl, 0, 5);
				setup_dir(fl, room_nr, dir);
			}
			/* extra description field */
			else if (ch == 'E') {
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
				new_new_descr->next = world[room_nr].ex_description;
				world[room_nr].ex_description = new_new_descr;
			}
			else if (ch == 'B')
					{
				struct deny_data *deni;
#ifdef LEAK_CHECK
				deni = (struct deny_data *)calloc(1,sizeof(struct deny_data));
#else
				deni = (struct deny_data *)dc_alloc(1, sizeof(struct deny_data));
#endif
				deni->vnum = fread_int(fl, -1, LONG_MAX);
				deni->next = world[room_nr].denied;
				world[room_nr].denied = deni;
			}
			else if (ch == 'S') /* end of current room */
				break;
			else if (ch == 'C') {
				int c_class = fread_int(fl, 0, CLASS_MAX);
				world[room_nr].allow_class[c_class] = TRUE;
			}
		} // of for (;;) (get directions and extra descs)

		return TRUE;
	} // if == $
//  dc_free(temp); /* cleanup the area containing the terminal $  */
// we no longer free temp, cause it's no longer used as a terminating char
	return FALSE;
}

char * read_next_worldfile_name(FILE * flWorldIndex)
{
	char * temp;

	temp = fread_string(flWorldIndex, 0);

	// Check for end of file marker
	if (!strcmp(temp, "$"))
			{
		dc_free(temp);
		return NULL;
	}

	// Check for comments
	if (!strncmp(temp, "*", 1))
			{
		dc_free(temp);
		// Whee!  recursive loops!
		temp = read_next_worldfile_name(flWorldIndex);
	}

	return temp;
}

bool can_modify_this_room(char_data * ch, long vnum)
{
	if (has_skill(ch, COMMAND_RANGE))
		return TRUE;

	if (ch->pcdata->buildLowVnum <= 0 || ch->pcdata->buildHighVnum <= 0)
		return FALSE;
	if (ch->pcdata->buildLowVnum > vnum)
		return FALSE;
	if (ch->pcdata->buildHighVnum < vnum)
		return FALSE;
	return TRUE;
}

bool can_modify_room(char_data * ch, long vnum)
{
	if (has_skill(ch, COMMAND_RANGE))
		return TRUE;

	if (ch->pcdata->buildLowVnum <= 0 || ch->pcdata->buildHighVnum <= 0)
		return FALSE;
	if (ch->pcdata->buildLowVnum > vnum)
		return FALSE;
	if (ch->pcdata->buildHighVnum < vnum)
		return FALSE;
	return TRUE;
}

bool can_modify_this_mobile(char_data * ch, long vnum)
{
	if (has_skill(ch, COMMAND_RANGE))
		return TRUE;

	if (ch->pcdata->buildMLowVnum <= 0 || ch->pcdata->buildMHighVnum <= 0)
		return FALSE;
	if (ch->pcdata->buildMLowVnum > vnum)
		return FALSE;
	if (ch->pcdata->buildMHighVnum < vnum)
		return FALSE;
	return TRUE;
}

bool can_modify_mobile(char_data * ch, long mob)
{
	return can_modify_this_mobile(ch, mob);
}

bool can_modify_this_object(char_data * ch, long vnum)
{
	if (has_skill(ch, COMMAND_RANGE))
		return TRUE;

	if (ch->pcdata->buildOLowVnum <= 0 || ch->pcdata->buildOHighVnum <= 0)
		return FALSE;
	if (ch->pcdata->buildOLowVnum > vnum)
		return FALSE;
	if (ch->pcdata->buildOHighVnum < vnum)
		return FALSE;
	return TRUE;

}

bool can_modify_object(char_data * ch, long obj)
{
	return can_modify_this_object(ch, obj);
}

void set_zone_saved_zone(long room)
{
	int zone = world[room].zone;
	REMOVE_BIT(zone_table[zone].zone_flags, ZONE_MODIFIED);
}

void set_zone_modified_zone(long room)
{
	int zone = world[room].zone;
	SET_BIT(zone_table[zone].zone_flags, ZONE_MODIFIED);
}

void set_zone_modified(long modnum, world_file_list_item * list)
{
	world_file_list_item * curr = list;

	while (curr)
		if (modnum >= curr->firstnum && modnum <= curr->lastnum)
			break;
		else
			curr = curr->next;

	if (!curr) {
		log("ERROR in set_zone_modified: Cannot find room!!!", IMMORTAL, LOG_BUG);
		return;
	}

	curr->flags = WORLD_FILE_MODIFIED;
}

void set_zone_modified_world(long room)
{
	extern world_file_list_item * world_file_list;

	set_zone_modified(room, world_file_list);
}

// rnum of mob
void set_zone_modified_mob(long mob)
{
	extern world_file_list_item * mob_file_list;

	set_zone_modified(mob, mob_file_list);
}

// rnum of mob
void set_zone_modified_obj(long obj)
{
	extern world_file_list_item * obj_file_list;

	set_zone_modified(obj, obj_file_list);
}

void set_zone_saved(long modnum, world_file_list_item * list)
{
	world_file_list_item * curr = list;

	while (curr)
		if (modnum >= curr->firstnum && modnum <= curr->lastnum)
			break;
		else
			curr = curr->next;

	if (!curr) {
		log("ERROR in set_zone_modified: Cannot find room!!!", IMMORTAL, LOG_BUG);
		return;
	}

	REMOVE_BIT(curr->flags, WORLD_FILE_MODIFIED);
}

void set_zone_saved_world(long room)
{
	extern world_file_list_item * world_file_list;

	set_zone_saved(room, world_file_list);
}

void set_zone_saved_mob(long mob)
{
	extern world_file_list_item * mob_file_list;

	set_zone_saved(mob, mob_file_list);
}

void set_zone_saved_obj(long obj)
{
	extern world_file_list_item * obj_file_list;

	set_zone_saved(obj, obj_file_list);
}

/* destruct the world */
void free_world_from_memory()
{
	struct extra_descr_data * curr_extra = NULL;
	struct world_file_list_item * curr_wfli = NULL;

	for (int i = 0; i <= top_of_world; i++)
			{
		if (world_array[i] == 0)
			continue;

		if (world[i].name)
			dc_free(world[i].name);

		if (world[i].description)
			dc_free(world[i].description);

		while (world[i].ex_description)
		{
			curr_extra = world[i].ex_description->next;
			if (world[i].ex_description->keyword)
				dc_free(world[i].ex_description->keyword);
			if (world[i].ex_description->description)
				dc_free(world[i].ex_description->description);
			dc_free(world[i].ex_description);
			world[i].ex_description = curr_extra;
		}

		for (int j = 0; j < 6; j++)
			if (world[i].dir_option[j])
			{
				dc_free(world[i].dir_option[j]->general_description);
				dc_free(world[i].dir_option[j]->keyword);
				dc_free(world[i].dir_option[j]);
			}

		world[i].FreeTracks();
		dc_free(world_array[i]);
	}

	curr_wfli = world_file_list;

	while (curr_wfli) {
		dc_free(curr_wfli->filename);
		world_file_list = curr_wfli->next;
		dc_free(curr_wfli);
		curr_wfli = world_file_list;
	}
}

void free_mobs_from_memory()
{
	struct char_data * curr = NULL;

	for (int i = 0; i <= top_of_mobt; i++)
			{
		if ((curr = (struct char_data *)mob_index[i].item))
		{
			free_char(curr);
			mob_index[i].item = NULL;
		}
	}
}

void free_objs_from_memory()
{
	struct obj_data * curr = NULL;
	//struct extra_descr_data * curr_extra = NULL;

	for (int i = 0; i <= top_of_objt; i++)
		if ((curr = (struct obj_data *) obj_index[i].item))
		{
			free_obj(curr);
			obj_index[i].item = NULL;
		}
}

world_file_list_item * one_new_world_file_item(char * temp, long room_nr)
{
	world_file_list_item * curr = NULL;

#ifdef LEAK_CHECK
	curr = (world_file_list_item *) calloc(1, sizeof(world_file_list_item));
#else
	curr = (world_file_list_item *)dc_alloc(1, sizeof(world_file_list_item));
#endif

	curr->filename = temp;
	curr->firstnum = room_nr;
	curr->lastnum = -1;
	curr->flags = 0;
	curr->next = NULL;
	return curr;
}

world_file_list_item * new_w_file_item(char * temp, long room_nr, world_file_list_item *& list)
{
	world_file_list_item * curr = list;

	if (!list) {
		list = one_new_world_file_item(temp, room_nr);
		return list;
	}

	while (curr->next)
		curr = curr->next;

	curr->next = one_new_world_file_item(temp, room_nr);
	return curr->next;
}

world_file_list_item * new_world_file_item(char * temp, long room_nr)
{
	return new_w_file_item(temp, room_nr, world_file_list);
}

world_file_list_item * new_mob_file_item(char * temp, long room_nr)
{
	return new_w_file_item(temp, room_nr, mob_file_list);
}

world_file_list_item * new_obj_file_item(char * temp, long room_nr)
{
	return new_w_file_item(temp, room_nr, obj_file_list);
}

/* load the rooms */
void boot_world(void)
{
	FILE *fl;
	FILE *flWorldIndex;
	int room_nr = 0;
	char * temp;
	char endfile[200]; // hopefully noone is stupid and makes a 180 char filename
	struct world_file_list_item * pItem = NULL;
	extern short code_testing_mode;
	extern short code_testing_mode_world;

	object_list = 0;

	if ((!code_testing_mode || code_testing_mode_world))
	{
		if (!(flWorldIndex = dc_fopen(WORLD_INDEX_FILE, "r")))
		{
			perror("dc_fopen");
			log("boot_world: could not open world index file.", 0, LOG_BUG);
			abort();
		}
	}
	else if (!(flWorldIndex = dc_fopen(WORLD_INDEX_FILE_TINY, "r")))
	{
		perror("dc_fopen");
		log("boot_world: could not open world index file tiny.", 0, LOG_BUG);
		abort();
	}
	log("Booting individual world files", 0, LOG_MISC);

	// note, we don't worry about free'ing temp, cause it's held in the "world_file_list"
	for (temp = read_next_worldfile_name(flWorldIndex);
			temp;
			temp = read_next_worldfile_name(flWorldIndex))
					{
		strcpy(endfile, "world/");
		strcat(endfile, temp);

		if (verbose_mode) {
			log(temp, 0, LOG_MISC);
		}

		if (!(fl = dc_fopen(endfile, "r")))
		{
			perror("dc_fopen");
			log("boot_world: could not open world file.", 0, LOG_BUG);
			log(temp, 0, LOG_BUG);
			abort();
		}

		pItem = new_world_file_item(temp, room_nr);

		while (read_one_room(fl, room_nr))
			;

		// push the first num forward until it hits a room, that way it's
		// accurate.
		// "pItem->firstnum < top_of_world_alloc" check is to insure we dont access memory not allocated to world_array
		for (; pItem->firstnum < top_of_world_alloc && !world_array[pItem->firstnum]; pItem->firstnum++)
			;

		pItem->lastnum = room_nr / 100 * 100 + 99;

		room_nr++;

		dc_fclose(fl);
	}
	log("World Boot done.", 0, LOG_MISC);
	dc_fclose(flWorldIndex);

	top_of_world = --room_nr;
}

/* read direction data */
void setup_dir(FILE *fl, int room, int dir)
{
	int tmp;

	if (world[room].dir_option[dir])
	{
		char buf[200];
		sprintf(buf, "Room %d attemped to created two exits in the same direction.", world[room].number);
		log(buf, 0, LOG_WORLD);
		if (world[room].dir_option[dir]->general_description)
			dc_free(world[room].dir_option[dir]->general_description);
		if (world[room].dir_option[dir]->keyword)
			dc_free(world[room].dir_option[dir]->keyword);
		dc_free(world[room].dir_option[dir]);
	}

#ifdef LEAK_CHECK
	world[room].dir_option[dir] = (struct room_direction_data *)
	calloc(1, sizeof(struct room_direction_data));
#else
	world[room].dir_option[dir] = (struct room_direction_data *)
			dc_alloc(1, sizeof(struct room_direction_data));
#endif

	world[room].dir_option[dir]->general_description =
			fread_string(fl, 0);

	world[room].dir_option[dir]->keyword = fread_string(fl, 0);

	tmp = fread_bitvector(fl, -1, 300); /* tjs hack - not the right range */
	world[room].dir_option[dir]->exit_info = tmp;
	world[room].dir_option[dir]->bracee = NULL;

	world[room].dir_option[dir]->key = fread_int(fl, -62000, 62000);
	world[room].dir_option[dir]->to_room = fread_int(fl, -62000, 62000);
}

// return true for success
int create_one_room(CHAR_DATA *ch, int vnum)
{
	struct room_data *rp;
	extern int top_of_zone_table;
	int x;

	char buf[256];

	if (world_array[vnum])
		return 0;

	if (vnum > WORLD_MAX_ROOM)
		return 0;

	if (vnum > top_of_world)
		top_of_world = vnum;

	if (top_of_world + 1 >= top_of_world_alloc) {

		world_array = (room_data **)realloc(world_array, (top_of_world + 200) * sizeof(room_data *));
		for (int counter = top_of_world_alloc; counter < top_of_world + 200; counter++)
			world_array[counter] = 0;
		top_of_world_alloc = top_of_world + 200;

		if (!world_array) {
			log("Out of memory in create_one_room.", IMMORTAL, LOG_BUG);
			abort();
		}
	}

	world_array[vnum] = (room_data *)calloc(1, sizeof(room_data));

	rp = &world[vnum];

	rp->number = vnum;

	if (top_of_zone_table >= 0) {
		int zone;

		for (zone = 0;
				rp->number > zone_table[zone].top && zone <= top_of_zone_table;
				zone++)
			;
		if (zone > top_of_zone_table) {
			fprintf(stderr, "Room %d is outside of any zone.\n", rp->number);
			zone--;
		}
		rp->zone = zone;
	}

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
	rp->description = (char *)str_dup("Empty description.\n\r");
	return 1;
}

void renum_world(void)
{
	int room, door;

	for (room = 0; room <= top_of_world; room++)
		for (door = 0; door <= 5; door++)
			if (world_array[room])
				if (world[room].dir_option[door])
					if (world[room].dir_option[door]->to_room != NOWHERE)
						world[room].dir_option[door]->to_room =
								real_room(world[room].dir_option[door]->to_room);
}

void renum_zone_table(void)
{
	int zone, comm;

	for (zone = 0; zone <= top_of_zone_table; zone++)
		for (comm = 0; zone_table[zone].cmd[comm].command != 'S'; comm++)
				{
			zone_table[zone].cmd[comm].active = 1;
			switch (zone_table[zone].cmd[comm].command)
			{
			case 'M':
				/*if(real_room(zone_table[zone].cmd[comm].arg3) < 0) {

				 fprintf(stderr, "Problem in zonefile: no room number %d for mob "
				 "%d - setting to J command\n", zone_table[zone].cmd[comm].arg3,
				 zone_table[zone].cmd[comm].arg1);
				 zone_table[zone].cmd[comm].command = 'J';
				 }*/
				if (real_mobile(zone_table[zone].cmd[comm].arg1) >= 0
						&& real_room(zone_table[zone].cmd[comm].arg3) >= 0)
					zone_table[zone].cmd[comm].arg1 =
							real_mobile(zone_table[zone].cmd[comm].arg1);
				else {
					zone_table[zone].cmd[comm].active = 0;
				}
//          zone_table[zone].cmd[comm].arg3;// = 
//                real_room(zone_table[zone].cmd[comm].arg3);

				break;
			case 'O':
				if (real_object(zone_table[zone].cmd[comm].arg1) >= 0
						&& real_room(zone_table[zone].cmd[comm].arg3) >= 0)
					zone_table[zone].cmd[comm].arg1 = real_object(zone_table[zone].cmd[comm].arg1);
				else
					zone_table[zone].cmd[comm].active = 0;

//            if (zone_table[zone].cmd[comm].arg3 != NOWHERE)
				//          zone_table[zone].cmd[comm].arg3 =
				//        real_room(zone_table[zone].cmd[comm].arg3);
				break;
			case 'G':
				if (real_object(zone_table[zone].cmd[comm].arg1) >= 0)
					zone_table[zone].cmd[comm].arg1 =
							real_object(zone_table[zone].cmd[comm].arg1);
				else
					zone_table[zone].cmd[comm].active = 0;

				break;
			case 'E':
				if (real_object(zone_table[zone].cmd[comm].arg1) >= 0)
					zone_table[zone].cmd[comm].arg1 =
							real_object(zone_table[zone].cmd[comm].arg1);
				else
					zone_table[zone].cmd[comm].active = 0;

				break;
			case 'P':
				if (real_object(zone_table[zone].cmd[comm].arg1) >= 0
						&& real_object(zone_table[zone].cmd[comm].arg3) >= 0)
								{
					zone_table[zone].cmd[comm].arg1 =
							real_object(zone_table[zone].cmd[comm].arg1);
					zone_table[zone].cmd[comm].arg3 =
							real_object(zone_table[zone].cmd[comm].arg3);

				}
				else
					zone_table[zone].cmd[comm].active = 0;
				break;
			case 'D':
				if (real_room(zone_table[zone].cmd[comm].arg1) < 0)
					zone_table[zone].cmd[comm].active = 0;
				else {
					zone_table[zone].cmd[comm].arg1 =
							real_room(zone_table[zone].cmd[comm].arg1);
					if (zone_table[zone].cmd[comm].arg1 == -1)
							{
						fprintf(stderr, "Problem in zonefile: no room number for door"
								" - setting to J command\n");
						zone_table[zone].cmd[comm].command = 'J';
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
				log("Illegal char hit in renum_zone_table", 0, LOG_WORLD);
				break;
			}
		}
}

void free_zones_from_memory()
{
	for (int i = 0; i < MAX_ZONE; i++)
			{
		if (zone_table[i].name)
			dc_free(zone_table[i].name);
		if (zone_table[i].cmd)
		{
//      We're str_hsh'ing this now, so we don't need to worry about it
//      for(int j = 0; zone_table[i].cmd[j].command != 'S'; j++)
//        if(zone_table[i].cmd[j].comment)
//          dc_free(zone_table[i].cmd[j].comment);
			dc_free(zone_table[i].cmd);
			// don't have to free zone_table[i].cmd.comment cause it's str_hsh'd
		}
	}
}
#define ZCMD zone_table[zone].cmd[cmd_no]
#define ZWCMD zone_table[zon].cmd[i]

void write_one_zone(FILE * fl, int zon)
{
	fprintf(fl, "V2\n");
	fprintf(fl, "#%d\n", (zon ? ((zone_table[zon - 1].top + 1) / 100) : 0));
	fprintf(fl, "%s~\n", zone_table[zon].name);
	fprintf(fl, "%d %d %d %ld %d\n", zone_table[zon].top,
	zone_table[zon].lifespan,
	zone_table[zon].reset_mode,
	zone_table[zon].zone_flags,
	zone_table[zon].continent
			);

	for (int i = 0; zone_table[zon].cmd[i].command != 'S'; i++)
			{
		if (zone_table[zon].cmd[i].command == '*')
			fprintf(fl, "* %s\n", zone_table[zon].cmd[i].comment ?
			zone_table[zon].cmd[i].comment :
										"");
		else if (zone_table[zon].cmd[i].command == '%')
			fprintf(fl, "%% %2d %3d %3d %s\n", zone_table[zon].cmd[i].if_flag,
			zone_table[zon].cmd[i].arg1,
			zone_table[zon].cmd[i].arg2,
			zone_table[zon].cmd[i].comment ?
			zone_table[zon].cmd[i].comment :
								"");
		else if (zone_table[zon].cmd[i].command == 'X')
			fprintf(fl, "X %2d %5d %3d %5d%s\n", zone_table[zon].cmd[i].if_flag,
			zone_table[zon].cmd[i].arg1,
			zone_table[zon].cmd[i].arg2,
			zone_table[zon].cmd[i].arg3,
			zone_table[zon].cmd[i].comment ?
			zone_table[zon].cmd[i].comment :
								"");
		else if (zone_table[zon].cmd[i].command == 'K')
			fprintf(fl, "K %2d %5d %3d %5d%s\n", zone_table[zon].cmd[i].if_flag,
			zone_table[zon].cmd[i].arg1,
			zone_table[zon].cmd[i].arg2,
			zone_table[zon].cmd[i].arg3,
			zone_table[zon].cmd[i].comment ?
			zone_table[zon].cmd[i].comment :
								"");
		else if (zone_table[zon].cmd[i].command == 'M') {
			int virt = ZWCMD.active?mob_index[ZWCMD.arg1].virt:ZWCMD.arg1;
			fprintf(fl, "M %2d %5d %3d %5d %s\n", zone_table[zon].cmd[i].if_flag,
					virt,
					zone_table[zon].cmd[i].arg2,
					zone_table[zon].cmd[i].arg3,
					zone_table[zon].cmd[i].comment ?
					zone_table[zon].cmd[i].comment : "");
		}
		else if(zone_table[zon].cmd[i].command == 'P') {
			int virt = ZWCMD.active?obj_index[ZWCMD.arg1].virt:ZWCMD.arg1;
			int virt2 = ZWCMD.active?obj_index[ZWCMD.arg3].virt:ZWCMD.arg3;
			fprintf(fl, "P %2d %5d %3d %5d %s\n", zone_table[zon].cmd[i].if_flag,
					virt,
					zone_table[zon].cmd[i].arg2,
					virt2,
					zone_table[zon].cmd[i].comment ?
					zone_table[zon].cmd[i].comment : "");
		}
		else if(zone_table[zon].cmd[i].command == 'G') {
			int virt = ZWCMD.active?obj_index[ZWCMD.arg1].virt:ZWCMD.arg1;

			fprintf(fl, "G %2d %5d %3d %5d %s\n", zone_table[zon].cmd[i].if_flag,
					virt,
					zone_table[zon].cmd[i].arg2,
					zone_table[zon].cmd[i].arg3,
					zone_table[zon].cmd[i].comment ?
					zone_table[zon].cmd[i].comment : "");
		}
		else if(zone_table[zon].cmd[i].command == 'O') {
			int virt = ZWCMD.active?obj_index[ZWCMD.arg1].virt:ZWCMD.arg1;
			fprintf(fl, "O %2d %5d %3d %5d %s\n", zone_table[zon].cmd[i].if_flag,
					virt,
					zone_table[zon].cmd[i].arg2,
					zone_table[zon].cmd[i].arg3,
					zone_table[zon].cmd[i].comment ?
					zone_table[zon].cmd[i].comment : "");
		}
		else if(zone_table[zon].cmd[i].command == 'E') {
			int virt = ZWCMD.active?obj_index[ZWCMD.arg1].virt:ZWCMD.arg1;
			fprintf(fl, "E %2d %5d %3d %5d %s\n", zone_table[zon].cmd[i].if_flag,
					virt,
					zone_table[zon].cmd[i].arg2,
					zone_table[zon].cmd[i].arg3,
					zone_table[zon].cmd[i].comment ?
					zone_table[zon].cmd[i].comment : "");
		}
		else fprintf(fl, "%c %2d %5d %3d %5d %s\n", zone_table[zon].cmd[i].command,
				zone_table[zon].cmd[i].if_flag,
				zone_table[zon].cmd[i].arg1,
				zone_table[zon].cmd[i].arg2,
				zone_table[zon].cmd[i].arg3,
				zone_table[zon].cmd[i].comment ?
				zone_table[zon].cmd[i].comment : "");
	}

	fprintf(fl, "S\n$~\n");
}

void read_one_zone(FILE * fl, int zon)
{
	struct reset_com reset_tab[MAX_RESET];
	char *check, buf[161], ch;
	int reset_top, i, tmp;
	char * skipper = NULL;
	int version = 1;
	bool modified = false;

	ch = fread_char(fl);
	if (ch == 'V') {
		version = fread_int(fl, 0, 64000);
		ch = fread_char(fl);
		modified = true;
	}

	tmp = fread_int(fl, 0, 64000);
	check = fread_string(fl, 0);
	//a = fread_int(fl, 0, 64000);
	/* alloc a new_new zone */
	//*num = zon = a / 100;
	top_of_zonet = zon;
	if (zon >= MAX_ZONE)
	{
		log("Max zone hit!", 0, LOG_MISC);
		abort();
	}

//  top_of_zonet = zon;

	// log("Reading zone", 0, LOG_BUG);

	curr_virtno = tmp;
	curr_type = "Zone";
	curr_name = check;

	zone_table[zon].name = check;
	zone_table[zon].top = fread_int(fl, 0, 64000);
	zone_table[zon].clanowner = 0;
	zone_table[zon].gold = 0;
	zone_table[zon].repops_without_deaths = -1;
	zone_table[zon].repops_with_bonus = 0;
///  extern void debug_point();

//  if (tmp == 23)
	//   debug_point();

	/*
	 * this initialization is important for obtaining the
	 * actual values to be stored in the following 2 variables.
	 * grep this file to see why.
	 * -Sadus
	 */
	zone_table[zon].bottom_rnum = WORLD_MAX_ROOM;
	zone_table[zon].top_rnum = 0;

	zone_table[zon].lifespan = fread_int(fl, 0, 64000);
	zone_table[zon].reset_mode = fread_int(fl, 0, 64000);
	zone_table[zon].zone_flags = fread_bitvector(fl, 0, LONG_MAX);

	// if its old version set the altered flag so that
	// this zone will be saved with new format soon
	if (modified == true)
		SET_BIT(zone_table[zon].zone_flags, ZONE_MODIFIED);

	if (version > 1) {
		zone_table[zon].continent = fread_int(fl, 0, 64000);
	}

	/* read the command table */
	reset_top = 0;

	for (;;)
			{
		if (reset_top >= MAX_RESET) {
			perror("Too many zone resets");
			abort();
		}
		reset_tab[reset_top].comment = NULL; // needs to be initialized
		reset_tab[reset_top].command = fread_char(fl);
		reset_tab[reset_top].if_flag = 0;
		reset_tab[reset_top].last = 0;
		reset_tab[reset_top].arg1 = 0;
		reset_tab[reset_top].arg2 = 0;
		reset_tab[reset_top].arg3 = 0;
		if (reset_tab[reset_top].command == 'S')
				{
			reset_top++;
			break;
		}

		if (reset_tab[reset_top].command == '*')
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
				reset_tab[reset_top].comment = str_hsh(skipper);
			reset_top++;
			continue;
		}

		tmp = fread_int(fl, 0, 9);
		reset_tab[reset_top].if_flag = tmp;
		reset_tab[reset_top].last = time(NULL) - number(0, 12 * 3600);
		// randomize last repop on boot
		reset_tab[reset_top].arg1 = fread_int(fl, -64000, LONG_MAX);
		reset_tab[reset_top].arg2 = fread_int(fl, -64000, LONG_MAX);
		if (reset_tab[reset_top].arg1 > 64000)
			reset_tab[reset_top].arg1 = 2;

		if (reset_tab[reset_top].arg2 > 64000)
			reset_tab[reset_top].arg2 = 1;

		if (reset_tab[reset_top].command == 'M' ||
				reset_tab[reset_top].command == 'O' ||
				reset_tab[reset_top].command == 'E' ||
				reset_tab[reset_top].command == 'P' ||
				reset_tab[reset_top].command == 'G' ||
				reset_tab[reset_top].command == 'D' ||
				reset_tab[reset_top].command == 'X' ||
				reset_tab[reset_top].command == 'K' ||
				reset_tab[reset_top].command == 'J'
						// % only has 2 args
						)
			reset_tab[reset_top].arg3 = fread_int(fl, -64000, INT_MAX);
		else
			reset_tab[reset_top].arg3 = 0;

		reset_tab[reset_top].lastPop = 0;

		if (reset_tab[reset_top].arg3 > 64000)
			reset_tab[reset_top].arg1 = 1;

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
			reset_tab[reset_top].comment = str_hsh(skipper);

		reset_top++;

	} // for( ;; ) til end of zone commands

#ifdef LEAK_CHECK
	zone_table[zon].cmd = ((struct reset_com *) calloc(reset_top, sizeof(struct reset_com)));
#else
	zone_table[zon].cmd = ((struct reset_com *)dc_alloc(reset_top, sizeof(struct reset_com)));
#endif

	zone_table[zon].reset_total = reset_top;

	// copy the temp into the memory
	for (i = 0; i < reset_top; i++)
			{
		zone_table[zon].cmd[i].command = reset_tab[i].command;
		zone_table[zon].cmd[i].if_flag = reset_tab[i].if_flag;
		zone_table[zon].cmd[i].arg1 = reset_tab[i].arg1;
		zone_table[zon].cmd[i].arg2 = reset_tab[i].arg2;
		zone_table[zon].cmd[i].arg3 = reset_tab[i].arg3;
		if (reset_tab[i].comment)
			zone_table[zon].cmd[i].comment = reset_tab[i].comment;
	}
}

/* load the zone table and command tables */
void boot_zones(void)
{
	FILE *fl;
	FILE *flZoneIndex;
	int zon = 0;
	extern short code_testing_mode;
	char * temp;
	char endfile[200]; // hopefully noone is stupid and makes a 180 char filename

//  for (zon = 0;zon < MAX_ZONE;zon++)
	//  zone_table[zon] = NULL; // Null list, top_of_z can't be used now

	if (!code_testing_mode)
	{
		if (!(flZoneIndex = dc_fopen(ZONE_INDEX_FILE, "r")))
		{
			perror("dc_fopen");
			log("boot_world: could not open world index file.", 0, LOG_BUG);
			abort();
		}
	} else if (!(flZoneIndex = dc_fopen(ZONE_INDEX_FILE_TINY, "r")))
	{
		perror("dc_fopen");
		log("boot_world: could not open world index file tiny.", 0, LOG_BUG);
		abort();
	}
	log("Booting individual zone files", 0, LOG_MISC);

	for (temp = read_next_worldfile_name(flZoneIndex);
			temp;
			temp = read_next_worldfile_name(flZoneIndex))
					{
		strcpy(endfile, "zonefiles/");
		strcat(endfile, temp);

		if (verbose_mode) {
			log(temp, 0, LOG_MISC);
		}

		if (!(fl = dc_fopen(endfile, "r")))
		{
			perror(endfile);
			logf(IMMORTAL, LOG_BUG, "boot_zone: could not open zone file: %s", endfile);
			abort();
		}
		//int num = 0;
		zone_table[zon].filename = strdup(temp);
		read_one_zone(fl, zon);
		zon++;
//    if (num > zon) zon = num;
		dc_fclose(fl);
		dc_free(temp);
	}

	log("Zone Boot done.", 0, LOG_MISC);

	dc_fclose(flZoneIndex);

	top_of_zone_table = --zon;
//  dc_fclose(fl);
}

/*************************************************************************
 *  procedures for resetting, both play-time and boot-time        *
 *********************************************************************** */

/* read a mobile from MOB_FILE */
CHAR_DATA *read_mobile(int nr, FILE *fl)
{
	char buf[200];
	int i, j;
	long tmp, tmp2, tmp3;
	CHAR_DATA *mob;
	char letter;

	extern struct race_shit race_info[];
	extern int mob_race_mod[][5];

	i = nr;

#ifdef LEAK_CHECK
	mob = (CHAR_DATA *)calloc(1, sizeof(CHAR_DATA));
#else
	mob = (CHAR_DATA *)dc_alloc(1, sizeof(CHAR_DATA));
#endif

	clear_char(mob);
	GET_RACE(mob) = 0;

	/***** String data *** */

	mob->name = fread_string(fl, 1);
	/* set up the fread debug stuff */
	curr_type = "Mob";
	curr_name = mob->name;
	mob->short_desc = fread_string(fl, 1);
	mob->long_desc = fread_string(fl, 1);
	mob->description = fread_string(fl, 1);
	mob->title = 0;

#ifdef LEAK_CHECK
	mob->mobdata = (mob_data *) calloc(1, sizeof(mob_data));
#else
	mob->mobdata = (mob_data *)dc_alloc(1, sizeof(mob_data));
#endif
	mob->mobdata->reset = NULL;
	/* *** Numeric data *** */
	j = 0;
	while ((tmp = fread_int(fl, LONG_MIN, LONG_MAX)) != -1) {
		mob->mobdata->actflags[j] = tmp;
		j++;
	}
	for (; j < ACT_MAX / ASIZE + 1; j++)
		mob->mobdata->actflags[j] = 0;
	if (ISSET(mob->mobdata->actflags, ACT_NOTRACK))
		REMBIT(mob->mobdata->actflags, ACT_NOTRACK);
	SET_BIT(mob->misc, MISC_IS_MOB);

	j = 0;
	while ((tmp = fread_int(fl, LONG_MIN, LONG_MAX)) != -1) {
		mob->affected_by[j] = tmp;
		j++;
	}
	for (; j < AFF_MAX / ASIZE + 1; j++)
		mob->affected_by[j] = 0;

	mob->alignment = fread_int(fl, LONG_MIN, LONG_MAX);

	tmp = fread_int(fl, 0, MAX_RACE);
	GET_RACE(mob) = (char)tmp;

	mob->raw_str = mob->str = BASE_STAT + mob_race_mod[GET_RACE(mob)][0];
	mob->raw_dex = mob->dex = BASE_STAT + mob_race_mod[GET_RACE(mob)][1];
	mob->raw_con = mob->con = BASE_STAT + mob_race_mod[GET_RACE(mob)][2];
	mob->raw_intel = mob->intel = BASE_STAT + mob_race_mod[GET_RACE(mob)][3];
	mob->raw_wis = mob->wis = BASE_STAT + mob_race_mod[GET_RACE(mob)][4];

	GET_LEVEL(mob) = fread_int(fl, 0, IMP);

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
	mob->mobdata->last_room = -1;
	mob->mana = 100 + (mob->level * 10);
	mob->max_mana = 100 + (mob->level * 10);

	mob->move = 100 + (mob->level * 10);
	mob->max_move = 100 + (mob->level * 10);

	mob->gold = fread_int(fl, 0, LONG_MAX);
	mob->plat = 0;
	GET_EXP(mob) = (int64)fread_int(fl, LONG_MIN, LONG_MAX);

	mob->position = fread_int(fl, 0, 10);
	mob->mobdata->default_pos = fread_int(fl, 0, 10);

	tmp = fread_int(fl, 0, 12);

	/* Read in ISR vlues...  (sex +3) */
	// Eventually I can remove this "if" but not until I fix them all.
	if (tmp > 2)
		tmp -= 3;

	mob->sex = (char)tmp;

	mob->immune = fread_bitvector(fl, 0, LONG_MAX);
	mob->suscept = fread_bitvector(fl, 0, LONG_MAX);
	mob->resist = fread_bitvector(fl, 0, LONG_MAX);

	// if all three are 0, then chances are someone just didn't set them, so go with
	// the race defaults.
//    if(mob->immune == 0 && mob->suscept == 0 && mob->resist == 0)
	//  {
	SET_BIT(mob->immune, race_info[(int)GET_RACE(mob)].immune);
	SET_BIT(mob->suscept, race_info[(int)GET_RACE(mob)].suscept);
	SET_BIT(mob->resist, race_info[(int)GET_RACE(mob)].resist);
// TOODO:FIXTHIS         SETBIT(mob->affected_by, race_info[(int)GET_RACE(mob)].affects);
	//      mob->immune  = race_info[(int)GET_RACE(mob)].immune;
	//    mob->suscept = race_info[(int)GET_RACE(mob)].suscept;
	//  mob->resist  = race_info[(int)GET_RACE(mob)].resist;
//    }

	mob->c_class = 0;

	do {
		letter = fread_char(fl);
		switch (letter) {
		case 'C':
			mob->c_class = fread_int(fl, 0, LONG_MAX);
			fread_new_newline(fl);
			break;
		case 'T': // sTats
			mob->raw_str = mob->str = fread_int(fl, 0, 100);
			mob->raw_intel = mob->intel = fread_int(fl, 0, 100);
			mob->raw_wis = mob->wis = fread_int(fl, 0, 100);
			mob->raw_dex = mob->dex = fread_int(fl, 0, 100);
			mob->raw_con = mob->con = fread_int(fl, 0, 100);
			fread_int(fl, 0, 100); // junk var in case we add another stat
			fread_new_newline(fl);
			break;
		case '>':
			ungetc(letter, fl);
			mprog_read_programs(fl, nr, FALSE);
			break;
		case 'Y': // type
			mob->mobdata->mob_flags.type = (mob_type_t)fread_int(fl, mob_type_t::MOB_TYPE_FIRST, mob_type_t::MOB_TYPE_LAST);
			fread_new_newline(fl);
			break;
		case 'V': // value
			i = fread_int(fl, 0, MAX_MOB_VALUES - 1);
			mob->mobdata->mob_flags.value[i] = fread_int(fl, -1000, LONG_MAX);
			fread_new_newline(fl);
			break;
		case 'S':
			break;
		default:
			sprintf(buf, "Mob %s: Invalid additional flag.  (Class, S, etc)", mob->short_desc);
			log(buf, 0, LOG_BUG);
			break;
		}
	} while (letter != 'S');

	fread_new_newline(fl);

	mob->weight = 200;
	mob->height = 198;

	for (i = 0; i < 3; i++)
		GET_COND(mob, i) = -1;

// TODO - eventually have mob saving throws work by race too, but this should be good for now

	for (i = 0; i <= SAVE_TYPE_MAX; i++)
		mob->saves[i] = GET_LEVEL(mob) / 3;

	if (IS_SET(mob->resist, ISR_FIRE))
		mob->saves[SAVE_TYPE_FIRE] += 50;
	if (IS_SET(mob->resist, ISR_ACID))
		mob->saves[SAVE_TYPE_ACID] += 50;
	if (IS_SET(mob->resist, ISR_POISON))
		mob->saves[SAVE_TYPE_POISON] += 50;
	if (IS_SET(mob->resist, ISR_COLD))
		mob->saves[SAVE_TYPE_COLD] += 50;
	if (IS_SET(mob->resist, ISR_ENERGY))
		mob->saves[SAVE_TYPE_ENERGY] += 50;
	if (IS_SET(mob->resist, ISR_MAGIC))
		mob->saves[SAVE_TYPE_MAGIC] += 50;

	if (IS_SET(mob->suscept, ISR_FIRE))
		mob->saves[SAVE_TYPE_FIRE] -= 50;
	if (IS_SET(mob->suscept, ISR_ACID))
		mob->saves[SAVE_TYPE_ACID] -= 50;
	if (IS_SET(mob->suscept, ISR_POISON))
		mob->saves[SAVE_TYPE_POISON] -= 50;
	if (IS_SET(mob->suscept, ISR_COLD))
		mob->saves[SAVE_TYPE_COLD] -= 50;
	if (IS_SET(mob->suscept, ISR_ENERGY))
		mob->saves[SAVE_TYPE_ENERGY] -= 50;
	if (IS_SET(mob->suscept, ISR_MAGIC))
		mob->saves[SAVE_TYPE_MAGIC] -= 50;

	mob->mobdata->nr = nr;
	mob->desc = 0;

	return (mob);
}

// we write them recursively so they read in properly
void write_mprog_recur(FILE *fl, MPROG_DATA *mprg, bool mob)
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

void write_mprog_recur(ofstream &fl, MPROG_DATA *mprg, bool mob)
{
	if (mprg->next) {
		write_mprog_recur(fl, mprg->next, mob);
	}

	if (mob) {
		fl << ">" << mprog_type_to_name(mprg->type) << " ";
	} else {
		fl << "\\" << mprog_type_to_name(mprg->type) << " ";
	}

	if (mprg->arglist) {
		string_to_file(fl, mprg->arglist);
	} else {
		string_to_file(fl, "Saved During Edit");
	}

	if (mprg->comlist) {
		string_to_file(fl, mprg->comlist);
	} else {
		string_to_file(fl, "Saved During Edit");
	}
}

// Write a mob to file
// Assume valid mob, and file open for writing
//
void write_mobile(char_data * mob, FILE *fl)
{
	int i = 0;
	extern int mob_race_mod[][5];

	fprintf(fl, "#%d\n", mob_index[mob->mobdata->nr].virt);
	string_to_file(fl, mob->name);
	string_to_file(fl, mob->short_desc);
	string_to_file(fl, mob->long_desc);
	string_to_file(fl, mob->description);

	while (i < ACT_MAX / ASIZE + 1) {
		fprintf(fl, "%d ", mob->mobdata->actflags[i]);
		i++;
	}
	fprintf(fl, "-1\n");
	i = 0;

	while (i < AFF_MAX / ASIZE + 1) {
		fprintf(fl, "%d ", mob->affected_by[i]);
		i++;
	}
	fprintf(fl, "-1\n");

	fprintf(fl, "%d %d %d\n"
			"%d %d %dd%d+%d %dd%d+%d\n"
			"%lld %lld\n"
			"%d %d %d %d %d %d\n",
			mob->alignment,
			GET_RACE(mob),
			GET_LEVEL(mob),

			(20 - mob->hitroll),
			(int)(mob->armor / 10),
			GET_MAX_HIT(mob),
			1,
			0,
			mob->mobdata->damnodice,
			mob->mobdata->damsizedice,
			mob->damroll,

			mob->gold,
			GET_EXP(mob),

			mob->position,
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

	if (mob_index[mob->mobdata->nr].mobprogs) {
		write_mprog_recur(fl, mob_index[mob->mobdata->nr].mobprogs, TRUE);
		fprintf(fl, "|\n");
	}

	if (mob->mobdata->mob_flags.type > 0) {
		fprintf(fl, "Y %d\n", mob->mobdata->mob_flags.type);
		for (uint32_t i = 0; i < MAX_MOB_VALUES; ++i) {
			fprintf(fl, "V %d %d\n", i, mob->mobdata->mob_flags.value[i]);
		}
	}

	fprintf(fl, "S\n");
}

// If a mob is set to 0d0 we need to give it hps depending upon it's level
// and class.  And then since it's a mob, a bonus:)
//
void handle_automatic_mob_damdice(char_data * mob)
{
	int nodice = 1;
	int sizedice = 1;

	// set dependant on level
	if (GET_LEVEL(mob) < 5) {
		nodice = 1;
		sizedice = 2;
	}
	else if (GET_LEVEL(mob) < 10) {
		nodice = 1;
		sizedice = 4;
	}
	else if (GET_LEVEL(mob) < 15) {
		nodice = 2;
		sizedice = 3;
	}
	else if (GET_LEVEL(mob) < 20) {
		nodice = 2;
		sizedice = 4;
	}
	else if (GET_LEVEL(mob) < 25) {
		nodice = 3;
		sizedice = 3;
	}
	else if (GET_LEVEL(mob) < 30) {
		nodice = 3;
		sizedice = 4;
	}
	else if (GET_LEVEL(mob) < 35) {
		nodice = 4;
		sizedice = 3;
	}
	else if (GET_LEVEL(mob) < 40) {
		nodice = 4;
		sizedice = 4;
	}
	else if (GET_LEVEL(mob) < 45) {
		nodice = 5;
		sizedice = 3;
	}
	else if (GET_LEVEL(mob) < 50) {
		nodice = 5;
		sizedice = 4;
	}
	else if (GET_LEVEL(mob) < 55) {
		nodice = 5;
		sizedice = 5;
	}
	else if (GET_LEVEL(mob) < 60) {
		nodice = 6;
		sizedice = 6;
	}
	else if (GET_LEVEL(mob) < 65) {
		nodice = 7;
		sizedice = 7;
	}
	else if (GET_LEVEL(mob) < 70) {
		nodice = 8;
		sizedice = 8;
	}
	else {
		nodice = 10;
		sizedice = 10;
	}

	// small class adjustment
	switch (GET_CLASS(mob)) {
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

void handle_automatic_mob_hitpoints(char_data * mob)
{
	int base;

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

	if (GET_LEVEL(mob) > 0)
		base *= GET_LEVEL(mob);

	// mobs get auto hp bonus
	base = (int)(base * 1.5);

	mob->raw_hit = base;
	mob->hit = base;
	mob->max_hit = base;
}

// currently hit and dam are the same
void handle_automatic_mob_hitdamroll(char_data * mob)
{
	int curhit;

	curhit = GET_LEVEL(mob);

	if (GET_LEVEL(mob) > 1 && GET_LEVEL(mob) < 11)
		curhit--;

	if (GET_LEVEL(mob) > 25)
		curhit++;

	if (GET_LEVEL(mob) > 30)
		curhit++;

	if (GET_LEVEL(mob) > 35)
		curhit++;

	if (GET_LEVEL(mob) > 40)
		curhit++;

	if (GET_LEVEL(mob) > 45)
		curhit++;

	if (GET_LEVEL(mob) > 49)
		curhit++;

	if (GET_LEVEL(mob) > 54)
		curhit += 5;

	mob->hitroll = curhit;
	mob->damroll = curhit;
}

void handle_automatic_mob_settings(char_data * mob)
{
	extern struct mob_matrix_data mob_matrix[];
	// New matrix is handled here.
	if (ISSET(mob->mobdata->actflags, ACT_NOMATRIX))
		return;
	if (mob->level > 110)
		return;
	int baselevel = mob->level;
	float alevel = (float)mob->level;

	int percent = number(-3, 3);

	if (mob->c_class != 0)
		alevel -= mob->level > 20 ? 3.0 : 2.0;
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

	if (mob->gold != 0)
		mob->gold = mob_matrix[baselevel].gold + number(0 - (mob_matrix[baselevel].gold / 10), mob_matrix[baselevel].gold / 10);
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

	baselevel = MAX(alevel > 0 ? (int )alevel : 1, baselevel - 4);

	mob->hitroll = mob_matrix[baselevel].tohit;
	mob->damroll = mob_matrix[baselevel].todam;
	if (ISSET(mob->mobdata->actflags, ACT_BOSS))
		mob->armor = (int)(mob_matrix[baselevel].armor * 1.5);
	else
		mob->armor = mob_matrix[baselevel].armor;
	mob->max_hit = mob->raw_hit = mob->hit = mob_matrix[baselevel].hitpoints
			+ ((mob_matrix[baselevel].hitpoints / 100) * percent);
}

CHAR_DATA *clone_mobile(int nr)
{
	int i;
	CHAR_DATA *mob, *old;

	if (nr < 0)
		return 0;

#ifdef LEAK_CHECK
	mob = (CHAR_DATA *)calloc(1, sizeof(CHAR_DATA));
#else
	mob = (CHAR_DATA *)dc_alloc(1, sizeof(CHAR_DATA));
#endif

	clear_char(mob);
	old = ((CHAR_DATA *)(mob_index[nr].item)); /* cast void pointer */

	*mob = *old;

#ifdef LEAK_CHECK
	mob->mobdata = (mob_data *)calloc(1, sizeof(mob_data));
#else
	mob->mobdata = (mob_data *)dc_alloc(1, sizeof(mob_data));
#endif

	memcpy(mob->mobdata, old->mobdata, sizeof(mob_data));

	for (i = 0; i < MAX_WEAR; i++) /* Initialisering Ok */
		mob->equipment[i] = 0;

	mob->mobdata->nr = nr;
	mob->desc = 0;
	mob->mobdata->reset = NULL;

	auto &character_list = DC::instance().character_list;
	character_list.insert(mob);
	mob_index[nr].number++;
	mob->next_in_room = 0;

	handle_automatic_mob_settings(mob);
	float mult = 1.0;
	if (!ISSET(mob->mobdata->actflags, ACT_NOMATRIX))
			{
		if (GET_LEVEL(mob) > 100) {
			mult = 1.5;
		}
		else if (GET_LEVEL(mob) > 95) {
			mult = 1.4;
		}
		else if (GET_LEVEL(mob) > 90) {
			mult = 1.3;
		}
		else if (GET_LEVEL(mob) > 85) {
			mult = 1.2;
		}
		else if (GET_LEVEL(mob) > 75) {
			mult = 1.1;
		}
	}
	mob->max_hit = mob->raw_hit = mob->hit = (int32)(mob->max_hit * mult);
	mob->mobdata->damnodice = (int16)(mob->mobdata->damnodice * mult);
	mob->mobdata->damsizedice = (int16)(mob->mobdata->damsizedice * mult);
	mob->damroll = (int16)(mob->damroll * mult);
	mob->hometown = old->in_room;
	return (mob);
}

// add a new item to the index.  To do this, we need to update ALL the
// other items in the game after the one being inserted.  Pain in the
// ass but oh well.  it shouldn't hopefully happen that often.
// 
// Args:  int nr = virtual number of object (what gods know it as)
//
// return index of item on success, -1 on failure
//
int create_blank_item(int nr)
{
	struct obj_data *obj;
	struct obj_data *curr;
	int cur_index = 0;

	// check if room available in index
	if ((top_of_objt + 1) >= MAX_INDEX)
		return -1;

	// find how where our index will be
	// yes, i could check if the last item is smaller and then do a binary
	// search to do this faster but if everything in life was optimized I wouldn't
	// be playing solitaire at work on a windows machine. -pir
	while (obj_index[cur_index].virt < nr && cur_index < top_of_objt + 1)
		cur_index++;

	if (obj_index[cur_index].virt == nr) // item already exists
		return -1;

	// theoretically if top_of_objt+1 wasn't initialized properly it could
	// be junk data, which could be == nr, returning -1, but i'm not gonna worry about it

	// create

#ifdef LEAK_CHECK
	obj = (struct obj_data *)calloc(1, sizeof(struct obj_data));
#else
	obj = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
#endif

	clear_object(obj);
	obj->name = str_hsh("empty obj");
	obj->short_description = str_hsh("An empty obj");
	obj->description = str_hsh("An empty obj sits here dejectedly.");
	obj->action_description = str_hsh("Fixed.");
	obj->in_room = NOWHERE;
	obj->next_content = 0;
	obj->next_skill = 0;
	obj->table = 0;
	obj->carried_by = 0;
	obj->equipped_by = 0;
	obj->in_obj = 0;
	obj->contains = 0;
	obj->item_number = cur_index;
	obj->ex_description = 0;
	// shift > items right
	memmove(&obj_index[cur_index + 1], &obj_index[cur_index],
			((top_of_objt - cur_index + 1) * sizeof(index_data))
			);
	top_of_objt++;

	// insert
	obj_index[cur_index].virt = nr;
	obj_index[cur_index].number = 0;
	obj_index[cur_index].non_combat_func = 0;
	obj_index[cur_index].combat_func = 0;
	obj_index[cur_index].item = obj;

	// update index of all items in game
	for (curr = object_list; curr; curr = curr->next)
		if (curr->item_number >= cur_index)
			curr->item_number++;

	// update index of all the obj prototypes
	for (int i = cur_index + 1; i <= top_of_objt; i++)
		((obj_data *)obj_index[i].item)->item_number++;

	// update obj file indices
	world_file_list_item * wcurr = NULL;

	extern world_file_list_item * obj_file_list;

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
// Args:  int nr = virtual number of object (what gods know it as)
//
// return index of item on success, -1 on failure
//  Hack of create_blank_item.. Uriz
int create_blank_mobile(int nr)
{
	CHAR_DATA *mob;
	int cur_index = 0;

	// check if room available in index
	if ((top_of_mobt + 1) >= MAX_INDEX)
		return -1;

	// find how where our index will be
	// yes, i could check if the last mobile is smaller and then do a binary
	// search to do this faster but if everything in life was optimized I wouldn't
	// be playing solitaire at work on a windows machine. -pir
	while (mob_index[cur_index].virt < nr && cur_index < top_of_mobt + 1)
		cur_index++;

	if (mob_index[cur_index].virt == nr) // item already exists
		return -1;

	// theoretically if top_of_objt+1 wasn't initialized properly it could
	// be junk data, which could be == nr, returning -1, but i'm not gonna worry

	// create

#ifdef LEAK_CHECK
	mob = (struct char_data *)calloc(1, sizeof(struct char_data));
#else
	mob = (struct char_data *)dc_alloc(1, sizeof(struct char_data));
#endif

	clear_char(mob);
	reset_char(mob);
	mob->name = str_hsh("empty mob");
	mob->short_desc = str_hsh("an empty mob");
	mob->long_desc = str_hsh("an empty mob description");
	mob->description = str_hsh("");
	mob->title = 0;
	mob->fighting = 0;
	mob->pcdata = 0;
	mob->altar = 0;
	mob->desc = 0;
	GET_RAW_DEX(mob) = 11;
	GET_RAW_STR(mob) = 11;
	GET_RAW_INT(mob) = 11;
	GET_RAW_WIS(mob) = 11;
	GET_RAW_CON(mob) = 11;
	mob->height = 198;
	mob->weight = 200;
#ifdef LEAK_CHECK
	mob->mobdata = (mob_data *) calloc(1, sizeof(mob_data));
#else
	mob->mobdata = (mob_data *)dc_alloc(1, sizeof(mob_data));
#endif
	int i;
	for (i = 0; i < ACT_MAX / ASIZE + 1; i++)
		mob->mobdata->actflags[i] = 0;
	for (i = 0; i < AFF_MAX / ASIZE + 1; i++)
		mob->affected_by[i] = 0;
	mob->mobdata->reset = NULL;
	mob->mobdata->damnodice = 1;
	mob->mobdata->damsizedice = 1;
	mob->mobdata->default_pos = POSITION_STANDING;
	mob->mobdata->last_room = -1;
	mob->mobdata->nr = cur_index;
	mob->misc = MISC_IS_MOB;

	// shift > items right
	memmove(&mob_index[cur_index + 1], &mob_index[cur_index],
			((top_of_mobt - cur_index + 1) * sizeof(index_data))
			);
	top_of_mobt++;

	// insert
	mob_index[cur_index].virt = nr;
	mob_index[cur_index].number = 0;
	mob_index[cur_index].non_combat_func = 0;
	mob_index[cur_index].combat_func = 0;
	mob_index[cur_index].item = mob;

	mob_index[cur_index].mobprogs = 0;
	mob_index[cur_index].mobspec = 0;
	mob_index[cur_index].progtypes = 0;

	// update index of all mobiles in game
	auto &character_list = DC::instance().character_list;
	for_each(character_list.begin(), character_list.end(),
	        [&cur_index](char_data * const &curr) {
		if (IS_MOB(curr))
			if (curr->mobdata->nr >= cur_index)
				curr->mobdata->nr++;
	});


	// update index of all the mob prototypes
	for (i = cur_index + 1; i <= top_of_mobt; i++)
		((char_data *)mob_index[i].item)->mobdata->nr++;

	// update obj file indices
	world_file_list_item * wcurr = NULL;

	extern world_file_list_item * mob_file_list;

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
	extern struct shop_data shop_index[MAX_SHOP];
	//   int i;
	for (i = 0; i < MAX_SHOP; i++)
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
// Args:  int nr = real number of object (index in array)
//
// return index of mobile on success, -1 on failure
//
void delete_mob_from_index(int nr)
{
	int i = 0, j = 0;

	if (nr < 0 || nr > top_of_mobt) // doesn't exist!
		return;

	dc_free(mob_index[nr].item);
	// shift > items left
	memmove(&mob_index[nr], &mob_index[nr + 1],
			((top_of_mobt - nr) * sizeof(index_data))
			);
	top_of_mobt--;

	// update index of all mobiles in game - these store rnums
	auto &character_list = DC::instance().character_list;
	for_each(character_list.begin(), character_list.end(),
	        [&nr](char_data * const &curr) {
		if (IS_NPC(curr) && curr->mobdata->nr >= nr)
			curr->mobdata->nr--;
	});

	// update index of all the mob prototypes
	for (i = nr; i <= top_of_mobt; i++)
		((CHAR_DATA *)mob_index[i].item)->mobdata->nr--;

	// update mob file indices - these store rnums
	world_file_list_item * wcurr = NULL;

	extern world_file_list_item * mob_file_list;

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
	for (i = 0; i <= top_of_zonet; i++)
			{
		for (j = 0; zone_table[i].cmd[j].command != 'S'; j++)
				{
			switch ( zone_table[i].cmd[j].command)
			{
			case 'M': // just #1
				if ( zone_table[i].cmd[j].arg1 >= nr)
					zone_table[i].cmd[j].arg1--;
				break;
			default:
				break;
			}
		}
	}
	/*
	 Shop fixes follow.
	 */
	extern struct shop_data shop_index[MAX_SHOP];
	int z;
	for (z = 0; z < MAX_SHOP; z++)
			{
		if (shop_index[z].keeper >= nr)
			shop_index[z].keeper--;
	}

}

// Delete an item from the index and update everything to continue working
// without causing mass destruction and chaos.
//
// Note:  ALL copies of this item must have been removed from the game
// before calling this function.  Otherwise these old items will think
// they are the restrung version of the item that now holds that index.
// 
// Args:  int nr = real number of object (index in array)
//
// return index of item on success, -1 on failure
//
void delete_item_from_index(int nr)
{
	int i = 0, j = 0;
	struct obj_data * curr;

	if (nr < 0 || nr > top_of_objt) // doesn't exist!
		return;

	dc_free(obj_index[nr].item);

	// shift > items left
	memmove(&obj_index[nr], &obj_index[nr + 1],
			((top_of_objt - nr) * sizeof(index_data))
			);
	top_of_objt--;

	// update index of all items in game - these store rnums
	for (curr = object_list; curr; curr = curr->next)
		if (curr->item_number >= nr)
			curr->item_number--;

	// update index of all the obj prototypes
	for (i = nr; i <= top_of_objt; i++)
		((obj_data *)obj_index[i].item)->item_number--;

	// update obj file indices - these store rnums
	world_file_list_item * wcurr = NULL;

	extern world_file_list_item * obj_file_list;

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
	for (i = 0; i <= top_of_zonet; i++)
			{
		for (j = 0; zone_table[i].cmd[j].command != 'S'; j++)
				{
			switch ( zone_table[i].cmd[j].command)
			{
			case 'P': // 1 and 3
				if ( zone_table[i].cmd[j].arg3 >= nr)
					zone_table[i].cmd[j].arg3--;
				// no break here on purpose so it falls through and does the '1'
			case 'O':
				case 'G':
				case 'E': // 1 only
				if ( zone_table[i].cmd[j].arg1 >= nr)
					zone_table[i].cmd[j].arg1--;
				break;
			default:
				break;
			}
		}
	}
}

/* read an object from OBJ_FILE */
struct obj_data *read_object(int nr, FILE *fl, bool zz)
{
	struct obj_data *obj;
	int loc, mod;

	char chk;
	struct extra_descr_data *new_new_descr;

	if (nr < 0) {
		return 0;
	}

#ifdef LEAK_CHECK
	obj = (struct obj_data *)calloc(1, sizeof(struct obj_data));
#else
	obj = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
#endif

	clear_object(obj);

	/* *** string data *** */
	// read it, add it to the hsh table, free it
	// that way, we only have one copy of it in memory at any time
	obj->name = fread_string(fl, 1);
	char *tmpptr;

	tmpptr = fread_string(fl, 1);

	if (strlen(tmpptr) >= MAX_OBJ_SDESC_LENGTH) {
		tmpptr[MAX_OBJ_SDESC_LENGTH - 1] = 0;

		obj->short_description = str_dup(tmpptr);
		free(tmpptr);

		logf( IMMORTAL, LOG_BUG, "read_object: vnum %d short_description too long.", obj_index[nr].virt);
	} else {
		obj->short_description = tmpptr;
	}

	obj->description = fread_string(fl, 1);
	obj->action_description = fread_string(fl, 1);
	if ((obj->action_description && (obj->action_description[0] < ' ' || obj->action_description[0] > '~'))
			&& obj->action_description[0] != '\0') {
		logf( IMMORTAL, LOG_BUG, "read_object: vnum %d action description [%s] removed.", obj_index[nr].virt, obj->action_description);
		obj->action_description[0] = '\0';
	}
	obj->table = 0;
	curr_virtno = nr;
	curr_name = obj->name;
	curr_type = "Object";

	/* *** numeric data *** */

	obj->obj_flags.type_flag = fread_int(fl, -1000, LONG_MAX);
	obj->obj_flags.extra_flags = fread_bitvector(fl, 0, LONG_MAX);
	obj->obj_flags.wear_flags = fread_bitvector(fl, 0, LONG_MAX);
	obj->obj_flags.size = fread_bitvector(fl, 0, LONG_MAX);

	obj->obj_flags.value[0] = fread_int(fl, -1000, LONG_MAX);
	obj->obj_flags.value[1] = fread_int(fl, -1000, LONG_MAX);
	obj->obj_flags.value[2] = fread_int(fl, -1000, LONG_MAX);
	obj->obj_flags.value[3] = fread_int(fl, -1000, LONG_MAX);
	obj->obj_flags.eq_level = fread_int(fl, -1000, IMP);
	obj->obj_flags.weight = fread_int(fl, -1000, LONG_MAX);
	obj->obj_flags.cost = fread_int(fl, -1000, LONG_MAX);
	obj->obj_flags.more_flags = fread_bitvector(fl, -1000, LONG_MAX);

	/* currently not stored in object file */
	obj->obj_flags.timer = 0;

	obj->ex_description = NULL;
	obj->affected = NULL;
	obj->num_affects = 0;
	/* *** other flags *** */

	fscanf(fl, "%c\n", &chk);
	while (chk != 'S')
	{
		switch (chk) {
		// skip whitespace
		case ' ':
			case '\n':
			break;
		case 'E':
			#ifdef LEAK_CHECK
			new_new_descr = (struct extra_descr_data *) calloc(1, sizeof(struct extra_descr_data));
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
			mprog_read_programs(fl, nr, zz);
			break;

		case 'A':
			// these are only two members of obj_affected_type, so nothing else needs initializing
			loc = fread_int(fl, -1000, LONG_MAX);
			mod = fread_int(fl, -1000, 1000);
			add_obj_affect(obj, loc, mod);
			break;

		default:
			sprintf(log_buf, "Illegal obj addon flag %c in obj %s.", chk, obj->name);
			log(log_buf, IMP, LOG_BUG);
			break;
		} // switch
		  // read in next flag
		fscanf(fl, "%c\n", &chk);
	}

	obj->in_room = NOWHERE;
	obj->next_skill = 0;
	obj->next_content = 0;
	obj->carried_by = 0;
	obj->equipped_by = 0;
	obj->in_obj = 0;
	obj->contains = 0;
	obj->item_number = nr;

	return obj;
}

ifstream& operator>>(ifstream &in, obj_data *obj)
{
	int loc, mod, nr;

	char chk, c;
	struct extra_descr_data *new_new_descr;

	if (obj == NULL) {
		return in;
	}

	clear_object(obj);
	in >> c;
	if (c == '#') {
		in >> nr;
	}
	in >> ws;

	obj->name = fread_string(in, true);

	char *tmpptr;
	tmpptr = fread_string(in, true);

	if (strlen(tmpptr) >= MAX_OBJ_SDESC_LENGTH) {
		tmpptr[MAX_OBJ_SDESC_LENGTH - 1] = 0;

		obj->short_description = str_dup(tmpptr);
		free(tmpptr);

		logf( IMMORTAL, LOG_BUG, "read_object: vnum unknown short_description too long.");
	} else {
		obj->short_description = tmpptr;
	}
	obj->description = fread_string(in, 1);
	obj->action_description = fread_string(in, 1);
	obj->table = 0;
	curr_virtno = nr;
	curr_name = obj->name;
	curr_type = "Object";

	// numeric data

	obj->obj_flags.type_flag = fread_int(in, -1000, LONG_MAX);

	obj->obj_flags.extra_flags = fread_bitvector(in, 0, LONG_MAX);
	obj->obj_flags.wear_flags = fread_bitvector(in, 0, LONG_MAX);
	obj->obj_flags.size = fread_bitvector(in, 0, LONG_MAX);

	obj->obj_flags.value[0] = fread_int(in, -1000, LONG_MAX);
	obj->obj_flags.value[1] = fread_int(in, -1000, LONG_MAX);
	obj->obj_flags.value[2] = fread_int(in, -1000, LONG_MAX);
	obj->obj_flags.value[3] = fread_int(in, -1000, LONG_MAX);
	obj->obj_flags.eq_level = fread_int(in, -1000, IMP);
	obj->obj_flags.weight = fread_int(in, -1000, LONG_MAX);
	obj->obj_flags.cost = fread_int(in, -1000, LONG_MAX);
	obj->obj_flags.more_flags = fread_bitvector(in, -1000, LONG_MAX);

	// currently not stored in object file
	obj->obj_flags.timer = 0;

	obj->ex_description = NULL;
	obj->affected = NULL;
	obj->num_affects = 0;
	// other flags

	in >> chk;

	while (chk != 'S')
	{
		switch (chk) {
		// skip whitespace
		case ' ':
			case '\n':
			break;
		case 'E':
			#ifdef LEAK_CHECK
			new_new_descr = (struct extra_descr_data *) calloc(1, sizeof(struct extra_descr_data));
#else
			new_new_descr = (struct extra_descr_data *)dc_alloc(1, sizeof(struct extra_descr_data));
#endif
			new_new_descr->keyword = fread_string(in, 1);
			new_new_descr->description = fread_string(in, 1);
			new_new_descr->next = obj->ex_description;
			obj->ex_description = new_new_descr;
			break;

		case '\\':
			//ungetc( '\\', in );
			//mprog_read_programs( in, nr,zz );
			break;

		case 'A':
			// these are only two members of obj_affected_type, so nothing else needs initializing
			loc = fread_int(in, -1000, LONG_MAX);
			mod = fread_int(in, -1000, 1000);
			add_obj_affect(obj, loc, mod);
			break;

		default:
			sprintf(log_buf, "Illegal obj addon flag %c in obj %s.", chk, obj->name);
			log(log_buf, IMP, LOG_BUG);
			break;
		} // switch
		  // read in next flag
		in >> chk;
	}

	obj->in_room = NOWHERE;
	obj->next_skill = 0;
	obj->next_content = 0;
	obj->carried_by = 0;
	obj->equipped_by = 0;
	obj->in_obj = 0;
	obj->contains = 0;
	obj->item_number = 0;

	return in;
}

// write an object to file
// This assumes that the object is valid, and the file is open for writing
//
void write_object(obj_data * obj, FILE *fl)
{
	struct extra_descr_data * currdesc;

	fprintf(fl, "#%d\n", obj_index[obj->item_number].virt);
	string_to_file(fl, obj->name);
	string_to_file(fl, obj->short_description);
	string_to_file(fl, obj->description);
	string_to_file(fl, obj->action_description);

	fprintf(fl, "%d %d %d %d\n"
			"%d %d %d %d %d\n"
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
	while (currdesc) {
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

	if (obj_index[obj->item_number].mobprogs) {
		write_mprog_recur(fl, obj_index[obj->item_number].mobprogs, FALSE);
		fprintf(fl, "|\n");
	}

	fprintf(fl, "S\n");
}

ofstream& operator<<(ofstream &out, obj_data *obj)
{
	out << "#" << obj_index[obj->item_number].virt << "\n";
	string_to_file(out, obj->name);
	string_to_file(out, obj->short_description);
	string_to_file(out, obj->description);
	string_to_file(out, obj->action_description);

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
	while (currdesc) {
		out << "E\n";
		string_to_file(out, currdesc->keyword);
		string_to_file(out, currdesc->description);
		currdesc = currdesc->next;
	}

	for (int i = 0; i < obj->num_affects; i++) {
		out << "A\n";
		out << obj->affected[i].location << " "
				<< obj->affected[i].modifier << "\n";
	}

	if (obj_index[obj->item_number].mobprogs) {
		write_mprog_recur(out, obj_index[obj->item_number].mobprogs, FALSE);
		out << "|\n";
	}

	out << "S\n";

	return out;
}

string quotequotes(string &s1);

string quotequotes(const char *str)
{
	string s1(str);

	return quotequotes(s1);
}

string quotequotes(string &s1)
{
	size_t pos = s1.find('\"');
	while (pos != string::npos) {
		s1.insert(pos, 1, '\"');
		pos = s1.find('\"', pos + 2);
	}

	return s1;
}

string lf_to_crlf(string &s1)
{
	size_t pos = s1.find('\n');
	while (pos != string::npos) {
		s1.insert(pos, 1, '\r');
		pos = s1.find('\n', pos + 2);
	}

	return s1;
}

void write_bitvector_csv(unsigned long vector, char * const *array, ofstream &fout)
{
	int nr = 0;
	while (*array[nr] != '\n') {
		if (IS_SET(1, vector)) {
			fout << array[nr];
		}

		fout << ",";
		vector >>= 1;
		nr++;
	}

	return;
}

void write_object_csv(obj_data * obj, ofstream &fout)
{
	try {
		fout << obj_index[obj->item_number].virt << ",";
		fout << "\"" << obj->name << "\",";
		fout << "\"" << quotequotes(obj->short_description) << "\",";
		fout << "\"" << quotequotes(obj->description) << "\",";
		fout << "\"" << quotequotes(obj->action_description) << "\",";
		fout << item_types[obj->obj_flags.type_flag] << ",";
		fout << obj->obj_flags.size << ",";
		fout << obj->obj_flags.value[0] << ",";
		fout << obj->obj_flags.value[1] << ",";
		fout << obj->obj_flags.value[2] << ",";
		fout << obj->obj_flags.value[3] << ",";
		fout << obj->obj_flags.eq_level << ",";
		fout << obj->obj_flags.weight << ",";
		fout << obj->obj_flags.cost << ",";

		write_bitvector_csv(obj->obj_flags.wear_flags, wear_bits, fout);
		write_bitvector_csv(obj->obj_flags.extra_flags, extra_bits, fout);
		write_bitvector_csv(obj->obj_flags.more_flags, more_obj_bits, fout);

		char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
		for (int i = 0; i < obj->num_affects; i++) {
			if (obj->affected[i].location < 1000)
				sprinttype(obj->affected[i].location, apply_types, buf2);
			else if (get_skill_name(obj->affected[i].location / 1000))
				strcpy(buf2, get_skill_name(obj->affected[i].location / 1000));
			else
				strcpy(buf2, "Invalid");

			sprintf(buf, "%s by %d", buf2, obj->affected[i].modifier);
			fout << buf;

			if (i + 1 < obj->num_affects) {
				fout << " ";
			}
		}

	} catch (ofstream::failure &e) {
		stringstream errormsg;
		errormsg << "Exception while writing in write_obj_csv.";
		log(errormsg.str().c_str(), 108, LOG_MISC);
	}

	fout << endl;
}

bool has_random(OBJ_DATA *obj)
{

	return ((obj_index[obj->item_number].progtypes & RAND_PROG) || (obj_index[obj->item_number].progtypes & ARAND_PROG));
}

/* clone an object from obj_index */
struct obj_data *clone_object(int nr)
{
	struct obj_data *obj, *old;
	struct extra_descr_data *new_new_descr, *descr;

	if (nr < 0)
		return 0;

	obj = new obj_data;
	clear_object(obj);
	old = ((struct obj_data *)obj_index[nr].item); /* cast the void pointer */

	if (old != 0) {
	  *obj = *old;
	} else {
		fprintf(stderr, "clone_object(%d): Obj not found in obj_index.\n", nr);
		dc_free(obj);
		return NULL;
	}

	/* *** extra descriptions *** */
	obj->ex_description = 0;
	for (descr = old->ex_description; descr; descr = descr->next) {
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
	obj_index[nr].number++;
  obj->save_expiration = 0;

	if (obj_index[obj->item_number].non_combat_func ||
			obj->obj_flags.type_flag == ITEM_MEGAPHONE ||
			has_random(obj)) {
	  DC::instance().active_obj_list.insert(obj);
	}
	return obj;
}

void randomize_object_affects(obj_data *obj)
{
	if (obj == NULL) {
		return;
	}

	// Don't alter godload
	if (IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) {
		return;
	}

	for (int i = 0; i < obj->num_affects; i++) {
		switch (obj->affected[i].location) {
		case APPLY_STR:
			case APPLY_DEX:
			case APPLY_INT:
			case APPLY_WIS:
			case APPLY_CON:
			if (number(1, 100) <= 33) {
				obj->affected[i].modifier += number(-1, 1);
			}
			break;
		case APPLY_LIGHTNING_SHIELD:
			case WEP_LIGHTNING_BOLT:
			case WEP_FIREBALL:
			case WEP_FLAMESTRIKE:
			case WEP_DISPEL_EVIL:
			case WEP_MAGIC_MISSILE:
			case WEP_METEOR_SWARM:
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
			obj->affected[i].modifier = random_percent_change(-33, 33, obj->affected[i].modifier);
			break;
		}
	}
}

void randomize_object(obj_data *obj)
{
	if (obj == NULL) {
		return;
	}

	SET_BIT(obj->obj_flags.more_flags, ITEM_CUSTOM);

	switch (obj->obj_flags.type_flag) {
	case ITEM_WEAPON:
		//obj->obj_flags.weight = MAX(1,random_percent_change(-33, 33, obj->obj_flags.weight));
		obj->obj_flags.cost = MAX(1, random_percent_change(-33, 33, obj->obj_flags.cost));
		obj->obj_flags.value[1] = random_percent_change(-20, 20, obj->obj_flags.value[1]);
		obj->obj_flags.value[2] = random_percent_change(-20, 20, obj->obj_flags.value[2]);
		randomize_object_affects(obj);
		break;
	case ITEM_ARMOR:
		//obj->obj_flags.weight = MAX(1,random_percent_change(-33, 33, obj->obj_flags.weight));
		obj->obj_flags.cost = MAX(1, random_percent_change(-33, 33, obj->obj_flags.cost));
		// AC-apply
		obj->obj_flags.value[1] = random_percent_change(-25, 25, obj->obj_flags.value[1]);
		randomize_object_affects(obj);
		break;
	case ITEM_WAND:
		obj->obj_flags.cost = MAX(1, random_percent_change(-33, 33, obj->obj_flags.cost));
		// total charges
		obj->obj_flags.value[1] = random_percent_change(-10, 10, obj->obj_flags.value[2]);
		// current charges
		obj->obj_flags.value[2] = obj->obj_flags.value[1];
		break;
	}
}

void zone_update(void)
{
	int i;

	for (i = 0; i <= top_of_zone_table; i++) {
		if (zone_table[i].reset_mode == 0)
			continue;
		if (zone_table[i].age < zone_table[i].lifespan && !(zone_table[i].reset_mode == 1 && !zone_is_empty(i))) {
			zone_table[i].age++;
			continue;
		}
		if (zone_table[i].reset_mode == 1 && !zone_is_empty(i))
			continue;
		reset_zone(i);
		// update first repop numbers
		if (zone_table[i].num_mob_first_repop == 0)
			zone_table[i].num_mob_first_repop = zone_table[i].num_mob_on_repop;
	}
	DC::instance().removeDead();
}

/* execute the reset command table of a given zone */
void reset_zone(int zone)
{
	extern int top_of_world;
	extern short code_testing_mode;
	int cmd_no, last_cmd, last_mob, last_obj, last_percent;
	int last_no;
	CHAR_DATA *mob = NULL;
	struct obj_data *obj, *obj_to;
	last_cmd = last_mob = last_obj = last_percent = -1;

	char buf[MAX_STRING_LENGTH];

	if (zone_table[zone].died_this_tick == 0 && zone_is_empty(zone)) {
		zone_table[zone].repops_without_deaths++;
	} else {
		zone_table[zone].repops_without_deaths = 0;
	}

	// reset number of mobs that have died this tick to 0
	zone_table[zone].died_this_tick = 0;
	zone_table[zone].num_mob_on_repop = 0;
	// find last command in zone
	last_no = 0;
	while (zone_table[zone].cmd[last_no].command != 'S')
		last_no++;

	for (cmd_no = 0; cmd_no <= last_no; cmd_no++) {
		if ((zone_table[zone].cmd + cmd_no) == 0) {
			sprintf(buf,
					"Trapped zone error, Command is null, zone: %d cmd_no: %d",
					zone, cmd_no);
			log(buf, IMMORTAL, LOG_WORLD);
			break;
		}
		if (ZCMD.command == 'S')
		break;
		if (ZCMD.active == 0)
		continue;

		if (ZCMD.if_flag == 0 || // always
		(last_cmd == 1 && ZCMD.if_flag == 1) ||// if last command true
		(last_cmd == 0 && ZCMD.if_flag == 2) ||// if last command false
		(mud_is_booting && ZCMD.if_flag == 3) ||// on reboot
		(last_mob == 1 && ZCMD.if_flag == 4) ||// if-last-mob-true
		(last_mob == 0 && ZCMD.if_flag == 5) ||// if-last-mob-false
		(last_obj == 1 && ZCMD.if_flag == 6) ||// if-last-obj-true
		(last_obj == 0 && ZCMD.if_flag == 7) ||// if-last-obj-false
		(last_percent == 1 && ZCMD.if_flag == 8) ||// if-last-percent-true
		(last_percent == 0 && ZCMD.if_flag == 9)// if-last-percent-false
		) {
			switch (ZCMD.command) {

				case 'M': /* read a mobile */
				if ((ZCMD.arg2 == -1 || ZCMD.lastPop == 0)
				&& mob_index[ZCMD.arg1].number < ZCMD.arg2 && (mob =
						clone_mobile(ZCMD.arg1))) {
					char_to_room(mob, ZCMD.arg3);
					mob->mobdata->reset = &zone_table[zone].cmd[cmd_no];
					ZCMD.lastPop = mob;
					GET_HOME(mob) = world_array[ZCMD.arg3]->number;
					zone_table[zone].num_mob_on_repop++;
					last_cmd = 1;
					last_mob = 1;
					extern bool selfpurge;
					selfpurge = FALSE;
					mprog_load_trigger(mob);
					if (selfpurge) {
						mob = NULL;
						last_mob = 0;
						last_cmd = 0;
					}

				} else {
					last_cmd = 0;
					last_mob = 0;
				}
				break;

				case 'O': /* read an object */
				if (ZCMD.arg2 == -1
				|| obj_index[ZCMD.arg1].number < ZCMD.arg2) {
					if (ZCMD.arg3 >= 0) {
						if (!get_obj_in_list_num(ZCMD.arg1,
								world[ZCMD.arg3].contents) && (obj =
								clone_object(ZCMD.arg1))) {
							obj_to_room(obj, ZCMD.arg3);
							last_cmd = 1;
							last_obj = 1;
						} else {
							last_cmd = 0;
							last_obj = 0;
						}
					} else {
						if (!code_testing_mode) {
							sprintf(buf,
							"Obj %d loaded to NOWHERE. Zone %d Cmd %d",
							obj_index[ZCMD.arg1].virt, zone, cmd_no);
							log(buf, IMMORTAL, LOG_WORLD);
						}
						last_cmd = 0;
						last_obj = 0;
					}
				} else {
					last_cmd = 0;
					last_obj = 0;
				}
				break;

				case 'P': /* object to object */

				if (ZCMD.arg2 == -1
				|| obj_index[ZCMD.arg1].number < ZCMD.arg2) {
					obj_to = 0;
					obj = 0;
					if ((obj_to = get_obj_num(ZCMD.arg3)) && (obj =
							clone_object(ZCMD.arg1)))
					obj_to_obj(obj, obj_to);
					else
					logf(
					IMMORTAL,
					LOG_WORLD,
					"Null container obj in P command Zone: %d, Cmd: %d",
					zone, cmd_no);

					last_cmd = 1;
					last_obj = 1;
				} else {
					last_cmd = 0;
					last_obj = 0;
				}
				break;

				case 'G': /* obj_to_char */
				if (mob == NULL) {
					sprintf(buf, "Null mob in G, reseting zone %d cmd %d", zone,
					cmd_no + 1);
					log(buf, IMMORTAL, LOG_WORLD);
					last_cmd = 0;
					last_obj = 0;
					break;
				}
				if ((ZCMD.arg2 == -1 || obj_index[ZCMD.arg1].number < ZCMD.arg2
						|| number(0, 1)) && (obj = clone_object(ZCMD.arg1))) {
					obj_to_char(obj, mob);
					last_cmd = 1;
					last_obj = 0;
				} else {
					last_cmd = 0;
					last_obj = 0;
				}
				break;

				case '%': /* percent chance of next command happening */
				if (number(1, ZCMD.arg2) <= ZCMD.arg1) {
					ZCMD.last = time(NULL);
					last_percent = 1;
					last_cmd = 1;
				} else {
					last_cmd = 0;
					last_percent = 0;
				}
				break;

				case 'E': /* object to equipment list */
				if (mob == NULL) {
					sprintf(buf, "Null mob in E reseting zone %d cmd %d", zone,
					cmd_no);
					log(buf, IMMORTAL, LOG_WORLD);
					last_cmd = 0;
					last_obj = 0;
					break;
				}
				if ((obj = clone_object(ZCMD.arg1))) {
					randomize_object(obj);

					// Check if mob and object are safe to be equipped
					// Some of these checks are redundant compared to equip_char() but
					// we want to know if the position is filled already before running equip_char()
					// so we don't see unnecessary errors when a zone reset tries to reequip a mob
					if (mob == NULL) {
						log("NULL mob in reset_zone()!", ANGEL, LOG_BUG);
					} else if (ZCMD.arg3 < 0 || ZCMD.arg3 >= MAX_WEAR) {
						log("Invalid eq position in reset_zone()!", ANGEL,
						LOG_BUG);
					} else if (mob->equipment[ZCMD.arg3] == 0) {
						if (!equip_char(mob, obj, ZCMD.arg3)) {
							sprintf(buf, "Bad equip_char zone %d cmd %d", zone,
							cmd_no);
							log(buf, IMMORTAL, LOG_WORLD);
						}
					}

					if (ISSET(mob->mobdata->actflags, ACT_BOSS)) {
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
				} else {
					last_cmd = 0;
					last_obj = 0;
				}
				break;

				case 'D': /* set state of door */
				if (ZCMD.arg1 < 0 || ZCMD.arg1 > top_of_world) {
					sprintf(log_buf, "Illegal room number Z: %d cmd %d", zone,
					cmd_no);
					log(log_buf, IMMORTAL, LOG_WORLD);
					break;
				}
				if (ZCMD.arg2 < 0 || ZCMD.arg2 >= 6) {
					sprintf(log_buf,
					"Illegal direction %d doesn't exist Z: %d cmd %d",
					ZCMD.arg2, zone, cmd_no);
					log(log_buf, IMMORTAL, LOG_WORLD);
					break;
				}
				if (!world_array[ZCMD.arg1]) {
					sprintf(log_buf, "Room %d doesn't exist Z: %d cmd %d",
					ZCMD.arg1, zone, cmd_no);
					log(log_buf, IMMORTAL, LOG_WORLD);
					break;
				}

				if (world[ZCMD.arg1].dir_option[ZCMD.arg2] == 0) {
					sprintf(
					log_buf,
					"Attempt to reset direction %d on room %d that doesn't exist Z: %d cmd %d",
					ZCMD.arg2, world[ZCMD.arg1].number, zone, cmd_no);
					log(log_buf, IMMORTAL, LOG_WORLD);
					break;
				}
				switch (ZCMD.arg3) {
					case 0:
					REMOVE_BIT(
					world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
					EX_BROKEN);
					REMOVE_BIT(
					world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
					EX_LOCKED);
					REMOVE_BIT(
					world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
					EX_CLOSED);
					break;
					case 1:
					REMOVE_BIT(
					world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
					EX_BROKEN);
					SET_BIT( world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
					EX_CLOSED);
					REMOVE_BIT(
					world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
					EX_LOCKED);
					break;
					case 2:
					REMOVE_BIT(
					world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
					EX_BROKEN);
					SET_BIT( world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
					EX_LOCKED);
					SET_BIT( world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
					EX_CLOSED);
					break;
				}
				last_cmd = 1;
				break;

				case 'X':
				switch (ZCMD.arg1) {
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
				cmd_no += ZCMD.arg1;
				break;

				case '*':// ignore *
				case 'J':// ignore J
				break;

				default:
				sprintf(
				log_buf,
				"UNKNOWN COMMAND!!! ZONE %d cmd %d: '%c' Skipping zone..",
				zone, cmd_no, ZCMD.command);
				log(log_buf, IMMORTAL, LOG_WORLD);
				zone_table[zone].age = 0;
				return;
				break;
			}
		} else {
			switch (ZCMD.command) {

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

	zone_table[zone].age = 0;

	if (zone_table[zone].repops_without_deaths > 2
			&& zone_table[zone].repops_without_deaths < 7
			&& zone_table[zone].repops_with_bonus < 4) {
		zone_table[zone].repops_with_bonus++;

		auto &character_list = DC::instance().character_list;
		for (auto &tmp_victim : character_list) {
			if (tmp_victim->in_room == NOWHERE) {
				continue;
			}
			if (IS_NPC(tmp_victim)
					&& !ISSET(tmp_victim->mobdata->actflags, ACT_NO_GOLD_BONUS)
					&& world[tmp_victim->in_room].zone == zone) {
				tmp_victim->gold *= 1.10;
				tmp_victim->exp *= 1.10;
			}
		}
	}
}

#undef ZCMD

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int zone_is_empty(int zone_nr)
{
	struct descriptor_data *i;

	for (i = descriptor_list; i; i = i->next)
		if (STATE(i) == CON_PLAYING && i->character
				&& world[i->character->in_room].zone == zone_nr)
			return (0);

	return (1);
}

/************************************************************************
 *  procs of a (more or less) general utility nature         *
 ********************************************************************** */
char *fread_string(ifstream &in, int hasher)
{
	char buffer[MAX_STRING_LENGTH];

	// Save original exception mask so we can restore it later
	ios_base::iostate orig_exceptions = in.exceptions();
	in.exceptions(ifstream::failbit | ifstream::badbit | ifstream::eofbit);
	try {
		in.getline(buffer, MAX_STRING_LENGTH, '~');
		in >> ws;
	} catch (...) {
		logf(IMMORTAL, LOG_BUG, "fread_string() error reading");
		throw;
	}
	in.exceptions(orig_exceptions);

	string orig_str(buffer);
	// Change \n into \r\n
	string swapstr = lf_to_crlf(orig_str);

	char *retval;
	if (hasher) {
		retval = str_hsh(swapstr.c_str());
	} else {
		retval = new char[swapstr.length()];
		memcpy(retval, swapstr.c_str(), swapstr.length());
	}
	return retval;
}

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, int hasher)
{
	char buf[MAX_STRING_LENGTH];
	char * pAlloc;
	char * pBufLast;
	char * temp;

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
			*pBufLast++ = '\n';
			*pBufLast++ = '\r';
			break;

		case '~':
			getc(fl);
			if (pBufLast == buf) {
				if (hasher) {
					pAlloc = str_hsh("");
				} else {
					pAlloc = str_dup("");
				}
			} else if (hasher) {
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
			else {
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
		case EOF:
			perror("fread_string: EOF");
			throw error_eof();
			break;
		} // switch
	} // for

	perror("fread_string: string too long");
	abort();
	return ( NULL);
}

/* read and allocate space for a whitespace-terminated string from a given file */
char *fread_word(FILE *fl, int hasher)
{
	char buf[MAX_STRING_LENGTH];
	char * pAlloc;
	char * pBufLast;
	char * temp;
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

		case EOF:
			perror("fread_word: EOF");
			abort();
			break;

		case '\t':
			case '\n':
			case ' ':
			if (pBufLast == buf) {
				if (hasher)
					pAlloc = str_hsh("");
				else
					pAlloc = str_dup("");
			}
			else if (hasher) {
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
			else {
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

	perror("fread_word: string too long");
	abort();
	return ( NULL);
}

// This is here to allow us to read a bitvector in as either a number
// or as a string of characters.  ie, 4, and c are the same.
// 5 (1+4) would be the same at 'ac'

int fread_bitvector(FILE *fl, long beg_range, long end_range)
{
	char buf[200];
	int ch;
	long i = 0;

	// eat space till we hit the next one
	while ((ch = getc(fl))) {
		if (ch == EOF) {
			printf("Reading %s: %s, %d\n", curr_type, curr_name, curr_virtno);
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
			sprintf(buf, "Reading %s: %s, %d\n", curr_type, curr_name, curr_virtno);
			log(buf, 0, LOG_MISC);
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
		else {
			sprintf(buf, "Reading %s: %s, %d (%c)\n", curr_type, curr_name, curr_virtno, ch);
			log(buf, 0, LOG_MISC);
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

int fread_bitvector(ifstream &in, long beg_range, long end_range)
{
	int ch;
	long i = 0;

	// Save original exception mask so we can restore it later
	ios_base::iostate orig_exceptions = in.exceptions();
	in.exceptions(ifstream::failbit | ifstream::badbit | ifstream::eofbit);
	try {
		// eat space till we hit the next one
		while ((ch = in.get())) {
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
			else {
				logf(IMMORTAL, LOG_BUG, "fread_bitvector: illegal character");
				logf(IMMORTAL, LOG_BUG, "Reading %s: %s, %d (%c)\n", curr_type, curr_name, curr_virtno, ch);
				throw;
			}

			// if we hit here, we had a valid character.  read the next one.
			ch = in.get();

		} // for ;;
	} catch (...) {
		logf(IMMORTAL, LOG_BUG, "fread_bitvector: unknown error");
		logf(IMMORTAL, LOG_BUG, "Reading %s: %s, %d", curr_type, curr_name, curr_virtno);
		throw;
	}
	in.exceptions(orig_exceptions);

	return 0;
}

uint64_t fread_uint(FILE *fl, uint64_t beg_range, uint64_t end_range)
{
	char buf[MAX_STRING_LENGTH];
	char * pBufLast;
	int ch;
	uint64_t i;

	while ((ch = getc(fl))) {
		if (ch == EOF) {
			printf("Reading %s: %s, %d\n", curr_type, curr_name, curr_virtno);
			perror("fread_int: premature EOF");
			abort();
		}

		if (ch != ' ' && ch != '\n') /* eat the white space */
			break;
	}

	pBufLast = buf;

	if (ch == '-' && beg_range >= 0) {
		printf("Reading %s: %s, %d\n", curr_type, curr_name, curr_virtno);
		perror("fread_int: Bad value - < 0 on positive only num");
		while (isdigit(getc(fl))) {
		}
		throw error_negative_int();
	} else if (ch == '-') {
		*pBufLast = ch;
		pBufLast++;
		ch = getc(fl);
	}

	*pBufLast = ch;
	pBufLast++;

	for (; pBufLast < &buf[sizeof(buf) - 4];) {
		switch (ch = getc(fl)) {
		default:
			if (isdigit(ch)) {
				*pBufLast = ch;
				pBufLast++;
			} else {
				*pBufLast = 0;
				i = atoll(buf);
				if (i >= beg_range && i <= end_range) {
					return i;
				} else {
					printf("Buffer: '%s'\n", buf);
					printf("Reading %s: %s, %d\n", curr_type, curr_name,
							curr_virtno);
					printf("fread_int: Bad value for range %lld - %lld: %lld\n",
							beg_range, end_range, i);
					perror("fread_int: Value range error");
					throw error_range_int();
				}
			}
			break;

		case EOF:
			perror("fread_int: EOF");
			abort();
			break;
		}
	}

	perror("fread_int: something went wrong");
	abort();
	return (0);
}

int64_t fread_int(ifstream &in, int64_t beg_range, int64_t end_range)
{
	int64_t number;
	in >> number;

	if (number < beg_range) {
		cerr << "fread_int: error " << number << " less than " << beg_range << ". "
				<< "Setting to " << beg_range << endl;
		number = beg_range;
	} else if (number > end_range) {
		cerr << "fread_int: error " << number << " greater than " << beg_range << ". "
				<< "Setting to " << beg_range << endl;
		number = end_range;
	}

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
	char * pBufLast;
	int ch;
	int64_t i;

	while ((ch = getc(fl))) {
		if (ch == EOF) {
			printf("Reading %s: %s, %d\n", curr_type, curr_name, curr_virtno);
			perror("fread_int: premature EOF");
			abort();
		}

		if (ch != ' ' && ch != '\n') /* eat the white space */
			break;
	}

	pBufLast = buf;

	if (ch == '-' && beg_range >= 0) {
		printf("Reading %s: %s, %d\n", curr_type, curr_name, curr_virtno);
		perror("fread_int: Bad value - < 0 on positive only num");
		while (isdigit(getc(fl))) {
		}
		throw error_negative_int();
	} else if (ch == '-') {
		*pBufLast = ch;
		pBufLast++;
		ch = getc(fl);
	}

	*pBufLast = ch;
	pBufLast++;

	for (; pBufLast < &buf[sizeof(buf) - 4];) {
		switch (ch = getc(fl)) {
		default:
			if (isdigit(ch)) {
				*pBufLast = ch;
				pBufLast++;
			} else {
				*pBufLast = 0;
				i = atoll(buf);
				if (i >= beg_range && i <= end_range) {
					return i;
				} else {
					printf("Buffer: '%s'\n", buf);
					printf("Reading %s: %s, %d\n", curr_type, curr_name,
							curr_virtno);
					printf("fread_int: Bad value for range %lld - %lld: %lld\n",
							beg_range, end_range, i);
					perror("fread_int: Value range error");
					throw error_range_int();
				}
			}
			break;

		case EOF:
			perror("fread_int: EOF");
			abort();
			break;
		}
	}

	perror("fread_int: something went wrong");
	abort();
	return (0);
}

char fread_char(FILE *fl)
{
	int ch;

	while ((ch = getc(fl)))
	{
		if (ch == EOF)
		{
			printf("Reading %s: %s, %d\n", curr_type, curr_name, curr_virtno);
			perror("fread_char: premature EOF");
			abort();
		}

		if (ch != ' ' && ch != '\n') /* eat the white space */
			break;
	}

	return ch;
}

/* release memory allocated for a char struct */
void free_char(CHAR_DATA *ch)
{
	int iWear;
//  struct affected_type *af;
	struct char_player_alias * x;
	struct char_player_alias * next;
	MPROG_ACT_LIST * currmprog;
	auto &character_list = DC::instance().character_list;

	character_list.erase(ch);

	if (ch->tempVariable)
	{
		struct tempvariable *temp, *tmp;
		for (temp = ch->tempVariable; temp; temp = tmp)
				{
			tmp = temp->next;
			dc_free(temp->name);
			dc_free(temp->data);
			dc_free(temp);
		}
	}
	SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop falling off

	for (iWear = 0; iWear < MAX_WEAR; iWear++) {
		if (ch->equipment[iWear])
			obj_to_char(unequip_char(ch, iWear, 1), ch);
	}
	while (ch->carrying)
		extract_obj(ch->carrying);

	if (!IS_NPC(ch)) {
		if (ch->name)
			dc_free(ch->name);
		if (ch->short_desc)
			dc_free(ch->short_desc);
		if (ch->long_desc)
			dc_free(ch->long_desc);
		if (ch->description)
			dc_free(ch->description);
		if (ch->pcdata)
		{
			// these won't be here if you free an unloaded char
			ch->pcdata->skillchange = 0;
			if (ch->pcdata->last_site)
				dc_free(ch->pcdata->last_site);
			if (ch->pcdata->ignoring)
				dc_free(ch->pcdata->ignoring);
			if (ch->pcdata->poofin)
				dc_free(ch->pcdata->poofin);
			if (ch->pcdata->poofout)
				dc_free(ch->pcdata->poofout);
			if (ch->pcdata->prompt)
				dc_free(ch->pcdata->prompt);
			if (ch->pcdata->last_prompt)
				dc_free(ch->pcdata->last_prompt);
			if (ch->pcdata->last_tell)
				dc_free(ch->pcdata->last_tell);
			if (ch->pcdata->golem)
				log("Error, golem not released properly", ANGEL, LOG_BUG);
			if (ch->pcdata->joining)
				dc_free(ch->pcdata->joining);
			/* Free aliases... (I was to lazy to do before. ;) */
			for (x = ch->pcdata->alias; x; x = next) {
				next = x->next;
				if (x->keyword)
					dc_free(x->keyword);
				if (x->command)
					dc_free(x->command);
				dc_free(x);
			}

			if (ch->pcdata->away_msgs)
				delete ch->pcdata->away_msgs;

			if (ch->pcdata->lastseen)
				delete ch->pcdata->lastseen;

			dc_free(ch->pcdata);
		}
	}
	else {
		remove_memory(ch, 'f');
		remove_memory(ch, 'h');
		while (ch->mobdata->mpact) {
			currmprog = ch->mobdata->mpact->next;
			if (ch->mobdata->mpact->buf)
				dc_free(ch->mobdata->mpact->buf);
			dc_free(ch->mobdata->mpact);
			ch->mobdata->mpact = currmprog;
		}
		dc_free(ch->mobdata);
	}

	if (ch->title)
		dc_free(ch->title);
	ch->title = NULL;

	remove_memory(ch, 't');

// Since affect_remove updates the linked list itself, do it this way
	while (ch->affected)
		affect_remove(ch, ch->affected, SUPPRESS_ALL);

	dc_free(ch);
}

/* release memory allocated for an obj struct */
void free_obj(struct obj_data *obj)
{
	struct extra_descr_data *ths, *next_one;

	for (ths = obj->ex_description; ths; ths = next_one) {
		next_one = ths->next;
		dc_free(ths);
	}

	dc_free(obj->affected);

	dc_free(obj);
}

/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
	FILE *fl;
	char tmp[100];

	*buf = '\0';

	if (!(fl = dc_fopen(name, "r")))
	{
		perror(name);
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
				log("fl->strng: string too big (db.c, file_to_string)",
						0, LOG_BUG);
				*buf = '\0';
				return (-1);
			}

			strcat(buf, tmp);
			*(buf + strlen(buf) + 1) = '\0';
			*(buf + strlen(buf)) = '\r';
		}
	}
	while (!feof(fl));

	dc_fclose(fl);

	return (0);
}

/* clear some of the the working variables of a char */
void reset_char(CHAR_DATA *ch)
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
	ch->position = POSITION_STANDING;
	ch->carry_weight = 0;
	ch->carry_items = 0;

	switch (GET_CLASS(ch)) {
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

	if (GET_HIT(ch) < 1)
		GET_HIT(ch) = 1;
	if (GET_MOVE(ch) < 1)
		GET_MOVE(ch) = 1;
	if (GET_MANA(ch) < 1)
		GET_MANA(ch) = 1;

	ch->misc = 0;

	SET_BIT(ch->misc, LOG_BUG);
	SET_BIT(ch->misc, LOG_PRAYER);
	SET_BIT(ch->misc, LOG_GOD);
	SET_BIT(ch->misc, LOG_MORTAL);
	SET_BIT(ch->misc, LOG_SOCKET);
	SET_BIT(ch->misc, LOG_MISC);
	SET_BIT(ch->misc, LOG_PLAYER);
//  SET_BIT(ch->misc, CHANNEL_GOSSIP);
	SET_BIT(ch->misc, CHANNEL_DREAM);
	SET_BIT(ch->misc, CHANNEL_SHOUT);
	SET_BIT(ch->misc, CHANNEL_AUCTION);
	SET_BIT(ch->misc, CHANNEL_INFO);
	SET_BIT(ch->misc, CHANNEL_NEWBIE);
	SET_BIT(ch->misc, CHANNEL_TELL);
	SET_BIT(ch->misc, CHANNEL_HINTS);
	ch->group_name = 0;
	ch->ambush = 0;
	ch->guarding = 0;
	ch->guarded_by = 0;
}

/*
 * Clear but do not de-alloc.
 */
void clear_char(CHAR_DATA *ch)
{
	memset((char *)ch, (char)'\0', (int)sizeof(CHAR_DATA));

	ch->in_room = NOWHERE;
	ch->position = POSITION_STANDING;
	GET_HOME(ch) = START_ROOM;
	GET_AC(ch) = 100; /* Basic Armor */
}

void clear_object(struct obj_data *obj)
{
	//memset((char *)obj, (char)'\0', (int)sizeof(struct obj_data));
	obj->item_number = -1;
	obj->in_room = NOWHERE;
  obj->vroom = 0;
  obj->obj_flags = obj_flag_data();
  obj->num_affects = 0;
  obj->affected = nullptr;

  obj->name = nullptr;
  obj->description = nullptr;
  obj->short_description = nullptr;
  obj->action_description = nullptr;
  obj->ex_description = nullptr;
  obj->carried_by = nullptr;
  obj->equipped_by = nullptr;

  obj->in_obj = nullptr;
  obj->contains = nullptr;

  obj->next_content = nullptr;
  obj->next = nullptr;
  obj->next_skill = nullptr;;
  obj->table = nullptr;
  obj->slot = nullptr;
  obj->wheel = nullptr;
  obj->save_expiration = 0;
}

// Roll up the random modifiers to saving throw for new character
void apply_initial_saves(char_data *ch)
{
	for (int i = 0; i <= SAVE_TYPE_MAX; i++)
		if (number(0, 1))
			ch->pcdata->saves_mods[i] = number(-3, 3);
		else
			ch->pcdata->saves_mods[i] = 0;
}

void init_char(CHAR_DATA *ch)
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
	ch->hit = GET_MAX_HIT(ch);
	ch->move = GET_MAX_MOVE(ch);

	switch (GET_CLASS(ch)) {
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

	ch->altar = NULL;
	ch->spec = 0;
	GET_PROMPT(ch) = 0;
	GET_LAST_PROMPT(ch) = 0;
	ch->pcdata->skillchange = 0;
	ch->pcdata->joining = 0;
	ch->pcdata->practices = 0;
	ch->pcdata->time.birth = time(0);
	ch->pcdata->time.played = 0;
	ch->pcdata->time.logon = time(0);
	ch->pcdata->toggles = 0;
	ch->pcdata->golem = 0;
	ch->pcdata->quest_points = 0;
	for (int j = 0; j < QUEST_CANCEL; j++)
		ch->pcdata->quest_cancel[j] = 0;
	for (int j = 0; j <= QUEST_TOTAL / ASIZE; j++)
		ch->pcdata->quest_complete[j] = 0;

	SET_BIT(ch->pcdata->toggles, PLR_ANSI);
	SET_BIT(ch->pcdata->toggles, PLR_BARD_SONG);
	int i;
	for (i = 0; i < AFF_MAX / ASIZE + 1; i++)
		ch->affected_by[i] = 0;

	apply_initial_saves(ch);

	for (int i = 0; i < 3; i++)
		GET_COND(ch, i) = 50; // 50 ticks of "full-ness"

	reset_char(ch);
}

/* returns the real number of the room with given virt number */
int real_room(int virt)
{
	if (virt < 0 || virt > top_of_world)
		return -1;
	if (world_array[virt])
		return virt;
	return -1;
}

/* returns the real number of the monster with given virt number */
int real_mobile(int virt)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_mobt;

	/* perform binary search on mob-table */
	for (;;)
			{
		mid = (bot + top) / 2;

		if ((mob_index + mid)->virt == virt)
			return (mid);
		if (bot >= top)
			return (-1);
		if ((mob_index + mid)->virt > virt)
			top = mid - 1;
		else
			bot = mid + 1;
	}

	return -1;
}

/* returns the real number of the object with given virt number */
int real_object(int virt)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_objt;

	/* perform binary search on obj-table */
	for (;;)
			{
		mid = (bot + top) / 2;

		if ((obj_index + mid)->virt == virt)
			return (mid);
		if (bot >= top)
			return (-1);
		if ((obj_index + mid)->virt > virt)
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
int obj_in_index(char *name, int index)
{
	int i, j;

	for (i = 0, j = 1; (i < MAX_INDEX) && (j <= index) &&
			((struct obj_data *)(obj_index[i].item)); i++)
		if (isname(name, ((struct obj_data *)(obj_index[i].item))->name)) {
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
			((struct char_data *)(mob_index[i].item)); i++)
		if (isname(name, GET_NAME(((struct char_data *)(mob_index[i].item))))) {
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

int mprog_name_to_type(char *name)
{
	if (!str_cmp(name, "in_file_prog"))
		return IN_FILE_PROG;
	if (!str_cmp(name, "act_prog"))
		return ACT_PROG;
	if (!str_cmp(name, "speech_prog"))
		return SPEECH_PROG;
	if (!str_cmp(name, "rand_prog"))
		return RAND_PROG;
	if (!str_cmp(name, "arand_prog"))
		return ARAND_PROG;
	if (!str_cmp(name, "fight_prog"))
		return FIGHT_PROG;
	if (!str_cmp(name, "hitprcnt_prog"))
		return HITPRCNT_PROG;
	if (!str_cmp(name, "death_prog"))
		return DEATH_PROG;
	if (!str_cmp(name, "entry_prog"))
		return ENTRY_PROG;
	if (!str_cmp(name, "greet_prog"))
		return GREET_PROG;
	if (!str_cmp(name, "all_greet_prog"))
		return ALL_GREET_PROG;
	if (!str_cmp(name, "give_prog"))
		return GIVE_PROG;
	if (!str_cmp(name, "bribe_prog"))
		return BRIBE_PROG;
	if (!str_cmp(name, "catch_prog"))
		return CATCH_PROG;
	if (!str_cmp(name, "attack_prog"))
		return ATTACK_PROG;
	if (!str_cmp(name, "load_prog"))
		return LOAD_PROG;
	if (!str_cmp(name, "command_prog"))
		return COMMAND_PROG;
	if (!str_cmp(name, "weapon_prog"))
		return WEAPON_PROG;
	if (!str_cmp(name, "armour_prog"))
		return ARMOUR_PROG;
	if (!str_cmp(name, "can_see_prog"))
		return CAN_SEE_PROG;
	if (!str_cmp(name, "damage_prog"))
		return DAMAGE_PROG;
	return ( ERROR_PROG);
}

/* This routine reads in scripts of MOBprograms from a file */

void mprog_file_read(char *f, long i)
{
	MPROG_DATA *mprog;
	FILE *fp;
	char letter;
	char name[128];
	int type;

	sprintf(name, "%s%s", MOB_DIR, f);
	if (!(fp = fopen(name, "r")))
	{
		logf( IMMORTAL, LOG_WORLD, "Mob: %d couldn't opne mobprog file.", i);
		return;
	}
	for (;;)
			{
		if ((letter = fread_char(fp)) == '|')
			break;
		else if (letter != '>')
				{
			logf( IMMORTAL, LOG_WORLD, "Mprog_file_read: Invalid letter mob %d.", i);
			return;
		}
		switch ((type = fread_int(fp, 0, MPROG_MAX_TYPE_VALUE)))
		{
		case ERROR_PROG:
			logf( IMMORTAL, LOG_WORLD, "Mob %d: in file prog error.", i);
			return;
		case IN_FILE_PROG:
			logf( IMMORTAL, LOG_WORLD, "Mob %d: nested in file progs.", i);
			return;
		default:
			SET_BIT(mob_index[i].progtypes, type);
#ifdef LEAK_CHECK
			mprog = (MPROG_DATA *) calloc(1, sizeof(MPROG_DATA));
#else
			mprog = (MPROG_DATA *)dc_alloc(1, sizeof(MPROG_DATA));
#endif
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
			logf( IMMORTAL, LOG_WORLD, "Load_mobprogs: bad command '%c'.", letter);
			break;
		case 's':
			return;
		case '*':
			break;
		case 'm':
			value = fread_int(fp, 0, LONG_MAX);
			if (real_mobile(value) < 0)
					{
				logf( IMMORTAL, LOG_WORLD, "Load_mobprogs: vnum %d doesn't exist.", value);
				break;
			}
			mprog_file_read(fread_word(fp, 1), value);
			break;
		}
	return;
}

void mprog_read_programs(FILE *fp, long i, bool zz)
{
	MPROG_DATA *mprog;
	char letter;
	int type;
	MPROG_DATA lmprog;
	for (;;)
			{
		if ((letter = fread_char(fp)) == '|')
			break;
		else if (letter != '>' && letter != '\\')
				{
			logf( IMMORTAL, LOG_WORLD, "Load_mobiles: vnum %d MOBPROG char", i);
			ungetc(letter, fp);
			return;
		}
		type = mprog_name_to_type(fread_word(fp, 1));
		switch (type)
		{
		case ERROR_PROG:
			logf( IMMORTAL, LOG_WORLD, "Load_mobiles: vnum %d MOBPROG type.", i);
			return;
		case IN_FILE_PROG:
			mprog_file_read(fread_string(fp, 1), i);
			break;
		default:
			if (!zz) {
				if (letter == '>')
					SET_BIT(mob_index[i].progtypes, type);
				else
					SET_BIT(obj_index[i].progtypes, type);
			}
			if (!zz) {
#ifdef LEAK_CHECK
				mprog = (MPROG_DATA *) calloc(1, sizeof(MPROG_DATA));
#else
				mprog = (MPROG_DATA *)dc_alloc(1, sizeof(MPROG_DATA));
#endif
			} else
				mprog = &lmprog;
			mprog->type = type;
			mprog->arglist = fread_string(fp, 0);
			mprog->comlist = fread_string(fp, 0);
			if (!zz) {
				if (letter == '>') {
					mprog->next = mob_index[i].mobprogs;   // when we write them, we write last first
					mob_index[i].mobprogs = mprog;         // so reading them this way keeps them in order
				} else {
					mprog->next = obj_index[i].mobprogs;
					obj_index[i].mobprogs = mprog;
				}
			}
			break;
		}
	}
	return;
}

// * --- End MOBProgs stuff --- *

void find_unordered_objects(void)
{
	int cur_vnum, last_vnum = 0;

	for (int rnum = 0; rnum <= top_of_objt; rnum++, last_vnum = cur_vnum) {
		cur_vnum = obj_index[rnum].virt;

		if (cur_vnum < last_vnum) {
			logf(0, LOG_MISC, "Out of order vnum found. Vnum: %d Last Vnum: %d Rnum: %d", cur_vnum, last_vnum, rnum);
		}
	}
}

void find_unordered_mobiles(void)
{
	int cur_vnum, last_vnum = 0;

	for (int rnum = 0; rnum <= top_of_mobt; rnum++, last_vnum = cur_vnum) {
		cur_vnum = mob_index[rnum].virt;

		if (cur_vnum < last_vnum) {
			logf(0, LOG_MISC, "Out of order vnum found. Vnum: %d Last Vnum: %d Rnum: %d", cur_vnum, last_vnum, rnum);
		}
	}
}

void string_to_file(FILE *f, char *string)
{
	char * newbuf = new char[strlen(string) + 1];
	strcpy(newbuf, string);

	// remove all \r's
	for (char * curr = newbuf; *curr != '\0'; curr++)
			{
		if (*curr == '\r') {
			for (char * blah = curr; *blah != '\0'; blah++) // shift the rest of the string 1 left
				*blah = *(blah + 1);
			curr--; // (to check for \r\r cases)
		}
	}

	fprintf(f, "%s~\n", newbuf);
	delete[] newbuf;
}

void string_to_file(ofstream &f, char *string)
{
	char * newbuf = new char[strlen(string) + 1];
	strcpy(newbuf, string);

	// remove all \r's
	for (char * curr = newbuf; *curr != '\0'; curr++)
			{
		if (*curr == '\r') {
			for (char * blah = curr; *blah != '\0'; blah++) // shift the rest of the string 1 left
				*blah = *(blah + 1);
			curr--; // (to check for \r\r cases)
		}
	}

	f << newbuf << "~" << endl;
	delete[] newbuf;
}

void copySaveData(obj_data *target, obj_data *source)
{
	int i;
	if ((i = eq_current_damage(source)) > 0) {
		for (; i > 0; i--)
			damage_eq_once(target);
	}

	if (strcmp(GET_OBJ_SHORT(source), GET_OBJ_SHORT(target))) {
		GET_OBJ_SHORT(target) = str_hsh(GET_OBJ_SHORT(source));
	}
	if (strcmp(source->description, target->description))
	{
		target->description = str_hsh(source->description);
	}

	if (strcmp(source->name, target->name)) {
		target->name = str_hsh(source->name);
	}

	if (source->obj_flags.type_flag != target->obj_flags.type_flag)
	{
		target->obj_flags.type_flag = source->obj_flags.type_flag;
	}

	if (source->obj_flags.extra_flags != target->obj_flags.extra_flags) {
		target->obj_flags.extra_flags = source->obj_flags.extra_flags;
	}

	if (source->obj_flags.more_flags != target->obj_flags.more_flags) {
		target->obj_flags.more_flags = source->obj_flags.more_flags;
	}

	if (IS_SET(source->obj_flags.more_flags, ITEM_CUSTOM)
			&& source->obj_flags.value[0] != target->obj_flags.value[0]) {
		target->obj_flags.value[0] = source->obj_flags.value[0];
	}

	if ((IS_SET(source->obj_flags.more_flags, ITEM_CUSTOM) || source->obj_flags.type_flag == ITEM_DRINKCON)
			&& source->obj_flags.value[1] != target->obj_flags.value[1]) {
		target->obj_flags.value[1] = source->obj_flags.value[1];
	}

	if ((IS_SET(source->obj_flags.more_flags, ITEM_CUSTOM) || source->obj_flags.type_flag == ITEM_STAFF || source->obj_flags.type_flag == ITEM_WAND)
			&& source->obj_flags.value[2] != target->obj_flags.value[2]) {
		target->obj_flags.value[2] = source->obj_flags.value[2];
	}

	if (IS_SET(source->obj_flags.more_flags, ITEM_CUSTOM)
			&& source->obj_flags.value[3] != target->obj_flags.value[3]) {
		target->obj_flags.value[3] = source->obj_flags.value[3];
	}

	if ((source->obj_flags.type_flag == ITEM_ARMOR || source->obj_flags.type_flag == ITEM_WEAPON) && IS_SET(source->obj_flags.more_flags, ITEM_CUSTOM)) {
		target->obj_flags.weight = source->obj_flags.weight;
		target->obj_flags.cost = source->obj_flags.cost;
		target->obj_flags.value[1] = source->obj_flags.value[1];
		target->obj_flags.value[2] = source->obj_flags.value[2];

		// If new object does not have enough room for affects to be copied then realloc it
		if (source->num_affects != target->num_affects) {
			target->affected = (obj_affected_type *)realloc(target->affected,
					(sizeof(obj_affected_type) * source->num_affects));
			if (target->affected == NULL) {
				perror("realloc");
				exit(EXIT_FAILURE);
			}
			target->num_affects = source->num_affects;
		}

		for (int i = 0; i < source->num_affects; ++i) {
			target->affected[i].location = source->affected[i].location;
			target->affected[i].modifier = source->affected[i].modifier;
		}
	}

  if (IS_SET(source->obj_flags.more_flags, ITEM_24H_SAVE)) {
	  target->save_expiration = source->save_expiration;
  }

	return;
}

bool fullItemMatch(obj_data *obj, obj_data *obj2)
{
	if (strcmp(GET_OBJ_SHORT(obj), GET_OBJ_SHORT(obj2))) {
		return false;
	}

	if (strcmp(obj->name, obj2->name)) {
		return false;
	}

	if (obj->obj_flags.extra_flags != obj2->obj_flags.extra_flags) {
		return false;
	}

	if (obj->obj_flags.more_flags != obj2->obj_flags.more_flags) {
		return false;
	}

	if (IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM)
			&& obj->obj_flags.cost != obj2->obj_flags.cost) {
		return false;
	}

	if (IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM)
			&& obj->obj_flags.value[0] != obj2->obj_flags.value[0]) {
		return false;
	}

	if (obj->obj_flags.type_flag != obj2->obj_flags.type_flag) {
		return false;
	}

	if ((IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM) || obj->obj_flags.type_flag == ITEM_DRINKCON)
			&& obj->obj_flags.value[1] != obj2->obj_flags.value[1]) {
		return false;
	}

	if ((IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM) || obj->obj_flags.type_flag == ITEM_STAFF || obj->obj_flags.type_flag == ITEM_WAND)
			&& (obj->obj_flags.value[2] != obj2->obj_flags.value[2])) {
		return false;
	}

	if (IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM)
			&& obj->obj_flags.value[3] != obj2->obj_flags.value[3]) {
		return false;
	}

	if (IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM)
			&& obj->num_affects != obj2->num_affects) {
		return false;
	}

	// check if any of the affects don't match
	if (IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM)) {
		for (int i = 0; i < obj->num_affects; ++i) {
			if ((obj->affected[i].location != obj2->affected[i].location) ||
					(obj->affected[i].modifier != obj2->affected[i].modifier)) {
				return false;
			}
		}
	}

	return 1;
}

// Function to ensure an item is not bugged. If it is, replace it with the original.
bool verify_item(struct obj_data **obj)
{
	extern int top_of_objt;

	if (!str_cmp((*obj)->short_description, ((struct obj_data*)obj_index[(*obj)->item_number].item)->short_description))
		return FALSE;

	int newitem = -1;
	for (int i = 1;; i++)
			{

		if ((*obj)->item_number - i < 0 && (*obj)->item_number + i > top_of_objt)
			break; // No item at all found, it's a restring or deleted.

		if ((*obj)->item_number - i >= 0)
			if (!str_cmp((*obj)->short_description, ((struct obj_data*)obj_index[(*obj)->item_number - i].item)->short_description))
					{
				newitem = (*obj)->item_number - i;
				break;
			}

		if ((*obj)->item_number + i <= top_of_objt)
			if (!str_cmp((*obj)->short_description, ((struct obj_data*)obj_index[(*obj)->item_number + i].item)->short_description))
					{
				newitem = (*obj)->item_number + i;
				break;
			}
	}
	if (newitem == -1)
		return FALSE;

	*obj = clone_object(newitem); // Fixed!
	return TRUE;
}

