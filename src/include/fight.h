#ifndef FIGHT_H_
#define FIGHT_H_

/************************************************************************
| $Id: fight.h,v 1.3 2002/07/07 06:59:43 pirahna Exp $
| fight.h
| This file defines the header information for fight.
*/
#include <structs.h> // byte, ubyte, etc..
#include <obj.h> // WIELD, SECOND_WIELD

#ifdef NeXT
#ifndef bool
#define bool int
#endif
#endif

/* External prototype */
void special_log(char *arg);

/* Here are our function prototypes */
int  damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, 
            int weapon_type, int attacktype, int weapon); 
int spell_damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, 
            int weapon_type, int attacktype, int weapon);

 
extern CHAR_DATA *character_list;
/* extern CHAR_DATA *combat_list = NULL, *combat_next_dude = NULL; */

#define FIRST 	    WIELD
#define SECOND 	    SECOND_WIELD

#define COMBAT_MOD_FRENZY         1
#define COMBAT_MOD_UNUSED         1<<1

void make_husk(CHAR_DATA *ch);
void make_heart(CHAR_DATA *ch, CHAR_DATA *vict);
void make_head(CHAR_DATA *ch);
void make_scraps(CHAR_DATA *ch, struct obj_data *obj);
void remove_memory(CHAR_DATA *ch, char type);
void add_memory(CHAR_DATA *ch, char *victim, char type);
void stop_follower(CHAR_DATA *ch, int cmd);
int attack(CHAR_DATA *ch, CHAR_DATA *vict, int type, int attack = FIRST);
void send_info(char *messg);
void perform_violence(void);
void dam_message(int dam, CHAR_DATA *ch, CHAR_DATA *vict, int w_type, long modifier);
void group_gain(CHAR_DATA *ch, CHAR_DATA *vict);
int check_riposte(CHAR_DATA *ch, CHAR_DATA *vict);
bool check_shieldblock(CHAR_DATA *ch, CHAR_DATA *vict);
bool check_parry(CHAR_DATA *ch, CHAR_DATA *vict);
bool check_dodge(CHAR_DATA *ch, CHAR_DATA *vict);
void disarm(CHAR_DATA *ch, CHAR_DATA *vict);
void trip(CHAR_DATA *ch, CHAR_DATA *vict);

int one_hit(CHAR_DATA*ch, CHAR_DATA *vict, int type, int weapon);
int do_skewer(CHAR_DATA *ch, CHAR_DATA *vict, int dam, int weapon);
int do_behead(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int weapon);
int weapon_spells(CHAR_DATA *ch, CHAR_DATA *vict, int weapon);
void eq_damage(CHAR_DATA *ch, CHAR_DATA *vict, int dam, int weapon_type, int attacktype);
void fight_kill(CHAR_DATA *ch, CHAR_DATA *vict, int type);
int can_attack(CHAR_DATA *ch);
int can_be_attacked(CHAR_DATA *ch, CHAR_DATA *vict);
int second_attack(CHAR_DATA *ch);
int third_attack(CHAR_DATA *ch);
int fourth_attack(CHAR_DATA *ch);
int second_wield(CHAR_DATA *ch);
void set_cantquit(CHAR_DATA *ch, CHAR_DATA *vict);
int is_pkill(CHAR_DATA *ch, CHAR_DATA *vict);
void raw_kill(CHAR_DATA *ch, CHAR_DATA *victim);
void do_pkill(CHAR_DATA *ch, CHAR_DATA *victim);
void arena_kill(CHAR_DATA *ch, CHAR_DATA *victim);
void do_dead(CHAR_DATA *ch, CHAR_DATA *victim);
bool ArenaIsOpen();

// These are so that we only need one copy of one_hit and weapon_spells and
// skewer and behead

#define TYPE_CHOOSE       0
#define TYPE_PKILL        1
#define TYPE_RAW_KILL     2
#define TYPE_ARENA_KILL   3

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

#define DAMAGE_TYPE_PHYSICAL  0
#define DAMAGE_TYPE_MAGIC     1
#define DAMAGE_TYPE_SONG      2

/* Note sure why this didn't work.
enum {
   DAMAGE_TYPE_PHYSICAL = 0,
   DAMAGE_TYPE_MAGIC,
   DAMAGE_TYPE_SONG
};
*/

#endif
