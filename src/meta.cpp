/***************************************************************************
 *  file: meta.cpp , Special module.                     Part of DIKUMUD *
 *  Usage: Procedures handling special procedures for object/room/mobile   *
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

#include <cassert>

#include "DC/DC.h"
#include "DC/levels.h"
#include "DC/structs.h"

#include "DC/handler.h"
#include "DC/player.h"
#include "DC/interp.h"

#include "DC/race.h"
#include "DC/db.h"
#include "DC/act.h"
#include "DC/returnvals.h"
#include "DC/const.h"
#include "DC/meta.h"

/*

 START META-PHYSICIAN

 */
qint64 new_meta_platinum_cost(qint32 start, qint32 end)
{ // This is the laziest function ever. I didn't feel like
  // figuring out a formulae to work with the ranges, so I didn't.
  qint64 platcost = {};
  if (end <= start || end < 0 || start < 0)
    return 0; // That's cheap!
  while (start < end)
  {
    if (start < 1000)
      platcost += 100 + (start / 100);
    else if (start < 1250)
      platcost += 110 + (start / 30);
    else if (start < 1500)
      platcost += 150 + (start / 30);
    else if (start < 1750)
      platcost += 200 + (start / 35);
    else if (start < 2000)
      platcost += 250 + (start / 40);
    else if (start < 3000)
      platcost += 300 + (start / 15);
    else if (start < 4000)
      platcost += 500 + (start / 20);
    else if (start < 5000)
      platcost += 700 + (start / 25);
    else if (start < 6000)
      platcost += 900 + (start / 20);
    else
      platcost += (1200 + (start / 20)) > 1500 ? 1500 : (1200 + (start / 20));
    start += 5;
  }
  return platcost;
}

qint32 r_new_meta_platinum_cost(qint32 start, qint64 plats)
{ // This is a copy of the laziest function ever. I didn't feel like
  // figuring out a formulae to work with the ranges, so I didn't.
  qint64 platcost = {};
  if (plats <= 0 || start < 0)
    return 0;
  while (platcost < plats)
  {
    if (start < 1000)
      platcost += 100 + (start / 100);
    else if (start < 1250)
      platcost += 110 + (start / 30);
    else if (start < 1500)
      platcost += 150 + (start / 30);
    else if (start < 1750)
      platcost += 200 + (start / 35);
    else if (start < 2000)
      platcost += 250 + (start / 40);
    else if (start < 3000)
      platcost += 300 + (start / 15);
    else if (start < 4000)
      platcost += 500 + (start / 20);
    else if (start < 5000)
      platcost += 700 + (start / 25);
    else if (start < 6000)
      platcost += 900 + (start / 20);
    else
      platcost += (1200 + (start / 20)) > 1500 ? 1500 : (1200 + (start / 20));
    start += 5;
  }
  return start - 5;
}

qint32 r_new_meta_exp_cost(qint32 start, qint64 exp)
{
  if (exp <= 0)
    return start;
  while (exp > 0)
  {
    exp -= new_meta_platinum_cost(start, start + 1) * 51523;
    start += 5;
  }
  return start - 5;
}

qint64 new_meta_exp_cost_one(qint32 start)
{
  if (start < 0)
    return 0;
  return new_meta_platinum_cost(start, start + 1) * 51523;
}

qint64 Character::moves_exp_spent(void)
{
  qint32 start = GET_MAX_MOVE(this) - GET_MOVE_METAS(this);
  qint64 expcost = {};
  while (start < GET_MAX_MOVE(this))
  {
    expcost += (qint32)((5000000 + (start * 2500)) * 1.2);
    start++;
  }
  return expcost;
}

qint64 Character::moves_plats_spent(void)
{
  qint64 expcost = {};
  qint32 start = GET_MAX_MOVE(this) - GET_MOVE_METAS(this);
  while (start < GET_MAX_MOVE(this))
  {
    expcost += (qint64)(((qint32)(125 + (qint32)((0.025 * start * (start / 1000 == 0 ? 1 : start / 1000)))) * 0.9));
    start++;
  }
  return expcost;
}

qint64 Character::hps_exp_spent(void)
{
  qint64 expcost = {};
  qint32 cost;
  switch (GET_CLASS(this))
  {
  case CLASS_BARBARIAN:
    cost = 2000;
    break;
  case CLASS_WARRIOR:
    cost = 2100;
    break;
  case CLASS_PALADIN:
    cost = 2200;
    break;
  case CLASS_MONK:
    cost = 2300;
    break;
  case CLASS_RANGER:
    cost = 2500;
    break;
  case CLASS_ANTI_PAL:
    cost = 2500;
    break;
  case CLASS_THIEF:
    cost = 2600;
    break;
  case CLASS_BARD:
    cost = 2600;
    break;
  case CLASS_DRUID:
    cost = 2800;
    break;
  case CLASS_CLERIC:
    cost = 2900;
    break;
  case CLASS_MAGIC_USER:
    cost = 3000;
    break;
  default:
    cost = 3000;
    break;
  }
  qint32 base = GET_MAX_HIT(this) - GET_HP_METAS(this);
  while (base < GET_MAX_HIT(this))
  {
    expcost += (qint64)((5000000 + (cost * base)) * 1.2);
    base++;
  }
  return expcost;
}

qint64 Character::hps_plats_spent(void)
{
  qint32 cost;
  qint64 platcost = {};
  switch (GET_CLASS(this))
  {
  case CLASS_BARBARIAN:
    cost = {};
    break;
  case CLASS_WARRIOR:
    cost = 10;
    break;
  case CLASS_PALADIN:
    cost = 20;
    break;
  case CLASS_MONK:
    cost = 30;
    break;
  case CLASS_RANGER:
    cost = 50;
    break;
  case CLASS_ANTI_PAL:
    cost = 50;
    break;
  case CLASS_THIEF:
    cost = 60;
    break;
  case CLASS_BARD:
    cost = 60;
    break;
  case CLASS_DRUID:
    cost = 80;
    break;
  case CLASS_CLERIC:
    cost = 90;
    break;
  case CLASS_MAGIC_USER:
    cost = 100;
    break;
  default:
    cost = 100;
    break;
  }
  qint32 base = GET_MAX_HIT(this) - GET_HP_METAS(this);
  while (base < GET_MAX_HIT(this))
  {
    platcost += (qint64)((100 + cost + (qint32)(0.025 * base * (base / 1000 == 0 ? 1 : base / 1000))) * 0.9);
    base++;
  }
  return platcost;
}

qint64 Character::mana_exp_spent(void)
{
  qint32 cost;
  qint64 expcost = {};
  switch (GET_CLASS(this))
  {
  case CLASS_PALADIN:
    cost = 2800;
    break;
  case CLASS_RANGER:
    cost = 2500;
    break;
  case CLASS_ANTI_PAL:
    cost = 2500;
    break;
  case CLASS_DRUID:
    cost = 2200;
    break;
  case CLASS_CLERIC:
    cost = 2100;
    break;
  case CLASS_MAGIC_USER:
    cost = 2000;
    break;
  default:
    return 0;
  }
  qint32 base = GET_MAX_MANA(this) - GET_MANA_METAS(this);
  while (base < GET_MAX_MANA(this))
  {
    expcost += (qint64)((5000000 + (cost * base)) * 1.2);
    base++;
  }
  return expcost;
}

qint64 Character::mana_plats_spent(void)
{
  qint32 cost;
  qint64 platcost = {};
  switch (GET_CLASS(this))
  {
  case CLASS_PALADIN:
    cost = 80;
    break;
  case CLASS_RANGER:
    cost = 50;
    break;
  case CLASS_ANTI_PAL:
    cost = 50;
    break;
  case CLASS_DRUID:
    cost = 20;
    break;
  case CLASS_CLERIC:
    cost = 10;
    break;
  case CLASS_MAGIC_USER:
    cost = {};
    break;
  default:
    return 0;
  }
  qint32 base = GET_MAX_MANA(this) - GET_MANA_METAS(this);
  while (base < GET_MAX_MANA(this))
  {
    platcost += (qint64)((100 + cost + (qint32)(0.025 * base * (base / 1000 == 0 ? 1 : base / 1000))) * 0.9);
    base++;
  }
  return platcost;
}

qint32 Character::meta_get_stat_exp_cost(attribute_t stat)
{
  qint32 xp_price;
  qint32 curr_stat = {};
  switch (stat)
  {
  case attribute_t::CONSTITUTION:
    curr_stat = this->raw_con;
    break;
  case attribute_t::STRENGTH:
    curr_stat = this->raw_str;
    break;
  case attribute_t::DEXTERITY:
    curr_stat = this->raw_dex;
    break;
  case attribute_t::INTELLIGENCE:
    curr_stat = this->raw_intel;
    break;
  case attribute_t::WISDOM:
    curr_stat = this->raw_wis;
    break;
  default:
    xp_price = 9999999;
    break;
  }
  switch (curr_stat)
  {
  case 1:
  case 2:
  case 3:
  case 4:
    xp_price = 2000000;
    break;
  case 5:
    xp_price = 3000000;
    break;
  case 6:
    xp_price = 4000000;
    break;
  case 7:
    xp_price = 5000000;
    break;
  case 8:
    xp_price = 5500000;
    break;
  case 9:
    xp_price = 6000000;
    break;
  case 10:
    xp_price = 6500000;
    break;
  case 11:
    xp_price = 7000000;
    break;
  case 12:
    xp_price = 7500000;
    break;
  case 13:
    xp_price = 8000000;
    break;
  case 14:
    xp_price = 8500000;
    break;
  case 15:
    xp_price = 9000000;
    break;
  case 16:
    xp_price = 9500000;
    break;
  case 29:
    xp_price = 25000000;
    break;
  default:
    xp_price = (curr_stat - 7) * 1000000;
    break;
  }

  //    if(this->player->statmetas > 0)
  //     xp_price += this->player->statmetas * 20000;

  return xp_price;
}

qint32 Character::meta_get_stat_plat_cost(attribute_t targetstat)
{
  qint32 plat_cost;
  qint32 stat;

  switch (targetstat)
  {
  case attribute_t::CONSTITUTION:
    stat = this->raw_con;
    break;
  case attribute_t::STRENGTH:
    stat = this->raw_str;
    break;
  case attribute_t::DEXTERITY:
    stat = this->raw_dex;
    break;
  case attribute_t::WISDOM:
    stat = this->raw_wis;
    break;
  case attribute_t::INTELLIGENCE:
    stat = this->raw_intel;
    break;
  default:
    stat = 99;
    break;
  }

  if (stat < 5)
    plat_cost = 100;
  else if (stat < 13)
    plat_cost = 250;
  else if (stat < 28)
    plat_cost = 250 + ((stat - 12) * 50);
  else if (stat == 28)
    plat_cost = 1250;
  else
    plat_cost = 1500;

  return plat_cost;
}

void Character::meta_list_stats(void)
{
  qint32 xp_price, plat_cost, max_stat;

  xp_price = meta_get_stat_exp_cost(attribute_t::STRENGTH);
  plat_cost = meta_get_stat_plat_cost(attribute_t::STRENGTH);
  max_stat = get_max_stat(this, attribute_t::STRENGTH);
  if (this->raw_str >= max_stat)
    this->send(u"$B$31)$R Str:       Your strength is already %1.\r\n"_s.arg(max_stat));
  else
    this->send(u"$B$31)$R Str: %d        Cost: %d exp + %d Platinum coins. \r\n"_s.arg((this->raw_str + 1)).arg(xp_price).arg(plat_cost));

  xp_price = meta_get_stat_exp_cost(attribute_t::DEXTERITY);
  plat_cost = meta_get_stat_plat_cost(attribute_t::DEXTERITY);
  max_stat = get_max_stat(this, attribute_t::DEXTERITY);
  if (this->raw_dex >= max_stat)
    this->send(u"$B$32)$R Dex:       Your dexterity is already %1.\r\n"_s.arg(max_stat));
  else
    this->send(u"$B$32)$R Dex: %d        Cost: %d exp + %d Platinum coins.\r\n"_s.arg((this->raw_dex + 1)).arg(xp_price).arg(plat_cost));

  xp_price = meta_get_stat_exp_cost(attribute_t::CONSTITUTION);
  plat_cost = meta_get_stat_plat_cost(attribute_t::CONSTITUTION);
  max_stat = get_max_stat(this, attribute_t::CONSTITUTION);
  if (this->raw_con >= max_stat)
    this->send(u"$B$33)$R Con:       Your constitution is already %1.\r\n"_s.arg(max_stat));
  else
    this->send(u"$B$33)$R Con: %d        Cost: %d exp + %d Platinum coins.\r\n"_s.arg((this->raw_con + 1)).arg(xp_price).arg(plat_cost));

  xp_price = meta_get_stat_exp_cost(attribute_t::INTELLIGENCE);
  plat_cost = meta_get_stat_plat_cost(attribute_t::INTELLIGENCE);
  max_stat = get_max_stat(this, attribute_t::INTELLIGENCE);
  if (this->raw_intel >= max_stat)
    this->send(u"$B$34)$R Int:       Your intelligence is already %1.\r\n"_s.arg(max_stat));
  else
    this->send(u"$B$34)$R Int: %d        Cost: %d exp + %d Platinum coins.\r\n"_s.arg((this->raw_intel + 1)).arg(xp_price).arg(plat_cost));

  xp_price = meta_get_stat_exp_cost(attribute_t::WISDOM);
  plat_cost = meta_get_stat_plat_cost(attribute_t::WISDOM);
  max_stat = get_max_stat(this, attribute_t::WISDOM);
  if (this->raw_wis >= max_stat)
    this->send(u"$B$35)$R Wis:       Your wisdom is already %1.\r\n"_s.arg(max_stat));
  else
    this->send(u"$B$35)$R Wis: %d        Cost: %d exp + %d Platinum coins.\r\n"_s.arg((this->raw_wis + 1)).arg(xp_price).arg(plat_cost));
}

quint64 Character::meta_get_moves_exp_cost(void)
{
  qint32 meta = GET_MOVE_METAS(this);
  if (GET_MAX_MOVE(this) - GET_RAW_MOVE(this) < 0)
    meta += GET_MAX_MOVE(this) - GET_RAW_MOVE(this);
  return new_meta_exp_cost_one(MAX(0, meta));
}

quint64 Character::meta_get_moves_plat_cost(qint32 amount)
{
  qint32 meta = GET_MOVE_METAS(this);
  if (GET_MAX_MOVE(this) - GET_RAW_MOVE(this) < 0)
    meta += GET_MAX_MOVE(this) - GET_RAW_MOVE(this);
  return new_meta_platinum_cost(MAX(0, meta), MAX(0, meta) + amount);
}

quint64 Character::meta_get_hps_exp_cost(void)
{
  qint32 meta = GET_HP_METAS(this);
  qint32 bonus = {};

  for (qint32 i = 16; i < GET_RAW_CON(this); i++)
    bonus += (i * i) / 30;

  meta -= bonus;

  if (GET_RAW_HIT(this) + bonus - GET_MAX_HIT(this) > 0)
    meta -= GET_RAW_HIT(this) + bonus - GET_MAX_HIT(this);

  return new_meta_exp_cost_one(MAX(0, meta));
}

quint64 Character::meta_get_hps_plat_cost(qint32 amount)
{
  qint32 meta = GET_HP_METAS(this);
  qint32 bonus = {};

  for (qint32 i = 16; i < GET_RAW_CON(this); i++)
    bonus += (i * i) / 30;

  meta -= bonus;

  if (GET_RAW_HIT(this) + bonus - GET_MAX_HIT(this) > 0)
    meta -= GET_RAW_HIT(this) + bonus - GET_MAX_HIT(this);

  return new_meta_platinum_cost(MAX(0, meta), MAX(0, meta) + amount);
}

quint64 Character::meta_get_mana_exp_cost(void)
{
  qint32 meta = GET_MANA_METAS(this);
  qint32 stat, bonus = {};

  if (GET_CLASS(this) == CLASS_MAGIC_USER || GET_CLASS(this) == CLASS_ANTI_PAL || GET_CLASS(this) == CLASS_RANGER)
    stat = GET_RAW_INT(this);
  else if (GET_CLASS(this) == CLASS_CLERIC || GET_CLASS(this) == CLASS_PALADIN || GET_CLASS(this) == CLASS_DRUID)
    stat = GET_RAW_WIS(this);
  else
    stat = {};

  for (qint32 i = 16; i < stat; i++)
    bonus += (i * i) / 30;

  meta -= bonus;

  if (GET_RAW_MANA(this) + bonus - GET_MAX_MANA(this) > 0)
    meta -= GET_RAW_MANA(this) + bonus - GET_MAX_MANA(this);

  return new_meta_exp_cost_one(MAX(0, meta));
}

quint64 Character::meta_get_mana_plat_cost(qint32 amount)
{
  qint32 meta = GET_MANA_METAS(this);
  qint32 stat, bonus = {};

  if (GET_CLASS(this) == CLASS_MAGIC_USER || GET_CLASS(this) == CLASS_ANTI_PAL || GET_CLASS(this) == CLASS_RANGER)
    stat = GET_RAW_INT(this);
  else if (GET_CLASS(this) == CLASS_CLERIC || GET_CLASS(this) == CLASS_PALADIN || GET_CLASS(this) == CLASS_DRUID)
    stat = GET_RAW_WIS(this);
  else
    stat = {};

  for (qint32 i = 16; i < stat; i++)
    bonus += (i * i) / 30;

  meta -= bonus;

  if (GET_RAW_MANA(this) + bonus - GET_MAX_MANA(this) > 0)
    meta -= GET_RAW_MANA(this) + bonus - GET_MAX_MANA(this);

  return new_meta_platinum_cost(MAX(0, meta), MAX(0, meta) + amount);
}

qint32 Character::meta_get_ki_exp_cost(void)
{
  qint32 cost, stat;
  switch (GET_CLASS(this))
  {
  case CLASS_MONK:
    cost = 7700;
    stat = GET_RAW_WIS(this) - 15;
    stat = MAX(0, stat);
    break;
  case CLASS_BARD:
    cost = 7400;
    stat = GET_RAW_INT(this) - 15;
    stat = MAX(0, stat);
    break;
  default:
    return 0;
  }
  cost = 10000000 + ((GET_MAX_KI(this) - stat) * cost);
  return (qint32)(cost * 1.2);
}

quint64 Character::meta_get_ki_plat_cost(void)
{
  quint64 cost = {};
  quint64 stat = {};
  const quint64 adjusted_max_ki = MIN<quint64>(250UL, MAX(0, GET_MAX_KI(this)));

  switch (GET_CLASS(this))
  {
  case CLASS_MONK:
    cost = 500UL;
    stat = MAX(0UL, GET_RAW_WIS(this) - 15UL);
    break;
  case CLASS_BARD:
    cost = 400UL;
    stat = MAX(0UL, GET_RAW_INT(this) - 15UL);
    break;
  default:
    return 0UL;
  }
  cost = 500UL + cost + (((adjusted_max_ki - stat) / 2UL) * ((adjusted_max_ki - stat) / 10UL));
  return static_cast<quint64>(cost * 0.9);
}

qint32 meta_dude(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, const QString arg,
                 CharacterPtr owner)
{
  QString argument;

  qint32 stat;
  qint32 choice;
  qint32 increase;
  qint64 hit_cost, mana_cost, move_cost, ki_cost = 0, hit_exp, move_exp, mana_exp, ki_exp = {};
  qint32 statplatprice = 0, max_stat = {};

  qint8 *pstat = {};
  qint32 pprice = {};

  if ((cmd != cmd_t::LIST) && (cmd != cmd_t::BUY) && (cmd != cmd_t::ESTIMATE))
    return ReturnValue::eFAILURE;

  if (IS_AFFECTED(ch, AFF_BLIND))
    return ReturnValue::eFAILURE;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (ch->getLevel() < 10)
  {
    send_to_char("$B$2The Meta-physician tells you, 'You're too low level for$R "
                 "$B$2me to waste my time on you.$R\r\n"
                 "$B$2Prove to me you are gonna stick around first!'$R.",
                 ch);
    return ReturnValue::eSUCCESS;
  }

  hit_exp = ch->meta_get_hps_exp_cost();
  move_exp = ch->meta_get_moves_exp_cost();
  mana_exp = ch->meta_get_mana_exp_cost();

  hit_cost = ch->meta_get_hps_plat_cost(1);
  move_cost = ch->meta_get_moves_plat_cost(1);
  mana_cost = ch->meta_get_mana_plat_cost(1);

  if (!ch->isNonPlayer())
  {
    ki_exp = ch->meta_get_ki_exp_cost();
    ki_cost = ch->meta_get_ki_plat_cost();
  }

  if (cmd == cmd_t::ESTIMATE)
  {
    // Estimate costs
    QString arg2;
    arg = one_argument(arg, argument);
    one_argument(arg, arg2);

    if (!is_number(arg2) || !is_number(argument))
    {
      ch->sendln("$B$2The Meta-physician tells you, 'If you want to estimate a cost, specify which and how many points.'$R ");
      ch->sendln("$B$2The Meta-physician tells you, 'Example: estimate 1 1000'$R ");
      ch->sendln("$B$31)$R Estimate hit point cost.$R ");
      ch->sendln("$B$32)$R Estimate mana cost.$R ");
      ch->sendln("$B$33)$R Estimate move cost.$R ");
      return ReturnValue::eSUCCESS;
    }

    qint32 choice = atoi(argument);
    if (choice < 1 || choice > 3)
    {
      ch->sendln("$B$2The Meta-physician tells you, 'I cannot estimate that. Type estimate by itself for a list.'$R ");
      return ReturnValue::eSUCCESS;
    }
    qint32 amount = atoi(arg2);
    if (amount < 5 || amount > 10000)
    {
      ch->sendln("$B$2The Meta-physician tells you, 'The amount cannot be over 10000 or less than 5.'$R ");
      return ReturnValue::eSUCCESS;
    }

    qint64 platcost;
    qint64 expcost;
    switch (choice)
    {
    case 1:
      platcost = ch->meta_get_hps_plat_cost(amount);
      break;
    case 2:
      platcost = ch->meta_get_mana_plat_cost(amount);
      break;
    case 3:
      platcost = ch->meta_get_moves_plat_cost(amount);
      break;
    }
    expcost = platcost * 51523;
    ch->send(u"$B$2The Meta-physician tells you, 'That would cost you %ld platinum and %ld experience.'$R \r\n"_s.arg(platcost).arg(expcost));
    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::LIST)
  { /* List */
    ch->sendln("$B$2The Meta-physician tells you, 'This is what I can do for you...'$R ");

    ch->sendln("$BAttribute Meta:$R");
    ch->meta_list_stats();

    ch->sendln("$BStatistic Meta:$R");
    if (hit_exp && hit_cost)
      ch->sendln("$B$36)$R Add 5 points to your hit points:   %ld experience points and %ld Platinum coins.\r\n").arg(hit_exp).arg(hit_cost));
    else
      ch->sendln("$B$36)$R Add to your hit points:   You cannot do ch.");

    if (hit_exp && hit_cost)
      ch->sendln("$B$37)$R Add 1 point to your hit points:   %ld experience points and %ld Platinum coins.").arg((qint64)(hit_exp / 5 * 1.1)).arg((qint64)(hit_cost / 5 * 1.1)));
    else
      ch->sendln("$B$37)$R Add to your hit points:   You cannot do ch.");

    if (mana_exp && mana_cost)
      ch->sendln(u"$B$38)$R Add 5 points to your mana points:  %ld experience points and %ld Platinum coins."_s.arg(mana_exp).arg(mana_cost));
    else
      ch->sendln("$B$38)$R Add to your mana points:  You cannot do ch.");

    if (mana_exp && mana_cost)
      ch->sendln("$B$39)$R Add 1 point to your mana points:   %ld experience points and %ld Platinum coins.\r\n").arg((qint64)(mana_exp / 5 * 1.1)).arg((qint64)(mana_cost / 5 * 1.1)));
    else
      ch->sendln("$B$39)$R Add to your mana points:   You cannot do ch.");

    if (move_exp && move_cost)
      ch->sendln(u"$B$310)$R Add 5 points to your movement points: %ld experience points and %ld Platinum coins."_s.arg(move_exp).arg(move_cost));
    else
      ch->sendln("$B$310)$R Add to your movement points:  You cannot do ch.");

    if (move_exp && move_cost)
      ch->sendln("$B$311)$R Add 1 points to your movement points:   %ld experience points and %ld Platinum coins.").arg((qint64)(move_exp / 5 * 1.1)).arg((qint64)(move_cost / 5 * 1.1)));
    else
      ch->sendln("$B$311)$R Add to your movement points:   You cannot do ch.");

    ch->sendln("$BUse 'estimate' command to get costs for higher intervals.");

    if (!ch->isNonPlayer() && ki_cost && ki_exp)
    { // mobs can't meta ki
      ch->send(u"$B$312)$R Add a point of ki:        %ld experience points and %ld Platinum.\r\n"_s.arg(ki_exp).arg(ki_cost));
    }
    else if (!ch->isNonPlayer())
      ch->sendln("$B$312)$R Add a point of ki:        You cannot do ch.");

    ch->sendln("$BMonetary Exchange:$R");
    send_to_char(
        "$B$313)$R One (1) Platinum coin     Cost: 20,000 Gold Coins.\r\n"
        "$B$314)$R Five (5) Platinum coins   Cost: 100,000 Gold Coins.\r\n"
        "$B$315)$R 250 Platinum coins        Cost: 5,000,000 Gold Coins.\r\n"
        "$B$316)$R 100,000 Gold Coins        Cost: Five (5) Platinum coins.\r\n"
        "$B$317)$R 5,000,000 Gold Coins      Cost: 250 Platinum coins.\r\n"
        "$BOther Services:$R\r\n"
        "$B$318)$R Convert experience to gold. (100mil Exp. = 500000 Gold.)\r\n"
        "$B$319)$R A deep blue potion of healing. Cost: 25 Platinum coins.\r\n"
        "$B$320)$R Buy a practice session for 25 plats.\r\n",
        ch);
    if (!ch->isNonPlayer())
    {
      ch->send(u"$B$321)$R Add -2 points of AC for 10 qpoints. (-250 Max) (current -%d)\r\n"_s.arg(GET_AC_METAS(ch)));
      ch->sendln("$B$322)$R Add 2,000,000 experience for 1 qpoint.");
    }

    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::BUY)
  { /* buy  */
    one_argument(arg, argument);
    if ((choice = atoi(argument)) == 0 || choice < 0)
    {
      ch->sendln("The Meta-physician tells you, 'Pick a number.'");
      return ReturnValue::eSUCCESS;
    }
    switch (choice)
    {
    case 1:
      stat = ch->raw_str;
      pstat = &(ch->raw_str);
      pprice = ch->meta_get_stat_exp_cost(attribute_t::STRENGTH);
      statplatprice = ch->meta_get_stat_plat_cost(attribute_t::STRENGTH);
      max_stat = get_max_stat(ch, attribute_t::STRENGTH);
      break;
    case 2:
      stat = ch->raw_dex;
      pstat = &(ch->raw_dex);
      pprice = ch->meta_get_stat_exp_cost(attribute_t::DEXTERITY);
      statplatprice = ch->meta_get_stat_plat_cost(attribute_t::DEXTERITY);
      max_stat = get_max_stat(ch, attribute_t::DEXTERITY);
      break;
    case 3:
      stat = ch->raw_con;
      pstat = &(ch->raw_con);
      pprice = ch->meta_get_stat_exp_cost(attribute_t::CONSTITUTION);
      statplatprice = ch->meta_get_stat_plat_cost(attribute_t::CONSTITUTION);
      max_stat = get_max_stat(ch, attribute_t::CONSTITUTION);
      break;
    case 4:
      stat = ch->raw_intel;
      pstat = &(ch->raw_intel);
      pprice = ch->meta_get_stat_exp_cost(attribute_t::INTELLIGENCE);
      statplatprice = ch->meta_get_stat_plat_cost(attribute_t::INTELLIGENCE);
      max_stat = get_max_stat(ch, attribute_t::INTELLIGENCE);
      break;
    case 5:
      stat = ch->raw_wis;
      pstat = &(ch->raw_wis);
      pprice = ch->meta_get_stat_exp_cost(attribute_t::WISDOM);
      statplatprice = ch->meta_get_stat_plat_cost(attribute_t::WISDOM);
      max_stat = get_max_stat(ch, attribute_t::WISDOM);
      break;
    default:
      stat = {};
      break;
    }

    if (choice < 6)
    {

      if (GET_PLATINUM(ch) < (quint32)statplatprice)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services.  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }
      if (ch->exp < pprice)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
        return ReturnValue::eSUCCESS;
      }
      if (stat >= max_stat)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You're already as good at that as yer gonna get.'$R");
        return ReturnValue::eSUCCESS;
      }

      ch->exp -= pprice;
      GET_PLATINUM(ch) -= statplatprice;

      *pstat += 1;
      ch->player->statmetas++;

      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      QLocale::setDefault(QLocale("en_US"));
      ch->send(u"The Meta-physician takes %L1 platinum from you, leaving you with %L2 platinum.\r\n"_s.arg(statplatprice).arg(GET_PLATINUM(ch));

      // affect the stat by 0 to reflect the new raw stat
      affect_modify(ch, APPLY_STR, 0, -1, true);
      affect_modify(ch, APPLY_DEX, 0, -1, true);
      affect_modify(ch, APPLY_INT, 0, -1, true);
      affect_modify(ch, APPLY_WIS, 0, -1, true);
      affect_modify(ch, APPLY_CON, 0, -1, true);

      redo_hitpoints(ch);
      redo_mana(ch);
      redo_ki(ch);
      return ReturnValue::eSUCCESS;
    }

    if (choice == 6 && hit_exp && hit_cost)
    {
      if (ch->exp < hit_exp)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < (quint32)hit_cost)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }
      ch->exp -= hit_exp;
      GET_PLATINUM(ch) -= hit_cost;

      increase = 5;
      ch->raw_hit += increase;
      GET_HP_METAS(ch) += 5;
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      QLocale::setDefault(QLocale("en_US"));
      ch->send(u"The Meta-physician takes %L1 platinum from you, leaving you with %L2 platinum.\r\n"_s.arg(hit_cost).arg(GET_PLATINUM(ch)));
      redo_hitpoints(ch);
      return ReturnValue::eSUCCESS;
    }

    if (choice == 7 && hit_exp && hit_cost)
    {
      hit_exp = (qint32)(hit_exp / 5 * 1.1);
      hit_cost = (qint32)(hit_cost / 5 * 1.1);

      if (ch->exp < hit_exp)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < (quint32)hit_cost)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!$R'");
        return ReturnValue::eSUCCESS;
      }
      ch->exp -= hit_exp;
      GET_PLATINUM(ch) -= hit_cost;

      increase = 1;
      ch->raw_hit += increase;
      GET_HP_METAS(ch) += 1;
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      ch->send(u"The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", hit_cost, GET_PLATINUM(ch)));
      redo_hitpoints(ch);
      return ReturnValue::eSUCCESS;
    }

    if (choice == 8 && mana_exp && mana_cost)
    {

      if (ch->exp < mana_exp)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < (quint32)mana_cost)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }

      ch->exp -= mana_exp;
      GET_PLATINUM(ch) -= mana_cost;

      increase = 5;
      ch->raw_mana += increase;
      GET_MANA_METAS(ch) += 5;
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      ch->send(u"The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", mana_cost, GET_PLATINUM(ch)));
      redo_mana(ch);
      return ReturnValue::eSUCCESS;
    }

    if (choice == 9 && mana_exp && mana_cost)
    {
      mana_exp = (qint32)(mana_exp / 5 * 1.1);
      mana_cost = (qint32)(mana_cost / 5 * 1.1);

      if (ch->exp < mana_exp)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < (quint32)mana_cost)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }

      ch->exp -= mana_exp;
      GET_PLATINUM(ch) -= mana_cost;

      increase = 1;
      ch->raw_mana += increase;
      GET_MANA_METAS(ch) += 1;
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      ch->send(u"The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", mana_cost, GET_PLATINUM(ch)));
      redo_mana(ch);
      return ReturnValue::eSUCCESS;
    }

    if (choice == 10 && move_exp && move_cost)
    {
      if (ch->exp < move_exp)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < (quint32)move_cost)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }

      ch->exp -= move_exp;
      GET_PLATINUM(ch) -= move_cost;
      ch->raw_move += 5;
      ch->max_move += 5;
      GET_MOVE_METAS(ch) += 5;
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      ch->send(u"The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", move_cost, GET_PLATINUM(ch)));
      redo_hitpoints(ch);
      redo_mana(ch);
      return ReturnValue::eSUCCESS;
    }

    if (choice == 11 && move_exp && move_cost)
    {
      move_exp = (qint32)(move_exp / 5 * 1.1);
      move_cost = (qint32)(move_cost / 5 * 1.1);

      if (ch->exp < move_exp)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < (quint32)move_cost)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }

      ch->exp -= move_exp;
      GET_PLATINUM(ch) -= move_cost;
      ch->raw_move += 1;
      ch->max_move += 1;
      GET_MOVE_METAS(ch) += 1;
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      ch->send(u"The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", move_cost, GET_PLATINUM(ch)));
      redo_hitpoints(ch);
      redo_mana(ch);
      return ReturnValue::eSUCCESS;
    }
    if (choice == 12 && ki_exp && ki_cost)
    {
      if (ch->isNonPlayer())
      {
        ch->sendln("Mobs cannot meta ki.");
        return ReturnValue::eSUCCESS;
      }
      if (ch->exp < ki_exp)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < (quint32)(ki_cost))
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }

      ch->exp -= ki_exp;
      GET_PLATINUM(ch) -= ki_cost;

      ch->raw_ki += 1;
      GET_KI_METAS(ch) += 1;
      redo_ki(ch);
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      ch->send(u"The Meta-physician takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", ki_cost, GET_PLATINUM(ch)));
      return ReturnValue::eSUCCESS;
    }

    if (choice == 13)
    {
      if (ch->isPlayerGoldThief())
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You cannot do ch because of your criminal actions!'$R");
        return ReturnValue::eSUCCESS;
      }
      if (ch->getGold() < 20000)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }
      ch->removeGold(20000);
      GET_PLATINUM(ch) += 1;
      ch->sendln("Ok.");
      return ReturnValue::eSUCCESS;
    }
    if (choice == 14)
    {
      if (ch->isPlayerGoldThief())
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You cannot do ch because of your criminal actions!'$R");
        return ReturnValue::eSUCCESS;
      }
      if (ch->getGold() < 100000)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }

      ch->removeGold(100000);
      GET_PLATINUM(ch) += 5;
      ch->sendln("Ok.");
      return ReturnValue::eSUCCESS;
    }
    if (choice == 15)
    {
      if (!ch->isNonPlayer() && ch->isPlayerGoldThief())
      {
        ch->sendln("Your criminal acts prohibit it.");
        return ReturnValue::eSUCCESS;
      }

      if (ch->getGold() < 5000000)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }
      GET_PLATINUM(ch) += 250;
      ch->removeGold(5000000);
      ch->sendln("Ok.");
      return ReturnValue::eSUCCESS;
    }
    if (choice == 16)
    {
      if (GET_PLATINUM(ch) < 5)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R");
        return ReturnValue::eSUCCESS;
      }
      GET_PLATINUM(ch) -= 5;
      ch->addGold(100000);
      ch->sendln("Ok.");
      return ReturnValue::eSUCCESS;
    }

    if (choice == 17)
    {
      if (GET_PLATINUM(ch) < 250)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that!  SCRAM$R");
        return ReturnValue::eSUCCESS;
      }
      GET_PLATINUM(ch) -= 250;
      ch->addGold(5000000);
      ch->sendln("Ok.");
      return ReturnValue::eSUCCESS;
    }
    if (choice == 18)
    {
      if (ch->exp < 100000000)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You lack the experience.'$R");
        return ReturnValue::eSUCCESS;
      }
      if (ch->isNonPlayer())
      {
        ch->sendln("What would you have to spend $B$5gold$R on chode?");
        return ReturnValue::eSUCCESS;
      }

      ch->exp -= 100000000;
      ch->addGold(500000);

      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      return ReturnValue::eSUCCESS;
    }
    if (choice == 19)
    {
      if (GET_PLATINUM(ch) < 25)
      {
        ch->sendln("$B$2The Meta-physician tells you, 'You can't afford that!'$R");
        return ReturnValue::eSUCCESS;
      }
      ObjectPtr obj = clone_object(real_object(10003));
      if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
      {
        ch->sendln("You can't carry that many items.");
        extract_obj(obj);
        return ReturnValue::eSUCCESS;
      }

      if (IS_CARRYING_W(ch) + obj->obj_flags.weight > CAN_CARRY_W(ch))
      {
        ch->sendln("You can't carry that much weight.");
        extract_obj(obj);
        return ReturnValue::eSUCCESS;
      }
      GET_PLATINUM(ch) -= 25;
      obj_to_char(obj, ch);
      ch->sendln("$B$2The Meta-physician tells you, 'Here is your potion.'$R");
      return ReturnValue::eSUCCESS;
    }
    if (choice == 20)
    {
      if (GET_PLATINUM(ch) < 25)
      {
        ch->sendln("Costs 25 plats...which you don't have.");
        return ReturnValue::eSUCCESS;
      }
      if (ch->isNonPlayer())
      {
        ch->sendln("You can't buy practices chode...");
        return ReturnValue::eSUCCESS;
      }
      ch->sendln("The Meta-Physician gives you a practice session.");

      GET_PLATINUM(ch) -= 25;
      ch->player->practices += 1;
      return ReturnValue::eSUCCESS;
    }
    if (choice == 21)
    { // -2 AC
      if (GET_QPOINTS(ch) < 10)
      {
        ch->sendln("Costs 10 qpoints...which you don't have.");
        return ReturnValue::eSUCCESS;
      }
      if (ch->isNonPlayer())
      {
        ch->sendln("You can't buy AC, chode...");
        return ReturnValue::eSUCCESS;
      }
      if (GET_AC_METAS(ch) >= 250)
      {
        ch->sendln("You've reached the -250 AC limit that can be purchased per character.");
        return ReturnValue::eSUCCESS;
      }

      GET_QPOINTS(ch) -= 10;
      GET_AC_METAS(ch) += 2;
      GET_AC(ch) -= 2;
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      dc_->logf(110, DC::LogChannel::LOG_MORTAL, "%s metas -2 AC for 10 qpoints.", qPrintable(ch->name()));
      ch->save();

      return ReturnValue::eSUCCESS;
    }
    if (choice == 22)
    { // 2,000,000 experience
      if (GET_QPOINTS(ch) < 1)
      {
        ch->sendln("Costs 1 qpoint...which you don't have.");
        return ReturnValue::eSUCCESS;
      }
      if (ch->isNonPlayer())
      {
        ch->sendln("You can't buy experience, chode...");
        return ReturnValue::eSUCCESS;
      }

      GET_QPOINTS(ch) -= 1;
      ch->exp += 2000000;
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      dc_->logf(110, DC::LogChannel::LOG_MORTAL, "%s metas 2000000 XP for 1 qpoint.", qPrintable(ch->name()));
      ch->save();

      return ReturnValue::eSUCCESS;
    }
  }
  ch->sendln("$B$2The Meta-physician tells you, 'Buy what?!'$R");
  return ReturnValue::eSUCCESS;
}

/*

 END META-PHYSICIAN

 */

/*

 START CARDINAL THELONIUS

 */

void Character::undo_race_saves(void)
{
  switch (this->race)
  {
  case RACE_HUMAN:
    this->saves[SAVE_TYPE_FIRE] -= RACE_HUMAN_FIRE_MOD;
    this->saves[SAVE_TYPE_COLD] -= RACE_HUMAN_COLD_MOD;
    this->saves[SAVE_TYPE_ENERGY] -= RACE_HUMAN_ENERGY_MOD;
    this->saves[SAVE_TYPE_ACID] -= RACE_HUMAN_ACID_MOD;
    this->saves[SAVE_TYPE_MAGIC] -= RACE_HUMAN_MAGIC_MOD;
    this->saves[SAVE_TYPE_POISON] -= RACE_HUMAN_POISON_MOD;
    break;
  case RACE_ELVEN:
    this->saves[SAVE_TYPE_FIRE] -= RACE_ELVEN_FIRE_MOD;
    this->saves[SAVE_TYPE_COLD] -= RACE_ELVEN_COLD_MOD;
    this->saves[SAVE_TYPE_ENERGY] -= RACE_ELVEN_ENERGY_MOD;
    this->saves[SAVE_TYPE_ACID] -= RACE_ELVEN_ACID_MOD;
    this->saves[SAVE_TYPE_MAGIC] -= RACE_ELVEN_MAGIC_MOD;
    this->saves[SAVE_TYPE_POISON] -= RACE_ELVEN_POISON_MOD;
    this->spell_mitigation -= 1;
    break;
  case RACE_DWARVEN:
    this->saves[SAVE_TYPE_FIRE] -= RACE_DWARVEN_FIRE_MOD;
    this->saves[SAVE_TYPE_COLD] -= RACE_DWARVEN_COLD_MOD;
    this->saves[SAVE_TYPE_ENERGY] -= RACE_DWARVEN_ENERGY_MOD;
    this->saves[SAVE_TYPE_ACID] -= RACE_DWARVEN_ACID_MOD;
    this->saves[SAVE_TYPE_MAGIC] -= RACE_DWARVEN_MAGIC_MOD;
    this->saves[SAVE_TYPE_POISON] -= RACE_DWARVEN_POISON_MOD;
    this->melee_mitigation -= 1;
    break;
  case RACE_TROLL:
    this->saves[SAVE_TYPE_FIRE] -= RACE_TROLL_FIRE_MOD;
    this->saves[SAVE_TYPE_COLD] -= RACE_TROLL_COLD_MOD;
    this->saves[SAVE_TYPE_ENERGY] -= RACE_TROLL_ENERGY_MOD;
    this->saves[SAVE_TYPE_ACID] -= RACE_TROLL_ACID_MOD;
    this->saves[SAVE_TYPE_MAGIC] -= RACE_TROLL_MAGIC_MOD;
    this->saves[SAVE_TYPE_POISON] -= RACE_TROLL_POISON_MOD;
    this->spell_mitigation -= 2;
    break;
  case RACE_GIANT:
    this->saves[SAVE_TYPE_FIRE] -= RACE_GIANT_FIRE_MOD;
    this->saves[SAVE_TYPE_COLD] -= RACE_GIANT_COLD_MOD;
    this->saves[SAVE_TYPE_ENERGY] -= RACE_GIANT_ENERGY_MOD;
    this->saves[SAVE_TYPE_ACID] -= RACE_GIANT_ACID_MOD;
    this->saves[SAVE_TYPE_MAGIC] -= RACE_GIANT_MAGIC_MOD;
    this->saves[SAVE_TYPE_POISON] -= RACE_GIANT_POISON_MOD;
    this->melee_mitigation -= 2;
    break;
  case RACE_PIXIE:
    this->saves[SAVE_TYPE_FIRE] -= RACE_PIXIE_FIRE_MOD;
    this->saves[SAVE_TYPE_COLD] -= RACE_PIXIE_COLD_MOD;
    this->saves[SAVE_TYPE_ENERGY] -= RACE_PIXIE_ENERGY_MOD;
    this->saves[SAVE_TYPE_ACID] -= RACE_PIXIE_ACID_MOD;
    this->saves[SAVE_TYPE_MAGIC] -= RACE_PIXIE_MAGIC_MOD;
    this->saves[SAVE_TYPE_POISON] -= RACE_PIXIE_POISON_MOD;
    this->spell_mitigation -= 2;
    break;
  case RACE_HOBBIT:
    this->saves[SAVE_TYPE_FIRE] -= RACE_HOBBIT_FIRE_MOD;
    this->saves[SAVE_TYPE_COLD] -= RACE_HOBBIT_COLD_MOD;
    this->saves[SAVE_TYPE_ENERGY] -= RACE_HOBBIT_ENERGY_MOD;
    this->saves[SAVE_TYPE_ACID] -= RACE_HOBBIT_ACID_MOD;
    this->saves[SAVE_TYPE_MAGIC] -= RACE_HOBBIT_MAGIC_MOD;
    this->saves[SAVE_TYPE_POISON] -= RACE_HOBBIT_POISON_MOD;
    this->melee_mitigation -= 2;
    break;
  case RACE_GNOME:
    this->saves[SAVE_TYPE_FIRE] -= RACE_GNOME_FIRE_MOD;
    this->saves[SAVE_TYPE_COLD] -= RACE_GNOME_COLD_MOD;
    this->saves[SAVE_TYPE_ENERGY] -= RACE_GNOME_ENERGY_MOD;
    this->saves[SAVE_TYPE_ACID] -= RACE_GNOME_ACID_MOD;
    this->saves[SAVE_TYPE_MAGIC] -= RACE_GNOME_MAGIC_MOD;
    this->saves[SAVE_TYPE_POISON] -= RACE_GNOME_POISON_MOD;
    this->spell_mitigation -= 1;
    break;
  case RACE_ORC:
    this->saves[SAVE_TYPE_FIRE] -= RACE_ORC_FIRE_MOD;
    this->saves[SAVE_TYPE_COLD] -= RACE_ORC_COLD_MOD;
    this->saves[SAVE_TYPE_ENERGY] -= RACE_ORC_ENERGY_MOD;
    this->saves[SAVE_TYPE_ACID] -= RACE_ORC_ACID_MOD;
    this->saves[SAVE_TYPE_MAGIC] -= RACE_ORC_MAGIC_MOD;
    this->saves[SAVE_TYPE_POISON] -= RACE_ORC_POISON_MOD;
    this->melee_mitigation -= 1;
    break;
  default:
    break;
  }
}

bool Character::is_race_applicable(qint32 race)
{
  if (GET_CLASS(this) == CLASS_PALADIN && (race != RACE_HUMAN && race != RACE_ELVEN && race != RACE_DWARVEN))
    return false;
  if (GET_CLASS(this) == CLASS_ANTI_PAL && (race != RACE_HUMAN && race != RACE_ORC && race != RACE_DWARVEN))
    return false;
  if (GET_CLASS(this) == CLASS_BARBARIAN && race == RACE_PIXIE)
    return false;
  if (GET_CLASS(this) == CLASS_THIEF && race == RACE_GIANT)
    return false;
  switch (race)
  {
  case RACE_ELVEN:
    if (GET_RAW_DEX(this) - 2 < 10 || GET_RAW_INT(this) - 2 < 10)
      return false;
    break;
  case RACE_DWARVEN:
    if (GET_RAW_CON(this) - 2 < 10 || GET_RAW_WIS(this) - 2 < 10)
      return false;
    break;
  case RACE_HOBBIT:
    if (GET_RAW_DEX(this) - 2 < 10)
      return false;
    break;
  case RACE_PIXIE:
    if (GET_RAW_INT(this) - 2 < 10)
      return false;
    break;
  case RACE_GIANT:
    if (GET_RAW_STR(this) - 2 < 12)
      return false;
    break;
  case RACE_GNOME:
    if (GET_RAW_WIS(this) - 2 < 12)
      return false;
    break;
  case RACE_ORC:
    if (GET_RAW_CON(this) - 2 < 10 || GET_RAW_STR(this) - 2 < 10)
      return false;
    break;
  case RACE_TROLL:
    if (GET_RAW_CON(this) - 2 < 12)
      return false;
    break;
  default:
    break;
  }
  return true;
}

bool Character::would_die(void)
{
  if (GET_RAW_STR(this) < 8 || GET_RAW_CON(this) < 8 || GET_RAW_WIS(this) < 8 || GET_RAW_INT(this) < 8 || GET_RAW_DEX(this) < 8)
    return true;

  return false;
}

void Character::set_heightweight(void)
{
  switch (this->race)
  {
  case RACE_HUMAN:
    this->height = dc_->number(66, 77);
    this->weight = dc_->number(160, 200);
    break;
  case RACE_ELVEN:
    this->height = dc_->number(78, 101);
    this->weight = dc_->number(120, 160);
    break;
  case RACE_DWARVEN:
    this->height = dc_->number(42, 65);
    this->weight = dc_->number(140, 180);
    break;
  case RACE_HOBBIT:
    this->height = dc_->number(20, 41);
    this->weight = dc_->number(40, 80);
    break;
  case RACE_PIXIE:
    this->height = dc_->number(12, 33);
    this->weight = dc_->number(10, 40);
    break;
  case RACE_GIANT:
    this->height = dc_->number(106, 131);
    this->weight = dc_->number(260, 300);
    break;
  case RACE_GNOME:
    this->height = dc_->number(42, 65);
    this->weight = dc_->number(80, 120);
    break;
  case RACE_ORC:
    this->height = dc_->number(78, 101);
    this->weight = dc_->number(200, 240);
    break;
  case RACE_TROLL:
    this->height = dc_->number(102, 123);
    this->weight = dc_->number(240, 280);
    break;
  }
  dc_->logf(ANGEL, DC::LogChannel::LOG_MORTAL, "set_heightweight: %s's height set to %d", qPrintable(this->name()), GET_HEIGHT(this));
  dc_->logf(ANGEL, DC::LogChannel::LOG_MORTAL, "set_heightweight: %s's weight set to %d", qPrintable(this->name()), GET_WEIGHT(this));
}

qint32 changecost(qint32 oldrace, qint32 newrace)
{
  switch (oldrace)
  {
  case RACE_GIANT:
  case RACE_TROLL:
    if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT)
      return 7000;
    else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME)
      return 6500;
    else if (newrace == RACE_HUMAN)
      return 6000;
    else if (newrace == RACE_ELVEN || newrace == RACE_ORC)
      return 5500;
    else if (newrace == RACE_GIANT || newrace == RACE_TROLL)
      return 5000;
    break;
  case RACE_HOBBIT:
  case RACE_PIXIE:
    if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT)
      return 5000;
    else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME)
      return 5500;
    else if (newrace == RACE_HUMAN)
      return 6000;
    else if (newrace == RACE_ELVEN || newrace == RACE_ORC)
      return 6500;
    else if (newrace == RACE_GIANT || newrace == RACE_TROLL)
      return 7000;
    break;
  case RACE_DWARVEN:
  case RACE_GNOME:
    if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT)
      return 5500;
    else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME)
      return 5000;
    else if (newrace == RACE_HUMAN)
      return 5500;
    else if (newrace == RACE_ELVEN || newrace == RACE_ORC)
      return 6000;
    else if (newrace == RACE_GIANT || newrace == RACE_TROLL)
      return 6500;
    break;
  case RACE_ELVEN:
  case RACE_ORC:
    if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT)
      return 6500;
    else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME)
      return 6000;
    else if (newrace == RACE_HUMAN)
      return 5500;
    else if (newrace == RACE_ELVEN || newrace == RACE_ORC)
      return 5000;
    else if (newrace == RACE_GIANT || newrace == RACE_TROLL)
      return 5500;
    break;
  case RACE_HUMAN:
    if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT)
      return 6000;
    else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME)
      return 5500;
    else if (newrace == RACE_HUMAN)
      return 5000;
    else if (newrace == RACE_ELVEN || newrace == RACE_ORC)
      return 5500;
    else if (newrace == RACE_GIANT || newrace == RACE_TROLL)
      return 6000;
    break;
  default:
    return 1000000;
  }
  return 100000;
}

QString Character::race_message(qint32 race)
{
  static QString buf;
  if (this->race == race)
    return "You are already of this race.";
  else if (!is_race_applicable(race))
    return "You do not qualify for becoming this race.";

  dc_sprintf(buf, "%d platinum coins.", changecost(this->race, race));
  return &buf[0];
}

qint32 cardinal(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, const QString argument, CharacterPtr owner)
{
  if (cmd == cmd_t::LIST) // list
  {
    ch->sendln("$B$2Cardinal Thelonius tells you, 'Here's what I can do for you...'$R\r\nEnter \"buy <number>\" to make a selection.\r\n");
    ch->sendln("$BRace Change:$R\r\n(Remember a race change will reduce your base attributes by 2 points each.)");

    for (qint32 i = 1; i <= MAX_PC_RACE; i++)
      ch->send(u"$B$3%d)$R  %-32s - %s\r\n"_s.arg(i).arg(races[i].singular_name).arg(ch->race_message(i)));

    ch->sendln("$BOther Services:$R");

    ch->send(u"$B$3%d)$R %-32s - 1000 platinum coins.\r\n"_s.arg(MAX_PC_RACE + 1).arg("Sex Change"));
    ch->send(u"$B$3%d)$R %-32s - 50 platinum coins.\r\n"_s.arg(MAX_PC_RACE + 2).arg("A deep red vial of mana"));

    ch->sendln("$BHeight/Weight Change:$R");
    ch->heightweight(false);
    if (ch->height < races[ch->race].max_height)
      ch->send(u"$B$3%d)$R %-32s - 250 platinum coins.\r\n"_s.arg(MAX_PC_RACE + 3).arg("Increase your height by 1"));
    else
      ch->send(u"$B$3%d)$R %-32s.\r\n"_s.arg(MAX_PC_RACE + 3).arg("You cannot increase your height further"));

    if (ch->height > races[ch->race].min_height)
      ch->send(u"$B$3%d)$R %-32s - 250 platinum coins.\r\n"_s.arg(MAX_PC_RACE + 4).arg("Decrease your height by 1"));
    else
      ch->send(u"$B$3%d)$R %-32s.\r\n"_s.arg(MAX_PC_RACE + 4).arg("You cannot decrease your height further"));

    if (ch->weight < races[ch->race].max_weight)
      ch->send(u"$B$3%d)$R %-32s - 250 platinum coins.\r\n"_s.arg(MAX_PC_RACE + 5).arg("Increase your weight by 1"));
    else
      ch->send(u"$B$3%d)$R %-32s.\r\n"_s.arg(MAX_PC_RACE + 5).arg("You cannot increase your weight further"));

    if (ch->weight > races[ch->race].min_weight)
      ch->send(u"$B$3%d)$R %-32s - 250 platinum coins.\r\n"_s.arg(MAX_PC_RACE + 6).arg("Decrease your weight by 1"));
    else
      ch->send(u"$B$3%d)$R %-32s.\r\n"_s.arg(MAX_PC_RACE + 6).arg("You cannot decrease your weight further"));
    ch->heightweight(true);
    ch->send(u"$B$3%d)$R %-32s - 5 quest points.\r\n"_s.arg(MAX_PC_RACE + 7).arg("Increase your age by 1 (500 max)"));
    ch->send(u"$B$3%d)$R %-32s - 5 quest points.\r\n"_s.arg(MAX_PC_RACE + 8).arg("Decrease your age by 1  (18 min)"));

    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::BUY) // buy
  {
    QString arg, arg2;
    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);
    qint32 choice = atoi(arg);
    if (choice > 0 && choice <= MAX_PC_RACE)
    {
      if (ch->would_die())
      {
        ch->sendln("$B$2Cardinal Thelonius tells you, 'The process would kill you!'$R");
        return ReturnValue::eSUCCESS;
      }
      if (ch->race == choice)
      {
        ch->sendln("$B$2Cardinal Thelonius tells you, 'You are already a member of that race!'$R");
        return ReturnValue::eSUCCESS;
      }
      if (!ch->is_race_applicable(choice))
      {
        ch->sendln("$B$2Cardinal Thelonius tells you, 'You do not qualify for becoming that race!'$R");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < (quint32)changecost(ch->race, choice))
      {
        ch->sendln("$B$2Cardinal Thelonius tells you, 'You can't afford that!'$R");
        return ReturnValue::eSUCCESS;
      }

      if (!str_cmp(arg2, "confirm"))
      {
        GET_PLATINUM(ch) -= changecost(ch->race, choice);
        ch->undo_race_saves();

        ch->race = choice;

        GET_RAW_STR(ch) = MIN(get_max_stat(ch, attribute_t::STRENGTH), GET_RAW_STR(ch));
        GET_RAW_STR(ch) -= 2;
        GET_STR(ch) = GET_RAW_STR(ch) + GET_STR_BONUS(ch);

        GET_RAW_CON(ch) = MIN(get_max_stat(ch, attribute_t::CONSTITUTION), GET_RAW_CON(ch));
        GET_RAW_CON(ch) -= 2;
        GET_CON(ch) = GET_RAW_CON(ch) + GET_CON_BONUS(ch);

        GET_RAW_DEX(ch) = MIN(get_max_stat(ch, attribute_t::DEXTERITY), GET_RAW_DEX(ch));
        GET_RAW_DEX(ch) -= 2;
        GET_DEX(ch) = GET_RAW_DEX(ch) + GET_DEX_BONUS(ch);

        GET_RAW_WIS(ch) = MIN(get_max_stat(ch, attribute_t::WISDOM), GET_RAW_WIS(ch));
        GET_RAW_WIS(ch) -= 2;
        GET_WIS(ch) = GET_RAW_WIS(ch) + GET_WIS_BONUS(ch);

        GET_RAW_INT(ch) = MIN(get_max_stat(ch, attribute_t::INTELLIGENCE), GET_RAW_INT(ch));
        GET_RAW_INT(ch) -= 2;
        GET_INT(ch) = GET_RAW_INT(ch) + GET_INT_BONUS(ch);

        redo_hitpoints(ch);
        redo_mana(ch);
        redo_ki(ch);

        ch->do_inate_race_abilities();
        ch->verify_max_stats();
        // set_heightweight(ch);
        ch->check_hw();
        ch->recheck_height_wears();

        ch->sendln("The Cardinal prays loudly and summons the magic of the gods...");
        ch->sendln("After a brief moment of pain you are reborn!");
      }
      else
      {
        ch->send(u"$BYou must enter 'buy %1 CONFIRM' if you are positive you wish to make ch change!\r\n"_s.arg(choice));
        ch->sendln("$4NOTE$R$B: Your attributes will be adjusted to fit ch new race and then lowered by 2 points each.$R");
      }
      return ReturnValue::eSUCCESS;
    }
    else if (choice == MAX_PC_RACE + 1)
    {
      Character::sex_t newsex = {};
      if (arg2[0] == 'n')
        newsex = Character::sex_t::NEUTRAL;
      else if (arg2[0] == 'm')
        newsex = Character::sex_t::MALE;
      else if (arg2[0] == 'f')
        newsex = Character::sex_t::FEMALE;
      else
      {
        ch->send(u"Syntax: buy %d m/f/n\r\n"_s.arg(MAX_PC_RACE + 1));
        return ReturnValue::eSUCCESS;
      }
      if (GET_SEX(ch) == newsex)
      {
        ch->sendln("$B$2Cardinal Thelonius tells you, 'That wouldn't change much'$R");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < 1000)
      {
        ch->sendln("$B$2Cardinal Thelonius tells you, 'You can't afford that!'$R");
        return ReturnValue::eSUCCESS;
      }
      GET_PLATINUM(ch) -= 1000;
      GET_SEX(ch) = newsex;
      ch->sendln("The Cardinal prays loudly and summons the magic of the gods...");
      ch->sendln("After a brief moment of pain you are reborn!");
      return ReturnValue::eSUCCESS;
    }
    else if (choice == MAX_PC_RACE + 2)
    {
      if (GET_PLATINUM(ch) < 50)
      {
        ch->sendln("$B$2Cardinal Thelonius tells you, 'You can't afford that!'$R");
        return ReturnValue::eSUCCESS;
      }
      ObjectPtr obj = clone_object(real_object(10004));
      if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
      {
        ch->sendln("You can't carry that many items.");
        extract_obj(obj);
        return ReturnValue::eSUCCESS;
      }

      if (IS_CARRYING_W(ch) + obj->obj_flags.weight > CAN_CARRY_W(ch))
      {
        ch->sendln("You can't carry that much weight.");
        extract_obj(obj);
        return ReturnValue::eSUCCESS;
      }
      GET_PLATINUM(ch) -= 50;
      obj_to_char(obj, ch);
      ch->sendln("$B$2Cardinal Thelonius tells you, 'Here is your potion.'$R");
      return ReturnValue::eSUCCESS;
    }
    else if (choice >= MAX_PC_RACE + 3 && choice <= MAX_PC_RACE + 6)
    {
      choice -= MAX_PC_RACE;

      ch->heightweight(false);
      if (choice == 3 && ch->height >= races[ch->race].max_height)
      {
        ch->sendln("You cannot increase your height any more.");
        ch->heightweight(true);
        return ReturnValue::eSUCCESS;
      }
      else if (choice == 4 && ch->height <= races[ch->race].min_height)
      {
        ch->sendln("You cannot decrease your height any more.");
        ch->heightweight(true);
        return ReturnValue::eSUCCESS;
      }
      else if (choice == 5 && ch->weight >= races[ch->race].max_weight)
      {
        ch->sendln("You cannot increase your weight any more.");
        ch->heightweight(true);
        return ReturnValue::eSUCCESS;
      }
      else if (choice == 6 && ch->weight <= races[ch->race].min_weight)
      {
        ch->sendln("You cannot decrease your weight any more.");
        ch->heightweight(true);
        return ReturnValue::eSUCCESS;
      }
      ch->heightweight(true);

      if (GET_PLATINUM(ch) < 250)
      {
        ch->sendln("You cannot afford it.");
        return ReturnValue::eSUCCESS;
      }
      GET_PLATINUM(ch) -= 250;
      ch->sendln("Cardinal Thelonius gropes you.");
      if (choice == 3)
      {
        ch->height++;
        dc_->logf(ANGEL, DC::LogChannel::LOG_MORTAL, "%s metas height by 1 = %d", qPrintable(ch->name()), GET_HEIGHT(ch));
      }
      if (choice == 4)
      {
        ch->height--;
        dc_->logf(ANGEL, DC::LogChannel::LOG_MORTAL, "%s metas height by -1 = %d", qPrintable(ch->name()), GET_HEIGHT(ch));
      }
      if (choice == 5)
      {
        ch->weight++;
        dc_->logf(ANGEL, DC::LogChannel::LOG_MORTAL, "%s metas weight by 1 = %d", qPrintable(ch->name()), GET_WEIGHT(ch));
      }
      if (choice == 6)
      {
        ch->weight--;
        dc_->logf(ANGEL, DC::LogChannel::LOG_MORTAL, "%s metas weight by -1 = %d", qPrintable(ch->name()), GET_WEIGHT(ch));
      }
      return ReturnValue::eSUCCESS;
    }
    else if (choice == MAX_PC_RACE + 7)
    {
      if (GET_QPOINTS(ch) < 5)
      {
        ch->sendln("Costs 5 qpoints...which you don't have.");
        return ReturnValue::eSUCCESS;
      }
      if (ch->isNonPlayer())
      {
        ch->sendln("You can't buy age, chode...");
        return ReturnValue::eSUCCESS;
      }
      if (GET_AGE(ch) >= 500)
      {
        ch->sendln("You've reached the 500 age limit that can be purchased per character.");
        return ReturnValue::eSUCCESS;
      }

      GET_QPOINTS(ch) -= 5;
      GET_AGE_METAS(ch) += 1;
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      dc_->logf(110, DC::LogChannel::LOG_MORTAL, "%s metas 1 age for 5 qpoints.", qPrintable(ch->name()));
      ch->save();

      return ReturnValue::eSUCCESS;
    }
    else if (choice == MAX_PC_RACE + 8)
    {
      if (GET_QPOINTS(ch) < 5)
      {
        ch->sendln("Costs 5 qpoints...which you don't have.");
        return ReturnValue::eSUCCESS;
      }
      if (ch->isNonPlayer())
      {
        ch->sendln("You can't buy age, chode...");
        return ReturnValue::eSUCCESS;
      }
      if (GET_AGE(ch) <= 18)
      {
        ch->sendln("You've reached the age 18 minimum limit that can be purchased per character.");
        return ReturnValue::eSUCCESS;
      }

      GET_QPOINTS(ch) -= 5;
      GET_AGE_METAS(ch) -= 1;
      act_to_room("The Meta-physician touches $n.", ch, 0, 0, 0);
      act_to_character("The Meta-physician touches you.", ch, 0, 0, 0);
      dc_->logf(110, DC::LogChannel::LOG_MORTAL, "%s metas -1 age for 5 qpoints.", qPrintable(ch->name()));
      ch->save();

      return ReturnValue::eSUCCESS;
    }
    else
    {
      ch->sendln("$B$2Cardinal Thelonius tells you, 'I don't have that. Try \"list\".'$R");
      return ReturnValue::eSUCCESS;
    }
  }

  return ReturnValue::eFAILURE;
}

/*

 END CARDINAL THELONIUS

 */
