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
#include <netinet/in.h>
#include <string>

#include "character.h"
#include "obj.h"
#include "character.h"
#include "fileinfo.h"
#include "connect.h"

#include "Trace.h"

using namespace std;

typedef set<char_data *> character_list_t;
typedef set<obj_data*> obj_list_t;
typedef set<int> client_descriptor_list_t;
typedef set<int> server_descriptor_list_t;
typedef vector<in_port_t> port_list_t;
typedef set<char_data *>::iterator character_list_i;
typedef set<int>::iterator client_descriptor_list_i;
typedef set<int>::iterator server_descriptor_list_i;
typedef vector<in_port_t>::iterator port_list_i;
typedef unordered_map<char_data*, Trace> death_list_t;
typedef unordered_map<char_data *, Trace> free_list_t;
class DC {
  // Favor reference semantics over pointer semantics
public:
  DC();
  void removeDead(void);
  void handleShooting(void);

  static DC& instance();
  static string getVersion();
  static string getBuildTime();

  server_descriptor_list_t server_descriptor_list;
  client_descriptor_list_t client_descriptor_list;
  character_list_t character_list;

  death_list_t death_list;
  obj_list_t active_obj_list;
  obj_list_t obj_free_list;
  unordered_set<char_data*> shooting_list;
  
	free_list_t free_list;
  struct config {
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
    string dir = DFLT_DIR;
    string leaderboard_check;
  } cf;

  void init_game(void);

private:

  DC(const DC&) = delete; // non-copyable
  DC(DC&&) = delete; // and non-movable

  static string version;

  // as there is only one object, assignment would always be assign to self
  DC& operator=(const DC&) = delete;
  DC& operator=(DC&&) = delete;

  void game_loop(void);
  int init_socket(in_port_t port);
};

extern descriptor_data *descriptor_list;
extern vector<string> continent_names;
extern CVoteData *DCVote;
extern struct room_data ** world_array;
extern struct obj_data  *object_list;
extern struct zone_data *zone_table;
extern struct spell_info_type spell_info [ ];

#endif /* SRC_INCLUDE_DC_H_ */
