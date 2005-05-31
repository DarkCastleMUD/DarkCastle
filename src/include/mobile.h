#ifndef MOBILE_H_
#define MOBILE_H_
/************************************************************************
| $Id: mobile.h,v 1.14 2005/05/31 11:24:50 urizen Exp $
| mobile.h
| Description:  This file contains the header information for mobile
|   control.
*/
void rebuild_rnum_references(int startAt, int type);
void    mprog_driver            ( char* com_list, CHAR_DATA* mob,
                                       CHAR_DATA* actor, OBJ_DATA* obj,
                                       void* vo );


#define BASE_STAT      13

#define ACT_SPEC       0
#define ACT_SENTINEL   1
#define ACT_SCAVENGER  2
#define ACT_NOTRACK    3
#define ACT_NICE_THIEF 4
#define ACT_AGGRESSIVE 5
#define ACT_STAY_ZONE  6
#define ACT_WIMPY      7
			        /* aggressive only attack sleeping players */
#define ACT_2ND_ATTACK 8
#define ACT_3RD_ATTACK 9
#define ACT_4TH_ATTACK 10
			         /* Each attack bit must be set to get up */
			         /* 4 attacks                             */
/*
 * For ACT_AGGRESSIVE_XXX, you must also set ACT_AGGRESSIVE
 * These switches can be combined, if none are selected, then
 * the mobile will attack any alignment (same as if all 3 were set)
 */
#define ACT_AGGR_EVIL       11
#define ACT_AGGR_GOOD       12
#define ACT_AGGR_NEUT       13
#define ACT_UNDEAD          14
#define ACT_STUPID          15
#define ACT_CHARM           16
#define ACT_HUGE            17
#define ACT_DODGE           18
#define ACT_PARRY           19
#define ACT_RACIST          20
#define ACT_FRIENDLY        21
#define ACT_STAY_NO_TOWN    22
#define ACT_NOMAGIC 	    23
#define ACT_DRAINY          24
#define ACT_BARDCHARM	    25
#define ACT_NOKI	    26
#define ACT_NOMATRIX	    27
#define ACT_BOSS 	    28
#define ACT_MAX             28

struct race_shit
{
  char *singular_name;  /* dwarf, elf, etc.     */
  char *plural_name;     /* dwarves, elves, etc. */
   
  long body_parts;  /* bitvector for body parts       */
  long immune;      /* bitvector for immunities       */
  long resist;      /* bitvector for resistances      */
  long suscept;     /* bitvector for susceptibilities */
  long hate_fear;   /* bitvector for hate/fear        */
  long friendly;    /* bitvector for friendliness     */
  int  weight;      /* average weight of ths race    */
  int  height;      /* average height for ths race   */
  int affects;      /* automatically added affects   */
};

int oprog_act_trigger( char *txt, CHAR_DATA *ch );
int oprog_speech_trigger( char *txt, CHAR_DATA *ch );
int oprog_command_trigger( char *txt, CHAR_DATA *ch );
int oprog_weapon_trigger( CHAR_DATA *ch, OBJ_DATA *item );
int oprog_armour_trigger( CHAR_DATA *ch, OBJ_DATA *item );
int oprog_rand_trigger( OBJ_DATA *item);
int oprog_arand_trigger( OBJ_DATA *item);
int oprog_greet_trigger( CHAR_DATA *ch);
int oprog_load_trigger( OBJ_DATA *item);

struct mob_matrix_data
{
  long long experience;
  int hitpoints;
  int tohit;
  int todam;
  int armor;
  int gold;
};

#endif
