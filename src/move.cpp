/************************************************************************
 | $Id: move.cpp,v 1.99 2015/06/14 02:38:12 pirahna Exp $
 | move.C
 | Movement commands and stuff.
 *************************************************************************
 *  Revision History                                                     *
 *  11/09/2003   Onager   Changed do_unstable() and do_fall() to call    *
 *                        noncombat_damage() for damage                  *
 *************************************************************************
 */
#include "DC/character.h"
#include "DC/affect.h"
#include "DC/room.h"
#include "DC/utility.h"
#include "DC/player.h"
#include "DC/fight.h"
#include "DC/mobile.h"
#include "DC/interp.h"
#include "DC/spells.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/DC.h"
#include "DC/connect.h"
#include "DC/act.h"
#include "DC/race.h" // RACE_FISH
#include "DC/clan.h" // clan_room_data
#include <cstring>
#include "DC/returnvals.h"
#include "DC/game_portal.h"
#include "DC/innate.h"
#include "DC/weather.h"
#include "DC/const.h"
#include "DC/move.h"

int check_ethereal_focus(Character *ch, int trigger_type); // class/cl_mage.cpp

int move_player(Character *ch, int room)
{
	int retval;

	retval = move_char(ch, room);

	if (!isSet(retval, eSUCCESS))
	{
		retval = move_char(ch, real_room(START_ROOM));
		if (!isSet(retval, eSUCCESS))
			logentry(QStringLiteral("Error in move_player(), Failure moving ch to start room. move_player_home_nofail"),
					 IMMORTAL, DC::LogChannel::LOG_BUG);
	}

	return retval;
}

void move_player_home(Character *victim)
{
	int was_in = victim->in_room;
	int found = 0;
	clan_data *clan = nullptr;
	clan_room_data *room = nullptr;

	// check for homes that don't exist
	if (real_room(GET_HOME(victim) < 1))
		GET_HOME(victim) = START_ROOM;

	// next four lines ping-pong people from meta to tavern to help lessen spam
	if (DC::getInstance()->world[was_in].number == GET_HOME(victim) && GET_HOME(victim) == START_ROOM)
		move_player(victim, real_room(SECOND_START_ROOM));
	else if (DC::getInstance()->world[was_in].number == GET_HOME(victim) || IS_AFFECTED(victim, AFF_CHAMPION))
		move_player(victim, real_room(START_ROOM));
	// recalling into a clan room
	else if (!isSet(DC::getInstance()->world[real_room(GET_HOME(victim))].room_flags, CLAN_ROOM))
		move_player(victim, real_room(GET_HOME(victim)));
	// Clanroom else
	else
	{
		if (!victim->clan || !(clan = get_clan(victim)))
		{
			victim->sendln("The gods frown on you, and reset your home.");
			GET_HOME(victim) = START_ROOM;
			move_player(victim, real_room(GET_HOME(victim)));
		}
		else
		{
			for (room = clan->rooms; room; room = room->next)
				if (room->room_number == GET_HOME(victim))
					found = 1;
			if (!found)
			{
				victim->sendln("The gods frown on you, and reset your home.");
				GET_HOME(victim) = START_ROOM;
			}
			move_player(victim, real_room(GET_HOME(victim)));
		}
	}
}

// Rewritten 9/1/96
// -Sadus
void record_track_data(Character *ch, int cmd)
{
	room_track_data *newScent;

	// Rangers outdoors leave no tracks
	if (DC::getInstance()->world[ch->in_room].sector_type != SECT_INSIDE &&
		GET_CLASS(ch) == CLASS_RANGER)
		return;

	// If we just used a scent killer like catstink, don't leave tracks
	// on our next move
	if (ISSET(ch->affected_by, AFF_UTILITY))
	{
		REMBIT(ch->affected_by, AFF_UTILITY);
		return;
	}

	if (DC::getInstance()->world[ch->in_room].sector_type == SECT_WATER_SWIM || DC::getInstance()->world[ch->in_room].sector_type == SECT_WATER_NOSWIM)
		return;

#ifdef LEAK_CHECK
	newScent = (room_track_data *)calloc(1, sizeof(struct room_track_data));
#else
	newScent = (room_track_data *)dc_alloc(1, sizeof(struct room_track_data));
#endif
	newScent->direction = cmd;
	newScent->weight = (int)ch->weight;
	newScent->race = (int)ch->race;
	newScent->sex = (int)ch->sex;
	newScent->condition = ((ch->getHP() * 100) / (GET_MAX_HIT(ch) == 0 ? 100 : GET_MAX_HIT(ch)));
	newScent->next = nullptr;	  // just in case
	newScent->previous = nullptr; // just in case

	newScent->trackee = ch->getName();

	DC::getInstance()->world[ch->in_room].AddTrackItem(newScent);

	return;
}

// Removed this due to it being a funky cold medina. - Nocturnal 09/28/05
// void do_muddy(Character *ch)
//{
//   short chance = number(0,30);
//
//   if(IS_NPC(ch) || IS_AFFECTED(ch, AFF_FLYING) || ch->isImmortalPlayer() || IS_AFFECTED(ch, AFF_FREEFLOAT)) {
//     ; //poop on a stick!
//   } else if(GET_DEX(ch) > chance) {
//      act("You barely avoid slipping in the mud.", ch, 0, 0, TO_CHAR, 0);
//   } else {
//      act("$n slips on the muddy terrain and goes down.", ch, 0, 0, TO_ROOM, 0);
//      act("Your feet slide out from underneath you in the mud.", ch, 0, 0, TO_CHAR, 0);
//      ch->setSitting();
//   }
//}

int do_unstable(Character *ch)
{
	char death_log[MAX_STRING_LENGTH];
	int retval;

	if (IS_NPC(ch))
		return eFAILURE;

	short chance = number(0, 30);

	if (IS_AFFECTED(ch, AFF_FLYING) || IS_AFFECTED(ch, AFF_FREEFLOAT) ||
		ch->isImmortalPlayer())
		return eFAILURE;

	if (GET_DEX(ch) > chance)
	{
		act("You dextrously maintain your footing.", ch, 0, 0, TO_CHAR, 0);
		return eFAILURE;
	}

	act("$n looses his footing on the unstable ground.", ch, 0, 0, TO_ROOM, 0);
	act("You fall flat on your ass.", ch, 0, 0, TO_CHAR, 0);
	ch->setSitting();

	sprintf(death_log, "%s slipped to death in room %d.", GET_NAME(ch), DC::getInstance()->world[ch->in_room].number);
	retval = noncombat_damage(ch, GET_MAX_HIT(ch) / 12, "You feel your back snap painfully and all goes dark...",
							  "$n's frail body snaps in half as $e is buffeted about the room.", death_log, KILL_FALL);

	if (SOMEONE_DIED(retval))
		return eSUCCESS | eCH_DIED;
	else
		return eSUCCESS;
}

int do_fall(Character *ch, short dir)
{
	int target;
	char damage[MAX_STRING_LENGTH];
	int retval;
	int dam = number(50, 100);

	if (IS_AFFECTED(ch, AFF_FLYING))
		return eFAILURE;

	if (ch->getLevel() == 50)
		dam = number(100, 200);

	if (GET_MAX_HIT(ch) > 2000)
		dam = number(400, 500);
	else if (GET_MAX_HIT(ch) > 1000)
		dam = number(200, 400);

	if (ch->isImmortalPlayer())
	{
		return eFAILURE;
	}

	if (IS_AFFECTED(ch, AFF_FREEFLOAT))
	{
		dam = 0;
		ch->sendln("Your freefloating magics reduce your fall to a safer speed.");
	}

	// Don't effect mobs
	if (IS_NPC(ch))
	{
		act("$n clings to the terrain around $m and avoids falling.", ch, 0, 0, TO_ROOM, 0);
		return eFAILURE;
	}

	if (!CAN_GO(ch, dir))
		return eFAILURE;

	target = DC::getInstance()->world[ch->in_room].dir_option[dir]->to_room;

	if (isSet(DC::getInstance()->world[target].room_flags, IMP_ONLY) && ch->getLevel() < IMPLEMENTER)
		return eFAILURE;

	if (isSet(DC::getInstance()->world[target].room_flags, TUNNEL))
	{
		int ppl = 0;
		Character *k;
		for (k = DC::getInstance()->world[target].people; k; k = k->next_in_room)
			if (IS_PC(k))
				ppl++;
		if (ppl > 2)
		{
			ch->sendln("There isn't enough space for you to follow.");
			return eFAILURE;
		}
	}

	if (isSet(DC::getInstance()->world[target].room_flags, PRIVATE))
	{
		int ppl = 0;
		Character *k;
		for (k = DC::getInstance()->world[target].people; k; k = k->next_in_room)
			if (IS_PC(k))
				ppl++;
		if (ppl > 4)
		{
			ch->sendln("There isn't enough space for you to follow.");
			return eFAILURE;
		}
	}

	switch (dir)
	{
	case 0:
		act("$n rolls out to the north.", ch, 0, 0, TO_ROOM, 0);
		ch->sendln("You tumble to the north...");
		break;
	case 1:
		act("$n rolls out to the east.", ch, 0, 0, TO_ROOM, 0);
		ch->sendln("You tumble to the east...");
		break;
	case 2:
		act("$n rolls out to the south.", ch, 0, 0, TO_ROOM, 0);
		ch->sendln("You tumble to the south...");
		break;
	case 3:
		act("$n rolls out to the west.", ch, 0, 0, TO_ROOM, 0);
		ch->sendln("You tumble to the west...");
		break;
	case 4:
		act("$n is launched upwards.", ch, 0, 0, TO_ROOM, 0);
		ch->sendln("You are launched into the air...");
		break;
	case 5:
		act("$n falls through the room.", ch, 0, 0, TO_ROOM, 0);
		ch->sendln("You fall...");
		break;
	default:
		logentry(QStringLiteral("Default hit in do_fall"), IMMORTAL, DC::LogChannel::LOG_MORTAL);
		break;
	}

	retval = move_char(ch, target);

	if (!isSet(retval, eSUCCESS))
	{
		ch->sendln("You are miraculously upheld by divine powers!");
		return retval;
	}

	do_look(ch, "\0", 15);

	sprintf(damage, "%s's fall from %d was lethal and it killed them.", GET_NAME(ch), DC::getInstance()->world[ch->in_room].number);
	retval = noncombat_damage(ch, dam, "Luckily the ground breaks your fall.\r\n", "$n plummets into the room and hits the ground with a wet-sounding splat!",
							  damage, KILL_FALL);

	if (SOMEONE_DIED(retval))
		return eSUCCESS;

	if (!SOMEONE_DIED(retval))
	{
		act("$n plummets into the room and hits the floor HARD.", ch, 0, 0, TO_ROOM, 0);
	}

	if ((isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_DOWN) && (dir = 5)) || (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_UP) && (dir = 4)) || (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_EAST) && (dir = 1)) || (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_WEST) && (dir = 3)) || (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_SOUTH) && (dir = 2)) || (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_NORTH) && (dir = 0)))
	{
		return do_fall(ch, dir);
	}

	// We lived
	return eSUCCESS;
}

// Assumes
// 1. No master and no followers.
// 2. That the direction exists.
int do_simple_move(Character *ch, int cmd, int following)
{
	char tmp[80];
	int dir;
	int was_in;
	int need_movement, learned, mvinroom = 0, mvtoroom = 0;
	int retval;
	class Object *obj;
	bool has_boat;

	/*
	 I think this is taken care of in the command interpreter.  Disabling it for now.
	 -pir 7/25/01
	 // Check for special routines (North is 1)
	 retval = special(ch, cmd+1, "");
	 if(isSet(retval, eSUCCESS) || isSet(retval, eCH_DIED))
	 return retval;
	 */

	if (ch == nullptr || ch->in_room < 1 || cmd < 0 || DC::getInstance()->world[ch->in_room].dir_option[cmd] == nullptr || DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room < 1)
	{
		logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Error in room %d.", ch->in_room);
		ch->send("There was an error performing that movement.\r\n");
		return eFAILURE;
	}

	if (IS_AFFECTED(ch, AFF_FLYING))
		need_movement = 1;
	else
	{
		if ((learned = ch->has_skill(SKILL_NATURES_LORE)))
		{

			if (learned > 90)
			{
				if (DC::getInstance()->world[ch->in_room].sector_type == SECT_UNDERWATER)
				{
					mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type] / 2;
					ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
				}
				if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_UNDERWATER)
					mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
			}

			if (learned > 80)
			{
				if (DC::getInstance()->world[ch->in_room].sector_type == SECT_MOUNTAIN)
				{
					mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type] / 2;
					if (learned < 91)
						ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
				}
				if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_MOUNTAIN)
					mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
			}

			if (learned > 70)
			{
				if (DC::getInstance()->world[ch->in_room].sector_type == SECT_ARCTIC)
				{
					mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type] / 2;
					if (learned < 81)
						ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
				}
				if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_ARCTIC)
					mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
			}

			if (learned > 60)
			{
				if (DC::getInstance()->world[ch->in_room].sector_type == SECT_HILLS)
				{
					mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type] / 2;
					if (learned < 71)
						ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
				}
				if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_HILLS)
					mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
			}

			if (learned > 50)
			{
				if (DC::getInstance()->world[ch->in_room].sector_type == SECT_WATER_SWIM)
				{
					mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type] / 2;
					if (learned < 61)
						ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
				}
				if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_WATER_SWIM)
					mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
			}

			if (learned > 40)
			{
				if (DC::getInstance()->world[ch->in_room].sector_type == SECT_FROZEN_TUNDRA)
				{
					mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type] / 2;
					if (learned < 51)
						ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
				}
				if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_FROZEN_TUNDRA)
					mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
			}

			if (learned > 30)
			{
				if (DC::getInstance()->world[ch->in_room].sector_type == SECT_DESERT)
				{
					mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type] / 2;
					if (learned < 41)
						ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
				}
				if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_DESERT)
					mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
			}

			if (learned > 20)
			{
				if (DC::getInstance()->world[ch->in_room].sector_type == SECT_BEACH)
				{
					mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type] / 2;
					if (learned < 31)
						ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
				}
				if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_BEACH)
					mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
			}

			if (learned > 10)
			{
				if (DC::getInstance()->world[ch->in_room].sector_type == SECT_FOREST)
				{
					mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type] / 2;
					if (learned < 21)
						ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
				}
				if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_FIELD)
					mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
			}

			if (learned > 0)
			{
				if (DC::getInstance()->world[ch->in_room].sector_type == SECT_FIELD)
				{
					mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type] / 2;
					if (learned < 11)
						ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
				}
				if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_FOREST)
					mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
			}

			if (!mvinroom)
				mvinroom = movement_loss[DC::getInstance()->world[ch->in_room].sector_type];
			if (!mvtoroom)
				mvtoroom = movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type];
			need_movement = (mvinroom + mvtoroom) / 2;
		}
		else
		{
			need_movement = (movement_loss[DC::getInstance()->world[ch->in_room].sector_type] + movement_loss[DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type]) / 2;
		}

		// if I'm trying to go "up" into a "fall down" room, etc.
		// it's OK to go east into a "fall north" room though
		// not ok, if room we're going to is AIR though
		if (!IS_AFFECTED(ch, AFF_FLYING) && ((cmd == 0 && isSet(DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[0]->to_room].room_flags, FALL_SOUTH)) || (cmd == 1 && isSet(DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[1]->to_room].room_flags, FALL_WEST)) || (cmd == 2 && isSet(DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[2]->to_room].room_flags, FALL_NORTH)) || (cmd == 3 && isSet(DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[3]->to_room].room_flags, FALL_EAST)) || (cmd == 4 && isSet(DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[4]->to_room].room_flags, FALL_DOWN)) || (cmd == 5 && isSet(DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[5]->to_room].room_flags, FALL_UP)) || DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_AIR))
		{
			ch->sendln("You would need to fly to go there!");
			return eFAILURE;
		}

		// fly doesn't work over water
		if ((DC::getInstance()->world[ch->in_room].sector_type == SECT_WATER_NOSWIM) || (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_WATER_NOSWIM))
		{
			has_boat = false;
			// See if char is carrying a boat
			for (obj = ch->carrying; obj; obj = obj->next_content)
				if (obj->obj_flags.type_flag == ITEM_BOAT)
					has_boat = true;
			// See if char is wearing a boat (boat ring, etc)
			if (!has_boat)
				for (int x = 0; x < MAX_WEAR; x++)
					if (ch->equipment[x])
						if (ch->equipment[x]->obj_flags.type_flag == ITEM_BOAT)
							has_boat = true;
			if (!has_boat && !IS_AFFECTED(ch, AFF_FLYING) && !ch->isImmortalPlayer() &&
				GET_RACE(ch) != RACE_FISH && GET_RACE(ch) != RACE_SLIME && !IS_AFFECTED(ch, AFF_FREEFLOAT))
			{
				ch->sendln("You need a boat to go there.");
				return eFAILURE;
			}
		}

		if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type != SECT_WATER_NOSWIM && DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type != SECT_WATER_SWIM && DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type != SECT_UNDERWATER)
		{
			// It's NOT a water room and we don't have fly
			if (GET_RACE(ch) == RACE_FISH)
			{
				ch->sendln("You can't swim around outside water without being able to fly!");
				act("$n flops around in a futile attempt to move out of water.", ch, 0, 0, TO_ROOM, INVIS_NULL);
				return eFAILURE;
			}
		}
	} // else if !FLYING

	if (isSet(DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].room_flags, IMP_ONLY) &&
		ch->getLevel() < IMPLEMENTER)
	{
		ch->sendln("No.");
		return eFAILURE;
	}

	Room *rm = &(DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room]);

	if (rm->sector_type != DC::getInstance()->world[ch->in_room].sector_type && ch->desc && ch->desc->original && ch->desc->original->getLevel() <= DC::MAX_MORTAL_LEVEL)
	{
		int s2 = rm->sector_type, s1 = DC::getInstance()->world[ch->in_room].sector_type;
		if ((s1 == SECT_CITY && (s2 != SECT_INSIDE && s2 != SECT_PAVED_ROAD)) || (s1 == SECT_INSIDE && (s2 != SECT_CITY && s2 != SECT_PAVED_ROAD)) || (s1 == SECT_PAVED_ROAD && (s2 != SECT_INSIDE && s2 != SECT_CITY)) || (s1 == SECT_FIELD && (s2 != SECT_HILLS && s2 != SECT_MOUNTAIN)) || (s1 == SECT_HILLS && (s2 != SECT_MOUNTAIN && s2 != SECT_FIELD)) || (s1 == SECT_MOUNTAIN && (s2 != SECT_HILLS && s2 != SECT_FIELD)) || (s1 == SECT_WATER_NOSWIM && (s2 != SECT_UNDERWATER && s2 != SECT_WATER_SWIM)) || (s1 == SECT_WATER_SWIM && (s2 != SECT_UNDERWATER && s2 != SECT_WATER_NOSWIM)) || (s1 == SECT_UNDERWATER && (s2 != SECT_WATER_NOSWIM && s2 != SECT_WATER_SWIM)) || (s1 == SECT_BEACH && (s2 != SECT_DESERT)) || (s1 == SECT_DESERT && (s2 != SECT_BEACH)) || (s1 == SECT_FROZEN_TUNDRA && (s2 != SECT_ARCTIC)) || (s1 == SECT_ARCTIC && (s2 != SECT_FROZEN_TUNDRA)) || (s1 == SECT_AIR) || (s1 == SECT_SWAMP))
		{
			ch->sendln("The ghost evaporates as you leave its habitat.");
			do_return(ch, "", 0);
			// extract_char(ch,true);
			return eSUCCESS | eCH_DIED;
		}
	}

	if (isSet(rm->room_flags, TUNNEL))
	{
		int ppl = 0;
		Character *k;
		for (k = rm->people; k; k = k->next_in_room)
			if (IS_PC(k))
				ppl++;
		if (ppl > 2)
		{
			ch->sendln("There's no room.");
			return eFAILURE;
		}
	}

	if (!ch->isImmortalPlayer())
	{
		bool classRestrictions = false;
		// Determine if any class restrictions are in place
		for (int c_class = 1; c_class < CLASS_MAX; c_class++)
		{
			if (rm->allow_class[c_class] == true)
			{
				classRestrictions = true;
			}
		}

		if (classRestrictions)
		{
			if (rm->allow_class[GET_CLASS(ch)] != true)
			{
				ch->sendln("Your class is not allowed there.");
				return eFAILURE;
			}
		}
	}

	if (isSet(rm->room_flags, PRIVATE))
	{
		int ppl = 0;
		Character *k;
		for (k = rm->people; k; k = k->next_in_room)
			if (IS_PC(k))
				ppl++;
		if (ppl > 4)
		{
			ch->sendln("There's no room.");
			return eFAILURE;
		}
	}

	if (IS_PC(ch) && DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_UNDERWATER && !(ch->affected_by_spell(SPELL_WATER_BREATHING) || IS_AFFECTED(ch, AFF_WATER_BREATHING)))
	{
		ch->sendln("You feel air trying to explode from your lungs as you swim around.");
		// ch->sendln("Underwater?!");
		// return eFAILURE;
	}

	// if I'm STAY_NO_TOWN, don't enter a Zone::Flag::IS_TOWN zone no matter what
	if (IS_NPC(ch) && ISSET(ch->mobdata->actflags, ACT_STAY_NO_TOWN) && DC::getInstance()->zones.value(DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].zone).isTown())
		return eFAILURE;

	int a = 0;
	if ((a = ch->has_skill(SKILL_VIGOR)) && number(1, 101) < a / 10)
		need_movement /= 2; // No skill improvement here. Too easy.

	if (GET_MOVE(ch) < need_movement && IS_PC(ch))
	{
		if (!following)
			ch->sendln("You are too exhausted.");
		else
			ch->sendln("You are too exhausted to follow.");
		return eFAILURE;
	}

	// If they were hit with a lullaby, go ahead and clear it out
	if (ISSET(ch->affected_by, AFF_NO_FLEE))
	{
		if (ch->affected_by_spell(SPELL_IRON_ROOTS))
		{
			ch->sendln("The roots bracing your legs keep you from moving!");
			return eFAILURE;
		}
		else
		{
			REMBIT(ch->affected_by, AFF_NO_FLEE);
			ch->sendln("The travel wakes you up some, and clears the drowsiness from your legs.");
		}
	}

	if (!ch->isImmortalPlayer())
		ch->decrementMove(need_movement);

	// Everyone
	if (!IS_AFFECTED(ch, AFF_SNEAK) && !IS_AFFECTED(ch, AFF_FOREST_MELD))
	{
		sprintf(tmp, "$n leaves %s.", dirs[cmd]);
		act(tmp, ch, 0, 0, TO_ROOM, INVIS_NULL);
	}
	// Sneaking
	else if (IS_AFFECTED(ch, AFF_SNEAK))
	{
		char tmp[100];
		if (!skill_success(ch, nullptr, SKILL_SNEAK))
		{
			sprintf(tmp, "$n leaves %s.", dirs[cmd]);
			act(tmp, ch, 0, 0, TO_ROOM, INVIS_NULL | STAYHIDE);
		}
		else
		{
			sprintf(tmp, "$n sneaks %s.", dirs[cmd]);
			act(tmp, ch, 0, 0, TO_GROUP, INVIS_NULL);
			act(tmp, ch, 0, 0, TO_ROOM, GODS);
		}
	}
	// Forest melded
	else
	{
		if (DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type != SECT_FOREST && DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[cmd]->to_room].sector_type != SECT_SWAMP)
		{
			sprintf(tmp, "$n leaves %s.", dirs[cmd]);
			REMBIT(ch->affected_by, AFF_FOREST_MELD);
			ch->sendln("You detach yourself from the forest.");
			act(tmp, ch, 0, 0, TO_ROOM, INVIS_NULL);
		}
		else
		{
			sprintf(tmp, "$n sneaks %s.", dirs[cmd]);
			act(tmp, ch, 0, 0, TO_ROOM, GODS);
		}
	}

	// at this point we messaged that we are moving, but we haven't actually moved yet.  Check if ethereal focus keeps us from moving.
	retval = check_ethereal_focus(ch, ETHEREAL_FOCUS_TRIGGER_MOVE);
	if (isSet(retval, eFAILURE))
		return retval;

	was_in = ch->in_room;
	record_track_data(ch, cmd);

	retval = move_char(ch, DC::getInstance()->world[was_in].dir_option[cmd]->to_room);

	if (isSet(retval, eSUCCESS) && IS_AFFECTED(ch, AFF_CRIPPLE))
	{
		ch->sendln("Your crippled body responds slowly.");
		WAIT_STATE(ch, DC::PULSE_VIOLENCE);
	}

	Object *tmp_obj;
	for (tmp_obj = DC::getInstance()->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
		if (tmp_obj->vnum == SILENCE_OBJ_NUMBER)
			ch->sendln("The sounds around you fade to nothing as the silence takes hold...");

	for (tmp_obj = DC::getInstance()->world[was_in].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
		if (tmp_obj->vnum == SILENCE_OBJ_NUMBER)
			ch->sendln("The noise around you returns as you leave the silenced area!");

	if (!isSet(retval, eSUCCESS))
	{
		ch->sendln("You fail to move.");
		return retval;
	}

	// Fighting
	if (ch->fighting)
	{
		Character *chaser = ch->fighting;
		if (IS_NPC(ch))
		{
			ch->add_memory(GET_NAME(chaser), 'f');
			remove_memory(ch, 'h');
		}
		if (IS_NPC(chaser) && chaser->hunting.isEmpty())
		{
			level_diff_t level_difference = ch->getLevel() - chaser->getLevel() / 2;
			if (level_difference >= 0 || ch->getLevel() >= 50)
			{
				chaser->add_memory(GET_NAME(ch), 't');
				struct timer_data *timer = new timer_data;
				timer->var_arg1 = chaser->hunting;
				timer->arg2 = (void *)chaser;
				timer->function = clear_hunt;
				timer->next = timer_list;
				timer_list = timer;
				timer->timeleft = (ch->getLevel() / 4.0) * 60.0;
			}
		}
		if (chaser->fighting == ch)
			stop_fighting(chaser);
		stop_fighting(ch);
		// This might be a bad idea...cause track calls move, which calls track, which...
		if (IS_NPC(chaser))
		{
			retval = chaser->do_track(chaser->mobdata->hated.split(' '));
			if (SOMEONE_DIED(retval))
				return retval;
		}
	}

	if (IS_AFFECTED(ch, AFF_SNEAK) || (IS_AFFECTED(ch, AFF_FOREST_MELD) && (DC::getInstance()->world[ch->in_room].sector_type == SECT_FOREST || DC::getInstance()->world[ch->in_room].sector_type == SECT_SWAMP)))
	{
		act("$n sneaks into the room.", ch, 0, 0, TO_ROOM, GODS);
		act("$n sneaks into the room.", ch, 0, 0, TO_GROUP, INVIS_NULL);
	}
	else
		act("$n has arrived.", ch, 0, 0, TO_ROOM, INVIS_NULL);

	do_look(ch, "\0", 15);

	// Elemental stuff goes HERE
	if (IS_NPC(ch))
	{
		int a = DC::getInstance()->mob_index[ch->mobdata->nr].virt;
		// code a bit repeaty, but whatever ;)
		if (a == 88 && DC::getInstance()->world[ch->in_room].sector_type == SECT_UNDERWATER)
		{
			act("Unable to survive underwater, $n returns to the elemental plane of fire.", ch, 0, 0, TO_ROOM, 0);
			extract_char(ch, true);
			return eSUCCESS | eCH_DIED;
		}
		if (a == 89 && DC::getInstance()->world[ch->in_room].sector_type == SECT_DESERT)
		{
			act("Unable to survive in the desert, $n returns to the elemental plane of water.", ch, 0, 0, TO_ROOM, 0);
			extract_char(ch, true);
			return eSUCCESS | eCH_DIED;
		}
		if (a == 90 && DC::getInstance()->world[ch->in_room].sector_type == SECT_SWAMP)
		{
			act("Unable to survive in the swamp, $n returns to the elemental plane of air.", ch, 0, 0, TO_ROOM, 0);
			extract_char(ch, true);
			return eSUCCESS | eCH_DIED;
		}
		if (a == 91 && DC::getInstance()->world[ch->in_room].sector_type == SECT_AIR)
		{
			act("Unable to survive in the air, $n returns to the elemental plane of earth.", ch, 0, 0, TO_ROOM, 0);
			extract_char(ch, true);
			return eSUCCESS | eCH_DIED;
		}
	}
	// Elemental stuff ends HERE

	if ((isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_NORTH) && (dir = 0)) || (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_DOWN) && (dir = 5)) || (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_UP) && (dir = 4)) || (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_EAST) && (dir = 1)) || (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_WEST) && (dir = 3)) || (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_SOUTH) && (dir = 2)))
	{
		retval = do_fall(ch, dir);
		if (SOMEONE_DIED(retval))
			return eSUCCESS | eCH_DIED;
	}

	if (isSet(DC::getInstance()->world[ch->in_room].room_flags, UNSTABLE))
	{
		retval = do_unstable(ch);
		if (SOMEONE_DIED(retval))
			return eSUCCESS | eCH_DIED;
	}

	/*
	 if(isSet(DC::getInstance()->world[ch->in_room].sector_type, SECT_FIELD) &&
	 weather_info.sky == SKY_HEAVY_RAIN && !number(0,19)) {
	 do_muddy(ch);
	 }
	 */
	if ((GET_CLASS(ch) == CLASS_BARD && ch->has_skill(SKILL_SONG_HYPNOTIC_HARMONY)) || GET_CLASS(ch) == CLASS_RANGER)
		for (Character *tmp_ch = DC::getInstance()->world[ch->in_room].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
		{
			if (IS_PC(tmp_ch))
				continue;
			if (IS_AFFECTED(tmp_ch, AFF_CHARM))
				continue;
			if (tmp_ch->fighting)
				continue;
			if (isSet(tmp_ch->immune, ISR_CHARM))
				continue;
			if (!number(0, 1))
				continue;

			if (GET_CLASS(ch) == CLASS_BARD && ISSET(tmp_ch->mobdata->actflags, ACT_BARDCHARM))
			{
				act("$N looks at you expectantly, perhaps hoping for a song?", ch, nullptr, tmp_ch, TO_CHAR, 0);
				act("$N looks at $n expectantly, perhaps hoping for a song?", ch, nullptr, tmp_ch, TO_ROOM, INVIS_NULL);
			}
			else if (GET_CLASS(ch) == CLASS_RANGER && ISSET(tmp_ch->mobdata->actflags, ACT_CHARM) && ch->getLevel() >= tmp_ch->getLevel() && CAN_SEE(tmp_ch, ch))
			{
				act("$N moves submissively out of your way.", ch, nullptr, tmp_ch, TO_CHAR, 0);
				act("$N moves submissively out of $n's way.", ch, nullptr, tmp_ch, TO_ROOM, INVIS_NULL);
			}
		}

	// let our mobs know they're here
	retval = mprog_entry_trigger(ch);
	if (SOMEONE_DIED(retval))
		return retval | eSUCCESS;
	retval = mprog_greet_trigger(ch);
	if (SOMEONE_DIED(retval))
		return retval | eSUCCESS;
	retval = oprog_greet_trigger(ch);
	if (SOMEONE_DIED(retval))
		return retval | eSUCCESS;

	return eSUCCESS;
}

int attempt_move(Character *ch, int cmd, int is_retreat)
{
	char tmp[80];
	int return_val;
	int was_in = ch->in_room;
	struct follow_type *k, *next_dude;

	if (ch->brace_at)
	{
		csendf(ch, "You can't move and brace the %s at the same time!\r\n", fname(ch->brace_at->keyword).toStdString().c_str());
		return eFAILURE;
	}

	--cmd;

	if (!DC::getInstance()->world[ch->in_room].dir_option[cmd])
	{
		ch->sendln("You can't go that way.");
		return eFAILURE;
	}

	if (isSet(EXIT(ch, cmd)->exit_info, EX_CLOSED))
	{
		if (isSet(EXIT(ch, cmd)->exit_info, EX_HIDDEN))
			ch->sendln("You can't go that way.");
		else if (EXIT(ch, cmd)->keyword)
			csendf(ch, "The %s seems to be closed.\r\n", fname(EXIT(ch, cmd)->keyword).toStdString().c_str());
		else
			ch->sendln("It seems to be closed.");
		return eFAILURE;
	}

	if (EXIT(ch, cmd)->to_room == DC::NOWHERE)
	{
		ch->sendln("Alas, you can't go that way.");
		return eFAILURE;
	}

	if (!ch->followers && !ch->master)
	{
		try
		{
			return_val = do_simple_move(ch, cmd, false);
		}
		catch (...)
		{
			logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Error performing movement in room %d.", ch->in_room);
			return_val = eFAILURE;
		}

		if (SOMEONE_DIED(return_val) || !isSet(return_val, eSUCCESS))
			return return_val;
		if (!IS_AFFECTED(ch, AFF_SNEAK))
			return_val = ambush(ch);
		if (SOMEONE_DIED(return_val) || !isSet(return_val, eSUCCESS))
			return return_val;
		return check_ethereal_focus(ch, ETHEREAL_FOCUS_TRIGGER_MOVE);
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master) && (ch->in_room == ch->master->in_room))
	{
		ch->sendln("You are unable to abandon your master.");
		act("$n trembles as $s mind attempts to free itself from its magical bondage.", ch, 0, 0, TO_ROOM, 0);
		if (IS_PC(ch->master) && GET_CLASS(ch->master) == CLASS_BARD)
		{
			ch->master->sendln("You struggle to maintain control.");
			/*
			 if (GET_KI(ch->master) < 5) {
			 ch->add_memory(GET_NAME(ch->master), 'h');
			 stop_follower(ch, BROKE_CHARM);
			 //ch->add_memory(GET_NAME(ch->master), 'h');
			 do_say(ch, "Hey! You tricked me!", CMD_DEFAULT);
			 ch->master->sendln("You lose control.");
			 }
			 else
			 GET_KI(ch->master) -= 5;
			 */
		}
		return eFAILURE;
	}

	try
	{
		return_val = do_simple_move(ch, cmd, true);
	}
	catch (...)
	{
		logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Error performing movement in room %d.", ch->in_room);
		return_val = eFAILURE;
	}

	// this may cause problems with leader being ambushed, dying, and group not moving
	// but we have to be careful in case leader was a mob (no longer valid memory)
	if (SOMEONE_DIED(return_val) || !isSet(return_val, eSUCCESS))
	{
		/*
		 sprintf(tmp, "%s group failed to follow. (died: %d ret: %d)",
		 GET_NAME(ch), SOMEONE_DIED(return_val), return_val);
		 logentry(tmp, OVERSEER, DC::LogChannel::LOG_BUG);
		 */
		return return_val;
	}

	if (ch->followers && !isSet(ch->combat, COMBAT_FLEEING))
	{
		for (k = ch->followers; k; k = next_dude)
		{ // no following a fleer
			next_dude = k->next;
			if ((was_in == k->follower->in_room) && ((is_retreat && k->follower->getPosition() > position_t::RESTING) || (k->follower->getPosition() >= position_t::STANDING)))
			{
				if (IS_AFFECTED(k->follower, AFF_NO_FLEE))
				{
					if (k->follower->affected_by_spell(SPELL_IRON_ROOTS))
						k->follower->sendln("The roots bracing your legs make it impossible to run!");
					else
						k->follower->sendln("Your legs are too tired for running away!");
					continue; // keep going through the groupies
				}
				if (is_retreat && k->follower->fighting && (number(1, 100) < 4) && IS_NPC(k->follower->fighting))
				{

					act("$n notices your intent and moves quickly to block your retreat!", k->follower->fighting, nullptr, k->follower, TO_VICT, 0);
					act("$n notices $N's intent and moves quickly to block $S retreat!", k->follower->fighting, nullptr, k->follower, TO_ROOM, NOTVICT);
					WAIT_STATE(k->follower, 8);
					continue;
				}
				if (CAN_SEE(k->follower, ch))
					sprintf(tmp, "You follow %s.\n\r\n\r", GET_SHORT(ch));
				else
					strcpy(tmp, "You follow someone.\n\r\n\r");
				k->follower->send(tmp);
				// do_move(k->follower, "", cmd + 1);
				char tempcommand[32];
				strcpy(tempcommand, dirs[cmd]);
				if (k->follower->fighting)
					stop_fighting(k->follower);
				k->follower->command_interpreter(tempcommand);
			}
			else
			{
				/*
				 sprintf(tmp, "%s attempted to follow %s but failed. (was_in:%d fol->in_room:%d pos: %d ret: %d",
				 GET_NAME(k->follower), GET_NAME(ch), was_in, k->follower->in_room,
				 k->follower->getPosition(), is_retreat);
				 logentry(tmp, OVERSEER, DC::LogChannel::LOG_BUG);
				 */
			}
		}
	}

	if (was_in != ch->in_room && !IS_AFFECTED(ch, AFF_SNEAK))
		return_val = ambush(ch);

	if (isSet(return_val, eCH_DIED))
		return eSUCCESS | eCH_DIED;

	return eSUCCESS;
}

//   Returns :
//   1 : If success.
//   0 : If fail
//  -1 : If dead.
int do_move(Character *ch, char *argument, int cmd)
{
	return attempt_move(ch, cmd);
}

int do_leave(Character *ch, char *arguement, int cmd)
{
	class Object *k;
	char buf[200];
	int retval;

	for (k = DC::getInstance()->object_list; k; k = k->next)
	{
		if (k->isPortal())
		{
			if (k->getPortalType() == Object::portal_types_t::Permanent || k->getPortalType() == Object::portal_types_t::Temp)
			{
				if (k->in_room != DC::NOWHERE && !k->hasPortalFlagNoLeave())
				{
					if (k->getPortalDestinationRoom() == DC::getInstance()->world[ch->in_room].number || k->obj_flags.value[2] == DC::getInstance()->world[ch->in_room].zone)
					{
						ch->sendln("You exit the area.");
						act("$n has left the area.", ch, 0, 0, TO_ROOM, INVIS_NULL | STAYHIDE);
						retval = move_char(ch, real_room(DC::getInstance()->world[k->in_room].number));
						if (!isSet(retval, eSUCCESS))
						{
							ch->sendln("You attempt to leave, but the door slams in your face!");
							act("$n attempts to leave, but can't!", ch, 0, 0, TO_ROOM, INVIS_NULL | STAYHIDE);
							return eFAILURE;
						}
						do_look(ch, "", CMD_DEFAULT);
						sprintf(buf, "%s walks out of %s.", GET_NAME(ch), k->short_description);
						act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL | STAYHIDE);
						return ambush(ch);
					}
				}
			}
		}
	}

	ch->sendln("There are no exits around!");

	return eFAILURE;
}

int do_enter(Character *ch, char *argument, int cmd)
{
	char buf[MAX_STRING_LENGTH];
	int retval;

	Character *sesame;
	Object *portal = nullptr;

	if ((ch->in_room != DC::NOWHERE) || (ch->in_room))
	{
		one_argument(argument, buf);
	}

	if (!*buf)
	{
		ch->sendln("Enter what?");
		return eFAILURE;
	}

	if ((portal = get_obj_in_list_vis(ch, buf, DC::getInstance()->world[ch->in_room].contents)) == nullptr)
	{
		ch->sendln("Nothing here by that name.");
		return eFAILURE;
	}

	if (!portal->isPortal())
	{
		ch->sendln("You can't enter that.");
		return eFAILURE;
	}

	if (real_room(portal->getPortalDestinationRoom()) == DC::NOWHERE)
	{
		sprintf(buf, "Error in do_enter(), value 0 on object %d < 0", portal->vnum);
		logentry(buf, OVERSEER, DC::LogChannel::LOG_BUG);
		ch->sendln("You can't enter that.");
		return eFAILURE;
	}

	if (DC::getInstance()->world[real_room(portal->getPortalDestinationRoom())].sector_type == SECT_UNDERWATER && !(ch->affected_by_spell(SPELL_WATER_BREATHING) || IS_AFFECTED(ch, AFF_WATER_BREATHING)))
	{
		ch->sendln("You bravely attempt to plunge through the portal - let's hope you have gills!");
		return eSUCCESS;
	}

	if (ch->getLevel() > IMMORTAL && ch->getLevel() < DEITY && isSet(DC::getInstance()->world[real_room(portal->getPortalDestinationRoom())].room_flags, CLAN_ROOM))
	{
		ch->sendln("You may not enter a clanhall at your level.");
		return eFAILURE;
	}

	if (IS_NPC(ch) && ch->master && DC::getInstance()->mob_index[ch->mobdata->nr].virt == 8)
	{
		sesame = ch->master;
		if (isSet(DC::getInstance()->world[real_room(portal->obj_flags.value[0])].room_flags, CLAN_ROOM))
		{
			// Is golem's master not a member of this clan
			if (others_clan_room(sesame, &DC::getInstance()->world[real_room(portal->obj_flags.value[0])]) == true)
			{
				ch->sendln("Your master is not from that clan.");
				act("$n finds $mself unable to enter!", ch, 0, 0, TO_ROOM, 0);
				do_say(ch, "I may not enter.", CMD_DEFAULT);
				return eFAILURE;
			}
		}
	}
	else
	{
		sesame = ch;
	}

	// should probably just combine this with 'if' below it, but i'm lazy
	if (isSet(portal->obj_flags.value[3], Object::portal_flags_t::No_Enter))
	{
		ch->sendln("The portal's destination rebels against you.");
		act("$n finds $mself unable to enter!", ch, 0, 0, TO_ROOM, 0);
		return eFAILURE;
	}

	// Thieves arent allowed through cleric portals
	if (ch->isPlayerObjectThief() && portal->isPortalTypePlayer())
	{
		ch->sendln("Your attempt to transport stolen goods through planes of magic fails!");
		return eFAILURE;
	}

	if (!IS_NPC(ch) && (ch->isPlayerObjectThief() || ch->isPlayerGoldThief() || IS_AFFECTED(ch, AFF_CHAMPION)) && (isSet(DC::getInstance()->world[real_room(portal->obj_flags.value[0])].room_flags, CLAN_ROOM) || (portal->obj_flags.value[0] >= 1900 && portal->obj_flags.value[0] <= 1999 && !portal->obj_flags.value[1])))
	{
		ch->sendln("The portal's destination rebels against you.");
		act("$n finds $mself unable to enter!", ch, 0, 0, TO_ROOM, 0);
		return eFAILURE;
	}

	if (isexact("only", portal->name) && !isexact(GET_NAME(sesame), portal->name))
	{
		ch->sendln("The portal fades when you draw near, then shimmers as you withdraw.");
		return eFAILURE;
	}

	switch (portal->obj_flags.value[1])
	{
	case 0:
		ch->sendln("You reach out tentatively and touch the portal...");
		act("$n reaches out to the glowing dimensional portal....", ch, 0, 0, TO_ROOM, 0);
		break;
	case 1:
	case 2:
		sprintf(buf, "You take a bold step towards %s.\r\n", portal->short_description);
		ch->send(buf);
		sprintf(buf, "%s boldly walks toward %s and disappears.", GET_NAME(ch), portal->short_description);
		act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL | STAYHIDE);
		break;
	case 3:
		ch->sendln("You cannot enter that.");
		return eFAILURE;
	default:
		sprintf(buf, "Error in do_enter(), value 1 on object %d returned default case", portal->vnum);
		logentry(buf, OVERSEER, DC::LogChannel::LOG_BUG);
		return eFAILURE;
	}

	retval = move_char(ch, real_room(portal->obj_flags.value[0]));

	if (SOMEONE_DIED(retval))
		return retval;

	if (!isSet(retval, eSUCCESS))
	{
		ch->sendln("You recoil in pain as the portal slams shut!");
		act("$n recoils in pain as the portal slams shut!", ch, 0, 0, TO_ROOM, 0);
	}

	switch (portal->obj_flags.value[1])
	{
	case 0:
		do_look(ch, "", CMD_DEFAULT);
		WAIT_STATE(ch, DC::PULSE_VIOLENCE);
		ch->sendln("\n\rYou are momentarily dazed from the dimensional shift.");
		act("The portal glows brighter for a second as $n appears beside you.", ch, 0, 0, TO_ROOM, 0);
		break;
	case 1:
	case 2:
		sprintf(buf, "%s has entered %s.", GET_NAME(ch), portal->short_description);
		act(buf, ch, 0, 0, TO_ROOM, STAYHIDE);
		do_look(ch, "", CMD_DEFAULT);
		break;
	case 3:
		break;
	default:
		break;
	}

	return ambush(ch);
}

int move_char(Character *ch, int dest, bool stop_all_fighting)
{
	if (!ch)
	{
		logentry(QStringLiteral("Error in move_char(), nullptr character"), OVERSEER, DC::LogChannel::LOG_BUG);
		return eINTERNAL_ERROR;
	}

	int origination = ch->in_room;

	if (ch->in_room != DC::NOWHERE)
	{
		// Couldn't move char from the room
		if (char_from_room(ch, stop_all_fighting) == 0)
		{
			logentry(QStringLiteral("Error in move_char(), character not DC::NOWHERE, but couldn't be moved."),
					 OVERSEER, DC::LogChannel::LOG_BUG);
			return eINTERNAL_ERROR;
		}
	}

	// Couldn't move char to new room
	if (char_to_room(ch, dest, stop_all_fighting) == 0)
	{
		// Now we have real problems
		if (char_to_room(ch, origination) == 0)
		{
			qFatal("%s", qUtf8Printable(QStringLiteral("Error in move_char(), character stuck in DC::NOWHERE: %1.\n").arg(GET_NAME(ch))));
		}
		logf(OVERSEER, DC::LogChannel::LOG_BUG, "Error in move_char(), could not move %s to %d.", GET_NAME(ch), DC::getInstance()->world[dest].number);
		return eINTERNAL_ERROR;
	}

	// At this point, the character is happily in the new room
	return eSUCCESS;
}

int do_climb(Character *ch, char *argument, int cmd)
{
	char buf[MAX_INPUT_LENGTH];
	Object *obj = nullptr;

	one_argument(argument, buf);

	if (!*buf)
	{
		ch->sendln("Climb what?");
		return eSUCCESS;
	}

	if (!(obj = get_obj_in_list_vis(ch, buf, DC::getInstance()->world[ch->in_room].contents)))
	{
		ch->sendln("Climb what?");
		return eSUCCESS;
	}

	if (obj->obj_flags.type_flag != ITEM_CLIMBABLE)
	{
		ch->sendln("You can't climb that.");
		return eSUCCESS;
	}

	int dest = obj->obj_flags.value[0];

	if (real_room(dest) < 0)
	{
		logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Error in do_climb(), illegal destination in object %d.", obj->vnum);
		ch->sendln("You can't climb that.");
		return eFAILURE | eINTERNAL_ERROR;
	}

	act("$n carefully climbs $p.", ch, obj, 0, TO_ROOM, INVIS_NULL);
	sprintf(buf, "You carefully climb %s.\r\n", obj->short_description);
	ch->send(buf);
	int retval = move_char(ch, real_room(dest));

	if (SOMEONE_DIED(retval))
		return retval;

	act("$n carefully climbs $p.", ch, obj, 0, TO_ROOM, INVIS_NULL);
	do_look(ch, "", CMD_DEFAULT);

	return eSUCCESS;
}

// The End

int ambush(Character *ch)
{
	Character *i, *next_i;
	int retval;

	for (i = DC::getInstance()->world[ch->in_room].people; i; i = next_i)
	{
		next_i = i->next_in_room;

		if (i == ch || i->ambush.isEmpty() || !CAN_SEE(i, ch) || i->fighting)
			continue;

		if (i->getPosition() <= position_t::RESTING ||
			i->getPosition() == position_t::FIGHTING ||
			IS_AFFECTED(i, AFF_PARALYSIS) ||
			(isSet(DC::getInstance()->world[i->in_room].room_flags, SAFE) &&
			 !IS_AFFECTED(ch, AFF_CANTQUIT)))
			continue;
		if (!IS_NPC(i) && !i->desc) // don't work if I'm linkdead
			continue;
		if (isexact(i->ambush, GET_NAME(ch)))
		{

			if (!i->canPerform(SKILL_AMBUSH))
			{
				continue;
			}

			if (IS_AFFECTED(ch, AFF_ALERT))
			{
				i->sendln("Your target is far too alert to accomplish an ambush!");
				continue;
			}

			if (!charge_moves(ch, SKILL_AMBUSH))
				return eSUCCESS;

			if (skill_success(i, ch, SKILL_AMBUSH))
			{
				//         act("$n ambushes $N in a brilliant surprise attack!", i, 0, ch, TO_ROOM, NOTVICT);
				//         act("$n ambushes you as you enter the room!", i, 0, ch, TO_VICT, 0);
				//         act("You ambush $N with a brilliant surprise attack!", i, 0, ch, TO_CHAR, 0);
				retval = damage(i, ch, i->getLevel() * 10, TYPE_HIT, SKILL_AMBUSH, 0);
				if (isSet(retval, eVICT_DIED))
					return (eSUCCESS | eCH_DIED); // ch = damage vict
				if (isSet(retval, eCH_DIED))
					return (eSUCCESS); // doesn't matter, but don't lag vict
				if (!IS_NPC(i) && isSet(i->player->toggles, Player::PLR_WIMPY))
					WAIT_STATE(i, DC::PULSE_VIOLENCE * 3);
				else
					WAIT_STATE(i, DC::PULSE_VIOLENCE * 2);
				WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1);
			}
			// we continue instead of breaking in case there are any OTHER rangers in the room
		}
	}
	return eSUCCESS;
}