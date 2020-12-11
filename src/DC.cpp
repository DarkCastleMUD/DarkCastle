/*
 * DC.cpp
 *
 *  Created on: Dec 24, 2017
 *      Author: jhhudso
 *      Based on http://www.cplusplus.com/forum/beginner/152735/#msg792909
 */

#include "DC.h"
#include "room.h" // NOWHERE
#include "db.h"

string DC::version = VERSION;

DC::DC() {

}

DC& DC::instance()
{
	static DC DC_instance { /* ... */};
	return DC_instance;
}

void DC::removeDead(void) {
	char_data *ch = 0;
	while (!death_list.empty()) {
		ch = death_list.front();
		character_list.erase(ch);
		shooting_list.erase(ch);
		free_char(ch);
		death_list.pop();
	}

	while (!obj_free_list.empty()) {
	  obj_data *obj = *(obj_free_list.cbegin());
	  active_obj_list.erase(obj);
	  obj_free_list.erase(obj);
    delete obj;
	}
}

void DC::handleShooting(void) {
	unordered_set<char_data *> remove_list;

	for (auto &ch : shooting_list) {
		// ignore the dead
		if (ch->in_room == NOWHERE) {
			continue;
		}

		if (ch->shotsthisround) {
			ch->shotsthisround--;
		} else {
			remove_list.insert(ch);
		}
	}

	for (auto &ch : remove_list) {
		shooting_list.erase(ch);
	}
}

string DC::getVersion(void)
{
  return version;
}

string DC::getBuildTime(void)
{
  return string(BUILD_TIME);
}
