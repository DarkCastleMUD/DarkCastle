/* $Id: nlog.cpp,v 1.12 2014/07/04 22:00:04 jhhudso Exp $ */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "connect.h"
#include "character.h"
#include "utility.h"
#include "terminal.h"
#include "player.h" // PLR_ANSI
#include "utility.h"
#include <iostream>
using namespace std;
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
void free_hsh_tree_from_memory()
{
  if (tree.left)
    kill_hsh_tree_func(tree.left);
  if (tree.right)
    kill_hsh_tree_func(tree.right);
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

/* logf(GET_LEVEL(ch), LogChannels::LOG_GOD, "%s restored all!", GET_NAME(ch)); */
void logf(int level, LogChannels type, const char *arg, ...)
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

  send_to_char(s, ch);

  return (1);
}

char *handle_ansi_(char *s, Character *ch)
{
  char *t;
  char *tp, *sp, *i;

  char nullstring[] = "";
  char dollarstring[] = "$";

  // Worse case scenario is a string of color codes that are all $R's.  These take up
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
      if (IS_MOB(ch) || IS_SET(ch->player->toggles, PLR_ANSI) || (ch->desc && ch->desc->color))
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

string handle_ansi(string haystack, Character *ch)
{
  map<size_t, bool> ignore;
  map<char, string> rep;
  rep['$'] = "$";
  rep['0'] = BLACK;
  rep['1'] = BLUE;
  rep['2'] = GREEN;
  rep['3'] = CYAN;
  rep['4'] = RED;
  rep['5'] = YELLOW;
  rep['6'] = PURPLE;
  rep['7'] = GREY;
  rep['B'] = BOLD;
  rep['R'] = NTEXT;
  rep['L'] = FLASH;
  rep['K'] = BLINK;
  rep['I'] = INVERSE;

  string result;
  try
  {
    bool code = false;
    for (auto &c : haystack)
    {
      if (code == true)
      {
        if (ch == nullptr || IS_MOB(ch) || (ch->player != nullptr && IS_SET(ch->player->toggles, PLR_ANSI)) || (ch->desc && ch->desc->color))
        {
          if (rep.find(c) != rep.end())
          {
            result += rep[c];
          }
          code = false;
          continue;
        }
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
  }
  catch (...)
  {
  }

  return result;
}
