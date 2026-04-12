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
#include <cctype>
#include <unistd.h>
#include <cstring>
#include <QString>
#include <fmt/format.h>

#include "DC/DC.h"
#include "DC/comm.h"
#include "DC/connect.h"
#include "DC/guild.h"
#include "DC/race.h"
#include "DC/player.h"
#include "DC/structs.h" // true

#include "DC/clan.h"
#include "DC/db.h" // init_char..

#include "DC/interp.h"

#include "DC/act.h"
#include "DC/clan.h"
#include "DC/spells.h"
#include "DC/fight.h"
#include "DC/handler.h"
#include "DC/DC.h"
#include "DC/const.h"
#include "DC/meta.h"
#include "DC/levels.h"
#include "DC/utility.h"
#define d ->connected((d)->connected)

bool is_bracing(CharacterPtr bracee, room_direction_data *exit);
void show_question_race(Connection *d);

bool wizlock = false;

extern QString greetings1;
extern QString greetings2;
extern QString greetings3;
extern QString greetings4;
extern QString webpage;
extern QString motd;
extern QString imotd;

extern ObjectPtr object_list;

qint32 _parse_email(QString arg);
bool check_deny(class Connection *d, QString name);
void isr_set(CharacterPtr ch);
bool check_reconnect(class Connection *d, QString name, bool fReconnect);
bool check_playing(class Connection *d, QString name);
QString str_str(QString first, QString second);
bool apply_race_attributes(CharacterPtr ch, qint32 race = 0);
bool check_race_attributes(CharacterPtr ch, qint32 race = 0);
bool handle_get_race(Connection *d, QString arg);
void show_question_race(Connection *d);
void show_question_class(Connection *d);
bool handle_get_class(Connection *d, QString arg);
bool is_clss_race_compat(CharacterPtr ch, qint32 clss);
void show_question_stats(Connection *d);
bool handle_get_stats(Connection *d, QString arg);

bool is_race_eligible(CharacterPtr ch, qint32 race)
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

bool is_clss_race_compat(const CharacterPtr ch, qint32 clss, qint32 race)
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

bool is_clss_eligible(CharacterPtr ch, qint32 clss)
{
  qint32 x = {};

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
    if (GET_RAW_DEX(ch) >= 15 && ch->race != RACE_GIANT)
      x = 1;
    break;
  case CLASS_WARRIOR:
    if (GET_RAW_STR(ch) >= 15)
      x = 1;
    break;
  case CLASS_ANTI_PAL:
    if (GET_RAW_INT(ch) >= 14 && GET_RAW_DEX(ch) >= 14 &&
        (ch->race == RACE_HUMAN || ch->race == RACE_ORC || ch->race == RACE_DWARVEN))
      x = 1;
    break;
  case CLASS_PALADIN:
    if (GET_RAW_WIS(ch) >= 14 && GET_RAW_STR(ch) >= 14 &&
        (ch->race == RACE_HUMAN || ch->race == RACE_ELVEN || ch->race == RACE_DWARVEN))
      x = 1;
    break;
  case CLASS_BARBARIAN:
    if (GET_RAW_STR(ch) >= 14 && GET_RAW_CON(ch) >= 14 && ch->race != RACE_PIXIE)
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
  switch (this->race)
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

ObjectPtr Character::clan_altar(void)
{
  if (clan)
    for (auto c = DC::getInstance()->clan_list; c; c = c->next)
      if (c->number == clan)
      {
        for (auto room = c->rooms; room; room = room->next)
        {
          if (real_room(room->room_number) == DC::NOWHERE)
            continue;
          ObjectPtr t = DC::getInstance()->world[real_room(room->room_number)].contents;
          for (; t; t = t->next_content)
          {
            if (t->obj_flags.type_flag == ITEM_ALTAR)
              return t;
          }
        }
      }
  return {};
}

void update_max_who(void)
{
  quint64 players = {};
  for (auto d = DC::getInstance()->connections_; d != nullptr; d = conn->next)
  {
    if (conn->isPlaying() || conn->isEditing())
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
  this->player->bad_pw_tries = {};
  redo_hitpoints(this);
  redo_mana(this);
  redo_ki(this);
  do_inate_race_abilities();
  check_hw();
  /* Add a character's skill item's to the list. */
  this->player->skillchange = {};
  this->spellcraftglyph = {};
  for (qint32 i = {}; i < MAX_WEAR; i++)
  {
    if (!this->equipment[i])
      continue;
    for (qint32 a = {}; a < this->equipment[i]->num_affects; a++)
    {
      if (this->equipment[i]->affected[a].location >= 1000)
      {
        this->equipment[i]->next_skill = this->player->skillchange;
        this->player->skillchange = this->equipment[i];
        this->equipment[i]->next_skill = {};
      }
    }
  }
  // add character base saves to saving throws
  for (qint32 i = {}; i <= SAVE_TYPE_MAX; i++)
  {
    this->saves[i] += this->getLevel() / 4;
    this->saves[i] += this->player->saves_mods[i];
  }

  if (this->title == nullptr)
  {
    this->title = QStringLiteral("is a virgin.");
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

  if (!this->isNonPlayer() && this->getLevel() >= IMMORTAL)
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

  this->curLeadBonus = {};
  this->changeLeadBonus = false;
  this->cRooms = {};
  REMBIT(this->affected_by, AFF_BLACKJACK_ALERT);
  for (qint32 i = {}; i < QUEST_MAX; i++)
  {
    this->player->quest_current[i] = -1;
    this->player->quest_current_ticksleft[i] = {};
  }
  auto &vault = DC::getInstance()->vaults_.has_vault(name());
  if (player->time.logon < 1172204700)
  {
    if (vault)
    {
      qint32 adder = this->getLevel() - 50;
      if (adder < 0)
        adder = {}; // Heh :P
      vault.size_ += adder * 10;
      if (vault.size_ < 100)
        vault.size_ = 100;
      DC::getInstance()->vaults_.save(vault.owner_);
    }
  }

  if (vault)
  {
    if (vault.size_ < getLevel() * 10)
    {
      DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "%s's vault reset from %d to %d during login.", qPrintable(this->name()), vault.size_, this->getLevel() * 10);
      vault.size_ = this->getLevel() * 10;
    }

    DC::getInstance()->vaults_.save(vault.owner_);
  }

  if (this->player->time.logon < 1151506181)
  {
    this->player->quest_points = {};
    for (qint32 i = {}; i < QUEST_MAX_CANCEL; i++)
      this->player->quest_cancel[i] = {};
    for (qint32 i = {}; i < QUEST_TOTAL / ASIZE; i++)
      this->player->quest_complete[i] = {};
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

  CharacterClassSkill *c_skills = this->get_skill_list();

  if (IS_MORTAL(this))
  {
    QQueue<skill_t> skills_to_delete = {};
    for (const auto &curr : this->skills)
    {
      if (curr.first < 600 && search_skills2(curr.first, c_skills) == -1 && search_skills2(curr.first, g_skills) == -1 && curr.first != META_REIMB && curr.first != NEW_SAVE)
      {
        DC::getInstance()->logentry(QStringLiteral("Removing skill %1 from %2").arg(curr.first).arg(qPrintable(this->name())), IMMORTAL, DC::LogChannel::LOG_PLAYER);
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
    qint32 new_ = MIN(r_new_meta_platinum_cost(0, hps_plats_spent()), r_new_meta_exp_cost(0, hps_exp_spent()));
    qint32 ometa = GET_HP_METAS(this);
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
  QQueue<QString> todelete;
  vault = DC::getInstance()->vaults_.has_vault(qPrintable(this->name()));
  if (vault)
  {
    for (const auto &access : vault.access)
    {
      if (!access.name.isEmpty())
      {
        if (!file_exists(QStringLiteral("%1/%2/%3").arg(SAVE_DIR).arg(access.name[0].toUpper()).arg(access.name)))
        {
          todelete.push(access.name);
        }
      }
    }
  }

  while (!todelete.empty())
  {
    DC::getInstance()->logentry(QStringLiteral("Deleting %1 from %2's vault access list.\n").arg(todelete.front()).arg(qPrintable(this->name())), 0, DC::LogChannel::LOG_MORTAL);
    remove_vault_access(todelete.front(), vault);
    todelete.pop();
  }

  if (this->getSetting("mode").startsWith("character"))
  {
    telnet_echo_off(this->desc);
    telnet_sga(this->desc);
  }
}

void Character::roll_and_display_stats(void)
{
  for (auto x = 0; x <= 4; x++)
  {
    auto a = dice(3, 6);
    auto b = dice(6, 3);
    desc->stats->str[x] = MAX(12 + number(0, 1), MAX(a, b));
    a = dice(3, 6);
    b = dice(6, 3);
    desc->stats->dex[x] = MAX(12 + number(0, 1), MAX(a, b));
    a = dice(3, 6);
    b = dice(6, 3);
    desc->stats->con[x] = MAX(12 + number(0, 1), MAX(a, b));
    a = dice(3, 6);
    b = dice(6, 3);
    desc->stats->tel[x] = MAX(12 + number(0, 1), MAX(a, b));
    a = dice(3, 6);
    b = dice(6, 3);
    desc->stats->wis[x] = MAX(12 + number(0, 1), MAX(a, b));
  }

  /*
  For testing purposes
  this->desc->stats->str[0] = 13;
  this->desc->stats->dex[0] = 14;
  this->desc->stats->con[0] = 13;
  this->desc->stats->tel[0] = 12;
  this->desc->stats->wis[0] = 14;
  */

  write_to_output("\r\n  Choose from any of the following groups of abilities...     \r\n", desc);
  write_to_output("Group: 1     2     3     4     5\r\n", this->desc);
  write_to_output(QStringLiteral("Str:   %1    %2    %3    %4    %5\r\n").arg(desc->stats->str[0], -2).arg(desc->stats->str[1], -2).arg(desc->stats->str[2], -2).arg(desc->stats->str[3], -2).arg(desc->stats->str[4], -2), desc);
  write_to_output(QStringLiteral("Dex:   %1    %2    %3    %4    %5\r\n").arg(desc->stats->dex[0], -2).arg(desc->stats->dex[1], -2).arg(desc->stats->dex[2], -2).arg(desc->stats->dex[3], -2).arg(desc->stats->dex[4], -2), desc);
  write_to_output(QStringLiteral("Con:   %1    %2    %3    %4    %5\r\n").arg(desc->stats->con[0], -2).arg(desc->stats->con[1], -2).arg(desc->stats->con[2], -2).arg(desc->stats->con[3], -2).arg(desc->stats->con[4], -2), desc);
  write_to_output(QStringLiteral("Int:   %1    %2    %3    %4    %5\r\n").arg(desc->stats->tel[0], -2).arg(desc->stats->tel[1], -2).arg(desc->stats->tel[2], -2).arg(desc->stats->tel[3], -2).arg(desc->stats->tel[4], -2), desc);
  write_to_output(QStringLiteral("Wis:   %1    %2    %3    %4    %5\r\n").arg(desc->stats->wis[0], -2).arg(desc->stats->str[1], -2).arg(desc->stats->wis[2], -2).arg(desc->stats->wis[3], -2).arg(desc->stats->wis[4], -2), desc);
  write_to_output("Choose a group <1-5>, or press return to reroll(Help <attribute> for more information) --> ", desc);
  telnet_ga(desc);

  WAIT_STATE(this, DC::PULSE_TIMER / 10);
}

qint32 DC::exceeded_connection_limit(class Connection *new_conn)
{
  if (new_conn->getPeerOriginalAddress().isNull() || new_conn->getPeerAddress().isLoopback())
  {
    return false;
  }

  quint64 count = {};
  QSet<Connection *> to_close_list;
  for (auto d = connections_; d; d = conn->next)
  {
    if (new_conn->getPeerOriginalAddress() == conn->getPeerOriginalAddress())
    {
      count++;
      to_close_list.insert(d);
    }
  }

  if (count > getConnectionLimit())
  {
    write_to_output(QStringLiteral("Sorry, there are more than %1 connections from IP %2\r\n"
                                   "already logged into Dark Castle.  If you have a valid reason\r\n"
                                   "for having this many connections from one IP please let an imm\r\n"
                                   "know and they will speak with you. Assuming this is an error and closing all connections.\r\n")
                        .arg(getConnectionLimit())
                        .arg(new_conn->getPeerOriginalAddress().toString()),
                    new_conn);

    for (const auto &d : to_close_list)
    {
      logsocket(QStringLiteral("Closing socket %1 from IP %2 due to > %3 connections.").arg(conn->desc_num).arg(conn->getPeerOriginalAddress().toString()).arg(getConnectionLimit()));
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
    DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_BUG, "check_hw: %s's height %d > max %d. height set to max.", qPrintable(this->name()), GET_HEIGHT(this), races[this->race].max_height);
    this->height = races[this->race].max_height;
  }
  if (this->height < races[this->race].min_height)
  {
    DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_BUG, "check_hw: %s's height %d < min %d. height set to min.", qPrintable(this->name()), GET_HEIGHT(this), races[this->race].min_height);
    this->height = races[this->race].min_height;
  }

  if (this->weight > races[this->race].max_weight)
  {
    DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_BUG, "check_hw: %s's weight %d > max %d. weight set to max.", qPrintable(this->name()), GET_WEIGHT(this), races[this->race].max_weight);
    this->weight = races[this->race].max_weight;
  }
  if (this->weight < races[this->race].min_weight)
  {
    DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_BUG, "check_hw: %s's weight %d < min %d. weight set to min.", qPrintable(this->name()), GET_WEIGHT(this), races[this->race].min_weight);
    this->weight = races[this->race].min_weight;
  }
  heightweight(true);
}

void Character::set_hw(void)
{
  this->height = number(races[this->race].min_height, races[this->race].max_height);
  // DC::getInstance()->logf(ANGEL, DC::LogChannel::LOG_MORTAL, "%s's height set to %d", qPrintable(this->name()), GET_HEIGHT(this));
  this->weight = number(races[this->race].min_weight, races[this->race].max_weight);
  // DC::getInstance()->logf(ANGEL, DC::LogChannel::LOG_MORTAL, "%s's weight set to %d", qPrintable(this->name()), GET_WEIGHT(this));
}

// Deal with sockets that haven't logged in yet.
void DC::nanny(class Connection *d, QString arg)
{
  auto badclssmsg = u"You must choose a class that matches your stats. These are marked by a '*'.\r\nSelect a class-> "_s;
  auto ch = conn->character;
  arg = arg.trimmed();

  if (!str_prefix("help", arg))
    switch (conn->connected)
    {
    case Connection::states::OLD_GET_CLASS:
    case Connection::states::OLD_GET_RACE:
    case Connection::states::OLD_CHOOSE_STATS:
    case Connection::states::GET_STATS:
      arg.erase(0, 4);
      do_new_help(conn->character, arg.data(), cmd_t::PAGING_HELP);
      return;
      break;
    }

  load_status_t ls = {};
  switch (conn->connected)
  {

  default:
    DC::getInstance()->logentry(QStringLiteral("Nanny: invalid conn->connected == %1").arg(conn->connected), 0, DC::LogChannel::LOG_BUG);
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
      write_to_output(webpage, d);
      close_socket(d);
      return;
    }

    // if it's not a webbrowser, display the entrance greeting
    switch (number(1, 4))
    {
    default:
    case 1:
      write_to_output(greetings1, d);
      break;

    case 2:
      write_to_output(greetings2, d);
      break;

    case 3:
      write_to_output(greetings3, d);
      break;

    case 4:
      write_to_output(greetings4, d);
      break;
    }

    if (exceeded_connection_limit(d))
      break;

    if (wizlock)
    {
      write_to_output("The game is currently WIZLOCKED. Only immortals can connect at this time.\r\n", d);
    }
    write_to_output("What name for the roster? ", d);
    telnet_ga(d);
    conn->connected = Connection::states::GET_NAME;

    // if they have already entered their name, drop through.  Otherwise stop and wait for input
    if (arg.empty())
    {
      break;
    }
    /* no break */

  case Connection::states::GET_PROXY:
    conn->connected = Connection::states::GET_NAME;

    // If first line of text is a proxy header then construct Proxy
    // otherwise assume it's a name.
    if (QString(arg.c_str()).indexOf("PROXY ") == 0)
    {
      conn->proxy = Proxy(arg.c_str());
      return;
    }
  case Connection::states::GET_NAME:

    if (arg.isEmpty())
    {
      write_to_output("Empty name.  Disconnecting...", d);
      close_socket(d);
      return;
    }

    // Capitlize first letter, lowercase rest
    arg[0] = UPPER(arg[0]);
    for (y = 1; arg[y] != '\0'; y++)
      arg[y] = LOWER(arg[y]);

    if (_parse_name(arg.c_str(), tmp_name))
    {
      write_to_output("Illegal name, try another.\r\nName: ", d);
      telnet_ga(d);
      return;
    }

    if (check_deny(d, tmp_name))
      return;

    // Uncomment this if you think a playerfile may be crashing the mud. -pir
    //      dc_sprintf(str_tmp, "Trying to login: %s", tmp_name);
    //    DC::getInstance()->logentry(str_tmp, 0, DC::LogChannel::LOG_MISC);

    // ch is allocated in load_char_obj
    ls = load_char_obj(d, tmp_name);
    if (ls == load_status_t::missing)
    {
      str_tmp << "../archive/" << tmp_name << ".gz";
      if (file_exists(str_tmp.str().c_str()))
      {
        write_to_output("That character is archived.\r\nPlease mail "
                        "imps@dcastle.org to request it be restored.\r\n",
                        d);
        close_socket(d);
        return;
      }
    }
    ch = conn->character;

    // This is needed for "check_reconnect"  we free it later during load_char_obj
    // TODO - this is memoryleaking qPrintable(ch->name()).  Check if qPrintable(ch->name()) is not there before
    // doing it to fix it.  (No time to verify this now, so i'll do it later)
    ch->name((tmp_name));

    // if (isAllowedHost(conn->getPeerOriginalAddress().toString(qPrintable())))
    // write_to_output("You are logging in from an ALLOWED host.\r\n", d);

    if (!check_reconnect(d, tmp_name, false) && wizlock && !DC::getInstance()->isAllowedHost(conn->getPeerOriginalAddress()))
    {
      write_to_output("The game is wizlocked.\r\n", d);
      close_socket(d);
      return;
    }

    if (ls == load_status_t::success)
    {
      /* Old player */
      write_to_output("Password: ", d);
      telnet_ga(d);
      conn->connected = Connection::states::GET_OLD_PASSWORD;
      return;
    }
    else if (ls == load_status_t::missing)
    {
      if (DC::getInstance()->cf.bport)
      {
        write_to_output("New chars not allowed on this port.\r\nEnter a new name: ", d);
        return;
      }
      /* New player */
      dc_sprintf(buf, "Did I get that right, %s (y/n)? ", tmp_name);
      write_to_output(buf, d);
      telnet_ga(d);
      conn->connected = Connection::states::CONFIRM_NEW_NAME;
      return;
    }
    else
    {
      write_to_output(QStringLiteral("There was an error loading %1").arg(tmp_name), d);
      close_socket(d);
      return;
    }
    break;

  case Connection::states::GET_OLD_PASSWORD:
    write_to_output("\r\n", d);

    // Default is to authenticate against character password
    password = ch->player->pwd;

    // If -P option passed and one of your other characters is an imp, allow this character with that imp's password
    if (DC::getInstance()->cf.allow_imp_password && DC::getInstance()->isAllowedHost(conn->getPeerOriginalAddress()))
    {
      for (Connection *ad = DC::getInstance()->connections_; ad && ad != (Connection *)0x95959595; ad = ad->next)
      {
        if (ad != d && conn->getPeerOriginalAddress() == ad->getPeerOriginalAddress())
        {
          if (ad->character && ad->character->getLevel() == IMPLEMENTER && ad->character->isPlayer())
          {
            password = ad->character->player->pwd;
            DC::getInstance()->logf(OVERSEER, DC::LogChannel::LOG_SOCKET, "Using %s's password for authentication.", qPrintable(ad->character->name()));
            break;
          }
        }
      }
    }

    if (QString(crypt(arg.c_str(), password)) != password)
    {
      write_to_output("Wrong password.\r\n", d);
      dc_sprintf(log_buf, "%s wrong password: %s", qPrintable(ch->name()), qPrintable(conn->getPeerOriginalAddress().toString()));
      DC::getInstance()->logentry(log_buf, OVERSEER, DC::LogChannel::LOG_SOCKET);
      if ((ch = get_pc(qPrintable(conn->character->name()))))
      {
        dc_sprintf(log_buf, "$4$BWARNING: Someone just tried to log in as you with the wrong password.\r\n"
                            //           "Attempt was from %s.$R\r\n"
                            "(If it's only once or twice, you can ignore it.  If it's several dozen tries, let a god know.)\r\n");
        ch->send(log_buf);
      }
      else
      {
        if (conn->character->player->bad_pw_tries > 100)
        {
          dc_sprintf(log_buf, "%s has 100+ bad pw tries...", qPrintable(conn->character->name()));
          DC::getInstance()->logentry(log_buf, SERAPH, DC::LogChannel::LOG_SOCKET);
        }
        else
        {
          conn->character->player->bad_pw_tries++;
          conn->character->save_char_obj();
        }
      }
      close_socket(d);
      return;
    }

    check_playing(d, ch->name());

    if (check_reconnect(d, ch->name(), true))
      return;

    buffer = QStringLiteral("%1@%2 has connected.").arg(qPrintable(ch->name())).arg(conn->getPeerOriginalAddress().toString());
    if (ch->getLevel() < ANGEL)
      DC::getInstance()->logentry(buffer, OVERSEER, DC::LogChannel::LOG_SOCKET);
    else
      DC::getInstance()->logentry(buffer, ch->getLevel(), DC::LogChannel::LOG_SOCKET);

    warn_if_duplicate_ip(ch);
    //    write_to_output(motd, d);
    if (ch->isMortalPlayer())
      conn->character->send(motd);
    else
      conn->character->send(imotd);

    Clan *clan;
    if ((clan = get_clan(ch->clan)) && clan->clanmotd)
    {
      ch->sendln("$B----------------------------------------------------------------------$R");
      ch->send(clan->clanmotd);
      ch->sendln("$B----------------------------------------------------------------------$R");
    }

    write_to_output(QStringLiteral("\r\nIf you have read this motd, press Return.\r\nLast connected from:\r\n%1\r\n").arg(ch->player->last_site), d);
    telnet_ga(d);

    if (conn->character->player->bad_pw_tries)
    {
      dc_sprintf(buf, "\r\n\r\n$4$BYou have had %d wrong passwords entered since your last complete login.$R\r\n\r\n", conn->character->player->bad_pw_tries);
      write_to_output(buf, d);
    }
    DC::getInstance()->TheAuctionHouse.CheckForSoldItems(conn->character);
    conn->connected = Connection::states::READ_MOTD;
    break;

  case Connection::states::CONFIRM_NEW_NAME:
    if (arg.empty())
    {
      write_to_output("Please type y or n: ", d);
      telnet_ga(d);
      break;
    }

    switch (arg[0])
    {
    case 'y':
    case 'Y':

      if (DC::getInstance()->bans_.is_banned(conn->getPeerOriginalAddress()) >= Ban::NEW)
      {
        dc_sprintf(buf, "Request for new character %s denied from [%s] (siteban)", qPrintable(conn->character->name()), qPrintable(conn->getPeerOriginalAddress().toString()));
        DC::getInstance()->logentry(buf, OVERSEER, DC::LogChannel::LOG_SOCKET);
        write_to_output("Sorry, new chars are not allowed from your site.\r\n"
                        "Questions may be directed to imps@dcastle.org\r\n",
                        d);
        conn->connected = Connection::states::CLOSE;
        return;
      }
      dc_sprintf(buf, "New character.\r\nGive me a password for %s: ", qPrintable(ch->name()));
      write_to_output(buf, d);
      telnet_ga(d);
      conn->connected = Connection::states::GET_NEW_PASSWORD;
      // at this point, player hasn't yet been created.  So we're going to go ahead and
      // allocate it since a new character is obviously a PC
      ch->player = new Player;
      ch->setType(Character::Type::Player);
      assert(ch->player);
      break;

    case 'n':
    case 'N':
      write_to_output("Ok, what IS it, then? ", d);
      telnet_ga(d);
      // TODO - double check this to make sure we're free'ing properly
      ch->name("");
      conn->character = {};
      conn->character = {};
      conn->connected = Connection::states::GET_NAME;
      break;

    default:
      write_to_output("Please type y or n: ", d);
      telnet_ga(d);
      break;
    }
    break;

  case Connection::states::GET_NEW_PASSWORD:
    write_to_output("\r\n", d);

    if (arg.length() < 6)
    {
      write_to_output("Password must be at least six characters long.\r\nPassword: ", d);
      telnet_ga(d);
      return;
    }

    ch->player->pwd = crypt(qPrintable(arg), qPrintable(ch->name()));
    write_to_output("Please retype password: ", d);
    telnet_ga(d);
    conn->connected = Connection::states::CONFIRM_NEW_PASSWORD;
    break;

  case Connection::states::CONFIRM_NEW_PASSWORD:
    write_to_output("\r\n", d);

    if (QString(crypt(arg.c_str(), ch->player->pwd)) != ch->player->pwd)
    {
      write_to_output("Passwords don't match.\r\nRetype password: ", d);
      telnet_ga(d);
      conn->connected = Connection::states::GET_NEW_PASSWORD;
      return;
    }

  case Connection::states::QUESTION_ANSI:
    write_to_output("Do you want ANSI color (y/n)? ", d);
    telnet_ga(d);
    conn->connected = Connection::states::GET_ANSI;
    break;

  case Connection::states::GET_ANSI:
    if (arg.empty())
    {
      conn->connected = Connection::states::QUESTION_ANSI;
      return;
    }

    switch (arg[0])
    {
    case 'y':
    case 'Y':
      conn->color = true;
      break;
    case 'n':
    case 'N':
      conn->color = false;
      break;
    default:
      conn->connected = Connection::states::QUESTION_ANSI;
      return;
    }

    conn->connected = Connection::states::QUESTION_SEX;
    break;

  case Connection::states::QUESTION_SEX:
    write_to_output("What is your sex (m/f)? ", d);
    telnet_ga(d);
    conn->connected = Connection::states::GET_NEW_SEX;
    break;

  case Connection::states::GET_NEW_SEX:
    if (arg.empty())
    {
      write_to_output("That's not a sex.\r\n", d);
      conn->connected = Connection::states::QUESTION_SEX;
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
      write_to_output("That's not a sex.\r\n", d);
      conn->connected = Connection::states::QUESTION_SEX;
      return;
    }

    /*
          if (!isAllowedHost(conn->getPeerOriginalAddress().toString(qPrintable())) && DC::getInstance()->cf.allow_newstatsys == false)
          {
             conn->connected = Connection::states::OLD_STAT_METHOD;
             break;
          }
    */

    write_to_output("\r\n", d);
    write_to_output("$R$7Note: If you see a word that is entirely CAPITALIZED and $Bbold$R then there\r\n", d);
    write_to_output("exists a helpfile for that word which you can lookup with the command\r\n", d);
    write_to_output("help keyword.\r\n", d);
    write_to_output("\r\n", d);
    write_to_output("Dark Castle supports two ways of picking your race, class and initial\r\n", d);
    write_to_output("$BATTRIBUTES$R such as $BSTRENGTH$R, $BDEXTERITY$R, $BCONSTITUION$R, $BINTELLIGENCE$R\r\n", d);
    write_to_output("or $BWISDOM$R.\r\n\r\n", d);
    write_to_output("The newest method is you first pick a race, a class then you get 12 points\r\n", d);
    write_to_output("in each attribute and 23 points you can divide among those five\r\n", d);
    write_to_output("attributes.\r\n\r\n", d);
    write_to_output("The old method is you roll virtual dice. The virtual dice consist of either\r\n", d);
    write_to_output("three 6-sided dice or six 3-sided dice. The game picks the larger group of\r\n", d);
    write_to_output("those two separate sets of dice rolls and throws out any sets with a sum less\r\n", d);
    write_to_output("than 12. You can roll virtual dice as many times as you like. After you finish\r\n", d);
    write_to_output("rolling dice then the game shows you what races you can pick based on the dice\r\n", d);
    write_to_output("rolls. After picking from the races your dice rolls allow then you have to\r\n", d);
    write_to_output("pick a class that fits your choosen stats.\r\n", d);
    conn->connected = Connection::states::QUESTION_STAT_METHOD;
    return;

  case Connection::states::QUESTION_STAT_METHOD:
    write_to_output("\r\n", d);
    write_to_output("1. Pick race, class then assign points to attributes. (new method)\r\n", d);
    write_to_output("2. Roll virtual dice for attributes then pick race and class. (old method)\r\n", d);
    write_to_output("What is your choice (1,2)? ", d);
    telnet_ga(d);
    conn->connected = Connection::states::GET_STAT_METHOD;
    break;

  case Connection::states::GET_STAT_METHOD:
    try
    {
      selection = stoul(arg);
    }
    catch (...)
    {
      selection = {};
    }

    if (selection == 1)
    {
      conn->connected = Connection::states::NEW_STAT_METHOD;
    }
    else if (selection == 2)
    {
      conn->connected = Connection::states::OLD_STAT_METHOD;
    }
    else
    {
      conn->connected = Connection::states::QUESTION_STAT_METHOD;
    }
    break;

  case Connection::states::NEW_STAT_METHOD:
    conn->connected = Connection::states::QUESTION_RACE;
    break;

  case Connection::states::QUESTION_RACE:
    show_question_race(d);

    conn->connected = Connection::states::GET_RACE;
    break;

  case Connection::states::GET_RACE:
    if (handle_get_race(d, arg) == true)
    {
      conn->connected = Connection::states::QUESTION_CLASS;
    }
    else
    {
      conn->connected = Connection::states::QUESTION_RACE;
    }
    break;

  case Connection::states::QUESTION_CLASS:
    show_question_class(d);

    conn->connected = Connection::states::GET_CLASS;
    break;

  case Connection::states::GET_CLASS:
    if (handle_get_class(d, arg) == false)
    {
      if (conn->connected != Connection::states::QUESTION_RACE)
      {
        conn->connected = Connection::states::QUESTION_CLASS;
      }
    }
    else
    {
      conn->connected = Connection::states::QUESTION_STATS;
    }
    break;

  case Connection::states::QUESTION_STATS:
    show_question_stats(d);

    conn->connected = Connection::states::GET_STATS;
    break;

  case Connection::states::GET_STATS:
    if (handle_get_stats(d, arg) == false)
    {
      conn->connected = Connection::states::QUESTION_STATS;
    }
    else
    {
      conn->connected = Connection::states::NEW_PLAYER;
    }
    break;

  case Connection::states::OLD_STAT_METHOD:
    if (ch->desc->stats != nullptr)
    {
      ch->desc->stats = {};
    }
    ch->desc->stats = new stat_data;

    conn->connected = Connection::states::OLD_CHOOSE_STATS;
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
      selection = {};
    }

    if (selection >= 1 && selection <= 5)
    {
      GET_RAW_STR(ch) = ch->desc->stats->str[selection - 1];
      GET_RAW_INT(ch) = ch->desc->stats->tel[selection - 1];
      GET_RAW_WIS(ch) = ch->desc->stats->wis[selection - 1];
      GET_RAW_DEX(ch) = ch->desc->stats->dex[selection - 1];
      GET_RAW_CON(ch) = ch->desc->stats->con[selection - 1];
      ch->desc->stats = {};
      ch->desc->stats = {};
      write_to_output("\r\nChoose a race(races you can select are marked with a *).\r\n", d);
      dc_sprintf(buf, "  %c1: Human\r\n  %c2: Elf\r\n  %c3: Dwarf\r\n"
                      "  %c4: Hobbit\r\n  %c5: Pixie\r\n  %c6: Ogre\r\n"
                      "  %c7: Gnome\r\n  %c8: Orc\r\n  %c9: Troll\r\n"
                      "\r\nSelect a race(Type help <race> for more information)-> ",
                 is_race_eligible(ch, 1) ? '*' : ' ', is_race_eligible(ch, 2) ? '*' : ' ', is_race_eligible(ch, 3) ? '*' : ' ', is_race_eligible(ch, 4) ? '*' : ' ', is_race_eligible(ch, 5) ? '*' : ' ', is_race_eligible(ch, 6) ? '*' : ' ',
                 is_race_eligible(ch, 7) ? '*' : ' ', is_race_eligible(ch, 8) ? '*' : ' ', is_race_eligible(ch, 9) ? '*' : ' ');

      write_to_output(buf, d);
      telnet_ga(d);
      conn->connected = Connection::states::OLD_GET_RACE;
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
      selection = {};
    }

    switch (selection)
    {
    default:
      write_to_output("That's not a race.\r\nWhat IS your race? ", d);
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
      ch->alignment = {};
      GET_RAW_STR(ch) += RACE_TROLL_STR_MOD;
      GET_RAW_INT(ch) += RACE_TROLL_INT_MOD;
      GET_RAW_WIS(ch) += RACE_TROLL_WIS_MOD;
      GET_RAW_DEX(ch) += RACE_TROLL_DEX_MOD;
      GET_RAW_CON(ch) += RACE_TROLL_CON_MOD;
      break;
    }

    ch->set_hw();
    write_to_output("\r\nA '*' denotes a class that fits your chosen stats.\r\n", d);
    dc_sprintf(buf, " %c 1: Warrior\r\n"
                    " %c 2: Cleric\r\n"
                    " %c 3: Mage\r\n"
                    " %c 4: Thief\r\n"
                    " %c 5: Anti-Paladin\r\n"
                    " %c 6: Paladin\r\n"
                    " %c 7: Barbarian \r\n"
                    " %c 8: Monk\r\n"
                    " %c 9: Ranger\r\n"
                    " %c 10: Bard\r\n"
                    " %c 11: Druid\r\n"
                    "\r\nSelect a class(Type help <class> for more information) > ",
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
    write_to_output(buf, d);
    telnet_ga(d);
    conn->connected = Connection::states::OLD_GET_CLASS;
    break;

  case Connection::states::OLD_GET_CLASS:
    try
    {
      selection = stoul(arg);
    }
    catch (...)
    {
      selection = {};
    }

    switch (selection)
    {
    default:
      write_to_output("That's not a class.\r\nWhat IS your class? ", d);
      telnet_ga(d);
      return;

    case 1:
      //          if(!is_clss_eligible(ch, CLASS_WARRIOR))
      //         { write_to_output(badclssmsg, d);  return; }
      GET_CLASS(ch) = CLASS_WARRIOR;
      break;
    case 2:
      if (!is_clss_eligible(ch, CLASS_CLERIC))
      {
        write_to_output(badclssmsg, d);
        return;
      }
      GET_CLASS(ch) = CLASS_CLERIC;
      break;
    case 3:
      if (!is_clss_eligible(ch, CLASS_MAGIC_USER))
      {
        write_to_output(badclssmsg, d);
        return;
      }
      GET_CLASS(ch) = CLASS_MAGIC_USER;
      break;
    case 4:
      if (!is_clss_eligible(ch, CLASS_THIEF))
      {
        write_to_output(badclssmsg, d);
        return;
      }
      write_to_output("Pstealing is legal on this mud :)\r\n"
                      "Check 'help psteal' before you try though.",
                      d);
      GET_CLASS(ch) = CLASS_THIEF;
      break;
    case 5:
      if (!is_clss_eligible(ch, CLASS_ANTI_PAL))
      {
        write_to_output(badclssmsg, d);
        return;
      }
      GET_CLASS(ch) = CLASS_ANTI_PAL;
      GET_ALIGNMENT(ch) = -1000;
      break;
    case 6:
      if (!is_clss_eligible(ch, CLASS_PALADIN))
      {
        write_to_output(badclssmsg, d);
        return;
      }
      GET_CLASS(ch) = CLASS_PALADIN;
      GET_ALIGNMENT(ch) = 1000;
      break;
    case 7:
      if (!is_clss_eligible(ch, CLASS_BARBARIAN))
      {
        write_to_output(badclssmsg, d);
        return;
      }
      GET_CLASS(ch) = CLASS_BARBARIAN;
      break;
    case 8:
      if (!is_clss_eligible(ch, CLASS_MONK))
      {
        write_to_output(badclssmsg, d);
        return;
      }
      GET_CLASS(ch) = CLASS_MONK;
      break;
    case 9:
      if (!is_clss_eligible(ch, CLASS_RANGER))
      {
        write_to_output(badclssmsg, d);
        return;
      }
      GET_CLASS(ch) = CLASS_RANGER;
      break;
    case 10:
      if (!is_clss_eligible(ch, CLASS_BARD))
      {
        write_to_output(badclssmsg, d);
        return;
      }
      GET_CLASS(ch) = CLASS_BARD;
      break;
    case 11:
      if (!is_clss_eligible(ch, CLASS_DRUID))
      {
        write_to_output(badclssmsg, d);
        return;
      }
      GET_CLASS(ch) = CLASS_DRUID;
      break;
    }
    conn->connected = Connection::states::NEW_PLAYER;
    break;

  case Connection::states::NEW_PLAYER:

    init_char(ch);

    dc_sprintf(log_buf, "%s@%s new player.", qPrintable(ch->name()), qPrintable(conn->getPeerOriginalAddress().toString()));
    DC::getInstance()->logentry(log_buf, OVERSEER, DC::LogChannel::LOG_SOCKET);
    write_to_output("\r\n", d);
    write_to_output(motd, d);
    write_to_output("\r\nIf you have read this motd, press Return.", d);
    telnet_ga(d);

    conn->connected = Connection::states::READ_MOTD;
    break;

  case Connection::states::READ_MOTD:
    write_to_output(DC::getInstance()->menu, d);
    telnet_ga(d);
    conn->connected = Connection::states::SELECT_MENU;
    break;

  case Connection::states::SELECT_MENU:
    if (arg.empty())
    {
      write_to_output(DC::getInstance()->menu, d);
      telnet_ga(d);
      break;
    }

    switch (arg[0])
    {
    case '0':
      close_socket(d);
      d = {};
      break;

    case '1':
      // I believe this is here to stop a dupe bug
      // by logging in twice, and leaving one at the password: prompt
      if (ch->getLevel() > 0)
      {
        dc_strcpy(tmp_name, qPrintable(ch->name()));
        free_char(conn->character, Trace("nanny Connection::states::SELECT_MENU 1"));
        conn->character = {};
        load_char_obj(d, tmp_name);
        ch = conn->character;

        if (!DC::getInstance()->cf.implementer.isEmpty())
        {
          if (QString(qPrintable(ch->name())).compare(DC::getInstance()->cf.implementer, Qt::CaseInsensitive) == 0)
          {
            ch->setLevel(110);
          }
        }

        if (!ch)
        {
          write_to_descriptor(conn->descriptor, "It seems your character has been deleted during logon, or you just experienced some obscure bug.");
          close_socket(d);
          d = {};
          break;
        }
      }
      unique_scan(ch);
      if (ch->getGold() > 1000000000)
      {
        dc_sprintf(log_buf, "%s has more than a billion gold. Bugged?", qPrintable(ch->name()));
        DC::getInstance()->logentry(log_buf, 100, DC::LogChannel::LOG_WARNING);
      }
      if (GET_BANK(ch) > 1000000000)
      {
        dc_sprintf(log_buf, "%s has more than a billion gold in the bank. Rich fucker or bugged.", qPrintable(ch->name()));
        DC::getInstance()->logentry(log_buf, 100, DC::LogChannel::LOG_WARNING);
      }
      ch->sendln("\r\nWelcome to Dark Castle.");
      character_list.insert(ch);

      if (IS_AFFECTED(ch, AFF_ITEM_REMOVE))
      {
        REMBIT(ch->affected_by, AFF_ITEM_REMOVE);
        ch->sendln("\r\n$I$B$4***WARNING*** Items you were previously wearing have been moved to your inventory, please check before moving out of a safe room.$R");
      }

      ch->do_on_login_stuff();

      if (ch->getLevel() < OVERSEER)
        clan_login(ch);

      act_to_room("$n has entered the game.", ch, 0, 0, INVIS_NULL);
      if (!GET_SHORT_ONLY(ch))
        ch->short_description(ch->name());
      DC::getInstance()->update_wizlist(ch);
      ch->check_maxes(); // Check skill maxes.

      conn->connected = Connection::states::PLAYING;
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
          send_to_char("\r\nThere is an active vote in which you have not yet voted.\r\n"
                       "Enter \"vote\" to see details\r\n\r\n",
                       ch);
        }
      }
      extern void zap_eq_check(CharacterPtr ch);
      zap_eq_check(ch);
      break;

    case '2':
      write_to_output("Enter a text you'd like others to see when they look at you.\r\n"
                      "Terminate with '/s' on a new line.\r\n",
                      d);
      if (!ch->description().isEmpty())
      {
        write_to_output("Old description:\r\n", d);
        write_to_output(ch->description(), d);
      }
      ch->description({});

      conn->qstrnew = ch->description();
      conn->max_str = 539;
      conn->connected = Connection::states::EXDSCR;
      break;

    case '3':
      write_to_output("Enter current password: ", d);
      telnet_ga(d);
      conn->connected = Connection::states::CONFIRM_PASSWORD_CHANGE;
      break;

    case '4':
      // delete this character
      if (conn->character->getLevel() >= 20)
      {
        write_to_output("\r\nOnly characters under level 20 can be self-deleted.\r\n", d);
        conn->connected = Connection::states::SELECT_MENU;
        write_to_output(DC::getInstance()->menu, d);
        telnet_ga(d);
      }
      else
      {
        write_to_output("This will _permanently_ erase you.\r\nType ERASE ME if this is really what you want: ", d);
        telnet_ga(d);
        conn->connected = Connection::states::DELETE_CHAR;
      }
      break;

    default:
      write_to_output(DC::getInstance()->menu, d);
      telnet_ga(d);
      break;
    }
    break;

  case Connection::states::ARCHIVE_CHAR:
    if (arg == "ARCHIVE ME")
    {
      str_tmp << qPrintable(conn->character->name());
      write_to_output("\r\nCharacter Archived.\r\n", d);
      DC::getInstance()->update_wizlist(conn->character);
      close_socket(d);
      util_archive(str_tmp.str().c_str(), 0);
    }
    else
    {
      conn->connected = Connection::states::SELECT_MENU;
      write_to_output(DC::getInstance()->menu, d);
    }
    break;

  case Connection::states::DELETE_CHAR:
    if (arg == "ERASE ME")
    {
      dc_sprintf(buf, "%s just deleted themself.", conn->qPrintable(character->name()));
      DC::getInstance()->logentry(buf, IMMORTAL, DC::LogChannel::LOG_MORTAL);

      DC::getInstance()->TheAuctionHouse.HandleDelete(conn->character->name());
      // To remove the vault from memory
      remove_familiars(conn->character->name(), SELFDELETED);
      DC::getInstance()->vaults_.remove_vault(conn->character->name(), SELFDELETED);
      if (conn->character->clan)
      {
        remove_clan_member(conn->character->clan, conn->character);
      }
      remove_character(conn->character->name(), SELFDELETED);

      conn->character->setLevel(1);
      DC::getInstance()->update_wizlist(conn->character);
      close_socket(d);
      d = {};
    }
    else
    {
      conn->connected = Connection::states::SELECT_MENU;
      write_to_output(DC::getInstance()->menu, d);
      telnet_ga(d);
    }
    break;

  case Connection::states::CONFIRM_PASSWORD_CHANGE:
    write_to_output("\r\n", d);
    if (QString(crypt(arg.c_str(), ch->player->pwd)) == ch->player->pwd)
    {
      write_to_output("Enter a new password: ", d);
      telnet_ga(d);
      conn->connected = Connection::states::RESET_PASSWORD;
      break;
    }
    else
    {
      write_to_output("Incorrect.", d);
      conn->connected = Connection::states::SELECT_MENU;
      write_to_output(DC::getInstance()->menu, d);
    }
    break;

  case Connection::states::RESET_PASSWORD:
    write_to_output("\r\n", d);

    if (arg.length() < 6)
    {
      write_to_output("Password must be at least six characters long.\r\nPassword: ", d);
      telnet_ga(d);
      return;
    }
    ch->player->pwd = crypt(qPrintable(arg), qPrintable(ch->name()));
    write_to_output("Please retype password: ", d);
    telnet_ga(d);
    conn->connected = Connection::states::CONFIRM_RESET_PASSWORD;
    break;

  case Connection::states::CONFIRM_RESET_PASSWORD:
    write_to_output("\r\n", d);

    if (QString(crypt(arg.c_str(), ch->player->pwd)) != ch->player->pwd)
    {
      write_to_output("Passwords don't match.\r\nRetype password: ", d);
      telnet_ga(d);
      conn->connected = Connection::states::RESET_PASSWORD;
      return;
    }

    write_to_output("\r\nDone.\r\n", d);
    write_to_output(DC::getInstance()->menu, d);
    conn->connected = Connection::states::SELECT_MENU;
    if (ch->getLevel() > 1)
    {
      QString blah1, blah2[50];
      // this prevents a dupe bug
      dc_strcpy(blah1, qPrintable(ch->name()));
      dc_strcpy(blah2, ch->player->pwd);
      free_char(conn->character, Trace("nanny Connection::states::CONFIRM_RESET_PASSWORD"));
      conn->character = {};
      load_char_obj(d, blah1);
      ch = conn->character;
      dc_strcpy(ch->player->pwd, blah2);
      ch->save_char_obj();
      dc_sprintf(log_buf, "%s password changed", qPrintable(ch->name()));
      DC::getInstance()->logentry(log_buf, SERAPH, DC::LogChannel::LOG_SOCKET);
    }

    break;
  case Connection::states::CLOSE:
    close_socket(d);
    break;
  }
}

// This is mostly just to keep people from putting meta-chars
// into their email address.
qint32 _parse_email(QString arg)
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
bool _parse_name(QString arg, QString name)
{
  arg = arg.trimmed();

  for (auto i = 0U; (name[i] = arg[i]) != '\0'; i++)
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
bool check_deny(class Connection *d, QString name)
{
  FILE *fpdeny = {};
  QString strdeny;
  QString bufdeny;

  dc_sprintf(strdeny, "%s/%c/%s.deny", SAVE_DIR, UPPER(name[0]), name);
  if ((fpdeny = fopen(strdeny, "rb")) == nullptr)
    return false;
  fclose(fpdeny);

  QString log_buf = {};
  dc_sprintf(log_buf, "Denying access to player %s@%s.", name, conn->getPeerOriginalAddress().toString(qPrintable()));
  DC::getInstance()->logentry(log_buf, ARCHANGEL, DC::LogChannel::LOG_MORTAL);
  file_to_string(strdeny, bufdeny);
  write_to_output(bufdeny, d);
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
    if (tmp_ch->isNonPlayer() || tmp_ch->desc != nullptr)
      continue;

    if (str_cmp(qPrintable(conn->character->name()), qPrintable(tmp_ch->name())))
      continue;

    //      if(fReconnect == false)
    //      {
    // TODO - why are we doing this?  we load the password doing load_char_obj
    // unless someone changed their password and didn't save this doesn't seem useful
    // removed 8/29/02..i think this might be related to the bug causing people
    // to morph into other people
    // if(conn->character->player)
    //  dc_strncpy( conn->character->player->pwd, tmp_ch->player->pwd, PASSWORD_LEN );
    //      }
    //      else {

    if (fReconnect == true)
    {
      free_char(conn->character, Trace("check_reconnect"));
      conn->character = tmp_ch;
      tmp_ch->desc = d;
      tmp_ch->timer = {};
      tmp_ch->sendln("Reconnecting.");

      QString log_buf = QStringLiteral("%1@%2 has reconnected.").arg(qPrintable(tmp_ch->name())).arg(conn->getPeerOriginalAddress().toString());
      act_to_room("$n has reconnected and is ready to kick ass.", tmp_ch, 0, 0, INVIS_NULL);

      if (tmp_ch->isMortalPlayer())
      {
        DC::getInstance()->logentry(log_buf, COORDINATOR, DC::LogChannel::LOG_SOCKET);
      }
      else
      {
        DC::getInstance()->logentry(log_buf, tmp_ch->getLevel(), DC::LogChannel::LOG_SOCKET);
      }

      conn->connected = Connection::states::PLAYING;

      if (tmp_ch->getSetting("mode").startsWith("character"))
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
  CharacterPtr compare = {};

  for (dold = DC::getInstance()->connections_; dold; dold = next_d)
  {
    next_d = dold->next;

    if ((dold == d) || (dold->character == 0))
      continue;

    compare = ((dold->original != 0) ? dold->original : dold->character);

    // If this is the case we fail our precondition to str_cmp
    if (name.isEmpty())
      continue;
    if (qPrintable(compare->name()) == 0)
      continue;

    if (name != qPrintable(compare->name()))
      continue;

    if (dold->connected == Connection::states::GET_NAME)
      continue;

    if (dold->connected == Connection::states::GET_OLD_PASSWORD)
    {
      free_char(dold->character, Trace("check_playing"));
      dold->character = {};
      close_socket(dold);
      continue;
    }
    close_socket(dold);
    return 0;
  }
  return 0;
}

QString str_str(QString first, QString second)
{
  QString pstr;

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
void add_command_lag(CharacterPtr ch, qint32 amount)
{
  if (ch->isMortalPlayer())
    ch->timer += amount;
}

qint32 check_command_lag(CharacterPtr ch)
{
  return ch->timer;
}

void remove_command_lag(CharacterPtr ch)
{
  ch->timer = {};
}

void update_characters()
{
  qint32 tmp, retval;
  QString log_msg, dammsg;
  affected_type af;

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
        i->send(QStringLiteral("You strain your muscles keeping the %s closed.\r\n").arg(qPrintable(fname(i->brace_at->keyword))));
        act_to_room("$n strains $s muscles keeping the $F blocked.", i, 0, i->brace_at->keyword, 0);
      }
    }
    if (IS_AFFECTED(i, AFF_POISON) && !(i->affected_by_spell(SPELL_POISON)))
    {
      DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Player %s affected by poison but not under poison spell. Removing poison affect.", qPrintable(i->name()));
      REMBIT(i->affected_by, AFF_POISON);
    }

    // handle poison
    if (IS_AFFECTED(i, AFF_POISON) && !i->fighting && i->affected_by_spell(SPELL_POISON) && i->affected_by_spell(SPELL_POISON)->location == APPLY_NONE)
    {
      qint32 tmp = number(1, 2) + i->affected_by_spell(SPELL_POISON)->duration;
      if (get_saves(i, SAVE_TYPE_POISON) > number(1, 101))
      {
        tmp *= get_saves(i, SAVE_TYPE_POISON) / 100;
        i->sendln("You feel very sick, but resist the $2poison's$R damage.");
      }
      if (tmp)
      {
        dc_sprintf(dammsg, "$B%d$R", tmp);
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
    if (i->isPlayer() && i->isMortalPlayer() && DC::getInstance()->world[i->in_room].sector_type == SECT_UNDERWATER && !(i->affected_by_spell(SPELL_WATER_BREATHING) || IS_AFFECTED(i, AFF_WATER_BREATHING) || i->affected_by_spell(SKILL_SONG_SUBMARINERS_ANTHEM)))
    {
      tmp = GET_MAX_HIT(i) / 5;
      dc_sprintf(log_msg, "%s drowned in room %d.", qPrintable(i->name()), DC::getInstance()->world[i->in_room].number);
      retval = noncombat_damage(i, tmp, "You gasp your last breath and everything goes dark...", "$n stops struggling as $e runs out of oxygen.", log_msg, KILL_DROWN);
      if (SOMEONE_DIED(retval))
        continue;
      else
      {
        dc_sprintf(dammsg, "$B%d$R", tmp);
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
        i->timer = {};
    }

    i->shotsthisround = {};

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
  ObjectPtr obj, tmp_obj;

  for (obj = DC::getInstance()->object_list; obj; obj = tmp_obj)
  {
    tmp_obj = obj->next;
    if (DC::getInstance()->obj_index[obj->item_number].vnum() == SILENCE_OBJ_NUMBER)
    {
      if (obj->obj_flags.value[0] == 0)
        extract_obj(obj);
      else
        obj->obj_flags.value[0]--;
    }
  }
}

void checkConsecrate(qint32 pulseType)
{
  ObjectPtr obj, tmp_obj;
  CharacterPtr ch = {}, tmp_ch, next_ch;
  qint32 align, amount, spl = {};
  QString buf;

  if (pulseType == DC::PULSE_REGEN)
  {
    for (obj = DC::getInstance()->object_list; obj; obj = tmp_obj)
    {
      tmp_obj = obj->next;
      if (DC::getInstance()->obj_index[obj->item_number].vnum() == CONSECRATE_OBJ_NUMBER)
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
                  ch->send(QStringLiteral("You sense your consecration of %s has ended.\r\n").arg(DC::getInstance()->world[obj->in_room].name));
                else
                  ch->sendln("Runes upon the ground glow brightly, then fade to nothing.\r\nYour holy consecration has ended.");
              }
              else
              {
                if (ch->in_room != obj->in_room)
                  ch->send(QStringLiteral("You sense your desecration of %s has ended.\r\n").arg(DC::getInstance()->world[obj->in_room].name));
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
      if (DC::getInstance()->obj_index[obj->item_number].vnum() == CONSECRATE_OBJ_NUMBER)
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
              dc_sprintf(buf, "%d", amount);
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
                act_to_room("The strength of $N's desecration proves to powerful for $n to overcome and $e drops dead!", tmp_ch, 0, ch, NOTVICT);
                act_to_victim("The strength of your desecration proves to powerful for $n to overcome and $e drops dead!", tmp_ch, 0, ch, 0);
                if (tmp_ch->isPlayer())
                {
                  act_to_character("The strength of $N's desecration proves fatal and the world fades to black...", tmp_ch, 0, ch, 0);
                  tmp_ch->sendln("You have been KILLED!!\r\n");
                }
                group_gain(ch, tmp_ch);
                fight_kill(ch, tmp_ch, TYPE_CHOOSE, SPELL_DESECRATE);
                continue;
              }
              dc_sprintf(buf, "%d", amount);
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
              dc_sprintf(buf, "%d", amount);
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
                act_to_room("The strength of $N's consecration proves to powerful for $n to overcome and $e drops dead!", tmp_ch, 0, ch, NOTVICT);
                act_to_victim("The strength of your consecration proves to powerful for $n to overcome and $e drops dead!", tmp_ch, 0, ch, 0);
                if (tmp_ch->isPlayer())
                {
                  act_to_character("The strength of $N's consecration proves fatal and the world fades to black...", tmp_ch, 0, ch, 0);
                  tmp_ch->sendln("You have been KILLED!!\r\n");
                }
                group_gain(ch, tmp_ch);
                fight_kill(ch, tmp_ch, TYPE_CHOOSE, SPELL_CONSECRATE);
                continue;
              }
              dc_sprintf(buf, "%d", amount);
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
}

/* check name to see if it is listed in the file of forbidden player names */
bool DC::on_forbidden_name_list(QString name)
{
  FILE *nameList;
  QString buf;
  bool found = false;
  qint32 i;

  nameList = fopen(FORBIDDEN_NAME_FILE, "ro");
  if (!nameList)
  {
    DC::getInstance()->logentry(QStringLiteral("Failed to open forbidden name file!"), 0, DC::LogChannel::LOG_MISC);
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
  if (d == nullptr || conn->character == nullptr)
  {
    return;
  }

  CharacterPtr ch = conn->character;
  QString buffer, races_buffer;
  buffer += "\r\nRacial Bonuses and Pentalties:\r\n";
  buffer += "$B$7   Race   STR DEX CON INT WIS$R\r\n";
  for (qint32 race = 1; race <= 9; race++)
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
  write_to_output(buffer.c_str(), d);
  telnet_ga(d);
}

bool handle_get_race(Connection *d, QString arg)
{
  if (d == nullptr || conn->character == nullptr || arg == "")
  {
    return false;
  }

  for (quint32 race = 1; race <= 9; race++)
  {
    if (races[race].lowercase_name == arg)
    {
      conn->character->race = race;
      return true;
    }
  }

  quint32 race = {};
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

  conn->character->race = race;
  apply_race_attributes(conn->character);

  return true;
}

void show_question_class(Connection *d)
{
  if (d == nullptr || conn->character == nullptr)
  {
    return;
  }

  CharacterPtr ch = conn->character;
  QString buffer, classes_buffer;
  buffer += "\r\n   Class$R\r\n";
  qint32 clss;
  for (clss = 1; clss <= CLASS_MAX_PROD; clss++)
  {
    if (pc_clss_types[clss] != nullptr)
    {
      if (!is_clss_race_compat(ch, clss, ch->race))
      {
        buffer += fmt::format("{:2}. {:11} (Unavailble for your race)\r\n", clss, pc_clss_types[clss]);
      }
      else
      {
        buffer += fmt::format("{:2}. {:11}\r\n", clss, pc_clss_types[clss]);
        classes_buffer += QString(pc_clss_types3[clss]);
        if (clss < CLASS_MAX_PROD)
        {
          classes_buffer += ",";
        }
      }
    }
  }

  buffer += "Type 'back' to go back and pick a different race.\r\n";
  buffer += "Type '1-" + std::to_string(CLASS_MAX_PROD) + "," + classes_buffer + "' or 'help keyword': ";
  write_to_output(buffer.c_str(), d);
  telnet_ga(d);
}

bool handle_get_class(Connection *d, QString arg)
{
  if (d == nullptr || conn->character == nullptr || arg == "")
  {
    return false;
  }

  if (arg == "back")
  {
    conn->connected = Connection::states::QUESTION_RACE;
    return false;
  }

  const CharacterPtr ch = conn->character;

  for (quint32 clss = 1; clss <= CLASS_MAX_PROD; clss++)
  {
    if (QString(pc_clss_types[clss]) == QString(arg) || QString(pc_clss_types3[clss]) == QString(arg))
    {
      if (!is_clss_race_compat(ch, clss, ch->race))
      {
        return false;
      }
      else
      {
        GET_CLASS(conn->character) = clss;
        return true;
      }
    }
  }

  quint32 clss = {};
  try
  {
    clss = stoul(arg);
  }
  catch (...)
  {
    return false;
  }

  if (clss < 1 || clss > CLASS_MAX_PROD || !is_clss_race_compat(ch, clss, ch->race))
  {
    return false;
  }

  GET_CLASS(conn->character) = clss;

  return true;
}

quint8 stat_data::getMin(quint8 cur, qint8 race_mod, quint8 min)
{
  if (min > cur + race_mod)
  {
    quint8 points_needed = min - (cur + race_mod);
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
  if (d == nullptr || conn->character == nullptr)
  {
    return;
  }

  if (conn->stats == nullptr)
  {
    conn->stats = new stat_data;

    CharacterPtr ch = conn->character;
    qint32 race = conn->stats->race = ch->race;
    qint8 clss = conn->stats->clss = GET_CLASS(ch);

    // Current
    conn->stats->str[0] = 12;
    conn->stats->dex[0] = 12;
    conn->stats->con[0] = 12;
    conn->stats->tel[0] = 12;
    conn->stats->wis[0] = 12;

    conn->stats->points = 23;

    conn->stats->setMin();
  }

  CharacterPtr ch = conn->character;
  quint32 race = ch->race;
  quint32 clss = GET_CLASS(ch);
  QString buffer = fmt::format("\r\nRace: {}\r\n", races[race].singular_name);
  buffer += fmt::format("Class: {}\r\n", classes[clss].name);
  buffer += fmt::format("Points left to assign: {}\r\n", conn->stats->points);
  buffer += fmt::format("## Attribute      Current  Racial Offsets  Total\r\n");
  if (conn->stats->selection == attribute_t::STRENGTH)
  {
    buffer += fmt::format("1. $B*Strength*$R     {:2}      {:2}               {:2}\r\n",
                          conn->stats->str[0], races[race].mod_str, conn->stats->str[0] + races[race].mod_str);
  }
  else
  {
    buffer += fmt::format("1. Strength       {:2}      {:2}               {:2}\r\n",
                          conn->stats->str[0], races[race].mod_str, conn->stats->str[0] + races[race].mod_str);
  }

  if (conn->stats->selection == attribute_t::DEXTERITY)
  {
    buffer += fmt::format("2. $B*Dexterity*$R    {:2}      {:2}               {:2}\r\n",
                          conn->stats->dex[0], races[race].mod_dex, conn->stats->dex[0] + races[race].mod_dex);
  }
  else
  {
    buffer += fmt::format("2. Dexterity      {:2}      {:2}               {:2}\r\n",
                          conn->stats->dex[0], races[race].mod_dex, conn->stats->dex[0] + races[race].mod_dex);
  }

  if (conn->stats->selection == attribute_t::INTELLIGENCE)
  {
    buffer += fmt::format("3. $B*Intelligence*$R {:2}      {:2}               {:2}\r\n",
                          conn->stats->tel[0], races[race].mod_int, conn->stats->tel[0] + races[race].mod_int);
  }
  else
  {
    buffer += fmt::format("3. Intelligence   {:2}      {:2}               {:2}\r\n",
                          conn->stats->tel[0], races[race].mod_int, conn->stats->tel[0] + races[race].mod_int);
  }

  if (conn->stats->selection == attribute_t::WISDOM)
  {
    buffer += fmt::format("4. $B*Wisdom*$R       {:2}      {:2}               {:2}\r\n",
                          conn->stats->wis[0], races[race].mod_wis, conn->stats->wis[0] + races[race].mod_wis);
  }
  else
  {
    buffer += fmt::format("4. Wisdom         {:2}      {:2}               {:2}\r\n",
                          conn->stats->wis[0], races[race].mod_wis, conn->stats->wis[0] + races[race].mod_wis);
  }

  if (conn->stats->selection == attribute_t::CONSTITUTION)
  {
    buffer += fmt::format("5. $B*Constitution*$R {:2}      {:2}               {:2}\r\n",
                          conn->stats->con[0], races[race].mod_con, conn->stats->con[0] + races[race].mod_con);
  }
  else
  {
    buffer += fmt::format("5. Constitution   {:2}      {:2}               {:2}\r\n",
                          conn->stats->con[0], races[race].mod_con, conn->stats->con[0] + races[race].mod_con);
  }

  if (conn->stats->selection == attribute_t::UNDEFINED)
  {
    buffer += "Type 1-5 or help strength,wisdom,etc: ";
  }
  else if (conn->stats->points > 0)
  {
    buffer += "Type max, +, -, 1-5, confirm or help strength,wisdom,etc: ";
  }
  else
  {
    buffer += "Type -, 1-5, confirm or help strength,wisdom,etc: ";
  }
  write_to_output(buffer.c_str(), d);
  telnet_ga(d);
}

bool handle_get_stats(Connection *d, QString arg)
{
  if (arg != "+" && arg != "-" && arg != "confirm" && arg != "max")
  {
    quint32 long input = {};
    try
    {
      input = stoul(arg);
    }
    catch (...)
    {
      input = {};
    }

    if (input >= 0 || input <= 5)
    {
      conn->stats->selection = static_cast<decltype(conn->stats->selection)>(input);
    }
    else
    {
      conn->stats->selection = attribute_t::UNDEFINED;
      write_to_output("Invalid number specified.\r\n", d);
    }

    return false;
  }

  if (conn->stats->selection != attribute_t::UNDEFINED)
  {
    if (arg == "+")
    {
      conn->stats->increase(1);
    }
    else if (arg == "max")
    {
      conn->stats->increase(conn->stats->points);
    }
    else if (arg == "-")
    {
      conn->stats->decrease(1);
      conn->stats->setMin();
    }
    else if (arg.find("help") == 0)
    {
      arg.erase(0, 5);
      do_help(conn->character, arg.data());
      return false;
    }
    else if (arg == "confirm")
    {
      if (conn->stats->points > 0)
      {
        write_to_output("You must assign all your points first.\r\n", d);
        return false;
      }

      CharacterPtr ch = conn->character;
      quint32 race = ch->race;
      GET_RAW_STR(ch) = conn->stats->str[0];
      GET_RAW_DEX(ch) = conn->stats->dex[0];
      GET_RAW_CON(ch) = conn->stats->con[0];
      GET_RAW_INT(ch) = conn->stats->tel[0];
      GET_RAW_WIS(ch) = conn->stats->wis[0];
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

bool apply_race_attributes(CharacterPtr ch, qint32 race)
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
    ch->alignment = {};
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

bool check_race_attributes(CharacterPtr ch, qint32 race)
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
    ch->alignment = {};
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

bool stat_data::increase(quint64 number)
{
  if (points <= 0)
  {
    return false;
  }

  qint32 *attribute_to_change = {};
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

  quint64 diff_from_natural_max = 18 - *attribute_to_change;
  if (number > diff_from_natural_max)
  {
    number = diff_from_natural_max;
  }

  points -= number;
  (*attribute_to_change) += number;
  return true;
}

bool stat_data::decrease(quint64 number)
{
  qint32 *attribute_to_change = {};
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
