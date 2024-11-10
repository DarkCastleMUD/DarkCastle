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
/* $Id: spells.cpp,v 1.292 2015/06/14 02:38:12 pirahna Exp $ */

#include <cstring>
#include <cstdio>
#include <cassert>
#include <cstdlib>

#include "DC/character.h"
#include "DC/race.h"
#include "DC/levels.h"
#include "DC/spells.h"
#include "DC/magic.h"
#include "DC/player.h"
#include "DC/isr.h"
#include "DC/utility.h"
#include "DC/fight.h"
#include "DC/mobile.h"
#include "DC/room.h"
#include "DC/db.h"
#include "DC/handler.h"
#include "DC/connect.h"
#include "DC/interp.h"
#include "DC/act.h"
#include "DC/returnvals.h"
#include "DC/ki.h"
#include "DC/sing.h"
#include "DC/clan.h"

// Global data

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
extern struct song_info_type song_info[];
extern char *spell_wear_off_msg[];

// Functions used in spells.C
int spl_lvl(int lev);

// Extern procedures
void make_dust(Character *ch);
int say_spell(Character *ch, int si, int room = 0);

#if (0)
uint8_t beats;               /* Waiting time after spell     */
position_t minimum_position; /* Position for caster          */
uint8_t min_usesmana;        /* Mana used                    */
int16_t targets;             /* Legal targets                */
SPELL_FUN *spell_pointer;    /* Function to call             */
int16_t difficulty;
#endif

struct spell_info_type spell_info[] =
    {
        {/* 00 */ /* Note: All arrays start at 0! CGT */ 0, position_t::DEAD, 0, 0, 0},

        {/* 01 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 8, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_armor, SKILL_INCREASE_MEDIUM},

        {/* 02 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 35, TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_teleport, SKILL_INCREASE_HARD},

        {/* 03 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 6, TAR_CHAR_ROOM | TAR_SELF_DEFAULT | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP, cast_bless, SKILL_INCREASE_MEDIUM},

        {/* 04 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 20, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_blindness, SKILL_INCREASE_HARD},

        {/* 05 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 15, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_burning_hands, SKILL_INCREASE_MEDIUM},

        {/* 06 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 15, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_iridescent_aura, SKILL_INCREASE_MEDIUM},

        {/* 07 */ /* 18, position_t::STANDING, 15, TAR_CHAR_ROOM|TAR_SELF_NONO, cast_charm_person */ 0, position_t::DEAD, 0, 0, 0, 0},

        {/* 08 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 20, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_chill_touch, SKILL_INCREASE_HARD},

        {/* 09 */ /* 3*DC::PULSE_TIMER, position_t::STANDING, 40, TAR_CHAR_ROOM, cast_clone); */ 0, position_t::DEAD, 0, 0, 0, 0},

        {/* 10 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 40, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_colour_spray, SKILL_INCREASE_HARD},

        {/* 11 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 25, TAR_IGNORE, cast_control_weather, SKILL_INCREASE_MEDIUM},

        {/* 12 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 5, TAR_IGNORE, cast_create_food, SKILL_INCREASE_MEDIUM},

        {/* 13 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 5, TAR_OBJ_INV | TAR_OBJ_EQUIP, cast_create_water, SKILL_INCREASE_MEDIUM},

        {/* 14 */ (uint8_t)(2.25 * DC::PULSE_TIMER), position_t::FIGHTING, 15, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_remove_blind, SKILL_INCREASE_MEDIUM},

        {/* 15 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 20, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_cure_critic, SKILL_INCREASE_MEDIUM},

        {/* 16 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 10, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_cure_light, SKILL_INCREASE_EASY},

        {/* 17 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 33, TAR_FIGHT_VICT | TAR_SELF_NONO | TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, cast_curse, SKILL_INCREASE_HARD},

        {/* 18 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 5, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_detect_evil, SKILL_INCREASE_EASY},

        {/* 19 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 8, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_detect_invisibility, SKILL_INCREASE_MEDIUM},

        {/* 20 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 6, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_detect_magic, SKILL_INCREASE_EASY},

        {/* 21 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 5, TAR_CHAR_ROOM | TAR_SELF_DEFAULT | TAR_OBJ_INV | TAR_OBJ_ROOM, cast_detect_poison, SKILL_INCREASE_EASY},

        {/* 22 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 20, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO | TAR_OBJ_ROOM, cast_dispel_evil, SKILL_INCREASE_MEDIUM},

        {/* 23 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 25, TAR_IGNORE, cast_earthquake, SKILL_INCREASE_HARD},

        {/* 24 */ 6 * DC::PULSE_TIMER, position_t::STANDING, 50, TAR_OBJ_INV | TAR_OBJ_EQUIP, cast_enchant_weapon, SKILL_INCREASE_MEDIUM},

        {/* 25 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 33, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_energy_drain, SKILL_INCREASE_HARD},

        {/* 26 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 25, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_fireball, SKILL_INCREASE_HARD},

        {/* 27 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 33, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_harm, SKILL_INCREASE_HARD},

        {/* 28 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 40, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_heal, SKILL_INCREASE_HARD},

        {/* 29 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 7, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP | TAR_SELF_DEFAULT, cast_invisibility, SKILL_INCREASE_MEDIUM},

        {/* 30 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 17, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_lightning_bolt, SKILL_INCREASE_HARD},

        {/* 31 */ 0, position_t::STANDING, 20, TAR_IGNORE, cast_locate_object, SKILL_INCREASE_HARD},

        {/* 32 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 10, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_magic_missile, SKILL_INCREASE_MEDIUM},

        {/* 33 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 15, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO | TAR_OBJ_INV | TAR_OBJ_ROOM, cast_poison, SKILL_INCREASE_HARD},

        {/* 34 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 50, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_protection_from_evil, SKILL_INCREASE_MEDIUM},

        {/* 35 */ (uint8_t)(2.25 * DC::PULSE_TIMER), position_t::FIGHTING, 18, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_OBJ_ROOM | TAR_SELF_DEFAULT, cast_remove_curse, SKILL_INCREASE_MEDIUM},

        {/* 36 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 60, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_sanctuary, SKILL_INCREASE_HARD},

        {/* 37 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 15, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_shocking_grasp, SKILL_INCREASE_MEDIUM},

        {/* 38 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 33, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_sleep, SKILL_INCREASE_HARD},

        {/* 39 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 20, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_strength, SKILL_INCREASE_MEDIUM},

        {/* 40 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 50, TAR_CHAR_WORLD | TAR_SELF_NONO, cast_summon, SKILL_INCREASE_HARD},

        {/* 41 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 5, TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_SELF_NONO, cast_ventriloquate, SKILL_INCREASE_EASY},

        {/* 42 */ (uint8_t)(2.25 * DC::PULSE_TIMER), position_t::FIGHTING, 40, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_word_of_recall, SKILL_INCREASE_MEDIUM},

        {/* 43 */ (uint8_t)(2.25 * DC::PULSE_TIMER), position_t::FIGHTING, 12, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_SELF_DEFAULT, cast_remove_poison, SKILL_INCREASE_MEDIUM},

        {/* 44 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 15, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_sense_life, SKILL_INCREASE_EASY},

        {/* 45 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 45, TAR_IGNORE, cast_summon_familiar, SKILL_INCREASE_MEDIUM},

        {/* 46 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 30, TAR_IGNORE, cast_lighted_path, SKILL_INCREASE_HARD},

        {/* 47 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 33, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_resist_acid, SKILL_INCREASE_HARD},

        {/* 48 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 35, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_sun_ray, SKILL_INCREASE_HARD},

        {/* 49 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 30, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_rapid_mend, SKILL_INCREASE_HARD},

        {/* 50 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 120, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_acid_shield, SKILL_INCREASE_HARD},

        {/* 51 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 22, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_water_breathing, SKILL_INCREASE_EASY},

        {/* 52 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 20, TAR_IGNORE, cast_globe_of_darkness, SKILL_INCREASE_HARD},

        {/* 53 */ 0, position_t::STANDING, 12, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, cast_identify, SKILL_INCREASE_EASY},

        {/* 54 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 75, TAR_OBJ_ROOM, cast_animate_dead, SKILL_INCREASE_MEDIUM},

        {/* 55 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 17, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_fear, SKILL_INCREASE_HARD},

        {/* 56 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 10, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_fly, SKILL_INCREASE_MEDIUM},

        {/* 57 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 7, TAR_NONE_OK | TAR_OBJ_INV | TAR_OBJ_ROOM, cast_cont_light, SKILL_INCREASE_EASY},

        {/* 58 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 5, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_know_alignment, SKILL_INCREASE_EASY},

        {/* 59 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 30, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_dispel_magic, SKILL_INCREASE_HARD},

        {/* 60 */ 6 * DC::PULSE_TIMER, position_t::STANDING, 150, TAR_IGNORE, cast_conjure_elemental, SKILL_INCREASE_MEDIUM},

        {/* 61 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 15, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_cure_serious, SKILL_INCREASE_EASY},

        {/* 62 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 12, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_cause_light, SKILL_INCREASE_EASY},

        {/* 63 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 24, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_cause_critical, SKILL_INCREASE_MEDIUM},

        {/* 64 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 18, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_cause_serious, SKILL_INCREASE_EASY},

        {/* 65 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 45, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_flamestrike, SKILL_INCREASE_HARD},

        {/* 66 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 33, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_stone_skin, SKILL_INCREASE_HARD},

        {/* 67 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 12, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_shield, SKILL_INCREASE_MEDIUM},

        {/* 68 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 20, TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_weaken, SKILL_INCREASE_HARD},

        {/* 69 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 33, TAR_IGNORE, cast_mass_invis, SKILL_INCREASE_MEDIUM},

        {/* 70 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 45, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_acid_blast, SKILL_INCREASE_HARD},

        {/* 71 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 55, TAR_CHAR_WORLD, cast_portal, SKILL_INCREASE_HARD},

        {/* 72 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 7, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_infravision, SKILL_INCREASE_EASY},

        {/* 73 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 12, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_refresh, SKILL_INCREASE_EASY},

        {/* 74 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 33, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_haste, SKILL_INCREASE_MEDIUM},

        {/* 75 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 20, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO | TAR_OBJ_ROOM, cast_dispel_good, SKILL_INCREASE_MEDIUM},

        {/* 76 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 80, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_hellstream, SKILL_INCREASE_HARD},

        {/* 77 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 60, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_power_heal, SKILL_INCREASE_HARD},

        {/* 78 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 80, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_full_heal, SKILL_INCREASE_HARD},

        {/* 79 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 55, TAR_IGNORE, cast_firestorm, SKILL_INCREASE_HARD},

        {/* 80 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 45, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_power_harm, SKILL_INCREASE_HARD},

        {/* 81 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 5, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_detect_good, SKILL_INCREASE_EASY},

        {/* 82 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 33, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_vampiric_touch, SKILL_INCREASE_HARD},

        {/* 83 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::FIGHTING, 40, TAR_IGNORE, cast_life_leech, SKILL_INCREASE_MEDIUM},

        {/* 84 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 33, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_paralyze, SKILL_INCREASE_HARD},

        {/* 85 */ (uint8_t)(2.25 * DC::PULSE_TIMER), position_t::FIGHTING, 18, TAR_CHAR_ROOM, cast_remove_paralysis, SKILL_INCREASE_MEDIUM},

        {/* 86 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 160, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_fireshield, SKILL_INCREASE_MEDIUM},

        {/* 87 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 40, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_meteor_swarm, SKILL_INCREASE_HARD},

        {/* 88 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 20, TAR_CHAR_WORLD, cast_wizard_eye, SKILL_INCREASE_HARD},

        {/* 89 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 33, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_true_sight, SKILL_INCREASE_HARD},

        {/* 90 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 0, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_mana, 0},

        {/* 91 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::FIGHTING, 200, TAR_IGNORE, cast_solar_gate, SKILL_INCREASE_MEDIUM},

        {/* 92 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 30, TAR_IGNORE, cast_heroes_feast, SKILL_INCREASE_EASY},

        {/* 93 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 100, TAR_IGNORE, cast_heal_spray, SKILL_INCREASE_MEDIUM},

        {/* 94 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 180, TAR_IGNORE, cast_group_sanc, SKILL_INCREASE_HARD},

        {/* 95 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 80, TAR_IGNORE, cast_group_recall, SKILL_INCREASE_MEDIUM},

        {/* 96 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 40, TAR_IGNORE, cast_group_fly, SKILL_INCREASE_MEDIUM},

        {/* 97 */ /* 24, position_t::STANDING, 250, TAR_OBJ_INV|TAR_OBJ_EQUIP, cast_enchant_armor */ 0, position_t::DEAD, 0, 0, 0, 0},

        {/* 98 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 33, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_resist_fire, SKILL_INCREASE_HARD},

        {/* 99 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 33, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_resist_cold, SKILL_INCREASE_HARD},

        {/* 100 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 8, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_bee_sting, SKILL_INCREASE_MEDIUM},

        {/* 101 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 25, TAR_IGNORE, cast_bee_swarm, SKILL_INCREASE_HARD},

        {/* 102 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 45, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_creeping_death, SKILL_INCREASE_HARD},

        {/* 103 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 20, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_barkskin, SKILL_INCREASE_HARD},

        {/* 104 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 45, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_herb_lore, SKILL_INCREASE_HARD},

        {/* 105 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 75, TAR_IGNORE, cast_call_follower, SKILL_INCREASE_MEDIUM},

        {/* 106 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 15, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_entangle, SKILL_INCREASE_HARD},

        {/* 107 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 5, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_eyes_of_the_owl, SKILL_INCREASE_EASY},

        {/* 108 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 20, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_feline_agility, SKILL_INCREASE_MEDIUM},

        {/* 109 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 30, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_forest_meld, SKILL_INCREASE_HARD},

        {/* 110 */ /* 3*DC::PULSE_TIMER, position_t::STANDING, 150, TAR_IGNORE, cast_companion */ 0, position_t::DEAD, 0, 0, 0, 0},

        {/* 111 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 33, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_drown, SKILL_INCREASE_HARD},

        {/* 112 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 25, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_howl, SKILL_INCREASE_HARD},

        {/* 113 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 33, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_souldrain, SKILL_INCREASE_MEDIUM},

        {/* 114 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 18, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_sparks, 0},

        {/* 115 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 20, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_camouflague, SKILL_INCREASE_HARD},

        {/* 116 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 24, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_farsight, SKILL_INCREASE_HARD},

        {/* 117 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 20, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_freefloat, SKILL_INCREASE_HARD},

        {/* 118 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 33, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_insomnia, SKILL_INCREASE_HARD},

        {/* 119 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 50, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_shadowslip, SKILL_INCREASE_HARD},

        {/* 120 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 33, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_resist_energy, SKILL_INCREASE_HARD},

        {/* 121 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 20, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_staunchblood, SKILL_INCREASE_HARD},

        {/* 122 */ 6 * DC::PULSE_TIMER, position_t::STANDING, 250, TAR_IGNORE, cast_create_golem, SKILL_INCREASE_EASY},

        {/* 123 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 60, TAR_IGNORE, spell_reflect, SKILL_INCREASE_HARD},

        {/* 124 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 22, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_SELF_NONO, cast_dispel_minor, SKILL_INCREASE_MEDIUM},

        {/* 125 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 25, TAR_IGNORE, spell_release_golem, SKILL_INCREASE_MEDIUM},

        {/* 126 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 30, TAR_IGNORE, spell_beacon, SKILL_INCREASE_MEDIUM},

        {/* 127 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 40, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_stone_shield, SKILL_INCREASE_HARD},

        {/* 128 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 55, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_greater_stone_shield, SKILL_INCREASE_HARD},

        {/* 129 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 15, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_iron_roots, SKILL_INCREASE_HARD},

        {/* 130 */ /* 3*DC::PULSE_TIMER, position_t::STANDING, 50, TAR_CHAR_ROOM|TAR_SELF_ONLY|TAR_SELF_DEFAULT, cast_eyes_of_the_eagle */ 0, position_t::DEAD, 0, 0, 0, 0},

        {/* 131 */ /* 3*DC::PULSE_TIMER, position_t::STANDING,  0, TAR_CHAR_ROOM, nullptr */ 0, position_t::DEAD, 0, 0, 0, 0},

        {/* 132 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 90, TAR_IGNORE, cast_icestorm, SKILL_INCREASE_HARD},

        {/* 133 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 65, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_lightning_shield, SKILL_INCREASE_HARD},

        {/* 134 */ (uint8_t)(2.25 * DC::PULSE_TIMER), position_t::FIGHTING, 10, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_blue_bird, SKILL_INCREASE_EASY},

        {/* 135 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 15, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_debility, SKILL_INCREASE_MEDIUM},

        {/* 136 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 30, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_attrition, SKILL_INCREASE_MEDIUM},

        {/* 137 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::FIGHTING, 120, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_vampiric_aura, SKILL_INCREASE_EASY},

        {/* 138 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::FIGHTING, 200, TAR_IGNORE, cast_holy_aura, SKILL_INCREASE_EASY},

        {/* 139 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 5, TAR_IGNORE, cast_dismiss_familiar, SKILL_INCREASE_MEDIUM},

        {/* 140 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 15, TAR_IGNORE, cast_dismiss_corpse, SKILL_INCREASE_MEDIUM},

        {/* 141 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 30, TAR_IGNORE, cast_blessed_halo, SKILL_INCREASE_MEDIUM},

        {/* 142 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 40, TAR_IGNORE, cast_visage_of_hate, SKILL_INCREASE_MEDIUM},

        {/* 143 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 50, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_protection_from_good, SKILL_INCREASE_MEDIUM},

        {/* 144 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 12, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_oaken_fortitude, SKILL_INCREASE_MEDIUM},

        {/* 145 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 24, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_frostshield, SKILL_INCREASE_MEDIUM},

        {/* 146 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 24, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_stability, SKILL_INCREASE_MEDIUM},

        {/* 147 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 24, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, nullptr, SKILL_INCREASE_MEDIUM},

        {/* 148*/ 3 * DC::PULSE_TIMER, position_t::STANDING, 24, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, nullptr, SKILL_INCREASE_MEDIUM},

        {/* 149*/ 3 * DC::PULSE_TIMER, position_t::STANDING, 24, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_solidity, SKILL_INCREASE_MEDIUM},

        {/* 150*/ 3 * DC::PULSE_TIMER, position_t::STANDING, 24, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, nullptr, SKILL_INCREASE_MEDIUM},

        {/* 151*/ 3 * DC::PULSE_TIMER, position_t::STANDING, 24, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, nullptr, SKILL_INCREASE_MEDIUM},

        {/* 152*/ 3 * DC::PULSE_TIMER, position_t::STANDING, 24, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, nullptr, SKILL_INCREASE_MEDIUM},

        {/* 153*/ 3 * DC::PULSE_TIMER, position_t::STANDING, 50, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_aegis, SKILL_INCREASE_HARD},

        {/* 154*/ 3 * DC::PULSE_TIMER, position_t::STANDING, 50, TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_SELF_DEFAULT, cast_aegis, SKILL_INCREASE_HARD},

        {/* 155 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 33, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_resist_magic, SKILL_INCREASE_HARD},

        {/* 156 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 30, TAR_CHAR_WORLD, cast_eagle_eye, SKILL_INCREASE_HARD},

        {/* 157 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 35, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_call_lightning, SKILL_INCREASE_HARD},

        {/* 158 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 45, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, cast_divine_fury, SKILL_INCREASE_HARD},

        {/* 159 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 45, TAR_IGNORE, cast_ghost_walk, SKILL_INCREASE_HARD},

        {/* 160 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 120, TAR_IGNORE, cast_mend_golem, SKILL_INCREASE_MEDIUM},

        {/* 161 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 10, TAR_CHAR_ROOM | TAR_SELF_DEFAULT, cast_clarity, SKILL_INCREASE_MEDIUM},

        {/* 162 */ 3 * DC::PULSE_TIMER, position_t::STUNNED, 200, TAR_CHAR_ROOM | TAR_SELF_DEFAULT | TAR_SELF_ONLY, cast_divine_intervention, SKILL_INCREASE_EASY},

        {/* 163 */ 4 * DC::PULSE_TIMER, position_t::FIGHTING, 75, TAR_IGNORE, cast_wrath_of_god, SKILL_INCREASE_HARD},

        {/* 164 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 100, TAR_CHAR_ROOM | TAR_SELF_DEFAULT | TAR_SELF_ONLY, cast_atonement, SKILL_INCREASE_EASY},

        {/* 165 */ 4 * DC::PULSE_TIMER, position_t::FIGHTING, 150, TAR_IGNORE, cast_silence, SKILL_INCREASE_HARD},

        {/* 166 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 180, TAR_CHAR_ROOM | TAR_SELF_DEFAULT | TAR_SELF_ONLY, cast_immunity, SKILL_INCREASE_HARD},

        {/* 167 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 150, TAR_CHAR_ROOM, cast_boneshield, SKILL_INCREASE_HARD},

        {/* 168 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 100, TAR_CHAR_ROOM | TAR_SELF_NONO, cast_channel, SKILL_INCREASE_HARD},

        {/* 169 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 15, TAR_IGNORE, cast_release_elemental, SKILL_INCREASE_MEDIUM},

        {/* 170 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 50, TAR_CHAR_ROOM | TAR_FIGHT_VICT, cast_wild_magic, SKILL_INCREASE_HARD},

        {/* 171 */ 3 * DC::PULSE_TIMER, position_t::FIGHTING, 125, TAR_IGNORE, cast_spirit_shield, SKILL_INCREASE_HARD},

        {/* 172 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 80, TAR_CHAR_ROOM | TAR_SELF_DEFAULT | TAR_SELF_ONLY, cast_villainy, SKILL_INCREASE_HARD},

        {/* 173 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::STANDING, 80, TAR_CHAR_ROOM | TAR_SELF_DEFAULT | TAR_SELF_ONLY, cast_heroism, SKILL_INCREASE_HARD},

        {/* 174 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::FIGHTING, 100, TAR_IGNORE, cast_consecrate, SKILL_INCREASE_HARD},

        {/* 175 */ (uint8_t)(4.5 * DC::PULSE_TIMER), position_t::FIGHTING, 100, TAR_IGNORE, cast_desecrate, SKILL_INCREASE_HARD},
        {/* 176 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 100, TAR_ROOM_EXIT, cast_elemental_wall, SKILL_INCREASE_MEDIUM},
        {/* 177 */ 3 * DC::PULSE_TIMER, position_t::STANDING, 100, TAR_IGNORE, cast_ethereal_focus, SKILL_INCREASE_EASY}};

struct skill_stuff skill_info[] =
    {
        /*  1 */ {"trip", SKILL_INCREASE_MEDIUM},
        /*  2 */ {"dodge", SKILL_INCREASE_HARD},
        /*  3 */ {"double", SKILL_INCREASE_HARD},
        /*  4 */ {"disarm", SKILL_INCREASE_MEDIUM},
        /*  5 */ {"triple", SKILL_INCREASE_HARD},
        /*  6 */ {"parry", SKILL_INCREASE_HARD},
        /*  7 */ {"deathstroke", SKILL_INCREASE_EASY},
        /*  8 */ {"circle", SKILL_INCREASE_MEDIUM},
        /*  9 */ {"berserk", SKILL_INCREASE_HARD},
        /* 10 */ {"headbutt", SKILL_INCREASE_HARD},
        /* 11 */ {"eagle claw", SKILL_INCREASE_MEDIUM},
        /* 12 */ {"quivering palm", SKILL_INCREASE_EASY},
        /* 13 */ {"palm", SKILL_INCREASE_HARD},
        /* 14 */ {"stalk", SKILL_INCREASE_HARD},
        /* 15 */ {"UNUSED", 0},
        /* 16 */ {"dual_backstab", SKILL_INCREASE_HARD},
        /* 17 */ {"hitall", SKILL_INCREASE_HARD},
        /* 18 */ {"stun", SKILL_INCREASE_HARD},
        /* 19 */ {"scan", SKILL_INCREASE_EASY},
        /* 20 */ {"consider", SKILL_INCREASE_EASY},
        /* 21 */ {"switch", SKILL_INCREASE_EASY},
        /* 22 */ {"redirect", SKILL_INCREASE_MEDIUM},
        /* 23 */ {"ambush", SKILL_INCREASE_MEDIUM},
        /* 24 */ {"forage", SKILL_INCREASE_HARD},
        /* 25 */ {"tame", SKILL_INCREASE_MEDIUM},
        /* 26 */ {"track", SKILL_INCREASE_HARD},
        /* 27 */ {"skewer", SKILL_INCREASE_HARD},
        /* 28 */ {"slip", SKILL_INCREASE_MEDIUM},
        /* 29 */ {"retreat", SKILL_INCREASE_HARD},
        /* 30 */ {"rage", SKILL_INCREASE_MEDIUM},
        /* 31 */ {"battlecry", SKILL_INCREASE_EASY},
        /* 32 */ {"archery", SKILL_INCREASE_HARD},
        /* 33 */ {"riposte", SKILL_INCREASE_HARD},
        /* 34 */ {"lay hands", SKILL_INCREASE_EASY},
        /* 35 */ {"insane chant", 0},
        /* 36 */ {"glitter dust", 0},
        /* 37 */ {"sneak", SKILL_INCREASE_HARD},
        /* 38 */ {"hide", SKILL_INCREASE_HARD},
        /* 39 */ {"steal", SKILL_INCREASE_MEDIUM},
        /* 40 */ {"backstab", SKILL_INCREASE_MEDIUM},
        /* 41 */ {"pick_lock", SKILL_INCREASE_EASY},
        /* 42 */ {"kick", SKILL_INCREASE_MEDIUM},
        /* 43 */ {"bash", SKILL_INCREASE_HARD},
        /* 44 */ {"rescue", SKILL_INCREASE_MEDIUM},
        /* 45 */ {"blood_fury", SKILL_INCREASE_EASY},
        /* 46 */ {"dual_wield", SKILL_INCREASE_EASY},
        /* 47 */ {"harm_touch", SKILL_INCREASE_EASY},
        /* 48 */ {"shield_block", SKILL_INCREASE_HARD},
        /* 49 */ {"blade_shield", SKILL_INCREASE_EASY},
        /* 50 */ {"pocket", SKILL_INCREASE_MEDIUM},
        /* 51 */ {"guard", SKILL_INCREASE_MEDIUM},
        /* 52 */ {"frenzy", SKILL_INCREASE_HARD},
        /* 53 */ {"blindfighting", SKILL_INCREASE_HARD},
        /* 54 */ {"focused_repelance", SKILL_INCREASE_EASY},
        /* 55 */ {"vital_strike", SKILL_INCREASE_EASY},
        /* 56 */ {"crazed_assault", SKILL_INCREASE_HARD},
        /* 57 */ {"divine_protection", 0},
        /* 58 */ {"bludgeoning_weapons", SKILL_INCREASE_MEDIUM},
        /* 59 */ {"piercing_weapons", SKILL_INCREASE_MEDIUM},
        /* 60 */ {"slashing_weapons", SKILL_INCREASE_MEDIUM},
        /* 61 */ {"whipping_weapons", SKILL_INCREASE_MEDIUM},
        /* 62 */ {"crushing_weapons", SKILL_INCREASE_MEDIUM},
        /* 63 */ {"two_handed_weapons", SKILL_INCREASE_MEDIUM},
        /* 64 */ {"hand_to_hand", SKILL_INCREASE_MEDIUM},
        /* 65 */ {"bullrush", SKILL_INCREASE_HARD},
        /* 66 */ {"ferocity", SKILL_INCREASE_MEDIUM},
        /* 67 */ {"tactics", SKILL_INCREASE_MEDIUM},
        /* 68 */ {"deceit", SKILL_INCREASE_MEDIUM},
        /* 69 */ {"release", SKILL_INCREASE_EASY},
        /* 70 */ {"fear gaze", 0},
        /* 71 */ {"eyegouge", SKILL_INCREASE_HARD},
        /* 72 */ {"magic resist", SKILL_INCREASE_HARD},
        /* 73 */ {"new save format", 0},
        /* 74 */ {"spellcraft", SKILL_INCREASE_HARD},
        /* 75 */ {"martial defense", SKILL_INCREASE_HARD},
        /* 76 */ {"knockback", SKILL_INCREASE_HARD},
        /* 77 */ {"stinging_weapons", SKILL_INCREASE_MEDIUM},
        /* 78 */ {"jab", SKILL_INCREASE_EASY},
        /* 79 */ {"appraise", SKILL_INCREASE_MEDIUM},
        /* 80 */ {"natures lore", SKILL_INCREASE_MEDIUM},
        /* 81 */ {"fire arrows", SKILL_INCREASE_HARD},
        /* 82 */ {"ice arrows", SKILL_INCREASE_MEDIUM},
        /* 83 */ {"tempest arrows", SKILL_INCREASE_EASY},
        /* 84 */ {"granite arrows", SKILL_INCREASE_HARD},
        /* 85 */ {"do not use", SKILL_INCREASE_HARD},
        /* 86 */ {"meta reimb", 0},
        /* 87 */ {"combat mastery", SKILL_INCREASE_HARD},
        /* 88 */ {"rapid join", SKILL_INCREASE_EASY},
        /* 89 */ {"enhanced regeneration", SKILL_INCREASE_HARD},
        /* 90 */ {"cripple", SKILL_INCREASE_HARD},
        /* 91 */ {"natural selection", SKILL_INCREASE_HARD},
        /* 92 */ {"ignoreclan", 0},
        /* 93 */ {"ignoreclan2", 0},
        /* 94 */ {"commune", SKILL_INCREASE_HARD},
        /* 95 */ {"scribe", SKILL_INCREASE_EASY},
        /* 96 */ {"make camp", SKILL_INCREASE_HARD},
        /* 97 */ {"battlesense", SKILL_INCREASE_HARD},
        /* 98 */ {"perseverance", SKILL_INCREASE_HARD},
        /* 99 */ {"triage", SKILL_INCREASE_HARD},
        /* 100*/ {"smite", SKILL_INCREASE_HARD},
        /* 101*/ {"leadership", SKILL_INCREASE_HARD},
        /* 102*/ {"execute", SKILL_INCREASE_HARD},
        /* 103*/ {"defenders stance", SKILL_INCREASE_HARD},
        /* 104*/ {"behead", SKILL_INCREASE_HARD},
        /* 105 */ {"primal fury", SKILL_INCREASE_EASY},
        /* 106 */ {"vigor", SKILL_INCREASE_MEDIUM},
        /* 107 */ {"escape", SKILL_INCREASE_HARD},
        /* 108 */ {"critical hit", SKILL_INCREASE_HARD},
        /* 109 */ {"batterbrace", SKILL_INCREASE_MEDIUM},
        /* 110 */ {"free_animal", SKILL_INCREASE_EASY},
        /* 111 */ {"offhand double", SKILL_INCREASE_HARD},
        /* 112 */ {"onslaught", SKILL_INCREASE_HARD},
        /* 113 */ {"counter strike", SKILL_INCREASE_HARD},
        /* 114 */ {"imbue", SKILL_INCREASE_HARD},
        /* 115 */ {"elemental filter", SKILL_INCREASE_HARD},
        /* 116 */ {"orchestrate", SKILL_INCREASE_HARD},
        /* 117 */ {"tumbling", SKILL_INCREASE_HARD},
        /* 118 */ {"brew", SKILL_INCREASE_EASY},
        /*    */ {"\n", 0},
};

const char *skills[] =
    {
        "trip", // 0
        "dodge",
        "second_attack",
        "disarm",
        "third_attack",
        "parry",
        "deathstroke",
        "circle",
        "berserk",
        "headbutt",
        "eagle claw", // 10
        "quivering palm",
        "palm",
        "stalk",
        "UNUSED",
        "dual_backstab",
        "hitall",
        "stun",
        "scan",
        "consider",
        "switch", // a20
        "redirect",
        "ambush",
        "forage",
        "tame",
        "track",
        "skewer",
        "slip",
        "retreat",
        "rage",
        "battlecry", // 30
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
        "guard", // 50
        "frenzy",
        "blindfighting",
        "focused_repelance",
        "vital_strike",
        "crazed_assault",
        "divine_protection",
        "bludgeoning_weapons",
        "piercing_weapons",
        "slashing_weapons",
        "whipping_weapons", // 60
        "crushing_weapons",
        "two_handed_weapons",
        "hand_to_hand",
        "bullrush",
        "ferocity",
        "tactics",
        "deceit",
        "release",
        "fear_gaze",
        "eyegouge", // 70
        "magic resist",
        "new save format",
        "spellcraft",
        "martial_defense",
        "knockback",
        "stinging_weapons",
        "jab",
        "appraise",
        "natures lore",
        "fire arrows", // 80
        "ice arrows",
        "tempest arrows",
        "granite arrows",
        "do not use",
        "meta reimb",
        "combat mastery",
        "rapid join",
        "enhanced regeneration",
        "cripple",
        "natural selection", // 90
        "ignoreclan",
        "ignoreclan2",
        "commune",
        "scribe scroll",
        "make camp",
        "battlesense",
        "perseverance",
        "triage",
        "smite",
        "leadership", // 100
        "execute",
        "defenders stance",
        "behead",
        "primal fury",
        "vigor",
        "escape",
        "critical hit",
        "batterbrace",
        "free animal",
        "offhand double", // 110
        "onslaught",
        "counter strike",
        "imbue",
        "elemental filter",
        "orchestrate",
        "tumbling",
        "brew",
        "pursuit",
        "profession",
        "legionnaire", // 120
        "gladiator",
        "battlerager",
        "chieftan",
        "pilferer",
        "assassin",
        "warmage",
        "spellbinder",
        "zealot",
        "ritualist",
        "elementalist", // 130
        "shapeshifter",
        "cultist",
        "reaver",
        "templar",
        "inquisitor",
        "scout",
        "tracker",
        "sensei",
        "spiritualist",
        "troubadour", // 140
        "ministrel",
        "\n"};

const char *spells[] =
    {
        "armor", /* 1 */
        "teleport",
        "bless",
        "blindness",
        "burning hands",
        "iridescent aura",
        "charm person",
        "chill touch",
        "clone",
        "colour spray",
        "control weather", /* 11 */
        "create food",
        "create water",
        "remove blind",
        "cure critical",
        "cure light",
        "curse",
        "detect evil",
        "detect invisibility",
        "detect magic",
        "detect poison", /* 21 */
        "dispel evil",
        "earthquake",
        "enchant weapon",
        "energy drain",
        "fireball",
        "harm",
        "heal",
        "invisibility",
        "lightning bolt",
        "locate object", /* 31 */
        "magic missile",
        "poison",
        "protection from evil",
        "remove curse",
        "sanctuary",
        "shocking grasp",
        "sleep",
        "strength",
        "summon",
        "ventriloquate", /* 41 */
        "word of recall",
        "remove poison",
        "sense life",    /* 44 */
        "call familiar", /* 45 */
        "lighted path",
        "resist acid",
        "sun ray",
        "rapid mend",
        "acid shield", /* 50 */
        "water breathing",
        "globe of darkness",
        "identify",
        "animate dead",
        "fear",
        "fly",
        "continual light",
        "know alignment",
        "dispel magic",
        "conjure elemental", /* 60 */
        "cure serious",
        "cause light",
        "cause critical",
        "cause serious",
        "flamestrike", /* 65 */
        "stoneskin",
        "shield",
        "weaken",
        "mass invisibility",
        "acid blast", /* 70 */
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
        "bee sting", // 100
        "bee swarm",
        "creeping death",
        "barkskin",
        "herb lore",
        "call follower",
        "entangle",
        "eyes of the owl",
        "feline agility",
        "forest meld",
        "companion", // 110
        "drown",
        "howl",
        "souldrain",
        "sparks", // 114
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
        "icestorm",
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
        "villainy",
        "heroism",
        "consecrate",
        "desecrate",
        "elemental wall",
        "ethereal focus",
        "\n"};

bool Character::canPerform(const int_fast32_t &skillType, QString failMessage)
{
  if (isPlayer() && !has_skill(skillType) && getLevel() < ARCHANGEL)
  {
    send(failMessage);
    return false;
  }

  return true;
}

// Figures out how many % of max your damage does
int dam_percent(int learned, int damage)
{
  float percent;
  percent = 50;
  if (!learned)
    percent /= 2;
  percent += learned / 2;
  //  else percent = 90 + ((learned - 90) *2);

  return (int)((float)damage * (float)percent / 100.0);
}

int use_mana(Character *ch, int sn)
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

      divisor = 2 + ch->getLevel();
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

void affect_update(int32_t duration_type)
{
  static struct affected_type *af, *next_af_dude;
  void update_char_objects(Character * ch); /* handler.c */

  if (duration_type != DC::PULSE_REGEN && duration_type != DC::PULSE_TIMER && duration_type != DC::PULSE_VIOLENCE && duration_type != DC::PULSE_TIME) // Default
    return;

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &i : character_list)
  {
    // This doesn't really belong here, but it beats creating an "update" just for it.
    // That way we don't have to traverse the entire list all over again
    if (duration_type == DC::PULSE_TIME && IS_PC(i))
      update_char_objects(i);

    for (af = i->affected; af; af = next_af_dude)
    {
      next_af_dude = af->next;
      if (af->duration_type == 0 && duration_type == DC::PULSE_TIME)
      {
        // Business as usual
      }
      else if (af->duration_type != duration_type)
      {
        continue;
      }

      if ((af->type == Character::PLAYER_OBJECT_THIEF || af->type == Character::PLAYER_CANTQUIT || af->type == Character::PLAYER_GOLD_THIEF) && !i->desc)
        continue;
      if (af->duration > 1)
      {
        af->duration--;
        if (af->type == SPELL_ICESTORM)
          af->modifier = 0 - af->duration;
        if (!(af->caster).empty()) // means bard song
        {
          Character *get_pc_room_vis_exact(Character * ch, const char *name);
          Character *bard = get_pc_room_vis_exact(i, (af->caster).c_str());
          if (!bard || !ARE_GROUPED(i, bard))
          {
            i->sendln("Away from the music, the effect weakens...");
            af->duration = 1;
            (af->caster).clear();
          }
        }
      }
      else if (af->duration == 1)
      {
        // warnings for certain spells
        switch (af->type)
        {
        case SKILL_SONG_SUBMARINERS_ANTHEM:
          i->sendln("Your musical ability to breath water weakens.");
          break;
        case SPELL_WATER_BREATHING:
          i->sendln("You feel the magical hold of your gills about to give way.");
          break;
        default:
          break;
        }
        af->duration--;
        if (af->type == SPELL_ICESTORM)
          af->modifier = 0 - af->duration;
      }
      else if (af->duration != -1)
      {
        if ((af->type > 0) && (af->type <= MAX_SPL_LIST)) // only spells for this part
          if (*spell_wear_off_msg[af->type])
          {
            send_to_char(spell_wear_off_msg[af->type], i);
            i->sendln("");
          }
        if (af->type == SPELL_ETHEREAL_FOCUS)
        {
          // NOTICE:  this is a TEMP room flag
          REMOVE_BIT(DC::getInstance()->world[i->in_room].temp_room_flags, ROOM_ETHEREAL_FOCUS);
          act("$n shakes his $s head suddenly in confusion losing $s magical focus.", i, nullptr, nullptr, TO_ROOM, NOTVICT);
        }
        affect_remove(i, af, 0);
      }
    }
    continue;
  }
  DC::getInstance()->removeDead();
}

// Sets any ISR's that go with a spell..  (ISR's arent saved)
void isr_set(Character *ch)
{
  // char buf[100];
  static struct affected_type *afisr;

  if (!ch)
  {
    logentry(QStringLiteral("nullptr ch in isr_set!"), 0, LogChannels::LOG_BUG);
    return;
  }

  /*  why do we need this spamming the logs?
     sprintf(buf, "isr_set ch %s", GET_NAME(ch));
     logentry(buf, 0, LogChannels::LOG_BUG);
  */
  for (afisr = ch->affected; afisr; afisr = afisr->next)
  {
    if (afisr->type == SPELL_STONE_SKIN)
      SET_BIT(ch->resist, ISR_PIERCE);
    else if (afisr->type == SPELL_BARKSKIN)
      SET_BIT(ch->resist, ISR_SLASH);
  }
}

bool many_charms(Character *ch)
{
  struct follow_type *k;
  for (k = ch->followers; k; k = k->next)
  {
    if (IS_AFFECTED(k->follower, AFF_CHARM))
      return true;
  }

  return false;
}
/* Stop the familiar without a master floods*/
void extractFamiliar(Character *ch)
{
  Character *victim = nullptr;
  for (struct follow_type *k = ch->followers; k; k = k->next)
    if (IS_MOB(k->follower) && IS_AFFECTED(k->follower, AFF_FAMILIAR))
    {
      victim = k->follower;
      break;
    }

  if (nullptr == victim)
    return;

  act("$n disappears in a flash of flame and shadow.", victim, 0, 0, TO_ROOM, 0);
  extract_char(victim, true);
}

bool any_charms(Character *ch)
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
      return true;
    else
      return false;
  */
}

// check if making ch follow victim will create an illegal
// follow "Loop/circle"
bool circle_follow(Character *ch, Character *victim)
{
  Character *k;

  for (k = victim; k; k = k->master)
  {
    if (k == ch)
      return (true);
  }

  return (false);
}

// Called when stop following persons, or stopping charm
// This will NOT do if a character quits/dies!!
void stop_follower(Character *ch, int cmd)
{
  struct follow_type *j, *k;

  if (ch->master == nullptr)
  {
    logentry(QStringLiteral("Stop_follower: null ch_master!"), ARCHANGEL, LogChannels::LOG_BUG);
    return;
  }
  /*
    if(ISSET(ch->affected_by, AFF_FAMILIAR)) {
      do_emote(ch, "screams in pain as its connection with its master is broken.", CMD_DEFAULT);
      extract_char(ch, true);
      return;
    }
  */
  //  if(IS_AFFECTED(ch, AFF_CHARM)) {
  if (cmd == BROKE_CHARM || cmd == BROKE_CHARM_LILITH)
  {

    if (GET_CLASS(ch->master) != CLASS_RANGER || cmd == BROKE_CHARM_LILITH)
    {
      act("You realize that $N is a jerk!", ch, 0, ch->master, TO_CHAR, 0);
      act("$n is free from the bondage of the spell.", ch, 0, 0, TO_ROOM, 0);
      act("$n hates your guts!", ch, 0, ch->master, TO_VICT, 0);
    }
    else
    {
      act("You lose interest in $N.", ch, 0, ch->master, TO_CHAR, 0);
      act("$n loses interest in $N.", ch, 0, ch->master, TO_ROOM, NOTVICT);
      act("$n loses interest in you, and goes back to its business.", ch, 0, ch->master, TO_VICT, 0);
    }
    if (ch->fighting && ch->fighting != ch->master)
    {
      do_say(ch, "Screw this, I'm going home!", 0);
      stop_fighting(ch->fighting);
      stop_fighting(ch);
    }
  }
  else
  {
    if (cmd == END_STALK)
    {
      act("You sneakily stop following $N.",
          ch, 0, ch->master, TO_CHAR, 0);
    }
    else
    {
      // multiple checks if ch->master is still valid are necessary because a mob program may mak it no longer valid
      if (ch->master != nullptr)
      {
        act("You stop following $N.", ch, 0, ch->master, TO_CHAR, 0);

        if (ch->master != nullptr)
        {
          act("$n stops following $N.", ch, 0, ch->master, TO_ROOM, NOTVICT);
        }

        if (ch->master != nullptr)
        {
          act("$n stops following you.", ch, 0, ch->master, TO_VICT, 0);
        }
      }
    }
  }

  if (ch != nullptr && ch->master != nullptr && ch->master->followers != nullptr && ch->master->followers->follower == ch)
  { /* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    dc_free(k);
  }
  else
  { /* locate follower who is not head of list */
    if (ch->master != nullptr)
    {
      for (k = ch->master->followers; k->next->follower != ch; k = k->next)
        ;

      j = k->next;
      k->next = j->next;
      dc_free(j);
    }
  }

  ch->master = 0;

  /* do this after setting master to nullptr, to prevent endless loop */
  /* between affect_remove() and stop_follower()                   */
  if (cmd != CHANGE_LEADER)
  {
    if (ch->affected_by_spell(SPELL_CHARM_PERSON))
      affect_from_char(ch, SPELL_CHARM_PERSON);
    REMBIT(ch->affected_by, AFF_CHARM);
    REMBIT(ch->affected_by, AFF_GROUP);
  }
}

/* Called when a character that follows/is followed dies */
void die_follower(Character *ch)
{
  struct follow_type *j, *k;
  Character *zombie;

  if (ch->master)
    stop_follower(ch, STOP_FOLLOW);

  for (k = ch->followers; k; k = j)
  {
    j = k->next;
    zombie = k->follower;
    if (!ISSET(zombie->affected_by, AFF_GOLEM))
    {
      if (zombie->affected_by_spell(SPELL_CHARM_PERSON))
        affect_from_char(zombie, SPELL_CHARM_PERSON);
      stop_follower(zombie, STOP_FOLLOW);
    }
    if (GET_RACE(zombie) == RACE_UNDEAD)
    {
      send_to_char("The forces holding you together are gone.  You cease "
                   "to exist.",
                   zombie);
      act("$n dissolves into a puddle of rotten ooze.", zombie, 0, 0,
          TO_ROOM, 0);

      make_dust(zombie);
      extract_char(zombie, true);
    }
  }
}

/* Do NOT call ths before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(Character *ch, Character *leader, int cmd)
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

  if (cmd == 1)
    act("You stalk $N.", ch, 0, leader, TO_CHAR, 0);

  else if (cmd == 2)
    return;

  else
  {
    act("You now follow $N.", ch, 0, leader, TO_CHAR, 0);
    act("$n starts following you.", ch, 0, leader, TO_VICT, INVIS_NULL);
    act("$n now follows $N.", ch, 0, leader, TO_ROOM, INVIS_NULL | NOTVICT);
  }
}

int say_spell(Character *ch, int si, int room)
{
  char buf[MAX_STRING_LENGTH], splwd[MAX_BUF_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  int j, offs, retval = 0;
  Character *temp_char;

  struct syllable
  {
    char org[10];
    char new_new[10];
  };

  struct syllable syls[] = {
      {" ", " "},
      {"ar", "andoa"},
      {"au", "hana"},
      {"bless", "amen"},
      {"blind", "ubra"},
      {"bur", "misa"},
      {"cu", "unmani"},
      {"de", "oculo"},
      {"en", "unso"},
      {"light", "sol"},
      {"lo", "hi"},
      {"mor", "zak"},
      {"move", "syfo"},
      {"ness", "licra"},
      {"ning", "illa"},
      {"per", "duca"},
      {"ra", "bru"},
      {"re", "xandu"},
      {"son", "sabra"},
      {"tect", "occa"},
      {"tri", "cula"},
      {"ven", "nofo"},
      {"a", "a"},
      {"b", "b"},
      {"c", "q"},
      {"d", "e"},
      {"e", "z"},
      {"f", "y"},
      {"g", "o"},
      {"h", "p"},
      {"i", "u"},
      {"j", "y"},
      {"k", "t"},
      {"l", "r"},
      {"m", "w"},
      {"n", "i"},
      {"o", "a"},
      {"p", "s"},
      {"q", "d"},
      {"r", "f"},
      {"s", "g"},
      {"t", "h"},
      {"u", "j"},
      {"v", "z"},
      {"w", "x"},
      {"x", "n"},
      {"y", "l"},
      {"z", "k"},
      {"", ""}};

  strcpy(buf, "");
  strcpy(splwd, spells[si - 1]);

  offs = 0;

  while (*(splwd + offs))
  {
    for (j = 0; *(syls[j].org); j++)
      if (strncmp(syls[j].org, splwd + offs, strlen(syls[j].org)) == 0)
      {
        strcat(buf, syls[j].new_new);
        if (strlen(syls[j].org))
          offs += strlen(syls[j].org);
        else
          ++offs;
      }
  }

  sprintf(buf2, "$n utters the words, '%s'", buf);
  sprintf(buf, "$n utters the words, '%s'", spells[si - 1]);

  Character *people;
  if (room > 0)
  {
    people = DC::getInstance()->world[room].people;
  }
  else
  {
    people = DC::getInstance()->world[ch->in_room].people;
  }

  for (temp_char = people;
       temp_char;
       temp_char = temp_char->next_in_room)
    if (temp_char != ch)
    {
      if (GET_CLASS(ch) == GET_CLASS(temp_char))
      {
        act_return ar = act(buf, ch, 0, temp_char, TO_VICT, 0);
        retval = ar.retval;
      }
      else
      {
        act_return ar = act(buf2, ch, 0, temp_char, TO_VICT, 0);
        retval = ar.retval;
      }

      // Need better solution, but this will keep DC from crashing when a act_prog trigger kills a mob in the room
      if (SOMEONE_DIED(retval))
      {
        return retval;
      }
    }
  return eSUCCESS;
}

// Takes the spell_base (higher = harder to resist)
// returns 0 or positive if saving throw is made. The more, the higher it was made.
// return -number of failure.   The lower, the more it was failed.
//
int saves_spell(Character *ch, Character *vict, int spell_base, int16_t save_type)
{
  double save = 0;

  // Gods always succeed saving throws.  We rock!
  if (IS_MINLEVEL_PC(vict, IMMORTAL))
  {
    return 100;
  }

  // If a God attacks you, nothing can save you
  if (IS_MINLEVEL_PC(ch, DEITY) && vict->getLevel() < ch->getLevel())
  {
    return -100;
  }

  // Get the base save type for this roll
  switch (save_type)
  {
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
    if (isSet(vict->immune, ISR_MAGIC))
      return (true);
    if (isSet(vict->suscept, ISR_MAGIC))
      save *= 0.7;
    if (isSet(vict->resist, ISR_MAGIC))
      save *= 1.3;
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
  for (; *string && (*string) == ' '; string++)
    ;

  return (string);
}

/*
    Release command.
*/
int do_release(Character *ch, char *argument, int cmd)
{
  struct affected_type *aff, *aff_next;
  bool printed = false;
  argument = skip_spaces(argument);
  bool done = false;
  int learned = ch->has_skill(SKILL_RELEASE);

  if (!learned)
  {
    ch->sendln("You don't know how!");
    return eFAILURE;
  }

  if (!*argument)
  {
    ch->sendln("Release what spell?");
    for (aff = ch->affected; aff; aff = aff_next)
    {
      aff_next = aff->next;
      //       while (aff_next && aff_next->type == aff->type)
      //	aff_next = aff_next->next;
      if (get_skill_name(aff->type).isEmpty())
        continue;
      if (!printed)
      {
        ch->sendln("You can release the following spells:");
        printed = true;
      }
      if (((spell_info[aff->type].targets & TAR_SELF_DEFAULT) ||
           aff->type == SPELL_HOLY_AURA) &&
          aff->type != SPELL_IMMUNITY)
      { // Spells that default to self seems a good measure of
        // allow to release spells..
        QString aff_name = get_skill_name(aff->type);
        ch->send(aff_name);
        ch->sendln("");
      }
    }
    return eSUCCESS;
  }
  else
  {
    if (ch->getMove() < 25)
    {
      ch->sendln("You don't have enough moves.");
      return eFAILURE;
    }

    for (aff = ch->affected; aff; aff = aff_next)
    {
      aff_next = aff->next;
      while (aff_next && aff_next->type == aff->type)
        aff_next = aff_next->next;

      if (get_skill_name(aff->type).isEmpty())
        continue;
      if (str_prefix(argument, get_skill_name(aff->type).toStdString().c_str()))
        continue;
      if (aff->type > MAX_SPL_LIST)
        continue;
      if (!isSet(spell_info[aff->type].targets,
                 TAR_SELF_DEFAULT) &&
          aff->type != SPELL_HOLY_AURA)
        continue;
      if ((aff->type > 0) && (aff->type <= MAX_SPL_LIST))
        if (!done && !skill_success(ch, nullptr, SKILL_RELEASE))
        {
          ch->sendln("You failed to release the spell, and are left momentarily dazed.");
          act(
              "$n fails to release the magic surrounding $m and is left momentarily dazed.",
              ch, 0, 0, TO_ROOM, INVIS_NULL);
          WAIT_STATE(ch, DC::PULSE_VIOLENCE / 2);
          ch->decrementMove(10);
          return eFAILURE;
        }

      if (!done)
      {
        ch->decrementMove(25);
      }
      ch->sendln("You release the spell.");
      char buffer[255];
      int aftype = aff->type;

      if (*spell_wear_off_msg[aff->type])
      {
        send_to_char(spell_wear_off_msg[aftype], ch);
        ch->sendln("");
      }
      affect_from_char(ch, aftype);
      //	  affect_remove(ch,aff,0);
      snprintf(buffer, 255, "$n concentrates for a moment and releases their %s.", get_skill_name(aftype).toStdString().c_str());
      act(buffer, ch, 0, 0, TO_ROOM, INVIS_NULL);

      done = true;
    }
  }
  if (!done)
    ch->sendln("No such spell to release.");
  return eSUCCESS;
}

int skill_value(Character *ch, int skillnum, int min = 33)
{
  if (ch->skills.contains(skillnum))
  {
    return MAX(min, (int)ch->skills[skillnum].learned);
  }

  return 0;
}

int stat_mod[] = {
    0, -5, -5, -4, -4, -3, -3, -2, -2, -1, -1,
    0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7,
    7, 8, 9, 10};

int get_difficulty(int skillnum)
{
  //  extern struct skill_stuff skill_info[];
  extern struct ki_info_type ki_info[];
  extern struct song_info_type song_info[];

  if (skillnum >= SKILL_BASE && skillnum <= SKILL_MAX)
    return skill_info[skillnum - SKILL_BASE].difficulty;
  if (skillnum >= KI_OFFSET && skillnum <= KI_OFFSET + MAX_KI_LIST)
    return ki_info[skillnum - KI_OFFSET].difficulty;
  if (skillnum >= SKILL_SONG_BASE && skillnum <= SKILL_SONG_MAX)
    return song_info[skillnum - SKILL_SONG_BASE].difficulty;

  return 0;
}

bool Character::skill_success(Character *victim, int skillnum, int mod)
{
  //  extern int stat_mod[];
  //  int modifier = 0;
  // struct class_skill_defines *t;
  attribute_t stat{};

  switch (skillnum)
  {
  case SKILL_AMBUSH:
    stat = attribute_t::INTELLIGENCE;
    break;
  case SKILL_KICK:
    stat = attribute_t::DEXTERITY;
    break;
  case SKILL_BASH:
    stat = attribute_t::STRENGTH;
    break;
  case SKILL_RAGE:
    stat = attribute_t::CONSTITUTION;
    break;
  case SKILL_BERSERK:
    stat = attribute_t::STRENGTH;
    break;
  case KI_OFFSET + KI_PUNCH:
    stat = attribute_t::DEXTERITY;
    break;
  case KI_OFFSET + KI_DISRUPT:
    stat = attribute_t::INTELLIGENCE;
    break;
  case SKILL_DISARM:
    stat = attribute_t::DEXTERITY;
    break;
  case SKILL_TRACK:
    stat = attribute_t::WISDOM;
    break;
  case SKILL_BULLRUSH:
    stat = attribute_t::STRENGTH;
    break;
  case SKILL_HEADBUTT:
    stat = attribute_t::CONSTITUTION;
    break;
  case SKILL_HITALL:
    stat = attribute_t::STRENGTH;
    break;
  case SKILL_STUN:
    stat = attribute_t::DEXTERITY;
    mod -= GET_DEX(victim) / 2; // ADDITIONAL mod
    break;
  case SKILL_DEATHSTROKE:
    stat = attribute_t::STRENGTH;
    break;
  case SKILL_QUIVERING_PALM:
    stat = attribute_t::STRENGTH;
    break;
  case SKILL_EAGLE_CLAW:
    stat = attribute_t::STRENGTH;
    break;
  case SKILL_BACKSTAB:
    stat = attribute_t::DEXTERITY;
    break;
  case SKILL_ARCHERY:
    stat = attribute_t::DEXTERITY;
    break;
  case SKILL_DUAL_BACKSTAB:
    stat = attribute_t::DEXTERITY;
    break;
  case SKILL_CIRCLE:
    stat = attribute_t::DEXTERITY;
    break;
  case SKILL_TRIP:
    stat = attribute_t::DEXTERITY;
    break;
  case SKILL_STEAL:
    stat = attribute_t::DEXTERITY;
    break;
  case SKILL_POCKET:
    stat = attribute_t::INTELLIGENCE;
    break;
  case SKILL_STALK:
    stat = attribute_t::CONSTITUTION;
    break;
  case SKILL_CONSIDER:
    stat = attribute_t::WISDOM;
    break;
  case SKILL_EYEGOUGE:
    stat = attribute_t::CONSTITUTION;
    break;
  }
  int i = 0, learned = 0;

  if (!isNPC())
  {
    i = learned = has_skill(skillnum);
    if (affected_by_spell(SKILL_DEFENDERS_STANCE) && skillnum == SKILL_DODGE)
    {
      learned = affected_by_spell(SKILL_DEFENDERS_STANCE)->modifier;
    }
    else if (!learned)
      return false;
  }
  else
  {
    if (getLevel() < 30)
      i = 30;
    else if (getLevel() < 50)
      i = 40;
    else if (getLevel() < 70)
      i = 60;
    else if (getLevel() < 90)
      i = 70;
    else
      i = 75;
  }
  if (stat != attribute_t::UNDEFINED && victim)
  {
    auto max_level_percent = getLevel() / 60.0;
    i -= stat_mod[victim->get_stat(stat)] * max_level_percent; // less impact on low levels..
  }
  i += mod;

  if (isPlayer())
    i = 50 + i / 2;

  if (skillnum != SKILL_THIRD_ATTACK && skillnum != SKILL_SECOND_ATTACK && skillnum != SKILL_DUAL_WIELD)
    i = MIN(96, i);
  else
    i = MIN(98, i);

  if (skillnum == SKILL_IMBUE)
    i = MIN(90, i); // max 90% success rate for imbue

  i = skillmax(skillnum, i);
  if (IS_AFFECTED(this, AFF_FOCUS) &&
      ((skillnum >= SKILL_SONG_BASE &&
        skillnum <= SKILL_SONG_MAX) ||
       (skillnum >= KI_OFFSET && skillnum <= (KI_OFFSET + MAX_KI_LIST))))
    i = 101; // auto success on songs and ki with focus

  int a = get_difficulty(skillnum);
  /*  int o = ch->getLevel()*2+1;
    if (o > 101 || IS_NPC(ch)) o = 101;
    if (i > o) o = i+1;
  */

  if (i > number(1, 100) || getLevel() >= IMMORTAL)
  {
    if (skillnum != SKILL_ENHANCED_REGEN || (skillnum == SKILL_ENHANCED_REGEN && getHP() + 50 < GET_MAX_HIT(this) && (GET_POS(this) == position_t::RESTING || GET_POS(this) == position_t::SLEEPING)))
      skill_increase_check(skillnum, learned, a + 500);
    return true; // Success
  }
  else
  {
    /* Check for skill improvement anyway */
    if (skillnum != SKILL_ENHANCED_REGEN || (skillnum == SKILL_ENHANCED_REGEN && this->getHP() + 50 < GET_MAX_HIT(this) && (GET_POS(this) == position_t::RESTING || GET_POS(this) == position_t::SLEEPING)))
      skill_increase_check(skillnum, learned, a);
    return false; // Failure
  }
}

bool skill_success(Character *ch, Character *victim, int skillnum, int mod)
{
  return ch->skill_success(victim, skillnum, mod);
}

void set_conc_loss(Character *ch, int spl)
{
  struct affected_type af;
  af.type = CONC_LOSS_FIXER;
  af.duration = 1;
  af.modifier = spl;
  af.location = 0;
  af.bitvector = -1;

  affect_to_char(ch, &af);
  return;
}
bool check_conc_loss(Character *ch, int spl)
{
  struct affected_type *af;
  int afspl;
  if (!(af = ch->affected_by_spell(CONC_LOSS_FIXER)))
    return false;

  afspl = af->modifier;

  affect_from_char(ch, CONC_LOSS_FIXER);

  if (afspl != spl)
    return false;

  return true;
}

// Assumes that *argument does start with first letter of chopped string
int do_cast(Character *ch, char *argument, int cmd)
{
  class Object *tar_obj;
  Character *tar_char;
  char name[MAX_STRING_LENGTH], filter[MAX_STRING_LENGTH];
  int qend, spl, i, learned;
  bool target_ok;

  //  if (IS_NPC(ch))
  //    return eFAILURE;
  // Need to allow mob_progs to use cast without allowing charmies to

  if (IS_NPC(ch) && ch->desc && ch->desc->original && ch->desc->original != ch->desc->character && ch->desc->original->isMortalPlayer())
  {
    ch->sendln("You cannot cast in this form.");
    return eFAILURE;
  }

  if (ch->affected_by_spell(SPELL_NO_CAST_TIMER))
  {
    ch->sendln("You seem unable to concentrate enough to cast any spells.");
    return eFAILURE;
  }

  Object *tmp_obj;
  for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (DC::getInstance()->obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
    {
      ch->sendln("The magical silence prevents you from casting!");
      return eFAILURE;
    }

  if (IS_AFFECTED(ch, AFF_CHARM))
  {
    ch->sendln("You cannot cast while charmed!");
    return eFAILURE;
  }

  if (ch->getLevel() < ARCHANGEL && (IS_PC(ch) || IS_AFFECTED(ch, AFF_CHARM)))
  {
    if (GET_CLASS(ch) == CLASS_WARRIOR)
    {
      ch->sendln("Think you had better stick to fighting...");
      return eFAILURE;
    }
    else if (GET_CLASS(ch) == CLASS_THIEF)
    {
      ch->sendln("Think you should stick to robbing and killing...");
      return eFAILURE;
    }
    else if (GET_CLASS(ch) == CLASS_BARBARIAN)
    {
      ch->sendln("Think you should stick to berserking...");
      return eFAILURE;
    }
    else if (GET_CLASS(ch) == CLASS_MONK)
    {
      ch->sendln("Think you should stick with meditating...");
      return eFAILURE;
    }
    else if ((GET_CLASS(ch) == CLASS_ANTI_PAL) && (!IS_EVIL(ch)))
    {
      ch->sendln("You're not evil enough!");
      return eFAILURE;
    }
    else if ((GET_CLASS(ch) == CLASS_PALADIN) && (!IS_GOOD(ch)))
    {
      ch->sendln("You're not pure enough!");
      return eFAILURE;
    }
    else if (GET_CLASS(ch) == CLASS_BARD)
    {
      ch->send("Stick to singing bucko.");
      return eFAILURE;
    }
    if (isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_MAGIC))
    {
      ch->sendln("You find yourself unable to weave magic here.");
      return eFAILURE;
    }
  }

  argument = skip_spaces(argument);

  /* If there is no chars in argument */
  if (!(*argument))
  {
    ch->sendln("Cast which what where?");
    return eFAILURE;
  }

  if (*argument != '\'')
  {
    ch->sendln("Magic must always be enclosed by the holy magic symbols : '");
    return eFAILURE;
  }

  /* Locate the last quote && lowercase the magic words (if any) */

  for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
    *(argument + qend) = LOWER(*(argument + qend));

  if (*(argument + qend) != '\'')
  {
    ch->sendln("Magic must always be enclosed by the holy magic symbols : '");
    return eFAILURE;
  }

  spl = old_search_block(argument, 1, qend - 1, spells, 0);
  if (spl <= 0)
  {
    ch->sendln("Your lips do not move, no magic appears.");
    return eFAILURE;
  }
  if (spl == SPELL_DIVINE_INTER && ch->affected_by_spell(SPELL_DIV_INT_TIMER))
  {
    ch->sendln("The gods are unwilling to intervene on your behalf again so soon.");
    return eFAILURE;
  }

  if (spell_info[spl].spell_pointer)
  {
    if (GET_POS(ch) < spell_info[spl].minimum_position)
    {
      switch (GET_POS(ch))
      {
      case position_t::SLEEPING:
        ch->sendln("You dream about great magical powers.");
        break;
      case position_t::RESTING:
        ch->sendln("You can't concentrate enough while resting.");
        break;
      case position_t::SITTING:
        ch->sendln("You can't do this sitting!");
        break;
      case position_t::FIGHTING:
        ch->sendln("Impossible! You can't concentrate enough!");
        break;
      default:
        ch->sendln("It seems like you're in a pretty bad shape!");
        break;
      } /* Switch */
    }
    else
    {
      if (!IS_MOB(ch))
      {
        if (!(learned = ch->has_skill(spl)))
        {
          if (ch->getLevel() < 101)
          {
            ch->sendln("You do not know how to cast that spell!");
            return eFAILURE;
          }
          else
          {
            learned = 80;
          }
        }
      }
      else
        learned = 80;

      argument += qend + 1; /* Point to the last ' */
      for (; *argument == ' '; argument++)
        ;

      /* **************** Locate targets **************** */

      target_ok = false;
      tar_char = 0;
      tar_obj = 0;
      bool ok_self = false;
      int oldroom = 0;
      int dir = -1;
      bool group_spell = false;
      if (spl == SPELL_LIGHTNING_BOLT && ch->has_skill(SKILL_SPELLCRAFT) && cmd != CMD_FILTER)
      { // Oh the special cases of spellcraft.

        name[0] = '\0';
        one_argument(argument, name);
        if (argument && strlen(argument) > strlen(name))
        {
          *argument = LOWER(*(argument + strlen(name) + 1));

          if (*argument == 'n')
            dir = 0;
          else if (*argument == 'e')
            dir = 1;
          else if (*argument == 's')
            dir = 2;
          else if (*argument == 'w')
            dir = 3;
          else if (*argument == 'u')
            dir = 4;
          else if (*argument == 'd')
            dir = 5;
          if (dir == -1)
          {
            ch->sendln("Fire a lightning bolt where?");
            return eFAILURE;
          }
          if (!DC::getInstance()->world[ch->in_room].dir_option[dir])
          {
            ch->sendln("The wall blocks your attempt.");
            return eFAILURE;
          }
          if (!CAN_GO(ch, dir))
          {
            ch->sendln("You cannot do that.");
            return eFAILURE;
          }
          if (ch->fighting)
          {
            ch->sendln("You cannot concentrate enough to fire a bolt of lightning into another room!");
            return eFAILURE;
          }
          int new_room = DC::getInstance()->world[ch->in_room].dir_option[dir]->to_room;
          if (isSet(DC::getInstance()->world[new_room].room_flags, SAFE) || isSet(DC::getInstance()->world[new_room].room_flags, NO_MAGIC))
          {
            ch->sendln("That room is protected from this harmful magic.");
            return eFAILURE;
          }

          // can't use spellcraft(ch, SPELL_LIGHTNING_BOLT) here because it
          // will cause spellcraft to increase possibly
          if ((ch->has_skill(SPELL_LIGHTNING_BOLT) < 71) ||
              (ch->has_skill(SKILL_SPELLCRAFT) < 21))
          {
            ch->sendln("You don't know how.");
            return eFAILURE;
          }

          oldroom = ch->in_room;
          char_from_room(ch);
          if (!char_to_room(ch, new_room))
          {
            char_to_room(ch, oldroom);
            ch->sendln("Error code: 57A. Report this to an immortal, along with what you typed and where.");
            return eFAILURE;
          }
          if (!(tar_char = ch->get_char_room_vis(name)))
          {
            char_from_room(ch);
            char_to_room(ch, oldroom);
            ch->sendln("You don't see anyone like that there.");
            return eFAILURE;
          }

          if (IS_NPC(tar_char) && DC::getInstance()->mob_index[tar_char->mobdata->nr].virt >= 2300 &&
              DC::getInstance()->mob_index[tar_char->mobdata->nr].virt <= 2399)
          {
            char_from_room(ch);
            char_to_room(ch, oldroom);
            tar_char = ch;
            ch->sendln("Your spell bounces off the fortress' enchantments, and the lightning bolt comes flying back towards you!");
            ok_self = true;
          }
          target_ok = true;

          // Reduce timer on paralyze even the victim is hit by a lightning bolt
          affected_type *af;
          if ((af = tar_char->affected_by_spell(SPELL_PARALYZE)) != nullptr)
          {
            af->duration--;
            if (af->duration <= 0)
            {
              affect_remove(tar_char, af, 0);
            }
          }
        }
        spellcraft(ch, SPELL_LIGHTNING_BOLT);
      } // end lightning bolt

      if (spl == SPELL_IMMUNITY)
      {
        argument = skip_spaces(argument);

        for (int64_t i = 0; i < MAX_SPL_LIST; i++)
        {
          if (!strcmp(spells[i], argument))
          {
            ok_self = true;
            tar_char = ch;
            target_ok = true;
            argument = (char *)i;
            break;
          }
        }
        if (!target_ok)
        {
          ch->sendln("There is no such known spell in the realms to protect yourself against.");
          return eFAILURE;
        }
      } // end spell immunity
      int fil = 0;
      float rel = 1;
      int fillvl = ch->has_skill(SKILL_ELEMENTAL_FILTER);
      if (cmd == CMD_FILTER && fillvl)
      {
        if (spl == SPELL_BURNING_HANDS || spl == SPELL_FIREBALL || spl == SPELL_FIRESTORM || spl == SPELL_HELLSTREAM ||
            spl == SPELL_MAGIC_MISSILE || spl == SPELL_METEOR_SWARM || spl == SPELL_LIGHTNING_BOLT || spl == SPELL_CHILL_TOUCH)
        {
          name[0] = '\0';
          filter[0] = '\0';
          argument = one_argument(argument, filter);

          if (*filter)
          {

            if (*filter == 'f')
              fil = FILTER_FIRE;
            else if (*filter == 'm')
              fil = FILTER_MAGIC;
            else if (*filter == 'c')
              fil = FILTER_COLD;
            else if (*filter == 'e')
              fil = FILTER_ENERGY;

            if (!fil)
            {
              ch->sendln("You do not know how to filter your spell through that.");
              return eFAILURE;
            }

            if (spl == SPELL_BURNING_HANDS || spl == SPELL_FIREBALL || spl == SPELL_FIRESTORM || spl == SPELL_HELLSTREAM)
            {
              if (fil == FILTER_MAGIC && fillvl > 50)
                rel = 1.25;
              else if (fil == FILTER_ENERGY && fillvl > 70)
                rel = 1.5;
              else if (fil == FILTER_COLD && fillvl > 90)
                rel = 1.75;
              else
                fil = 0;
            }
            else if (spl == SPELL_MAGIC_MISSILE || spl == SPELL_METEOR_SWARM)
            {
              if (fil == FILTER_FIRE && fillvl > 50)
                rel = 1.25;
              else if (fil == FILTER_COLD && fillvl > 70)
                rel = 1.5;
              else if (fil == FILTER_ENERGY && fillvl > 90)
                rel = 1.75;
              else
                fil = 0;
            }
            else if (spl == SPELL_LIGHTNING_BOLT)
            {
              if (fil == FILTER_COLD && fillvl > 50)
                rel = 1.25;
              else if (fil == FILTER_FIRE && fillvl > 70)
                rel = 1.5;
              else if (fil == FILTER_MAGIC && fillvl > 90)
                rel = 1.75;
              else
                fil = 0;
            }
            else if (spl == SPELL_CHILL_TOUCH)
            {
              if (fil == FILTER_ENERGY && fillvl > 50)
                rel = 1.25;
              else if (fil == FILTER_MAGIC && fillvl > 70)
                rel = 1.5;
              else if (fil == FILTER_FIRE && fillvl > 90)
                rel = 1.75;
              else
                fil = 0;
            }
          }
          else
          {
            ch->sendln("You need to specify a filter type.");
            return eFAILURE;
          }

        } // end if filterable spell
      } // end filter
      if (!target_ok && !isSet(spell_info[spl].targets, TAR_IGNORE))
      {
        argument = one_argument(argument, name);

        if (*name)
        {
          if (isSet(spell_info[spl].targets, TAR_CHAR_ROOM))
          {
            if ((tar_char = ch->get_char_room_vis(name)) != nullptr)
              target_ok = true;
            if (!str_cmp(name, "group") && ch->has_skill(SKILL_COMMUNE))
            {
              if ((ch->has_skill(SKILL_COMMUNE) >= 90 && (spl == SPELL_PROTECT_FROM_EVIL || spl == SPELL_RESIST_MAGIC || spl == SPELL_PROTECT_FROM_GOOD)) || (ch->has_skill(SKILL_COMMUNE) >= 70 && (spl == SPELL_CURE_CRITIC || spl == SPELL_SANCTUARY || spl == SPELL_REMOVE_POISON || spl == SPELL_REMOVE_BLIND || spl == SPELL_IRIDESCENT_AURA)) || (ch->has_skill(SKILL_COMMUNE) >= 40 && (spl == SPELL_ARMOR || spl == SPELL_REFRESH || spl == SPELL_REMOVE_PARALYSIS || spl == SPELL_CURE_SERIOUS || spl == SPELL_BLESS || spl == SPELL_FLY)) || (spl == SPELL_DETECT_INVISIBLE || spl == SPELL_DETECT_MAGIC || spl == SPELL_DETECT_POISON || spl == SPELL_SENSE_LIFE || spl == SPELL_CURE_LIGHT))
              {
                ch->skill_increase_check(SKILL_COMMUNE, ch->has_skill(SKILL_COMMUNE), SKILL_INCREASE_HARD);
                target_ok = true;
                group_spell = true;
                tar_char = ch;
              }
              else
              {
                ch->sendln("You cannot cast this spell on your entire group at once.");
                return eFAILURE;
              }
            }
          }

          if (!target_ok && isSet(spell_info[spl].targets, TAR_CHAR_WORLD))
          {
            bool orig = ISSET(ch->affected_by, AFF_true_SIGHT);
            if (spl == SPELL_EAGLE_EYE)
              SETBIT(ch->affected_by, AFF_true_SIGHT);
            if ((tar_char = get_char_vis(ch, name)) != nullptr)
              target_ok = true;
            if (!orig)
              REMBIT(ch->affected_by, AFF_true_SIGHT);
          }
          if (!target_ok && isSet(spell_info[spl].targets, TAR_OBJ_INV))
            if ((tar_obj = get_obj_in_list_vis(ch, name, ch->carrying)) != nullptr)
              target_ok = true;

          if (!target_ok && isSet(spell_info[spl].targets, TAR_OBJ_ROOM))
          {
            tar_obj = get_obj_in_list_vis(ch, name, DC::getInstance()->world[ch->in_room].contents);
            if (tar_obj != nullptr)
              target_ok = true;
          }

          if (!target_ok && isSet(spell_info[spl].targets, TAR_OBJ_WORLD))
            if ((tar_obj = get_obj_vis(ch, name, true)) != nullptr)
              /* && !(isSet(tar_obj->obj_flags.more_flags, ITEM_NOLOCATE)))*/
              target_ok = true;

          if (!target_ok && isSet(spell_info[spl].targets, TAR_OBJ_EQUIP))
          {
            for (i = 0; i < MAX_WEAR && !target_ok; i++)
              if (ch->equipment[i] && str_cmp(name, ch->equipment[i]->name) == 0)
              {
                tar_obj = ch->equipment[i];
                target_ok = true;
              }
          }

          if (!target_ok && isSet(spell_info[spl].targets, TAR_SELF_ONLY))
            if (str_cmp(GET_NAME(ch), name) == 0)
            {
              tar_char = ch;
              target_ok = true;
            }
        }
        else
        { // !*name No argument was typed

          if (isSet(spell_info[spl].targets, TAR_FIGHT_SELF))
            if ((ch->fighting) && ((ch->fighting)->in_room == ch->in_room))
            {
              tar_char = ch;
              target_ok = true;
            }

          if (!target_ok && isSet(spell_info[spl].targets, TAR_FIGHT_VICT))
            if (ch->fighting && (ch->in_room == ch->fighting->in_room))
            // added the in_room checks -pir2/23/01
            {
              tar_char = ch->fighting;
              target_ok = true;
            }

          if (!target_ok && (isSet(spell_info[spl].targets, TAR_SELF_ONLY) ||
                             isSet(spell_info[spl].targets, TAR_SELF_DEFAULT)))
          {
            tar_char = ch;
            target_ok = true;
          }

          if (!target_ok && isSet(spell_info[spl].targets, TAR_NONE_OK))
          {
            target_ok = true;
          }
        }
      }
      else
      {                   // tar_ignore is true
        target_ok = true; /* No target, is a good target */
      }

      if (!target_ok)
      {
        if (*name)
        {
          if (isSet(spell_info[spl].targets, TAR_CHAR_ROOM))
            ch->sendln("Nobody here by that name.");
          else if (isSet(spell_info[spl].targets, TAR_CHAR_WORLD))
            ch->sendln("Nobody playing by that name.");
          else if (isSet(spell_info[spl].targets, TAR_OBJ_INV))
            ch->sendln("You are not carrying anything like that.");
          else if (isSet(spell_info[spl].targets, TAR_OBJ_ROOM))
            ch->sendln("Nothing here by that name.");
          else if (isSet(spell_info[spl].targets, TAR_OBJ_WORLD))
            ch->sendln("Nothing at all by that name.");
          else if (isSet(spell_info[spl].targets, TAR_OBJ_EQUIP))
            ch->sendln("You are not wearing anything like that.");
          else if (isSet(spell_info[spl].targets, TAR_OBJ_WORLD))
            ch->sendln("Nothing at all by that name.");
        }
        else
        { /* Nothing was given as argument */
          if (spell_info[spl].targets < TAR_OBJ_INV)
            ch->sendln("Whom should the spell be cast upon?");
          else
            ch->sendln("What should the spell be cast upon?");
        }
        return eFAILURE;
      }
      else
      { /* TARGET IS OK */
        if ((tar_char == ch) && !ok_self && isSet(spell_info[spl].targets, TAR_SELF_NONO))
        {
          if (oldroom)
          {
            char_from_room(ch);
            char_to_room(ch, oldroom);
          }
          ch->sendln("You can not cast this spell upon yourself.");
          return eFAILURE;
        }
        else if ((tar_char != ch) && isSet(spell_info[spl].targets, TAR_SELF_ONLY))
        {
          ch->sendln("You can only cast this spell upon yourself.");
          return eFAILURE;
        }
        else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tar_char))
        {
          ch->sendln("You are afraid that it could harm your master.");
          return eFAILURE;
        }
      }

      if (ch->getLevel() < ARCHANGEL && !IS_MOB(ch))
      {
        if (GET_MANA(ch) < use_mana(ch, spl) * rel)
        {
          if (oldroom)
          {
            char_from_room(ch);
            char_to_room(ch, oldroom);
          }
          ch->sendln("You can't summon enough energy to cast the spell.");
          return eFAILURE;
        }
      }
      if (tar_char && isSet(spell_info[spl].targets, TAR_FIGHT_VICT))
      {
        if (!can_attack(ch) || !can_be_attacked(ch, tar_char))
        {
          if (oldroom)
          {
            char_from_room(ch);
            char_to_room(ch, oldroom);
          }
          return eFAILURE;
        }
      }

      int retval = 0;
      if (spl != SPELL_VENTRILOQUATE)
      { /* :-) */
        retval = say_spell(ch, spl, oldroom);

        // Someone died, most likely the mob due to an act_trigger program
        if (SOMEONE_DIED(retval))
        {
          return eSUCCESS;
        }
      }

      if (cmd == CMD_FILTER && fillvl)
      {
        ch->skill_increase_check(SKILL_ELEMENTAL_FILTER, fillvl, SKILL_INCREASE_HARD);
      }

      if (!spellcraft(ch, spl) || (spl != SPELL_MAGIC_MISSILE && spl != SPELL_FIREBALL))
        WAIT_STATE(ch, spell_info[spl].beats);
      else
        WAIT_STATE(ch, (int)(spell_info[spl].beats / 1.5));

      if ((spell_info[spl].spell_pointer == 0) && spl > 0)
        ch->sendln("Sorry, this magic has not yet been implemented :(");
      else
      {
        int chance = 50;

        if (IS_MOB(ch))
        {
          learned = ch->getLevel();
        }

        chance += learned / 1.5;

        if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL)
          chance += int_app[GET_INT(ch)].conc_bonus;
        else
          chance += wis_app[GET_WIS(ch)].conc_bonus;

        if (GET_RACE(ch) == RACE_HUMAN)
          chance = MIN(95, chance);
        else
          chance = MIN(97, chance);

        if (ch->getLevel() == 1)
          chance++;

        if (check_conc_loss(ch, spl))
          chance = 99;

        if (IS_AFFECTED(ch, AFF_CRIPPLE) && ch->affected_by_spell(SKILL_CRIPPLE))
          chance -= 1 + ch->affected_by_spell(SKILL_CRIPPLE)->modifier / 10;

        if (!ch->isImmortalPlayer() && number(1, 100) > chance && !IS_AFFECTED(ch, AFF_FOCUS) && !isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
        {
          set_conc_loss(ch, spl);
          csendf(ch, "You lost your concentration and are unable to cast %s!\n\r", spells[spl - 1]);
          if (rel > 1)
          {
            ch->sendln("The failed elemental filter drains you of additional mana.");
          }
          GET_MANA(ch) -= (use_mana(ch, spl) >> 1) * rel;
          act("$n loses $s concentration and is unable to complete $s spell.", ch, 0, 0, TO_ROOM, 0);
          ch->skill_increase_check(spl, learned, spell_info[spl].difficulty);
          if (oldroom)
          {
            char_from_room(ch);
            char_to_room(ch, oldroom);
          }
          return eSUCCESS;
        }

        if (IS_AFFECTED(ch, AFF_INVISIBLE) && !IS_AFFECTED(ch, AFF_ILLUSION) && ch->affected_by_spell(SPELL_INVISIBLE))
        {
          act("$n slowly fades into existence.", ch, 0, 0, TO_ROOM, 0);
          affect_from_char(ch, SPELL_INVISIBLE);
          REMBIT(ch->affected_by, AFF_INVISIBLE);
        }

        ch->sendln("Ok.");

        if (group_spell)
        {
          Character *leader;
          if (ch->master)
            leader = ch->master;
          else
            leader = ch;

          struct follow_type *k;
          int counter = 0;

          for (k = leader->followers; k; k = k->next)
            if (k->follower->in_room == ch->in_room)
              counter++;
          if (leader->in_room == ch->in_room)
            counter++;

          counter = MIN(5, counter);
          if (learned >= 80 && counter > 3)
            counter--;
          if (learned >= 90 && counter > 3)
            counter--;
          counter *= use_mana(ch, spl);
          if (GET_MANA(ch) < counter)
          {
            ch->sendln("You do not have enough mana to cast this group spell.");
            return eFAILURE;
          }
          GET_MANA(ch) -= counter;
        }
        else
          GET_MANA(ch) -= (use_mana(ch, spl) * rel);
        if (tar_char && !AWAKE(tar_char) && ch->in_room == tar_char->in_room && number(1, 5) < 3)
          tar_char->sendln("Your sleep is restless.");
        ch->skill_increase_check(spl, learned, 500 + spell_info[spl].difficulty);

        if (tar_char && tar_char != ch && IS_PC(ch) && IS_PC(tar_char) && tar_char->desc && ch->desc)
        {
          /*
          if (!strcmp(tar_char->desc->getPeerOriginalAddress().toString().toStdString().c_str(), ch->desc->getPeerOriginalAddress().toString().toStdString().c_str()))
          {
            sprintf(log_buf, "Multi: %s casted '%s' on %s", GET_NAME(ch),
                    get_skill_name(spl), GET_NAME(tar_char));
            logentry(log_buf, 110, LogChannels::LOG_PLAYER, ch);
          }*/

          // Wizard's eye (88) is ok to cast
          // Prize Arena
          auto &arena = DC::getInstance()->arena_;
          if (ch->room().isArena() && arena.isPrize() && spl != 88)
          {
            if (tar_char && tar_char != ch && !isSet(spell_info[spl].targets, TAR_FIGHT_VICT))
            {
              ch->sendln("You can't cast that spell on someone in a prize arena.");
              logf(IMMORTAL, LogChannels::LOG_ARENA, "%s was prevented from casting '%s' on %s.",
                   GET_NAME(ch), get_skill_name(spl).toStdString().c_str(), GET_NAME(tar_char));
              return eFAILURE;
            }

            if (ch->fighting && ch->fighting != tar_char)
            {
              ch->sendln("You can't cast that because you're in a fight with someone else.");
              logf(IMMORTAL, LogChannels::LOG_ARENA, "%s, whom was fighting %s, was prevented from casting '%s' on %s.", GET_NAME(ch),
                   GET_NAME(ch->fighting), get_skill_name(spl).toStdString().c_str(), GET_NAME(tar_char));
              return eFAILURE;
            }
            else if (tar_char->fighting && tar_char->fighting != ch)
            {
              ch->sendln("You can't cast that because they are fighting someone else.");
              logf(IMMORTAL, LogChannels::LOG_ARENA, "%s was prevented from casting '%s' on %s who was fighting %s.", GET_NAME(ch),
                   get_skill_name(spl).toStdString().c_str(), GET_NAME(tar_char), GET_NAME(tar_char->fighting));
              return eFAILURE;
            }
          }

          // Wizard's eye (88) is ok to cast
          // Clan Chaos
          if (ch->room().isArena() && arena.isChaos() && spl != 88)
          {
            if (tar_char && tar_char != ch && !isSet(spell_info[spl].targets, TAR_FIGHT_VICT) && !ARE_CLANNED(ch, tar_char))
            {
              ch->sendln("You can't cast that spell on someone from another clan in a prize arena.");
              logf(IMMORTAL, LogChannels::LOG_ARENA, "%s [%s] was prevented from casting '%s' on %s [%s].",
                   GET_NAME(ch), get_clan_name(ch), get_skill_name(spl).toStdString().c_str(), GET_NAME(tar_char), get_clan_name(tar_char));
              return eFAILURE;
            }

            if (ch->fighting && ch->fighting != tar_char && !ARE_CLANNED(ch->fighting, tar_char) && isSet(spell_info[spl].targets, TAR_FIGHT_VICT))
            {
              ch->sendln("You can't cast that because you're in a fight with someone else.");
              logf(IMMORTAL, LogChannels::LOG_ARENA, "%s [%s], whom was fighting %s [%s], was prevented from casting '%s' on %s [%s].",
                   GET_NAME(ch), get_clan_name(ch),
                   GET_NAME(ch->fighting), get_clan_name(ch->fighting),
                   get_skill_name(spl).toStdString().c_str(),
                   GET_NAME(tar_char), get_clan_name(tar_char));
              return eFAILURE;
            }
            else if (tar_char->fighting && tar_char->fighting != ch && !ARE_CLANNED(tar_char->fighting, ch) && isSet(spell_info[spl].targets, TAR_FIGHT_VICT))
            {
              ch->sendln("You can't cast that because they are fighting someone else.");
              logf(IMMORTAL, LogChannels::LOG_ARENA, "%s [%s] was prevented from casting '%s' on %s [%s] who was fighting %s [%s].",
                   GET_NAME(ch), get_clan_name(ch),
                   get_skill_name(spl).toStdString().c_str(),
                   GET_NAME(tar_char), get_clan_name(tar_char),
                   GET_NAME(tar_char->fighting), get_clan_name(tar_char->fighting));
              return eFAILURE;
            }
          }
        }

        if (tar_char && IS_AFFECTED(tar_char, AFF_REFLECT) && number(0, 99) < tar_char->spell_reflect)
        {
          if (ch == tar_char || ARE_GROUPED(ch, tar_char))
          { // some idiot was shooting at himself
            // out		  act("The spell harmlessly reflects off you and disperses.", tar_char, 0, 0, TO_CHAR, 0);
            // for		  act("The spell harmlessly reflects off $n and disperses.", tar_char, 0, 0, TO_ROOM, 0);
            // heals		  return eSUCCESS;
          }
          else
          {
            act("$n's spell bounces back at $m.", ch, 0, tar_char, TO_VICT, 0);
            act("Oh SHIT! Your spell bounces off of $N and heads right back at you.", ch, 0, tar_char, TO_CHAR, 0);
            act("$n's spell reflects off of $N's magical aura", ch, 0, tar_char, TO_ROOM, NOTVICT);
            tar_char = ch;

            // Ping-pong
            if (IS_AFFECTED(tar_char, AFF_REFLECT) && number(0, 99) < tar_char->spell_reflect)
            {
              act("The spell harmlessly reflects off you and disperses.", tar_char, 0, 0, TO_CHAR, 0);
              act("The spell harmlessly reflects off $n and disperses.", tar_char, 0, 0, TO_ROOM, 0);
              if (oldroom)
              {
                char_from_room(ch);
                char_to_room(ch, oldroom);
              }
              return eSUCCESS;
            }
          }
        }

        uint8_t level = ch->getLevel();
        char *argument_ptr = nullptr;

        if (group_spell)
        {
          ch->sendln("You utter a swift prayer to the gods to amplify your powers.");
          act("$n utters a swift prayer to the gods to amplify $s powers.", ch, 0, 0, TO_ROOM, 0);
          argument = strdup("communegroupspell");
          argument_ptr = argument;
        }
        else if (tar_char && tar_char->affected_by_spell(SPELL_IMMUNITY) && tar_char->affected_by_spell(SPELL_IMMUNITY)->modifier == spl - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_char, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_char, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_char, TO_ROOM, NOTVICT);
          return eSUCCESS;
        }
        else if (fil)
        {
          switch (fil)
          {
          case FILTER_FIRE:
            csendf(ch, "Upon casting, your %s filters through a $B$4blast of flame$R!\r\n", spells[spl - 1]);
            act("Upon casting, $n filters $s magic through a $B$4blast of flame$R!", ch, 0, 0, TO_ROOM, 0);
            level = 200 + TYPE_FIRE - TYPE_HIT;
            break;
          case FILTER_ACID:
            csendf(ch, "Upon casting, your %s filters through $B$2sizzling acid$R!\r\n", spells[spl - 1]);
            act("Upon casting, $n filters $s magic through $B$2sizzling acid$R!", ch, 0, 0, TO_ROOM, 0);
            level = 200 + TYPE_ACID - TYPE_HIT;
            break;
          case FILTER_COLD:
            csendf(ch, "Upon casting, your %s filters through $B$3shards of ice$R!\r\n", spells[spl - 1]);
            act("Upon casting, $n filters $s magic through $B$3shards of ice$R!", ch, 0, 0, TO_ROOM, 0);
            level = 200 + TYPE_COLD - TYPE_HIT;
            break;
          case FILTER_ENERGY:
            csendf(ch, "Upon casting, your %s filters through $B$5crackling energy$R!\r\n", spells[spl - 1]);
            act("Upon casting, $n filters $s magic through $B$4crackling energy$R!", ch, 0, 0, TO_ROOM, 0);
            level = 200 + TYPE_ENERGY - TYPE_HIT;
            break;
          case FILTER_MAGIC:
            csendf(ch, "Upon casting, your %s filters through a $B$7burst of magic$R!\r\n", spells[spl - 1]);
            act("Upon casting, $n filters $s magic through a $B$4burst of magic$R!", ch, 0, 0, TO_ROOM, 0);
            level = 200 + TYPE_MAGIC - TYPE_HIT;
            break;
          case FILTER_POISON:
            csendf(ch, "Upon casting, your %s filters through $2poisonous fumes$R!\r\n", spells[spl - 1]);
            act("Upon casting, $n filters $s magic through $2poisonous fumes$R!", ch, 0, 0, TO_ROOM, 0);
            level = 200 + TYPE_POISON - TYPE_HIT;
            break;
          default:
            ch->sendln("WTF?!?!?!?!, tell an immortal about this.");
            break;
          }
        }

        int retval = ((*spell_info[spl].spell_pointer)(level, ch, argument, SPELL_TYPE_SPELL, tar_char, tar_obj, learned));
        if (argument_ptr != nullptr)
        {
          free(argument_ptr);
        }

        if (oldroom && !isSet(retval, eCH_DIED))
        {
          char_from_room(ch);
          char_to_room(ch, oldroom);
          WAIT_STATE(ch, (int)(spell_info[spl].beats));

          if (spl == SPELL_LIGHTNING_BOLT)
          {
            char buffer[MAX_STRING_LENGTH];
            strcpy(buffer, "$n unleashes a bolt of $B$5lightning$R to the ");
            switch (dir)
            {
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
    } /* if GET_POS < min_pos */
    return eFAILURE;
  }
  else
  {
    ch->sendln("Your lips do not move, no magic appears.");
  }
  return eFAILURE;
}

int do_skills(Character *ch, char *arg, int cmd)
{
  char buf[16384];
  char buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
  int mage, cleric, thief, warrior, anti, pal, barb, monk, ranger, bard, druid;
  if (IS_NPC(ch))
    return eFAILURE;

  buf[0] = '\0';

  for (int i = SKILL_BASE; i <= SKILL_MAX; i++)
  {
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
    for (int j = 0; m_skills[j].skillnum; j++)
    {
      if (m_skills[j].skillnum == i)
      {
        mage = j;
        sprintf(buf2, "Mag(%d)", m_skills[j].levelavailable);
        break;
      }
    }
    for (int j = 0; c_skills[j].skillnum; j++)
    {
      if (c_skills[j].skillnum == i)
      {
        cleric = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Cle(%d)", c_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; t_skills[j].skillnum; j++)
    {
      if (t_skills[j].skillnum == i)
      {
        thief = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Thi(%d)", t_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; w_skills[j].skillnum; j++)
    {
      if (w_skills[j].skillnum == i)
      {
        warrior = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "War(%d)", w_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; a_skills[j].skillnum; j++)
    {
      if (a_skills[j].skillnum == i)
      {
        anti = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Ant(%d)", a_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; p_skills[j].skillnum; j++)
    {
      if (p_skills[j].skillnum == i)
      {
        pal = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Pal(%d)", p_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; b_skills[j].skillnum; j++)
    {
      if (b_skills[j].skillnum == i)
      {
        barb = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Bar(%d)", b_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; k_skills[j].skillnum; j++)
    {
      if (k_skills[j].skillnum == i)
      {
        monk = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Mon(%d)", k_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; r_skills[j].skillnum; j++)
    {
      if (r_skills[j].skillnum == i)
      {
        ranger = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Ran(%d)", r_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; d_skills[j].skillnum; j++)
    {
      if (d_skills[j].skillnum == i)
      {
        bard = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Brd(%d)", d_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; u_skills[j].skillnum; j++)
    {
      if (u_skills[j].skillnum == i)
      {
        druid = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Dru(%d)", u_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    if (mage)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*m_skills[mage].skillname), m_skills[mage].skillname + 1, skill_cost.find(i)->second);
    }
    else if (cleric)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*c_skills[cleric].skillname), c_skills[cleric].skillname + 1, skill_cost.find(i)->second);
    }
    else if (thief)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*t_skills[thief].skillname), t_skills[thief].skillname + 1, skill_cost.find(i)->second);
    }
    else if (warrior)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*w_skills[warrior].skillname), w_skills[warrior].skillname + 1, skill_cost.find(i)->second);
    }
    else if (anti)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*a_skills[anti].skillname), a_skills[anti].skillname + 1, skill_cost.find(i)->second);
    }
    else if (pal)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*p_skills[pal].skillname), p_skills[pal].skillname + 1, skill_cost.find(i)->second);
    }
    else if (barb)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*b_skills[barb].skillname), b_skills[barb].skillname + 1, skill_cost.find(i)->second);
    }
    else if (monk)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*k_skills[monk].skillname), k_skills[monk].skillname + 1, skill_cost.find(i)->second);
    }
    else if (ranger)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*r_skills[ranger].skillname), r_skills[ranger].skillname + 1, skill_cost.find(i)->second);
    }
    else if (bard)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*d_skills[bard].skillname), d_skills[bard].skillname + 1, skill_cost.find(i)->second);
    }
    else if (druid)
    {
      sprintf(buf + strlen(buf), "$B$7Skill:$R %c%-20s  $B$7Moves:$R %-3d  $B$7Class:$R ",
              UPPER(*u_skills[druid].skillname), u_skills[druid].skillname + 1, skill_cost.find(i)->second);
    }
    else
      continue;

    strcat(buf, buf2);
    strcat(buf, "\n\r");
  }

  strcat(buf, "\n\r");
  page_string(ch->desc, buf, 1);

  return eSUCCESS;
}

int do_songs(Character *ch, char *arg, int cmd)
{
  char buf[16384];

  if (IS_NPC(ch))
    return eFAILURE;

  buf[0] = '\0';

  for (int i = 0; d_skills[i].skillnum; i++)
  {
    if (d_skills[i].skillnum >= SKILL_SONG_BASE && d_skills[i].skillnum <= SKILL_SONG_MAX)
    {
      sprintf(buf + strlen(buf), "$B$7Song:$R %c%-22s  $B$7Ki:$R %-3d  $B$7Class:$R %s (%d)",
              UPPER(*d_skills[i].skillname), d_skills[i].skillname + 1,
              song_info[d_skills[i].skillnum - SKILL_SONG_BASE].min_useski,
              "Brd", d_skills[i].levelavailable);
      strcat(buf, "\n\r");
    }
  }

  strcat(buf, "\n\r");
  page_string(ch->desc, buf, 1);

  return eSUCCESS;
}

int do_spells(Character *ch, char *arg, int cmd)
{
  char buf[16384];
  char buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
  int mage, cleric, anti, pal, ranger, druid;

  if (IS_NPC(ch))
    return eFAILURE;

  buf[0] = '\0';

  for (int i = 0; i <= MAX_SPL_LIST; i++)
  {
    mage = 0;
    cleric = 0;
    anti = 0;
    pal = 0;
    ranger = 0;
    druid = 0;
    buf2[0] = '\0';
    for (int j = 0; m_skills[j].skillnum; j++)
    {
      if (m_skills[j].skillnum == i)
      {
        mage = j;
        sprintf(buf2, "Mag(%d)", m_skills[j].levelavailable);
        break;
      }
    }
    for (int j = 0; c_skills[j].skillnum; j++)
    {
      if (c_skills[j].skillnum == i)
      {
        cleric = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Cle(%d)", c_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; a_skills[j].skillnum; j++)
    {
      if (a_skills[j].skillnum == i)
      {
        anti = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Ant(%d)", a_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; p_skills[j].skillnum; j++)
    {
      if (p_skills[j].skillnum == i)
      {
        pal = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Pal(%d)", p_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; r_skills[j].skillnum; j++)
    {
      if (r_skills[j].skillnum == i)
      {
        ranger = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Ran(%d)", r_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    for (int j = 0; u_skills[j].skillnum; j++)
    {
      if (u_skills[j].skillnum == i)
      {
        druid = j;
        if (buf2[0] != '\0')
          strcat(buf2, ", ");
        sprintf(buf3, "Dru(%d)", u_skills[j].levelavailable);
        strcat(buf2, buf3);
        break;
      }
    }
    if (mage)
    {
      sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
              UPPER(*m_skills[mage].skillname), m_skills[mage].skillname + 1, spell_info[mage].min_usesmana);
    }
    else if (cleric)
    {
      sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
              UPPER(*c_skills[cleric].skillname), c_skills[cleric].skillname + 1, spell_info[cleric].min_usesmana);
    }
    else if (anti)
    {
      sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
              UPPER(*a_skills[anti].skillname), a_skills[anti].skillname + 1, spell_info[anti].min_usesmana);
    }
    else if (pal)
    {
      sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
              UPPER(*p_skills[pal].skillname), p_skills[pal].skillname + 1, spell_info[pal].min_usesmana);
    }
    else if (ranger)
    {
      sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
              UPPER(*r_skills[ranger].skillname), r_skills[ranger].skillname + 1, spell_info[ranger].min_usesmana);
    }
    else if (druid)
    {
      sprintf(buf + strlen(buf), "$B$7Spell:$R %c%-20s  $B$7Mana:$R %-3d  $B$7Class:$R ",
              UPPER(*u_skills[druid].skillname), u_skills[druid].skillname + 1, spell_info[druid].min_usesmana);
    }
    else
      continue;

    strcat(buf, buf2);
    strcat(buf, "\n\r");
  }

  strcat(buf, "\n\r");
  page_string(ch->desc, buf, 1);

  return eSUCCESS;
}

int spl_lvl(int lev)
{
  if (lev >= MIN_GOD)
    return 0;
  return lev;
}

// search through a character's list to see if they have a particular skill
// if so, return their level of knowledge
// if not, return 0
int Character::has_skill(skill_t skill)
{
  class Object *o;
  int bonus = 0;

  if (isNPC() || !isPlayer())
    return 0;

  if (affected_by_spell(SKILL_DEFENDERS_STANCE) && skill == SKILL_DODGE)
    return affected_by_spell(SKILL_DEFENDERS_STANCE)->modifier;

  if (affected_by_spell(SPELL_VILLAINY) && (skill == SPELL_VILLAINY))
  {
    bonus = 0;
  }
  else if (affected_by_spell(SPELL_VILLAINY))
    bonus += affected_by_spell(SPELL_VILLAINY)->modifier / 5;
  if (affected_by_spell(SPELL_HEROISM) && (skill == SPELL_HEROISM))
  {
    bonus = 0;
  }
  else if (affected_by_spell(SPELL_HEROISM))
    bonus += affected_by_spell(SPELL_HEROISM)->modifier / 5;

  if (this->skills.contains(skill))
  {
    const auto &curr = this->skills[skill];

    for (o = this->player->skillchange; o; o = o->next_skill)
    {
      int a;
      for (a = 0; a < o->num_affects; a++)
      {
        if (o->affected[a].location == skill * 1000)
        {
          bonus += o->affected[a].modifier;
          if ((int)curr.learned + bonus > 150)
            bonus = 150 - curr.learned;
        }
      }
    }
    return ((int)curr.learned) + bonus;
  }

  return 0;
}

int has_skill(Character *ch, skill_t skill)
{
  return ch->has_skill(skill);
}
