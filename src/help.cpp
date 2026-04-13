
#include <cctype>
#include <cstring>

#include "DC/DC.h"

#include "DC/db.h" // exp_table
#include "DC/interp.h"
#include "DC/help.h"

#include <QMap>
#include <algorithm>

// Externs
extern help_index_element_new *new_help_table;
qint32 get_line(FILE *fl, QString buf);
bool is_abbrev(const QString arg1, const QString arg2);
void help_string_to_file(FILE *f, QString string);

// locals
help_index_element_new *find_help(QString keyword);
qint32 strn_cmp(QString arg1, QString arg2, qint32 n);
qint32 count_hash_records(FILE *fl);
void show_hedit_usage(CharacterPtr ch);
void save_help(CharacterPtr ch);
qint32 get_line_with_space(FILE *fl, QString buf);
qint32 show_one_help_entry(qint32 entry, CharacterPtr ch, qint32 count);
void show_help_header(CharacterPtr ch);
void show_help_bar(CharacterPtr ch);

// da functions
command_return_t do_mortal_help(CharacterPtr ch, QString argument, cmd_t cmd)
{
  extern QString new_help;
  ch->send(new_help);
  return ReturnValue::eSUCCESS;
}

class ltstr
{
public:
  bool operator()(qint32 a, qint32 b) const
  {
    return a < b;
  }
};

qint32 levenshtein(const QString s, const QString t)
{
  quint32 i, j, n, m, cost;
  quint32 d[MAX_HELP_KEYWORD_LENGTH + 1];

  m = strlen(s);
  n = strlen(t);

  // Zero Matrix
  for (i = {}; i <= m; i++)
    for (j = {}; j <= n; j++)
      d[i][j] = {};

  // Initialize Matrix
  for (i = {}; i <= m; i++)
    d[i][0] = i;
  for (j = {}; j <= n; j++)
    d[0][j] = j;

  for (i = 1; i <= m; i++)
    for (j = 1; j <= n; j++)
    {
      if (tolower(s[i]) == tolower(t[j]))
        cost = {};
      else
        cost = 1;
      d[i][j] = std::min(std::min(d[i - 1][j] + 1,
                                  d[i][j - 1] + 1),
                         d[i - 1][j - 1] + cost);
    }

  return d[m][n];
}

command_return_t do_new_help(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  extern QString new_help;
  extern QString new_ihelp;
  help_index_element_new *this_help;
  QString entry;
  QString key1, key2[256], key3[256], key4[256], key5[256], rec_level[256];

  if (!ch->desc)
    return ReturnValue::eFAILURE;

  argument = argument.trimmed();

  if (argument.isEmpty())
  {
    if (!ch->isImmortalPlayer())
      ch->send(new_help);
    else
      ch->send(new_ihelp);
    return ReturnValue::eFAILURE;
  }

  if (!new_help_table)
  {
    ch->sendln("No help available.");
    return ReturnValue::eFAILURE;
  }

  auto upper_argument = remove_trailing_spaces(argument);

  if (!(this_help = find_help(upper_argument)))
  {
    dc_snprintf(buf, 256, "There is no help entry for \'%s\'.\r\n",
                upper_argument);
    ch->send(buf);

    // Find similar help entries based on the Levenshtein distance
    // between keywords.

    qint32 h;
    quint32 l;
    quint32 argSize = argument.length();
    std::multimap<quint32, QString, ltstr> ltable;
    std::multimap<quint32, QString, ltstr>::iterator cur;

    qint32 level = ch->getLevel() == 0 ? 1 : ch->getLevel();

    for (h = {}; h < dc_->new_top_of_helpt; h++)
    {
      if (new_help_table[h].min_level_ > level)
      {
        continue;
      }

      if (!new_help_table[h].keyword1_.isEmpty())
      {
        l = levenshtein(argument, new_help_table[h].keyword1_);
        ltable.insert(std::pair<qint32, QString>(l, new_help_table[h].keyword1_));
      }

      if (!new_help_table[h].keyword2_.isEmpty())
      {
        l = levenshtein(argument, new_help_table[h].keyword2_);
        ltable.insert(std::pair<qint32, QString>(l, new_help_table[h].keyword2_));
      }

      if (!new_help_table[h].keyword3_.isEmpty())
      {
        l = levenshtein(argument, new_help_table[h].keyword3_);
        ltable.insert(std::pair<qint32, QString>(l, new_help_table[h].keyword3_));
      }

      if (!new_help_table[h].keyword4_.isEmpty())
      {
        l = levenshtein(argument, new_help_table[h].keyword4_);
        ltable.insert(std::pair<qint32, QString>(l, new_help_table[h].keyword4_));
      }

      if (!new_help_table[h].keyword5_.isEmpty())
      {
        l = levenshtein(argument, new_help_table[h].keyword5_);
        ltable.insert(std::pair<qint32, QString>(l, new_help_table[h].keyword5_));
      }
    }

    if (ltable.size() > 0)
    {
    }

    QList<QString> results;
    for (cur = ltable.begin(); cur != ltable.end(); cur++)
    {
      if (std::find(results.begin(), results.end(), (*cur).second) == results.end())
      {
        // Skip words which matches less than 50% of the original
        if ((*cur).first > argSize / 2)
          continue;

        if (results.size() == 0)
        {
          ch->send("Suggested help entries: ");
        }

        results.push_back((*cur).second);
        ch->send(u"%s"_s.arg((*cur).second));

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

    dc_sprintf(buf, "'%s' has no help entry.  %s just tried to call it.",
               upper_argument, qPrintable(ch->name()));
    dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_HELP);

    upper_argument = {};
    return ReturnValue::eFAILURE;
  }

  upper_argument = {};
  qint32 a = ch->getLevel() == 0 ? 1 : ch->getLevel();
  if (this_help->min_level > a)
  {
    ch->sendln("There is no help on that word.");
    return ReturnValue::eFAILURE;
  }

  dc_sprintf(key1, "'%s'", this_help->keyword1_);
  dc_sprintf(key2, "'%s'", this_help->keyword2_);
  dc_sprintf(key3, "'%s'", this_help->keyword3_);
  dc_sprintf(key4, "'%s'", this_help->keyword4_);
  dc_sprintf(key5, "'%s'", this_help->keyword5_);

  dc_sprintf(buf, "%s %s %s %s %s", key1, key2, key3, key4, key5);
  dc_sprintf(rec_level, "\r\nLevel Required: %d", this_help->min_level_);
  dc_sprintf(entry,
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
             buf, ch->isImmortalPlayer() ? rec_level : " ",
             ((this_help->min_level < IMMORTAL) ? " " : "\r\nImmortal-only command.\r\n"),
             this_help->entry, this_help->related);

  if (cmd != cmd_t::UNDEFINED)
    page_string(ch->desc, entry, 1);
  else
    ch->send(entry);

  return ReturnValue::eSUCCESS;
}

help_index_element_new &find_help(QString keyword)
{
  keyword = keyword.toUpper();
  if (keyword == u"NONE"_s)
    return {};

  for (auto &help_entry : new_help_table)
    if (keyword == entry.keyword1 ||
        keyword == entry.keyword2 ||
        keyword == entry.keyword3 ||
        keyword == entry.keyword4 ||
        keyword == entry.keyword5)
      return help_entry;

  return {};
}

constexpr auto ENTRY_MAX = 32384;

qint32 load_new_help(FILE *fl, qint32 reload, CharacterPtr ch)
{
  QString entry, line, tmpentry, buf, tmpbuffer;
  help_index_element_new new_help;
  qint32 version = 0, level = -1, linenum = {};

  linenum += get_line(fl, line);
  if (sscanf(line, "@Version: %d", &version) != 1)
  {
    if (reload == 1)
    {
      dc_->logentry(u"Error in verion number in help file.\r\n"_s, OVERSEER, DC::LogChannel::LOG_HELP);
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
    new_help.keyword1 = line;

    linenum += get_line(fl, line);
    new_help.keyword2 = line;

    linenum += get_line(fl, line);
    new_help.keyword3 = line;

    linenum += get_line(fl, line);
    new_help.keyword4 = line;

    linenum += get_line(fl, line);
    new_help.keyword5 = line;

    linenum += get_line(fl, line);
    new_help.related = line;

    linenum += get_line(fl, line);
    if (sscanf(line, "L: %d", &level) == 1)
    {
      new_help.min_level = level;
    }
    else
    {
      new_help.min_level = {};
    }

    linenum += get_line(fl, line);
    // E: Here.
    linenum += get_line_with_space(fl, line);
    *tmpentry = '\0';
    while (*line != '#')
    {
      if (*line == '\0')
      {
        dc_snprintf(tmpbuffer, ENTRY_MAX, "%s\r\n", tmpentry);
        dc_strncpy(tmpentry, tmpbuffer, ENTRY_MAX);
      }
      else
      {
        dc_snprintf(tmpbuffer, ENTRY_MAX, "%s%s\r\n", tmpentry, line);
        dc_strncpy(tmpentry, tmpbuffer, ENTRY_MAX);
      }
      linenum += get_line_with_space(fl, line);
    }

    if (strlen(tmpentry) > MAX_HELP_LENGTH)
      tmpentry[MAX_HELP_LENGTH - 1] = '\0';

    if (!(new_help.entry = tmpentry))
      new_help.entry = u"Error reading help entry. Please notify an Immortal!\n"_s;

    new_help_table[dc_->new_top_of_helpt] = new_help;
    dc_->new_top_of_helpt++;

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
      dc_sprintf(buf, "%s just reloaded the help files.", qPrintable(ch->name()));
    }
    dc_->logentry(buf, OVERSEER, DC::LogChannel::LOG_HELP);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_areas(CharacterPtr ch, QString arg, cmd_t cmd)
{
  dc_strcpy(arg, "areas");
  return do_new_help(ch, arg, cmd);
}

QString help_buf;

command_return_t do_hindex(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 i, minlen, count = {};
  QString arg;

  half_chop(argument, argument, arg);
  if (argument.isEmpty())
  {
    ch->sendln(u"Usage: hindex <ID#>\r\n       hindex <low ID#> <high ID#>  (you can display up to 30 at a time)\r\n       hindex <start of a word(s)>\r\n       hindex -<l|i|u|d>\r\n              l = level, l <start> <end>\r\n              i = immortal\r\n              d = defunct, < level 0 OR > level %1\r\n              u = unfinished, level 75\r\n"_s.arg(IMPLEMENTER));
    return ReturnValue::eFAILURE;
  }
  qint32 start = {};
  if (*argument == '-')
  { // we are doing a function, not a normal search
    if ((*(argument + 1) == 'l' || *(argument + 1) == 'L'))
    { // show help based on level range, excluded all level 1's
      // half_chop(arg, argument, arg);
      QString arg2, arg3;
      argument = &arg[0];
      argument = one_argument(argument, arg2);
      argument = one_argument(argument, arg3);
      one_argument(argument, arg);
      if (arg[0] && is_number(arg))
        start = atoi(arg);
      argument = &arg2[0];
      dc_strcpy(arg, arg3);
      if ((((atoi(argument)) > 0) || *argument == '0') && ((atoi(arg)) > 0))
      { // not valid numbers
        if (atoi(argument) > atoi(arg))
        {
          ch->sendln("Usage: hindex -l <low level> <high level>");
          return ReturnValue::eFAILURE;
        }
        if (arg.isEmpty())
          dc_sprintf(arg, "%s", argument); // if they left off the second arg, copy the first, show only one level
        show_help_header(ch);
        for (i = {}; i < dc_->new_top_of_helpt; i++)
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
      for (i = {}; i < dc_->new_top_of_helpt; i++)
      {
        if (new_help_table[i].min_level == 75)
          count = show_one_help_entry(i, ch, count);
      }
      show_help_bar(ch);
    }
    else if ((*(argument + 1) == 'd' || *(argument + 1) == 'D'))
    { // show defunct ones with out of range levels
      show_help_header(ch);
      for (i = {}; i < dc_->new_top_of_helpt; i++)
      {
        if (new_help_table[i].min_level <= 0 || new_help_table[i].min_level > IMPLEMENTER)
          count = show_one_help_entry(i, ch, count);
      }
      show_help_bar(ch);
    }
    else if ((*(argument + 1) == 'i' || *(argument + 1) == 'I'))
    { // show defunct ones with out of range levels
      show_help_header(ch);
      for (i = {}; i < dc_->new_top_of_helpt; i++)
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
    else if (atoi(argument) >= dc_->new_top_of_helpt || atoi(arg) >= dc_->new_top_of_helpt)
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
    if (atoi(argument) >= dc_->new_top_of_helpt)
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
    for (i = {}; i < dc_->new_top_of_helpt; i++)
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
  ch->send(u"$B$7Total Shown: $B$5%1$R\r\n"_s.arg(count));
  ch->send(u"$B$7Total Help Entries: $B$5%1$R\r\n"_s.arg(dc_->new_top_of_helpt));

  return ReturnValue::eSUCCESS;
}

command_return_t do_index(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 i, minlen, count = {};
  QString arg;

  half_chop(argument, argument, arg);
  if (argument.isEmpty())
  {
    ch->send(u"Usage: index <ID#>\r\n       index <low ID#> <high ID#>  (you can display up to 30 at a time)\r\n       index <start of a word(s)>\r\n\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  if ((((atoi(argument)) > 0) || *argument == '0') && ((atoi(arg)) > 0))
  { // index #s out of range
    if (atoi(argument) > atoi(arg))
    {
      ch->sendln("Usage: index <low ID#> <high ID#>"); // wrong order, first > second
      return ReturnValue::eFAILURE;
    }
    if (atoi(argument) >= dc_->new_top_of_helpt || atoi(arg) >= dc_->new_top_of_helpt)
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
    if (atoi(argument) >= dc_->new_top_of_helpt)
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
    for (i = {}; i < dc_->new_top_of_helpt; i++)
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
  ch->send(u"$B$7Total Shown: $B$5%1$R\r\n"_s.arg(count));
  ch->send(u"$B$7Total Help Entries: $B$5%1$R\r\n"_s.arg(dc_->new_top_of_helpt));

  return ReturnValue::eSUCCESS;
}

qint32 show_one_help_entry(qint32 entry, CharacterPtr ch, qint32 count)
{

  ch->sendln(u "$B$6%3d $7- $5%3d $7[$3%-20.20s$7] [$3%-20.20s$B$7] [$3%-20.20s$B$7] [$3%-20.20s$B$7] [$3%-20.20s$B$7]"_s
                 .arg(entry)
                 .arg((new_help_table[entry].min_level >= 0 ? new_help_table[entry].min_level : 999))
                 .arg((*new_help_table[entry].keyword1 ? new_help_table[entry].keyword1 : "None"))
                 .arg((*new_help_table[entry].keyword2 ? new_help_table[entry].keyword2 : "None"))
                 .arg((*new_help_table[entry].keyword2 ? new_help_table[entry].keyword3 : "None"))
                 .arg((*new_help_table[entry].keyword2 ? new_help_table[entry].keyword4 : "None"))
                 .arg((*new_help_table[entry].keyword3 ? new_help_table[entry].keyword5 : "None")));
  return ++count;
}

void show_help_header(CharacterPtr ch)
{
  send_to_char("$B$6ID# $B$7- $RLVL $3 Keyword 1              Keyword 2              Keyword 3"
               "              Keyword 4              Keyword 5\r\n",
               ch);
  show_help_bar(ch);
}

void show_help_bar(CharacterPtr ch)
{
  send_to_char("$B$7--------------------------------------------------------------------------"
               "--------------------------------------------------\r\n$R",
               ch);
}

/* strn_cmp: a case-insensitive version of strncmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
qint32 strn_cmp(QString arg1, QString arg2, qint32 n)
{
  qint32 chk, i;

  for (i = {}; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--)
  {
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i))))
    {
      if (chk < 0)
        return (-1);
      else
        return (1);
    }
  }
  return {};
}

command_return_t do_reload_help(CharacterPtr ch, QString argument, cmd_t cmd)
{

  FILE *new_help_fl;
  qint32 help_rec_count = 0, ret = {};

  // ch->sendln("Command disabled!");
  // return ReturnValue::eFAILURE;

  if (!(new_help_fl = fopen(NEW_HELP_FILE, "r")))
  {
    dc_->logentry(u"Error opening help file for reload."_s, OVERSEER, DC::LogChannel::LOG_HELP);
    return ReturnValue::eFAILURE;
  }

  help_rec_count = count_hash_records(new_help_fl);
  fclose(new_help_fl);

  if (!(new_help_fl = fopen(NEW_HELP_FILE, "r")))
  {
    dc_->logentry(u"Error opening help file for reload."_s, OVERSEER, DC::LogChannel::LOG_HELP);
    return ReturnValue::eFAILURE;
  }

  dc_->new_top_of_helpt = {};
  new_help_table = new help_index_element_new[help_rec_count];
  ret = load_new_help(new_help_fl, 1, ch);
  fclose(new_help_fl);

  if (ret == ReturnValue::eFAILURE)
  {
    ch->sendln("Error reloading help files!");
    return ReturnValue::eFAILURE;
  }

  ch->sendln("Help files reloaded.");
  return ReturnValue::eSUCCESS;
}

command_return_t do_hedit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf, buf2[200], field[200], buf3[200], value[200];
  help_index_element_new new_help;
  qint32 help_id = -1, i, key_id = -1, level = -1;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (!ch->has_skill(COMMAND_HEDIT))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  half_chop(argument, buf, buf2);

  if (buf.isEmpty())
  {
    show_hedit_usage(ch);
    return ReturnValue::eFAILURE;
  }

  if (!str_cmp(buf, "save"))
  { // save all help entries
    save_help(ch);
    return ReturnValue::eSUCCESS;
  }

  if (buf.isEmpty() 2)
  {
    show_hedit_usage(ch);
    return ReturnValue::eFAILURE;
  }

  if (!str_cmp(buf, "new"))
  { // New Help Entry
    for (i = {}; i < (qint32)strlen(buf2); i++)
      buf2[i] = UPPER(buf2[i]);
    new_help.keyword1 = buf2;
    new_help.keyword2 = u"NONE"_s;
    new_help.keyword3 = u"NONE"_s;
    new_help.keyword4 = u"NONE"_s;
    new_help.keyword5 = u"NONE"_s;
    new_help.related = u"NONE"_s;
    new_help.min_level = 75;
    new_help.entry = u"Blank help file!\r\n"_s;

    auto old_table = new_help_table;
    new_help_table = new help_index_element_new[dc_->new_top_of_helpt + 1];
    old_table = {};
    new_help_table[dc_->new_top_of_helpt] = new_help;
    dc_sprintf(buf, "Help entry #%d added with keyword '%s'.\r\n", dc_->new_top_of_helpt, buf2);
    ch->send(buf);
    dc_->new_top_of_helpt++;
    dc_sprintf(buf, "%s just created a help file for '%s'.", qPrintable(ch->name()), buf2);
    dc_->logentry(buf, OVERSEER, DC::LogChannel::LOG_HELP);
  }
  else if ((help_id = atoi(buf)) || *buf == '0')
  { // Edit a specific help entry
    if (*buf == 0)
      help_id = {};
    if (help_id < 0 || help_id >= dc_->new_top_of_helpt)
    {
      ch->sendln("Not a valid help ID number.  Try using 'hindex'");
      return ReturnValue::eFAILURE;
    }
    half_chop(buf2, field, buf2);
    if (is_abbrev(field, "key"))
    { // changing one of the keys
      half_chop(buf2, buf3, value);
      if (buf.isEmpty() 3 || value.isEmpty())
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
        for (i = {}; i < (qint32)strlen(value); i++)
          value[i] = UPPER(value[i]);
        switch (key_id)
        {
        case 1:
          new_help_table[help_id].keyword1 = value;
          break;
        case 2:
          new_help_table[help_id].keyword2 = value;
          break;
        case 3:
          new_help_table[help_id].keyword3 = value;
          break;
        case 4:
          new_help_table[help_id].keyword4 = value;
          break;
        case 5:
          new_help_table[help_id].keyword5 = value;
          break;
        default:
          ch->sendln("Not a valid key #.");
          return ReturnValue::eFAILURE;
          break;
        }
        dc_sprintf(buf, "Keyword %d changed to '%s' for ID# %d.\r\n", key_id, value, help_id);
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
          level = {};
        new_help_table[help_id].min_level = level;
        dc_sprintf(buf, "Level changed to '%d' for ID# %d.\r\n", level, help_id);
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
        for (i = {}; i < (qint32)strlen(buf2); i++)
          buf2[i] = UPPER(buf2[i]);
        new_help_table[help_id].related = buf2;
        dc_sprintf(buf, "Related changed to '%s' for ID# %d.\r\n", buf2, help_id);
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
      ch->desc->backstr = {};
      send_to_char("        Write your help entry and stay within the line.  (/s saves /h for help)\r\n"
                   "   |--------------------------------------------------------------------------------|\r\n",
                   ch);
      if (new_help_table[help_id].entry)
      {
        ch->desc->backstr = (new_help_table[help_id].entry);
        ch->send(ch->desc->backstr);
      }

      ch->desc->connected = Connection::states::EDITING;
      ch->desc->strnew = &(new_help_table[help_id].entry);
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

void show_hedit_usage(CharacterPtr ch)
{

  send_to_char("$3Syntax$R: hedit <id#> <field> [arg] [value]\r\n"
               "        hedit new <first keyword>\r\n"
               "        hedit save\r\n\r\n"
               "  Fields: key, level, entry, related\r\n"
               "  Args: Only apply to key in which it is 1-5\r\n"
               "  Values: The value of the specified field.  Does not affect entry\r\n",
               ch);
}

void save_help(CharacterPtr ch)
{
  qint32 i;
  QString file, buf[256];

  dc_sprintf(file, "%s", NEW_HELP_FILE);
  LegacyFile lf(".", file, "Couldn't open help file '%1' for saving.");
  if (lf.isOpen())
  {
    dc_fprintf(lf.file_handle_, "@Version: 2\n");

    for (i = {}; i < dc_->new_top_of_helpt; i++)
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
  dc_sprintf(buf, "%s just saved the help files.", qPrintable(ch->name()));
  dc_->logentry(buf, OVERSEER, DC::LogChannel::LOG_HELP);

  dc_sprintf(file, "%s", WEB_HELP_FILE);
  LegacyFile lf_web_help(".", file, "Unable to open '%1'");
  if (lf_web_help.isOpen())
  {
    for (i = {}; i < dc_->new_top_of_helpt; i++)
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

void help_string_to_file(FILE *f, QString str)
{
  QString newbuf;
  dc_strcpy(newbuf, str);

  // remove all \r's
  for (QString curr = newbuf; *curr != '\0'; curr++)
  {
    if (*curr == '\r')
    {
      for (QString blah = curr; *blah != '\0'; blah++) // shift the rest of the QString 1 left
        *blah = *(blah + 1);
      curr--; // (to check for \r\r cases)
    }
  }

  if (newbuf[strlen(newbuf) - 1] == '\n')
    newbuf[strlen(newbuf) - 1] = '\0';

  dc_fprintf(f, "%s\n", newbuf);
}

qint32 get_line_with_space(FILE *fl, QString buf)
{
  QString temp;
  qint32 lines = {};

  do
  {
    lines++;
    fgets(temp, 256, fl);
    if (!temp.isEmpty())
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && *temp == '*');
  // } while (!feof(fl) && (*temp == '*' || temp.isEmpty()));

  if (feof(fl))
    return 0;
  else
  {
    dc_strcpy(buf, temp);
    return lines;
  }
}
