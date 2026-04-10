/*

  Specilization stuff

*/

#include "DC/structs.h"
#include "DC/DC.h"

#include "DC/interp.h"
#include "DC/returnvals.h"

class spec_data
{
public:
  const QString name = {};
  const QString description = {};
  qint32 pcclass = {};
  qint32 skills[10] = {};
};

const spec_data spec_list[] =
    {
        {"Blah", "Blehe", 1, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {nullptr, nullptr, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}};

command_return_t do_spec(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  QString arg;
  argument = one_argument(argument, arg);
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;
  if (!str_cmp(arg, "list"))
  {
    /*
       for (qint32 i = {}; spec_list[i].name != nullptr; i++)
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
      return ReturnValue::eFAILURE;
    }
    if (ch->spec == 0)
    {
      ch->sendln("You do not have a specialization.");
      return ReturnValue::eFAILURE;
    }
    if (GET_PLATINUM(ch) < 10000)
    {
      ch->sendln("You do not have the required 10000 platinum.");
      return ReturnValue::eFAILURE;
    }
    for (qint32 i = {}; i < 10; i++)
    { // Free up skills tied to the specilization.
      QQueue<skill_t> skills_to_delete = {};
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
    ch->spec = {};
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
  return ReturnValue::eSUCCESS;
}
