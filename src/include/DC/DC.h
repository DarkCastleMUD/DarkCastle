/*
 * Copyright 2017-2023 Jared H. Hudson
 * Licensed under the LGPL.
 */
#ifndef DC_H_
#define DC_H_

#define SAVE_DIR "../save"
#define BSAVE_DIR "../bsave"
#define QSAVE_DIR "../save/qdata"
#define NEWSAVE_DIR "../newsave"
#define ARCHIVE_DIR "../archive"
#define MOB_DIR "../MOBProgs/"
#define BAN_FILE "banned.txt"
#define SHOP_DIR "../lib/shops"
#define PLAYER_SHOP_DIR "../lib/playershops"
#define FORBIDDEN_NAME_FILE "../lib/forbidden_names.txt"
#define SKILL_QUEST_FILE "../lib/skill_quests.txt"
#define FAMILIAR_DIR "../familiar"
#define FOLLOWER_DIR "../follower"
#define VAULT_DIR "../vaults"

// TODO - Remove tinyworld.shp and divide the stops up into some meaningful
//        format in their own directory like the world/mob/obj files
#define SHOP_FILE "tinyworld.shp"

#define WEBPAGE_FILE "webresponse.txt"
#define GREETINGS1_FILE "greetings1.txt"
#define GREETINGS2_FILE "greetings3.txt"
#define GREETINGS3_FILE "greetings4.txt"
#define GREETINGS4_FILE "greetings5.txt"
#define CREDITS_FILE "credits.txt"
#define MOTD_FILE "../lib/motd.txt"
#define IMOTD_FILE "motdimm.txt"
#define STORY_FILE "story.txt"
#define TIME_FILE "time.txt"
#define IDEA_LOG "ideas.log"
#define TYPO_LOG "typos.log"
#define MESS_FILE "messages.txt"
#define MESS2_FILE "messages2.txt"
#define SOCIAL_FILE "social.txt"
#define HELP_KWRD_FILE "help_key.txt"
#define HELP_PAGE_FILE "help.txt"
#define INFO_FILE "info.txt"
#define LOCAL_WHO_FILE "onlinewho.txt"

#define WEB_WHO_FILE "/srv/www/www.dcastle.org/htdocs/onlinewho.txt"
#define WEB_AUCTION_FILE "/srv/www/www.dcastle.org/htdocs/auctions.txt"
#define NEW_HELP_FILE "new_help.txt"
#define WEB_HELP_FILE "/srv/www/www.dcastle.org/htdocs/webhelp.txt"
#define NEW_HELP_PAGE_FILE "new_help_screen.txt"
#define NEW_IHELP_PAGE_FILE "new_ihelp_screen.txt"
#define LEADERBOARD_FILE "leaderboard.txt"
#define QUEST_FILE "quests.txt"
#define WEBCLANSLIST_FILE "webclanslist.txt"
#define HTDOCS_DIR "/srv/www/www.dcastle.org/htdocs/"

#define PLAYER_DIR "player/"
#define BUG_LOG "bug.log"
#define GOD_LOG "god.log"
#define MORTAL_LOG "mortal.log"
#define SOCKET_LOG "socket.log"
#define PLAYER_LOG "player.log"
#define WORLD_LOG "world.log"
#define ARENA_LOG "arena.log"
#define CLAN_LOG "clan.log"
#define OBJECTS_LOG "objects.log"
#define QUEST_LOG "quest.log"
#define VAULT_LOG "vault.log"

#define WORLD_INDEX_FILE "worldindex"
#define OBJECT_INDEX_FILE "objectindex"
#define MOB_INDEX_FILE "mobindex"
#define ZONE_INDEX_FILE "zoneindex"
#define PLAYER_SHOP_INDEX "playershopindex"

#define OBJECT_INDEX_FILE_TINY "objectindex.tiny"
#define WORLD_INDEX_FILE_TINY "worldindex.tiny"
#define MOB_INDEX_FILE_TINY "mobindex.tiny"
#define ZONE_INDEX_FILE_TINY "zoneindex.tiny"

#define VAULT_INDEX_FILE "../vaults/vaultindex"
#define VAULT_INDEX_FILE_TMP "../vaults/vaultindex.tmp"

#include <set>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <netinet/in.h>
#include <string>
#include <map>
#include <expected>

#include <QSharedPointer>
#include <QCoreApplication>
#include <QMap>
#include <QString>
#include <QtHttpServer/QHttpServer>
#include <QtConcurrent/QtConcurrent>
#include "DC/DC_global.h"

typedef uint64_t vnum_t;
typedef qint64 level_diff_t;
typedef QMap<QString, bool> joining_t;

typedef QList<QString> hints_t;
#include "DC/levels.h"
#include "DC/connect.h"
#include "DC/character.h"
#include "DC/obj.h"
#include "DC/fileinfo.h"
#include "DC/Trace.h"
#include "DC/SSH.h"
#include "DC/weather.h"
#include "DC/Zone.h"
#include "DC/Shops.h"
#include "DC/room.h"
#include "DC/Database.h"
#include "DC/interp.h"
#include "DC/Command.h"
#include "DC/clan.h"

class Connection;
class index_data;

using special_function = int (*)(Character *, class Object *, int, const char *, Character *);
void close_file(std::FILE *fp);
using unique_file_t = std::unique_ptr<std::FILE, decltype(&close_file)>;

typedef std::set<Character *> character_list_t;
typedef std::set<class Object *> obj_list_t;
typedef std::set<int> client_descriptor_list_t;
typedef std::set<int> server_descriptor_list_t;
typedef std::vector<in_port_t> port_list_t;
typedef std::set<Character *>::iterator character_list_i;
typedef std::set<int>::iterator client_descriptor_list_i;
typedef std::set<int>::iterator server_descriptor_list_i;
typedef std::vector<in_port_t>::iterator port_list_i;
typedef std::unordered_map<Character *, Trace> death_list_t;
typedef std::unordered_map<Character *, Trace> free_list_t;
typedef uint64_t zone_t;
typedef uint64_t room_t;
typedef uint64_t gold_t;
typedef std::map<vnum_t, special_function> special_function_list_t;
// class Zone;
typedef QMap<zone_t, Zone> zones_t;

/* element in monster and object index-tables   */
class index_data
{
public:
  int virt{};                                                                            /* virt number of ths mob/obj           */
  int number{};                                                                          /* number of existing units of ths mob/obj */
  int (*non_combat_func)(Character *, class Object *, int, const char *, Character *){}; // non Combat special proc
  int (*combat_func)(Character *, class Object *, int, const char *, Character *){};     // combat special proc
  void *item{};                                                                          /* the mobile/object itself                 */

  struct mob_prog_data *mobprogs{};
  mob_prog_data *mobspec{};
  int progtypes{};
};

class World
{
public:
  Room &operator[](room_t room_key);
};

class bestowable_god_commands_type
{
public:
  QString name{}; // name of command
  int16_t num{};  // ID # of command
  bool testcmd{}; // true = test command, false = normal command
};

class command_lag
{
public:
  command_lag *next;
  class Character *ch;
  int cmd_number;
  int lag;
};

class Arena
{
public:
  static constexpr room_t ARENA_LOW = 14600;
  static constexpr room_t ARENA_HIGH = 14680;
  static constexpr room_t ARENA_DEATHTRAP = 14680;

  enum class Types
  {
    NORMAL,
    CHAOS,
    POTATO,
    PRIZE,
    HP,
    PLAYER_FREE,
    PLAYER_NOT_FREE
  };

  enum class Statuses
  {
    CLOSED,
    OPENED
  };

  auto Low(void) const -> level_t { return low_; }
  auto High(void) const -> level_t { return high_; }
  auto Number(void) const -> quint64 { return number_; }
  auto CurrentNumber(void) const -> quint64 { return current_number_; }
  auto IncrementCurrentNumber(void) -> void { current_number_++; }
  auto HPLimit(void) const -> quint64 { return hp_limit_; }
  auto Type(void) const -> Types { return type_; }
  auto Status(void) const -> Statuses { return status_; }
  auto EntryFee(void) const -> gold_t { return entry_fee_; }

  auto isNormal(void) const -> bool { return Type() == Types::NORMAL; }
  auto isChaos(void) const -> bool { return Type() == Types::CHAOS; }
  auto isPotato(void) const -> bool { return Type() == Types::POTATO; }
  auto isPrize(void) const -> bool { return Type() == Types::PRIZE; }
  auto isHP(void) const -> bool { return Type() == Types::HP; }
  auto isPlayerFree(void) const -> bool { return Type() == Types::PLAYER_FREE; }
  auto isPlayerNotFree(void) const -> bool { return Type() == Types::PLAYER_NOT_FREE; }

  auto isOpened(void) const -> bool { return Status() == Statuses::OPENED; }
  auto isClosed(void) const -> bool { return Status() == Statuses::CLOSED; }

private:
  level_t low_{};
  level_t high_{};
  quint64 number_{};
  quint64 current_number_{};
  quint64 hp_limit_{};
  Types type_{};
  Statuses status_{};
  gold_t entry_fee_{};
};

void logentry(QString str, uint64_t god_level = 0, LogChannels type = LogChannels::LOG_MISC, Character *vict = nullptr);

#define auction_duration 1209600
#define AUC_MIN_PRICE 1000
#define AUC_MAX_PRICE 2000000000

struct AuctionTicket;
Object *ticket_object_load(QMap<unsigned int, AuctionTicket>::iterator Item_it, int ticket);

enum ListOptions
{
  LIST_ALL = 0,
  LIST_MINE,
  LIST_PRIVATE,
  LIST_BY_NAME,
  LIST_BY_LEVEL,
  LIST_BY_SLOT,
  LIST_BY_SELLER,
  LIST_BY_CLASS,
  LIST_BY_RACE,
  LIST_RECENT
};

enum AuctionStates
{
  AUC_FOR_SALE = 0,
  AUC_EXPIRED,
  AUC_SOLD,
  AUC_DELETED
};

/*
TICKET STRUCT
*/
struct AuctionTicket
{
  int vitem;
  QString item_name;
  unsigned int price;
  QString seller;
  QString buyer;
  AuctionStates state;
  unsigned int end_time;
  Object *obj;
};
class Character;
class AuctionHouse
{
public:
  AuctionHouse(QString in_file);
  AuctionHouse();
  ~AuctionHouse();
  void CollectTickets(Character *ch, unsigned int ticket = 0);
  void CancelAll(Character *ch);
  void AddItem(Character *ch, Object *obj, unsigned int price, QString buyer);
  void RemoveTicket(Character *ch, unsigned int ticket);
  void BuyItem(Character *ch, unsigned int ticket);
  void ListItems(Character *ch, ListOptions options, QString name, unsigned int to, unsigned int from);
  void CheckExpire();
  void Identify(Character *ch, unsigned int ticket);
  void AddRoom(Character *ch, int room);
  void RemoveRoom(Character *ch, int room);
  void ListRooms(Character *ch);
  void HandleRename(Character *ch, QString old_name, QString new_name);
  void HandleDelete(QString name);
  void CheckForSoldItems(Character *ch);
  bool IsAuctionHouse(int room);
  void DoModify(Character *ch, unsigned int ticket, unsigned int new_price);
  void ShowStats(Character *ch);
  void Save();
  void Load();
  [[nodiscard]] unsigned int getItemsPosted(void) { return ItemsPosted; }
  void setItemsPosted(unsigned int items_posted) { ItemsPosted = items_posted; }

private:
  unsigned int ItemsPosted;
  unsigned int ItemsExpired;
  unsigned int ItemsSold;
  unsigned int TaxCollected;
  unsigned int Revenue;
  unsigned int UncollectedGold;
  unsigned int ItemsActive;
  void ParseStats();
  bool CanSellMore(Character *ch);
  bool IsOkToSell(Object *obj);
  bool IsWearable(Character *ch, int vnum);
  bool IsNoTrade(int vnum);
  bool IsSeller(QString in_name, QString seller);
  bool IsExist(QString name, int vnum);
  bool IsClass(int vnum, QString isclass);
  bool IsRace(int vnum, QString israce);
  bool IsName(QString name, int vnum);
  bool IsSlot(QString slot, int vnum);
  bool IsLevel(unsigned int to, unsigned int from, int vnum);
  QMap<int, int> auction_rooms;
  unsigned int cur_index;
  QString file_name;
  QMap<unsigned int, AuctionTicket> Items_For_Sale;
};

struct wizlist_info
{
  QString name;
  level_t level = {};
};
class DC_EXPORT DC : public QCoreApplication
{
  Q_OBJECT
  // Favor reference semantics over pointer semantics
public:
  struct config
  {
    config(int argc = {}, char **argv = {})
        : argc_(argc), argv_(argv) {}
    int argc_{};
    char **argv_{};
    bool sql = true;
    port_list_t ports;
    bool allow_imp_password = false;
    bool verbose_mode = false;
    bool test_mobs = false;
    bool test_objs = false;
    bool test_world = false;
    bool bport = false;
    bool check_syntax = false;
    bool stderr_timestamp = true;
    bool allow_multi = false;
    bool allow_newstatsys = false;
    bool testing = false;
    QString library_directory = DEFAULT_LIBRARY_PATH;
    QString leaderboard_check;
    QString implementer;
  } cf;

  static constexpr room_t SORPIGAL_BANK_ROOM = 3005;
  static constexpr room_t NOWHERE = 0ULL;
  static constexpr uint64_t PASSES_PER_SEC = 100;
  static constexpr uint64_t PULSE_TIMER = 1 * PASSES_PER_SEC;
  static constexpr uint64_t PULSE_MOBILE = 4 * PASSES_PER_SEC;
  static constexpr uint64_t PULSE_OBJECT = 4 * PASSES_PER_SEC;
  static constexpr uint64_t PULSE_VIOLENCE = 2 * PASSES_PER_SEC;
  static constexpr uint64_t PULSE_BARD = 1 * PASSES_PER_SEC;
  static constexpr uint64_t PULSE_TENSEC = 10 * PASSES_PER_SEC;
  static constexpr uint64_t PULSE_WEATHER = 45 * PASSES_PER_SEC;
  static constexpr uint64_t PULSE_TIME = 60 * PASSES_PER_SEC;
  static constexpr uint64_t PULSE_REGEN = 15 * PASSES_PER_SEC;
  static constexpr uint64_t PULSE_SHORT = 1; // Pulses all the time.
  static constexpr level_t MAX_MORTAL_LEVEL = 60ULL;
  static const QString HINTS_FILE_NAME;
  static const QString DEFAULT_LIBRARY_PATH;
  static const QStringList connected_states;
  static QList<bestowable_god_commands_type> bestowable_god_commands;

  Connection *descriptor_list = nullptr; /* master desc list */
  server_descriptor_list_t server_descriptor_list;
  client_descriptor_list_t client_descriptor_list;
  character_list_t character_list;
  death_list_t death_list;
  obj_list_t active_obj_list;
  obj_list_t obj_free_list;
  std::unordered_set<Character *> shooting_list;
  special_function_list_t mob_non_combat_functions;
  special_function_list_t mob_combat_functions;
  special_function_list_t obj_non_combat_functions;
  special_function_list_t obj_combat_functions;
  free_list_t free_list;
  SSH::SSH ssh;
  fd_set input_set = {};
  fd_set output_set = {};
  fd_set exc_set = {};
  fd_set null_set = {};
  zones_t zones = {};
  QMap<room_t, Room> rooms;
  class World world;
  clan_data *clan_list{};
  clan_data *end_clan_list{};
  class index_data obj_index_array[MAX_INDEX] = {};
  class index_data *obj_index = obj_index_array;

  class index_data mob_index_array[MAX_INDEX] = {};
  class index_data *mob_index = mob_index_array;

  int top_of_helpt = 0;          /* top of help index table         */
  int new_top_of_helpt = 0;      /* top of help index table         */
  room_t top_of_world_alloc = 0; // index of last alloc'd memory in world
  room_t top_of_world = 0;
  int total_rooms = 0; /* total amount of rooms in memory */
  AuctionHouse TheAuctionHouse;
  QList<struct wizlist_info> wizlist; /* the actual wizlist            */

  static QString getBuildVersion();
  static QString getBuildTime();
  static DC *getInstance();
  static zone_t getRoomZone(room_t room_nr);
  static QString getZoneName(zone_t zone_key);
  static void setZoneClanOwner(zone_t zone_key, int clan_key);
  static void setZoneClanGold(zone_t zone_key, gold_t gold);
  static void setZoneTopRoom(zone_t zone_key, room_t room_key);
  static void setZoneBottomRoom(zone_t zone_key, room_t room_key);
  static void setZoneModified(zone_t zone_key);
  static void setZoneNotModified(zone_t zone_key);
  static void incrementZoneDiedTick(zone_t zone_key);
  static void resetZone(zone_t zone_key, Zone::ResetType reset_type = Zone::ResetType::normal);
  Object *getObject(vnum_t vnum);
  void findLibrary(void);
  int create_one_room(Character *ch, int vnum);
  void update_wizlist(Character *ch);
  void do_godlist(void);
  void write_wizlist(std::stringstream &filename);
  void write_wizlist(std::string filename);
  void write_wizlist(const char filename[]);
  explicit DC(int &argc, char **argv);
  explicit DC(config c);
  void setup(void);
  DC(const DC &) = delete; // non-copyable
  DC(DC &&) = delete;      // and non-movable
  DC &operator=(const DC &) = delete;
  DC &operator=(DC &&) = delete;
  void main_loop2(void);
  void removeDead(void);
  void handleShooting(void);
  void init_game(void);
  void boot_db(void);
  void boot_zones(void);
  void boot_world(void);
  void write_one_zone(FILE *fl, zone_t zone_key);
  zone_t read_one_zone(FILE *fl);
  int read_one_room(FILE *fl, int &room_nr);
  void load_hints(void);
  void save_hints(void);
  void send_hint(void);
  void assign_mobiles(void);
  bool authenticate(QString username, QString password, uint64_t level = 0);
  bool authenticate(const QHttpServerRequest &request, uint64_t level = 0);
  void sendAll(QString message);
  bool isAllowedHost(QHostAddress host);
  Database getDatabase(void) { return database_; }
  Database db(void) { return database_; }
  command_lag *getCommandLag(void) const { return command_lag_list_; }
  void setCommandLag(command_lag *cl) { command_lag_list_ = cl; }

  [[nodiscard]] inline QString currentType(void) { return current_type_; }
  void currentType(QString current_type) { current_type_ = current_type; }

  [[nodiscard]] inline QString currentName(void) { return current_name_; }
  void currentName(QString current_name) { current_name_ = current_name; }

  [[nodiscard]] inline vnum_t currentVNUM(void) { return current_VNUM_; }
  void currentVNUM(vnum_t current_VNUM) { current_VNUM_ = current_VNUM; }

  [[nodiscard]] inline QString currentFilename(void) { return current_filename_; }
  void currentFilename(QString current_filename) { current_filename_ = current_filename; }

  void current(QString current_type, QString current_name, vnum_t current_VNUM, QString current_filename)
  {
    currentType(current_type);
    currentName(current_name);
    currentVNUM(current_VNUM);
    currentFilename(current_filename);
  }

  [[nodiscard]] inline QString current(void)
  {
    return QStringLiteral("%1:%2:%3:%4").arg(currentType()).arg(currentName()).arg(currentVNUM()).arg(currentFilename());
  }

  void logverbose(QString str, uint64_t god_level = 0, LogChannels type = LogChannels::LOG_MISC, Character *vict = nullptr)
  {
    if (cf.verbose_mode)
    {
      logentry(str, god_level, type, vict);
    }
  }

  void clean_socials_from_memory(void);
  void remove_all_mobs_from_world(void);
  void remove_all_objs_from_world(void);
  void free_zones_from_memory(void);
  void free_clans_from_memory(void);
  void free_world_from_memory(void);
  void free_mobs_from_memory(void);
  void free_objs_from_memory(void);
  void free_messages_from_memory(void);
  void free_hsh_tree_from_memory(void);
  void free_help_from_memory(void);
  void free_emoting_objects_from_memory(void);
  void free_game_portals_from_memory(void);
  void free_ban_list_from_memory(void);
  void free_buff_pool_from_memory(void);
  void load_vaults(void);
  void testing_load_vaults(void);
  void reload_vaults(void);
  void load_corpses(void);
  int write_hotboot_file(void);
  int load_hotboot_descs(void);

  ~DC(void)
  {
    remove_all_mobs_from_world();
    remove_all_objs_from_world();
    clean_socials_from_memory();
    free_zones_from_memory();
    free_clans_from_memory();
    free_world_from_memory();
    free_mobs_from_memory();
    free_objs_from_memory();
    free_messages_from_memory();
    free_hsh_tree_from_memory();
    wizlist.clear();
    free_help_from_memory();
    shop_index.clear();
    free_emoting_objects_from_memory();
    free_game_portals_from_memory();
    free_ban_list_from_memory();
    free_buff_pool_from_memory();
    removeDead();
  }

  QRandomGenerator random_;
  QMap<uint64_t, Shop> shop_index;
  CVoteData DCVote;

  QString last_processed_cmd = {};
  QString last_char_name = {};
  room_t last_char_room = {};
  Commands CMD_;
  Arena arena_;

private:
  struct timeval last_time_ = {}, delay_time_ = {}, now_time_ = {};
  hints_t hints_;
  Shops shops_;
  QList<QHostAddress> host_list_ = {QHostAddress("127.0.0.1")};
  Database database_;
  command_lag *command_lag_list_{};
  QString current_type_;
  QString current_name_;
  vnum_t current_VNUM_{};
  QString current_filename_;

  void game_loop_init(void);
  void game_loop(void);
  int init_socket(in_port_t port);
};

void produce_coredump(void *ptr = 0);

template <typename T>
T number(T from, T to, QRandomGenerator *rng = &(DC::getInstance()->random_))
{
  if (from == to)
  {
    return to;
  }

  if (from > to)
  {

    logentry(QStringLiteral("BACKWARDS usage: number(%1, %2)!").arg(from).arg(to));
    produce_coredump();
    return to;
  }

  if (std::is_unsigned<T>::value)
  {
    return rng->bounded(static_cast<quint64>(from), static_cast<quint64>(to + 1));
  }
  else if (std::is_signed<T>::value)
  {
    return rng->bounded(static_cast<qint64>(from), static_cast<qint64>(to + 1));
  }
}

extern std::vector<std::string> continent_names;

extern class Object *object_list;
extern struct spell_info_type spell_info[];
void renum_world(void);
void renum_zone_table(void);

union varg_t
{
  Character *ch;
  clan_t clan;
  class player_data *player;
  class table_data *table;
  class machine_data *machine;
  class wheel_data *wheel;
};

typedef void TIMER_FUNC(varg_t arg1, void *arg2, void *arg3);

struct timer_data
{
  int timeleft{};
  struct timer_data *next{};
  varg_t arg1{};
  QVariant var_arg1{};
  void *arg2{};
  void *arg3{};
  TIMER_FUNC *function{};
};

void clear_hunt(varg_t arg1, void *arg2, void *arg3);
void clear_hunt(varg_t arg1, Character *arg2, void *arg3);

auto get_bestow_command(QString command_name) -> std::expected<bestowable_god_commands_type, search_error>;
#define REMOVE_BIT(var, bit) ((var) = (var) & ~(bit))

auto &operator>>(auto &in, Room &room)
{
  room_t room_nr = {};
  char *temp = nullptr;
  char ch = 0;
  int dir = 0;
  struct extra_descr_data *new_new_descr{};
  zone_t zone_nr = {};

  ch = fread_char(in);

  if (ch != '$')
  {
    room_nr = fread_int(in, 0, 1000000);
    temp = fread_string(in, 0);

    if (room_nr)
    {
      /*
      DC::getInstance()->currentVNUM(room_nr);
      DC::getInstance()->currentType("Room");
      DC::getInstance()->currentName(temp);

      if (room_nr >= DC::getInstance()->top_of_world_alloc)
      {
        DC::getInstance()->top_of_world_alloc = room_nr + 200;
      }

      if (DC::getInstance()->top_of_world < room_nr)
        DC::getInstance()->top_of_world = room_nr;
      */

      room.paths = 0;
      room.number = room_nr;
      room.name = temp;
    }
    char *description = fread_string(in, 0);
    if (room_nr)
    {
      room.description = description;
      room.nTracks = 0;
      room.tracks = 0;
      room.last_track = 0;
      room.denied = 0;
      // DC::getInstance()->total_rooms++;
    }
    // Ignore recorded zone number since it may not longer be valid
    fread_int(in, -1, 64000); // zone nr

    if (room_nr)
    {
      // Go through the zone table until room.number is
      // in the current zone.

      bool found = false;
      zone_t zone_nr = {};
      for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
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
        // QString error = QStringLiteral("Room %1 is outside of any zone.").arg(room_nr);
        // logentry(error);
        // logentry(QStringLiteral("Room outside of ANY zone.  ERROR"), IMMORTAL, LogChannels::LOG_BUG);
      }
      else
      {
        auto &zone = DC::getInstance()->zones[zone_nr];
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

    uint32_t room_flags = fread_bitvector(in, -1, 2147483467);

    if (room_nr)
    {
      room.room_flags = room_flags;
      if (isSet(room.room_flags, NO_ASTRAL))
      {
        REMOVE_BIT(room.room_flags, NO_ASTRAL);
      }

      // This bitvector is for runtime and not stored in the files, so just initialize it to 0
      room.temp_room_flags = 0;
    }

    int sector_type = fread_int(in, -1, 64000);

    if (room_nr)
    {
      room.sector_type = sector_type;
      room.funct = 0;
      room.contents = 0;
      room.people = 0;
      room.light = 0; /* Zero light sources */

      for (size_t tmp = 0; tmp <= 5; tmp++)
        room.dir_option[tmp] = 0;

      room.ex_description = 0;
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

        new_new_descr = new struct extra_descr_data;
        new_new_descr->keyword = fread_string(in, 0);
        new_new_descr->description = fread_string(in, 0);

        if (room_nr)
        {
          new_new_descr->next = room.ex_description;
          room.ex_description = new_new_descr;
        }
        else
        {
          delete new_new_descr;
        }
      }
      else if (ch == 'B')
      {
        struct deny_data *deni;

        deni = new struct deny_data;
        deni->vnum = fread_int(in, -1, 2147483467);

        if (room_nr)
        {
          deni->next = room.denied;
          room.denied = deni;
        }
        else
        {
          delete deni;
        }
      }
      else if (ch == 'S') /* end of current room */
        break;
      else if (ch == 'C')
      {
        int c_class = fread_int(in, 0, CLASS_MAX);
        if (room_nr)
        {
          room.allow_class[c_class] = true;
        }
      }
    } // of for (;;) (get directions and extra descs)
  } // if == $

  return in;
}

template <typename T>
T check_returns(T in_str)
{
  T new_string;
  for (auto checker = in_str.begin(); checker != in_str.end(); checker++)
  {
    if (*checker == '\n')
    {
      if (checker + 1 != in_str.end() && *(checker + 1) != '\r')
        new_string.push_back('\r');
    }
    new_string.push_back(*checker);
  }

  return new_string;
}

#endif
