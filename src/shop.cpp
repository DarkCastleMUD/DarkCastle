/***************************************************************************
 *  file: shop.c , Shop module.                            Part of DIKUMUD *
 *  Usage: Procedures handling shops and shopkeepers.                      *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec\@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: shop.cpp,v 1.33 2014/07/04 22:00:04 jhhudso Exp $ */

#include <cstdio>
#include <cstring>
#include <fmt/format.h>

#include "DC/levels.h"
#include "DC/obj.h"
#include "DC/affect.h"
#include "DC/DC.h"

#include "DC/interp.h"
#include "DC/player.h"
#include "DC/handler.h"

#include "DC/db.h"
#include "DC/act.h"
#include "DC/returnvals.h"
#include "DC/shop.h"
#include "DC/inventory.h"
#include "DC/const.h"
#include "DC/wizard.h"

player_shop *g_playershops;

extern time_info_data time_info;

qint32 max_shop;

// extern function
qint32 fwrite_string(const QString buf, FILE *fl);

QMap<QString, reroll_t> reroll_sessions = {};

/*
 * See if a shop keeper wants to trade.
 */
bool is_ok(CharacterPtr keeper, CharacterPtr ch, qint32 shop_nr)
{
  QString buf;

  // Undesirables.
  // TODO - Figure out if KILLER does anything we want to kill...
  // If not, let's use the AFF bit for something useful....
  if (ISSET(ch->affected_by, AFF_KILLER))
  {
    do_say(keeper, "Go away before I call the guards!!");
    dc_sprintf(buf, "%s the KILLER is over here!\r\n", qPrintable(ch->shortdesc_or_name()));
    do_shout(keeper, buf);
    return false;
  }

  /*
   * Invisible people.
   */
  if (!CAN_SEE(keeper, ch))
  {
    do_say(keeper, "I don't trade with someone I can't see!");
    return false;
  }

  /*
   * Shop hours.
   */
  if (time_info.hours < DC::getInstance()->shop_index[shop_nr].open1)
  {
    do_say(keeper, "Come back later!");
    return false;
  }

  else if (time_info.hours <= DC::getInstance()->shop_index[shop_nr].close1)
  {
    return true;
  }
  else if (time_info.hours < DC::getInstance()->shop_index[shop_nr].open2)
  {
    do_say(keeper, "Come back later!");
    return false;
  }
  else if (time_info.hours <= DC::getInstance()->shop_index[shop_nr].close2)
  {
    return true;
  }
  else
  {
    do_say(keeper, "Sorry, come back tomorrow.");
    return false;
  }

  return true;
}

/*
 * See if a shop will buy an item.
 */
qint32 trade_with(ObjectPtr item, qint32 shop_nr)
{
  qint32 counter;

  if (item->obj_flags.cost < 1)
    return false;

  for (const auto &type : DC::getInstance()->shop_index[shop_nr].type)
  {
    if (GET_ITEM_TYPE(item) == type)
      return true;
  }

  return false;
}

qint32 unlimited_supply(ObjectPtr item, qint32 shop_nr)
{
  ObjectPtr obj;

  for (obj = DC::getInstance()->shop_index[shop_nr].inventory; obj; obj = obj->next_content)
  {
    if (item->item_number == obj->item_number)
      return true;
  }

  return false;
}

void restock_keeper(CharacterPtr keeper, qint32 shop_nr)
{
  ObjectPtr obj, obj2;
  QString buf;

  dc_sprintf(buf, "Restocking shop keeper: %d", shop_nr);
  DC::getInstance()->logentry(buf, OVERSEER, DC::LogChannel::LOG_MISC);

  for (obj = DC::getInstance()->shop_index[shop_nr].inventory; obj; obj = obj->next_content)
  {
    obj2 = clone_object(obj->item_number);
    obj_to_char(obj2, keeper);
  }
}

/*
 * Buy an item from a shop.
 */
void shopping_buy(const QString arg, CharacterPtr ch,
                  CharacterPtr keeper, qint32 shop_nr)
{
  QString buf;
  QString argm;
  ObjectPtr obj;
  quint32 cost;

  if (!is_ok(keeper, ch, shop_nr))
    return;

  one_argument(arg, argm);
  if (*argm == '\0')
  {
    dc_sprintf(buf, "%s What do you want to buy?", qPrintable(ch->name()));
    keeper->do_tell(QString(buf).split(' '));
    return;
  }

  if (!ch->isNonPlayer() && ch->isPlayerGoldThief())
  {
    ch->sendln("Your criminal acts prohibit it.");
    return;
  }

  auto &shop = DC::getInstance()->shop_index[shop_nr];
  if ((obj = get_obj_in_list_vis(ch, argm, keeper->carrying)) == nullptr)
  {
    keeper->do_tell(shop.no_such_item1.arg(ch->name()).split(' '));
    return;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
  {
    ch->sendln("The shop keeper changes his mind and refuses to sell such a special item.");
    return;
  }

  if (obj->obj_flags.cost <= 0)
  {
    extract_obj(obj);
    keeper->do_tell(shop.no_such_item1.arg(qPrintable(ch->name())).split(' '));
    return;
  }

  cost = (qint32)(obj->obj_flags.cost * shop.profit_buy);

  if (cost < 1)
    cost = 1;

  if (ch->getGold() < cost)
  {
    keeper->do_tell(shop.missing_cash2.arg(qPrintable(ch->name())).split(' '));
    return;
  }

  if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
  {
    ch->sendln("You can't carry that many items.");
    return;
  }

  if (IS_CARRYING_W(ch) + obj->obj_flags.weight > CAN_CARRY_W(ch))
  {
    ch->sendln("You can't carry that much weight.");
    return;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE))
  {
    if (search_char_for_item(ch, obj->item_number, false))
    {
      ch->sendln("The item's uniqueness prevents it!");
      return;
    }
  }

  act("$n buys $p.", ch, obj, 0, TO_ROOM, 0);
  keeper->do_tell(shop.message_buy.arg(qPrintable(ch->name())).arg(QString::number(cost)).split(' '));
  ch->send(QStringLiteral("You now have %1.\r\n").arg(obj->short_description()));
  ch->removeGold(cost);
  keeper->addGold(cost);

  if (keeper->getGold() > 3000000)
    keeper->setGold(3000000);

  // Wormhole to map_eq_level
  /*
  if( obj->obj_flags.eq_level == 1000 )
       obj = clone_object(obj->item_number);
  else
      obj_from_char( obj );
  */

  if (unlimited_supply(obj, shop_nr))
    obj = clone_object(obj->item_number);
  else
    obj_from_char(obj);

  obj_to_char(obj, ch);
  ch->save(cmd_t::SAVE_SILENTLY);
}

/*
 * Sell an item to a shop keeper.
 */
void shopping_sell(const QString arg, CharacterPtr ch,
                   CharacterPtr keeper, qint32 shop_nr)
{
  QString buf;
  QString argm;
  ObjectPtr obj;
  quint32 cost;

  if (!is_ok(keeper, ch, shop_nr))
    return;

  one_argument(arg, argm);

  if (*argm == '\0')
  {
    keeper->do_tell(QStringLiteral("%1 What do you want to sell?").arg(qPrintable(ch->name())).split(' '));
    return;
  }

  if (!ch->isNonPlayer() && ch->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
  {
    ch->sendln("Your criminal acts prohibit it.");
    return;
  }

  if ((obj = get_obj_in_list_vis(ch, argm, ch->carrying)) == nullptr)
  {
    keeper->do_tell(DC::getInstance()->shop_index[shop_nr].no_such_item2.arg(qPrintable(ch->name())).split(' '));
    return;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE))
  {
    ch->sendln("It seems magically attached to you.");
    return;
  }

  if (contains_no_trade_item(obj))
  {
    if (ch->getLevel() > IMMORTAL)
      ch->sendln("That was a NO_TRADE item btw....");
    else
    {
      ch->sendln("Something inside it seems magically attached to you.");
      return;
    }
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
  {
    ch->sendln("That would be really fucking smart.");
    return;
  }

  if (!trade_with(obj, shop_nr) || obj->obj_flags.cost < 1)
  {
    keeper->do_tell(DC::getInstance()->shop_index[shop_nr].do_not_buy.arg(qPrintable(ch->name())).split(' '));
    return;
  }

  qint32 virt = DC::getInstance()->obj_index[obj->item_number].vnum();
  if (virt >= 13400 && virt <= 13707 &&
      DC::getInstance()->mob_index[keeper->mobdata->nr].vnum() != 13416)
  {
    keeper->do_tell(QStringLiteral("%1 There is only one merchant in the land that deals with such fine jewels.").arg(qPrintable(ch->name())).split(' '));
    return;
  }

  // don't allow non-empty containers to be sold
  if (obj->obj_flags.type_flag == ITEM_CONTAINER && obj->contains)
  {
    keeper->do_tell(QStringLiteral("%1 %2$B$2 needs to be emptied first.").arg(qPrintable(ch->name())).arg(GET_OBJ_SHORT(obj)).split(' '));
    return;
  }

  cost = (qint32)(obj->obj_flags.cost * DC::getInstance()->shop_index[shop_nr].profit_sell);
  if (keeper->getGold() < cost)
  {
    keeper->do_tell(DC::getInstance()->shop_index[shop_nr].missing_cash1.arg(qPrintable(ch->name())).split(' '));
    return;
  }

  act("$n sells $p.", ch, obj, 0, TO_ROOM, 0);
  keeper->do_tell(DC::getInstance()->shop_index[shop_nr].message_sell.arg(qPrintable(ch->name())).arg(QString::number(cost)).split(' '));
  ch->send(QStringLiteral("The shopkeeper now has %1.\r\n").arg(obj->short_description()));
  ch->addGold(cost);
  keeper->removeGold(cost);

  strcpy(argm, qPrintable(obj->name()));

  if (get_obj_in_list(argm, keeper->carrying) || GET_ITEM_TYPE(obj) == ITEM_TRASH || unlimited_supply(obj, shop_nr))
  {
    extract_obj(obj);
  }
  else
    move_obj(obj, keeper);

  ch->save(cmd_t::SAVE_SILENTLY);
}

/*
 * Value an item.
 */
void shopping_value(const QString arg, CharacterPtr ch,
                    CharacterPtr keeper, qint32 shop_nr)
{
  QString buf, buf2[MAX_STRING_LENGTH];
  QString argm;
  ObjectPtr obj;
  qint32 cost;
  bool keeperhas = false;

  if (!is_ok(keeper, ch, shop_nr))
    return;

  one_argument(arg, argm);

  if (*argm == '\0')
  {
    keeper->do_tell(QStringLiteral("%1 What do you want to value?").arg(qPrintable(ch->name())).split(' '));
    return;
  }

  if ((obj = get_obj_in_list_vis(ch, argm, ch->carrying)) == nullptr)
  {
    if ((obj = get_obj_in_list_vis(keeper, argm, keeper->carrying)) != nullptr)
      keeperhas = true;
    else
    {
      keeper->do_tell(DC::getInstance()->shop_index[shop_nr].no_such_item2.arg(qPrintable(ch->name())).split(' '));
      return;
    }
  }

  if (DC::getInstance()->mob_index[keeper->mobdata->nr].vnum() == 3003)
  { // if the weaponsmith in town
    if (keeperhas)
    {
      act("The Weaponsmith holds up his $p for you to examine.", ch, obj, 0, TO_CHAR, 0);
      act("The Weaponsmith holds up his $p for $n to examine.", ch, obj, 0, TO_ROOM, 0);
    }
    else
    {
      act("You hold up $p for the Weaponsmith to examine.", ch, obj, 0, TO_CHAR, 0);
      act("$n holds up $p for the Weaponsmith to examine.", ch, obj, 0, TO_ROOM, 0);
      do_emote(keeper, QStringLiteral("looks carefully at the item."));
    }
    if (GET_ITEM_TYPE(obj) == ITEM_WEAPON)
    {
      if (obj->obj_flags.eq_level < 20)
      {
        dc_sprintf(buf, "Well, %s is able to be used by ", qPrintable(obj->short_description()));
        sprintbit(obj->obj_flags.size, Object::size_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        dc_sprintf(buf, "and it can be wielded by these classes: ");
        sprintbit(obj->obj_flags.extra_flags, Object::extra_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        dc_sprintf(buf, "The minimum level necessary to use it is %llu.", obj->obj_flags.eq_level);
        do_say(keeper, buf);
        dc_sprintf(buf, "The damage dice are '%dD%d'", obj->obj_flags.value[1], obj->obj_flags.value[2]);
        do_say(keeper, buf);
        for (qint32 i = {}; i < obj->num_affects; i++)
        {
          if (obj->affected[i].location == APPLY_HITROLL && obj->affected[i].modifier != 0)
          {
            dc_sprintf(buf, "It increases your hit roll by %d.", obj->affected[i].modifier);
            do_say(keeper, buf);
          }
          if (obj->affected[i].location == APPLY_DAMROLL && obj->affected[i].modifier != 0)
          {
            dc_sprintf(buf, "It increases your damage by %d.", obj->affected[i].modifier);
            do_say(keeper, buf);
          }
        }
      }
      else
        do_say(keeper, "This weapon is unknown to me.");
    }
    else
      do_say(keeper, "I'm a weapons expert, that is all.");
  }
  if (DC::getInstance()->mob_index[keeper->mobdata->nr].vnum() == 3004)
  { // if the armourer in town
    if (keeperhas)
    {
      act("The Armourer holds up his $p for you to examine.", ch, obj, 0, TO_CHAR, 0);
      act("The Armourer holds up his $p for $n to examine.", ch, obj, 0, TO_ROOM, 0);
    }
    else
    {
      act("You hold up $p for the Armourer to examine.", ch, obj, 0, TO_CHAR, 0);
      act("$n holds up $p to the Armourer to examine.", ch, obj, 0, TO_ROOM, 0);
      do_emote(keeper, QStringLiteral("looks carefully at the item."));
    }
    if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
    {
      if (obj->obj_flags.eq_level < 20)
      {
        dc_sprintf(buf, "Ah yes, %s can be worn by ", qPrintable(obj->short_description()));
        sprintbit(obj->obj_flags.size, Object::size_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        dc_sprintf(buf, "and it can be worn by these classes: ");
        sprintbit(obj->obj_flags.extra_flags, Object::extra_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        dc_sprintf(buf, "The minimum level necessary to use it is %llu.", obj->obj_flags.eq_level);
        do_say(keeper, buf);
        for (qint32 i = {}; i < obj->num_affects; i++)
        {
          if (obj->affected[i].location == APPLY_AC && obj->affected[i].modifier != 0)
          {
            dc_sprintf(buf, "Your armor class will change by %d.", obj->affected[i].modifier);
            do_say(keeper, buf);
            if (obj->affected[i].modifier < 0)
              do_say(keeper, "Don't worry, this is a good thing.");
          }
        }
      }
      else
        do_say(keeper, "This armor is crafted using too advanced techniques for me.");
    }
    else
      do_say(keeper, "I deal with armor exclusively.");
  }
  if (DC::getInstance()->mob_index[keeper->mobdata->nr].vnum() == 3000)
  { // if the wizard in town
    if (keeperhas)
    {
      act("The Wizard holds up $p for you to examine.", ch, obj, 0, TO_CHAR, 0);
      act("The Wizard holds up $p for $n to examine.", ch, obj, 0, TO_ROOM, 0);
    }
    else
    {
      act("You hold up $p for the Wizard to examine.", ch, obj, 0, TO_CHAR, 0);
      act("$n holds up $p for the Wizard to examine.", ch, obj, 0, TO_ROOM, 0);
      do_emote(keeper, "looks carefully at the item.");
    }
    if (GET_ITEM_TYPE(obj) == ITEM_SCROLL || GET_ITEM_TYPE(obj) == ITEM_WAND || GET_ITEM_TYPE(obj) == ITEM_POTION || GET_ITEM_TYPE(obj) == ITEM_STAFF)
    {
      if (obj->obj_flags.value[0] < 20)
      {
        dc_sprintf(buf, "Excellent, %s has been imbued with energies of the %dth level.", qPrintable(obj->short_description()), obj->obj_flags.value[0]);
        do_say(keeper, buf);
        if (GET_ITEM_TYPE(obj) == ITEM_WAND || GET_ITEM_TYPE(obj) == ITEM_STAFF)
        {
          if (obj->obj_flags.value[3] >= 1)
          {
            dc_sprintf(buf, "It is eminating the aura of ");
            sprinttype(obj->obj_flags.value[3] - 1, spells, buf2);
            strcat(buf, buf2);
            do_say(keeper, buf);
          }
          if (obj->obj_flags.value[1] == obj->obj_flags.value[2])
            do_say(keeper, "It's fully charged as well.");
          else if (obj->obj_flags.value[2] == 0)
            do_say(keeper, "Though unfortunately, there are no more charges left.");
          else
            do_say(keeper, "It looks like it has been used some.");
        }
        else
        {
          if (obj->obj_flags.value[1] >= 1)
          {
            dc_sprintf(buf, "I can easily identify the signatures of ");
            sprinttype(obj->obj_flags.value[1] - 1, spells, buf2);
            strcat(buf, buf2);
            do_say(keeper, buf);
          }
          if (obj->obj_flags.value[2] >= 1)
          {
            do_say(keeper, "There are more enchantments held within, but I'm rather busy.");
          }
        }
      }
      else
        do_say(keeper, "This item contains magics too powerful for me to discern.");
    }
    else
      do_say(keeper, "I only know the properties of scrolls, potions, staves, and wands.");
  }

  if (DC::getInstance()->mob_index[keeper->mobdata->nr].vnum() == 3010 && keeperhas)
  { // if the leather worker in town
    act("The Leather Worker holds up $p for you to examine.", ch, obj, 0, TO_CHAR, 0);
    act("The Leather Worker holds up $p for $n to examine.", ch, obj, 0, TO_ROOM, 0);
    if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
    {
      if (obj->obj_flags.eq_level < 20)
      {
        dc_sprintf(buf, "Ah yes, %s can be worn by ", qPrintable(obj->short_description()));
        sprintbit(obj->obj_flags.size, Object::size_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        dc_sprintf(buf, "and it can be worn by these classes: ");
        sprintbit(obj->obj_flags.extra_flags, Object::extra_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        dc_sprintf(buf, "The minimum level necessary to use it is %llu.", obj->obj_flags.eq_level);
        do_say(keeper, buf);
        for (qint32 i = {}; i < obj->num_affects; i++)
        {
          if (obj->affected[i].location == APPLY_AC && obj->affected[i].modifier != 0)
          {
            dc_sprintf(buf, "Your armor class will change by %d.", obj->affected[i].modifier);
            do_say(keeper, buf);
            if (obj->affected[i].modifier < 0)
              do_say(keeper, "Don't worry, this is a good thing.");
          }
        }
      }
      else
        do_say(keeper, "This armor is crafted using too advanced techniques for me.");
    }
    else
      do_say(keeper, "I don't know anything about this item, actually.");
  }

  if (!trade_with(obj, shop_nr) || obj->obj_flags.cost < 1)
  {
    keeper->do_tell(DC::getInstance()->shop_index[shop_nr].do_not_buy.arg(qPrintable(ch->name())).split(' '));
    return;
  }
  if (!keeperhas)
  {
    cost = (qint32)(obj->obj_flags.cost * DC::getInstance()->shop_index[shop_nr].profit_sell);
    keeper->do_tell(QStringLiteral("%1 I'll give you %2 gold coins for that.").arg(qPrintable(ch->name())).arg(QString::number(cost)).split(' '));
  }
}

/*
 * List available items.
 */
void shopping_list(const QString arg, CharacterPtr ch,
                   CharacterPtr keeper, qint32 shop_nr)
{
  QString buf;
  ObjectPtr obj, tobj;
  qint32 cost;
  //    extern QStringList drinks;
  qint32 found;
  qint32 done[100]; // To show 'em numbered instead of a long list of duplicates
  qint32 i, a;
  if (!is_ok(keeper, ch, shop_nr))
    return;

  if (!keeper->carrying && DC::getInstance()->shop_index[shop_nr].inventory)
  {
    keeper->do_tell(QStringLiteral("%1 Oops, I seem to be out of inventory.").arg(qPrintable(ch->name())).split(' '));
    keeper->do_tell(QStringLiteral("%1 One minute while I restock.").arg(qPrintable(ch->name())).split(' '));
    restock_keeper(keeper, shop_nr);
  }
  i = {};
  ch->sendln("[Amt] [ Price ] [ VNUM ] Item");
  found = false;
  vnum_t first_vnum = {};
  for (obj = keeper->carrying; obj; obj = obj->next_content)
  {
    if (!CAN_SEE_OBJ(ch, obj) || obj->obj_flags.cost <= 0)
      continue;

    found = true;

    cost = (qint32)(obj->obj_flags.cost * DC::getInstance()->shop_index[shop_nr].profit_buy);

    qint32 vnum = DC::getInstance()->obj_index[obj->item_number].vnum();
    bool loop = false;
    for (a = {}; a < i; a++)
      if (done[a] == vnum)
        loop = true;
    if (loop)
      continue;
    if (i < 100)
      done[i++] = DC::getInstance()->obj_index[obj->item_number].vnum();
    else
      break;
    a = {};
    for (tobj = keeper->carrying; tobj; tobj = tobj->next_content)
      if (DC::getInstance()->obj_index[tobj->item_number].vnum() == DC::getInstance()->obj_index[obj->item_number].vnum())
        a++;
    /*        if ( GET_ITEM_TYPE(obj) == ITEM_DRINKCON && obj->obj_flags.value[1] )
            {
                dc_sprintf( buf, "[%3d] [%7d] %s of %s.\r\n",
                    a, cost, qPrintable(obj->short_description()),
                    drinks[obj->obj_flags.value[2]] );
            }
            else
            {*/

    //        }
    if (!first_vnum || first_vnum == DC::INVALID_VNUM)
    {
      first_vnum = DC::getInstance()->getObjectVNUM(obj);
    }
    ch->sendln(QStringLiteral("[%1] [%2] [%3] %4.").arg(QString::number(a), 3).arg(QString::number(cost), 7).arg(QString::number(DC::getInstance()->getObjectVNUM(obj)), 6).arg(obj->short_description()));
  }
  if (first_vnum && first_vnum != DC::INVALID_VNUM)
  {
    ch->sendln(QStringLiteral("Type 'identify vVNUM' for details about a specific object. Example: identify v%1").arg(QString::number(first_vnum)));
  }

  if (!found)
    ch->sendln("You can't buy anything here!");
}

// Spec proc for shop keepers.
// TODO - Remove goto's from this....I hate goto's.  This is C, not BASIC....
qint32 shop_keeper(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr invoker)
{
  CharacterPtr keeper;
  qint32 shop_nr;

  /*
   * Find a shop keeper in the room.
   */
  //    for ( keeper = DC::getInstance()->world[ch->in_room].people;
  //        keeper != nullptr;
  //        keeper = keeper->next_in_room )
  //    {
  //        if ( keeper->isNonPlayer() && DC::getInstance()->mob_index[keeper->mobdata->nr].non_combat_func == shop_keeper )
  //            goto LFound1;
  //    }

  // This (using original log from above stuff) should take care of finding the keeper
  // instead of looping through.  Should allow for multiple keepers too:)
  if (!(keeper = invoker))
  {
    DC::getInstance()->logentry(QStringLiteral("Shop_keeper: keeper not found."), ANGEL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  }

  // LFound1:
  for (shop_nr = {}; shop_nr < max_shop; shop_nr++)
  {
    if (DC::getInstance()->shop_index[shop_nr].keeper == keeper->mobdata->nr)
      goto LFound2;
  }
  DC::getInstance()->logentry(QStringLiteral("Shop_keeper: shop_nr not found."), ANGEL, DC::LogChannel::LOG_BUG);
  return ReturnValue::eFAILURE;

LFound2:
  //    if ( ch->in_room != DC::getInstance()->shop_index[shop_nr].in_room )
  //      return ReturnValue::eFAILURE;

  switch (cmd)
  {
  default:
    return ReturnValue::eFAILURE;
  case cmd_t::BUY:
    shopping_buy(arg, ch, keeper, shop_nr);
    break;
  case cmd_t::SELL:
    shopping_sell(arg, ch, keeper, shop_nr);
    break;
  case cmd_t::VALUE:
    shopping_value(arg, ch, keeper, shop_nr);
    break;
  case cmd_t::LIST:
    shopping_list(arg, ch, keeper, shop_nr);
    break;
  }

  return ReturnValue::eSUCCESS;
}

void boot_the_shops()
{
  QString buf;
  qint32 temp;
  qint32 count;
  FILE *fp;

  if ((fp = fopen(SHOP_FILE, "r")) == nullptr)
  {
    perror(SHOP_FILE);
    exit(1);
  }

  max_shop = {};

  for (;;)
  {
    buf = fread_string(fp, 0);
    if (*buf == '$')
    {
      buf = {};
      break;
    }
    if (*buf != '#')
    {
      buf = {};
      continue;
    }

    // we don't seem to use buff after this point, so I'm going to free it
    // otherise, we're leaking memory
    buf = {};

    if (max_shop >= MAX_SHOP)
    {
      perror("Too many shops.\n");
      exit(1);
    }

    /*
     * Ignore "producing" list.
     */
    for (count = {}; count < 6; count++)
      fscanf(fp, "%d \n", &temp);

    fscanf(fp, "%f \n", &DC::getInstance()->shop_index[max_shop].profit_buy_base);
    fscanf(fp, "%f \n", &DC::getInstance()->shop_index[max_shop].profit_sell);
    for (count = {}; count < MAX_TRADE; count++)
    {
      fscanf(fp, "%d \n", &DC::getInstance()->shop_index[max_shop].type[count]);
    }

    DC::getInstance()->shop_index[max_shop].profit_buy = DC::getInstance()->shop_index[max_shop].profit_buy_base;
    DC::getInstance()->shop_index[max_shop].no_such_item1 = QString(fread_string(fp, 0)).replace("%s", "%1").replace("%d", "%2");
    DC::getInstance()->shop_index[max_shop].no_such_item2 = QString(fread_string(fp, 0)).replace("%s", "%1").replace("%d", "%2");
    DC::getInstance()->shop_index[max_shop].do_not_buy = QString(fread_string(fp, 0)).replace("%s", "%1").replace("%d", "%2");
    DC::getInstance()->shop_index[max_shop].missing_cash1 = QString(fread_string(fp, 0)).replace("%s", "%1").replace("%d", "%2");
    DC::getInstance()->shop_index[max_shop].missing_cash2 = QString(fread_string(fp, 0)).replace("%s", "%1").replace("%d", "%2");
    DC::getInstance()->shop_index[max_shop].message_buy = QString(fread_string(fp, 0)).replace("%s", "%1").replace("%d", "%2");
    DC::getInstance()->shop_index[max_shop].message_sell = QString(fread_string(fp, 0)).replace("%s", "%1").replace("%d", "%2");

    fscanf(fp, "%d \n", &temp); /* Temper       */
    fscanf(fp, "%d \n", &temp); /* Temper       */

    fscanf(fp, "%d \n", &temp);
    DC::getInstance()->shop_index[max_shop].keeper = real_mobile(temp);

    fscanf(fp, "%d \n", &temp); /* With_whom    */

    fscanf(fp, "%d \n", &temp);

    qint32 room_nr = real_room(temp);
    if (room_nr < 0 || room_nr > DC::getInstance()->top_of_world)
    {
      DC::getInstance()->logf(100, DC::LogChannel::LOG_BUG, "shopkeeper %d loaded with in_room set to %d. Setting to 0.", max_shop, room_nr);
      room_nr = {};
    }

    DC::getInstance()->shop_index[max_shop].in_room = room_nr;

    fscanf(fp, "%d \n", &DC::getInstance()->shop_index[max_shop].open1);
    fscanf(fp, "%d \n", &DC::getInstance()->shop_index[max_shop].close1);
    fscanf(fp, "%d \n", &DC::getInstance()->shop_index[max_shop].open2);
    fscanf(fp, "%d \n", &DC::getInstance()->shop_index[max_shop].close2);

    DC::getInstance()->shop_index[max_shop].inventory = {};

    if (real_room(temp) == DC::NOWHERE)
    {
      QString log_buf = {};
      dc_sprintf(log_buf, "BAD SHOP IN missing ROOM %d -- FIX THIS!", temp);
      DC::getInstance()->logverbose(log_buf);
      continue;
      /* This way we don't increment if it was bad */
    }
    max_shop++;
  }

  fclose(fp);
}

void assign_the_shopkeepers()
{
  qint32 shop_nr;

  for (shop_nr = {}; shop_nr < max_shop; shop_nr++)
    DC::getInstance()->mob_index[DC::getInstance()->shop_index[shop_nr].keeper].non_combat_func = shop_keeper;
}

void fix_shopkeepers_inventory()
{
  qint32 shop_nr;

  CharacterPtr keeper = {};
  ObjectPtr obj, *last_obj, cloned;

  // set up the unlimited supply items. Those the shop_keeper has on start up.

  for (shop_nr = {}; shop_nr < max_shop; shop_nr++)
    for (keeper = DC::getInstance()->world[DC::getInstance()->shop_index[shop_nr].in_room].people; keeper != nullptr;
         keeper = keeper->next_in_room)
    {
      if (keeper->isNonPlayer() && DC::getInstance()->mob_index[keeper->mobdata->nr].non_combat_func == shop_keeper)
      {
        if (keeper->carrying)
        {
          last_obj = clone_object(keeper->carrying->item_number);
          DC::getInstance()->shop_index[shop_nr].inventory = last_obj;
          for (obj = keeper->carrying->next_content; obj;
               obj = obj->next_content)
          {
            cloned = clone_object(obj->item_number);
            last_obj->next_content = cloned;
            last_obj = cloned;
          }
        }
        else
          DC::getInstance()->shop_index[shop_nr].inventory = {};
      }
    }
}

// return {} for failure
// return pointer to new shop on success
player_shop *read_one_player_shop(FILE *fp)
{
  qint32 count;
  QString code;

  player_shop_item *item = {};
  auto shop = new player_shop;

  fread(&shop->owner, sizeof(QChar), PC_SHOP_OWNER_SIZE, fp);
  fread(&shop->room_num, sizeof(qint32), 1, fp);
  fread(&shop->sell_message, sizeof(QChar), PC_SHOP_SELL_MESS_SIZE, fp);
  fread(&shop->money_on_hand, sizeof(qint32), 1, fp);

  code[3] = '\0';
  fread(&code, sizeof(QChar), 3, fp);

  while (strcmp(code, "END"))
  {
    // add future stuff here

    DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Illegal code in player shop %s", shop->owner);
    DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Illegal code in player shop %s", shop->owner);
    exit(1);
  }

  fread(&count, sizeof(qint32), 1, fp);

  shop->sale_list = {};
  for (qint32 i = {}; i < count; i++)
  {
    item = new player_shop_item;

    fread(&item->item_vnum, sizeof(qint32), 1, fp);
    fread(&item->price, sizeof(qint32), 1, fp);
    fread(&code, sizeof(QChar), 3, fp);
    // code junk right now.  Add future stuff before it if needed
    item->next = shop->sale_list;
    shop->sale_list = item;
  }

  return shop;
}

// on failure, warn player if online, log, then keep going
// assumes valid shop
void write_one_player_shop(player_shop *shop)
{
  FILE *fp;
  player_shop_item *item;
  QString buf;
  qint32 count = {};

  dc_sprintf(buf, "%s/%s", PLAYER_SHOP_DIR, shop->owner);

  if ((fp = fopen(buf, "w")) == nullptr)
  {
    DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Could not open %s for writing.", buf);
    return;
  }

  fwrite(&(shop->owner), sizeof(QChar), PC_SHOP_OWNER_SIZE, fp);
  fwrite(&(shop->room_num), sizeof(qint32), 1, fp);
  fwrite(&(shop->sell_message), sizeof(QChar), PC_SHOP_SELL_MESS_SIZE, fp);
  fwrite(&(shop->money_on_hand), sizeof(qint32), 1, fp);

  // add stuff later here with 3 digit code
  // end of variable data
  fwrite("END", sizeof(QChar), 3, fp);

  for (item = shop->sale_list; item; item = item->next)
    count++;

  fwrite(&(count), sizeof(qint32), 1, fp);

  for (item = shop->sale_list; item; item = item->next)
  {
    fwrite(&(item->item_vnum), sizeof(qint32), 1, fp);
    fwrite(&(item->price), sizeof(qint32), 1, fp);
    fwrite("END", sizeof(QChar), 3, fp);
  }

  fclose(fp);
}

// save the list of shopfiles (not an individual shop)
// this only needs to be done when a shop is created or deleted
void save_shop_list()
{
  FILE *fp;

  if ((fp = fopen(PLAYER_SHOP_INDEX, "w")) == nullptr)
  {
    perror(PLAYER_SHOP_INDEX);
    exit(1);
  }

  for (player_shop *shop = g_playershops; shop; shop = shop->next)
    fwrite_string(shop->owner, fp);

  fwrite_string("$", fp);
  fclose(fp);
}

void save_player_shop_world_range()
{
  world_file_list_item *curr;
  QString buf;

  curr = DC::getInstance()->world_file_list;
  while (curr && curr->firstnum != 23000)
    curr = curr->next;

  if (!curr)
  {
    // panic!
    DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Could not find player shop range to save files.");
    exit(1);
  }

  LegacyFile lf("world", curr->filename, "Couldn't open room save file %1 for player shops.");
  if (lf.isOpen())
  {
    for (qint32 x = curr->firstnum; x <= curr->lastnum; x++)
    {
      write_one_room(lf, x);
    }
    qfprintf(lf.file_handle_, "$~\n");
  }
}

void boot_player_shops()
{
  FILE *fp;
  FILE *shopfp;
  player_shop *shop;
  QString filename;
  QString buf;

  g_playershops = {};

  if ((fp = fopen(PLAYER_SHOP_INDEX, "r")) == nullptr)
  {
    perror(PLAYER_SHOP_INDEX);
    exit(1);
  }

  // read list of player owned shops

  filename = fread_string(fp, 0);
  while (strcmp(filename, "$"))
  {
    dc_sprintf(buf, "%s/%s", PLAYER_SHOP_DIR, filename);
    if ((shopfp = fopen(buf, "r")) == nullptr)
    {
      perror(buf);
      exit(1);
    }

    if (!(shop = read_one_player_shop(shopfp)))
    {
      perror(buf);
      exit(1);
    }
    shop->next = g_playershops;
    g_playershops = shop;

    fclose(shopfp);
    filename = fread_string(fp, 0);
  }
  fclose(fp);
}

player_shop *find_player_shop(CharacterPtr keeper)
{
  player_shop *shop = g_playershops;

  for (; shop; shop = shop->next)
    if (real_room(shop->room_num) == keeper->in_room)
      break;

  return shop;
}

// put an item up for sale
void player_shopping_stock(const QString arg, CharacterPtr ch, CharacterPtr keeper)
{
  player_shop *shop = find_player_shop(keeper);
  if (!shop)
  {
    ch->sendln("Invalid player shop keeper.  Let a god know.");
    return;
  }

  if (strcmp(shop->owner, qPrintable(ch->name())))
  {
    ch->sendln("You don't own this shop, you can't stock the shelves!");
    return;
  }

  QString item;
  QString price;

  half_chop(arg, item, price);

  if (!*item || !*price)
  {
    ch->sendln("Syntax:  stock <item> <price>");
    return;
  }

  qint32 value;
  value = atol(price);
  if (value < 1 || value > 20000000)
  {
    ch->sendln("Invalid price.  The price must be between 1 $B$5gold$R and 20 million $B$5gold$R.");
    return;
  }

  // find item
  ObjectPtr obj;

  if ((obj = get_obj_in_list_vis(ch, item, ch->carrying)) == nullptr)
  {
    ch->sendln("Stock what item?");
    return;
  }

  // make sure it isn't NO_DROP, NO_TRADE, etc
  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL) ||
      isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
      isSet(obj->obj_flags.extra_flags, ITEM_NODROP) ||
      isSet(obj->obj_flags.extra_flags, ITEM_NOSAVE))
  {
    ch->sendln("You can't give that to the shop keeper to sell!");
    return;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE))
  {
    ch->sendln("For now you can't sell unique items.");
    return;
  }

  // add it to list
  auto newitem = new player_shop_item;
  newitem->item_vnum = DC::getInstance()->obj_index[obj->item_number].vnum();
  newitem->price = value;
  newitem->next = shop->sale_list;
  shop->sale_list = newitem;
  extract_obj(obj);
  ch->sendln("You put the item up for sale.");
  write_one_player_shop(shop);
}

void player_shopping_buy(const QString arg, CharacterPtr ch, CharacterPtr keeper)
{
  player_shop *shop = find_player_shop(keeper);
  if (!shop)
  {
    ch->sendln("Invalid player shop keeper.  Let a god know.");
    return;
  }

  QString buf;

  one_argument(arg, buf);
  if (!*buf)
  {
    keeper->do_tell(QStringLiteral("%1 Which do you want to buy?").arg(qPrintable(ch->name())).split(' '));
    return;
  }

  qint32 item_pos = atoi(buf);
  player_shop_item *item = shop->sale_list;
  for (qint32 j = 1; (item && j < item_pos); item = item->next, j++)
    ;

  if (!item || item_pos < 1)
  {
    ch->sendln("Choose a valid number!");
    return;
  }

  // make sure they can afford it
  if (ch->getGold() < item->price)
  {
    ch->sendln("You can't afford that!");
    return;
  }

  qint32 robj = real_object(item->item_vnum);
  if (robj < 0)
  {
    ch->sendln("Error, that is not a valid item.  Let a god know.");
    return;
  }

  // give it to them, thank them, take the money
  ObjectPtr obj = clone_object(robj);
  obj_to_char(obj, ch);
  ch->removeGold(item->price);
  shop->money_on_hand += item->price;

  if (!*shop->sell_message)
  {
    keeper->do_tell(QStringLiteral("%1 Thank you, come again!").arg(qPrintable(ch->name())).split(' '));
  }
  else
  {
    keeper->do_tell(QStringLiteral("%1 %2").arg(qPrintable(ch->name())).arg(shop->sell_message).split(' '));
  }

  // update inventory
  for (player_shop_item *curr = shop->sale_list; curr; curr = curr->next)
  {
    if (curr == item)
    { // first item
      shop->sale_list = curr->next;
      curr = {};
      break;
    }
    if (curr->next == item)
    {
      curr->next = item->next;
      item = {};
      break;
    }
  }

  // save the shop
  write_one_player_shop(shop);
}

void player_shopping_withdraw(const QString arg, CharacterPtr ch, CharacterPtr keeper)
{
  player_shop *shop = find_player_shop(keeper);
  if (!shop)
  {
    ch->sendln("Invalid player shop keeper.  Let a god know.");
    return;
  }

  if (strcmp(shop->owner, qPrintable(ch->name())))
  {
    ch->sendln("You don't own this shop!  Go rob a bank or something.");
    return;
  }

  QString price;
  one_argument(arg, price);

  if (!*price)
  {
    ch->sendln("Withdraw how much from your store?");
    return;
  }

  qint32 value;
  value = atol(price);
  if (value < 1 || value > 20000000)
  {
    ch->sendln("Invalid amount.  The amount must be between 1 gold and 20 million gold.");
    return;
  }

  if (value > shop->money_on_hand)
  {
    ch->sendln("You don't have that much in the till!");
    return;
  }

  shop->money_on_hand -= value;
  ch->addGold(value);
  ch->send(QStringLiteral("You take %1 $B$5gold$R out of the till.\r\n").arg(QString::number(value)));
  write_one_player_shop(shop);
}

void player_shopping_design(const QString arg, CharacterPtr ch, CharacterPtr keeper)
{
  QString select;
  QString text;
  qint16 skill;

  if (ch->isNonPlayer())
    return;

  const QStringList pdesign_values = {
      "sellmessage",
      "roomname",
      "roomdesc",
      "\n"};

  player_shop *shop = find_player_shop(keeper);
  if (!shop)
  {
    ch->sendln("Invalid player shop keeper.  Let a god know.");
    return;
  }

  if (strcmp(shop->owner, qPrintable(ch->name())))
  {
    ch->sendln("You don't own this shop, you can't change the design!");
    return;
  }

  half_chop(arg, select, text);

  if (!(*select))
  {
    send_to_char("$3Usage$R: design <field> <correct arguments>\r\n"
                 "Fields are the following.\r\n",
                 ch);
    ch->display_string_list(pdesign_values);
    return;
  }

  for (skill = {};; skill++)
  {
    if (pdesign_values[skill][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return;
    }
    if (is_abbrev(select, pdesign_values[skill]))
      break;
  }

  switch (skill)
  {
  case 0: // sellmessage
    if (!*text)
    {
      send_to_char("$3Syntax$R: design sellmessage <message>\r\n"
                   "  Put 'none' if you want no special message.\r\n",
                   ch);
      return;
    }
    if (strlen(text) > (PC_SHOP_SELL_MESS_SIZE - 20))
    {
      ch->sendln("That sell message is too long.");
      return;
    }
    if (text == u"none"_s)
      *shop->sell_message = '\0';
    else
      strcpy(shop->sell_message, text);
    ch->send(QStringLiteral("Shop sell message changed to '%1'.\r\n").arg(shop->sell_message));
    write_one_player_shop(shop); // save it
    break;

  case 1: // roomname
    if (!*text)
    {
      ch->sendln("$3Syntax$R: design roomname <name>");
      return;
    }
    if (strlen(text) > 60)
    {
      ch->sendln("That room name is too long (60 chars max).");
      return;
    }
    DC::getInstance()->world[shop->room_num].name_ = text;
    ch->sendln(QStringLiteral("Room name set to '%1'.").arg(DC::getInstance()->world[shop->room_num].name_));
    save_player_shop_world_range();
    break;

  case 2: // roomdesc
    ch->sendln("Not active yet.");
    break;

  default:
    send_to_char("$3Usage$R: design <field> <correct arguments>\r\nFields are the following.\r\n", ch);
    ch->display_string_list(pdesign_values);
    break;
  }
}

void player_shopping_sell(const QString arg, CharacterPtr ch, CharacterPtr keeper)
{
  ch->sendln("These shop keeper's don't buy stuff.");
}

void player_shopping_value(const QString arg, CharacterPtr ch, CharacterPtr keeper)
{
  ch->sendln("These shop keeper's don't buy stuff.");
}

void player_shopping_list(const QString arg, CharacterPtr ch, CharacterPtr keeper)
{
  qint32 count = {};
  qint32 robj;
  player_shop *shop = find_player_shop(keeper);
  if (!shop)
  {
    ch->sendln("Invalid player shop keeper.  Let a god know.");
    return;
  }
  send_to_char("Item                                          Price\r\n"
               "------------------------------------------------------\r\n",
               ch);

  if (!shop->sale_list)
    ch->sendln("There is nothing for sale here :(");

  else
    for (player_shop_item *item = shop->sale_list; item; item = item->next)
    {
      count++;
      robj = real_object(item->item_vnum);
      if (robj < 0)
        ch->send(QStringLiteral("%1$3)$R %2 %3\r\n").arg(QString::number(count), -3).arg("INVALID ITEM NUMBER", -40).arg(QString::number(item->price)));
      else
        ch->send(QStringLiteral("%1$3)$R %2 %3\r\n").arg(QString::number(count), -3).arg(((ObjectPtr)DC::getInstance()->obj_index[robj].item)->short_description(), -40).arg(QString::number(item->price)));
    }

  if (!strcmp(shop->owner, qPrintable(ch->name())))
    ch->send(QStringLiteral("\r\nYour shop has %1 cash in the till.\r\n").arg(QString::number(shop->money_on_hand)));
}

qint32 player_shop_keeper(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr invoker)
{
  CharacterPtr keeper;

  if (!(keeper = invoker))
  {
    DC::getInstance()->logentry(QStringLiteral("Shop_keeper: keeper not found."), ANGEL, DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  }

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  switch (cmd)
  {
  case cmd_t::WITHDRAW:
    player_shopping_withdraw(arg, ch, keeper);
    break;
  case cmd_t::DESIGN:
    player_shopping_design(arg, ch, keeper);
    break;
  case cmd_t::STOCK:
    player_shopping_stock(arg, ch, keeper);
    break;
  case cmd_t::BUY:
    player_shopping_buy(arg, ch, keeper);
    break;
  case cmd_t::SELL:
    player_shopping_sell(arg, ch, keeper);
    break;
  case cmd_t::VALUE:
    player_shopping_value(arg, ch, keeper);
    break;
  case cmd_t::LIST:
    player_shopping_list(arg, ch, keeper);
    break;
  default:
    return ReturnValue::eFAILURE;
  }

  return ReturnValue::eSUCCESS;
}
/*
command_return_t do_pshopedit(CharacterPtr  ch, QString arg, cmd_t cmd)
{
  QString buf;
  QString select;
  QString text;
  qint16 skill, i;
  player_shop * shop;

  if(ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if(!ch->has_skill( COMMAND_PSHOPEDIT)) {
        ch->sendln("Huh?");
        return ReturnValue::eFAILURE;
  }

  QStringList pshopedit_values [] = {
    "new",
    "delete",
    "list"
  };

  arg = one_argumentnolow(arg, select);

  if(!(*select)) {
    send_to_char("$3Usage$R: pshopedit <field> <correct arguments>\r\n"
                 "Fields are the following.\r\n"
                 , ch);
    display_string_list(pshopedit_values, ch);
    return ReturnValue::eFAILURE;
  }

  for(skill = 0 ;; skill++)
  {
    if(pshopedit_values[skill][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return ReturnValue::eFAILURE;
    }
    if(is_abbrev(select, pshopedit_values[skill]))
      break;
  }

  switch(skill) {
    case 0: // new
      half_chop(arg, buf, text);
      if(!*buf || !*text) {
         send_to_char("$3Syntax$R: pshopedit new <Playername> <roomnum>\r\n"
                      "  Make sure that your CAPITALIZE the player name or\r\n"
                      "  they won't be able to use it.\r\n", ch);
         return ReturnValue::eFAILURE;
      }
      i = atoi(text);
      if(i < 1 || i > DC::getInstance()->top_of_world || !DC::getInstance()->rooms[i]) {
         ch->sendln("You must choose a valid room number.");
         return ReturnValue::eFAILURE;
      }
      shop = (player_shop *)dc_alloc(1, sizeof(player_shop));
      strcpy(shop->owner, buf);
      *shop->sell_message = '\0';
      shop->room_num = i;
      shop->money_on_hand = {};
      shop->sale_list = {};
      shop->next = g_playershops;
      g_playershops = shop;
      save_shop_list();
      write_one_player_shop(shop);
      ch->send(QStringLiteral("Shop created for player '%s' in room %d.\r\n").arg(shop->owner).arg(shop->room_num));
      break;

    case 1: // delete

      save_shop_list();
      break;

    case 2: // list
      ch->sendln("Player Shops\r\n-------------------------");
      if(!g_playershops)
         ch->sendln("No current shops.");
      else for(i = 1, shop = g_playershops; shop; shop = shop->next, i++)
        ch->send(QStringLiteral("%2d$3)$R %-15s $3Room$R: %5d\r\n").arg(i).arg(shop->owner).arg(shop->room_num));
      break;

    default:
      send_to_char("$3Usage$R: pshopedit <field> <correct arguments>\r\n"
                 "Fields are the following.\r\n"
                 , ch);
      display_string_list(pshopedit_values, ch);
      break;
  }

  return ReturnValue::eSUCCESS;
}
*/
void assign_the_player_shopkeepers()
{
  DC::getInstance()->mob_index[real_mobile(PLAYER_SHOP_KEEPER)].non_combat_func = player_shop_keeper;
}

void redo_shop_profit()
{
  switch (number(0, 3))
  {
  case 0:
    break;
  case 1:
    for (qint32 i = {}; i < 58; i++)
    {
      DC::getInstance()->shop_index[i].profit_buy = DC::getInstance()->shop_index[i].profit_buy_base;
    }
    break;
  case 2:
    for (qint32 i = {}; i < 58; i++)
      DC::getInstance()->shop_index[i].profit_buy *= 1.0 + number(10, 50) / 100.0;
    break;
  case 3:
    for (qint32 i = {}; i < 58; i++)
    {
      DC::getInstance()->shop_index[i].profit_buy *= 1.0 - number(10, 50) / 100.0;
      DC::getInstance()->shop_index[i].profit_buy = MAX<float>(DC::getInstance()->shop_index[i].profit_buy, DC::getInstance()->shop_index[i].profit_sell + 0.1);
    }
    break;
  default:
    break;
  }
}

class obj_exchange
{
public:
  qint32 item_qty;
  qint32 item_vnum;
  qint32 cost_qty;
  qint32 cost_vnum;
  quint64 cost_exp;
  quint64 cost_plats;
};

const qint32 OBJ_APOCALYPSE = 27905;
const qint32 OBJ_BROWNIE = 27906;
const qint32 OBJ_MEATBALL = 27908;
const qint32 OBJ_WINGDING = 27909;
const qint32 OBJ_CLOVERLEAF = 27910;

const qint32 MAX_EDDIE_ITEMS = 16;

obj_exchange eddie[MAX_EDDIE_ITEMS] = {
    {1, OBJ_CLOVERLEAF, 2, OBJ_WINGDING, 0, 0},
    {1, OBJ_CLOVERLEAF, 2, OBJ_MEATBALL, 0, 0},
    {1, OBJ_CLOVERLEAF, 2, OBJ_APOCALYPSE, 0, 0},
    {1, OBJ_CLOVERLEAF, 0, 0, 5000000000, 0},
    {2, OBJ_CLOVERLEAF, 1, OBJ_BROWNIE, 0, 0},
    {1, OBJ_WINGDING, 3, OBJ_CLOVERLEAF, 0, 0},
    {1, OBJ_WINGDING, 2, OBJ_MEATBALL, 0, 0},
    {1, OBJ_WINGDING, 2, OBJ_APOCALYPSE, 0, 0},
    {1, OBJ_WINGDING, 0, 0, 0, 1000},
    {1, OBJ_MEATBALL, 3, OBJ_CLOVERLEAF, 0, 0},
    {1, OBJ_MEATBALL, 2, OBJ_WINGDING, 0, 0},
    {1, OBJ_MEATBALL, 2, OBJ_APOCALYPSE, 0, 0},
    {1, OBJ_APOCALYPSE, 3, OBJ_CLOVERLEAF, 0, 0},
    {1, OBJ_APOCALYPSE, 2, OBJ_WINGDING, 0, 0},
    {1, OBJ_APOCALYPSE, 2, OBJ_MEATBALL, 0, 0},
    {1, OBJ_BROWNIE, 10, OBJ_CLOVERLEAF, 0, 0}};

qint32 eddie_shopkeeper(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  if (cmd != cmd_t::LIST && cmd != cmd_t::BUY)
    return ReturnValue::eFAILURE;

  if (IS_AFFECTED(ch, AFF_BLIND))
  {
    ch->send("You're too blind to do that!\r\n");
    return ReturnValue::eFAILURE;
  }

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (cmd == cmd_t::LIST)
  {
    ch->send(QStringLiteral("$B$2%s tells you, 'This is what I can do for you...\r\n$R").arg(qPrintable(owner->shortdesc_or_name())));
    ch->sendln("  | Item                              | Cost                                   |");
    ch->sendln("--------------------------------------------------------------------------------");
    qint32 last_vnum = {};
    for (qint32 i = {}; i < MAX_EDDIE_ITEMS; i++)
    {
      QString buf = {};
      QString item_buf = {};
      QString cost_buf = {};
      if (eddie[i].item_vnum > 0)
      {
        strncpy(item_buf, qPrintable(((ObjectPtr)DC::getInstance()->obj_index[real_object(eddie[i].item_vnum)].item)->short_description()), 1024);
      }
      else
      {
        continue;
      }

      if (eddie[i].cost_vnum > 0)
      {
        strncpy(cost_buf, qPrintable(((ObjectPtr)DC::getInstance()->obj_index[real_object(eddie[i].cost_vnum)].item)->short_description()), 1024);
      }
      else if (eddie[i].cost_exp > 0)
      {
        QString cost_buf_str = fmt::format(std::locale("en_US.UTF-8"), "{:L} experience", eddie[i].cost_exp);
        dc_snprintf(cost_buf, 1024, "%s", cost_buf_str.c_str());
      }
      else if (eddie[i].cost_plats > 0)
      {
        QString cost_buf_str = fmt::format(std::locale("en_US.UTF-8"), "{:L} platinum", eddie[i].cost_plats);
        dc_snprintf(cost_buf, 1024, "%s", cost_buf_str.c_str());
      }

      if (last_vnum != 0 && last_vnum != eddie[i].item_vnum)
      {
        ch->sendln("--------------------------------------------------------------------------------");
      }

      last_vnum = eddie[i].item_vnum;
      qint32 item_qty = eddie[i].item_qty;
      qint32 cost_qty = eddie[i].cost_qty;
      quint64 cost_plats = eddie[i].cost_plats;

      // setup format specifier based length of item short descriptions
      if (cost_qty > 0)
      {

        ch->sendln(QStringLiteral("$B$3%%2d$R|%%2d x %%-%zus|%%2d x %%-%zus|").arg(i + 1).arg(item_qty).arg(item_buf).arg(cost_qty).arg(cost_buf));
      }
      else if (cost_plats > 0)
      {

        ch->sendln(QStringLiteral("$B$3%%2d$R|%%2d x %%-%zus| $B%%-%zus$R    |\r\n").arg(i + 1).arg(item_qty).arg(item_buf).arg(cost_buf));
      }
      else
      {
        ch->sendln(QStringLiteral("$B$3%%2d$R|%%2d x %%-%zus| %%-%zus    |\r\n").arg(i + 1).arg(item_qty).arg(item_buf).arg(cost_buf));
      }
    }
    ch->sendln("--------------------------------------------------------------------------------");

    /*
    ch->sendln(" $B$31)$R Cloverleaf Token      Cost: 2 Wingding tokens.");
    ch->sendln(" $B$32)$R Cloverleaf Token      Cost: 2 Meatball tokens.");
    ch->sendln(" $B$33)$R Cloverleaf Token      Cost: 2 Apocalypse tokens.");

    ch->sendln(" $B$34)$R Wingding Token        Cost: 3 Cloverleaf tokens.");
    ch->sendln(" $B$35)$R Wingding Token        Cost: 2 Meatball tokens.");
    ch->sendln(" $B$36)$R Wingding Token        Cost: 2 Apocalypse tokens.");

    ch->sendln(" $B$37)$R Meatball Token        Cost: 3 Cloverleaf tokens.");
    ch->sendln(" $B$38)$R Meatball Token        Cost: 2 Wingding tokens.");
    ch->sendln(" $B$39)$R Meatball Token        Cost: 2 Apocalypse tokens.");

    ch->sendln("$B$310)$R Apocalypse Token      Cost: 2 Wingding tokens.");
    ch->sendln("$B$311)$R Apocalypse Token      Cost: 2 Meatball tokens.");

    ch->sendln("$B$312)$R Brownie Point         Cost: 10 Cloverleaf tokens.");
    */
    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::BUY)
  {
    QString arg1;
    QString buf;

    one_argument(arg, arg1);

    if (*arg1 == 0)
    {
      ch->sendln("Buy what?");
      return ReturnValue::eSUCCESS;
    }

    qint32 choice = atoi(arg1);
    if (choice < 1 || choice > MAX_EDDIE_ITEMS)
    {
      ch->send(QStringLiteral("Invalid number. Choose between 1 and %1.\r\n").arg(QString::number(MAX_EDDIE_ITEMS)));
      return ReturnValue::eSUCCESS;
    }

    if ((eddie[choice - 1].cost_qty == 0 || eddie[choice - 1].cost_vnum < 1) && eddie[choice - 1].cost_exp > 0)
    {
      if (ch->exp >= eddie[choice - 1].cost_exp)
      {
        ch->exp -= eddie[choice - 1].cost_exp;
        ch->send(fmt::format(std::locale("en_US.UTF-8"), "Eddie takes {:L} experience from you, leaving you with {:L} experience.\r\n", eddie[choice - 1].cost_exp, ch->exp));
      }
      else
      {
        ch->send(fmt::format(std::locale("en_US.UTF-8"), "You do not have the {:L} experience to pay for that item. You need {:L} more experience.\r\n", eddie[choice - 1].cost_exp, eddie[choice - 1].cost_exp - ch->exp));
        return ReturnValue::eSUCCESS;
      }
    }
    else if (eddie[choice - 1].cost_plats > 0)
    {
      if (GET_PLATINUM(ch) >= eddie[choice - 1].cost_plats)
      {
        GET_PLATINUM(ch) -= eddie[choice - 1].cost_plats;
        ch->send(fmt::format(std::locale("en_US.UTF-8"), "Eddie takes {:L} platinum from you, leaving you with {:L} platinum.\r\n", eddie[choice - 1].cost_plats, GET_PLATINUM(ch)));
      }
      else
      {
        ch->send(fmt::format(std::locale("en_US.UTF-8"), "You do not have the {:L} platinum to pay for that item. You need {:L} more platinum.\r\n", eddie[choice - 1].cost_plats, eddie[choice - 1].cost_plats - GET_PLATINUM(ch)));
        return ReturnValue::eSUCCESS;
      }
    }
    else
    {

      qint32 count = search_char_for_item_count(ch, real_object(eddie[choice - 1].cost_vnum), false);

      if (count < eddie[choice - 1].cost_qty)
      {
        ch->sendln("You don't have enough to trade.");
        return ReturnValue::eSUCCESS;
      }

      for (qint32 i = {}; i < eddie[choice - 1].cost_qty; i++)
      {
        ObjectPtr obj = search_char_for_item(ch, real_object(eddie[choice - 1].cost_vnum), false);
        if (obj != 0)
        {
          if (obj->in_obj)
          {
            obj_from_obj(obj);
          }
          else
          {
            obj_from_char(obj);
          }

          act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL | NOTVICT);
          act("$n gives you $p.", ch, obj, owner, TO_VICT, 0);
          act("You give $p to $N.", ch, obj, owner, TO_CHAR, 0);

          dc_sprintf(buf, "%s gives %s to %s (removed)", qPrintable(ch->name()), qPrintable(obj->name()), qPrintable(owner->name()));
          DC::getInstance()->logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
        }
        else
        {
          ch->sendln("An error occured.");
          ch->save(cmd_t::SAVE_SILENTLY);
          return ReturnValue::eSUCCESS;
        }
      }
    }

    for (qint32 i = {}; i < eddie[choice - 1].item_qty; i++)
    {
      ObjectPtr item = clone_object(real_object(eddie[choice - 1].item_vnum));
      if (item != 0)
      {
        obj_to_char(item, ch);

        act("$n gives $p to $N.", owner, item, ch, TO_ROOM, INVIS_NULL | NOTVICT);
        act("$n gives you $p.", owner, item, ch, TO_VICT, 0);
        act("You give $p to $N.", owner, item, ch, TO_CHAR, 0);

        dc_sprintf(buf, "%s gives %s to %s (created)", qPrintable(owner->name()), qPrintable(item->name()), qPrintable(ch->name()));
        DC::getInstance()->logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
      }
      else
      {
        ch->sendln("An error occured.");
        ch->save(cmd_t::SAVE_SILENTLY);
        return ReturnValue::eSUCCESS;
      }
    }
    ch->save();
  }

  return ReturnValue::eSUCCESS;
}

qint32 reroll_trader(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  if (ch == nullptr || ch->isNonPlayer())
  {
    return ReturnValue::eFAILURE;
  }

  QString arg1, remainder_args;
  std::tie(arg1, remainder_args) = half_chop(arg);

  reroll_t r = {};
  if (reroll_sessions.contains(qPrintable(ch->name())))
  {
    r = reroll_sessions[qPrintable(ch->name())];
  }

  obj_list_t obj_list = {};
  switch (cmd)
  {
  case cmd_t::LIST:
    if (r.state == reroll_t::reroll_states_t::PICKED_OBJ_TO_REROLL)
    {
      owner->tell(ch, QStringLiteral("You need to confirm or cancel rerolling %1.").arg(GET_OBJ_SHORT(r.orig_obj)));
      return ReturnValue::eSUCCESS;
    }

    owner->tell(ch, "Type reroll <object keyword> to reroll that object.");
    owner->tell(ch, "I will then ask you to confirm the object you want re-rolled.");
    owner->tell(ch, "The cost will be 1 Cloverleaf token.");
    owner->tell(ch, "You will get two re-rolled choices or the original to pick from.");
    owner->tell(ch, "Type choose 1, 2 or 3 to choose either one of the two rerolls or the original.");
    return ReturnValue::eSUCCESS;
    break;

  case cmd_t::REROLL:
    if (r.state == reroll_t::reroll_states_t::PICKED_OBJ_TO_REROLL)
    {
      owner->tell(ch, QStringLiteral("You need to confirm or cancel rerolling %1.").arg(GET_OBJ_SHORT(r.orig_obj)));
      return ReturnValue::eSUCCESS;
    }
    else if (r.state == reroll_t::reroll_states_t::REROLLED)
    {
      owner->tell(ch, "You need to choose 1, 2, 3 or type cancel before you can reroll again.");
      return ReturnValue::eSUCCESS;
    }

    if (arg1.empty())
    {
      owner->tell(ch, "You have to type reroll <object keyword> to reroll that object.");
      return ReturnValue::eSUCCESS;
    }
    else
    {
      obj = get_obj_in_list_vis(ch, arg1.c_str(), ch->carrying);
      if (obj == nullptr)
      {
        owner->tell(ch, QStringLiteral("I don't see anything on you matching \'%1\'").arg(arg1.c_str()));
        return ReturnValue::eSUCCESS;
      }
      else
      {
        if (GET_OBJ_TYPE(obj) != ITEM_WEAPON &&
            GET_OBJ_TYPE(obj) != ITEM_ARMOR &&
            GET_OBJ_TYPE(obj) != ITEM_INSTRUMENT &&
            GET_OBJ_TYPE(obj) != ITEM_WAND &&
            GET_OBJ_TYPE(obj) != ITEM_STAFF &&
            GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
        {
          owner->tell(ch, "I can only reroll weapons, armor, instruments, wands, staffs and containers.");
          return ReturnValue::eSUCCESS;
        }

        if (obj->isGodload())
        {
          owner->tell(ch, "I can't reroll GL weapons or armor.");
          return ReturnValue::eSUCCESS;
        }

        if (obj->isQuest())
        {
          owner->tell(ch, "I can't reroll quest weapons or armor.");
          return ReturnValue::eSUCCESS;
        }

        if (obj->isCustom())
        {
          owner->tell(ch, "I can't reroll objects with the NO_CUSTOM flag set on them.");
          return ReturnValue::eSUCCESS;
        }

        r = {};
        r.orig_obj = obj;
        r.orig_rnum = GET_OBJ_RNUM(obj);
        r.state = reroll_t::reroll_states_t::PICKED_OBJ_TO_REROLL;
        reroll_sessions[qPrintable(ch->name())] = r;
        owner->tell(ch, QStringLiteral("Are you sure you want me to reroll %1 for you?").arg(GET_OBJ_SHORT(obj)));
        owner->tell(ch, "Type confirm and I'll reroll it otherwise type cancel if you changed your mind.");
      }
    }
    break;

  case cmd_t::CONFIRM:
    if (r.state == reroll_t::reroll_states_t::PICKED_OBJ_TO_REROLL)
    {
      if (search_char_for_item_count(ch, real_object(OBJ_CLOVERLEAF), false) < 1)
      {
        owner->tell(ch, "You don't have the required cloverleaf token.");
        return ReturnValue::eSUCCESS;
      }

      obj = search_char_for_item(ch, real_object(OBJ_CLOVERLEAF), false);
      if (obj != 0)
      {
        obj_from(obj);
        act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL | NOTVICT);
        act("$n gives you $p.", ch, obj, owner, TO_VICT, 0);
        act("You give $p to $N.", ch, obj, owner, TO_CHAR, 0);
        DC::getInstance()->logentry(QStringLiteral("%1 gives %2 to %3").arg(qPrintable(ch->name())).arg(obj->name()).arg(qPrintable(owner->name())), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
      }

      if (r.orig_obj != nullptr)
      {
        move_obj(r.orig_obj, owner);
        act("$n gives $p to $N.", ch, r.orig_obj, owner, TO_ROOM, INVIS_NULL | NOTVICT);
        act("$n gives you $p.", ch, r.orig_obj, owner, TO_VICT, 0);
        act("You give $p to $N.", ch, r.orig_obj, owner, TO_CHAR, 0);
        DC::getInstance()->logentry(QStringLiteral("%1 gives %2 to %3").arg(qPrintable(ch->name())).arg(r.orig_obj->name()).arg(qPrintable(owner->name())), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
      }
      else
      {
        ch->send("An error occurred. The object is missing.\r\n");
      }

      obj = r.orig_obj;
      obj_list = oload(owner, GET_OBJ_RNUM(obj), 2, true);
      for (const auto &o : obj_list)
      {
        if (r.choice1_obj == nullptr)
        {
          owner->tell(ch, "Choice 1 is:");
          identify(ch, o);
          ch->send("\r\n");
          r.choice1_obj = o;
        }
        else if (r.choice2_obj == nullptr)
        {
          owner->tell(ch, "Choice 2 is:");
          identify(ch, o);
          ch->send("\r\n");
          r.choice2_obj = o;
        }
        r.state = reroll_t::reroll_states_t::REROLLED;
        reroll_sessions[qPrintable(ch->name())] = r;
      }
      owner->tell(ch, "Choice 3 is:");
      identify(ch, r.orig_obj);
      owner->tell(ch, "Type choose 1, 2 or 3.");
    }
    else
    {
      return ReturnValue::eFAILURE;
    }

    break;

  case cmd_t::CHOOSE:
    if (r.state != reroll_t::reroll_states_t::REROLLED)
    {
      return ReturnValue::eFAILURE;
    }

    if (arg1 == "1")
    {
      if (r.choice1_obj != nullptr)
      {
        move_obj(r.choice1_obj, ch);
        DC::getInstance()->logentry(QStringLiteral("%1 gives %2 to %3").arg(qPrintable(owner->name())).arg(r.choice1_obj->name()).arg(qPrintable(ch->name())), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
        act("$n gives $p to $N.", owner, r.choice1_obj, ch, TO_ROOM, INVIS_NULL | NOTVICT);
        act("$n gives you $p.", owner, r.choice1_obj, ch, TO_VICT, 0);
        act("You give $p to $N.", owner, r.choice1_obj, ch, TO_CHAR, 0);
      }
      else
      {
        ch->send("An error occurred. The object is missing.\r\n");
      }
      obj_from(r.choice2_obj);
      obj_from(r.orig_obj);
    }
    else if (arg1 == "2")
    {
      if (r.choice2_obj != nullptr)
      {
        move_obj(r.choice2_obj, ch);
        DC::getInstance()->logentry(QStringLiteral("%1 gives %2 to %3").arg(qPrintable(owner->name())).arg(r.choice2_obj->name()).arg(qPrintable(ch->name())), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
        act("$n gives $p to $N.", owner, r.choice2_obj, ch, TO_ROOM, INVIS_NULL | NOTVICT);
        act("$n gives you $p.", owner, r.choice2_obj, ch, TO_VICT, 0);
        act("You give $p to $N.", owner, r.choice2_obj, ch, TO_CHAR, 0);
      }
      else
      {
        ch->send("An error occurred. The object is missing.\r\n");
      }

      obj_from(r.choice1_obj);
      obj_from(r.orig_obj);
    }
    else if (arg1 == "3")
    {
      if (r.orig_obj != nullptr)
      {
        move_obj(r.orig_obj, ch);
        DC::getInstance()->logentry(QStringLiteral("%1 gives %2 to %3").arg(qPrintable(owner->name())).arg(r.orig_obj->name()).arg(qPrintable(ch->name())), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
        act("$n gives $p to $N.", owner, r.orig_obj, ch, TO_ROOM, INVIS_NULL | NOTVICT);
        act("$n gives you $p.", owner, r.orig_obj, ch, TO_VICT, 0);
        act("You give $p to $N.", owner, r.orig_obj, ch, TO_CHAR, 0);
      }
      else
      {
        ch->send("An error occurred. The object is missing.\r\n");
      }
      obj_from(r.choice1_obj);
      obj_from(r.choice2_obj);
    }
    else
    {
      owner->tell(ch, "Type choose 1, 2 or 3.");
      return ReturnValue::eSUCCESS;
    }
    reroll_sessions.erase(qPrintable(ch->name()));
    break;

  case cmd_t::CANCEL:
    owner->tell(ch, "I'm canceling this reroll.");

    obj_from(r.choice1_obj);
    obj_from(r.choice2_obj);
    if (r.orig_obj != nullptr)
    {
      move_obj(r.orig_obj, ch);
      DC::getInstance()->logentry(QStringLiteral("%1 gives %2 to %3").arg(qPrintable(owner->name())).arg(r.orig_obj->name()).arg(qPrintable(ch->name())), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
      act("$n gives $p to $N.", owner, r.orig_obj, ch, TO_ROOM, INVIS_NULL | NOTVICT);
      act("$n gives you $p.", owner, r.orig_obj, ch, TO_VICT, 0);
      act("You give $p to $N.", owner, r.orig_obj, ch, TO_CHAR, 0);
    }
    else
    {
      ch->send("An error occurred. The object is missing.\r\n");
    }
    reroll_sessions.erase(qPrintable(ch->name()));
    break;

  default:
    return ReturnValue::eFAILURE;
    break;
  }

  return ReturnValue::eSUCCESS;
}

qint32 redeem_trader(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  switch (cmd)
  {
  case cmd_t::REDEEM:
  case cmd_t::LIST:
  case cmd_t::CANCEL:
  case cmd_t::CONFIRM:
    break;
  default:
    return ReturnValue::eFAILURE;
  }

  if (!ch || !ch->isPlayer() || !arg)
  {
    return ReturnValue::eFAILURE;
  }

  QStringList arguments = QString(arg).split(' ');
  QString arg1 = arguments.value(0);
  QString arg2 = arguments.value(1);
  QString arg3 = arguments.value(2);
  redeem_t r = DC::getInstance()->redeem_sessions.value(ch->name());

  switch (r.state)
  {
  case redeem_t::state_t::BEGIN:
    switch (cmd)
    {
    case cmd_t::REDEEM:
      if (!arg1.isEmpty())
        break;
    case cmd_t::LIST:
    case cmd_t::CANCEL:
    case cmd_t::CONFIRM:
      owner->tell(ch, "Type 'redeem v#' or 'redeem v# random' to redeem Apocalypse tokens "
                      "for the item with vnum v#. I will then ask you to confirm the item "
                      "you want redeemed. The cost will be 3 Apocalypse tokens for a level "
                      "50 or under item or 6 Apocalypse tokens for a level 51 to 59 item.");
      return ReturnValue::eSUCCESS;
      break;
    default:
      return ReturnValue::eFAILURE;
      break;
    }

    if (!arg1.isEmpty())
    {
      r = {};

      if (arg1.contains("random", Qt::CaseInsensitive))
      {
        r.random = true;
        arg1 = arg2;
      }
      if (arg2.contains("random", Qt::CaseInsensitive))
      {
        r.random = true;
      }

      // needs to lookup world by vnum
      obj = get_objindex_vnum(arg1);
      if (obj == nullptr)
      {
        owner->tell(ch, QStringLiteral("I can't find an object with vnum \'%1\'").arg(arg1));
        return ReturnValue::eSUCCESS;
      }
      else
      {
        if (GET_OBJ_TYPE(obj) != ITEM_WEAPON &&
            GET_OBJ_TYPE(obj) != ITEM_ARMOR &&
            GET_OBJ_TYPE(obj) != ITEM_INSTRUMENT &&
            GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
        {
          owner->tell(ch, "I can only redeem Apocalypse tokens for weapons, armor, instruments and containers.");
          return ReturnValue::eSUCCESS;
        }

        if (obj->isGodload())
        {
          owner->tell(ch, "I can't redeem for GL items.");
          return ReturnValue::eSUCCESS;
        }

        if (obj->isQuest())
        {
          owner->tell(ch, "I can't redeem for quest items.");
          return ReturnValue::eSUCCESS;
        }

        if (obj->isCustom() && r.random)
        {
          owner->tell(ch, "I can't redeem randomized versions of items with NO_CUSTOM flag set.");
          return ReturnValue::eSUCCESS;
        }

        if (obj->isTest())
        {
          owner->tell(ch, "You can't redeem for test items.");
          return ReturnValue::eSUCCESS;
        }

        if (obj->getLevel() >= 60)
        {
          owner->tell(ch, "You can't redeem Apocalypse tokens for a level 60 item.");
          return ReturnValue::eSUCCESS;
        }

        ch->do_identify(QStringLiteral("v%1").arg(DC::getInstance()->obj_index[obj->item_number].vnum()).split(' '));

        r.orig_obj = obj;
        r.orig_rnum = GET_OBJ_RNUM(obj);
        r.state = redeem_t::state_t::PICKED_OBJ_TO_REDEEM;
        if (obj->getLevel() >= 51)
          r.token_count = 6;
        else
          r.token_count = 3;
        DC::getInstance()->redeem_sessions[qPrintable(ch->name())] = r;

        auto random_str = r.random ? "randomized" : "default";
        owner->tell(ch, QStringLiteral("Type 'confirm' if you want me to redeem %1 Apocalypse tokens for one %2 '%3'? Type "
                                       "'cancel' if not.")
                            .arg(QString::number(r.token_count))
                            .arg(random_str)
                            .arg(GET_OBJ_SHORT(obj)));
        return ReturnValue::eSUCCESS;
      }
    }
    break;
  case redeem_t::state_t::PICKED_OBJ_TO_REDEEM:
    switch (cmd)
    {
    case cmd_t::REDEEM:
    case cmd_t::LIST:
      owner->tell(ch, QStringLiteral("You need to confirm or cancel redeeming %1.").arg(GET_OBJ_SHORT(r.orig_obj)));
      return ReturnValue::eSUCCESS;
      break;
    case cmd_t::CONFIRM:
      break;
    case cmd_t::UNDEFINED:
    case cmd_t::NORTH:
    case cmd_t::EAST:
    case cmd_t::SOUTH:
    case cmd_t::WEST:
    case cmd_t::UP:
    case cmd_t::DOWN:
    case cmd_t::BELLOW:
    case cmd_t::DEFAULT:
    case cmd_t::TRACK:
    case cmd_t::PALM:
    case cmd_t::SAY:
    case cmd_t::LOOK:
    case cmd_t::BACKSTAB:
    case cmd_t::SBS:
    case cmd_t::ORCHESTRATE:
    case cmd_t::REPLY:
    case cmd_t::WHISPER:
    case cmd_t::GLANCE:
    case cmd_t::FLEE:
    case cmd_t::ESCAPE:
    case cmd_t::PICK:
    case cmd_t::STOCK:
    case cmd_t::BUY:
    case cmd_t::SELL:
    case cmd_t::VALUE:
    case cmd_t::ENTER:
    case cmd_t::CLIMB:
    case cmd_t::DESIGN:
    case cmd_t::PRICE:
    case cmd_t::REPAIR:
    case cmd_t::READ:
    case cmd_t::REMOVE:
    case cmd_t::ERASE:
    case cmd_t::ESTIMATE:
    case cmd_t::REMORT:
    case cmd_t::REROLL:
    case cmd_t::CHOOSE:
    case cmd_t::CANCEL:
    case cmd_t::SLIP:
    case cmd_t::GIVE:
    case cmd_t::DROP:
    case cmd_t::DONATE:
    case cmd_t::QUIT:
    case cmd_t::SACRIFICE:
    case cmd_t::PUT:
    case cmd_t::OPEN:
    case cmd_t::EDITOR:
    case cmd_t::FORCE:
    case cmd_t::WRITE:
    case cmd_t::WATCH:
    case cmd_t::PRACTICE:
    case cmd_t::TRAIN:
    case cmd_t::PROFESSION:
    case cmd_t::GAIN:
    case cmd_t::BALANCE:
    case cmd_t::DEPOSIT:
    case cmd_t::WITHDRAW:
    case cmd_t::CLEAN:
    case cmd_t::PLAY:
    case cmd_t::FINISH:
    case cmd_t::VETERNARIAN:
    case cmd_t::FEED:
    case cmd_t::ASSEMBLE:
    case cmd_t::PAY:
    case cmd_t::RESTRING:
    case cmd_t::PUSH:
    case cmd_t::PULL:
    case cmd_t::LEAVE:
    case cmd_t::TREMOR:
    case cmd_t::BET:
    case cmd_t::INSURANCE:
    case cmd_t::DOUBLE:
    case cmd_t::STAY:
    case cmd_t::SPLIT:
    case cmd_t::HIT:
    case cmd_t::LOOT:
    case cmd_t::GTELL:
    case cmd_t::CTELL:
    case cmd_t::SETVOTE:
    case cmd_t::VOTE:
    case cmd_t::VEND:
    case cmd_t::FILTER:
    case cmd_t::EXAMINE:
    case cmd_t::GAG:
    case cmd_t::IMMORT:
    case cmd_t::IMPCHAN:
    case cmd_t::TELL:
    case cmd_t::TELLH:
    case cmd_t::PRIZE:
    case cmd_t::OTHER:
    case cmd_t::TELL_REPLY:
    case cmd_t::GAZE:
    case cmd_t::SAVE_SILENTLY:
    case cmd_t::ONEWAY:
    case cmd_t::TWOWAY:
    case cmd_t::MLOCATE_CHARACTER:
    case cmd_t::FEAR:
    case cmd_t::PAGING_HELP:
    case cmd_t::QUEST_CANCEL:
    case cmd_t::QUEST_START:
    case cmd_t::QUEST_FINISH:
    case cmd_t::QUEST_LIST:
    case cmd_t::GOLEMSCORE:
    case cmd_t::FSCORE:
      break;
    }

    return ReturnValue::eSUCCESS;
    break;
  case redeem_t::state_t::REDEEM:
    owner->tell(ch, "You need to choose 1, 2, 3 or type cancel before you can reroll again.");
    return ReturnValue::eSUCCESS;
    break;
  case redeem_t::CHOSEN:
    break;
  }

  /*
        if (r.orig_obj->getLevel() <= 50)
        {
          token_count = 3;
        }
        else if (r.orig_obj->getLevel() <= 59)
        {
          token_count = 6;
        }

  */

  return ReturnValue::eFAILURE;
}