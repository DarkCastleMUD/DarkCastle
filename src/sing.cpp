/**************************************************************************
 * sing.cpp - implementation of bard songs                                *
 * Pirahna                                                                *
 *                                                                        *
 **************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "DC/sing.h"
#include "DC/room.h"
#include "DC/character.h"
#include "DC/spells.h" // tar_char..
#include "DC/race.h"
#include "DC/utility.h"
#include "DC/player.h"
#include "DC/interp.h"
#include "DC/mobile.h"
#include "DC/fight.h"
#include "DC/handler.h"
#include "DC/connect.h"
#include "DC/act.h"
#include "DC/db.h"
#include "DC/magic.h"  // dispel_magic
#include "DC/innate.h" // SKILL_INNATE_EVASION
#include "DC/returnvals.h"
#include "DC/const.h"
#include "DC/inventory.h"
#include "DC/handler.h"

Character *origsing = nullptr;

void check_eq(Character *ch);

//        uint8_t beats;     /* Waiting time after ki */
//        uint8_t minimum_position; /* min position for use */
//        uint8_t min_useski;       /* minimum ki used */
//        int16_t skill_num;       /* skill number of the song */
//        int16_t targets;         /* Legal targets */
//        int16_t rating;		/* Rating for orchestrate */
//        SING_FUN *song_pointer; /* function to call */
//        SING_FUN *exec_pointer; /* other function to call */
//        SING_FUN *song_pulse;    /* other other function to call */
//        SING_FUN *intrp_pointer; /* other other function to call */
//	  int difficulty

const QList<song_info_type> song_info = {

	{/* 0 */
	 1, position_t::RESTING, 0, SKILL_SONG_LIST_SONGS,
	 TAR_IGNORE, 0, song_listsongs, nullptr, nullptr, nullptr,
	 SKILL_INCREASE_EASY},

	{/* 1 */
	 1, position_t::FIGHTING, 1, SKILL_SONG_WHISTLE_SHARP,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 1, song_whistle_sharp, nullptr, nullptr, nullptr,
	 SKILL_INCREASE_MEDIUM},

	{/* 2 */
	 0, position_t::RESTING, 0, SKILL_SONG_STOP,
	 TAR_IGNORE, 0, song_stop, nullptr, nullptr, nullptr, SKILL_INCREASE_EASY},

	{/* 3 */
	 10, position_t::RESTING, 2, SKILL_SONG_TRAVELING_MARCH,
	 TAR_IGNORE, 1, song_traveling_march, execute_song_traveling_march, nullptr, nullptr,
	 SKILL_INCREASE_EASY},

	{/* 4 */
	 10, position_t::RESTING, 6, SKILL_SONG_BOUNT_SONNET,
	 TAR_IGNORE, 1, song_bountiful_sonnet, execute_song_bountiful_sonnet,
	 nullptr, nullptr, SKILL_INCREASE_EASY},

	{/* 5 */
	 5, position_t::FIGHTING, 9, SKILL_SONG_INSANE_CHANT,
	 TAR_IGNORE, 2, song_insane_chant, execute_song_insane_chant,
	 nullptr, nullptr, SKILL_INCREASE_MEDIUM},

	{/* 7 */
	 4, position_t::RESTING, 5, SKILL_SONG_GLITTER_DUST,
	 TAR_IGNORE, 2, song_glitter_dust, execute_song_glitter_dust,
	 nullptr, nullptr, SKILL_INCREASE_HARD},

	{/* 8 */
	 6, position_t::RESTING, 2, SKILL_SONG_SYNC_CHORD,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, song_synchronous_chord, execute_song_synchronous_chord, nullptr,
	 nullptr, SKILL_INCREASE_MEDIUM},

	{/* 9 */
	 10, position_t::RESTING, 4, SKILL_SONG_HEALING_MELODY,
	 TAR_IGNORE, 1, song_healing_melody, execute_song_healing_melody, nullptr, nullptr,
	 SKILL_INCREASE_MEDIUM},

	{/* 10 */
	 3, position_t::SITTING, 7, SKILL_SONG_STICKY_LULL,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, 2, song_sticky_lullaby, execute_song_sticky_lullaby, nullptr, nullptr,
	 SKILL_INCREASE_HARD},

	{/* 11 */
	 1, position_t::RESTING, 1, SKILL_SONG_REVEAL_STACATO,
	 TAR_IGNORE, 2, song_revealing_stacato, execute_song_revealing_stacato, nullptr,
	 nullptr, SKILL_INCREASE_HARD},

	{/* 12 */
	 5, position_t::RESTING, 5, SKILL_SONG_FLIGHT_OF_BEE,
	 TAR_IGNORE, 1, song_flight_of_bee, execute_song_flight_of_bee,
	 nullptr, nullptr,
	 SKILL_INCREASE_MEDIUM},

	{/* 13 */
	 5, position_t::FIGHTING, 4, SKILL_SONG_JIG_OF_ALACRITY,
	 TAR_IGNORE, 3, song_jig_of_alacrity, execute_song_jig_of_alacrity, pulse_jig_of_alacrity, intrp_jig_of_alacrity,
	 SKILL_INCREASE_HARD},

	{/* 14 */
	 7, position_t::RESTING, 3, SKILL_SONG_NOTE_OF_KNOWLEDGE,
	 TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_CHAR_ROOM, 1, song_note_of_knowledge, execute_song_note_of_knowledge, nullptr, nullptr,
	 SKILL_INCREASE_MEDIUM},

	{/* 15 */
	 2, position_t::FIGHTING, 3, SKILL_SONG_TERRIBLE_CLEF,
	 TAR_IGNORE, 2, song_terrible_clef, execute_song_terrible_clef, nullptr,
	 nullptr,
	 SKILL_INCREASE_MEDIUM},

	{/* 16 */
	 10, position_t::RESTING, 6, SKILL_SONG_SOOTHING_REMEM,
	 TAR_IGNORE, 2, song_soothing_remembrance, execute_song_soothing_remembrance,
	 nullptr, nullptr, SKILL_INCREASE_MEDIUM},

	{/* 17 */
	 10, position_t::RESTING, 2, SKILL_SONG_FORGETFUL_RHYTHM,
	 TAR_CHAR_ROOM, 3, song_forgetful_rhythm, execute_song_forgetful_rhythm,
	 nullptr, nullptr, SKILL_INCREASE_HARD},

	{/* 18 */
	 7, position_t::RESTING, 4, SKILL_SONG_SEARCHING_SONG,
	 TAR_CHAR_WORLD, 3, song_searching_song, execute_song_searching_song, nullptr, nullptr, SKILL_INCREASE_HARD},

	{/* 19 */
	 4, position_t::RESTING, 6, SKILL_SONG_VIGILANT_SIREN,
	 TAR_IGNORE, 2, song_vigilant_siren, execute_song_vigilant_siren, pulse_vigilant_siren, intrp_vigilant_siren,
	 SKILL_INCREASE_HARD},

	{/* 20 */
	 17, position_t::RESTING, 10, SKILL_SONG_ASTRAL_CHANTY,
	 TAR_CHAR_WORLD, 3, song_astral_chanty, execute_song_astral_chanty, pulse_song_astral_chanty, nullptr, SKILL_INCREASE_HARD},

	{/* 21 */
	 1, position_t::FIGHTING, 8, SKILL_SONG_DISARMING_LIMERICK,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 2, song_disrupt, nullptr,
	 nullptr, nullptr, SKILL_INCREASE_HARD},

	{/* 22 */
	 2, position_t::FIGHTING, 6, SKILL_SONG_SHATTERING_RESO,
	 TAR_OBJ_ROOM, 2, song_shattering_resonance, execute_song_shattering_resonance,
	 nullptr, nullptr, SKILL_INCREASE_HARD},

	{/* 23 */
	 8, position_t::RESTING, 4, SKILL_SONG_UNRESIST_DITTY,
	 TAR_IGNORE, 2, song_unresistable_ditty, execute_song_unresistable_ditty,
	 nullptr, nullptr, SKILL_INCREASE_MEDIUM},
	{/* 24 */
	 8, position_t::RESTING, 8, SKILL_SONG_FANATICAL_FANFARE,
	 TAR_IGNORE, 1, song_fanatical_fanfare, execute_song_fanatical_fanfare, nullptr,
	 nullptr, SKILL_INCREASE_MEDIUM

	},
	{/* 25 */
	 9, position_t::FIGHTING, 7, SKILL_SONG_DISCHORDANT_DIRGE,
	 TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, song_dischordant_dirge, execute_song_dischordant_dirge, nullptr,
	 nullptr, SKILL_INCREASE_HARD},
	{/* 26 */
	 2, position_t::SITTING, 6, SKILL_SONG_CRUSHING_CRESCENDO,
	 TAR_IGNORE, 3, song_crushing_crescendo, execute_song_crushing_crescendo,
	 nullptr, nullptr, SKILL_INCREASE_HARD},
	{/* 27 */
	 15, position_t::STANDING, 20, SKILL_SONG_HYPNOTIC_HARMONY,
	 TAR_CHAR_ROOM, 3, song_hypnotic_harmony, execute_song_hypnotic_harmony, nullptr, nullptr, SKILL_INCREASE_HARD},
	{/* 28 */
	 12, position_t::SITTING, 6, SKILL_SONG_MKING_CHARGE,
	 TAR_IGNORE, 3, song_mking_charge, execute_song_mking_charge, pulse_mking_charge, intrp_mking_charge, SKILL_INCREASE_MEDIUM

	},
	{/* 29 */
	 20, position_t::RESTING, 8, SKILL_SONG_SUBMARINERS_ANTHEM,
	 TAR_IGNORE, 1, song_submariners_anthem, execute_song_submariners_anthem,
	 nullptr, nullptr, SKILL_INCREASE_MEDIUM},
	{/* 30 */
	 12, position_t::STANDING, 20, SKILL_SONG_SUMMONING_SONG,
	 TAR_IGNORE, 2, song_summon_song, execute_song_summon_song, nullptr,
	 nullptr, SKILL_INCREASE_MEDIUM},
};

int16_t use_song(Character *ch, int kn);
bool ARE_GROUPED(Character *sub, Character *obj);

int16_t use_song(Character *ch, int kn)
{
	return (song_info[kn].min_useski());
}

int getTotalRating(Character *ch)
{
	int rating = 0;
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		rating += song_info[(*i).song_number].rating();
	}

	return rating;
}

void stop_grouped_bards(Character *ch, int bardsing)
{
	Character *master = nullptr;
	follow_type *fvictim = nullptr;

	if (IS_SINGING(ch))
	{ // kill everybody elses love
		if (!(master = ch->master))
			master = ch;
		else
		{
			if (bardsing)
				origsing = master;
			do_sing(ch, "stop");
			origsing = nullptr;
		}
		for (fvictim = master->followers; fvictim; fvictim = fvictim->next)
		{
			if (fvictim->follower == ch && bardsing)
				continue;
			else
				origsing = fvictim->follower;
			do_sing(ch, "stop");
			origsing = nullptr;
		}
	}
	else
	{ // kill the person's love

		origsing = ch;

		if (!(master = ch->master))
			master = ch;

		for (fvictim = master->followers; fvictim; fvictim = fvictim->next)
		{
			// end any performances
			if (IS_SINGING(fvictim->follower))
				do_sing(fvictim->follower, "stop");
		}

		if (IS_SINGING(master))
		{
			do_sing(master, "stop");
		}
		origsing = nullptr;
	}
}

void get_instrument_bonus(Character *ch, int &comb, int &non_comb)
{
	comb = 0;
	non_comb = 0;

	if (!ch->equipment[WEAR_HOLD])
		return;
	if (GET_ITEM_TYPE(ch->equipment[WEAR_HOLD]) != ITEM_INSTRUMENT)
		return;

	comb = ch->equipment[WEAR_HOLD]->obj_flags.value[1];
	non_comb = ch->equipment[WEAR_HOLD]->obj_flags.value[0];
}

int do_sing(Character *ch, char *arg, cmd_t cmd)
{
	Character *tar_char = 0;
	Object *tar_obj = 0;
	char name[MAX_STRING_LENGTH];
	char spellarg[MAX_STRING_LENGTH];
	char *argument = nullptr;
	int qend, spl = -1;
	bool target_ok;
	int learned;
	std::vector<songInfo>::iterator i;

	if (IS_PC(ch) && GET_CLASS(ch) != CLASS_BARD && !ch->isImmortalPlayer())
	{
		ch->check_social("sing");
		return eSUCCESS;
	}

	// we do this so we can pass constants to "do_sing" and no crash
	strcpy(spellarg, arg);
	argument = spellarg;

	argument = skip_spaces(argument);

	if (!(*argument))
	{
		ch->sendln("Yes, but WHAT would you like to sing?");
		return eFAILURE;
	}

	if (*argument == '\'') // song is in 's
	{
		argument++;
		for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
			*(argument + qend) = LOWER(*(argument + qend));
		if (*(argument + qend) != '\'')
		{
			ch->sendln("If you start with a ' you have to end with a ' too.");
			return eFAILURE;
		}
	}
	else
	{
		for (qend = 1; *(argument + qend) && (*(argument + qend) != ' '); qend++)
			*(argument + qend) = LOWER(*(argument + qend));
	}
	spl = old_search_block(argument, 0, qend, Character::song_names, 0);
	spl--; /* songs goes from 0+ not 1+ like spells */

	if (spl < 0)
	{
		ch->sendln("You know not of that song.");
		return eFAILURE;
	}
	if (cmd == cmd_t::ORCHESTRATE)
	{
		if (!IS_SINGING(ch))
		{
			ch->sendln("You must be singing a song to orchestrate another melody.");
			return eFAILURE;
		}
		if ((!ch->equipment[WEAR_HOLD] || GET_ITEM_TYPE(ch->equipment[WEAR_HOLD]) != ITEM_INSTRUMENT) && (!ch->equipment[WEAR_HOLD2] || GET_ITEM_TYPE(ch->equipment[WEAR_HOLD2]) != ITEM_INSTRUMENT))
		{
			ch->sendln("You must be holding an instrument to orchestrate songs.");
			return eFAILURE;
		}
	}
	else if (song_info[spl].rating() > 0 && IS_SINGING(ch))
	{
		if (ch->has_skill(SKILL_ORCHESTRATE))
			ch->sendln("You are already in the middle of another song!  Try using orchestrate.");
		else
			ch->sendln("You are already in the middle of another song!");
		return eFAILURE;
	}
	else if (spl == 2 && !IS_SINGING(ch))
	{
		ch->sendln("You are not even singing a song to stop!");
		return eFAILURE;
	}

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == spl)
		{
			ch->sendln("You are already singing this song!");
			return eFAILURE;
		}

	if ((isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE)) && (ch->getLevel() < IMPLEMENTER) && (spl == SKILL_SONG_WHISTLE_SHARP - SKILL_SONG_BASE || spl == SKILL_SONG_UNRESIST_DITTY - SKILL_SONG_BASE || spl == SKILL_SONG_GLITTER_DUST - SKILL_SONG_BASE || spl == SKILL_SONG_STICKY_LULL - SKILL_SONG_BASE || spl == SKILL_SONG_REVEAL_STACATO - SKILL_SONG_BASE || spl == SKILL_SONG_TERRIBLE_CLEF - SKILL_SONG_BASE || spl == SKILL_SONG_DISCHORDANT_DIRGE - SKILL_SONG_BASE || spl == SKILL_SONG_INSANE_CHANT - SKILL_SONG_BASE || spl == SKILL_SONG_JIG_OF_ALACRITY - SKILL_SONG_BASE || spl == SKILL_SONG_DISARMING_LIMERICK - SKILL_SONG_BASE || spl == SKILL_SONG_CRUSHING_CRESCENDO - SKILL_SONG_BASE || spl == SKILL_SONG_SHATTERING_RESO - SKILL_SONG_BASE || spl == SKILL_SONG_MKING_CHARGE - SKILL_SONG_BASE || spl == SKILL_SONG_HYPNOTIC_HARMONY - SKILL_SONG_BASE))
	{
		ch->sendln("This room feels too safe to sing an offensive song such as this.");
		return eFAILURE;
	}

	if (song_info[spl].song_pointer())
	{
		if (GET_POS(ch) < song_info[spl].minimum_position() && IS_PC(ch) && spl != SPELL_TYPE_WAND)
		{
			switch (GET_POS(ch))
			{
			case position_t::SLEEPING:
				ch->sendln("You dream of beautiful music.");
				break;
			case position_t::RESTING:
				ch->sendln("You can't sing this resting!!");
				break;
			case position_t::SITTING:
				ch->sendln("You can't do this sitting.  You must stand up.");
				break;
			case position_t::FIGHTING:
				ch->sendln("This is a peaceful song.  Not for battle.");
				break;
			default:
				ch->sendln("It seems like you're in a pretty bad shape!");
				break;
			}
			return eFAILURE;
		}
		else
		{
			if (ch->getLevel() < ARCHANGEL && spl != 0 && spl != SPELL_TYPE_WAND)
				if (!(learned = ch->has_skill(song_info[spl].skill_num())))
				{
					if (IS_NPC(ch) && !ch->master)
						learned = 50;
					else
					{
						ch->sendln("You haven't learned that song.");
						return eFAILURE;
					}
				}
		}

		if (getTotalRating(ch) + song_info[spl].rating() > BARD_MAX_RATING)
		{
			ch->sendln("You are unable to orchestrate such a complicated melody!");
			return eFAILURE;
		}

		argument += qend;	   /* Point to the space after the last ' */
		if (*argument == '\'') // they sang 'song with space'
			argument++;
		for (; *argument == ' '; argument++)
			; /* skip spaces */

		/* Locate targets */
		target_ok = false;

		one_argument(argument, name);

		if (!isSet(song_info[spl].targets(), TAR_IGNORE))
		{
			if (*name)
			{
				if (isSet(song_info[spl].targets(), TAR_CHAR_ROOM))
					if ((tar_char = ch->get_char_room_vis(name)) != nullptr)
						target_ok = true;

				if (!target_ok && isSet(song_info[spl].targets(), TAR_CHAR_WORLD))
					if ((tar_char = get_char_vis(ch, name)) != nullptr)
						target_ok = true;

				if (!target_ok && isSet(song_info[spl].targets(), TAR_OBJ_INV))
					if ((tar_obj = get_obj_in_list_vis(ch, name, ch->carrying)) != nullptr)
						target_ok = true;

				if (!target_ok && isSet(song_info[spl].targets(), TAR_OBJ_ROOM))
				{
					tar_obj = get_obj_in_list_vis(ch, name, DC::getInstance()->world[ch->in_room].contents);
					if (tar_obj != nullptr)
						target_ok = true;
				}

				if (!target_ok && isSet(song_info[spl].targets(), TAR_OBJ_EQUIP))
				{
					for (int i = 0; i < MAX_WEAR && !target_ok; i++)
						if (ch->equipment[i] && ch->equipment[i]->Name() == name)
						{
							tar_obj = ch->equipment[i];
							target_ok = true;
						}
				}

				if (!target_ok && isSet(song_info[spl].targets(), TAR_OBJ_WORLD))
					if ((tar_obj = get_obj_vis(ch, name)) != nullptr)
						target_ok = true;

				if (!target_ok && isSet(song_info[spl].targets(), TAR_SELF_ONLY))
					if (str_cmp(GET_NAME(ch), name) == 0)
					{
						tar_char = ch;
						target_ok = true;
					} // of !target_ok
			} // of *name

			/* No argument was typed */
			else if (!*name)
			{
				if (isSet(song_info[spl].targets(), TAR_FIGHT_VICT))
					if (ch->fighting)
						if ((ch->fighting)->in_room == ch->in_room)
						{
							tar_char = ch->fighting;
							target_ok = true;
						}
				if (!target_ok && isSet(song_info[spl].targets(), TAR_SELF_ONLY))
				{
					tar_char = ch;
					target_ok = true;
				}
			} // of !*name

			else
				target_ok = false;
		}

		if (isSet(song_info[spl].targets(), TAR_IGNORE))
			target_ok = true;

		if (target_ok != true)
		{
			if (*name)
			{
				if (isSet(song_info[spl].targets(), TAR_CHAR_ROOM))
					ch->sendln("Nobody here by that name.");
				else if (isSet(song_info[spl].targets(), TAR_CHAR_WORLD))
					ch->sendln("Nobody playing by that name.");
				else if (isSet(song_info[spl].targets(), TAR_OBJ_INV))
					ch->sendln("You are not carrying anything like that.");
				else if (isSet(song_info[spl].targets(), TAR_OBJ_ROOM))
					ch->sendln("Nothing here by that name.");
				else if (isSet(song_info[spl].targets(), TAR_OBJ_WORLD))
					ch->sendln("Nothing at all by that name.");
				else if (isSet(song_info[spl].targets(), TAR_OBJ_EQUIP))
					ch->sendln("You are not wearing anything like that.");
				else if (isSet(song_info[spl].targets(), TAR_OBJ_WORLD))
					ch->sendln("Nothing at all by that name.");
			}
			else
				/* No arguments were given */
				ch->sendln("Whom should you sing to?");
			return eFAILURE;
		}

		else if (target_ok)
		{
			if ((tar_char == ch) && isSet(song_info[spl].targets(), TAR_SELF_NONO))
			{
				ch->sendln("You cannot sing this to yourself!");
				return eFAILURE;
			}
			else if ((tar_char != ch) && isSet(song_info[spl].targets(), TAR_SELF_ONLY))
			{
				ch->sendln("You can only sing this song to yourself.");
				return eFAILURE;
			}
			else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tar_char))
			{
				ch->sendln("You are afraid that it might harm your master.");
				return eFAILURE;
			}
		}

		if (!isSet(song_info[spl].targets(), TAR_IGNORE))
			if (!tar_char && !tar_obj)
			{
				DC::getInstance()->logentry(QStringLiteral("Dammit, fix that null tar_char thing in do_song"), IMPLEMENTER, DC::LogChannel::LOG_BUG);
				send_to_char("If you triggered this message, you almost crashed the\r\n"
							 "game.  Tell a god what you did immediately.\r\n",
							 ch);
				return eFAILURE | eINTERNAL_ERROR;
			}

		if (spl != SKILL_SONG_STOP - SKILL_SONG_BASE && isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_KI))
		{
			ch->sendln("You find yourself unable to use energy based chants here.");
			return eFAILURE;
		}

		if (ch->getLevel() < ARCHANGEL && ch->isPlayer() &&
			GET_KI(ch) < use_song(ch, spl))
		{
			ch->sendln("You do not have enough ki!");
			return eFAILURE;
		}

		// WAIT_STATE(ch, song_info[spl].beats());
		// Bards don't get a wait state for singing.  The songs take time
		// to go off, and 'beats' is how long it takes them.  Certain songs
		// DO give a wait state, but those songs apply the wait state internal
		// to the "do_song" code

		if ((song_info[spl].song_pointer() == nullptr) && spl > 0)
		{
			ch->sendln("Sorry, this power has not yet been implemented.");
			return eFAILURE;
		}
		else
		{

			learned = ch->has_skill(song_info[spl].skill_num());

			if (spl == SKILL_SONG_HYPNOTIC_HARMONY - SKILL_SONG_BASE || spl == SKILL_SONG_SUMMONING_SONG - SKILL_SONG_BASE || spl == SKILL_SONG_DISARMING_LIMERICK - SKILL_SONG_BASE || spl == SKILL_SONG_SHATTERING_RESO - SKILL_SONG_BASE || spl == SKILL_SONG_SEARCHING_SONG - SKILL_SONG_BASE || spl == SKILL_SONG_FANATICAL_FANFARE - SKILL_SONG_BASE || spl == SKILL_SONG_MKING_CHARGE - SKILL_SONG_BASE || spl == SKILL_SONG_VIGILANT_SIREN - SKILL_SONG_BASE)
			{
				if ((!ch->equipment[WEAR_HOLD] || GET_ITEM_TYPE(ch->equipment[WEAR_HOLD]) != ITEM_INSTRUMENT) && (!ch->equipment[WEAR_HOLD2] || GET_ITEM_TYPE(ch->equipment[WEAR_HOLD2]) != ITEM_INSTRUMENT))
				{
					ch->sendln("You can't even begin this song without an instrument.");
					return eFAILURE;
				}
			}

			bool skill_succeeded = skill_success(ch, tar_char, spl + SKILL_SONG_BASE);
			bool in_room_safe = isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE);
			if (spl != SPELL_TYPE_WAND && !skill_succeeded && !in_room_safe)
			{

				ch->sendln("You forgot the words!");
				GET_KI(ch) -= use_song(ch, spl) / 2;
				return eSUCCESS;
			}

			/* Stop abusing your betters  */
			if (!isSet(song_info[spl].targets(), TAR_IGNORE) && !tar_obj)
				if (IS_PC(tar_char) && (ch->getLevel() > ARCHANGEL) && (tar_char->getLevel() > ch->getLevel()))
				{
					ch->sendln("That just might annoy them!");
					return eFAILURE;
				}

			/* Imps ignore safe flags  */
			if (!isSet(song_info[spl].targets(), TAR_IGNORE) && !tar_obj)
				if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) && IS_PC(ch) && (ch->getLevel() == IMPLEMENTER))
				{
					tar_char->sendln("There is no safe haven from an angry IMPLEMENTER!");
				}

			if (cmd != cmd_t::ORCHESTRATE && IS_SINGING(ch)) // I'm singing
			{
				if (!origsing)
				{
					if (ch->songs.size() > 1 && !*name)
						ch->sendln("You stop orchestrating all of your music.");
					else if (ch->songs.size() > 1 && *name)
					{
						int hold = old_search_block(name, 0, strlen(name), Character::song_names, 0);
						bool found = false;
						if (--hold < 0)
						{
							ch->sendln("You do not know of that song.");
							return eFAILURE;
						}
						for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
							if ((*i).song_number == hold)
							{
								found = true;
								break;
							}
						if (!found)
						{
							ch->sendln("You are not singing that song.");
							return eFAILURE;
						}
						else
						{
							ch->send("You stop singing ");
							send_to_char(Character::song_names.value((*i).song_number), ch);
							ch->sendln(".");
						}
					}
					else
					{
						ch->send("You stop singing ");
						send_to_char(Character::song_names.value((*ch->songs.begin()).song_number), ch);
						ch->sendln(".");
					}
				}
				// If the song is a steady one, (like flight) than it needs to be
				// interrupted so we stop and remove the affects
				for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
				{
					if ((song_info[(*i).song_number].intrp_pointer()))
						((*song_info[(*i).song_number].intrp_pointer())(ch->getLevel(), ch, nullptr, nullptr, learned));
				}
				if (spl != SPELL_TYPE_WAND && !origsing) // song 'stop'
					ch->songs.clear();
			}
			else if (cmd == cmd_t::ORCHESTRATE)
			{
				if (!skill_success(ch, nullptr, SKILL_ORCHESTRATE))
				{
					ch->sendln("You failed to orchestrate your music!");
					return eFAILURE;
				}
				else
				{
					csendf(ch, "You seamlessly orchestrate a %s melody with your current song, playing them in perfect concert!\r\n",
						   numToStringTH(ch->songs.size() + 1));
					act("$n seamlessly orchestrates another melody with $s current song, playing them in perfect concert!", ch, 0, 0, TO_ROOM, 0);
				}
			}
			if (origsing)
				return eSUCCESS;

			GET_KI(ch) -= use_song(ch, spl);

			// There's no sense adding a song to the list if it's a 1-time song with no stop function
			// like stop, whistle sharp or listsongs
			if (song_info[spl].rating() > 0 && spl != SKILL_SONG_WHISTLE_SHARP - SKILL_SONG_BASE)
			{
				struct songInfo data;
				data.song_number = spl;
				data.song_timer = 0;
				data.song_data = 0;
				ch->songs.push_back(data);
			}

			return ((*song_info[spl].song_pointer())(ch->getLevel(), ch, argument, tar_char, learned));
		}
	}
	return eFAILURE;
}

void update_character_singing(Character *ch)
{
	for (std::vector<songInfo>::iterator j = ch->songs.begin(); j != ch->songs.end(); ++j)
	{
		if ((*j).song_timer == -1)
		{
			ch->sendln("You run out of lyrics and end the song.");
			if ((song_info[(*j).song_number].intrp_pointer()))
			{
				((*song_info[(*j).song_number].intrp_pointer())(ch->getLevel(), ch, nullptr, nullptr, -1));
				if (ch->songs.empty())
				{
					break;
				}
			}
			(*j).song_timer = 0;
			if ((*j).song_data)
			{
				if ((int64_t)(*j).song_data > 10) // Otherwise it's a temp variable.
					delete[] (*j).song_data;
				(*j).song_data = 0;
			}
			ch->songs.erase(j);
			--j;
			continue;
		}
		if ((*j).song_timer > 0)
		{
			if (ISSET(ch->affected_by, AFF_HIDE))
			{
				REMBIT(ch->affected_by, AFF_HIDE);
				ch->sendln("Your singing ruins your hiding place.");
			}
			if (ch->getLevel() < IMPLEMENTER && ((isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_KI) || isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE)) && ((*j).song_number == SKILL_SONG_WHISTLE_SHARP - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_UNRESIST_DITTY - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_GLITTER_DUST - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_STICKY_LULL - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_REVEAL_STACATO - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_TERRIBLE_CLEF - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_DISCHORDANT_DIRGE - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_INSANE_CHANT - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_JIG_OF_ALACRITY - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_SUMMONING_SONG - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_DISARMING_LIMERICK - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_CRUSHING_CRESCENDO - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_SHATTERING_RESO - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_MKING_CHARGE - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_HYPNOTIC_HARMONY - SKILL_SONG_BASE)))
			{
				ch->sendln("In this room, your song quiety fades away.");
				if ((song_info[(*j).song_number].intrp_pointer()))
				{
					((*song_info[(*j).song_number].intrp_pointer())(ch->getLevel(), ch, nullptr, nullptr, -1));
					if (ch->songs.empty())
					{
						break;
					}
				}
				if ((*j).song_data)
				{
					if ((int64_t)(*j).song_data > 10) // Otherwise it's a temp variable.
						delete[] (*j).song_data;
					(*j).song_data = 0;
				}
				(*j).song_timer = 0;
				ch->songs.erase(j);
				--j;
				continue;
			}
			else if ((((GET_POS(ch) < song_info[(*j).song_number].minimum_position()) && IS_PC(ch)) || isSet(ch->combat, COMBAT_STUNNED) || isSet(ch->combat, COMBAT_STUNNED2) || isSet(ch->combat, COMBAT_SHOCKED) || isSet(ch->combat, COMBAT_SHOCKED2) || (isSet(ch->combat, COMBAT_BASH1) || isSet(ch->combat, COMBAT_BASH2))) && ((*j).song_number == SKILL_SONG_TRAVELING_MARCH - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_BOUNT_SONNET - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_HEALING_MELODY - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_SYNC_CHORD - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_NOTE_OF_KNOWLEDGE - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_SOOTHING_REMEM - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_SEARCHING_SONG - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_STICKY_LULL - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_FORGETFUL_RHYTHM - SKILL_SONG_BASE))
			{
				ch->sendln("You can't keep singing in this position!");
				(*j).song_timer = 0;
				if ((song_info[(*j).song_number].intrp_pointer()))
				{
					((*song_info[(*j).song_number].intrp_pointer())(ch->getLevel(), ch, nullptr, nullptr, -1));
					if (ch->songs.empty())
					{
						break;
					}
				}
				if ((*j).song_data)
				{
					if ((int64_t)(*j).song_data > 10) // Otherwise it's a temp variable.
						delete[] (*j).song_data;
					(*j).song_data = 0;
				}
				ch->songs.erase(j);
				--j;
				continue;
			}
			else if ((*j).song_number == SKILL_SONG_HYPNOTIC_HARMONY - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_DISARMING_LIMERICK - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_SHATTERING_RESO - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_SEARCHING_SONG - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_FANATICAL_FANFARE - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_MKING_CHARGE - SKILL_SONG_BASE || (*j).song_number == SKILL_SONG_VIGILANT_SIREN - SKILL_SONG_BASE || j != ch->songs.begin())
			{
				if ((!ch->equipment[WEAR_HOLD] || GET_ITEM_TYPE(ch->equipment[WEAR_HOLD]) != ITEM_INSTRUMENT) && (!ch->equipment[WEAR_HOLD2] || GET_ITEM_TYPE(ch->equipment[WEAR_HOLD2]) != ITEM_INSTRUMENT))
				{
					ch->sendln("Without an instrument, your song dies away.");
					if ((song_info[(*j).song_number].intrp_pointer()))
					{
						((*song_info[(*j).song_number].intrp_pointer())(ch->getLevel(), ch, nullptr, nullptr, -1));
						if (ch->songs.empty())
						{
							break;
						}
					}
					(*j).song_timer = 0;
					if ((*j).song_data)
					{
						if ((int64_t)(*j).song_data > 10) // Otherwise it's a temp variable.
							delete[] (*j).song_data;
						(*j).song_data = 0;
					}
					ch->songs.erase(j);
					--j;
					continue;
				}
			}
		}

		if ((*j).song_timer > 1)
		{
			(*j).song_timer--;

			std::string buffer_for_singer = "Singing [" + Character::song_names.value((*j).song_number).toStdString() + "]: ";
			std::string buffer_for_group = "$N is singing [" + Character::song_names.value((*j).song_number).toStdString() + "]: ";
			std::string buffer_for_room = "$N is singing " + Character::song_names.value((*j).song_number).toStdString() + ".";
			for (int k = 0; k < (*j).song_timer; k++)
			{
				buffer_for_singer += "* ";
				buffer_for_group += "* ";
			}
			act(buffer_for_singer, ch, 0, ch, TO_CHAR, BARDSONG);
			act(buffer_for_group, ch, 0, ch, TO_GROUP, BARDSONG);
			act(buffer_for_room, ch, 0, ch, TO_ROOM_NOT_GROUP, BARDSONG);
		}
		else if ((*j).song_timer == 1)
		{
			(*j).song_timer = 0;

			int learned = ch->has_skill(((*j).song_number + SKILL_SONG_BASE));
			int retval = 0;

			if ((song_info[(*j).song_number].exec_pointer()))
			{
				retval = ((*song_info[(*j).song_number].exec_pointer())(ch->getLevel(), ch, nullptr, nullptr, learned));
				if (ch->songs.empty())
				{
					break;
				}
			}
			else
			{
				ch->sendln("Bad exec pointer on the song you sang.  Tell a god.");
			}

			if (retval == eEXTRA_VALUE)
			{ // the song killed itself
				--j;
			}
		}
		else if ((*j).song_timer == 0)
		{
			ch->songs.erase(j);
			--j;
			continue;
		}
	}
}
// Go down the list of chars, and update song timers.  If the timer runs
// out, then activate the effect
void update_bard_singing()
{
	for (const auto &ch : DC::getInstance()->character_list)
	{
		if (ch->isPlayer() && ch->getClass() != CLASS_BARD && !ch->isImmortalPlayer())
		{
			continue;
		}

		if (ch->songs.empty())
		{
			continue;
		}

		if (ch->getPosition() == position_t::DEAD || ch->in_room == DC::NOWHERE)
		{
			continue;
		}

		update_character_singing(ch);
	}

	DC::getInstance()->removeDead();
}

int song_hypnotic_harmony(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	if (!victim || !ch)
	{
		DC::getInstance()->logentry(QStringLiteral("Serious problem in song_hypnotic_harmony!"), ANGEL, DC::LogChannel::LOG_BUG);
		return eFAILURE | eINTERNAL_ERROR;
	}
	act("$n sings an incredibly beautiful hymn, making you want to just give up your dayjob and follow $m around!", ch, 0, victim, TO_VICT, 0);
	act("$n sings an entrancing hymn to $N!", ch, 0, victim, TO_ROOM, NOTVICT);
	ch->sendln("You sing your most enchanting hymn, hoping to attract some fans.");

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_HYPNOTIC_HARMONY - SKILL_SONG_BASE)
			break;
	}

	(*i).song_data = str_dup(arg);
	(*i).song_timer = (song_info[(*i).song_number].beats() - (skill / 15));

	return eSUCCESS;
}

int execute_song_hypnotic_harmony(uint8_t level, Character *ch, char *Arg, Character *victim, int skill)
{
	struct affected_type af;
	std::vector<songInfo>::iterator i;

	if (!ch || ch->songs.empty())
	{
		DC::getInstance()->logentry(QStringLiteral("Serious problem in execute_song_hypnotic_harmony!"), ANGEL, DC::LogChannel::LOG_BUG);
		return eFAILURE | eINTERNAL_ERROR;
	}

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_HYPNOTIC_HARMONY - SKILL_SONG_BASE)
			break;
	}

	if (!(victim = ch->get_char_room_vis((*i).song_data)))
	{
		delete[] (*i).song_data;
		(*i).song_data = 0;
		ch->sendln("They seem to have left.\r\nIn the middle of your performance too!");
		return eFAILURE;
	}
	delete[] (*i).song_data;
	(*i).song_data = 0;

	WAIT_STATE(ch, DC::PULSE_VIOLENCE);
	if (IS_PC(victim) || !ISSET(victim->mobdata->actflags, ACT_BARDCHARM))
	{
		ch->sendln("They don't seem particularily interested.");
		victim->sendln("You manage to resist the entrancing lyrics.");
		return eFAILURE;
	}

	if (circle_follow(victim, ch))
	{
		ch->sendln("Sorry, following in circles can not be allowed.");
		return eFAILURE;
	}

	int charm_levels(Character * ch);
	int charm_space(int level);

	if (charm_levels(ch) - charm_space(victim->getLevel()) < 0 && victim->master != ch)
	{
		ch->sendln("How you plan on controlling so many followers?");
		return eFAILURE;
	}

	if (victim->master)
		stop_follower(victim);

	remove_memory(victim, 'h');

	add_follower(victim, ch);

	af.type = SPELL_CHARM_PERSON;
	af.duration = 24 + ((level > 40) * 6) + ((level > 60) * 6) + ((level > 80) * 12);

	af.modifier = 0;
	af.location = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(victim, &af);

	/* remove any !charmie eq the charmie is wearing */
	check_eq(victim);

	act("You decide to follow $n's musical genius to the end.", ch, 0, victim, TO_VICT, 0);
	ch->sendln("You succeed, and you find yourself having a new fan.");
	return eSUCCESS;
}

int song_disrupt(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	if (!victim || !ch)
	{
		DC::getInstance()->logentry(QStringLiteral("Serious problem in song_disrupt!"), ANGEL, DC::LogChannel::LOG_BUG);
		return eFAILURE | eINTERNAL_ERROR;
	}

	int learned = ch->has_skill(song_info[SKILL_SONG_DISARMING_LIMERICK - SKILL_SONG_BASE].skill_num());

	act("$n sings a witty little limerick to you!\r\nYour laughing makes it hard to concentrate on keeping your spells up!", ch, 0, victim, TO_VICT, 0);
	act("$n sings a hilarious limerick about a man from Nantucket to $N!", ch, 0, victim, TO_ROOM, NOTVICT);
	ch->sendln("You sing your funniest limerick!");

	WAIT_STATE(ch, DC::PULSE_VIOLENCE);
	if (number(1, 100) < get_saves(victim, SAVE_TYPE_MAGIC))
	{
		act("$N resists your disarming limerick!", ch, nullptr, victim, TO_CHAR, 0);
		act("$N resists $n's disarming limerick!", ch, nullptr, victim, TO_ROOM, NOTVICT);
		act("You resist $n's disarming limerick!", ch, nullptr, victim, TO_VICT, 0);
		return eFAILURE;
	}

	if (learned > 90)
	{
		if (isSet(victim->combat, COMBAT_REPELANCE))
		{
			act("Your limerick disrupts $S magical barrier!", ch, 0, victim, TO_CHAR, 0);
			act("$n's limerick broke your concentration of your magical barrier!", ch, 0, victim, TO_VICT, 0);
			act("$N's concentration faultered from $n's gut-busting limerick!", ch, 0, victim, TO_ROOM, NOTVICT);
			REMOVE_BIT(victim->combat, COMBAT_REPELANCE);
			return eSUCCESS;
		}
	}
	if (learned > 85)
	{
		if (victim->affected_by_spell(KI_STANCE + KI_OFFSET))
		{
			act("Your limerick breaks $S stance!", ch, 0, victim, TO_CHAR, 0);
			act("$n's limerick causes you to break your stance!", ch, 0, victim, TO_VICT, 0);
			act("$N's stance breaks down from $n's hilarious limerick!", ch, 0, victim, TO_ROOM, NOTVICT);
			affect_from_char(victim, KI_STANCE + KI_OFFSET);
			return eSUCCESS;
		}
	}

	return spell_dispel_magic(ch->getLevel() - 1, ch, victim, 0, 0);
}

int song_whistle_sharp(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	int dam = 0;
	int retval;
	std::vector<songInfo>::iterator i;

	if (!victim)
	{
		DC::getInstance()->logentry(QStringLiteral("No vict send to song whistle sharp!"), ANGEL, DC::LogChannel::LOG_BUG);
		return eFAILURE | eINTERNAL_ERROR;
	}

	if (!can_attack(ch) || !can_be_attacked(ch, victim))
		return eFAILURE;

	set_cantquit(ch, victim);

	if (number(1, 1000) == 1)
	{
		act("$n's piercing note causes $N's brains to leak from $S ears. EEEW!", ch, 0, victim, TO_ROOM, NOTVICT);
		act("$n's piercing note turns your brains to pulp!", ch, 0, victim, TO_VICT, 0);
		act("Your piercing note causes $N's brain to leak out $S ears in a "
			"painful death.",
			ch, 0, victim, TO_CHAR, 0);
		return damage(ch, victim, 9999999, TYPE_UNDEFINED, SKILL_SONG_WHISTLE_SHARP);
	}

	int combat, non_combat;
	get_instrument_bonus(ch, combat, non_combat);

	//   dam = ch->getLevel() + GET_INT(ch) + combat;
	dam = 80;

	if (number(1, 100) < get_saves(victim, SAVE_TYPE_MAGIC))
	{
		act("$N resists your whistle sharp!", ch, nullptr, victim, TO_CHAR, 0);
		act("$N resists $n's whistle sharp!", ch, nullptr, victim, TO_ROOM, NOTVICT);
		act("You resist $n's whistle sharp!", ch, nullptr, victim, TO_VICT, 0);
		dam /= 2;
	}

	char buf[MAX_STRING_LENGTH];
	strcpy(buf, victim->short_desc);

	retval = damage(ch, victim, dam, TYPE_SONG, SKILL_SONG_WHISTLE_SHARP);
	if (isSet(retval, eCH_DIED))
		return retval;

	if (isSet(retval, eVICT_DIED))
	{
		ch->sendln("You dance a small jig on the corpse.");
		act("$n dances a little jig on the fallen corpse.", ch, 0, victim, TO_ROOM, 0);
		return retval;
	}

	WAIT_STATE(ch, DC::PULSE_VIOLENCE / 3);
	return eSUCCESS;
}

int song_healing_melody(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing a song of healing...");
	act("$n raises $s voice in a soothing melody...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_HEALING_MELODY - SKILL_SONG_BASE)
			break;

	(*i).song_timer = (song_info[(*i).song_number].beats() - (skill / 15));

	if (ch->getLevel() > MORTAL)
		(*i).song_timer = 1;

	return eSUCCESS;
}

int execute_song_healing_melody(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	int heal = 0;
	int combat, non_combat;
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_HEALING_MELODY - SKILL_SONG_BASE)
			break;
	}

	get_instrument_bonus(ch, combat, non_combat);

	if (GET_KI(ch) < 2)
	{
		ch->sendln("You don't have enough ki to continue singing.");
		return eSUCCESS;
	}
	GET_KI(ch) -= 2;

	for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
	{
		if (!ARE_GROUPED(ch, tmp_char))
			continue;

		heal = skill / 2 + ((GET_MAX_HIT(tmp_char) * 2) / 100) + (number(0, 20) - 10) + non_combat;
		if (heal < 5)
			heal = 5;

		if (IS_PC(tmp_char) && isSet(tmp_char->player->toggles, Player::PLR_DAMAGE))
		{
			if (tmp_char == ch)
			{
				ch->send(QStringLiteral("You feel your Healing Melody soothing %1 points of your health.\r\n").arg(heal));
			}
			else
			{
				csendf(tmp_char, "You feel %s's Healing Melody soothing %d points of your health.\r\n", GET_NAME(ch), heal);
			}
		}
		else
		{
			tmp_char->sendln("You feel a little better.");
		}

		tmp_char->addHP(heal);
	}

	if (!skill_success(ch, nullptr, SKILL_SONG_HEALING_MELODY))
	{
		ch->sendln("You run out of lyrics and end the song.");
		ch->songs.erase(i);
		return eEXTRA_VALUE;
	}
	if (ch->songs.size() > 1 && !skill_success(ch, nullptr, SKILL_ORCHESTRATE))
	{
		csendf(ch, "You miss a note, ruining your orchestration of %s!\r\n", qPrintable(Character::song_names.value((*i).song_number)));
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "$n misses a note, ruining $s orchestration of %s!", qPrintable(Character::song_names.value((*i).song_number)));
		act(buf, ch, 0, 0, TO_ROOM, 0);
		return eSUCCESS;
	}

	(*i).song_timer = (song_info[(*i).song_number].beats() - (skill / 15));

	if (ch->getLevel() > MORTAL)
		(*i).song_timer = 1;

	return eSUCCESS;
}

int song_revealing_stacato(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing a song of revealing...");
	act("$n begins to chant in rhythm...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_REVEAL_STACATO - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();

	return eSUCCESS;
}

int execute_song_revealing_stacato(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *i;
	Room *room;
	char buf[MAX_STRING_LENGTH];
	char *direction[] = {
		"to the North",
		"to the East",
		"to the South",
		"to the West",
		"above you",
		"below you",
		"\n",
	};

	std::vector<songInfo>::iterator k;

	for (k = ch->songs.begin(); k != ch->songs.end(); ++k)
	{
		if ((*k).song_number == SKILL_SONG_REVEAL_STACATO - SKILL_SONG_BASE)
			break;
	}
	for (i = DC::getInstance()->world[ch->in_room].people; i; i = i->next_in_room)
	{
		if (!ISSET(i->affected_by, AFF_HIDE) && !ISSET(i->affected_by, AFF_FOREST_MELD))
			continue;
		REMBIT(i->affected_by, AFF_HIDE);
		affect_from_char(i, AFF_FOREST_MELD);
		//      REMBIT(i->affected_by, AFF_FOREST_MELD);
		if (i == ch)
		{
			act("$n continues $s singing...", ch, 0, 0, TO_ROOM, 0);
			ch->sendln("Your singing ruins your hiding place.");
		}
		else
		{
			act("$n's song makes you notice $N hiding over in the corner.", ch, 0, i, TO_ROOM, NOTVICT);
			act("Your song makes you notice $N hiding over in the corner.", ch, 0, i, TO_CHAR, 0);
		}
	}

	if (skill > 80)
	{
		for (int j = 0; j < 6; j++)
		{
			if (CAN_GO(ch, j))
			{
				room = &DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[j]->to_room];
				if (room == &DC::getInstance()->world[ch->in_room] || isSet(room->room_flags, SAFE))
					continue;
				for (i = room->people; i; i = i->next_in_room)
				{
					if (!ISSET(i->affected_by, AFF_HIDE) && !ISSET(i->affected_by, AFF_FOREST_MELD))
						continue;
					REMBIT(i->affected_by, AFF_HIDE);
					affect_from_char(i, AFF_FOREST_MELD);
					sprintf(buf, "$n's song makes you notice $N hiding a little bit %s", direction[j]);
					act(buf, ch, 0, i, TO_ROOM, NOTVICT);
					sprintf(buf, "Your song makes you notice $N hiding a little bit %s", direction[j]);
					act(buf, ch, 0, i, TO_CHAR, 0);
				}
			}
		}
	}
	ch->sendln("You tap your foot along to the revealing staccato.");

	if (!skill_success(ch, nullptr, SKILL_SONG_REVEAL_STACATO))
	{
		ch->sendln("You run out of lyrics and end the song.");
		ch->songs.erase(k);
		return eEXTRA_VALUE;
	}
	if (ch->songs.size() > 1 && !skill_success(ch, nullptr, SKILL_ORCHESTRATE))
	{
		csendf(ch, "You miss a note, ruining your orchestration of %s!\r\n", qPrintable(Character::song_names.value((*k).song_number)));
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "$n misses a note, ruining $s orchestration of %s!", qPrintable(Character::song_names.value((*k).song_number)));
		act(buf, ch, 0, 0, TO_ROOM, 0);
		return eSUCCESS;
	}

	(*k).song_timer = song_info[(*k).song_number].beats();
	return eSUCCESS;
}

int song_note_of_knowledge(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_NOTE_OF_KNOWLEDGE - SKILL_SONG_BASE)
			break;

	// store the obj name here, cause A, we don't pass tar_obj
	// and B, there's no place to save it before we execute
	(*i).song_data = str_dup(arg);

	ch->sendln("You begin to sing a long single note...");
	act("$n sings a long solitary note.", ch, 0, 0, TO_ROOM, 0);
	(*i).song_timer = song_info[(*i).song_number].beats();

	return eSUCCESS;
}

int execute_song_note_of_knowledge(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Object *obj = nullptr;
	Character *vict = nullptr;
	Object *corpse = nullptr;
	char buf[MAX_STRING_LENGTH];
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_NOTE_OF_KNOWLEDGE - SKILL_SONG_BASE)
			break;
	}

	obj = get_obj_in_list((*i).song_data, ch->carrying);
	vict = ch->get_char_room_vis((*i).song_data);
	corpse = get_obj_in_list_vis(ch, (*i).song_data, DC::getInstance()->world[ch->in_room].contents);
	if (corpse && (GET_ITEM_TYPE(corpse) != ITEM_CONTAINER || corpse->obj_flags.value[3] != 1))
		corpse = nullptr;

	delete[] (*i).song_data;
	(*i).song_data = 0;

	if (obj)
	{
		spell_identify(ch->getLevel(), ch, 0, obj, 0);
	}
	else if (skill > 80 && corpse)
	{
		ch->sendln(QStringLiteral("Corpse '%1'").arg(corpse->Name()));
		spell_identify(ch->getLevel(), ch, 0, corpse, 0);
	}
	else if (skill > 85 && vict)
	{
		spell_identify(ch->getLevel(), ch, vict, 0, 0);
	}
	else
		ch->sendln("You can't seem to find that item.");
	return eSUCCESS;
}

int song_terrible_clef(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin a song of battle!");
	act("$n sings a horrible battle hymn!", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_TERRIBLE_CLEF - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();
	return eSUCCESS;
}

int execute_song_terrible_clef(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	int dam = 0;
	int retval;
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_TERRIBLE_CLEF - SKILL_SONG_BASE)
			break;
	}

	victim = ch->fighting;

	if (!victim)
	{
		ch->sendln("Your song fades outside of battle.");
		return eSUCCESS;
	}

	int combat, non_combat;
	get_instrument_bonus(ch, combat, non_combat);

	dam = ((ch->has_skill(SKILL_SONG_TERRIBLE_CLEF)) * 1.5 + 225);
	if (number(1, 100) < get_saves(victim, SAVE_TYPE_MAGIC))
	{
		act("$N resists your terrible clef!", ch, nullptr, victim, TO_CHAR, 0);
		act("$N resists $n's terrible clef!", ch, nullptr, victim, TO_ROOM, NOTVICT);
		act("You resist $n's terrible clef!", ch, nullptr, victim, TO_VICT, 0);
		dam /= 2;
	}
	char buf[MAX_STRING_LENGTH];
	strcpy(buf, victim->short_desc);
	retval = damage(ch, victim, dam, TYPE_SONG, SKILL_SONG_TERRIBLE_CLEF);
	if (isSet(retval, eCH_DIED))
		return retval;
	if (isSet(retval, eVICT_DIED))
	{
		ch->sendln("You dance a small jig on the corpse.");
		act("$n dances a little jig on the fallen corpse.", ch, 0, victim, TO_ROOM, 0);
		return retval;
	}

	if (!skill_success(ch, victim, SKILL_SONG_TERRIBLE_CLEF))
	{
		ch->sendln("You run out of lyrics and end the song.");
		ch->songs.erase(i);
		return eEXTRA_VALUE;
	}

	if (ch->songs.size() > 1 && !skill_success(ch, nullptr, SKILL_ORCHESTRATE))
	{
		csendf(ch, "You miss a note, ruining your orchestration of %s!\r\n", qPrintable(Character::song_names[(*i).song_number]));
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "$n misses a note, ruining $s orchestration of %s!", qPrintable(Character::song_names[(*i).song_number]));
		act(buf, ch, 0, 0, TO_ROOM, 0);
		return eSUCCESS;
	}

	(*i).song_timer = song_info[(*i).song_number].beats();
	return eSUCCESS;
}

int song_listsongs(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	char buf[200];

	ch->sendln("Available Songs\r\n---------------");
	for (qsizetype i = 0; i < Character::song_names.length(); i++)
	{
		if (!ch->isImmortalPlayer() && !ch->has_skill(song_info[i].skill_num()))
			continue;

		sprintf(buf, " %-50s    %d ki\r\n", qPrintable(Character::song_names.value(i)), song_info[i].min_useski());
		ch->send(buf);
	}
	return eSUCCESS;
}

int song_soothing_remembrance(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing a song of rememberance...");
	act("$n raises $s voice in a soothing ballad...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_SOOTHING_REMEM - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats() - skill / 18;
	return eSUCCESS;
}

int execute_song_soothing_remembrance(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	int heal = 0;
	char buf[MAX_STRING_LENGTH];
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_SOOTHING_REMEM - SKILL_SONG_BASE)
			break;
	}

	if (GET_KI(ch) < 3)
	{
		ch->sendln("You don't have enough ki to continue singing.");
		return eSUCCESS;
	}
	GET_KI(ch) -= 3;

	int combat, non_combat;
	get_instrument_bonus(ch, combat, non_combat);

	for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
	{
		if (!ARE_GROUPED(ch, tmp_char))
			continue;

		heal = skill / 2 + ((GET_MAX_MANA(tmp_char) * 2) / 100) + (number(0, 20) - 10) + non_combat;
		if (heal < 5)
			heal = 5;

		if (IS_PC(tmp_char) && isSet(tmp_char->player->toggles, Player::PLR_DAMAGE))
		{
			if (tmp_char == ch)
				sprintf(buf, "You feel your Soothing Rememberance revitalize %d points of your mana.\r\n", heal);
			else
				sprintf(buf, "You feel %s's Soothing Rememberance revitalize %d points of your mana.\r\n", GET_NAME(ch), heal);
			tmp_char->send(buf);
		}
		else
			tmp_char->sendln("You feel soothed.");

		GET_MANA(tmp_char) += heal;
		if (GET_MANA(tmp_char) > GET_MAX_MANA(tmp_char))
			GET_MANA(tmp_char) = GET_MAX_MANA(tmp_char);
	}

	if (!skill_success(ch, nullptr, SKILL_SONG_SOOTHING_REMEM))
	{
		ch->sendln("You run out of lyrics and end the song.");
		ch->songs.erase(i);
		return eEXTRA_VALUE;
	}
	if (ch->songs.size() > 1 && !skill_success(ch, nullptr, SKILL_ORCHESTRATE))
	{
		csendf(ch, "You miss a note, ruining your orchestration of %s!\r\n", qPrintable(Character::song_names.value((*i).song_number)));
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "$n misses a note, ruining $s orchestration of %s!", qPrintable(Character::song_names.value((*i).song_number)));
		act(buf, ch, 0, 0, TO_ROOM, 0);
		return eSUCCESS;
	}

	(*i).song_timer = song_info[(*i).song_number].beats() - skill / 18;

	return eSUCCESS;
}

int song_traveling_march(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing a song of travel...");
	act("$n raises $s voice in an uplifting march...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_TRAVELING_MARCH - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats() - (skill / 15);

	if (ch->getLevel() > MORTAL)
		(*i).song_timer = 1;

	return eSUCCESS;
}

int execute_song_traveling_march(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	int heal;
	struct affected_type af;
	char buf[MAX_STRING_LENGTH];

	int combat, non_combat;
	get_instrument_bonus(ch, combat, non_combat);

	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_TRAVELING_MARCH - SKILL_SONG_BASE)
			break;
	}

	if (GET_KI(ch) == 0)
	{
		ch->sendln("You don't have enough ki to continue singing.");
		return eSUCCESS;
	}

	GET_KI(ch) -= 1;

	af.type = SKILL_SONG_TRAVELING_MARCH;
	af.modifier = -10 - ch->has_skill(SKILL_SONG_TRAVELING_MARCH) / 3;
	af.duration = 1;
	af.location = APPLY_AC;
	af.bitvector = -1;
	af.caster = GET_NAME(ch);

	for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
	{
		if (!ARE_GROUPED(ch, tmp_char))
			continue;

		heal = skill / 2 + ((GET_MAX_MOVE(tmp_char) * 2) / 100) + (number(0, 20) - 10) + non_combat;
		if (heal < 5)
			heal = 5;

		if (IS_PC(tmp_char) && isSet(tmp_char->player->toggles, Player::PLR_DAMAGE))
		{
			if (tmp_char == ch)
			{
				tmp_char->send(QStringLiteral("You feel your Travelling March recover %1 moves for you.\r\n").arg(heal));
			}
			else
			{
				sprintf(buf, "You feel %s's Travelling March recovering %d moves for you.\r\n", GET_NAME(ch), heal);
				tmp_char->send(buf);
			}
		}
		else
			tmp_char->sendln("Your feet feel lighter.");

		tmp_char->incrementMove(heal);
	}

	if (!skill_success(ch, nullptr, SKILL_SONG_TRAVELING_MARCH))
	{
		ch->sendln("You run out of lyrics and end the song.");
		ch->songs.erase(i);
		return eEXTRA_VALUE;
	}
	if (ch->songs.size() > 1 && !skill_success(ch, nullptr, SKILL_ORCHESTRATE))
	{
		csendf(ch, "You miss a note, ruining your orchestration of %s!\r\n", qPrintable(Character::song_names.value((*i).song_number)));
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "$n misses a note, ruining $s orchestration of %s!", qPrintable(Character::song_names.value((*i).song_number)));
		act(buf, ch, 0, 0, TO_ROOM, 0);
		return eSUCCESS;
	}

	(*i).song_timer = song_info[(*i).song_number].beats() - (skill / 15);

	if (ch->getLevel() > MORTAL)
		(*i).song_timer = 1;
	return eSUCCESS;
}

int song_stop(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	if (origsing)
		return eFAILURE;
	if (ch->songs.empty())
	{
		ch->sendln("Might wanna start the performance first...Hope this isn't indicative of your love life...");
		return eFAILURE;
	}

	std::vector<songInfo>::iterator i;

	if (*arg)
	{ // sing 'stop' <song>
		int spl = old_search_block(arg, 0, strlen(arg), Character::song_names, 0);
		spl--; /* songs goes from 0+ not 1+ like spells */

		std::vector<songInfo>::iterator i;

		for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		{
			if (spl == (*i).song_number)
			{
				if ((song_info[(*i).song_number].intrp_pointer()))
					((*song_info[(*i).song_number].intrp_pointer())(ch->getLevel(), ch, nullptr, nullptr, -1));
				ch->songs.erase(i);
				--i;
			}
			if ((*i).song_number == 2)
			{ // get rid of song stop
				ch->songs.erase(i);
				--i;
			}
		}
	}
	else
	{
		for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
			if ((song_info[(*i).song_number].intrp_pointer()))
				((*song_info[(*i).song_number].intrp_pointer())(ch->getLevel(), ch, nullptr, nullptr, -1));
		ch->songs.clear();
	}

	ch->skill_increase_check(SKILL_SONG_STOP, ch->has_skill(song_info[SKILL_SONG_STOP - SKILL_SONG_BASE].skill_num()), SKILL_INCREASE_EASY);

	ch->sendln("You finish off your music with a flourish...");
	act("$n finishes $s music in a flourish and a bow.", ch, 0, 0, TO_ROOM, 0);

	return eSUCCESS;
}

int song_astral_chanty(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing an astral chanty...");
	act("$n starts quietly in a sea chanty...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_ASTRAL_CHANTY - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats() - (skill / 10);

	// Store it for later, since we can't store the vict pointer
	(*i).song_data = str_dup(arg);
	return eSUCCESS;
}

void do_astral_chanty_movement(Character *victim, Character *target)
{
	int retval;

	if (!victim || !target)
	{
		DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "do_astral_chanty_movement: nullptr pointer passed.");
		produce_coredump();
		return;
	}

	if (isSet(DC::getInstance()->world[target->in_room].room_flags, PRIVATE) ||
		isSet(DC::getInstance()->world[target->in_room].room_flags, IMP_ONLY) ||
		isSet(DC::getInstance()->world[target->in_room].room_flags, NO_PORTAL))
	{
		victim->sendln("Your astral travels fail to find your destination.");
		return;
	}

	if ((IS_PC(target)) && (target->getLevel() >= IMMORTAL))
	{
		victim->sendln("Just who do you think you are?");
		return;
	}

	if (IS_AFFECTED(target, AFF_SHADOWSLIP))
	{
		victim->sendln("Something seems to block your astral travel to this target.");
		return;
	}

	if (victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
	{
		victim->sendln("Your attempt to transport stolen goods through the astral planes fails!");
		return;
	}

	Character *tmpch;

	for (tmpch = DC::getInstance()->world[target->in_room].people; tmpch; tmpch = tmpch->next_in_room)
		if (search_char_for_item(tmpch, real_object(76), false) || search_char_for_item(tmpch, real_object(51), false))
		{
			victim->sendln("Your astral travels fail to find your destination.");
			return;
		}

	retval = move_char(victim, target->in_room);

	if (!isSet(retval, eSUCCESS))
	{
		victim->sendln("Mystic winds shock you back into your old reality.");
		act("$n shudders as magical reality refuses to set in.", victim, 0, 0, TO_ROOM, 0);
		WAIT_STATE(victim, DC::PULSE_VIOLENCE * 3);
		return;
	}

	do_look(victim, "");
	WAIT_STATE(victim, DC::PULSE_VIOLENCE);
	act("$n appears out of nowhere in a chorus of light and song.", victim, 0, 0, TO_ROOM, 0);
}

int execute_song_astral_chanty(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	int status = 0;
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_ASTRAL_CHANTY - SKILL_SONG_BASE)
			break;
	}

	victim = get_char((*i).song_data);

	if (!victim)
	{
		ch->sendln("You cannot seem to accurately focus your song.");
		status = eFAILURE;
	}
	else if (victim->getLevel() > ch->getLevel())
	{
		ch->sendln("Your target resists the song's draw.");
		status = eFAILURE;
	}
	else if (isSet(DC::getInstance()->world[victim->in_room].room_flags, NO_PORTAL) ||
			 DC::getInstance()->zones.value(DC::getInstance()->world[victim->in_room].zone).isNoTeleport() ||
			 victim->room().isArena())
	{
		ch->sendln("A mystical force seems to be keeping you out.");
		status = eFAILURE;
	}
	else
	{

		Character *tmpch;

		for (tmpch = DC::getInstance()->world[victim->in_room].people; tmpch; tmpch = tmpch->next_in_room)
			if (search_char_for_item(tmpch, real_object(51), false))
			{
				ch->sendln("$B$1Phire whispers, 'You had to know I wouldn't make it THAT easy now didn't you? You're just going to have to walk!$R");
				status = eFAILURE;
				break;
			}

		if (status != eFAILURE)
		{
			// Additional costs for astral chanty across continents
			if (DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).continent != DC::getInstance()->zones.value(DC::getInstance()->world[victim->in_room].zone).continent)
			{
				if (GET_KI(ch) < use_song(ch, SKILL_SONG_ASTRAL_CHANTY - SKILL_SONG_BASE))
				{
					ch->sendln("You don't posses the energy to travel that far.");
					GET_KI(ch) += use_song(ch, SKILL_SONG_ASTRAL_CHANTY - SKILL_SONG_BASE);

					// free our stored char name
					if ((*i).song_data)
					{
						delete[] (*i).song_data;
						(*i).song_data = 0;
					}

					return eFAILURE;
				}
				else
				{
					ch->sendln("The long distance drains additional ki from you.");
					GET_KI(ch) -= use_song(ch, SKILL_SONG_ASTRAL_CHANTY - SKILL_SONG_BASE);
				}
			}

			Character *next_char = 0;
			for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = next_char)
			{
				next_char = tmp_char->next_in_room;
				if (!ARE_GROUPED(ch, tmp_char))
					continue;

				do_astral_chanty_movement(tmp_char, victim);
			}

			ch->sendln("Your song completes, and your vision fades.");
			act("$n's voice fades off into the ether.", ch, 0, 0, TO_ROOM, 0);
			status = eSUCCESS;
		}
	}

	// free our stored char name
	if ((*i).song_data)
	{
		delete[] (*i).song_data;
		(*i).song_data = 0;
	}

	return status;
}

int pulse_song_astral_chanty(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	if (number(1, 3) == 3)
		act("$n sings a rousing chanty!", ch, 0, 0, TO_ROOM, 0);

	return eSUCCESS;
}

int song_forgetful_rhythm(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing a song of forgetfulness...");
	act("$n begins an entrancing rhythm...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_FORGETFUL_RHYTHM - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats() - skill / 15;

	// store the arg here, cause there's no place to save it before we execute
	(*i).song_data = str_dup(arg);

	return eSUCCESS;
}

int execute_song_forgetful_rhythm(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	int retval;
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_FORGETFUL_RHYTHM - SKILL_SONG_BASE)
			break;
	}

	if (!(victim = ch->get_char_room_vis((*i).song_data)))
	{
		ch->sendln("You don't see that person here.");
		delete[] (*i).song_data;
		(*i).song_data = 0;
		return eFAILURE;
	}
	delete[] (*i).song_data;
	(*i).song_data = 0;

	act("$n sings to $N about beautiful rainbows.", ch, 0, victim, TO_ROOM, NOTVICT);

	if (IS_PC(victim))
	{
		ch->sendln("They seem to be looking at you really strangly.");
		victim->sendln("You are sung to about butterflies and bullfrogs.");
		return eSUCCESS;
	}

	if (number(0, 1))
	{
		// monster forgets hate/fear/track list
		victim->sendln("Hrm.....who were you mad at again??");
		ch->sendln("You have soothed the savage beast.");
		remove_memory(victim, 'h');
		remove_memory(victim, 'f');
		remove_memory(victim, 't');
	}
	else
	{
		// Die bard!
		ch->sendln("Uh oh.");
		do_say(victim, "Die you spoony bard!");
		retval = attack(victim, ch, TYPE_UNDEFINED);
		retval = SWAP_CH_VICT(retval);
		return retval;
	}
	return eSUCCESS;
}

int song_shattering_resonance(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing a song of shattering...");
	act("$n begins a fading resonance...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_SHATTERING_RESO - SKILL_SONG_BASE)
			break;

	if (ch->getLevel() > 49)
		(*i).song_timer = (song_info[(*i).song_number].beats() - 1);
	else
		(*i).song_timer = song_info[(*i).song_number].beats();

	// store the arg here, cause there's no place to save it before we execute
	(*i).song_data = str_dup(arg);

	return eSUCCESS;
}

int execute_song_shattering_resonance(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Object *obj = nullptr;
	Object *tobj = nullptr;
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_SHATTERING_RESO - SKILL_SONG_BASE)
			break;
	}

	if (!(obj = get_obj_in_list((*i).song_data, DC::getInstance()->world[ch->in_room].contents)))
	{
		ch->sendln("You don't see that object here.");
		delete[] (*i).song_data;
		(*i).song_data = 0;
		return eFAILURE;
	}
	delete[] (*i).song_data;
	(*i).song_data = 0;

	// code to shatter a beacon
	if (GET_ITEM_TYPE(obj) == ITEM_BEACON)
	{
		act("$n's song fades to an end.", ch, 0, 0, TO_ROOM, 0);
		if (!obj->equipped_by)
		{
			// Someone load it or something?
			ch->sendln("The magic fades away back to the ether.");
			act("$p fades away gently.", ch, obj, 0, TO_ROOM, INVIS_NULL);
		}
		else
		{
			ch->sendln("The magic is shattered by your will!");
			act("$p blinks out of existence with a bang!", ch, obj, 0, TO_ROOM, INVIS_NULL);
			obj->equipped_by->sendln("Your magic beacon is shattered!");
			obj->equipped_by->beacon = nullptr;
			obj->equipped_by = nullptr;
		}
		extract_obj(obj);
		return eSUCCESS;
	}

	// make sure the obj is a player portal
	if (!obj->isPortal())
	{
		ch->sendln("You can't shatter that!");
		return eFAILURE;
	}
	if (!isexact("pcportal", obj->Name()))
	{
		ch->sendln("The portal resists your song.");
		return eFAILURE;
	}

	act("$n's song fades to an end.", ch, 0, 0, TO_ROOM, 0);

	// determine chance of destroying it
	if (number(0, 1)) // 50/50 for now
	{
		ch->sendln("The portal resists your song.");
		return eFAILURE;
	}

	send_to_room("You hear a loud shattering sound of magic discharging and the portal fades away.\r\n", obj->in_room);
	// we remove it from the room, in case the other portal is also in the same room
	// we extract both portals at the end
	obj_from_room(obj);

	// find it's match
	if (!(tobj = get_obj_in_list("pcportal", DC::getInstance()->world[real_room(obj->obj_flags.value[0])].contents)))
	{
		ch->sendln("Could not find matching exit portal? Tell an Immortal.");
		return eFAILURE;
	}

	// destroy it
	send_to_room("You hear a loud shattering sound of magic discharging and the portal fades away.\r\n", tobj->in_room);
	extract_obj(obj);
	extract_obj(tobj);
	return eSUCCESS;
}

int song_insane_chant(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin chanting insanely...");
	act("$n begins chanting wildly...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_INSANE_CHANT - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();
	return eSUCCESS;
}

int execute_song_insane_chant(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	struct affected_type af;

	af.type = SKILL_INSANE_CHANT;
	af.duration = 1;
	af.modifier = 0;
	af.location = APPLY_INSANE_CHANT;
	af.bitvector = -1;
	af.caster = GET_NAME(ch);

	act("$n's singing starts to drive you INSANE!!!", ch, 0, 0, TO_ROOM, 0);
	ch->sendln("Your singing drives everyone around you INSANE!!!");

	for (victim = DC::getInstance()->world[ch->in_room].people; victim && victim != ch; victim = victim->next_in_room)
	{
		// don't effect gods unless it was a higher level god singing
		if (victim->getLevel() >= IMMORTAL && ch->getLevel() <= victim->getLevel())
			continue;

		if (number(1, 100) < get_saves(victim, SAVE_TYPE_POISON))
		{
			act("$N resists your insane chant!", ch, nullptr, victim, TO_CHAR, 0);
			act("$N resists $n's insane chant!", ch, nullptr, victim, TO_ROOM, NOTVICT);
			act("You resist $n's insane chant!", ch, nullptr, victim, TO_VICT, 0);
			continue;
		}

		if (victim->affected_by_spell(SKILL_INSANE_CHANT))
		{
			affect_from_char(victim, SKILL_INSANE_CHANT);
		}

		affect_to_char(victim, &af);
	}
	return eSUCCESS;
}

int song_flight_of_bee(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing a lofty song...");
	act("$n raises $s voice in an flighty quick march...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_FLIGHT_OF_BEE - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();

	return eSUCCESS;
}

int execute_song_flight_of_bee(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	struct affected_type af;

	af.type = SKILL_SONG_FLIGHT_OF_BEE;
	af.duration = 1 + skill / 10;
	af.modifier = 0;
	af.location = 0;
	af.caster = GET_NAME(ch);
	af.bitvector = AFF_FLYING;

	for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
	{
		if (!ARE_GROUPED(ch, tmp_char))
			continue;

		if (tmp_char->affected_by_spell(SPELL_FLY))
		{
			affect_from_char(tmp_char, SPELL_FLY);
			tmp_char->send("Your fly spell dissipates.");
		}

		if (tmp_char->affected_by_spell(SKILL_SONG_FLIGHT_OF_BEE))
		{
			affect_from_char(tmp_char, SKILL_SONG_FLIGHT_OF_BEE);
		}
		affect_to_char(tmp_char, &af);

		tmp_char->sendln("Your feet feel like air.");
	}

	return eSUCCESS;
}

int song_searching_song(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("Your voice raises sending out a song to search the lands...");
	act("$n raises $s voice sending out a song to search the lands....", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_SEARCHING_SONG - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();

	// store the char name here, cause A, we don't pass tar_char
	// and B, there's no place to save it before we execute
	(*i).song_data = str_dup(arg);

	return eSUCCESS;
}

int execute_song_searching_song(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *target = nullptr;
	char buf[200];
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_SEARCHING_SONG - SKILL_SONG_BASE)
			break;
	}

	target = get_char((*i).song_data);

	delete[] (*i).song_data;
	(*i).song_data = 0;

	act("$n's song ends and quietly fades away.", ch, 0, 0, TO_ROOM, 0);

	if (!target || ch->getLevel() < target->getLevel())
	{
		ch->sendln("Your song fades away, its search unfinished.");
		return eFAILURE;
	}
	if (target->affected_by_spell(SKILL_INNATE_EVASION) || isSet(DC::getInstance()->world[target->in_room].room_flags, NO_KI))
	{
		ch->sendln("Something blocks your vision.");
		return eFAILURE;
	}

	snprintf(buf, 200, "Your song finds %s ", GET_SHORT(target));

	switch (GET_POS(target))
	{
	case position_t::STUNNED:
		sprintf(buf, "%s%s at ", buf, "on the ground, stunned");
		break;
	case position_t::DEAD:
		sprintf(buf, "%s%s at ", buf, "lying dead");
		break;
	case position_t::STANDING:
		sprintf(buf, "%s%s at ", buf, "standing around");
		break;
	case position_t::SITTING:
		sprintf(buf, "%s%s at ", buf, "sitting");
		break;
	case position_t::RESTING:
		sprintf(buf, "%s%s at ", buf, "resting");
		break;
	case position_t::SLEEPING:
		sprintf(buf, "%s%s at ", buf, "sleeping");
		break;
	case position_t::FIGHTING:
		sprintf(buf, "%s%s at ", buf, "fighting");
		break;
	default:
		sprintf(buf, "%s%s at ", buf, "masturbating");
		break;
	}

	if (target->affected_by_spell(SPELL_DETECT_MAGIC) && target->affected_by_spell(SPELL_DETECT_MAGIC)->modifier > 80)
		target->sendln("You sense you are the target of scrying.");

	sprintf(buf, "%s%s.\r\n", buf, DC::getInstance()->world[target->in_room].name);
	ch->send(buf);
	return eSUCCESS;
}

int song_jig_of_alacrity(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing a quick little jig of alacrity...");
	act("$n starts humming a quick little ditty...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_JIG_OF_ALACRITY - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();

	return eSUCCESS;
}

int song_fanatical_fanfare(uint8_t level, Character *ch, char *Aag, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing loudly, and poke everyone in your surroundings with a stick..");
	act("$n starts singing loudly, and begins to poke everyone around $m with a stick. Hey!", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_FANATICAL_FANFARE - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();

	return eSUCCESS;
}
int song_summon_song(uint8_t level, Character *ch, char *Aag, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin an inappropriately bawdy tune of your intimacy with pets.");
	act("$n begins an inappropriately bawdy tune of $s intimacy with pets.", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_SUMMONING_SONG - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats() - (skill / 10);

	return eSUCCESS;
}
int execute_song_summon_song(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	bool summoned = false;
	follow_type *fvictim = nullptr;

	if (ch->followers)
		for (fvictim = ch->followers; fvictim; fvictim = fvictim->next)
		{
			if (IS_AFFECTED(fvictim->follower, AFF_CHARM) && IS_NPC(fvictim->follower) && ch->in_room != fvictim->follower->in_room)
			{
				summoned = true;
				do_emote(fvictim->follower, "disappears in a flash of $B$6m$4u$1l$7t$4i$7-$6c$4o$1l$6o$7r$4e$1d$R (disco?) light.\r\n");
				move_char(fvictim->follower, ch->in_room);
				act("With a $B$6m$4u$1l$7t$4i$7-$6c$4o$1l$6o$7r$4e$1d$R flash of (disco?) light $n appears!", fvictim->follower, 0, 0, TO_ROOM, 0);
			}
		}
	if (false == summoned)
	{
		ch->sendln("You don't have any followers to summon. You are sad. :(");
		act("$n hangs $s head in disappointment and surreptitiously wipes away a tear.", ch, 0, 0, TO_ROOM, 0);
	}
	return eSUCCESS;
}
int song_mking_charge(uint8_t level, Character *ch, char *Aag, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You inspire your allies with your rousing songs about rising against oppression!");
	act("$n starts singing songs about former glory and past victories, rousing $s allies!", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_MKING_CHARGE - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();

	return eSUCCESS;
}

int execute_song_jig_of_alacrity(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	// Note, the jig effects everyone in the group BUT the bard.
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_JIG_OF_ALACRITY - SKILL_SONG_BASE)
			break;
	}

	if (GET_KI(ch) < 2)
	{ // we don't have the ki to keep the song going
		return intrp_jig_of_alacrity(level, ch, arg, victim, -1);
	}

	affected_type af;
	af.type = SKILL_SONG_JIG_OF_ALACRITY;
	af.duration = 1;
	af.modifier = 0;
	af.location = 0;
	af.caster = GET_NAME(ch);
	af.bitvector = AFF_HASTE;

	for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
	{
		if (!ARE_GROUPED(ch, tmp_char))
			continue;
		if (tmp_char == ch)
			continue;

		if (tmp_char->affected_by_spell(SPELL_HASTE))
		{
			affect_from_char(tmp_char, SPELL_HASTE);
			tmp_char->sendln("Your limbs slow back to normal.");
		}

		if (tmp_char->affected_by_spell(SKILL_SONG_JIG_OF_ALACRITY))
		{
			affect_from_char(tmp_char, SKILL_SONG_JIG_OF_ALACRITY);
		}

		if (!tmp_char->affected_by_spell(SKILL_SONG_JIG_OF_ALACRITY))
		{
			affect_to_char(tmp_char, &af);
			tmp_char->sendln("Your dance quickens your pulse!");
		}
	}

	if (!skill_success(ch, nullptr, SKILL_SONG_JIG_OF_ALACRITY))
	{
		(*i).song_timer = -1;
		return eSUCCESS;
	}
	if (ch->songs.size() > 1 && !skill_success(ch, nullptr, SKILL_ORCHESTRATE))
	{
		(*i).song_timer = -1;
		csendf(ch, "You miss a note, ruining your orchestration of %s!\r\n", qPrintable(Character::song_names.value((*i).song_number)));
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "$n misses a note, ruining $s orchestration of %s!", qPrintable(Character::song_names.value((*i).song_number)));
		act(buf, ch, 0, 0, TO_ROOM, 0);
		return eSUCCESS;
	}

	GET_KI(ch) -= 2;

	(*i).song_timer = song_info[(*i).song_number].beats() + (ch->getLevel() > 33) + (ch->getLevel() > 43);

	return eSUCCESS;
}

int execute_song_fanatical_fanfare(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	struct affected_type af1, af2, af3;

	af1.type = SKILL_SONG_FANATICAL_FANFARE;
	af1.duration = skill / 30;
	af1.modifier = 0;
	af1.location = 0;
	af1.bitvector = AFF_INSOMNIA;
	af1.caster = GET_NAME(ch);

	af2.type = SKILL_SONG_FANATICAL_FANFARE;
	af2.duration = skill / 30;
	af2.modifier = 0;
	af2.location = 0;
	af2.bitvector = AFF_FEARLESS;
	af2.caster = GET_NAME(ch);

	af3.type = SKILL_SONG_FANATICAL_FANFARE;
	af3.duration = skill / 30;
	af3.modifier = 0;
	af3.location = 0;
	af3.bitvector = AFF_NO_PARA;
	af3.caster = GET_NAME(ch);

	for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
	{
		if (!ARE_GROUPED(ch, tmp_char))
			continue;

		if (tmp_char->affected_by_spell(SKILL_SONG_FANATICAL_FANFARE))
		{
			tmp_char->sendln("You manage to get far enough away to avoid being poked again!");
			continue;
		}

		if (tmp_char->affected_by_spell(SPELL_INSOMNIA))
		{
			affect_from_char(tmp_char, SPELL_INSOMNIA);
			tmp_char->sendln("Your mind returns to its normal state.");
		}

		affect_to_char(tmp_char, &af1);
		if (skill > 85)
			affect_to_char(tmp_char, &af2);
		if (skill > 90)
			affect_to_char(tmp_char, &af3);

		if (ch == tmp_char)
			tmp_char->sendln("Your song causes your mind to race at a thousand miles an hour!");
		else
			csendf(tmp_char, "%s's song causes your mind to race at a thousand miles an hour!\r\n", GET_NAME(ch));
	}

	return eSUCCESS;
}

int execute_song_mking_charge(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_MKING_CHARGE - SKILL_SONG_BASE)
			break;
	}

	if (GET_KI(ch) < 5) // we don't have the ki to keep the song going
	{
		return intrp_mking_charge(level, ch, arg, victim, -1);
	}

	struct affected_type af;
	af.type = SKILL_SONG_MKING_CHARGE;
	af.duration = 1;
	af.modifier = 0;
	af.location = 0;
	af.caster = GET_NAME(ch);

	if (skill > 80)
		af.bitvector = AFF_HASTE;
	else
		af.bitvector = -1;

	for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
	{
		if (!ARE_GROUPED(ch, tmp_char))
			continue;

		if (tmp_char->affected_by_spell(SKILL_SONG_MKING_CHARGE))
		{
			affect_from_char(tmp_char, SKILL_SONG_MKING_CHARGE);
			tmp_char->sendln("You lose the inspiration.");
		}

		if (!tmp_char->affected_by_spell(SKILL_SONG_MKING_CHARGE))
		{
			affect_to_char(tmp_char, &af);

			if (ch == tmp_char)
			{
				tmp_char->sendln("Your songs and tales fuel you with rage, sending you into a temporary frenzy!  To arms!");
				if (af.bitvector == AFF_HASTE)
				{
					ch->sendln("Weaving the jig of alacrity into your song of battle, your allies move faster.");
				}
			}
			else
			{
				act("$n's songs and tales of your ancestors' struggles fuel you with rage, sending you into a temporary frenzy!  To arms!", ch, 0, tmp_char,
					TO_VICT, 0);
			}
		}
	}

	if (!skill_success(ch, nullptr, SKILL_SONG_MKING_CHARGE))
	{
		(*i).song_timer = -1;
		return eSUCCESS;
	}
	if (ch->songs.size() > 1 && !skill_success(ch, nullptr, SKILL_ORCHESTRATE))
	{
		(*i).song_timer = -1;
		csendf(ch, "You miss a note, ruining your orchestration of %s!\r\n", qPrintable(Character::song_names.value((*i).song_number)));
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "$n misses a note, ruining $s orchestration of %s!", qPrintable(Character::song_names.value((*i).song_number)));
		act(buf, ch, 0, 0, TO_ROOM, 0);
		return eSUCCESS;
	}

	GET_KI(ch) -= 5;

	(*i).song_timer = song_info[(*i).song_number].beats();
	return eSUCCESS;
}

/*int pulse_song_fanatical_fanfare(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
 {
 if (number(1,5) == 2)
 act("$n combines singing and poking the people with a stick, getting people worked up.", ch, 0, 0, TO_ROOM,0);
 return eSUCCESS;
 }*/

int pulse_mking_charge(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	return eSUCCESS;
}

int pulse_jig_of_alacrity(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	if (number(1, 5) == 3)
		act("$n prances around like a fairy.", ch, 0, 0, TO_ROOM, 0);
	return eSUCCESS;
}

int intrp_jig_of_alacrity(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *master = nullptr;
	follow_type *fvictim = nullptr;

	if (ch->master && ISSET(ch->affected_by, AFF_GROUP))
		master = ch->master;
	else
		master = ch;

	for (fvictim = master->followers; fvictim; fvictim = fvictim->next)
	{
		if (origsing && origsing != fvictim->follower)
			continue;
		if (ISSET(fvictim->follower->affected_by, AFF_HASTE) && !fvictim->follower->affected_by_spell(SPELL_HASTE))
		{
			REMBIT(fvictim->follower->affected_by, AFF_HASTE);
			fvictim->follower->sendln("Your limbs slow back to normal.");
		}
	}

	if (!origsing || origsing == master)
		if (ISSET(master->affected_by, AFF_HASTE) && !master->affected_by_spell(SPELL_HASTE))
		{
			REMBIT(master->affected_by, AFF_HASTE);
			master->sendln("Your limbs slow back to normal.");
		}
	return eSUCCESS;
}

int intrp_song_fanatical_fanfare(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *master = nullptr;
	follow_type *fvictim = nullptr;

	if (ch->master && ISSET(ch->affected_by, AFF_GROUP))
		master = ch->master;
	else
		master = ch;
	for (fvictim = master->followers; fvictim; fvictim = fvictim->next)
	{
		if (origsing && origsing != fvictim->follower)
			continue;
		if (ISSET(fvictim->follower->affected_by, AFF_INSOMNIA) && !fvictim->follower->affected_by_spell(SPELL_INSOMNIA))
		{
			REMBIT(fvictim->follower->affected_by, AFF_INSOMNIA);
			fvictim->follower->sendln("Your mind returns to its normal state.");
		}
		if (IS_AFFECTED(fvictim->follower, AFF_FEARLESS))
			REMBIT(fvictim->follower->affected_by, AFF_FEARLESS);
		if (IS_AFFECTED(fvictim->follower, AFF_NO_PARA))
			REMBIT(fvictim->follower->affected_by, AFF_NO_PARA);
	}

	if (!origsing || origsing == master)
		if (ISSET(master->affected_by, AFF_INSOMNIA) && !master->affected_by_spell(SPELL_INSOMNIA))
		{
			REMBIT(master->affected_by, AFF_INSOMNIA);
			master->sendln("Your mind returns to its normal state.");
		}
	if (IS_AFFECTED(master, AFF_FEARLESS))
		REMBIT(master->affected_by, AFF_FEARLESS);
	if (IS_AFFECTED(master, AFF_NO_PARA))
		REMBIT(master->affected_by, AFF_NO_PARA);

	return eSUCCESS;
}
int intrp_mking_charge(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *master = nullptr;
	follow_type *fvictim = nullptr;

	if (ch->master && ISSET(ch->affected_by, AFF_GROUP))
		master = ch->master;
	else
		master = ch;

	for (fvictim = master->followers; fvictim; fvictim = fvictim->next)
	{
		if (origsing && origsing != fvictim->follower)
			continue;
		if (fvictim->follower->affected_by_spell(SKILL_SONG_MKING_CHARGE))
		{
			affect_from_char(fvictim->follower, SKILL_SONG_MKING_CHARGE);
			fvictim->follower->sendln("You lose the inspiration.");
		}
	}

	if (!origsing || origsing == master)
		if (master->affected_by_spell(SKILL_SONG_MKING_CHARGE))
		{
			affect_from_char(master, SKILL_SONG_MKING_CHARGE);
			master->sendln("You lose the inspiration.");
		}
	return eSUCCESS;
}

int song_glitter_dust(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You throw dust in the air and sing a wily ditty...");
	act("$n throws some dust in the air and sings a wily ditty...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_GLITTER_DUST - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();

	return eSUCCESS;
}

int execute_song_glitter_dust(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	struct affected_type af;
	struct affected_type af2;

	af.type = SKILL_GLITTER_DUST;
	af.duration = (ch->getLevel() > 25) ? 2 : 1;
	af.modifier = 0;
	af.location = APPLY_GLITTER_DUST;
	af.bitvector = -1;
	af.caster = GET_NAME(ch);

	af2.type = SKILL_GLITTER_DUST;
	af2.duration = (ch->getLevel() > 25) ? 2 : 1;
	af2.modifier = 10 + ch->has_skill(SKILL_SONG_GLITTER_DUST) / 3;
	af2.location = APPLY_AC;
	af2.bitvector = -1;
	af2.caster = GET_NAME(ch);

	act("The dust in the air clings to you, and begins to shine!", ch, 0, 0, TO_ROOM, 0);
	ch->sendln("Your dust clings to everyone, showing where they are!");

	for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = victim->next_in_room)
	{
		// don't effect gods unless it was a higher level god singing
		if (victim->getLevel() >= IMMORTAL && ch->getLevel() <= victim->getLevel())
			continue;

		// don't want it affecting the bard
		if (victim == ch)
			continue;

		// prevent stacking
		if (IS_AFFECTED(victim, AFF_GLITTER_DUST))
		{
			csendf(ch, "%s is already covered in glitter.\r\n", GET_SHORT(victim));
			continue;
		}

		bool pre_see = CAN_SEE(ch, victim);
		affect_to_char(victim, &af);
		affect_to_char(victim, &af2);
		bool post_see = CAN_SEE(ch, victim);
		if (!pre_see && post_see)
		{
			csendf(ch, "Your glitter reveals %s.\r\n", GET_SHORT(victim));
		}
	}

	Object *item;
	for (item = DC::getInstance()->world[ch->in_room].contents; item; item = item->next_content)
	{
		if (GET_ITEM_TYPE(item) == ITEM_BEACON && isSet(item->obj_flags.extra_flags, ITEM_INVISIBLE))
		{
			ch->sendln("Your glitter reveals a beacon.");
			REMOVE_BIT(item->obj_flags.extra_flags, ITEM_INVISIBLE);
		}
	}

	return eSUCCESS;
}

int song_bountiful_sonnet(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin long restoring sonnet...");
	act("$n begins a long restorous sonnet...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_BOUNT_SONNET - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();

	return eSUCCESS;
}

int execute_song_bountiful_sonnet(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *master = nullptr;
	follow_type *fvictim = nullptr;

	if (ch->master && ch->master->in_room == ch->in_room && ISSET(ch->affected_by, AFF_GROUP))
		master = ch->master;
	else
		master = ch;

	for (fvictim = master->followers; fvictim; fvictim = fvictim->next)
	{
		if (!ISSET(fvictim->follower->affected_by, AFF_GROUP) || fvictim->follower->in_room != ch->in_room)
			continue;

		fvictim->follower->sendln("Your appetite has been completely satiated.");
		if (GET_COND(fvictim->follower, FULL) != -1 && fvictim->follower->getLevel() < 60)
		{
			GET_COND(fvictim->follower, FULL) = 24;
		}
		if (GET_COND(fvictim->follower, THIRST) != -1 && fvictim->follower->getLevel() < 60)
		{
			GET_COND(fvictim->follower, THIRST) = 24;
		}
	}
	if (ch->in_room == master->in_room)
	{
		master->sendln("Your appetite has been completely satiated.");
		if (GET_COND(master, FULL) != -1 && master->getLevel() < 60)
		{
			GET_COND(master, FULL) = 24;
		}
		if (GET_COND(master, THIRST) != -1 && master->getLevel() < 60)
		{
			GET_COND(master, THIRST) = 24;
		}
	}
	return eSUCCESS;
}

int execute_song_dischordant_dirge(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *target = nullptr;
	// char buf[400];
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_DISCHORDANT_DIRGE - SKILL_SONG_BASE)
			break;
	}

	target = ch->get_char_room_vis((*i).song_data);

	delete[] (*i).song_data;
	(*i).song_data = 0;

	act("$n's dirge ends in a shriek.", ch, 0, 0, TO_ROOM, 0);

	if (!target || ch->getLevel() < target->getLevel())
	{
		ch->sendln("Your dirge fades, its effect neutralized.");
		return eFAILURE;
	}

	if (ch == target)
	{
		ch->sendln("Your loyalties have been broken, what did you think?");
		return eFAILURE;
	}

	if (IS_PC(target))
	{
		csendf(ch, "%s is too strong willed for you to break any of %s loyalties.\r\n", GET_NAME(target), HSHR(target));
		return eFAILURE;
	}
	if (!target->affected_by_spell(SPELL_CHARM_PERSON) && !IS_AFFECTED(target, AFF_FAMILIAR))
	{
		ch->sendln("As far as you can tell, they are not loyal to anyone.");
		return eFAILURE;
	}
	int type = 0;
	if (DC::getInstance()->mob_index[target->mobdata->nr].virt == 8)
		type = 4;
	else if (IS_AFFECTED(target, AFF_FAMILIAR))
		type = 3;
	else if (DC::getInstance()->mob_index[target->mobdata->nr].virt >= 22394 && DC::getInstance()->mob_index[target->mobdata->nr].virt <= 22398)
		type = 2;
	else
		type = 1;
	if ((type == 4 && !number(0, 9)) || (type == 2 && !number(0, 4)) || (type == 1 && !number(0, 1)))
	{
		ch->sendln("Ooops, that didn't work out like you hoped.");
		act("$N charges at $n, for trying to break its bond with its master.\r\n", ch, 0, target, TO_ROOM, NOTVICT);
		act("$N charges at you!", ch, 0, target, TO_CHAR, 0);
		return attack(target, ch, TYPE_UNDEFINED);
	}

	// int i;
	/*   for (i = 22394; i < 22399; i++)
	 if (real_mobile(i) == target->mobdata->nr)
	 {
	 ch->sendln("The undead being is unaffected by your song.");
	 return eFAILURE;
	 }*/
	if (IS_AFFECTED(target, AFF_FAMILIAR))
	{
		act("$n shatters $N's bond with this realm, and the creature vanishes.", ch, 0, target, TO_ROOM, NOTVICT);
		act("At your dirge's completion, $N vanishes.", ch, 0, target, TO_CHAR, 0);
		make_dust(target);
		extract_char(target, true);
		return eSUCCESS;
	}
	if (type == 4)
	{
		act("$n shatters!", target, 0, 0, TO_ROOM, 0);
		make_dust(target);
		extract_char(target, false);
		return eSUCCESS;
	}
	else if (type == 2)
	{
		act("$n's mind is set free, and the body falls onto the ground", target, 0, 0, TO_ROOM, 0);
		make_dust(target);
		extract_char(target, true);
		return eSUCCESS;
	}
	affect_from_char(target, SPELL_CHARM_PERSON);
	ch->sendln("You shatter their magical chains.");
	target->sendln("Boogie! Your mind has been set free!");

	act("$N blinks and shakes its head, clearing its thoughts.", ch, 0, target, TO_CHAR, 0);
	act("$N blinks and shakes its head, clearing its thoughts.", ch, 0, target, TO_ROOM, NOTVICT);
	if (target->fighting)
	{
		do_say(target, "Hey, this sucks. I'm goin' home!");
		if (target->fighting->fighting == target)
			stop_fighting(target->fighting);
		stop_fighting(target);
	}
	return eSUCCESS;
}

int song_dischordant_dirge(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin a wailing dirge...");
	act("$n begins to sing a wailing dirge...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_DISCHORDANT_DIRGE - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats() - (ch->getLevel() / 10);
	if ((*i).song_timer < 1)
		(*i).song_timer = 1;

	// store the char name here, cause A, we don't pass tar_char
	// and B, there's no place to save it before we execute
	(*i).song_data = str_dup(arg);

	return eSUCCESS;
}

int song_synchronous_chord(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin a strong chord...");
	act("$n begins to sound a chord...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_SYNC_CHORD - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats() - (ch->getLevel() / 10);
	if ((*i).song_timer < 1)
		(*i).song_timer = 1;

	// store the char name here, cause A, we don't pass tar_char
	// and B, there's no place to save it before we execute
	(*i).song_data = str_dup(arg);

	return eSUCCESS;
}

int execute_song_synchronous_chord(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *target = nullptr;
	char buf[400];
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_SYNC_CHORD - SKILL_SONG_BASE)
			break;
	}

	target = ch->get_char_room_vis((*i).song_data);

	delete[] (*i).song_data;
	(*i).song_data = 0;

	act("$n's song ends with an abrupt stop.", ch, 0, 0, TO_ROOM, 0);

	if (!target)
	{
		ch->sendln("Your song fades away, its target unknown.");
		return eFAILURE;
	}

	if (ch == target)
	{
		ch->sendln("You hate yourself, you self-loathing bastard.");
		return eFAILURE;
	}

	if (IS_PC(target))
	{
		ch->sendln("They don't hate anyone, but they are looking at you kinda funny...");
		return eFAILURE;
	}

	act("You enter $S mind...", ch, 0, target, TO_CHAR, INVIS_NULL);
	auto new_hate = target->get_random_hate();
	sprintf(buf, "%s seems to hate... %s.\r\n", GET_SHORT(target), new_hate.isEmpty() ? "no one!" : qPrintable(new_hate));
	ch->send(buf);

	if (skill > 80)
	{
		sprintbit(target->resist, isr_bits, buf);
		if (!strcmp(buf, "NoBits"))
		{
			strcpy(buf, "nothing");
		}

		csendf(ch, "%s is resistant to: %s\r\n", GET_SHORT(target), buf);
	}
	if (skill > 85)
	{
		sprintbit(target->immune, isr_bits, buf);
		if (!strcmp(buf, "NoBits"))
		{
			strcpy(buf, "nothing");
		}

		csendf(ch, "%s is immune to: %s\r\n", GET_SHORT(target), buf);
	}
	if (skill > 90)
	{
		sprintbit(target->suscept, isr_bits, buf);
		if (!strcmp(buf, "NoBits"))
		{
			strcpy(buf, "nothing");
		}

		csendf(ch, "%s is susceptible to: %s\r\n", GET_SHORT(target), buf);
	}

	return eSUCCESS;
}

int song_sticky_lullaby(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin a slow numbing lullaby...");
	act("$n starts singing an eye-drooping lullaby.", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_STICKY_LULL - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();
	if (ch->getLevel() < 40)
		(*i).song_timer += 2;

	// store the char name here, cause A, we don't pass tar_char
	// and B, there's no place to save it before we execute
	(*i).song_data = str_dup(arg);

	return eSUCCESS;
}

int execute_song_sticky_lullaby(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_STICKY_LULL - SKILL_SONG_BASE)
			break;
	}

	if (!(victim = ch->get_char_room_vis((*i).song_data)))
	{
		if (ch->fighting)
			victim = ch->fighting;
		else
		{
			ch->sendln("You don't see that person here.");
			delete[] (*i).song_data;
			(*i).song_data = 0;
			return eFAILURE;
		}
	}
	delete[] (*i).song_data;
	(*i).song_data = 0;
	if (number(1, 100) < get_saves(victim, SAVE_TYPE_POISON))
	{
		act("$N resists your sticky lullaby!", ch, nullptr, victim, TO_CHAR, 0);
		act("$N resists $n's sticky lullaby!", ch, nullptr, victim, TO_ROOM, NOTVICT);
		act("You resist $n's sticky lullaby!", ch, nullptr, victim, TO_VICT, 0);
		return eFAILURE;
	}

	act("$n lulls $N's feet into a numbing sleep.", ch, 0, victim, TO_ROOM, NOTVICT);
	act("$N's feet fall into a numbing sleep.", ch, 0, victim, TO_CHAR, 0);
	victim->sendln("Your eyes begin to droop, and your feet fall asleep!");
	SETBIT(victim->affected_by, AFF_NO_FLEE);
	return eSUCCESS;
}

int song_vigilant_siren(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing a fast nervous tune...");
	act("$n starts mumbling out a quick, nervous tune...", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_VIGILANT_SIREN - SKILL_SONG_BASE)
			break;

	(*i).song_timer = 1;

	return eSUCCESS;
}

int execute_song_vigilant_siren(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	if (GET_KI(ch) < 2) // we don't have the ki to keep the song going
	{
		return intrp_vigilant_siren(level, ch, arg, victim, -1);
	}

	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_VIGILANT_SIREN - SKILL_SONG_BASE)
			break;
	}

	struct affected_type af1, af2, af3;

	af1.type = SKILL_SONG_VIGILANT_SIREN;
	af1.duration = 1;
	af1.modifier = 0;
	af1.location = 0;
	af1.bitvector = AFF_ALERT;
	af1.caster = GET_NAME(ch);

	af2.type = SKILL_SONG_VIGILANT_SIREN;
	af2.duration = 1;
	af2.modifier = 0;
	af2.location = 0;
	af2.bitvector = AFF_NO_CIRCLE;
	af2.caster = GET_NAME(ch);

	af3.type = SKILL_SONG_VIGILANT_SIREN;
	af3.duration = 1;
	af3.modifier = 0;
	af3.location = 0;
	af3.bitvector = AFF_NO_BEHEAD;
	af3.caster = GET_NAME(ch);

	for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
	{
		if (!ARE_GROUPED(ch, tmp_char))
			continue;

		if (tmp_char->affected_by_spell(SKILL_SONG_VIGILANT_SIREN))
		{
			affect_from_char(tmp_char, SKILL_SONG_VIGILANT_SIREN);
		}

		affect_to_char(tmp_char, &af1);

		if (skill > 85)
			affect_to_char(tmp_char, &af2);

		if (skill > 90)
			affect_to_char(tmp_char, &af3);

		tmp_char->sendln("You nervously watch your surroundings with magical speed!");
	}

	if (!skill_success(ch, nullptr, SKILL_SONG_VIGILANT_SIREN))
	{
		(*i).song_timer = -1;
		return eSUCCESS;
	}
	if (ch->songs.size() > 1 && !skill_success(ch, nullptr, SKILL_ORCHESTRATE))
	{
		(*i).song_timer = -1;
		csendf(ch, "You miss a note, ruining your orchestration of %s!\r\n", qPrintable(Character::song_names.value((*i).song_number)));
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "$n misses a note, ruining $s orchestration of %s!", qPrintable(Character::song_names.value((*i).song_number)));
		act(buf, ch, 0, 0, TO_ROOM, 0);
		return eSUCCESS;
	}

	GET_KI(ch) -= skill > 85 ? 2 : 1;

	(*i).song_timer = song_info[(*i).song_number].beats() + (ch->getLevel() > 48);
	return eSUCCESS;
}

int pulse_vigilant_siren(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	if (number(1, 5) == 3)
		act("$n chatters a ditty about being alert and ever watchful.", ch, 0, 0, TO_ROOM, 0);
	return eSUCCESS;
}

int intrp_vigilant_siren(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *master = nullptr;
	follow_type *fvictim = nullptr;

	if (ch->master && ISSET(ch->affected_by, AFF_GROUP))
		master = ch->master;
	else
		master = ch;

	for (fvictim = master->followers; fvictim; fvictim = fvictim->next)
	{
		if (origsing && origsing != fvictim->follower)
			continue;
		if (ISSET(fvictim->follower->affected_by, AFF_ALERT))
		{
			REMBIT(fvictim->follower->affected_by, AFF_ALERT);
			fvictim->follower->sendln("You stop watching your back so closely.");
		}
		if (IS_AFFECTED(fvictim->follower, AFF_NO_CIRCLE))
			REMBIT(fvictim->follower->affected_by, AFF_NO_CIRCLE);
		if (IS_AFFECTED(fvictim->follower, AFF_NO_BEHEAD))
			REMBIT(fvictim->follower->affected_by, AFF_NO_BEHEAD);
	}

	if (!origsing || origsing == ch)
		if (ISSET(master->affected_by, AFF_ALERT))
		{
			REMBIT(master->affected_by, AFF_ALERT);
			master->sendln("You stop watching your back so closely.");
		}
	if (IS_AFFECTED(master, AFF_NO_CIRCLE))
		REMBIT(master->affected_by, AFF_NO_CIRCLE);
	if (IS_AFFECTED(master, AFF_NO_BEHEAD))
		REMBIT(master->affected_by, AFF_NO_BEHEAD);

	return eSUCCESS;
}

void make_person_dance(Character *ch)
{
	char *dances[] = {"dance",										   // 0
					  "tango", "boogie", "jig", "waltz", "bellydance", // 5
					  "\n"};

	int numdances = 5;
	char dothis[50];

	strcpy(dothis, dances[number(0, numdances)]);

	ch->command_interpreter(dothis);
}

int song_unresistable_ditty(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing an irresistable little ditty...");
	act("$n begins to sing, 'du du dudu du du dudu du du dudu!'", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_UNRESIST_DITTY - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();

	return eSUCCESS;
}

int execute_song_unresistable_ditty(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *i;

	act("$n finishs $s song, 'Ahhhhh!  Macarena!'", ch, 0, 0, TO_ROOM, 0);
	ch->sendln("Ahhh....such beautiful music.");

	//   int specialization = skill / 100;
	skill %= 100;

	for (i = DC::getInstance()->world[ch->in_room].people; i; i = i->next_in_room)
	{
		if (number(1, 100) < get_saves(i, SAVE_TYPE_MAGIC))
		{
			act("$N resists your irresistible ditty!", ch, nullptr, i,
				TO_CHAR, 0);
			act("$N resists $n's irresitble ditty! Not so irresistible, eh!", ch, nullptr, i, TO_ROOM, NOTVICT);
			act("You resist $n's \"irresistible\" ditty!!", ch, nullptr, i, TO_VICT, 0);
			continue;
		}

		if (i->getLevel() <= ch->getLevel())
		{
			make_person_dance(i);
			if (skill > 80)
				WAIT_STATE(i, DC::PULSE_VIOLENCE * 2);
		}
	}

	return eSUCCESS;
}

int song_crushing_crescendo(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing, approaching crescendo!");
	act("$n begins to sing, raising the volume to deafening levels!", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_CRUSHING_CRESCENDO - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats();
	(*i).song_data = 0; // first round.

	return eSUCCESS;
}

int execute_song_crushing_crescendo(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	int dam = 0;
	int retval;
	std::vector<songInfo>::iterator i;

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
	{
		if ((*i).song_number == SKILL_SONG_CRUSHING_CRESCENDO - SKILL_SONG_BASE)
			break;
	}

	// int specialization = skill / 100;
	skill %= 100;

	victim = ch->fighting;

	if (!victim)
	{
		ch->sendln("With the battle broken you end your crescendo.");
		return eSUCCESS;
	}

	int combat, non_combat;
	get_instrument_bonus(ch, combat, non_combat);

	int j;
	dam = ((ch->has_skill(SKILL_SONG_CRUSHING_CRESCENDO)) + 25);
	for (j = 0; j < (int64_t)(*i).song_data; j++)
		dam = dam * 2;
	dam += combat * 5;										// Make it hurt some more.
															//   if ((int)(*i).song_data < 3) // Doesn't help beyond that.
	(*i).song_data = (char *)((int64_t)(*i).song_data + 1); // Add one round.
	// Bleh, C allows easier pointer manipulation
	if (isSet(victim->immune, ISR_SONG))
	{
		act("$N laughs at your crushing crescendo!", ch, 0, victim, TO_CHAR, 0);
		act("You laugh at $n's crushing crescendo.", ch, 0, victim, TO_VICT, 0);
		act("$N laughs at $n's crushing crescendo.", ch, 0, victim, TO_ROOM, NOTVICT);
	}
	else
	{
		if (number(1, 100) < get_saves(victim, SAVE_TYPE_MAGIC))
		{
			act("$N resists your crushing crescendo!", ch, nullptr, victim, TO_CHAR, 0);
			act("$N resists $n's crushing crescendo!", ch, nullptr, victim, TO_ROOM, NOTVICT);
			act("You resist $n's crushing crescendo!", ch, nullptr, victim, TO_VICT, 0);
			dam /= 2;
		}
		char dmgmsg[MAX_STRING_LENGTH];
		sprintf(dmgmsg, "$B%d$R", dam);
		switch ((int64_t)(*i).song_data)
		{
		case 1:
			send_damage("$N is injured for | damage by the strength of your music!", ch, 0, victim, dmgmsg, "$N is injured by the strength of your music!",
						TO_CHAR);
			send_damage("The strength of $n's music injures $N for |!", ch, 0, victim, dmgmsg, "The strength of $n's music injures $N!", TO_ROOM);
			send_damage("The strength of $n's crushing crescendo injures you for |!", ch, 0, victim, dmgmsg,
						"The strength of $n's crushing crescendo injures you!", TO_VICT);
			break;
		case 2:
			send_damage("$N is injured further by the intensity of your music for | damage!", ch, 0, victim, dmgmsg,
						"$N is injured further by the intensity of your music!", TO_CHAR);
			send_damage("The strength of $n's music increases, and causes further injury to $N for | damage!", ch, 0, victim, dmgmsg,
						"The strength of $n's music increases, and causes further injury to $N!", TO_ROOM);
			send_damage("The strength of $n's crushing crescendo increases, and hurts for | damage!", ch, 0, victim, dmgmsg,
						"The strength of $n's crushing crescendo increases, and hurts even more!", TO_VICT);
			break;
		case 3:
			send_damage("The force of your song powerfully crushes $N for | damage!", ch, 0, victim, dmgmsg,
						"The force of your song powerfully crushes the life out of $N!", TO_CHAR);
			send_damage("The force of $n's crushes $N for | damage!", ch, 0, victim, dmgmsg, "The force of $n's crushes the life out of $N!", TO_ROOM);
			send_damage("The force of $n's crushes you for | damage!", ch, 0, victim, dmgmsg, "The force of $n's crushes the life out of you!", TO_VICT);
			break;
		default:
			send_damage("$N is injured for | damage by the strength of your music!", ch, 0, victim, dmgmsg, "$N is injured by the strength of your music!",
						TO_CHAR);
			send_damage("The strength of $n's music injures $N for | damage!", ch, 0, victim, dmgmsg, "The strength of $n's music injures $N!", TO_ROOM);
			send_damage("The strength of $n's crushing crescendo injures you for | damage!", ch, 0, victim, dmgmsg,
						"The strength of $n's crushing crescendo injures you!", TO_VICT);
			break;
		}
	}

	bool ispc = IS_PC(victim);
	char buf[MAX_STRING_LENGTH];
	strcpy(buf, victim->short_desc);

	retval = damage(ch, victim, dam, TYPE_SONG, SKILL_SONG_CRUSHING_CRESCENDO);
	if (isSet(retval, eCH_DIED))
		return retval;

	if (isSet(retval, eVICT_DIED))
	{
		char buf2[MAX_STRING_LENGTH];
		sprintf(buf2, "$n's crushing crescendo has completely crushed %s and they are no more.", buf);
		act(buf2, ch, nullptr, victim, TO_ROOM, NOTVICT);
		if (ispc)
			act("$n's song has completely crushed you!", ch, nullptr, victim, TO_VICT, 0);
		sprintf(buf2, "The power of your song has completely crushed %s!", buf);
		act(buf2, ch, nullptr, victim, TO_CHAR, 0);

		ch->sendln("You dance a small jig on the corpse.");
		act("$n dances a little jig on the fallen corpse.", ch, 0, victim, TO_ROOM, 0);
		return retval;
	}

	if ((int64_t)(*i).song_data > ch->has_skill(SKILL_SONG_CRUSHING_CRESCENDO) / 20 || (int64_t)(*i).song_data > 3)
	{
		ch->sendln("You run out of lyrics and end the song.");
		ch->songs.erase(i);
		return eEXTRA_VALUE;
	}

	if (((int64_t)(*i).song_data) != 3)
	{
		if (GET_KI(ch) < song_info[(*i).song_number].min_useski())
		{
			ch->sendln("Having run out of ki, your song ends abruptly.");
			(*i).song_data = 0; // Reset, just in case.
			return eSUCCESS;
		}
		GET_KI(ch) -= song_info[(*i).song_number].min_useski();
	}
	(*i).song_timer = song_info[(*i).song_number].beats();
	return eSUCCESS;
}

int song_submariners_anthem(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	std::vector<songInfo>::iterator i;

	ch->sendln("You begin to sing about the shining sea and her terrible ways...");
	act("$n sings a surly number about $s fickle mistress, the briny deep.", ch, 0, 0, TO_ROOM, 0);

	for (i = ch->songs.begin(); i != ch->songs.end(); ++i)
		if ((*i).song_number == SKILL_SONG_SUBMARINERS_ANTHEM - SKILL_SONG_BASE)
			break;

	(*i).song_timer = song_info[(*i).song_number].beats() - (1 + skill / 10);

	return eSUCCESS;
}

int execute_song_submariners_anthem(uint8_t level, Character *ch, char *arg, Character *victim, int skill)
{
	Character *master = nullptr;
	follow_type *fvictim = nullptr;
	struct affected_type af;
	af.type = SKILL_SONG_SUBMARINERS_ANTHEM;
	af.duration = 1 + (skill / 10);
	af.modifier = 0;
	af.caster = GET_NAME(ch);
	af.location = APPLY_NONE;
	af.bitvector = -1;

	if (ch->master && ch->master->in_room == ch->in_room && ISSET(ch->affected_by, AFF_GROUP))
		master = ch->master;
	else
		master = ch;

	for (fvictim = master->followers; fvictim; fvictim = fvictim->next)
	{
		if (!ISSET(fvictim->follower->affected_by, AFF_GROUP))
			continue;

		if (ch->in_room != fvictim->follower->in_room)
			continue;

		if (fvictim->follower->affected_by_spell(SPELL_WATER_BREATHING))
		{
			affect_from_char(fvictim->follower, SPELL_WATER_BREATHING);
			fvictim->follower->send("Your magical gills disappear.");
		}

		if (fvictim->follower->affected_by_spell(SKILL_SONG_SUBMARINERS_ANTHEM))
			affect_from_char(fvictim->follower, SKILL_SONG_SUBMARINERS_ANTHEM);

		affect_to_char(fvictim->follower, &af);
		fvictim->follower->sendln("Your lungs absorb oxygen from any fluid!");
	}

	if (ch->in_room == master->in_room)
	{
		if (master->affected_by_spell(SKILL_SONG_SUBMARINERS_ANTHEM))
			affect_from_char(master, SKILL_SONG_SUBMARINERS_ANTHEM);
		affect_to_char(master, &af);
		master->sendln("Your lungs absorb oxygen from any fluid!");
	}

	return eSUCCESS;
}

QDebug operator<<(QDebug debug, const songInfo &song)
{
	QDebugStateSaver saver(debug);
	debug.nospace() << "Song:" << song.song_data << " number:" << song.song_number << " timer:" << song.song_timer;
	return debug;
}
