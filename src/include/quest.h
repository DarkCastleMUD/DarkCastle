/************************************
one liner quest shit
************************************/
#ifndef QUEST_H_
#define QUEST_H_

#define QUEST_MAX    1     //max quests at a time
#define QUEST_SHOW   10    //max quests shown at a time
#define QUEST_CANCEL 15    //max quests canceled at a time
#define QUEST_TOTAL  500   //max total quests in file
#define QUEST_MASTER 10027 //vnum of questmaster

int  load_quests();
int  save_quests();
struct quest_info * get_quest_struct(int);
struct quest_info * get_quest_struct(char *);
int  do_add_quest(CHAR_DATA *, char *);
void list_quests(CHAR_DATA *);
void show_quest_info(CHAR_DATA *, int);
bool check_available_quest(CHAR_DATA *, struct quest_info *);
bool check_quest_current(CHAR_DATA *, int);
bool check_quest_pass(CHAR_DATA *, int);
bool check_quest_complete(CHAR_DATA *, int);
int  get_quest_price(struct quest_info *);
void show_quest_header(CHAR_DATA *);
void show_quest_amount(CHAR_DATA *);
void show_quest_closer(CHAR_DATA *);
int  show_one_quest(CHAR_DATA *, struct quest_info *, int);
int  show_one_complete_quest(CHAR_DATA *, struct quest_info *, int);
int  show_one_available_quest(CHAR_DATA *, struct quest_info *, int);
void show_available_quests(CHAR_DATA *);
void show_pass_quests(CHAR_DATA *);
void show_current_quests(CHAR_DATA *);
void show_complete_quests(CHAR_DATA *);
int  start_quest(CHAR_DATA *, struct quest_info *);
int  pass_quest(CHAR_DATA *, struct quest_info *);
int  complete_quest(CHAR_DATA *, struct quest_info *);
int  stop_current_quest(CHAR_DATA *, struct quest_info *);
int  stop_current_quest(CHAR_DATA *, int);
int  stop_all_quests(CHAR_DATA *);
void quest_update();
int  quest_handler(CHAR_DATA *, CHAR_DATA *, int, char *);
int  quest_master(CHAR_DATA *, OBJ_DATA *, int, char *, CHAR_DATA *);
int  do_quest(CHAR_DATA *, char *, int);


struct quest_info {
   int  number;
   char * name;
   char * hint1;
   char * hint2;
   char * hint3;
   char * objshort;
   char * objlong;
   char * objkey;
   int  level;
   int  objnum;
   int  mobnum;
   int  timer;
   int  reward;
   int  cost;
   int  brownie;
   bool active;
   struct quest_info * next;
};

#endif
