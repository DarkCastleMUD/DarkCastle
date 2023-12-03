/***************************************************************************
 *  file: utils.h, Utility module.                         Part of DIKUMUD *
 *  Usage: Utility macros                                                  *
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
 *  10/21/2003   Onager    Changed IS_ANONYMOUS() to handle mobs without   *
 *                         crashing                                        *
 ***************************************************************************/
/* $Id: utility.h,v 1.100 2014/07/27 00:20:02 jhhudso Exp $ */

#ifndef UTILITY_H_
#define UTILITY_H_

#include <time.h>
#include <stdlib.h>
#include <stdint.h>

#include <string>
#include <vector>
#include <queue>

#include "common.h"
#include "DC.h"
#include "structs.h"
#include "weather.h"
#include "memory.h"
#include "player.h"
#include "character.h"
#include "Trace.h"

enum LogChannels
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
   LOG_WARNINGS = 1U << 18,
   LOG_HELP = 1U << 19,
   LOG_DATABASE = 1U << 20,
   LOG_OBJECTS = 1U << 21,
   CHANNEL_TELL = 1U << 22,
   CHANNEL_HINTS = 1U << 23,
   LOG_VAULT = 1U << 24,
   LOG_QUEST = 1U << 25,
   LOG_DEBUG = 1U << 26
};

extern struct weather_data weather_info;

void check_timer();

static const int COREDUMP_MAX = 10;

#ifdef WIN32
inline int random()
{
   return (rand());
}
char *index(char *buf, char op);
#endif

#define MOB_WAIT_STATE(ch) ((ch)->deaths)

#define GET_WAIT(ch) (IS_MOB((ch)) ? (ch)->deaths : ((ch)->desc ? (ch)->desc->wait : 0))

#define WAIT_STATE(czh, cycle) (((czh)->desc) ? (czh)->desc->wait > (cycle) ? 0 : (czh)->desc->wait = (cycle) : (IS_MOB((czh)) ? MOB_WAIT_STATE((czh)) = (cycle) : 0))

#define REM_WAIT_STATE(czh, cycle) (((czh)->desc) ? (czh)->desc->wait < (cycle) ? (czh)->desc->wait = 0 : (czh)->desc->wait -= (cycle) : IS_MOB((czh)) ? MOB_WAIT_STATE((czh)) < (cycle) ? MOB_WAIT_STATE((czh)) = 0 : MOB_WAIT_STATE((czh)) -= (cycle) \
                                                                                                                                                       : 0)

// Defines for gradual skill increase code
// Usage is defined in guild.cpp

#define SKILL_INCREASE_EASY 100
#define SKILL_INCREASE_MEDIUM 200
#define SKILL_INCREASE_HARD 300
void check_timer();

void skill_increase_check(Character *ch, int skill, int learned, int difficulty);
bool is_hiding(Character *ch, Character *vict);

// End defines for gradual skill increase code

#define SILENCE_OBJ_NUMBER 407
#define SPIRIT_SHIELD_OBJ_NUMBER 408
#define CONSECRATE_OBJ_NUMBER 409
#define CONSECRATE_COMP_OBJ_NUMBER 3094
#define DESECRATE_COMP_OBJ_NUMBER 303

#define REMOVE_FROM_LIST(item, head, next)   \
   if ((item) == (head))                     \
      head = (item)->next;                   \
   else                                      \
   {                                         \
      temp = head;                           \
      while (temp && (temp->next != (item))) \
         temp = temp->next;                  \
      if (temp)                              \
         temp->next = (item)->next;          \
   }

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define LOWER(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + ('a' - 'A')) : (c))
#define UPPER(c) (((c) >= 'a' && (c) <= 'z') ? ((c) + ('A' - 'a')) : (c))

// #define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r' || (ch) == '|')
#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') // replaced to leave off the pipe and put it eclusively in comm.c
                                                  // where we could check to see if we were in an editor first.

#define CAP(st) (*(st) = UPPER(*(st)), st)

#ifdef LEAK_CHECK
#define CREATE(result, type, number)                            \
   do                                                           \
   {                                                            \
      if (!((result) = (type *)calloc((number), sizeof(type)))) \
      {                                                         \
         perror("calloc failure in CREATE: ");                  \
         abort();                                               \
      }                                                         \
   } while (0)
#else
#define CREATE(result, type, number)                              \
   do                                                             \
   {                                                              \
      if (!((result) = (type *)dc_alloc((number), sizeof(type)))) \
      {                                                           \
         perror("calloc failure in CREATE: ");                    \
         abort();                                                 \
      }                                                           \
   } while (0)
#endif

#define RECREATE(result, type, number)                                         \
   do                                                                          \
   {                                                                           \
      if (!((result) = (type *)dc_realloc((result), sizeof(type) * (number)))) \
      {                                                                        \
         perror("realloc failure in RECREATE");                                \
         abort();                                                              \
      }                                                                        \
   } while (0)

#define FREE(p)           \
   do                     \
   {                      \
      if ((p) != nullptr) \
      {                   \
         dc_free((p));    \
         (p) = 0;         \
      }                   \
   } while (0)

#define ASIZE 32 // don't change unless you want to be screwed
#define SETBIT(var, bit) ((var)[(bit) / ASIZE] |= (1 << (((bit) - (((bit) / ASIZE) * ASIZE) - 1))))
// setting with an OR
#define REMBIT(var, bit) ((var)[(bit) / ASIZE] &= ~(1 << (((bit) - (((bit) / ASIZE) * ASIZE)) - 1)))
// setting with an AND
#define TOGBIT(var, bit) ((var)[(bit) / ASIZE] ^= (1 << (((bit) - (((bit) / ASIZE) * ASIZE)) - 1)))
// setting with an XOR
#define ISSET(var, bit) ((var)[(bit) / ASIZE] & (1 << (((bit) - (((bit) / ASIZE) * ASIZE)) - 1)))
// using an AND

#define SET_BIT(var, bit) ((var) = (var) | (bit))
#define REMOVE_BIT(var, bit) ((var) = (var) & ~(bit))
#define TOGGLE_BIT(var, bit) ((var) = (var) ^ (bit))

#define IS_AFFECTED(ch, skill) (ISSET((ch)->affected_by, (skill)))

int DARK_AMOUNT(int room);
bool IS_DARK(int room);
#define IS_LIGHT(room) (!IS_DARK(room))

#define IS_ARENA(room) (DC::isSet(DC::getInstance()->world[room].room_flags, ARENA))

#define HSHR(ch) ((ch)->sex ? (((ch)->sex == 1) ? "his" : "her") : "its")
#define HSSH(ch) ((ch)->sex ? (((ch)->sex == 1) ? "he" : "she") : "it")
#define HMHR(ch) ((ch)->sex ? (((ch)->sex == 1) ? "him" : "her") : "it")
// #define ANA(obj) (index("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
// #define SANA(obj) (index("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")

#define IS_PC(ch) (!IS_NPC(ch) && ch->player != nullptr)
#define IS_NPC(ch) (DC::isSet((ch)->misc, MISC_IS_MOB))
#define IS_MOB(ch) (IS_NPC(ch))
#define IS_OBJ(ch) (DC::isSet((ch)->misc, MISC_IS_OBJ))
#define IS_FAMILIAR(ch) (IS_AFFECTED(ch, AFF_FAMILIAR))

#define IS_MINLEVEL_PC(ch, level) (ch->getLevel() >= level && IS_PC(ch))
#define IS_MAXLEVEL_PC(ch, level) (ch->getLevel() <= level && IS_PC(ch))
#define IS_MINLEVEL_NPC(ch, level) (ch->getLevel() >= level && IS_NPC(ch))
#define IS_IMMORTAL(ch) (IS_MINLEVEL_PC(ch, IMMORTAL))
#define IS_MORTAL(ch) (IS_MAXLEVEL_PC(ch, IMMORTAL - 1))

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
#define GET_NAME(ch) ((ch)->getNameC())
#define GET_SHORT(ch) ((ch)->short_desc ? (ch)->short_desc : (ch)->getNameC())
#define GET_SHORT_ONLY(ch) ((ch)->short_desc)
#define GET_TITLE(ch) ((ch)->title)

#define GET_ZONE(ch) (DC::getInstance()->world[(ch)->in_room].zone)

#define GET_OBJ_SHORT(obj) ((obj)->short_description)
#define GET_OBJ_NAME(obj) ((obj)->name)

#define GET_OBJ_RNUM(obj) ((obj)->item_number)
#define GET_OBJ_VAL(obj, val) ((obj)->obj_flags.value[(val)])
#define GET_OBJ_VROOM(obj) ((obj)->vroom)
#define GET_OBJ_EXTRA(obj) ((obj)->obj_flags.extra_flags)
#define GET_OBJ_TIMER(obj) ((obj)->obj_flags.timer)
#define GET_OBJ_TYPE(obj) ((obj)->obj_flags.type_flag)
#define GET_OBJ_WEAR(obj) ((obj)->obj_flags.wear_flags)
#define GET_OBJ_COST(obj) ((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj) ((obj)->obj_flags.cost_per_day)
#define GET_OBJ_VNUM(obj) (GET_OBJ_RNUM(obj) >= 0 ? obj_index[GET_OBJ_RNUM(obj)].virt : -1)
#define VALID_ROOM_RNUM(rnum) ((rnum) != DC::NOWHERE && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
   ((int32_t)(VALID_ROOM_RNUM(rnum) ? DC::getInstance()->world[(rnum)].number : DC::NOWHERE))

#define GET_PROMPT(ch) ((ch)->player->prompt)
#define GET_LAST_PROMPT(ch) ((ch)->player->last_prompt)
#define GET_TOGGLES(ch) ((ch)->player->toggles)

#define GET_CLASS(ch) ((ch)->c_class)
#define GET_HOME(ch) ((ch)->hometown)
#define GET_AGE(ch) (ch->age().year)

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
#define GET_EXP(ch) ((ch)->exp)
#define GET_HEIGHT(ch) ((ch)->height)
#define GET_WEIGHT(ch) ((ch)->weight)
#define GET_SEX(ch) ((ch)->sex)
#define GET_HITROLL(ch) ((ch)->hitroll)
#define GET_REAL_HITROLL(ch) ((ch)->hitroll + dex_app[GET_DEX((ch))].tohit)
#define GET_DAMROLL(ch) ((ch)->damroll)
#define GET_REAL_DAMROLL(ch) ((ch)->damroll + str_app[GET_STR((ch))].todam)
#define GET_QPOINTS(ch) ((ch)->player->quest_points)
#define GET_SPELLDAMAGE(ch) ((ch)->spelldamage)

#define GET_RACE(ch) ((ch)->race)
#define GET_BITV(ch) ((ch)->race == 1 ? 1 : (1 << (((ch)->race) - 1)))
#define IS_UNDEAD(ch) ((GET_RACE(ch) == RACE_UNDEAD) || (GET_RACE(ch) == RACE_GHOST))

#define AWAKE(ch) (GET_POS(ch) != position_t::SLEEPING)

#define IS_ANONYMOUS(ch) (IS_MOB(ch) ? 1 : ((ch->getLevel() >= 101) ? 0 : DC::isSet((ch)->player->toggles, Player::PLR_ANONYMOUS)))
/*
inline const short IS_ANONYMOUS(Character *ch)
{
  if (IS_MOB(ch))
     // this should really never be called on mobs
     return 1;
  else if (ch->getLevel() >= 101)
     return 0;
  else
     return (DC::isSet(ch->player->toggles, Player::PLR_ANONYMOUS) != 0);
}
*/
/* Object And Carry related macros */

#define GET_ITEM_TYPE(obj) ((obj)->obj_flags.type_flag)
#define GET_MOB_TYPE(mob) ((mob)->mobdata->mob_flags.type)
#define GET_OBJ_WEIGHT(obj) ((obj)->obj_flags.weight)

#define CAN_WEAR(obj, part) (DC::isSet((obj)->obj_flags.wear_flags, part))

#define CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w + ch->has_skill(SKILL_VIGOR))
#define CAN_CARRY_N(ch) (5 + GET_DEX(ch))
#define IS_CARRYING_W(ch) ((ch)->carry_weight)
#define IS_CARRYING_N(ch) ((ch)->carry_items)

#define CAN_CARRY_OBJ(ch, obj)                                        \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) && \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))
#define CAN_GET_OBJ(ch, obj)                                    \
   (CAN_WEAR((obj), ITEM_TAKE) && CAN_CARRY_OBJ((ch), (obj)) && \
    CAN_SEE_OBJ((ch), (obj)))

#define IS_OBJ_STAT(obj, stat) (DC::isSet((obj)->obj_flags.extra_flags, stat))
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

/* char name/short_desc(for mobs) or someone?  */

#define PERS(ch, vict) ( \
    ch->getLevel() > MIN_GOD ? (CAN_SEE(vict, ch) ? GET_SHORT(ch) : "an immortal presence") : (CAN_SEE(vict, ch) ? GET_SHORT(ch) : "someone"))

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? (obj)->short_description : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? fname((obj)->name) : "something")

#define IS_EXIT(room, door) (DC::getInstance()->world[(room)].dir_option[(door)])
#define EXIT_TO(room, door) (DC::getInstance()->world[(room)].dir_option[(door)]->to_room)
#define IS_OPEN(room, door) (!DC::isSet(DC::getInstance()->world[(room)].dir_option[(door)]->exit_info, EX_CLOSED))

#define OUTSIDE(ch) (!DC::isSet(DC::getInstance()->world[(ch)->in_room].room_flags, INDOORS))
#define EXIT(ch, door) (DC::getInstance()->world[(ch)->in_room].dir_option[door])
#define CAN_GO(ch, door) (EXIT(ch, door) && (EXIT(ch, door)->to_room != DC::NOWHERE) && (EXIT(ch, door)->to_room != DC::NOWHERE) && !DC::isSet(EXIT(ch, door)->exit_info, EX_CLOSED))

#define GET_ALIGNMENT(ch) ((ch)->alignment)
#define IS_GOOD(ch) (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch) (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_SINGING(ch) (!((ch)->songs.empty()))

enum MatchType
{
   Failure,
   Subset,
   Exact
};

char *str_hsh(const char *);
bool ishashed(char *);
void double_dollars(char *destination, char *source);
std::string double_dollars(std::string source);

void clan_death(char *b, Character *ch);

int move_char(Character *ch, int dest, bool stop_all_fighting = true);
// int number(int from, int to);

int dice(int number, int size);
int str_cmp(const char *arg1, const char *arg2);
int str_nosp_cmp(const char *arg1, const char *arg2);
int str_nosp_cmp(QString arg1, QString arg2);
int str_n_nosp_cmp(const char *arg1, const char *arg2, int size);
MatchType str_n_nosp_cmp_begin(std::string arg1, std::string arg2);
char *str_nospace(const char *stri);
char *str_dup(const char *str);
char *str_dup0(const char *str);
void logentry(QString str, uint64_t god_level = 0, LogChannels type = LogChannels::LOG_MISC, Character *vict = nullptr);
void logf(int level, LogChannels type, const char *arg, ...);
void logf(int level, LogChannels type, QString arg);
int send_to_gods(QString message, uint64_t god_level, LogChannels type);

void sprintbit(uint value[], const char *names[], char *result);
std::string sprintbit(uint value[], const char *names[]);

void sprintbit(uint32_t vektor, const char *names[], char *result);
std::string sprintbit(uint32_t vektor, const char *names[]);

void sprintbit(uint32_t vektor, QStringList names, char *result);
QString sprintbit(uint32_t vektor, QStringList names);

// void sprinttype(quint64 type, const char *names[], char *result);
template <typename T>
void sprinttype(T type, const char *names[], char *result)
{
   if (!result)
   {
      return;
   }

   qsizetype nr{};
   for (; *names[nr] != '\n'; nr++)
   {
      ;
   }

   if (type > -1 && type < nr)
   {
      strcpy(result, names[type]);
   }
   else
   {
      strcpy(result, "Undefined");
   }
}

std::string sprinttype(int type, const char *names[]);

void sprinttype(int type, std::vector<const char *>, char *result);
void sprinttype(int type, QStringList, char *result);
QString sprinttype(uint64_t type, QStringList names);

// void sprinttype(uint64_t type, QStringList names, char *result);
template <typename T>
void sprinttype(T type, QStringList names, char *result)
{
   if (result == nullptr)
   {
      return;
   }
   strcpy(result, names.value(static_cast<qsizetype>(type), "Undefined").toStdString().c_str());
}

std::string sprinttype(int type, std::vector<const char *>);

int consttype(char *search_str, const char *names[]);
QString constindex(const qsizetype index, const QStringList names);
struct time_info_data mud_time_passed(time_t t2, time_t t1);
bool circle_follow(Character *ch, Character *victim);
bool ARE_GROUPED(Character *sub, Character *obj);
bool ARE_CLANNED(Character *sub, Character *obj);
// bool is_number(const char *str);
bool is_number(QString str);
void gain_condition(Character *ch, int condition, int value);
void set_fighting(Character *ch, Character *vict);
void stop_fighting(Character *ch, int clearlag = 1);
int do_simple_move(Character *ch, int cmd, int following);
// int	attempt_move	(Character *ch, int cmd, int is_retreat = 0);
int32_t mana_limit(Character *ch);
int32_t ki_limit(Character *ch);
int32_t hit_limit(Character *ch);
typedef int16_t skill_t;
const char *get_skill_name(int skillnum);
void gain_exp_regardless(Character *ch, int gain);
void advance_level(Character *ch, int is_conversion);
int close_socket(class Connection *d);
int isname(std::string arg, std::string namelist);
int isname(std::string arg, const char *namelist);
int isname(QString arg, const char *namelist);
int isname(QString arg, QString namelist);
int isname(QString arg, QStringList namelist);
int isname(const char *arg, const char *namelist);
int isname(const char *arg, std::string namelist);
int isname(const char *arg, joining_t &namelist);
void page_string(class Connection *d, const char *str,
                 int keep_internal);
void gain_exp(Character *ch, int64_t gain);
void redo_hitpoints(Character *ch); /* Rua's put in  */
void redo_mana(Character *ch);      /* Rua's put in  */
void redo_ki(Character *ch);        /* And Urizen*/
void assign_rooms(void);
void assign_objects(void);
void free_obj(class Object *obj);

int char_from_room(Character *ch, bool stop_fighting);
int char_from_room(Character *ch);
void do_start(Character *ch);

void update_pos(Character *victim);
void clear_object(class Object *obj);
void death_cry(Character *ch);
std::vector<std::string> splitstring(std::string splitme, std::string delims, bool ignore_empty = false);
std::string joinstring(std::vector<std::string> joinme, std::string delims, bool ignore_empty = false);

void add_follower(Character *ch, Character *leader, int cmd);
void send_to_outdoor(char *messg);
void send_to_zone(char *messg, int zone);
void weather_and_time(int mode);
void night_watchman(void);
int special(Character *ch, int cmd, char *arg);
int process_output(class Connection *t);
int file_to_string(const char *name, char *buf);
bool load_char_obj(class Connection *d, QString name);
void save_char_obj(Character *ch);
bool load_char_obj(class Connection *d, QString name);

#ifdef USE_SQL
void save_char_obj_db(Character *ch);
#endif

void unique_scan(Character *victim);
void char_to_store(Character *ch, struct char_file_u4 *st, struct time_data &tmpage);
bool obj_to_store(class Object *obj, Character *ch, FILE *fpsave, int wear_pos);
void check_idling(Character *ch);
void stop_follower(Character *ch, int cmd);
bool CAN_SEE(Character *sub, Character *obj, bool noprog = false);
int SWAP_CH_VICT(int value);
bool SOMEONE_DIED(int value);
bool CAN_SEE_OBJ(Character *sub, class Object *obj, bool bf = false);
bool check_blind(Character *ch);
void raw_kill(Character *ch, Character *victim);
void check_killer(Character *ch, Character *victim);
int map_eq_level(Character *mob);
void disarm(Character *ch, Character *victim);
int shop_keeper(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker);
void send_to_all(QString messg);
void ansi_color(char *txt, Character *ch);
void send_to_char(QString messg, Character *ch);
void send_to_char(std::string messg, Character *ch);
void send_to_char(const char *messg, Character *ch);
void send_to_char_nosp(const char *messg, Character *ch);
void send_to_char_nosp(QString messg, Character *ch);
void send_to_room(QString messg, int room, bool awakeonly = false, Character *nta = nullptr);
void record_track_data(Character *ch, int cmd);
int write_to_descriptor(int desc, std::string txt);
int write_to_descriptor_fd(int desc, char *txt);
void write_to_q(const std::string txt, std::queue<std::string> &queue);
int use_mana(Character *ch, int sn);
void automail(char *name);
bool file_exists(const char *);
void util_archive(const char *, Character *);
void util_unarchive(char *, Character *);
int is_busy(Character *ch);
int is_ignoring(const Character *const ch, const Character *const i);
void colorCharSend(char *s, Character *ch);
void send_to_char_regardless(QString messg, Character *ch);
void send_to_char_regardless(std::string messg, Character *ch);
int csendf(Character *ch, const char *arg, ...);
bool check_range_valid_and_convert(uint64_t &value, QString buf, uint64_t begin, uint64_t end);
bool check_range_valid_and_convert(int64_t &value, QString buf, int64_t begin, int64_t end);
bool check_range_valid_and_convert(uint32_t &value, QString buf, uint32_t begin, uint32_t end);
bool check_range_valid_and_convert(int32_t &value, QString buf, int32_t begin, int32_t end);
/*
TODO
bool check_range_valid_and_convert(uint16_t &value, QString buf, uint16_t begin, uint16_t end);
bool check_range_valid_and_convert(int16_t &value, QString buf, int begin, int end);
bool check_range_valid_and_convert(uint8_t &value, QString buf, int begin, int end);
bool check_range_valid_and_convert(int8_t &value, QString buf, int begin, int end);
*/

bool check_valid_and_convert(int &value, char *buf);
void parse_bitstrings_into_int(const char *bits[], const char *strings, Character *ch, uint32_t value[]);
void parse_bitstrings_into_int(const char *bits[], const char *strings, Character *ch, uint32_t &value);
void parse_bitstrings_into_int(QStringList bits, QString strings, Character *ch, uint32_t &value);
void parse_bitstrings_into_int(const char *bits[], const char *strings, Character *ch, uint16_t &value);
void parse_bitstrings_into_int(const char *bits[], std::string strings, Character *ch, uint32_t value[]);
void parse_bitstrings_into_int(const char *bits[], std::string strings, Character *ch, uint32_t &value);
void parse_bitstrings_into_int(const char *bits[], std::string strings, Character *ch, uint16_t &value);
int contains_no_trade_item(Object *obj);
int contents_cause_unique_problem(Object *obj, Character *vict);
bool check_make_camp(int);
int get_leadership_bonus(Character *);
void update_make_camp_and_leadership(void);
int _parse_name(const char *arg, char *name);

void mob_suprised_sayings(Character *ch, Character *aggressor);

// MOBProgs prototypes
int mprog_wordlist_check(const char *arg, Character *mob,
                         Character *actor, Object *object,
                         void *vo, int type, bool reverse = false);
void mprog_percent_check(Character *mob, Character *actor,
                         Object *object, void *vo,
                         int type);
int mprog_act_trigger(std::string buf, Character *mob,
                      Character *ch, Object *obj,
                      void *vo);
int mprog_bribe_trigger(Character *mob, Character *ch,
                        int amount);
int mprog_entry_trigger(Character *mob);
int mprog_give_trigger(Character *mob, Character *ch,
                       Object *obj);
int mprog_greet_trigger(Character *mob);
int mprog_fight_trigger(Character *mob, Character *ch);
int mprog_hitprcnt_trigger(Character *mob, Character *ch);
int mprog_death_trigger(Character *mob, Character *killer);
int mprog_random_trigger(Character *mob);
int mprog_arandom_trigger(Character *mob);
int mprog_speech_trigger(const char *txt, Character *mob);
int mprog_catch_trigger(Character *mob, int catch_num, char *var, int opt, Character *actor, Object *obj, void *vo, Character *rndm);
int mprog_attack_trigger(Character *mob, Character *ch);
int mprog_load_trigger(Character *mob);
int mprog_can_see_trigger(Character *ch, Character *mob);
int mprog_damage_trigger(Character *mob, Character *ch, int amount);

int oprog_catch_trigger(Object *obj, int catch_num, char *var, int opt, Character *actor, Object *obj2, void *vo, Character *rndm);
int oprog_act_trigger(const char *txt, Character *ch);
int oprog_speech_trigger(const char *txt, Character *ch);
int oprog_command_trigger(const char *txt, Character *ch, char *arg);
int oprog_weapon_trigger(Character *ch, Object *item);
int oprog_armour_trigger(Character *ch, Object *item);
int oprog_rand_trigger(Object *item);
int oprog_arand_trigger(Object *item);
int oprog_greet_trigger(Character *ch);
int oprog_load_trigger(Object *item);
int oprog_can_see_trigger(Character *ch, Object *item);
bool is_in_game(Character *ch);
int get_stat(Character *ch, attribute_t stat);
char *pluralize(int qty, char ending[] = "s");
size_t nocolor_strlen(const char *s);
size_t nocolor_strlen(QString str);
void make_prompt(class Connection *d, std::string &prompt);
std::string remove_all_codes(std::string input);
void prog_error(Character *mob, char *format, ...);
bool str_prefix(const char *astr, const char *bstr);
bool str_infix(const char *astr, const char *bstr);

extern const char menu[];

#define MAX_THROW_NAME 60
#define MPROG_CATCH_MIN 1
#define MPROG_CATCH_MAX 100

struct mprog_throw_type
{
   int target_mob_num;                   // num of mob to recieve
   char target_mob_name[MAX_THROW_NAME]; // std::string used to find target name

   int data_num; // number of catch call to activate on target
   int delay;    // how int32_t until the mob gets it

   int pitcher; // vnum of mob that threw the call
   int opt;
   mprog_throw_type *next;
   bool mob;  // Mob or object.
   char *var; // temporary variable
   Character *actor;
   Object *obj;
   void *vo;
   Character *rndm; // $r

   // new mppause crap below..
   Character *tMob;   // it should NOT throw it to another similar mob :P
   int ifchecks[256]; // Let's hope noone nests more ifs than that.
   int startPos;
   int cPos;
   char *orig;
   // end mppause crap
};

struct mprog_variable_data
{
   char *invoker;
   char *object;
   char *rndm;
   char *voi;
   int nested; // amount of nested ifs, at time of pause
   char *program;
};

int handle_poisoned_weapon_attack(Character *ch, Character *vict, int percent);

enum BACKUP_TYPE
{
   NONE,
   SELFDELETED,
   CONDEATH,
   ZAPPED
};
void remove_character(QString name, BACKUP_TYPE backup = NONE);
void remove_familiars(QString name, BACKUP_TYPE backup = NONE);

std::string replaceString(std::string message, std::string find, std::string replace);

char *numToStringTH(int);
bool champion_can_go(int room);
bool class_can_go(int ch_class, int room);

const char *find_profession(int c_class, uint8_t profession);

std::string get_isr_string(uint32_t, int8_t);

void produce_coredump(void *ptr = 0);

bool isDead(Character *ch);
bool isNowhere(Character *ch);
bool file_exists(std::string filename);
bool file_exists(QString filename);
bool char_file_exists(std::string name);
void show_obj_class_size_mini(Object *obj, Character *ch);
const char *item_condition(class Object *obj);
int random_percent_change(uint percentage, int value);
int random_percent_change(int from, int to, int value);
bool identify(Character *ch, Object *obj);
extern void end_oproc(Character *ch, Trace trace = Trace("unknown"));
void undo_race_saves(Character *ch);
QByteArray handle_ansi(QByteArray, Character *ch);
QString handle_ansi(QString, Character *ch);
std::string handle_ansi(std::string s, Character *ch);
char *handle_ansi_(char *s, Character *ch);
void blackjack_prompt(Character *ch, std::string &prompt, bool ascii);
void show_string(class Connection *d, const char *input);
void special_log(char *arg);

template <typename T>
T number(T from, T to)
{
   if (from == to)
   {
      return to;
   }

   if (from > to)
   {

      logentry(QString("BACKWARDS usage: number(%1, %2)!").arg(from).arg(to));
      produce_coredump();
      return to;
   }

   if (std::is_unsigned<T>::value)
   {
      return QRandomGenerator::global()->bounded(static_cast<quint64>(from), static_cast<quint64>(to + 1));
   }
   else if (std::is_signed<T>::value)
   {
      return QRandomGenerator::global()->bounded(static_cast<qint64>(from), static_cast<qint64>(to + 1));
   }
}

int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6);

#endif /* UTILITY_H_ */
