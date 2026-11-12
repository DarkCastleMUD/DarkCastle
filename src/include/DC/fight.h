#pragma once
#include <qstring.h>
#include <stdint.h>

/************************************************************************
| $Id: fight.h,v 1.45 2015/06/16 04:10:54 pirahna Exp $
| fight.h
| This file defines the header information for fight.
*/
#include "DC/structs.h" // uint8_t, uint8_t, etc..
#include "DC/character.h"
#include "DC/obj.h"

class Character;
class follow_type;

/* External prototype */
void debug_point();

/* Here are our function prototypes */
int noncombat_damage(CharacterPtr ch, int dam, char *char_death_msg, char *room_death_msg, char *death_log_msg, int type);
int getRealSpellDamage(CharacterPtr ch);

#define FIRST WEAR_WIELD
#define SECOND WEAR_SECOND_WIELD

#define COMBAT_MOD_FRENZY 1
#define COMBAT_MOD_RESIST 1 << 1
#define COMBAT_MOD_SUSCEPT 1 << 2
#define COMBAT_MOD_IGNORE 1 << 3
#define COMBAT_MOD_REDUCED 1 << 4
void make_dust(CharacterPtr ch);
bool do_frostshield(CharacterPtr ch, CharacterPtr vict);
int speciality_bonus(CharacterPtr ch, int attacktype, int level);
void make_husk(CharacterPtr ch);
void make_heart(CharacterPtr ch, CharacterPtr vict);
void make_head(CharacterPtr ch);
void make_arm(CharacterPtr ch);
void make_leg(CharacterPtr ch);
void make_bowels(CharacterPtr ch);
void make_blood(CharacterPtr ch);
void make_scraps(CharacterPtr ch, ObjectPtr obj);
void room_mobs_only_hate(CharacterPtr ch);
int attack(CharacterPtr ch, CharacterPtr vict, int type, int attack = FIRST);
void group_gain(CharacterPtr ch, CharacterPtr vict);
int check_magic_block(CharacterPtr ch, CharacterPtr victim, int attacktype);
int check_riposte(CharacterPtr ch, CharacterPtr vict, int attacktype);
int check_shieldblock(CharacterPtr ch, CharacterPtr vict, int attacktype);
bool check_parry(CharacterPtr ch, CharacterPtr vict, int attacktype, bool display_results = true);
bool check_dodge(CharacterPtr ch, CharacterPtr vict, int attacktype, bool display_results = true);
void disarm(CharacterPtr ch, CharacterPtr vict);
void trip(CharacterPtr ch, CharacterPtr vict);
int checkCounterStrike(CharacterPtr, CharacterPtr);
int doTumblingCounterStrike(CharacterPtr, CharacterPtr);

int one_hit(CharacterPtr ch, CharacterPtr vict, int type, int weapon);
int do_skewer(CharacterPtr ch, CharacterPtr vict, int dam, int wt, int wt2, int weapon);
void do_combatmastery(CharacterPtr ch, CharacterPtr vict, int weapon);
int do_behead_skill(CharacterPtr ch, CharacterPtr victim);
int do_execute_skill(CharacterPtr, CharacterPtr, int);
int weapon_spells(CharacterPtr ch, CharacterPtr vict, int weapon);
void eq_damage(CharacterPtr ch, CharacterPtr vict, int dam, int weapon_type, int attacktype);
void fight_kill(CharacterPtr ch, CharacterPtr vict, int type, int spec_type);
int can_attack(CharacterPtr ch);
int can_be_attacked(CharacterPtr ch, CharacterPtr vict);
int second_attack(CharacterPtr ch);
int third_attack(CharacterPtr ch);
int fourth_attack(CharacterPtr ch);
int second_wield(CharacterPtr ch);
void set_cantquit(CharacterPtr, CharacterPtr, bool = false);
int is_pkill(CharacterPtr ch, CharacterPtr vict);
void raw_kill(CharacterPtr ch, CharacterPtr victim);
void do_pkill(CharacterPtr ch, CharacterPtr victim, int type, bool vict_is_attacker = false);
void arena_kill(CharacterPtr ch, CharacterPtr victim, int type);
void do_dead(CharacterPtr ch, CharacterPtr victim);
void eq_destroyed(CharacterPtr ch, ObjectPtr obj, int pos);
int is_stunned(CharacterPtr ch);
void update_flags(CharacterPtr vict);
void update_stuns(CharacterPtr ch);
void do_dam_msgs(CharacterPtr ch, CharacterPtr victim, int dam, int attacktype, int weapon, int filter = 0);
int act_poisonous(CharacterPtr ch);
int isHit(CharacterPtr ch, CharacterPtr victim, int attacktype, int &type, int &reduce);
void inform_victim(CharacterPtr ch, CharacterPtr victim, int dam);
CharacterPtr loop_followers(follow_type **f);
CharacterPtr get_highest_level_killer(CharacterPtr leader, CharacterPtr killer);
int32_t count_xp_eligibles(CharacterPtr leader, CharacterPtr killer, int32_t highest_level, int32_t *total_levels);
int64_t scale_char_xp(CharacterPtr ch, CharacterPtr killer, CharacterPtr victim, int32_t no_killers, int32_t total_levels, int32_t highest_level, int64_t base_xp, int64_t *bonus_xp);
void remove_active_potato(CharacterPtr vict);

// These are so that we only need one copy of one_hit and weapon_spells and
// skewer and behead

#define TYPE_CHOOSE 0
#define TYPE_PKILL 1
#define TYPE_RAW_KILL 2
#define TYPE_ARENA_KILL 3

#define KILL_OTHER 0
#define KILL_DROWN 1
#define KILL_FALL 2
#define KILL_POISON 3
#define KILL_SUICIDE 4
#define KILL_POTATO 5
#define KILL_MASHED 6
#define KILL_BINGO 7
#define KILL_BATTER 8
#define KILL_MORTAR 9

#define COMBAT_SHOCKED 1
#define COMBAT_BASH1 1 << 1
#define COMBAT_BASH2 1 << 2
#define COMBAT_STUNNED 1 << 3
#define COMBAT_STUNNED2 1 << 4
#define COMBAT_CIRCLE 1 << 5
#define COMBAT_BERSERK 1 << 6
#define COMBAT_HITALL 1 << 7
#define COMBAT_RAGE1 1 << 8
#define COMBAT_RAGE2 1 << 9
#define COMBAT_BLADESHIELD1 1 << 10
#define COMBAT_BLADESHIELD2 1 << 11
#define COMBAT_REPELANCE 1 << 12
#define COMBAT_VITAL_STRIKE 1 << 13
#define COMBAT_MONK_STANCE 1 << 14
#define COMBAT_MISS_AN_ATTACK 1 << 15
#define COMBAT_ORC_BLOODLUST1 1 << 16
#define COMBAT_ORC_BLOODLUST2 1 << 17
#define COMBAT_THI_EYEGOUGE 1 << 18
#define COMBAT_THI_EYEGOUGE2 1 << 19
#define COMBAT_FLEEING 1 << 20
#define COMBAT_SHOCKED2 1 << 21
#define COMBAT_CRUSH_BLOW 1 << 22
#define COMBAT_ATTACKER 1 << 23
#define COMBAT_CRUSH_BLOW2 1 << 24

#define DAMAGE_TYPE_PHYSICAL 0
#define DAMAGE_TYPE_MAGIC 1
#define DAMAGE_TYPE_SONG 2
