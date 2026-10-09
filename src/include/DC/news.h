#pragma once
#include <ctime>
#include <QString>
class news_data
{
public:
  news_data *next;
  time_t time;
  QString news;
  char *addedby;
};
