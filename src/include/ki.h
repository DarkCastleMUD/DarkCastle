/*
 * ki.h contains the header files for the
 * ki powers, although the ki structure is in
 * spells.h, so that must be included
 */
/* $Id: ki.h,v 1.3 2002/07/07 06:59:43 pirahna Exp $ */

#ifndef KI_H_
#define KI_H_

#include <structs.h> // byte, ubyte, etc..

typedef int	KI_FUN		( byte level, CHAR_DATA *ch, char *arg, CHAR_DATA *vict);

/************************************************************************
| These are pretty worthless, since I never did anything with them
|   and it was a pretty stupid idea anyway ;P We can take them out
|   at some point.  Morc XXX
*/
/* The following defs refer to player ki and its effects */
#define MAXIMUM_KI   100
#define MINIMUM_KI   0
#define MIN_REACT_KI   20
#define NO_EFFECT   0
#define DIVINE   1
#define MIRACLE   2
#define MAJOR_EFFECT   3
#define MINOR_EFFECT   4

struct ki_info_type
{
	byte beats;	/* Waiting time after ki */
	byte minimum_position; /* min position for use */
	byte min_level_monk; /* Minimum monk level */
	ubyte min_useski;	/* minimum ki used */
	sh_int targets;		/* Legal targets */
	KI_FUN *ki_pointer;	/* function to call */
};

/************************************************************************
| Function declarations
*/

int ki_gain(CHAR_DATA *ch);
int ki_check(CHAR_DATA *ch);
void reduce_ki(CHAR_DATA *ch, int type);
char * skip_spaces(char *string);

KI_FUN ki_blast;
KI_FUN ki_punch;
KI_FUN ki_sense;
KI_FUN ki_storm;
KI_FUN ki_speed;
KI_FUN ki_purify;
KI_FUN ki_disrupt;
KI_FUN ki_stance;

#endif
