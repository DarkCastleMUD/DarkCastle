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
/* $Id: const.cpp,v 1.96 2004/05/13 20:20:18 urizen Exp $ */
/* I KNOW THESE SHOULD BE SOMEWHERE ELSE -- Morc XXX */

extern "C"
{
#include <stdio.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <player.h> // *app_type
#include <character.h>
#include <spells.h>
#include <levels.h>
#include <mobile.h>
bestowable_god_commands_type bestowable_god_commands[] =
{
{ "goto",	COMMAND_GOTO },
{ "/",		COMMAND_IMP_CHAN },
{ "fakelog", 	COMMAND_FAKELOG },
{ "snoop",	COMMAND_SNOOP },
{ "log",	COMMAND_LOG },
{ "global",	COMMAND_GLOBAL },
{ "pview",	COMMAND_PROMPT_VIEW },
{ "restore",	COMMAND_RESTORE },
{ "purloin",	COMMAND_PURLOIN },
{ "arena",	COMMAND_ARENA },
{ "set",	COMMAND_SET },
{ "unban",	COMMAND_UNBAN },
{ "ban",	COMMAND_BAN },
{ "echo",	COMMAND_ECHO },
{ "send",	COMMAND_SEND },
{ "load",	COMMAND_LOAD },
{ "shutdown",   COMMAND_SHUTDOWN },
{ "mpedit",     COMMAND_MP_EDIT },
{ "range",      COMMAND_RANGE },
{ "mpstat",     COMMAND_MPSTAT },
{ "pshopedit",  COMMAND_PSHOPEDIT },
{ "sockets",    COMMAND_SOCKETS },
{ "sedit",      COMMAND_SEDIT },
{ "punish",     COMMAND_PUNISH },
{ "sqedit",     COMMAND_SQEDIT },
{ "\n",		-1 }
};

// every spell needs an entry in here
char *spell_wear_off_msg[] =
{
  "RESERVED DB.C",                                             /* 0 */
  "You feel less protected.",
  "!Teleport!",
  "You feel less righteous.",
  "You feel a cloak of blindness dissolve.",
  "!Burning Hands!",                                           /* 5 */
  "!Call Lightning",
  "You feel more self-confident.",
  "A warmth returns to your bones dispelling the deep chill.",
  "!Clone!",
  "!Color Spray!",                                             /* 10 */
  "!Control Weather!",
  "!Create Food!",
  "!Create Water!",
  "!Cure Blind!",
  "!Cure Critic!",                                             /* 15 */
  "!Cure Light!",
  "You feel better.",
  "You sense the red in your vision disappear.",
  "The detect invisible wears off.",
  "The detect magic wears off.",                               /* 20 */
  "The detect poison wears off.",
  "!Dispel Evil!",
  "!Earthquake!",
  "!Enchant Weapon!",
  "!Energy Drain!",                                            /* 25 */
  "!Fireball!",
  "!Harm!",
  "!Heal",
  "You feel yourself exposed.",
  "!Lightning Bolt!",                                          /* 30 */
  "!Locate object!",
  "!Magic Missile!",
  "You feel less sick.",
  "You feel your moral vulnerability.",
  "!Remove Curse!",                                            /* 35 */
  "The white aura around your body fades.",
  "!Shocking Grasp!",
  "You feel less tired.",
  "You feel weaker.",
  "!Summon!",                                                  /* 40 */
  "!Ventriloquate!",
  "!Word of Recall!",
  "!Remove Poison!",
  "You feel less aware of your surroundings.",
  "!Summon Familiar!",                                         /* 45 */
  "!Lighted Path!",
  "The green in your skin fades.",
  "!Sun ray!",
  "Your body's healing process slows down to normal.",
  "The shield of $B$2acid$R around you dissapates into the air.", /* 50 */
  "The gills fade from your neck.",
  "!Globe Of Darkness!",
  "!Identify!",
  "!Animate Dead!",
  "You are not as fearful now.",                               /* 55 */
  "You slowly float to the ground.",
  "Your light slowly fizzes into nothing.",
  "You can no longer see into a person's aura.",
  "!Dispel Magic!",
  "!Conjure Elemental!",                                       /* 60 */
  "!Cure Serious!",
  "!Cause Light!",
  "!Cause Critical!",
  "!Cause Serious!",
  "!Flamestrike!",                                             /* 65 */
  "Your skin does not feel as hard as it used to.",
  "Your force shield shimmers then fades away.",
  "You feel stronger.",
  "!Mass Invisibility!",  /* Uses the invisibility message */
  "!Acid Blast!",                                              /* 70 */
  "!Gate!",
  "You can no longer see in the dark.",
  "You feel better now.",
  "You don't feel as fast anymore.",
  "!Dispel Good!", /* 75 */
  "!Hellstream!",
  "!Power Heal!",
  "!Full Heal!",
  "!Firestorm!",
  "!Power Harm!",             /* 80  */
  "You can no longer see the good in a person.",
  "!Vampiric Touch!",
  "!Life Leech!",
  "You can move again.",
  "!Remove Paralysis!", /* 85 */
  "The flames surrounding you fade away.",
  "!Meteor Swarm!",
  "!Wizard's Eye!",
  "You don't seem to be able to see everything as clearly anymore.",
  "!Mana!", /* 90 */
  "!Solar Gate!",
  "!Heroes Feast!",
  "!Heal Spray!",
  "!Group Sanc!",
  "!Group Recall!",
  "!Group Fly!",
  "!Enchant Armor!",
  "The red in your skin fades.",
  "The blue in your skin fades.", /* 99 */
  "!Bee Sting!",
  "!Bee Swarm!",
  "!Creeping Death!",
  "Your skin returns to its normal consistency.",
  "!Herb Lore!",
  "!Call Follower!",
  "!Entangle!",
  "Your vision is not so acute anymore.",
  "You no longer feel like a giant pussy.",
  "The forest kicks you out!",
  "!Companion!",	/* 110 */
  "!Drown!",
  "!Howl!",
  "!Souldrain!",
  "!Sparks!",     // 114
  "Your $2c$7a$0$Bmo$Ru$2fl$7a$0$Bg$R$2u$7e$R has worn off.",
  "!FarSight!",
  "!FreeFloat!",
  "!Insomnia!",
  "!ShadowSlip!",
  "The yellow in your skin fades.",  // 120
  "!StaunchBlood!",
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
  "The lightning around you fades away leaving only static cling.",
  "!Blue bird!",
  "With a rush of strength, the $6debility$R fades from your body.",
  "Your rapid decay ends and your health returns to normal.",
  "The shadow in your aura fades away into the ethereal.",
  "Your serene aura of holiness fades.",
  "!DismissFamiliar!",
  "!DismissCorpse!",
  "Your blessed halo fades.",
  "You don't feel quite so angry anymore.",
  "The foul mantle surrounding you dissipates.",
  "Your Oaken Fortitude fades.",
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
    "CON_GET_ACCOUNT",                  
    "CON_CONFIRM_NEW_ACCOUNT",
    "CON_ACCOUNT_GET_EMAIL_ADDRESS",
    "CON_ACCOUNT_CONFIRM_NEW_PASSWORD",
    "CON_ACCOUNT_GET_FIRST_NAME",
    "CON_ACCOUNT_GET_LAST_NAME",      
    "CON_ACCOUNT_GET_ADDR1",       
    "CON_ACCOUNT_GET_ADDR2",           
    "CON_ACCOUNT_GET_ADDR3",   
    "CON_ACCOUNT_GET_CITYSTATEZIP",
    "CON_ACCOUNT_GET_COUNTRY",    
    "CON_ACCOUNT_GET_PHONE",
    "CON_ACCOUNT_GET_SECRET_QUESTION",
    "CON_ACCOUNT_GET_SECRET_ANSWER",
    "CON_ACCOUNT_GET_OLD_PASSWORD",
    "CON_ACCOUNT_GET_NEW_PASSWORD",
    "CON_ACCOUNT_MENU",
    "CON_ACCOUNT_LOGIN_CHAR",
    "\n"
};

int movement_loss[]=
{
    1,  /* Inside     */
    2,  /* City       */
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
    "whisky",
    "lemonade",
    "firebreather",
    "local speciality",
    "slime mold juice",
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
    "whisky",
    "lemonade",
    "firebreather",
    "local",
    "juice",
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
    { 0,1,10 },  /* Water    */
    { 3,2,5 },   /* beer     */
    { 5,2,5 },   /* wine     */
    { 2,2,5 },   /* ale      */
    { 1,2,5 },   /* ale      */
    { 6,1,4 },   /* Whiskey  */
    { 0,1,8 },   /* lemonade */
    { 10,0,0 },  /* firebr   */
    { 3,3,3 },   /* local    */
    { 0,4,-8 },  /* juice    */
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
1,          L(1 K),     L(3 K),     L(9 K),     L(15 K),   // level 5
L(30 K),    L(50 K),    L(100 K),   L(200 K),   L(300 K),  // level 10
L(400 K),   L(550 K),   L(850 K),   L(1.2 M),   L(1.4 M),  // level 15
L(1.8 M),   L(2.3 M),   L(3 M),     L(3.8 M),   L(4.8 M),  // level 20
L(7 M),     L(8.8 M),   L(9.7 M),   L(11 M),    L(12 M),   // level 25
L(13.5 M),  L(14.5 M),  L(16.5 M),  L(18 M),    L(21.5 M), // level 30
L(22 M),    L(24 M),    L(26 M),    L(28 M),    L(31 M),   // level 35
L(34 M),    L(37 M),    L(40 M),    L(45 M),    L(50 M),   // level 40
L(60 M),    L(70 M),    L(80 M),    L(90 M),    L(100 M),  // level 45
L(120 M),   L(140 M),   L(160 M),   L(180 M),   L(200 M),  // level 50
L(400 M),   L(2 M),     L(3 M),     L(4 M),     L(5 M),    // level 55
L(6 M),     L(7 M),     L(8 M),     L(9 M),     L(10 M),   // level 60
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
    "WAISTE",
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
    ""
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
    "MAGIC",
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
    "UNUSED",
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
    "unused1",
    "no_mob",
    "indoors",
    "lawful",
    "neutral",
    "chaotic",
    "no_magic",
    "tunnel",
    "private",
    "safe",
    "no_summon",
    "unused2",
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
char *affected_bits2[] =
{
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
    "\n"
};

char *affected_bits[] =
{
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
    "FAMILIAR",
    "PROTECT-GOOD",
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
//    sh_int skillnum;          // ID # of skill
//    sh_int levelavailable;    // what level class can get it
//    byte   maximum;           // maximum value PC can train it to (1-99)
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
{    "double",          SKILL_SECOND_ATTACK,      7,    90,     {DEX,INT} },
{    "disarm",          SKILL_DISARM,             10,   70,     {DEX,WIS} },
{    "headbutt",        SKILL_SHOCK,              12,   55,     {CON,WIS} },
{    "shield block",    SKILL_SHIELDBLOCK,        15,   85,     {STR,DEX} },
{    "retreat",         SKILL_RETREAT,            17,   98,     {WIS,INT} },
{    "frenzy",          SKILL_FRENZY,             18,   80,     {CON,INT} },
{    "parry",           SKILL_PARRY,              20,   90,     {DEX,WIS} },
{    "blindfighting",   SKILL_BLINDFIGHTING,      21,   80,     {INT,DEX} },
{    "triple",          SKILL_THIRD_ATTACK,       23,   90,     {DEX,INT} },
{    "hitall",          SKILL_HITALL,             25,   80,     {STR,CON} },
{    "dual wield",      SKILL_DUAL_WIELD,         28,   90,     {DEX,CON} },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS,   30,   90,     {STR,DEX} },
{    "crushing",        SKILL_CRUSHING_WEAPONS,   30,   90,     {STR,DEX} },
{    "piercing",        SKILL_PIERCEING_WEAPONS,  30,   90,     {DEX,STR} },
{    "slashing",        SKILL_SLASHING_WEAPONS,   30,   90,     {DEX,STR} },
{    "whipping",        SKILL_WHIPPING_WEAPONS,   30,   90,     {DEX,STR} },
{    "tactics",         SKILL_TACTICS,            31,   98,     {INT,WIS} },
{    "archery",         SKILL_ARCHERY,            32,   55,     {DEX,WIS} },
{    "two handers",     SKILL_TWO_HANDED_WEAPONS, 34,   85,     {STR,CON} },
{    "stun",            SKILL_STUN,               35,   75,     {DEX,STR} },
{    "guard",           SKILL_GUARD,              37,   98,     {STR,WIS} },
{    "deathstroke",     SKILL_DEATHSTROKE,        39,   98,     {STR,INT} },
{    "riposte",         SKILL_RIPOSTE,            40,   98,     {INT,DEX} },
{    "dodge",           SKILL_DODGE,              42,   55,     {DEX,INT} },
{    "skewer",          SKILL_SKEWER,             45,   98,     {STR,CON} },
{    "blade shield",    SKILL_BLADESHIELD,        47,   98,     {CON,DEX} },
//{  "combat mastery",  SKILL_COMBAT_MASTERY      50,   98,     {0,0} },
{    "\n",              0,                        1,    0,      {0,0} }
};

// thief 3022 guildmaster - done and checked, Apoc
struct class_skill_defines t_skills[] = { // thief skills

//   Ability Name       Ability File          Level   Max     Requisites
//   ------------       ------------          -----   ---     ----------
{    "sneak",           SKILL_SNEAK,            1,      90,   {DEX,INT} },  
{    "backstab",        SKILL_BACKSTAB,         2,      90,   {STR,DEX} },  
{    "shield block",    SKILL_SHIELDBLOCK,      4,      55,   {STR,DEX} },  
{    "pick",            SKILL_PICK_LOCK,        6,      98,   {WIS,DEX} },  
{    "hide",            SKILL_HIDE,             7,      90,   {INT,WIS} },  
{    "dual wield",      SKILL_DUAL_WIELD,       10,     90,   {DEX,CON} },  
{    "palm",            SKILL_PALM,             12,     98,   {DEX,INT} },  
{    "slip",            SKILL_SLIP,             13,     98,   {DEX,INT} },  
{    "dodge",           SKILL_DODGE,            15,     90,   {DEX,INT} },
{    "trip",            SKILL_TRIP,             17,     85,   {DEX,STR} },
{    "pocket",          SKILL_POCKET,           20,     98,   {INT,DEX} },
{    "stalk",           SKILL_STALK,            22,     98,   {CON,DEX} },
{    "steal",           SKILL_STEAL,            25,     98,   {DEX,WIS} },
{    "blindfighting",   SKILL_BLINDFIGHTING,    28,     80,   {INT,DEX} },
{    "piercing",        SKILL_PIERCEING_WEAPONS,30,     90,   {DEX,STR} },
{    "slashing",        SKILL_SLASHING_WEAPONS, 30,     55,   {DEX,STR} },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS, 30,     70,   {STR,DEX} },
{    "crushing",        SKILL_CRUSHING_WEAPONS, 30,     55,   {STR,DEX} },  
{    "deceit",          SKILL_DECEIT,           31,     98,   {WIS,INT} },  
{    "circle",          SKILL_CIRCLE,           35,     98,   {STR,DEX} },  
{    "disarm",          SKILL_DISARM,           38,     85,   {DEX,WIS} },  
{    "dualbackstab",    SKILL_DUAL_BACKSTAB,    40,     98,   {DEX,INT} },  
{    "eyegouge",        SKILL_EYEGOUGE,         42,     98,   {STR,CON} },  
{    "vitalstrike",     SKILL_VITAL_STRIKE,     45,     98,   {CON,DEX} },  
//{  "diguise",         SKILL_DISGUISE          50,     98,   {0,0} },
{    "\n",              0,                      1,      0,    {0,0} }
};

// anti-paladin 10005 guildmaster - done and checked, Apoc
struct class_skill_defines a_skills[] = { // anti-paladin skills

//   Ability Name            Ability File           Level     Max     Requisites
//   ------------            ------------           -----     ---     ----------
{    "harmtouch",            SKILL_HARM_TOUCH,        1,      98,     {STR,CON} },
{    "kick",                 SKILL_KICK,              2,      70,     {DEX,STR} },
{    "sneak",                SKILL_SNEAK,             3,      85,     {DEX,INT} },
{    "shield block",         SKILL_SHIELDBLOCK,       5,      70,     {STR,DEX} },
{    "infravision",          SPELL_INFRAVISION,       7,      90,     {INT,DEX} },
{    "detect good",          SPELL_DETECT_GOOD,       8,      98,     {WIS,INT} },
{    "dual wield",           SKILL_DUAL_WIELD,        10,     85,     {DEX,CON} },
{    "shocking grasp",       SPELL_SHOCKING_GRASP,    11,     98,     {INT,DEX} },
{    "detect invisibility",  SPELL_DETECT_INVISIBLE,  12,     85,     {INT,DEX} },
{    "invisibility",         SPELL_INVISIBLE,         13,     90,     {INT,DEX} },
{    "backstab",             SKILL_BACKSTAB,          15,     90,     {STR,DEX} },
{    "hide",                 SKILL_HIDE,              17,     85,     {INT,WIS} },
{    "trip",                 SKILL_TRIP,              19,     85,     {DEX,STR} },
{    "chill touch",          SPELL_CHILL_TOUCH,       20,     85,     {CON,WIS} },
{    "double",               SKILL_SECOND_ATTACK,     22,     85,     {DEX,INT} },
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
{    "curse",                SPELL_CURSE,             41,     70,     {WIS,CON} },
{    "firestorm",            SPELL_FIRESTORM,         42,     85,     {INT,STR} },
{    "stone skin",           SPELL_STONE_SKIN,        44,     85,     {STR,CON} },
{    "protection from good", SPELL_PROTECT_FROM_GOOD, 45,     90,     {WIS,DEX} },
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
{    "double",               SKILL_SECOND_ATTACK,      5,      85,     {DEX,INT} },
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
{    "parry",                SKILL_PARRY,              23,     80,     {DEX,WIS} },
{    "bash",                 SKILL_BASH,               25,     85,     {STR,CON} },
{    "sense life",           SPELL_SENSE_LIFE,         26,     85,     {CON,INT} },
{    "strength",             SPELL_STRENGTH,           28,     70,     {STR,CON} },
{    "earthquake",           SPELL_EARTHQUAKE,         29,     70,     {DEX,WIS} },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,   30,     85,     {STR,DEX} },
{    "slashing",             SKILL_SLASHING_WEAPONS,   30,     85,     {DEX,STR} },
{    "crushing",             SKILL_CRUSHING_WEAPONS,   30,     85,     {STR,DEX} },
{    "blessed halo",         SPELL_BLESSED_HALO,       31,     98,     {WIS,INT} },
{    "triple",               SKILL_THIRD_ATTACK,       33,     80,     {DEX,INT} },
{    "two handers",          SKILL_TWO_HANDED_WEAPONS, 35,     85,     {STR,CON} },
{    "heal",                 SPELL_HEAL,               37,     85,     {WIS,INT} },
{    "harm",                 SPELL_HARM,               38,     85,     {WIS,CON} },
{    "sanctuary",            SPELL_SANCTUARY,          40,     90,     {WIS,INT} },
{    "armor",                SPELL_ARMOR,              41,     80,     {STR,INT} },
{    "dispel evil",          SPELL_DISPEL_EVIL,        42,     90,     {WIS,STR} },
{    "protection from evil", SPELL_PROTECT_FROM_EVIL,  45,     90,     {WIS,DEX} },
{    "power harm",           SPELL_POWER_HARM,         48,     90,     {WIS,CON} },
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
{    "double",          SKILL_SECOND_ATTACK,      8,    85,  {DEX,INT} },
{    "shield block",    SKILL_SHIELDBLOCK,        10,   70,  {STR,DEX} },
{    "blood fury",      SKILL_BLOOD_FURY,         12,   98,  {CON,WIS} },
{    "crazedassault",   SKILL_CRAZED_ASSAULT,     15,   98,  {WIS,STR} },
{    "frenzy",          SKILL_FRENZY,             18,   90,  {CON,INT} },
{    "rage",            SKILL_RAGE,               20,   98,  {CON,STR} },
{    "triple",          SKILL_THIRD_ATTACK,       25,   85,  {DEX,INT} },
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
//{  "magic resistance" SKILL_MAGIC_RESISTANCE    47,   98,  {INT,CON} },
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
{    "shield block",    SKILL_SHIELDBLOCK,      10,     80,     {STR,DEX} },
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
{    "stun",            SKILL_STUN,             33,     90,     {DEX,STR} },
{    "storm",           KI_STORM+KI_OFFSET,     35,     98,     {CON,WIS} },
{    "blindfighting",   SKILL_BLINDFIGHTING,    38,     85,     {INT,DEX} },
{    "quiver",          SKILL_QUIVERING_PALM,   40,     98,     {STR,INT} },
{    "blast",           KI_BLAST+KI_OFFSET,     45,     98,     {WIS,STR} },
{    "disrupt",         KI_DISRUPT+KI_OFFSET,   47,     98,     {INT,DEX} },
//   "quest skill",     MONKQUESTSKILL          50,     98,     {0,0} },
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
{    "shield block",    SKILL_SHIELDBLOCK,       10,     80,     {STR,DEX} },
{    "tame",            SKILL_TAME,              11,     98,     {WIS,INT} },
{    "double",          SKILL_SECOND_ATTACK,     12,     85,     {DEX,INT} },
{    "feline agility",  SPELL_FELINE_AGILITY,    14,     98,     {DEX,INT} },
{    "bee swarm",       SPELL_BEE_SWARM,         15,     98,     {CON,WIS} },
{    "forage",          SKILL_FORAGE,            16,     90,     {INT,CON} },
{    "entangle",        SPELL_ENTANGLE,          18,     90,     {WIS,DEX} },
{    "archery",         SKILL_ARCHERY,           20,     90,     {DEX,WIS} },
{    "blindfighting",   SKILL_BLINDFIGHTING,     21,     90,     {INT,DEX} },
{    "dodge",           SKILL_DODGE,             22,     80,     {DEX,INT} },
{    "herb lore",       SPELL_HERB_LORE,         23,     90,     {INT,WIS} },
{    "poison",          SPELL_POISON,            25,     85,     {CON,WIS} },
//   "wind arrows"      SKILL_WIND_ARROW         26,     98,     {INT,DEX} },
{    "track",           SKILL_TRACK,             28,     98,     {WIS,INT} },
{    "barkskin",        SPELL_BARKSKIN,          29,     90,     {CON,DEX} },
{    "piercing",        SKILL_PIERCEING_WEAPONS, 30,     85,     {DEX,STR} },
{    "slashing",        SKILL_SLASHING_WEAPONS,  30,     85,     {DEX,STR} },
{    "whipping",        SKILL_WHIPPING_WEAPONS,  30,     85,     {DEX,STR} },
//   "frost arrows"     SKILL_FROST_ARROW        31,     98,     {CON,DEX} },
{    "rescue",          SKILL_RESCUE,            32,     80,     {WIS,INT} },
{    "ambush",          SKILL_AMBUSH,            35,     98,     {INT,DEX} },
//   "fire arrows"      SKILL_FIRE_ARROW         36,     98,     {WIS,DEX} },
{    "call follower",   SPELL_CALL_FOLLOWER,     38,     98,     {CON,STR} },
{    "stun",            SKILL_STUN,              40,     70,     {DEX,STR} },
//   "stone arrows"     SKILL_STONE_ARROW        41,     98,     {STR,DEX} },
{    "disarm",          SKILL_DISARM,            42,     70,     {DEX,WIS} },
{    "forest meld",     SPELL_FOREST_MELD,       45,     90,     {WIS,DEX} },
{    "camouflage",      SPELL_CAMOUFLAGE,        46,     90,     {INT,DEX} },
{    "creeping death",  SPELL_CREEPING_DEATH,    48,     98,     {WIS,STR} },
//{  "frostshield",     SPELL_FROSTSHIELD,       50,     98,     {0,0} },
{      "\n",            0,                       1,      0,      {0,0} }
};

// bard 3204 guildmaster - done and checked, Apoc
struct class_skill_defines d_skills[] = { // bard skills

// Ability Name            Ability File                  Level    Max      Requisites
// ------------            ------------                  -----    ---      ----------
{ "listsongs",             SKILL_SONG_LIST_SONGS,          1,      98,     {INT,WIS} },
{ "whistle sharp",         SKILL_SONG_WHISTLE_SHARP,       1,      98,     {STR,INT} },
{ "stop",                  SKILL_SONG_STOP,                2,      98,     {INT,WIS} },
{ "irresistable ditty",    SKILL_SONG_UNRESIST_DITTY,      3,      98,     {DEX,WIS} },
{ "dodge",                 SKILL_DODGE,                    5,      60,     {DEX,INT} },
{ "hide",                  SKILL_HIDE,                     7,      70,     {INT,WIS} },
{ "traveling march",       SKILL_SONG_TRAVELING_MARCH,     9,      98,     {DEX,CON} },
{ "dual wield",            SKILL_DUAL_WIELD,               10,     60,     {DEX,CON} },
{ "bountiful sonnet",      SKILL_SONG_BOUNT_SONNET,        12,     98,     {CON,WIS} },
{ "healing melody",        SKILL_SONG_HEALING_MELODY,      13,     98,     {WIS,CON} },
{ "glitter dust",          SKILL_SONG_GLITTER_DUST,        15,     98,     {INT,DEX} },
{ "synchronous chord",     SKILL_SONG_SYNC_CHORD,          17,     98,     {INT,STR} },
{ "sticky lullaby",        SKILL_SONG_STICKY_LULL,         18,     98,     {STR,WIS} },
{ "flight of the bumblebee", SKILL_SONG_FLIGHT_OF_BEE,     20,     98,     {DEX,INT} },
{ "note of knowledge",     SKILL_SONG_NOTE_OF_KNOWLEDGE,   21,     98,     {INT,WIS} },
//{ "fanatical fanfare",   SKILL_FANATICAL_FANFARE         23,     98,     {CON,INT} },
{ "revealing stacato",     SKILL_SONG_REVEAL_STACATO,      25,     98,     {INT,DEX} },
{ "double",                SKILL_SECOND_ATTACK,            26,     80,     {DEX,INT} },
{ "terrible clef",         SKILL_SONG_TERRIBLE_CLEF,       28,     98,     {STR,INT} },
{ "piercing",              SKILL_PIERCEING_WEAPONS,        30,     70,     {DEX,STR} },
{ "slashing",              SKILL_SLASHING_WEAPONS,         30,     70,     {DEX,STR} },
{ "bludgeoning",           SKILL_BLUDGEON_WEAPONS,         30,     70,     {STR,DEX} },
{ "crushing",              SKILL_CRUSHING_WEAPONS,         30,     70,     {STR,DEX} },
{ "soothing rememberance", SKILL_SONG_SOOTHING_REMEM,      31,     98,     {INT,WIS} },
{ "searching song",        SKILL_SONG_SEARCHING_SONG,      32,     98,     {INT,DEX} },
//{ "dischordant dirge",   SKILL_DISCHORDANT_DIRGE,        34,     98,     {WIS,CON} },
{ "insane chant",          SKILL_SONG_INSANE_CHANT,        35,     98,     {WIS,INT} },
{ "jig of alacrity",       SKILL_SONG_JIG_OF_ALACRITY,     38,     98,     {DEX,CON} },
{ "vigilant siren",        SKILL_SONG_VIGILANT_SIREN,      40,     98,     {INT,CON} },
{ "forgetful rhythm",      SKILL_SONG_FORGETFUL_RHYTHM,    42,     98,     {INT,STR} },
{ "disarming limerick",    SKILL_SONG_DISARMING_LIMERICK,  43,     98,     {CON,INT} },
{ "astral chanty",         SKILL_SONG_ASTRAL_CHANTY,       45,     98,     {STR,DEX} },
//{ "lyrical enchantment", SKILL_LYRICAL_ENCHANTMENT,      46,     98,     {INT,DEX} },
{ "shattering resonance",  SKILL_SONG_SHATTERING_RESO,     48,     98,     {STR,CON} },
//{ "hypnotic harmony",    SKILL_HYPNOTIC_HARMONY,         50,     98,     {WIS,INT} },
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
{    "create food",          SPELL_CREATE_FOOD,          8,      90,     {INT,CON} },  
{    "sense life",           SPELL_SENSE_LIFE,           10,     85,     {CON,INT} },  
{    "weaken",               SPELL_WEAKEN,               11,     98,     {STR,CON} },  
{    "cure serious",         SPELL_CURE_SERIOUS,         12,     85,     {WIS,INT} },
{    "camouflage",           SPELL_CAMOUFLAGE,           14,     85,     {INT,DEX} },
{    "oaken fortitude",      SPELL_OAKEN_FORTITUDE,      15,     98,     {CON,STR} },
{    "water breathing",      SPELL_WATER_BREATHING,      17,     98,     {DEX,INT} },
{    "resist acid",          SPELL_RESIST_ACID,          18,     98,     {CON,WIS} },
{    "stone shield",         SPELL_STONE_SHIELD,         20,     98,     {STR,WIS} },
{    "poison",               SPELL_POISON,               21,     90,     {CON,WIS} },
{    "cure critic",          SPELL_CURE_CRITIC,          23,     85,     {WIS,INT} },
{    "summon familiar",      SPELL_SUMMON_FAMILIAR,      25,     90,     {INT,STR} },
{    "dismiss familiar",     SPELL_DISMISS_FAMILIAR,     26,     90,     {WIS,CON} },  
{    "debility",             SPELL_DEBILITY,             27,     98,     {DEX,CON} },  
{    "drown",                SPELL_DROWN,                28,     98,     {INT,CON} },  
{    "entangle",             SPELL_ENTANGLE,             29,     90,     {WIS,DEX} },  
{    "whipping",             SKILL_WHIPPING_WEAPONS,     30,     70,     {DEX,STR} },  
{    "crushing",             SKILL_CRUSHING_WEAPONS,     30,     70,     {STR,DEX} },  
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,     30,     70,     {STR,DEX} },  
{    "rapid mend",           SPELL_RAPID_MEND,           31,     98,     {WIS,INT} },  
{    "herb lore",            SPELL_HERB_LORE,            32,     90,     {INT,WIS} },
{    "lighted path",         SPELL_LIGHTED_PATH,         33,     98,     {WIS,DEX} },
{    "curse",                SPELL_CURSE,                34,     90,     {WIS,CON} },
{    "sun ray",              SPELL_SUN_RAY,              35,     98,     {INT,WIS} },
{    "control weather",      SPELL_CONTROL_WEATHER,      36,     90,     {CON,WIS} },
{    "barkskin",             SPELL_BARKSKIN,             37,     85,     {CON,DEX} },
{    "iron roots",           SPELL_IRON_ROOTS,           38,     98,     {STR,DEX} },
{    "earthquake",           SPELL_EARTHQUAKE,           40,     90,     {DEX,WIS} },
{    "lightning shield",     SPELL_LIGHTNING_SHIELD,     41,     98,     {WIS,INT} },
{    "blindness",            SPELL_BLINDNESS,            42,     98,     {CON,WIS} },  
{    "forage",               SKILL_FORAGE,               43,     90,     {INT,CON} },  
{    "stone skin",           SPELL_STONE_SKIN,           44,     70,     {STR,CON} },  
{    "power heal",           SPELL_POWER_HEAL,           45,     98,     {WIS,STR} },  
{    "forest meld",          SPELL_FOREST_MELD,          45,     90,     {WIS,DEX} },  
{    "greater stone shield", SPELL_GREATER_STONE_SHIELD, 47,     98,     {STR,WIS} },  
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
{    "cure blind",           SPELL_CURE_BLIND,        16,     98,     {INT,WIS} },
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
{    "dispel evil",          SPELL_DISPEL_EVIL,       32,     90,     {WIS,STR} },
{    "dispel good",          SPELL_DISPEL_GOOD,       33,     90,     {WIS,STR} },     
{    "call lightning",       SPELL_CALL_LIGHTNING,    34,     98,     {INT,CON} },     
{    "control weather",      SPELL_CONTROL_WEATHER,   35,     70,     {CON,WIS} },     
{    "protection from evil", SPELL_PROTECT_FROM_EVIL, 36,     90,     {WIS,DEX} },     
{    "protection from good", SPELL_PROTECT_FROM_GOOD, 37,     90,     {WIS,DEX} },     
{    "portal",               SPELL_PORTAL,            38,     85,     {STR,INT} },     
{    "true sight",           SPELL_TRUE_SIGHT,        39,     90,     {WIS,INT} },     
{    "full heal",            SPELL_FULL_HEAL,         40,     98,     {WIS,INT} },
{    "power harm",           SPELL_POWER_HARM,        41,     85,     {WIS,CON} },
{    "resist cold",          SPELL_RESIST_COLD,       42,     90,     {CON,STR} },
{    "resist fire",          SPELL_RESIST_FIRE,       43,     90,     {INT,CON} },
{    "resist energy",        SPELL_RESIST_ENERGY,     44,     98,     {CON,DEX} },
{    "dispel magic",         SPELL_DISPEL_MAGIC,      45,     85,     {INT,CON} },
{    "flamestrike",          SPELL_FLAMESTRIKE,       46,     98,     {INT,WIS} },
{    "group recall",         SPELL_GROUP_RECALL,      47,     98,     {DEX,STR} },
{    "heal spray",           SPELL_HEAL_SPRAY,        48,     98,     {WIS,CON} },
{    "group sanctuary",      SPELL_GROUP_SANC,        49,     98,     {STR,WIS} },
//{    "turn undead",        SPELL_TURN_UNDEAD,       50,     98,     {0,0} },
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
{    "know alignment",      SPELL_KNOW_ALIGNMENT,    14,     98,     {INT,WIS} },
{    "fear",                SPELL_FEAR,              15,     90,     {WIS,INT} },
{    "identify",            SPELL_IDENTIFY,          16,     98,     {INT,WIS} }, 
{    "locate object",       SPELL_LOCATE_OBJECT,     17,     90,     {INT,WIS} }, 
{    "summon familiar",     SPELL_SUMMON_FAMILIAR,   18,     90,     {INT,STR} }, 
{    "dismiss familiar",    SPELL_DISMISS_FAMILIAR,  18,     90,     {WIS,CON} }, 
{    "chill touch",         SPELL_CHILL_TOUCH,       20,     90,     {CON,WIS} }, 
{    "shield",              SPELL_SHIELD,            21,     98,     {WIS,STR} }, 
{    "souldrain",           SPELL_SOULDRAIN,         22,     98,     {WIS,CON} }, 
{    "enchant weapon",      SPELL_ENCHANT_WEAPON,    24,     98,     {WIS,DEX} }, 
{    "dispel minor",        SPELL_DISPEL_MINOR,      25,     90,     {WIS,CON} }, 
{    "mass invisibility",   SPELL_MASS_INVISIBILITY, 26,     98,     {DEX,INT} }, 
{    "life leech",          SPELL_LIFE_LEECH,        27,     98,     {CON,STR} },
{    "portal",              SPELL_PORTAL,            28,     90,     {STR,INT} },
{    "fireball",            SPELL_FIREBALL,          29,     98,     {INT,CON} },
{    "focused repelance",   SKILL_FOCUSED_REPELANCE, 30,     98,     {DEX,INT} },
{    "piercing",            SKILL_PIERCEING_WEAPONS, 30,     55,     {DEX,STR} },
{    "bludgeoning",         SKILL_BLUDGEON_WEAPONS,  30,     55,     {STR,DEX} },
{    "sleep",               SPELL_SLEEP,             31,     98,     {INT,WIS} },
{    "haste",               SPELL_HASTE,             33,     98,     {DEX,INT} }, 
{    "true sight",          SPELL_TRUE_SIGHT,        34,     85,     {WIS,INT} }, 
{    "dispel magic",        SPELL_DISPEL_MAGIC,      35,     90,     {INT,CON} }, 
{    "resist fire",         SPELL_RESIST_FIRE,       36,     70,     {INT,CON} }, 
{    "wizard eye",          SPELL_WIZARD_EYE,        37,     98,     {INT,WIS} }, 
{    "teleport",            SPELL_TELEPORT,          38,     98,     {CON,INT} }, 
{    "stone skin",          SPELL_STONE_SKIN,        39,     70,     {STR,CON} }, 
{    "meteor swarm",        SPELL_METEOR_SWARM,      40,     98,     {STR,INT} }, 
{    "word of recall",      SPELL_WORD_OF_RECALL,    42,     85,     {STR,WIS} }, 
{    "firestorm",           SPELL_FIRESTORM,         43,     90,     {INT,STR} }, 
{    "paralyze",            SPELL_PARALYZE,          44,     98,     {INT,DEX} },
{    "hellstream",          SPELL_HELLSTREAM,        45,     98,     {INT,STR} },
{    "resist cold",         SPELL_RESIST_COLD,       46,     90,     {CON,STR} },
{    "fireshield",          SPELL_FIRESHIELD,        47,     98,     {CON,INT} },
//{    "create golem",        SPELL_CREATE_GOLEM,      48,     90,     
//{WIS,STR} },
//{    "release golem",       SPELL_RELEASE_GOLEM,     48,     90,     
//{WIS,INT} },
{    "solar gate",          SPELL_SOLAR_GATE,        49,     98,     {WIS,INT} },
//{    "mana shield",       SPELL_MANA_SHIELD,       50,     98,     {0,0} },
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
  {"NPC",       "NPC",
   63,0,0,0,0,0,0,0},
  {"Human",     "Humans",
   63,0,0,0,0,3,150,200},
  {"Elf",       "Elves",
   63,0,0,0,1<<7 + 1<<2,18,0,0},
  {"Dwarf",     "Dwarves",
   63,0,0,0,1<<1+1<<5,12,0,0},
  {"Hobbit",    "Hobbits",
   63,0,0,0,1<<8,12,0,0},
  {"Pixie",     "Pixies",
   63,0,0,0,1<<5 + 1<<8,18,0,0},
  {"Giant",     "Giants",
   63,0,0,0,1<<2 + 1<<4,1,0,0},
  {"Gnome",     "Gnomes",
   63,0,0,0,1<<9,70,0,0},
  {"Orc",       "Orcs",
   63,0,0,0,1<<1 + 1<<3,128,0,0},
  {"Troll",     "Trolls",
   63,0,0,16,1<<3 + 1<<4,0,0,0},
  {"Goblin",    "Goblins",
   63,0,0,0,1<<6,0,0,0},
  {"Reptile",   "Reptiles",
   59,0,0,0,0,0,0,0},
  {"Dragon",    "Dragons",
   91,16,0,0,1<<6 + 1<<9,0,0,0},
  {"Snake",     "Snakes",
   3,0,0,0,24,0,0,0},
  {"Horse",     "Horses",
   19,0,0,0,0,0,0,0},
  {"Bird",      "Birds",
   63,0,0,0,98304,16384,0,0},    /* 15 */
  {"Rodent",    "Rodents",
   59,0,0,0,262145,0,0,0},
  {"Fish",      "Fishes",
   3,256,0,0,262144,0,0,0},
  {"Arachnid",  "Arachnids",
   19,0,0,0,262168,0,0,0},
  {"Insect",    "Insects",
   19,0,0,0,3,0,0,0},
  {"Slime",     "Slimes",
   2,3,0,0,0,0,0,0},
  {"Animal",    "Animals",
   27,0,0,0,0,0,0,0},
  {"Tree",      "Trees",
   2, 1,0,0,0,0,0,0},
  {"Enfan",     "Enfans",
   63,0,0,0,1,4194304,0,0},
  {"Undead",    "Undead",
   63,129,0,0,0,0,0,0},
  {"Ghost",     "Spirits",
   63,723,0,0,1,0,0,0},
  {"Golem",     "Golems",
   63,386,0,0,2,0,0,0},
  {"Elemental", "Elementals",
   63, 384,  0,  0,   0,67108864,0,0},
  {"Planar",    "Planar",
   63,   0,  0,  0,   0, 7,  0,  0},
  {"Demon",     "Demons",
   127, 528,  0,  0,  15, 0,  0,  0},
  {"Yrnali",    "Yrnali",
   63, 10240, 0, 0, 1, 0, 0, 0}
};


char *action_bits[] =
{
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

// +hit +dam weight-carry weight-wield
struct str_app_type str_app[31] =
{
    { -5,-4,   0,  0 },  /* 0  */
    { -5,-4,   3,  1 },  /* 1  */
    { -3,-2,   3,  2 },
    { -3,-1,  10,  3 },  /* 3  */
    { -2,-1,  25,  4 },
    { -2,-1,  35,  5 },  /* 5  */
    { -1, 0,  45,  6 },
    { -1, 0,  55,  7 },
    {  0, 0,  65,  8 },
    {  0, 0,  75,  9 },
    {  0, 0,  85, 10 }, /* 10  */
    {  0, 0,  95, 11 },
    {  0, 0, 105, 12 },
    {  0, 0, 110, 13 }, /* 13  */
    {  0, 1, 120, 14 },
    {  1, 1, 130, 15 }, /* 15  */
    {  1, 2, 140, 16 },
    {  2, 3, 150, 17 },
    {  2, 4, 160, 18 }, /* 18  */
    {  3, 5, 170, 19 },
    {  3, 6, 180, 20 }, /* 20  */
    {  4, 7, 190, 21 },
    {  4, 8, 200, 22 },
    {  5, 9, 220, 23 },
    {  5,10, 240, 24 },
    {  6,11, 260, 25 }, /* 25  */
    {  6,12, 280, 26 },
    {  7,13, 300, 27 },
    {  7,14, 320, 28 },
    {  8,15, 340, 29 },
    {  9,16, 360, 30 }  /* 30 */
};

/* [level] backstab multiplyer (thieves only) */
byte backstab_mult[61] =
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

// + hp/lvl, shock(unused)
struct con_app_type con_app[31] =
{
    { 0,20},   /* 0 */
    { 0,25},   /* 1 */
    { 0,30},
    { 0,35},
    { 0,40},
    { 0,45},   /* 5 */
    { 0,50},
    { 0,55},
    { 0,60},
    { 0,65},
    { 0,70},   /* 10 */
    { 0,75},
    { 1,80},
    { 1,85},
    { 1,88},
    { 1,90},   /* 15 */
    { 2,95},
    { 2,97},
    { 2,99},
    { 2,99},
    { 3,99},   /* 20 */
    { 3,99},
    { 3,99},
    { 3,99},
    { 4,99},
    { 4,100},  /* 25 */
    { 4,100},
    { 4,100},
    { 5,100},
    { 5,100},
    { 6,100}  /* 30 */
};

// how many points you can learn a skill with 1 practice
struct int_app_type int_app[31] =
{
    {  1 },
    {  1 },    /* 1 */
    {  1 },
    {  1 },
    {  1 },
    {  1 },   /* 5 */
    {  1 },
    {  1 },
    {  2 },
    {  2 },
    {  2 },   /* 10 */
    {  2 },
    {  2 },
    {  2 },
    {  3 },
    {  3 },   /* 15 */
    {  3 },
    {  3 },
    {  3 },
    {  3 },
    {  4 },   /* 20 */
    {  4 },
    {  4 },
    {  4 },
    {  4 },
    {  4 },   /* 25 */
    {  4 },
    {  5 },
    {  5 },
    {  5 },
    {  5 }
};


// how many practices you get each level
struct wis_app_type wis_app[31] =
{
    { 0 },   /* 0 */
    { 0 },   /* 1 */
    { 0 },
    { 1 },
    { 1 },
    { 1 },   /* 5 */
    { 1 },
    { 1 },
    { 1 },
    { 2 },
    { 2 },   /* 10 */
    { 2 },
    { 3 },
    { 3 },
    { 3 },
    { 4 },   /* 15 */
    { 4 },
    { 4 },
    { 5 },   /* 18 */
    { 5 },
    { 6 },   /* 20 */
    { 6 },
    { 7 },
    { 7 },
    { 8 },
    { 8 },  /* 25 */
    { 9 },
    { 9 },
    { 10 },
    { 10 },
    { 11 }
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

