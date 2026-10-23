#include <fmt/format.h>
#include <qstring.h>
#include <stdio.h>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <string>

#include "DC/connect.h" // Connection
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/interp.h"
#include "DC/DC.h"
#include "DC/memory.h"
#include "DC/comm.h"
#include "DC/common.h"
#include "DC/levels.h"
#include "DC/structs.h"

// send_to_char("Write your note.  (/s saves /h for help)
void new_edit_board_unlock_board(Character *ch, int abort);
QString format_text(QString str, int mode);
void check_for_awaymsgs(Character *);

std::pair<QString, unsigned long long> replace_str(QString str, QString pattern, QString replacement, bool rep_all)
{
  if (pattern == replacement)
    return {str, 0ULL};

  if (rep_all)
  {
    auto replacement_count = str.count(pattern);
    return {str.replace(pattern, replacement), replacement_count};
  }

  str.replace(str.indexOf(pattern), pattern.size(), replacement);
  return {str, 1};
}

/*  handle some editor commands */
void Connection::parse_action(parse_t action, QString str)
{
  int indent{}, flags{}, line_low{}, line_high{};
  QString buf, t;
  auto arguments = str.split(' ');
  auto arg1 = arguments.value(0);
  auto arg1_num = arg1.toLongLong();
  auto arg2 = arguments.value(1);
  auto arguments_copy = arguments;
  if (!arguments_copy.isEmpty())
    arguments_copy.removeFirst();
  auto arg_remaining = arguments_copy.join(' ');

  auto s = *strnew;
  if (s.endsWith("\n"))
  {
    s.removeLast();
    s.removeLast();
  }
  auto lines = s.split("\n");
  while (!lines.isEmpty() && lines.first().isEmpty())
    lines.removeFirst();

  // qDebug("[%s] %lld lines detected (\\n=%lld \=%lld)", qPrintable(lines.value(0)), lines.size(), s.count('\n'), s.count(''));

  switch (action)
  {
  case parse_t::DELETE:
  case parse_t::LIST_NORM:
  case parse_t::LIST_NUM:
  case parse_t::EDIT:
  case parse_t::INSERT:
    // qDebug("[%s][%s]", qPrintable(arg1), qPrintable(arg_remaining));
    if (arg1.isEmpty())
      break;
    if (!arg1[0].isDigit())
      break;

    if (arg1.contains("-"))
    {
      bool ok = false;
      line_low = arg1.split('-').value(0).toULongLong(&ok);
      if (!ok)
      {
        write_to_output(u"'%1' is an invalid number.\n"_s.arg(arg1));
        return;
      }

      line_high = arg1.split('-').value(1).toULongLong(&ok);
      if (!ok)
      {
        write_to_output(u"'%1' is an invalid number.\n"_s.arg(arg1));
        return;
      }

      if (line_high < line_low)
      {
        write_to_output(u"'%1' is an invalid range.\n"_s.arg(arg1));
        return;
      }
    }
    else
    {
      bool ok = false;
      line_low = line_high = arg1.toULongLong(&ok);
      if (!ok)
      {
        write_to_output(u"'%1' is an invalid number.\n"_s.arg(arg1));
        return;
      }
    }
    break;
  default:
    break;
  }
  // qDebug("linelow=%d linehigh=%d", line_low, line_high);
  auto replacement_count = 0ULL;
  switch (action)
  {
  case parse_t::HELP:
    buf = u"Editor command formats: /<letter>\n\n"
          "/a         -  aborts editor\n"
          "/c         -  clears buffer\n"
          "/d#        -  deletes a line #\n"
          "/e# <text> -  changes the line at # with <text>\n"
          "/f         -  formats text\n"
          "/fi        -  indented formatting of text\n"
          "/h         -  list text editor commands\n"
          "/i# <text> -  inserts <text> before line #\n"
          "/l         -  lists buffer\n"
          "/n         -  lists buffer with line numbers\n"
          "/r 'a' 'b' -  replace 1st occurance of text <a> in buffer with text <b>\n"
          "/ra 'a' 'b'-  replace all occurances of text <a> within buffer with text <b>\n"
          "              usage: /r[a] 'pattern' 'replacement'\n"
          "/s         -  saves text\n"_s;
    write_to_output(buf);
    break;
  case parse_t::FORMAT:
    if (arg1.startsWith('i'))
    {
      indent = true;
      flags += FORMAT_INDENT;
    }

    *strnew = format_text(*strnew, flags);
    write_to_output(u"Text formatted with%1 indent.\n"_s.arg(indent ? "" : "out"));
    break;
  case parse_t::REPLACE:
    if (!arg_remaining.contains('\''))
    {
      write_to_output("Invalid format. Must use quotes.\n");
      return;
    }

    s = arg_remaining.split('\'').value(1);
    if (s.isEmpty())
    {
      write_to_output("Target string must be enclosed in single quotes like: /r 'silver' 'gold'\n");
      return;
    }
    t = arg_remaining.split('\'').value(3);
    if (t.isEmpty())
    {
      write_to_output("No replacement string. Example: /r 'silver' 'gold'\n");
      return;
    }

    std::tie(*strnew, replacement_count) = replace_str(*strnew, s, t, arg1 == u"a"_s);
    write_to_output(u"Replaced %1 occurance of '%2' with '%3'.\n"_s.arg(replacement_count).arg(s).arg(t));
    break;
  case parse_t::DELETE:
  {
    if (line_low > 0 && line_high > 0 && line_high >= line_low)
    {
      if (lines.size() < line_low || lines.size() < line_high)
      {
        write_to_output("Line(s) out of range; not deleting.\n");
        return;
      }

      auto to_delete_count = std::max(1, line_high - line_low);

      auto old_size = lines.size();
      lines.remove(line_low - 1, to_delete_count);
      *strnew = lines.join("\n") + u"\n"_s;
      auto new_size = lines.size();
      auto total_len = 0ULL;
      if (old_size >= new_size)
        total_len = old_size - new_size;

      buf = u"%1 line%2deleted.\n"_s.arg(total_len).arg(((total_len != 1) ? "s " : " "));
      write_to_output(buf);
    }
    else
    {
      write_to_output("Invalid line numbers to delete must be higher than 0.\n");
      return;
    }
    break;
  }
  case parse_t::LIST_NORM:
  case parse_t::LIST_NUM:
  {
    auto lines = s.split("\n");
    while (!lines.isEmpty() && lines.first().isEmpty())
      lines.removeFirst();

    qsizetype i = 0;
    for (; i < lines.size(); ++i)
    {
      if (action == parse_t::LIST_NUM)
        write_to_output(u"%1: %2\n"_s.arg(i + 1, 4).arg(lines[i]));
      else
        write_to_output(u"%1\n"_s.arg(lines[i]));
    }

    // page_string(d, buf, true);

    break;
  }

  case parse_t::INSERT:
    if (!line_low)
    {
      write_to_output("Line number must be higher than 0.\n");
      return;
    }

    // qDebug("[%s][%s]", qPrintable(arg1), qPrintable(arg_remaining));
    if (line_low > lines.size())
    {
      write_to_output("Line number out of range; change aborted.\n");
      return;
    }

    lines.insert(line_low - 1, arg_remaining);
    *strnew = lines.join("\n") + u"\n"_s;
    write_to_output("Line inserted.\n");
    break;

  case parse_t::EDIT:
    if (s.isEmpty())
    {
      write_to_output("Buffer is empty, nothing to change.\n");
      return;
    }
    if (!line_low)
    {
      write_to_output("Line number must be higher than 0.\n");
      return;
    }

    // qDebug("[%s][%s]", qPrintable(arg1), qPrintable(arg_remaining));
    if (line_low > lines.size())
    {
      write_to_output("Line number out of range; change aborted.\n");
      return;
    }

    lines[line_low - 1] = arg_remaining;
    *strnew = lines.join("\n") + u"\n"_s;
    write_to_output("Line changed.\n");
    return;

    break;
  default:
    write_to_output("Invalid option.\n");
    logentry(QStringLiteral("SYSERR: invalid command passed to parse_action"), OVERSEER, DC::LogChannel::LOG_MISC);
    return;
  }
}

QString format_text(QString str, int mode)
{
  if (isSet(mode, FORMAT_INDENT))
    str = u"   "_s + str.trimmed();

  return str.replace("\n", " ");
}

void Connection::editor_parse_line(QString str)
{
  assert(strnew);
  if (str.isEmpty())
    return;
  if (str.contains("~"))
  {
    write_to_output("Cannot add tildes.\n");
    return;
  }
  str.remove('\n');
  if (str.trimmed()[0] != '/')
  {
    *strnew += str + u"\n"_s;
    return;
  }

  auto arguments = str;
  arguments.remove(0, 2);
  bool abort_edit = false;
  bool save_edit = false;

  switch (str[1].toLatin1())
  {
  case 'a':
    abort_edit = true;
    break;
  case 'c':
    *strnew = {};
    write_to_output("Current buffer cleared.\n");
    break;
  case 'd':
    parse_action(parse_t::DELETE, arguments);
    break;
  case 'e':
    parse_action(parse_t::EDIT, arguments);
    break;
  case 'f':
    parse_action(parse_t::FORMAT, arguments);
    break;
  case 'i':
    parse_action(parse_t::INSERT, arguments);
    break;
  case 'h':
    parse_action(parse_t::HELP, arguments);
    break;
  case 'l':
    parse_action(parse_t::LIST_NORM, arguments);
    break;
  case 'n':
    parse_action(parse_t::LIST_NUM, arguments);
    break;
  case 'r':
    parse_action(parse_t::REPLACE, arguments);
    break;
  case 's':
    save_edit = true;
    str = {};
    break;
  default:
    write_to_output("Invalid option.\n");
    break;
  }

  if (abort_edit || save_edit)
  {
    if (abort_edit)
    {
      write_to_output("Aborted.\n");
      // write_to_output(u"Replaced %1 characters with the original %2 characters!\n"_s.arg(strnew->size()).arg(backstr.size()));
      *strnew = backstr;
    }
    else
      write_to_output("Ok.\n");

    switch (connected)
    {
    case Connection::states::WRITE_BOARD:
      if (character)
        new_edit_board_unlock_board(character, 1);
      break;
    default:
      break;
    }

    if (connected == Connection::states::EXDSCR)
    {
      connected = Connection::states::SELECT_MENU;
      write_to_output(menu);
    }
    else
      connected = Connection::states::PLAYING;

    if (character)
      check_for_awaymsgs(character);

    strnew = {};
  }
}
