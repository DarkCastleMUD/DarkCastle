#ifndef PLAYER_H_
#define PLAYER_H_
/************************************************************************
| $Id: player.h,v 1.44 2010/02/19 06:09:18 jhhudso Exp $
| player.h
| Description: This file defines the player vectors..punishment, specials,
|   etc...
*/
#include "DC/punish.h"  // punishment vectors
#include "DC/structs.h" // uint8_t
#include "DC/class.h"

/************************************************************************
| Thirst, FULL, etc..
*/
#define DRUNK 0
#define FULL 1
#define THIRST 2

/*  For cut and paste purposes
   switch(GET_CLASS(mob))
   {
      case CLASS_MAGIC_USER:
      case CLASS_CLERIC:
      case CLASS_THIEF:
      case CLASS_WARRIOR:
      case CLASS_ANTI_PAL:
      case CLASS_PALADIN:
      case CLASS_BARBARIAN:
      case CLASS_MONK:
      case CLASS_RANGER:
      case CLASS_BARD:
      case CLASS_DRUID:
      case CLASS_PSIONIC:
      case CLASS_NECROMANCER:
      default:
         break;
   }

*/
/************************************************************************
| These should not be here - in fact, some of them should not exist.  We're
|   leaving them here for compatibility until we can get rid of them.
|   Morc XXX
*/
#define APPLY_NONE 0
#define APPLY_STR 1
#define APPLY_DEX 2
#define APPLY_INT 3
#define APPLY_WIS 4
#define APPLY_CON 5
#define APPLY_SEX 6
#define APPLY_CLASS 7
#define APPLY_LEVEL 8
#define APPLY_AGE 9
#define APPLY_CHAR_WEIGHT 10
#define APPLY_CHAR_HEIGHT 11
#define APPLY_MANA 12
#define APPLY_HIT 13
#define APPLY_MOVE 14
#define APPLY_GOLD 15
#define APPLY_EXP 16
#define APPLY_AC 17
#define APPLY_ARMOR 17
#define APPLY_HITROLL 18
#define APPLY_DAMROLL 19
#define APPLY_SAVING_FIRE 20
#define APPLY_SAVING_COLD 21
#define APPLY_SAVING_ENERGY 22
#define APPLY_SAVING_ACID 23
#define APPLY_SAVING_MAGIC 24
#define APPLY_SAVING_POISON 25
#define APPLY_HIT_N_DAM 26
#define APPLY_SANCTUARY 27
#define APPLY_SENSE_LIFE 28
#define APPLY_DETECT_INVIS 29
#define APPLY_INVISIBLE 30
#define APPLY_SNEAK 31
#define APPLY_INFRARED 32
#define APPLY_HASTE 33
#define APPLY_PROTECT_EVIL 34
#define APPLY_FLY 35
#define WEP_MAGIC_MISSILE 36
#define WEP_BLIND 37
#define WEP_EARTHQUAKE 38
#define WEP_CURSE 39
#define WEP_COLOUR_SPRAY 40
#define WEP_DISPEL_EVIL 41
#define WEP_ENERGY_DRAIN 42
#define WEP_FIREBALL 43
#define WEP_LIGHTNING_BOLT 44
#define WEP_HARM 45
#define WEP_POISON 46
#define WEP_SLEEP 47
#define WEP_FEAR 48
#define WEP_DISPEL_MAGIC 49
#define WEP_WEAKEN 50
#define WEP_CAUSE_LIGHT 51
#define WEP_CAUSE_CRITICAL 52
#define WEP_PARALYZE 53
#define WEP_ACID_BLAST 54
#define WEP_BEE_STING 55
#define WEP_CURE_LIGHT 56
#define WEP_FLAMESTRIKE 57
#define WEP_HEAL_SPRAY 58
#define WEP_DROWN 59
#define WEP_HOWL 60
#define WEP_SOULDRAIN 61
#define WEP_SPARKS 62
#define APPLY_BARKSKIN 63
#define APPLY_RESIST_FIRE 64
#define APPLY_RESIST_COLD 65
#define APPLY_KI 66
#define APPLY_CAMOUFLAGE 67
#define APPLY_FARSIGHT 68
#define APPLY_FREEFLOAT 69
#define APPLY_FROSTSHIELD 70
#define APPLY_INSOMNIA 71
#define APPLY_LIGHTNING_SHIELD 72
#define APPLY_REFLECT 73
#define APPLY_RESIST_ELECTRIC 74
#define APPLY_SHADOWSLIP 75
#define APPLY_SOLIDITY 76
#define APPLY_STABILITY 77
#define APPLY_STAUNCHBLOOD 78
#define WEP_DISPEL_GOOD 79
#define WEP_TELEPORT 80
#define WEP_CHILL_TOUCH 81
#define WEP_POWER_HARM 82
#define WEP_VAMPIRIC_TOUCH 83
#define WEP_LIFE_LEECH 84
#define WEP_METEOR_SWARM 85
#define WEP_ENTANGLE 86
#define APPLY_INSANE_CHANT 87
#define APPLY_GLITTER_DUST 88
#define APPLY_RESIST_ACID 89
#define APPLY_HP_REGEN 90
#define APPLY_MANA_REGEN 91
#define APPLY_MOVE_REGEN 92
#define APPLY_KI_REGEN 93
#define WEP_CREATE_FOOD 94
#define APPLY_DAMAGED 95
#define WEP_THIEF_POISON 96
#define APPLY_PROTECT_GOOD 97
#define APPLY_MELEE_DAMAGE 98
#define APPLY_SPELL_DAMAGE 99
#define APPLY_SONG_DAMAGE 100
#define APPLY_RESIST_MAGIC 101
#define APPLY_SAVES 102
#define APPLY_SPELLDAMAGE 103
#define APPLY_BOUNT_SONNET_HUNGER 104
#define APPLY_BOUNT_SONNET_THIRST 105
#define APPLY_BLIND 106
#define APPLY_WATER_BREATHING 107
#define APPLY_DETECT_MAGIC 108
#define WEP_WILD_MAGIC 109
#define APPLY_MAXIMUM_VALUE 109

/*
 1000+ are reserved, so if you were thinking about using, think
 again.
*/
/* RESERVED: 100-150 for more weapon affects */
/* Morc XXX */

// different stat combos for skill groups
#define STRDEX 1
#define STRCON 2
#define STRINT 3
#define STRWIS 4
#define DEXCON 5
#define DEXINT 6
#define DEXWIS 7
#define CONINT 8
#define CONWIS 9
#define INTWIS 10

#define MAX_PROFESSIONS 2

// Constructor commented out for const.C initialization purposes
struct str_app_type
{
    int16_t todam;           /* Damage Bonus/Penalty                */
    int16_t carry_w;         /* Maximum weight that can be carrried */
    int16_t cold_resistance; /* Cold resistance */
};

struct dex_app_type
{
    int16_t tohit;
    int16_t ac_mod;
    int16_t move_gain;
    int16_t fire_resistance;
};

// Constructor commented out for const.C initialization purposes
struct wis_app_type
{
    int16_t mana_regen;
    int16_t ki_regen;
    int16_t bonus; /* how many bonus skills a player can */
    /* practice pr. level                 */
    int16_t energy_resistance;
    int16_t conc_bonus;
    int16_t spell_dam_bonus; // For Cleric/Druid/Paladins naturally
};

// Constructor commented out for const.C initialization purposes
struct int_app_type
{
    int16_t mana_regen;
    int16_t ki_regen;
    int16_t easy_bonus;
    int16_t medium_bonus;
    int16_t hard_bonus;
    int16_t learn_bonus;
    int16_t magic_resistance;
    int16_t conc_bonus;
    int16_t spell_dam_bonus; // For Mage/Anti/Bard
};

// Constructor commented out for const.C initialization purposes
struct con_app_type
{
    int16_t hp_regen;
    int16_t move_regen;
    int16_t hp_gain;
    int16_t poison_resistance;
};

/* Extern definitions. These are all in const.cpp. */
extern const struct dex_app_type dex_app[];
extern const struct con_app_type con_app[];
extern const struct int_app_type int_app[];
extern const struct str_app_type str_app[];
extern const struct wis_app_type wis_app[];

/* Various function declarations */
int get_saves(Character *ch, int savetype);

#endif
