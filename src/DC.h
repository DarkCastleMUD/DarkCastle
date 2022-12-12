/*
 * DC.h
 *
 *  Created on: Dec 24, 2017
 *      Author: jhhudso
 *      Based on http://www.cplusplus.com/forum/beginner/152735/#msg792909
 */

#ifndef SRC_INCLUDE_DC_H_
#define SRC_INCLUDE_DC_H_

#include <set>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <netinet/in.h>
#include <string>

#include <QSharedPointer>
#include <QCoreApplication>
#include <QMap>
#include <QString>

typedef uint64_t vnum_t;
typedef QMap<QString, bool> joining_t;

#include "character.h"
#include "obj.h"
#include "character.h"
#include "fileinfo.h"
#include "connect.h"
#include "Trace.h"
#include "connect.h"
#include "SSH.h"

using namespace std;

using special_function = int (*)(char_data *, struct obj_data *, int, const char *, char_data *);

typedef set<char_data *> character_list_t;
typedef set<struct obj_data *> obj_list_t;
typedef set<int> client_descriptor_list_t;
typedef set<int> server_descriptor_list_t;
typedef vector<in_port_t> port_list_t;
typedef set<char_data *>::iterator character_list_i;
typedef set<int>::iterator client_descriptor_list_i;
typedef set<int>::iterator server_descriptor_list_i;
typedef vector<in_port_t>::iterator port_list_i;
typedef unordered_map<char_data *, Trace> death_list_t;
typedef unordered_map<char_data *, Trace> free_list_t;
typedef uint64_t zone_t;
typedef uint64_t room_t;
typedef map<vnum_t, special_function> special_function_list_t;

class DC : public QCoreApplication
{
  Q_OBJECT
  // Favor reference semantics over pointer semantics
public:
  void main_loop2(void);
  explicit DC(int &argc, char **argv);

  void removeDead(void);
  void handleShooting(void);

  static string getVersion();
  static string getBuildTime();
  static DC *getInstance();

  server_descriptor_list_t server_descriptor_list;
  client_descriptor_list_t client_descriptor_list;
  character_list_t character_list;

  death_list_t death_list;
  obj_list_t active_obj_list;
  obj_list_t obj_free_list;
  unordered_set<char_data *> shooting_list;
  special_function_list_t mob_non_combat_functions;
  special_function_list_t mob_combat_functions;
  special_function_list_t obj_non_combat_functions;
  special_function_list_t obj_combat_functions;

  free_list_t free_list;
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
  } cf;

  SSH::SSH ssh;
  fd_set input_set = {};
  fd_set output_set = {};
  fd_set exc_set = {};
  fd_set null_set = {};

  void init_game(void);

private:
  DC(const DC &) = delete; // non-copyable
  DC(DC &&) = delete;      // and non-movable

  static string version;
  struct timeval last_time = {}, delay_time = {}, now_time = {};
  int32_t secDelta = {}, usecDelta = {};

  // as there is only one object, assignment would always be assign to self
  DC &operator=(const DC &) = delete;
  DC &operator=(DC &&) = delete;

  void game_loop_init(void);
  void game_loop(void);
  int init_socket(in_port_t port);
};

extern struct descriptor_data *descriptor_list;
extern vector<string> continent_names;
extern class CVoteData *DCVote;
extern struct room_data **world_array;
extern struct obj_data *object_list;
extern struct zone_data *zone_table;
extern struct spell_info_type spell_info[];
void boot_zones(void);
void boot_world(void);
void renum_world(void);
void renum_zone_table(void);

#endif /* SRC_INCLUDE_DC_H_ */
