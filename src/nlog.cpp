/* $Id: nlog.cpp,v 1.12 2014/07/04 22:00:04 jhhudso Exp $ */

#include <cstdio>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <stdarg.h>

#include "DC/DC.h"

#include "DC/terminal.h"

/*
 * DC::getInstance()->logf, QStringLiteral, and csendf by Sadus, others by Ysafar.
 */

class hash_info
{
public:
  QString name;
  hash_info *left;
  hash_info *right;
};

hash_info tree = {"m", 0, 0};
//  hash_info nulltree = {"", 0, 0};

void kill_hsh_tree_func(hash_info *leaf)
{
  if (leaf->left)
    kill_hsh_tree_func(leaf->left);
  if (leaf->right)
    kill_hsh_tree_func(leaf->right);

  leaf->name = {};
  leaf = {};
}

// Since the top of the tree is static, we have to free both sides
void DC::free_hsh_tree_from_memory()
{
  if (tree.left)
    kill_hsh_tree_func(tree.left);
  if (tree.right)
    kill_hsh_tree_func(tree.right);
  tree = {"m", 0, 0};
}

bool ishashed(QString arg)
{
  hash_info *current = &tree;
  for (; current; current = current->right)
  {
    if (current->name == arg)
      return true;
  }
  return false;
}

/* DC::getInstance()->logf(ch->getLevel(), DC::LogChannel::LOG_GOD, "%s restored all!", qPrintable(ch->name())); */
void DC::logf(level_t level, DC::LogChannel type, QString cformat, ...)
{
  va_list args;
  QString s;

  va_start(args, cformat);
  s = QString::vasprintf(qPrintable(cformat), args);
  va_end(args);

  DC::getInstance()->logentry(s, level, type);
}

QString handle_ansi_(QString s, CharacterPtr ch)
{
  QString t;
  QString tp, *sp;
  const QString i;

  QString nullstring = "";
  QString dollarstring = "$";

  // Worse case scenario is a QString of color codes that are all $R's.  These take up
  // 11 characters each.  So to handle that, we'll count the number of $'s and multiply
  // that by 11 for the amount of extra space we need.

  qint32 numdollars = {};

  t = s;
  while ((t = strstr(t, "$")))
  {
    numdollars++;
    t++;
  }

  t = {};
  *t = '\0';

  i = nullstring;
  tp = t;
  sp = s;
  while (*sp)
  {
    if (*sp != '$')
    {
      *tp++ = *sp++;
    }
    else
    {
      if (ch->isNonPlayer() || isSet(ch->player->toggles, Player::PLR_ANSI) || (ch->desc && ch->desc->color))
      {
        switch (*++sp)
        {
          //             case 'B':  i = BLACK; break;
          //             case 'R':  i = RED; break;
          //             case 'g':  i = GREEN; break;
          //             case 'Y':  i = YELLOW; break;
          //             case 'b':  i = BLUE; break;
          //             case 'P':  i = PURPLE; break;
          //             case 'C':  i = CYAN; break;
          //             case 'G':  i = GREY; break;
          //             case '!':  i = BOLD; break;
          //             case 'N':  i = NTEXT; break;

        case '0':
          i = BLACK;
          break;
        case '1':
          i = BLUE;
          break;
        case '2':
          i = GREEN;
          break;
        case '3':
          i = CYAN;
          break;
        case '4':
          i = RED;
          break;
        case '5':
          i = YELLOW;
          break;
        case '6':
          i = PURPLE;
          break;
        case '7':
          i = GREY;
          break;
        case 'B':
          i = BOLD;
          break;
        case 'R':
          i = NTEXT;
          break;
        case 'L':
          i = FLASH;
          break;
        case 'K':
          i = BLINK;
          break;
        case 'I':
          i = INVERSE;
          break;
        case '$':
          i = dollarstring;
          break;
        case '\0': // this happens if we end a line with $
          sp--;    // back up to the $ character so we don't go past our \0
                   // no break here so the default catchs it and uses a nullstring
        default:
          i = nullstring;
          break;
        }
      }
      else
      {
        sp++;
        if (*sp == '$')
          i = dollarstring;
        else
          i = nullstring;
      }
      while ((*tp++ = *i++))
        ;
      tp--;
      sp++;
    }
  }
  *tp = '\0';

  return t;
}

QString handle_ansi(QString haystack, CharacterPtr ch)
{
  return handle_ansi(haystack.toLatin1(), ch);
}

QByteArray handle_ansi(QByteArray haystack, CharacterPtr ch)
{
  QMap<size_t, bool> ignore;
  QMap<QChar, QPair<QByteArray, QByteArray>> rep;
  rep['$'] = {"$", "$"};
  rep['0'] = {BLACK, ""};
  rep['1'] = {BLUE, ""};
  rep['2'] = {GREEN, ""};
  rep['3'] = {CYAN, ""};
  rep['4'] = {RED, ""};
  rep['5'] = {YELLOW, ""};
  rep['6'] = {PURPLE, ""};
  rep['7'] = {GREY, ""};
  rep['B'] = {BOLD, ""};
  rep['R'] = {NTEXT, ""};
  rep['L'] = {FLASH, ""};
  rep['K'] = {BLINK, ""};
  rep['I'] = {INVERSE, ""};

  QByteArray result;
  bool code = false;
  for (const auto &c : haystack)
  {
    if (code == true)
    {
      if (rep.contains(c))
      {
        if (ch && ch->allowColor())
        {
          result += rep.value(c).first;
        }
        else
        {
          result += rep.value(c).second;
        }
      }
      code = false;
      continue;
    }

    if (c == '$' && code == false)
    {
      code = true;
    }
    else
    {
      result += c;
    }
  }

  return result;
}
