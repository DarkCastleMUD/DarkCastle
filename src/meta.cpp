/***************************************************************************
 *  file: meta.cpp , Special module.                     Part of DIKUMUD *
 *  Usage: Procedures handling special procedures for object/room/mobile   *
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

#include <assert.h>
#include <string.h>

#include "character.h"
#include "structs.h"
#include "utility.h"
#include "mobile.h"
#include "spells.h"
#include "room.h"
#include "handler.h"
#include "magic.h"
#include "levels.h"
#include "fight.h"
#include "obj.h"
#include "player.h"
#include "connect.h"
#include "interp.h"
#include "isr.h"
#include "race.h"
#include "db.h"
#include "sing.h"
#include "act.h"
#include "ki.h"
#include "returnvals.h"
#include "obj.h"
#include "const.h"

/*

 START META-PHYSICIAN

 */
int64_t new_meta_platinum_cost(int start, int end)
{ // This is the laziest function ever. I didn't feel like
	// figuring out a formulae to work with the ranges, so I didn't.
	int64_t platcost = 0;
	if (end <= start || end < 0 || start < 0)
		return 0; // That's cheap!
	while (start < end)
	{
		if (start < 1000)
			platcost += 100 + (start / 100);
		else if (start < 1250)
			platcost += 110 + (start / 30);
		else if (start < 1500)
			platcost += 150 + (start / 30);
		else if (start < 1750)
			platcost += 200 + (start / 35);
		else if (start < 2000)
			platcost += 250 + (start / 40);
		else if (start < 3000)
			platcost += 300 + (start / 15);
		else if (start < 4000)
			platcost += 500 + (start / 20);
		else if (start < 5000)
			platcost += 700 + (start / 25);
		else if (start < 6000)
			platcost += 900 + (start / 20);
		else
			platcost += (1200 + (start / 20)) > 1500 ? 1500 : (1200 + (start / 20));
		start += 5;
	}
	return platcost;
}

int r_new_meta_platinum_cost(int start, int64_t plats)
{ // This is a copy of the laziest function ever. I didn't feel like
	// figuring out a formulae to work with the ranges, so I didn't.
	int64_t platcost = 0;
	if (plats <= 0 || start < 0)
		return 0;
	while (platcost < plats)
	{
		if (start < 1000)
			platcost += 100 + (start / 100);
		else if (start < 1250)
			platcost += 110 + (start / 30);
		else if (start < 1500)
			platcost += 150 + (start / 30);
		else if (start < 1750)
			platcost += 200 + (start / 35);
		else if (start < 2000)
			platcost += 250 + (start / 40);
		else if (start < 3000)
			platcost += 300 + (start / 15);
		else if (start < 4000)
			platcost += 500 + (start / 20);
		else if (start < 5000)
			platcost += 700 + (start / 25);
		else if (start < 6000)
			platcost += 900 + (start / 20);
		else
			platcost += (1200 + (start / 20)) > 1500 ? 1500 : (1200 + (start / 20));
		start += 5;
	}
	return start - 5;
}

int r_new_meta_exp_cost(int start, int64_t exp)
{
	if (exp <= 0)
		return start;
	while (exp > 0)
	{
		exp -= new_meta_platinum_cost(start, start + 1) * 51523;
		start += 5;
	}
	return start - 5;
}

int64_t new_meta_exp_cost_one(int start)
{
	if (start < 0)
		return 0;
	return new_meta_platinum_cost(start, start + 1) * 51523;
}

int64_t moves_exp_spent(Character *ch)
{
	int start = GET_MAX_MOVE(ch) - GET_MOVE_METAS(ch);
	int64_t expcost = 0;
	while (start < GET_MAX_MOVE(ch))
	{
		expcost += (int)((5000000 + (start * 2500)) * 1.2);
		start++;
	}
	return expcost;
}

int64_t moves_plats_spent(Character *ch)
{
	int64_t expcost = 0;
	int start = GET_MAX_MOVE(ch) - GET_MOVE_METAS(ch);
	while (start < GET_MAX_MOVE(ch))
	{
		expcost += (int64_t)(((int)(125 + (int)((0.025 * start * (start / 1000 == 0 ? 1 : start / 1000)))) * 0.9));
		start++;
	}
	return expcost;
}

int64_t hps_exp_spent(Character *ch)
{
	int64_t expcost = 0;
	int cost;
	switch (GET_CLASS(ch))
	{
	case CLASS_BARBARIAN:
		cost = 2000;
		break;
	case CLASS_WARRIOR:
		cost = 2100;
		break;
	case CLASS_PALADIN:
		cost = 2200;
		break;
	case CLASS_MONK:
		cost = 2300;
		break;
	case CLASS_RANGER:
		cost = 2500;
		break;
	case CLASS_ANTI_PAL:
		cost = 2500;
		break;
	case CLASS_THIEF:
		cost = 2600;
		break;
	case CLASS_BARD:
		cost = 2600;
		break;
	case CLASS_DRUID:
		cost = 2800;
		break;
	case CLASS_CLERIC:
		cost = 2900;
		break;
	case CLASS_MAGIC_USER:
		cost = 3000;
		break;
	default:
		cost = 3000;
		break;
	}
	int base = GET_MAX_HIT(ch) - GET_HP_METAS(ch);
	while (base < GET_MAX_HIT(ch))
	{
		expcost += (int64_t)((5000000 + (cost * base)) * 1.2);
		base++;
	}
	return expcost;
}

int64_t hps_plats_spent(Character *ch)
{
	int cost;
	int64_t platcost = 0;
	switch (GET_CLASS(ch))
	{
	case CLASS_BARBARIAN:
		cost = 0;
		break;
	case CLASS_WARRIOR:
		cost = 10;
		break;
	case CLASS_PALADIN:
		cost = 20;
		break;
	case CLASS_MONK:
		cost = 30;
		break;
	case CLASS_RANGER:
		cost = 50;
		break;
	case CLASS_ANTI_PAL:
		cost = 50;
		break;
	case CLASS_THIEF:
		cost = 60;
		break;
	case CLASS_BARD:
		cost = 60;
		break;
	case CLASS_DRUID:
		cost = 80;
		break;
	case CLASS_CLERIC:
		cost = 90;
		break;
	case CLASS_MAGIC_USER:
		cost = 100;
		break;
	default:
		cost = 100;
		break;
	}
	int base = GET_MAX_HIT(ch) - GET_HP_METAS(ch);
	while (base < GET_MAX_HIT(ch))
	{
		platcost += (int64_t)((100 + cost + (int)(0.025 * base * (base / 1000 == 0 ? 1 : base / 1000))) * 0.9);
		base++;
	}
	return platcost;
}

int64_t mana_exp_spent(Character *ch)
{
	int cost;
	int64_t expcost = 0;
	switch (GET_CLASS(ch))
	{
	case CLASS_PALADIN:
		cost = 2800;
		break;
	case CLASS_RANGER:
		cost = 2500;
		break;
	case CLASS_ANTI_PAL:
		cost = 2500;
		break;
	case CLASS_DRUID:
		cost = 2200;
		break;
	case CLASS_CLERIC:
		cost = 2100;
		break;
	case CLASS_MAGIC_USER:
		cost = 2000;
		break;
	default:
		return 0;
	}
	int base = GET_MAX_MANA(ch) - GET_MANA_METAS(ch);
	while (base < GET_MAX_MANA(ch))
	{
		expcost += (int64_t)((5000000 + (cost * base)) * 1.2);
		base++;
	}
	return expcost;
}

int64_t mana_plats_spent(Character *ch)
{
	int cost;
	int64_t platcost = 0;
	switch (GET_CLASS(ch))
	{
	case CLASS_PALADIN:
		cost = 80;
		break;
	case CLASS_RANGER:
		cost = 50;
		break;
	case CLASS_ANTI_PAL:
		cost = 50;
		break;
	case CLASS_DRUID:
		cost = 20;
		break;
	case CLASS_CLERIC:
		cost = 10;
		break;
	case CLASS_MAGIC_USER:
		cost = 0;
		break;
	default:
		return 0;
	}
	int base = GET_MAX_MANA(ch) - GET_MANA_METAS(ch);
	while (base < GET_MAX_MANA(ch))
	{
		platcost += (int64_t)((100 + cost + (int)(0.025 * base * (base / 1000 == 0 ? 1 : base / 1000))) * 0.9);
		base++;
	}
	return platcost;
}

int meta_get_stat_exp_cost(Character *ch, attribute_t stat)
{
	int xp_price;
	int curr_stat = 0;
	switch (stat)
	{
	case attribute_t::CONSTITUTION:
		curr_stat = ch->raw_con;
		break;
	case attribute_t::STRENGTH:
		curr_stat = ch->raw_str;
		break;
	case attribute_t::DEXTERITY:
		curr_stat = ch->raw_dex;
		break;
	case attribute_t::INTELLIGENCE:
		curr_stat = ch->raw_intel;
		break;
	case attribute_t::WISDOM:
		curr_stat = ch->raw_wis;
		break;
	default:
		xp_price = 9999999;
		break;
	}
	switch (curr_stat)
	{
	case 1:
	case 2:
	case 3:
	case 4:
		xp_price = 2000000;
		break;
	case 5:
		xp_price = 3000000;
		break;
	case 6:
		xp_price = 4000000;
		break;
	case 7:
		xp_price = 5000000;
		break;
	case 8:
		xp_price = 5500000;
		break;
	case 9:
		xp_price = 6000000;
		break;
	case 10:
		xp_price = 6500000;
		break;
	case 11:
		xp_price = 7000000;
		break;
	case 12:
		xp_price = 7500000;
		break;
	case 13:
		xp_price = 8000000;
		break;
	case 14:
		xp_price = 8500000;
		break;
	case 15:
		xp_price = 9000000;
		break;
	case 16:
		xp_price = 9500000;
		break;
	case 29:
		xp_price = 25000000;
		break;
	default:
		xp_price = (curr_stat - 7) * 1000000;
		break;
	}

	//    if(ch->player->statmetas > 0)
	//     xp_price += ch->player->statmetas * 20000;

	return xp_price;
}

int meta_get_stat_plat_cost(Character *ch, attribute_t targetstat)
{
	int plat_cost;
	int stat;

	switch (targetstat)
	{
	case attribute_t::CONSTITUTION:
		stat = ch->raw_con;
		break;
	case attribute_t::STRENGTH:
		stat = ch->raw_str;
		break;
	case attribute_t::DEXTERITY:
		stat = ch->raw_dex;
		break;
	case attribute_t::WISDOM:
		stat = ch->raw_wis;
		break;
	case attribute_t::INTELLIGENCE:
		stat = ch->raw_intel;
		break;
	default:
		stat = 99;
		break;
	}

	if (stat < 5)
		plat_cost = 100;
	else if (stat < 13)
		plat_cost = 250;
	else if (stat < 28)
		plat_cost = 250 + ((stat - 12) * 50);
	else if (stat == 28)
		plat_cost = 1250;
	else
		plat_cost = 1500;

	return plat_cost;
}

void meta_list_stats(Character *ch)
{
	int xp_price, plat_cost, max_stat;

	xp_price = meta_get_stat_exp_cost(ch, attribute_t::STRENGTH);
	plat_cost = meta_get_stat_plat_cost(ch, attribute_t::STRENGTH);
	max_stat = get_max_stat(ch, attribute_t::STRENGTH);
	if (ch->raw_str >= max_stat)
		ch->send(QString("$B$31)$R Str:       Your strength is already %1.\r\n").arg(max_stat));
	else
		csendf(ch, "$B$31)$R Str: %d        Cost: %d exp + %d Platinum coins. \n\r",
			   (ch->raw_str + 1), xp_price, plat_cost);

	xp_price = meta_get_stat_exp_cost(ch, attribute_t::DEXTERITY);
	plat_cost = meta_get_stat_plat_cost(ch, attribute_t::DEXTERITY);
	max_stat = get_max_stat(ch, attribute_t::DEXTERITY);
	if (ch->raw_dex >= max_stat)
		ch->send(QString("$B$32)$R Dex:       Your dexterity is already %1.\r\n").arg(max_stat));
	else
		csendf(ch, "$B$32)$R Dex: %d        Cost: %d exp + %d Platinum coins.\r\n",
			   (ch->raw_dex + 1), xp_price, plat_cost);

	xp_price = meta_get_stat_exp_cost(ch, attribute_t::CONSTITUTION);
	plat_cost = meta_get_stat_plat_cost(ch, attribute_t::CONSTITUTION);
	max_stat = get_max_stat(ch, attribute_t::CONSTITUTION);
	if (ch->raw_con >= max_stat)
		ch->send(QString("$B$33)$R Con:       Your constitution is already %1.\r\n").arg(max_stat));
	else
		csendf(ch, "$B$33)$R Con: %d        Cost: %d exp + %d Platinum coins.\r\n",
			   (ch->raw_con + 1), xp_price, plat_cost);

	xp_price = meta_get_stat_exp_cost(ch, attribute_t::INTELLIGENCE);
	plat_cost = meta_get_stat_plat_cost(ch, attribute_t::INTELLIGENCE);
	max_stat = get_max_stat(ch, attribute_t::INTELLIGENCE);
	if (ch->raw_intel >= max_stat)
		ch->send(QString("$B$34)$R Int:       Your intelligence is already %1.\r\n").arg(max_stat));
	else
		csendf(ch, "$B$34)$R Int: %d        Cost: %d exp + %d Platinum coins.\r\n",
			   (ch->raw_intel + 1), xp_price, plat_cost);

	xp_price = meta_get_stat_exp_cost(ch, attribute_t::WISDOM);
	plat_cost = meta_get_stat_plat_cost(ch, attribute_t::WISDOM);
	max_stat = get_max_stat(ch, attribute_t::WISDOM);
	if (ch->raw_wis >= max_stat)
		ch->send(QString("$B$35)$R Wis:       Your wisdom is already %1.\r\n").arg(max_stat));
	else
		csendf(ch, "$B$35)$R Wis: %d        Cost: %d exp + %d Platinum coins.\r\n",
			   (ch->raw_wis + 1), xp_price, plat_cost);
}

int64_t meta_get_moves_exp_cost(Character *ch)
{
	int meta = GET_MOVE_METAS(ch);
	if (GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch) < 0)
		meta += GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch);
	return new_meta_exp_cost_one(MAX(0, meta));
}

int64_t meta_get_moves_plat_cost(Character *ch, int amount)
{
	int meta = GET_MOVE_METAS(ch);
	if (GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch) < 0)
		meta += GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch);
	return new_meta_platinum_cost(MAX(0, meta), MAX(0, meta) + amount);
}

int64_t meta_get_hps_exp_cost(Character *ch)
{
	int meta = GET_HP_METAS(ch);
	int bonus = 0;

	for (int i = 16; i < GET_RAW_CON(ch); i++)
		bonus += (i * i) / 30;

	meta -= bonus;

	if (GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch) > 0)
		meta -= GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch);

	return new_meta_exp_cost_one(MAX(0, meta));
}

int64_t meta_get_hps_plat_cost(Character *ch, int amount)
{
	int meta = GET_HP_METAS(ch);
	int bonus = 0;

	for (int i = 16; i < GET_RAW_CON(ch); i++)
		bonus += (i * i) / 30;

	meta -= bonus;

	if (GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch) > 0)
		meta -= GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch);

	return new_meta_platinum_cost(MAX(0, meta), MAX(0, meta) + amount);
}

int64_t meta_get_mana_exp_cost(Character *ch)
{
	int meta = GET_MANA_METAS(ch);
	int stat, bonus = 0;

	if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL || GET_CLASS(ch) == CLASS_RANGER)
		stat = GET_RAW_INT(ch);
	else if (GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_PALADIN || GET_CLASS(ch) == CLASS_DRUID)
		stat = GET_RAW_WIS(ch);
	else
		stat = 0;

	for (int i = 16; i < stat; i++)
		bonus += (i * i) / 30;

	meta -= bonus;

	if (GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch) > 0)
		meta -= GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch);

	return new_meta_exp_cost_one(MAX(0, meta));
}

int64_t meta_get_mana_plat_cost(Character *ch, int amount)
{
	int meta = GET_MANA_METAS(ch);
	int stat, bonus = 0;

	if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL || GET_CLASS(ch) == CLASS_RANGER)
		stat = GET_RAW_INT(ch);
	else if (GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_PALADIN || GET_CLASS(ch) == CLASS_DRUID)
		stat = GET_RAW_WIS(ch);
	else
		stat = 0;

	for (int i = 16; i < stat; i++)
		bonus += (i * i) / 30;

	meta -= bonus;

	if (GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch) > 0)
		meta -= GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch);

	return new_meta_platinum_cost(MAX(0, meta), MAX(0, meta) + amount);
}

int meta_get_ki_exp_cost(Character *ch)
{
	int cost, stat;
	switch (GET_CLASS(ch))
	{
	case CLASS_MONK:
		cost = 7700;
		stat = GET_RAW_WIS(ch) - 15;
		stat = MAX(0, stat);
		break;
	case CLASS_BARD:
		cost = 7400;
		stat = GET_RAW_INT(ch) - 15;
		stat = MAX(0, stat);
		break;
	default:
		return 0;
	}
	cost = 10000000 + ((GET_MAX_KI(ch) - stat) * cost);
	return (int)(cost * 1.2);
}

uint64_t meta_get_ki_plat_cost(Character *ch)
{
	uint64_t cost{};
	uint64_t stat{};
	const uint64_t adjusted_max_ki = MIN(250UL, MAX(0, GET_MAX_KI(ch)));

	switch (GET_CLASS(ch))
	{
	case CLASS_MONK:
		cost = 500UL;
		stat = MAX(0UL, GET_RAW_WIS(ch) - 15UL);
		break;
	case CLASS_BARD:
		cost = 400UL;
		stat = MAX(0UL, GET_RAW_INT(ch) - 15UL);
		break;
	default:
		return 0UL;
	}
	cost = 500UL + cost + (((adjusted_max_ki - stat) / 2UL) * ((adjusted_max_ki - stat) / 10UL));
	return static_cast<uint64_t>(cost * 0.9);
}

int meta_dude(Character *ch, class Object *obj, int cmd, const char *arg,
			  Character *owner)
{
	char argument[MAX_INPUT_LENGTH];

	int stat;
	int choice;
	int increase;
	int64_t hit_cost, mana_cost, move_cost, ki_cost = 0, hit_exp, move_exp, mana_exp, ki_exp = 0;
	int statplatprice = 0, max_stat = 0;

	int8_t *pstat = 0;
	int pprice = 0;

	if ((cmd != CMD_LIST) && (cmd != CMD_BUY) && (cmd != CMD_ESTIMATE))
		return eFAILURE;

	if (IS_AFFECTED(ch, AFF_BLIND))
		return eFAILURE;

	if (IS_NPC(ch))
		return eFAILURE;

	if (ch->getLevel() < 10)
	{
		send_to_char("$B$2The Meta-physician tells you, 'You're too low level for$R "
					 "$B$2me to waste my time on you.$R\n\r"
					 "$B$2Prove to me you are gonna stick around first!'$R.",
					 ch);
		return eSUCCESS;
	}

	hit_exp = meta_get_hps_exp_cost(ch);
	move_exp = meta_get_moves_exp_cost(ch);
	mana_exp = meta_get_mana_exp_cost(ch);

	hit_cost = meta_get_hps_plat_cost(ch, 1);
	move_cost = meta_get_moves_plat_cost(ch, 1);
	mana_cost = meta_get_mana_plat_cost(ch, 1);

	if (!IS_MOB(ch))
	{
		ki_exp = meta_get_ki_exp_cost(ch);
		ki_cost = meta_get_ki_plat_cost(ch);
	}

	if (cmd == CMD_ESTIMATE)
	{
		// Estimate costs
		char arg2[MAX_INPUT_LENGTH];
		arg = one_argument(arg, argument);
		one_argument(arg, arg2);

		if (!is_number(arg2) || !is_number(argument))
		{
			ch->sendln("$B$2The Meta-physician tells you, 'If you want to estimate a cost, specify which and how many points.'$R ");
			ch->sendln("$B$2The Meta-physician tells you, 'Example: estimate 1 1000'$R ");
			ch->sendln("$B$31)$R Estimate hit point cost.$R ");
			ch->sendln("$B$32)$R Estimate mana cost.$R ");
			ch->sendln("$B$33)$R Estimate move cost.$R ");
			return eSUCCESS;
		}

		int choice = atoi(argument);
		if (choice < 1 || choice > 3)
		{
			ch->sendln("$B$2The Meta-physician tells you, 'I cannot estimate that. Type estimate by itself for a list.'$R ");
			return eSUCCESS;
		}
		int amount = atoi(arg2);
		if (amount < 5 || amount > 10000)
		{
			ch->sendln("$B$2The Meta-physician tells you, 'The amount cannot be over 10000 or less than 5.'$R ");
			return eSUCCESS;
		}

		int64_t platcost;
		int64_t expcost;
		switch (choice)
		{
		case 1:
			platcost = meta_get_hps_plat_cost(ch, amount);
			break;
		case 2:
			platcost = meta_get_mana_plat_cost(ch, amount);
			break;
		case 3:
			platcost = meta_get_moves_plat_cost(ch, amount);
			break;
		}
		expcost = platcost * 51523;
		csendf(ch, "$B$2The Meta-physician tells you, 'That would cost you %ld platinum and %ld experience.'$R \n\r", platcost, expcost);
		return eSUCCESS;
	}
	else if (cmd == CMD_LIST)
	{ /* List */
		ch->sendln("$B$2The Meta-physician tells you, 'This is what I can do for you...'$R ");

		ch->sendln("$BAttribute Meta:$R");
		meta_list_stats(ch);

		ch->sendln("$BStatistic Meta:$R");
		if (hit_exp && hit_cost)
			csendf(ch, "$B$36)$R Add 5 points to your hit points:   %ld experience points and %ld"
					   " Platinum coins.\r\n",
				   hit_exp, hit_cost);
		else
			csendf(ch, "$B$36)$R Add to your hit points:   You cannot do this.\r\n");

		if (hit_exp && hit_cost)
			csendf(ch, "$B$37)$R Add 1 point to your hit points:   %ld experience points and %ld"
					   " Platinum coins.\r\n",
				   (int64_t)(hit_exp / 5 * 1.1), (int64_t)(hit_cost / 5 * 1.1));
		else
			csendf(ch, "$B$37)$R Add to your hit points:   You cannot do this.\r\n");

		if (mana_exp && mana_cost)
			csendf(ch, "$B$38)$R Add 5 points to your mana points:  %ld experience points and %ld"
					   " Platinum coins.\r\n",
				   mana_exp, mana_cost);
		else
			csendf(ch, "$B$38)$R Add to your mana points:  You cannot do this.\r\n");

		if (mana_exp && mana_cost)
			csendf(ch, "$B$39)$R Add 1 point to your mana points:   %ld experience points and %ld"
					   " Platinum coins.\r\n",
				   (int64_t)(mana_exp / 5 * 1.1), (int64_t)(mana_cost / 5 * 1.1));
		else
			csendf(ch, "$B$39)$R Add to your mana points:   You cannot do this.\r\n");

		if (move_exp && move_cost)
			csendf(ch, "$B$310)$R Add 5 points to your movement points: %ld experience points and %ld"
					   " Platinum coins.\r\n",
				   move_exp, move_cost);
		else
			csendf(ch, "$B$310)$R Add to your movement points:  You cannot do this.\r\n");

		if (move_exp && move_cost)
			csendf(ch, "$B$311)$R Add 1 points to your movement points:   %ld experience points and %ld"
					   " Platinum coins.\r\n",
				   (int64_t)(move_exp / 5 * 1.1), (int64_t)(move_cost / 5 * 1.1));
		else
			csendf(ch, "$B$311)$R Add to your movement points:   You cannot do this.\r\n");

		ch->sendln("$BUse 'estimate' command to get costs for higher intervals.");

		if (!IS_MOB(ch) && ki_cost && ki_exp)
		{ // mobs can't meta ki
			csendf(ch, "$B$312)$R Add a point of ki:        %ld experience points and %ld Platinum.\r\n", ki_exp, ki_cost);
		}
		else if (!IS_MOB(ch))
			csendf(ch, "$B$312)$R Add a point of ki:        You cannot do this.\r\n");

		ch->sendln("$BMonetary Exchange:$R");
		send_to_char(
			"$B$313)$R One (1) Platinum coin     Cost: 20,000 Gold Coins.\r\n"
			"$B$314)$R Five (5) Platinum coins   Cost: 100,000 Gold Coins.\r\n"
			"$B$315)$R 250 Platinum coins        Cost: 5,000,000 Gold Coins.\r\n"
			"$B$316)$R 100,000 Gold Coins        Cost: Five (5) Platinum coins.\r\n"
			"$B$317)$R 5,000,000 Gold Coins      Cost: 250 Platinum coins.\r\n"
			"$BOther Services:$R\r\n"
			"$B$318)$R Convert experience to gold. (100mil Exp. = 500000 Gold.)\r\n"
			"$B$319)$R A deep blue potion of healing. Cost: 25 Platinum coins.\r\n"
			"$B$320)$R Buy a practice session for 25 plats.\r\n",
			ch);
		if (!IS_MOB(ch))
		{
			csendf(ch, "$B$321)$R Add -2 points of AC for 10 qpoints. (-50 Max) (current -%d)\r\n", GET_AC_METAS(ch));
			ch->sendln("$B$322)$R Add 2,000,000 experience for 1 qpoint.");
		}

		return eSUCCESS;
	}
	else if (cmd == CMD_BUY)
	{ /* buy  */
		one_argument(arg, argument);
		if ((choice = atoi(argument)) == 0 || choice < 0)
		{
			ch->sendln("The Meta-physician tells you, 'Pick a number.'");
			return eSUCCESS;
		}
		switch (choice)
		{
		case 1:
			stat = ch->raw_str;
			pstat = &(ch->raw_str);
			pprice = meta_get_stat_exp_cost(ch, attribute_t::STRENGTH);
			statplatprice = meta_get_stat_plat_cost(ch, attribute_t::STRENGTH);
			max_stat = get_max_stat(ch, attribute_t::STRENGTH);
			break;
		case 2:
			stat = ch->raw_dex;
			pstat = &(ch->raw_dex);
			pprice = meta_get_stat_exp_cost(ch, attribute_t::DEXTERITY);
			statplatprice = meta_get_stat_plat_cost(ch, attribute_t::DEXTERITY);
			max_stat = get_max_stat(ch, attribute_t::DEXTERITY);
			break;
		case 3:
			stat = ch->raw_con;
			pstat = &(ch->raw_con);
			pprice = meta_get_stat_exp_cost(ch, attribute_t::CONSTITUTION);
			statplatprice = meta_get_stat_plat_cost(ch, attribute_t::CONSTITUTION);
			max_stat = get_max_stat(ch, attribute_t::CONSTITUTION);
			break;
		case 4:
			stat = ch->raw_intel;
			pstat = &(ch->raw_intel);
			pprice = meta_get_stat_exp_cost(ch, attribute_t::INTELLIGENCE);
			statplatprice = meta_get_stat_plat_cost(ch, attribute_t::INTELLIGENCE);
			max_stat = get_max_stat(ch, attribute_t::INTELLIGENCE);
			break;
		case 5:
			stat = ch->raw_wis;
			pstat = &(ch->raw_wis);
			pprice = meta_get_stat_exp_cost(ch, attribute_t::WISDOM);
			statplatprice = meta_get_stat_plat_cost(ch, attribute_t::WISDOM);
			max_stat = get_max_stat(ch, attribute_t::WISDOM);
			break;
		default:
			stat = 0;
			break;
		}

		if (choice < 6)
		{

			if (GET_PLATINUM(ch) < (unsigned)statplatprice)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services.  SCRAM!'$R");
				return eSUCCESS;
			}
			if (GET_EXP(ch) < pprice)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
				return eSUCCESS;
			}
			if (stat >= max_stat)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You're already as good at that as yer gonna get.'$R");
				return eSUCCESS;
			}

			GET_EXP(ch) -= pprice;
			GET_PLATINUM(ch) -= statplatprice;

			*pstat += 1;
			ch->player->statmetas++;

			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			ch->send(fmt::format(std::locale("en_US.UTF-8"), "The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", statplatprice, GET_PLATINUM(ch)));

			// affect the stat by 0 to reflect the new raw stat
			affect_modify(ch, APPLY_STR, 0, -1, true);
			affect_modify(ch, APPLY_DEX, 0, -1, true);
			affect_modify(ch, APPLY_INT, 0, -1, true);
			affect_modify(ch, APPLY_WIS, 0, -1, true);
			affect_modify(ch, APPLY_CON, 0, -1, true);

			redo_hitpoints(ch);
			redo_mana(ch);
			redo_ki(ch);
			return eSUCCESS;
		}

		if (choice == 6 && hit_exp && hit_cost)
		{
			if (GET_EXP(ch) < hit_exp)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
				return eSUCCESS;
			}
			if (GET_PLATINUM(ch) < (uint32_t)hit_cost)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
				return eSUCCESS;
			}
			GET_EXP(ch) -= hit_exp;
			GET_PLATINUM(ch) -= hit_cost;

			increase = 5;
			ch->raw_hit += increase;
			GET_HP_METAS(ch) += 5;
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			ch->send(fmt::format(std::locale("en_US.UTF-8"), "The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", hit_cost, GET_PLATINUM(ch)));
			redo_hitpoints(ch);
			return eSUCCESS;
		}

		if (choice == 7 && hit_exp && hit_cost)
		{
			hit_exp = (int)(hit_exp / 5 * 1.1);
			hit_cost = (int)(hit_cost / 5 * 1.1);

			if (GET_EXP(ch) < hit_exp)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
				return eSUCCESS;
			}
			if (GET_PLATINUM(ch) < (uint32_t)hit_cost)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!$R'");
				return eSUCCESS;
			}
			GET_EXP(ch) -= hit_exp;
			GET_PLATINUM(ch) -= hit_cost;

			increase = 1;
			ch->raw_hit += increase;
			GET_HP_METAS(ch) += 1;
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			ch->send(fmt::format(std::locale("en_US.UTF-8"), "The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", hit_cost, GET_PLATINUM(ch)));
			redo_hitpoints(ch);
			return eSUCCESS;
		}

		if (choice == 8 && mana_exp && mana_cost)
		{

			if (GET_EXP(ch) < mana_exp)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
				return eSUCCESS;
			}
			if (GET_PLATINUM(ch) < (uint32_t)mana_cost)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
				return eSUCCESS;
			}

			GET_EXP(ch) -= mana_exp;
			GET_PLATINUM(ch) -= mana_cost;

			increase = 5;
			ch->raw_mana += increase;
			GET_MANA_METAS(ch) += 5;
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			ch->send(fmt::format(std::locale("en_US.UTF-8"), "The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", mana_cost, GET_PLATINUM(ch)));
			redo_mana(ch);
			return eSUCCESS;
		}

		if (choice == 9 && mana_exp && mana_cost)
		{
			mana_exp = (int)(mana_exp / 5 * 1.1);
			mana_cost = (int)(mana_cost / 5 * 1.1);

			if (GET_EXP(ch) < mana_exp)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
				return eSUCCESS;
			}
			if (GET_PLATINUM(ch) < (uint32_t)mana_cost)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
				return eSUCCESS;
			}

			GET_EXP(ch) -= mana_exp;
			GET_PLATINUM(ch) -= mana_cost;

			increase = 1;
			ch->raw_mana += increase;
			GET_MANA_METAS(ch) += 1;
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			ch->send(fmt::format(std::locale("en_US.UTF-8"), "The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", mana_cost, GET_PLATINUM(ch)));
			redo_mana(ch);
			return eSUCCESS;
		}

		if (choice == 10 && move_exp && move_cost)
		{
			if (GET_EXP(ch) < move_exp)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
				return eSUCCESS;
			}
			if (GET_PLATINUM(ch) < (uint32_t)move_cost)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
				return eSUCCESS;
			}

			GET_EXP(ch) -= move_exp;
			GET_PLATINUM(ch) -= move_cost;
			ch->raw_move += 5;
			ch->max_move += 5;
			GET_MOVE_METAS(ch) += 5;
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			ch->send(fmt::format(std::locale("en_US.UTF-8"), "The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", move_cost, GET_PLATINUM(ch)));
			redo_hitpoints(ch);
			redo_mana(ch);
			return eSUCCESS;
		}

		if (choice == 11 && move_exp && move_cost)
		{
			move_exp = (int)(move_exp / 5 * 1.1);
			move_cost = (int)(move_cost / 5 * 1.1);

			if (GET_EXP(ch) < move_exp)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
				return eSUCCESS;
			}
			if (GET_PLATINUM(ch) < (uint32_t)move_cost)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
				return eSUCCESS;
			}

			GET_EXP(ch) -= move_exp;
			GET_PLATINUM(ch) -= move_cost;
			ch->raw_move += 1;
			ch->max_move += 1;
			GET_MOVE_METAS(ch) += 1;
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			ch->send(fmt::format(std::locale("en_US.UTF-8"), "The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", move_cost, GET_PLATINUM(ch)));
			redo_hitpoints(ch);
			redo_mana(ch);
			return eSUCCESS;
		}
		if (choice == 12 && ki_exp && ki_cost)
		{
			if (IS_MOB(ch))
			{
				ch->sendln("Mobs cannot meta ki.");
				return eSUCCESS;
			}
			if (GET_EXP(ch) < ki_exp)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
				return eSUCCESS;
			}
			if (GET_PLATINUM(ch) < (uint32_t)(ki_cost))
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
				return eSUCCESS;
			}

			GET_EXP(ch) -= ki_exp;
			GET_PLATINUM(ch) -= ki_cost;

			ch->raw_ki += 1;
			GET_KI_METAS(ch) += 1;
			redo_ki(ch);
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			ch->send(fmt::format(std::locale("en_US.UTF-8"), "The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", ki_cost, GET_PLATINUM(ch)));
			return eSUCCESS;
		}

		if (choice == 13)
		{
			if (affected_by_spell(ch, FUCK_GTHIEF))
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You cannot do this because of your criminal actions!'$R");
				return eSUCCESS;
			}
			if (ch->getGold() < 20000)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R");
				return eSUCCESS;
			}
			ch->removeGold(20000);
			GET_PLATINUM(ch) += 1;
			ch->sendln("Ok.");
			return eSUCCESS;
		}
		if (choice == 14)
		{
			if (affected_by_spell(ch, FUCK_GTHIEF))
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You cannot do this because of your criminal actions!'$R");
				return eSUCCESS;
			}
			if (ch->getGold() < 100000)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R");
				return eSUCCESS;
			}

			ch->removeGold(100000);
			GET_PLATINUM(ch) += 5;
			ch->sendln("Ok.");
			return eSUCCESS;
		}
		if (choice == 15)
		{
			if (!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF))
			{
				ch->sendln("Your criminal acts prohibit it.");
				return eSUCCESS;
			}

			if (ch->getGold() < 5000000)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R");
				return eSUCCESS;
			}
			GET_PLATINUM(ch) += 250;
			ch->removeGold(5000000);
			ch->sendln("Ok.");
			return eSUCCESS;
		}
		if (choice == 16)
		{
			if (GET_PLATINUM(ch) < 5)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R");
				return eSUCCESS;
			}
			GET_PLATINUM(ch) -= 5;
			ch->addGold(100000);
			ch->sendln("Ok.");
			return eSUCCESS;
		}

		if (choice == 17)
		{
			if (GET_PLATINUM(ch) < 250)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that!  SCRAM$R");
				return eSUCCESS;
			}
			GET_PLATINUM(ch) -= 250;
			ch->addGold(5000000);
			ch->sendln("Ok.");
			return eSUCCESS;
		}
		if (choice == 18)
		{
			if (GET_EXP(ch) < 100000000)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
				return eSUCCESS;
			}
			if (IS_MOB(ch))
			{
				ch->sendln("What would you have to spend $B$5gold$R on chode?");
				return eSUCCESS;
			}

			GET_EXP(ch) -= 100000000;
			ch->addGold(500000);

			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			return eSUCCESS;
		}
		if (choice == 19)
		{
			if (GET_PLATINUM(ch) < 25)
			{
				ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that!'$R");
				return eSUCCESS;
			}
			class Object *obj = clone_object(real_object(10003));
			if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
			{
				ch->sendln("You can't carry that many items.");
				extract_obj(obj);
				return eSUCCESS;
			}

			if (IS_CARRYING_W(ch) + obj->obj_flags.weight > CAN_CARRY_W(ch))
			{
				ch->sendln("You can't carry that much weight.");
				extract_obj(obj);
				return eSUCCESS;
			}
			GET_PLATINUM(ch) -= 25;
			obj_to_char(obj, ch);
			ch->sendln("$B$2The Meta-physician tells you, 'Here is your potion.'$R");
			return eSUCCESS;
		}
		if (choice == 20)
		{
			if (GET_PLATINUM(ch) < 25)
			{
				ch->sendln("Costs 25 plats...which you don't have.");
				return eSUCCESS;
			}
			if (IS_MOB(ch))
			{
				ch->sendln("You can't buy practices chode...");
				return eSUCCESS;
			}
			ch->sendln("The Meta-Physician gives you a practice session.");

			GET_PLATINUM(ch) -= 25;
			ch->player->practices += 1;
			return eSUCCESS;
		}
		if (choice == 21)
		{ // -2 AC
			if (GET_QPOINTS(ch) < 10)
			{
				ch->sendln("Costs 10 qpoints...which you don't have.");
				return eSUCCESS;
			}
			if (IS_MOB(ch))
			{
				ch->sendln("You can't buy AC, chode...");
				return eSUCCESS;
			}
			if (GET_AC_METAS(ch) >= 50)
			{
				ch->sendln("You've reached the -50 AC limit that can be purchased per character.");
				return eSUCCESS;
			}

			GET_QPOINTS(ch) -= 10;
			GET_AC_METAS(ch) += 2;
			GET_AC(ch) -= 2;
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			logf(110, LogChannels::LOG_MORTAL, "%s metas -2 AC for 10 qpoints.", GET_NAME(ch));
			ch->save(10);

			return eSUCCESS;
		}
		if (choice == 22)
		{ // 2,000,000 experience
			if (GET_QPOINTS(ch) < 1)
			{
				ch->sendln("Costs 1 qpoint...which you don't have.");
				return eSUCCESS;
			}
			if (IS_MOB(ch))
			{
				ch->sendln("You can't buy experience, chode...");
				return eSUCCESS;
			}

			GET_QPOINTS(ch) -= 1;
			GET_EXP(ch) += 2000000;
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			logf(110, LogChannels::LOG_MORTAL, "%s metas 2000000 XP for 1 qpoint.", GET_NAME(ch));
			ch->save(10);

			return eSUCCESS;
		}
	}
	ch->sendln("$B$2The Meta-physician tells you, 'Buy what?!'$R");
	return eSUCCESS;
}

/*

 END META-PHYSICIAN

 */

/*

 START CARDINAL THELONIUS

 */

void undo_race_saves(Character *ch)
{
	switch (GET_RACE(ch))
	{
	case RACE_HUMAN:
		ch->saves[SAVE_TYPE_FIRE] -= RACE_HUMAN_FIRE_MOD;
		ch->saves[SAVE_TYPE_COLD] -= RACE_HUMAN_COLD_MOD;
		ch->saves[SAVE_TYPE_ENERGY] -= RACE_HUMAN_ENERGY_MOD;
		ch->saves[SAVE_TYPE_ACID] -= RACE_HUMAN_ACID_MOD;
		ch->saves[SAVE_TYPE_MAGIC] -= RACE_HUMAN_MAGIC_MOD;
		ch->saves[SAVE_TYPE_POISON] -= RACE_HUMAN_POISON_MOD;
		break;
	case RACE_ELVEN:
		ch->saves[SAVE_TYPE_FIRE] -= RACE_ELVEN_FIRE_MOD;
		ch->saves[SAVE_TYPE_COLD] -= RACE_ELVEN_COLD_MOD;
		ch->saves[SAVE_TYPE_ENERGY] -= RACE_ELVEN_ENERGY_MOD;
		ch->saves[SAVE_TYPE_ACID] -= RACE_ELVEN_ACID_MOD;
		ch->saves[SAVE_TYPE_MAGIC] -= RACE_ELVEN_MAGIC_MOD;
		ch->saves[SAVE_TYPE_POISON] -= RACE_ELVEN_POISON_MOD;
		ch->spell_mitigation -= 1;
		break;
	case RACE_DWARVEN:
		ch->saves[SAVE_TYPE_FIRE] -= RACE_DWARVEN_FIRE_MOD;
		ch->saves[SAVE_TYPE_COLD] -= RACE_DWARVEN_COLD_MOD;
		ch->saves[SAVE_TYPE_ENERGY] -= RACE_DWARVEN_ENERGY_MOD;
		ch->saves[SAVE_TYPE_ACID] -= RACE_DWARVEN_ACID_MOD;
		ch->saves[SAVE_TYPE_MAGIC] -= RACE_DWARVEN_MAGIC_MOD;
		ch->saves[SAVE_TYPE_POISON] -= RACE_DWARVEN_POISON_MOD;
		ch->melee_mitigation -= 1;
		break;
	case RACE_TROLL:
		ch->saves[SAVE_TYPE_FIRE] -= RACE_TROLL_FIRE_MOD;
		ch->saves[SAVE_TYPE_COLD] -= RACE_TROLL_COLD_MOD;
		ch->saves[SAVE_TYPE_ENERGY] -= RACE_TROLL_ENERGY_MOD;
		ch->saves[SAVE_TYPE_ACID] -= RACE_TROLL_ACID_MOD;
		ch->saves[SAVE_TYPE_MAGIC] -= RACE_TROLL_MAGIC_MOD;
		ch->saves[SAVE_TYPE_POISON] -= RACE_TROLL_POISON_MOD;
		ch->spell_mitigation -= 2;
		break;
	case RACE_GIANT:
		ch->saves[SAVE_TYPE_FIRE] -= RACE_GIANT_FIRE_MOD;
		ch->saves[SAVE_TYPE_COLD] -= RACE_GIANT_COLD_MOD;
		ch->saves[SAVE_TYPE_ENERGY] -= RACE_GIANT_ENERGY_MOD;
		ch->saves[SAVE_TYPE_ACID] -= RACE_GIANT_ACID_MOD;
		ch->saves[SAVE_TYPE_MAGIC] -= RACE_GIANT_MAGIC_MOD;
		ch->saves[SAVE_TYPE_POISON] -= RACE_GIANT_POISON_MOD;
		ch->melee_mitigation -= 2;
		break;
	case RACE_PIXIE:
		ch->saves[SAVE_TYPE_FIRE] -= RACE_PIXIE_FIRE_MOD;
		ch->saves[SAVE_TYPE_COLD] -= RACE_PIXIE_COLD_MOD;
		ch->saves[SAVE_TYPE_ENERGY] -= RACE_PIXIE_ENERGY_MOD;
		ch->saves[SAVE_TYPE_ACID] -= RACE_PIXIE_ACID_MOD;
		ch->saves[SAVE_TYPE_MAGIC] -= RACE_PIXIE_MAGIC_MOD;
		ch->saves[SAVE_TYPE_POISON] -= RACE_PIXIE_POISON_MOD;
		ch->spell_mitigation -= 2;
		break;
	case RACE_HOBBIT:
		ch->saves[SAVE_TYPE_FIRE] -= RACE_HOBBIT_FIRE_MOD;
		ch->saves[SAVE_TYPE_COLD] -= RACE_HOBBIT_COLD_MOD;
		ch->saves[SAVE_TYPE_ENERGY] -= RACE_HOBBIT_ENERGY_MOD;
		ch->saves[SAVE_TYPE_ACID] -= RACE_HOBBIT_ACID_MOD;
		ch->saves[SAVE_TYPE_MAGIC] -= RACE_HOBBIT_MAGIC_MOD;
		ch->saves[SAVE_TYPE_POISON] -= RACE_HOBBIT_POISON_MOD;
		ch->melee_mitigation -= 2;
		break;
	case RACE_GNOME:
		ch->saves[SAVE_TYPE_FIRE] -= RACE_GNOME_FIRE_MOD;
		ch->saves[SAVE_TYPE_COLD] -= RACE_GNOME_COLD_MOD;
		ch->saves[SAVE_TYPE_ENERGY] -= RACE_GNOME_ENERGY_MOD;
		ch->saves[SAVE_TYPE_ACID] -= RACE_GNOME_ACID_MOD;
		ch->saves[SAVE_TYPE_MAGIC] -= RACE_GNOME_MAGIC_MOD;
		ch->saves[SAVE_TYPE_POISON] -= RACE_GNOME_POISON_MOD;
		ch->spell_mitigation -= 1;
		break;
	case RACE_ORC:
		ch->saves[SAVE_TYPE_FIRE] -= RACE_ORC_FIRE_MOD;
		ch->saves[SAVE_TYPE_COLD] -= RACE_ORC_COLD_MOD;
		ch->saves[SAVE_TYPE_ENERGY] -= RACE_ORC_ENERGY_MOD;
		ch->saves[SAVE_TYPE_ACID] -= RACE_ORC_ACID_MOD;
		ch->saves[SAVE_TYPE_MAGIC] -= RACE_ORC_MAGIC_MOD;
		ch->saves[SAVE_TYPE_POISON] -= RACE_ORC_POISON_MOD;
		ch->melee_mitigation -= 1;
		break;
	default:
		break;
	}
}

bool is_race_applicable(Character *ch, int race)
{
	if (GET_CLASS(ch) == CLASS_PALADIN && (race != RACE_HUMAN && race != RACE_ELVEN && race != RACE_DWARVEN))
		return false;
	if (GET_CLASS(ch) == CLASS_ANTI_PAL && (race != RACE_HUMAN && race != RACE_ORC && race != RACE_DWARVEN))
		return false;
	if (GET_CLASS(ch) == CLASS_BARBARIAN && race == RACE_PIXIE)
		return false;
	if (GET_CLASS(ch) == CLASS_THIEF && race == RACE_GIANT)
		return false;
	switch (race)
	{
	case RACE_ELVEN:
		if (GET_RAW_DEX(ch) - 2 < 10 || GET_RAW_INT(ch) - 2 < 10)
			return false;
		break;
	case RACE_DWARVEN:
		if (GET_RAW_CON(ch) - 2 < 10 || GET_RAW_WIS(ch) - 2 < 10)
			return false;
		break;
	case RACE_HOBBIT:
		if (GET_RAW_DEX(ch) - 2 < 10)
			return false;
		break;
	case RACE_PIXIE:
		if (GET_RAW_INT(ch) - 2 < 10)
			return false;
		break;
	case RACE_GIANT:
		if (GET_RAW_STR(ch) - 2 < 12)
			return false;
		break;
	case RACE_GNOME:
		if (GET_RAW_WIS(ch) - 2 < 12)
			return false;
		break;
	case RACE_ORC:
		if (GET_RAW_CON(ch) - 2 < 10 || GET_RAW_STR(ch) - 2 < 10)
			return false;
		break;
	case RACE_TROLL:
		if (GET_RAW_CON(ch) - 2 < 12)
			return false;
		break;
	default:
		break;
	}
	return true;
}

bool would_die(Character *ch)
{
	if (GET_RAW_STR(ch) < 8 || GET_RAW_CON(ch) < 8 || GET_RAW_WIS(ch) < 8 || GET_RAW_INT(ch) < 8 || GET_RAW_DEX(ch) < 8)
		return true;

	return false;
}

void set_heightweight(Character *ch)
{
	switch (GET_RACE(ch))
	{
	case RACE_HUMAN:
		ch->height = number(66, 77);
		ch->weight = number(160, 200);
		break;
	case RACE_ELVEN:
		ch->height = number(78, 101);
		ch->weight = number(120, 160);
		break;
	case RACE_DWARVEN:
		ch->height = number(42, 65);
		ch->weight = number(140, 180);
		break;
	case RACE_HOBBIT:
		ch->height = number(20, 41);
		ch->weight = number(40, 80);
		break;
	case RACE_PIXIE:
		ch->height = number(12, 33);
		ch->weight = number(10, 40);
		break;
	case RACE_GIANT:
		ch->height = number(106, 131);
		ch->weight = number(260, 300);
		break;
	case RACE_GNOME:
		ch->height = number(42, 65);
		ch->weight = number(80, 120);
		break;
	case RACE_ORC:
		ch->height = number(78, 101);
		ch->weight = number(200, 240);
		break;
	case RACE_TROLL:
		ch->height = number(102, 123);
		ch->weight = number(240, 280);
		break;
	}
	logf(ANGEL, LogChannels::LOG_MORTAL, "set_heightweight: %s's height set to %d", GET_NAME(ch), GET_HEIGHT(ch));
	logf(ANGEL, LogChannels::LOG_MORTAL, "set_heightweight: %s's weight set to %d", GET_NAME(ch), GET_WEIGHT(ch));
}

int changecost(int oldrace, int newrace)
{
	switch (oldrace)
	{
	case RACE_GIANT:
	case RACE_TROLL:
		if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT)
			return 7000;
		else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME)
			return 6500;
		else if (newrace == RACE_HUMAN)
			return 6000;
		else if (newrace == RACE_ELVEN || newrace == RACE_ORC)
			return 5500;
		else if (newrace == RACE_GIANT || newrace == RACE_TROLL)
			return 5000;
		break;
	case RACE_HOBBIT:
	case RACE_PIXIE:
		if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT)
			return 5000;
		else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME)
			return 5500;
		else if (newrace == RACE_HUMAN)
			return 6000;
		else if (newrace == RACE_ELVEN || newrace == RACE_ORC)
			return 6500;
		else if (newrace == RACE_GIANT || newrace == RACE_TROLL)
			return 7000;
		break;
	case RACE_DWARVEN:
	case RACE_GNOME:
		if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT)
			return 5500;
		else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME)
			return 5000;
		else if (newrace == RACE_HUMAN)
			return 5500;
		else if (newrace == RACE_ELVEN || newrace == RACE_ORC)
			return 6000;
		else if (newrace == RACE_GIANT || newrace == RACE_TROLL)
			return 6500;
		break;
	case RACE_ELVEN:
	case RACE_ORC:
		if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT)
			return 6500;
		else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME)
			return 6000;
		else if (newrace == RACE_HUMAN)
			return 5500;
		else if (newrace == RACE_ELVEN || newrace == RACE_ORC)
			return 5000;
		else if (newrace == RACE_GIANT || newrace == RACE_TROLL)
			return 5500;
		break;
	case RACE_HUMAN:
		if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT)
			return 6000;
		else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME)
			return 5500;
		else if (newrace == RACE_HUMAN)
			return 5000;
		else if (newrace == RACE_ELVEN || newrace == RACE_ORC)
			return 5500;
		else if (newrace == RACE_GIANT || newrace == RACE_TROLL)
			return 6000;
		break;
	default:
		return 1000000;
	}
	return 100000;
}

char *race_message(Character *ch, int race)
{
	static char buf[MAX_STRING_LENGTH];
	if (GET_RACE(ch) == race)
		return "You are already of this race.";
	else if (!is_race_applicable(ch, race))
		return "You do not qualify for becoming this race.";

	sprintf(buf, "%d platinum coins.", changecost(GET_RACE(ch), race));
	return &buf[0];
}

int cardinal(Character *ch, class Object *obj, int cmd, const char *argument, Character *owner)
{
	if (cmd == 59) // list
	{

		send_to_char("$B$2Cardinal Thelonius tells you, 'Here's what I can do for you...'$R\r\nEnter \"buy <number>\" to make a selection.\r\n\r\n",
					 ch);
		ch->sendln("$BRace Change:$R\r\n(Remember a race change will reduce your base attributes by 2 points each.)");

		for (int i = 1; i <= MAX_PC_RACE; i++)
			csendf(ch, "$B$3%d)$R  %-32s - %s\r\n", i, races[i].singular_name, race_message(ch, i));

		ch->sendln("$BOther Services:$R");

		csendf(ch, "$B$3%d)$R %-32s - 1000 platinum coins.\r\n", MAX_PC_RACE + 1, "Sex Change");
		csendf(ch, "$B$3%d)$R %-32s - 50 platinum coins.\r\n", MAX_PC_RACE + 2, "A deep red vial of mana");

		ch->sendln("$BHeight/Weight Change:$R");
		heightweight(ch, false);
		if (ch->height < races[ch->race].max_height)
			csendf(ch, "$B$3%d)$R %-32s - 250 platinum coins.\r\n", MAX_PC_RACE + 3, "Increase your height by 1");
		else
			csendf(ch, "$B$3%d)$R %-32s.\r\n", MAX_PC_RACE + 3, "You cannot increase your height further");

		if (ch->height > races[ch->race].min_height)
			csendf(ch, "$B$3%d)$R %-32s - 250 platinum coins.\r\n", MAX_PC_RACE + 4, "Decrease your height by 1");
		else
			csendf(ch, "$B$3%d)$R %-32s.\r\n", MAX_PC_RACE + 4, "You cannot decrease your height further");

		if (ch->weight < races[ch->race].max_weight)
			csendf(ch, "$B$3%d)$R %-32s - 250 platinum coins.\r\n", MAX_PC_RACE + 5, "Increase your weight by 1");
		else
			csendf(ch, "$B$3%d)$R %-32s.\r\n", MAX_PC_RACE + 5, "You cannot increase your weight further");

		if (ch->weight > races[ch->race].min_weight)
			csendf(ch, "$B$3%d)$R %-32s - 250 platinum coins.\r\n", MAX_PC_RACE + 6, "Decrease your weight by 1");
		else
			csendf(ch, "$B$3%d)$R %-32s.\r\n", MAX_PC_RACE + 6, "You cannot decrease your weight further");
		heightweight(ch, true);
		csendf(ch, "$B$3%d)$R %-32s - 5 quest points.\r\n", MAX_PC_RACE + 7, "Increase your age by 1 (500 max)");
		csendf(ch, "$B$3%d)$R %-32s - 5 quest points.\r\n", MAX_PC_RACE + 8, "Decrease your age by 1  (18 min)");

		return eSUCCESS;
	}
	else if (cmd == 56) // buy
	{
		char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
		argument = one_argument(argument, arg);
		argument = one_argument(argument, arg2);
		int choice = atoi(arg);
		if (choice > 0 && choice <= MAX_PC_RACE)
		{
			if (would_die(ch))
			{
				ch->sendln("$B$2Cardinal Thelonius tells you, 'The process would kill you!'$R");
				return eSUCCESS;
			}
			if (GET_RACE(ch) == choice)
			{
				ch->sendln("$B$2Cardinal Thelonius tells you, 'You are already a member of that race!'$R");
				return eSUCCESS;
			}
			if (!is_race_applicable(ch, choice))
			{
				ch->sendln("$B$2Cardinal Thelonius tells you, 'You do not qualify for becoming that race!'$R");
				return eSUCCESS;
			}
			if (GET_PLATINUM(ch) < (unsigned)changecost(GET_RACE(ch), choice))
			{
				ch->sendln("$B$2Cardinal Thelonius tells you, 'You can't afford that!'$R");
				return eSUCCESS;
			}

			if (!str_cmp(arg2, "confirm"))
			{
				GET_PLATINUM(ch) -= changecost(GET_RACE(ch), choice);
				undo_race_saves(ch);

				GET_RACE(ch) = choice;

				GET_RAW_STR(ch) = MIN(get_max_stat(ch, attribute_t::STRENGTH), GET_RAW_STR(ch));
				GET_RAW_STR(ch) -= 2;
				GET_STR(ch) = GET_RAW_STR(ch) + GET_STR_BONUS(ch);

				GET_RAW_CON(ch) = MIN(get_max_stat(ch, attribute_t::CONSTITUTION), GET_RAW_CON(ch));
				GET_RAW_CON(ch) -= 2;
				GET_CON(ch) = GET_RAW_CON(ch) + GET_CON_BONUS(ch);

				GET_RAW_DEX(ch) = MIN(get_max_stat(ch, attribute_t::DEXTERITY), GET_RAW_DEX(ch));
				GET_RAW_DEX(ch) -= 2;
				GET_DEX(ch) = GET_RAW_DEX(ch) + GET_DEX_BONUS(ch);

				GET_RAW_WIS(ch) = MIN(get_max_stat(ch, attribute_t::WISDOM), GET_RAW_WIS(ch));
				GET_RAW_WIS(ch) -= 2;
				GET_WIS(ch) = GET_RAW_WIS(ch) + GET_WIS_BONUS(ch);

				GET_RAW_INT(ch) = MIN(get_max_stat(ch, attribute_t::INTELLIGENCE), GET_RAW_INT(ch));
				GET_RAW_INT(ch) -= 2;
				GET_INT(ch) = GET_RAW_INT(ch) + GET_INT_BONUS(ch);

				redo_hitpoints(ch);
				redo_mana(ch);
				redo_ki(ch);

				extern void do_inate_race_abilities(Character * ch);
				do_inate_race_abilities(ch);
				extern void verify_max_stats(Character * ch);
				verify_max_stats(ch);
				// set_heightweight(ch);
				extern void check_hw(Character * ch);
				check_hw(ch);

				extern int recheck_height_wears(Character * ch);
				recheck_height_wears(ch);

				ch->sendln("The Cardinal prays loudly and summons the magic of the gods...");
				ch->sendln("After a brief moment of pain you are reborn!");
			}
			else
			{
				ch->send(QString("$BYou must enter 'buy %1 CONFIRM' if you are positive you wish to make this change!\r\n").arg(choice));
				ch->sendln("$4NOTE$R$B: Your attributes will be adjusted to fit this new race and then lowered by 2 points each.$R");
			}
			return eSUCCESS;
		}
		else if (choice == MAX_PC_RACE + 1)
		{
			Character::sex_t newsex{};
			if (arg2[0] == 'n')
				newsex = Character::sex_t::NEUTRAL;
			else if (arg2[0] == 'm')
				newsex = Character::sex_t::MALE;
			else if (arg2[0] == 'f')
				newsex = Character::sex_t::FEMALE;
			else
			{
				csendf(ch, "Syntax: buy %d m/f/n\r\n", MAX_PC_RACE + 1);
				return eSUCCESS;
			}
			if (GET_SEX(ch) == newsex)
			{
				ch->sendln("$B$2Cardinal Thelonius tells you, 'That wouldn't change much'$R");
				return eSUCCESS;
			}
			if (GET_PLATINUM(ch) < 1000)
			{
				ch->sendln("$B$2Cardinal Thelonius tells you, 'You can't afford that!'$R");
				return eSUCCESS;
			}
			GET_PLATINUM(ch) -= 1000;
			GET_SEX(ch) = newsex;
			ch->sendln("The Cardinal prays loudly and summons the magic of the gods...");
			ch->sendln("After a brief moment of pain you are reborn!");
			return eSUCCESS;
		}
		else if (choice == MAX_PC_RACE + 2)
		{
			if (GET_PLATINUM(ch) < 50)
			{
				ch->sendln("$B$2Cardinal Thelonius tells you, 'You can't afford that!'$R");
				return eSUCCESS;
			}
			class Object *obj = clone_object(real_object(10004));
			if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
			{
				ch->sendln("You can't carry that many items.");
				extract_obj(obj);
				return eSUCCESS;
			}

			if (IS_CARRYING_W(ch) + obj->obj_flags.weight > CAN_CARRY_W(ch))
			{
				ch->sendln("You can't carry that much weight.");
				extract_obj(obj);
				return eSUCCESS;
			}
			GET_PLATINUM(ch) -= 50;
			obj_to_char(obj, ch);
			ch->sendln("$B$2Cardinal Thelonius tells you, 'Here is your potion.'$R");
			return eSUCCESS;
		}
		else if (choice >= MAX_PC_RACE + 3 && choice <= MAX_PC_RACE + 6)
		{
			choice -= MAX_PC_RACE;

			heightweight(ch, false);
			if (choice == 3 && ch->height >= races[ch->race].max_height)
			{
				ch->sendln("You cannot increase your height any more.");
				heightweight(ch, true);
				return eSUCCESS;
			}
			else if (choice == 4 && ch->height <= races[ch->race].min_height)
			{
				ch->sendln("You cannot decrease your height any more.");
				heightweight(ch, true);
				return eSUCCESS;
			}
			else if (choice == 5 && ch->weight >= races[ch->race].max_weight)
			{
				ch->sendln("You cannot increase your weight any more.");
				heightweight(ch, true);
				return eSUCCESS;
			}
			else if (choice == 6 && ch->weight <= races[ch->race].min_weight)
			{
				ch->sendln("You cannot decrease your weight any more.");
				heightweight(ch, true);
				return eSUCCESS;
			}
			heightweight(ch, true);

			if (GET_PLATINUM(ch) < 250)
			{
				ch->sendln("You cannot afford it.");
				return eSUCCESS;
			}
			GET_PLATINUM(ch) -= 250;
			ch->sendln("Cardinal Thelonius gropes you.");
			if (choice == 3)
			{
				ch->height++;
				logf(ANGEL, LogChannels::LOG_MORTAL, "%s metas height by 1 = %d", GET_NAME(ch), GET_HEIGHT(ch));
			}
			if (choice == 4)
			{
				ch->height--;
				logf(ANGEL, LogChannels::LOG_MORTAL, "%s metas height by -1 = %d", GET_NAME(ch), GET_HEIGHT(ch));
			}
			if (choice == 5)
			{
				ch->weight++;
				logf(ANGEL, LogChannels::LOG_MORTAL, "%s metas weight by 1 = %d", GET_NAME(ch), GET_WEIGHT(ch));
			}
			if (choice == 6)
			{
				ch->weight--;
				logf(ANGEL, LogChannels::LOG_MORTAL, "%s metas weight by -1 = %d", GET_NAME(ch), GET_WEIGHT(ch));
			}
			return eSUCCESS;
		}
		else if (choice == MAX_PC_RACE + 7)
		{
			if (GET_QPOINTS(ch) < 5)
			{
				ch->sendln("Costs 5 qpoints...which you don't have.");
				return eSUCCESS;
			}
			if (IS_MOB(ch))
			{
				ch->sendln("You can't buy age, chode...");
				return eSUCCESS;
			}
			if (GET_AGE(ch) >= 500)
			{
				ch->sendln("You've reached the 500 age limit that can be purchased per character.");
				return eSUCCESS;
			}

			GET_QPOINTS(ch) -= 5;
			GET_AGE_METAS(ch) += 1;
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			logf(110, LogChannels::LOG_MORTAL, "%s metas 1 age for 5 qpoints.", GET_NAME(ch));
			ch->save(10);

			return eSUCCESS;
		}
		else if (choice == MAX_PC_RACE + 8)
		{
			if (GET_QPOINTS(ch) < 5)
			{
				ch->sendln("Costs 5 qpoints...which you don't have.");
				return eSUCCESS;
			}
			if (IS_MOB(ch))
			{
				ch->sendln("You can't buy age, chode...");
				return eSUCCESS;
			}
			if (GET_AGE(ch) <= 18)
			{
				ch->sendln("You've reached the age 18 minimum limit that can be purchased per character.");
				return eSUCCESS;
			}

			GET_QPOINTS(ch) -= 5;
			GET_AGE_METAS(ch) -= 1;
			act("The Meta-physician touches $n.", ch, 0, 0, TO_ROOM, 0);
			act("The Meta-physician touches you.", ch, 0, 0, TO_CHAR, 0);
			logf(110, LogChannels::LOG_MORTAL, "%s metas -1 age for 5 qpoints.", GET_NAME(ch));
			ch->save(10);

			return eSUCCESS;
		}
		else
		{
			ch->sendln("$B$2Cardinal Thelonius tells you, 'I don't have that. Try \"list\".'$R");
			return eSUCCESS;
		}
	}

	return eFAILURE;
}

/*

 END CARDINAL THELONIUS

 */
