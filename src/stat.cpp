#
#include <cctype>
#include <cstring>

#include <string>
#include <map>
#include <queue>
#include <algorithm>
#include <list>

#include "DC/obj.h"
#include "DC/spells.h"
#include "DC/player.h"
#include "DC/terminal.h"
#include "DC/levels.h"
#include "DC/character.h"
#include "DC/room.h"
#include "DC/utility.h"
#include "cassert"
#include "DC/db.h"
#include "DC/vault.h"
#include "DC/returnvals.h"
#include "DC/interp.h"

/******************* Area start **************************************/
enum SortState
{
	SORT_XP,
	SORT_GOLD,
	SORT_MOB
};

struct MobKills
{
	unsigned int howmany;
	int name;
};

struct AreaStats
{
	zone_t area;
	int64_t xps;
	int64_t gold;
	std::vector<MobKills> mobKills;
};
typedef std::map<zone_t, AreaStats> area_stats_t;

class AreaData
{
public:
	AreaData();
	~AreaData() {}
	void GetAreaData(zone_t zone_nr, int mob, int64_t xps, int64_t gold);
	void DisplaySingleArea(Character *ch, zone_t area);
	void DisplayAreaData(Character *ch);
	void SortAreaData(Character *ch, SortState state);
	void Load() {}
	void Save() {}

private:
	area_stats_t areaStats;
};

AreaData::AreaData()
{
}
bool CompareAreaXPStats(AreaStats first, AreaStats second)
{
	return first.xps > second.xps;
}

bool CompareAreaGoldStats(AreaStats first, AreaStats second)
{
	return first.gold > second.gold;
}

void AreaData::SortAreaData(Character *ch, SortState state)
{
	std::list<AreaStats> lAreaStats;
	area_stats_t::iterator it;
	AreaStats aStats;
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	std::string output_buf;
	int i = 0;

	for (it = areaStats.begin(); it != areaStats.end(); it++)
	{
		if ((state == SORT_XP) && (it->second.xps < 1))
			continue;
		if ((state == SORT_GOLD) && (it->second.gold < 1))
			continue;
		lAreaStats.push_back(it->second);
	}
	if (state == SORT_XP)
	{
		lAreaStats.sort(CompareAreaXPStats);
		for (std::list<AreaStats>::iterator lit = lAreaStats.begin(); lit != lAreaStats.end(); lit++)
		{
			i++;
			snprintf(buf, 35 + (strlen(DC::getInstance()->zones.value(lit->area).NameC()) - nocolor_strlen(DC::getInstance()->zones.value(lit->area).NameC())), "%s", DC::getInstance()->zones.value(lit->area).NameC());
			snprintf(buf2, MAX_STRING_LENGTH, "%%3d)%%-%ds $5%%15lld$R xps\r\n", 35 + (strlen(DC::getInstance()->zones.value(lit->area).NameC()) - nocolor_strlen(DC::getInstance()->zones.value(lit->area).NameC())));
			csendf(ch, buf2, i, buf, lit->xps);
		}
	}
	if (state == SORT_GOLD)
	{
		lAreaStats.sort(CompareAreaGoldStats);
		for (std::list<AreaStats>::iterator lit = lAreaStats.begin(); lit != lAreaStats.end(); lit++)
		{
			i++;
			snprintf(buf, 35 + (strlen(DC::getInstance()->zones.value(lit->area).NameC()) - nocolor_strlen(DC::getInstance()->zones.value(lit->area).NameC())), "%s",
					 DC::getInstance()->zones.value(lit->area).NameC());
			snprintf(buf2, MAX_STRING_LENGTH, "%%3d)%%-%ds $5%%15lld$R gold\r\n",
					 35 + (strlen(DC::getInstance()->zones.value(lit->area).NameC()) - nocolor_strlen(DC::getInstance()->zones.value(lit->area).NameC())));
			csendf(ch, buf2, i, buf, lit->gold);
		}
	}
	return;
}

void AreaData::DisplaySingleArea(Character *ch, zone_t area)
{

	char buf[MAX_STRING_LENGTH];
	std::string output_buf;
	std::vector<MobKills>::iterator mobs;
	Character *get_mob_vnum(int vnum);
	Character *tmpchar;

	if (DC::getInstance()->zones.contains(area) == false)
	{
		ch->send("Area number is outside the limits\r\n");
		return;
	}
	snprintf(buf, MAX_STRING_LENGTH, "%d)%30s -- $5%12ld$R xps -- $5%12ld$R gold\n\r", area, DC::getInstance()->zones.value(area).NameC(), areaStats[area].xps, areaStats[area].gold);
	csendf(ch, buf);
	snprintf(buf, MAX_STRING_LENGTH, "%-30s %-5s\r\n", "Mob Name", "Killed");
	csendf(ch, buf);
	for (mobs = areaStats[area].mobKills.begin(); mobs != areaStats[area].mobKills.end(); mobs++)
	{
		tmpchar = get_mob_vnum(mobs->name);
		if (!tmpchar)
		{
			ch->sendln("Shit a mob is missing from game!!!");
			continue;
		}
		snprintf(buf, MAX_STRING_LENGTH, "%-30s %-5d %s\r\n", get_mob_vnum(mobs->name)->short_desc,
				 mobs->howmany, mobs->howmany < 2 ? "time" : "times");
		csendf(ch, buf);
	}
	page_string(ch->desc, output_buf.c_str(), 1);
	return;
}
void AreaData::DisplayAreaData(Character *ch)
{
	QString buf, buf2;
	for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
	{
		if (areaStats[zone_key].xps == 0)
			continue;
		buf = QStringLiteral("%%3d)%%-%1s|$5%%12lld$R xps|$5%%12lld$R gold|\n\r").arg(35 + zone.Name().length() - nocolor_strlen(zone.Name()));
		ch->send(buf.arg(zone_key).arg(zone.Name()).arg(areaStats[zone_key].xps).arg(areaStats[zone_key].gold));
	}
	return;
}
void AreaData::GetAreaData(zone_t zone, int mob, int64_t xps, int64_t gold)
{
	MobKills mobObj;
	std::vector<MobKills>::iterator mobs;

	if (areaStats.end() == areaStats.find(zone))
	{
		areaStats[zone].area = zone;
		areaStats[zone].xps = xps;
		areaStats[zone].gold = gold;
	}
	else
	{
		areaStats[zone].area = zone;
		areaStats[zone].gold += gold;
		areaStats[zone].xps += xps;
	}
	for (mobs = areaStats[zone].mobKills.begin(); mobs != areaStats[zone].mobKills.end(); mobs++)
	{
		if (mobs->name == mob)
		{
			mobs->howmany++;
			return;
		}
	}
	mobObj.name = mob;
	mobObj.howmany = 1;
	areaStats[zone].mobKills.push_back(mobObj);
	return;
}

AreaData areaData;

int do_areastats(Character *ch, char *argument, int cmd)
{
	char buf[MAX_STRING_LENGTH];

	argument = one_argument(argument, buf);
	if (!*buf)
	{
		ch->sendln("$BUsage:$R ");
		ch->sendln("$Bareastats$R ");
		ch->sendln("$Bareastats area #$R");
		ch->sendln("$Bareastats topxps$R");
		ch->sendln("$Bareastats topgold$R");
		return eSUCCESS;
	}
	if (!strcmp(buf, "area"))
	{
		argument = one_argument(argument, buf);
		if (!*buf)
		{
			ch->sendln("You need to specify a zone number.");
			return eSUCCESS;
		}
		areaData.DisplaySingleArea(ch, atoi(buf));
		return eSUCCESS;
	}
	if (!strcmp(buf, "all"))
	{
		areaData.DisplayAreaData(ch);
		return eSUCCESS;
	}
	if (!strcmp(buf, "topxps"))
	{
		areaData.SortAreaData(ch, SORT_XP);
		return eSUCCESS;
	}
	if (!strcmp(buf, "topgold"))
	{
		areaData.SortAreaData(ch, SORT_GOLD);
		return eSUCCESS;
	}
	return eSUCCESS;
}

void getAreaData(unsigned int zone, int mob, unsigned int xps, unsigned int gold)
{
	areaData.GetAreaData(zone, mob, xps, gold);
	return;
}
