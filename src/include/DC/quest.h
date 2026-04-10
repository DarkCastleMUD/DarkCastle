/************************************
one liner quest shit
************************************/
#pragma once
#include <QtTypes>
class quest_info
{
public:
  qint32 number;
  QString name;
  QString hint1;
  QString hint2;
  QString hint3;
  QString objshort;
  QString objlong;
  QString objkey;
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
quest_info *get_quest_struct(QString);
void quest_update();