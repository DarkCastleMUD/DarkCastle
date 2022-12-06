/***************************************************************************
 *  File: nanny.c, for people who haven't logged in        Part of DIKUMUD *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as this   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 *                                                                         *
 * Revision History                                                        *
 * 10/16/2003   Onager    Added on_forbidden_name_list() to load           *
 *                        forbidden names from a file instead of a hard-   *
 *                        coded list.                                      *
 ***************************************************************************/
/* $Id: nanny.cpp,v 1.198 2015/05/26 08:55:40 zen Exp $ */
extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
// #include <arpa/telnet.h>
#include <unistd.h>
}
#include <string.h>
#include <queue>
#include <fmt/format.h>

#include "character.h"
#include "comm.h"
#include "connect.h"
#include "race.h"
#include "player.h"
#include "structs.h" // TRUE
#include "utility.h"
#include "levels.h"
#include "ki.h"
#include "clan.h"
#include "fileinfo.h" // SAVE_DIR
#include "db.h"       // init_char..
#include "mobile.h"
#include "interp.h"
#include "room.h"
#include "act.h"
#include "clan.h"
#include "spells.h"
#include "fight.h"
#include "handler.h"
#include "vault.h"
#include "const.h"
#include "guild.h"
#include <string>

using namespace std;

#define STATE(d) ((d)->connected)

void AuctionHandleDelete(string name);
bool is_bracing(char_data *bracee, struct room_direction_data *exit);
void check_for_sold_items(char_data *ch);
void show_question_race(descriptor_data *d);

const char menu[] = "\n\rWelcome to Dark Castle Mud\n\r\n\r"
                    "0) Exit Dark Castle.\n\r"
                    "1) Enter the game.\n\r"
                    "2) Enter your character's description.\n\r"
                    "3) Change your password.\n\r"
                    "4) Archive this character.\n\r"
                    "5) Delete this character.\n\r\n\r"
                    "   Make your choice: ";

bool wizlock = false;

extern char greetings1[MAX_STRING_LENGTH];
extern char greetings2[MAX_STRING_LENGTH];
extern char greetings3[MAX_STRING_LENGTH];
extern char greetings4[MAX_STRING_LENGTH];
extern char webpage[MAX_STRING_LENGTH];
extern char motd[MAX_STRING_LENGTH];
extern char imotd[MAX_STRING_LENGTH];
extern struct descriptor_data *descriptor_list;
extern obj_data *object_list;
extern struct index_data *obj_index;
extern CWorld world;
extern CVoteData *DCVote;

int isbanned(char *hostname);
int _parse_email(char *arg);
bool check_deny(struct descriptor_data *d, char *name);
void update_wizlist(char_data *ch);
void isr_set(char_data *ch);
bool check_reconnect(struct descriptor_data *d, char *name, bool fReconnect);
bool check_playing(struct descriptor_data *d, char *name);
bool on_forbidden_name_list(char *name);
void check_hw(char_data *ch);
char *str_str(char *first, char *second);
bool apply_race_attributes(char_data *ch, int race = 0);
bool check_race_attributes(char_data *ch, int race = 0);
bool handle_get_race(descriptor_data *d, string arg);
void show_question_race(descriptor_data *d);
void show_question_class(descriptor_data *d);
bool handle_get_class(descriptor_data *d, string arg);
int is_clss_race_compat(char_data *ch, int clss);
void show_question_stats(descriptor_data *d);
bool handle_get_stats(descriptor_data *d, string arg);

int is_race_eligible(char_data *ch, int race)
{
   if (race == 2 && (GET_RAW_DEX(ch) < 10 || GET_RAW_INT(ch) < 10))
      return FALSE;
   if (race == 3 && (GET_RAW_CON(ch) < 10 || GET_RAW_WIS(ch) < 10))
      return FALSE;
   if (race == 4 && (GET_RAW_DEX(ch) < 10))
      return FALSE;
   if (race == 5 && (GET_RAW_DEX(ch) < 12))
      return FALSE;
   if (race == 6 && (GET_RAW_STR(ch) < 12))
      return FALSE;
   if (race == 7 && (GET_RAW_WIS(ch) < 12))
      return FALSE;
   if (race == 8 && (GET_RAW_CON(ch) < 10 || GET_RAW_STR(ch) < 10))
      return FALSE;
   if (race == 9 && (GET_RAW_CON(ch) < 12))
      return FALSE;
   return TRUE;
}

int is_clss_race_compat(const char_data *ch, int clss, int race)
{
   bool compat = false;

   switch (clss)
   {
   case CLASS_MAGIC_USER:
      compat = true;
      break;
   case CLASS_CLERIC:
      compat = true;
      break;
   case CLASS_THIEF:
      if (race != RACE_GIANT)
      {
         compat = true;
      }
      break;
   case CLASS_WARRIOR:
      compat = true;
      break;
   case CLASS_ANTI_PAL:
      if (race == RACE_HUMAN || race == RACE_ORC || race == RACE_DWARVEN)
      {
         compat = true;
      }
      break;
   case CLASS_PALADIN:
      if (race == RACE_HUMAN || race == RACE_ELVEN || race == RACE_DWARVEN)
      {
         compat = true;
      }
      break;
   case CLASS_BARBARIAN:
      if (race != RACE_PIXIE)
      {
         compat = true;
      }
      break;
   case CLASS_MONK:
      compat = true;
      break;
   case CLASS_RANGER:
      compat = true;
      break;
   case CLASS_BARD:
      compat = true;
      break;
   case CLASS_DRUID:
      compat = true;
      break;
   }
   return (compat);
}

int is_clss_eligible(char_data *ch, int clss)
{
   int x = 0;

   switch (clss)
   {
   case CLASS_MAGIC_USER:
      if (GET_RAW_INT(ch) >= 15)
         x = 1;
      ;
      break;
   case CLASS_CLERIC:
      if (GET_RAW_WIS(ch) >= 15)
         x = 1;
      break;
   case CLASS_THIEF:
      if (GET_RAW_DEX(ch) >= 15 && GET_RACE(ch) != RACE_GIANT)
         x = 1;
      break;
   case CLASS_WARRIOR:
      if (GET_RAW_STR(ch) >= 15)
         x = 1;
      break;
   case CLASS_ANTI_PAL:
      if (GET_RAW_INT(ch) >= 14 && GET_RAW_DEX(ch) >= 14 &&
          (GET_RACE(ch) == RACE_HUMAN || GET_RACE(ch) == RACE_ORC || GET_RACE(ch) == RACE_DWARVEN))
         x = 1;
      break;
   case CLASS_PALADIN:
      if (GET_RAW_WIS(ch) >= 14 && GET_RAW_STR(ch) >= 14 &&
          (GET_RACE(ch) == RACE_HUMAN || GET_RACE(ch) == RACE_ELVEN || GET_RACE(ch) == RACE_DWARVEN))
         x = 1;
      break;
   case CLASS_BARBARIAN:
      if (GET_RAW_STR(ch) >= 14 && GET_RAW_CON(ch) >= 14 && GET_RACE(ch) != RACE_PIXIE)
         x = 1;
      break;
   case CLASS_MONK:
      if (GET_RAW_CON(ch) >= 14 && GET_RAW_WIS(ch) >= 14)
         x = 1;
      break;
   case CLASS_RANGER:
      if (GET_RAW_CON(ch) >= 14 && GET_RAW_DEX(ch) >= 14)
         x = 1;
      break;
   case CLASS_BARD:
      if (GET_RAW_CON(ch) >= 14 && GET_RAW_INT(ch) >= 14)
         x = 1;
      break;
   case CLASS_DRUID:
      if (GET_RAW_WIS(ch) >= 14 && GET_RAW_WIS(ch) >= 14)
         x = 1;
      break;
   }
   return (x);
}

void do_inate_race_abilities(char_data *ch)
{

   // Add race base saving throw mods
   // Yes, I could combine this 'switch' with the next one, but this is
   // alot more readable
   switch (GET_RACE(ch))
   {
   case RACE_HUMAN:
      ch->saves[SAVE_TYPE_FIRE] += RACE_HUMAN_FIRE_MOD;
      ch->saves[SAVE_TYPE_COLD] += RACE_HUMAN_COLD_MOD;
      ch->saves[SAVE_TYPE_ENERGY] += RACE_HUMAN_ENERGY_MOD;
      ch->saves[SAVE_TYPE_ACID] += RACE_HUMAN_ACID_MOD;
      ch->saves[SAVE_TYPE_MAGIC] += RACE_HUMAN_MAGIC_MOD;
      ch->saves[SAVE_TYPE_POISON] += RACE_HUMAN_POISON_MOD;
      break;
   case RACE_ELVEN:
      ch->saves[SAVE_TYPE_FIRE] += RACE_ELVEN_FIRE_MOD;
      ch->saves[SAVE_TYPE_COLD] += RACE_ELVEN_COLD_MOD;
      ch->saves[SAVE_TYPE_ENERGY] += RACE_ELVEN_ENERGY_MOD;
      ch->saves[SAVE_TYPE_ACID] += RACE_ELVEN_ACID_MOD;
      ch->saves[SAVE_TYPE_MAGIC] += RACE_ELVEN_MAGIC_MOD;
      ch->saves[SAVE_TYPE_POISON] += RACE_ELVEN_POISON_MOD;
      ch->spell_mitigation += 1;
      break;
   case RACE_DWARVEN:
      ch->saves[SAVE_TYPE_FIRE] += RACE_DWARVEN_FIRE_MOD;
      ch->saves[SAVE_TYPE_COLD] += RACE_DWARVEN_COLD_MOD;
      ch->saves[SAVE_TYPE_ENERGY] += RACE_DWARVEN_ENERGY_MOD;
      ch->saves[SAVE_TYPE_ACID] += RACE_DWARVEN_ACID_MOD;
      ch->saves[SAVE_TYPE_MAGIC] += RACE_DWARVEN_MAGIC_MOD;
      ch->saves[SAVE_TYPE_POISON] += RACE_DWARVEN_POISON_MOD;
      ch->melee_mitigation += 1;
      break;
   case RACE_TROLL:
      ch->saves[SAVE_TYPE_FIRE] += RACE_TROLL_FIRE_MOD;
      ch->saves[SAVE_TYPE_COLD] += RACE_TROLL_COLD_MOD;
      ch->saves[SAVE_TYPE_ENERGY] += RACE_TROLL_ENERGY_MOD;
      ch->saves[SAVE_TYPE_ACID] += RACE_TROLL_ACID_MOD;
      ch->saves[SAVE_TYPE_MAGIC] += RACE_TROLL_MAGIC_MOD;
      ch->saves[SAVE_TYPE_POISON] += RACE_TROLL_POISON_MOD;
      ch->spell_mitigation += 2;
      break;
   case RACE_GIANT:
      ch->saves[SAVE_TYPE_FIRE] += RACE_GIANT_FIRE_MOD;
      ch->saves[SAVE_TYPE_COLD] += RACE_GIANT_COLD_MOD;
      ch->saves[SAVE_TYPE_ENERGY] += RACE_GIANT_ENERGY_MOD;
      ch->saves[SAVE_TYPE_ACID] += RACE_GIANT_ACID_MOD;
      ch->saves[SAVE_TYPE_MAGIC] += RACE_GIANT_MAGIC_MOD;
      ch->saves[SAVE_TYPE_POISON] += RACE_GIANT_POISON_MOD;
      ch->melee_mitigation += 2;
      break;
   case RACE_PIXIE:
      ch->saves[SAVE_TYPE_FIRE] += RACE_PIXIE_FIRE_MOD;
      ch->saves[SAVE_TYPE_COLD] += RACE_PIXIE_COLD_MOD;
      ch->saves[SAVE_TYPE_ENERGY] += RACE_PIXIE_ENERGY_MOD;
      ch->saves[SAVE_TYPE_ACID] += RACE_PIXIE_ACID_MOD;
      ch->saves[SAVE_TYPE_MAGIC] += RACE_PIXIE_MAGIC_MOD;
      ch->saves[SAVE_TYPE_POISON] += RACE_PIXIE_POISON_MOD;
      ch->spell_mitigation += 2;
      break;
   case RACE_HOBBIT:
      ch->saves[SAVE_TYPE_FIRE] += RACE_HOBBIT_FIRE_MOD;
      ch->saves[SAVE_TYPE_COLD] += RACE_HOBBIT_COLD_MOD;
      ch->saves[SAVE_TYPE_ENERGY] += RACE_HOBBIT_ENERGY_MOD;
      ch->saves[SAVE_TYPE_ACID] += RACE_HOBBIT_ACID_MOD;
      ch->saves[SAVE_TYPE_MAGIC] += RACE_HOBBIT_MAGIC_MOD;
      ch->saves[SAVE_TYPE_POISON] += RACE_HOBBIT_POISON_MOD;
      ch->melee_mitigation += 2;
      break;
   case RACE_GNOME:
      ch->saves[SAVE_TYPE_FIRE] += RACE_GNOME_FIRE_MOD;
      ch->saves[SAVE_TYPE_COLD] += RACE_GNOME_COLD_MOD;
      ch->saves[SAVE_TYPE_ENERGY] += RACE_GNOME_ENERGY_MOD;
      ch->saves[SAVE_TYPE_ACID] += RACE_GNOME_ACID_MOD;
      ch->saves[SAVE_TYPE_MAGIC] += RACE_GNOME_MAGIC_MOD;
      ch->saves[SAVE_TYPE_POISON] += RACE_GNOME_POISON_MOD;
      ch->spell_mitigation += 1;
      break;
   case RACE_ORC:
      ch->saves[SAVE_TYPE_FIRE] += RACE_ORC_FIRE_MOD;
      ch->saves[SAVE_TYPE_COLD] += RACE_ORC_COLD_MOD;
      ch->saves[SAVE_TYPE_ENERGY] += RACE_ORC_ENERGY_MOD;
      ch->saves[SAVE_TYPE_ACID] += RACE_ORC_ACID_MOD;
      ch->saves[SAVE_TYPE_MAGIC] += RACE_ORC_MAGIC_MOD;
      ch->saves[SAVE_TYPE_POISON] += RACE_ORC_POISON_MOD;
      ch->melee_mitigation += 1;
      break;
   default:
      break;
   }
}

obj_data *clan_altar(char_data *ch)
{
   clan_data *clan;
   struct clan_room_data *room;
   extern clan_data *clan_list;

   if (ch->clan)
      for (clan = clan_list; clan; clan = clan->next)
         if (clan->number == ch->clan)
         {
            for (room = clan->rooms; room; room = room->next)
            {
               if (real_room(room->room_number) == -1)
                  continue;
               obj_data *t = world[real_room(room->room_number)].contents;
               for (; t; t = t->next_content)
               {
                  if (t->obj_flags.type_flag == ITEM_ALTAR)
                     return t;
               }
            }
         }
   return NULL;
}

void update_max_who(void)
{
   uint64_t players = 0;
   for (auto d = descriptor_list; d != nullptr; d = d->next)
   {
      switch (d->connected)
      {
      case conn::PLAYING:
      case conn::EDIT_MPROG:
      case conn::EDITING:
      case conn::EXDSCR:
      case conn::SEND_MAIL:
      case conn::WRITE_BOARD:
         players++;
         break;
      }
   }

   if (players > max_who)
   {
      max_who = players;
   }
}

// stuff that has to be done on both a normal login, as well as on
// a hotboot login
void do_on_login_stuff(char_data *ch)
{
   void add_to_bard_list(char_data * ch);

   add_to_bard_list(ch);
   ch->pcdata->bad_pw_tries = 0;
   redo_hitpoints(ch);
   redo_mana(ch);
   redo_ki(ch);
   do_inate_race_abilities(ch);
   check_hw(ch);
   /* Add a character's skill item's to the list. */
   ch->pcdata->skillchange = NULL;
   ch->spellcraftglyph = 0;
   for (int i = 0; i < MAX_WEAR; i++)
   {
      if (!ch->equipment[i])
         continue;
      for (int a = 0; a < ch->equipment[i]->num_affects; a++)
      {
         if (ch->equipment[i]->affected[a].location >= 1000)
         {
            ch->equipment[i]->next_skill = ch->pcdata->skillchange;
            ch->pcdata->skillchange = ch->equipment[i];
            ch->equipment[i]->next_skill = NULL;
         }
      }
   }
   // add character base saves to saving throws
   for (int i = 0; i <= SAVE_TYPE_MAX; i++)
   {
      ch->saves[i] += GET_LEVEL(ch) / 4;
      ch->saves[i] += ch->pcdata->saves_mods[i];
   }

   if (GET_TITLE(ch) == NULL)
   {
      GET_TITLE(ch) = str_dup("is a virgin.");
   }

   if (GET_CLASS(ch) == CLASS_MONK)
   {
      GET_AC(ch) -= (GET_LEVEL(ch) * 2);
   }
   GET_AC(ch) -= has_skill(ch, SKILL_COMBAT_MASTERY) / 2;

   GET_AC(ch) -= GET_AC_METAS(ch);

   if (affected_by_spell(ch, INTERNAL_SLEEPING))
   {
      affect_from_char(ch, INTERNAL_SLEEPING);
   }
   /* Set ISR's cause they're not saved...   */
   isr_set(ch);
   ch->altar = clan_altar(ch);

   if (!IS_MOB(ch) && GET_LEVEL(ch) >= IMMORTAL)
   {
      ch->pcdata->holyLite = TRUE;
      GET_COND(ch, THIRST) = -1;
      GET_COND(ch, FULL) = -1;
   }
   add_totem_stats(ch);
   if (GET_LEVEL(ch) < 5 && GET_AGE(ch) < 21)
      char_to_room(ch, real_room(200));
   else if (ch->in_room >= 2)
      char_to_room(ch, ch->in_room);
   else if (GET_LEVEL(ch) >= IMMORTAL)
      char_to_room(ch, real_room(17));
   else
      char_to_room(ch, real_room(START_ROOM));

   ch->curLeadBonus = 0;
   ch->changeLeadBonus = FALSE;
   ch->cRooms = 0;
   REMBIT(ch->affected_by, AFF_BLACKJACK_ALERT);
   for (int i = 0; i < QUEST_MAX; i++)
   {
      ch->pcdata->quest_current[i] = -1;
      ch->pcdata->quest_current_ticksleft[i] = 0;
   }
   struct vault_data *vault = has_vault(GET_NAME(ch));
   if (ch->pcdata->time.logon < 1172204700)
   {
      if (vault)
      {
         int adder = GET_LEVEL(ch) - 50;
         if (adder < 0)
            adder = 0; // Heh :P
         vault->size += adder * 10;
         if (vault->size < 100)
            vault->size = 100;
         save_vault(vault->owner);
      }
   }

   if (vault)
   {
      if (vault->size < (unsigned)(GET_LEVEL(ch) * 10))
      {
         logf(IMMORTAL, LogChannels::LOG_BUG, "%s's vault reset from %d to %d during login.", GET_NAME(ch), vault->size, GET_LEVEL(ch) * 10);
         vault->size = GET_LEVEL(ch) * 10;
      }

      save_vault(vault->owner);
   }

   if (ch->pcdata->time.logon < 1151506181)
   {
      ch->pcdata->quest_points = 0;
      for (int i = 0; i < QUEST_CANCEL; i++)
         ch->pcdata->quest_cancel[i] = 0;
      for (int i = 0; i < QUEST_TOTAL / ASIZE; i++)
         ch->pcdata->quest_complete[i] = 0;
   }
   if (ch->pcdata->time.logon < 1151504181)
      SET_BIT(ch->misc, LogChannels::CHANNEL_TELL);

   if (ch->pcdata->time.logon < 1171757100)
   {
      switch (GET_CLASS(ch))
      {
      case CLASS_MAGE:
         GET_AC(ch) += 100;
         break;
      case CLASS_DRUID:
         GET_AC(ch) += 85;
         break;
      case CLASS_CLERIC:
         GET_AC(ch) += 70;
         break;
      case CLASS_ANTI_PAL:
         GET_AC(ch) += 55;
         break;
      case CLASS_THIEF:
         GET_AC(ch) += 40;
         break;
      case CLASS_BARD:
         GET_AC(ch) += 25;
         break;
      case CLASS_BARBARIAN:
         GET_AC(ch) += 10;
         break;
      case CLASS_RANGER:
         GET_AC(ch) -= 5;
         break;
      case CLASS_PALADIN:
         GET_AC(ch) -= 20;
         break;
      case CLASS_WARRIOR:
         GET_AC(ch) -= 35;
         break;
      case CLASS_MONK:
         GET_AC(ch) -= 50;
         break;
      default:
         break;
      }
   }

   if (GET_CLASS(ch) == CLASS_MONK && GET_LEVEL(ch) > 10)
   {
      ch->swapSkill(SKILL_SHIELDBLOCK, SKILL_DEFENSE);
   }
   if (GET_CLASS(ch) == CLASS_PALADIN && GET_LEVEL(ch) >= 41)
   {
      ch->swapSkill(SPELL_ARMOR, SPELL_AEGIS);
      ch->swapSkill(SPELL_POWER_HARM, SPELL_DIVINE_FURY);
   }
   if (GET_CLASS(ch) == CLASS_RANGER && GET_LEVEL(ch) > 9)
   {
      if (ch->skills.contains(SKILL_SHIELDBLOCK))
      {
         ch->swapSkill(SKILL_SHIELDBLOCK, SKILL_DODGE);
         ch->setSkillMin(SKILL_DODGE, 50);
      }
   }
   if (GET_CLASS(ch) == CLASS_ANTI_PAL && GET_LEVEL(ch) >= 44)
   {
      ch->swapSkill(SPELL_STONE_SKIN, SPELL_U_AEGIS);
   }
   if (GET_CLASS(ch) == CLASS_BARD && GET_LEVEL(ch) >= 30)
   {
      ch->swapSkill(SKILL_BLUDGEON_WEAPONS, SKILL_STINGING_WEAPONS);
   }
   if (GET_CLASS(ch) == CLASS_CLERIC && GET_LEVEL(ch) >= 42)
   {
      ch->swapSkill(SPELL_RESIST_FIRE, SPELL_RESIST_MAGIC);
      ch->skills.erase(SPELL_RESIST_COLD);
   }
   if (GET_CLASS(ch) == CLASS_MAGIC_USER)
   {
      ch->skills.erase(SPELL_SLEEP);
      ch->skills.erase(SPELL_RESIST_COLD);
      ch->skills.erase(SPELL_KNOW_ALIGNMENT);
   }
   // Remove pick if they're no longer allowed to have it.
   if (GET_CLASS(ch) == CLASS_THIEF && GET_LEVEL(ch) < 22 && has_skill(ch, SKILL_PICK_LOCK))
   {
      ch->skills.erase(SKILL_PICK_LOCK);
   }
   if (GET_CLASS(ch) == CLASS_BARD && has_skill(ch, SKILL_HIDE))
   {
      ch->skills.erase(SKILL_HIDE);
   }
   // Remove listsongs
   if (GET_CLASS(ch) == CLASS_BARD && has_skill(ch, SKILL_SONG_LIST_SONGS))
   {
      ch->skills.erase(SKILL_SONG_LIST_SONGS);
   }
   // Replace shieldblock on barbs
   if (GET_CLASS(ch) == CLASS_BARBARIAN && has_skill(ch, SKILL_SHIELDBLOCK))
   {
      ch->swapSkill(SKILL_SHIELDBLOCK, SKILL_DODGE);
   }
   // Replace eagle-eye on druids
   if (GET_CLASS(ch) == CLASS_DRUID && has_skill(ch, SPELL_EAGLE_EYE))
   {
      ch->swapSkill(SPELL_EAGLE_EYE, SPELL_GHOSTWALK);
   }
   // Replace crushing on bards
   if (GET_CLASS(ch) == CLASS_BARD && has_skill(ch, SKILL_CRUSHING_WEAPONS))
   {
      ch->swapSkill(SKILL_CRUSHING_WEAPONS, SKILL_WHIPPING_WEAPONS);
   }
   // Replace crushing on thieves
   if (GET_CLASS(ch) == CLASS_THIEF && has_skill(ch, SKILL_CRUSHING_WEAPONS))
   {
      ch->swapSkill(SKILL_CRUSHING_WEAPONS, SKILL_STINGING_WEAPONS);
   }
   // Replace firestorm on antis
   if (GET_CLASS(ch) == CLASS_ANTI_PAL && has_skill(ch, SPELL_FIRESTORM))
   {
      ch->swapSkill(SPELL_FIRESTORM, SPELL_LIFE_LEECH);
   }

   class_skill_defines *c_skills = get_skill_list(ch);

   if (IS_MORTAL(ch))
   {
      queue<skill_t> skills_to_delete = {};
      for (auto &curr : ch->skills)
      {
         if (curr.first < 600 && search_skills2(curr.first, c_skills) == -1 && search_skills2(curr.first, g_skills) == -1 && curr.first != META_REIMB && curr.first != NEW_SAVE)
         {
            log(fmt::format("Removing skill {} from {}", curr.first, GET_NAME(ch)), IMMORTAL, LogChannels::LOG_PLAYER);
            // ch->send(fmt::format("Removing skill {}\r\n", curr.first));
            skills_to_delete.push(curr.first);
         }
      }
      while (skills_to_delete.empty() == false)
      {
         ch->skills.erase(skills_to_delete.front());
         skills_to_delete.pop();
      }
   }

   barb_magic_resist(ch, 0, has_skill(ch, SKILL_MAGIC_RESIST));
   /* meta reimbursement */
   if (!has_skill(ch, META_REIMB))
   {
      learn_skill(ch, META_REIMB, 1, 100);
      extern int64_t new_meta_platinum_cost(int start, int end);
      extern int r_new_meta_platinum_cost(int start, int64_t plats);
      extern int r_new_meta_exp_cost(int start, int64_t exp);

      extern int64_t moves_exp_spent(char_data * ch);
      extern int64_t moves_plats_spent(char_data * ch);
      extern int64_t hps_exp_spent(char_data * ch);
      extern int64_t hps_plats_spent(char_data * ch);
      extern int64_t mana_exp_spent(char_data * ch);
      extern int64_t mana_plats_spent(char_data * ch);
      int new_ = MIN(r_new_meta_platinum_cost(0, hps_plats_spent(ch)), r_new_meta_exp_cost(0, hps_exp_spent(ch)));
      int ometa = GET_HP_METAS(ch);
      GET_HP_METAS(ch) = new_;
      GET_RAW_HIT(ch) += new_ - ometa;
      new_ = MIN(r_new_meta_platinum_cost(0, mana_plats_spent(ch)), r_new_meta_exp_cost(0, mana_exp_spent(ch)));
      ometa = GET_MANA_METAS(ch);
      GET_RAW_MANA(ch) += new_ - ometa;
      GET_MANA_METAS(ch) = new_;
      new_ = MIN(r_new_meta_platinum_cost(0, moves_plats_spent(ch)), r_new_meta_exp_cost(0, moves_exp_spent(ch)));
      ometa = GET_MOVE_METAS(ch);
      GET_MOVE_METAS(ch) = new_;
      GET_RAW_MOVE(ch) += new_ - ometa;
   }
   /* end meta reimbursement */

   prepare_character_for_sixty(ch);

   // Check for deleted characters listed in access list
   char buf[255];
   std::queue<char *> todelete;
   vault = has_vault(GET_NAME(ch));
   if (vault)
   {
      for (vault_access_data *access = vault->access; access && access != (vault_access_data *)0x95959595; access = access->next)
      {
         if (access->name)
         {
            snprintf(buf, 255, "%s/%c/%s", SAVE_DIR, UPPER(access->name[0]), access->name);
            if (!file_exists(buf))
            {
               todelete.push(access->name);
            }
         }
      }
   }

   while (!todelete.empty())
   {
      snprintf(buf, 255, "Deleting %s from %s's vault access list.\n", todelete.front(), GET_NAME(ch));
      log(buf, 0, LogChannels::LOG_MORTAL);
      remove_vault_access(ch, todelete.front(), vault);
      todelete.pop();
   }
}

void roll_and_display_stats(char_data *ch)
{
   int x, a, b;
   char buf[MAX_STRING_LENGTH];

   for (x = 0; x <= 4; x++)
   {
      a = dice(3, 6);
      b = dice(6, 3);
      ch->desc->stats->str[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      ch->desc->stats->dex[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      ch->desc->stats->con[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      ch->desc->stats->tel[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      ch->desc->stats->wis[x] = MAX(12 + number(0, 1), MAX(a, b));
   }

   /*
   For testing purposes
   ch->desc->stats->str[0] = 13;
   ch->desc->stats->dex[0] = 14;
   ch->desc->stats->con[0] = 13;
   ch->desc->stats->tel[0] = 12;
   ch->desc->stats->wis[0] = 14;
   */

   SEND_TO_Q("\n\r  Choose from any of the following groups of abilities...     \n\r", ch->desc);

   SEND_TO_Q("Group: 1     2     3     4     5\n\r", ch->desc);
   sprintf(buf, "Str:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
           ch->desc->stats->str[0], ch->desc->stats->str[1], ch->desc->stats->str[2],
           ch->desc->stats->str[3], ch->desc->stats->str[4]);
   SEND_TO_Q(buf, ch->desc);
   sprintf(buf, "Dex:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
           ch->desc->stats->dex[0], ch->desc->stats->dex[1], ch->desc->stats->dex[2],
           ch->desc->stats->dex[3], ch->desc->stats->dex[4]);
   SEND_TO_Q(buf, ch->desc);
   sprintf(buf, "Con:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
           ch->desc->stats->con[0], ch->desc->stats->con[1], ch->desc->stats->con[2],
           ch->desc->stats->con[3], ch->desc->stats->con[4]);
   SEND_TO_Q(buf, ch->desc);
   sprintf(buf, "Int:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
           ch->desc->stats->tel[0], ch->desc->stats->tel[1], ch->desc->stats->tel[2],
           ch->desc->stats->tel[3], ch->desc->stats->tel[4]);
   SEND_TO_Q(buf, ch->desc);
   sprintf(buf, "Wis:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
           ch->desc->stats->wis[0], ch->desc->stats->wis[1], ch->desc->stats->wis[2],
           ch->desc->stats->wis[3], ch->desc->stats->wis[4]);
   SEND_TO_Q(buf, ch->desc);
   SEND_TO_Q("Choose a group <1-5>, or press return to reroll(Help <attribute> for more information) --> ", ch->desc);
   telnet_ga(ch->desc);

   WAIT_STATE(ch, PULSE_TIMER / 10);
}

int count_IP_connections(struct descriptor_data *new_conn)
{
   int count = 0;
   for (struct descriptor_data *d = descriptor_list; d; d = d->next)
   {
      if (!d->host)
         continue;
      if (!strcmp(new_conn->host, d->host))
         count++;
   }

   if (count > 20)
   {
      SEND_TO_Q("Sorry, there are more than 20 connections from this IP address\r\n"
                "already logged into Dark Castle.  If you have a valid reason\r\n"
                "for having this many connections from one IP please let an imm\r\n"
                "know and they will speak with you.\r\n",
                new_conn);
      close_socket(new_conn);
      return 1;
   }

   return 0;
}

const char *host_list[] =
    {
        "127.0.0.1", // localhost (duh)
};

bool allowed_host(char *host)
{ /* Wizlock uses hosts for wipe. */
   int i;
   for (i = 0; i < (int)((sizeof(host_list) / sizeof(char *))); i++)
      if (!str_prefix(host_list[i], host))
         return TRUE;
   return FALSE;
}

void check_hw(char_data *ch)
{
   heightweight(ch, FALSE);
   if (ch->height > races[ch->race].max_height)
   {
      logf(IMP, LogChannels::LOG_BUG, "check_hw: %s's height %d > max %d. height set to max.", GET_NAME(ch), GET_HEIGHT(ch), races[ch->race].max_height);
      ch->height = races[ch->race].max_height;
   }
   if (ch->height < races[ch->race].min_height)
   {
      logf(IMP, LogChannels::LOG_BUG, "check_hw: %s's height %d < min %d. height set to min.", GET_NAME(ch), GET_HEIGHT(ch), races[ch->race].min_height);
      ch->height = races[ch->race].min_height;
   }

   if (ch->weight > races[ch->race].max_weight)
   {
      logf(IMP, LogChannels::LOG_BUG, "check_hw: %s's weight %d > max %d. weight set to max.", GET_NAME(ch), GET_WEIGHT(ch), races[ch->race].max_weight);
      ch->weight = races[ch->race].max_weight;
   }
   if (ch->weight < races[ch->race].min_weight)
   {
      logf(IMP, LogChannels::LOG_BUG, "check_hw: %s's weight %d < min %d. weight set to min.", GET_NAME(ch), GET_WEIGHT(ch), races[ch->race].min_weight);
      ch->weight = races[ch->race].min_weight;
   }
   heightweight(ch, TRUE);
}

void set_hw(char_data *ch)
{
   ch->height = number(races[ch->race].min_height, races[ch->race].max_height);
   logf(ANGEL, LogChannels::LOG_MORTAL, "%s's height set to %d", GET_NAME(ch), GET_HEIGHT(ch));
   ch->weight = number(races[ch->race].min_weight, races[ch->race].max_weight);
   logf(ANGEL, LogChannels::LOG_MORTAL, "%s's weight set to %d", GET_NAME(ch), GET_WEIGHT(ch));
}

// Deal with sockets that haven't logged in yet.
void nanny(struct descriptor_data *d, string arg)
{
   char buf[MAX_STRING_LENGTH];
   stringstream str_tmp;
   char tmp_name[20];
   char *password;
   bool fOld;
   char_data *ch;
   int y;
   char badclssmsg[] = "You must choose a class that matches your stats. These are marked by a '*'.\n\rSelect a class-> ";
   unsigned selection = 0;
   auto &character_list = DC::getInstance()->character_list;
   char log_buf[MAX_STRING_LENGTH] = {};

   ch = d->character;
   arg.erase(0, arg.find_first_not_of(' '));

   if (!str_prefix("help", arg.c_str()) &&
       (STATE(d) == conn::OLD_GET_CLASS ||
        STATE(d) == conn::OLD_GET_RACE ||
        STATE(d) == conn::OLD_CHOOSE_STATS ||
        STATE(d) == conn::GET_CLASS ||
        STATE(d) == conn::GET_RACE ||
        STATE(d) == conn::GET_STATS))
   {
      arg.erase(0, 4);
      do_new_help(d->character, arg.data(), 88);
      return;
   }

   switch (STATE(d))
   {

   default:
      log("Nanny: illegal STATE(d)", 0, LogChannels::LOG_BUG);
      close_socket(d);
      return;

   case conn::PRE_DISPLAY_ENTRANCE:
      // _shouldn't_ get here, but if we do, let it fall through to conn::DISPLAY_ENTRANCE
      // This is here to allow the mud to 'skip' this descriptor until the next pulse on
      // a new connection.  That allows the "GET" and "POST" from a webbrowser to get there.
      // no break;

   case conn::DISPLAY_ENTRANCE:

      // Check for people trying to connect to a webserver
      if (arg == "GET" || arg == "POST")
      {
         // send webpage
         SEND_TO_Q(webpage, d);
         close_socket(d);
         return;
      }

      // if it's not a webbrowser, display the entrance greeting
      switch (number(1, 4))
      {
      default:
      case 1:
         SEND_TO_Q(greetings1, d);
         break;

      case 2:
         SEND_TO_Q(greetings2, d);
         break;

      case 3:
         SEND_TO_Q(greetings3, d);
         break;

      case 4:
         SEND_TO_Q(greetings4, d);
         break;
      }

      if (count_IP_connections(d))
         break;

      if (wizlock)
      {
         SEND_TO_Q("The game is currently WIZLOCKED. Only immortals can connect at this time.\r\n", d);
      }
      SEND_TO_Q("What name for the roster? ", d);
      telnet_ga(d);
      STATE(d) = conn::GET_NAME;

      // if they have already entered their name, drop through.  Otherwise stop and wait for input
      if (arg.empty())
      {
         break;
      }
      /* no break */

   case conn::GET_NAME:

      if (arg.empty())
      {
         SEND_TO_Q("Empty name.  Disconnecting...", d);
         close_socket(d);
         return;
      }

      // Capitlize first letter, lowercase rest
      arg[0] = UPPER(arg[0]);
      for (y = 1; arg[y] != '\0'; y++)
         arg[y] = LOWER(arg[y]);

      if (_parse_name(arg.c_str(), tmp_name))
      {
         SEND_TO_Q("Illegal name, try another.\n\rName: ", d);
         telnet_ga(d);
         return;
      }

      if (check_deny(d, tmp_name))
         return;

      // Uncomment this if you think a playerfile may be crashing the mud. -pir
      //      sprintf(str_tmp, "Trying to login: %s", tmp_name);
      //    log(str_tmp, 0, LogChannels::LOG_MISC);

      // ch is allocated in load_char_obj
      fOld = load_char_obj(d, tmp_name);
      if (!fOld)
      {
         str_tmp << "../archive/" << tmp_name << ".gz";
         if (file_exists(str_tmp.str().c_str()))
         {
            SEND_TO_Q("That character is archived.\n\rPlease mail "
                      "imps@dcastle.org to request it be restored.\n\r",
                      d);
            close_socket(d);
            return;
         }
      }
      ch = d->character;

      // This is needed for "check_reconnect"  we free it later during load_char_obj
      // TODO - this is memoryleaking ch->name.  Check if ch->name is not there before
      // doing it to fix it.  (No time to verify this now, so i'll do it later)
      GET_NAME(ch) = str_dup(tmp_name);

      // if (allowed_host(d->host))
      // SEND_TO_Q("You are logging in from an ALLOWED host.\r\n", d);

      if (check_reconnect(d, tmp_name, FALSE))
         fOld = TRUE;
      else if ((wizlock) && !allowed_host(d->host))
      {
         SEND_TO_Q("The game is wizlocked.\n\r", d);
         close_socket(d);
         return;
      }

      if (fOld)
      {
         /* Old player */
         SEND_TO_Q("Password: ", d);
         telnet_ga(d);
         STATE(d) = conn::GET_OLD_PASSWORD;
         return;
      }
      else
      {
         if (DC::getInstance()->cf.bport)
         {
            SEND_TO_Q("New chars not allowed on this port.\r\nEnter a new name: ", d);
            return;
         }
         /* New player */
         sprintf(buf, "Did I get that right, %s (y/n)? ", tmp_name);
         SEND_TO_Q(buf, d);
         telnet_ga(d);
         STATE(d) = conn::CONFIRM_NEW_NAME;
         return;
      }
      break;

   case conn::GET_OLD_PASSWORD:
      SEND_TO_Q("\n\r", d);

      // Default is to authenticate against character password
      password = ch->pcdata->pwd;

      // If -P option passed and one of your other characters is an imp, allow this char with that imp's password
      if (DC::getInstance()->cf.allow_imp_password && allowed_host(d->host))
      {
         for (descriptor_data *ad = descriptor_list; ad && ad != (descriptor_data *)0x95959595; ad = ad->next)
         {
            if (ad != d && !str_cmp(d->host, ad->host))
            {
               if (ad->character && GET_LEVEL(ad->character) == IMP && IS_PC(ad->character))
               {
                  password = ad->character->pcdata->pwd;
                  logf(OVERSEER, LogChannels::LOG_SOCKET, "Using %s's password for authentication.", GET_NAME(ad->character));
                  break;
               }
            }
         }
      }

      if (string(crypt(arg.c_str(), password)) != password)
      {
         SEND_TO_Q("Wrong password.\n\r", d);
         sprintf(log_buf, "%s wrong password: %s", GET_NAME(ch), d->host);
         log(log_buf, OVERSEER, LogChannels::LOG_SOCKET);
         if ((ch = get_pc(GET_NAME(d->character))))
         {
            sprintf(log_buf, "$4$BWARNING: Someone just tried to log in as you with the wrong password.\r\n"
                             //           "Attempt was from %s.$R\r\n"
                             "(If it's only once or twice, you can ignore it.  If it's several dozen tries, let a god know.)\r\n");
            send_to_char(log_buf, ch);
         }
         else
         {
            if (d->character->pcdata->bad_pw_tries > 100)
            {
               sprintf(log_buf, "%s has 100+ bad pw tries...", GET_NAME(d->character));
               log(log_buf, SERAPH, LogChannels::LOG_SOCKET);
            }
            else
            {
               d->character->pcdata->bad_pw_tries++;
               save_char_obj(d->character);
            }
         }
         close_socket(d);
         return;
      }

      check_playing(d, GET_NAME(ch));

      if (check_reconnect(d, GET_NAME(ch), TRUE))
         return;

      sprintf(log_buf, "%s@%s has connected.", GET_NAME(ch), d->host);
      if (GET_LEVEL(ch) < ANGEL)
         log(log_buf, OVERSEER, LogChannels::LOG_SOCKET);
      else
         log(log_buf, GET_LEVEL(ch), LogChannels::LOG_SOCKET);

      warn_if_duplicate_ip(ch);
      //    SEND_TO_Q(motd, d);
      if (GET_LEVEL(ch) < IMMORTAL)
         send_to_char(motd, d->character);
      else
         send_to_char(imotd, d->character);

      clan_data *clan;
      if ((clan = get_clan(ch->clan)) && clan->clanmotd)
      {
         send_to_char("$B----------------------------------------------------------------------$R\r\n", ch);
         send_to_char(clan->clanmotd, ch);
         send_to_char("$B----------------------------------------------------------------------$R\r\n", ch);
      }

      sprintf(log_buf, "\n\rIf you have read this motd, press Return."
                       "\n\rLast connected from:\n\r%s\n\r",
              ch->pcdata->last_site);
      SEND_TO_Q(log_buf, d);
      telnet_ga(d);

      if (d->character->pcdata->bad_pw_tries)
      {
         sprintf(buf, "\r\n\r\n$4$BYou have had %d wrong passwords entered since your last complete login.$R\r\n\r\n", d->character->pcdata->bad_pw_tries);
         SEND_TO_Q(buf, d);
      }
      check_for_sold_items(d->character);
      STATE(d) = conn::READ_MOTD;
      break;

   case conn::CONFIRM_NEW_NAME:
      if (arg.empty())
      {
         SEND_TO_Q("Please type y or n: ", d);
         telnet_ga(d);
         break;
      }

      switch (arg[0])
      {
      case 'y':
      case 'Y':

         if (isbanned(d->host) >= BAN_NEW)
         {
            sprintf(buf, "Request for new character %s denied from [%s] (siteban)",
                    GET_NAME(d->character), d->host);
            log(buf, OVERSEER, LogChannels::LOG_SOCKET);
            SEND_TO_Q("Sorry, new chars are not allowed from your site.\n\r"
                      "Questions may be directed to imps@dcastle.org\n\r",
                      d);
            STATE(d) = conn::CLOSE;
            return;
         }
         sprintf(buf, "New character.\n\rGive me a password for %s: ", GET_NAME(ch));
         SEND_TO_Q(buf, d);
         telnet_ga(d);
         STATE(d) = conn::GET_NEW_PASSWORD;
         // at this point, pcdata hasn't yet been created.  So we're going to go ahead and
         // allocate it since a new character is obviously a PC
#ifdef LEAK_CHECK
         ch->pcdata = (pc_data *)calloc(1, sizeof(pc_data));
#else
         ch->pcdata = (pc_data *)dc_alloc(1, sizeof(pc_data));
#endif
         break;

      case 'n':
      case 'N':
         SEND_TO_Q("Ok, what IS it, then? ", d);
         telnet_ga(d);
         // TODO - double check this to make sure we're free'ing properly
         delete GET_NAME(ch);
         GET_NAME(ch) = NULL;
         delete d->character;
         d->character = NULL;
         STATE(d) = conn::GET_NAME;
         break;

      default:
         SEND_TO_Q("Please type y or n: ", d);
         telnet_ga(d);
         break;
      }
      break;

   case conn::GET_NEW_PASSWORD:
      SEND_TO_Q("\r\n", d);

      if (arg.length() < 6)
      {
         SEND_TO_Q("Password must be at least six characters long.\n\rPassword: ", d);
         telnet_ga(d);
         return;
      }

      strncpy(ch->pcdata->pwd, crypt(arg.c_str(), ch->name), PASSWORD_LEN);
      ch->pcdata->pwd[PASSWORD_LEN] = '\0';
      SEND_TO_Q("Please retype password: ", d);
      telnet_ga(d);
      STATE(d) = conn::CONFIRM_NEW_PASSWORD;
      break;

   case conn::CONFIRM_NEW_PASSWORD:
      SEND_TO_Q("\n\r", d);

      if (string(crypt(arg.c_str(), ch->pcdata->pwd)) != ch->pcdata->pwd)
      {
         SEND_TO_Q("Passwords don't match.\n\rRetype password: ", d);
         telnet_ga(d);
         STATE(d) = conn::GET_NEW_PASSWORD;
         return;
      }

   case conn::QUESTION_ANSI:
      SEND_TO_Q("Do you want ANSI color (y/n)? ", d);
      telnet_ga(d);
      STATE(d) = conn::GET_ANSI;
      break;

   case conn::GET_ANSI:
      if (arg.empty())
      {
         STATE(d) = conn::QUESTION_ANSI;
         return;
      }

      switch (arg[0])
      {
      case 'y':
      case 'Y':
         d->color = true;
         break;
      case 'n':
      case 'N':
         d->color = false;
         break;
      default:
         STATE(d) = conn::QUESTION_ANSI;
         return;
      }

      STATE(d) = conn::QUESTION_SEX;
      break;

   case conn::QUESTION_SEX:
      SEND_TO_Q("What is your sex (m/f)? ", d);
      telnet_ga(d);
      STATE(d) = conn::GET_NEW_SEX;
      break;

   case conn::GET_NEW_SEX:
      if (arg.empty())
      {
         SEND_TO_Q("That's not a sex.\r\n", d);
         STATE(d) = conn::QUESTION_SEX;
         break;
      }

      switch (arg[0])
      {
      case 'm':
      case 'M':
         ch->sex = SEX_MALE;
         break;
      case 'f':
      case 'F':
         ch->sex = SEX_FEMALE;
         break;
      default:
         SEND_TO_Q("That's not a sex.\r\n", d);
         STATE(d) = conn::QUESTION_SEX;
         return;
      }

      /*
            if (!allowed_host(d->host) && DC::getInstance()->cf.allow_newstatsys == false)
            {
               STATE(d) = conn::OLD_STAT_METHOD;
               break;
            }
      */

      SEND_TO_Q("\r\n", d);
      SEND_TO_Q("$R$7Note: If you see a word that is entirely CAPITALIZED and $Bbold$R then there\r\n", d);
      SEND_TO_Q("exists a helpfile for that word which you can lookup with the command\r\n", d);
      SEND_TO_Q("help keyword.\r\n", d);
      SEND_TO_Q("\r\n", d);
      SEND_TO_Q("Dark Castle supports two ways of picking your race, class and initial\r\n", d);
      SEND_TO_Q("$BATTRIBUTES$R such as $BSTRENGTH$R, $BDEXTERITY$R, $BCONSTITUION$R, $BINTELLIGENCE$R\r\n", d);
      SEND_TO_Q("or $BWISDOM$R.\r\n\r\n", d);
      SEND_TO_Q("The newest method is you first pick a race, a class then you get 12 points\r\n", d);
      SEND_TO_Q("in each attribute and 23 points you can divide among those five\r\n", d);
      SEND_TO_Q("attributes.\r\n\r\n", d);
      SEND_TO_Q("The old method is you roll virtual dice. The virtual dice consist of either\r\n", d);
      SEND_TO_Q("three 6-sided dice or six 3-sided dice. The game picks the larger group of\r\n", d);
      SEND_TO_Q("those two separate sets of dice rolls and throws out any sets with a sum less\r\n", d);
      SEND_TO_Q("than 12. You can roll virtual dice as many times as you like. After you finish\r\n", d);
      SEND_TO_Q("rolling dice then the game shows you what races you can pick based on the dice\r\n", d);
      SEND_TO_Q("rolls. After picking from the races your dice rolls allow then you have to\r\n", d);
      SEND_TO_Q("pick a class that fits your choosen stats.\r\n", d);
      STATE(d) = conn::QUESTION_STAT_METHOD;
      return;

   case conn::QUESTION_STAT_METHOD:
      SEND_TO_Q("\r\n", d);
      SEND_TO_Q("1. Pick race, class then assign points to attributes. (new method)\r\n", d);
      SEND_TO_Q("2. Roll virtual dice for attributes then pick race and class. (old method)\r\n", d);
      SEND_TO_Q("What is your choice (1,2)? ", d);
      telnet_ga(d);
      STATE(d) = conn::GET_STAT_METHOD;
      break;

   case conn::GET_STAT_METHOD:
      try
      {
         selection = stoul(arg);
      }
      catch (...)
      {
         selection = 0;
      }

      if (selection == 1)
      {
         STATE(d) = conn::NEW_STAT_METHOD;
      }
      else if (selection == 2)
      {
         STATE(d) = conn::OLD_STAT_METHOD;
      }
      else
      {
         STATE(d) = conn::QUESTION_STAT_METHOD;
      }
      break;

   case conn::NEW_STAT_METHOD:
      STATE(d) = conn::QUESTION_RACE;
      break;

   case conn::QUESTION_RACE:
      show_question_race(d);

      STATE(d) = conn::GET_RACE;
      break;

   case conn::GET_RACE:
      if (handle_get_race(d, arg) == true)
      {
         STATE(d) = conn::QUESTION_CLASS;
      }
      else
      {
         STATE(d) = conn::QUESTION_RACE;
      }
      break;

   case conn::QUESTION_CLASS:
      show_question_class(d);

      STATE(d) = conn::GET_CLASS;
      break;

   case conn::GET_CLASS:
      if (handle_get_class(d, arg) == false)
      {
         if (STATE(d) != conn::QUESTION_RACE)
         {
            STATE(d) = conn::QUESTION_CLASS;
         }
      }
      else
      {
         STATE(d) = conn::QUESTION_STATS;
      }
      break;

   case conn::QUESTION_STATS:
      show_question_stats(d);

      STATE(d) = conn::GET_STATS;
      break;

   case conn::GET_STATS:
      if (handle_get_stats(d, arg) == false)
      {
         STATE(d) = conn::QUESTION_STATS;
      }
      else
      {
         STATE(d) = conn::NEW_PLAYER;
      }
      break;

   case conn::OLD_STAT_METHOD:
      if (ch->desc->stats != nullptr)
      {
         delete ch->desc->stats;
      }
      ch->desc->stats = new stat_data;

      STATE(d) = conn::OLD_CHOOSE_STATS;
      arg.clear();
      // break;  no break on purpose...might as well do this now.  no point in waiting

   case conn::OLD_CHOOSE_STATS:

      if (arg.length() > 1)
      {
         arg[1] = '\0';
      }

      // first time, this has the m/f from sex and goes right through

      try
      {
         selection = stoul(arg);
      }
      catch (...)
      {
         selection = 0;
      }

      if (selection >= 1 && selection <= 5)
      {
         GET_RAW_STR(ch) = ch->desc->stats->str[selection - 1];
         GET_RAW_INT(ch) = ch->desc->stats->tel[selection - 1];
         GET_RAW_WIS(ch) = ch->desc->stats->wis[selection - 1];
         GET_RAW_DEX(ch) = ch->desc->stats->dex[selection - 1];
         GET_RAW_CON(ch) = ch->desc->stats->con[selection - 1];
         delete ch->desc->stats;
         ch->desc->stats = nullptr;
         SEND_TO_Q("\n\rChoose a race(races you can select are marked with a *).\n\r", d);
         sprintf(buf, "  %c1: Human\n\r  %c2: Elf\n\r  %c3: Dwarf\n\r"
                      "  %c4: Hobbit\n\r  %c5: Pixie\n\r  %c6: Ogre\n\r"
                      "  %c7: Gnome\r\n  %c8: Orc\r\n  %c9: Troll\r\n"
                      "\n\rSelect a race(Type help <race> for more information)-> ",
                 is_race_eligible(ch, 1) ? '*' : ' ', is_race_eligible(ch, 2) ? '*' : ' ', is_race_eligible(ch, 3) ? '*' : ' ', is_race_eligible(ch, 4) ? '*' : ' ', is_race_eligible(ch, 5) ? '*' : ' ', is_race_eligible(ch, 6) ? '*' : ' ',
                 is_race_eligible(ch, 7) ? '*' : ' ', is_race_eligible(ch, 8) ? '*' : ' ', is_race_eligible(ch, 9) ? '*' : ' ');

         SEND_TO_Q(buf, d);
         telnet_ga(d);
         STATE(d) = conn::OLD_GET_RACE;
         break;
      }
      roll_and_display_stats(ch);
      break;

   case conn::OLD_GET_RACE:
      try
      {
         selection = stoul(arg);
      }
      catch (...)
      {
         selection = 0;
      }

      switch (selection)
      {
      default:
         SEND_TO_Q("That's not a race.\n\rWhat IS your race? ", d);
         telnet_ga(d);
         return;

      case 1:
         ch->race = RACE_HUMAN;
         GET_RAW_STR(ch) += RACE_HUMAN_STR_MOD;
         GET_RAW_INT(ch) += RACE_HUMAN_INT_MOD;
         GET_RAW_WIS(ch) += RACE_HUMAN_WIS_MOD;
         GET_RAW_DEX(ch) += RACE_HUMAN_DEX_MOD;
         GET_RAW_CON(ch) += RACE_HUMAN_CON_MOD;
         break;

      case 2:
         if (GET_RAW_DEX(ch) < 10 || GET_RAW_INT(ch) < 10)
         {
            send_to_char("Your stats do not qualify for that race.\r\n", ch);
            return;
         }
         ch->race = RACE_ELVEN;
         ch->alignment = 1000;
         GET_RAW_STR(ch) += RACE_ELVEN_STR_MOD;
         GET_RAW_INT(ch) += RACE_ELVEN_INT_MOD;
         GET_RAW_WIS(ch) += RACE_ELVEN_WIS_MOD;
         GET_RAW_DEX(ch) += RACE_ELVEN_DEX_MOD;
         GET_RAW_CON(ch) += RACE_ELVEN_CON_MOD;
         break;

      case 3:
         if (GET_RAW_CON(ch) < 10 || GET_RAW_WIS(ch) < 10)
         {
            send_to_char("Your stats do not qualify for that race.\r\n", ch);
            return;
         }
         ch->race = RACE_DWARVEN;
         GET_RAW_STR(ch) += RACE_DWARVEN_STR_MOD;
         GET_RAW_INT(ch) += RACE_DWARVEN_INT_MOD;
         GET_RAW_WIS(ch) += RACE_DWARVEN_WIS_MOD;
         GET_RAW_DEX(ch) += RACE_DWARVEN_DEX_MOD;
         GET_RAW_CON(ch) += RACE_DWARVEN_CON_MOD;
         break;

      case 4:
         if (GET_RAW_DEX(ch) < 10)
         {
            send_to_char("Your stats do not qualify for that race.\r\n", ch);
            return;
         }
         ch->race = RACE_HOBBIT;
         GET_RAW_STR(ch) += RACE_HOBBIT_STR_MOD;
         GET_RAW_INT(ch) += RACE_HOBBIT_INT_MOD;
         GET_RAW_WIS(ch) += RACE_HOBBIT_WIS_MOD;
         GET_RAW_DEX(ch) += RACE_HOBBIT_DEX_MOD;
         GET_RAW_CON(ch) += RACE_HOBBIT_CON_MOD;
         break;

      case 5:
         if (GET_RAW_INT(ch) < 12)
         {
            send_to_char("Your stats do not qualify for that race.\r\n", ch);
            return;
         }
         ch->race = RACE_PIXIE;
         GET_RAW_STR(ch) += RACE_PIXIE_STR_MOD;
         GET_RAW_INT(ch) += RACE_PIXIE_INT_MOD;
         GET_RAW_WIS(ch) += RACE_PIXIE_WIS_MOD;
         GET_RAW_DEX(ch) += RACE_PIXIE_DEX_MOD;
         GET_RAW_CON(ch) += RACE_PIXIE_CON_MOD;
         break;

      case 6:
         if (GET_RAW_STR(ch) < 12)
         {
            send_to_char("Your stats do not qualify for that race.\r\n", ch);
            return;
         }
         ch->race = RACE_GIANT;
         GET_RAW_STR(ch) += RACE_GIANT_STR_MOD;
         GET_RAW_INT(ch) += RACE_GIANT_INT_MOD;
         GET_RAW_WIS(ch) += RACE_GIANT_WIS_MOD;
         GET_RAW_DEX(ch) += RACE_GIANT_DEX_MOD;
         GET_RAW_CON(ch) += RACE_GIANT_CON_MOD;
         break;

      case 7:
         if (GET_RAW_WIS(ch) < 12)
         {
            send_to_char("Your stats do not qualify for that race.\r\n", ch);
            return;
         }
         ch->race = RACE_GNOME;
         GET_RAW_STR(ch) += RACE_GNOME_STR_MOD;
         GET_RAW_INT(ch) += RACE_GNOME_INT_MOD;
         GET_RAW_WIS(ch) += RACE_GNOME_WIS_MOD;
         GET_RAW_DEX(ch) += RACE_GNOME_DEX_MOD;
         GET_RAW_CON(ch) += RACE_GNOME_CON_MOD;
         break;

      case 8:
         if (GET_RAW_CON(ch) < 10 || GET_RAW_STR(ch) < 10)
         {
            send_to_char("Your stats do not qualify for that race.\r\n", ch);
            return;
         }

         ch->race = RACE_ORC;
         ch->alignment = -1000;
         GET_RAW_STR(ch) += RACE_ORC_STR_MOD;
         GET_RAW_INT(ch) += RACE_ORC_INT_MOD;
         GET_RAW_WIS(ch) += RACE_ORC_WIS_MOD;
         GET_RAW_DEX(ch) += RACE_ORC_DEX_MOD;
         GET_RAW_CON(ch) += RACE_ORC_CON_MOD;
         break;
      case 9:
         if (GET_RAW_CON(ch) < 12)
         {
            send_to_char("Your stats do not qualify for that race.\r\n", ch);
            return;
         }
         ch->race = RACE_TROLL;
         ch->alignment = 0;
         GET_RAW_STR(ch) += RACE_TROLL_STR_MOD;
         GET_RAW_INT(ch) += RACE_TROLL_INT_MOD;
         GET_RAW_WIS(ch) += RACE_TROLL_WIS_MOD;
         GET_RAW_DEX(ch) += RACE_TROLL_DEX_MOD;
         GET_RAW_CON(ch) += RACE_TROLL_CON_MOD;
         break;
      }

      set_hw(ch);
      SEND_TO_Q("\n\rA '*' denotes a class that fits your chosen stats.\n\r", d);
      sprintf(buf, " %c 1: Warrior\n\r"
                   " %c 2: Cleric\n\r"
                   " %c 3: Mage\n\r"
                   " %c 4: Thief\n\r"
                   " %c 5: Anti-Paladin\n\r"
                   " %c 6: Paladin\n\r"
                   " %c 7: Barbarian \n\r"
                   " %c 8: Monk\n\r"
                   " %c 9: Ranger\n\r"
                   " %c 10: Bard\n\r"
                   " %c 11: Druid\n\r"
                   "\n\rSelect a class(Type help <class> for more information) > ",
              //           (is_clss_eligible(ch, CLASS_WARRIOR) ? '*' : ' '),
              '*',
              (is_clss_eligible(ch, CLASS_CLERIC) ? '*' : ' '),
              (is_clss_eligible(ch, CLASS_MAGIC_USER) ? '*' : ' '),
              (is_clss_eligible(ch, CLASS_THIEF) ? '*' : ' '),
              (is_clss_eligible(ch, CLASS_ANTI_PAL) ? '*' : ' '),
              (is_clss_eligible(ch, CLASS_PALADIN) ? '*' : ' '),
              (is_clss_eligible(ch, CLASS_BARBARIAN) ? '*' : ' '),
              (is_clss_eligible(ch, CLASS_MONK) ? '*' : ' '),
              (is_clss_eligible(ch, CLASS_RANGER) ? '*' : ' '),
              //            '*',
              (is_clss_eligible(ch, CLASS_BARD) ? '*' : ' '),
              (is_clss_eligible(ch, CLASS_DRUID) ? '*' : ' '));
      SEND_TO_Q(buf, d);
      telnet_ga(d);
      STATE(d) = conn::OLD_GET_CLASS;
      break;

   case conn::OLD_GET_CLASS:
      try
      {
         selection = stoul(arg);
      }
      catch (...)
      {
         selection = 0;
      }

      switch (selection)
      {
      default:
         SEND_TO_Q("That's not a class.\n\rWhat IS your class? ", d);
         telnet_ga(d);
         return;

      case 1:
         //          if(!is_clss_eligible(ch, CLASS_WARRIOR))
         //         { SEND_TO_Q(badclssmsg, d);  return; }
         GET_CLASS(ch) = CLASS_WARRIOR;
         break;
      case 2:
         if (!is_clss_eligible(ch, CLASS_CLERIC))
         {
            SEND_TO_Q(badclssmsg, d);
            return;
         }
         GET_CLASS(ch) = CLASS_CLERIC;
         break;
      case 3:
         if (!is_clss_eligible(ch, CLASS_MAGIC_USER))
         {
            SEND_TO_Q(badclssmsg, d);
            return;
         }
         GET_CLASS(ch) = CLASS_MAGIC_USER;
         break;
      case 4:
         if (!is_clss_eligible(ch, CLASS_THIEF))
         {
            SEND_TO_Q(badclssmsg, d);
            return;
         }
         SEND_TO_Q("Pstealing is legal on this mud :)\n\r"
                   "Check 'help psteal' before you try though.",
                   d);
         GET_CLASS(ch) = CLASS_THIEF;
         break;
      case 5:
         if (!is_clss_eligible(ch, CLASS_ANTI_PAL))
         {
            SEND_TO_Q(badclssmsg, d);
            return;
         }
         GET_CLASS(ch) = CLASS_ANTI_PAL;
         GET_ALIGNMENT(ch) = -1000;
         break;
      case 6:
         if (!is_clss_eligible(ch, CLASS_PALADIN))
         {
            SEND_TO_Q(badclssmsg, d);
            return;
         }
         GET_CLASS(ch) = CLASS_PALADIN;
         GET_ALIGNMENT(ch) = 1000;
         break;
      case 7:
         if (!is_clss_eligible(ch, CLASS_BARBARIAN))
         {
            SEND_TO_Q(badclssmsg, d);
            return;
         }
         GET_CLASS(ch) = CLASS_BARBARIAN;
         break;
      case 8:
         if (!is_clss_eligible(ch, CLASS_MONK))
         {
            SEND_TO_Q(badclssmsg, d);
            return;
         }
         GET_CLASS(ch) = CLASS_MONK;
         break;
      case 9:
         if (!is_clss_eligible(ch, CLASS_RANGER))
         {
            SEND_TO_Q(badclssmsg, d);
            return;
         }
         GET_CLASS(ch) = CLASS_RANGER;
         break;
      case 10:
         if (!is_clss_eligible(ch, CLASS_BARD))
         {
            SEND_TO_Q(badclssmsg, d);
            return;
         }
         GET_CLASS(ch) = CLASS_BARD;
         break;
      case 11:
         if (!is_clss_eligible(ch, CLASS_DRUID))
         {
            SEND_TO_Q(badclssmsg, d);
            return;
         }
         GET_CLASS(ch) = CLASS_DRUID;
         break;
      }
      STATE(d) = conn::NEW_PLAYER;
      break;

   case conn::NEW_PLAYER:

      init_char(ch);

      sprintf(log_buf, "%s@%s new player.", GET_NAME(ch), d->host);
      log(log_buf, OVERSEER, LogChannels::LOG_SOCKET);
      SEND_TO_Q("\n\r", d);
      SEND_TO_Q(motd, d);
      SEND_TO_Q("\n\rIf you have read this motd, press Return.", d);
      telnet_ga(d);

      STATE(d) = conn::READ_MOTD;
      break;

   case conn::READ_MOTD:
      SEND_TO_Q(menu, d);
      telnet_ga(d);
      STATE(d) = conn::SELECT_MENU;
      break;

   case conn::SELECT_MENU:
      if (arg.empty())
      {
         SEND_TO_Q(menu, d);
         telnet_ga(d);
         break;
      }

      switch (arg[0])
      {
      case '0':
         close_socket(d);
         d = NULL;
         break;

      case '1':
         // I believe this is here to stop a dupe bug
         // by logging in twice, and leaving one at the password: prompt
         if (GET_LEVEL(ch) > 0)
         {
            strcpy(tmp_name, GET_NAME(ch));
            free_char(d->character, Trace("nanny conn::SELECT_MENU 1"));
            d->character = 0;
            load_char_obj(d, tmp_name);
            ch = d->character;
            if (!ch)
            {
               write_to_descriptor(d->descriptor, "It seems your character has been deleted during logon, or you just experienced some obscure bug.");
               close_socket(d);
               d = NULL;
               break;
            }
         }
         unique_scan(ch);
         if (GET_GOLD(ch) > 1000000000)
         {
            sprintf(log_buf, "%s has more than a billion gold. Bugged?", GET_NAME(ch));
            log(log_buf, 100, LogChannels::LOG_WARNINGS);
         }
         if (GET_BANK(ch) > 1000000000)
         {
            sprintf(log_buf, "%s has more than a billion gold in the bank. Rich fucker or bugged.", GET_NAME(ch));
            log(log_buf, 100, LogChannels::LOG_WARNINGS);
         }
         send_to_char("\n\rWelcome to Dark Castle.\r\n", ch);
         character_list.insert(ch);

         if (IS_AFFECTED(ch, AFF_ITEM_REMOVE))
         {
            REMBIT(ch->affected_by, AFF_ITEM_REMOVE);
            send_to_char("\r\n$I$B$4***WARNING*** Items you were previously wearing have been moved to your inventory, please check before moving out of a safe room.$R\r\n", ch);
         }

         do_on_login_stuff(ch);

         if (GET_LEVEL(ch) < OVERSEER)
            clan_login(ch);

         act("$n has entered the game.", ch, 0, 0, TO_ROOM, INVIS_NULL);
         if (!GET_SHORT_ONLY(ch))
            GET_SHORT_ONLY(ch) = str_dup(GET_NAME(ch));
         update_wizlist(ch);
         check_maxes(ch); // Check skill maxes.

         STATE(d) = conn::PLAYING;
         update_max_who();

         if (GET_LEVEL(ch) == 0)
         {
            do_start(ch);
            do_new_help(ch, "new", 0);
         }
         do_look(ch, "", 8);
         {
            if (GET_LEVEL(ch) >= 40 && DCVote->IsActive() && !DCVote->HasVoted(ch))
            {
               send_to_char("\n\rThere is an active vote in which you have not yet voted.\n\r"
                            "Enter \"vote\" to see details\n\r\n\r",
                            ch);
            }
         }
         extern void zap_eq_check(char_data * ch);
         zap_eq_check(ch);
         break;

      case '2':
         SEND_TO_Q("Enter a text you'd like others to see when they look at you.\n\r"
                   "Terminate with '/s' on a new line.\n\r",
                   d);
         if (ch->description)
         {
            SEND_TO_Q("Old description:\n\r", d);
            SEND_TO_Q(ch->description, d);
            dc_free(ch->description);
         }
#ifdef LEAK_CHECK
         ch->description = (char *)calloc(540, sizeof(char));
#else
         ch->description = (char *)dc_alloc(540, sizeof(char));
#endif

         // TODO - what happens if I get to this point, then disconnect, and reconnect?  memory leak?

         d->strnew = &ch->description;
         d->max_str = 539;
         STATE(d) = conn::EXDSCR;
         break;

      case '3':
         SEND_TO_Q("Enter current password: ", d);
         telnet_ga(d);
         STATE(d) = conn::CONFIRM_PASSWORD_CHANGE;
         break;

      case '4':
         // Archive the charater
         SEND_TO_Q("This will archive your character until you ask to be unarchived.\n\rYou will need to speak to a god greater than level 107.\n\rType ARCHIVE ME if this is what you want:  ", d);
         telnet_ga(d);
         STATE(d) = conn::ARCHIVE_CHAR;
         break;

      case '5':
         // delete this character
         SEND_TO_Q("This will _permanently_ erase you.\n\rType ERASE ME if this is really what you want: ", d);
         telnet_ga(d);
         STATE(d) = conn::DELETE_CHAR;
         break;

      default:
         SEND_TO_Q(menu, d);
         telnet_ga(d);
         break;
      }
      break;

   case conn::ARCHIVE_CHAR:
      if (arg == "ARCHIVE ME")
      {
         str_tmp << GET_NAME(d->character);
         SEND_TO_Q("\n\rCharacter Archived.\n\r", d);
         update_wizlist(d->character);
         close_socket(d);
         util_archive(str_tmp.str().c_str(), 0);
      }
      else
      {
         STATE(d) = conn::SELECT_MENU;
         SEND_TO_Q(menu, d);
      }
      break;

   case conn::DELETE_CHAR:
      if (arg == "ERASE ME")
      {
         sprintf(buf, "%s just deleted themself.", d->character->name);
         log(buf, IMMORTAL, LogChannels::LOG_MORTAL);

         AuctionHandleDelete(d->character->name);
         // To remove the vault from memory
         remove_familiars(d->character->name, SELFDELETED);
         remove_vault(d->character->name, SELFDELETED);
         if (d->character->clan)
         {
            remove_clan_member(d->character->clan, d->character);
         }
         remove_character(d->character->name, SELFDELETED);

         GET_LEVEL(d->character) = 1;
         update_wizlist(d->character);
         close_socket(d);
         d = NULL;
      }
      else
      {
         STATE(d) = conn::SELECT_MENU;
         SEND_TO_Q(menu, d);
         telnet_ga(d);
      }
      break;

   case conn::CONFIRM_PASSWORD_CHANGE:
      SEND_TO_Q("\n\r", d);
      if (string(crypt(arg.c_str(), ch->pcdata->pwd)) == ch->pcdata->pwd)
      {
         SEND_TO_Q("Enter a new password: ", d);
         telnet_ga(d);
         STATE(d) = conn::RESET_PASSWORD;
         break;
      }
      else
      {
         SEND_TO_Q("Incorrect.", d);
         STATE(d) = conn::SELECT_MENU;
         SEND_TO_Q(menu, d);
      }
      break;

   case conn::RESET_PASSWORD:
      SEND_TO_Q("\n\r", d);

      if (arg.length() < 6)
      {
         SEND_TO_Q("Password must be at least six characters long.\n\rPassword: ", d);
         telnet_ga(d);
         return;
      }
      strncpy(ch->pcdata->pwd, crypt(arg.c_str(), ch->name), PASSWORD_LEN);
      ch->pcdata->pwd[PASSWORD_LEN] = '\0';
      SEND_TO_Q("Please retype password: ", d);
      telnet_ga(d);
      STATE(d) = conn::CONFIRM_RESET_PASSWORD;
      break;

   case conn::CONFIRM_RESET_PASSWORD:
      SEND_TO_Q("\n\r", d);

      if (string(crypt(arg.c_str(), ch->pcdata->pwd)) != ch->pcdata->pwd)
      {
         SEND_TO_Q("Passwords don't match.\n\rRetype password: ", d);
         telnet_ga(d);
         STATE(d) = conn::RESET_PASSWORD;
         return;
      }

      SEND_TO_Q("\n\rDone.\n\r", d);
      SEND_TO_Q(menu, d);
      STATE(d) = conn::SELECT_MENU;
      if (GET_LEVEL(ch) > 1)
      {
         char blah1[50], blah2[50];
         // this prevents a dupe bug
         strcpy(blah1, GET_NAME(ch));
         strcpy(blah2, ch->pcdata->pwd);
         free_char(d->character, Trace("nanny conn::CONFIRM_RESET_PASSWORD"));
         d->character = 0;
         load_char_obj(d, blah1);
         ch = d->character;
         strcpy(ch->pcdata->pwd, blah2);
         save_char_obj(ch);
         sprintf(log_buf, "%s password changed", GET_NAME(ch));
         log(log_buf, SERAPH, LogChannels::LOG_SOCKET);
      }

      break;
   case conn::CLOSE:
      close_socket(d);
      break;
   }
}

// This is mostly just to keep people from putting meta-chars
// into their email address.
int _parse_email(char *arg)
{
   if (strlen(arg) < 4)
      return 0;

   for (; *arg != '\0'; arg++)
      if (!isalnum(*arg) && *arg != '@' && *arg != '.' &&
          *arg != '_' && *arg != '-')
         return 0;

   return 1;
}

// Parse a name for acceptability.
int _parse_name(const char *arg, char *name)
{
   int i;

   /* skip whitespaces */
   for (; isspace(*arg); arg++)
      ;

   for (i = 0; (name[i] = arg[i]) != '\0'; i++)
   {
      if (name[i] < 0 || !isalpha(name[i]) || i >= MAX_NAME_LENGTH)
         return 1;
   }

   if (i < 3)
      return 1;

   if (on_forbidden_name_list(name))
      return 1;

   return 0;
}

// Check for denial of service.
bool check_deny(struct descriptor_data *d, char *name)
{
   FILE *fpdeny = NULL;
   char strdeny[MAX_INPUT_LENGTH];
   char bufdeny[MAX_STRING_LENGTH];

   sprintf(strdeny, "%s/%c/%s.deny", SAVE_DIR, UPPER(name[0]), name);
   if ((fpdeny = dc_fopen(strdeny, "rb")) == NULL)
      return FALSE;
   dc_fclose(fpdeny);

   char log_buf[MAX_STRING_LENGTH] = {};
   sprintf(log_buf, "Denying access to player %s@%s.", name, d->host);
   log(log_buf, ARCHANGEL, LogChannels::LOG_MORTAL);
   file_to_string(strdeny, bufdeny);
   SEND_TO_Q(bufdeny, d);
   close_socket(d);
   return TRUE;
}

// Look for link-dead player to reconnect.
bool check_reconnect(struct descriptor_data *d, char *name, bool fReconnect)
{
   auto &character_list = DC::getInstance()->character_list;
   for (auto &tmp_ch : character_list)
   {
      if (IS_NPC(tmp_ch) || tmp_ch->desc != NULL)
         continue;

      if (str_cmp(GET_NAME(d->character), GET_NAME(tmp_ch)))
         continue;

      if (GET_LEVEL(d->character) < 2)
         continue;

      //      if(fReconnect == FALSE)
      //      {
      // TODO - why are we doing this?  we load the password doing load_char_obj
      // unless someone changed their password and didn't save this doesn't seem useful
      // removed 8/29/02..i think this might be related to the bug causing people
      // to morph into other people
      // if(d->character->pcdata)
      //  strncpy( d->character->pcdata->pwd, tmp_ch->pcdata->pwd, PASSWORD_LEN );
      //      }
      //      else {

      if (fReconnect == TRUE)
      {
         free_char(d->character, Trace("check_reconnect"));
         d->character = tmp_ch;
         tmp_ch->desc = d;
         tmp_ch->timer = 0;
         send_to_char("Reconnecting.\n\r", tmp_ch);
         char log_buf[MAX_STRING_LENGTH] = {};
         sprintf(log_buf, "%s@%s has reconnected.",
                 GET_NAME(tmp_ch), d->host);
         act("$n has reconnected and is ready to kick ass.", tmp_ch, 0,
             0, TO_ROOM, INVIS_NULL);

         if (GET_LEVEL(tmp_ch) < IMMORTAL)
         {
            log(log_buf, COORDINATOR, LogChannels::LOG_SOCKET);
         }
         else
         {
            log(log_buf, GET_LEVEL(tmp_ch), LogChannels::LOG_SOCKET);
         }

         STATE(d) = conn::PLAYING;
      }
      return TRUE;
   }
   return FALSE;
}

/*
 * Check if already playing (on an open descriptor.)
 */
bool check_playing(struct descriptor_data *d, char *name)
{
   struct descriptor_data *dold, *next_d;
   char_data *compare = 0;

   for (dold = descriptor_list; dold; dold = next_d)
   {
      next_d = dold->next;

      if ((dold == d) || (dold->character == 0))
         continue;

      compare = ((dold->original != 0) ? dold->original : dold->character);

      // If this is the case we fail our precondition to str_cmp
      if (name == 0)
         continue;
      if (GET_NAME(compare) == 0)
         continue;

      if (str_cmp(name, GET_NAME(compare)))
         continue;

      if (STATE(dold) == conn::GET_NAME)
         continue;

      if (STATE(dold) == conn::GET_OLD_PASSWORD)
      {
         free_char(dold->character, Trace("check_playing"));
         dold->character = 0;
         close_socket(dold);
         continue;
      }
      close_socket(dold);
      return 0;
   }
   return 0;
}

char *str_str(char *first, char *second)
{
   char *pstr;

   for (pstr = first; *pstr; pstr++)
      *pstr = LOWER(*pstr);

   for (pstr = second; *pstr; pstr++)
      *pstr = LOWER(*pstr);

   pstr = strstr(first, second);

   return pstr;
}

void short_activity()
{ // handles short activity. at the moment, only archery.
   DC::getInstance()->handleShooting();
}

// these are for my special lag that only keeps you from doing certain
// commands, while still allowing other.
void add_command_lag(char_data *ch, int amount)
{
   if (GET_LEVEL(ch) < IMMORTAL)
      ch->timer += amount;
}

int check_command_lag(char_data *ch)
{
   return ch->timer;
}

void remove_command_lag(char_data *ch)
{
   ch->timer = 0;
}

void update_characters()
{
   int tmp, retval;
   char log_msg[MAX_STRING_LENGTH], dammsg[MAX_STRING_LENGTH];
   struct affected_type af;

   auto &character_list = DC::getInstance()->character_list;
   for (auto &i : character_list)
   {

      if (i->brace_at) // if character is bracing
      {
         if (!charge_moves(i, SKILL_BATTERBRACE, 0.5) || !is_bracing(i, i->brace_at))
         {
            do_brace(i, "", 0);
         }
         else
         {
            csendf(i, "You strain your muscles keeping the %s closed.\r\n", fname(i->brace_at->keyword));
            act("$n strains $s muscles keeping the $F blocked.", i, 0, i->brace_at->keyword, TO_ROOM, 0);
         }
      }
      if (IS_AFFECTED(i, AFF_POISON) && !(affected_by_spell(i, SPELL_POISON)))
      {
         logf(IMMORTAL, LogChannels::LOG_BUG, "Player %s affected by poison but not under poison spell. Removing poison affect.", i->name);
         REMBIT(i->affected_by, AFF_POISON);
      }

      // handle poison
      if (IS_AFFECTED(i, AFF_POISON) && !i->fighting && affected_by_spell(i, SPELL_POISON) && affected_by_spell(i, SPELL_POISON)->location == APPLY_NONE)
      {
         int tmp = number(1, 2) + affected_by_spell(i, SPELL_POISON)->duration;
         if (get_saves(i, SAVE_TYPE_POISON) > number(1, 101))
         {
            tmp *= get_saves(i, SAVE_TYPE_POISON) / 100;
            send_to_char("You feel very sick, but resist the $2poison's$R damage.\n\r", i);
         }
         if (tmp)
         {
            sprintf(dammsg, "$B%d$R", tmp);
            send_damage("You feel burning $2poison$R in your blood and suffer painful convulsions for | damage.", i, 0, i, dammsg,
                        "You feel burning $2poison$R in your blood and suffer painful convulsions.", TO_CHAR);
            send_damage("$n looks extremely sick and shivers uncomfortably from the $2poison$R in $s veins that did | damage.", i, 0, 0, dammsg,
                        "$n looks extremely sick and shivers uncomfortably from the $2poison$R in $s veins.", TO_ROOM);
            retval = noncombat_damage(i, tmp, "You quiver from the effects of the poison and have no enegry left...",
                                      "$n stops struggling as $e is consumed by poison.", 0, KILL_POISON);
            if (SOMEONE_DIED(retval))
               continue;
         }
      }

      // handle drowning
      if (!IS_NPC(i) && GET_LEVEL(i) < IMMORTAL && world[i->in_room].sector_type == SECT_UNDERWATER && !(affected_by_spell(i, SPELL_WATER_BREATHING) || IS_AFFECTED(i, AFF_WATER_BREATHING) || affected_by_spell(i, SKILL_SONG_SUBMARINERS_ANTHEM)))
      {
         tmp = GET_MAX_HIT(i) / 5;
         sprintf(log_msg, "%s drowned in room %d.", GET_NAME(i), world[i->in_room].number);
         retval = noncombat_damage(i, tmp, "You gasp your last breath and everything goes dark...", "$n stops struggling as $e runs out of oxygen.", log_msg,
                                   KILL_DROWN);
         if (SOMEONE_DIED(retval))
            continue;
         else
         {
            sprintf(dammsg, "$B%d$R", tmp);
            send_damage("$n thrashes and gasps, struggling vainly for air, taking | damage.", i, 0, 0, dammsg,
                        "$n thrashes and gasps, stuggling vainly for air.", TO_ROOM);
            send_damage("You gasp and fight madly for air; you are drowning and take | damage!", i, 0, 0, dammsg,
                        "You gasp and fight madly for air; you are drowning!", TO_CHAR);
         }
      }

      // handle command lag
      if (i->timer > 0)
      {
         if (GET_LEVEL(i) < IMMORTAL)
            i->timer--;
         else
            i->timer = 0;
      }

      i->shotsthisround = 0;

      if (IS_AFFECTED(i, AFF_CMAST_WEAKEN))
         REMBIT(i->affected_by, AFF_CMAST_WEAKEN);
      affect_from_char(i, SKILL_COMBAT_MASTERY);

      // perseverance stuff
      if (affected_by_spell(i, SKILL_PERSEVERANCE))
      {
         affect_from_char(i, SKILL_PERSEVERANCE_BONUS);
         af.type = SKILL_PERSEVERANCE_BONUS;
         af.duration = -1;
         af.modifier = 0 - ((2 + affected_by_spell(i, SKILL_PERSEVERANCE)->modifier / 10) * (1 + affected_by_spell(i, SKILL_PERSEVERANCE)->modifier / 11 - affected_by_spell(i, SKILL_PERSEVERANCE)->duration));
         af.location = APPLY_AC;
         af.bitvector = -1;
         affect_to_char(i, &af);

         GET_MOVE(i) += 5 * (1 + affected_by_spell(i, SKILL_PERSEVERANCE)->modifier / 11 - affected_by_spell(i, SKILL_PERSEVERANCE)->duration);
         GET_MOVE(i) = MIN(GET_MOVE(i), GET_MAX_MOVE(i));
      }
   }
}

void check_silence_beacons(void)
{
   obj_data *obj, *tmp_obj;

   for (obj = object_list; obj; obj = tmp_obj)
   {
      tmp_obj = obj->next;
      if (obj_index[obj->item_number].virt == SILENCE_OBJ_NUMBER)
      {
         if (obj->obj_flags.value[0] == 0)
            extract_obj(obj);
         else
            obj->obj_flags.value[0]--;
      }
   }

   return;
}

void checkConsecrate(int pulseType)
{
   obj_data *obj, *tmp_obj;
   char_data *ch = NULL, *tmp_ch, *next_ch;
   int align, amount, spl = 0;
   char buf[MAX_STRING_LENGTH];

   if (pulseType == PULSE_REGEN)
   {
      for (obj = object_list; obj; obj = tmp_obj)
      {
         tmp_obj = obj->next;
         if (obj_index[obj->item_number].virt == CONSECRATE_OBJ_NUMBER)
         {
            spl = obj->obj_flags.value[0];
            obj->obj_flags.value[1]--;
            if (obj->obj_flags.value[1] <= 0)
            {
               if ((ch = obj->obj_flags.origin) && charExists(ch))
               {
                  ch->cRooms--;
                  if (ch->desc)
                  {
                     if (spl == SPELL_CONSECRATE)
                     {
                        if (ch->in_room != obj->in_room)
                           csendf(ch, "You sense your consecration of %s has ended.\r\n", world[obj->in_room].name);
                        else
                           send_to_char("Runes upon the ground glow brightly, then fade to nothing.\r\nYour holy consecration has ended.\r\n", ch);
                     }
                     else
                     {
                        if (ch->in_room != obj->in_room)
                           csendf(ch, "You sense your desecration of %s has ended.\r\n", world[obj->in_room].name);
                        else
                           send_to_char("The runes upon the ground shatter with a burst of magic!\r\nYour unholy desecration has ended.\r\n", ch);
                     }
                  }
               }
               for (tmp_ch = world[obj->in_room].people; tmp_ch; tmp_ch = next_ch)
               {
                  next_ch = tmp_ch->next_in_room;
                  if (tmp_ch == ch)
                  {
                     continue;
                  }

                  if (IS_PC(tmp_ch) && GET_LEVEL(tmp_ch) >= IMMORTAL)
                  {
                     continue;
                  }

                  if (spl == SPELL_CONSECRATE)
                  {
                     if (affected_by_spell(tmp_ch, SPELL_DETECT_GOOD) && affected_by_spell(tmp_ch, SPELL_DETECT_GOOD)->modifier >= 80)
                        send_to_char("Runes upon the ground glow brightly, then fade to nothing.\r\nThe holy consecration here has ended.\n\r", tmp_ch);
                  }
                  else
                  {
                     if (affected_by_spell(tmp_ch, SPELL_DETECT_EVIL) && affected_by_spell(tmp_ch, SPELL_DETECT_EVIL)->modifier >= 80)
                        if (ch && tmp_ch != ch)
                        {
                           send_damage("Runes upon the ground glow softly as your holy consecration heals $N of | damage.", ch, 0, tmp_ch, buf, "Runes upon the ground glow softly as your holy consecration heals $N.", TO_CHAR);
                        }
                     send_to_char("The runes upon the ground shatter with a burst of magic!\r\nThe unholy desecration has ended.\n\r", tmp_ch);
                  }
               }
               extract_obj(obj);
            }
         }
      }
   }
   else if (pulseType == PULSE_TENSEC)
   {
      for (obj = object_list; obj; obj = tmp_obj)
      {
         tmp_obj = obj->next;
         if (obj_index[obj->item_number].virt == CONSECRATE_OBJ_NUMBER)
         {
            spl = obj->obj_flags.value[0];
            if (charExists(obj->obj_flags.origin))
            {
               ch = obj->obj_flags.origin;
            }
            for (tmp_ch = world[obj->in_room].people; tmp_ch; tmp_ch = next_ch)
            {
               next_ch = tmp_ch->next_in_room;
               if (IS_PC(tmp_ch) && GET_LEVEL(tmp_ch) >= IMMORTAL)
               {
                  continue;
               }

               align = GET_ALIGNMENT(tmp_ch);
               if (align > 0)
               {
                  amount = obj->obj_flags.value[2] + 10 + align / 10;
                  if (spl == SPELL_CONSECRATE)
                  {
                     if (GET_HIT(tmp_ch) + amount > GET_MAX_HIT(tmp_ch))
                        amount = GET_MAX_HIT(tmp_ch) - GET_HIT(tmp_ch);
                     sprintf(buf, "%d", amount);
                     tmp_ch->addHP(amount);
                     if (tmp_ch == ch)
                        send_damage("The runes upon the ground glow softly as your holy consecration heals you of | damage.", tmp_ch, 0, 0, buf, "The runes upon the ground glow softly as your holy consecration heals you. ", TO_CHAR);
                     else if (affected_by_spell(tmp_ch, SPELL_DETECT_GOOD) && affected_by_spell(tmp_ch, SPELL_DETECT_GOOD)->modifier >= 80)
                        send_damage("Runes upon the ground glow softly as $n's holy consecration heals you of | damage.", tmp_ch, 0, 0, buf, "Runes upon the ground glow softly as $n's holy consecration heals you.", TO_CHAR);
                     else
                        send_damage("Runes upon the ground glow softly as a holy consecration heals you of | damage.", tmp_ch, 0, 0, buf, "Runes upon the ground glow softly as a holy consecration heals you.", TO_CHAR);

                     if (ch && tmp_ch != ch)
                     {
                        send_damage("Runes upon the ground glow softly as your holy consecration heals $N of | damage.", ch, 0, tmp_ch, buf, "Runes upon the ground glow softly as your holy consecration heals $N.", TO_CHAR);
                     }
                  }
                  else
                  {
                     if (ch && !ARE_GROUPED(ch, tmp_ch))
                     {
                        set_cantquit(ch, tmp_ch);
                     }
                     if (GET_HIT(tmp_ch) - amount < 0)
                     {
                        act("The strength of $N's desecration proves to powerful for $n to overcome and $e drops dead!", tmp_ch, 0, ch, TO_ROOM, NOTVICT);
                        act("The strength of your desecration proves to powerful for $n to overcome and $e drops dead!", tmp_ch, 0, ch, TO_VICT, 0);
                        if (!IS_NPC(tmp_ch))
                        {
                           act("The strength of $N's desecration proves fatal and the world fades to black...", tmp_ch, 0, ch, TO_CHAR, 0);
                           send_to_char("You have been KILLED!!\n\r\n\r", tmp_ch);
                        }
                        group_gain(ch, tmp_ch);
                        fight_kill(ch, tmp_ch, TYPE_CHOOSE, SPELL_DESECRATE);
                        continue;
                     }
                     sprintf(buf, "%d", amount);
                     tmp_ch->removeHP(amount);
                     if (tmp_ch == ch)
                        send_damage("The runes upon the ground hum ominously as your unholy desecration injures you, dealing | damage.", tmp_ch, 0, 0, buf, "The runes upon the ground hum ominously as your unholy desecration injures you. ", TO_CHAR);
                     else if (affected_by_spell(tmp_ch, SPELL_DETECT_GOOD) && affected_by_spell(tmp_ch, SPELL_DETECT_GOOD)->modifier >= 80)
                        send_damage("Runes upon the ground hum ominously as $n's unholy desecration injures you, dealing | damage.", tmp_ch, 0, 0, buf, "Runes upon the ground hum ominously as $n's unholy desecration injures you.", TO_CHAR);
                     else
                        send_damage("Runes upon the ground hum ominously as an unholy desecration injures you, dealing | damage.", tmp_ch, 0, 0, buf, "Runes upon the ground hum ominously as an unholy desecration injures you.", TO_CHAR);

                     if (ch && tmp_ch != ch)
                     {
                        send_damage("Runes upon the ground hum ominously as an unholy desecration injures $N dealing | damage.", ch, 0, tmp_ch, buf, "Runes upon the ground hum ominously as an unholy desecration injures $N.", TO_CHAR);
                     }
                  }
               }
               else if (align < 0)
               {
                  amount = obj->obj_flags.value[2] + 10 - align / 10;
                  if (spl == SPELL_DESECRATE)
                  {
                     if (GET_HIT(tmp_ch) + amount > GET_MAX_HIT(tmp_ch))
                        amount = GET_MAX_HIT(tmp_ch) - GET_HIT(tmp_ch);
                     sprintf(buf, "%d", amount);
                     tmp_ch->addHP(amount);
                     if (tmp_ch == ch)
                        send_damage("The runes upon the ground hum ominously as your unholy desecration heals you of | damage.", tmp_ch, 0, 0, buf, "The runes upon the ground hum ominously as your unholy desecration heals you. ", TO_CHAR);
                     else if (affected_by_spell(tmp_ch, SPELL_DETECT_EVIL) && affected_by_spell(tmp_ch, SPELL_DETECT_EVIL)->modifier >= 80)
                        send_damage("Runes upon the ground hum ominously as $n's unholy desecration heals you of | damage.", tmp_ch, 0, 0, buf, "Runes upon the ground hum ominously as $n's unholy desecration heals you.", TO_CHAR);
                     else
                        send_damage("Runes upon the ground hum ominously as an unholy desecration heals you of | damage.", tmp_ch, 0, 0, buf, "Runes upon the ground hum ominously as an unholy desecration heals you.", TO_CHAR);

                     if (ch && tmp_ch != ch)
                     {
                        send_damage("Runes upon the ground hum ominously as an unholy desecration heals $N of | damage.", ch, 0, tmp_ch, buf, "Runes upon the ground hum ominously as an unholy desecration heals $N.", TO_CHAR);
                     }
                  }
                  else
                  {

                     if (ch && !ARE_GROUPED(ch, tmp_ch))
                     {
                        set_cantquit(ch, tmp_ch);
                     }
                     if (GET_HIT(tmp_ch) - amount < 0)
                     {
                        act("The strength of $N's consecration proves to powerful for $n to overcome and $e drops dead!", tmp_ch, 0, ch, TO_ROOM, NOTVICT);
                        act("The strength of your consecration proves to powerful for $n to overcome and $e drops dead!", tmp_ch, 0, ch, TO_VICT, 0);
                        if (!IS_NPC(tmp_ch))
                        {
                           act("The strength of $N's consecration proves fatal and the world fades to black...", tmp_ch, 0, ch, TO_CHAR, 0);
                           send_to_char("You have been KILLED!!\n\r\n\r", tmp_ch);
                        }
                        group_gain(ch, tmp_ch);
                        fight_kill(ch, tmp_ch, TYPE_CHOOSE, SPELL_CONSECRATE);
                        continue;
                     }
                     sprintf(buf, "%d", amount);
                     tmp_ch->removeHP(amount);
                     if (tmp_ch == ch)
                        send_damage("The runes upon the ground glow softly as your holy consecration injures you, dealing | damage.", tmp_ch, 0, 0, buf, "The runes upon the ground glow softly as your holy consecration injures you. ", TO_CHAR);
                     else if (affected_by_spell(tmp_ch, SPELL_DETECT_GOOD) && affected_by_spell(tmp_ch, SPELL_DETECT_GOOD)->modifier >= 80)
                        send_damage("Runes upon the ground glow softly as $n's holy consecration injures you, dealing | damage.", tmp_ch, 0, 0, buf, "Runes upon the ground glow softly as $n's holy consecration injures you.", TO_CHAR);
                     else
                        send_damage("Runes upon the ground glow softly as a holy consecration injures you, dealing | damage.", tmp_ch, 0, 0, buf, "Runes upon the ground glow softly as a holy consecration injures you.", TO_CHAR);
                     if (ch && tmp_ch != ch)
                     {
                        send_damage("Runes upon the ground hum softly as an holy consecration injures $N, dealing | damage.", ch, 0, tmp_ch, buf, "Runes upon the ground hum softly as a holy consecration injures $N.", TO_CHAR);
                     }
                  }
               }
            }
            break;
         }
      }
   }
   DC::getInstance()->removeDead();
   return;
}

/* check name to see if it is listed in the file of forbidden player names */
bool on_forbidden_name_list(char *name)
{
   FILE *nameList;
   char buf[MAX_STRING_LENGTH + 1];
   bool found = FALSE;
   int i;

   nameList = dc_fopen(FORBIDDEN_NAME_FILE, "ro");
   if (!nameList)
   {
      log("Failed to open forbidden name file!", 0, LogChannels::LOG_MISC);
      return FALSE;
   }
   else
   {
      while (fgets(buf, MAX_STRING_LENGTH, nameList) && !found)
      {
         /* chop off trailing \n */
         if ((i = strlen(buf)) > 0)
            buf[i - 1] = '\0';
         if (!str_cmp(name, buf))
            found = TRUE;
      }
      dc_fclose(nameList);
   }
   return found;
}

void show_question_race(descriptor_data *d)
{
   if (d == nullptr || d->character == nullptr)
   {
      return;
   }

   char_data *ch = d->character;
   string buffer, races_buffer;
   buffer += "\r\nRacial Bonuses and Pentalties:\r\n";
   buffer += "$B$7   Race   STR DEX CON INT WIS$R\r\n";
   for (int race = 1; race <= 9; race++)
   {
      if (races[race].singular_name != nullptr)
      {
         buffer += fmt::format("{}. {:6} {:3} {:3} {:3} {:3} {:3}\r\n", race, races[race].singular_name,
                               races[race].mod_str, races[race].mod_dex, races[race].mod_con, races[race].mod_int,
                               races[race].mod_wis);
         races_buffer += races[race].lowercase_name;
         if (race < MAX_PC_RACE)
         {
            races_buffer += ",";
         }
      }
      undo_race_saves(ch);
   }
   buffer += "Type 1-" + to_string(MAX_PC_RACE) + "," + races_buffer + " or help <keyword>: ";
   SEND_TO_Q(buffer.c_str(), d);
   telnet_ga(d);
}

bool handle_get_race(descriptor_data *d, string arg)
{
   if (d == nullptr || d->character == nullptr || arg == "")
   {
      return false;
   }

   for (unsigned race = 1; race <= 9; race++)
   {
      if (races[race].lowercase_name == arg)
      {
         GET_RACE(d->character) = race;
         return true;
      }
   }

   uint32_t race = 0;
   try
   {
      race = stoul(arg);
   }
   catch (...)
   {
      return false;
   }

   if (race < 1 || race > MAX_PC_RACE)
   {
      return false;
   }

   GET_RACE(d->character) = race;
   apply_race_attributes(d->character);

   return true;
}

void show_question_class(descriptor_data *d)
{
   if (d == nullptr || d->character == nullptr)
   {
      return;
   }

   char_data *ch = d->character;
   string buffer, classes_buffer;
   buffer += "\r\n   Class$R\r\n";
   int clss;
   for (clss = 1; clss <= CLASS_MAX_PROD; clss++)
   {
      if (pc_clss_types[clss] != nullptr)
      {
         if (!is_clss_race_compat(ch, clss, GET_RACE(ch)))
         {
            buffer += fmt::format("{:2}. {:11} (Unavailble for your race)\r\n", clss, pc_clss_types[clss]);
         }
         else
         {
            buffer += fmt::format("{:2}. {:11}\r\n", clss, pc_clss_types[clss]);
            classes_buffer += string(pc_clss_types3[clss]);
            if (clss < CLASS_MAX_PROD)
            {
               classes_buffer += ",";
            }
         }
      }
   }

   buffer += "Type 'back' to go back and pick a different race.\r\n";
   buffer += "Type '1-" + to_string(CLASS_MAX_PROD) + "," + classes_buffer + "' or 'help keyword': ";
   SEND_TO_Q(buffer.c_str(), d);
   telnet_ga(d);
}

bool handle_get_class(descriptor_data *d, string arg)
{
   if (d == nullptr || d->character == nullptr || arg == "")
   {
      return false;
   }

   if (arg == "back")
   {
      STATE(d) = conn::QUESTION_RACE;
      return false;
   }

   const char_data *ch = d->character;

   for (unsigned clss = 1; clss <= CLASS_MAX_PROD; clss++)
   {
      if (string(pc_clss_types[clss]) == string(arg) || string(pc_clss_types3[clss]) == string(arg))
      {
         if (!is_clss_race_compat(ch, clss, GET_RACE(ch)))
         {
            return false;
         }
         else
         {
            GET_CLASS(d->character) = clss;
            return true;
         }
      }
   }

   uint32_t clss = 0;
   try
   {
      clss = stoul(arg);
   }
   catch (...)
   {
      return false;
   }

   if (clss < 1 || clss > CLASS_MAX_PROD || !is_clss_race_compat(ch, clss, GET_RACE(ch)))
   {
      return false;
   }

   GET_CLASS(d->character) = clss;

   return true;
}

uint8_t stat_data::getMin(uint8_t cur, int8_t race_mod, uint8_t min)
{
   if (min > cur + race_mod)
   {
      uint8_t points_needed = min - (cur + race_mod);
      if (points > points_needed)
      {
         points -= points_needed;
         cur += points_needed;
      }
   }

   return cur;
}

void stat_data::setMin(void)
{
   str[0] = getMin(str[0], races[race].mod_str, MAX(races[race].min_str, classes[clss].min_str));
   dex[0] = getMin(dex[0], races[race].mod_dex, MAX(races[race].min_dex, classes[clss].min_dex));
   con[0] = getMin(con[0], races[race].mod_con, MAX(races[race].min_con, classes[clss].min_con));
   tel[0] = getMin(tel[0], races[race].mod_int, MAX(races[race].min_int, classes[clss].min_int));
   wis[0] = getMin(wis[0], races[race].mod_wis, MAX(races[race].min_wis, classes[clss].min_wis));
}

void show_question_stats(descriptor_data *d)
{
   if (d == nullptr || d->character == nullptr)
   {
      return;
   }

   if (d->stats == nullptr)
   {
      d->stats = new stat_data;

      char_data *ch = d->character;
      int32_t race = d->stats->race = GET_RACE(ch);
      int8_t clss = d->stats->clss = GET_CLASS(ch);

      // Current
      d->stats->str[0] = 12;
      d->stats->dex[0] = 12;
      d->stats->con[0] = 12;
      d->stats->tel[0] = 12;
      d->stats->wis[0] = 12;

      d->stats->points = 23;

      d->stats->setMin();
   }

   char_data *ch = d->character;
   unsigned race = GET_RACE(ch);
   unsigned clss = GET_CLASS(ch);
   string buffer = fmt::format("\r\nRace: {}\r\n", races[race].singular_name);
   buffer += fmt::format("Class: {}\r\n", classes[clss].name);
   buffer += fmt::format("Points left to assign: {}\r\n", d->stats->points);
   buffer += fmt::format("## Attribute      Current  Racial Offsets  Total\r\n");
   if (d->stats->selection == 1)
   {
      buffer += fmt::format("1. $B*Strength*$R     {:2}      {:2}               {:2}\r\n",
                            d->stats->str[0], races[race].mod_str, d->stats->str[0] + races[race].mod_str);
   }
   else
   {
      buffer += fmt::format("1. Strength       {:2}      {:2}               {:2}\r\n",
                            d->stats->str[0], races[race].mod_str, d->stats->str[0] + races[race].mod_str);
   }

   if (d->stats->selection == 2)
   {
      buffer += fmt::format("2. $B*Dexterity*$R    {:2}      {:2}               {:2}\r\n",
                            d->stats->dex[0], races[race].mod_dex, d->stats->dex[0] + races[race].mod_dex);
   }
   else
   {
      buffer += fmt::format("2. Dexterity      {:2}      {:2}               {:2}\r\n",
                            d->stats->dex[0], races[race].mod_dex, d->stats->dex[0] + races[race].mod_dex);
   }

   if (d->stats->selection == 3)
   {
      buffer += fmt::format("3. $B*Constitution*$R {:2}      {:2}               {:2}\r\n",
                            d->stats->con[0], races[race].mod_con, d->stats->con[0] + races[race].mod_con);
   }
   else
   {
      buffer += fmt::format("3. Constitution   {:2}      {:2}               {:2}\r\n",
                            d->stats->con[0], races[race].mod_con, d->stats->con[0] + races[race].mod_con);
   }

   if (d->stats->selection == 4)
   {
      buffer += fmt::format("4. $B*Intelligence*$R {:2}      {:2}               {:2}\r\n",
                            d->stats->tel[0], races[race].mod_int, d->stats->tel[0] + races[race].mod_int);
   }
   else
   {
      buffer += fmt::format("4. Intelligence   {:2}      {:2}               {:2}\r\n",
                            d->stats->tel[0], races[race].mod_int, d->stats->tel[0] + races[race].mod_int);
   }

   if (d->stats->selection == 5)
   {
      buffer += fmt::format("5. $B*Wisdom*$R       {:2}      {:2}               {:2}\r\n",
                            d->stats->wis[0], races[race].mod_wis, d->stats->wis[0] + races[race].mod_wis);
   }
   else
   {
      buffer += fmt::format("5. Wisdom         {:2}      {:2}               {:2}\r\n",
                            d->stats->wis[0], races[race].mod_wis, d->stats->wis[0] + races[race].mod_wis);
   }

   if (d->stats->selection == 0)
   {
      buffer += "Type '1-5' or 'help keyword': ";
   }
   else if (d->stats->points > 0)
   {
      buffer += "Type '+', '-', '1-5', 'confirm' or 'help keyword': ";
   }
   else
   {
      buffer += "Type '-', '1-5', 'confirm' or 'help keyword': ";
   }
   SEND_TO_Q(buffer.c_str(), d);
   telnet_ga(d);
}

bool handle_get_stats(descriptor_data *d, string arg)
{
   if (arg != "+" && arg != "-" && arg != "confirm")
   {
      try
      {
         d->stats->selection = stoul(arg);
      }
      catch (...)
      {
         d->stats->selection = 0;
         SEND_TO_Q("Invalid number specified.\r\n", d);
      }
      return false;
   }

   if (d->stats->selection > 0 && d->stats->selection < 6)
   {
      if (arg == "+")
      {
         if (d->stats->points > 0)
         {
            switch (d->stats->selection)
            {
            case 1: // STR
               if (d->stats->str[0] < 18)
               {
                  d->stats->str[0]++;
                  d->stats->points--;
               }
               break;

            case 2: // DEX
               if (d->stats->dex[0] < 18)
               {
                  d->stats->dex[0]++;
                  d->stats->points--;
               }
               break;
            case 3: // CON
               if (d->stats->con[0] < 18)
               {
                  d->stats->con[0]++;
                  d->stats->points--;
               }
               break;
            case 4: // INT
               if (d->stats->tel[0] < 18)
               {
                  d->stats->tel[0]++;
                  d->stats->points--;
               }
               break;
            case 5: // WIS
               if (d->stats->wis[0] < 18)
               {
                  d->stats->wis[0]++;
                  d->stats->points--;
               }
               break;
            }
         }
      }
      else if (arg == "-")
      {
         switch (d->stats->selection)
         {
         case 1: // STR
            if (d->stats->str[0] > 12)
            {
               d->stats->str[0]--;
               d->stats->points++;
            }
            break;

         case 2: // DEX
            if (d->stats->dex[0] > 12)
            {
               d->stats->dex[0]--;
               d->stats->points++;
            }
            break;
         case 3: // CON
            if (d->stats->con[0] > 12)
            {
               d->stats->con[0]--;
               d->stats->points++;
            }
            break;
         case 4: // INT
            if (d->stats->tel[0] > 12)
            {
               d->stats->tel[0]--;
               d->stats->points++;
            }
            break;
         case 5: // WIS
            if (d->stats->wis[0] > 12)
            {
               d->stats->wis[0]--;
               d->stats->points++;
            }
            break;
         }
         d->stats->setMin();
      }
      else if (arg.find("help") == 0)
      {
         arg.erase(0, 5);
         do_help(d->character, arg.data(), CMD_DEFAULT);
         return false;
      }
      else if (arg == "confirm")
      {
         if (d->stats->points > 0)
         {
            SEND_TO_Q("You must assign all your points first.\r\n", d);
            return false;
         }

         char_data *ch = d->character;
         unsigned race = GET_RACE(ch);
         GET_RAW_STR(ch) = d->stats->str[0];
         GET_RAW_DEX(ch) = d->stats->dex[0];
         GET_RAW_CON(ch) = d->stats->con[0];
         GET_RAW_INT(ch) = d->stats->tel[0];
         GET_RAW_WIS(ch) = d->stats->wis[0];
         apply_race_attributes(ch);

         if (GET_CLASS(ch) == CLASS_ANTI_PAL)
         {
            GET_ALIGNMENT(ch) = -1000;
         }
         else if (GET_CLASS(ch) == CLASS_PALADIN)
         {
            GET_ALIGNMENT(ch) = 1000;
         }

         set_hw(ch);

         return true;
      }
   }

   return false;
}

bool apply_race_attributes(char_data *ch, int race)
{
   if (ch == nullptr)
   {
      return false;
   }

   if (race == 0)
   {
      race = ch->race;
   }

   switch (race)
   {
   case 1:
      ch->race = RACE_HUMAN;
      GET_RAW_STR(ch) += RACE_HUMAN_STR_MOD;
      GET_RAW_INT(ch) += RACE_HUMAN_INT_MOD;
      GET_RAW_WIS(ch) += RACE_HUMAN_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_HUMAN_DEX_MOD;
      GET_RAW_CON(ch) += RACE_HUMAN_CON_MOD;
      return true;
      break;

   case 2:
      ch->race = RACE_ELVEN;
      ch->alignment = 1000;
      GET_RAW_STR(ch) += RACE_ELVEN_STR_MOD;
      GET_RAW_INT(ch) += RACE_ELVEN_INT_MOD;
      GET_RAW_WIS(ch) += RACE_ELVEN_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_ELVEN_DEX_MOD;
      GET_RAW_CON(ch) += RACE_ELVEN_CON_MOD;
      return true;
      break;

   case 3:
      ch->race = RACE_DWARVEN;
      GET_RAW_STR(ch) += RACE_DWARVEN_STR_MOD;
      GET_RAW_INT(ch) += RACE_DWARVEN_INT_MOD;
      GET_RAW_WIS(ch) += RACE_DWARVEN_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_DWARVEN_DEX_MOD;
      GET_RAW_CON(ch) += RACE_DWARVEN_CON_MOD;
      return true;
      break;

   case 4:
      ch->race = RACE_HOBBIT;
      GET_RAW_STR(ch) += RACE_HOBBIT_STR_MOD;
      GET_RAW_INT(ch) += RACE_HOBBIT_INT_MOD;
      GET_RAW_WIS(ch) += RACE_HOBBIT_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_HOBBIT_DEX_MOD;
      GET_RAW_CON(ch) += RACE_HOBBIT_CON_MOD;
      return true;
      break;

   case 5:
      ch->race = RACE_PIXIE;
      GET_RAW_STR(ch) += RACE_PIXIE_STR_MOD;
      GET_RAW_INT(ch) += RACE_PIXIE_INT_MOD;
      GET_RAW_WIS(ch) += RACE_PIXIE_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_PIXIE_DEX_MOD;
      GET_RAW_CON(ch) += RACE_PIXIE_CON_MOD;
      return true;
      break;

   case 6:
      ch->race = RACE_GIANT;
      GET_RAW_STR(ch) += RACE_GIANT_STR_MOD;
      GET_RAW_INT(ch) += RACE_GIANT_INT_MOD;
      GET_RAW_WIS(ch) += RACE_GIANT_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_GIANT_DEX_MOD;
      GET_RAW_CON(ch) += RACE_GIANT_CON_MOD;
      return true;
      break;

   case 7:
      ch->race = RACE_GNOME;
      GET_RAW_STR(ch) += RACE_GNOME_STR_MOD;
      GET_RAW_INT(ch) += RACE_GNOME_INT_MOD;
      GET_RAW_WIS(ch) += RACE_GNOME_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_GNOME_DEX_MOD;
      GET_RAW_CON(ch) += RACE_GNOME_CON_MOD;
      return true;
      break;

   case 8:
      ch->race = RACE_ORC;
      ch->alignment = -1000;
      GET_RAW_STR(ch) += RACE_ORC_STR_MOD;
      GET_RAW_INT(ch) += RACE_ORC_INT_MOD;
      GET_RAW_WIS(ch) += RACE_ORC_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_ORC_DEX_MOD;
      GET_RAW_CON(ch) += RACE_ORC_CON_MOD;
      return true;
      break;
   case 9:
      ch->race = RACE_TROLL;
      ch->alignment = 0;
      GET_RAW_STR(ch) += RACE_TROLL_STR_MOD;
      GET_RAW_INT(ch) += RACE_TROLL_INT_MOD;
      GET_RAW_WIS(ch) += RACE_TROLL_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_TROLL_DEX_MOD;
      GET_RAW_CON(ch) += RACE_TROLL_CON_MOD;
      return true;
      break;

   default:
      return false;
      break;
   }

   return false;
}

bool check_race_attributes(char_data *ch, int race)
{
   if (ch == nullptr)
   {
      return false;
   }

   if (race == 0)
   {
      race = ch->race;
   }

   switch (race)
   {
   case 1:
      ch->race = RACE_HUMAN;
      GET_RAW_STR(ch) += RACE_HUMAN_STR_MOD;
      GET_RAW_INT(ch) += RACE_HUMAN_INT_MOD;
      GET_RAW_WIS(ch) += RACE_HUMAN_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_HUMAN_DEX_MOD;
      GET_RAW_CON(ch) += RACE_HUMAN_CON_MOD;
      return true;
      break;

   case 2:
      if (GET_RAW_DEX(ch) < 10 || GET_RAW_INT(ch) < 10)
      {
         return false;
      }
      ch->race = RACE_ELVEN;
      ch->alignment = 1000;
      GET_RAW_STR(ch) += RACE_ELVEN_STR_MOD;
      GET_RAW_INT(ch) += RACE_ELVEN_INT_MOD;
      GET_RAW_WIS(ch) += RACE_ELVEN_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_ELVEN_DEX_MOD;
      GET_RAW_CON(ch) += RACE_ELVEN_CON_MOD;
      return true;
      break;

   case 3:
      if (GET_RAW_CON(ch) < 10 || GET_RAW_WIS(ch) < 10)
      {
         return false;
      }
      ch->race = RACE_DWARVEN;
      GET_RAW_STR(ch) += RACE_DWARVEN_STR_MOD;
      GET_RAW_INT(ch) += RACE_DWARVEN_INT_MOD;
      GET_RAW_WIS(ch) += RACE_DWARVEN_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_DWARVEN_DEX_MOD;
      GET_RAW_CON(ch) += RACE_DWARVEN_CON_MOD;
      return true;
      break;

   case 4:
      if (GET_RAW_DEX(ch) < 10)
      {
         return false;
      }
      ch->race = RACE_HOBBIT;
      GET_RAW_STR(ch) += RACE_HOBBIT_STR_MOD;
      GET_RAW_INT(ch) += RACE_HOBBIT_INT_MOD;
      GET_RAW_WIS(ch) += RACE_HOBBIT_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_HOBBIT_DEX_MOD;
      GET_RAW_CON(ch) += RACE_HOBBIT_CON_MOD;
      return true;
      break;

   case 5:
      if (GET_RAW_INT(ch) < 12)
      {
         return false;
      }
      ch->race = RACE_PIXIE;
      GET_RAW_STR(ch) += RACE_PIXIE_STR_MOD;
      GET_RAW_INT(ch) += RACE_PIXIE_INT_MOD;
      GET_RAW_WIS(ch) += RACE_PIXIE_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_PIXIE_DEX_MOD;
      GET_RAW_CON(ch) += RACE_PIXIE_CON_MOD;
      return true;
      break;

   case 6:
      if (GET_RAW_STR(ch) < 12)
      {
         return false;
      }
      ch->race = RACE_GIANT;
      GET_RAW_STR(ch) += RACE_GIANT_STR_MOD;
      GET_RAW_INT(ch) += RACE_GIANT_INT_MOD;
      GET_RAW_WIS(ch) += RACE_GIANT_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_GIANT_DEX_MOD;
      GET_RAW_CON(ch) += RACE_GIANT_CON_MOD;
      return true;
      break;

   case 7:
      if (GET_RAW_WIS(ch) < 12)
      {
         return false;
      }
      ch->race = RACE_GNOME;
      GET_RAW_STR(ch) += RACE_GNOME_STR_MOD;
      GET_RAW_INT(ch) += RACE_GNOME_INT_MOD;
      GET_RAW_WIS(ch) += RACE_GNOME_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_GNOME_DEX_MOD;
      GET_RAW_CON(ch) += RACE_GNOME_CON_MOD;
      return true;
      break;

   case 8:
      if (GET_RAW_CON(ch) < 10 || GET_RAW_STR(ch) < 10)
      {
         return false;
      }
      ch->race = RACE_ORC;
      ch->alignment = -1000;
      GET_RAW_STR(ch) += RACE_ORC_STR_MOD;
      GET_RAW_INT(ch) += RACE_ORC_INT_MOD;
      GET_RAW_WIS(ch) += RACE_ORC_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_ORC_DEX_MOD;
      GET_RAW_CON(ch) += RACE_ORC_CON_MOD;
      return true;
      break;
   case 9:
      if (GET_RAW_CON(ch) < 12)
      {
         return false;
      }
      ch->race = RACE_TROLL;
      ch->alignment = 0;
      GET_RAW_STR(ch) += RACE_TROLL_STR_MOD;
      GET_RAW_INT(ch) += RACE_TROLL_INT_MOD;
      GET_RAW_WIS(ch) += RACE_TROLL_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_TROLL_DEX_MOD;
      GET_RAW_CON(ch) += RACE_TROLL_CON_MOD;
      return true;
      break;

   default:
      return false;
      break;
   }

   return false;
}

stat_data::stat_data(void)
    : min_str(0), min_dex(0), min_con(0), min_int(0), min_wis(0), points(0), selection(0), race(0), clss(0)
{
   memset(str, 0, sizeof(str));
   memset(dex, 0, sizeof(dex));
   memset(con, 0, sizeof(con));
   memset(tel, 0, sizeof(tel));
   memset(wis, 0, sizeof(wis));
}