/*
 * sing.h contains the header files for the
 * singing bard powers, although the sing structure is in
 * spells.h, so that must be included
 */
/* $Id: sing.h,v 1.17 2012/01/11 03:28:37 jhhudso Exp $ */

#ifndef SING_H_
#define SING_H_

#include "DC/structs.h" // uint8_t, uint8_t, etc..
#include "DC/common.h"

#define BARD_MAX_RATING 3

typedef int SING_FUN(uint8_t level, Character *ch, char *arg, Character *victim, int skill);

class song_info_type
{
	uint8_t beats_;				  /* Waiting time after ki */
	position_t minimum_position_; /* min position for use */
	uint8_t min_useski_;		  /* minimum ki used */
	int16_t skill_num_;			  /* skill number of the song */
	int16_t targets_;			  /* Legal targets */
	int16_t rating_;			  /* Rating for orchestrate */
	SING_FUN *song_pointer_;	  /* function to call */
	SING_FUN *exec_pointer_;	  /* other function to call */
	SING_FUN *song_pulse_;		  /* other other function to call */
	SING_FUN *intrp_pointer_;	  /* other other function to call */
	int difficulty_;

public:
	song_info_type(uint8_t beats, position_t minimum_position, uint8_t min_useski, int16_t skill_num,
				   int16_t targets, int16_t rating, SING_FUN *song_pointer, SING_FUN *exec_pointer, SING_FUN *song_pulse,
				   SING_FUN *intrp_pointer, int difficulty)
		: beats_(beats), minimum_position_(minimum_position), min_useski_(min_useski), skill_num_(skill_num),
		  targets_(targets), rating_(rating), song_pointer_(song_pointer), exec_pointer_(exec_pointer), song_pulse_(song_pulse),
		  intrp_pointer_(intrp_pointer), difficulty_(difficulty)
	{
	}
	uint8_t beats(void) const { return beats_; }
	position_t minimum_position(void) const { return minimum_position_; }
	uint8_t min_useski(void) const { return min_useski_; }
	int16_t skill_num(void) const { return skill_num_; }
	int16_t targets(void) const { return targets_; }
	int16_t rating(void) const { return rating_; }
	SING_FUN *song_pointer(void) const { return song_pointer_; }
	SING_FUN *exec_pointer(void) const { return exec_pointer_; }
	SING_FUN *song_pulse(void) const { return song_pulse_; }
	SING_FUN *intrp_pointer(void) const { return intrp_pointer_; }
	int difficulty(void) const { return difficulty_; }
};
extern const QList<song_info_type> song_info;

struct songInfo
{
	int16_t song_timer;	 /* status for songs being sung */
	int16_t song_number; /* number of song being sung */
	char *song_data;
};

/************************************************************************
| Function declarations
*/
QDebug operator<<(QDebug debug, const songInfo &song);
char *skip_spaces(char *string);
void stop_grouped_bards(Character *ch, int action);
void update_character_singing(Character *ch);
void get_instrument_bonus(Character *ch, int &comb, int &non_comb);

SING_FUN song_whistle_sharp;
SING_FUN song_disrupt;
SING_FUN song_healing_melody;
SING_FUN execute_song_healing_melody;
SING_FUN song_revealing_stacato;
SING_FUN execute_song_revealing_stacato;
SING_FUN song_note_of_knowledge;
SING_FUN execute_song_note_of_knowledge;
SING_FUN song_terrible_clef;
SING_FUN execute_song_terrible_clef;
SING_FUN song_listsongs;
SING_FUN song_soothing_remembrance;
SING_FUN execute_song_soothing_remembrance;
SING_FUN song_traveling_march;
SING_FUN execute_song_traveling_march;
SING_FUN song_stop;
SING_FUN song_summon_song;
SING_FUN execute_song_summon_song;
SING_FUN song_astral_chanty;
SING_FUN execute_song_astral_chanty;
SING_FUN pulse_song_astral_chanty;
SING_FUN song_forgetful_rhythm;
SING_FUN execute_song_forgetful_rhythm;
SING_FUN song_shattering_resonance;
SING_FUN execute_song_shattering_resonance;
SING_FUN song_insane_chant;
SING_FUN execute_song_insane_chant;
SING_FUN song_flight_of_bee;
SING_FUN execute_song_flight_of_bee;
SING_FUN pulse_flight_of_bee;
SING_FUN intrp_flight_of_bee;
SING_FUN song_searching_song;
SING_FUN execute_song_searching_song;
SING_FUN song_jig_of_alacrity;
SING_FUN execute_song_jig_of_alacrity;
SING_FUN pulse_jig_of_alacrity;
SING_FUN intrp_jig_of_alacrity;
SING_FUN song_glitter_dust;
SING_FUN execute_song_glitter_dust;
SING_FUN song_bountiful_sonnet;
SING_FUN execute_song_bountiful_sonnet;
SING_FUN song_synchronous_chord;
SING_FUN execute_song_synchronous_chord;
SING_FUN song_sticky_lullaby;
SING_FUN execute_song_sticky_lullaby;
SING_FUN song_vigilant_siren;
SING_FUN execute_song_vigilant_siren;
SING_FUN pulse_vigilant_siren;
SING_FUN intrp_vigilant_siren;
SING_FUN song_unresistable_ditty;
SING_FUN execute_song_unresistable_ditty;
SING_FUN song_fanatical_fanfare;
SING_FUN execute_song_fanatical_fanfare;
SING_FUN pulse_song_fanatical_fanfare;
SING_FUN intrp_song_fanatical_fanfare;
SING_FUN song_dischordant_dirge;
SING_FUN execute_song_dischordant_dirge;
SING_FUN song_crushing_crescendo;
SING_FUN execute_song_crushing_crescendo;
SING_FUN song_hypnotic_harmony;
SING_FUN execute_song_hypnotic_harmony;
SING_FUN song_mking_charge;
SING_FUN execute_song_mking_charge;
SING_FUN pulse_mking_charge;
SING_FUN intrp_mking_charge;
SING_FUN song_submariners_anthem;
SING_FUN execute_song_submariners_anthem;
SING_FUN pulse_submariners_chorus;
SING_FUN intrp_submariners_chorus;

#endif // SING_H_
