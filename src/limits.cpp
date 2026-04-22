/***************************************************************************
 *  file: limits.c , Limit and gain control module.        Part of DIKUMUD *
 *  Usage: Procedures controling gain and limit.                           *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: limits.cpp,v 1.99 2014/07/04 22:00:04 jhhudso Exp $ */

#include "DC/DC.h"

qint32 FOUNTAINisPresent(CharacterPtr ch);

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
qint32 graf(qint32 age, qint32 p0, qint32 p1, qint32 p2, qint32 p3, qint32 p4, qint32 p5, qint32 p6)
{

  if (age < 15)
    return (p0); /* < 15   */
  else if (age <= 29)
    return (qint32)(p1 + (((age - 15) * (p2 - p1)) / 15)); /* 15..29 */
  else if (age <= 44)
    return (qint32)(p2 + (((age - 30) * (p3 - p2)) / 15)); /* 30..44 */
  else if (age <= 59)
    return (qint32)(p3 + (((age - 45) * (p4 - p3)) / 15)); /* 45..59 */
  else if (age <= 79)
    return (qint32)(p4 + (((age - 60) * (p5 - p4)) / 20)); /* 60..79 */
  else
    return (p6); /* >= 80 */
}

/* The three MAX functions define a characters Effective maximum */
/* Which is NOT the same as the ch->max_xxxx !!!          */
qint32 mana_limit(CharacterPtr ch)
{
  qint32 max;

  if (ch->isPlayer())
    max = (ch->max_mana);
  else
    max = (ch->max_mana);

  return (max);
}

// Previously any NPC got 0 returned.
qint32 ki_limit(CharacterPtr ch)
{
  return ch->max_ki;
}

qint32 hit_limit(CharacterPtr ch)
{
  qint32 max;

  if (ch->isPlayer())
    max = (ch->max_hit) + (graf(ch->age().year, 2, 4, 17, 14, 8, 4, 3));
  else
    max = (ch->max_hit);

  /* Class/Level calculations */

  /* Skill/Spell calculations */

  return (max);
}

const qint32 mana_regens[] = {0, 13, 13, 1, 1, 10, 9, 1, 1, 9, 1, 13, 0, 0};

/* manapoint gain pr. game hour */
qint32 Character::mana_gain_lookup(void)
{
  qint32 gain = {};
  qint32 divisor = 1;
  qint32 modifier;

  if (isNonPlayer())
    gain = getLevel();
  else
  {
    //    gain = graf(age().year, 2,3,4,6,7,8,9);

    gain = (qint32)(max_mana * (qreal)mana_regens[GET_CLASS(this)] / 100);
    switch (GET_POS(this))
    {
    case position_t::SLEEPING:
      divisor = 1;
      break;
    case position_t::RESTING:
      divisor = 2;
      break;
    case position_t::SITTING:
      divisor = 2;
      break;
    default:
      divisor = 3;
      break;
    }

    if (GET_CLASS(this) == CLASS_MAGIC_USER ||
        GET_CLASS(this) == CLASS_ANTI_PAL || GET_CLASS(this) == CLASS_RANGER)
    {
      if (GET_INT(this) < 0)
        modifier = int_app[0].mana_regen;
      else
        modifier = int_app[GET_INT(this)].mana_regen;

      modifier += GET_INT(this);
    }
    else
    {
      modifier = wis_app[GET_WIS(this)].mana_regen;
      modifier += GET_WIS(this);
    }
    gain += modifier;
  }

  if (((GET_COND(this, FULL) == 0) || (GET_COND(this, THIRST) == 0)) && getLevel() < 60)
    gain >>= 2;
  gain /= 4;
  gain /= divisor;
  gain += MIN(age().year, 100) / 5;
  if (getLevel() < 50)

    gain = (qint32)((qreal)gain * (2.0 - (qreal)getLevel() / 50.0));

  if (mana_regen > 0)
    gain += mana_regen;
  if (in_room >= 0)
    if (isSet(dc_->world[in_room].room_flags, SAFE) || check_make_camp(in_room))
      gain = (qint32)(gain * 1.25);

  if (mana_regen < 0)
    gain += mana_regen;
  return MAX(1, gain);
}

const qint32 hit_regens[] = {0, 7, 7, 9, 10, 8, 9, 12, 9, 8, 8, 7, 0, 0};

qint32 Character::hit_gain(position_t position, bool improve)
{
  qint32 gain = 1;
  affected_typePtr af;
  qint32 divisor = 1;
  qint32 learned = has_skill(SKILL_ENHANCED_REGEN);
  /* Neat and fast */
  if (isNonPlayer())
  {
    if (fighting)
      gain = (GET_MAX_HIT(this) / 24);
    else
      gain = (GET_MAX_HIT(this) / 6);
  }
  /* PC's */
  else
  {
    gain = (qint32)(max_hit * (qreal)hit_regens[GET_CLASS(this)] / 100);

    /* Position calculations    */

    switch (position)
    {
    case position_t::SLEEPING:
      divisor = 1;
      break;
    case position_t::RESTING:
      divisor = 2;
      break;
    case position_t::SITTING:
      divisor = 2;
      break;
    default:
      divisor = 3;
      break;
    }

    if (gain < 1)
      gain = 1;

    if ((af = affected_by_spell(SPELL_RAPID_MEND)))
      gain += af->modifier;

    // con multiplier modifier 15 = 1.0  30 = 1.45 (.03 increments)
    /*    if(GET_CON(this) > 15)
     gain = (qint32)(gain * ((qreal)1+ (.03 * (GET_CON(this) - 15.0))));

     if(GET_CLASS(this) == CLASS_MAGIC_USER || GET_CLASS(this) == CLASS_CLERIC || GET_CLASS(this) == CLASS_DRUID)
     gain = (qint32)((qreal)gain * 0.7);*/

    if (GET_CON(this) < 0)
      gain += con_app[0].hp_regen;
    else
      gain += con_app[GET_CON(this)].hp_regen;

    gain += GET_CON(this);
  }
  if (ISSET(affected_by, AFF_REGENERATION))
    gain += (gain / 2);

  if (learned && (!improve || skill_success(nullptr, SKILL_ENHANCED_REGEN)))
    gain += 3 + learned / 5;

  if (((GET_COND(this, FULL) == 0) || (GET_COND(this, THIRST) == 0)) && getLevel() < 60)
    gain >>= 2;

  gain /= 4;
  //  gain -= MIN(age().year,100) / 10;

  gain /= divisor;
  if (hit_regen > 0)
    gain += hit_regen;
  if (getLevel() < 50)
    gain = (qint32)((qreal)gain * (2.0 - (qreal)getLevel() / 50.0));

  if (in_room >= 0)
    if (isSet(dc_->world[in_room].room_flags, SAFE) || check_make_camp(in_room))
      gain = (qint32)(gain * 1.5);
  if (hit_regen < 0)
    gain += hit_regen;
  return MAX(1, gain);
}

qint32 Character::move_gain_lookup(qint32 extra)
/* move gain pr. game hour */
{
  qint32 gain;
  qint32 divisor = 100000;
  qint32 learned = has_skill(SKILL_ENHANCED_REGEN);
  affected_typePtr af;
  bool improve = true;
  if (extra == 777)
    improve = false;

  if (isNonPlayer())
  {
    return getLevel();
  }
  else
  {
    //	gain = graf(ch->age().year, 4,5,6,7,4,3,2);
    gain = (qint32)(max_move * 0.15);
    //	gain /= 2;
    switch (getPosition())
    {
    case position_t::SLEEPING:
      divisor = 1;
      break;
    case position_t::RESTING:
      divisor = 2;
      break;
    default:
      divisor = 3;
      break;
    }
    gain += GET_DEX(this);

    if (GET_CON(this) < 0)
      gain += con_app[0].move_regen;
    else
      gain += con_app[GET_CON(this)].move_regen;

    if ((af = affected_by_spell(SPELL_RAPID_MEND)))
      gain += (qint32)(af->modifier * 1.5);
  }

  if (((GET_COND(this, FULL) == 0) || (GET_COND(this, THIRST) == 0)) && getLevel() < 60)
    gain >>= 2;
  gain /= divisor;
  gain -= MIN(100, age().year) / 10;

  if (move_regen > 0)
    gain += move_regen;

  if (learned && (!improve || skill_success(nullptr, SKILL_ENHANCED_REGEN)))
    gain += 3 + learned / 10;

  if (getLevel() < 50)
    gain = (qint32)((qreal)gain * (2.0 - (qreal)getLevel() / 50.0));

  if (in_room >= 0)
    if (isSet(dc_->world[in_room].room_flags, SAFE) || check_make_camp(in_room))
      gain = (qint32)(gain * 1.5);
  if (move_regen < 0)
    gain += move_regen;

  return MAX(1, gain);
}

void redo_hitpoints(CharacterPtr ch)
{
  /*affected_typePtr af;*/
  qint32 i, j, bonus = {};

  ch->max_hit = ch->raw_hit;
  for (i = 16; i < GET_CON(ch); i++)
    bonus += (i * i) / 30;
  //  bonus = (GET_CON(ch) * GET_CON(ch)) / 30;

  ch->max_hit += bonus;
  affected_typePtr af = ch->affected;

  while (af)
  {
    if (af->location == APPLY_HIT)
      ch->max_hit += af->modifier;
    af = af->next;
  }

  for (i = {}; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
      for (j = {}; j < ch->equipment[i]->num_affects; j++)
      {
        if (ch->equipment[i]->affected[j].location == APPLY_HIT)
          affect_modify(ch, ch->equipment[i]->affected[j].location, ch->equipment[i]->affected[j].modifier, -1, true);
      }
  }
  add_totem_stats(ch, APPLY_HIT);
}

void redo_mana(CharacterPtr ch)

{
  /*affected_typePtr af;*/
  qint32 i, j, bonus = 0, stat = {};
  if (ch->isNonPlayer())
    return;
  ch->max_mana = ch->raw_mana;

  if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL || GET_CLASS(ch) == CLASS_RANGER)
    stat = GET_INT(ch);
  else
    stat = GET_WIS(ch);

  for (i = 16; i < stat; i++)
    bonus += (i * i) / 30;

  if ((GET_CLASS(ch) == CLASS_WARRIOR) || (GET_CLASS(ch) == CLASS_THIEF) || (GET_CLASS(ch) == CLASS_BARBARIAN) || (GET_CLASS(ch) == CLASS_MONK))
    bonus = {};

  ch->max_mana += bonus;

  affected_typePtr af;
  af = ch->affected;
  while (af)
  {
    if (af->location == APPLY_MANA)
      ch->max_mana += af->modifier;
    af = af->next;
  }
  for (i = {}; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
      for (j = {}; j < ch->equipment[i]->num_affects; j++)
      {
        if (ch->equipment[i]->affected[j].location == APPLY_MANA)
          affect_modify(ch, ch->equipment[i]->affected[j].location, ch->equipment[i]->affected[j].modifier, -1, true);
      }
  }
  add_totem_stats(ch, APPLY_MANA);
}

void redo_ki(CharacterPtr ch)
{
  qint32 i, j;
  ch->max_ki = ch->raw_ki;
  if (GET_CLASS(ch) == CLASS_MONK)
    ch->max_ki += GET_WIS(ch) > 15 ? GET_WIS(ch) - 15 : 0;
  else if (GET_CLASS(ch) == CLASS_BARD)
    ch->max_ki += GET_INT(ch) > 15 ? GET_INT(ch) - 15 : 0;

  affected_typePtr af;
  af = ch->affected;
  while (af)
  {
    if (af->location == APPLY_KI)
      ch->max_ki += af->modifier;
    af = af->next;
  }

  for (i = {}; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
      for (j = {}; j < ch->equipment[i]->num_affects; j++)
      {
        if (ch->equipment[i]->affected[j].location == APPLY_KI)
          affect_modify(ch, ch->equipment[i]->affected[j].location, ch->equipment[i]->affected[j].modifier, -1, true);
      }
  }
  add_totem_stats(ch, APPLY_KI);
}

/* Gain maximum in various */
void advance_level(CharacterPtr ch, bool is_conversion)
{
  qint32 add_hp = {};
  qint32 add_mana = 1;
  qint32 add_moves = {};
  qint32 add_ki = {};
  qint32 add_practices;
  qint32 i;
  QString buf;

  auto effective_level = MAX(ch->getLevel(), 1);
  auto effective_con = MAX(GET_CON(ch), 2);
  switch (GET_CLASS(ch))
  {
  case CLASS_MAGIC_USER:
    add_ki += (effective_level % 2);
    add_hp += dc_->number(3, 6);
    add_mana += dc_->number(5, 10);
    add_moves += dc_->number(1, (effective_con / 2));
    break;

  case CLASS_CLERIC:
    add_ki += (effective_level % 2);
    add_hp += dc_->number(4, 8);
    add_mana += dc_->number(4, 8);
    add_moves += dc_->number(1, (effective_con / 2));
    break;

  case CLASS_THIEF:
    add_ki += (effective_level % 2);
    add_hp += dc_->number(4, 11);
    add_moves += dc_->number(1, (effective_con / 2));
    break;

  case CLASS_WARRIOR:
    add_ki += (effective_level % 2);
    add_hp += dc_->number(14, 18);
    add_moves += dc_->number(1, (effective_con / 2));
    break;

  case CLASS_ANTI_PAL:
    add_ki += (effective_level % 2);
    add_hp += dc_->number(8, 12);
    add_mana += dc_->number(3, 5);
    add_moves += dc_->number(1, (effective_con / 2));
    break;

  case CLASS_PALADIN:
    add_ki += (effective_level % 2);
    add_hp += dc_->number(10, 14);
    add_mana += dc_->number(2, 4);
    add_moves += dc_->number(1, (effective_con / 2));
    break;

  case CLASS_BARBARIAN:
    add_ki += (effective_level % 2);
    add_hp += dc_->number(16, 20);
    add_moves += dc_->number(1, (effective_con / 2));
    break;

  case CLASS_MONK:
    add_ki += 1;
    add_hp += dc_->number(10, 14);
    add_moves += dc_->number(1, (effective_con / 2));
    GET_AC(ch) += -2;
    break;

  case CLASS_RANGER:
    add_ki += (effective_level % 2);
    add_hp += dc_->number(8, 12);
    add_mana += dc_->number(3, 5);
    add_moves += dc_->number(1, (effective_con / 2));
    break;

  case CLASS_BARD:
    add_ki += 1;
    add_hp += dc_->number(6, 10);
    add_mana += 0;
    add_moves += dc_->number(1, (effective_con / 2));
    break;

  case CLASS_DRUID:
    add_ki += (effective_level % 2);
    ;
    add_hp += dc_->number(5, 9);
    add_mana += dc_->number(4, 9);
    add_moves += dc_->number(1, (effective_con / 2));
    break;

  default:
    dc_->logentry(u"Unknown class in advance level?"_s, OVERSEER, DC::LogChannel::LOG_BUG);
    return;
  }

  /*  if ((GET_CLASS(ch) == CLASS_MAGIC_USER) ||
   (GET_CLASS(ch) == CLASS_RANGER) ||
   (GET_CLASS(ch) == CLASS_ANTI_PAL))
   add_mana +=  int_app[GET_INT(ch)].mana_gain;
   else if (GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_DRUID ||
   GET_CLASS(ch) == CLASS_PALADIN)
   add_mana += wis_app[GET_WIS(ch)].mana_gain;
   */
  if (GET_CON(ch) < 0)
    add_hp += con_app[0].hp_gain;
  else
    add_hp += con_app[GET_CON(ch)].hp_gain;

  if (GET_DEX(ch) < 0)
    add_moves += dex_app[0].move_gain;
  else
    add_moves += dex_app[GET_DEX(ch)].move_gain;

  add_hp = MAX(1, add_hp);
  add_mana = MAX(0, add_mana);
  add_moves = MAX(1, add_moves);
  add_practices = 1 + wis_app[GET_WIS(ch)].bonus;

  // hp and mana have stat bonuses related to level so have to have their stuff recalculated
  ch->raw_hit += add_hp;
  ch->raw_mana += add_mana;
  redo_hitpoints(ch);
  redo_mana(ch);
  // move and ki aren't stat related, so we just add directly to the totals
  ch->raw_move += add_moves;
  ch->max_move += add_moves;

  ch->raw_ki += add_ki;
  ch->max_ki += add_ki;
  redo_ki(ch); // Ki gets level bonuses now
  if (!ch->isNonPlayer() && !is_conversion)
    ch->player->practices += add_practices;

  dc_sprintf(buf, "Your gain is: %d/%d hp, %d/%d m, %d/%d mv, %d/%d prac, %d/%d ki.\r\n", add_hp, GET_MAX_HIT(ch), add_mana, GET_MAX_MANA(ch), add_moves,
             GET_MAX_MOVE(ch),
             ch->isNonPlayer() ? 0 : add_practices, ch->isNonPlayer() ? 0 : ch->player->practices, add_ki, GET_MAX_KI(ch));
  if (!is_conversion)
    ch->send(buf);

  if (effective_level % 3 == 0)
    for (qint32 i = {}; i <= SAVE_TYPE_MAX; i++)
      ch->saves[i]++;

  ch->fillHP();
  GET_MANA(ch) = GET_MAX_MANA(ch);
  ch->setMove(GET_MAX_MOVE(ch));
  GET_KI(ch) = GET_MAX_KI(ch);

  if (effective_level > IMMORTAL)
    for (i = {}; i < 3; i++)
      ch->conditions[i] = -1;

  if (effective_level > 10 && !isSet(ch->player->toggles, Player::PLR_REMORTED))
  {
    auto &vault = dc_->vaults_.has_vault(ch->name());
    if (vault)
    {
      ch->sendln(u"10 lbs has been added to your vault!"_s);
      vault.size += 10;
      dc_->vaults_.save(vault.owner);
    }
  }

  if (effective_level == 6)
    ch->sendln(u"You are now able to participate in pkilling!\r\nRead HELP PKILL for more information."_s);
  if (effective_level == 10)
  {
    ch->sendln(u"You have been given a vault in which to place your valuables!\r\nRead HELP VAULT for more information."_s);
    dc_->vaults_.add_new_vault(ch->name(), 0);
  }
  if (effective_level == 11)
    ch->sendln(u"It now costs you $B$5gold$R every time you recall."_s);
  if (effective_level == 20)
    ch->sendln(u"You will no longer keep your equipment when you suffer a death to a mob.\r\nThere is now a chance you may lose attribute points when you die to a mob.\r\nRead HELP RDEATH and HELP STAT LOSS for more information."_s);
  if (effective_level == 40)
    ch->sendln(u"You are now able to use the Anonymous command. See \"HELP ANON\" for details."_s);
  if (effective_level == 50)
    ch->sendln(u"The protective covenant of your corpse weakens, upon death players may steal 1 item from you. (See help LOOT for details)"_s);
}

void gain_exp(CharacterPtr ch, qint64 gain)
{
  qint32 x = {};
  qint64 y;

  if (ch->isImmortalPlayer())
    return;

  y = exp_table[ch->getLevel() + 1];

  if (ch->exp >= y)
    x = 1;

  /*  if(ch->exp > 2000000000)
   {
   ch->sendln(u"You have hit the 2 billion xp cap.  Convert or meta chode."_s);
   return;
   }*/
  ch->exp += gain;
  if (ch->exp < 0)
    ch->exp = {};

  void golem_gain_exp(CharacterPtr ch);

  if (ch->isPlayer() && ch->player->golem && ch->in_room == ch->player->golem->in_room) // Golems get mage's exp, when they're in the same room
    gain_exp(ch->player->golem, gain);

  if (ch->isNonPlayer() && dc_->mob_index_[ch->mobdata->nr].vnum() == 8) // it's a golem
    golem_gain_exp(ch);

  if (ch->isNonPlayer())
    return;

  if (!x && ch->exp >= y)
  {
    ch->sendln(u"You now have enough experience to level!"_s);
    if (ch->getLevel() == 1)
      ch->send(u"$B$2An acolyte of Pirahna tells you, 'To find the way to your guild, young %s, please read $7HELP GUILD$2'$R\r\n"_s.arg(pc_clss_types[GET_CLASS(ch)]));
  }
}

void gain_exp_regardless(CharacterPtr ch, qint32 gain)
{
  ch->exp += (qint64)gain;

  if (ch->exp < 0)
    ch->exp = {};

  if (ch->isNonPlayer())
    return;

  while (ch->exp >= (qint32)exp_table[ch->getLevel() + 1])
  {
    ch->send(u"You raise a level!!  "_s);
    ch->incrementLevel();
    advance_level(ch, 0);
  }
}

void gain_condition(CharacterPtr ch, qint32 condition, qint32 value)
{
  bool intoxicated;

  //    if(GET_COND(ch, condition)==-1) /* No change */
  //	return;

  if (condition == FULL && IS_AFFECTED(ch, AFF_BOUNT_SONNET_HUNGER))
    return;
  if (condition == THIRST && IS_AFFECTED(ch, AFF_BOUNT_SONNET_THIRST))
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, (qint32)GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, (qint32)GET_COND(ch, condition));

  if (GET_COND(ch, condition) || ch->getLevel() >= 60)
    return;

  switch (condition)
  {
  case FULL:
  {
    ch->sendln(u"You are hungry."_s);
    return;
  }
  case THIRST:
  {
    ch->sendln(u"You are thirsty."_s);
    return;
  }
  case DRUNK:
  {
    if (intoxicated)
      ch->sendln(u"You are now sober."_s);
    return;
  }
  default:
    break;
  }

  // just for fun
  if (1 == dc_->number(1, 2000))
  {
    ch->sendln(u"You are horny"_s);
  }
}

void DC::food_update(void)
{
  ObjectPtr food = {};

  for (const auto &i : character_list)
  {
    if (i->affected_by_spell(SPELL_PARALYZE))
      continue;
    qint32 amt = -1;
    if (i->equipment[WEAR_FACE] && dc_->obj_index_[i->equipment[WEAR_FACE]->item_number].vnum() == 536)
      amt = -3;
    gain_condition(i, FULL, amt);
    if (!GET_COND(i, FULL) && i->getLevel() < 60)
    { // i'm hungry
      if (!i->isNonPlayer() && isSet(i->player->toggles, Player::PLR_AUTOEAT) && (GET_POS(i) > position_t::SLEEPING))
      {
        if (IS_DARK(i->in_room) && !i->isNonPlayer() && !i->player->holyLite && !i->affected_by_spell(SPELL_INFRAVISION))
          i->sendln(u"It's too dark to see what's safe to eat!"_s);
        else if (FOUNTAINisPresent(i))
          i->do_drink({u"fountain"_s});
        else if ((food = bring_type_to_front(i, ITEM_FOOD)))
          i->do_eat(food->name().split(' '));
        else
          i->sendln(u"You are out of food."_s);
      }
    }
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, amt);
    if (!GET_COND(i, THIRST) && i->getLevel() < 60)
    { // i'm thirsty
      if (!i->isNonPlayer() && isSet(i->player->toggles, Player::PLR_AUTOEAT) && (GET_POS(i) > position_t::SLEEPING))
      {
        if (IS_DARK(i->in_room) && !i->isNonPlayer() && !i->player->holyLite && !i->affected_by_spell(SPELL_INFRAVISION))
          i->sendln(u"It's too dark to see if there's any potable liquid around!"_s);
        else if (FOUNTAINisPresent(i))
          i->do_drink({u"fountain"_s});
        else if ((food = bring_type_to_front(i, ITEM_DRINKCON)))
          i->do_drink(food->name().split(' '));
        else
          i->sendln(u"You are out of drink."_s);
      }
    }
  }
  dc_->removeDead();
}

// Update the HP of mobs and players
// Also clears out any linkdead level 1s
void DC::point_update(void)
{
  /* characters */
  const auto &character_list = dc_->character_list;
  for (const auto &i : character_list)
  {
    if (i->in_room == DC::NOWHERE)
      continue;
    if (i->affected_by_spell(SPELL_POISON))
      continue;

    qint32 a;
    CharacterPtr temp;
    if (i->isPlayer() && ISSET(i->affected_by, AFF_HIDE) && (a = i->has_skill(SKILL_HIDE)))
    {
      qint32 o;
      for (o = {}; o < MAX_HIDE; o++)
        i->player->hiding_from[o] = {};
      o = {};
      for (temp = dc_->world[i->in_room].people_; temp; temp = temp->next_in_room)
      {
        if (i == temp)
          continue;
        if (o >= MAX_HIDE)
          break;
        if (ch->dc_->number(1, 101) > a) // Failed.
        {
          i->player->hiding_from[o] = temp;
          i->player->hide[o++] = false;
        }
        else
        {
          i->player->hiding_from[o] = temp;
          i->player->hide[o++] = true;
        }
      }
    }

    // only heal linkalive's and mobs
    if (GET_POS(i) > position_t::DEAD && (i->isNonPlayer() || i->conn_))
    {
      i->setHP(MIN(i->getHP() + i->hit_gain(), hit_limit(i)));

      GET_MANA(i) = MIN(GET_MANA(i) + i->mana_gain_lookup(), mana_limit(i));

      i->setMove(MIN(GET_MOVE(i) + i->move_gain_lookup(), i->move_limit()));
      GET_KI(i) = MIN(GET_KI(i) + i->ki_gain_lookup(), ki_limit(i));
    }
    else if (!i->isNonPlayer() && i->getLevel() < 1 && !i->conn_)
    {
      act_to_room("$n fades away into obscurity; $s life leaving history with nothing of note.", i, 0, 0, 0);
      do_quit(i, "", cmd_t::SAVE_SILENTLY);
    }
  } /* for */
}

void DC::update_corpses_and_portals(void)
{
  // QString buf;
  ObjectPtr j, next_thing;
  ObjectPtr jj, next_thing2;
  qint32 proc = {}; // Processed items. Debugging.
  bool corpses_need_saving = false;
  void extract_obj(ObjectPtr obj); /* handler.c */
  /* objects */
  for (j = dc_->object_list; j; j = next_thing, proc++)
  {
    next_thing = j->next; /* Next in object list */
    /* Type 1 is a permanent game portal, and type 3 is a look_only
     |  object.  Type 0 is the spell portal and type 2 is a game_portal
     |  Type 4 is a no look permanent game portal
     */

    if ((j->isTotem() && isSet(j->flags_.more_flags, ITEM_POOF_AFTER_24H)) || ((j->isPortal()) && (j->isPortalTypePlayer() || j->isPortalTypeTemp())))
    {
      if (j->flags_.timer > 0)
        (j->flags_.timer)--;
      if (!(j->flags_.timer))
      {
        if (j->in_room != DC::NOWHERE)
        {
          auto str = u"%1 shimmers brightly and then fades away.\r\n"_s.arg(GET_OBJ_SHORT(j));
          str[0] = str[0].toUpper();
          send_to_room(str, j->in_room);
        }
        else if (j->in_obj && j->in_obj->in_room != DC::NOWHERE)
        {
          auto str = u"%1 shimmers brightly for a moment.\r\n"_s.arg(GET_OBJ_SHORT(j->in_obj));
          str[0] = str[0].toUpper();
          send_to_room(str, j->in_obj->in_room);
        }
        else if (j->in_obj && j->in_obj->carried_by)
        {
          act_to_character("$p shimmers brightly for a moment.", j->in_obj->carried_by, j->in_obj, 0, INVIS_NULL);
        }
        else if (j->carried_by)
        {
          act_to_character("$p shimmers brightly and then fades away.", j->carried_by, j, 0, INVIS_NULL);
        }

        if (j->isTotem() && j->in_obj && j->in_obj->flags_.type_flag == ITEM_ALTAR)
          remove_totem(j->in_obj, j);

        extract_obj(j);
        continue;
      }
    }

    /* If this is a corpse */
    else if ((GET_ITEM_TYPE(j) == ITEM_CONTAINER) && (j->flags_.value[3]))
    {
      // TODO ^^^ - makes value[3] for containers a bitvector instead of a boolean

      /* timer count down */
      if (j->flags_.timer > 0)
      {
        j->flags_.timer--;
      }

      if (!j->flags_.timer)
      {
        if (j->carried_by)
          act_to_character("$p decays in your hands.", j->carried_by, j, 0, 0);
        else if ((j->in_room != DC::NOWHERE) && (dc_->world[j->in_room].people_))
        {
          act_to_room("A quivering horde of maggots consumes $p.", dc_->world[j->in_room].people_, j, 0, INVIS_NULL);
          act_to_character("A quivering horde of maggots consumes $p.", dc_->world[j->in_room].people_, j, 0, 0);
        }
        bool corpse_contained = j->contains != nullptr;
        for (jj = j->contains; jj; jj = next_thing2)
        {
          next_thing2 = jj->next_content; /* Next in inventory */

          if (GET_ITEM_TYPE(jj) == ITEM_CONTAINER)
          {
            ObjectPtr oo, oon;
            for (oo = jj->contains; oo; oo = oon)
            {
              oon = oo->next_content;

              if (isSet(oo->flags_.more_flags, ITEM_NO_TRADE))
              {
                log_sacrifice((CharacterPtr)j, oo, true);
                extract_obj(oo);
              }
            }
          }
          if (j->in_obj)
          {
            if (isSet(jj->flags_.more_flags, ITEM_NO_TRADE))
            {
              jj->setOwner(j->getOwner());
            }

            move_obj(jj, j->in_obj);
          }
          else if (j->carried_by)
          {
            if (isSet(jj->flags_.more_flags, ITEM_NO_TRADE))
            {
              jj->setOwner(j->getOwner());
            }

            move_obj(jj, j->carried_by);
          }
          else if (j->in_room != DC::NOWHERE)
          {
            if (isSet(jj->flags_.more_flags, ITEM_NO_TRADE))
            {
              jj->setOwner(j->getOwner());
            }

            move_obj(jj, j->in_room);
          }
          else
          {
            dc_->logentry(u"BIIIG problem in limits.c!"_s, OVERSEER, DC::LogChannel::LOG_BUG);
            return;
          }
        }
        while (next_thing && next_thing->in_obj == j)
          next_thing = next_thing->next;
        // Is THIS what caused the crasher then?
        // Wtf: damnit.
        if (IS_OBJ_STAT(j, ITEM_PC_CORPSE) && corpse_contained)
          corpses_need_saving = true;
        extract_obj(j);
      }
    }
  }
  dc_->removeDead();
  if (corpses_need_saving == true)
  {
    save_corpses();
  }
  // dc_sprintf(buf, "DEBUG: Processed Objects: %d", proc);
  // dc_->logentry(buf, 108, DC::LogChannel::LOG_BUG);
  /* Now process the portals */
  // process_portals();
}

void prepare_character_for_sixty(CharacterPtr ch)
{
  if (ch->isPlayer() && DC::MAX_MORTAL_LEVEL == 60)
  {
    qint32 skl = -1;
    switch (GET_CLASS(ch))
    {
    case CLASS_MAGE:
      skl = SKILL_SPELLCRAFT;
      break;
    case CLASS_BARBARIAN:
      skl = SKILL_BULLRUSH;
      break;
    case CLASS_PALADIN:
      skl = SPELL_HOLY_AURA;
      break;
    case CLASS_MONK:
      skl = KI_OFFSET + KI_MEDITATION;
      break;
    case CLASS_WARRIOR:
      skl = SKILL_COMBAT_MASTERY;
      break;
    case CLASS_THIEF:
      skl = SKILL_CRIPPLE;
      break;
    case CLASS_RANGER:
      skl = SKILL_NAT_SELECT;
      break;
    case CLASS_CLERIC:
      skl = SPELL_DIVINE_INTER;
      break;
    case CLASS_ANTI_PAL:
      skl = SPELL_VAMPIRIC_AURA;
      break;
    case CLASS_DRUID:
      skl = SPELL_CONJURE_ELEMENTAL;
      break;
    case CLASS_BARD:
      skl = SKILL_SONG_HYPNOTIC_HARMONY;
      break;
    }
    if (ch->has_skill(skl) && !isSet(ch->player->toggles, Player::PLR_50PLUS))
    {
      SET_BIT(ch->player->toggles, Player::PLR_50PLUS);
      qint32 i = (ch->exp / 100000000) * 500000;
      if (i > 0)
      {
        ch->send(u"$B$3You have been credited %d $B$5gold$R coins for your %ld experience.$R\r\n"_s.arg(i).arg(ch->exp));
        ch->addGold(i);
      }
      else if (ch->exp > 0)
      {
        ch->sendln(u"Since you already have your Quest Skill, your experience has been set to 0 to allow advancement to level 60."_s);
      }
      ch->exp = {};
    }
  }
}
