#ifndef CHARACTER_H_
#define CHARACTER_H_
/******************************************************************************
| $Id: character.h,v 1.3 2002/06/20 21:39:49 pirahna Exp $
| Description: This file contains the header information for the character
|   class implementation.
*/
#include <affect.h>  /* MAX_AFFECTS, etc.. */
#include <alias.h>   /* struct char_player_alias, MAX_ALIASES, etc.. */
#include <structs.h> /* byte, ubyte, sh_int, etc.. */
#include <timeinfo.h> // time data, etc..
#include <event.h> // eventBrief
#include <isr.h>   // SAVE_TYPE_MAX

//#define START_ROOM        3001 // Where you login
#define START_ROOM      350 // Where you login
//#define SECOND_START_ROOM 3059 // Where you go if killed in start room
#define SECOND_START_ROOM 351 // Where you go if killed in start room

#define PASSWORD_LEN    20
#define DESC_LENGTH     80
#define CHAR_VERSION    -4
#define MAX_NAME_LENGTH 12

/************************************************************************
| max stuff - this is needed almost everywhere
*/
#define MAX_WEAR     22
#define MAX_AFFECT   25

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

// * ------- Begin MOBProg stuff ----------- *

typedef struct  mob_prog_data           MPROG_DATA;
typedef struct  mob_prog_act_list       MPROG_ACT_LIST;

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

#define MPROG_MAX_TYPE_VALUE 8192  // this should be the next bitvector up from max

// * ------- End MOBProg stuff ----------- *

struct char_skill_data
{
    sh_int skillnum;          // ID # of skill.
    byte   learned;           // % chance for success must be > 0
    long   unused[5];         // for future use

    char_skill_data * next;   // Next skill in ch's skill list    
};

struct class_skill_defines
{
    char * skillname;         // name of skill
    sh_int skillnum;          // ID # of skill
    sh_int levelavailable;    // what level class can get it
    sh_int maximum;           // maximum value PC can train it to (1-100)
    long   trainer;           // what mob trains them (only one currently) 0 = any
    char * clue;              // what mob will say if he can't train them
};

/* Used in CHAR_FILE_U *DO*NOT*CHANGE* */
struct affected_type
{
    long type;           /* The type of spell that caused ths      */
    sh_int duration;      /* For how long its effects will last      */
    sbyte modifier;       /* This is added to apropriate ability     */
    byte location;        /* Tells which ability to change(APPLY_XXX)*/
    long bitvector;       /* Tells which bits to set (AFF_XXX)       */

    struct affected_type *next;
};


struct follow_type
{
    CHAR_DATA *follower;
    struct follow_type *next;
};

struct pc_data
{
    char pwd[PASSWORD_LEN+1];
    char *ignoring;                 /* List of ignored names */

    struct char_player_alias * alias; /* Aliases */

    uint32 totalpkills;         // total number of pkills THIS LOGIN
    uint64 totalpkillslv;       // sum of levels of pkills THIS LOGIN
    uint32 pdeathslogin;        // pdeaths THIS LOGIN

    uint64 rdeaths;             // total number of real deaths
    uint64 pdeaths;             // total number of times pkilled
    uint64 pkills;              // # of pkills ever 
    uint64 pklvl;               // # sum of levels of pk victims ever
    uint64 group_kills;         // # of kills for group 
    uint64 grplvl;              // sum of levels of group victims 

    char *last_site;                /* Last login from.. */
    struct time_data time;          // PC time data.  logon, played, birth
   
    uint32 bad_pw_tries;        // How many times people have entered bad pws

    long wizinvis;

    uint16 practices;         // How many can you learn yet this level
    uint16 specializations;   // How many specializations a player has left

     int16 saves_mods[SAVE_TYPE_MAX+1];  // Character dependant mods to saves (meta'able)

    uint64 bank;           /* gold in bank                            */

    uint64 toggles;            // Bitvector for toggles.  (Was specials.act)
    uint64 punish;             // flags for punishments
    uint64 quest_bv1;          // 1st bitvector for quests

    char *poofin;       /* poofin message */
    char *poofout;      /* poofout message */    
    char *prompt;       /* Sadus' disguise.. unused */

    char *rooms;       /*  Builder Room Range   */
    char *mobiles;     /*  Builder Mobile Range */
    char *objects;     /*  Builder Object Range */

     int32 last_mob_edit;       // vnum of last mob edited
     int32 last_obj_edit;       // vnum of last obj edited

    char *last_tell;          /* last person who told           */
     int16 last_mess_read;     /* for reading messages */

    // TODO: these 3 need to become PLR toggles
    bool holyLite;          // Holy lite mode
    bool stealth;           // If on, you are more stealth then norm. (god)
    bool incognito;         // invis imms will be seen by people in same room

    bool possesing; 	      /*  is the person possessing? */
};

struct mob_data
{
     int32 nr;
     sbyte default_pos;    // Default position for NPC
     sbyte last_direction; // Last direction the mobile went in
    uint64 attack_type;    // Bitvector of damage type for bare-handed combat
    uint64 actflags;       // flags for NPC behavior

     int16 damnodice;         // The number of damage dice's           
     int16 damsizedice;       // The size of the damage dice's         

    char *fears;       /* will flee from ths person on sight     */
    char *hatred;      /* List of PC's I hate */

    MPROG_ACT_LIST *    mpact; // list of MOBProgs
     int16                 mpactnum; // num
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

    sbyte str; 
    sbyte raw_str; 
    sbyte intel;
    sbyte raw_intel;
    sbyte wis;
    sbyte raw_wis;
    sbyte dex;
    sbyte raw_dex;
    sbyte con;
    sbyte raw_con;

    sbyte conditions[3];      // Drunk full etc.                       

    ubyte weight;       /* PC/NPC's weight */
    ubyte height;       /* PC/NPC's height */

    int16 hometown;      /* PC/NPC home town */
    uint64 gold;           /* Money carried                           */
    uint64 plat;           /* Platinum                                */
     int64 exp;            /* The experience of the player            */
     int64 in_room;

    uint64 immune;         // Bitvector of damage types I'm immune to
    uint64 resist;         // Bitvector of damage types I'm resistant to
    uint64 suscept;        // Bitvector of damage types I'm susceptible to
     int16 saves[SAVE_TYPE_MAX+1];  // Saving throw bonuses

     int64 mana;         
     int64 max_mana;     /* Not useable                             */
     int64 raw_mana;     /* before int bonus                        */
     int64 hit;   
     int64 max_hit;      /* Max hit for NPC                         */
     int64 raw_hit;      /* before con bonus                        */
     int64 move;  
     int64 raw_move;
     int64 max_move;     /* Max move for NPC                        */
     int64 ki;
     int64 max_ki;
     int64 raw_ki;

     int16 alignment;          // +-1000 for alignments                 

    uint32 hpmetas;             // total number of times meta'd hps
    uint32 manametas;           // total number of times meta'd mana
    uint32 movemetas;           // total number of times meta'd moves

     int16 hit_regen;           // modifier to hp regen
     int16 mana_regen;          // modifier to mana regen
     int16 move_regen;          // modifier to move regen
     int16 ki_regen;            // modifier to ki regen

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

    uint64 affected_by;                // Quick reference bitvector for spell affects
    uint64 affected_by2;               // More quick reference bitvectors 
    uint64 combat;                     // Bitvector for combat related flags (bash, stun, shock)
    uint64 misc;                       // Bitvector for IS_MOB/logs/channels.  So possessed mobs can channel

    CHAR_DATA *fighting;                 /* Opponent     */
    CHAR_DATA *next;                     /* Next anywhere in game */
    CHAR_DATA *next_in_room;             /* Next in room */
    CHAR_DATA *next_fighting;            /* Next fighting */

    struct follow_type *followers;  /* List of followers */
    CHAR_DATA *master;              /* Who is char following? */
    char *group_name;                /* Name of group */
    
    int32 timer;                         // Timer for update                       

// TODO - see if we can move the "wait" timer from desc to char
// since we need something to lag mobs too

    int32 deaths;                   /* deaths is reused for mobs as a
                                       timer to check for WAIT_STATE */

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
    byte sex;         /* Sex */
    byte c_class;     /* Class */
    byte race;        /* Race */
    byte level;       /* Level */
   
    sbyte raw_str;
    sbyte raw_intel;
    sbyte raw_wis;
    sbyte raw_dex;
    sbyte raw_con;
    sbyte conditions[3]; 

    ubyte weight;
    ubyte height;

    long hometown;
    long gold;
    long plat;
    long exp;
    long immune;
    long resist;
    long suscept;

    sh_int mana;        // current
    sh_int raw_mana;    // max without eq/stat bonuses
    sh_int hit;
    sh_int raw_hit;
    sh_int move;
    sh_int raw_move;
    sh_int ki;
    sh_int raw_ki;

    sh_int alignment;
    sh_int hpmetas;
    sh_int manametas;
    sh_int movemetas;

    sh_int armor;       // have to save these since mobs have different bases
    sh_int hitroll;
    sh_int damroll;
    uint32 afected_by;  // SHOULD BE 64
    uint32 afected_by2; // SHOULD BE 64

    sh_int apply_saving_throw[5]; // no longer used, but kept so I don't have to convert all the files
                                  // right now.
                                  // TODO - write a pfile convertor, so I can remove these extra 5 ints
                                  // without pwiping everything all over again

    long misc;          // channel flags

    sh_int clan; 
    sh_int load_room;                  // Which room to place char in

    long   extra_ints[5];             // available just in case
};

#endif
