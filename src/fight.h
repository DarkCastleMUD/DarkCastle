#ifndef FIGHT_H_
#define FIGHT_H_

/************************************************************************
| $Id: fight.h,v 1.45 2015/06/16 04:10:54 pirahna Exp $
| fight.h
| This file defines the header information for fight.
*/
#include "structs.h" // ubyte, ubyte, etc..
#include "obj.h" // WIELD, SECOND_WIELD
#include "character.h"
#include "utility.h" // FALSE
#ifdef NeXT
#ifndef bool
#define bool int
#endif
#endif

/* External prototype */
void special_log(char *arg);
void debug_point();

/* Here are our function prototypes */
int  damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, 
            int weapon_type, int attacktype, int weapon, bool is_death_prog = false); 
int noncombat_damage(CHAR_DATA * ch, int dam, char *char_death_msg,
                     char *room_death_msg, char *death_log_msg, int type);
void send_damage(char const *, CHAR_DATA *, OBJ_DATA *, CHAR_DATA *, char const *, char const *, int);
int getRealSpellDamage(CHAR_DATA *ch);
 
#define FIRST 	    WIELD
#define SECOND 	    SECOND_WIELD

#define COMBAT_MOD_FRENZY            1
#define COMBAT_MOD_RESIST            1<<1
#define COMBAT_MOD_SUSCEPT           1<<2
#define COMBAT_MOD_IGNORE            1<<3
#define COMBAT_MOD_REDUCED            1<<4
void make_dust(CHAR_DATA * ch);
bool do_frostshield(CHAR_DATA *ch, CHAR_DATA *vict);
int speciality_bonus(CHAR_DATA *ch, int attacktype, int level);
void make_husk(CHAR_DATA *ch);
void make_heart(CHAR_DATA *ch, CHAR_DATA *vict);
void make_head(CHAR_DATA *ch);
void make_arm(CHAR_DATA *ch);
void make_leg(CHAR_DATA *ch);
void make_bowels(CHAR_DATA *ch);
void make_blood(CHAR_DATA *ch);
void make_scraps(CHAR_DATA *ch, struct obj_data *obj);
void remove_memory(CHAR_DATA *ch, char type);
void room_mobs_only_hate(char_data *ch);
void add_memory(CHAR_DATA *ch, char *victim, char type);
void stop_follower(CHAR_DATA *ch, int cmd);
int attack(CHAR_DATA *ch, CHAR_DATA *vict, int type, int attack = FIRST);
void perform_violence(void);
void dam_message(int dam, CHAR_DATA *ch, CHAR_DATA *vict, int w_type, long modifier);
void group_gain(CHAR_DATA *ch, CHAR_DATA *vict);
int check_magic_block(CHAR_DATA *ch, CHAR_DATA *victim, int attacktype);
int check_riposte(CHAR_DATA *ch, CHAR_DATA *vict, int attacktype);
int check_shieldblock(CHAR_DATA *ch, CHAR_DATA *vict, int attacktype);
bool check_parry(CHAR_DATA *ch, CHAR_DATA *vict, int attacktype, bool display_results = true);
bool check_dodge(CHAR_DATA *ch, CHAR_DATA *vict, int attacktype, bool display_results = true);
void disarm(CHAR_DATA *ch, CHAR_DATA *vict);
void trip(CHAR_DATA *ch, CHAR_DATA *vict);
int checkCounterStrike(CHAR_DATA *, CHAR_DATA *);
int doTumblingCounterStrike(CHAR_DATA *, CHAR_DATA *);

int one_hit(CHAR_DATA*ch, CHAR_DATA *vict, int type, int weapon);
int do_skewer(CHAR_DATA *ch, CHAR_DATA *vict, int dam, int wt, int wt2, int weapon);
void do_combatmastery(CHAR_DATA *ch, CHAR_DATA *vict, int weapon);
int do_behead_skill(CHAR_DATA *ch, CHAR_DATA *victim);
int do_execute_skill(CHAR_DATA *, CHAR_DATA *, int);
int weapon_spells(CHAR_DATA *ch, CHAR_DATA *vict, int weapon);
void eq_damage(CHAR_DATA *ch, CHAR_DATA *vict, int dam, int weapon_type, int attacktype);
void fight_kill(CHAR_DATA *ch, CHAR_DATA *vict, int type, int spec_type);
int can_attack(CHAR_DATA *ch);
int can_be_attacked(CHAR_DATA *ch, CHAR_DATA *vict);
int second_attack(CHAR_DATA *ch);
int third_attack(CHAR_DATA *ch);
int fourth_attack(CHAR_DATA *ch);
int second_wield(CHAR_DATA *ch);
void set_cantquit(CHAR_DATA *, CHAR_DATA *, bool = FALSE);
int is_pkill(CHAR_DATA *ch, CHAR_DATA *vict);
void raw_kill(CHAR_DATA *ch, CHAR_DATA *victim);
void do_pkill(CHAR_DATA *ch, CHAR_DATA *victim, int type, bool vict_is_attacker = false);
void arena_kill(CHAR_DATA *ch, CHAR_DATA *victim, int type);
void do_dead(CHAR_DATA *ch, CHAR_DATA *victim);
bool ArenaIsOpen();
void eq_destroyed(char_data * ch, obj_data * obj, int pos);

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
