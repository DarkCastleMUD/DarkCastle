/***************************************************************************
 *  file: spells.c , Basic routines and parsing            Part of DIKUMUD *
 *  Usage : Interpreter of spells                                          *
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
/* $Id: spells.cpp,v 1.32 2002/12/27 01:45:08 pirahna Exp $ */

extern "C"
{
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <character.h>
#include <race.h>
#include <levels.h>
#include <spells.h>
#include <magic.h>
#include <player.h>
#include <isr.h>
#include <utility.h>
#include <fight.h>
#include <mobile.h>
#include <room.h>
#include <db.h>
#include <handler.h>
#include <connect.h>
#include <interp.h>
#include <act.h>
#include <returnvals.h> 

// Global data 

extern CWorld world;
 
extern CHAR_DATA *character_list;
extern char *spell_wear_off_msg[];

// Functions used in spells.C
int spl_lvl(int lev);

// Extern procedures 
int do_fall(CHAR_DATA *ch, short dir);
void remove_memory(CHAR_DATA *ch, char type);
void add_memory(CHAR_DATA *ch, char *victim, char type);
void make_dust(CHAR_DATA * ch);

#if(0)
    byte        beats;                  /* Waiting time after spell     */
    byte        minimum_position;       /* Position for caster          */
    ubyte       min_usesmana;           /* Mana used                    */
    sh_int      targets;                /* Legal targets                */
    SPELL_FUN * spell_pointer;          /* Function to call             */
#endif

struct spell_info_type spell_info [ ] =
{
    { /* 00 */ /* THIS WAS MISSING... Arrays start a 0!!! CGT */
         0, 0, 0, 0, 0
    },

    { /* 01 */
	12, POSITION_STANDING,  5, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_armor
    },

    { /* 02 */
	12, POSITION_FIGHTING, 35, TAR_SELF_ONLY, cast_teleport
    },

    { /* 03 */
	12, POSITION_STANDING, 5, TAR_OBJ_INV|TAR_OBJ_EQUIP|TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_bless
    },

    { /* 04 */
	12, POSITION_FIGHTING,  5, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_blindness
    },

    { /* 05 */
	12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_burning_hands
    },

    { /* 06 */
	12, POSITION_FIGHTING, 40, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_call_lightning
    },

    { /* 07 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_SELF_NONO, cast_charm_person
    },

    { /* 08 */
	12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_chill_touch
    },

    { /* 09 */ 
         /* 12,POSITION_STANDING,15, 32, 40, TAR_CHAR_ROOM, cast_clone); */
         0, 0, 0, 0, 0
    },

    { /* 10 */
	12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_colour_spray
    },

    { /* 11 */
	12, POSITION_STANDING, 25, TAR_IGNORE, cast_control_weather
    },

    { /* 12 */
	12, POSITION_STANDING, 5, TAR_IGNORE, cast_create_food
    },

    { /* 13 */
	12, POSITION_STANDING, 5, TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_create_water
    },

    { /* 14 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_cure_blind
    },

    { /* 15 */
	12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_cure_critic
    },

    { /* 16 */
	12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_cure_light
    },

    { /* 17 */
	12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_OBJ_ROOM|TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_curse
    },

    { /* 18 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_detect_evil
    },

    { /* 19 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_detect_invisibility
    },

    { /* 20 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_detect_magic
    },

    { /* 21 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_detect_poison
    },

    { /* 22 */
	12, POSITION_FIGHTING, 25, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_dispel_evil
    },

    { /* 23 */
	12, POSITION_FIGHTING, 15, TAR_IGNORE, cast_earthquake
    },

    { /* 24 */
	24, POSITION_STANDING, 100, TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_enchant_weapon
    },

    { /* 25 */
	12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_energy_drain
    },

    { /* 26 */
	12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_fireball
    },

    { /* 27 */
	12, POSITION_FIGHTING, 33, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_harm
    },

    { /* 28 */
	12, POSITION_FIGHTING, 50, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_heal
    },

    { /* 29 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM|TAR_OBJ_EQUIP|TAR_SELF_DEFAULT, cast_invisibility
    },

    { /* 30 */
	12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_lightning_bolt
    },

    { /* 31 */
	12, POSITION_STANDING, 20, TAR_OBJ_WORLD, cast_locate_object
    },

    { /* 32 */
	12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_magic_missile
    },

    { /* 33 */
	12, POSITION_STANDING, 10, TAR_CHAR_ROOM|TAR_SELF_NONO|TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_poison
    },

    { /* 34 */
	12, POSITION_STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_protection_from_evil
    },

    { /* 35 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_EQUIP|TAR_OBJ_ROOM, cast_remove_curse
    },

    { /* 36 */
	12, POSITION_STANDING, 75, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_sanctuary
    },

    { /* 37 */
	12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_shocking_grasp
    },

    { /* 38 */
	12, POSITION_STANDING, 15, TAR_CHAR_ROOM, cast_sleep
    },

    { /* 39 */
	12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_strength
    },

    { /* 40 */
	12, POSITION_STANDING, 50, TAR_CHAR_WORLD|TAR_SELF_NONO, cast_summon
    },

    { /* 41 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_OBJ_ROOM|TAR_SELF_NONO, cast_ventriloquate
    },

    { /* 42 */
	12, POSITION_FIGHTING, 50, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_word_of_recall
    },

    { /* 43 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM|TAR_SELF_DEFAULT, cast_remove_poison
    },

    { /* 44 */
	12, POSITION_STANDING, 15, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_sense_life
    },

    { /* 45 */
	12, POSITION_STANDING, 50, TAR_IGNORE, cast_summon_familiar
    },

    { /* 46 */
	 12, POSITION_STANDING, 30, TAR_IGNORE, cast_lighted_path
    },

    { /* 47 */
	 12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_resist_acid
    },

    { /* 48 */
	 12, POSITION_FIGHTING, 40, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_sun_ray
    },

    { /* 49 */
	 12, POSITION_STANDING, 30, TAR_CHAR_ROOM, cast_rapid_mend
    },

    { /* 50 */
	12, POSITION_STANDING, 150, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_acid_shield
    },

    { /* 51 */
	 12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_water_breathing
    },

    { // 52
	 12, POSITION_STANDING, 20, TAR_IGNORE, cast_globe_of_darkness
    },

    { // 53
	24, POSITION_STANDING, 12, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM, cast_identify
    },
    
    { // 54
        24, POSITION_STANDING, 20, TAR_OBJ_ROOM, cast_animate_dead
    },

    { // 55
	12, POSITION_FIGHTING, 7, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_fear
    },

    { /* 56 */
	12, POSITION_STANDING, 10, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_fly
    },

    { /* 57 */
	12, POSITION_STANDING, 7, TAR_NONE_OK|TAR_OBJ_INV, cast_cont_light
    },

    { /* 58 */
	12, POSITION_STANDING, 9, TAR_CHAR_ROOM, cast_know_alignment
    },

    { /* 59 */
	12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_dispel_magic
    },

    { /* 60 */
	24, POSITION_STANDING, 33, TAR_CHAR_WORLD, cast_conjure_elemental
    },

    { /* 61 */
	12, POSITION_FIGHTING, 17, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_cure_serious
    },

    { /* 62 */
	12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_cause_light
    },

    { /* 63 */
	12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_cause_critical
    },

    { /* 64 */
	12, POSITION_FIGHTING, 17, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_cause_serious
    },

    { /* 65 */
	12, POSITION_FIGHTING, 33, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_flamestrike
    },

    { /* 66 */
	12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_stone_skin
    },

    { /* 67 */
	12, POSITION_STANDING, 12, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_shield
    },

    { /* 68 */
	12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_weaken
    },

    { /* 69 */
	24, POSITION_STANDING, 33, TAR_IGNORE, cast_mass_invis
    },

    { /* 70 */
	12, POSITION_FIGHTING, 45, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_acid_blast
    },

    { /* 71 */
	12, POSITION_STANDING, 50, TAR_CHAR_WORLD, cast_portal
    },

    { /* 72 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_infravision
    },

    { /* 73 */
	12, POSITION_STANDING, 12, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_refresh
    },

    { /* 74 */
	12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_haste
    },

    { /* 75 */
	12, POSITION_FIGHTING, 25, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_dispel_good
    },

    { /* 76 */
	12, POSITION_FIGHTING, 80, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_hellstream
    },

    { /* 77 */
	12, POSITION_FIGHTING, 75, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_power_heal 
    },

    { /* 78 */
	12, POSITION_FIGHTING, 80, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_full_heal
    },

    { /* 79 */
	12, POSITION_FIGHTING, 60, TAR_IGNORE, cast_firestorm
    },

    { /* 80 */
	12, POSITION_FIGHTING, 40, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_power_harm
    },

    { /* 81 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_detect_good
    },

    { /* 82 */
	12, POSITION_FIGHTING, 33, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_vampiric_touch
    },

    { /* 83 */
	12, POSITION_FIGHTING, 40, TAR_IGNORE, cast_life_leech
    },

    { /* 84 */
	12, POSITION_FIGHTING, 25, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_paralyze
    },

    { /* 85 */
	12, POSITION_STANDING, 5, TAR_CHAR_ROOM, cast_remove_paralysis
    },

    { /* 86 */
	12, POSITION_STANDING, 150, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_fireshield
    },

    { /* 87 */
	12, POSITION_FIGHTING, 40, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_meteor_swarm
    },

    { /* 88 */
	12, POSITION_STANDING, 20, TAR_CHAR_WORLD, cast_wizard_eye
    },

    { /* 89 */
	12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_true_sight
    },

    { /* 90 */
	12, POSITION_RESTING, 0, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_mana
    },

    { /* 91 */
	12, POSITION_FIGHTING, 200, TAR_IGNORE, cast_solar_gate
    },

    { /* 92 */
	12, POSITION_STANDING, 33, TAR_IGNORE, cast_heroes_feast
    },

    { /* 93 */
	12, POSITION_STANDING, 100, TAR_IGNORE, cast_heal_spray
    },

    { /* 94 */
	12, POSITION_STANDING, 200, TAR_IGNORE, cast_group_sanc
    },

    { /* 95 */
     12, POSITION_STANDING, 100, TAR_IGNORE, cast_group_recall
    },

    { /* 96 */
	12, POSITION_STANDING, 40, TAR_IGNORE, cast_group_fly
    },

    { /* 97 */
	12, POSITION_STANDING, 250, TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_enchant_armor
    },

    { /* 98 */
	12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_resist_fire
    },

    { /* 99 */
	12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_resist_cold
    },

    { /* 100 */
	12, POSITION_FIGHTING, 5, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_bee_sting
    }, 

    { /* 101 */
	12, POSITION_FIGHTING, 15, TAR_IGNORE, cast_bee_swarm
    }, 

    { /* 102 */
	12, POSITION_FIGHTING, 50, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_creeping_death
    }, 

    { /* 103 */
    	12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_barkskin
    }, 

    { /* 104 */
	12, POSITION_FIGHTING, 50, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_herb_lore
    }, 

    { /* 105 */
    	12, POSITION_STANDING, 75, TAR_CHAR_WORLD, cast_call_follower
    }, 

    { /* 106 */
	12, POSITION_FIGHTING, 10, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_entangle
    }, 

    { /* 107 */
    	12, POSITION_STANDING, 5, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_eyes_of_the_owl
    },

    { /* 108 */
    	12, POSITION_STANDING, 30, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_feline_agility
    }, 

    { /* 109 */
    	12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_forest_meld
    },
    
    { // 110
    	12, POSITION_STANDING, 150, TAR_IGNORE, cast_companion
    },

    { // 111
    	12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_drown
    },

    { // 112
    	12, POSITION_FIGHTING, 30, TAR_FIGHT_VICT, cast_howl
    },

    { // 113
    	12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_souldrain
    },

    { // 114
    	12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_sparks
    },

    { // 115
        12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_camouflague
    },

    { // 116
        12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_farsight
    },

    { // 117
        12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_freefloat
    },

    { // 118
        12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_insomnia
    },

    { // 119
        12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_shadowslip
    },
    
    { // 120
        12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_resist_energy
    },

    { // 121
         12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY, cast_staunchblood
    },

    { // 122
	12, POSITION_STANDING, 255, TAR_CHAR_ROOM, cast_create_golem
    },

    { // 123  this spell isn't castable but this has to be here.
        12, POSITION_STANDING, 1, TAR_IGNORE, spell_reflect
    },

    { /* 124 */
	12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_OBJ_ROOM, cast_dispel_minor
    },

    { // 125
        12, POSITION_STANDING, 5, TAR_IGNORE, spell_release_golem
    },

    { // 126
        12, POSITION_FIGHTING, 30, TAR_IGNORE, spell_beacon
    },

    { // 127
        12, POSITION_STANDING, 30, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_stone_shield
    },

    { // 128
        12, POSITION_STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_greater_stone_shield
    },

    { // 129
        12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_iron_roots
    },

    { // 130
        12, POSITION_STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_eyes_of_the_eagle
    },

    { // 131
        12, POSITION_STANDING, 40, TAR_CHAR_ROOM, NULL
    },

    { // 132
        12, POSITION_FIGHTING, 25, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_ice_shards
    },

    { // 133
	12, POSITION_STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_lightning_shield
    },

    { // 134
	12, POSITION_FIGHTING, 10, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_blue_bird
    },

    { // 135
	12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_debility
    },

    { // 136
	12, POSITION_FIGHTING, 30, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_attrition
    }

};

char *skills[]=
{
  "trip",    // 0
  "dodge",
  "second_attack",
  "disarm",
  "third_attack",
  "parry",
  "deathstroke",
  "circle",
  "berserk",
  "headbutt",
  "eagle claw",     // 10
  "quivering palm",
  "palm",
  "stalk",
  "UNUSED",
  "dual_backstab",
  "hitall",
  "stun",
  "scan",
  "consider",
  "switch",
  "redirect",
  "ambush",
  "forage",
  "tame",
  "track", // 25
  "skewer",
  "slip",
  "retreat",
  "rage", // 29
  "battlecry",
  "archery",
  "riposte",
  "lay hands",
  "insane chant",
  "glitter dust",
  "sneak",
  "hide",
  "steal",
  "backstab",
  "pick_lock", // 40
  "kick",
  "bash",
  "rescue",
  "blood_fury",
  "dual_wield",
  "harm_touch",
  "shield_block",
  "blade_shield",
  "pocket",
  "guard",    // 50
  "frenzy",
  "blindfighting",
  "focused_repelance",
  "vital_strike",
  "crazed_assault",
  "divine_protection",
  "bludgeoning_weapons",
  "piercing_weapons",
  "slashing_weapons",
  "whipping_weapons",
  "crushing_weapons",
  "two_handed_weapons",
  "\n"
};

char *spells[]=
{
   "armor",               /* 1 */
   "teleport",
   "bless",
   "blindness",
   "burning hands",
   "call lightning",
   "charm person",
   "chill touch",
   "clone",  
   "colour spray",
   "control weather",     /* 11 */
   "create food",
   "create water",
   "cure blind",
   "cure critical",
   "cure light",
   "curse",
   "detect evil",
   "detect invisibility",
   "detect magic",
   "detect poison",       /* 21 */
   "dispel evil",
   "earthquake",
   "enchant weapon",
   "energy drain",
   "fireball",
   "harm",
   "heal",
   "invisibility",
   "lightning bolt",
   "locate object",      /* 31 */
   "magic missile",
   "poison",
   "protection from evil",
   "remove curse",
   "sanctuary",
   "shocking grasp",
   "sleep",
   "strength",
   "summon",
   "ventriloquate",      /* 41 */
   "word of recall",
   "remove poison",
   "sense life",         /* 44 */
   "summon familiar",        /* 45 */
   "lighted path",
   "resist acid",
   "sun ray",
   "rapid mend",
   "acid shield",         /* 50 */
   "water breathing",
   "globe of darkness",
   "identify",
   "animate dead",
   "fear",        
   "fly",
   "continual light",
   "know alignment",
   "dispel magic",
   "conjure elemental",  /* 60 */
   "cure serious",
   "cause light",
   "cause critical",
   "cause serious",
   "flamestrike",        /* 65 */
   "stone skin",
   "shield",
   "weaken",
   "mass invisibility",
   "acid blast",         /* 70 */
   "portal",
   "infravision",
   "refresh",
    "haste",
   "dispel good",
   "hellstream",
   "power heal",
   "full heal",
   "firestorm",
   "power harm", /* 80 */
   "detect good",
   "vampiric touch",
   "life leech",
   "paralyze",
   "remove paralysis",
   "fireshield",
   "meteor swarm",
   "wizard eye",
   "true sight",
   "mana", /* 90 */
   "solar gate",
   "heroes feast",
   "heal spray",
   "group sanctuary",
   "group recall",
   "group fly",
   "enchant armor",
   "resist fire",
   "resist cold", 
   "bee sting",      // 100
   "bee swarm",		
   "creeping death",
   "barkskin",
   "herb lore",
   "call follower",
   "entangle",
   "eyes of the owl",
   "feline agility",
   "forest meld",
   "companion",  // 110
   "drown",
   "howl",
   "souldrain",
   "sparks",     // 114
   "camouflague",
   "farsight",
   "freefloat",
   "insomnia",
   "shadowslip",
   "resist energy",
   "staunchblood",
   "create golem",
   "reflect",
   "dispel minor",
   "release golem",
   "beacon",
   "stone shield",
   "greater stone shield",
   "iron roots",
   "eyes of the eagle",
   "unused",
   "ice shards",
   "lightning shield",
   "blue bird",
   "debility",
   "attrition",
   "\n"
};

int use_mana( CHAR_DATA *ch, int sn )
{
    int base = spell_info[sn].min_usesmana;

// TODO - if we want mana to be modified by anything, we'll need to put something
// here.  Since the "min_level_x" stuff doesn't exist anymore, and i'm too lazy
// right now to go through and have it search the class skill lists, I'll just let
// people use it at the base mana from the level they get it.  I doubt they will
// complain any.  I think I like it better that way anyway. - pir

    return base;
/*
    int divisor;

    divisor = 2 + GET_LEVEL(ch);
    if ( GET_CLASS(ch) == CLASS_CLERIC )
	divisor -= spell_info[sn].min_level_cleric;
    else
    if ( GET_CLASS(ch) == CLASS_MAGIC_USER )
	divisor -= spell_info[sn].min_level_magic;
    else 
    if ( GET_CLASS(ch) == CLASS_ANTI_PAL )
        divisor -= spell_info[sn].min_level_anti;
    else
    if (GET_CLASS(ch) == CLASS_PALADIN)
        divisor -= spell_info[sn].min_level_paladin;

    else // it's a ranger 
	divisor -= spell_info[sn].min_level_ranger;
    if ( divisor != 0 )
	return MAX( base, 100 / divisor );
    else
	return MAX( base, 20 );
*/
}


void affect_update( void )
{
    static struct affected_type *af, *next_af_dude;
    static CHAR_DATA *i, * i_next;
    short dir;
    void update_char_objects( CHAR_DATA *ch ); /* handler.c */

    for (i = character_list; i; i = i_next) { 
      i_next = i->next;
      if(!IS_NPC(i) && !(i->desc))
        continue; 
      for (af = i->affected; af; af = next_af_dude) {
	next_af_dude = af->next;

        // This doesn't really belong here, but it beats creating an "update" just for it.
        // That way we don't have to traverse the entire list all over again
        if(!IS_NPC(i))
          update_char_objects(i);

	if (af->duration > 1)
	  af->duration--;
	else if(af->duration == 1) {
          // warnings for certain spells
          switch(af->type) {
            case SPELL_WATER_BREATHING:
              send_to_char("You feel the magical hold of your gills about to give way.\r\n", i);
              break;
            default: break;
          }
          af->duration--;
        }
	else if (af->duration == -1)
	  /* No action */
          ;
	else {
	  if ((af->type > 0) && (af->type <= MAX_SPL_LIST)) // only spells for this part
	     if (*spell_wear_off_msg[af->type]) {
	        send_to_char(spell_wear_off_msg[af->type], i);
	        send_to_char("\n\r", i);
	     }

	  affect_remove(i, af);

          switch( af->type ) { // Anything special goes here
            case SPELL_FLY: 
            { /* Fly wears off...you fall :) */ 
             if((IS_SET(world[i->in_room].room_flags, FALL_DOWN) && (dir = 5)) ||
               (IS_SET(world[i->in_room].room_flags, FALL_UP) && (dir = 4)) ||
               (IS_SET(world[i->in_room].room_flags, FALL_EAST) && (dir = 1)) ||
               (IS_SET(world[i->in_room].room_flags, FALL_WEST) && (dir = 3)) ||
               (IS_SET(world[i->in_room].room_flags, FALL_SOUTH) && (dir = 2))||
               (IS_SET(world[i->in_room].room_flags, FALL_NORTH) && (dir = 0))) 
               { 
                  do_fall(i, dir);
               }
               break;
            }
            case SPELL_IRON_ROOTS:
               REMOVE_BIT(i->affected_by2, AFF_NO_FLEE);
               break;
            case SPELL_STONE_SKIN:  /* Stone skin wears off... Remove resistance */
               REMOVE_BIT(i->resist, ISR_PIERCE);
               break;
            case SPELL_RESIST_FIRE:   /* resist fire wears off  */
               REMOVE_BIT(i->resist, ISR_FIRE);
               break;
	     case SPELL_BARKSKIN: /* barkskin wears off */
 	       REMOVE_BIT(i->resist, ISR_SLASH);
               break;
             case SPELL_RESIST_ENERGY:
               REMOVE_BIT(i->resist, ISR_ENERGY);
               break;
             case SPELL_RESIST_ACID:
               REMOVE_BIT(i->resist, ISR_ACID);
             case 191:
               REMOVE_BIT(i->immune, ISR_POISON);
               break;
             case SPELL_RESIST_COLD:   /* resist cold wears off */
               REMOVE_BIT(i->resist, ISR_COLD);
               break;
             case SKILL_INSANE_CHANT:
               send_to_char("The insane chanting in your mind wears off.\r\n", i);
               break;
             case SKILL_GLITTER_DUST:
               send_to_char("The dust around your body stops glowing.\r\n", i);
               break;
             case SKILL_BLOOD_FURY:
               send_to_char("Your blood cools to normal levels.\r\n", i);
               break;
             case SKILL_BLADESHIELD:
               send_to_char("The draining affect of the blade shield technique has worn off.\r\n", i);
               break;
             case SKILL_FOCUSED_REPELANCE:
               REMOVE_BIT(i->combat, COMBAT_REPELANCE);
               send_to_char("Your mind recovers from the repelance.\n\r", i);
               break;
             case SKILL_VITAL_STRIKE:
               send_to_char("The internal strength and speed from your vital strike has returned.\r\n", i);
               break;
             case SPELL_WATER_BREATHING:
               if(world[i->in_room].sector_type == SECT_UNDERWATER) // uh oh
               {
                  // you just drowned!
                  act("$n begins to choke on the water, a look of panic filling $s eyes as it fill $s lungs.\n\r"
                      "$n is DEAD!!", i, 0, 0, TO_ROOM, 0);
                  send_to_char("The water rushes into your lungs and the light fades with your oxygen.\n\r"
                               "You have been KILLED!!!\n\r", i);
                  fight_kill(NULL, i, TYPE_RAW_KILL);
               }
               break;
             case KI_STANCE + KI_OFFSET:
               send_to_char("Your body finishes venting the energy absorbed from your last ki stance.\r\n", i);
               break;
             case 7: {   /* Charm Wears off */
               remove_memory(i, 'h');
               add_memory(i, GET_NAME(i->master), 'h');
               stop_follower(i,0);
               break;
              } 
             default:
               break;
          }
	}
      }
  }
}

// Sets any ISR's that go with a spell..  (ISR's arent saved) 
void isr_set(CHAR_DATA *ch)
{
  // char buf[100];
  static struct affected_type *afisr;

  if(!ch) {
    log( "NULL ch in isr_set!", 0, LOG_BUG);
    return;
  }

/*  why do we need this spamming the logs?
   sprintf(buf, "isr_set ch %s", GET_NAME(ch));
   log(buf, 0, LOG_BUG);
*/
  for (afisr = ch->affected; afisr; afisr = afisr->next) {
    if (afisr->type == SPELL_STONE_SKIN)
      SET_BIT(ch->resist, ISR_PIERCE);
    else if (afisr->type == SPELL_BARKSKIN)
      SET_BIT(ch->resist, ISR_SLASH);

  }
}

bool many_charms(CHAR_DATA *ch)
{
  struct follow_type *k;
   
  for(k = ch->followers; k; k = k->next) {
     if(IS_AFFECTED(k->follower, AFF_CHARM)) 
        return TRUE;
  }

  return FALSE;
}


bool any_charms(CHAR_DATA *ch)
{
  return many_charms(ch);
/*
  struct follow_type *k;
  int counter = 0;

  for(k = ch->followers; k; k = k->next) {
     if(IS_AFFECTED(k->follower, AFF_CHARM)) 
       counter++;
  }

  if(counter > 1)
    return TRUE;
  else
    return FALSE;
*/
}


// check if making ch follow victim will create an illegal 
// follow "Loop/circle"
bool circle_follow(CHAR_DATA *ch, CHAR_DATA *victim)
{
    CHAR_DATA *k;

    for(k=victim; k; k=k->master) {
	if (k == ch)
	    return(TRUE);
    }

    return(FALSE);
}

// Called when stop following persons, or stopping charm
// This will NOT do if a character quits/dies!!
void stop_follower(CHAR_DATA *ch, int cmd)
{
  struct follow_type *j, *k;

  if(ch->master == NULL) { 
    log( "Stop_follower: null ch_master!", ARCHANGEL, LOG_BUG );
    return;
  }
/*
  if(IS_SET(ch->affected_by2, AFF_FAMILIAR)) {
    do_emote(ch, "screams in pain as its connection with its master is broken.", 9); 
    extract_char(ch, TRUE);
    return;
  }
*/
  if(IS_AFFECTED(ch, AFF_CHARM)) {
    act("You realize that $N is a jerk!", ch, 0, ch->master, TO_CHAR, 0);
    act("$n is free from the bondage of the spell.", ch, 0, 0, TO_ROOM, 0);
    act("$n hates your guts!", ch, 0, ch->master, TO_VICT, 0);
    if(affected_by_spell(ch, SPELL_CHARM_PERSON))
      affect_from_char(ch, SPELL_CHARM_PERSON);
    if(IS_SET(ch->affected_by, AFF_CHARM))
      REMOVE_BIT(ch->affected_by, AFF_CHARM); 
  }
  else {
    if(cmd == 1) {
      act("You sneakily stop following $N.",
           ch, 0, ch->master, TO_CHAR, 0);
    }
    else {
      act("You stop following $N.", ch, 0, ch->master, TO_CHAR, 0);
      act("$n stops following $N.", ch, 0, ch->master, TO_ROOM, NOTVICT);
      act("$n stops following you.", ch, 0, ch->master, TO_VICT, 0);
    }
  }

  if(ch->master->followers->follower == ch) { /* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    dc_free(k);
  }
  else { /* locate follower who is not head of list */
    for(k = ch->master->followers; k->next->follower != ch; k=k->next)
       ;

    j = k->next;
    k->next = j->next;
    dc_free(j);
  }

  ch->master = 0;
  if(cmd != 2)
    REMOVE_BIT(ch->affected_by, AFF_CHARM | AFF_GROUP);
}



/* Called when a character that follows/is followed dies */
void die_follower(CHAR_DATA *ch)
{
    struct follow_type *j, *k;
    CHAR_DATA * zombie;
    
    if (ch->master)
	stop_follower(ch, 0);

    for (k = ch->followers; k; k = j) {
	j = k->next;
	zombie = k->follower;
        if(!IS_SET(zombie->affected_by2, AFF_GOLEM))
	  stop_follower(zombie, 0);
	if(GET_RACE(zombie) == RACE_UNDEAD) {
          send_to_char("The forces holding you together are gone.  You cease "
                       "to exist.", zombie);
          act("$n dissolves into a puddle of rotten ooze.", zombie, 0, 0,
              TO_ROOM, 0);
    
          make_dust(zombie);
          extract_char(zombie, TRUE);
	}
    }
}



/* Do NOT call ths before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(CHAR_DATA *ch, CHAR_DATA *leader, int cmd)
{
    struct follow_type *k;

    if (cmd != 2)
      REMOVE_BIT(ch->affected_by, AFF_GROUP);

    assert(!ch->master);

    ch->master = leader;

#ifdef LEAK_CHECK
    k = (struct follow_type *)calloc(1, sizeof(struct follow_type));
#else
    k = (struct follow_type *)dc_alloc(1, sizeof(struct follow_type));
#endif

    k->follower = ch;
    k->next = leader->followers;
    leader->followers = k;

    if(cmd == 1)
      act("You stalk $N.", ch, 0, leader, TO_CHAR, 0);

    else if(cmd == 2)
      return;

    else { 
      act("You now follow $N.",  ch, 0, leader, TO_CHAR, 0);
      act("$n starts following you.", ch, 0, leader, TO_VICT, INVIS_NULL);
      act("$n now follows $N.", ch, 0, leader, TO_ROOM, INVIS_NULL|NOTVICT);
    }
}


void say_spell( CHAR_DATA *ch, int si )
{
    char buf[MAX_STRING_LENGTH], splwd[MAX_BUF_LENGTH];
    char buf2[MAX_STRING_LENGTH];

    int j, offs;
    CHAR_DATA *temp_char;


    struct syllable {
	char org[10];
	char new_new[10];
    };

    struct syllable syls[] = {
    { " ",        " " },
    { "ar",   "andoa" },
    { "au",    "hana" },
    { "bless", "amen" },
    { "blind", "ubra" },
    { "bur",   "misa" },
    { "cu",  "unmani" },
    { "de",    "oculo"},
    { "en",    "unso" },
    { "light",  "sol" },
    { "lo",      "hi" },
    { "mor",    "zak" },
    { "move",  "syfo" },
    { "ness", "licra" },
    { "ning",  "illa" },
    { "per",   "duca" },
    { "ra",     "bru" },
    { "re",   "xandu" },
    { "son",  "sabra" },
    { "tect",  "occa" },
    { "tri",   "cula" },
    { "ven",   "nofo" },
    {"a", "a"},{"b","b"},{"c","q"},{"d","e"},{"e","z"},{"f","y"},{"g","o"},
    {"h", "p"},{"i","u"},{"j","y"},{"k","t"},{"l","r"},{"m","w"},{"n","i"},
    {"o", "a"},{"p","s"},{"q","d"},{"r","f"},{"s","g"},{"t","h"},{"u","j"},
    {"v", "z"},{"w","x"},{"x","n"},{"y","l"},{"z","k"}, {"",""}
    };



    strcpy(buf, "");
    strcpy(splwd, spells[si-1]);

    offs = 0;

    while(*(splwd+offs)) {
	for(j=0; *(syls[j].org); j++)
	    if (strncmp(syls[j].org, splwd+offs, strlen(syls[j].org))==0) {
		strcat(buf, syls[j].new_new);
		if (strlen(syls[j].org))
		    offs+=strlen(syls[j].org);
		else
		    ++offs;
	    }
    }

    sprintf(buf2,"$n utters the words, '%s'", buf);
    sprintf(buf, "$n utters the words, '%s'", spells[si-1]);

    for(temp_char = world[ch->in_room].people;
	temp_char;
	temp_char = temp_char->next_in_room)
	if(temp_char != ch) {
	    if (GET_CLASS(ch) == GET_CLASS(temp_char))
		act(buf, ch, 0, temp_char, TO_VICT, 0);
	    else
		act(buf2, ch, 0, temp_char, TO_VICT, 0);
	}
}

// Takes the spell_base (higher = harder to resist)
// returns 0 or positive if saving throw is made. The more, the higher it was made.
// return -number of failure.   The lower, the more it was failed.
//
int saves_spell(CHAR_DATA *ch, CHAR_DATA *vict, int spell_base, sh_int save_type)
{
    double save = 0;

    // Gods always succeed saving throws.  We rock!
    if(!IS_NPC(vict) && (GET_LEVEL(vict) >= ARCHANGEL)) {
        return(TRUE);
    }

    // Get the base save type for this roll
    switch(save_type) {
      case SAVE_TYPE_FIRE:
            save = ch->saves[SAVE_TYPE_FIRE];
            break;
      case SAVE_TYPE_COLD:
            save = ch->saves[SAVE_TYPE_COLD];
            break;
      case SAVE_TYPE_ENERGY:
            save = ch->saves[SAVE_TYPE_ENERGY];
            break;
      case SAVE_TYPE_ACID:
            save = ch->saves[SAVE_TYPE_ACID];
            break;
      case SAVE_TYPE_MAGIC:
            save = ch->saves[SAVE_TYPE_MAGIC];
            // ISR Magic has to affect saving throws as well as damage so they don't get
            // para'd or slept or something
            if(IS_SET(vict->immune, ISR_MAGIC))       return(TRUE);
            if(IS_SET(vict->suscept, ISR_MAGIC))      save *= 0.7;
            if(IS_SET(vict->resist, ISR_MAGIC))       save *= 1.3;
            break;
      case SAVE_TYPE_POISON:
            save = ch->saves[SAVE_TYPE_POISON];
            break;
      default:
        break;
    }

    save += number(1, 100);
    spell_base += number(1, 100);
    return (int)(save - spell_base); 
}


char *skip_spaces(char *string)
{
    for(;*string && (*string)==' ';string++);

    return(string);
}


// Assumes that *argument does start with first letter of chopped string 
int do_cast(CHAR_DATA *ch, char *argument, int cmd)
{
  struct obj_data *tar_obj;
  CHAR_DATA *tar_char;
  char name[MAX_STRING_LENGTH];
  int qend, spl, i, learned;
  bool target_ok;

//  if (IS_NPC(ch))
//    return eFAILURE;
// Need to allow mob_progs to use cast without allowing charmies to
  
  if(IS_AFFECTED(ch, AFF_CHARM))
  {
    send_to_char("You can't cast while charmed!\n\r", ch);
    return eFAILURE;
  }

  if (GET_LEVEL(ch) < ARCHANGEL) {
    if (GET_CLASS(ch) == CLASS_WARRIOR) {
        send_to_char("Think you had better stick to fighting...\n\r", ch);
        return eFAILURE;
    } else if (GET_CLASS(ch) == CLASS_THIEF) {
       send_to_char("Think you should stick to robbing and killing...\n\r", ch);
        return eFAILURE;
    } else if (GET_CLASS(ch) == CLASS_BARBARIAN) {
        send_to_char("Think you should stick to berserking...\n\r", ch);
        return eFAILURE;
    } else if (GET_CLASS(ch) == CLASS_MONK) {
        send_to_char("Think you should stick with meditating...\n\r", ch);
        return eFAILURE;
    } else if ((GET_CLASS(ch) == CLASS_ANTI_PAL) && (!IS_EVIL(ch))) {
        send_to_char("You're not evil enough!\n\r", ch);
        return eFAILURE;
    } else if ((GET_CLASS(ch) == CLASS_PALADIN) && (!IS_GOOD(ch))) {
        send_to_char("You're not pure enough!\n\r", ch);
        return eFAILURE;
    } else if (GET_CLASS(ch) == CLASS_BARD) {
        send_to_char("Stick to singing bucko.", ch);
        return eFAILURE;
    }
    if (IS_SET(world[ch->in_room].room_flags, NO_MAGIC)) {
        send_to_char("You find yourself unable to weave magic here.\n\r", ch);
        return eFAILURE;
    }
  }

  argument = skip_spaces(argument);
  
  /* If there is no chars in argument */
  if (!(*argument)) {
    send_to_char("Cast which what where?\n\r", ch);
    return eFAILURE;
  }

  if (*argument != '\'') {
    send_to_char("Magic must always be enclosed by the holy magic symbols : '\n\r",ch);
    return eFAILURE;
  }

  /* Locate the last quote && lowercase the magic words (if any) */
  
  for (qend=1; *(argument+qend) && (*(argument+qend) != '\'') ; qend++)
    *(argument+qend) = LOWER(*(argument+qend));
  
  if (*(argument+qend) != '\'') {
    send_to_char("Magic must always be enclosed by the holy magic symbols : '\n\r", ch);
    return eFAILURE;
  }
  
  spl = old_search_block(argument, 1, qend-1, spells, 0);
  
  if (spl <= 0) {
    send_to_char("Your lips do not move, no magic appears.\n\r",ch);
    return eFAILURE;
  }

  if (spell_info[spl].spell_pointer)
  {
    if (GET_POS(ch) < spell_info[spl].minimum_position) {
      switch(GET_POS(ch)) {
        case POSITION_SLEEPING :
          send_to_char("You dream about great magical powers.\n\r", ch);
          break;
        case POSITION_RESTING :
          send_to_char("You can't concentrate enough while resting.\n\r",ch);
          break;
        case POSITION_SITTING :
          send_to_char("You can't do this sitting!\n\r", ch);
          break;
        case POSITION_FIGHTING :
          send_to_char("Impossible! You can't concentrate enough!.\n\r", ch);
          break;
        default:
          send_to_char("It seems like you're in a pretty bad shape!\n\r",ch);
          break;
      } /* Switch */
    } else 
    {
      if (GET_LEVEL(ch) < ARCHANGEL && !IS_MOB(ch)) 
      {
        if(!(learned = has_skill(ch, spl))) {
          send_to_char("You do not know how to cast that spell!\n\r", ch);
          return eFAILURE;
        }
      }
      else learned = 80;

      argument+=qend+1; /* Point to the last ' */
      for(;*argument == ' '; argument++);
      
      /* **************** Locate targets **************** */
      
      target_ok = FALSE;
      tar_char = 0;
      tar_obj = 0;
      
      if (!IS_SET(spell_info[spl].targets, TAR_IGNORE)) 
      {
        argument = one_argument(argument, name);

        if (*name) 
        {
          if (IS_SET(spell_info[spl].targets, TAR_CHAR_ROOM))
            if ( ( tar_char = get_char_room_vis(ch, name) ) != NULL )
              target_ok = TRUE;

          if (!target_ok && IS_SET(spell_info[spl].targets, TAR_CHAR_WORLD))
            if ( ( tar_char = get_char_vis(ch, name) ) != NULL )
              target_ok = TRUE;
      
          if (!target_ok && IS_SET(spell_info[spl].targets, TAR_OBJ_INV))
            if ( ( tar_obj = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL )
              target_ok = TRUE;

          if (!target_ok && IS_SET(spell_info[spl].targets, TAR_OBJ_ROOM))
          {
            tar_obj = get_obj_in_list_vis( ch, name, world[ch->in_room].contents );
            if ( tar_obj != NULL )
              target_ok = TRUE;
          }

          if (!target_ok && IS_SET(spell_info[spl].targets, TAR_OBJ_WORLD))
            if ( ( tar_obj = get_obj_vis(ch, name) ) != NULL )
              target_ok = TRUE;

          if (!target_ok && IS_SET(spell_info[spl].targets, TAR_OBJ_EQUIP)) 
          {
            for(i=0; i<MAX_WEAR && !target_ok; i++)
              if (ch->equipment[i] && str_cmp(name, ch->equipment[i]->name) == 0) {
                tar_obj = ch->equipment[i];
                target_ok = TRUE;
              }
          }

          if (!target_ok && IS_SET(spell_info[spl].targets, TAR_SELF_ONLY))
            if (str_cmp(GET_NAME(ch), name) == 0) {
              tar_char = ch;
              target_ok = TRUE;
            }
        } else { // !*name No argument was typed 
    
          if (IS_SET(spell_info[spl].targets, TAR_FIGHT_SELF))
            if ((ch->fighting) 
                 && ((ch->fighting)->in_room == ch->in_room)) {
              tar_char = ch;
              target_ok = TRUE;
            }

          if (!target_ok && IS_SET(spell_info[spl].targets, TAR_FIGHT_VICT))
            if (ch->fighting && (ch->in_room == ch->fighting->in_room)) 
                   // added the in_room checks -pir2/23/01
            {
              tar_char = ch->fighting;
              target_ok = TRUE;
            }

          if (!target_ok && ( IS_SET(spell_info[spl].targets, TAR_SELF_ONLY) ||
                              IS_SET(spell_info[spl].targets, TAR_SELF_DEFAULT))) {
            tar_char = ch;
            target_ok = TRUE;
          }

          if (!target_ok && IS_SET(spell_info[spl].targets, TAR_NONE_OK)) {
            target_ok = TRUE;
          }
        }

      } else { // tar_ignore is true
        target_ok = TRUE; /* No target, is a good target */
      }

      if (!target_ok) {
        if (*name) 
        {
          if (IS_SET(spell_info[spl].targets, TAR_CHAR_ROOM))
            send_to_char("Nobody here by that name.\n\r", ch);
          else if (IS_SET(spell_info[spl].targets, TAR_CHAR_WORLD))
            send_to_char("Nobody playing by that name.\n\r", ch);
          else if (IS_SET(spell_info[spl].targets, TAR_OBJ_INV))
            send_to_char("You are not carrying anything like that.\n\r", ch);
          else if (IS_SET(spell_info[spl].targets, TAR_OBJ_ROOM))
            send_to_char("Nothing here by that name.\n\r", ch);
          else if (IS_SET(spell_info[spl].targets, TAR_OBJ_WORLD))
            send_to_char("Nothing at all by that name.\n\r", ch);
          else if (IS_SET(spell_info[spl].targets, TAR_OBJ_EQUIP))
            send_to_char("You are not wearing anything like that.\n\r", ch);
          else if (IS_SET(spell_info[spl].targets, TAR_OBJ_WORLD))
            send_to_char("Nothing at all by that name.\n\r", ch);
        } else { /* Nothing was given as argument */
          if (spell_info[spl].targets < TAR_OBJ_INV)
            send_to_char("Whom should the spell be cast upon?\n\r", ch);
          else
            send_to_char("What should the spell be cast upon?\n\r", ch);
        }
        return eFAILURE;
      } else { /* TARGET IS OK */
        if ((tar_char == ch) && IS_SET(spell_info[spl].targets, TAR_SELF_NONO)) {
          send_to_char("You can not cast this spell upon yourself.\n\r", ch);
          return eFAILURE;
        }
        else if ((tar_char != ch) && IS_SET(spell_info[spl].targets, TAR_SELF_ONLY)) {
          send_to_char("You can only cast this spell upon yourself.\n\r", ch);
          return eFAILURE;
        } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tar_char)) {
          send_to_char("You are afraid that it could harm your master.\n\r", ch);
          return eFAILURE;
        }
      }

      if (GET_LEVEL(ch) < ARCHANGEL && !IS_MOB(ch)) {
        if (GET_MANA(ch) < use_mana(ch, spl)) {
          send_to_char("You can't summon enough energy to cast the spell.\n\r",ch);
          return eFAILURE;
        }
      }

      if (spl != SPELL_VENTRILOQUATE)  /* :-) */
        say_spell(ch, spl);

      WAIT_STATE(ch, spell_info[spl].beats);
      
      if ((spell_info[spl].spell_pointer == 0) && spl>0)
        send_to_char("Sorry, this magic has not yet been implemented :(\n\r", ch);
      else 
      {
/****
| The new one:  people have a skill that runs from 0 to 100.  So we roll a 
|   random number between 1 and 105 (inclusive).  We take their skill, and
|   subtract some amount based on their wisdom (it does something now!); bonuses
|   to people with over 20 wisdom (it adds instead of subtracting).  So
|   with 25 you could still conceivably fail on a really crappy roll, but only
|   1/105 chance ;).  Mobs have 0 wisdom, and so are except.  If the random
|   number is higher than their modified skill, they fail the spell.  Otherwise
|   it works. Morc 24 Apr 1997
| Modified: 
|   Skills should only go to 99 max. -pir
*/
        // make sure we don't count any specialization in the casting:)
        learned = learned % 100;

        if(IS_MOB(ch))
          learned = 50;

        // TODO - make this use int for mages
        if(GET_LEVEL(ch) < IMMORTAL && number(1,105) >
           (80 - (20 - (GET_WIS(ch)==0 ? 20 : GET_WIS(ch))))
          )
        {
          send_to_char("You lost your concentration!\n\r", ch);
          GET_MANA(ch) -= (use_mana(ch, spl) >> 1);
          return eSUCCESS;
        }

        if (IS_AFFECTED(ch, AFF_INVISIBLE)) {
           act("$n slowly fades into existence.", ch, 0, 0, TO_ROOM, 0);
           affect_from_char(ch, SPELL_INVISIBLE);
           REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
        }

        send_to_char("Ok.\n\r", ch);

// TODO - pass 'learned' to the spell so it can use it to determine spell effect
//        (we could just call "has_skill" again, but that wouldn't be very smart)

        GET_MANA(ch) -= (use_mana(ch, spl));
        return ((*spell_info[spl].spell_pointer) (GET_LEVEL(ch), ch, argument, SPELL_TYPE_SPELL, tar_char, tar_obj, learned));
      }
    }   /* if GET_POS < min_pos */
    return eFAILURE;
  }
  return eFAILURE;
}

int do_spells(CHAR_DATA *ch, char *argument, int cmd_arg)
{
    if (IS_NPC(ch))
        return eFAILURE;

// TODO - fix spells command to show a PC his spells data
// or...if a god, all the spells
    send_to_char("The spells command is disabled.  Pirahna will fix it later.\n\r", ch);
    return eSUCCESS;

/*
    char buf[16384];
    int cmd, clss, level;

    clss = GET_CLASS(ch);
    level = GET_LEVEL(ch);

    if (level > ARCHANGEL)
      clss = ARCHANGEL;

    buf[0] = '\0';
    for ( cmd = 0; cmd < MAX_SPL_LIST; cmd++ )
    {
      if ( (cmd > 43) && (cmd < 53) )
         continue;
    switch(clss) {
    case CLASS_CLERIC:
      if (spl_lvl(spell_info[cmd+1].min_level_cleric) > 0)
      {
        sprintf( buf + strlen(buf), "Spell %-20s  Level: %-2d  Mana: %-3d",
        spells[cmd], spl_lvl(spell_info[cmd+1].min_level_cleric), 
        spell_info[cmd+1].min_usesmana);

            strcat(buf, "\n\r");
      }
    break;

    case CLASS_MAGIC_USER:
      if (spl_lvl(spell_info[cmd+1].min_level_magic) > 0)
      {
        sprintf( buf + strlen(buf), "Spell %-19s  Level: %-2d  Mana: %-3d",
        spells[cmd], spl_lvl(spell_info[cmd+1].min_level_magic),
        spell_info[cmd+1].min_usesmana);

            strcat(buf, "\n\r");
      }
    break;

    case CLASS_ANTI_PAL:
      if (spl_lvl(spell_info[cmd+1].min_level_anti) > 0)
      {
        sprintf( buf + strlen(buf), "Spell %-19s  Level: %-2d  Mana: %-3d",
        spells[cmd], spl_lvl(spell_info[cmd+1].min_level_anti),
        spell_info[cmd+1].min_usesmana);

            strcat(buf, "\n\r");
      }
    break;

    case CLASS_PALADIN:
      if (spl_lvl(spell_info[cmd+1].min_level_paladin) > 0)
      {
        sprintf( buf + strlen(buf), "Spell %-19s  Level: %-2d  Mana: %-3d",
        spells[cmd], spl_lvl(spell_info[cmd+1].min_level_paladin),
        spell_info[cmd+1].min_usesmana);

            strcat(buf, "\n\r");
      }
    break;

    case CLASS_RANGER:
      if (spl_lvl(spell_info[cmd+1].min_level_ranger) > 0)
      {
        sprintf( buf + strlen(buf), "Spell %-19s  Level: %-2d  Mana: %-3d",
        spells[cmd], spl_lvl(spell_info[cmd+1].min_level_ranger),
        spell_info[cmd+1].min_usesmana);

            strcat(buf, "\n\r");
      }
    break;

    case CLASS_DRUID:
      if (spl_lvl(spell_info[cmd+1].min_level_druid) > 0)
      {
        sprintf( buf + strlen(buf), "Spell %-19s  Level: %-2d  Mana: %-3d",
        spells[cmd], spl_lvl(spell_info[cmd+1].min_level_druid),
        spell_info[cmd+1].min_usesmana);

            strcat(buf, "\n\r");
      }
    break;

    case ARCHANGEL:
        sprintf( buf + strlen(buf), "Spell %-19s Ma %-2d Cl %-2d AP %-2d Pl %-2d Rn %-2d Du %-2d  Mana: %-3d",
        spells[cmd], spl_lvl(spell_info[cmd+1].min_level_magic),spl_lvl(spell_info[cmd+1].min_level_cleric),
        spl_lvl(spell_info[cmd+1].min_level_anti), spl_lvl(spell_info[cmd+1].min_level_paladin),
        spl_lvl(spell_info[cmd+1].min_level_ranger), spl_lvl(spell_info[cmd+1].min_level_druid),
        spell_info[cmd+1].min_usesmana);

            strcat(buf, "\n\r");
    break;

    default:
      strcat(buf, "You do not have any spells. -->BONK<--\n\r");
      cmd = MAX_SPL_LIST + 1;
      strcat(buf, "\n\r");
    break;
    }
    }

    strcat(buf, "\n\r");
    page_string(ch->desc, buf, 1);
    return eSUCCESS;
*/
}

int spl_lvl(int lev)
{
  if(lev >= MIN_GOD)
    return 0;
  return lev;
}
