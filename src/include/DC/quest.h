/************************************
one liner quest shit
************************************/
#ifndef QUEST_H_
#define QUEST_H_

#include "DC/character.h"

#define QUEST_MAX 1        // max quests at a time
#define QUEST_SHOW 10      // max quests shown at a time
#define QUEST_CANCEL 15    // max quests canceled at a time
#define QUEST_TOTAL 500    // max total quests in file
#define QUEST_MASTER 10027 // vnum of questmaster

int load_quests();
int save_quests();
struct quest_info *get_quest_struct(int);
struct quest_info *get_quest_struct(char *);
int do_add_quest(Character *, char *);
void list_quests(Character *);
void show_quest_info(Character *, int);
bool check_available_quest(Character *, struct quest_info *);
bool check_quest_current(Character *, int);
bool check_quest_pass(Character *, int);
bool check_quest_complete(Character *, int);
int get_quest_price(struct quest_info *);
void show_quest_header(Character *);
void show_quest_amount(Character *);
void show_quest_closer(Character *);
int show_one_quest(Character *, struct quest_info *, int);
int show_one_complete_quest(Character *, struct quest_info *, int);
int show_one_available_quest(Character *, struct quest_info *, int);
void show_available_quests(Character *);
void show_pass_quests(Character *);
void show_current_quests(Character *);
void show_complete_quests(Character *);
int start_quest(Character *, struct quest_info *);
int pass_quest(Character *, struct quest_info *);
int complete_quest(Character *, struct quest_info *);
int stop_current_quest(Character *, struct quest_info *);
int stop_current_quest(Character *, int);
int stop_all_quests(Character *);
void quest_update();
int quest_handler(Character *, Character *, int, char *);
int quest_master(Character *, Object *, int, char *, Character *);
int do_quest(Character *, char *, int);

struct quest_info
{
   int number;
   char *name;
   char *hint1;
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
   struct quest_info *next;
};

#endif
