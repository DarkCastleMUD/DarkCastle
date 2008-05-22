/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *        The prototypes for functions defined in magic.c that can/are       *
 *        used else where in the mud. (Spells)                               *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id: magic.h,v 1.34 2008/05/22 02:37:42 kkoons Exp $ */
#ifndef MAGIC_H_
#define MAGIC_H_

#include <structs.h> // ubyte, etc..

#define GLOBE_OF_DARKNESS_OBJECT      101


typedef  int (*SPELL_POINTER) (ubyte, CHAR_DATA*, char*, int, CHAR_DATA*, struct obj_data *,int);


bool resist_spell(int perc);
bool resist_spell(CHAR_DATA *ch, int skill);
int spellcraft(CHAR_DATA *ch, int spell);

int spell_iridescent_aura(ubyte level, CHAR_DATA *ch,
    CHAR_DATA *victim, struct obj_data *obj, int skill);
int spell_resist_fire(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_resist_cold(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_acid_blast(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_acid_breath(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_animate_dead(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * corpse, int skill);
int spell_armor(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_aegis(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_portal(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_resist_magic(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);

int spell_bee_sting(ubyte level, CHAR_DATA *ch,
   CHAR_DATA *victim, struct obj_data *obj, int skill);
int spell_bee_swarm(ubyte level, CHAR_DATA *ch,
   CHAR_DATA *victim, struct obj_data *obj, int skill);
int spell_bless(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_blindness(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);



int spell_burning_hands(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_burning_hands(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int cast_oaken_fortitude(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_clarity(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_divine_intervention(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);


int spell_call_lightning(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_cause_critical(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_cause_light(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_cause_serious(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_charm_person(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_chill_touch(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_colour_spray(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_conjure_elemental(ubyte level, CHAR_DATA * ch,
   char *arg, CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_cont_light(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_create_food(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_create_water(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_remove_blind(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_cure_critic(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_cure_light(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_cure_serious(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_curse(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);

int spell_shadowslip(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_camouflague(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_farsight(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_resist_energy(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_staunchblood(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data *obj, int skill);
int spell_insomnia(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_freefloat(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_earthquake(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_energy_drain(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_drown(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_howl(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_souldrain(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_sparks(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_fireball(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_heal_spray(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_life_leech(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_lightning_bolt(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_magic_missile(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_meteor_swarm(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_shocking_grasp(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_vampiric_touch(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_firestorm(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_dispel_evil(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_dispel_good(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_harm(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_power_harm(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_divine_fury(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_teleport(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_paralyze(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_remove_paralysis(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_detect_evil(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_detect_good(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_true_sight(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_detect_invisibility(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_infravision(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_detect_magic(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_haste(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_detect_poison(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_enchant_weapon(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_eyes_of_the_owl(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_feline_agility(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_heal(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_power_heal(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_full_heal(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_invisibility(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_locate_object(ubyte level, CHAR_DATA * ch, char *arg,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_oaken_fortitude(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_poison(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_protection_from_evil(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_protection_from_good(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_remove_curse(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_remove_poison(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_fireshield(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_sanctuary(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_sleep(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_strength(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_ventriloquate(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_word_of_recall(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_wizard_eye(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_eagle_eye(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_summon(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_sense_life(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_identify(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_frost_breath(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_fire_breath(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_gas_breath(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_lightning_breath(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_fear(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_refresh(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_fly(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_know_alignment(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_dispel_magic(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_flamestrike(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_stone_skin(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_shield(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_weaken(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_mass_invis(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_hellstream(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_group_sanc(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, 
	struct obj_data *obj, int skill);
int spell_ghost_walk(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_mend_golem(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);

int cast_ghost_walk(ubyte level, CHAR_DATA *ch, char *arg, int type,
   CHAR_DATA *victim, struct obj_data *tar_obj, int skill);
int cast_mend_golem(ubyte level, CHAR_DATA *ch, char *arg, int type,
   CHAR_DATA *victim, struct obj_data *tar_obj, int skill);
int cast_iridescent_aura(ubyte level, CHAR_DATA *ch, char *arg, int type,
   CHAR_DATA *victim, struct obj_data *tar_obj, int skill);
int cast_camouflague(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_farsight(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_resist_energy(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_staunchblood(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_freefloat(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_insomnia(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_shadowslip(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_call_lightning(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_chill_touch(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_shocking_grasp(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_colour_spray(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_earthquake(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_life_leech(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_firestorm(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_energy_drain(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_drown(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_howl(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_souldrain(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_sparks(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_vampiric_touch(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_meteor_swarm(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_fireball(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_harm(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_power_harm(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_divine_fury(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_lightning_bolt(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_magic_missile(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_armor(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_aegis(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_teleport(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_bless(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_paralyze(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_blindness(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_control_weather(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_create_food(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_create_water(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_remove_paralysis(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_remove_blind(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cure_critic(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cure_light(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_curse(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_detect_evil(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_true_sight(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_detect_good(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_detect_invisibility(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_detect_magic(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_haste(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_detect_poison(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_dispel_evil(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_dispel_good(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_enchant_weapon(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_heal(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_power_heal(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_full_heal(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_invisibility(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_locate_object(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_poison(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_protection_from_evil(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_protection_from_good(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_remove_curse(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_remove_poison(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_fireshield(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_sanctuary(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_sleep(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_strength(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_ventriloquate(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_word_of_recall(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_wizard_eye(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_eagle_eye(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_summon(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_charm_person(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_sense_life(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_identify(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_frost_breath(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_acid_breath(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_fire_breath(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_gas_breath(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_lightning_breath(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_fear(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_refresh(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_fly(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cont_light(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_know_alignment(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_dispel_magic(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_conjure_elemental(ubyte level, CHAR_DATA * ch,
   char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cure_serious(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cause_light(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_cause_critical(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_cause_serious(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_flamestrike(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_stone_skin(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_shield(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_weaken(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_mass_invis(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_acid_blast(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_hellstream(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_portal(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_infravision(ubyte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj, int skill);
int cast_animate_dead(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_mana(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_solar_gate(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_heroes_feast(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_heal_spray(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_group_sanc(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_group_recall(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_group_fly(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_enchant_armor(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_resist_fire(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_resist_magic(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_resist_cold(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_bee_sting(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_bee_swarm(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_creeping_death(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_barkskin(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_herb_lore(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_call_follower(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_entangle(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_eyes_of_the_owl(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_feline_agility(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_forest_meld(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_companion(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int cast_create_golem(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);

int spell_dispel_minor(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_dispel_minor(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);

int spell_release_golem(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int spell_beacon(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);
int spell_reflect(ubyte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj, int skill);

int spell_stone_shield(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_stone_shield(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_greater_stone_shield(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_summon_familiar(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_summon_familiar(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_lighted_path(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_lighted_path(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_resist_acid(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_resist_acid(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_sun_ray(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_sun_ray(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_rapid_mend(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_rapid_mend(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_iron_roots(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_iron_roots(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_acid_shield(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_acid_shield(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_water_breathing(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_water_breathing(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_globe_of_darkness(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_globe_of_darkness(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_barkskin(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_entangle(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int spell_eyes_of_the_eagle(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_eyes_of_the_eagle(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_ice_shards(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_ice_shards(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_lightning_shield(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_lightning_shield(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_blue_bird(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_blue_bird(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_debility(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_debility(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_attrition(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_attrition(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_vampiric_aura(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_vampiric_aura(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_holy_aura(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_holy_aura(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);


SPELL_POINTER get_wild_magic_defensive(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, int skill);
SPELL_POINTER get_wild_magic_offensive(ubyte level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, int skill);

int spell_wild_magic(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_wild_magic(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);


int spell_stability(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_stability(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_solidity(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_solidity(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_frostshield(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_frostshield(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_release_elemental(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_release_elemental(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);


int spell_dismiss_familiar(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_dismiss_familiar(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_dismiss_corpse(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_dismiss_corpse(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_visage_of_hate(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_visage_of_hate(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_blessed_halo(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);
int cast_blessed_halo(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);

int spell_mana(ubyte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj, int skill);

int cast_wrath_of_god(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_atonement(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_silence(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_immunity(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_boneshield(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
int cast_channel(ubyte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj, int skill);
#endif
