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

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <unistd.h>
#include <cstring>
#include <queue>
#include <fmt/format.h>

#include "DC/character.h"
#include "DC/comm.h"
#include "DC/connect.h"
#include "DC/race.h"
#include "DC/player.h"
#include "DC/structs.h" // true
#include "DC/utility.h"
#include "DC/ki.h"
#include "DC/clan.h"
#include "DC/fileinfo.h" // SAVE_DIR
#include "DC/db.h"       // init_char..
#include "DC/mobile.h"
#include "DC/interp.h"
#include "DC/room.h"
#include "DC/act.h"
#include "DC/clan.h"
#include "DC/spells.h"
#include "DC/fight.h"
#include "DC/handler.h"
#include "DC/vault.h"
#include "DC/const.h"
#include "DC/guild.h"
#include "DC/meta.h"
#include <string>

#define STATE(d) ((d)->connected)

bool is_bracing(Character *bracee, struct room_direction_data *exit);
void show_question_race(Connection *d);

const char menu[] = "\n\rWelcome to Dark Castle Mud\n\r\n\r"
                    "0) Exit Dark Castle.\r\n"
                    "1) Enter the game.\r\n"
                    "2) Enter your character's description.\r\n"
                    "3) Change your password.\r\n"
                    "4) Delete this character.\n\r\n\r"
                    "   Make your choice: ";

bool wizlock = false;

extern char greetings1[MAX_STRING_LENGTH];
extern char greetings2[MAX_STRING_LENGTH];
extern char greetings3[MAX_STRING_LENGTH];
extern char greetings4[MAX_STRING_LENGTH];
extern char webpage[MAX_STRING_LENGTH];
extern char motd[MAX_STRING_LENGTH];
extern char imotd[MAX_STRING_LENGTH];

extern Object *object_list;

int _parse_email(char *arg);
bool check_deny(class Connection *d, char *name);
void isr_set(Character *ch);
bool check_reconnect(class Connection *d, QString name, bool fReconnect);
bool check_playing(class Connection *d, QString name);
char *str_str(char *first, char *second);
bool apply_race_attributes(Character *ch, int race = 0);
bool check_race_attributes(Character *ch, int race = 0);
bool handle_get_race(Connection *d, std::string arg);
void show_question_race(Connection *d);
void show_question_class(Connection *d);
bool handle_get_class(Connection *d, std::string arg);
int is_clss_race_compat(Character *ch, int clss);
void show_question_stats(Connection *d);
bool handle_get_stats(Connection *d, std::string arg);

int is_race_eligible(Character *ch, int race)
{
   if (race == 2 && (GET_RAW_DEX(ch) < 10 || GET_RAW_INT(ch) < 10))
      return false;
   if (race == 3 && (GET_RAW_CON(ch) < 10 || GET_RAW_WIS(ch) < 10))
      return false;
   if (race == 4 && (GET_RAW_DEX(ch) < 10))
      return false;
   if (race == 5 && (GET_RAW_DEX(ch) < 12))
      return false;
   if (race == 6 && (GET_RAW_STR(ch) < 12))
      return false;
   if (race == 7 && (GET_RAW_WIS(ch) < 12))
      return false;
   if (race == 8 && (GET_RAW_CON(ch) < 10 || GET_RAW_STR(ch) < 10))
      return false;
   if (race == 9 && (GET_RAW_CON(ch) < 12))
      return false;
   return true;
}

int is_clss_race_compat(const Character *ch, int clss, int race)
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

int is_clss_eligible(Character *ch, int clss)
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

void Character::do_inate_race_abilities(void)
{
   // Add race base saving throw mods
   // Yes, I could combine this 'switch' with the next one, but this is
   // alot more readable
   switch (GET_RACE(this))
   {
   case RACE_HUMAN:
      this->saves[SAVE_TYPE_FIRE] += RACE_HUMAN_FIRE_MOD;
      this->saves[SAVE_TYPE_COLD] += RACE_HUMAN_COLD_MOD;
      this->saves[SAVE_TYPE_ENERGY] += RACE_HUMAN_ENERGY_MOD;
      this->saves[SAVE_TYPE_ACID] += RACE_HUMAN_ACID_MOD;
      this->saves[SAVE_TYPE_MAGIC] += RACE_HUMAN_MAGIC_MOD;
      this->saves[SAVE_TYPE_POISON] += RACE_HUMAN_POISON_MOD;
      break;
   case RACE_ELVEN:
      this->saves[SAVE_TYPE_FIRE] += RACE_ELVEN_FIRE_MOD;
      this->saves[SAVE_TYPE_COLD] += RACE_ELVEN_COLD_MOD;
      this->saves[SAVE_TYPE_ENERGY] += RACE_ELVEN_ENERGY_MOD;
      this->saves[SAVE_TYPE_ACID] += RACE_ELVEN_ACID_MOD;
      this->saves[SAVE_TYPE_MAGIC] += RACE_ELVEN_MAGIC_MOD;
      this->saves[SAVE_TYPE_POISON] += RACE_ELVEN_POISON_MOD;
      this->spell_mitigation += 1;
      break;
   case RACE_DWARVEN:
      this->saves[SAVE_TYPE_FIRE] += RACE_DWARVEN_FIRE_MOD;
      this->saves[SAVE_TYPE_COLD] += RACE_DWARVEN_COLD_MOD;
      this->saves[SAVE_TYPE_ENERGY] += RACE_DWARVEN_ENERGY_MOD;
      this->saves[SAVE_TYPE_ACID] += RACE_DWARVEN_ACID_MOD;
      this->saves[SAVE_TYPE_MAGIC] += RACE_DWARVEN_MAGIC_MOD;
      this->saves[SAVE_TYPE_POISON] += RACE_DWARVEN_POISON_MOD;
      this->melee_mitigation += 1;
      break;
   case RACE_TROLL:
      this->saves[SAVE_TYPE_FIRE] += RACE_TROLL_FIRE_MOD;
      this->saves[SAVE_TYPE_COLD] += RACE_TROLL_COLD_MOD;
      this->saves[SAVE_TYPE_ENERGY] += RACE_TROLL_ENERGY_MOD;
      this->saves[SAVE_TYPE_ACID] += RACE_TROLL_ACID_MOD;
      this->saves[SAVE_TYPE_MAGIC] += RACE_TROLL_MAGIC_MOD;
      this->saves[SAVE_TYPE_POISON] += RACE_TROLL_POISON_MOD;
      this->spell_mitigation += 2;
      break;
   case RACE_GIANT:
      this->saves[SAVE_TYPE_FIRE] += RACE_GIANT_FIRE_MOD;
      this->saves[SAVE_TYPE_COLD] += RACE_GIANT_COLD_MOD;
      this->saves[SAVE_TYPE_ENERGY] += RACE_GIANT_ENERGY_MOD;
      this->saves[SAVE_TYPE_ACID] += RACE_GIANT_ACID_MOD;
      this->saves[SAVE_TYPE_MAGIC] += RACE_GIANT_MAGIC_MOD;
      this->saves[SAVE_TYPE_POISON] += RACE_GIANT_POISON_MOD;
      this->melee_mitigation += 2;
      break;
   case RACE_PIXIE:
      this->saves[SAVE_TYPE_FIRE] += RACE_PIXIE_FIRE_MOD;
      this->saves[SAVE_TYPE_COLD] += RACE_PIXIE_COLD_MOD;
      this->saves[SAVE_TYPE_ENERGY] += RACE_PIXIE_ENERGY_MOD;
      this->saves[SAVE_TYPE_ACID] += RACE_PIXIE_ACID_MOD;
      this->saves[SAVE_TYPE_MAGIC] += RACE_PIXIE_MAGIC_MOD;
      this->saves[SAVE_TYPE_POISON] += RACE_PIXIE_POISON_MOD;
      this->spell_mitigation += 2;
      break;
   case RACE_HOBBIT:
      this->saves[SAVE_TYPE_FIRE] += RACE_HOBBIT_FIRE_MOD;
      this->saves[SAVE_TYPE_COLD] += RACE_HOBBIT_COLD_MOD;
      this->saves[SAVE_TYPE_ENERGY] += RACE_HOBBIT_ENERGY_MOD;
      this->saves[SAVE_TYPE_ACID] += RACE_HOBBIT_ACID_MOD;
      this->saves[SAVE_TYPE_MAGIC] += RACE_HOBBIT_MAGIC_MOD;
      this->saves[SAVE_TYPE_POISON] += RACE_HOBBIT_POISON_MOD;
      this->melee_mitigation += 2;
      break;
   case RACE_GNOME:
      this->saves[SAVE_TYPE_FIRE] += RACE_GNOME_FIRE_MOD;
      this->saves[SAVE_TYPE_COLD] += RACE_GNOME_COLD_MOD;
      this->saves[SAVE_TYPE_ENERGY] += RACE_GNOME_ENERGY_MOD;
      this->saves[SAVE_TYPE_ACID] += RACE_GNOME_ACID_MOD;
      this->saves[SAVE_TYPE_MAGIC] += RACE_GNOME_MAGIC_MOD;
      this->saves[SAVE_TYPE_POISON] += RACE_GNOME_POISON_MOD;
      this->spell_mitigation += 1;
      break;
   case RACE_ORC:
      this->saves[SAVE_TYPE_FIRE] += RACE_ORC_FIRE_MOD;
      this->saves[SAVE_TYPE_COLD] += RACE_ORC_COLD_MOD;
      this->saves[SAVE_TYPE_ENERGY] += RACE_ORC_ENERGY_MOD;
      this->saves[SAVE_TYPE_ACID] += RACE_ORC_ACID_MOD;
      this->saves[SAVE_TYPE_MAGIC] += RACE_ORC_MAGIC_MOD;
      this->saves[SAVE_TYPE_POISON] += RACE_ORC_POISON_MOD;
      this->melee_mitigation += 1;
      break;
   default:
      break;
   }
}

Object *Character::clan_altar(void)
{
   if (clan)
      for (auto c = DC::getInstance()->clan_list; c; c = c->next)
         if (c->number == clan)
         {
            for (auto room = c->rooms; room; room = room->next)
            {
               if (real_room(room->room_number) == DC::NOWHERE)
                  continue;
               Object *t = DC::getInstance()->world[real_room(room->room_number)].contents;
               for (; t; t = t->next_content)
               {
                  if (t->obj_flags.type_flag == ITEM_ALTAR)
                     return t;
               }
            }
         }
   return nullptr;
}

void update_max_who(void)
{
   uint64_t players = 0;
   for (auto d = DC::getInstance()->descriptor_list; d != nullptr; d = d->next)
   {
      if (d->isPlaying() || d->isEditing())
      {
         players++;
      }
   }

   if (players > max_who)
   {
      max_who = players;
   }
}

// stuff that has to be done on both a normal login, as well as on
// a hotboot login
void Character::do_on_login_stuff(void)
{
   add_to_bard_list();
   this->player->bad_pw_tries = 0;
   redo_hitpoints(this);
   redo_mana(this);
   redo_ki(this);
   do_inate_race_abilities();
   check_hw();
   /* Add a character's skill item's to the list. */
   this->player->skillchange = nullptr;
   this->spellcraftglyph = 0;
   for (int i = 0; i < MAX_WEAR; i++)
   {
      if (!this->equipment[i])
         continue;
      for (int a = 0; a < this->equipment[i]->num_affects; a++)
      {
         if (this->equipment[i]->affected[a].location >= 1000)
         {
            this->equipment[i]->next_skill = this->player->skillchange;
            this->player->skillchange = this->equipment[i];
            this->equipment[i]->next_skill = nullptr;
         }
      }
   }
   // add character base saves to saving throws
   for (int i = 0; i <= SAVE_TYPE_MAX; i++)
   {
      this->saves[i] += this->getLevel() / 4;
      this->saves[i] += this->player->saves_mods[i];
   }

   if (GET_TITLE(this) == nullptr)
   {
      GET_TITLE(this) = str_dup("is a virgin.");
   }

   if (GET_CLASS(this) == CLASS_MONK)
   {
      GET_AC(this) -= (this->getLevel() * 2);
   }
   GET_AC(this) -= this->has_skill(SKILL_COMBAT_MASTERY) / 2;

   GET_AC(this) -= GET_AC_METAS(this);

   if (affected_by_spell(INTERNAL_SLEEPING))
   {
      affect_from_char(this, INTERNAL_SLEEPING);
   }
   /* Set ISR's cause they're not saved...   */
   isr_set(this);
   this->altar = clan_altar();

   if (!IS_NPC(this) && this->getLevel() >= IMMORTAL)
   {
      this->player->holyLite = true;
      GET_COND(this, THIRST) = -1;
      GET_COND(this, FULL) = -1;
   }
   add_totem_stats(this);
   if (this->getLevel() < 5 && GET_AGE(this) < 21)
      char_to_room(this, real_room(200));
   else if (this->in_room >= 2)
      char_to_room(this, this->in_room);
   else if (this->getLevel() >= IMMORTAL)
      char_to_room(this, real_room(17));
   else
      char_to_room(this, real_room(START_ROOM));

   this->curLeadBonus = 0;
   this->changeLeadBonus = false;
   this->cRooms = 0;
   REMBIT(this->affected_by, AFF_BLACKJACK_ALERT);
   for (int i = 0; i < QUEST_MAX; i++)
   {
      this->player->quest_current[i] = -1;
      this->player->quest_current_ticksleft[i] = 0;
   }
   struct vault_data *vault = has_vault(GET_NAME(this));
   if (this->player->time.logon < 1172204700)
   {
      if (vault)
      {
         int adder = this->getLevel() - 50;
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
      if (vault->size < (unsigned)(this->getLevel() * 10))
      {
         logf(IMMORTAL, DC::LogChannel::LOG_BUG, "%s's vault reset from %d to %d during login.", GET_NAME(this), vault->size, this->getLevel() * 10);
         vault->size = this->getLevel() * 10;
      }

      save_vault(vault->owner);
   }

   if (this->player->time.logon < 1151506181)
   {
      this->player->quest_points = 0;
      for (int i = 0; i < QUEST_MAX_CANCEL; i++)
         this->player->quest_cancel[i] = 0;
      for (int i = 0; i < QUEST_TOTAL / ASIZE; i++)
         this->player->quest_complete[i] = 0;
   }
   if (this->player->time.logon < 1151504181)
      SET_BIT(this->misc, DC::LogChannel::CHANNEL_TELL);

   if (this->player->time.logon < 1171757100)
   {
      switch (GET_CLASS(this))
      {
      case CLASS_MAGE:
         GET_AC(this) += 100;
         break;
      case CLASS_DRUID:
         GET_AC(this) += 85;
         break;
      case CLASS_CLERIC:
         GET_AC(this) += 70;
         break;
      case CLASS_ANTI_PAL:
         GET_AC(this) += 55;
         break;
      case CLASS_THIEF:
         GET_AC(this) += 40;
         break;
      case CLASS_BARD:
         GET_AC(this) += 25;
         break;
      case CLASS_BARBARIAN:
         GET_AC(this) += 10;
         break;
      case CLASS_RANGER:
         GET_AC(this) -= 5;
         break;
      case CLASS_PALADIN:
         GET_AC(this) -= 20;
         break;
      case CLASS_WARRIOR:
         GET_AC(this) -= 35;
         break;
      case CLASS_MONK:
         GET_AC(this) -= 50;
         break;
      default:
         break;
      }
   }

   if (GET_CLASS(this) == CLASS_MONK && this->getLevel() > 10)
   {
      this->swapSkill(SKILL_SHIELDBLOCK, SKILL_DEFENSE);
   }
   if (GET_CLASS(this) == CLASS_PALADIN && this->getLevel() >= 41)
   {
      this->swapSkill(SPELL_ARMOR, SPELL_AEGIS);
      this->swapSkill(SPELL_POWER_HARM, SPELL_DIVINE_FURY);
   }
   if (GET_CLASS(this) == CLASS_RANGER && this->getLevel() > 9)
   {
      if (this->skills.contains(SKILL_SHIELDBLOCK))
      {
         this->swapSkill(SKILL_SHIELDBLOCK, SKILL_DODGE);
         this->setSkillMin(SKILL_DODGE, 50);
      }
   }
   if (GET_CLASS(this) == CLASS_ANTI_PAL && this->getLevel() >= 44)
   {
      this->swapSkill(SPELL_STONE_SKIN, SPELL_U_AEGIS);
   }
   if (GET_CLASS(this) == CLASS_BARD && this->getLevel() >= 30)
   {
      this->swapSkill(SKILL_BLUDGEON_WEAPONS, SKILL_STINGING_WEAPONS);
   }
   if (GET_CLASS(this) == CLASS_CLERIC && this->getLevel() >= 42)
   {
      this->swapSkill(SPELL_RESIST_FIRE, SPELL_RESIST_MAGIC);
      this->skills.erase(SPELL_RESIST_COLD);
   }
   if (GET_CLASS(this) == CLASS_MAGIC_USER)
   {
      this->skills.erase(SPELL_SLEEP);
      this->skills.erase(SPELL_RESIST_COLD);
      this->skills.erase(SPELL_KNOW_ALIGNMENT);
   }
   // Remove pick if they're no longer allowed to have it.
   if (GET_CLASS(this) == CLASS_THIEF && this->getLevel() < 22 && this->has_skill(SKILL_PICK_LOCK))
   {
      this->skills.erase(SKILL_PICK_LOCK);
   }
   if (GET_CLASS(this) == CLASS_BARD && this->has_skill(SKILL_HIDE))
   {
      this->skills.erase(SKILL_HIDE);
   }
   // Remove listsongs
   if (GET_CLASS(this) == CLASS_BARD && this->has_skill(SKILL_SONG_LIST_SONGS))
   {
      this->skills.erase(SKILL_SONG_LIST_SONGS);
   }
   // Replace shieldblock on barbs
   if (GET_CLASS(this) == CLASS_BARBARIAN && this->has_skill(SKILL_SHIELDBLOCK))
   {
      this->swapSkill(SKILL_SHIELDBLOCK, SKILL_DODGE);
   }
   // Replace eagle-eye on druids
   if (GET_CLASS(this) == CLASS_DRUID && this->has_skill(SPELL_EAGLE_EYE))
   {
      this->swapSkill(SPELL_EAGLE_EYE, SPELL_GHOSTWALK);
   }
   // Replace crushing on bards
   if (GET_CLASS(this) == CLASS_BARD && this->has_skill(SKILL_CRUSHING_WEAPONS))
   {
      this->swapSkill(SKILL_CRUSHING_WEAPONS, SKILL_WHIPPING_WEAPONS);
   }
   // Replace crushing on thieves
   if (GET_CLASS(this) == CLASS_THIEF && this->has_skill(SKILL_CRUSHING_WEAPONS))
   {
      this->swapSkill(SKILL_CRUSHING_WEAPONS, SKILL_STINGING_WEAPONS);
   }
   // Replace firestorm on antis
   if (GET_CLASS(this) == CLASS_ANTI_PAL && this->has_skill(SPELL_FIRESTORM))
   {
      this->swapSkill(SPELL_FIRESTORM, SPELL_LIFE_LEECH);
   }

   class_skill_defines *c_skills = this->get_skill_list();

   if (IS_MORTAL(this))
   {
      std::queue<skill_t> skills_to_delete = {};
      for (const auto &curr : this->skills)
      {
         if (curr.first < 600 && search_skills2(curr.first, c_skills) == -1 && search_skills2(curr.first, g_skills) == -1 && curr.first != META_REIMB && curr.first != NEW_SAVE)
         {
            logentry(QStringLiteral("Removing skill %1 from %2").arg(curr.first).arg(GET_NAME(this)), IMMORTAL, DC::LogChannel::LOG_PLAYER);
            // this->send(fmt::format("Removing skill {}\r\n", curr.first));
            skills_to_delete.push(curr.first);
         }
      }
      while (skills_to_delete.empty() == false)
      {
         this->skills.erase(skills_to_delete.front());
         skills_to_delete.pop();
      }
   }

   barb_magic_resist(this, 0, this->has_skill(SKILL_MAGIC_RESIST));
   /* meta reimbursement */
   if (!this->has_skill(META_REIMB))
   {
      learn_skill(META_REIMB, 1, 100);
      int new_ = MIN(r_new_meta_platinum_cost(0, hps_plats_spent()), r_new_meta_exp_cost(0, hps_exp_spent()));
      int ometa = GET_HP_METAS(this);
      GET_HP_METAS(this) = new_;
      GET_RAW_HIT(this) += new_ - ometa;
      new_ = MIN(r_new_meta_platinum_cost(0, mana_plats_spent()), r_new_meta_exp_cost(0, mana_exp_spent()));
      ometa = GET_MANA_METAS(this);
      GET_RAW_MANA(this) += new_ - ometa;
      GET_MANA_METAS(this) = new_;
      new_ = MIN(r_new_meta_platinum_cost(0, moves_plats_spent()), r_new_meta_exp_cost(0, moves_exp_spent()));
      ometa = GET_MOVE_METAS(this);
      GET_MOVE_METAS(this) = new_;
      GET_RAW_MOVE(this) += new_ - ometa;
   }
   /* end meta reimbursement */

   prepare_character_for_sixty(this);

   // Check for deleted characters listed in access list
   std::queue<QString> todelete;
   vault = has_vault(GET_NAME(this));
   if (vault)
   {
      for (vault_access_data *access = vault->access; access && access != (vault_access_data *)0x95959595; access = access->next)
      {
         if (!access->name.isEmpty())
         {
            if (!file_exists(QStringLiteral("%1/%2/%3").arg(SAVE_DIR).arg(access->name[0].toUpper()).arg(access->name)))
            {
               todelete.push(access->name);
            }
         }
      }
   }

   while (!todelete.empty())
   {
      logentry(QStringLiteral("Deleting %1 from %2's vault access list.\n").arg(todelete.front()).arg(GET_NAME(this)), 0, DC::LogChannel::LOG_MORTAL);
      remove_vault_access(todelete.front(), vault);
      todelete.pop();
   }

   if (this->getSetting("mode").startsWith("char"))
   {
      telnet_echo_off(this->desc);
      telnet_sga(this->desc);
   }
}

void Character::roll_and_display_stats(void)
{
   int x, a, b;
   char buf[MAX_STRING_LENGTH];

   for (x = 0; x <= 4; x++)
   {
      a = dice(3, 6);
      b = dice(6, 3);
      this->desc->stats->str[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      this->desc->stats->dex[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      this->desc->stats->con[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      this->desc->stats->tel[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      this->desc->stats->wis[x] = MAX(12 + number(0, 1), MAX(a, b));
   }

   /*
   For testing purposes
   this->desc->stats->str[0] = 13;
   this->desc->stats->dex[0] = 14;
   this->desc->stats->con[0] = 13;
   this->desc->stats->tel[0] = 12;
   this->desc->stats->wis[0] = 14;
   */

   SEND_TO_Q("\n\r  Choose from any of the following groups of abilities...     \n\r", this->desc);

   SEND_TO_Q("Group: 1     2     3     4     5\n\r", this->desc);
   sprintf(buf, "Str:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
           this->desc->stats->str[0], this->desc->stats->str[1], this->desc->stats->str[2],
           this->desc->stats->str[3], this->desc->stats->str[4]);
   SEND_TO_Q(buf, this->desc);
   sprintf(buf, "Dex:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
           this->desc->stats->dex[0], this->desc->stats->dex[1], this->desc->stats->dex[2],
           this->desc->stats->dex[3], this->desc->stats->dex[4]);
   SEND_TO_Q(buf, this->desc);
   sprintf(buf, "Con:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
           this->desc->stats->con[0], this->desc->stats->con[1], this->desc->stats->con[2],
           this->desc->stats->con[3], this->desc->stats->con[4]);
   SEND_TO_Q(buf, this->desc);
   sprintf(buf, "Int:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
           this->desc->stats->tel[0], this->desc->stats->tel[1], this->desc->stats->tel[2],
           this->desc->stats->tel[3], this->desc->stats->tel[4]);
   SEND_TO_Q(buf, this->desc);
   sprintf(buf, "Wis:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
           this->desc->stats->wis[0], this->desc->stats->wis[1], this->desc->stats->wis[2],
           this->desc->stats->wis[3], this->desc->stats->wis[4]);
   SEND_TO_Q(buf, this->desc);
   SEND_TO_Q("Choose a group <1-5>, or press return to reroll(Help <attribute> for more information) --> ", this->desc);
   telnet_ga(this->desc);

   WAIT_STATE(this, DC::PULSE_TIMER / 10);
}

int DC::exceeded_connection_limit(class Connection *new_conn)
{
   if (new_conn->getPeerOriginalAddress().isNull() || new_conn->getPeerAddress().isLoopback())
   {
      return false;
   }

   quint64 count = 0;
   QSet<Connection *> to_close_list;
   for (auto d = descriptor_list; d; d = d->next)
   {
      if (new_conn->getPeerOriginalAddress() == d->getPeerOriginalAddress())
      {
         count++;
         to_close_list.insert(d);
      }
   }

   if (count > getConnectionLimit())
   {
      SEND_TO_Q(QStringLiteral("Sorry, there are more than %1 connections from IP %2\r\n"
                               "already logged into Dark Castle.  If you have a valid reason\r\n"
                               "for having this many connections from one IP please let an imm\r\n"
                               "know and they will speak with you. Assuming this is an error and closing all connections.\r\n")
                    .arg(getConnectionLimit())
                    .arg(new_conn->getPeerOriginalAddress().toString()),
                new_conn);

      for (const auto &d : to_close_list)
      {
         logsocket(QStringLiteral("Closing socket %1 from IP %2 due to > %3 connections.").arg(d->desc_num).arg(d->getPeerOriginalAddress().toString()).arg(getConnectionLimit()));
         close_socket(d);
      }
      return true;
   }

   return false;
}

void Character::check_hw(void)
{
   heightweight(false);
   if (this->height > races[this->race].max_height)
   {
      logf(IMPLEMENTER, DC::LogChannel::LOG_BUG, "check_hw: %s's height %d > max %d. height set to max.", GET_NAME(this), GET_HEIGHT(this), races[this->race].max_height);
      this->height = races[this->race].max_height;
   }
   if (this->height < races[this->race].min_height)
   {
      logf(IMPLEMENTER, DC::LogChannel::LOG_BUG, "check_hw: %s's height %d < min %d. height set to min.", GET_NAME(this), GET_HEIGHT(this), races[this->race].min_height);
      this->height = races[this->race].min_height;
   }

   if (this->weight > races[this->race].max_weight)
   {
      logf(IMPLEMENTER, DC::LogChannel::LOG_BUG, "check_hw: %s's weight %d > max %d. weight set to max.", GET_NAME(this), GET_WEIGHT(this), races[this->race].max_weight);
      this->weight = races[this->race].max_weight;
   }
   if (this->weight < races[this->race].min_weight)
   {
      logf(IMPLEMENTER, DC::LogChannel::LOG_BUG, "check_hw: %s's weight %d < min %d. weight set to min.", GET_NAME(this), GET_WEIGHT(this), races[this->race].min_weight);
      this->weight = races[this->race].min_weight;
   }
   heightweight(true);
}

void Character::set_hw(void)
{
   this->height = number(races[this->race].min_height, races[this->race].max_height);
   // logf(ANGEL, DC::LogChannel::LOG_MORTAL, "%s's height set to %d", GET_NAME(this), GET_HEIGHT(this));
   this->weight = number(races[this->race].min_weight, races[this->race].max_weight);
   // logf(ANGEL, DC::LogChannel::LOG_MORTAL, "%s's weight set to %d", GET_NAME(this), GET_WEIGHT(this));
}

// Deal with sockets that haven't logged in yet.
void DC::nanny(class Connection *d, std::string arg)
{
   char buf[MAX_STRING_LENGTH];
   std::stringstream str_tmp;
   char tmp_name[20];
   char *password;
   Character *ch;
   int y;
   char badclssmsg[] = "You must choose a class that matches your stats. These are marked by a '*'.\n\rSelect a class-> ";
   unsigned selection = 0;
   auto &character_list = DC::getInstance()->character_list;
   char log_buf[MAX_STRING_LENGTH] = {};
   QString buffer;

   ch = d->character;
   arg.erase(0, arg.find_first_not_of(' '));

   if (!str_prefix("help", arg.c_str()) &&
       (STATE(d) == Connection::states::OLD_GET_CLASS ||
        STATE(d) == Connection::states::OLD_GET_RACE ||
        STATE(d) == Connection::states::OLD_CHOOSE_STATS ||
        STATE(d) == Connection::states::GET_CLASS ||
        STATE(d) == Connection::states::GET_RACE ||
        STATE(d) == Connection::states::GET_STATS))
   {
      arg.erase(0, 4);
      do_new_help(d->character, arg.data(), cmd_t::PAGING_HELP);
      return;
   }

   load_status_t ls{};
   switch (STATE(d))
   {

   default:
      logentry(QStringLiteral("Nanny: invalid STATE(d) == %1").arg(STATE(d)), 0, DC::LogChannel::LOG_BUG);
      close_socket(d);
      return;

   case Connection::states::PRE_DISPLAY_ENTRANCE:
      // _shouldn't_ get here, but if we do, let it fall through to Connection::states::DISPLAY_ENTRANCE
      // This is here to allow the mud to 'skip' this descriptor until the next pulse on
      // a new connection.  That allows the "GET" and "POST" from a webbrowser to get there.
      // no break;

   case Connection::states::DISPLAY_ENTRANCE:

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

      if (exceeded_connection_limit(d))
         break;

      if (wizlock)
      {
         SEND_TO_Q("The game is currently WIZLOCKED. Only immortals can connect at this time.\r\n", d);
      }
      SEND_TO_Q("What name for the roster? ", d);
      telnet_ga(d);
      STATE(d) = Connection::states::GET_NAME;

      // if they have already entered their name, drop through.  Otherwise stop and wait for input
      if (arg.empty())
      {
         break;
      }
      /* no break */

   case Connection::states::GET_PROXY:
      STATE(d) = Connection::states::GET_NAME;

      // If first line of text is a proxy header then construct Proxy
      // otherwise assume it's a name.
      if (QString(arg.c_str()).indexOf("PROXY ") == 0)
      {
         d->proxy = Proxy(arg.c_str());
         return;
      }
   case Connection::states::GET_NAME:

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
      //    logentry(str_tmp, 0, DC::LogChannel::LOG_MISC);

      // ch is allocated in load_char_obj
      ls = load_char_obj(d, tmp_name);
      if (ls == load_status_t::missing)
      {
         str_tmp << "../archive/" << tmp_name << ".gz";
         if (file_exists(str_tmp.str().c_str()))
         {
            SEND_TO_Q("That character is archived.\n\rPlease mail "
                      "imps@dcastle.org to request it be restored.\r\n",
                      d);
            close_socket(d);
            return;
         }
      }
      ch = d->character;

      // This is needed for "check_reconnect"  we free it later during load_char_obj
      // TODO - this is memoryleaking ch->getNameC().  Check if ch->getNameC() is not there before
      // doing it to fix it.  (No time to verify this now, so i'll do it later)
      ch->setName(str_dup(tmp_name));

      // if (isAllowedHost(d->getPeerOriginalAddress().toString().toStdString().c_str()))
      // SEND_TO_Q("You are logging in from an ALLOWED host.\r\n", d);

      if (!check_reconnect(d, tmp_name, false) && wizlock && !DC::getInstance()->isAllowedHost(d->getPeerOriginalAddress()))
      {
         SEND_TO_Q("The game is wizlocked.\r\n", d);
         close_socket(d);
         return;
      }

      if (ls == load_status_t::success)
      {
         /* Old player */
         SEND_TO_Q("Password: ", d);
         telnet_ga(d);
         STATE(d) = Connection::states::GET_OLD_PASSWORD;
         return;
      }
      else if (ls == load_status_t::missing)
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
         STATE(d) = Connection::states::CONFIRM_NEW_NAME;
         return;
      }
      else
      {
         SEND_TO_Q(QStringLiteral("There was an error loading %1").arg(tmp_name), d);
         close_socket(d);
         return;
      }
      break;

   case Connection::states::GET_OLD_PASSWORD:
      SEND_TO_Q("\n\r", d);

      // Default is to authenticate against character password
      password = ch->player->pwd;

      // If -P option passed and one of your other characters is an imp, allow this char with that imp's password
      if (DC::getInstance()->cf.allow_imp_password && DC::getInstance()->isAllowedHost(d->getPeerOriginalAddress()))
      {
         for (Connection *ad = DC::getInstance()->descriptor_list; ad && ad != (Connection *)0x95959595; ad = ad->next)
         {
            if (ad != d && d->getPeerOriginalAddress() == ad->getPeerOriginalAddress())
            {
               if (ad->character && ad->character->getLevel() == IMPLEMENTER && IS_PC(ad->character))
               {
                  password = ad->character->player->pwd;
                  logf(OVERSEER, DC::LogChannel::LOG_SOCKET, "Using %s's password for authentication.", GET_NAME(ad->character));
                  break;
               }
            }
         }
      }

      if (std::string(crypt(arg.c_str(), password)) != password)
      {
         SEND_TO_Q("Wrong password.\r\n", d);
         sprintf(log_buf, "%s wrong password: %s", GET_NAME(ch), d->getPeerOriginalAddress().toString().toStdString().c_str());
         logentry(log_buf, OVERSEER, DC::LogChannel::LOG_SOCKET);
         if ((ch = get_pc(GET_NAME(d->character))))
         {
            sprintf(log_buf, "$4$BWARNING: Someone just tried to log in as you with the wrong password.\r\n"
                             //           "Attempt was from %s.$R\r\n"
                             "(If it's only once or twice, you can ignore it.  If it's several dozen tries, let a god know.)\r\n");
            ch->send(log_buf);
         }
         else
         {
            if (d->character->player->bad_pw_tries > 100)
            {
               sprintf(log_buf, "%s has 100+ bad pw tries...", GET_NAME(d->character));
               logentry(log_buf, SERAPH, DC::LogChannel::LOG_SOCKET);
            }
            else
            {
               d->character->player->bad_pw_tries++;
               d->character->save_char_obj();
            }
         }
         close_socket(d);
         return;
      }

      check_playing(d, ch->getName());

      if (check_reconnect(d, ch->getName(), true))
         return;

      buffer = QStringLiteral("%1@%2 has connected.").arg(GET_NAME(ch)).arg(d->getPeerOriginalAddress().toString().toStdString().c_str());
      if (ch->getLevel() < ANGEL)
         logentry(buffer, OVERSEER, DC::LogChannel::LOG_SOCKET);
      else
         logentry(buffer, ch->getLevel(), DC::LogChannel::LOG_SOCKET);

      warn_if_duplicate_ip(ch);
      //    SEND_TO_Q(motd, d);
      if (ch->isMortalPlayer())
         d->character->send(motd);
      else
         d->character->send(imotd);

      clan_data *clan;
      if ((clan = get_clan(ch->clan)) && clan->clanmotd)
      {
         ch->sendln("$B----------------------------------------------------------------------$R");
         ch->send(clan->clanmotd);
         ch->sendln("$B----------------------------------------------------------------------$R");
      }

      SEND_TO_Q(QStringLiteral("\n\rIf you have read this motd, press Return.\n\rLast connected from:\n\r%1\n\r").arg(ch->player->last_site), d);
      telnet_ga(d);

      if (d->character->player->bad_pw_tries)
      {
         sprintf(buf, "\r\n\r\n$4$BYou have had %d wrong passwords entered since your last complete login.$R\r\n\r\n", d->character->player->bad_pw_tries);
         SEND_TO_Q(buf, d);
      }
      DC::getInstance()->TheAuctionHouse.CheckForSoldItems(d->character);
      STATE(d) = Connection::states::READ_MOTD;
      break;

   case Connection::states::CONFIRM_NEW_NAME:
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

         if (isbanned(d->getPeerOriginalAddress()) >= BAN_NEW)
         {
            sprintf(buf, "Request for new character %s denied from [%s] (siteban)",
                    GET_NAME(d->character), d->getPeerOriginalAddress().toString().toStdString().c_str());
            logentry(buf, OVERSEER, DC::LogChannel::LOG_SOCKET);
            SEND_TO_Q("Sorry, new chars are not allowed from your site.\r\n"
                      "Questions may be directed to imps@dcastle.org\n\r",
                      d);
            STATE(d) = Connection::states::CLOSE;
            return;
         }
         sprintf(buf, "New character.\n\rGive me a password for %s: ", GET_NAME(ch));
         SEND_TO_Q(buf, d);
         telnet_ga(d);
         STATE(d) = Connection::states::GET_NEW_PASSWORD;
         // at this point, player hasn't yet been created.  So we're going to go ahead and
         // allocate it since a new character is obviously a PC
         ch->player = new Player;
         ch->setType(Character::Type::Player);
         assert(ch->player);
         break;

      case 'n':
      case 'N':
         SEND_TO_Q("Ok, what IS it, then? ", d);
         telnet_ga(d);
         // TODO - double check this to make sure we're free'ing properly
         ch->setName("");
         delete d->character;
         d->character = nullptr;
         STATE(d) = Connection::states::GET_NAME;
         break;

      default:
         SEND_TO_Q("Please type y or n: ", d);
         telnet_ga(d);
         break;
      }
      break;

   case Connection::states::GET_NEW_PASSWORD:
      SEND_TO_Q("\r\n", d);

      if (arg.length() < 6)
      {
         SEND_TO_Q("Password must be at least six characters long.\n\rPassword: ", d);
         telnet_ga(d);
         return;
      }

      strncpy(ch->player->pwd, crypt(arg.c_str(), ch->getNameC()), PASSWORD_LEN);
      ch->player->pwd[PASSWORD_LEN] = '\0';
      SEND_TO_Q("Please retype password: ", d);
      telnet_ga(d);
      STATE(d) = Connection::states::CONFIRM_NEW_PASSWORD;
      break;

   case Connection::states::CONFIRM_NEW_PASSWORD:
      SEND_TO_Q("\n\r", d);

      if (std::string(crypt(arg.c_str(), ch->player->pwd)) != ch->player->pwd)
      {
         SEND_TO_Q("Passwords don't match.\n\rRetype password: ", d);
         telnet_ga(d);
         STATE(d) = Connection::states::GET_NEW_PASSWORD;
         return;
      }

   case Connection::states::QUESTION_ANSI:
      SEND_TO_Q("Do you want ANSI color (y/n)? ", d);
      telnet_ga(d);
      STATE(d) = Connection::states::GET_ANSI;
      break;

   case Connection::states::GET_ANSI:
      if (arg.empty())
      {
         STATE(d) = Connection::states::QUESTION_ANSI;
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
         STATE(d) = Connection::states::QUESTION_ANSI;
         return;
      }

      STATE(d) = Connection::states::QUESTION_SEX;
      break;

   case Connection::states::QUESTION_SEX:
      SEND_TO_Q("What is your sex (m/f)? ", d);
      telnet_ga(d);
      STATE(d) = Connection::states::GET_NEW_SEX;
      break;

   case Connection::states::GET_NEW_SEX:
      if (arg.empty())
      {
         SEND_TO_Q("That's not a sex.\r\n", d);
         STATE(d) = Connection::states::QUESTION_SEX;
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
         STATE(d) = Connection::states::QUESTION_SEX;
         return;
      }

      /*
            if (!isAllowedHost(d->getPeerOriginalAddress().toString().toStdString().c_str()) && DC::getInstance()->cf.allow_newstatsys == false)
            {
               STATE(d) = Connection::states::OLD_STAT_METHOD;
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
      STATE(d) = Connection::states::QUESTION_STAT_METHOD;
      return;

   case Connection::states::QUESTION_STAT_METHOD:
      SEND_TO_Q("\r\n", d);
      SEND_TO_Q("1. Pick race, class then assign points to attributes. (new method)\r\n", d);
      SEND_TO_Q("2. Roll virtual dice for attributes then pick race and class. (old method)\r\n", d);
      SEND_TO_Q("What is your choice (1,2)? ", d);
      telnet_ga(d);
      STATE(d) = Connection::states::GET_STAT_METHOD;
      break;

   case Connection::states::GET_STAT_METHOD:
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
         STATE(d) = Connection::states::NEW_STAT_METHOD;
      }
      else if (selection == 2)
      {
         STATE(d) = Connection::states::OLD_STAT_METHOD;
      }
      else
      {
         STATE(d) = Connection::states::QUESTION_STAT_METHOD;
      }
      break;

   case Connection::states::NEW_STAT_METHOD:
      STATE(d) = Connection::states::QUESTION_RACE;
      break;

   case Connection::states::QUESTION_RACE:
      show_question_race(d);

      STATE(d) = Connection::states::GET_RACE;
      break;

   case Connection::states::GET_RACE:
      if (handle_get_race(d, arg) == true)
      {
         STATE(d) = Connection::states::QUESTION_CLASS;
      }
      else
      {
         STATE(d) = Connection::states::QUESTION_RACE;
      }
      break;

   case Connection::states::QUESTION_CLASS:
      show_question_class(d);

      STATE(d) = Connection::states::GET_CLASS;
      break;

   case Connection::states::GET_CLASS:
      if (handle_get_class(d, arg) == false)
      {
         if (STATE(d) != Connection::states::QUESTION_RACE)
         {
            STATE(d) = Connection::states::QUESTION_CLASS;
         }
      }
      else
      {
         STATE(d) = Connection::states::QUESTION_STATS;
      }
      break;

   case Connection::states::QUESTION_STATS:
      show_question_stats(d);

      STATE(d) = Connection::states::GET_STATS;
      break;

   case Connection::states::GET_STATS:
      if (handle_get_stats(d, arg) == false)
      {
         STATE(d) = Connection::states::QUESTION_STATS;
      }
      else
      {
         STATE(d) = Connection::states::NEW_PLAYER;
      }
      break;

   case Connection::states::OLD_STAT_METHOD:
      if (ch->desc->stats != nullptr)
      {
         delete ch->desc->stats;
      }
      ch->desc->stats = new stat_data;

      STATE(d) = Connection::states::OLD_CHOOSE_STATS;
      arg.clear();
      // break;  no break on purpose...might as well do this now.  no point in waiting

   case Connection::states::OLD_CHOOSE_STATS:

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
         SEND_TO_Q("\n\rChoose a race(races you can select are marked with a *).\r\n", d);
         sprintf(buf, "  %c1: Human\n\r  %c2: Elf\n\r  %c3: Dwarf\n\r"
                      "  %c4: Hobbit\n\r  %c5: Pixie\n\r  %c6: Ogre\n\r"
                      "  %c7: Gnome\r\n  %c8: Orc\r\n  %c9: Troll\r\n"
                      "\n\rSelect a race(Type help <race> for more information)-> ",
                 is_race_eligible(ch, 1) ? '*' : ' ', is_race_eligible(ch, 2) ? '*' : ' ', is_race_eligible(ch, 3) ? '*' : ' ', is_race_eligible(ch, 4) ? '*' : ' ', is_race_eligible(ch, 5) ? '*' : ' ', is_race_eligible(ch, 6) ? '*' : ' ',
                 is_race_eligible(ch, 7) ? '*' : ' ', is_race_eligible(ch, 8) ? '*' : ' ', is_race_eligible(ch, 9) ? '*' : ' ');

         SEND_TO_Q(buf, d);
         telnet_ga(d);
         STATE(d) = Connection::states::OLD_GET_RACE;
         break;
      }
      ch->roll_and_display_stats();
      break;

   case Connection::states::OLD_GET_RACE:
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
            ch->sendln("Your stats do not qualify for that race.");
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
            ch->sendln("Your stats do not qualify for that race.");
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
            ch->sendln("Your stats do not qualify for that race.");
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
            ch->sendln("Your stats do not qualify for that race.");
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
            ch->sendln("Your stats do not qualify for that race.");
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
            ch->sendln("Your stats do not qualify for that race.");
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
            ch->sendln("Your stats do not qualify for that race.");
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
            ch->sendln("Your stats do not qualify for that race.");
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

      ch->set_hw();
      SEND_TO_Q("\n\rA '*' denotes a class that fits your chosen stats.\r\n", d);
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
      STATE(d) = Connection::states::OLD_GET_CLASS;
      break;

   case Connection::states::OLD_GET_CLASS:
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
      STATE(d) = Connection::states::NEW_PLAYER;
      break;

   case Connection::states::NEW_PLAYER:

      init_char(ch);

      sprintf(log_buf, "%s@%s new player.", GET_NAME(ch), d->getPeerOriginalAddress().toString().toStdString().c_str());
      logentry(log_buf, OVERSEER, DC::LogChannel::LOG_SOCKET);
      SEND_TO_Q("\n\r", d);
      SEND_TO_Q(motd, d);
      SEND_TO_Q("\n\rIf you have read this motd, press Return.", d);
      telnet_ga(d);

      STATE(d) = Connection::states::READ_MOTD;
      break;

   case Connection::states::READ_MOTD:
      SEND_TO_Q(menu, d);
      telnet_ga(d);
      STATE(d) = Connection::states::SELECT_MENU;
      break;

   case Connection::states::SELECT_MENU:
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
         d = nullptr;
         break;

      case '1':
         // I believe this is here to stop a dupe bug
         // by logging in twice, and leaving one at the password: prompt
         if (ch->getLevel() > 0)
         {
            strcpy(tmp_name, GET_NAME(ch));
            free_char(d->character, Trace("nanny Connection::states::SELECT_MENU 1"));
            d->character = 0;
            load_char_obj(d, tmp_name);
            ch = d->character;

            if (!DC::getInstance()->cf.implementer.isEmpty())
            {
               if (QString(GET_NAME(ch)).compare(DC::getInstance()->cf.implementer, Qt::CaseInsensitive) == 0)
               {
                  ch->setLevel(110);
               }
            }

            if (!ch)
            {
               write_to_descriptor(d->descriptor, "It seems your character has been deleted during logon, or you just experienced some obscure bug.");
               close_socket(d);
               d = nullptr;
               break;
            }
         }
         unique_scan(ch);
         if (ch->getGold() > 1000000000)
         {
            sprintf(log_buf, "%s has more than a billion gold. Bugged?", GET_NAME(ch));
            logentry(log_buf, 100, DC::LogChannel::LOG_WARNING);
         }
         if (GET_BANK(ch) > 1000000000)
         {
            sprintf(log_buf, "%s has more than a billion gold in the bank. Rich fucker or bugged.", GET_NAME(ch));
            logentry(log_buf, 100, DC::LogChannel::LOG_WARNING);
         }
         ch->sendln("\n\rWelcome to Dark Castle.");
         character_list.insert(ch);

         if (IS_AFFECTED(ch, AFF_ITEM_REMOVE))
         {
            REMBIT(ch->affected_by, AFF_ITEM_REMOVE);
            ch->sendln("\r\n$I$B$4***WARNING*** Items you were previously wearing have been moved to your inventory, please check before moving out of a safe room.$R");
         }

         ch->do_on_login_stuff();

         if (ch->getLevel() < OVERSEER)
            clan_login(ch);

         act("$n has entered the game.", ch, 0, 0, TO_ROOM, INVIS_NULL);
         if (!GET_SHORT_ONLY(ch))
            GET_SHORT_ONLY(ch) = str_dup(GET_NAME(ch));
         DC::getInstance()->update_wizlist(ch);
         ch->check_maxes(); // Check skill maxes.

         STATE(d) = Connection::states::PLAYING;
         update_max_who();

         if (ch->getLevel() == 0)
         {
            do_start(ch);
            do_new_help(ch, "new");
         }
         do_look(ch, "");
         {
            if (ch->getLevel() >= 40 && DC::getInstance()->DCVote.IsActive() && !DC::getInstance()->DCVote.HasVoted(ch))
            {
               send_to_char("\n\rThere is an active vote in which you have not yet voted.\r\n"
                            "Enter \"vote\" to see details\n\r\n\r",
                            ch);
            }
         }
         extern void zap_eq_check(Character * ch);
         zap_eq_check(ch);
         break;

      case '2':
         SEND_TO_Q("Enter a text you'd like others to see when they look at you.\r\n"
                   "Terminate with '/s' on a new line.\r\n",
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
         STATE(d) = Connection::states::EXDSCR;
         break;

      case '3':
         SEND_TO_Q("Enter current password: ", d);
         telnet_ga(d);
         STATE(d) = Connection::states::CONFIRM_PASSWORD_CHANGE;
         break;

      case '4':
         // delete this character
         if (d->character->getLevel() >= 20)
         {
            SEND_TO_Q("\r\nOnly characters under level 20 can be self-deleted.\r\n", d);
            STATE(d) = Connection::states::SELECT_MENU;
            SEND_TO_Q(menu, d);
            telnet_ga(d);
         }
         else
         {
            SEND_TO_Q("This will _permanently_ erase you.\n\rType ERASE ME if this is really what you want: ", d);
            telnet_ga(d);
            STATE(d) = Connection::states::DELETE_CHAR;
         }
         break;

      default:
         SEND_TO_Q(menu, d);
         telnet_ga(d);
         break;
      }
      break;

   case Connection::states::ARCHIVE_CHAR:
      if (arg == "ARCHIVE ME")
      {
         str_tmp << GET_NAME(d->character);
         SEND_TO_Q("\n\rCharacter Archived.\r\n", d);
         DC::getInstance()->update_wizlist(d->character);
         close_socket(d);
         util_archive(str_tmp.str().c_str(), 0);
      }
      else
      {
         STATE(d) = Connection::states::SELECT_MENU;
         SEND_TO_Q(menu, d);
      }
      break;

   case Connection::states::DELETE_CHAR:
      if (arg == "ERASE ME")
      {
         sprintf(buf, "%s just deleted themself.", d->character->getNameC());
         logentry(buf, IMMORTAL, DC::LogChannel::LOG_MORTAL);

         DC::getInstance()->TheAuctionHouse.HandleDelete(d->character->getName());
         // To remove the vault from memory
         remove_familiars(d->character->getName(), SELFDELETED);
         remove_vault(d->character->getName(), SELFDELETED);
         if (d->character->clan)
         {
            remove_clan_member(d->character->clan, d->character);
         }
         remove_character(d->character->getName(), SELFDELETED);

         d->character->setLevel(1);
         DC::getInstance()->update_wizlist(d->character);
         close_socket(d);
         d = nullptr;
      }
      else
      {
         STATE(d) = Connection::states::SELECT_MENU;
         SEND_TO_Q(menu, d);
         telnet_ga(d);
      }
      break;

   case Connection::states::CONFIRM_PASSWORD_CHANGE:
      SEND_TO_Q("\n\r", d);
      if (std::string(crypt(arg.c_str(), ch->player->pwd)) == ch->player->pwd)
      {
         SEND_TO_Q("Enter a new password: ", d);
         telnet_ga(d);
         STATE(d) = Connection::states::RESET_PASSWORD;
         break;
      }
      else
      {
         SEND_TO_Q("Incorrect.", d);
         STATE(d) = Connection::states::SELECT_MENU;
         SEND_TO_Q(menu, d);
      }
      break;

   case Connection::states::RESET_PASSWORD:
      SEND_TO_Q("\n\r", d);

      if (arg.length() < 6)
      {
         SEND_TO_Q("Password must be at least six characters long.\n\rPassword: ", d);
         telnet_ga(d);
         return;
      }
      strncpy(ch->player->pwd, crypt(arg.c_str(), ch->getNameC()), PASSWORD_LEN);
      ch->player->pwd[PASSWORD_LEN] = '\0';
      SEND_TO_Q("Please retype password: ", d);
      telnet_ga(d);
      STATE(d) = Connection::states::CONFIRM_RESET_PASSWORD;
      break;

   case Connection::states::CONFIRM_RESET_PASSWORD:
      SEND_TO_Q("\n\r", d);

      if (std::string(crypt(arg.c_str(), ch->player->pwd)) != ch->player->pwd)
      {
         SEND_TO_Q("Passwords don't match.\n\rRetype password: ", d);
         telnet_ga(d);
         STATE(d) = Connection::states::RESET_PASSWORD;
         return;
      }

      SEND_TO_Q("\n\rDone.\r\n", d);
      SEND_TO_Q(menu, d);
      STATE(d) = Connection::states::SELECT_MENU;
      if (ch->getLevel() > 1)
      {
         char blah1[50], blah2[50];
         // this prevents a dupe bug
         strcpy(blah1, GET_NAME(ch));
         strcpy(blah2, ch->player->pwd);
         free_char(d->character, Trace("nanny Connection::states::CONFIRM_RESET_PASSWORD"));
         d->character = 0;
         load_char_obj(d, blah1);
         ch = d->character;
         strcpy(ch->player->pwd, blah2);
         ch->save_char_obj();
         sprintf(log_buf, "%s password changed", GET_NAME(ch));
         logentry(log_buf, SERAPH, DC::LogChannel::LOG_SOCKET);
      }

      break;
   case Connection::states::CLOSE:
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
bool check_deny(class Connection *d, char *name)
{
   FILE *fpdeny = nullptr;
   char strdeny[MAX_INPUT_LENGTH];
   char bufdeny[MAX_STRING_LENGTH];

   sprintf(strdeny, "%s/%c/%s.deny", SAVE_DIR, UPPER(name[0]), name);
   if ((fpdeny = fopen(strdeny, "rb")) == nullptr)
      return false;
   fclose(fpdeny);

   char log_buf[MAX_STRING_LENGTH] = {};
   sprintf(log_buf, "Denying access to player %s@%s.", name, d->getPeerOriginalAddress().toString().toStdString().c_str());
   logentry(log_buf, ARCHANGEL, DC::LogChannel::LOG_MORTAL);
   file_to_string(strdeny, bufdeny);
   SEND_TO_Q(bufdeny, d);
   close_socket(d);
   return true;
}

// Look for link-dead player to reconnect.
bool check_reconnect(class Connection *d, QString name, bool fReconnect)
{
   if (!DC::getInstance()->death_list.empty())
   {
      DC::getInstance()->removeDead();
   }
   const auto &character_list = DC::getInstance()->character_list;
   for (const auto &tmp_ch : character_list)
   {
      if (IS_NPC(tmp_ch) || tmp_ch->desc != nullptr)
         continue;

      if (str_cmp(GET_NAME(d->character), GET_NAME(tmp_ch)))
         continue;

      //      if(fReconnect == false)
      //      {
      // TODO - why are we doing this?  we load the password doing load_char_obj
      // unless someone changed their password and didn't save this doesn't seem useful
      // removed 8/29/02..i think this might be related to the bug causing people
      // to morph into other people
      // if(d->character->player)
      //  strncpy( d->character->player->pwd, tmp_ch->player->pwd, PASSWORD_LEN );
      //      }
      //      else {

      if (fReconnect == true)
      {
         free_char(d->character, Trace("check_reconnect"));
         d->character = tmp_ch;
         tmp_ch->desc = d;
         tmp_ch->timer = 0;
         tmp_ch->sendln("Reconnecting.");

         QString log_buf = QStringLiteral("%1@%2 has reconnected.").arg(GET_NAME(tmp_ch)).arg(d->getPeerOriginalAddress().toString());
         act("$n has reconnected and is ready to kick ass.", tmp_ch, 0, 0, TO_ROOM, INVIS_NULL);

         if (tmp_ch->isMortalPlayer())
         {
            logentry(log_buf, COORDINATOR, DC::LogChannel::LOG_SOCKET);
         }
         else
         {
            logentry(log_buf, tmp_ch->getLevel(), DC::LogChannel::LOG_SOCKET);
         }

         STATE(d) = Connection::states::PLAYING;

         if (tmp_ch->getSetting("mode").startsWith("char"))
         {
            telnet_echo_off(d);
            telnet_sga(d);
         }
      }
      return true;
   }
   return false;
}

/*
 * Check if already playing (on an open descriptor.)
 */
bool check_playing(class Connection *d, QString name)
{
   class Connection *dold, *next_d;
   Character *compare = 0;

   for (dold = DC::getInstance()->descriptor_list; dold; dold = next_d)
   {
      next_d = dold->next;

      if ((dold == d) || (dold->character == 0))
         continue;

      compare = ((dold->original != 0) ? dold->original : dold->character);

      // If this is the case we fail our precondition to str_cmp
      if (name.isEmpty())
         continue;
      if (GET_NAME(compare) == 0)
         continue;

      if (name != GET_NAME(compare))
         continue;

      if (STATE(dold) == Connection::states::GET_NAME)
         continue;

      if (STATE(dold) == Connection::states::GET_OLD_PASSWORD)
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
void add_command_lag(Character *ch, int amount)
{
   if (ch->isMortalPlayer())
      ch->timer += amount;
}

int check_command_lag(Character *ch)
{
   return ch->timer;
}

void remove_command_lag(Character *ch)
{
   ch->timer = 0;
}

void update_characters()
{
   int tmp, retval;
   char log_msg[MAX_STRING_LENGTH], dammsg[MAX_STRING_LENGTH];
   struct affected_type af;

   const auto &character_list = DC::getInstance()->character_list;
   for (const auto &i : character_list)
   {

      if (i->brace_at) // if character is bracing
      {
         if (!charge_moves(i, SKILL_BATTERBRACE, 0.5) || !is_bracing(i, i->brace_at))
         {
            do_brace(i, "");
         }
         else
         {
            csendf(i, "You strain your muscles keeping the %s closed.\r\n", fname(i->brace_at->keyword).toStdString().c_str());
            act("$n strains $s muscles keeping the $F blocked.", i, 0, i->brace_at->keyword, TO_ROOM, 0);
         }
      }
      if (IS_AFFECTED(i, AFF_POISON) && !(i->affected_by_spell(SPELL_POISON)))
      {
         logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Player %s affected by poison but not under poison spell. Removing poison affect.", i->getNameC());
         REMBIT(i->affected_by, AFF_POISON);
      }

      // handle poison
      if (IS_AFFECTED(i, AFF_POISON) && !i->fighting && i->affected_by_spell(SPELL_POISON) && i->affected_by_spell(SPELL_POISON)->location == APPLY_NONE)
      {
         int tmp = number(1, 2) + i->affected_by_spell(SPELL_POISON)->duration;
         if (get_saves(i, SAVE_TYPE_POISON) > number(1, 101))
         {
            tmp *= get_saves(i, SAVE_TYPE_POISON) / 100;
            i->sendln("You feel very sick, but resist the $2poison's$R damage.");
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
      if (IS_PC(i) && i->isMortalPlayer() && DC::getInstance()->world[i->in_room].sector_type == SECT_UNDERWATER && !(i->affected_by_spell(SPELL_WATER_BREATHING) || IS_AFFECTED(i, AFF_WATER_BREATHING) || i->affected_by_spell(SKILL_SONG_SUBMARINERS_ANTHEM)))
      {
         tmp = GET_MAX_HIT(i) / 5;
         sprintf(log_msg, "%s drowned in room %d.", GET_NAME(i), DC::getInstance()->world[i->in_room].number);
         retval = noncombat_damage(i, tmp, "You gasp your last breath and everything goes dark...", "$n stops struggling as $e runs out of oxygen.", log_msg, KILL_DROWN);
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
         if (i->isMortalPlayer())
            i->timer--;
         else
            i->timer = 0;
      }

      i->shotsthisround = 0;

      if (IS_AFFECTED(i, AFF_CMAST_WEAKEN))
         REMBIT(i->affected_by, AFF_CMAST_WEAKEN);
      affect_from_char(i, SKILL_COMBAT_MASTERY);

      // perseverance stuff
      if (i->affected_by_spell(SKILL_PERSEVERANCE))
      {
         affect_from_char(i, SKILL_PERSEVERANCE_BONUS);
         af.type = SKILL_PERSEVERANCE_BONUS;
         af.duration = -1;
         af.modifier = 0 - ((2 + i->affected_by_spell(SKILL_PERSEVERANCE)->modifier / 10) * (1 + i->affected_by_spell(SKILL_PERSEVERANCE)->modifier / 11 - i->affected_by_spell(SKILL_PERSEVERANCE)->duration));
         af.location = APPLY_AC;
         af.bitvector = -1;
         affect_to_char(i, &af);

         i->incrementMove(5 * (1 + i->affected_by_spell(SKILL_PERSEVERANCE)->modifier / 11 - i->affected_by_spell(SKILL_PERSEVERANCE)->duration));
      }
   }
}

void check_silence_beacons(void)
{
   Object *obj, *tmp_obj;

   for (obj = DC::getInstance()->object_list; obj; obj = tmp_obj)
   {
      tmp_obj = obj->next;
      if (DC::getInstance()->obj_index[obj->item_number].virt == SILENCE_OBJ_NUMBER)
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
   Object *obj, *tmp_obj;
   Character *ch = nullptr, *tmp_ch, *next_ch;
   int align, amount, spl = 0;
   char buf[MAX_STRING_LENGTH];

   if (pulseType == DC::PULSE_REGEN)
   {
      for (obj = DC::getInstance()->object_list; obj; obj = tmp_obj)
      {
         tmp_obj = obj->next;
         if (DC::getInstance()->obj_index[obj->item_number].virt == CONSECRATE_OBJ_NUMBER)
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
                           csendf(ch, "You sense your consecration of %s has ended.\r\n", DC::getInstance()->world[obj->in_room].name);
                        else
                           ch->sendln("Runes upon the ground glow brightly, then fade to nothing.\r\nYour holy consecration has ended.");
                     }
                     else
                     {
                        if (ch->in_room != obj->in_room)
                           csendf(ch, "You sense your desecration of %s has ended.\r\n", DC::getInstance()->world[obj->in_room].name);
                        else
                           ch->sendln("The runes upon the ground shatter with a burst of magic!\r\nYour unholy desecration has ended.");
                     }
                  }
               }
               for (tmp_ch = DC::getInstance()->world[obj->in_room].people; tmp_ch; tmp_ch = next_ch)
               {
                  next_ch = tmp_ch->next_in_room;
                  if (tmp_ch == ch)
                  {
                     continue;
                  }

                  if (tmp_ch->isImmortalPlayer())
                  {
                     continue;
                  }

                  if (spl == SPELL_CONSECRATE)
                  {
                     if (tmp_ch->affected_by_spell(SPELL_DETECT_GOOD) && tmp_ch->affected_by_spell(SPELL_DETECT_GOOD)->modifier >= 80)
                        tmp_ch->sendln("Runes upon the ground glow brightly, then fade to nothing.\r\nThe holy consecration here has ended.");
                  }
                  else
                  {
                     if (tmp_ch->affected_by_spell(SPELL_DETECT_EVIL) && tmp_ch->affected_by_spell(SPELL_DETECT_EVIL)->modifier >= 80)
                        if (ch && tmp_ch != ch)
                        {
                           send_damage("Runes upon the ground glow softly as your holy consecration heals $N of | damage.", ch, 0, tmp_ch, buf, "Runes upon the ground glow softly as your holy consecration heals $N.", TO_CHAR);
                        }
                     tmp_ch->sendln("The runes upon the ground shatter with a burst of magic!\r\nThe unholy desecration has ended.");
                  }
               }
               extract_obj(obj);
            }
         }
      }
   }
   else if (pulseType == DC::PULSE_TENSEC)
   {
      for (obj = DC::getInstance()->object_list; obj; obj = tmp_obj)
      {
         tmp_obj = obj->next;
         if (DC::getInstance()->obj_index[obj->item_number].virt == CONSECRATE_OBJ_NUMBER)
         {
            spl = obj->obj_flags.value[0];
            if (charExists(obj->obj_flags.origin))
            {
               ch = obj->obj_flags.origin;
            }
            for (tmp_ch = DC::getInstance()->world[obj->in_room].people; tmp_ch; tmp_ch = next_ch)
            {
               next_ch = tmp_ch->next_in_room;
               if (tmp_ch->isImmortalPlayer())
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
                     else if (tmp_ch->affected_by_spell(SPELL_DETECT_GOOD) && tmp_ch->affected_by_spell(SPELL_DETECT_GOOD)->modifier >= 80)
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
                        if (IS_PC(tmp_ch))
                        {
                           act("The strength of $N's desecration proves fatal and the world fades to black...", tmp_ch, 0, ch, TO_CHAR, 0);
                           tmp_ch->sendln("You have been KILLED!!\n\r");
                        }
                        group_gain(ch, tmp_ch);
                        fight_kill(ch, tmp_ch, TYPE_CHOOSE, SPELL_DESECRATE);
                        continue;
                     }
                     sprintf(buf, "%d", amount);
                     tmp_ch->removeHP(amount);
                     if (tmp_ch == ch)
                        send_damage("The runes upon the ground hum ominously as your unholy desecration injures you, dealing | damage.", tmp_ch, 0, 0, buf, "The runes upon the ground hum ominously as your unholy desecration injures you. ", TO_CHAR);
                     else if (tmp_ch->affected_by_spell(SPELL_DETECT_GOOD) && tmp_ch->affected_by_spell(SPELL_DETECT_GOOD)->modifier >= 80)
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
                     else if (tmp_ch->affected_by_spell(SPELL_DETECT_EVIL) && tmp_ch->affected_by_spell(SPELL_DETECT_EVIL)->modifier >= 80)
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
                        if (IS_PC(tmp_ch))
                        {
                           act("The strength of $N's consecration proves fatal and the world fades to black...", tmp_ch, 0, ch, TO_CHAR, 0);
                           tmp_ch->sendln("You have been KILLED!!\n\r");
                        }
                        group_gain(ch, tmp_ch);
                        fight_kill(ch, tmp_ch, TYPE_CHOOSE, SPELL_CONSECRATE);
                        continue;
                     }
                     sprintf(buf, "%d", amount);
                     tmp_ch->removeHP(amount);
                     if (tmp_ch == ch)
                        send_damage("The runes upon the ground glow softly as your holy consecration injures you, dealing | damage.", tmp_ch, 0, 0, buf, "The runes upon the ground glow softly as your holy consecration injures you. ", TO_CHAR);
                     else if (tmp_ch->affected_by_spell(SPELL_DETECT_GOOD) && tmp_ch->affected_by_spell(SPELL_DETECT_GOOD)->modifier >= 80)
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
bool on_forbidden_name_list(const char *name)
{
   FILE *nameList;
   char buf[MAX_STRING_LENGTH + 1];
   bool found = false;
   int i;

   nameList = fopen(FORBIDDEN_NAME_FILE, "ro");
   if (!nameList)
   {
      logentry(QStringLiteral("Failed to open forbidden name file!"), 0, DC::LogChannel::LOG_MISC);
      return false;
   }
   else
   {
      while (fgets(buf, MAX_STRING_LENGTH, nameList) && !found)
      {
         /* chop off trailing \n */
         if ((i = strlen(buf)) > 0)
            buf[i - 1] = '\0';
         if (!str_cmp(name, buf))
            found = true;
      }
      fclose(nameList);
   }
   return found;
}

void show_question_race(Connection *d)
{
   if (d == nullptr || d->character == nullptr)
   {
      return;
   }

   Character *ch = d->character;
   std::string buffer, races_buffer;
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
      ch->undo_race_saves();
   }
   buffer += "Type 1-" + std::to_string(MAX_PC_RACE) + "," + races_buffer + " or help <keyword>: ";
   SEND_TO_Q(buffer.c_str(), d);
   telnet_ga(d);
}

bool handle_get_race(Connection *d, std::string arg)
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

void show_question_class(Connection *d)
{
   if (d == nullptr || d->character == nullptr)
   {
      return;
   }

   Character *ch = d->character;
   std::string buffer, classes_buffer;
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
            classes_buffer += std::string(pc_clss_types3[clss]);
            if (clss < CLASS_MAX_PROD)
            {
               classes_buffer += ",";
            }
         }
      }
   }

   buffer += "Type 'back' to go back and pick a different race.\r\n";
   buffer += "Type '1-" + std::to_string(CLASS_MAX_PROD) + "," + classes_buffer + "' or 'help keyword': ";
   SEND_TO_Q(buffer.c_str(), d);
   telnet_ga(d);
}

bool handle_get_class(Connection *d, std::string arg)
{
   if (d == nullptr || d->character == nullptr || arg == "")
   {
      return false;
   }

   if (arg == "back")
   {
      STATE(d) = Connection::states::QUESTION_RACE;
      return false;
   }

   const Character *ch = d->character;

   for (unsigned clss = 1; clss <= CLASS_MAX_PROD; clss++)
   {
      if (std::string(pc_clss_types[clss]) == std::string(arg) || std::string(pc_clss_types3[clss]) == std::string(arg))
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

void show_question_stats(Connection *d)
{
   if (d == nullptr || d->character == nullptr)
   {
      return;
   }

   if (d->stats == nullptr)
   {
      d->stats = new stat_data;

      Character *ch = d->character;
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

   Character *ch = d->character;
   unsigned race = GET_RACE(ch);
   unsigned clss = GET_CLASS(ch);
   std::string buffer = fmt::format("\r\nRace: {}\r\n", races[race].singular_name);
   buffer += fmt::format("Class: {}\r\n", classes[clss].name);
   buffer += fmt::format("Points left to assign: {}\r\n", d->stats->points);
   buffer += fmt::format("## Attribute      Current  Racial Offsets  Total\r\n");
   if (d->stats->selection == attribute_t::STRENGTH)
   {
      buffer += fmt::format("1. $B*Strength*$R     {:2}      {:2}               {:2}\r\n",
                            d->stats->str[0], races[race].mod_str, d->stats->str[0] + races[race].mod_str);
   }
   else
   {
      buffer += fmt::format("1. Strength       {:2}      {:2}               {:2}\r\n",
                            d->stats->str[0], races[race].mod_str, d->stats->str[0] + races[race].mod_str);
   }

   if (d->stats->selection == attribute_t::DEXTERITY)
   {
      buffer += fmt::format("2. $B*Dexterity*$R    {:2}      {:2}               {:2}\r\n",
                            d->stats->dex[0], races[race].mod_dex, d->stats->dex[0] + races[race].mod_dex);
   }
   else
   {
      buffer += fmt::format("2. Dexterity      {:2}      {:2}               {:2}\r\n",
                            d->stats->dex[0], races[race].mod_dex, d->stats->dex[0] + races[race].mod_dex);
   }

   if (d->stats->selection == attribute_t::INTELLIGENCE)
   {
      buffer += fmt::format("3. $B*Intelligence*$R {:2}      {:2}               {:2}\r\n",
                            d->stats->tel[0], races[race].mod_int, d->stats->tel[0] + races[race].mod_int);
   }
   else
   {
      buffer += fmt::format("3. Intelligence   {:2}      {:2}               {:2}\r\n",
                            d->stats->tel[0], races[race].mod_int, d->stats->tel[0] + races[race].mod_int);
   }

   if (d->stats->selection == attribute_t::WISDOM)
   {
      buffer += fmt::format("4. $B*Wisdom*$R       {:2}      {:2}               {:2}\r\n",
                            d->stats->wis[0], races[race].mod_wis, d->stats->wis[0] + races[race].mod_wis);
   }
   else
   {
      buffer += fmt::format("4. Wisdom         {:2}      {:2}               {:2}\r\n",
                            d->stats->wis[0], races[race].mod_wis, d->stats->wis[0] + races[race].mod_wis);
   }

   if (d->stats->selection == attribute_t::CONSTITUTION)
   {
      buffer += fmt::format("5. $B*Constitution*$R {:2}      {:2}               {:2}\r\n",
                            d->stats->con[0], races[race].mod_con, d->stats->con[0] + races[race].mod_con);
   }
   else
   {
      buffer += fmt::format("5. Constitution   {:2}      {:2}               {:2}\r\n",
                            d->stats->con[0], races[race].mod_con, d->stats->con[0] + races[race].mod_con);
   }

   if (d->stats->selection == attribute_t::UNDEFINED)
   {
      buffer += "Type 1-5 or help strength,wisdom,etc: ";
   }
   else if (d->stats->points > 0)
   {
      buffer += "Type max, +, -, 1-5, confirm or help strength,wisdom,etc: ";
   }
   else
   {
      buffer += "Type -, 1-5, confirm or help strength,wisdom,etc: ";
   }
   SEND_TO_Q(buffer.c_str(), d);
   telnet_ga(d);
}

bool handle_get_stats(Connection *d, std::string arg)
{
   if (arg != "+" && arg != "-" && arg != "confirm" && arg != "max")
   {
      unsigned long input{};
      try
      {
         input = stoul(arg);
      }
      catch (...)
      {
         input = 0;
      }

      if (input >= 0 || input <= 5)
      {
         d->stats->selection = static_cast<decltype(d->stats->selection)>(input);
      }
      else
      {
         d->stats->selection = attribute_t::UNDEFINED;
         SEND_TO_Q("Invalid number specified.\r\n", d);
      }

      return false;
   }

   if (d->stats->selection != attribute_t::UNDEFINED)
   {
      if (arg == "+")
      {
         d->stats->increase(1);
      }
      else if (arg == "max")
      {
         d->stats->increase(d->stats->points);
      }
      else if (arg == "-")
      {
         d->stats->decrease(1);
         d->stats->setMin();
      }
      else if (arg.find("help") == 0)
      {
         arg.erase(0, 5);
         do_help(d->character, arg.data());
         return false;
      }
      else if (arg == "confirm")
      {
         if (d->stats->points > 0)
         {
            SEND_TO_Q("You must assign all your points first.\r\n", d);
            return false;
         }

         Character *ch = d->character;
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

         ch->set_hw();

         return true;
      }
   }

   return false;
}

bool apply_race_attributes(Character *ch, int race)
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

bool check_race_attributes(Character *ch, int race)
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
    : min_str(0), min_dex(0), min_con(0), min_int(0), min_wis(0), points(0), selection(attribute_t::UNDEFINED), race(0), clss(0)
{
   memset(str, 0, sizeof(str));
   memset(dex, 0, sizeof(dex));
   memset(con, 0, sizeof(con));
   memset(tel, 0, sizeof(tel));
   memset(wis, 0, sizeof(wis));
}

bool stat_data::increase(uint64_t number)
{
   if (points <= 0)
   {
      return false;
   }

   int *attribute_to_change = nullptr;
   switch (selection)
   {
   case attribute_t::STRENGTH:
      if (str[0] < 18)
      {
         attribute_to_change = &str[0];
      }
      break;

   case attribute_t::DEXTERITY:
      if (dex[0] < 18)
      {
         attribute_to_change = &dex[0];
      }
      break;
   case attribute_t::CONSTITUTION:
      if (con[0] < 18)
      {
         attribute_to_change = &con[0];
      }
      break;
   case attribute_t::INTELLIGENCE:
      if (tel[0] < 18)
      {
         attribute_to_change = &tel[0];
      }
      break;
   case attribute_t::WISDOM:
      if (wis[0] < 18)
      {
         attribute_to_change = &wis[0];
      }
      break;
      // selection wasn't a recognized attribute
   default:
      return false;
      break;
   }
   // attribute selected was already at max (18+)
   if (attribute_to_change == nullptr)
   {
      return false;
   }

   // Can't increase by more than you have
   if (number > points)
   {
      number = points;
   }

   uint64_t diff_from_natural_max = 18 - *attribute_to_change;
   if (number > diff_from_natural_max)
   {
      number = diff_from_natural_max;
   }

   points -= number;
   (*attribute_to_change) += number;
   return true;
}

bool stat_data::decrease(uint64_t number)
{
   int *attribute_to_change = nullptr;
   switch (selection)
   {
   case attribute_t::STRENGTH:
      if (str[0] > 12)
      {
         attribute_to_change = &str[0];
      }
      break;
   case attribute_t::DEXTERITY:
      if (dex[0] > 12)
      {
         attribute_to_change = &dex[0];
      }
      break;
   case attribute_t::CONSTITUTION:
      if (con[0] > 12)
      {
         attribute_to_change = &con[0];
      }
      break;
   case attribute_t::INTELLIGENCE:
      if (tel[0] > 12)
      {
         attribute_to_change = &tel[0];
      }
      break;
   case attribute_t::WISDOM:
      if (wis[0] > 12)
      {
         attribute_to_change = &wis[0];
      }
      break;
      // selection wasn't a recognized attribute
   default:
      return false;
      break;
   }
   // attribute selected was already at max (18+)
   if (attribute_to_change == nullptr)
   {
      return false;
   }

   if (number > *attribute_to_change)
   {
      number = *attribute_to_change;
   }

   points += number;
   (*attribute_to_change) -= number;
   return true;
}
