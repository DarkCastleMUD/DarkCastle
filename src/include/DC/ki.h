/*
 * ki.h contains the header files for the
 * ki powers, although the ki structure is in
 * spells.h, so that must be included
 */
/* $Id: ki.h,v 1.9 2009/05/02 22:41:34 shane Exp $ */

#ifndef KI_H_
#define KI_H_

#include "DC/structs.h" // uint8_t, uint8_t, etc..
#include "DC/character.h"

typedef int KI_FUN(uint8_t level, Character *ch, char *arg, Character *vict);

/************************************************************************
| These are pretty worthless, since I never did anything with them
|   and it was a pretty stupid idea anyway ;P We can take them out
|   at some point.  Morc XXX
*/
/* The following defs refer to player ki and its effects */
#define MAXIMUM_KI 100
#define MINIMUM_KI 0
#define MIN_REACT_KI 20
#define NO_EFFECT 0
#define DIVINE 1
#define MIRACLE 2
#define MAJOR_EFFECT 3
#define MINOR_EFFECT 4

struct ki_info_type
{
	uint32_t beats;				 /* Waiting time after ki */
	position_t minimum_position; /* min position for use */
	uint8_t min_useski;			 /* minimum ki used */
	int16_t targets;			 /* Legal targets */
	KI_FUN *ki_pointer;			 /* function to call */
	int difficulty;
};

/************************************************************************
| Function declarations
*/

int ki_check(Character *ch);
void reduce_ki(Character *ch, int type);
char *skip_spaces(char *string);

KI_FUN ki_blast;
KI_FUN ki_punch;
KI_FUN ki_sense;
KI_FUN ki_storm;
KI_FUN ki_speed;
KI_FUN ki_purify;
KI_FUN ki_disrupt;
KI_FUN ki_stance;
KI_FUN ki_agility;
KI_FUN ki_meditation;
KI_FUN ki_transfer;

#endif
