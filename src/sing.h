/*
 * sing.h contains the header files for the
 * singing bard powers, although the sing structure is in
 * spells.h, so that must be included
 */
/* $Id: sing.h,v 1.17 2012/01/11 03:28:37 jhhudso Exp $ */

#ifndef SING_H_
#define SING_H_

#include <structs.h> // ubyte, ubyte, etc..

#define BARD_MAX_RATING 3

typedef int	SING_FUN		( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill);

struct song_info_type
{
	ubyte beats;	/* Waiting time after ki */
	ubyte minimum_position; /* min position for use */
	ubyte min_useski;	/* minimum ki used */
        int16 skill_num;       /* skill number of the song */
	int16 targets;		/* Legal targets */
        int16 rating;		/* Rating for orchestrate */
	SING_FUN *song_pointer;	/* function to call */
        SING_FUN *exec_pointer; /* other function to call */
        SING_FUN *song_pulse;    /* other other function to call */
        SING_FUN *intrp_pointer; /* other other function to call */
        int difficulty;
};

struct songInfo
{
	int16 song_timer; /* status for songs being sung */
	int16 song_number; /* number of song being sung */
	char * song_data; 
};

/************************************************************************
| Function declarations
*/

char * skip_spaces(char *string);
void stop_grouped_bards(CHAR_DATA *ch, int action);

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
