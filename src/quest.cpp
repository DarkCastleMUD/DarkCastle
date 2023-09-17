/*****************************************************
one liner quest shit
*****************************************************/

#include <math.h>
#include "structs.h"
#include "character.h"
#include "comm.h"
#include "fileinfo.h"
#include "returnvals.h"
#include "obj.h"
#include "act.h"
#include "levels.h"
#include "interp.h"
#include "handler.h"
#include "db.h"
#include "connect.h"
#include "quest.h"
#include "spells.h"
#include <vector>
#include <string.h>
#include "room.h"
#include "inventory.h"

using namespace std;

typedef vector<quest_info *> quest_list_t;
quest_list_t quest_list;

extern CWorld world;

char *valid_fields[] = {
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

extern void wear(Character *, Object *, int);
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern char *gl_item(Object *obj, int number, Character *ch, bool platinum);

int load_quests(void)
{
   FILE *fl;
   quest_info *quest;

   if (!(fl = fopen(QUEST_FILE, "r")))
   {
      logentry("Failed to open quest file for reading!", 0, LogChannels::LOG_MISC);
      return eFAILURE;
   }

   while (fgetc(fl) != '$')
   {

#ifdef LEAK_CHECK
      quest = (struct quest_info *)calloc(1, sizeof(struct quest_info));
#else
      quest = (struct quest_info *)dc_alloc(1, sizeof(struct quest_info));
#endif

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

   return eSUCCESS;
}

int save_quests(void)
{
   FILE *fl;
   quest_info *quest;

   if (!(fl = fopen(QUEST_FILE, "w")))
   {
      logentry("Failed to open quest file for writing!", 0, LogChannels::LOG_MISC);
      return eFAILURE;
   }

   for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
   {
      quest = *node;
      fprintf(fl, "#%d\n", quest->number);
      string_to_file(fl, quest->name);
      string_to_file(fl, quest->hint1);
      string_to_file(fl, quest->hint2);
      string_to_file(fl, quest->hint3);
      string_to_file(fl, quest->objshort);
      string_to_file(fl, quest->objlong);
      string_to_file(fl, quest->objkey);
      fprintf(fl, "%d %d %d %d %d %d %d\n", quest->level, quest->objnum, quest->mobnum, quest->timer, quest->reward, quest->cost, quest->brownie);
   }

   fprintf(fl, "$");

   fclose(fl);

   return eSUCCESS;
}

struct quest_info *get_quest_struct(int num)
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

struct quest_info *get_quest_struct(char *name)
{
   if (name == nullptr || !*name)
      return 0;

   for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
   {
      struct quest_info *quest = *node;

      if (!str_nosp_cmp(name, quest->name))
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

int do_add_quest(Character *ch, char *name)
{
   struct quest_info *quest; // new quest

#ifdef LEAK_CHECK
   quest = (struct quest_info *)calloc(1, sizeof(struct quest_info));
#else
   quest = (struct quest_info *)dc_alloc(1, sizeof(struct quest_info));
#endif

   quest->name = str_hsh(name);
   quest->hint1 = str_hsh(" ");
   quest->hint2 = str_hsh(" ");
   quest->hint3 = str_hsh(" ");
   quest->objshort = str_hsh("a Quest Item");
   quest->objlong = str_hsh("A quest item lies here.");
   quest->objkey = str_hsh("quest item");
   quest->level = 75;
   quest->objnum = 51;
   quest->mobnum = QUEST_MASTER;
   quest->timer = 0;
   quest->reward = 0;
   quest->active = false;
   quest->cost = 0;

   if (quest_list.empty() == true)
      quest->number = 1;
   else
      quest->number = quest_list.back()->number + 1;

   quest_list.push_back(quest);

   csendf(ch, "Quest number %d added.\n\r\n\r", quest->number);
   show_quest_info(ch, quest->number);

   return eSUCCESS;
}

void list_quests(Character *ch, int lownum, int highnum)
{
   char buffer[MAX_STRING_LENGTH];
   struct quest_info *quest;

   for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
   {
      quest = *node;

      if (quest->number <= highnum && quest->number >= lownum)
      {
         // Create a format string based on a space offset that takes color codes into account
         snprintf(buffer, MAX_STRING_LENGTH,
                  "%%3d. $B$2Name:$7 %%-%ds$R Cost: %%-4d%%1s Reward: %%-4d Lvl: %%d\n\r",
                  35 + (strlen(quest->name) - nocolor_strlen(quest->name)));

         csendf(ch, buffer, quest->number, quest->name, quest->cost, quest->brownie ? "$5*$R" : "", quest->reward, quest->level);
      }
   }

   return;
}

void show_quest_info(Character *ch, int num)
{
   struct quest_info *quest;

   for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
   {
      quest = *node;

      if (quest->number == num)
      {
         csendf(ch,
                "$3Quest Info for #$R%d\n\r"
                "$3========================================$R\n\r"
                "$3Name:$R   %s\n\r"
                "$3Level:$R  %d\n\r"
                "$3Cost:$R   %d plats\n\r"
                "$3Brownie:$R%s\n\r"
                "$3Reward:$R %d qpoints\n\r"
                "$3Timer:$R  %d\n\r"
                "$3----------------------------------------$R\n\r"
                "$3Quest Mob Vnum:$R %d (%s)\n\r"
                "$3----------------------------------------$R\n\r"
                "$3Quest Object Vnum:$R %d\n\r"
                "$3Keywords:$R          %s\n\r"
                "$3Short description:$R %s\n\r"
                "$3Long description:$R  %s\n\r"
                "$3----------------------------------------$R\n\r"
                "$3Hints:$R\n\r"
                "$31.$R %s\n\r"
                "$32.$R %s\n\r"
                "$33.$R %s\n\r",
                quest->number, quest->name, quest->level, quest->cost,
                quest->brownie ? "Required" : "Not Required",
                quest->reward, quest->timer, quest->mobnum,
                real_mobile(quest->mobnum) > 0 ? ((Character *)(mob_index[real_mobile(quest->mobnum)].item))->short_desc : "no current mob",
                quest->objnum, quest->objkey,
                quest->objshort, quest->objlong, quest->hint1, quest->hint2, quest->hint3);
         return;
      }
   }
   send_to_char("That quest doesn't exist.\r\n", ch);
}

bool check_available_quest(Character *ch, struct quest_info *quest)
{
   if (!quest)
      return false;

   if (GET_LEVEL(ch) >= quest->level && !check_quest_current(ch, quest->number) && !check_quest_complete(ch, quest->number) && !(quest->active))
      return true;

   return false;
}

bool check_quest_current(Character *ch, int number)
{
   for (int i = 0; i < QUEST_MAX; i++)
      if (ch->player->quest_current[i] == number)
         return true;
   return false;
}

bool check_quest_cancel(Character *ch, int number)
{
   for (int i = 0; i < QUEST_CANCEL; i++)
      if (ch->player->quest_cancel[i] == number)
         return true;
   return false;
}

bool check_quest_complete(Character *ch, int number)
{
   if (ISSET(ch->player->quest_complete, number))
      return true;
   return false;
}

int get_quest_price(struct quest_info *quest)
{
   return MIN(500, (int)(3.76 * pow(2.71828, quest->level * 0.0976) + 1));
}

void show_quest_header(Character *ch)
{
   csendf(ch, "  .-------------------------------------------------------------------------.\r\n"
              " /.-.                                                                     .-.\\\n\r"
              "[/   \\                                                                   /   \\]\n\r"
              "[\\__. !                    $B$2Dark Castle Quest System$R                     ! ._/]\n\r"
              "[\\  ! /                                                                 \\ !  /]\n\r"
              "[ `--'                                                                   `--' ]\n\r"
              "[-----------------------------------------------------------------------------]\n\r\n\r");
   return;
}

void show_quest_amount(Character *ch, int remaining)
{
   csendf(ch,
          "\n\r $B$2Completed: $7%-4d $2Remaining: $7%-4d $2Total: $7%-4d$R\n\r", quest_list.size() - remaining, remaining, quest_list.size());
   return;
}

void show_quest_footer(Character *ch)
{
   quest_info *quest;
   int attempting = 0;
   int completed = 0;
   int total = 0;

   for (quest_list_t::iterator node = quest_list.begin();
        node != quest_list.end();
        node++)
   {
      quest = *node;

      if (GET_LEVEL(ch) >= quest->level)
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

   csendf(ch,
          "\n\r $B$2Attempting: $7%-4d $B$2Completed: $7%-4d $2Remaining: $7%-4d $2Total: $7%-4d$R\n\r",
          attempting, completed, total - completed - attempting, total);

   csendf(ch, "[-----------------------------------------------------------------------------]\n\r");
   return;
}

int show_one_quest(Character *ch, struct quest_info *quest, int count)
{
   int i, amount = 0;

   csendf(ch, " $B$2Name:$7 %-35s    $B$2Quest Number:$7 %d$R\n\r"
              " $B$2Hint:$7 %-52s$R\n\r",
          quest->name, quest->number, quest->hint1);
   if (quest->hint2)
      csendf(ch, " $B$7%-52s$R\n\r", quest->hint2);
   if (quest->hint3)
      csendf(ch, " $B$7%-52s$R\n\r", quest->hint3);
   if (quest->timer)
   {
      for (i = 0; i < QUEST_MAX; i++)
      {
         if (quest->number == ch->player->quest_current[i])
         {
            amount = ch->player->quest_current_ticksleft[i];
         }

         if (!amount)
         {
            logentry("Somebody passed a quest into here that they don't really have.", IMMORTAL, LogChannels::LOG_BUG);
         }

         csendf(ch, " $B$2Level:$7 %d  $2Time remaining:$7 %-7ld  $2Reward:$7 %-5d$R\n\r\n\r",
                quest->level, amount, quest->reward);
      }
   }
   else
   {
      csendf(ch, " $B$2Level:$7 %d  $2Reward:$7 %-5d$R\n\r\n\r",
             quest->level, quest->reward);
   }
   return ++count;
}

int show_one_complete_quest(Character *ch, struct quest_info *quest, int count)
{
   csendf(ch, " $B$2Name:$7 %-35s $2Reward:$7 %-5d$R\n\r", quest->name, quest->reward);
   return ++count;
}

int show_one_available_quest(Character *ch, struct quest_info *quest, int count)
{
   char buffer[MAX_STRING_LENGTH];
   // Create a format string based on a space offset that takes color codes into account
   snprintf(buffer, MAX_STRING_LENGTH,
            "$B$7%3d. $2Name:$7 %%-%ds$R Cost: %%-4d%%1s Reward: %%-4d\n\r",
            quest->number, 35 + (strlen(quest->name) - nocolor_strlen(quest->name)));

   csendf(ch, buffer, quest->name, quest->cost, quest->brownie ? "$5*$R" : "", quest->reward);
   return ++count;
}

void show_available_quests(Character *ch)
{
   int count = 0;
   struct quest_info *quest;

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
      send_to_char("$B$7There are currently no available quests for you, try later.$R\n\r", ch);
   else
      //       show_quest_amount(ch, count);

      show_quest_footer(ch);

   return;
}

void show_canceled_quests(Character *ch)
{
   int count = 0;
   struct quest_info *quest;

   show_quest_header(ch);

   for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
   {
      quest = *node;

      if (check_quest_cancel(ch, quest->number))
         count = show_one_complete_quest(ch, quest, count);
   }
   //   show_quest_amount(ch, count);
   show_quest_footer(ch);

   return;
}

void show_current_quests(Character *ch)
{
   int num_attempting = 0;
   struct quest_info *quest;

   show_quest_header(ch);

   for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
   {
      quest = *node;

      if (check_quest_current(ch, quest->number))
         num_attempting = show_one_quest(ch, quest, num_attempting);
   }
   show_quest_footer(ch);

   return;
}

void show_complete_quests(Character *ch)
{
   int count = 0;
   struct quest_info *quest;

   show_quest_header(ch);

   for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
   {
      quest = *node;

      if (check_quest_complete(ch, quest->number))
         count = show_one_complete_quest(ch, quest, count);
   }
   //   show_quest_amount(ch, count);
   show_quest_footer(ch);

   return;
}

int start_quest(Character *ch, struct quest_info *quest)
{
   int count = 0;
   uint16_t price;
   Object *obj, *brownie = 0;
   Character *mob;
   char buf[MAX_STRING_LENGTH];
   Character *qmaster = get_mob_vnum(QUEST_MASTER);

   if (check_quest_current(ch, quest->number))
   {
      sprintf(buf, "q%d", quest->number);
      obj = get_obj(buf);
      if (!obj)
         return 0;
   }

   if (!check_available_quest(ch, quest))
   {
      send_to_char("That quest is not available to you.\r\n", ch);
      return eFAILURE;
   }

   while (count < QUEST_MAX)
   {
      if (ch->player->quest_current[count] == -1)
         break;
      count++;
      if (count == QUEST_MAX)
      {
         send_to_char("You've got too many quests started already.\r\n", ch);
         return eEXTRA_VALUE;
      }
   }

   price = quest->cost;
   if (GET_PLATINUM(ch) < price)
   {
      csendf(ch, "You need %d platinum coins to start this quest, which you don't have!\n\r", price);
      return eEXTRA_VAL2;
   }

   if (quest->brownie)
   {
      brownie = get_obj_in_list_num(real_object(27906), ch->carrying);
      if (!brownie)
      {
         csendf(ch, "You need a brownie point to start this quest!\n\r", price);
         return eEXTRA_VAL2;
      }
   }

   int dontwannabeinthisforever = 0;

   if (!quest->number)
   { // recurring quest
      while (++dontwannabeinthisforever < 100)
      {
         mob = get_mob_vnum(number(1, 34000));
         if (mob && (GET_LEVEL(mob) < 90) && DC::getInstance()->zones.value(world[mob->in_room].zone).isNoHunt() == false && (strlen(mob->description) > 80))
            break;
      }
      quest->hint1 = str_hsh(mob->description);
   }
   else
      mob = get_mob_vnum(quest->mobnum);

   if (!mob)
   {
      send_to_char("This quest is temporarily unavailable.\r\n", ch);
      return eFAILURE;
   }

   obj = clone_object(real_object(quest->objnum));
   obj->short_description = str_hsh(quest->objshort);
   obj->description = str_hsh(quest->objlong);

   sprintf(buf, "%s %s q%d", quest->objkey, GET_NAME(ch), quest->number);
   obj->name = str_hsh(buf);

   SET_BIT(obj->obj_flags.extra_flags, ITEM_SPECIAL);
   SET_BIT(obj->obj_flags.extra_flags, ITEM_QUEST);

   obj_to_char(obj, mob);
   wear(mob, obj, obj->keywordfind());

   logf(IMMORTAL, LogChannels::LOG_QUEST, "%s started quest %d (%s) costing %d plats %d brownie(s).", GET_NAME(ch), quest->number, quest->name, quest->cost, quest->brownie);

   ch->player->quest_current[count] = quest->number;
   ch->player->quest_current_ticksleft[count] = quest->timer;
   if (quest->number)
      quest->active = true;
   count = 0;
   while (count < QUEST_CANCEL)
   {
      if (ch->player->quest_cancel[count] == quest->number)
      {
         ch->player->quest_cancel[count] = 0;
         break;
      }
      count++;
   }

   if (quest->brownie)
   {
      csendf(ch, "%s takes a brownie point from you.\r\n", GET_SHORT(qmaster));
      obj_from_char(brownie);
   }

   csendf(ch, "%s takes %d platinum from you.\r\n", GET_SHORT(qmaster), price);
   GET_PLATINUM(ch) -= price;

   return eSUCCESS;
}

int cancel_quest(Character *ch, struct quest_info *quest)
{
   int count = 0;

   if (!quest)
      return eFAILURE;
   if (!check_quest_current(ch, quest->number))
      return eFAILURE;

   while (count < QUEST_CANCEL)
   {
      if (!ch->player->quest_cancel[count])
         break;
      count++;
      if (count >= QUEST_CANCEL)
         return eEXTRA_VALUE;
   }

   logf(IMMORTAL, LogChannels::LOG_QUEST, "%s canceled quest %d (%s).", GET_NAME(ch), quest->number, quest->name);

   ch->player->quest_cancel[count] = quest->number;

   return stop_current_quest(ch, quest);
}

int complete_quest(Character *ch, struct quest_info *quest)
{
   int count = 0;
   Object *obj;
   char buf[MAX_STRING_LENGTH];

   if (!quest)
      return eEXTRA_VALUE;

   while (count < QUEST_MAX)
   {
      if (ch->player->quest_current[count] == quest->number)
         break;
      count++;
      if (count >= QUEST_MAX)
      {
         return eEXTRA_VALUE;
      }
   }
   sprintf(buf, "q%d", quest->number);
   obj = get_obj_in_list(buf, ch->carrying);

   if (!obj)
   {
      send_to_char("You do not appear to have the quest object yet.\r\n", ch);
      return eFAILURE;
   }

   obj_from_char(obj);
   ch->player->quest_points += quest->reward;
   ch->player->quest_current[count] = -1;
   ch->player->quest_current_ticksleft[count] = 0;
   if (quest->number) // quest 0 is recurring auto quest
      SETBIT(ch->player->quest_complete, quest->number);
   quest->active = false;

   logf(IMMORTAL, LogChannels::LOG_QUEST, "%s completed quest %d (%s) and won %d qpoints.", GET_NAME(ch), quest->number, quest->name, quest->reward);

   return eSUCCESS;
}

int stop_current_quest(Character *ch, struct quest_info *quest)
{
   int count = 0;
   Object *obj;
   char buf[MAX_STRING_LENGTH];

   if (!quest)
      return eFAILURE;

   while (count < QUEST_MAX)
   {
      if (ch->player->quest_current[count] == quest->number)
         break;
      count++;
      if (count >= QUEST_MAX)
      {
         return eFAILURE;
      }
   }
   ch->player->quest_current[count] = -1;
   ch->player->quest_current_ticksleft[count] = 0;
   quest->active = false;
   sprintf(buf, "q%d", quest->number);
   obj = get_obj(buf);
   if (obj)
      extract_obj(obj);

   return eSUCCESS;
}

int stop_current_quest(Character *ch, int number)
{
   if (!number)
      return eFAILURE;

   struct quest_info *quest = get_quest_struct(number);
   return stop_current_quest(ch, quest);
}

int stop_all_quests(Character *ch)
{
   int retval = 0;

   for (int i = 0; i < QUEST_MAX; i++)
   {
      retval &= stop_current_quest(ch, ch->player->quest_current[i]);
   }
   return retval;
}

void quest_update()
{
   char buf[MAX_STRING_LENGTH];
   Character *mob;
   Object *obj;
   struct quest_info *quest;

   const auto &character_list = DC::getInstance()->character_list;
   for (const auto &i : character_list)
   {
      if (!i->desc || IS_NPC(i))
         continue;

      for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
      {
         quest = *node;

         if (quest->timer)
            for (int j = 0; j < QUEST_MAX; j++)
               if (i->player->quest_current[j] == quest->number)
               {
                  if (i->player->quest_current_ticksleft[j] <= 0)
                  {
                     stop_current_quest(i, quest);

                     logf(IMMORTAL, LogChannels::LOG_QUEST, "%s ran out of time on quest %d (%s).", GET_NAME(i), quest->number, quest->name);

                     csendf(i, "Time has expired for %s.  This quest has ended.\r\n", quest->name);
                  }
                  i->player->quest_current_ticksleft[j]--;
                  break;
               }

         if (check_quest_current(i, quest->number))
         {
            sprintf(buf, "q%d", quest->number);
            obj = get_obj(buf);
            if (!obj)
            {
               if ((mob = get_mob_vnum(quest->mobnum)))
               {
                  obj = clone_object(quest->objnum);
                  obj->short_description = str_hsh(quest->objshort);
                  obj->description = str_hsh(quest->objlong);
                  sprintf(buf, "%s q%d", quest->objkey, quest->number);
                  obj->name = str_hsh(buf);
                  obj_to_char(obj, mob);
                  wear(mob, obj, obj->keywordfind());
               }
            }
         }
      }
   }
   DC::getInstance()->removeDead();
}

int quest_handler(Character *ch, Character *qmaster, int cmd, char *name)
{
   int retval = 0;
   char buf[MAX_STRING_LENGTH];
   struct quest_info *quest;

   if (cmd != 1)
   {
      quest = get_quest_struct(name);
      if (quest == 0)
      {
         csendf(ch, "That is not a valid quest name or number.\r\n");
         return eFAILURE;
      }
   }

   switch (cmd)
   {
   case 1:
      do_emote(qmaster, "looks at his notes and writes a scroll.", CMD_DEFAULT);
      sprintf(buf, "%s Here are some currently available quests.", GET_NAME(ch));
      do_psay(qmaster, buf, CMD_DEFAULT);
      show_available_quests(ch);
      break;
   case 2:
      retval = cancel_quest(ch, quest);
      if (IS_SET(retval, eSUCCESS))
      {
         sprintf(buf, "%s You may begin this quest again if you speak with me.", GET_NAME(ch));
         do_psay(qmaster, buf, CMD_DEFAULT);
      }
      else if (IS_SET(retval, eEXTRA_VALUE))
      {
         sprintf(buf, "%s You cannot cancel up any more quests without completing some of them.", GET_NAME(ch));
         do_psay(qmaster, buf, CMD_DEFAULT);
      }
      else
      {
         sprintf(buf, "%s You weren't doing this quest to begin with.", GET_NAME(ch));
         do_psay(qmaster, buf, CMD_DEFAULT);
      }
      break;
   case 3:
      retval = start_quest(ch, quest);
      if (IS_SET(retval, eSUCCESS))
      {
         if (quest->number)
         {
            sprintf(buf, "%s Excellent!  Let me write down the quest information for you.", GET_NAME(ch));
            do_psay(qmaster, buf, CMD_DEFAULT);
            do_emote(qmaster, "gives up the scroll.", CMD_DEFAULT);
            show_quest_header(ch);
            show_one_quest(ch, quest, 0);
            show_quest_footer(ch);
         }
         else
         {
            sprintf(buf, "%s I have placed a token of Phire upon a creature somewhere within the realms.", GET_NAME(ch));
            do_psay(qmaster, buf, CMD_DEFAULT);
            sprintf(buf, "%s Retrieve it for me within 12 hours for a reward!", GET_NAME(ch));
            do_psay(qmaster, buf, CMD_DEFAULT);
            show_quest_header(ch);
            show_one_quest(ch, quest, 0);
            show_quest_footer(ch);
         }
      }
      else if (IS_SET(retval, eEXTRA_VALUE))
      {
         sprintf(buf, "%s You cannot start any more quests without completing some first.", GET_NAME(ch));
         do_psay(qmaster, buf, CMD_DEFAULT);
      }
      else if (IS_SET(retval, eEXTRA_VAL2))
      {
         sprintf(buf, "%s You do not have the required funds to get the clue from me, beggar!", GET_NAME(ch));
         do_psay(qmaster, buf, CMD_DEFAULT);
      }
      else if (!retval)
      {
         sprintf(buf, "%s The quest item has left this world.  It will appear again soon.", GET_NAME(ch));
         do_psay(qmaster, buf, CMD_DEFAULT);
      }
      else
      {
         sprintf(buf, "%s Sorry, you cannot start this quest right now.", GET_NAME(ch));
         do_psay(qmaster, buf, CMD_DEFAULT);
      }
      break;
   case 4:
      retval = complete_quest(ch, quest);
      if (IS_SET(retval, eSUCCESS))
      {
         sprintf(buf, "%s This is it!  Wonderful job, I will add your reward to your current amount of points!", GET_NAME(ch));
         do_psay(qmaster, buf, CMD_DEFAULT);
         ch->save(666);
      }
      else if (IS_SET(retval, eEXTRA_VALUE))
      {
         sprintf(buf, "%s You weren't doing this quest to begin with.", GET_NAME(ch));
         do_psay(qmaster, buf, CMD_DEFAULT);
      }
      else
      {
         sprintf(buf, "%s You have not yet completed this quest.", GET_NAME(ch));
         do_say(qmaster, buf, CMD_DEFAULT);
      }
      break;
   default:
      logentry("Bug in quest_handler, how'd they get here?", IMMORTAL, LogChannels::LOG_BUG);
      return eFAILURE;
   }
   return retval;
}

// Not used currently. Use quest list or quest start <name> instead of list or buy.
int quest_master(Character *ch, Object *obj, int cmd, char *arg, Character *owner)
{
   int choice;
   char buf[MAX_STRING_LENGTH];

   if ((cmd != 59) && (cmd != 56))
      return eFAILURE;

   if (IS_AFFECTED(ch, AFF_BLIND))
      return eFAILURE;

   if (IS_NPC(ch))
      return eFAILURE;

   if (cmd == 59)
   {
      show_available_quests(ch);
      return eSUCCESS;
   }

   if (cmd == 56)
   {
      if ((choice = atoi(arg)) == 0 || choice < 0)
      {
         sprintf(buf, "%s Try a number from the list.", GET_NAME(ch));
         do_tell(owner, buf, CMD_DEFAULT);
         return eSUCCESS;
      }
      switch (atoi(arg))
      {
      case 1:
         do_say(owner, "Sure, bum.", CMD_DEFAULT);
         break;
      default:
         sprintf(buf, "%s I don't offer that service.", GET_NAME(ch));
         do_tell(owner, buf, CMD_DEFAULT);
         break;
      }
   }

   return eSUCCESS;
}

int do_quest(Character *ch, char *arg, int cmd)
{
   int retval = 0;
   char name[MAX_STRING_LENGTH];
   Character *qmaster = get_mob_vnum(QUEST_MASTER);

   half_chop(arg, arg, name);

   if (is_abbrev(arg, "current"))
      show_current_quests(ch);
   else if (is_abbrev(arg, "canceled") && !*name)
      show_canceled_quests(ch);
   else if (is_abbrev(arg, "completed"))
      show_complete_quests(ch);
   else if (is_abbrev(arg, "list"))
   {
      if (!qmaster)
         return eFAILURE;
      if (ch->in_room != qmaster->in_room)
         send_to_char("You must ask the Quest Master for available quests.\r\n", ch);
      else
         retval = quest_handler(ch, qmaster, 1, 0);
   }
   else if (is_abbrev(arg, "cancel") && *name)
   {
      if (!qmaster)
         return eFAILURE;
      if (ch->in_room != qmaster->in_room)
         send_to_char("You must let the Quest Master know of your intentions.\r\n", ch);
      else
         retval = quest_handler(ch, qmaster, 2, name);
      return retval;
   }
   else if (is_abbrev(arg, "start") && *name)
   {
      if (!qmaster)
         return eFAILURE;
      if (ch->in_room != qmaster->in_room)
         send_to_char("You may only begin quests given from the Quest Master.\r\n", ch);
      else
         retval = quest_handler(ch, qmaster, 3, name);
      return retval;
   }
   else if (is_abbrev(arg, "finish") && *name)
   {
      if (!qmaster)
         return eFAILURE;
      if (ch->in_room != qmaster->in_room)
         send_to_char("You may only finish quests in the presence of the Quest Master.\r\n", ch);
      else
         retval = quest_handler(ch, qmaster, 4, name);
      return retval;
   }
   else if (is_abbrev(arg, "reset"))
   {
      if (!qmaster)
      {
         return eFAILURE;
      }

      if (ch->in_room != qmaster->in_room)
      {
         send_to_char("You may only reset all quests in the presence of the Quest Master.\r\n", ch);
         return eFAILURE;
      }

      quest_info *quest;
      int attempting = 0;
      int completed = 0;
      int total = 0;
      for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++)
      {
         quest = *node;

         if (GET_LEVEL(ch) >= quest->level)
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
         csendf(ch, "You will need to complete at least 100 quests before you can reset.\r\n");
         return eFAILURE;
      }

      if (GET_PLATINUM(ch) < 2000)
      {
         csendf(ch, "You need 2000 platinum coins to reset all quests, which you don't have!\n\r");
         return eEXTRA_VAL2;
      }

      Object *brownie = get_obj_in_list_num(real_object(27906), ch->carrying);
      if (!brownie)
      {
         csendf(ch, "You need a brownie point to reset all quests!\n\r");
         return eFAILURE;
      }

      csendf(ch, "%s takes 2000 platinum from you.\r\n", GET_SHORT(qmaster));
      GET_PLATINUM(ch) -= 2000;
      csendf(ch, "%s takes a brownie point from you.\r\n", GET_SHORT(qmaster));
      obj_from_char(brownie);

      stop_all_quests(ch);
      for (int i = 0; i < QUEST_MAX; i++)
      {
         ch->player->quest_current[i] = -1;
         ch->player->quest_current_ticksleft[i] = 0;
      }
      memset(ch->player->quest_cancel, 0, sizeof(ch->player->quest_cancel));
      memset(ch->player->quest_complete, 0, sizeof(ch->player->quest_complete));
      send_to_char("All quests have been reset.\r\n", ch);
      return retval;
   }
   else
   {
      csendf(ch, "Usage: quest current            (lists current quests)\n\r"
                 "       quest completed          (lists completed quests)\n\r"
                 "       quest canceled           (lists canceled quests)\n\r\n\r"
                 "The following commands may only be used at the Quest Master.\r\n"
                 "       quest list               (lists available quests)\n\r"
                 "       quest cancel <name or #> (cancel the current quest)\n\r"
                 "       quest start <name or #>  (starts a new quest)\n\r"
                 "       quest finish <name or #> (finishes a current quest)\n\r"
                 "       quest reset              (reset all quests. costs 2k plats, 1 brownie)\n\r"
                 "\n\r");
      return eFAILURE;
   }

   return eSUCCESS;
}

int do_qedit(Character *ch, char *argument, int cmd)
{
   char arg[MAX_STRING_LENGTH];
   char field[MAX_STRING_LENGTH];
   char value[MAX_STRING_LENGTH];
   int holdernum;
   int i, lownum, highnum;
   struct quest_info *quest = nullptr;
   Character *vict = nullptr;

   half_chop(argument, arg, argument);
   for (; *argument == ' '; argument++)
      ;

   if (!*arg)
   {
      csendf(ch, "Usage: qedit list                      (list all quest names and numbers)\n\r"
                 "       qedit list <lownum> <highnum>   (lists names and numbers between)\n\r"
                 "       qedit show <number>             (show detailed information)\n\r"
                 "       qedit <number> <field> <value>  (edit a quest)\n\r"
                 "       qedit new <name>                (add a quest)\n\r"
                 "       qedit save                      (saves all quests)\n\r"
                 "       qedit stat <playername>         (show player's current qpoints)\n\r"
                 "       qedit set <playername> <value>  (alter player's current qpoints)\n\r"
                 "       qedut reset <playername>\r\n"
                 "\n\r"
                 "Valid qedit fields:\n\r");

      // Display all of qedit's valid fields in rows of 4 columns
      //
      char **tmp = valid_fields;
      int i = 0;
      while (*tmp != nullptr)
      {
         csendf(ch, "%s\t", *tmp);
         if (++i % 4 == 0)
         {
            csendf(ch, "\n\r");
         }
         tmp++;
      }
      csendf(ch, "\n\r");

      return eFAILURE;
   }

   if (is_abbrev(arg, "save"))
   {
      send_to_char("Quests saved.\r\n", ch);
      return save_quests();
   }

   if (is_abbrev(arg, "new"))
   {
      if (!*argument)
      {
         send_to_char("Usage: qedit new <name>\n\r", ch);
         return eFAILURE;
      }
      else
      {
         quest = get_quest_struct(argument);
         if (quest)
         {
            send_to_char("A quest by this name already exists.\r\n", ch);
            return eFAILURE;
         }
         else
         {
            do_add_quest(ch, argument);
            return eSUCCESS;
         }
      }
   }

   half_chop(argument, field, argument);
   for (; *argument == ' '; argument++)
      ;

   if (*arg && is_number(arg) && !*field)
   {
      show_quest_info(ch, atoi(arg));
      return eSUCCESS;
   }

   if (is_abbrev(arg, "stat"))
   {
      if (!*field)
      {
         send_to_char("Usage: qedit stat <playername>\n\r", ch);
         return eFAILURE;
      }
      else
      {
         if (!(vict = get_char_vis(ch, field)) || IS_MOB(vict))
         {
            send_to_char("No living thing by that name.\r\n", ch);
            return eFAILURE;
         }

         csendf(ch, "%s's quest points: %d\n\r", GET_NAME(vict), vict->player->quest_points);
      }
      return eSUCCESS;
   }

   if (is_abbrev(arg, "reset"))
   {
      if (!*field)
      {
         send_to_char("Usage: qedit reset <playername>\n\r", ch);
         return eFAILURE;
      }
      else
      {
         if (!(vict = get_char_vis(ch, field)) || IS_MOB(vict))
         {
            send_to_char("No living thing by that name.\r\n", ch);
            return eFAILURE;
         }

         memset(vict->player->quest_cancel, 0, sizeof(vict->player->quest_cancel));
         memset(vict->player->quest_complete, 0, sizeof(vict->player->quest_complete));

         ch->send(QString("Reset quests for player %1\r\n").arg(GET_NAME(vict)));
         vict->save(666);
         return eSUCCESS;
      }
   }

   if (is_abbrev(arg, "set"))
   {
      half_chop(argument, value, argument);
      for (; *argument == ' '; argument++)
         ;

      if (!*field || !*value || !is_number(value))
      {
         send_to_char("Usage: qedit set <playername> <value>\n\r", ch);
         return eFAILURE;
      }
      else
      {
         if (!(vict = get_char_vis(ch, field)) || IS_MOB(vict))
         {
            send_to_char("No living thing by that name.\r\n", ch);
            return eFAILURE;
         }

         logf(IMMORTAL, LogChannels::LOG_QUEST, "%s set %s's quest points from %d to %d.", GET_NAME(ch), GET_NAME(vict),
              vict->player->quest_points, atoi(value));
         csendf(ch, "Setting %s's quest points from %d to %d.\r\n", GET_NAME(vict),
                vict->player->quest_points, atoi(value));

         vict->player->quest_points = atoi(value);
      }
      return eSUCCESS;
   }

   if (is_abbrev(arg, "show"))
   {
      if (!*field || !is_number(field))
         send_to_char("Usage: qedit show <number>\n\r", ch);
      else
         show_quest_info(ch, atoi(field));
      return eSUCCESS;
   }

   half_chop(argument, value, argument);
   for (; *argument == ' '; argument++)
      ;

   if (is_abbrev(arg, "list") && !*field)
   {
      list_quests(ch, 0, QUEST_TOTAL);
      return eSUCCESS;
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
      return eSUCCESS;
   }

   if (!is_number(arg))
   {
      send_to_char("Usage: qedit <number> <field> <value>\n\r", ch);
      return eFAILURE;
   }

   holdernum = atoi(arg);

   if (holdernum <= 0 || holdernum > QUEST_TOTAL)
   {
      send_to_char("Invalid quest number.\r\n", ch);
      return eFAILURE;
   }

   if (!(quest = get_quest_struct(holdernum)))
   {
      send_to_char("That quest doesn't exist.\r\n", ch);
      return eFAILURE;
   }

   if (!*field)
   {
      send_to_char("Valid fields: name, level, cost, brownie, objnum, objshort, objlong, objkey, mobnum, timer, reward or hints.\r\n", ch);
      return eFAILURE;
   }

   if (!(*value))
   {
      send_to_char("You must enter a value.\r\n", ch);
      return eFAILURE;
   }

   i = 0;
   while (valid_fields[i] != nullptr)
   {
      if (is_abbrev(field, valid_fields[i]))
         break;
      else
         i++;
   }

   if (valid_fields[i] == nullptr)
   {
      send_to_char("Valid fields: name, level, cost, brownie, objnum, objshort, objlong, objkey, mobnum, timer, reward, hint1, hint2, or hint3.\r\n", ch);
      return eFAILURE;
   }

   struct quest_info *oldquest;

   switch (i)
   {
   case 0: // name
      sprintf(field, "%s %s", value, argument);
      oldquest = get_quest_struct(field);
      if (oldquest)
      {
         send_to_char("A quest by this name already exists.\r\n", ch);
         return eFAILURE;
      }
      else
      {
         csendf(ch, "Name changed from %s ", quest->name);
         quest->name = str_hsh(field);
         csendf(ch, "to %s.\r\n", quest->name);
      }
      break;
   case 1: // level
      csendf(ch, "Level changed from %d ", quest->level);
      quest->level = atoi(value);
      csendf(ch, "to %d.\r\n", quest->level);
      break;
   case 2: // objnum
      csendf(ch, "Objnum changed from %d ", quest->objnum);
      quest->objnum = atoi(value);
      csendf(ch, "to %d.\r\n", quest->objnum);
      break;
   case 3: // objshort
      csendf(ch, "Objshort changed from %s ", quest->objshort);
      sprintf(field, "%s %s", value, argument);
      quest->objshort = str_hsh(field);
      csendf(ch, "to %s.\r\n", quest->objshort);
      break;
   case 4: // objlong
      csendf(ch, "Objlong changed from %s ", quest->objlong);
      sprintf(field, "%s %s", value, argument);
      quest->objlong = str_hsh(field);
      csendf(ch, "to %s.\r\n", quest->objlong);
      break;
   case 5: // objkey
      csendf(ch, "Objkey changed from %s ", quest->objkey);
      sprintf(field, "%s %s", value, argument);
      quest->objkey = str_hsh(field);
      csendf(ch, "to %s.\r\n", quest->objkey);
      break;
   case 6: // mobnum
      csendf(ch, "Mobnum changed from %d ", quest->mobnum);
      quest->mobnum = atoi(value);
      csendf(ch, "to %d.\r\n", quest->mobnum);
      break;
   case 7: // timer
      csendf(ch, "Timer changed from %d ", quest->timer);
      quest->timer = atoi(value);
      csendf(ch, "to %d.\r\n", quest->timer);
      break;
   case 8: // reward
      csendf(ch, "Reward changed from %d ", quest->reward);
      quest->reward = atoi(value);
      csendf(ch, "to %d.\r\n", quest->reward);
      break;
   case 9: // hint1
      sprintf(field, "%s %s", value, argument);
      csendf(ch, "Hint #1 changed from %s ", quest->hint1);
      quest->hint1 = str_hsh(field);
      csendf(ch, "to %s.\r\n", quest->hint1);
      break;
   case 10: // hint2
      sprintf(field, "%s %s", value, argument);
      csendf(ch, "Hint #2 changed from %s ", quest->hint2);
      quest->hint2 = str_hsh(field);
      csendf(ch, "to %s.\r\n", quest->hint2);
      break;
   case 11: // hint3
      sprintf(field, "%s %s", value, argument);
      csendf(ch, "Hint #3 changed from %s ", quest->hint3);
      quest->hint3 = str_hsh(field);
      csendf(ch, "to %s.\r\n", quest->hint3);
      break;
   case 12: // cost
      csendf(ch, "Cost changed from %d ", quest->cost);
      quest->cost = atoi(value);
      csendf(ch, "to %d.\r\n", quest->cost);
      break;
   case 13: // brownie
      if (quest->brownie)
      {
         csendf(ch, "Brownie toggled to NOT required.\r\n");
         quest->brownie = 0;
      }
      else
      {
         csendf(ch, "Brownie toggled to required.\r\n");
         quest->brownie = 1;
      }
      break;
   default:
      logentry("Screw up in do_edit_quest, whatsamaddahyou?", IMMORTAL, LogChannels::LOG_BUG);
      return eFAILURE;
   }
   return eSUCCESS;
}

int quest_vendor(Character *ch, Object *obj, int cmd, const char *arg, Character *owner)
{
   char buf[MAX_STRING_LENGTH];
   int rnum = 0;

   // list & buy & sell
   if ((cmd != CMD_LIST) && (cmd != CMD_BUY) && (cmd != CMD_SELL))
   {
      return eFAILURE;
   }

   if (!CAN_SEE(ch, owner))
   {
      return eFAILURE;
   }

   if (IS_MOB(ch))
   {
      return eFAILURE;
   }

   if (!CAN_SEE(owner, ch))
   {
      do_say(owner, "I don't trade with people I can't see!", 0);
      return eSUCCESS;
   }

   if (cmd == CMD_LIST)
   { /* List */
      send_to_char("$B$2Orro tells you, 'This is what I can do for you...$R \n\r", ch);
      send_to_char("$BQuest Equipment:$R\r\n", ch);

      int n = 0;
      for (int qvnum = 27975; qvnum < 28000; qvnum++)
      {
         rnum = real_object(qvnum);
         if (rnum >= 0)
         {
            char *buffer = gl_item((Object *)obj_index[rnum].item, n++, ch, false);
            send_to_char(buffer, ch);
            dc_free(buffer);
         }
      }
      for (int qvnum = 27943; qvnum <= 27953; qvnum++)
      {
         rnum = real_object(qvnum);
         if (rnum >= 0)
         {
            char *buffer = gl_item((Object *)obj_index[rnum].item, n++, ch, false);
            send_to_char(buffer, ch);
            dc_free(buffer);
         }
      }
      for (int qvnum = 3124; qvnum <= 3127; qvnum++)
      {
         rnum = real_object(qvnum);
         if (rnum >= 0)
         {
            char *buffer = gl_item((Object *)obj_index[rnum].item, n++, ch, false);
            send_to_char(buffer, ch);
            dc_free(buffer);
         }
      }
   }
   else if (cmd == CMD_BUY)
   { /* buy */
      char arg2[MAX_INPUT_LENGTH];
      one_argument(arg, arg2);

      if (!is_number(arg2))
      {
         sprintf(buf, "%s Sorry, mate. You type buy <number> to specify what you want..", GET_NAME(ch));
         do_tell(owner, buf, 0);
         return eSUCCESS;
      }

      bool FOUND = false;
      int want_num = atoi(arg2) - 1;
      int n = 0;
      for (int qvnum = 27975; qvnum <= 27999; qvnum++)
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
         for (int qvnum = 27943; qvnum <= 27953; qvnum++)
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
         for (int qvnum = 3124; qvnum <= 3127; qvnum++)
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
         sprintf(buf, "%s Don't have that I'm afraid. Type \"list\" to see my wares.", GET_NAME(ch));
         do_tell(owner, buf, 0);
         return eSUCCESS;
      }

      class Object *obj;
      obj = clone_object(rnum);

      /*      if (class_restricted(ch, obj)) {
           sprintf(buf, "%s That item is meant for another class.", GET_NAME(ch));
           do_tell(owner, buf, 0);
           extract_obj(obj);
           return eSUCCESS;
            } else if (size_restricted(ch, obj)) {
           sprintf(buf, "%s That item would not fit you.", GET_NAME(ch));
           do_tell(owner, buf, 0);
           extract_obj(obj);
           return eSUCCESS;
            } else */
      if (IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE) &&
          search_char_for_item(ch, obj->item_number, false))
      {
         sprintf(buf, "%s You already have one of those.", GET_NAME(ch));
         do_tell(owner, buf, 0);
         extract_obj(obj);
         return eSUCCESS;
      }

      if (GET_QPOINTS(ch) < (unsigned int)(obj->obj_flags.cost / 10000))
      {
         sprintf(buf, "%s Come back when you've got the qpoints.", GET_NAME(ch));
         do_tell(owner, buf, 0);
         extract_obj(obj);
         return eSUCCESS;
      }

      GET_QPOINTS(ch) -= (obj->obj_flags.cost / 10000);

      SET_BIT(obj->obj_flags.more_flags, ITEM_24H_NO_SELL);
      SET_BIT(obj->obj_flags.more_flags, ITEM_CUSTOM);
      obj->no_sell_expiration = time(nullptr) + (60 * 60 * 24);

      obj_to_char(obj, ch);
      sprintf(buf, "%s Here's your %s$B$2. Have a nice time with it.", GET_NAME(ch), obj->short_description);
      do_tell(owner, buf, 0);
      return eSUCCESS;
   }
   else if (cmd == CMD_SELL)
   { /* Sell */
      char arg2[MAX_INPUT_LENGTH];
      one_argument(arg, arg2);

      Object *obj = get_obj_in_list_vis(ch, arg2, ch->carrying);
      if (!obj)
      {
         sprintf(buf, "%s Try that on the kooky meta-physician..", GET_NAME(ch));
         do_tell(owner, buf, 0);
         return eSUCCESS;
      }

      if (!isname("quest", ((Object *)(obj_index[obj->item_number].item))->name) &&
          obj_index[obj->item_number].virt != 3124 &&
          obj_index[obj->item_number].virt != 3125 &&
          obj_index[obj->item_number].virt != 3126 &&
          obj_index[obj->item_number].virt != 3127)
      {
         sprintf(buf, "%s I only buy quest equipment.", GET_NAME(ch));
         do_tell(owner, buf, 0);
         return eSUCCESS;
      }

      if (IS_SET(obj->obj_flags.more_flags, ITEM_24H_NO_SELL))
      {
         time_t now = time(nullptr);
         time_t expires = obj->no_sell_expiration;
         if (now < expires)
         {
            sprintf(buf, "%s I won't buy that for another %u seconds.", GET_NAME(ch), expires - now);
            do_tell(owner, buf, 0);
            return eSUCCESS;
         }
      }

      int cost = obj->obj_flags.cost / 10000.0;

      sprintf(buf, "%s I'll give you %d qpoints for that. Thanks for shoppin'.", GET_NAME(ch), cost);
      do_tell(owner, buf, 0);
      extract_obj(obj);
      GET_QPOINTS(ch) += cost;
      return eSUCCESS;
   }

   return eSUCCESS;
}
