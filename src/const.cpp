/***************************************************************************
*  file: const.c                                          Part of DIKUMUD *
*  Usage: For constants used by the game.                                 *
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
/**************************************************************************/
/* Revision History                                                       */
/* 12/09/2003   Onager   Added protection from good to cleric and anti    */
/*                       spell list                                       */
/**************************************************************************/
/* $Id: const.cpp,v 1.172 2006/04/26 19:27:52 shane Exp $ */
/* I KNOW THESE SHOULD BE SOMEWHERE ELSE -- Morc XXX */

extern "C"
{
#include <stdio.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <obj.h>
#include <player.h> // *app_type
#include <character.h>
#include <spells.h>
#include <levels.h>
#include <mobile.h>
bestowable_god_commands_type bestowable_god_commands[] =
{
{ "impchan",		COMMAND_IMP_CHAN },
{ "snoop",	COMMAND_SNOOP },
{ "global",	COMMAND_GLOBAL },
{ "restore",	COMMAND_RESTORE },
{ "purloin",	COMMAND_PURLOIN },
{ "possess",   COMMAND_POSSESS},
{ "arena",	COMMAND_ARENA },
{ "set",	COMMAND_SET },
{ "load",	COMMAND_LOAD },
{ "shutdown",   COMMAND_SHUTDOWN },
{ "procedit",     COMMAND_MP_EDIT },
{ "range",      COMMAND_RANGE },
{ "procstat",     COMMAND_MPSTAT },
{ "pshopedit",  COMMAND_PSHOPEDIT },
{ "sedit",      COMMAND_SEDIT },
{ "punish",     COMMAND_PUNISH },
{ "sqedit",     COMMAND_SQEDIT },
{ "install",    COMMAND_INSTALL },
{ "hedit",      COMMAND_HEDIT },
{ "hindex",     COMMAND_HINDEX },
{ "opstat",	COMMAND_OPSTAT },
{ "opedit",	COMMAND_OPEDIT },
{ "eqmax",COMMAND_EQMAX},
{"force",COMMAND_FORCE},
{"string",COMMAND_STRING},
{"stat",COMMAND_STAT},
{"sqsave",COMMAND_SQSAVE},
{"whattonerf",COMMAND_WHATTONERF},
{"find",COMMAND_FIND},
{"log",COMMAND_LOG},
{"addnews",COMMAND_ADDNEWS},
{ "\n",		-1 }
};

// WEAR, ITEM_WEAR correspondances
int wear_corr[] =
{
  ITEM_LIGHT_SOURCE, //0
  ITEM_WEAR_FINGER,
  ITEM_WEAR_FINGER,
  ITEM_WEAR_NECK,
  ITEM_WEAR_NECK,
  ITEM_WEAR_BODY, // 5
  ITEM_WEAR_HEAD,
  ITEM_WEAR_LEGS,
  ITEM_WEAR_FEET,
  ITEM_WEAR_HANDS, 
  ITEM_WEAR_ARMS, // 10
  ITEM_WEAR_SHIELD,
  ITEM_WEAR_ABOUT,
  ITEM_WEAR_WAISTE,
  ITEM_WEAR_WRIST,
  ITEM_WEAR_WRIST, //15
  ITEM_WIELD,
  ITEM_WIELD,
  ITEM_HOLD,
  ITEM_HOLD,
  ITEM_WEAR_FACE,//20
  ITEM_WEAR_EAR, 
  ITEM_WEAR_EAR,
  0
};

// Obj proc types
char *obj_types[] = {
 "act_prog",
 "speech_prog",
 "rand_prog",
 "all_greet_prog",
 "catch_prog",
 "arand_prog",
 "load_prog",
 "command_prog",
 "weapon_prog",
 "armour_prog",
 "\n"
};

// every spell needs an entry in here
char *spell_wear_off_msg[] =
{
  "RESERVED DB.C",                                             /* 0 */
  "Your magical armour fades to nothing.",
  "!Teleport!",
  "Your blessing has expired.",
  "Your blinding affliction dissolves.",
  "!Burning Hands!",                                           /* 5 */
  "Your iridescent aura fades.",
  "You feel the affects of the charm end.",
  "A warmth returns to your bones, dispelling the deep $B$3chill$R.",
  "!Clone!",
  "!Color Spray!",                                             /* 10 */
  "!Control Weather!",
  "!Create Food!",
  "!Create Water!",
  "!Remove Blind!",
  "!Cure Critic!",                                             /* 15 */
  "!Cure Light!",
  "The curse upon you has ended.",
  "Your ability to sense evil has expired.",
  "Your ability to detect invisibility has expired.",
  "Your ability to detect magic has expired.",                 /* 20 */
  "Your ability to detect poison has expired.",
  "!Dispel Evil!",
  "!Earthquake!",
  "!Enchant Weapon!",
  "!Energy Drain!",                                            /* 25 */
  "!Fireball!",
  "!Harm!",
  "!Heal",
  "You feel your invisibility dissipate.",
  "!Lightning Bolt!",                                          /* 30 */
  "!Locate object!",
  "!Magic Missile!",
  "The $2poison$R in your blood has dissolved.",
  "You feel your moral vulnerability.",
  "!Remove Curse!",                                            /* 35 */
  "The $B$7white aura$R around your body fades.",
  "!Shocking Grasp!",
  "You feel less exhausted as the magical sleep expires.",
  "Your magically enhanced strength has worn off.",
  "!Summon!",                                                  /* 40 */
  "!Ventriloquate!",
  "!Word of Recall!",
  "!Remove Poison!",
  "You feel less aware of your surroundings.",
  "!Call Familiar!",                                           /* 45 */
  "!Lighted Path!",
  "The $2green$R in your skin fades.",
  "!Sun ray!",
  "Your body's healing process slows to normal.",
  "The shield of $2acid$R around you dissapates into the air.", /* 50 */
  "The gills on your neck shrink to nothing.",
  "!Globe Of Darkness!",
  "!Identify!",
  "!Animate Dead!",
  "!Fear!.",                                                   /* 55 */
  "You slowly float to the ground.",
  "Your light slowly fizzes into nothing.",
  "You can no longer sense the auras of others.",
  "!Dispel Magic!",
  "!Conjure Elemental!",                                       /* 60 */
  "!Cure Serious!",
  "!Cause Light!",
  "!Cause Critical!",
  "!Cause Serious!",
  "!Flamestrike!",                                             /* 65 */
  "Your skin does not feel as hard as it used to.",
  "Your force shield shimmers then fades away.",
  "You feel your strength return as the magical weakness fades.",
  "!Mass Invisibility!",  /* Uses the invisibility message */
  "!Acid Blast!",                                              /* 70 */
  "!Gate!",
  "You can no longer see in the dark.",
  "You feel your endurance restored.",
  "You don't feel as fast anymore.",
  "!Dispel Good!",                                             /* 75 */
  "!Hellstream!",
  "!Power Heal!",
  "!Full Heal!",
  "!Firestorm!",
  "!Power Harm!",                                              /* 80  */
  "Your ability to sense good has expired.",
  "!Vampiric Touch!",
  "!Life Leech!",
  "The paralysis fades and you can move again.",
  "!Remove Paralysis!", /* 85 */
  "The $B$4flames$R surrounding you fade away.",
  "!Meteor Swarm!",
  "!Wizard's Eye!",
  "You don't seem to be able to see everything as clearly anymore.",
  "!Mana!",                                                    /* 90 */
  "!Solar Gate!",
  "!Heroes Feast!",
  "!Heal Spray!",
  "!Group Sanc!",
  "!Group Recall!",
  "!Group Fly!",
  "!Enchant Armor!",
  "The $4red$R in your skin fades.",
  "The $3blue$R in your skin fades.",                          /* 99 */
  "!Bee Sting!",
  "!Bee Swarm!",
  "!Creeping Death!",
  "You feel less woody as your skin returns to its normal consistency.",
  "!Herb Lore!",
  "!Call Follower!",
  "!Entangle!",
  "Your vision is not so acute anymore.",
  "You no longer feel like such a big pussy.",
  "The forest kicks you out!",
  "!Companion!",	/* 110 */
  "!Drown!",
  "!Howl!",
  "!Souldrain!",
  "!Sparks!",     // 114
  "Your $2c$7a$0$Bmo$Ru$2fl$7a$0$Bg$R$7e$R has worn off.",
  "!FarSight!",
  "You slowly float down to the ground.",
  "!Insomnia!",
  "You step out of the shadows.",
  "The $5yellow$R in your skin fades.",  // 120
  "You no longer feel immune to the affects of poisons.",
  "!CreateGolem!",  // 122
  "!Reflect!",
  "!DispelMinor!",
  "!ReleaseGolem!",
  "!Beacon!",
  "The ethereal stones around you fade away.",
  "The ethereal stones around you fade away.", // greater stone shield
  "!Iron Roots!",
  "!Eyes of the Eagle!",
  "!Haste Other!",
  "!Ice Shards!",  
  "The $B$5lightning$R around you fades away leaving only static cling.",
  "!Blue bird!",
  "With a rush of strength, the $6debility$R fades from your body.",
  "Your rapid decay ends and your health returns to normal.",
  "The $B$0shadow$R in your aura fades away into the ethereal.",
  "Your serene aura of holiness fades.",
  "!DismissFamiliar!",
  "!DismissCorpse!",
  "Your blessed halo fades.",
  "You don't feel quite so hateful anymore.",
  "The foul mantle surrounding you dissipates to nothing.",
  "Your oaken fortitude fades, returning your constitution to normal.",
  "The $B$2icy$R shield of frost around you fades away.",
  "Your ability to stand firm ends as the magical stability fades.",
  "You are no longer flagged as a thief.",
  "You are no longer CANTQUIT flagged.",
  "You are again susceptible to magical transport as your solidity fades.",
  "You feel more susceptable to damage.",
  "!ALIGN_GOOD!",
  "!ALIGN_EVIL!",
  "Your protective aegis dissipates.",
  "Your unholy aegis dissipates.",
  "Your natural resistance to magic fades.",
  "!EAGLE_EYE!",
  "!UNUSED!"
};


int rev_dir[] =
{
    2,
    3,
    0,
    1,
    5,
    4
};

char *connected_states[] =
{
    "CON_PLAYING",
    "CON_GET_NAME",
    "CON_GET_OLD_PASSWORD",
    "CON_CONFIRM_NEW_NAME",
    "CON_GET_NEW_PASSWORD",
    "CON_CONFIRM_NEW_PASSWORD",
    "CON_GET_NEW_SEX",
    "CON_GET_NEW_CLASS",
    "CON_READ_MOTD",
    "CON_SELECT_MENU",
    "CON_RESET_PASSWORD",
    "CON_CONFIRM_RESET_PASSWORD",
    "CON_EXDSCR",
    "CON_GET_RACE",
    "CON_WRITE_BOARD",
    "CON_EDITING",
    "CON_SEND_MAIL",
    "CON_DELETE_CHAR",
    "CON_CHOOSE_STATS",
    "CON_PFILE_WIPE",
    "CON_ARCHIVE_CHAR",
    "CON_CLOSE",
    "CON_CONFIRM_PASSWORD_CHANGE",
    "CON_EDIT_MPROG",
    "CON_DISPLAY_ENTRANCE",
    "CON_PRE_DISPLAY_ENTRANCE",
    "\n"
};

int movement_loss[]=
{
    1,  /* Inside     */
    1,  /* City       */
    2,  /* Field      */
    3,  /* Forest     */
    4,  /* Hills      */
    6,  /* Mountains  */
    4,  /* Swimming   */
    1,  /* Unswimable */
    3,  /* Beach      */
    1,  /* Paved_road */
    4,  /* Desert     */
    6,  /* Underwater */
    5,  /* Swamp      */
    1,  /* Air        */
    4,  /* Frozen Tundra */
    6   /* Arctic */
};

char *dirs[] =
{
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "\n"
};

char *dirswards[] =
{
    "northward",
    "eastward",
    "southward",
    "westward",
    "upward",
    "downward",
    "\n"
};

char *weekdays[7] =
{
    "Aetherday",
    "the Day of Firesong",
    "the Day of the Wave",
    "the Day of Stonefall",
    "the Day of Glowing Eyes",
    "the Day of Balance",
    "Praiseday"
};

char *month_name[17] =
{
    "Month of the Whistling Wind",           /* 0 */
    "Month of the Frozen Sky",
    "Month of Water",
    "Month of the New Blossom",
    "Month of the Hungry Moon",
    "Month of the Honey Wind",
    "Month of War",
    "Month of Journeys",
    "Month of the King's Birth",
    "Month of Fever Dreams",
    "Month of the Barren Stone",
    "Month of Fire",
    "Month of Returns",
    "Month of the Harvest",
    "Month of the Scarlet Woods",
    "Month of the Bare Trees",
    "Month of Thin Shadows"
};

char *where[] =
{
    "<used as light> ",
    "<right finger>  ",
    "<left finger>   ",
    "<around neck>   ",
    "<around neck>   ",
    "<on body>       ",
    "<on head>       ",
    "<on legs>       ",
    "<on feet>       ",
    "<on hands>      ",
    "<on arms>       ",
    "<as shield>     ",
    "<about body>    ",
    "<about waist>   ",
    "<around wrist>  ",
    "<around wrist>  ",
    "<wielded>       ",
    "<second wield>  ",
    "<held>          ",
    "<held>          ",
    "<on face>       ",
    "<in left ear>   ",
    "<in right ear>  ",
    "<on penis>      "
};

char *strs_damage_types[] = 
{
    "hit",
    "bludgeon",
    "pierce",
    "slash",
    "whip",
    "claw",
    "bite",
    "sting",
    "crush",
    "suffering",
    "magic",
    "charm",
    "fire",
    "energy",
    "acid",
    "poison",
    "sleep",
    "cold",
    "para",
    "ki",
    "song",
    "physicalmagic",
    "water",
    "\n"
};

char *drinks[] =
{
    "water",
    "beer",
    "wine",
    "ale",
    "dark ale",
    "whiskey",
    "lemonade",
    "firebreather",
    "local speciality",
    "dirty water",
    "milk",
    "tea",
    "coffee",
    "blood",
    "salt water",
    "coca cola",
    "gatorade",
    "holy water",
    "\n"
};

char *drinknames[] =
{
    "water",
    "beer",
    "wine",
    "ale",
    "ale",
    "whiskey",
    "lemonade",
    "firebreather",
    "speciality",
    "dirt",
    "milk",
    "tea",
    "coffee",
    "blood",
    "salt",
    "cola",
    "gatorade",
    "water",
    "\n"
};

int drink_aff[][3] =
{
    { 0,1,10 },  /* water    */
    { 3,2,5 },   /* beer     */
    { 5,2,5 },   /* wine     */
    { 2,2,5 },   /* ale      */
    { 1,2,5 },   /* ale      */
    { 6,1,4 },   /* whiskey  */
    { 0,1,8 },   /* lemonade */
    { 10,0,0 },  /* firebr   */
    { 3,3,3 },   /* local    */
    { 0,4,-8 },  /* dirty    */
    { 0,3,6 },
    { 0,1,6 },
    { 0,1,6 },
    { 0,2,-1 },
    { 0,1,-2 },
    { 0,1,5 },
    { 0,1,10 },
    { 0,1,10 }
};

char *color_liquid[]=
{
    "clear",
    "brown",
    "clear",
    "brown",
    "dark",
    "golden",
    "red",
    "green",
    "clear",
    "light green",
    "white",
    "brown",
    "black",
    "red",
    "clear",
    "black",
    "neon green",
    "clear"
};

char *fullness[] =
{
    "less than half ",
    "about half ",
    "more than half ",
    ""
};

#define K    * 1000
#define M    K K
#define L  (long)

int exp_table[ ] =
{
0,   
1,          L(1 K),     L(2.5 K),   L(5 K),     L(10 K),   // level 5
L(20 K),    L(40 K),    L(60 K),    L(80 K),    L(100 K),  // level 10
L(200 K),   L(300 K),   L(500 K),   L(750 K),   L(1 M),    // level 15
L(1.5 M),   L(2 M),     L(3 M),     L(4 M),     L(5 M),    // level 20
L(6 M),     L(7 M),     L(8 M),     L(9 M),     L(10 M),   // level 25
L(12 M),    L(14 M),    L(16 M),    L(18 M),    L(20 M),   // level 30
L(22 M),    L(24 M),    L(26 M),    L(28 M),    L(31 M),   // level 35
L(34 M),    L(37 M),    L(40 M),    L(45 M),    L(50 M),   // level 40
L(60 M),    L(70 M),    L(80 M),    L(90 M),    L(100 M),  // level 45
L(120 M),   L(140 M),   L(160 M),   L(180 M),   L(200 M),  // level 50
L(400 M),   L(600 M),     L(800 M),     L(1000 M),     L(1200 M),    // level 55
L(1400 M),     L(1600 M),     L(1800 M),     L(2000 M),     L(2000 M),   // level 60
L(0x7FFFFFFF)
};

#undef M
#undef K
#undef L


char *item_types[] =
{
    "UNDEFINED",
    "LIGHT",
    "SCROLL",
    "WAND",
    "STAFF",
    "WEAPON",
    "FIRE WEAPON",
    "MISSILE",
    "TREASURE",
    "ARMOR",
    "POTION",
    "UNUSED",
    "OTHER",
    "TRASH",
    "TRAP",
    "CONTAINER",
    "NOTE",
    "LIQUID CONTAINER",
    "KEY",
    "FOOD",
    "MONEY",
    "PEN",
    "BOAT",
    "BOARD",
    "PORTAL",
    "FOUNTAIN",
    "INSTRUMENT",
    "UTILITY",
    "BEACON",
    "LOCKPICK",
    "CLIMBABLE",
    "MEGAPHONE",
    "ALTAR",
    "TOTEM",
    "\n"
};

char *wear_bits[] =
{
    "TAKE",
    "FINGER",
    "NECK",
    "BODY",
    "HEAD",
    "LEGS",
    "FEET",
    "HANDS",
    "ARMS",
    "SHIELD",
    "ABOUT",
    "WAIST",
    "WRIST",
    "WIELD",
    "HOLD",
    "THROW",
    "LIGHT-SOURCE",
    "FACE",
    "EAR",
    "\n"
};

char *zone_modes[] =
{
    "Don't_Update",
    "RepopIfEmpty",
    "Always_Repop",
    "\n"
};

char *zone_bits[] =
{
    "NO_TELEPORT",
    "IS_TOWN(keep out STAY_NO_TOWN mobs)",
    "MODIFIED",
    "UNUSED",
    "BPORT",
    "NOCLAIM",
    "\n"
};

// new obj flags
char *extra_bits[] =
{
    "GLOW",
    "HUM",
    "DARK",
    "LOCK",
    "ANY_CLASS",
    "INVISIBLE",
    "MAGICAL",
    "NODROP",
    "BLESS",
    "ANTI-GOOD",
    "ANTI-EVIL",
    "ANTI-NEUTRAL",
    "WARRIOR",
    "MAGE",
    "THIEF",
    "CLERIC",
    "PALADIN",
    "ANTI_PAL",
    "BARBARIAN",
    "MONK",
    "RANGER",
    "DRUID",
    "BARD",
    "TWO-HANDED",
    "ENCHANTED",
    "SPECIAL_GOD_ITEM",
    "NO_SAVE",
    "NO_SEE",
    "NO_REPAIR",
    "NEWBIE_ITEM",
    "PC_CORPSE",
   // "NPC_CORPSE",
    "\n"
};

// more_flags obj flags

char *more_obj_bits[] =
{
    "NO_RESTRING",
    "UNUSED",
    "UNIQUE",
    "NO_TRADE",
    "NO_NOTICE",
    "NO_LOCATE",
    "UNIQUE_SAVE",
    "NPC_CORPSE",
    "\n"
};

char *size_bitfields[] =
{
    "ANY",
    "SMALL",
    "MEDIUM",
    "LARGE",
    "CHARMIE",
    "\n"
};

char *size_bits[] =
{
    "Any race",
    "Small races",
    "Medium races",
    "Large races",
    "Charmies",
    "\n"
};

char *room_bits[] =
{
    "dark",
    "nohome",
    "no_mob",
    "indoors",
    "teleport_block",
    "noki",
    "nolearn",
    "no_magic",
    "tunnel",
    "private",
    "safe",
    "no_summon",
    "unused",
    "no_portal",
    "imp_only",
    "fall_down",
    "arena",
    "quiet",
    "unstable",
    "no_quit",
    "fall_up",
    "fall_east",
    "fall_west",
    "fall_south",
    "fall_north",
    "no_teleport",
    "no_track",
    "clan_room",
    "no_scan",
    "no_where",
    "light",
    "\n"
};

char *exit_bits[] =
{
    "IS-DOOR",
    "CLOSED",
    "LOCKED",
    "HIDDEN",
    "IMM_ONLY",
    "PICKPROOF",
    "\n"
};

char *sector_types[] =
{
    "inside",
    "city",
    "field",
    "forest",
    "hills",
    "mountains",
    "water_swim",
    "water_no_swim",
    "beach",
    "paved_road",
    "desert",
    "underwater",
    "swamp",
    "air",
    "frozen tundra",
    "arctic",
    "\n"
};

char *equipment_types[] =
{
    "Special",
    "Worn Right finger",
    "Worn Left finger",
    "First Neck",
    "Second Neck",
    "Worn Body",
    "Worn Head",
    "Worn Legs",
    "Worn Feet",
    "Worn Hands",
    "Worn Arms",
    "Worn Shield",
    "Worn About Body",
    "Worn Waist",
    "Worn Right Wrist",
    "Worn Left Wrist",
    "Wielded",
    "Second Wield",
    "Held",
    "Worn Face",
    "Worn Right Ear",
    "Worn Left Ear",
    "Worn Penis",
    "\n"
};


/* Should be in exact correlation as the AFF types -Kahn */
char *affected_bits[] =
{ // When you modify this, modify skill_aff in mob_commands
    "BLIND",
    "INVISIBLE",
    "DETECT-EVIL",
    "DETECT-INVISIBLE",
    "DETECT-MAGIC",
    "SENSE-LIFE",
    "REFLECT",
    "SANCTUARY",
    "GROUP",
    "EAS",
    "CURSE",
    "FROSTSHIELD",
    "POISON",
    "PROTECT-EVIL",
    "PARALYSIS",
    "DETECT-GOOD",
    "FIRESHIELD",
    "SLEEP",
    "TRUE-SIGHT",
    "SNEAK",
    "HIDE",
    "UNUSED",
    "CHARM",
    "RAGE",
    "SOLIDITY",
    "INFARED",
    "CANTQUIT",
    "KILLER",
    "FLYING",
    "LIGHTNING_SHIELD",
    "HASTE",
    "UNUSED",
    "SHADOWSLIP",
    "INSOMNIA",
    "FREEFLOAT",
    "FARSIGHT",
    "CAMOUFLAGE",
    "STABILITY",
    "NOT-USED",
    "GOLEM",
    "FOREST_MELD",
    "INSANE",
    "GLITTER",
    "UTILITY",
    "ALERT",
    "NO_FLEE",
    "FAMILIAR_NO_SET",
    "PROTECT_VS_GOOD",
    "POWERWIELD",
    "REGENERATION",
    "FOCUS",
    "ILLUSION",
    "KNOW_ALIGN",
    "BLACKJACK_ALERT",
    "WATERBREATHING",
    "\n"
};

/* Should be in exact correlation as the APPLY types -Kahn */

char *apply_types[] =
{
    "NONE", // 0
    "STR",
    "DEX",
    "INT",
    "WIS",
    "CON",
    "SEX",
    "CLASS",
    "LEVEL",
    "AGE",
    "CHAR_WEIGHT", // 10
    "CHAR_HEIGHT",
    "MANA",
    "HIT_POINTS",
    "MOVE",
    "GOLD",
    "EXP",
    "ARMOR",
    "HITROLL",
    "DAMROLL",
    "SAVE_VS_FIRE", // 20
    "SAVE_VS_COLD",
    "SAVE_VS_ENERGY",
    "SAVE_VS_ACID",
    "SAVE_VS_MAGIC",
    "SAVE_VS_POISON",
    "HIT -N- DAM",
    "SANCTUARY",
    "SENSE LIFE",
    "DETECT INVISIBLE",
    "INVISIBILITY", // 30
    "SNEAK",
    "INFRARED",
    "HASTE",
    "PROTECTION FROM EVIL",
    "FLY",
    "MAGIC MISSILE",
    "BLIND",
    "EARTHQUAKE",
    "CURSE",
    "COLOUR SPRAY", // 40
    "DISPEL EVIL",
    "ENERGY DRAIN",
    "FIREBALL",
    "LIGHTNING BOLT",
    "HARM",
    "POISON",
    "SLEEP",
    "FEAR",
    "DISPEL MAGIC",
    "WEAKEN", // 50
    "CAUSE LIGHT",
    "CAUSE CRITICAL",
    "PARALYZE",
    "ACID BLAST",
    "BEE STING",
    "CURE LIGHT",
    "FLAMESTRIKE",
    "HEAL SPRAY",
    "DROWN",
    "HOWL", // 60
    "SOULDRAIN",
    "SPARKS",
    "BARKSKIN",
    "RESIST FIRE",
    "RESIST COLD",
    "KI",
    "CAMOUFLAGE",
    "FARSIGHT",
    "FREEFLOAT",
    "FROSTSHIELD", // 70
    "INSOMNIA",
    "LIGHTNING SHIELD",
    "REFLECT",
    "RESIST ELECTRICITY",
    "SHADOWSLIP",
    "SOLIDITY",
    "STABILITY",
    "STAUNCHBLOOD",
    "DISPEL GOOD",
    "TELEPORT", // 80
    "CHILL TOUCH",
    "POWER HARM",
    "VAMPIRIC TOUCH",
    "LIFE LEECH",
    "METEOR SWARM",
    "ENTANGLE",
    "INSANE",
    "GLITTER DUST",
    "RESIST ACID",
    "HP REGEN",
    "MANA REGEN",
    "MOVE REGEN",
    "KI REGEN",
    "CREATE FOOD",
    "DAMAGED",
    "THIEF_POISON",
    "PROTECTION FROM GOOD",
    "MELEE MITIGATION",
    "SPELL MITIGATION",
    "SONG MITIGATION",
    "RESIST MAGIC",
    "ALL SAVES",
    "SPELLEFFECT",
    "\n"
};

char *pc_clss_types[] =
{
    "UNDEFINED",
    "Magic User",
    "Cleric",
    "Rogue",
    "Warrior",
    "AntiPaladin",
    "Paladin",
    "Barbarian",
    "Monk",
    "Ranger",
    "Bard",
    "Druid",
    "Psionic",
    "Necromancer",
    "\n"
};
char *pc_clss_types2[] =
{
    "UNDEFINED",
    "MagicUser",
    "Cleric",
    "Rogue",
    "Warrior",
    "AntiPaladin",
    "Paladin",
    "Barbarian",
    "Monk",
    "Ranger",
    "Bard",
    "Druid",
    "Psionic",
    "Necromancer",
    "\n"
};
char *pc_clss_types3[] =
{
    "UNDEFINED",
    "mage",
    "cleric",
    "rogue",
    "warrior",
    "anti-paladin",
    "paladin",
    "barbarian",
    "monk",
    "ranger",
    "bard",
    "druid",
    "psionic",
    "necromancer",
    "\n"
};



char* pc_clss_abbrev[] =
{
    "Und",
    "Mag",
    "Cle",
    "Thi",
    "War",
    "Ant",
    "Pal",
    "Bar",
    "Mon",
    "Ran",
    "Brd",
    "Drd",
    "Psi",
    "Nec",
    "\n"
};


// Following X_skills[] arrays use the following format

//struct class_skill_defines
//    char * skillname;         // name of skill
//    int16 skillnum;          // ID # of skill
//    int16 levelavailable;    // what level class can get it
//    ubyte   maximum;           // maximum value PC can train it to (1-99)
//    long   trainer;           // what mob trains them (only one currently) REMOVED 
//    char *                    // hint to who trains it REMOVED

// Begin abilities listings for each class.

// skills master 10011 - done and checked, Apoc
struct class_skill_defines g_skills[] = { // all-class skills

//   Ability Name       Ability File          Level     Max     Requisites
//   ------------       ------------          -----     ---     ----------
{    "consider",        SKILL_CONSIDER,         1,      90,     {INT,WIS} },
{    "scan",            SKILL_SCAN,             3,      90,     {CON,WIS} },
{    "switch",          SKILL_SWITCH,           5,      90,     {DEX,INT} },
{    "release",         SKILL_RELEASE,          10,     90,     {WIS,INT} },
{    "\n",              0,                      1,      0,      {0,0} }
};

// warrior 3023 guildmaster - done and checked, Apoc
struct class_skill_defines w_skills[] = { // warrior skills

//   Ability Name       Ability File            Level  Max  Requisites
//   ------------       ------------            -----  ---  ----------
{    "kick",            SKILL_KICK,               1,    80,     {DEX,STR} },
{    "bash",            SKILL_BASH,               2,    80,     {STR,CON} },
{    "redirect",        SKILL_REDIRECT,           4,    90,     {INT,CON} },
{    "rescue",          SKILL_RESCUE,             5,    70,     {WIS,INT} },
{    "double",          SKILL_SECOND_ATTACK,      7,    90,     {STR,DEX} },
{    "disarm",          SKILL_DISARM,             10,   70,     {DEX,WIS} },
{    "headbutt",        SKILL_SHOCK,              12,   85,     
{CON,WIS} },
{    "shield block",    SKILL_SHIELDBLOCK,        15,   85,     {STR,DEX} },
{    "retreat",         SKILL_RETREAT,            17,   98,     {WIS,INT} },
{    "frenzy",          SKILL_FRENZY,             18,   80,     {CON,INT} },
{    "parry",           SKILL_PARRY,              20,   90,     {DEX,WIS} },
{    "blindfighting",   SKILL_BLINDFIGHTING,      21,   80,     {INT,DEX} },
{    "triple",          SKILL_THIRD_ATTACK,       23,   90,     {STR,DEX} },
{    "hitall",          SKILL_HITALL,             25,   80,     {STR,CON} },
{    "dual wield",      SKILL_DUAL_WIELD,         28,   90,     {DEX,CON} },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS,   30,   90,     {STR,DEX} },
{    "crushing",        SKILL_CRUSHING_WEAPONS,   30,   90,     {STR,DEX} },
{    "piercing",        SKILL_PIERCEING_WEAPONS,  30,   90,     {DEX,STR} },
{    "slashing",        SKILL_SLASHING_WEAPONS,   30,   90,     {DEX,STR} },
{    "whipping",        SKILL_WHIPPING_WEAPONS,   30,   90,     {DEX,STR} },
{    "tactics",         SKILL_TACTICS,            31,   98,     {INT,WIS} },
{    "archery",         SKILL_ARCHERY,            32,   55,     {DEX,WIS} },
{    "stun",            SKILL_STUN,               35,   85,     {DEX,INT} },
{    "guard",           SKILL_GUARD,              37,   98,     {STR,WIS} },
{    "deathstroke",     SKILL_DEATHSTROKE,        39,   98,     {STR,INT} },
{    "riposte",         SKILL_RIPOSTE,            40,   98,     {INT,DEX} },
{    "two handers",     SKILL_TWO_HANDED_WEAPONS, 42,   85,     {STR,CON} },
{    "stinging",        SKILL_STINGING_WEAPONS,   43,   90,     {DEX,INT} },
{    "skewer",          SKILL_SKEWER,             45,   98,     {STR,CON} },
{    "blade shield",    SKILL_BLADESHIELD,        47,   98,     {CON,DEX} },
{    "combat mastery",  SKILL_COMBAT_MASTERY,     50,   98,     {DEX, CON} },
{    "\n",              0,                        1,    0,      {0,0} }
};

// thief 3022 guildmaster - done and checked, Apoc
struct class_skill_defines t_skills[] = { // thief skills

//   Ability Name       Ability File          Level    Max    Requisites
//   ------------       ------------          -----    ---    ----------
{    "backstab",        SKILL_BACKSTAB,         1,      90,   {DEX,STR} },  
{    "sneak",           SKILL_SNEAK,            2,      90,   {DEX,INT} },  
{    "shield block",    SKILL_SHIELDBLOCK,      4,      40,   {STR,DEX} },  
{    "stalk",           SKILL_STALK,            6,      98,   {CON,DEX} },
{    "hide",            SKILL_HIDE,             7,      90,   {INT,WIS} },  
{    "dual wield",      SKILL_DUAL_WIELD,       10,     90,   {DEX,CON} },  
{    "trip",            SKILL_TRIP,             11,     85,   {DEX,STR} },
{    "palm",            SKILL_PALM,             12,     98,   {DEX,INT} },  
{    "slip",            SKILL_SLIP,             13,     98,   {DEX,INT} },  
{    "dodge",           SKILL_DODGE,            15,     90,   {DEX,INT} },
{    "blackjack",       SKILL_BLACKJACK,        17,     98,   {WIS,STR} },
{    "pocket",          SKILL_POCKET,           20,     98,   {INT,DEX} },
{    "appraise",        SKILL_APPRAISE,         21,     98,   {INT,WIS} },
{    "pick",            SKILL_PICK_LOCK,        22,     98,   {WIS,DEX} },  
{    "steal",           SKILL_STEAL,            25,     98,   {DEX,WIS} },
{    "blindfighting",   SKILL_BLINDFIGHTING,    28,     80,   {INT,DEX} },
{    "piercing",        SKILL_PIERCEING_WEAPONS,30,     90,   {DEX,STR} },
{    "slashing",        SKILL_SLASHING_WEAPONS, 30,     55,   {DEX,STR} },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS, 30,     70,   {STR,DEX} },
{    "stinging",        SKILL_STINGING_WEAPONS, 30,     85,   {DEX,INT} },  
{    "deceit",          SKILL_DECEIT,           31,     98,   {WIS,INT} },  
{    "circle",          SKILL_CIRCLE,           35,     98,   {STR,DEX} },  
{    "disarm",          SKILL_DISARM,           38,     85,   {DEX,WIS} },  
{    "dual backstab",   SKILL_DUAL_BACKSTAB,    40,     98,   {DEX,INT} },  
{    "eyegouge",        SKILL_EYEGOUGE,         42,     98,   {STR,CON} },  
{    "vitalstrike",     SKILL_VITAL_STRIKE,     45,     98,   {CON,DEX} },  
{    "\n",              0,                      1,      0,    {0,0} }
};

// anti-paladin 10005 guildmaster - done and checked, Apoc
struct class_skill_defines a_skills[] = { // anti-paladin skills

//   Ability Name            Ability File           Level     Max     Requisites
//   ------------            ------------           -----     ---     ----------
{    "harmtouch",            SKILL_HARM_TOUCH,        1,      98,     {STR,CON} },
{    "kick",                 SKILL_KICK,              2,      70,     {DEX,STR} },
{    "sneak",                SKILL_SNEAK,             3,      85,     {DEX,INT} },
{    "shield block",         SKILL_SHIELDBLOCK,       5,      60,     {STR,DEX} },
{    "infravision",          SPELL_INFRAVISION,       7,      90,     {INT,DEX} },
{    "detect good",          SPELL_DETECT_GOOD,       8,      98,     {WIS,INT} },
{    "dual wield",           SKILL_DUAL_WIELD,        10,     85,     {DEX,CON} },
{    "shocking grasp",       SPELL_SHOCKING_GRASP,    11,     98,     {INT,DEX} },
{    "detect invisibility",  SPELL_DETECT_INVISIBLE,  12,     85,     {INT,DEX} },
{    "invisibility",         SPELL_INVISIBLE,         13,     90,     {INT,DEX} },
{    "backstab",             SKILL_BACKSTAB,          15,     90,     {DEX,STR} },
{    "hide",                 SKILL_HIDE,              17,     85,     {INT,WIS} },
{    "trip",                 SKILL_TRIP,              19,     85,     {DEX,STR} },
{    "chill touch",          SPELL_CHILL_TOUCH,       20,     85,     {CON,WIS} },
{    "double",               SKILL_SECOND_ATTACK,     22,     85,     {STR,DEX} },
{    "dodge",                SKILL_DODGE,             23,     80,     {DEX,INT} },
{    "vampiric touch",       SPELL_VAMPIRIC_TOUCH,    25,     98,     {CON,INT} },
{    "poison",               SPELL_POISON,            27,     70,     {CON,WIS} },
{    "animate dead",         SPELL_ANIMATE_DEAD,      28,     80,     {STR,WIS} },
{    "dismiss corpse",       SPELL_DISMISS_CORPSE,    29,     80,     {CON,WIS} },
{    "piercing",             SKILL_PIERCEING_WEAPONS, 30,     85,     {DEX,STR} },
{    "slashing",             SKILL_SLASHING_WEAPONS,  30,     85,     {DEX,STR} },
{    "crushing",             SKILL_CRUSHING_WEAPONS,  30,     85,     {STR,DEX} },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,  30,     85,     {STR,DEX} },
{    "visage of hate",       SPELL_VISAGE_OF_HATE,    31,     98,     {INT,WIS} },
{    "blindfighting",        SKILL_BLINDFIGHTING,     32,     80,     {INT,DEX} },
{    "globe of darkness",    SPELL_GLOBE_OF_DARKNESS, 33,     98,     {INT,DEX} },
{    "beacon",               SPELL_BEACON,            35,     98,     {DEX,INT} },
{    "fear",                 SPELL_FEAR,              37,     90,     {WIS,INT} },
{    "dispel good",          SPELL_DISPEL_GOOD,       38,     90,     {WIS,STR} },
{    "acid shield",          SPELL_ACID_SHIELD,       40,     98,     {INT,STR} },
{    "curse",                SPELL_CURSE,             41,     70,     {WIS,INT} },
{    "firestorm",            SPELL_FIRESTORM,         42,     85,     {INT,STR} },
{    "unholy aegis",         SPELL_U_AEGIS,           44,     90,     {WIS,INT} },
{    "protection from good", SPELL_PROTECT_FROM_GOOD, 45,     90,     {WIS,DEX} },
{    "resist acid",          SPELL_RESIST_ACID,       46,     70,     {CON,INT} },
{    "acid blast",           SPELL_ACID_BLAST,        48,     98,     {STR,INT} },
{    "vampiric aura",        SPELL_VAMPIRIC_AURA,     50,     98,     {INT,CON} },
{    "\n",                   0,                       1,      0,      {0,0} }
};

// paladin 10006 guildmaster - done and checked, Apoc
struct class_skill_defines p_skills[] = { // paladin skills

//   Ability Name            Ability File            Level     Max     Requisites 
//   ------------            ------------            -----     ---     ---------- 
{    "layhands",             SKILL_LAY_HANDS,          1,      98,     {CON,WIS} },
{    "kick",                 SKILL_KICK,               2,      70,     {DEX,STR} },
{    "bless",                SPELL_BLESS,              3,      90,     {WIS,CON} },
{    "double",               SKILL_SECOND_ATTACK,      5,      85,     {STR,DEX} },
{    "shield block",         SKILL_SHIELDBLOCK,        7,      90,     {STR,DEX} },
{    "rescue",               SKILL_RESCUE,             8,      85,     {WIS,INT} },
{    "cure light",           SPELL_CURE_LIGHT,         9,      85,     {WIS,INT} },
{    "dual wield",           SKILL_DUAL_WIELD,         10,     80,     {DEX,CON} },
{    "detect poison",        SPELL_DETECT_POISON,      11,     90,     {INT,WIS} },
{    "create food",          SPELL_CREATE_FOOD,        12,     85,     {INT,CON} },
{    "create water",         SPELL_CREATE_WATER,       13,     85,     {INT,CON} },
{    "cure serious",         SPELL_CURE_SERIOUS,       15,     85,     {WIS,INT} },
{    "detect evil",          SPELL_DETECT_EVIL,        17,     98,     {INT,WIS} },
{    "remove poison",        SPELL_REMOVE_POISON,      18,     90,     {CON,WIS} },
{    "detect invisibility",  SPELL_DETECT_INVISIBLE,   20,     85,     {INT,DEX} },
{    "cure critic",          SPELL_CURE_CRITIC,        22,     85,     {WIS,INT} },
{    "parry",                SKILL_PARRY,              23,     70,     {DEX,WIS} },
{    "bash",                 SKILL_BASH,               25,     85,     {STR,CON} },
{    "sense life",           SPELL_SENSE_LIFE,         26,     85,     {CON,INT} },
{    "strength",             SPELL_STRENGTH,           28,     70,     {STR,CON} },
{    "earthquake",           SPELL_EARTHQUAKE,         29,     70,     {DEX,WIS} },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,   30,     85,     {STR,DEX} },
{    "slashing",             SKILL_SLASHING_WEAPONS,   30,     85,     {DEX,STR} },
{    "crushing",             SKILL_CRUSHING_WEAPONS,   30,     85,     {STR,DEX} },
{    "blessed halo",         SPELL_BLESSED_HALO,       31,     98,     {WIS,INT} },
{    "triple",               SKILL_THIRD_ATTACK,       33,     80,     {STR,DEX} },
{    "two handers",          SKILL_TWO_HANDED_WEAPONS, 35,     75,     {STR,CON} },
{    "heal",                 SPELL_HEAL,               37,     85,     {WIS,INT} },
{    "harm",                 SPELL_HARM,               38,     85,     {WIS,CON} },
{    "sanctuary",            SPELL_SANCTUARY,          40,     90,     {WIS,INT} },
{    "holy aegis",           SPELL_AEGIS,              42,     90,     {WIS,INT} },
{    "dispel evil",          SPELL_DISPEL_EVIL,        43,     90,     {WIS,STR} },
{    "protection from evil", SPELL_PROTECT_FROM_EVIL,  45,     90,     {WIS,DEX} },
{    "resist cold",          SPELL_RESIST_COLD,        46,     70,     {CON,STR} },
{    "divine fury",          SPELL_DIVINE_FURY,        48,     98,     {STR,CON} },
{    "holy aura",            SPELL_HOLY_AURA,          50,     98,     {WIS,STR} },
{    "\n",                   0,                        1,      0,      {0,0} }
};

// barbarian 10007 guildmaster - done and checked, Apoc
struct class_skill_defines b_skills[] = { // barbarian skills

//   Ability Name       Ability File            Level  Max   Requisites
//   ------------       ------------            -----  ---   ----------
{    "dual wield",      SKILL_DUAL_WIELD,         1,    90,  {DEX,CON} },
{    "bash",            SKILL_BASH,               2,    90,  {STR,CON} },
{    "kick",            SKILL_KICK,               3,    80,  {DEX,STR} },
{    "parry",           SKILL_PARRY,              5,    70,  {DEX,WIS} },
{    "double",          SKILL_SECOND_ATTACK,      8,    85,  {STR,DEX} },
{    "dodge",           SKILL_DODGE,  	          10,   70,  {DEX,INT} },
{    "blood fury",      SKILL_BLOOD_FURY,         12,   98,  {CON,WIS} },
{    "crazedassault",   SKILL_CRAZED_ASSAULT,     15,   98,  {WIS,STR} },
{    "frenzy",          SKILL_FRENZY,             18,   90,  {CON,INT} },
{    "rage",            SKILL_RAGE,               20,   98,  {CON,STR} },
{    "triple",          SKILL_THIRD_ATTACK,       25,   85,  {STR,DEX} },
{    "battlecry",       SKILL_BATTLECRY,          27,   98,  {WIS,INT} },
{    "blindfighting",   SKILL_BLINDFIGHTING,      28,   65,  {INT,DEX} },
{    "whipping",        SKILL_WHIPPING_WEAPONS,   30,   85,  {DEX,STR} },
{    "piercing",        SKILL_PIERCEING_WEAPONS,  30,   85,  {DEX,STR} },
{    "slashing",        SKILL_SLASHING_WEAPONS,   30,   85,  {DEX,STR} },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS,   30,   85,  {STR,DEX} },
{    "crushing",        SKILL_CRUSHING_WEAPONS,   30,   85,  {STR,DEX} },
{    "ferocity",        SKILL_FEROCITY,           31,   98,  {INT,STR} },
{    "headbutt",        SKILL_SHOCK,              33,   90,  {CON,WIS} },
{    "two handers",     SKILL_TWO_HANDED_WEAPONS, 35,   90,  {STR,CON} },
{    "archery",         SKILL_ARCHERY,            38,   80,  {DEX,WIS} },
{    "berserk",         SKILL_BERSERK,            40,   98,  {STR,CON} },
{    "hitall",          SKILL_HITALL,             45,   90,  {STR,CON} },
{    "magic resistance", SKILL_MAGIC_RESIST,      47,   98,  {INT,CON} },
{    "knockback",       SKILL_KNOCKBACK,          48,   98,  {STR,DEX} },
{    "bullrush",        SKILL_BULLRUSH,           50,   98,  {STR,CON} },
{    "\n",              0,                        1,    0,   {0,0} }
};

// monk 10008 guildmaster - done and checked, Apoc
struct class_skill_defines k_skills[] = { // monk skills

//   Ability Name       Ability File          Level    Max      Requisites
//   ------------       ------------          -----    ---      ----------
{    "kick",            SKILL_KICK,             1,      85,     {DEX,STR} },
{    "dodge",           SKILL_DODGE,            2,      85,     {DEX,INT} },
{    "redirect",        SKILL_REDIRECT,         3,      85,     {INT,CON} },
{    "trip",            SKILL_TRIP,             5,      70,     {DEX,STR} },
{    "purify",          KI_PURIFY+KI_OFFSET,    8,      98,     {CON,WIS} },
{    "martial defense", SKILL_DEFENSE,          10,     80,     {STR,DEX} },
{    "rescue",          SKILL_RESCUE,           12,     75,     {WIS,INT} },
{    "punch",           KI_PUNCH+KI_OFFSET,     15,     98,     {STR,DEX} },
{    "eagleclaw",       SKILL_EAGLE_CLAW,       17,     98,     {STR,DEX} },
{    "dual wield",      SKILL_DUAL_WIELD,       20,     55,     {DEX,CON} },
{    "sense",           KI_SENSE+KI_OFFSET,     21,     98,     {INT,WIS} },
{    "stance",          KI_STANCE+KI_OFFSET,    24,     98,     {CON,DEX} },
{    "speed",           KI_SPEED+KI_OFFSET,     27,     98,     {DEX,INT} },
{    "whipping",        SKILL_WHIPPING_WEAPONS, 30,     60,     {DEX,STR} },
{    "hand to hand",    SKILL_HAND_TO_HAND,     30,     98,     {DEX,STR} },
{    "agility",         KI_AGILITY+KI_OFFSET,   31,     98,     {DEX,CON} },
{    "stun",            SKILL_STUN,             33,     90,     {DEX,INT} },
{    "storm",           KI_STORM+KI_OFFSET,     35,     98,     {CON,WIS} },
{    "blindfighting",   SKILL_BLINDFIGHTING,    38,     85,     {INT,DEX} },
{    "quiver",          SKILL_QUIVERING_PALM,   40,     98,     {STR,INT} },
{    "blast",           KI_BLAST+KI_OFFSET,     45,     98,     {WIS,STR} },
{    "disrupt",         KI_DISRUPT+KI_OFFSET,   47,     98,     {INT,DEX} },
{    "\n",              0,                      1,      0,      {0,0} }
};

// ranger 10013 guildmaster - done and checked, Apoc
struct class_skill_defines r_skills[] = { // ranger skills

//   Ability Name       Ability File           Level     Max     Requisites 
//   ------------       ------------           -----     ---     ---------- 
{    "bee sting",       SPELL_BEE_STING,         1,      98,     {CON,WIS} },
{    "hide",            SKILL_HIDE,              2,      80,     {INT,WIS} },
{    "kick",            SKILL_KICK,              3,      70,     {DEX,STR} },
{    "dual wield",      SKILL_DUAL_WIELD,        5,      90,     {DEX,CON} },
{    "redirect",        SKILL_REDIRECT,          7,      80,     {INT,CON} },
{    "eyes of the owl", SPELL_EYES_OF_THE_OWL,   8,      90,     {INT,DEX} },
{    "sense life",      SPELL_SENSE_LIFE,        9,      90,     {CON,INT} },
{    "shield block",    SKILL_SHIELDBLOCK,       10,     60,     {STR,DEX} },
{    "tame",            SKILL_TAME,              11,     98,     {WIS,INT} },
{    "double",          SKILL_SECOND_ATTACK,     12,     85,     {STR,DEX} },
{    "feline agility",  SPELL_FELINE_AGILITY,    14,     98,     {DEX,INT} },
{    "bee swarm",       SPELL_BEE_SWARM,         15,     98,     {CON,WIS} },
{    "forage",          SKILL_FORAGE,            16,     90,     {INT,CON} },
{    "entangle",        SPELL_ENTANGLE,          18,     90,     {WIS,DEX} },
{    "archery",         SKILL_ARCHERY,           20,     90,     {DEX,WIS} },
{    "blindfighting",   SKILL_BLINDFIGHTING,     21,     90,     {INT,DEX} },
{    "parry",           SKILL_PARRY,             22,     80,     {DEX,WIS} },
{    "herb lore",       SPELL_HERB_LORE,         23,     90,     {INT,WIS} },
{    "poison",          SPELL_POISON,            25,     85,     {CON,WIS} },
{    "tempest arrows",  SKILL_TEMPEST_ARROW,     26,     98,     {INT,DEX} },
{    "track",           SKILL_TRACK,             28,     98,     {WIS,INT} },
{    "barkskin",        SPELL_BARKSKIN,          29,     90,     {CON,DEX} },
{    "piercing",        SKILL_PIERCEING_WEAPONS, 30,     85,     {DEX,STR} },
{    "slashing",        SKILL_SLASHING_WEAPONS,  30,     85,     {DEX,STR} },
{    "whipping",        SKILL_WHIPPING_WEAPONS,  30,     85,     {DEX,STR} },
{    "ice arrows",      SKILL_ICE_ARROW,         31,     98,     {CON,DEX} },
{    "rescue",          SKILL_RESCUE,            32,     80,     {WIS,INT} },
{    "trip",            SKILL_TRIP,              33,     80,     {DEX,STR} },
{    "ambush",          SKILL_AMBUSH,            35,     98,     {INT,DEX} },
{    "fire arrows",     SKILL_FIRE_ARROW,        36,     98,     {WIS,DEX} },
{    "call follower",   SPELL_CALL_FOLLOWER,     38,     98,     {CON,STR} },
{    "stun",            SKILL_STUN,              40,     80,     {DEX,INT} },
{    "granite arrows",  SKILL_GRANITE_ARROW,     41,     98,     {STR,DEX} },
{    "disarm",          SKILL_DISARM,            42,     70,     {DEX,WIS} },
{    "staunchblood",    SPELL_STAUNCHBLOOD,      44,     70,     {CON,WIS} },
{    "forest meld",     SPELL_FOREST_MELD,       45,     90,     {WIS,DEX} },
{    "camouflage",      SPELL_CAMOUFLAGE,        46,     90,     {INT,DEX} },
{    "creeping death",  SPELL_CREEPING_DEATH,    48,     98,     {WIS,STR} },
{      "\n",            0,                       1,      0,      {0,0} }
};

// bard 3204 guildmaster - done and checked, Apoc
struct class_skill_defines d_skills[] = { // bard skills

// Ability Name            Ability File                  Level    Max      Requisites
// ------------            ------------                  -----    ---      ----------
{ "whistle sharp",         SKILL_SONG_WHISTLE_SHARP,       1,      98,     {STR,INT} },
{ "stop",                  SKILL_SONG_STOP,                2,      98,     {INT,WIS} },
{ "irresistable ditty",    SKILL_SONG_UNRESIST_DITTY,      3,      98,     {DEX,WIS} },
{ "dodge",                 SKILL_DODGE,                    5,      60,     {DEX,INT} },
{ "hide",                  SKILL_HIDE,                     7,      70,     {INT,WIS} },
{ "travelling march",      SKILL_SONG_TRAVELING_MARCH,     9,      98,     {DEX,CON} },
{ "trip",                  SKILL_TRIP,                     10,     70,     {DEX,STR} },
{ "bountiful sonnet",      SKILL_SONG_BOUNT_SONNET,        12,     98,     {CON,WIS} },
{ "healing melody",        SKILL_SONG_HEALING_MELODY,      13,     98,     {WIS,CON} },
{ "glitter dust",          SKILL_SONG_GLITTER_DUST,        15,     98,     {INT,DEX} },
{ "synchronous chord",     SKILL_SONG_SYNC_CHORD,          17,     98,     {INT,STR} },
{ "sticky lullaby",        SKILL_SONG_STICKY_LULL,         18,     98,     {STR,WIS} },
{ "flight of the bumblebee", SKILL_SONG_FLIGHT_OF_BEE,     20,     98,     {DEX,INT} },
{ "note of knowledge",     SKILL_SONG_NOTE_OF_KNOWLEDGE,   21,     98,     {INT,WIS} },
{ "fanatical fanfare",     SKILL_SONG_FANATICAL_FANFARE,   23,     98,     {CON,INT} },
{ "revealing staccato",    SKILL_SONG_REVEAL_STACATO,      25,     98,     {INT,DEX} },
{ "double",                SKILL_SECOND_ATTACK,            26,     80,     {STR,DEX} },
{ "terrible clef",         SKILL_SONG_TERRIBLE_CLEF,       28,     98,     {INT,STR} },
{ "piercing",              SKILL_PIERCEING_WEAPONS,        30,     70,     {DEX,STR} },
{ "slashing",              SKILL_SLASHING_WEAPONS,         30,     70,     {DEX,STR} },
{ "stinging",              SKILL_STINGING_WEAPONS,         30,     85,     {DEX,INT} },
{ "whipping",              SKILL_WHIPPING_WEAPONS,         30,     85,     {STR,DEX} },
{ "soothing rememberance", SKILL_SONG_SOOTHING_REMEM,      31,     98,     {INT,WIS} },
{ "searching song",        SKILL_SONG_SEARCHING_SONG,      32,     98,     {INT,DEX} },
{ "dischordant dirge",     SKILL_SONG_DISCHORDANT_DIRGE,   34,     98,     {WIS,CON} },
{ "insane chant",          SKILL_SONG_INSANE_CHANT,        35,     98,     {WIS,INT} },
{ "jig of alacrity",       SKILL_SONG_JIG_OF_ALACRITY,     38,     98,     {DEX,CON} },
{ "vigilant siren",        SKILL_SONG_VIGILANT_SIREN,      40,     98,     {INT,CON} },
{ "forgetful rhythm",      SKILL_SONG_FORGETFUL_RHYTHM,    42,     98,     {INT,STR} },
{ "disarming limerick",    SKILL_SONG_DISARMING_LIMERICK,  43,     98,     {CON,INT} },
{ "astral chanty",         SKILL_SONG_ASTRAL_CHANTY,       45,     98,     {STR,DEX} },
{ "crushing crescendo",    SKILL_SONG_CRUSHING_CRESCENDO,  46,     98,     {CON,STR} },
{ "shattering resonance",  SKILL_SONG_SHATTERING_RESO,     48,     98,     {STR,CON} },
{ "mountain king's charge",SKILL_SONG_MKING_CHARGE,        49,     98,     {STR,CON} },
{ "hypnotic harmony",      SKILL_SONG_HYPNOTIC_HARMONY,    50,     98,     {WIS,INT} },
{ "\n",                    0,                              1,      0,      {0,0} }
};


// druid 3203 guildmaster - done and checked, Apoc
struct class_skill_defines u_skills[] = { // druid skills

//   Ability Name            Ability File              Level     Max     Requisites
//   ------------            ------------              -----     ---     ----------
{    "blue bird",            SPELL_BLUE_BIRD,            1,      98,     {WIS,DEX} },  
{    "eyes of the owl",      SPELL_EYES_OF_THE_OWL,      2,      85,     {INT,DEX} },  
{    "cure light",           SPELL_CURE_LIGHT,           3,      85,     {WIS,INT} },  
{    "create water",         SPELL_CREATE_WATER,         5,      90,     {INT,CON} },  
{    "attrition",            SPELL_ATTRITION,            6,      98,     {CON,DEX} },  
{    "natures lore",         SKILL_NATURES_LORE,         7,      98,     {DEX,WIS} },
{    "create food",          SPELL_CREATE_FOOD,          8,      90,     {INT,CON} },  
{    "sense life",           SPELL_SENSE_LIFE,           10,     85,     {CON,INT} },  
{    "weaken",               SPELL_WEAKEN,               11,     98,     {STR,CON} },  
{    "cure serious",         SPELL_CURE_SERIOUS,         12,     85,     {WIS,INT} },
{    "resist cold",          SPELL_RESIST_COLD,          13,     90,     {CON,STR} },
{    "camouflage",           SPELL_CAMOUFLAGE,           14,     85,     {INT,DEX} },
{    "oaken fortitude",      SPELL_OAKEN_FORTITUDE,      15,     98,     {CON,STR} },
{    "water breathing",      SPELL_WATER_BREATHING,      17,     98,     {DEX,INT} },
{    "resist acid",          SPELL_RESIST_ACID,          18,     90,     {CON,INT} },
{    "stoneshield",          SPELL_STONE_SHIELD,         20,     98,     {STR,WIS} },
{    "poison",               SPELL_POISON,               21,     90,     {CON,WIS} },
{    "staunchblood",         SPELL_STAUNCHBLOOD,         22,     90,     {CON,WIS} },
{    "cure critic",          SPELL_CURE_CRITIC,          23,     85,     {WIS,INT} },
{    "call familiar",        SPELL_SUMMON_FAMILIAR,      25,     90,     {INT,STR} },
{    "dismiss familiar",     SPELL_DISMISS_FAMILIAR,     26,     90,     {WIS,CON} },  
{    "debility",             SPELL_DEBILITY,             27,     98,     {DEX,CON} },  
{    "drown",                SPELL_DROWN,                28,     98,     {INT,CON} },  
{    "entangle",             SPELL_ENTANGLE,             29,     90,     {WIS,DEX} },  
{    "whipping",             SKILL_WHIPPING_WEAPONS,     30,     70,     {DEX,STR} },  
{    "crushing",             SKILL_CRUSHING_WEAPONS,     30,     70,     {STR,DEX} },  
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,     30,     70,     {STR,DEX} },  
{    "resist energy",        SPELL_RESIST_ENERGY,        30,     90,     {CON,DEX} },
{    "rapid mend",           SPELL_RAPID_MEND,           31,     98,     {WIS,INT} },  
{    "herb lore",            SPELL_HERB_LORE,            32,     90,     {INT,WIS} },
{    "lighted path",         SPELL_LIGHTED_PATH,         33,     98,     {WIS,DEX} },
{    "curse",                SPELL_CURSE,                34,     90,     {WIS,INT} },
{    "sun ray",              SPELL_SUN_RAY,              35,     98,     {INT,CON} },
{    "call lightning",       SPELL_CALL_LIGHTNING,    35,     98,     {INT,CON} },     
{    "control weather",      SPELL_CONTROL_WEATHER,      36,     90,     {CON,WIS} },
{    "barkskin",             SPELL_BARKSKIN,             37,     85,     {CON,DEX} },
{    "iron roots",           SPELL_IRON_ROOTS,           38,     98,     {STR,DEX} },
{    "resist fire",          SPELL_RESIST_FIRE,          39,     90,     {INT,CON} },
{    "earthquake",           SPELL_EARTHQUAKE,           40,     90,     {DEX,WIS} },
{    "lightning shield",     SPELL_LIGHTNING_SHIELD,     41,     98,     {WIS,INT} },
{    "blindness",            SPELL_BLINDNESS,            42,     98,     {CON,WIS} },  
{    "forage",               SKILL_FORAGE,               43,     90,     {INT,CON} },  
{    "spiritwalk",            SPELL_GHOSTWALK,            43,     98,     {WIS,INT} },
{    "stoneskin",            SPELL_STONE_SKIN,           44,     70,     {STR,CON} },  
{    "power heal",           SPELL_POWER_HEAL,           45,     98,     {WIS,STR} },  
{    "forest meld",          SPELL_FOREST_MELD,          46,     90,     {WIS,DEX} },  
{    "greater stoneshield",  SPELL_GREATER_STONE_SHIELD, 47,     98,     {STR,WIS} },  
{    "colour spray",         SPELL_COLOUR_SPRAY,         48,     98,     {WIS,INT} },  
{    "summon",               SPELL_SUMMON,               49,     98,     {INT,STR} },  
//{ "conjure elemental",     SPELL_CONJURE_ELEMENTAL,    50,     98,     {0,0} },
{    "\n",                   0,                          1,      0,      {0,0} }
};


// cleric 3021 guildmaster - done and checked, Apoc
struct class_skill_defines c_skills[] = { // cleric skills

//   Ability Name            Ability File           Level     Max     Requisites
//   ------------            ------------           -----     ---     ----------
{    "cure light",           SPELL_CURE_LIGHT,        1,      90,     {WIS,INT} },     
{    "cause light",          SPELL_CAUSE_LIGHT,       2,      98,     {STR,WIS} },     
{    "armor",                SPELL_ARMOR,             3,      90,     {STR,INT} },     
{    "continual light",      SPELL_CONT_LIGHT,        4,      85,     {INT,WIS} },     
{    "detect poison",        SPELL_DETECT_POISON,     5,      90,     {INT,WIS} },     
{    "know alignment",       SPELL_KNOW_ALIGNMENT,    5,      90,     {WIS,INT} },
{    "detect magic",         SPELL_DETECT_MAGIC,      6,      85,     {INT,WIS} },     
{    "refresh",              SPELL_REFRESH,           7,      85,     {DEX,CON} },     
{    "bless",                SPELL_BLESS,             8,      90,     {WIS,CON} },
{    "create water",         SPELL_CREATE_WATER,      9,      70,     {INT,CON} },
{    "create food",          SPELL_CREATE_FOOD,       9,      70,     {INT,CON} },
{    "cure serious",         SPELL_CURE_SERIOUS,      10,     90,     {WIS,INT} },
{    "cause serious",        SPELL_CAUSE_SERIOUS,     11,     98,     {STR,WIS} },
{    "detect invisibility",  SPELL_DETECT_INVISIBLE,  12,     85,     {INT,DEX} },
{    "remove poison",        SPELL_REMOVE_POISON,     13,     90,     {CON,WIS} },
{    "dispel minor",         SPELL_DISPEL_MINOR,      14,     85,     {INT,CON} },
{    "dual wield",           SKILL_DUAL_WIELD,        15,     55,     {DEX,CON} },
{    "remove blind",         SPELL_REMOVE_BLIND,      16,     98,     {INT,WIS} },
{    "sense life",           SPELL_SENSE_LIFE,        17,     85,     {CON,INT} },     
{    "sanctuary",            SPELL_SANCTUARY,         18,     90,     {WIS,INT} },     
{    "remove curse",         SPELL_REMOVE_CURSE,      19,     98,     {INT,WIS} },     
{    "cure critical",        SPELL_CURE_CRITIC,       20,     90,     {WIS,INT} },     
{    "cause critical",       SPELL_CAUSE_CRITICAL,    21,     98,     {STR,WIS} },     
{    "remove paralysis",     SPELL_REMOVE_PARALYSIS,  22,     98,     {INT,DEX} },     
{    "locate object",        SPELL_LOCATE_OBJECT,     23,     80,     {INT,WIS} },     
{    "word of recall",       SPELL_WORD_OF_RECALL,    24,     90,     {STR,WIS} },
{    "animate dead",         SPELL_ANIMATE_DEAD,      25,     90,     {STR,WIS} },
{    "dismiss corpse",       SPELL_DISMISS_CORPSE,    26,     90,     {CON,WIS} },
{    "group fly",            SPELL_GROUP_FLY,         27,     98,     {CON,DEX} },
{    "heal",                 SPELL_HEAL,              28,     90,     {WIS,INT} },
{    "harm",                 SPELL_HARM,              29,     90,     {WIS,CON} },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,  30,     80,     {STR,DEX} },
{    "crushing",             SKILL_CRUSHING_WEAPONS,  30,     80,     {STR,DEX} },
{    "heroes feast",         SPELL_HEROES_FEAST,      31,     98,     {WIS,CON} },
{    "dispel evil",          SPELL_DISPEL_EVIL,       32,     90,     {STR,INT} },
{    "dispel good",          SPELL_DISPEL_GOOD,       33,     90,     {STR,INT} },     
{    "iridescent aura",      SPELL_IRIDESCENT_AURA,   35,     98,     {WIS,INT} },
{    "protection from evil", SPELL_PROTECT_FROM_EVIL, 36,     90,     {WIS,DEX} },     
{    "protection from good", SPELL_PROTECT_FROM_GOOD, 37,     90,     {WIS,DEX} },     
{    "portal",               SPELL_PORTAL,            38,     85,     {STR,INT} },     
{    "true sight",           SPELL_TRUE_SIGHT,        39,     90,     {WIS,INT} },     
{    "full heal",            SPELL_FULL_HEAL,         40,     98,     {WIS,INT} },
{    "power harm",           SPELL_POWER_HARM,        41,     98,     {STR,CON} },
{    "resist magic",         SPELL_RESIST_MAGIC,      43,     90,     {INT,WIS} },
{    "resist energy",        SPELL_RESIST_ENERGY,     44,     85,     {CON,DEX} },
{    "dispel magic",         SPELL_DISPEL_MAGIC,      45,     85,     {INT,CON} },
{    "flamestrike",          SPELL_FLAMESTRIKE,       46,     98,     {STR,WIS} },
{    "group recall",         SPELL_GROUP_RECALL,      47,     98,     {DEX,STR} },
{    "heal spray",           SPELL_HEAL_SPRAY,        48,     98,     {WIS,CON} },
{    "group sanctuary",      SPELL_GROUP_SANC,        49,     98,     {STR,WIS} },
{    "\n",                         0,                         1,      0,      {0,0} }  
};

// mage 3020 guildmaster - done and checked, Apoc
struct class_skill_defines m_skills[] = { // mage skills

//   Ability Name           Ability File           Level     Max     Requisites   
//   ------------           ------------           -----     ---     ----------   
{    "magic missile",       SPELL_MAGIC_MISSILE,     1,      98,     {INT,STR} }, 
{    "ventriloquate",       SPELL_VENTRILOQUATE,     2,      98,     {WIS,INT} }, 
{    "detect magic",        SPELL_DETECT_MAGIC,      3,      90,     {INT,WIS} }, 
{    "detect invisibility", SPELL_DETECT_INVISIBLE,  4,      90,     {INT,DEX} }, 
{    "invisibility",        SPELL_INVISIBLE,         5,      90,     {INT,DEX} }, 
{    "burning hands",       SPELL_BURNING_HANDS,     6,      98,     {INT,DEX} }, 
{    "armor",               SPELL_ARMOR,             7,      80,     {STR,INT} }, 
{    "continual light",     SPELL_CONT_LIGHT,        8,      80,     {INT,WIS} }, 
{    "refresh",             SPELL_REFRESH,           9,      90,     {DEX,CON} },
{    "lightning bolt",      SPELL_LIGHTNING_BOLT,    10,     98,     {INT,STR} },
{    "infravision",         SPELL_INFRAVISION,       11,     90,     {INT,DEX} },
{    "fly",                 SPELL_FLY,               12,     98,     {DEX,CON} },
{    "strength",            SPELL_STRENGTH,          13,     85,     {STR,CON} },
{    "fear",                SPELL_FEAR,              15,     90,     {WIS,INT} },
{    "identify",            SPELL_IDENTIFY,          16,     98,     {INT,WIS} }, 
{    "locate object",       SPELL_LOCATE_OBJECT,     17,     90,     {INT,WIS} }, 
{    "call familiar",       SPELL_SUMMON_FAMILIAR,   18,     90,     {INT,STR} }, 
{    "dismiss familiar",    SPELL_DISMISS_FAMILIAR,  18,     90,     {WIS,CON} }, 
{    "chill touch",         SPELL_CHILL_TOUCH,       20,     90,     {CON,WIS} }, 
{    "shield",              SPELL_SHIELD,            21,     98,     {WIS,STR} }, 
{    "souldrain",           SPELL_SOULDRAIN,         22,     98,     {WIS,CON} }, 
{    "dispel minor",        SPELL_DISPEL_MINOR,      25,     90,     {WIS,CON} }, 
{    "mass invisibility",   SPELL_MASS_INVISIBILITY, 26,     98,     {DEX,INT} }, 
{    "life leech",          SPELL_LIFE_LEECH,        27,     98,     {CON,STR} },
{    "portal",              SPELL_PORTAL,            28,     90,     {STR,INT} },
{    "fireball",            SPELL_FIREBALL,          29,     98,     {INT,CON} },
{    "focused repelance",   SKILL_FOCUSED_REPELANCE, 30,     98,     {DEX,INT} },
{    "piercing",            SKILL_PIERCEING_WEAPONS, 30,     55,     {DEX,STR} },
{    "bludgeoning",         SKILL_BLUDGEON_WEAPONS,  30,     55,     {STR,DEX} },
{    "resist magic",        SPELL_RESIST_MAGIC,      31,     90,     {INT,WIS} },
{    "haste",               SPELL_HASTE,             33,     98,     {DEX,INT} }, 
{    "true sight",          SPELL_TRUE_SIGHT,        34,     85,     {WIS,INT} }, 
{    "dispel magic",        SPELL_DISPEL_MAGIC,      35,     85,     {INT,CON} },
{    "resist fire",         SPELL_RESIST_FIRE,       36,     70,     {INT,CON} }, 
{    "wizard eye",          SPELL_WIZARD_EYE,        37,     98,     {INT,WIS} }, 
{    "teleport",            SPELL_TELEPORT,          38,     98,     {CON,INT} }, 
{    "stoneskin",           SPELL_STONE_SKIN,        39,     70,     {STR,CON} }, 
{    "meteor swarm",        SPELL_METEOR_SWARM,      40,     98,     {STR,INT} }, 
{    "word of recall",      SPELL_WORD_OF_RECALL,    42,     85,     {STR,WIS} }, 
{    "firestorm",           SPELL_FIRESTORM,         43,     90,     {INT,STR} }, 
{    "paralyze",            SPELL_PARALYZE,          44,     98,     {INT,DEX} },
{    "hellstream",          SPELL_HELLSTREAM,        45,     98,     {INT,STR} },
{    "fireshield",          SPELL_FIRESHIELD,        47,     98,     {CON,INT} },
{    "create golem",        SPELL_CREATE_GOLEM,      48,     90,     {WIS,STR} },
{    "release golem",       SPELL_RELEASE_GOLEM,     48,     90,     {WIS,INT} },
{    "solar gate",          SPELL_SOLAR_GATE,        49,     98,     {WIS,INT} },
{    "spellcraft",          SKILL_SPELLCRAFT,        50,     98,     {WIS,INT} },
{    "\n",                  0,                       1,      0,      {0,0} }
};

// End of abilities listings for each class.



char *languages[] =
{
  "common"
  "human",
  "elvish",
  "dwarven",
  "halfling",
  "brownie",
  "giant",
  "gnomish",
  "drow",
  "orcish",
  "dragon",
  "animal",
  "flora",
  "planar",
  "demon",
  "deity",
  "\n"
};

char *race_types[] =
{
    "Undefined",
    "Human",
    "Elven",
    "Dwarven",
    "Hobbit",
    "Pixie",
    "Giant",
    "Gnome",
    "Orc",
    "Troll",
    "\n"
};

char *race_abbrev[] =
{
    "Und",
    "Hum",
    "Elf",
    "Dwf",
    "Hob",
    "Pix",
    "Gia",
    "Gno",
    "Orc",
    "Tro",
    "Gob",
    "Rep",
    "Drg",
    "Sna",
    "Hor",
    "Brd",
    "Rod",
    "Fis",
    "Arc",
    "Ins",
    "Sli",
    "Ani",
    "Tre",
    "Enf",
    "Und",
    "Gho",
    "Gol",
    "Ele",
    "Pla",
    "Dem",
    "Yrn",
    "\n",
};

/*
struct race_shit
{
  char *singular_name;  // dwarf, elf, etc.
  char *plural_name;     // dwarves, elves, etc.

  long body_parts;  // bitvector for body parts
  long immune;      // bitvector for immunities
  long resist;      // bitvector for resistances
  long suscept;     // bitvector for susceptibilities
  long hate_fear;   // bitvector for hate/fear
  long friendly;    // bitvector for friendliness
  int  weight;      // average weight of ths race
  int  height;      // average height for ths race
};
*/

struct race_shit race_info[] =
{
// Name,        Plural,     Bodyparts, Immunities, Resists, Suscepts, 
//Hates/Fears, Friendly, Weight, Height, Affects, UnarmedHitType
{  "NPC",       "NPC",         63,         0,         0,       0,          0,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT, "punch"},
{  "Human",     "Humans",      63,         0,         0,       0,     1<<22|1<<29,    0,     175,     70,    AFF_IGNORE_WEAPON_WEIGHT, "punch" },
{  "Elf",       "Elves",       63,         0,       264,       0,      1<<7|1<<2,     0,     150,     85,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "punch"},
{  "Dwarf",     "Dwarves",     63,         0,       128,       0,      1<<1|1<<5,     0,     200,     60,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "hook" },
{  "Hobbit",    "Hobbits",     63,         0,         0,       0,      1<<7|1<<8,     0,     100,     35,    AFF_IGNORE_WEAPON_WEIGHT|AFF_HIDE, "jab" },
{  "Pixie",     "Pixies",      63,         0,         0,       0,      1<<5|1<<8,     0,      50,     20,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_FLYING, "bite" },
{  "Giant",     "Giants",      63,         0,         0,       0,      1<<2|1<<4,     0,     500,    125,    AFF_IGNORE_WEAPON_WEIGHT, "uppercut" },
{  "Gnome",     "Gnomes",      63,         0,         0,       0,      1<<9|1<<11,    0,     125,     55,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "punch" },
{  "Orc",       "Orcs",        63,         0,         0,       0,      1<<1|1<<3,     0,     225,     90,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "hook" },
{  "Troll",     "Trolls",      63,         0,       128,      80,      1<<3|1<<4,     0,     300,    115,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "uppercut" },
{  "Goblin",    "Goblins",     63,         0,         0,       0,      1<<6|1<<11,    0,     175,     70,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "jab" },
{  "Reptile",   "Reptiles",    59,         0,         0,     520,          0,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT, "strike" },
{  "Dragon",    "Dragons",     91,         0,       268,       0,      1<<6|1<<9,     0,    2500,    200,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE|AFF_FLYING|AFF_TRUE_SIGHT|AFF_SOLIDITY, "bite" },
{  "Snake",     "Snakes",       3,         0,         1, 2097160,      32768,      4096,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT, "bite" },
{  "Horse",     "Horses",      19,         0,         0,       0,       2048,         3,     600,     70,    AFF_IGNORE_WEAPON_WEIGHT, "bite" },
{  "Bird",      "Birds",       63,         0,   2097152,       0,      65536,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_FLYING, "peck" },
{  "Rodent",    "Rodents",     59,         0,         0,     128,       4096,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT, "bite"},
{  "Fish",      "Fishes",       3,         0,   4194304,       0,      16384,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT, "bite" },
{  "Arachnid",  "Arachnids",   19,         0,       128,       0,     262144,    131072,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE, "bite" },
{  "Insect",    "Insects",     19,         0,         0,       0,     131072,    262144,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "bite" },
{  "Slime",     "Slimes",       2,         1,        64,     528,          0,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT, "slap" },
{  "Animal",    "Animals",     27,         0,         0,     128,    8388608,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_SENSE_LIFE, "bite" },
{  "Plant",     "Plants",      63,         0,   4194304,      16,    1048576,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT, "slap" },
{  "Enfan",     "Enfans",      63,         0,         0,     512,          1,   4194304,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "jab" },
{  "Undead",    "Undead",      63,       392,         0,      64,          0,   8388608,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE, "punch" },
{  "Ghost",     "Spirits",     63,       904,        80,      36,          0,  16777216,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE|AFF_FLYING|AFF_INVISIBLE, "scream" },
{  "Golem",     "Golems",      63,       256,       128,      64,          0,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT, "punch" },
{  "Elemental", "Elementals",  63,         0,      1024,      12,          0,  67108864,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_SOLIDITY, "punch" },
{  "Planar",    "Planar",      63,        64,         4,      32,          0,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE|AFF_SOLIDITY|AFF_REFLECT, "punch" },
{  "Demon",     "Demons",     127,        16,         8, 1048576,       1023,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE, "punch" },
{  "Yrnali",    "Yrnali",      63,        16,         8, 1048576,          1, 536870912,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE, "punch" },
{  "Immortal",  "Immortals",  127,      1288,   1048576,       0,          0,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE|AFF_FLYING|AFF_TRUE_SIGHT|AFF_SOLIDITY|AFF_SANCTUARY, "gaze" },
{  "Feline",    "Felines",     27,         0,         0,     128,    8388608,         0,       0,      0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_SNEAK, "claw" }
}; 

int mob_race_mod[][5] =
{
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0},
{0, 0, 0, 0, 0}
};

char 
*action_bits[] = {
    "SPEC",
    "SENTINEL",
    "SCAVENGER",
    "NOTRACK",
    "NICE-THIEF",
    "AGGRESSIVE",
    "STAY_ZONE",
    "WIMPY",
    "2ND_ATTACK",
    "3RD_ATTACK",
    "4TH_ATTACK",
    "AGGRESSIVE_EVIL",
    "AGGRESSIVE_GOOD",
    "AGGRESSIVE_NEUTRAL",
    "IS_UNDEAD",
    "IS_STUPID",
    "CHARMABLE",
    "IS_HUGE",
    "CAN_DODGE",
    "CAN_PARRY",
    "RACIST",
    "FRIENDLY",
    "STAY_NO_TOWN",
    "NOMAGIC",
    "DRAINY",
    "BARDCHARM",
    "NOKI",
    "NOMATRIX",
    "BOSS",
    "SOLID",
    "\n"
};


char *player_bits[] =
{
    "BRIEF",
    "COMPACT",
    "DONTSET",
    "NOTELL",
    "NOHASSLE",
    "NOSUMMON",
    "WIMPY",
    "ANSI",
    "VT100",
    "ONEWAY ",
    "DISGUISED ",
    "NOTELL",
    "NO-PAGER",
    "BEEP",
    "NO-SONG",
    "ANONYMOUS",
    "AUTO-EAT",
    "LFG",
    "NOTELL",
    "NOTAX",
    "GUIDE",
    "GUIDE_TOG",
    "NEWS",
    "50PLUS",
    "ASCII",
    "\n"
};

char *punish_bits[] =
{
    "Silenced",
    "NoEmote",
    "Logged",
    "Freeze",
    "Deny",
    "Thief",
    "NoName",
    "Spammer",
    "NoArena",
    "\n"
};

char *combat_bits[] =
{
    "Shocked",
    "Bash1",
    "Bash2",
    "Stunned1",
    "Stunned2",
    "Circle",
    "Berserk",
    "Hitall",
    "Rage1",
    "Rage2",
    "Bladeshield1",
    "Bladeshield2",
    "Repelance",
    "VitalStrike",
    "MonkStance",
    "MissAnAttack",
    "Bloodlust1",
    "Bloodlust2",
    "Eyegouge1",
    "Eyegouge2",
    "\n"
};

char *isr_bits[] =
{
    "PIERCE",
    "SLASH",
    "MAGIC",
    "CHARM",
    "FIRE",
    "ENERGY",
    "ACID",
    "POISON",
    "SLEEP",
    "COLD",
    "PARA",
    "BLUDGEON",
    "WHIP",
    "CRUSH",
    "HIT",
    "BITE",
    "STING",
    "CLAW",
    "PHYSICAL",
    "NON-MAGIC",
    "KI",
    "SONG",
    "WATER",
    "\n"
};


// Mortally wounded and Incapacitated are no longer used.
// Dead is used in fight.C but should never been seen by a player
char *position_types[] =
{
    "Dead",
    "Mortally wounded",
    "Incapacitated",
    "Stunned",
    "Sleeping",
    "Resting",
    "Sitting",
    "Fighting",
    "Standing",
    "\n"
};

char *connected_types[] =
{
    "Playing",
    "Get name",
    "Get old password",
    "Confirm name",
    "Get new password",
    "Confirm new password",      // 5
    "Get sex",
    "Get new class",
    "Read messages of today",
    "Select Menu",
    "Reset password",            // 10
    "Confirm reset password",
    "Get extra description",
    "Get race",
    "Write board",
    "Editing",
    "Sending Mail",
    "Delete character screen",
    "Choose stats",
    "Pfile wipe",
    "Archive char screen",
    "Closing",
    "Confirm password change",
    "Editing mprog",
    "\n"
};

/* [clss], [level] (all) */
int thaco[12][61] =
{
    {  /* mage */
        100,20,20,19,19,18,18,17,17,17,16,16,16,15,15,15,14,14,14,14,13,
        13,13,12,12,12,12,13,13,13,13,12,12,12,12,11,11,11,11,10,10,10,10,
        9,9,8,8,7,7,6,6,5,5,4,3,3,3,3,3,3
    },
    {  /* cleric */
        100,20,20,19,19,18,18,17,17,16,16,16,15,15,15,14,14,14,13,13,13,
        12,12,12,11,11,11,10,10,10,9,9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,1,0,
        -1,-2,-3,-3,-3,-4,-5,-5,-5,-5,-5,-5
    },
    {  /* thief */
        100,20,20,19,19,18,18,17,17,16,16,16,15,15,15,14,14,14,13,13,13,
        12,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,6,6,6,5,5,5,4,4,3,2,
        1,0,0,0,0,0,0,0,0,0,0,0
    },
    {  /* warrior */
        100,20,20,19,19,18,18,17,17,16,16,15,15,14,14,13,13,12,12,11,11,
        10,10,9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1,0,0,-1,-1,-2,-3,-4,-5,
        -6,-7,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8
    },
    {  /* anti-paladin */
        100,20,20,19,19,18,18,17,17,16,16,16,15,15,15,14,14,14,13,13,13,
        12,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,6,6,6,5,5,5,4,4,3,2,
        1,0,0,0,0,0,0,0,0,0,0,0
     },
    {  /* paladin */
        100,20,20,19,19,18,18,17,17,16,16,16,15,15,15,14,14,14,13,13,13,
        12,12,12,11,11,11,10,10,10,9,9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,1,0,
        -1,-2,-3,-3,-3,-4,-5,-5,-5,-5,-5,-5
     },
    {  /* barbarian */
        100,20,20,19,19,18,18,17,17,16,16,15,15,14,14,13,13,12,12,11,11,
        10,10,9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1,0,0,-1,-1,-2,-3,-4,-5,
        -6,-7,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8
    },
    {  /* monk */
        100,20,20,19,19,18,18,17,17,16,16,15,15,14,14,13,13,12,12,11,11,
        10,10,9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1,0,0,-1,-1,-2,-3,-4,-5,
        -6,-7,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8
     },
    {  /* ranger */
        100,20,20,19,19,18,18,17,17,16,16,16,15,15,15,14,14,14,13,13,13,
        12,12,12,11,11,11,10,10,10,9,9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,1,0,
        -1,-2,-3,-3,-3,-4,-5,-5,-5,-5,-5,-5
     },
    {  /* bard */
        100,20,20,19,19,18,18,17,17,16,16,16,15,15,15,14,14,14,13,13,13,
        12,12,12,11,11,11,10,10,10,9,9,9,8,8,8,7,7,7,6,6,6,5,5,5,4,4,3,2,
        1,0,0,0,0,0,0,0,0,0,0,0
    },
    {  /* druid */
        100,20,20,19,19,18,18,17,17,16,16,16,15,15,15,14,14,14,13,13,13,
        12,12,12,11,11,11,10,10,10,9,9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,1,0,
        -1,-2,-3,-3,-3,-4,-5,-5,-5,-5,-5,-5
    },
    {  /* psionicist */
        100,20,20,19,19,18,18,17,17,17,16,16,16,15,15,15,14,14,14,14,13,
        13,13,12,12,12,12,13,13,13,13,12,12,12,12,11,11,11,11,10,10,10,10,
        9,9,8,8,7,7,6,6,5,5,4,3,3,3,3,3,3
    }

};

// Constitution Attribute Modifiers
const struct con_app_type con_app[] = {
/*STAT#  { HP_REGEN/TICK, MOVE_REGEN/TICK, HP_GAIN/LEVEL, POSION_RES	},*/
/*  0 */ {	-20,		-20,		-4,		-6,	},
/*  1 */ {	-18,		-18,		-4,		-5,	},
/*  2 */ {	-16,		-16,		-3,		-5,	},
/*  3 */ {	-14,		-14,		-3,		-4,	},
/*  4 */ {	-12,		-12,		-3,		-4,	},
/*  5 */ {	-10,		-10,		-2,		-3,	},
/*  6 */ {	-8,		-8,		-2,		-3,	},
/*  7 */ {	-6,		-6,		-2,		-2,	},
/*  8 */ {	-4,		-4,		-1,		-2,	},
/*  9 */ {	-2,		-2,		-1,		-2,	},
/* 10 */ {	-1,		-1,		-1,		-1,	},
/* 11 */ {	 0,		 0,		 0,		-1,	},
/* 12 */ {	 0,		 0,		 0,		-1,	},
/* 13 */ {	 0,		 0,		 0,		 0,	},
/* 14 */ {	 1,		 1,		 0,		 0,	},
/* 15 */ {	 1,		 1,		 0,		 0,	},
/* 16 */ {	 2,		 2,		 1,		 0,	},
/* 17 */ {	 2,		 2,		 1,		 0,	},
/* 18 */ {	 3,		 3,		 1,		 0,	},
/* 19 */ {	 4,		 4,		 1,		 1,	},
/* 20 */ {	 5,		 5,		 1,		 1,	},
/* 21 */ {	 6,		 6,		 2,		 1,	},
/* 22 */ {	 7,		 7,		 2,		 2,	},
/* 23 */ {	 8,		 8,		 2,		 2,	},
/* 24 */ {	 9,		 9,		 2,		 2,	},
/* 25 */ {	 10,		 10,		 2,		 3,	},
/* 26 */ {	 12,		 12,		 3,		 3,	},
/* 27 */ {	 14,		 14,		 3,		 4,	},
/* 28 */ {	 16,		 16,		 3,		 4,	},
/* 29 */ {	 18,		 18,		 3,		 5,	},
/* 30 */ {	 20,		 20,		 3,		 5,	},
};

// Intelligence Attribute Modifiers
const struct int_app_type int_app[] = {
/* STAT# { MANA_REGEN/TICK, KI_REGEN/TICK, EASY_BONUS,   MEDIUM_BONUS, HARD_BONUS, PRAC_BONUS, MAGIC_RES, CONC_BONUS	
},*/
/*  0 */ {	-10,		-5,		-12,		-3,	 -1,		0,  	-6,	  -5,		},
/*  1 */ {	-10,		-5,		-10,		-3,	 -1,		0,	-5,	  -5,		},	
/*  2 */ {	-9,		-5,		-9,		-3,	 -1,		0,	-5,	  -4,		},
/*  3 */ {	-8,		-4,		-8,		-3,	 -1,		0,	-4,	  -4,		},
/*  4 */ {	-7,		-4,		-7,		-3,	 -1,		0,	-4,	  -3,		},
/*  5 */ {	-6,		-3,		-6,		-3, 	 -1,		0,	-3,	  -3,		},
/*  6 */ {	-5,		-3,		-5,		-2,	 -1,		0,	-3,	  -2,		},
/*  7 */ {	-4,		-2,		-4,		-2,	 -1,		0,	-2,	  -2,		},
/*  8 */ {	-3,		-2,		-3,		-1,	  0,		0,	-2,	  -1,		},
/*  9 */ {	-2,		-1,		-2,		 0,	  0,		0,	-2,	  -1,		},
/* 10 */ {	-1,		-1,		-1,		 0,	  0,		0,	-1,	   0,		},
/* 11 */ {	 0,		 0,		 0,		 0,	  0,		0,	-1,	   0,		},
/* 12 */ {	 0,		 0,		 0,		 0,	  0,		0,	-1,	   0,		},
/* 13 */ {	 0,		 0,		 0,		 0,	  0,		0,	 0,	   0,		},
/* 14 */ {	 1,		 0,		 0,		 0,	  0,		0,	 0,	   0,		},
/* 15 */ {	 1,		 1,		 0,		 0,	  0,		0,	 0,	   1,		},
/* 16 */ {	 2,		 1,		 0,		 0,	  0,		0,	 0,	   1,		},
/* 17 */ {	 2,		 1,		 1,		 0,	  0,		0,	 0,	   2,		},
/* 18 */ {	 3,		 2,		 1,		 0,	  0,		0,	 0,	   2,		},
/* 19 */ {	 4,		 2,		 2,		 0,	  0,		1,	 1,	   3,		},
/* 20 */ {	 5,		 2,		 2,		 1,	  0,		1,	 1,	   3,		},
/* 21 */ {	 6,		 3,		 2,		 1,	  1,		1,	 1,	   4,		},
/* 22 */ {	 7,		 3,		 3,		 1,	  1,		1,	 2,	   4,		},
/* 23 */ {	 8,		 3,		 3,		 2,	  1,		1,	 2,	   5,		},
/* 24 */ {	 9,		 4,		 4,		 2,	  1,		2,	 2,	   5,		},
/* 25 */ {	 10,		 4,		 4,		 2,	  2,		2,	 3,	   6,		},
/* 26 */ {	 11,		 5,		 4,		 3, 	  2,		2,	 3,	   6,		},
/* 27 */ {	 12,		 5,		 5,		 3,  	  2,		2,	 4,	   7,		},
/* 28 */ {	 13,		 6,		 5,		 4,	  2,		3,	 4,	   8,		},
/* 29 */ {	 14,		 7,		 6,		 4,	  3,		3,	 5,	   9,		},
/* 30 */ {	 15,		 8,		 7,		 4,	  3,		3,	 5,	  10,		},
};

// Wisdom Attribute Modifiers
const struct wis_app_type wis_app[] = {
/* STAT# { MANA_REGEN/TICK, KI_REGEN/TICK, PRACS/LEVEL BONUS, ENERGY_RES, CONC_BONUS 	},*/
/*  0 */ {	-10,		-9,		-1,		  -6,		-5,	},
/*  1 */ {	-10,		-8,		-1,		  -5,		-5,	},	
/*  2 */ {	-9,		-7,		-1,		  -5,		-4,	},
/*  3 */ {	-8,		-6,		-1,		  -4,		-4,	},
/*  4 */ {	-7,		-5,		-1,		  -4,		-3,	},
/*  5 */ {	-6,		-4,		-1,		  -3,		-3,	},
/*  6 */ {	-5,		-3,		 0,		  -3,		-2,	},
/*  7 */ {	-4,		-2,		 0,		  -2,		-2,	},
/*  8 */ {	-3,		-2,		 0,		  -2,		-1,	},
/*  9 */ {	-2,		-1,		 0,		  -2,		-1,	},
/* 10 */ {	-1,		-1,		 0,		  -1,		 0,	},
/* 11 */ {	 0,		 0,		 0,		  -1,		 0,	},
/* 12 */ {	 0,		 0,		 0,		  -1,		 0,	},
/* 13 */ {	 0,		 0,		 0,		   0,		 0,	},
/* 14 */ {	 1,		 0,		 1,		   0,		 0,	},
/* 15 */ {	 1,		 1,		 1,		   0,		 1,	},
/* 16 */ {	 2,		 1,		 1,		   0,		 1,	},
/* 17 */ {	 2,		 1,		 2,		   0,		 2,	},
/* 18 */ {	 3,		 2,		 2,		   0,		 2,	},
/* 19 */ {	 4,		 2,		 3,		   1,		 3,	},
/* 20 */ {	 5,		 2,		 3,		   1,		 3,	},
/* 21 */ {	 6,		 3,		 4,		   1,		 4,	},
/* 22 */ {	 7,		 3,		 4,		   2,		 4,	},
/* 23 */ {	 8,		 3,		 5,		   2,		 5,	},
/* 24 */ {	 9,		 4,		 5,		   2,		 5,	},
/* 25 */ {	 10,		 4,		 6,		   3,		 6,	},
/* 26 */ {	 11,		 5,		 6,		   3,		 6,	},
/* 27 */ {	 12,		 5,		 7,		   4,		 7,	},
/* 28 */ {	 13,		 6,		 7,		   4,		 8,	},
/* 29 */ {	 14,		 7,		 8,		   5,		 9,	},
/* 30 */ {	 15,		 8,		 8,		   5,		10,	},
};

// Dexterity Attribute Modifiers
const struct dex_app_type dex_app[] = {
/* STAT# { TO_HIT_ BONUS, AC_BONUS, MOVE_GAIN/LEVEL, FIRE_RES	},*/
/*  0 */ {	-5,	     48,	-4,		-6,	},
/*  1 */ {	-4,	     40,	-4,     	-5,	},
/*  2 */ {	-3,	     36,	-3,		-5,	},
/*  3 */ {	-3,	     32,	-3,		-4,	},
/*  4 */ {	-2,	     28,	-3,		-4,	},
/*  5 */ {	-2,	     24,	-2,		-3,	},
/*  6 */ {	-2,	     20,	-2,		-3,	},
/*  7 */ {	-2,	     16,	-2,		-2,	},
/*  8 */ {	-1,	     12,	-1,		-2,	},
/*  9 */ {	-1,	     8,		-1,		-2,	},
/* 10 */ {	-3,	     6,		-1,		-1,	},
/* 11 */ {	-1,	     4,		 0,		-1,	},
/* 12 */ {	 0, 	     3,		 0,		-1,	},
/* 13 */ {	 0,	     1,		 0,		 0,	},
/* 14 */ {	 0,	     0,		 0,		 0,	},
/* 15 */ {	 0,	    -1,		 0,		 0,	},
/* 16 */ {	 1,	    -2,		 1,		 0,	},
/* 17 */ {	 1,	    -4,		 1,		 0,	},
/* 18 */ {	 1,	    -6,		 1,		 0,	},
/* 19 */ {	 2,	    -8,		 1,		 1,	},
/* 20 */ {	 2,	    -10,	 1,		 1,	},
/* 21 */ {	 3,	    -12,	 2,		 1,	},
/* 22 */ {	 4,	    -14,	 2,		 2,	},
/* 23 */ {	 5,	    -16,	 2,		 2,	},
/* 24 */ {	 6,	    -18,	 2,		 2,	},
/* 25 */ {	 7,	    -20,	 2,		 3,	},
/* 26 */ {	 8,	    -24,	 3,		 3,	},
/* 27 */ {	 9,	    -28,	 3,		 4,	},
/* 28 */ {	 10,	    -32,	 3,		 4,	},
/* 29 */ {	 11,	    -36,	 3,		 5,	},
/* 30 */ {	 12,	    -40,	 3,		 5,	},
};

// Strength Attribute Modifiers
const struct str_app_type str_app[] = {
/* STAT# { TO_DAMAGE_BONUS, MAX_CARRIED, COLD RES	},*/
/*  0 */ {	-7,	    	25,	   -6,		},
/*  1 */ {	-6,		25,	   -5,		},
/*  2 */ {	-5,		30,	   -5,		},
/*  3 */ {	-4,		40,	   -4,		},
/*  4 */ {	-3,		50,	   -4,		},
/*  5 */ {	-3,		60,	   -3,		},
/*  6 */ {	-2,		70,	   -3,		},
/*  7 */ {	-2,		80,	   -2,		},
/*  8 */ {	-2,		90,	   -2,		},
/*  9 */ {	-1,		100,	   -2,		},
/* 10 */ {	-1,		110,	   -1,		},
/* 11 */ {	-1,		120,	   -1,		},
/* 12 */ {	 0,		130,	   -1,		},
/* 13 */ {	 0,		140,	    0,		},
/* 14 */ {	 0,		150,	    0,		},
/* 15 */ {	 0,		160,	    0,		},
/* 16 */ {	 1,		170,	    0,		},
/* 17 */ {	 1,		180,	    0,		},
/* 18 */ {	 1,		190,	    0,		},
/* 19 */ {	 2,		200,	    1,		},
/* 20 */ {	 2,		210,	    1,		},
/* 21 */ {	 3,		220,	    1,		},
/* 22 */ {	 4,		230,	    2,		},
/* 23 */ {	 5,		240,	    2,		},
/* 24 */ {	 6,		250,	    2,		},
/* 25 */ {	 7,		260,	    3,		},
/* 26 */ {	 8,		280,	    3,		},
/* 27 */ {	 9,		300,	    4,		},
/* 28 */ {	 10,		320,	    4,		},
/* 29 */ {	 11,		360,	    5,		},
/* 30 */ {	 12,		400,	    5,		},
};

/* [level] backstab multiplyer (thieves only) */
ubyte backstab_mult[61] =
{
    1,   /* 0 */
    6,   /* 1 */
    6,
    6,
    6,
    6,   /* 5 */
    6,
    6,
    6,
    6,
    6,    /* 10 */
    7,
    7,
    7,
    7,
    7,     /* 15 */
    7,
    7,
    7,
    7,
    7,      /* 20 */
    8,
    8,
    8,
    8,
    8,     /* 25 */
    8,
    8,
    8,
    8,
    8,     /* 30 */
    9,
    9,
    9,
    9,
    9,      /* 35 */
    9,
    9,
    9,
    9,
    9,      /* 40 */
    9,
    9,
    9,
   10,
   10,    /* 45 */
   10,
   10,
   10,
   10,
   11,    /* 50 */
   12,
   13,
   14,
   15,
   16,    /* 55 */
   17,
   18,
   19,
   20,
   21
};

int mana_bonus[31] =
{
       0,
       0,    /* 1 */
       0,
       0,
       0,
       0,
       0,
       0,
       0,
       0,
       0,      /* 10 */
       0,
       0,
       0,
       5,
      10,
      20,
      30,
      40,       /* 18 */
      50,
      60,       /* 20 */
      70,
      80,
      90,
     100,
     110,        /* 25 */
     120,
     130,
     140,
     150,
     160        /* 30 */
};

struct mob_matrix_data mob_matrix[] = 
{
/* 0 */{5,5,1,1,100,0},
/* 1 */{500,5,1,1,100,250},
/* 2 */{750,10,2,1,100,500},
/* 3 */{1000,20,3,2,95,750},
/* 4 */{2000,30,4,2,95,1000},
/* 5 */{3000,40,5,3,90,1250},
/* 6 */{4000,50,6,3,90,1500},
/* 7 */{5000,50,6,3,90,1750},
/* 8 */{6500,80,8,4,85,2000},
/* 9 */{8000,95,9,5,80,2250},
/* 10 */{11000,110,10,5,80,2500},
/* 11 */{14000,125,11,6,75,3000},
/* 12 */{17000,150,12,7,70,3500},
/* 13 */{20000,175,13,8,65,4000},
/* 14 */{30000,200,14,9,60,4500},
/* 15 */{40000,225,15,10,55,5000},
/* 16 */{50000,250,16,11,50,5500},
/* 17 */{60000,275,17,12,45,6000},
/* 18 */{70000,300,18,13,40,6500},
/* 19 */{80000,325,19,14,35,7000},
/* 20 */{90000,350,20,15,30,7500},
/* 21 */{100000,375,21,16,25,8000},
/* 22 */{110000,400,22,17,20,8750},
/* 23 */{120000,430,23,18,15,9500},
/* 24 */{130000,460,24,19,10,10250},
/* 25 */{140000,490,25,20,5,11000},
/* 26 */{150000,520,26,20,0,11750},
/* 27 */{160000,550,27,21,-5,12500},
/* 28 */{170000,580,28,21,-10,13250},
/* 29 */{180000,610,29,22,-15,14000},
/* 30 */{190000,640,30,22,-20,15000},
/* 31 */{200000,670,31,23,-30,16000},
/* 32 */{220000,700,32,23,-35,17000},
/* 33 */{240000,740,33,24,-40,18000},
/* 34 */{260000,780,34,24,-45,19000},
/* 35 */{280000,820,35,25,-50,20000},
/* 36 */{300000,860,36,26,-60,21500},
/* 37 */{325000,900,37,27,-70,23000},
/* 38 */{350000,935,38,28,-80,24500},
/* 39 */{375000,970,39,29,-90,26000},
/* 40 */{400000,1000,40,20,-100,27500},
/* 41 */{425000,1050,41,31,-115,30000},
/* 42 */{450000,1100,42,32,-130,32500},
/* 43 */{475000,1150,43,33,-145,35000},
/* 44 */{500000,1200,44,34,-160,37500},
/* 45 */{550000,1250,45,35,-175,40000},
/* 46 */{600000,1300,46,36,-190,42500},
/* 47 */{650000,1350,47,37,-205,45000},
/* 48 */{700000,1400,48,38,-220,47500},
/* 49 */{750000,1450,49,39,-235,50000},
/* 50 */{800000,1500,50,40,-250,55000},
/* 51 */{1000000,1600,51,41,-260,60000},
/* 52 */{1050000,1700,52,42,-270,65000},
/* 53 */{1100000,1800,53,43,-280,70000},
/* 54 */{1150000,1900,54,44,-290,75000},
/* 55 */{1200000,2000,55,45,-300,80000},
/* 56 */{1250000,2100,56,46,-310,85000},
/* 57 */{1300000,2200,57,47,-320,90000},
/* 58 */{1350000,2300,58,48,-330,95000},
/* 59 */{1400000,2400,59,49,-340,100000},
/* 60 */{1500000,2500,60,50,-350,105000},
/* 61 */{1600000,2600,61,51,-360,110000},
/* 62 */{1700000,2700,62,52,-270,115000},
/* 63 */{1800000,2800,63,53,-380,120000},
/* 64 */{1900000,2900,64,54,-390,125000},
/* 65 */{2000000,3000,65,55,-400,130000},
/* 66 */{2100000,3200,66,56,-410,135000},
/* 67 */{2200000,3400,67,57,-420,140000},
/* 68 */{2300000,3600,68,58,-430,145000},
/* 69 */{2400000,3800,69,59,-440,150000},
/* 70 */{2500000,4000,70,60,-450,155000},
/* 71 */{2600000,4250,72,62,-460,160000},
/* 72 */{2700000,4500,72,62,-470,165000},
/* 73 */{2800000,4750,73,63,-480,170000},
/* 74 */{2900000,5000,74,64,-490,175000},
/* 75 */{5000000,6000,75,65,-500,350000},
/* 76 */{5100000,6250,76,66,-510,400000},
/* 77 */{5200000,6500,77,67,-520,425000},
/* 78 */{5300000,6750,78,68,-530,450000},
/* 79 */{5400000,7000,79,69,-540,475000},
/* 80 */{5500000,7250,80,70,-550,500000},
/* 81 */{5600000,7500,81,71,-560,525000},
/* 82 */{5700000,8000,82,72,-570,550000},
/* 83 */{5800000,8500,83,73,-580,575000},
/* 84 */{5900000,9000,84,74,-590,600000},
/* 85 */{6000000,9500,85,75,-600,625000},
/* 86 */{6100000,10000,86,76,-610,650000},
/* 87 */{6200000,10500,87,77,-620,675000},
/* 88 */{6300000,11000,88,78,-630,700000},
/* 89 */{6400000,11500,89,79,-640,800000},
/* 90 */{6500000,12000,90,80,-650,900000},
/* 91 */{6600000,12500,91,81,-660,1000000},
/* 92 */{6700000,13000,92,82,-670,1200000},
/* 93 */{6800000,13500,93,83,-680,1400000},
/* 94 */{7000000,14000,94,84,-690,1600000},
/* 95 */{7250000,15000,95,85,-700,1800000},
/* 96 */{7500000,16000,96,86,-710,2000000},
/* 97 */{7750000,17000,97,87,-720,2200000},
/* 98 */{8000000,18000,98,88,-730,2400000},
/* 99 */{8250000,19000,99,89,-740,2600000},
/* 100 */{8500000,20000,100,90,-750,2800000},
/* 101 */{8750000,21000,101,91,-775,3000000},
/* 102 */{9000000,22000,102,92,-800,3200000},
/* 103 */{9250000,23000,103,93,-825,3400000},
/* 104 */{9500000,24000,104,94,-850,3600000},
/* 105 */{9750000,25000,105,95,-875,3800000},
/* 106 */{10000000,26000,106,96,-900,4000000},
/* 107 */{10250000,27000,107,97,-925,4250000},
/* 108 */{10500000,28000,108,98,-950,4500000},
/* 109 */{10750000,29000,109,99,-975,4750000},
/* 110 */{11000000,30000,110,100,-1000,5000000}
};
