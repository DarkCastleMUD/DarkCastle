/*

  Specilization stuff

*/
extern "C"
{
#include <ctype.h>
#include <string.h>
}

#include "structs.h"
#include "room.h"
#include "character.h"
#include "obj.h"
#include "utility.h"
#include "terminal.h"
#include "player.h"
#include "levels.h"
#include "mobile.h"
#include "clan.h"
#include "handler.h"
#include "db.h"
#include "interp.h"
#include "connect.h"
#include "spells.h"
#include "race.h"
#include "act.h"
#include "set.h"
#include "returnvals.h"
#include "timeinfo.h"

struct spec_data
{
  char *name;
  char *description;
  int pcclass;
  int skills[10];
};

const struct spec_data spec_list[] =
    {
        {"Blah", "Blehe", 1, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {NULL, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}};

int do_spec(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg);
  if (IS_NPC(ch))
    return eFAILURE;
  if (!str_cmp(arg, "list"))
  {
    /*
       for (int i = 0; spec_list[i].name != NULL; i++)
        {
           if (spec_list[i].pcclass != GET_CLASS(ch)) continue;
           sprintf(buf, "%d. %s : %s\r\n",i+1, spec_list[i].name,
        spec_list[i].description);
        }
    */

    send_to_char(buf, ch);
  }
  else if (!str_cmp(arg, "unlearn"))
  {
    argument = one_argument(argument, arg);
    if (str_cmp(arg, "iamsure"))
    {
      // messagemoose, couple below too
      send_to_char("You will have to type \"profession unlearn iamsure\"\r\nThis costs 10,000 platinum coins and is not reversable.\r\n", ch);
      return eFAILURE;
    }
    if (ch->spec == 0)
    {
      send_to_char("You do not have a specialization.\r\n", ch);
      return eFAILURE;
    }
    if (GET_PLATINUM(ch) < 10000)
    {
      send_to_char("You do not have the required 10000 platinum.\r\n", ch);
      return eFAILURE;
    }
    for (int i = 0; i < 10; i++)
    { // Free up skills tied to the specilization.
      queue<skill_t> skills_to_delete = {};
      for (auto &curr : ch->skills)
      {
        if (curr.first == spec_list[ch->spec].skills[i])
        {
          skills_to_delete.push(curr.first);
        }
      }
      while(skills_to_delete.empty() == false)
      {
        ch->skills.erase(skills_to_delete.front());
        skills_to_delete.pop();
      }
    }
    ch->spec = 0;
    GET_PLATINUM(ch) -= 10000;
    ch->saves[SAVE_TYPE_FIRE] -= (GET_LEVEL(ch) == 60 ? 1 : 0 + GET_LEVEL(ch) > 56 ? 1
                                                        : 0 + GET_LEVEL(ch) > 53   ? 1
                                                        : 0 + GET_LEVEL(ch) > 50   ? 1
                                                                                   : 0);
    ch->saves[SAVE_TYPE_COLD] -= (GET_LEVEL(ch) == 60 ? 1 : 0 + GET_LEVEL(ch) > 56 ? 1
                                                        : 0 + GET_LEVEL(ch) > 53   ? 1
                                                        : 0 + GET_LEVEL(ch) > 50   ? 1
                                                                                   : 0);
    ch->saves[SAVE_TYPE_ENERGY] -= (GET_LEVEL(ch) == 60 ? 1 : 0 + GET_LEVEL(ch) > 56 ? 1
                                                          : 0 + GET_LEVEL(ch) > 53   ? 1
                                                          : 0 + GET_LEVEL(ch) > 50   ? 1
                                                                                     : 0);
    ch->saves[SAVE_TYPE_ACID] -= (GET_LEVEL(ch) == 60 ? 1 : 0 + GET_LEVEL(ch) > 56 ? 1
                                                        : 0 + GET_LEVEL(ch) > 53   ? 1
                                                        : 0 + GET_LEVEL(ch) > 50   ? 1
                                                                                   : 0);
    ch->saves[SAVE_TYPE_MAGIC] -= (GET_LEVEL(ch) == 60 ? 1 : 0 + GET_LEVEL(ch) > 56 ? 1
                                                         : 0 + GET_LEVEL(ch) > 53   ? 1
                                                         : 0 + GET_LEVEL(ch) > 50   ? 1
                                                                                    : 0);
    ch->saves[SAVE_TYPE_POISON] -= (GET_LEVEL(ch) == 60 ? 1 : 0 + GET_LEVEL(ch) > 56 ? 1
                                                          : 0 + GET_LEVEL(ch) > 53   ? 1
                                                          : 0 + GET_LEVEL(ch) > 50   ? 1
                                                                                     : 0);
    GET_LEVEL(ch) = 51;

    // messagemoose
    send_to_char("You forget your specilization.\r\n", ch);
  }
  else if (!str_cmp(arg, "learn"))
  {
  }
  return eSUCCESS;
}
