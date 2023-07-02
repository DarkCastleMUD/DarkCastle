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
#include "version.h"

string DC::version = VERSION;

DC::DC(int &argc, char **argv)
	: QCoreApplication(argc, argv), ssh(this), shops_(this)
{
	// ssh = QSharedPointer<SSH::SSH>(new SSH::SSH);
}

void DC::removeDead(void)
{

	for (auto &node : death_list)
	{
		character_list.erase(node.first);
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
	unordered_set<Character *> remove_list;

	for (auto &ch : shooting_list)
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

	for (auto &ch : remove_list)
	{
		shooting_list.erase(ch);
	}
}

string DC::getVersion(void)
{
	return version;
}

string DC::getBuildTime(void)
{
	return string(BUILD_TIME);
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
			if (dc->zones.value(zone_key).name != nullptr)
			{
				return dc->zones.value(zone_key).name;
			}
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

bool DC::authenticate(QString username, QString password, level_t level)
{
	username = username.toLower();
	username[0] = username[0].toUpper();
	if (!Character::validateName(username))
	{
		return false;
	}

	Connection d(this);
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
		if (GET_LEVEL(d.character) >= level)
		{
			return true;
		}
	}

	return false;
}

bool DC::authenticate(const QHttpServerRequest &request, level_t level)
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
