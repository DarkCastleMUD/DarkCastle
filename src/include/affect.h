#ifndef AFFECT_H_
#define AFFECT_H_

/************************************************************************
| $Id: affect.h,v 1.16 2005/05/31 11:24:50 urizen Exp $
| affect.h
| This contains the bits for affected_by
*/

/* Bits for 'affected_by' */
#define AFF_BLIND               0
#define AFF_INVISIBLE           1
#define AFF_DETECT_EVIL         2
#define AFF_DETECT_INVISIBLE    3
#define AFF_DETECT_MAGIC        4
#define AFF_SENSE_LIFE          5
#define AFF_REFLECT             6
#define AFF_SANCTUARY           7
#define AFF_GROUP               8
#define AFF_EAS                 9
#define AFF_CURSE               10
#define AFF_FROSTSHIELD         11
#define AFF_POISON              12
#define AFF_PROTECT_EVIL        13
#define AFF_PARALYSIS           14
#define AFF_DETECT_GOOD         15
#define AFF_FIRESHIELD          16
#define AFF_SLEEP               17
#define AFF_TRUE_SIGHT          18
#define AFF_SNEAK               19
#define AFF_HIDE                20
#define AFF_IGNORE_WEAPON_WEIGHT 21
#define AFF_CHARM               22
#define AFF_RAGE                23
#define AFF_SOLIDITY            24
#define AFF_INFRARED            25
#define AFF_CANTQUIT            26
#define AFF_KILLER              27
#define AFF_FLYING              28
#define AFF_LIGHTNINGSHIELD     29
#define AFF_HASTE               30
#define AFF_SHADOWSLIP          31
#define AFF_INSOMNIA            32
#define AFF_FREEFLOAT           33
#define AFF_FARSIGHT            34
#define AFF_CAMOUFLAGUE         35
#define AFF_STABILITY           36
#define AFF_NEWSAVE             37 // Newsave
#define AFF_GOLEM               38 // This is used for IRON golem, not stone. 
				     // It differs the two.
#define AFF_FOREST_MELD         39
#define AFF_INSANE              40
#define AFF_GLITTER_DUST        41
#define AFF_UTILITY             42
#define AFF_ALERT               43
#define AFF_NO_FLEE             44
#define AFF_FAMILIAR            45
#define AFF_PROTECT_GOOD        46
#define AFF_POWERWIELD		47 // Innate Powerwield
#define AFF_REGENERATION	48 // Innate Regeneration
#define AFF_FOCUS		49 // Innate focus
#define AFF_ILLUSION		50 // Innate illusion
#define AFF_KNOW_ALIGN		51
#define AFF_BLACKJACK_ALERT 	52 // cannot be blackjacked, or blackjack
#define AFF_WATERBREATHING      53
#define AFF_MAX                 53
#endif

