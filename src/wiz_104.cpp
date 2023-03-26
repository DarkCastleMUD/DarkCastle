#include <vector>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <string>
#include <utility>

#include <QTimeZone>

#include "wizard.h"
#include "utility.h"
#include "connect.h"
#include "mobile.h"
#include "player.h"
#include "levels.h"
#include "obj.h"
#include "handler.h"
#include "db.h"
#include "room.h"
#include "interp.h"
#include "returnvals.h"
#include "spells.h"
#include "race.h"
#include "const.h"
#include "corpse.h"

using namespace std;

int count_rooms(int start, int end)
{
	if (start < 0 || end < 0 || start > 1000000 || end > 1000000)
	{
		return 0;
	}

	if (start > end)
	{
		swap<int>(start, end);
	}

	int count = 0;
	for (int i = start; i < top_of_world && i <= end; i++)
	{
		if (!world_array[i])
			continue;
		count++;
	}

	return count;
}

int do_thunder(Character *ch, char *argument, int cmd)
{
	char buf1[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	class Connection *i;
	char buf3[MAX_INPUT_LENGTH];

	if (IS_PC(ch) && ch->player->wizinvis)
		sprintf(buf3, "someone");
	else
		sprintf(buf3, GET_SHORT(ch));

	for (; *argument == ' '; argument++)
		;

	if (!(*argument))
		send_to_char("It's not gonna look that impressive...\r\n", ch);
	else
	{
		if (cmd == CMD_DEFAULT)
			sprintf(buf2, "$4$BYou thunder '%s'$R", argument);
		else
			sprintf(buf2, "$7$BYou bellow '%s'$R", argument);
		act(buf2, ch, 0, 0, TO_CHAR, 0);

		for (i = DC::getInstance()->descriptor_list; i; i = i->next)
			if (i->character != ch && !i->connected)
			{
				if (IS_PC(ch) && ch->player->wizinvis && i->character->level < ch->player->wizinvis)
					sprintf(buf3, "Someone");
				else
					sprintf(buf3, GET_SHORT(ch));

				if (cmd == CMD_DEFAULT)
				{
					sprintf(buf1, "$B$4%s thunders '%s'$R\n\r", buf3, argument);
				}
				else
				{
					sprintf(buf1, "$7$B%s bellows '%s'$R\r\n", buf3, argument);
				}

				send_to_char(buf1, i->character);
			}
	}
	return eSUCCESS;
}

int do_incognito(Character *ch, char *argument, int cmd)
{
	if (IS_MOB(ch))
		return eFAILURE;

	if (ch->player->incognito == true)
	{
		send_to_char("Incognito off.\r\n", ch);
		ch->player->incognito = false;
	}
	else
	{
		send_to_char("Incognito on.  Even while invis, anyone in your room can "
					 "see you.\r\n",
					 ch);
		ch->player->incognito = true;
	}
	return eSUCCESS;
}

int do_load(Character *ch, char *arg, int cmd)
{
	char type[MAX_INPUT_LENGTH] = {0};
	char name[MAX_INPUT_LENGTH] = {0};
	char arg2[MAX_INPUT_LENGTH] = {0};
	char arg3[MAX_INPUT_LENGTH] = {0};
	char qty[MAX_INPUT_LENGTH] = {0};
	char random[MAX_INPUT_LENGTH] = {0};
	char buf[MAX_STRING_LENGTH] = {0};

	char *c;
	int x, number = 0, num = 0, cnt = 1;

	char *types[] = {
		"mobile",
		"object",
	};

	if (IS_NPC(ch))
		return eFAILURE;

	if (!has_skill(ch, COMMAND_LOAD) && cmd == CMD_DEFAULT)
	{
		send_to_char("Huh?\r\n", ch);
		return eFAILURE;
	}
	if (!has_skill(ch, COMMAND_PRIZE) && cmd == CMD_PRIZE)
	{
		send_to_char("Huh?\r\n", ch);
		return eFAILURE;
	}

	half_chop(arg, type, arg2);

	if (cmd == CMD_DEFAULT && (!*type || !*arg2))
	{
		send_to_char("Usage:  load <mob> <name|vnum> [qty]\n\r", ch);
		send_to_char("        load <obj> <name|vnum> [qty] [random]\n\r", ch);
		return eFAILURE;
	}
	if (cmd == CMD_PRIZE && !*type)
	{

		*buf = '\0';
		send_to_char("[#  ] [OBJ #] OBJECT'S DESCRIPTION\n\n\r", ch);

		for (x = 0; (x < obj_index[top_of_objt].virt); x++)
		{
			if ((num = real_object(x)) < 0)
				continue;

			if (isname("prize", ((class Object *)(obj_index[num].item))->name))
			{
				cnt++;
				sprintf(buf, "[%3d] [%5d] %s\n\r", cnt, x, ((class Object *)(obj_index[num].item))->short_description);
				send_to_char(buf, ch);
			}

			if (cnt > 200)
			{
				send_to_char("Maximum number of searchable items hit.  Search ended.\r\n", ch);
				break;
			}
		}

		send_to_char("To load: prize <name|vnum>\r\n", ch);
		return eFAILURE;
	}

	if (cmd == CMD_DEFAULT)
	{
		half_chop(arg2, name, arg3);

		if (arg3[0])
		{
			cnt = atoi(arg3);
			half_chop(arg3, qty, random);
		}

		if (cnt > 50)
		{
			send_to_char("Sorry, you can only load at most 50 of something at a time.\r\n", ch);
			return eFAILURE;
		}
	}

	if (cmd == CMD_DEFAULT)
		c = name;
	else
		c = type;

	if (cmd == CMD_DEFAULT)
	{
		for (x = 0; x <= 2; x++)
		{
			if (x == 2)
			{
				send_to_char("Type must mobile or object.\r\n", ch);
				return eFAILURE;
			}
			if (is_abbrev(type, types[x]))
				break;
		}
	}
	else
		x = 1;

	switch (x)
	{
	default:
		send_to_char("Problem...fuck up in do_load.\r\n", ch);
		logentry("Default in do_load...should NOT happen.", ANGEL, LogChannels::LOG_BUG);
		return eFAILURE;
	case 0: /* mobile */
		if ((number = number_or_name(&c, &num)) == 0)
			return eFAILURE;
		else if (number == -1)
		{
			if ((number = real_mobile(num)) < 0)
			{
				send_to_char("No such mobile.\r\n", ch);
				return eFAILURE;
			}
			if (GET_LEVEL(ch) < DEITY && !can_modify_mobile(ch, num))
			{
				send_to_char("You may only load mobs inside of your range.\r\n", ch);
				return eFAILURE;
			}
			do_mload(ch, number, cnt);
			return eSUCCESS;
		}
		if ((num = mob_in_index(c, number)) == -1)
		{
			send_to_char("No such mobile.\r\n", ch);
			return eFAILURE;
		}
		do_mload(ch, num, cnt);
		return eSUCCESS;
	case 1: /* object */
		if ((number = number_or_name(&c, &num)) == 0)
			return eFAILURE;
		else if (number == -1)
		{
			if ((number = real_object(num)) < 0)
			{
				send_to_char("No such object.\r\n", ch);
				return eFAILURE;
			}
			if ((GET_LEVEL(ch) < 108) &&
				IS_SET(((class Object *)(obj_index[number].item))->obj_flags.extra_flags, ITEM_SPECIAL))
			{
				send_to_char("Why would you want to load that?\n\r", ch);
				return eFAILURE;
			}
			else if (cmd == CMD_PRIZE && !isname("prize", ((class Object *)(obj_index[number].item))->name))
			{
				send_to_char("This command can only load prize items.\r\n", ch);
				return eFAILURE;
			}
			else if (cmd != CMD_PRIZE && GET_LEVEL(ch) < DEITY && !can_modify_object(ch, num))
			{
				send_to_char("You may only load objects inside of your range.\r\n", ch);
				return eFAILURE;
			}

			if (random[0] == 'r')
			{
				Object *obj = (Object *)(obj_index[number].item);
				if (IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
				{
					csendf(ch, "You cannot random load vnum %d because extra flag ITEM_SPECIAL is set.\r\n", num);
					return eFAILURE;
				}
				else if (IS_SET(obj->obj_flags.extra_flags, ITEM_QUEST))
				{
					csendf(ch, "You cannnot random load vnum %d because extra flag ITEM_QUEST is set.\r\n", num);
					return eFAILURE;
				}
				else if (IS_SET(obj->obj_flags.more_flags, ITEM_NO_CUSTOM))
				{
					csendf(ch, "You cannot random load vnum %d because more flag ITEM_NO_CUSTOM is set.\r\n", num);
					return eFAILURE;
				}
			}

			do_oload(ch, number, cnt, (random[0] == 'r' ? true : false));
			return eSUCCESS;
		}
		if ((num = obj_in_index(c, number)) == -1)
		{
			send_to_char("No such object.\r\n", ch);
			return eFAILURE;
		}
		if ((GET_LEVEL(ch) < IMPLEMENTER) &&
			IS_SET(((class Object *)(obj_index[num].item))->obj_flags.extra_flags,
				   ITEM_SPECIAL))
		{
			send_to_char("Why would you want to load that?\n\r", ch);
			return eFAILURE;
		}
		else if (cmd == CMD_PRIZE && !isname("prize", ((class Object *)(obj_index[num].item))->name))
		{
			send_to_char("This command can only load prize items.\r\n", ch);
			return eFAILURE;
		}

		do_oload(ch, num, cnt, (random[0] == 'r' ? true : false));
		return eSUCCESS;
	}
	return eSUCCESS;
}

int do_purge(Character *ch, char *argument, int cmd)
{
	Character *vict, *next_v;
	class Object *obj, *next_o;

	char name[100], buf[300];

	if (IS_NPC(ch))
		return eFAILURE;

	one_argument(argument, name);

	if (*name)
	{ /* argument supplied. destroy single object or char */
		if ((vict = get_char_room_vis(ch, name)) && (GET_LEVEL(ch) > G_POWER))
		{
			if (IS_PC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict)))
			{
				sprintf(buf, "%s is surrounded with scorching flames but is"
							 " unharmed.\r\n",
						GET_SHORT(vict));
				send_to_char(buf, ch);
				act("$n tried to purge you.", ch, 0, vict, TO_VICT, 0);
				return eFAILURE;
			}

			act("$n disintegrates $N.", ch, 0, vict, TO_ROOM, NOTVICT);
			act("You disintegrate $N.", ch, 0, vict, TO_CHAR, 0);

			if (vict->desc)
			{
				close_socket(vict->desc);
				vict->desc = nullptr;
			}

			extract_char(vict, true);
		}
		else if ((obj = get_obj_in_list_vis(ch, name,
											world[ch->in_room].contents)) != nullptr)
		{
			act("$n purges $p.", ch, obj, 0, TO_ROOM, 0);
			act("You purge $p.", ch, obj, 0, TO_CHAR, 0);
			extract_obj(obj);
		}
		else
		{
			send_to_char("You can't find it to purge!\n\r", ch);
			return eFAILURE;
		}
	}
	else
	{ /* no argument. clean out the room */
		if (IS_NPC(ch))
		{
			send_to_char("Don't... You would kill yourself too.\r\n", ch);
			return eFAILURE;
		}

		act("$n gestures... the room is filled with scorching flames!",
			ch, 0, 0, TO_ROOM, 0);
		send_to_char("You gesture...the room is filled with scorching "
					 "flames!\n\r",
					 ch);

		for (vict = world[ch->in_room].people; vict; vict = next_v)
		{
			next_v = vict->next_in_room;
			if (IS_NPC(vict))
				extract_char(vict, true);
		}

		for (obj = world[ch->in_room].contents; obj; obj = next_o)
		{
			next_o = obj->next_content;
			extract_obj(obj);
		}
	}
	save_corpses();
	return eSUCCESS;
}

char *dirNumToChar(int dir)
{
	switch (dir)
	{
	case 0:
		return "North";
		break;
	case 1:
		return "East";
		break;
	case 2:
		return "South";
		break;
	case 3:
		return "West";
		break;
	case 4:
		return "Up";
		break;
	case 5:
		return "Down";
		break;
	}

	return "ERROR";
}

int Zone::show_info(Character *ch)
{
	char buf[MAX_STRING_LENGTH];

	string continent_name;
	if (continent && (unsigned)continent < continent_names.size())
		continent_name = continent_names.at(continent);

	ch->send(QString("$3Name:$R %1\r\n"
					 "$3Filename:$R %2\r\n"
					 "$3Starts:$R    %3 $3Ends:$R  %4     $3Continent:$R %5\n\r"
					 "$3Starts:$R    %6 $3Ends:$R  %7\n\r"
					 "$3Lifetime:$R  %8 $3Age:$R   %9     $3Left:$R   %10\r\n"
					 "$3PC'sInZone:$R  %11 $3Mode:$R %12 $3Last full reset:$R %13 %14\r\n"
					 "$3Flags:$R ")
				 .arg(name)
				 .arg(filename)
				 .arg(bottom, 6)
				 .arg(top, 13)
				 .arg(continent_name.c_str())
				 .arg(bottom_rnum, 6)
				 .arg(top_rnum, 13)
				 .arg(lifespan, 6)
				 .arg(age, 13)
				 .arg(lifespan - age, 6)
				 .arg(players, 4)
				 .arg(zone_modes[reset_mode], -18)
				 .arg(last_full_reset.toLocalTime().toString().toStdString().c_str())
				 .arg(last_full_reset.toLocalTime().timeZoneAbbreviation().toStdString().c_str()));

	sprintbit(zone_flags, zone_bits, buf);
	send_to_char(buf, ch);
	sprintf(buf, "\r\n"
				 "$3MobsLastPop$R:  %3d $3DeathCounter$R: %6d     $3ReduceCounter$R: %d\r\n"
				 "$3DiedThisTick$R: %3d $3Repops without Deaths$R: %d $3Repops with bonus$R: %d\r\n",
			num_mob_on_repop,
			death_counter,
			counter_mod,
			died_this_tick,
			repops_without_deaths,
			repops_with_bonus);
	send_to_char(buf, ch);
	send_to_char("\r\n", ch);

	return eSUCCESS;
}

int show_zone_commands(Character *ch, int zone_key, int start)
{
	char buf[MAX_STRING_LENGTH];
	int k = 0;
	int num_to_show;

	if (start < 0)
		start = 0;

	if (DC::getInstance()->zones.contains(zone_key) == false)
	{
		ch->send(QString("Zone %1 not found.\r\n").arg(zone_key));
		return eFAILURE;
	}

	auto &zone = DC::getInstance()->zones[zone_key];

	if (zone.cmd[0].command == 'S')
	{
		send_to_char("This zone has no zone commands.\r\n", ch);
		return eFAILURE;
	}

	for (k = 0; zone.cmd[k].command != 'S'; k++)
		;

	if (k < start)
	{
		sprintf(buf, "Last command in this zone is %d.\r\n", k);
		send_to_char(buf, ch);
		return eFAILURE;
	}

	if (IS_SET(ch->player->toggles, PLR_PAGER))
		num_to_show = 50;
	else
		num_to_show = 20;

	// show zone cmds
	for (int j = start; (j < start + num_to_show) && (zone.cmd[j].command != 'S'); j++)
	{
		time_t last = zone.cmd[j].last;
		string lastStr = "never";
		if (last)
		{
			lastStr = fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(last));
		}

		time_t lastSuccess = zone.cmd[j].lastSuccess;
		string lastSuccessStr = "never";
		if (lastSuccess)
		{
			lastSuccessStr = fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(lastSuccess));
		}

		uint64_t attempts = zone.cmd[j].attempts;
		uint64_t successes = zone.cmd[j].successes;
		double successRate = 0.0;
		if (attempts > 0)
		{
			successRate = (double)successes / (double)attempts;
		}

		// show command # and if_flag
		// note that we show the command as cmd+1.  This is so we don't have a
		// command 0 from the user's perspective.
		if (zone.cmd[j].command == '*')
		{
			sprintf(buf, "[%3d] Comment: ", j + 1);
		}
		else
			switch (zone.cmd[j].if_flag)
			{
			case 0:
				sprintf(buf, "[%3d] Always ", j + 1);
				break;
			case 1:
				sprintf(buf, "[%3d] $B$2OnTrue$R ", j + 1);
				break;
			case 2:
				sprintf(buf, "[%3d] $4OnFals$R ", j + 1);
				break;
			case 3:
				sprintf(buf, "[%3d] $B$5OnBoot$R ", j + 1);
				break;
			case 4:
				sprintf(buf, "[%3d] $B$2Ls$1Mb$2Tr$R ", j + 1);
				break;
			case 5:
				sprintf(buf, "[%3d] $B$4Ls$1Mb$4Fl$R ", j + 1);
				break;
			case 6:
				sprintf(buf, "[%3d] $B$2Ls$7Ob$2Tr$R ", j + 1);
				break;
			case 7:
				sprintf(buf, "[%3d] $B$4Ls$7Ob$4Fl$R ", j + 1);
				break;
			case 8:
				sprintf(buf, "[%3d] $B$2Ls$R%%%%$B$2Tr$R ", j + 1);
				break;
			case 9:
				sprintf(buf, "[%3d] $B$4Ls$R%%%%$B$4Fl$R ", j + 1);
				break;
			default:
				sprintf(buf, "[%3d] $B$4ERROR(%d)$R", j + 1, zone.cmd[j].if_flag);
				break;
			}
		int virt;
#define ZCMD zone.cmd[j]
		switch (zone.cmd[j].command)
		{
		case 'M':
			virt = ZCMD.active ? mob_index[ZCMD.arg1].virt : ZCMD.arg1;
			sprintf(buf, "%s $B$1Load mob  [%5d] ", buf, virt);
			if (zone.cmd[j].arg2 == -1)
				strcat(buf, "(  always ) in room ");
			else
				sprintf(buf, "%s(if< [%3d]) in room ", buf, zone.cmd[j].arg2);
			sprintf(buf, "%s[%5d].$R", buf, zone.cmd[j].arg3);
			if (ch && !str_cmp(ch->name, "Urizen"))
			{
				sprintf(buf, "%s [%d] [%d] %s", buf,
						zone.cmd[j].lastPop ? 1 : 0, charExists(zone.cmd[j].lastPop),
						charExists(zone.cmd[j].lastPop) ? GET_SHORT(zone.cmd[j].lastPop) : "Unknown");
			}
			sprintf(buf, "%s\r\n", buf);
			break;
		case 'O':
			virt = ZCMD.active ? obj_index[ZCMD.arg1].virt : ZCMD.arg1;
			sprintf(buf, "%s $BLoad obj  [%5d] ", buf, virt);
			if (zone.cmd[j].arg2 == -1)
				strcat(buf, "(  always ) in room ");
			else
				sprintf(buf, "%s(if< [%3d]) in room ", buf, zone.cmd[j].arg2);
			//      sprintf(buf, "%s[%5d].$R\r\n", buf,
			// world[zone.cmd[j].arg3].number);
			sprintf(buf, "%s[%5d].$R\r\n", buf, zone.cmd[j].arg3);
			break;
		case 'P':
			virt = ZCMD.active ? obj_index[ZCMD.arg1].virt : ZCMD.arg1;
			sprintf(buf, "%s $5Place obj [%5d] ", buf, virt);
			if (zone.cmd[j].arg2 == -1)
				strcat(buf, "(  always ) in objt ");
			else
				sprintf(buf, "%s(if< [%3d]) in objt ", buf, zone.cmd[j].arg2);
			virt = ZCMD.active ? obj_index[ZCMD.arg3].virt : ZCMD.arg3;
			sprintf(buf, "%s[%5d] (in last created).$R\r\n", buf, virt);
			break;
		case 'G':
			virt = ZCMD.active ? obj_index[ZCMD.arg1].virt : ZCMD.arg1;
			sprintf(buf, "%s $6Place obj [%5d] ", buf, virt);
			if (zone.cmd[j].arg2 == -1)
				strcat(buf, "(  always ) on last mob loaded.$R\r\n");
			else
				sprintf(buf, "%s(if< [%3d]) on last mob loaded.$R\r\n", buf, zone.cmd[j].arg2);
			break;
		case 'E':
			virt = ZCMD.active ? obj_index[ZCMD.arg1].virt : ZCMD.arg1;
			sprintf(buf, "%s $2Equip obj [%5d] ", buf, virt);
			if (zone.cmd[j].arg2 == -1)
				strcat(buf, "(  always ) on last mob on ");
			else
				sprintf(buf, "%s(if< [%3d]) on last mob on ", buf, zone.cmd[j].arg2);
			if (zone.cmd[j].arg3 > MAX_WEAR - 1 ||
				zone.cmd[j].arg3 < 0)
				sprintf(buf, "%s[%d](InvalidArg3).$R\r\n", buf, zone.cmd[j].arg3);
			else
				sprintf(buf, "%s[%d](%s).$R\r\n", buf, zone.cmd[j].arg3,
						equipment_types[zone.cmd[j].arg3]);
			break;
		case 'D':
			sprintf(buf, "%s $3Room [%5d] Dir: [%s]", buf,
					zone.cmd[j].arg1,
					dirNumToChar(zone.cmd[j].arg2));

			switch (zone.cmd[j].arg3)
			{
			case 0:
				strcat(buf, "Unlock/Open$R\r\n");
				break;
			case 1:
				strcat(buf, "Unlock/Close$R\r\n");
				break;
			case 2:
				strcat(buf, "Lock/Close$R\r\n");
				break;
			default:
				strcat(buf, "ERROR: Unknown$R\r\n");
				break;
			}
			break;
		case '%':
			sprintf(buf, "%s Consider myself true on %d times out of %d.\r\n", buf,
					zone.cmd[j].arg1,
					zone.cmd[j].arg2);

			break;
		case 'J':
			sprintf(buf, "%s Temp Command. [%d] [%d] [%d]\r\n", buf,
					zone.cmd[j].arg1,
					zone.cmd[j].arg2,
					zone.cmd[j].arg3);
			break;
		case '*':
			sprintf(buf, "%s %s\r\n", buf,
					zone.cmd[j].comment ? zone.cmd[j].comment : "Empty Comment");
			break;
		case 'K':
			sprintf(buf, "%s Skip next [%d] commands.\r\n", buf,
					zone.cmd[j].arg1);
			break;
		case 'X':
		{
			char xstrone[] = "Set all if-flags to 'unsure' state.";
			char xstrtwo[] = "Set mob if-flag to 'unsure' state.";
			char xstrthree[] = "Set obj if-flag to 'unsure' state.";
			char xstrfour[] = "Set %% if-flag to 'unsure' state.";
			char xstrerror[] = "Illegal value in arg1.";
			char *xresultstr;

			switch (zone.cmd[j].arg1)
			{
			case 0:
				xresultstr = xstrone;
				break;
			case 1:
				xresultstr = xstrtwo;
				break;
			case 2:
				xresultstr = xstrthree;
				break;
			case 3:
				xresultstr = xstrfour;
				break;
			default:
				xresultstr = xstrerror;
				break;
			}

			sprintf(buf, "%s [%d] %s\r\n", buf,
					zone.cmd[j].arg1, xresultstr);
		}
		break;
		default:
			sprintf(buf, "Illegal Command: %c %d %d %d %d\r\n",
					zone.cmd[j].command,
					zone.cmd[j].if_flag,
					zone.cmd[j].arg1,
					zone.cmd[j].arg2,
					zone.cmd[j].arg3);
			break;
		} // switch

		if (zone.cmd[j].comment && zone.cmd[j].command != '*')
			sprintf(buf, "%s       %s\r\n", buf, zone.cmd[j].comment);

		send_to_char(buf, ch);
		csendf(ch, "[%3d] Last attempt: $B%s$R Last success: $B%s$R Average: $B%.2f$R\r\n", j + 1, lastStr.c_str(), lastSuccessStr.c_str(), successRate * 100.0);
	} // for

	send_to_char("\r\nUse zedit to see the rest of the commands if they were truncated.\r\n", ch);
	return eSUCCESS;
}

int find_file(world_file_list_item *itm, int high)
{
	int i;
	world_file_list_item *tmp;
	for (i = 0, tmp = itm; tmp; tmp = tmp->next, i++)
		if (tmp->lastnum / 100 == high / 100)
			return i;
	return -1;
}

void show_legacy_files(Character *ch, world_file_list_item *head)
{
	world_file_list_item *curr = head;
	uint64_t i = 0;

	ch->send("ID ) Filename                       Begin  End\r\n"
			 "----------------------------------------------------------\r\n");

	while (curr != nullptr)
	{
		QString file_in_progress, file_ready, file_approved, file_modified;
		if (IS_SET(curr->flags, WORLD_FILE_IN_PROGRESS))
		{
			file_in_progress = "$B$1*$R";
		}

		if (IS_SET(curr->flags, WORLD_FILE_READY))
		{
			file_ready = "$B$5*$R";
		}

		if (IS_SET(curr->flags, WORLD_FILE_APPROVED))
		{
			file_approved = "$B$2*$R";
		}

		if (IS_SET(curr->flags, WORLD_FILE_MODIFIED))
		{
			file_modified = "MODIFIED";
		}

		ch->send(QString("%1) %2 %3 %4 %5%6%7 %8\r\n").arg(i++, 3).arg(curr->filename, -30).arg(curr->firstnum, -6).arg(curr->lastnum, -6).arg(file_in_progress, 1).arg(file_ready, 1).arg(file_approved, 1).arg(file_modified));
		curr = curr->next;
	}
}

int do_show(Character *ch, char *argument, int cmd)
{
	char name[MAX_INPUT_LENGTH], buf[200];
	char beginrange[MAX_INPUT_LENGTH];
	char endrange[MAX_INPUT_LENGTH];
	char type[MAX_INPUT_LENGTH];
	world_file_list_item *curr = nullptr;
	int i;
	int nr;
	int count = 0;
	int begin, end;

	//  half_chop(argument, type, name);
	argument = one_argument(argument, type);

	// argument = one_argument(argument,name);

	int has_range = has_skill(ch, COMMAND_RANGE);

	if (!*type)
	{
		send_to_char("Format: show <type> <name>.\r\n"
					 "Types:\r\n"
					 "  keydoorcombo\r\n"
					 "  mob\r\n"
					 "  obj\r\n"
					 "  room\r\n"
					 "  zone\r\n"
					 "  zone all\r\n",
					 ch);
		if (has_range)
			send_to_char("  rfiles\r\n"
						 "  mfiles\r\n"
						 "  ofiles\r\n"
						 "  search\r\n"
						 " msearch\r\n"
						 " rsearch\r\n"
						 " counts\r\n",
						 ch);
		return eFAILURE;
	}

	if (is_abbrev(type, "mobile"))
	{
		argument = one_argument(argument, name);
		if (!*name)
		{
			send_to_char("Format:  show mob <keyword>\r\n"
						 "                  <number>\r\n"
						 "                  <beginrange> <endrange>\r\n",
						 ch);
			return eFAILURE;
		}

		if (isdigit(*name))
		{
			//       half_chop(name, beginrange, endrange);
			strcpy(beginrange, name);
			//        beginrange = name;
			argument = one_argument(argument, endrange);
			if (!*endrange)
				strcpy(endrange, "-1");

			if (!check_range_valid_and_convert(begin, beginrange, 0, 100000) || !check_range_valid_and_convert(end, endrange, -1,
																											   100000))
			{
				send_to_char(
					"The begin and end ranges must be valid numbers.\r\n",
					ch);
				return eFAILURE;
			}
			if (end != -1 && end < begin)
			{ // swap um
				i = end;
				end = begin;
				begin = i;
			}

			*buf = '\0';
			send_to_char("[#  ] [MOB #] [LV] MOB'S DESCRIPTION\n\n\r", ch);

			if (end == -1)
			{
				if ((nr = real_mobile(begin)) >= 0)
				{
					sprintf(buf, "[  1] [%5d] [%2d] %s\n\r", begin,
							((Character *)(mob_index[nr].item))->level,
							((Character *)(mob_index[nr].item))->short_desc);
					send_to_char(buf, ch);
				}
			}
			else
			{
				for (i = begin; i <= mob_index[top_of_mobt].virt && i <= end;
					 i++)
				{
					if ((nr = real_mobile(i)) < 0)
						continue;

					count++;
					sprintf(buf, "[%3d] [%5d] [%2d] %s\n\r", count, i,
							((Character *)(mob_index[nr].item))->level,
							((Character *)(mob_index[nr].item))->short_desc);
					send_to_char(buf, ch);

					if (count > 200)
					{
						send_to_char(
							"Maximum number of searchable items hit.  Search ended.\r\n",
							ch);
						break;
					}
				}
			}
		}
		else
		{
			*buf = '\0';
			send_to_char("[#  ] [MOB #] [LV] MOB'S DESCRIPTION\n\n\r", ch);

			for (i = 0; (i <= mob_index[top_of_mobt].virt); i++)
			{
				if ((nr = real_mobile(i)) < 0)
					continue;

				if (isname(name,
						   ((Character *)(mob_index[nr].item))->name))
				{
					count++;
					sprintf(buf, "[%3d] [%5d] [%2d] %s\n\r", count, i,
							((Character *)(mob_index[nr].item))->level,
							((Character *)(mob_index[nr].item))->short_desc);
					send_to_char(buf, ch);

					if (count > 200)
					{
						send_to_char(
							"Maximum number of searchable items hit.  Search ended.\r\n",
							ch);
						break;
					}
				}
			}
		}
		if (!*buf)
			send_to_char("Couldn't find any MOBS by that NAME.\r\n", ch);
	} /* "mobile" */
	else if (is_abbrev(type, "counts") && has_range)
	{
		csendf(ch, "$3Rooms$R: %d\r\n$3Mobiles$R: %d\r\n$3Objects$R: %d\r\n",
			   total_rooms, top_of_mobt, top_of_objt);
		return eSUCCESS;
	}
	else if (is_abbrev(type, "object"))
	{
		argument = one_argument(argument, name);
		if (!*name)
		{
			send_to_char("Format:  show obj <keyword>\r\n"
						 "                  <number>\r\n"
						 "                  <beginrange> <endrange>\r\n",
						 ch);
			return eFAILURE;
		}

		if (isdigit(*name))
		{
			//      half_chop(name, beginrange, endrange);
			argument = one_argument(argument, endrange);
			// beginrange = name;
			strcpy(beginrange, name);
			if (!*endrange)
				strcpy(endrange, "-1");

			if (!check_range_valid_and_convert(begin, beginrange, 0, 100000) || !check_range_valid_and_convert(end, endrange, -1,
																											   100000))
			{
				send_to_char(
					"The begin and end ranges must be valid numbers.\r\n",
					ch);
				return eFAILURE;
			}
			if (end != -1 && end < begin)
			{ // swap um
				i = end;
				end = begin;
				begin = i;
			}

			*buf = '\0';
			send_to_char("[#  ] [OBJ #] [LV] OBJECT'S DESCRIPTION\n\n\r", ch);

			if (end == -1)
			{
				if ((nr = real_object(begin)) >= 0)
				{
					sprintf(buf, "[  1] [%5d] [%2d] %s\n\r", begin,
							((class Object *)(obj_index[nr].item))->obj_flags.eq_level,
							((class Object *)(obj_index[nr].item))->short_description);
					send_to_char(buf, ch);
				}
			}
			else
			{
				for (i = begin; i <= obj_index[top_of_objt].virt && i <= end;
					 i++)
				{
					if ((nr = real_object(i)) < 0)
						continue;

					count++;
					sprintf(buf, "[%3d] [%5d] [%2d] %s\n\r", count, i,
							((class Object *)(obj_index[nr].item))->obj_flags.eq_level,
							((class Object *)(obj_index[nr].item))->short_description);
					send_to_char(buf, ch);

					if (count > 200)
					{
						send_to_char(
							"Maximum number of searchable items hit.  Search ended.\r\n",
							ch);
						break;
					}
				}
			}
		}
		else
		{
			*buf = '\0';
			send_to_char("[#  ] [OBJ #] [LV] OBJECT'S DESCRIPTION\n\n\r", ch);

			for (i = 0; (i <= obj_index[top_of_objt].virt); i++)
			{
				if ((nr = real_object(i)) < 0)
					continue;

				if (isname(name,
						   ((class Object *)(obj_index[nr].item))->name))
				{
					count++;
					sprintf(buf, "[%3d] [%5d] [%2d] %s\n\r", count, i,
							((class Object *)(obj_index[nr].item))->obj_flags.eq_level,
							((class Object *)(obj_index[nr].item))->short_description);
					send_to_char(buf, ch);
				}

				if (count > 200)
				{
					send_to_char(
						"Maximum number of searchable items hit.  Search ended.\r\n",
						ch);
					break;
				}
			}
		}
		if (!*buf)
			send_to_char("Couldn't find any OBJECTS by that NAME.\r\n", ch);
	} /* "object" */
	else if (is_abbrev(type, "room"))
	{
		argument = one_argument(argument, name);
		if (!*name)
		{
			send_to_char("Format:  show room <beginrange> <endrange>\r\n", ch);
			return eFAILURE;
		}

		if (isdigit(*name))
		{
			//      half_chop(name, beginrange, endrange);
			argument = one_argument(argument, endrange);
			//	beginrange = name;
			strcpy(beginrange, name);
			if (!*endrange)
				strcpy(endrange, "-1");

			if (!check_range_valid_and_convert(begin, beginrange, 0, 100000) || !check_range_valid_and_convert(end, endrange, -1,
																											   100000))
			{
				send_to_char(
					"The begin and end ranges must be valid numbers.\r\n",
					ch);
				return eFAILURE;
			}
			if (end != -1 && end < begin)
			{ // swap um
				i = end;
				end = begin;
				begin = i;
			}

			*buf = '\0';
			send_to_char("[#  ] [ROOM#] ROOM'S NAME\n\n\r", ch);

			if (end == -1)
			{
				if (world_array[begin])
				{
					sprintf(buf, "[  1] [%5d] %s\n\r", begin,
							world[begin].name);
					send_to_char(buf, ch);
				}
			}
			else
			{
				for (i = begin; i < top_of_world && i <= end; i++)
				{
					if (!world_array[i])
						continue;
					count++;
					sprintf(buf, "[%3d] [%5d] %s\n\r", count, i, world[i].name);
					send_to_char(buf, ch);

					if (count > 200)
					{
						send_to_char(
							"Maximum number of searchable items hit.  Search ended.\r\n",
							ch);
						break;
					}
				}
			}
		}
		if (!*buf)
			send_to_char("Couldn't find any ROOMS in that range.\r\n", ch);
	} /* "object" */
	else if (is_abbrev(type, "zone"))
	{
		argument = one_argument(argument, name);
		if (!*name)
		{
			send_to_char("Show which zone? (# or 'all')\r\n", ch);
			return eFAILURE;
		}

		if (!isdigit(*name)) /* show them all */
		{
			send_to_char(
				"     Range        Usage\r\n"
				"Num  Start-End    Start-End    Rooms   Name\r\n"
				//	 174  31900-32100  32001-32079   78   the Battle of Troy
				"---  -----------  -----------  -----  ----------------------\r\n",
				ch);
			for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
			{
				room_t range_start = zone.getBottom();
				room_t range_end = zone.getTop();
				int num = count_rooms(range_start, range_end);

				ch->send(QString("%1  %2-%3  $0$B%4-%5  %6$R  %7$R\r\n").arg(zone_key, 3).arg(zone.getBottom(), 5).arg(zone.getTop(), -5).arg(zone.getRealBottom(), 5).arg(zone.getRealTop(), -5).arg(num, 5).arg(zone.name));
			}
		}
		else /* was a digit */
		{
			i = atoi(name);
			if ((!i && *name != '0') || i < 0 || DC::getInstance()->zones.contains(i) == false)
			{
				send_to_char("Which zone was that?\r\n", ch);
				return eFAILURE;
			}
			DC::getInstance()->zones.value(i).show_info(ch);

		} // else was a digit
	}	  // zone
	else if (is_abbrev(type, "rsearch") && has_range)
	{
		char arg1[MAX_INPUT_LENGTH];
		int zon, bits = 0, sector = 0;
		argument = one_argument(argument, arg1);
		if (!is_number(arg1))
		{
			send_to_char(
				"Syntax: show rsearch <zone#> <sectorname/roomflag>\r\n",
				ch);
			return eSUCCESS;
		}
		zon = atoi(arg1);
		//     Room
		//   zone
		//   sector_type
		// room_flags
		while ((argument = one_argument(argument, arg1)) != nullptr)
		{
			if (arg1[0] == '\0')
				break;
			int i;
			bool found = false;
			for (i = 0; room_bits[i][0] != '\n'; i++)
				if (!str_cmp(arg1, room_bits[i]))
				{
					SET_BIT(bits, 1 << i);
					found = true;
					break;
				}
			for (i = 0; sector_types[i][0] != '\n'; i++)
				if (!str_cmp(arg1, sector_types[i]))
				{
					sector = i - 1;
					found = true;
					break;
				}

			if (!found)
				send_to_char("Unknown room-flag or sector type.\r\n", ch);
		}
		if (!bits && !sector)
		{
			send_to_char(
				"Syntax: show rsearch <zone number> <flags/sector type\r\n",
				ch);
			return eSUCCESS;
		}
		room_t last_room = DC::getInstance()->zones.lastKey();
		if (zon > last_room)
		{

			ch->send(QString("Unknown zone. Zone %1 is greater than last valid zone %2.\r\n").arg(zon).arg(last_room));
			return eFAILURE;
		}
		char buf[MAX_INPUT_LENGTH];
		for (i = DC::getInstance()->zones.value(zon).getRealBottom(); i < DC::getInstance()->zones.value(zon).getRealTop();
			 i++)
		{
			if (!world_array[i])
				continue;
			if (bits)
				if (!IS_SET(world[i].room_flags, bits))
					continue;
			if (sector)
				if (world[i].sector_type != sector)
					continue;
			sprintf(buf, "[%3d] %s\r\n", i, world[i].name);
			send_to_char(buf, ch);
		}
	}
	else if (is_abbrev(type, "msearch") && has_range)
	{ // Mobile search.
		char arg1[MAX_STRING_LENGTH];
		uint32_t affect[AFF_MAX / ASIZE + 1] = {};
		uint32_t act[ACT_MAX / ASIZE + 1] = {};
		int clas = 0, levlow = -555, levhigh = -555, immune = 0, race = -1,
			align = 0;
		// int its;
		//    if (
		bool fo = false;
		while ((argument = one_argument(argument, arg1)))
		{
			int i;
			if (strlen(arg1) < 2)
				break;
			fo = true;
			for (i = 0; *pc_clss_types2[i] != '\n'; i++)
				if (!str_cmp(pc_clss_types2[i], arg1))
				{
					clas = i;
					goto thisLoop;
				}
			for (i = 0; *isr_bits[i] != '\n'; i++)
				if (!str_nosp_cmp(isr_bits[i], arg1))
				{
					SET_BIT(immune, 1 << i);
					goto thisLoop;
				}
			for (i = 0; *action_bits[i] != '\n'; i++)
				if (!str_nosp_cmp(action_bits[i], arg1))
				{
					SETBIT(act, i);
					goto thisLoop;
				}
			for (i = 0; *affected_bits[i] != '\n'; i++)
				if (!str_nosp_cmp(affected_bits[i], arg1))
				{
					SETBIT(affect, i);
					goto thisLoop;
				}
			for (i = 0; i <= MAX_RACE; i++)
				if (!str_nosp_cmp(races[i].singular_name, arg1))
				{
					race = i;
					goto thisLoop;
				}
			if (!str_cmp(arg1, "evil"))
				align = 3;
			else if (!str_cmp(arg1, "good"))
				align = 1;
			else if (!str_cmp(arg1, "neutral"))
				align = 2;
			if (!str_cmp(arg1, "level"))
			{
				argument = one_argument(argument, arg1);
				if (is_number(arg1))
					levlow = atoi(arg1);
				argument = one_argument(argument, arg1);
				if (is_number(arg1))
					levhigh = atoi(arg1);
				if (levhigh == -555 || levlow == -555)
				{
					send_to_char("Incorrect level requirement.\r\n", ch);
					return eFAILURE;
				}
			}
		thisLoop:
			continue;
		}
		if (!fo)
		{
			int z, o = 0;
			for (z = 0; *action_bits[z] != '\n'; z++)
			{
				o++;
				send_to_char_nosp(action_bits[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			for (z = 0; *isr_bits[z] != '\n'; z++)
			{
				o++;
				send_to_char_nosp(isr_bits[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			for (z = 0; *affected_bits[z] != '\n'; z++)
			{
				o++;
				send_to_char_nosp(affected_bits[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			for (z = 0; *pc_clss_types2[z] != '\n'; z++)
			{
				o++;
				send_to_char(pc_clss_types2[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			for (i = 0; i <= MAX_RACE; i++)
			{
				o++;
				send_to_char_nosp(races[i].singular_name, ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			send_to_char("level", ch);
			if (o % 7 == 0)
				send_to_char("\r\n", ch);
			else
				send_to_char(" ", ch);

			send_to_char("good", ch);
			if (o % 7 == 0)
				send_to_char("\r\n", ch);
			else
				send_to_char(" ", ch);
			send_to_char("evil", ch);
			if (o % 7 == 0)
				send_to_char("\r\n", ch);
			else
				send_to_char(" ", ch);
			send_to_char("neutral", ch);
			if (o % 7 == 0)
				send_to_char("\r\n", ch);
			else
				send_to_char(" ", ch);

			return eSUCCESS;
		}
		int c, nr;
		if (!*act && !clas && !levlow && !levhigh && !*affect && !immune && !race && !align)
		{
			send_to_char("No valid search supplied.\r\n", ch);
			return eFAILURE;
		}
		for (c = 0; c < mob_index[top_of_mobt].virt; c++)
		{
			if ((nr = real_mobile(c)) < 0)
				continue;
			if (race > -1)
				if (((Character *)(mob_index[nr].item))->race != race)
					continue;
			if (align)
			{
				if (align == 1 && ((Character *)(mob_index[nr].item))->alignment < 350)
					continue;
				else if (align == 2 && (((Character *)(mob_index[nr].item))->alignment < -350 || ((Character *)(mob_index[nr].item))->alignment > 350))
					continue;
				else if (align == 3 && ((Character *)(mob_index[nr].item))->alignment > -350)
					continue;
			}
			if (immune)
				if (!IS_SET(((Character *)(mob_index[nr].item))->immune,
							immune))
					continue;
			if (clas)
				if (((Character *)(mob_index[nr].item))->c_class != clas)
					continue;
			if (levlow != -555)
				if (((Character *)(mob_index[nr].item))->level < levlow)
					continue;
			if (levhigh != -555)
				if (((Character *)(mob_index[nr].item))->level > levhigh)
					continue;
			if (*act)
				for (i = 0; i < ACT_MAX; i++)
					if (ISSET(act, i))
						if (!ISSET(
								((Character *)(mob_index[nr].item))->mobdata->actflags,
								i + 1))
							goto eheh;
			if (*affect)
				for (i = 0; i < AFF_MAX; i++)
					if (ISSET(affect, i))
						if (!ISSET(
								((Character *)(mob_index[nr].item))->affected_by,
								i + 1))
							goto eheh;
			count++;
			if (count > 200)
			{
				send_to_char("Limit reached.\r\n", ch);
				break;
			}
			sprintf(buf, "[%3d] [%5d] [%2d] %s\n\r", count, c,
					((Character *)(mob_index[nr].item))->level,
					((Character *)(mob_index[nr].item))->short_desc);
			send_to_char(buf, ch);
		eheh:
			continue;
		}
	}
	else if (is_abbrev(type, "search"))
	{ // Object search.
		char arg1[MAX_STRING_LENGTH];
		int affect = 0, size = 0, extra = 0, more = 0, wear = 0, type = 0;
		int levlow = -555, levhigh = -555, dam = 0, lweight = -555, hweight = -555;
		int any = 0;
		bool fo = false;
		int item_type = 0;
		int its = 0;
		int spellnum = -1;
		while ((argument = one_argument(argument, arg1)))
		{
			int i;
			if (strlen(arg1) < 2)
				break;
			fo = true;

			if (!str_nosp_cmp("wand", arg1))
			{
				item_type = ITEM_WAND;
				goto endy;
			}
			if (!str_nosp_cmp("staff", arg1))
			{
				item_type = ITEM_STAFF;
				goto endy;
			}
			if (!str_nosp_cmp("scroll", arg1))
			{
				item_type = ITEM_SCROLL;
				goto endy;
			}
			if (!str_nosp_cmp("potion", arg1))
			{
				item_type = ITEM_POTION;
				goto endy;
			}

			for (i = 0; i < Object::wear_bits.size(); i++)
				if (!str_nosp_cmp(Object::wear_bits[i], arg1))
				{
					SET_BIT(wear, 1 << i);
					goto endy;
				}
			for (i = 0; i < item_types.size(); i++)
			{
				if (!str_nosp_cmp(item_types[i], arg1))
				{
					type = i;
					goto endy;
				}
			}
			for (i = 0; *strs_damage_types[i] != '\n'; i++)
				if (!str_nosp_cmp(strs_damage_types[i], arg1))
				{
					dam = i;
					goto endy;
				}
			for (i = 0; i < Object::extra_bits.size(); i++)
				if (!str_nosp_cmp(Object::extra_bits[i].toStdString().c_str(), arg1))
				{
					if (!str_cmp(Object::extra_bits[i].toStdString().c_str(), "ANY_CLASS"))
						any = i;
					else
						SET_BIT(extra, 1 << i);
					goto endy;
				}
			for (i = 0; Object::more_obj_bits[i] != '\n'; i++)
				if (!str_nosp_cmp(Object::more_obj_bits[i], arg1))
				{
					SET_BIT(more, 1 << i);
					goto endy;
				}
			for (i = 0; *size_bitfields[i] != '\n'; i++)
				if (!str_nosp_cmp(size_bitfields[i], arg1))
				{
					SET_BIT(size, 1 << i);
					goto endy;
				}

			if (!item_type)
			{
				for (i = 0; *apply_types[i] != '\n'; i++)
					if (!str_nosp_cmp(apply_types[i], arg1))
					{
						affect = i;
						goto endy;
					}
			}
			else
			{
				for (i = 0; *spells[i] != '\n'; i++)
					if (!str_nosp_cmp(spells[i], arg1))
					{
						spellnum = i + 1;
						goto endy;
					}
			}

			if (!str_cmp(arg1, "olevel"))
			{
				argument = one_argument(argument, arg1);
				if (is_number(arg1))
					levlow = atoi(arg1);

				argument = one_argument(argument, arg1);
				if (is_number(arg1))
					levhigh = atoi(arg1);

				if (levhigh == -555 || levlow == -555)
				{
					send_to_char("Incorrect level requirement.\r\n", ch);
					return eFAILURE;
				}
			}
			if (!str_cmp(arg1, "oweight"))
			{
				argument = one_argument(argument, arg1);
				if (is_number(arg1))
					lweight = atoi(arg1);

				argument = one_argument(argument, arg1);
				if (is_number(arg1))
					hweight = atoi(arg1);

				if (lweight == -555 || hweight == -555)
				{
					send_to_char("Incorrect weight requirement.\r\n", ch);
					return eFAILURE;
				}
			}
			csendf(ch, "Unknown type: %s.\r\n", arg1);
		endy:
			continue;
		}
		int c, nr, aff;
		//     csendf(ch,"%d %d %d %d %d", more, extra, wear, size, affect);
		bool found = false;
		int o = 0, z;
		if (!fo)
		{
			for (z = 0; i < Object::wear_bits.size(); z++)
			{
				o++;
				send_to_char_nosp(Object::wear_bits[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			for (z = 0; z < Object::extra_bits.size(); z++)
			{
				o++;
				send_to_char_nosp(Object::extra_bits[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			for (z = 0; *strs_damage_types[z] != '\n'; z++)
			{
				o++;
				send_to_char_nosp(strs_damage_types[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}

			for (z = 0; Object::more_obj_bits[z] != '\n'; z++)
			{
				o++;
				send_to_char_nosp(Object::more_obj_bits[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			for (z = 0; z < item_types.size(); z++)
			{
				o++;
				send_to_char_nosp(item_types[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			for (z = 0; *size_bitfields[z] != '\n'; z++)
			{
				o++;
				send_to_char_nosp(size_bitfields[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			for (z = 0; *apply_types[z] != '\n'; z++)
			{
				o++;
				send_to_char_nosp(apply_types[z], ch);
				if (o % 7 == 0)
					send_to_char("\r\n", ch);
				else
					send_to_char(" ", ch);
			}
			send_to_char("oweight", ch);
			if (o % 7 == 0)
				send_to_char("\r\n", ch);
			else
				send_to_char(" ", ch);

			send_to_char("olevel", ch);
			if (o % 7 == 0)
				send_to_char("\r\n", ch);
			else
				send_to_char(" ", ch);

			return eSUCCESS;
		}

		for (c = 0; c < obj_index[top_of_objt].virt; c++)
		{
			found = false;
			if ((nr = real_object(c)) < 0)
				continue;
			if (wear)
				for (i = 0; i < 20; i++)
					if (IS_SET(wear, 1 << i))
						if (!IS_SET(
								((class Object *)(obj_index[nr].item))->obj_flags.wear_flags,
								1 << i))
							goto endLoop;
			if (type)
				if (((class Object *)(obj_index[nr].item))->obj_flags.type_flag != type)
					continue;
			if (lweight != -555)
				if (((class Object *)(obj_index[nr].item))->obj_flags.weight < lweight)
					continue;
			if (hweight != -555)
				if (((class Object *)(obj_index[nr].item))->obj_flags.weight > hweight)
					continue;

			if (levhigh != -555)
				if (((class Object *)(obj_index[nr].item))->obj_flags.eq_level > levhigh)
					continue;
			if (levlow != -555)
				if (((class Object *)(obj_index[nr].item))->obj_flags.eq_level < levlow)
					continue;
			if (size)
				for (i = 0; i < 10; i++)
					if (IS_SET(size, 1 << i))
						if (!IS_SET(
								((class Object *)(obj_index[nr].item))->obj_flags.size,
								1 << i))
							goto endLoop;
			if (((class Object *)(obj_index[nr].item))->obj_flags.type_flag == ITEM_WEAPON)
			{
				int get_weapon_damage_type(class Object * wielded);
				its = get_weapon_damage_type(
					((class Object *)(obj_index[nr].item)));
			}
			if (dam && dam != (its - 1000))
				continue;
			if (extra)
				for (i = 0; i < 30; i++)
					if (IS_SET(extra, 1 << i))
						if (!IS_SET(
								((class Object *)(obj_index[nr].item))->obj_flags.extra_flags,
								1 << i) &&
							!(any && IS_SET(
										 ((class Object *)(obj_index[nr].item))->obj_flags.extra_flags,
										 1 << any)))
							goto endLoop;

			if (more)
				for (i = 0; i < 10; i++)
					if (IS_SET(more, 1 << i))
						if (!IS_SET(
								((class Object *)(obj_index[nr].item))->obj_flags.more_flags,
								1 << i))
							goto endLoop;
			//      int aff,total = 0;
			//    bool found = false;
			if (!item_type)
				for (aff = 0;
					 aff < ((class Object *)(obj_index[nr].item))->num_affects;
					 aff++)
					if (affect == ((class Object *)(obj_index[nr].item))->affected[aff].location)
						found = true;
			if (affect && !item_type)
				if (!found)
					continue;

			if (item_type)
			{
				bool spell_found = false;
				if (((class Object *)(obj_index[nr].item))->obj_flags.type_flag != item_type)
					continue;
				if (item_type == ITEM_POTION || item_type == ITEM_SCROLL)
					for (i = 1; i < 4; i++)
						if (((class Object *)(obj_index[nr].item))->obj_flags.value[i] == spellnum)
							spell_found = true;
				if (item_type == ITEM_STAFF || item_type == ITEM_WAND)
					if (((class Object *)(obj_index[nr].item))->obj_flags.value[3] == spellnum)
						spell_found = true;

				if (!spell_found)
					continue;
			}

			count++;
			if (count > 200)
			{
				send_to_char("Limit reached.\r\n", ch);
				break;
			}
			sprintf(buf, "[%3d] [%5d] [%2d] %s\n\r", count, c,
					((class Object *)(obj_index[nr].item))->obj_flags.eq_level,
					((class Object *)(obj_index[nr].item))->short_description);
			send_to_char(buf, ch);
		endLoop:
			continue;
		}
	}
	else if (is_abbrev(type, "rfiles") && has_range)
	{
		show_legacy_files(ch, world_file_list);
	}
	else if (is_abbrev(type, "mfiles") && has_range)
	{
		show_legacy_files(ch, mob_file_list);
	}
	else if (is_abbrev(type, "ofiles") && has_range)
	{
		show_legacy_files(ch, obj_file_list);
	}
	else if (is_abbrev(type, "keydoorcombo"))
	{
		if (!*name)
		{
			send_to_char("Show which key? (# of key)\r\n", ch);
			return eFAILURE;
		}

		count = atoi(name);
		if ((!count && *name != '0') || count < 0)
		{
			send_to_char("Which key was that?\r\n", ch);
			return eFAILURE;
		}

		csendf(ch, "$3Doors in game that use key %d$R:\r\n\r\n", count);
		for (i = 0; i < top_of_world; i++)
			for (nr = 0; nr < MAX_DIRS; nr++)
				if (world_array[i] && world_array[i]->dir_option[nr])
				{
					if (IS_SET(world_array[i]->dir_option[nr]->exit_info,
							   EX_ISDOOR) &&
						world_array[i]->dir_option[nr]->key == count)
					{
						csendf(ch,
							   " $3Room$R: %5d $3Dir$R: %5s $3Key$R: %d\r\n",
							   world_array[i]->number, dirs[nr],
							   world_array[i]->dir_option[nr]->key);
					}
				}
	}
	else
		send_to_char("Illegal type.  Type just 'show' for legal types.\r\n",
					 ch);
	return eSUCCESS;
}

command_return_t do_transfer(Character *ch, string arguments, int cmd)
{
	if (IS_NPC(ch) || ch == nullptr)
	{
		return eFAILURE;
	}

	room_t destination_room = ch->in_room;
	if (destination_room == 0)
	{
		return eFAILURE;
	}

	string arg1;
	tie(arg1, arguments) = half_chop(arguments);
	if (arg1.empty())
	{
		ch->send("Usage: transfer <name>\r\n");
		ch->send("       transfer all\r\n");
		return eFAILURE;
	}

	Character *victim = nullptr;
	room_t source_room = {};
	Connection *i = nullptr;
	if (arg1 == "all")
	{
		for (i = DC::getInstance()->descriptor_list; i; i = i->next)
		{
			victim = i->character;
			source_room = victim->in_room;
			if (victim != ch && i->connected == Connection::states::PLAYING && source_room != 0)
			{
				act("$n disappears in a mushroom cloud.", victim, 0, 0, TO_ROOM, 0);
				ch->send(fmt::format("Moving {} from {} to {}.\r\n", GET_NAME(victim), world[source_room].number, world[destination_room].number));
				move_char(victim, destination_room);
				act("$n arrives from a puff of smoke.", victim, 0, 0, TO_ROOM, 0);
				act("$n has transferred you!", ch, 0, victim, TO_VICT, 0);
				do_look(victim, "", 15);
			}
		}

		send_to_char("Ok.\r\n", ch);
		return eSUCCESS;
	}

	victim = get_char_vis(ch, arg1);
	if (victim == nullptr)
	{
		ch->send("No-one by that name around.\r\n");
		return eFAILURE;
	}
	source_room = victim->in_room;

	if (world[destination_room].number == IMM_PIRAHNA_ROOM && !isname(GET_NAME(ch), "Pirahna"))
	{
		send_to_char("Damn! That is rude! This ain't your place. :P\r\n", ch);
		return eFAILURE;
	}

	act("$n disappears in a mushroom cloud.", victim, 0, 0, TO_ROOM, 0);
	ch->send(fmt::format("Moving {} from {} to {}.\r\n", GET_NAME(victim), world[source_room].number, world[destination_room].number));
	move_char(victim, destination_room);
	act("$n arrives from a puff of smoke.", victim, 0, 0, TO_ROOM, 0);
	act("$n has transferred you!", ch, 0, victim, TO_VICT, 0);
	do_look(victim, "", 15);
	send_to_char("Ok.\r\n", ch);

	return eSUCCESS;
}

int do_teleport(Character *ch, char *argument, int cmd)
{
	Character *victim, *target_mob, *pers;
	char person[MAX_INPUT_LENGTH], room[MAX_INPUT_LENGTH];
	int target;
	int loop;

	if (IS_NPC(ch))
		return eFAILURE;

	half_chop(argument, person, room);

	if (!*person)
	{
		send_to_char("Who do you wish to teleport?\n\r", ch);
		return eFAILURE;
	} /* if */

	if (!*room)
	{
		send_to_char("Where do you wish to send ths person?\n\r", ch);
		return eFAILURE;
	} /* if */

	if (!(victim = get_char_vis(ch, person)))
	{
		send_to_char("No-one by that name around.\r\n", ch);
		return eFAILURE;
	} /* if */

	if (isdigit(*room))
	{
		target = atoi(&room[0]);
		if ((*room != '0' && target == 0) || !world_array[target])
		{
			send_to_char("No room exists with that number.\r\n", ch);
			return eFAILURE;
		}
		//      for (loop = 0; loop <= top_of_world; loop++) {
		//         if (world[loop].number == target) {
		//            target = (int16_t)loop;
		//            break;
		//      } else if (loop == top_of_world) {
		//            send_to_char("No room exists with that number.\r\n", ch);
		//            return eFAILURE;
		//      } /* if */
		//       } /* for */
	}
	else if ((target_mob = get_char_vis(ch, room)) != nullptr)
	{
		target = target_mob->in_room;
	}
	else
	{
		send_to_char("No such target (person) can be found.\r\n", ch);
		return eFAILURE;
	} /* if */

	if (IS_SET(world[target].room_flags, PRIVATE))
	{
		for (loop = 0, pers = world[target].people; pers;
			 pers = pers->next_in_room, loop++)
			;
		if (loop > 1)
		{
			send_to_char(
				"There's a private conversation going on in that room\n\r",
				ch);
			return eFAILURE;
		} /* if */
	}	  /* if */

	if (IS_SET(world[target].room_flags, IMP_ONLY) && GET_LEVEL(ch) < IMPLEMENTER)
	{
		send_to_char("No.\r\n", ch);
		return eFAILURE;
	}

	if (IS_SET(world[target].room_flags, CLAN_ROOM) &&
		GET_LEVEL(ch) < DEITY)
	{
		send_to_char("No.\r\n", ch);
		return eFAILURE;
	}

	act("$n disappears in a puff of smoke.", victim, 0, 0, TO_ROOM, 0);
	csendf(ch, "Moving %s from %d to %d.\r\n", GET_NAME(victim),
		   world[victim->in_room].number, world[target].number);
	move_char(victim, target);
	act("$n arrives from a puff of smoke.", victim, 0, 0, TO_ROOM, 0);
	act("$n has teleported you!", ch, 0, (char *)victim, TO_VICT, 0);
	do_look(victim, "", 15);
	send_to_char("Teleport completed.\r\n", ch);

	return eSUCCESS;
} /* do_teleport */

int do_gtrans(Character *ch, char *argument, int cmd)
{
	// class Connection *i;
	Character *victim;
	char buf[100];
	int target;
	struct follow_type *k, *next_dude;

	if (IS_NPC(ch))
		return eFAILURE;

	one_argument(argument, buf);
	if (!*buf)
	{
		send_to_char("Whom is the group leader you wish to transfer?\n\r", ch);
		return eFAILURE;
	}

	if (!(victim = get_char_vis(ch, buf)))
	{
		send_to_char("No-one by that name around.\r\n", ch);
		return eFAILURE;
	}
	else
	{
		act("$n disappears in a mushroom cloud.",
			victim, 0, 0, TO_ROOM, 0);
		target = ch->in_room;
		csendf(ch, "Moving %s from %d to %d.\r\n", GET_NAME(victim),
			   world[victim->in_room].number, world[target].number);
		move_char(victim, target);
		act("$n arrives from a puff of smoke.",
			victim, 0, 0, TO_ROOM, 0);
		act("$n has transferred you!", ch, 0, victim, TO_VICT, 0);
		do_look(victim, "", 15);

		if (victim->followers)
			for (k = victim->followers; k; k = next_dude)
			{
				next_dude = k->next;
				if (IS_PC(k->follower) && IS_AFFECTED(k->follower, AFF_GROUP))
				{
					act("$n disappears in a mushroom cloud.",
						victim, 0, 0, TO_ROOM, 0);
					target = ch->in_room;
					csendf(ch, "Moving %s from %d to %d.\r\n", GET_NAME(k->follower),
						   world[k->follower->in_room].number, world[target].number);
					move_char(k->follower, target);
					act("$n arrives from a puff of smoke.",
						k->follower, 0, 0, TO_ROOM, 0);
					act("$n has transferred you!", ch, 0, k->follower, TO_VICT, 0);
					do_look(k->follower, "", 15);
				}
			} /* for */
		send_to_char("Ok.\r\n", ch);
	} /* else */
	return eSUCCESS;
}

char *oprog_type_to_name(int type)
{
	switch (type)
	{
	case ALL_GREET_PROG:
		return "all_greet_prog";
	case WEAPON_PROG:
		return "weapon_prog";
	case ARMOUR_PROG:
		return "armour_prog";
	case LOAD_PROG:
		return "load_prog";
	case COMMAND_PROG:
		return "command_prog";
	case ACT_PROG:
		return "act_prog";
	case ARAND_PROG:
		return "arand_prog";
	case CATCH_PROG:
		return "catch_prog";
	case SPEECH_PROG:
		return "speech_prog";
	case RAND_PROG:
		return "rand_prog";
	case CAN_SEE_PROG:
		return "can_see_prog";
	default:
		return "ERROR_PROG";
	}
}

void opstat(Character *ch, int vnum)
{
	int num = real_object(vnum);
	Object *obj;
	char buf[MAX_STRING_LENGTH];
	if (num < 0)
	{
		send_to_char("Error, non-existant object.\r\n", ch);
		return;
	}
	obj = (Object *)obj_index[num].item;
	sprintf(buf, "$3Object$R: %s   $3Vnum$R: %d.\r\n",
			obj->name, vnum);
	send_to_char(buf, ch);
	if (obj_index[num].progtypes == 0)
	{
		send_to_char("This object has no special procedures.\r\n", ch);
		return;
	}
	send_to_char("\r\n", ch);
	mob_prog_data *mprg;
	int i;
	char buf2[MAX_STRING_LENGTH];
	for (mprg = obj_index[num].mobprogs, i = 1; mprg != nullptr;
		 i++, mprg = mprg->next)
	{
		sprintf(buf, "$3%d$R>$3$B", i);
		send_to_char(buf, ch);
		send_to_char(oprog_type_to_name(mprg->type), ch);
		send_to_char("$R ", ch);
		sprintf(buf, "$B$5%s$R\n\r", mprg->arglist);
		send_to_char(buf, ch);
		sprintf(buf, "%s\n\r", mprg->comlist);
		double_dollars(buf2, buf);
		send_to_char(buf2, ch);
	}
}

int do_opstat(Character *ch, char *argument, int cmd)
{
	char buf[MAX_STRING_LENGTH];
	int vnum = -1;

	if (!*argument)
	{
		send_to_char("Usage: opstat <vnum>\r\n ", ch);
		return eFAILURE;
	}
	one_argument(argument, buf);

	if (!has_skill(ch, COMMAND_OPSTAT))
	{
		send_to_char("Huh?\r\n", ch);
		return eFAILURE;
	}
	if (isdigit(*buf))
	{
		vnum = atoi(argument);
	}
	else
	{
		vnum = ch->player->last_obj_vnum;
	}

	opstat(ch, vnum);
	return eSUCCESS;
}

void update_objprog_bits(int num)
{
	mob_prog_data *prog = obj_index[num].mobprogs;
	obj_index[num].progtypes = 0;

	while (prog)
	{
		SET_BIT(obj_index[num].progtypes, prog->type);
		prog = prog->next;
	}
}

int do_opedit(Character *ch, char *argument, int cmd)
{
	int num = -1, vnum = -1, i = -1, a = -1;
	char arg[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg);
	if (IS_NPC(ch))
		return eFAILURE;
	if (isdigit(*arg))
	{
		vnum = atoi(arg);
		argument = one_argument(argument, arg);
	}
	else
	{
		vnum = ch->player->last_obj_vnum;
	}

	if ((num = real_object(vnum)) < 0)
	{
		send_to_char("No such object.\r\n", ch);
		return eFAILURE;
	}
	if (!can_modify_object(ch, vnum))
	{
		send_to_char("You are unable to work creation outside your range.\r\n", ch);
		return eFAILURE;
	}
	ch->player->last_obj_vnum = vnum;
	/*  if (!*arg)
	  {
		opstat(ch, vnum);
		return eSUCCESS;
	  }*/
	mob_prog_data *prog, *currprog;
	if (!str_cmp(arg, "add"))
	{
		argument = one_argument(argument, arg);
		if (!*arg)
		{
			send_to_char("$3Syntax$R: opedit [num] add new\r\n"
						 "This creates a new object proc.\r\n",
						 ch);
			return eFAILURE;
		}
#ifdef LEAK_CHECK
		prog = (mob_prog_data *)calloc(1, sizeof(mob_prog_data));
#else
		prog = (mob_prog_data *)dc_alloc(1, sizeof(mob_prog_data));
#endif
		prog->type = ALL_GREET_PROG;
		prog->arglist = strdup("80");
		prog->comlist = strdup("say This is my new obj prog!\n\r");
		prog->next = nullptr;

		if ((currprog = obj_index[num].mobprogs))
		{
			while (currprog->next)
				currprog = currprog->next;
			currprog->next = prog;
		}
		else
			obj_index[num].mobprogs = prog;
		update_objprog_bits(num);
		send_to_char("New obj proc created.\r\n", ch);
		return eSUCCESS;
	}
	else if (!str_cmp(arg, "remove"))
	{
		argument = one_argument(argument, arg);
		int a = -1;
		if (!*arg || !isdigit(*arg))
		{
			send_to_char("$3Syntax$R: opedit [obj_num] remove <prog>\r\n"
						 "This removes an object procedure completly\r\n",
						 ch);
			return eFAILURE;
		}
		a = atoi(arg);
		prog = nullptr;
		for (i = 1, currprog = obj_index[num].mobprogs;
			 currprog && i != a;
			 i++, prog = currprog, currprog = currprog->next)
			;
		if (!currprog)
		{
			send_to_char("Invalid proc number.\r\n", ch);
			return eFAILURE;
		}
		if (prog)
			prog->next = currprog->next;
		else
			obj_index[num].mobprogs = currprog->next;

		currprog->type = 0;
		dc_free(currprog->arglist);
		dc_free(currprog->comlist);
		dc_free(currprog);

		update_objprog_bits(num);

		send_to_char("Program deleted.\r\n", ch);
		return eSUCCESS;
	}
	else if (!str_cmp(arg, "type"))
	{
		argument = one_argument(argument, arg);
		if (!*arg || !argument || !*argument || !isdigit(*arg) || !isdigit(*(1 + argument)))
		{
			send_to_char("$3Syntax$R: opedit [obj_num] type <prog> <type>\r\n", ch);
			send_to_char("$3Valid types are$R:\r\n", ch);
			char buf[MAX_STRING_LENGTH];
			for (i = 0; *obj_types[i] != '\n'; i++)
			{
				sprintf(buf, " %2d - %15s\r\n",
						i + 1, obj_types[i]);
				send_to_char(buf, ch);
			}
			return eFAILURE;
		}
		int a = atoi(arg);
		for (i = 1, currprog = obj_index[num].mobprogs;
			 currprog && i != a;
			 i++, currprog = currprog->next)
			;

		if (!currprog)
		{
			send_to_char("Invalid prog number.\r\n", ch);
			return eFAILURE;
		}
		switch (atoi(argument + 1))
		{
		case 1:
			a = ACT_PROG;
			break;
		case 2:
			a = SPEECH_PROG;
			break;
		case 3:
			a = RAND_PROG;
			break;
		case 4:
			a = ALL_GREET_PROG;
			break;
		case 5:
			a = CATCH_PROG;
			break;
		case 6:
			a = ARAND_PROG;
			break;
		case 7:
			a = LOAD_PROG;
			break;
		case 8:
			a = COMMAND_PROG;
			break;
		case 9:
			a = WEAPON_PROG;
			break;
		case 10:
			a = ARMOUR_PROG;
			break;
		case 11:
			a = CAN_SEE_PROG;
			break;
		default:
			send_to_char("Invalid progtype.\r\n", ch);
			return eFAILURE;
		}
		currprog->type = a;
		update_objprog_bits(num);
		send_to_char("Proc type changed.\r\n", ch);
		return eSUCCESS;
	}
	else if (!str_cmp(arg, "arglist"))
	{
		//    char arg1[MAX_INPUT_LENGTH];
		argument = one_argument(argument, arg);
		//    argument = one_argument(argument, arg1);
		if (!*arg || !argument || !*argument || !isdigit(*arg))
		{
			send_to_char("$3Syntax$R: opedit [obj_num] arglist <prog> <new arglist>\r\n", ch);
			return eFAILURE;
		}
		a = atoi(arg);
		for (i = 1, currprog = obj_index[num].mobprogs;
			 currprog && i != a;
			 i++, currprog = currprog->next)
			;

		if (!currprog)
		{
			send_to_char("Invalid prog number.\r\n", ch);
			return eFAILURE;
		}
		dc_free(currprog->arglist);
		currprog->arglist = strdup(argument + 1);

		send_to_char("Arglist changed.\r\n", ch);
		return eSUCCESS;
	}
	else if (!str_cmp(arg, "command"))
	{
		argument = one_argument(argument, arg);
		if (!*arg || !isdigit(*arg))
		{
			send_to_char("$3Syntax$R: opedit [obj_num] command <prog>\r\n", ch);
			return eFAILURE;
		}
		a = atoi(arg);
		for (i = 1, currprog = obj_index[num].mobprogs;
			 currprog && i != a;
			 i++, currprog = currprog->next)
			;

		if (!currprog)
		{ // intval was too high
			send_to_char("Invalid prog number.\r\n", ch);
			return eFAILURE;
		}

		ch->desc->backstr = nullptr;
		ch->desc->strnew = &(currprog->comlist);
		ch->desc->max_str = MAX_MESSAGE_LENGTH;

		if (IS_SET(ch->player->toggles, PLR_EDITOR_WEB))
		{
			ch->desc->web_connected = Connection::states::EDIT_MPROG;
		}
		else
		{
			ch->desc->connected = Connection::states::EDIT_MPROG;

			send_to_char("        Write your help entry and stay within the line.(/s saves /h for help)\r\n"
						 "|--------------------------------------------------------------------------------|\r\n",
						 ch);

			if (currprog->comlist)
			{
				ch->desc->backstr = str_dup(currprog->comlist);
				send_to_char(ch->desc->backstr, ch);
			}
		}

		return eSUCCESS;
	}
	else if (!str_cmp(arg, "list"))
	{
		opstat(ch, vnum);
		return eSUCCESS;
	}
	send_to_char("$3Syntax$R: opedit [obj_num] [field] [arg]\r\n"
				 "Edit a field with no args for help on that field.\r\n\r\n"
				 "The field must be one of the following:\r\n"
				 "\tadd\tremove\ttype\targlist\r\n\tcommand\tlist\r\n\r\n",
				 ch);
	char buf[MAX_STRING_LENGTH];
	sprintf(buf, "$3Current object set to: %llu\r\n", ch->player->last_obj_vnum);
	send_to_char(buf, ch);
	return eSUCCESS;
}

int do_oclone(Character *ch, char *argument, int cmd)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg1);
	one_argument(argument, arg2);
	if (!arg1[0] || !arg2[0] || !is_number(arg1) || !is_number(arg2))
	{
		send_to_char("Syntax: oclone <source vnum> <destination vnum>\n\r", ch);
		return eFAILURE;
	}
	Object *obj, *otmp;
	int v1 = atoi(arg1), v2 = atoi(arg2);
	int r1 = real_object(v1), r2 = real_object(v2);
	if (r1 < 0)
	{
		send_to_char("Source vnum does not exist.\r\n", ch);
		return eFAILURE;
	}

	if (!can_modify_object(ch, v2))
	{
		send_to_char("You are unable to work creation outside of your range.\r\n",
					 ch);
		return eFAILURE;
	}

	if (r2 < 0)
	{
		char buf[30];
		sprintf(buf, "new %d", v2);
		int retval = do_oedit(ch, buf, CMD_DEFAULT);
		if (!IS_SET(retval, eSUCCESS))
			return eFAILURE;
		r1 = real_object(v1);
		r2 = real_object(v2);
		if (r2 == -1)
		{
			send_to_char("Something failed. Possibly your destination vnum was too high.\r\n", ch);
			return eFAILURE;
		}
	}
	obj = clone_object(r1);
	if (!obj)
	{
		csendf(ch, "Failure. Unable to clone item.\r\n");
		return eFAILURE;
	}

	/*
	  if(obj_index[obj->item_number].non_combat_func ||
			obj->obj_flags.type_flag == ITEM_MEGAPHONE ||
			has_random(obj)) {
		DC::getInstance()->obj_free_list.insert(obj);
	  }
	*/

	csendf(ch, "Ok.\n\rYou copied item %d (%s) and replaced item %d (%s).\r\n",
		   v1, ((Object *)obj_index[real_object(v1)].item)->short_description,
		   v2, ((Object *)obj_index[real_object(v2)].item)->short_description);

	object_list = object_list->next;
	otmp = (Object *)obj_index[r2].item;
	obj->item_number = r2;
	obj_index[r2].item = (void *)obj;
	obj_index[r2].non_combat_func = 0;
	obj_index[r2].number = 0;
	obj_index[r2].virt = v2;
	obj_index[r2].mobprogs = nullptr;
	obj_index[r2].combat_func = 0;
	obj_index[r2].mobspec = 0;
	// extract_obj(otmp);

	ch->player->last_obj_vnum = v2;
	set_zone_modified_obj(r2);

	return eSUCCESS;
}

int do_mclone(Character *ch, char *argument, int cmd)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg1);
	one_argument(argument, arg2);
	if (!arg1[0] || !arg2[0] || !is_number(arg1) || !is_number(arg2))
	{
		send_to_char("Syntax: mclone <source vnum> <destination vnum>\n\r", ch);
		return eFAILURE;
	}
	Character *mob;
	int vdst = atoi(arg2), vsrc = atoi(arg1);
	int dst = real_mobile(vdst), src = real_mobile(vsrc);
	if (src < 0)
	{
		send_to_char("Source vnum does not exist.\r\n", ch);
		return eFAILURE;
	}

	if (!can_modify_mobile(ch, vdst))
	{
		send_to_char("You are unable to work creation outside of your range.\r\n",
					 ch);
		return eFAILURE;
	}

	if (dst < 0)
	{
		char buf[30];
		sprintf(buf, "new %d", vdst);
		int retval = do_medit(ch, buf, CMD_DEFAULT);
		if (!IS_SET(retval, eSUCCESS))
			return eFAILURE;
		dst = real_mobile(vdst);
		src = real_mobile(vsrc);
		if (dst == -1)
		{
			send_to_char("Something failed. Possibly your destination vnum was too high.\r\n", ch);
			return eFAILURE;
		}
	}
	mob = clone_mobile(src);
	if (!mob)
	{
		csendf(ch, "Failure. Unable to copy mobile.\r\n");
		return eFAILURE;
	}

	// clone_mobile assigns the start of character_list to be mob
	// This undos the change
	mob_index[src].number--;

	auto &character_list = DC::getInstance()->character_list;
	character_list.erase(mob);
	mob->mobdata->nr = dst;

	// Find old mobile in world and remove
	Character *old_mob = (Character *)mob_index[dst].item;
	if (old_mob && old_mob->mobdata)
	{
		auto &character_list = DC::getInstance()->character_list;
		for (auto &tmpch : character_list)
		{
			if (!tmpch->mobdata)
				continue;
			if (old_mob->mobdata->nr == tmpch->mobdata->nr)
				extract_char(tmpch, true);
		}
	}

	csendf(ch, "Ok.\n\rYou copied mob %d (%s) and replaced mob %d (%s).\r\n",
		   vsrc, ((Character *)mob_index[src].item)->short_desc,
		   vdst, ((Character *)mob_index[dst].item)->short_desc);

	// Overwrite old mob with new mob
	mob_index[dst].item = (void *)mob;
	mob_index[dst].number = 0;
	mob_index[dst].non_combat_func = 0;
	mob_index[dst].combat_func = 0;
	mob_index[dst].mobprogs = nullptr;
	mob_index[dst].mobspec = 0;
	mob_index[dst].progtypes = 0;
	mob_index[dst].virt = vdst;

	add_mobspec(dst);

	if (mob_index[src].non_combat_func)
	{
		send_to_char("Warning: hardcoded non_combat function found. Notify coder.\r\n", ch);
	}
	if (mob_index[src].combat_func)
	{
		send_to_char("Warning: hardcoded combat function found. Notify coder.\r\n", ch);
	}
	if (mob_index[src].mobprogs)
	{
		send_to_char("Warning: mob program found. This will need to be copied manually.\r\n", ch);
	}

	ch->player->last_mob_edit = dst;
	set_zone_modified_mob(dst);

	return eSUCCESS;
}
