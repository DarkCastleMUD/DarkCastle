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
/* $Id: const.cpp,v 1.87 2004/04/28 22:05:45 urizen Exp $ */
/* I KNOW THESE SHOULD BE SOMEWHERE ELSE -- Morc XXX */

extern "C"
{
#include <stdio.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <mobile.h> // race_shit
#include <player.h> // *app_type
#include <character.h>
#include <spells.h>
#include <levels.h>

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
  "Youur Oaken Fortitude fades.",
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
//    long   trainer;           // what mob trains them (only one currently) 
//    char *                    // hint to who trains it


struct class_skill_defines g_skills[] = {
{    "scan",		SKILL_SCAN,		1,	90,	10011,	NULL, {0, 0} },
{    "consider",	SKILL_CONSIDER,		1,	90,	10011,	NULL,{0,0} },
{    "switch",		SKILL_SWITCH,		1,	90,	10011,	NULL,{0,0} },
{    "release",         SKILL_RELEASE,		1,	90,	10011, NULL, {0,0}},
{    "\n",		0,			1,	0,	0,	NULL,{0,0} }
};

struct class_skill_defines w_skills[] = {
{    "kick",		SKILL_KICK,		1,	80,	-2,	"Only the king of lobsters can teach you this.",{DEX,0} },
{    "bash",		SKILL_BASH,		2,	80,	3023, 	"You must find Bjorn, the warrior guildmaster" ,{STR,0}},
{    "redirect",	SKILL_REDIRECT,		4,	90,	-2, 	"Seek out Joshua, last I heard he was exploring somewhere north of the turning point.",{INT,0} },
{    "rescue",		SKILL_RESCUE,		5,	70,	-2, 	"The chief of the gnomes is who you want, lad.",{WIS,0} },
{    "double",          SKILL_SECOND_ATTACK,    7,      90,     -2,     "Icannot teach you that. Youmust journey north to see the son of the Delcim king.  He will train you.",{DEX,0} },
{    "disarm",  	SKILL_DISARM,		10,	70,	-2, 	"The duke of Marok is who you need to see.",{DEX,0} },
{    "headbutt",	SKILL_SHOCK,		10,	75,	-2, 	"A queen of the light holds the knowledge within her tiny skull.",{CON,0} },
{    "shield block",    SKILL_SHIELDBLOCK,      15,     85,     -2,"To learn this skill you must seek its master, the God of War in hisarena.  Ask him to please teach you.",{STR,0} },
{    "dual wield",      SKILL_DUAL_WIELD,       25,     90,	-2,	"With whirling blades and rigid honor, only these masters of weaponry can teach you the skill you need.",{DEX,0} },
{    "retreat",		SKILL_RETREAT,		17,	95,	-2, 	"A warrior of diverse colors, including yellow, can teach you this one.",{WIS,INT} },
{    "frenzy",		SKILL_FRENZY,		18,	80,	-2,	"There is a highlander chief.  The chief of mountain trail guard.  You must locate him to learn this skill.",{CON,0} },
{    "parry",		SKILL_PARRY,		20,	90,	-2, 	"You must see the constable of Newtonia to learn parry.",{DEX,0} },
{    "blindfighting",   SKILL_BLINDFIGHTING,    21,     80,     -2,	"Only on the astral planes can you find what you seek.",{INT,0}},
{    "triple",		SKILL_THIRD_ATTACK,	23,	90,	-2, 	"Only a palace chef could move his blades so fast.",{DEX,0} },
{    "hitall",		SKILL_HITALL,		27,	80,	-2, 	"Ask the son of the Sarina King to please teach you that.",{STR,0} },
{    "archery",		SKILL_ARCHERY,		33,	55,	-2, 	"Seek out the local fletcher.",{DEX,0} },
{    "deathstroke",	SKILL_DEATHSTROKE,	39,	95,	-2, 	"Only he who helps you off this mortal coil can teach you this skill.",{STR,DEX} }, 
{    "bludgeoning",	SKILL_BLUDGEON_WEAPONS,	30,	90,	3023,	NULL,{STR,0} },
{    "piercing",	SKILL_PIERCEING_WEAPONS,30,	90,	3023,	NULL,{DEX,0} },
{    "slashing",	SKILL_SLASHING_WEAPONS,	30,	90,	3023,	NULL,{DEX,0} },
{    "whipping",	SKILL_WHIPPING_WEAPONS,	30,	90,	3023,	NULL,{DEX,0} },
{    "crushing",	SKILL_CRUSHING_WEAPONS,	30,	90,	3023,	NULL,{STR,0} },
{    "tactics",		SKILL_TACTICS,		31,	95,	-2,	"The guardian of this guild has the knowledge you seek.  Speak the name of this skill in his presence.",{INT,WIS} },
{    "two handers",     SKILL_TWO_HANDED_WEAPONS,33,	85,	3023,	NULL,{STR,0} },
{    "stun",		SKILL_STUN,		35,	75,	-2, 	"Go to see any master shaolin monk.",{DEX,0} },
{    "guard",		SKILL_GUARD,		37,	95,	-2,	"A crabby old hermit who lives alone and failed college is who you want.",{STR,WIS} },
{    "riposte",		SKILL_RIPOSTE,		40,	95,	-2, 	"A race not entirely of thise world holds the knowledge, they special in weapons.",{DEX,INT} },
{    "skewer",		SKILL_SKEWER,		45,	95,	-2, 	"The man who runs the guard of sorpigal knows how to skewer, I believe.",{STR,CON} },
{    "blade shield",	SKILL_BLADESHIELD,	47,	95,	-2, 	"A short, humanoid with happiness to spare and who specializes in sleight-of-hand could help you.",{CON,DEX} }, 
{    "\n",		0,			
1,	0, 	0, 	NULL,{0,0} } };

struct class_skill_defines t_skills[] = {
{    "sneak",		SKILL_SNEAK,		1,	75,	-2, 	"You require the essence of a forest creature, a particularly sly one at that.",{0,0} },
{    "shield block",    SKILL_SHIELDBLOCK,      1, 40, -2, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you.",{0,0} },
{    "backstab",	SKILL_BACKSTAB,		2,	90,	3022,	"You have to go see Skiv the thief guildmaster.",{0,0} },
{    "hide",		SKILL_HIDE,		5, 	75,	-2, "Seek out the Abbot of a hidden monastery off shore road.",{0,0} },
{    "pick",		SKILL_PICK_LOCK,	7,	75,	-2,	"One of those swine that are homage to the old god, they are hard to find but they reside in sorpigal.",{0,0} },
{    "slip",  		SKILL_SLIP,		10,	80,	-2,	"The desert banditos know, but they take their secrets to the grave.",{0,0} },
{    "palm",		SKILL_PALM,		10,	80,	-2,	"Seek out a pirate captain in a relatively calm port of call.  He believes in honor amongst thieves.",{0,0} },
{    "dual wield",      SKILL_DUAL_WIELD,       10,	90,     -2, "With whirling blades and rigid honor, only these masters of weaponry can teach you the skill you need.",{0,0} },
{    "pocket",		SKILL_POCKET,		12,	80,	-2, 	"You might wish to find that conniving dullard in silverdale, can't 'member his name but it rhymes with knave.",{0,0} },
{    "dodge",		SKILL_DODGE,		15,	90,	-2, 	"Hop like a bunny, quick like a bee, this location is so funny, you'll go wee wee wee.  Don't forget to bring your scythe however.",{0,0} },
{    "stalk",		SKILL_STALK,		15,	80,	-2, 	"Seek out the sneaky little rodent who is a gossiper and thief!",{0,0} },
{    "trip",		SKILL_TRIP,		17,	75,	-2, 	"The old goblin in the caves knows, but he will not be cooperative.  You might need to use violence.",{0,0} },
{    "blindfighting",   SKILL_BLINDFIGHTING, 19, 60,     -2, "Only on the astral planes can you find what you seek.",{0,0}},
{    "steal",		SKILL_STEAL,		20,	80,	-2, "There's a little thief around town named Caijin.  Annoying little bastard.  Ask him to please teach you.",{0,0} },
{    "disarm",		SKILL_DISARM,		23,	60,	-2, "The duke of Marok is who you need to see.",{0,0}},
{    "eyegouge", SKILL_EYEGOUGE, 42, 95, 3022, NULL, {0,0}},
{    "vitalstrike",     SKILL_VITAL_STRIKE,	25,	80,	-2,	"The individual you seek is skilled in the arts of legerdemain, both verbal and physical.",{0,0} },
{    "circle",		SKILL_CIRCLE,		30,	75,	-2, "Seek the king of a long lost civilization.  I only hear that they are the last of the Jiran.",{0,0} },
{    "bludgeoning",	SKILL_BLUDGEON_WEAPONS,	30,	80,	3022,	NULL,{0,0} },
{    "piercing",	SKILL_PIERCEING_WEAPONS,30,	80,	3022,	NULL,{0,0} },
{    "slashing",	SKILL_SLASHING_WEAPONS,	30,	80,	3022,	NULL,{0,0} },
{    "crushing",	SKILL_CRUSHING_WEAPONS,	30,	80,	3022,	NULL,{0,0} },
{    "deceit",		SKILL_DECEIT,		31,	90,	-2,	"The guardian of this guild has the knowledge you seek.  Speak the name of this skill in his presence.",{0,0} },
{    "dualbackstab",	SKILL_DUAL_BACKSTAB,	40,	80,	-2, 	"Look for a thief who travels in the most dangerous of places.",{0,0} },
{    "\n",		0,			1,	0,	0,	NULL,{0,0}}
};

struct class_skill_defines a_skills[] = {
{    "harmtouch",	SKILL_HARM_TOUCH,	1,	90,	-2, 	"He who lays the dead in the ground has your ticket.",{0,0} },
{    "sneak",		SKILL_SNEAK,		2,	60,	-2,	"You require the essence of a forest creature, a particularly sly one at that.",{0,0} }, 
{    "kick",		SKILL_KICK,		3,	75,	10005,	NULL,{0,0} },
{    "shield block",    SKILL_SHIELDBLOCK, 5, 50, -2, "To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you.",{0,0} },
{    "dual wield",      SKILL_DUAL_WIELD, 10,	60, -2, "With whirling blades and rigid honor, only these masters of weaponry can teach you the skill you need.",{0,0} },
{    "hide", 		SKILL_HIDE,		15,	60,	-2, "Seek out the Abbot of a hidden monastery off shore road.",{0,0} },
{    "backstab",	SKILL_BACKSTAB,		17,	80, 10005, 	NULL,{0,0}},
{    "double",		SKILL_SECOND_ATTACK,	20,	75, -2, 	"Travel to Delcim and seek the prince.  He will teach you that.",{0,0} },
{    "dodge",   	SKILL_DODGE,		23,	75,	-2, 	"Hop like a bunny, quick like a bee, this location is so funny, you'll go wee wee wee.  Don't forget to bring your scythe however.",{0,0} },
{    "trip",  		SKILL_TRIP,		25,	75,	-2, 	"The old goblin in the caves knows, but he will not be cooperative.  You might need to use violence.",{0,0} },
{    "blindfighting",   SKILL_BLINDFIGHTING,    30,   50,-2, "Only on the astral planes can you find what you seek.",{0,0}},
{    "bludgeoning",	SKILL_BLUDGEON_WEAPONS,	30,	60,	10005,	NULL,{0,0} },
{    "piercing",	SKILL_PIERCEING_WEAPONS,30,	60,	10005,	NULL,{0,0} },
{    "slashing",	SKILL_SLASHING_WEAPONS,	30,	60,	10005,	NULL,{0,0} },
{    "crushing",	SKILL_CRUSHING_WEAPONS,	30,	60,	10005,	NULL,{0,0} },
{    "detect good",	SPELL_DETECT_GOOD,	11,	90,	-2, "Seek out a Dark Priest in a church full of zombies.",{0,0} },
{    "detect invisibility",SPELL_DETECT_INVISIBLE,	12,	90,	10005,	NULL,{0,0} },
{    "curse",		SPELL_CURSE,		13,	90,	10005,	NULL,{0,0} },
{    "vampiric touch",	SPELL_VAMPIRIC_TOUCH,	15,	90,	10005,	NULL,{0,0} },
{    "animate dead",	SPELL_ANIMATE_DEAD,	15,	90,	10005,	NULL,{0,0} },
{    "dismiss corpse",  SPELL_DISMISS_CORPSE,	15,	90,	10005,	NULL,{0,0} },
{    "invisibility",	SPELL_INVISIBLE,	16,	90,	10005,	NULL,{0,0} },
{    "globe of darkness",SPELL_GLOBE_OF_DARKNESS, 17,	90, -2, "Find the Black King in the game of life.",{0,0} },
{    "lightning bolt",	SPELL_LIGHTNING_BOLT,	18,	90,	10005,	NULL,{0,0} },
{    "poison",		SPELL_POISON,		21,	90,	-2, "Find the spider with a red hourglass in the mushroom caverns.",{0,0} },
{    "weaken",		SPELL_WEAKEN,		23,	90,	-2,	"What tangled web we weave, when we first deceive.",{0,0} },
{    "enchant weapon",	SPELL_ENCHANT_WEAPON,	24,	90,10005,	NULL,{0,0} },
{    "fireball",	SPELL_FIREBALL,		28,	90,	10005,	NULL,{0,0} },
{    "visage of hate",  SPELL_VISAGE_OF_HATE,	31,	90,	-2,	"The guardian of this guild has the knowledge you seek.  Speak the name of this spell in his presence.",{0,0} },
{    "beacon",		SPELL_BEACON,		33,	90,	10005,	NULL,{0,0} },
{    "dispel good",	SPELL_DISPEL_GOOD,	35,	90, 10005,	NULL,{0,0} },
{    "firestorm",	SPELL_FIRESTORM,	38,	90,	10005,	NULL,{0,0} },
{    "acid shield",	SPELL_ACID_SHIELD,	40,	90,	10005,	NULL,{0,0} },
{    "blindness",	SPELL_BLINDNESS,	42,	90,	10005,	NULL,{0,0} },
{    "protection from good",	SPELL_PROTECT_FROM_GOOD,	45,	90,	10005,	NULL,{0,0} },
{    "stone skin",	SPELL_STONE_SKIN,	46,	90,	-2, "A woman strong in battle surrounded by heavy magical influences calls to you.",{0,0} }, 
{    "acid blast",	SPELL_ACID_BLAST,	48,	90,	 10005,	NULL ,{0,0}}, 
{    "vampiric aura",  SPELL_VAMPIRIC_AURA, 50, 95, -2, "I don't know it myself.. but I heard Lord Soth knew something about it.",{0,0}},
{    "\n",		0,			1,	0,	0, 	NULL,{0,0}}
};

struct class_skill_defines p_skills[] = {
{    "layhands",	SKILL_LAY_HANDS,	1,	90,	-2,	"A physician of cunning skill and high honor can teach you this skill, my lad.",{0,0} },
{    "bash",		SKILL_BASH,		2,	75, 10006, "You must find a local warrior.  He said something about a field filled with bees.",{0,0} },
{    "shield block",    SKILL_SHIELDBLOCK, 3, 50, 10006, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you.",{0,0} },
{    "kick",		SKILL_KICK,		5,	80,	-2,	"Only the king of lobsters can teach you this.",{0,0} },
{    "rescue",		SKILL_RESCUE,		7,	80,	-2, 	"The chief of the gnomes is who you want, lad." ,{0,0} },
{    "dual wield",      SKILL_DUAL_WIELD,       10,     60,-2, "With whirling blades and rigid honor, only these masters of weaponry can teach you the skill you need.",{0,0}  },
{    "dodge",		SKILL_DODGE,		13,	20,	-2, 	"Hop like a bunny, quick like a bee, this location is so funny, you'll go wee wee wee.  Don't forget to bring your scythe however.",{0,0} },
{    "double",		SKILL_SECOND_ATTACK,	20,	75,	-2, 	"Seek the Delcim Prince my child.  He shall train you." ,{0,0}},
{    "two handers",     SKILL_TWO_HANDED_WEAPONS,23,	90,	10006,	NULL,{0,0} },
{    "parry",		SKILL_PARRY,		25,	50,	-2, 	"You have to go see the constable of Newtonia to learn parry.",{0,0} },
{    "triple",		SKILL_THIRD_ATTACK,	30,	70,	-2, 	"Only a palace chef could move his blades so fast.",{0,0} },
{    "bludgeoning",	SKILL_BLUDGEON_WEAPONS,	30,	80,	10006,	NULL,{0,0} },
{    "slashing",	SKILL_SLASHING_WEAPONS,	30,	80,	10006,	NULL,{0,0} },
{    "crushing",	SKILL_CRUSHING_WEAPONS,	30,	80,	10006,	NULL,{0,0} },
{    "bless",		SPELL_BLESS,		10,	90,	-2, "Go see the newt priest in Newtonia.",{0,0}},
{    "cure light",	SPELL_CURE_LIGHT,	10,	90,	-2, "You need to go see Minare.   He was last seen in the caves north east of town.",{0,0}},
{    "earthquake",	SPELL_EARTHQUAKE,	11,	90,	-2,	"You need to go see Minare.   He was last seen in the caves north east of town." ,{0,0}}, 
{    "create food",	SPELL_CREATE_FOOD,	12,	90,	10006,	NULL ,{0,0}},
{    "create water",	SPELL_CREATE_WATER,	13,	90,	10006,	NULL ,{0,0}},
{    "detect evil",	SPELL_DETECT_EVIL,	14,	90,	10006,	NULL ,{0,0}},
{    "cure serious",	SPELL_CURE_SERIOUS,	15,	90,	-2,	"Go see the newt priest in Newtonia.",{0,0}},
{    "detect invisibility", SPELL_DETECT_INVISIBLE,	16,	90,	10006,	NULL ,{0,0}},
{    "detect poison",	SPELL_DETECT_POISON,	18,	90,	10006,	NULL ,{0,0}},
{    "remove poison",	SPELL_REMOVE_POISON,	19,	90,	10006,	NULL ,{0,0}},
{    "cure critic",	SPELL_CURE_CRITIC,	25,	90,	-2,	"Find Aruncus the Druid in the fields north of town!" ,{0,0}},
{    "sense life",	SPELL_SENSE_LIFE,	26,	90,	-2,	"Find the Black Queen in the game of life." ,{0,0}},
{    "strength",	SPELL_STRENGTH,		27,	90,	10006,	NULL ,{0,0}},
{    "heal",		SPELL_HEAL,		30,	90,	10006,	NULL ,{0,0}},
{    "blessed halo",	SPELL_BLESSED_HALO,	31,	90,	-2,	"The guardian of this guild has the knowledge you seek.  Speak the name of this spell in his presence." ,{0,0}},
{    "protection from evil", SPELL_PROTECT_FROM_EVIL, 33, 90,	-2,	"A short rotund priest with a face you could cut diamonds with can help you.",{0,0}},
{    "harm",		SPELL_HARM,		35,	90,	-2,	"You want an Abbot of mystical power who stands menancingly." ,{0,0}},
{    "dispel evil",	SPELL_DISPEL_EVIL,	40,	90,	10006,	NULL ,{0,0}},
{    "sanctuary",	SPELL_SANCTUARY,	45,	90,	-2, "Find a dragon in the forest, she is armed and dangerous." ,{0,0}},
{    "power harm",	SPELL_POWER_HARM,	48,	90,	10006,	NULL ,{0,0}},
{    "\n",		0,			2,	0,	0,	NULL,{0,0}}
};


struct class_skill_defines b_skills[] = {
{    "dual wield",      SKILL_DUAL_WIELD,       1,      85,     10007, 	"You have to go see Mooral the barbarian guildmaster." ,{DEX,0}},
{    "kick",  		SKILL_KICK,		3,	80,	10007,	"Only the king of lobsters can teach you this."  ,{DEX,0}},
{    "bash",		SKILL_BASH,		2,	90,	-2, 	"You must find a local warrior.  He said something about a field filled with bees." ,{STR,0}},
{    "parry",		SKILL_PARRY,		5,	70,	-2, 	"You must see the constable of Newtonia to learn parry." ,{DEX,0}},
{    "shield block",    SKILL_SHIELDBLOCK,      10,     70,     -2, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you." ,{DEX,0}},
{    "double",		SKILL_SECOND_ATTACK,	8,	85,	-2, 	"Seek the Prince of Delcim for your training in that." ,{DEX,0}},
{    "blood fury",	SKILL_BLOOD_FURY,	12,	95,	-2, 	"Look for a gpaceless kimg who pigks his nose" ,{CON,WIS}},
{    "crazedassault",	SKILL_CRAZED_ASSAULT,	15,	95,	-2, 	"Speak with the guard of the monk guild.  He is skilled in matters of concentration such as this." ,{WIS,STR}},
{    "frenzy",		SKILL_FRENZY,		18,	90,	-2,	"There is a highlander chief.  The chief of mountain trail guard.  You must locate him to learn this skill." ,{CON,0}},
{    "rage",		SKILL_RAGE,		20,	95,	-2, 	"A fierceful force of elementals is who you seek." ,{STR,CON}},
{    "triple",		SKILL_THIRD_ATTACK,	25,	85,	-2, 	"Only a palace chef could move his blades so fast." ,{DEX,0}},
{    "battlecry",	SKILL_BATTLECRY,	27,	95,	-2, 	"Ahh! You must seek out the bravest barbarian of them all, a mortal god of the norse he is." ,{WIS,INT}},
{    "blindfighting",   SKILL_BLINDFIGHTING,    28,     55,	-2,	"Only on the astral planes can you find what you seek.",{INT,0}},
{    "headbutt",	SKILL_SHOCK,		33,	90,	-2, 	"A queen of the light holds the knowledge within her tiny skull." ,{CON,0}},
{    "bludgeoning",	SKILL_BLUDGEON_WEAPONS,	30,	85,	10007,	NULL ,{STR,0}},
{    "whipping",	SKILL_WHIPPING_WEAPONS, 30,     85,     10007,  NULL, {DEX,0}},
{    "piercing",	SKILL_PIERCEING_WEAPONS,30,	85,	10007,	NULL ,{DEX,0}},
{    "slashing",	SKILL_SLASHING_WEAPONS,	30,	85,	10007,	NULL ,{DEX,0}},
{    "crushing",	SKILL_CRUSHING_WEAPONS,	30,	85,	10007,	NULL ,{STR,0}},
{    "ferocity",	SKILL_FEROCITY,		31,	95,	-2,	"The guardian of this guild has the knowledge you seek.  Speak the name of this skill in his presence." ,{INT,STR}},
{    "two handers",     SKILL_TWO_HANDED_WEAPONS,35,	90,	10007,	NULL ,{STR,0}},
{    "archery",		SKILL_ARCHERY, 		38,	80,	10007, NULL,{DEX,0}},
{    "berserk",		SKILL_BERSERK,		40,	95,	-2, 	"A being of pure rage who lives in a tower, he is a overgrown lizard with a forked tongue." ,{CON,STR}}, 
{    "hitall",		SKILL_HITALL,		45, 	90,	10007, NULL, {STR,0}},
{    "bullrush",	SKILL_BULLRUSH,		50,     95,     -2, "Speak the name of this skill to Gorgar, our guild's guardian to be taught.", {STR,CON}},

{    "\n",		0,			1,	
0,	0, 	NULL,{0,0}} };

struct class_skill_defines k_skills[] = {
{    "kick",		SKILL_KICK,		1,	90,	10008, 	"You have to go see Grox the monk guildmaster.",{0,0}},
{    "dodge",		SKILL_DODGE,		2,	75,	-2, 	"Hop like a bunny, quick like a bee, this location is so funny, you'll go wee wee wee.  Don't forget to bring your scythe however." ,{0,0}},
{    "redirect",	SKILL_REDIRECT,		3,	75,	-2, 	"Seek out Joshua, last I heard he was exploring somewhere north of the turning point.",{0,0} }, 
{    "trip",		SKILL_TRIP,		5,	75,	10008, 	NULL,{0,0}},
{    "shield block",    SKILL_SHIELDBLOCK,      7,      60,     -2, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you.",{0,0}},
{    "rescue",  	SKILL_RESCUE,		10,	70,	-2, 	"The chief of the gnomes is who you want, lad.",{0,0} },
{    "eagleclaw",	SKILL_EAGLE_CLAW,	17,	75,	-2, 	"A hermit has the knowledge to this skill, but luckily for you this hermit is a nice one.",{0,0}},
{    "dual wield",      SKILL_DUAL_WIELD,       20,     60,	-2,	"With whirling blades and rigid honor, only these masters of weaponry can teach you the skill you need.",{0,0} },
{    "stun",		SKILL_STUN,		30,	60,	-2, 	"You must see a master shaolin monk.",{0,0}},
{    "hand to hand",	SKILL_HAND_TO_HAND,	30,	90,	10008,	NULL,{0,0} },
{    "quiver",		SKILL_QUIVERING_PALM,	40,	75,	-2, 	"A knight of a most dangerous and twisted place quite large holds the knowledge you seek.",{0,0}},
{    "purify",		KI_PURIFY+KI_OFFSET,	5,	80,	-2, 	"A tower of water so pure, only he could know.",{0,0}},
{    "punch",		KI_PUNCH+KI_OFFSET,	10,	80,	-2, 	"You seek a brother in a secret monastery.",{0,0}},
{    "sense",		KI_SENSE+KI_OFFSET,	20,	80,	-2, 	"A creature who lives in the damp dark, and causes it, could know.",{0,0}},
{    "stance",		KI_STANCE+KI_OFFSET,	24,	80,	-2, "A fair-skinned warrior of untold skill in weapons awaits your challenge in the thundering depths.",{0,0}},
{    "speed",		KI_SPEED+KI_OFFSET,	27,	80,	-2, 	"Hmm, I do not know where the knowledge went, but I sense it on the east continent, moving around.",{0,0}},
{    "agility",		KI_AGILITY+KI_OFFSET,	31,	90,	-2,	"The guardian of this guild has the knowledge you seek.  Speak the name of this spell in his presence.",{0,0} },
{    "storm",		KI_STORM+KI_OFFSET,	35,	80,	-2, 	"See a purveyer of storms about this one",{0,0}},
{    "disrupt",		KI_DISRUPT+KI_OFFSET,	41,	80,	10008, NULL,{0,0}},
{    "blast",		KI_BLAST+KI_OFFSET,	45,	80,	-2, 	"The brother of a secret monastery who is constantly plagued by indegestion can help you out.",{0,0}}, {    "\n",		0,			1,	0,	0, 	NULL,{0,0}}
 };

struct class_skill_defines r_skills[] = {
{    "hide",		SKILL_HIDE,		1,	80,	10013, 	"You have to go see Hawk the ranger guildmaster.",{0,0} },
{    "redirect",	SKILL_REDIRECT,		2,	75,	-2, 	"Seek out Joshua, last I heard he was exploring somewhere north of the turning point." ,{0,0}},
{    "tame",		SKILL_TAME,		3,	75,	10013, 	"You have to go see Hawk the ranger guildmaster." ,{0,0}},
{    "dodge",		SKILL_DODGE,		4,	70,	-2, 	"Hop like a bunny, quick like a bee, this location is so funny, you'll go wee wee wee.  Don't forget to bring your scythe however." ,{0,0}},
{    "dual wield",	SKILL_DUAL_WIELD,       5,      80,	-2,	"With whirling blades and rigid honor, only these masters of weaponry can teach you the skill you need." ,{0,0}},
{    "trip",		SKILL_TRIP,		7,	75,	-2, 	"The old goblin in the caves knows, but he will not be cooperative.  You might need to use violence." ,{0,0}},
{    "shield block",    SKILL_SHIELDBLOCK,      9,      40,     -2,	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you." ,{0,0}},
{    "kick",		SKILL_KICK,		11,	50,	-2, 	"Only the king of lobsters can teach you this." ,{0,0}},
{    "archery",		SKILL_ARCHERY,		12,	80,	-2, 	"Seek out the local fletcher." ,{0,0}},
{    "ambush",		SKILL_AMBUSH,		13,	75,	-2, 	"Seek out Samuel, one of the oldest rangers, for this skill.  He roams in a forest." ,{0,0}},
{    "double",		SKILL_SECOND_ATTACK,	14,	80,	-2, 	"See the King of Delcim's son." ,{0,0}},
{    "disarm",		SKILL_DISARM,		15,	50,	-2, 	"The duke of Marok is who you need to see.",{0,0}},
{    "forage",		SKILL_FORAGE,		17,	75,	-2, 	"Seek out the swampy hermit." ,{0,0}},
{    "track",		SKILL_TRACK,		20,	80,	-2, 	"I am not too sure who knows of this one, he is but a shopkeeper now." ,{0,0}},
{    "blindfighting",   SKILL_BLINDFIGHTING,    21,     60,	-2,	"Only on the astral planes can you find what you seek.",{0,0}},
{    "rescue",		SKILL_RESCUE,		23,	75,	-2, 	"The chief of the gnomes is who you want, lad.",{0,0} },
{    "stun",  		SKILL_STUN,		30,	60,	-2, 	"You must see a master shaolin monk." ,{0,0}},
{    "piercing",	SKILL_PIERCEING_WEAPONS,30,	50,	10013,	NULL ,{0,0}},
{    "slashing",	SKILL_SLASHING_WEAPONS,	30,	50,	10013,	NULL ,{0,0}},
{    "whipping",	SKILL_WHIPPING_WEAPONS,	30,	50,	10013,	NULL ,{0,0}},
{    "bee sting",	SPELL_BEE_STING,	1,	90,	10013,	NULL ,{0,0}},
{    "eyes of the owl",	SPELL_EYES_OF_THE_OWL,	5,	90,	-2, "Find and kill a weak goblin in the mushroom caverns." ,{0,0}},
{    "bee swarm",	SPELL_BEE_SWARM,	10,	90,	10013,	NULL ,{0,0}},
{    "entangle",	SPELL_ENTANGLE,		15,	90,	10013,	NULL ,{0,0}},
{    "barkskin",	SPELL_BARKSKIN,		25,	90, -2, "Talk to Shargugh, he's the MAN! Baby! Just ask dinas ;)",{0,0}},
{    "feline agility",	SPELL_FELINE_AGILITY,	30,	90,	10013,	NULL ,{0,0}},
{    "herb lore",	SPELL_HERB_LORE,	35,	90,	10013,	NULL ,{0,0}},
{    "forest meld",	SPELL_FOREST_MELD,	37,	90,	10013,	NULL ,{0,0}},
{    "call follower",	SPELL_CALL_FOLLOWER,	40,	90,	10013,	NULL ,{0,0}},
{    "creeping death",	SPELL_CREEPING_DEATH,	45,	90,	10013,	NULL ,{0,0}},
{      "\n",		0,			1,	0,	0, 	NULL,{0,0}}
};

struct class_skill_defines d_skills[] = { // bard skills
{      "hide",			SKILL_HIDE,			2,	75,	-2, 	"Seek out the Abbot of a hidden monastery off shore road.",{0,0}},
{      "dodge",			SKILL_DODGE,			5,	85,	-2, 	"Hop like a bunny, quick like a bee, this location is so funny, you'll go wee wee wee.  Don't forget to bring your scythe however." ,{0,0}},
{      "dual wield",    	SKILL_DUAL_WIELD,       	10,     75, -2, "With whirling blades and rigid honor, only these masters of weaponry can teach you the skill you need." ,{0,0}},
{      "double",		SKILL_SECOND_ATTACK,		17,	85,	-2, 	"You must travel to the Prince of Delcim to recieve training.",{0,0}},
{      "bludgeoning",		SKILL_BLUDGEON_WEAPONS,		30,	80,	3201,	NULL ,{0,0}},
{      "piercing",		SKILL_PIERCEING_WEAPONS,	30,	80,	3201,	NULL ,{0,0}},
{      "slashing",		SKILL_SLASHING_WEAPONS,		30,	80,	3201,	NULL ,{0,0}},
{      "whipping",		SKILL_WHIPPING_WEAPONS,		30,	80,	3201,	NULL,{0,0} },
{      "crushing",		SKILL_CRUSHING_WEAPONS,		30,	80,	3201,	NULL ,{0,0}},
{      "listsongs",		SKILL_SONG_LIST_SONGS,		1,	100,	3201, 	NULL,{0,0}},
{      "whistle sharp",		SKILL_SONG_WHISTLE_SHARP, 	1,	90,	3201, 	NULL,{0,0}},
{      "stop",			SKILL_SONG_STOP,		5,	90,	3201, 	NULL,{0,0}},
{      "irresistable ditty",	SKILL_SONG_UNRESIST_DITTY,	5,	90,	3201, 	NULL,{0,0}},
{      "traveling march",	SKILL_SONG_TRAVELING_MARCH,	7,	90,	3201, 	NULL,{0,0}},
{      "bountiful sonnet",	SKILL_SONG_BOUNT_SONNET,	9,	90,	3201, 	NULL,{0,0}},
{      "insane chant",		SKILL_SONG_INSANE_CHANT,	10,	90,	3201, 	NULL,{0,0}},
{      "glitter dust",		SKILL_SONG_GLITTER_DUST,	10,	90,	3201, 	NULL,{0,0}},
{      "synchronous chord",	SKILL_SONG_SYNC_CHORD,		13,	90,	3201, 	NULL,{0,0}},
{      "healing melody",	SKILL_SONG_HEALING_MELODY,	15,	90,	3201, 	NULL,{0,0}},
{      "sticky lullaby",	SKILL_SONG_STICKY_LULL,		17,	90,	3201, 	NULL,{0,0}},
{      "revealing stacato",	SKILL_SONG_REVEAL_STACATO,	20,	90,	3201, 	NULL,{0,0}},
{      "flight of the bumblebee", SKILL_SONG_FLIGHT_OF_BEE,	20,	90,	3201, 	NULL,{0,0}},
{      "jig of alacrity",	SKILL_SONG_JIG_OF_ALACRITY,	23,	90,	3201, 	NULL,{0,0}},
{      "note of knowledge",	SKILL_SONG_NOTE_OF_KNOWLEDGE,	25,	90,	3201, 	NULL,{0,0}},
{      "terrible clef",		SKILL_SONG_TERRIBLE_CLEF,	27,	90,	3201, 	NULL,{0,0}},
{      "soothing rememberance",	SKILL_SONG_SOOTHING_REMEM,	31,	90,	3201, 	NULL,{0,0}},
{      "forgetful rhythm",	SKILL_SONG_FORGETFUL_RHYTHM,	35,	90,	3201, 	NULL,{0,0}},
{      "searching song",	SKILL_SONG_SEARCHING_SONG,	35,	90,	3201, 	NULL,{0,0}},
{      "vigilant siren",	SKILL_SONG_VIGILANT_SIREN,	43,	90,	3201, 	NULL,{0,0}},
{      "astral chanty",		SKILL_SONG_ASTRAL_CHANTY,	40,	90,	3201, 	NULL,{0,0}},
{      "disarming limerick", 	SKILL_SONG_DISARMING_LIMERICK,	43,	90,	3201, 	NULL,{0,0}},
{      "shattering resonance",	SKILL_SONG_SHATTERING_RESO,	45,	90,	3201, 	NULL,{0,0}},
{      "\n",			0,				1,	0,	0, 	NULL,{0,0}}
};


// 3203 is the druid guild guard
struct class_skill_defines u_skills[] = { // druid skills
{    "dodge",		SKILL_DODGE,		7,	5,	-2, 	NULL,{0,0}},
{    "dual wield",      SKILL_DUAL_WIELD,       11,      40, -2, "With whirling blades and rigid honor, only these masters of weaponry can teach you the skill you need." ,{0,0}},
{    "bludgeoning",	SKILL_BLUDGEON_WEAPONS,	30,	40,	3203,	NULL ,{0,0}},
{    "piercing",	SKILL_PIERCEING_WEAPONS,30,	40,	3203,	NULL ,{0,0}},
{    "eyes of the owl",	SPELL_EYES_OF_THE_OWL,	1,	90,	-2,	"Find and kill a weak goblin in the mushroom caverns." ,{0,0}},
{    "blue bird",       SPELL_BLUE_BIRD,        2,      90,     3203,   NULL ,{0,0}},
{    "cure light",	SPELL_CURE_LIGHT,	3,	90,	3203,	NULL ,{0,0}},
{    "cure serious",	SPELL_CURE_SERIOUS,	6,	90, -2, "Go see the newt priest in Newtonia.",{0,0}},
{    "oaken fortitude", SPELL_OAKEN_FORTITUDE,  8,      95, 3203, NULL, {0,0}},
{    "cure critic",	SPELL_CURE_CRITIC,	10,	90,	-2, "Find Aruncus the Druid in the fields north of town!" ,{0,0}},
{    "control weather",	SPELL_CONTROL_WEATHER,	13,	90, -2, "Only a wizard skilled in weather can teach you, Duhh!",{0,0}},
{    "sun ray",		SPELL_SUN_RAY,		14,	90,	3203,	NULL ,{0,0}},
{    "sense life",	SPELL_SENSE_LIFE,	15,	90,	-2, "Find the Black Queen in the game of life.",{0,0} },
{    "water breathing",	SPELL_WATER_BREATHING,	17,	90,	3203,	NULL ,{0,0}},
{    "rapid mend",	SPELL_RAPID_MEND,	18,	90,	-2, "I will teach you this if you return to me a stolen peacock's feather." ,{0,0}},
{    "attrition",	SPELL_ATTRITION,	19,	90,	-2, "Bring me a pair of indigo gloves if you wish to learn this." ,{0,0}},
{    "stone shield",	SPELL_STONE_SHIELD,	20,	90,	-2, "The druid of druids can help you learn this most quintisential skill.",{0,0}},
{    "drown",		SPELL_DROWN,		22,	90,	3203,	NULL ,{0,0}},
{    "lightning shield",SPELL_LIGHTNING_SHIELD,	23,	90,	-2,	"A near victim of infanticide the Lord of the Tallest mountain knows lightning shield.",{0,0}},
{    "power heal",	SPELL_POWER_HEAL,	26,	90,	-2,	"Only the most pure of beasts with horn atop head can help you.  Look in the mountains." ,{0,0}},
{    "camouflage",	SPELL_CAMOUFLAGE,	28,	90,	3203,	NULL ,{0,0}},
{    "summon",SPELL_SUMMON,40,90,-2,"You seek out an individual of much control over the transcendental in a tower of pure energy.",{0,0}},
{    "summon familiar", SPELL_SUMMON_FAMILIAR,	30,	90,	-2,	"To learn this skill you must bust a nut....err...bring me a nut.",{0,0}},
{    "dismiss familiar",SPELL_DISMISS_FAMILIAR, 30,	90,	3203,	NULL ,{0,0}},
{    "lighted path",    SPELL_LIGHTED_PATH,	33,	90,	-2,	"You must learn about tracking to accomplish this spell.  Go talk to the ranger Woody about lighted path.",{0,0}},
{    "barkskin",	SPELL_BARKSKIN,		35,	90,	-2,	"Talk to Shargugh, he's the MAN! Baby! Just ask dinas ;)",{0,0}},
{    "iron roots",	SPELL_IRON_ROOTS,	36,	90,	-2,	"A druid of old age and virulance is who you want. He will be in the company of other druids.",{0,0}},
{    "resist acid",	SPELL_RESIST_ACID,	37,	90,	-2,	"You need to seek out Bill the pharmicist.  He knows about such things." ,{0,0}},
{    "resist energy",	SPELL_RESIST_ENERGY,	39,	90,	-2,	"An electric individual who will surely light you up with his mastery of energetic discourse. (phew)" ,{0,0}},
{    "greater stone shield",SPELL_GREATER_STONE_SHIELD,	42,	90,-2,	"He of an earthy personality who rules with a stone fist is who you want.",{0,0}},
{    "stone skin",	SPELL_STONE_SKIN,	45,	90,	-2,	"A woman strong in battle surrounded by heavy magical influences calls to you." ,{0,0}}, 
{    "\n",		0,			1,	0,	0, 	 NULL,{0,0}} 
};


// cleric 3021 guildmaster
struct class_skill_defines c_skills[] = { // cleric skills
{    "dodge",		SKILL_DODGE,		7,	5,	-2, 	"Hop like a bunny, quick like a bee, this location is so funny, you'll go wee wee wee.  Don't forget to bring your scythe however." ,{0,0}},
{    "dual wield",      SKILL_DUAL_WIELD,       10,     40,-2, "With whirling blades and rigid honor, only these masters of weaponry can teach you the skill you need."  ,{0,0}},
{    "bludgeoning",	SKILL_BLUDGEON_WEAPONS,	30,	40,	3021,	NULL,{0,0} },
{    "crushing",	SKILL_CRUSHING_WEAPONS,	30,	40,	3021,	NULL ,{0,0}},
{    "armor",		SPELL_ARMOR,		1,	90,	3021,	NULL,{0,0} },
{    "cause light",	SPELL_CAUSE_LIGHT,	1,	90,	3021,	NULL,{0,0} },
{    "cure light",	SPELL_CURE_LIGHT,	1,	90,	3021,	NULL,{0,0} },
{    "create water",	SPELL_CREATE_WATER,	2,	90,	3021,	NULL,{0,0} },
{    "detect poison",	SPELL_DETECT_POISON,	2,	90,	3021,	NULL,{0,0} },
{    "continual light",	SPELL_CONT_LIGHT,	2,	90,	3021,	NULL,{0,0} },
{    "create food",	SPELL_CREATE_FOOD,	3,	90,	3021,	NULL ,{0,0}},
{    "detect magic",	SPELL_DETECT_MAGIC,	3,	90,	3021,	NULL ,{0,0}},
{    "refresh",		SPELL_REFRESH,		3,	90,	3021,	NULL ,{0,0}},
{    "cure blind",	SPELL_CURE_BLIND,	4,	90,	3021,	NULL ,{0,0}},
{    "detect evil",	SPELL_DETECT_EVIL,	4,	90,	3021,	NULL ,{0,0}},
{    "detect good",	SPELL_DETECT_GOOD,	4,	90,	-2, "Seek out a Dark Priest in a church full of zombies." ,{0,0}},
{    "bless",		SPELL_BLESS,		5,	90,	-2, "Go see the newt priest in Newtonia." ,{0,0}},
{    "cause serious",	SPELL_CAUSE_SERIOUS,	5,	90,	-2, "You need to go see Minare.   He was last seen in the caves north east of town.",{0,0}},
{    "cure serious",	SPELL_CURE_SERIOUS,	5,	90,-2, "Go see the newt priest in Newtonia.",{0,0}},
{    "detect invisibility", SPELL_DETECT_INVISIBLE,	5,	90,	3021,	NULL ,{0,0}},
{    "know alignment",	SPELL_KNOW_ALIGNMENT,	5,	90,	3021,	NULL ,{0,0}},
{    "blindness",	SPELL_BLINDNESS,	6,	90,	3021,	NULL ,{0,0}},
{    "earthquake",	SPELL_EARTHQUAKE,	7,	90,	-2,	"You need to go see Minare.  He was last seen in the caves north east of town." ,{0,0}},
{    "poison",		SPELL_POISON,		8,	90,	-2, "Find the spider with a red hourglass in the mushroom caverns." ,{0,0}},
{    "cause critical",	SPELL_CAUSE_CRITICAL,	9,	90,	3021,	NULL ,{0,0}},
{    "cure critical",	SPELL_CURE_CRITIC,	9,	90,	3021,	NULL ,{0,0}},
{    "remove poison",	SPELL_REMOVE_POISON,	9,	90,	3021,	NULL ,{0,0}},
{    "dispel evil",	SPELL_DISPEL_EVIL,	10,	90,	3021,	NULL ,{0,0}},
{    "dispel good",	SPELL_DISPEL_GOOD,	10,	90,	3021,	NULL ,{0,0}},
{    "locate object",	SPELL_LOCATE_OBJECT,	10,	90,	-2, "A sanitation engineer works behind what people see, always knowing where everything is." ,{0,0}},
{    "sense life",	SPELL_SENSE_LIFE,	10,	90,-2, "Find the Black Queen in the game of life." ,{0,0}},
{    "word of recall",	SPELL_WORD_OF_RECALL,	11,	90,	3021,	NULL ,{0,0}},
{    "call lightning",	SPELL_CALL_LIGHTNING,	12,	90,	3021,	NULL ,{0,0}},
{    "harm",		SPELL_HARM,		12,	90,	-2, "You want an Abbot of mystical power who stands menancingly." ,{0,0}},
{    "remove curse",	SPELL_REMOVE_CURSE,	12,	90,	3021,	NULL ,{0,0}},
{    "remove paralysis",SPELL_REMOVE_PARALYSIS,	12,	90,	3021,	NULL ,{0,0}},
{    "control weather",	SPELL_CONTROL_WEATHER,	13,	90, -2, "Only a wizard skilled in weather can teach you, Duhh!",{0,0}},
{    "flamestrike",	SPELL_FLAMESTRIKE,	13,	90,	3021,	NULL ,{0,0}},
{    "heal",		SPELL_HEAL,		14,	90,	3021,	NULL ,{0,0}},
{    "sanctuary",	SPELL_SANCTUARY,	18,	90,	-2, "A charmed woman taken with a man who stole her shoe.",{0,0}},
{    "portal",		SPELL_PORTAL,		20,	90,	3021,	NULL ,{0,0}},
{    "full heal",	SPELL_FULL_HEAL,	25,	90,	-2, "Seek out a druid who is charmed by all. His healing skills are second only to a Physician in Sorpigal." ,{0,0}},
{    "protection from evil", SPELL_PROTECT_FROM_EVIL,	26,	90,	-2, "A short rotund priest with a face you could cut diamonds with can help you." ,{0,0}},
{    "protection from good", SPELL_PROTECT_FROM_GOOD,	26,	90,	3021, NULL ,{0,0}},
{    "dispel magic",	SPELL_DISPEL_MAGIC,	28,	90,	-2, "Find a mage at the junction of three dreams.",{0,0}},
{    "dispel minor",	SPELL_DISPEL_MINOR,	28,	90,	3021,	NULL ,{0,0}},
{    "power harm",	SPELL_POWER_HARM,	30,	90,	-2, "Find a shaman amongst the muck who has crooked teeth." ,{0,0}},
{    "heroes feast",	SPELL_HEROES_FEAST,	33,	90,	-2, "She has a story like no other, and the lives of her kinsmen are on display for you." ,{0,0}}, 
{    "animate dead",	SPELL_ANIMATE_DEAD,	35,	90,	3021,	NULL ,{0,0}},
{    "dismiss corpse",  SPELL_DISMISS_CORPSE,	35,	90,	3021,	NULL ,{0,0}},
{    "true sight",	SPELL_TRUE_SIGHT,	35,	90,	-2, "As his name suggests, this is a bright and wise person." ,{0,0}},
{    "group recall",	SPELL_GROUP_RECALL,	38,	90, -2, "An elven queen of sparkling radiance possesses the knowledge to save your people." ,{0,0}},
{    "group fly",	SPELL_GROUP_FLY,	39,	90,	-2, "It’s a bird! It’s an air traffic controller! No, but whatever it is, its elementary :)." ,{0,0}},
{    "resist cold",	SPELL_RESIST_COLD,	39,	90,	-2, "This cold worm lies beneath the surface of the earth in a very cold place." ,{0,0}}, 
{    "heal spray",	SPELL_HEAL_SPRAY,	40,	90,	3021,	NULL }, 
{    "resist fire",	SPELL_RESIST_FIRE,	41,	90,	3021,	NULL ,{0,0}},
{    "water breathing",	SPELL_WATER_BREATHING,	45,	90,	3021,	NULL ,{0,0}},
{    "group sanctuary",	SPELL_GROUP_SANC,	50,	90,	3021,	NULL ,{0,0}},
{    "\n",		0,			1,	0,	0, 	NULL ,{0,0}}
};

// mage 3020 guildmaster
struct class_skill_defines m_skills[] = { // mage skills 
{    "dodge",		SKILL_DODGE,		7,	5,	-2, 	"Hop like a bunny, quick like a bee, this location is so funny, you'll go wee wee wee.  Don't forget to bring our scythe however.",{0,0} },
{    "dual wield",      SKILL_DUAL_WIELD,       10,      25, -2, "With whirling blades and rigid honor, only these masters of weaponry can teach you the skill you need." ,{0,0}},
{    "focused repelance",SKILL_FOCUSED_REPELANCE, 25,	80,	3020,	NULL ,{0,0}},
{    "bludgeoning",	SKILL_BLUDGEON_WEAPONS,	30,	40,	3020,	NULL ,{0,0}},
{    "piercing",	SKILL_PIERCEING_WEAPONS,30,	40,	3020,	NULL ,{0,0}},
{    "magic missile",	SPELL_MAGIC_MISSILE,	1,	90,	3020,	NULL ,{0,0}},
{    "ventriloquate",	SPELL_VENTRILOQUATE,	1,	90,	3020,	NULL ,{0,0}},
{    "detect invisibility", SPELL_DETECT_INVISIBLE,	2,	90,	3020,	NULL ,{0,0}},
{    "detect magic",	SPELL_DETECT_MAGIC,	2,	90,	3020,	NULL ,{0,0}},
{    "chill touch",	SPELL_CHILL_TOUCH,	3,	90,	3020,	NULL ,{0,0}},
{    "continual light",	SPELL_CONT_LIGHT,	4,	90,	3020,	NULL ,{0,0}},
{    "invisibility",	SPELL_INVISIBLE,	4,	90,	3020,	NULL ,{0,0}},
{    "armor",		SPELL_ARMOR,		5,	90,	3020,	NULL ,{0,0}},
{    "burning hands",	SPELL_BURNING_HANDS,	5,	90,	3020,	NULL ,{0,0}},
{    "refresh",		SPELL_REFRESH,		5,	90,	3020,	NULL ,{0,0}},
{    "fear",		SPELL_FEAR,		6,	90,	3020,	NULL ,{0,0}},
{    "infravision",	SPELL_INFRAVISION,	6,	90,	3020,	NULL ,{0,0}},
{    "locate object",	SPELL_LOCATE_OBJECT,	6,	90,	-2, "A sanitation engineer works behind what people see, always knowing where everything is." ,{0,0}}, 
{    "fly",		SPELL_FLY,		7,	90,	3020,	NULL }, 
{    "strength",	SPELL_STRENGTH,		7,	90,	3020,	NULL ,{0,0}},
{    "weaken",		SPELL_WEAKEN,		7,	90,	-2, "A druid of old age and virulance is who you want. He will be in the company of other druids." ,{0,0}},
{    "blindness",	SPELL_BLINDNESS,	8,	90,	3020,	NULL ,{0,0}},
{    "identify",	SPELL_IDENTIFY,	9,	90,	3020,	NULL ,{0,0}},
{    "enchant weapon",	SPELL_ENCHANT_WEAPON,	10,	90,	3020,	NULL ,{0,0}},
{    "lightning bolt",	SPELL_LIGHTNING_BOLT,	10,	90,	3020,	NULL ,{0,0}},
{    "curse",		SPELL_CURSE,		12,	90,	3020,	NULL,{0,0} },
{    "energy drain",	SPELL_ENERGY_DRAIN,	13,	90,	3020,	NULL,{0,0} },
{    "shield",		SPELL_SHIELD,		13,	90,	3020,	NULL ,{0,0}},
{    "charm person",	SPELL_CHARM_PERSON,	14,	90,	3020,	NULL ,{0,0}},
{    "haste",		SPELL_HASTE,		14,	90,	3020,	NULL ,{0,0}},
{    "sleep",		SPELL_SLEEP,		15,	90,	3020,	NULL ,{0,0}},
{    "summon familiar", SPELL_SUMMON_FAMILIAR,	16,	90,	3020,	NULL ,{0,0}},
{    "dismiss familiar",SPELL_DISMISS_FAMILIAR, 16,	90,	3020,	NULL ,{0,0}},
{    "fireball",	SPELL_FIREBALL,		20,	90,	3020,	NULL ,{0,0}},
{    "teleport",	SPELL_TELEPORT,		21,	90,	-2, "A mage of much knowledge, he is wise in the ways of the world." ,{0,0}},
{    "dispel magic",	SPELL_DISPEL_MAGIC,	22,	90,	-2, "Find a mage at the junction of three dreams." ,{0,0}},
{    "dispel minor",	SPELL_DISPEL_MINOR,	22,	90,	3020,	NULL ,{0,0}},
{    "mass invisibility", SPELL_MASS_INVISIBILITY,	24,	90,	3020,	NULL ,{0,0}},
{    "wizard eye",	SPELL_WIZARD_EYE,	26,	90,	3020,	NULL ,{0,0}},
{    "debility",	SPELL_DEBILITY,		28,	90,	-2,	"Bring me a glass earring and I will teach this to you.",{0,0} },
{    "meteor swarm",	SPELL_METEOR_SWARM,	30,	90,	3020,	NULL ,{0,0}},
{    "stone skin",	SPELL_STONE_SKIN,	32,	90,	-2, "A woman strong in battle surrounded by heavy magical influences calls to you." ,{0,0}},
{    "group fly",	SPELL_GROUP_FLY,	34,	90, -2,	"It’s a bird! It’s an air traffic controller! No, but whatever it is, its elementary :)." ,{0,0}},
{    "life leech",	SPELL_LIFE_LEECH,	34,	90,	-2, "A statue with a cold heart could help you.  She is quite an artefact.",{0,0}},
{    "portal",		SPELL_PORTAL,		35,	90,	-2, "A mage of much knowledge, he is wise in the ways of the world." ,{0,0}},
{    "resist cold",	SPELL_RESIST_COLD,	36,	90,	-2, "This cold worm lies beneath the surface of the earth in a very cold place.",{0,0}},
{    "true sight",	SPELL_TRUE_SIGHT,	37,	90,	-2, "As his name suggests, this is a bright and wise person." ,{0,0}},
{    "resist fire",	SPELL_RESIST_FIRE,	39,	90,	3020,	NULL,{0,0} },
{    "firestorm",	SPELL_FIRESTORM,	40,	90,	3020,	NULL ,{0,0}},
{    "hellstream",	SPELL_HELLSTREAM,	40,	90,	3020,	NULL ,{0,0}},
{    "enchant armor",	SPELL_ENCHANT_ARMOR,	44,	90,	3020,	NULL ,{0,0}},
{    "paralyze",	SPELL_PARALYZE,		45,	90,	3020,	NULL ,{0,0}},
{    "word of recall",	SPELL_WORD_OF_RECALL,	45,	90,	3020,	NULL ,{0,0}},
{    "fireshield",	SPELL_FIRESHIELD,	48,	90,	3020,	NULL ,{0,0}},
//{    "create golem",	SPELL_CREATE_GOLEM,	50,	90,	3020,	NULL ,{0,0}},
//{    "release golem",	SPELL_RELEASE_GOLEM,	50,	90,	3020,	NULL ,{0,0}},
{    "solar gate",	SPELL_SOLAR_GATE,	50,	90,	-2,	"To acquire this powerful spell you must defeat the sun itself, freeing it from its prison." ,{0,0}}, 
{    "\n",		0,			1,	0,	0, 	NULL,{0,0}} 
};

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
   63,0,0,0,0,18,0,0},
  {"Dwarf",     "Dwarves",
   63,0,0,0,768,12,0,0},
  {"Hobbit",    "Hobbits",
   63,0,0,0,0,12,0,0},
  {"Pixie",     "Pixies",
   63,0,0,0,0,18,0,0},
  {"Giant",     "Giants",
   63,0,0,0,128,1,0,0},
  {"Gnome",     "Gnomes",
   63,0,0,0,0,70,0,0},
  {"Orc",       "Orcs",
   63,0,0,0,5004,128,0,0},
  {"Troll",     "Trolls",
   63,0,0,16,9,0,0,0},
  {"Goblin",    "Goblins",
   63,0,0,0,17,0,0,0},
  {"Reptile",   "Reptiles",
   59,0,0,0,0,0,0,0},
  {"Dragon",    "Dragons",
   91,16,0,0,7,0,0,0},
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
   63,0,0,0,0,4194304,0,0},
  {"Undead",    "Undead",
   63,129,0,0,0,0,0,0},
  {"Ghost",     "Spirits",
   63,723,0,0,15,0,0,0},
  {"Golem",     "Golems",
   63,386,0,0,2,0,0,0},
  {"Elemental", "Elementals",
   63, 384,  0,  0,   0,67108864,0,0},
  {"Planar",    "Planar",
   63,   0,  0,  0,   0, 7,  0,  0},
  {"Demon",     "Demons",
   127, 528,  0,  0,  15, 0,  0,  0},
  {"Yrnali",    "Yrnali",
   63, 10240, 0, 0, 0, 0, 0, 0}
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

