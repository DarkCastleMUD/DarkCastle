

struct news_data
{
   struct news_data *next;
   time_t time;
   char *news;
   char *addedby;
};
