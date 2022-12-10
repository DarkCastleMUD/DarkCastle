
#ifndef CHARACTER_H_
#define CHARACTER_H_
struct char_data;
/******************************************************************************
| $Id: character.h,v 1.85 2014/07/26 23:21:23 jhhudso Exp $
| Description: This file contains the header information for the character
|   class implementation.
*/
#include <sys/time.h>
#include <stdint.h>
#include <strings.h>

#include <queue>
#include <map>
#include <string>
#include <vector>
#include <string>
#include <algorithm>
#include <set>

#include "DC.h"
#include "affect.h"   /* MAX_AFFECTS, etc.. */
#include "alias.h"    /* struct char_player_alias, MAX_ALIASES, etc.. */
#include "structs.h"  /* uint8_t, uint8_t, int16_t, etc.. */
#include "timeinfo.h" // time data, etc..
#include "event.h"    // eventBrief
#include "isr.h"      // SAVE_TYPE_MAX
#include "mobile.h"
#include "sing.h"
#include "quest.h"
#include "interp.h"
#include "utility.h"

struct char_data;

struct strcasecmp_compare
{
    bool operator()(const string &l, const string &r) const
    {
        return strcasecmp(l.c_str(), r.c_str()) < 0;
    }
};

struct ignore_entry
{
    bool ignore;
    uint64_t ignored_count;
};

/*
bool ignore_entry::operator=(ignore_entry& a, ignore_entry& b)
{
    return ((a.ignore == b.ignore) && (a.ignored_count == b.ignored_count));
}
*/

typedef std::map<std::string, ignore_entry, strcasecmp_compare> ignoring_t;

class communication;
typedef std::queue<communication> history_t;

#define ASIZE 32
#define MAX_GOLEMS 2 // amount of golems above +1

#define START_ROOM 3001        // Where you login
#define CFLAG_HOME 3014        // Where the champion flag normally rests
#define SECOND_START_ROOM 3059 // Where you go if killed in start room
#define FARREACH_START_ROOM 17868
#define THALOS_START_ROOM 5317

#define PASSWORD_LEN 20
#define DESC_LENGTH 80
#define CHAR_VERSION -4
#define MAX_NAME_LENGTH 12

/************************************************************************
| max stuff - this is needed almost everywhere
*/
#define MAX_WEAR 23

#define SEX_NEUTRAL 0
#define SEX_MALE 1
#define SEX_FEMALE 2

#define POSITION_DEAD 0
// #define POSITION_MORTALLYW   1
// #define POSITION_INCAP       2
#define POSITION_STUNNED 3
#define POSITION_SLEEPING 4
#define POSITION_RESTING 5
#define POSITION_SITTING 6
#define POSITION_FIGHTING 7
#define POSITION_STANDING 8

#define STRENGTH 1
#define DEXTERITY 2
#define INTELLIGENCE 3
#define WISDOM 4
#define CONSTITUTION 5

#define STR STRENGTH
#define DEX DEXTERITY
#define INT INTELLIGENCE
#define WIS WISDOM
#define CON CONSTITUTION // Gawddamn I'm lazy ;)

#define MAX_HIDE 10
#define CHAMPION_ITEM 45

// * ------- Begin MOBProg stuff ----------- *

typedef int16_t skill_t;
typedef map<skill_t, struct char_skill_data> skill_list_t;

struct tempvariable
{
    struct tempvariable *next;
    char *name;
    char *data;
    int16_t save; // save or not
};

struct mob_prog_act_list
{
    mob_prog_act_list *next;
    char *buf;
    char_data *ch;
    obj_data *obj;
    void *vo;
};

struct mob_prog_data
{
    mob_prog_data *next;
    int type;
    char *arglist;
    char *comlist;
};

#define ERROR_PROG -1
#define IN_FILE_PROG 0
#define ACT_PROG 1
#define SPEECH_PROG 2
#define RAND_PROG 4
#define FIGHT_PROG 8
#define DEATH_PROG 16
#define HITPRCNT_PROG 32
#define ENTRY_PROG 64
#define GREET_PROG 128
#define ALL_GREET_PROG 256
#define GIVE_PROG 512
#define BRIBE_PROG 1024
#define CATCH_PROG 2048
#define ATTACK_PROG 4096
#define ARAND_PROG 8192
#define LOAD_PROG 16384
#define COMMAND_PROG 16384 << 1
#define WEAPON_PROG 16384 << 2
#define ARMOUR_PROG 16384 << 3
#define CAN_SEE_PROG 16384 << 4
#define DAMAGE_PROG 16384 << 5
#define MPROG_MAX_TYPE_VALUE (16384 << 6)

// * ------- End MOBProg stuff ----------- *

struct char_skill_data
{
    skill_t skillnum;  // ID # of skill.
    int16_t learned;   // % chance for success must be > 0
    int32_t unused[5]; // for future use
};

struct class_skill_defines
{
    char *skillname;        // name of skill
    int16_t skillnum;       // ID # of skill
    int16_t levelavailable; // what level class can get it
    int16_t maximum;        // maximum value PC can train it to (1-100)
    uint8_t group;          // which class tree group it is assigned
    int16_t attrs;          // What attributes the skill is based on
};

/* Used in CHAR_FILE_U *DO*NOT*CHANGE* */
struct affected_type
{
    uint32_t type = {};    /* The type of spell that caused ths      */
    int16_t duration = {}; /* For how long its effects will last      */
    int32_t duration_type = {};
    int32_t modifier = {};  /* This is added to apropriate ability     */
    int32_t location = {};  /* Tells which ability to change(APPLY_XXX)*/
    int32_t bitvector = {}; /* Tells which bits to set (AFF_XXX)       */
    string caster = {};
    struct affected_type *next = {};
    char_data *origin = {};
    char_data *victim = {};
};

struct follow_type
{
    char_data *follower;
    struct follow_type *next;
};

// DO NOT change most of these types without checking the save files
// first, or you will probably end up corrupting all the pfiles
struct pc_data
{
    char pwd[PASSWORD_LEN + 1] = {};
    ignoring_t ignoring = {}; /* List of ignored names */

    struct char_player_alias *alias = {}; /* Aliases */

    uint32_t totalpkills = {};   // total number of pkills THIS LOGIN
    uint32_t totalpkillslv = {}; // sum of levels of pkills THIS LOGIN
    uint32_t pdeathslogin = {};  // pdeaths THIS LOGIN

    uint32_t rdeaths = {};      // total number of real deaths
    uint32_t pdeaths = {};      // total number of times pkilled
    uint32_t pkills = {};       // # of pkills ever
    uint32_t pklvl = {};        // # sum of levels of pk victims ever
    uint32_t group_pkills = {}; // # of pkills for group
    uint32_t grpplvl = {};      // sum of levels of group pkill victims
    uint32_t group_kills = {};  // # of kills for group

    char *last_site = {};       /* Last login from.. */
    struct time_data time = {}; // PC time data.  logon, played, birth

    uint32_t bad_pw_tries = {}; // How many times people have entered bad pws

    int16_t statmetas = {}; // How many times I've metad a stat
                            // This number could go negative from stat loss
    uint16_t kimetas = {};  // How many times I've metad ki (pc only)

    int32_t wizinvis = {};

    uint16_t practices = {};       // How many can you learn yet this level
    uint16_t specializations = {}; // How many specializations a player has left

    int16_t saves_mods[SAVE_TYPE_MAX + 1] = {}; // Character dependant mods to saves (meta'able)

    uint32_t bank = {}; /* gold in bank                            */

    uint32_t toggles = {};   // Bitvector for toggles.  (Was specials.act)
    uint32_t punish = {};    // flags for punishments
    uint32_t quest_bv1 = {}; // 1st bitvector for quests

    char *poofin = {};  /* poofin message */
    char *poofout = {}; /* poofout message */
    char *prompt = {};  /* Sadus' disguise.. unused */

    int16_t buildLowVnum = {}, buildHighVnum = {};
    int16_t buildMLowVnum = {}, buildMHighVnum = {};
    int16_t buildOLowVnum = {}, buildOHighVnum = {};
    obj_data *skillchange = {}; /* Skill changing equipment. */

    int32_t last_mob_edit = {}; // vnum of last mob edited
    vnum_t last_obj_vnum = {};  // vnum of last obj edited

    string last_tell = {};       /* last person who told           */
    int16_t last_mess_read = {}; /* for reading messages */

    // TODO: these 3 need to become PLR toggles
    bool holyLite = {};  // Holy lite mode
    bool stealth = {};   // If on, you are more stealth then norm. (god)
    bool incognito = {}; // invis imms will be seen by people in same room

    bool possesing = {};   /*  is the person possessing? */
    bool unjoinable = {};  // Do NOT autojoin
    char_data *golem = {}; // CURRENT golem.
    bool hide[MAX_HIDE] = {};
    char_data *hiding_from[MAX_HIDE] = {};
    std::queue<string> *away_msgs = {};
    history_t *tell_history = {};
    history_t *gtell_history = {};
    char *joining = {};
    uint32_t quest_points = {};
    int16_t quest_current[QUEST_MAX] = {};
    uint32_t quest_current_ticksleft[QUEST_MAX] = {};
    int16_t quest_cancel[QUEST_CANCEL] = {};
    uint32_t quest_complete[QUEST_TOTAL / ASIZE + 1] = {};
    char *last_prompt = {};
    std::multimap<int, std::pair<timeval, timeval>> *lastseen = {};
    uint8_t profession = {};
    bool multi = {};
    std::map<string, string> *options = {};
};

enum mob_type_t
{
    MOB_NORMAL = 0,
    MOB_GUARD,
    MOB_CLAN_GUARD,
    MOB_TYPE_FIRST = MOB_NORMAL,
    MOB_TYPE_LAST = MOB_CLAN_GUARD
};
const int MAX_MOB_VALUES = 4;

struct mob_flag_data
{
    int32_t value[MAX_MOB_VALUES]; /* Mob type-specific value numbers */
    mob_type_t type;               /* Type of mobile                     */
};

struct mob_data
{
public:
    int32_t nr = {};
    int8_t default_pos = {};                     // Default position for NPC
    int8_t last_direction = {};                  // Last direction the mobile went in
    uint32_t attack_type = {};                   // Bitvector of damage type for bare-handed combat
    uint32_t actflags[ACT_MAX / ASIZE + 1] = {}; // flags for NPC behavior

    int16_t damnodice = {};   // The number of damage dice's
    int16_t damsizedice = {}; // The size of the damage dice's

    char *fears = {};  /* will flee from ths person on sight     */
    char *hatred = {}; /* List of PC's I hate */

    mob_prog_act_list *mpact = {}; // list of MOBProgs
    int16_t mpactnum = {};         // num
    int32_t last_room = {};        // Room rnum the mob was last in. Used
                                   // For !magic,!track changing flags.
    struct threat_struct *threat = {};
    struct reset_com *reset = {};
    mob_flag_data mob_flags = {}; /* Mobile information               */
    bool paused = {};

    void setObject(obj_data *);
    obj_data *getObject(void);
    bool isObject(void);

private:
    obj_data *object = {};
};

// char_data, char_data
// This contains all memory items for a player/mob
// All non-specific data is held in this structure
// PC/MOB specific data are held in the appropriate pointed-to structs
struct char_data
{
    struct mob_data *mobdata = nullptr;
    struct pc_data *pcdata = nullptr;
    struct obj_data *objdata = nullptr;

    struct descriptor_data *desc = nullptr; // NULL normally for mobs

    char *name = nullptr;        // Keyword 'kill X'
    char *short_desc = nullptr;  // Action 'X hits you.'
    char *long_desc = nullptr;   // For 'look room'
    char *description = nullptr; // For 'look mob'
    char *title = nullptr;

    int8_t sex = {};
    int8_t c_class = {};
    int8_t race = {};
    int8_t level = {};
    int8_t position = {}; // Standing, sitting, fighting

    int8_t str = {};
    int8_t raw_str = {};
    int8_t str_bonus = {};
    int8_t intel = {};
    int8_t raw_intel = {};
    int8_t intel_bonus = {};
    int8_t wis = {};
    int8_t raw_wis = {};
    int8_t wis_bonus = {};
    int8_t dex = {};
    int8_t raw_dex = {};
    int8_t dex_bonus = {};
    int8_t con = {};
    int8_t raw_con = {};
    int8_t con_bonus = {};

    int8_t conditions[3] = {}; // Drunk full etc.

    uint8_t weight = {}; /* PC/NPC's weight */
    uint8_t height = {}; /* PC/NPC's height */

    int16_t hometown = {}; /* PC/NPC home town */
    int64_t gold = {};     /* Money carried                           */
    uint32_t plat = {};    /* Platinum                                */
    int64_t exp = {};      /* The experience of the player            */
    int32_t in_room = {};

    uint32_t immune = {};                  // Bitvector of damage types I'm immune to
    uint32_t resist = {};                  // Bitvector of damage types I'm resistant to
    uint32_t suscept = {};                 // Bitvector of damage types I'm susceptible to
    int16_t saves[SAVE_TYPE_MAX + 1] = {}; // Saving throw bonuses

    int32_t mana = {};
    int32_t max_mana = {}; /* Not useable                             */
    int32_t raw_mana = {}; /* before int bonus                        */
    int32_t hit = {};
    int32_t max_hit = {}; /* Max hit for NPC                         */
    int32_t raw_hit = {}; /* before con bonus                        */
    int32_t move = {};
    int32_t raw_move = {};
    int32_t max_move = {}; /* Max move for NPC                        */
    int32_t ki = {};
    int32_t max_ki = {};
    int32_t raw_ki = {};

    int16_t alignment = {}; // +-1000 for alignments

    uint32_t hpmetas = {};   // total number of times meta'd hps
    uint32_t manametas = {}; // total number of times meta'd mana
    uint32_t movemetas = {}; // total number of times meta'd moves
    uint32_t acmetas = {};   // total number of times meta'd ac
    int32_t agemetas = {};   // number of years age has been meta'd

    int16_t hit_regen = {};  // modifier to hp regen
    int16_t mana_regen = {}; // modifier to mana regen
    int16_t move_regen = {}; // modifier to move regen
    int16_t ki_regen = {};   // modifier to ki regen

    int16_t melee_mitigation = {}; // modifies melee damage
    int16_t spell_mitigation = {}; // modified spell damage
    int16_t song_mitigation = {};  // modifies song damage
    int16_t spell_reflect = {};

    intptr_t clan = {}; /* Clan the char is in */

    int16_t armor = {};   // Armor class
    int16_t hitroll = {}; // Any bonus or penalty to the hit roll
    int16_t damroll = {}; // Any bonus or penalty to the damage roll

    int16_t glow_factor = {}; // Amount that the character glows

    obj_data *beacon = nullptr; /* pointer to my beacon */

    std::vector<struct songInfo> songs = {}; // Song list
                                             //     int16_t song_timer = {};       /* status for songs being sung */
                                             //     int16_t song_number = {};      /* number of song being sung */
                                             //     char * song_data = {};        /* args for the songs */

    struct obj_data *equipment[MAX_WEAR] = {}; // Equipment List

    skill_list_t skills = {};                 // Skills List
    struct affected_type *affected = nullptr; // Affected by list
    struct obj_data *carrying = nullptr;      // Inventory List

    int16_t poison_amount = {}; // How much poison damage I'm taking every few seconds

    int16_t carry_weight = {}; // Carried weight
    int16_t carry_items = {};  // Number of items carried

    char *hunting = {}; // Name of "track" target
    char *ambush = {};  // Name of "ambush" target

    char_data *guarding = {};     // Pointer to who I am guarding
    follow_type *guarded_by = {}; // List of people guarding me

    uint32_t affected_by[AFF_MAX / ASIZE + 1] = {}; // Quick reference bitvector for spell affects
    uint32_t combat = {};                           // Bitvector for combat related flags (bash, stun, shock)
    uint32_t misc = {};                             // Bitvector for IS_MOB/logs/channels.  So possessed mobs can channel

    char_data *fighting = {};      /* Opponent     */
    char_data *next = {};          /* Next anywhere in game */
    char_data *next_in_room = {};  /* Next in room */
    char_data *next_fighting = {}; /* Next fighting */
    obj_data *altar = {};
    struct follow_type *followers = {}; /* List of followers */
    char_data *master = {};             /* Who is char following? */
    char *group_name = {};              /* Name of group */

    int32_t timer = {};           // Timer for update
    int32_t shotsthisround = {};  // Arrows fired this round
    int32_t spellcraftglyph = {}; // Used for spellcraft glyphs
    bool changeLeadBonus = {};
    int32_t curLeadBonus = {};
    int cRooms = {}; // number of rooms consecrated/desecrated

    // TODO - see if we can move the "wait" timer from desc to char
    // since we need something to lag mobs too

    int32_t deaths = {}; /* deaths is reused for mobs as a
                     timer to check for WAIT_STATE */

    int cID = {}; // character ID

    struct timer_data *timerAttached = {};
    struct tempvariable *tempVariable = {};
    int spelldamage = {};
#ifdef USE_SQL
    int player_id = {};
#endif
    int spec = {};

    struct room_direction_data *brace_at, *brace_exit; // exits affected by brace
    void tell_history(char_data *sender, string message);
    void gtell_history(char_data *sender, string message);
    time_t first_damage = {};
    uint64_t damage_done = {};
    uint64_t damages = {};
    time_t last_damage = {};
    uint64_t damage_per_second = {};
    void setPOSFighting();
    int32_t getHP(void);
    void setHP(int hp, char_data *causer = nullptr);
    void addHP(int hp, char_data *causer = nullptr);
    void removeHP(int dam, char_data *causer = nullptr);
    void fillHP(void);
    void fillHPLimit(void);
    void send(string);
    command_return_t tell(char_data *, string);
    void sendRaw(string);
    vector<char_data *> getFollowers(void);
    void setPlayerLastMob(u_int64_t mobvnum);

    void swapSkill(skill_t oldSkill, skill_t newSkill);
    void setSkillMin(skill_t skill, int value);
    char_skill_data &getSkill(skill_t skill);
    void setSkill(skill_t, int value = 0);
    void upSkill(skill_t skillnum, int learned = 1);
};

class communication
{
public:
    communication(char_data *ch, string message);
    string sender;
    bool sender_ispc;
    string message;
    time_t timestamp;
};

// This structure is written to the disk.  DO NOT MODIFY THIS STRUCTURE
// There is a method in save.C for adding additional items to the pfile
// Check there if you need to add something
// This structure contains everything that would be serialized for both
// a 'saved' mob, and for a player
// Note, any "strings" are done afterwards in the functions.  Since these
// are variable length, we can't do them with a single write

// This structure was created as a replacement for char_file_u4 so that it
// would be portable between 32-bit and 64-bit code unlike char_file_u4.
struct char_file_u4
{
    char_file_u4();
    int8_t sex = {};     /* Sex */
    int8_t c_class = {}; /* Class */
    int8_t race = {};    /* Race */
    int8_t level = {};   /* Level */

    int8_t raw_str = {};
    int8_t raw_intel = {};
    int8_t raw_wis = {};
    int8_t raw_dex = {};

    int8_t raw_con = {};
    int8_t conditions[3] = {};

    uint8_t weight = {};
    uint8_t height = {};
    int16_t hometown = {};

    uint32_t gold = {};
    uint32_t plat = {};
    int64_t exp = {};
    uint32_t immune = {};
    uint32_t resist = {};
    uint32_t suscept = {};

    int32_t mana = {};     // current
    int32_t raw_mana = {}; // max without eq/stat bonuses
    int32_t hit = {};
    int32_t raw_hit = {};
    int32_t move = {};
    int32_t raw_move = {};
    int32_t ki = {};
    int32_t raw_ki = {};

    int16_t alignment = {};
    int16_t unused1 = {};

    uint32_t hpmetas = {}; // Used by familiars too... why not.
    uint32_t manametas = {};
    uint32_t movemetas = {};

    int16_t armor = {}; // have to save these since mobs have different bases
    int16_t hitroll = {};

    int16_t damroll = {};
    int16_t unused2 = {};

    int32_t afected_by = {};
    int32_t afected_by2 = {};
    uint32_t misc = {}; // channel flags

    int16_t clan = {};
    int16_t unused3 = {};
    int32_t load_room = {}; // Which room to place char in

    uint32_t acmetas = {};
    int32_t agemetas = {};
    int32_t extra_ints[3] = {}; // available just in case
} __attribute__((packed));

struct char_file_u
{
    int8_t sex;     /* Sex */
    int8_t c_class; /* Class */
    int8_t race;    /* Race */
    int8_t level;   /* Level */

    int8_t raw_str;
    int8_t raw_intel;
    int8_t raw_wis;
    int8_t raw_dex;
    int8_t raw_con;
    int8_t conditions[3];

    uint8_t weight;
    uint8_t height;

    int16_t hometown;
    uint32_t gold;
    uint32_t plat;
    int64_t exp;
    uint32_t immune;
    uint32_t resist;
    uint32_t suscept;

    int32_t mana;     // current
    int32_t raw_mana; // max without eq/stat bonuses
    int32_t hit;
    int32_t raw_hit;
    int32_t move;
    int32_t raw_move;
    int32_t ki;
    int32_t raw_ki;

    int16_t alignment;
    uint32_t hpmetas; // Used by familiars too... why not.
    uint32_t manametas;
    uint32_t movemetas;

    int16_t armor; // have to save these since mobs have different bases
    int16_t hitroll;
    int16_t damroll;
    int32_t afected_by;
    int32_t afected_by2;
    uint32_t misc; // channel flags

    int16_t clan;
    int32_t load_room; // Which room to place char in

    uint32_t acmetas;
    int32_t agemetas;
    int32_t extra_ints[3]; // available just in case
};

struct profession
{
    std::string name;
    std::string Name;
    uint16_t skillno;
    uint8_t c_class;
};

void clear_hunt(void *arg1, void *arg2, void *arg3);
void clear_hunt(char *arg1, char_data *arg2, void *arg3);
void prepare_character_for_sixty(char_data *ch);
bool isPaused(char_data *mob);

#endif
