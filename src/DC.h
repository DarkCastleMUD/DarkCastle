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

#include <QSharedPointer>
#include <QCoreApplication>
#include <QMap>
#include <QString>
#include <QtHttpServer/QHttpServer>
#include <QtConcurrent/QtConcurrent>
#include "DC_global.h"

typedef uint64_t vnum_t;
typedef quint64 level_t;
typedef qint64 level_diff_t;
typedef QMap<QString, bool> joining_t;

typedef QList<QString> hints_t;
#include "connect.h"
#include "character.h"
#include "obj.h"
#include "fileinfo.h"
#include "Trace.h"
#include "SSH.h"
#include "weather.h"
#include "Zone.h"
#include "Shops.h"
#include "room.h"
#include "Database.h"

class Connection;

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

class World
{
public:
  Room &operator[](room_t room_key);
};

class DC_EXPORT DC : public QCoreApplication
{
  Q_OBJECT
  // Favor reference semantics over pointer semantics
public:
  struct config
  {
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
  static Object *getObject(vnum_t vnum);
  static bool isSet(auto flag, auto bit) { return flag & bit; };

  explicit DC(int &argc, char **argv);
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

  QRandomGenerator random_;
  QMap<uint64_t, Shop> shop_index;
  CVoteData DCVote;

private:
  static const QString build_version_;
  static const QString build_time_;
  struct timeval last_time_ = {}, delay_time_ = {}, now_time_ = {};
  hints_t hints_;
  Shops shops_;
  QList<QHostAddress> host_list_ = {QHostAddress("127.0.0.1")};
  Database database_;

  void game_loop_init(void);
  void game_loop(void);
  int init_socket(in_port_t port);
};

extern std::vector<std::string> continent_names;

extern class Object *object_list;
extern struct spell_info_type spell_info[];
void renum_world(void);
void renum_zone_table(void);

union varg_t
{
  Character *ch;
  clan_t clan;
  struct player_data *player;
  struct table_data *table;
  struct machine_data *machine;
  struct wheel_data *wheel;
};

typedef void TIMER_FUNC(varg_t arg1, void *arg2, void *arg3);

struct timer_data
{
  int timeleft;
  struct timer_data *next;
  varg_t arg1;
  QVariant var_arg1;
  void *arg2;
  void *arg3;
  TIMER_FUNC *function;
};

void clear_hunt(varg_t arg1, void *arg2, void *arg3);
void clear_hunt(varg_t arg1, Character *arg2, void *arg3);
typedef int command_return_t;
typedef int (*command_gen1_t)(Character *ch, char *argument, int cmd);
typedef command_return_t (*command_gen2_t)(Character *ch, std::string argument, int cmd);
typedef command_return_t (Character::*command_gen3_t)(QStringList arguments, int cmd);
typedef int (*command_special_t)(Character *ch, int cmd, char *arg);

#endif
