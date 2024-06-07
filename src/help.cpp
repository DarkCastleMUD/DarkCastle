
#include <cctype>
#include <cstring>

#include "DC/structs.h"
#include "DC/room.h"
#include "DC/character.h"
#include "DC/obj.h"
#include "DC/utility.h"
#include "DC/terminal.h"
#include "DC/player.h"
#include "DC/levels.h"
#include "DC/mobile.h"
#include "DC/clan.h"
#include "DC/handler.h"
#include "DC/db.h" // exp_table
#include "DC/interp.h"
#include "DC/connect.h"
#include "DC/spells.h"
#include "DC/act.h"
#include "DC/returnvals.h"
#include "DC/help.h"
#include "DC/fileinfo.h"

#include <map>
#include <vector>
#include <algorithm>

// Externs
extern void skip_spaces(char **string);
extern struct help_index_element_new *new_help_table;
extern int new_top_of_helpt;
int get_line(FILE *fl, char *buf);
int is_abbrev(const char *arg1, const char *arg2);
void help_string_to_file(FILE *f, char *string);

// locals
struct help_index_element_new *find_help(char *keyword);
int strn_cmp(char *arg1, char *arg2, int n);
int count_hash_records(FILE *fl);
void show_hedit_usage(Character *ch);
void save_help(Character *ch);
int get_line_with_space(FILE *fl, char *buf);
int show_one_help_entry(int entry, Character *ch, int count);
void show_help_header(Character *ch);
void show_help_bar(Character *ch);

// da functions
int do_mortal_help(Character *ch, char *argument, int cmd)
{
  extern char new_help[MAX_STRING_LENGTH];
  ch->send(new_help);
  return eSUCCESS;
}

struct ltstr
{
  bool operator()(int a, int b) const
  {
    return a < b;
  }
};

int levenshtein(const char *s, const char *t)
{
  unsigned int i, j, n, m, cost;
  unsigned int d[MAX_INPUT_LENGTH + 1][MAX_HELP_KEYWORD_LENGTH + 1];

  m = strlen(s);
  n = strlen(t);

  // Zero Matrix
  for (i = 0; i <= m; i++)
    for (j = 0; j <= n; j++)
      d[i][j] = 0;

  // Initialize Matrix
  for (i = 0; i <= m; i++)
    d[i][0] = i;
  for (j = 0; j <= n; j++)
    d[0][j] = j;

  for (i = 1; i <= m; i++)
    for (j = 1; j <= n; j++)
    {
      if (tolower(s[i]) == tolower(t[j]))
        cost = 0;
      else
        cost = 1;
      d[i][j] = std::min(std::min(d[i - 1][j] + 1,
                                  d[i][j - 1] + 1),
                         d[i - 1][j - 1] + cost);
    }

  return d[m][n];
}

int do_new_help(Character *ch, char *argument, int cmd)
{
  char buf[256];
  extern char new_help[MAX_STRING_LENGTH];
  extern char new_ihelp[MAX_STRING_LENGTH];
  struct help_index_element_new *this_help;
  char entry[MAX_STRING_LENGTH];
  char key1[256], key2[256], key3[256], key4[256], key5[256], rec_level[256];

  if (!ch->desc)
    return eFAILURE;

  skip_spaces(&argument);

  if (!*argument)
  {
    if (ch->isMortal())
      ch->send(new_help);
    else
      ch->send(new_ihelp);
    return eFAILURE;
  }

  if (!new_help_table)
  {
    ch->sendln("No help available.");
    return eFAILURE;
  }

  char *upper_argument = str_dup(argument);
  upper_argument = remove_trailing_spaces(upper_argument);

  if (!(this_help = find_help(upper_argument)))
  {
    snprintf(buf, 256, "There is no help entry for \'%s\'.\r\n",
             upper_argument);
    ch->send(buf);

    // Find similar help entries based on the Levenshtein distance
    // between keywords.

    int h;
    unsigned int l;
    unsigned int argSize = strlen(argument);
    std::multimap<unsigned int, char *, ltstr> ltable;
    std::multimap<unsigned int, char *, ltstr>::iterator cur;

    int level = ch->getLevel() == 0 ? 1 : ch->getLevel();

    for (h = 0; h < new_top_of_helpt; h++)
    {
      if (new_help_table[h].min_level > level)
      {
        continue;
      }

      if (new_help_table[h].keyword1)
      {
        l = levenshtein(argument, new_help_table[h].keyword1);
        ltable.insert(std::pair<int, char *>(l, new_help_table[h].keyword1));
      }

      if (new_help_table[h].keyword2)
      {
        l = levenshtein(argument, new_help_table[h].keyword2);
        ltable.insert(std::pair<int, char *>(l, new_help_table[h].keyword2));
      }

      if (new_help_table[h].keyword3)
      {
        l = levenshtein(argument, new_help_table[h].keyword3);
        ltable.insert(std::pair<int, char *>(l, new_help_table[h].keyword3));
      }

      if (new_help_table[h].keyword4)
      {
        l = levenshtein(argument, new_help_table[h].keyword4);
        ltable.insert(std::pair<int, char *>(l, new_help_table[h].keyword4));
      }

      if (new_help_table[h].keyword5)
      {
        l = levenshtein(argument, new_help_table[h].keyword5);
        ltable.insert(std::pair<int, char *>(l, new_help_table[h].keyword5));
      }
    }

    if (ltable.size() > 0)
    {
    }

    std::vector<char *> results;
    for (cur = ltable.begin(); cur != ltable.end(); cur++)
    {
      if (find(results.begin(), results.end(), (*cur).second) == results.end())
      {
        // Skip words which matches less than 50% of the original
        if ((*cur).first > argSize / 2)
          continue;

        if (results.size() == 0)
        {
          ch->send("Suggested help entries: ");
        }

        results.push_back((*cur).second);
        csendf(ch, "%s", (*cur).second);

        if (results.size() >= 5)
        {
          ch->sendln("");
          break;
        }
        else
        {
          ch->send(", ");
        }
      }
    }

    sprintf(buf, "'%s' has no help entry.  %s just tried to call it.",
            upper_argument, GET_NAME(ch));
    logentry(buf, IMMORTAL, LogChannels::LOG_HELP);

    dc_free(upper_argument);
    return eFAILURE;
  }

  dc_free(upper_argument);
  int a = ch->getLevel() == 0 ? 1 : ch->getLevel();
  if (this_help->min_level > a)
  {
    ch->sendln("There is no help on that word.");
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
  sprintf(entry,
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
          buf, ((ch->getLevel() >= IMMORTAL) ? rec_level : " "),
          ((this_help->min_level < IMMORTAL) ? " " : "\r\nImmortal-only command.\r\n"),
          this_help->entry, this_help->related);

  if (cmd)
    page_string(ch->desc, entry, 1);
  else
    ch->send(entry);

  return eSUCCESS;
}

struct help_index_element_new *find_help(char *keyword)
{
  int i;

  if (!strcmp(keyword, "NONE"))
    return nullptr;

  for (i = 0; i < (int)strlen(keyword); i++)
    keyword[i] = UPPER(keyword[i]);

  for (i = 0; i < new_top_of_helpt; i++)
    if (!strcmp(keyword, new_help_table[i].keyword1) ||
        !strcmp(keyword, new_help_table[i].keyword2) ||
        !strcmp(keyword, new_help_table[i].keyword3) ||
        !strcmp(keyword, new_help_table[i].keyword4) ||
        !strcmp(keyword, new_help_table[i].keyword5))
      return (new_help_table + i);

  return nullptr;
}

#define ENTRY_MAX 32384

int load_new_help(FILE *fl, int reload, Character *ch)
{
  char entry[ENTRY_MAX], line[READ_SIZE + 1], tmpentry[ENTRY_MAX], buf[256], tmpbuffer[ENTRY_MAX];
  struct help_index_element_new new_help;
  int version = 0, level = -1, linenum = 0;

  linenum += get_line(fl, line);
  if (sscanf(line, "@Version: %d", &version) != 1)
  {
    if (reload == 1)
    {
      logentry(QStringLiteral("Error in verion number in help file.\r\n"), OVERSEER, LogChannels::LOG_HELP);
      return eFAILURE;
    }
    else
    {
      perror("Error in verion number in help file.\r\n");
      abort();
    }
  }

  linenum += get_line(fl, line);

  while (*line != '$')
  {
    new_help.keyword1 = str_hsh(line);

    linenum += get_line(fl, line);
    new_help.keyword2 = str_hsh(line);

    linenum += get_line(fl, line);
    new_help.keyword3 = str_hsh(line);

    linenum += get_line(fl, line);
    new_help.keyword4 = str_hsh(line);

    linenum += get_line(fl, line);
    new_help.keyword5 = str_hsh(line);

    linenum += get_line(fl, line);
    new_help.related = str_hsh(line);

    linenum += get_line(fl, line);
    if (sscanf(line, "L: %d", &level) == 1)
    {
      new_help.min_level = level;
    }
    else
    {
      new_help.min_level = 0;
    }

    linenum += get_line(fl, line);
    // E: Here.
    linenum += get_line_with_space(fl, line);
    *tmpentry = '\0';
    while (*line != '#')
    {
      if (*line == '\0')
      {
        snprintf(tmpbuffer, ENTRY_MAX, "%s\r\n", tmpentry);
        strncpy(tmpentry, tmpbuffer, ENTRY_MAX);
      }
      else
      {
        snprintf(tmpbuffer, ENTRY_MAX, "%s%s\r\n", tmpentry, line);
        strncpy(tmpentry, tmpbuffer, ENTRY_MAX);
      }
      linenum += get_line_with_space(fl, line);
    }

    if (strlen(tmpentry) > MAX_HELP_LENGTH)
      tmpentry[MAX_HELP_LENGTH - 1] = '\0';

    if (!(new_help.entry = str_hsh(tmpentry)))
      new_help.entry = str_hsh("Error reading help entry. Please notify an Immortal!\n");

    new_help_table[new_top_of_helpt] = new_help;
    new_top_of_helpt++;

    level = -1;
    *entry = '\0';

    linenum += get_line(fl, line);
    // ~ is here.
    linenum += get_line(fl, line);
  }

  if (reload == 1)
  {
    sprintf(buf, "%s just reloaded the help files.", GET_NAME(ch));
    logentry(buf, OVERSEER, LogChannels::LOG_HELP);
  }
  return eSUCCESS;
}

int do_areas(Character *ch, char *arg, int cmd)
{
  strcpy(arg, "areas");
  return do_new_help(ch, arg, cmd);
}

char help_buf[MAX_STRING_LENGTH * 4];

int do_hindex(Character *ch, char *argument, int cmd)
{
  int i, minlen, count = 0;
  char arg[256];

  half_chop(argument, argument, arg);
  if (!*argument)
  {
    csendf(ch, "Usage: hindex <ID#>\r\n"
               "       hindex <low ID#> <high ID#>  (you can display up to 30 at a time)\r\n"
               "       hindex <start of a word(s)>\r\n"
               "       hindex -<l|i|u|d>\n\r"
               "              l = level, l <start> <end>\r\n"
               "              i = immortal\r\n"
               "              d = defunct, < level 0 OR > level %d\r\n"
               "              u = unfinished, level 75\r\n"
               "\r\n",
           IMPLEMENTER);
    return eFAILURE;
  }
  int start = 0;
  if (*argument == '-')
  { // we are doing a function, not a normal search
    if ((*(argument + 1) == 'l' || *(argument + 1) == 'L'))
    { // show help based on level range, excluded all level 1's
      // half_chop(arg, argument, arg);
      char arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
      argument = &arg[0];
      argument = one_argument(argument, arg2);
      argument = one_argument(argument, arg3);
      one_argument(argument, arg);
      if (arg[0] && is_number(arg))
        start = atoi(arg);
      argument = &arg2[0];
      strcpy(arg, arg3);
      if ((((atoi(argument)) > 0) || *argument == '0') && ((atoi(arg)) > 0))
      { // not valid numbers
        if (atoi(argument) > atoi(arg))
        {
          ch->sendln("Usage: hindex -l <low level> <high level>");
          return eFAILURE;
        }
        if (!*arg)
          sprintf(arg, "%s", argument); // if they left off the second arg, copy the first, show only one level
        show_help_header(ch);
        for (i = 0; i < new_top_of_helpt; i++)
        {
          if (new_help_table[i].min_level >= atoi(argument) && new_help_table[i].min_level <= atoi(arg) && start-- <= 0) // too many level 1s, so we must exclude them or get an OVERFLOW
            count = show_one_help_entry(i, ch, count);
        }
        show_help_bar(ch);
      }
    }
    else if ((*(argument + 1) == 'u' || *(argument + 1) == 'U'))
    { // unfinished help = level 75
      show_help_header(ch);
      for (i = 0; i < new_top_of_helpt; i++)
      {
        if (new_help_table[i].min_level == 75)
          count = show_one_help_entry(i, ch, count);
      }
      show_help_bar(ch);
    }
    else if ((*(argument + 1) == 'd' || *(argument + 1) == 'D'))
    { // show defunct ones with out of range levels
      show_help_header(ch);
      for (i = 0; i < new_top_of_helpt; i++)
      {
        if (new_help_table[i].min_level <= 0 || new_help_table[i].min_level > IMPLEMENTER)
          count = show_one_help_entry(i, ch, count);
      }
      show_help_bar(ch);
    }
    else if ((*(argument + 1) == 'i' || *(argument + 1) == 'I'))
    { // show defunct ones with out of range levels
      show_help_header(ch);
      for (i = 0; i < new_top_of_helpt; i++)
      {
        if (new_help_table[i].min_level >= IMMORTAL && new_help_table[i].min_level <= IMPLEMENTER)
          count = show_one_help_entry(i, ch, count);
      }
      show_help_bar(ch);
    }
  }
  else if ((((atoi(argument)) > 0) || *argument == '0') && ((atoi(arg)) > 0))
  { // index #s out of range
    if (atoi(argument) > atoi(arg))
    {
      ch->sendln("Usage: hindex <low ID#> <high ID#>"); // wrong order, first > second
      return eFAILURE;
    }
    else if ((atoi(arg) - atoi(argument)) >= 30)
    { // too many listed, only 30 at a time or we get too much spam
      ch->sendln("You can only list 30 help entries at a time.");
      return eFAILURE;
    }
    else if (atoi(argument) >= new_top_of_helpt || atoi(arg) >= new_top_of_helpt)
    {
      ch->sendln("Out of range."); // wrong order, first > second
      return eFAILURE;
    }

    show_help_header(ch);
    for (i = atoi(argument); i <= atoi(arg); i++)
    {
      count = show_one_help_entry(i, ch, count);
    }
    show_help_bar(ch);
  }
  else if (((atoi(argument)) > 0) || *argument == '0')
  { // show a specific ID #
    if (atoi(argument) >= new_top_of_helpt)
    {
      ch->sendln("Out of range."); // wrong order, first > second
      return eFAILURE;
    }

    show_help_header(ch);
    count = show_one_help_entry(atoi(argument), ch, count);
    show_help_bar(ch);
  }
  else
  { // we are searching based on keywords, show as many as you find
    minlen = strlen(argument);
    show_help_header(ch);
    for (i = 0; i < new_top_of_helpt; i++)
    {
      if (!strn_cmp(argument, new_help_table[i].keyword1, minlen) ||
          !strn_cmp(argument, new_help_table[i].keyword2, minlen) ||
          !strn_cmp(argument, new_help_table[i].keyword3, minlen) ||
          !strn_cmp(argument, new_help_table[i].keyword4, minlen) ||
          !strn_cmp(argument, new_help_table[i].keyword5, minlen))
      {
        count = show_one_help_entry(i, ch, count);
      }
    }
    show_help_bar(ch);
  }
  ch->send(help_buf);
  ch->send(QStringLiteral("$B$7Total Shown: $B$5%1$R\r\n").arg(count));
  ch->send(QStringLiteral("$B$7Total Help Entries: $B$5%1$R\r\n").arg(new_top_of_helpt));

  return eSUCCESS;
}

int do_index(Character *ch, char *argument, int cmd)
{
  int i, minlen, count = 0;
  char arg[256];

  half_chop(argument, argument, arg);
  if (!*argument)
  {
    csendf(ch, "Usage: index <ID#>\r\n"
               "       index <low ID#> <high ID#>  (you can display up to 30 at a time)\r\n"
               "       index <start of a word(s)>\r\n"
               "\r\n");
    return eFAILURE;
  }

  if ((((atoi(argument)) > 0) || *argument == '0') && ((atoi(arg)) > 0))
  { // index #s out of range
    if (atoi(argument) > atoi(arg))
    {
      ch->sendln("Usage: index <low ID#> <high ID#>"); // wrong order, first > second
      return eFAILURE;
    }
    if (atoi(argument) >= new_top_of_helpt || atoi(arg) >= new_top_of_helpt)
    {
      ch->sendln("Out of range."); // wrong order, first > second
      return eFAILURE;
    }
    show_help_header(ch);
    for (i = atoi(argument); i <= atoi(arg); i++)
    {
      if (new_help_table[i].min_level > 1)
        continue;
      if (count >= 30)
        break;
      count = show_one_help_entry(i, ch, count);
    }
    show_help_bar(ch);
  }
  else if (((atoi(argument)) > 0) || *argument == '0')
  { // show a specific ID #
    if (atoi(argument) >= new_top_of_helpt)
    {
      ch->sendln("Out of range.");
      return eFAILURE;
    }

    if (new_help_table[atoi(argument)].min_level > 1)
      ch->sendln("You are not high enough level to view this helpfile.");
    else
    {
      show_help_header(ch);
      count = show_one_help_entry(atoi(argument), ch, count);
      show_help_bar(ch);
    }
  }
  else
  { // we are searching based on keywords, show as many as you find
    minlen = strlen(argument);
    show_help_header(ch);
    for (i = 0; i < new_top_of_helpt; i++)
    {
      if (new_help_table[i].min_level > 1)
        continue;
      if (count >= 30)
        break;
      if (!strn_cmp(argument, new_help_table[i].keyword1, minlen) ||
          !strn_cmp(argument, new_help_table[i].keyword2, minlen) ||
          !strn_cmp(argument, new_help_table[i].keyword3, minlen) ||
          !strn_cmp(argument, new_help_table[i].keyword4, minlen) ||
          !strn_cmp(argument, new_help_table[i].keyword5, minlen))
      {
        count = show_one_help_entry(i, ch, count);
      }
    }
    show_help_bar(ch);
  }
  ch->send(help_buf);
  ch->send(QStringLiteral("$B$7Total Shown: $B$5%1$R\r\n").arg(count));
  ch->send(QStringLiteral("$B$7Total Help Entries: $B$5%1$R\r\n").arg(new_top_of_helpt));

  return eSUCCESS;
}

int show_one_help_entry(int entry, Character *ch, int count)
{

  csendf(ch, "$B$6%3d $7- $5%3d $7[$3%-20.20s$7] [$3%-20.20s$B$7] [$3%-20.20s$B$7] "
             "[$3%-20.20s$B$7] [$3%-20.20s$B$7]\n\r",
         entry,
         (new_help_table[entry].min_level >= 0 ? new_help_table[entry].min_level : 999),
         (*new_help_table[entry].keyword1 ? new_help_table[entry].keyword1 : "None"),
         (*new_help_table[entry].keyword2 ? new_help_table[entry].keyword2 : "None"),
         (*new_help_table[entry].keyword2 ? new_help_table[entry].keyword3 : "None"),
         (*new_help_table[entry].keyword2 ? new_help_table[entry].keyword4 : "None"),
         (*new_help_table[entry].keyword3 ? new_help_table[entry].keyword5 : "None"));
  return ++count;
}

void show_help_header(Character *ch)
{
  send_to_char("$B$6ID# $B$7- $RLVL $3 Keyword 1              Keyword 2              Keyword 3"
               "              Keyword 4              Keyword 5\r\n",
               ch);
  show_help_bar(ch);
}

void show_help_bar(Character *ch)
{
  send_to_char("$B$7--------------------------------------------------------------------------"
               "--------------------------------------------------\r\n$R",
               ch);
}

/* strn_cmp: a case-insensitive version of strncmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
int strn_cmp(char *arg1, char *arg2, int n)
{
  int chk, i;

  for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--)
  {
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i))))
    {
      if (chk < 0)
        return (-1);
      else
        return (1);
    }
  }
  return (0);
}

int do_reload_help(Character *ch, char *argument, int cmd)
{

  FILE *new_help_fl;
  int help_rec_count = 0, ret = 0;

  // ch->sendln("Command disabled!");
  // return eFAILURE;

  if (!(new_help_fl = fopen(NEW_HELP_FILE, "r")))
  {
    logentry(QStringLiteral("Error opening help file for reload."), OVERSEER, LogChannels::LOG_HELP);
    return eFAILURE;
  }

  help_rec_count = count_hash_records(new_help_fl);
  fclose(new_help_fl);

  if (!(new_help_fl = fopen(NEW_HELP_FILE, "r")))
  {
    logentry(QStringLiteral("Error opening help file for reload."), OVERSEER, LogChannels::LOG_HELP);
    return eFAILURE;
  }

  FREE(new_help_table);
  new_top_of_helpt = 0;
  CREATE(new_help_table, struct help_index_element_new, help_rec_count);
  ret = load_new_help(new_help_fl, 1, ch);
  fclose(new_help_fl);

  if (ret == eFAILURE)
  {
    ch->sendln("Error reloading help files!");
    return eFAILURE;
  }

  ch->sendln("Help files reloaded.");
  return eSUCCESS;
}

int do_hedit(Character *ch, char *argument, int cmd)
{
  char buf[200], buf2[200], field[200], buf3[200], value[200];
  struct help_index_element_new new_help;
  int help_id = -1, i, key_id = -1, level = -1;

  if (IS_NPC(ch))
    return eFAILURE;

  if (!ch->has_skill(COMMAND_HEDIT))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  half_chop(argument, buf, buf2);

  if (!*buf)
  {
    show_hedit_usage(ch);
    return eFAILURE;
  }

  if (!str_cmp(buf, "save"))
  { // save all help entries
    save_help(ch);
    return eSUCCESS;
  }

  if (!*buf2)
  {
    show_hedit_usage(ch);
    return eFAILURE;
  }

  if (!str_cmp(buf, "new"))
  { // New Help Entry
    for (i = 0; i < (int)strlen(buf2); i++)
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
    ch->send(buf);
    new_top_of_helpt++;
    sprintf(buf, "%s just created a help file for '%s'.", GET_NAME(ch), buf2);
    logentry(buf, OVERSEER, LogChannels::LOG_HELP);
  }
  else if ((help_id = atoi(buf)) || *buf == '0')
  { // Edit a specific help entry
    if (*buf == 0)
      help_id = 0;
    if (help_id < 0 || help_id >= new_top_of_helpt)
    {
      ch->sendln("Not a valid help ID number.  Try using 'hindex'");
      return eFAILURE;
    }
    half_chop(buf2, field, buf2);
    if (is_abbrev(field, "key"))
    { // changing one of the keys
      half_chop(buf2, buf3, value);
      if (!*buf3 || !*value)
      {
        ch->sendln("Not a valid key # or no value specified.");
        return eFAILURE;
      }
      if ((key_id = atoi(buf3)))
      {
        if (strlen(value) > MAX_HELP_KEYWORD_LENGTH)
        {
          value[MAX_HELP_KEYWORD_LENGTH - 1] = '\0';
        }
        for (i = 0; i < (int)strlen(value); i++)
          value[i] = UPPER(value[i]);
        switch (key_id)
        {
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
          ch->sendln("Not a valid key #.");
          return eFAILURE;
          break;
        }
        sprintf(buf, "Keyword %d changed to '%s' for ID# %d.\r\n", key_id, value, help_id);
        ch->send(buf);
      }
      else
      {
        ch->sendln("Not a valid key #.");
        return eFAILURE;
      }
    }
    else if (is_abbrev(field, "level"))
    { // changing the level
      if (*buf2 && ((level = atoi(buf2)) || *buf2 == '0') && level >= 0 && level <= 110)
      {
        if (*buf2 == '0')
          level = 0;
        new_help_table[help_id].min_level = level;
        sprintf(buf, "Level changed to '%d' for ID# %d.\r\n", level, help_id);
        ch->send(buf);
      }
      else
      {
        ch->sendln("Invalid level specified.");
      }
    }
    else if (is_abbrev(field, "related"))
    { // changing the related
      if (*buf2)
      {
        if (strlen(buf2) > MAX_HELP_RELATED_LENGTH)
        {
          buf2[MAX_HELP_KEYWORD_LENGTH - 1] = '\0';
        }
        for (i = 0; i < (int)strlen(buf2); i++)
          buf2[i] = UPPER(buf2[i]);
        new_help_table[help_id].related = str_hsh(buf2);
        sprintf(buf, "Related changed to '%s' for ID# %d.\r\n", buf2, help_id);
        ch->send(buf);
      }
      else
      {
        show_hedit_usage(ch);
        return eFAILURE;
      }
    }
    else if (is_abbrev(field, "entry"))
    { // changing the actual help entry
      ch->desc->backstr = nullptr;
      send_to_char("        Write your help entry and stay within the line.  (/s saves /h for help)\r\n"
                   "   |--------------------------------------------------------------------------------|\r\n",
                   ch);
      if (new_help_table[help_id].entry)
      {
        ch->desc->backstr = str_dup(new_help_table[help_id].entry);
        ch->send(ch->desc->backstr);
      }

      ch->desc->connected = Connection::states::EDITING;
      ch->desc->strnew = &(new_help_table[help_id].entry);
      ch->desc->max_str = MAX_HELP_LENGTH;
    }
    else
    { // no idea wtf they are doing
      show_hedit_usage(ch);
      return eFAILURE;
    }
  }
  else
  {
    show_hedit_usage(ch);
    return eFAILURE;
  }

  return eSUCCESS;
}

void show_hedit_usage(Character *ch)
{

  send_to_char("$3Syntax$R: hedit <id#> <field> [arg] [value]\r\n"
               "        hedit new <first keyword>\r\n"
               "        hedit save\r\n\r\n"
               "  Fields: key, level, entry, related\r\n"
               "  Args: Only apply to key in which it is 1-5\r\n"
               "  Values: The value of the specified field.  Does not affect entry\r\n",
               ch);
}

void save_help(Character *ch)
{
  FILE *f;
  int i;
  char file[256], buf[256];

  sprintf(file, "%s", NEW_HELP_FILE);

  if ((f = fopen(file, "w")) == nullptr)
  {
    ch->sendln("Couldn't open help file for saving.");
    perror("Couldn't open help file for saving.\r\n");
    return;
  }

  fprintf(f, "@Version: 2\n");

  for (i = 0; i < new_top_of_helpt; i++)
  {
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

  fclose(f);
  ch->sendln("Saved.");
  sprintf(buf, "%s just saved the help files.", GET_NAME(ch));
  logentry(buf, OVERSEER, LogChannels::LOG_HELP);

  sprintf(file, "%s", WEB_HELP_FILE);

  if ((f = fopen(file, "w")) == nullptr)
  {
    ch->sendln("Couldn't open web help file for saving.");
    perror("Couldn't open web help file for saving.\r\n");
    return;
  }

  for (i = 0; i < new_top_of_helpt; i++)
  {
    if (new_help_table[i].min_level <= DC::MAX_MORTAL_LEVEL)
    {
      help_string_to_file(f, new_help_table[i].keyword1);
      help_string_to_file(f, new_help_table[i].keyword2);
      help_string_to_file(f, new_help_table[i].keyword3);
      help_string_to_file(f, new_help_table[i].keyword4);
      help_string_to_file(f, new_help_table[i].keyword5);
      //      help_string_to_file(f, new_help_table[i].related);
      //      fprintf(f, "L: %d\n", new_help_table[i].min_level);
      //      fprintf(f, "E:\n");
      help_string_to_file(f, new_help_table[i].entry);
      fprintf(f, "#\n");
      //      fprintf(f, "~\n");
    }
  }

  // end file
  fprintf(f, "$~\n");

  fclose(f);
}

void help_string_to_file(FILE *f, char *str)
{
  char *newbuf = new char[strlen(str) + 1];
  strcpy(newbuf, str);

  // remove all \r's
  for (char *curr = newbuf; *curr != '\0'; curr++)
  {
    if (*curr == '\r')
    {
      for (char *blah = curr; *blah != '\0'; blah++) // shift the rest of the std::string 1 left
        *blah = *(blah + 1);
      curr--; // (to check for \r\r cases)
    }
  }

  if (newbuf[strlen(newbuf) - 1] == '\n')
    newbuf[strlen(newbuf) - 1] = '\0';

  fprintf(f, "%s\n", newbuf);
  delete[] newbuf;
}

int get_line_with_space(FILE *fl, char *buf)
{
  char temp[256];
  int lines = 0;

  do
  {
    lines++;
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && *temp == '*');
  // } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl))
    return 0;
  else
  {
    strcpy(buf, temp);
    return lines;
  }
}
