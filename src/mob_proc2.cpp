/***************************************************************************
 *  file: spec_pro2.c , Special module.                    Part of DIKUMUD *
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
/* $Id: mob_proc2.cpp,v 1.89 2012/05/25 02:15:46 jhhudso Exp $ */

#include "DC/DC.h"

extern qint32 class_restricted(CharacterPtr ch, ObjectPtr obj);
extern qint32 size_restricted(CharacterPtr ch, ObjectPtr obj);

void repair_shop_fix_eq(CharacterPtr ch, CharacterPtr owner, qint32 price, ObjectPtr obj)
{
  QString buf;

  ch->removeGold(price);
  eq_remove_damage(obj);
  dc_sprintf(buf, "It will cost you %d coins to repair %s.", price, qPrintable(obj->short_description()));
  do_say(owner, buf);
  act_to_character("You watch $N fix $p...", ch, obj, owner, 0);
  act_to_room("You watch $N fix $p...", ch, obj, owner, 0);
  do_say(owner, "All fixed!");
  act_to_character("$N gives you $p.", ch, obj, owner, 0);
  act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
}

void repair_shop_complain_no_cash(CharacterPtr ch, CharacterPtr owner, qint32 price, ObjectPtr obj)
{
  QString buf;

  do_say(owner, "Trying to sucker me for a free repair job?");
  dc_sprintf(buf, "It would cost %d coins to repair %s, which you don't have!", price, qPrintable(obj->short_description()));
  do_say(owner, buf);
  act_to_character("$N gives you $p.", ch, obj, owner, 0);
  act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
}

void repair_shop_price_check(CharacterPtr ch, CharacterPtr owner, qint32 price, ObjectPtr obj)
{
  QString buf;

  dc_sprintf(buf, "It will only cost you %d coins to repair %s.'", price, qPrintable(obj->short_description()));
  do_say(owner, buf);
  act_to_character("$N gives you $p.", ch, obj, owner, 0);
  act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
}

qint32 repair_guy(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  QString item;
  qint32 value0, cost, price;
  qint32 percent, eqdam;

  if ((cmd != cmd_t::REPAIR) && (cmd != cmd_t::PRICE))
    return ReturnValue::eFAILURE;

  if (!ch->isNonPlayer() && ch->isPlayerGoldThief())
  {
    ch->sendln(u"Your criminal acts prohibit it."_s);
    return ReturnValue::eSUCCESS;
  }

  one_argument(arg, item);

  if (item.isEmpty())
  {
    ch->sendln(u"What item?"_s);
    return ReturnValue::eSUCCESS;
  }

  obj = get_obj_in_list_vis(ch, item, ch->carrying);

  if (obj == nullptr)
  {
    ch->sendln(u"You don't have that item."_s);
    return ReturnValue::eSUCCESS;
  }

  act_to_character("You give $N $p.", ch, obj, owner, 0);
  act_to_room("$n gives $p to $N.", ch, obj, owner, INVIS_NULL);
  act_to_character("\r\n$N examines $p...", ch, obj, owner, 0);
  act_to_room("\r\n$N examines $p...", ch, obj, owner, INVIS_NULL);

  if (IS_OBJ_STAT(obj, ITEM_NOREPAIR) || obj->flags_.type_flag != ITEM_ARMOR || isSet(obj->flags_.extra_flags, ITEM_SPECIAL))
  {
    do_say(owner, "I can't repair this.");
    act_to_character("$N gives you $p.", ch, obj, owner, 0);
    act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  eqdam = eq_current_damage(obj);

  if (eqdam <= 0)
  {
    do_say(owner, "Looks fine to me.");
    act_to_character("$N gives you $p.", ch, obj, owner, 0);
    act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  cost = obj->flags_.cost;
  value0 = eq_max_damage(obj);
  percent = ((100 * eqdam) / value0);
  price = ((cost * percent) / 100); // now we know what to charge them fuckers!

  if (price < 100)
    price = 100; // Welp.. Repair Guy needs to feed the kids somehow.. :)

  if (cmd == cmd_t::PRICE)
  {
    repair_shop_price_check(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }

  if (ch->getGold() < (quint32)price)
  {
    repair_shop_complain_no_cash(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }

  repair_shop_fix_eq(ch, owner, price, obj);
  return ReturnValue::eSUCCESS;
}

qint32 super_repair_guy(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  QString item;
  qint32 value0, value2, cost, price;
  qint32 percent, eqdam;

  if ((cmd != cmd_t::REPAIR) && (cmd != cmd_t::PRICE))
    return ReturnValue::eFAILURE;

  if (!ch->isNonPlayer() && ch->isPlayerGoldThief())
  {
    ch->sendln(u"Your criminal acts prohibit it."_s);
    return ReturnValue::eSUCCESS;
  }

  one_argument(arg, item);

  if (item.isEmpty())
  {
    ch->sendln(u"What item?"_s);
    return ReturnValue::eSUCCESS;
  }

  obj = get_obj_in_list_vis(ch, item, ch->carrying);

  if (obj == nullptr)
  {
    ch->sendln(u"You don't have that item."_s);
    return ReturnValue::eSUCCESS;
  }

  if (IS_OBJ_STAT(obj, ITEM_NOREPAIR))
  {
    do_say(owner, "I can't repair this.");
    return ReturnValue::eSUCCESS;
  }

  act_to_character("You give $N $p.", ch, obj, owner, 0);
  act_to_room("$n gives $p to $N.", ch, obj, owner, INVIS_NULL);
  act_to_character("\r\n$N examines $p...", ch, obj, owner, 0);
  act_to_room("\r\n$N examines $p...", ch, obj, owner, INVIS_NULL);

  eqdam = eq_current_damage(obj);

  if (eqdam <= 0)
  {
    do_say(owner, "Looks fine to me.");
    act_to_character("$N gives you $p.", ch, obj, owner, 0);
    act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  cost = obj->flags_.cost;
  value0 = eq_max_damage(obj);
  value2 = obj->flags_.value[2];

  if ((obj->flags_.type_flag == ITEM_ARMOR || ARE_CONTAINERS(obj) || obj->flags_.type_flag == ITEM_LIGHT) && !isSet(obj->flags_.extra_flags, ITEM_SPECIAL))
  {
    percent = ((100 * eqdam) / value0); /* now we know what percent to repair ..  */
    price = ((cost * percent) / 100);   /* now we know what to charge */
    price *= 2;                         /* he likes to charge more..  */
                                        /*  for armor... cuz he can.. */
  }
  else if ((obj->flags_.type_flag == ITEM_WEAPON || obj->flags_.type_flag == ITEM_FIREWEAPON || obj->flags_.type_flag == ITEM_INSTRUMENT || obj->flags_.type_flag == ITEM_STAFF || obj->flags_.type_flag == ITEM_WAND) && !isSet(obj->flags_.extra_flags, ITEM_SPECIAL))
  {
    percent = ((100 * eqdam) / (value0 + value2)); /* now we know what percent to repair ..  */
    price = ((cost * percent) / 100);              /* now we know what to charge */
  }
  else
  {
    // Dunno how to repair non-weapons/armor
    do_say(owner, "I can't repair this.");
    act_to_character("$N gives you $p.", ch, obj, owner, 0);
    act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  if (price < 1000)
    price = 1000; // Minimum price

  if (cmd == cmd_t::PRICE)
  {
    repair_shop_price_check(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }

  if (ch->getGold() < (quint32)price)
  {
    repair_shop_complain_no_cash(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }
  else
  {
    repair_shop_fix_eq(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }

  return ReturnValue::eSUCCESS;
}

// Fingers
qint32 repair_shop(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  QString item;
  qint32 value0, value2, cost, price;
  qint32 percent, eqdam;

  if ((cmd != cmd_t::REPAIR) && (cmd != cmd_t::PRICE))
    return ReturnValue::eFAILURE;

  if (!ch->isNonPlayer() && ch->isPlayerGoldThief())
  {
    ch->sendln(u"Your criminal acts prohibit it."_s);
    return ReturnValue::eSUCCESS;
  }

  one_argument(arg, item);

  if (item.isEmpty())
  {
    ch->sendln(u"What item?"_s);
    return ReturnValue::eSUCCESS;
  }

  obj = get_obj_in_list_vis(ch, item, ch->carrying);

  if (obj == nullptr)
  {
    ch->sendln(u"You don't have that item."_s);
    return ReturnValue::eSUCCESS;
  }

  if (IS_OBJ_STAT(obj, ITEM_NOREPAIR))
  {
    do_say(owner, "I can't repair this.");
    return ReturnValue::eSUCCESS;
  }

  act_to_character("You give $N $p.", ch, obj, owner, 0);
  act_to_room("$n gives $p to $N.", ch, obj, owner, INVIS_NULL);
  act_to_character("\r\n$N examines $p...", ch, obj, owner, 0);
  act_to_room("\r\n$N examines $p...", ch, obj, owner, INVIS_NULL);

  eqdam = eq_current_damage(obj);

  if (eqdam <= 0)
  {
    do_say(owner, "Looks fine to me.");
    act_to_character("$N gives you $p.", ch, obj, owner, 0);
    act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  cost = obj->flags_.cost;
  value0 = eq_max_damage(obj);
  value2 = obj->flags_.value[2];

  if ((obj->flags_.type_flag == ITEM_ARMOR || obj->flags_.type_flag == ITEM_LIGHT) && !isSet(obj->flags_.extra_flags, ITEM_SPECIAL))
  {

    percent = ((100 * eqdam) / value0); /* now we know what percent to repair ..  */
    price = ((cost * percent) / 100);   /* now we know what to charge them fuckers! */
    price *= 4;                         /* he likes to charge more..  */
                                        /*  for armor... cuz he's a crook..  */
  }
  else if ((obj->flags_.type_flag == ITEM_WEAPON || obj->flags_.type_flag == ITEM_FIREWEAPON || ARE_CONTAINERS(obj) || obj->flags_.type_flag == ITEM_STAFF || obj->flags_.type_flag == ITEM_WAND) && !isSet(obj->flags_.extra_flags, ITEM_SPECIAL))

  {

    percent = ((100 * eqdam) / (value0 + value2));
    //  x = (100 - percent);          /* now we know what percent to repair ..  */
    price = ((cost * percent) / 100); /* now we know what to charge them fuckers! */
    price *= 3;
  }
  else
  {
    // Dunno how to repair non-weapons/armor
    do_say(owner, "I can't repair this.");
    act_to_character("$N gives you $p.", ch, obj, owner, 0);
    act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  if (price < 5000)
    price = 5000; /* Welp.. Repair Guy needs to feed the kids somehow.. :) */

  if (cmd == cmd_t::PRICE)
  {
    repair_shop_price_check(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }

  if (ch->getGold() < (quint32)price)
  {
    repair_shop_complain_no_cash(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }
  else
  {
    repair_shop_fix_eq(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }
}

qint32 corpse_cost(CharacterPtr ch)
{
  qint32 cost = {};
  ObjectPtr curr_cont;

  for (ObjectPtr obj2 = ch->carrying; obj2; obj2 = obj2->next_content)
  {
    if (obj2->flags_.type_flag == ITEM_MONEY)
      continue;
    for (curr_cont = obj2->contains; curr_cont; curr_cont = curr_cont->next_content)
    {
      if (!isSet(curr_cont->flags_.extra_flags, ITEM_SPECIAL))
        cost += curr_cont->flags_.cost;
    }
    if (!isSet(obj2->flags_.extra_flags, ITEM_SPECIAL))
      cost += obj2->flags_.cost;
  }
  for (qint32 x = {}; x < MAX_WEAR; x++)
  {
    if (ch->equipment[x])
    {
      for (curr_cont = ch->equipment[x]->contains; curr_cont; curr_cont = curr_cont->next_content)
        if (!isSet(curr_cont->flags_.extra_flags, ITEM_SPECIAL))
          cost += curr_cont->flags_.cost;

      if (!isSet(ch->equipment[x]->flags_.extra_flags, ITEM_SPECIAL))
        cost += ch->equipment[x]->flags_.cost;
    }
  }
  return cost;
}

qint32 corpse_cost(ObjectPtr obj)
{
  qint32 cost = {};
  ObjectPtr curr_cont;

  for (ObjectPtr obj2 = obj->contains; obj2; obj2 = obj2->next_content)
  {
    if (obj2->flags_.type_flag == ITEM_MONEY)
      continue;
    for (curr_cont = obj2->contains; curr_cont; curr_cont = curr_cont->next_content)
      cost += curr_cont->flags_.cost;
    cost += obj2->flags_.cost;
  }
  return cost;
}

qint32 mortician(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  qint32 x = 0, cost = 0, which;
  qint32 count = {};
  QString buf;

  if (cmd != cmd_t::BUY && cmd != cmd_t::LIST && cmd != cmd_t::VALUE)
    return ReturnValue::eFAILURE;

  // TODO - when determining price, it WILL NOT work if we ever institute
  // containers being inside other containers.

  if (cmd == cmd_t::LIST) // list
  {
    dc_sprintf(buf, "%s_consent", qPrintable(ch->name()));
    ch->send(u"Available corpses (freshest first):\r\n$B"_s);
    for (obj = ch->dc_->object_list; obj; obj = obj->next)
    {
      if (GET_ITEM_TYPE(obj) != ITEM_CONTAINER || obj->flags_.value[3] != 1) // only look at corpses
        continue;

      if (!isexact("pc", obj->name()) || (!isexact(qPrintable(ch->name()), obj->name()) && !isexact(buf, obj->name())))
        continue;

      if (obj->in_room == ch->in_room)
        continue;
      if (!obj->contains) // skip empty corpses
        continue;

      cost = corpse_cost(obj);
      cost /= 20000;
      cost = MAX(cost, 30);
      dc_sprintf(buf, "%d) %-21s %d Platinum coins.\r\n", ++count, qPrintable(obj->short_description()), cost);
      ch->send(buf);
    }
    send_to_char("$RIf any corpses were listed, they are still where you left them.  This\r\n"
                 "list is therefore always changing.  If you purchase one, it will be\r\n"
                 "placed at your feet. Use \"buy <number>\" to purchase a corpse.\r\n"
                 "Use 'value' to find how much your eq would cost with what you\r\n"
                 "have on you now.\r\n",
                 ch);
    return ReturnValue::eSUCCESS;
  }

  if (cmd == cmd_t::VALUE) // value
  {
    cost = corpse_cost(ch);
    cost /= 20000;
    cost = MAX(cost, 30);
    ch->send(u"The Undertaker takes a look at you and estimates your corpse would cost around %1 platinum coins.\r\n"_s.arg(cost));
    return ReturnValue::eSUCCESS;
  }

  /* buy */
  if ((which = dc_atoi(arg)) == 0)
  {
    send_to_char("Try \"buy <number>\", or \"list\" for a list of "
                 "available corpses.\r\n",
                 ch);
    return ReturnValue::eSUCCESS;
  }

  for (obj = ch->dc_->object_list; obj; obj = obj->next)
  {
    dc_sprintf(buf, "%s_consent", qPrintable(ch->name()));

    if (GET_ITEM_TYPE(obj) != ITEM_CONTAINER || obj->flags_.value[3] != 1) // only look at corpses
      continue;

    if (!isexact("pc", obj->name()) || (!isexact(qPrintable(ch->name()), obj->name()) && !isexact(buf, obj->name())) || ++x < which)
      continue;

    if (!obj->contains) // skip empty corpses
      continue;

    if (obj->in_room == ch->in_room)
      continue; // Skip bought corpses

    cost = corpse_cost(obj);
    cost /= 20000;
    cost = MAX(cost, 30);
    if (GET_PLATINUM(ch) < (quint32)cost)
    {
      ch->sendln(u"You can't afford that!"_s);
      return ReturnValue::eSUCCESS;
    }
    move_obj(obj, ch->in_room);
    REMOVE_BIT(obj->flags_.extra_flags, ITEM_INVISIBLE);
    send_to_char("The mortician goes into his freezer and returns with a corpse, which he\r\n"
                 "places at your feet.\r\n",
                 ch);
    GET_PLATINUM(ch) -= cost;
    ch->save();
    save_corpses();
    return ReturnValue::eSUCCESS;
  }

  ch->sendln(u"No such corpse was found.  Try \"list\"."_s);
  return ReturnValue::eSUCCESS;
}

QString gl_item(ObjectPtr obj, qint32 number, CharacterPtr ch, bool platinum = true)
{
  QString buf;
  if (platinum)
  {
    buf = u"$B$7%1$R) %2 "_s.arg(number + 1, -2).arg(obj->short_description());
  }
  else
  {
    buf = u"$B$7%1$R) $3$B%2$R "_s.arg(number + 1, -2).arg(obj->short_description());
  }

  if (obj->flags_.type_flag == ITEM_WEAPON)
  { // weapon
    buf = u"%1%2d%3, %4, "_s.arg(buf).arg(obj->flags_.value[1]).arg(obj->flags_.value[2]).arg(isSet(obj->flags_.extra_flags, ITEM_TWO_HANDED) ? "Two-handed" : "One-handed");
  }

  QString buf2;
  qsizetype length = {};
  for (decltype(obj->num_affects) i = {}; i < obj->num_affects; i++)
  {
    if ((obj->affected[i].location != APPLY_NONE) && (obj->affected[i].modifier != 0))
    {
      if (obj->affected[i].location < 1000)
      {
        buf2 = sprinttype(obj->affected[i].location, apply_types).c_str();
      }
      else if (!get_skill_name(obj->affected[i].location / 1000).isEmpty())
      {
        buf2 = get_skill_name(obj->affected[i].location / 1000);
      }
      else
      {
        buf2 = u"Invalid"_s;
      }

      QString buf3 = u"%1 by %2, "_s.arg(buf2).arg(obj->affected[i].modifier).toLower();

      QString potential_buffer = buf + buf3;
      qsizetype starting_point = potential_buffer.lastIndexOf("\n");
      if (starting_point == -1)
      {
        starting_point = {};
      }
      length = nocolor_strlen(potential_buffer.sliced(starting_point));

      if (length > 79)
      {
        buf = u"%1\r\n    %2"_s.arg(buf).arg(buf3);
      }
      else
      {
        buf = potential_buffer;
      }
    }
  }

  // Room where Orro, the Quest Guide is located
  if (ch->in_room != 3055)
  {
    if (class_restricted(ch, obj) || size_restricted(ch, obj))
    {
      buf2 = u"$4[restricted]$R, "_s;

      QString potential_buffer = buf + buf2;
      qsizetype starting_point = potential_buffer.lastIndexOf("\n");
      if (starting_point == -1)
      {
        starting_point = {};
      }
      length = nocolor_strlen(potential_buffer.sliced(starting_point));
      if (length > 79)
      {
        buf = u"%1\r\n    %2"_s.arg(buf).arg(buf2);
      }
      else
      {
        buf = potential_buffer;
      }
    }
  }
  else
  {
    auto a = obj->flags_.extra_flags;
    a &= ITEM_WARRIOR | ITEM_MAGE | ITEM_THIEF | ITEM_CLERIC | ITEM_PAL | ITEM_ANTI | ITEM_BARB | ITEM_MONK | ITEM_RANGER | ITEM_DRUID | ITEM_BARD;

    QString buffer = sprintbit(a, Object::extra_bits);
    buf2 = u"%1]$R, "_s.arg(buffer);

    if (a)
    {
      QString potential_buffer = u"%1$4[%2"_s.arg(buf).arg(buf2);
      qsizetype starting_point = potential_buffer.lastIndexOf("\n");
      if (starting_point == -1)
      {
        starting_point = {};
      }
      length = nocolor_strlen(potential_buffer.sliced(starting_point));

      if (length > 79)
      {
        buf = u"%1\r\n    $4[%2"_s.arg(buf).arg(buf2);
      }
      else
      {
        buf = potential_buffer;
      }
    }
  }

  if (platinum)
  {
    buf2 = u"costing %1 coins."_s.arg(obj->flags_.cost / 10);
  }
  else
  {
    buf2 = u"costing %1 qpoints."_s.arg(obj->flags_.cost / 10000);
  }

  QString potential_buffer = buf + buf2;
  qsizetype starting_point = potential_buffer.lastIndexOf("\n");
  if (starting_point == -1)
  {
    starting_point = {};
  }
  length = nocolor_strlen(potential_buffer.sliced(starting_point));

  if (length > 79)
  {
    buf = u"%1\r\n    %2"_s.arg(buf).arg(buf2);
  }
  else
  {
    buf = potential_buffer;
  }

  buf += "\r\n";
  return buf;
}

class platsmith
{
public:
  qint32 vnum;
  QList<qint32> sales;
};

const platsmith platsmith_list[] = {{10019, {512, 513, 514, 515, 537, 538, 539, 540, 541, 0, 0, 0, 0}}, {10020, {554, 555, 556, 557, 524, 525, 526, 527, 504, 505, 506, 511, 0}}, {10021, {516, 517, 518, 519, 507, 508, 509, 510, 546, 547, 548, 549, 0}}, {10022, {500, 501, 502, 503, 520, 521, 522, 523, 528, 529, 530, 531, 0}}, {10023, {542, 543, 544, 545, 532, 533, 534, 535, 536, 550, 551, 552, 553}}, {10026, {558, 559, 560, 561, 562, 563, 564, 565, 566, 0, 0, 0, 0}}, {10004, {570, 571, 575, 577, 578, 580, 582, 584, 586, 587, 590, 591, 598}}, // weapon dude in cozy
                                    {10024, {592, 593, 594, 567, 568, 0, 0, 0, 0, 0, 0, 0, 0}},                                                                                                                                                                                                                                                                                                                                                                                                                                                                   // 2handed weapon/bow dude
                                    {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}};

// Apoc enjoys the dirty mooselove. Honest.
qint32 godload_sales(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{

  qint32 mobvnum = ch->dc_->mob_index_[owner->mobdata->nr].vnum();
  qint32 o;
  QString buf;
  //  return ReturnValue::eFAILURE; //disabled for now
  if (cmd == cmd_t::LIST)
  {
    if (!CAN_SEE(owner, ch))
    {
      do_say(owner, "I don't trade with people I can't see!");
      return ReturnValue::eSUCCESS;
    }

    for (o = {}; platsmith_list[o].vnum != 0; o++)
      if (mobvnum == platsmith_list[o].vnum)
        break;
    if (platsmith_list[o].vnum == 0)
    {
      owner->do_tell(u"%1 Sorry, I don't seem to be working correctly. Do tell someone."_s.arg(qPrintable(ch->name())).split(' '));
      return ReturnValue::eSUCCESS;
    }
    owner->do_tell(u"%1 Here's what I can do for you, %2."_s.arg(qPrintable(ch->name())).arg(pc_clss_types3[GET_CLASS(ch)]).split(' '));

    for (qint32 z = {}; z < 13 && platsmith_list[o].sales[z] != 0; z++)
    {
      auto tmp = gl_item(dc_->obj_index_[real_object(platsmith_list[o].sales[z])]->item, z, ch);
      ch->send(tmp);
    }
    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::BUY)
  {
    if (!CAN_SEE(owner, ch))
    {
      do_say(owner, "I don't trade with people I can't see!");
      return ReturnValue::eSUCCESS;
    }

    for (o = {}; platsmith_list[o].vnum != 0; o++)
      if (mobvnum == platsmith_list[o].vnum)
        break;
    QString buf, arg2;
    one_argument(arg, arg2);
    if (platsmith_list[o].vnum == 0)
    {
      owner->do_tell(u"%1 Sorry, I don't seem to be working correctly. Do tell someone."_s.arg(qPrintable(ch->name())).split(' '));
      return ReturnValue::eSUCCESS;
    }
    if (!is_number(arg2))
    {
      owner->do_tell(u"%1 Sorry, mate. You type buy <number> to specify what you want.."_s.arg(qPrintable(ch->name())).split(' '));
      return ReturnValue::eSUCCESS;
    }
    qint32 k = dc_atoi(arg2) - 1;
    if (k >= 13 || k < 0 || platsmith_list[o].sales[k] == 0)
    {
      owner->do_tell(u"%1 Don't have that I'm afraid. Type \"list\" to see my wares."_s.arg(qPrintable(ch->name())).split(' '));
      return ReturnValue::eSUCCESS;
    }
    ObjectPtr obj;
    obj = clone_object(real_object(platsmith_list[o].sales[k]));

    if (class_restricted(ch, obj) || size_restricted(ch, obj) || search_char_for_item(ch, obj->item_number, false))
    {
      owner->do_tell(u"%1 That item is not available to you."_s.arg(qPrintable(ch->name())).split(' '));
      extract_obj(obj);
      return ReturnValue::eSUCCESS;
    }
    if (GET_PLATINUM(ch) < (quint32)(obj->flags_.cost / 10))
    {
      owner->do_tell(u"%1 Come back when you've got the platinum."_s.arg(qPrintable(ch->name())).split(' '));
      extract_obj(obj);
      return ReturnValue::eSUCCESS;
    }
    GET_PLATINUM(ch) -= obj->flags_.cost / 10;
    dc_sprintf(buf, "%s %s", qPrintable(obj->name()), qPrintable(ch->name()));
    obj->name(buf);
    obj_to_char(obj, ch);
    owner->do_tell(u"%1 Here's your %2$B$2. Have a nice time with it."_s.arg(qPrintable(ch->name())).arg(obj->short_description()).split(' '));
    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::SELL)
  {
    ObjectPtr obj;
    QString arg2;
    one_argument(arg, arg2);
    obj = get_obj_in_list_vis(ch, arg2, ch->carrying);
    if (!CAN_SEE(owner, ch))
    {
      do_say(owner, "I don't trade with people I can't see!");
      return ReturnValue::eSUCCESS;
    }
    if (!obj)
    {
      owner->do_tell(u"%1 Try that on the kooky meta-physician.."_s.arg(qPrintable(ch->name())).split(' '));
      return ReturnValue::eSUCCESS;
    }
    if (!isSet(obj->flags_.extra_flags, ITEM_SPECIAL))
    {
      owner->do_tell(u"%1 I don't deal in worthless junk."_s.arg(qPrintable(ch->name())).split(' '));
      return ReturnValue::eSUCCESS;
    }

    // don't allow non-empty containers to be sold
    if (obj->flags_.type_flag == ITEM_CONTAINER && obj->contains)
    {
      owner->do_tell(u"%1 %2$B$2 needs to be emptied first."_s.arg(qPrintable(ch->name())).arg(GET_OBJ_SHORT(obj)).split(' '));
      return ReturnValue::eSUCCESS;
    }

    qint32 cost = obj->flags_.cost / 10;

    owner->do_tell(u"%1 I'll give you %2 plats for that. Thanks for shoppin'."_s.arg(qPrintable(ch->name())).arg(cost).split(' '));
    extract_obj(obj);
    GET_PLATINUM(ch) += cost;
    return ReturnValue::eSUCCESS;
  }
  return ReturnValue::eFAILURE;
}

// gl_repair_guy
qint32 gl_repair_shop(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  QString item;
  qint32 value0, value2, cost, price;
  qint32 percent, eqdam;

  if ((cmd != cmd_t::REPAIR) && (cmd != cmd_t::PRICE))
    return ReturnValue::eFAILURE;

  if (!ch->isNonPlayer() && ch->isPlayerGoldThief())
  {
    ch->sendln(u"Your criminal acts prohibit it."_s);
    return ReturnValue::eSUCCESS;
  }

  one_argument(arg, item);

  if (item.isEmpty())
  {
    ch->sendln(u"What item?"_s);
    return ReturnValue::eSUCCESS;
  }

  obj = get_obj_in_list_vis(ch, item, ch->carrying);

  if (obj == nullptr)
  {
    ch->sendln(u"You don't have that item."_s);
    return ReturnValue::eSUCCESS;
  }

  if (IS_OBJ_STAT(obj, ITEM_NOREPAIR))
  {
    do_say(owner, "I can't repair this.");
    return ReturnValue::eSUCCESS;
  }

  act_to_character("You give $N $p.", ch, obj, owner, 0);
  act_to_room("$n gives $p to $N.", ch, obj, owner, INVIS_NULL);
  act_to_character("\r\n$N examines $p...", ch, obj, owner, 0);
  act_to_room("\r\n$N examines $p...", ch, obj, owner, INVIS_NULL);

  eqdam = eq_current_damage(obj);

  if (eqdam <= 0)
  {
    do_say(owner, "Looks fine to me.");
    act_to_character("$N gives you $p.", ch, obj, owner, 0);
    act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  cost = obj->flags_.cost;
  value0 = eq_max_damage(obj);
  value2 = obj->flags_.value[2];

  if (!isSet(obj->flags_.extra_flags, ITEM_SPECIAL))
  {
    do_say(owner, "I don't repair this kind of junk.");
    act_to_character("$N gives you $p.", ch, obj, owner, 0);
    act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }
  if (obj->flags_.type_flag == ITEM_ARMOR || obj->flags_.type_flag == ITEM_LIGHT)
  {

    percent = ((100 * eqdam) / value0);
    price = ((cost * percent) / 100); /* now we know what to charge them fuckers! */
    price *= 4;                       /* he likes to charge more..  */
                                      /*  for armor... cuz he's a crook..  */
  }
  else if (obj->flags_.type_flag == ITEM_WEAPON || obj->flags_.type_flag == ITEM_FIREWEAPON || ARE_CONTAINERS(obj) || obj->flags_.type_flag == ITEM_STAFF || obj->flags_.type_flag == ITEM_WAND || obj->flags_.type_flag == ITEM_INSTRUMENT)
  {

    percent = ((100 * eqdam) / (value0 + value2));
    price = ((cost * percent) / 100); /* now we know what to charge them fuckers! */
    price *= 5;
  }
  else
  {
    // Dunno how to repair non-weapons/armor
    do_say(owner, "I can't repair this.");
    act_to_character("$N gives you $p.", ch, obj, owner, 0);
    act_to_room("$N gives $n $p.", ch, obj, owner, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  if (price < 50000)
    price = 50000; /* Welp.. Repair Guy needs to feed the kids somehow.. :) */

  if (cmd == cmd_t::PRICE)
  {
    repair_shop_price_check(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }

  if (ch->getGold() < (quint32)price)
  {
    repair_shop_complain_no_cash(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }
  else
  {
    repair_shop_fix_eq(ch, owner, price, obj);
    return ReturnValue::eSUCCESS;
  }
}
