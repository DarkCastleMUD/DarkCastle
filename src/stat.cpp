
extern "C"
{
#include <ctype.h>
#include <string.h>
}
#include <obj.h>
#include <spells.h>
#include <player.h> 
#include <terminal.h> 
#include <levels.h> 
#include <character.h> 
#include <room.h>
#include <utility.h> 
#include <assert.h>
#include <db.h>
#include <vault.h>
#include <returnvals.h>
#include <interp.h>
#include <string>
#include <map>
#include <queue>
#include <algorithm>

using namespace std;

/******************* Area start **************************************/
enum SortState
{
	SORT_XP=0,
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
	unsigned int xps;
	unsigned int gold;
	vector<MobKills> mobKills;
};

class AreaData
{
	public:
		AreaData();
		~AreaData() {}
		void GetAreaData(unsigned int zone, int mob, unsigned int xps, unsigned int gold);
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
void AreaData::SortAreaData(CHAR_DATA *ch, SortState state)
{
	vector< vector<int> > v2dArray;
	map<unsigned int,AreaStats>::iterator it;

	switch(state)
	{
		case SORT_XP:
		for(it=areaStats.begin(); it!=areaStats.end();it++)
		{
			if(it->second.xps == 0) continue;
			
		}				
		return;
		break;
		case SORT_MOB:
		return;
		break;
		case SORT_GOLD:
		return;
		break;
		default:
		return;
		break;
	}
	return;
}
void AreaData::DisplaySingleArea(CHAR_DATA *ch, int area)
{
	
	char buf[MAX_STRING_LENGTH];
	string output_buf;
	vector<MobKills>::iterator mobs;
	extern int top_of_zone_table;
	extern struct zone_data *zone_table;
	CHAR_DATA *get_mob_vnum(int vnum);
	CHAR_DATA *tmpchar;

	if( (area <0) || (area > top_of_zone_table))
	{
		send_to_char("Area number is outside the limits\r\n",ch);
		return;
	}
	sprintf(buf, "%d)%s -- $5%d$R xps -- $5%d$R gold\n\r", area, zone_table[area].name, areaStats[area].xps, areaStats[area].gold);
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
	string output_buf;
	extern struct zone_data *zone_table;
	extern int top_of_zone_table;

	for(i=0;i<=top_of_zone_table;i++)
	{
		if(areaStats[i].xps ==0) continue;
		sprintf(buf, "%d)%s -- $5%d$R xps -- $5%d$R gold\n\r", i, zone_table[i].name, areaStats[i].xps, areaStats[i].gold);
		output_buf += buf; 	
	}
	page_string(ch->desc, output_buf.c_str(), 1);
	return;
}
void AreaData::GetAreaData(unsigned int zone, int mob, unsigned int xps, unsigned int gold)
{
	struct MobKills mobObj;
        vector<MobKills>::iterator mobs;

	if(areaStats.end() == areaStats.find(zone))
	{
		areaStats[zone].xps  = xps;
		areaStats[zone].gold = gold;
	}	
	else
	{
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
		areaData.DisplayAreaData(ch);
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
	send_to_char("areastats or areastats area #\r\n",ch);
	return eSUCCESS;
}

void getAreaData(unsigned int zone, int mob, unsigned int xps, unsigned int gold)
{
	areaData.GetAreaData(zone, mob, xps, gold);
	return;
}

