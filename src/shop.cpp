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

#include "DC/obj.h"
#include "DC/affect.h"
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/interp.h"
#include "DC/DC.h"
#include "DC/player.h"
#include "DC/handler.h"
#include "DC/mobile.h"
#include "DC/room.h"
#include "DC/fileinfo.h"
#include "DC/db.h"
#include "DC/act.h"
#include "DC/returnvals.h"
#include "DC/shop.h"
#include "DC/spells.h"
#include "DC/inventory.h"
#include "DC/const.h"
#include "DC/wizard.h"

struct player_shop *g_playershops;

extern struct time_info_data time_info;

int max_shop;

// extern function
int fwrite_string(char *buf, FILE *fl);

std::map<std::string, reroll_t> reroll_sessions = {};

/*
 * See if a shop keeper wants to trade.
 */
int is_ok(Character *keeper, Character *ch, int shop_nr)
{
  char buf[240];

  // Undesirables.
  // TODO - Figure out if KILLER does anything we want to kill...
  // If not, let's use the AFF bit for something useful....
  if (ISSET(ch->affected_by, AFF_KILLER))
  {
    do_say(keeper, "Go away before I call the guards!!");
    sprintf(buf, "%s the KILLER is over here!\n\r", GET_SHORT(ch));
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
int trade_with(class Object *item, int shop_nr)
{
  int counter;

  if (item->obj_flags.cost < 1)
    return false;

  for (const auto &type : DC::getInstance()->shop_index[shop_nr].type)
  {
    if (GET_ITEM_TYPE(item) == type)
      return true;
  }

  return false;
}

int unlimited_supply(class Object *item, int shop_nr)
{
  class Object *obj;

  for (obj = DC::getInstance()->shop_index[shop_nr].inventory; obj; obj = obj->next_content)
  {
    if (item->item_number == obj->item_number)
      return true;
  }

  return false;
}

void restock_keeper(Character *keeper, int shop_nr)
{
  class Object *obj, *obj2;
  char buf[50];

  sprintf(buf, "Restocking shop keeper: %d", shop_nr);
  logentry(buf, OVERSEER, DC::LogChannel::LOG_MISC);

  for (obj = DC::getInstance()->shop_index[shop_nr].inventory; obj; obj = obj->next_content)
  {
    obj2 = clone_object(obj->item_number);
    obj_to_char(obj2, keeper);
  }
}

/*
 * Buy an item from a shop.
 */
void shopping_buy(const char *arg, Character *ch,
                  Character *keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH];
  char argm[MAX_INPUT_LENGTH + 1];
  class Object *obj;
  uint32_t cost;

  if (!is_ok(keeper, ch, shop_nr))
    return;

  one_argument(arg, argm);
  if (*argm == '\0')
  {
    sprintf(buf, "%s What do you want to buy?", GET_NAME(ch));
    keeper->do_tell(QString(buf).split(' '));
    return;
  }

  if (!IS_NPC(ch) && ch->isPlayerGoldThief())
  {
    ch->sendln("Your criminal acts prohibit it.");
    return;
  }

  auto &shop = DC::getInstance()->shop_index[shop_nr];
  if ((obj = get_obj_in_list_vis(ch, argm, keeper->carrying)) == nullptr)
  {
    keeper->do_tell(shop.no_such_item1.arg(ch->getName()).split(' '));
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
    keeper->do_tell(shop.no_such_item1.arg(GET_NAME(ch)).split(' '));
    return;
  }

  cost = (int)(obj->obj_flags.cost * shop.profit_buy);

  if (cost < 1)
    cost = 1;

  if (ch->getGold() < cost)
  {
    keeper->do_tell(shop.missing_cash2.arg(GET_NAME(ch)).split(' '));
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
  keeper->do_tell(shop.message_buy.arg(GET_NAME(ch)).arg(QString::number(cost)).split(' '));
  ch->send(QStringLiteral("You now have %1.\r\n").arg(obj->short_description));
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

  return;
}

/*
 * Sell an item to a shop keeper.
 */
void shopping_sell(const char *arg, Character *ch,
                   Character *keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH];
  char argm[MAX_INPUT_LENGTH + 1];
  class Object *obj;
  uint32_t cost;

  if (!is_ok(keeper, ch, shop_nr))
    return;

  one_argument(arg, argm);

  if (*argm == '\0')
  {
    keeper->do_tell(QStringLiteral("%1 What do you want to sell?").arg(GET_NAME(ch)).split(' '));
    return;
  }

  if (!IS_NPC(ch) && ch->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
  {
    ch->sendln("Your criminal acts prohibit it.");
    return;
  }

  if ((obj = get_obj_in_list_vis(ch, argm, ch->carrying)) == nullptr)
  {
    keeper->do_tell(DC::getInstance()->shop_index[shop_nr].no_such_item2.arg(GET_NAME(ch)).split(' '));
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
    keeper->do_tell(DC::getInstance()->shop_index[shop_nr].do_not_buy.arg(GET_NAME(ch)).split(' '));
    return;
  }

  int virt = DC::getInstance()->obj_index[obj->item_number].virt;
  if (virt >= 13400 && virt <= 13707 &&
      DC::getInstance()->mob_index[keeper->mobdata->nr].virt != 13416)
  {
    keeper->do_tell(QStringLiteral("%1 There is only one merchant in the land that deals with such fine jewels.").arg(GET_NAME(ch)).split(' '));
    return;
  }

  // don't allow non-empty containers to be sold
  if (obj->obj_flags.type_flag == ITEM_CONTAINER && obj->contains)
  {
    keeper->do_tell(QStringLiteral("%1 %2$B$2 needs to be emptied first.").arg(GET_NAME(ch)).arg(GET_OBJ_SHORT(obj)).split(' '));
    return;
  }

  cost = (int)(obj->obj_flags.cost * DC::getInstance()->shop_index[shop_nr].profit_sell);
  if (keeper->getGold() < cost)
  {
    keeper->do_tell(DC::getInstance()->shop_index[shop_nr].missing_cash1.arg(GET_NAME(ch)).split(' '));
    return;
  }

  act("$n sells $p.", ch, obj, 0, TO_ROOM, 0);
  keeper->do_tell(DC::getInstance()->shop_index[shop_nr].message_sell.arg(GET_NAME(ch)).arg(QString::number(cost)).split(' '));
  ch->send(QStringLiteral("The shopkeeper now has %1.\r\n").arg(obj->short_description));
  ch->addGold(cost);
  keeper->removeGold(cost);

  strcpy(argm, qPrintable(obj->Name()));

  if (get_obj_in_list(argm, keeper->carrying) || GET_ITEM_TYPE(obj) == ITEM_TRASH || unlimited_supply(obj, shop_nr))
  {
    extract_obj(obj);
  }
  else
    move_obj(obj, keeper);

  ch->save(cmd_t::SAVE_SILENTLY);

  return;
}

/*
 * Value an item.
 */
void shopping_value(const char *arg, Character *ch,
                    Character *keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  char argm[MAX_INPUT_LENGTH + 1];
  class Object *obj;
  int cost;
  bool keeperhas = false;

  if (!is_ok(keeper, ch, shop_nr))
    return;

  one_argument(arg, argm);

  if (*argm == '\0')
  {
    keeper->do_tell(QStringLiteral("%1 What do you want to value?").arg(GET_NAME(ch)).split(' '));
    return;
  }

  if ((obj = get_obj_in_list_vis(ch, argm, ch->carrying)) == nullptr)
  {
    if ((obj = get_obj_in_list_vis(keeper, argm, keeper->carrying)) != nullptr)
      keeperhas = true;
    else
    {
      keeper->do_tell(DC::getInstance()->shop_index[shop_nr].no_such_item2.arg(GET_NAME(ch)).split(' '));
      return;
    }
  }

  if (DC::getInstance()->mob_index[keeper->mobdata->nr].virt == 3003)
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
      do_emote(keeper, "looks carefully at the item.");
    }
    if (GET_ITEM_TYPE(obj) == ITEM_WEAPON)
    {
      if (obj->obj_flags.eq_level < 20)
      {
        sprintf(buf, "Well, %s is able to be used by ", obj->short_description);
        sprintbit(obj->obj_flags.size, Object::size_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        sprintf(buf, "and it can be wielded by these classes: ");
        sprintbit(obj->obj_flags.extra_flags, Object::extra_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        sprintf(buf, "The minimum level necessary to use it is %d.", obj->obj_flags.eq_level);
        do_say(keeper, buf);
        sprintf(buf, "The damage dice are '%dD%d'", obj->obj_flags.value[1], obj->obj_flags.value[2]);
        do_say(keeper, buf);
        for (int i = 0; i < obj->num_affects; i++)
        {
          if (obj->affected[i].location == APPLY_HITROLL && obj->affected[i].modifier != 0)
          {
            sprintf(buf, "It increases your hit roll by %d.", obj->affected[i].modifier);
            do_say(keeper, buf);
          }
          if (obj->affected[i].location == APPLY_DAMROLL && obj->affected[i].modifier != 0)
          {
            sprintf(buf, "It increases your damage by %d.", obj->affected[i].modifier);
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
  if (DC::getInstance()->mob_index[keeper->mobdata->nr].virt == 3004)
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
      do_emote(keeper, "looks carefully at the item.");
    }
    if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
    {
      if (obj->obj_flags.eq_level < 20)
      {
        sprintf(buf, "Ah yes, %s can be worn by ", obj->short_description);
        sprintbit(obj->obj_flags.size, Object::size_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        sprintf(buf, "and it can be worn by these classes: ");
        sprintbit(obj->obj_flags.extra_flags, Object::extra_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        sprintf(buf, "The minimum level necessary to use it is %d.", obj->obj_flags.eq_level);
        do_say(keeper, buf);
        for (int i = 0; i < obj->num_affects; i++)
        {
          if (obj->affected[i].location == APPLY_AC && obj->affected[i].modifier != 0)
          {
            sprintf(buf, "Your armor class will change by %d.", obj->affected[i].modifier);
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
  if (DC::getInstance()->mob_index[keeper->mobdata->nr].virt == 3000)
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
        sprintf(buf, "Excellent, %s has been imbued with energies of the %dth level.", obj->short_description, obj->obj_flags.value[0]);
        do_say(keeper, buf);
        if (GET_ITEM_TYPE(obj) == ITEM_WAND || GET_ITEM_TYPE(obj) == ITEM_STAFF)
        {
          if (obj->obj_flags.value[3] >= 1)
          {
            sprintf(buf, "It is eminating the aura of ");
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
            sprintf(buf, "I can easily identify the signatures of ");
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

  if (DC::getInstance()->mob_index[keeper->mobdata->nr].virt == 3010 && keeperhas)
  { // if the leather worker in town
    act("The Leather Worker holds up $p for you to examine.", ch, obj, 0, TO_CHAR, 0);
    act("The Leather Worker holds up $p for $n to examine.", ch, obj, 0, TO_ROOM, 0);
    if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
    {
      if (obj->obj_flags.eq_level < 20)
      {
        sprintf(buf, "Ah yes, %s can be worn by ", obj->short_description);
        sprintbit(obj->obj_flags.size, Object::size_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        sprintf(buf, "and it can be worn by these classes: ");
        sprintbit(obj->obj_flags.extra_flags, Object::extra_bits, buf2);
        strcat(buf, buf2);
        do_say(keeper, buf);
        sprintf(buf, "The minimum level necessary to use it is %d.", obj->obj_flags.eq_level);
        do_say(keeper, buf);
        for (int i = 0; i < obj->num_affects; i++)
        {
          if (obj->affected[i].location == APPLY_AC && obj->affected[i].modifier != 0)
          {
            sprintf(buf, "Your armor class will change by %d.", obj->affected[i].modifier);
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
    keeper->do_tell(DC::getInstance()->shop_index[shop_nr].do_not_buy.arg(GET_NAME(ch)).split(' '));
    return;
  }
  if (!keeperhas)
  {
    cost = (int)(obj->obj_flags.cost * DC::getInstance()->shop_index[shop_nr].profit_sell);
    keeper->do_tell(QStringLiteral("%1 I'll give you %2 gold coins for that.").arg(GET_NAME(ch)).arg(QString::number(cost)).split(' '));
  }
  return;
}

/*
 * List available items.
 */
void shopping_list(const char *arg, Character *ch,
                   Character *keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH];
  class Object *obj, *tobj;
  int cost;
  //    extern char *drinks[];
  int found;
  int done[100]; // To show 'em numbered instead of a long list of duplicates
  int i, a;
  if (!is_ok(keeper, ch, shop_nr))
    return;

  if (!keeper->carrying && DC::getInstance()->shop_index[shop_nr].inventory)
  {
    keeper->do_tell(QStringLiteral("%1 Oops, I seem to be out of inventory.").arg(GET_NAME(ch)).split(' '));
    keeper->do_tell(QStringLiteral("%1 One minute while I restock.").arg(GET_NAME(ch)).split(' '));
    restock_keeper(keeper, shop_nr);
  }
  i = 0;
  ch->sendln("[Amt] [ Price ] [ VNUM ] Item");
  found = false;
  vnum_t first_vnum{};
  for (obj = keeper->carrying; obj; obj = obj->next_content)
  {
    if (!CAN_SEE_OBJ(ch, obj) || obj->obj_flags.cost <= 0)
      continue;

    found = true;

    cost = (int)(obj->obj_flags.cost * DC::getInstance()->shop_index[shop_nr].profit_buy);

    int vnum = DC::getInstance()->obj_index[obj->item_number].virt;
    bool loop = false;
    for (a = 0; a < i; a++)
      if (done[a] == vnum)
        loop = true;
    if (loop)
      continue;
    if (i < 100)
      done[i++] = DC::getInstance()->obj_index[obj->item_number].virt;
    else
      break;
    a = 0;
    for (tobj = keeper->carrying; tobj; tobj = tobj->next_content)
      if (DC::getInstance()->obj_index[tobj->item_number].virt == DC::getInstance()->obj_index[obj->item_number].virt)
        a++;
    /*        if ( GET_ITEM_TYPE(obj) == ITEM_DRINKCON && obj->obj_flags.value[1] )
            {
                sprintf( buf, "[%3d] [%7d] %s of %s.\r\n",
                    a, cost, obj->short_description,
                    drinks[obj->obj_flags.value[2]] );
            }
            else
            {*/

    //        }
    if (!first_vnum || first_vnum == DC::INVALID_VNUM)
    {
      first_vnum = DC::getInstance()->getObjectVNUM(obj);
    }
    ch->sendln(QStringLiteral("[%1] [%2] [%3] %4.").arg(QString::number(a), 3).arg(QString::number(cost), 7).arg(QString::number(DC::getInstance()->getObjectVNUM(obj)), 6).arg(obj->short_description));
  }
  if (first_vnum && first_vnum != DC::INVALID_VNUM)
  {
    ch->sendln(QStringLiteral("Type 'identify vVNUM' for details about a specific object. Example: identify v%1").arg(QString::number(first_vnum)));
  }

  if (!found)
    ch->sendln("You can't buy anything here!");

  return;
}

// Spec proc for shop keepers.
// TODO - Remove goto's from this....I hate goto's.  This is C, not BASIC....
int shop_keeper(Character *ch, class Object *obj, cmd_t cmd, const char *arg, Character *invoker)
{
  Character *keeper;
  int shop_nr;

  /*
   * Find a shop keeper in the room.
   */
  //    for ( keeper = DC::getInstance()->world[ch->in_room].people;
  //        keeper != nullptr;
  //        keeper = keeper->next_in_room )
  //    {
  //        if ( IS_NPC(keeper) && DC::getInstance()->mob_index[keeper->mobdata->nr].non_combat_func == shop_keeper )
  //            goto LFound1;
  //    }

  // This (using original log from above stuff) should take care of finding the keeper
  // instead of looping through.  Should allow for multiple keepers too:)
  if (!(keeper = invoker))
  {
    logentry(QStringLiteral("Shop_keeper: keeper not found."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  // LFound1:
  for (shop_nr = 0; shop_nr < max_shop; shop_nr++)
  {
    if (DC::getInstance()->shop_index[shop_nr].keeper == keeper->mobdata->nr)
      goto LFound2;
  }
  logentry(QStringLiteral("Shop_keeper: shop_nr not found."), ANGEL, DC::LogChannel::LOG_BUG);
  return eFAILURE;

LFound2:
  //    if ( ch->in_room != DC::getInstance()->shop_index[shop_nr].in_room )
  //      return eFAILURE;

  switch (cmd)
  {
  default:
    return eFAILURE;
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

  return eSUCCESS;
}

void boot_the_shops()
{
  char *buf;
  int temp;
  int count;
  FILE *fp;

  if ((fp = fopen(SHOP_FILE, "r")) == nullptr)
  {
    perror(SHOP_FILE);
    exit(1);
  }

  max_shop = 0;

  for (;;)
  {
    buf = fread_string(fp, 0);
    if (*buf == '$')
    {
      dc_free(buf);
      break;
    }
    if (*buf != '#')
    {
      dc_free(buf);
      continue;
    }

    // we don't seem to use buff after this point, so I'm going to free it
    // otherise, we're leaking memory
    dc_free(buf);

    if (max_shop >= MAX_SHOP)
    {
      perror("Too many shops.\n");
      exit(1);
    }

    /*
     * Ignore "producing" list.
     */
    for (count = 0; count < 6; count++)
      fscanf(fp, "%d \n", &temp);

    fscanf(fp, "%f \n", &DC::getInstance()->shop_index[max_shop].profit_buy_base);
    fscanf(fp, "%f \n", &DC::getInstance()->shop_index[max_shop].profit_sell);
    for (count = 0; count < MAX_TRADE; count++)
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

    int room_nr = real_room(temp);
    if (room_nr < 0 || room_nr > DC::getInstance()->top_of_world)
    {
      logf(100, DC::LogChannel::LOG_BUG, "shopkeeper %d loaded with in_room set to %d. Setting to 0.", max_shop, room_nr);
      room_nr = 0;
    }

    DC::getInstance()->shop_index[max_shop].in_room = room_nr;

    fscanf(fp, "%d \n", &DC::getInstance()->shop_index[max_shop].open1);
    fscanf(fp, "%d \n", &DC::getInstance()->shop_index[max_shop].close1);
    fscanf(fp, "%d \n", &DC::getInstance()->shop_index[max_shop].open2);
    fscanf(fp, "%d \n", &DC::getInstance()->shop_index[max_shop].close2);

    DC::getInstance()->shop_index[max_shop].inventory = 0;

    if (real_room(temp) == DC::NOWHERE)
    {
      char log_buf[MAX_STRING_LENGTH] = {};
      sprintf(log_buf, "BAD SHOP IN missing ROOM %d -- FIX THIS!", temp);
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
  int shop_nr;

  for (shop_nr = 0; shop_nr < max_shop; shop_nr++)
    DC::getInstance()->mob_index[DC::getInstance()->shop_index[shop_nr].keeper].non_combat_func = shop_keeper;

  return;
}

void fix_shopkeepers_inventory()
{
  int shop_nr;

  Character *keeper = 0;
  class Object *obj, *last_obj, *cloned;

  // set up the unlimited supply items. Those the shop_keeper has on start up.

  for (shop_nr = 0; shop_nr < max_shop; shop_nr++)
    for (keeper = DC::getInstance()->world[DC::getInstance()->shop_index[shop_nr].in_room].people; keeper != nullptr;
         keeper = keeper->next_in_room)
    {
      if (IS_NPC(keeper) && DC::getInstance()->mob_index[keeper->mobdata->nr].non_combat_func == shop_keeper)
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
          DC::getInstance()->shop_index[shop_nr].inventory = 0;
      }
    }

  return;
}

// return nullptr for failure
// return pointer to new shop on success
player_shop *read_one_player_shop(FILE *fp)
{
  int32_t count;
  char code[4];

  player_shop_item *item = nullptr;
  player_shop *shop = (player_shop *)dc_alloc(1, sizeof(player_shop));

  fread(&shop->owner, sizeof(char), PC_SHOP_OWNER_SIZE, fp);
  fread(&shop->room_num, sizeof(int32_t), 1, fp);
  fread(&shop->sell_message, sizeof(char), PC_SHOP_SELL_MESS_SIZE, fp);
  fread(&shop->money_on_hand, sizeof(int32_t), 1, fp);

  code[3] = '\0';
  fread(&code, sizeof(char), 3, fp);

  while (strcmp(code, "END"))
  {
    // add future stuff here

    logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Illegal code in player shop %s", shop->owner);
    logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Illegal code in player shop %s", shop->owner);
    exit(1);
  }

  fread(&count, sizeof(int32_t), 1, fp);

  shop->sale_list = nullptr;
  for (int i = 0; i < count; i++)
  {
    item = (player_shop_item *)dc_alloc(1, sizeof(player_shop_item));

    fread(&item->item_vnum, sizeof(int), 1, fp);
    fread(&item->price, sizeof(int), 1, fp);
    fread(&code, sizeof(char), 3, fp);
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
  char buf[80];
  int32_t count = 0;

  sprintf(buf, "%s/%s", PLAYER_SHOP_DIR, shop->owner);

  if ((fp = fopen(buf, "w")) == nullptr)
  {
    logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Could not open %s for writing.", buf);
    return;
  }

  fwrite(&(shop->owner), sizeof(char), PC_SHOP_OWNER_SIZE, fp);
  fwrite(&(shop->room_num), sizeof(int32_t), 1, fp);
  fwrite(&(shop->sell_message), sizeof(char), PC_SHOP_SELL_MESS_SIZE, fp);
  fwrite(&(shop->money_on_hand), sizeof(int32_t), 1, fp);

  // add stuff later here with 3 digit code
  // end of variable data
  fwrite("END", sizeof(char), 3, fp);

  for (item = shop->sale_list; item; item = item->next)
    count++;

  fwrite(&(count), sizeof(int32_t), 1, fp);

  for (item = shop->sale_list; item; item = item->next)
  {
    fwrite(&(item->item_vnum), sizeof(int), 1, fp);
    fwrite(&(item->price), sizeof(int), 1, fp);
    fwrite("END", sizeof(char), 3, fp);
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
  char buf[180];

  curr = DC::getInstance()->world_file_list;
  while (curr && curr->firstnum != 23000)
    curr = curr->next;

  if (!curr)
  {
    // panic!
    logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Could not find player shop range to save files.");
    exit(1);
  }

  LegacyFile lf("world", curr->filename, "Couldn't open room save file %1 for player shops.");
  if (lf.isOpen())
  {
    for (int x = curr->firstnum; x <= curr->lastnum; x++)
    {
      write_one_room(lf, x);
    }
    fprintf(lf.file_handle_, "$~\n");
  }
}

void boot_player_shops()
{
  FILE *fp;
  FILE *shopfp;
  player_shop *shop;
  char *filename;
  char buf[80];

  g_playershops = nullptr;

  if ((fp = fopen(PLAYER_SHOP_INDEX, "r")) == nullptr)
  {
    perror(PLAYER_SHOP_INDEX);
    exit(1);
  }

  // read list of player owned shops

  filename = fread_string(fp, 0);
  while (strcmp(filename, "$"))
  {
    sprintf(buf, "%s/%s", PLAYER_SHOP_DIR, filename);
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

player_shop *find_player_shop(Character *keeper)
{
  player_shop *shop = g_playershops;

  for (; shop; shop = shop->next)
    if (real_room(shop->room_num) == keeper->in_room)
      break;

  return shop;
}

// put an item up for sale
void player_shopping_stock(const char *arg, Character *ch, Character *keeper)
{
  player_shop *shop = find_player_shop(keeper);
  if (!shop)
  {
    ch->sendln("Invalid player shop keeper.  Let a god know.");
    return;
  }

  if (strcmp(shop->owner, GET_NAME(ch)))
  {
    ch->sendln("You don't own this shop, you can't stock the shelves!");
    return;
  }

  char item[MAX_INPUT_LENGTH];
  char price[MAX_INPUT_LENGTH];

  half_chop(arg, item, price);

  if (!*item || !*price)
  {
    ch->sendln("Syntax:  stock <item> <price>");
    return;
  }

  int32_t value;
  value = atol(price);
  if (value < 1 || value > 20000000)
  {
    ch->sendln("Invalid price.  The price must be between 1 $B$5gold$R and 20 million $B$5gold$R.");
    return;
  }

  // find item
  Object *obj;

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
  player_shop_item *newitem = (player_shop_item *)dc_alloc(1, sizeof(player_shop_item));
  newitem->item_vnum = DC::getInstance()->obj_index[obj->item_number].virt;
  newitem->price = value;
  newitem->next = shop->sale_list;
  shop->sale_list = newitem;
  extract_obj(obj);
  ch->sendln("You put the item up for sale.");
  write_one_player_shop(shop);
}

void player_shopping_buy(const char *arg, Character *ch, Character *keeper)
{
  player_shop *shop = find_player_shop(keeper);
  if (!shop)
  {
    ch->sendln("Invalid player shop keeper.  Let a god know.");
    return;
  }

  char buf[MAX_INPUT_LENGTH];

  one_argument(arg, buf);
  if (!*buf)
  {
    keeper->do_tell(QStringLiteral("%1 Which do you want to buy?").arg(GET_NAME(ch)).split(' '));
    return;
  }

  int item_pos = atoi(buf);
  player_shop_item *item = shop->sale_list;
  for (int j = 1; (item && j < item_pos); item = item->next, j++)
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

  int robj = real_object(item->item_vnum);
  if (robj < 0)
  {
    ch->sendln("Error, that is not a valid item.  Let a god know.");
    return;
  }

  // give it to them, thank them, take the money
  Object *obj = clone_object(robj);
  obj_to_char(obj, ch);
  ch->removeGold(item->price);
  shop->money_on_hand += item->price;

  if (!shop->sell_message || !*shop->sell_message)
  {
    keeper->do_tell(QStringLiteral("%1 Thank you, come again!").arg(GET_NAME(ch)).split(' '));
  }
  else
  {
    keeper->do_tell(QStringLiteral("%1 %2").arg(GET_NAME(ch)).arg(shop->sell_message).split(' '));
  }

  // update inventory
  for (player_shop_item *curr = shop->sale_list; curr; curr = curr->next)
  {
    if (curr == item)
    { // first item
      shop->sale_list = curr->next;
      dc_free(curr);
      break;
    }
    if (curr->next == item)
    {
      curr->next = item->next;
      dc_free(item);
      break;
    }
  }

  // save the shop
  write_one_player_shop(shop);
}

void player_shopping_withdraw(const char *arg, Character *ch, Character *keeper)
{
  player_shop *shop = find_player_shop(keeper);
  if (!shop)
  {
    ch->sendln("Invalid player shop keeper.  Let a god know.");
    return;
  }

  if (strcmp(shop->owner, GET_NAME(ch)))
  {
    ch->sendln("You don't own this shop!  Go rob a bank or something.");
    return;
  }

  char price[MAX_INPUT_LENGTH];
  one_argument(arg, price);

  if (!*price)
  {
    ch->sendln("Withdraw how much from your store?");
    return;
  }

  int32_t value;
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

void player_shopping_design(const char *arg, Character *ch, Character *keeper)
{
  char select[MAX_INPUT_LENGTH];
  char text[MAX_INPUT_LENGTH];
  int16_t skill;

  if (IS_NPC(ch))
    return;

  const char *pdesign_values[] = {
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

  if (strcmp(shop->owner, GET_NAME(ch)))
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

  for (skill = 0;; skill++)
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
    if (!strcmp(text, "none"))
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
    dc_free(DC::getInstance()->world[shop->room_num].name);
    DC::getInstance()->world[shop->room_num].name = str_dup(text);
    csendf(ch, "Room name set to '%s'.\r\n", DC::getInstance()->world[shop->room_num].name);
    save_player_shop_world_range();
    break;

  case 2: // roomdesc
    ch->sendln("Not active yet.");
    break;

  default:
    send_to_char("$3Usage$R: design <field> <correct arguments>\r\n"
                 "Fields are the following.\r\n",
                 ch);
    ch->display_string_list(pdesign_values);
    break;
  }
}

void player_shopping_sell(const char *arg, Character *ch, Character *keeper)
{
  ch->sendln("These shop keeper's don't buy stuff.");
}

void player_shopping_value(const char *arg, Character *ch, Character *keeper)
{
  ch->sendln("These shop keeper's don't buy stuff.");
}

void player_shopping_list(const char *arg, Character *ch, Character *keeper)
{
  int count = 0;
  int robj;
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
        ch->send(QStringLiteral("%1$3)$R %2 %3\r\n").arg(QString::number(count), -3).arg(((Object *)DC::getInstance()->obj_index[robj].item)->short_description, -40).arg(QString::number(item->price)));
    }

  if (!strcmp(shop->owner, GET_NAME(ch)))
    ch->send(QStringLiteral("\r\nYour shop has %1 cash in the till.\r\n").arg(QString::number(shop->money_on_hand)));
}

int player_shop_keeper(Character *ch, class Object *obj, cmd_t cmd, const char *arg, Character *invoker)
{
  Character *keeper;

  if (!(keeper = invoker))
  {
    logentry(QStringLiteral("Shop_keeper: keeper not found."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (IS_NPC(ch))
    return eFAILURE;

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
    return eFAILURE;
  }

  return eSUCCESS;
}
/*
int do_pshopedit(Character * ch, char * arg, cmd_t cmd)
{
  char buf[MAX_STRING_LENGTH];
  char select[MAX_INPUT_LENGTH];
  char text[MAX_INPUT_LENGTH];
  int16_t skill, i;
  player_shop * shop;

  if(IS_NPC(ch))
    return eFAILURE;

  if(!ch->has_skill( COMMAND_PSHOPEDIT)) {
        ch->sendln("Huh?");
        return eFAILURE;
  }

  char * pshopedit_values [] = {
    "new",
    "delete",
    "list",
    "\n"
  };

  arg = one_argumentnolow(arg, select);

  if(!(*select)) {
    send_to_char("$3Usage$R: pshopedit <field> <correct arguments>\r\n"
                 "Fields are the following.\r\n"
                 , ch);
    display_string_list(pshopedit_values, ch);
    return eFAILURE;
  }

  for(skill = 0 ;; skill++)
  {
    if(pshopedit_values[skill][0] == '\n')
    {
      ch->sendln("Invalid field.");
      return eFAILURE;
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
         return eFAILURE;
      }
      i = atoi(text);
      if(i < 1 || i > DC::getInstance()->top_of_world || !DC::getInstance()->rooms[i]) {
         ch->sendln("You must choose a valid room number.");
         return eFAILURE;
      }
      shop = (player_shop *)dc_alloc(1, sizeof(player_shop));
      strcpy(shop->owner, buf);
      *shop->sell_message = '\0';
      shop->room_num = i;
      shop->money_on_hand = 0;
      shop->sale_list = nullptr;
      shop->next = g_playershops;
      g_playershops = shop;
      save_shop_list();
      write_one_player_shop(shop);
      csendf(ch, "Shop created for player '%s' in room %d.\r\n", shop->owner, shop->room_num);
      break;

    case 1: // delete

      save_shop_list();
      break;

    case 2: // list
      ch->sendln("Player Shops\r\n-------------------------");
      if(!g_playershops)
         ch->sendln("No current shops.");
      else for(i = 1, shop = g_playershops; shop; shop = shop->next, i++)
        csendf(ch, "%2d$3)$R %-15s $3Room$R: %5d\r\n", i, shop->owner, shop->room_num);
      break;

    default:
      send_to_char("$3Usage$R: pshopedit <field> <correct arguments>\r\n"
                 "Fields are the following.\r\n"
                 , ch);
      display_string_list(pshopedit_values, ch);
      break;
  }

  return eSUCCESS;
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
    for (int i = 0; i < 58; i++)
    {
      DC::getInstance()->shop_index[i].profit_buy = DC::getInstance()->shop_index[i].profit_buy_base;
    }
    break;
  case 2:
    for (int i = 0; i < 58; i++)
      DC::getInstance()->shop_index[i].profit_buy *= 1.0 + number(10, 50) / 100.0;
    break;
  case 3:
    for (int i = 0; i < 58; i++)
    {
      DC::getInstance()->shop_index[i].profit_buy *= 1.0 - number(10, 50) / 100.0;
      DC::getInstance()->shop_index[i].profit_buy = MAX(DC::getInstance()->shop_index[i].profit_buy, DC::getInstance()->shop_index[i].profit_sell + 0.1);
    }
    break;
  default:
    break;
  }
}

struct obj_exchange
{
  int item_qty;
  int item_vnum;
  int cost_qty;
  int cost_vnum;
  uint64_t cost_exp;
  uint64_t cost_plats;
};

const int OBJ_APOCALYPSE = 27905;
const int OBJ_BROWNIE = 27906;
const int OBJ_MEATBALL = 27908;
const int OBJ_WINGDING = 27909;
const int OBJ_CLOVERLEAF = 27910;

const int MAX_EDDIE_ITEMS = 16;

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

int eddie_shopkeeper(Character *ch, class Object *obj, cmd_t cmd, const char *arg, Character *owner)
{
  if (cmd != cmd_t::LIST && cmd != cmd_t::BUY)
    return eFAILURE;

  if (IS_AFFECTED(ch, AFF_BLIND))
  {
    ch->send("You're too blind to do that!\r\n");
    return eFAILURE;
  }

  if (IS_NPC(ch))
    return eFAILURE;

  if (cmd == cmd_t::LIST)
  {
    csendf(ch, "$B$2%s tells you, 'This is what I can do for you...\n\r$R", GET_SHORT(owner));
    ch->sendln("  | Item                              | Cost                                   |");
    ch->sendln("--------------------------------------------------------------------------------");
    int last_vnum = 0;
    for (int i = 0; i < MAX_EDDIE_ITEMS; i++)
    {
      char buf[1024] = {};
      char item_buf[1024] = {};
      char cost_buf[1024] = {};
      if (eddie[i].item_vnum > 0)
      {
        strncpy(item_buf, ((Object *)DC::getInstance()->obj_index[real_object(eddie[i].item_vnum)].item)->short_description, 1024);
      }
      else
      {
        continue;
      }

      if (eddie[i].cost_vnum > 0)
      {
        strncpy(cost_buf, ((Object *)DC::getInstance()->obj_index[real_object(eddie[i].cost_vnum)].item)->short_description, 1024);
      }
      else if (eddie[i].cost_exp > 0)
      {
        std::string cost_buf_str = fmt::format(std::locale("en_US.UTF-8"), "{:L} experience", eddie[i].cost_exp);
        snprintf(cost_buf, 1024, "%s", cost_buf_str.c_str());
      }
      else if (eddie[i].cost_plats > 0)
      {
        std::string cost_buf_str = fmt::format(std::locale("en_US.UTF-8"), "{:L} platinum", eddie[i].cost_plats);
        snprintf(cost_buf, 1024, "%s", cost_buf_str.c_str());
      }

      if (last_vnum != 0 && last_vnum != eddie[i].item_vnum)
      {
        ch->sendln("--------------------------------------------------------------------------------");
      }

      last_vnum = eddie[i].item_vnum;
      int item_qty = eddie[i].item_qty;
      int cost_qty = eddie[i].cost_qty;
      uint64_t cost_plats = eddie[i].cost_plats;

      // setup format specifier based length of item short descriptions
      if (cost_qty > 0)
      {
        snprintf(buf, 1024, "$B$3%%2d$R|%%2d x %%-%ds|%%2d x %%-%ds|\r\n",
                 30 + (strlen(item_buf) - nocolor_strlen(item_buf)),
                 35 + (strlen(cost_buf) - nocolor_strlen(cost_buf)));
        csendf(ch, buf, i + 1, item_qty, item_buf, cost_qty, cost_buf);
      }
      else if (cost_plats > 0)
      {
        snprintf(buf, 1024, "$B$3%%2d$R|%%2d x %%-%ds| $B%%-%ds$R    |\r\n",
                 30 + (strlen(item_buf) - nocolor_strlen(item_buf)),
                 35 + (strlen(cost_buf) - nocolor_strlen(cost_buf)));
        csendf(ch, buf, i + 1, item_qty, item_buf, cost_buf);
      }
      else
      {
        snprintf(buf, 1024, "$B$3%%2d$R|%%2d x %%-%ds| %%-%ds    |\r\n",
                 30 + (strlen(item_buf) - nocolor_strlen(item_buf)),
                 35 + (strlen(cost_buf) - nocolor_strlen(cost_buf)));
        csendf(ch, buf, i + 1, item_qty, item_buf, cost_buf);
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
    return eSUCCESS;
  }
  else if (cmd == cmd_t::BUY)
  {
    char arg1[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    one_argument(arg, arg1);

    if (*arg1 == 0)
    {
      ch->sendln("Buy what?");
      return eSUCCESS;
    }

    int choice = atoi(arg1);
    if (choice < 1 || choice > MAX_EDDIE_ITEMS)
    {
      ch->send(QStringLiteral("Invalid number. Choose between 1 and %1.\r\n").arg(QString::number(MAX_EDDIE_ITEMS)));
      return eSUCCESS;
    }

    if ((eddie[choice - 1].cost_qty == 0 || eddie[choice - 1].cost_vnum < 1) && eddie[choice - 1].cost_exp > 0)
    {
      if (GET_EXP(ch) >= eddie[choice - 1].cost_exp)
      {
        GET_EXP(ch) -= eddie[choice - 1].cost_exp;
        ch->send(fmt::format(std::locale("en_US.UTF-8"), "Eddie takes {:L} experience from you, leaving you with {:L} experience.\r\n", eddie[choice - 1].cost_exp, GET_EXP(ch)));
      }
      else
      {
        ch->send(fmt::format(std::locale("en_US.UTF-8"), "You do not have the {:L} experience to pay for that item. You need {:L} more experience.\r\n", eddie[choice - 1].cost_exp, eddie[choice - 1].cost_exp - GET_EXP(ch)));
        return eSUCCESS;
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
        return eSUCCESS;
      }
    }
    else
    {

      int count = search_char_for_item_count(ch, real_object(eddie[choice - 1].cost_vnum), false);

      if (count < eddie[choice - 1].cost_qty)
      {
        ch->sendln("You don't have enough to trade.");
        return eSUCCESS;
      }

      for (int i = 0; i < eddie[choice - 1].cost_qty; i++)
      {
        Object *obj = search_char_for_item(ch, real_object(eddie[choice - 1].cost_vnum), false);
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

          sprintf(buf, "%s gives %s to %s (removed)", GET_NAME(ch), qPrintable(obj->Name()), GET_NAME(owner));
          logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
        }
        else
        {
          ch->sendln("An error occured.");
          ch->save(cmd_t::SAVE_SILENTLY);
          return eSUCCESS;
        }
      }
    }

    for (int i = 0; i < eddie[choice - 1].item_qty; i++)
    {
      Object *item = clone_object(real_object(eddie[choice - 1].item_vnum));
      if (item != 0)
      {
        obj_to_char(item, ch);

        act("$n gives $p to $N.", owner, item, ch, TO_ROOM, INVIS_NULL | NOTVICT);
        act("$n gives you $p.", owner, item, ch, TO_VICT, 0);
        act("You give $p to $N.", owner, item, ch, TO_CHAR, 0);

        sprintf(buf, "%s gives %s to %s (created)", GET_NAME(owner), qPrintable(item->Name()), GET_NAME(ch));
        logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
      }
      else
      {
        ch->sendln("An error occured.");
        ch->save(cmd_t::SAVE_SILENTLY);
        return eSUCCESS;
      }
    }
    ch->save();
  }

  return eSUCCESS;
}

int reroll_trader(Character *ch, Object *obj, cmd_t cmd, const char *arg, Character *owner)
{
  if (ch == nullptr || IS_NPC(ch))
  {
    return eFAILURE;
  }

  std::string arg1, remainder_args;
  std::tie(arg1, remainder_args) = half_chop(arg);

  reroll_t r = {};
  if (reroll_sessions.contains(GET_NAME(ch)))
  {
    r = reroll_sessions[GET_NAME(ch)];
  }

  obj_list_t obj_list = {};
  switch (cmd)
  {
  case cmd_t::LIST:
    if (r.state == reroll_t::reroll_states_t::PICKED_OBJ_TO_REROLL)
    {
      owner->tell(ch, QStringLiteral("You need to confirm or cancel rerolling %1.").arg(GET_OBJ_SHORT(r.orig_obj)));
      return eSUCCESS;
    }

    owner->tell(ch, "Type reroll <object keyword> to reroll that object.");
    owner->tell(ch, "I will then ask you to confirm the object you want re-rolled.");
    owner->tell(ch, "The cost will be 1 Cloverleaf token.");
    owner->tell(ch, "You will get two re-rolled choices or the original to pick from.");
    owner->tell(ch, "Type choose 1, 2 or 3 to choose either one of the two rerolls or the original.");
    return eSUCCESS;
    break;

  case cmd_t::REROLL:
    if (r.state == reroll_t::reroll_states_t::PICKED_OBJ_TO_REROLL)
    {
      owner->tell(ch, QStringLiteral("You need to confirm or cancel rerolling %1.").arg(GET_OBJ_SHORT(r.orig_obj)));
      return eSUCCESS;
    }
    else if (r.state == reroll_t::reroll_states_t::REROLLED)
    {
      owner->tell(ch, "You need to choose 1, 2, 3 or type cancel before you can reroll again.");
      return eSUCCESS;
    }

    if (arg1.empty())
    {
      owner->tell(ch, "You have to type reroll <object keyword> to reroll that object.");
      return eSUCCESS;
    }
    else
    {
      obj = get_obj_in_list_vis(ch, arg1.c_str(), ch->carrying);
      if (obj == nullptr)
      {
        owner->tell(ch, QStringLiteral("I don't see anything on you matching \'%1\'").arg(arg1.c_str()));
        return eSUCCESS;
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
          return eSUCCESS;
        }

        if (obj->isGodload())
        {
          owner->tell(ch, "I can't reroll GL weapons or armor.");
          return eSUCCESS;
        }

        if (obj->isQuest())
        {
          owner->tell(ch, "I can't reroll quest weapons or armor.");
          return eSUCCESS;
        }

        if (obj->isCustom())
        {
          owner->tell(ch, "I can't reroll objects with the NO_CUSTOM flag set on them.");
          return eSUCCESS;
        }

        r = {};
        r.orig_obj = obj;
        r.orig_rnum = GET_OBJ_RNUM(obj);
        r.state = reroll_t::reroll_states_t::PICKED_OBJ_TO_REROLL;
        reroll_sessions[GET_NAME(ch)] = r;
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
        return eSUCCESS;
      }

      obj = search_char_for_item(ch, real_object(OBJ_CLOVERLEAF), false);
      if (obj != 0)
      {
        obj_from(obj);
        act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL | NOTVICT);
        act("$n gives you $p.", ch, obj, owner, TO_VICT, 0);
        act("You give $p to $N.", ch, obj, owner, TO_CHAR, 0);
        logentry(QStringLiteral("%1 gives %2 to %3").arg(GET_NAME(ch)).arg(obj->Name()).arg(GET_NAME(owner)), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
      }

      if (r.orig_obj != nullptr)
      {
        move_obj(r.orig_obj, owner);
        act("$n gives $p to $N.", ch, r.orig_obj, owner, TO_ROOM, INVIS_NULL | NOTVICT);
        act("$n gives you $p.", ch, r.orig_obj, owner, TO_VICT, 0);
        act("You give $p to $N.", ch, r.orig_obj, owner, TO_CHAR, 0);
        logentry(QStringLiteral("%1 gives %2 to %3").arg(GET_NAME(ch)).arg(r.orig_obj->Name()).arg(GET_NAME(owner)), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
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
        reroll_sessions[GET_NAME(ch)] = r;
      }
      owner->tell(ch, "Choice 3 is:");
      identify(ch, r.orig_obj);
      owner->tell(ch, "Type choose 1, 2 or 3.");
    }
    else
    {
      return eFAILURE;
    }

    break;

  case cmd_t::CHOOSE:
    if (r.state != reroll_t::reroll_states_t::REROLLED)
    {
      return eFAILURE;
    }

    if (arg1 == "1")
    {
      if (r.choice1_obj != nullptr)
      {
        move_obj(r.choice1_obj, ch);
        logentry(QStringLiteral("%1 gives %2 to %3").arg(GET_NAME(owner)).arg(r.choice1_obj->Name()).arg(GET_NAME(ch)), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
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
        logentry(QStringLiteral("%1 gives %2 to %3").arg(GET_NAME(owner)).arg(r.choice2_obj->Name()).arg(GET_NAME(ch)), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
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
        logentry(QStringLiteral("%1 gives %2 to %3").arg(GET_NAME(owner)).arg(r.orig_obj->Name()).arg(GET_NAME(ch)), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
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
      return eSUCCESS;
    }
    reroll_sessions.erase(GET_NAME(ch));
    break;

  case cmd_t::CANCEL:
    owner->tell(ch, "I'm canceling this reroll.");

    obj_from(r.choice1_obj);
    obj_from(r.choice2_obj);
    if (r.orig_obj != nullptr)
    {
      move_obj(r.orig_obj, ch);
      logentry(QStringLiteral("%1 gives %2 to %3").arg(GET_NAME(owner)).arg(r.orig_obj->Name()).arg(GET_NAME(ch)), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
      act("$n gives $p to $N.", owner, r.orig_obj, ch, TO_ROOM, INVIS_NULL | NOTVICT);
      act("$n gives you $p.", owner, r.orig_obj, ch, TO_VICT, 0);
      act("You give $p to $N.", owner, r.orig_obj, ch, TO_CHAR, 0);
    }
    else
    {
      ch->send("An error occurred. The object is missing.\r\n");
    }
    reroll_sessions.erase(GET_NAME(ch));
    break;

  default:
    return eFAILURE;
    break;
  }

  return eSUCCESS;
}

int redeem_trader(Character *ch, Object *obj, cmd_t cmd, const char *arg, Character *owner)
{
  switch (cmd)
  {
  case cmd_t::REDEEM:
  case cmd_t::LIST:
  case cmd_t::CANCEL:
  case cmd_t::CONFIRM:
    break;
  default:
    return eFAILURE;
  }

  if (!ch || !ch->isPlayer() || !arg)
  {
    return eFAILURE;
  }

  QStringList arguments = QString(arg).split(' ');
  QString arg1 = arguments.value(0);
  QString arg2 = arguments.value(1);
  QString arg3 = arguments.value(2);
  redeem_t r = DC::getInstance()->redeem_sessions.value(ch->getName());

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
      return eSUCCESS;
      break;
    default:
      return eFAILURE;
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
        return eSUCCESS;
      }
      else
      {
        if (GET_OBJ_TYPE(obj) != ITEM_WEAPON &&
            GET_OBJ_TYPE(obj) != ITEM_ARMOR &&
            GET_OBJ_TYPE(obj) != ITEM_INSTRUMENT &&
            GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
        {
          owner->tell(ch, "I can only redeem Apocalypse tokens for weapons, armor, instruments and containers.");
          return eSUCCESS;
        }

        if (obj->isGodload())
        {
          owner->tell(ch, "I can't redeem for GL items.");
          return eSUCCESS;
        }

        if (obj->isQuest())
        {
          owner->tell(ch, "I can't redeem for quest items.");
          return eSUCCESS;
        }

        if (obj->isCustom() && r.random)
        {
          owner->tell(ch, "I can't redeem randomized versions of items with NO_CUSTOM flag set.");
          return eSUCCESS;
        }

        if (obj->isTest())
        {
          owner->tell(ch, "You can't redeem for test items.");
          return eSUCCESS;
        }

        if (obj->getLevel() >= 60)
        {
          owner->tell(ch, "You can't redeem Apocalypse tokens for a level 60 item.");
          return eSUCCESS;
        }

        ch->do_identify(QStringLiteral("v%1").arg(DC::getInstance()->obj_index[obj->item_number].virt).split(' '));

        r.orig_obj = obj;
        r.orig_rnum = GET_OBJ_RNUM(obj);
        r.state = redeem_t::state_t::PICKED_OBJ_TO_REDEEM;
        if (obj->getLevel() >= 51)
          r.token_count = 6;
        else
          r.token_count = 3;
        DC::getInstance()->redeem_sessions[GET_NAME(ch)] = r;

        auto random_str = r.random ? "randomized" : "default";
        owner->tell(ch, QStringLiteral("Type 'confirm' if you want me to redeem %1 Apocalypse tokens for one %2 '%3'? Type "
                                       "'cancel' if not.")
                            .arg(QString::number(r.token_count))
                            .arg(random_str)
                            .arg(GET_OBJ_SHORT(obj)));
        return eSUCCESS;
      }
    }
    break;
  case redeem_t::state_t::PICKED_OBJ_TO_REDEEM:
    switch (cmd)
    {
    case cmd_t::REDEEM:
    case cmd_t::LIST:
      owner->tell(ch, QStringLiteral("You need to confirm or cancel redeeming %1.").arg(GET_OBJ_SHORT(r.orig_obj)));
      return eSUCCESS;
      break;
    case cmd_t::CONFIRM:
      break;
    }

    return eSUCCESS;
    break;
  case redeem_t::state_t::REDEEM:
    owner->tell(ch, "You need to choose 1, 2, 3 or type cancel before you can reroll again.");
    return eSUCCESS;
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

  return eFAILURE;
}