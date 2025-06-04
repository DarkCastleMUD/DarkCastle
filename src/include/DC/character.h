
#ifndef CHARACTER_H_
#define CHARACTER_H_
/******************************************************************************
| $Id: character.h,v 1.85 2014/07/26 23:21:23 jhhudso Exp $
| Description: This file contains the header information for the character
|   class implementation.
*/
#include <ctime>
#include <cstdint>
#include <strings.h>

#include <queue>
#include <map>
#include <string>
#include <vector>
#include <string>
#include <algorithm>
#include <set>

#include <QString>
#include <QMap>

class Character;
#include "DC/DC.h"
#include "DC/affect.h"   /* MAX_AFFECTS, etc.. */
#include "DC/alias.h"    /* struct char_player_alias, MAX_ALIASES, etc.. */
#include "DC/structs.h"  /* uint8_t, uint8_t, int16_t, etc.. */
#include "DC/timeinfo.h" // time data, etc..
#include "DC/event.h"    // eventBrief
#include "DC/isr.h"      // SAVE_TYPE_MAX
#include "DC/mobile.h"
#include "DC/sing.h"
#include "DC/quest.h"
#include "DC/interp.h"
#include "DC/utility.h"
#include "DC/Zone.h"
#include "DC/room.h"
#include "DC/types.h"

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
typedef QQueue<communication> history_t;

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

#define SEX_NEUTRAL Character::sex_t::NEUTRAL
#define SEX_MALE Character::sex_t::MALE
#define SEX_FEMALE Character::sex_t::FEMALE
#define MAX_HIDE 10
#define CHAMPION_ITEM 45

// * ------- Begin MOBProg stuff ----------- *

typedef int16_t skill_t;
typedef std::map<skill_t, struct char_skill_data> skill_list_t;

struct tempvariable
{
    struct tempvariable *next{};
    QString name;
    QString data;
    int16_t save{}; // save or not
};

struct mob_prog_act_list
{
    mob_prog_act_list *next{};
    char *buf{};
    Character *ch{};
    Object *obj{};
    void *vo{};
};

class MobProgram
{
public:
    QSharedPointer<class MobProgram> next{};
    int type{};
    char *arglist{};
    char *comlist{};
};
char *mprog_type_to_name(int type);
void write_mprog_recur(auto &fl, QSharedPointer<MobProgram> mprg, bool mob)
{
    if (mprg->next)
    {
        write_mprog_recur(fl, mprg->next, mob);
    }

    if (mob)
    {
        fl << ">" << mprog_type_to_name(mprg->type) << " ";
    }
    else
    {
        fl << "\\" << mprog_type_to_name(mprg->type) << " ";
    }

    if (mprg->arglist)
    {
        string_to_file(fl, QString(mprg->arglist));
    }
    else
    {
        string_to_file(fl, QStringLiteral("Saved During Edit"));
    }

    if (mprg->comlist)
    {
        string_to_file(fl, QString(mprg->comlist));
    }
    else
    {
        string_to_file(fl, QStringLiteral("Saved During Edit"));
    }
}
void write_mprog_recur(FILE *fl, QSharedPointer<class MobProgram> mprg, bool mob);
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
    std::string caster = {};
    struct affected_type *next = {};
    Character *origin = {};
    Character *victim = {};
};

struct follow_type
{
    Character *follower;
    struct follow_type *next;
};
typedef command_return_t (Character::*command_gen3_t)(QStringList arguments, int cmd);
class Toggle
{
public:
    Toggle(void) = default;
    Toggle(QString name, uint64_t shift, command_gen3_t function, uint64_t dependency_shift = UINT64_MAX, QString on_message = "$B$2on$R", QString off_message = "$B$4off$R");
    bool isValid(void) { return valid_; }

    QString name_;
    bool valid_ = false;
    uint64_t shift_{};
    uint64_t dependency_shift_{};
    uint64_t value_{};
    QString on_message_;
    QString off_message_;
    command_return_t (Character::*function_)(QStringList arguments, int cmd);
};

// DO NOT change most of these types without checking the save files
// first, or you will probably end up corrupting all the pfiles
class Player
{
public:
    /************************************************************************
    | Player vectors
    | Character->player->toggles
    */
    QString last_site;        /* Last login from.. */
    QString poofin;           /* poofin message */
    QString poofout;          /* poofout message */
    char *prompt = {};        /* Sadus' disguise.. unused */
    Object *skillchange = {}; /* Skill changing equipment. */
    Character *golem = {};    // CURRENT golem.

    constexpr static uint32_t PLR_BRIEF = 1U;
    constexpr static uint32_t PLR_BRIEF_BIT = 0;
    constexpr static uint32_t PLR_COMPACT = 1U << 1;
    constexpr static uint32_t PLR_COMPACT_BIT = 1;
    constexpr static uint32_t PLR_DONTSET = 1U << 2;
    constexpr static uint32_t PLR_DONTSET_BIT = 2;
    constexpr static uint32_t PLR_DONOTUSE = 1U << 3;
    constexpr static uint32_t PLR_DONOTUSE_BIT = 3;
    constexpr static uint32_t PLR_NOHASSLE = 1U << 4;
    constexpr static uint32_t PLR_NOHASSLE_BIT = 4;
    constexpr static uint32_t PLR_SUMMONABLE = 1U << 5;
    constexpr static uint32_t PLR_SUMMONABLE_BIT = 5;
    constexpr static uint32_t PLR_WIMPY = 1U << 6;
    constexpr static uint32_t PLR_WIMPY_BIT = 6;
    constexpr static uint32_t PLR_ANSI = 1U << 7;
    constexpr static uint32_t PLR_ANSI_BIT = 7;
    constexpr static uint32_t PLR_VT100 = 1U << 8;
    constexpr static uint32_t PLR_VT100_BIT = 8;
    constexpr static uint32_t PLR_ONEWAY = 1U << 9;
    constexpr static uint32_t PLR_ONEWAY_BIT = 9;
    constexpr static uint32_t PLR_DISGUISED = 1U << 10;
    constexpr static uint32_t PLR_DISGUISED_BIT = 10;
    constexpr static uint32_t PLR_UNUSED = 1U << 11;
    constexpr static uint32_t PLR_UNUSED_BIT = 11;
    constexpr static uint32_t PLR_PAGER = 1U << 12;
    constexpr static uint32_t PLR_PAGER_BIT = 12;
    constexpr static uint32_t PLR_BEEP = 1U << 13;
    constexpr static uint32_t PLR_BEEP_BIT = 13;
    constexpr static uint32_t PLR_BARD_SONG = 1U << 14;
    constexpr static uint32_t PLR_BARD_SONG_BIT = 14;
    constexpr static uint32_t PLR_ANONYMOUS = 1U << 15;
    constexpr static uint32_t PLR_ANONYMOUS_BIT = 15;
    constexpr static uint32_t PLR_AUTOEAT = 1U << 16;
    constexpr static uint32_t PLR_AUTOEAT_BIT = 16;
    constexpr static uint32_t PLR_LFG = 1U << 17;
    constexpr static uint32_t PLR_LFG_BIT = 17;
    constexpr static uint32_t PLR_CHARMIEJOIN = 1U << 18;
    constexpr static uint32_t PLR_CHARMIEJOIN_BIT = 18;
    constexpr static uint32_t PLR_NOTAX = 1U << 19;
    constexpr static uint32_t PLR_NOTAX_BIT = 19;
    constexpr static uint32_t PLR_GUIDE = 1U << 20;
    constexpr static uint32_t PLR_GUIDE_BIT = 20;
    constexpr static uint32_t PLR_GUIDE_TOG = 1U << 21;
    constexpr static uint32_t PLR_GUIDE_TOG_BIT = 21;
    constexpr static uint32_t PLR_NEWS = 1U << 22;
    constexpr static uint32_t PLR_NEWS_BIT = 22;
    constexpr static uint32_t PLR_50PLUS = 1U << 23;
    constexpr static uint32_t PLR_50PLUS_BIT = 23;
    constexpr static uint32_t PLR_ASCII = 1U << 24;
    constexpr static uint32_t PLR_ASCII_BIT = 24;
    constexpr static uint32_t PLR_DAMAGE = 1U << 25;
    constexpr static uint32_t PLR_DAMAGE_BIT = 25;
    constexpr static uint32_t PLR_CLS_TREE_A = 1U << 26;
    constexpr static uint32_t PLR_CLS_TREE_A_BIT = 26;
    constexpr static uint32_t PLR_CLS_TREE_B = 1U << 27;
    constexpr static uint32_t PLR_CLS_TREE_B_BIT = 27;
    constexpr static uint32_t PLR_CLS_TREE_C = 1U << 28; // might happen one day
    constexpr static uint32_t PLR_CLS_TREE_C_BIT = 28;
    constexpr static uint32_t PLR_EDITOR_WEB = 1U << 29;
    constexpr static uint32_t PLR_EDITOR_WEB_BIT = 29;
    constexpr static uint32_t PLR_REMORTED = 1U << 30;
    constexpr static uint32_t PLR_REMORTED_BIT = 30;
    constexpr static uint32_t PLR_NODUPEKEYS = 1U << 31;
    constexpr static uint32_t PLR_NODUPEKEYS_BIT = 31;
    static const QList<Toggle> togglables;
    static const QStringList toggle_txt;

    char pwd[PASSWORD_LEN + 1] = {};
    ignoring_t ignoring = {}; /* List of ignored names */

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

    int16_t buildLowVnum = {}, buildHighVnum = {};
    int16_t buildMLowVnum = {}, buildMHighVnum = {};
    int16_t buildOLowVnum = {}, buildOHighVnum = {};

    vnum_t last_mob_edit = {}; // vnum of last mob edited
    vnum_t last_obj_vnum = {}; // vnum of last obj edited

    QString last_tell = {};      /* last person who told           */
    int16_t last_mess_read = {}; /* for reading messages */

    // TODO: these 3 need to become PLR toggles
    bool holyLite = {};  // Holy lite mode
    bool stealth = {};   // If on, you are more stealth then norm. (god)
    bool incognito = {}; // invis imms will be seen by people in same room

    bool possesing = {};  /*  is the person possessing? */
    bool unjoinable = {}; // Do NOT autojoin
    bool hide[MAX_HIDE] = {};
    Character *hiding_from[MAX_HIDE] = {};
    QQueue<QString> away_msgs = {};
    history_t tell_history;
    history_t gtell_history;
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

    QString getJoining(void);
    void setJoining(QString list);
    void toggleJoining(QString key);
    void save_char_aliases(FILE *fpsave);
    QString perform_alias(QString orig);
    void save(FILE *fpsave, time_data tmpage);
    bool read(FILE *fpsave, Character *ch, QString filename);

    aliases_t aliases_; /* Aliases */
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

class Mobile
{
public:
    int32_t nr = {};
    position_t default_pos = {};                 // Default position for NPC
    int8_t last_direction = {};                  // Last direction the mobile went in
    uint32_t attack_type = {};                   // Bitvector of damage type for bare-handed combat
    uint32_t actflags[ACT_MAX / ASIZE + 1] = {}; // flags for NPC behavior

    int16_t damnodice = {};   // The number of damage dice's
    int16_t damsizedice = {}; // The size of the damage dice's

    char *fears = {};   /* will flee from ths person on sight     */
    QString hated = {}; /* List of PC's I hate */

    mob_prog_act_list *mpact = {}; // list of MOBProgs
    int16_t mpactnum = {};         // num
    int32_t last_room = {};        // Room rnum the mob was last in. Used
                                   // For !magic,!track changing flags.
    struct threat_struct *threat = {};
    QSharedPointer<ResetCommand> reset = {};
    mob_flag_data mob_flags = {}; /* Mobile information               */
    bool paused = {};

    void setObject(Object *);
    Object *getObject(void);
    bool isObject(void);
    void save(FILE *fpsave);
    void read(FILE *fpsave);

private:
    Object *object = {};
};

typedef uint_fast8_t class_t;
typedef int32_t move_t;
typedef QSharedPointer<Mobile> mobdata_t;
// Character, Character
// This contains all memory items for a player/mob
// All non-specific data is held in this structure
// PC/MOB specific data are held in the appropriate pointed-to structs
class Character : public Entity
{
    Q_GADGET
public:
    enum Type
    {
        Undefined,
        Player,
        NPC,
        ObjectProgram
    };
    Q_ENUM(Type)
    void setType(const Type type);
    auto getType(void) const -> Type;

    enum sex_t : int8_t
    {
        NEUTRAL = 0,
        MALE = 1,
        FEMALE = 2
    };
    enum class PromptVariableType
    {
        Legacy,
        Advanced
    };
    static constexpr qsizetype MIN_NAME_SIZE = 3;
    static constexpr qsizetype MAX_NAME_SIZE = 12;
    static const QList<int> wear_to_item_wear;
    static bool validateName(QString name);

    mobdata_t mobdata = {};
    class Player *player = nullptr;
    class Object *objdata = nullptr;

    class Connection *desc = nullptr; // nullptr normally for mobs

    const char *getNameC(void) const;

    inline QString getName(void)
    {
        return name_;
    }
    void setName(QString new_name)
    {
        name_ = new_name;
    }

    char *short_desc = nullptr;  // Action 'X hits you.'
    char *long_desc = nullptr;   // For 'look room'
    char *description = nullptr; // For 'look mob'
    char *title = nullptr;

    sex_t sex = {};

    int8_t c_class = {};
    auto getClass(void) const
    {
        return c_class;
    }
    void setClass(auto new_class)
    {
        c_class = new_class;
    }
    QString class_to_string(class_t class_nr) const
    {
        return Character::class_names.value(class_nr);
    }
    QString getClassName(void) const
    {
        return class_to_string(c_class);
    }

    int8_t race = {};
    auto getRace(void) const
    {
        return race;
    }
    void setRace(auto new_race)
    {
        race = new_race;
    }
    QString getRaceName(void) const
    {
        return race_names.value(race);
    }

    level_t getLevel(void) const;
    level_t *getLevelPtr(void)
    {
        return &level_;
    }
    void setLevel(level_t level);
    void incrementLevel(level_t level_change = 1)
    {
        if (level_ <= INT64_MAX - level_change)
        {
            level_ += level_change;
        }
    }
    void decrementLevel(level_t level_change = 1)
    {
        if (level_ >= level_change)
        {
            level_ -= level_change;
        }
        else
        {
            level_ = 0;
        }
    }

    bool isDead(void) const
    {
        return position_ == position_t::DEAD;
    }
    void setDead(void)
    {
        position_ = position_t::DEAD;
    }

    bool isStunned(void) const
    {
        return position_ == position_t::STUNNED;
    }
    void setStunned(void)
    {
        position_ = position_t::STUNNED;
    }

    bool isSleeping(void) const
    {
        return position_ == position_t::SLEEPING;
    }
    void setSleeping(void)
    {
        position_ = position_t::SLEEPING;
    }

    bool isResting(void) const
    {
        return position_ == position_t::RESTING;
    }
    void setResting(void)
    {
        position_ = position_t::RESTING;
    }

    bool isSitting(void) const
    {
        return position_ == position_t::SITTING;
    }
    void setSitting(void)
    {
        position_ = position_t::SITTING;
    }

    bool isFighting(void) const
    {
        return position_ == position_t::FIGHTING;
    }
    void setFighting(void)
    {
        position_ = position_t::FIGHTING;
    }

    bool isStanding(void) const
    {
        return position_ == position_t::STANDING;
    }
    void setStanding(void)
    {
        position_ = position_t::STANDING;
    }

    void setPosition(position_t position)
    {
        position_ = position;
    }
    position_t getPosition(void)
    {
        return position_;
    }
    position_t *getPositionPtr(void)
    {
        return &position_;
    }

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

    uint32_t plat = {};                    /* Platinum                                */
    int64_t exp = {};                      /* The experience of the player            */
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

    move_t move_limit(void);
    move_t getMove(void)
    {
        return move_;
    }
    move_t *getMovePtr(void)
    {
        return &move_;
    }
    void setMove(move_t new_move)
    {
        move_ = new_move;
    }
    bool incrementMove(move_t move_change = 1)
    {
        if (move_ <= INT64_MAX - move_change)
        {
            move_ += move_change;

            if (move_ > move_limit())
            {
                move_ = move_limit();
            }

            return true;
        }
        else
        {
            return false;
        }
    }
    bool decrementMove(move_t move_change = 1, QString message = QStringLiteral("You're too out of breath!"))
    {
        if (move_ >= move_change)
        {
            move_ -= move_change;
            return true;
        }
        else
        {
            move_ = 0;
            if (!message.isEmpty())
            {
                sendln(message);
            }
        }
        return false;
    }

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

    clan_t clan = {}; /* Clan the char is in */

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

    QString hunting = {}; // Name of "track" target
    QString ambush = {};  // Name of "ambush" target

    Character *guarding = {};     // Pointer to who I am guarding
    follow_type *guarded_by = {}; // List of people guarding me

    uint32_t affected_by[AFF_MAX / ASIZE + 1] = {}; // Quick reference bitvector for spell affects
    uint32_t combat = {};                           // Bitvector for combat related flags (bash, stun, shock)
    uint32_t misc = {};                             // Bitvector for logs/channels.  So possessed mobs can channel

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

    bool getDebug(void) const
    {
        return debug_;
    }
    void setDebug(bool state)
    {
        debug_ = state;
    }

    struct room_direction_data *brace_at{}, *brace_exit{}; // exits affected by brace
    time_t first_damage = {};
    uint64_t damage_done = {};
    uint64_t damages = {};
    time_t last_damage = {};
    uint64_t damage_per_second = {};

    bool addGold(uint64_t gold);
    bool save_pc_or_mob_data(FILE *fpsave, time_data tmpage);
    void add_command_lag(int cmdnum, int lag);
    bool canPerform(const int_fast32_t &learned, QString failMessage = QString());
    int char_to_store_variable_data(FILE *fpsave);
    void display_string_list(QStringList list);
    void display_string_list(const char **list);
    bool charge_moves(int skill, double modifier = 1);
    void check_maxes(void);
    int check_charmiejoin(void);
    struct time_info_data age(void);
    void add_memory(QString victim_name, char type);
    bool can_use_command(int cmdnum);
    Object *clan_altar(void);
    void do_inate_race_abilities(void);
    void do_on_login_stuff(void);
    void check_hw(void);
    void add_to_bard_list(void);

    command_return_t check_pursuit(Character *victim, QString dircommand);
    command_return_t check_social(QString pcomm);
    command_return_t command_interpreter(QString argument, bool procced = 0);
    Object *get_object_in_equip_vis(char *arg, Object *equipment[], int *j, bool blindfighting);

    command_return_t do_clanarea(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_config(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_experience(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_split(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_zsave(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_wizhelp(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_goto(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_save(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_search(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_identify(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_recall(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_cdeposit(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t generic_command(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_sockets(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_who(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_beep_set(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_bard_song_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_brief(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_news_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_ascii_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_damage_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_charmiejoin_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_guide(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_lfg_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_notax_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_guide_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_summon_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_nodupekeys_toggle(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_compact(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_anonymous(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_ansi(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_vt100(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_wimpy(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_pager(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_autoeat(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_shutdow(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_shutdown(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_linkload(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_rename_char(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_auction(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_shout(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_test(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_tell(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_wake(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_rescue(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_rage(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_join(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_outcast(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_backstab(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_snoop(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_zap(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_track(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_hit(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_ambush(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_botcheck(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_mpsettemp(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_bestow(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    command_return_t do_alias(QStringList arguments = {}, int cmd = CMD_DEFAULT);
    auto do_arena(QStringList arguments = {}, int cmd = CMD_DEFAULT) -> command_return_t;
    auto do_arena_info(QStringList arguments) -> command_return_t;
    auto do_arena_start(QStringList arguments) -> command_return_t;
    auto do_arena_join(QStringList arguments) -> command_return_t;
    auto do_arena_cancel(QStringList arguments) -> command_return_t;
    auto do_arena_usage(QStringList arguments) -> command_return_t;

    command_return_t wake(Character *victim = nullptr);
    command_return_t oprog_command_trigger(QString command, QString arguments);
    command_return_t save(int cmd = CMD_DEFAULT);
    command_return_t special(QString arg, int cmd = CMD_DEFAULT);

    void show_obj_to_char(Object *object, int mode);
    void list_obj_to_char(Object *list, int mode, bool show);
    void list_char_to_char(Character *list, int mode);

    bool mprog_seval(QString lhs, QString opr, QString rhs);
    QString getTemp(QString name);

    QString get_random_hate(void);
    Character *getVisiblePlayer(QString name);
    Character *getVisibleCharacter(QString name);
    Object *getVisibleObject(QString name);
    QString getStatDiff(int base, int random, bool swapcolors = false);
    void tell_history(Character *sender, QString message);
    void gtell_history(Character *sender, QString message);
    void setPOSFighting();
    int32_t getHP(void);
    void setHP(int hp, Character *causer = nullptr);
    void addHP(int hp, Character *causer = nullptr);
    void removeHP(int dam, Character *causer = nullptr);
    void fillHP(void);
    void fillHPLimit(void);
    void send(const char *buffer);
    void send(std::string buffer);
    void send(QString buffer);
    void sendln(QString buffer = {})
    {
        buffer = buffer.append("\r\n");
        send(buffer);
    }
    command_return_t tell(Character *, QString);
    void sendRaw(std::string);
    std::vector<Character *> getFollowers(void);
    void setPlayerLastMob(u_int64_t mobvnum);

    void swapSkill(skill_t oldSkill, skill_t newSkill);
    void setSkillMin(skill_t skill, int value);
    char_skill_data &getSkill(skill_t skill);
    void setSkill(skill_t, int value = 0);
    void upSkill(skill_t skillnum, int learned = 1);

    QString getSetting(QString key, QString defaultValue = QString());
    QString getSettingAsColor(QString key, QString defaultValue = QString());

    bool isMortalPlayer(void) const;
    bool isImmortalPlayer(void) const;
    bool isImplementerPlayer(void) const;
    bool isPlayer(void) const;
    bool isNPC(void) const;
    bool isObjectProgram(void) const;

    uint64_t getGold(void);
    uint64_t &getGoldReference(void);
    void setGold(uint64_t gold);

    bool multiplyGold(double mult);
    bool removeGold(uint64_t gold);
    int store_to_char_variable_data(FILE *fpsave);

    bool load_charmie_equipment(QString name, bool previous = false);
    int has_skill(skill_t skill);
    affected_type *affected_by_spell(uint32_t skill);
    bool skill_success(Character *victim, int skillnum, int mod = 0);
    int skillmax(int skill, int eh);
    char charthing(int known, int skill, int maximum);
    void output_praclist(class_skill_defines *skilllist);
    int skills_guild(const char *arg, Character *owner);
    int get_stat(attribute_t stat);
    int get_stat_bonus(int stat);
    void skill_increase_check(int skill, int learned, int difficulty);
    void verify_max_stats(void);
    int get_max(int skill);
    int learn_skill(int skill, int amount, int maximum);
    class_skill_defines *get_skill_list(void);
    Character *get_char_room_vis(QString name);
    Character *get_rand_other_char_room_vis(void);
    int hit_gain(position_t position, bool improve = true);
    int hit_gain(void)
    {
        return hit_gain(getPosition(), true);
    }

    int hit_gain_lookup(void)
    {
        return hit_gain(getPosition(), false);
    }
    int mana_gain_lookup(void);
    int move_gain_lookup(int extra = 0);
    int ki_gain_lookup(void);
    static QString position_to_string(position_t position)
    {
        return position_types.value(static_cast<qint64>(position));
    }

    QString getPositionQString(void)
    {
        return position_to_string(getPosition());
    }
    Character *get_active_pc_vis(QString name);
    void swap_hate_memory(void);
    void set_hw(void);
    void roll_and_display_stats(void);
    void remove_from_bard_list(void);
    int64_t moves_exp_spent(void);
    int64_t moves_plats_spent(void);
    int64_t hps_exp_spent(void);
    int64_t hps_plats_spent(void);
    int64_t mana_exp_spent(void);
    int64_t mana_plats_spent(void);
    int meta_get_stat_exp_cost(attribute_t stat);
    int meta_get_stat_plat_cost(attribute_t targetstat);
    void meta_list_stats(void);
    quint64 meta_get_moves_exp_cost(void);
    quint64 meta_get_moves_plat_cost(int amount);
    quint64 meta_get_hps_exp_cost(void);
    quint64 meta_get_hps_plat_cost(int amount);
    quint64 meta_get_mana_exp_cost(void);
    quint64 meta_get_mana_plat_cost(int amount);
    quint64 meta_get_ki_plat_cost(void);
    int meta_get_ki_exp_cost(void);
    void undo_race_saves(void);
    bool is_race_applicable(int race);
    bool would_die(void);
    void set_heightweight(void);
    char *race_message(int race);
    int hands_are_free(int number);
    int recheck_height_wears(void);
    void heightweight(bool add);
    static const QStringList class_names;
    static const QStringList race_names;
    static const QStringList position_types;
    static const QStringList song_names;
    static constexpr quint64 PLAYER_OBJECT_THIEF = 297ULL;
    static constexpr quint64 PLAYER_GOLD_THIEF = 298ULL;
    static constexpr quint64 PLAYER_CANTQUIT = 299ULL;
    bool isPlayerObjectThief(void) { return affected_by_spell(PLAYER_OBJECT_THIEF); }
    bool isPlayerGoldThief(void) { return affected_by_spell(PLAYER_GOLD_THIEF); }
    bool isPlayerCantQuit(void) { return affected_by_spell(PLAYER_CANTQUIT); }
    bool allowColor(void);
    QString generate_prompt(void);
    QString parse_prompt_variable(QString variable, PromptVariableType type = PromptVariableType::Advanced);
    QString get_parsed_legacy_prompt_variable(QString var);
    QString calc_name(bool use_color = false);

private:
    Type type_ = Type::Undefined;
    gold_t gold_ = {}; /* Money carried */
    level_t level_ = {};
    bool debug_ = false;
    move_t move_ = {};
    QString name_; // Keyword 'kill X'
    position_t position_ = {};
};

class communication
{
public:
    communication(Character *ch, QString message);
    QString sender;
    bool sender_ispc;
    QString message;
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
    Character::sex_t sex = {}; /* Sex */
    int8_t c_class = {};       /* Class */
    int8_t race = {};          /* Race */
    int8_t level = {};         /* Level */

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

void prepare_character_for_sixty(Character *ch);
bool isPaused(Character *mob);

#endif
