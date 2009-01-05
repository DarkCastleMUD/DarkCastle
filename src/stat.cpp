
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







/********************Stolen start **************************************/


/* struct of items stolen from players via steal command */ 
struct StolenItems
{
	int item;      //What was Stolen
	string victim; //Who stolen from
	string thief;  //Who stole it
};

/* struct of stolen data via the pocket and steal command */
struct StolenStruct 
{
	unsigned int stolen_gold;        //How much gold has this person stolen
	unsigned int had_stolen;         //How much gold had been stolen from this person
	vector<StolenItems> stolenItems; //Item stolen/been stolem by/from player
};

/* Class to wrap up all the stolen data manipulation */
class StolenStats
{

	public:
		StolenStats() {};
		~StolenStats(){};
		void GoldFromPlayer(CHAR_DATA *ch, CHAR_DATA *victim, unsigned int amount);
		void ItemFromPlayer(CHAR_DATA *ch, CHAR_DATA *victim, int item);
		void DisplayStolenInformation(CHAR_DATA *ch);
		void SortDataXP(CHAR_DATA *ch);
		void Load() {}
		void Save() {}
	private:
		map<string, StolenStruct> stolenData; 
};

void StolenStats::DisplayStolenInformation(CHAR_DATA *ch)
{
	string output_buf, output_buf2;
        char buf[MAX_STRING_LENGTH];
	OBJ_DATA *get_obj_vnum(int vnum);
	OBJ_DATA *tempobj;
	map<string, StolenStruct>::iterator Item_it;

        sprintf(buf,"%-20s %-15s %-15s\n\r","Thief","Gold Stolen","Lost Gold");
	send_to_char(buf,ch);
	for(Item_it = stolenData.begin(); Item_it != stolenData.end(); Item_it++)
	{
		sprintf(buf, "\n\r%-20s $5%-15d$R $5%-15d$R",
		Item_it->first.c_str(),
		Item_it->second.stolen_gold,
		Item_it->second.had_stolen);
      		output_buf += buf;
	}
	page_string(ch->desc, output_buf.c_str(), 1);
        
	send_to_char("\n\rPilfer log! ---\n\r",ch);
	for(Item_it = stolenData.begin(); Item_it != stolenData.end(); Item_it++)
        {
                
                if(Item_it->second.stolenItems.empty())
		{
			sprintf(buf,"%s has not been pilfered or has not pilfered anyone!\n\r",Item_it->first.c_str());
			send_to_char(buf,ch);
		}
		else
		{
                        vector<StolenItems>::iterator item;
			for(item = Item_it->second.stolenItems.begin(); item != Item_it->second.stolenItems.end(); item++)
			{
				tempobj = get_obj_vnum(item->item);
				if(!tempobj)
				{
					send_to_char("An object is missing from game!!!\n\r",ch);
					continue;
				}
				if(item->thief.empty())
				{
					sprintf(buf,"%s stole %s from %s\n\r",Item_it->first.c_str(),
					tempobj->short_description,item->victim.c_str()); 		  
					output_buf2 += buf;
				}
			}
		}
        }
	page_string(ch->desc, output_buf2.c_str(), 1);
	return;
}

void StolenStats::GoldFromPlayer(CHAR_DATA *ch, CHAR_DATA *victim, unsigned int amount) 
{
	if(stolenData.end() == stolenData.find(GET_NAME(ch)))
	
		stolenData[GET_NAME(ch)].stolen_gold = amount;
	else
		stolenData[GET_NAME(ch)].stolen_gold += amount;
	if(stolenData.end() == stolenData.find(GET_NAME(victim)))
	
		stolenData[GET_NAME(victim)].had_stolen = amount;
	else
		stolenData[GET_NAME(victim)].had_stolen += amount;
	
}

void StolenStats::ItemFromPlayer(CHAR_DATA *ch, CHAR_DATA *victim, int item)
{
	struct StolenItems thief;
	struct StolenItems sucker;
	
	thief.item =item;
	thief.victim = GET_NAME(victim);

	sucker.item = item;
	sucker.thief = GET_NAME(ch);

	if(stolenData.end() == stolenData.find(GET_NAME(ch))) 
        {
		stolenData[GET_NAME(ch)].stolenItems.empty();
		stolenData[GET_NAME(ch)].stolenItems.push_back(thief);		                
	}
        else
		stolenData[GET_NAME(ch)].stolenItems.push_back(thief);		                

        if(stolenData.end() == stolenData.find(GET_NAME(victim)))
	{
		stolenData[GET_NAME(victim)].stolenItems.push_back(sucker);		                
		stolenData[GET_NAME(victim)].stolenItems.push_back(sucker);		                
        }
	else
		stolenData[GET_NAME(victim)].stolenItems.push_back(sucker);		                

}






StolenStats stolenStats;

void gold_from_player(CHAR_DATA *ch, CHAR_DATA *victim, unsigned int amount)
{
	stolenStats.GoldFromPlayer(ch, victim, amount);
	return;
}

void item_from_player(CHAR_DATA *ch, CHAR_DATA *victim, int item)
{
	stolenStats.ItemFromPlayer(ch, victim, item);
	return;
}

int do_stolen(CHAR_DATA *ch, char *argument, int cmd)
{
	stolenStats.DisplayStolenInformation(ch);
	return eSUCCESS;
}
/********************Stolen end **************************************/

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

