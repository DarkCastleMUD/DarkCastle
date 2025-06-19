/*
 * DC.cpp
 *
 *  Created on: Dec 24, 2017
 *      Author: jhhudso
 *      Based on http://www.cplusplus.com/forum/beginner/152735/#msg792909
 */

#include <cassert>
#include <expected>

#include "DC/DC.h"
#include "DC/db.h"
#include "DC/Version.h"
#include "DC/character.h"
#include "DC/connect.h"
#include "DC/obj.h"

const QString DC::DEFAULT_LIBRARY_PATH = "../lib";
const QString DC::HINTS_FILE_NAME = "playerhints.txt";

DC::DC(int &argc, char **argv)
	: QCoreApplication(argc, argv), cf(argc, argv), ssh(this), shops_(this), random_(*QRandomGenerator::global()), clan_list(nullptr), end_clan_list(nullptr), TheAuctionHouse(this, "auctionhouse")
{
	setup();
}

DC::DC(config c)
	: QCoreApplication(c.argc_, c.argv_), cf(c), ssh(this), shops_(this), random_(*QRandomGenerator::global()), clan_list(nullptr), end_clan_list(nullptr), TheAuctionHouse(this, "auctionhouse")
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
			dc->zones[zone_key].setNeedsSaving();
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
			dc->zones[zone_key].setNeedsSaving(false);
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
	if (obj_index.contains(vnum))
	{
		return obj_index[vnum].item;
	}
	return {};
}

void DC::logverbose(QString str, uint64_t god_level, DC::LogChannel type, Character *vict)
{
	if (cf.verbose_mode)
	{
		logentry(str, god_level, type, vict);
	}
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
	  logf(IMMORTAL, DC::LogChannel::LOG_ARENA, "%s started a Clan Chaos arena.", GET_NAME(ch));
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

world_file_list_item &FileIndexes::newRange(QString filename, vnum_t firstvnum, vnum_t lastvnum)
{
	auto &item = files_[filename];
	item.filename = filename;
	item.firstnum = firstvnum;
	item.lastnum = firstvnum;
	item.flags = {};

	item.setNeedsSaving(true);
	saveRangeIndex();
	return item;
}

bool FileIndexes::saveRangeIndex(void)
{
	QQueue<world_file_list_item> ranges_to_remove{};
	for (auto &range : files_)
	{
		for (auto &larger_range : files_)
		{
			if (!range.isRemoved() &&
				!larger_range.isRemoved() &&
				range != larger_range &&
				range.firstnum >= larger_range.firstnum &&
				range.firstnum <= larger_range.lastnum &&
				range.lastnum >= larger_range.firstnum &&
				range.lastnum <= larger_range.lastnum)
			{
				range.setRemoved();
				ranges_to_remove.push_back(range);
				larger_range.setNeedsSaving();
			}
		}
	}

	while (!ranges_to_remove.isEmpty())
	{
		auto range = ranges_to_remove.back();
		files_.remove(range.filename);
		if (!QFile(range.filename).remove())
			qDebug("Failed to remove range file '%s'", qUtf8Printable(range.filename));
		else
			qDebug("Removed range file '%s'", qUtf8Printable(range.filename));
		ranges_to_remove.pop_back();
	}

	QFile objectindex{QFile(QStringLiteral("objectindex"))};
	if (objectindex.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		objectindex.write("* This file contains the list of the files which make up the objects\n"
						  "* Comments are on lines that begin with a \"*\"\n"
						  "* The files themselves, are located in the lib/objects directory\n"
						  "* The file name should be surrounded with <tilda>s on a line of it's own.\n"
						  "* ie: <tilda><enter>filename<tilda>\n"
						  "* DO NOT USE TILDAS IN THE COMMENTS*\n"
						  "~\n");
		for (const auto &range : files_)
		{
			objectindex.write(qUtf8Printable(QStringLiteral("%1~\n").arg(range.filename)));
		}
		objectindex.write(qUtf8Printable(QStringLiteral("$~")));
		objectindex.close();
	}
	else
	{
		logbug("Error writing to objectindex.");
		return false;
	}

	return true;
}

world_file_list_item::operator bool(void) const
{
	return !filename.isEmpty();
}

world_file_list_t FileIndexes::getFiles(void) const
{
	return files_;
}

world_file_list_item &FileIndexes::findRange(QString filename)
{
	return files_[filename];
}

world_file_list_item &FileIndexes::findRange(vnum_t firstvnum, vnum_t lastvnum)
{
	for (auto &owf : files_)
	{
		if (firstvnum >= owf.firstnum && lastvnum <= owf.lastnum)
		{
			return owf;
		}
	}
	static world_file_list_item empty{};
	empty = {};
	return empty;
}

bool world_file_list_item::isNeedsSaving(void)
{
	return isSet(flags, WORLD_FILE_MODIFIED);
}

bool world_file_list_item::isRemoved(void)
{
	return isSet(flags, WORLD_FILE_REMOVED);
}

void world_file_list_item::setNeedsSaving(bool modified)
{
	if (modified)
		SET_BIT(flags, WORLD_FILE_MODIFIED);
	else
		REMOVE_BIT(flags, WORLD_FILE_MODIFIED);
}

void world_file_list_item::setRemoved(bool removed)
{
	if (removed)
		SET_BIT(flags, WORLD_FILE_REMOVED);
	else
		REMOVE_BIT(flags, WORLD_FILE_REMOVED);
}

bool world_file_list_item::operator==(const world_file_list_item wfli)
{
	return wfli.filename == filename && wfli.firstnum == firstnum && wfli.lastnum == lastnum && wfli.flags == flags;
}
