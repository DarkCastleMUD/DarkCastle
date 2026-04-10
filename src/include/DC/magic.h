/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *        The prototypes for functions defined in magic.c that can/are       *
 *        used else where in the mud. (Spells)                               *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id: magic.h,v 1.41 2015/06/14 02:38:12 pirahna Exp $ */
#pragma once

#include "DC/structs.h" // quint8, etc..

constexpr auto GLOBE_OF_DARKNESS_OBJECT = 101;

typedef qint32 (*SPELL_POINTER)(quint8, CharacterPtr, QString, qint32, CharacterPtr, ObjectPtr, qint32);

bool resist_spell(qint32 perc);
bool resist_spell(CharacterPtr ch, qint32 skill);
qint32 spellcraft(CharacterPtr ch, qint32 spell);

qint32 spell_iridescent_aura(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_resist_fire(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_resist_cold(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_acid_blast(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_acid_breath(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_animate_dead(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr corpse, qint32 skill);
qint32 spell_armor(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_aegis(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_portal(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_resist_magic(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);

qint32 spell_bee_sting(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_bee_swarm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_bless(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_blindness(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);

qint32 spell_burning_hands(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_burning_hands(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 cast_oaken_fortitude(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_clarity(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_divine_intervention(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_call_lightning(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cause_critical(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cause_light(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cause_serious(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_charm_person(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_chill_touch(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_colour_spray(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_conjure_elemental(quint8 level, CharacterPtr ch, QString arg, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cont_light(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_create_food(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_create_water(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_remove_blind(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cure_critic(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cure_light(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_cure_serious(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_curse(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);

qint32 spell_shadowslip(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_camouflague(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_farsight(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_resist_energy(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_staunchblood(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_insomnia(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_freefloat(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_earthquake(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_energy_drain(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_drown(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_howl(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_souldrain(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_sparks(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_fireball(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_heal_spray(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_life_leech(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_lightning_bolt(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_magic_missile(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_meteor_swarm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_shocking_grasp(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_vampiric_touch(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_firestorm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_dispel_evil(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_dispel_good(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_harm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_power_harm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_divine_fury(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_teleport(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_paralyze(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_remove_paralysis(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_detect_evil(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_detect_good(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_true_sight(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_detect_invisibility(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_infravision(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_detect_magic(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_haste(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_detect_poison(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_enchant_weapon(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_eyes_of_the_owl(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_feline_agility(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_heal(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_power_heal(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_full_heal(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_invisibility(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_locate_object(quint8 level, CharacterPtr ch, const QString arg, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_oaken_fortitude(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_poison(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_protection_from_evil(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_protection_from_good(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_remove_curse(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill, quint64 mana_cost = {});
qint32 spell_remove_poison(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_fireshield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_sanctuary(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_sleep(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_strength(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_ventriloquate(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_word_of_recall(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_wizard_eye(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_eagle_eye(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_summon(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_sense_life(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_identify(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_frost_breath(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_fire_breath(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_gas_breath(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_lightning_breath(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_fear(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_refresh(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_fly(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_know_alignment(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_dispel_magic(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill, qint32 spell = 0);
qint32 spell_flamestrike(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_stone_skin(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_shield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_weaken(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_mass_invis(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_hellstream(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_group_sanc(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_ghost_walk(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_mend_golem(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);

qint32 cast_ghost_walk(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_mend_golem(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_iridescent_aura(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_camouflague(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_farsight(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_resist_energy(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_staunchblood(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_freefloat(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_insomnia(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_shadowslip(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_call_lightning(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_chill_touch(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_shocking_grasp(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_colour_spray(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_earthquake(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_life_leech(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_firestorm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_energy_drain(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_drown(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_howl(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_souldrain(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_sparks(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_vampiric_touch(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_meteor_swarm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_fireball(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_harm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_power_harm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_divine_fury(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_lightning_bolt(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_magic_missile(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_armor(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_aegis(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_teleport(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_bless(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_paralyze(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_blindness(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_control_weather(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_create_food(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_create_water(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_remove_paralysis(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_remove_blind(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cure_critic(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cure_light(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_curse(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_detect_evil(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_true_sight(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_detect_good(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_detect_invisibility(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_detect_magic(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_haste(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_detect_poison(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_dispel_evil(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_dispel_good(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_enchant_weapon(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_heal(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_power_heal(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_full_heal(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_invisibility(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_locate_object(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_poison(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_protection_from_evil(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_protection_from_good(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_remove_curse(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill, quint64 mana_cost = {});
qint32 cast_remove_poison(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_fireshield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_sanctuary(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_sleep(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_strength(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_ventriloquate(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_word_of_recall(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_wizard_eye(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_eagle_eye(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_summon(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_charm_person(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_sense_life(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_identify(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_frost_breath(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_acid_breath(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_fire_breath(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_gas_breath(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_lightning_breath(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_fear(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_refresh(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_fly(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cont_light(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_know_alignment(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_dispel_magic(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_conjure_elemental(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cure_serious(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cause_light(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cause_critical(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_cause_serious(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_flamestrike(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_stone_skin(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_weaken(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_mass_invis(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_acid_blast(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_hellstream(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_portal(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_infravision(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_animate_dead(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_mana(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_solar_gate(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_heroes_feast(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_heal_spray(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_group_sanc(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_group_recall(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_group_fly(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_enchant_armor(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_resist_fire(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_resist_magic(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_resist_cold(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_bee_sting(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_bee_swarm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_creeping_death(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_barkskin(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_herb_lore(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_call_follower(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_entangle(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_eyes_of_the_owl(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_feline_agility(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_forest_meld(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_companion(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 cast_create_golem(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);

qint32 spell_dispel_minor(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_dispel_minor(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);

qint32 spell_release_golem(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 spell_beacon(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);
qint32 spell_reflect(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill);

qint32 spell_stone_shield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_stone_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_greater_stone_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_summon_familiar(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_summon_familiar(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_lighted_path(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_lighted_path(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_resist_acid(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_resist_acid(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_sun_ray(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_sun_ray(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_rapid_mend(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_rapid_mend(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_iron_roots(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_iron_roots(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_acid_shield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_acid_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_water_breathing(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_water_breathing(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_globe_of_darkness(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_globe_of_darkness(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_barkskin(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_entangle(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 spell_eyes_of_the_eagle(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_eyes_of_the_eagle(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_icestorm(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_icestorm(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_lightning_shield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_lightning_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_blue_bird(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_blue_bird(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_debility(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_debility(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_attrition(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_attrition(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_vampiric_aura(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_vampiric_aura(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_holy_aura(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_holy_aura(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

SPELL_POINTER get_wild_magic_defensive(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
SPELL_POINTER get_wild_magic_offensive(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);

qint32 spell_wild_magic(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_wild_magic(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_stability(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_stability(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_solidity(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_solidity(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_frostshield(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_frostshield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_release_elemental(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_release_elemental(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_dismiss_familiar(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_dismiss_familiar(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_dismiss_corpse(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_dismiss_corpse(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_visage_of_hate(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_visage_of_hate(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_blessed_halo(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_blessed_halo(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_mana(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);

qint32 cast_wrath_of_god(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_atonement(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_silence(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_immunity(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_boneshield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_channel(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 cast_spirit_shield(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_villainy(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_villainy(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_heroism(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_heroism(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_consecrate(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_consecrate(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
qint32 spell_desecrate(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_desecrate(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_elemental_wall(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_elemental_wall(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);

qint32 spell_ethereal_focus(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
qint32 cast_ethereal_focus(quint8 level, CharacterPtr ch, const QString arg, qint32 type, CharacterPtr victim, ObjectPtr tar_obj, qint32 skill);
