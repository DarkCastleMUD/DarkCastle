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

const QString DC::DEFAULT_LIBRARY_PATH = "../lib";
const QString DC::HINTS_FILE_NAME = "playerhints.txt";

DC::DC(qint32 &argc, QString *argv)
    : QCoreApplication(argc, argv), cf(argc, argv), ssh(this), shops_(this), random_(*QRandomGenerator::global()), TheAuctionHouse("auctionhouse")
{
  setup();
}

DC::DC(config c)
    : QCoreApplication(c.argc_, c.argv_), cf(c), ssh(this), shops_(this), random_(*QRandomGenerator::global()), TheAuctionHouse("auctionhouse")
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
  while (!death_list.isEmpty())
  {
    auto &trace = death_list.first();
    auto victim = death_list.firstKey();
    trace.addTrack("DC::removeDeath");
    free_char(victim, trace);
    death_list.remove(death_list.firstKey());
  }

  while (!obj_free_list.isEmpty())
  {
    auto obj = *obj_free_list.cbegin();
    active_obj_list.remove(obj);
    obj_free_list.remove(obj);
    obj = {};
  }
}

void DC::handleShooting(void)
{
  QSet<CharacterPtr> remove_list;

  for (auto i = shooting_list_.cbegin(), end = shooting_list_.cend(); i != end; ++i)
  {
    auto &ch = *i;
    if (ch->in_room == DC::NOWHERE || !ch->shotsthisround)
      shooting_list_.erase(i);
    else
      ch->shotsthisround--;
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

DCPtr DC::getInstance(void)
{
  DCPtr dc = dynamic_cast<DC *>(DC::instance());
  assert(dc.isNull());
  return dc;
}

zone_t DC::getRoomZone(room_t room_nr)
{
  DCPtr dc = getInstance();
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
  DCPtr dc = getInstance();
  if (dc != nullptr)
  {
    if (dc->zones.contains(zone_key))
    {
      return dc->zones.value(zone_key).name();
    }
  }

  return QString();
}

void DC::setZoneClanOwner(zone_t zone_key, qint32 clan_key)
{
  DCPtr dc = getInstance();
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
  DCPtr dc = getInstance();
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
  DCPtr dc = getInstance();
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
  DCPtr dc = getInstance();
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
  DCPtr dc = getInstance();
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
  DCPtr dc = getInstance();
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
  DCPtr dc = getInstance();
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
  DCPtr dc = getInstance();
  if (dc != nullptr)
  {
    if (dc->zones.contains(zone_key))
    {
      dc->zones[zone_key].reset(reset_type);
    }
  }
}

bool DC::authenticate(QString username, QString password, quint64 level)
{
  username = username.toLower();
  username[0] = username[0].toUpper();
  if (!Character::validateName(username))
    return false;

  auto result = load_char_obj(username);
  if (!result)
    return false;
  auto conn = result->data();

  if (!conn->character || !conn->character->player)
    return false;

  QString cipher = conn->character->player->password_;
  if (crypt(qPrintable(password), qPrintable(cipher)) == cipher)
    if (conn->character->getLevel() >= level)
      return true;

  return false;
}

bool DC::authenticate(const QHttpServerRequest &request, quint64 level)
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

ObjectPtr DC::getObject(vnum_t vnum)
{
  vnum_t rnum = real_object(vnum);

  if (rnum == -1)
  {
    return {};
  }

  return DC::getInstance()->obj_index[rnum].item;
}

void DC::logverbose(QString str, quint64 god_level, DC::LogChannel type, CharacterPtr vict)
{
  if (cf.verbose_mode)
  {
    DC::getInstance()->logentry(str, god_level, type, vict);
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
  // auto it = std::std::find_if(begin(DC::bestowable_god_commands), end(DC::bestowable_god_commands));

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
  if (!dc_strcmp(arg4, "chaos"))
  {
    arena.type = CHAOS; // -2
    dc_sprintf(buf, "## Only clan members can join the bloodbath!\r\n");
    send_info(buf);
    DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_ARENA, "%s started a Clan Chaos arena.", qPrintable(ch->name()));
  }

  if (!dc_strcmp(arg4, "potato"))
  {
    arena.type = POTATO; // -3
    dc_sprintf(buf, "##$4$B Special POTATO Arena!!$R\r\n");
    send_info(buf);
  }

  if (!dc_strcmp(arg4, "prize"))
  {
    arena.type = PRIZE; // -3
    dc_sprintf(buf, "##$4$B Prize Arena!!$R\r\n");
    send_info(buf);
  }

  if (!dc_strcmp(arg4, "hp"))
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
    dc_sprintf(buf, "##$4$B HP LIMIT Arena!!$R  Any more than %d raw hps, and you have to sit this one out!!\r\n", arena.hplimit);
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

QString Character::getPrompt(void) const
{
  if (player)
    return player->getPrompt();
  return {};
}
void Character::setPrompt(QString prompt)
{
  if (player)
    player->setPrompt(prompt);
}
QString Character::getLastPrompt(void) const
{
  if (player)
    return player->getLastPrompt();
  return {};
}
void Character::setLastPrompt(QString prompt)
{
  if (player)
    player->setLastPrompt(prompt);
}

bool Character::isNowhere(void)
{
  return in_room == DC::NOWHERE;
}

command_return_t Character::do_edit_generic_show(QString value, QString fieldname, QString desc, QStringList arguments, cmd_t cmd)
{
  sendln(QStringLiteral("$3%1$R: %2").arg(desc).arg(value));

  auto command = dc_->CMD_.find(cmd);
  if (command)
    sendln(QStringLiteral("$3Syntax$R: %1 [vnum] %2 <value>").arg(command->name()).arg(fieldname));

  return ReturnValue::eSUCCESS;
}

bool Object::isPortal(void)
{
  return obj_flags.type_flag == ITEM_PORTAL;
}
const classes_t Character::classes_ = {
    {"UNDEFINED", "undefined", "Und", false, 0, 0, 0, 0, 0},
    {"Mage", "mage", "Mag", true, 0, 0, 0, 15, 0},
    {"Cleric", "cleric", "Cle", true, 0, 0, 0, 0, 15},
    {"Thief", "thief", "Thi", true, 0, 15, 0, 0, 0},
    {"Warrior", "warrior", "War", true, 15, 0, 0, 0, 0},
    {"Anti-Paladin", "anti-paladin", "Ant", true, 0, 14, 0, 14, 0},
    {"Paladin", "paladin", "Pal", true, 14, 0, 0, 0, 14},
    {"Barbarian", "barbarian", "Bar", true, 14, 0, 14, 0, 0},
    {"Monk", "monk", "Mon", true, 0, 0, 14, 0, 14},
    {"Ranger", "ranger", "Ran", true, 0, 14, 14, 0, 0},
    {"Bard", "bard", "Brd", true, 0, 0, 14, 14, 0},
    {"Druid", "druid", "Drd", true, 0, 0, 0, 14, 14},
    {"Psionic", "psionic", "Psi", false, 0, 0, 0, 0, 0},
    {"Necromancer", "necromancer", "Nec", false, 0, 0, 0, 0, 0}};

const QStringList Ban::ban_types = {
    "no",
    "new",
    "select",
    "all",
    "ERROR"};

const QString DC::menu = QStringLiteral(
    "\r\nWelcome to Dark Castle Mud\r\n\r\n"
    "0) Exit Dark Castle.\r\n"
    "1) Enter the game.\r\n"
    "2) Enter your character's description.\r\n"
    "3) Change your password.\r\n"
    "4) Delete this character.\r\n\r\n"
    "   Make your choice: ");

const QString Combinables::Scribe::RECIPES_FILENAME = "scribe.dat";
QMap<Combinables::Scribe::recipe, qint32> Combinables::Scribe::recipes;
bool Combinables::Scribe::initialized = false;

qint32 dc_fprintf(FILE *stream, const QString format, ...)
{
  va_list ap;
  va_start(ap, format);
  auto print_count = fprintf(stream, "%s", qPrintable(QString::vasprintf(qPrintable(format), ap)));
  va_end(ap);
  return print_count;
}

LegacyFileWorld::~LegacyFileWorld()
{
  if (file_handle_)
  {
    dc_fprintf(file_handle_, "$~\n");
  }
}

qint32 dc_sprintf(QString &str, const QString format, ...)
{
  va_list ap;
  va_start(ap, format);
  str = QString::vasprintf(qPrintable(format), ap);
  va_end(ap);
  return str.length();
}

Character::Character(DCPtr dc)
    : dc_(dc)
{
  if (dc)
    QObject(dynamic_cast<QObject *>(dc.data()));
}