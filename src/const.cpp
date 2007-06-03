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
/* $Id: const.cpp,v 1.242 2007/06/03 16:17:37 urizen Exp $ */
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
{ "impchan",	COMMAND_IMP_CHAN, false },
{ "snoop",	COMMAND_SNOOP, false },
{ "global",	COMMAND_GLOBAL, false },
{ "restore",	COMMAND_RESTORE, false },
{ "purloin",	COMMAND_PURLOIN, false },
{ "possess",	COMMAND_POSSESS, false},
{ "arena",	COMMAND_ARENA, false },
{ "set",	COMMAND_SET, false },
{ "load",	COMMAND_LOAD, false },
{ "shutdown",   COMMAND_SHUTDOWN, false },
{ "procedit",	COMMAND_MP_EDIT, false },
{ "range",      COMMAND_RANGE, false },
{ "procstat",	COMMAND_MPSTAT, false },
{ "pshopedit",  COMMAND_PSHOPEDIT, false },
{ "sedit",      COMMAND_SEDIT, false },
{ "punish",     COMMAND_PUNISH, false },
{ "sqedit",     COMMAND_SQEDIT, false },
{ "hedit",      COMMAND_HEDIT, false },
{ "hindex",     COMMAND_HINDEX, false },
{ "opstat",	COMMAND_OPSTAT, false },
{ "opedit",	COMMAND_OPEDIT, false },
{ "eqmax",	COMMAND_EQMAX, true },
{ "force",	COMMAND_FORCE, false },
{ "string",	COMMAND_STRING, false },
{ "stat",	COMMAND_STAT, false },
{ "sqsave",	COMMAND_SQSAVE, false },
{ "whattonerf",	COMMAND_WHATTONERF, true },
{ "find",	COMMAND_FIND, false },
{ "log",	COMMAND_LOG, false },
{ "addnews",	COMMAND_ADDNEWS, false },
{ "prize",	COMMAND_PRIZE, false },
{ "sockets",	COMMAND_SOCKETS, false },
{ "qedit",	COMMAND_QEDIT, false },
{ "rename",	COMMAND_RENAME, false },
{ "findpath",   COMMAND_FINDPATH, true },
{ "findpath2",  COMMAND_FINDPATH2, true },
{ "addroom",    COMMAND_ADDROOM, true },
{ "newpath",    COMMAND_NEWPATH, true },
{ "listpathsbyzone", COMMAND_LISTPATHSBYZONE, true },
{ "listallpaths",    COMMAND_LISTALLPATHS, true },
{ "testhand",   COMMAND_TESTHAND, true },
{ "dopathpath", COMMAND_DOPATHPATH, true },
{ "do_the_thing", COMMAND_DO_THE_THING, true },
{ "quest", COMMAND_QUEST, true },
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
 "can_see_prog",
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
  "The shield of $2acid$R around you dissipates into the air.", /* 50 */
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
  "You are able to sleep once again.",
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
  "!Eyes of the Eagle!", // 130
  "!Haste Other!",
  "!Ice Shards!",  
  "The $B$5lightning$R around you fades away leaving only static cling.",
  "!Blue bird!",
  "With a rush of strength, the $6debility$R fades from your body.",
  "Your rapid decay ends and your health returns to normal.",
  "The $B$0shadow$R in your aura fades away into the ethereal.",
  "Your serene aura of holiness fades.",
  "!DismissFamiliar!",
  "!DismissCorpse!", // 140
  "Your blessed halo fades.",
  "You don't feel quite so hateful anymore.",
  "The foul mantle surrounding you dissipates to nothing.",
  "Your oaken fortitude fades, returning your constitution to normal.",
  "The $B$2icy$R shield of frost around you fades away.",
  "Your ability to stand firm ends as the magical stability fades.",
  "You are no longer flagged as a thief.",
  "You are no longer CANTQUIT flagged.",
  "You are again susceptible to magical transport as your solidity fades.",
  "You feel more susceptible to damage.", // 150
  "!ALIGN_GOOD!",
  "!ALIGN_EVIL!",
  "Your protective aegis dissipates.",
  "Your unholy aegis dissipates.",
  "Your resistance to $Bmagic$R fades.",
  "!EAGLE_EYE!",
  "!UNUSED!",
  "!UNUSED!",
  "!UNUSED!",
  "!UNUSED!", // 160
  "You feel dumber.",
  "The protective intervention of the gods has ended."
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
    4,  /* Tundra */
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
L(400 M),   L(550 M),   L(700 M),   L(850 M),   L(1000 M), // level 55
L(1200 M),  L(1400 M),  L(1600 M),  L(1800 M),  L(2000 M), // level 60
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
    "NOHUNT",
    "\n"
};

// new obj flags
char *extra_bits[] =
{
    "GLOWING",
    "HUMMING",
    "DARK",
    "LOCK",
    "ANY-CLASS",
    "INVISIBLE",
    "MAGICAL",
    "CURSED",
    "BLESSED",
    "ANTI-GOOD",
    "ANTI-EVIL",
    "ANTI-NEUTRAL",
    "WARRIOR",
    "MAGE",
    "THIEF",
    "CLERIC",
    "PALADIN",
    "ANTI-PALADIN",
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
    "PC_CORPSE_LOOTED",
    "NO_SCRAP",
    "\n"
};

char *size_bitfields[] =
{
    "ANY",
    "SMALL",
    "MEDIUM",
    "LARGE",
    "\n"
};

char *size_bits[] =
{
    "Any race",
    "Small races",
    "Medium races",
    "Large races",
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

char *time_look[] =
{
    "night time",
    "sun rise",
    "day time",
    "sun set",
    "/n"
};

char *sky_look[] =
{
    "cloudless",
    "cloudy",
    "rainy",
    "pouring rain",
    "lit by flashes of lightning",
    "/n"
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
    "Secondary Wield",
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
    "AMBUSH_ALERT",
    "FEARLESS",
    "NO_PARA",
    "NO_CIRCLE",
    "NO_BEHEAD",
    "BOUNT_SONNET_HUNGER",
    "BOUNT_SONNET_THIRST",
    "CMAST_WEAKEN",
    "RESERVEDBAD",
    "BULLRUSHCD",
    "CRIPPLE",
    "CHAMPION",
    "BLACKJACK",
    "NO_REGEN",
    "ACID_SHIELD",
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
    "WEP BLIND",
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
    "FREEDOM FROM HUNGER",
    "FREEDOM FROM THIRST",
    "AFF BLIND",
    "WATERBREATHING",
    "DETECT MAGIC",
    "\n"
};

char *pc_clss_types[] =
{
    "UNDEFINED",
    "Mage",
    "Cleric",
    "Thief",
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
    "Mage",
    "Cleric",
    "Thief",
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
    "thief",
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

// Begin abilities listings for each class.

// skills master 10011 - done and checked, Apoc
struct class_skill_defines g_skills[] = { // all-class skills

//   Ability Name       Ability File          Level     Max     Requisites
//   ------------       ------------          -----     ---     ----------
{    "consider",        SKILL_CONSIDER,         1,      90,     INTWIS },
{    "scan",            SKILL_SCAN,             3,      90,     CONWIS },
{    "switch",          SKILL_SWITCH,           5,      90,     DEXINT },
{    "rapid join",      SKILL_FASTJOIN,         7,      90,     STRDEX },
{    "release",         SKILL_RELEASE,          10,     90,     INTWIS },
{    "\n",              0,                      1,      0,      0 }
};

// warrior 3023 guildmaster - done and checked, Apoc
struct class_skill_defines w_skills[] = { // warrior skills

//   Ability Name       Ability File            Level  Max  Requisite
//   ------------       ------------            -----  ---  ---------
{    "kick",            SKILL_KICK,               1,    80,     STRDEX },
{    "bash",            SKILL_BASH,               2,    85,     STRDEX },
{    "redirect",        SKILL_REDIRECT,           4,    90,     DEXINT },
{    "rescue",          SKILL_RESCUE,             5,    70,     DEXINT },
{    "double",          SKILL_SECOND_ATTACK,      7,    90,     STRDEX },
{    "disarm",          SKILL_DISARM,             10,   70,     DEXINT },
{    "headbutt",        SKILL_HEADBUTT,           12,   80,     STRDEX },
{    "shield block",    SKILL_SHIELDBLOCK,        15,   85,     STRCON },
{    "retreat",         SKILL_RETREAT,            17,  100,     DEXINT },
{    "frenzy",          SKILL_FRENZY,             18,   80,     STRCON },
{    "parry",           SKILL_PARRY,              20,   90,     STRCON },
{    "blindfighting",   SKILL_BLINDFIGHTING,      21,   80,     STRCON },
{"enhanced regeneration",SKILL_ENHANCED_REGEN,    22,   80,     STRCON },
{    "triple",          SKILL_THIRD_ATTACK,       23,   90,     STRDEX },
{    "hitall",          SKILL_HITALL,             25,   80,     STRDEX },
{    "dual wield",      SKILL_DUAL_WIELD,         28,   90,     DEXINT },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS,   30,   90,     CONWIS },
{    "crushing",        SKILL_CRUSHING_WEAPONS,   30,   90,     CONWIS },
{    "piercing",        SKILL_PIERCEING_WEAPONS,  30,   90,     CONWIS },
{    "slashing",        SKILL_SLASHING_WEAPONS,   30,   90,     CONWIS },
{    "whipping",        SKILL_WHIPPING_WEAPONS,   30,   90,     CONWIS },
{    "tactics",         SKILL_TACTICS,            31,  100,     DEXINT },
{    "archery",         SKILL_ARCHERY,            32,   70,     CONWIS },
{    "stun",            SKILL_STUN,               35,   85,     DEXINT },
{    "guard",           SKILL_GUARD,              37,  100,     STRCON },
{    "deathstroke",     SKILL_DEATHSTROKE,        39,  100,     STRDEX },
{    "riposte",         SKILL_RIPOSTE,            40,  100,     STRCON },
{    "two handers",     SKILL_TWO_HANDED_WEAPONS, 42,   85,     CONWIS },
{    "stinging",        SKILL_STINGING_WEAPONS,   43,   90,     CONWIS },
{    "skewer",          SKILL_SKEWER,             45,  100,     STRDEX },
{    "blade shield",    SKILL_BLADESHIELD,        47,  100,     STRCON },
{    "combat mastery",  SKILL_COMBAT_MASTERY,     50,  100,     DEXINT },
{    "\n",              0,                        1,    0,      0 }
};

// thief 3022 guildmaster - done and checked, Apoc
struct class_skill_defines t_skills[] = { // thief skills

//   Ability Name       Ability File          Level    Max    Requisites
//   ------------       ------------          -----    ---    ----------
{    "backstab",        SKILL_BACKSTAB,         1,      90,   STRDEX },  
{    "sneak",           SKILL_SNEAK,            2,      90,   DEXINT },  
{    "shield block",    SKILL_SHIELDBLOCK,      4,      40,   CONINT },  
{    "stalk",           SKILL_STALK,            6,     100,   DEXINT },
{    "hide",            SKILL_HIDE,             7,      90,   DEXINT },  
{    "dual wield",      SKILL_DUAL_WIELD,       10,     90,   CONINT },  
{    "trip",            SKILL_TRIP,             11,     85,   DEXINT },
{    "palm",            SKILL_PALM,             12,    100,   DEXWIS },  
{    "slip",            SKILL_SLIP,             13,    100,   DEXWIS },  
{    "dodge",           SKILL_DODGE,            15,     90,   DEXWIS },
{    "blackjack",       SKILL_BLACKJACK,        17,    100,   STRDEX },
{    "pocket",          SKILL_POCKET,           20,    100,   DEXINT },
{    "appraise",        SKILL_APPRAISE,         21,    100,   DEXWIS },
{    "pick",            SKILL_PICK_LOCK,        22,    100,   DEXWIS },  
{    "steal",           SKILL_STEAL,            25,    100,   DEXINT },
{    "blindfighting",   SKILL_BLINDFIGHTING,    28,     80,   DEXWIS },
{    "piercing",        SKILL_PIERCEING_WEAPONS,30,     90,   CONINT },
{    "slashing",        SKILL_SLASHING_WEAPONS, 30,     60,   CONINT },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS, 30,     70,   CONINT },
{    "stinging",        SKILL_STINGING_WEAPONS, 30,     85,   CONINT },  
{    "deceit",          SKILL_DECEIT,           31,    100,   DEXWIS },  
{    "circle",          SKILL_CIRCLE,           35,    100,   DEXINT },  
{    "disarm",          SKILL_DISARM,           38,     85,   STRDEX },  
{    "dual backstab",   SKILL_DUAL_BACKSTAB,    40,    100,   STRDEX },  
{    "eyegouge",        SKILL_EYEGOUGE,         42,    100,   STRDEX },  
{    "vitalstrike",     SKILL_VITAL_STRIKE,     45,    100,   STRDEX },  
{    "cripple",		SKILL_CRIPPLE,		50,    100,   STRDEX },
{    "\n",              0,                      1,      0,    0 }
};

// anti-paladin 10005 guildmaster - done and checked, Apoc
struct class_skill_defines a_skills[] = { // anti-paladin skills

//   Ability Name            Ability File           Level     Max     Requisites
//   ------------            ------------           -----     ---     ----------
{    "harmtouch",            SKILL_HARM_TOUCH,        1,     100,     CONWIS },
{    "kick",                 SKILL_KICK,              2,      70,     STRDEX },
{    "sneak",                SKILL_SNEAK,             3,      85,     DEXINT },
{    "shield block",         SKILL_SHIELDBLOCK,       5,      60,     STRDEX },
{    "infravision",          SPELL_INFRAVISION,       7,      90,     STRINT },
{    "detect good",          SPELL_DETECT_GOOD,       8,     100,     CONINT },
{    "dual wield",           SKILL_DUAL_WIELD,        10,     85,     STRDEX },
{    "shocking grasp",       SPELL_SHOCKING_GRASP,    11,    100,     STRINT },
{    "detect invisibility",  SPELL_DETECT_INVISIBLE,  12,     85,     STRINT },
{    "invisibility",         SPELL_INVISIBLE,         13,     85,     DEXINT },
{    "backstab",             SKILL_BACKSTAB,          15,     90,     DEXINT },
{    "hide",                 SKILL_HIDE,              17,     85,     DEXINT },
{    "trip",                 SKILL_TRIP,              19,     85,     DEXINT },
{    "chill touch",          SPELL_CHILL_TOUCH,       20,     85,     STRINT },
{    "double",               SKILL_SECOND_ATTACK,     22,     85,     STRDEX },
{    "dodge",                SKILL_DODGE,             23,     80,     DEXINT },
{    "vampiric touch",       SPELL_VAMPIRIC_TOUCH,    25,    100,     CONWIS },
{    "poison",               SPELL_POISON,            27,     70,     CONINT },
{    "animate dead",         SPELL_ANIMATE_DEAD,      28,     80,     CONWIS },
{    "dismiss corpse",       SPELL_DISMISS_CORPSE,    29,     90,     CONWIS },
{    "piercing",             SKILL_PIERCEING_WEAPONS, 30,     85,     STRDEX },
{    "slashing",             SKILL_SLASHING_WEAPONS,  30,     85,     STRDEX },
{    "crushing",             SKILL_CRUSHING_WEAPONS,  30,     85,     STRDEX },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,  30,     85,     STRDEX },
{    "visage of hate",       SPELL_VISAGE_OF_HATE,    31,    100,     CONINT },
{    "blindfighting",        SKILL_BLINDFIGHTING,     32,     70,     DEXINT },
{    "globe of darkness",    SPELL_GLOBE_OF_DARKNESS, 33,    100,     DEXINT },
{    "beacon",               SPELL_BEACON,            35,    100,     CONWIS },
{    "fear",                 SPELL_FEAR,              37,     85,     CONINT },
{    "dispel good",          SPELL_DISPEL_GOOD,       38,     90,     CONINT },
{    "acid shield",          SPELL_ACID_SHIELD,       40,    100,     STRINT },
{    "curse",                SPELL_CURSE,             41,     70,     CONWIS },
{    "life leech",           SPELL_LIFE_LEECH,        42,    100,     CONWIS },
{    "unholy aegis",         SPELL_U_AEGIS,           44,    100,     CONINT },
{    "protection from good", SPELL_PROTECT_FROM_GOOD, 45,     85,     CONINT },
{    "resist acid",          SPELL_RESIST_ACID,       46,     70,     STRINT },
{    "acid blast",           SPELL_ACID_BLAST,        48,    100,     STRINT },
{    "vampiric aura",        SPELL_VAMPIRIC_AURA,     50,    100,     CONWIS },
{    "\n",                   0,                       1,      0,      0 }
};

// paladin 10006 guildmaster - done and checked, Apoc
struct class_skill_defines p_skills[] = { // paladin skills

//   Ability Name            Ability File            Level     Max     Requisites 
//   ------------            ------------            -----     ---     ---------- 
{    "layhands",             SKILL_LAY_HANDS,          1,     100,     INTWIS },
{    "kick",                 SKILL_KICK,               2,      70,     STRCON },
{    "bless",                SPELL_BLESS,              3,      90,     CONWIS },
{    "double",               SKILL_SECOND_ATTACK,      5,      85,     DEXINT },
{    "shield block",         SKILL_SHIELDBLOCK,        7,      90,     CONWIS },
{    "rescue",               SKILL_RESCUE,             8,      85,     CONWIS },
{    "cure light",           SPELL_CURE_LIGHT,         9,      85,     INTWIS },
{    "dual wield",           SKILL_DUAL_WIELD,         10,     80,     STRCON },
{    "detect poison",        SPELL_DETECT_POISON,      11,     85,     STRWIS },
{    "create food",          SPELL_CREATE_FOOD,        12,     85,     INTWIS },
{    "create water",         SPELL_CREATE_WATER,       13,     85,     INTWIS },
{    "cure serious",         SPELL_CURE_SERIOUS,       15,     85,     INTWIS },
{    "detect evil",          SPELL_DETECT_EVIL,        17,    100,     STRWIS },
{    "remove poison",        SPELL_REMOVE_POISON,      18,     85,     CONWIS },
{    "detect invisibility",  SPELL_DETECT_INVISIBLE,   20,     85,     STRWIS },
{    "cure critical",        SPELL_CURE_CRITIC,        22,     85,     INTWIS },
{    "parry",                SKILL_PARRY,              23,     70,     DEXINT },
{    "bash",                 SKILL_BASH,               25,     80,     STRCON },
{    "sense life",           SPELL_SENSE_LIFE,         26,     85,     STRWIS },
{    "strength",             SPELL_STRENGTH,           28,     70,     STRWIS },
{    "earthquake",           SPELL_EARTHQUAKE,         29,     70,     STRCON },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,   30,     85,     DEXINT },
{    "slashing",             SKILL_SLASHING_WEAPONS,   30,     85,     DEXINT },
{    "crushing",             SKILL_CRUSHING_WEAPONS,   30,     85,     DEXINT },
{    "blessed halo",         SPELL_BLESSED_HALO,       31,    100,     STRWIS },
{    "triple",               SKILL_THIRD_ATTACK,       33,     80,     DEXINT },
{    "two handers",          SKILL_TWO_HANDED_WEAPONS, 35,     80,     DEXINT },
{    "heal",                 SPELL_HEAL,               37,     85,     INTWIS },
{    "harm",                 SPELL_HARM,               38,     85,     STRCON },
{    "sanctuary",            SPELL_SANCTUARY,          40,     85,     CONWIS },
{    "holy aegis",           SPELL_AEGIS,              42,    100,     STRWIS },
{    "dispel evil",          SPELL_DISPEL_EVIL,        43,     90,     STRCON },
{    "protection from evil", SPELL_PROTECT_FROM_EVIL,  45,     85,     CONWIS },
{    "resist cold",          SPELL_RESIST_COLD,        46,     70,     STRWIS },
{    "divine fury",          SPELL_DIVINE_FURY,        48,    100,     STRCON },
{    "holy aura",            SPELL_HOLY_AURA,          50,    100,     CONWIS },
{    "\n",                   0,                        1,      0,      0 }
};

// barbarian 10007 guildmaster - done and checked, Apoc
struct class_skill_defines b_skills[] = { // barbarian skills

//   Ability Name       Ability File            Level  Max   Requisites
//   ------------       ------------            -----  ---   ----------
{    "dual wield",      SKILL_DUAL_WIELD,         1,    85,  DEXCON },
{    "bash",            SKILL_BASH,               2,    90,  STRINT },
{    "kick",            SKILL_KICK,               3,    80,  STRINT },
{    "parry",           SKILL_PARRY,              5,    70,  DEXCON },
{    "double",          SKILL_SECOND_ATTACK,      8,    85,  DEXWIS },
{    "dodge",           SKILL_DODGE,  	          10,   70,  DEXCON },
{    "blood fury",      SKILL_BLOOD_FURY,         12,  100,  STRCON },
{    "crazed assault",  SKILL_CRAZED_ASSAULT,     15,  100,  STRCON },
{    "frenzy",          SKILL_FRENZY,             18,   90,  DEXCON },
{    "rage",            SKILL_RAGE,               20,  100,  STRCON },
{"enhanced regeneration",SKILL_ENHANCED_REGEN,    22,   90,  DEXCON },
{    "triple",          SKILL_THIRD_ATTACK,       25,   85,  DEXWIS },
{    "battlecry",       SKILL_BATTLECRY,          27,  100,  STRINT },
{    "blindfighting",   SKILL_BLINDFIGHTING,      28,   60,  DEXCON },
{    "whipping",        SKILL_WHIPPING_WEAPONS,   30,   85,  DEXWIS },
{    "piercing",        SKILL_PIERCEING_WEAPONS,  30,   85,  DEXWIS },
{    "slashing",        SKILL_SLASHING_WEAPONS,   30,   85,  DEXWIS },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS,   30,   85,  DEXWIS },
{    "crushing",        SKILL_CRUSHING_WEAPONS,   30,   85,  DEXWIS },
{    "ferocity",        SKILL_FEROCITY,           31,  100,  STRINT },
{    "headbutt",        SKILL_HEADBUTT,           33,   90,  STRINT },
{    "two handers",     SKILL_TWO_HANDED_WEAPONS, 35,   90,  DEXWIS },
{    "archery",         SKILL_ARCHERY,            38,   80,  DEXWIS },
{    "berserk",         SKILL_BERSERK,            40,  100,  STRCON },
{    "hitall",          SKILL_HITALL,             45,   90,  STRCON },
{    "magic resistance", SKILL_MAGIC_RESIST,      47,  100,  DEXCON },
{    "knockback",       SKILL_KNOCKBACK,          48,  100,  STRINT },
{    "bullrush",        SKILL_BULLRUSH,           50,  100,  STRCON },
{    "\n",              0,                        1,    0,   0 }
};

// monk 10008 guildmaster - done and checked, Apoc
struct class_skill_defines k_skills[] = { // monk skills

//   Ability Name       Ability File          Level    Max      Requisites
//   ------------       ------------          -----    ---      ----------
{    "kick",            SKILL_KICK,             1,      90,     STRDEX },
{    "dodge",           SKILL_DODGE,            2,      80,     DEXWIS },
{    "redirect",        SKILL_REDIRECT,         3,      85,     DEXWIS },
{    "trip",            SKILL_TRIP,             5,      70,     DEXWIS },
{    "purify",          KI_PURIFY+KI_OFFSET,    8,     100,     CONWIS },
{    "martial defense", SKILL_DEFENSE,          10,    100,     DEXWIS },
{    "rescue",          SKILL_RESCUE,           12,     80,     DEXWIS },
{    "punch",           KI_PUNCH+KI_OFFSET,     15,    100,     STRDEX },
{    "eagleclaw",       SKILL_EAGLE_CLAW,       17,    100,     CONWIS },
{    "dual wield",      SKILL_DUAL_WIELD,       20,     50,     DEXWIS },
{    "sense",           KI_SENSE+KI_OFFSET,     21,    100,     CONWIS },
{    "stance",          KI_STANCE+KI_OFFSET,    24,    100,     CONWIS },
{    "speed",           KI_SPEED+KI_OFFSET,     27,    100,     STRDEX },
{    "whipping",        SKILL_WHIPPING_WEAPONS, 30,     60,     STRDEX },
{    "hand to hand",    SKILL_HAND_TO_HAND,     30,    100,     STRDEX },
{    "agility",         KI_AGILITY+KI_OFFSET,   31,    100,     STRDEX },
{    "stun",            SKILL_STUN,             33,     90,     CONINT },
{    "storm",           KI_STORM+KI_OFFSET,     35,    100,     CONINT },
{    "blindfighting",   SKILL_BLINDFIGHTING,    38,     90,     CONWIS },
{    "quiver",          SKILL_QUIVERING_PALM,   40,    100,     CONINT },
{    "blast",           KI_BLAST+KI_OFFSET,     45,    100,     CONINT },
{    "disrupt",         KI_DISRUPT+KI_OFFSET,   47,    100,     CONINT },
{    "meditation",      KI_MEDITATION+KI_OFFSET,50,    100,     CONWIS },
{    "\n",              0,                      1,      0,      0 }
};

// ranger 10013 guildmaster - done and checked, Apoc
struct class_skill_defines r_skills[] = { // ranger skills

//   Ability Name       Ability File           Level     Max     Requisites 
//   ------------       ------------           -----     ---     ---------- 
{    "bee sting",       SPELL_BEE_STING,         1,     100,     STRCON },
{    "hide",            SKILL_HIDE,              2,      80,     CONWIS },
{    "kick",            SKILL_KICK,              3,      70,     STRDEX },
{    "dual wield",      SKILL_DUAL_WIELD,        5,      90,     STRDEX },
{    "redirect",        SKILL_REDIRECT,          7,      80,     CONWIS },
{    "eyes of the owl", SPELL_EYES_OF_THE_OWL,   8,      85,     STRCON },
{    "sense life",      SPELL_SENSE_LIFE,        9,      85,     CONWIS },
{    "dodge",    	SKILL_DODGE,    	 10,     60,     CONWIS },
{    "tame",            SKILL_TAME,              11,    100,     STRCON },
{    "double",          SKILL_SECOND_ATTACK,     12,     85,     STRDEX },
{    "feline agility",  SPELL_FELINE_AGILITY,    14,    100,     STRCON },
{    "bee swarm",       SPELL_BEE_SWARM,         15,    100,     STRCON },
{    "forage",          SKILL_FORAGE,            16,     85,     INTWIS },
{    "entangle",        SPELL_ENTANGLE,          18,     85,     INTWIS },
{    "archery",         SKILL_ARCHERY,           20,     90,     DEXINT },
{    "blindfighting",   SKILL_BLINDFIGHTING,     21,     85,     CONWIS },
{    "parry",           SKILL_PARRY,             22,     80,     CONWIS },
{    "herb lore",       SPELL_HERB_LORE,         23,     85,     INTWIS },
{    "poison",          SPELL_POISON,            25,     85,     INTWIS },
{    "tempest arrows",  SKILL_TEMPEST_ARROW,     26,    100,     DEXINT },
{    "track",           SKILL_TRACK,             28,    100,     DEXINT },
{    "barkskin",        SPELL_BARKSKIN,          29,     85,     INTWIS },
{    "piercing",        SKILL_PIERCEING_WEAPONS, 30,     85,     STRDEX },
{    "slashing",        SKILL_SLASHING_WEAPONS,  30,     85,     STRDEX },
{    "whipping",        SKILL_WHIPPING_WEAPONS,  30,     90,     STRDEX },
{    "ice arrows",      SKILL_ICE_ARROW,         31,    100,     DEXINT },
{    "rescue",          SKILL_RESCUE,            32,     85,     STRDEX },
{    "trip",            SKILL_TRIP,              33,     85,     STRDEX },
{    "ambush",          SKILL_AMBUSH,            35,    100,     DEXINT },
{    "fire arrows",     SKILL_FIRE_ARROW,        36,    100,     DEXINT },
{    "call follower",   SPELL_CALL_FOLLOWER,     38,    100,     STRCON },
{    "stun",            SKILL_STUN,              40,     80,     INTWIS },
{    "granite arrows",  SKILL_GRANITE_ARROW,     41,    100,     DEXINT },
{    "disarm",          SKILL_DISARM,            42,     80,     CONWIS },
{    "staunchblood",    SPELL_STAUNCHBLOOD,      44,     85,     CONWIS },
{    "forest meld",     SPELL_FOREST_MELD,       45,     85,     INTWIS },
{    "camouflage",      SPELL_CAMOUFLAGE,        46,     85,     INTWIS },
{    "creeping death",  SPELL_CREEPING_DEATH,    48,    100,     STRCON },
{ "natural selection",	SKILL_NAT_SELECT,	 50,	100,	 DEXINT },
{      "\n",            0,                       1,      0,      0 }
};

// bard 3204 guildmaster - done and checked, Apoc
struct class_skill_defines d_skills[] = { // bard skills

// Ability Name            Ability File                  Level    Max      Requisites
// ------------            ------------                  -----    ---      ----------
{ "whistle sharp",         SKILL_SONG_WHISTLE_SHARP,       1,     100,     DEXCON },
{ "stop",                  SKILL_SONG_STOP,                2,     100,     CONINT },
{ "irresistable ditty",    SKILL_SONG_UNRESIST_DITTY,      3,     100,     INTWIS },
{ "dodge",                 SKILL_DODGE,                    5,      80,     STRDEX },
{ "hide",                  SKILL_HIDE,                     7,      70,     STRDEX },
{ "travelling march",      SKILL_SONG_TRAVELING_MARCH,     9,     100,     CONWIS },
{ "trip",                  SKILL_TRIP,                     10,     70,     STRDEX },
{ "bountiful sonnet",      SKILL_SONG_BOUNT_SONNET,        12,    100,     CONWIS },
{ "healing melody",        SKILL_SONG_HEALING_MELODY,      13,    100,     CONWIS },
{ "glitter dust",          SKILL_SONG_GLITTER_DUST,        15,    100,     INTWIS },
{ "synchronous chord",     SKILL_SONG_SYNC_CHORD,          17,    100,     INTWIS },
{ "sticky lullaby",        SKILL_SONG_STICKY_LULL,         18,    100,     DEXCON },
{ "flight of the bumblebee", SKILL_SONG_FLIGHT_OF_BEE,     20,    100,     CONWIS },
{ "note of knowledge",     SKILL_SONG_NOTE_OF_KNOWLEDGE,   21,    100,     INTWIS },
{ "fanatical fanfare",     SKILL_SONG_FANATICAL_FANFARE,   23,    100,     CONWIS },
{ "revealing staccato",    SKILL_SONG_REVEAL_STACATO,      25,    100,     INTWIS },
{ "double",                SKILL_SECOND_ATTACK,            26,     80,     STRDEX },
{ "terrible clef",         SKILL_SONG_TERRIBLE_CLEF,       28,    100,     DEXCON },
{ "piercing",              SKILL_PIERCEING_WEAPONS,        30,     70,     STRDEX },
{ "slashing",              SKILL_SLASHING_WEAPONS,         30,     70,     STRDEX },
{ "stinging",              SKILL_STINGING_WEAPONS,         30,     85,     STRDEX },
{ "whipping",              SKILL_WHIPPING_WEAPONS,         30,     85,     STRDEX },
{ "soothing rememberance", SKILL_SONG_SOOTHING_REMEM,      31,    100,     CONWIS },
{ "searching song",        SKILL_SONG_SEARCHING_SONG,      32,    100,     INTWIS },
{ "dischordant dirge",     SKILL_SONG_DISCHORDANT_DIRGE,   34,    100,     CONINT },
{ "insane chant",          SKILL_SONG_INSANE_CHANT,        35,    100,     CONINT },
{ "jig of alacrity",       SKILL_SONG_JIG_OF_ALACRITY,     38,    100,     CONWIS },
{ "vigilant siren",        SKILL_SONG_VIGILANT_SIREN,      40,    100,     CONWIS },
{ "forgetful rhythm",      SKILL_SONG_FORGETFUL_RHYTHM,    42,    100,     CONINT },
{ "disarming limerick",    SKILL_SONG_DISARMING_LIMERICK,  43,    100,     CONINT },
{ "astral chanty",         SKILL_SONG_ASTRAL_CHANTY,       45,    100,     INTWIS },
{ "crushing crescendo",    SKILL_SONG_CRUSHING_CRESCENDO,  46,    100,     DEXCON },
{ "shattering resonance",  SKILL_SONG_SHATTERING_RESO,     48,    100,     CONINT },
{ "mountain king's charge",SKILL_SONG_MKING_CHARGE,        49,    100,     DEXCON },
{ "hypnotic harmony",      SKILL_SONG_HYPNOTIC_HARMONY,    50,    100,     CONINT },
{ "\n",                    0,                              1,      0,      0 }
};


// druid 3203 guildmaster - done and checked, Apoc
struct class_skill_defines u_skills[] = { // druid skills

//   Ability Name            Ability File              Level     Max     Requisites
//   ------------            ------------              -----     ---     ----------
{    "blue bird",            SPELL_BLUE_BIRD,            1,     100,     STRDEX },  
{    "eyes of the owl",      SPELL_EYES_OF_THE_OWL,      2,      90,     DEXWIS },
{    "cure light",           SPELL_CURE_LIGHT,           3,      85,     INTWIS },
{    "create water",         SPELL_CREATE_WATER,         5,      90,     DEXWIS },
{    "attrition",            SPELL_ATTRITION,            6,     100,     CONWIS },
{    "natures lore",         SKILL_NATURES_LORE,         7,     100,     DEXWIS },
{    "create food",          SPELL_CREATE_FOOD,          8,      90,     DEXWIS },
{    "sense life",           SPELL_SENSE_LIFE,           10,     90,     DEXWIS },
{    "weaken",               SPELL_WEAKEN,               11,    100,     CONWIS },
{    "cure serious",         SPELL_CURE_SERIOUS,         12,     85,     INTWIS },
{    "resist cold",          SPELL_RESIST_COLD,          13,     90,     STRCON },
{    "camouflage",           SPELL_CAMOUFLAGE,           14,     90,     STRCON },
{    "oaken fortitude",      SPELL_OAKEN_FORTITUDE,      15,    100,     INTWIS },
{    "water breathing",      SPELL_WATER_BREATHING,      17,    100,     CONINT },
{    "resist acid",          SPELL_RESIST_ACID,          18,     90,     STRCON },
{    "stoneshield",          SPELL_STONE_SHIELD,         20,    100,     CONINT },
{    "poison",               SPELL_POISON,               21,     90,     CONWIS },
{    "staunchblood",         SPELL_STAUNCHBLOOD,         22,     90,     INTWIS },
{    "cure critical",        SPELL_CURE_CRITIC,          23,     85,     INTWIS },
{    "call familiar",        SPELL_SUMMON_FAMILIAR,      25,     90,     DEXWIS },
{    "dismiss familiar",     SPELL_DISMISS_FAMILIAR,     26,     90,     DEXWIS },
{    "debility",             SPELL_DEBILITY,             27,    100,     CONWIS },
{    "drown",                SPELL_DROWN,                28,    100,     STRDEX },
{    "entangle",             SPELL_ENTANGLE,             29,     90,     CONWIS },
{    "whipping",             SKILL_WHIPPING_WEAPONS,     30,     60,     STRDEX },
{    "crushing",             SKILL_CRUSHING_WEAPONS,     30,     60,     STRDEX },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,     30,     60,     STRDEX },
{    "resist energy",        SPELL_RESIST_ENERGY,        30,     90,     STRCON },
{    "rapid mend",           SPELL_RAPID_MEND,           31,    100,     INTWIS },
{    "herb lore",            SPELL_HERB_LORE,            32,     90,     INTWIS },
{    "lighted path",         SPELL_LIGHTED_PATH,         33,    100,     DEXWIS },
{    "curse",                SPELL_CURSE,                34,     90,     CONWIS },
{    "sun ray",              SPELL_SUN_RAY,              35,    100,     STRDEX },
{    "call lightning",       SPELL_CALL_LIGHTNING,       35,    100,     STRDEX },
{    "control weather",      SPELL_CONTROL_WEATHER,      36,     90,     CONINT },
{    "barkskin",             SPELL_BARKSKIN,             37,     90,     STRCON },
{    "iron roots",           SPELL_IRON_ROOTS,           38,    100,     STRCON },
{    "resist fire",          SPELL_RESIST_FIRE,          39,     90,     STRCON },
{    "earthquake",           SPELL_EARTHQUAKE,           40,     90,     CONINT },
{    "lightning shield",     SPELL_LIGHTNING_SHIELD,     41,    100,     CONINT },
{    "blindness",            SPELL_BLINDNESS,            42,    100,     CONWIS },
{    "forage",               SKILL_FORAGE,               43,     90,     DEXWIS },
{    "spiritwalk",           SPELL_GHOSTWALK,            43,    100,     CONINT },
{    "stoneskin",            SPELL_STONE_SKIN,           44,     90,     STRCON },
{    "power heal",           SPELL_POWER_HEAL,           45,    100,     INTWIS },
{    "forest meld",          SPELL_FOREST_MELD,          46,     90,     DEXWIS },
{    "greater stoneshield",  SPELL_GREATER_STONE_SHIELD, 47,    100,     CONINT },
{    "colour spray",         SPELL_COLOUR_SPRAY,         48,    100,     STRDEX },
{    "summon",               SPELL_SUMMON,               49,    100,     STRCON },
{    "conjure elemental",    SPELL_CONJURE_ELEMENTAL,    50,    100,     CONINT },
{    "\n",                   0,                          1,      0,      0 }
};


// cleric 3021 guildmaster - done and checked, Apoc
struct class_skill_defines c_skills[] = { // cleric skills

//   Ability Name            Ability File           Level     Max     Requisites
//   ------------            ------------           -----     ---     ----------
{    "cure light",           SPELL_CURE_LIGHT,        1,      90,     INTWIS },     
{    "cause light",          SPELL_CAUSE_LIGHT,       2,     100,     STRINT },
{    "armor",                SPELL_ARMOR,             3,      90,     CONWIS },
{    "continual light",      SPELL_CONT_LIGHT,        4,      85,     DEXINT },
{    "detect poison",        SPELL_DETECT_POISON,     5,      90,     DEXINT },
{    "know alignment",       SPELL_KNOW_ALIGNMENT,    5,     100,     DEXINT },
{    "detect magic",         SPELL_DETECT_MAGIC,      6,      90,     DEXINT },
{    "refresh",              SPELL_REFRESH,           7,      85,     DEXWIS },
{    "bless",                SPELL_BLESS,             8,      90,     DEXWIS },
{    "create water",         SPELL_CREATE_WATER,      9,      70,     DEXWIS },
{    "create food",          SPELL_CREATE_FOOD,       9,      70,     DEXWIS },
{    "cure serious",         SPELL_CURE_SERIOUS,      10,     90,     INTWIS },
{    "cause serious",        SPELL_CAUSE_SERIOUS,     11,    100,     STRINT },
{    "detect invisibility",  SPELL_DETECT_INVISIBLE,  12,     85,     DEXINT },
{    "remove poison",        SPELL_REMOVE_POISON,     13,     90,     INTWIS },
{    "dispel minor",         SPELL_DISPEL_MINOR,      14,     90,     STRCON },
{    "dual wield",           SKILL_DUAL_WIELD,        15,     40,     STRINT },
{    "remove blind",         SPELL_REMOVE_BLIND,      16,    100,     INTWIS },
{    "sense life",           SPELL_SENSE_LIFE,        17,     85,     DEXINT },
{    "sanctuary",            SPELL_SANCTUARY,         18,     90,     CONWIS },
{    "remove curse",         SPELL_REMOVE_CURSE,      19,    100,     INTWIS },
{    "cure critical",        SPELL_CURE_CRITIC,       20,     90,     INTWIS },
{    "cause critical",       SPELL_CAUSE_CRITICAL,    21,    100,     STRINT },
{    "remove paralysis",     SPELL_REMOVE_PARALYSIS,  22,    100,     INTWIS },
{    "locate object",        SPELL_LOCATE_OBJECT,     23,     85,     DEXINT },
{    "word of recall",       SPELL_WORD_OF_RECALL,    24,     90,     DEXWIS },
{    "animate dead",         SPELL_ANIMATE_DEAD,      25,     90,     STRCON },
{    "dismiss corpse",       SPELL_DISMISS_CORPSE,    26,     90,     STRCON },
{    "group fly",            SPELL_GROUP_FLY,         27,    100,     DEXWIS },
{    "heal",                 SPELL_HEAL,              28,     90,     INTWIS },
{    "harm",                 SPELL_HARM,              29,     90,     STRINT },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,  30,     70,     STRINT },
{    "crushing",             SKILL_CRUSHING_WEAPONS,  30,     70,     STRINT },
{    "heroes feast",         SPELL_HEROES_FEAST,      31,    100,     DEXWIS },
{    "dispel evil",          SPELL_DISPEL_EVIL,       32,     90,     STRCON },
{    "dispel good",          SPELL_DISPEL_GOOD,       33,     90,     STRCON },
{    "iridescent aura",      SPELL_IRIDESCENT_AURA,   35,    100,     CONWIS },
{    "protection from evil", SPELL_PROTECT_FROM_EVIL, 36,     90,     CONWIS },
{    "protection from good", SPELL_PROTECT_FROM_GOOD, 37,     90,     CONWIS },
{    "portal",               SPELL_PORTAL,            38,     90,     DEXWIS },
{    "true sight",           SPELL_TRUE_SIGHT,        39,     90,     DEXINT },
{    "full heal",            SPELL_FULL_HEAL,         40,    100,     INTWIS },
{    "power harm",           SPELL_POWER_HARM,        41,    100,     STRINT },
{    "resist magic",         SPELL_RESIST_MAGIC,      43,     85,     CONWIS },
{    "resist energy",        SPELL_RESIST_ENERGY,     44,     85,     CONWIS },
{    "dispel magic",         SPELL_DISPEL_MAGIC,      45,     90,     STRCON },
{    "flamestrike",          SPELL_FLAMESTRIKE,       46,    100,     STRINT },
{    "group recall",         SPELL_GROUP_RECALL,      47,    100,     DEXWIS },
{    "heal spray",           SPELL_HEAL_SPRAY,        48,    100,     STRCON },
{    "group sanctuary",      SPELL_GROUP_SANC,        49,    100,     CONWIS },
{    "divine intervention",  SPELL_DIVINE_INTER,      50,    100,     CONWIS },
{    "\n",                   0,                        1,      0,     0 }
};

// mage 3020 guildmaster - done and checked, Apoc
struct class_skill_defines m_skills[] = { // mage skills

//   Ability Name           Ability File           Level     Max     Requisites   
//   ------------           ------------           -----     ---     ----------   
{    "magic missile",       SPELL_MAGIC_MISSILE,     1,     100,     STRINT }, 
{    "ventriloquate",       SPELL_VENTRILOQUATE,     2,     100,     DEXWIS },
{    "clarity",             SPELL_CLARITY,           2,     100,     DEXINT },
{    "detect magic",        SPELL_DETECT_MAGIC,      3,      90,     INTWIS },
{    "detect invisibility", SPELL_DETECT_INVISIBLE,  4,      90,     INTWIS },
{    "invisibility",        SPELL_INVISIBLE,         5,      90,     DEXWIS },
{    "burning hands",       SPELL_BURNING_HANDS,     6,     100,     STRINT },
{    "armor",               SPELL_ARMOR,             7,      85,     CONWIS },
{    "continual light",     SPELL_CONT_LIGHT,        8,      90,     STRCON },
{    "refresh",             SPELL_REFRESH,           9,      90,     DEXWIS },
{    "lightning bolt",      SPELL_LIGHTNING_BOLT,    10,    100,     STRINT },
{    "infravision",         SPELL_INFRAVISION,       11,     90,     INTWIS },
{    "fly",                 SPELL_FLY,               12,    100,     DEXINT },
{    "strength",            SPELL_STRENGTH,          13,     90,     DEXINT },
{    "fear",                SPELL_FEAR,              15,     90,     CONWIS },
{    "identify",            SPELL_IDENTIFY,          16,    100,     INTWIS },
{    "locate object",       SPELL_LOCATE_OBJECT,     17,     90,     INTWIS },
{    "call familiar",       SPELL_SUMMON_FAMILIAR,   18,     85,     STRCON },
{    "dismiss familiar",    SPELL_DISMISS_FAMILIAR,  18,     85,     STRCON },
{    "chill touch",         SPELL_CHILL_TOUCH,       20,     90,     DEXWIS },
{    "shield",              SPELL_SHIELD,            21,    100,     CONWIS },
{    "souldrain",           SPELL_SOULDRAIN,         22,    100,     DEXWIS },
{    "dispel minor",        SPELL_DISPEL_MINOR,      25,     90,     CONWIS },
{    "mass invisibility",   SPELL_MASS_INVISIBILITY, 26,    100,     DEXWIS },
{    "firestorm",           SPELL_FIRESTORM,         27,    100,     STRINT },
{    "portal",              SPELL_PORTAL,            28,     90,     STRCON },
{    "fireball",            SPELL_FIREBALL,          29,    100,     STRINT },
{    "focused repelance",   SKILL_FOCUSED_REPELANCE, 30,    100,     CONWIS },
{    "piercing",            SKILL_PIERCEING_WEAPONS, 30,     50,     DEXINT },
{    "bludgeoning",         SKILL_BLUDGEON_WEAPONS,  30,     50,     DEXINT },
{    "resist magic",        SPELL_RESIST_MAGIC,      31,     90,     CONWIS },
{    "haste",               SPELL_HASTE,             33,    100,     DEXINT },
{    "true sight",          SPELL_TRUE_SIGHT,        34,     90,     INTWIS },
{    "dispel magic",        SPELL_DISPEL_MAGIC,      35,     90,     CONWIS },
{    "resist fire",         SPELL_RESIST_FIRE,       36,     70,     CONWIS },
{    "wizard eye",          SPELL_WIZARD_EYE,        37,    100,     INTWIS },
{    "teleport",            SPELL_TELEPORT,          38,    100,     DEXWIS },
{    "stoneskin",           SPELL_STONE_SKIN,        39,     70,     CONWIS },
{    "meteor swarm",        SPELL_METEOR_SWARM,      40,    100,     STRCON },
{    "word of recall",      SPELL_WORD_OF_RECALL,    42,     90,     DEXWIS },
{    "create golem",        SPELL_CREATE_GOLEM,      43,    100,     STRCON },
{    "release golem",       SPELL_RELEASE_GOLEM,     43,    100,     STRCON },
{    "mend golem",          SPELL_MEND_GOLEM,        44,    100,     STRCON },
{    "hellstream",          SPELL_HELLSTREAM,        45,    100,     STRINT },
{    "fireshield",          SPELL_FIRESHIELD,        47,    100,     STRINT },
{    "paralyze",            SPELL_PARALYZE,          48,    100,     DEXINT },
{    "solar gate",          SPELL_SOLAR_GATE,        49,    100,     STRINT },
{    "spellcraft",          SKILL_SPELLCRAFT,        50,    100,     INTWIS },
{    "\n",                  0,                       1,      0,      0 }
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
  "ogrish",
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
    "Ogre",
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
    "Ogr",
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
//Name,        Plural,      Parts, Immun,    Res,     Sus, Hates/Fears,  Frnd, MinWt, MaxWt, MinHt, MaxHt,  Affects, UnarmedHitType
{ "NPC",       "NPC",         63,    0,       0,       0,       0,         0,  150,   150,    72,    72,    AFF_IGNORE_WEAPON_WEIGHT, "hit"},
{ "Human",     "Humans",      63,    0,       0,       0,  1<<22|1<<29,    0,  125,   175,    66,    78,    AFF_IGNORE_WEAPON_WEIGHT, "punch" },
{ "Elf",       "Elves",       63,    0,     264,       0,   1<<7|1<<2,     0,  176,   215,    85,   102,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "hook"},
{ "Dwarf",     "Dwarves",     63,    0,     128,       0,   1<<1|1<<5,     0,   85,   124,    48,    65,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "uppercut" },
{ "Hobbit",    "Hobbits",     63,    0,       0,       0,   1<<7|1<<8,     0,   35,    74,    24,    41,    AFF_IGNORE_WEAPON_WEIGHT|AFF_HIDE, "jab" },
{ "Pixie",     "Pixies",      63,    0,       0,       0,   1<<5|1<<8,     0,   25,    64,    18,    35,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_FLYING, "bite" },
{ "Ogre",      "Ogres",       63,    0,       0,       0,   1<<2|1<<4,     0,  236,   275,   103,   120,    AFF_IGNORE_WEAPON_WEIGHT, "smash" },
{ "Gnome",     "Gnomes",      63,    0,       0,       0,   1<<9|1<<11,    0,   75,   114,    42,    59,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "cuff" },
{ "Orc",       "Orcs",        63,    0,       0,       0,   1<<1|1<<3,     0,  186,   225,    79,    96,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "punch" },
{ "Troll",     "Trolls",      63,    0,     128,      80,   1<<3|1<<4,     0,  226,   265,   109,   126,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "claw" },
{ "Goblin",    "Goblins",     63,    0,       0,       0,   1<<6|1<<11,    0,  100,   100,    60,    60,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "jab" }, 
{ "Reptile",   "Reptiles",    59,    0,       0,     520,       0,         0,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT, "strike" },
{ "Dragon",    "Dragons",     91,    0,     268,       0,   1<<6|1<<9,     0,  500,  2000,   144,   240,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE|AFF_FLYING|AFF_TRUE_SIGHT|AFF_SOLIDITY, "maul" },
{ "Snake",     "Snakes",       3,    0,       1, 2097160,   32768,      4096,    5,    50,     2,     8,    AFF_IGNORE_WEAPON_WEIGHT, "bite" },
{ "Horse",     "Horses",      19,    0,       0,       0,    2048,         3,  400,   600,    72,    84,    AFF_IGNORE_WEAPON_WEIGHT, "hoof" },
{ "Bird",      "Birds",       63,    0, 2097152,       0,   65536,         0,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_FLYING, "peck" },
{ "Rodent",    "Rodents",     59,    0,       0,     128,    4096,         0,    5,    25,     6,    24,    AFF_IGNORE_WEAPON_WEIGHT, "bite"},
{ "Fish",      "Fishes",       3,    0, 4194304,       0,   16384,         0,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT, "bite" },
{ "Arachnid",  "Arachnids",   19,    0,     128,       0,  262144,    131072,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE, "bite" },
{ "Insect",    "Insects",     19,    0,       0,       0,  131072,    262144,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "bite" },
{ "Slime",     "Slimes",       2,    1,      64,     528,       0,         0,   30,   120,     1,    12,    AFF_IGNORE_WEAPON_WEIGHT, "smother" },
{ "Animal",    "Animals",     27,    0,       0,     128, 8388608,         0,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_SENSE_LIFE, "bite" },
{ "Plant",     "Plants",      63,    0, 4194304,      16, 1048576,         0,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT, "choke" },
{ "Enfan",     "Enfans",      63,    0,       0,     512,       1,   4194304,  200,   250,   100,   124,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED, "claw" },
{ "Undead",    "Undead",      63,  392,       0,      64,       0,   8388608,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE, "punch" },
{ "Ghost",     "Ghosts",      63,  904,      80,      36,       0,  16777216,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE|AFF_FLYING|AFF_INVISIBLE, "scream" },
{ "Golem",     "Golems",      63,  256,     128,      64,       0,         0,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT, "smash" },
{ "Elemental", "Elementals",  63,    0,    1024,      12,       0,  67108864,  100,   400,    84,   120,    AFF_IGNORE_WEAPON_WEIGHT|AFF_SOLIDITY, "punch" },
{ "Planar",    "Planar",      63,   64,       4,      32,       0,         0,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE|AFF_SOLIDITY|AFF_REFLECT, "punch" },
{ "Demon",     "Demons",     127,   16,       8, 1048576,    1023,         0,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE, "maul" },
{ "Yrnali",    "Yrnali",      63,   16,       8, 1048576,       1, 536870912,  150,   300,    72,    96,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE, "maul" },
{ "Immortal",  "Immortals",  127, 1288, 1048576,       0,       0,         0,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_DETECT_INVISIBLE|AFF_FLYING|AFF_TRUE_SIGHT|AFF_SOLIDITY|AFF_SANCTUARY, "ethereal strike" },
{ "Feline",    "Felines",     27,    0,       0,     128, 8388608,         0,    0,     0,     0,     0,    AFF_IGNORE_WEAPON_WEIGHT|AFF_INFRARED|AFF_SENSE_LIFE|AFF_SNEAK, "claw" }
}; 

int mob_race_mod[][5] =
/* str, dex, con, int, wis */
{
{  15,  15,  15,  15,  15 }, // NPC
{  18,  18,  18,  18,  18 }, // human
{  17,  20,  16,  20,  17 }, // elf
{  20,  15,  21,  15,  19 }, // dwarf
{  15,  23,  16,  18,  18 }, // hobbit
{  13,  21,  13,  23,  20 }, // pixie
{  23,  16,  20,  15,  16 }, // ogre
{  15,  15,  17,  20,  23 }, // gnome
{  20,  18,  19,  17,  16 }, // orc
{  21,  18,  23,  13,  15 }, // troll
{  19,  15,  21,  10,  15 }, // goblin
{  20,  20,  20,  10,  10 }, // reptile
{  28,  28,  28,  28,  28 }, // dragon
{  10,  20,  10,  20,  20 }, // snake
{  20,  20,  15,  10,  15 }, // horse
{  10,  30,  10,  15,  15 }, // bird
{  15,  20,  20,  15,  10 }, // rodent
{  15,  30,  15,  10,  10 }, // fish
{  15,  25,  20,  10,  10 }, // arachnid
{  20,  25,  15,  10,  10 }, // insect
{  15,  15,  30,  10,  10 }, // slime
{  18,  21,  18,  13,  10 }, // animal
{  25,  10,  20,  10,  15 }, // tree
{  19,  19,  19,  19,  19 }, // enfan
{  20,  20,  20,  10,  10 }, // undead
{  10,  25,  15,  15,  15 }, // ghost
{  20,  15,  20,  15,  10 }, // golem
{  20,  20,  20,  10,  10 }, // element
{  20,  20,  20,  20,  20 }, // planar
{  20,  20,  20,  20,  20 }, // demon
{  21,  21,  21,  21,  21 }, // yrnali
{  30,  30,  30,  30,  30 }, // immortal
{  16,  24,  20,  15,  15 }, // feline
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
    "NOHEADBUTT",
    "NOATTACKS",
    "nodontuse",
    "IS_SWARM",
    "IS_TINY",
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
    "CHARMIEJOIN",
    "NOTAX",
    "GUIDE",
    "GUIDE_TOG",
    "NEWS",
    "50PLUS",
    "ASCII",
    "DAMAGE",
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

/* [level] backstab multiplyer (thieves / antis only) */
ubyte backstab_mult[71] =
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
   11,
   11,
   11,
   11,
   11,    /* 55 */
   11,
   11,
   11,
   11,
   11,    /* 60 */
   12,
   12,
   13,
   14,    /* 65 */
   15,
   16,
   17,
   18,
   19,   /* 70 */
   20
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
/* LVL       EXP    HP   HIT DAM   AC    GOLD */
/*  0 */{      5,     1,  1,  1,  100,       0},
/*  1 */{    500,     5,  1,  1,  100,     250},
/*  2 */{    750,     7,  2,  1,  100,     500},
/*  3 */{   1000,    10,  3,  1,   95,     750},
/*  4 */{   2000,    20,  4,  2,   95,    1000},
/*  5 */{   3000,    30,  5,  2,   90,    1250},
/*  6 */{   4000,    40,  6,  2,   90,    1500},
/*  7 */{   5000,    50,  6,  3,   85,    1750},
/*  8 */{   6500,    65,  8,  3,   85,    2000},
/*  9 */{   8000,    80,  9,  3,   80,    2250},
/* 10 */{  11000,   100, 10,  4,   80,    2500},
/* 11 */{  14000,   120, 11,  4,   75,    3000},
/* 12 */{  17000,   140, 12,  5,   75,    3500},
/* 13 */{  20000,   160, 13,  5,   70,    4000},
/* 14 */{  30000,   180, 14,  6,   70,    4500},
/* 15 */{  40000,   200, 15,  7,   65,    5000},
/* 16 */{  50000,   225, 16,  9,   65,    5500},
/* 17 */{  60000,   250, 17, 11,   60,    6000},
/* 18 */{  70000,   275, 18, 12,   55,    6500},
/* 19 */{  80000,   300, 19, 13,   55,    7000},
/* 20 */{  90000,   325, 20, 15,   50,    7500},
/* Lowbie Range Above Here */
/* 21 */{ 100000,   350, 21, 16,   45,    8000},
/* 22 */{ 110000,   375, 22, 17,   40,    8750},
/* 23 */{ 120000,   400, 23, 18,   35,    9500},
/* 24 */{ 130000,   425, 24, 19,   30,   10250},
/* 25 */{ 140000,   450, 25, 20,   25,   11000},
/* 26 */{ 150000,   475, 26, 20,   20,   11750},
/* 27 */{ 160000,   500, 27, 21,   15,   12500},
/* 28 */{ 170000,   525, 28, 21,   10,   13250},
/* 29 */{ 180000,   550, 29, 22,    5,   14000},
/* 30 */{ 190000,   575, 30, 22,    0,   15000},
/* 31 */{ 200000,   600, 31, 23,   -5,   16000},
/* 32 */{ 220000,   640, 32, 23,  -10,   17000},
/* 33 */{ 240000,   680, 33, 24,  -15,   18000},
/* 34 */{ 260000,   720, 34, 24,  -20,   19000},
/* 35 */{ 280000,   760, 35, 25,  -25,   20000},
/* 36 */{ 300000,   800, 36, 26,  -30,   21500},
/* 37 */{ 325000,   850, 37, 27,  -35,   23000},
/* 38 */{ 350000,   900, 38, 28,  -40,   24500},
/* 39 */{ 375000,   950, 39, 29,  -45,   26000},
/* Midbie Range Above Here */
/* 40 */{ 400000,  1000, 40, 20,  -50,   27500},
/* 41 */{ 425000,  1050, 41, 31,  -55,   30000},
/* 42 */{ 450000,  1100, 42, 32,  -60,   32500},
/* 43 */{ 475000,  1150, 43, 33,  -65,   35000},
/* 44 */{ 500000,  1200, 44, 34,  -70,   37500},
/* 45 */{ 550000,  1250, 45, 35,  -75,   40000},
/* 46 */{ 600000,  1300, 46, 36,  -80,   42500},
/* 47 */{ 650000,  1350, 47, 37,  -85,   45000},
/* 48 */{ 700000,  1400, 48, 38,  -90,   47500},
/* 49 */{ 750000,  1450, 49, 39,  -95,   50000},
/* 50 */{ 800000,  1500, 50, 40, -100,   55000},
/* 51 */{1000000,  1600, 51, 41, -110,   60000},
/* 52 */{1050000,  1700, 52, 42, -120,   65000},
/* 53 */{1100000,  1800, 53, 43, -130,   70000},
/* 54 */{1150000,  1900, 54, 44, -140,   75000},
/* 55 */{1200000,  2000, 55, 45, -150,   80000},
/* 56 */{1250000,  2100, 56, 46, -160,   85000},
/* 57 */{1300000,  2200, 57, 47, -170,   90000},
/* 58 */{1350000,  2300, 58, 48, -180,   95000},
/* 59 */{1400000,  2400, 59, 49, -190,  100000},
/* 60 */{1500000,  2500, 60, 50, -200,  105000},
/* 61 */{1600000,  2600, 61, 51, -210,  110000},
/* 62 */{1700000,  2700, 62, 52, -220,  115000},
/* 63 */{1800000,  2800, 63, 53, -230,  120000},
/* 64 */{1900000,  2900, 64, 54, -240,  125000},
/* 65 */{2000000,  3000, 65, 55, -250,  130000},
/* 66 */{2100000,  3200, 66, 56, -260,  135000},
/* 67 */{2200000,  3400, 67, 57, -270,  140000},
/* 68 */{2300000,  3600, 68, 58, -280,  145000},
/* 69 */{2400000,  3800, 69, 59, -290,  150000},
/* 70 */{2500000,  4000, 70, 60, -300,  155000},
/* 71 */{2600000,  4250, 71, 61, -320,  160000},
/* 72 */{2700000,  4500, 72, 62, -340,  165000},
/* 73 */{2800000,  4750, 73, 63, -360,  170000},
/* 74 */{2900000,  5000, 74, 64, -380,  175000},
/* High Level and 50+ Range Above Here */
/* ------ EQ Mob Range Starts Here ------    */
/* 75 */{5000000,  6000, 75, 65, -400,  400000},
/* 76 */{5100000,  6250, 76, 66, -420,  425000},
/* 77 */{5200000,  6500, 77, 67, -440,  450000},
/* 78 */{5300000,  6750, 78, 68, -460,  475000},
/* 79 */{5400000,  7000, 79, 69, -480,  500000},
/* 80 */{5500000,  7500, 80, 70, -500,  600000},
/* 81 */{5600000,  8000, 81, 71, -520,  700000},
/* 82 */{5700000,  8500, 82, 72, -540,  800000},
/* 83 */{5800000,  9000, 83, 73, -560,  900000},
/* 84 */{5900000,  9500, 84, 74, -580, 1000000},
/* 85 */{6000000, 10000, 85, 75, -600, 1250000},
/* 86 */{6100000, 10250, 86, 76, -620, 1500000},
/* 87 */{6200000, 10500, 87, 77, -640, 1750000},
/* 88 */{6300000, 11000, 88, 78, -660, 2000000},
/* 89 */{6400000, 11500, 89, 79, -680, 2250000},
/* 90 */{6500000, 12000, 90, 80, -700, 2500000},
/* 91 */{6600000, 12500, 91, 81, -720, 2750000},
/* 92 */{6700000, 13000, 92, 82, -740, 3000000},
/* 93 */{6800000, 13500, 93, 83, -760, 3250000},
/* 94 */{7000000, 14000, 94, 84, -780, 3500000},
/* 95 */{7250000, 15000, 95, 85, -800, 3750000},
/* 96 */{7500000, 16000, 96, 86, -820, 4000000},
/* 97 */{7750000, 17000, 97, 87, -840, 4250000},
/* 98 */{8000000, 18000, 98, 88, -860, 4500000},
/* 99 */{8250000, 19000, 99, 89, -880, 4750000},
/*100 */{8500000, 20000,100, 90, -900, 5000000},
/*101 */{8750000, 21000,101, 91, -910, 5250000},
/*102 */{9000000, 22000,102, 92, -920, 5500000},
/*103 */{9250000, 23000,103, 93, -930, 5750000},
/*104 */{9500000, 24000,104, 94, -940, 6000000},
/*105 */{9750000, 25000,105, 95, -950, 6250000},
/*106 */{10000000,26000,106, 96, -960, 6500000},
/*107 */{10250000,27000,107, 97, -970, 6750000},
/*108 */{10500000,28000,108, 98, -980, 7000000},
/*109 */{10750000,29000,109, 99, -990, 7250000},
/*110 */{11000000,30000,110,100,-1000, 7500000}
};
