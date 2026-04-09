/************************************
one liner quest shit
************************************/
#pragma once
#include <QtTypes>
class quest_info
{
public:
  qint32 number;
  char *name;
  char *hint1;
  char *hint2;
  char *hint3;
  char *objshort;
  char *objlong;
  char *objkey;
  qint32 level;
  qint32 objnum;
  qint32 mobnum;
  qint32 timer;
  qint32 reward;
  qint32 cost;
  qint32 brownie;
  bool active;
  quest_info *next;
};

qint32 load_quests();
qint32 save_quests();
quest_info *get_quest_struct(qint32);
quest_info *get_quest_struct(char *);
void quest_update();