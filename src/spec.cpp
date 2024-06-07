/*

  Specilization stuff

*/

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
#include "DC/db.h"
#include "DC/interp.h"
#include "DC/connect.h"
#include "DC/spells.h"
#include "DC/race.h"
#include "DC/act.h"
#include "DC/set.h"
#include "DC/returnvals.h"
#include "DC/timeinfo.h"

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
        {nullptr, nullptr, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}};

int do_spec(Character *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg);
  if (IS_NPC(ch))
    return eFAILURE;
  if (!str_cmp(arg, "list"))
  {
    /*
       for (int i = 0; spec_list[i].name != nullptr; i++)
        {
           if (spec_list[i].pcclass != GET_CLASS(ch)) continue;
           sprintf(buf, "%d. %s : %s\r\n",i+1, spec_list[i].name,
        spec_list[i].description);
        }
    */

    ch->send(buf);
  }
  else if (!str_cmp(arg, "unlearn"))
  {
    argument = one_argument(argument, arg);
    if (str_cmp(arg, "iamsure"))
    {
      // messagemoose, couple below too
      ch->sendln("You will have to type \"profession unlearn iamsure\"\r\nThis costs 10,000 platinum coins and is not reversable.");
      return eFAILURE;
    }
    if (ch->spec == 0)
    {
      ch->sendln("You do not have a specialization.");
      return eFAILURE;
    }
    if (GET_PLATINUM(ch) < 10000)
    {
      ch->sendln("You do not have the required 10000 platinum.");
      return eFAILURE;
    }
    for (int i = 0; i < 10; i++)
    { // Free up skills tied to the specilization.
      std::queue<skill_t> skills_to_delete = {};
      for (const auto &curr : ch->skills)
      {
        if (curr.first == spec_list[ch->spec].skills[i])
        {
          skills_to_delete.push(curr.first);
        }
      }
      while (skills_to_delete.empty() == false)
      {
        ch->skills.erase(skills_to_delete.front());
        skills_to_delete.pop();
      }
    }
    ch->spec = 0;
    GET_PLATINUM(ch) -= 10000;
    ch->saves[SAVE_TYPE_FIRE] -= (ch->getLevel() == 60 ? 1 : 0 + ch->getLevel() > 56 ? 1
                                                         : 0 + ch->getLevel() > 53   ? 1
                                                         : 0 + ch->getLevel() > 50   ? 1
                                                                                     : 0);
    ch->saves[SAVE_TYPE_COLD] -= (ch->getLevel() == 60 ? 1 : 0 + ch->getLevel() > 56 ? 1
                                                         : 0 + ch->getLevel() > 53   ? 1
                                                         : 0 + ch->getLevel() > 50   ? 1
                                                                                     : 0);
    ch->saves[SAVE_TYPE_ENERGY] -= (ch->getLevel() == 60 ? 1 : 0 + ch->getLevel() > 56 ? 1
                                                           : 0 + ch->getLevel() > 53   ? 1
                                                           : 0 + ch->getLevel() > 50   ? 1
                                                                                       : 0);
    ch->saves[SAVE_TYPE_ACID] -= (ch->getLevel() == 60 ? 1 : 0 + ch->getLevel() > 56 ? 1
                                                         : 0 + ch->getLevel() > 53   ? 1
                                                         : 0 + ch->getLevel() > 50   ? 1
                                                                                     : 0);
    ch->saves[SAVE_TYPE_MAGIC] -= (ch->getLevel() == 60 ? 1 : 0 + ch->getLevel() > 56 ? 1
                                                          : 0 + ch->getLevel() > 53   ? 1
                                                          : 0 + ch->getLevel() > 50   ? 1
                                                                                      : 0);
    ch->saves[SAVE_TYPE_POISON] -= (ch->getLevel() == 60 ? 1 : 0 + ch->getLevel() > 56 ? 1
                                                           : 0 + ch->getLevel() > 53   ? 1
                                                           : 0 + ch->getLevel() > 50   ? 1
                                                                                       : 0);
    ch->setLevel(51);

    // messagemoose
    ch->sendln("You forget your specilization.");
  }
  else if (!str_cmp(arg, "learn"))
  {
  }
  return eSUCCESS;
}
