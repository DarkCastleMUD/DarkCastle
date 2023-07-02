/*
 * Copyright 2017-2023 Jared H. Hudson
 * Licensed under the LGPL.
 */
#ifndef DC_H_
#define DC_H_

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

#include "typedefs.h"
#include "obj.h"
#include "character.h"

#include "character.h"
#include "fileinfo.h"
#include "connect.h"
#include "Trace.h"
#include "connect.h"
#include "SSH.h"
#include "weather.h"
#include "Zone.h"
#include "Shops.h"

using namespace std;

class DC : public QCoreApplication
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
    string dir = DFLT_DIR;
    string leaderboard_check;
    QString implementer;
  } cf;

  static constexpr room_t SORPIGAL_BANK_ROOM = 3005;
  static constexpr room_t NOWHERE = 0;
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

  Connection *descriptor_list = nullptr; /* master desc list */
  server_descriptor_list_t server_descriptor_list;
  client_descriptor_list_t client_descriptor_list;
  character_list_t character_list;
  death_list_t death_list;
  obj_list_t active_obj_list;
  obj_list_t obj_free_list;
  unordered_set<Character *> shooting_list;
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

  static string getVersion();
  static string getBuildTime();
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

  explicit DC(int &argc, char **argv);
  void main_loop2(void);
  void removeDead(void);
  void handleShooting(void);
  void init_game(void);
  void boot_db(void);
  void boot_zones(void);
  void boot_world(void);
  void write_one_zone(FILE *fl, zone_t zone_key);
  zone_t read_one_zone(FILE *fl);
  bool read_one_room(QTextStream &, room_t &room_nr);
  void load_hints(void);
  void save_hints(void);
  void send_hint(void);
  void assign_mobiles(void);
  bool authenticate(QString username, QString password, level_t level = 0);
  bool authenticate(const QHttpServerRequest &request, level_t level = 0);
  void sendAll(QString message);

private:
  DC(const DC &) = delete; // non-copyable
  DC(DC &&) = delete;      // and non-movable
  static string version;
  struct timeval last_time = {}, delay_time = {}, now_time = {};
  hints_t hints;
  const QString HINTS_FILE_NAME = "playerhints.txt";

  // as there is only one object, assignment would always be assign to self
  DC &operator=(const DC &) = delete;
  DC &operator=(DC &&) = delete;

  void game_loop_init(void);
  void game_loop(void);
  int init_socket(in_port_t port);
  Shops shops_;
};

extern vector<string> continent_names;
extern class CVoteData *DCVote;
extern class Room **world_array;
extern class Object *object_list;
extern struct spell_info_type spell_info[];
void renum_world(void);
void renum_zone_table(void);

#endif
