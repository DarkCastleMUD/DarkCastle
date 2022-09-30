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
int  do_add_quest(struct char_data *, char *);
void list_quests(struct char_data *);
void show_quest_info(struct char_data *, int);
bool check_available_quest(struct char_data *, struct quest_info *);
bool check_quest_current(struct char_data *, int);
bool check_quest_pass(struct char_data *, int);
bool check_quest_complete(struct char_data *, int);
int  get_quest_price(struct quest_info *);
void show_quest_header(struct char_data *);
void show_quest_amount(struct char_data *);
void show_quest_closer(struct char_data *);
int  show_one_quest(struct char_data *, struct quest_info *, int);
int  show_one_complete_quest(struct char_data *, struct quest_info *, int);
int  show_one_available_quest(struct char_data *, struct quest_info *, int);
void show_available_quests(struct char_data *);
void show_pass_quests(struct char_data *);
void show_current_quests(struct char_data *);
void show_complete_quests(struct char_data *);
int  start_quest(struct char_data *, struct quest_info *);
int  pass_quest(struct char_data *, struct quest_info *);
int  complete_quest(struct char_data *, struct quest_info *);
int  stop_current_quest(struct char_data *, struct quest_info *);
int  stop_current_quest(struct char_data *, int);
int  stop_all_quests(struct char_data *);
void quest_update();
int  quest_handler(struct char_data *, struct char_data *, int, char *);
int  quest_master(struct char_data *, obj_data *, int, char *, struct char_data *);
int  do_quest(struct char_data *, char *, int);


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
