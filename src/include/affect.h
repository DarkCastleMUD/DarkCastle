#ifndef AFFECT_H_
#define AFFECT_H_

/************************************************************************
| $Id: affect.h,v 1.6 2003/12/01 17:39:08 staylor Exp $
| affect.h
| This contains the bitvectors for affected_by
*/

/* Bitvector for 'affected_by' */
#define AFF_BLIND               1     // 1
#define AFF_INVISIBLE           1<<1
#define AFF_DETECT_EVIL         1<<2
#define AFF_DETECT_INVISIBLE    1<<3
#define AFF_DETECT_MAGIC        1<<4
#define AFF_SENSE_LIFE          1<<5
#define AFF_REFLECT             1<<6
#define AFF_SANCTUARY           1<<7
#define AFF_GROUP               1<<8
#define AFF_EAS                 1<<9  // 10
#define AFF_CURSE               1<<10
#define AFF_FROSTSHIELD         1<<11
#define AFF_POISON              1<<12
#define AFF_PROTECT_EVIL        1<<13
#define AFF_PARALYSIS           1<<14
#define AFF_DETECT_GOOD         1<<15
#define AFF_FIRESHIELD          1<<16
#define AFF_SLEEP               1<<17
#define AFF_TRUE_SIGHT          1<<18
#define AFF_SNEAK               1<<19 // 20
#define AFF_HIDE                1<<20
#define AFF_IGNORE_WEAPON_WEIGHT 1<<21
#define AFF_CHARM               1<<22
#define AFF_RAGE                1<<23
#define AFF_SOLIDITY            1<<24
#define AFF_INFRARED            1<<25
#define AFF_CANTQUIT            1<<26
#define AFF_KILLER              1<<27
#define AFF_FLYING              1<<28
#define AFF_LIGHTNINGSHIELD     1<<29  // 30
#define AFF_HASTE               1<<30

/* Bits used with affected_by2 */
#define AFF_SHADOWSLIP          1
#define AFF_INSOMNIA            1<<1
#define AFF_FREEFLOAT           1<<2
#define AFF_FARSIGHT            1<<3
#define AFF_CAMOUFLAGUE         1<<4
#define AFF_STABILITY           1<<5
#define AFF_NOT_USED            1<<6
#define AFF_GOLEM               1<<7
#define AFF_FOREST_MELD         1<<8
#define AFF_INSANE              1<<9
#define AFF_GLITTER_DUST        1<<10
#define AFF_UTILITY             1<<11
#define AFF_ALERT               1<<12
#define AFF_NO_FLEE             1<<13
#define AFF_FAMILIAR            1<<14
#define AFF_PROTECT_GOOD        1<<15

#endif
