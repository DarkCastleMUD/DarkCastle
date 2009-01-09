#
extern "C"
{
#include <ctype.h>
#include <string.h>
}
#include <string>
#include <map>
#include <queue>
#include <algorithm>
#include <list>

#include "obj.h"
#include "spells.h"
#include "player.h"
#include "terminal.h"
#include "levels.h"
#include "character.h"
#include "room.h"
#include "utility.h"
#include "assert.h"
#include "db.h"
#include "vault.h"
#include "returnvals.h"
#include "interp.h"

using namespace std;


extern int top_of_zone_table;
extern zone_data *zone_table;


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
	unsigned int area;
	int64_t xps;
	int64_t gold;
	vector<MobKills> mobKills;
};

class AreaData
{
	public:
		AreaData();
		~AreaData() {}
		void GetAreaData(unsigned int zone, int mob, int64_t xps, int64_t gold);
		void DisplaySingleArea(CHAR_DATA *ch, int area);
		void DisplayAreaData(CHAR_DATA *ch);
		void SortAreaData(CHAR_DATA *ch, SortState state);
		void Load() {}
		void Save() {}
	private:
		map<unsigned int,AreaStats> areaStats;
};	

AreaData::AreaData() 
{

}
bool CompareAreaXPStats( AreaStats first, AreaStats second)
{
	return first.xps > second.xps;
}

bool CompareAreaGoldStats( AreaStats first, AreaStats second)
{
	return first.xps > second.xps;
}

void AreaData::SortAreaData(CHAR_DATA *ch, SortState state)
{
	list<AreaStats> lAreaStats;
	map<unsigned int,AreaStats>::iterator it;
	AreaStats aStats;
	char buf2[MAX_STRING_LENGTH];
	string output_buf;
	int i=0;

	for(it = areaStats.begin(); it!=areaStats.end(); it++)
	{
		if ( (state==SORT_XP) && (it->second.xps <1) ) continue; 
		if ( (state==SORT_GOLD) && (it->second.gold <1) ) continue; 
			lAreaStats.push_back(it->second);
	}
	if(state==SORT_XP)
	{
		lAreaStats.sort(CompareAreaXPStats);
		for(list<AreaStats>::iterator lit = lAreaStats.begin(); lit!=lAreaStats.end();lit++)
		{
			i++;
			snprintf(buf2,MAX_STRING_LENGTH,"%%d)%%-%ds $5%%15lld$R xps\r\n", 
			60+(strlen(zone_table[lit->area].name)-nocolor_strlen(zone_table[lit->area].name)));
			csendf(ch, buf2,i,zone_table[lit->area].name,lit->xps);
		}
	}
	if(state==SORT_GOLD)
	{
		lAreaStats.sort(CompareAreaGoldStats);
		for(list<AreaStats>::iterator lit = lAreaStats.begin(); lit!=lAreaStats.end();lit++)
		{
			i++;
			snprintf(buf2,MAX_STRING_LENGTH,"%%d)%%-%ds $5%%15lld$R gold\r\n", 
			60+(strlen(zone_table[lit->area].name)-nocolor_strlen(zone_table[lit->area].name)));
			csendf(ch, buf2,i,zone_table[lit->area].name,lit->gold);
		}
	}
	return;	
}

void AreaData::DisplaySingleArea(CHAR_DATA *ch, int area)
{
	
	char buf[MAX_STRING_LENGTH];
	string output_buf;
	vector<MobKills>::iterator mobs;
	CHAR_DATA *get_mob_vnum(int vnum);
	CHAR_DATA *tmpchar;

	if( (area <0) || (area > top_of_zone_table))
	{
		send_to_char("Area number is outside the limits\r\n",ch);
		return;
	}
	sprintf(buf, "%d)%30s -- $5%15lld$R xps -- $5%15lld$R gold\n\r", area, zone_table[area].name, areaStats[area].xps, areaStats[area].gold);
	output_buf += buf;
	sprintf(buf, "%-30s %-5s\r\n","Mob Name","Killed");
	output_buf += buf;
	for(mobs = areaStats[area].mobKills.begin(); mobs != areaStats[area].mobKills.end(); mobs++)	
	{
		tmpchar = get_mob_vnum(mobs->name);
		if(!tmpchar)
		{
			send_to_char("Shit a mob is missing from game!!!\n\r",ch);
			continue;
		}
		sprintf(buf, "%-30s %-5d %s\r\n",get_mob_vnum(mobs->name)->short_desc ,mobs->howmany, mobs->howmany < 2 ? "time":"times");
		output_buf += buf;
	}
	page_string(ch->desc, output_buf.c_str(), 1);
	return;	
}
void AreaData::DisplayAreaData(CHAR_DATA *ch) 
{
	int i;
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	string output_buf;
	
	output_buf+=buf;
	for(i=0;i<=top_of_zone_table;i++)
	{
		if(areaStats[i].xps ==0) continue;
		snprintf(buf2,MAX_STRING_LENGTH, "%%3d)%%-%ds|$5%%15lld$R xps|$5%%15lld$R gold|\n\r", 
		60+(strlen(zone_table[i].name)-nocolor_strlen(zone_table[i].name)));
		csendf(ch, buf2,i,zone_table[i].name, areaStats[i].xps, areaStats[i].gold);
	}
	return;
}
void AreaData::GetAreaData(unsigned int zone, int mob, int64_t xps, int64_t gold)
{
	MobKills mobObj;
        vector<MobKills>::iterator mobs;

	if(areaStats.end() == areaStats.find(zone))
	{
		areaStats[zone].area = zone;
		areaStats[zone].xps  = xps;
		areaStats[zone].gold = gold;
	}	
	else
	{
		areaStats[zone].area = zone;
		areaStats[zone].gold += gold;
		areaStats[zone].xps  += xps;
	}
	for(mobs = areaStats[zone].mobKills.begin(); mobs != areaStats[zone].mobKills.end(); mobs++)
	{	
		if(mobs->name==mob)
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

int do_areastats(CHAR_DATA *ch, char *argument, int cmd)
{
	char buf[MAX_STRING_LENGTH];

	argument = one_argument(argument , buf);
	if(!*buf)
	{
		send_to_char("$BUsage:$R \r\n",ch);
		send_to_char("$Bareastats$R \r\n",ch);
		send_to_char("$Bareastats area #$R\r\n",ch);
		send_to_char("$Bareastats topxps$R\r\n",ch);
		send_to_char("$Bareastats topgold$R\r\n",ch);
		return eSUCCESS;
	}
	if(!strcmp(buf, "area"))
	{
		argument = one_argument(argument , buf);
		if(!*buf)
		{
			send_to_char("You need to specify a zone number.\r\n",ch);
			return eSUCCESS;
		}
		areaData.DisplaySingleArea(ch, atoi(buf));
		return eSUCCESS; 
	}
	if(!strcmp(buf, "all"))
	{
		areaData.DisplayAreaData(ch);
		return eSUCCESS;
	}
	if(!strcmp(buf, "topxps"))
	{
		areaData.SortAreaData(ch ,SORT_XP);
		return eSUCCESS;
	}
	if(!strcmp(buf, "topgold"))
	{
		areaData.SortAreaData(ch ,SORT_GOLD);
		return eSUCCESS;
	}
	return eSUCCESS;
}

void getAreaData(unsigned int zone, int mob, unsigned int xps, unsigned int gold)
{
	areaData.GetAreaData(zone, mob, xps, gold);
	return;
}

