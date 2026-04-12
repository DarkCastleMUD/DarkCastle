/*****************************************************
one liner quest shit
*****************************************************/

#include <math.h>

#include "DC/levels.h"
#include "DC/obj.h"
#include "DC/structs.h"
#include "DC/DC.h"
#include "DC/returnvals.h"
#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/quest.h"
#include <vector>
#include <cstring>

#include "DC/inventory.h"

quest_list_t quest_list;

const QStringList valid_fields = {
    "name",
    "level",
    "objnum",
    "objshort",
    "objlong",
    "objkey",
    "mobnum",
    "timer",
    "reward",
    "hint1",
    "hint2",
    "hint3",
    "cost",
    "brownie",
    nullptr};

qint32 load_quests(void)
{
  FILE *fl;
  quest_info *quest;

  if (!(fl = fopen(QUEST_FILE, "r")))
  {
    DC::getInstance()->logentry(u"Failed to open quest file for reading!"_s, 0, DC::LogChannel::LOG_MISC);
    return ReturnValue::eFAILURE;
  }

  while (fgetc(fl) != '$')
  {

    auto quest = new quest_info;

    quest->number = fread_int(fl, 0, 32768);
    quest->name = fread_string(fl, 1);
    quest->hint1 = fread_string(fl, 1);
    quest->hint2 = fread_string(fl, 1);
    quest->hint3 = fread_string(fl, 1);
    quest->objshort = fread_string(fl, 1);
    quest->objlong = fread_string(fl, 1);
    quest->objkey = fread_string(fl, 1);
    quest->level = fread_int(fl, 0, 32768);
    quest->objnum = fread_int(fl, 0, 32768);
    quest->mobnum = fread_int(fl, 0, 32768);
    quest->timer = fread_int(fl, 0, 32768);
    quest->reward = fread_int(fl, 0, 32768);
    quest->cost = fread_int(fl, 0, 32768);
    quest->brownie = fread_int(fl, 0, 32768);
    quest->active = false;

    quest_list.push_back(quest);
  }

  fclose(fl);

  return ReturnValue::eSUCCESS;
}

qint32 save_quests(void)
{
  FILE *fl;
  quest_info *quest;

  if (!(fl = fopen(QUEST_FILE, "w")))
  {
    DC::getInstance()->logentry(u"Failed to open quest file for writing!"_s, 0, DC::LogChannel::LOG_MISC);
    return ReturnValue::eFAILURE;
  }

  for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
  {
    quest = *node;
    dc_fprintf(fl, "#%d\n", quest->number);
    string_to_file(fl, quest->name);
    string_to_file(fl, quest->hint1);
    string_to_file(fl, quest->hint2);
    string_to_file(fl, quest->hint3);
    string_to_file(fl, quest->objshort);
    string_to_file(fl, quest->objlong);
    string_to_file(fl, quest->objkey);
    dc_fprintf(fl, "%d %d %d %d %d %d %d\n", quest->level, quest->objnum, quest->mobnum, quest->timer, quest->reward, quest->cost, quest->brownie);
  }

  dc_fprintf(fl, "$");

  fclose(fl);

  return ReturnValue::eSUCCESS;
}

quest_info *get_quest_(qint32 num)
{
  quest_info *quest;
  if (!num)
    return 0;

  for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
  {
    quest = *node;

    if (quest->number == num)
      return quest;
  }

  return 0;
}

quest_info *get_quest_(QString name)
{
  if (name == nullptr || name.isEmpty())
    return 0;

  for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
  {
    quest_info *quest = *node;

    if (str_nosp_equal(name, quest->name))
      return quest;

    if (atoi(name) == 0 && name[0] != '0')
    {
      continue;
    }

    if (quest->number == atoi(name))
    {
      return quest;
    }
  }

  return 0;
}

command_return_t do_add_quest(CharacterPtr ch, QString name)
{
  auto quest = new quest_info;

  quest->name = name;
  quest->hint1 = u" "_s;
  quest->hint2 = u" "_s;
  quest->hint3 = u" "_s;
  quest->objshort = u"a Quest Item"_s;
  quest->objlong = u"A quest item lies here."_s;
  quest->objkey = u"quest item"_s;
  quest->level = 75;
  quest->objnum = 51;
  quest->mobnum = QUEST_MASTER;
  quest->timer = {};
  quest->reward = {};
  quest->active = false;
  quest->cost = {};

  if (quest_list.empty() == true)
    quest->number = 1;
  else
    quest->number = quest_list.back()->number + 1;

  quest_list.push_back(quest);

  ch->send(u"Quest number %1 added.\r\n\r\n"_s).arg(quest->number));
  show_quest_info(ch, quest->number);

  return ReturnValue::eSUCCESS;
}

void list_quests(CharacterPtr ch, qint32 lownum, qint32 highnum)
{
  QString buffer;
  quest_info *quest;

  for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
  {
    quest = *node;

    if (quest->number <= highnum && quest->number >= lownum)
    {
      // Create a format QString based on a space offset that takes color codes into account

      ch->send(u"%1. $B$2Name:$7 %2$R Cost: %3 Reward: %4 Lvl: %5"_s.arg(quest->number, 3).arg(quest->name).arg(quest->cost).arg(quest->brownie ? "$5*$R" : "").arg(quest->reward).arg(quest->level));
    }
  }
}

void show_quest_info(CharacterPtr ch, qint32 num)
{
  quest_info *quest;

  for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
  {
    quest = *node;

    if (quest->number == num)
    {
      ch->sendln(u"$3Quest Info for #$R%d\r\n$3========================================$R\r\n$3Name:$R   %s\r\n$3Level:$R  %d\r\n$3Cost:$R   %d plats\r\n$3Brownie:$R%s\r\n$3Reward:$R %d qpoints\r\n$3Timer:$R  %d\r\n$3----------------------------------------$R\r\n$3Quest Mob Vnum:$R %d (%s)\r\n$3----------------------------------------$R\r\n$3Quest Object Vnum:$R %d\r\n$3Keywords:$R          %s\r\n$3Short description:$R %s\r\n$3Long description:$R  %s\r\n$3----------------------------------------$R\r\n$3Hints:$R\r\n$31.$R %s\r\n$32.$R %s\r\n$33.$R %s\r\n"_s
                     .arg(quest->number)
                     .arg(quest->name)
                     .arg(quest->level)
                     .arg(quest->cost)
                     .arg(quest->brownie ? "Required" : "Not Required")
                     .arg(quest->reward)
                     .arg(quest->timer)
                     .arg(quest->mobnum)
                     .arg(real_mobile(quest->mobnum) > 0 ? qPrintable(((CharacterPtr)(DC::getInstance()->mob_index[real_mobile(quest->mobnum)].item))->short_description()) : "no current mob")
                     .arg(quest->objnum)
                     .arg(quest->objkey)
                     .arg(quest->objshort)
                     .arg(quest->objlong)
                     .arg(quest->hint1)
                     .arg(quest->hint2)
                     .arg(quest->hint3));
      return;
    }
  }
  ch->sendln("That quest doesn't exist.");
}

bool check_available_quest(CharacterPtr ch, quest_info *quest)
{
  if (!quest)
    return false;

  if (ch->getLevel() >= quest->level && !check_quest_current(ch, quest->number) && !check_quest_complete(ch, quest->number) && !(quest->active))
    return true;

  return false;
}

bool check_quest_current(CharacterPtr ch, qint32 number)
{
  for (qint32 i = {}; i < QUEST_MAX; i++)
    if (ch->player->quest_current[i] == number)
      return true;
  return false;
}

bool check_quest_cancel(CharacterPtr ch, qint32 number)
{
  for (qint32 i = {}; i < QUEST_MAX_CANCEL; i++)
    if (ch->player->quest_cancel[i] == number)
      return true;
  return false;
}

bool check_quest_complete(CharacterPtr ch, qint32 number)
{
  if (ISSET(ch->player->quest_complete, number))
    return true;
  return false;
}

qint32 get_quest_price(quest_info *quest)
{
  return MIN(500, (qint32)(3.76 * pow(2.71828, quest->level * 0.0976) + 1));
}

void show_quest_header(CharacterPtr ch)
{
  ch->sendln(u"  .-------------------------------------------------------------------------."_s);
  ch->sendln(u" /.-.                                                                     .-.\\"_s);
  ch->sendln(u"[/   \\                                                                   /   \\]"_s);
  ch->sendln(u"[\\__. !                    $B$2Dark Castle Quest System$R                     ! ._/]"_s);
  ch->sendln(u"[\\  ! /                                                                 \\ !  /]"_s);
  ch->sendln(u"[ `--'                                                                   `--' ]"_s);
  ch->sendln(u"[-----------------------------------------------------------------------------]"_s);
  ch->sendln();
}

void show_quest_amount(CharacterPtr ch, qint32 remaining)
{
  ch->send(u"\r\n $B$2Completed: $7%-4d $2Remaining: $7%-4d $2Total: $7%-4d$R\r\n"_s.arg(quest_list.size() - remaining).arg(remaining).arg(quest_list.size()));
}

void show_quest_footer(CharacterPtr ch)
{
  quest_info *quest;
  qint32 attempting = {};
  qint32 completed = {};
  qint32 total = {};

  for (quest_list_t::iterator node = quest_list.begin();
       node != quest_list.end();
       node++)
  {
    quest = *node;

    if (ch->getLevel() >= quest->level)
    {
      if (check_quest_current(ch, quest->number))
      {
        // We are attempting this quest currently
        attempting++;
      }

      if (check_quest_complete(ch, quest->number))
      {
        // We did this quest already
        completed++;
      }

      if (!quest->active || check_quest_current(ch, quest->number))
      {
        // No other person is doing this quest right now
        total++;
      }
    }
  }

  ch->send(u"\r\n $B$2Attempting: $7%-4d $B$2Completed: $7%-4d $2Remaining: $7%-4d $2Total: $7%-4d$R\r\n"_s.arg(attempting).arg(completed).arg(total - completed - attempting).arg(total));

  ch->sendln("[-----------------------------------------------------------------------------]");
}

qint32 show_one_quest(CharacterPtr ch, quest_info *quest, qint32 count)
{
  qint32 i, amount = {};

  ch->sendln(u" $B$2Name:$7 %-35s    $B$2Quest Number:$7 %d$R\r\n $B$2Hint:$7 %-52s$R\r\n"_s.arg(quest->name).arg(quest->number).arg(quest->hint1));
  if (quest->hint2)
    ch->send(u" $B$7%-52s$R\r\n"_s.arg(quest->hint2));
  if (quest->hint3)
    ch->send(u" $B$7%-52s$R\r\n"_s.arg(quest->hint3));
  if (quest->timer)
  {
    for (i = {}; i < QUEST_MAX; i++)
    {
      if (quest->number == ch->player->quest_current[i])
      {
        amount = ch->player->quest_current_ticksleft[i];
      }

      if (!amount)
      {
        DC::getInstance()->logentry(u"Somebody passed a quest into here that they don't really have."_s, IMMORTAL, DC::LogChannel::LOG_BUG);
      }

      ch->send(u" $B$2Level:$7 %d  $2Time remaining:$7 %-7ld  $2Reward:$7 %-5d$R\r\n\r\n"_s.arg(quest->level).arg(amount).arg(quest->reward));
    }
  }
  else
  {
    ch->send(u" $B$2Level:$7 %d  $2Reward:$7 %-5d$R\r\n\r\n"_s.arg(quest->level).arg(quest->reward));
  }
  return ++count;
}

qint32 show_one_complete_quest(CharacterPtr ch, quest_info *quest, qint32 count)
{
  ch->send(u" $B$2Name:$7 %1 $2Reward:$7 %2$R\r\n"_s.arg(quest->name, -35).arg(quest->reward, -5));
  return ++count;
}

qint32 show_one_available_quest(CharacterPtr ch, quest_info *quest, qint32 count)
{
  ch->sendln(u"$B$7%1. $2Name:$7 %2$R Cost: %3 Reward: %4"_s
                 .arg(quest->name)
                 .arg(quest->cost)
                 .arg(quest->brownie ? "$5*$R" : "")
                 .arg(quest->reward));
  return ++count;
}

void show_available_quests(CharacterPtr ch)
{
  qint32 count = {};
  quest_info *quest;

  show_quest_header(ch);

  for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
  {
    quest = *node;

    if (check_available_quest(ch, quest))
    {
      count = show_one_available_quest(ch, quest, count);
      //         if(count >= QUEST_SHOW) break;
    }
  }
  if (!count)
    ch->sendln("$B$7There are currently no available quests for you, try later.$R");
  else
    //       show_quest_amount(ch, count);

    show_quest_footer(ch);
}

void show_canceled_quests(CharacterPtr ch)
{
  qint32 count = {};
  quest_info *quest;

  show_quest_header(ch);

  for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
  {
    quest = *node;

    if (check_quest_cancel(ch, quest->number))
      count = show_one_complete_quest(ch, quest, count);
  }
  //   show_quest_amount(ch, count);
  show_quest_footer(ch);
}

void show_current_quests(CharacterPtr ch)
{
  qint32 num_attempting = {};
  quest_info *quest;

  show_quest_header(ch);

  for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
  {
    quest = *node;

    if (check_quest_current(ch, quest->number))
      num_attempting = show_one_quest(ch, quest, num_attempting);
  }
  show_quest_footer(ch);
}

void show_complete_quests(CharacterPtr ch)
{
  qint32 count = {};
  quest_info *quest;

  show_quest_header(ch);

  for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
  {
    quest = *node;

    if (check_quest_complete(ch, quest->number))
      count = show_one_complete_quest(ch, quest, count);
  }
  //   show_quest_amount(ch, count);
  show_quest_footer(ch);
}

qint32 start_quest(CharacterPtr ch, quest_info *quest)
{
  qint32 count = {};
  quint16 price;
  ObjectPtr obj, brownie = {};
  CharacterPtr mob;
  QString buf;
  CharacterPtr qmaster = get_mob_vnum(QUEST_MASTER);

  if (check_quest_current(ch, quest->number))
  {
    dc_sprintf(buf, "q%d", quest->number);
    obj = get_obj(buf);
    if (!obj)
      return 0;
  }

  if (!check_available_quest(ch, quest))
  {
    ch->sendln("That quest is not available to you.");
    return ReturnValue::eFAILURE;
  }

  while (count < QUEST_MAX)
  {
    if (ch->player->quest_current[count] == -1)
      break;
    count++;
    if (count == QUEST_MAX)
    {
      ch->sendln("You've got too many quests started already.");
      return ReturnValue::eEXTRA_VALUE;
    }
  }

  price = quest->cost;
  if (GET_PLATINUM(ch) < price)
  {
    ch->send(u"You need %1 platinum coins to start this quest, which you don't have!\r\n"_s.arg(price));
    return ReturnValue::eEXTRA_VAL2;
  }

  if (quest->brownie)
  {
    brownie = get_obj_in_list_num(real_object(27906), ch->carrying);
    if (!brownie)
    {
      ch->send(u"You need a brownie point to start this quest!\r\n"_s.arg(price));
      return ReturnValue::eEXTRA_VAL2;
    }
  }

  qint32 dontwannabeinthisforever = {};

  if (!quest->number)
  { // recurring quest
    while (++dontwannabeinthisforever < 100)
    {
      mob = get_mob_vnum(number(1, 34000));
      if (mob && (mob->getLevel() < 90) && DC::getInstance()->zones.value(DC::getInstance()->world[mob->in_room].zone).isNoHunt() == false && (mob->description().length() > 80))
        break;
    }
    quest->hint1 = mob->description();
  }
  else
    mob = get_mob_vnum(quest->mobnum);

  if (!mob)
  {
    ch->sendln("This quest is temporarily unavailable.");
    return ReturnValue::eFAILURE;
  }

  obj = clone_object(real_object(quest->objnum));
  obj->short_description(quest->objshort);
  obj->long_description(quest->objlong);

  dc_sprintf(buf, "%s %s q%d", quest->objkey, qPrintable(ch->name()), quest->number);
  obj->name(buf);

  SET_BIT(obj->obj_flags.extra_flags, ITEM_SPECIAL);
  SET_BIT(obj->obj_flags.extra_flags, ITEM_QUEST);

  obj_to_char(obj, mob);
  wear(mob, obj, obj->keywordfind());

  DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_QUEST, "%s started quest %d (%s) costing %d plats %d brownie(s).", qPrintable(ch->name()), quest->number, quest->name, quest->cost, quest->brownie);

  ch->player->quest_current[count] = quest->number;
  ch->player->quest_current_ticksleft[count] = quest->timer;
  if (quest->number)
    quest->active = true;
  count = {};
  while (count < QUEST_MAX_CANCEL)
  {
    if (ch->player->quest_cancel[count] == quest->number)
    {
      ch->player->quest_cancel[count] = {};
      break;
    }
    count++;
  }

  if (quest->brownie)
  {
    ch->send(u"%s takes a brownie point from you.\r\n"_s.arg(qPrintable(qmaster->shortdesc_or_name())));
    obj_from_char(brownie);
  }

  ch->send(u"%s takes %d platinum from you.\r\n"_s.arg(qPrintable(qmaster->shortdesc_or_name())).arg(price));
  GET_PLATINUM(ch) -= price;

  return ReturnValue::eSUCCESS;
}

qint32 cancel_quest(CharacterPtr ch, quest_info *quest)
{
  qint32 count = {};

  if (!quest)
    return ReturnValue::eFAILURE;
  if (!check_quest_current(ch, quest->number))
    return ReturnValue::eFAILURE;

  while (count < QUEST_MAX_CANCEL)
  {
    if (!ch->player->quest_cancel[count])
      break;
    count++;
    if (count >= QUEST_MAX_CANCEL)
      return ReturnValue::eEXTRA_VALUE;
  }

  DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_QUEST, "%s canceled quest %d (%s).", qPrintable(ch->name()), quest->number, quest->name);

  ch->player->quest_cancel[count] = quest->number;

  return stop_current_quest(ch, quest);
}

qint32 complete_quest(CharacterPtr ch, quest_info *quest)
{
  qint32 count = {};
  ObjectPtr obj;
  QString buf;

  if (!quest)
    return ReturnValue::eEXTRA_VALUE;

  while (count < QUEST_MAX)
  {
    if (ch->player->quest_current[count] == quest->number)
      break;
    count++;
    if (count >= QUEST_MAX)
    {
      return ReturnValue::eEXTRA_VALUE;
    }
  }
  dc_sprintf(buf, "q%d", quest->number);
  obj = get_obj_in_list(buf, ch->carrying);

  if (!obj)
  {
    ch->sendln("You do not appear to have the quest object yet.");
    return ReturnValue::eFAILURE;
  }

  obj_from_char(obj);
  ch->player->quest_points += quest->reward;
  ch->player->quest_current[count] = -1;
  ch->player->quest_current_ticksleft[count] = {};
  if (quest->number) // quest 0 is recurring auto quest
    SETBIT(ch->player->quest_complete, quest->number);
  quest->active = false;

  DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_QUEST, "%s completed quest %d (%s) and won %d qpoints.", qPrintable(ch->name()), quest->number, quest->name, quest->reward);

  return ReturnValue::eSUCCESS;
}

qint32 stop_current_quest(CharacterPtr ch, quest_info *quest)
{
  qint32 count = {};
  ObjectPtr obj;
  QString buf;

  if (!quest)
    return ReturnValue::eFAILURE;

  while (count < QUEST_MAX)
  {
    if (ch->player->quest_current[count] == quest->number)
      break;
    count++;
    if (count >= QUEST_MAX)
    {
      return ReturnValue::eFAILURE;
    }
  }
  ch->player->quest_current[count] = -1;
  ch->player->quest_current_ticksleft[count] = {};
  quest->active = false;
  dc_sprintf(buf, "q%d", quest->number);
  obj = get_obj(buf);
  if (obj)
    extract_obj(obj);

  return ReturnValue::eSUCCESS;
}

qint32 stop_current_quest(CharacterPtr ch, qint32 number)
{
  if (!number)
    return ReturnValue::eFAILURE;

  quest_info *quest = get_quest_(number);
  return stop_current_quest(ch, quest);
}

qint32 stop_all_quests(CharacterPtr ch)
{
  if (!ch->player)
  {
    return ReturnValue::eFAILURE;
  }

  qint32 retval = {};

  for (qint32 i = {}; i < QUEST_MAX; i++)
  {
    retval &= stop_current_quest(ch, ch->player->quest_current[i]);
  }
  return retval;
}

void quest_update()
{
  QString buf;
  CharacterPtr mob;
  ObjectPtr obj;
  quest_info *quest;

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &i : character_list)
  {
    if (!i->desc || i->isNonPlayer())
      continue;

    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
    {
      quest = *node;

      if (quest->timer)
        for (qint32 j = {}; j < QUEST_MAX; j++)
          if (i->player->quest_current[j] == quest->number)
          {
            if (i->player->quest_current_ticksleft[j] <= 0)
            {
              stop_current_quest(i, quest);

              DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_QUEST, "%s ran out of time on quest %d (%s).", qPrintable(i->name()), quest->number, quest->name);

              i->send(u"Time has expired for %1.  This quest has ended.\r\n"_s).arg(quest->name));
            }
            i->player->quest_current_ticksleft[j]--;
            break;
          }

      if (check_quest_current(i, quest->number))
      {
        obj = get_obj(u"q%1"_s.arg(quest->number));
        if (!obj)
        {
          if ((mob = get_mob_vnum(quest->mobnum)))
          {
            obj = clone_object(quest->objnum);
            obj->short_description(quest->objshort);
            obj->long_description(quest->objlong);
            obj->name(u"%1 q%2"_s.arg(quest->objkey).arg(quest->number));
            obj_to_char(obj, mob);
            wear(mob, obj, obj->keywordfind());
          }
        }
      }
    }
  }
  DC::getInstance()->removeDead();
}

qint32 quest_handler(CharacterPtr ch, CharacterPtr qmaster, cmd_t cmd, QString name)
{
  qint32 retval = {};
  QString buf;
  quest_info *quest;

  if (cmd != cmd_t::QUEST_LIST)
  {
    quest = get_quest_(name);
    if (quest == 0)
    {
      ch->sendln("That is not a valid quest name or number.");
      return ReturnValue::eFAILURE;
    }
  }

  switch (cmd)
  {
  case cmd_t::QUEST_LIST:
    do_emote(qmaster, "looks at his notes and writes a scroll.");
    dc_sprintf(buf, "%s Here are some currently available quests.", qPrintable(ch->name()));
    do_psay(qmaster, buf);
    show_available_quests(ch);
    break;
  case cmd_t::QUEST_CANCEL:
    retval = cancel_quest(ch, quest);
    if (isSet(retval, ReturnValue::eSUCCESS))
    {
      dc_sprintf(buf, "%s You may begin this quest again if you speak with me.", qPrintable(ch->name()));
      do_psay(qmaster, buf);
    }
    else if (isSet(retval, ReturnValue::eEXTRA_VALUE))
    {
      dc_sprintf(buf, "%s You cannot cancel up any more quests without completing some of them.", qPrintable(ch->name()));
      do_psay(qmaster, buf);
    }
    else
    {
      dc_sprintf(buf, "%s You weren't doing this quest to begin with.", qPrintable(ch->name()));
      do_psay(qmaster, buf);
    }
    break;
  case cmd_t::QUEST_START:
    retval = start_quest(ch, quest);
    if (isSet(retval, ReturnValue::eSUCCESS))
    {
      if (quest->number)
      {
        dc_sprintf(buf, "%s Excellent!  Let me write down the quest information for you.", qPrintable(ch->name()));
        do_psay(qmaster, buf);
        do_emote(qmaster, "gives up the scroll.");
        show_quest_header(ch);
        show_one_quest(ch, quest, 0);
        show_quest_footer(ch);
      }
      else
      {
        dc_sprintf(buf, "%s I have placed a token of Phire upon a creature somewhere within the realms.", qPrintable(ch->name()));
        do_psay(qmaster, buf);
        dc_sprintf(buf, "%s Retrieve it for me within 12 hours for a reward!", qPrintable(ch->name()));
        do_psay(qmaster, buf);
        show_quest_header(ch);
        show_one_quest(ch, quest, 0);
        show_quest_footer(ch);
      }
    }
    else if (isSet(retval, ReturnValue::eEXTRA_VALUE))
    {
      dc_sprintf(buf, "%s You cannot start any more quests without completing some first.", qPrintable(ch->name()));
      do_psay(qmaster, buf);
    }
    else if (isSet(retval, ReturnValue::eEXTRA_VAL2))
    {
      dc_sprintf(buf, "%s You do not have the required funds to get the clue from me, beggar!", qPrintable(ch->name()));
      do_psay(qmaster, buf);
    }
    else if (!retval)
    {
      dc_sprintf(buf, "%s The quest item has left this world.  It will appear again soon.", qPrintable(ch->name()));
      do_psay(qmaster, buf);
    }
    else
    {
      dc_sprintf(buf, "%s Sorry, you cannot start this quest right now.", qPrintable(ch->name()));
      do_psay(qmaster, buf);
    }
    break;
  case cmd_t::QUEST_FINISH:
    retval = complete_quest(ch, quest);
    if (isSet(retval, ReturnValue::eSUCCESS))
    {
      dc_sprintf(buf, "%s This is it!  Wonderful job, I will add your reward to your current amount of points!", qPrintable(ch->name()));
      do_psay(qmaster, buf);
      ch->save(cmd_t::SAVE_SILENTLY);
    }
    else if (isSet(retval, ReturnValue::eEXTRA_VALUE))
    {
      dc_sprintf(buf, "%s You weren't doing this quest to begin with.", qPrintable(ch->name()));
      do_psay(qmaster, buf);
    }
    else
    {
      dc_sprintf(buf, "%s You have not yet completed this quest.", qPrintable(ch->name()));
      do_say(qmaster, buf);
    }
    break;
  default:
    DC::getInstance()->logentry(u"Bug in quest_handler, how'd they get here?"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  }
  return retval;
}

// Not used currently. Use quest list or quest start <name> instead of list or buy.
qint32 quest_master(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  qint32 choice;
  QString buf;

  if ((cmd != cmd_t::LIST) && (cmd != cmd_t::BUY))
    return ReturnValue::eFAILURE;

  if (IS_AFFECTED(ch, AFF_BLIND))
    return ReturnValue::eFAILURE;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (cmd == cmd_t::LIST)
  {
    show_available_quests(ch);
    return ReturnValue::eSUCCESS;
  }

  if (cmd == cmd_t::BUY)
  {
    if ((choice = atoi(arg)) == 0 || choice < 0)
    {
      dc_sprintf(buf, "%s Try a number from the list.", qPrintable(ch->name()));
      owner->do_tell(QString(buf).split(' '));
      return ReturnValue::eSUCCESS;
    }
    switch (atoi(arg))
    {
    case 1:
      do_say(owner, "Sure, bum.");
      break;
    default:
      dc_sprintf(buf, "%s I don't offer that service.", qPrintable(ch->name()));
      owner->do_tell(QString(buf).split(' '));
      break;
    }
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_quest(CharacterPtr ch, QString arg, cmd_t cmd)
{
  qint32 retval = {};
  QString name;
  QString new_arg = " ";
  CharacterPtr qmaster = get_mob_vnum(QUEST_MASTER);

  if (arg && strlen(arg) > 0 && arg[0] != ' ')
  {
    dc_strncat(new_arg, arg, sizeof(new_arg) - 1);
    arg = new_arg;
  }

  half_chop(arg, arg, name);

  if (is_abbrev(arg, "current"))
    show_current_quests(ch);
  else if (is_abbrev(arg, "canceled") && name.isEmpty())
    show_canceled_quests(ch);
  else if (is_abbrev(arg, "completed"))
    show_complete_quests(ch);
  else if (is_abbrev(arg, "list"))
  {
    if (!qmaster)
      return ReturnValue::eFAILURE;
    if (ch->in_room != qmaster->in_room)
      ch->sendln("You must ask the Quest Master for available quests.");
    else
      retval = quest_handler(ch, qmaster, cmd_t::QUEST_LIST, 0);
  }
  else if (is_abbrev(arg, "cancel") && *name)
  {
    if (!qmaster)
      return ReturnValue::eFAILURE;
    if (ch->in_room != qmaster->in_room)
      ch->sendln("You must let the Quest Master know of your intentions.");
    else
      retval = quest_handler(ch, qmaster, cmd_t::QUEST_CANCEL, name);
    return retval;
  }
  else if (is_abbrev(arg, "start") && *name)
  {
    if (!qmaster)
      return ReturnValue::eFAILURE;
    if (ch->in_room != qmaster->in_room)
      ch->sendln("You may only begin quests given from the Quest Master.");
    else
      retval = quest_handler(ch, qmaster, cmd_t::QUEST_START, name);
    return retval;
  }
  else if (is_abbrev(arg, "finish") && *name)
  {
    if (!qmaster)
      return ReturnValue::eFAILURE;
    if (ch->in_room != qmaster->in_room)
      ch->sendln("You may only finish quests in the presence of the Quest Master.");
    else
      retval = quest_handler(ch, qmaster, cmd_t::QUEST_FINISH, name);
    return retval;
  }
  else if (is_abbrev(arg, "reset"))
  {
    if (!qmaster)
    {
      return ReturnValue::eFAILURE;
    }

    if (ch->in_room != qmaster->in_room)
    {
      ch->sendln("You may only reset all quests in the presence of the Quest Master.");
      return ReturnValue::eFAILURE;
    }

    quest_info *quest;
    qint32 attempting = {};
    qint32 completed = {};
    qint32 total = {};
    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
    {
      quest = *node;

      if (ch->getLevel() >= quest->level)
      {
        if (check_quest_current(ch, quest->number))
        {
          // We are attempting this quest currently
          attempting++;
        }

        if (check_quest_complete(ch, quest->number))
        {
          // We did this quest already
          completed++;
        }

        if (!quest->active || check_quest_current(ch, quest->number))
        {
          // No other person is doing this quest right now
          total++;
        }
      }
    }

    if (completed < 100)
    {
      ch->sendln("You will need to complete at least 100 quests before you can reset.");
      return ReturnValue::eFAILURE;
    }

    if (GET_PLATINUM(ch) < 2000)
    {
      ch->sendln("You need 2000 platinum coins to reset all quests, which you don't have!");
      return ReturnValue::eEXTRA_VAL2;
    }

    ObjectPtr brownie = get_obj_in_list_num(real_object(27906), ch->carrying);
    if (!brownie)
    {
      ch->sendln("You need a brownie point to reset all quests!");
      return ReturnValue::eFAILURE;
    }

    ch->send(u"%s takes 2000 platinum from you.\r\n"_s).arg(qPrintable(qmaster->shortdesc_or_name())));
    GET_PLATINUM(ch) -= 2000;
    ch->send(u"%s takes a brownie point from you.\r\n"_s.arg(qPrintable(qmaster->shortdesc_or_name())));
    obj_from_char(brownie);

    stop_all_quests(ch);
    for (qint32 i = {}; i < QUEST_MAX; i++)
    {
      ch->player->quest_current[i] = -1;
      ch->player->quest_current_ticksleft[i] = {};
    }
    memset(ch->player->quest_cancel, 0, sizeof(ch->player->quest_cancel));
    memset(ch->player->quest_complete, 0, sizeof(ch->player->quest_complete));
    ch->sendln("All quests have been reset.");
    return retval;
  }
  else
  {
    ch->sendln(u"Usage: quest current            (lists current quests)\r\n       quest completed          (lists completed quests)\r\n       quest canceled           (lists canceled quests)\r\n\r\nThe following commands may only be used at the Quest Master.\r\n       quest list               (lists available quests)\r\n       quest cancel <name or #> (cancel the current quest)\r\n       quest start <name or #>  (starts a new quest)\r\n       quest finish <name or #> (finishes a current quest)\r\n       quest reset              (reset all quests. costs 2k plats, 1 brownie)\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_qedit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  QString field;
  QString value;
  qint32 holdernum;
  qint32 i, lownum, highnum;
  quest_info *quest = {};
  CharacterPtr vict = {};

  half_chop(argument, arg, argument);
  for (; *argument == ' '; argument++)
    ;

  if (arg.isEmpty())
  {
    ch->sendln(u"Usage: qedit list                      (list all quest names and numbers)\r\n       qedit list <lownum> <highnum>   (lists names and numbers between)\r\n       qedit show <number>             (show detailed information)\r\n       qedit <number> <field> <value>  (edit a quest)\r\n       qedit new <name>                (add a quest)\r\n       qedit save                      (saves all quests)\r\n       qedit stat <playername>         (show player's current qpoints)\r\n       qedit set <playername> <value>  (alter player's current qpoints)\r\n       qedut reset <playername>\r\n\r\nValid qedit fields:"_s);

    // Display all of qedit's valid fields in rows of 4 columns
    //
    QString *tmp = valid_fields;
    qint32 i = {};
    while (*tmp != nullptr)
    {
      ch->send(u"%1\t"_s.arg(*tmp));
      if (++i % 4 == 0)
      {
        ch->sendln("");
      }
      tmp++;
    }
    ch->sendln("");

    return ReturnValue::eFAILURE;
  }

  if (is_abbrev(arg, "save"))
  {
    ch->sendln("Quests saved.");
    return save_quests();
  }

  if (is_abbrev(arg, "new"))
  {
    if (argument.isEmpty())
    {
      ch->sendln("Usage: qedit new <name>");
      return ReturnValue::eFAILURE;
    }
    else
    {
      quest = get_quest_(argument);
      if (quest)
      {
        ch->sendln("A quest by this name already exists.");
        return ReturnValue::eFAILURE;
      }
      else
      {
        do_add_quest(ch, argument);
        return ReturnValue::eSUCCESS;
      }
    }
  }

  half_chop(argument, field, argument);
  for (; *argument == ' '; argument++)
    ;

  if (*arg && is_number(arg) && field.isEmpty())
  {
    show_quest_info(ch, atoi(arg));
    return ReturnValue::eSUCCESS;
  }

  if (is_abbrev(arg, "stat"))
  {
    if (field.isEmpty())
    {
      ch->sendln("Usage: qedit stat <playername>");
      return ReturnValue::eFAILURE;
    }
    else
    {
      if (!(vict = get_char_vis(ch, field)) || vict->isNonPlayer())
      {
        ch->sendln("No living thing by that name.");
        return ReturnValue::eFAILURE;
      }

      ch->send(u"%s's quest points: %d\r\n"_s.arg(qPrintable(vict->name())).arg(vict->player->quest_points));
    }
    return ReturnValue::eSUCCESS;
  }

  if (is_abbrev(arg, "reset"))
  {
    if (field.isEmpty())
    {
      ch->sendln("Usage: qedit reset <playername>");
      return ReturnValue::eFAILURE;
    }
    else
    {
      if (!(vict = get_char_vis(ch, field)) || vict->isNonPlayer())
      {
        ch->sendln("No living thing by that name.");
        return ReturnValue::eFAILURE;
      }

      memset(vict->player->quest_cancel, 0, sizeof(vict->player->quest_cancel));
      memset(vict->player->quest_complete, 0, sizeof(vict->player->quest_complete));

      ch->send(u"Reset quests for player %1\r\n"_s.arg(qPrintable(vict->name())));
      vict->save(cmd_t::SAVE_SILENTLY);
      return ReturnValue::eSUCCESS;
    }
  }

  if (is_abbrev(arg, "set"))
  {
    half_chop(argument, value, argument);
    for (; *argument == ' '; argument++)
      ;

    if (field.isEmpty() || value.isEmpty() || !is_number(value))
    {
      ch->sendln("Usage: qedit set <playername> <value>");
      return ReturnValue::eFAILURE;
    }
    else
    {
      if (!(vict = get_char_vis(ch, field)) || vict->isNonPlayer())
      {
        ch->sendln("No living thing by that name.");
        return ReturnValue::eFAILURE;
      }

      DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_QUEST, "%s set %s's quest points from %d to %d.", qPrintable(ch->name()), qPrintable(vict->name()),
                              vict->player->quest_points, atoi(value));
      ch->send(u"Setting %s's quest points from %d to %d.\r\n"_s.arg(qPrintable(vict->name())).arg(vict->player->quest_points).arg(atoi(value)));

      vict->player->quest_points = atoi(value);
    }
    return ReturnValue::eSUCCESS;
  }

  if (is_abbrev(arg, "show"))
  {
    if (field.isEmpty() || !is_number(field))
      ch->sendln("Usage: qedit show <number>");
    else
      show_quest_info(ch, atoi(field));
    return ReturnValue::eSUCCESS;
  }

  half_chop(argument, value, argument);
  for (; *argument == ' '; argument++)
    ;

  if (is_abbrev(arg, "list") && field.isEmpty())
  {
    list_quests(ch, 0, QUEST_TOTAL);
    return ReturnValue::eSUCCESS;
  }
  else if (is_abbrev(arg, "list") && *field && is_number(field))
  {
    lownum = MAX(0, atoi(field));
    if (*value && is_number(value))
    {
      highnum = MIN(QUEST_TOTAL, atoi(value));
      if (lownum > highnum)
      {
        holdernum = lownum;
        lownum = highnum;
        highnum = holdernum;
      }
      list_quests(ch, lownum, highnum);
    }
    else
      list_quests(ch, lownum, QUEST_TOTAL);
    return ReturnValue::eSUCCESS;
  }

  if (!is_number(arg))
  {
    ch->sendln("Usage: qedit <number> <field> <value>");
    return ReturnValue::eFAILURE;
  }

  holdernum = atoi(arg);

  if (holdernum <= 0 || holdernum > QUEST_TOTAL)
  {
    ch->sendln("Invalid quest number.");
    return ReturnValue::eFAILURE;
  }

  if (!(quest = get_quest_(holdernum)))
  {
    ch->sendln("That quest doesn't exist.");
    return ReturnValue::eFAILURE;
  }

  if (field.isEmpty())
  {
    ch->sendln("Valid fields: name, level, cost, brownie, objnum, objshort, objlong, objkey, mobnum, timer, reward or hints.");
    return ReturnValue::eFAILURE;
  }

  if (!(*value))
  {
    ch->sendln("You must enter a value.");
    return ReturnValue::eFAILURE;
  }

  i = {};
  while (valid_fields[i] != nullptr)
  {
    if (is_abbrev(field, valid_fields[i]))
      break;
    else
      i++;
  }

  if (valid_fields[i] == nullptr)
  {
    ch->sendln("Valid fields: name, level, cost, brownie, objnum, objshort, objlong, objkey, mobnum, timer, reward, hint1, hint2, or hint3.");
    return ReturnValue::eFAILURE;
  }

  quest_info *oldquest;

  switch (i)
  {
  case 0: // name
    dc_sprintf(field, "%s %s", value, argument);
    oldquest = get_quest_(field);
    if (oldquest)
    {
      ch->sendln("A quest by this name already exists.");
      return ReturnValue::eFAILURE;
    }
    else
    {
      ch->send(u"Name changed from %1 "_s.arg(quest->name));
      quest->name = field;
      ch->send(u"to %1.\r\n"_s).arg(quest->name));
    }
    break;
  case 1: // level
    ch->send(u"Level changed from %1 "_s.arg(quest->level));
    quest->level = atoi(value);
    ch->send(u"to %1.\r\n"_s).arg(quest->level));
    break;
  case 2: // objnum
    ch->send(u"Objnum changed from %1 "_s.arg(quest->objnum));
    quest->objnum = atoi(value);
    ch->send(u"to %1.\r\n"_s).arg(quest->objnum));
    break;
  case 3: // objshort
    ch->send(u"Objshort changed from %1 "_s.arg(quest->objshort));
    dc_sprintf(field, "%s %s", value, argument);
    quest->objshort = field;
    ch->send(u"to %1.\r\n"_s).arg(quest->objshort));
    break;
  case 4: // objlong
    ch->send(u"Objlong changed from %1 "_s.arg(quest->objlong));
    dc_sprintf(field, "%s %s", value, argument);
    quest->objlong = field;
    ch->send(u"to %1.\r\n"_s).arg(quest->objlong));
    break;
  case 5: // objkey
    ch->send(u"Objkey changed from %1 "_s.arg(quest->objkey));
    dc_sprintf(field, "%s %s", value, argument);
    quest->objkey = field;
    ch->send(u"to %1.\r\n"_s).arg(quest->objkey));
    break;
  case 6: // mobnum
    ch->send(u"Mobnum changed from %1 "_s.arg(quest->mobnum));
    quest->mobnum = atoi(value);
    ch->send(u"to %1.\r\n"_s).arg(quest->mobnum));
    break;
  case 7: // timer
    ch->send(u"Timer changed from %1 "_s.arg(quest->timer));
    quest->timer = atoi(value);
    ch->send(u"to %1.\r\n"_s).arg(quest->timer));
    break;
  case 8: // reward
    ch->send(u"Reward changed from %1 "_s.arg(quest->reward));
    quest->reward = atoi(value);
    ch->send(u"to %1.\r\n"_s).arg(quest->reward));
    break;
  case 9: // hint1
    dc_sprintf(field, "%s %s", value, argument);
    ch->send(u"Hint #1 changed from %s "_s.arg(quest->hint1));
    quest->hint1 = field;
    ch->send(u"to %s.\r\n"_s.arg(quest->hint1));
    break;
  case 10: // hint2
    dc_sprintf(field, "%s %s", value, argument);
    ch->send(u"Hint #2 changed from %s "_s.arg(quest->hint2));
    quest->hint2 = field;
    ch->send(u"to %s.\r\n"_s.arg(quest->hint2));
    break;
  case 11: // hint3
    dc_sprintf(field, "%s %s", value, argument);
    ch->send(u"Hint #3 changed from %s "_s.arg(quest->hint3));
    quest->hint3 = field;
    ch->send(u"to %s.\r\n"_s.arg(quest->hint3));
    break;
  case 12: // cost
    ch->send(u"Cost changed from %1 "_s.arg(quest->cost));
    quest->cost = atoi(value);
    ch->send(u"to %1.\r\n"_s).arg(quest->cost));
    break;
  case 13: // brownie
    if (quest->brownie)
    {
      ch->sendln("Brownie toggled to NOT required.");
      quest->brownie = {};
    }
    else
    {
      ch->sendln("Brownie toggled to required.");
      quest->brownie = 1;
    }
    break;
  default:
    DC::getInstance()->logentry(u"Screw up in do_edit_quest, whatsamaddahyou?"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  }
  return ReturnValue::eSUCCESS;
}

qint32 quest_vendor(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  QString buf;
  qint32 rnum = {};

  // list & buy & sell
  if ((cmd != cmd_t::LIST) && (cmd != cmd_t::BUY) && (cmd != cmd_t::SELL))
  {
    return ReturnValue::eFAILURE;
  }

  if (!CAN_SEE(ch, owner))
  {
    return ReturnValue::eFAILURE;
  }

  if (ch->isNonPlayer())
  {
    return ReturnValue::eFAILURE;
  }

  if (!CAN_SEE(owner, ch))
  {
    do_say(owner, "I don't trade with people I can't see!");
    return ReturnValue::eSUCCESS;
  }

  if (cmd == cmd_t::LIST)
  { /* List */
    ch->sendln("$B$2Orro tells you, 'This is what I can do for you...$R ");
    ch->sendln("$BQuest Equipment:$R");

    qint32 n = {};
    for (qint32 qvnum = 27975; qvnum < 27997; qvnum++)
    {
      rnum = real_object(qvnum);
      if (rnum >= 0)
      {
        auto buffer = gl_item(DC::getInstance()->obj_index[rnum].item, n++, ch, false);
        ch->send(buffer);
      }
    }
    for (qint32 qvnum = 27943; qvnum <= 27953; qvnum++)
    {
      rnum = real_object(qvnum);
      if (rnum >= 0)
      {
        auto buffer = gl_item(DC::getInstance()->obj_index[rnum].item, n++, ch, false);
        ch->send(buffer);
      }
    }
    for (qint32 qvnum = 3124; qvnum <= 3128; qvnum++)
    {
      rnum = real_object(qvnum);
      if (rnum >= 0)
      {
        auto buffer = gl_item(DC::getInstance()->obj_index[rnum].item, n++, ch, false);
        ch->send(buffer);
      }
    }
    for (qint32 qvnum = 3151; qvnum <= 3158; qvnum++)
    {
      rnum = real_object(qvnum);
      if (rnum >= 0)
      {
        auto buffer = gl_item(DC::getInstance()->obj_index[rnum].item, n++, ch, false);
        ch->send(buffer);
      }
    }
  }
  else if (cmd == cmd_t::BUY)
  { /* buy */
    QString arg2;
    one_argument(arg, arg2);

    if (!is_number(arg2))
    {
      owner->do_tell(u"%1 Sorry, mate. You type buy <number> to specify what you want.."_s.arg(qPrintable(ch->name())).split(' '));
      return ReturnValue::eSUCCESS;
    }

    bool FOUND = false;
    qint32 want_num = atoi(arg2) - 1;
    qint32 n = {};
    for (qint32 qvnum = 27975; qvnum <= 27996; qvnum++)
    {
      rnum = real_object(qvnum);
      if (rnum >= 0 && n++ == want_num)
      {
        FOUND = true;
        break;
      }
    }
    if (!FOUND)
    {
      for (qint32 qvnum = 27943; qvnum <= 27953; qvnum++)
      {
        rnum = real_object(qvnum);
        if (rnum >= 0 && n++ == want_num)
        {
          FOUND = true;
          break;
        }
      }
    }
    if (!FOUND)
    {
      for (qint32 qvnum = 3124; qvnum <= 3128; qvnum++)
      {
        rnum = real_object(qvnum);
        if (rnum >= 0 && n++ == want_num)
        {
          FOUND = true;
          break;
        }
      }
    }
    if (!FOUND)
    {
      for (qint32 qvnum = 3151; qvnum <= 3158; qvnum++)
      {
        rnum = real_object(qvnum);
        if (rnum >= 0 && n++ == want_num)
        {
          FOUND = true;
          break;
        }
      }
    }

    if (!FOUND)
    {
      owner->do_tell(u"%1 Don't have that I'm afraid. Type \"list\" to see my wares."_s.arg(qPrintable(ch->name())).split(' '));
      return ReturnValue::eSUCCESS;
    }

    ObjectPtr obj;
    obj = clone_object(rnum);

    /*      if (class_restricted(ch, obj)) {
         dc_sprintf(buf, "%s That item is meant for another class.", qPrintable(ch->name()));
         do_tell(owner, buf);
         extract_obj(obj);
         return ReturnValue::eSUCCESS;
          } else if (size_restricted(ch, obj)) {
         dc_sprintf(buf, "%s That item would not fit you.", qPrintable(ch->name()));
         do_tell(owner, buf);
         extract_obj(obj);
         return ReturnValue::eSUCCESS;
          } else */
    if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) &&
        search_char_for_item(ch, obj->item_number, false))
    {
      owner->do_tell(u"%1 You already have one of those."_s).arg(qPrintable(ch->name())).split(' '));
      extract_obj(obj);
      return ReturnValue::eSUCCESS;
    }

    if (GET_QPOINTS(ch) < (quint32)(obj->obj_flags.cost / 10000))
    {
      owner->do_tell(u"%1 Come back when you've got the qpoints."_s.arg(qPrintable(ch->name())).split(' '));
      extract_obj(obj);
      return ReturnValue::eSUCCESS;
    }

    GET_QPOINTS(ch) -= (obj->obj_flags.cost / 10000);

    SET_BIT(obj->obj_flags.more_flags, ITEM_24H_NO_SELL);
    SET_BIT(obj->obj_flags.more_flags, ITEM_CUSTOM);
    obj->no_sell_expiration = time(nullptr) + (60 * 60 * 24);

    obj_to_char(obj, ch);
    owner->do_tell(u"%1 Here's your %2$B$2. Have a nice time with it."_s.arg(qPrintable(ch->name())).arg(obj->short_description()).split(' '));
    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::SELL)
  { /* Sell */
    QString arg2;
    one_argument(arg, arg2);

    ObjectPtr obj = get_obj_in_list_vis(ch, arg2, ch->carrying);
    if (!obj)
    {
      owner->do_tell(u"%1 Try that on the kooky meta-physician.."_s.arg(qPrintable(ch->name())).split(' '));
      return ReturnValue::eSUCCESS;
    }

    if (!obj->isQuest())
    {
      owner->do_tell(u"%1 I only buy quest equipment."_s).arg(qPrintable(ch->name())).split(' '));
      return ReturnValue::eSUCCESS;
    }

    if (isSet(obj->obj_flags.more_flags, ITEM_24H_NO_SELL))
    {
      time_t now = time(nullptr);
      time_t expires = obj->no_sell_expiration;
      if (now < expires)
      {
        owner->do_tell(u"%1 I won't buy that for another %2 seconds."_s.arg(qPrintable(ch->name())).arg(expires - now).split(' '));
        return ReturnValue::eSUCCESS;
      }
    }

    qint32 cost = obj->obj_flags.cost / 10000.0;

    owner->do_tell(u"%1 I'll give you %2 qpoints for that. Thanks for shoppin'."_s.arg(qPrintable(ch->name())).arg(cost).split(' '));
    extract_obj(obj);
    GET_QPOINTS(ch) += cost;
    return ReturnValue::eSUCCESS;
  }

  return ReturnValue::eSUCCESS;
}
