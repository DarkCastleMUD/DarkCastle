/***************************************************************************
 *  file: limits.c , Limit and gain control module.        Part of DIKUMUD *
 *  Usage: Procedures controling gain and limit.                           *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: limits.cpp,v 1.99 2014/07/04 22:00:04 jhhudso Exp $ */

#include <cstdio>
#include <cstring>

#ifdef BANDWIDTH
#include "DC/bandwidth.h"
#endif
#include "DC/room.h"
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/mobile.h"
#include "DC/isr.h"
#include "DC/spells.h" // TYPE
#include "DC/player.h"
#include "DC/DC.h"
#include "DC/connect.h"
#include "DC/db.h"	  // exp_table
#include "DC/fight.h" // damage
#include "DC/ki.h"
#include "DC/game_portal.h"
#include "DC/act.h"
#include "DC/handler.h"
#include "DC/race.h"
#include "DC/returnvals.h"
#include "DC/interp.h"
#include "DC/clan.h" //totem
#include "DC/vault.h"
#include "DC/inventory.h"
#include "DC/const.h"
#include "DC/corpse.h"

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

	if (age < 15)
		return (p0); /* < 15   */
	else if (age <= 29)
		return (int)(p1 + (((age - 15) * (p2 - p1)) / 15)); /* 15..29 */
	else if (age <= 44)
		return (int)(p2 + (((age - 30) * (p3 - p2)) / 15)); /* 30..44 */
	else if (age <= 59)
		return (int)(p3 + (((age - 45) * (p4 - p3)) / 15)); /* 45..59 */
	else if (age <= 79)
		return (int)(p4 + (((age - 60) * (p5 - p4)) / 20)); /* 60..79 */
	else
		return (p6); /* >= 80 */
}

/* The three MAX functions define a characters Effective maximum */
/* Which is NOT the same as the ch->max_xxxx !!!          */
int32_t mana_limit(Character *ch)
{
	int max;

	if (IS_PC(ch))
		max = (ch->max_mana);
	else
		max = (ch->max_mana);

	return (max);
}

// Previously any NPC got 0 returned.
int32_t ki_limit(Character *ch)
{
	return ch->max_ki;
}

int32_t hit_limit(Character *ch)
{
	int max;

	if (IS_PC(ch))
		max = (ch->max_hit) + (graf(ch->age().year, 2, 4, 17, 14, 8, 4, 3));
	else
		max = (ch->max_hit);

	/* Class/Level calculations */

	/* Skill/Spell calculations */

	return (max);
}

const int mana_regens[] = {0, 13, 13, 1, 1, 10, 9, 1, 1, 9, 1, 13, 0, 0};

/* manapoint gain pr. game hour */
int Character::mana_gain_lookup(void)
{
	int gain = 0;
	int divisor = 1;
	int modifier;

	if (IS_NPC(this))
		gain = this->getLevel();
	else
	{
		//    gain = graf(age().year, 2,3,4,6,7,8,9);

		gain = (int)(this->max_mana * (float)mana_regens[GET_CLASS(this)] / 100);
		switch (this->getPosition())
		{
		case position_t::SLEEPING:
			divisor = 1;
			break;
		case position_t::RESTING:
			divisor = 2;
			break;
		case position_t::SITTING:
			divisor = 2;
			break;
		default:
			divisor = 3;
			break;
		}

		if (GET_CLASS(this) == CLASS_MAGIC_USER ||
			GET_CLASS(this) == CLASS_ANTI_PAL || GET_CLASS(this) == CLASS_RANGER)
		{
			if (GET_INT(this) < 0)
				modifier = int_app[0].mana_regen;
			else
				modifier = int_app[GET_INT(this)].mana_regen;

			modifier += GET_INT(this);
		}
		else
		{
			modifier = wis_app[GET_WIS(this)].mana_regen;
			modifier += GET_WIS(this);
		}
		gain += modifier;
	}

	if (((GET_COND(this, FULL) == 0) || (GET_COND(this, THIRST) == 0)) && this->getLevel() < 60)
		gain >>= 2;
	gain /= 4;
	gain /= divisor;
	gain += MIN(age().year, 100) / 5;
	if (this->getLevel() < 50)

		gain = (int)((float)gain * (2.0 - (float)this->getLevel() / 50.0));

	if (this->mana_regen > 0)
		gain += this->mana_regen;
	if (this->in_room >= 0)
		if (isSet(DC::getInstance()->world[this->in_room].room_flags, SAFE) || check_make_camp(this->in_room))
			gain = (int)(gain * 1.25);

	if (this->mana_regen < 0)
		gain += this->mana_regen;
	return MAX(1, gain);
}

const int hit_regens[] = {0, 7, 7, 9, 10, 8, 9, 12, 9, 8, 8, 7, 0, 0};

int Character::hit_gain(position_t position, bool improve)
{
	int gain = 1;
	struct affected_type *af;
	int divisor = 1;
	int learned = has_skill(SKILL_ENHANCED_REGEN);
	/* Neat and fast */
	if (IS_NPC(this))
	{
		if (this->fighting)
			gain = (GET_MAX_HIT(this) / 24);
		else
			gain = (GET_MAX_HIT(this) / 6);
	}
	/* PC's */
	else
	{
		gain = (int)(this->max_hit * (float)hit_regens[GET_CLASS(this)] / 100);

		/* Position calculations    */

		switch (position)
		{
		case position_t::SLEEPING:
			divisor = 1;
			break;
		case position_t::RESTING:
			divisor = 2;
			break;
		case position_t::SITTING:
			divisor = 2;
			break;
		default:
			divisor = 3;
			break;
		}

		if (gain < 1)
			gain = 1;

		if ((af = affected_by_spell(SPELL_RAPID_MEND)))
			gain += af->modifier;

		// con multiplier modifier 15 = 1.0  30 = 1.45 (.03 increments)
		/*    if(GET_CON(this) > 15)
		 gain = (int)(gain * ((float)1+ (.03 * (GET_CON(this) - 15.0))));

		 if(GET_CLASS(this) == CLASS_MAGIC_USER || GET_CLASS(this) == CLASS_CLERIC || GET_CLASS(this) == CLASS_DRUID)
		 gain = (int)((float)gain * 0.7);*/

		if (GET_CON(this) < 0)
			gain += con_app[0].hp_regen;
		else
			gain += con_app[GET_CON(this)].hp_regen;

		gain += GET_CON(this);
	}
	if (ISSET(this->affected_by, AFF_REGENERATION))
		gain += (gain / 2);

	if (learned && (!improve || skill_success(nullptr, SKILL_ENHANCED_REGEN)))
		gain += 3 + learned / 5;

	if (((GET_COND(this, FULL) == 0) || (GET_COND(this, THIRST) == 0)) && this->getLevel() < 60)
		gain >>= 2;

	gain /= 4;
	//  gain -= MIN(age().year,100) / 10;

	gain /= divisor;
	if (this->hit_regen > 0)
		gain += this->hit_regen;
	if (this->getLevel() < 50)
		gain = (int)((float)gain * (2.0 - (float)this->getLevel() / 50.0));

	if (this->in_room >= 0)
		if (isSet(DC::getInstance()->world[this->in_room].room_flags, SAFE) || check_make_camp(this->in_room))
			gain = (int)(gain * 1.5);
	if (this->hit_regen < 0)
		gain += this->hit_regen;
	return MAX(1, gain);
}

int Character::move_gain_lookup(int extra)
/* move gain pr. game hour */
{
	int gain;
	int divisor = 100000;
	int learned = has_skill(SKILL_ENHANCED_REGEN);
	struct affected_type *af;
	bool improve = true;
	if (extra == 777)
		improve = false;

	if (isNPC())
	{
		return getLevel();
	}
	else
	{
		//	gain = graf(ch->age().year, 4,5,6,7,4,3,2);
		gain = (int)(max_move * 0.15);
		//	gain /= 2;
		switch (getPosition())
		{
		case position_t::SLEEPING:
			divisor = 1;
			break;
		case position_t::RESTING:
			divisor = 2;
			break;
		default:
			divisor = 3;
			break;
		}
		gain += GET_DEX(this);

		if (GET_CON(this) < 0)
			gain += con_app[0].move_regen;
		else
			gain += con_app[GET_CON(this)].move_regen;

		if ((af = affected_by_spell(SPELL_RAPID_MEND)))
			gain += (int)(af->modifier * 1.5);
	}

	if (((GET_COND(this, FULL) == 0) || (GET_COND(this, THIRST) == 0)) && this->getLevel() < 60)
		gain >>= 2;
	gain /= divisor;
	gain -= MIN(100, this->age().year) / 10;

	if (this->move_regen > 0)
		gain += this->move_regen;

	if (learned && (!improve || skill_success(nullptr, SKILL_ENHANCED_REGEN)))
		gain += 3 + learned / 10;

	if (this->getLevel() < 50)
		gain = (int)((float)gain * (2.0 - (float)this->getLevel() / 50.0));

	if (this->in_room >= 0)
		if (isSet(DC::getInstance()->world[this->in_room].room_flags, SAFE) || check_make_camp(this->in_room))
			gain = (int)(gain * 1.5);
	if (this->move_regen < 0)
		gain += this->move_regen;

	return MAX(1, gain);
}

void redo_hitpoints(Character *ch)
{
	/*struct affected_type *af;*/
	int i, j, bonus = 0;

	ch->max_hit = ch->raw_hit;
	for (i = 16; i < GET_CON(ch); i++)
		bonus += (i * i) / 30;
	//  bonus = (GET_CON(ch) * GET_CON(ch)) / 30;

	ch->max_hit += bonus;
	struct affected_type *af = ch->affected;

	while (af)
	{
		if (af->location == APPLY_HIT)
			ch->max_hit += af->modifier;
		af = af->next;
	}

	for (i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
			for (j = 0; j < ch->equipment[i]->num_affects; j++)
			{
				if (ch->equipment[i]->affected[j].location == APPLY_HIT)
					affect_modify(ch, ch->equipment[i]->affected[j].location, ch->equipment[i]->affected[j].modifier, -1, true);
			}
	}
	add_totem_stats(ch, APPLY_HIT);
}

void redo_mana(Character *ch)

{
	/*struct affected_type *af;*/
	int i, j, bonus = 0, stat = 0;
	if (IS_NPC(ch))
		return;
	ch->max_mana = ch->raw_mana;

	if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL || GET_CLASS(ch) == CLASS_RANGER)
		stat = GET_INT(ch);
	else
		stat = GET_WIS(ch);

	for (i = 16; i < stat; i++)
		bonus += (i * i) / 30;

	if ((GET_CLASS(ch) == CLASS_WARRIOR) || (GET_CLASS(ch) == CLASS_THIEF) || (GET_CLASS(ch) == CLASS_BARBARIAN) || (GET_CLASS(ch) == CLASS_MONK))
		bonus = 0;

	ch->max_mana += bonus;

	struct affected_type *af;
	af = ch->affected;
	while (af)
	{
		if (af->location == APPLY_MANA)
			ch->max_mana += af->modifier;
		af = af->next;
	}
	for (i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
			for (j = 0; j < ch->equipment[i]->num_affects; j++)
			{
				if (ch->equipment[i]->affected[j].location == APPLY_MANA)
					affect_modify(ch, ch->equipment[i]->affected[j].location, ch->equipment[i]->affected[j].modifier, -1, true);
			}
	}
	add_totem_stats(ch, APPLY_MANA);
}

void redo_ki(Character *ch)
{
	int i, j;
	ch->max_ki = ch->raw_ki;
	if (GET_CLASS(ch) == CLASS_MONK)
		ch->max_ki += GET_WIS(ch) > 15 ? GET_WIS(ch) - 15 : 0;
	else if (GET_CLASS(ch) == CLASS_BARD)
		ch->max_ki += GET_INT(ch) > 15 ? GET_INT(ch) - 15 : 0;

	struct affected_type *af;
	af = ch->affected;
	while (af)
	{
		if (af->location == APPLY_KI)
			ch->max_ki += af->modifier;
		af = af->next;
	}

	for (i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
			for (j = 0; j < ch->equipment[i]->num_affects; j++)
			{
				if (ch->equipment[i]->affected[j].location == APPLY_KI)
					affect_modify(ch, ch->equipment[i]->affected[j].location, ch->equipment[i]->affected[j].modifier, -1, true);
			}
	}
	add_totem_stats(ch, APPLY_KI);
}

/* Gain maximum in various */
void advance_level(Character *ch, int is_conversion)
{
	int add_hp = 0;
	int add_mana = 1;
	int add_moves = 0;
	int add_ki = 0;
	int add_practices;
	int i;
	char buf[MAX_STRING_LENGTH];

	auto effective_level = MAX(ch->getLevel(), 1);
	auto effective_con = MAX(GET_CON(ch), 2);
	switch (GET_CLASS(ch))
	{
	case CLASS_MAGIC_USER:
		add_ki += (effective_level % 2);
		add_hp += number(3, 6);
		add_mana += number(5, 10);
		add_moves += number(1, (effective_con / 2));
		break;

	case CLASS_CLERIC:
		add_ki += (effective_level % 2);
		add_hp += number(4, 8);
		add_mana += number(4, 9);
		add_moves += number(1, (effective_con / 2));
		break;

	case CLASS_THIEF:
		add_ki += (effective_level % 2);
		add_hp += number(4, 11);
		add_moves += number(1, (effective_con / 2));
		break;

	case CLASS_WARRIOR:
		add_ki += (effective_level % 2);
		add_hp += number(14, 18);
		add_moves += number(1, (effective_con / 2));
		break;

	case CLASS_ANTI_PAL:
		add_ki += (effective_level % 2);
		add_hp += number(8, 12);
		add_mana += number(3, 5);
		add_moves += number(1, (effective_con / 2));
		break;

	case CLASS_PALADIN:
		add_ki += (effective_level % 2);
		add_hp += number(10, 14);
		add_mana += number(2, 4);
		add_moves += number(1, (effective_con / 2));
		break;

	case CLASS_BARBARIAN:
		add_ki += (effective_level % 2);
		add_hp += number(16, 20);
		add_moves += number(1, (effective_con / 2));
		break;

	case CLASS_MONK:
		add_ki += 1;
		add_hp += number(10, 14);
		add_moves += number(1, (effective_con / 2));
		GET_AC(ch) += -2;
		break;

	case CLASS_RANGER:
		add_ki += (effective_level % 2);
		add_hp += number(8, 12);
		add_mana += number(3, 5);
		add_moves += number(1, (effective_con / 2));
		break;

	case CLASS_BARD:
		add_ki += 1;
		add_hp += number(6, 10);
		add_mana += 0;
		add_moves += number(1, (effective_con / 2));
		break;

	case CLASS_DRUID:
		add_ki += (effective_level % 2);
		;
		add_hp += number(5, 9);
		add_mana += number(4, 9);
		add_moves += number(1, (effective_con / 2));
		break;

	default:
		logentry(QStringLiteral("Unknown class in advance level?"), OVERSEER, DC::LogChannel::LOG_BUG);
		return;
	}

	/*  if ((GET_CLASS(ch) == CLASS_MAGIC_USER) ||
	 (GET_CLASS(ch) == CLASS_RANGER) ||
	 (GET_CLASS(ch) == CLASS_ANTI_PAL))
	 add_mana +=  int_app[GET_INT(ch)].mana_gain;
	 else if (GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_DRUID ||
	 GET_CLASS(ch) == CLASS_PALADIN)
	 add_mana += wis_app[GET_WIS(ch)].mana_gain;
	 */
	if (GET_CON(ch) < 0)
		add_hp += con_app[0].hp_gain;
	else
		add_hp += con_app[GET_CON(ch)].hp_gain;

	if (GET_DEX(ch) < 0)
		add_moves += dex_app[0].move_gain;
	else
		add_moves += dex_app[GET_DEX(ch)].move_gain;

	add_hp = MAX(1, add_hp);
	add_mana = MAX(0, add_mana);
	add_moves = MAX(1, add_moves);
	add_practices = 1 + wis_app[GET_WIS(ch)].bonus;

	// hp and mana have stat bonuses related to level so have to have their stuff recalculated
	ch->raw_hit += add_hp;
	ch->raw_mana += add_mana;
	redo_hitpoints(ch);
	redo_mana(ch);
	// move and ki aren't stat related, so we just add directly to the totals
	ch->raw_move += add_moves;
	ch->max_move += add_moves;

	ch->raw_ki += add_ki;
	ch->max_ki += add_ki;
	redo_ki(ch); // Ki gets level bonuses now
	if (!IS_NPC(ch) && !is_conversion)
		ch->player->practices += add_practices;

	sprintf(buf, "Your gain is: %d/%d hp, %d/%d m, %d/%d mv, %d/%d prac, %d/%d ki.\r\n", add_hp, GET_MAX_HIT(ch), add_mana, GET_MAX_MANA(ch), add_moves,
			GET_MAX_MOVE(ch),
			IS_NPC(ch) ? 0 : add_practices, IS_NPC(ch) ? 0 : ch->player->practices, add_ki, GET_MAX_KI(ch));
	if (!is_conversion)
		ch->send(buf);

	if (effective_level % 3 == 0)
		for (int i = 0; i <= SAVE_TYPE_MAX; i++)
			ch->saves[i]++;

	ch->fillHP();
	GET_MANA(ch) = GET_MAX_MANA(ch);
	ch->setMove(GET_MAX_MOVE(ch));
	GET_KI(ch) = GET_MAX_KI(ch);

	if (effective_level > IMMORTAL)
		for (i = 0; i < 3; i++)
			ch->conditions[i] = -1;

	if (effective_level > 10 && !isSet(ch->player->toggles, Player::PLR_REMORTED))
	{
		struct vault_data *vault = has_vault(GET_NAME(ch));
		if (vault)
		{
			ch->sendln("10 lbs has been added to your vault!");
			vault->size += 10;
			save_vault(vault->owner);
		}
	}

	if (effective_level == 6)
		ch->sendln("You are now able to participate in pkilling!\n\rRead HELP PKILL for more information.");
	if (effective_level == 10)
	{
		ch->sendln("You have been given a vault in which to place your valuables!\n\rRead HELP VAULT for more information.");
		add_new_vault(GET_NAME(ch), 0);
	}
	if (effective_level == 11)
		ch->sendln("It now costs you $B$5gold$R every time you recall.");
	if (effective_level == 20)
		ch->sendln("You will no longer keep your equipment when you suffer a death to a mob.\n\rThere is now a chance you may lose attribute points when you die to a mob.\n\rRead HELP RDEATH and HELP STAT LOSS for more information.");
	if (effective_level == 40)
		ch->sendln("You are now able to use the Anonymous command. See \"HELP ANON\" for details.");
	if (effective_level == 50)
		ch->sendln("The protective covenant of your corpse weakens, upon death players may steal 1 item from you. (See help LOOT for details)");
}

void gain_exp(Character *ch, int64_t gain)
{
	int x = 0;
	int64_t y;

	if (ch->isImmortalPlayer())
		return;

	y = exp_table[ch->getLevel() + 1];

	if (GET_EXP(ch) >= y)
		x = 1;

	/*  if(GET_EXP(ch) > 2000000000)
	 {
	 ch->sendln("You have hit the 2 billion xp cap.  Convert or meta chode.");
	 return;
	 }*/
	GET_EXP(ch) += gain;
	if (GET_EXP(ch) < 0)
		GET_EXP(ch) = 0;

	void golem_gain_exp(Character * ch);

	if (IS_PC(ch) && ch->player->golem && ch->in_room == ch->player->golem->in_room) // Golems get mage's exp, when they're in the same room
		gain_exp(ch->player->golem, gain);

	if (IS_NPC(ch) && DC::getInstance()->mob_index[ch->mobdata->vnum].vnum == 8) // it's a golem
		golem_gain_exp(ch);

	if (IS_NPC(ch))
		return;

	if (!x && GET_EXP(ch) >= y)
	{
		ch->sendln("You now have enough experience to level!");
		if (ch->getLevel() == 1)
			csendf(ch, "$B$2An acolyte of Pirahna tells you, 'To find the way to your guild, young %s, please read $7HELP GUILD$2'$R\n\r",
				   pc_clss_types[GET_CLASS(ch)]);
	}

	return;
}

void gain_exp_regardless(Character *ch, int gain)
{
	GET_EXP(ch) += (int64_t)gain;

	if (GET_EXP(ch) < 0)
		GET_EXP(ch) = 0;

	if (IS_NPC(ch))
		return;

	while (GET_EXP(ch) >= (int32_t)exp_table[ch->getLevel() + 1])
	{
		ch->send("You raise a level!!  ");
		ch->incrementLevel();
		advance_level(ch, 0);
	}

	return;
}

void gain_condition(Character *ch, int condition, int value)
{
	bool intoxicated;

	//    if(GET_COND(ch, condition)==-1) /* No change */
	//	return;

	if (condition == FULL && IS_AFFECTED(ch, AFF_BOUNT_SONNET_HUNGER))
		return;
	if (condition == THIRST && IS_AFFECTED(ch, AFF_BOUNT_SONNET_THIRST))
		return;

	intoxicated = (GET_COND(ch, DRUNK) > 0);

	GET_COND(ch, condition) += value;

	GET_COND(ch, condition) = MAX(0, (int)GET_COND(ch, condition));
	GET_COND(ch, condition) = MIN(24, (int)GET_COND(ch, condition));

	if (GET_COND(ch, condition) || ch->getLevel() >= 60)
		return;

	switch (condition)
	{
	case FULL:
	{
		ch->sendln("You are hungry.");
		return;
	}
	case THIRST:
	{
		ch->sendln("You are thirsty.");
		return;
	}
	case DRUNK:
	{
		if (intoxicated)
			ch->sendln("You are now sober.");
		return;
	}
	default:
		break;
	}

	// just for fun
	if (1 == number(1, 2000))
	{
		ch->sendln("You are horny");
	}
}

void food_update(void)
{
	class Object *bring_type_to_front(Character * ch, int item_type);
	int do_eat(Character * ch, char *argument, int cmd);
	int do_drink(Character * ch, char *argument, int cmd);
	int FOUNTAINisPresent(Character * ch);

	class Object *food = nullptr;

	const auto &character_list = (dynamic_cast<DC *>(DC::instance()))->character_list;
	for (const auto &i : character_list)
	{
		if (i->affected_by_spell(SPELL_PARALYZE))
			continue;
		int amt = -1;
		if (i->equipment[WEAR_FACE] && i->equipment[WEAR_FACE]->vnum == 536)
			amt = -3;
		gain_condition(i, FULL, amt);
		if (!GET_COND(i, FULL) && i->getLevel() < 60)
		{ // i'm hungry
			if (!IS_NPC(i) && isSet(i->player->toggles, Player::PLR_AUTOEAT) && (i->getPosition() > position_t::SLEEPING))
			{
				if (IS_DARK(i->in_room) && !IS_NPC(i) && !i->player->holyLite && !i->affected_by_spell(SPELL_INFRAVISION))
					i->sendln("It's too dark to see what's safe to eat!");
				else if (FOUNTAINisPresent(i))
					do_drink(i, "fountain", CMD_DEFAULT);
				else if ((food = bring_type_to_front(i, ITEM_FOOD)))
					do_eat(i, food->name, CMD_DEFAULT);
				else
					i->sendln("You are out of food.");
			}
		}
		gain_condition(i, DRUNK, -1);
		gain_condition(i, THIRST, amt);
		if (!GET_COND(i, THIRST) && i->getLevel() < 60)
		{ // i'm thirsty
			if (!IS_NPC(i) && isSet(i->player->toggles, Player::PLR_AUTOEAT) && (i->getPosition() > position_t::SLEEPING))
			{
				if (IS_DARK(i->in_room) && !IS_NPC(i) && !i->player->holyLite && !i->affected_by_spell(SPELL_INFRAVISION))
					i->sendln("It's too dark to see if there's any potable liquid around!");
				else if (FOUNTAINisPresent(i))
					do_drink(i, "fountain", CMD_DEFAULT);
				else if ((food = bring_type_to_front(i, ITEM_DRINKCON)))
					do_drink(i, food->name, CMD_DEFAULT);
				else
					i->sendln("You are out of drink.");
			}
		}
	}
	DC::getInstance()->removeDead();
}

// Update the HP of mobs and players
// Also clears out any linkdead level 1s
void point_update(void)
{
	/* characters */
	const auto &character_list = DC::getInstance()->character_list;
	for (const auto &i : character_list)
	{
		if (i->in_room == DC::NOWHERE)
			continue;
		if (i->affected_by_spell(SPELL_POISON))
			continue;

		int a;
		Character *temp;
		if (IS_PC(i) && ISSET(i->affected_by, AFF_HIDE) && (a = i->has_skill(SKILL_HIDE)))
		{
			int o;
			for (o = 0; o < MAX_HIDE; o++)
				i->player->hiding_from[o] = nullptr;
			o = 0;
			for (temp = DC::getInstance()->world[i->in_room].people; temp; temp = temp->next_in_room)
			{
				if (i == temp)
					continue;
				if (o >= MAX_HIDE)
					break;
				if (number(1, 101) > a) // Failed.
				{
					i->player->hiding_from[o] = temp;
					i->player->hide[o++] = false;
				}
				else
				{
					i->player->hiding_from[o] = temp;
					i->player->hide[o++] = true;
				}
			}
		}

		// only heal linkalive's and mobs
		if (i->getPosition() > position_t::DEAD && (IS_NPC(i) || i->desc))
		{
			i->setHP(MIN(i->getHP() + i->hit_gain(), hit_limit(i)));

			GET_MANA(i) = MIN(GET_MANA(i) + i->mana_gain_lookup(), mana_limit(i));

			i->setMove(MIN(GET_MOVE(i) + i->move_gain_lookup(), i->move_limit()));
			GET_KI(i) = MIN(GET_KI(i) + i->ki_gain_lookup(), ki_limit(i));
		}
		else if (!IS_NPC(i) && i->getLevel() < 1 && !i->desc)
		{
			act("$n fades away into obscurity; $s life leaving history with nothing of note.", i, 0, 0, TO_ROOM, 0);
			do_quit(i, "", 666);
		}
	} /* for */
}

void update_corpses_and_portals(void)
{
	// char buf[256];
	class Object *j, *next_thing;
	class Object *jj, *next_thing2;
	int proc = 0; // Processed items. Debugging.
	bool corpses_need_saving = false;
	void extract_obj(class Object * obj); /* handler.c */
	/* objects */
	for (j = DC::getInstance()->object_list; j; j = next_thing, proc++)
	{
		if (j == (class Object *)0x95959595)
			break;
		next_thing = j->next; /* Next in object list */
		/* Type 1 is a permanent game portal, and type 3 is a look_only
		 |  object.  Type 0 is the spell portal and type 2 is a game_portal
		 |  Type 4 is a no look permanent game portal
		 */

		if ((j->isTotem() && isSet(j->obj_flags.more_flags, ITEM_POOF_AFTER_24H)) || ((j->isPortal()) && (j->isPortalTypePlayer() || j->isPortalTypeTemp())))
		{
			if (j->obj_flags.timer > 0)
				(j->obj_flags.timer)--;
			if (!(j->obj_flags.timer))
			{
				if (j->in_room != DC::NOWHERE)
				{
					auto str = QStringLiteral("%1 shimmers brightly and then fades away.\r\n").arg(GET_OBJ_SHORT(j));
					str[0] = str[0].toUpper();
					send_to_room(str, j->in_room);
				}
				else if (j->in_obj && j->in_obj->in_room != DC::NOWHERE)
				{
					auto str = QStringLiteral("%1 shimmers brightly for a moment.\r\n").arg(GET_OBJ_SHORT(j->in_obj));
					str[0] = str[0].toUpper();
					send_to_room(str, j->in_obj->in_room);
				}
				else if (j->in_obj && j->in_obj->carried_by)
				{
					act("$p shimmers brightly for a moment.", j->in_obj->carried_by, j->in_obj, 0, TO_CHAR, INVIS_NULL);
				}
				else if (j->carried_by)
				{
					act("$p shimmers brightly and then fades away.", j->carried_by, j, 0, TO_CHAR, INVIS_NULL);
				}

				if (j->isTotem() && j->in_obj && j->in_obj->obj_flags.type_flag == ITEM_ALTAR)
					remove_totem(j->in_obj, j);

				extract_obj(j);
				continue;
			}
		}

		/* If this is a corpse */
		else if ((GET_ITEM_TYPE(j) == ITEM_CONTAINER) && (j->obj_flags.value[3]))
		{
			// TODO ^^^ - makes value[3] for containers a bitvector instead of a boolean

			/* timer count down */
			if (j->obj_flags.timer > 0)
			{
				j->obj_flags.timer--;
			}

			if (!j->obj_flags.timer)
			{
				if (j->carried_by)
					act("$p decays in your hands.", j->carried_by, j, 0, TO_CHAR, 0);
				else if ((j->in_room != DC::NOWHERE) && (DC::getInstance()->world[j->in_room].people))
				{
					act("A quivering horde of maggots consumes $p.", DC::getInstance()->world[j->in_room].people, j, 0, TO_ROOM, INVIS_NULL);
					act("A quivering horde of maggots consumes $p.", DC::getInstance()->world[j->in_room].people, j, 0, TO_CHAR, 0);
				}
				bool corpse_contained = j->contains != nullptr;
				for (jj = j->contains; jj; jj = next_thing2)
				{
					next_thing2 = jj->next_content; /* Next in inventory */

					if (GET_ITEM_TYPE(jj) == ITEM_CONTAINER)
					{
						Object *oo, *oon;
						for (oo = jj->contains; oo; oo = oon)
						{
							oon = oo->next_content;

							if (isSet(oo->obj_flags.more_flags, ITEM_NO_TRADE))
							{
								log_sacrifice((Character *)j, oo, true);
								extract_obj(oo);
							}
						}
					}
					if (j->in_obj)
					{
						if (isSet(jj->obj_flags.more_flags, ITEM_NO_TRADE))
						{
							jj->setOwner(j->getOwner());
						}

						move_obj(jj, j->in_obj);
					}
					else if (j->carried_by)
					{
						if (isSet(jj->obj_flags.more_flags, ITEM_NO_TRADE))
						{
							jj->setOwner(j->getOwner());
						}

						move_obj(jj, j->carried_by);
					}
					else if (j->in_room != DC::NOWHERE)
					{
						if (isSet(jj->obj_flags.more_flags, ITEM_NO_TRADE))
						{
							jj->setOwner(j->getOwner());
						}

						move_obj(jj, j->in_room);
					}
					else
					{
						logentry(QStringLiteral("BIIIG problem in limits.c!"), OVERSEER, DC::LogChannel::LOG_BUG);
						return;
					}
				}
				while (next_thing && next_thing->in_obj == j)
					next_thing = next_thing->next;
				// Is THIS what caused the crasher then?
				// Wtf: damnit.
				if (IS_OBJ_STAT(j, ITEM_PC_CORPSE) && corpse_contained)
					corpses_need_saving = true;
				extract_obj(j);
			}
		}
	}
	DC::getInstance()->removeDead();
	if (corpses_need_saving == true)
	{
		save_corpses();
	}
	// sprintf(buf, "DEBUG: Processed Objects: %d", proc);
	// logentry(buf, 108, DC::LogChannel::LOG_BUG);
	/* Now process the portals */
	// process_portals();
}

void prepare_character_for_sixty(Character *ch)
{
	if (IS_PC(ch) && DC::MAX_MORTAL_LEVEL == 60)
	{
		int skl = -1;
		switch (GET_CLASS(ch))
		{
		case CLASS_MAGE:
			skl = SKILL_SPELLCRAFT;
			break;
		case CLASS_BARBARIAN:
			skl = SKILL_BULLRUSH;
			break;
		case CLASS_PALADIN:
			skl = SPELL_HOLY_AURA;
			break;
		case CLASS_MONK:
			skl = KI_OFFSET + KI_MEDITATION;
			break;
		case CLASS_WARRIOR:
			skl = SKILL_COMBAT_MASTERY;
			break;
		case CLASS_THIEF:
			skl = SKILL_CRIPPLE;
			break;
		case CLASS_RANGER:
			skl = SKILL_NAT_SELECT;
			break;
		case CLASS_CLERIC:
			skl = SPELL_DIVINE_INTER;
			break;
		case CLASS_ANTI_PAL:
			skl = SPELL_VAMPIRIC_AURA;
			break;
		case CLASS_DRUID:
			skl = SPELL_CONJURE_ELEMENTAL;
			break;
		case CLASS_BARD:
			skl = SKILL_SONG_HYPNOTIC_HARMONY;
			break;
		}
		if (ch->has_skill(skl) && !isSet(ch->player->toggles, Player::PLR_50PLUS))
		{
			SET_BIT(ch->player->toggles, Player::PLR_50PLUS);
			int i = (ch->exp / 100000000) * 500000;
			if (i > 0)
			{
				csendf(ch, "$B$3You have been credited %d $B$5gold$R coins for your %ld experience.$R\n\r", i, ch->exp);
				ch->addGold(i);
			}
			else if (ch->exp > 0)
			{
				ch->sendln("Since you already have your Quest Skill, your experience has been set to 0 to allow advancement to level 60.");
			}
			ch->exp = 0;
		}
	}
}
