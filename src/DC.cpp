/*
 * DC.cpp
 *
 *  Created on: Dec 24, 2017
 *      Author: jhhudso
 *      Based on http://www.cplusplus.com/forum/beginner/152735/#msg792909
 */

#include <cassert>

#include "DC/DC.h"
#include "DC/db.h"
#include "DC/Version.h"
#include "DC/character.h"

const QString DC::DEFAULT_LIBRARY_PATH = "../lib";
const QString DC::HINTS_FILE_NAME = "playerhints.txt";

DC::DC(int &argc, char **argv)
	: QCoreApplication(argc, argv), cf(argc, argv), ssh(this), shops_(this), random_(*QRandomGenerator::global()), clan_list(nullptr), end_clan_list(nullptr), TheAuctionHouse("auctionhouse")
{
	setup();
}

DC::DC(config c)
	: QCoreApplication(c.argc_, c.argv_), cf(c), ssh(this), shops_(this), random_(*QRandomGenerator::global()), clan_list(nullptr), end_clan_list(nullptr), TheAuctionHouse("auctionhouse")
{
	setup();
}

void DC::setup(void)
{
	qSetMessagePattern(QStringLiteral("%{if-category}%{category}:%{endif}%{function}:%{line}:%{message}"));
	QCoreApplication::setOrganizationName("DarkCastleMUD");
	QCoreApplication::setOrganizationDomain("dcastle.org");
	QCoreApplication::setApplicationName("DarkCastle");
	if (cf.sql)
	{
		database_ = Database("dcastle");
	}
	findLibrary();
}

void DC::findLibrary(void)
{
	QDir libraryDirectory;
	QString absolutePath = libraryDirectory.absolutePath();
	QSet<QString> searchedDirectories;

	if (libraryDirectory.dirName() != QStringLiteral("lib"))
	{
		searchedDirectories.insert(absolutePath);
		if (!libraryDirectory.cd(absolutePath + QStringLiteral("/lib")))
		{
			searchedDirectories.insert(absolutePath + QStringLiteral("/lib"));
			if (!libraryDirectory.cd(absolutePath + QStringLiteral("/../lib")))
			{
				searchedDirectories.insert(absolutePath + QStringLiteral("/../lib"));
				if (!libraryDirectory.cd(QCoreApplication::applicationDirPath() + QStringLiteral("/../lib")))
				{
					searchedDirectories.insert(QCoreApplication::applicationDirPath() + QStringLiteral("/../lib"));
				}
			}
		}
	}
	if (libraryDirectory.exists() && libraryDirectory.dirName() == QStringLiteral("lib"))
	{
		QDir::setCurrent(libraryDirectory.absolutePath());
	}
	else
	{
		qWarning("Unable to locate lib directory at following locations:");
		qWarning() << searchedDirectories;
	}
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

	return static_cast<Object *>(DC::getInstance()->obj_index[rnum].item);
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

auto Character::do_arena_info(QStringList arguments) -> command_return_t
{
	sendln("Arena info:");
	return command_return_t();
}

auto Character::do_arena_start(QStringList arguments) -> command_return_t
{
	/*
	if (*arg4)
  {
	if (!strcmp(arg4, "chaos"))
	{
	  arena.type = CHAOS; // -2
	  sprintf(buf, "## Only clan members can join the bloodbath!\r\n");
	  send_info(buf);
	  logf(IMMORTAL, LogChannels::LOG_ARENA, "%s started a Clan Chaos arena.", GET_NAME(ch));
	}

	if (!strcmp(arg4, "potato"))
	{
	  arena.type = POTATO; // -3
	  sprintf(buf, "##$4$B Special POTATO Arena!!$R\r\n");
	  send_info(buf);
	}

	if (!strcmp(arg4, "prize"))
	{
	  arena.type = PRIZE; // -3
	  sprintf(buf, "##$4$B Prize Arena!!$R\r\n");
	  send_info(buf);
	}

	if (!strcmp(arg4, "hp"))
	{
	  if (*arg5)
	  {
		arena.hplimit = atoi(arg5);
		if (arena.hplimit <= 0)
		  arena.hplimit = 1000;
	  }
	  else
		arena.hplimit = 1000;

	  arena.type = HP; // -4
	  sprintf(buf, "##$4$B HP LIMIT Arena!!$R  Any more than %d raw hps, and you have to sit this one out!!\r\n", arena.hplimit);
	  send_info(buf);
	}
  }
  else
  {
	arena.type = NORMAL;
  }
	ch->sendln("The Arena has been opened for the specified levels.");
  */
	sendln("Arena start:");
	return command_return_t();
}

auto Character::do_arena_join(QStringList arguments) -> command_return_t
{
	sendln("Arena join:");
	return command_return_t();
}

auto Character::do_arena_cancel(QStringList arguments) -> command_return_t
{
	sendln("Arena cancel:");
	return command_return_t();
}

auto Character::do_arena_usage(QStringList arguments) -> command_return_t
{
	sendln("Usage:");
	sendln("arena info          - Shows current arena status");
	sendln("arena start         - Start an arena open to anyone for free");
	sendln("arena start # gold  - Start an arena open to anyone whom pays # gold. Winner takes all.");
	sendln("arena join          - Join an arena that's free to play");
	sendln("arena join # gold   - Join an arena by paying the entrance fee of # gold");
	sendln("arena cancel        - Cancel an arena");

	return command_return_t();
}
