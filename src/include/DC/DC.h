#pragma once
/*
 * Copyright 2017-2023 Jared H. Hudson
 * Licensed under the LGPL.
 */
#include <QtTypes>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QHttpServerRequest>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QQueue>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QSet>
#include <QString>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QThread>

#include <libssh/libssh.h>
#include <libssh/server.h>

#include <cstdlib>
#include <expected>
#include <qobject.h>
#include <qtmetamacros.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "DC/DC_global.h"

// Create RSA host key with: ssh-keygen -t rsa -f ssh_host_rsa_key
// Place resulting two files in lib/

namespace DCNS
{
  Q_NAMESPACE

  enum ObjectPosition
  {
    TAKE = 1 << 0,
    FINGER = 1 << 1,
    NECK = 1 << 2,
    BODY = 1 << 3,
    HEAD = 1 << 4,
    LEGS = 1 << 5,
    FEET = 1 << 6,
    HANDS = 1 << 7,
    ARMS = 1 << 8,
    SHIELD = 1 << 9,
    ABOUT = 1 << 10,
    WAISTE = 1 << 11,
    WRIST = 1 << 12,
    WIELD = 1 << 13,
    HOLD = 1 << 14,
    THROW = 1 << 15,
    LIGHT_SOURCE = 1 << 16,
    FACE = 1 << 17,
    EAR = 1 << 18
  };
  Q_DECLARE_FLAGS(ObjectPositions, ObjectPosition)
  Q_DECLARE_OPERATORS_FOR_FLAGS(ObjectPositions)
  Q_FLAG_NS(ObjectPositions)

  template <typename T>
  QStringList QFlagsToStrings(void)
  {
    QStringList list;
    auto metaEnum = QMetaEnum::fromType<T>();
    for (qint32 i = {}; i < metaEnum.keyCount(); ++i)
    {
      if (metaEnum.key(i))
        list.push_back(QString(metaEnum.key(i)).replace('_', '-'));
    }
    return list;
  }
  template <typename T>
  QString QFlagsToStrings(T flags)
  {
    return QMetaEnum::fromType<T>().valueToKeys(flags).replace('_', '-').replace('|', ' ');
  }

}
using namespace DCNS;

enum class cmd_t
{
  UNDEFINED,   // 0
  NORTH,       // 1
  EAST,        // 2
  SOUTH,       // 3
  WEST,        // 4
  UP,          // 5
  DOWN,        // 6
  BELLOW,      // 8
  DEFAULT,     // 9
  TRACK,       // 10
  PALM,        // 10
  SAY,         // 11
  LOOK,        // 12
  BACKSTAB,    // 13
  SBS,         // 14
  ORCHESTRATE, // 15
  REPLY,       // 16
  WHISPER,     // 17
  GLANCE,      // 20
  FLEE,        // 28
  ESCAPE,      // 29
  PICK,        // 35
  STOCK,       // 56
  BUY,         // 56
  SELL,        // 57
  VALUE,       // 58
  LIST,        // 59
  ENTER,       // 60
  CLIMB,       // 60
  DESIGN,      // 62
  PRICE,       // 65
  REPAIR,      // 66
  READ,        // 67
  REMOVE,      // 69
  ERASE,       // 70
  ESTIMATE,    // 71
  REMORT,      // 80
  REROLL,      // 81
  CHOOSE,      // 82
  CONFIRM,     // 83
  CANCEL,      // 84
  SLIP,        // 87
  GIVE,        // 88
  DROP,        // 89
  DONATE,      // 90
  QUIT,        // 91
  SACRIFICE,   // 92
  PUT,         // 93
  OPEN,        // 98
  EDITOR,      // 100
  FORCE,       // 123
  WRITE,       // 128
  WATCH,       // 155
  PRACTICE,    // 164
  TRAIN,       // 165
  PROFESSION,  // 166
  GAIN,        // 171
  BALANCE,     // 172
  DEPOSIT,     // 173
  WITHDRAW,    // 174
  CLEAN,       // 177
  PLAY,        // 178
  FINISH,      // 179
  VETERNARIAN, // 180
  FEED,        // 181
  ASSEMBLE,    // 182
  PAY,         // 183
  RESTRING,    // 184
  PUSH,        // 185
  PULL,        // 186
  LEAVE,       // 187
  TREMOR,      // 188
  BET,         // 189
  INSURANCE,   // 190
  DOUBLE,      // 191
  STAY,        // 192
  SPLIT,       // 193
  HIT,         // 194
  LOOT,        // 195
  GTELL,       // 200
  CTELL,       // 201
  SETVOTE,     // 202
  VOTE,        // 203
  VEND,        // 204
  FILTER,      // 205
  EXAMINE,     // 206
  GAG,         // 207
  IMMORT,      // 208
  IMPCHAN,     // 209
  TELL,        // 210
  TELLH,       // 211
  PRIZE,       // 999
  OTHER,       // 999
  TELL_REPLY,  // 9999
  GAZE,        // 1820
  SAVE_SILENTLY,
  ONEWAY,
  TWOWAY,
  MLOCATE_CHARACTER,
  FEAR,
  PAGING_HELP,
  QUEST_CANCEL,
  QUEST_START,
  QUEST_FINISH,
  QUEST_LIST,
  GOLEMSCORE,
  FSCORE,
  REDEEM
};
enum class targets_t
{
  Self,
  Tank,
  Fighting,
  Charmie,
  GrouptMember
};
enum SortState
{
  SORT_XP,
  SORT_GOLD,
  SORT_MOB
};
enum class ReturnValue
{
  eFAILURE = 1U,
  eSUCCESS = 1U << 1,
  eCH_DIED = 1U << 2,
  eVICT_DIED = 1U << 3,
  eINTERNAL_ERROR = 1U << 4,
  eEXTRA_VALUE = 1U << 5, // Added to act like a flag, setting if something
                          // Special happened in the function.. (verify_existing_components use at the moment)
  eEXTRA_VAL2 = 1U << 6,  // damage() needs two

  eDELAYED_EXEC = 1U << 7, // Mobprogs, MPPAUSE
  eIMMUNE_VICTIM = 1U << 8 // returned by damage() when attacking somethat's immune to attack
};
enum Continents
{
  NO_CONTINENT = 1,    // 1
  SORPIGAL_CONTINENT,  // 2
  FAR_REACH,           // 3
  DIAMOND_ISLE,        // 4
  UNDERDARK,           // 5
  BEHIND_THE_MIRROR,   // 6
  PLANES_OF_EXISTANCE, // 7
  FORBIDDEN_ISLAND,    // 8
  OTHER_CONTINENT,     // 9
  MAX_CONTINENTS       // for iteration
};
enum mob_type_t
{
  MOB_NORMAL = 0,
  MOB_GUARD,
  MOB_CLAN_GUARD,
  MOB_TYPE_FIRST = MOB_NORMAL,
  MOB_TYPE_LAST = MOB_CLAN_GUARD
};
enum class attribute_t : quint8
{
  UNDEFINED = 0,
  STRENGTH = 1,
  DEXTERITY = 2,
  INTELLIGENCE = 3,
  WISDOM = 4,
  CONSTITUTION = 5
};
enum class vault_search_type
{
  UNDEFINED,
  KEYWORD,
  LEVEL,
  MIN_LEVEL,
  MAX_LEVEL
};
enum class load_status_t
{
  unknown,  // default unknown value
  success,  // successfully loaded something
  missing,  // not found
  error,    // error loading
  bad_input // bad input
};
enum class inet_protocol_family_t
{
  UNKNOWN,
  TCP4,
  TCP6,
  UNRECOGNIZED
};
enum class search_error
{
  invalid_input,
  not_found
};
enum class create_error
{
  unknown_error,
  index_full,
  entry_exists
};
enum class CommandType
{
  all,
  players_only,
  non_players_only,
  immortals_only,
  implementors_only
};
enum class position_t : quint8
{
  DEAD = 0,
  STUNNED = 3,
  SLEEPING = 4,
  RESTING = 5,
  SITTING = 6,
  FIGHTING = 7,
  STANDING = 8
};
enum telnet
{
  will_opt = '\xFB',
  wont_opt = '\xFC',
  do_opt = '\xFD',
  dont_opt = '\xFE',
  iac = '\xFF'
};
enum mprog_ifs
{
  eUNDEFINED,
  eRAND,
  eRAND1K,
  eAMTITEMS,
  eNUMPCS,
  eNUMOFMOBSINWORLD,
  eNUMOFMOBSINROOM,
  eISPC,
  eISWIELDING,
  eISWEAPPRI,
  eISWEAPSEC,
  eISNPC,
  eISGOOD,
  eISNEUTRAL,
  eISEVIL,
  eISWORN,
  eISFIGHT,
  eISTANK,
  eISIMMORT,
  eISCHARMED,
  eISFOLLOW,
  eISSPELLED,
  eISAFFECTED,
  eHITPRCNT,
  eWEARS,
  eCARRIES,
  eNUMBER,
  eTEMPVAR,
  eISMOBVNUMINROOM,
  eISOBJVNUMINROOM,
  eCANSEE,
  eHASDONEQUEST1,
  eINSAMEZONE,
  eCLAN,
  eISDAYTIME,
  eISRAINING,
  eNUMOFOBJSINWORLD
};
enum BACKUP_TYPE
{
  NONE,
  SELFDELETED,
  CONDEATH,
  ZAPPED
};
enum ListOptions
{
  LIST_ALL = 0,
  LIST_MINE,
  LIST_PRIVATE,
  LIST_BY_NAME,
  LIST_BY_LEVEL,
  LIST_BY_SLOT,
  LIST_BY_SELLER,
  LIST_BY_CLASS,
  LIST_BY_RACE,
  LIST_RECENT
};
enum AuctionStates
{
  AUC_FOR_SALE = 0,
  AUC_EXPIRED,
  AUC_SOLD,
  AUC_DELETED
};
enum class follower_reasons_t
{
  DEFAULT,           // 0
  END_STALK,         // 1
  CHANGE_LEADER,     // 2
  BROKE_CHARM,       // 3
  BROKE_CHARM_LILITH // 4
};
enum MatchType
{
  Failure,
  Subset,
  Exact
};
enum class parse_t
{
  FORMAT,    // 0
  REPLACE,   // 1
  HELP,      // 2
  DELETE,    // 3
  INSERT,    // 4
  LIST_NORM, // 5
  LIST_NUM,  // 6
  EDIT       // 7
};
enum pulse_type
{
  TIMER,
  MOBILE,
  OBJECT,
  VIOLENCE,
  BARD,
  TENSEC,
  WEATHER,
  TIME,
  REGEN,
  SHORT
};

using attribute_points_t = qint8;
using clan_id_t = quint64;
using class_t = quint8;
using gold_t = quint64;
using help_index_id_t = quint64;
using item_types_t = QStringList;
using legacy_rnum_t = qint32;
using level_diff_t = qint64;
using level_t = quint64;
using location_t = qint32;
using modifier_t = qint32;
using move_t = quint64;
using object_type_t = quint16;
using object_value_t = qint32;
using player_config_key_t = QString;
using player_config_value_t = QString;
using rnum_t = quint64;
using room_t = quint64;
using selfpurge_t = class SelfPurge;
using skill_t = qint16;
using socket_t = qint32;
using vnum_t = quint64;
using zone_t = quint64;

using affected_typePtr = QPointer<class affected_type>;
using CasinoPlayerPtr = QPointer<class CasinoPlayer>;
using CasinoRouletteWheelPtr = QPointer<class CasinoRouletteWheel>;
using CasinoSlotMachinePtr = QPointer<class CasinoSlotMachine>;
using CasinoTablePtr = QPointer<class CasinoTable>;
using cDeckPtr = QPointer<class cDeck>;
using ChannelMessagePtr = QPointer<class ChannelMessage>;
using CharacterPtr = QPointer<class Character>;
using ClanMemberPtr = QPointer<class ClanMember>;
using ClanPtr = QPointer<class Clan>;
using ColumnPtr = QPointer<class Column>;
using ConnectionPtr = QPointer<class Connection>;
using DatabasePtr = QPointer<class Database>;
using DCPtr = QPointer<class DC>;
using DenyPtr = QPointer<class Deny>;
using ExtraDescriptionPtr = QPointer<class ExtraDescription>;
using hunt_dataPtr = QPointer<class hunt_data>;
using hunt_itemsPtr = QPointer<class hunt_items>;
using MobileProgramPtr = QPointer<class MobileProgram>;
using MobilePtr = QPointer<class Mobile>;
using NewCharacterStatsPtr = QPointer<class NewCharacterStats>;
using ObjectProgramPtr = QPointer<class ObjectProgram>;
using ObjectPtr = QPointer<class Object>;
using ObjectIndexPtr = QPointer<class ObjectIndex>;
using PathPtr = QPointer<class Path>;
using PlayerPtr = QPointer<class Pointer>;
using ProgramPtr = QPointer<class Program>;
using PulsePtr = QPointer<class Pulse>;
using quest_infoPtr = QPointer<class quest_info>;
using ReservationPtr = QPointer<class Reservation>;
using ResetCommandPtr = QPointer<class ResetCommand>;
using RoomDirectionPtr = QPointer<class RoomDirection>;
using RoomPtr = QPointer<class Room>;
using TablePtr = QPointer<class Table>;
using TimerPtr = QPointer<class Timer>;
using TracksPtr = QPointer<class Tracks>;
using vault_access_dataPtr = QPointer<class vault_access_data>;
using vault_items_dataPtr = QPointer<class vault_items_data>;
using VaultPtr = QPointer<class Vault>;
using ZonePtr = QPointer<class Zone>;
using CasinoTablePlayerPtr = QPointer<class CasinoTablePlayer>;
using TexasHoldemTablePtr = QPointer<class TexasHoldemTable>;
using CasinoPotPtr = QPointer<class CasinoPot>;
using MobileIndexPtr = QPointer<class MobileIndex>;

union varg_t
{
  CharacterPtr ch;
  clan_id_t clan;
  CasinoPlayerPtr player;
  CasinoTablePtr table;
  CasinoSlotMachinePtr machine;
  CasinoRouletteWheelPtr wheel;
};

using command_gen2_t = ReturnValue (*)(CharacterPtr ch, QString arguments, cmd_t cmd);
using command_gen3_t = ReturnValue (*)(QStringList arguments, cmd_t cmd);
using command_special_t = ReturnValue (*)(QString arguments, cmd_t cmd);
using getter_t = QString (*)(void);
using HAND_FUNC = qint32 (*)(QList<qint32> hand);
using test_function_t = ReturnValue (*)(CharacterPtr ch);
using ROOM_PROC = test_function_t (*)(CharacterPtr ch, cmd_t cmd, QString argument);
using setter_t = bool (*)(QString);
using special_function = qint32 (*)(CharacterPtr, ObjectPtr, cmd_t, QString, CharacterPtr);
using SING_FUN = qint32 (*)(quint8 level, CharacterPtr ch, QString arg, CharacterPtr victim, qint32 skill);
using SPELL_FUN = qint32 (*)(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
using SPELL_FUN2 = qint32 (*)(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill, quint64 mana_cost);
using spell_gen1_t = qint32 (*)(quint8 level, CharacterPtr ch, QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
using spell_gen2_t = ReturnValue (*)(quint8 level, CharacterPtr ch, QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill, quint64 mana_cost);
using SPELL_POINTER = qint32 (*)(quint8, CharacterPtr, QString, qint32, CharacterPtr, ObjectPtr, qint32);
using SPEC_FUN = qint32 (*)(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString argument, CharacterPtr owner);
using TIMER_FUNC = void (*)(varg_t arg1, void *arg2, void *arg3);

using aliases_t = QMap<QString, QString>;
using area_stats_t = QMap<zone_t, class AreaStats>;
using character_list_i = QSet<CharacterPtr>::iterator;
using character_list_t = QSet<CharacterPtr>;
using clan_list_t = QMap<clan_id_t, class Clan>;
using classes_t = QList<class class_data>;
using client_descriptor_list_i = QSet<qint32>::iterator;
using client_descriptor_list_t = QSet<qint32>;
using help_index_t = QMap<QString, quint64>;
using hints_t = QList<QString>;
using history_t = QQueue<class communication>;
using ignoring_t = QMap<QString, class ignore_entry>;
using joining_t = QMap<QString, bool>;
using obj_list_t = QSet<ObjectPtr>;
using player_config_t = QMap<player_config_key_t, player_config_value_t>;
using port_list_i = QList<in_port_t>::iterator;
using port_list_t = QList<in_port_t>;
using quest_list_t = QList<quest_infoPtr>;
using server_descriptor_list_i = QSet<qint32>::iterator;
using server_descriptor_list_t = QSet<qint32>;
using skill_list_t = QMap<skill_t, class char_skill_data>;
using skill_results_t = QMap<QString, quint64>;
using special_function_list_t = QMap<vnum_t, special_function>;
using tests_t = QMap<QString, class Test>;
using zone_commands_t = QList<ResetCommandPtr>;
using zones_t = QMap<zone_t, class Zone>;

using namespace Qt::StringLiterals;

constexpr auto TYPE_UNDEFINED = -1;
constexpr auto SPELL_RESERVED_DBC = 0;
constexpr auto SPELL_ARMOR = 1;
constexpr auto SPELL_TELEPORT = 2;
constexpr auto SPELL_BLESS = 3;
constexpr auto SPELL_BLINDNESS = 4;
constexpr auto SPELL_BURNING_HANDS = 5;
constexpr auto SPELL_IRIDESCENT_AURA = 6;
constexpr auto SPELL_CHARM_PERSON = 7;
constexpr auto SPELL_CHILL_TOUCH = 8;
constexpr auto SPELL_CLONE = 9;
constexpr auto SPELL_COLOUR_SPRAY = 10;
constexpr auto SPELL_CONTROL_WEATHER = 11;
constexpr auto SPELL_CREATE_FOOD = 12;
constexpr auto SPELL_CREATE_WATER = 13;
constexpr auto SPELL_REMOVE_BLIND = 14;
constexpr auto SPELL_CURE_CRITIC = 15;
constexpr auto SPELL_CURE_LIGHT = 16;
constexpr auto SPELL_CURSE = 17;
constexpr auto SPELL_DETECT_EVIL = 18;
constexpr auto SPELL_DETECT_INVISIBLE = 19;
constexpr auto SPELL_DETECT_MAGIC = 20;
constexpr auto SPELL_DETECT_POISON = 21;
constexpr auto SPELL_DISPEL_EVIL = 22;
constexpr auto SPELL_EARTHQUAKE = 23;
constexpr auto SPELL_ENCHANT_WEAPON = 24;
constexpr auto SPELL_ENERGY_DRAIN = 25;
constexpr auto SPELL_FIREBALL = 26;
constexpr auto SPELL_HARM = 27;
constexpr auto SPELL_HEAL = 28;
constexpr auto SPELL_INVISIBLE = 29;
constexpr auto SPELL_LIGHTNING_BOLT = 30;
constexpr auto SPELL_LOCATE_OBJECT = 31;
constexpr auto SPELL_MAGIC_MISSILE = 32;
constexpr auto SPELL_POISON = 33;
constexpr auto SPELL_PROTECT_FROM_EVIL = 34;
constexpr auto SPELL_REMOVE_CURSE = 35;
constexpr auto SPELL_SANCTUARY = 36;
constexpr auto SPELL_SHOCKING_GRASP = 37;
constexpr auto SPELL_SLEEP = 38;
constexpr auto SPELL_STRENGTH = 39;
constexpr auto SPELL_SUMMON = 40;
constexpr auto SPELL_VENTRILOQUATE = 41;
constexpr auto SPELL_WORD_OF_RECALL = 42;
constexpr auto SPELL_REMOVE_POISON = 43;
constexpr auto SPELL_SENSE_LIFE = 44;
constexpr auto SPELL_SUMMON_FAMILIAR = 45;
constexpr auto SPELL_LIGHTED_PATH = 46;
constexpr auto SPELL_RESIST_ACID = 47;
constexpr auto SPELL_SUN_RAY = 48;
constexpr auto SPELL_RAPID_MEND = 49;
constexpr auto SPELL_ACID_SHIELD = 50;
constexpr auto SPELL_WATER_BREATHING = 51;
constexpr auto SPELL_GLOBE_OF_DARKNESS = 52;
constexpr auto SPELL_IDENTIFY = 53;
constexpr auto SPELL_ANIMATE_DEAD = 54;
constexpr auto SPELL_FEAR = 55;
constexpr auto SPELL_FLY = 56;
constexpr auto SPELL_CONT_LIGHT = 57;
constexpr auto SPELL_KNOW_ALIGNMENT = 58;
constexpr auto SPELL_DISPEL_MAGIC = 59;
constexpr auto SPELL_CONJURE_ELEMENTAL = 60;
constexpr auto SPELL_CURE_SERIOUS = 61;
constexpr auto SPELL_CAUSE_LIGHT = 62;
constexpr auto SPELL_CAUSE_CRITICAL = 63;
constexpr auto SPELL_CAUSE_SERIOUS = 64;
constexpr auto SPELL_FLAMESTRIKE = 65;
constexpr auto SPELL_STONE_SKIN = 66;
constexpr auto SPELL_SHIELD = 67;
constexpr auto SPELL_WEAKEN = 68;
constexpr auto SPELL_MASS_INVISIBILITY = 69;
constexpr auto SPELL_ACID_BLAST = 70;
constexpr auto SPELL_PORTAL = 71;
constexpr auto SPELL_INFRAVISION = 72;
constexpr auto SPELL_REFRESH = 73;
constexpr auto SPELL_HASTE = 74;
constexpr auto SPELL_DISPEL_GOOD = 75;
constexpr auto SPELL_HELLSTREAM = 76;
constexpr auto SPELL_POWER_HEAL = 77;
constexpr auto SPELL_FULL_HEAL = 78;
constexpr auto SPELL_FIRESTORM = 79;
constexpr auto SPELL_POWER_HARM = 80;
constexpr auto SPELL_DETECT_GOOD = 81;
constexpr auto SPELL_VAMPIRIC_TOUCH = 82;
constexpr auto SPELL_LIFE_LEECH = 83;
constexpr auto SPELL_PARALYZE = 84;
constexpr auto SPELL_REMOVE_PARALYSIS = 85;
constexpr auto SPELL_FIRESHIELD = 86;
constexpr auto SPELL_METEOR_SWARM = 87;
constexpr auto SPELL_WIZARD_EYE = 88;
constexpr auto SPELL_TRUE_SIGHT = 89;
constexpr auto SPELL_MANA = 90;
constexpr auto SPELL_SOLAR_GATE = 91;
constexpr auto SPELL_HEROES_FEAST = 92;
constexpr auto SPELL_HEAL_SPRAY = 93;
constexpr auto SPELL_GROUP_SANC = 94;
constexpr auto SPELL_GROUP_RECALL = 95;
constexpr auto SPELL_GROUP_FLY = 96;
constexpr auto SPELL_ENCHANT_ARMOR = 97;
constexpr auto SPELL_RESIST_FIRE = 98;
constexpr auto SPELL_RESIST_COLD = 99;
constexpr auto SPELL_BEE_STING = 100;
constexpr auto SPELL_BEE_SWARM = 101;
constexpr auto SPELL_CREEPING_DEATH = 102;
constexpr auto SPELL_BARKSKIN = 103;
constexpr auto SPELL_HERB_LORE = 104;
constexpr auto SPELL_CALL_FOLLOWER = 105;
constexpr auto SPELL_ENTANGLE = 106;
constexpr auto SPELL_EYES_OF_THE_OWL = 107;
constexpr auto SPELL_FELINE_AGILITY = 108;
constexpr auto SPELL_FOREST_MELD = 109;
constexpr auto SPELL_COMPANION = 110;
constexpr auto SPELL_DROWN = 111;
constexpr auto SPELL_HOWL = 112;
constexpr auto SPELL_SOULDRAIN = 113;
constexpr auto SPELL_SPARKS = 114;
constexpr auto SPELL_CAMOUFLAGE = 115;
constexpr auto SPELL_FARSIGHT = 116;
constexpr auto SPELL_FREEFLOAT = 117;
constexpr auto SPELL_INSOMNIA = 118;
constexpr auto SPELL_SHADOWSLIP = 119;
constexpr auto SPELL_RESIST_ENERGY = 120;
constexpr auto SPELL_STAUNCHBLOOD = 121;
constexpr auto SPELL_CREATE_GOLEM = 122;
constexpr auto SPELL_REFLECT = 123;
constexpr auto SPELL_DISPEL_MINOR = 124;
constexpr auto SPELL_RELEASE_GOLEM = 125;
constexpr auto SPELL_BEACON = 126;
constexpr auto SPELL_STONE_SHIELD = 127;
constexpr auto SPELL_GREATER_STONE_SHIELD = 128;
constexpr auto SPELL_IRON_ROOTS = 129;
constexpr auto SPELL_EYES_OF_THE_EAGLE = 130;
constexpr auto SPELL_MISANRA_QUIVER = 131;
constexpr auto SPELL_ICESTORM = 132;
constexpr auto SPELL_LIGHTNING_SHIELD = 133;
constexpr auto SPELL_BLUE_BIRD = 134;
constexpr auto SPELL_DEBILITY = 135;
constexpr auto SPELL_ATTRITION = 136;
constexpr auto SPELL_VAMPIRIC_AURA = 137;
constexpr auto SPELL_HOLY_AURA = 138;
constexpr auto SPELL_DISMISS_FAMILIAR = 139;
constexpr auto SPELL_DISMISS_CORPSE = 140;
constexpr auto SPELL_BLESSED_HALO = 141;
constexpr auto SPELL_VISAGE_OF_HATE = 142;
constexpr auto SPELL_PROTECT_FROM_GOOD = 143;
constexpr auto SPELL_OAKEN_FORTITUDE = 144;
constexpr auto SPELL_FROSTSHIELD = 145;
constexpr auto SPELL_STABILITY = 146;
constexpr auto SPELL_KILLER = 147;
constexpr auto SPELL_CANTQUIT = 148;
constexpr auto SPELL_SOLIDITY = 149;
constexpr auto SPELL_EAS = 150;
constexpr auto SPELL_ALIGN_GOOD = 151; // uriel's fire of redemption
constexpr auto SPELL_ALIGN_EVIL = 152; // Moruk's heart
constexpr auto SPELL_AEGIS = 153;
constexpr auto SPELL_U_AEGIS = 154;
constexpr auto SPELL_RESIST_MAGIC = 155;
constexpr auto SPELL_EAGLE_EYE = 156;
constexpr auto SPELL_CALL_LIGHTNING = 157;
constexpr auto SPELL_DIVINE_FURY = 158;
constexpr auto SPELL_GHOSTWALK = 159;
constexpr auto SPELL_MEND_GOLEM = 160;
constexpr auto SPELL_CLARITY = 161;
constexpr auto SPELL_DIVINE_INTER = 162;
constexpr auto SPELL_WRATH_OF_GOD = 163;
constexpr auto SPELL_ATONEMENT = 164;
constexpr auto SPELL_SILENCE = 165;
constexpr auto SPELL_IMMUNITY = 166;
constexpr auto SPELL_BONESHIELD = 167;
constexpr auto SPELL_CHANNEL = 168;
constexpr auto SPELL_RELEASE_ELEMENTAL = 169;
constexpr auto SPELL_WILD_MAGIC = 170;
constexpr auto SPELL_SPIRIT_SHIELD = 171;
constexpr auto SPELL_VILLAINY = 172;
constexpr auto SPELL_HEROISM = 173;
constexpr auto SPELL_CONSECRATE = 174;
constexpr auto SPELL_DESECRATE = 175;
constexpr auto SPELL_ELEMENTAL_WALL = 176;
constexpr auto SPELL_ETHEREAL_FOCUS = 177;
constexpr auto MAX_SPL_LIST = 177;

// if you add a spell, make sure you update "spells[]" in spells.C

/*
 * 150 to 249 reserved for more spells.
 */

/*
 * KI usage is here to avoid code duplication
 */
constexpr auto KI_BLAST = 0;
constexpr auto KI_PUNCH = 1;
constexpr auto KI_SENSE = 2;
constexpr auto KI_STORM = 3;
constexpr auto KI_SPEED = 4;
constexpr auto KI_PURIFY = 5;
constexpr auto KI_DISRUPT = 6;
constexpr auto KI_STANCE = 7;
constexpr auto KI_AGILITY = 8;
constexpr auto KI_MEDITATION = 9;
constexpr auto KI_TRANSFER = 10;
constexpr auto MAX_KI_LIST = 10;
constexpr auto KI_OFFSET = 250; // why this is done differently than the rest, I have no
                                // idea....ki skills are 250-296.  -pir

constexpr auto SKILL_BASE = 300;
constexpr auto SKILL_TRIP = 300;
constexpr auto SKILL_DODGE = 301;
constexpr auto SKILL_SECOND_ATTACK = 302;
constexpr auto SKILL_DISARM = 303;
constexpr auto SKILL_THIRD_ATTACK = 304;
constexpr auto SKILL_PARRY = 305;
constexpr auto SKILL_DEATHSTROKE = 306;
constexpr auto SKILL_CIRCLE = 307;
constexpr auto SKILL_BERSERK = 308;
constexpr auto SKILL_HEADBUTT = 309;
constexpr auto SKILL_EAGLE_CLAW = 310;
constexpr auto SKILL_QUIVERING_PALM = 311;
constexpr auto SKILL_PALM = 312;
constexpr auto SKILL_STALK = 313;
constexpr auto SKILL_UNUSED = 314;
constexpr auto SKILL_DUAL_BACKSTAB = 315;
constexpr auto SKILL_HITALL = 316;
constexpr auto SKILL_STUN = 317;
constexpr auto SKILL_SCAN = 318;
constexpr auto SKILL_CONSIDER = 319;
constexpr auto SKILL_SWITCH = 320;
constexpr auto SKILL_REDIRECT = 321;
constexpr auto SKILL_AMBUSH = 322;
constexpr auto SKILL_FORAGE = 323;
constexpr auto SKILL_TAME = 324;
constexpr auto SKILL_TRACK = 325;
constexpr auto SKILL_SKEWER = 326;
constexpr auto SKILL_SLIP = 327;
constexpr auto SKILL_RETREAT = 328;
constexpr auto SKILL_RAGE = 329;
constexpr auto SKILL_BATTLECRY = 330;
constexpr auto SKILL_ARCHERY = 331;
constexpr auto SKILL_RIPOSTE = 332;
constexpr auto SKILL_LAY_HANDS = 333;
constexpr auto SKILL_INSANE_CHANT = 334;
constexpr auto SKILL_GLITTER_DUST = 335;
constexpr auto SKILL_SNEAK = 336;
constexpr auto SKILL_HIDE = 337;
constexpr auto SKILL_STEAL = 338;
constexpr auto SKILL_BACKSTAB = 339;
constexpr auto SKILL_PICK_LOCK = 340;
constexpr auto SKILL_KICK = 341;
constexpr auto SKILL_BASH = 342;
constexpr auto SKILL_RESCUE = 343;
constexpr auto SKILL_BLOOD_FURY = 344;
constexpr auto SKILL_DUAL_WIELD = 345;
constexpr auto SKILL_HARM_TOUCH = 346;
constexpr auto SKILL_SHIELDBLOCK = 347;
constexpr auto SKILL_BLADESHIELD = 348;
constexpr auto SKILL_POCKET = 349;
constexpr auto SKILL_GUARD = 350;
constexpr auto SKILL_FRENZY = 351;
constexpr auto SKILL_BLINDFIGHTING = 352;
constexpr auto SKILL_FOCUSED_REPELANCE = 353;
constexpr auto SKILL_VITAL_STRIKE = 354;
constexpr auto SKILL_CRAZED_ASSAULT = 355;
constexpr auto SKILL_DIVINE_PROTECTION = 356;
constexpr auto SKILL_BLUDGEON_WEAPONS = 357;
constexpr auto SKILL_PIERCEING_WEAPONS = 358;
constexpr auto SKILL_SLASHING_WEAPONS = 359;
constexpr auto SKILL_WHIPPING_WEAPONS = 360;
constexpr auto SKILL_CRUSHING_WEAPONS = 361;
constexpr auto SKILL_TWO_HANDED_WEAPONS = 362;
constexpr auto SKILL_HAND_TO_HAND = 363;
constexpr auto SKILL_BULLRUSH = 364;
constexpr auto SKILL_FEROCITY = 365;
constexpr auto SKILL_TACTICS = 366;
constexpr auto SKILL_DECEIT = 367;
constexpr auto SKILL_RELEASE = 368;
constexpr auto SKILL_FEARGAZE = 369;
constexpr auto SKILL_EYEGOUGE = 370;
constexpr auto SKILL_MAGIC_RESIST = 371;
constexpr auto NEW_SAVE = 372; // Savefix.
constexpr auto SKILL_SPELLCRAFT = 373;
constexpr auto SKILL_DEFENSE = 374; // MArtial defense
constexpr auto SKILL_KNOCKBACK = 375;
constexpr auto SKILL_STINGING_WEAPONS = 376;
constexpr auto SKILL_JAB = 377;
constexpr auto SKILL_APPRAISE = 378;
constexpr auto SKILL_NATURES_LORE = 379;
constexpr auto SKILL_FIRE_ARROW = 380;
constexpr auto SKILL_ICE_ARROW = 381;
constexpr auto SKILL_TEMPEST_ARROW = 382;
constexpr auto SKILL_GRANITE_ARROW = 383;
constexpr auto DO_NOT_USE = 384; // Oh how convenient skills are for this,
                                 // and so bad looking. yay. I want mysql.
constexpr auto META_REIMB = 385;
constexpr auto SKILL_COMBAT_MASTERY = 386;
constexpr auto SKILL_FASTJOIN = 387;
constexpr auto SKILL_ENHANCED_REGEN = 388;
constexpr auto SKILL_CRIPPLE = 389;
constexpr auto SKILL_NAT_SELECT = 390;
constexpr auto SKILL_CLANAREA_CLAIM = 391;
constexpr auto SKILL_CLANAREA_CHALLENGE = 392;
constexpr auto SKILL_COMMUNE = 393;
constexpr auto SKILL_SCRIBE = 394;
constexpr auto SKILL_MAKE_CAMP = 395;
constexpr auto SKILL_BATTLESENSE = 396;
constexpr auto SKILL_PERSEVERANCE = 397;
constexpr auto SKILL_TRIAGE = 398;
constexpr auto SKILL_SMITE = 399;
constexpr auto SKILL_LEADERSHIP = 400;
constexpr auto SKILL_EXECUTE = 401;
constexpr auto SKILL_DEFENDERS_STANCE = 402;
constexpr auto SKILL_BEHEAD = 403;
constexpr auto SKILL_PRIMAL_FURY = 404;
constexpr auto SKILL_VIGOR = 405;
constexpr auto SKILL_ESCAPE = 406;
constexpr auto SKILL_CRIT_HIT = 407;
constexpr auto SKILL_BATTERBRACE = 408;
constexpr auto SKILL_FREE_ANIMAL = 409;
constexpr auto SKILL_OFFHAND_DOUBLE = 410;
constexpr auto SKILL_ONSLAUGHT = 411;
constexpr auto SKILL_COUNTER_STRIKE = 412;
constexpr auto SKILL_IMBUE = 413;
constexpr auto SKILL_ELEMENTAL_FILTER = 414;
constexpr auto SKILL_ORCHESTRATE = 415;
constexpr auto SKILL_TUMBLING = 416;
constexpr auto SKILL_BREW = 417;
constexpr auto SKILL_PURSUIT = 418;
//			                     419
// warrior
constexpr auto SKILL_LEGIONNAIRE = 420;
constexpr auto SKILL_GLADIATOR = 421;
// Barbarian
constexpr auto SKILL_BATTLERAGER = 422;
constexpr auto SKILL_CHIEFTAN = 423;
// thief
constexpr auto SKILL_PILFERER = 424;
constexpr auto SKILL_ASSASSIN = 425;
// mage
constexpr auto SKILL_WARMAGE = 426;
constexpr auto SKILL_SPELLBINDER = 427;
// cleric
constexpr auto SKILL_ZEALOT = 428;
constexpr auto SKILL_RITUALIST = 429;
// druid
constexpr auto SKILL_ELEMENTALIST = 430;
constexpr auto SKILL_SHAPESHIFTER = 431;
// anti-paladin
constexpr auto SKILL_CULTIST = 432;
constexpr auto SKILL_REAVER = 433;
// paladin
constexpr auto SKILL_TEMPLAR = 434;
constexpr auto SKILL_INQUISITOR = 435;
// ranger
constexpr auto SKILL_SCOUT = 436;
constexpr auto SKILL_TRACKER = 437;
// monk
constexpr auto SKILL_SENSEI = 438;
constexpr auto SKILL_SPIRITUALIST = 439;
// bard
constexpr auto SKILL_TROUBADOUR = 440;
constexpr auto SKILL_MINISTREL = 441;

constexpr auto SKILL_MAX = 441;

// if you add a skill, make sure you update "skills[]" in spells.C
// as well as SKILL_MAX

constexpr auto SKILL_SONG_BASE = 525;
constexpr auto SKILL_SONG_LIST_SONGS = 525;
constexpr auto SKILL_SONG_WHISTLE_SHARP = 526;
constexpr auto SKILL_SONG_STOP = 527;
constexpr auto SKILL_SONG_TRAVELING_MARCH = 528;
constexpr auto SKILL_SONG_BOUNT_SONNET = 529;
constexpr auto SKILL_SONG_INSANE_CHANT = 530;
constexpr auto SKILL_SONG_GLITTER_DUST = 531;
constexpr auto SKILL_SONG_SYNC_CHORD = 532;
constexpr auto SKILL_SONG_HEALING_MELODY = 533;
constexpr auto SKILL_SONG_STICKY_LULL = 534;
constexpr auto SKILL_SONG_REVEAL_STACATO = 535;
constexpr auto SKILL_SONG_FLIGHT_OF_BEE = 536;
constexpr auto SKILL_SONG_JIG_OF_ALACRITY = 537;
constexpr auto SKILL_SONG_NOTE_OF_KNOWLEDGE = 538;
constexpr auto SKILL_SONG_TERRIBLE_CLEF = 539;
constexpr auto SKILL_SONG_SOOTHING_REMEM = 540;
constexpr auto SKILL_SONG_FORGETFUL_RHYTHM = 541;
constexpr auto SKILL_SONG_SEARCHING_SONG = 542;
constexpr auto SKILL_SONG_VIGILANT_SIREN = 543;
constexpr auto SKILL_SONG_ASTRAL_CHANTY = 544;
constexpr auto SKILL_SONG_DISARMING_LIMERICK = 545;
constexpr auto SKILL_SONG_SHATTERING_RESO = 546;
constexpr auto SKILL_SONG_UNRESIST_DITTY = 547;
constexpr auto SKILL_SONG_FANATICAL_FANFARE = 548;
constexpr auto SKILL_SONG_DISCHORDANT_DIRGE = 549;
constexpr auto SKILL_SONG_CRUSHING_CRESCENDO = 550;
constexpr auto SKILL_SONG_HYPNOTIC_HARMONY = 551;
constexpr auto SKILL_SONG_MKING_CHARGE = 552;
constexpr auto SKILL_SONG_SUBMARINERS_ANTHEM = 553;
constexpr auto SKILL_SONG_SUMMONING_SONG = 554;
constexpr auto SKILL_SONG_MAX = 554;
// if you add a song, make sure you update "songs[]" in sing.C
// as well as SKILL_SONG_MAX

// God commands that are "bestow"/"revoke"able
constexpr auto COMMAND_BASE = 600;
constexpr auto COMMAND_STRING = 600;
constexpr auto COMMAND_IMP_CHAN = 601;
constexpr auto COMMAND_STAT = 602;
constexpr auto COMMAND_SNOOP = 603;
constexpr auto COMMAND_FIND = 604;
constexpr auto COMMAND_POSSESS = 606;
constexpr auto COMMAND_RESTORE = 607;
constexpr auto COMMAND_PURLOIN = 608;
constexpr auto COMMAND_ARENA = 609;
constexpr auto COMMAND_SET = 610;
constexpr auto COMMAND_SQSAVE = 611;
constexpr auto COMMAND_WHATTONERF = 612;
constexpr auto COMMAND_FORCE = 613;
constexpr auto COMMAND_SEND = 614;
constexpr auto COMMAND_LOAD = 615;
constexpr auto COMMAND_SHUTDOWN = 616;
constexpr auto COMMAND_MP_EDIT = 617;
constexpr auto COMMAND_RANGE = 618;
constexpr auto COMMAND_MPSTAT = 619;
constexpr auto COMMAND_SEDIT = 621;
constexpr auto COMMAND_SOCKETS = 622;
constexpr auto COMMAND_PUNISH = 623;
constexpr auto COMMAND_SQEDIT = 624;
constexpr auto COMMAND_OCLONE = 625;
constexpr auto COMMAND_RELOAD = 626;
constexpr auto COMMAND_HEDIT = 627;
constexpr auto COMMAND_OPSTAT = 629;
constexpr auto COMMAND_OPEDIT = 630;
constexpr auto COMMAND_EQMAX = 631;
constexpr auto COMMAND_LOG = 632;
constexpr auto COMMAND_ADDNEWS = 633;
constexpr auto COMMAND_PRIZE = 634;
constexpr auto COMMAND_QEDIT = 635;
constexpr auto COMMAND_RENAME = 636;
constexpr auto COMMAND_FINDPATH = 637;
constexpr auto COMMAND_FINDPATH2 = 638;
constexpr auto COMMAND_ADDROOM = 639;
constexpr auto COMMAND_NEWPATH = 640;
constexpr auto COMMAND_LISTPATHSBYZONE = 641;
constexpr auto COMMAND_LISTALLPATHS = 642;
constexpr auto COMMAND_TESTHAND = 643;
constexpr auto COMMAND_DOPATHPATH = 644;
constexpr auto COMMAND_DO_THE_THING = 645;
constexpr auto COMMAND_QUEST = 646;
constexpr auto COMMAND_TESTPORT = 647;
constexpr auto COMMAND_REMORT = 648;
constexpr auto COMMAND_TESTHIT = 649;
constexpr auto COMMAND_TESTUSER = 650;
constexpr auto SKILL_TRADE_BASE = 700;
constexpr auto SKILL_TRADE_POISON = 700;
constexpr auto SKILL_TRADE_MAX = 700;
constexpr auto SKILL_RECALL = 800;
constexpr auto INTERNAL_SLEEPING = 801;
constexpr auto SKILL_FLAMESLASH = 850;
constexpr auto SPELL_FIRE_BREATH = 900;
constexpr auto SPELL_GAS_BREATH = 901;
constexpr auto SPELL_FROST_BREATH = 902;
constexpr auto SPELL_ACID_BREATH = 903;
constexpr auto SPELL_LIGHTNING_BREATH = 904;

/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 * If you change this, update strs_damage_types[] in const.cpp
 */
constexpr auto TYPE_HIT = 1000;
#define TYPE_BLUDGEON (TYPE_HIT + 1)
#define TYPE_PIERCE (TYPE_HIT + 2)
#define TYPE_SLASH (TYPE_HIT + 3)
#define TYPE_WHIP (TYPE_HIT + 4)
#define TYPE_CLAW (TYPE_HIT + 5)
#define TYPE_BITE (TYPE_HIT + 6)
#define TYPE_STING (TYPE_HIT + 7)
#define TYPE_CRUSH (TYPE_HIT + 8)
#define TYPE_SUFFERING (TYPE_HIT + 9)
#define TYPE_MAGIC (TYPE_HIT + 10)
#define TYPE_CHARM (TYPE_HIT + 11)
#define TYPE_FIRE (TYPE_HIT + 12)
#define TYPE_ENERGY (TYPE_HIT + 13)
#define TYPE_ACID (TYPE_HIT + 14)
#define TYPE_POISON (TYPE_HIT + 15)
#define TYPE_SLEEP (TYPE_HIT + 16)
#define TYPE_COLD (TYPE_HIT + 17)
#define TYPE_PARA (TYPE_HIT + 18)
#define TYPE_KI (TYPE_HIT + 19)
#define TYPE_SONG (TYPE_HIT + 20)
#define TYPE_PHYSICAL_MAGIC (TYPE_HIT + 21)
#define TYPE_WATER (TYPE_HIT + 22)
// If you change this, update strs_damage_types[] in const.cpp
////////////////////////////////////////////////////////////////

constexpr auto BASE_TIMERS = 1100;

// NOTE  "skill" numbers 1500-1599 are reserved for innate skill abilities
// These are in innate.h

//////////////////////////////////////////////////////////////////////
// NOTE 'spell' wear off timers are here.  Reserved messages 4000-4099
// If you change this, update reserved[] in const.cpp
//////////////////////////////////////////////////////////////////////
constexpr auto RESERVED_BASE = 4000;
constexpr auto SPELL_HOLY_AURA_TIMER = 4000;
constexpr auto SPELL_NAT_SELECT_TIMER = 4001;
constexpr auto SPELL_DIV_INT_TIMER = 4002;
constexpr auto SPELL_NO_CAST_TIMER = 4003;
constexpr auto SKILL_CM_TIMER = 4004;
constexpr auto OBJ_CHAMPFLAG_TIMER = 4005;
constexpr auto SKILL_TRIAGE_TIMER = 4006;
constexpr auto SKILL_SMITE_TIMER = 4007;
constexpr auto SKILL_MAKE_CAMP_TIMER = 4008;
constexpr auto SKILL_LEADERSHIP_BONUS = 4009;
constexpr auto SKILL_PERSEVERANCE_BONUS = 4010;
constexpr auto SKILL_DECEIT_TIMER = 4011;
constexpr auto SKILL_FEROCITY_TIMER = 4012;
constexpr auto SKILL_TACTICS_TIMER = 4013;
constexpr auto CONC_LOSS_FIXER = 4014;
constexpr auto SKILL_ONSLAUGHT_TIMER = 4015;
constexpr auto SPELL_KI_TRANS_TIMER = 4016;
constexpr auto SKILL_BREW_TIMER = 4017;
constexpr auto SKILL_SCRIBE_TIMER = 4018;
constexpr auto SKILL_PROFESSION = 4019;
constexpr auto OBJ_LILITHRING = 4020;
constexpr auto OBJ_DAWNSWORD = 4021;
constexpr auto OBJ_DURENDAL = 4022;
constexpr auto SPELL_VAMPIRIC_AURA_TIMER = 4023;
constexpr auto RESERVED_MAX = 4023;

///////////////////////////////////////////////////////////////////////

constexpr auto TAR_IGNORE = 1;
constexpr auto TAR_CHAR_ROOM = 1 << 1;
constexpr auto TAR_CHAR_WORLD = 1 << 2;
constexpr auto TAR_FIGHT_SELF = 1 << 3;
constexpr auto TAR_FIGHT_VICT = 1 << 4;
constexpr auto TAR_SELF_ONLY = 1 << 5;
constexpr auto TAR_SELF_NONO = 1 << 6;
constexpr auto TAR_OBJ_INV = 1 << 7;
constexpr auto TAR_OBJ_ROOM = 1 << 8;
constexpr auto TAR_OBJ_WORLD = 1 << 9;
constexpr auto TAR_OBJ_EQUIP = 1 << 10;
constexpr auto TAR_NONE_OK = 1 << 11;
constexpr auto TAR_SELF_DEFAULT = 1 << 12;
constexpr auto TAR_ROOM_EXIT = 1 << 13;

////////////////////////////////////////////////////////////////////////

constexpr auto ETHEREAL_FOCUS_TRIGGER_ACT = 1;
constexpr auto ETHEREAL_FOCUS_TRIGGER_MOVE = 2;
constexpr auto ETHEREAL_FOCUS_TRIGGER_SOCIAL = 3;

////////////////////////////////////////////////////////////////////////

constexpr auto SPELL_TYPE_SPELL = 0;
constexpr auto SPELL_TYPE_POTION = 1;
constexpr auto SPELL_TYPE_WAND = 2;
constexpr auto SPELL_TYPE_STAFF = 3;
constexpr auto SPELL_TYPE_SCROLL = 4;

constexpr auto FIRE_ELEMENTAL = 88;
constexpr auto WATER_ELEMENTAL = 89;
constexpr auto AIR_ELEMENTAL = 90;
constexpr auto EARTH_ELEMENTAL = 91;

constexpr auto WILD_OFFENSIVE = 0;
constexpr auto WILD_DEFENSIVE = 1;
constexpr auto FORMAT_INDENT = 1 << 0;

constexpr auto MAXIMUM_KI = 100;
constexpr auto MINIMUM_KI = 0;
constexpr auto MIN_REACT_KI = 20;
constexpr auto NO_EFFECT = 0;
constexpr auto DIVINE = 1;
constexpr auto MIRACLE = 2;
constexpr auto MAJOR_EFFECT = 3;
constexpr auto MINOR_EFFECT = 4;

// Global defines for innate skill abilities

// Innate skills can go from 1500 - 1599

constexpr auto SKILL_INNATE_BASE = 1500;
constexpr auto SKILL_INNATE_POWERWIELD = 1500;
constexpr auto SKILL_INNATE_FOCUS = 1501;
constexpr auto SKILL_INNATE_REGENERATION = 1502;
constexpr auto SKILL_INNATE_BLOODLUST = 1503;
constexpr auto SKILL_INNATE_ILLUSION = 1504;
constexpr auto SKILL_INNATE_EVASION = 1505;
constexpr auto SKILL_INNATE_SHADOWSLIP = 1506;
constexpr auto SKILL_INNATE_REPAIR = 1507;
constexpr auto SKILL_INNATE_TIMER = 1508;
constexpr auto SKILL_INNATE_FLY = 1509;
constexpr auto SKILL_INNATE_MAX = 1509;

constexpr auto RARE1_PAPER = 1;
constexpr auto RARE2_PAPER = 1 << 1;
constexpr auto RARE3_PAPER = 1 << 2;
constexpr auto RARE4_PAPER = 1 << 3;
constexpr auto RARE5_PAPER = 1 << 4;
// #define FREE_SLOT	1<<5

constexpr auto CLERIC_PEN = 1 << 6;
constexpr auto MAGE_PEN = 1 << 7;
constexpr auto DRUID_PEN = 1 << 8;
constexpr auto ANTI_PEN = 1 << 9;
constexpr auto RANGER_PEN = 1 << 10;
constexpr auto NONE_PEN = 1 << 11;
// #define FREE_SLOT	1<<12

constexpr auto MAGIC_INK = 1 << 13;
constexpr auto FIRE_INK = 1 << 14;
constexpr auto EVIL_INK = 1 << 15;
// #define FREE_SLOT	1<<16

constexpr auto FLASHY_DUST = 1 << 17;
constexpr auto EXPLOSIVE_DUST = 1 << 18;
constexpr auto GENERIC_DUST = 1 << 19;
// #define FREE_SLOT	1<<20

constexpr auto FILTER_FIRE = 1;
constexpr auto FILTER_MAGIC = 2;
constexpr auto FILTER_COLD = 3;
constexpr auto FILTER_ENERGY = 4;
constexpr auto FILTER_ACID = 5;
constexpr auto FILTER_POISON = 6;

constexpr auto DETECT_GOOD_VNUM = 6302;
constexpr auto DETECT_EVIL_VNUM = 6301;
constexpr auto DETECT_INVISIBLE_VNUM = 6306;
constexpr auto SENSE_LIFE_VNUM = 6304;
constexpr auto INFRA_VNUM = 6308;
constexpr auto INVIS_VNUM = 6303;
constexpr auto FARSIGHT_VNUM = 6307;
constexpr auto SOLIDITY_VNUM = 6309;
constexpr auto LIGHTNING_SHIELD_VNUM = 6310;
constexpr auto INSOMNIA_VNUM = 6311;
constexpr auto HASTE_VNUM = 6312;
constexpr auto TRUE_VNUM = 6305;

constexpr auto MAX_STRING_LENGTH = 8192;
constexpr auto MAX_GAME_PORTALS = 9;
constexpr auto FOREVER = -5;

constexpr auto MAX_INPUT_LENGTH = 160;
constexpr auto MAX_MESSAGES = 150;
constexpr auto MAX_OBJ_SDESC_LENGTH = 100;

constexpr auto MESS_ATTACKER = 1;
constexpr auto MESS_VICTIM = 2;
constexpr auto MESS_ROOM = 3;

constexpr auto BASE_STAT = 0;
// #define NOTHING      0
constexpr auto ACT_SPEC = 1;
constexpr auto ACT_SENTINEL = 2;
constexpr auto ACT_SCAVENGER = 3;
constexpr auto ACT_NOTRACK = 4;
constexpr auto ACT_NICE_THIEF = 5;
constexpr auto ACT_AGGRESSIVE = 6;
constexpr auto ACT_STAY_ZONE = 7;
constexpr auto ACT_WIMPY = 8;
/* aggressive only attack sleeping players */
constexpr auto ACT_2ND_ATTACK = 9;
constexpr auto ACT_3RD_ATTACK = 10;
constexpr auto ACT_4TH_ATTACK = 11;
/* Each attack bit must be set to get up */
/* 4 attacks                             */
/*
 * For ACT_AGGRESSIVE_XXX, you must also set ACT_AGGRESSIVE
 * These switches can be combined, if none are selected, then
 * the mobile will attack any alignment (same as if all 3 were set)
 */
constexpr auto ACT_AGGR_EVIL = 12;
constexpr auto ACT_AGGR_GOOD = 13;
constexpr auto ACT_AGGR_NEUT = 14;
constexpr auto ACT_UNDEAD = 15;
constexpr auto ACT_STUPID = 16;
constexpr auto ACT_CHARM = 17;
constexpr auto ACT_HUGE = 18;
constexpr auto ACT_DODGE = 19;
constexpr auto ACT_PARRY = 20;
constexpr auto ACT_RACIST = 21;
constexpr auto ACT_FRIENDLY = 22;
constexpr auto ACT_STAY_NO_TOWN = 23;
constexpr auto ACT_NOMAGIC = 24;
constexpr auto ACT_DRAINY = 25;
constexpr auto ACT_BARDCHARM = 26;
constexpr auto ACT_NOKI = 27;
constexpr auto ACT_NOMATRIX = 28;
constexpr auto ACT_BOSS = 29;
constexpr auto ACT_NOHEADBUTT = 30;
constexpr auto ACT_NOATTACK = 31;
// #define CHECKTHISACT      32 //Do not change unless ASIZE changes
constexpr auto ACT_SWARM = 33;
constexpr auto ACT_TINY = 34;
constexpr auto ACT_NODISPEL = 35;
constexpr auto ACT_POISONOUS = 36;
constexpr auto ACT_NO_GOLD_BONUS = 37;
constexpr auto ACT_NO_HUNT = 38;
// #define CHECKTHISACT      64 //Do not chance unless ASIZE changes

constexpr auto MPROG_CATCH_MIN = 1;
constexpr auto MPROG_CATCH_MAX = 100;
constexpr auto FLAG_DEFAULT = 0;       // "someone" if invisible, sleepers skipped
constexpr auto NOTVICT = 1 << 0;       // Sends to destination except victim
constexpr auto GODS = 1 << 1;          // Sends to destination, gods only
constexpr auto ASLEEP = 1 << 2;        // Will send even to sleepers
constexpr auto INVIS_NULL = 1 << 3;    // Invisible messages are skipped completely
constexpr auto INVIS_VISIBLE = 1 << 4; // Invisible messages are shown w/names visible
constexpr auto FLAG_FORCE = 1 << 5;    // Sends regardless of nanny state
constexpr auto STAYHIDE = 1 << 6;      // Stayhide flag keeps thieves in hiding.
constexpr auto BARDSONG = 1 << 7;      // Bard song so only show it to people in room with BARD_SONG toggle set to verbose
constexpr auto CLASS_MAGIC_USER = 1;
constexpr auto CLASS_MAGE = 1; // Laziness > consistency
constexpr auto CLASS_CLERIC = 2;
constexpr auto CLASS_THIEF = 3;
constexpr auto CLASS_WARRIOR = 4;
constexpr auto CLASS_ANTI_PAL = 5;
constexpr auto CLASS_PALADIN = 6;
constexpr auto CLASS_BARBARIAN = 7;
constexpr auto CLASS_MONK = 8;
constexpr auto CLASS_RANGER = 9;
constexpr auto CLASS_BARD = 10;
constexpr auto CLASS_DRUID = 11;
constexpr auto CLASS_MAX_PROD = 11;
constexpr auto CLASS_PSIONIC = 12;
constexpr auto CLASS_NECROMANCER = 13;
constexpr auto SECS_PER_REAL_MIN = 60;
constexpr auto SECS_PER_REAL_HOUR = (60 * SECS_PER_REAL_MIN);
constexpr auto SECS_PER_REAL_DAY = (24 * SECS_PER_REAL_HOUR);
constexpr auto SECS_PER_REAL_YEAR = (365 * SECS_PER_REAL_DAY);
constexpr auto SECS_PER_MUD_HOUR = 65;
constexpr auto SECS_PER_MUD_DAY = (24 * SECS_PER_MUD_HOUR);
constexpr auto SECS_PER_MUD_MONTH = (35 * SECS_PER_MUD_DAY);
constexpr auto SECS_PER_MUD_YEAR = (17 * SECS_PER_MUD_MONTH);
constexpr auto ISR_PIERCE = 1;
constexpr auto ISR_SLASH = 1 << 1;
constexpr auto ISR_MAGIC = 1 << 2;
constexpr auto ISR_CHARM = 1 << 3;
constexpr auto ISR_FIRE = 1 << 4;
constexpr auto ISR_ENERGY = 1 << 5;
constexpr auto ISR_ACID = 1 << 6;
constexpr auto ISR_POISON = 1 << 7;
constexpr auto ISR_SLEEP = 1 << 8;
constexpr auto ISR_COLD = 1 << 9;
constexpr auto ISR_PARA = 1 << 10;
constexpr auto ISR_BLUDGEON = 1 << 11;
constexpr auto ISR_WHIP = 1 << 12;
constexpr auto ISR_CRUSH = 1 << 13;
constexpr auto ISR_HIT = 1 << 14;
constexpr auto ISR_BITE = 1 << 15;
constexpr auto ISR_STING = 1 << 16;
constexpr auto ISR_CLAW = 1 << 17;
constexpr auto ISR_PHYSICAL = 1 << 18;
constexpr auto ISR_NON_MAGIC = 1 << 19;
constexpr auto ISR_KI = 1 << 20;
constexpr auto ISR_SONG = 1 << 21;
constexpr auto ISR_WATER = 1 << 22;
constexpr auto ISR_FEAR = 1 << 23;
constexpr auto ISR_MAX = 23;
constexpr auto SAVE_TYPE_FIRE = 0;
constexpr auto SAVE_TYPE_COLD = 1;
constexpr auto SAVE_TYPE_ENERGY = 2;
constexpr auto SAVE_TYPE_ACID = 3;
constexpr auto SAVE_TYPE_MAGIC = 4;
constexpr auto SAVE_TYPE_POISON = 5;
constexpr auto MAX_THROW_NAME = 60;
constexpr auto AFF_MAX = 73;
constexpr auto MAX_WEAR = 23;
constexpr auto GLOBE_OF_DARKNESS_OBJECT = 101;
constexpr auto ACT_MAX = 38;
constexpr auto MAX_MOB_VALUES = 4;
constexpr auto MAX_HIDE = 10;
constexpr auto QUEST_MAX = 1;         // max quests at a time
constexpr auto QUEST_SHOW = 10;       // max quests shown at a time
constexpr auto QUEST_MAX_CANCEL = 15; // max quests canceled at a time
constexpr auto QUEST_TOTAL = 500;     // max total quests in file
constexpr auto QUEST_MASTER = 10027;  // vnum of questmaster
constexpr auto ITEM_LIGHT = 1;
constexpr auto ITEM_SCROLL = 2;
constexpr auto ITEM_WAND = 3;
constexpr auto ITEM_STAFF = 4;
constexpr auto ITEM_WEAPON = 5;
constexpr auto ITEM_FIREWEAPON = 6;
constexpr auto ITEM_MISSILE = 7;
constexpr auto ITEM_TREASURE = 8;
constexpr auto ITEM_ARMOR = 9;
constexpr auto ITEM_POTION = 10;
constexpr auto ITEM_WORN = 11; // not used, can change
constexpr auto ITEM_OTHER = 12;
constexpr auto ITEM_TRASH = 13;
constexpr auto ITEM_TRAP = 14;
constexpr auto ITEM_CONTAINER = 15;
constexpr auto ITEM_NOTE = 16;
constexpr auto ITEM_DRINKCON = 17;
constexpr auto ITEM_KEY = 18;
constexpr auto ITEM_FOOD = 19;
constexpr auto ITEM_MONEY = 20;
constexpr auto ITEM_PEN = 21;
constexpr auto ITEM_BOAT = 22;
constexpr auto ITEM_BOARD = 23;
constexpr auto ITEM_PORTAL = 24;
constexpr auto ITEM_FOUNTAIN = 25;
constexpr auto ITEM_INSTRUMENT = 26;
constexpr auto ITEM_UTILITY = 27;
constexpr auto ITEM_BEACON = 28;
constexpr auto ITEM_LOCKPICK = 29;
constexpr auto ITEM_CLIMBABLE = 30;
constexpr auto ITEM_MEGAPHONE = 31;
constexpr auto ITEM_ALTAR = 32;
constexpr auto ITEM_TOTEM = 33;
constexpr auto ITEM_KEYRING = 34;
constexpr auto ITEM_TYPE_MAX = 34;
constexpr auto ITEM_GLOW = 1U;
constexpr auto ITEM_HUM = 1U << 1;
constexpr auto ITEM_DARK = 1U << 2;
constexpr auto ITEM_LOCK = 1U << 3;
constexpr auto ITEM_ANY_CLASS = 1U << 4;
constexpr auto ITEM_INVISIBLE = 1U << 5;
constexpr auto ITEM_MAGIC = 1U << 6;
constexpr auto ITEM_NODROP = 1U << 7;
constexpr auto ITEM_BLESS = 1U << 8;
constexpr auto ITEM_ANTI_GOOD = 1U << 9;
constexpr auto ITEM_ANTI_EVIL = 1U << 10;
constexpr auto ITEM_ANTI_NEUTRAL = 1U << 11;
constexpr auto ITEM_WARRIOR = 1U << 12;
constexpr auto ITEM_MAGE = 1U << 13;
constexpr auto ITEM_THIEF = 1U << 14;
constexpr auto ITEM_CLERIC = 1U << 15;
constexpr auto ITEM_PAL = 1U << 16;
constexpr auto ITEM_ANTI = 1U << 17;
constexpr auto ITEM_BARB = 1U << 18;
constexpr auto ITEM_MONK = 1U << 19;
constexpr auto ITEM_RANGER = 1U << 20;
constexpr auto ITEM_DRUID = 1U << 21;
constexpr auto ITEM_BARD = 1U << 22;
constexpr auto ITEM_TWO_HANDED = 1U << 23;
constexpr auto ITEM_ENCHANTED = 1U << 24;
constexpr auto ITEM_SPECIAL = 1U << 25;
constexpr auto ITEM_NOSAVE = 1U << 26;
constexpr auto ITEM_NOSEE = 1U << 27;
constexpr auto ITEM_NOREPAIR = 1U << 28;
constexpr auto ITEM_NEWBIE = 1U << 29;
constexpr auto ITEM_PC_CORPSE = 1U << 30;
constexpr auto ITEM_QUEST = 1U << 31;
constexpr auto ITEM_NO_RESTRING = 1U;
constexpr auto ITEM_LIMIT_SACRIFICE = 1U << 1;
constexpr auto ITEM_UNIQUE = 1U << 2;
constexpr auto ITEM_NO_TRADE = 1U << 3;
constexpr auto ITEM_NONOTICE = 1U << 4;
constexpr auto ITEM_NOLOCATE = 1U << 5;
constexpr auto ITEM_UNIQUE_SAVE = 1U << 6;
constexpr auto ITEM_NPC_CORPSE = 1U << 7;
constexpr auto ITEM_PC_CORPSE_LOOTED = 1U << 8;
constexpr auto ITEM_NO_SCRAP = 1U << 9;
constexpr auto ITEM_CUSTOM = 1U << 10;
constexpr auto ITEM_24H_SAVE = 1U << 11;
constexpr auto ITEM_NO_DISARM = 1U << 12;
constexpr auto ITEM_TOGGLE = 1U << 13;
constexpr auto ITEM_NO_CUSTOM = 1U << 14;
constexpr auto ITEM_24H_NO_SELL = 1U << 15;
constexpr auto ITEM_POOF_AFTER_24H = 1U << 16;
constexpr auto ITEM_POOF_NEVER = 1U << 17;
constexpr auto SIZE_ANY = 1U;
constexpr auto SIZE_SMALL = 1U << 1;
constexpr auto SIZE_MEDIUM = 1U << 2;
constexpr auto SIZE_LARGE = 1U << 3;
constexpr auto UTILITY_CATSTINK = 1;
constexpr auto UTILITY_EXIT_TRAP = 2;
constexpr auto UTILITY_MOVEMENT_TRAP = 3;
constexpr auto UTILITY_MORTAR = 4;
constexpr auto UTILITY_ITEM_MAX = 4;
constexpr auto LIQ_WATER = 0;
constexpr auto LIQ_BEER = 1;
constexpr auto LIQ_WINE = 2;
constexpr auto LIQ_ALE = 3;
constexpr auto LIQ_DARKALE = 4;
constexpr auto LIQ_WHISKY = 5;
constexpr auto LIQ_LEMONADE = 6;
constexpr auto LIQ_FIREBRT = 7;
constexpr auto LIQ_LOCALSPC = 8;
constexpr auto LIQ_SLIME = 9;
constexpr auto LIQ_MILK = 10;
constexpr auto LIQ_TEA = 11;
constexpr auto LIQ_COFFEE = 12;
constexpr auto LIQ_BLOOD = 13;
constexpr auto LIQ_SALTWATER = 14;
constexpr auto LIQ_COKE = 15;
constexpr auto LIQ_GATORADE = 16;
constexpr auto LIQ_HOLYWATER = 17;
constexpr auto LIQ_INK = 18;
constexpr auto PLAYER_OBJECT_THIEF = 297UL;
constexpr auto PLAYER_GOLD_THIEF = 298UL;
constexpr auto PLAYER_CANTQUIT = 299UL;
constexpr auto MAX_DIRS = 6;
constexpr auto CLASS_MAX = 13;
constexpr auto SAVE_TYPE_MAX = 5; // Do not change this value.  Used in pfile writing
constexpr auto MAX_INDEX = 6000;
constexpr auto ERROR_PROG = -1;
constexpr auto IN_FILE_PROG = 0;
constexpr auto ACT_PROG = 1;
constexpr auto SPEECH_PROG = 2;
constexpr auto RAND_PROG = 4;
constexpr auto FIGHT_PROG = 8;
constexpr auto DEATH_PROG = 16;
constexpr auto HITPRCNT_PROG = 32;
constexpr auto ENTRY_PROG = 64;
constexpr auto GREET_PROG = 128;
constexpr auto ALL_GREET_PROG = 256;
constexpr auto GIVE_PROG = 512;
constexpr auto BRIBE_PROG = 1024;
constexpr auto CATCH_PROG = 2048;
constexpr auto ATTACK_PROG = 4096;
constexpr auto ARAND_PROG = 8192;
constexpr auto LOAD_PROG = 16384;
constexpr auto COMMAND_PROG = 16384 << 1;
constexpr auto WEAPON_PROG = 16384 << 2;
constexpr auto ARMOUR_PROG = 16384 << 3;
constexpr auto CAN_SEE_PROG = 16384 << 4;
constexpr auto DAMAGE_PROG = 16384 << 5;
constexpr auto MPROG_MAX_TYPE_VALUE = 16384 << 6;
constexpr level_t MORTAL = 60;
constexpr level_t GIFTED_COMMAND = 101; // noone should ever "be" this level
constexpr level_t IMMORTAL = 102;
constexpr level_t ANGEL = 103;
constexpr level_t DEITY = 104;
constexpr level_t OVERSEER = 105;
constexpr level_t DIVINITY = 106;
constexpr level_t COORDINATOR = 108;
constexpr level_t IMPLEMENTER = 110;
constexpr level_t ARCHITECT = ANGEL;
constexpr level_t ARCHANGEL = ANGEL;
constexpr level_t SERAPH = ANGEL;
constexpr level_t PATRON = DEITY;
constexpr level_t POWER = DEITY;
constexpr level_t G_POWER = DEITY;
constexpr level_t MIN_GOD = IMMORTAL;
constexpr level_t PIRAHNA_FAKE_LVL = 102;
constexpr auto AFF_BLIND = 1;
constexpr auto AFF_INVISIBLE = 2;
constexpr auto AFF_DETECT_EVIL = 3;
constexpr auto AFF_DETECT_INVISIBLE = 4;
constexpr auto AFF_DETECT_MAGIC = 5;
constexpr auto AFF_SENSE_LIFE = 6;
constexpr auto AFF_REFLECT = 7;
constexpr auto AFF_SANCTUARY = 8;
constexpr auto AFF_GROUP = 9;
constexpr auto AFF_EAS = 10;
constexpr auto AFF_CURSE = 11;
constexpr auto AFF_FROSTSHIELD = 12;
constexpr auto AFF_POISON = 13;
constexpr auto AFF_PROTECT_EVIL = 14;
constexpr auto AFF_PARALYSIS = 15;
constexpr auto AFF_DETECT_GOOD = 16;
constexpr auto AFF_FIRESHIELD = 17;
constexpr auto AFF_SLEEP = 18;
constexpr auto AFF_TRUE_SIGHT = 19;
constexpr auto AFF_SNEAK = 20;
constexpr auto AFF_HIDE = 21;
constexpr auto AFF_IGNORE_WEAPON_WEIGHT = 22;
constexpr auto AFF_CHARM = 23;
constexpr auto AFF_RAGE = 24;
constexpr auto AFF_SOLIDITY = 25;
constexpr auto AFF_INFRARED = 26;
constexpr auto AFF_CANTQUIT = 27;
constexpr auto AFF_KILLER = 28;
constexpr auto AFF_FLYING = 29;
constexpr auto AFF_LIGHTNINGSHIELD = 30;
constexpr auto AFF_HASTE = 31;
constexpr auto AFF_SHADOWSLIP = 33;
constexpr auto AFF_INSOMNIA = 34;
constexpr auto AFF_FREEFLOAT = 35;
constexpr auto AFF_FARSIGHT = 36;
constexpr auto AFF_CAMOUFLAGUE = 37;
constexpr auto AFF_STABILITY = 38;
constexpr auto AFF_NEWSAVE = 39; // Newsave
constexpr auto AFF_GOLEM = 40;   // This is used for IRON golem, not stone. It differs the two.
constexpr auto AFF_FOREST_MELD = 41;
constexpr auto AFF_INSANE = 42;
constexpr auto AFF_GLITTER_DUST = 43;
constexpr auto AFF_UTILITY = 44;
constexpr auto AFF_ALERT = 45;
constexpr auto AFF_NO_FLEE = 46;
constexpr auto AFF_FAMILIAR = 47;
constexpr auto AFF_PROTECT_GOOD = 48;
constexpr auto AFF_POWERWIELD = 49;   // Innate Powerwield
constexpr auto AFF_REGENERATION = 50; // Innate Regeneration
constexpr auto AFF_FOCUS = 51;        // Innate focus
constexpr auto AFF_ILLUSION = 52;     // Innate illusion
constexpr auto AFF_KNOW_ALIGN = 53;
constexpr auto AFF_BLACKJACK_ALERT = 54; // cannot be blackjacked, or blackjack //no longer used?
constexpr auto AFF_WATER_BREATHING = 55;
constexpr auto AFF_AMBUSH_ALERT = 56; // alert enough even for ambush
constexpr auto AFF_FEARLESS = 57;
constexpr auto AFF_NO_PARA = 58;
constexpr auto AFF_NO_CIRCLE = 59;
constexpr auto AFF_NO_BEHEAD = 60;
constexpr auto AFF_BOUNT_SONNET_HUNGER = 61;
constexpr auto AFF_BOUNT_SONNET_THIRST = 62;
constexpr auto AFF_CMAST_WEAKEN = 63;
constexpr auto AFF_RUSH_CD = 65; // bullrush cooldown
constexpr auto AFF_CRIPPLE = 66;
constexpr auto AFF_CHAMPION = 67;
constexpr auto AFF_BLACKJACK = 68;
constexpr auto AFF_NO_REGEN = 69;
constexpr auto AFF_ACID_SHIELD = 70;
constexpr auto AFF_PRIMAL_FURY = 71;
constexpr auto AFF_ELEMENTAL = 72;
constexpr auto AFF_ITEM_REMOVE = 73;
constexpr auto ASIZE = 32; // don't change unless you want to be screwed
constexpr auto DARK = 1;
constexpr auto NOHOME = 1 << 1;
constexpr auto NO_MOB = 1 << 2;
constexpr auto INDOORS = 1 << 3;
constexpr auto TELEPORT_BLOCK = 1 << 4;
constexpr auto NO_KI = 1 << 5;
constexpr auto NOLEARN = 1 << 6;
constexpr auto NO_MAGIC = 1 << 7;
constexpr auto TUNNEL = 1 << 8;
constexpr auto PRIVATE = 1 << 9;
constexpr auto SAFE = 1 << 10;
constexpr auto NO_SUMMON = 1 << 11;
constexpr auto NO_ASTRAL = 1 << 12; // usused
constexpr auto NO_PORTAL = 1 << 13;
constexpr auto IMP_ONLY = 1 << 14;
constexpr auto FALL_DOWN = 1 << 15;
constexpr auto ARENA = 1 << 16;
constexpr auto QUIET = 1 << 17;
constexpr auto UNSTABLE = 1 << 18;
constexpr auto NO_QUIT = 1 << 19;
constexpr auto FALL_UP = 1 << 20;
constexpr auto FALL_EAST = 1 << 21;
constexpr auto FALL_WEST = 1 << 22;
constexpr auto FALL_SOUTH = 1 << 23;
constexpr auto FALL_NORTH = 1 << 24;
constexpr auto NO_TELEPORT = 1 << 25;
constexpr auto NO_TRACK = 1 << 26;
constexpr auto CLAN_ROOM = 1 << 27;
constexpr auto NO_SCAN = 1 << 28;
constexpr auto NO_WHERE = 1 << 29;
constexpr auto LIGHT_ROOM = 1 << 30;
constexpr auto PASSWORD_LEN = 20;
constexpr auto auction_duration = 1209600UL;
constexpr auto AUC_MIN_PRICE = 1000;
constexpr auto AUC_MAX_PRICE = 2000000000;
constexpr auto SUPPRESS_CONSEQUENCES = 1;
constexpr auto SUPPRESS_MESSAGES = 2;
constexpr auto SUPPRESS_ALL = (SUPPRESS_CONSEQUENCES | SUPPRESS_MESSAGES);
constexpr auto FIND_CHAR_ROOM = 1;
constexpr auto FIND_CHAR_WORLD = 2;
constexpr auto FIND_OBJ_INV = 4;
constexpr auto FIND_OBJ_ROOM = 8;
constexpr auto FIND_OBJ_WORLD = 16;
constexpr auto FIND_OBJ_EQUIP = 32;
constexpr auto READ_SIZE = 256;
constexpr auto MAX_HELP_KEYWORD_LENGTH = 20;
constexpr auto MAX_HELP_RELATED_LENGTH = 60;
constexpr auto MAX_HELP_LENGTH = 8192;
constexpr auto COMBAT_MOD_FRENZY = 1;
constexpr auto COMBAT_MOD_RESIST = 1 << 1;
constexpr auto COMBAT_MOD_SUSCEPT = 1 << 2;
constexpr auto COMBAT_MOD_IGNORE = 1 << 3;
constexpr auto COMBAT_MOD_REDUCED = 1 << 4;
constexpr auto TYPE_CHOOSE = 0;
constexpr auto TYPE_PKILL = 1;
constexpr auto TYPE_RAW_KILL = 2;
constexpr auto TYPE_ARENA_KILL = 3;
constexpr auto KILL_OTHER = 0;
constexpr auto KILL_DROWN = 1;
constexpr auto KILL_FALL = 2;
constexpr auto KILL_POISON = 3;
constexpr auto KILL_SUICIDE = 4;
constexpr auto KILL_POTATO = 5;
constexpr auto KILL_MASHED = 6;
constexpr auto KILL_BINGO = 7;
constexpr auto KILL_BATTER = 8;
constexpr auto KILL_MORTAR = 9;
constexpr auto COMBAT_SHOCKED = 1;
constexpr auto COMBAT_BASH1 = 1 << 1;
constexpr auto COMBAT_BASH2 = 1 << 2;
constexpr auto COMBAT_STUNNED = 1 << 3;
constexpr auto COMBAT_STUNNED2 = 1 << 4;
constexpr auto COMBAT_CIRCLE = 1 << 5;
constexpr auto COMBAT_BERSERK = 1 << 6;
constexpr auto COMBAT_HITALL = 1 << 7;
constexpr auto COMBAT_RAGE1 = 1 << 8;
constexpr auto COMBAT_RAGE2 = 1 << 9;
constexpr auto COMBAT_BLADESHIELD1 = 1 << 10;
constexpr auto COMBAT_BLADESHIELD2 = 1 << 11;
constexpr auto COMBAT_REPELANCE = 1 << 12;
constexpr auto COMBAT_VITAL_STRIKE = 1 << 13;
constexpr auto COMBAT_MONK_STANCE = 1 << 14;
constexpr auto COMBAT_MISS_AN_ATTACK = 1 << 15;
constexpr auto COMBAT_ORC_BLOODLUST1 = 1 << 16;
constexpr auto COMBAT_ORC_BLOODLUST2 = 1 << 17;
constexpr auto COMBAT_THI_EYEGOUGE = 1 << 18;
constexpr auto COMBAT_THI_EYEGOUGE2 = 1 << 19;
constexpr auto COMBAT_FLEEING = 1 << 20;
constexpr auto COMBAT_SHOCKED2 = 1 << 21;
constexpr auto COMBAT_CRUSH_BLOW = 1 << 22;
constexpr auto COMBAT_ATTACKER = 1 << 23;
constexpr auto COMBAT_CRUSH_BLOW2 = 1 << 24;
constexpr auto DAMAGE_TYPE_PHYSICAL = 0;
constexpr auto DAMAGE_TYPE_MAGIC = 1;
constexpr auto DAMAGE_TYPE_SONG = 2;
constexpr auto MAX_RAW_INPUT_LENGTH = 512;
constexpr auto TONGUE_COMMON = 0;
constexpr auto TONGUE_HUMAN = 1;
constexpr auto TONGUE_ELVISH = 2;
constexpr auto TONGUE_DWARVEN = 3;
constexpr auto TONGUE_HALFLING = 4;
constexpr auto TONGUE_BROWNIE = 5;
constexpr auto TONGUE_GIANT = 6;
constexpr auto TONGUE_GNOMISH = 7;
constexpr auto TONGUE_DROW = 8;
constexpr auto TONGUE_ORCISH = 9;
constexpr auto TONGUE_DRAGON = 10;
constexpr auto TONGUE_ANIMAL = 11;
constexpr auto TONGUE_FLORA = 12;
constexpr auto TONGUE_PLANAR = 13;
constexpr auto TONGUE_DEMON = 14;
constexpr auto TONGUE_DEITY = 15;
constexpr auto ANY_BOARD = 0;
constexpr auto CLASS_BOARD = 1;
constexpr auto CLAN_BOARD = 2;
constexpr auto NO_OWNER = -1;
constexpr auto CLAN_ULNHYRR = 1;
constexpr auto CLAN_DARKTIDE = 2;
constexpr auto CLAN_ARCANA = 3;
constexpr auto CLAN_DARKENED = 4;
constexpr auto CLAN_DCGUARD = 5;
constexpr auto CLAN_TIMEWARP = 6;
constexpr auto CLAN_CONTINUUM = 7;
constexpr auto CLAN_MERC = 8;
constexpr auto CLAN_NAZGUL = 9;
constexpr auto CLAN_BLACKAXE = 10;
constexpr auto CLAN_TRIAD = 11;
constexpr auto CLAN_KOBAL = 12;
constexpr auto CLAN_SLACKERS = 13;
constexpr auto CLAN_KEHUA = 14;
constexpr auto CLAN_ASKANI = 15;
constexpr auto CLAN_HOUSELESSROGUES = 16;
constexpr auto CLAN_THEHORDE = 17;
constexpr auto CLAN_ANARCHIST = 18;
constexpr auto CLAN_SOLARIS = 19;
constexpr auto CLAN_SINDICATE = 20;
/* Our own constants */
constexpr auto WEAR_LIGHT = 0;
constexpr auto WEAR_FINGER_R = 1;
constexpr auto WEAR_FINGER_L = 2;
constexpr auto WEAR_NECK_1 = 3;
constexpr auto WEAR_NECK_2 = 4;
constexpr auto WEAR_BODY = 5;
constexpr auto WEAR_HEAD = 6;
constexpr auto WEAR_LEGS = 7;
constexpr auto WEAR_FEET = 8;
constexpr auto WEAR_HANDS = 9;
constexpr auto WEAR_ARMS = 10;
constexpr auto WEAR_SHIELD = 11;
constexpr auto WEAR_ABOUT = 12;
constexpr auto WEAR_WAISTE = 13;
constexpr auto WEAR_WRIST_R = 14;
constexpr auto WEAR_WRIST_L = 15;
constexpr auto WEAR_WIELD = 16;
constexpr auto WEAR_SECOND_WIELD = 17;
constexpr auto WEAR_HOLD = 18;
constexpr auto WEAR_HOLD2 = 19;
constexpr auto WEAR_FACE = 20;
constexpr auto WEAR_EAR_L = 21;
constexpr auto WEAR_EAR_R = 22;
// #define WEAR_MAX        22
constexpr qint32 ANSI = 1 << 0;  /* If it's an ansi code */
constexpr qint32 VT100 = 1 << 1; /* If it's a vt100 code */
constexpr qint32 CODE = 1 << 2;  /* If it should be interped */
constexpr qint32 TEXT = 1 << 3;  /* If it's just text */
constexpr qint32 _MAX_STR = 2048;

constexpr auto DRUNK = 0;
constexpr auto FULL = 1;
constexpr auto THIRST = 2;

/*  For cut and paste purposes
   switch(GET_CLASS(mob))
   {
      case CLASS_MAGIC_USER:
      case CLASS_CLERIC:
      case CLASS_THIEF:
      case CLASS_WARRIOR:
      case CLASS_ANTI_PAL:
      case CLASS_PALADIN:
      case CLASS_BARBARIAN:
      case CLASS_MONK:
      case CLASS_RANGER:
      case CLASS_BARD:
      case CLASS_DRUID:
      case CLASS_PSIONIC:
      case CLASS_NECROMANCER:
      default:
         break;
   }

*/
/************************************************************************
| These should not be here - in fact, some of them should not exist.  We're
|   leaving them here for compatibility until we can get rid of them.
|   Morc XXX
*/
constexpr auto APPLY_NONE = 0;
constexpr auto APPLY_STR = 1;
constexpr auto APPLY_DEX = 2;
constexpr auto APPLY_INT = 3;
constexpr auto APPLY_WIS = 4;
constexpr auto APPLY_CON = 5;
constexpr auto APPLY_SEX = 6;
constexpr auto APPLY_CLASS = 7;
constexpr auto APPLY_LEVEL = 8;
constexpr auto APPLY_AGE = 9;
constexpr auto APPLY_CHAR_WEIGHT = 10;
constexpr auto APPLY_CHAR_HEIGHT = 11;
constexpr auto APPLY_MANA = 12;
constexpr auto APPLY_HIT = 13;
constexpr auto APPLY_MOVE = 14;
constexpr auto APPLY_GOLD = 15;
constexpr auto APPLY_EXP = 16;
constexpr auto APPLY_AC = 17;
constexpr auto APPLY_ARMOR = 17;
constexpr auto APPLY_HITROLL = 18;
constexpr auto APPLY_DAMROLL = 19;
constexpr auto APPLY_SAVING_FIRE = 20;
constexpr auto APPLY_SAVING_COLD = 21;
constexpr auto APPLY_SAVING_ENERGY = 22;
constexpr auto APPLY_SAVING_ACID = 23;
constexpr auto APPLY_SAVING_MAGIC = 24;
constexpr auto APPLY_SAVING_POISON = 25;
constexpr auto APPLY_HIT_N_DAM = 26;
constexpr auto APPLY_SANCTUARY = 27;
constexpr auto APPLY_SENSE_LIFE = 28;
constexpr auto APPLY_DETECT_INVIS = 29;
constexpr auto APPLY_INVISIBLE = 30;
constexpr auto APPLY_SNEAK = 31;
constexpr auto APPLY_INFRARED = 32;
constexpr auto APPLY_HASTE = 33;
constexpr auto APPLY_PROTECT_EVIL = 34;
constexpr auto APPLY_FLY = 35;
constexpr auto WEP_MAGIC_MISSILE = 36;
constexpr auto WEP_BLIND = 37;
constexpr auto WEP_EARTHQUAKE = 38;
constexpr auto WEP_CURSE = 39;
constexpr auto WEP_COLOUR_SPRAY = 40;
constexpr auto WEP_DISPEL_EVIL = 41;
constexpr auto WEP_ENERGY_DRAIN = 42;
constexpr auto WEP_FIREBALL = 43;
constexpr auto WEP_LIGHTNING_BOLT = 44;
constexpr auto WEP_HARM = 45;
constexpr auto WEP_POISON = 46;
constexpr auto WEP_SLEEP = 47;
constexpr auto WEP_FEAR = 48;
constexpr auto WEP_DISPEL_MAGIC = 49;
constexpr auto WEP_WEAKEN = 50;
constexpr auto WEP_CAUSE_LIGHT = 51;
constexpr auto WEP_CAUSE_CRITICAL = 52;
constexpr auto WEP_PARALYZE = 53;
constexpr auto WEP_ACID_BLAST = 54;
constexpr auto WEP_BEE_STING = 55;
constexpr auto WEP_CURE_LIGHT = 56;
constexpr auto WEP_FLAMESTRIKE = 57;
constexpr auto WEP_HEAL_SPRAY = 58;
constexpr auto WEP_DROWN = 59;
constexpr auto WEP_HOWL = 60;
constexpr auto WEP_SOULDRAIN = 61;
constexpr auto WEP_SPARKS = 62;
constexpr auto APPLY_BARKSKIN = 63;
constexpr auto APPLY_RESIST_FIRE = 64;
constexpr auto APPLY_RESIST_COLD = 65;
constexpr auto APPLY_KI = 66;
constexpr auto APPLY_CAMOUFLAGE = 67;
constexpr auto APPLY_FARSIGHT = 68;
constexpr auto APPLY_FREEFLOAT = 69;
constexpr auto APPLY_FROSTSHIELD = 70;
constexpr auto APPLY_INSOMNIA = 71;
constexpr auto APPLY_LIGHTNING_SHIELD = 72;
constexpr auto APPLY_REFLECT = 73;
constexpr auto APPLY_RESIST_ELECTRIC = 74;
constexpr auto APPLY_SHADOWSLIP = 75;
constexpr auto APPLY_SOLIDITY = 76;
constexpr auto APPLY_STABILITY = 77;
constexpr auto APPLY_STAUNCHBLOOD = 78;
constexpr auto WEP_DISPEL_GOOD = 79;
constexpr auto WEP_TELEPORT = 80;
constexpr auto WEP_CHILL_TOUCH = 81;
constexpr auto WEP_POWER_HARM = 82;
constexpr auto WEP_VAMPIRIC_TOUCH = 83;
constexpr auto WEP_LIFE_LEECH = 84;
constexpr auto WEP_METEOR_SWARM = 85;
constexpr auto WEP_ENTANGLE = 86;
constexpr auto APPLY_INSANE_CHANT = 87;
constexpr auto APPLY_GLITTER_DUST = 88;
constexpr auto APPLY_RESIST_ACID = 89;
constexpr auto APPLY_HP_REGEN = 90;
constexpr auto APPLY_MANA_REGEN = 91;
constexpr auto APPLY_MOVE_REGEN = 92;
constexpr auto APPLY_KI_REGEN = 93;
constexpr auto WEP_CREATE_FOOD = 94;
constexpr auto APPLY_DAMAGED = 95;
constexpr auto WEP_THIEF_POISON = 96;
constexpr auto APPLY_PROTECT_GOOD = 97;
constexpr auto APPLY_MELEE_DAMAGE = 98;
constexpr auto APPLY_SPELL_DAMAGE = 99;
constexpr auto APPLY_SONG_DAMAGE = 100;
constexpr auto APPLY_RESIST_MAGIC = 101;
constexpr auto APPLY_SAVES = 102;
constexpr auto APPLY_SPELLDAMAGE = 103;
constexpr auto APPLY_BOUNT_SONNET_HUNGER = 104;
constexpr auto APPLY_BOUNT_SONNET_THIRST = 105;
constexpr auto APPLY_BLIND = 106;
constexpr auto APPLY_WATER_BREATHING = 107;
constexpr auto APPLY_DETECT_MAGIC = 108;
constexpr auto WEP_WILD_MAGIC = 109;
constexpr auto APPLY_MAXIMUM_VALUE = 109;

/*
 1000+ are reserved, so if you were thinking about using, think
 again.
*/
/* RESERVED: 100-150 for more weapon affects */
/* Morc XXX */

// different stat combos for skill groups
constexpr auto STRDEX = 1;
constexpr auto STRCON = 2;
constexpr auto STRINT = 3;
constexpr auto STRWIS = 4;
constexpr auto DEXCON = 5;
constexpr auto DEXINT = 6;
constexpr auto DEXWIS = 7;
constexpr auto CONINT = 8;
constexpr auto CONWIS = 9;
constexpr auto INTWIS = 10;

constexpr auto MAX_PROFESSIONS = 2;
// End defines for gradual skill increase code

static const qint32 COREDUMP_MAX = 10;
// Defines for gradual skill increase code
// Usage is defined in guild.cpp
constexpr auto SKILL_INCREASE_EASY = 100;
constexpr auto SKILL_INCREASE_MEDIUM = 200;
constexpr auto SKILL_INCREASE_HARD = 300;

constexpr auto SILENCE_OBJ_NUMBER = 407;
constexpr auto SPIRIT_SHIELD_OBJ_NUMBER = 408;
constexpr auto CONSECRATE_OBJ_NUMBER = 409;
constexpr auto CONSECRATE_COMP_OBJ_NUMBER = 3094;
constexpr auto DESECRATE_COMP_OBJ_NUMBER = 303;

constexpr auto WORLD_MAX_ROOM = 50000;
constexpr auto VERSION_NUMBER = 2; /* used for changing pfile format */
constexpr auto MAX_BUF_LENGTH = 240;
constexpr auto CONT_CLOSEABLE = 1;
constexpr auto CONT_PICKPROOF = 2;
constexpr auto CONT_CLOSED = 4;
constexpr auto CONT_LOCKED = 8;

constexpr auto OBJ_NOTIMER = -7000000;

constexpr auto CURRENT_OBJ_VERSION = 1;

// functions from objects.cpp
constexpr auto PUNISH_SILENCED = 1;
constexpr auto PUNISH_NOEMOTE = 1 << 1;
constexpr auto PUNISH_LOG = 1 << 2;
constexpr auto PUNISH_FREEZE = 1 << 3;
constexpr auto PUNISH_DENY = 1 << 4;
constexpr auto PUNISH_UNUSED = 1 << 5;
constexpr auto PUNISH_NONAME = 1 << 6;
constexpr auto PUNISH_SPAMMER = 1 << 7;
constexpr auto PUNISH_STUPID = 1 << 8;
constexpr auto PUNISH_NOARENA = 1 << 9;
constexpr auto PUNISH_NOTITLE = 1 << 10;
constexpr auto PUNISH_UNLUCKY = 1 << 11;
constexpr auto PUNISH_NOTELL = 1 << 12;
constexpr auto PUNISH_NOPRAY = 1 << 13;

constexpr auto RACE_NONE = 0;
constexpr auto RACE_HUMAN = 1;
constexpr auto RACE_ELVEN = 2;
constexpr auto RACE_DWARVEN = 3;
constexpr auto RACE_HOBBIT = 4;
constexpr auto RACE_PIXIE = 5;
constexpr auto RACE_GIANT = 6;
constexpr auto RACE_GNOME = 7;
constexpr auto RACE_ORC = 8;
constexpr auto RACE_TROLL = 9;

constexpr auto MAX_PC_RACE = 9;
/* Not player races from here down */
constexpr auto RACE_GOBLIN = 10;
constexpr auto RACE_REPTILE = 11;
constexpr auto RACE_DRAGON = 12;
constexpr auto RACE_SNAKE = 13;
constexpr auto RACE_HORSE = 14;
constexpr auto RACE_BIRD = 15;
constexpr auto RACE_RODENT = 16;
constexpr auto RACE_FISH = 17;
constexpr auto RACE_ARACHNID = 18;
constexpr auto RACE_INSECT = 19;
constexpr auto RACE_SLIME = 20;
constexpr auto RACE_ANIMAL = 21;
constexpr auto RACE_TREE = 22;
constexpr auto RACE_ENFAN = 23;
constexpr auto RACE_UNDEAD = 24;
constexpr auto RACE_GHOST = 25;
constexpr auto RACE_GOLEM = 26;
constexpr auto RACE_ELEMENT = 27;
constexpr auto RACE_PLANAR = 28;
constexpr auto RACE_DEMON = 29;
constexpr auto RACE_YRNALI = 30;
constexpr auto RACE_IMMORTAL = 31;
constexpr auto RACE_FELINE = 32;
constexpr auto MAX_RACE = 32;

/* Bitvectors for racial shit */
constexpr auto BITV_HUMAN = 1;
constexpr auto BITV_ELVEN = 1 << 1;
constexpr auto BITV_DWARVEN = 1 << 2;
constexpr auto BITV_HOBBIT = 1 << 3;
constexpr auto BITV_PIXIE = 1 << 4;
constexpr auto BITV_GIANT = 1 << 5;
constexpr auto BITV_GNOME = 1 << 6;
constexpr auto BITV_ORC = 1 << 7;
constexpr auto BITV_TROLL = 1 << 8;
constexpr auto BITV_GOBLIN = 1 << 9;
constexpr auto BITV_REPTILE = 1 << 10;
constexpr auto BITV_DRAGON = 1 << 11;
constexpr auto BITV_SNAKE = 1 << 12;
constexpr auto BITV_HORSE = 1 << 13;
constexpr auto BITV_BIRD = 1 << 14;
constexpr auto BITV_RODENT = 1 << 15;
constexpr auto BITV_FISH = 1 << 16;
constexpr auto BITV_ARACHNID = 1 << 17;
constexpr auto BITV_INSECT = 1 << 18;
constexpr auto BITV_SLIME = 1 << 19;
constexpr auto BITV_ANIMAL = 1 << 20;
constexpr auto BITV_TREE = 1 << 21;
constexpr auto BITV_ENFAN = 1 << 22;
constexpr auto BITV_UNDEAD = 1 << 23;
constexpr auto BITV_GHOST = 1 << 24;
constexpr auto BITV_GOLEM = 1 << 25;
constexpr auto BITV_ELEMENT = 1 << 26;
constexpr auto BITV_PLANAR = 1 << 27;
constexpr auto BITV_DEMON = 1 << 28;
constexpr auto BITV_YRNALI = 1 << 29;
constexpr auto BITV_IMMORTAL = 1 << 30;

// Following are modifiers from the base maximum stat for a race
constexpr auto BASE_MAX_STAT = 27;
// Modifiers below only happen on creation and has little influence
// On actual maxes

constexpr auto RACE_HUMAN_STR_MOD = 0;
constexpr auto RACE_HUMAN_DEX_MOD = 0;
constexpr auto RACE_HUMAN_INT_MOD = 0;
constexpr auto RACE_HUMAN_WIS_MOD = 0;
constexpr auto RACE_HUMAN_CON_MOD = 0;

constexpr auto RACE_ELVEN_STR_MOD = -1;
constexpr auto RACE_ELVEN_DEX_MOD = 1;
constexpr auto RACE_ELVEN_INT_MOD = 1;
constexpr auto RACE_ELVEN_WIS_MOD = 0;
constexpr auto RACE_ELVEN_CON_MOD = -1;

constexpr auto RACE_DWARVEN_STR_MOD = 2;
constexpr auto RACE_DWARVEN_DEX_MOD = -2;
constexpr auto RACE_DWARVEN_WIS_MOD = 1;
constexpr auto RACE_DWARVEN_INT_MOD = -2;
constexpr auto RACE_DWARVEN_CON_MOD = 1;

constexpr auto RACE_HOBBIT_STR_MOD = -2;
constexpr auto RACE_HOBBIT_DEX_MOD = 3;
constexpr auto RACE_HOBBIT_CON_MOD = -1;
constexpr auto RACE_HOBBIT_INT_MOD = 0;
constexpr auto RACE_HOBBIT_WIS_MOD = 0;

constexpr auto RACE_PIXIE_STR_MOD = -4;
constexpr auto RACE_PIXIE_CON_MOD = -2;
constexpr auto RACE_PIXIE_WIS_MOD = 1;
constexpr auto RACE_PIXIE_INT_MOD = 3;
constexpr auto RACE_PIXIE_DEX_MOD = 2;

constexpr auto RACE_GIANT_STR_MOD = 3;
constexpr auto RACE_GIANT_DEX_MOD = -2;
constexpr auto RACE_GIANT_CON_MOD = 1;
constexpr auto RACE_GIANT_WIS_MOD = 0;
constexpr auto RACE_GIANT_INT_MOD = -2;

constexpr auto RACE_GNOME_STR_MOD = -2;
constexpr auto RACE_GNOME_DEX_MOD = -2;
constexpr auto RACE_GNOME_INT_MOD = 1;
constexpr auto RACE_GNOME_WIS_MOD = 3;
constexpr auto RACE_GNOME_CON_MOD = 0;

constexpr auto RACE_ORC_STR_MOD = 1;
constexpr auto RACE_ORC_DEX_MOD = 0;
constexpr auto RACE_ORC_INT_MOD = 0;
constexpr auto RACE_ORC_WIS_MOD = -2;
constexpr auto RACE_ORC_CON_MOD = 1;

constexpr auto RACE_TROLL_STR_MOD = 2;
constexpr auto RACE_TROLL_DEX_MOD = 0;
constexpr auto RACE_TROLL_CON_MOD = 3;
constexpr auto RACE_TROLL_WIS_MOD = -2;
constexpr auto RACE_TROLL_INT_MOD = -3;

//       Save modifications dependant upon race
constexpr auto RACE_HUMAN_FIRE_MOD = 0;
constexpr auto RACE_HUMAN_COLD_MOD = 0;
constexpr auto RACE_HUMAN_ENERGY_MOD = 0;
constexpr auto RACE_HUMAN_ACID_MOD = 0;
constexpr auto RACE_HUMAN_MAGIC_MOD = 0;
constexpr auto RACE_HUMAN_POISON_MOD = 0;

constexpr auto RACE_TROLL_FIRE_MOD = -6;
constexpr auto RACE_TROLL_COLD_MOD = 0;
constexpr auto RACE_TROLL_ENERGY_MOD = 3;
constexpr auto RACE_TROLL_ACID_MOD = -3;
constexpr auto RACE_TROLL_MAGIC_MOD = 0;
constexpr auto RACE_TROLL_POISON_MOD = 6;

constexpr auto RACE_ELVEN_FIRE_MOD = 0;
constexpr auto RACE_ELVEN_COLD_MOD = 0;
constexpr auto RACE_ELVEN_ENERGY_MOD = 3;
constexpr auto RACE_ELVEN_ACID_MOD = -3;
constexpr auto RACE_ELVEN_MAGIC_MOD = 3;
constexpr auto RACE_ELVEN_POISON_MOD = -3;

constexpr auto RACE_DWARVEN_FIRE_MOD = -3;
constexpr auto RACE_DWARVEN_COLD_MOD = -3;
constexpr auto RACE_DWARVEN_ENERGY_MOD = 0;
constexpr auto RACE_DWARVEN_ACID_MOD = 3;
constexpr auto RACE_DWARVEN_MAGIC_MOD = 0;
constexpr auto RACE_DWARVEN_POISON_MOD = 3;

constexpr auto RACE_GIANT_FIRE_MOD = 3;
constexpr auto RACE_GIANT_COLD_MOD = 6;
constexpr auto RACE_GIANT_ENERGY_MOD = -3;
constexpr auto RACE_GIANT_ACID_MOD = 0;
constexpr auto RACE_GIANT_MAGIC_MOD = -6;
constexpr auto RACE_GIANT_POISON_MOD = 0;

constexpr auto RACE_PIXIE_FIRE_MOD = -3;
constexpr auto RACE_PIXIE_COLD_MOD = 0;
constexpr auto RACE_PIXIE_ENERGY_MOD = 3;
constexpr auto RACE_PIXIE_ACID_MOD = 0;
constexpr auto RACE_PIXIE_MAGIC_MOD = 6;
constexpr auto RACE_PIXIE_POISON_MOD = -6;

constexpr auto RACE_HOBBIT_FIRE_MOD = 6;
constexpr auto RACE_HOBBIT_COLD_MOD = -6;
constexpr auto RACE_HOBBIT_ENERGY_MOD = 0;
constexpr auto RACE_HOBBIT_ACID_MOD = 0;
constexpr auto RACE_HOBBIT_MAGIC_MOD = -3;
constexpr auto RACE_HOBBIT_POISON_MOD = 3;

constexpr auto RACE_GNOME_FIRE_MOD = 0;
constexpr auto RACE_GNOME_COLD_MOD = 0;
constexpr auto RACE_GNOME_ENERGY_MOD = -3;
constexpr auto RACE_GNOME_ACID_MOD = 3;
constexpr auto RACE_GNOME_MAGIC_MOD = 3;
constexpr auto RACE_GNOME_POISON_MOD = -3;

constexpr auto RACE_ORC_FIRE_MOD = 3;
constexpr auto RACE_ORC_COLD_MOD = 3;
constexpr auto RACE_ORC_ENERGY_MOD = -3;
constexpr auto RACE_ORC_ACID_MOD = 0;
constexpr auto RACE_ORC_MAGIC_MOD = -6;
constexpr auto RACE_ORC_POISON_MOD = 0;
constexpr auto SOCIAL_FALSE = 0;
constexpr auto SOCIAL_TRUE = 1;
constexpr auto SOCIAL_TRUE_WITH_NOISE = 2;
constexpr auto BASE_SETS = 1400;
constexpr auto SET_SAIYAN = 0;
constexpr auto SET_VESTMENTS = 1;
constexpr auto SET_HUNTERS = 2;
constexpr auto SET_CAPTAINS = 3;
constexpr auto SET_CELEBRANTS = 4;
constexpr auto SET_RAGER = 5;
constexpr auto SET_FIELDPLATE = 6;
constexpr auto SET_MOAD = 7;
constexpr auto SET_FERAL = 8;
constexpr auto SET_WHITECRYSTAL = 9;
constexpr auto SET_BLACKCRYSTAL = 10;
constexpr auto SET_AQUA = 11;
constexpr auto SET_APPARATUS = 12;
constexpr auto SET_TITANIC = 13;
constexpr auto SET_MOSS = 14;
constexpr auto SET_BLACKSTEEL = 15;
constexpr auto SET_MOAD2 = 16;
constexpr auto SET_RAGER2 = 17;
constexpr auto SET_TRAPPINGS = 18;
constexpr auto SET_FINERY = 19;
constexpr auto SET_MAX = 1419;
constexpr auto MAX_SHOP = 70;
constexpr auto MAX_TRADE = 5;
constexpr auto PC_SHOP_OWNER_SIZE = 20;
constexpr auto PC_SHOP_SELL_MESS_SIZE = 120;
constexpr auto PLAYER_SHOP_KEEPER = 23000;
constexpr auto BARD_MAX_RATING = 3;
constexpr auto MAX_CLAN_LEN = 15;
constexpr auto CLAN_RIGHTS_ACCEPT = 1;
constexpr auto CLAN_RIGHTS_OUTCAST = 1 << 1;
constexpr auto CLAN_RIGHTS_B_READ = 1 << 2;
constexpr auto CLAN_RIGHTS_B_WRITE = 1 << 3;
constexpr auto CLAN_RIGHTS_B_REMOVE = 1 << 4;
constexpr auto CLAN_RIGHTS_MEMBER_LIST = 1 << 5;
constexpr auto CLAN_RIGHTS_RIGHTS = 1 << 6;
constexpr auto CLAN_RIGHTS_MESSAGES = 1 << 7;
constexpr auto CLAN_RIGHTS_INFO = 1 << 8;
constexpr auto CLAN_RIGHTS_TAX = 1 << 9;
constexpr auto CLAN_RIGHTS_WITHDRAW = 1 << 10;
constexpr auto CLAN_RIGHTS_CHANNEL = 1 << 11;
constexpr auto CLAN_RIGHTS_AREA = 1 << 12;
constexpr auto CLAN_RIGHTS_VAULT = 1 << 13;
constexpr auto CLAN_RIGHTS_VAULTLOG = 1 << 14;
constexpr auto CLAN_RIGHTS_LOG = 1 << 15;
constexpr auto MAX_GOLEMS = 2;           // amount of golems above +1
constexpr auto START_ROOM = 3001;        // Where you login
constexpr auto CFLAG_HOME = 3014;        // Where the champion flag normally rests
constexpr auto SECOND_START_ROOM = 3059; // Where you go if killed in start room
constexpr auto FARREACH_START_ROOM = 17868;
constexpr auto THALOS_START_ROOM = 5317;
constexpr auto DESC_LENGTH = 80;
constexpr auto CHAR_VERSION = -4;
constexpr auto MAX_NAME_LENGTH = 12;
constexpr auto CHAMPION_ITEM = 45;
constexpr auto VAULT_UPGRADE_COST = 100;       // plats
constexpr auto VAULT_BASE_SIZE = 10;           // weight
constexpr auto VAULT_MAX_SIZE = 10000;         // weight
constexpr auto VAULT_MAX_DEPWITH = 2000000000; // 2 bil max to add/remove from bank at a time
constexpr auto SUN_DARK = 0;
constexpr auto SUN_RISE = 1;
constexpr auto SUN_LIGHT = 2;
constexpr auto SUN_SET = 3;
constexpr auto SKY_CLOUDLESS = 0;
constexpr auto SKY_CLOUDY = 1;
constexpr auto SKY_RAINING = 2;
constexpr auto SKY_HEAVY_RAIN = 3;
constexpr auto SKY_LIGHTNING = 4;
constexpr auto SKY_SNOWING = 6; // unused
constexpr auto SMALL_BUFSIZE = 1024;
constexpr auto LARGE_BUFSIZE = 24 * 2048;
constexpr auto GARBAGE_SPACE = 32;
constexpr auto NUM_RESERVED_DESCS = 8;
constexpr auto HOST_LENGTH = 30;
constexpr auto REAL = 0;
constexpr auto VIRTUAL = 1;
constexpr auto WORLD_FILE_MODIFIED = 1;
constexpr auto WORLD_FILE_IN_PROGRESS = 1 << 1;
constexpr auto WORLD_FILE_READY = 1 << 2;
constexpr auto WORLD_FILE_APPROVED = 1 << 3;

const auto CORPSE_FILE = u"corpse.save"_s;
const auto SAVE_DIR = u"../save"_s;
const auto BSAVE_DIR = u"../bsave"_s;
const auto QSAVE_DIR = u"../save/qdata"_s;
const auto NEWSAVE_DIR = u"../newsave"_s;
const auto ARCHIVE_DIR = u"../archive"_s;
const auto MOB_DIR = u"../MOBProgs/"_s;
const auto BAN_FILE = u"banned.txt"_s;
const auto SHOP_DIR = u"../lib/shops"_s;
const auto PLAYER_SHOP_DIR = u"../lib/playershops"_s;
const auto FORBIDDEN_NAME_FILE = u"../lib/forbidden_names.txt"_s;
const auto SKILL_QUEST_FILE = u"../lib/skill_quests.txt"_s;
const auto FAMILIAR_DIR = u"../familiar"_s;
const auto FOLLOWER_DIR = u"../follower"_s;
const auto VAULT_DIR = u"../vaults"_s;
const auto SHOP_FILE = u"tinyworld.shp"_s;
const auto WEBPAGE_FILE = u"webresponse.txt"_s;
const auto GREETINGS1_FILE = u"greetings1.txt"_s;
const auto GREETINGS2_FILE = u"greetings3.txt"_s;
const auto GREETINGS3_FILE = u"greetings4.txt"_s;
const auto GREETINGS4_FILE = u"greetings5.txt"_s;
const auto CREDITS_FILE = u"credits.txt"_s;
const auto MOTD_FILE = u"../lib/motd.txt"_s;
const auto IMOTD_FILE = u"motdimm.txt"_s;
const auto STORY_FILE = u"story.txt"_s;
const auto TIME_FILE = u"time.txt"_s;
const auto IDEA_LOG = u"ideas.log"_s;
const auto TYPO_LOG = u"typos.log"_s;
const auto MESS_FILE = u"messages.txt"_s;
const auto MESS2_FILE = u"messages2.txt"_s;
const auto SOCIAL_FILE = u"social.txt"_s;
const auto HELP_KWRD_FILE = u"help_key.txt"_s;
const auto HELP_PAGE_FILE = u"help.txt"_s;
const auto INFO_FILE = u"info.txt"_s;
const auto LOCAL_WHO_FILE = u"onlinewho.txt"_s;
const auto WEB_WHO_FILE = u"/srv/www/www.dcastle.org/htdocs/onlinewho.txt"_s;
const auto WEB_AUCTION_FILE = u"/srv/www/www.dcastle.org/htdocs/auctions.txt"_s;
const auto NEW_HELP_FILE = u"new_help.txt"_s;
const auto WEB_HELP_FILE = u"/srv/www/www.dcastle.org/htdocs/webhelp.txt"_s;
const auto NEW_HELP_PAGE_FILE = u"new_help_screen.txt"_s;
const auto NEW_IHELP_PAGE_FILE = u"new_ihelp_screen.txt"_s;
const auto LEADERBOARD_FILE = u"leaderboard.txt"_s;
const auto QUEST_FILE = u"quests.txt"_s;
const auto WEBCLANSLIST_FILE = u"webclanslist.txt"_s;
const auto HTDOCS_DIR = u"/srv/www/www.dcastle.org/htdocs/"_s;
const auto PLAYER_DIR = u"player/"_s;
const auto BUG_LOG = u"bug.log"_s;
const auto GOD_LOG = u"god.log"_s;
const auto MORTAL_LOG = u"mortal.log"_s;
const auto SOCKET_LOG = u"socket.log"_s;
const auto PLAYER_LOG = u"player.log"_s;
const auto WORLD_LOG = u"world.log"_s;
const auto ARENA_LOG = u"arena.log"_s;
const auto CLAN_LOG = u"clan.log"_s;
const auto OBJECTS_LOG = u"objects.log"_s;
const auto QUEST_LOG = u"quest.log"_s;
const auto VAULT_LOG = u"vault.log"_s;
const auto WORLD_INDEX_FILE = u"worldindex"_s;
const auto OBJECT_INDEX_FILE = u"objectindex"_s;
const auto MOB_INDEX_FILE = u"mobindex"_s;
const auto ZONE_INDEX_FILE = u"zoneindex"_s;
const auto PLAYER_SHOP_INDEX = u"playershopindex"_s;
const auto OBJECT_INDEX_FILE_TINY = u"objectindex.tiny"_s;
const auto WORLD_INDEX_FILE_TINY = u"worldindex.tiny"_s;
const auto MOB_INDEX_FILE_TINY = u"mobindex.tiny"_s;
const auto ZONE_INDEX_FILE_TINY = u"zoneindex.tiny"_s;
const auto VAULT_INDEX_FILE = u"../vaults/vaultindex"_s;
const auto VAULT_INDEX_FILE_TMP = u"../vaults/vaultindex.tmp"_s;
const auto BLACK = "[30m"_ba;
const auto RED = "[31m"_ba;
const auto GREEN = "[32m"_ba;
const auto YELLOW = "[33m"_ba;
const auto BLUE = "[34m"_ba;
const auto PURPLE = "[35m"_ba;
const auto CYAN = "[36m"_ba;
const auto GREY = "[37m"_ba;
const auto EEEE = "#8"_ba;    /* Turns screen to EEEEs */
const auto CLRSCR = "[2j"_ba; /* Clear screen          */
const auto CLREOL = "["_ba;   /* Clear to end of line  */
const auto UPARR = "[A"_ba;
const auto DOWNARR = "[B"_ba;
const auto RIGHTARR = "[C"_ba;
const auto LEFTARR = "[D"_ba;
const auto HOMEPOS = "[H"_ba;
const auto FLASH = "[4m"_ba;
const auto BLINK = "[5m"_ba;
const auto BOLD = "[1m"_ba;
const auto INVERSE = "[7m"_ba;
const auto NTEXT = "[0m[37m"_ba; /* Makes it normal */

constexpr auto ROOM_ETHEREAL_FOCUS = 1;
constexpr auto TEMP_ROOM_FLAG_AVAILABLE = 1 << 1;
constexpr auto iNO_TRACK = 1;
constexpr auto iNO_MAGIC = 1 << 1;
constexpr auto NORTH = 0;
constexpr auto EAST = 1;
constexpr auto SOUTH = 2;
constexpr auto WEST = 3;
constexpr auto UP = 4;
constexpr auto DOWN = 5;
constexpr auto EX_ISDOOR = 1;
constexpr auto EX_CLOSED = 2;
constexpr auto EX_LOCKED = 4;
constexpr auto EX_HIDDEN = 8;
constexpr auto EX_IMM_ONLY = 16;
constexpr auto EX_PICKPROOF = 32;
constexpr auto EX_BROKEN = 64;
constexpr auto SECT_INSIDE = 0;
constexpr auto SECT_CITY = 1;
constexpr auto SECT_FIELD = 2;
constexpr auto SECT_FOREST = 3;
constexpr auto SECT_HILLS = 4;
constexpr auto SECT_MOUNTAIN = 5;
constexpr auto SECT_WATER_SWIM = 6;
constexpr auto SECT_WATER_NOSWIM = 7;
constexpr auto SECT_BEACH = 8;
constexpr auto SECT_PAVED_ROAD = 9;
constexpr auto SECT_DESERT = 10;
constexpr auto SECT_UNDERWATER = 11;
constexpr auto SECT_SWAMP = 12;
constexpr auto SECT_AIR = 13;
constexpr auto SECT_FROZEN_TUNDRA = 14;
constexpr auto SECT_ARCTIC = 15;
constexpr auto SECT_MAX_SECT = 15;
constexpr room_t IMM_PIRAHNA_ROOM = 25;

static const QStringList race_names;
static const QStringList position_types;
static const QStringList song_names;
static const QStringList cond_colorcodes;

extern item_types_t item_types;
extern QString credits;
extern QString info;
extern QString story;
extern QStringList where;
extern QStringList color_liquid;
extern QStringList fullness;
extern QStringList sky_look;
extern const QStringList temp_room_bits;
extern qint32 max_who;
extern QList<QString> continent_names;

extern const QStringList spells;
extern const QStringList apply_types;
extern const QStringList race_types;
extern const QStringList zone_modes;
extern const QStringList isr_bits;
extern const QStringList sector_types;
extern const QStringList room_bits;
extern const QStringList sector_types;
extern const QStringList pc_clss_types;
extern const QStringList pc_clss_types2;
extern const QStringList pc_clss_types3;
extern const QStringList pc_clss_abbrev;
extern qint32 max_who;
extern QString globalBuf;
extern bool wizlock;
extern QStringList nonew_new_list;
extern const QStringList zone_modes;
extern const QStringList equipment_types;
extern const QStringList utility_item_types;
extern qint32 top_of_mobt;
extern qint32 top_of_objt;
extern const QStringList action_bits;
extern const QStringList affected_bits;
extern const QStringList size_bitfields;
extern QStringList strs_damage_types;
extern QList<QString> continent_names;
extern qint32 top_of_objt;
extern const QStringList drinks;
extern const QStringList portal_bits;
extern const QStringList player_bits;
extern const QStringList combat_bits;
extern const QStringList connected_types;
extern const QStringList mob_types;
extern const QStringList exit_bits;
extern const QStringList race_abbrev;
extern qint32 max_who;
extern qint32 movement_loss[];
extern const QStringList skills;
extern const QStringList ki;
extern const QStringList innate_skills;
extern const QStringList reserved;
extern QStringList time_look;
extern qint32 top_of_objt;
extern time_t start_time; /* mud start time */
extern qint32 exp_table[61 + 1];
extern const QStringList dirs;
extern selfpurge_t selfpurge;
extern QList<QString> continent_names;
extern QMap<QString, Timer> PerfTimers;

extern QMap<QString, class reroll_t> reroll_sessions;
extern void debugpoint();
extern class skill_quest *skill_list;
/* Extern definitions. These are all in const.cpp. */
extern const class dex_app_type dex_app[];
extern const class con_app_type con_app[];
extern const class int_app_type int_app[];
extern const class str_app_type str_app[];
extern const class wis_app_type wis_app[];
extern class CharacterClassSkill g_skills[];
extern class CharacterClassSkill w_skills[];
extern class CharacterClassSkill t_skills[];
extern class CharacterClassSkill d_skills[];
extern class CharacterClassSkill b_skills[];
extern class CharacterClassSkill a_skills[];
extern class CharacterClassSkill p_skills[];
extern class CharacterClassSkill r_skills[];
extern class CharacterClassSkill k_skills[];
extern class CharacterClassSkill u_skills[];
extern class CharacterClassSkill c_skills[];
extern class CharacterClassSkill m_skills[];
extern CharacterPtr character_list;
extern CharacterPtr combat_list;
extern const QList<class ki_info_type> ki_info;
extern void end_oproc(CharacterPtr ch);
extern class weather_data weather_info;
extern const QList<class spell_info_type> spell_info;
extern const QList<class song_info_type> song_info;
extern class Leaderboard leaderboard;

class MinimumEntity
{
  QString name_;

public:
  MinimumEntity(QString n = {}) : name_(n) {}

  QString name(void) const
  {
    return name_;
  }
  void name(QString n)
  {
    name_ = n;
  }
};
class hunt_data : public QObject
{
  Q_OBJECT
public:
  QString huntname;
  qint32 itemnum;
  qint32 time;
  QList<qint32> itemsAvail;
};

class hunt_items : public QObject
{
  Q_OBJECT
public:
  hunt_dataPtr hunt;
  ObjectPtr obj;
  QString mobname;
};

class AreaData
{
public:
  void GetAreaData(zone_t zone_nr, qint32 mob, qint64 xps, qint64 gold);
  void DisplaySingleArea(CharacterPtr ch, zone_t area);
  void DisplayAreaData(CharacterPtr ch);
  void SortAreaData(CharacterPtr ch, SortState state);

private:
  area_stats_t areaStats;
};

class message
{
public:
  QString date;
  QString title;
  QString author;
  QString text;
};

class BOARD_INFO
{
public:
  CharacterPtr locked_for = {};
  bool lock = {};
  qint32 min_read_level = {};
  qint32 min_write_level = {};
  qint32 min_remove_level = {};
  qint32 type = {};
  qint32 owner = {};
  QString save_file;
  QList<message> msgs;
};

class Direction
{
public:
  enum Type : quint8
  {
    NORTH = 0,
    EAST = 1,
    SOUTH = 2,
    WEST = 3,
    UP = 4,
    DOWN = 5,
    UNDEFINED
  };
  Direction();
  Direction(Type value);
  Direction(QString string);
  constexpr operator Type() const
  {
    return value_;
  }
  [[nodiscard]] QString toString(void) const;
  [[nodiscard]] cmd_t toCommand(void) const;
  [[nodiscard]] Direction getReverse(void) const;

private:
  static const QMap<Type, QString> TypeToString_;
  static const QMap<Type, QString> TypeToStringAlt_;
  static const QMap<QString, Type> StringToType_;

  Type value_;
  bool valid_;
};
class ClanMember
{
public:
  ClanMember(CharacterPtr ch = {});

  [[nodiscard]] inline auto name(void) const
  {
    return name_;
  }
  [[nodiscard]] inline auto unused1(void) const
  {
    return unused1_;
  }
  [[nodiscard]] inline auto unused2(void) const
  {
    return unused2_;
  }
  [[nodiscard]] inline auto unused3(void) const
  {
    return unused3_;
  }
  [[nodiscard]] inline auto unused4(void) const
  {
    return unused4_;
  }
  [[nodiscard]] inline auto rights(void) const
  {
    return rights_;
  }
  [[nodiscard]] inline auto rank(void) const
  {
    return rank_;
  }
  [[nodiscard]] inline auto time_joined(void) const
  {
    return time_joined_;
  }

  inline auto name(auto s)
  {
    name_ = s;
    return s;
  }
  inline auto unused1(auto u)
  {
    unused1_ = u;
    return u;
  }
  inline auto unused2(auto u)
  {
    unused2_ = u;
    return u;
  }
  inline auto unused3(auto u)
  {
    unused3_ = u;
    return u;
  }
  inline auto unused4(auto u)
  {
    unused4_ = u;
    return u;
  }
  inline auto rights(auto r)
  {
    rights_ = r;
    return r;
  }
  inline auto rank(auto r)
  {
    rank_ = r;
    return r;
  }
  inline auto time_joined(auto tj)
  {
    time_joined_ = tj;
    return tj;
  }

private:
  QString name_{};
  qint64 unused1_{};
  qint64 unused2_{};
  quint64 unused3_{};
  QString unused4_{};
  quint32 rights_{};
  qint32 rank_{};
  quint32 time_joined_{};
};

class Clan : public QObject, public MinimumEntity
{
  Q_OBJECT
public:
  Clan(QObject *parent, QString clan_id = {});
  QString leader_{};
  QString founder_{};
  QString email_{};
  QString description_{};
  QString login_message_{};
  QString death_message_{};
  QString logout_message_{};
  QString clanmotd_{};
  quint16 tax_{};
  clan_id_t id_{};
  quint16 amt_{};
  QSet<room_t> rooms_;
  QList<ClanMember> members_;
  QList<vault_access_dataPtr> acc_;
  QQueue<QString> ctell_history_;

  void cdeposit(gold_t deposit);
  void cwithdraw(gold_t withdraw);
  gold_t getBalance(void);
  void setBalance(gold_t value);
  void log(QString entry);

private:
  gold_t balance_{};
  DCPtr dc_;
};

class Tracks
{
public:
  qint32 weight;
  qint32 race;
  qint32 direction;
  qint32 sex;
  qint32 condition;
  QString trackee;
};
class Path : public QObject
{
  Q_OBJECT
public:
  explicit Path(QObject *parent);
  QString determineRoute(CharacterPtr, qint32, qint32); // ch, from, to
  void addRoom(CharacterPtr, qint32, bool);             // ch, room, IgnoreConnectingIssues

  bool isRoomPathed(qint32 room);
  bool isRoomConnected(qint32 room);
  bool isPathConnected(PathPtr pa);
  qint32 connectRoom(PathPtr);

  QString name;
  qint32 s{};
  QMap<qint32, qint32> index_;
  DCPtr dc_;

private:
  bool findRoom(qint32 from, qint32 to, qint32 steps, qint32 leaststeps, QString buf);
  void resetPath();
  qint32 leastSteps(qint32 from, qint32 to, qint32 val, qint32 *bestval);
};

class PlayerConfig : public QObject
{
  Q_OBJECT
public:
  explicit PlayerConfig(QObject *parent);
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
  DCPtr dc_;
};

class class_data
{
public:
  QString name;
  QString lname;
  QString abbrev;
  bool playable;
  qint8 min_str;
  qint8 min_dex;
  qint8 min_con;
  qint8 min_int;
  qint8 min_wis;
};
class error_eof
{
};
class error_negative_int
{
};
class error_range_int
{
};
class error_range_under
{
};
class error_range_over
{
};
class attack_hit_type
{
public:
  QString singular_;
  QString plural_;
};

class race_data
{
public:
  const QString singular_name; /* Dwarf, Elf, etc.     */
  QString lowercase_name;      /* dwarf, elf, etc.     */
  const QString plural_name;   /* dwarves, elves, etc. */
  bool playable;               /* Can a player play as this race? */
  qint32 body_parts;           /* bitvector for body parts       */
  qint32 immune;               /* bitvector for immunities       */
  qint32 resist;               /* bitvector for resistances      */
  qint32 suscept;              /* bitvector for susceptibilities */
  qint32 hate_fear;            /* bitvector for hate/fear        */
  qint32 friendly;             /* bitvector for friendliness     */
  qint32 min_weight;           /* min weight */
  qint32 max_weight;

  qint32 min_height;
  qint32 max_height;

  quint32 min_str;
  quint32 max_str;
  qint32 mod_str;

  quint32 min_dex;
  quint32 max_dex;
  qint32 mod_dex;

  quint32 min_con;
  quint32 max_con;
  qint32 mod_con;

  quint32 min_int;
  quint32 max_int;
  qint32 mod_int;

  quint32 min_wis;
  quint32 max_wis;
  qint32 mod_wis;

  qint32 affects;        /* automatically added affects   */
  const QString unarmed; // unarmed attack message
};
class mob_matrix_data
{
public:
  qint64 experience;
  qint32 hitpoints;
  qint32 tohit;
  qint32 todam;
  qint32 armor;
  qint32 gold;
};
class char_file_u
{
public:
  qint8 sex;     /* Sex */
  qint8 c_class; /* Class */
  qint8 race;    /* Race */
  qint8 level;   /* Level */

  qint8 raw_str;
  qint8 raw_intel;
  qint8 raw_wis;
  qint8 raw_dex;
  qint8 raw_con;
  qint8 conditions[3];

  quint8 weight;
  quint8 height;

  qint16 hometown;
  quint32 gold;
  quint32 plat;
  qint64 exp;
  quint32 immune;
  quint32 resist;
  quint32 suscept;

  qint32 mana;     // current
  qint32 raw_mana; // max without eq/stat bonuses
  qint32 hit;
  qint32 raw_hit;
  qint32 move;
  qint32 raw_move;
  qint32 ki;
  qint32 raw_ki;

  qint16 alignment;
  quint32 hpmetas; // Used by familiars too... why not.
  quint32 manametas;
  quint32 movemetas;

  qint16 armor; // have to save these since mobs have different bases
  qint16 hitroll;
  qint16 damroll;
  qint32 afected_by;
  qint32 afected_by2;
  quint32 misc; // channel flags

  qint16 clan;
  qint32 load_room; // Which room to place character in

  quint32 acmetas;
  qint32 agemetas;
  qint32 extra_ints[3]; // available just in case
};
class profession
{
public:
  QString name;
  QString Name;
  quint16 skillno;
  quint8 c_class;
};
class ErrorHandler
{
public:
  class underrun
  {
  };
  class overrun
  {
  };
};
class help_index_element_new
{
public:
  QString keyword1_;
  QString keyword2_;
  QString keyword3_;
  QString keyword4_;
  QString keyword5_;
  QString entry_;
  QString related_;
  QString min_level_;
};
class threat_data
{
public:
  qint32 threat{};
  QString name_;
};
template <typename T>
class DataOperations
{
public:
  T data_;
  DataOperations(T data) : data_(data) {}
  [[nodiscard]] inline operator T() const { return data_; }
};

class ignore_entry
{
public:
  bool ignore;
  quint64 ignored_count;
};
class strcasecmp_compare
{
public:
  bool operator()(QString l, QString r) const
  {
    return l.compare(r, Qt::CaseInsensitive);
  }
};
class communication
{
public:
  communication(CharacterPtr ch, QString message);
  QString sender;
  bool sender_ispc;
  QString message;
  time_t timestamp;
};

class MobKills
{
public:
  quint32 howmany;
  qint32 name;
};

class AreaStats
{
public:
  zone_t area;
  qint64 xps;
  qint64 gold;
  QList<MobKills> mobKills;
};

class Test
{
public:
  Test() : function_(nullptr) {}
  Test(QString name, test_function_t function = {})
      : name_(name), function_(function)
  {
  }
  ReturnValue run(CharacterPtr ch)
  {
    if (function_ && ch)
    {
      return function_(ch);
    }
    return ReturnValue::eFAILURE;
  }
  QString getName(void) const { return name_; }

private:
  QString name_;
  test_function_t function_;
};

class Timer
{
public:
  qint32 timeleft = {};
  varg_t arg1 = {};
  QVariant var_arg1{};
  void *arg2 = {};
  void *arg3 = {};
  TIMER_FUNC *function = {};
};
class redeem_t
{
public:
  ObjectPtr choice1_obj = {};
  ObjectPtr choice2_obj = {};
  quint64 orig_rnum = {};
  vnum_t orig_vnum = {};
  ObjectPtr orig_obj = {};
  quint8 token_count = {};
  bool random = false;

  enum state_t
  {
    BEGIN,
    PICKED_OBJ_TO_REDEEM,
    REDEEM,
    CHOSEN
  } state = {};
};

class Table
{
public:
  Table(DatabasePtr database, QString name = "");
  DatabasePtr getDatabase(void) { return database_; }
  QString getName(void) { return name_; }
  ColumnPtr column(QString name, QString type);

private:
  DatabasePtr database_;
  QString name_;
};
class Database
{
public:
  Database(QObject *parent, QString name, QString hostname = "", QString type = "QPSQL");
  QSqlDatabase getQSqlDatabase(void) { return database_; }
  QString getName(void) { return name_; }
  QString getHostname(void) { return hostname_; }
  QString getType(void) { return type_; }
  TablePtr table(QString name);

  QMap<QString, TablePtr> tables;

private:
  QString name_;
  QString hostname_;
  QString type_;
  QSqlDatabase database_;
  DCPtr dc_;
};

class Column
{
public:
  Column(TablePtr table, QString name = "", QString type = "");
  TablePtr getTable(void) { return table_; }
  QString getName(void) { return name_; }
  QString getType(void) { return type_; }
  Column column(QString name, QString type);

private:
  TablePtr table_;
  QString name_;
  QString type_;
};

class Shop
{
public:
  QMap<quint64, qint32> type; /* Types of things shop will buy.       */
  float profit_buy = {};      /* Factor to multiply cost with.        */
  float profit_sell = {};     /* Factor to multiply cost with.        */
  float profit_buy_base = {};
  QString no_such_item1{};      /* Message if keeper hasn't got an item */
  QString no_such_item2{};      /* Message if player hasn't got an item */
  QString missing_cash1{};      /* Message if keeper hasn't got cash    */
  QString missing_cash2{};      /* Message if player hasn't got cash    */
  QString do_not_buy = {};      /* If keeper doesn't buy such things.   */
  QString message_buy = {};     /* Message when player buys item        */
  QString message_sell = {};    /* Message when player sells item       */
  vnum_t keeper = {};           /* The mob who owns the shop (virt)  */
  room_t in_room = {};          /* Where is the shop?                   */
  qint32 open1{}, open2 = {};   /* When does the shop open?             */
  qint32 close1{}, close2 = {}; /* When does the shop close?            */
  ObjectPtr inventory = {};     /* list of things shop never runs out of
                                 */
};

class Program : public QObject
{
  Q_OBJECT
  bool is_object_{};
  qint32 type_{};
  QString arglist_;
  QString comlist_;
  DCPtr dc_;

public:
  Program(QObject *parent) : QObject(parent), dc_(qobject_cast<class DC *>(parent)) {}
  [[nodiscard]] qint32 type(void) const { return type_; }
  qint32 type(qint32 type)
  {
    type_ = type;
    return type;
  }
  [[nodiscard]] QString typeString(void) const;
  [[nodiscard]] QString arglist(void) const { return arglist_; }
  QString arglist(QString list)
  {
    arglist_ = list;
    return list;
  }
  [[nodiscard]] QString comlist(void) const { return comlist_; }
  QString comlist(QString list)
  {
    return comlist_ = list;
    return list;
  }
};

class MobileProgram : public Program
{
public:
  MobileProgram(QObject *parent) : Program(parent) {}
  static qint32 name_to_type(QString name);
};

class ObjectProgram : public Program
{
public:
  ObjectProgram(QObject *parent) : Program(parent) {}
};

class index_data
{
public:
  void vnum(vnum_t v) { vnum_ = v; }
  [[nodiscard]] vnum_t vnum(void) const { return vnum_; }
  quint64 qty = {};                                                                         /* number of existing units of ths mob/obj */
  qint32 (*non_combat_func)(CharacterPtr, ObjectPtr, cmd_t, const QString, CharacterPtr){}; // non Combat special proc
  qint32 (*combat_func)(CharacterPtr, ObjectPtr, cmd_t, const QString, CharacterPtr){};     // combat special proc
  qint32 progtypes_ = {};

private:
  vnum_t vnum_ = {}; /* virt number of ths mob/obj           */
};

class ObjectIndex : public index_data
{
public:
  ObjectPtr item = {}; /* the mobile/object itself                 */
  QList<ObjectProgramPtr> programs_;
  QList<ObjectProgramPtr> class_programs_;
};

class MobileIndex : public index_data
{
public:
  CharacterPtr item = {};
  QList<MobileProgramPtr> programs_;
  QList<MobileProgramPtr> class_programs_;
};

namespace SSH
{
  class SSH : public QObject
  {
    Q_OBJECT
  public:
    explicit SSH(QObject *parent);
    void setup(void);
    qint32 poll(void);
    void close(void);
    ~SSH();

  signals:

  public slots:
    void run(void);

  private:
    ssh_bind sshbind = {};
    ssh_session sshsession = {};
    QString data = {};
  };
}

class SVoteData
{
public:
  QString answer;
  qint32 votes;
};

class CVoteData
{
public:
  void SetQuestion(CharacterPtr ch, QString question);
  void AddAnswer(CharacterPtr ch, QString answer);
  void RemoveAnswer(CharacterPtr ch, quint32 answer);
  void StartVote(CharacterPtr ch);
  void EndVote(CharacterPtr ch);
  void Reset(CharacterPtr ch);
  void OutToFile();
  bool HasVoted(CharacterPtr ch);
  bool Vote(CharacterPtr ch, quint32 vote);
  void DisplayVote(CharacterPtr ch);
  void DisplayResults(CharacterPtr ch);
  bool IsActive() { return active; }
  CVoteData();
  ~CVoteData();

private:
  bool active;
  QString vote_question;
  QList<SVoteData> answers;
  qint32 total_votes;
  QMap<QString, bool> ip_voted;
  QMap<QString, bool> char_voted;
};

class Ban : public MinimumEntity
{
public:
  enum class type_t : qsizetype
  {
    NOT,
    NEW,
    SELECT,
    ALL
  };
  static const QStringList ban_types;

  Ban(QString name = {}, QString site = {}, type_t type = {}, QDateTime dt = {}) : MinimumEntity(name),
                                                                                   site_(site),
                                                                                   type_(type),
                                                                                   date_(dt)
  {
  }

  inline auto site(QString s)
  {
    site_ = s;
    return s;
  }
  [[nodiscard]] inline auto site(void) const { return site_; }

  inline auto type(type_t t)
  {
    type_ = t;
    return t;
  }
  inline auto type(QString t)
  {
    auto i = ban_types.indexOf(t);
    type_ = type_t(i);
    return t;
  }
  [[nodiscard]] inline auto type(void) const { return type_; }

  inline auto date(QDateTime dt)
  {
    date_ = dt;
    return dt;
  }
  [[nodiscard]] inline auto date(void) const { return date_; }

  void save(QTextStream &out) const;

private:
  QString site_;
  type_t type_ = {};
  QDateTime date_;
};
auto &operator<<(auto &out, Ban::type_t type)
{
  out << qint32(type);
  return out;
}
auto &operator>>(auto &stream, Ban::type_t &type)
{
  qint32 t;
  stream >> t;
  type = Ban::type_t(t);
  return stream;
}

class Reservation
{
public:
  QString buf;
  message new_post;
  QMap<QString, BOARD_INFO>::iterator board;
};

class Bans
{
public:
  void load(void);
  void save(void) const;
  void clear(void);
  Ban::type_t is_banned(QString site) const;
  void add(Ban ban);
  void remove(QString site) { list_.remove(site); }
  auto list(void) const { return list_; }
  operator bool() const { return !list_.isEmpty(); }

private:
  const QString BANNED_FILE = u"banned"_s;
  QMap<QString, Ban> list_;
};

class Vaults
{
  QMap<QString, VaultPtr> list_;

public:
  void save(QString name);
  [[nodiscard]] inline VaultPtr has_vault(QString name)
  {
    if (list_.contains(name))
      return list_[name];

    return {};
  }
  void add_new_vault(QString name, qint32 indexonly);
  void remove_vault(QString name, BACKUP_TYPE backup = BACKUP_TYPE::NONE);
  void rename_vault_owner(QString oldname, QString newname);
  void remove_vault_accesses(QString name);

  ObjectPtr get_obj_in_all_vaults(QString object, qint32 num);
  vault_items_dataPtr get_items_in_all_vaults(QString object, qint32 num);
};

class Shops
{
public:
  explicit Shops(QObject *parent);
  DCPtr dc_;
};

class Room : public QObject
{
  Q_OBJECT
public:
  explicit Room(QObject *parent, qint16 room_number = {});
  explicit Room(QObject *parent);
  operator bool() const { return number > 0; }
  qint16 number = {}; // Rooms number
  zone_t zone = {};   // Room zone (for resetting)
  ZonePtr zonePtr = {};
  qint32 sector_type = {}; // sector type (move/hide)
  DenyPtr denied = {};
  QString name_;                              // Rooms name 'You are ...'
  QString description_;                       // Shown when entered
  ExtraDescriptionPtr ex_description = {};    // for examine/look
  RoomDirectionPtr dir_option[MAX_DIRS] = {}; // Directions
  quint32 room_flags = {};                    // DEATH, DARK ... etc
  bool isDark() const;
  bool isNoHome() const;
  bool isNoMob() const;
  bool isIndoors() const;
  bool isTeleportBlocked() const;
  bool isNoKi() const;
  bool isNoLearn() const;
  bool isNoMagic() const;
  bool isTunnel() const;
  bool isPrivate() const;
  bool isSafe() const;
  bool isNoSummon() const;
  bool isNoAstral() const;
  bool isNoPortal() const;
  bool isImpOnly() const;
  bool isFallDown() const;
  bool isArena() const;
  bool isQuiet() const;
  bool isUnstable() const;
  bool isNoQuit() const;
  bool isFallUp() const;
  bool isFallEast() const;
  bool isFallWest() const;
  bool isFallSouth() const;
  bool isFallNorth() const;
  bool isNoTeleport() const;
  bool isNoTrack() const;
  bool isClanRoom() const;
  bool isNoScan() const;
  bool isNoWhere() const;
  bool isLightRoom() const;

  auto arena() -> class Arena &;

  quint32 temp_room_flags = {}; // A second bitvector for flags that do NOT get saved.  These are temporary runtime flags_.
  qint16 light = {};            // Light factor of room

  qint32 (*funct)(CharacterPtr, cmd_t, const QString) = {}; // special procedure

  QList<ObjectPtr> contents_ = {};  // List of items in room
  QList<CharacterPtr> people_ = {}; // List of NPC / PC in room
  QList<Tracks> tracks_;            // beginning of the list of scents
  qint32 iFlags = {};               // Internal flags_. These do NOT save.
  // QList<path_data> paths_;
  DCPtr dc_;

  bool allow_class[CLASS_MAX] = {};

  void AddTrackItem(Tracks &newTrack);
  Tracks &TrackItem(qint32 nIndex);

  enum class room_errors_t
  {
    direction,
    number,
    zone,
    zonePtr,
    sector_type,
    denied,
    name,
    description,
    ex_description,
    room_flags,
    temp_room_flags,
    light,
    funct,
    alllow_class
  };
};

class World
{
public:
  Room &operator[](room_t room_key);
};

class ObjectFlags
{
public:
  object_value_t value[4] = {}; /* Values of the item (see list)    */
  object_type_t type_flag = {}; /* Type of item                     */
  ObjectPositions wear_flags = {};
  quint16 size = {};        /* Race restrictions                */
  quint32 extra_flags = {}; /* If it hums, glows etc            */
  qint16 weight = {};       /* Weight what else                 */
  qint32 cost = {};         /* Value when sold (gp.)            */
  quint32 more_flags = {};  /* A second bitvector (extra_flags2)*/
  level_t eq_level = {};    /* Min level to use it for eq       */
  qint16 timer = {};        /* Timer for object                 */
  CharacterPtr origin = {}; /* Creator of object, previously was stored at value[3] */
  bool Value(qsizetype i, object_value_t v)
  {
    if (i >= 0 && i < 4)
    {
      value[i] = v;
      return true;
    }
    return false;
  }
  object_value_t Value(qsizetype i)
  {
    if (i >= 0 && i < 4)
    {
      return value[i];
    }
    return {};
  }
};

class Toggle
{
public:
  Toggle(void) = default;
  Toggle(QString name, quint64 shift, command_gen3_t function, quint64 dependency_shift = UINT64_MAX, QString on_message = "$B$2on$R", QString off_message = "$B$4off$R");
  bool isValid(void) { return valid_; }

  QString name_;
  bool valid_ = false;
  quint64 shift_ = {};
  quint64 dependency_shift_ = {};
  quint64 value_ = {};
  QString on_message_;
  QString off_message_;
  ReturnValue (Character::*function_)(QStringList arguments, cmd_t cmd);
};

class Time
{
public:
  qint32 birth;  /* This represents the character's age                */
  qint32 logon;  /* Time of the last logon (used to calculate played) */
  qint32 played; /* This is the total accumulated time played in secs */
};

class Player
{
  QString prompt_;
  QString last_prompt_;

public:
  void setPrompt(QString prompt)
  {
    prompt_ = prompt;
  }
  QString getPrompt(void) const
  {
    return prompt_;
  }
  void setLastPrompt(QString last_prompt)
  {
    last_prompt_ = last_prompt;
  }
  QString getLastPrompt(void) const
  {
    return last_prompt_;
  }
  /************************************************************************
  | Player vectors
  | Character->player->toggles
  */
  QString last_site;          /* Last login from.. */
  QString poofin;             /* poofin message */
  QString poofout;            /* poofout message */
  ObjectPtr skillchange = {}; /* Skill changing equipment. */
  CharacterPtr golem = {};    // CURRENT golem.

  constexpr static quint32 PLR_BRIEF = 1U;
  constexpr static quint32 PLR_BRIEF_BIT = {};
  constexpr static quint32 PLR_COMPACT = 1U << 1;
  constexpr static quint32 PLR_COMPACT_BIT = 1;
  constexpr static quint32 PLR_DONTSET = 1U << 2;
  constexpr static quint32 PLR_DONTSET_BIT = 2;
  constexpr static quint32 PLR_DONOTUSE = 1U << 3;
  constexpr static quint32 PLR_DONOTUSE_BIT = 3;
  constexpr static quint32 PLR_NOHASSLE = 1U << 4;
  constexpr static quint32 PLR_NOHASSLE_BIT = 4;
  constexpr static quint32 PLR_SUMMONABLE = 1U << 5;
  constexpr static quint32 PLR_SUMMONABLE_BIT = 5;
  constexpr static quint32 PLR_WIMPY = 1U << 6;
  constexpr static quint32 PLR_WIMPY_BIT = 6;
  constexpr static quint32 PLR_ANSI = 1U << 7;
  constexpr static quint32 PLR_ANSI_BIT = 7;
  constexpr static quint32 PLR_VT100 = 1U << 8;
  constexpr static quint32 PLR_VT100_BIT = 8;
  constexpr static quint32 PLR_ONEWAY = 1U << 9;
  constexpr static quint32 PLR_ONEWAY_BIT = 9;
  constexpr static quint32 PLR_DISGUISED = 1U << 10;
  constexpr static quint32 PLR_DISGUISED_BIT = 10;
  constexpr static quint32 PLR_UNUSED = 1U << 11;
  constexpr static quint32 PLR_UNUSED_BIT = 11;
  constexpr static quint32 PLR_PAGER = 1U << 12;
  constexpr static quint32 PLR_PAGER_BIT = 12;
  constexpr static quint32 PLR_BEEP = 1U << 13;
  constexpr static quint32 PLR_BEEP_BIT = 13;
  constexpr static quint32 PLR_BARD_SONG = 1U << 14;
  constexpr static quint32 PLR_BARD_SONG_BIT = 14;
  constexpr static quint32 PLR_ANONYMOUS = 1U << 15;
  constexpr static quint32 PLR_ANONYMOUS_BIT = 15;
  constexpr static quint32 PLR_AUTOEAT = 1U << 16;
  constexpr static quint32 PLR_AUTOEAT_BIT = 16;
  constexpr static quint32 PLR_LFG = 1U << 17;
  constexpr static quint32 PLR_LFG_BIT = 17;
  constexpr static quint32 PLR_CHARMIEJOIN = 1U << 18;
  constexpr static quint32 PLR_CHARMIEJOIN_BIT = 18;
  constexpr static quint32 PLR_NOTAX = 1U << 19;
  constexpr static quint32 PLR_NOTAX_BIT = 19;
  constexpr static quint32 PLR_GUIDE = 1U << 20;
  constexpr static quint32 PLR_GUIDE_BIT = 20;
  constexpr static quint32 PLR_GUIDE_TOG = 1U << 21;
  constexpr static quint32 PLR_GUIDE_TOG_BIT = 21;
  constexpr static quint32 PLR_NEWS = 1U << 22;
  constexpr static quint32 PLR_NEWS_BIT = 22;
  constexpr static quint32 PLR_50PLUS = 1U << 23;
  constexpr static quint32 PLR_50PLUS_BIT = 23;
  constexpr static quint32 PLR_ASCII = 1U << 24;
  constexpr static quint32 PLR_ASCII_BIT = 24;
  constexpr static quint32 PLR_DAMAGE = 1U << 25;
  constexpr static quint32 PLR_DAMAGE_BIT = 25;
  constexpr static quint32 PLR_CLS_TREE_A = 1U << 26;
  constexpr static quint32 PLR_CLS_TREE_A_BIT = 26;
  constexpr static quint32 PLR_CLS_TREE_B = 1U << 27;
  constexpr static quint32 PLR_CLS_TREE_B_BIT = 27;
  constexpr static quint32 PLR_CLS_TREE_C = 1U << 28; // might happen one day
  constexpr static quint32 PLR_CLS_TREE_C_BIT = 28;
  constexpr static quint32 PLR_EDITOR_WEB = 1U << 29;
  constexpr static quint32 PLR_EDITOR_WEB_BIT = 29;
  constexpr static quint32 PLR_REMORTED = 1U << 30;
  constexpr static quint32 PLR_REMORTED_BIT = 30;
  constexpr static quint32 PLR_NODUPEKEYS = 1U << 31;
  constexpr static quint32 PLR_NODUPEKEYS_BIT = 31;
  static const QList<Toggle> togglables;
  static const QStringList toggle_txt;

  QString password_;
  ignoring_t ignoring = {}; /* List of ignored names */

  quint32 totalpkills = {};   // total number of pkills THIS LOGIN
  quint32 totalpkillslv = {}; // sum of levels of pkills THIS LOGIN
  quint32 pdeathslogin = {};  // pdeaths THIS LOGIN

  quint32 rdeaths = {};      // total number of real deaths
  quint32 pdeaths = {};      // total number of times pkilled
  quint32 pkills = {};       // # of pkills ever
  quint32 pklvl = {};        // # sum of levels of pk victims ever
  quint32 group_pkills = {}; // # of pkills for group
  quint32 grpplvl = {};      // sum of levels of group pkill victims
  quint32 group_kills = {};  // # of kills for group

  Time time = {}; // PC time data.  logon, played, birth

  quint32 bad_pw_tries = {}; // How many times people have entered bad pws

  qint16 statmetas = {}; // How many times I've metad a stat
                         // This number could go negative from stat loss
  quint16 kimetas = {};  // How many times I've metad ki (pc only)

  qint32 wizinvis = {};

  quint16 practices = {};       // How many can you learn yet this level
  quint16 specializations = {}; // How many specializations a player has left

  qint16 saves_mods[SAVE_TYPE_MAX + 1] = {}; // character dependant mods to saves (meta'able)

  quint32 bank = {}; /* gold in bank                            */

  quint32 toggles = {};   // Bitvector for toggles.  (Was specials.act)
  quint32 punish = {};    // flags for punishments
  quint32 quest_bv1 = {}; // 1st bitvector for quests

  qint16 buildLowVnum = {}, buildHighVnum = {};
  qint16 buildMLowVnum = {}, buildMHighVnum = {};
  qint16 buildOLowVnum = {}, buildOHighVnum = {};

  vnum_t last_mob_edit = {}; // vnum of last mob edited
  vnum_t last_obj_vnum = {}; // vnum of last obj edited

  QString last_tell = {};     /* last person who told           */
  qint16 last_mess_read = {}; /* for reading messages */

  // TODO: these 3 need to become PLR toggles
  bool holyLite = {};  // Holy lite mode
  bool stealth = {};   // If on, you are more stealth then norm. (god)
  bool incognito = {}; // invis imms will be seen by people in same room

  bool possesing = {};  /*  is the person possessing? */
  bool unjoinable = {}; // Do NOT autojoin
  bool hide[MAX_HIDE] = {};
  CharacterPtr hiding_from[MAX_HIDE] = {};
  QQueue<QString> away_msgs = {};
  QQueue<ChannelMessagePtr> tell_history = {};
  history_t gtell_history = {};
  joining_t joining = {};
  quint32 quest_points = {};
  qint16 quest_current[QUEST_MAX] = {};
  quint32 quest_current_ticksleft[QUEST_MAX] = {};
  qint16 quest_cancel[QUEST_MAX_CANCEL] = {};
  quint32 quest_complete[QUEST_TOTAL / ASIZE + 1] = {};
  std::multimap<qint32, std::pair<timeval, timeval>> lastseen = {};
  quint8 profession = {};
  bool multi = {};
  PlayerConfig *config = {};

  QString getJoining(void);
  void setJoining(QString list);
  void toggleJoining(QString key);
  void save_char_aliases(auto &streamfpsave);
  QString perform_alias(QString orig);
  void save(auto &streamfpsave, Time tmpage);
  bool read(auto &streamfpsave, CharacterPtr ch, QString filename);

  aliases_t aliases_; /* Aliases */
};

class obj_affected_type
{
public:
  location_t location = {}; /* Which ability to change (APPLY_XXX) */
  modifier_t modifier = {}; /* How much it changes by              */
};

class Entity
{
  QString description_;
  QString short_description_; /* when worn/carry/in cont.         */
  QString long_description_;  /* When in room                     */
  QString action_description_;

public:
  class Room &room(void);
  room_t in_room = {};

  QString description(QString s)
  {
    description_ = s;
    return s;
  }
  QString description(void)
  {
    return description_;
  }

  QString short_description(QString s)
  {
    short_description_ = s;
    return s;
  }
  [[nodiscard]] QString short_description(void) const { return short_description_; }

  QString long_description(QString s)
  {
    long_description_ = s;
    return s;
  }
  [[nodiscard]] QString long_description(void) const { return long_description_; }

  QString action_description(QString s)
  {
    action_description_ = s;
    return s;
  }
  [[nodiscard]] QString action_description(void) const { return action_description_; }
};

[[nodiscard]] inline constexpr bool isSet(auto flag, auto bit)
{
  return flag & bit;
};
class Object : public QObject, public MinimumEntity, public Entity
{
  Q_OBJECT
public:
  enum class portal_types_t
  {
    Player = 0,
    Permanent = 1,
    Temp = 2,
    LookOnly = 3,
    PermanentNoLook = 4
  };

  enum portal_flags_t
  {
    No_Leave = 1 << 0,
    No_Enter = 1 << 1
  };

  static const QStringList size_bits;
  static const QStringList more_obj_bits;
  static const QStringList extra_bits;
  static const QStringList apply_types;

  qint32 item_number = {}; /* Where in data-base               */
  qint32 vroom = {};       /* for corpse saving */
  ObjectFlags flags_ = {}; /* Object information               */
  qint16 num_affects = {};
  QList<obj_affected_type> affected = {};  /* Which abilities in PC to change  */
  ExtraDescriptionPtr ex_description = {}; /* extra descriptions     */
  CharacterPtr carried_by = {};            /* Carried by :NULL in room/conta   */
  CharacterPtr equipped_by = {};           /* so I can access the player :)    */

  ObjectPtr in_obj = {};   /* In what object NULL when none    */
  ObjectPtr contains = {}; /* Contains objects                 */

  ObjectPtr next_content = {}; /* For 'contains' lists             */
  ObjectPtr next = {};         /* For the object list              */
  ObjectPtr next_skill = {};
  CasinoTablePtr table = {};
  CasinoSlotMachinePtr slot = {};
  CasinoRouletteWheelPtr wheel = {};
  time_t save_expiration = {};
  time_t no_sell_expiration = {};

  explicit Object(QObject *parent);
  bool isDark(void);
  bool isPortal(void);
  bool isTotem(void)
  {
    return flags_.type_flag == ITEM_TOTEM;
  }
  bool isWeapon(void)
  {
    return flags_.type_flag == ITEM_WEAPON;
  }
  bool isArmor(void)
  {
    return flags_.type_flag == ITEM_ARMOR;
  }
  bool isInstrument(void)
  {
    return flags_.type_flag == ITEM_INSTRUMENT;
  }
  bool isContainer(void)
  {
    return flags_.type_flag == ITEM_CONTAINER;
  }
  bool isLight(void)
  {
    return flags_.type_flag == ITEM_LIGHT;
  }
  bool isScroll(void)
  {
    return flags_.type_flag == ITEM_SCROLL;
  }
  bool isWand(void)
  {
    return flags_.type_flag == ITEM_WAND;
  }
  bool isStaff(void)
  {
    return flags_.type_flag == ITEM_STAFF;
  }
  bool isFireWeapon(void)
  {
    return flags_.type_flag == ITEM_FIREWEAPON;
  }
  bool isMissle(void)
  {
    return flags_.type_flag == ITEM_MISSILE;
  }
  bool isTreasure(void)
  {
    return flags_.type_flag == ITEM_TREASURE;
  }
  bool isPotion(void)
  {
    return flags_.type_flag == ITEM_POTION;
  }
  bool isWorn(void)
  {
    return flags_.type_flag == ITEM_WORN;
  }
  bool isOther(void)
  {
    return flags_.type_flag == ITEM_OTHER;
  }
  bool isTrash(void)
  {
    return flags_.type_flag == ITEM_TRASH;
  }
  bool isTrap(void)
  {
    return flags_.type_flag == ITEM_TRAP;
  }
  bool isNote(void)
  {
    return flags_.type_flag == ITEM_NOTE;
  }
  bool isDrinkContainer(void)
  {
    return flags_.type_flag == ITEM_DRINKCON;
  }
  bool isKey(void)
  {
    return flags_.type_flag == ITEM_KEY;
  }
  bool isFood(void)
  {
    return flags_.type_flag == ITEM_FOOD;
  }
  bool isMoney(void)
  {
    return flags_.type_flag == ITEM_MONEY;
  }
  bool isPen(void)
  {
    return flags_.type_flag == ITEM_PEN;
  }
  bool isBoat(void)
  {
    return flags_.type_flag == ITEM_BOAT;
  }
  bool isBoard(void)
  {
    return flags_.type_flag == ITEM_BOARD;
  }
  bool isFountain(void)
  {
    return flags_.type_flag == ITEM_FOUNTAIN;
  }
  bool isUtility(void)
  {
    return flags_.type_flag == ITEM_UTILITY;
  }
  bool isBeacon(void)
  {
    return flags_.type_flag == ITEM_BEACON;
  }
  bool isLockpick(void)
  {
    return flags_.type_flag == ITEM_LOCKPICK;
  }
  bool isClimbable(void)
  {
    return flags_.type_flag == ITEM_CLIMBABLE;
  }
  bool isMegaphone(void)
  {
    return flags_.type_flag == ITEM_MEGAPHONE;
  }
  bool isAltar(void)
  {
    return flags_.type_flag == ITEM_ALTAR;
  }
  bool isKeyring(void)
  {
    return flags_.type_flag == ITEM_KEYRING;
  }

  room_t getPortalDestinationRoom(void)
  {
    if (!isPortal())
    {
      return 0;
    }
    return flags_.value[0];
  }
  void setPortalDestinationRoom(room_t room)
  {
    if (!isPortal())
    {
      return;
    }
    flags_.value[0] = room;
  }

  portal_types_t getPortalType(void)
  {
    if (!isPortal())
    {
      return portal_types_t::Player;
    }
    return static_cast<portal_types_t>(flags_.value[1]);
  }
  bool isPortalTypePlayer(void)
  {
    return getPortalType() == portal_types_t::Player;
  }
  bool isPortalTypePermanent(void)
  {
    return getPortalType() == portal_types_t::Permanent;
  }
  bool isPortalTypeTemp(void)
  {
    return getPortalType() == portal_types_t::Temp;
  }
  bool isPortalTypeLookOnly(void)
  {
    return getPortalType() == portal_types_t::LookOnly;
  }
  bool isPortalTypePermanentNoLook(void)
  {
    return getPortalType() == portal_types_t::PermanentNoLook;
  }
  bool isQuest(void);
  bool isTest(void);
  bool isGodload(void);
  bool isCustom(void)
  {
    return isSet(flags_.more_flags, ITEM_NO_CUSTOM);
  }

  qint32 getPortalLeaveZone(void)
  {
    if (!isPortal())
    {
      return -1;
    }
    return flags_.value[2];
  }
  qint32 getPortalFlags(void)
  {
    if (!isPortal())
    {
      return 0;
    }
    return flags_.value[3];
  }
  bool hasPortalFlagNoLeave(void);
  bool hasPortalFlagNoEnter(void);

  quint64 getLevel(void);

  qint32 keywordfind(void);
  void setOwner(QString owner) { owner_ = owner; }
  QString getOwner(void) { return owner_; }

  [[nodiscard]] inline bool isCorpse(void) const
  {
    return isSet(flags_.extra_flags, ITEM_PC_CORPSE) || isSet(flags_.extra_flags, ITEM_PC_CORPSE_LOOTED);
  }
  [[nodiscard]] inline bool isTradable(void) const
  {
    return !isSet(flags_.more_flags, ITEM_NO_TRADE);
  }
  bool ActionDescription(QString action_description)
  {
    action_description_ = action_description;
    if (action_description.isEmpty())
      return false;
    else
      return true;
  }
  QString ActionDescription(void) const { return action_description_; }
  object_type_t Type(void) { return flags_.type_flag; }
  QString TypeString(void);
  bool Type(object_type_t type)
  {
    if (type >= ITEM_TYPE_MAX)
    {
      type = {};
      flags_.Value(2, 0);
      return false;
    }
    flags_.type_flag = type;
    if (flags_.type_flag == 24)
      flags_.Value(2, -1);
    else
      flags_.Value(2, 0);
    return true;
  }
  bool TypeString(QString type);
  ~Object();
  DCPtr dc_;

private:
  QString owner_;
  QString name_;               /* Title of object :get etc.        */
  QString action_description_; /* What to write when used          */
};

class CasinoPlayer
{
public:
  CasinoPlayerPtr next;
  CasinoTablePtr table;
  CharacterPtr ch;
  qint32 hand_data[21];
  // theoretical cardmax is lower than 21, but whatever
  qint32 bet;
  bool insurance;
  bool doubled;
  qint32 state;
};

class CasinoTable
{
public:
  ObjectPtr obj; // linked to obj
  cDeckPtr deck;
  CasinoPlayerPtr plr;
  CasinoPlayerPtr cr; // current
  bool gold;
  qint32 options;
  CharacterPtr dealer;
  qint32 hand_data[21]; // dealer
  qint32 handnr;
  qint32 state;
  qint32 won;
  qint32 lost;
};

class cDeck : public QObject
{
public:
  cDeck(QObject *parent);
  CasinoTablePtr table;
  QList<qint32> cards;
  qint32 pos;
  qint32 decks;
  DCPtr dc_;
};
class char_skill_data
{
public:
  skill_t skillnum{}; // ID # of skill.
  qint16 learned{};   // % chance for success must be > 0
  qint32 unused[5]{}; // for future use
};

class CharacterClassSkill
{
public:
  QString skillname_;       // name of skill
  qint16 skillnum{};        // ID # of skill
  level_t levelavailable{}; // what level class can get it
  qint16 maximum{};         // maximum value PC can train it to (1-100)
  quint8 group{};           // which class tree group it is assigned
  qint16 attrs{};           // What attributes the skill is based on
};

class ResetCommand
{
public:
  ResetCommand() {};
  ResetCommand(QChar comm) : command(comm), active(1) {};
  QChar command = {};  /* current command                      */
  qint32 if_flag = {}; // 0=always 1=if prev exe'd  2=if prev DIDN'T exe   3=ONLY on reboot
  qint32 arg1 = {};
  qint32 arg2 = {};
  qint32 arg3 = {};
  QString comment = {}; /* Any comments that went with the command */
  qint32 active = {};   // is it active? alot aren't on the builders' port
  time_t last = {};     // when was it last reset
  CharacterPtr lastPop = {};
  time_t lastSuccess = {};
  quint64 attempts = {};
  quint64 successes = {};
  /*
   *  Commands:              *
   *  'M': Read a mobile     *
   *  'O': Read an object    *
   *  'P': Put obj in obj    *
   *  'G': Obj to character       *
   *  'E': Obj to character equip *
   *  'D': Set state of door *
   *  '%': arg1 in arg2 chance of being true *
   *       (this is used for putting a %chance on next command *
   */
};

class weather_data
{
public:
  qint32 pressure; // How is the pressure ( Mb )
  qint32 change;   // How fast and what way does it change.
  qint32 sky;      // How is the sky.
  qint32 sunlight; // And how much sun.

  // following are usused at this time
  qint32 windspeed;     // How fast wind is blowing
  qint32 winddirection; // What direction it is blowing in
  qint32 temperature;   // Duh...
  qint32 modifiers;     // fog?  Ice?
};

class Zone : public MinimumEntity
{

public:
  enum class ResetType
  {
    normal,
    full
  };

  // Remember to update const.C  Zone::zone_bits if you change this
  enum Flag
  {
    NO_TELEPORT = 1,
    IS_TOWN = 1 << 1, // Keep out the really bad baddies that are STAY_NO_TOWN
    MODIFIED = 1 << 2,
    UNUSED = 1 << 3,
    BPORT = 1 << 4,
    NOCLAIM = 1 << 5, // cannot claim this area
    NOHUNT = 1 << 6,
  };

  static QStringList zone_bits;

  Zone(quint64 zone_key = 0);

  quint64 lifespan = {}; /* how long between resets (minutes)  */
  QDateTime last_full_reset = {};
  quint64 age = {}; /* current age of ths zone (minutes) */

  quint64 players = {}; // Number of PCs in the zone

  qint32 reset_mode = {}; /* conditions for reset (see below)   */

  zone_commands_t cmd = {}; /* command table for reset             */

  /*
   *  Reset mode:                              *
   *  0: Don't reset, and don't update age.    *
   *  1: Reset if no PC's are located in zone. *
   *  2: Just reset.                           *
   *  Update QStringList zone_modes (const.C) if you change this *
   */

  weather_data weather_info = {}; // for zones with unique weather

  qint32 num_mob_first_repop = {}; // number of mobs in this zone that were repoped in first repop
  qint32 num_mob_on_repop = {};    // number of mobs in this zone that were repoped in last repop
  qint32 death_counter = {};       // +- counter for how often mobs in zone are killed
  qint32 counter_mod = {};         // how quickly mobs are taken off the death_counter

  qint32 clanowner = {};
  qint32 gold = {};                  // gold (possibly the most descriptive comment of all time)
  qint32 continent = {};             // what continent the zone belongs to
  qint32 repops_without_deaths = {}; // Number of repops in a row with no deaths
  qint32 repops_with_bonus = {};     // Number of repops where a 10% bonus occurred.

  void reset(ResetType type = ResetType::normal);
  bool isEmpty(void);

  bool isTown(void);
  void setTown(bool flag = true);

  bool isNoTeleport(void);
  void setNoTeleport(bool flag = true);

  bool isNoClaim(void);
  void setNoClaim(bool flag = true);

  bool isNoHunt(void);
  void setNoHunt(bool flag = true);

  bool isModified(void);
  void setModified(bool flag = true);

  void incrementDiedThisTick(void);
  void setDiedThisTick(quint64 died = {});
  quint64 getDiedThisTick(void);

  QString getFilename(void);
  void setFilename(QString);

  void setZoneFlags(quint64);
  quint64 getZoneFlags(void) { return zone_flags; }

  void setGold(quint64 value);
  void addGold(quint64 value);
  void incrementPlayers(void);
  void decrementPlayers(void);

  room_t getBottom(void);
  void setBottom(qint32 room_key);

  qint32 getTop(void);
  void setTop(qint32 room_key);

  room_t getRealBottom(void);
  void setRealBottom(qint32 room_key);

  qint32 getRealTop(void);
  void setRealTop(qint32 room_key);

  void write(auto &stream);
  qint32 show_info(CharacterPtr ch);

  zone_t getID(void) const
  {
    return id_;
  }

private:
  zone_t id_ = {};
  quint64 died_this_tick = {}; // number of mobs that have died in this zone this pop
  quint64 zone_flags = {};     /* flags for the entire zone eg: !teleport */
  QString filename = {};       /* name of the file this zone is kept in */
  room_t bottom = {};          /* bottom limit for room vnums in this zone */
  room_t top = {};             /* upper limit for room vnums in this zone */
  room_t bottom_rnum = {};
  room_t top_rnum = {};
};

class bestowable_god_commands_type
{
public:
  QString name;   // name of command
  qint16 num{};   // ID # of command
  bool testcmd{}; // true = test command, false = normal command
};

class command_lag
{
public:
  command_lag *next{};
  CharacterPtr ch{};
  cmd_t cmd_number{};
  qint32 lag{};
};

class Arena
{
public:
  static constexpr room_t ARENA_LOW = 14600;
  static constexpr room_t ARENA_HIGH = 14680;
  static constexpr room_t ARENA_DEATHTRAP = 14680;

  enum class Types
  {
    NORMAL,
    CHAOS,
    POTATO,
    PRIZE,
    HP,
    PLAYER_FREE,
    PLAYER_NOT_FREE
  };

  enum class Statuses
  {
    CLOSED,
    OPENED
  };

  auto Low(void) const { return low_; }
  auto High(void) const { return high_; }
  auto Number(void) const { return number_; }
  auto CurrentNumber(void) const { return current_number_; }
  auto IncrementCurrentNumber(void) -> void { current_number_++; }
  auto HPLimit(void) const -> quint64 { return hp_limit_; }
  auto Type(void) const -> Types { return type_; }
  auto Status(void) const -> Statuses { return status_; }
  auto EntryFee(void) const -> gold_t { return entry_fee_; }

  auto isNormal(void) const -> bool { return Type() == Types::NORMAL; }
  auto isChaos(void) const -> bool { return Type() == Types::CHAOS; }
  auto isPotato(void) const -> bool { return Type() == Types::POTATO; }
  auto isPrize(void) const -> bool { return Type() == Types::PRIZE; }
  auto isHP(void) const -> bool { return Type() == Types::HP; }
  auto isPlayerFree(void) const -> bool { return Type() == Types::PLAYER_FREE; }
  auto isPlayerNotFree(void) const -> bool { return Type() == Types::PLAYER_NOT_FREE; }

  auto isOpened(void) const -> bool { return Status() == Statuses::OPENED; }
  auto isClosed(void) const -> bool { return Status() == Statuses::CLOSED; }

private:
  level_t low_ = {};
  level_t high_ = {};
  quint64 number_ = {};
  quint64 current_number_ = {};
  quint64 hp_limit_ = {};
  Types type_ = {};
  Statuses status_ = {};
  gold_t entry_fee_ = {};
};

class AuctionTicket
{
public:
  qint32 vitem;
  QString item_name;
  quint32 price;
  QString seller;
  QString buyer;
  AuctionStates state;
  quint32 end_time;
  ObjectPtr obj;
};

class AuctionHouse
{
public:
  AuctionHouse(QString in_file, QObject *parent);
  void CollectTickets(CharacterPtr ch, quint32 ticket = 0);
  void CancelAll(CharacterPtr ch);
  void AddItem(CharacterPtr ch, ObjectPtr obj, quint32 price, QString buyer);
  void RemoveTicket(CharacterPtr ch, quint32 ticket);
  void BuyItem(CharacterPtr ch, quint32 ticket);
  void ListItems(CharacterPtr ch, ListOptions options, QString name, quint32 to, quint32 from);
  void CheckExpire();
  void Identify(CharacterPtr ch, quint32 ticket);
  void AddRoom(CharacterPtr ch, qint32 room);
  void RemoveRoom(CharacterPtr ch, qint32 room);
  void ListRooms(CharacterPtr ch);
  void HandleRename(CharacterPtr ch, QString old_name, QString new_name);
  void HandleDelete(QString name);
  void CheckForSoldItems(CharacterPtr ch);
  bool IsAuctionHouse(qint32 room);
  void DoModify(CharacterPtr ch, quint32 ticket, quint32 new_price);
  void ShowStats(CharacterPtr ch);
  void Save();
  void Load();
  [[nodiscard]] quint32 getItemsPosted(void) { return ItemsPosted; }
  void setItemsPosted(quint32 items_posted) { ItemsPosted = items_posted; }

private:
  quint32 ItemsPosted;
  quint32 ItemsExpired;
  quint32 ItemsSold;
  quint32 TaxCollected;
  quint32 Revenue;
  quint32 UncollectedGold;
  quint32 ItemsActive;
  void ParseStats();
  bool CanSellMore(CharacterPtr ch);
  bool IsOkToSell(ObjectPtr obj);
  bool IsWearable(CharacterPtr ch, qint32 vnum);
  bool IsNoTrade(qint32 vnum);
  bool IsSeller(QString in_name, QString seller);
  bool IsExist(QString name, qint32 vnum);
  bool IsClass(qint32 vnum, QString isclass);
  bool IsRace(qint32 vnum, QString israce);
  bool IsName(QString name, qint32 vnum);
  bool IsSlot(QString slot, qint32 vnum);
  bool IsLevel(quint32 to, quint32 from, qint32 vnum);
  QMap<qint32, qint32> auction_rooms;
  quint32 cur_index;
  QString filename_;
  QMap<quint32, AuctionTicket> Items_For_Sale;
  DCPtr dc_;
};

class wizlist_info
{
  QString name_;
  level_t level_ = {};

public:
  wizlist_info(QString name, level_t level) : name_(name), level_(level) {}
  QString getName(void) const { return name_; }
  void setName(QString name) { name_ = name; }
  level_t getLevel(void) const { return level_; }
  void setLevel(level_t level) { level_ = level; }
};
class world_file_list_item
{
public:
  QString filename;
  vnum_t firstnum;
  vnum_t lastnum;
  qint32 flags;
  world_file_list_item *next;
};

class vault_items_data
{
public:
  qint32 item_vnum;
  qint32 count;
  ObjectPtr obj; // for full-save items
};

class vault_access_data
{
public:
  QString name;
};

class sorted_vault
{
public:
  // This stores the quantity of each item found in a vault
  QMap<QString, std::pair<ObjectPtr, quint32>> vault_content_qty = {};

  // This stores the order in which vault items are found
  QList<QString> vault_contents = {};

  quint32 weight = {};
};

// NOTE:  If you change this structure, keep in mind how it is used in guild.C
// The min_level_XXX stuff MUST be updated in guild.C if you change this.  It is
// using an offset from min_level_magic depending on class *(min_level_magic+2bytes)
class spell_info_type
{
public:
  spell_info_type(quint32 beats, position_t minimum_position, quint8 min_usesmana, qint16 targets, qint16 difficulty)
      : beats_(beats), minimum_position_(minimum_position), min_usesmana_(min_usesmana), targets_(targets), spell_pointer_(nullptr), spell_pointer2_(nullptr), difficulty_(difficulty)
  {
  }
  spell_info_type(quint32 beats, position_t minimum_position, quint8 min_usesmana, qint16 targets, SPELL_FUN *spell_pointer, qint16 difficulty)
      : beats_(beats), minimum_position_(minimum_position), min_usesmana_(min_usesmana), targets_(targets), spell_pointer_(spell_pointer), difficulty_(difficulty)
  {
  }
  spell_info_type(quint32 beats, position_t minimum_position, quint8 min_usesmana, qint16 targets, SPELL_FUN2 *spell_pointer, qint16 difficulty)
      : beats_(beats), minimum_position_(minimum_position), min_usesmana_(min_usesmana), targets_(targets), spell_pointer2_(spell_pointer), difficulty_(difficulty)
  {
  }
  // spell_info_type(quint32 beats, position_t minimum_position, quint8 min_usesmana, qint16 targets, spell_gen2_t spell_pointer, qint16 difficulty)
  //     : beats_(beats), minimum_position_(minimum_position), min_usesmana_(min_usesmana), targets_(targets), spell_pointer_(nullptr), spell_pointer2_(spell_pointer), difficulty_(difficulty)
  //{
  // }
  quint32 beats(void) const { return beats_; }
  position_t minimum_position(void) const { return minimum_position_; }
  quint8 min_usesmana(void) const { return min_usesmana_; }
  qint16 targets(void) const { return targets_; }
  SPELL_FUN *spell_pointer(void) const { return spell_pointer_; }
  qint16 difficulty(void) const { return difficulty_; }
  SPELL_FUN2 *spell_pointer2(void) const { return spell_pointer2_; }

private:
  quint32 beats_;               /* Waiting time after spell	*/
  position_t minimum_position_; /* Position for caster		*/
  quint8 min_usesmana_;         /* Mana used			*/
  qint16 targets_;              /* Legal targets		*/
  SPELL_FUN *spell_pointer_;    /* Function to call		*/
  qint16 difficulty_;           /* Spell difficulty */
  SPELL_FUN2 *spell_pointer2_;  /* Function to call		*/
};

class Command : public MinimumEntity
{
public:
  Command(QString name, command_gen2_t ptr2,
          position_t min_pos = {}, level_t min_lvl = {}, cmd_t nr = cmd_t::DEFAULT,
          bool allow_charmie = false, quint8 toggle_hide = {}, CommandType type = {})
      : MinimumEntity(name),
        command_pointer2_(ptr2), command_pointer3_(nullptr),
        minimum_position_(min_pos), minimum_level_(min_lvl), command_number_(nr),
        allow_charmie_(allow_charmie), toggle_hide_(toggle_hide), type_(type)
  {
  }
  Command(QString name, command_gen3_t ptr3,
          position_t min_pos = {}, level_t min_lvl = {}, cmd_t nr = cmd_t::DEFAULT,
          bool allow_charmie = false, quint8 toggle_hide = {}, CommandType type = {})
      : MinimumEntity(name),
        command_pointer2_(nullptr), command_pointer3_(ptr3),
        minimum_position_(min_pos), minimum_level_(min_lvl), command_number_(nr),
        allow_charmie_(allow_charmie), toggle_hide_(toggle_hide), type_(type)
  {
  }

  [[nodiscard]] inline command_gen2_t getFunction2(void) const { return command_pointer2_; }
  void setFunction2(const command_gen2_t function) { command_pointer2_ = function; }

  [[nodiscard]] inline command_gen3_t getFunction3(void) const { return command_pointer3_; }
  void setFunction3(const command_gen3_t function) { command_pointer3_ = function; }

  [[nodiscard]] inline cmd_t getNumber(void) const { return command_number_; }
  void setNumber(const cmd_t number) { command_number_ = number; }

  [[nodiscard]] inline level_t getMinimumLevel(void) const { return minimum_level_; }
  void setMinimumLevel(const level_t level) { minimum_level_ = level; }

  [[nodiscard]] inline position_t getMinimumPosition(void) const { return minimum_position_; }
  void setMinimumPosition(const position_t minimum_position) { minimum_position_ = minimum_position; }

  bool isCharmieAllowed(void) const { return allow_charmie_ == true; }

  [[nodiscard]] inline CommandType getType(void) const { return type_; }
  void setType(const CommandType type) { type_ = type; }

  // qint32 (*command_pointer_)(CharacterPtr ch, QString argument, cmd_t cmd);               /* Function that does it            */
  // ReturnValue (*command_pointer2_)(CharacterPtr ch, QString argument, cmd_t cmd); /* Function that does it            */
  // ReturnValue (Character::*command_pointer3_)(QStringList arguments, cmd_t cmd);    /* Function that does it            */

  command_gen2_t command_pointer2_;
  command_gen3_t command_pointer3_;
  position_t minimum_position_; /* Position commander must be in    */
  level_t minimum_level_;       /* Minimum level needed             */
  cmd_t command_number_;        /* Passed to function as argument   */
  bool allow_charmie_;
  quint8 toggle_hide_;
  CommandType type_;
};

class Commands
{
public:
  Commands(void);
  void add(Command cmd);
  auto find(QString arg) -> std::expected<Command, search_error>;
  auto find(cmd_t cmd) -> std::expected<Command, search_error>;
  static QList<Command> commands_;
  static QMap<QString, Command> qstring_command_map_;
  static QMap<cmd_t, Command> cmd_t_command_map_;
};

class Proxy
{
public:
  Proxy(QString);
  Proxy(void) {}

  inet_protocol_family_t getInet_Protocol_Fanily(void) { return inet_protocol_family; }
  QString getHeader(void) { return header; }
  bool isActive(void) { return active; }
  QHostAddress getSourceAddress(void) { return source_address; }
  QHostAddress getDestinationAddress(void) { return destination_address; }
  quint16 getSourcePort(void) { return source_port; }
  quint16 getDestinationPort(void) { return destination_port; }

private:
  inet_protocol_family_t inet_protocol_family = {};
  QString header = {};
  bool active = false;
  QHostAddress source_address = {};
  QHostAddress destination_address = {};
  quint16 source_port = {};
  quint16 destination_port = {};
};

class Connection : public QObject
{
  Q_OBJECT
public:
  enum states
  {
    PLAYING,
    GET_PROXY,
    GET_NAME,
    GET_OLD_PASSWORD,
    CONFIRM_NEW_NAME,
    GET_NEW_PASSWORD,
    CONFIRM_NEW_PASSWORD,
    GET_NEW_SEX,
    OLD_GET_CLASS,
    READ_MOTD,
    SELECT_MENU,
    RESET_PASSWORD,
    CONFIRM_RESET_PASSWORD,
    EXDSCR,
    OLD_GET_RACE,
    WRITE_BOARD,
    EDITING,
    EDITING_V2,
    SEND_MAIL,
    DELETE_CHAR,
    OLD_CHOOSE_STATS,
    PFILE_WIPE,
    ARCHIVE_CHAR,
    CLOSE,
    CONFIRM_PASSWORD_CHANGE,
    EDIT_MPROG,
    DISPLAY_ENTRANCE,
    PRE_DISPLAY_ENTRANCE,
    SELECT_RECOVERY_MENU,
    GET_NEW_RECOVERY_QUESTION,
    GET_NEW_RECOVERY_ANSWER,
    GET_NEW_RECOVERY_EMAIL,
    QUESTION_ANSI,
    GET_ANSI,
    QUESTION_SEX,
    QUESTION_STAT_METHOD,
    GET_STAT_METHOD,
    OLD_STAT_METHOD,
    NEW_STAT_METHOD,
    NEW_PLAYER,
    QUESTION_RACE,
    GET_RACE,
    QUESTION_CLASS,
    GET_CLASS,
    QUESTION_STATS,
    GET_STATS
  };

  explicit Connection(QObject *parent);

  Proxy proxy = {};

  qint32 descriptor = {}; /* file descriptor for socket	*/
  qint32 desc_num = {};

  states connected = {}; /* mode of 'connectedness'	*/
  qint32 web_connected = {};
  qint32 wait = {};          /* wait for how many loops	*/
  QString showstr_head = {}; /* for paging through texts	*/
  qint32 showstr_count = {};
  qint32 showstr_page = {};
  bool new_newline = {}; /* prepend newline in output	*/
  //  character	**str;			/* for the modify-str system	*/
  QString *hashstr = {};
  QString astr = {};
  QString buf = {};        /* buffer for raw input	*/
  QString last_input = {}; /* the last input	*/
  QByteArray output = {};  /* output buffer for writing to connection	*/
  QString inbuf = {};
  QQueue<QString> input = {};  /* queue of unprocessed input	*/
  CharacterPtr character = {}; /* linked to character		*/
  CharacterPtr original = {};  /* for switch / return		*/
  ConnectionPtr snooping = {}; // Who is this character snooping
  ConnectionPtr snoop_by = {}; // And who is snooping this character
  ConnectionPtr next = {};     // link to next descriptor
  qint32 tick_wait = {};       /* # ticks desired to wait	*/
  qint32 reallythere = {};     /* Goddamm #&@$*% sig 13 (hack) */
  qint32 prompt_mode = {};
  quint8 idle_tics = {};
  time_t login_time = {};
  NewCharacterStatsPtr stats = {}; // for rolling up a character

  QString *strnew{}; /* for the modify-str system	*/
  QString qstrnew;
  QString backstr;
  qint32 idle_time = {}; // How long the descriptor has been idle, overall.
  bool color = {};
  bool server_size_echo = false;
  bool allowColor = 1;

  void send(QString txt);

  QHostAddress getPeerAddress(void)
  {
    return peer_address_;
  }
  QHostAddress getPeerOriginalAddress(void)
  {
    if (proxy.isActive())
    {
      return proxy.getSourceAddress();
    }
    return getPeerAddress();
  }

  QString getPeerFullAddressString(void)
  {
    if (proxy.isActive())
    {
      return u"%1 via %2"_s.arg(getPeerOriginalAddress().toString()).arg(getPeerAddress().toString());
    }
    else
    {
      return getPeerOriginalAddress().toString();
    }
  }

  void setPeerAddress(QHostAddress address)
  {
    peer_address_ = address;
  }

  void setPeerPort(quint16 port)
  {
    peer_port_ = port;
  }

  QString getName(void);
  inline bool isEditing(void) const noexcept
  {
    return connected == Connection::states::EDITING ||
           connected == Connection::states::EDITING_V2 ||
           connected == Connection::states::WRITE_BOARD ||
           connected == Connection::states::EDIT_MPROG ||
           connected == Connection::states::SEND_MAIL ||
           connected == Connection::states::EXDSCR;
  }
  inline bool isPlaying(void) const noexcept
  {
    return connected == Connection::states::PLAYING;
  }
  [[nodiscard]] inline QString name(void) const { return name_; }
  inline QString name(QString s)
  {
    name_ = s;
    return s;
  }
  qint32 process_output(void);
  QString createBlackjackPrompt(void);
  QString createPrompt(void);
  void setOutput(QString output_buffer);
  void appendOutput(QString output_buffer);
  QByteArray getOutput(void) const;
  DCPtr dc_;

private:
  QHostAddress peer_address_ = {};
  quint16 peer_port_ = {};
  QString name_; /* Copy of the player name	*/
};

bool operator!(load_status_t ls);
class mprog_throw_type
{
public:
  qint32 target_mob_num;   // num of mob to recieve
  QString target_mob_name; // string used to find target name

  qint32 data_num; // number of catch call to activate on target
  qint32 delay;    // how qint32 until the mob gets it

  qint32 pitcher; // vnum of mob that threw the call
  qint32 opt;
  mprog_throw_type *next;
  bool mob;    // Mob or object.
  QString var; // temporary variable
  CharacterPtr actor;
  ObjectPtr obj;
  void *vo;
  CharacterPtr rndm; // $r

  // new mppause crap below..
  CharacterPtr tMob; // it should NOT throw it to another similar mob :P
  qint32 ifchecks;   // Let's hope noone nests more ifs than that.
  qint32 startPos;
  qint32 cPos;
  QString orig;
  // end mppause crap
};
class Leaderboard : public QObject
{
  Q_OBJECT
public:
  Leaderboard(QObject *parent);
  virtual ~Leaderboard();
  void check(void);
  void check_offline(void);
  void read_file(void);
  void write_file(QString filename);
  qint32 pdscore(CharacterPtr ch);
  void rename(QString oldname, QString newname);
  void setHP(quint32 placement, QString name, qint32 value);
  qint32 scan(CharacterPtr ch);

private:
  QString hpactivename[5];
  QString mnactivename[5];
  QString kiactivename[5];
  QString pkactivename[5];
  QString pdactivename[5];
  QString rdactivename[5];
  QString mvactivename[5];
  qint32 hpactive[5];
  qint32 mnactive[5];
  qint32 kiactive[5];
  qint32 pkactive[5];
  qint32 pdactive[5];
  qint32 rdactive[5];
  qint32 mvactive[5];
  QString hpactiveclassname[CLASS_MAX - 2][5];
  QString mnactiveclassname[CLASS_MAX - 2][5];
  QString kiactiveclassname[CLASS_MAX - 2][5];
  QString pkactiveclassname[CLASS_MAX - 2][5];
  QString pdactiveclassname[CLASS_MAX - 2][5];
  QString rdactiveclassname[CLASS_MAX - 2][5];
  QString mvactiveclassname[CLASS_MAX - 2][5];
  qint32 hpactiveclass[CLASS_MAX - 2][5];
  qint32 mnactiveclass[CLASS_MAX - 2][5];
  qint32 kiactiveclass[CLASS_MAX - 2][5];
  qint32 pkactiveclass[CLASS_MAX - 2][5];
  qint32 pdactiveclass[CLASS_MAX - 2][5];
  qint32 rdactiveclass[CLASS_MAX - 2][5];
  qint32 mvactiveclass[CLASS_MAX - 2][5];
  DCPtr dc_;
};
class song_info_type
{
public:
  quint8 beats_;                /* Waiting time after ki */
  position_t minimum_position_; /* min position for use */
  quint8 min_useski_;           /* minimum ki used */
  qint16 skill_num_;            /* skill number of the song */
  qint16 targets_;              /* Legal targets */
  qint16 rating_;               /* Rating for orchestrate */
  SING_FUN *song_pointer_;      /* function to call */
  SING_FUN *exec_pointer_;      /* other function to call */
  SING_FUN *song_pulse_;        /* other other function to call */
  SING_FUN *intrp_pointer_;     /* other other function to call */
  qint32 difficulty_;

public:
  song_info_type(quint8 beats, position_t minimum_position, quint8 min_useski, qint16 skill_num,
                 qint16 targets, qint16 rating, SING_FUN *song_pointer, SING_FUN *exec_pointer, SING_FUN *song_pulse,
                 SING_FUN *intrp_pointer, qint32 difficulty)
      : beats_(beats), minimum_position_(minimum_position), min_useski_(min_useski), skill_num_(skill_num),
        targets_(targets), rating_(rating), song_pointer_(song_pointer), exec_pointer_(exec_pointer), song_pulse_(song_pulse),
        intrp_pointer_(intrp_pointer), difficulty_(difficulty)
  {
  }
  quint8 beats(void) const { return beats_; }
  position_t minimum_position(void) const { return minimum_position_; }
  quint8 min_useski(void) const { return min_useski_; }
  qint16 skill_num(void) const { return skill_num_; }
  qint16 targets(void) const { return targets_; }
  qint16 rating(void) const { return rating_; }
  SING_FUN *song_pointer(void) const { return song_pointer_; }
  SING_FUN *exec_pointer(void) const { return exec_pointer_; }
  SING_FUN *song_pulse(void) const { return song_pulse_; }
  SING_FUN *intrp_pointer(void) const { return intrp_pointer_; }
  qint32 difficulty(void) const { return difficulty_; }
};

class deny_data
{
public:
  DenyPtr next;
  qint32 vnum;
};

class ExtraDescription
{
public:
  QString keyword_ = {};         /* Keyword in look/examine          */
  QString description_ = {};     /* What to see                      */
  ExtraDescriptionPtr next = {}; /* Next in list                     */
};

auto &operator<<(auto &out, ExtraDescriptionPtr currdesc)
{
  while (currdesc)
  {
    out << "E\n";
    string_to_file(out, currdesc->keyword_);
    string_to_file(out, currdesc->description_);
    currdesc = currdesc->next;
  }
  return out;
}

class RoomDirection : public QObject
{
  Q_OBJECT
public:
  explicit RoomDirection(QObject *parent);
  QString general_description; /* When look DIR.                  */
  QString keyword;             /* for open/close                  */
  qint16 exit_info;            /* Exit info                       */
  CharacterPtr bracee;         /* This is who is bracing the door */
  qint16 key;                  /* Key's number (-1 for no key)    */
  qint16 to_room;              /* Where direction leeds (NOWHERE) */
  DCPtr dc_;
};

class follow_type
{
public:
  CharacterPtr follower;
};

class tempvariable
{
public:
  QString name;
  QString data;
  qint16 save = {}; // save or not
};

class affected_type
{
public:
  quint32 type = {};    /* The type of spell that caused ths      */
  qint16 duration = {}; /* For how long its effects will last      */
  qint32 duration_type = {};
  qint32 modifier = {};  /* This is added to apropriate ability     */
  qint32 location = {};  /* Tells which ability to change(APPLY_XXX)*/
  qint32 bitvector = {}; /* Tells which bits to set (AFF_XXX)       */
  QString caster = {};
  affected_typePtr next = {};
  CharacterPtr origin = {};
  CharacterPtr victim = {};
};

class songInfo
{
public:
  qint16 song_timer;  /* status for songs being sung */
  qint16 song_number; /* number of song being sung */
  QString song_data_;
};

class time_info_data
{
public:
  qint32 hours;
  qint32 day;
  qint32 month;
  qint32 year;
};

class Vault : public QObject
{
  Q_OBJECT
public:
  Vault(QObject *parent, QString owner = {}, quint32 size = {}, quint32 weight = {}, quint64 gold = {})
      : QObject(parent), dc_(qobject_cast<DC *>(parent)), owner_(owner), size_(size), weight_(weight), gold_(gold)
  {
  }
  void access_remove(QString name);
  void item_remove(ObjectPtr obj);
  void item_add(qint32 vnum);

  ObjectPtr get_obj(QString keyword, qint32 num);
  ObjectPtr get_unique_obj(QString keyword, qint32 num);
  vault_items_dataPtr get_item(QString keyword, qint32 num);
  vault_items_dataPtr get_unique_item(QString keyword, qint32 num);

  void sort(sorted_vault &sv);
  void save(void);

  QString owner_;
  quint32 size_;
  quint32 weight_;
  quint64 gold_;
  DCPtr dc_;

  QList<vault_access_dataPtr> access;
  QList<vault_items_data> items;
  operator bool() { return !owner_.isEmpty(); }
};

class mob_flag_data
{
public:
  qint32 value[MAX_MOB_VALUES]; /* Mob type-specific value numbers */
  mob_type_t type;              /* Type of mobile                     */
};

class mob_prog_act_list
{
public:
  mob_prog_act_list *next = {};
  QString buf_;
  CharacterPtr ch = {};
  ObjectPtr obj = {};
  void *vo = {};
};

class Mobile
{
public:
  qint32 nr = {};
  position_t default_pos = {};                // Default position for NPC
  qint8 last_direction = {};                  // Last direction the mobile went in
  quint32 attack_type = {};                   // Bitvector of damage type for bare-handed combat
  quint32 actflags[ACT_MAX / ASIZE + 1] = {}; // flags for NPC behavior

  qint16 damnodice = {};   // The number of damage dice's
  qint16 damsizedice = {}; // The size of the damage dice's

  QString fears = {}; /* will flee from ths person on sight     */
  QString hated = {}; /* List of PC's I hate */

  mob_prog_act_list *mpact = {}; // list of MOBProgs
  qint16 mpactnum = {};          // num
  qint32 last_room = {};         // Room rnum the mob was last in. Used
                                 // For !magic,!track changing flags_.
  class threat_data *threat = {};
  ResetCommandPtr reset = {};
  mob_flag_data mob_flags = {}; /* Mobile information               */
  bool paused = {};

  void setObject(ObjectPtr);
  ObjectPtr getObject(void);
  bool isObject(void);
  void save(auto &streamfpsave);
  void read(auto &streamfpsave);

private:
  ObjectPtr object = {};
};

class Character : public QObject, public MinimumEntity, public Entity
{
  Q_OBJECT
public:
  explicit Character(QObject *parent);
  enum Type
  {
    Undefined,
    Player,
    NPC,
    Object
  };
  Q_ENUM(Type)
  void setType(const Type type);
  auto getType(void) const -> Type;

  enum sex_t : qint8
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
  static const QList<qint32> wear_to_item_wear;
  static const QStringList class_names;

  MobilePtr mobdata;
  PlayerPtr player;
  ObjectPtr objdata;
  ConnectionPtr conn_;

  QString title_;

  sex_t sex = {};

  qint8 c_class = {};
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

  qint8 race = {};
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
      level_ = {};
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
  QString shortdesc_or_name(void)
  {
    if (short_description().isEmpty())
      return name();
    return short_description();
  }

  attribute_points_t str = {};
  attribute_points_t raw_str = {};
  attribute_points_t str_bonus = {};
  attribute_points_t intel = {};
  attribute_points_t raw_intel = {};
  attribute_points_t intel_bonus = {};
  attribute_points_t wis = {};
  attribute_points_t raw_wis = {};
  attribute_points_t wis_bonus = {};
  attribute_points_t dex = {};
  attribute_points_t raw_dex = {};
  attribute_points_t dex_bonus = {};
  attribute_points_t con = {};
  attribute_points_t raw_con = {};
  attribute_points_t con_bonus = {};

  qint8 conditions[3] = {}; // Drunk full etc.

  quint8 weight = {}; /* PC/NPC's weight */
  quint8 height = {}; /* PC/NPC's height */

  qint16 hometown = {}; /* PC/NPC home town */

  quint32 plat = {};                    /* Platinum                                */
  qint64 exp = {};                      /* The experience of the player            */
  quint32 immune = {};                  // Bitvector of damage types I'm immune to
  quint32 resist = {};                  // Bitvector of damage types I'm resistant to
  quint32 suscept = {};                 // Bitvector of damage types I'm susceptible to
  qint16 saves[SAVE_TYPE_MAX + 1] = {}; // Saving throw bonuses

  qint32 mana = {};
  qint32 max_mana = {}; /* Not useable                             */
  qint32 raw_mana = {}; /* before qint32 bonus                        */
  qint32 hit = {};
  qint32 max_hit = {}; /* Max hit for NPC                         */
  qint32 raw_hit = {}; /* before con bonus                        */

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
  bool decrementMove(move_t move_change = 1, QString message = u"You're too out of breath!"_s)
  {
    if (move_ >= move_change)
    {
      move_ -= move_change;
      return true;
    }
    else
    {
      move_ = {};
      if (!message.isEmpty())
      {
        sendln(message);
      }
    }
    return false;
  }

  qint32 raw_move = {};
  qint32 max_move = {}; /* Max move for NPC                        */
  qint32 ki = {};
  qint32 max_ki = {};
  qint32 raw_ki = {};

  qint16 alignment = {}; // +-1000 for alignments

  quint32 hpmetas = {};   // total number of times meta'd hps
  quint32 manametas = {}; // total number of times meta'd mana
  quint32 movemetas = {}; // total number of times meta'd moves
  quint32 acmetas = {};   // total number of times meta'd ac
  qint32 agemetas = {};   // number of years age has been meta'd

  qint16 hit_regen = {};  // modifier to hp regen
  qint16 mana_regen = {}; // modifier to mana regen
  qint16 move_regen = {}; // modifier to move regen
  qint16 ki_regen = {};   // modifier to ki regen

  qint16 melee_mitigation = {}; // modifies melee damage
  qint16 spell_mitigation = {}; // modified spell damage
  qint16 song_mitigation = {};  // modifies song damage
  qint16 spell_reflect = {};

  clan_id_t clan{}; /* Clan the character is in */

  qint16 armor = {};   // Armor class
  qint16 hitroll = {}; // Any bonus or penalty to the hit roll
  qint16 damroll = {}; // Any bonus or penalty to the damage roll

  qint16 glow_factor = {}; // Amount that the character glows

  ObjectPtr beacon = {}; /* pointer to my beacon */

  QList<songInfo> songs = {}; // Song list
                              //     qint16 song_timer = {};       /* status for songs being sung */
                              //     qint16 song_number = {};      /* number of song being sung */
                              //     QString song_data = {};        /* args for the songs */

  ObjectPtr equipment[MAX_WEAR] = {}; // Equipment List

  skill_list_t skills = {};         // Skills List
  QList<affected_typePtr> affected; // Affected by list
  QList<ObjectPtr> carrying;        // Inventory List

  qint16 poison_amount = {}; // How much poison damage I'm taking every few seconds

  qint16 carry_weight = {}; // Carried weight
  qint16 carry_items = {};  // Number of items carried

  QString hunting = {}; // Name of "track" target
  QString ambush = {};  // Name of "ambush" target

  CharacterPtr guarding = {};   // Pointer to who I am guarding
  follow_type *guarded_by = {}; // List of people guarding me

  quint32 affected_by[AFF_MAX / ASIZE + 1] = {}; // Quick reference bitvector for spell affects
  quint32 combat = {};                           // Bitvector for combat related flags (bash, stun, shock)
  quint32 misc = {};                             // Bitvector for logs/channels.  So possessed mobs can channel

  CharacterPtr fighting = {};      /* Opponent     */
  CharacterPtr next = {};          /* Next anywhere in game */
  CharacterPtr next_in_room = {};  /* Next in room */
  CharacterPtr next_fighting = {}; /* Next fighting */
  ObjectPtr altar = {};
  follow_type *followers = {}; /* List of followers */
  CharacterPtr master = {};    /* Who is character following? */
  QString group_name_;         /* Name of group */

  qint32 timer = {};           // Timer for update
  qint32 shotsthisround = {};  // Arrows fired this round
  qint32 spellcraftglyph = {}; // Used for spellcraft glyphs
  bool changeLeadBonus = {};
  qint32 curLeadBonus = {};
  qint32 cRooms = {}; // number of rooms consecrated/desecrated

  // TODO - see if we can move the "wait" timer from desc to character
  // since we need something to lag mobs too

  qint32 deaths = {}; /* deaths is reused for mobs as a
                   timer to check for WAIT_STATE */

  qint32 cID = {}; // character ID

  TimerPtr timerAttached = {};
  tempvariable *tempVariable = {};
  qint32 spelldamage = {};
  qint32 player_id = {};
  qint32 spec = {};
  DCPtr dc_;

  bool getDebug(void) const
  {
    return debug_;
  }
  void setDebug(bool state)
  {
    debug_ = state;
  }

  RoomDirectionPtr brace_at, brace_exit;
  time_t first_damage = {};
  quint64 damage_done = {};
  quint64 damages = {};
  time_t last_damage = {};
  quint64 damage_per_second = {};
  static const classes_t classes_;

  bool addGold(quint64 gold);
  bool save_pc_or_mob_data(auto &streamfpsave, Time tmpage);
  void add_command_lag(cmd_t cmd, qint32 lag);
  bool canPerform(const int_fast32_t &learned, QString failMessage = QString());
  qint32 char_to_store_variable_data(auto &streamfpsave);
  void display_string_list(QStringList list);
  bool charge_moves(qint32 skill, double modifier = 1);
  void check_maxes(void);
  qint32 check_charmiejoin(void);
  void add_memory(QString victim_name, QChar type);
  bool can_use_command(cmd_t cmd);
  ObjectPtr clan_altar(void);
  void do_inate_race_abilities(void);
  void do_on_login_stuff(void);
  void check_hw(void);
  void add_to_bard_list(void);
  time_info_data age(void);

  ReturnValue check_pursuit(CharacterPtr victim, QString dircommand);
  ReturnValue check_social(QString pcomm);
  ReturnValue command_interpreter(QString argument, bool procced = 0);
  ObjectPtr get_object_in_equip_vis(QString arg, ObjectPtr equipment[], qint32 *j, bool blindfighting);

  ReturnValue do_clanarea(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_config(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_experience(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_split(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);

  ReturnValue do_edit_generic_show(QString value, QString fieldname, QString desc, QStringList arguments, cmd_t cmd = cmd_t::DEFAULT);

  ReturnValue do_edit_generic(auto getfnc, auto setfnc, QString fieldname, QString desc, QStringList arguments, cmd_t cmd = cmd_t::DEFAULT)
  {
    if (arguments.isEmpty())
      return do_edit_generic_show(getfnc(), fieldname, desc, arguments, cmd);

    setfnc(arguments.join(' '));
    sendln(u"%1 set to '%2'."_s.arg(fieldname).arg(getfnc()));
    return ReturnValue::eSUCCESS;
  }

  ReturnValue do_edit_generic(auto *entity, auto getfnc, auto setfnc, QString fieldname, QString desc, QStringList arguments, cmd_t cmd = cmd_t::DEFAULT)
  {
    if (!entity)
      return ReturnValue::eFAILURE;

    auto getter = [entity, getfnc]()
    {
      return (*entity.*(getfnc))();
    };
    auto setter = [entity, setfnc](QString value)
    {
      return (*entity.*(setfnc))(value);
    };

    return do_edit_generic(getter, setter, fieldname, desc, arguments, cmd);
  }

  ReturnValue do_edit_generic(auto **field, QString fieldname, QString desc, QStringList arguments, cmd_t cmd = cmd_t::DEFAULT)
  {
    if (!field)
      return ReturnValue::eFAILURE;

    auto getter = [field]()
    {
      return QString(*field);
    };
    auto setter = [field](QString value)
    {
      *field = value;
    };
    return do_edit_generic(getter, setter, fieldname, desc, arguments, cmd);
  }

  template <typename T>
  ReturnValue do_edit_generic_numeric(auto *entity, auto getfnc, auto setfnc, QString fieldname, QString desc, QStringList arguments, cmd_t cmd = cmd_t::DEFAULT)
  {
    if (!entity)
      return ReturnValue::eFAILURE;

    if (arguments.isEmpty())
    {
      do_edit_generic_show((*entity.*(getfnc))(), fieldname, desc, arguments, cmd);
      sendln(u"$3Valid types$R:"_s);

      quint64 longest_typename{};
      for (const auto &type : item_types)
        if (longest_typename < type.length())
          longest_typename = type.length();

      quint8 column = 1;
      for (const auto &type : item_types)
      {
        send(u"%1"_s.arg(type.toLower(), longest_typename));
        if (!(column++ % 3))
          sendln();
        else
          send(u"   "_s);
      }
      sendln();
      return ReturnValue::eFAILURE;
    }

    if ((!(*entity.*(setfnc))(arguments.join(' ').toLower())))
    {
      sendln(u"Invalid input."_s);
      return ReturnValue::eFAILURE;
    }
    sendln(u"%1 set to '%2'."_s.arg(fieldname).arg(arguments.join(' ').toLower()));
    return ReturnValue::eSUCCESS;
  }

  ReturnValue do_oedit(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_oedit_new(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_oedit_delete(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_zsave(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_wizhelp(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_goto(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_save(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_search(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_identify(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_recall(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_cdeposit(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue generic_command(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_sockets(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_who(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_beep_set(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_bard_song_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_brief(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_news_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_ascii_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_damage_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_charmiejoin_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_guide(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_lfg_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_notax_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_guide_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_summon_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_nodupekeys_toggle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_compact(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_anonymous(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_ansi(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_vt100(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_wimpy(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_pager(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_autoeat(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_shutdow(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_shutdown(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_linkload(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_rename_char(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_auction(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_test(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_tell(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_wake(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_rescue(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_rage(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_join(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_outcast(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_backstab(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_pview(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_snoop(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_zap(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_track(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_hit(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_ambush(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_botcheck(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_mpsettemp(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_bestow(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_alias(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_pets(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_kick(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_givealldot(QString name, QString target, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_give(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_drink(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_eat(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_ban(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_unban(QStringList rguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue do_force(QStringList arguments = {}, cmd_t cmd = cmd_t::FORCE);
  ReturnValue do_open(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT);
  auto do_arena(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT) -> ReturnValue;
  auto do_notitle(QStringList arguments = {}, cmd_t cmd = cmd_t::DEFAULT) -> ReturnValue;
  auto do_arena_info(QStringList arguments) -> ReturnValue;
  auto do_arena_start(QStringList arguments) -> ReturnValue;
  auto do_arena_join(QStringList arguments) -> ReturnValue;
  auto do_arena_cancel(QStringList arguments) -> ReturnValue;
  auto do_arena_usage(QStringList arguments) -> ReturnValue;

  ReturnValue wake(CharacterPtr victim = {});
  ReturnValue oprog_command_trigger(QString command, QString arguments);
  ReturnValue save(cmd_t cmd = cmd_t::DEFAULT);
  ReturnValue special(QString arg, cmd_t cmd = cmd_t::DEFAULT);

  void show_obj_to_char(ObjectPtr object, qint32 mode);
  void list_obj_to_char(ObjectPtr list, qint32 mode, bool show);
  void list_char_to_char(CharacterPtr list, qint32 mode);

  bool mprog_seval(QString lhs, QString opr, QString rhs);
  QString getTemp(QString name);

  QString get_random_hate(void);
  CharacterPtr getVisiblePlayer(QString name);
  CharacterPtr getVisibleCharacter(QString name);
  ObjectPtr getVisibleObject(QString name);
  QString getStatDiff(qint32 base, qint32 random, bool swapcolors = false);
  void tell_history(CharacterPtr sender, QString message);
  void gtell_history(CharacterPtr sender, QString message);
  void setPOSFighting();
  qint32 getHP(void);
  void setHP(qint32 hp, CharacterPtr causer = {});
  void addHP(qint32 hp, CharacterPtr causer = {});
  void removeHP(qint32 dam, CharacterPtr causer = {});
  void fillHP(void);
  void fillHPLimit(void);
  void send(QString buffer);
  void sendln(QString buffer = {})
  {
    send(buffer.append(u"\r\n"_s));
  }
  ReturnValue tell(CharacterPtr, QString);
  void sendRaw(QString);
  QList<CharacterPtr> getFollowers(void);
  void setPlayerLastMob(u_int64_t mobvnum);

  void swapSkill(skill_t oldSkill, skill_t newSkill);
  void setSkillMin(skill_t skill, qint32 value);
  char_skill_data &getSkill(skill_t skill);
  void setSkill(skill_t, qint32 value = 0);
  void upSkill(skill_t skillnum, qint32 learned = 1);

  QString getSetting(QString key, QString defaultValue = QString());
  QString getSettingAsColor(QString key, QString defaultValue = QString());

  bool isMortalPlayer(void) const;
  bool isImmortalPlayer(void) const;
  bool isImplementerPlayer(void) const;
  bool isPlayer(void) const;
  bool isNonPlayer(void) const;
  bool isObjectProgram(void) const;

  quint64 getGold(void);
  quint64 &getGoldReference(void);
  void setGold(quint64 gold);

  bool multiplyGold(double mult);
  bool removeGold(quint64 gold);
  qint32 store_to_char_variable_data(auto &streamfpsave);

  bool load_charmie_equipment(QString name, bool previous = false);
  qint32 has_skill(skill_t skill);
  affected_typePtr affected_by_spell(quint32 skill);
  bool skill_success(CharacterPtr victim, qint32 skillnum, qint32 mod = 0);
  qint32 skillmax(qint32 skill, qint32 eh);
  QChar charthing(qint32 known, qint32 skill, qint32 maximum);
  void output_praclist(CharacterClassSkill *skilllist);
  qint32 skills_guild(const QString arg, CharacterPtr owner);
  qint32 get_stat(attribute_t stat);
  qint32 get_stat_bonus(qint32 stat);
  void skill_increase_check(qint32 skill, qint32 learned, qint32 difficulty);
  void verify_max_stats(void);
  qint32 get_max(qint32 skill);
  qint32 learn_skill(qint32 skill, qint32 amount, qint32 maximum);
  CharacterClassSkill *get_skill_list(void);
  CharacterPtr get_char_room_vis(QString name);
  CharacterPtr get_rand_other_char_room_vis(void);
  qint32 hit_gain(position_t position, bool improve = true);
  qint32 hit_gain(void)
  {
    return hit_gain(getPosition(), true);
  }

  qint32 hit_gain_lookup(void)
  {
    return hit_gain(getPosition(), false);
  }
  qint32 mana_gain_lookup(void);
  qint32 move_gain_lookup(qint32 extra = 0);
  qint32 ki_gain_lookup(void);
  static QString position_to_string(position_t position)
  {
    return position_types.value(static_cast<qint64>(position));
  }

  QString getPositionQString(void)
  {
    return position_to_string(getPosition());
  }
  CharacterPtr get_active_pc_vis(QString name);
  void swap_hate_memory(void);
  void set_hw(void);
  void roll_and_display_stats(void);
  void remove_from_bard_list(void);
  qint64 moves_exp_spent(void);
  qint64 moves_plats_spent(void);
  qint64 hps_exp_spent(void);
  qint64 hps_plats_spent(void);
  qint64 mana_exp_spent(void);
  qint64 mana_plats_spent(void);
  qint32 meta_get_stat_exp_cost(attribute_t stat);
  qint32 meta_get_stat_plat_cost(attribute_t targetstat);
  void meta_list_stats(void);
  quint64 meta_get_moves_exp_cost(void);
  quint64 meta_get_moves_plat_cost(qint32 amount);
  quint64 meta_get_hps_exp_cost(void);
  quint64 meta_get_hps_plat_cost(qint32 amount);
  quint64 meta_get_mana_exp_cost(void);
  quint64 meta_get_mana_plat_cost(qint32 amount);
  quint64 meta_get_ki_plat_cost(void);
  qint32 meta_get_ki_exp_cost(void);
  void undo_race_saves(void);
  bool is_race_applicable(qint32 race);
  bool would_die(void);
  void set_heightweight(void);
  QString race_message(qint32 race);

  qint32 hands_are_free(qint32 number);
  qint32 recheck_height_wears(void);
  void heightweight(bool add);
  bool isPlayerObjectThief(void) { return affected_by_spell(PLAYER_OBJECT_THIEF); }
  bool isPlayerGoldThief(void) { return affected_by_spell(PLAYER_GOLD_THIEF); }
  bool isPlayerCantQuit(void) { return affected_by_spell(PLAYER_CANTQUIT); }
  bool allowColor(void);

  QString parse_prompt_variable(QString variable, PromptVariableType type = PromptVariableType::Advanced);
  QString get_parsed_legacy_prompt_variable(QString var);
  QString calc_name(bool use_color = false);

  QString getPrompt(void) const;
  void setPrompt(QString prompt);
  QString getLastPrompt(void) const;
  void setLastPrompt(QString prompt);

  QString createPrompt(void);
  QString createBlackjackPrompt(void);
  void sendBlackjackPrompt(void);
  void prog_error(QString error_message);
  bool isNowhere(void);
  void vault_access(QString owner);
  void vault_myaccess(QString owner);
  void vault_balance(QString owner);
  void vault_stats(QString owner);
  void add_vault_access(QString name, VaultPtr vault);
  void remove_vault_access(QString name, VaultPtr vault);
  void vault_list(QString owner);
  void load_golem_data(qint32 golemtype);
  qint32 mprog_greet_trigger(void);
  qint32 mprog_can_see_trigger(CharacterPtr mob);
  qint32 mprog_speech_trigger(const QString txt);
  qint32 oprog_can_see_trigger(ObjectPtr item);
  qint32 oprog_speech_trigger(const QString txt);
  qint32 oprog_act_trigger(QString txt);
  qint32 oprog_greet_trigger(void);
  qint32 oprog_load_trigger(void);
  qint32 oprog_weapon_trigger(ObjectPtr item);
  qint32 oprog_armour_trigger(ObjectPtr item);
  bool isTank(void);
  void save_char_obj(void);

  bool equip_char(ObjectPtr obj, qint32 pos, bool flag = false);
  ObjectPtr unequip_char(qint32 pos, bool flag = false);
  void vault_withdraw(quint32 amount, QString owner);
  void vault_deposit(quint32 amount, QString owner);
  void vault_get(QString object_keyword, QString owner);
  void vault_put(QString object_keyword, QString owner);
  void my_vault_access(void);
  void vault_sell(QString object_keyword, QStringList arguments);
  void vault_cost(QString object_keyword, QStringList arguments);
  qint32 vault_search(QString object_keyword);

private:
  Type type_ = Type::Undefined;
  gold_t gold_ = {}; /* Money carried */
  level_t level_ = {};
  bool debug_ = false;
  move_t move_ = {};
  QString name_; // Keyword 'kill X'
  position_t position_ = {};
};

class snoop_data
{
public:
  CharacterPtr snooping;
  CharacterPtr snoop_by;
};
class Sockets
{
public:
  Sockets(CharacterPtr ch = {}, QString searchkey = "");
  auto getIPs(void) const { return IPs_; }
  auto getConnections(void) const { return connections_; }
  quint64 getLongestNameSize(void) const { return longest_name_size_; }
  quint64 getLongestIPSize(void) const { return longest_IP_size_; }
  quint64 getLongestConnectionStateSize(void) const { return longest_connection_state_size_; }
  quint64 getLongestIdleSize(void) const { return longest_idle_size_; }

private:
  QMap<QString, quint64> IPs_;
  QList<ConnectionPtr> connections_;
  qsizetype longest_name_size_ = {};
  qsizetype longest_IP_size_ = {};
  qsizetype longest_connection_state_size_ = {};
  qsizetype longest_idle_size_ = {};
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
class char_file_u4
{
public:
  char_file_u4();
  Character::sex_t sex = {}; /* Sex */
  qint8 c_class = {};        /* Class */
  qint8 race = {};           /* Race */
  qint8 level = {};          /* Level */

  qint8 raw_str = {};
  qint8 raw_intel = {};
  qint8 raw_wis = {};
  qint8 raw_dex = {};

  qint8 raw_con = {};
  qint8 conditions[3] = {};

  quint8 weight = {};
  quint8 height = {};
  qint16 hometown = {};

  quint32 gold = {};
  quint32 plat = {};
  qint64 exp = {};
  quint32 immune = {};
  quint32 resist = {};
  quint32 suscept = {};

  qint32 mana = {};     // current
  qint32 raw_mana = {}; // max without eq/stat bonuses
  qint32 hit = {};
  qint32 raw_hit = {};
  qint32 move = {};
  qint32 raw_move = {};
  qint32 ki = {};
  qint32 raw_ki = {};

  qint16 alignment = {};
  qint16 unused1 = {};

  quint32 hpmetas = {}; // Used by familiars too... why not.
  quint32 manametas = {};
  quint32 movemetas = {};

  qint16 armor = {}; // have to save these since mobs have different bases
  qint16 hitroll = {};

  qint16 damroll = {};
  qint16 unused2 = {};

  qint32 afected_by = {};
  qint32 afected_by2 = {};
  quint32 misc = {}; // channel flags

  qint16 clan = {};
  qint16 unused3 = {};
  qint32 load_room = {}; // Which room to place character in

  quint32 acmetas = {};
  qint32 agemetas = {};
  qint32 extra_ints[3] = {}; // available just in case
}
__attribute__((packed));

void SET_BIT(auto var, auto bit)
{
  (var) = (var) | (bit);
}

class DC_EXPORT DC : public QCoreApplication
{
  Q_OBJECT
public:
  // Obj proc types
  static const QStringList obj_types;

  class config
  {
  public:
    config(int &argc, char **argv)
        : argc_(argc), argv_(argv) {}
    qint32 argc_ = {};
    char **argv_ = {};
    bool sql = true;
    port_list_t ports;
    bool allow_imp_password = false;
    bool verbose_mode = false;
    bool test_mobs = false;
    bool test_objs = false;
    bool test_world = false;
    bool bport = false;
    bool check_syntax = false;
    bool stderr_timestamp = true;
    bool allow_multi = false;
    bool allow_newstatsys = false;
    bool testing = false;
    QString library_directory = DEFAULT_LIBRARY_PATH;
    QString leaderboard_check;
    QString implementer;
  } cf;

  enum LogChannel
  {
    LOG_BUG = 1U,
    LOG_PRAYER = 1U << 1,
    LOG_GOD = 1U << 2,
    LOG_MORTAL = 1U << 3,
    LOG_SOCKET = 1U << 4,
    LOG_MISC = 1U << 5,
    LOG_PLAYER = 1U << 6,
    CHANNEL_GOSSIP = 1U << 7,
    CHANNEL_AUCTION = 1U << 8,
    CHANNEL_INFO = 1U << 9,
    CHANNEL_TRIVIA = 1U << 10,
    CHANNEL_DREAM = 1U << 11,
    CHANNEL_CLAN = 1U << 12,
    CHANNEL_NEWBIE = 1U << 13,
    CHANNEL_SHOUT = 1U << 14,
    LOG_WORLD = 1U << 15,
    LOG_ARENA = 1U << 16,
    LOG_CLAN = 1U << 17,
    LOG_WARNING = 1U << 18,
    LOG_HELP = 1U << 19,
    LOG_DATABASE = 1U << 20,
    LOG_OBJECTS = 1U << 21,
    CHANNEL_TELL = 1U << 22,
    CHANNEL_HINTS = 1U << 23,
    LOG_VAULT = 1U << 24,
    LOG_QUEST = 1U << 25,
    LOG_DEBUG = 1U << 26
  };
  Q_ENUM(LogChannel);

  static constexpr room_t SORPIGAL_BANK_ROOM = 3005;
  static constexpr room_t NOWHERE = 0ULL;
  static constexpr vnum_t INVALID_VNUM = -1ULL;
  static constexpr vnum_t INVALID_RNUM = -1ULL;
  static constexpr quint64 PASSES_PER_SEC = 100;
  static constexpr quint64 PULSE_TIMER = 1 * PASSES_PER_SEC;
  static constexpr quint64 PULSE_MOBILE = 4 * PASSES_PER_SEC;
  static constexpr quint64 PULSE_OBJECT = 4 * PASSES_PER_SEC;
  static constexpr quint64 PULSE_VIOLENCE = 2 * PASSES_PER_SEC;
  static constexpr quint64 PULSE_BARD = 1 * PASSES_PER_SEC;
  static constexpr quint64 PULSE_TENSEC = 10 * PASSES_PER_SEC;
  static constexpr quint64 PULSE_WEATHER = 45 * PASSES_PER_SEC;
  static constexpr quint64 PULSE_TIME = 60 * PASSES_PER_SEC;
  static constexpr quint64 PULSE_REGEN = 15 * PASSES_PER_SEC;
  static constexpr quint64 PULSE_SHORT = 1; // Pulses all the time.
  static constexpr level_t MAX_MORTAL_LEVEL = 60ULL;
  static constexpr quint64 PER_IP_CONNECTION_LIMIT = 20;
  static const QString HINTS_FILE_NAME;
  static const QString DEFAULT_LIBRARY_PATH;
  static const QStringList connected_states;
  static const QList<bestowable_god_commands_type> bestowable_god_commands;
  static const QList<QList<qint32>> mob_race_mod;
  static const QList<class set_data> set_list;
  static const QString menu;

  QMap<CharacterPtr, ReservationPtr> wait_for_write;
  QList<class game_portal> game_portals_;
  QList<ConnectionPtr> connections_; // master desc list
  server_descriptor_list_t server_descriptor_list;
  client_descriptor_list_t client_descriptor_list;
  character_list_t character_list;
  obj_list_t active_obj_list;
  obj_list_t obj_free_list;
  QSet<CharacterPtr> shooting_list_;
  special_function_list_t mob_non_combat_functions;
  special_function_list_t mob_combat_functions;
  special_function_list_t obj_non_combat_functions;
  special_function_list_t obj_combat_functions;
  SSH::SSH ssh;
  fd_set input_set = {};
  fd_set output_set = {};
  fd_set exc_set = {};
  fd_set null_set = {};
  zones_t zones_ = {};
  QMap<room_t, Room> rooms;
  World world;
  clan_list_t clan_list_;
  QList<ObjectIndexPtr> obj_index_;

  world_file_list_item *world_file_list = {}; // List of the world files
  world_file_list_item *mob_file_list = {};   // List of the mob files
  world_file_list_item *obj_file_list = {};   // List of the obj files
  QList<ObjectPtr> object_list_ = {};         // the global linked list of obj's
  QList<PulsePtr> bard_list = {};             // global l-list of bards
  qint32 top_of_helpt = {};                   // top of help index table
  qint32 new_top_of_helpt = {};               // top of help index table
  room_t top_of_world_alloc = {};             // index of last alloc'd memory in world
  room_t top_of_world = {};
  qint32 total_rooms = {}; // total amount of rooms in memory
  AuctionHouse TheAuctionHouse = AuctionHouse(u"auctionhouse"_s, this);
  QList<wizlist_info> wizlist;
  QMap<QString, redeem_t> redeem_sessions = {};
  help_index_t help_index_;
  QList<TimerPtr> timer_list;
  QList<hunt_dataPtr> hunt_list_ = {};
  QList<hunt_itemsPtr> hunt_items_list_ = {};
  ObjectPtr obj_proto;
  qint16 frozen_start_room = 1;

  static QString getBuildVersion();
  static QString getBuildTime();
  static DCPtr getInstance();
  zone_t getRoomZone(room_t room_number);
  QString getZoneName(zone_t zone_key);
  static void setZoneClanOwner(zone_t zone_key, qint32 clan_key);
  static void setZoneClanGold(zone_t zone_key, gold_t gold);
  static void setZoneTopRoom(zone_t zone_key, room_t room_key);
  static void setZoneBottomRoom(zone_t zone_key, room_t room_key);
  static void setZoneModified(zone_t zone_key);
  static void setZoneNotModified(zone_t zone_key);
  static void incrementZoneDiedTick(zone_t zone_key);
  static void resetZone(zone_t zone_key, Zone::ResetType reset_type = Zone::ResetType::normal);
  void save_corpses(void);
  bool validateName(QString name);
  bool IS_ARENA(auto room)
  {
    return isSet(world[room].room_flags, ARENA);
  }
  void assign_rooms(void);
  void assign_objects(void);
  bool on_forbidden_name_list(QString name);
  void assign_one_mob_non(qint32 vnum, special_function func);
  void assign_one_mob_com(qint32 vnum, special_function func);
  void assign_one_obj_non(qint32 vnum, special_function func);
  void assign_one_obj_com(qint32 vnum, special_function func);
  void boot_the_shops(void);
  void boot_player_shops(void);
  void assign_the_shopkeepers(void);
  void assign_the_player_shopkeepers(void);
  void assign_non_combat_procs(void);
  void assign_combat_procs(void);
  qint32 count_controlled_areas(qint32 clan);
  ObjectPtr create_obj_new(void);
  qint32 corpse_save(ObjectPtr obj, FILE *stream, qint32 location, bool recurse_this_tree);

  ClanPtr get_clan(qint32 nClan);
  ClanPtr get_clan(CharacterPtr ch);
  void add_clan(ClanPtr new_new_clan);
  void add_clan_member(ClanPtr theClan, ClanMemberPtr new_new_member);
  void add_clan_member(ClanPtr theClan, CharacterPtr ch);
  void remove_clan_member(ClanPtr theClan, CharacterPtr ch);
  QString get_clan_name(ClanPtr clan);

  void mprog_read_programs(auto &stream, qint32 i, bool ignore)
  {
    QChar letter{};
    MobileProgramPtr program;
    for (;;)
    {
      stream >> letter;

      if (letter == '|')
      {
        break;
      }
      else if (letter != '>' && letter != '\\')
      {
        logentry(u"Load_mobiles: vnum %1 MOBPROG character"_s.arg(i));
        return;
      }
      auto word = fread_word(stream);
      auto type = MobileProgram::name_to_type(word);
      switch (type)
      {
      case ERROR_PROG:
        logentry(u"Load_mobiles: vnum %1 MOBPROG type."_s.arg(i));
        return;
      case IN_FILE_PROG:
        mprog_file_read(fread_string(stream, 1), i);
        break;
      default:
        if (!ignore)
        {
          if (letter == '>')
            SET_BIT(mob_index_[i]->progtypes_, type);
          else
            SET_BIT(obj_index_[i]->progtypes_, type);
        }
        program = MobileProgramPtr(new MobileProgram(this));
        program->type(type);
        program->arglist(fread_string(stream, false));
        program->comlist(fread_string(stream, false));
        if (!ignore)
        {
          if (letter == '>')
            mob_index_[i]->programs_.push_back(program); // when we write them, we write last first
          else
            obj_index_[i]->programs_.push_back(program);
        }
      }
      break;
    }
  }

  bool write_corpse_to_disk(auto &stream, ObjectPtr obj, qint32 locate)
  {
    /* This is basically Patrick's my_obj_save_to_disk function with    */
    /* a few minor tweaks to make it work for corpses. Basically it     */
    /* writes one object out to the corpse file every time it is called.*/
    /* It can handle regular obj's and XAP objects.                     */

    qint32 counter;
    ExtraDescriptionPtr ex_desc;
    // QString buf2;

    auto action_description = obj->ActionDescription().remove('\r');
    dc_fprintf(stream,
               "#%lu\n"
               "%d %d %d %d %d %u %d %d\n",
               GET_OBJ_VNUM(obj),
               locate,
               GET_OBJ_VAL(obj, 0),
               GET_OBJ_VAL(obj, 1),
               GET_OBJ_VAL(obj, 2),
               GET_OBJ_VAL(obj, 3),
               GET_OBJ_EXTRA(obj),
               GET_OBJ_VROOM(obj),  /*vroom is the virtual room a corpse*/
               GET_OBJ_TIMER(obj)); /* was created in. See make_corpse */

    if (!(IS_OBJ_STAT(obj, ITEM_UNIQUE_SAVE)))
    {
      return 1;
    }
    dc_fprintf(stream,
               "XAP\n"
               "%s~\n"
               "%s~\n"
               "%s~\n"
               "%s~\n"
               "%d %d %d %d %d\n",
               !obj->name().isEmpty() ? qPrintable(obj->name()) : "undefined",
               qPrintable(obj->short_description()) ? qPrintable(obj->short_description()) : "undefined",
               !obj->long_description().isEmpty() ? qPrintable(obj->long_description()) : "undefined",
               obj->ActionDescription().remove('\r'),
               GET_OBJ_TYPE(obj),
               GET_OBJ_WEAR(obj).toInt(),
               (GET_OBJ_WEIGHT(obj) < 0 ? 0 : GET_OBJ_WEIGHT(obj)),
               GET_OBJ_COST(obj), obj->num_affects);
    /* Do we have affects? */
    for (counter = {}; counter < obj->num_affects; counter++)
      if (obj->affected[counter].modifier)
        dc_fprintf(stream, "A\n"
                           "%d %d\n",
                   obj->affected[counter].location,
                   obj->affected[counter].modifier);

    /* Do we have extra descriptions? */
    if (obj->ex_description)
    { /*. Yep, save them too . */
      for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next)
      {
        /*. Sanity check to prevent nasty protection faults . */
        if (ex_desc->keyword_.isEmpty() || ex_desc->description_.isEmpty())
        {
          continue;
        }
        dc_fprintf(stream, "E\n"
                           "%s~\n"
                           "%s~\n",
                   qPrintable(ex_desc->keyword_),
                   ex_desc->description_.remove('\r'));
      }
    }
    return 1;
  }

  ObjectPtr read_object(qint32 nr, auto &stream, bool ignore)
  {
    qint32 loc{}, mod = {};

    QString chk;

    if (nr < 0)
    {
      return 0;
    }

    ObjectPtr obj = new Object(this);
    clear_object(obj);

    /* *** QString data *** */
    // read it, add it to the hsh table, free it
    // that way, we only have one copy of it in memory at any time

    obj->name(fread_string(stream));

    qDebug("%s", qPrintable(u"Object name: %1"_s.arg(obj->name())));
    obj->short_description(fread_string(stream));
    if (obj->short_description().length() >= MAX_OBJ_SDESC_LENGTH)
    {
      logf(IMMORTAL, DC::LogChannel::LOG_BUG, "read_object: vnum %d short_description too long.", obj_index_[nr].vnum());
    }

    obj->long_description(fread_string(stream));

    obj->ActionDescription(fread_string(stream));
    stream.skipWhiteSpace();
    if (!obj->ActionDescription().isEmpty() && !obj->ActionDescription()[0].isNull() && (obj->ActionDescription()[0] < ' ' || obj->ActionDescription()[0] > '~'))
    {
      logentry(u"read_object: vnum %1 action description [%2] removed."_s.arg(obj_index_[nr].vnum()).arg(obj->ActionDescription()));
      obj->ActionDescription(QString());
    }
    obj->table = {};
    currentVNUM(nr);
    currentName(obj->name());
    currentType("Object");
    obj->flags_.type_flag = fread_int<decltype(obj->flags_.size)>(stream);
    obj->flags_.extra_flags = fread_int<decltype(obj->flags_.extra_flags)>(stream);
    obj->flags_.wear_flags = fread_bitvector<ObjectPositions>(stream);
    obj->flags_.size = fread_int<decltype(obj->flags_.size)>(stream);

    obj->flags_.value[0] = fread_int<object_value_t>(stream);
    obj->flags_.value[1] = fread_int<object_value_t>(stream);
    obj->flags_.value[2] = fread_int<object_value_t>(stream);
    obj->flags_.value[3] = fread_int<object_value_t>(stream);
    obj->flags_.eq_level = fread_int<decltype(obj->flags_.eq_level)>(stream, 0, IMPLEMENTER);

    obj->flags_.weight = fread_int<decltype(obj->flags_.weight)>(stream);
    obj->flags_.cost = fread_int<decltype(obj->flags_.cost)>(stream);
    obj->flags_.more_flags = fread_int<decltype(obj->flags_.more_flags)>(stream);
    /* currently not stored in object file */
    obj->flags_.timer = {};

    obj->ex_description = {};
    obj->affected = {};
    obj->num_affects = {};
    /* *** other flags *** */

    if (nr == 2866 && !obj->ActionDescription().isEmpty() && obj->ActionDescription()[0] == 'P')
    {
      qDebug("Debug point");
    }

    qDebugQTextStreamLine(stream, "read_object(), before stream >> chk >> Qt::ws");
    stream >> chk >> Qt::ws;
    qDebugQTextStreamLine(stream, "read_object(), after stream >> chk >> Qt::ws");
    qDebug() << "First chk " << chk;
    ExtraDescriptionPtr new_new_descr = {};
    qint64 current_pos = {};
    QString current_line = {};
    while (!chk.isEmpty() && chk != "S")
    {
      bool ok = false;
      switch (chk[0].toLatin1())
      {
      case 'E':
        qDebugQTextStreamLine(stream, "Type E before first fread_string");
        new_new_descr = new ExtraDescription;

        new_new_descr->keyword_ = fread_string(stream);

        qDebugQTextStreamLine(stream, "Type E before second fread_string");

        new_new_descr->description_ = fread_string(stream);

        qDebugQTextStreamLine(stream, "Type E after second fread_string");

        new_new_descr->next = obj->ex_description;
        obj->ex_description = new_new_descr;
        break;

      case '\\':
        qDebugQTextStreamLine(stream, "before seek: ");
        ok = stream.seek(stream.pos() - 1);
        if (!ok)
        {
          qFatal("Failed to seek -1 in read_object");
        }

        qDebugQTextStreamLine(stream, "after seek: ");

        mprog_read_programs(stream, nr, ignore);

        qDebugQTextStreamLine(stream, "after mprog_read_programs seek: ");
        break;

      case 'A':
        // these are only two members of obj_affected_type, so nothing else needs initializing
        loc = fread_int<decltype(loc)>(stream);
        mod = fread_int<decltype(mod)>(stream, -1000, 1000);
        add_obj_affect(obj, loc, mod);
        break;

      default:
        logentry(u"Illegal obj addon flag [%1] in obj [%2]."_s.arg(chk).arg(obj->name()), IMPLEMENTER, DC::LogChannel::LOG_BUG);
        break;
      } // switch
        // read in next flag
      assert(stream.status() == QTextStream::Status::Ok);
      stream >> chk >> Qt::ws;
      assert(stream.status() == QTextStream::Status::Ok);
      qDebug() << "subsequent chk [" << chk << "]";
    }

    obj->in_room = DC::NOWHERE;
    obj->next_skill = {};
    obj->next_content = {};
    obj->carried_by = {};
    obj->equipped_by = {};
    obj->in_obj = {};
    obj->contains = {};
    obj->item_number = nr;

    // Keys will now save for up to 24 hours. If there are any with
    // ITEM_NOSAVE that flag will be removed.
    if (IS_KEY(obj))
    {
      SET_BIT(obj->flags_.more_flags, ITEM_24H_SAVE);
      REMOVE_BIT(obj->flags_.extra_flags, ITEM_NOSAVE);
    }

    return obj;
  }

  cDeckPtr create_deck(qint32 decks);

  void signal_setup(void);
  qint32 new_descriptor(qint32 s);
  void check_idle_passwords(void);
  void init_heartbeat(void);
  void report_debug_logging();
  void pulse_takeover(void);
  void zone_update(void);
  void point_update(void); /* In limits.c */
  void food_update(void);  /* In limits.c */
  void mobile_activity(void);
  void update_corpses_and_portals(void);
  void perform_violence(void);
  void time_update(void);
  void weather_update(void);
  void pulse_command_lag(void);
  void checkConsecrate(qint32);
  void another_hour(qint32 mode);
  void weather_change(void);

  void load_messages(const QString file, qint32 base = 0);
  void boot_social_messages(void);
  void assign_clan_rooms(void);
  void find_unordered_objects(void);
  void boot_clans(void);
  void update_make_camp_and_leadership(void);
  qint32 _parse_name(const QString arg, QString name);

  void pulse_table_bj(CasinoTablePtr tbl, qint32 recall = 0);
  void reset_table(CasinoTablePtr tbl);
  void nextturn(CasinoTablePtr tbl);
  void bj_dealer_ai(varg_t arg1, void *arg2, void *arg3);
  void add_timer_bj_dealer(CasinoTablePtr tbl);
  void addtimer(TimerPtr add);
  qint32 hand_number(CasinoPlayerPtr plr);
  qint32 hands(CasinoPlayerPtr plr);
  bool charExists(CharacterPtr ch);
  void reel_spin(varg_t, void *, void *);

  ReturnValue save_boards(void);
  bool is_forbidden(QString name);
  ObjectPtr getObject(vnum_t vnum);
  void findLibrary(void);
  qint32 create_one_room(CharacterPtr ch, qint32 vnum);
  void update_wizlist(CharacterPtr ch);
  void do_godlist(void);
  void write_wizlist(QString filename);
  explicit DC(QString argv);
  explicit DC(config c);
  void setup(void);
  DC(const DC &) = delete; // non-copyable
  DC(DC &&) = delete;      // and non-movable
  DC &operator=(const DC &) = delete;
  DC &operator=(DC &&) = delete;
  void main_loop2(void);
  void removeDead(void);
  void handleShooting(void);
  void init_game(void);
  void boot_db(void);
  void boot_zones(void);
  void boot_world(void);
  void write_one_zone(auto &stream, zone_t zone_key);
  zone_t read_one_zone(auto &stream);
  qint32 read_one_room(auto &stream, qint32 &room_number);
  void load_hints(void);
  void save_hints(void);
  void send_hint(void);
  void assign_mobiles(void);
  bool authenticate(QString username, QString password, quint64 level = 0);
  bool authenticate(const QHttpServerRequest &request, quint64 level = 0);
  void crash_hotboot(void);
  void loadnews(void);
  void sendAll(QString message);
  bool isAllowedHost(QHostAddress host);
  Database getDatabase(void) { return database_; }
  Database db(void) { return database_; }
  command_lag *getCommandLag(void) const { return command_lag_list_; }
  void setCommandLag(command_lag *cl) { command_lag_list_ = cl; }

  [[nodiscard]] inline QString currentType(void) { return current_type_; }
  void currentType(QString current_type) { current_type_ = current_type; }

  [[nodiscard]] inline QString currentName(void) { return current_name_; }
  void currentName(QString current_name) { current_name_ = current_name; }

  [[nodiscard]] inline vnum_t currentVNUM(void) { return current_VNUM_; }
  void currentVNUM(vnum_t current_VNUM) { current_VNUM_ = current_VNUM; }

  [[nodiscard]] inline QString currentFilename(void) { return current_filename_; }
  void currentFilename(QString current_filename) { current_filename_ = current_filename; }

  void current(QString current_type, QString current_name, vnum_t current_VNUM, QString current_filename)
  {
    currentType(current_type);
    currentName(current_name);
    currentVNUM(current_VNUM);
    currentFilename(current_filename);
  }

  [[nodiscard]] inline QString current(void)
  {
    return u"%1:%2:%3:%4"_s.arg(currentType()).arg(currentName()).arg(QString::number(currentVNUM())).arg(currentFilename());
  }

  void logverbose(QString str, quint64 god_level = 0, DC::LogChannel type = DC::LogChannel::LOG_MISC, CharacterPtr vict = {});
  [[nodiscard]] quint64 getConnectionLimit(void) { return PER_IP_CONNECTION_LIMIT; }

  template <typename T>
  T number(T from, T to)
  {
    if (from == to)
      return to;

    if (from > to)
    {
      logentry(u"BACKWARDS usage: dc_->number(%1, %2)!"_s.arg(from).arg(to));
      produce_coredump();
      return to;
    }

    if (std::is_unsigned<T>::value)
      return random_.bounded(static_cast<quint64>(from), static_cast<quint64>(to + 1));
    else if (std::is_signed<T>::value)
      return random_.bounded(static_cast<qint64>(from), static_cast<qint64>(to + 1));
  }

  void clean_socials_from_memory(void);
  void remove_all_mobs_from_world(void);
  void remove_all_objs_from_world(void);
  void free_zones_from_memory(void);
  void free_clans_from_memory(void);
  void set_zone_saved_zone(qint32 room);
  void set_zone_modified_zone(qint32 room);
  [[nodiscard]] auto findWorldFileWithVNUM(vnum_t vnum) -> std::expected<world_file_list_item *, search_error>;
  void set_zone_modified(qint32 modnum, world_file_list_item *list);
  void set_zone_modified_world(qint32 room);
  void set_zone_modified_mob(qint32 mob);
  void set_zone_modified_obj(qint32 obj);
  void set_zone_saved(qint32 modnum, world_file_list_item *list);
  void set_zone_saved_world(qint32 room);
  void set_zone_saved_mob(qint32 mob);
  void set_zone_saved_obj(qint32 obj);
  void free_world_from_memory(void);
  void free_mobs_from_memory(void);
  void free_objs_from_memory(void);
  void free_messages_from_memory(void);
  void free_hsh_tree_from_memory(void);
  void free_help_from_memory(void);
  void free_emoting_objects_from_memory(void);
  void free_game_portals_from_memory(void);
  void free_ban_list_from_memory(void);
  void free_buff_pool_from_memory(void);
  void load_vaults(void);
  void testing_load_vaults(void);
  void reload_vaults(void);
  void load_corpses(void);
  ObjectPtr ticket_object_load(QMap<quint32, AuctionTicket>::iterator Item_it, qint32 ticket);
  qint32 write_hotboot_file(void);
  qint32 load_hotboot_descs(void);
  vnum_t getObjectVNUM(ObjectPtr obj, bool *ok = {});
  vnum_t getObjectVNUM(qint32 nr, bool *ok = {});
  vnum_t getObjectVNUM(rnum_t nr, bool *ok = {});
  MobileIndexPtr generate_mob_indices(qint32 *top, MobileIndexPtr index);
  ObjectIndexPtr generate_obj_indices(qint32 *top, ObjectIndexPtr index);
  CharacterPtr read_mobile(qint32 nr, FILE *stream);
  CharacterPtr clone_mobile(qint32 nr);
  auto create_blank_item(qint32 nr) -> std::expected<qint32, create_error>;
  qint32 create_blank_mobile(qint32 nr);
  void game_test_init(void);
  void heartbeat(void);
  void finish_hotboot(void);
  std::expected<ConnectionPtr, load_status_t> load_char_obj(QString name);
  bool has_vault_access(QString owner, VaultPtr vault);
  bool has_vault_access(CharacterPtr ch, VaultPtr vault);
  void update_mprog_throws(void);
  CharacterPtr initiate_oproc(CharacterPtr ch, ObjectPtr obj);
  qint32 oprog_catch_trigger(ObjectPtr obj, qint32 catch_num, QString var, qint32 opt, CharacterPtr actor, ObjectPtr obj2, void *vo, CharacterPtr rndm);
  qint32 oprog_rand_trigger(ObjectPtr item);
  qint32 oprog_arand_trigger(ObjectPtr item);
  void logentry(QString str, quint64 god_level = 0, DC::LogChannel type = DC::LogChannel::LOG_MISC, CharacterPtr vict = {});
  void logf(qint32 level, DC::LogChannel type, QString cformat, ...);
  qint32 send_to_gods(QString message, quint64 god_level, DC::LogChannel type);
  qint32 make_arbitrary_portal(qint32 from_room, qint32 to_room, qint32 duplicate, qint32 timer);
  void load_game_portals(void);
  void process_portals(void);

  ~DC(void)
  {
    /* TODO enable and fix all memory leaks
    remove_all_mobs_from_world();
    remove_all_objs_from_world();
    clean_socials_from_memory();
    free_zones_from_memory();
    free_clans_from_memory();
    free_world_from_memory();
    free_mobs_from_memory();
    free_objs_from_memory();
    free_messages_from_memory();
    free_hsh_tree_from_memory();
    wizlist.clear();
    free_help_from_memory();
    shop_index.clear();
    free_emoting_objects_from_memory();
    free_game_portals_from_memory();
    free_ban_list_from_memory();
    free_buff_pool_from_memory();
    removeDead();
    */
  }

  QRandomGenerator random_;
  QMap<quint64, Shop> shop_index;
  CVoteData DCVote;

  QString last_processed_cmd = {};
  QString last_char_name = {};
  room_t last_char_room = {};
  Commands CMD_;
  Arena arena_;
  QMap<vnum_t, MobileIndexPtr> mob_index_;
  Bans bans_;
  Vaults vaults_;

private:
  timeval last_time_ = {}, delay_time_ = {}, now_time_ = {};
  hints_t hints_;
  Shops shops_;
  QList<QHostAddress> host_list_ = {QHostAddress("127.0.0.1")};
  Database database_;
  command_lag *command_lag_list_ = {};
  QString current_type_;
  QString current_name_;
  vnum_t current_VNUM_ = {};
  QString current_filename_;
  void game_loop_init(void);
  void game_loop(void);
  qint32 init_socket(in_port_t port);
  qint32 exceeded_connection_limit(ConnectionPtr new_conn);
  void nanny(ConnectionPtr conn, QString arg = "");
  void object_activity(quint64 pulse_type);
};

const QStringList DC::obj_types = {
    "act_prog",
    "speech_prog",
    "rand_prog",
    "all_greet_prog",
    "catch_prog",
    "arand_prog",
    "load_prog",
    "command_prog",
    "weapon_prog",
    "armour_prog",
    "can_see_prog"};

class ChannelMessage
{
  QString sender_name_;
  level_t wizinvis_;
  DC::LogChannel type_;
  QString msg_;
  QDateTime timestamp_;

public:
  ChannelMessage(const CharacterPtr sender, const DC::LogChannel type, QString msg) : type_(type), msg_(msg), timestamp_(QDateTime::currentDateTime())
  {
    set_wizinvis(sender);
    set_name(sender);
  }

  QString getMessage(CharacterPtr ch) const;
  QString getMessage(const level_t receiver_level, bool show_timestamps = false, QTimeZone timezone = {}, Qt::DateFormat timestamp_format = {}) const
  {
    QString msg;
    QTextStream output(&msg);
    QString sender_name;

    if (receiver_level < wizinvis_)
    {
      sender_name = "Someone";
    }
    else
    {
      sender_name = sender_name_;
    }

    switch (type_)
    {
    case DC::LogChannel::CHANNEL_GOSSIP:
      if (show_timestamps)
        output << "$5$B" << getTimestamp(timezone, timestamp_format) << ": " << sender_name << " gossips '" << msg_ << "$5$B'$R";
      else
        output << "$5$B" << sender_name << " gossips '" << msg_ << "$5$B'$R";
      break;
    case DC::LogChannel::CHANNEL_TELL:
      if (show_timestamps)
        output << "$2$B" << getTimestamp(timezone, timestamp_format) << ": " << msg_ << "$R";
      else
        output << "$2$B" << msg_ << "$R";
      break;
    default:
      output << "$5$B" << sender_name << " " << type_ << " '" << msg << "$5$B'$R";
      break;
    }

    return msg;
  }

  QString getTimestamp(void) const
  {
    return timestamp_.toString();
  }

  QString getTimestamp(const QTimeZone &timezone, const Qt::DateFormat format = Qt::DateFormat::ISODate) const
  {
    return timestamp_.toTimeZone(timezone).toString(format);
  }

  void set_wizinvis(const CharacterPtr sender);
  void set_name(const CharacterPtr sender);
};
RoomDirectionPtr EXIT(auto ch, qsizetype door)
{
  return ch->dc_->world[ch->in_room].dir_option[door];
}

class news_data
{
public:
  news_data *next;
  time_t time;
  QString news;
  QString addedby;
};

class obj_file_elem
{
public:
  qint16 version = {};
  qint32 item_number = {};
  qint16 timer = {};
  qint16 wear_pos = {};
  qint16 container_depth = {};
  qint32 other[5] = {}; // unused
};

class quest_info
{
public:
  qint32 number;
  QString name;
  QString hint1;
  QString hint2;
  QString hint3;
  QString objshort;
  QString objlong;
  QString objkey;
  qint32 level;
  qint32 objnum;
  qint32 mobnum;
  qint32 timer;
  qint32 reward;
  qint32 cost;
  qint32 brownie;
  bool active;
  quest_infoPtr next;
};

class set_data
{
public:
  const QString SetName = {};
  qint32 amount = {};
  qint32 vnum[19] = {};
  const QString Set_Wear_Message = {};
  const QString Set_Remove_Message = {};
};

class player_shop_item
{
public:
  qint32 item_vnum;       // id of item for sale
  quint32 price;          // asking price of item
  player_shop_item *next; // next item in list
};

class player_shop
{
public:
  QString owner;               // name of player that owns shop (max is 12, but oh well)
  qint32 room_num;             // number of room players shop is in
  QString sell_message;        // special message (if any) when someone buys something
  qint32 money_on_hand;        // cash the player has in the bank right now
  player_shop_item *sale_list; // list of items player has for sale
  player_shop *next;
};

class Legacy
{
public:
  Legacy(QString filename);
  QStringList toList(void);

private:
  QFile file_;
  bool open_status_ = {};
};

class social_messg
{
public:
  QString name;
  qint32 hide = {};
  position_t min_victim_position = {}; /* Position of victim */

  /* No argument was supplied */
  QString char_no_arg;
  QString others_no_arg;

  /* An argument was there, and a victim was found */
  QString char_found; /* if nullptr, read no further, ignore args */
  QString others_found;
  QString vict_found;

  /* An argument was there, but no victim was found */
  QString not_found;

  /* The victim turned out to be the character */
  QString char_auto;
  QString others_auto;
};

class TimeVal
{
public:
  TimeVal(time_t sec = 0, suseconds_t usec = 0);
  TimeVal operator+(TimeVal t);
  TimeVal operator-(TimeVal t);
  TimeVal operator/(qint32 value);
  bool operator<(TimeVal t1);
  bool operator>(TimeVal t1);
  bool operator>=(TimeVal t1);
  void gettime(void);
  uint_fast64_t tv_sec;  /* Seconds.  */
  uint_fast64_t tv_usec; /* Microseconds.  */
};

class SystemTimer
{
public:
  SystemTimer();
  virtual ~SystemTimer();
  void start();
  void stop();
  auto Count(uint_fast64_t c)
  {
    stopCount = c;
    return c;
  }
  auto Count(void) const
  {
    return stopCount;
  }

  auto Total(uint_fast64_t t)
  {
    totalTime = t;
    return t;
  }
  auto Total(void) const
  {
    return totalTime;
  }

  TimeVal getDiff();
  TimeVal getDiffMin();
  TimeVal getDiffMax();
  TimeVal getDiffAvg();

private:
  TimeVal starttv;
  TimeVal diff_cur;
  TimeVal diff_min;
  TimeVal diff_max;
  TimeVal diff_avg;
  uint_fast64_t stopCount;
  uint_fast64_t totalTime;
  friend std::ostream &operator<<(std::ostream &, Timer t);
};

class Version
{
public:
  const static QString build_version_;
  const static QString build_time_;
};

class CommandStack
{
public:
  CommandStack(void);
  CommandStack(quint32 initial);
  CommandStack(quint32 initial, quint32 max);
  ~CommandStack();
  bool setDepth(quint32 value);
  quint32 getDepth(void);
  bool setMax(quint32 value);
  quint32 getMax(void);
  bool isOverflow(void);
  quint32 getOverflowCount(void);

private:
  // Current depth in Command stack
  static quint32 depth;

  // Maximum depth before it's considered an overflow
  static quint32 max_depth;

  // How many times have we overflowed since last reset
  static quint32 overflow_count;

  DCPtr dc_;
};

class pulse_info
{
public:
  pulse_type pulse;
  quint64 duration;
  QString name;
};

class active_object
{
  ObjectPtr obj = {};
  active_object *next = {};
};
class SelfPurge
{
public:
  SelfPurge(void);
  SelfPurge(bool);
  void setOwner(CharacterPtr, QString);
  explicit operator bool(void) const;
  QString getFunction(void) const;
  bool getState(void) const;
  CharacterPtr getOwner(void) const { return owner; }

private:
  bool state = {};
  CharacterPtr owner = {};
  QString function = {};
};

class ki_info_type
{
  quint32 beats_;               /* Waiting time after ki */
  position_t minimum_position_; /* min position for use */
  quint8 min_useski_;           /* minimum ki used */
  qint16 targets_;              /* Legal targets */
  KI_FUN *ki_pointer_;          /* function to call */
  qint32 difficulty_;

public:
  ki_info_type(quint32 beats, position_t minimum_position, quint8 min_useski, qint16 targets, KI_FUN *ki_pointer, qint32 difficulty)
      : beats_(beats), minimum_position_(minimum_position), min_useski_(min_useski), targets_(targets), ki_pointer_(ki_pointer), difficulty_(difficulty) {}
  quint32 beats(void) const { return beats_; }
  position_t minimum_position(void) const { return minimum_position_; }
  quint8 min_useski(void) const { return min_useski_; }
  qint16 targets(void) const { return targets_; }
  KI_FUN *ki_pointer(void) const { return ki_pointer_; }
  qint32 difficulty(void) const { return difficulty_; }
};
class Pulse
{ /* list for keeping tract of 'pulsing' chars */
public:
  CharacterPtr thechar;
  PulsePtr next;
};

class LegacyFile : public QObject
{
  Q_OBJECT
public:
  LegacyFile(QString directory, QString filename, QString error_message);
  ~LegacyFile();
  FILE *openFile(void);
  bool backupFile(void);
  bool isOpen(void)
  {
    if (!file_handle_ || feof(file_handle_) || ferror(file_handle_))
    {
      return false;
    }
    return true;
  }
  FILE *file_handle_;
  QString directory_;
  QString filename_;
  QString filepath_;
  QString error_message_;

private:
};

class LegacyFileWorld : public LegacyFile
{
public:
  LegacyFileWorld(QString filename)
      : LegacyFile("world", filename, "Unable to open world file '%1")
  {
  }
  ~LegacyFileWorld();
};
class Programs
{
  bool object = {};
  QList<ProgramPtr> list_;

public:
  friend qint32 mprog_wordlist_check(QString arg, CharacterPtr mob, CharacterPtr actor, ObjectPtr obj, void *vo, qint32 type, bool reverse);
  [[nodiscard]] bool isEmpty(void) const { return list_.isEmpty(); }
  [[nodiscard]] ProgramPtr value(qsizetype i) { return list_.value(i); }
  [[nodiscard]] qint32 types(void) const
  {
    qint32 t = {};
    for (const auto &program : list_)
      t = t | program->type();
    return t;
  }
  void write(auto &stream, bool mob)
  {
    for (const auto &mprg : list_)
    {
      if (mob)
        stream << ">" << mprg->typeString() << " ";
      else
        stream << "\\" << mprg->typeString() << " ";

      if (mprg->arglist().isEmpty())
        string_to_file(stream, "Saved During Edit");
      else
        string_to_file(stream, mprg->arglist());

      if (mprg->comlist().isEmpty())
        string_to_file(stream, "Saved During Edit");
      else
        string_to_file(stream, mprg->comlist());
    }
  }
  QString list(void);
};

auto &operator<<(auto &out, Programs programs)
{
  if (!programs.isEmpty())
  {
    programs.write(out, false);
    out << "|\n";
  }
  return out;
}
class NewCharacterStats
{
public:
  NewCharacterStats();
  void setMin(void);
  quint8 getMin(quint8 cur, qint8 mod, quint8 min);
  qint32 str[5], tel[5], wis[5], dex[5], con[5];
  qint32 min_str, min_int, min_wis, min_dex, min_con;
  quint64 points;
  attribute_t selection = {};
  quint8 race;
  quint8 clss;
  bool increase(quint64 points = 1);
  bool decrease(quint64 points = 1);
};
class Token
{
public:
  //--
  // Public functions
  //--

  Token(QString rhs);
  ~Token();
  bool isAnsi() { return (type & ANSI); }
  bool isVt100() { return (type & VT100); }
  bool isCode() { return (type & CODE); }
  bool isText() { return (type & TEXT); }
  QString GetBuf() { return (buf); }
  void SetBuf(QString);

private:
  //--
  // Private variables
  //--
  QString buf;   /* Holds the buffer */
  qint32 type{}; /* Holds type of buffer */
  Token *next{}; /* Next token in the list */
}; // end of Token class

class TokenList
{
public:
  //--
  // Public functions
  //--

  TokenList(QString);
  ~TokenList();
  QString Interpret(CharacterPtr from, ObjectPtr obj, ObjectPtr vict_obj, CharacterPtr send_to, qint32 flags);
  QString Interpret(CharacterPtr from, ObjectPtr obj, CharacterPtr vict_obj, CharacterPtr send_to, qint32 flags);

private:
  //--
  // Private functions
  //--
  void AddToken(const Token *);
  void Reset();
  void Next();

  //--
  // Private constants
  //--

  //--
  // Private variables
  //--
  QList<Token> list_;
  QString interp; // Interpreted tokens

}; // end of TokenList class
class str_app_type
{
public:
  qint16 todam;           /* Damage Bonus/Penalty                */
  qint16 carry_w;         /* Maximum weight that can be carrried */
  qint16 cold_resistance; /* Cold resistance */
};

class dex_app_type
{
public:
  qint16 tohit;
  qint16 ac_mod;
  qint16 move_gain;
  qint16 fire_resistance;
};

// Constructor commented out for const.C initialization purposes
class wis_app_type
{
public:
  qint16 mana_regen;
  qint16 ki_regen;
  qint16 bonus; /* how many bonus skills a player can */
  /* practice pr. level                 */
  qint16 energy_resistance;
  qint16 conc_bonus;
  qint16 spell_dam_bonus; // For Cleric/Druid/Paladins naturally
};

// Constructor commented out for const.C initialization purposes
class int_app_type
{
public:
  qint16 mana_regen;
  qint16 ki_regen;
  qint16 easy_bonus;
  qint16 medium_bonus;
  qint16 hard_bonus;
  qint16 learn_bonus;
  qint16 magic_resistance;
  qint16 conc_bonus;
  qint16 spell_dam_bonus; // For Mage/Anti/Bard
};

// Constructor commented out for const.C initialization purposes
class con_app_type
{
public:
  qint16 hp_regen;
  qint16 move_regen;
  qint16 hp_gain;
  qint16 poison_resistance;
};

class mprog_variable_data
{
public:
  QString invoker_;
  QString object_;
  QString rndm_;
  QString voi_;
  qint32 nested; // amount of nested ifs, at time of pause
  QString program_;
};

class act_return
{
public:
  QString str;
  qint32 retval;
};

class send_tokens_return
{
public:
  QString str;
  qint32 retval;
};

class skill_quest
{
public:
  skill_quest *next;
  QString message_;
  qint32 num;
  qint32 clas;
  qint32 level;
};

class skill_stuff
{
  QString name_;
  qint32 difficulty_;

public:
  skill_stuff(const QString name, qint32 difficulty) : name_(name), difficulty_(difficulty) {}
  QString name(void) const { return name_; }
  qint32 difficulty(void) const { return difficulty_; }
};

class txt_block
{
public:
  QString text = {};
  txt_block *next = {};
  qint32 aliased = {};
};

typedef class txt_q
{
public:
  txt_block *head;
  txt_block *tail;
} TXT_Q;

class msg_type
{
public:
  QString attacker_msg; /* message to attacker */
  QString victim_msg;   /* message to victim   */
  QString room_msg;     /* message to room     */
};

class message_type
{
public:
  msg_type die_msg;       /* messages when death            */
  msg_type miss_msg;      /* messages when miss             */
  msg_type hit_msg;       /* messages when hit              */
  msg_type sanctuary_msg; /* messages when hit on sanctuary */
  msg_type god_msg;       /* messages when hit on god       */
  message_type *next;     /* to next messages of ths kind.*/
};

class message_list
{
public:
  qint32 a_type;            /* Attack type				*/
  qint32 number_of_attacks; /* # messages to chose from		*/
  message_type *msg;        /* List of messages			*/
  message_type *msg2;       /* List of messages with toggle damage ON */
};

class reroll_t
{
public:
  ObjectPtr choice1_obj = {};
  ObjectPtr choice2_obj = {};
  quint64 orig_rnum = {};
  vnum_t orig_vnum = {};
  ObjectPtr orig_obj = {};

  enum reroll_states_t
  {
    BEGIN,
    PICKED_OBJ_TO_REROLL,
    REROLLED,
    CHOSEN
  } state = {};
};

class game_portal
{
public:
  room_t to_room;          /* Room to make the portal to */
  QSet<room_t> from_rooms; /* Rooms to make the portal from */
  qint32 obj_num;          /* Object to duplicate for portal */
  qint32 max_timer;        /* What does the timer reset to? -- game days */
  qint32 cur_timer;        /* What is the timer at now? -- game days */
};

void load_emoting_objects(void);
qint32 create_entry(QString name);
void delete_item_from_index(qint32 nr);
void delete_mob_from_index(qint32 nr);
qint32 real_object(qint32 virt);
qint32 real_mobile(qint32 virt);
void load_hints();
void find_unordered_mobiles(void);
void write_wizlist(QString filename);
void string_to_file(QTextStream &stream, QString str);
qint32 write_to_descriptor(qint32 desc, QByteArray txt);
QString scramble_text(QString txt);
void send_info(QString messg);
void update_bard_singing(void);
void affect_update(qint32 duration_type); /* In spells.c */
const QString calc_color(qint32 hit, qint32 max_hit);
const QString calc_color_align(qint32 align);
QString str_str(QString first, QString second);
void setup_dir(auto &stream, qint32 room, qint32 dir);
qint32 real_roomb(qint32 virt);
void save_ban_list(void);
void save_nonew_new_list(void);
void add_command_lag(CharacterPtr ch, cmd_t cmd, qint32 lag);
typedef qint32 DO_FUN(CharacterPtr ch, QString argument, cmd_t cmd);
Direction reverse_direction(Direction dir);
qint32 attempt_move(CharacterPtr ch, cmd_t cmd, bool is_retreat = 0);
qint32 ambush(CharacterPtr ch);
bool resist_spell(qint32 perc);
bool resist_spell(CharacterPtr ch, qint32 skill);
qint32 spellcraft(CharacterPtr ch, qint32 spell);
qint32 spell_iridescent_aura(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_resist_fire(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_resist_cold(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_acid_blast(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_acid_breath(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_animate_dead(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr corpse, qint32 skill);
qint32 spell_armor(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_aegis(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_portal(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_resist_magic(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_bee_sting(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_bee_swarm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_bless(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_blindness(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_burning_hands(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_burning_hands(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_oaken_fortitude(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_clarity(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_divine_intervention(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_call_lightning(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cause_critical(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cause_light(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cause_serious(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_charm_person(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_chill_touch(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_colour_spray(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_conjure_elemental(quint8 level, CharacterPtr ch, QString arg, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cont_light(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_create_food(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_create_water(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_remove_blind(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cure_critic(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cure_light(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cure_serious(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_curse(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_shadowslip(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_camouflague(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_farsight(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_resist_energy(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_staunchblood(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_insomnia(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_freefloat(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_earthquake(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_energy_drain(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_drown(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_howl(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_souldrain(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_sparks(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_fireball(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_heal_spray(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_life_leech(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_lightning_bolt(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_magic_missile(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_meteor_swarm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_shocking_grasp(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_vampiric_touch(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_firestorm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_dispel_evil(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_dispel_good(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_harm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_power_harm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_divine_fury(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_teleport(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_paralyze(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_remove_paralysis(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_detect_evil(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_detect_good(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_TRUE_sight(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_detect_invisibility(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_infravision(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_detect_magic(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_haste(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_detect_poison(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_enchant_weapon(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_eyes_of_the_owl(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_feline_agility(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_heal(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_power_heal(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_full_heal(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_invisibility(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_locate_object(quint8 level, CharacterPtr ch, const QString arg, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_oaken_fortitude(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_poison(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_protection_from_evil(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_protection_from_good(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_remove_curse(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill, quint64 mana_cost = {});
qint32 spell_remove_poison(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_fireshield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_sanctuary(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_sleep(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_strength(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_ventriloquate(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_word_of_recall(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_wizard_eye(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_eagle_eye(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_summon(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_sense_life(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_identify(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_frost_breath(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_fire_breath(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_gas_breath(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_lightning_breath(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_fear(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_refresh(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_fly(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_know_alignment(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_dispel_magic(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill, qint32 spell = 0);
qint32 spell_flamestrike(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_stone_skin(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_shield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_weaken(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_mass_invis(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_hellstream(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_group_sanc(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_ghost_walk(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_mend_golem(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_ghost_walk(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_mend_golem(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_iridescent_aura(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_camouflague(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_farsight(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_resist_energy(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_staunchblood(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_freefloat(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_insomnia(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_shadowslip(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_call_lightning(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_chill_touch(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_shocking_grasp(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_colour_spray(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_earthquake(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_life_leech(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_firestorm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_energy_drain(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_drown(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_howl(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_souldrain(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_sparks(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_vampiric_touch(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_meteor_swarm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_fireball(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_harm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_power_harm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_divine_fury(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_lightning_bolt(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_magic_missile(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_armor(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_aegis(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_teleport(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_bless(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_paralyze(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_blindness(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_control_weather(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_create_food(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_create_water(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_remove_paralysis(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_remove_blind(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cure_critic(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cure_light(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_curse(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_detect_evil(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_TRUE_sight(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_detect_good(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_detect_invisibility(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_detect_magic(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_haste(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_detect_poison(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_dispel_evil(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_dispel_good(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_enchant_weapon(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_heal(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_power_heal(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_full_heal(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_invisibility(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_locate_object(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_poison(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_protection_from_evil(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_protection_from_good(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_remove_curse(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill, quint64 mana_cost = {});
qint32 cast_remove_poison(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_fireshield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_sanctuary(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_sleep(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_strength(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_ventriloquate(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_word_of_recall(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_wizard_eye(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_eagle_eye(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_summon(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_charm_person(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_sense_life(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_identify(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_frost_breath(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_acid_breath(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_fire_breath(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_gas_breath(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_lightning_breath(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_fear(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_refresh(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_fly(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cont_light(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_know_alignment(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_dispel_magic(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_conjure_elemental(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cure_serious(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cause_light(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cause_critical(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cause_serious(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_flamestrike(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_stone_skin(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_weaken(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_mass_invis(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_acid_blast(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_hellstream(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_portal(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_infravision(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_animate_dead(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_mana(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_solar_gate(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_heroes_feast(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_heal_spray(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_group_sanc(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_group_recall(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_group_fly(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_enchant_armor(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_resist_fire(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_resist_magic(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_resist_cold(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_bee_sting(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_bee_swarm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_creeping_death(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_barkskin(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_herb_lore(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_call_follower(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_entangle(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_eyes_of_the_owl(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_feline_agility(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_forest_meld(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_companion(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_create_golem(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 spell_dispel_minor(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_dispel_minor(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 spell_release_golem(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 spell_beacon(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 spell_reflect(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 spell_stone_shield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_stone_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_greater_stone_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_summon_familiar(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_summon_familiar(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_lighted_path(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_lighted_path(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_resist_acid(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_resist_acid(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_sun_ray(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_sun_ray(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_rapid_mend(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_rapid_mend(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_iron_roots(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_iron_roots(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_acid_shield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_acid_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_water_breathing(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_water_breathing(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_globe_of_darkness(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_globe_of_darkness(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_barkskin(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_entangle(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_eyes_of_the_eagle(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_eyes_of_the_eagle(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_icestorm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_icestorm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_lightning_shield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_lightning_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_blue_bird(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_blue_bird(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_debility(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_debility(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_attrition(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_attrition(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_vampiric_aura(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_vampiric_aura(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_holy_aura(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_holy_aura(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
SPELL_POINTER get_wild_magic_defensive(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
SPELL_POINTER get_wild_magic_offensive(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_wild_magic(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_wild_magic(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_stability(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_stability(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_solidity(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_solidity(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_frostshield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_frostshield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_release_elemental(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_release_elemental(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_dismiss_familiar(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_dismiss_familiar(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_dismiss_corpse(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_dismiss_corpse(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_visage_of_hate(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_visage_of_hate(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_blessed_halo(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_blessed_halo(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_mana(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_wrath_of_god(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_atonement(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_silence(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_immunity(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_boneshield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_channel(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_spirit_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_villainy(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_villainy(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_heroism(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_heroism(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_consecrate(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_consecrate(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_desecrate(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_desecrate(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_elemental_wall(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_elemental_wall(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_ethereal_focus(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_ethereal_focus(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
ReturnValue do_add_quest(CharacterPtr, QString);
void list_quests(CharacterPtr);
void show_quest_info(CharacterPtr, qint32);
bool check_available_quest(CharacterPtr, quest_infoPtr);
bool check_quest_current(CharacterPtr, qint32);
bool check_quest_pass(CharacterPtr, qint32);
bool check_quest_complete(CharacterPtr, qint32);
qint32 get_quest_price(quest_infoPtr);
void show_quest_header(CharacterPtr);
void show_quest_amount(CharacterPtr);
void show_quest_closer(CharacterPtr);
qint32 show_one_quest(CharacterPtr, quest_infoPtr, qint32);
qint32 show_one_complete_quest(CharacterPtr, quest_infoPtr, qint32);
qint32 show_one_available_quest(CharacterPtr, quest_infoPtr, qint32);
void show_available_quests(CharacterPtr);
void show_pass_quests(CharacterPtr);
void show_current_quests(CharacterPtr);
void show_complete_quests(CharacterPtr);
qint32 start_quest(CharacterPtr, quest_infoPtr);
qint32 pass_quest(CharacterPtr, quest_infoPtr);
qint32 complete_quest(CharacterPtr, quest_infoPtr);
qint32 stop_current_quest(CharacterPtr, quest_infoPtr);
qint32 stop_current_quest(CharacterPtr, qint32);
qint32 stop_all_quests(CharacterPtr);
qint32 quest_handler(CharacterPtr, CharacterPtr, qint32, QString);
qint32 quest_master(CharacterPtr, ObjectPtr, qint32, QString, CharacterPtr);
ReturnValue do_quest(CharacterPtr, QString, qint32);
ReturnValue do_mscore(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_huntstart(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_huntclear(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_metastat(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_findfix(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_reload(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_abandon(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_accept(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_acfinder(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_action(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_addnews(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_addRoom(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_advance(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_areastats(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_awaymsgs(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_archive(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_autojoin(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_ambush(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_appraise(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_assemble(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_release(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_jab(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_areas(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_ask(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_at(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_auction(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_ban(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_bandwidth(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_bash(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_batter(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_battlecry(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_battlesense(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_beacon(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_beep(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_behead(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_berserk(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_bladeshield(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_bloodfury(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_boot(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_boss(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_brace(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_brew(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_bug(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_bullrush(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_cast(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_channel(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_check(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_chpwd(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_cinfo(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_circle(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_clans(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_clear(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_clearaff(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_climb(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_close(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_cmotd(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_ctax(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_cwithdraw(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_cbalance(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_colors(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_config(CharacterPtr ch, QStringList arguments, cmd_t cmd);
ReturnValue do_consent(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_consider(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_count(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_cpromote(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_crazedassault(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_credits(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_cripple(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_ctell(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_deathstroke(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_debug(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_deceit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_defenders_stance(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_disarm(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_disband(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_disconnect(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_dmg_eq(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_donate(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_dream(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_drop(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_eagle_claw(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_echo(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_emote(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_setvote(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_vote(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_enter(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_equipment(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_eyegouge(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_examine(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_exits(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_export(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_ferocity(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_fighting(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_fill(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_find(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_findpath(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_findPath(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_fire(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_flee(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
/* DO_FUN  do_fly; */
ReturnValue do_focused_repelance(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_follow(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_forage(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_found(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_free_animal(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_freeze(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_fsave(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_get(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_global(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_gossip(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_golem_score(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_guild(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_install(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_reload_help(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_hindex(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_hedit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_grab(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_group(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_grouptell(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_gtrans(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_guard(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_harmtouch(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_help(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mortal_help(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_new_help(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_hide(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_highfive(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_hitall(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_holylite(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_home(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_idea(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_identify(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_ignore(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_imotd(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_imbue(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_incognito(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_index(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_info(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_initiate(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_innate(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_instazone(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_insult(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_inventory(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_joinarena(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_ki(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_kill(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_knockback(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
// ReturnValue do_land (CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_layhands(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_leaderboard(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_leadership(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_leave(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_levels(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);

ReturnValue do_linkdead(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_listAllPaths(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_listPathsByZone(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_listproc(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_load(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_medit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mortal_set(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
// ReturnValue do_motdload (CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_msave(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_procedit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpbestow(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpstat(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_opedit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_eqmax(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_opstat(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_lock(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_log(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_look(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_make_camp(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_matrixinfo(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_maxes(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mlocate(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_move(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_motd(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpretval(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpasound(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpat(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpdamage(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpecho(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpechoaround(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpechoaroundnotbad(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpechoat(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpforce(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpgoto(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpjunk(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpkill(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mphit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpsetmath(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpaddlag(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpmload(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpoload(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mppause(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mppeace(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mppurge(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpteachskill(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpsetalign(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpthrow(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpothrow(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mptransfer(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpxpreward(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mpteleport(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_murder(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_name(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_natural_selection(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_newbie(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_newPath(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_news(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_noemote(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_nohassle(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_noname(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue generic_command(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_oclone(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_mclone(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_offer(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_olocate(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_oneway(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_onslaught(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_order(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_orchestrate(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_osave(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_pardon(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_pathpath(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_peace(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_perseverance(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_pick(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_plats(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_pocket(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_poisonmaking(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_pour(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_poof(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_possess(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_practice(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_pray(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_profession(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_primalfury(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_promote(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_prompt(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_lastprompt(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_processes(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_psay(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
// ReturnValue do_pshopedit (CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_pview(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_punish(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_purge(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_purloin(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_put(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_qedit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_quaff(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_quest(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_qui(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_quivering_palm(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_quit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_rage(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_random(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_range(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_rdelete(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_read(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_recite(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_redirect(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_redit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_remove(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_rent(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_reply(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_repop(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_report(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_rest(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_restore(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_retreat(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_return(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_revoke(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_rsave(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_rstat(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_sacrifice(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_say(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::SAY);
ReturnValue do_scan(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_score(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_scribe(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_sector(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_sedit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_send(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_set(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_shout(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_showhunt(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_skills(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_social(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_songs(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_stromboli(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_headbutt(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_show(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_showbits(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_silence(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_stupid(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_sing(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_sip(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_sit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_slay(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_sleep(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_slip(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_smite(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_sneak(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_spells(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_sqedit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_stalk(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_stand(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_stat(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_steal(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_stealth(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_story(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_string(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_stun(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_suicide(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_switch(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_tactics(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_tame(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_taste(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_teleport(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_tellhistory(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_testhand(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_testhit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_testport(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_testuser(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_thunder(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_tick(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_time(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_title(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_transfer(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_triage(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_trip(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_trivia(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_typo(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_unarchive(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_unlock(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_use(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_varstat(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_vault(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_vend(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_version(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_visible(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_vitalstrike(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_wear(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_weather(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_where(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_whisper(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_whoarena(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_whoclan(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_whogroup(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_whosolo(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_wield(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_fakelog(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_wiz(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_wizinvis(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_wizlist(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_wizlock(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_world(CharacterPtr ch, QString args, cmd_t cmd);
ReturnValue do_write_skillquest(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_write(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_zedit(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_zoneexits(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_editor(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
ReturnValue do_pursue(CharacterPtr ch, QString argument, cmd_t cmd = cmd_t::DEFAULT);
/* handling the affected-structures */
void affect_total(CharacterPtr ch);
void affect_modify(CharacterPtr ch, qint32 loc, qint32 mod, qint32 bitv, bool add, qint32 flag = 0);
void affect_from_char(CharacterPtr ch, qint32 skill, qint32 flags = 0);
void affect_remove(CharacterPtr ch, affected_typePtr af, qint32 flags);
void affect_join(CharacterPtr ch, affected_typePtr af, bool avg_dur, bool avg_mod);
void char_to_store(class char_file_u4 *st, Time &tmpage);
void renum_world(void);
void renum_zone_table(void);
void clear_hunt(varg_t arg1, void *arg2, void *arg3);
void clear_hunt(varg_t arg1, CharacterPtr arg2, void *arg3);
auto get_bestow_command(QString command_name) -> std::expected<bestowable_god_commands_type, search_error>;
bool operator==(class ResetCommand a, class ResetCommand b);
void extractFamiliar(CharacterPtr ch);
bool skill_success(CharacterPtr ch, CharacterPtr victim, qint32 skillnum, qint32 mod = 0);
qint32 GET_WAIT(CharacterPtr ch);
void WAIT_STATE(CharacterPtr ch, qint32 cycle);
bool is_hiding(CharacterPtr ch, CharacterPtr vict);
void clan_death(QString b, CharacterPtr ch);
qint32 move_char(CharacterPtr ch, qint32 dest, bool stop_all_fighting = true);
bool circle_follow(CharacterPtr ch, CharacterPtr victim);
bool ARE_GROUPED(CharacterPtr sub, CharacterPtr obj);
bool ARE_CLANNED(CharacterPtr sub, CharacterPtr obj);
void gain_condition(CharacterPtr ch, qint32 condition, qint32 value);
void set_fighting(CharacterPtr ch, CharacterPtr vict);
void stop_fighting(CharacterPtr ch, qint32 clearlag = 1);
ReturnValue do_simple_move(CharacterPtr ch, cmd_t cmd, qint32 following);
qint32 mana_limit(CharacterPtr ch);
qint32 ki_limit(CharacterPtr ch);
qint32 hit_limit(CharacterPtr ch);
QString get_skill_name(qint32 skillnum);
void gain_exp_regardless(CharacterPtr ch, qint32 gain);
void advance_level(CharacterPtr ch, bool is_conversion);
qint32 close_socket(ConnectionPtr conn);
void page_string(ConnectionPtr conn, const QString str, qint32 keep_internal);
void gain_exp(CharacterPtr ch, qint64 gain);
void redo_hitpoints(CharacterPtr ch); /* Rua's put in  */
void redo_mana(CharacterPtr ch);      /* Rua's put in  */
void redo_ki(CharacterPtr ch);        /* And Urizen*/
void free_obj(ObjectPtr obj);
qint32 char_from_room(CharacterPtr ch, bool stop_fighting);
qint32 char_from_room(CharacterPtr ch);
void do_start(CharacterPtr ch);
void update_pos(CharacterPtr victim);
void clear_object(ObjectPtr obj);
void death_cry(CharacterPtr ch);
void unique_scan(CharacterPtr victim);
bool obj_to_store(ObjectPtr obj, CharacterPtr ch, FILE *fpsave, qint32 wear_pos);
void check_idling(CharacterPtr ch);
void stop_follower(CharacterPtr ch, follower_reasons_t reason = follower_reasons_t::DEFAULT);
void add_follower(CharacterPtr ch, CharacterPtr leader, follower_reasons_t reason = follower_reasons_t::DEFAULT);
bool CAN_SEE(CharacterPtr sub, CharacterPtr obj, bool noprog = false);
qint32 SWAP_CH_VICT(qint32 value);
bool SOMEONE_DIED(qint32 value);
bool CAN_SEE_OBJ(CharacterPtr sub, ObjectPtr obj, bool bf = false);
bool check_blind(CharacterPtr ch);
void raw_kill(CharacterPtr ch, CharacterPtr victim);
void check_killer(CharacterPtr ch, CharacterPtr victim);
qint32 map_eq_level(CharacterPtr mob);
void disarm(CharacterPtr ch, CharacterPtr victim);
qint32 shop_keeper(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr invoker);
void ansi_color(QString txt, CharacterPtr ch);
void send_to_char(QString messg, CharacterPtr ch);
void send_to_char_nosp(QString messg, CharacterPtr ch);
void send_to_char_nosp(QString messg, CharacterPtr ch);
void util_archive(QString, CharacterPtr);
void util_unarchive(QString, CharacterPtr);
bool is_busy(CharacterPtr ch);
bool is_ignoring(CharacterPtr ch, CharacterPtr i);
void colorCharSend(QString s, CharacterPtr ch);
void send_to_char_regardless(QString messg, CharacterPtr ch);
void send_to_char_regardless(QString messg, CharacterPtr ch);
qint32 csendf(CharacterPtr ch, QString arg, ...);
void record_track_data(CharacterPtr ch, cmd_t cmd);
void send_to_room(QString messg, qint32 room, bool awakeonly = false, CharacterPtr nta = {});
qint32 use_mana(CharacterPtr ch, qint32 sn);
void mob_suprised_sayings(CharacterPtr ch, CharacterPtr aggressor);
void parse_bitstrings_into_int(QStringList bits, QString strings, CharacterPtr ch, quint32 value[]);
void parse_bitstrings_into_int(QStringList bits, QString strings, CharacterPtr ch, quint32 &value);
void parse_bitstrings_into_int(QStringList bits, QString strings, CharacterPtr ch, quint32 &value);
void parse_bitstrings_into_int(QStringList bits, QString strings, CharacterPtr ch, quint16 &value);
void parse_bitstrings_into_int(QStringList bits, QString strings, CharacterPtr ch, quint32 value[]);
void parse_bitstrings_into_int(QStringList bits, QString strings, CharacterPtr ch, quint32 &value);
void parse_bitstrings_into_int(QStringList bits, QString strings, CharacterPtr ch, quint16 &value);
qint32 contains_no_trade_item(ObjectPtr obj);
qint32 contents_cause_unique_problem(ObjectPtr obj, CharacterPtr vict);
bool check_make_camp(qint32);
qint32 get_leadership_bonus(CharacterPtr);
qint32 mprog_wordlist_check(QString arg, CharacterPtr mob, CharacterPtr actor, ObjectPtr object, void *vo, qint32 type, bool reverse = false);
void mprog_percent_check(CharacterPtr mob, CharacterPtr actor, ObjectPtr object, void *vo, qint32 type);
qint32 mprog_act_trigger(QString buf, CharacterPtr mob, CharacterPtr ch, ObjectPtr obj, void *vo);
qint32 mprog_bribe_trigger(CharacterPtr mob, CharacterPtr ch, qint32 amount);
qint32 mprog_entry_trigger(CharacterPtr mob);
qint32 mprog_give_trigger(CharacterPtr mob, CharacterPtr ch, ObjectPtr obj);
qint32 mprog_fight_trigger(CharacterPtr mob, CharacterPtr ch);
qint32 mprog_hitprcnt_trigger(CharacterPtr mob, CharacterPtr ch);
qint32 mprog_death_trigger(CharacterPtr mob, CharacterPtr killer);
qint32 mprog_random_trigger(CharacterPtr mob);
qint32 mprog_arandom_trigger(CharacterPtr mob);
qint32 mprog_catch_trigger(CharacterPtr mob, qint32 catch_num, QString var, qint32 opt, CharacterPtr actor, ObjectPtr obj, void *vo, CharacterPtr rndm);
qint32 mprog_attack_trigger(CharacterPtr mob, CharacterPtr ch);
qint32 mprog_load_trigger(CharacterPtr mob);
qint32 mprog_damage_trigger(CharacterPtr mob, CharacterPtr ch, qint32 amount);
bool is_in_game(CharacterPtr ch);
qint32 get_stat(CharacterPtr ch, attribute_t stat);
qint32 handle_poisoned_weapon_attack(CharacterPtr ch, CharacterPtr vict, qint32 percent);
void show_obj_class_size_mini(ObjectPtr obj, CharacterPtr ch);
const QString item_condition(ObjectPtr obj);
bool identify(CharacterPtr ch, ObjectPtr obj);
QByteArray handle_ansi(QByteArray, CharacterPtr ch);
QString handle_ansi(QString, CharacterPtr ch);
QString handle_ansi(QString s, CharacterPtr ch);
QString handle_ansi_(QString s, CharacterPtr ch);
void show_string(ConnectionPtr conn, QString input);
qint32 get_saves(CharacterPtr ch, qint32 savetype);
QDebug operator<<(QDebug debug, const songInfo &song);
void stop_grouped_bards(CharacterPtr ch, qint32 action);
void update_character_singing(CharacterPtr ch);
void get_instrument_bonus(CharacterPtr ch, qint32 &comb, qint32 &non_comb);
QString skip_spaces(QString s);

SING_FUN song_whistle_sharp;
SING_FUN song_disrupt;
SING_FUN song_healing_melody;
SING_FUN execute_song_healing_melody;
SING_FUN song_revealing_stacato;
SING_FUN execute_song_revealing_stacato;
SING_FUN song_note_of_knowledge;
SING_FUN execute_song_note_of_knowledge;
SING_FUN song_terrible_clef;
SING_FUN execute_song_terrible_clef;
SING_FUN song_listsongs;
SING_FUN song_soothing_remembrance;
SING_FUN execute_song_soothing_remembrance;
SING_FUN song_traveling_march;
SING_FUN execute_song_traveling_march;
SING_FUN song_stop;
SING_FUN song_summon_song;
SING_FUN execute_song_summon_song;
SING_FUN song_astral_chanty;
SING_FUN execute_song_astral_chanty;
SING_FUN pulse_song_astral_chanty;
SING_FUN song_forgetful_rhythm;
SING_FUN execute_song_forgetful_rhythm;
SING_FUN song_shattering_resonance;
SING_FUN execute_song_shattering_resonance;
SING_FUN song_insane_chant;
SING_FUN execute_song_insane_chant;
SING_FUN song_flight_of_bee;
SING_FUN execute_song_flight_of_bee;
SING_FUN pulse_flight_of_bee;
SING_FUN intrp_flight_of_bee;
SING_FUN song_searching_song;
SING_FUN execute_song_searching_song;
SING_FUN song_jig_of_alacrity;
SING_FUN execute_song_jig_of_alacrity;
SING_FUN pulse_jig_of_alacrity;
SING_FUN intrp_jig_of_alacrity;
SING_FUN song_glitter_dust;
SING_FUN execute_song_glitter_dust;
SING_FUN song_bountiful_sonnet;
SING_FUN execute_song_bountiful_sonnet;
SING_FUN song_synchronous_chord;
SING_FUN execute_song_synchronous_chord;
SING_FUN song_sticky_lullaby;
SING_FUN execute_song_sticky_lullaby;
SING_FUN song_vigilant_siren;
SING_FUN execute_song_vigilant_siren;
SING_FUN pulse_vigilant_siren;
SING_FUN intrp_vigilant_siren;
SING_FUN song_unresistable_ditty;
SING_FUN execute_song_unresistable_ditty;
SING_FUN song_fanatical_fanfare;
SING_FUN execute_song_fanatical_fanfare;
SING_FUN pulse_song_fanatical_fanfare;
SING_FUN intrp_song_fanatical_fanfare;
SING_FUN song_dischordant_dirge;
SING_FUN execute_song_dischordant_dirge;
SING_FUN song_crushing_crescendo;
SING_FUN execute_song_crushing_crescendo;
SING_FUN song_hypnotic_harmony;
SING_FUN execute_song_hypnotic_harmony;
SING_FUN song_mking_charge;
SING_FUN execute_song_mking_charge;
SING_FUN pulse_mking_charge;
SING_FUN intrp_mking_charge;
SING_FUN song_submariners_anthem;
SING_FUN execute_song_submariners_anthem;
SING_FUN pulse_submariners_chorus;
SING_FUN intrp_submariners_chorus;
KI_FUN ki_blast;
KI_FUN ki_punch;
KI_FUN ki_sense;
KI_FUN ki_storm;
KI_FUN ki_speed;
KI_FUN ki_purify;
KI_FUN ki_disrupt;
KI_FUN ki_stance;
KI_FUN ki_agility;
KI_FUN ki_meditation;
KI_FUN ki_transfer;

qint32 eq_max_damage(ObjectPtr obj);
qint32 damage_eq_once(ObjectPtr obj);
qint32 eq_current_damage(ObjectPtr obj);
void eq_remove_damage(ObjectPtr obj);
void add_obj_affect(ObjectPtr obj, qint32 loc, qint32 mod);
void remove_obj_affect_by_index(ObjectPtr obj, qint32 index);
void remove_obj_affect_by_type(ObjectPtr obj, qint32 loc);
bool fullSave(ObjectPtr obj);
void heightweight(CharacterPtr ch, bool add);
void wear(CharacterPtr ch, ObjectPtr obj_object, qint32 keyword);
qint32 obj_from(ObjectPtr obj);
void mprog_driver(QString com_list, CharacterPtr mob, CharacterPtr actor, ObjectPtr obj, void *vo, class mprog_throw_type *thrw, CharacterPtr rndm);
bool charExists(CharacterPtr ch);
// QDebug operator<<(QDebug dbg, const SelfPurge &sp);
void translate_value(QString leftptr, QString rightptr, qint16 **vali, quint32 **valui, QString **valstr, qint64 **vali64, quint64 **valui64, qint8 **valb, CharacterPtr mob, CharacterPtr actor, ObjectPtr obj, void *vo, CharacterPtr rndm, QString &valqstr);
void save_golem_data(CharacterPtr ch);
void save_charmie_data(CharacterPtr ch);
typedef qint32 KI_FUN(quint8 level, CharacterPtr ch, const QString arg, CharacterPtr vict);
qint32 ki_check(CharacterPtr ch);
void reduce_ki(CharacterPtr ch, qint32 type);
ObjectPtr create_money(qint32 amount);
qint32 get_max_stat(CharacterPtr ch, attribute_t stat);
bool isTimer(CharacterPtr ch, qint32 spell);
void addTimer(CharacterPtr ch, qint32 spell, qint32 ticks);
qint32 move_obj(ObjectPtr obj, qint32 dest);
qint32 move_obj(ObjectPtr obj, CharacterPtr ch);
qint32 move_obj(ObjectPtr obj, ObjectPtr dest_obj);
qint32 obj_to_char(ObjectPtr object, CharacterPtr ch);
qint32 obj_from_char(ObjectPtr object);
qint32 obj_to_room(ObjectPtr object, qint32 room);
qint32 obj_from_room(ObjectPtr object);
qint32 obj_to_obj(ObjectPtr obj, ObjectPtr obj_to);
qint32 obj_from_obj(ObjectPtr obj);
ObjectPtr get_obj_in_list(QString name, ObjectPtr list);
ObjectPtr get_obj_in_list_num(qint32 num, ObjectPtr list);
affected_typePtr affected_by_random(CharacterPtr ch);
ObjectPtr get_obj(QString name);
ObjectPtr get_obj(qint32 vnum);
ObjectPtr get_obj_num(qint32 nr);
void object_list_new_new_owner(ObjectPtr list, CharacterPtr ch);
void extract_obj(ObjectPtr obj);
/* ******* characters ********* */
CharacterPtr get_char_room(QString name, room_t room, bool careful = false);
CharacterPtr get_char_num(qint32 nr);
CharacterPtr get_char(QString name);
CharacterPtr get_mob(QString name);
CharacterPtr get_pc(QString name);
qint32 char_from_room(CharacterPtr ch, bool stop_all_fighting);
qint32 char_from_room(CharacterPtr ch);
qint32 char_to_room(CharacterPtr ch, room_t room, bool stop_all_fighting = true);
/* find if character can see */
CharacterPtr get_active_pc(QString name);
CharacterPtr get_all_pc(QString name);
CharacterPtr get_char_vis(CharacterPtr ch, QString name);
CharacterPtr get_pc_vis(CharacterPtr ch, QString name);
CharacterPtr get_pc_vis_exact(CharacterPtr ch, QString name);
CharacterPtr get_mob_vis(CharacterPtr ch, QString name);
CharacterPtr get_random_mob_vnum(qint32 vnum);
CharacterPtr get_mob_room_vis(CharacterPtr ch, const QString name);
CharacterPtr get_mob_vnum(qint32 vnum);
ObjectPtr get_obj_vnum(qint32 vnum);
ObjectPtr get_obj_vnum(QString vnum);
ObjectPtr get_objindex_vnum(vnum_t vnum);
ObjectPtr get_objindex_vnum(QString vnum);
vnum_t get_vnum(QString vnum_str);
ObjectPtr get_obj_in_list_vis(CharacterPtr ch, QString name, ObjectPtr list, bool bf = false);
ObjectPtr get_obj_in_list_vis(CharacterPtr ch, qint32 item_num, ObjectPtr list, bool bf = false);
ObjectPtr get_obj_vis(CharacterPtr ch, QString name, bool loc = false);
void extract_char(CharacterPtr ch, bool pull);
void redo_shop_profit(void);

void getAreaData(quint32 zone, qint32 mob, quint32 xps, quint32 gold);
std::ostream &operator<<(std::ostream &out, Timer t);
std::ostream &operator<<(std::ostream &out, TimeVal tv);
std::ostream &operator<<(std::ostream &out, QString str);
qint32 vault_log_to_string(const QString name, QString buf);
void logvault(QString message, QString name);

skill_results_t find_skills_by_name(QString name);
CharacterPtr get_pc_vis(CharacterPtr ch, QString name);
qint32 generic_find(const QString arg, qint32 bitvector, CharacterPtr ch, CharacterPtr tar_ch, ObjectPtr tar_obj, bool verbose = false);
bool is_wearing(CharacterPtr ch, ObjectPtr item);
bool objExists(ObjectPtr obj);
bool charge_moves(CharacterPtr ch, qint32 skill, double modifier = 1);
void die_follower(CharacterPtr ch);
void stop_guarding_me(CharacterPtr victim);
void stop_guarding(CharacterPtr guard);
void remove_memory(CharacterPtr ch, QChar type);
void remove_memory(CharacterPtr ch, QChar type, CharacterPtr vict);
void write_object_csv(ObjectPtr obj, std::ofstream &fout);
ObjectPtr clone_object(qint32 nr);
void randomize_object(ObjectPtr obj);
void copySaveData(ObjectPtr new_obj, ObjectPtr obj);
bool verify_item(ObjectPtr obj);
bool fullItemMatch(ObjectPtr obj, ObjectPtr obj2);
bool has_random(ObjectPtr obj);

bool can_modify_this_room(CharacterPtr ch, qint32 room);
bool can_modify_room(CharacterPtr ch, qint32 room);
bool can_modify_mobile(CharacterPtr ch, qint32 room);
bool can_modify_object(CharacterPtr ch, qint32 room);

void write_one_room(LegacyFile &stream, qint32 nr);
void write_mobile(LegacyFile &lf, CharacterPtr mob);
void write_object(LegacyFile &lf, ObjectPtr obj);
void write_mprog_recur(auto &stream, MobileProgramPtr mprg, bool mob);
qint32 load_new_help(auto &stream, qint32 reload = 0, CharacterPtr ch = {});

void init_char(CharacterPtr ch);
void clear_char(CharacterPtr ch);
void clear_object(ObjectPtr obj);
void reset_char(CharacterPtr ch);
void free_char(CharacterPtr ch);
void get(CharacterPtr ch, ObjectPtr obj_object, ObjectPtr sub_object, bool has_consent, cmd_t cmd);
void log_sacrifice(CharacterPtr ch, ObjectPtr obj, bool decay);
qint32 search_char_for_item_count(CharacterPtr ch, qint16 item_number, bool wearonly);
ObjectPtr search_char_for_item(CharacterPtr ch, qint16 item_number, bool wearonly);
qint32 find_door(CharacterPtr ch, QString type, QString dir);
qint32 palm(CharacterPtr ch, ObjectPtr obj_object, ObjectPtr sub_object, bool has_consent);
bool search_container_for_item(ObjectPtr obj, qint32 item_number);
ObjectPtr bring_type_to_front(CharacterPtr ch, qint32 item_type);

qint32 damage(CharacterPtr ch, CharacterPtr victim, qint32 dam, qint32 weapon_type, qint32 attacktype, qint32 weapon = {}, bool is_death_prog = false, ObjectPtr obj = {});
qint32 noncombat_damage(CharacterPtr ch, qint32 dam, const QString char_death_msg, const QString room_death_msg, const QString death_log_msg, qint32 type);
void send_damage(QString, CharacterPtr, ObjectPtr, CharacterPtr, QString, QString, qint32);
void send_damage(QString buf, CharacterPtr, ObjectPtr, CharacterPtr, QString dmg, QString buf2, qint32);
void affect_to_char(CharacterPtr ch, affected_typePtr af, qint32 duration_type = DC::PULSE_TIME);
QString one_argument(QString arguments, QString &arg1);
QString one_argumentnolow(QString arguments, QString &arg1);
void write_to_output(const QString txt, ConnectionPtr t);
void write_to_output(QByteArray txt, ConnectionPtr conn);
void write_to_output(QString txt, ConnectionPtr conn);
void write_to_output(QString txt, ConnectionPtr t);
void new_string_add(ConnectionPtr conn, QString str);
void telnet_ga(ConnectionPtr conn);
void telnet_sga(ConnectionPtr conn);
void telnet_echo_off(ConnectionPtr conn);
void telnet_echo_on(ConnectionPtr conn);
void parse_action(parse_t action, QString string, ConnectionPtr conn);
qint32 find_skill_num(QString name);
QString remove_trailing_spaces(QString arg);
qsizetype search_list(QString argument, const QStringList list);
qsizetype old_search_block(QString argument, qint32 begin, qint32 length, const QStringList list, qint32 mode);
void argument_interpreter(QString argument, QString first_arg, QString second_arg);
void half_chop(const QString str, QString arg1, QString arg2);
std::tuple<QString, QString> last_argument(QString arguments);
std::tuple<QString, QString> half_chop(QString arguments, const QChar token = ' ');
void chop_half(QString str, QString arg1, QString arg2);
void update_max_who(void);
bool is_abbrev(QString abbrev, QString word);
// bool is_abbrev( QString abbrev,  QString word);
// bool is_abbrev(const QString arg1, const QString arg2);
QString ltrim(QString str);
QString rtrim(QString str);
void add_mobspec(qint32 i);
void string_to_file(auto &stream, QString str);
void string_to_file(QTextStream &stream, QString str);
QString lf_to_crlf(QString &s1);
QString lf_to_crlf(QString s1);
FILE *legacyFileOpen(QString directory, QString filename, QString error_message);
void rebuild_rnum_references(qint32 startAt, qint32 type);
QString mprog_next_command(QString clist);
void save_clans(void);
QString color_to_code(QString color);
void debug_point(void);
qint32 r_new_meta_platinum_cost(qint32 start, qint64 plats);
qint32 r_new_meta_exp_cost(qint32 start, qint64 exp);
void setup_bandwidth();
void add_bandwidth(qint32 amount);
qint32 write_bandwidth();
qint32 get_bandwidth_start();
qint32 get_bandwidth_amount();
void page_string(ConnectionPtr conn, const QString str, qint32 keep_internal);
qint32 getRealSpellDamage(CharacterPtr ch);
void showStatDiff(CharacterPtr ch, qint32 base, qint32 random, bool swapcolors = false);
void board_write_msg(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board);
qint32 board_display_msg(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board);
qint32 board_remove_msg(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board);
void board_save_board(QMap<QString, BOARD_INFO>::iterator board);
void board_load_board();
qint32 board_show_board(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board);
qint32 fwrite_string(const QString buf, FILE *stream);
void new_edit_board_unlock_board(CharacterPtr ch, qint32 abort);
qint32 get_line_new(auto &stream, QString buf);
time_info_data mud_time_passed(time_t t2, time_t t1);
qint32 load_quests();
qint32 save_quests();
quest_infoPtr get_quest_struct(qint32);
quest_infoPtr get_quest_struct(QString);
void quest_update();
QString fname(QString namelist);
bool isprefix(const QString str, const QString namel);
qint32 get_number(QString *name);
qint32 get_number(QString &name);
qint32 get_number(QString &name);
void warn_if_duplicate_ip(CharacterPtr ch);
void record_msg(QString messg, CharacterPtr ch);
bool is_multi(CharacterPtr ch);
QString calc_condition(CharacterPtr ch, bool colour = false);
CharacterPtr get_charmie(CharacterPtr ch);
void remove_character(QString name, BACKUP_TYPE backup = NONE);
void remove_familiars(QString name, BACKUP_TYPE backup = NONE);
void char_to_store(CharacterPtr ch, class char_file_u4 *st, Time &tmpage); /* These data contain information about a players time data */
auto &operator<<(auto &out, RoomPtr room);
bool operator==(RoomPtr r1, RoomPtr r2);
room_t real_room(room_t virt);
bool isValidZoneKey(CharacterPtr ch, const zone_t zone_key);
bool isValidZoneCommandKey(CharacterPtr ch, const Zone &zone, const qsizetype zone_command_key);
qsizetype getZoneLastCommandNumber(const Zone &zone);
zone_t getZoneKey(CharacterPtr ch, const QString input, bool *ok = {});
quint64 getZoneCommandKey(CharacterPtr ch, const Zone &zone, const QString input, bool *ok = {});
zone_t zedit_add(CharacterPtr ch, QStringList arguments, Zone &zone);
bool isCommandTypeDirection(cmd_t cmd);
bool isCommandTypeCasino(cmd_t cmd);
auto getCommandFromDirection(qint32 dir) -> std::expected<cmd_t, bool>;
auto getDirectionFromCommand(cmd_t cmd) -> std::expected<qint32, bool>;
void produce_coredump(void *ptr = 0);
qint32 getRealSpellDamage(CharacterPtr ch);
void make_dust(CharacterPtr ch);
bool do_frostshield(CharacterPtr ch, CharacterPtr vict);
qint32 speciality_bonus(CharacterPtr ch, qint32 attacktype, qint32 level);
void make_husk(CharacterPtr ch);
void make_heart(CharacterPtr ch, CharacterPtr vict);
void make_head(CharacterPtr ch);
void make_arm(CharacterPtr ch);
void make_leg(CharacterPtr ch);
void make_bowels(CharacterPtr ch);
void make_blood(CharacterPtr ch);
void make_scraps(CharacterPtr ch, ObjectPtr obj);
void room_mobs_only_hate(CharacterPtr ch);
qint32 attack(CharacterPtr ch, CharacterPtr vict, qint32 type, qint32 attack = WEAR_WIELD);
void dam_message(qint32 dam, CharacterPtr ch, CharacterPtr vict, qint32 w_type, qint32 modifier);
void group_gain(CharacterPtr ch, CharacterPtr vict);
qint32 check_magic_block(CharacterPtr ch, CharacterPtr victim, qint32 attacktype);
qint32 check_riposte(CharacterPtr ch, CharacterPtr vict, qint32 attacktype);
qint32 check_shieldblock(CharacterPtr ch, CharacterPtr vict, qint32 attacktype);
bool check_parry(CharacterPtr ch, CharacterPtr vict, qint32 attacktype, bool display_results = true);
bool check_dodge(CharacterPtr ch, CharacterPtr vict, qint32 attacktype, bool display_results = true);
void disarm(CharacterPtr ch, CharacterPtr vict);
void trip(CharacterPtr ch, CharacterPtr vict);
qint32 checkCounterStrike(CharacterPtr, CharacterPtr);
qint32 doTumblingCounterStrike(CharacterPtr, CharacterPtr);
qint32 one_hit(CharacterPtr ch, CharacterPtr vict, qint32 type, qint32 weapon);
ReturnValue do_skewer(CharacterPtr ch, CharacterPtr vict, qint32 dam, qint32 wt, qint32 wt2, qint32 weapon);
void do_combatmastery(CharacterPtr ch, CharacterPtr vict, qint32 weapon);
ReturnValue do_behead_skill(CharacterPtr ch, CharacterPtr victim);
ReturnValue do_execute_skill(CharacterPtr, CharacterPtr, qint32);
qint32 weapon_spells(CharacterPtr ch, CharacterPtr vict, qint32 weapon);
void eq_damage(CharacterPtr ch, CharacterPtr vict, qint32 dam, qint32 weapon_type, qint32 attacktype);
void fight_kill(CharacterPtr ch, CharacterPtr vict, qint32 type, qint32 spec_type);
qint32 can_attack(CharacterPtr ch);
qint32 can_be_attacked(CharacterPtr ch, CharacterPtr vict);
qint32 second_attack(CharacterPtr ch);
qint32 third_attack(CharacterPtr ch);
qint32 fourth_attack(CharacterPtr ch);
qint32 second_wield(CharacterPtr ch);
void set_cantquit(CharacterPtr, CharacterPtr, bool = false);
bool is_pkill(CharacterPtr ch, CharacterPtr vict);
void raw_kill(CharacterPtr ch, CharacterPtr victim);
void do_pkill(CharacterPtr ch, CharacterPtr victim, qint32 type, bool vict_is_attacker = false);
void arena_kill(CharacterPtr ch, CharacterPtr victim, qint32 type);
void do_dead(CharacterPtr ch, CharacterPtr victim);
void eq_destroyed(CharacterPtr ch, ObjectPtr obj, qint32 pos);
bool is_stunned(CharacterPtr ch);
void update_flags(CharacterPtr vict);
void update_stuns(CharacterPtr ch);
void do_dam_msgs(CharacterPtr ch, CharacterPtr victim, qint32 dam, qint32 attacktype, qint32 weapon, qint32 filter = 0);
qint32 act_poisonous(CharacterPtr ch);
bool isHit(CharacterPtr ch, CharacterPtr victim, qint32 attacktype, qint32 &type, qint32 &reduce);
void inform_victim(CharacterPtr ch, CharacterPtr victim, qint32 dam);
CharacterPtr loop_followers(follow_type **f);
CharacterPtr get_highest_level_killer(CharacterPtr leader, CharacterPtr killer);
qint32 count_xp_eligibles(CharacterPtr leader, CharacterPtr killer, qint32 highest_level, qint32 *total_levels);
qint64 scale_char_xp(CharacterPtr ch, CharacterPtr killer, CharacterPtr victim, qint32 no_killers, qint32 total_levels, qint32 highest_level, qint64 base_xp, qint64 *bonus_xp);
void remove_active_potato(CharacterPtr vict);
void prepare_character_for_sixty(CharacterPtr ch);
bool isPaused(CharacterPtr mob);
void barb_magic_resist(CharacterPtr ch, qint32 old, qint32 nw);
void display_punishes(CharacterPtr ch, Character vict);
bool is_in_range(CharacterPtr ch, qint32 virt);
void isr_set(CharacterPtr ch);
ReturnValue mob_stat(CharacterPtr ch, CharacterPtr k);
void obj_stat(CharacterPtr ch, ObjectPtr j);
qint32 number_or_name(QString *name, qint32 *num);
qint32 mob_in_index(QString name, qint32 index);
qint32 obj_in_index(QString name, qint32 index);
void do_oload(CharacterPtr ch, qint32 rnum, qint32 cnt, bool random = false);
void do_mload(CharacterPtr ch, qint32 rnum, qint32 cnt);
void colorCharSend(QString s, CharacterPtr ch);
obj_list_t oload(CharacterPtr ch, qint32 rnum, qint32 cnt, bool random);
qint32 show_zone_commands(CharacterPtr ch, const Zone &zone, quint64 start = 0, quint64 num_to_show = 0, bool stats = false);
qint32 show_zone_commands(CharacterPtr ch, zone_t zone_key, quint64 start = 0, quint64 num_to_show = 0, bool stats = false);
void add_totem(ObjectPtr altar, ObjectPtr totem);
void remove_totem(ObjectPtr altar, ObjectPtr totem);
void add_totem_stats(CharacterPtr ch, qint32 stat = 0);
void remove_totem_stats(CharacterPtr ch, qint32 stat = 0);
bool others_clan_room(CharacterPtr ch, class Room *room);
void clan_login(CharacterPtr ch);
void clan_logout(CharacterPtr ch);
qint32 has_right(CharacterPtr ch, quint32 bit);
QString get_clan_name(qint32 nClan);
QString get_clan_name(CharacterPtr ch);
qint32 plr_rights(CharacterPtr ch);
void remove_clan_member(qint32 clannumber, CharacterPtr ch);
void free_member(ClanMemberPtr member);
ClanMemberPtr get_member(QString strName, qint32 clan_id);
void show_clan_log(CharacterPtr ch);
void clan_death(CharacterPtr ch, CharacterPtr killer);
void check_timer(void);

#define REMOVE_BIT(var, bit) ((var) = (var) & ~(bit))

QString qDebugQTextStreamLine(auto &stream, QString message = "Current line")
{
  assert(stream.status() == QTextStream::Status::Ok);
  auto current_pos = stream.pos();
  auto current_line = stream.readLine();
  assert(stream.status() == QTextStream::Status::Ok);

  if (!message.isEmpty())
  {
    qDebug("%s", qPrintable(u"%1: [%2]"_s.arg(message).arg(current_line)));
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
auto &operator<<(auto &out, MobileProgramPtr mobprogs)
{
  if (mobprogs)
  {
    write_mprog_recur(out, mobprogs, false);
    out << "|\n";
  }
  return out;
}
template <typename T>
T fread_int(auto &stream, T minval = std::numeric_limits<T>::min(), T maxval = std::numeric_limits<T>::max())
{
  T number;

  QString line = qDebugQTextStreamLine(stream, "");
  QStringList namelist = line.split(' ');
  QString arg1 = namelist.value(0);
  stream >> number >> Qt::ws;

  bool ok = false;
  if (std::is_signed<T>::value)
  {
    if (arg1.toLongLong(&ok) != number && ok)
    {
      qDebug() << u"fread_int<%1> value %2 from \"%3\" != %4"_s.arg(typeid(minval).name()).arg(arg1.toULongLong(&ok)).arg(arg1).arg(number);
    }
    else if (!ok)
    {
      qDebug() << u"fread_int<%1> arg2.toLongLong not ok."_s.arg(typeid(minval).name());
    }
  }
  else if (std::is_unsigned<T>::value)
  {
    if (arg1.toULongLong(&ok) != number && ok)
    {
      qDebug() << u"fread_int<%1> value %2 from \"%3\" != %4"_s.arg(typeid(minval).name()).arg(arg1.toULongLong(&ok)).arg(arg1).arg(number);
    }
    else if (!ok)
    {
      qDebug() << u"fread_int<%1> arg2.toULongLong not ok."_s.arg(typeid(minval).name());
    }
  }
  else
  {
    qFatal("arg1 neither signed nor quint32");
  }

  if (number < minval)
  {
    qDebug("increasing number");
    number = minval;
  }
  else if (number > maxval)
  {
    qDebug("decreasing number");
    number = maxval;
  }

  // qDebug() << "fread_int returning" << number;
  // qDebugQTextStreamLine(stream, "After fread_int");
  return number;
}

template <typename T>
T check_returns(T in_str)
{
  T new_string;
  for (auto checker = in_str.begin(); checker != in_str.end(); checker++)
  {
    if (*checker == '\n')
    {
      if (checker + 1 != in_str.end() && *(checker + 1) != '\r')
        new_string.push_back('\r');
    }
    new_string.push_back(*checker);
  }

  return new_string;
}

void affects_to_file(auto &out, ObjectPtr obj)
{
  for (qint32 i = {}; i < obj->num_affects; i++)
  {
    out << "A\n";
    out << obj->affected[i].location << " " << obj->affected[i].modifier << "\n";
  }
}

auto &operator<<(auto &out, const ObjectFlags &of)
{
  out << of.type_flag << " " << of.extra_flags << " " << of.wear_flags << " " << of.size << "\n";
  out << of.value[0] << " " << of.value[1] << " " << of.value[2] << " " << of.value[3] << " " << of.eq_level << "\n";
  out << of.weight << " " << of.cost << " " << of.more_flags << "\n";
  return out;
}

auto &operator<<(auto &out, RoomPtr room)
{
  auto temp_room_flags = room.room_flags;
  if (room.iFlags)
  {
    REMOVE_BIT(temp_room_flags, room.iFlags);
  }

  ExtraDescriptionPtr extra;
  if (!dc_->rooms.contains(room.number))
    return out;

  out << "#" << room.number << "\n";
  string_to_file(out, room.name);
  string_to_file(out, room.description);

  out << room.zone << " " << room.room_flags << " " << room.sector_type << "\n";

  /* exits */
  for (qint32 b = {}; b <= 5; b++)
  {
    if (!(room.dir_option[b]))
      continue;
    out << "D" << b << "\n";
    if (room.dir_option[b]->general_description)
      string_to_file(out, room.dir_option[b]->general_description);
    else
      out << "~\n"; // print blank
    if (room.dir_option[b]->keyword)
      string_to_file(out, room.dir_option[b]->keyword);
    else
      out << "~\n"; // print blank
    out << room.dir_option[b]->exit_info << " " << room.dir_option[b]->key << " " << room.dir_option[b]->to_room << "\n";
  } /* exits */

  /* extra descriptions */
  for (extra = room.ex_description; extra; extra = extra->next)
  {
    if (!extra)
      break;
    out << "E\n";
    if (extra->keyword)
      string_to_file(out, extra->keyword);
    else
      out << "~\n"; // print blank
    if (extra->description)
      string_to_file(out, extra->description);
    else
      out << "~\n"; // print blank
  } /* extra descriptions */

  DenyPtr deni;
  for (deni = room.denied; deni; deni = deni->next)
  {
    out << "B\n"
        << deni->vnum << "\n";
  }

  // Write out allowed classes if any
  for (qint32 i = {}; i < CLASS_MAX; i++)
  {
    if (room.allow_class[i] == true)
    {
      out << "C" << i << "\n";
    }
  }

  out << "S\n";
  return out;
}
auto &operator<<(auto &stream, ObjectPtr obj)
{
  if (!obj)
    return stream;

  stream << "#" << obj->obj_index_[obj->item_number].vnum() << "\n";
  string_to_file(stream, obj->name());
  string_to_file(stream, obj->short_description());
  string_to_file(stream, obj->long_description());
  string_to_file(stream, obj->ActionDescription());

  stream << qint32(obj->flags_.type_flag) << " "
         << obj->flags_.extra_flags << " "
         << obj->flags_.wear_flags << " "
         << obj->flags_.size << "\n";

  stream << obj->flags_.value[0] << " "
         << obj->flags_.value[1] << " "
         << obj->flags_.value[2] << " "
         << obj->flags_.value[3] << " "
         << obj->flags_.eq_level << "\n";

  stream << obj->flags_.weight << " "
         << obj->flags_.cost << " "
         << obj->flags_.more_flags << "\n";

  ExtraDescriptionPtr currdesc = obj->ex_description;
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

  if (obj->obj_index_[obj->item_number]->programs_)
  {
    write_mprog_recur(stream, obj->obj_index_[obj->item_number]->programs_, false);
    stream << "|\n";
  }

  stream << "S\n";

  return stream;
}
auto &operator>>(auto &stream, AuctionStates &at)
{
  quint64 buffer;
  stream >> buffer;
  at = AuctionStates(buffer);
  return stream;
}

template <typename T>
T parse_bitstrings(QString arg1, CharacterPtr ch = {}, T value = {})
{
  bool found = false;
  auto metaEnum = QMetaEnum::fromType<T>();

  for (qint32 x = {}; x < metaEnum.keyCount(); ++x)
  {
    auto obj_position = ObjectPosition(1 << x);
    if (is_abbrev(arg1, metaEnum.key(x)))
    {
      if (value.testFlag(obj_position))
      {
        // value
        value.setFlag(obj_position, false);
        if (ch)
          ch->send(u"%1 flag REMOVED.\r\n"_s.arg(metaEnum.key(x)));
      }
      else
      {
        value.setFlag(obj_position);
        if (ch)
          ch->send(u"%1 flag ADDED.\r\n"_s.arg(metaEnum.key(x)));
      }
      found = true;
      break;
    }
  }
  if (!found && ch)
    ch->sendln(u"No matching bits found."_s);
  return value;
}

void SETBIT(auto var, auto bit)
{
  (var)[(bit) / ASIZE] |= (1 << (((bit) - (((bit) / ASIZE) * ASIZE) - 1)));
}

void REMBIT(auto var, auto bit)
{
  (var)[(bit) / ASIZE] &= ~(1 << (((bit) - (((bit) / ASIZE) * ASIZE) - 1)));
}

void TOGBIT(auto var, auto bit)
{
  (var)[(bit) / ASIZE] ^= (1 << (((bit) - (((bit) / ASIZE) * ASIZE) - 1)));
}

bool ISSET(auto var, auto bit)
{
  return (var)[(bit) / ASIZE] & (1 << (((bit) - (((bit) / ASIZE) * ASIZE) - 1)));
}
bool IS_OBJ(auto ch)
{
  return ch->getType() == Character::Type::Object;
}
bool IS_IMMORTAL(auto ch) { return IS_MINLEVEL_PC(ch, IMMORTAL); }
bool IS_MORTAL(auto ch) { return IS_MAXLEVEL_PC(ch, IMMORTAL - 1); }
template <typename T, typename U, typename... Args>
void dc_sprintf(T &buffer, U cformat, Args... args)
{
  buffer = QString::asprintf(cformat, args...);
}

template <typename T, typename U, typename... Args>
void dc_snprintf(T &buffer, int, U cformat, Args... args)
{
  buffer = QString::asprintf(cformat, args...);
}

template <typename T>
void dc_strcpy(T &dst, T src)
{
  dst = src;
}

template <typename T>
void dc_strcpy(T &dst, const char *src)
{
  if (src)
    dst = src;
}

template <typename T, typename U>
void dc_strncpy(T &dst, T src, U n)
{
  dst = src;
}

template <typename T, typename U>
void dc_strncpy(T &dst, const char *src, U n)
{
  if (src)
    dst = src;
}

template <typename T>
void dc_strcat(T &dst, T src)
{
  dst += src;
}

template <typename T>
void dc_strcat(T &dst, const char *src)
{
  if (src)
    dst += src;
}

template <typename T>
int dc_strcmp(T dst, T src)
{
  return dst.compare(src);
}

template <typename T>
int dc_strcmp(T dst, const char *src)
{
  if (src)
    return dst.compare(src);
  return -1;
}

template <typename T>
int dc_strcmp(const char *dst, T src)
{
  if (dst)
    return src.compare(dst);
  return -1;
}

template <typename T>
qsizetype dc_strlen(T dst)
{
  return dst.length();
}

template <typename T>
qint64 dc_atoi(T str)
{
  return str.toLongLong();
}

void write_programs(auto &stream, auto &programs)
{
  for (const auto &program : programs)
  {
    if (!program)
      continue;

    if (program->is_object_)
      stream << "\\" << program->typeString() << " ";
    else
      stream << ">" << program->typeString() << " ";

    if (program->arglist.isEmpty())
      string_to_file(stream, "Saved During Edit");
    else
      string_to_file(stream, program->arglist);

    if (program->comlist.isEmpty())
      string_to_file(stream, "Saved During Edit");
    else
      string_to_file(stream, program->comlist);
  }
}

auto &MOB_WAIT_STATE(auto ch)
{
  return ch->deaths;
}

void REM_WAIT_STATE(auto czh, auto cycle)
{
  if (czh->conn_)
  {
    if (czh->conn_->wait < cycle)
    {
      czh->conn_->wait = 0;
    }
    else
    {
      czh->conn_->wait -= cycle;
    }
  }
  else
  {
    if (czh->isNonPlayer())
    {
      if (MOB_WAIT_STATE(czh) < cycle)
      {
        MOB_WAIT_STATE(czh) = 0;
      }
      else
      {
        MOB_WAIT_STATE(czh) -= cycle;
      }
    }
  }
}

void check_timer();

void REMOVE_FROM_LIST(auto item, auto head, auto next)
{
  if (item == head)
    head = item->next;
  else
  {
    auto temp = head;
    while (temp && temp->next != item)
      temp = temp->next;

    if (temp)
      temp->next = item->next;
  }
}

template <typename T>
T MIN(T a, T b)
{
  if (a < b)
    return a;
  else
    return b;
}

template <typename T>
T MAX(T a, T b)
{
  if (a > b)
    return a;
  else
    return b;
}

template <typename T>
T LOWER(T c)
{
  return c.toLower();
}

template <typename T>
T UPPER(T c)
{
  return c.toUpper();
}

// #define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r' || (ch) == '|')
// replaced to leave off the pipe and put it eclusively in comm.c
// where we could check to see if we were in an editor first.
auto ISNEWL(auto ch)
{
  return ch == '\n' || ch == '\r';
}

auto CAP(auto st)
{
  *st = UPPER(*st);
  return st;
}

void TOGGLE_BIT(auto var, auto bit)
{
  (var) = (var) ^ (bit);
}

bool IS_AFFECTED(auto ch, auto skill)
{
  return ISSET(ch->affected_by, skill);
}

qint32 DARK_AMOUNT(qint32 room);
bool IS_DARK(qint32 room);
bool IS_LIGHT(auto room) { return !IS_DARK(room); }

auto HSHR(auto ch)
{
  if (ch->sex)
  {
    if (ch->sex == 1)
    {
      return "his";
    }
    else
    {
      return "her";
    }
  }
  return "its";
}

auto HSSH(auto ch)
{
  return (ch)->sex ? (((ch)->sex == 1) ? "he" : "she") : "it";
}

auto HMHR(auto ch)
{
  return (ch)->sex ? (((ch)->sex == 1) ? "him" : "her") : "it";
}

bool CAN_GO(auto ch, auto door)
{
  return EXIT(ch, door) && (EXIT(ch, door)->to_room != DC::NOWHERE) && (EXIT(ch, door)->to_room != DC::NOWHERE) && !isSet(EXIT(ch, door)->exit_info, EX_CLOSED);
}

template <class T>
T fread_bitvector(auto &in)
{
  auto value = fread_uint(in);
  T flags = T::fromInt(value);

  return flags;
}
auto &operator>>(auto &stream, ObjectPtr obj)
{
  qint32 loc, mod, nr;

  QChar chk, c;
  ExtraDescriptionPtr new_new_descr;

  if (obj == nullptr)
  {
    return stream;
  }

  clear_object(obj);
  stream >> c;
  if (c == '#')
  {
    stream >> nr;
  }
  stream >> Qt::ws;

  obj->name(fread_string(stream));

  obj->short_description(fread_string(stream));
  obj->long_description(fread_string(stream));
  obj->ActionDescription(fread_string(stream));
  obj->table = {};
  obj->dc_->currentVNUM(nr);
  obj->dc_->currentName(obj->name());
  obj->dc_->currentType("Object");

  // numeric data

  obj->flags_.type_flag = fread_int(stream, -1000, 2147483467);

  obj->flags_.extra_flags = fread_uint(stream);
  obj->flags_.wear_flags = fread_bitvector<ObjectPositions>(stream);
  obj->flags_.size = fread_uint<quint16>(stream);

  obj->flags_.value[0] = fread_int(stream, -1000, 2147483467);
  obj->flags_.value[1] = fread_int(stream, -1000, 2147483467);
  obj->flags_.value[2] = fread_int(stream, -1000, 2147483467);
  obj->flags_.value[3] = fread_int(stream, -1000, 2147483467);
  obj->flags_.eq_level = fread_int<qint64>(stream, -1000, IMPLEMENTER);
  obj->flags_.weight = fread_int(stream, -1000, 2147483467);
  obj->flags_.cost = fread_int(stream, -1000, 2147483467);
  obj->flags_.more_flags = fread_uint<quint32>(stream);

  // currently not stored stream object file
  obj->flags_.timer = {};

  obj->ex_description = {};
  obj->affected = {};
  obj->num_affects = {};
  // other flags

  stream >> chk;

  QString log_buf = {};
  while (chk != 'S')
  {
    switch (chk.toLatin1())
    {
    // skip whitespace
    case ' ':
    case '\n':
      break;
    case 'E':
    {
      auto new_new_descr = new ExtraDescription;
      new_new_descr->keyword_ = fread_string(stream);
      new_new_descr->description_ = fread_string(stream);
      new_new_descr->next = obj->ex_description;
      obj->ex_description = new_new_descr;
    }
    break;

    case '\\':
      // ungetc( '\\', stream );
      // mprog_read_programs( stream, nr,ignore );
      break;

    case 'A':
      // these are only two members of obj_affected_type, so nothing else needs initializing
      loc = fread_int(stream, -1000, 2147483467);
      mod = fread_int(stream, -1000, 1000);
      add_obj_affect(obj, loc, mod);
      break;

    default:
      logbug(u"Illegal obj addon flag %1 stream obj %2."_s.arg(chk).arg(obj->name()));
      break;
    } // switch
      // read stream next flag
    stream >> chk;
  }

  obj->in_room = DC::NOWHERE;
  obj->next_skill = {};
  obj->next_content = {};
  obj->carried_by = {};
  obj->equipped_by = {};
  obj->in_obj = {};
  obj->contains = {};
  obj->item_number = {};

  return stream;
}

void write_object(ObjectPtr obj, auto &out)
{
  out << u"#%1\n"_s.arg(obj->dc_->obj_index_[obj->item_number].vnum());
  string_to_file(out, obj->name());
  string_to_file(out, obj->short_description());
  string_to_file(out, obj->long_description());
  string_to_file(out, obj->ActionDescription());
  out << obj->flags_;
  out << obj->ex_description;
  affects_to_file(out, obj);
  out << obj->dc_->obj_index_[obj->item_number]->programs_;
  out << "S\n";
}

void string_to_file(auto &stream, QString str)
{
  stream << str.remove('\r').toStdString() << "~\n";
}

template <typename T>
T fread_uint(auto &in, T minval = std::numeric_limits<T>::min(), T maxval = std::numeric_limits<T>::max())
{
  T val;
  in >> val;
  return val;
}

QString fread_string(auto &stream, bool *ok = nullptr)
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

QString fread_word(auto &stream)
{
  QString buffer;
  stream >> buffer;
  return buffer;
}

QChar fread_char(auto &stream)
{
  if (stream.atEnd())
  {
    perror("fread_char: premature EOF");
    abort();
  }

  QChar c;
  stream >> c;

  return c;
}
QString fread_string_new(auto &stream)
{
  QByteArray return_buffer;
  while (stream.canReadLine())
  {
    auto buffer = stream.readLine();
    auto index_of_tilde = buffer.indexOf("~");
    if (index_of_tilde == -1)
    {
      return_buffer += buffer.trimmed().append("\r\n");
    }
    else
    {
      buffer.resize(index_of_tilde);
      return_buffer += buffer.trimmed();
      break;
    }
  }
  return return_buffer;
}

auto &operator>>(QTextStream &stream, Room &room)
{
  room_t room_number = {};
  QString temp = {};
  qint32 dir = {};
  ExtraDescriptionPtr new_new_descr{};
  zone_t zone_nr = {};

  auto c = fread_char(stream);

  if (c != '$')
  {
    room_number = fread_int<room_t>(stream, 0, 1000000);
    temp = fread_string(stream, 0);

    if (room_number)
    {
      /*
      dc_->currentVNUM(room_number);
      dc_->currentType("Room");
      dc_->currentName(temp);

      if (room_number >= dc_->top_of_world_alloc)
      {
        dc_->top_of_world_alloc = room_number + 200;
      }

      if (dc_->top_of_world < room_number)
        dc_->top_of_world = room_number;
      */

      // room.paths_ = {};
      room.number = room_number;
      room.name_ = temp;
    }
    QString description = fread_string(stream, 0);
    if (room_number)
    {
      room.description_ = description;
      room.tracks_ = {};
      room.denied = {};
      // dc_->total_rooms++;
    }
    // Ignore recorded zone number since it may not longer be valid
    fread_int<quint64>(stream, -1, 64000); // zone nr

    if (room_number)
    {
      // Go through the zone table until room.number is
      // in the current zone.

      bool found = false;
      zone_t zone_nr = {};
      for (auto [zone_key, zone] : room.dc_->zones_.asKeyValueRange())
      {
        if (zone.getBottom() <= room.number && zone.getTop() >= room.number)
        {
          found = true;
          zone_nr = zone_key;
          break;
        }
      }
      if (!found)
      {
        // QString error = u"Room %1 is outside of any zone."_s.arg(room_number);
        // dc_->logentry(error);
        // dc_->logentry(u"Room outside of ANY zone.  ERROR"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
      }
      else
      {
        auto &zone = room.dc_->zones_[zone_nr];
        if (room_number >= zone.getBottom() && room_number <= zone.getTop())
        {
          if (room_number < zone.getRealBottom() || zone.getRealBottom() == 0)
          {
            zone.setRealBottom(room_number);
          }
          if (room_number > zone.getRealTop() || zone.getRealTop() == 0)
          {
            zone.setRealTop(room_number);
          }
        }
        room.zone = zone_nr;
      }
    }

    quint32 room_flags = fread_uint<quint32>(stream);

    if (room_number)
    {
      room.room_flags = room_flags;
      if (isSet(room.room_flags, NO_ASTRAL))
      {
        REMOVE_BIT(room.room_flags, NO_ASTRAL);
      }

      // This bitvector is for runtime and not stored in the files, so just initialize it to 0
      room.temp_room_flags = {};
    }

    qint32 sector_type = fread_int<qint32>(stream, -1, 64000);

    if (room_number)
    {
      room.sector_type = sector_type;
      room.funct = {};
      room.contents_ = {};
      room.people_ = {};
      room.light = {}; /* Zero light sources */

      for (size_t tmp = {}; tmp <= 5; tmp++)
        room.dir_option[tmp] = {};

      room.ex_description = {};
    }

    for (;;)
    {
      c = fread_char(in); /* dir field */

      /* direction field */
      if (c == 'D')
      {
        dir = fread_int(stream, 0, 5);
        setup_dir(stream, room_number, dir);
      }
      /* extra description field */
      else if (c == 'E')
      {
        // strip off the \n after the E
        if (fread_char(stream) != '\n')
        {
          // fseek(stream, -1, SEEK_CUR);
          stream.seek(-1);
        }

        new_new_descr = new ExtraDescription;
        new_new_descr->keyword_ = fread_string(stream, 0);
        new_new_descr->description_ = fread_string(stream, 0);

        if (room_number)
        {
          new_new_descr->next = room.ex_description;
          room.ex_description = new_new_descr;
        }
        else
        {
          new_new_descr = {};
        }
      }
      else if (c == 'B')
      {
        DenyPtr deni;

        deni = new Deny;
        deni->vnum = fread_int(stream, -1, 2147483467);

        if (room_number)
        {
          deni->next = room.denied;
          room.denied = deni;
        }
        else
        {
          deni = {};
        }
      }
      else if (c == 'S') /* end of current room */
        break;
      else if (c == 'C')
      {
        qint32 c_class = fread_int(stream, 0, CLASS_MAX);
        if (room_number)
        {
          room.allow_class[c_class] = true;
        }
      }
    } // of for (;;) (get directions and extra descs)
  } // if == $

  return stream;
}

qsizetype count_hash_records(auto &stream)
{
  return stream.readAll().count('#');
}

QString ANA(ObjectPtr obj);
QString SANA(ObjectPtr obj);

bool IS_FAMILIAR(auto ch) { return IS_AFFECTED(ch, AFF_FAMILIAR); }

bool IS_MINLEVEL_PC(auto ch, auto level) { return ch->getLevel() >= level && ch->isPlayer(); }
bool IS_MAXLEVEL_PC(auto ch, auto level) { return ch->getLevel() <= level && ch->isPlayer(); }
bool IS_MINLEVEL_NPC(auto ch, auto level) { return ch->getLevel() >= level && ch->isNonPlayer(); }

#define GET_RDEATHS(ch) ((ch)->player->rdeaths)
#define GET_PDEATHS(ch) ((ch)->player->pdeaths)
#define GET_PKILLS(ch) ((ch)->player->pkills)
#define GET_PKILLS_TOTAL(ch) ((ch)->player->pklvl)

#define GET_PKILLS_LOGIN(ch) ((ch)->player->totalpkills)
#define GET_PKILLS_TOTAL_LOGIN(ch) ((ch)->player->totalpkillslv)
#define GET_PDEATHS_LOGIN(ch) ((ch)->player->pdeathslogin)

#define GET_GROUP_KILLS(ch) ((ch)->player->group_kills)
#define GET_GROUP_PKILLS(ch) ((ch)->player->group_pkills)
#define GET_GROUP_PKILLSTOTAL(ch) ((ch)->player->grpplvl)

#define GET_HP_METAS(ch) ((ch)->hpmetas)
#define GET_MANA_METAS(ch) ((ch)->manametas)
#define GET_MOVE_METAS(ch) ((ch)->movemetas)
#define GET_AC_METAS(ch) ((ch)->acmetas)
#define GET_AGE_METAS(ch) ((ch)->agemetas)
#define GET_KI_METAS(ch) ((ch)->player->kimetas)

#define GET_POS(ch) ((ch)->getPosition())
#define GET_COND(ch, i) ((ch)->conditions[(i)])

#define GET_SHORT_ONLY(ch) (qPrintable((ch)->short_description()))

#define GET_OBJ_SHORT(obj) (qPrintable((obj)->short_description()))

#define GET_OBJ_RNUM(obj) ((obj)->item_number)
#define GET_OBJ_VAL(obj, val) ((obj)->flags_.value[(val)])
#define GET_OBJ_VROOM(obj) ((obj)->vroom)
#define GET_OBJ_EXTRA(obj) ((obj)->flags_.extra_flags)
#define GET_OBJ_TIMER(obj) ((obj)->flags_.timer)
#define GET_OBJ_TYPE(obj) ((obj)->flags_.type_flag)
#define GET_OBJ_WEAR(obj) ((obj)->flags_.wear_flags)
#define GET_OBJ_COST(obj) ((obj)->flags_.cost)
#define GET_OBJ_RENT(obj) ((obj)->flags_.cost_per_day)
#define GET_OBJ_VNUM(obj) (GET_OBJ_RNUM(obj) >= 0 ? obj->dc_->obj_index_[GET_OBJ_RNUM(obj)].vnum() : -1)
#define VALID_ROOM_RNUM(rnum) ((rnum) != DC::NOWHERE && (rnum) <= dc_->top_of_world)
#define GET_ROOM_VNUM(rnum) ((qint32)(VALID_ROOM_RNUM(rnum) ? dc_->world[(rnum)].number : DC::NOWHERE))

#define GET_TOGGLES(ch) ((ch)->player->toggles)

#define GET_CLASS(ch) ((ch)->c_class)
#define GET_AGE(ch) ((ch)->age().year)

#define GET_STR(ch) ((ch)->str)
#define GET_DEX(ch) ((ch)->dex)
#define GET_INT(ch) ((ch)->intel)
#define GET_WIS(ch) ((ch)->wis)
#define GET_CON(ch) ((ch)->con)

#define GET_STR_BONUS(ch) ((ch)->str_bonus)
#define GET_DEX_BONUS(ch) ((ch)->dex_bonus)
#define GET_INT_BONUS(ch) ((ch)->intel_bonus)
#define GET_WIS_BONUS(ch) ((ch)->wis_bonus)
#define GET_CON_BONUS(ch) ((ch)->con_bonus)

#define GET_RAW_STR(ch) ((ch)->raw_str)
#define GET_RAW_DEX(ch) ((ch)->raw_dex)
#define GET_RAW_INT(ch) ((ch)->raw_intel)
#define GET_RAW_WIS(ch) ((ch)->raw_wis)
#define GET_RAW_CON(ch) ((ch)->raw_con)

#define GET_POISON_AMOUNT(ch) ((ch)->poison_amount)

#define STRENGTH_APPLY_INDEX(ch) \
  (GET_STR(ch))

#define GET_AC(ch) ((ch)->armor)
#define GET_ARMOR(ch) ((ch)->armor + dex_app[GET_DEX((ch))].ac_mod)
#define GET_HIT(ch) ((ch)->hit)
#define GET_RAW_HIT(ch) ((ch)->raw_hit)
#define GET_MAX_HIT(ch) (hit_limit(ch))
#define GET_MOVE(ch) ((ch)->getMove())
#define GET_RAW_MOVE(ch) ((ch)->raw_move)
#define GET_MAX_MOVE(ch) ((ch)->move_limit())
#define GET_MANA(ch) ((ch)->mana)
#define GET_RAW_MANA(ch) ((ch)->raw_mana)
#define GET_MAX_MANA(ch) (mana_limit(ch))
#define GET_KI(ch) ((ch)->ki)
#define GET_RAW_KI(ch) ((ch)->raw_ki)
#define GET_MAX_KI(ch) ((ch)->max_ki)

#define GET_PLATINUM(ch) ((ch)->plat)
#define GET_BANK(ch) ((ch)->player->bank)
#define GET_CLAN(ch) ((ch)->clan)

#define GET_HEIGHT(ch) ((ch)->height)
#define GET_WEIGHT(ch) ((ch)->weight)
#define GET_SEX(ch) ((ch)->sex)
#define GET_HITROLL(ch) ((ch)->hitroll)
#define GET_REAL_HITROLL(ch) ((ch)->hitroll + dex_app[GET_DEX((ch))].tohit)
#define GET_DAMROLL(ch) ((ch)->damroll)
#define GET_REAL_DAMROLL(ch) ((ch)->damroll + str_app[GET_STR((ch))].todam)
#define GET_QPOINTS(ch) ((ch)->player->quest_points)

auto GET_SPELLDAMAGE(auto ch)
{
  return ch->spelldamage;
}

// #define GET_BITV(ch) ((ch)->race == 1 ? 1 : (1 << (((ch)->race) - 1)))
auto getBitvector(auto value)
{
  if (value == 0)
  {
    return 0;
  }

  if (value == 1)
  {
    return 1;
  }

  return 1 << (value - 1);
}
#define IS_UNDEAD(ch) ((ch->race == RACE_UNDEAD) || (ch->race == RACE_GHOST))

#define AWAKE(ch) (GET_POS(ch) != position_t::SLEEPING)

#define IS_ANONYMOUS(ch) (ch->isNonPlayer() ? 1 : ((ch->getLevel() >= 101) ? 0 : isSet((ch)->player->toggles, Player::PLR_ANONYMOUS)))
/*
inline const short IS_ANONYMOUS(CharacterPtr ch)
{
  if (ch->isNonPlayer())
     // this should really never be called on mobs
     return 1;
  else if (ch->getLevel() >= 101)
     return 0;
  else
     return (isSet(ch->player->toggles, Player::PLR_ANONYMOUS) != 0);
}
*/
/* Object And Carry related macros */

#define GET_ITEM_TYPE(obj) ((obj)->flags_.type_flag)
#define GET_MOB_TYPE(mob) ((mob)->mobdata->mob_flags.type)
#define GET_OBJ_WEIGHT(obj) ((obj)->flags_.weight)

#define CAN_WEAR(obj, part) (isSet((obj)->flags_.wear_flags, part))

#define CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w + ch->has_skill(SKILL_VIGOR))
#define CAN_CARRY_N(ch) (5 + GET_DEX(ch))
#define IS_CARRYING_W(ch) ((ch)->carry_weight)
#define IS_CARRYING_N(ch) ((ch)->carry_items)

#define CAN_CARRY_OBJ(ch, obj)                                       \
  (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) && \
   ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))
#define CAN_GET_OBJ(ch, obj)                              \
  (CAN_WEAR((obj), TAKE) && CAN_CARRY_OBJ((ch), (obj)) && \
   CAN_SEE_OBJ((ch), (obj)))

#define IS_OBJ_STAT(obj, stat) (isSet((obj)->flags_.extra_flags, stat))
#define IS_SPECIAL(obj) (IS_OBJ_STAT(obj, ITEM_SPECIAL))
#define NOT_SPECIAL(obj) (!IS_SPECIAL(obj))

#define IS_CONTAINER(obj) (GET_ITEM_TYPE(obj) == ITEM_CONTAINER)
#define NOT_CONTAINER(obj) (!IS_CONTAINER(obj))

#define IS_ALTAR(obj) (GET_ITEM_TYPE(obj) == ITEM_ALTAR)
#define NOT_ALTAR(obj) (!IS_ALTAR(obj))

#define IS_KEYRING(obj) (GET_ITEM_TYPE(obj) == ITEM_KEYRING)
#define NOT_KEYRING(obj) (!IS_KEYRING(obj))

#define IS_KEY(obj) (GET_ITEM_TYPE(obj) == ITEM_KEY)
#define NOT_KEY(obj) (!IS_KEY(obj))

#define ARE_CONTAINERS(obj) (IS_CONTAINER(obj) || IS_ALTAR(obj) || IS_KEYRING(obj))
#define NOT_CONTAINERS(obj) (NOT_CONTAINER(obj) && NOT_ALTAR(obj) && NOT_KEYRING(obj))

/* character name/short_desc(for mobs) or someone?  */

#define PERS(ch, vict) ( \
    ch->getLevel() > MIN_GOD ? (CAN_SEE(vict, ch) ? qPrintable(ch->shortdesc_or_name()) : "an immortal presence") : (CAN_SEE(vict, ch) ? qPrintable(ch->shortdesc_or_name()) : "someone"))

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? (obj)->short_description : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? fname((obj)->name) : "something")

#define IS_EXIT(room, door) (dc_->world[(room)].dir_option[(door)])
#define EXIT_TO(room, door) (dc_->world[(room)].dir_option[(door)]->to_room)
#define IS_OPEN(room, door) (!isSet(dc_->world[(room)].dir_option[(door)]->exit_info, EX_CLOSED))

#define OUTSIDE(ch) (!isSet(dc_->world[(ch)->in_room].room_flags, INDOORS))

#define GET_ALIGNMENT(ch) ((ch)->alignment)
#define IS_GOOD(ch) (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch) (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_SINGING(ch) (!((ch)->songs.isEmpty()))

QString str_hsh(const QString);
bool ishashed(QString);

template <typename T>
T double_dollars(T source)
{
  T destination = {};

  for (const auto &c : source)
  {
    if (c == '$')
    {
      destination += "$$";
    }
    else
    {
      destination += c;
    }
  }

  return destination;
}

// Tested in TestUtility::test_space_to_underscore
template <typename T>
T space_to_underscore(T str)
{
  for (auto &c : str)
  {
    if (c == ' ')
    {
      c = '_';
    }
  }

  return str;
}

// Tested in TestUtility::str_n_nosp_cmp_begin
template <typename T>
MatchType str_n_nosp_cmp_begin(T arg1, T arg2)
{
  auto tmp_arg1 = space_to_underscore(arg1);
  auto tmp_arg1_len = tmp_arg1.length();
  auto tmp_arg2 = space_to_underscore(arg2);
  auto tmp_arg2_len = tmp_arg2.length();

  qint32 compare_result = -1;
  if constexpr (std::convertible_to<T, std::string_view>)
  {
    compare_result = strncasecmp(tmp_arg1.c_str(), tmp_arg2.c_str(), tmp_arg1_len);
  }
  else if constexpr (std::convertible_to<T, QStringView>)
  {
    tmp_arg2.truncate(tmp_arg1_len);
    compare_result = tmp_arg1.compare(tmp_arg2, Qt::CaseInsensitive);
  }
  else
  {
    static_assert(false, "Unhandled variable type passed to str_n_nosp_cmp_begin");
  }

  if (compare_result == 0)
  {
    if (tmp_arg1_len == tmp_arg2_len)
    {
      return MatchType::Exact;
    }
    else
    {
      return MatchType::Subset;
    }
  }
  else
  {
    return MatchType::Failure;
  }
}

// qint32 dc_->number(qint32 from, qint32 to);

qint32 dice(qint32 number, qint32 size, QRandomGenerator *rng = QRandomGenerator::global());

qint32 str_cmp(QString arg1, QString arg2);

bool str_nosp_equal(QString arg1, QString arg2);
bool str_n_nosp_equal(QString arg1, QString arg2, qsizetype pos);
QString str_nospace(QString stri);

void logarena(QString message);
void logbug(QString message);
void logclan(QString message);
void logprayer(QString message);
void logquest(QString message);
void logsocket(QString message);
void logvault(QString message);
void logdatabase(QString message);
void logdebug(QString message);
void loggod(QString message);
void loghelp(QString message);
void logmisc(QString message);
void logmortal(QString message);
void logworld(QString message);
void logobjects(QString message);
void logplayer(QString message);

void sprintbit(uint value[], const QStringList names, QString result);
QString sprintbit(uint value[], const QStringList names);

void sprintbit(quint32 vektor, const QStringList names, QString result);
QString sprintbit(quint32 vektor, const QStringList names);

void sprintbit(quint32 vektor, QStringList names, QString result);
QString sprintbit(quint32 vektor, QStringList names);

QString sprinttype(qint32 type, const QStringList names);

void sprinttype(qint32 type, QList<const QString>, QString result);
void sprinttype(qint32 type, QStringList, QString result);
QString sprinttype(quint64 type, QStringList names);

// void sprinttype(quint64 type, QStringList names, QString result);
template <typename T>
void sprinttype(T type, QStringList names, QString result)
{
  if (result == nullptr)
  {
    return;
  }
  result = names.value(static_cast<qsizetype>(type), "Undefined");
}

QString sprinttype(qint32 type, QList<const QString>);

qint32 consttype(QString search_str, const QStringList names);
QString constindex(const qsizetype index, const QStringList names);
// bool is_number(const QString str);
bool is_number(QString str);

bool isprefix(QString str, QString namel);

bool isexact(QString arg, joining_t &namelist);
bool isexact(QString arg, QStringList namelist);
bool isexact(QString arg, QString namelist);

QList<QString> splitstring(QString splitme, QString delims, bool ignore_empty = false);
QString joinstring(QList<QString> joinme, QString delims, bool ignore_empty = false);

void send_to_outdoor(QString messg);
void send_to_zone(const QString messg, qint32 zone);
void weather_and_time(qint32 mode);
void night_watchman(void);
qint32 file_to_string(const QString name, QString buf);
void save_char_obj_db(CharacterPtr ch);

void send_to_all(QString messg);

qint32 write_to_descriptor_fd(qint32 desc, QString txt);
void write_to_q(const QString txt, QQueue<QString> &queue);

void automail(QString name);
bool file_exists(const QString);

template <typename T>
bool check_range_valid_and_convert(T &value, QString buf, T begin, T end)
{
  bool ok = false;

  if (std::is_unsigned<T>::value)
  {
    value = buf.toULongLong(&ok);
  }
  else if (std::is_signed<T>::value)
  {
    value = buf.toLongLong(&ok);
  }

  if (!ok)
  {
    value = {};
    return false;
  }

  if (value < begin)
  {
    value = begin;
    return false;
  }

  if (value > end)
  {
    value = end;
    return false;
  }

  return true;
}

bool check_valid_and_convert(qint32 &value, QString buf);

const QString pluralize(qint32 qty, const QString ending = "s");
size_t nocolor_strlen(const QString s);
size_t nocolor_strlen(const QStringView str);

qsizetype find(QString haystack, auto needle, qsizetype pos)
{
  return haystack.indexOf(needle, pos);
}

template <typename T>
T remove_all_codes(T input)
{
  auto found_pos = find(input, "$", 0);
  decltype(found_pos) pos{}, skip = {};

  while (found_pos != -1)
  {
    skip = 1;

    if (found_pos + 1 <= input.length())
    {
      try
      {
        input = input.replace(found_pos, 1, "$$");
        skip = 2;
      }
      catch (...)
      {
      }
    }
    pos = found_pos + skip;
    found_pos = find(input, "$", pos);
  }

  return input;
}

template <typename T>
T remove_non_color_codes(T input)
{
  auto found_pos = find(input, "$", 0);
  decltype(found_pos) pos = {};

  T output = {};
  while (found_pos != -1)
  {
    if (found_pos + 1 == input.length())
    {
      output += input.sliced(0, found_pos + 1);
      output += "$";
      input = input.remove(0, found_pos + 1);
      output += input;
      return output;
    }

    QChar code = input.at(found_pos + 1);
    switch (code.toLatin1())
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'I':
    case 'L':
    case '*':
    case 'R':
    case 'B':
      output += input.sliced(0, found_pos + 2);
      input = input.remove(0, found_pos + 2);
      break;
    default:
      output += input.sliced(0, found_pos + 1);
      output += "$";
      input = input.remove(0, found_pos + 1);
      break;
    }
    found_pos = find(input, "$", 0);
  }
  output += input;

  return output;
}

bool str_prefix(const QString astr, const QString bstr);
bool str_infix(QString astr, QString bstr);

QString replaceString(QString message, QString find, QString replace);
QString replaceString(QString message, QString find, QString replace);
QString numToStringTH(qint32 number);
bool champion_can_go(qint32 room);
bool class_can_go(qint32 ch_class, qint32 room);

QString find_profession(qint32 c_class, quint8 profession);

QString get_isr_string(quint32, qint8);

bool file_exists(QString filename);
bool file_exists(QString filename);
bool char_file_exists(QString name);
qint32 random_percent_change(uint percentage, qint32 value);
qint32 random_percent_change(qint32 from, qint32 to, qint32 value);

void special_log(QString message);
qint32 graf(qint32 age, qint32 p0, qint32 p1, qint32 p2, qint32 p3, qint32 p4, qint32 p5, qint32 p6);
qint32 len_cmp(const QString s1, const QString s2);
qint32 len_cmp(QString s1, QString s2);

act_return act_to_room(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags);
act_return act_to_victim(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags);
act_return act_to_character(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags);
act_return act_to_zone(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags);
act_return act_to_world(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags);
act_return act_to_group(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags);
act_return act_to_room_not_group(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags);
act_return act(QString str, CharacterPtr ch, ObjectPtr obj, ObjectPtr vict_obj, qint16 destination, qint16 flags);
act_return act(QString str, CharacterPtr ch, ObjectPtr obj, CharacterPtr vict_obj, qint16 destination, qint16 flags);

send_tokens_return send_tokens(TokenList &tokens, CharacterPtr ch, CharacterPtr to, CharacterPtr victim, ObjectPtr obj, ObjectPtr vic_obj, qint32 flags);

void send_message(const QString str, CharacterPtr to);
void send_message(QString str, CharacterPtr to);

qint32 search_skills(QString arg, CharacterClassSkill *list_skills);
qint32 search_skills2(qint32 arg, CharacterClassSkill *list_skills);
qint32 guild(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, const QString arg, CharacterPtr owner);

qint32 dc_fprintf(auto &stream, const QString format, ...);

QMap<qint32, qint32> fill_skill_cost(void);

skill_quest *find_sq(qint32 sq);
skill_quest *find_sq(QString);

qint32 dam_percent(qint32 learned, qint32 damage);

/* ======================================================================== */
static const QMap<qint32, qint32> skill_cost = fill_skill_cost();

namespace Combinables
{

  class Brew
  {
  public:
    class recipe
    {
    public:
      bool operator<(const recipe &r2) const
      {
        if (container < r2.container)
        {
          return true;
        }
        else if (container == r2.container)
        {
          if (liquid < r2.liquid)
          {
            return true;
          }
          else if (liquid == r2.liquid)
          {
            if (herb < r2.herb)
            {
              return true;
            }
          }
        }

        return false;
      }
      vnum_t herb = {};
      qint64 liquid = {};
      vnum_t container = {};
    };

    Brew();
    ~Brew();
    void load(void);
    void save(void);
    void list(CharacterPtr ch);
    qint32 add(CharacterPtr ch, QString argument);
    qint32 remove(CharacterPtr ch, QString argument);
    qint32 size(void);
    qint32 find(recipe);

  private:
    static QMap<recipe, qint32> recipes;
    class loadError
    {
    };
    static const QString RECIPES_FILENAME;
    static bool initialized;
  };

  // I feel just wrong doing this, but it's the easiest way at the moment
  // this really should be combined into a parent class with 2 children
  // inheriting common functionality...

  class Scribe
  {
  public:
    class recipe
    {
    public:
      bool operator<(const recipe &r2) const
      {
        if (ink < r2.ink)
        {
          return true;
        }
        else if (ink == r2.ink)
        {
          if (dust < r2.dust)
          {
            return true;
          }
          else if (dust == r2.dust)
          {
            if (pen < r2.pen)
            {
              return true;
            }
            else if (pen == r2.pen)
            {
              if (paper < r2.paper)
              {
                return true;
              }
            }
          }
        }

        return false;
      }
      vnum_t ink;
      vnum_t dust;
      vnum_t pen;
      vnum_t paper;
    };

    Scribe();
    ~Scribe();
    void load(void);
    void save(void);
    void list(CharacterPtr ch);
    qint32 add(CharacterPtr ch, QString argument);
    qint32 remove(CharacterPtr ch, QString argument);
    qint32 size(void);
    qint32 find(recipe);

  private:
    static QMap<recipe, qint32> recipes;
    class loadError
    {
    };
    static const QString RECIPES_FILENAME;
    static bool initialized;
  };

}
