#pragma once
#include <QString>
/************************************************************************
| $Id: fight.h,v 1.45 2015/06/16 04:10:54 pirahna Exp $
| fight.h
| This file defines the header information for fight.
*/

/* External prototype */
void debug_point(void);

/* Here are our function prototypes */

constexpr auto COMBAT_MOD_FRENZY = 1;
constexpr auto COMBAT_MOD_RESIST = 1 << 1;
constexpr auto COMBAT_MOD_SUSCEPT = 1 << 2;
constexpr auto COMBAT_MOD_IGNORE = 1 << 3;
constexpr auto COMBAT_MOD_REDUCED = 1 << 4;

// These are so that we only need one copy of one_hit and weapon_spells and
// skewer and behead

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
#define COMBAT_BASH1 1 << 1
#define COMBAT_BASH2 1 << 2
constexpr auto COMBAT_STUNNED = 1 << 3;
#define COMBAT_STUNNED2 1 << 4
constexpr auto COMBAT_CIRCLE = 1 << 5;
constexpr auto COMBAT_BERSERK = 1 << 6;
constexpr auto COMBAT_HITALL = 1 << 7;
#define COMBAT_RAGE1 1 << 8
#define COMBAT_RAGE2 1 << 9
#define COMBAT_BLADESHIELD1 1 << 10
#define COMBAT_BLADESHIELD2 1 << 11
constexpr auto COMBAT_REPELANCE = 1 << 12;
constexpr auto COMBAT_VITAL_STRIKE = 1 << 13;
constexpr auto COMBAT_MONK_STANCE = 1 << 14;
constexpr auto COMBAT_MISS_AN_ATTACK = 1 << 15;
#define COMBAT_ORC_BLOODLUST1 1 << 16
#define COMBAT_ORC_BLOODLUST2 1 << 17
constexpr auto COMBAT_THI_EYEGOUGE = 1 << 18;
#define COMBAT_THI_EYEGOUGE2 1 << 19
constexpr auto COMBAT_FLEEING = 1 << 20;
#define COMBAT_SHOCKED2 1 << 21
constexpr auto COMBAT_CRUSH_BLOW = 1 << 22;
constexpr auto COMBAT_ATTACKER = 1 << 23;
#define COMBAT_CRUSH_BLOW2 1 << 24

constexpr auto DAMAGE_TYPE_PHYSICAL = 0;
constexpr auto DAMAGE_TYPE_MAGIC = 1;
constexpr auto DAMAGE_TYPE_SONG = 2;

class threat_data
{
public:
  threat_data *next{};
  qint32 threat{};
  QString name_;
};
