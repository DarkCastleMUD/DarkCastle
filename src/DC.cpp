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
  return string(VERSION);
}

string DC::getBuildTime(void)
{
  return string(BUILD_TIME);
}
