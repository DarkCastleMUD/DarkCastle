/*
 * DC.cpp
 *
 *  Created on: Dec 24, 2017
 *      Author: jhhudso
 *      Based on http://www.cplusplus.com/forum/beginner/152735/#msg792909
 */
#include "DC/DC.h"

const QString DC::DEFAULT_LIBRARY_PATH = "../lib";
const QString DC::HINTS_FILE_NAME = "playerhints.txt";

DC::DC(qint32 &argc, char **argv)
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
    database_ = Database(this, "dcastle");
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

zone_t DC::getRoomZone(room_t room_number)
{
  for (auto [zone_key, zone] : zones_.asKeyValueRange())
    if (room_number >= zone.getRealBottom() && room_number <= zone.getTop())
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

  return obj_index_[rnum]->item;
}

void DC::logverbose(QString str, quint64 god_level, DC::LogChannel type, CharacterPtr vict)
{
  if (cf.verbose_mode)
  {
    logentry(str, god_level, type, vict);
  }
}

void close_file(std::FILE *stream)
{
  if (stream)
  {
    std::
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

auto Character::do_arena_info(QStringList arguments) -> ReturnValue
{
  sendln(u"Arena info:"_s);
  return ReturnValue();
}

auto Character::do_arena_start(QStringList arguments) -> ReturnValue
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
          arena.hplimit = dc_atoi(arg5);
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
  ch->sendln(u"The Arena has been opened for the specified levels."_s);
*/
  sendln(u"Arena start:"_s);
  return ReturnValue();
}

auto Character::do_arena_join(QStringList arguments) -> ReturnValue
{
  sendln(u"Arena join:"_s);
  return ReturnValue();
}

auto Character::do_arena_cancel(QStringList arguments) -> ReturnValue
{
  sendln(u"Arena cancel:"_s);
  return ReturnValue();
}

auto Character::do_arena_usage(QStringList arguments) -> ReturnValue
{
  sendln(u"Usage:"_s);
  sendln(u"arena info          - Shows current arena status"_s);
  sendln(u"arena start         - Start an arena open to anyone for free"_s);
  sendln(u"arena start # gold  - Start an arena open to anyone whom pays # gold. Winner takes all."_s);
  sendln(u"arena join          - Join an arena that's free to play"_s);
  sendln(u"arena join # gold   - Join an arena by paying the entrance fee of # gold"_s);
  sendln(u"arena cancel        - Cancel an arena"_s);

  return ReturnValue();
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

ReturnValue Character::do_edit_generic_show(QString value, QString fieldname, QString desc, QStringList arguments, cmd_t cmd)
{
  sendln(u"$3%1$R: %2"_s.arg(desc).arg(value));

  auto command = dc_->CMD_.find(cmd);
  if (command)
    sendln(u"$3Syntax$R: %1 [vnum] %2 <value>"_s.arg(command->name()).arg(fieldname));

  return ReturnValue::eSUCCESS;
}

bool Object::isPortal(void)
{
  return flags_.type_flag == ITEM_PORTAL;
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

qint32 dc_fprintf(auto &stream, const QString format, ...)
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

Character::Character(QObject *parent) : QObject(parent), dc_(qobject_cast<DC *>(parent))
{
}

Connection::Connection(QObject *parent) : QObject(parent), dc_(qobject_cast<DC *>(parent))
{
}

RoomDirection::RoomDirection(QObject *parent) : QObject(parent), dc_(qobject_cast<DC *>(parent))
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

Path::Path(QObject *parent)
    : QObject(parent), dc_(qobject_cast<DC *>(parent))
{
}

cDeck::cDeck(QObject *parent)
    : QObject(parent), dc_(qobject_cast<DC *>(parent))
{
}

const QStringList cond_colorcodes = {
    BOLD + GREEN,
    GREEN,
    BOLD + YELLOW,
    YELLOW,
    RED,
    BOLD + RED,
    BOLD + GREY};