#ifndef AFFECT_H_
#define AFFECT_H_

/************************************************************************
| $Id: affect.h,v 1.17 2005/06/14 22:13:45 shane Exp $
| affect.h
| This contains the bits for affected_by
*/

/* Bits for 'affected_by' */
//#define AFF_CHECKTHISASIZE     0
#define AFF_BLIND               1
#define AFF_INVISIBLE           2
#define AFF_DETECT_EVIL         3
#define AFF_DETECT_INVISIBLE    4
#define AFF_DETECT_MAGIC        5
#define AFF_SENSE_LIFE          6
#define AFF_REFLECT             7
#define AFF_SANCTUARY           8
#define AFF_GROUP               9
#define AFF_EAS                 10
#define AFF_CURSE               11
#define AFF_FROSTSHIELD         12
#define AFF_POISON              13
#define AFF_PROTECT_EVIL        14
#define AFF_PARALYSIS           15
#define AFF_DETECT_GOOD         16
#define AFF_FIRESHIELD          17
#define AFF_SLEEP               18
#define AFF_TRUE_SIGHT          19
#define AFF_SNEAK               20
#define AFF_HIDE                21
#define AFF_IGNORE_WEAPON_WEIGHT 22
#define AFF_CHARM               23
#define AFF_RAGE                24
#define AFF_SOLIDITY            25
#define AFF_INFRARED            26
#define AFF_CANTQUIT            27
#define AFF_KILLER              28
#define AFF_FLYING              29
#define AFF_LIGHTNINGSHIELD     30
#define AFF_HASTE               31
//#define AFF_CHECKTHISASIZE     32 //do not change unless ASIZE changes
#define AFF_SHADOWSLIP          33
#define AFF_INSOMNIA            34
#define AFF_FREEFLOAT           35
#define AFF_FARSIGHT            36
#define AFF_CAMOUFLAGUE         37
#define AFF_STABILITY           38
#define AFF_NEWSAVE             39 // Newsave
#define AFF_GOLEM               40 // This is used for IRON golem, not stone. 
				     // It differs the two.
#define AFF_FOREST_MELD         41
#define AFF_INSANE              42
#define AFF_GLITTER_DUST        43
#define AFF_UTILITY             44
#define AFF_ALERT               45
#define AFF_NO_FLEE             46
#define AFF_FAMILIAR            47
#define AFF_PROTECT_GOOD        48
#define AFF_POWERWIELD		49 // Innate Powerwield
#define AFF_REGENERATION	50 // Innate Regeneration
#define AFF_FOCUS		51 // Innate focus
#define AFF_ILLUSION		52 // Innate illusion
#define AFF_KNOW_ALIGN		53
#define AFF_BLACKJACK_ALERT 	54 // cannot be blackjacked, or blackjack
#define AFF_WATERBREATHING      55
#define AFF_MAX                 55
//#define AFF_CHECKTHISASIZE     64 //do not change unless ASIZE changes
//#define AFF_CHECKTHISASIZE     96 //do not change unless ASIZE changes
#endif

