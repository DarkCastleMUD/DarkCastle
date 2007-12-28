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
#include <act.h>
#include <levels.h>
#include <interp.h>
#include <handler.h>
#include <db.h>
#include <connect.h>
#include <quest.h>
#include <vector>

using namespace std;

typedef vector<quest_info *> quest_list_t;
quest_list_t quest_list;

extern void string_to_file(FILE *, char *);
extern int keywordfind(OBJ_DATA *);
extern void wear(CHAR_DATA *, OBJ_DATA *, int);
extern CHAR_DATA *character_list;
extern struct index_data *mob_index;

int load_quests(void)
{
   FILE *fl;
   quest_info *quest;

   if(!(fl = dc_fopen(QUEST_FILE, "r"))) {
      log("Failed to open quest file for reading!", 0, LOG_MISC);
      return eFAILURE;
   }

   while(fgetc(fl) != '$') {

#ifdef LEAK_CHECK
  quest = (struct quest_info *)calloc(1, sizeof(struct quest_info));
#else
  quest = (struct quest_info *)dc_alloc(1, sizeof(struct quest_info));
#endif

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
      quest->cost   = fread_int(fl, 0, INT_MAX);
      quest->brownie= fread_int(fl, 0, INT_MAX);
      quest->active = FALSE;

      quest_list.push_back(quest);
   }

   dc_fclose(fl);

   return eSUCCESS;
}

int save_quests(void)
{
   FILE *fl;
   quest_info *quest;

   if(!(fl = dc_fopen(QUEST_FILE, "w"))) {
      log("Failed to open quest file for writing!", 0, LOG_MISC);
      return eFAILURE;
   }

   for(quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++) {
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

   dc_fclose(fl);

   return eSUCCESS;
}

struct quest_info * get_quest_struct(int num)
{
    quest_info *quest;
    if (!num)
	return 0;
    
    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++) {
	quest = *node;
	
	if (quest->number == num)
	    return quest;
    }

    return 0;
}

struct quest_info * get_quest_struct(char *name)
{
   if (!*name)
       return 0;
   
   struct quest_info *quest;

    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++) {
	quest = *node;
	
	if(!str_nosp_cmp(name, quest->name))
	    return quest;
    }

    return 0;
}

int do_add_quest(CHAR_DATA *ch, char *name)
{
   struct quest_info * quest; //new quest

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
   quest->active = FALSE;
   quest->cost = 0;

   if (quest_list.empty() == true)
       quest->number = quest_list.back()->number + 1;
   else
       quest->number = 1;

   csendf(ch, "Quest number %d added.\n\r", quest->number);
   show_quest_info(ch, quest->number);

   return eSUCCESS;
}

void list_quests(CHAR_DATA *ch, int lownum, int highnum)
{
    struct quest_info * quest;
    
    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++) {
	quest = *node;
	
	if(quest->number <= highnum && quest->number >= lownum)
	    csendf(ch, "%2d. %s\n\r", quest->number, quest->name);
    }
    
    return;
}

void show_quest_info(CHAR_DATA *ch, int num)
{
   struct quest_info * quest;

    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++) {
	quest = *node;
	
      if(quest->number == num) {
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
                real_mobile(quest->mobnum) > 0 ? ((CHAR_DATA *)(mob_index[real_mobile(quest->mobnum)].item))->short_desc : "no current mob",
                quest->objnum, quest->objkey,
                quest->objshort, quest->objlong,quest->hint1, quest->hint2, quest->hint3);
         return;
      }
   }
   send_to_char("That quest doesn't exist.\n\r", ch);
}

bool check_available_quest(CHAR_DATA *ch, struct quest_info *quest)
{
   if(!quest) return FALSE;

   if(GET_LEVEL(ch) >= quest->level && !check_quest_current(ch, quest->number)
         && !check_quest_complete(ch, quest->number) && !(quest->active) )
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
   if(ISSET(ch->pcdata->quest_complete, number))
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
             " /.-.                                                                     .-.\\\n\r"
             "[/   \\                                                                   /   \\]\n\r"
             "[\\__. !                    $B$2Dark Castle Quest System$R                     ! .__/]\n\r"
             "[\\  ! /                                                                 \\ !  /]\n\r"
             "[ `--'                                                                   `--' ]\n\r"
	     "[-----------------------------------------------------------------------------]\n\r\n\r"
          );
   return;
}

void show_quest_amount(CHAR_DATA *ch, int remaining)
{
   csendf(ch,
	  "\n\r $B$2Completed: $7%-4d $2Remaining: $7%-4d $2Total: $7%-4d$R\n\r", quest_list.size()-remaining, remaining, quest_list.size());
   return;
}

void show_quest_footer(CHAR_DATA *ch)
{	     
   csendf(ch,"[-----------------------------------------------------------------------------]\n\r" );
   return;
}

int show_one_quest(CHAR_DATA *ch, struct quest_info *quest, int count)
{
   int i, amount = 0;

   csendf(ch," $B$2Name:$7 %-35s$R\n\r"
             " $B$2Hint:$7 %-52s$R\n\r",
             quest->name, quest->hint1);
   if(quest->hint2)
      csendf(ch," $B$7%-52s$R\n\r", quest->hint2);
   if(quest->hint3)
      csendf(ch," $B$7%-52s$R\n\r", quest->hint3);
   if(quest->timer) {
       for(i = 0;i < QUEST_MAX;i++) {
	   if (quest->number == ch->pcdata->quest_current[i]) {
	       amount = ch->pcdata->quest_current_ticksleft[i];
	   }

	   if (!amount) {
	       log("Somebody passed a quest into here that they don't really have.", IMMORTAL, LOG_BUG);
	   }

	   csendf(ch," $B$2Level:$7 %d  $2Time remaining:$7 %-7ld  $2Reward:$7 %-5d$R\n\r\n\r",
		  quest->level, amount, quest->reward);
       }
   } else {
       csendf(ch," $B$2Level:$7 %d  $2Reward:$7 %-5d$R\n\r\n\r",
	      quest->level, quest->reward);
   }
   return ++count;
}

int show_one_complete_quest(CHAR_DATA *ch, struct quest_info *quest, int count)
{
   csendf(ch," $B$2Name:$7 %-35s $2Reward:$7 %-5d$R\n\r", quest->name, quest->reward);
   return ++count;
}

int show_one_available_quest(CHAR_DATA *ch, struct quest_info *quest, int count)
{
    char buffer[MAX_STRING_LENGTH];
    // Create a format string based on a space offset that takes color codes into account
    snprintf(buffer, MAX_STRING_LENGTH,
	     " $B$2Name:$7 %%-%ds$R Cost: %%-4d%%1s Reward: %%-4d\n\r",
	     35 + (strlen(quest->name) - nocolor_strlen(quest->name)));
    
    csendf(ch, buffer, quest->name, quest->cost, quest->brownie ? "$5*$R" : "", quest->reward);
   return ++count;
}

void show_available_quests(CHAR_DATA *ch)
{
   int count = 0;
   struct quest_info *quest;

   show_quest_header(ch);

    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++) {
	quest = *node;
	
      if(check_available_quest(ch, quest)) {
         count = show_one_available_quest(ch, quest, count);
//         if(count >= QUEST_SHOW) break;
      }
   }
   if(!count)
       send_to_char("$B$7There are currently no available quests for you, try later.$R\n\r", ch);
   else
       show_quest_amount(ch, count);

   show_quest_footer(ch);

   return;
}

void show_pass_quests(CHAR_DATA *ch)
{
   int count = 0;
   struct quest_info *quest;

   show_quest_header(ch);

    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++) {
	quest = *node;
	
      if(check_quest_pass(ch, quest->number))
         count = show_one_complete_quest(ch, quest, count);
   }
   show_quest_amount(ch, count);
   show_quest_footer(ch);

   return;
}

void show_current_quests(CHAR_DATA *ch)
{
    int num_attempting = 0, num_passed = 0;
    struct quest_info *quest;

    show_quest_header(ch);

    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++) {
	quest = *node;
	
	if(check_quest_pass(ch, quest->number))
	    num_passed = show_one_quest(ch, quest, num_passed);
	if(check_quest_current(ch, quest->number))
	    num_attempting = show_one_quest(ch, quest, num_attempting);
    }

    csendf(ch, " $B$2Attempting: $7%d$R", num_attempting);
    show_quest_amount(ch, num_passed);
    show_quest_footer(ch);

    return;
}

void show_complete_quests(CHAR_DATA *ch)
{
   int count = 0;
   struct quest_info *quest;

   show_quest_header(ch);

    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++) {
	quest = *node;
	
      if(check_quest_complete(ch, quest->number))
         count = show_one_complete_quest(ch, quest, count);
   }
   show_quest_amount(ch, count);
   show_quest_footer(ch);

   return;
}

int start_quest(CHAR_DATA *ch, struct quest_info *quest)
{
   int count = 0;
   uint16 price;
   OBJ_DATA *obj, *brownie;
   CHAR_DATA *mob;
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *qmaster = get_mob_vnum(QUEST_MASTER);

   if(check_quest_current(ch, quest->number)) {
      sprintf(buf, "q%d", quest->number);
      obj = get_obj(buf);
      if(!obj) return 0;
   }

   if(!check_available_quest(ch, quest)) {
     send_to_char("That quest is not available to you.\n\r", ch);
     return eFAILURE;
   }

   while(count < QUEST_MAX) {
      if(!ch->pcdata->quest_current[count])
         break;
      count++;
      if(count == QUEST_MAX) {
	send_to_char("You've got too many quests started already.\n\r", ch);
	return eEXTRA_VALUE;
      }
   }

   price = quest->cost;
   if(GET_PLATINUM(ch) < price) {
     csendf(ch, "You need %d platinum coins to start this quest, which you don't have!\n\r", price);
     return eEXTRA_VAL2;
   }
  
   if (quest->brownie) {
       brownie = get_obj_in_list_num(real_object(2), ch->carrying);
       if (!brownie) {
	   csendf(ch, "You need a brownie point to start this quest!\n\r", price);
	   return eEXTRA_VAL2;
       }
   }

   if((mob = get_mob_vnum(quest->mobnum))) {
      obj = clone_object(real_object(quest->objnum));
      obj->short_description = str_hsh(quest->objshort);
      obj->description = str_hsh(quest->objlong);

      sprintf(buf, "%s %s q%d", quest->objkey,  GET_NAME(ch), quest->number);
      obj->name = str_hsh(buf);

      SET_BIT(obj->obj_flags.extra_flags, ITEM_SPECIAL);

      obj_to_char(obj, mob);
      wear(mob, obj, keywordfind(obj));

      logf(IMMORTAL, LOG_QUEST, "%s started quest %d (%s) costing %d plats %d brownie(s).", GET_NAME(ch), quest->number, quest->name, quest->cost, quest->brownie);
   } else {
     send_to_char("This quest is temporarily unavailable.\n\r", ch);
     return eFAILURE;
   }

   ch->pcdata->quest_current[count] = quest->number;     
   ch->pcdata->quest_current_ticksleft[count] = quest->timer;
   quest->active = TRUE;
   count = 0;
   while(count < QUEST_PASS) {
      if(ch->pcdata->quest_pass[count] == quest->number) {
         ch->pcdata->quest_pass[count] = 0;
         break;
      }
      count++;
   }

   if (quest->brownie) {
	csendf(ch, "%s takes a brownie point from you.\n\r", GET_SHORT(qmaster));
	obj_from_char(brownie);
   }

   csendf(ch, "%s takes %d platinum from you.\n\r", GET_SHORT(qmaster), price);
   GET_PLATINUM(ch) -= price;

   return eSUCCESS;
}

int pass_quest(CHAR_DATA *ch, struct quest_info *quest)
{
   int count = 0;

   if(!quest) return eFAILURE;
   if(!check_quest_current(ch, quest->number)) return eFAILURE;

   while(count < QUEST_PASS) {
      if(!ch->pcdata->quest_pass[count])
         break;
      count++;
      if(count >= QUEST_PASS) return eEXTRA_VALUE;
   }

   logf(IMMORTAL, LOG_QUEST, "%s passed quest %d (%s).", GET_NAME(ch), quest->number, quest->name);

   ch->pcdata->quest_pass[count] = quest->number;

   return stop_current_quest(ch, quest);
}

int complete_quest(CHAR_DATA *ch, struct quest_info *quest)
{
   int count = 0;
   OBJ_DATA *obj;
   char buf[MAX_STRING_LENGTH];

   if(!quest) return eEXTRA_VALUE;

   while(count < QUEST_MAX) {
      if(ch->pcdata->quest_current[count] == quest->number)
         break;
      count++;
      if(count >= QUEST_MAX) {
         return eEXTRA_VALUE;
      }
   }
   sprintf(buf, "q%d", quest->number);
   obj = get_obj_in_list(buf, ch->carrying);

   if(!obj) {
     send_to_char("You do not appear to have the quest object yet.\n\r", ch);
     return eFAILURE;
   }

   obj_from_char(obj);
   ch->pcdata->quest_points += quest->reward;
   ch->pcdata->quest_current[count] = 0;
   ch->pcdata->quest_current_ticksleft[count] = 0;
   SETBIT(ch->pcdata->quest_complete, quest->number);
   quest->active = FALSE;
   
   logf(IMMORTAL, LOG_QUEST, "%s completed quest %d (%s) and won %d qpoints.", GET_NAME(ch), quest->number, quest->name, quest->reward);

   return eSUCCESS;
}

int stop_current_quest(CHAR_DATA *ch, struct quest_info *quest)
{
   int count = 0;
   OBJ_DATA *obj;
   char buf[MAX_STRING_LENGTH];

   if (!quest) return eFAILURE;

   while(count < QUEST_MAX) {
      if(ch->pcdata->quest_current[count] == quest->number)
         break;
      count++;
      if(count >= QUEST_MAX) {
         return eFAILURE;
      }
   }
   ch->pcdata->quest_current[count] = 0;
   ch->pcdata->quest_current_ticksleft[count] = 0;
   quest->active = FALSE;
   sprintf(buf, "q%d", quest->number);
   obj = get_obj(buf);
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

   for(int i = 0;i < QUEST_MAX; i++) {
      retval &= stop_current_quest(ch, ch->pcdata->quest_current[i]);
   }
   return retval;
}

void quest_update()
{
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *i, *next_dude, *mob;
   OBJ_DATA *obj;
   struct quest_info *quest;
   int retval;

   for(i = character_list; i; i = next_dude) {
      next_dude = i->next;

      if(!i->desc || IS_NPC(i)) continue;

    for (quest_list_t::iterator node = quest_list.begin(); node != quest_list.end(); node++) {
	quest = *node;
	
         if(quest->timer)
            for(int j=0;j<QUEST_MAX;j++)
               if(i->pcdata->quest_current[j] == quest->number) {
                  if(i->pcdata->quest_current_ticksleft[j] <= 0) {
                     retval = stop_current_quest(i, quest);

		     logf(IMMORTAL, LOG_QUEST, "%s ran out of time on quest %d (%s).", GET_NAME(i), quest->number, quest->name);

                     csendf(i, "Time has expired for %s.  This quest has ended.\n\r", quest->name);
                  }
                  i->pcdata->quest_current_ticksleft[j]--;
                  break;
               }

         if(check_quest_current(i, quest->number)) {
            sprintf(buf, "q%d", quest->number);
            obj = get_obj(buf);
            if(!obj) {
               if((mob = get_mob_vnum(quest->mobnum))) {
                  obj = clone_object(quest->objnum);
                  obj->short_description = str_hsh(quest->objshort);
                  obj->description = str_hsh(quest->objlong);
                  sprintf(buf, "%s q%d", quest->objkey, quest->number);
                  obj->name = str_hsh(buf);
                  obj_to_char(obj, mob);
                  wear(mob, obj, keywordfind(obj));
               }
            }            
         }
      }
   }
}

int quest_handler(CHAR_DATA *ch, CHAR_DATA *qmaster, int cmd, char *name)
{
   int retval;
   char buf[MAX_STRING_LENGTH];
   struct quest_info *quest;

   if(cmd != 1) {
     quest = get_quest_struct(name);
     if (quest == 0) {
       csendf(ch, "That is not a valid quest name.\n\r");
       return eFAILURE;
     }
   }

   switch(cmd) {
      case 1:
         do_emote(qmaster, "looks at his notes and writes a scroll.", 9);
         sprintf(buf, "%s Here are some currently available quests.", GET_NAME(ch));
         do_psay(qmaster, buf, 9);
         show_available_quests(ch);
         break;
      case 2:
         retval = pass_quest(ch, quest);
         if(IS_SET(retval, eSUCCESS)) {
            sprintf(buf, "%s You may begin this quest again if you speak with me.", GET_NAME(ch));
            do_psay(qmaster, buf, 9);
         }
         else if(IS_SET(retval, eEXTRA_VALUE)) {
            sprintf(buf, "%s You cannot pass up any more quests without completing some of them.", GET_NAME(ch));
            do_psay(qmaster, buf, 9);
         }
         else {
            sprintf(buf, "%s You weren't doing this quest to begin with.", GET_NAME(ch));
            do_psay(qmaster, buf, 9);
         }
         break;
      case 3:
         retval = start_quest(ch, quest);
         if(IS_SET(retval, eSUCCESS)) {
            sprintf(buf, "%s Excellent!  Let me write down the quest information for you.", GET_NAME(ch));
            do_psay(qmaster, buf, 9);
            do_emote(qmaster, "gives up the scroll.", 9);
            show_quest_header(ch);
            show_one_quest(ch, quest, 0);
            show_quest_footer(ch);
         }
         else if(IS_SET(retval, eEXTRA_VALUE)) {
            sprintf(buf, "%s You cannot start any more quests without completing some first.", GET_NAME(ch));
            do_psay(qmaster, buf, 9);
         }
         else if(IS_SET(retval, eEXTRA_VAL2)) {
            sprintf(buf, "%s You do not have the required funds to get the clue from me, beggar!", GET_NAME(ch));
            do_psay(qmaster, buf, 9);
         }
         else if(!retval) {
            sprintf(buf, "%s The quest item has left this world.  It will appear again soon.", GET_NAME(ch));
            do_psay(qmaster, buf, 9);
         }
         else {
            sprintf(buf, "%s Sorry, you cannot start this quest right now.", GET_NAME(ch));
            do_psay(qmaster, buf, 9);
         }
         break;
      case 4:
         retval = complete_quest(ch, quest);
         if(IS_SET(retval, eSUCCESS)) {
            sprintf(buf, "%s This is it!  Wonderful job, I will add your reward to your current amount of points!", GET_NAME(ch));
            do_psay(qmaster, buf, 9);
         }
         else if(IS_SET(retval, eEXTRA_VALUE)) {
            sprintf(buf, "%s You weren't doing this quest to begin with.", GET_NAME(ch));
            do_psay(qmaster, buf, 9);
         }
         else {
            sprintf(buf, "%s You have not yet completed this quest.", GET_NAME(ch));
            do_say(qmaster, buf, 9);
         }
         break;
      default:
         log("Bug in quest_handler, how'd they get here?", IMMORTAL, LOG_BUG);
         return eFAILURE;
   }
   return retval;
}

int quest_master(CHAR_DATA *ch, OBJ_DATA *obj, int cmd, char *arg, CHAR_DATA *owner)
{
   int choice;
   char buf[MAX_STRING_LENGTH];

   if((cmd != 59) && (cmd != 56))
      return eFAILURE;

   if (IS_AFFECTED(ch, AFF_BLIND))
      return eFAILURE;

   if(IS_NPC(ch))
      return eFAILURE;


   if(cmd == 59) {
      show_available_quests(ch);
      return eSUCCESS;
   }

   if(cmd == 56) {
      if((choice = atoi(arg)) == 0 || choice < 0) {
         sprintf(buf, "%s Try a number from the list.", GET_NAME(ch));
         do_tell(owner, buf, 9);
         return eSUCCESS;
      }
      switch (atoi(arg)){
         case 1:
            do_say(owner, "Sure, bum.", 9);
            break;
         default:
            sprintf(buf, "%s I don't offer that service.", GET_NAME(ch));
            do_tell(owner, buf, 9);
      }
   }

   return eSUCCESS;
}

int do_quest(CHAR_DATA *ch, char *arg, int cmd)
{
   int retval;
   char name[MAX_STRING_LENGTH];
   CHAR_DATA *qmaster = get_mob_vnum(QUEST_MASTER);

   half_chop(arg, arg, name);

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

int do_qedit(CHAR_DATA *ch, char *argument, int cmd)
{
   char arg[MAX_STRING_LENGTH];
   char field[MAX_STRING_LENGTH];
   char value[MAX_STRING_LENGTH];
   int  holdernum;
   int i, lownum, highnum;
   struct quest_info *quest = NULL;

   half_chop(argument, arg, argument);
   for(; *argument==' ';argument++);

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

   if(is_abbrev(arg, "save")) {
      send_to_char("Quests saved.\n\r", ch);
      return save_quests();
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
      "hint1",
      "hint2",
      "hint3",
      "cost",
      "brownie",
      NULL
   };

   if(is_abbrev(arg, "new")) {
      if(!*argument) {
         send_to_char("Usage: qedit new <name>\n\r", ch);
         return eFAILURE;
      }
      else {
         quest = get_quest_struct(argument);
         if(quest) {
            send_to_char("A quest by this name already exists.\n\r", ch);
            return eFAILURE;
         }
         else {
            do_add_quest(ch, argument);
            return eSUCCESS;
         }
      }
   }

   half_chop(argument, field, argument);
   for(; *argument==' ';argument++);

   if (*arg && is_number(arg) && !*field) {
     show_quest_info(ch, atoi(arg));
     return eSUCCESS;
   }

   if(is_abbrev(arg, "show")) {
      if(!*field || !is_number(field)) send_to_char("Usage: qedit show <number>\n\r", ch);
      else
         show_quest_info(ch, atoi(field));
      return eSUCCESS;
   }

   half_chop(argument, value, argument);
   for(; *argument==' ';argument++);

   if(is_abbrev(arg, "list") && !*field) {
      list_quests(ch, 0, QUEST_TOTAL);
      return eSUCCESS;
   } else if(is_abbrev(arg, "list") && *field && is_number(field)) {
      lownum = MAX(0,atoi(field));
      if(*value && is_number(value)) {
         highnum = MIN(QUEST_TOTAL, atoi(value));
         if(lownum > highnum) {
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

   if(!is_number(arg)) {
      send_to_char("Usage: qedit <number> <field> <value>\n\r", ch);
      return eFAILURE;
   }

   holdernum = atoi(arg);

   if(holdernum <= 0 || holdernum > QUEST_TOTAL) {
      send_to_char("Invalid quest number.\n\r", ch);
      return eFAILURE;
   }

   if(!(quest = get_quest_struct(holdernum))) {
      send_to_char("That quest doesn't exist.\n\r", ch);
      return eFAILURE;
   }

   if(!*field) {
      send_to_char("Valid fields: name, level, cost, brownie, objnum, objshort, objlong, objkey, mobnum, timer, reward or hints.\n\r", ch);
      return eFAILURE;
   }

   if(!(*value)) {
      send_to_char("You must enter a value.\n\r", ch);
      return eFAILURE;
   }

   i=0;
   while (valid_fields[i] != NULL) {
       if (is_abbrev(field, valid_fields[i]))
	   break;
       else
	   i++;
   }

   if (valid_fields[i] == NULL) {
       send_to_char("Valid fields: name, level, cost, brownie, objnum, objshort, objlong, objkey, mobnum, timer, reward, hint1, hint2, or hint3.\n\r",ch);
       return eFAILURE;
   }

   struct quest_info *oldquest;   

   switch(i) {
   case 0: // name
         sprintf(field, "%s %s", value, argument);
         oldquest = get_quest_struct(field);
         if(oldquest) {
            send_to_char("A quest by this name already exists.\n\r", ch);
            return eFAILURE;
         }
         else {
            csendf(ch, "Name changed from %s ", quest->name);
            quest->name = str_hsh(field);
            csendf(ch, "to %s.\n\r", quest->name);
         }
         break;
   case 1: // level
         csendf(ch, "Level changed from %d ", quest->level);
         quest->level = atoi(value);
         csendf(ch, "to %d.\n\r", quest->level);
         break;
   case 2: // objnum
         csendf(ch, "Objnum changed from %d ", quest->objnum);
         quest->objnum = atoi(value);
         csendf(ch, "to %d.\n\r", quest->objnum);
         break;
   case 3: // objshort
         csendf(ch, "Objshort changed from %s ", quest->objshort);
         sprintf(field, "%s %s", value, argument);
         quest->objshort = str_hsh(field);
         csendf(ch, "to %s.\n\r", quest->objshort);
         break;
   case 4: // objlong
         csendf(ch, "Objlong changed from %s ", quest->objlong);
         sprintf(field, "%s %s", value, argument);
         quest->objlong = str_hsh(field);
         csendf(ch, "to %s.\n\r", quest->objlong);
         break;
   case 5: // objkey
         csendf(ch, "Objkey changed from %s ", quest->objkey);
         sprintf(field, "%s %s", value, argument);
         quest->objkey = str_hsh(field);
         csendf(ch, "to %s.\n\r", quest->objkey);
         break;
   case 6: // mobnum
         csendf(ch, "Mobnum changed from %d ", quest->mobnum);
         quest->mobnum = atoi(value);
         csendf(ch, "to %d.\n\r", quest->mobnum);
         break;
   case 7: //timer
         csendf(ch, "Timer changed from %d ", quest->timer);
         quest->timer = atoi(value);
         csendf(ch, "to %d.\n\r", quest->timer);
         break;
   case 8: //reward
         csendf(ch, "Reward changed from %d ", quest->reward);
         quest->reward = atoi(value);
         csendf(ch, "to %d.\n\r", quest->reward);
         break;
   case 9: // hint1
         sprintf(field, "%s %s", value, argument);
         csendf(ch, "Hint #1 changed from %s ", quest->hint1);
         quest->hint1 = str_hsh(field);
         csendf(ch, "to %s.\n\r", quest->hint1);
         break;
   case 10: // hint2
         sprintf(field, "%s %s", value, argument);
         csendf(ch, "Hint #2 changed from %s ", quest->hint2);
         quest->hint2 = str_hsh(field);
         csendf(ch, "to %s.\n\r", quest->hint2);
         break;
   case 11: //hint3
         sprintf(field, "%s %s", value, argument);
         csendf(ch, "Hint #3 changed from %s ", quest->hint3);
         quest->hint3 = str_hsh(field);
         csendf(ch, "to %s.\n\r", quest->hint3);
         break;
   case 12: //cost
         csendf(ch, "Cost changed from %d ", quest->cost);
         quest->cost = atoi(value);
         csendf(ch, "to %d.\n\r", quest->cost);
	 break;
   case 13: // brownie
       if (quest->brownie) {
	   csendf(ch, "Brownie toggled to NOT required.\n\r");
	   quest->brownie = 0;
       } else {
	   csendf(ch, "Brownie toggled to required.\n\r");
	   quest->brownie = 1;
       }
       break;
      default:
         log("Screw up in do_edit_quest, whatsamaddahyou?", IMMORTAL, LOG_BUG);
         return eFAILURE;
   }
   return eSUCCESS;
}
