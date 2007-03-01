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

#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <assert.h>
#include <character.h>
#include <structs.h>
#include <utility.h>
#include <mobile.h>
#include <spells.h>
#include <room.h>
#include <handler.h>
#include <magic.h>
#include <levels.h>
#include <fight.h>
#include <obj.h>
#include <player.h>
#include <connect.h>
#include <interp.h>
#include <isr.h>
#include <race.h>
#include <db.h>
#include <sing.h>
#include <act.h>
#include <ki.h>
#include <string.h>
#include <returnvals.h>


/*

  START META-PHYSICIAN

*/
long long  new_meta_platinum_cost(int start, int end)
{ // This is the laziest function ever. I didn't feel like
  // figuring out a formulae to work with the ranges, so I didn't.
  long long platcost = 0;
  if (end <= start || end < 0 || start < 0) return 0; // That's cheap!
  while (start < end)
  {
    if (start < 1000) platcost += 100 + (start / 100);
    else if (start < 1250) platcost += 110 + (start / 30);
    else if (start < 1500) platcost += 150 + (start / 30);
    else if (start < 1750) platcost += 200 + (start / 35);
    else if (start < 2000) platcost += 250 + (start / 40);
    else if (start < 3000) platcost += 300 + (start / 15);
    else if (start < 4000) platcost += 500 + (start / 20);
    else if (start < 5000) platcost += 700 + (start / 25);
    else if (start < 6000) platcost += 900 + (start / 20);
    else platcost += (1200 + (start / 20)) > 1500 ? 1500: (1200 + (start / 20));
    start += 5;
  }
  return platcost;
}

int r_new_meta_platinum_cost(int start, long long plats)
{ // This is a copy of the laziest function ever. I didn't feel like
  // figuring out a formulae to work with the ranges, so I didn't.
  long long platcost = 0;
  if (plats <= 0 || start < 0) return 0;
  while (platcost < plats)
  {
    if (start < 1000) platcost += 100 + (start / 100);
    else if (start < 1250) platcost += 110 + (start / 30);
    else if (start < 1500) platcost += 150 + (start / 30);
    else if (start < 1750) platcost += 200 + (start / 35);
    else if (start < 2000) platcost += 250 + (start / 40);
    else if (start < 3000) platcost += 300 + (start / 15);
    else if (start < 4000) platcost += 500 + (start / 20);
    else if (start < 5000) platcost += 700 + (start / 25);
    else if (start < 6000) platcost += 900 + (start / 20);
    else platcost += (1200 + (start / 20)) > 1500 ? 1500: (1200 + (start / 20));
    start += 5;
  }
  return start-5;
}

int r_new_meta_exp_cost(int start, long long exp)
{
   if (exp <= 0) return start;
   while (exp > 0)
   {
     exp -= new_meta_platinum_cost(start, start+1) * 51523;
     start += 5;
   }
   return start-5;
}

int new_meta_exp_cost_one(int start)
{
   if (start < 0) return 0;
   return new_meta_platinum_cost(start, start+1) * 51523;
}


long long moves_exp_spent(char_data * ch)
{
   int start = GET_MAX_MOVE(ch) - GET_MOVE_METAS(ch);
   long long expcost = 0;
   while (start < GET_MAX_MOVE(ch))
   {
    expcost += (int)((5000000 + (start * 2500))*1.2);
    start++;
   }
   return expcost;
}

long long moves_plats_spent(char_data * ch)
{
  long long expcost = 0;
  int start = GET_MAX_MOVE(ch) - GET_MOVE_METAS(ch);
  while (start < GET_MAX_MOVE(ch))
  {
    expcost += (long long)(((int)(125 + (int)((0.025 * start *(start/1000 == 0 ? 1: start/1000))))*0.9));
    start++;
  }
  return expcost;
}

long long hps_exp_spent(char_data * ch)
{
   long long expcost = 0;
   int cost;
   switch (GET_CLASS(ch))
   {
      case CLASS_BARBARIAN: cost = 2000; break;
      case CLASS_WARRIOR: cost = 2100; break;
      case CLASS_PALADIN: cost = 2200; break;
      case CLASS_MONK: cost = 2300; break;
      case CLASS_RANGER: cost = 2500; break;
      case CLASS_ANTI_PAL: cost = 2500; break;
      case CLASS_THIEF: cost = 2600; break;
      case CLASS_BARD: cost = 2600; break;
      case CLASS_DRUID: cost = 2800; break;
      case CLASS_CLERIC: cost = 2900; break;
      case CLASS_MAGIC_USER: cost = 3000; break;
      default:
        cost = 3000; break;
   }
   int base = GET_MAX_HIT(ch) - GET_HP_METAS(ch);
   while (base < GET_MAX_HIT(ch))
   {
     expcost += (long long)((5000000 + (cost * base))*1.2);
     base++;
   }
   return expcost;
}

long long hps_plats_spent(char_data * ch)
{
   int cost;
   long long platcost = 0;
   switch (GET_CLASS(ch))
   {
      case CLASS_BARBARIAN: cost = 0; break;
      case CLASS_WARRIOR: cost = 10; break;
      case CLASS_PALADIN: cost = 20; break;
      case CLASS_MONK: cost = 30; break;
      case CLASS_RANGER: cost = 50; break;
      case CLASS_ANTI_PAL: cost = 50; break;
      case CLASS_THIEF: cost = 60; break;
      case CLASS_BARD: cost = 60; break;
      case CLASS_DRUID: cost = 80; break;
      case CLASS_CLERIC: cost = 90; break;
      case CLASS_MAGIC_USER: cost = 100; break;
      default:
        cost = 100; break;
   }
   int base = GET_MAX_HIT(ch) - GET_HP_METAS(ch);
   while (base < GET_MAX_HIT(ch))
   {
     platcost += (long long)((100 + cost + (int)(0.025 * base *(base/1000 == 0 ? 1: base/1000))) * 0.9);
     base++;
   }
   return platcost;
}

long long mana_exp_spent(char_data * ch)
{
   int cost;
   long long expcost = 0;
   switch (GET_CLASS(ch))
   {
      case CLASS_PALADIN: cost = 2800; break;
      case CLASS_RANGER: cost = 2500; break;
      case CLASS_ANTI_PAL: cost = 2500; break;
      case CLASS_DRUID: cost = 2200; break;
      case CLASS_CLERIC: cost = 2100; break;
      case CLASS_MAGIC_USER: cost = 2000; break;
      default:
        return 0;
   }
   int base = GET_MAX_MANA(ch) - GET_MANA_METAS(ch);
   while (base < GET_MAX_MANA(ch))
   {
     expcost += (long long)((5000000 + (cost * base))*1.2);
     base++;
   }
   return expcost;
}


long long mana_plats_spent(char_data * ch)
{
   int cost;
   long long platcost = 0;
   switch (GET_CLASS(ch))
   {
      case CLASS_PALADIN: cost = 80; break;
      case CLASS_RANGER: cost = 50; break;
      case CLASS_ANTI_PAL: cost = 50; break;
      case CLASS_DRUID: cost = 20; break;
      case CLASS_CLERIC: cost = 10; break;
      case CLASS_MAGIC_USER: cost = 0; break;
      default:
        return 0;
   }
  int base = GET_MAX_MANA(ch) - GET_MANA_METAS(ch);
  while (base < GET_MAX_MANA(ch))
  {
    platcost += (long long)((100 + cost + (int)(0.025 * base * (base/1000 == 0 ? 1: base/1000)))*0.9);
    base++;
  }
  return platcost;
}


int meta_get_stat_exp_cost(char_data * ch, ubyte stat)
{
    int xp_price;
    int curr_stat;
    switch(stat) {
      case CONSTITUTION:
        curr_stat = ch->raw_con;
        break;
      case STRENGTH:
          curr_stat = ch->raw_str;
        break;
      case DEXTERITY:
        curr_stat = ch->raw_dex;
        break;
      case INTELLIGENCE:
        curr_stat = ch->raw_intel;
        break;
      case WISDOM:
        curr_stat = ch->raw_wis;
        break;
      default:
        xp_price = 9999999;
        break;
    }
    switch(curr_stat)
    {
        case 1:
        case 2:
        case 3:
        case 4:
          xp_price = 2000000; break;
        case 5:
          xp_price = 3000000; break;
        case 6: xp_price = 4000000;break;
        case 7: xp_price = 5000000;break;
        case 8: xp_price = 5500000;break;
        case 9: xp_price = 6000000;break;
        case 10: xp_price = 6500000;break;
        case 11: xp_price = 7000000;break;
        case 12: xp_price = 7500000;break;
        case 13: xp_price = 8000000;break;
        case 14: xp_price = 8500000; break;
        case 15: xp_price = 9000000; break;
        case 16: xp_price = 9500000; break;
        case 29: xp_price = 25000000;break;
        default: xp_price = (curr_stat-7)*1000000; break;
    }

//    if(ch->pcdata->statmetas > 0)
 //     xp_price += ch->pcdata->statmetas * 20000;

    return xp_price;
}

int meta_get_stat_plat_cost(char_data * ch, ubyte targetstat)
{
  int plat_cost;
  int stat;

  switch(targetstat) {
    case CONSTITUTION:
      stat = ch->raw_con;
      break;
    case STRENGTH:
      stat = ch->raw_str;
      break;
    case DEXTERITY:
      stat = ch->raw_dex;
      break;
    case WISDOM:
      stat = ch->raw_wis;
      break;
    case INTELLIGENCE:
      stat = ch->raw_intel;
      break;
    default:
      stat = 99;
      break;
  }

  if (stat < 5) plat_cost = 100;
  else if (stat < 13) plat_cost = 250;
  else if (stat < 28) plat_cost = 250 + ((stat-12) *50);
  else if (stat == 28) plat_cost = 1250;
  else plat_cost = 1500;

  return plat_cost;
}

void meta_list_stats(char_data * ch)
{
    int xp_price, plat_cost, max_stat;

    xp_price = meta_get_stat_exp_cost(ch, STRENGTH);
    plat_cost = meta_get_stat_plat_cost(ch, STRENGTH);
    max_stat = get_max_stat(ch, STRENGTH);
    if(ch->raw_str >= max_stat)
      csendf(ch, "$B$31)$R Str:       Your strength is already %d.\n\r", max_stat);
    else
      csendf(ch, "$B$31)$R Str: %d        Cost: %d exp + %d Platinum coins. \n\r",
              ( ch->raw_str + 1), xp_price, plat_cost);

    xp_price = meta_get_stat_exp_cost(ch, DEXTERITY);
    plat_cost = meta_get_stat_plat_cost(ch, DEXTERITY);
    max_stat = get_max_stat(ch, DEXTERITY);
    if(ch->raw_dex >= max_stat)
      csendf(ch, "$B$32)$R Dex:       Your dexterity is already %d.\n\r", max_stat);
    else
      csendf(ch, "$B$32)$R Dex: %d        Cost: %d exp + %d Platinum coins.\n\r",
              ( ch->raw_dex + 1 ), xp_price, plat_cost);

    xp_price = meta_get_stat_exp_cost(ch, CONSTITUTION);
    plat_cost = meta_get_stat_plat_cost(ch, CONSTITUTION);
    max_stat = get_max_stat(ch, CONSTITUTION);
    if(ch->raw_con >= max_stat)
      csendf(ch, "$B$33)$R Con:       Your constitution is already %d.\n\r", max_stat);
    else
      csendf(ch, "$B$33)$R Con: %d        Cost: %d exp + %d Platinum coins.\n\r",
              ( ch->raw_con + 1 ), xp_price, plat_cost);

    xp_price = meta_get_stat_exp_cost(ch, INTELLIGENCE);
    plat_cost = meta_get_stat_plat_cost(ch, INTELLIGENCE);
    max_stat = get_max_stat(ch, INTELLIGENCE);
    if(ch->raw_intel >= max_stat)
      csendf(ch, "$B$34)$R Int:       Your intelligence is already %d.\n\r", max_stat);
    else
      csendf(ch, "$B$34)$R Int: %d        Cost: %d exp + %d Platinum coins.\n\r",
              ( ch->raw_intel + 1 ), xp_price, plat_cost);

    xp_price = meta_get_stat_exp_cost(ch, WISDOM);
    plat_cost = meta_get_stat_plat_cost(ch, WISDOM);
    max_stat = get_max_stat(ch, WISDOM);
    if(ch->raw_wis >= max_stat)
      csendf(ch, "$B$35)$R Wis:       Your wisdom is already %d.\n\r", max_stat);
    else
      csendf(ch, "$B$35)$R Wis: %d        Cost: %d exp + %d Platinum coins.\n\r",
              ( ch->raw_wis + 1 ), xp_price, plat_cost);

}

int meta_get_moves_exp_cost(char_data * ch)
{
   int meta = GET_MOVE_METAS(ch);
  if (GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch) < 0)
   meta += GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch);
   return new_meta_exp_cost_one(MAX(0,meta));
}

int meta_get_moves_plat_cost(char_data * ch)
{
   int meta = GET_MOVE_METAS(ch);
  if (GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch) < 0)
   meta += GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch);
   return (int)new_meta_platinum_cost(MAX(0,meta), MAX(0,meta)+1);
}

int meta_get_hps_exp_cost(char_data * ch)
{
   int meta = GET_HP_METAS(ch);
   int bonus = 0;

   for(int i = 16; i < GET_RAW_CON(ch); i++)
      bonus += (i * i) / 30;

   meta -= bonus;

   if (GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch) > 0)
      meta -= GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch);

   return new_meta_exp_cost_one(MAX(0,meta));
}

int meta_get_hps_plat_cost(char_data * ch)
{
   int meta = GET_HP_METAS(ch);
   int bonus = 0;

   for(int i = 16; i < GET_RAW_CON(ch); i++)
      bonus += (i * i) / 30;

   meta -= bonus;

   if (GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch) > 0)
      meta -= GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch);

   return (int)new_meta_platinum_cost(MAX(0,meta), MAX(0,meta)+1);
}

int meta_get_mana_exp_cost(char_data * ch)
{
   int meta = GET_MANA_METAS(ch);
   int stat, bonus = 0;

   if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL || GET_CLASS(ch) == CLASS_RANGER)
      stat = GET_RAW_INT(ch);
   else if(GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_PALADIN || GET_CLASS(ch) == CLASS_DRUID)
      stat = GET_RAW_WIS(ch);
   else stat = 0;

   for(int i = 16; i < stat; i++)
      bonus += (i * i) / 30;

   meta -= bonus;

   if (GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch) > 0)
      meta -= GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch);

   return new_meta_exp_cost_one(MAX(0,meta));
}

int meta_get_mana_plat_cost(char_data * ch)
{
   int meta = GET_MANA_METAS(ch);
   int stat, bonus = 0;

   if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL || GET_CLASS(ch) == CLASS_RANGER)
      stat = GET_RAW_INT(ch);
   else if(GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_PALADIN || GET_CLASS(ch) == CLASS_DRUID)
      stat = GET_RAW_WIS(ch);
   else stat = 0;

   for(int i = 16; i < stat; i++)
      bonus += (i * i) / 30;

   meta -= bonus;

   if (GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch) > 0)
      meta -= GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch);

   return (int)new_meta_platinum_cost(MAX(0,meta), MAX(0,meta)+1);
}

int meta_get_ki_exp_cost(char_data * ch)
{
  int cost, stat;
  switch (GET_CLASS(ch))
  {
    case CLASS_MONK:
      cost = 7700;
      stat = GET_RAW_WIS(ch) - 15;
      stat = MAX(0,stat);
      break;
    case CLASS_BARD:
      cost = 7400;
      stat = GET_RAW_INT(ch) - 15;
      stat = MAX(0,stat);
      break;
    default: return 0;
  }
  cost = 10000000 + ((GET_MAX_KI(ch)-stat) * cost);
  return (int)(cost*1.2);
}

int meta_get_ki_plat_cost(char_data * ch)
{
  int cost, stat;
  switch (GET_CLASS(ch))
  {
    case CLASS_MONK:
      cost = 500;
      stat = GET_RAW_WIS(ch) - 15;
      stat = MAX(0,stat);
      break;
    case CLASS_BARD:
      cost = 400;
      stat = GET_RAW_INT(ch) - 15;
      stat = MAX(0,stat);
      break;
    default: return 0;
  }
  cost = 500 + cost + (((GET_MAX_KI(ch)-stat)/2) * ((GET_MAX_KI(ch)-stat)/10));
  return (int)(cost*0.9);
}

int meta_dude(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
          struct char_data *owner)
{
  char argument[256];

  int stat;
  int choice;
  int increase;
  int hit_cost, mana_cost, move_cost, ki_cost, hit_exp, move_exp, mana_exp, ki_exp;
  int statplatprice, max_stat;

  sbyte *pstat;
  int pprice;

  if((cmd != 59) && (cmd != 56))
    return eFAILURE;

  if (IS_AFFECTED(ch, AFF_BLIND))
    return eFAILURE;

  if(IS_NPC(ch))
    return eFAILURE;

  if(GET_LEVEL(ch) < 10) {
    send_to_char("$B$2The Meta-physician tells you, 'You're too low level for$R "
                 "$B$2me to waste my time on you.$R\n\r"
                 "$B$2Prove to me you are gonna stick around first!'$R.", ch);
    return eSUCCESS;
  }

  hit_exp = meta_get_hps_exp_cost(ch);
  move_exp = meta_get_moves_exp_cost(ch);
  mana_exp = meta_get_mana_exp_cost(ch);

  hit_cost = meta_get_hps_plat_cost(ch);
  move_cost = meta_get_moves_plat_cost(ch);
  mana_cost = meta_get_mana_plat_cost(ch);

  if(!IS_MOB(ch)) {
    ki_exp = meta_get_ki_exp_cost(ch);
    ki_cost = meta_get_ki_plat_cost(ch);
  }

  if(cmd == 59) {           /* List */
    send_to_char("$B$2The Meta-physician tells you, 'This is what I can do for you...$R \n\r", ch);

    send_to_char("$BAttribute Meta:$R\r\n",ch);
    meta_list_stats(ch);

    send_to_char("$BStatistic Meta:$R\r\n",ch);
    if (hit_exp && hit_cost)
    csendf(ch, "$B$36)$R Add 5 points to your hit points:   %d experience points and %d"
            " Platinum coins.\n\r", hit_exp, hit_cost);
    else
    csendf(ch, "$B$36)$R Add to your hit points:   You cannot do this.\r\n");

    if (hit_exp && hit_cost)
    csendf(ch, "$B$37)$R Add 1 point to your hit points:   %d experience points and %d"
            " Platinum coins.\n\r", (int)(hit_exp/5*1.1), (int)(hit_cost/5*1.1));
    else
    csendf(ch, "$B$37)$R Add to your hit points:   You cannot do this.\r\n");

    if (mana_exp && mana_cost)
    csendf(ch, "$B$38)$R Add 5 points to your mana points:  %d experience points and %d"
            " Platinum coins.\n\r", mana_exp, mana_cost);
    else
    csendf(ch, "$B$38)$R Add to your mana points:  You cannot do this.\r\n");

    if (mana_exp && mana_cost)
    csendf(ch, "$B$39)$R Add 1 point to your mana points:   %d experience points and %d"
            " Platinum coins.\n\r", (int)(mana_exp/5*1.1), (int)(mana_cost/5*1.1));
    else
    csendf(ch, "$B$39)$R Add to your mana points:   You cannot do this.\r\n");

    if (move_exp && move_cost)
    csendf(ch, "$B$310)$R Add 5 points to your movement points: %d experience points and %d"
            " Platinum coins.\n\r", move_exp, move_cost);
    else
    csendf(ch, "$B$310)$R Add to your movement points:  You cannot do this.\r\n");

    if (move_exp && move_cost)
    csendf(ch, "$B$311)$R Add 1 points to your movement points:   %d experience points and %d"
            " Platinum coins.\n\r", (int)(move_exp/5*1.1), (int)(move_cost/5*1.1));
    else
    csendf(ch, "$B$311)$R Add to your movement points:   You cannot do this.\r\n");

    if(!IS_MOB(ch) && ki_cost && ki_exp) {   // mobs can't meta ki
      csendf(ch, "$B$312)$R Add a point of ki:        %d experience points and %d Platinum.\n\r", ki_exp, ki_cost);
    }
    else if (!IS_MOB(ch))
    csendf(ch, "$B$312)$R Add a point of ki:        You cannot do this.\r\n");

    send_to_char("$BMonetary Exchange:$R\r\n",ch);
    send_to_char(
    "$B$313)$R One (1) Platinum coin     Cost: 20,000 Gold Coins.\n\r"
    "$B$314)$R Five (5) Platinum coins   Cost: 100,000 Gold Coins.\n\r"
    "$B$315)$R 250 Platinum coins        Cost: 5,000,000 Gold Coins.\r\n"
    "$B$316)$R 100,000 Gold Coins        Cost: Five (5) Platinum coins.\r\n"
    "$B$317)$R 5,000,000 Gold Coins      Cost: 250 Platinum coins.\r\n"
    "$BOther Services:$R\r\n"
    "$B$318)$R Convert experience to gold. (100mil Exp. = 500000 Gold.)\r\n"
    "$B$319)$R A deep blue potion of healing. Cost: 25 Platinum coins.\r\n"
    "$B$320)$R Buy a practice session for 25 plats.\r\n"
                 , ch);

    return eSUCCESS;
  }

  if(cmd == 56)   {      /* buy  */
    one_argument(arg, argument);
    if((choice = atoi(argument)) == 0 || choice < 0) {
      send_to_char("The Meta-physician tells you, 'Pick a number.'\n\r", ch);
      return eSUCCESS;
    }
    switch(choice) {
      case 1: stat = ch->raw_str;
             pstat = &(ch->raw_str);
             pprice = meta_get_stat_exp_cost(ch, STRENGTH);
             statplatprice = meta_get_stat_plat_cost(ch, STRENGTH);
             max_stat = get_max_stat(ch, STRENGTH);
             break;
      case 2: stat = ch->raw_dex;
             pstat = &(ch->raw_dex);
             pprice =  meta_get_stat_exp_cost(ch, DEXTERITY);
             statplatprice = meta_get_stat_plat_cost(ch, DEXTERITY);
             max_stat = get_max_stat(ch, DEXTERITY);
             break;
      case 3: stat = ch->raw_con;
             pstat = &(ch->raw_con);
             pprice = meta_get_stat_exp_cost(ch, CONSTITUTION);
             statplatprice = meta_get_stat_plat_cost(ch, CONSTITUTION);
             max_stat = get_max_stat(ch, CONSTITUTION);
             break;
      case 4: stat = ch->raw_intel;
             pstat = &(ch->raw_intel);
             pprice = meta_get_stat_exp_cost(ch, INTELLIGENCE);
             statplatprice = meta_get_stat_plat_cost(ch, INTELLIGENCE);
             max_stat = get_max_stat(ch, INTELLIGENCE);
             break;
      case 5: stat = ch->raw_wis;
             pstat = &(ch->raw_wis);
             pprice = meta_get_stat_exp_cost(ch, WISDOM);
             statplatprice = meta_get_stat_plat_cost(ch, WISDOM);
             max_stat = get_max_stat(ch, WISDOM);
             break;
      default: stat = 0;
    }

    if(choice < 6) {

      if(GET_PLATINUM(ch) < (unsigned)statplatprice) {
        send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services.  SCRAM!'$R\n\r", ch);
        return eSUCCESS;
      }
      if(GET_EXP(ch) < pprice) {
        send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
        return eSUCCESS;
      }
      if(stat >= max_stat) {
        send_to_char("$B$2The Meta-physician tells you, 'You're already as good at that as yer gonna get.'$R\n\r", ch);
        return eSUCCESS;
      }

      GET_EXP(ch) -= pprice;
      GET_PLATINUM(ch) -= statplatprice;

      *pstat += 1;
      ch->pcdata->statmetas++;

      act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
      act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);

      // affect the stat by 0 to reflect the new raw stat
      affect_modify(ch, APPLY_STR, 0, -1, TRUE);
      affect_modify(ch, APPLY_DEX, 0, -1, TRUE);
      affect_modify(ch, APPLY_INT, 0, -1, TRUE);
      affect_modify(ch, APPLY_WIS, 0, -1, TRUE);
      affect_modify(ch, APPLY_CON, 0, -1, TRUE);

      redo_hitpoints(ch);
      redo_mana(ch);
      redo_ki(ch);
      return eSUCCESS;
    }

   if(choice == 6 && hit_exp && hit_cost) {
     if(GET_EXP(ch) < hit_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)hit_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }
     GET_EXP(ch) -= hit_exp;
     GET_PLATINUM(ch) -= hit_cost;

     increase = 5;
     ch->raw_hit += increase;
     GET_HP_METAS(ch) += 5;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_hitpoints(ch);
     return eSUCCESS;
   }

   if(choice == 7 && hit_exp && hit_cost) {
     hit_exp = (int)(hit_exp/5*1.1);
     hit_cost = (int)(hit_cost/5*1.1);

     if(GET_EXP(ch) < hit_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)hit_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!$R'\n\r", ch);
       return eSUCCESS;
     }
     GET_EXP(ch) -= hit_exp;
     GET_PLATINUM(ch) -= hit_cost;

     increase = 1;
     ch->raw_hit += increase;
     GET_HP_METAS(ch) += 1;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_hitpoints(ch);
     return eSUCCESS;
   }

   if(choice == 8 && mana_exp && mana_cost) {

     if(GET_EXP(ch) < mana_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)mana_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch) -= mana_exp;
     GET_PLATINUM(ch) -= mana_cost;

     increase = 5;
     ch->raw_mana += increase;
     GET_MANA_METAS(ch) += 5;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_mana(ch);
     return eSUCCESS;
   }

   if(choice == 9 && mana_exp && mana_cost) {
     mana_exp = (int)(mana_exp/5*1.1);
     mana_cost = (int)(mana_cost/5*1.1);

     if(GET_EXP(ch) < mana_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)mana_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch) -= mana_exp;
     GET_PLATINUM(ch) -= mana_cost;

     increase = 1;
     ch->raw_mana += increase;
     GET_MANA_METAS(ch) += 1;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_mana(ch);
     return eSUCCESS;
   }

   if(choice == 10 && move_exp && move_cost)
   {
     if(GET_EXP(ch) < move_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)move_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch)  -= move_exp;
     GET_PLATINUM(ch) -= move_cost;
     ch->raw_move += 5;
     ch->max_move += 5;
     GET_MOVE_METAS(ch) += 5;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_hitpoints(ch);
     redo_mana(ch);
     return eSUCCESS;
   }

   if(choice == 11 && move_exp && move_cost)
   {
     move_exp = (int)(move_exp/5*1.1);
     move_cost = (int)(move_cost/5*1.1);

     if(GET_EXP(ch) < move_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)move_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch)  -= move_exp;
     GET_PLATINUM(ch) -= move_cost;
     ch->raw_move += 1;
     ch->max_move += 1;
     GET_MOVE_METAS(ch) += 1;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_hitpoints(ch);
     redo_mana(ch);
     return eSUCCESS;
   }
  if(choice == 12 && ki_exp && ki_cost) {
    if(IS_MOB(ch)) {
      send_to_char("Mobs cannot meta ki.\r\n", ch);
      return eSUCCESS;
    }
    if(GET_EXP(ch) < ki_exp) {
      send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
      return eSUCCESS;
    }
    if(GET_PLATINUM(ch) < (uint32)(ki_cost)) {
      send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
      return eSUCCESS;
    }

    GET_EXP(ch) -= ki_exp;
    GET_PLATINUM(ch) -= ki_cost;

    ch->raw_ki += 1;
    GET_KI_METAS(ch) += 1;
    redo_ki(ch);
    act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
    act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
    return eSUCCESS;
  }

   if(choice == 13) {
     if (affected_by_spell(ch, FUCK_GTHIEF))
     {
        send_to_char("$B$2The Meta-physician tells you, 'You cannot do this because of your criminal actions!'$R\r\n",ch);
        return eSUCCESS;
     }
     if(GET_GOLD(ch) < 20000) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }
     GET_GOLD(ch) -= 20000;
     GET_PLATINUM(ch) += 1;
     send_to_char("Ok.\n\r", ch);
     return eSUCCESS;
   }
   if(choice == 14) {
     if (affected_by_spell(ch, FUCK_GTHIEF))
     {
        send_to_char("$B$2The Meta-physician tells you, 'You cannot do this because of your criminal actions!'$R\r\n",ch);
        return eSUCCESS;
     }
     if(GET_GOLD(ch) < 100000) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }

     GET_GOLD(ch) -= 100000;
     GET_PLATINUM(ch) += 5;
     send_to_char("Ok.\n\r", ch);
     return eSUCCESS;
   }
  if(choice == 15) {
    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF))
    {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return eSUCCESS;
    }

    if(GET_GOLD(ch) < 5000000) {
      send_to_char("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R\n\r", ch);
      return eSUCCESS;
    }
    GET_PLATINUM(ch) += 250;
    GET_GOLD(ch) -= 5000000;
    send_to_char("Ok.\n\r", ch);
    return eSUCCESS;
  }
  if(choice == 16) {
    if(GET_PLATINUM(ch) < 5) {
      send_to_char("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R\n\r", ch);
      return eSUCCESS;
    }
    GET_PLATINUM(ch) -= 5;
    GET_GOLD(ch) += 100000;
    send_to_char("Ok.\n\r", ch);
    return eSUCCESS;
  }

  if(choice == 17) {
    if(GET_PLATINUM(ch) < 250) {
      send_to_char("$B$2The Meta-physician tells you, 'You can't afford that!  SCRAM$R\n\r", ch);
      return eSUCCESS;
    }
    GET_PLATINUM(ch) -= 250;
    GET_GOLD(ch) += 5000000;
    send_to_char("Ok.\n\r", ch);
    return eSUCCESS;
  }
  if (choice == 18) {
    if (GET_EXP(ch) < 100000000) {
      send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
      return eSUCCESS;
    }
    if (IS_MOB(ch)) {
       send_to_char ("What would you have to spend gold on chode?\r\n", ch);
       return eSUCCESS;
    }

    GET_EXP(ch) -= 100000000;
    GET_GOLD(ch) += 500000;

    act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
    act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
    return eSUCCESS;
  }
  if (choice == 19)
  {
   if (GET_PLATINUM(ch) < 25)
   {
      send_to_char("$B$2The Meta-physician tells you, 'You can't afford that!'$R\r\n",ch);
      return eSUCCESS;
   }
   struct obj_data *obj = clone_object(real_object(27903));
   if ( IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch) )
    {
        send_to_char( "You can't carry that many items.\n\r", ch );
        extract_obj(obj);
        return eSUCCESS;
    }

    if ( IS_CARRYING_W(ch) + obj->obj_flags.weight > CAN_CARRY_W(ch) )
    {
        send_to_char( "You can't carry that much weight.\n\r", ch );
        extract_obj(obj);
        return eSUCCESS;
   }
   GET_PLATINUM(ch) -= 25;
   obj_to_char(obj,ch);
   send_to_char("$B$2The Meta-physician tells you, 'Here is your potion.'$R\r\n",ch);
   return eSUCCESS;
  }
  if(choice == 20) {
    if (GET_PLATINUM(ch) < 25) {
       send_to_char ("Costs 25 plats...which you don't have.\n\r", ch);
       return eSUCCESS;
    }
    if (IS_MOB(ch)) {
       send_to_char ("You can't buy practices chode...\r\n", ch);
       return eSUCCESS;
    }
    send_to_char("The Meta-Physician gives you a practice session.\n\r", ch);

    GET_PLATINUM(ch) -= 25;
    ch->pcdata->practices += 1;
    return eSUCCESS;
  }
 }
  send_to_char("$B$2The Meta-physician tells you, 'Buy what?!'$R\n\r", ch);
  return eSUCCESS;
}

/*

 END META-PHYSICIAN

*/

/*

 START CARDINAL THELONIUS

*/

void undo_race_saves(char_data * ch)
{
   switch(GET_RACE(ch)) {
     case RACE_HUMAN:
       ch->saves[SAVE_TYPE_FIRE]   -= RACE_HUMAN_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   -= RACE_HUMAN_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] -= RACE_HUMAN_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   -= RACE_HUMAN_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  -= RACE_HUMAN_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] -= RACE_HUMAN_POISON_MOD;
       break;
     case RACE_ELVEN:
       ch->saves[SAVE_TYPE_FIRE]   -= RACE_ELVEN_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   -= RACE_ELVEN_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] -= RACE_ELVEN_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   -= RACE_ELVEN_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  -= RACE_ELVEN_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] -= RACE_ELVEN_POISON_MOD;
       break;
     case RACE_DWARVEN:
       ch->saves[SAVE_TYPE_FIRE]   -= RACE_DWARVEN_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   -= RACE_DWARVEN_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] -= RACE_DWARVEN_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   -= RACE_DWARVEN_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  -= RACE_DWARVEN_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] -= RACE_DWARVEN_POISON_MOD;
       break;
     case RACE_TROLL:
       ch->saves[SAVE_TYPE_FIRE]   -= RACE_TROLL_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   -= RACE_TROLL_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] -= RACE_TROLL_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   -= RACE_TROLL_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  -= RACE_TROLL_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] -= RACE_TROLL_POISON_MOD;
       break;
     case RACE_GIANT:
       ch->saves[SAVE_TYPE_FIRE]   -= RACE_GIANT_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   -= RACE_GIANT_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] -= RACE_GIANT_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   -= RACE_GIANT_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  -= RACE_GIANT_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] -= RACE_GIANT_POISON_MOD;
       break;
     case RACE_PIXIE:
       ch->saves[SAVE_TYPE_FIRE]   -= RACE_PIXIE_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   -= RACE_PIXIE_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] -= RACE_PIXIE_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   -= RACE_PIXIE_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  -= RACE_PIXIE_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] -= RACE_PIXIE_POISON_MOD;
       break;
     case RACE_HOBBIT:
       ch->saves[SAVE_TYPE_FIRE]   -= RACE_HOBBIT_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   -= RACE_HOBBIT_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] -= RACE_HOBBIT_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   -= RACE_HOBBIT_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  -= RACE_HOBBIT_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] -= RACE_HOBBIT_POISON_MOD;
       break;
     case RACE_GNOME:
       ch->saves[SAVE_TYPE_FIRE]   -= RACE_GNOME_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   -= RACE_GNOME_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] -= RACE_GNOME_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   -= RACE_GNOME_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  -= RACE_GNOME_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] -= RACE_GNOME_POISON_MOD;
       break;
     case RACE_ORC:
       ch->saves[SAVE_TYPE_FIRE]   -= RACE_ORC_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   -= RACE_ORC_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] -= RACE_ORC_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   -= RACE_ORC_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  -= RACE_ORC_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] -= RACE_ORC_POISON_MOD;
       break;
     default:
       break;
   }
}


bool is_race_applicable(char_data *ch, int race)
{
  if (GET_CLASS(ch) == CLASS_PALADIN && (race != RACE_HUMAN && race != RACE_ELVEN))
    return FALSE;
  if (GET_CLASS(ch) == CLASS_ANTI_PAL && (race != RACE_HUMAN && race != RACE_ORC))
    return FALSE;
  if (GET_CLASS(ch) == CLASS_BARBARIAN && race == RACE_PIXIE)
    return FALSE;
  if (GET_CLASS(ch) == CLASS_THIEF && race == RACE_GIANT)
    return FALSE;
  switch (race)
  {
      case RACE_ELVEN:
	 if (GET_RAW_DEX(ch) - 2 < 10 || GET_RAW_INT(ch) - 2 < 10)
	   return FALSE;
         break;
      case RACE_DWARVEN:
	 if (GET_RAW_CON(ch) - 2 < 10 || GET_RAW_WIS(ch) - 2 < 10)
           return FALSE;
	  break;
      case RACE_HOBBIT:
	 if (GET_RAW_DEX(ch) - 2 < 10)
           return FALSE;
	  break;
      case RACE_PIXIE:
	 if (GET_RAW_INT(ch) - 2 < 10)
           return FALSE;
	  break;
      case RACE_GIANT:
	 if (GET_RAW_STR(ch) - 2 < 12)
           return FALSE;
	  break;
      case RACE_GNOME:
	 if (GET_RAW_WIS(ch) - 2 < 12)
           return FALSE;
	  break;
      case RACE_ORC:
	 if (GET_RAW_CON(ch) - 2 < 10 || GET_RAW_STR(ch) - 2 < 10)
           return FALSE;
	  break;
      case RACE_TROLL:
	 if (GET_RAW_CON(ch) - 2 < 12)
           return FALSE;
	  break;
      default: break;
  }
  return TRUE;
}

bool would_die(char_data *ch)
{
  if (GET_RAW_STR(ch) < 8 || GET_RAW_CON(ch) < 8 || GET_RAW_WIS(ch) < 8 || GET_RAW_INT(ch) < 8 || GET_RAW_DEX(ch) < 8)
    return TRUE;

  return FALSE;
}

void set_heightweight(char_data *ch)
{
    switch (GET_RACE(ch))
    {
	case RACE_HUMAN:
            ch->height = number(66, 77);
            ch->weight = number(160, 200);
 	    break;		
	case RACE_ELVEN:
            ch->height = number(78, 101);
            ch->weight = number(120, 160);
 	    break;		
	case RACE_DWARVEN:
            ch->height = number(42, 65);
            ch->weight = number(140, 180);
 	    break;		
	case RACE_HOBBIT:
            ch->height = number(20, 41);
            ch->weight = number(40, 80);
 	    break;		
	case RACE_PIXIE:
            ch->height = number(12, 33);
            ch->weight = number(10, 40);
 	    break;		
	case RACE_GIANT:
            ch->height = number(106, 131);
            ch->weight = number(260, 300);
 	    break;		
	case RACE_GNOME:
            ch->height = number(42, 65);
            ch->weight = number(80, 120);
 	    break;		
	case RACE_ORC:
            ch->height = number(78, 101);
            ch->weight = number(200, 240);
 	    break;
	case RACE_TROLL:
            ch->height = number(102, 123);
            ch->weight = number(240, 280);
 	    break;		
    }

}

int changecost(int oldrace, int newrace)
{
  switch (oldrace)
  {
     case RACE_GIANT:
     case RACE_TROLL:
	if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT) return 7000;
	else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME) return 6500;
	else if (newrace == RACE_HUMAN) return 6000;
	else if (newrace == RACE_ELVEN || newrace == RACE_ORC) return 5500;
	else if (newrace == RACE_GIANT || newrace == RACE_TROLL) return 5000;
        break;
    case RACE_HOBBIT:
    case RACE_PIXIE:
	if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT) return 5000;
	else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME) return 5500;
	else if (newrace == RACE_HUMAN) return 6000;
	else if (newrace == RACE_ELVEN || newrace == RACE_ORC) return 6500;
	else if (newrace == RACE_GIANT || newrace == RACE_TROLL) return 7000;
        break;
    case RACE_DWARVEN:
    case RACE_GNOME:
	if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT) return 5500;
	else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME) return 5000;
	else if (newrace == RACE_HUMAN) return 5500;
	else if (newrace == RACE_ELVEN || newrace == RACE_ORC) return 6000;
	else if (newrace == RACE_GIANT || newrace == RACE_TROLL) return 6500;
        break;
    case RACE_ELVEN:
    case RACE_ORC:
	if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT) return 6500;
	else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME) return 6000;
	else if (newrace == RACE_HUMAN) return 5500;
	else if (newrace == RACE_ELVEN || newrace == RACE_ORC) return 5000;
	else if (newrace == RACE_GIANT || newrace == RACE_TROLL) return 5500;
        break;
    case RACE_HUMAN:
	if (newrace == RACE_PIXIE || newrace == RACE_HOBBIT) return 6000;
	else if (newrace == RACE_DWARVEN || newrace == RACE_GNOME) return 5500;
	else if (newrace == RACE_HUMAN) return 5000;
	else if (newrace == RACE_ELVEN || newrace == RACE_ORC) return 5500;
	else if (newrace == RACE_GIANT || newrace == RACE_TROLL) return 6000;
        break;
    default:
       return 1000000;
  }
  return 100000;
}

char *race_message(char_data *ch, int race)
{
  static char buf[MAX_STRING_LENGTH];
  if (GET_RACE(ch) == race)
     return "You are already of this race.";
  else if (!is_race_applicable(ch, race))
     return "You do not qualify for becoming this race.";


  sprintf(buf, "%d platinum coins.", changecost(GET_RACE(ch), race));
  return &buf[0];
}


extern struct race_shit race_info[];

int cardinal(struct char_data *ch, struct obj_data *obj, int cmd, char *argument, struct char_data *owner)
{
  if (cmd == 59) // list
  {

    send_to_char("$B$2Cardinal Thelonius tells you, 'Here's what I can do for you...'$R\r\nEnter \"buy <number>\" to make a selection.\r\n\r\n",ch);
    send_to_char("$BRace Change:$R\r\n(Remember a race change will reduce your base attributes by 2 points each.)\r\n",ch);

    for (int i = 1; i <= MAX_PC_RACE; i++)
      csendf(ch, "$B$3%d)$R  %-24s - %s\r\n", i, race_info[i].singular_name, race_message(ch, i));

    send_to_char("$BOther Services:$R\r\n",ch);

    csendf(ch, "$B$3%d)$R %-24s - 1000 platinum coins.\r\n", MAX_PC_RACE+1,"Sex Change");
    csendf(ch, "$B$3%d)$R %-24s - 50 platinum coins.\r\n", MAX_PC_RACE+2,"A deep red vial of mana");

    send_to_char("$BHeight/Weight Change:$R\r\n",ch);

    if (ch->height < race_info[ch->race].max_height) 
	csendf(ch,"$B$3%d)$R %-24s - 250 platinum coins.\r\n", MAX_PC_RACE+3, "Increase your height by 1");
    else
	csendf(ch,"$B$3%d)$R %-24s.\r\n", MAX_PC_RACE+3, "You cannot increase your height further");

    if (ch->height > race_info[ch->race].min_height)
	csendf(ch,"$B$3%d)$R %-24s - 250 platinum coins.\r\n", MAX_PC_RACE+4, "Decrease your height by 1");
    else
	csendf(ch,"$B$3%d)$R %-24s.\r\n", MAX_PC_RACE+4, "You cannot decrease your height further");

    if (ch->weight < race_info[ch->race].max_weight) 
	csendf(ch,"$B$3%d)$R %-24s - 250 platinum coins.\r\n", MAX_PC_RACE+5, "Increase your weight by 1");
    else
	csendf(ch,"$B$3%d)$R %-24s.\r\n", MAX_PC_RACE+5, "You cannot increase your weight further");

    if (ch->weight > race_info[ch->race].min_weight)
	csendf(ch,"$B$3%d)$R %-24s - 250 platinum coins.\r\n", MAX_PC_RACE+6, "Decrease your weight by 1");
    else
	csendf(ch,"$B$3%d)$R %-24s.\r\n", MAX_PC_RACE+6, "You cannot decrease your weight further");

    return eSUCCESS;
  } 
  else if (cmd == 56) // buy
  {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    argument = one_argument(argument,arg);
    argument = one_argument(argument,arg2);
    int choice = atoi(arg);
    if (choice > 0 && choice <= MAX_PC_RACE)
    {
      if (would_die(ch))
      {
        send_to_char("$B$2Cardinal Thelonius tells you, 'The process would kill you!'$R\r\n",ch);
	return eSUCCESS;
      }
      if (GET_RACE(ch) == choice)
      {
        send_to_char("$B$2Cardinal Thelonius tells you, 'You are already a member of that race!'$R\r\n",ch);
	return eSUCCESS;
      }
      if (!is_race_applicable(ch, choice))
      {
        send_to_char("$B$2Cardinal Thelonius tells you, 'You do not qualify for becoming that race!'$R\r\n",ch);
	return eSUCCESS;
      }
      if (GET_PLATINUM(ch) < (unsigned)changecost(GET_RACE(ch), choice))
      {
        send_to_char("$B$2Cardinal Thelonius tells you, 'You can't afford that!'$R\r\n",ch);
	return eSUCCESS;
      } 

      if (!str_cmp(arg2, "confirm"))
      {
	GET_PLATINUM(ch) -= changecost(GET_RACE(ch), choice);
	undo_race_saves(ch);

        GET_RACE(ch) = choice;

	GET_RAW_STR(ch) = MIN(get_max_stat(ch, STRENGTH), GET_RAW_STR(ch));
	GET_RAW_STR(ch) -= 2;
        GET_STR(ch) = GET_RAW_STR(ch) + GET_STR_BONUS(ch);

	GET_RAW_CON(ch) = MIN(get_max_stat(ch, CONSTITUTION), GET_RAW_CON(ch));
	GET_RAW_CON(ch) -= 2;
        GET_CON(ch) = GET_RAW_CON(ch) + GET_CON_BONUS(ch);

	GET_RAW_DEX(ch) = MIN(get_max_stat(ch, DEXTERITY), GET_RAW_DEX(ch));
	GET_RAW_DEX(ch) -= 2;
        GET_DEX(ch) = GET_RAW_DEX(ch) + GET_DEX_BONUS(ch);

	GET_RAW_WIS(ch) = MIN(get_max_stat(ch, WISDOM), GET_RAW_WIS(ch));
	GET_RAW_WIS(ch) -= 2;
        GET_WIS(ch) = GET_RAW_WIS(ch) + GET_WIS_BONUS(ch);

	GET_RAW_INT(ch) = MIN(get_max_stat(ch, INTELLIGENCE), GET_RAW_INT(ch));
	GET_RAW_INT(ch) -= 2;
        GET_INT(ch) = GET_RAW_INT(ch) + GET_INT_BONUS(ch);

        redo_hitpoints (ch);
        redo_mana (ch);
        redo_ki(ch);

	extern void do_inate_race_abilities(char_data * ch);
	do_inate_race_abilities(ch);
	extern void verify_max_stats(char_data *ch);
        verify_max_stats(ch);
	//set_heightweight(ch);
	extern void check_hw(char_data *ch);
	check_hw(ch);

	extern int recheck_height_wears(char_data * ch);
        recheck_height_wears(ch);

        send_to_char("The Cardinal prays loudly and summons the magic of the gods...\r\n",ch);
        send_to_char("After a brief moment of pain you are reborn!\r\n",ch);
      } else {
	csendf(ch, "$BYou must enter 'buy %d CONFIRM' if you are positive you wish to make this change!\r\n",choice);
        send_to_char("$4NOTE$R$B: Your attributes will be adjusted to fit this new race and then lowered by 2 points each.$R\r\n",ch);
      }
      return eSUCCESS;
    } else if (choice == MAX_PC_RACE+1) {
      int newsex;
      if (arg2[0] == 'n') newsex = 0;
      else if (arg2[0] == 'm') newsex = 1;
      else if (arg2[0] == 'f') newsex = 2;
      else {
	csendf(ch, "Syntax: buy %d m/f/n\r\n",MAX_PC_RACE+1);
	return eSUCCESS;
      }
      if (GET_SEX(ch) == newsex)
      {
        send_to_char("$B$2Cardinal Thelonius tells you, 'That wouldn't change much'$R\r\n",ch);
	return eSUCCESS;
      }
      if (GET_PLATINUM(ch) < 1000)
      {
        send_to_char("$B$2Cardinal Thelonius tells you, 'You can't afford that!'$R\r\n",ch);
	return eSUCCESS;
      }
      GET_PLATINUM(ch) -= 1000;
      GET_SEX(ch) = newsex;
      send_to_char("The Cardinal prays loudly and summons the magic of the gods...\r\n",ch);
      send_to_char("After a brief moment of pain you are reborn!\r\n",ch);
      return eSUCCESS;
    } else if (choice == MAX_PC_RACE+2) {
      if (GET_PLATINUM(ch) < 50)
      {
        send_to_char("$B$2Cardinal Thelonius tells you, 'You can't afford that!'$R\r\n",ch);
        return eSUCCESS;
      }
      struct obj_data *obj = clone_object(real_object(27904));
      if ( IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch) )
      {
        send_to_char( "You can't carry that many items.\n\r", ch );
        extract_obj(obj);
        return eSUCCESS;
      }

      if ( IS_CARRYING_W(ch) + obj->obj_flags.weight > CAN_CARRY_W(ch) )
      {
        send_to_char( "You can't carry that much weight.\n\r", ch );
        extract_obj(obj);
        return eSUCCESS;
      }
      GET_PLATINUM(ch) -= 50;
      obj_to_char(obj,ch);
      send_to_char("$B$2Cardinal Thelonius tells you, 'Here is your potion.'$R\r\n",ch);
      return eSUCCESS;      
    } else if (choice >= MAX_PC_RACE+3 && choice <= MAX_PC_RACE+6) {
      choice -= MAX_PC_RACE;

      if (choice == 3 && ch->height >= race_info[ch->race].max_height)
      { send_to_char("You cannot increase your height any more.\r\n",ch); return eSUCCESS;}    
      else if (choice == 4 && ch->height <= race_info[ch->race].max_height)
      { send_to_char("You cannot decrease your height any more.\r\n",ch); return eSUCCESS;} 
      else if (choice == 5 && ch->weight >= race_info[ch->race].max_weight)
      { send_to_char("You cannot increase your weight any more.\r\n",ch); return eSUCCESS;}  
      else if (choice == 6 && ch->weight <= race_info[ch->race].max_weight)
      { send_to_char("You cannot decrease your weight any more.\r\n",ch); return eSUCCESS;}
      
      if (GET_PLATINUM(ch) < 250)
      {
	send_to_char("You cannot afford it.\r\n",ch);
	return eSUCCESS;
      }
      GET_PLATINUM(ch) -= 250;
      send_to_char("Cardinal Thelonius gropes you.\r\n",ch);
      if (choice == 3) ch->height++;
      if (choice == 4) ch->height--;
      if (choice == 5) ch->weight++;
      if (choice == 6) ch->weight--;
      return eSUCCESS;
    } else {
      send_to_char("$B$2Cardinal Thelonius tells you, 'I don't have that. Try \"list\".'$R\n\r", ch);
      return eSUCCESS;
    }
  }

  return eFAILURE;
}


/*

 END CARDINAL THELONIUS

*/
