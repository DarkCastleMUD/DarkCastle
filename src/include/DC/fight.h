#ifndef FIGHT_H_
#define FIGHT_H_

/************************************************************************
| $Id: fight.h,v 1.45 2015/06/16 04:10:54 pirahna Exp $
| fight.h
| This file defines the header information for fight.
*/
#include "DC/structs.h" // uint8_t, uint8_t, etc..
#include "DC/character.h"
#include "DC/utility.h" // false

/* External prototype */
void debug_point();

/* Here are our function prototypes */
int damage(Character *ch, Character *victim, int dam,
           int weapon_type, int attacktype, int weapon, bool is_death_prog = false);
int noncombat_damage(Character *ch, int dam, char *char_death_msg,
                     char *room_death_msg, char *death_log_msg, int type);
void send_damage(char const *, Character *, Object *, Character *, char const *, char const *, int);
void send_damage(QString buf, Character *, Object *, Character *, QString dmg, QString buf2, int);

int getRealSpellDamage(Character *ch);

#define FIRST WIELD
#define SECOND SECOND_WIELD

#define COMBAT_MOD_FRENZY 1
#define COMBAT_MOD_RESIST 1 << 1
#define COMBAT_MOD_SUSCEPT 1 << 2
#define COMBAT_MOD_IGNORE 1 << 3
#define COMBAT_MOD_REDUCED 1 << 4
void make_dust(Character *ch);
bool do_frostshield(Character *ch, Character *vict);
int speciality_bonus(Character *ch, int attacktype, int level);
void make_husk(Character *ch);
void make_heart(Character *ch, Character *vict);
void make_head(Character *ch);
void make_arm(Character *ch);
void make_leg(Character *ch);
void make_bowels(Character *ch);
void make_blood(Character *ch);
void make_scraps(Character *ch, class Object *obj);
void room_mobs_only_hate(Character *ch);
int attack(Character *ch, Character *vict, int type, int attack = FIRST);
void perform_violence(void);
void dam_message(int dam, Character *ch, Character *vict, int w_type, int32_t modifier);
void group_gain(Character *ch, Character *vict);
int check_magic_block(Character *ch, Character *victim, int attacktype);
int check_riposte(Character *ch, Character *vict, int attacktype);
int check_shieldblock(Character *ch, Character *vict, int attacktype);
bool check_parry(Character *ch, Character *vict, int attacktype, bool display_results = true);
bool check_dodge(Character *ch, Character *vict, int attacktype, bool display_results = true);
void disarm(Character *ch, Character *vict);
void trip(Character *ch, Character *vict);
int checkCounterStrike(Character *, Character *);
int doTumblingCounterStrike(Character *, Character *);

int one_hit(Character *ch, Character *vict, int type, int weapon);
int do_skewer(Character *ch, Character *vict, int dam, int wt, int wt2, int weapon);
void do_combatmastery(Character *ch, Character *vict, int weapon);
int do_behead_skill(Character *ch, Character *victim);
int do_execute_skill(Character *, Character *, int);
int weapon_spells(Character *ch, Character *vict, int weapon);
void eq_damage(Character *ch, Character *vict, int dam, int weapon_type, int attacktype);
void fight_kill(Character *ch, Character *vict, int type, int spec_type);
int can_attack(Character *ch);
int can_be_attacked(Character *ch, Character *vict);
int second_attack(Character *ch);
int third_attack(Character *ch);
int fourth_attack(Character *ch);
int second_wield(Character *ch);
void set_cantquit(Character *, Character *, bool = false);
int is_pkill(Character *ch, Character *vict);
void raw_kill(Character *ch, Character *victim);
void do_pkill(Character *ch, Character *victim, int type, bool vict_is_attacker = false);
void arena_kill(Character *ch, Character *victim, int type);
void do_dead(Character *ch, Character *victim);
void eq_destroyed(Character *ch, Object *obj, int pos);
int is_stunned(Character *ch);
void update_flags(Character *vict);
void update_stuns(Character *ch);
void do_dam_msgs(Character *ch, Character *victim, int dam, int attacktype, int weapon, int filter = 0);
int act_poisonous(Character *ch);
int isHit(Character *ch, Character *victim, int attacktype, int &type, int &reduce);
void inform_victim(Character *ch, Character *victim, int dam);
Character *loop_followers(struct follow_type **f);
Character *get_highest_level_killer(Character *leader, Character *killer);
int32_t count_xp_eligibles(Character *leader, Character *killer, int32_t highest_level, int32_t *total_levels);
int64_t scale_char_xp(Character *ch, Character *killer, Character *victim, int32_t no_killers, int32_t total_levels, int32_t highest_level, int64_t base_xp, int64_t *bonus_xp);
void remove_active_potato(Character *vict);

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

struct threat_struct
{
  struct threat_struct *next;
  int threat;
  char *name;
};

#endif
