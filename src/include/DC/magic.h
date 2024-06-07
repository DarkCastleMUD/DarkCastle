/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *        The prototypes for functions defined in magic.c that can/are       *
 *        used else where in the mud. (Spells)                               *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id: magic.h,v 1.41 2015/06/14 02:38:12 pirahna Exp $ */
#ifndef MAGIC_H_
#define MAGIC_H_

#include "DC/structs.h" // uint8_t, etc..

#define GLOBE_OF_DARKNESS_OBJECT 101

typedef int (*SPELL_POINTER)(uint8_t, Character *, char *, int, Character *, class Object *, int);

bool resist_spell(int perc);
bool resist_spell(Character *ch, int skill);
int spellcraft(Character *ch, int spell);

int spell_iridescent_aura(uint8_t level, Character *ch,
                          Character *victim, class Object *obj, int skill);
int spell_resist_fire(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_resist_cold(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_acid_blast(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_acid_breath(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_animate_dead(uint8_t level, Character *ch,
                       Character *victim, class Object *corpse, int skill);
int spell_armor(uint8_t level, Character *ch,
                Character *victim, class Object *obj, int skill);
int spell_aegis(uint8_t level, Character *ch,
                Character *victim, class Object *obj, int skill);
int spell_portal(uint8_t level, Character *ch,
                 Character *victim, class Object *obj, int skill);
int spell_resist_magic(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);

int spell_bee_sting(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int spell_bee_swarm(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int spell_bless(uint8_t level, Character *ch,
                Character *victim, class Object *obj, int skill);
int spell_blindness(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);

int spell_burning_hands(uint8_t level, Character *ch,
                        Character *victim, class Object *obj, int skill);
int cast_burning_hands(uint8_t level, Character *ch, char *arg, int type,
                       Character *victim, class Object *tar_obj, int skill);

int cast_oaken_fortitude(uint8_t level, Character *ch, char *arg, int type,
                         Character *victim, class Object *tar_obj, int skill);
int cast_clarity(uint8_t level, Character *ch, char *arg, int type,
                 Character *victim, class Object *tar_obj, int skill);
int cast_divine_intervention(uint8_t level, Character *ch, char *arg, int type,
                             Character *victim, class Object *tar_obj, int skill);

int spell_call_lightning(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int spell_cause_critical(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int spell_cause_light(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_cause_serious(uint8_t level, Character *ch,
                        Character *victim, class Object *obj, int skill);
int spell_charm_person(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_chill_touch(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_colour_spray(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_conjure_elemental(uint8_t level, Character *ch,
                            char *arg, Character *victim, class Object *obj, int skill);
int spell_cont_light(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_create_food(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_create_water(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_remove_blind(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_cure_critic(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_cure_light(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_cure_serious(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_curse(uint8_t level, Character *ch,
                Character *victim, class Object *obj, int skill);

int spell_shadowslip(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_camouflague(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_farsight(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int spell_resist_energy(uint8_t level, Character *ch,
                        Character *victim, class Object *obj, int skill);
int spell_staunchblood(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_insomnia(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int spell_freefloat(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int spell_earthquake(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_energy_drain(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_drown(uint8_t level, Character *ch,
                Character *victim, class Object *obj, int skill);
int spell_howl(uint8_t level, Character *ch,
               Character *victim, class Object *obj, int skill);
int spell_souldrain(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int spell_sparks(uint8_t level, Character *ch,
                 Character *victim, class Object *obj, int skill);
int spell_fireball(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int spell_heal_spray(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_life_leech(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_lightning_bolt(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int spell_magic_missile(uint8_t level, Character *ch,
                        Character *victim, class Object *obj, int skill);
int spell_meteor_swarm(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_shocking_grasp(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int spell_vampiric_touch(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int spell_firestorm(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int spell_dispel_evil(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_dispel_good(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_harm(uint8_t level, Character *ch,
               Character *victim, class Object *obj, int skill);
int spell_power_harm(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_divine_fury(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_teleport(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int spell_paralyze(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int spell_remove_paralysis(uint8_t level, Character *ch,
                           Character *victim, class Object *obj, int skill);
int spell_detect_evil(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_detect_good(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_true_sight(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_detect_invisibility(uint8_t level, Character *ch,
                              Character *victim, class Object *obj, int skill);
int spell_infravision(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_detect_magic(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_haste(uint8_t level, Character *ch,
                Character *victim, class Object *obj, int skill);
int spell_detect_poison(uint8_t level, Character *ch,
                        Character *victim, class Object *obj, int skill);
int spell_enchant_weapon(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int spell_eyes_of_the_owl(uint8_t level, Character *ch,
                          Character *victim, class Object *obj, int skill);
int spell_feline_agility(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int spell_heal(uint8_t level, Character *ch,
               Character *victim, class Object *obj, int skill);
int spell_power_heal(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_full_heal(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int spell_invisibility(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_locate_object(uint8_t level, Character *ch, char *arg,
                        Character *victim, class Object *obj, int skill);
int spell_oaken_fortitude(uint8_t level, Character *ch,
                          Character *victim, class Object *obj, int skill);
int spell_poison(uint8_t level, Character *ch,
                 Character *victim, class Object *obj, int skill);
int spell_protection_from_evil(uint8_t level, Character *ch,
                               Character *victim, class Object *obj, int skill);
int spell_protection_from_good(uint8_t level, Character *ch,
                               Character *victim, class Object *obj, int skill);
int spell_remove_curse(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_remove_poison(uint8_t level, Character *ch,
                        Character *victim, class Object *obj, int skill);
int spell_fireshield(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_sanctuary(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int spell_sleep(uint8_t level, Character *ch,
                Character *victim, class Object *obj, int skill);
int spell_strength(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int spell_ventriloquate(uint8_t level, Character *ch,
                        Character *victim, class Object *obj, int skill);
int spell_word_of_recall(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int spell_wizard_eye(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_eagle_eye(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int spell_summon(uint8_t level, Character *ch,
                 Character *victim, class Object *obj, int skill);
int spell_sense_life(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_identify(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int spell_frost_breath(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int spell_fire_breath(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_gas_breath(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_lightning_breath(uint8_t level, Character *ch,
                           Character *victim, class Object *obj, int skill);
int spell_fear(uint8_t level, Character *ch,
               Character *victim, class Object *obj, int skill);
int spell_refresh(uint8_t level, Character *ch,
                  Character *victim, class Object *obj, int skill);
int spell_fly(uint8_t level, Character *ch,
              Character *victim, class Object *obj, int skill);
int spell_know_alignment(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int spell_dispel_magic(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill, int spell = 0);
int spell_flamestrike(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int spell_stone_skin(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_shield(uint8_t level, Character *ch,
                 Character *victim, class Object *obj, int skill);
int spell_weaken(uint8_t level, Character *ch,
                 Character *victim, class Object *obj, int skill);
int spell_mass_invis(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_hellstream(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_group_sanc(uint8_t level, Character *ch, Character *victim,
                     class Object *obj, int skill);
int spell_ghost_walk(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int spell_mend_golem(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);

int cast_ghost_walk(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);
int cast_mend_golem(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);
int cast_iridescent_aura(uint8_t level, Character *ch, char *arg, int type,
                         Character *victim, class Object *tar_obj, int skill);
int cast_camouflague(uint8_t level, Character *ch, char *arg, int type,
                     Character *victim, class Object *tar_obj, int skill);
int cast_farsight(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill);
int cast_resist_energy(uint8_t level, Character *ch, char *arg, int type,
                       Character *victim, class Object *tar_obj, int skill);
int cast_staunchblood(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill);
int cast_freefloat(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill);
int cast_insomnia(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill);
int cast_shadowslip(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);
int cast_call_lightning(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill);
int cast_chill_touch(uint8_t level, Character *ch, char *arg, int type,
                     Character *victim, class Object *tar_obj, int skill);
int cast_shocking_grasp(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill);
int cast_colour_spray(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill);
int cast_earthquake(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);
int cast_life_leech(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);
int cast_firestorm(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill);
int cast_energy_drain(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill);
int cast_drown(uint8_t level, Character *ch, char *arg, int type,
               Character *victim, class Object *tar_obj, int skill);
int cast_howl(uint8_t level, Character *ch, char *arg, int type,
              Character *victim, class Object *tar_obj, int skill);
int cast_souldrain(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill);
int cast_sparks(uint8_t level, Character *ch, char *arg, int type,
                Character *victim, class Object *tar_obj, int skill);
int cast_vampiric_touch(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill);
int cast_meteor_swarm(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill);
int cast_fireball(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill);
int cast_harm(uint8_t level, Character *ch, char *arg, int type,
              Character *victim, class Object *tar_obj, int skill);
int cast_power_harm(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);
int cast_divine_fury(uint8_t level, Character *ch, char *arg, int type,
                     Character *victim, class Object *tar_obj, int skill);
int cast_lightning_bolt(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill);
int cast_magic_missile(uint8_t level, Character *ch, char *arg, int type,
                       Character *victim, class Object *tar_obj, int skill);
int cast_armor(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill);
int cast_aegis(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill);
int cast_teleport(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill);
int cast_bless(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill);
int cast_paralyze(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill);
int cast_blindness(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill);
int cast_control_weather(uint8_t level, Character *ch, char *arg, int type,
                         Character *tar_ch, class Object *tar_obj, int skill);
int cast_create_food(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_create_water(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_remove_paralysis(uint8_t level, Character *ch, char *arg, int type,
                          Character *tar_ch, class Object *tar_obj, int skill);
int cast_remove_blind(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_cure_critic(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_cure_light(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_curse(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill);
int cast_detect_evil(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_true_sight(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_detect_good(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_detect_invisibility(uint8_t level, Character *ch, char *arg, int type,
                             Character *tar_ch, class Object *tar_obj, int skill);
int cast_detect_magic(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_haste(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill);
int cast_detect_poison(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill);
int cast_dispel_evil(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_dispel_good(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_enchant_weapon(uint8_t level, Character *ch, char *arg, int type,
                        Character *tar_ch, class Object *tar_obj, int skill);
int cast_heal(uint8_t level, Character *ch, char *arg, int type,
              Character *tar_ch, class Object *tar_obj, int skill);
int cast_power_heal(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_full_heal(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill);
int cast_invisibility(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_locate_object(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill);
int cast_poison(uint8_t level, Character *ch, char *arg, int type,
                Character *tar_ch, class Object *tar_obj, int skill);
int cast_protection_from_evil(uint8_t level, Character *ch, char *arg, int type,
                              Character *tar_ch, class Object *tar_obj, int skill);
int cast_protection_from_good(uint8_t level, Character *ch, char *arg, int type,
                              Character *tar_ch, class Object *tar_obj, int skill);
int cast_remove_curse(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_remove_poison(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill);
int cast_fireshield(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_sanctuary(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill);
int cast_sleep(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill);
int cast_strength(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill);
int cast_ventriloquate(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill);
int cast_word_of_recall(uint8_t level, Character *ch, char *arg, int type,
                        Character *tar_ch, class Object *tar_obj, int skill);
int cast_wizard_eye(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_eagle_eye(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill);
int cast_summon(uint8_t level, Character *ch, char *arg, int type,
                Character *tar_ch, class Object *tar_obj, int skill);
int cast_charm_person(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_sense_life(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_identify(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill);
int cast_frost_breath(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_acid_breath(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_fire_breath(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_gas_breath(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_lightning_breath(uint8_t level, Character *ch, char *arg, int type,
                          Character *tar_ch, class Object *tar_obj, int skill);
int cast_fear(uint8_t level, Character *ch, char *arg, int type,
              Character *tar_ch, class Object *tar_obj, int skill);
int cast_refresh(uint8_t level, Character *ch, char *arg, int type,
                 Character *tar_ch, class Object *tar_obj, int skill);
int cast_fly(uint8_t level, Character *ch, char *arg, int type,
             Character *tar_ch, class Object *tar_obj, int skill);
int cast_cont_light(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_know_alignment(uint8_t level, Character *ch, char *arg, int type,
                        Character *tar_ch, class Object *tar_obj, int skill);
int cast_dispel_magic(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_conjure_elemental(uint8_t level, Character *ch,
                           char *arg, int type,
                           Character *tar_ch, class Object *tar_obj, int skill);
int cast_cure_serious(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_cause_light(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_cause_critical(uint8_t level, Character *ch, char *arg,
                        int type, Character *tar_ch,
                        class Object *tar_obj, int skill);
int cast_cause_serious(uint8_t level, Character *ch, char *arg,
                       int type, Character *tar_ch,
                       class Object *tar_obj, int skill);
int cast_flamestrike(uint8_t level, Character *ch, char *arg,
                     int type, Character *tar_ch,
                     class Object *tar_obj, int skill);
int cast_stone_skin(uint8_t level, Character *ch, char *arg,
                    int type, Character *tar_ch,
                    class Object *tar_obj, int skill);
int cast_shield(uint8_t level, Character *ch, char *arg,
                int type, Character *tar_ch,
                class Object *tar_obj, int skill);
int cast_weaken(uint8_t level, Character *ch, char *arg,
                int type, Character *tar_ch,
                class Object *tar_obj, int skill);
int cast_mass_invis(uint8_t level, Character *ch, char *arg,
                    int type, Character *tar_ch,
                    class Object *tar_obj, int skill);
int cast_acid_blast(uint8_t level, Character *ch, char *arg,
                    int type, Character *tar_ch,
                    class Object *tar_obj, int skill);
int cast_hellstream(uint8_t level, Character *ch, char *arg,
                    int type, Character *tar_ch,
                    class Object *tar_obj, int skill);
int cast_portal(uint8_t level, Character *ch, char *arg,
                int type, Character *tar_ch,
                class Object *tar_obj, int skill);
int cast_infravision(uint8_t level, Character *ch, char *arg,
                     int type, Character *tar_ch,
                     class Object *tar_obj, int skill);
int cast_animate_dead(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_mana(uint8_t level, Character *ch, char *arg, int type,
              Character *tar_ch, class Object *tar_obj, int skill);
int cast_solar_gate(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_heroes_feast(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_heal_spray(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_group_sanc(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill);
int cast_group_recall(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_group_fly(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill);
int cast_enchant_armor(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill);
int cast_resist_fire(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_resist_magic(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);
int cast_resist_cold(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_bee_sting(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill);
int cast_bee_swarm(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill);
int cast_creeping_death(uint8_t level, Character *ch, char *arg, int type,
                        Character *tar_ch, class Object *tar_obj, int skill);
int cast_barkskin(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill);
int cast_herb_lore(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill);
int cast_call_follower(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill);
int cast_entangle(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill);
int cast_eyes_of_the_owl(uint8_t level, Character *ch, char *arg, int type,
                         Character *tar_ch, class Object *tar_obj, int skill);
int cast_feline_agility(uint8_t level, Character *ch, char *arg, int type,
                        Character *tar_ch, class Object *tar_obj, int skill);
int cast_forest_meld(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill);
int cast_companion(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill);
int cast_create_golem(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);

int spell_dispel_minor(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int cast_dispel_minor(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill);

int spell_release_golem(uint8_t level, Character *ch, char *arg, int type,
                        Character *tar_ch, class Object *tar_obj, int skill);
int spell_beacon(uint8_t level, Character *ch, char *arg, int type,
                 Character *tar_ch, class Object *tar_obj, int skill);
int spell_reflect(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill);

int spell_stone_shield(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int cast_stone_shield(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill);
int cast_greater_stone_shield(uint8_t level, Character *ch, char *arg, int type,
                              Character *victim, class Object *tar_obj, int skill);

int spell_summon_familiar(uint8_t level, Character *ch,
                          Character *victim, class Object *obj, int skill);
int cast_summon_familiar(uint8_t level, Character *ch, char *arg, int type,
                         Character *victim, class Object *tar_obj, int skill);

int spell_lighted_path(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int cast_lighted_path(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill);

int spell_resist_acid(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int cast_resist_acid(uint8_t level, Character *ch, char *arg, int type,
                     Character *victim, class Object *tar_obj, int skill);

int spell_sun_ray(uint8_t level, Character *ch,
                  Character *victim, class Object *obj, int skill);
int cast_sun_ray(uint8_t level, Character *ch, char *arg, int type,
                 Character *victim, class Object *tar_obj, int skill);

int spell_rapid_mend(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int cast_rapid_mend(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);

int spell_iron_roots(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int cast_iron_roots(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);

int spell_acid_shield(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int cast_acid_shield(uint8_t level, Character *ch, char *arg, int type,
                     Character *victim, class Object *tar_obj, int skill);

int spell_water_breathing(uint8_t level, Character *ch,
                          Character *victim, class Object *obj, int skill);
int cast_water_breathing(uint8_t level, Character *ch, char *arg, int type,
                         Character *victim, class Object *tar_obj, int skill);

int spell_globe_of_darkness(uint8_t level, Character *ch,
                            Character *victim, class Object *obj, int skill);
int cast_globe_of_darkness(uint8_t level, Character *ch, char *arg, int type,
                           Character *victim, class Object *tar_obj, int skill);

int spell_barkskin(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int spell_entangle(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int spell_eyes_of_the_eagle(uint8_t level, Character *ch,
                            Character *victim, class Object *obj, int skill);
int cast_eyes_of_the_eagle(uint8_t level, Character *ch, char *arg, int type,
                           Character *victim, class Object *tar_obj, int skill);

int spell_icestorm(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int cast_icestorm(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill);

int spell_lightning_shield(uint8_t level, Character *ch,
                           Character *victim, class Object *obj, int skill);
int cast_lightning_shield(uint8_t level, Character *ch, char *arg, int type,
                          Character *victim, class Object *tar_obj, int skill);

int spell_blue_bird(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int cast_blue_bird(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill);

int spell_debility(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int cast_debility(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill);

int spell_attrition(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int cast_attrition(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill);

int spell_vampiric_aura(uint8_t level, Character *ch,
                        Character *victim, class Object *obj, int skill);
int cast_vampiric_aura(uint8_t level, Character *ch, char *arg, int type,
                       Character *victim, class Object *tar_obj, int skill);

int spell_holy_aura(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int cast_holy_aura(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill);

SPELL_POINTER get_wild_magic_defensive(uint8_t level, Character *ch, Character *victim, Object *obj, int skill);
SPELL_POINTER get_wild_magic_offensive(uint8_t level, Character *ch, Character *victim, Object *obj, int skill);

int spell_wild_magic(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int cast_wild_magic(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);

int spell_stability(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int cast_stability(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill);

int spell_solidity(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int cast_solidity(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill);

int spell_frostshield(uint8_t level, Character *ch,
                      Character *victim, class Object *obj, int skill);
int cast_frostshield(uint8_t level, Character *ch, char *arg, int type,
                     Character *victim, class Object *tar_obj, int skill);

int spell_release_elemental(uint8_t level, Character *ch,
                            Character *victim, class Object *obj, int skill);
int cast_release_elemental(uint8_t level, Character *ch, char *arg, int type,
                           Character *victim, class Object *tar_obj, int skill);

int spell_dismiss_familiar(uint8_t level, Character *ch,
                           Character *victim, class Object *obj, int skill);
int cast_dismiss_familiar(uint8_t level, Character *ch, char *arg, int type,
                          Character *victim, class Object *tar_obj, int skill);

int spell_dismiss_corpse(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int cast_dismiss_corpse(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill);

int spell_visage_of_hate(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int cast_visage_of_hate(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill);

int spell_blessed_halo(uint8_t level, Character *ch,
                       Character *victim, class Object *obj, int skill);
int cast_blessed_halo(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill);

int spell_mana(uint8_t level, Character *ch,
               Character *victim, class Object *obj, int skill);

int cast_wrath_of_god(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill);
int cast_atonement(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill);
int cast_silence(uint8_t level, Character *ch, char *arg, int type,
                 Character *victim, class Object *tar_obj, int skill);
int cast_immunity(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill);
int cast_boneshield(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);
int cast_channel(uint8_t level, Character *ch, char *arg, int type,
                 Character *victim, class Object *tar_obj, int skill);
int cast_spirit_shield(uint8_t level, Character *ch, char *arg, int type,
                       Character *victim, class Object *tar_obj, int skill);

int spell_villainy(uint8_t level, Character *ch,
                   Character *victim, class Object *obj, int skill);
int cast_villainy(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill);

int spell_heroism(uint8_t level, Character *ch,
                  Character *victim, class Object *obj, int skill);
int cast_heroism(uint8_t level, Character *ch, char *arg, int type,
                 Character *victim, class Object *tar_obj, int skill);

int spell_consecrate(uint8_t level, Character *ch,
                     Character *victim, class Object *obj, int skill);
int cast_consecrate(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill);
int spell_desecrate(uint8_t level, Character *ch,
                    Character *victim, class Object *obj, int skill);
int cast_desecrate(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill);

int spell_elemental_wall(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int cast_elemental_wall(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill);

int spell_ethereal_focus(uint8_t level, Character *ch,
                         Character *victim, class Object *obj, int skill);
int cast_ethereal_focus(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill);

#endif
