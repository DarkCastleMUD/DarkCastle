#ifndef AFFECT_H_
#define AFFECT_H_

/************************************************************************
| $Id: affect.h,v 1.15 2005/05/28 18:56:22 shane Exp $
| affect.h
| This contains the bits for affected_by
*/

/* Bits for 'affected_by' */
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
#define AFF_SHADOWSLIP          32
#define AFF_INSOMNIA            33
#define AFF_FREEFLOAT           34
#define AFF_FARSIGHT            35
#define AFF_CAMOUFLAGUE         36
#define AFF_STABILITY           37
#define AFF_NEWSAVE             38 // Newsave
#define AFF_GOLEM               39 // This is used for IRON golem, not stone. 
				     // It differs the two.
#define AFF_FOREST_MELD         40
#define AFF_INSANE              41
#define AFF_GLITTER_DUST        42
#define AFF_UTILITY             43
#define AFF_ALERT               44
#define AFF_NO_FLEE             45
#define AFF_FAMILIAR            46
#define AFF_PROTECT_GOOD        47
#define AFF_POWERWIELD		48 // Innate Powerwield
#define AFF_REGENERATION	49 // Innate Regeneration
#define AFF_FOCUS		50 // Innate focus
#define AFF_ILLUSION		51 // Innate illusion
#define AFF_KNOW_ALIGN		52
#define AFF_BLACKJACK_ALERT 	53 // cannot be blackjacked, or blackjack
#define AFF_WATERBREATHING      54
#define AFF_MAX                 54
#endif

