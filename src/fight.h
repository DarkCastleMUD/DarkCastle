#ifndef FIGHT_H_
#define FIGHT_H_

/************************************************************************
| $Id: fight.h,v 1.45 2015/06/16 04:10:54 pirahna Exp $
| fight.h
| This file defines the header information for fight.
*/
#include "structs.h" // uint8_t, uint8_t, etc..
#include "obj.h"     // WIELD, SECOND_WIELD
#include "character.h"
#include "utility.h" // FALSE

/* External prototype */
void debug_point();

/* Here are our function prototypes */
int damage(struct char_data *ch, struct char_data *victim, int dam,
           int weapon_type, int attacktype, int weapon, bool is_death_prog = false);
int noncombat_damage(struct char_data *ch, int dam, char *char_death_msg,
                     char *room_death_msg, char *death_log_msg, int type);
void send_damage(char const *, struct char_data *, obj_data *, struct char_data *, char const *, char const *, int);
int getRealSpellDamage(struct char_data *ch);

#define FIRST WIELD
#define SECOND SECOND_WIELD

#define COMBAT_MOD_FRENZY 1
#define COMBAT_MOD_RESIST 1 << 1
#define COMBAT_MOD_SUSCEPT 1 << 2
#define COMBAT_MOD_IGNORE 1 << 3
#define COMBAT_MOD_REDUCED 1 << 4
void make_dust(struct char_data *ch);
bool do_frostshield(struct char_data *ch, struct char_data *vict);
int speciality_bonus(struct char_data *ch, int attacktype, int level);
void make_husk(struct char_data *ch);
void make_heart(struct char_data *ch, struct char_data *vict);
void make_head(struct char_data *ch);
void make_arm(struct char_data *ch);
void make_leg(struct char_data *ch);
void make_bowels(struct char_data *ch);
void make_blood(struct char_data *ch);
void make_scraps(struct char_data *ch, struct obj_data *obj);
void room_mobs_only_hate(struct char_data *ch);
void add_memory(struct char_data *ch, char *victim, char type);
void stop_follower(struct char_data *ch, int cmd);
int attack(struct char_data *ch, struct char_data *vict, int type, int attack = FIRST);
void perform_violence(void);
void dam_message(int dam, struct char_data *ch, struct char_data *vict, int w_type, int32_t modifier);
void group_gain(struct char_data *ch, struct char_data *vict);
int check_magic_block(struct char_data *ch, struct char_data *victim, int attacktype);
int check_riposte(struct char_data *ch, struct char_data *vict, int attacktype);
int check_shieldblock(struct char_data *ch, struct char_data *vict, int attacktype);
bool check_parry(struct char_data *ch, struct char_data *vict, int attacktype, bool display_results = true);
bool check_dodge(struct char_data *ch, struct char_data *vict, int attacktype, bool display_results = true);
void disarm(struct char_data *ch, struct char_data *vict);
void trip(struct char_data *ch, struct char_data *vict);
int checkCounterStrike(struct char_data *, struct char_data *);
int doTumblingCounterStrike(struct char_data *, struct char_data *);

int one_hit(struct char_data *ch, struct char_data *vict, int type, int weapon);
int do_skewer(struct char_data *ch, struct char_data *vict, int dam, int wt, int wt2, int weapon);
void do_combatmastery(struct char_data *ch, struct char_data *vict, int weapon);
int do_behead_skill(struct char_data *ch, struct char_data *victim);
int do_execute_skill(struct char_data *, struct char_data *, int);
int weapon_spells(struct char_data *ch, struct char_data *vict, int weapon);
void eq_damage(struct char_data *ch, struct char_data *vict, int dam, int weapon_type, int attacktype);
void fight_kill(struct char_data *ch, struct char_data *vict, int type, int spec_type);
int can_attack(struct char_data *ch);
int can_be_attacked(struct char_data *ch, struct char_data *vict);
int second_attack(struct char_data *ch);
int third_attack(struct char_data *ch);
int fourth_attack(struct char_data *ch);
int second_wield(struct char_data *ch);
void set_cantquit(struct char_data *, struct char_data *, bool = FALSE);
int is_pkill(struct char_data *ch, struct char_data *vict);
void raw_kill(struct char_data *ch, struct char_data *victim);
void do_pkill(struct char_data *ch, struct char_data *victim, int type, bool vict_is_attacker = false);
void arena_kill(struct char_data *ch, struct char_data *victim, int type);
void do_dead(struct char_data *ch, struct char_data *victim);
bool ArenaIsOpen();
void eq_destroyed(struct char_data *ch, obj_data *obj, int pos);
int is_stunned(struct char_data *ch);
void update_flags(struct char_data *vict);
void update_stuns(struct char_data *ch);
void do_dam_msgs(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int weapon, int filter = 0);
int act_poisonous(struct char_data *ch);
int isHit(struct char_data *ch, struct char_data *victim, int attacktype, int &type, int &reduce);
void inform_victim(struct char_data *ch, struct char_data *victim, int dam);
struct char_data *loop_followers(struct follow_type **f);
struct char_data *get_highest_level_killer(struct char_data *leader, struct char_data *killer);
int32_t count_xp_eligibles(struct char_data *leader, struct char_data *killer, int32_t highest_level, int32_t *total_levels);
int64_t scale_char_xp(struct char_data *ch, struct char_data *killer, struct char_data *victim, int32_t no_killers, int32_t total_levels, int32_t highest_level, int64_t base_xp, int64_t *bonus_xp);
void remove_active_potato(struct char_data *vict);
int check_pursuit(struct char_data *ch, struct char_data *victim, char *dircommand);

// These are so that we only need one copy of one_hit and weapon_spells and
// skewer and behead

#define TYPE_CHOOSE       0
#define TYPE_PKILL        1
#define TYPE_RAW_KILL     2
#define TYPE_ARENA_KILL   3

#define KILL_OTHER        0
#define KILL_DROWN        1
#define KILL_FALL         2
#define KILL_POISON       3
#define KILL_SUICIDE      4
#define KILL_POTATO       5
#define KILL_MASHED       6
#define KILL_BINGO        7
#define KILL_BATTER	  8
#define KILL_MORTAR       9

#define COMBAT_SHOCKED      1
#define COMBAT_BASH1        1<<1
#define COMBAT_BASH2        1<<2
#define COMBAT_STUNNED      1<<3
#define COMBAT_STUNNED2     1<<4
#define COMBAT_CIRCLE       1<<5
#define COMBAT_BERSERK      1<<6
#define COMBAT_HITALL       1<<7
#define COMBAT_RAGE1        1<<8
#define COMBAT_RAGE2        1<<9
#define COMBAT_BLADESHIELD1 1<<10
#define COMBAT_BLADESHIELD2 1<<11
#define COMBAT_REPELANCE    1<<12
#define COMBAT_VITAL_STRIKE 1<<13
#define COMBAT_MONK_STANCE  1<<14
#define COMBAT_MISS_AN_ATTACK 1<<15
#define COMBAT_ORC_BLOODLUST1 1<<16
#define COMBAT_ORC_BLOODLUST2 1<<17
#define COMBAT_THI_EYEGOUGE  1<<18
#define COMBAT_THI_EYEGOUGE2 1<<19
#define COMBAT_FLEEING       1<<20
#define COMBAT_SHOCKED2      1<<21
#define COMBAT_CRUSH_BLOW    1<<22
#define COMBAT_ATTACKER	     1<<23
#define COMBAT_CRUSH_BLOW2   1<<24

#define DAMAGE_TYPE_PHYSICAL  0
#define DAMAGE_TYPE_MAGIC     1
#define DAMAGE_TYPE_SONG      2

struct threat_struct
{
  struct threat_struct *next;
  int threat;
  char *name;
};

#endif
