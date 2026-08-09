#pragma once
#include <ctime>
class news_data
{
public:
  news_data *next;
  time_t time;
  char *news;
  char *addedby;
};
