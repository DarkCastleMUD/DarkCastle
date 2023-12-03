#ifndef MOBILE_H_
#define MOBILE_H_
/************************************************************************
| $Id: mobile.h,v 1.32 2010/01/01 03:03:14 jhhudso Exp $
| mobile.h
| Description:  This file contains the header information for mobile
|   control.
*/

#include <string>
#include <ostream>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "character.h"

void rebuild_rnum_references(int startAt, int type);
void mprog_driver(char *com_list, Character *mob,
                  Character *actor, class Object *obj,
                  void *vo, struct mprog_throw_type *thrw, Character *rndm);
char *mprog_next_command(char *clist);

bool charExists(Character *ch);

char *getTemp(Character *ch, char *name);

#define BASE_STAT 0

// #define NOTHING      0
#define ACT_SPEC 1
#define ACT_SENTINEL 2
#define ACT_SCAVENGER 3
#define ACT_NOTRACK 4
#define ACT_NICE_THIEF 5
#define ACT_AGGRESSIVE 6
#define ACT_STAY_ZONE 7
#define ACT_WIMPY 8
/* aggressive only attack sleeping players */
#define ACT_2ND_ATTACK 9
#define ACT_3RD_ATTACK 10
#define ACT_4TH_ATTACK 11
/* Each attack bit must be set to get up */
/* 4 attacks                             */
/*
 * For ACT_AGGRESSIVE_XXX, you must also set ACT_AGGRESSIVE
 * These switches can be combined, if none are selected, then
 * the mobile will attack any alignment (same as if all 3 were set)
 */
#define ACT_AGGR_EVIL 12
#define ACT_AGGR_GOOD 13
#define ACT_AGGR_NEUT 14
#define ACT_UNDEAD 15
#define ACT_STUPID 16
#define ACT_CHARM 17
#define ACT_HUGE 18
#define ACT_DODGE 19
#define ACT_PARRY 20
#define ACT_RACIST 21
#define ACT_FRIENDLY 22
#define ACT_STAY_NO_TOWN 23
#define ACT_NOMAGIC 24
#define ACT_DRAINY 25
#define ACT_BARDCHARM 26
#define ACT_NOKI 27
#define ACT_NOMATRIX 28
#define ACT_BOSS 29
#define ACT_NOHEADBUTT 30
#define ACT_NOATTACK 31
// #define CHECKTHISACT      32 //Do not change unless ASIZE changes
#define ACT_SWARM 33
#define ACT_TINY 34
#define ACT_NODISPEL 35
#define ACT_POISONOUS 36
#define ACT_NO_GOLD_BONUS 37
#define ACT_NO_HUNT 38
#define ACT_MAX 38
// #define CHECKTHISACT      64 //Do not chance unless ASIZE changes

class SelfPurge
{
public:
  SelfPurge(void);
  SelfPurge(bool);
  void setOwner(Character *, std::string);
  explicit operator bool(void) const;
  std::string getFunction(void) const;
  bool getState(void) const;
  Character *getOwner(void) const { return owner; }

private:
  bool state = {};
  Character *owner = {};
  std::string function = {};
};

template <>
struct fmt::formatter<SelfPurge>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(SelfPurge const &sp, FormatContext &ctx)
  {
    return fmt::format_to(ctx.out(), "selfpurge {}:{}", sp.getState(), sp.getFunction());
  }
};

typedef class SelfPurge selfpurge_t;
extern selfpurge_t selfpurge;

struct race_data
{
  char *singular_name;        /* Dwarf, Elf, etc.     */
  std::string lowercase_name; /* dwarf, elf, etc.     */
  char *plural_name;          /* dwarves, elves, etc. */
  bool playable;              /* Can a player play as this race? */
  int32_t body_parts;         /* bitvector for body parts       */
  int32_t immune;             /* bitvector for immunities       */
  int32_t resist;             /* bitvector for resistances      */
  int32_t suscept;            /* bitvector for susceptibilities */
  int32_t hate_fear;          /* bitvector for hate/fear        */
  int32_t friendly;           /* bitvector for friendliness     */
  int min_weight;             /* min weight */
  int max_weight;

  int min_height;
  int max_height;

  unsigned min_str;
  unsigned max_str;
  int mod_str;

  unsigned min_dex;
  unsigned max_dex;
  int mod_dex;

  unsigned min_con;
  unsigned max_con;
  int mod_con;

  unsigned min_int;
  unsigned max_int;
  int mod_int;

  unsigned min_wis;
  unsigned max_wis;
  int mod_wis;

  int affects;         /* automatically added affects   */
  const char *unarmed; // unarmed attack message
};

struct mob_matrix_data
{
  int64_t experience;
  int hitpoints;
  int tohit;
  int todam;
  int armor;
  int gold;
};

void translate_value(char *leftptr, char *rightptr, int16_t **vali, uint32_t **valui,
                     char ***valstr, int64_t **vali64, uint64_t **valui64, int8_t **valb, Character *mob, Character *actor,
                     Object *obj, void *vo, Character *rndm);

void save_golem_data(Character *ch);
void save_charmie_data(Character *ch);

#endif
