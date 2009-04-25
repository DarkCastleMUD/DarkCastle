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
/* $Id: spells.h,v 1.124 2009/04/25 00:55:07 shane Exp $ */
#include <structs.h> // ubyte, int16

#include <map>
#define MAX_BUF_LENGTH               240

std::map<int,int> fill_skill_cost();

const std::map<int,int> skill_cost = fill_skill_cost();

void extractFamiliar(CHAR_DATA *ch);

bool skill_success(CHAR_DATA *ch, CHAR_DATA *victim, int skillnum, int mod=0);

/* New skill quest thingy. */
struct skill_quest
{
  struct skill_quest *next;
  char *message;
  int num;
  int clas;
  int level;
};

struct skill_stuff
{
  char *name;
  int difficulty;
};

void barb_magic_resist(char_data *ch, int old, int nw);
struct skill_quest *find_sq(int sq);
struct skill_quest *find_sq(char *);
int dam_percent(int learned, int damage);
void check_maxes(CHAR_DATA *ch);
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
#define SPELL_IRIDESCENT_AURA         6
#define SPELL_CHARM_PERSON            7
#define SPELL_CHILL_TOUCH             8
#define SPELL_CLONE                   9
#define SPELL_COLOUR_SPRAY           10
#define SPELL_CONTROL_WEATHER        11
#define SPELL_CREATE_FOOD            12
#define SPELL_CREATE_WATER           13
#define SPELL_REMOVE_BLIND           14
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
#define SPELL_CAMOUFLAGE            115
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
#define SPELL_MISANRA_QUIVER           131
#define SPELL_ICE_SHARDS            132
#define SPELL_LIGHTNING_SHIELD      133
#define SPELL_BLUE_BIRD             134
#define SPELL_DEBILITY              135
#define SPELL_ATTRITION             136
#define SPELL_VAMPIRIC_AURA         137
#define SPELL_HOLY_AURA             138
#define SPELL_DISMISS_FAMILIAR      139
#define SPELL_DISMISS_CORPSE        140
#define SPELL_BLESSED_HALO          141
#define SPELL_VISAGE_OF_HATE        142
#define SPELL_PROTECT_FROM_GOOD     143
#define SPELL_OAKEN_FORTITUDE	    144
#define SPELL_FROSTSHIELD	    145
#define SPELL_STABILITY		    146
#define SPELL_KILLER 		    147
#define SPELL_CANTQUIT 		    148
#define SPELL_SOLIDITY 		    149
#define SPELL_EAS 		    150
#define SPELL_ALIGN_GOOD 	    151 // uriel's fire of redemption
#define SPELL_ALIGN_EVIL 	    152 // Moruk's heart
#define SPELL_AEGIS 		    153
#define SPELL_U_AEGIS               154
#define SPELL_RESIST_MAGIC          155
#define SPELL_EAGLE_EYE             156
#define SPELL_CALL_LIGHTNING	    157
#define SPELL_DIVINE_FURY           158
#define SPELL_GHOSTWALK		    159
#define SPELL_MEND_GOLEM            160
#define SPELL_CLARITY               161
#define SPELL_DIVINE_INTER	    162
#define SPELL_WRATH_OF_GOD	    163
#define SPELL_ATONEMENT             164
#define SPELL_SILENCE               165
#define SPELL_IMMUNITY              166
#define SPELL_BONESHIELD            167
#define SPELL_CHANNEL               168
#define SPELL_RELEASE_ELEMENTAL     169
#define SPELL_WILD_MAGIC	    170
#define SPELL_SPIRIT_SHIELD	    171
#define MAX_SPL_LIST                171

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
#define KI_AGILITY    8
#define KI_MEDITATION 9
#define MAX_KI_LIST   9 
#define KI_OFFSET     250     // why this is done differently than the rest, I have no
                              // idea....ki skills are 250-296.  -pir

#define FUCK_PTHIEF                  297
#define FUCK_GTHIEF                  298
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
#define SKILL_HEADBUTT               309
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
#define SKILL_BLUDGEON_WEAPONS       357
#define SKILL_PIERCEING_WEAPONS      358
#define SKILL_SLASHING_WEAPONS       359
#define SKILL_WHIPPING_WEAPONS       360
#define SKILL_CRUSHING_WEAPONS       361
#define SKILL_TWO_HANDED_WEAPONS     362
#define SKILL_HAND_TO_HAND           363
#define SKILL_BULLRUSH               364
#define SKILL_FEROCITY               365
#define SKILL_TACTICS                366
#define SKILL_DECEIT                 367
#define SKILL_RELEASE		     368
#define SKILL_FEARGAZE		     369
#define SKILL_EYEGOUGE		     370
#define SKILL_MAGIC_RESIST	     371
#define NEW_SAVE		     372 // Savefix.
#define SKILL_SPELLCRAFT	     373
#define SKILL_DEFENSE 		     374 // MArtial defense
#define SKILL_KNOCKBACK              375
#define SKILL_STINGING_WEAPONS       376
#define SKILL_JAB		     377
#define SKILL_APPRAISE               378
#define SKILL_NATURES_LORE           379
#define SKILL_FIRE_ARROW             380
#define SKILL_ICE_ARROW              381
#define SKILL_TEMPEST_ARROW          382
#define SKILL_GRANITE_ARROW          383
#define DO_NOT_USE_I_SUCK	     384 // Oh how convenient skills are for this, 
					// and so bad looking. yay. I want mysql.
#define META_REIMB		     385
#define SKILL_COMBAT_MASTERY		     386
#define SKILL_FASTJOIN		     387
#define SKILL_ENHANCED_REGEN         388
#define SKILL_CRIPPLE                389
#define SKILL_NAT_SELECT             390
#define SKILL_CLANAREA_CLAIM         391
#define SKILL_CLANAREA_CHALLENGE     392
#define SKILL_COMMUNE                393
#define SKILL_SCRIBE_SCROLL          394
#define SKILL_MAKE_CAMP              395
#define SKILL_BATTLESENSE            396
#define SKILL_PERSEVERANCE           397
#define SKILL_TRIAGE                 398
#define SKILL_SMITE                  399
#define SKILL_LEADERSHIP             400
#define SKILL_EXECUTE                401
#define SKILL_DEFENDERS_STANCE       402
#define SKILL_BEHEAD                 403
#define SKILL_PRIMAL_FURY            404
#define SKILL_VIGOR		     405
#define SKILL_ESCAPE		     406
#define SKILL_CRIT_HIT		     407
#define SKILL_BATTERBRACE	     408
#define SKILL_FREE_ANIMAL            409
#define SKILL_OFFHAND_DOUBLE	     410
#define SKILL_MAX                    410



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
#define SKILL_SONG_UNRESIST_DITTY    547
#define SKILL_SONG_FANATICAL_FANFARE 548
#define SKILL_SONG_DISCHORDANT_DIRGE 549
#define SKILL_SONG_CRUSHING_CRESCENDO 550
#define SKILL_SONG_HYPNOTIC_HARMONY  551
#define SKILL_SONG_MKING_CHARGE      552
#define SKILL_SONG_SUBMARINERS_ANTHEM 553
#define SKILL_SONG_SUMMONING_SONG    554
#define SKILL_SONG_MAX               554
// if you add a song, make sure you update "songs[]" in sing.C
// as well as SKILL_SONG_MAX

// God commands that are "bestow"/"revoke"able
#define COMMAND_BASE                 600
#define COMMAND_STRING               600
#define COMMAND_IMP_CHAN             601
#define COMMAND_STAT                 602
#define COMMAND_SNOOP                603
#define COMMAND_FIND                 604
#define COMMAND_POSSESS              606
#define COMMAND_RESTORE              607
#define COMMAND_PURLOIN              608
#define COMMAND_ARENA                609
#define COMMAND_SET                  610
#define COMMAND_SQSAVE               611
#define COMMAND_WHATTONERF           612
#define COMMAND_FORCE                613
#define COMMAND_SEND                 614
#define COMMAND_LOAD                 615
#define COMMAND_SHUTDOWN             616
#define COMMAND_MP_EDIT              617
#define COMMAND_RANGE                618
#define COMMAND_MPSTAT               619
#define COMMAND_SEDIT                621
#define COMMAND_SOCKETS              622
#define COMMAND_PUNISH               623
#define COMMAND_SQEDIT		     624
#define COMMAND_OCLONE		     625
#define COMMAND_RELOAD 		     626
#define COMMAND_HEDIT		     627
#define COMMAND_OPSTAT		     629
#define COMMAND_OPEDIT		     630
#define COMMAND_EQMAX		     631
#define COMMAND_LOG 		     632
#define COMMAND_ADDNEWS		     633
#define COMMAND_PRIZE		     634
#define COMMAND_QEDIT                635
#define COMMAND_RENAME		     636
#define COMMAND_FINDPATH             637
#define COMMAND_FINDPATH2            638
#define COMMAND_ADDROOM              639
#define COMMAND_NEWPATH              640
#define COMMAND_LISTPATHSBYZONE      641
#define COMMAND_LISTALLPATHS         642
#define COMMAND_TESTHAND             643
#define COMMAND_DOPATHPATH           644
#define COMMAND_DO_THE_THING         645
#define COMMAND_QUEST                646
#define COMMAND_TESTPORT             647
#define COMMAND_REMORT		     648
#define COMMAND_TESTHIT		     649
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
 * If you change this, update strs_damage_types[] in const.cpp
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
// If you change this, update strs_damage_types[] in const.cpp
////////////////////////////////////////////////////////////////

#define BASE_TIMERS 1100

// NOTE  "skill" numbers 1500-1599 are reserved for innate skill abilities
// These are in innate.h

//////////////////////////////////////////////////////////////////////
// NOTE 'spell' wear off timers are here.  Reserved messages 4000-4099
// If you change this, update reserved[] in const.cpp
//////////////////////////////////////////////////////////////////////
#define RESERVED_BASE                4000
#define SPELL_HOLY_AURA_TIMER        4000
#define SPELL_NAT_SELECT_TIMER       4001
#define SPELL_DIV_INT_TIMER	     4002
#define SPELL_NO_CAST_TIMER	     4003
#define SKILL_CM_TIMER               4004
#define OBJ_CHAMPFLAG_TIMER          4005
#define SKILL_TRIAGE_TIMER           4006
#define SKILL_SMITE_TIMER            4007
#define SKILL_MAKE_CAMP_TIMER        4008
#define SKILL_LEADERSHIP_BONUS       4009
#define SKILL_PERSEVERANCE_BONUS     4010
#define SKILL_DECEIT_TIMER           4011
#define SKILL_FEROCITY_TIMER         4012
#define SKILL_TACTICS_TIMER          4013
#define CONC_LOSS_FIXER              4014
#define RESERVED_MAX                 4014
///////////////////////////////////////////////////////////////////////

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







typedef	int	SPELL_FUN	( ubyte level, CHAR_DATA *ch,
				  char *arg, int type,
				  CHAR_DATA *tar_ch,
				  struct obj_data *tar_obj,
                                  int skill );

// NOTE:  If you change this structure, keep in mind how it is used in guild.C
// The min_level_XXX stuff MUST be updated in guild.C if you change this.  It is
// using an offset from min_level_magic depending on class *(min_level_magic+2bytes)
struct spell_info_type
{
    ubyte	beats;			/* Waiting time after spell	*/
    ubyte	minimum_position;	/* Position for caster		*/
    ubyte	min_usesmana;		/* Mana used			*/
    int16	targets;		/* Legal targets		*/
    SPELL_FUN *	spell_pointer;		/* Function to call		*/
    int16      difficulty; 		/* Spell difficulty */
};


#define SPELL_TYPE_SPELL    0
#define SPELL_TYPE_POTION   1
#define SPELL_TYPE_WAND     2
#define SPELL_TYPE_STAFF    3
#define SPELL_TYPE_SCROLL   4


#define FIRE_ELEMENTAL	88
#define WATER_ELEMENTAL	89
#define AIR_ELEMENTAL	90
#define EARTH_ELEMENTAL	91

#define WILD_OFFENSIVE  0
#define WILD_DEFENSIVE  1

/*
 * Attack types with grammar.
 */
struct attack_hit_type
{
  char *singular;
  char *plural;
};

#define RARE1_PAPER	1
#define RARE2_PAPER	1<<1
#define RARE3_PAPER	1<<2
#define RARE4_PAPER	1<<3
#define RARE5_PAPER	1<<4
//#define FREE_SLOT	1<<5

#define CLERIC_PEN 	1<<6
#define MAGE_PEN 	1<<7
#define DRUID_PEN 	1<<8
#define ANTI_PEN	1<<9
#define RANGER_PEN	1<<10
#define NONE_PEN	1<<11
//#define FREE_SLOT	1<<12

#define MAGIC_INK	1<<13
#define FIRE_INK	1<<14
#define EVIL_INK	1<<15
//#define FREE_SLOT	1<<16

#define FLASHY_DUST	1<<17
#define EXPLOSIVE_DUST	1<<18
#define GENERIC_DUST	1<<19
//#define FREE_SLOT	1<<20



/*
 * reasons for stopping following
 * passed as the cmd arg to stop_follow()
 */
#define STOP_FOLLOW	0
#define END_STALK	1
#define CHANGE_LEADER	2
#define BROKE_CHARM	3

  #define DETECT_GOOD_VNUM 6302
  #define DETECT_EVIL_VNUM 6301
  #define DETECT_INVISIBLE_VNUM 6306
  #define SENSE_LIFE_VNUM 6304
  #define INFRA_VNUM 6308
  #define INVIS_VNUM 6303
  #define FARSIGHT_VNUM 6307
  #define SOLIDITY_VNUM 6309
  #define LIGHTNING_SHIELD_VNUM 6310
  #define INSOMNIA_VNUM 6311
  #define HASTE_VNUM 6312
  #define TRUE_VNUM 6305

#endif
