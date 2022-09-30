/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *        The prototypes for functions defined in magic.c that can/are       *
 *        used else where in the mud. (Spells)                               *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id: magic.h,v 1.41 2015/06/14 02:38:12 pirahna Exp $ */
#ifndef MAGIC_H_
#define MAGIC_H_

#include "structs.h" // ubyte, etc..

#define GLOBE_OF_DARKNESS_OBJECT      101


typedef  int (*SPELL_POINTER) (ubyte, char_data*, char*, int, char_data*, struct obj_data *,int);


bool resist_spell(int perc);
bool resist_spell(char_data *ch, int skill);
int spellcraft(char_data *ch, int spell);

int spell_iridescent_aura(ubyte level, char_data *ch,
    char_data *victim, struct obj_data *obj, int skill);
int spell_resist_fire(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_resist_cold(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_acid_blast(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_acid_breath(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_animate_dead(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * corpse, int skill);
int spell_armor(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_aegis(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_portal(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_resist_magic(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);

int spell_bee_sting(ubyte level, char_data *ch,
   char_data *victim, struct obj_data *obj, int skill);
int spell_bee_swarm(ubyte level, char_data *ch,
   char_data *victim, struct obj_data *obj, int skill);
int spell_bless(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_blindness(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);



int spell_burning_hands(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_burning_hands(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int cast_oaken_fortitude(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_clarity(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_divine_intervention(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);


int spell_call_lightning(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_cause_critical(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_cause_light(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_cause_serious(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_charm_person(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_chill_touch(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_colour_spray(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_conjure_elemental(ubyte level, char_data * ch,
   char *arg, char_data * victim, struct obj_data * obj, int skill);
int spell_cont_light(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_create_food(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_create_water(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_remove_blind(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_cure_critic(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_cure_light(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_cure_serious(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_curse(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);

int spell_shadowslip(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_camouflague(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_farsight(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_resist_energy(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_staunchblood(ubyte level, char_data * ch,
   char_data * victim, struct obj_data *obj, int skill);
int spell_insomnia(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_freefloat(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_earthquake(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_energy_drain(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_drown(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_howl(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_souldrain(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_sparks(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_fireball(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_heal_spray(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_life_leech(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_lightning_bolt(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_magic_missile(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_meteor_swarm(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_shocking_grasp(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_vampiric_touch(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_firestorm(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_dispel_evil(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_dispel_good(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_harm(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_power_harm(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_divine_fury(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_teleport(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_paralyze(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_remove_paralysis(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_detect_evil(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_detect_good(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_true_sight(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_detect_invisibility(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_infravision(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_detect_magic(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_haste(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_detect_poison(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_enchant_weapon(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_eyes_of_the_owl(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_feline_agility(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_heal(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_power_heal(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_full_heal(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_invisibility(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_locate_object(ubyte level, char_data * ch, char *arg,
   char_data * victim, struct obj_data * obj, int skill);
int spell_oaken_fortitude(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_poison(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_protection_from_evil(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_protection_from_good(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_remove_curse(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_remove_poison(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_fireshield(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_sanctuary(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_sleep(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_strength(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_ventriloquate(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_word_of_recall(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_wizard_eye(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_eagle_eye(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_summon(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_sense_life(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_identify(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_frost_breath(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_fire_breath(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_gas_breath(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_lightning_breath(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_fear(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_refresh(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_fly(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_know_alignment(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_dispel_magic(ubyte level, char_data * ch, char_data * victim, struct obj_data * obj, int skill, int spell = 0);
int spell_flamestrike(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_stone_skin(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_shield(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_weaken(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_mass_invis(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_hellstream(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_group_sanc(ubyte level, char_data *ch, char_data *victim, 
	struct obj_data *obj, int skill);
int spell_ghost_walk(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_mend_golem(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);

int cast_ghost_walk(ubyte level, char_data *ch, char *arg, int type,
   char_data *victim, struct obj_data *tar_obj, int skill);
int cast_mend_golem(ubyte level, char_data *ch, char *arg, int type,
   char_data *victim, struct obj_data *tar_obj, int skill);
int cast_iridescent_aura(ubyte level, char_data *ch, char *arg, int type,
   char_data *victim, struct obj_data *tar_obj, int skill);
int cast_camouflague(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_farsight(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_resist_energy(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_staunchblood(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_freefloat(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_insomnia(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_shadowslip(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_call_lightning(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_chill_touch(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_shocking_grasp(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_colour_spray(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_earthquake(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_life_leech(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_firestorm(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_energy_drain(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_drown(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_howl(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_souldrain(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_sparks(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_vampiric_touch(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_meteor_swarm(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_fireball(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_harm(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_power_harm(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_divine_fury(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_lightning_bolt(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_magic_missile(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_armor(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_aegis(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_teleport(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_bless(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_paralyze(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_blindness(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_control_weather(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_create_food(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_create_water(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_remove_paralysis(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_remove_blind(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cure_critic(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cure_light(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_curse(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_detect_evil(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_true_sight(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_detect_good(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_detect_invisibility(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_detect_magic(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_haste(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_detect_poison(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_dispel_evil(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_dispel_good(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_enchant_weapon(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_heal(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_power_heal(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_full_heal(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_invisibility(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_locate_object(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_poison(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_protection_from_evil(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_protection_from_good(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_remove_curse(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_remove_poison(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_fireshield(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_sanctuary(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_sleep(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_strength(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_ventriloquate(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_word_of_recall(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_wizard_eye(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_eagle_eye(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_summon(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_charm_person(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_sense_life(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_identify(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_frost_breath(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_acid_breath(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_fire_breath(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_gas_breath(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_lightning_breath(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_fear(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_refresh(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_fly(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cont_light(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_know_alignment(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_dispel_magic(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_conjure_elemental(ubyte level, char_data * ch,
   char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cure_serious(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cause_light(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cause_critical(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_cause_serious(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_flamestrike(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_stone_skin(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_shield(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_weaken(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_mass_invis(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_acid_blast(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_hellstream(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_portal(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_infravision(ubyte level, char_data * ch, char *arg,
   int type, char_data * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_animate_dead(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_mana(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_solar_gate(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_heroes_feast(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_heal_spray(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_group_sanc(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_group_recall(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_group_fly(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_enchant_armor(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_resist_fire(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_resist_magic(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_resist_cold(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_bee_sting(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_bee_swarm(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_creeping_death(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_barkskin(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_herb_lore(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_call_follower(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_entangle(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_eyes_of_the_owl(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_feline_agility(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_forest_meld(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_companion(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int cast_create_golem(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);

int spell_dispel_minor(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_dispel_minor(ubyte level, char_data * ch, char *arg, int type,
   char_data * tar_ch, struct obj_data * tar_obj, int skill);

int spell_release_golem(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int spell_beacon(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);
int spell_reflect(ubyte level, char_data * ch, char *arg, int type,
	char_data * tar_ch, struct obj_data * tar_obj, int skill);

int spell_stone_shield(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_stone_shield(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_greater_stone_shield(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_summon_familiar(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_summon_familiar(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_lighted_path(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_lighted_path(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_resist_acid(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_resist_acid(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_sun_ray(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_sun_ray(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_rapid_mend(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_rapid_mend(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_iron_roots(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_iron_roots(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_acid_shield(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_acid_shield(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_water_breathing(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_water_breathing(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_globe_of_darkness(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_globe_of_darkness(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_barkskin(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_entangle(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int spell_eyes_of_the_eagle(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_eyes_of_the_eagle(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_icestorm(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_icestorm(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_lightning_shield(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_lightning_shield(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_blue_bird(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_blue_bird(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_debility(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_debility(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_attrition(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_attrition(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_vampiric_aura(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_vampiric_aura(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_holy_aura(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_holy_aura(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);


SPELL_POINTER get_wild_magic_defensive(ubyte level, char_data *ch, char_data *victim, OBJ_DATA *obj, int skill);
SPELL_POINTER get_wild_magic_offensive(ubyte level, char_data *ch, char_data *victim, OBJ_DATA *obj, int skill);

int spell_wild_magic(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_wild_magic(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);


int spell_stability(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_stability(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_solidity(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_solidity(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_frostshield(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_frostshield(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_release_elemental(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_release_elemental(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);


int spell_dismiss_familiar(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_dismiss_familiar(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_dismiss_corpse(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_dismiss_corpse(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_visage_of_hate(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_visage_of_hate(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_blessed_halo(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_blessed_halo(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_mana(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);

int cast_wrath_of_god(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_atonement(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_silence(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_immunity(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_boneshield(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_channel(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int cast_spirit_shield(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_villainy(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_villainy(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_heroism(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_heroism(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_consecrate(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_consecrate(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);
int spell_desecrate(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_desecrate(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_elemental_wall(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_elemental_wall(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

int spell_ethereal_focus(ubyte level, char_data * ch,
   char_data * victim, struct obj_data * obj, int skill);
int cast_ethereal_focus(ubyte level, char_data * ch, char *arg, int type,
   char_data * victim, struct obj_data * tar_obj, int skill);

#endif
