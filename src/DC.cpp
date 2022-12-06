/*
 * DC.cpp
 *
 *  Created on: Dec 24, 2017
 *      Author: jhhudso
 *      Based on http://www.cplusplus.com/forum/beginner/152735/#msg792909
 */

#include <assert.h>

#include "DC.h"
#include "room.h" // NOWHERE
#include "db.h"
#include "version.h"

string DC::version = VERSION;

DC::DC(int &argc, char **argv)
	: QCoreApplication(argc, argv), ssh(this)
{
	// ssh = QSharedPointer<SSH::SSH>(new SSH::SSH);
}

void DC::removeDead(void)
{

	for (auto &node : death_list)
	{
		character_list.erase(node.first);
		shooting_list.erase(node.first);
		Trace &t = node.second;
		t.addTrack("DC::removeDeath");
		free_char(node.first, t);
	}
	death_list.clear();

	while (!obj_free_list.empty())
	{
		obj_data *obj = *(obj_free_list.cbegin());
		active_obj_list.erase(obj);
		obj_free_list.erase(obj);
		delete obj;
	}
}

void DC::handleShooting(void)
{
	unordered_set<char_data *> remove_list;

	for (auto &ch : shooting_list)
	{
		// ignore the dead
		if (ch->in_room == NOWHERE)
		{
			continue;
		}

		if (ch->shotsthisround)
		{
			ch->shotsthisround--;
		}
		else
		{
			remove_list.insert(ch);
		}
	}

	for (auto &ch : remove_list)
	{
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

DC *DC::getInstance(void)
{
	DC *dc = dynamic_cast<DC *>(DC::instance());
	assert(dc != nullptr);
	return dc;
}