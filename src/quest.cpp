/*****************************************************
one liner quest shit
*****************************************************/

#include <math.h>
#include <structs.h>
#include <character.h>
#include <comm.h>
#include <fileinfo.h>
#include <returnvals.h>
#include <obj.h>
#include <timeinfo.h>
#include <act.h>
#include <levels.h>
#include <interp.h>
#include <handler.h>
#include <db.h>
#include <connect.h>
#include <quest.h>

//figure out something to do if the object gets destroyed quest restart name?
//make sure somebody else can't get the quest object
//put a quest object looker-uper to make sure the object is still there for the quest to continue?
//quests get reversed every time there's a read and write of the quests?


extern struct quest_info *quest_list;
extern void string_to_file(FILE *, char *);
extern int keywordfind(OBJ_DATA *);
extern void wear(CHAR_DATA *, OBJ_DATA *, int);
extern CHAR_DATA *character_list;

int load_quests()
{
   FILE *fl;
   struct quest_info *quest = NULL;

   if(!(fl = dc_fopen(QUEST_FILE, "r"))) {
      log("Failed to open quest file for reading!", 0, LOG_MISC);
      return eFAILURE;
   }

   while(!EOF) {
      quest->number = fread_int(fl, 0, INT_MAX);
      quest->name   = fread_string(fl, 1);
      quest->hint1  = fread_string(fl, 1);
      quest->hint2  = fread_string(fl, 1);
      quest->hint3  = fread_string(fl, 1);
      quest->objshort = fread_string(fl, 1);
      quest->objlong = fread_string(fl, 1);
      quest->objkey = fread_string(fl, 1);
      quest->level  = fread_int(fl, 0, INT_MAX);
      quest->objnum = fread_int(fl, 0, INT_MAX);
      quest->mobnum = fread_int(fl, 0, INT_MAX);
      quest->timer  = fread_int(fl, 0, INT_MAX);
      quest->reward = fread_int(fl, 0, INT_MAX);
      quest->next = quest_list;
      quest_list = quest;
   }

   dc_fclose(fl);

   return eSUCCESS;
}

int save_quests()
{
   FILE *fl;
   struct quest_info *quest = quest_list;

   if(!(fl = dc_fopen(QUEST_FILE, "w"))) {
      log("Failed to open quest file for writing!", 0, LOG_MISC);
      return eFAILURE;
   }

   while(quest) {
      fprintf(fl, "%d\n", quest->number);
      string_to_file(fl, quest->name);
      string_to_file(fl, quest->hint1);
      string_to_file(fl, quest->hint2);
      string_to_file(fl, quest->hint3);
      string_to_file(fl, quest->objshort);
      string_to_file(fl, quest->objlong);
      string_to_file(fl, quest->objkey);
      fprintf(fl, "%d %d %d %d %d\n", quest->level, quest->objnum, quest->mobnum, quest->timer, quest->reward);
      quest = quest->next;
   }

   dc_fclose(fl);

   return eSUCCESS;
}

struct quest_info * get_quest_struct(int num)
{
   struct quest_info *quest = quest_list;

   while(quest) {
      if(quest->number == num)
         return quest;
      quest = quest->next;
   }

   return 0;
}

struct quest_info * get_quest_struct(char *name)
{
   struct quest_info *quest = quest_list;

   while(quest) {
      if(!strcmp(name, quest->name))
         return quest;
      quest = quest->next;
   }

   return 0;
}

int do_add_quest(CHAR_DATA *ch, char *name)
{
   struct quest_info *quest = NULL; //new quest

   quest->name = str_hsh(name);
   quest->hint1 = str_hsh(" ");
   quest->hint2 = str_hsh(0);
   quest->hint3 = str_hsh(0);
   quest->objshort = str_hsh("a Quest Item");
   quest->objlong = str_hsh("A quest item lies here.");
   quest->objkey = str_hsh("quest item");
   quest->level = 75;
   quest->objnum = 51;
   quest->mobnum = QUEST_MASTER;
   quest->timer = 0;
   quest->reward = 0;

   quest->number = quest_list->number + 1;
   quest->next = quest_list;
   quest_list = quest;

   csendf(ch, "Quest number %d added.\n\r", quest->number);
   show_quest_info(ch, quest->number);

   return eSUCCESS;
}

void list_quests(CHAR_DATA *ch, int lownum, int highnum)
{
   struct quest_info * quest = quest_list;

   while(quest) {
      if(quest->number <= highnum && quest->number >= lownum)
         csendf(ch, "Quest Name: %-35s  Number: %d\n\r", quest->name, quest->number);
      quest = quest->next;
   }
   return;
}

void show_quest_info(CHAR_DATA *ch, int num)
{
   struct quest_info * quest = quest_list;

   while(quest) {
      if(quest->number == num) {
         csendf(ch, "Quest Name: %-35s   Level: %d\n\r"
                    "Hint: %s\n\r%s\n\r%s"
                    "Objnum: %d   Quest Mobnum: %d\n\r"
                    "Objshort: %s\n\r"
                    "Objlong: %s\n\r"
                    "Keywords: %s\n\r"
                    "Timer: %d    Reward: %d\n\r",
                     quest->name, quest->level, quest->hint1, quest->hint2, quest->hint3,
                     quest->objnum, quest->mobnum, quest->objshort, quest->objlong,
                     quest->objkey, quest->timer, quest->reward);
         break;
      }
      quest = quest->next;
   }
         
}

int check_quest_timer(CHAR_DATA *ch, struct quest_info *quest)
{
   int i;
   int retval = -1;
   int timeonquest;

   if(quest->timer) {
      for(i=0;i<QUEST_MAX;i++)
         if(ch->pcdata->quest_current[i] == quest->number)
            timeonquest = time(0) - ch->pcdata->quest_current_timestarted[i];
      if(timeonquest > quest->timer) retval = 0;
      else retval = timeonquest;
   }

   return retval;
}

bool check_available_quest(CHAR_DATA *ch, struct quest_info *quest)
{
   if(!quest) return FALSE;

   if(GET_LEVEL(ch) >= quest->level && check_quest_current(ch, quest->number)
         && check_quest_pass(ch, quest->number) && check_quest_complete(ch, quest->number)
         && !(quest->active) )
      return TRUE;

   return FALSE;
}

bool check_quest_current(CHAR_DATA *ch, int number)
{
   for(int i = 0;i<QUEST_MAX;i++)
      if(ch->pcdata->quest_current[i] == number)
         return TRUE;
   return FALSE;
}

bool check_quest_pass(CHAR_DATA *ch, int number)
{
   for(int i = 0;i<QUEST_PASS;i++)
      if(ch->pcdata->quest_pass[i] == number)
         return TRUE;
   return FALSE;
}

bool check_quest_complete(CHAR_DATA *ch, int number)
{
   if(IS_SET(ch->pcdata->quest_complete[number/QSIZE], 1 >> (number % QSIZE)))
         return TRUE;
   return FALSE;
}

int get_quest_price(struct quest_info *quest)
{
   return MIN(500, (int) (3.76 * pow(2.71828, quest->level * 0.0976) + 1));
}

void show_quest_header(CHAR_DATA *ch)
{
   csendf(ch,"  .-------------------------------------------------------------------------.\n\r"
             " /  .-.                                                                  .-.  \\\n\r"
             "[  /   \\                                                               /   \\  ]\n\r"
             "[ !\\_.  !                  Dark Castle Quest System                   !    /! ]\n\r"
             "[\\!  ! /]                                                             [\\  ! !/]\n\r"
             "[ `---' ]                                                             [ `---' ]\n\r"
          );
   return;
}

void show_quest_amount(CHAR_DATA *ch, int count)
{
   csendf(ch,"[       ]                                                             [       ]\n\r"
             "[       ] Total Quests Shown: %-4d                                    [       ]\n\r", count
          );
   return;
}

void show_quest_closer(CHAR_DATA *ch)
{
   csendf(ch,"[       ],-----------------------------------------------------------,[       ]\n\r"
             "\\       ]                                                             [       /\n\r"
             " \\     /                                                               \\     /\n\r"
             "  `---'                                                                 `---'\n\r"
             "\n\r"
         );
   return;
}

int show_one_quest(CHAR_DATA *ch, struct quest_info *quest, int count)
{
   int i, amount = 0;

   csendf(ch,"[       ] Name: %-35s                   [       ]\n\r"
             "[       ] Hint: %-52s  [       ]\n\r",
             quest->name, quest->hint1);
   if(quest->hint2)
      csendf(ch,"[       ]       %-52s  [       ]\n\r", quest->hint2);
   if(quest->hint3)
      csendf(ch,"[       ]       %-52s  [       ]\n\r", quest->hint3);
   if(quest->timer) {
      for(i = 0;i < QUEST_MAX;i++)
         if(quest->number == ch->pcdata->quest_current[i])
            amount = ch->pcdata->quest_current_timestarted[i];
      if(!amount)
         log("Somebody passed a quest into here that they don't really have.", IMMORTAL, LOG_BUG);
      csendf(ch,"[       ] Level: %d  Time remaining: %-7ld  Reward: %-5d           [       ]\n\r",
                quest->level, quest->timer - time(0) + amount, quest->reward);
   }
   else csendf(ch,"[       ] Level: %d  Reward: %-5d                                    [       ]\n\r",
                  quest->level, quest->reward);

   return ++count;
}

int show_one_complete_quest(CHAR_DATA *ch, struct quest_info *quest, int count)
{
   csendf(ch,"[       ] Name: %-35s Reward: %-5d       [       ]\n\r", quest->name, quest->reward);
   return ++count;
}

int show_one_available_quest(CHAR_DATA *ch, struct quest_info *quest, int count)
{
   csendf(ch,"[       ] Name: %-35s                   [       ]\n\r"
             "[       ] Price: %-3d          Reward: %-5d                           [       ]\n\r",
             quest->name, get_quest_price(quest), quest->reward);
   return ++count;
}

void show_available_quests(CHAR_DATA *ch)
{
   int count;
   struct quest_info *quest = quest_list;

   show_quest_header(ch);
   while(quest) {
      if(check_available_quest(ch, quest)) {
         count = show_one_available_quest(ch, quest, count);
         if(count >= QUEST_SHOW) break;
      }
      quest = quest->next;
   }
   if(!count) send_to_char("[       ]                                                             [       ]\n\r"
                           "[       ]                                                             [       ]\n\r"
                           "[       ]                                                             [       ]\n\r"
                           "[       ] There are currently no available quests for you, try later. [       ]\n\r"
                           "[       ]                                                             [       ]\n\r"
                           "[       ]                                                             [       ]\n\r"
                           "[       ]                                                             [       ]\n\r", 
ch);
   else show_quest_amount(ch, count);
   show_quest_closer(ch);

   return;
}

void show_pass_quests(CHAR_DATA *ch)
{
   int count = 0;
   struct quest_info *quest = quest_list;

   show_quest_header(ch);

   while(quest) {
      if(check_quest_pass(ch, quest->number))
         count = show_one_complete_quest(ch, quest, count);
      quest = quest->next;
   }
   show_quest_amount(ch, count);
   show_quest_closer(ch);

   return;
}

void show_current_quests(CHAR_DATA *ch)
{
   int count = 0;
   struct quest_info *quest = quest_list;

   show_quest_header(ch);

   while(quest) {
      if(check_quest_current(ch, quest->number))
         count = show_one_quest(ch, quest, count);
      quest = quest->next;
   }
   show_quest_amount(ch, count);
   show_quest_closer(ch);

   return;
}

void show_complete_quests(CHAR_DATA *ch)
{
   int count = 0;
   struct quest_info *quest = quest_list;

   show_quest_header(ch);
   while(quest) {
      if(check_quest_complete(ch, quest->number))
         count = show_one_complete_quest(ch, quest, count);
      quest = quest->next;
   }
   show_quest_amount(ch, count);
   show_quest_closer(ch);

   return;
}

int start_quest(CHAR_DATA *ch, struct quest_info *quest)
{
   int count = 0;
   uint16 price;
   OBJ_DATA *obj;
   CHAR_DATA *mob;
   char *buf;

   if(!check_available_quest(ch, quest)) return eFAILURE;

   while(count < QUEST_MAX) {
      if(!ch->pcdata->quest_current[count])
         break;
      count++;
      if(count == QUEST_MAX) return eEXTRA_VALUE;
   }

   price = get_quest_price(quest);
   if(GET_PLATINUM(ch) < price) return eEXTRA_VAL2;

   if((mob = get_mob_vnum(quest->mobnum))) {
      obj = clone_object(quest->objnum);
      obj->short_description = str_hsh(quest->objshort);
      obj->description = str_hsh(quest->objlong);
      obj->name = str_hsh(quest->objkey);
      sprintf(buf, "q%d", quest->number);
      obj->name = str_dup(buf);
      dc_free(buf);
      obj_to_char(obj, mob);
      wear(mob, obj, keywordfind(obj));
   } else return eFAILURE;

   ch->pcdata->quest_current[count] = quest->number;     
   ch->pcdata->quest_current_timestarted[count] = time(0);
   quest->active = TRUE;
   count = 0;
   while(count < QUEST_PASS) {
      if(ch->pcdata->quest_pass[count] == quest->number) {
         ch->pcdata->quest_pass[count] = 0;
         break;
      }
      count++;
   }

   GET_PLATINUM(ch) -= price;

   return eSUCCESS;
}

int pass_quest(CHAR_DATA *ch, struct quest_info *quest)
{
   int count = 0;

   if(!quest) return eFAILURE;

   while(count < QUEST_PASS) {
      if(!ch->pcdata->quest_pass[count])
         break;
      count++;
      if(count >= QUEST_PASS) return eEXTRA_VALUE;
   }
   ch->pcdata->quest_pass[count] = quest->number;

   return stop_current_quest(ch, quest);
}

int complete_quest(CHAR_DATA *ch, struct quest_info *quest)
{
   int count = 0;
   OBJ_DATA *obj;
   char *buf;

   if(!quest) return eFAILURE;

   while(count < QUEST_MAX) {
      if(ch->pcdata->quest_current[count] == quest->number)
         break;
      count++;
      if(count >= QUEST_MAX) {
         log("Somebody ending a quest they weren't on?", IMMORTAL, LOG_BUG);
         return eFAILURE;
      }
   }
   sprintf(buf, "q%d", quest->number);
   obj = get_obj_in_list(buf, ch->carrying);
   dc_free(buf);
   if(!obj) return eFAILURE;
   obj_from_char(obj);
   ch->pcdata->quest_points += quest->reward;
   ch->pcdata->quest_current[count] = 0;
   ch->pcdata->quest_current_timestarted[count] = 0;
   SET_BIT(ch->pcdata->quest_complete[quest->number/QSIZE], 1 >> (quest->number % QSIZE));
   quest->active = FALSE;

   return eSUCCESS;
}

int stop_current_quest(CHAR_DATA *ch, struct quest_info *quest)
{
   int count;
   OBJ_DATA *obj;
   char *buf;

   if (!quest) return eFAILURE;

   while(count < QUEST_MAX) {
      if(ch->pcdata->quest_current[count] == quest->number)
         break;
      count++;
      if(count >= QUEST_MAX) {
         log("Somebody stopping a quest they aren't on?", IMMORTAL, LOG_BUG);
         return eFAILURE;
      }
   }
   ch->pcdata->quest_current[count] = 0;
   ch->pcdata->quest_current_timestarted[count] = 0;
   quest->active = FALSE;
   sprintf(buf, "q%d", quest->number);
   obj = get_obj(buf);
   dc_free(buf);
   if(obj) extract_obj(obj);

   return eSUCCESS;
}

int stop_current_quest(CHAR_DATA *ch, int number)
{
   if (!number) return eFAILURE;

   struct quest_info *quest = get_quest_struct(number);
   return stop_current_quest(ch, quest);
}

int stop_all_quests(CHAR_DATA *ch)
{
   int retval;

   for(int i = 0;i < QUEST_MAX; i++)
      retval &= stop_current_quest(ch, ch->pcdata->quest_current[i]);
   return retval;
}

void quest_update()
{
   CHAR_DATA *i, *next_dude;
   struct quest_info *quest, *next_quest;
   int retval;

   for(i = character_list; i; i = next_dude) {
      next_dude = i->next;
      for(quest = quest_list; quest; quest = next_quest) {
         next_quest = quest->next;
         retval = check_quest_timer(i, quest);
         if(!retval) retval = stop_current_quest(i, quest);
      }
   }
}

int quest_handler(CHAR_DATA *ch, CHAR_DATA *qmaster, int cmd, char *name)
{
   int retval;
   struct quest_info *quest = get_quest_struct(name);

   switch(cmd) {
      case 1:
         do_emote(qmaster, "looks at his notes and hands you a scroll.", 9);
         do_say(qmaster, "Here are some currently available quests.", 9);
         show_available_quests(ch);
         break;
      case 2:
         retval = pass_quest(ch, quest);
         if(IS_SET(retval, eSUCCESS))
            do_say(qmaster, "You may begin this quest again if you speak with me.", 9);
         else if(IS_SET(retval, eEXTRA_VALUE))
            do_say(qmaster, "You cannot pass up any more quests without completing some of them.", 9);
         else
            do_say(qmaster, "You weren't doing this quest to begin with.", 9);
         break;
      case 3:
         retval = start_quest(ch, quest);
         if(IS_SET(retval, eSUCCESS))
            do_say(qmaster, "Excellent!  Let me write down the quest information for you.", 9);
         else if(IS_SET(retval, eEXTRA_VALUE))
            do_say(qmaster, "You cannot start any more quests without completing some first.", 9);
         else if(IS_SET(retval, eEXTRA_VAL2))
            do_say(qmaster, "You do not have the required funds to get the clue from me, beggar!", 9);
         else
            do_say(qmaster, "Sorry, you cannot start this quest right now.", 9);
         break;
      case 4:
         retval = complete_quest(ch, quest);
         if(IS_SET(retval, eSUCCESS))
            do_say(qmaster, "Wonderful job!  Here is your reward!", 9);
         else
            do_say(qmaster, "You have not yet completed this quest.", 9);
         break;
      default:
         log("Bug in quest_handler, how'd they get here?", IMMORTAL, LOG_BUG);
         return eFAILURE;
   }
   return retval;
}

int quest_master(CHAR_DATA *ch, OBJ_DATA *obj, int cmd, char *arg, CHAR_DATA *owner)
{


   if((cmd != 59) && (cmd != 56))
      return eFAILURE;

   if (IS_AFFECTED(ch, AFF_BLIND))
      return eFAILURE;

   if(IS_NPC(ch))
      return eFAILURE;


   if(cmd == 59) {
      send_to_char("The Quest Master tells you, 'Here is what I can do for you.'\n\r", ch);
      csendf(ch, "1) something\n\r"
                 "2) something else\n\r"
                 "3) something fun\n\r"
                 "\n\r"
             );
   }

   if(cmd == 56 && isdigit(*arg))
      switch (atoi(arg)){
         case 1:
            //stuff
         default:
            do_say(owner, "I don't offer that service.", 9);
      }
      
   return eSUCCESS;
}

int do_quest(CHAR_DATA *ch, char *arg, int cmd)
{
   int retval;
   char name[MAX_INPUT_LENGTH-1];
   char field[MAX_INPUT_LENGTH-1];
   CHAR_DATA *qmaster = get_mob_vnum(QUEST_MASTER);

   half_chop(arg, arg, name);
   half_chop(name, name, field);

   if(!*arg)
      show_current_quests(ch);
   else if(is_abbrev(arg, "passed") && !*name)
      show_pass_quests(ch);
   else if(is_abbrev(arg, "completed"))
      show_complete_quests(ch);
   else if(is_abbrev(arg, "available")) {
      if(!qmaster) return eFAILURE;
      if(ch->in_room != qmaster->in_room)
         send_to_char("You must ask the Quest Master for available quests.\n\r", ch);
      else retval = quest_handler(ch, qmaster, 1, 0);
   }
   else if(is_abbrev(arg, "passed") && *name) {
      if(!qmaster) return eFAILURE;
      if(ch->in_room != qmaster->in_room)
         send_to_char("You must let the Quest Master know of your intentions.\n\r", ch);
      else retval = quest_handler(ch, qmaster, 2, name);
      return retval;
   }
   else if(is_abbrev(arg, "start") && *name) {
      if(!qmaster) return eFAILURE;
      if(ch->in_room != qmaster->in_room)
         send_to_char("You may only begin quests given from the Quest Master.\n\r", ch);
      else retval = quest_handler(ch, qmaster, 3, name);
      return retval;
   }
   else if(is_abbrev(arg, "finish") && *name) {
      if(!qmaster) return eFAILURE;
      if(ch->in_room != qmaster->in_room)
         send_to_char("You may only finish quests in the presence of the Quest Master.\n\r", ch);
      else retval = quest_handler(ch, qmaster, 4, name);
      return retval;
   }
   else {
      csendf(ch, "Usage: quest              (lists current quests)\n\r"
                 "       quest completed    (lists completed quests)\n\r"
                 "       quest passed       (lists passed quests)\n\r"
                 "The following commands may only be used at the Quest Master.\n\r"
                 "       quest available    (lists available quests)\n\r"
                 "       quest pass <name>  (passes a current quest)\n\r"
                 "       quest start <name> (starts a new quest)\n\r"
                 "       quest finish <name>(finishes a current quest)\n\r"
                 "\n\r");
      return eFAILURE;
   }

   return eSUCCESS;
}

int do_qedit(CHAR_DATA *ch, char *arg, int cmd)
{
   char field[MAX_STRING_LENGTH];
   char value[MAX_STRING_LENGTH];
   int  holdernum;
   int i, lownum, highnum;
   struct quest_info *quest = NULL;

   half_chop(arg, arg, field);

   if(!*arg) {
      csendf(ch, "Usage: qedit list                      (list all quest names and numbers)\n\r"
                 "       qedit list <lownum> <highnum>   (lists names and numbers between)\n\r"
                 "       qedit show <number>             (show detailed information)\n\r"
                 "       qedit <number> <field> <value>  (edit a quest)\n\r"
                 "       qedit new <name>                (add a quest)\n\r"
                 "       qedit save                      (saves all quests)\n\r"
                 "\n\r");
      return eFAILURE;
   }

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
      "hints",
      "\n"
   };

   if(is_abbrev(arg, "show")) {
      if(!*field || !is_number(field)) send_to_char("Usage: qedit show <number>\n\r", ch);
      else
         show_quest_info(ch, atoi(field));
      return eSUCCESS;
   }

   if(is_abbrev(arg, "new")) {
      if(!*field || is_number(field)) {
         send_to_char("Usage: qedit new <name>\n\r", ch);
         return eFAILURE;
      }
      else {
         quest = get_quest_struct(field);
         if(quest) {
            send_to_char("A quest by this name already exists.\n\r", ch);
            return eFAILURE;
         }
         else {
            do_add_quest(ch, field);
            return eSUCCESS;
         }
      }
   }

   if(is_abbrev(arg, "save")) {
      return save_quests();
   }

   half_chop(field, field, value);

   if(is_abbrev(arg, "list") && !*field) {
      list_quests(ch, 0, QUEST_MAX);
      return eSUCCESS;
   } else if(*field && is_number(field)) {
      lownum = MAX(0,atoi(field));
      if(*value && is_number(value)) {
         highnum = MIN(QUEST_MAX, atoi(value));
         if(lownum > highnum) {
            holdernum = lownum;
            lownum = highnum;
            highnum = holdernum;
         }
         list_quests(ch, lownum, highnum);
      }
      else
         list_quests(ch, lownum, QUEST_MAX);
      return eSUCCESS;
   }

   if(!is_number(arg)) {
      send_to_char("Usage: qedit <number> <field> <value>\n\r", ch);
      return eFAILURE;
   }

   holdernum = atoi(arg);

   if(holdernum <= 0 || holdernum >= QUEST_MAX) {
      send_to_char("Invalid quest number.\n\r", ch);
      return eFAILURE;
   }

   if(!*field) {
      send_to_char("Valid fields: level, objnum, objshort, objlong, objkey, mobnum, timer, reward or hints.\n\r", ch);
      return eFAILURE;
   }

   if(!(*value) && !is_abbrev(field, "hints")) {
      send_to_char("You must enter a value.\n\r", ch);
      return eFAILURE;
   }

   for(i = 0; i <= 10; i++) {
      if (i == 10) {
         send_to_char("Valid fields: name, level, objnum, objshort, objlong, objkey, mobnum, timer, reward or hints.\n\r",ch);
         return eFAILURE;
      }
      if (is_abbrev(field, valid_fields[i]))
         break;
   }
   
   quest = get_quest_struct(holdernum);

   switch(i) {
      case 0:
         csendf(ch, "Name changed from %s ", quest->name);
         quest->name = str_hsh(value);
         csendf(ch, "to %s.\n\r", quest->name);
         break;
      case 1:
         csendf(ch, "Level changed from %d ", quest->level);
         quest->level = atoi(value);
         csendf(ch, "to %d.\n\r", quest->level);
         break;
      case 2:
         csendf(ch, "Objnum changed from %d ", quest->objnum);
         quest->objnum = atoi(value);
         csendf(ch, "to %d.\n\r", quest->objnum);
         break;
      case 3:
         csendf(ch, "Objshort changed from %s ", quest->objshort);
         quest->objshort = str_hsh(value);
         csendf(ch, "to %s.\n\r", quest->objshort);
         break;
      case 4:
         csendf(ch, "Objlong changed from %s ", quest->objlong);
         quest->objlong = str_hsh(value);
         csendf(ch, "to %s.\n\r", quest->objlong);
         break;
      case 5:
         csendf(ch, "Objkey changed from %s ", quest->objkey);
         quest->objkey = str_hsh(value);
         csendf(ch, "to %s.\n\r", quest->objkey);
         break;
      case 6:
         csendf(ch, "Mobnum changed from %d ", quest->mobnum);
         quest->mobnum = atoi(value);
         csendf(ch, "to %d.\n\r", quest->mobnum);
         break;
      case 7:
         csendf(ch, "Timer changed from %d ", quest->timer);
         quest->timer = atoi(value);
         csendf(ch, "to %d.\n\r", quest->timer);
         break;
      case 8:
         csendf(ch, "Reward changed from %d ", quest->reward);
         quest->reward = atoi(value);
         csendf(ch, "to %d.\n\r", quest->reward);
         break;
      case 9:
         send_to_char("|--Enter first line for quest between the lines ---|", ch);
         if (quest->hint1) send_to_char(quest->hint1, ch);
         ch->desc->connected = CON_EDITING;
         ch->desc->strnew = &(quest->hint1);
         ch->desc->max_str = 52;
         send_to_char("|--Enter second line for quest hint between lines--|", ch);
         if (quest->hint2) send_to_char(quest->hint2, ch);
         ch->desc->connected = CON_EDITING;
         ch->desc->strnew = &(quest->hint2);
         ch->desc->max_str = 52;
         send_to_char("|--Enter third line for quest hint between lines --|", ch);
         if (quest->hint3) send_to_char(quest->hint3, ch);
         ch->desc->connected = CON_EDITING;
         ch->desc->strnew = &(quest->hint3);
         ch->desc->max_str = 52;
         break;
      default:
         log("Screw up in do_edit_quest, whatsamaddahyou?", IMMORTAL, LOG_BUG);
         return eFAILURE;
   }
   return eSUCCESS;
}
