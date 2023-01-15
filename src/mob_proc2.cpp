/***************************************************************************
 *  file: spec_pro2.c , Special module.                    Part of DIKUMUD *
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
/* $Id: mob_proc2.cpp,v 1.89 2012/05/25 02:15:46 jhhudso Exp $ */
#include <string.h>

#include <fmt/format.h>

#include "room.h"
#include "obj.h"
#include "connect.h"
#include "timeinfo.h"
#include "utility.h"
#include "character.h"
#include "handler.h"
#include "db.h"
#include "player.h"
#include "levels.h"
#include "interp.h"
#include "act.h"
#include "returnvals.h"
#include "spells.h"
#include "const.h"
#include "inventory.h"
#include "corpse.h"

using namespace std;

extern struct obj_data *object_list;
extern struct index_data *obj_index;
extern int class_restricted(char_data *ch, struct obj_data *obj);
extern int size_restricted(char_data *ch, struct obj_data *obj);

void repair_shop_fix_eq(char_data *ch, char_data *owner, int price, obj_data *obj)
{
	char buf[256];

	GET_GOLD(ch) -= price;
	eq_remove_damage(obj);
	sprintf(buf, "It will cost you %d coins to repair %s.", price, obj->short_description);
	do_say(owner, buf, CMD_DEFAULT);
	act("You watch $N fix $p...", ch, obj, owner, TO_CHAR, 0);
	act("You watch $N fix $p...", ch, obj, owner, TO_ROOM, 0);
	do_say(owner, "All fixed!", CMD_DEFAULT);
	act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
	act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
}

void repair_shop_complain_no_cash(char_data *ch, char_data *owner, int price, obj_data *obj)
{
	char buf[256];

	do_say(owner, "Trying to sucker me for a free repair job?", CMD_DEFAULT);
	sprintf(buf, "It would cost %d coins to repair %s, which you don't have!", price, obj->short_description);
	do_say(owner, buf, CMD_DEFAULT);
	act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
	act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
}

void repair_shop_price_check(char_data *ch, char_data *owner, int price, obj_data *obj)
{
	char buf[256];

	sprintf(buf, "It will only cost you %d coins to repair %s.'", price, obj->short_description);
	do_say(owner, buf, CMD_DEFAULT);
	act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
	act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
}

int repair_guy(char_data *ch, struct obj_data *obj, int cmd, const char *arg, char_data *owner)
{
	char item[256];
	int value0, cost, price;
	int percent, eqdam;

	if ((cmd != 66) && (cmd != 65))
		return eFAILURE;

	if (!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF))
	{
		send_to_char("Your criminal acts prohibit it.\r\n", ch);
		return eSUCCESS;
	}

	one_argument(arg, item);

	if (!*item)
	{
		send_to_char("What item?\n\r", ch);
		return eSUCCESS;
	}

	obj = get_obj_in_list_vis(ch, item, ch->carrying);

	if (obj == NULL)
	{
		send_to_char("You don't have that item.\r\n", ch);
		return eSUCCESS;
	}

	act("You give $N $p.", ch, obj, owner, TO_CHAR, 0);
	act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL);
	act("\n\r$N examines $p...", ch, obj, owner, TO_CHAR, 0);
	act("\n\r$N examines $p...", ch, obj, owner, TO_ROOM, INVIS_NULL);

	if (IS_OBJ_STAT(obj, ITEM_NOREPAIR) || obj->obj_flags.type_flag != ITEM_ARMOR || IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
	{
		do_say(owner, "I can't repair this.", CMD_DEFAULT);
		act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
		act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
		return eSUCCESS;
	}

	eqdam = eq_current_damage(obj);

	if (eqdam <= 0)
	{
		do_say(owner, "Looks fine to me.", CMD_DEFAULT);
		act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
		act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
		return eSUCCESS;
	}

	cost = obj->obj_flags.cost;
	value0 = eq_max_damage(obj);
	percent = ((100 * eqdam) / value0);
	price = ((cost * percent) / 100); // now we know what to charge them fuckers!

	if (price < 100)
		price = 100; // Welp.. Repair Guy needs to feed the kids somehow.. :)

	if (cmd == 65)
	{
		repair_shop_price_check(ch, owner, price, obj);
		return eSUCCESS;
	}

	if (GET_GOLD(ch) < (uint32_t)price)
	{
		repair_shop_complain_no_cash(ch, owner, price, obj);
		return eSUCCESS;
	}

	repair_shop_fix_eq(ch, owner, price, obj);
	return eSUCCESS;
}

int super_repair_guy(char_data *ch, struct obj_data *obj, int cmd, const char *arg, char_data *owner)
{
	char item[256];
	int value0, value2, cost, price;
	int percent, eqdam;

	if ((cmd != 66) && (cmd != 65))
		return eFAILURE;

	if (!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF))
	{
		send_to_char("Your criminal acts prohibit it.\r\n", ch);
		return eSUCCESS;
	}

	one_argument(arg, item);

	if (!*item)
	{
		send_to_char("What item?\n\r", ch);
		return eSUCCESS;
	}

	obj = get_obj_in_list_vis(ch, item, ch->carrying);

	if (obj == NULL)
	{
		send_to_char("You don't have that item.\r\n", ch);
		return eSUCCESS;
	}

	if (IS_OBJ_STAT(obj, ITEM_NOREPAIR))
	{
		do_say(owner, "I can't repair this.", CMD_DEFAULT);
		return eSUCCESS;
	}

	act("You give $N $p.", ch, obj, owner, TO_CHAR, 0);
	act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL);
	act("\n\r$N examines $p...", ch, obj, owner, TO_CHAR, 0);
	act("\n\r$N examines $p...", ch, obj, owner, TO_ROOM, INVIS_NULL);

	eqdam = eq_current_damage(obj);

	if (eqdam <= 0)
	{
		do_say(owner, "Looks fine to me.", CMD_DEFAULT);
		act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
		act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
		return eSUCCESS;
	}

	cost = obj->obj_flags.cost;
	value0 = eq_max_damage(obj);
	value2 = obj->obj_flags.value[2];

	if ((obj->obj_flags.type_flag == ITEM_ARMOR || ARE_CONTAINERS(obj) || obj->obj_flags.type_flag == ITEM_LIGHT) && !IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
	{
		percent = ((100 * eqdam) / value0); /* now we know what percent to repair ..  */
		price = ((cost * percent) / 100);	/* now we know what to charge */
		price *= 2;							/* he likes to charge more..  */
											/*  for armor... cuz he can.. */
	}
	else if ((obj->obj_flags.type_flag == ITEM_WEAPON || obj->obj_flags.type_flag == ITEM_FIREWEAPON || obj->obj_flags.type_flag == ITEM_INSTRUMENT || obj->obj_flags.type_flag == ITEM_STAFF || obj->obj_flags.type_flag == ITEM_WAND) && !IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
	{
		percent = ((100 * eqdam) / (value0 + value2)); /* now we know what percent to repair ..  */
		price = ((cost * percent) / 100);			   /* now we know what to charge */
	}
	else
	{
		// Dunno how to repair non-weapons/armor
		do_say(owner, "I can't repair this.", CMD_DEFAULT);
		act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
		act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
		return eSUCCESS;
	}

	if (price < 1000)
		price = 1000; // Minimum price

	if (cmd == 65)
	{
		repair_shop_price_check(ch, owner, price, obj);
		return eSUCCESS;
	}

	if (GET_GOLD(ch) < (uint32_t)price)
	{
		repair_shop_complain_no_cash(ch, owner, price, obj);
		return eSUCCESS;
	}
	else
	{
		repair_shop_fix_eq(ch, owner, price, obj);
		return eSUCCESS;
	}

	return eSUCCESS;
}

// Fingers
int repair_shop(char_data *ch, struct obj_data *obj, int cmd, const char *arg, char_data *owner)
{
	char item[256];
	int value0, value2, cost, price;
	int percent, eqdam;

	if ((cmd != 66) && (cmd != 65))
		return eFAILURE;

	if (!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF))
	{
		send_to_char("Your criminal acts prohibit it.\r\n", ch);
		return eSUCCESS;
	}

	one_argument(arg, item);

	if (!*item)
	{
		send_to_char("What item?\n\r", ch);
		return eSUCCESS;
	}

	obj = get_obj_in_list_vis(ch, item, ch->carrying);

	if (obj == NULL)
	{
		send_to_char("You don't have that item.\r\n", ch);
		return eSUCCESS;
	}

	if (IS_OBJ_STAT(obj, ITEM_NOREPAIR))
	{
		do_say(owner, "I can't repair this.", CMD_DEFAULT);
		return eSUCCESS;
	}

	act("You give $N $p.", ch, obj, owner, TO_CHAR, 0);
	act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL);
	act("\n\r$N examines $p...", ch, obj, owner, TO_CHAR, 0);
	act("\n\r$N examines $p...", ch, obj, owner, TO_ROOM, INVIS_NULL);

	eqdam = eq_current_damage(obj);

	if (eqdam <= 0)
	{
		do_say(owner, "Looks fine to me.", CMD_DEFAULT);
		act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
		act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
		return eSUCCESS;
	}

	cost = obj->obj_flags.cost;
	value0 = eq_max_damage(obj);
	value2 = obj->obj_flags.value[2];

	if ((obj->obj_flags.type_flag == ITEM_ARMOR || obj->obj_flags.type_flag == ITEM_LIGHT) && !IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
	{

		percent = ((100 * eqdam) / value0); /* now we know what percent to repair ..  */
		price = ((cost * percent) / 100);	/* now we know what to charge them fuckers! */
		price *= 4;							/* he likes to charge more..  */
											/*  for armor... cuz he's a crook..  */
	}
	else if ((obj->obj_flags.type_flag == ITEM_WEAPON || obj->obj_flags.type_flag == ITEM_FIREWEAPON || ARE_CONTAINERS(obj) || obj->obj_flags.type_flag == ITEM_STAFF || obj->obj_flags.type_flag == ITEM_WAND) && !IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))

	{

		percent = ((100 * eqdam) / (value0 + value2));
		//  x = (100 - percent);          /* now we know what percent to repair ..  */
		price = ((cost * percent) / 100); /* now we know what to charge them fuckers! */
		price *= 3;
	}
	else
	{
		// Dunno how to repair non-weapons/armor
		do_say(owner, "I can't repair this.", CMD_DEFAULT);
		act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
		act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
		return eSUCCESS;
	}

	if (price < 5000)
		price = 5000; /* Welp.. Repair Guy needs to feed the kids somehow.. :) */

	if (cmd == 65)
	{
		repair_shop_price_check(ch, owner, price, obj);
		return eSUCCESS;
	}

	if (GET_GOLD(ch) < (uint32_t)price)
	{
		repair_shop_complain_no_cash(ch, owner, price, obj);
		return eSUCCESS;
	}
	else
	{
		repair_shop_fix_eq(ch, owner, price, obj);
		return eSUCCESS;
	}
}

int corpse_cost(char_data *ch)
{
	int cost = 0;
	obj_data *curr_cont;

	for (obj_data *obj2 = ch->carrying; obj2; obj2 = obj2->next_content)
	{
		if (obj2->obj_flags.type_flag == ITEM_MONEY)
			continue;
		for (curr_cont = obj2->contains; curr_cont; curr_cont = curr_cont->next_content)
		{
			if (!IS_SET(curr_cont->obj_flags.extra_flags, ITEM_SPECIAL))
				cost += curr_cont->obj_flags.cost;
		}
		if (!IS_SET(obj2->obj_flags.extra_flags, ITEM_SPECIAL))
			cost += obj2->obj_flags.cost;
	}
	for (int x = 0; x < MAX_WEAR; x++)
	{
		if (ch->equipment[x])
		{
			for (curr_cont = ch->equipment[x]->contains; curr_cont; curr_cont = curr_cont->next_content)
				if (!IS_SET(curr_cont->obj_flags.extra_flags, ITEM_SPECIAL))
					cost += curr_cont->obj_flags.cost;

			if (!IS_SET(ch->equipment[x]->obj_flags.extra_flags, ITEM_SPECIAL))
				cost += ch->equipment[x]->obj_flags.cost;
		}
	}
	return cost;
}

int corpse_cost(obj_data *obj)
{
	int cost = 0;
	obj_data *curr_cont;

	for (obj_data *obj2 = obj->contains; obj2; obj2 = obj2->next_content)
	{
		if (obj2->obj_flags.type_flag == ITEM_MONEY)
			continue;
		for (curr_cont = obj2->contains; curr_cont; curr_cont = curr_cont->next_content)
			cost += curr_cont->obj_flags.cost;
		cost += obj2->obj_flags.cost;
	}
	return cost;
}

int mortician(char_data *ch, struct obj_data *obj, int cmd, const char *arg, char_data *owner)
{
	int x = 0, cost = 0, which;
	int count = 0;
	char buf[100];

	if (cmd != 56 && cmd != 59 && cmd != 58)
		return eFAILURE;

	// TODO - when determining price, it WILL NOT work if we ever institute
	// containers being inside other containers.

	if (cmd == 59) // list
	{
		sprintf(buf, "%s_consent", GET_NAME(ch));
		send_to_char("Available corpses (freshest first):\n\r$B", ch);
		for (obj = object_list; obj; obj = obj->next)
		{
			if (GET_ITEM_TYPE(obj) != ITEM_CONTAINER || obj->obj_flags.value[3] != 1) // only look at corpses
				continue;

			if (!isname("pc", obj->name) || (!isname(GET_NAME(ch), obj->name) && !isname(buf, obj->name)))
				continue;

			if (obj->in_room == ch->in_room)
				continue;
			if (!obj->contains) // skip empty corpses
				continue;

			cost = corpse_cost(obj);
			cost /= 20000;
			cost = MAX(cost, 30);
			sprintf(buf, "%d) %-21s %d Platinum coins.\r\n", ++count, obj->short_description, cost);
			send_to_char(buf, ch);
		}
		send_to_char("$RIf any corpses were listed, they are still where you left them.  This\n\r"
					 "list is therefore always changing.  If you purchase one, it will be\n\r"
					 "placed at your feet. Use \"buy <number>\" to purchase a corpse.\r\n"
					 "Use 'value' to find how much your eq would cost with what you\n\r"
					 "have on you now.\r\n",
					 ch);
		return eSUCCESS;
	}

	if (cmd == 58) // value
	{
		cost = corpse_cost(ch);
		cost /= 20000;
		cost = MAX(cost, 30);
		csendf(ch, "The Undertaker takes a look at you and estimates your corpse would cost around %d platinum coins.\r\n", cost);
		return eSUCCESS;
	}

	/* buy */
	if ((which = atoi(arg)) == 0)
	{
		send_to_char("Try \"buy <number>\", or \"list\" for a list of "
					 "available corpses.\r\n",
					 ch);
		return eSUCCESS;
	}

	for (obj = object_list; obj; obj = obj->next)
	{
		sprintf(buf, "%s_consent", GET_NAME(ch));

		if (GET_ITEM_TYPE(obj) != ITEM_CONTAINER || obj->obj_flags.value[3] != 1) // only look at corpses
			continue;

		if (!isname("pc", obj->name) || (!isname(GET_NAME(ch), obj->name) && !isname(buf, obj->name)) || ++x < which)
			continue;

		if (!obj->contains) // skip empty corpses
			continue;

		if (obj->in_room == ch->in_room)
			continue; // Skip bought corpses

		cost = corpse_cost(obj);
		cost /= 20000;
		cost = MAX(cost, 30);
		if (GET_PLATINUM(ch) < (uint32_t)cost)
		{
			send_to_char("You can't afford that!\n\r", ch);
			return eSUCCESS;
		}
		move_obj(obj, ch->in_room);
		REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
		send_to_char("The mortician goes into his freezer and returns with a corpse, which he\n\r"
					 "places at your feet.\r\n",
					 ch);
		GET_PLATINUM(ch) -= cost;
		ch->save(10);
		save_corpses();
		return eSUCCESS;
	}

	send_to_char("No such corpse was found.  Try \"list\".\r\n", ch);
	return eSUCCESS;
}

char *gl_item(obj_data *obj, int number, char_data *ch, bool platinum = true)
{
	string buf = {}, buf2 = {}, buf3 = {};
	size_t length = {};

	if (platinum)
	{
		buf = fmt::format("$B$7{:-2}$R) {} ", number + 1, obj->short_description);
	}
	else
	{
		buf = fmt::format("$B$7{:-2}$R) $3$B{}$R ", number + 1, obj->short_description);
	}

	if (obj->obj_flags.type_flag == ITEM_WEAPON)
	{ // weapon
		buf = fmt::format("{}{}d{}, {}, ", buf, obj->obj_flags.value[1], obj->obj_flags.value[2], IS_SET(obj->obj_flags.extra_flags, ITEM_TWO_HANDED) ? "Two-handed" : "One-handed");
	}

	for (uint i = 0; i < obj->num_affects; i++)
	{
		if ((obj->affected[i].location != APPLY_NONE) && (obj->affected[i].modifier != 0))
		{
			if (obj->affected[i].location < 1000)
			{
				buf2 = sprinttype(obj->affected[i].location, apply_types);
			}
			else if (get_skill_name(obj->affected[i].location / 1000))
			{
				buf2 = get_skill_name(obj->affected[i].location / 1000);
			}
			else
			{
				buf2 = "Invalid";
			}

			buf3 = fmt::format("{} by {}, ", buf2, obj->affected[i].modifier);

			for (auto &ch : buf3)
			{
				ch = LOWER(ch);
			}

			auto potential_buffer = buf + buf3;
			auto starting_point = potential_buffer.find_last_of("\n");
			if (starting_point == potential_buffer.npos)
			{
				starting_point = 0;
			}
			length = nocolor_strlen(potential_buffer.substr(starting_point).c_str());

			if (length > 79)
			{
				buf = fmt::format("{}\r\n    {}", buf, buf3);
			}
			else
			{
				buf = potential_buffer;
			}
		}
	}

	// Room where Orro, the Quest Guide is located
	if (ch->in_room != 3055)
	{
		if (class_restricted(ch, obj) || size_restricted(ch, obj))
		{
			buf2 = "$4[restricted]$R, ";

			auto potential_buffer = buf + buf2;
			auto starting_point = potential_buffer.find_last_of("\n");
			if (starting_point == potential_buffer.npos)
			{
				starting_point = 0;
			}
			length = nocolor_strlen(potential_buffer.substr(starting_point).c_str());
			if (length > 79)
			{
				buf = fmt::format("{}\r\n    {}", buf, buf2);
			}
			else
			{
				buf = potential_buffer;
			}
		}
	}
	else
	{
		uint32_t a = obj->obj_flags.extra_flags;
		a &= ALL_CLASSES;

		buf2 = fmt::format("{}]$R, ", sprintbit(a, extra_bits));

		if (a)
		{
			auto potential_buffer = fmt::format("{}$4[{}", buf, buf2);
			auto starting_point = potential_buffer.find_last_of("\n");
			if (starting_point == potential_buffer.npos)
			{
				starting_point = 0;
			}
			length = nocolor_strlen(potential_buffer.substr(starting_point).c_str());

			if (length > 79)
			{
				buf = fmt::format("{}\r\n    $4[{}", buf, buf2);
			}
			else
			{
				buf = potential_buffer;
			}
		}
	}

	if (platinum)
	{
		buf2 = fmt::format("costing {} coins.", obj->obj_flags.cost / 10);
	}
	else
	{
		buf2 = fmt::format("costing {} qpoints.", obj->obj_flags.cost / 10000);
	}

	auto potential_buffer = buf + buf2;
	auto starting_point = potential_buffer.find_last_of("\n");
	if (starting_point == potential_buffer.npos)
	{
		starting_point = 0;
	}
	length = nocolor_strlen(potential_buffer.substr(starting_point).c_str());

	if (length > 79)
	{
		buf = fmt::format("{}\r\n    {}", buf, buf2);
	}
	else
	{
		buf = potential_buffer;
	}

	buf += "\r\n";
	return str_dup(buf.c_str());
}

struct platsmith
{
	int vnum;
	int sales[13];
};

const struct platsmith platsmith_list[] = {{10019, {512, 513, 514, 515, 537, 538, 539, 540, 541, 0, 0, 0, 0}}, {10020, {554, 555, 556, 557, 524, 525, 526, 527, 504, 505, 506, 511, 0}}, {10021, {516, 517, 518, 519, 507, 508, 509, 510, 546, 547, 548, 549, 0}}, {10022, {500, 501, 502, 503, 520, 521, 522, 523, 528, 529, 530, 531, 0}}, {10023, {542, 543, 544, 545, 532, 533, 534, 535, 536, 550, 551, 552, 553}}, {10026, {558, 559, 560, 561, 562, 563, 564, 565, 566, 0, 0, 0, 0}}, {10004, {570, 571, 575, 577, 578, 580, 582, 584, 586, 587, 590, 591, 598}}, // weapon dude in cozy
										   {10024, {592, 593, 594, 567, 568, 0, 0, 0, 0, 0, 0, 0, 0}},																																																																																																																	 // 2handed weapon/bow dude
										   {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}};

// Apoc enjoys the dirty mooselove. Honest.
int godload_sales(char_data *ch, struct obj_data *obj, int cmd, const char *arg, char_data *owner)
{
	extern struct index_data *mob_index;
	int mobvnum = mob_index[owner->mobdata->nr].virt;
	int o;
	char buf[MAX_STRING_LENGTH];
	//  return eFAILURE; //disabled for now
	if (cmd == 59)
	{
		if (!CAN_SEE(owner, ch))
		{
			do_say(owner, "I don't trade with people I can't see!", 0);
			return eSUCCESS;
		}

		for (o = 0; platsmith_list[o].vnum != 0; o++)
			if (mobvnum == platsmith_list[o].vnum)
				break;
		if (platsmith_list[o].vnum == 0)
		{
			sprintf(buf, "%s Sorry, I don't seem to be working correctly. Do tell someone.", GET_NAME(ch));
			do_tell(owner, buf, 0);
			return eSUCCESS;
		}
		sprintf(buf, "%s Here's what I can do for you, %s.", GET_NAME(ch), pc_clss_types3[GET_CLASS(ch)]);
		do_tell(owner, buf, 0);
		for (int z = 0; z < 13 && platsmith_list[o].sales[z] != 0; z++)
		{
			char *tmp = gl_item((obj_data *)obj_index[real_object(platsmith_list[o].sales[z])].item, z, ch);
			send_to_char(tmp, ch);
			dc_free(tmp);
		}
		return eSUCCESS;
	}
	else if (cmd == 56)
	{
		if (!CAN_SEE(owner, ch))
		{
			do_say(owner, "I don't trade with people I can't see!", 0);
			return eSUCCESS;
		}

		for (o = 0; platsmith_list[o].vnum != 0; o++)
			if (mobvnum == platsmith_list[o].vnum)
				break;
		char buf[MAX_STRING_LENGTH], arg2[MAX_INPUT_LENGTH];
		one_argument(arg, arg2);
		if (platsmith_list[o].vnum == 0)
		{
			sprintf(buf, "%s Sorry, I don't seem to be working correctly. Do tell someone.", GET_NAME(ch));
			do_tell(owner, buf, 0);
			return eSUCCESS;
		}
		if (!is_number(arg2))
		{
			sprintf(buf, "%s Sorry, mate. You type buy <number> to specify what you want..", GET_NAME(ch));
			do_tell(owner, buf, 0);
			return eSUCCESS;
		}
		int k = atoi(arg2) - 1;
		if (k >= 13 || k < 0 || platsmith_list[o].sales[k] == 0)
		{
			sprintf(buf, "%s Don't have that I'm afraid. Type \"list\" to see my wares.", GET_NAME(ch));
			do_tell(owner, buf, 0);
			return eSUCCESS;
		}
		struct obj_data *obj;
		obj = clone_object(real_object(platsmith_list[o].sales[k]));

		if (class_restricted(ch, obj) || size_restricted(ch, obj) || search_char_for_item(ch, obj->item_number, false))
		{
			sprintf(buf, "%s That item is not available to you.", GET_NAME(ch));
			do_tell(owner, buf, 0);
			extract_obj(obj);
			return eSUCCESS;
		}
		if (GET_PLATINUM(ch) < (unsigned int)(obj->obj_flags.cost / 10))
		{
			sprintf(buf, "%s Come back when you've got the platinum.", GET_NAME(ch));
			do_tell(owner, buf, 0);
			extract_obj(obj);
			return eSUCCESS;
		}
		GET_PLATINUM(ch) -= obj->obj_flags.cost / 10;
		sprintf(buf, "%s %s", obj->name, GET_NAME(ch));
		obj->name = str_hsh(buf);
		obj_to_char(obj, ch);
		sprintf(buf, "%s Here's your %s$B$2. Have a nice time with it.", GET_NAME(ch), obj->short_description);
		do_tell(owner, buf, 0);
		return eSUCCESS;
	}
	else if (cmd == 57)
	{
		obj_data *obj;
		char arg2[MAX_INPUT_LENGTH];
		one_argument(arg, arg2);
		obj = get_obj_in_list_vis(ch, arg2, ch->carrying);
		if (!CAN_SEE(owner, ch))
		{
			do_say(owner, "I don't trade with people I can't see!", 0);
			return eSUCCESS;
		}
		if (!obj)
		{
			sprintf(buf, "%s Try that on the kooky meta-physician..", GET_NAME(ch));
			do_tell(owner, buf, 0);
			return eSUCCESS;
		}
		if (!IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
		{
			sprintf(buf, "%s I don't deal in worthless junk.", GET_NAME(ch));
			do_tell(owner, buf, 0);
			return eSUCCESS;
		}

		// don't allow non-empty containers to be sold
		if (obj->obj_flags.type_flag == ITEM_CONTAINER && obj->contains)
		{
			sprintf(buf, "%s %s$B$2 needs to be emptied first.", GET_NAME(ch), GET_OBJ_SHORT(obj));
			do_tell(owner, buf, 0);
			return eSUCCESS;
		}

		int cost = obj->obj_flags.cost / 10;

		sprintf(buf, "%s I'll give you %d plats for that. Thanks for shoppin'.", GET_NAME(ch), cost);
		do_tell(owner, buf, 0);
		extract_obj(obj);
		GET_PLATINUM(ch) += cost;
		return eSUCCESS;
	}
	return eFAILURE;
}

// gl_repair_guy
int gl_repair_shop(char_data *ch, struct obj_data *obj, int cmd, const char *arg, char_data *owner)
{
	char item[256];
	int value0, value2, cost, price;
	int percent, eqdam;

	if ((cmd != 66) && (cmd != 65))
		return eFAILURE;

	if (!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF))
	{
		send_to_char("Your criminal acts prohibit it.\r\n", ch);
		return eSUCCESS;
	}

	one_argument(arg, item);

	if (!*item)
	{
		send_to_char("What item?\n\r", ch);
		return eSUCCESS;
	}

	obj = get_obj_in_list_vis(ch, item, ch->carrying);

	if (obj == NULL)
	{
		send_to_char("You don't have that item.\r\n", ch);
		return eSUCCESS;
	}

	if (IS_OBJ_STAT(obj, ITEM_NOREPAIR))
	{
		do_say(owner, "I can't repair this.", CMD_DEFAULT);
		return eSUCCESS;
	}

	act("You give $N $p.", ch, obj, owner, TO_CHAR, 0);
	act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL);
	act("\n\r$N examines $p...", ch, obj, owner, TO_CHAR, 0);
	act("\n\r$N examines $p...", ch, obj, owner, TO_ROOM, INVIS_NULL);

	eqdam = eq_current_damage(obj);

	if (eqdam <= 0)
	{
		do_say(owner, "Looks fine to me.", CMD_DEFAULT);
		act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
		act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
		return eSUCCESS;
	}

	cost = obj->obj_flags.cost;
	value0 = eq_max_damage(obj);
	value2 = obj->obj_flags.value[2];

	if (!IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
	{
		do_say(owner, "I don't repair this kind of junk.", CMD_DEFAULT);
		act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
		act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
		return eSUCCESS;
	}
	if (obj->obj_flags.type_flag == ITEM_ARMOR || obj->obj_flags.type_flag == ITEM_LIGHT)
	{

		percent = ((100 * eqdam) / value0);
		price = ((cost * percent) / 100); /* now we know what to charge them fuckers! */
		price *= 4;						  /* he likes to charge more..  */
										  /*  for armor... cuz he's a crook..  */
	}
	else if (obj->obj_flags.type_flag == ITEM_WEAPON || obj->obj_flags.type_flag == ITEM_FIREWEAPON || ARE_CONTAINERS(obj) || obj->obj_flags.type_flag == ITEM_STAFF || obj->obj_flags.type_flag == ITEM_WAND || obj->obj_flags.type_flag == ITEM_INSTRUMENT)
	{

		percent = ((100 * eqdam) / (value0 + value2));
		price = ((cost * percent) / 100); /* now we know what to charge them fuckers! */
		price *= 5;
	}
	else
	{
		// Dunno how to repair non-weapons/armor
		do_say(owner, "I can't repair this.", CMD_DEFAULT);
		act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
		act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
		return eSUCCESS;
	}

	if (price < 50000)
		price = 50000; /* Welp.. Repair Guy needs to feed the kids somehow.. :) */

	if (cmd == 65)
	{
		repair_shop_price_check(ch, owner, price, obj);
		return eSUCCESS;
	}

	if (GET_GOLD(ch) < (uint32_t)price)
	{
		repair_shop_complain_no_cash(ch, owner, price, obj);
		return eSUCCESS;
	}
	else
	{
		repair_shop_fix_eq(ch, owner, price, obj);
		return eSUCCESS;
	}
}
