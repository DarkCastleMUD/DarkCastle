/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *        The prototypes for functions defined in magic.c that can/are       *
 *        used else where in the mud. (Spells)                               *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id: magic.h,v 1.3 2002/07/23 20:34:37 pirahna Exp $ */
#ifndef MAGIC_H_
#define MAGIC_H_

#include <structs.h> // byte, etc..

#define GLOBE_OF_DARKNESS_OBJECT      101

int spell_resist_fire(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_resist_cold(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_acid_blast(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_acid_breath(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_animate_dead(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * corpse);
int spell_armor(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_portal(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);

int spell_bee_sting(byte level, CHAR_DATA *ch,
   CHAR_DATA *victim, struct obj_data *obj);
int spell_bless(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_blindness(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_burning_hands(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_burning_hands(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_call_lightning(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_cause_critical(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_cause_light(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_cause_serious(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_charm_person(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_chill_touch(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_colour_spray(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_conjure_elemental(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_cont_light(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_create_food(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_create_water(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_cure_blind(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_cure_critic(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_cure_light(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_cure_serious(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_curse(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);

int spell_shadowslip(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_camouflague(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_farsight(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_resist_energy(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_staunchblood(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data *obj);
int spell_insomnia(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_freefloat(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_earthquake(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_energy_drain(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_drown(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_howl(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_souldrain(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_sparks(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_fireball(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_heal_spray(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_life_leech(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_lightning_bolt(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_magic_missile(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_meteor_swarm(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_shocking_grasp(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_vampiric_touch(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_firestorm(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_dispel_evil(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_dispel_good(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_harm(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_power_harm(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_teleport(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_paralyze(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_remove_paralysis(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_detect_evil(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_detect_good(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_true_sight(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_detect_invisibility(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_infravision(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_detect_magic(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_haste(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_detect_poison(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_enchant_weapon(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_heal(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_power_heal(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_full_heal(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_invisibility(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_locate_object(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_poison(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_protection_from_evil(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_remove_curse(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_remove_poison(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_fireshield(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_sanctuary(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_sleep(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_strength(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_ventriloquate(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_word_of_recall(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_wizard_eye(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_summon(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_sense_life(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_identify(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_frost_breath(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_fire_breath(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_gas_breath(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_lightning_breath(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_fear(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_refresh(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_fly(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_know_alignment(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_dispel_magic(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_flamestrike(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_stone_skin(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_shield(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_weaken(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_mass_invis(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int spell_hellstream(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);

int cast_camouflague(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_farsight(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_resist_energy(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_staunchblood(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_freefloat(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_insomnia(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_shadowslip(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_call_lightning(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_chill_touch(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_shocking_grasp(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_colour_spray(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_earthquake(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_life_leech(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_firestorm(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_energy_drain(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_drown(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_howl(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_souldrain(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_sparks(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_vampiric_touch(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_meteor_swarm(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_fireball(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_harm(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_power_harm(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_lightning_bolt(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_magic_missile(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_armor(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_teleport(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_bless(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_paralyze(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_blindness(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_control_weather(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_create_food(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_create_water(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_remove_paralysis(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_cure_blind(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_cure_critic(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_cure_light(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_curse(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_detect_evil(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_true_sight(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_detect_good(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_detect_invisibility(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_detect_magic(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_haste(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_detect_poison(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_dispel_evil(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_dispel_good(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_enchant_weapon(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_heal(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_power_heal(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_full_heal(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_invisibility(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_locate_object(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_poison(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_protection_from_evil(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_remove_curse(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_remove_poison(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_fireshield(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_sanctuary(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_sleep(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_strength(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_ventriloquate(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_word_of_recall(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_wizard_eye(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_summon(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_charm_person(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_sense_life(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_identify(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_frost_breath(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_acid_breath(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_fire_breath(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_gas_breath(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_lightning_breath(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_fear(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_refresh(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_fly(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_cont_light(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_know_alignment(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_dispel_magic(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_conjure_elemental(byte level, CHAR_DATA * ch,
   char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_cure_serious(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_cause_light(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_cause_critical(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);
int cast_cause_serious(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);
int cast_flamestrike(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);
int cast_stone_skin(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);
int cast_shield(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);;
int cast_weaken(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);
int cast_mass_invis(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);
int cast_acid_blast(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);
int cast_hellstream(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);
int cast_portal(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);
int cast_infravision(byte level, CHAR_DATA * ch, char *arg,
   int type, CHAR_DATA * tar_ch,
   struct obj_data * tar_obj);
int cast_animate_dead(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_mana(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_solar_gate(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_heroes_feast(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_heal_spray(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_group_sanc(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_group_recall(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_group_fly(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_enchant_armor(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_resist_fire(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_resist_cold(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_bee_sting(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_bee_swarm(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_creeping_death(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_barkskin(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_herb_lore(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_call_follower(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_entangle(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_eyes_of_the_owl(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_feline_agility(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_forest_meld(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_companion(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int cast_create_golem(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int spell_dispel_minor(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_dispel_minor(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int spell_release_golem(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int spell_beacon(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);
int spell_reflect(byte level, CHAR_DATA * ch, char *arg, int type,
	CHAR_DATA * tar_ch, struct obj_data * tar_obj);

int spell_stone_shield(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_stone_shield(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);
int cast_greater_stone_shield(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_summon_familiar(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_summon_familiar(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_lighted_path(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_lighted_path(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_resist_acid(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_resist_acid(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_sun_ray(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_sun_ray(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_rapid_mend(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_rapid_mend(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_iron_roots(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_iron_roots(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_acid_shield(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_acid_shield(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_water_breathing(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_water_breathing(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_globe_of_darkness(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_globe_of_darkness(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_eyes_of_the_eagle(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_eyes_of_the_eagle(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_haste_other(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_haste_other(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_ice_shards(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_ice_shards(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);

int spell_lightning_shield(byte level, CHAR_DATA * ch,
   CHAR_DATA * victim, struct obj_data * obj);
int cast_lightning_shield(byte level, CHAR_DATA * ch, char *arg, int type,
   CHAR_DATA * victim, struct obj_data * tar_obj);


#endif
