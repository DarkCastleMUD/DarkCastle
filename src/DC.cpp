/*
 * DC.cpp
 *
 *  Created on: Dec 24, 2017
 *      Author: jhhudso
 *      Based on http://www.cplusplus.com/forum/beginner/152735/#msg792909
 */

#include <cassert>
#include <qcoreapplication.h>
#include <qtypes.h>

#include "DC/DC.h"
#include "DC/db.h"
#include "DC/Version.h"

const QString DC::DEFAULT_LIBRARY_PATH = "../lib";
const QString DC::HINTS_FILE_NAME = "playerhints.txt";

DC::DC(qint32 &argc, QString *argv)
    : QCoreApplication(argc, argv), cf(argc, argv), ssh(this), shops_(this), random_(*QRandomGenerator::global())
{
  setup();
}

DC::DC(config c)
    : QCoreApplication(c.argc_, c.argv_), cf(c), ssh(this), shops_(this), random_(*QRandomGenerator::global())
{
  setup();
}

void DC::setup(void)
{
  qSetMessagePattern(u"%{if-category}%{category}:%{endif}%{function}:%{line}:%{message}"_s);
  QCoreApplication::setOrganizationName("DarkCastleMUD");
  QCoreApplication::setOrganizationDomain("dcastle.org");
  QCoreApplication::setApplicationName("DarkCastle");
  if (cf.sql)
  {
    database_ = Database("dcastle");
  }
  findLibrary();
  QLocale::setDefault(QLocale::English);
}

void DC::findLibrary(void)
{
  QDir libraryDirectory;
  QString absolutePath = libraryDirectory.absolutePath();
  QSet<QString> searchedDirectories;

  if (libraryDirectory.dirName() != u"lib"_s)
  {
    searchedDirectories.insert(absolutePath);
    if (!libraryDirectory.cd(absolutePath + u"/lib"_s))
    {
      searchedDirectories.insert(absolutePath + u"/lib"_s);
      if (!libraryDirectory.cd(absolutePath + u"/../lib"_s))
      {
        searchedDirectories.insert(absolutePath + u"/../lib"_s);
        if (!libraryDirectory.cd(QCoreApplication::applicationDirPath() + u"/../lib"_s))
        {
          searchedDirectories.insert(QCoreApplication::applicationDirPath() + u"/../lib"_s);
        }
      }
    }
  }
  if (libraryDirectory.exists() && libraryDirectory.dirName() == u"lib"_s)
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
  return dynamic_cast<DC *>(DC::instance());
}

zone_t DC::getRoomZone(room_t room_nr)
{
  for (auto [zone_key, zone] : zones_.asKeyValueRange())
    if (room_nr >= zone.getRealBottom() && room_nr <= zone.getTop())
      return zone_key;
  return zone_t();
}

QString DC::getZoneName(zone_t zone_key)
{
  if (zones_.contains(zone_key))
  {
    return zones_.value(zone_key).name();
  }

  return {};
}

void DC::setZoneClanOwner(zone_t zone_key, qint32 clan_key)
{
  DCPtr dc = getInstance();
  if (dc != nullptr)
  {
    if (dc->zones_.contains(zone_key))
    {
      dc->zones_[zone_key].clanowner = clan_key;
    }
  }
}

void DC::setZoneClanGold(zone_t zone_key, gold_t gold)
{
  DCPtr dc = getInstance();
  if (dc != nullptr)
  {
    if (dc->zones_.contains(zone_key))
    {
      dc->zones_[zone_key].gold = gold;
    }
  }
}

void DC::setZoneTopRoom(zone_t zone_key, room_t room_key)
{
  DCPtr dc = getInstance();
  if (dc != nullptr)
  {
    if (dc->zones_.contains(zone_key))
    {
      dc->zones_[zone_key].setRealTop(room_key);
    }
  }
}

void DC::setZoneBottomRoom(zone_t zone_key, room_t room_key)
{
  DCPtr dc = getInstance();
  if (dc != nullptr)
  {
    if (dc->zones_.contains(zone_key))
    {
      dc->zones_[zone_key].setRealBottom(room_key);
    }
  }
}

void DC::setZoneModified(zone_t zone_key)
{
  DCPtr dc = getInstance();
  if (dc != nullptr)
  {
    if (dc->zones_.contains(zone_key))
    {
      dc->zones_[zone_key].setModified();
    }
  }
}

void DC::setZoneNotModified(zone_t zone_key)
{
  DCPtr dc = getInstance();
  if (dc != nullptr)
  {
    if (dc->zones_.contains(zone_key))
    {
      dc->zones_[zone_key].setModified(false);
    }
  }
}

void DC::incrementZoneDiedTick(zone_t zone_key)
{
  DCPtr dc = getInstance();
  if (dc != nullptr)
  {
    if (dc->zones_.contains(zone_key))
    {
      dc->zones_[zone_key].incrementDiedThisTick();
    }
  }
}

void DC::resetZone(zone_t zone_key, Zone::ResetType reset_type)
{
  DCPtr dc = getInstance();
  if (dc != nullptr)
  {
    if (dc->zones_.contains(zone_key))
    {
      dc->zones_[zone_key].reset(reset_type);
    }
  }
}

bool DC::authenticate(QString username, QString password, quint64 level)
{
  username = username.toLower();
  username[0] = username[0].toUpper();
  if (!validateName(username))
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

  return obj_index[rnum].item;
}

void DC::logverbose(QString str, quint64 god_level, DC::LogChannel type, CharacterPtr vict)
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
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_ARENA, "%s started a Clan Chaos arena.", qPrintable(ch->name()));
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
  sendln(u"$3%1$R: %2"_s.arg(desc).arg(value));

  auto command = dc_->CMD_.find(cmd);
  if (command)
    sendln(u"$3Syntax$R: %1 [vnum] %2 <value>"_s.arg(command->name()).arg(fieldname));

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

const QString DC::menu = u"\r\nWelcome to Dark Castle Mud\r\n\r\n0) Exit Dark Castle.\r\n1) Enter the game.\r\n2) Enter your character's description.\r\n3) Change your password.\r\n4) Delete this character.\r\n\r\n   Make your choice: "_s;

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
    : dc_(dc), QObject(dc)
{
}

Connection::Connection(DCPtr dc)
    : QObject(dc), dc_(dc)
{
}

RoomDirection::RoomDirection(DCPtr dc)
    : dc_(dc), QObject(dc)
{
}

QString ANA(ObjectPtr obj)
{
  if (!obj || obj->name().isEmpty())
    return {};

  if (u"aeiouyAEIOUY"_s.contains(obj->name()[0]))
    return u"An"_s;
  else
    return u"A"_s;
}

QString SANA(ObjectPtr obj)
{
  if (!obj || obj->name().isEmpty())
    return {};

  if (u"aeiouyAEIOUY"_s.contains(obj->name()[0]))
    return u"an"_s;
  else
    return u"a"_s;
}

QTextStream &operator>>(QTextStream &in, Room &room)
{
  room_t room_nr = {};
  QString temp = {};
  QChar ch = {};
  qint32 dir = {};
  extra_descr_data *new_new_descr{};
  zone_t zone_nr = {};

  ch = fread_char(in);

  if (ch != '$')
  {
    room_nr = fread_int<room_t>(in, 0, 1000000);
    temp = fread_string(in, 0);

    if (room_nr)
    {
      /*
      dc_->currentVNUM(room_nr);
      dc_->currentType("Room");
      dc_->currentName(temp);

      if (room_nr >= dc_->top_of_world_alloc)
      {
        dc_->top_of_world_alloc = room_nr + 200;
      }

      if (dc_->top_of_world < room_nr)
        dc_->top_of_world = room_nr;
      */

      room.paths_ = {};
      room.number = room_nr;
      room.name_ = temp;
    }
    QString description = fread_string(in, 0);
    if (room_nr)
    {
      room.description_ = description;
      room.tracks_ = {};
      room.denied = {};
      // dc_->total_rooms++;
    }
    // Ignore recorded zone number since it may not longer be valid
    fread_int<quint64>(in, -1, 64000); // zone nr

    if (room_nr)
    {
      // Go through the zone table until room.number is
      // in the current zone.

      bool found = false;
      zone_t zone_nr = {};
      for (auto [zone_key, zone] : room.dc_->zones_.asKeyValueRange())
      {
        if (zone.getBottom() <= room.number && zone.getTop() >= room.number)
        {
          found = true;
          zone_nr = zone_key;
          break;
        }
      }
      if (!found)
      {
        // QString error = u"Room %1 is outside of any zone."_s.arg(room_nr);
        // dc_->logentry(error);
        // dc_->logentry(u"Room outside of ANY zone.  ERROR"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
      }
      else
      {
        auto &zone = room.dc_->zones_[zone_nr];
        if (room_nr >= zone.getBottom() && room_nr <= zone.getTop())
        {
          if (room_nr < zone.getRealBottom() || zone.getRealBottom() == 0)
          {
            zone.setRealBottom(room_nr);
          }
          if (room_nr > zone.getRealTop() || zone.getRealTop() == 0)
          {
            zone.setRealTop(room_nr);
          }
        }
        room.zone = zone_nr;
      }
    }

    quint32 room_flags = fread_bitvector(in);

    if (room_nr)
    {
      room.room_flags = room_flags;
      if (isSet(room.room_flags, NO_ASTRAL))
      {
        REMOVE_BIT(room.room_flags, NO_ASTRAL);
      }

      // This bitvector is for runtime and not stored in the files, so just initialize it to 0
      room.temp_room_flags = {};
    }

    qint32 sector_type = fread_int<qint32>(in, -1, 64000);

    if (room_nr)
    {
      room.sector_type = sector_type;
      room.funct = {};
      room.contents_ = {};
      room.people_ = {};
      room.light = {}; /* Zero light sources */

      for (size_t tmp = {}; tmp <= 5; tmp++)
        room.dir_option[tmp] = {};

      room.ex_description = {};
    }

    for (;;)
    {
      ch = fread_char(in); /* dir field */

      /* direction field */
      if (ch == 'D')
      {
        dir = fread_int(in, 0, 5);
        setup_dir(in, room_nr, dir);
      }
      /* extra description field */
      else if (ch == 'E')
      {
        // strip off the \n after the E
        if (fread_char(in) != '\n')
        {
          fseek(in, -1, SEEK_CUR);
        }

        new_new_descr = new extra_descr_data;
        new_new_descr->keyword_ = fread_string(in, 0);
        new_new_descr->description_ = fread_string(in, 0);

        if (room_nr)
        {
          new_new_descr->next = room.ex_description;
          room.ex_description = new_new_descr;
        }
        else
        {
          new_new_descr = {};
        }
      }
      else if (ch == 'B')
      {
        deny_data *deni;

        deni = new deny_data;
        deni->vnum = fread_int(in, -1, 2147483467);

        if (room_nr)
        {
          deni->next = room.denied;
          room.denied = deni;
        }
        else
        {
          deni = {};
        }
      }
      else if (ch == 'S') /* end of current room */
        break;
      else if (ch == 'C')
      {
        qint32 c_class = fread_int(in, 0, CLASS_MAX);
        if (room_nr)
        {
          room.allow_class[c_class] = true;
        }
      }
    } // of for (;;) (get directions and extra descs)
  } // if == $

  return in;
}
