
#include <qcompare.h>
#include <qsharedpointer.h>
#include <qstring.h>
#include <qswap.h>
#include <stdio.h>
#include <stdlib.h>
#include <cctype>
#include <cstring>
#include <map>
#include <vector>
#include <algorithm>

#include "DC/structs.h"
#include "DC/character.h"
#include "DC/DC.h"
#include "DC/utility.h"
#include "DC/db.h" // exp_table
#include "DC/interp.h"
#include "DC/connect.h"
#include "DC/spells.h"
#include "DC/returnvals.h"
#include "DC/help.h"
#include "DC/common.h"
#include "DC/memory.h"
#include "DC/dcstdio.h"
#include "DC/levels.h"

// Externs
extern void skip_spaces(char **string);
extern help_index_element_new *new_help_table;
int get_line(FILEPtr fl, char *buf);
int is_abbrev(const char *arg1, const char *arg2);
void help_string_to_file(FILEPtr f, QString helpstr);

// locals
help_index_element_new *find_help(QString keyword);
int strn_cmp(char *arg1, char *arg2, int n);
int count_hash_records(FILEPtr fl);
void show_hedit_usage(Character *ch);
void save_help(Character *ch);
int get_line_with_space(FILEPtr fl, char *buf);
int show_one_help_entry(int entry, Character *ch, int count);
void show_help_header(Character *ch);
void show_help_bar(Character *ch);

// da functions
int do_mortal_help(Character *ch, char *argument, cmd_t cmd)
{
  extern char new_help[MAX_STRING_LENGTH];
  ch->send(new_help);
  return ReturnValue::eSUCCESS;
}

class ltstr
{
public:
  bool operator()(int a, int b) const
  {
    return a < b;
  }
};

int levenshtein(QString s, QString t)
{
  unsigned int i, j, n, m, cost;
  unsigned int d[MAX_INPUT_LENGTH + 1][MAX_HELP_KEYWORD_LENGTH + 1];

  m = s.size();
  n = t.size();

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
      if (s[i].toLower() == t[j].toLower())
        cost = 0;
      else
        cost = 1;
      d[i][j] = std::min(std::min(d[i - 1][j] + 1,
                                  d[i][j - 1] + 1),
                         d[i - 1][j - 1] + cost);
    }

  return d[m][n];
}

command_return_t Character::do_new_help(QStringList arguments, cmd_t cmd)
{
  extern char new_help[MAX_STRING_LENGTH];
  extern char new_ihelp[MAX_STRING_LENGTH];
  help_index_element_new *this_help{};
  QString entry;
  QString key1, key2, key3, key4, key5, rec_level;

  if (!desc)
    return ReturnValue::eFAILURE;

  if (arguments.isEmpty())
  {
    if (isMortalPlayer())
      send(new_help);
    else
      send(new_ihelp);
    return ReturnValue::eFAILURE;
  }

  if (!new_help_table)
  {
    sendln("No help available.");
    return ReturnValue::eFAILURE;
  }

  auto arg1 = arguments.at(0);

  if (auto this_help = find_help(arg1); !this_help)
  {
    sendln(u"There is no help entry for \'%1\'."_s.arg(arg1));

    // Find similar help entries based on the Levenshtein distance
    // between keywords.

    int h;
    unsigned int l;
    unsigned int argSize = arg1.length();
    std::multimap<unsigned int, QString, ltstr> ltable;
    std::multimap<unsigned int, QString, ltstr>::iterator cur;

    int level = getLevel() == 0 ? 1 : getLevel();

    for (h = 0; h < DC::getInstance()->new_top_of_helpt; h++)
    {
      if (new_help_table[h].min_level > level)
      {
        continue;
      }

      if (new_help_table[h].keyword1)
      {
        l = levenshtein(arg1, new_help_table[h].keyword1);
        ltable.insert(std::pair<unsigned int, QString>(l, new_help_table[h].keyword1));
      }

      if (new_help_table[h].keyword2)
      {
        l = levenshtein(arg1, new_help_table[h].keyword2);
        ltable.insert(std::pair<unsigned int, QString>(l, new_help_table[h].keyword2));
      }

      if (new_help_table[h].keyword3)
      {
        l = levenshtein(arg1, new_help_table[h].keyword3);
        ltable.insert(std::pair<unsigned int, QString>(l, new_help_table[h].keyword3));
      }

      if (new_help_table[h].keyword4)
      {
        l = levenshtein(arg1, new_help_table[h].keyword4);
        ltable.insert(std::pair<unsigned int, QString>(l, new_help_table[h].keyword4));
      }

      if (new_help_table[h].keyword5)
      {
        l = levenshtein(arg1, new_help_table[h].keyword5);
        ltable.insert(std::pair<unsigned int, QString>(l, new_help_table[h].keyword5));
      }
    }

    if (ltable.size() > 0)
    {
    }

    std::vector<QString> results;
    for (cur = ltable.begin(); cur != ltable.end(); cur++)
    {
      if (find(results.begin(), results.end(), (*cur).second) == results.end())
      {
        // Skip words which matches less than 50% of the original
        if ((*cur).first > argSize / 2)
          continue;

        if (results.size() == 0)
        {
          send("Suggested help entries: ");
        }

        results.push_back((*cur).second);
        send((*cur).second);

        if (results.size() >= 5)
        {
          sendln("");
          break;
        }
        else
        {
          send(", ");
        }
      }
    }

    logentry(u"'%1' has no help entry.  %2 just tried to call it."_s.arg(arg1).arg(getName()), IMMORTAL, DC::LogChannel::LOG_HELP);
    return ReturnValue::eFAILURE;
  }

  int a = getLevel() == 0 ? 1 : getLevel();
  if (this_help->min_level > a)
  {
    sendln("There is no help on that word.");
    return ReturnValue::eFAILURE;
  }

  key1 = u"'%1'"_s.arg(this_help->keyword1);
  key2 = u"'%1'"_s.arg(this_help->keyword2);
  key3 = u"'%1'"_s.arg(this_help->keyword3);
  key4 = u"'%1'"_s.arg(this_help->keyword4);
  key5 = u"'%1'"_s.arg(this_help->keyword5);

  auto buf = u"%1 %2 %3 %4 %5"_s
                 .arg(key1)
                 .arg(this_help->keyword2 && key2 != u"'NONE'"_s ? key2 : u" "_s)
                 .arg(this_help->keyword3 && key3 != u"'NONE'"_s ? key3 : u" "_s)
                 .arg(this_help->keyword3 && key4 != u"'NONE'"_s ? key4 : u" "_s)
                 .arg(this_help->keyword3 && key5 != u"'NONE'"_s ? key5 : u" "_s);

  rec_level = u"\r\nLevel Required: %1"_s.arg(this_help->min_level);
  entry = u"$1$B+=+=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=+=+\r\n"
          "$1$B| | $B$5Help For: $B$7%1 $1$B| |\r\n"
          "$1$B+=+=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=+=+\r\n"
          "$B$7%2%3$R"
          "\r\n"
          "$R%4"
          "\r\n"
          "$1$B+=+=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=+=+\r\n"
          "$1$B| | $B$5Related Help: $B$7%5 $1$B| |\r\n"
          "$1$B+=+=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=+=+\r\n$R"_s
              .arg(buf, 61)
              .arg(isImmortalPlayer() ? rec_level : u" "_s)
              .arg(this_help->min_level < IMMORTAL ? u" "_s : u"\r\nImmortal-only command.\r\n"_s)
              .arg(this_help->entry)
              .arg(this_help->related, 57);

  if (cmd != cmd_t::UNDEFINED)
    page_string(desc, qPrintable(entry), 1);
  else
    send(entry);

  return ReturnValue::eSUCCESS;
}

help_index_element_new *find_help(QString keyword)
{
  keyword = keyword.toUpper();
  if (keyword == "NONE")
    return {};

  for (auto i = 0; i < DC::getInstance()->new_top_of_helpt; i++)
    if (keyword == new_help_table[i].keyword1 ||
        keyword == new_help_table[i].keyword2 ||
        keyword == new_help_table[i].keyword3 ||
        keyword == new_help_table[i].keyword4 ||
        keyword == new_help_table[i].keyword5)
      return (new_help_table + i);

  return nullptr;
}

#define ENTRY_MAX 32384

int load_new_help(FILEPtr fl, int reload, Character *ch)
{
  char entry[ENTRY_MAX], line[READ_SIZE + 1], tmpentry[ENTRY_MAX], buf[256], tmpbuffer[ENTRY_MAX];
  help_index_element_new new_help;
  int version = 0, level = -1, linenum = 0;

  linenum += get_line(fl, line);
  if (sscanf(line, "@Version: %d", &version) != 1)
  {
    if (reload == 1)
    {
      logentry(QStringLiteral("Error in verion number in help file.\r\n"), OVERSEER, DC::LogChannel::LOG_HELP);
      return ReturnValue::eFAILURE;
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

    new_help.entry = tmpentry;
    if (new_help.entry.isEmpty())
      new_help.entry = str_hsh("Error reading help entry. Please notify an Immortal!\n");

    new_help_table[DC::getInstance()->new_top_of_helpt] = new_help;
    DC::getInstance()->new_top_of_helpt++;

    level = -1;
    *entry = '\0';

    linenum += get_line(fl, line);
    // ~ is here.
    linenum += get_line(fl, line);
  }

  if (reload == 1)
  {
    if (ch)
    {
      sprintf(buf, "%s just reloaded the help files.", GET_NAME(ch));
    }
    logentry(buf, OVERSEER, DC::LogChannel::LOG_HELP);
  }
  return ReturnValue::eSUCCESS;
}

int do_areas(Character *ch, char *arg, cmd_t cmd)
{
  strcpy(arg, "areas");
  return ch->do_new_help(QString(arg).split(' '), cmd);
}

char help_buf[MAX_STRING_LENGTH * 4];

int do_hindex(Character *ch, char *argument, cmd_t cmd)
{
  int i, minlen, count = 0;
  char arg[256];

  half_chop(argument, argument, arg);
  if (!*argument)
  {
    csendf(ch, "Usage: hindex <ID#>\r\n"
               "       hindex <low ID#> <high ID#>  (you can display up to 30 at a time)\r\n"
               "       hindex <start of a word(s)>\r\n"
               "       hindex -<l|i|u|d>\r\n"
               "              l = level, l <start> <end>\r\n"
               "              i = immortal\r\n"
               "              d = defunct, < level 0 OR > level %d\r\n"
               "              u = unfinished, level 75\r\n"
               "\r\n",
           IMPLEMENTER);
    return ReturnValue::eFAILURE;
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
          return ReturnValue::eFAILURE;
        }
        if (!*arg)
          sprintf(arg, "%s", argument); // if they left off the second arg, copy the first, show only one level
        show_help_header(ch);
        for (i = 0; i < DC::getInstance()->new_top_of_helpt; i++)
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
      for (i = 0; i < DC::getInstance()->new_top_of_helpt; i++)
      {
        if (new_help_table[i].min_level == 75)
          count = show_one_help_entry(i, ch, count);
      }
      show_help_bar(ch);
    }
    else if ((*(argument + 1) == 'd' || *(argument + 1) == 'D'))
    { // show defunct ones with out of range levels
      show_help_header(ch);
      for (i = 0; i < DC::getInstance()->new_top_of_helpt; i++)
      {
        if (new_help_table[i].min_level <= 0 || new_help_table[i].min_level > IMPLEMENTER)
          count = show_one_help_entry(i, ch, count);
      }
      show_help_bar(ch);
    }
    else if ((*(argument + 1) == 'i' || *(argument + 1) == 'I'))
    { // show defunct ones with out of range levels
      show_help_header(ch);
      for (i = 0; i < DC::getInstance()->new_top_of_helpt; i++)
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
      return ReturnValue::eFAILURE;
    }
    else if ((atoi(arg) - atoi(argument)) >= 30)
    { // too many listed, only 30 at a time or we get too much spam
      ch->sendln("You can only list 30 help entries at a time.");
      return ReturnValue::eFAILURE;
    }
    else if (atoi(argument) >= DC::getInstance()->new_top_of_helpt || atoi(arg) >= DC::getInstance()->new_top_of_helpt)
    {
      ch->sendln("Out of range."); // wrong order, first > second
      return ReturnValue::eFAILURE;
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
    if (atoi(argument) >= DC::getInstance()->new_top_of_helpt)
    {
      ch->sendln("Out of range."); // wrong order, first > second
      return ReturnValue::eFAILURE;
    }

    show_help_header(ch);
    count = show_one_help_entry(atoi(argument), ch, count);
    show_help_bar(ch);
  }
  else
  { // we are searching based on keywords, show as many as you find
    minlen = strlen(argument);
    show_help_header(ch);
    for (i = 0; i < DC::getInstance()->new_top_of_helpt; i++)
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
  ch->send(QStringLiteral("$B$7Total Help Entries: $B$5%1$R\r\n").arg(DC::getInstance()->new_top_of_helpt));

  return ReturnValue::eSUCCESS;
}

int do_index(Character *ch, char *argument, cmd_t cmd)
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
    return ReturnValue::eFAILURE;
  }

  if ((((atoi(argument)) > 0) || *argument == '0') && ((atoi(arg)) > 0))
  { // index #s out of range
    if (atoi(argument) > atoi(arg))
    {
      ch->sendln("Usage: index <low ID#> <high ID#>"); // wrong order, first > second
      return ReturnValue::eFAILURE;
    }
    if (atoi(argument) >= DC::getInstance()->new_top_of_helpt || atoi(arg) >= DC::getInstance()->new_top_of_helpt)
    {
      ch->sendln("Out of range."); // wrong order, first > second
      return ReturnValue::eFAILURE;
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
    if (atoi(argument) >= DC::getInstance()->new_top_of_helpt)
    {
      ch->sendln("Out of range.");
      return ReturnValue::eFAILURE;
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
    for (i = 0; i < DC::getInstance()->new_top_of_helpt; i++)
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
  ch->send(QStringLiteral("$B$7Total Help Entries: $B$5%1$R\r\n").arg(DC::getInstance()->new_top_of_helpt));

  return ReturnValue::eSUCCESS;
}

int show_one_help_entry(int entry, Character *ch, int count)
{

  csendf(ch, "$B$6%3d $7- $5%3d $7[$3%-20.20s$7] [$3%-20.20s$B$7] [$3%-20.20s$B$7] "
             "[$3%-20.20s$B$7] [$3%-20.20s$B$7]\r\n",
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

int do_reload_help(Character *ch, char *argument, cmd_t cmd)
{

  FILEPtr new_help_fl;
  int help_rec_count = 0, ret = 0;

  // ch->sendln("Command disabled!");
  // return ReturnValue::eFAILURE;

  if (!(new_help_fl = dc_fopen(NEW_HELP_FILE, "r")))
  {
    logentry(QStringLiteral("Error opening help file for reload."), OVERSEER, DC::LogChannel::LOG_HELP);
    return ReturnValue::eFAILURE;
  }

  help_rec_count = count_hash_records(new_help_fl);

  new_help_fl = {};
  if (!(new_help_fl = dc_fopen(NEW_HELP_FILE, "r")))
  {
    logentry(QStringLiteral("Error opening help file for reload."), OVERSEER, DC::LogChannel::LOG_HELP);
    return ReturnValue::eFAILURE;
  }

  FREE(new_help_table);
  DC::getInstance()->new_top_of_helpt = 0;
  CREATE(new_help_table, help_index_element_new, help_rec_count);
  ret = load_new_help(new_help_fl, 1, ch);
  new_help_fl = {};

  if (ret == ReturnValue::eFAILURE)
  {
    ch->sendln("Error reloading help files!");
    return ReturnValue::eFAILURE;
  }

  ch->sendln("Help files reloaded.");
  return ReturnValue::eSUCCESS;
}

int do_hedit(Character *ch, char *argument, cmd_t cmd)
{
  char buf[200], buf2[200], field[200], buf3[200], value[200];
  help_index_element_new new_help;
  int help_id = -1, i, key_id = -1, level = -1;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (!ch->has_skill(COMMAND_HEDIT))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  half_chop(argument, buf, buf2);

  if (!*buf)
  {
    show_hedit_usage(ch);
    return ReturnValue::eFAILURE;
  }

  if (!str_cmp(buf, "save"))
  { // save all help entries
    save_help(ch);
    return ReturnValue::eSUCCESS;
  }

  if (!*buf2)
  {
    show_hedit_usage(ch);
    return ReturnValue::eFAILURE;
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

    RECREATE(new_help_table, help_index_element_new, DC::getInstance()->new_top_of_helpt + 1);
    new_help_table[DC::getInstance()->new_top_of_helpt] = new_help;
    sprintf(buf, "Help entry #%d added with keyword '%s'.\r\n", DC::getInstance()->new_top_of_helpt, buf2);
    ch->send(buf);
    DC::getInstance()->new_top_of_helpt++;
    sprintf(buf, "%s just created a help file for '%s'.", GET_NAME(ch), buf2);
    logentry(buf, OVERSEER, DC::LogChannel::LOG_HELP);
  }
  else if ((help_id = atoi(buf)) || *buf == '0')
  { // Edit a specific help entry
    if (*buf == 0)
      help_id = 0;
    if (help_id < 0 || help_id >= DC::getInstance()->new_top_of_helpt)
    {
      ch->sendln("Not a valid help ID number.  Try using 'hindex'");
      return ReturnValue::eFAILURE;
    }
    half_chop(buf2, field, buf2);
    if (is_abbrev(field, "key"))
    { // changing one of the keys
      half_chop(buf2, buf3, value);
      if (!*buf3 || !*value)
      {
        ch->sendln("Not a valid key # or no value specified.");
        return ReturnValue::eFAILURE;
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
          return ReturnValue::eFAILURE;
          break;
        }
        sprintf(buf, "Keyword %d changed to '%s' for ID# %d.\r\n", key_id, value, help_id);
        ch->send(buf);
      }
      else
      {
        ch->sendln("Not a valid key #.");
        return ReturnValue::eFAILURE;
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
        return ReturnValue::eFAILURE;
      }
    }
    else if (is_abbrev(field, "entry"))
    { // changing the actual help entry
      ch->desc->backstr = nullptr;
      send_to_char("        Write your help entry and stay within the line.  (/s saves /h for help)\r\n"
                   "   |--------------------------------------------------------------------------------|\r\n",
                   ch);
      if (!new_help_table[help_id].entry.isEmpty())
      {
        ch->desc->backstr = new_help_table[help_id].entry;
        ch->send(ch->desc->backstr);
      }

      ch->desc->connected = Connection::states::EDITING;
      ch->desc->strnew = &(new_help_table[help_id].entry);
      ch->desc->max_str = MAX_HELP_LENGTH;
    }
    else
    { // no idea wtf they are doing
      show_hedit_usage(ch);
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    show_hedit_usage(ch);
    return ReturnValue::eFAILURE;
  }

  return ReturnValue::eSUCCESS;
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
  int i;
  char file[256], buf[256];

  sprintf(file, "%s", NEW_HELP_FILE);
  LegacyFile lf(".", file, "Couldn't open help file '%1' for saving.");
  if (lf.isOpen())
  {
    dc_fprintf(lf.file_handle_, "@Version: 2\n");

    for (i = 0; i < DC::getInstance()->new_top_of_helpt; i++)
    {
      help_string_to_file(lf.file_handle_, new_help_table[i].keyword1);
      help_string_to_file(lf.file_handle_, new_help_table[i].keyword2);
      help_string_to_file(lf.file_handle_, new_help_table[i].keyword3);
      help_string_to_file(lf.file_handle_, new_help_table[i].keyword4);
      help_string_to_file(lf.file_handle_, new_help_table[i].keyword5);
      help_string_to_file(lf.file_handle_, new_help_table[i].related);
      dc_fprintf(lf.file_handle_, "L: %d\n", new_help_table[i].min_level);
      dc_fprintf(lf.file_handle_, "E:\n");
      help_string_to_file(lf.file_handle_, new_help_table[i].entry);
      dc_fprintf(lf.file_handle_, "#\n");
      dc_fprintf(lf.file_handle_, "~\n");
    }

    // end file
    dc_fprintf(lf.file_handle_, "$~\n");
  }
  else
  {
    ch->sendln("Couldn't open help file for saving.");
    return;
  }

  ch->sendln("Saved.");
  sprintf(buf, "%s just saved the help files.", GET_NAME(ch));
  logentry(buf, OVERSEER, DC::LogChannel::LOG_HELP);

  sprintf(file, "%s", WEB_HELP_FILE);
  LegacyFile lf_web_help(".", file, "Unable to open '%1'");
  if (lf_web_help.isOpen())
  {
    for (i = 0; i < DC::getInstance()->new_top_of_helpt; i++)
    {
      if (new_help_table[i].min_level <= DC::MAX_MORTAL_LEVEL)
      {
        help_string_to_file(lf_web_help.file_handle_, new_help_table[i].keyword1);
        help_string_to_file(lf_web_help.file_handle_, new_help_table[i].keyword2);
        help_string_to_file(lf_web_help.file_handle_, new_help_table[i].keyword3);
        help_string_to_file(lf_web_help.file_handle_, new_help_table[i].keyword4);
        help_string_to_file(lf_web_help.file_handle_, new_help_table[i].keyword5);
        //      help_string_to_file(lf.file_handle_ new_help_table[i].related);
        //      dc_fprintf(lf.file_handle_ "L: %d\n", new_help_table[i].min_level);
        //      dc_fprintf(lf.file_handle_ "E:\n");
        help_string_to_file(lf_web_help.file_handle_, new_help_table[i].entry);
        dc_fprintf(lf_web_help.file_handle_, "#\n");
        //      dc_fprintf(lf.file_handle_ "~\n");
      }
    }

    // end file
    dc_fprintf(lf.file_handle_, "$~\n");
  }
  else
  {
    ch->sendln("Couldn't open web help file for saving.");
    perror("Couldn't open web help file for saving.\r\n");
    return;
  }
}

void help_string_to_file(FILEPtr f, char *str)
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

  dc_fprintf(f, "%s\n", newbuf);
  delete[] newbuf;
}

int get_line_with_space(FILEPtr fl, char *buf)
{
  char temp[256];
  int lines = 0;

  do
  {
    lines++;
    dc_fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!dc_feof(fl) && *temp == '*');
  // } while (!dc_feof(fl) && (*temp == '*' || !*temp));

  if (dc_feof(fl))
    return 0;
  else
  {
    strcpy(buf, temp);
    return lines;
  }
}
