/****************************************************************************
 *  file: spells.c , Basic routines and parsing            Part of DIKUMUD  *
 *  Usage : Interpreter of spells                                           *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information.  *
 *                                                                          *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse   *
 *  Performance optimization and bug fixes by MERC Industries.              *
 *  You can use our stuff in any way you like whatsoever so long as ths     *
 *  copyright notice remains intact.  If you like it please drop a line     *
 *  to mec@garnet.berkeley.edu.                                             *
 *                                                                          *
 *  This is free software and you are benefitting.  We hope that you        *
 *  share your changes too.  What goes around, comes around.                *
 *                                                                          *
 *  Revision History                                                        *
 *  10/23/2003   Onager   Commented out effect wear-off stuff in            *
 *                        affect_update() (moved to affect_remove())        *
 *  10/27/2003   Onager   Changed stop_follower() cmd values to be readable *
 *                        #defines, added a BROKE_CHARM cmd                 *
 *  12/07/2003   Onager   Changed PFE/PFG entries in spell_info[] to allow  *
 *                        casting on others                                 *
 ***************************************************************************/
/* $Id: spells.cpp,v 1.247 2009/01/23 19:31:22 kkoons Exp $ */

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
#include <ki.h>
#include <sing.h>
#include <arena.h>
#include <clan.h>

// Global data 

extern CWorld world;
extern struct class_skill_defines w_skills[];
extern struct class_skill_defines t_skills[];
extern struct class_skill_defines d_skills[];
extern struct class_skill_defines b_skills[];
extern struct class_skill_defines a_skills[];
extern struct class_skill_defines p_skills[];
extern struct class_skill_defines r_skills[];
extern struct class_skill_defines k_skills[];
extern struct class_skill_defines u_skills[];
extern struct class_skill_defines c_skills[];
extern struct class_skill_defines m_skills[];
extern char * attrstring(int);
extern CHAR_DATA *character_list;
extern struct song_info_type song_info[];
extern struct ki_info_type ki_info[];
extern char *spell_wear_off_msg[];
extern struct index_data *obj_index;

// Functions used in spells.C
int spl_lvl(int lev);

// Extern procedures 
int do_fall(CHAR_DATA *ch, short dir);
void remove_memory(CHAR_DATA *ch, char type);
void add_memory(CHAR_DATA *ch, char *victim, char type);
void make_dust(CHAR_DATA * ch);
void say_spell( CHAR_DATA *ch, int si, int room = 0);
extern struct index_data *mob_index;

#if(0)
    ubyte        beats;                  /* Waiting time after spell     */
    ubyte        minimum_position;       /* Position for caster          */
    ubyte       min_usesmana;           /* Mana used                    */
    int16      targets;                /* Legal targets                */
    SPELL_FUN * spell_pointer;          /* Function to call             */
    int16      difficulty;
#endif


struct spell_info_type spell_info [] =
{
 { /* 00 */ /* Note: All arrays start at 0! CGT */  0, 0, 0, 0, 0 },

 { /* 01 */ 12, POSITION_STANDING,  8, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_armor, SKILL_INCREASE_MEDIUM },

 { /* 02 */ 12, POSITION_FIGHTING, 35, TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_teleport, SKILL_INCREASE_HARD },

 { /* 03 */ 12, POSITION_STANDING,  6, TAR_CHAR_ROOM|TAR_SELF_DEFAULT|TAR_OBJ_INV|TAR_OBJ_ROOM|TAR_OBJ_EQUIP, cast_bless, SKILL_INCREASE_MEDIUM },

 { /* 04 */ 12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_blindness, SKILL_INCREASE_HARD },

 { /* 05 */ 12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_burning_hands, SKILL_INCREASE_MEDIUM },

 { /* 06 */ 12, POSITION_STANDING, 15, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_iridescent_aura, SKILL_INCREASE_MEDIUM },

 { /* 07 */ /* 18, POSITION_STANDING, 15, TAR_CHAR_ROOM|TAR_SELF_NONO, cast_charm_person */ 0, 0, 0, 0, 0, 0 },

 { /* 08 */ 12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_chill_touch, SKILL_INCREASE_HARD },

 { /* 09 */ /* 12, POSITION_STANDING, 40, TAR_CHAR_ROOM, cast_clone); */ 0, 0, 0, 0, 0, 0 },

 { /* 10 */ 12, POSITION_FIGHTING, 40, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_colour_spray, SKILL_INCREASE_HARD },

 { /* 11 */ 18, POSITION_STANDING, 25, TAR_IGNORE, cast_control_weather, SKILL_INCREASE_MEDIUM },

 { /* 12 */ 18, POSITION_STANDING,  5, TAR_IGNORE, cast_create_food, SKILL_INCREASE_MEDIUM },

 { /* 13 */ 18, POSITION_STANDING,  5, TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_create_water, SKILL_INCREASE_MEDIUM },

 { /* 14 */  9, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_remove_blind, SKILL_INCREASE_MEDIUM },

 { /* 15 */ 12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_cure_critic, SKILL_INCREASE_MEDIUM },

 { /* 16 */ 12, POSITION_FIGHTING, 10, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_cure_light, SKILL_INCREASE_EASY },

 { /* 17 */ 12, POSITION_FIGHTING, 33, TAR_FIGHT_VICT|TAR_SELF_NONO|TAR_CHAR_ROOM|TAR_OBJ_ROOM|TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_curse, SKILL_INCREASE_HARD },

 { /* 18 */ 12, POSITION_STANDING,  5, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_detect_evil, SKILL_INCREASE_EASY },

 { /* 19 */ 12, POSITION_STANDING,  8, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_detect_invisibility, SKILL_INCREASE_MEDIUM },

 { /* 20 */ 12, POSITION_STANDING,  6, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_detect_magic, SKILL_INCREASE_EASY },

 { /* 21 */ 12, POSITION_STANDING,  5, TAR_CHAR_ROOM|TAR_SELF_DEFAULT|TAR_OBJ_INV|TAR_OBJ_ROOM, cast_detect_poison, SKILL_INCREASE_EASY },

 { /* 22 */ 12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_dispel_evil, SKILL_INCREASE_MEDIUM },

 { /* 23 */ 12, POSITION_FIGHTING, 25, TAR_IGNORE, cast_earthquake, SKILL_INCREASE_HARD },

 { /* 24 */ 24, POSITION_STANDING, 50, TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_enchant_weapon, SKILL_INCREASE_MEDIUM },

 { /* 25 */ 12, POSITION_FIGHTING, 33, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_energy_drain, SKILL_INCREASE_HARD },

 { /* 26 */ 12, POSITION_FIGHTING, 25, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_fireball, SKILL_INCREASE_HARD },

 { /* 27 */ 12, POSITION_FIGHTING, 33, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_harm, SKILL_INCREASE_HARD },

 { /* 28 */ 12, POSITION_FIGHTING, 40, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_heal, SKILL_INCREASE_HARD },

 { /* 29 */ 12, POSITION_STANDING,  7, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM|TAR_OBJ_EQUIP|TAR_SELF_DEFAULT, cast_invisibility, SKILL_INCREASE_MEDIUM },

 { /* 30 */ 12, POSITION_FIGHTING, 17, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_lightning_bolt, SKILL_INCREASE_HARD },

 { /* 31 */ 12, POSITION_STANDING, 20, TAR_IGNORE, cast_locate_object, SKILL_INCREASE_HARD},

 { /* 32 */ 12, POSITION_FIGHTING, 10, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_magic_missile, SKILL_INCREASE_MEDIUM },

 { /* 33 */ 12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO|TAR_OBJ_INV|TAR_OBJ_ROOM, cast_poison, SKILL_INCREASE_HARD },

 { /* 34 */ 12, POSITION_STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_protection_from_evil, SKILL_INCREASE_MEDIUM },

 { /* 35 */  9, POSITION_FIGHTING, 18, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_EQUIP|TAR_OBJ_ROOM|TAR_SELF_DEFAULT, cast_remove_curse, SKILL_INCREASE_MEDIUM },

 { /* 36 */ 12, POSITION_STANDING, 60, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_sanctuary, SKILL_INCREASE_HARD },

 { /* 37 */ 12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_shocking_grasp, SKILL_INCREASE_MEDIUM },

 { /* 38 */ 18, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_sleep, SKILL_INCREASE_HARD },

 { /* 39 */ 12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_strength, SKILL_INCREASE_MEDIUM },

 { /* 40 */ 12, POSITION_FIGHTING, 50, TAR_CHAR_WORLD|TAR_SELF_NONO, cast_summon, SKILL_INCREASE_HARD },

 { /* 41 */ 12, POSITION_FIGHTING,  5, TAR_CHAR_ROOM|TAR_OBJ_ROOM|TAR_SELF_NONO, cast_ventriloquate, SKILL_INCREASE_EASY },

 { /* 42 */ 12, POSITION_FIGHTING, 40, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_word_of_recall, SKILL_INCREASE_MEDIUM },

 { /* 43 */ 9, POSITION_FIGHTING, 12, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM|TAR_SELF_DEFAULT, cast_remove_poison, SKILL_INCREASE_MEDIUM },

 { /* 44 */ 12, POSITION_STANDING, 15, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_sense_life, SKILL_INCREASE_EASY },

 { /* 45 */ 18, POSITION_STANDING, 45, TAR_IGNORE, cast_summon_familiar, SKILL_INCREASE_MEDIUM },

 { /* 46 */ 12, POSITION_STANDING, 30, TAR_IGNORE, cast_lighted_path, SKILL_INCREASE_HARD },

 { /* 47 */ 12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_resist_acid, SKILL_INCREASE_HARD },

 { /* 48 */ 12, POSITION_FIGHTING, 35, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_sun_ray, SKILL_INCREASE_HARD },

 { /* 49 */ 12, POSITION_STANDING, 30, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_rapid_mend, SKILL_INCREASE_HARD },

 { /* 50 */ 18, POSITION_STANDING, 120, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_acid_shield, SKILL_INCREASE_HARD },

 { /* 51 */ 12, POSITION_STANDING, 22, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_water_breathing, SKILL_INCREASE_EASY },

 { /* 52 */ 12, POSITION_FIGHTING, 20, TAR_IGNORE, cast_globe_of_darkness, SKILL_INCREASE_HARD },

 { /* 53 */ 24, POSITION_STANDING, 12, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM, cast_identify, SKILL_INCREASE_EASY },

 { /* 54 */ 12, POSITION_STANDING, 75, TAR_OBJ_ROOM, cast_animate_dead, SKILL_INCREASE_MEDIUM },

 { /* 55 */ 12, POSITION_FIGHTING, 17, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_fear, SKILL_INCREASE_HARD },

 { /* 56 */ 12, POSITION_STANDING, 10, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_fly, SKILL_INCREASE_MEDIUM },

 { /* 57 */ 12, POSITION_STANDING,  7, TAR_NONE_OK|TAR_OBJ_INV|TAR_OBJ_ROOM, cast_cont_light, SKILL_INCREASE_EASY },

 { /* 58 */ 12, POSITION_STANDING,  5, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_know_alignment, SKILL_INCREASE_EASY },

 { /* 59 */ 12, POSITION_FIGHTING, 30, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_dispel_magic, SKILL_INCREASE_HARD },

 { /* 60 */  24, POSITION_STANDING, 150, TAR_IGNORE, cast_conjure_elemental, SKILL_INCREASE_MEDIUM },

 { /* 61 */ 12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_cure_serious, SKILL_INCREASE_EASY },

 { /* 62 */ 12, POSITION_FIGHTING, 12, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_cause_light, SKILL_INCREASE_EASY },

 { /* 63 */ 12, POSITION_FIGHTING, 24, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_cause_critical, SKILL_INCREASE_MEDIUM },

 { /* 64 */ 12, POSITION_FIGHTING, 18, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_cause_serious, SKILL_INCREASE_EASY },

 { /* 65 */ 12, POSITION_FIGHTING, 45, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_flamestrike, SKILL_INCREASE_HARD },

 { /* 66 */ 12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_stone_skin, SKILL_INCREASE_HARD },

 { /* 67 */ 12, POSITION_STANDING, 12, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_shield, SKILL_INCREASE_MEDIUM },

 { /* 68 */ 12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_weaken, SKILL_INCREASE_HARD },

 { /* 69 */ 18, POSITION_STANDING, 33, TAR_IGNORE, cast_mass_invis, SKILL_INCREASE_MEDIUM },

 { /* 70 */ 12, POSITION_FIGHTING, 45, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_acid_blast, SKILL_INCREASE_HARD },

 { /* 71 */ 12, POSITION_STANDING, 55, TAR_CHAR_WORLD, cast_portal, SKILL_INCREASE_HARD },

 { /* 72 */ 12, POSITION_STANDING,  7, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_infravision, SKILL_INCREASE_EASY },

 { /* 73 */ 12, POSITION_STANDING, 12, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_refresh, SKILL_INCREASE_EASY },

 { /* 74 */ 12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_haste, SKILL_INCREASE_MEDIUM },

 { /* 75 */ 12, POSITION_FIGHTING, 20, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_dispel_good, SKILL_INCREASE_MEDIUM },

 { /* 76 */ 12, POSITION_FIGHTING, 80, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_hellstream, SKILL_INCREASE_HARD },

 { /* 77 */ 12, POSITION_FIGHTING, 60, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_power_heal, SKILL_INCREASE_HARD },

 { /* 78 */ 12, POSITION_FIGHTING, 80, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_full_heal, SKILL_INCREASE_HARD },

 { /* 79 */ 12, POSITION_FIGHTING, 55, TAR_IGNORE, cast_firestorm, SKILL_INCREASE_HARD },

 { /* 80 */ 12, POSITION_FIGHTING, 45, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_power_harm, SKILL_INCREASE_HARD },

 { /* 81 */ 12, POSITION_STANDING,  5, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_detect_good, SKILL_INCREASE_EASY },

 { /* 82 */ 12, POSITION_FIGHTING, 33, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_vampiric_touch, SKILL_INCREASE_HARD },

 { /* 83 */ 18, POSITION_FIGHTING, 40, TAR_IGNORE, cast_life_leech, SKILL_INCREASE_MEDIUM },

 { /* 84 */ 12, POSITION_FIGHTING, 33, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_paralyze, SKILL_INCREASE_HARD },

 { /* 85 */  9, POSITION_FIGHTING, 18, TAR_CHAR_ROOM, cast_remove_paralysis, SKILL_INCREASE_MEDIUM },

 { /* 86 */ 18, POSITION_STANDING, 160, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_fireshield, SKILL_INCREASE_MEDIUM },

 { /* 87 */ 12, POSITION_FIGHTING, 40, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_meteor_swarm, SKILL_INCREASE_HARD },

 { /* 88 */ 12, POSITION_STANDING, 20, TAR_CHAR_WORLD, cast_wizard_eye, SKILL_INCREASE_HARD },

 { /* 89 */ 12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_true_sight, SKILL_INCREASE_HARD },

 { /* 90 */ 12, POSITION_STANDING,  0, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_mana, 0 },

 { /* 91 */ 18, POSITION_FIGHTING, 200, TAR_IGNORE, cast_solar_gate, SKILL_INCREASE_MEDIUM },

 { /* 92 */ 12, POSITION_STANDING, 30, TAR_IGNORE, cast_heroes_feast, SKILL_INCREASE_EASY },

 { /* 93 */ 12, POSITION_FIGHTING, 100, TAR_IGNORE, cast_heal_spray, SKILL_INCREASE_MEDIUM },

 { /* 94 */ 12, POSITION_STANDING, 180, TAR_IGNORE, cast_group_sanc, SKILL_INCREASE_HARD },

 { /* 95 */ 12, POSITION_STANDING, 80, TAR_IGNORE, cast_group_recall, SKILL_INCREASE_MEDIUM },

 { /* 96 */ 12, POSITION_STANDING, 40, TAR_IGNORE, cast_group_fly, SKILL_INCREASE_MEDIUM },

 { /* 97 */ /* 24, POSITION_STANDING, 250, TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_enchant_armor */ 0, 0, 0, 0, 0, 0 },

 { /* 98 */ 12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_resist_fire, SKILL_INCREASE_HARD },

 { /* 99 */ 12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_resist_cold, SKILL_INCREASE_HARD },

 { /* 100 */ 12, POSITION_FIGHTING, 8, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_bee_sting, SKILL_INCREASE_MEDIUM },

 { /* 101 */ 12, POSITION_FIGHTING, 25, TAR_IGNORE, cast_bee_swarm, SKILL_INCREASE_HARD },

 { /* 102 */ 12, POSITION_FIGHTING, 45, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_creeping_death, SKILL_INCREASE_HARD },

 { /* 103 */ 12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_barkskin, SKILL_INCREASE_HARD },

 { /* 104 */ 12, POSITION_FIGHTING, 45, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_herb_lore, SKILL_INCREASE_HARD },

 { /* 105 */ 12, POSITION_FIGHTING, 75, TAR_IGNORE, cast_call_follower, SKILL_INCREASE_MEDIUM },

 { /* 106 */ 12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_entangle, SKILL_INCREASE_HARD },

 { /* 107 */ 12, POSITION_STANDING,  5, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_eyes_of_the_owl, SKILL_INCREASE_EASY },

 { /* 108 */ 12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_feline_agility, SKILL_INCREASE_MEDIUM },

 { /* 109 */ 12, POSITION_STANDING, 30, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_forest_meld, SKILL_INCREASE_HARD },

 { /* 110 */ /* 12, POSITION_STANDING, 150, TAR_IGNORE, cast_companion */ 0, 0, 0, 0, 0, 0 },

 { /* 111 */ 12, POSITION_FIGHTING, 33, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_drown, SKILL_INCREASE_HARD },

 { /* 112 */ 12, POSITION_FIGHTING, 25, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_howl, SKILL_INCREASE_HARD },

 { /* 113 */ 12, POSITION_FIGHTING, 33, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_souldrain, SKILL_INCREASE_MEDIUM },

 { /* 114 */ 12, POSITION_FIGHTING, 18, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_sparks, 0 },

 { /* 115 */ 12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_camouflague, SKILL_INCREASE_HARD },

 { /* 116 */ 12, POSITION_STANDING, 24, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_farsight, SKILL_INCREASE_HARD },

 { /* 117 */ 12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_freefloat, SKILL_INCREASE_HARD },

 { /* 118 */ 12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_insomnia, SKILL_INCREASE_HARD  },

 { /* 119 */ 12, POSITION_STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_shadowslip, SKILL_INCREASE_HARD  },

 { /* 120 */ 12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_resist_energy, SKILL_INCREASE_HARD },

 { /* 121 */ 12, POSITION_STANDING, 20, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_staunchblood, SKILL_INCREASE_HARD  },

 { /* 122 */ 24, POSITION_STANDING, 250, TAR_IGNORE, cast_create_golem, SKILL_INCREASE_EASY },

 { /* 123 */ 12, POSITION_STANDING, 60, TAR_IGNORE, spell_reflect, SKILL_INCREASE_HARD },

 { /* 124 */ 12, POSITION_FIGHTING, 22, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_OBJ_ROOM|TAR_OBJ_INV|TAR_SELF_NONO, cast_dispel_minor, SKILL_INCREASE_MEDIUM },

 { /* 125 */ 12, POSITION_FIGHTING, 25, TAR_IGNORE, spell_release_golem, SKILL_INCREASE_MEDIUM },

 { /* 126 */ 12, POSITION_FIGHTING, 30, TAR_IGNORE, spell_beacon, SKILL_INCREASE_MEDIUM },

 { /* 127 */ 12, POSITION_STANDING, 40, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_stone_shield, SKILL_INCREASE_HARD },

 { /* 128 */ 18, POSITION_STANDING, 55, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_greater_stone_shield, SKILL_INCREASE_HARD },

 { /* 129 */ 12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_iron_roots, SKILL_INCREASE_HARD },

 { /* 130 */ /* 12, POSITION_STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_eyes_of_the_eagle */ 0, 0, 0, 0, 0, 0 },

 { /* 131 */ /* 12, POSITION_STANDING,  0, TAR_CHAR_ROOM, NULL */ 0, 0, 0, 0, 0, 0 },

 { /* 132 */ /* 12, POSITION_FIGHTING, 25, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_ice_shards */ 0, 0, 0, 0, 0, 0 },

 { /* 133 */ 18, POSITION_STANDING, 65, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_lightning_shield, SKILL_INCREASE_HARD },

 { /* 134 */  9, POSITION_FIGHTING, 10, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_blue_bird, SKILL_INCREASE_EASY },

 { /* 135 */ 12, POSITION_FIGHTING, 15, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_debility, SKILL_INCREASE_MEDIUM },

 { /* 136 */ 12, POSITION_FIGHTING, 30, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_attrition, SKILL_INCREASE_MEDIUM },

 { /* 137 */ 18, POSITION_FIGHTING, 120, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_vampiric_aura, SKILL_INCREASE_EASY },

 { /* 138 */ 18, POSITION_FIGHTING, 200, TAR_IGNORE, cast_holy_aura, SKILL_INCREASE_EASY },

 { /* 139 */ 12, POSITION_STANDING,  5, TAR_IGNORE, cast_dismiss_familiar, SKILL_INCREASE_MEDIUM },

 { /* 140 */ 12, POSITION_STANDING, 15, TAR_IGNORE, cast_dismiss_corpse, SKILL_INCREASE_MEDIUM },

 { /* 141 */ 12, POSITION_FIGHTING, 30, TAR_IGNORE, cast_blessed_halo, SKILL_INCREASE_MEDIUM },

 { /* 142 */ 12, POSITION_FIGHTING, 40, TAR_IGNORE, cast_visage_of_hate, SKILL_INCREASE_MEDIUM },

 { /* 143 */ 12, POSITION_STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_protection_from_good, SKILL_INCREASE_MEDIUM },

 { /* 144 */ 12, POSITION_STANDING, 12, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_oaken_fortitude, SKILL_INCREASE_MEDIUM },
 
 { /* 145 */ 12, POSITION_STANDING, 24, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, NULL, SKILL_INCREASE_MEDIUM },

 { /* 146 */ 12, POSITION_STANDING, 24, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, NULL, SKILL_INCREASE_MEDIUM },

 { /* 147 */ 12, POSITION_STANDING, 24, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, NULL, SKILL_INCREASE_MEDIUM },

 { /* 148*/ 12, POSITION_STANDING, 24, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, NULL, SKILL_INCREASE_MEDIUM },

 { /* 149*/ 12, POSITION_STANDING, 24, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, NULL, SKILL_INCREASE_MEDIUM },

 { /* 150*/ 12, POSITION_STANDING, 24, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, NULL, SKILL_INCREASE_MEDIUM },

 { /* 151*/ 12, POSITION_STANDING, 24, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, NULL, SKILL_INCREASE_MEDIUM },

 { /* 152*/ 12, POSITION_STANDING, 24, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, NULL, SKILL_INCREASE_MEDIUM },

 { /* 153*/ 12, POSITION_STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_aegis, SKILL_INCREASE_HARD },

 { /* 154*/ 12, POSITION_STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_aegis, SKILL_INCREASE_HARD },

 { /* 155 */ 12, POSITION_STANDING, 33, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_resist_magic, SKILL_INCREASE_HARD },
 
 { /* 156 */ 12, POSITION_STANDING, 30, TAR_CHAR_WORLD, cast_eagle_eye, SKILL_INCREASE_HARD },

 { /* 157 */ 12, POSITION_FIGHTING, 35, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_call_lightning, SKILL_INCREASE_HARD },

 { /* 158 */ 12, POSITION_FIGHTING, 45, TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, cast_divine_fury, SKILL_INCREASE_HARD },

 { /* 159 */ 12, POSITION_STANDING, 45, TAR_IGNORE, cast_ghost_walk, SKILL_INCREASE_HARD },

 { /* 160 */ 12, POSITION_STANDING, 120, TAR_IGNORE, cast_mend_golem, SKILL_INCREASE_MEDIUM },

 { /* 161 */ 12, POSITION_STANDING, 10, TAR_CHAR_ROOM|TAR_SELF_DEFAULT, cast_clarity, SKILL_INCREASE_MEDIUM },

 { /* 162 */ 12, POSITION_STUNNED, 200, TAR_CHAR_ROOM|TAR_SELF_DEFAULT|TAR_SELF_ONLY, cast_divine_intervention, SKILL_INCREASE_EASY },

 { /* 163 */ 16, POSITION_FIGHTING, 75, TAR_IGNORE, cast_wrath_of_god, SKILL_INCREASE_HARD },

 { /* 164 */ 12, POSITION_FIGHTING, 100, TAR_CHAR_ROOM|TAR_SELF_DEFAULT|TAR_SELF_ONLY, cast_atonement, SKILL_INCREASE_EASY },

 { /* 165 */ 16, POSITION_FIGHTING, 150, TAR_IGNORE, cast_silence, SKILL_INCREASE_HARD },

 { /* 166 */ 12, POSITION_STANDING, 180, TAR_CHAR_ROOM|TAR_SELF_DEFAULT|TAR_SELF_ONLY, cast_immunity, SKILL_INCREASE_HARD },

 { /* 167 */ 12, POSITION_STANDING, 150, TAR_CHAR_ROOM, cast_boneshield, SKILL_INCREASE_HARD },

 { /* 168 */ 12, POSITION_FIGHTING, 100, TAR_CHAR_ROOM|TAR_SELF_NONO, cast_channel, SKILL_INCREASE_HARD },

 { /* 169 */ 12, POSITION_STANDING, 15, TAR_IGNORE, cast_release_elemental, SKILL_INCREASE_MEDIUM },
 
 { /* 170 */ 12, POSITION_FIGHTING, 50, TAR_CHAR_ROOM|TAR_FIGHT_VICT, cast_wild_magic, SKILL_INCREASE_HARD },

 { /* 171 */ 12, POSITION_FIGHTING, 125, TAR_IGNORE, cast_spirit_shield, SKILL_INCREASE_HARD },

};


struct skill_stuff skill_info[] =
{
/*  1 */                { "trip", SKILL_INCREASE_MEDIUM },
/*  2 */               { "dodge", SKILL_INCREASE_HARD },
/*  3 */              { "double", SKILL_INCREASE_HARD },
/*  4 */              { "disarm", SKILL_INCREASE_MEDIUM },
/*  5 */              { "triple", SKILL_INCREASE_HARD },
/*  6 */               { "parry", SKILL_INCREASE_HARD },
/*  7 */         { "deathstroke", SKILL_INCREASE_EASY },
/*  8 */              { "circle", SKILL_INCREASE_MEDIUM },
/*  9 */             { "berserk", SKILL_INCREASE_HARD },
/* 10 */            { "headbutt", SKILL_INCREASE_HARD },
/* 11 */          { "eagle claw", SKILL_INCREASE_MEDIUM },
/* 12 */      { "quivering palm", SKILL_INCREASE_EASY },
/* 13 */                { "palm", SKILL_INCREASE_HARD },
/* 14 */               { "stalk", SKILL_INCREASE_HARD },
/* 15 */              { "UNUSED", 0 },
/* 16 */       { "dual_backstab", SKILL_INCREASE_HARD },
/* 17 */              { "hitall", SKILL_INCREASE_HARD },
/* 18 */                { "stun", SKILL_INCREASE_HARD },
/* 19 */                { "scan", SKILL_INCREASE_EASY },
/* 20 */            { "consider", SKILL_INCREASE_EASY },
/* 21 */              { "switch", SKILL_INCREASE_EASY },
/* 22 */            { "redirect", SKILL_INCREASE_MEDIUM },
/* 23 */              { "ambush", SKILL_INCREASE_MEDIUM },
/* 24 */              { "forage", SKILL_INCREASE_HARD },
/* 25 */                { "tame", SKILL_INCREASE_MEDIUM },
/* 26 */               { "track", SKILL_INCREASE_HARD },
/* 27 */              { "skewer", SKILL_INCREASE_HARD },
/* 28 */                { "slip", SKILL_INCREASE_MEDIUM },
/* 29 */             { "retreat", SKILL_INCREASE_HARD },
/* 30 */                { "rage", SKILL_INCREASE_MEDIUM },
/* 31 */           { "battlecry", SKILL_INCREASE_EASY },
/* 32 */             { "archery", SKILL_INCREASE_HARD },
/* 33 */             { "riposte", SKILL_INCREASE_HARD },
/* 34 */           { "lay hands", SKILL_INCREASE_EASY },
/* 35 */        { "insane chant", 0 },
/* 36 */        { "glitter dust", 0 },
/* 37 */               { "sneak", SKILL_INCREASE_HARD },
/* 38 */                { "hide", SKILL_INCREASE_HARD },
/* 39 */               { "steal", SKILL_INCREASE_MEDIUM },
/* 40 */            { "backstab", SKILL_INCREASE_MEDIUM },
/* 41 */           { "pick_lock", SKILL_INCREASE_EASY },
/* 42 */                { "kick", SKILL_INCREASE_MEDIUM },
/* 43 */                { "bash", SKILL_INCREASE_HARD },
/* 44 */              { "rescue", SKILL_INCREASE_MEDIUM },
/* 45 */          { "blood_fury", SKILL_INCREASE_EASY },
/* 46 */          { "dual_wield", SKILL_INCREASE_EASY },
/* 47 */          { "harm_touch", SKILL_INCREASE_EASY },
/* 48 */        { "shield_block", SKILL_INCREASE_HARD },
/* 49 */        { "blade_shield", SKILL_INCREASE_EASY },
/* 50 */              { "pocket", SKILL_INCREASE_MEDIUM },
/* 51 */               { "guard", SKILL_INCREASE_MEDIUM },
/* 52 */              { "frenzy", SKILL_INCREASE_HARD },
/* 53 */       { "blindfighting", SKILL_INCREASE_HARD },
/* 54 */   { "focused_repelance", SKILL_INCREASE_EASY },
/* 55 */        { "vital_strike", SKILL_INCREASE_EASY },
/* 56 */      { "crazed_assault", SKILL_INCREASE_HARD },
/* 57 */   { "divine_protection", 0 },
/* 58 */ { "bludgeoning_weapons", SKILL_INCREASE_MEDIUM },
/* 59 */    { "piercing_weapons", SKILL_INCREASE_MEDIUM },
/* 60 */    { "slashing_weapons", SKILL_INCREASE_MEDIUM },
/* 61 */    { "whipping_weapons", SKILL_INCREASE_MEDIUM },
/* 62 */    { "crushing_weapons", SKILL_INCREASE_MEDIUM },
/* 63 */  { "two_handed_weapons", SKILL_INCREASE_MEDIUM },
/* 64 */        { "hand_to_hand", SKILL_INCREASE_MEDIUM },
/* 65 */            { "bullrush", SKILL_INCREASE_HARD },
/* 66 */            { "ferocity", SKILL_INCREASE_MEDIUM },
/* 67 */             { "tactics", SKILL_INCREASE_MEDIUM },
/* 68 */              { "deceit", SKILL_INCREASE_MEDIUM },
/* 69 */             { "release", SKILL_INCREASE_EASY },
/* 70 */           { "fear gaze", 0 },
/* 71 */            { "eyegouge", SKILL_INCREASE_HARD },
/* 72 */        { "magic resist", SKILL_INCREASE_HARD },
/* 73 */          { "ignorethis", 0},
/* 74 */	  { "spellcraft", SKILL_INCREASE_HARD},
/* 75 */     { "martial defense", SKILL_INCREASE_HARD},
/* 76 */           { "knockback", SKILL_INCREASE_HARD},
/* 77 */    { "stinging_weapons", SKILL_INCREASE_MEDIUM },
/* 78 */           { "blackjack", SKILL_INCREASE_HARD},
/* 79 */            { "appraise", SKILL_INCREASE_MEDIUM},
/* 80 */        { "natures lore", SKILL_INCREASE_MEDIUM},
/* 81 */         { "fire arrows", SKILL_INCREASE_HARD},
/* 82 */          { "ice arrows", SKILL_INCREASE_MEDIUM},
/* 83 */      { "tempest arrows", SKILL_INCREASE_EASY},
/* 84 */      { "granite arrows", SKILL_INCREASE_HARD},
/* 85 */         { "ignore_this", SKILL_INCREASE_HARD},
/* 86 */         { "ignorethiss",0},
/* 87 */      { "combat mastery", SKILL_INCREASE_HARD},
/* 88 */          { "rapid join", SKILL_INCREASE_EASY},
/* 89 */{"enhanced regeneration", SKILL_INCREASE_HARD},
/* 90 */             { "cripple", SKILL_INCREASE_HARD },
/* 91 */   { "natural selection", SKILL_INCREASE_HARD },
/* 92 */         {  "ignoreclan", 0 },
/* 93 */         { "ignoreclan2", 0 },
/* 94 */             { "commune", SKILL_INCREASE_HARD },
/* 95 */       { "scribe scroll", SKILL_INCREASE_HARD },
/* 96 */           { "make camp", SKILL_INCREASE_HARD },
/* 97 */         { "battlesense", SKILL_INCREASE_HARD },
/* 98 */        { "perseverance", SKILL_INCREASE_HARD },
/* 99 */              { "triage", SKILL_INCREASE_HARD },
/* 100*/               { "smite", SKILL_INCREASE_HARD },
/* 101*/          { "leadership", SKILL_INCREASE_HARD },
/* 102*/             { "execute", SKILL_INCREASE_HARD },
/* 103*/    { "defenders stance", SKILL_INCREASE_HARD },
/* 104*/              { "behead", SKILL_INCREASE_HARD },
/* 105 */   { "primal fury", SKILL_INCREASE_EASY},
/* 106 */   { "vigor", SKILL_INCREASE_MEDIUM},
/* 107 */   { "escape", SKILL_INCREASE_HARD},
/* 108 */   { "critical hit", SKILL_INCREASE_HARD},

/*    */                  { "\n", 0 },
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
  "hand_to_hand",
  "bullrush",
  "ferocity",
  "tactics",
  "deceit",
  "release",
  "fear_gaze",
  "eyegouge",
  "magic resist",
  "ignorethis",
  "spellcraft",
  "martial_defense",
  "knockback",
  "stinging_weapons",
  "blackjack",
  "appraise",
  "natures lore",
  "fire arrows",
  "ice arrows",
  "tempest arrows",
  "granite arrows",
  "ignoreme101",
  "Ignorethisaa",
  "combat mastery",
  "rapid join",
  "enhanced regeneration",
  "cripple",
  "natural selection",
  "ignoreclan",
  "ignoreclan2",
  "commune",
  "scribe scroll",
  "make camp",
  "battlesense",
  "perseverance",
  "triage",
  "smite",
  "leadership",
  "execute",
  "defenders stance",
  "behead",
  "primal fury",
  "vigor",
  "escape",
  "critical hit",
  "\n"
};

char *spells[]=
{
   "armor",               /* 1 */
   "teleport",
   "bless",
   "blindness",
   "burning hands",
   "iridescent aura",
   "charm person",
   "chill touch",
   "clone",  
   "colour spray",
   "control weather",     /* 11 */
   "create food",
   "create water",
   "remove blind",
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
   "call familiar",        /* 45 */
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
   "stoneskin",
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
   "camouflage",
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
   "stoneshield",
   "greater stoneshield",
   "iron roots",
   "eyes of the eagle",
   "unused",
   "ice shards",
   "lightning shield",
   "blue bird",
   "debility",
   "attrition",
   "vampiric aura",
   "holy aura",
   "dismiss familiar",
   "dismiss corpse",
   "blessed halo",
   "visage of hate",
   "protection from good",
   "oaken fortitude",
   "frostshield",
   "stability",
   "killer",
   "cantquit",
   "solidity",   
   "eas",
   "align-good",
   "align-evil",
   "holy aegis",
   "unholy aegis",
   "resist magic",
   "eagle eye",
   "call lightning",
   "divine fury",
   "spiritwalk",
   "mend golem",
   "clarity",
   "divine intervention",
   "wrath of god",
   "atonement",
   "silence",
   "immunity",
   "boneshield",
   "channel",
   "release elemental",
   "wild magic",
   "spirit shield",
   "\n"
};

// Figures out how many % of max your damage does
int dam_percent(int learned, int damage)
{
  float percent;
  percent = 50;
  if (!learned) percent /= 2;
  percent += learned/2;
//  else percent = 90 + ((learned - 90) *2);
  
  return (int)((float)damage * (float)percent/100.0);
}

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


void affect_update( int32 duration_type )
{
    static struct affected_type *af, *next_af_dude;
    static CHAR_DATA *i, * i_next;
    void update_char_objects( CHAR_DATA *ch ); /* handler.c */
    unsigned int faded_spells[20];
    int a =0;

    if (duration_type != PULSE_REGEN &&
	duration_type != PULSE_TIMER &&
	duration_type != PULSE_VIOLENCE &&
	duration_type != PULSE_TIME) // Default
      return;

    for (i = character_list; i; i = i_next) { 
      i_next = i->next;
//      if(!IS_NPC(i) ) // && !(i->desc)) Linkdeadness doens't save you 
//now.
  //      continue; 

        // This doesn't really belong here, but it beats creating an "update" just for it.
        // That way we don't have to traverse the entire list all over again
        if(duration_type == PULSE_TIME && !IS_NPC(i))
          update_char_objects(i);

      for (a=0; a < 20; a++)
	faded_spells[a] = 0;
      a=0;

      for (af = i->affected; af; af = next_af_dude) {
	next_af_dude = af->next;
	if (af->duration_type == 0 && duration_type == PULSE_TIME) {
	  // Business as usual
	} else if (af->duration_type != duration_type) {
	  continue;
	}
//	while (next_af_dude && next_af_dude->type == af->type) next_af_dude = next_af_dude->next;

	if ((af->type == FUCK_PTHIEF || af->type == FUCK_CANTQUIT || af->type == FUCK_GTHIEF) && !i->desc)
	  continue;
	if (af->duration > 1)
	  af->duration--;
	else if(af->duration == 1) {
          // warnings for certain spells
          switch(af->type) {
            case SKILL_SONG_SUBMARINERS_ANTHEM:
              send_to_char("Your musical ability to breath water weakens.\r\n", i);
              break;
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
		int z;
		bool fadeit = TRUE;
		for (z = 0;z < a;z++)
		{
		  if (faded_spells[z] == af->type)
		   fadeit = FALSE;
		} 
		if (fadeit) {
		if (a < 19)
  		  faded_spells[a++] = af->type;	
	        send_to_char(spell_wear_off_msg[af->type], i);
	        send_to_char("\n\r", i);
		}
	     }
//	  affect_from_char(i, af->type);
	  affect_remove(i, af, 0);
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
/* Stop the familiar without a master floods*/
void extractFamiliar(CHAR_DATA *ch)
{
    CHAR_DATA *victim = NULL;
    for(struct follow_type *k = ch->followers; k; k = k->next)
     if(IS_MOB(k->follower) && IS_AFFECTED(k->follower, AFF_FAMILIAR))
     {
        victim = k->follower;
        break;
     }

   if (NULL == victim)
      return;

   act("$n disappears in a flash of flame and shadow.", victim, 0, 0, TO_ROOM, 0);
   extract_char(victim, TRUE);
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
  if(ISSET(ch->affected_by, AFF_FAMILIAR)) {
    do_emote(ch, "screams in pain as its connection with its master is broken.", 9); 
    extract_char(ch, TRUE);
    return;
  }
*/
//  if(IS_AFFECTED(ch, AFF_CHARM)) {
  if(cmd == BROKE_CHARM) {

   if (GET_CLASS(ch->master) != CLASS_RANGER) {
    act("You realize that $N is a jerk!", ch, 0, ch->master, TO_CHAR, 0);
    act("$n is free from the bondage of the spell.", ch, 0, 0, TO_ROOM, 0);
    act("$n hates your guts!", ch, 0, ch->master, TO_VICT, 0);
   } else {
    act("You lose interest in $N.",ch,0,ch->master, TO_CHAR, 0);
     act("$n loses interest in $N.",ch,0, ch->master, TO_ROOM, NOTVICT);
     act("$n loses interest in you, and goes back to its business.",ch,0,ch->master,TO_VICT,0);
  }
    if (ch->fighting && ch->fighting != ch->master)
    {
      do_say(ch, "Screw this, I'm going home!",0);
      stop_fighting(ch->fighting);
      stop_fighting(ch);
    }
  }
  else {
    if(cmd == END_STALK) {
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

  /* do this after setting master to NULL, to prevent endless loop */
  /* between affect_remove() and stop_follower()                   */
  if (cmd != CHANGE_LEADER) {
     if(affected_by_spell(ch, SPELL_CHARM_PERSON))
       affect_from_char(ch, SPELL_CHARM_PERSON);
     REMBIT(ch->affected_by, AFF_CHARM);
     REMBIT(ch->affected_by, AFF_GROUP); 
  }
}



/* Called when a character that follows/is followed dies */
void die_follower(CHAR_DATA *ch)
{
    struct follow_type *j, *k;
    CHAR_DATA * zombie;
    
    if (ch->master)
	stop_follower(ch, STOP_FOLLOW);

    for (k = ch->followers; k; k = j) {
	j = k->next;
	zombie = k->follower;
        if(!ISSET(zombie->affected_by, AFF_GOLEM)) {
          if(affected_by_spell(zombie, SPELL_CHARM_PERSON))
             affect_from_char(zombie, SPELL_CHARM_PERSON);
	  stop_follower(zombie, STOP_FOLLOW);
        }
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
      REMBIT(ch->affected_by, AFF_GROUP);

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


void say_spell( CHAR_DATA *ch, int si, int room )
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

    CHAR_DATA *people;
    if (room > 0) {
      people = world[room].people;
    } else {
      people = world[ch->in_room].people;
    }

    for(temp_char = people;
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
int saves_spell(CHAR_DATA *ch, CHAR_DATA *vict, int spell_base, int16 save_type)
{
    double save = 0;

    // Gods always succeed saving throws.  We rock!
    if (IS_MINLEVEL_PC(vict, IMMORTAL)) {
        return 100;
    }

    // If a God attacks you, nothing can save you
    if (IS_MINLEVEL_PC(ch, DEITY) && GET_LEVEL(vict) < GET_LEVEL(ch)) {
      return -100;
    }

    // Get the base save type for this roll
    switch(save_type) {
      case SAVE_TYPE_FIRE:
            save = get_saves(vict, SAVE_TYPE_FIRE);
            break;
      case SAVE_TYPE_COLD:
            save = get_saves(vict, SAVE_TYPE_COLD);
            break;
      case SAVE_TYPE_ENERGY:
            save = get_saves(vict, SAVE_TYPE_ENERGY);
            break;
      case SAVE_TYPE_ACID:
            save = get_saves(vict, SAVE_TYPE_ACID);
            break;
      case SAVE_TYPE_MAGIC:
            save = get_saves(vict, SAVE_TYPE_MAGIC);
            // ISR Magic has to affect saving throws as well as damage so they don't get
            // para'd or slept or something
            if(IS_SET(vict->immune, ISR_MAGIC))       return(TRUE);
            if(IS_SET(vict->suscept, ISR_MAGIC))      save *= 0.7;
            if(IS_SET(vict->resist, ISR_MAGIC))       save *= 1.3;
            break;
      case SAVE_TYPE_POISON:
            save = get_saves(vict, SAVE_TYPE_POISON);
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

/* 
    Release command. 
*/
int do_release(CHAR_DATA *ch, char *argument, int cmd)
{
  struct affected_type *aff,*aff_next;
  bool printed = FALSE;
  argument = skip_spaces(argument);
  extern bool str_prefix(const char *astr, const char *bstr);  
  bool done = FALSE;
  int learned = has_skill(ch,SKILL_RELEASE);

  if (!learned)
  {
     send_to_char("You don't know how!\r\n",ch);
     return eFAILURE;
  }

  if (!*argument)
  {
    send_to_char("Release what spell?\r\n",ch);
    for (aff = ch->affected; aff; aff = aff_next)
    {
       aff_next = aff->next;
//       while (aff_next && aff_next->type == aff->type)
//	aff_next = aff_next->next;
       if (!get_skill_name(aff->type))
          continue;
       if (!printed)
       {
	  send_to_char("You can release the following spells:\r\n",ch);
	  printed=TRUE;
       }
       if ( ((spell_info[aff->type].targets & TAR_SELF_DEFAULT) || 
             aff->type == SPELL_HOLY_AURA) && aff->type != SPELL_IMMUNITY)
       { // Spells that default to self seems a good measure of
	 // allow to release spells..
         char * aff_name = get_skill_name(aff->type);
	 send_to_char(aff_name,ch);
         send_to_char("\r\n",ch);
       }
    }
     return eSUCCESS;
    } else {
       if (ch->move < 25)
       {
	send_to_char("You don't have enough moves.\r\n",ch);
	return eFAILURE;
       }

       for (aff = ch->affected; aff; aff = aff_next)
	 {
        aff_next = aff->next;
	while (aff_next && aff_next->type == aff->type)
         aff_next = aff_next->next;

         if (!get_skill_name(aff->type))
            continue;
	 if (str_prefix(argument,get_skill_name(aff->type)))
            continue;
	if (aff->type > MAX_SPL_LIST) continue;
          if (!IS_SET(spell_info[aff->type].targets, TAR_SELF_DEFAULT) && aff->type != SPELL_HOLY_AURA)
            continue;
         if ((aff->type > 0) && (aff->type <= MAX_SPL_LIST))
	       if (!done && !skill_success(ch,NULL, SKILL_RELEASE))
       {
         send_to_char("You failed to release the spell, and are left momentarily dazed.\r\n",ch);
	 act( "$n fails to release the magic surrounding $m and is left momentarily dazed.", ch, 0, 0, TO_ROOM , INVIS_NULL);
         WAIT_STATE(ch,PULSE_VIOLENCE/2);
	 ch->move -= 10;
         return eFAILURE;
       }

	if (!done)
	  ch->move -= 25;
	send_to_char("You release the spell.\r\n",ch);
	char buffer[255];
	int aftype = aff->type;
	
             if (*spell_wear_off_msg[aff->type]) {
                send_to_char(spell_wear_off_msg[aftype], ch);
                send_to_char("\n\r", ch);
             }
	  affect_from_char(ch, aftype);
//	  affect_remove(ch,aff,0);
	snprintf(buffer, 255, "$n concentrates for a moment and releases their %s.", get_skill_name(aftype));
	act( buffer, ch, 0, 0, TO_ROOM , INVIS_NULL);

	 done = TRUE;
	 }
    }
    if (!done)
    send_to_char("No such spell to release.\r\n",ch);
    return eSUCCESS;
}

int skill_value(CHAR_DATA *ch, int skillnum, int min = 33)
{
  struct char_skill_data * curr = ch->skills;

  while(curr) {
    if(curr->skillnum == skillnum)
      return MAX(min, (int)curr->learned);
    curr = curr->next;
  }
  return 0;
}

int stat_mod [] = {
0,-5,-5,-4,-4,-3,-3,-2,-2,-1,-1,
0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,
7,8,9,10
};
extern int skillmax(struct char_data *ch, int skill, int eh);

int get_difficulty(int skillnum)
{
//  extern struct skill_stuff skill_info[];
  extern struct ki_info_type ki_info [ ];
  extern struct song_info_type song_info[];

  if (skillnum >= SKILL_BASE && skillnum <= SKILL_MAX) 
    return skill_info[skillnum - SKILL_BASE].difficulty;
  if (skillnum >= KI_OFFSET && skillnum <= KI_OFFSET+MAX_KI_LIST)
     return ki_info[skillnum - KI_OFFSET].difficulty;
  if (skillnum >= SKILL_SONG_BASE && skillnum <= SKILL_SONG_MAX)
     return song_info[skillnum - SKILL_SONG_BASE].difficulty;

  return 0;
}


bool skill_success(CHAR_DATA *ch, CHAR_DATA *victim, int skillnum, int mod )
{
//  extern int stat_mod[];
//  int modifier = 0;
  extern class_skill_defines *get_skill_list(char_data *ch);
  extern int get_stat(CHAR_DATA *ch, int stat);
  //struct class_skill_defines *t;
  int stat=0;


  switch (skillnum)
  {
     case SKILL_AMBUSH:
       stat = INT;
	break;
     case SKILL_KICK:
	stat = DEX;
	break;
     case SKILL_BASH:
	stat = STR;
	break;
     case SKILL_RAGE:
	stat = CON;
	break;
     case SKILL_BERSERK:
	stat = STR;
	break;
     case KI_OFFSET+KI_PUNCH:
	stat = DEX;
	break;
     case KI_OFFSET+KI_DISRUPT:
	stat = INT;
	break;
     case SKILL_DISARM:
	stat = DEX;
	break;
     case SKILL_TRACK:
	stat = WIS;
	break;
     case SKILL_BULLRUSH:
	stat = STR;
	break;
     case SKILL_HEADBUTT:
	stat = CON;
	break;
     case SKILL_HITALL:
	stat = STR;
	break;
     case SKILL_STUN:
	stat = DEX;
	mod -= GET_DEX(victim) / 2; // ADDITIONAL mod
	break;
     case SKILL_DEATHSTROKE:
	stat = STR;
	break;
     case SKILL_QUIVERING_PALM:
	stat = STR;
	break;
     case SKILL_EAGLE_CLAW:
	stat = STR;
	break;
     case SKILL_BACKSTAB:
	stat = DEX;
	break;
     case SKILL_ARCHERY:
	stat = DEX;
	break;
     case SKILL_DUAL_BACKSTAB:
	stat = DEX;
	break;
     case SKILL_CIRCLE:
	stat = DEX;
	break;
     case SKILL_TRIP:
	stat = DEX;
	break;
     case SKILL_STEAL:
	stat = DEX;
	break;
     case SKILL_POCKET:
	stat = INT;
	break;
     case SKILL_STALK:
	stat = CON;
     case SKILL_CONSIDER:
        stat = WIS;
	break;
     case SKILL_EYEGOUGE: 
	stat = CON;
	break;
       }
  int i = 0,learned = 0;

// if (!IS_NPC(ch)) debug_point();
    if (!IS_MOB(ch)) {
        i = learned = has_skill(ch, skillnum);
        if(affected_by_spell(ch, SKILL_DEFENDERS_STANCE) && skillnum == SKILL_DODGE) {
          learned = affected_by_spell(ch, SKILL_DEFENDERS_STANCE)->modifier;
send_to_char("got here", ch);
}
        else if(!learned) return FALSE;
    }
    else {
	if (GET_LEVEL(ch) < 30) i = 30;
	else if (GET_LEVEL(ch) < 50) i =40;
	else if (GET_LEVEL(ch) < 70) i =60;
	else if (GET_LEVEL(ch) < 90) i =70;
	else i = 75;
   }
    if (stat && victim)
	i -= stat_mod[get_stat(victim,stat)] * GET_LEVEL(ch) / 60; // less impact on low levels..
  i += mod;

  if (!IS_NPC(ch))
  i = 50 + i / 2;

  if (skillnum != SKILL_THIRD_ATTACK && skillnum != SKILL_SECOND_ATTACK && skillnum != SKILL_DUAL_WIELD)
    i = MIN(96, i);
  else i = MIN(98,i);

  i = skillmax(ch, skillnum, i);
  if (IS_AFFECTED(ch, AFF_FOCUS) && 
((skillnum >= SKILL_SONG_BASE && 
skillnum <= SKILL_SONG_MAX) || (skillnum >= KI_OFFSET && skillnum <= (KI_OFFSET+MAX_KI_LIST))))
   i = 101; // auto success on songs and ki with focus

  int a = get_difficulty(skillnum);
/*  int o = GET_LEVEL(ch)*2+1;
  if (o > 101 || IS_NPC(ch)) o = 101;
  if (i > o) o = i+1;
*/

  if (i > number(1,101) || GET_LEVEL(ch) >= IMMORTAL)
  {
    if(skillnum != SKILL_ENHANCED_REGEN || ( skillnum == SKILL_ENHANCED_REGEN && GET_HIT(ch) + 50 < GET_MAX_HIT(ch) && ( GET_POS(ch) == POSITION_RESTING || GET_POS(ch) == POSITION_SLEEPING ) ) )
      skill_increase_check(ch,skillnum,learned,a+500);
    return TRUE; // Success
  }
  else {
  /* Check for skill improvement anyway */
    if(skillnum != SKILL_ENHANCED_REGEN || ( skillnum == SKILL_ENHANCED_REGEN && GET_HIT(ch) + 50 < GET_MAX_HIT(ch) && ( GET_POS(ch) == POSITION_RESTING || GET_POS(ch) == POSITION_SLEEPING ) ) )
      skill_increase_check(ch, skillnum, learned,a);
    return FALSE; // Failure  
  }
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

  if (IS_NPC(ch) && ch->desc && ch->desc->original && ch->desc->original != ch->desc->character && GET_LEVEL(ch->desc->original) < IMMORTAL) {
    send_to_char("You cannot cast in this form.\n\r", ch);
    return eFAILURE;
  }

  if(affected_by_spell(ch, SPELL_NO_CAST_TIMER)) {
    send_to_char("You seem unable to concentrate enough to cast any spells.\n\r", ch);
    return eFAILURE;
  }
  
  OBJ_DATA *tmp_obj;
  for(tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) {
      send_to_char("The magical silence prevents you from casting!\n\r", ch);
      return eFAILURE;
    }
  
  if(IS_AFFECTED(ch, AFF_CHARM))
  {
    send_to_char("You cannot cast while charmed!\n\r", ch);
    return eFAILURE;
  }

  if (GET_LEVEL(ch) < ARCHANGEL && (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM))) {
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
  if (spl == SPELL_DIVINE_INTER && affected_by_spell(ch, SPELL_DIV_INT_TIMER)) {
    send_to_char("The gods are unwilling to intervene on your behalf again so soon.\n\r", ch);
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
          send_to_char("Impossible! You can't concentrate enough!\n\r", ch);
          break;
        default:
          send_to_char("It seems like you're in a pretty bad shape!\n\r",ch);
          break;
      } /* Switch */
    } else 
    {
      if (!IS_MOB(ch))
      {
        if(!(learned = has_skill(ch, spl))) {
	if (GET_LEVEL(ch) < 101) {
          send_to_char("You do not know how to cast that spell!\n\r", ch);
          return eFAILURE;
        } else {
	  learned = 80;
	}
        }
      }
      else learned = 80;

      argument+=qend+1; /* Point to the last ' */
      for(;*argument == ' '; argument++);
      
      /* **************** Locate targets **************** */
      
      target_ok = FALSE;
      tar_char = 0;
      tar_obj = 0;
      bool ok_self = FALSE;
      int oldroom = 0;
      int dir = -1;
      bool group_spell = FALSE;
      if (spl == SPELL_LIGHTNING_BOLT && has_skill(ch, SKILL_SPELLCRAFT))
      { // Oh the special cases of spellcraft.
	 name[0] = '\0';
         one_argument(argument, name);
	 if (argument && strlen(argument) > strlen(name))
	 {
	   *argument = LOWER(*(argument + strlen(name) +1));
 	   if(*argument == 'n') dir = 0;
	   else if(*argument == 'e') dir = 1;
 	   else if (*argument == 's') dir = 2;
	   else if(*argument == 'w') dir = 3;
	   else if(*argument == 'u') dir = 4;
	   else if(*argument == 'd') dir = 5;
	   if (dir == -1)
	   {
		send_to_char("Fire a lightning bolt where?\r\n",ch);
		return eFAILURE;
	   }
	   if (!world[ch->in_room].dir_option[dir])
	   {
		send_to_char("The wall blocks your attempt.\r\n",ch);
		return eFAILURE;
	   }
	   if (!CAN_GO(ch, dir))
	   {
		send_to_char("You cannot do that.\r\n",ch);
		return eFAILURE;
	   }
           if(ch->fighting) {
             send_to_char("You cannot concentrate enough to fire a bolt of lightning into another room!\n\r", ch);
             return eFAILURE;
           }
	   int new_room = world[ch->in_room].dir_option[dir]->to_room;
	   if(IS_SET(world[new_room].room_flags, SAFE) || IS_SET(world[new_room].room_flags, NO_MAGIC))
	   {
   	     send_to_char("That room is protected from this harmful magic.\r\n", ch);
	     return eFAILURE;
	   }

	   // can't use spellcraft(ch, SPELL_LIGHTNING_BOLT) here because it
	   // will cause spellcraft to increase possibly
	   if ((has_skill(ch, SPELL_LIGHTNING_BOLT) < 71) ||
	       (has_skill(ch, SKILL_SPELLCRAFT) < 21)) {
	     send_to_char("You don't know how.\r\n",ch);
	     return eFAILURE;
           }

	   oldroom = ch->in_room;
	   char_from_room(ch);
	   if (!char_to_room(ch, new_room)) {
	     char_to_room(ch, oldroom);
	    send_to_char("Error code: 57A. Report this to an immortal, along with what you typed and where.\r\n",ch);
	    return eFAILURE;
	   }
	   if (!(tar_char = get_char_room_vis(ch, name)))
	   {
		char_from_room(ch); 
		char_to_room(ch, oldroom);
	        send_to_char("You don't see anyone like that there.\r\n",ch);
		return eFAILURE;
	   }

	   if (IS_NPC(tar_char) && mob_index[tar_char->mobdata->nr].virt >= 2300 &&
		mob_index[tar_char->mobdata->nr].virt <= 2399)
	   {
   	        char_from_room(ch); 
		char_to_room(ch, oldroom);
		tar_char = ch;
		send_to_char("Your spell bounces off the fortress' enchantments, and the lightning bolt comes flying back towards you!\r\n",ch);
	 	ok_self = TRUE;
	   }
	   target_ok = TRUE;

	   // Reduce timer on paralyze even the victim is hit by a lightning bolt
	   affected_type *af;
	   if ((af = affected_by_spell(tar_char, SPELL_PARALYZE)) != NULL) {
	     af->duration--;
	     if (af->duration <= 0) {
	       affect_remove(tar_char, af, 0);
	     }
	   }
	 }
	 spellcraft(ch, SPELL_LIGHTNING_BOLT);
      }
      if(spl == SPELL_IMMUNITY) {
        argument = skip_spaces(argument);

        for(int i = 0;i<MAX_SPL_LIST;i++) {
          if(!strcmp(spells[i], argument)) {
            ok_self = TRUE;
            tar_char = ch;
            target_ok = TRUE;
            argument = (char *)i;
            break;
          }
        }
        if(!target_ok) {
          send_to_char("There is no such known spell in the realms to protect yourself against.\n\r", ch);
          return eFAILURE;
        }
      }
      if (!target_ok && !IS_SET(spell_info[spl].targets, TAR_IGNORE)) 
      {
        argument = one_argument(argument, name);

        if (*name) 
        {
          if (IS_SET(spell_info[spl].targets, TAR_CHAR_ROOM)) {
            if ( ( tar_char = get_char_room_vis(ch, name) ) != NULL )
              target_ok = TRUE;
            if(!str_cmp(name, "group") && has_skill(ch, SKILL_COMMUNE)) {
              if( (has_skill(ch, SKILL_COMMUNE) >= 90 && (spl == SPELL_PROTECT_FROM_EVIL || spl == SPELL_PROTECT_FROM_GOOD)) || 
                  (has_skill(ch, SKILL_COMMUNE) >= 70 && (spl == SPELL_CURE_CRITIC || spl == SPELL_SANCTUARY || spl == SPELL_IRIDESCENT_AURA)) ||
                  (has_skill(ch, SKILL_COMMUNE) >= 40 && (spl == SPELL_ARMOR || spl == SPELL_REFRESH || spl == SPELL_CURE_SERIOUS || spl == SPELL_BLESS || spl == SPELL_FLY)) || 
                  (spl == SPELL_DETECT_INVISIBLE || spl == SPELL_DETECT_MAGIC || spl == SPELL_DETECT_POISON || spl == SPELL_CURE_LIGHT))
              {
                skill_increase_check(ch, SKILL_COMMUNE, has_skill(ch, SKILL_COMMUNE), SKILL_INCREASE_HARD);
                target_ok = TRUE;
                group_spell = TRUE;
                tar_char = ch;
              } else {
                send_to_char("You cannot cast this spell on your entire group at once.\n\r", ch);
                return eFAILURE;
              }          
            }
          }

          if (!target_ok && IS_SET(spell_info[spl].targets, TAR_CHAR_WORLD))
	  {
		bool orig = ISSET(ch->affected_by, AFF_TRUE_SIGHT);
		if (spl == SPELL_EAGLE_EYE)
		  SETBIT(ch->affected_by, AFF_TRUE_SIGHT);
            if ( ( tar_char = get_char_vis(ch, name) ) != NULL )
              target_ok = TRUE;
		if (!orig)
		  REMBIT(ch->affected_by, AFF_TRUE_SIGHT);
          }
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
            if ( ( tar_obj = get_obj_vis(ch, name, TRUE) ) != NULL)
/* && !(IS_SET(tar_obj->obj_flags.more_flags, ITEM_NOLOCATE)))*/
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
        if ((tar_char == ch) && !ok_self && IS_SET(spell_info[spl].targets, TAR_SELF_NONO)) {
	  if (oldroom) {
		char_from_room(ch); 
		char_to_room(ch, oldroom);
	  }
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
	  if (oldroom) {
		char_from_room(ch); 
		char_to_room(ch, oldroom);
	  }
          send_to_char("You can't summon enough energy to cast the spell.\n\r",ch);
          return eFAILURE;
        }
      }
     if (tar_char && IS_SET(spell_info[spl].targets, TAR_FIGHT_VICT))
     {
	  if (!can_attack(ch) || !can_be_attacked(ch, tar_char))
	  {
	  if (oldroom) {
		char_from_room(ch); 
		char_to_room(ch, oldroom);
	  }
	    return eFAILURE;
	  }
     }

      if (spl != SPELL_VENTRILOQUATE) /* :-) */
	say_spell(ch, spl, oldroom);

	if (!spellcraft(ch, spl) || (spl != SPELL_MAGIC_MISSILE && spl != SPELL_FIREBALL) )
      WAIT_STATE(ch, spell_info[spl].beats);
	else 
	WAIT_STATE(ch, (int)(spell_info[spl].beats/1.5));
      
      if ((spell_info[spl].spell_pointer == 0) && spl>0)
        send_to_char("Sorry, this magic has not yet been implemented :(\n\r", ch);
      else 
      {
	int chance;

        if(IS_MOB(ch)) {
          learned = GET_LEVEL(ch);
          chance = 50;
        }
        else
	chance = 40;
	

	chance += learned / 2;

	if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL)
	  chance += int_app[GET_INT(ch)].conc_bonus;
	else 
          chance += wis_app[GET_WIS(ch)].conc_bonus;
	extern int get_max(CHAR_DATA *ch, int skill);

//	chance = MIN(get_max(ch, spl), chance);
        if (GET_RACE(ch) == RACE_HUMAN)
  	  chance = MIN(95, chance);
	else
	  chance = MIN(97, chance);
//	int o = GET_LEVEL(ch)*2+1;
 //       if (chance > o) o = chance + 1;
//	if (o > 101) o = 101; // imms/mobs
//        if ((float)((float)chance/(float)o) < 0.4) chance = (int)(o*0.4);

	if (GET_LEVEL(ch) == 1) chance++;
        if(IS_AFFECTED(ch, AFF_CRIPPLE) && affected_by_spell(ch, SKILL_CRIPPLE))
           chance -= 1 + affected_by_spell(ch, SKILL_CRIPPLE)->modifier/10;
        if(GET_LEVEL(ch) < IMMORTAL && number(1,100) > chance && !IS_AFFECTED(ch,AFF_FOCUS) && !IS_SET(world[ch->in_room].room_flags, SAFE) )
        {
          csendf(ch, "You lost your concentration and are unable to cast %s!\n\r", spells[spl-1]);
          GET_MANA(ch) -= (use_mana(ch, spl) >> 1);
          act("$n loses $s concentration and is unable to complete $s spell.", ch, 0, 0, TO_ROOM, 0);
	  skill_increase_check(ch, spl, learned, spell_info[spl].difficulty);
	  if (oldroom) {
		char_from_room(ch); 
		char_to_room(ch, oldroom);
	  }
          return eSUCCESS;
        }

        if (IS_AFFECTED(ch, AFF_INVISIBLE) && !IS_AFFECTED(ch, AFF_ILLUSION)
		&& affected_by_spell(ch, SPELL_INVISIBLE)) {
           act("$n slowly fades into existence.", ch, 0, 0, TO_ROOM, 0);
           affect_from_char(ch, SPELL_INVISIBLE);
           REMBIT(ch->affected_by, AFF_INVISIBLE);
        }

        send_to_char("Ok.\n\r", ch);

        if(group_spell) {
          CHAR_DATA *leader;
          if(ch->master) leader = ch->master;
          else leader = ch;

          struct follow_type *k;
          int counter = 0;

          for(k = leader->followers; k; k = k->next)
            if(k->follower->in_room == ch->in_room) 
              counter++;
          if(leader->in_room == ch->in_room) counter++;

          counter = MIN(5, counter);
          if(learned >= 80 && counter > 3) counter--;
          if(learned >= 90 && counter > 3) counter--;
          counter *= use_mana(ch, spl);
          if(GET_MANA(ch) < counter) {
            send_to_char("You do not have enough mana to cast this group spell.\n\r", ch);
            return eFAILURE;
          }
          GET_MANA(ch) -= counter;
        } else GET_MANA(ch) -= (use_mana(ch, spl));
        if (tar_char && !AWAKE(tar_char) && ch->in_room == tar_char->in_room && number(1,5) < 3)
            send_to_char("Your sleep is restless.\r\n",tar_char);
	 skill_increase_check(ch, spl, learned,500+ spell_info[spl].difficulty);

	 if (tar_char && tar_char != ch && IS_PC(ch) && IS_PC(tar_char) && tar_char->desc && ch->desc) {
	   if (!strcmp(tar_char->desc->host, ch->desc->host)) {
	     sprintf( log_buf, "Multi: %s casted '%s' on %s", GET_NAME(ch),
		      get_skill_name(spl), GET_NAME(tar_char));
	     log( log_buf, 110, LOG_PLAYER, ch );
	   }

	   // Wizard's eye (88) is ok to cast
	   // Prize Arena
	   if (IS_SET(world[ch->in_room].room_flags, ARENA) && arena.type == PRIZE && spl != 88) {
	       if (tar_char && tar_char != ch && !IS_SET(spell_info[spl].targets, TAR_FIGHT_VICT)) {
		   send_to_char("You can't cast that spell on someone in a prize arena.\n\r", ch);
		   logf(IMMORTAL, LOG_ARENA, "%s was prevented from casting '%s' on %s.",
			GET_NAME(ch), get_skill_name(spl), GET_NAME(tar_char));
		   return eFAILURE;
	       }

	       if (ch->fighting && ch->fighting != tar_char) {
		   send_to_char("You can't cast that because you're in a fight with someone else.\n\r", ch);
		   logf(IMMORTAL, LOG_ARENA, "%s, whom was fighting %s, was prevented from casting '%s' on %s.", GET_NAME(ch),
			GET_NAME(ch->fighting), get_skill_name(spl), GET_NAME(tar_char));
		   return eFAILURE;
	       } else if (tar_char->fighting && tar_char->fighting != ch) {
		   send_to_char("You can't cast that because they are fighting someone else.\n\r", ch);
		   logf(IMMORTAL, LOG_ARENA, "%s was prevented from casting '%s' on %s who was fighting %s.", GET_NAME(ch),
			get_skill_name(spl), GET_NAME(tar_char), GET_NAME(tar_char->fighting));
		   return eFAILURE;
	       }
	   }
  	 

	 // Wizard's eye (88) is ok to cast
	 // Clan Chaos
	   if (IS_SET(world[ch->in_room].room_flags, ARENA) && arena.type == CHAOS && spl != 88) {
	       if (tar_char && tar_char != ch && !IS_SET(spell_info[spl].targets, TAR_FIGHT_VICT) && !ARE_CLANNED(ch, tar_char)) {
	       send_to_char("You can't cast that spell on someone from another clan in a prize arena.\n\r", ch);
	       logf(IMMORTAL, LOG_ARENA, "%s [%s] was prevented from casting '%s' on %s [%s].",
		    GET_NAME(ch), get_clan_name(ch), get_skill_name(spl), GET_NAME(tar_char), get_clan_name(tar_char));
	       return eFAILURE;
	     }

	       if (ch->fighting && ch->fighting != tar_char && !ARE_CLANNED(ch->fighting, tar_char) && IS_SET(spell_info[spl].targets, TAR_FIGHT_VICT)) {
	       send_to_char("You can't cast that because you're in a fight with someone else.\n\r", ch);
	       logf(IMMORTAL, LOG_ARENA, "%s [%s], whom was fighting %s [%s], was prevented from casting '%s' on %s [%s].", 
		    GET_NAME(ch), get_clan_name(ch),
		    GET_NAME(ch->fighting), get_clan_name(ch->fighting),
		    get_skill_name(spl),
		    GET_NAME(tar_char), get_clan_name(tar_char));
	       return eFAILURE;
	       } else if (tar_char->fighting && tar_char->fighting != ch && !ARE_CLANNED(tar_char->fighting, ch) && IS_SET(spell_info[spl].targets, TAR_FIGHT_VICT)) {
	       send_to_char("You can't cast that because they are fighting someone else.\n\r", ch);
	       logf(IMMORTAL, LOG_ARENA, "%s [%s] was prevented from casting '%s' on %s [%s] who was fighting %s [%s].",
		    GET_NAME(ch), get_clan_name(ch),
		    get_skill_name(spl),
		    GET_NAME(tar_char), get_clan_name(tar_char),
		    GET_NAME(tar_char->fighting), get_clan_name(tar_char->fighting));
	       return eFAILURE;
	     }
	   }
  	 }
    
         if (tar_char && IS_AFFECTED(tar_char, AFF_REFLECT) && number(0,99) < tar_char->spell_reflect) {
           if(ch == tar_char) { // some idiot was shooting at himself
//out		  act("The spell harmlessly reflects off you and disperses.", tar_char, 0, 0, TO_CHAR, 0);
//for		  act("The spell harmlessly reflects off $n and disperses.", tar_char, 0, 0, TO_ROOM, 0);
//heals		  return eSUCCESS;
	   } else {
		  act("$n's spell bounces back at $m.", ch, 0, tar_char, TO_VICT, 0);
		  act("Oh SHIT! Your spell bounces off of $N and heads right back at you.", ch, 0, tar_char, TO_CHAR, 0);
		  act("$n's spell reflects off of $N's magical aura", ch, 0, tar_char, TO_ROOM, NOTVICT);
		  tar_char = ch;

		  // Ping-pong
		   if (IS_AFFECTED(tar_char, AFF_REFLECT) && number(0,99) < tar_char->spell_reflect) {
		     act("The spell harmlessly reflects off you and disperses.", tar_char, 0, 0, TO_CHAR, 0);
		     act("The spell harmlessly reflects off $n and disperses.", tar_char, 0, 0, TO_ROOM, 0);
		     return eSUCCESS;
	           }
	    }
        }
    
        if(group_spell) {
           send_to_char("You utter a swift prayer to the gods to amplify your powers.\n\r", ch);
           act("$n utters a swift prayer to the gods to amplify $s powers.", ch, 0, 0, TO_ROOM, 0);
           strcpy(argument,"communegroupspell");
        }
        else if(tar_char && affected_by_spell(tar_char, SPELL_IMMUNITY) && affected_by_spell(tar_char, SPELL_IMMUNITY)->modifier == spl-1) {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_char, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_char, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_char, TO_ROOM, NOTVICT);
          return eSUCCESS;
        }

	int retval = ((*spell_info[spl].spell_pointer) (GET_LEVEL(ch), ch, argument, SPELL_TYPE_SPELL, tar_char, tar_obj, learned));

	if (oldroom && !IS_SET(retval, eCH_DIED)) {
	  char_from_room(ch);
	  char_to_room(ch, oldroom);
	  WAIT_STATE(ch, (int)(spell_info[spl].beats));

	  if (spl == SPELL_LIGHTNING_BOLT) {
	    char buffer[MAX_STRING_LENGTH];
	    strcpy(buffer, "$n unleashes a bolt of $B$5lightning$R to the ");
	    switch (dir) {
	    case 0:
	      strcat(buffer, "north.");
	      break;
	    case 1:
	      strcat(buffer, "east.");
	      break;
	    case 2:
	      strcat(buffer, "south.");
	      break;
	    case 3:
	      strcat(buffer, "west.");
	      break;
	    case 4:
	      strcat(buffer, "area above you.");
	      break;
	    case 5:
	      strcat(buffer, "area below you.");
	      break;
	    }
	    act(buffer, ch, 0, 0, TO_ROOM, 0);
	  }
	}

        return retval;
    }
    }   /* if GET_POS < min_pos */
    return eFAILURE;
  }
  return eFAILURE;
}

int do_skills(CHAR_DATA *ch, char *arg, int cmd)
{
   char buf[16384];
   char buf2[MAX_STRING_LENGTH],buf3[MAX_STRING_LENGTH];
   int mage, cleric, thief, warrior, anti, pal, barb, monk, ranger, bard, druid;
   if(IS_NPC(ch)) return eFAILURE;

   buf[0] = '\0';

   for(int i = SKILL_BASE; i <= SKILL_MAX; i++) {
      mage = 0;
      cleric = 0;
      thief = 0;
      warrior = 0;
      anti = 0;
      pal = 0;
      barb = 0;
      monk = 0;
      ranger = 0;
      bard = 0;
      druid = 0;
      buf2[0] = '\0';
      for(int j = 0; m_skills[j].skillnum; j++) {
         if(m_skills[j].skillnum == i) {
            mage = j;
            sprintf(buf2, "Mag(%d)", m_skills[j].levelavailable);
            break;
         }
      }
      for(int j = 0; c_skills[j].skillnum; j++) {
         if(c_skills[j].skillnum == i) {
            cleric = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Cle(%d)", c_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; t_skills[j].skillnum; j++) {
         if(t_skills[j].skillnum == i) {
            thief = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Thi(%d)", t_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; w_skills[j].skillnum; j++) {
         if(w_skills[j].skillnum == i) {
            warrior = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "War(%d)", w_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; a_skills[j].skillnum; j++) {
         if(a_skills[j].skillnum == i) {
            anti = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Ant(%d)", a_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; p_skills[j].skillnum; j++) {
         if(p_skills[j].skillnum == i) {
            pal = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Pal(%d)", p_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; b_skills[j].skillnum; j++) {
         if(b_skills[j].skillnum == i) {
            barb = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Bar(%d)", b_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; k_skills[j].skillnum; j++) {
         if(k_skills[j].skillnum == i) {
            monk = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Mon(%d)", k_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; r_skills[j].skillnum; j++) {
         if(r_skills[j].skillnum == i) {
            ranger = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Ran(%d)", r_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; d_skills[j].skillnum; j++) {
         if(d_skills[j].skillnum == i) {
            bard = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Brd(%d)", d_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; u_skills[j].skillnum; j++) {
         if(u_skills[j].skillnum == i) {
            druid = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Dru(%d)", u_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      if(mage) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
		 UPPER(*m_skills[mage].skillname), m_skills[mage].skillname+1, skill_cost.find(i)->second);
      } else if(cleric) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
		 UPPER(*c_skills[cleric].skillname),c_skills[cleric].skillname+1, skill_cost.find(i)->second);
      } else if(thief) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
		 UPPER(*t_skills[thief].skillname),t_skills[thief].skillname+1, skill_cost.find(i)->second);
      } else if(warrior) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
               UPPER(*w_skills[warrior].skillname),w_skills[warrior].skillname+1, skill_cost.find(i)->second);
      } else if(anti) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
               UPPER(*a_skills[anti].skillname),a_skills[anti].skillname+1, skill_cost.find(i)->second);
      } else if(pal) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
               UPPER(*p_skills[pal].skillname),p_skills[pal].skillname+1, skill_cost.find(i)->second);
      } else if(barb) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
               UPPER(*b_skills[barb].skillname),b_skills[barb].skillname+1, skill_cost.find(i)->second);
      } else if(monk) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
               UPPER(*k_skills[monk].skillname),k_skills[monk].skillname+1, skill_cost.find(i)->second);
      } else if(ranger) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
               UPPER(*r_skills[ranger].skillname),r_skills[ranger].skillname+1, skill_cost.find(i)->second);
      } else if(bard) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
               UPPER(*d_skills[bard].skillname),d_skills[bard].skillname+1, skill_cost.find(i)->second);
      } else if(druid) {
         sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
               UPPER(*u_skills[druid].skillname),u_skills[druid].skillname+1, skill_cost.find(i)->second);
      } else continue;

      strcat(buf, buf2);
      strcat(buf, "\n\r");
   }

   strcat(buf, "\n\r");
   page_string(ch->desc, buf, 1);

   return eSUCCESS;
}

int do_songs(CHAR_DATA *ch, char *arg, int cmd)
{
   char buf[16384];

   if (IS_NPC(ch))
      return eFAILURE;

   buf[0] = '\0';

   for(int i = 0; d_skills[i].skillnum; i++) {
      if(d_skills[i].skillnum >= SKILL_SONG_BASE && d_skills[i].skillnum <= SKILL_SONG_MAX) {
         sprintf(buf + strlen(buf), "$B$7Song:$R %c%-22s  $B$7Ki:$R %-3d  $B$7Class:$R %s (%d)",
               UPPER(*d_skills[i].skillname),d_skills[i].skillname+1,
               song_info[d_skills[i].skillnum - SKILL_SONG_BASE].min_useski,
               "Brd", d_skills[i].levelavailable);
         strcat(buf, "\n\r");
      }
   }

   strcat(buf, "\n\r");
   page_string(ch->desc, buf, 1);

   return eSUCCESS;
}

int do_spells(CHAR_DATA *ch, char *arg, int cmd)
{
   char buf[16384];
   char buf2[MAX_STRING_LENGTH],buf3[MAX_STRING_LENGTH];
   int mage, cleric, anti, pal, ranger, druid;

   if (IS_NPC(ch))
      return eFAILURE;

   buf[0] = '\0';

   for(int i = 0; i <= MAX_SPL_LIST; i++) {
      mage = 0;
      cleric = 0;
      anti = 0;
      pal = 0;
      ranger = 0;
      druid = 0;
      buf2[0] = '\0';
      for(int j = 0; m_skills[j].skillnum; j++) {
         if(m_skills[j].skillnum == i) {
            mage = j;
            sprintf(buf2, "Mag(%d)", m_skills[j].levelavailable);
            break;
         }
      }
      for(int j = 0; c_skills[j].skillnum; j++) {
         if(c_skills[j].skillnum == i) {
            cleric = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Cle(%d)", c_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; a_skills[j].skillnum; j++) {
         if(a_skills[j].skillnum == i) {
            anti = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Ant(%d)", a_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; p_skills[j].skillnum; j++) {
         if(p_skills[j].skillnum == i) {
            pal = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Pal(%d)", p_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; r_skills[j].skillnum; j++) {
         if(r_skills[j].skillnum == i) {
            ranger = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Ran(%d)", r_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      for(int j = 0; u_skills[j].skillnum; j++) {
         if(u_skills[j].skillnum == i) {
            druid = j;
            if(buf2[0] != '\0') strcat(buf2, ", ");
            sprintf(buf3, "Dru(%d)", u_skills[j].levelavailable);
            strcat(buf2, buf3);
            break;
         }
      }
      if(mage) {
         sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
               UPPER(*m_skills[mage].skillname),m_skills[mage].skillname+1,spell_info[mage].min_usesmana);
      } else if(cleric) {
         sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
               UPPER(*c_skills[cleric].skillname),c_skills[cleric].skillname+1,spell_info[cleric].min_usesmana);
      } else if(anti) {
         sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
               UPPER(*a_skills[anti].skillname),a_skills[anti].skillname+1,spell_info[anti].min_usesmana);
      } else if(pal) {
         sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
               UPPER(*p_skills[pal].skillname),p_skills[pal].skillname+1,spell_info[pal].min_usesmana);
      } else if(ranger) {
         sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
               UPPER(*r_skills[ranger].skillname),r_skills[ranger].skillname+1,spell_info[ranger].min_usesmana);
      } else if(druid) {
         sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
               UPPER(*u_skills[druid].skillname),u_skills[druid].skillname+1,spell_info[druid].min_usesmana);
      } else continue;

      strcat(buf, buf2);
      strcat(buf, "\n\r");
   }

   strcat(buf, "\n\r");
   page_string(ch->desc, buf, 1);

   return eSUCCESS;
}

int spl_lvl(int lev)
{
  if(lev >= MIN_GOD)
    return 0;
  return lev;
}
