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
//struct hash_info nulltree = {"", 0, 0};

void kill_hsh_tree_func( struct hash_info * leaf )
{
  if(leaf->left)
    kill_hsh_tree_func(leaf->left);
  if(leaf->right)
    kill_hsh_tree_func(leaf->right);

  dc_free(leaf->name);
  dc_free(leaf);
}

// Since the top of the tree is static, we have to free both sides
void free_hsh_tree_from_memory( )
{
   if(tree.left)
     kill_hsh_tree_func( tree.left );
   if(tree.right)
     kill_hsh_tree_func( tree.right );
}

bool ishashed(char *arg)
{
  struct hash_info *current = &tree;
  for(; current; current = current->right) {
   if (current->name == arg)
     return TRUE;
  }
  return FALSE;
}

char *str_hsh(const char *arg)
{
  int scratch;
  struct hash_info *current = &tree;
  struct hash_info *next;
  struct hash_info *temp;

  // Second spot for "" args so we don't leak them all over the place
  //if(*arg == '\0')
  //  return(nulltree.name);
  if (!arg) return NULL;

  for(; current; current = next) {
     if((scratch = strcmp(arg, current->name)) == 0)
       return(current->name);

     if(scratch < 0)
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

  current->right = current->left = NULL;
  if(scratch < 0)
    temp->left  = current;
  else
    temp->right = current;
  current->name   = (char *)str_dup(arg);

  return(current->name);
}


/* logf(GET_LEVEL(ch), LOG_GOD, "%s restored all!", GET_NAME(ch)); */
void logf(int level, long type, const char *arg, ...)
{ 
  va_list args;
  char s[MAX_STRING_LENGTH];

  va_start(args, arg);
  vsnprintf(s, MAX_STRING_LENGTH, arg, args);
  va_end(args);

  log(s, level, type);
}

int csendf(struct char_data *ch, const char *arg, ...)
{ 
  va_list args;
  char s[MAX_STRING_LENGTH];

  va_start(args, arg);
  /* vsnprintf(s, MAX_STRING_LENGTH, arg, args); */
  vsprintf(s, arg, args); 
  va_end(args);

  send_to_char(s, ch);

  return(1);
}

string handle_ansi(string haystack, char_data *ch)
{
  map<size_t, bool> ignore;
  map<string, string> rep;
  rep["$$"] = "$";
  rep["$0"] = BLACK;
  rep["$1"] = BLUE;
  rep["$2"] = GREEN;
  rep["$3"] = CYAN;
  rep["$4"] = RED;
  rep["$5"] = YELLOW;
  rep["$6"] = PURPLE;
  rep["$7"] = GREY;
  rep["$B"] = BOLD;
  rep["$b"] = BOLD;
  rep["$R"] = NTEXT;
  rep["$r"] = NTEXT;
  rep["$L"] = FLASH;
  rep["$K"] = BLINK;
  rep["$I"] = INVERSE;

  try
  {
    for (auto &key : rep)
    {
      string needle = key.first;
      string replacement = key.second;

      size_t pos = 0, found_pos = 0;
      while ((found_pos = haystack.find(needle, pos)) != string::npos)
      {
        if (ch == nullptr || IS_MOB(ch) || IS_SET(ch->pcdata->toggles, PLR_ANSI) || (ch->desc && ch->desc->color))
        {
          haystack.replace(found_pos, 2, replacement);
          pos = found_pos + 1;
        }
        else
        {
          haystack.erase(found_pos, 2);
        }
      }
    }
  }
  catch (...)
  {
  }

  return haystack;
}
