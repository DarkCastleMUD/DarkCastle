#pragma once
/************************************
one liner quest shit
************************************/
#include "DC/types.h"
#include <QString>
class Character;
class Object;

#define QUEST_MAX 1         // max quests at a time
#define QUEST_SHOW 10       // max quests shown at a time
#define QUEST_MAX_CANCEL 15 // max quests canceled at a time
#define QUEST_TOTAL 500     // max total quests in file
#define QUEST_MASTER 10027  // vnum of questmaster

class quest_info
{
public:
  int number;
  char *name;
  QString hint1;
  char *hint2;
  char *hint3;
  char *objshort;
  char *objlong;
  char *objkey;
  int level;
  int objnum;
  int mobnum;
  int timer;
  int reward;
  int cost;
  int brownie;
  bool active;
  quest_info *next;
};

int load_quests();
int save_quests();
quest_info *get_quest_struct(int);
quest_info *get_quest_struct(char *);
int do_add_quest(CharacterPtr, char *);
void list_quests(CharacterPtr);
void show_quest_info(CharacterPtr, int);
bool check_available_quest(CharacterPtr, quest_info *);
bool check_quest_current(CharacterPtr, int);
bool check_quest_pass(CharacterPtr, int);
bool check_quest_complete(CharacterPtr, int);
int get_quest_price(quest_info *);
void show_quest_header(CharacterPtr);
void show_quest_amount(CharacterPtr);
void show_quest_closer(CharacterPtr);
int show_one_quest(CharacterPtr, quest_info *, int);
int show_one_complete_quest(CharacterPtr, quest_info *, int);
int show_one_available_quest(CharacterPtr, quest_info *, int);
void show_available_quests(CharacterPtr);
void show_pass_quests(CharacterPtr);
void show_current_quests(CharacterPtr);
void show_complete_quests(CharacterPtr);
int start_quest(CharacterPtr, quest_info *);
int pass_quest(CharacterPtr, quest_info *);
int complete_quest(CharacterPtr, quest_info *);
int stop_current_quest(CharacterPtr, quest_info *);
int stop_current_quest(CharacterPtr, int);
int stop_all_quests(CharacterPtr);
void quest_update();
int quest_handler(CharacterPtr, CharacterPtr, int, char *);
int quest_master(CharacterPtr, ObjectPtr, int, char *, CharacterPtr);
int do_quest(CharacterPtr, char *, int);