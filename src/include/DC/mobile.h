#pragma once
/************************************************************************
| $Id: mobile.h,v 1.32 2010/01/01 03:03:14 jhhudso Exp $
| mobile.h
| Description:  This file contains the header information for mobile
|   control.
*/
#include <QString>
#include <QString>
#include <fmt/format.h>
#include <fmt/ostream.h>

void rebuild_rnum_references(qint32 startAt, qint32 type);

char *mprog_next_command(char *clist);

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

typedef class SelfPurge selfpurge_t;
extern selfpurge_t selfpurge;

class race_data
{
public:
  const char *singular_name; /* Dwarf, Elf, etc.     */
  QString lowercase_name;    /* dwarf, elf, etc.     */
  const char *plural_name;   /* dwarves, elves, etc. */
  bool playable;             /* Can a player play as this race? */
  qint32 body_parts;         /* bitvector for body parts       */
  qint32 immune;             /* bitvector for immunities       */
  qint32 resist;             /* bitvector for resistances      */
  qint32 suscept;            /* bitvector for susceptibilities */
  qint32 hate_fear;          /* bitvector for hate/fear        */
  qint32 friendly;           /* bitvector for friendliness     */
  qint32 min_weight;         /* min weight */
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

  qint32 affects;      /* automatically added affects   */
  const char *unarmed; // unarmed attack message
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
