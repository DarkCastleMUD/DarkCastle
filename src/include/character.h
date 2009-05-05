#ifndef CHARACTER_H_
#define CHARACTER_H_
/******************************************************************************
| $Id: character.h,v 1.73 2009/05/05 19:01:36 shane Exp $
| Description: This file contains the header information for the character
|   class implementation.
*/

#define  COMPILE_WITH_CHANGES 1

#include <affect.h>  /* MAX_AFFECTS, etc.. */
#include <alias.h>   /* struct char_player_alias, MAX_ALIASES, etc.. */
#include <structs.h> /* ubyte, ubyte, int16, etc.. */
#include <timeinfo.h> // time data, etc..
#include <event.h> // eventBrief
#include <isr.h>   // SAVE_TYPE_MAX
#include <utility.h>
#include <mobile.h>
#include <queue>
#include <quest.h>
#include <map>
#include <sys/time.h>
#include <string>

#define ASIZE 32
#define MAX_GOLEMS           2 // amount of golems above +1

#define START_ROOM        3001 // Where you login
#define CFLAG_HOME        3014 // Where the champion flag normally rests
#define SECOND_START_ROOM 3059 // Where you go if killed in start room

#define PASSWORD_LEN    20
#define DESC_LENGTH     80
#define CHAR_VERSION    -4
#define MAX_NAME_LENGTH 12

/************************************************************************
| max stuff - this is needed almost everywhere
*/
#define MAX_WEAR     23

struct char_data;
typedef struct char_data CHAR_DATA;

#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2

#define POSITION_DEAD        0
//#define POSITION_MORTALLYW   1
//#define POSITION_INCAP       2
#define POSITION_STUNNED     3
#define POSITION_SLEEPING    4
#define POSITION_RESTING     5
#define POSITION_SITTING     6
#define POSITION_FIGHTING    7
#define POSITION_STANDING    8

#define STRENGTH             1
#define DEXTERITY            2
#define INTELLIGENCE         3
#define WISDOM               4
#define CONSTITUTION         5

#define STR STRENGTH
#define DEX DEXTERITY
#define INT INTELLIGENCE
#define WIS WISDOM
#define CON CONSTITUTION // Gawddamn I'm lazy ;)

#define MAX_HIDE 10
#define CHAMPION_ITEM 45

// * ------- Begin MOBProg stuff ----------- *

typedef struct  mob_prog_data           MPROG_DATA;
typedef struct  mob_prog_act_list       MPROG_ACT_LIST;

struct tempvariable
{
  struct tempvariable *next;
  char *name;
  char *data;
  int16 save; // save or not
};

struct  mob_prog_act_list
{
    MPROG_ACT_LIST * next;
    char *           buf;
    CHAR_DATA *      ch;
    OBJ_DATA *       obj;
    void *           vo;
};

struct  mob_prog_data
{
    MPROG_DATA *next;
    int         type;
    char *      arglist;
    char *      comlist;
};

#define ERROR_PROG        -1
#define IN_FILE_PROG       0
#define ACT_PROG           1
#define SPEECH_PROG        2
#define RAND_PROG          4
#define FIGHT_PROG         8
#define DEATH_PROG        16
#define HITPRCNT_PROG     32
#define ENTRY_PROG        64
#define GREET_PROG       128
#define ALL_GREET_PROG   256
#define GIVE_PROG        512
#define BRIBE_PROG      1024
#define CATCH_PROG      2048
#define ATTACK_PROG     4096
#define ARAND_PROG      8192
#define LOAD_PROG      16384
#define COMMAND_PROG   16384 <<1
#define WEAPON_PROG    16384 <<2
#define ARMOUR_PROG    16384 <<3
#define CAN_SEE_PROG   16384 <<4
#define DAMAGE_PROG    16384 <<5
#define MPROG_MAX_TYPE_VALUE (16384 << 6)

// * ------- End MOBProg stuff ----------- *


struct char_skill_data
{
    int16  skillnum;          // ID # of skill.
    int16  learned;           // % chance for success must be > 0
    int32  unused[5];         // for future use

    char_skill_data * next;   // Next skill in ch's skill list    
};

struct class_skill_defines
{
    char * skillname;         // name of skill
    int16  skillnum;          // ID # of skill
    int16  levelavailable;    // what level class can get it
    int16  maximum;           // maximum value PC can train it to (1-100)
    int16  group;             // which class tree group it is assigned
    int16  attrs;	      // What attributes the skill is based on
};

/* Used in CHAR_FILE_U *DO*NOT*CHANGE* */
struct affected_type
{
    uint32 type;           /* The type of spell that caused ths      */
    int16  duration;       /* For how long its effects will last      */
    int32  duration_type;  
    int32  modifier;       /* This is added to apropriate ability     */
    int32  location;       /* Tells which ability to change(APPLY_XXX)*/
    int32 bitvector;      /* Tells which bits to set (AFF_XXX)       */
    std::string caster;
    struct affected_type *next;
};


struct follow_type
{
    CHAR_DATA *follower;
    struct follow_type *next;
};
/*
#define VIEW_ACCESS 1
#define DEPOSIT_ACCESS 2
#define WITHDRAW_ACCESS 3
#define FULL_ACCESS 4

struct vault_access_data
{
  struct vault_access_data *next;
  bool self; // own vault, or remote vault
  char *name; // also used as an integer for clans. 
  int segment;
  bool view;
  bool deposit;
  bool withdraw;
};


struct player_vault
{
  char *segment[20];
  char *last_search;
  int max_contain, contains;
  int nr_items;
  uint amt;
  OBJ_DATA *content;
  struct vault_access_data *acc;
};
*/

// DO NOT change most of these types without checking the save files
// first, or you will probably end up corrupting all the pfiles
struct pc_data
{
    char pwd[PASSWORD_LEN+1];
    char *ignoring;                 /* List of ignored names */

    struct char_player_alias * alias; /* Aliases */

    uint32 totalpkills;         // total number of pkills THIS LOGIN
    uint32 totalpkillslv;       // sum of levels of pkills THIS LOGIN
    uint32 pdeathslogin;        // pdeaths THIS LOGIN

    uint32 rdeaths;             // total number of real deaths
    uint32 pdeaths;             // total number of times pkilled
    uint32 pkills;              // # of pkills ever 
    uint32 pklvl;               // # sum of levels of pk victims ever
    uint32 group_kills;         // # of kills for group 
    uint32 grplvl;              // sum of levels of group victims 

    char *last_site;                /* Last login from.. */
    struct time_data time;          // PC time data.  logon, played, birth
   
    uint32 bad_pw_tries;        // How many times people have entered bad pws

     int16 statmetas;           // How many times I've metad a stat
                                // This number could go negative from stat loss
    uint16 kimetas;             // How many times I've metad ki (pc only)

    long wizinvis;

    uint16 practices;         // How many can you learn yet this level
    uint16 specializations;   // How many specializations a player has left

     int16 saves_mods[SAVE_TYPE_MAX+1];  // Character dependant mods to saves (meta'able)

     uint32 bank;           /* gold in bank                            */

    uint32 toggles;            // Bitvector for toggles.  (Was specials.act)
    uint32 punish;             // flags for punishments
    uint32 quest_bv1;          // 1st bitvector for quests

    char *poofin;       /* poofin message */
    char *poofout;      /* poofout message */    
    char *prompt;       /* Sadus' disguise.. unused */

    int16 buildLowVnum, buildHighVnum;
    int16 buildMLowVnum, buildMHighVnum;
    int16 buildOLowVnum, buildOHighVnum;
    obj_data *skillchange; /* Skill changing equipment. */

     int32 last_mob_edit;       // vnum of last mob edited
     int32 last_obj_edit;       // vnum of last obj edited

    char *last_tell;          /* last person who told           */
     int16 last_mess_read;     /* for reading messages */

    // TODO: these 3 need to become PLR toggles
    bool holyLite;          // Holy lite mode
    bool stealth;           // If on, you are more stealth then norm. (god)
    bool incognito;         // invis imms will be seen by people in same room

    bool possesing; 	      /*  is the person possessing? */
    bool unjoinable;        // Do NOT autojoin
    struct char_data *golem; // CURRENT golem. 
    bool hide[MAX_HIDE];
    CHAR_DATA *hiding_from[MAX_HIDE];
    std::queue<char *> *away_msgs;
    char *joining;
    uint32 quest_points;
    int16  quest_current[QUEST_MAX];
    uint32 quest_current_ticksleft[QUEST_MAX];
    int16  quest_cancel[QUEST_CANCEL];
    uint32 quest_complete[QUEST_TOTAL/ASIZE+1];
    char *last_prompt;
    std::multimap<int, std::pair<timeval, timeval> > *lastseen;
};

struct mob_data
{
    int32 nr;
    sbyte default_pos;    // Default position for NPC
    sbyte last_direction; // Last direction the mobile went in
    uint32 attack_type;    // Bitvector of damage type for bare-handed combat
    uint32 actflags[ACT_MAX/ASIZE+1]; // flags for NPC behavior

    int16 damnodice;         // The number of damage dice's           
    int16 damsizedice;       // The size of the damage dice's         

    char *fears;       /* will flee from ths person on sight     */
    char *hatred;      /* List of PC's I hate */

    MPROG_ACT_LIST *    mpact; // list of MOBProgs
     int16                 mpactnum; // num
    int32 last_room; // Room rnum the mob was last in. Used
		      // For !magic,!track changing flags.
    struct threat_struct *threat;
    struct reset_com *reset;
};


// CHAR_DATA, char_data
// This contains all memory items for a player/mob
// All non-specific data is held in this structure
// PC/MOB specific data are held in the appropriate pointed-to structs
struct char_data
{
    struct mob_data * mobdata;
    struct pc_data * pcdata;
    
    struct descriptor_data *desc;       // NULL normally for mobs 

    char *name;         // Keyword 'kill X'
    char *short_desc;   // Action 'X hits you.'
    char *long_desc;    // For 'look room'
    char *description;  // For 'look mob'
    char *title;

    sbyte sex;
    sbyte c_class;
    sbyte race;
    sbyte level;
    sbyte position;      // Standing, sitting, fighting

    sbyte str;         sbyte raw_str;        sbyte str_bonus;
    sbyte intel;       sbyte raw_intel;      sbyte intel_bonus;
    sbyte wis;         sbyte raw_wis;        sbyte wis_bonus;
    sbyte dex;         sbyte raw_dex;        sbyte dex_bonus;
    sbyte con;         sbyte raw_con;        sbyte con_bonus;

    sbyte conditions[3];      // Drunk full etc.                       

    ubyte weight;       /* PC/NPC's weight */
    ubyte height;       /* PC/NPC's height */

    int16 hometown;      /* PC/NPC home town */
    int64 gold;           /* Money carried                           */
    uint32 plat;           /* Platinum                                */
     int64 exp;            /* The experience of the player            */
			   /* Changed to a long long */
     int32 in_room;

    uint32 immune;         // Bitvector of damage types I'm immune to
    uint32 resist;         // Bitvector of damage types I'm resistant to
    uint32 suscept;        // Bitvector of damage types I'm susceptible to
     int16 saves[SAVE_TYPE_MAX+1];  // Saving throw bonuses

     int32 mana;         
     int32 max_mana;     /* Not useable                             */
     int32 raw_mana;     /* before int bonus                        */
     int32 hit;   
     int32 max_hit;      /* Max hit for NPC                         */
     int32 raw_hit;      /* before con bonus                        */
     int32 move;  
     int32 raw_move;
     int32 max_move;     /* Max move for NPC                        */
     int32 ki;
     int32 max_ki;
     int32 raw_ki;

     int16 alignment;          // +-1000 for alignments                 

    uint32 hpmetas;             // total number of times meta'd hps
    uint32 manametas;           // total number of times meta'd mana
    uint32 movemetas;           // total number of times meta'd moves
    uint32 acmetas;             // total number of times meta'd ac
     int32 agemetas;            // number of years age has been meta'd

     int16 hit_regen;           // modifier to hp regen
     int16 mana_regen;          // modifier to mana regen
     int16 move_regen;          // modifier to move regen
     int16 ki_regen;            // modifier to ki regen

     int16 melee_mitigation;    // modifies melee damage
     int16 spell_mitigation;    // modified spell damage
     int16 song_mitigation;     // modifies song damage
     int16 spell_reflect;

     int16 clan;                       /* Clan the char is in */

     int16 armor;                 // Armor class
     int16 hitroll;               // Any bonus or penalty to the hit roll
     int16 damroll;               // Any bonus or penalty to the damage roll

     int16 glow_factor;           // Amount that the character glows

    obj_data * beacon;       /* pointer to my beacon */

     int16 song_timer;       /* status for songs being sung */
     int16 song_number;      /* number of song being sung */
    char * song_data;        /* args for the songs */

    struct obj_data *equipment[MAX_WEAR]; // Equipment List

    struct char_skill_data * skills;   // Skills List
    struct affected_type *affected;    // Affected by list
    struct obj_data *carrying;         // Inventory List

     int16 poison_amount;              // How much poison damage I'm taking every few seconds

     int16 carry_weight;               // Carried weight
     int16 carry_items;                // Number of items carried                

    char *hunting;                     // Name of "track" target
    char *ambush;                      // Name of "ambush" target

    char_data * guarding;              // Pointer to who I am guarding
    follow_type * guarded_by;          // List of people guarding me

    uint32 affected_by[AFF_MAX/ASIZE+1]; // Quick reference bitvector for spell affects
    uint32 combat;                     // Bitvector for combat related flags (bash, stun, shock)
    uint32 misc;                       // Bitvector for IS_MOB/logs/channels.  So possessed mobs can channel

    CHAR_DATA *fighting;                 /* Opponent     */
    CHAR_DATA *next;                     /* Next anywhere in game */
    CHAR_DATA *next_in_room;             /* Next in room */
    CHAR_DATA *next_fighting;            /* Next fighting */
    OBJ_DATA *altar;
    struct follow_type *followers;  /* List of followers */
    CHAR_DATA *master;              /* Who is char following? */
    char *group_name;                /* Name of group */
    
    int32 timer;                         // Timer for update
    int32 shotsthisround;                // Arrows fired this round
    int32 spellcraftglyph;               // Used for spellcraft glyphs
    bool  changeLeadBonus;
    int32 curLeadBonus;
    int   cRooms;			// number of rooms consecrated/desecrated

// TODO - see if we can move the "wait" timer from desc to char
// since we need something to lag mobs too

    int32 deaths;                   /* deaths is reused for mobs as a
                                       timer to check for WAIT_STATE */

    int cID; // character ID

    struct timer_data *timerAttached;
    struct tempvariable *tempVariable;
    int spelldamage;
#ifdef USE_SQL
    int player_id;
#endif
    int spec;
};


// This structure is written to the disk.  DO NOT MODIFY THIS STRUCTURE
// There is a method in save.C for adding additional items to the pfile
// Check there if you need to add something
// This structure contains everything that would be serialized for both
// a 'saved' mob, and for a player
// Note, any "strings" are done afterwards in the functions.  Since these
// are variable length, we can't do them with a single write
struct char_file_u
{
    sbyte sex;         /* Sex */
    sbyte c_class;     /* Class */
    sbyte race;        /* Race */
    sbyte level;       /* Level */
   
    sbyte raw_str;
    sbyte raw_intel;
    sbyte raw_wis;
    sbyte raw_dex;
    sbyte raw_con;
    sbyte conditions[3]; 

    ubyte weight;
    ubyte height;

    int16 hometown;
    uint32 gold;
    uint32 plat;
    int64 exp;
    uint32 immune;
    uint32 resist;
    uint32 suscept;

    int32 mana;        // current
    int32 raw_mana;    // max without eq/stat bonuses
    int32 hit;
    int32 raw_hit;
    int32 move;
    int32 raw_move;
    int32 ki;
    int32 raw_ki;

    int16 alignment;
   uint32 hpmetas; // Used by familiars too... why not.
   uint32 manametas;
   uint32 movemetas;

    int16 armor;       // have to save these since mobs have different bases
    int16 hitroll;
    int16 damroll;
    int32 afected_by;
    int32 afected_by2;
    uint32 misc;          // channel flags

    int16 clan; 
    int32 load_room;                  // Which room to place char in

    uint32 acmetas;
    int32 agemetas;
    int32 extra_ints[3];             // available just in case
};


void clear_hunt(void *arg1, void *arg2, void *arg3);
void clear_hunt(char *arg1, CHAR_DATA *arg2, void *arg3);
void prepare_character_for_sixty(CHAR_DATA *ch);

#endif
