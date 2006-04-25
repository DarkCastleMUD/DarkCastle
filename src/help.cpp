extern "C"
{
#include <ctype.h>
#include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <structs.h>
#include <room.h>
#include <character.h>
#include <obj.h>
#include <utility.h>
#include <terminal.h>
#include <player.h>
#include <levels.h>
#include <mobile.h>
#include <clan.h>
#include <handler.h>
#include <db.h> // exp_table
#include <interp.h>
#include <connect.h>
#include <spells.h>
#include <act.h>
#include <returnvals.h>
#include <help.h>
#include <fileinfo.h>

// Externs
extern void skip_spaces(char **string);
extern struct help_index_element_new *new_help_table;
extern int new_top_of_helpt;
int get_line(FILE * fl, char *buf);
int is_abbrev(char *arg1, char *arg2);
void help_string_to_file(FILE *f, char *string);

// locals
struct help_index_element_new *find_help(char *keyword);
int strn_cmp(char *arg1, char *arg2, int n);
int count_hash_records(FILE * fl);
void show_hedit_usage(struct char_data *ch);
void save_help(struct char_data *ch);
int get_line_with_space(FILE * fl, char *buf);
int show_one_help_entry(int entry, struct char_data *ch, int count);
void show_help_header(struct char_data *ch);
void show_help_bar(struct char_data *ch);

// da functions
int do_mortal_help(struct char_data *ch, char *argument, int cmd) {
  extern char new_help[MAX_STRING_LENGTH];
  send_to_char(new_help, ch);
  return eSUCCESS; 
}

int do_new_help(struct char_data *ch, char *argument, int cmd)
{
  char buf[256];
  extern char new_help[MAX_STRING_LENGTH];
  extern char new_ihelp[MAX_STRING_LENGTH];
  struct help_index_element_new *this_help;
  char entry[MAX_STRING_LENGTH];
  char key1[256],key2[256],key3[256],key4[256],key5[256],rec_level[256];

  if (!ch->desc)
    return eFAILURE; 

  skip_spaces(&argument);

  if (!*argument) {
    if (GET_LEVEL(ch) < IMMORTAL)
      send_to_char(new_help, ch);
    else
      send_to_char(new_ihelp, ch);
    return eFAILURE; 
  }

  if (!new_help_table) {
    send_to_char("No help available.\r\n", ch);
    return eFAILURE; 
  }


  if (!(this_help = find_help(argument))) {
      send_to_char("There is no help on that word.\r\n", ch);
      sprintf(buf, "'%s' has no help entry.  %s just tried to call it.", argument, GET_NAME(ch));
      log(buf, DEITY, LOG_HELP);
      return eFAILURE;
    }

  if (this_help->min_level > GET_LEVEL(ch)) {
    send_to_char("There is no help on that word.\r\n", ch);
    return eFAILURE;
  }

  sprintf(key1, "'%s'", this_help->keyword1);
  sprintf(key2, "'%s'", this_help->keyword2);
  sprintf(key3, "'%s'", this_help->keyword3);
  sprintf(key4, "'%s'", this_help->keyword4);
  sprintf(key5, "'%s'", this_help->keyword5);

  sprintf(buf, "%s %s %s %s %s", key1,
        ((this_help->keyword2 && strcmp(key2, "'NONE'")) ? key2 : " "),
        ((this_help->keyword3 && strcmp(key3, "'NONE'")) ? key3 : " "),
        ((this_help->keyword3 && strcmp(key4, "'NONE'")) ? key4 : " "),
        ((this_help->keyword3 && strcmp(key5, "'NONE'")) ? key5 : " "));

  sprintf(rec_level, "\r\nLevel Required: %d", this_help->min_level);
  sprintf (entry,
         "$1$B+=+=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=+=+\r\n"
         "$1$B| | $B$5Help For: $B$7%-61.61s $1$B| |\r\n"
         "$1$B+=+=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=+=+\r\n"
         "$B$7%s%s$R"
         "\r\n"
         "$R%s"
         "\r\n"
         "$1$B+=+=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=+=+\r\n"
         "$1$B| | $B$5Related Help: $B$7%-57.57s $1$B| |\r\n"
         "$1$B+=+=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=+=+\r\n$R",
          buf, ((GET_LEVEL(ch) >= IMMORTAL) ? rec_level : " "), 
               ((this_help->min_level < IMMORTAL) ? " " : "\r\nImmortal-only command.\r\n"),
         this_help->entry, this_help->related);

  page_string(ch->desc, entry, 1);

  return eSUCCESS;
}

struct help_index_element_new *find_help(char *keyword)
{
  int i;

  if (!strcmp(keyword, "NONE"))
     return NULL;

  for (i = 0;i < (int)strlen(keyword);i++)
     keyword[i] = UPPER(keyword[i]);

  for (i = 0; i < new_top_of_helpt; i++)
    if (!strcmp(keyword, new_help_table[i].keyword1) ||
        !strcmp(keyword, new_help_table[i].keyword2) ||
        !strcmp(keyword, new_help_table[i].keyword3) ||
        !strcmp(keyword, new_help_table[i].keyword4) ||
        !strcmp(keyword, new_help_table[i].keyword5))
      return (new_help_table + i);

  return NULL;
}

int load_new_help(FILE *fl, int reload, struct char_data *ch)
{     
  char entry[32384], line[READ_SIZE+1], tmpentry[32384], buf[256];
  struct help_index_element_new new_help;
  int version = 0, level = -1, linenum = 0;
      
  linenum+=get_line(fl, line);
  if (sscanf(line, "@Version: %d", &version) != 1) {
     if (reload == 1) {
        log("Error in verion number in help file.\r\n", OVERSEER, LOG_HELP);
        return eFAILURE;
      } else {
        perror("Error in verion number in help file.\r\n");
        abort();
      }
  }   
      
  linenum+=get_line(fl, line);
    
  while (*line != '$') {
    new_help.keyword1 = str_hsh(line);
  
    linenum+=get_line(fl, line);
    new_help.keyword2 = str_hsh(line);
  
    linenum+=get_line(fl, line);
    new_help.keyword3 = str_hsh(line);

    linenum+=get_line(fl, line);
    new_help.keyword4 = str_hsh(line);

    linenum+=get_line(fl, line);
    new_help.keyword5 = str_hsh(line);

    linenum+=get_line(fl, line); 
    new_help.related = str_hsh(line);

    linenum+=get_line(fl, line);
    if (sscanf(line, "L: %d", &level) == 1) {
       new_help.min_level = level;
    } else {
       new_help.min_level = 0;
    }
    
    linenum+=get_line(fl, line);
    // E: Here.
    linenum+=get_line_with_space(fl, line);
    *tmpentry = '\0';
    while (*line != '#') {
      if (*line == '\0') 
        sprintf(tmpentry, "%s\r\n", tmpentry);
      else 
        sprintf(tmpentry, "%s%s\r\n", tmpentry, line);
      linenum+=get_line_with_space(fl, line);
    }

    if (strlen(tmpentry) > MAX_HELP_LENGTH) 
      tmpentry[MAX_HELP_LENGTH - 1] = '\0';

    if (!(new_help.entry = str_hsh(tmpentry)))
       new_help.entry = str_hsh("Error reading help entry. Please notify an Immortal!\n");

    new_help_table[new_top_of_helpt] = new_help;
    new_top_of_helpt++;

    level = -1;
    *entry = '\0';

    linenum+=get_line(fl, line);
    // ~ is here.
    linenum+=get_line(fl, line);
  }

  if (reload == 1) {
    sprintf(buf, "%s just reloaded the help files.", GET_NAME(ch));
    log(buf, OVERSEER, LOG_HELP);
  }
  return eSUCCESS;
}

char help_buf[MAX_STRING_LENGTH*4];

int do_hindex(struct char_data *ch, char *argument, int cmd)
{
   int i, minlen, count = 0;
   char arg[256];

  if(!has_skill(ch, COMMAND_HINDEX)) {
      send_to_char("Huh?\r\n", ch);
      return eFAILURE;
  }

  half_chop(argument, argument, arg);
   if (!arg[0]) {
     csendf(ch, "Usage: hindex <ID#>\r\n"
                  "       hindex <low ID#> <high ID#>  (you can display up to 30 at a time)\r\n"
                  "       hindex <start of a word(s)>\r\n"
                  "       hindex -<l|i|u|d>\n\r"
                  "              l = level, l <start> <end>\r\n"
                  "              i = immortal\r\n"
                  "              d = defunct, < level 0 OR > level %d\r\n"
                  "              u = unfinished, level 75\r\n"
                  "\r\n", IMP);
     return eFAILURE;
   }
   int start = 0;
   if (*argument == '-') { // we are doing a function, not a normal search
     if ((*(argument+1) == 'l' || *(argument+1) == 'L')) {  // show help based on level range, excluded all level 1's
       //half_chop(arg, argument, arg);
	char arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
	argument = &arg[0];
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);
	one_argument(argument, arg);
	if (arg[0] && is_number(arg))
	  start = atoi(arg);
	argument = &arg2[0];
//	arg = str_cpy(arg3);
	strcpy(arg, arg3);
       if ((((atoi(argument)) > 0) || *argument == '0') && ((atoi(arg)) > 0)) {  // not valid numbers
         if (atoi(argument) > atoi(arg)) {
            send_to_char("Usage: hindex -l <low level> <high level>\r\n", ch);
            return eFAILURE;
         }
         if (!*arg) sprintf(arg, "%s", argument); // if they left off the second arg, copy the first, show only one level
         show_help_header(ch);
         for (i = 0; i < new_top_of_helpt; i++) {
           if (new_help_table[i].min_level >= atoi(argument) && new_help_table[i].min_level <= atoi(arg) 
               && start-- <= 0) // too many level 1s, so we must exclude them or get an OVERFLOW
             count = show_one_help_entry(i, ch, count);
         }
         show_help_bar(ch);
       }
     } else if ((*(argument+1) == 'u' || *(argument+1) == 'U')) { // unfinished help = level 75
       show_help_header(ch);
       for (i = 0; i < new_top_of_helpt; i++) {
         if (new_help_table[i].min_level == 75)
           count = show_one_help_entry(i, ch, count);
       }
       show_help_bar(ch);
     } else if ((*(argument+1) == 'd' || *(argument+1) == 'D')) { // show defunct ones with out of range levels
       show_help_header(ch);
       for (i = 0; i < new_top_of_helpt; i++) {
         if (new_help_table[i].min_level <= 0 || new_help_table[i].min_level > IMP)
           count = show_one_help_entry(i, ch, count);
       }
       show_help_bar(ch);
     } else if ((*(argument+1) == 'i' || *(argument+1) == 'I')) { // show defunct ones with out of range levels
       show_help_header(ch);
       for (i = 0; i < new_top_of_helpt; i++) {
         if (new_help_table[i].min_level >= IMMORTAL && new_help_table[i].min_level <= IMP)
           count = show_one_help_entry(i, ch, count);
       }
       show_help_bar(ch);
     }
   } else if ((((atoi(argument)) > 0) || *argument == '0') && ((atoi(arg)) > 0)) { // index #s out of range
     if (atoi(argument) > atoi(arg)) {
        send_to_char("Usage: hindex <low ID#> <high ID#>\r\n", ch); // wrong order, first > second
        return eFAILURE;
     } else if ((atoi(arg) - atoi(argument)) > 30) { // too many listed, only 30 at a time or we get too much spam
        send_to_char("You can only list 30 help entries at a time.\r\n", ch);
        return eFAILURE;
     } 
     show_help_header(ch);
     for (i = 0; i < new_top_of_helpt; i++) {
       if (i >= atoi(argument) && i <= atoi(arg)) 
         count = show_one_help_entry(i, ch, count);
     }
     show_help_bar(ch);
   } else if (((atoi(argument)) > 0) || *argument == '0') { // show a specific ID #
     show_help_header(ch);
     for (i = 0; i < new_top_of_helpt; i++) {
       if (i == atoi(argument)) 
          count = show_one_help_entry(i, ch, count);
     }
     show_help_bar(ch);
   } else { // we are searching based on keywords, show as many as you find
     minlen = strlen(argument);
     show_help_header(ch);
     for (i = 0; i < new_top_of_helpt; i++) {
       if (!strn_cmp(argument, new_help_table[i].keyword1, minlen) ||
           !strn_cmp(argument, new_help_table[i].keyword2, minlen) ||
           !strn_cmp(argument, new_help_table[i].keyword3, minlen) ||
           !strn_cmp(argument, new_help_table[i].keyword4, minlen) ||
           !strn_cmp(argument, new_help_table[i].keyword5, minlen)) {
           count = show_one_help_entry(i, ch, count);
       }
     }
     show_help_bar(ch);
   }
   send_to_char(help_buf, ch);
   csendf(ch, "$B$7Total Shown: $B$5%d$R\r\n", count);
   csendf(ch, "$B$7Total Help Entries: $B$5%d$R\r\n", new_top_of_helpt);

   return eSUCCESS;
}

int show_one_help_entry(int entry, struct char_data *ch, int count) {

   csendf(ch, "$B$6%3d $7- $5%3d $7[$3%-20.20s$7] [$3%-20.20s$B$7] [$3%-20.20s$B$7] "
               "[$3%-20.20s$B$7] [$3%-20.20s$B$7]\n\r", entry,
                (new_help_table[entry].min_level >= 0 ? new_help_table[entry].min_level : 999),
                (*new_help_table[entry].keyword1 ? new_help_table[entry].keyword1 : "None"),
                (*new_help_table[entry].keyword2 ? new_help_table[entry].keyword2 : "None"),
                (*new_help_table[entry].keyword2 ? new_help_table[entry].keyword3 : "None"),
                (*new_help_table[entry].keyword2 ? new_help_table[entry].keyword4 : "None"),
                (*new_help_table[entry].keyword3 ? new_help_table[entry].keyword5 : "None"));
    return count++;
}

void show_help_header(struct char_data *ch) {
   send_to_char("$B$6ID# $B$7- $RLVL $3 Keyword 1              Keyword 2              Keyword 3"
                "              Keyword 4              Keyword 5\r\n", ch);
   show_help_bar(ch);
}

void show_help_bar(struct char_data *ch) {
   send_to_char("%s$B$7--------------------------------------------------------------------------"
                "--------------------------------------------------\r\n$R", ch);
}

/* strn_cmp: a case-insensitive version of strncmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
int strn_cmp(char *arg1, char *arg2, int n)
{
  int chk, i;

  for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--) {
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))) {
      if (chk < 0)
        return (-1);
      else
        return (1);
    }
  } 
  return (0);
}

 
int do_reload_help(struct char_data *ch, char *argument, int cmd)
{
    
  FILE *new_help_fl;
  int help_rec_count = 0, ret = 0;

  //send_to_char("Command disabled!\r\n", ch);
  //return eFAILURE;


  if(!(new_help_fl = dc_fopen(NEW_HELP_FILE, "r"))) {
    log("Error opening help file for reload.", OVERSEER, LOG_HELP);
    return eFAILURE;
  }

  help_rec_count = count_hash_records(new_help_fl);
  dc_fclose(new_help_fl);

  if(!(new_help_fl = dc_fopen(NEW_HELP_FILE, "r"))) {
    log("Error opening help file for reload.", OVERSEER, LOG_HELP);
    return eFAILURE;
  }

  FREE(new_help_table);
  new_top_of_helpt = 0;
  CREATE(new_help_table, struct help_index_element_new, help_rec_count);
  ret = load_new_help(new_help_fl,1,ch);
  dc_fclose(new_help_fl);

  if (ret == eFAILURE) {
    send_to_char("Error reloading help files!\r\n", ch);
    return eFAILURE;
  }

  send_to_char("Help files reloaded.\r\n", ch);
  return eSUCCESS;
}

int do_hedit(struct char_data *ch, char *argument, int cmd)
{
    char buf[200], buf2[200], field[200], buf3[200], value[200];
    struct help_index_element_new new_help;
    int help_id = -1, i, key_id = -1, level = -1;

    if(IS_NPC(ch))
      return eFAILURE;

    if(!has_skill(ch, COMMAND_HEDIT)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
    }

    half_chop(argument, buf, buf2);

    if (!*buf) {
      show_hedit_usage(ch);
      return eFAILURE;
    }

    if (!str_cmp(buf, "save")) { // save all help entries
      save_help(ch);
      return eSUCCESS;
    }

    if (!*buf2) {
      show_hedit_usage(ch);
      return eFAILURE;
    }

    if (!str_cmp(buf, "new")) { // New Help Entry
       for (i = 0;i < (int)strlen(buf2);i++)
         buf2[i] = UPPER(buf2[i]);
       new_help.keyword1 = str_hsh(buf2);
       new_help.keyword2 = str_hsh("NONE");
       new_help.keyword3 = str_hsh("NONE");
       new_help.keyword4 = str_hsh("NONE");
       new_help.keyword5 = str_hsh("NONE");
       new_help.related = str_hsh("NONE");
       new_help.min_level = 75;
       new_help.entry = str_hsh("Blank help file!\r\n");

       RECREATE(new_help_table, struct help_index_element_new, new_top_of_helpt + 1);
       new_help_table[new_top_of_helpt] = new_help;
       sprintf(buf, "Help entry #%d added with keyword '%s'.\r\n", new_top_of_helpt, buf2);
       send_to_char(buf, ch);
       new_top_of_helpt++;
       sprintf(buf, "%s just created a help file for '%s'.", GET_NAME(ch), buf2);
       log(buf, OVERSEER, LOG_HELP);
    } else if ((help_id = atoi(buf)) || *buf == '0') { // Edit a specific help entry
        if (*buf == 0) help_id = 0;
        if (help_id < 0 || help_id >= new_top_of_helpt) {
          send_to_char("Not a valid help ID number.  Try using 'hindex'\r\n", ch);
          return eFAILURE;
        }
        half_chop(buf2, field, buf2);
        if (is_abbrev(field, "key")) { // changing one of the keys
           half_chop(buf2, buf3, value);
           if (!*buf3 || !*value) {
             send_to_char("Not a valid key # or no value specified.\r\n", ch);
             return eFAILURE;
           }
           if ((key_id = atoi(buf3))) {
             if (strlen(value) > MAX_HELP_KEYWORD_LENGTH) {
               value[MAX_HELP_KEYWORD_LENGTH - 1] = '\0';
             }
             for (i = 0;i < (int)strlen(value);i++)
               value[i] = UPPER(value[i]);
             switch (key_id) {
               case 1:
                 new_help_table[help_id].keyword1 = str_hsh(value);
                 break;
               case 2:
                 new_help_table[help_id].keyword2 = str_hsh(value);
                 break;
               case 3:
                 new_help_table[help_id].keyword3 = str_hsh(value);
                 break;
               case 4:
                 new_help_table[help_id].keyword4 = str_hsh(value);
                 break;
               case 5:
                 new_help_table[help_id].keyword5 = str_hsh(value);
                 break;
               default:
                 send_to_char("Not a valid key #.\r\n", ch);
                 return eFAILURE;
                 break;
             }
             sprintf(buf, "Keyword %d changed to '%s' for ID# %d.\r\n", key_id, value, help_id);
             send_to_char(buf, ch);
           } else {
             send_to_char("Not a valid key #.\r\n", ch);
             return eFAILURE;
           }
        } else if (is_abbrev(field, "level")) { // changing the level
           if (*buf2 && ((level = atoi(buf2)) || *buf2 == '0') && level >= 0 && level <= 110) {
             if (*buf2 == '0') level = 0;
             new_help_table[help_id].min_level = level;
             sprintf(buf, "Level changed to '%d' for ID# %d.\r\n", level, help_id);
             send_to_char(buf, ch);
           } else {
             send_to_char("Invalid level specified.\r\n", ch);
           }
        } else if (is_abbrev(field, "related")) { //changing the related
           if (*buf2) {
             if (strlen(buf2) > MAX_HELP_RELATED_LENGTH) {
               buf2[MAX_HELP_KEYWORD_LENGTH - 1] = '\0';
             }
             for (i = 0;i < (int)strlen(buf2);i++)
               buf2[i] = UPPER(buf2[i]);
             new_help_table[help_id].related = str_hsh(buf2);
             sprintf(buf, "Related changed to '%s' for ID# %d.\r\n", buf2, help_id);
             send_to_char(buf, ch);
           } else {
            show_hedit_usage(ch);
            return eFAILURE;
           }
        } else if (is_abbrev(field, "entry")) { //changing the actual help entry
          ch->desc->backstr = NULL;
          send_to_char("        Write your help entry and stay within the line.  (/s saves /h for help)\r\n"
                       "   |--------------------------------------------------------------------------------|\r\n", ch);
          if (new_help_table[help_id].entry) {
            ch->desc->backstr = str_dup(new_help_table[help_id].entry);
            send_to_char(ch->desc->backstr, ch);
          }

          ch->desc->connected = CON_EDITING;
          ch->desc->strnew = &(new_help_table[help_id].entry);
          ch->desc->max_str = MAX_HELP_LENGTH;
        } else {  // no idea wtf they are doing
          show_hedit_usage(ch);
          return eFAILURE;
        }
    } else {
      show_hedit_usage(ch);
      return eFAILURE;
    }

    return eSUCCESS;
}

void show_hedit_usage(struct char_data *ch)
{

   send_to_char("$3Syntax$R: hedit <id#> <field> [arg] [value]\r\n"
                "        hedit new <first keyword>\r\n"
                "        hedit save\r\n\r\n"
                "  Fields: key, level, entry, related\r\n"
                "  Args: Only apply to key in which it is 1-5\r\n"
                "  Values: The value of the specified field.  Does not affect entry\r\n", ch);
}

void save_help(struct char_data *ch) {
  FILE *f;
  int i;
  char file[256], buf[256];

  sprintf(file, "%s", NEW_HELP_FILE);
  
  if((f = dc_fopen(file, "w")) == NULL) {
    send_to_char("Couldn't open help file for saving.\n\r", ch);
    perror("Couldn't open help file for saving.\n\r");
    return;
  }

  fprintf(f, "@Version: 2\n");

  for (i = 0; i < new_top_of_helpt; i++) {
    help_string_to_file(f, new_help_table[i].keyword1);
    help_string_to_file(f, new_help_table[i].keyword2);
    help_string_to_file(f, new_help_table[i].keyword3);
    help_string_to_file(f, new_help_table[i].keyword4);
    help_string_to_file(f, new_help_table[i].keyword5);
    help_string_to_file(f, new_help_table[i].related);
    fprintf(f, "L: %d\n", new_help_table[i].min_level);
    fprintf(f, "E:\n");
    help_string_to_file(f, new_help_table[i].entry);
    fprintf(f, "#\n");
    fprintf(f, "~\n");
  }

  // end file
  fprintf(f, "$~\n");

  dc_fclose(f);
  send_to_char("Saved.\n\r", ch);
  sprintf(buf, "%s just saved the help files.", GET_NAME(ch));
  log(buf, OVERSEER, LOG_HELP);
}

void help_string_to_file(FILE *f, char *string)
{    
  char * newbuf = new char[strlen(string)+1];
  strcpy(newbuf, string);
  
  // remove all \r's
  for(char * curr = newbuf; *curr != '\0'; curr++)
  {
    if(*curr == '\r') {
      for(char * blah = curr; *blah != '\0'; blah++) // shift the rest of the string 1 left
        *blah = *(blah + 1);
      curr--; // (to check for \r\r cases)
    }
  }
   
  if (newbuf[strlen(newbuf) - 1] == '\n')
    newbuf[strlen(newbuf) - 1] = '\0';

  fprintf(f, "%s\n", newbuf);
  delete [] newbuf;
}


int get_line_with_space(FILE * fl, char *buf)
{
  char temp[256];
  int lines = 0;

  do {
    lines++;
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && *temp == '*');
 // } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl))
    return 0;
  else {
    strcpy(buf, temp);
    return lines;
  }
} 
