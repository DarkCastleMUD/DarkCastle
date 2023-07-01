
#ifndef CHARACTER_H_
#define CHARACTER_H_
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
using namespace std;

#include <QString>
#include <QMap>

class Character;
class Connection;
#include "typedefs.h"
#include "utility.h"
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
#include "Entity.h"
#include "player.h"

bool on_forbidden_name_list(const char *name);
QString color_to_code(QString color);

struct strcasecmp_compare
{
    bool operator()(const std::string &l, const std::string &r) const
    {
        return strcasecmp(l.c_str(), r.c_str()) < 0;
    }
};

struct ignore_entry
{
    bool ignore;
    uint64_t ignored_count;
};

typedef std::map<std::string, ignore_entry, strcasecmp_compare> ignoring_t;

class communication;
typedef std::queue<communication> history_t;

typedef QString player_config_key_t;
typedef QString player_config_value_t;
typedef QMap<player_config_key_t, player_config_value_t> player_config_t;
class PlayerConfig : public QObject
{
    Q_OBJECT
public:
    explicit PlayerConfig(QObject *parent = nullptr);
    player_config_t::iterator begin();
    player_config_t::iterator end();
    player_config_t::const_iterator constBegin() const;
    player_config_t::const_iterator constEnd() const;
    player_config_value_t value(const player_config_key_t &key, const player_config_value_t &defaultValue = player_config_value_t()) const;
    player_config_key_t key(const player_config_value_t &value, const player_config_key_t &defaultKey = player_config_key_t()) const;
    player_config_t::iterator find(const player_config_key_t &k);
    player_config_t::iterator insert(const player_config_key_t &key, const player_config_value_t &value);
    player_config_t &getQMap(void);

private:
    player_config_t config;
};

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
    Character *ch;
    Object *obj;
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
    level_t levelavailable; // what level class can get it
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
    Character *origin = {};
    Character *victim = {};
};

struct follow_type
{
    Character *follower;
    struct follow_type *next;
};

// DO NOT change most of these types without checking the save files
// first, or you will probably end up corrupting all the pfiles
class Player
{
public:
    explicit Player(void) {}
    Player(Player &player) {}
    void duplicate(Player &player) {}

    QString getJoining(void);
    void setJoining(QString list);
    void toggleJoining(QString key);

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
    Object *skillchange = {}; /* Skill changing equipment. */

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
    Character *golem = {}; // CURRENT golem.
    bool hide[MAX_HIDE] = {};
    Character *hiding_from[MAX_HIDE] = {};
    std::queue<string> *away_msgs = {};
    history_t *tell_history = {};
    history_t *gtell_history = {};
    joining_t joining = {};
    uint32_t quest_points = {};
    int16_t quest_current[QUEST_MAX] = {};
    uint32_t quest_current_ticksleft[QUEST_MAX] = {};
    int16_t quest_cancel[QUEST_CANCEL] = {};
    uint32_t quest_complete[QUEST_TOTAL / ASIZE + 1] = {};
    char *last_prompt = {};
    std::multimap<int, std::pair<timeval, timeval>> *lastseen = {};
    uint8_t profession = {};
    bool multi = {};
    PlayerConfig *config = {};

private:
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

class Mobile : public Entity
{
public:
    explicit Mobile(void) {}
    explicit Mobile(const Mobile &mobile) { duplicate(mobile); }
    void duplicate(const Mobile &mobile) {}

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

    void setObject(Object *);
    Object *getObject(void);
    bool isObject(void);

private:
    Object *object = {};
};

enum sex_t
{
    NEUTRAL = 0,
    MALE = 1,
    FEMALE = 2
};

template <typename T>
inline T *duplicateClass(T *source)
{
    if (source != nullptr)
    {
        T *destination = new T(*source);
        return destination;
    }
    return nullptr;
}

inline char *duplicateCString(const char *source)
{
    if (source == nullptr)
    {
        return strdup("");
    }
    else
    {
        return strdup(source);
    }
}

// Character, Character
// This contains all memory items for a player/mob
// All non-specific data is held in this structure
// PC/MOB specific data are held in the appropriate pointed-to structs
class Character : public QObject
{
    Q_OBJECT
public:
    static constexpr uint64_t MIN_NAME_SIZE = 3;
    static constexpr uint64_t MAX_NAME_SIZE = 12;
    static const QList<int> wear_to_item_wear;
    static bool validateName(QString name);

    explicit Character(QObject *parent = nullptr) : QObject(parent) {}
    explicit Character(const Character &character, QObject *parent = nullptr) : QObject(parent) { duplicate(character); }
    void duplicate(const Character &character);
    void clear(void);

    class Mobile *mobile = nullptr;
    inline Mobile *getMobile(void) { return mobile; }

    class Player *player = nullptr;
    inline Player *getPlayer(void) { return player; }

    class Object *object = nullptr;
    inline Object *getObject(void) { return object; }

    class Connection *desc = nullptr; // nullptr normally for mobs
    inline Connection *getConnection(void) { return desc; }

    char *name = nullptr; // Keyword 'kill X'
    inline QString getName(void) const { return name; }
    void setName(QString n) { name = strdup(n.toStdString().c_str()); }

    char *short_desc = nullptr; // Action 'X hits you.'
    inline char *getShortDescriptionC(void) { return short_desc; }
    inline QString getShortDescription(void) { return short_desc; }
    void setShortDescription(QString sd) { short_desc = strdup(sd.toStdString().c_str()); }

    char *long_desc = nullptr; // For 'look room'
    inline char *getLongDescriptionC(void) { return long_desc; }
    inline QString getLongDescription(void) { return long_desc; }
    void setLongDescription(QString ld) { long_desc = strdup(ld.toStdString().c_str()); }

    char *description = nullptr; // For 'look mob'
    inline char *getDescriptionC(void) { return description; }
    inline QString getDescription(void) { return description; }
    void setDescription(QString d) { description = strdup(d.toStdString().c_str()); }

    char *title = nullptr;
    inline char *getTitleC(void) { return title; }
    inline QString getTitle(void) { return title; }
    void setTitle(QString t) { title = strdup(t.toStdString().c_str()); }

    void setNeutral(void) { data_.sex_ = sex_t::NEUTRAL; }
    void setMale(void) { data_.sex_ = sex_t::MALE; }
    void setFemale(void) { data_.sex_ = sex_t::FEMALE; }
    sex_t getSex(void) { return data_.sex_; }
    void setSex(sex_t s) { data_.sex_ = s; }
    sex_t &getSexReference(void);
    bool isMale(void) { return data_.sex_ == sex_t::MALE; }
    bool isFemale(void) { return data_.sex_ == sex_t::FEMALE; }
    bool isNeutral(void) { return data_.sex_ == sex_t::NEUTRAL; }

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

    uint32_t plat = {}; /* Platinum                                */
    int64_t exp = {};   /* The experience of the player            */
    room_t in_room = {};

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

    Object *beacon = nullptr; /* pointer to my beacon */

    std::vector<struct songInfo> songs = {}; // Song list
                                             //     int16_t song_timer = {};       /* status for songs being sung */
                                             //     int16_t song_number = {};      /* number of song being sung */
                                             //     char * song_data = {};        /* args for the songs */

    class Object *equipment[MAX_WEAR] = {}; // Equipment List

    skill_list_t skills = {};                 // Skills List
    struct affected_type *affected = nullptr; // Affected by list
    class Object *carrying = nullptr;         // Inventory List

    int16_t poison_amount = {}; // How much poison damage I'm taking every few seconds

    int16_t carry_weight = {}; // Carried weight
    int16_t carry_items = {};  // Number of items carried

    char *hunting = {}; // Name of "track" target
    char *ambush = {};  // Name of "ambush" target

    Character *guarding = {};     // Pointer to who I am guarding
    follow_type *guarded_by = {}; // List of people guarding me

    uint32_t affected_by[AFF_MAX / ASIZE + 1] = {}; // Quick reference bitvector for spell affects
    uint32_t combat = {};                           // Bitvector for combat related flags (bash, stun, shock)

    Character *fighting = {};      /* Opponent     */
    Character *next = {};          /* Next anywhere in game */
    Character *next_in_room = {};  /* Next in room */
    Character *next_fighting = {}; /* Next fighting */
    Object *altar = {};
    struct follow_type *followers = {}; /* List of followers */
    Character *master = {};             /* Who is char following? */
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
    time_t first_damage = {};
    uint64_t damage_done = {};
    uint64_t damages = {};
    time_t last_damage = {};
    uint64_t damage_per_second = {};
    struct room_direction_data *brace_at = {}, *brace_exit = {}; // exits affected by brace

    void tell_history(Character *sender, string message);
    void gtell_history(Character *sender, string message);
    void setPOSFighting();
    int32_t getHP(void);
    void setHP(int hp, Character *causer = nullptr);
    void addHP(int hp, Character *causer = nullptr);
    void removeHP(int dam, Character *causer = nullptr);
    void fillHP(void);
    void fillHPLimit(void);
    void send(const char *buffer);
    void send(string buffer);
    void send(QString buffer);
    command_return_t tell(Character *, string);
    void sendRaw(string);
    vector<Character *> getFollowers(void);
    void setPlayerLastMob(u_int64_t mobvnum);

    void swapSkill(skill_t oldSkill, skill_t newSkill);
    void setSkillMin(skill_t skill, int value);
    char_skill_data &getSkill(skill_t skill);
    void setSkill(skill_t, int value = 0);
    void upSkill(skill_t skillnum, int learned = 1);

    QString getSetting(QString key, QString defaultValue = QString());
    QString getSettingAsColor(QString key, QString defaultValue = QString());

    command_return_t do_clanarea(QStringList arguments, int cmd);
    command_return_t do_config(QStringList arguments, int cmd);
    command_return_t do_experience(QStringList arguments, int cmd);
    command_return_t do_split(QStringList arguments, int cmd);
    command_return_t do_zsave(QStringList arguments, int cmd);
    command_return_t do_goto(QStringList arguments, int cmd);
    command_return_t do_save(QStringList arguments, int cmd);
    command_return_t do_search(QStringList arguments, int cmd);
    command_return_t do_identify(QStringList arguments, int cmd);
    command_return_t do_recall(QStringList arguments, int cmd);
    command_return_t do_cdeposit(QStringList arguments, int cmd);
    command_return_t do_drink(QStringList arguments, int cmd);
    command_return_t do_eat(QStringList arguments, int cmd);
    command_return_t do_string(QStringList arguments, int cmd);
    command_return_t generic_command(QStringList arguments, int cmd);
    command_return_t do_sockets(QStringList arguments, int cmd);
    command_return_t save(int cmd = CMD_DEFAULT);
    Character *getVisiblePlayer(QString name);
    Character *getVisibleCharacter(QString name);
    Object *getVisibleObject(QString name);
    QString getStatDiff(int base, int random, bool swapcolors = false);
    bool isMortal(void);
    bool isImmortal(void);
    bool isImplementer(void);
    inline bool isPlayer(void) { return isMobile() == false && player != nullptr; }
    bool isMobile(void);
    uint64_t getGold(void);
    uint64_t &getGoldReference(void);
    void setGold(uint64_t gold);
    bool addGold(uint64_t gold);
    bool multiplyGold(double mult);
    bool removeGold(uint64_t gold);
    int store_to_char_variable_data(FILE *fpsave);
    int char_to_store_variable_data(FILE *fpsave);
    bool load_charmie_equipment(QString name, bool previous = false);

    uint32_t getMisc(void) const { return data_.misc_; }
    uint32_t &getMiscReference(void) { return data_.misc_; }
    void setMisc(uint32_t misc) { data_.misc_ = misc; }

private:
    struct Character_data_t
    {
        sex_t sex_ = {};
        uint64_t gold_ = {}; /* Money carried */
        uint32_t misc_ = {}; // Bitvector for IS_MOB/logs/channels.  So possessed mobs can channel
    } data_;
};

class communication
{
public:
    communication(Character *ch, string message);
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

// This structure was created as a replacement for char_file_u so that it
// would be portable between 32-bit and 64-bit code.
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
void clear_hunt(char *arg1, Character *arg2, void *arg3);
void prepare_character_for_sixty(Character *ch);
bool isPaused(Character *mob);

#endif
