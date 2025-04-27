/* $Id: nlog.cpp,v 1.12 2014/07/04 22:00:04 jhhudso Exp $ */

#include <cstdio>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <stdarg.h>

#include "DC/connect.h"
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/terminal.h"
#include "DC/player.h" // Player::PLR_ANSI
#include "DC/utility.h"
#include <iostream>

/*
 * logf, str_hsh, and csendf by Sadus, others by Ysafar.
 */

struct hash_info
{
  char *name;
  struct hash_info *left;
  struct hash_info *right;
};

struct hash_info tree = {"m", 0, 0};
// struct hash_info nulltree = {"", 0, 0};

void kill_hsh_tree_func(struct hash_info *leaf)
{
  if (leaf->left)
    kill_hsh_tree_func(leaf->left);
  if (leaf->right)
    kill_hsh_tree_func(leaf->right);

  dc_free(leaf->name);
  dc_free(leaf);
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

bool ishashed(char *arg)
{
  struct hash_info *current = &tree;
  for (; current; current = current->right)
  {
    if (current->name == arg)
      return true;
  }
  return false;
}

char *str_hsh(const char *arg)
{
  int scratch;
  struct hash_info *current = &tree;
  struct hash_info *next;
  struct hash_info *temp;

  // Second spot for "" args so we don't leak them all over the place
  // if(*arg == '\0')
  //  return(nulltree.name);
  if (!arg)
    return nullptr;

  for (; current; current = next)
  {
    if ((scratch = strcmp(arg, current->name)) == 0)
      return (current->name);

    if (scratch < 0)
      next = current->left;
    else
      next = current->right;
    temp = current;
  }

#ifdef LEAK_CHECK
  current = (struct hash_info *)calloc(1, sizeof(struct hash_info));
#else
  current = (struct hash_info *)dc_alloc(1, sizeof(struct hash_info));
#endif

  current->right = current->left = nullptr;
  if (scratch < 0)
    temp->left = current;
  else
    temp->right = current;
  current->name = (char *)str_dup(arg);

  return (current->name);
}

void logf(int level, LibDC::LogChannels type, QString arg)
{
  logentry(arg, level, type);
}

/* logf(ch->getLevel(), LibDC::LogChannels::LOG_GOD, "%s restored all!", GET_NAME(ch)); */
void logf(int level, LibDC::LogChannels type, const char *arg, ...)
{
  va_list args;
  char s[MAX_STRING_LENGTH];

  va_start(args, arg);
  vsnprintf(s, MAX_STRING_LENGTH, arg, args);
  va_end(args);

  logentry(s, level, type);
}

int csendf(Character *ch, const char *arg, ...)
{
  va_list args;
  char s[MAX_STRING_LENGTH];

  va_start(args, arg);
  /* vsnprintf(s, MAX_STRING_LENGTH, arg, args); */
  vsprintf(s, arg, args);
  va_end(args);

  ch->send(s);

  return (1);
}

char *handle_ansi_(char *s, Character *ch)
{
  char *t;
  char *tp, *sp, *i;

  char nullstring[] = "";
  char dollarstring[] = "$";

  // Worse case scenario is a std::string of color codes that are all $R's.  These take up
  // 11 characters each.  So to handle that, we'll count the number of $'s and multiply
  // that by 11 for the amount of extra space we need.

  int numdollars = 0;

  t = s;
  while ((t = strstr(t, "$")))
  {
    numdollars++;
    t++;
  }

#ifdef LEAK_CHECK
  t = (char *)calloc((strlen(s) + numdollars * 11 + 1), sizeof(char));
#else
  t = (char *)dc_alloc((strlen(s) + numdollars * 11 + 1), sizeof(char));
#endif
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
      if (IS_MOB(ch) || isSet(ch->player->toggles, Player::PLR_ANSI) || (ch->desc && ch->desc->color))
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
          sp--;    // back up to the $ char so we don't go past our \0
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

QString handle_ansi(QString haystack, Character *ch)
{
  return handle_ansi(QByteArray(haystack.toStdString().c_str()), ch);
}

QByteArray handle_ansi(QByteArray haystack, Character *ch)
{
  QMap<size_t, bool> ignore;
  QMap<char, QPair<QByteArray, QByteArray>> rep;
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
  for (const char &c : haystack)
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
