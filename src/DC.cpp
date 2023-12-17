/*
 * DC.cpp
 *
 *  Created on: Dec 24, 2017
 *      Author: jhhudso
 *      Based on http://www.cplusplus.com/forum/beginner/152735/#msg792909
 */

#include <assert.h>

#include "DC.h"
#include "db.h"
#include "Version.h"

const QString DC::DEFAULT_LIBRARY_PATH = "../lib";
const QString DC::HINTS_FILE_NAME = "playerhints.txt";
extern struct index_data *obj_index;

DC::DC(int &argc, char **argv)
	: QCoreApplication(argc, argv), ssh(this), shops_(this), random_(*QRandomGenerator::global()), clan_list(nullptr), end_clan_list(nullptr)
{
}

void DC::removeDead(void)
{
	for (auto &node : death_list)
	{
		character_list.erase(node.first);
		assert(!character_list.contains(node.first));
		shooting_list.erase(node.first);
		Trace &t = node.second;
		t.addTrack("DC::removeDeath");
		free_char(node.first, t);
	}
	death_list.clear();

	while (!obj_free_list.empty())
	{
		Object *obj = *(obj_free_list.cbegin());
		active_obj_list.erase(obj);
		obj_free_list.erase(obj);
		delete obj;
	}
}

void DC::handleShooting(void)
{
	std::unordered_set<Character *> remove_list;

	for (const auto &ch : shooting_list)
	{
		// ignore the dead
		if (ch->in_room == DC::NOWHERE)
		{
			continue;
		}

		if (ch->shotsthisround)
		{
			ch->shotsthisround--;
		}
		else
		{
			remove_list.insert(ch);
		}
	}

	for (const auto &ch : remove_list)
	{
		shooting_list.erase(ch);
	}
}

QString DC::getBuildVersion(void)
{
	return Version::build_version_;
}

QString DC::getBuildTime(void)
{
	return Version::build_time_;
}

DC *DC::getInstance(void)
{
	DC *dc = dynamic_cast<DC *>(DC::instance());
	assert(dc != nullptr);
	return dc;
}

zone_t DC::getRoomZone(room_t room_nr)
{
	DC *dc = getInstance();
	if (dc != nullptr)
	{
		for (auto [zone_key, zone] : dc->zones.asKeyValueRange())
		{
			if (room_nr >= zone.getRealBottom() && room_nr <= zone.getTop())
			{
				return zone_key;
			}
		}
	}
	return zone_t();
}

QString DC::getZoneName(zone_t zone_key)
{
	DC *dc = getInstance();
	if (dc != nullptr)
	{
		if (dc->zones.contains(zone_key))
		{
			return dc->zones.value(zone_key).Name();
		}
	}

	return QString();
}

void DC::setZoneClanOwner(zone_t zone_key, int clan_key)
{
	DC *dc = getInstance();
	if (dc != nullptr)
	{
		if (dc->zones.contains(zone_key))
		{
			dc->zones[zone_key].clanowner = clan_key;
		}
	}
}

void DC::setZoneClanGold(zone_t zone_key, gold_t gold)
{
	DC *dc = getInstance();
	if (dc != nullptr)
	{
		if (dc->zones.contains(zone_key))
		{
			dc->zones[zone_key].gold = gold;
		}
	}
}

void DC::setZoneTopRoom(zone_t zone_key, room_t room_key)
{
	DC *dc = getInstance();
	if (dc != nullptr)
	{
		if (dc->zones.contains(zone_key))
		{
			dc->zones[zone_key].setRealTop(room_key);
		}
	}
}

void DC::setZoneBottomRoom(zone_t zone_key, room_t room_key)
{
	DC *dc = getInstance();
	if (dc != nullptr)
	{
		if (dc->zones.contains(zone_key))
		{
			dc->zones[zone_key].setRealBottom(room_key);
		}
	}
}

void DC::setZoneModified(zone_t zone_key)
{
	DC *dc = getInstance();
	if (dc != nullptr)
	{
		if (dc->zones.contains(zone_key))
		{
			dc->zones[zone_key].setModified();
		}
	}
}

void DC::setZoneNotModified(zone_t zone_key)
{
	DC *dc = getInstance();
	if (dc != nullptr)
	{
		if (dc->zones.contains(zone_key))
		{
			dc->zones[zone_key].setModified(false);
		}
	}
}

void DC::incrementZoneDiedTick(zone_t zone_key)
{
	DC *dc = getInstance();
	if (dc != nullptr)
	{
		if (dc->zones.contains(zone_key))
		{
			dc->zones[zone_key].incrementDiedThisTick();
		}
	}
}

void DC::resetZone(zone_t zone_key, Zone::ResetType reset_type)
{
	DC *dc = getInstance();
	if (dc != nullptr)
	{
		if (dc->zones.contains(zone_key))
		{
			dc->zones[zone_key].reset(reset_type);
		}
	}
}

bool DC::authenticate(QString username, QString password, uint64_t level)
{
	username = username.toLower();
	username[0] = username[0].toUpper();
	if (!Character::validateName(username))
	{
		return false;
	}

	Connection d;
	if (!load_char_obj(&d, username.toStdString().c_str()))
	{
		return false;
	}

	if (d.character == nullptr || d.character->player == nullptr)
	{
		return false;
	}

	QString cipher = d.character->player->pwd;
	if (crypt(password.toStdString().c_str(), cipher.toStdString().c_str()) == cipher)
	{
		if (d.character->getLevel() >= level)
		{
			return true;
		}
	}

	return false;
}

bool DC::authenticate(const QHttpServerRequest &request, uint64_t level)
{
	const auto query = request.query();
	if (!query.hasQueryItem("username") || !query.hasQueryItem("password"))
	{
		return false;
	}

	QString username = query.queryItemValue("username");
	QString password = query.queryItemValue("password");

	return authenticate(username, password, level);
}

bool DC::isAllowedHost(QHostAddress address)
{
	for (const auto &host : host_list_)
	{
		if (host == address)
		{
			return true;
		}
	}
	return false;
}

Object *DC::getObject(vnum_t vnum)
{
	vnum_t rnum = real_object(vnum);

	if (rnum == -1)
	{
		return nullptr;
	}

	return static_cast<Object *>(obj_index[rnum].item);
}

void close_file(std::FILE *fp)
{
	if (fp)
	{
		std::fclose(fp);
	}
}

auto get_bestow_command(QString command_name) -> std::expected<bestowable_god_commands_type, search_error>
{
	// auto it = std::find_if(begin(DC::bestowable_god_commands), end(DC::bestowable_god_commands));

	for (const auto &bgc : DC::bestowable_god_commands)
	{
		if (bgc.name == command_name)
		{
			return bgc;
		}
	}
	return std::unexpected(search_error::not_found);
}
