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
/* $Id: const.cpp,v 1.14 2002/08/01 19:14:33 pirahna Exp $ */
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
  "!Camoflague!",
  "!FarSight!",
  "!FreeFloat!",
  "!Insomnia!",
  "!ShadowSlip!",
  "!ResistEnergy!",  // 120
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
    1   /* Air        */
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
 L(30 K),    L(60 K),    L(120 K),   L(240 K),   L(350 K),  // level 10
 L(450 K),   L(600 K),   L(850 K),   L(1.2 M),   L(2.5 M),  // level 15
 L(3.6 M),   L(4.8 M),   L(6 M),     L(7.1 M),   L(8.4 M),  // level 20
 L(9.4 M),   L(10.4 M),  L(11.7 M),  L(13.3 M),  L(14.5 M), // level 25
 L(15.7 M),  L(17.3 M),  L(18.5 M),  L(20.3 M),  L(22.5 M), // level 30
 L(24.5 M),  L(26.5 M),  L(29.5 M),  L(32.5 M),  L(35.5 M), // level 35
 L(37.5 M),  L(40.5 M),  L(43.5 M),  L(46.5 M),  L(50 M),   // level 40 
 L(55 M),    L(70 M),    L(80 M),    L(90 M),    L(100 M),  // level 45
 L(120 M),   L(140 M),   L(160 M),   L(180 M),   L(200 M),  // level50
 L(125 M),   L(2 M),     L(3 M),     L(4 M),     L(5 M),    // level 55
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
    "CAMOUFLAGUE",
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
    "CAMOUFLAGUE",
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
//    byte   maximum;           // maximum value PC can train it to (1-100)
//    long   trainer;           // what mob trains them (only one currently) 0 = any
//    char *                    // hint to who trains it

struct class_skill_defines g_skills[] = {
{    "scan",		SKILL_SCAN,		1,	90,	10011,	NULL },
{    "consider",	SKILL_CONSIDER,		1,	90,	10011,	NULL },
{    "switch",		SKILL_SWITCH,		1,	90,	10011,	NULL },
{    "\n",		0,			1,	0,	0,	NULL }
};

struct class_skill_defines w_skills[] = {
{    "kick",		SKILL_KICK,		1,	80,	3023,	"You have to go see Bjorn the warrior guildmaster." },
{    "bash",		SKILL_BASH,		1,	75,	14517, 	"You must find a local warrior.  He said something about a field filled with bees." },
{    "redirect",	SKILL_REDIRECT,		1,	75,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "rescue",		SKILL_RESCUE,		5,	75,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "disarm",  	SKILL_DISARM,		5,	50,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "double",		SKILL_SECOND_ATTACK,	8,	80,	702, 	"I cannot teach you that.  You must journey north to see the son of the Delcim king.  He will train you." },
{    "headbutt",	SKILL_SHOCK,		10,	75,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "dodge",		SKILL_DODGE,		12,	40,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "dual wield",      SKILL_DUAL_WIELD,       15,     90,     3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "retreat",		SKILL_RETREAT,		17,	75,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "frenzy",		SKILL_FRENZY,		18,	50,	16006,	"There is a highlander chief.  The chief of mountain trail guard.  You must locate him to learn this skill." },
{    "parry",		SKILL_PARRY,		20,	50,	18015, 	"You must see the constable of Newtonia to learn parry." },
{    "blindfighting",   SKILL_BLINDFIGHTING,    21,     80,     3023,   NULL },
{    "triple",		SKILL_THIRD_ATTACK,	23,	75,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "hitall",		SKILL_HITALL,		25,	75,	3023, 	"Ask the son of the Sarina King to please teach you that." },
{    "archery",		SKILL_ARCHERY,		27,	60,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "deathstroke",	SKILL_DEATHSTROKE,	30,	75,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "shield block",    SKILL_SHIELDBLOCK,      33,     50,     725, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you." },
{    "stun",		SKILL_STUN,		35,	50,	6902, 	"Go to see any master shaolin monk." },
{    "guard",		SKILL_GUARD,		37,	75,	3023,	NULL },
{    "riposte",		SKILL_RIPOSTE,		40,	75,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "skewer",		SKILL_SKEWER,		45,	75,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "blade shield",	SKILL_BLADESHIELD,	47,	75,	3023, 	"You have to go see Bjorn the warrior guildmaster." },
{    "\n",		0,			1,	0, 	0, 	NULL }
};

struct class_skill_defines t_skills[] = {
{    "sneak",		SKILL_SNEAK,		1,	75,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "shield block",    SKILL_SHIELDBLOCK,      1,      40,     725, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you." },
{    "backstab",	SKILL_BACKSTAB,		2,	90,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "hide",		SKILL_HIDE,		5, 	75,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "pick",		SKILL_PICK_LOCK,	7,	75,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "slip",  		SKILL_SLIP,		10,	80,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "palm",		SKILL_PALM,		10,	80,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "dual wield",      SKILL_DUAL_WIELD,       10,	90,     3022, 	"You have to go see Skiv the thief guildmaster." },
{    "pocket",		SKILL_POCKET,		12,	80,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "dodge",		SKILL_DODGE,		15,	90,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "stalk",		SKILL_STALK,		15,	80,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "trip",		SKILL_TRIP,		17,	75,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "blindfighting",   SKILL_BLINDFIGHTING,    19,     60,     3022,   NULL },
{    "steal",		SKILL_STEAL,		20,	80,	-2, 	"There's a little thief around town named Caijin.  Annoying little bastard.  Ask him to please teach you." },
{    "disarm",		SKILL_DISARM,		23,	60,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "vitalstrike",     SKILL_VITAL_STRIKE,	25,	80,	3022,	NULL },
{    "circle",		SKILL_CIRCLE,		30,	75,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "dualbackstab",	SKILL_DUAL_BACKSTAB,	40,	80,	3022, 	"You have to go see Skiv the thief guildmaster." },
{    "\n",		0,			1,	0,	0,	NULL}
};
    
struct class_skill_defines a_skills[] = {
{    "harmtouch",	SKILL_HARM_TOUCH,	1,	90,	10005, 	"You have to go see Skarl the anti-paladin guildmaster." },
{    "sneak",		SKILL_SNEAK,		2,	60,	10005,	"You have to go see Skarl the anti-paladin guildmaster." },
{    "kick",		SKILL_KICK,		3,	75,	10005, 	"You have to go see Skarl the anti-paladin guildmaster." },
{    "shield block",    SKILL_SHIELDBLOCK,      5,      50,     725, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you." },
{    "dual wield",      SKILL_DUAL_WIELD,       10,	60,     10005, 	"You have to go see Skarl the anti-paladin guildmaster." },
{    "hide", 		SKILL_HIDE,		15,	60,	10005, 	"You have to go see Skarl the anti-paladin guildmaster." },
{    "backstab",	SKILL_BACKSTAB,		17,	80,	10005, 	"You have to go see Skarl the anti-paladin guildmaster." },
{    "double",		SKILL_SECOND_ATTACK,	20,	75,	702, 	"Travel to Delcim and seek the prince.  He will teach you that." },
{    "dodge",   	SKILL_DODGE,		23,	75,	10005, 	"You have to go see Skarl the anti-paladin guildmaster." },
{    "trip",  		SKILL_TRIP,		25,	75,	10005, 	"You have to go see Skarl the anti-paladin guildmaster." },
{    "blindfighting",   SKILL_BLINDFIGHTING,    30,     50,     10005,   NULL },

{    "detect good",	SPELL_DETECT_GOOD,	11,	90,	10005,	NULL },
{    "detect invisibility",SPELL_DETECT_INVISIBLE,	12,	90,	10005,	NULL },
{    "curse",		SPELL_CURSE,		13,	90,	10005,	NULL },
{    "vampiric touch",	SPELL_VAMPIRIC_TOUCH,	15,	90,	10005,	NULL },
{    "animate dead",	SPELL_ANIMATE_DEAD,	15,	90,	10005,	NULL },
{    "invisibility",	SPELL_INVISIBLE,	16,	90,	10005,	NULL },
{    "globe of darkness",SPELL_GLOBE_OF_DARKNESS, 17,	90,	10005,	NULL },
{    "lightning bolt",	SPELL_LIGHTNING_BOLT,	18,	90,	10005,	NULL },
{    "poison",		SPELL_POISON,		21,	90,	10005,	NULL },
{    "weaken",		SPELL_WEAKEN,		23,	90,	10005,	NULL },
{    "enchant weapon",	SPELL_ENCHANT_WEAPON,	24,	90,	10005,	NULL },
{    "fireball",	SPELL_FIREBALL,		28,	90,	10005,	NULL },
{    "beacon",		SPELL_BEACON,		33,	90,	10005,	NULL },
{    "dispel good",	SPELL_DISPEL_GOOD,	35,	90,	10005,	NULL },
{    "firestorm",	SPELL_FIRESTORM,	38,	90,	10005,	NULL },
{    "acid shield",	SPELL_ACID_SHIELD,	40,	90,	10005,	NULL },
{    "blindness",	SPELL_BLINDNESS,	42,	90,	10005,	NULL },
{    "stone skin",	SPELL_STONE_SKIN,	46,	90,	10005,	NULL },
{    "acid blast",	SPELL_ACID_BLAST,	48,	90,	10005,	NULL },
{    "\n",		0,			1,	0,	0, 	NULL}
};
    
struct class_skill_defines p_skills[] = {
{    "layhands",	SKILL_LAY_HANDS,	1,	90,	10006, 	"You have to go see the paladin guildmaster." },
{    "bash",		SKILL_BASH,		2,	75,	14517, 	"You must find a local warrior.  He said something about a field filled with bees." },
{    "shield block",    SKILL_SHIELDBLOCK,      3,      50,     725, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you." },
{    "kick",		SKILL_KICK,		5,	80,	10006, 	"You have to go see the paladin guildmaster." },
{    "rescue",		SKILL_RESCUE,		7,	80,	10006, 	"You have to go see the paladin guildmaster." },
{    "dual wield",      SKILL_DUAL_WIELD,       10,     60,     10006, 	"You have to go see the paladin guildmaster." },
{    "dodge",		SKILL_DODGE,		13,	20,	10006, 	"You have to go see the paladin guildmaster." },
{    "double",		SKILL_SECOND_ATTACK,	20,	75,	702, 	"Seek the Delcim Prince my child.  He shall train you." },
{    "parry",		SKILL_PARRY,		25,	50,	18015, 	"You have to go see the constable of Newtonia to learn parry." },
{    "triple",		SKILL_THIRD_ATTACK,	30,	70,	10006, 	"You have to go see the paladin guildmaster." },

{    "bless",		SPELL_BLESS,		10,	90,	10006,	NULL },
{    "cure light",	SPELL_CURE_LIGHT,	10,	90,	10006,	NULL },
{    "earthquake",	SPELL_EARTHQUAKE,	11,	90,	-1,	"You need to go see Minare.  He was last seen in the caves north east of town." }, 
{    "create food",	SPELL_CREATE_FOOD,	12,	90,	10006,	NULL },
{    "create water",	SPELL_CREATE_WATER,	13,	90,	10006,	NULL },
{    "detect evil",	SPELL_DETECT_EVIL,	14,	90,	10006,	NULL },
{    "cure serious",	SPELL_CURE_SERIOUS,	15,	90,	10006,	NULL },
{    "detect invisibility", SPELL_DETECT_INVISIBLE,	16,	90,	10006,	NULL },
{    "detect poison",	SPELL_DETECT_POISON,	18,	90,	10006,	NULL },
{    "remove poison",	SPELL_REMOVE_POISON,	19,	90,	10006,	NULL },
{    "cure critic",	SPELL_CURE_CRITIC,	25,	90,	10006,	NULL },
{    "sense life",	SPELL_SENSE_LIFE,	26,	90,	10006,	NULL },
{    "strength",	SPELL_STRENGTH,		27,	90,	10006,	NULL },
{    "heal",		SPELL_HEAL,		30,	90,	10006,	NULL },
{    "protection from evil", SPELL_PROTECT_FROM_EVIL,	33,	90,	10006,	NULL },
{    "harm",		SPELL_HARM,		35,	90,	10006,	NULL },
{    "dispel evil",	SPELL_DISPEL_EVIL,	40,	90,	10006,	NULL },
{    "sanctuary",	SPELL_SANCTUARY,	45,	90,	10006,	NULL },
{    "power harm",	SPELL_POWER_HARM,	48,	90,	10006,	NULL },
{    "\n",		0,			2,	0,	0,	NULL}
};
    
    
struct class_skill_defines b_skills[] = {
{    "dual wield",      SKILL_DUAL_WIELD,       1,      75,     10007, 	"You have to go see Mooral the barbarian guildmaster." },
{    "kick",  		SKILL_KICK,		2,	75,	10007, 	"You have to go see Mooral the barbarian guildmaster." },
{    "bash",		SKILL_BASH,		3,	75,	14517, 	"You must find a local warrior.  He said something about a field filled with bees." },
{    "parry",		SKILL_PARRY,		5,	40,	18015, 	"You must see the constable of Newtonia to learn parry." },
{    "dodge",		SKILL_DODGE,		7,	10,	10007, 	"You have to go see Mooral the barbarian guildmaster." },
{    "shield block",    SKILL_SHIELDBLOCK,      8,      40,     725, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you." },
{    "double",		SKILL_SECOND_ATTACK,	10,	80,	702, 	"Seek the Prince of Delcim for your training in that." },
{    "blood fury",	SKILL_BLOOD_FURY,	13,	80,	10007, 	"You have to go see Mooral the barbarian guildmaster." },
{    "frenzy",		SKILL_FRENZY,		18,	50,	16006,	"There is a highlander chief.  The chief of mountain trail guard.  You must locate him to learn this skill." },
{    "rage",		SKILL_RAGE,		20,	80,	10007, 	"You have to go see Mooral the barbarian guildmaster." },
{    "triple",		SKILL_THIRD_ATTACK,	25,	70,	10007, 	"You have to go see Mooral the barbarian guildmaster." },
{    "battlecry",	SKILL_BATTLECRY,	27,	75,	10007, 	"You have to go see Mooral the barbarian guildmaster." },
{    "blindfighting",   SKILL_BLINDFIGHTING,    28,     50,     10007,   NULL },
{    "headbutt",	SKILL_SHOCK,		30,	75,	10007, 	"You have to go see Mooral the barbarian guildmaster." },
{    "berserk",		SKILL_BERSERK,		40,	90,	10007, 	"You have to go see Mooral the barbarian guildmaster." },
{    "\n",		0,			1,	0,	0, 	NULL}
};
    
struct class_skill_defines k_skills[] = {
{    "kick",		SKILL_KICK,		1,	90,	10008, 	"You have to go see Grox the monk guildmaster."},
{    "dodge",		SKILL_DODGE,		2,	75,	10008,  "You have to go see Grox the monk guildmaster." },
{    "redirect",	SKILL_REDIRECT,		3,	75,	10008, 	"You have to go see Grox the monk guildmaster."},
{    "trip",		SKILL_TRIP,		5,	75,	10008, 	"You have to go see Grox the monk guildmaster."},
{    "shield block",    SKILL_SHIELDBLOCK,      7,      60,     725, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you."},
{    "rescue",  	SKILL_RESCUE,		10,	70,	10008, 	"You have to go see Grox the monk guildmaster."},
{    "eagleclaw",	SKILL_EAGLE_CLAW,	17,	75,	10008, 	"You have to go see Grox the monk guildmaster."},
{    "dual wield",      SKILL_DUAL_WIELD,       20,     60,     10008, 	NULL},
{    "stun",		SKILL_STUN,		30,	60,	6902, 	"You must see a master shaolin monk."},
{    "quiver",		SKILL_QUIVERING_PALM,	40,	75,	10008, 	"You have to go see Grox the monk guildmaster."},
{    "purify",		KI_PURIFY+KI_OFFSET,	5,	80,	10008, 	NULL},
{    "punch",		KI_PUNCH+KI_OFFSET,	10,	80,	10008, 	NULL},
{    "sense",		KI_SENSE+KI_OFFSET,	20,	80,	10008, 	NULL},
{    "stance",		KI_STANCE+KI_OFFSET,	24,	80,	10008, 	NULL},
{    "speed",		KI_SPEED+KI_OFFSET,	27,	80,	10008, 	NULL},
{    "storm",		KI_STORM+KI_OFFSET,	35,	80,	10008, 	NULL},
{    "disrupt",		KI_DISRUPT+KI_OFFSET,	41,	80,	10008, 	NULL},
{    "blast",		KI_BLAST+KI_OFFSET,	45,	80,	10008, 	NULL},
{    "\n",		0,			1,	0,	0, 	NULL}
};
    
struct class_skill_defines r_skills[] = {
{    "hide",		SKILL_HIDE,		1,	80,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "redirect",	SKILL_REDIRECT,		2,	75,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "tame",		SKILL_TAME,		3,	75,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "dodge",		SKILL_DODGE,		4,	70,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "dual wield",	SKILL_DUAL_WIELD,       5,      80,     10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "trip",		SKILL_TRIP,		7,	75,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "shield block",    SKILL_SHIELDBLOCK,      9,      40,     725, 	"To learn this skill you must seek its master, the God of War in his arena.  Ask him to please teach you." },
{    "archery",		SKILL_ARCHERY,		12,	80,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "ambush",		SKILL_AMBUSH,		13,	75,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "double",		SKILL_SECOND_ATTACK,	14,	80,	702, 	"See the King of Delcim's son." },
{    "disarm",		SKILL_DISARM,		15,	50,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "forage",		SKILL_FORAGE,		17,	75,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "track",		SKILL_TRACK,		20,	80,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "blindfighting",   SKILL_BLINDFIGHTING,    21,     60,     10013,   NULL },
{    "rescue",		SKILL_RESCUE,		23,	75,	10013, 	"You have to go see Hawk the ranger guildmaster." },
{    "stun",  		SKILL_STUN,		30,	60,	6902, 	"You must see a master shaolin monk." },

{    "bee sting",	SPELL_BEE_STING,	1,	90,	10013,	NULL },
{    "eyes of the owl",	SPELL_EYES_OF_THE_OWL,	5,	90,	10013,	NULL },
{    "bee swarm",	SPELL_BEE_SWARM,	10,	90,	10013,	NULL },
{    "entangle",	SPELL_ENTANGLE,		15,	90,	10013,	NULL },
{    "barkskin",	SPELL_BARKSKIN,		25,	90,	10013,	NULL },
{    "feline agility",	SPELL_FELINE_AGILITY,	30,	90,	10013,	NULL },
{    "herb lore",	SPELL_HERB_LORE,	35,	90,	10013,	NULL },
{    "forest meld",	SPELL_FOREST_MELD,	37,	90,	10013,	NULL },
{    "call follower",	SPELL_CALL_FOLLOWER,	40,	90,	10013,	NULL },
{    "creeping death",	SPELL_CREEPING_DEATH,	45,	90,	10013,	NULL },
{      "\n",		0,			1,	0,	0, 	NULL}
};
    
struct class_skill_defines d_skills[] = { // bard skills
{      "hide",			SKILL_HIDE,			2,	75,	0, 	NULL},
{      "dodge",			SKILL_DODGE,			5,	85,	0, 	NULL},
{      "dual wield",    	SKILL_DUAL_WIELD,       	10,     75,     0, 	NULL},
{      "double",		SKILL_SECOND_ATTACK,		17,	85,	702, 	"You must travel to the Prince of Delcim to recieve training."},
{      "listsongs",		SKILL_SONG_LIST_SONGS,		1,	100,	0, 	NULL},
{      "whistle sharp",		SKILL_SONG_WHISTLE_SHARP, 	1,	90,	0, 	NULL},
{      "stop",			SKILL_SONG_STOP,		7,	90,	0, 	NULL},
{      "traveling march",	SKILL_SONG_TRAVELING_MARCH,	7,	90,	0, 	NULL},
{      "bountiful sonnet",	SKILL_SONG_BOUNT_SONNET,	9,	90,	0, 	NULL},
{      "insane chant",		SKILL_SONG_INSANE_CHANT,	10,	90,	0, 	NULL},
{      "glitter dust",		SKILL_SONG_GLITTER_DUST,	10,	90,	0, 	NULL},
{      "synchronous chord",	SKILL_SONG_SYNC_CHORD,		13,	90,	0, 	NULL},
{      "healing melody",	SKILL_SONG_HEALING_MELODY,	15,	90,	0, 	NULL},
{      "sticky lullaby",	SKILL_SONG_STICKY_LULL,		17,	90,	0, 	NULL},
{      "revealing stacato",	SKILL_SONG_REVEAL_STACATO,	20,	90,	0, 	NULL},
{      "flight of the bumblebee", SKILL_SONG_FLIGHT_OF_BEE,	20,	90,	0, 	NULL},
{      "jig of alacrity",	SKILL_SONG_JIG_OF_ALACRITY,	23,	90,	0, 	NULL},
{      "note of knowledge",	SKILL_SONG_NOTE_OF_KNOWLEDGE,	25,	90,	0, 	NULL},
{      "terrible clef",		SKILL_SONG_TERRIBLE_CLEF,	27,	90,	0, 	NULL},
{      "soothing rememberance",	SKILL_SONG_SOOTHING_REMEM,	31,	90,	0, 	NULL},
{      "forgetful rhythm",	SKILL_SONG_FORGETFUL_RHYTHM,	35,	90,	0, 	NULL},
{      "searching song",	SKILL_SONG_SEARCHING_SONG,	35,	90,	0, 	NULL},
{      "vigilant siren",	SKILL_SONG_VIGILANT_SIREN,	43,	90,	0, 	NULL},
{      "astral chanty",		SKILL_SONG_ASTRAL_CHANTY,	40,	90,	0, 	NULL},
{      "disarming limerick", 	SKILL_SONG_DISARMING_LIMERICK,	43,	90,	0, 	NULL},
{      "shattering resonance",	SKILL_SONG_SHATTERING_RESO,	45,	90,	0, 	NULL},
{      "\n",			0,				1,	0,	0, 	NULL}
};

struct class_skill_defines u_skills[] = { // druid skills
{    "dodge",		SKILL_DODGE,		7,	5,	0, 	NULL},
{    "dual wield",      SKILL_DUAL_WIELD,       11,      40,     0, 	NULL},

{    "eyes of the owl",	SPELL_EYES_OF_THE_OWL,	1,	90,	0,	NULL },
{    "cure light",	SPELL_CURE_LIGHT,	3,	90,	0,	NULL },
{    "cure serious",	SPELL_CURE_SERIOUS,	6,	90,	0,	NULL },
{    "cure critic",	SPELL_CURE_CRITIC,	10,	90,	0,	NULL },
{    "control weather",	SPELL_CONTROL_WEATHER,	13,	90,	0,	NULL },
{    "sun ray",		SPELL_SUN_RAY,		14,	90,	0,	NULL },
{    "sense life",	SPELL_SENSE_LIFE,	15,	90,	0,	NULL },
{    "water breathing",	SPELL_WATER_BREATHING,	17,	90,	0,	NULL },
{    "stone shield",	SPELL_STONE_SHIELD,	20,	90,	0,	NULL },
{    "drown",		SPELL_DROWN,		22,	90,	0,	NULL },
{    "lightning shield",SPELL_LIGHTNING_SHIELD,	23,	90,	0,	NULL },
{    "power heal",	SPELL_POWER_HEAL,	26,	90,	0,	NULL },
{    "camouflague",	SPELL_CAMOUFLAGUE,	28,	90,	0,	NULL },
{    "barkskin",	SPELL_BARKSKIN,		35,	90,	0,	NULL },
{    "iron roots",	SPELL_IRON_ROOTS,	36,	90,	0,	NULL },
{    "resist energy",	SPELL_RESIST_ENERGY,	39,	90,	0,	NULL },
{    "summon",		SPELL_SUMMON,		40,	90,	0,	NULL },
{    "greater stone shield",SPELL_GREATER_STONE_SHIELD,	42,	90,	0,	NULL },
{    "stone skin",	SPELL_STONE_SKIN,	45,	90,	0,	NULL },
{    "\n",		0,			1,	0,	0, 	NULL}
};

struct class_skill_defines c_skills[] = { // cleric skills
{    "dodge",		SKILL_DODGE,		7,	5,	3021, 	"You have to go see Drawan the cleric guildmaster." },
{    "dual wield",      SKILL_DUAL_WIELD,       10,      40,     3021, 	"You have to go see Drawan the cleric guildmaster." },

{    "armor",		SPELL_ARMOR,		1,	90,	3021,	NULL }, 
{    "cause light",	SPELL_CAUSE_LIGHT,	1,	90,	3021,	NULL }, 
{    "cure light",	SPELL_CURE_LIGHT,	1,	90,	3021,	NULL }, 
{    "create water",	SPELL_CREATE_WATER,	2,	90,	3021,	NULL }, 
{    "detect poison",	SPELL_DETECT_POISON,	2,	90,	3021,	NULL }, 
{    "continual light",	SPELL_CONT_LIGHT,	2,	90,	3021,	NULL }, 
{    "create food",	SPELL_CREATE_FOOD,	3,	90,	3021,	NULL }, 
{    "detect magic",	SPELL_DETECT_MAGIC,	3,	90,	3021,	NULL }, 
{    "refresh",		SPELL_REFRESH,		3,	90,	3021,	NULL }, 
{    "cure blind",	SPELL_CURE_BLIND,	4,	90,	3021,	NULL }, 
{    "detect evil",	SPELL_DETECT_EVIL,	4,	90,	3021,	NULL }, 
{    "detect good",	SPELL_DETECT_GOOD,	4,	90,	3021,	NULL }, 
{    "bless",		SPELL_BLESS,		5,	90,	3021,	NULL }, 
{    "cause serious",	SPELL_CAUSE_SERIOUS,	5,	90,	3021,	NULL }, 
{    "cure serious",	SPELL_CURE_SERIOUS,	5,	90,	3021,	NULL }, 
{    "detect invisibility", SPELL_DETECT_INVISIBLE,	5,	90,	3021,	NULL }, 
{    "know alignment",	SPELL_KNOW_ALIGNMENT,	5,	90,	3021,	NULL }, 
{    "blindness",	SPELL_BLINDNESS,	6,	90,	3021,	NULL }, 
{    "earthquake",	SPELL_EARTHQUAKE,	7,	90,	-1,	"You need to go see Minare.  He was last seen in the caves north east of town." }, 
{    "poison",		SPELL_POISON,		8,	90,	3021,	NULL }, 
{    "cause critical",	SPELL_CAUSE_CRITICAL,	9,	90,	3021,	NULL }, 
{    "cause critic",	SPELL_CURE_CRITIC,	9,	90,	3021,	NULL }, 
{    "remove poison",	SPELL_REMOVE_POISON,	9,	90,	3021,	NULL }, 
{    "dispel evil",	SPELL_DISPEL_EVIL,	10,	90,	3021,	NULL }, 
{    "dispel good",	SPELL_DISPEL_GOOD,	10,	90,	3021,	NULL }, 
{    "locate object",	SPELL_LOCATE_OBJECT,	10,	90,	3021,	NULL }, 
{    "sense life",	SPELL_SENSE_LIFE,	10,	90,	3021,	NULL }, 
{    "word of recall",	SPELL_WORD_OF_RECALL,	11,	90,	3021,	NULL }, 
{    "call lightning",	SPELL_CALL_LIGHTNING,	12,	90,	3021,	NULL }, 
{    "harm",		SPELL_HARM,		12,	90,	3021,	NULL }, 
{    "remove curse",	SPELL_REMOVE_CURSE,	12,	90,	3021,	NULL }, 
{    "remove paralysis",SPELL_REMOVE_PARALYSIS,	12,	90,	3021,	NULL }, 
{    "control weather",	SPELL_CONTROL_WEATHER,	13,	90,	3021,	NULL }, 
{    "flamestrike",	SPELL_FLAMESTRIKE,	13,	90,	3021,	NULL }, 
{    "heal",		SPELL_HEAL,		14,	90,	3021,	NULL }, 
{    "sanctuary",	SPELL_SANCTUARY,	18,	90,	3021,	NULL }, 
{    "portal",		SPELL_PORTAL,		20,	90,	3021,	NULL }, 
{    "full heal",	SPELL_FULL_HEAL,	25,	90,	3021,	NULL }, 
{    "protection from evil", SPELL_PROTECT_FROM_EVIL,	26,	90,	3021,	NULL }, 
{    "dispel magic",	SPELL_DISPEL_MAGIC,	28,	90,	3021,	NULL }, 
{    "dispel minor",	SPELL_DISPEL_MINOR,	28,	90,	3021,	NULL }, 
{    "power harm",	SPELL_POWER_HARM,	30,	90,	3021,	NULL }, 
{    "heroes feast",	SPELL_HEROES_FEAST,	33,	90,	3021,	NULL }, 
{    "animate dead",	SPELL_ANIMATE_DEAD,	35,	90,	3021,	NULL }, 
{    "true sight",	SPELL_TRUE_SIGHT,	35,	90,	3021,	NULL }, 
{    "group recall",	SPELL_GROUP_RECALL,	38,	90,	3021,	NULL }, 
{    "group fly",	SPELL_GROUP_FLY,	39,	90,	3021,	NULL }, 
{    "resist cold",	SPELL_RESIST_COLD,	39,	90,	3021,	NULL }, 
{    "heal spray",	SPELL_HEAL_SPRAY,	40,	90,	3021,	NULL }, 
{    "resist fire",	SPELL_RESIST_FIRE,	41,	90,	3021,	NULL }, 
{    "water breathing",	SPELL_WATER_BREATHING,	45,	90,	3021,	NULL },
{    "group sanctuary",	SPELL_GROUP_SANC,	50,	90,	3021,	NULL },  
{    "\n",		0,			1,	0,	0, 	NULL }
};

struct class_skill_defines m_skills[] = { // mage skills
{    "dodge",		SKILL_DODGE,		7,	5,	3020, 	"You have to go see Alianna the mage guildmaster." },
{    "dual wield",      SKILL_DUAL_WIELD,       10,      25,     3020, 	"You have to go see Alianna the mage guildmaster." },
{    "focused repelance",SKILL_FOCUSED_REPELANCE, 25,	80,	3020,	NULL },

{    "magic missile",	SPELL_MAGIC_MISSILE,	1,	90,	3020,	NULL }, 
{    "ventriloquate",	SPELL_VENTRILOQUATE,	1,	90,	3020,	NULL },
{    "detect invisibility", SPELL_DETECT_INVISIBLE,	2,	90,	3020,	NULL },
{    "detect magic",	SPELL_DETECT_MAGIC,	2,	90,	3020,	NULL },
{    "chill touch",	SPELL_CHILL_TOUCH,	3,	90,	3020,	NULL },
{    "continual light",	SPELL_CONT_LIGHT,	4,	90,	3020,	NULL },
{    "invisibility",	SPELL_INVISIBLE,	4,	90,	3020,	NULL },
{    "armor",		SPELL_ARMOR,		5,	90,	3020,	NULL },
{    "burning hands",	SPELL_BURNING_HANDS,	5,	90,	3020,	NULL },
{    "refresh",		SPELL_REFRESH,		5,	90,	3020,	NULL },
{    "fear",		SPELL_FEAR,		6,	90,	3020,	NULL },
{    "infravision",	SPELL_INFRAVISION,	6,	90,	3020,	NULL },
{    "locate object",	SPELL_LOCATE_OBJECT,	6,	90,	3020,	NULL },
{    "fly",		SPELL_FLY,		7,	90,	3020,	NULL },
{    "strength",	SPELL_STRENGTH,		7,	90,	3020,	NULL },
{    "weaken",		SPELL_WEAKEN,		7,	90,	3020,	NULL },
{    "blindness",	SPELL_BLINDNESS,	8,	90,	3020,	NULL },
{    "enchant weapon",	SPELL_ENCHANT_WEAPON,	10,	90,	3020,	NULL },
{    "lightning bolt",	SPELL_LIGHTNING_BOLT,	10,	90,	3020,	NULL },
{    "curse",		SPELL_CURSE,		12,	90,	3020,	NULL },
{    "energy drain",	SPELL_ENERGY_DRAIN,	13,	90,	3020,	NULL },
{    "shield",		SPELL_SHIELD,		13,	90,	3020,	NULL },
{    "charm person",	SPELL_CHARM_PERSON,	14,	90,	3020,	NULL },
{    "haste",		SPELL_HASTE,		14,	90,	3020,	NULL },
{    "sleep",		SPELL_SLEEP,		14,	90,	3020,	NULL },
{    "fireball",	SPELL_FIREBALL,		20,	90,	3020,	NULL },
{    "teleport",	SPELL_TELEPORT,		21,	90,	3020,	NULL },
{    "dispel magic",	SPELL_DISPEL_MAGIC,	22,	90,	3020,	NULL },
{    "dispel minor",	SPELL_DISPEL_MINOR,	22,	90,	3020,	NULL },
{    "mass invisibility", SPELL_MASS_INVISIBILITY,	24,	90,	3020,	NULL },
{    "wizard eye",	SPELL_WIZARD_EYE,	26,	90,	3020,	NULL },
{    "haste other",	SPELL_HASTE_OTHER,	28,	90,	3020,	NULL },
{    "meteor swarm",	SPELL_METEOR_SWARM,	30,	90,	3020,	NULL },
{    "stone skin",	SPELL_STONE_SKIN,	32,	90,	3020,	NULL },
{    "group fly",	SPELL_GROUP_FLY,	34,	90,	3020,	NULL },
{    "life leech",	SPELL_LIFE_LEECH,	34,	90,	3020,	NULL },
{    "portal",		SPELL_PORTAL,		35,	90,	3020,	NULL },
{    "resist cold",	SPELL_RESIST_COLD,	36,	90,	3020,	NULL },
{    "true sight",	SPELL_TRUE_SIGHT,	37,	90,	3020,	NULL },
{    "resist fire",	SPELL_RESIST_FIRE,	39,	90,	3020,	NULL },
{    "firestorm",	SPELL_FIRESTORM,	40,	90,	3020,	NULL },
{    "hellstream",	SPELL_HELLSTREAM,	40,	90,	3020,	NULL },
{    "enchant armor",	SPELL_ENCHANT_ARMOR,	44,	90,	3020,	NULL },
{    "paralyze",	SPELL_PARALYZE,		45,	90,	3020,	NULL },
{    "word of recall",	SPELL_WORD_OF_RECALL,	45,	90,	3020,	NULL },
{    "fireshield",	SPELL_FIRESHIELD,	48,	90,	3020,	NULL },
{    "create golem",	SPELL_CREATE_GOLEM,	50,	90,	3020,	NULL },
{    "release golem",	SPELL_RELEASE_GOLEM,	50,	90,	3020,	NULL },
{    "solar gate",	SPELL_SOLAR_GATE,	50,	90,	3020,	NULL },
{    "\n",		0,			1,	0,	0, 	NULL}
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
    "UNUSED",
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
    2,   /* 1 */
    2,
    2,
    2,
    3,   /* 5 */
    3,
    3,
    3,
    3,
    4,    /* 10 */
    4,
    4,
    4,
    4,
    4,     /* 15 */
    4,   
    4,
    5,
    5,
    5,      /* 20 */
    5,
    5,
    5,
    6,
    6,     /* 25 */
    6,
    6,
    6,
    6,
    7,     /* 30 */
    7,   
    7,
    7,
    7,
    7,      /* 35 */
    8,
    8,
    8,
    8,
    8,      /* 40 */
    8,
    9,
    9,
    9,
    9,    /* 45 */
    9,
    9,
   10,
   10,
   11,    /* 50 */
   13,
   14,
   15,
   16,
   17,    /* 55 */
   18,
   19,
   20,
   22,
   25
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

      
