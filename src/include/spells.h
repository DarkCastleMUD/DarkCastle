#ifndef SPELLS_H_
#define SPELLS_H_
/***************************************************************************
 *  file: spells.h , Implementation of magic spells.       Part of DIKUMUD *
 *  Usage : Spells                                                         *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: spells.h,v 1.16 2002/09/20 18:42:58 pirahna Exp $ */
#include <structs.h> // byte, sh_int

#define MAX_BUF_LENGTH               240

/*
 * Spell numbers are well known.
 * They appear in tinyworld files attached to magical items.
 * Spells and skills share the same address space (for practicing)
 *   and also for special damage messages as attack types.
 */
#define TYPE_UNDEFINED               -1
#define SPELL_RESERVED_DBC            0
#define SPELL_ARMOR                   1
#define SPELL_TELEPORT                2
#define SPELL_BLESS                   3
#define SPELL_BLINDNESS               4
#define SPELL_BURNING_HANDS           5
#define SPELL_CALL_LIGHTNING          6
#define SPELL_CHARM_PERSON            7
#define SPELL_CHILL_TOUCH             8
#define SPELL_CLONE                   9
#define SPELL_COLOUR_SPRAY           10
#define SPELL_CONTROL_WEATHER        11
#define SPELL_CREATE_FOOD            12
#define SPELL_CREATE_WATER           13
#define SPELL_CURE_BLIND             14
#define SPELL_CURE_CRITIC            15
#define SPELL_CURE_LIGHT             16
#define SPELL_CURSE                  17
#define SPELL_DETECT_EVIL            18
#define SPELL_DETECT_INVISIBLE       19
#define SPELL_DETECT_MAGIC           20
#define SPELL_DETECT_POISON          21
#define SPELL_DISPEL_EVIL            22
#define SPELL_EARTHQUAKE             23
#define SPELL_ENCHANT_WEAPON         24
#define SPELL_ENERGY_DRAIN           25
#define SPELL_FIREBALL               26
#define SPELL_HARM                   27
#define SPELL_HEAL                   28
#define SPELL_INVISIBLE              29
#define SPELL_LIGHTNING_BOLT         30
#define SPELL_LOCATE_OBJECT          31
#define SPELL_MAGIC_MISSILE          32
#define SPELL_POISON                 33
#define SPELL_PROTECT_FROM_EVIL      34
#define SPELL_REMOVE_CURSE           35
#define SPELL_SANCTUARY              36
#define SPELL_SHOCKING_GRASP         37
#define SPELL_SLEEP                  38
#define SPELL_STRENGTH               39
#define SPELL_SUMMON                 40
#define SPELL_VENTRILOQUATE          41
#define SPELL_WORD_OF_RECALL         42
#define SPELL_REMOVE_POISON          43
#define SPELL_SENSE_LIFE             44
#define SPELL_SUMMON_FAMILIAR        45
#define SPELL_LIGHTED_PATH           46
#define SPELL_RESIST_ACID            47
#define SPELL_SUN_RAY                48
#define SPELL_RAPID_MEND             49
#define SPELL_ACID_SHIELD            50
#define SPELL_WATER_BREATHING        51
#define SPELL_GLOBE_OF_DARKNESS      52
#define SPELL_IDENTIFY               53
#define SPELL_ANIMATE_DEAD           54
#define SPELL_FEAR                   55
#define SPELL_FLY                    56
#define SPELL_CONT_LIGHT             57
#define SPELL_KNOW_ALIGNMENT         58
#define SPELL_DISPEL_MAGIC           59
#define SPELL_CONJURE_ELEMENTAL      60
#define SPELL_CURE_SERIOUS           61
#define SPELL_CAUSE_LIGHT            62
#define SPELL_CAUSE_CRITICAL         63
#define SPELL_CAUSE_SERIOUS          64
#define SPELL_FLAMESTRIKE            65
#define SPELL_STONE_SKIN             66
#define SPELL_SHIELD                 67
#define SPELL_WEAKEN                 68
#define SPELL_MASS_INVISIBILITY      69
#define SPELL_ACID_BLAST             70
#define SPELL_PORTAL                 71
#define SPELL_INFRAVISION            72
#define SPELL_REFRESH                73
#define SPELL_HASTE		     74
#define SPELL_DISPEL_GOOD            75
#define SPELL_HELLSTREAM             76
#define SPELL_POWER_HEAL             77
#define SPELL_FULL_HEAL              78
#define SPELL_FIRESTORM              79
#define SPELL_POWER_HARM             80
#define SPELL_DETECT_GOOD            81
#define SPELL_VAMPIRIC_TOUCH         82
#define SPELL_LIFE_LEECH             83
#define SPELL_PARALYZE               84
#define SPELL_REMOVE_PARALYSIS       85
#define SPELL_FIRESHIELD             86
#define SPELL_METEOR_SWARM           87
#define SPELL_WIZARD_EYE             88
#define SPELL_TRUE_SIGHT             89
#define SPELL_MANA                   90
#define SPELL_SOLAR_GATE             91
#define SPELL_HEROES_FEAST           92
#define SPELL_HEAL_SPRAY             93
#define SPELL_GROUP_SANC             94
#define SPELL_GROUP_RECALL           95
#define SPELL_GROUP_FLY              96
#define SPELL_ENCHANT_ARMOR          97
#define SPELL_RESIST_FIRE            98
#define SPELL_RESIST_COLD            99
#define SPELL_BEE_STING	            100
#define SPELL_BEE_SWARM	            101
#define SPELL_CREEPING_DEATH	    102
#define SPELL_BARKSKIN	            103
#define SPELL_HERB_LORE	            104
#define SPELL_CALL_FOLLOWER	    105
#define SPELL_ENTANGLE	            106
#define SPELL_EYES_OF_THE_OWL	    107
#define SPELL_FELINE_AGILITY        108
#define SPELL_FOREST_MELD	    109
#define SPELL_COMPANION	            110
#define SPELL_DROWN	            111
#define SPELL_HOWL	            112
#define SPELL_SOULDRAIN	            113
#define SPELL_SPARKS	            114
#define SPELL_CAMOUFLAGUE           115
#define SPELL_FARSIGHT              116
#define SPELL_FREEFLOAT             117
#define SPELL_INSOMNIA              118
#define SPELL_SHADOWSLIP            119
#define SPELL_RESIST_ENERGY         120
#define SPELL_STAUNCHBLOOD          121
#define SPELL_CREATE_GOLEM	    122
#define SPELL_REFLECT               123
#define SPELL_DISPEL_MINOR          124
#define SPELL_RELEASE_GOLEM         125
#define SPELL_BEACON                126
#define SPELL_STONE_SHIELD          127
#define SPELL_GREATER_STONE_SHIELD  128
#define SPELL_IRON_ROOTS            129
#define SPELL_EYES_OF_THE_EAGLE     130
#define SPELL_HASTE_OTHER           131
#define SPELL_ICE_SHARDS            132
#define SPELL_LIGHTNING_SHIELD      133
#define SPELL_BLUE_BIRD             134
#define MAX_SPL_LIST                134

// if you add a spell, make sure you update "spells[]" in spells.C

/*
 * 150 to 249 reserved for more spells.
 */

/* 
 * KI usage is here to avoid code duplication
 */
#define KI_BLAST      0
#define KI_PUNCH      1
#define KI_SENSE      2
#define KI_STORM      3
#define KI_SPEED      4
#define KI_PURIFY     5
#define KI_DISRUPT    6
#define KI_STANCE     7
#define MAX_KI_LIST   7 
#define KI_OFFSET     250     // why this is done differently than the rest, I have no
                              // idea....ki skills are 250-298.  -pir

#define FUCK_CANTQUIT                299

#define SKILL_BASE                   300
#define SKILL_TRIP                   300
#define SKILL_DODGE                  301
#define SKILL_SECOND_ATTACK          302
#define SKILL_DISARM                 303
#define SKILL_THIRD_ATTACK           304
#define SKILL_PARRY                  305
#define SKILL_DEATHSTROKE            306
#define SKILL_CIRCLE                 307
#define SKILL_BERSERK                308
#define SKILL_SHOCK                  309
#define SKILL_EAGLE_CLAW             310
#define SKILL_QUIVERING_PALM         311
#define SKILL_PALM                   312
#define SKILL_STALK                  313
#define SKILL_UNUSED                 314
#define SKILL_DUAL_BACKSTAB          315
#define SKILL_HITALL                 316
#define SKILL_STUN                   317
#define SKILL_SCAN                   318
#define SKILL_CONSIDER               319
#define SKILL_SWITCH                 320
#define SKILL_REDIRECT               321
#define SKILL_AMBUSH                 322
#define SKILL_FORAGE                 323
#define SKILL_TAME                   324
#define SKILL_TRACK                  325
#define SKILL_SKEWER		     326
#define SKILL_SLIP		     327
#define SKILL_RETREAT		     328
#define SKILL_RAGE		     329
#define SKILL_BATTLECRY		     330
#define SKILL_ARCHERY		     331
#define SKILL_RIPOSTE                332
#define SKILL_LAY_HANDS              333
#define SKILL_INSANE_CHANT           334
#define SKILL_GLITTER_DUST           335
#define SKILL_SNEAK                  336
#define SKILL_HIDE                   337
#define SKILL_STEAL                  338
#define SKILL_BACKSTAB               339
#define SKILL_PICK_LOCK              340
#define SKILL_KICK                   341
#define SKILL_BASH                   342 
#define SKILL_RESCUE                 343
#define SKILL_BLOOD_FURY             344
#define SKILL_DUAL_WIELD             345
#define SKILL_HARM_TOUCH             346
#define SKILL_SHIELDBLOCK            347
#define SKILL_BLADESHIELD            348
#define SKILL_POCKET                 349
#define SKILL_GUARD                  350
#define SKILL_FRENZY                 351
#define SKILL_BLINDFIGHTING          352
#define SKILL_FOCUSED_REPELANCE      353
#define SKILL_VITAL_STRIKE           354
#define SKILL_CRAZED_ASSAULT         355
#define SKILL_DIVINE_PROTECTION      356
#define SKILL_MAX                    356

// if you add a skill, make sure you update "skills[]" in spells.C
// as well as SKILL_MAX


#define SKILL_SONG_BASE              525
#define SKILL_SONG_LIST_SONGS        525
#define SKILL_SONG_WHISTLE_SHARP     526
#define SKILL_SONG_STOP              527
#define SKILL_SONG_TRAVELING_MARCH   528
#define SKILL_SONG_BOUNT_SONNET      529
#define SKILL_SONG_INSANE_CHANT      530
#define SKILL_SONG_GLITTER_DUST      531
#define SKILL_SONG_SYNC_CHORD        532
#define SKILL_SONG_HEALING_MELODY    533
#define SKILL_SONG_STICKY_LULL       534
#define SKILL_SONG_REVEAL_STACATO    535
#define SKILL_SONG_FLIGHT_OF_BEE     536
#define SKILL_SONG_JIG_OF_ALACRITY   537
#define SKILL_SONG_NOTE_OF_KNOWLEDGE 538
#define SKILL_SONG_TERRIBLE_CLEF     539
#define SKILL_SONG_SOOTHING_REMEM    540
#define SKILL_SONG_FORGETFUL_RHYTHM  541
#define SKILL_SONG_SEARCHING_SONG    542
#define SKILL_SONG_VIGILANT_SIREN    543
#define SKILL_SONG_ASTRAL_CHANTY     544
#define SKILL_SONG_DISARMING_LIMERICK 545
#define SKILL_SONG_SHATTERING_RESO   546
#define SKILL_SONG_MAX               546
// if you add a song, make sure you update "songs[]" in sing.C
// as well as SKILL_SONG_MAX

// God commands that are "bestow"/"revoke"able
#define COMMAND_BASE                 600
#define COMMAND_GOTO                 600
#define COMMAND_IMP_CHAN             601
#define COMMAND_FAKELOG              602
#define COMMAND_SNOOP                603
#define COMMAND_LOG                  604
#define COMMAND_GLOBAL               605
#define COMMAND_PROMPT_VIEW          606
#define COMMAND_RESTORE              607
#define COMMAND_PURLOIN              608
#define COMMAND_ARENA                609
#define COMMAND_SET                  610
#define COMMAND_UNBAN                611
#define COMMAND_BAN                  612
#define COMMAND_ECHO                 613
#define COMMAND_SEND                 614
#define COMMAND_LOAD                 615
#define COMMAND_SHUTDOWN             616
#define COMMAND_MP_EDIT              617
#define COMMAND_RANGE                618
#define COMMAND_MPSTAT               619
#define COMMAND_PSHOPEDIT            620
#define COMMAND_SEDIT                621
#define COMMAND_SOCKETS              622

// make sure up you update bestowable_god_commands_type bestowable_god_commands[]
// if you modify this command list any


#define SKILL_TRADE_BASE             700
#define SKILL_TRADE_POISON           700
#define SKILL_TRADE_MAX              700
// make sure you update tradeskills[] in combinables.cpp if you add to this


#define SKILL_RECALL                 800
#define INTERNAL_SLEEPING            801

/*
 * Only for dragon breaths, not char abilities.
 */
#define SPELL_FIRE_BREATH            900
#define SPELL_GAS_BREATH             901
#define SPELL_FROST_BREATH           902
#define SPELL_ACID_BREATH            903
#define SPELL_LIGHTNING_BREATH       904


/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_HIT                     1000
#define TYPE_BLUDGEON                (TYPE_HIT +  1)
#define TYPE_PIERCE                  (TYPE_HIT +  2)
#define TYPE_SLASH                   (TYPE_HIT +  3)
#define TYPE_WHIP                    (TYPE_HIT +  4)
#define TYPE_CLAW                    (TYPE_HIT +  5)
#define TYPE_BITE                    (TYPE_HIT +  6)
#define TYPE_STING                   (TYPE_HIT +  7)
#define TYPE_CRUSH                   (TYPE_HIT +  8)
#define TYPE_SUFFERING               (TYPE_HIT +  9)
#define TYPE_MAGIC                   (TYPE_HIT + 10)
#define TYPE_CHARM                   (TYPE_HIT + 11)
#define TYPE_FIRE                    (TYPE_HIT + 12)
#define TYPE_ENERGY                  (TYPE_HIT + 13)
#define TYPE_ACID                    (TYPE_HIT + 14)
#define TYPE_POISON                  (TYPE_HIT + 15)
#define TYPE_SLEEP                   (TYPE_HIT + 16)
#define TYPE_COLD                    (TYPE_HIT + 17)
#define TYPE_PARA                    (TYPE_HIT + 18)
#define TYPE_KI                      (TYPE_HIT + 19)
#define TYPE_SONG                    (TYPE_HIT + 20)
#define TYPE_PHYSICAL_MAGIC          (TYPE_HIT + 21)
#define TYPE_WATER                   (TYPE_HIT + 22)

// NOTE  "skill" numbers 1500-1599 are reserved for innate skill abilities
// These are in innate.h

// NOTE  "skill" numbers 2000-2199 are reserved for weapon poisoning damage message

#define POISON_MESSAGE_BASE          2000



#define TAR_IGNORE         1
#define TAR_CHAR_ROOM      1<<1
#define TAR_CHAR_WORLD     1<<2
#define TAR_FIGHT_SELF     1<<3
#define TAR_FIGHT_VICT     1<<4
#define TAR_SELF_ONLY      1<<5
#define TAR_SELF_NONO      1<<6
#define TAR_OBJ_INV        1<<7
#define TAR_OBJ_ROOM       1<<8
#define TAR_OBJ_WORLD      1<<9
#define TAR_OBJ_EQUIP      1<<10
#define TAR_NONE_OK        1<<11
#define TAR_SELF_DEFAULT   1<<12

typedef	int	SPELL_FUN	( byte level, CHAR_DATA *ch,
				  char *arg, int type,
				  CHAR_DATA *tar_ch,
				  struct obj_data *tar_obj,
                                  int skill );

// NOTE:  If you change this structure, keep in mind how it is used in guild.C
// The min_level_XXX stuff MUST be updated in guild.C if you change this.  It is
// using an offset from min_level_magic depending on class *(min_level_magic+2bytes)
struct spell_info_type
{
    byte	beats;			/* Waiting time after spell	*/
    byte	minimum_position;	/* Position for caster		*/
    ubyte	min_usesmana;		/* Mana used			*/
    sh_int	targets;		/* Legal targets		*/
    SPELL_FUN *	spell_pointer;		/* Function to call		*/
};


#define SPELL_TYPE_SPELL    0
#define SPELL_TYPE_POTION   1
#define SPELL_TYPE_WAND     2
#define SPELL_TYPE_STAFF    3
#define SPELL_TYPE_SCROLL   4


/*
 * Attack types with grammar.
 */
struct attack_hit_type
{
  char *singular;
  char *plural;
};
#endif
