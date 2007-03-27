#ifndef MOBILE_H_
#define MOBILE_H_
/************************************************************************
| $Id: mobile.h,v 1.22 2007/03/27 10:53:15 urizen Exp $
| mobile.h
| Description:  This file contains the header information for mobile
|   control.
*/
void rebuild_rnum_references(int startAt, int type);
void    mprog_driver            ( char* com_list, CHAR_DATA* mob,
                                       CHAR_DATA* actor, OBJ_DATA* obj,
                                       void* vo, struct mprog_throw_type *thrw = NULL );
char *  mprog_next_command      ( char* clist );


#define BASE_STAT      0

//#define NOTHING      0
#define ACT_SPEC       1
#define ACT_SENTINEL   2
#define ACT_SCAVENGER  3
#define ACT_NOTRACK    4
#define ACT_NICE_THIEF 5
#define ACT_AGGRESSIVE 6
#define ACT_STAY_ZONE  7
#define ACT_WIMPY      8
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
#define ACT_AGGR_EVIL       12
#define ACT_AGGR_GOOD       13
#define ACT_AGGR_NEUT       14
#define ACT_UNDEAD          15
#define ACT_STUPID          16
#define ACT_CHARM           17
#define ACT_HUGE            18
#define ACT_DODGE           19
#define ACT_PARRY           20
#define ACT_RACIST          21
#define ACT_FRIENDLY        22
#define ACT_STAY_NO_TOWN    23
#define ACT_NOMAGIC 	    24
#define ACT_DRAINY          25
#define ACT_BARDCHARM	    26
#define ACT_NOKI	    27
#define ACT_NOMATRIX	    28
#define ACT_BOSS 	    29
#define ACT_NOHEADBUTT      30
#define ACT_NOATTACK        31
#define ACT_MAX             31
//#define CHECKTHISACT      32 //Do not change unless ASIZE changes
//#define CHECKTHISACT      64 //Do not chance unless ASIZE changes

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
  int  min_weight;      /* min weight */
  int  max_weight;
  int  min_height;      
  int  max_height;
  int affects;      /* automatically added affects   */
  char *unarmed;    // unarmed attack message
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
