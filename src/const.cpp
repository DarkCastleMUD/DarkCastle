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
/* $Id: const.cpp,v 1.336 2015/06/16 04:10:54 pirahna Exp $ */

extern "C"
{
#include <stdio.h>
}

#include <map>
#include <string>

#include "obj.h"
#include "player.h" // *app_type
#include "character.h"
#include "spells.h"
#include "levels.h"
#include "mobile.h"

using namespace std;

map<int,int> fill_skill_cost()
{
  map<int,int> skill_cost_map;
  skill_cost_map[SKILL_KICK] = 5;
  skill_cost_map[SKILL_TRIP] = 7;
  skill_cost_map[SKILL_RESCUE] = 10;
  skill_cost_map[SKILL_REDIRECT] = 5;
  skill_cost_map[SKILL_EAGLE_CLAW] = 12;
  skill_cost_map[SKILL_QUIVERING_PALM] = 45;
  skill_cost_map[SKILL_STUN] = 15;
  skill_cost_map[SKILL_BACKSTAB] = 4;
  skill_cost_map[SKILL_SNEAK] = 5;
  skill_cost_map[SKILL_HIDE] = 2;
  skill_cost_map[SKILL_HARM_TOUCH] = 30;
  skill_cost_map[SKILL_STALK] = 12;
  skill_cost_map[SKILL_POCKET] = 15;
  skill_cost_map[SKILL_STEAL] = 20;
  skill_cost_map[SKILL_CIRCLE] = 12;
  skill_cost_map[SKILL_DISARM] = 20;
  skill_cost_map[SKILL_VITAL_STRIKE] = 15;
  skill_cost_map[SKILL_EYEGOUGE] = 12;
  skill_cost_map[SKILL_JAB] = 4;
  skill_cost_map[SKILL_CRIPPLE] = 30;
  skill_cost_map[SKILL_PALM] = 2;
  skill_cost_map[SKILL_SLIP] = 2;
  skill_cost_map[SKILL_DECEIT] = 16;
  skill_cost_map[SKILL_APPRAISE] = 6;
  skill_cost_map[SKILL_PICK_LOCK] = 3;
  skill_cost_map[SKILL_ESCAPE] = 25;
  skill_cost_map[SKILL_LAY_HANDS] = 30;
  skill_cost_map[SKILL_BASH] = 10;
  skill_cost_map[SKILL_FOCUSED_REPELANCE] =45 ;
  skill_cost_map[SKILL_BLOOD_FURY] = 60;
  skill_cost_map[SKILL_CRAZED_ASSAULT] = 18;
  skill_cost_map[SKILL_RAGE] = 6;
  skill_cost_map[SKILL_FEROCITY] = 15;
  skill_cost_map[SKILL_BATTLECRY] = 24;
  skill_cost_map[SKILL_BERSERK] = 12;
  skill_cost_map[SKILL_HITALL] = 24;
  skill_cost_map[SKILL_BULLRUSH] = 20;
  skill_cost_map[SKILL_KNOCKBACK] = 24;
  skill_cost_map[SKILL_ARCHERY] = 2;
  skill_cost_map[SKILL_FORAGE] = 8;
  skill_cost_map[SKILL_TAME] = 15;
  skill_cost_map[SKILL_AMBUSH] = 24;
  skill_cost_map[SKILL_TRACK] = 3;
  skill_cost_map[SKILL_TACTICS] = 15;
  skill_cost_map[SKILL_RETREAT] = 15;
  skill_cost_map[SKILL_DEATHSTROKE] = 20;
  skill_cost_map[SKILL_BLADESHIELD] = 15;
  skill_cost_map[SKILL_GUARD] = 5;
  skill_cost_map[SKILL_PRIMAL_FURY] = 40;
  skill_cost_map[SKILL_HEADBUTT] = 15;
  skill_cost_map[SKILL_FREE_ANIMAL] = 25;
  skill_cost_map[SKILL_BEHEAD] = 25;
  skill_cost_map[SKILL_MAKE_CAMP] = 50;
  skill_cost_map[SKILL_ONSLAUGHT] = 50;
  skill_cost_map[SKILL_IMBUE] = 50;
  skill_cost_map[SKILL_BATTERBRACE] = 60;
  skill_cost_map[SKILL_TRIAGE] = 40;
  skill_cost_map[SKILL_BREW] = 100;
  skill_cost_map[SKILL_SCRIBE] = 100;
  skill_cost_map[SKILL_PURSUIT] = 20;
  return skill_cost_map;
}

map<int, int> fill_scribe_recipes()
{
  map<int, int> tmp_scribe_recipes;
  tmp_scribe_recipes[RARE1_PAPER | CLERIC_PEN 	| FIRE_INK 	| GENERIC_DUST] = 	SPELL_CREATE_FOOD;
  tmp_scribe_recipes[RARE1_PAPER | CLERIC_PEN 	| MAGIC_INK 	| GENERIC_DUST] = 	SPELL_CREATE_WATER;
  tmp_scribe_recipes[RARE1_PAPER | MAGE_PEN 	| MAGIC_INK 	| FLASHY_DUST] = 	SPELL_CONT_LIGHT;
  tmp_scribe_recipes[RARE1_PAPER | MAGE_PEN 	| MAGIC_INK 	| GENERIC_DUST] = 	SPELL_IDENTIFY;
  tmp_scribe_recipes[RARE1_PAPER | MAGE_PEN 	| FIRE_INK 	| EXPLOSIVE_DUST] =	SPELL_BURNING_HANDS;
  tmp_scribe_recipes[RARE1_PAPER | CLERIC_PEN 	| EVIL_INK 	| GENERIC_DUST] = 	SPELL_CAUSE_CRITICAL;
  tmp_scribe_recipes[RARE1_PAPER | NONE_PEN 	| FIRE_INK 	| FLASHY_DUST] = 	SPELL_SPARKS;
  tmp_scribe_recipes[RARE1_PAPER | MAGE_PEN 	| EVIL_INK 	| FLASHY_DUST] = 	SPELL_SHOCKING_GRASP;
  tmp_scribe_recipes[RARE1_PAPER | DRUID_PEN 	| MAGIC_INK 	| GENERIC_DUST] = 	SPELL_BLUE_BIRD;

  tmp_scribe_recipes[RARE2_PAPER | CLERIC_PEN 	| FIRE_INK 	| FLASHY_DUST] = 	SPELL_HEROES_FEAST;
  tmp_scribe_recipes[RARE2_PAPER | NONE_PEN 	| MAGIC_INK 	| GENERIC_DUST] =	SPELL_INSOMNIA;
  tmp_scribe_recipes[RARE2_PAPER | DRUID_PEN 	| FIRE_INK 	| GENERIC_DUST] =	SPELL_CONTROL_WEATHER;
  tmp_scribe_recipes[RARE2_PAPER | ANTI_PEN 	| FIRE_INK 	| FLASHY_DUST] =	SPELL_GLOBE_OF_DARKNESS;
  tmp_scribe_recipes[RARE2_PAPER | NONE_PEN 	| MAGIC_INK 	| GENERIC_DUST] =	SPELL_HOWL;
  tmp_scribe_recipes[RARE2_PAPER | CLERIC_PEN 	| EVIL_INK 	| GENERIC_DUST] =	SPELL_HARM;
  tmp_scribe_recipes[RARE2_PAPER | MAGE_PEN 	| MAGIC_INK 	| GENERIC_DUST] =	SPELL_CHILL_TOUCH;
  tmp_scribe_recipes[RARE2_PAPER | RANGER_PEN 	| MAGIC_INK 	| GENERIC_DUST] =	SPELL_BEE_STING;
  tmp_scribe_recipes[RARE2_PAPER | MAGE_PEN 	| MAGIC_INK 	| FLASHY_DUST] =	SPELL_LIGHTNING_BOLT;
  tmp_scribe_recipes[RARE2_PAPER | MAGE_PEN	| FIRE_INK 	| EXPLOSIVE_DUST] =	SPELL_FIREBALL;

  tmp_scribe_recipes[RARE3_PAPER | MAGE_PEN	| MAGIC_INK	| FLASHY_DUST] = 	SPELL_MASS_INVISIBILITY;
  tmp_scribe_recipes[RARE3_PAPER | CLERIC_PEN	| MAGIC_INK	| GENERIC_DUST] =	SPELL_REMOVE_PARALYSIS;
  tmp_scribe_recipes[RARE3_PAPER | MAGE_PEN	| FIRE_INK	| GENERIC_DUST] =	SPELL_DISPEL_MINOR;
  tmp_scribe_recipes[RARE3_PAPER | DRUID_PEN	| MAGIC_INK	| GENERIC_DUST] =	SPELL_WEAKEN;
  tmp_scribe_recipes[RARE3_PAPER | DRUID_PEN	| EVIL_INK	| GENERIC_DUST] =	SPELL_POISON;
  tmp_scribe_recipes[RARE3_PAPER | MAGE_PEN	| EVIL_INK	| GENERIC_DUST] =	SPELL_FEAR;
  tmp_scribe_recipes[RARE3_PAPER | DRUID_PEN	| MAGIC_INK	| FLASHY_DUST] =	SPELL_DROWN;
  tmp_scribe_recipes[RARE3_PAPER | ANTI_PEN	| FIRE_INK	| GENERIC_DUST] =	SPELL_VAMPIRIC_TOUCH;
  tmp_scribe_recipes[RARE3_PAPER | DRUID_PEN	| FIRE_INK	| EXPLOSIVE_DUST] =	SPELL_EARTHQUAKE;
  tmp_scribe_recipes[RARE3_PAPER | RANGER_PEN	| MAGIC_INK	| FLASHY_DUST] =	SPELL_BEE_SWARM;

  tmp_scribe_recipes[RARE4_PAPER | DRUID_PEN	| EVIL_INK	| FLASHY_DUST] =	SPELL_CURSE;
  tmp_scribe_recipes[RARE4_PAPER | DRUID_PEN	| EVIL_INK	| GENERIC_DUST] =	SPELL_ATTRITION;
  tmp_scribe_recipes[RARE4_PAPER | DRUID_PEN	| MAGIC_INK	| GENERIC_DUST] = 	SPELL_DEBILITY;
  tmp_scribe_recipes[RARE4_PAPER | DRUID_PEN	| MAGIC_INK	| FLASHY_DUST] =	SPELL_ENTANGLE;
  tmp_scribe_recipes[RARE4_PAPER | CLERIC_PEN	| FIRE_INK	| EXPLOSIVE_DUST] = 	SPELL_FLAMESTRIKE;
  tmp_scribe_recipes[RARE4_PAPER | ANTI_PEN	| MAGIC_INK	| EXPLOSIVE_DUST] =	SPELL_ACID_BLAST;
  tmp_scribe_recipes[RARE4_PAPER | CLERIC_PEN	| EVIL_INK	| FLASHY_DUST] =	SPELL_DISPEL_GOOD;
  tmp_scribe_recipes[RARE4_PAPER | CLERIC_PEN	| MAGIC_INK	| FLASHY_DUST] =	SPELL_DISPEL_EVIL;
  tmp_scribe_recipes[RARE4_PAPER | MAGE_PEN	| FIRE_INK	| EXPLOSIVE_DUST] =	SPELL_FIRESTORM;
  tmp_scribe_recipes[RARE4_PAPER | NONE_PEN	| MAGIC_INK	| FLASHY_DUST] =	SPELL_WILD_MAGIC;

  tmp_scribe_recipes[RARE5_PAPER | MAGE_PEN	| MAGIC_INK	| GENERIC_DUST] =	SPELL_TELEPORT;
  tmp_scribe_recipes[RARE5_PAPER | DRUID_PEN	| EVIL_INK	| GENERIC_DUST] =	SPELL_BLINDNESS;
  tmp_scribe_recipes[RARE5_PAPER | RANGER_PEN	| MAGIC_INK	| FLASHY_DUST] =	SPELL_CREEPING_DEATH;
  tmp_scribe_recipes[RARE5_PAPER | DRUID_PEN	| FIRE_INK	| FLASHY_DUST] =	SPELL_COLOUR_SPRAY;
  tmp_scribe_recipes[RARE5_PAPER | DRUID_PEN	| MAGIC_INK	| GENERIC_DUST] =	SPELL_SUN_RAY;
  tmp_scribe_recipes[RARE5_PAPER | DRUID_PEN	| MAGIC_INK	| EXPLOSIVE_DUST] =	SPELL_CALL_LIGHTNING;
  tmp_scribe_recipes[RARE5_PAPER | CLERIC_PEN	| EVIL_INK	| GENERIC_DUST] =	SPELL_POWER_HARM;
  tmp_scribe_recipes[RARE5_PAPER | MAGE_PEN	| FIRE_INK	| EXPLOSIVE_DUST] =	SPELL_SOLAR_GATE;

  return tmp_scribe_recipes;
}

map<int, int> scribe_recipes = fill_scribe_recipes();

map<int, int> fill_scribe_ingredients()
{
  map<int, int> tmp_scribe_ingredients;
  tmp_scribe_ingredients[0] = RARE1_PAPER;
  tmp_scribe_ingredients[0] = RARE2_PAPER;
  tmp_scribe_ingredients[0] = RARE3_PAPER;
  tmp_scribe_ingredients[0] = RARE4_PAPER;
  tmp_scribe_ingredients[0] = RARE5_PAPER;

  tmp_scribe_ingredients[0] = CLERIC_PEN;
  tmp_scribe_ingredients[0] = MAGE_PEN;
  tmp_scribe_ingredients[0] = DRUID_PEN;
  tmp_scribe_ingredients[0] = ANTI_PEN;
  tmp_scribe_ingredients[0] = RANGER_PEN;
  tmp_scribe_ingredients[0] = NONE_PEN;

  tmp_scribe_ingredients[0] = MAGIC_INK;
  tmp_scribe_ingredients[0] = FIRE_INK;
  tmp_scribe_ingredients[0] = EVIL_INK;

  tmp_scribe_ingredients[0] = FLASHY_DUST;
  tmp_scribe_ingredients[0] = EXPLOSIVE_DUST;
  tmp_scribe_ingredients[0] = GENERIC_DUST;

  return tmp_scribe_ingredients;
}

map<int, int> scribe_ingredients = fill_scribe_ingredients();

vector<profession> fill_professions(void)
{
  vector<profession> tmp_professions;
  
  profession p;
  p.name = string("legionnaire");
  p.Name = string("Legionnaire");
  p.c_class = CLASS_WARRIOR;
  p.skillno = SKILL_LEGIONNAIRE;
  tmp_professions.push_back(p);

  p.name = string("gladiator");
  p.Name = string("Gladiator");
  p.c_class = CLASS_WARRIOR;
  p.skillno = SKILL_GLADIATOR;
  tmp_professions.push_back(p);

  p.name = string("battlerager");
  p.Name = string("Battlerager");
  p.c_class = CLASS_BARBARIAN;
  p.skillno = SKILL_BATTLERAGER;
  tmp_professions.push_back(p);

  p.name = string("chieftan");
  p.Name = string("Chieftan");
  p.c_class = CLASS_BARBARIAN;
  p.skillno = SKILL_CHIEFTAN;
  tmp_professions.push_back(p);

  p.name = string("pilferer");
  p.Name = string("Pilferer");
  p.c_class = CLASS_THIEF;
  p.skillno = SKILL_PILFERER;
  tmp_professions.push_back(p);

  p.name = string("assassin");
  p.Name = string("Assassin");
  p.c_class = CLASS_THIEF;
  p.skillno = SKILL_ASSASSIN;
  tmp_professions.push_back(p);

  p.name = string("warmage");
  p.Name = string("Warmage");
  p.c_class = CLASS_MAGE;
  p.skillno = SKILL_WARMAGE;
  tmp_professions.push_back(p);

  p.name = string("spellbinder");
  p.Name = string("Spellbinder");
  p.c_class = CLASS_MAGE;
  p.skillno = SKILL_SPELLBINDER;
  tmp_professions.push_back(p);

  p.name = string("zealot");
  p.Name = string("Zealot");
  p.c_class = CLASS_CLERIC;
  p.skillno = SKILL_ZEALOT;
  tmp_professions.push_back(p);

  p.name = string("ritualist");
  p.Name = string("Ritualist");
  p.c_class = CLASS_CLERIC;
  p.skillno = SKILL_RITUALIST;
  tmp_professions.push_back(p);

  p.name = string("elementalist");
  p.Name = string("Elementalist");
  p.c_class = CLASS_DRUID;
  p.skillno = SKILL_ELEMENTALIST;
  tmp_professions.push_back(p);

  p.name = string("shapeshifter");
  p.Name = string("Shapeshifter");
  p.c_class = CLASS_DRUID;
  p.skillno = SKILL_SHAPESHIFTER;
  tmp_professions.push_back(p);

  p.name = string("cultist");
  p.Name = string("Cultist");
  p.c_class = CLASS_ANTI_PAL;
  p.skillno = SKILL_CULTIST;
  tmp_professions.push_back(p);

  p.name = string("reaver");
  p.Name = string("Reaver");
  p.c_class = CLASS_ANTI_PAL;
  p.skillno = SKILL_REAVER;
  tmp_professions.push_back(p);

  p.name = string("templar");
  p.Name = string("Templar");
  p.c_class = CLASS_PALADIN;
  p.skillno = SKILL_TEMPLAR;
  tmp_professions.push_back(p);

  p.name = string("inquisitor");
  p.Name = string("Inquisitor");
  p.c_class = CLASS_PALADIN;
  p.skillno = SKILL_INQUISITOR;
  tmp_professions.push_back(p);

  p.name = string("scout");
  p.Name = string("Scout");
  p.c_class = CLASS_RANGER;
  p.skillno = SKILL_SCOUT;
  tmp_professions.push_back(p);

  p.name = string("tracker");
  p.Name = string("Tracker");
  p.c_class = CLASS_RANGER;
  p.skillno = SKILL_TRACKER;
  tmp_professions.push_back(p);

  p.name = string("sensei");
  p.Name = string("Sensei");
  p.c_class = CLASS_MONK;
  p.skillno = SKILL_SENSEI;
  tmp_professions.push_back(p);

  p.name = string("spiritualist");
  p.Name = string("Spiritualist");
  p.c_class = CLASS_MONK;
  p.skillno = SKILL_SPIRITUALIST;
  tmp_professions.push_back(p);

  p.name = string("troubadour");
  p.Name = string("Troubadour");
  p.c_class = CLASS_BARD;
  p.skillno = SKILL_TROUBADOUR;
  tmp_professions.push_back(p);

  p.name = string("minstrel");
  p.Name = string("Minstrel");
  p.c_class = CLASS_BARD;
  p.skillno = SKILL_MINISTREL;
  tmp_professions.push_back(p);

  return tmp_professions;
}

vector<profession> professions = fill_professions();

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

const char *utility_item_types[] {
 "Nothing - Useless",
 "CatStink - Track remover",
 "ExitTrap - Do Not Use",
 "MoveTrap - Do Not Use",
 "Mortar",
 "\n"
};

// every spell needs an entry in here
const char *spell_wear_off_msg[] =
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
  "Your farsight ability has expired.",
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
  "The protective intervention of the gods has ended.",
  "!WRATHOFGOD!",
  "!ATONEMENT!",
  "!SILENCE!",
  "The $Bs$3h$5i$7m$3m$5e$7r$3i$5n$7g$3 l$5i$7g$3h$5t$R surrounding your body slowly fades.",
  "!BONESHIELD!",
  "!CHANNEL!",
  "!RELEASEELEMENTAL!",
  "!WILDMAGIC!",
  "!SPIRITSHIELD!",
  "You no longer feel especially villianous.",
  "You no longer feel especially heroic.",
  "!CONSECRATE!",
  "!DESECRATE!",
  "!SPELL_ELEMENTAL_WALL!",
  "You cannot hold your ethereal focus any longer.",
  "BUG DETECTED: Tell an Imm. (Spell Wear Off Message)"
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

const char *connected_states[] =
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

const char *dirs[] =
{
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "\n"
};

const char *dirswards[] =
{
    "northward",
    "eastward",
    "southward",
    "westward",
    "upward",
    "downward",
    "\n"
};

const char *weekdays[7] =
{
    "Aetherday",
    "the Day of Firesong",
    "the Day of the Wave",
    "the Day of Stonefall",
    "the Day of Glowing Eyes",
    "the Day of Balance",
    "Praiseday"
};

const char *month_name[17] =
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

const char *where[] =
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

const char *strs_damage_types[] = 
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

const char *drinks[] =
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
    "ink",
    "\n"
};

const char *drinknames[] =
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

const char *color_liquid[]=
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

const char *fullness[] =
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


const char *mob_types[] =
{
		"NORMAL",
		"GUARD",
		"CLAN GUARD",
		"\n"
};

const char *item_types[] =
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

const char *wear_bits[] =
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

const char *zone_modes[] =
{
    "Don't_Update",
    "RepopIfEmpty",
    "Always_Repop",
    "\n"
};


vector<string> get_cont_names()
{
  vector<string> tmp;
  tmp.push_back("");
  tmp.push_back("Undefined");
  tmp.push_back("Sorpigal");
  tmp.push_back("Kyu Shi'i Kaze");
  tmp.push_back("Diamond Isle");
  tmp.push_back("Underdark");
  tmp.push_back("Behind the Mirror");
  tmp.push_back("Planes of Existence");
  tmp.push_back("Forbidden Island");
  tmp.push_back("Other");
  return tmp;
}

vector<string> continent_names = get_cont_names();


const char *zone_bits[] =
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
const char *extra_bits[] =
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
    "QUEST_ITEM",
    "\n"
};

// more_flags obj flags

const char *more_obj_bits[] =
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
    "CUSTOM",
	"24H_SAVE",
	"NO_DISARM",
    "PROCTOGGLE",
    "\n"
};

const char *size_bitfields[] =
{
    "ANY",
    "SMALL",
    "MEDIUM",
    "LARGE",
    "\n"
};

const char *size_bits[] =
{
    "Any race",
    "Small races",
    "Medium races",
    "Large races",
    "\n"
};

const char *room_bits[] =
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

const char *temp_room_bits[] =
{
    "ETHEREAL_FOCUS",
    "\n"
};

const char *exit_bits[] =
{
    "IS-DOOR",
    "CLOSED",
    "LOCKED",
    "HIDDEN",
    "IMM_ONLY",
    "PICKPROOF",
    "BROKEN",
    "\n"
};

const char *sector_types[] =
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

const char *time_look[] =
{
    "night time",
    "sun rise",
    "day time",
    "sun set",
    "/n"
};

const char *sky_look[] =
{
    "cloudless",
    "cloudy",
    "rainy",
    "pouring rain",
    "lit by flashes of lightning",
    "/n"
};

const char *equipment_types[] =
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
    "Held 2",
    "Worn Face",
    "Worn Right Ear",
    "Worn Left Ear",
    "Worn Penis",
    "\n"
};


/* Should be in exact correlation as the AFF types -Kahn */
const char *affected_bits[] =
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
    "JAB_ALERT",
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
    "JAB",
    "NO_REGEN",
    "ACID_SHIELD",
    "PRIMAL_FURY",
    "IS_ELEMENTAL",
    "ONSLAUGHT",
    "\n"
};

/* Should be in exact correlation as the APPLY types -Kahn */

const char *apply_types[] =
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
    "SPELLDAMAGE",
    "FREEDOM FROM HUNGER",
    "FREEDOM FROM THIRST",
    "AFF BLIND",
    "WATERBREATHING",
    "DETECT MAGIC",
    "WILD MAGIC",
    "\n"
};

const char *pc_clss_types[] =
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

const char *pc_clss_types2[] =
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

const char *pc_clss_types3[] =
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



const char* pc_clss_abbrev[] =
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


const char *class_tree_name[11][3] = {
  {"Warmage", "Spellbinder", "MAGE3"},
  {"Zealot", "Ritualist", "CLERIC3"},
  {"Pilferer", "Assassin", "THIEF3"},
  {"Gladiator", "Legionnaire", "WARRIOR3"},
  {"Cultist", "Reaver", "ANTI3"},
  {"Samurai", "Templar", "PALADIN3"},
  {"Battlerager", "Chieftan", "BARB3"},
  {"Sensei", "Spiritualist", "MONK3"},
  {"Scout", "Tracker", "RANGER3"},
  {"Troubadour", "Minstrel", "BARD3"},
  {"Elementalist", "Shapeshifter", "DRUID3"}
};

// Following X_skills[] arrays use the following format

// Begin abilities listings for each class.

// skills master 10011 - done and checked, Apoc
struct class_skill_defines g_skills[] = { // all-class skills

//   Ability Name       Ability File          Level     Max  Group  Requisites
//   ------------       ------------          -----     ---  -----  ----------
{    "consider",        SKILL_CONSIDER,         1,      90,  0,   INTWIS },
{    "scan",            SKILL_SCAN,             3,      90,  0,   CONWIS },
{    "switch",          SKILL_SWITCH,           5,      90,  0,   DEXINT },
{    "rapid join",      SKILL_FASTJOIN,         7,      90,  0,   STRDEX },
{    "release",         SKILL_RELEASE,          10,     90,  0,   INTWIS },
{    "\n",              0,                      1,      0,   0,   0 }
};

// warrior 3023 guildmaster - done and checked, Apoc
struct class_skill_defines w_skills[] = { // warrior skills

//   Ability Name       Ability File            Level  Max  Group  Requisite
//   ------------       ------------            -----  ---  -----  ---------
{    "kick",            SKILL_KICK,               1,    80,  0,   STRDEX },
{    "bash",            SKILL_BASH,               2,    85,  0,   STRDEX },
{    "redirect",        SKILL_REDIRECT,           4,    90,  0,   DEXINT },
{    "rescue",          SKILL_RESCUE,             5,    70,  0,   DEXINT },
{    "double",          SKILL_SECOND_ATTACK,      7,    90,  0,   STRDEX },
{    "disarm",          SKILL_DISARM,             10,   70,  0,   DEXINT },
{    "headbutt",        SKILL_HEADBUTT,           12,   80,  0,   STRDEX },
{    "shield block",    SKILL_SHIELDBLOCK,        15,   85,  0,   STRCON },
{    "retreat",         SKILL_RETREAT,            17,  100,  0,   DEXINT },
{    "frenzy",          SKILL_FRENZY,             18,   80,  0,   STRCON },
{    "parry",           SKILL_PARRY,              20,   90,  0,   STRCON },
{    "blindfighting",   SKILL_BLINDFIGHTING,      21,   80,  0,   STRCON },
{"enhanced regeneration",SKILL_ENHANCED_REGEN,    22,   80,  0,   STRCON },
{    "triple",          SKILL_THIRD_ATTACK,       23,   90,  0,   STRDEX },
{    "hitall",          SKILL_HITALL,             25,   80,  0,   STRDEX },
{    "dual wield",      SKILL_DUAL_WIELD,         28,   90,  0,   DEXINT },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS,   30,   90,  0,   CONWIS },
{    "crushing",        SKILL_CRUSHING_WEAPONS,   30,   90,  0,   CONWIS },
{    "piercing",        SKILL_PIERCEING_WEAPONS,  30,   90,  0,   CONWIS },
{    "slashing",        SKILL_SLASHING_WEAPONS,   30,   90,  0,   CONWIS },
{    "whipping",        SKILL_WHIPPING_WEAPONS,   30,   90,  0,   CONWIS },
{    "tactics",         SKILL_TACTICS,            31,  100,  0,   DEXINT },
{    "archery",         SKILL_ARCHERY,            32,   70,  0,   CONWIS },
{    "stun",            SKILL_STUN,               35,   85,  0,   DEXINT },
{    "guard",           SKILL_GUARD,              37,  100,  0,   STRCON },
{    "deathstroke",     SKILL_DEATHSTROKE,        39,  100,  0,   STRDEX },
{    "riposte",         SKILL_RIPOSTE,            40,  100,  0,   STRCON },
{    "two handers",     SKILL_TWO_HANDED_WEAPONS, 42,   85,  0,   CONWIS },
{    "stinging",        SKILL_STINGING_WEAPONS,   43,   90,  0,   CONWIS },
{    "skewer",          SKILL_SKEWER,             45,  100,  0,   STRDEX },
{    "blade shield",    SKILL_BLADESHIELD,        47,  100,  0,   STRCON },
{    "combat mastery",  SKILL_COMBAT_MASTERY,     50,  100,  0,   DEXINT },
{    "onslaught",       SKILL_ONSLAUGHT,          51,  100,  0,   DEXINT },
//{    "battlesense",     SKILL_BATTLESENSE,        53,  100,  1,   STRCON },
//{    "perseverance",    SKILL_PERSEVERANCE,       53,  100,  2,   CONWIS },
{    "triage",          SKILL_TRIAGE,             55,  100,  0,   DEXINT },
//{    "smite",           SKILL_SMITE,              57,  100,  1,   STRDEX },
//{    "leadership",      SKILL_LEADERSHIP,         57,  100,  2,   STRDEX },
//{    "execute",         SKILL_EXECUTE,            60,  100,  1,   CONWIS },
//{    "defenders stance",SKILL_DEFENDERS_STANCE,   60,  100,  2,   STRCON },
{    "\n",              0,                        1,    0,   0,   0 }
};

// thief 3022 guildmaster - done and checked, Apoc
struct class_skill_defines t_skills[] = { // thief skills

//   Ability Name       Ability File          Level    Max  Group  Requisites
//   ------------       ------------          -----    ---  -----  ----------
{    "backstab",        SKILL_BACKSTAB,         1,      90,  0,  STRDEX },  
{    "sneak",           SKILL_SNEAK,            2,      90,  0,  DEXINT },  
{    "parry",           SKILL_PARRY,            4,      40,  0,  CONINT },
{    "stalk",           SKILL_STALK,            6,     100,  0,  DEXINT },
{    "hide",            SKILL_HIDE,             7,      90,  0,  DEXINT },  
{    "dual wield",      SKILL_DUAL_WIELD,       10,     90,  0,  CONINT },  
{    "trip",            SKILL_TRIP,             11,     85,  0,  DEXINT },
{    "palm",            SKILL_PALM,             12,    100,  0,  DEXWIS },  
{    "slip",            SKILL_SLIP,             13,    100,  0,  DEXWIS },  
{    "dodge",           SKILL_DODGE,            15,     90,  0,  DEXWIS },
{    "jab",             SKILL_JAB,              17,    100,  0,  STRDEX },
{    "pocket",          SKILL_POCKET,           20,    100,  0,  DEXINT },
{    "appraise",        SKILL_APPRAISE,         21,    100,  0,  DEXWIS },
{    "pick",            SKILL_PICK_LOCK,        22,    100,  0,  DEXWIS },  
{    "steal",           SKILL_STEAL,            25,    100,  0,  DEXINT },
{    "blindfighting",   SKILL_BLINDFIGHTING,    28,     80,  0,  DEXWIS },
{    "piercing",        SKILL_PIERCEING_WEAPONS,30,     90,  0,  CONINT },
{    "slashing",        SKILL_SLASHING_WEAPONS, 30,     60,  0,  CONINT },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS, 30,     70,  0,  CONINT },
{    "stinging",        SKILL_STINGING_WEAPONS, 30,     85,  0,  CONINT },  
{    "deceit",          SKILL_DECEIT,           31,    100,  0,  DEXWIS },  
{    "circle",          SKILL_CIRCLE,           35,    100,  0,  DEXINT },  
{    "disarm",          SKILL_DISARM,           38,     85,  0,  STRDEX },  
{    "dual backstab",   SKILL_DUAL_BACKSTAB,    40,    100,  0,  STRDEX },  
{    "eyegouge",        SKILL_EYEGOUGE,         42,    100,  0,  STRDEX },  
{    "vitalstrike",     SKILL_VITAL_STRIKE,     45,    100,  0,  STRDEX },  
{    "cripple",		SKILL_CRIPPLE,		50,    100,  0,  STRDEX },
{    "escape",		SKILL_ESCAPE,		51,    100,  0,  DEXWIS },
{    "critical hit",	SKILL_CRIT_HIT,		55,    100,  0,  CONINT },
{    "\n",              0,                      1,      0,   0,  0 }
};

// anti-paladin 10005 guildmaster - done and checked, Apoc
struct class_skill_defines a_skills[] = { // anti-paladin skills

//   Ability Name            Ability File           Level     Max  Group   Requisites
//   ------------            ------------           -----     ---  -----   ----------
{    "harmtouch",            SKILL_HARM_TOUCH,        1,     100,  0,   CONWIS },
{    "kick",                 SKILL_KICK,              2,      70,  0,   STRDEX },
{    "sneak",                SKILL_SNEAK,             3,      85,  0,   DEXINT },
{    "shield block",         SKILL_SHIELDBLOCK,       5,      60,  0,   STRDEX },
{    "infravision",          SPELL_INFRAVISION,       7,      90,  0,   STRINT },
{    "detect good",          SPELL_DETECT_GOOD,       8,     100,  0,   CONINT },
{    "dual wield",           SKILL_DUAL_WIELD,        10,     85,  0,   STRDEX },
{    "shocking grasp",       SPELL_SHOCKING_GRASP,    11,    100,  0,   STRINT },
{    "detect invisibility",  SPELL_DETECT_INVISIBLE,  12,     85,  0,   STRINT },
{    "invisibility",         SPELL_INVISIBLE,         13,     85,  0,   DEXINT },
{    "backstab",             SKILL_BACKSTAB,          15,     90,  0,   DEXINT },
{    "hide",                 SKILL_HIDE,              17,     85,  0,   DEXINT },
{    "trip",                 SKILL_TRIP,              19,     85,  0,   DEXINT },
{    "chill touch",          SPELL_CHILL_TOUCH,       20,     85,  0,   STRINT },
{    "double",               SKILL_SECOND_ATTACK,     22,     85,  0,   STRDEX },
{    "dodge",                SKILL_DODGE,             23,     80,  0,   DEXINT },
{    "vampiric touch",       SPELL_VAMPIRIC_TOUCH,    25,    100,  0,   CONWIS },
{    "poison",               SPELL_POISON,            27,     70,  0,   CONINT },
{    "animate dead",         SPELL_ANIMATE_DEAD,      28,     80,  0,   CONWIS },
{    "dismiss corpse",       SPELL_DISMISS_CORPSE,    29,     90,  0,   CONWIS },
{    "piercing",             SKILL_PIERCEING_WEAPONS, 30,     85,  0,   STRDEX },
{    "slashing",             SKILL_SLASHING_WEAPONS,  30,     85,  0,   STRDEX },
{    "crushing",             SKILL_CRUSHING_WEAPONS,  30,     85,  0,   STRDEX },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,  30,     85,  0,   STRDEX },
{    "visage of hate",       SPELL_VISAGE_OF_HATE,    31,    100,  0,   CONINT },
{    "blindfighting",        SKILL_BLINDFIGHTING,     32,     70,  0,   DEXINT },
{    "globe of darkness",    SPELL_GLOBE_OF_DARKNESS, 33,    100,  0,   DEXINT },
{    "beacon",               SPELL_BEACON,            35,    100,  0,   CONWIS },
{    "fear",                 SPELL_FEAR,              37,     85,  0,   CONINT },
{    "dispel good",          SPELL_DISPEL_GOOD,       38,     90,  0,   CONINT },
{    "acid shield",          SPELL_ACID_SHIELD,       40,    100,  0,   STRINT },
{    "curse",                SPELL_CURSE,             41,     70,  0,   CONWIS },
{    "life leech",           SPELL_LIFE_LEECH,        42,    100,  0,   CONWIS },
{    "unholy aegis",         SPELL_U_AEGIS,           44,    100,  0,   CONINT },
{    "protection from good", SPELL_PROTECT_FROM_GOOD, 45,     85,  0,   CONINT },
{    "resist acid",          SPELL_RESIST_ACID,       46,     70,  0,   STRINT },
{    "acid blast",           SPELL_ACID_BLAST,        48,    100,  0,   STRINT },
{    "vampiric aura",        SPELL_VAMPIRIC_AURA,     50,    100,  0,   CONWIS },
{    "villainy",             SPELL_VILLAINY,          51,    100,  0,   STRINT },
{    "desecrate",            SPELL_DESECRATE,         55,    100,  0,   CONINT },
{    "\n",                   0,                       1,      0,   0,   0 }
};

// paladin 10006 guildmaster - done and checked, Apoc
struct class_skill_defines p_skills[] = { // paladin skills

//   Ability Name            Ability File            Level     Max  Group   Requisites 
//   ------------            ------------            -----     ---  -----   ---------- 
{    "layhands",             SKILL_LAY_HANDS,          1,     100,  0,   INTWIS },
{    "kick",                 SKILL_KICK,               2,      70,  0,   STRCON },
{    "bless",                SPELL_BLESS,              3,      90,  0,   CONWIS },
{    "double",               SKILL_SECOND_ATTACK,      5,      85,  0,   DEXINT },
{    "shield block",         SKILL_SHIELDBLOCK,        7,      90,  0,   CONWIS },
{    "rescue",               SKILL_RESCUE,             8,      85,  0,   CONWIS },
{    "cure light",           SPELL_CURE_LIGHT,         9,      85,  0,   INTWIS },
{    "dual wield",           SKILL_DUAL_WIELD,         10,     80,  0,   STRCON },
{    "detect poison",        SPELL_DETECT_POISON,      11,     85,  0,   STRWIS },
{    "create food",          SPELL_CREATE_FOOD,        12,     85,  0,   INTWIS },
{    "create water",         SPELL_CREATE_WATER,       13,     85,  0,   INTWIS },
{    "cure serious",         SPELL_CURE_SERIOUS,       15,     85,  0,   INTWIS },
{    "detect evil",          SPELL_DETECT_EVIL,        17,    100,  0,   STRWIS },
{    "remove poison",        SPELL_REMOVE_POISON,      18,     85,  0,   CONWIS },
{    "detect invisibility",  SPELL_DETECT_INVISIBLE,   20,     85,  0,   STRWIS },
{    "cure critical",        SPELL_CURE_CRITIC,        22,     85,  0,   INTWIS },
{    "parry",                SKILL_PARRY,              23,     70,  0,   DEXINT },
{    "bash",                 SKILL_BASH,               25,     80,  0,   STRCON },
{    "sense life",           SPELL_SENSE_LIFE,         26,     85,  0,   STRWIS },
{    "strength",             SPELL_STRENGTH,           28,     70,  0,   STRWIS },
{    "earthquake",           SPELL_EARTHQUAKE,         29,     70,  0,   STRCON },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,   30,     85,  0,   DEXINT },
{    "slashing",             SKILL_SLASHING_WEAPONS,   30,     85,  0,   DEXINT },
{    "crushing",             SKILL_CRUSHING_WEAPONS,   30,     85,  0,   DEXINT },
{    "blessed halo",         SPELL_BLESSED_HALO,       31,    100,  0,   STRWIS },
{    "triple",               SKILL_THIRD_ATTACK,       33,     80,  0,   DEXINT },
{    "two handers",          SKILL_TWO_HANDED_WEAPONS, 35,     80,  0,   DEXINT },
{    "heal",                 SPELL_HEAL,               37,     85,  0,   INTWIS },
{    "harm",                 SPELL_HARM,               38,     85,  0,   STRCON },
{    "sanctuary",            SPELL_SANCTUARY,          40,     85,  0,   CONWIS },
{    "holy aegis",           SPELL_AEGIS,              42,    100,  0,   STRWIS },
{    "dispel evil",          SPELL_DISPEL_EVIL,        43,     90,  0,   STRCON },
{    "protection from evil", SPELL_PROTECT_FROM_EVIL,  45,     85,  0,   CONWIS },
{    "resist cold",          SPELL_RESIST_COLD,        46,     70,  0,   STRWIS },
{    "divine fury",          SPELL_DIVINE_FURY,        48,    100,  0,   STRCON },
{    "behead",               SKILL_BEHEAD,             49,    100,  0,   DEXINT },
{    "holy aura",            SPELL_HOLY_AURA,          50,    100,  0,   CONWIS },
{    "heroism",              SPELL_HEROISM,            51,    100,  0,   CONWIS },
{    "consecrate",           SPELL_CONSECRATE,         55,    100,  0,   STRWIS },
//{    "spirit shield",        SPELL_SPIRIT_SHIELD,      57,    100,  0,   INTWIS },
{    "\n",                   0,                        1,      0,   0,   0 }
};

// barbarian 10007 guildmaster - done and checked, Apoc
struct class_skill_defines b_skills[] = { // barbarian skills

//   Ability Name       Ability File            Level  Max  Group  Requisites
//   ------------       ------------            -----  ---  -----  ----------
{    "dual wield",      SKILL_DUAL_WIELD,         1,    85,  0,  DEXCON },
{    "bash",            SKILL_BASH,               2,    90,  0,  STRINT },
{    "kick",            SKILL_KICK,               3,    80,  0,  STRINT },
{    "parry",           SKILL_PARRY,              5,    70,  0,  DEXCON },
{    "double",          SKILL_SECOND_ATTACK,      8,    85,  0,  DEXWIS },
{    "dodge",           SKILL_DODGE,  	          10,   70,  0,  DEXCON },
{    "blood fury",      SKILL_BLOOD_FURY,         12,  100,  0,  STRCON },
{    "crazed assault",  SKILL_CRAZED_ASSAULT,     15,  100,  0,  STRCON },
{    "frenzy",          SKILL_FRENZY,             18,   90,  0,  DEXCON },
{    "rage",            SKILL_RAGE,               20,  100,  0,  STRCON },
{"enhanced regeneration",SKILL_ENHANCED_REGEN,    22,   90,  0,  DEXCON },
{    "triple",          SKILL_THIRD_ATTACK,       25,   85,  0,  DEXWIS },
{    "battlecry",       SKILL_BATTLECRY,          27,  100,  0,  STRINT },
{    "blindfighting",   SKILL_BLINDFIGHTING,      28,   60,  0,  DEXCON },
{    "whipping",        SKILL_WHIPPING_WEAPONS,   30,   85,  0,  DEXWIS },
{    "piercing",        SKILL_PIERCEING_WEAPONS,  30,   85,  0,  DEXWIS },
{    "slashing",        SKILL_SLASHING_WEAPONS,   30,   85,  0,  DEXWIS },
{    "bludgeoning",     SKILL_BLUDGEON_WEAPONS,   30,   85,  0,  DEXWIS },
{    "crushing",        SKILL_CRUSHING_WEAPONS,   30,   85,  0,  DEXWIS },
{    "ferocity",        SKILL_FEROCITY,           31,  100,  0,  STRINT },
{    "headbutt",        SKILL_HEADBUTT,           33,   90,  0,  STRINT },
{    "two handers",     SKILL_TWO_HANDED_WEAPONS, 35,   90,  0,  DEXWIS },
{    "archery",         SKILL_ARCHERY,            38,   80,  0,  DEXWIS },
{    "berserk",         SKILL_BERSERK,            40,  100,  0,  STRCON },
{    "hitall",          SKILL_HITALL,             45,   90,  0,  STRCON },
{    "magic resistance", SKILL_MAGIC_RESIST,      47,  100,  0,  DEXCON },
{    "knockback",       SKILL_KNOCKBACK,          48,  100,  0,  STRINT },
{    "bullrush",        SKILL_BULLRUSH,           50,  100,  0,  STRCON },
{    "batterbrace",     SKILL_BATTERBRACE,        51,  100,  0,  DEXWIS },
{    "vigor",           SKILL_VIGOR,              55,  100,  0,  DEXCON },
{    "pursuit",         SKILL_PURSUIT,            55,  100,  1,  DEXCON },
{    "\n",              0,                        1,    0,   0,  0 }
};

// monk 10008 guildmaster - done and checked, Apoc
struct class_skill_defines k_skills[] = { // monk skills

//   Ability Name       Ability File          Level    Max   Group   Requisites
//   ------------       ------------          -----    ---   -----   ----------
{    "kick",            SKILL_KICK,             1,      90,  0,   STRDEX },
{    "dodge",           SKILL_DODGE,            2,      80,  0,   DEXWIS },
{    "redirect",        SKILL_REDIRECT,         3,      85,  0,   DEXWIS },
{    "trip",            SKILL_TRIP,             5,      70,  0,   DEXWIS },
{    "purify",          KI_PURIFY+KI_OFFSET,    8,     100,  0,   CONWIS },
{    "martial defense", SKILL_DEFENSE,          10,    100,  0,   DEXWIS },
{    "rescue",          SKILL_RESCUE,           12,     80,  0,   DEXWIS },
{    "punch",           KI_PUNCH+KI_OFFSET,     15,    100,  0,   STRDEX },
{    "eagleclaw",       SKILL_EAGLE_CLAW,       17,    100,  0,   CONWIS },
{    "dual wield",      SKILL_DUAL_WIELD,       20,     50,  0,   DEXWIS },
{    "sense",           KI_SENSE+KI_OFFSET,     21,    100,  0,   CONWIS },
{    "stance",          KI_STANCE+KI_OFFSET,    24,    100,  0,   CONWIS },
{    "speed",           KI_SPEED+KI_OFFSET,     27,    100,  0,   STRDEX },
{    "whipping",        SKILL_WHIPPING_WEAPONS, 30,     60,  0,   STRDEX },
{    "hand to hand",    SKILL_HAND_TO_HAND,     30,    100,  0,   STRDEX },
{    "agility",         KI_AGILITY+KI_OFFSET,   31,    100,  0,   STRDEX },
{    "stun",            SKILL_STUN,             33,     90,  0,   CONINT },
{    "storm",           KI_STORM+KI_OFFSET,     35,    100,  0,   CONINT },
{    "blindfighting",   SKILL_BLINDFIGHTING,    38,     90,  0,   CONWIS },
{    "quiver",          SKILL_QUIVERING_PALM,   40,    100,  0,   CONINT },
{    "blast",           KI_BLAST+KI_OFFSET,     45,    100,  0,   CONINT },
{    "disrupt",         KI_DISRUPT+KI_OFFSET,   47,    100,  0,   CONINT },
{    "meditation",      KI_MEDITATION+KI_OFFSET,50,    100,  0,   CONWIS },
{    "transfer",        KI_TRANSFER+KI_OFFSET,  51,    100,  0,   CONWIS },
{    "counter strike",  SKILL_COUNTER_STRIKE,   55,    100,  0,   STRDEX },
{    "\n",              0,                      1,      0,   0,   0 }
};

// ranger 10013 guildmaster - done and checked, Apoc
struct class_skill_defines r_skills[] = { // ranger skills

//   Ability Name       Ability File           Level     Max  Group   Requisites 
//   ------------       ------------           -----     ---  -----   ---------- 
{    "bee sting",       SPELL_BEE_STING,         1,     100,  0,   STRCON },
{    "hide",            SKILL_HIDE,              2,      80,  0,   CONWIS },
{    "kick",            SKILL_KICK,              3,      70,  0,   STRDEX },
{    "dual wield",      SKILL_DUAL_WIELD,        5,      90,  0,   STRDEX },
{    "redirect",        SKILL_REDIRECT,          7,      80,  0,   CONWIS },
{    "eyes of the owl", SPELL_EYES_OF_THE_OWL,   8,      85,  0,   STRCON },
{    "sense life",      SPELL_SENSE_LIFE,        9,      85,  0,   CONWIS },
{    "dodge",    	SKILL_DODGE,    	 10,     70,  0,   CONWIS },
{    "tame",            SKILL_TAME,              11,    100,  0,   STRCON },
{    "double",          SKILL_SECOND_ATTACK,     12,     85,  0,   STRDEX },
{    "free animal",     SKILL_FREE_ANIMAL,       13,    100,  0,   STRCON },
{    "feline agility",  SPELL_FELINE_AGILITY,    14,    100,  0,   STRCON },
{    "bee swarm",       SPELL_BEE_SWARM,         15,    100,  0,   STRCON },
{    "forage",          SKILL_FORAGE,            16,     85,  0,   INTWIS },
{    "entangle",        SPELL_ENTANGLE,          18,     85,  0,   INTWIS },
{    "archery",         SKILL_ARCHERY,           20,     90,  0,   DEXINT },
{    "blindfighting",   SKILL_BLINDFIGHTING,     21,     85,  0,   CONWIS },
{    "parry",           SKILL_PARRY,             22,     80,  0,   CONWIS },
{    "herb lore",       SPELL_HERB_LORE,         23,     85,  0,   INTWIS },
{    "poison",          SPELL_POISON,            25,     85,  0,   INTWIS },
{    "tempest arrows",  SKILL_TEMPEST_ARROW,     26,    100,  0,   DEXINT },
{    "track",           SKILL_TRACK,             28,    100,  0,   DEXINT },
{    "barkskin",        SPELL_BARKSKIN,          29,     85,  0,   INTWIS },
{    "piercing",        SKILL_PIERCEING_WEAPONS, 30,     85,  0,   STRDEX },
{    "slashing",        SKILL_SLASHING_WEAPONS,  30,     85,  0,   STRDEX },
{    "whipping",        SKILL_WHIPPING_WEAPONS,  30,     90,  0,   STRDEX },
{    "ice arrows",      SKILL_ICE_ARROW,         31,    100,  0,   DEXINT },
{    "rescue",          SKILL_RESCUE,            32,     85,  0,   STRDEX },
{    "trip",            SKILL_TRIP,              33,     85,  0,   STRDEX },
{    "ambush",          SKILL_AMBUSH,            35,    100,  0,   DEXINT },
{    "fire arrows",     SKILL_FIRE_ARROW,        36,    100,  0,   DEXINT },
{    "call follower",   SPELL_CALL_FOLLOWER,     38,    100,  0,   STRCON },
{    "stun",            SKILL_STUN,              40,     80,  0,   INTWIS },
{    "granite arrows",  SKILL_GRANITE_ARROW,     41,    100,  0,   DEXINT },
{    "disarm",          SKILL_DISARM,            42,     80,  0,   CONWIS },
{    "staunchblood",    SPELL_STAUNCHBLOOD,      44,     85,  0,   CONWIS },
{    "forest meld",     SPELL_FOREST_MELD,       45,     85,  0,   INTWIS },
{    "camouflage",      SPELL_CAMOUFLAGE,        46,     85,  0,   INTWIS },
{    "creeping death",  SPELL_CREEPING_DEATH,    48,    100,  0,   STRCON },
{ "natural selection",	SKILL_NAT_SELECT,	 50,	100,  0,   DEXINT },
{    "make camp",       SKILL_MAKE_CAMP,          51,  100,  0,   CONWIS },
{    "offhand double",  SKILL_OFFHAND_DOUBLE,    55,    100,  0,   STRDEX },
{      "\n",            0,                       1,      0,   0,   0 }
};

// bard 3204 guildmaster - done and checked, Apoc
struct class_skill_defines d_skills[] = { // bard skills

// Ability Name            Ability File                  Level    Max   Group   Requisites
// ------------            ------------                  -----    ---   -----   ----------
{ "whistle sharp",         SKILL_SONG_WHISTLE_SHARP,       1,     100,  0,   DEXCON },
{ "stop",                  SKILL_SONG_STOP,                2,     100,  0,   CONINT },
{ "irresistable ditty",    SKILL_SONG_UNRESIST_DITTY,      3,     100,  0,   INTWIS },
{ "dodge",                 SKILL_DODGE,                    5,      80,  0,   STRDEX },
{ "sneak",                 SKILL_SNEAK,                    7,      70,  0,   STRDEX },
{ "travelling march",      SKILL_SONG_TRAVELING_MARCH,     9,     100,  0,   CONWIS },
{ "trip",                  SKILL_TRIP,                     10,     70,  0,   STRDEX },
{ "kick",                  SKILL_KICK,                     11,     70,  0,   STRDEX },
{ "bountiful sonnet",      SKILL_SONG_BOUNT_SONNET,        12,    100,  0,   CONWIS },
{ "healing melody",        SKILL_SONG_HEALING_MELODY,      13,    100,  0,   CONWIS },
{ "glitter dust",          SKILL_SONG_GLITTER_DUST,        15,    100,  0,   INTWIS },
{ "synchronous chord",     SKILL_SONG_SYNC_CHORD,          17,    100,  0,   INTWIS },
{ "sticky lullaby",        SKILL_SONG_STICKY_LULL,         18,    100,  0,   DEXCON },
{ "flight of the bumblebee", SKILL_SONG_FLIGHT_OF_BEE,     20,    100,  0,   CONWIS },
{ "note of knowledge",     SKILL_SONG_NOTE_OF_KNOWLEDGE,   21,    100,  0,   INTWIS },
{ "fanatical fanfare",     SKILL_SONG_FANATICAL_FANFARE,   23,    100,  0,   CONWIS },
{ "revealing staccato",    SKILL_SONG_REVEAL_STACATO,      25,    100,  0,   INTWIS },
{ "double",                SKILL_SECOND_ATTACK,            26,     80,  0,   STRDEX },
{ "terrible clef",         SKILL_SONG_TERRIBLE_CLEF,       28,    100,  0,   DEXCON },
{ "piercing",              SKILL_PIERCEING_WEAPONS,        30,     70,  0,   STRDEX },
{ "slashing",              SKILL_SLASHING_WEAPONS,         30,     70,  0,   STRDEX },
{ "stinging",              SKILL_STINGING_WEAPONS,         30,     85,  0,   STRDEX },
{ "whipping",              SKILL_WHIPPING_WEAPONS,         30,     85,  0,   STRDEX },
{ "soothing rememberance", SKILL_SONG_SOOTHING_REMEM,      31,    100,  0,   CONWIS },
{ "searching song",        SKILL_SONG_SEARCHING_SONG,      32,    100,  0,   INTWIS },
{ "submariner's anthem",   SKILL_SONG_SUBMARINERS_ANTHEM,  33,    100,  0,   CONINT },
{ "dischordant dirge",     SKILL_SONG_DISCHORDANT_DIRGE,   34,    100,  0,   CONINT },
{ "insane chant",          SKILL_SONG_INSANE_CHANT,        35,    100,  0,   CONINT },
{ "jig of alacrity",       SKILL_SONG_JIG_OF_ALACRITY,     38,    100,  0,   CONWIS },
{ "vigilant siren",        SKILL_SONG_VIGILANT_SIREN,      40,    100,  0,   CONWIS },
{ "forgetful rhythm",      SKILL_SONG_FORGETFUL_RHYTHM,    42,    100,  0,   CONINT },
{ "disarming limerick",    SKILL_SONG_DISARMING_LIMERICK,  43,    100,  0,   CONINT },
{ "astral chanty",         SKILL_SONG_ASTRAL_CHANTY,       45,    100,  0,   INTWIS },
{ "crushing crescendo",    SKILL_SONG_CRUSHING_CRESCENDO,  46,    100,  0,   DEXCON },
{ "shattering resonance",  SKILL_SONG_SHATTERING_RESO,     48,    100,  0,   CONINT },
{ "mountain king's charge",SKILL_SONG_MKING_CHARGE,        49,    100,  0,   DEXCON },
{ "hypnotic harmony",      SKILL_SONG_HYPNOTIC_HARMONY,    50,    100,  0,   CONINT },
{ "summoning song",        SKILL_SONG_SUMMONING_SONG,      50,    100,  0,   INTWIS },
{ "orchestrate",           SKILL_ORCHESTRATE,              51,    100,  0,   CONWIS },
{ "tumbling",              SKILL_TUMBLING,                 55,    100,  0,   STRDEX },
{ "\n",                    0,                              1,      0,   0,   0 }
};


// druid 3203 guildmaster - done and checked, Apoc
struct class_skill_defines u_skills[] = { // druid skills

//   Ability Name            Ability File              Level     Max  Group   Requisites
//   ------------            ------------              -----     ---  -----   ----------
{    "blue bird",            SPELL_BLUE_BIRD,            1,     100,  0,   STRDEX },  
{    "eyes of the owl",      SPELL_EYES_OF_THE_OWL,      2,      90,  0,   DEXWIS },
{    "cure light",           SPELL_CURE_LIGHT,           3,      85,  0,   INTWIS },
{    "shield block",         SKILL_SHIELDBLOCK,          5,      50,  0,   STRCON },
{    "attrition",            SPELL_ATTRITION,            6,     100,  0,   CONWIS },
{    "natures lore",         SKILL_NATURES_LORE,         7,     100,  0,   DEXWIS },
{    "create water",         SPELL_CREATE_WATER,         8,      90,  0,   DEXWIS },
{    "create food",          SPELL_CREATE_FOOD,          8,      90,  0,   DEXWIS },
{    "sense life",           SPELL_SENSE_LIFE,           10,     90,  0,   DEXWIS },
{    "weaken",               SPELL_WEAKEN,               11,    100,  0,   CONWIS },
{    "cure serious",         SPELL_CURE_SERIOUS,         12,     85,  0,   INTWIS },
{    "resist cold",          SPELL_RESIST_COLD,          13,     90,  0,   STRCON },
{    "camouflage",           SPELL_CAMOUFLAGE,           14,     90,  0,   STRCON },
{    "oaken fortitude",      SPELL_OAKEN_FORTITUDE,      15,    100,  0,   INTWIS },
{    "water breathing",      SPELL_WATER_BREATHING,      17,    100,  0,   CONINT },
{    "resist acid",          SPELL_RESIST_ACID,          18,     90,  0,   STRCON },
{    "stoneshield",          SPELL_STONE_SHIELD,         20,    100,  0,   CONINT },
{    "poison",               SPELL_POISON,               21,     90,  0,   CONWIS },
{    "staunchblood",         SPELL_STAUNCHBLOOD,         22,     90,  0,   INTWIS },
{    "cure critical",        SPELL_CURE_CRITIC,          23,     85,  0,   INTWIS },
{    "call familiar",        SPELL_SUMMON_FAMILIAR,      25,     90,  0,   DEXWIS },
{    "dismiss familiar",     SPELL_DISMISS_FAMILIAR,     26,     90,  0,   DEXWIS },
{    "debility",             SPELL_DEBILITY,             27,    100,  0,   CONWIS },
{    "drown",                SPELL_DROWN,                28,    100,  0,   STRDEX },
{    "entangle",             SPELL_ENTANGLE,             29,     90,  0,   CONWIS },
{    "whipping",             SKILL_WHIPPING_WEAPONS,     30,     60,  0,   STRDEX },
{    "crushing",             SKILL_CRUSHING_WEAPONS,     30,     60,  0,   STRDEX },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,     30,     60,  0,   STRDEX },
{    "resist energy",        SPELL_RESIST_ENERGY,        30,     90,  0,   STRCON },
{    "rapid mend",           SPELL_RAPID_MEND,           31,    100,  0,   INTWIS },
{    "herb lore",            SPELL_HERB_LORE,            32,     90,  0,   INTWIS },
{    "lighted path",         SPELL_LIGHTED_PATH,         33,    100,  0,   DEXWIS },
{    "curse",                SPELL_CURSE,                34,     90,  0,   CONWIS },
{    "sun ray",              SPELL_SUN_RAY,              35,    100,  0,   STRDEX },
{    "call lightning",       SPELL_CALL_LIGHTNING,       35,    100,  0,   STRDEX },
{    "control weather",      SPELL_CONTROL_WEATHER,      36,     90,  0,   CONINT },
{    "barkskin",             SPELL_BARKSKIN,             37,     90,  0,   STRCON },
{    "iron roots",           SPELL_IRON_ROOTS,           38,    100,  0,   STRCON },
{    "resist fire",          SPELL_RESIST_FIRE,          39,     90,  0,   STRCON },
{    "earthquake",           SPELL_EARTHQUAKE,           40,     90,  0,   CONINT },
{    "lightning shield",     SPELL_LIGHTNING_SHIELD,     41,    100,  0,   CONINT },
{    "blindness",            SPELL_BLINDNESS,            42,    100,  0,   CONWIS },
{    "forage",               SKILL_FORAGE,               43,     90,  0,   DEXWIS },
{    "spiritwalk",           SPELL_GHOSTWALK,            43,    100,  0,   CONINT },
{    "stoneskin",            SPELL_STONE_SKIN,           44,     90,  0,   STRCON },
{    "power heal",           SPELL_POWER_HEAL,           45,    100,  0,   INTWIS },
{    "forest meld",          SPELL_FOREST_MELD,          46,     90,  0,   DEXWIS },
{    "greater stoneshield",  SPELL_GREATER_STONE_SHIELD, 47,    100,  0,   CONINT },
{    "colour spray",         SPELL_COLOUR_SPRAY,         48,    100,  0,   STRDEX },
{    "summon",               SPELL_SUMMON,               49,    100,  0,   STRCON },
{    "conjure elemental",    SPELL_CONJURE_ELEMENTAL,    50,    100,  0,   CONINT },
{    "release elemental",    SPELL_RELEASE_ELEMENTAL,    50,    100,  0,   CONINT },
{    "icestorm",             SPELL_ICESTORM,             51,    100,  0,   STRDEX },
{    "brew",                 SKILL_BREW,                 55,    100,  0,   CONINT },
{    "\n",                   0,                          1,      0,   0,   0 }
};


// cleric 3021 guildmaster - done and checked, Apoc
struct class_skill_defines c_skills[] = { // cleric skills

//   Ability Name            Ability File           Level     Max  Group   Requisites
//   ------------            ------------           -----     ---  -----   ----------
{    "cure light",           SPELL_CURE_LIGHT,        1,      90,  0,   INTWIS },     
{    "cause light",          SPELL_CAUSE_LIGHT,       2,     100,  0,   STRINT },
{    "armor",                SPELL_ARMOR,             3,      90,  0,   CONWIS },
{    "continual light",      SPELL_CONT_LIGHT,        4,      85,  0,   DEXINT },
{    "detect poison",        SPELL_DETECT_POISON,     5,      90,  0,   DEXINT },
{    "know alignment",       SPELL_KNOW_ALIGNMENT,    5,     100,  0,   DEXINT },
{    "detect magic",         SPELL_DETECT_MAGIC,      6,      90,  0,   DEXINT },
{    "refresh",              SPELL_REFRESH,           7,      85,  0,   DEXWIS },
{    "bless",                SPELL_BLESS,             8,      90,  0,   DEXWIS },
{    "create water",         SPELL_CREATE_WATER,      9,      70,  0,   DEXWIS },
{    "create food",          SPELL_CREATE_FOOD,       9,      70,  0,   DEXWIS },
{    "cure serious",         SPELL_CURE_SERIOUS,      10,     90,  0,   INTWIS },
{    "cause serious",        SPELL_CAUSE_SERIOUS,     11,    100,  0,   STRINT },
{    "detect invisibility",  SPELL_DETECT_INVISIBLE,  12,     85,  0,   DEXINT },
{    "remove poison",        SPELL_REMOVE_POISON,     13,     90,  0,   INTWIS },
{    "dispel minor",         SPELL_DISPEL_MINOR,      14,     90,  0,   STRCON },
{    "dual wield",           SKILL_DUAL_WIELD,        15,     50,  0,   STRINT },
{    "remove blind",         SPELL_REMOVE_BLIND,      16,    100,  0,   INTWIS },
{    "sense life",           SPELL_SENSE_LIFE,        17,     85,  0,   DEXINT },
{    "sanctuary",            SPELL_SANCTUARY,         18,     90,  0,   CONWIS },
{    "remove curse",         SPELL_REMOVE_CURSE,      19,    100,  0,   INTWIS },
{    "cure critical",        SPELL_CURE_CRITIC,       20,     90,  0,   INTWIS },
{    "cause critical",       SPELL_CAUSE_CRITICAL,    21,    100,  0,   STRINT },
{    "remove paralysis",     SPELL_REMOVE_PARALYSIS,  22,    100,  0,   INTWIS },
{    "locate object",        SPELL_LOCATE_OBJECT,     23,     85,  0,   DEXINT },
{    "word of recall",       SPELL_WORD_OF_RECALL,    24,     90,  0,   DEXWIS },
{    "animate dead",         SPELL_ANIMATE_DEAD,      25,     90,  0,   STRCON },
{    "dismiss corpse",       SPELL_DISMISS_CORPSE,    26,     90,  0,   STRCON },
{    "group fly",            SPELL_GROUP_FLY,         27,    100,  0,   DEXWIS },
{    "heal",                 SPELL_HEAL,              28,     90,  0,   INTWIS },
{    "harm",                 SPELL_HARM,              29,     90,  0,   STRINT },
{    "bludgeoning",          SKILL_BLUDGEON_WEAPONS,  30,     70,  0,   STRINT },
{    "crushing",             SKILL_CRUSHING_WEAPONS,  30,     70,  0,   STRINT },
{    "heroes feast",         SPELL_HEROES_FEAST,      31,    100,  0,   DEXWIS },
{    "dispel evil",          SPELL_DISPEL_EVIL,       32,     90,  0,   STRCON },
{    "dispel good",          SPELL_DISPEL_GOOD,       33,     90,  0,   STRCON },
{    "iridescent aura",      SPELL_IRIDESCENT_AURA,   35,    100,  0,   CONWIS },
{    "protection from evil", SPELL_PROTECT_FROM_EVIL, 36,     90,  0,   CONWIS },
{    "protection from good", SPELL_PROTECT_FROM_GOOD, 37,     90,  0,   CONWIS },
{    "portal",               SPELL_PORTAL,            38,     90,  0,   DEXWIS },
{    "true sight",           SPELL_TRUE_SIGHT,        39,     90,  0,   DEXINT },
{    "full heal",            SPELL_FULL_HEAL,         40,    100,  0,   INTWIS },
{    "power harm",           SPELL_POWER_HARM,        41,    100,  0,   STRINT },
{    "resist magic",         SPELL_RESIST_MAGIC,      43,     85,  0,   CONWIS },
{    "resist energy",        SPELL_RESIST_ENERGY,     44,     85,  0,   CONWIS },
{    "dispel magic",         SPELL_DISPEL_MAGIC,      45,     90,  0,   STRCON },
{    "flamestrike",          SPELL_FLAMESTRIKE,       46,    100,  0,   STRINT },
{    "group recall",         SPELL_GROUP_RECALL,      47,    100,  0,   DEXWIS },
{    "heal spray",           SPELL_HEAL_SPRAY,        48,    100,  0,   STRCON },
{    "group sanctuary",      SPELL_GROUP_SANC,        49,    100,  0,   CONWIS },
{    "divine intervention",  SPELL_DIVINE_INTER,      50,    100,  0,   CONWIS },
{    "commune",              SKILL_COMMUNE,           51,    100,  0,   DEXWIS },
//{    "boneshield", 	     SPELL_BONESHIELD,        53,    100,  2,   STRCON },
//{    "silence", 	     SPELL_SILENCE,           53,    100,  1,   CONWIS },
{    "scribe",               SKILL_SCRIBE,            55,    100,  0,   DEXINT }, 
//{    "immunity", 	     SPELL_IMMUNITY,          57,    100,  2,   CONWIS },
//{    "atonement", 	     SPELL_ATONEMENT,         57,    100,  1,   STRCON },
//{    "channel", 	     SPELL_CHANNEL,           60,    100,  2,   INTWIS },
//{    "wrath of god", 	     SPELL_WRATH_OF_GOD,      60,    100,  1,   STRINT },
{    "\n",                   0,                        1,      0,  0,   0 }
};

// mage 3020 guildmaster - done and checked, Apoc
struct class_skill_defines m_skills[] = { // mage skills

//   Ability Name           Ability File           Level     Max  Group   Requisites   
//   ------------           ------------           -----     ---  -----   ----------   
{    "magic missile",       SPELL_MAGIC_MISSILE,     1,     100,  0,   STRINT }, 
{    "ventriloquate",       SPELL_VENTRILOQUATE,     2,     100,  0,   DEXWIS },
{    "clarity",             SPELL_CLARITY,           2,     100,  0,   DEXINT },
{    "detect magic",        SPELL_DETECT_MAGIC,      3,      90,  0,   INTWIS },
{    "detect invisibility", SPELL_DETECT_INVISIBLE,  4,      90,  0,   INTWIS },
{    "invisibility",        SPELL_INVISIBLE,         5,      90,  0,   DEXWIS },
{    "burning hands",       SPELL_BURNING_HANDS,     6,     100,  0,   STRINT },
{    "armor",               SPELL_ARMOR,             7,      85,  0,   CONWIS },
{    "continual light",     SPELL_CONT_LIGHT,        8,      90,  0,   STRCON },
{    "refresh",             SPELL_REFRESH,           9,      90,  0,   DEXWIS },
{    "lightning bolt",      SPELL_LIGHTNING_BOLT,    10,    100,  0,   STRINT },
{    "infravision",         SPELL_INFRAVISION,       11,     90,  0,   INTWIS },
{    "fly",                 SPELL_FLY,               12,    100,  0,   DEXINT },
{    "strength",            SPELL_STRENGTH,          13,     90,  0,   DEXINT },
{    "fear",                SPELL_FEAR,              15,     90,  0,   CONWIS },
{    "identify",            SPELL_IDENTIFY,          16,    100,  0,   INTWIS },
{    "locate object",       SPELL_LOCATE_OBJECT,     17,     90,  0,   INTWIS },
{    "call familiar",       SPELL_SUMMON_FAMILIAR,   18,     85,  0,   STRCON },
{    "dismiss familiar",    SPELL_DISMISS_FAMILIAR,  18,     85,  0,   STRCON },
{    "chill touch",         SPELL_CHILL_TOUCH,       20,     90,  0,   DEXWIS },
{    "shield",              SPELL_SHIELD,            21,    100,  0,   CONWIS },
{    "souldrain",           SPELL_SOULDRAIN,         22,    100,  0,   DEXWIS },
{    "ethereal focus",      SPELL_ETHEREAL_FOCUS,    24,    100,  0,   DEXWIS },
{    "dispel minor",        SPELL_DISPEL_MINOR,      25,     90,  0,   CONWIS },
{    "mass invisibility",   SPELL_MASS_INVISIBILITY, 26,    100,  0,   DEXWIS },
{    "firestorm",           SPELL_FIRESTORM,         27,    100,  0,   STRINT },
{    "portal",              SPELL_PORTAL,            28,     90,  0,   STRCON },
{    "fireball",            SPELL_FIREBALL,          29,    100,  0,   STRINT },
{    "focused repelance",   SKILL_FOCUSED_REPELANCE, 30,    100,  0,   CONWIS },
{    "piercing",            SKILL_PIERCEING_WEAPONS, 30,     50,  0,   DEXINT },
{    "bludgeoning",         SKILL_BLUDGEON_WEAPONS,  30,     50,  0,   DEXINT },
{    "resist magic",        SPELL_RESIST_MAGIC,      31,     90,  0,   CONWIS },
{    "haste",               SPELL_HASTE,             33,    100,  0,   DEXINT },
{    "true sight",          SPELL_TRUE_SIGHT,        34,     90,  0,   INTWIS },
{    "dispel magic",        SPELL_DISPEL_MAGIC,      35,     90,  0,   CONWIS },
{    "resist fire",         SPELL_RESIST_FIRE,       36,     70,  0,   CONWIS },
{    "wizard eye",          SPELL_WIZARD_EYE,        37,    100,  0,   INTWIS },
{    "teleport",            SPELL_TELEPORT,          38,    100,  0,   DEXWIS },
{    "stoneskin",           SPELL_STONE_SKIN,        39,     70,  0,   CONWIS },
{    "meteor swarm",        SPELL_METEOR_SWARM,      40,    100,  0,   STRCON },
{    "life leech",          SPELL_LIFE_LEECH,        41,    100,  0,   CONWIS },
{    "word of recall",      SPELL_WORD_OF_RECALL,    42,     90,  0,   DEXWIS },
{    "create golem",        SPELL_CREATE_GOLEM,      43,    100,  0,   STRCON },
{    "release golem",       SPELL_RELEASE_GOLEM,     43,    100,  0,   STRCON },
{    "mend golem",          SPELL_MEND_GOLEM,        44,    100,  0,   STRCON },
{    "hellstream",          SPELL_HELLSTREAM,        45,    100,  0,   STRINT },
{    "fireshield",          SPELL_FIRESHIELD,        47,    100,  0,   STRINT },
{    "paralyze",            SPELL_PARALYZE,          48,    100,  0,   DEXINT },
{    "solar gate",          SPELL_SOLAR_GATE,        49,    100,  0,   STRINT },
{    "spellcraft",          SKILL_SPELLCRAFT,        50,    100,  0,   INTWIS },
{    "imbue",               SKILL_IMBUE,             51,    100,  0,   INTWIS },
{    "elemental filter",    SKILL_ELEMENTAL_FILTER,  55,    100,  0,   DEXWIS },
{    "\n",                  0,                       1,      0,   0,   0 }
};

// End of abilities listings for each class.



const char *languages[] =
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

const char *race_types[] =
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

const char *race_abbrev[] =
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

const char 
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
    "NODISPEL",
    "POISONOUS",
    "NO_GOLD_BONUS",
    "\n"
};


const char *player_bits[] =
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
    "CLASS_TREE_A",
    "CLASS_TREE_B",
    "CLASS_TREE_C",
    "REMORTED",
    "\n"
};

const char *punish_bits[] =
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

const char *combat_bits[] =
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
    "Fleeing",
    "Shocked2",
    "CrushBlow",
    "\n"
};

const char *isr_bits[] =
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
const char *position_types[] =
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

const char *connected_types[] =
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
/* STAT# { MANA_REGEN/TICK, KI_REGEN/TICK, EASY_BONUS,   MEDIUM_BONUS, HARD_BONUS, PRAC_BONUS, MAGIC_RES, CONC_BONUS,	SPELL_DAM	
},*/
/*  0 */ {	-10,		-5,		-12,		-3,	 -1,		0,  	-6,	  -5,		-7,},
/*  1 */ {	-10,		-5,		-10,		-3,	 -1,		0,	-5,	  -5,		-6,},	
/*  2 */ {	-9,		-5,		-9,		-3,	 -1,		0,	-5,	  -4,		-5,},
/*  3 */ {	-8,		-4,		-8,		-3,	 -1,		0,	-4,	  -4,		-4,},
/*  4 */ {	-7,		-4,		-7,		-3,	 -1,		0,	-4,	  -3,		-3,},
/*  5 */ {	-6,		-3,		-6,		-3, 	 -1,		0,	-3,	  -3,		-3,},
/*  6 */ {	-5,		-3,		-5,		-2,	 -1,		0,	-3,	  -2,		-2},
/*  7 */ {	-4,		-2,		-4,		-2,	 -1,		0,	-2,	  -2,		-2},
/*  8 */ {	-3,		-2,		-3,		-1,	  0,		0,	-2,	  -1,		-2,},
/*  9 */ {	-2,		-1,		-2,		 0,	  0,		0,	-2,	  -1,		-1,},
/* 10 */ {	-1,		-1,		-1,		 0,	  0,		0,	-1,	   0,		-1,},
/* 11 */ {	 0,		 0,		 0,		 0,	  0,		0,	-1,	   0,		-1,},
/* 12 */ {	 0,		 0,		 0,		 0,	  0,		0,	-1,	   0,		0,},
/* 13 */ {	 0,		 0,		 0,		 0,	  0,		0,	 0,	   0,		0,},
/* 14 */ {	 1,		 0,		 0,		 0,	  0,		0,	 0,	   0,		0,},
/* 15 */ {	 1,		 1,		 0,		 0,	  0,		0,	 0,	   1,		0,},
/* 16 */ {	 2,		 1,		 0,		 0,	  0,		0,	 0,	   1,		1,},
/* 17 */ {	 2,		 1,		 1,		 0,	  0,		0,	 0,	   2,		1,},
/* 18 */ {	 3,		 2,		 1,		 0,	  0,		0,	 0,	   2,		1,},
/* 19 */ {	 4,		 2,		 2,		 0,	  0,		1,	 1,	   3,		2,},
/* 20 */ {	 5,		 2,		 2,		 1,	  0,		1,	 1,	   3,		2,},
/* 21 */ {	 6,		 3,		 2,		 1,	  1,		1,	 1,	   4,		3,},
/* 22 */ {	 7,		 3,		 3,		 1,	  1,		1,	 2,	   4,		4,},
/* 23 */ {	 8,		 3,		 3,		 2,	  1,		1,	 2,	   5,		5,},
/* 24 */ {	 9,		 4,		 4,		 2,	  1,		2,	 2,	   5,		6,},
/* 25 */ {	 10,		 4,		 4,		 2,	  2,		2,	 3,	   6,		7,},
/* 26 */ {	 11,		 5,		 4,		 3, 	  2,		2,	 3,	   6,		8,},
/* 27 */ {	 12,		 5,		 5,		 3,  	  2,		2,	 4,	   7,		9,},
/* 28 */ {	 13,		 6,		 5,		 4,	  2,		3,	 4,	   8,		10,},
/* 29 */ {	 14,		 7,		 6,		 4,	  3,		3,	 5,	   9,		11,},
/* 30 */ {	 15,		 8,		 7,		 4,	  3,		3,	 5,	  10,		12,},
};

// Wisdom Attribute Modifiers
const struct wis_app_type wis_app[] = {
/* STAT# { MANA_REGEN/TICK, KI_REGEN/TICK, PRACS/LEVEL BONUS, ENERGY_RES, CONC_BONUS,	SPELL_DAM 	},*/
/*  0 */ {	-10,		-9,		-1,		  -6,		-5,	-7,},
/*  1 */ {	-10,		-8,		-1,		  -5,		-5,	-6,},	
/*  2 */ {	-9,		-7,		-1,		  -5,		-4,	-5,},
/*  3 */ {	-8,		-6,		-1,		  -4,		-4,	-4,},
/*  4 */ {	-7,		-5,		-1,		  -4,		-3,	-3,},
/*  5 */ {	-6,		-4,		-1,		  -3,		-3,	-3,},
/*  6 */ {	-5,		-3,		 0,		  -3,		-2,	-2,},
/*  7 */ {	-4,		-2,		 0,		  -2,		-2,	-2,},
/*  8 */ {	-3,		-2,		 0,		  -2,		-1,	-2,},
/*  9 */ {	-2,		-1,		 0,		  -2,		-1,	-1,},
/* 10 */ {	-1,		-1,		 0,		  -1,		 0,	-1,},
/* 11 */ {	 0,		 0,		 0,		  -1,		 0,	-1,},
/* 12 */ {	 0,		 0,		 0,		  -1,		 0,	0,},
/* 13 */ {	 0,		 0,		 0,		   0,		 0,	0,},
/* 14 */ {	 1,		 0,		 1,		   0,		 0,	0,},
/* 15 */ {	 1,		 1,		 1,		   0,		 1,	0,},
/* 16 */ {	 2,		 1,		 1,		   0,		 1,	1,},
/* 17 */ {	 2,		 1,		 2,		   0,		 2,	1,},
/* 18 */ {	 3,		 2,		 2,		   0,		 2,	1,},
/* 19 */ {	 4,		 2,		 3,		   1,		 3,	2,},
/* 20 */ {	 5,		 2,		 3,		   1,		 3,	2,},
/* 21 */ {	 6,		 3,		 4,		   1,		 4,	3,},
/* 22 */ {	 7,		 3,		 4,		   2,		 4,	4,},
/* 23 */ {	 8,		 3,		 5,		   2,		 5,	5,},
/* 24 */ {	 9,		 4,		 5,		   2,		 5,	6,},
/* 25 */ {	 10,		 4,		 6,		   3,		 6,	7,},
/* 26 */ {	 11,		 5,		 6,		   3,		 6,	8,},
/* 27 */ {	 12,		 5,		 7,		   4,		 7,	9,},
/* 28 */ {	 13,		 6,		 7,		   4,		 8,	10,},
/* 29 */ {	 14,		 7,		 8,		   5,		 9,	11,},
/* 30 */ {	 15,		 8,		 8,		   5,		10,	12,},
};

// Dexterity Attribute Modifiers
const struct dex_app_type dex_app[] = {
/* STAT# { TO_HIT_ BONUS, AC_BONUS, MOVE_GAIN/LEVEL, FIRE_RES	},*/
/*  0 */ {	-5,	     30,	-4,		-6,	},
/*  1 */ {	-4,	     27,	-4,     	-5,	},
/*  2 */ {	-3,	     24,	-3,		-5,	},
/*  3 */ {	-3,	     21,	-3,		-4,	},
/*  4 */ {	-2,	     18,	-3,		-4,	},
/*  5 */ {	-2,	     15,	-2,		-3,	},
/*  6 */ {	-2,	     12,	-2,		-3,	},
/*  7 */ {	-2,	      9,	-2,		-2,	},
/*  8 */ {	-1,	      6,	-1,		-2,	},
/*  9 */ {	-1,	      3,	-1,		-2,	},
/* 10 */ {	-3,	      0,	-1,		-1,	},
/* 11 */ {	-1,	     -3,	 0,		-1,	},
/* 12 */ {	 0, 	     -6,	 0,		-1,	},
/* 13 */ {	 0,	     -9,	 0,		 0,	},
/* 14 */ {	 0,	    -12,	 0,		 0,	},
/* 15 */ {	 0,	    -15,	 0,		 0,	},
/* 16 */ {	 1,	    -18,	 1,		 0,	},
/* 17 */ {	 1,	    -21,	 1,		 0,	},
/* 18 */ {	 1,	    -24,	 1,		 0,	},
/* 19 */ {	 2,	    -27,	 1,		 1,	},
/* 20 */ {	 2,	    -30,	 1,		 1,	},
/* 21 */ {	 3,	    -33,	 2,		 1,	},
/* 22 */ {	 4,	    -36,	 2,		 2,	},
/* 23 */ {	 5,	    -39,	 2,		 2,	},
/* 24 */ {	 6,	    -42,	 2,		 2,	},
/* 25 */ {	 7,	    -45,	 2,		 3,	},
/* 26 */ {	 8,	    -48,	 3,		 3,	},
/* 27 */ {	 9,	    -51,	 3,		 4,	},
/* 28 */ {	 10,	    -54,	 3,		 4,	},
/* 29 */ {	 11,	    -57,	 3,		 5,	},
/* 30 */ {	 12,	    -60,	 3,		 5,	},
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
/*  2 */{    750,     7,  1,  1,   98,     500},
/*  3 */{   1000,    10,  1,  1,   96,     750},
/*  4 */{   2000,    20,  1,  2,   94,    1000},
/*  5 */{   3000,    30,  2,  2,   92,    1250},
/*  6 */{   4000,    40,  2,  2,   90,    1500},
/*  7 */{   5000,    50,  2,  3,   87,    1750},
/*  8 */{   6500,    65,  2,  3,   84,    2000},
/*  9 */{   8000,    80,  3,  3,   81,    2250},
/* 10 */{  11000,   100,  3,  4,   78,    2500},
/* 11 */{  14000,   120,  3,  4,   74,    3000},
/* 12 */{  17000,   140,  4,  5,   70,    3500},
/* 13 */{  20000,   160,  4,  5,   68,    4000},
/* 14 */{  30000,   180,  4,  6,   64,    4500},
/* 15 */{  40000,   200,  5,  7,   60,    5000},
/* 16 */{  50000,   225,  5,  9,   56,    5500},
/* 17 */{  60000,   250,  5, 11,   52,    6000},
/* 18 */{  70000,   275,  6, 12,   48,    6500},
/* 19 */{  80000,   300,  6, 13,   44,    7000},
/* 20 */{  90000,   325,  7, 15,   40,    7500},
/* Lowbie Range Above Here */
/* 21 */{ 100000,   350,  7, 16,   36,    8000},
/* 22 */{ 110000,   375,  8, 17,   32,    8750},
/* 23 */{ 120000,   400,  8, 18,   28,    9500},
/* 24 */{ 130000,   425,  9, 19,   24,   10250},
/* 25 */{ 140000,   450, 10, 20,   20,   11000},
/* 26 */{ 150000,   475, 10, 20,   16,   11750},
/* 27 */{ 160000,   500, 11, 21,   12,   12500},
/* 28 */{ 170000,   525, 11, 21,    8,   13250},
/* 29 */{ 180000,   550, 12, 22,    4,   14000},
/* 30 */{ 190000,   575, 12, 22,    0,   15000},
/* 31 */{ 200000,   600, 13, 23,   -4,   16000},
/* 32 */{ 220000,   640, 13, 23,   -8,   17000},
/* 33 */{ 240000,   680, 14, 24,  -12,   18000},
/* 34 */{ 260000,   720, 14, 24,  -16,   19000},
/* 35 */{ 280000,   760, 15, 25,  -20,   20000},
/* 36 */{ 300000,   800, 15, 26,  -24,   21500},
/* 37 */{ 325000,   850, 16, 27,  -28,   23000},
/* 38 */{ 350000,   900, 16, 28,  -32,   24500},
/* 39 */{ 375000,   950, 17, 29,  -36,   26000},
/* Midbie Range Above Here */
/* 40 */{ 400000,  1000, 17, 30,  -40,   27500},
/* 41 */{ 425000,  1050, 18, 31,  -45,   30000},
/* 42 */{ 450000,  1100, 18, 32,  -50,   32500},
/* 43 */{ 475000,  1150, 19, 33,  -55,   35000},
/* 44 */{ 500000,  1200, 19, 34,  -60,   37500},
/* 45 */{ 550000,  1250, 20, 35,  -65,   40000},
/* 46 */{ 600000,  1300, 20, 36,  -70,   42500},
/* 47 */{ 650000,  1350, 21, 37,  -75,   45000},
/* 48 */{ 700000,  1400, 21, 38,  -80,   47500},
/* 49 */{ 750000,  1450, 22, 39,  -85,   50000},
/* 50 */{ 800000,  1500, 22, 40,  -90,   55000},
/* 51 */{1000000,  1600, 23, 41,  -95,   60000},
/* 52 */{1050000,  1700, 23, 42, -100,   65000},
/* 53 */{1100000,  1800, 24, 43, -105,   70000},
/* 54 */{1150000,  1900, 24, 44, -110,   75000},
/* 55 */{1200000,  2000, 25, 45, -115,   80000},
/* 56 */{1250000,  2100, 25, 46, -120,   85000},
/* 57 */{1300000,  2200, 26, 47, -125,   90000},
/* 58 */{1350000,  2300, 26, 48, -130,   95000},
/* 59 */{1400000,  2400, 27, 49, -135,  100000},
/* 60 */{1500000,  2500, 27, 50, -140,  105000},
/* 61 */{1600000,  2600, 28, 51, -146,  110000},
/* 62 */{1700000,  2700, 28, 52, -154,  115000},
/* 63 */{1800000,  2800, 29, 53, -162,  120000},
/* 64 */{1900000,  2900, 29, 54, -170,  125000},
/* 65 */{2000000,  3000, 30, 55, -178,  130000},
/* 66 */{2100000,  3200, 30, 56, -186,  135000},
/* 67 */{2200000,  3400, 31, 57, -194,  140000},
/* 68 */{2300000,  3600, 31, 58, -202,  145000},
/* 69 */{2400000,  3800, 32, 59, -210,  150000},
/* 70 */{2500000,  4000, 32, 60, -218,  155000},
/* 71 */{2600000,  4250, 33, 61, -226,  160000},
/* 72 */{2700000,  4500, 33, 62, -234,  165000},
/* 73 */{2800000,  4750, 34, 63, -242,  170000},
/* 74 */{2900000,  5000, 34, 64, -250,  175000},
/* High Level and 50+ Range Above Here */
/* 75 */{4000000,  6000, 35, 65, -260,  250000},
/* 76 */{4100000,  6250, 36, 66, -270,  275000},
/* 77 */{4200000,  6500, 37, 67, -280,  300000},
/* 78 */{4300000,  6750, 38, 68, -290,  325000},
/* 79 */{4400000,  7000, 39, 69, -300,  350000},
/* 80 */{4600000,  7500, 40, 70, -310,  375000},
/* 81 */{4800000,  8000, 41, 71, -320,  400000},
/* 82 */{5000000,  8500, 42, 72, -330,  425000},
/* 83 */{5200000,  9000, 43, 73, -340,  450000},
/* 84 */{5400000,  9500, 44, 74, -350,  475000},
/* 85 */{5600000, 10000, 45, 75, -360,  500000},
/* 86 */{5800000, 10250, 46, 76, -370,  525000},
/* 87 */{6000000, 10500, 47, 77, -380,  550000},
/* 88 */{6200000, 11000, 48, 78, -390,  575000},
/* 89 */{6400000, 11500, 49, 79, -400,  600000},
/* ------ EQ Mob Range Starts Here ------    */
/* 90 */{6500000, 12000, 60, 80, -510, 1500000},
/* 91 */{6600000, 12500, 61, 81, -520, 1750000},
/* 92 */{6700000, 13000, 62, 82, -530, 2000000},
/* 93 */{6800000, 13500, 63, 83, -540, 2250000},
/* 94 */{7000000, 14000, 64, 84, -550, 2500000},
/* 95 */{7250000, 15000, 65, 85, -560, 2750000},
/* 96 */{7500000, 16000, 66, 86, -570, 3000000},
/* 97 */{7750000, 17000, 67, 87, -580, 3250000},
/* 98 */{8000000, 18000, 68, 88, -590, 3500000},
/* 99 */{8250000, 19000, 69, 89, -600, 3750000},
/*100 */{8500000, 20000, 70, 90, -610, 4000000},
/*101 */{8750000, 21000, 71, 91, -620, 4250000},
/*102 */{9000000, 22000, 72, 92, -630, 4500000},
/*103 */{9250000, 23000, 73, 93, -640, 4750000},
/*104 */{9500000, 24000, 74, 94, -650, 5000000},
/*105 */{9750000, 25000, 75, 95, -675, 5250000},
/*106 */{10000000,26000, 76, 96, -700, 5500000},
/*107 */{10250000,27000, 77, 97, -725, 5750000},
/*108 */{10500000,28000, 78, 98, -750, 6000000},
/*109 */{10750000,29000, 79, 99, -775, 6250000},
/*110 */{11000000,30000, 80,100, -800, 6500000}
};

const char *reserved[] = {
    "holy aura timer",
    "natural select timer",
    "divine intervention timer",
    "cannot cast timer",
    "combat mastery timer",
    "champion flag timer",
    "triage timer",
    "smite timer",
    "make camp timer",
    "leadership bonus",
    "perseverance bonus",
    "deceit reuse timer",
    "ferocity reuse timer",
    "tactics reuse timer",
    "concentration loss fixer",
    "onslaught reuse timer",
    "ki transfer reuse timer",
    "brew reuse timer",
    "scribe reuse timer",
    "profession",
    "lilith ring reuse timer",
    "dawn reuse timer",
    "durendal resue timer"
};

bestowable_god_commands_type bestowable_god_commands[] =
{
{ "impchan",	COMMAND_IMP_CHAN, false },
{ "snoop",	COMMAND_SNOOP, false },
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
{ "sedit",      COMMAND_SEDIT, false },
{ "punish",     COMMAND_PUNISH, false },
{ "sqedit",     COMMAND_SQEDIT, false },
{ "hedit",      COMMAND_HEDIT, false },
{ "opstat",	COMMAND_OPSTAT, false },
{ "opedit",	COMMAND_OPEDIT, false },
{ "force",	COMMAND_FORCE, false },
{ "string",	COMMAND_STRING, false },
{ "stat",	COMMAND_STAT, false },
{ "sqsave",	COMMAND_SQSAVE, false },
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
{ "testport", COMMAND_TESTPORT, false },
{ "testuser", COMMAND_TESTUSER, false },
{ "remort", COMMAND_REMORT, true },
{ "testhit", COMMAND_TESTHIT, true },
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
