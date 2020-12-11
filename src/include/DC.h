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
#include "character.h"
#include "obj.h"


using namespace std;

typedef set<char_data*> character_list_t;
typedef set<obj_data*> obj_list_t;

class DC {
  // Favor reference semantics over pointer semantics
public:
  DC();
  void removeDead(void);
  void handleShooting(void);

  static DC& instance();
  static string getVersion();
  static string getBuildTime();

  character_list_t character_list;
  queue<char_data*> death_list;
  obj_list_t active_obj_list;
  obj_list_t obj_free_list;
  unordered_set<char_data*> shooting_list;

private:

  DC(const DC&) = delete; // non-copyable
  DC(DC&&) = delete; // and non-movable

  static string version;

  // as there is only one object, assignment would always be assign to self
  DC& operator=(const DC&) = delete;
  DC& operator=(DC&&) = delete;
};

#endif /* SRC_INCLUDE_DC_H_ */
