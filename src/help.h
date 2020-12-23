
#define READ_SIZE 256
#define MAX_HELP_KEYWORD_LENGTH     20
#define MAX_HELP_RELATED_LENGTH     60
#define MAX_HELP_LENGTH             8192

struct help_index_element_new {
   char *keyword1;
   char *keyword2;
   char *keyword3;
   char *keyword4;
   char *keyword5;
   char *entry;
   char *related;
   int min_level;
};

