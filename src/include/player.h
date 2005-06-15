#ifndef PLAYER_H_
#define PLAYER_H_
/************************************************************************
| $Id: player.h,v 1.19 2005/06/15 19:38:04 shane Exp $
| player.h
| Description: This file defines the player vectors..punishment, specials,
|   etc...
*/
#include <punish.h> // punishment vectors
#include <structs.h> // byte

/************************************************************************
| CHAR_DATA.misc vector
|  These should probably not be combined, and they probably shouldn't
|  be here, but they are.  Morc XXX
|  I really shouldn't have the flag here to differentiate mob's from PC's
|  but it's as good a place as any and this is all fubar'd anyway.
|  -pir 7/30
*/
#define LOG_BUG            1
#define LOG_PRAYER         1<<1
#define LOG_GOD            1<<2
#define LOG_MORTAL         1<<3
#define LOG_SOCKET         1<<4
#define LOG_MISC           1<<5
#define LOG_PLAYER         1<<6
#define CHANNEL_GOSSIP     1<<7
#define CHANNEL_AUCTION    1<<8
#define CHANNEL_INFO       1<<9
#define CHANNEL_TRIVIA     1<<10
#define CHANNEL_DREAM      1<<11
#define CHANNEL_CLAN       1<<12
#define CHANNEL_NEWBIE     1<<13
#define CHANNEL_SHOUT      1<<14
#define LOG_WORLD          1<<15
#define LOG_CHAOS          1<<16
#define LOG_CLAN           1<<17
#define LOG_WARNINGS       1<<18
#define LOG_HELP	   1<<19

#define LOG_DATABASE	1<<20
// ...
#define MISC_IS_MOB        1<<31

/************************************************************************
| Thirst, FULL, etc..
*/
#define DRUNK         0
#define FULL          1
#define THIRST        2

/************************************************************************
| Player vectors
| CHAR_DATA->pcdata->toggles
*/
#define PLR_BRIEF        1
#define PLR_COMPACT      1<<1
#define PLR_DONTSET      1<<2
#define PLR_DONOTUSE     1<<3
#define PLR_NOHASSLE     1<<4
#define PLR_SUMMONABLE   1<<5
#define PLR_WIMPY        1<<6
#define PLR_ANSI         1<<7
#define PLR_VT100        1<<8
#define PLR_ONEWAY       1<<9
#define PLR_DISGUISED    1<<10
#define PLR_UNUSED       1<<11
#define PLR_PAGER        1<<12
#define PLR_BEEP         1<<13
#define PLR_BARD_SONG    1<<14
#define PLR_ANONYMOUS    1<<15
#define PLR_AUTOEAT      1<<16
#define PLR_LFG          1<<17
#define PLR_NOTELL       1<<18
#define PLR_NOTAX	 1<<19
#define PLR_GUIDE	 1<<20
#define PLR_GUIDE_TOG	 1<<21

/************************************************************************
| Class types for PCs
*/
#define CLASS_MAGIC_USER   1
#define CLASS_CLERIC       2
#define CLASS_THIEF        3
#define CLASS_WARRIOR      4
#define CLASS_ANTI_PAL     5
#define CLASS_PALADIN      6
#define CLASS_BARBARIAN    7
#define CLASS_MONK         8
#define CLASS_RANGER       9
#define CLASS_BARD        10
#define CLASS_DRUID       11
#define CLASS_PSIONIC     12
#define CLASS_NECROMANCER 13
#define CLASS_MAX         13

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
#define APPLY_NONE              0
#define APPLY_STR               1
#define APPLY_DEX               2
#define APPLY_INT               3
#define APPLY_WIS               4
#define APPLY_CON               5
#define APPLY_SEX               6
#define APPLY_CLASS             7
#define APPLY_LEVEL             8
#define APPLY_AGE               9
#define APPLY_CHAR_WEIGHT      10
#define APPLY_CHAR_HEIGHT      11
#define APPLY_MANA             12
#define APPLY_HIT              13
#define APPLY_MOVE             14
#define APPLY_GOLD             15
#define APPLY_EXP              16
#define APPLY_AC               17
#define APPLY_ARMOR            17
#define APPLY_HITROLL          18
#define APPLY_DAMROLL          19
#define APPLY_SAVING_FIRE      20
#define APPLY_SAVING_COLD      21
#define APPLY_SAVING_ENERGY    22
#define APPLY_SAVING_ACID      23
#define APPLY_SAVING_MAGIC     24
#define APPLY_SAVING_POISON    25
#define APPLY_HIT_N_DAM        26
#define APPLY_SANCTUARY        27
#define APPLY_SENSE_LIFE       28
#define APPLY_DETECT_INVIS     29
#define APPLY_INVISIBLE        30
#define APPLY_SNEAK            31
#define APPLY_INFRARED         32
#define APPLY_HASTE            33
#define APPLY_PROTECT_EVIL     34
#define APPLY_FLY              35
#define WEP_MAGIC_MISSILE      36
#define WEP_BLIND              37
#define WEP_EARTHQUAKE         38
#define WEP_CURSE              39
#define WEP_COLOUR_SPRAY       40
#define WEP_DISPEL_EVIL        41
#define WEP_ENERGY_DRAIN       42
#define WEP_FIREBALL           43
#define WEP_LIGHTNING_BOLT     44
#define WEP_HARM               45
#define WEP_POISON             46
#define WEP_SLEEP              47
#define WEP_FEAR               48
#define WEP_DISPEL_MAGIC       49
#define WEP_WEAKEN             50
#define WEP_CAUSE_LIGHT        51
#define WEP_CAUSE_CRITICAL     52
#define WEP_PARALYZE           53
#define WEP_ACID_BLAST         54
#define WEP_BEE_STING          55
#define WEP_CURE_LIGHT         56
#define WEP_FLAMESTRIKE        57
#define WEP_HEAL_SPRAY         58
#define WEP_DROWN              59
#define WEP_HOWL               60
#define WEP_SOULDRAIN          61
#define WEP_SPARKS             62
#define APPLY_BARKSKIN         63
#define APPLY_RESIST_FIRE      64
#define APPLY_RESIST_COLD      65
#define APPLY_KI               66
#define APPLY_CAMOUFLAGE       67
#define APPLY_FARSIGHT         68
#define APPLY_FREEFLOAT        69
#define APPLY_FROSTSHIELD      70
#define APPLY_INSOMNIA         71
#define APPLY_LIGHTNING_SHIELD 72
#define APPLY_REFLECT          73
#define APPLY_RESIST_ELECTRIC  74
#define APPLY_SHADOWSLIP       75
#define APPLY_SOLIDITY         76
#define APPLY_STABILITY        77
#define APPLY_STAUNCHBLOOD     78
#define WEP_DISPEL_GOOD        79
#define WEP_TELEPORT           80
#define WEP_CHILL_TOUCH        81
#define WEP_POWER_HARM         82
#define WEP_VAMPIRIC_TOUCH     83
#define WEP_LIFE_LEECH         84
#define WEP_METEOR_SWARM       85
#define WEP_ENTANGLE           86
#define APPLY_INSANE_CHANT     87
#define APPLY_GLITTER_DUST     88
#define APPLY_RESIST_ACID      89
#define APPLY_HP_REGEN         90
#define APPLY_MANA_REGEN       91
#define APPLY_MOVE_REGEN       92
#define APPLY_KI_REGEN         93
#define WEP_CREATE_FOOD        94
#define APPLY_DAMAGED          95
#define WEP_THIEF_POISON       96
#define APPLY_PROTECT_GOOD     97
#define APPLY_MELEE_DAMAGE     98
#define APPLY_SPELL_DAMAGE     99
#define APPLY_SONG_DAMAGE      100
#define APPLY_RESIST_MAGIC     101
#define APPLY_SAVES            102
#define APPLY_MAXIMUM_VALUE    102

/*
 1000+ are reserved, so if you were thinking about using, think
 again.
*/
/* RESERVED: 100-150 for more weapon affects */
/* Morc XXX */

// Constructor commented out for const.C initialization purposes
struct str_app_type
{
    sh_int todam;    /* Damage Bonus/Penalty                */
    sh_int carry_w;  /* Maximum weight that can be carrried */
    sh_int cold_resistance; /* Cold resistance */
};

struct dex_app_type
{
  sh_int tohit;
  sh_int ac_mod;
  sh_int move_gain;
  sh_int fire_resistance;
};

// Constructor commented out for const.C initialization purposes
struct wis_app_type
{
    sh_int mana_regen;
    sh_int ki_regen;
    sh_int bonus;       /* how many bonus skills a player can */
		      /* practice pr. level                 */
    sh_int energy_resistance;
    sh_int conc_bonus;
};

// Constructor commented out for const.C initialization purposes
struct int_app_type
{
    sh_int mana_regen;
    sh_int ki_regen;
    sh_int easy_bonus;
    sh_int medium_bonus;
    sh_int hard_bonus;
    sh_int learn_bonus;
    sh_int magic_resistance;
    sh_int conc_bonus;
};

// Constructor commented out for const.C initialization purposes
struct con_app_type
{
    sh_int hp_regen;
    sh_int move_regen;
    sh_int hp_gain;
    sh_int poison_resistance;
};

/* Extern definitions. These are all in const.cpp. */
extern const struct dex_app_type dex_app[];
extern const struct con_app_type con_app[];
extern const struct int_app_type int_app[];
extern const struct str_app_type str_app[];
extern const struct wis_app_type wis_app[];

/* Various function declarations */
int get_saves(CHAR_DATA *ch, int savetype);


#endif
