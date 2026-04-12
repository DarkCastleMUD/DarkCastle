/************************************************************************
| Description:  This file contains implementation of inventory-management
|   commands: get, give, put, etc..
|
| Authors: DikuMUD, Pirahna, Staylor, Urizen, Rahz, Zaphod, Shane, Jhhudso, Heaven1 and others
*/
#include <cstring>

#include <fmt/format.h>

#include "DC/common.h"
#include "DC/levels.h"
#include "DC/obj.h"
#include "DC/DC.h"

#include "DC/structs.h"

#include "DC/player.h"
#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/act.h"
#include "DC/returnvals.h"
#include "DC/spells.h"
#include "DC/clan.h"
#include "DC/inventory.h"
#include "DC/corpse.h"
#include "DC/comm.h"

/* extern variables */

extern qint32 rev_dir[];

/* procedures related to get */
void get(CharacterPtr ch, ObjectPtr obj_object, ObjectPtr sub_object, bool has_consent, cmd_t cmd)
{
  QString buffer;

  if (!sub_object || sub_object->carried_by != ch)
  {
    if (isSet(obj_object->obj_flags.more_flags, ITEM_NO_TRADE))
    {
      if (ch->isNonPlayer())
      {
        ch->sendln("You cannot get that item.");
        return;
      }
      else if (!obj_object->getOwner().isEmpty() && obj_object->getOwner() != qPrintable(ch->name()))
      {
        ch->send(u"You cannot get that item because it's marked NO_TRADE and owned by %1\r\n"_s.arg(obj_object->getOwner()));
        return;
      }
    }

    // we only have to check for uniqueness if the container is not on the character
    // or if there is no container
    if (isSet(obj_object->obj_flags.more_flags, ITEM_UNIQUE))
    {
      if (search_char_for_item(ch, obj_object->item_number, false))
      {
        ch->sendln("The item's uniqueness prevents it!");
        return;
      }
    }
    if (contents_cause_unique_problem(obj_object, ch))
    {
      ch->sendln("Something inside the item is unique and prevents it!");
      return;
    }
  }

  if ((ch->isNonPlayer() || ch->affected_by_spell(OBJ_CHAMPFLAG_TIMER)) && DC::getInstance()->obj_index[obj_object->item_number].vnum() == CHAMPION_ITEM)
  {
    ch->sendln("No champion flag for you, two years!");
    return;
  }

  if (sub_object)
  {
    buffer = u"%1_consent"_s.arg(ch->name());
    if (has_consent && obj_object->obj_flags.type_flag != ITEM_MONEY)
    {
      if ((cmd == cmd_t::LOOT && isexact("lootable", sub_object->name())) && !isexact(buffer, sub_object->name()))
      {
        SET_BIT(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED);
        WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
        logmortal(u"%1 looted %2[%3] from %4"_s.arg(ch->name()).arg(obj_object->short_description()).arg(DC::getInstance()->obj_index[obj_object->item_number].vnum()).arg(sub_object->name()));

        ch->sendln("You suddenly feel very guilty...shame on you stealing from the dead!");

        affected_type pthiefaf;
        pthiefaf.type = Character::PLAYER_OBJECT_THIEF;
        pthiefaf.duration = 10;
        pthiefaf.modifier = {};
        pthiefaf.location = APPLY_NONE;
        pthiefaf.bitvector = -1;

        if (ch->isPlayerObjectThief())
        {
          affect_from_char(ch, Character::PLAYER_OBJECT_THIEF);
          affect_to_char(ch, &pthiefaf);
        }
        else
          affect_to_char(ch, &pthiefaf);
      }
    }
    else if (has_consent && obj_object->obj_flags.type_flag == ITEM_MONEY && !isexact(buffer, sub_object->name()))
    {
      if (cmd == cmd_t::LOOT && isexact("lootable", sub_object->name()))
      {
        affected_type pthiefaf;

        pthiefaf.type = Character::PLAYER_GOLD_THIEF;
        pthiefaf.duration = 10;
        pthiefaf.modifier = {};
        pthiefaf.location = APPLY_NONE;
        pthiefaf.bitvector = -1;
        WAIT_STATE(ch, DC::PULSE_VIOLENCE);
        ch->sendln("You suddenly feel very guilty...shame on you stealing from the dead!");
        logmortal(u"%1 looted %2 coins from %3"_s.arg(qPrintable(ch->name())).arg(obj_object->obj_flags.value[0]).arg(sub_object->name()));

        if (ch->isPlayerGoldThief())
        {
          affect_from_char(ch, Character::PLAYER_GOLD_THIEF);
          affect_to_char(ch, &pthiefaf);
        }
        else
          affect_to_char(ch, &pthiefaf);
      }
    }

    if (sub_object->in_room && obj_object->obj_flags.type_flag != ITEM_MONEY && sub_object->carried_by != ch)
    { // Logging gold gets from corpses would just be too much.
      logobjects(u"%1 gets %2[%3] from %4[%5]"_s.arg(qPrintable(ch->name())).arg(obj_object->name()).arg(DC::getInstance()->obj_index[obj_object->item_number].vnum()).arg(sub_object->name()).arg(DC::getInstance()->obj_index[sub_object->item_number].vnum()));
      for (ObjectPtr loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
        logobjects(u"The %1[%2] contained %3[%4]"_s.arg(obj_object->short_description()).arg(DC::getInstance()->obj_index[obj_object->item_number].vnum()).arg(loop_obj->short_description).arg(DC::getInstance()->obj_index[loop_obj->item_number].vnum()));
    }
    move_obj(obj_object, ch);
    if (sub_object->carried_by == ch)
    {
      act_to_character("You get $p from $P.", ch, obj_object, sub_object, 0);
      act_to_room("$n gets $p from $s $P.", ch, obj_object, sub_object, INVIS_NULL);
    }
    else
    {
      act_to_character("You get $p from $P.", ch, obj_object, sub_object, INVIS_NULL);
      act_to_room("$n gets $p from $P.", ch, obj_object, sub_object, INVIS_NULL);
    }
  }
  else
  {
    move_obj(obj_object, ch);
    act_to_character("You get $p.", ch, obj_object, 0, 0);
    act_to_room("$n gets $p.", ch, obj_object, 0, INVIS_NULL);
    if (obj_object->obj_flags.type_flag != ITEM_MONEY)
    {
      logobjects(u"%1 gets %2[%3] from room %4"_s.arg(qPrintable(ch->name())).arg(obj_object->name()).arg(DC::getInstance()->obj_index[obj_object->item_number].vnum()).arg(ch->in_room));
      for (ObjectPtr loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
        logobjects(u"The %1 contained %2[%3]"_s.arg(obj_object->short_description()).arg(loop_obj->short_description()).arg(DC::getInstance()->obj_index[loop_obj->item_number].vnum()));
    }

    if (DC::getInstance()->obj_index[obj_object->item_number].vnum() == CHAMPION_ITEM)
    {
      SETBIT(ch->affected_by, AFF_CHAMPION);
      buffer = u"\r\n##%1 has just picked up %2!\r\n"_s.arg(qPrintable(ch->name())).arg(static_cast<ObjectPtr>(DC::getInstance()->obj_index[obj_object->item_number].item)->short_description());
      send_info(buffer);
    }
  }
  if (sub_object && sub_object->obj_flags.value[3] == 1 &&
      isexact("pc", sub_object->name()))
    ch->save(cmd_t::SAVE_SILENTLY);

  if ((obj_object->obj_flags.type_flag == ITEM_MONEY) &&
      (obj_object->obj_flags.value[0] >= 1))
  {
    obj_from_char(obj_object);

    buffer = u"There was %1 coins."_s).arg(obj_object->obj_flags.value[0]);
    if (ch->isNonPlayer() || !isSet(ch->player->toggles, Player::PLR_BRIEF))
    {
      ch->send(buffer);
      ch->sendln("");
    }
    bool tax = false;

    if (DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner > 0 && ch->clan !=
                                                                                                        DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner)
    {
      qint32 cgold = (qint32)((qreal)(obj_object->obj_flags.value[0]) * 0.1);
      obj_object->obj_flags.value[0] -= cgold;
      DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).addGold(cgold);
      if (!ch->isNonPlayer() && isSet(ch->player->toggles, Player::PLR_BRIEF))
      {
        tax = true;
        buffer += u"Bounty: %2"_s.arg(cgold);
        DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).addGold(cgold);
      }
      else
        ch->sendln(u"Clan %1 collects %2 bounty, leaving %3 for you."_s).arg(get_clan(DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner)->name).arg(cgold).arg(obj_object->obj_flags.value[0]));
    }
    //	if (sub_object && sub_object->obj_flags.value[3] == 1 &&
    //           !isexact("pc",sub_object->name()) && ch->clan
    //            && get_clan(ch)->tax && !isSet(GET_TOGGLES(ch), Player::PLR_NOTAX))
    if (((sub_object && sub_object->obj_flags.value[3] == 1 && !isexact("pc", sub_object->name())) || !sub_object) &&
        ch->clan &&
        get_clan(ch)->tax &&
        !isSet(GET_TOGGLES(ch), Player::PLR_NOTAX))
    {
      qint32 cgold = (qint32)((qreal)(obj_object->obj_flags.value[0]) * (qreal)((qreal)(get_clan(ch)->tax) / 100.0));
      obj_object->obj_flags.value[0] -= cgold;
      ch->addGold(obj_object->obj_flags.value[0]);
      get_clan(ch)->cdeposit(cgold);
      if (!ch->isNonPlayer() && isSet(ch->player->toggles, Player::PLR_BRIEF))
      {
        tax = true;
        buffer += u"ClanTax: %2"_s.arg(cgold);
      }
      else
      {
        ch->sendln(u"Your clan taxes you %1 $B$5gold$R, leaving %2 $B$5gold$R for you."_s.arg(cgold).arg(obj_object->obj_flags.value[0]));
      }
      save_clans();
    }
    else
      ch->addGold(obj_object->obj_flags.value[0]);

    // If a mob gets gold, we disable its ability to receive a gold bonus. This keeps
    // the mob from turning into an interest bearing savings account. :)
    if (ch->isNonPlayer())
    {
      SETBIT(ch->mobdata->actflags, ACT_NO_GOLD_BONUS);
    }

    if (tax)
    {
      buffer += u". %1 $B$5gold$R remaining.\r\n"_s.arg(obj_object->obj_flags.value[0]);
    }
    else
    {
      buffer += u"\r\n"_s;
    }

    if (!ch->isNonPlayer() && isSet(ch->player->toggles, Player::PLR_BRIEF))
      ch->send(buffer);
    extract_obj(obj_object);
  }

  save_corpses();
}

// return ReturnValue::eSUCCESS if item was picked up
// return ReturnValue::eFAILURE if not
// TODO - currently this is designed with icky if-logic.  While it allows you to put
// code at the end that would affect any attempted 'get' it looks really nasty and
// is never utilized.  Restructure it so it is clear.  Pay proper attention to 'saving'
// however so as not to introduce a potential dupe-bug.
command_return_t do_get(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  QString arg2;
  QString buffer;
  ObjectPtr sub_object;
  ObjectPtr obj_object;
  ObjectPtr next_obj;
  bool found = false;
  bool fail = false;
  bool has_consent = false;
  qint32 type = 3;
  bool alldot = false;
  bool inventorycontainer = false, blindlag = false;
  QString allbuf;

  argument_interpreter(argument, arg1, arg2);

  /* get type */
  if (arg.isEmpty() 1)
  {
    type = {};
  }
  if (*arg1 && arg.isEmpty() 2)
  {
    alldot = false;
    allbuf[0] = '\0';
    if ((str_cmp(arg1, "all") != 0) &&
        (sscanf(arg1, "all.%s", allbuf) != 0))
    {
      dc_strcpy(arg1, "all");
      alldot = true;
    }
    if (!str_cmp(arg1, "all"))
    {
      type = 1;
    }
    else
    {
      type = 2;
    }
  }
  if (*arg1 && *arg2)
  {
    alldot = false;
    allbuf[0] = '\0';
    if ((str_cmp(arg1, "all") != 0) &&
        (sscanf(arg1, "all.%s", allbuf) != 0))
    {
      dc_strcpy(arg1, "all");
      alldot = true;
    }
    if (!str_cmp(arg1, "all"))
    {
      if (!str_cmp(arg2, "all"))
      {
        type = 3;
      }
      else
      {
        type = 4;
      }
    }
    else
    {
      if (!str_cmp(arg2, "all"))
      {
        type = 5;
      }
      else
      {
        type = 6;
      }
    }
  }

  if ((cmd == cmd_t::PALM) && (GET_CLASS(ch) != CLASS_THIEF) &&
      (!ch->isImmortalPlayer()))
  {
    ch->sendln("I bet you think you're a thief.");
    return ReturnValue::eFAILURE;
  }

  if (cmd == cmd_t::PALM && type != 2 && type != 6 && type != 0)
  {
    send_to_char("You can only palm objects that are in the same room, "
                 "one at a time.\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  if (cmd == cmd_t::LOOT && type != 0 && type != 6)
  {
    ch->sendln("You can only loot 1 item from a non-consented corpse.");
    return ReturnValue::eFAILURE;
  }

  switch (type)
  {
  /* get */
  case 0:
  {
    switch (cmd)
    {
    case cmd_t::PALM:
      ch->sendln("Palm what?");
      break;
    case cmd_t::LOOT:
      ch->sendln("Loot what?");
      break;
    default:
      ch->sendln("Get what?");
    }
  }
  break;
  /* get all */
  case 1:
  {
    if (ch->in_room == real_room(3099))
    {
      ch->sendln("Not in the donation room.");
      return ReturnValue::eFAILURE;
    }
    sub_object = {};
    found = false;
    fail = false;
    for (obj_object = DC::getInstance()->world[ch->in_room].contents;
         obj_object;
         obj_object = next_obj)
    {
      next_obj = obj_object->next_content;

      /* IF all.obj, only get those named "obj" */
      if (alldot && !isexact(allbuf, obj_object->name()))
        continue;

      // Can't pick up NO_NOTICE items with 'get all'  only 'all.X' or 'X'
      if (!alldot && isSet(obj_object->obj_flags.more_flags, ITEM_NONOTICE) && !ch->isImmortalPlayer())
        continue;

      // Ignore NO_TRADE items on a 'get all'
      if (isSet(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) && !ch->isImmortalPlayer())
      {
        ch->send(u"The %1 appears to be NO_TRADE so you don't pick it up.\r\n"_s.arg(obj_object->short_description()));
        continue;
      }
      if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
          obj_object->obj_flags.value[0] > 10000 &&
          ch->getLevel() < 5)
      {
        ch->sendln("You cannot pick up that much money!");
        continue;
      }

      if (obj_object->obj_flags.eq_level > 9 && ch->getLevel() < 5)
      {
        ch->send(u"%1 is too powerful for you to possess.\r\n"_s).arg(obj_object->short_description()));
        continue;
      }

      if (isSet(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
          !isexact(qPrintable(ch->name()), obj_object->name()) && ch->getLevel() < IMPLEMENTER)
      {
        ch->send(u"The %1 appears to be SPECIAL. Only its rightful owner can take it.\r\n"_s).arg(obj_object->short_description()));
        continue;
      }

      // PC corpse
      if ((obj_object->obj_flags.value[3] == 1 && isexact("pc", obj_object->name())) || isexact("thiefcorpse", obj_object->name()))
      {
        dc_sprintf(buffer, "%s_consent", qPrintable(ch->name()));
        if ((isexact("thiefcorpse", obj_object->name()) &&
             !isexact(qPrintable(ch->name()), obj_object->name())) ||
            isexact(qPrintable(ch->name()), obj_object->name()) || ch->getLevel() >= OVERSEER)
          has_consent = true;
        if (!has_consent && !isexact(qPrintable(ch->name()), obj_object->name()))
        {
          if (ch->getLevel() < OVERSEER)
          {
            ch->sendln("You don't have consent to take the corpse.");
            continue;
          }
        }
        if (has_consent && contains_no_trade_item(obj_object))
        {
          if (ch->getLevel() < OVERSEER)
          {
            ch->sendln("This item contains no_trade items that cannot be picked up.");
            has_consent = false; // bugfix, could loot without consent
            continue;
          }
        }
        if (ch->getLevel() < OVERSEER)
          has_consent = false; // reset it for the next item:P
        else
          has_consent = true; // reset it for the next item:P
      }

      if (CAN_SEE_OBJ(ch, obj_object) && !ch->isImmortalPlayer())
      {
        // Don't bother checking this if item is gold coins.
        if ((IS_CARRYING_N(ch) + 1) > CAN_CARRY_N(ch) &&
            !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && !ch->isImmortalPlayer()))
        {
          dc_sprintf(buffer, "%s : You can't carry that many items.\r\n", qPrintable(fname(obj_object->name())));
          ch->send(buffer);
          fail = true;
        }
        else if ((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) > CAN_CARRY_W(ch) && !ch->isImmortalPlayer() && GET_ITEM_TYPE(obj_object) != ITEM_MONEY)
        {
          dc_sprintf(buffer, "%s : You can't carry that much weight.\r\n", qPrintable(fname(obj_object->name())));
          ch->send(buffer);
          fail = true;
        }
        else if (CAN_WEAR(obj_object, TAKE))
        {
          get(ch, obj_object, sub_object, 0, cmd);
          found = true;
        }
        else
        {
          ch->sendln("You can't take that.");
          fail = true;
        }
      }
      else if (CAN_SEE_OBJ(ch, obj_object) && ch->isImmortalPlayer() && CAN_WEAR(obj_object, TAKE))
      {
        get(ch, obj_object, sub_object, 0, cmd);
        found = true;
      }
    } // of for loop
    if (found)
    {
      //		ch->sendln("OK.");
      ch->save(cmd_t::SAVE_SILENTLY);
    }
    else
    {
      if (!fail)
        ch->sendln("You see nothing here.");
    }
  }
  break;
  /* get ??? */
  case 2:
  {
    sub_object = {};
    found = false;
    fail = false;
    obj_object = get_obj_in_list_vis(ch, arg1,
                                     DC::getInstance()->world[ch->in_room].contents);
    if (obj_object)
    {
      if (obj_object->obj_flags.type_flag == ITEM_CONTAINER &&
          obj_object->obj_flags.value[3] == 1 &&
          isexact("pc", obj_object->name()))
      {
        dc_sprintf(buffer, "%s_consent", qPrintable(ch->name()));
        if (isexact(qPrintable(ch->name()), obj_object->name()))
          has_consent = true;
        if (!has_consent && !isexact(qPrintable(ch->name()), obj_object->name()))
        {
          send_to_char("You don't have consent to take the "
                       "corpse.\r\n",
                       ch);
          return ReturnValue::eFAILURE;
        }
        has_consent = false; // reset it
      }

      if (isSet(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
          !isexact(qPrintable(ch->name()), obj_object->name()) && ch->getLevel() < IMPLEMENTER)
      {
        ch->send(u"The %1 appears to be SPECIAL. Only its rightful owner can take it.\r\n"_s).arg(obj_object->short_description()));
      }
      else if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
               !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && !ch->isImmortalPlayer()))
      {
        dc_sprintf(buffer, "%s : You can't carry that many items.\r\n", qPrintable(fname(obj_object->name())));
        ch->send(buffer);
        fail = true;
      }
      else if ((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) > CAN_CARRY_W(ch) &&
               !ch->isImmortalPlayer() && GET_ITEM_TYPE(obj_object) != ITEM_MONEY)
      {
        dc_sprintf(buffer, "%s : You can't carry that much weight.\r\n", qPrintable(fname(obj_object->name())));
        ch->send(buffer);
        fail = true;
      }
      else if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
               obj_object->obj_flags.value[0] > 10000 &&
               ch->getLevel() < 5)
      {
        ch->sendln("You cannot pick up that much money!");
        fail = true;
      }

      else if (obj_object->obj_flags.eq_level > 19 && ch->getLevel() < 5)
      {
        if (ch->in_room != real_room(3099))
        {
          ch->send(u"%1 is too powerful for you to possess.\r\n"_s).arg(obj_object->short_description()));
          fail = true;
        }
        else
        {
          ch->send(u"The aura of the donation room allows you to pick up %1.\r\n"_s).arg(obj_object->short_description()));
          get(ch, obj_object, sub_object, 0, cmd);
          ch->save(cmd_t::SAVE_SILENTLY);
          found = true;
        }
      }
      else if (CAN_WEAR(obj_object, TAKE))
      {
        if (cmd == cmd_t::PALM)
          palm(ch, obj_object, sub_object, 0);
        else
          get(ch, obj_object, sub_object, 0, cmd);

        ch->save(cmd_t::SAVE_SILENTLY);
        found = true;
      }
      else
      {
        ch->sendln("You can't take that.");
        fail = true;
      }
    }
    else
    {
      dc_sprintf(buffer, "You do not see a %s here.\r\n", arg1);
      ch->send(buffer);
      fail = true;
    }
  }
  break;
  /* get all all */
  case 3:
  {
    ch->sendln("You must be joking?!");
  }
  break;
  /* get all ??? */
  case 4:
  {
    found = false;
    fail = false;
    sub_object = get_obj_in_list_vis(ch, arg2,
                                     DC::getInstance()->world[ch->in_room].contents);
    if (!sub_object)
    {
      sub_object = get_obj_in_list_vis(ch, arg2, ch->carrying);
      inventorycontainer = true;
    }

    if (sub_object)
    {
      if (sub_object->obj_flags.type_flag == ITEM_CONTAINER &&
          ((sub_object->obj_flags.value[3] == 1 &&
            isexact("pc", sub_object->name())) ||
           isexact("thiefcorpse", sub_object->name())))
      {
        dc_sprintf(buffer, "%s_consent", qPrintable(ch->name()));
        if ((isexact("thiefcorpse", sub_object->name()) && !isexact(qPrintable(ch->name()), sub_object->name())) || isexact(buffer, sub_object->name()) || ch->getLevel() > 105)
          has_consent = true;
        if (!has_consent && !isexact(qPrintable(ch->name()), sub_object->name()))
        {
          ch->sendln("You don't have consent to touch the corpse.");
          return ReturnValue::eFAILURE;
        }
      }
      if (ARE_CONTAINERS(sub_object))
      {
        if (isSet(sub_object->obj_flags.value[1], CONT_CLOSED))
        {
          dc_sprintf(buffer, "The %s is closed.\r\n", qPrintable(fname(sub_object->name())));
          ch->send(buffer);
          return ReturnValue::eFAILURE;
        }
        for (obj_object = sub_object->contains;
             obj_object;
             obj_object = next_obj)
        {
          next_obj = obj_object->next_content;
          if (GET_ITEM_TYPE(obj_object) == ITEM_CONTAINER && contains_no_trade_item(obj_object))
          {
            ch->send(u"%s : It seems magically attached to the corpse.\r\n"_s.arg(qPrintable(fname(obj_object->name()))));
            continue;
          } /*
       ObjectPtr temp,*next_contentthing;
       for (temp = obj_object->contains;temp;temp = next_contentthing)
       {
      next_contentthing = temp->next_content;
      if(isSet(temp->obj_flags.more_flags, ITEM_NO_TRADE))
      {
      ch->send(u"Whoa!  The %s inside the %s poofed into thin air!\r\n"_s.arg(temp->short_description,obj_object->short_description));
      extract_obj(temp);
      }
       }*/
          //		}
          /* IF all.obj, only get those named "obj" */
          if (alldot && !isexact(allbuf, obj_object->name()))
          {
            continue;
          }

          // Ignore NO_TRADE items on a 'get all'
          if (isSet(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) && ch->getLevel() < 100)
          {
            ch->send(u"The %1 appears to be NO_TRADE so you don't pick it up.\r\n"_s.arg(obj_object->short_description()));
            continue;
          }

          if (isSet(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
              !isexact(qPrintable(ch->name()), obj_object->name()) && ch->getLevel() < IMPLEMENTER)
          {
            ch->send(u"The %1 appears to be SPECIAL. Only its rightful owner can take it.\r\n"_s).arg(obj_object->short_description()));
            continue;
          }

          if (CAN_SEE_OBJ(ch, obj_object))
          {
            if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
                !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && !ch->isImmortalPlayer()))
            {
              dc_sprintf(buffer, "%s : You can't carry that many items.\r\n", qPrintable(fname(obj_object->name())));
              ch->send(buffer);
              fail = true;
            }
            else
            {
              if (inventorycontainer ||
                  (IS_CARRYING_W(ch) + obj_object->obj_flags.weight) < CAN_CARRY_W(ch) ||
                  ch->getLevel() > IMMORTAL || GET_ITEM_TYPE(obj_object) == ITEM_MONEY)
              {
                if (has_consent && isSet(obj_object->obj_flags.more_flags, ITEM_NO_TRADE))
                {
                  // if I have consent and i'm touching the corpse, then I shouldn't be able
                  // to pick up no_trade items because it is someone else's corpse.  If I am
                  // the other of the corpse, has_consent will be false.
                  if (!ch->isImmortalPlayer())
                  {
                    if (isexact(obj_object->name(), "thiefcorpse"))
                    {
                      ch->send(u"Whoa!  The %1 poofed into thin air!\r\n"_s.arg(obj_object->short_description()));
                      extract_obj(obj_object);
                      continue;
                    }
                    ch->send(u"%s : It seems magically attached to the corpse.\r\n"_s.arg(qPrintable(fname(obj_object->name()))));
                    continue;
                  }
                }
                if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
                    obj_object->obj_flags.value[0] > 10000 &&
                    ch->getLevel() < 5)
                {
                  ch->sendln("You cannot pick up that much money!");
                  continue;
                }

                if (sub_object->carried_by != ch && obj_object->obj_flags.eq_level > 9 && ch->getLevel() < 5)
                {
                  ch->send(u"%1 is too powerful for you to possess.\r\n"_s).arg(obj_object->short_description()));
                  continue;
                }

                if (CAN_WEAR(obj_object, TAKE))
                {
                  get(ch, obj_object, sub_object, 0, cmd);
                  found = true;
                }
                else
                {
                  ch->sendln("You can't take that.");
                  fail = true;
                }
              }
              else
              {
                dc_sprintf(buffer, "%s : You can't carry that much weight.\r\n", qPrintable(fname(obj_object->name())));
                ch->send(buffer);
                fail = true;
              }
            }
          }
        }
        if (!found && !fail)
        {
          dc_sprintf(buffer, "You do not see anything in the %s.\r\n", qPrintable(fname(sub_object->name())));
          ch->send(buffer);
          fail = true;
        }
      }
      else
      {
        dc_sprintf(buffer, "The %s is not a container.\r\n", qPrintable(fname(sub_object->name())));
        ch->send(buffer);
        fail = true;
      }
    }
    else
    {
      dc_sprintf(buffer, "You do not see or have the %s.\r\n", arg2);
      ch->send(buffer);
      fail = true;
    }
  }
  break;
  case 5:
  {
    ch->sendln("You can't take a thing from more than one container.");
  }
  break;
  case 6:
  { // get ??? ???
    found = false;
    fail = false;
    sub_object = get_obj_in_list_vis(ch, arg2,
                                     DC::getInstance()->world[ch->in_room].contents);
    if (!sub_object)
    {
      if (cmd == cmd_t::LOOT)
      {
        ch->sendln("You can only loot 1 item from a non-consented corpse.");
        return ReturnValue::eFAILURE;
      }

      sub_object = get_obj_in_list_vis(ch, arg2, ch->carrying, true);
      inventorycontainer = true;
    }
    if (sub_object)
    {
      if (sub_object->obj_flags.type_flag == ITEM_CONTAINER &&
          ((sub_object->obj_flags.value[3] == 1 &&
            isexact("pc", sub_object->name())) ||
           isexact("thiefcorpse", sub_object->name())))
      {
        dc_sprintf(buffer, "%s_consent", qPrintable(ch->name()));

        if ((cmd != cmd_t::LOOT && (isexact("thiefcorpse", sub_object->name()) && !isexact(qPrintable(ch->name()), sub_object->name()))) || isexact(buffer, sub_object->name()))
          has_consent = true;
        if (!isexact(qPrintable(ch->name()), sub_object->name()) && (cmd == cmd_t::LOOT && isexact("lootable", sub_object->name())) && !isSet(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED) && !isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) && ch->getLevel() >= 50)
          has_consent = true;
        if (!has_consent && !isexact(qPrintable(ch->name()), sub_object->name()))
        {
          send_to_char("You don't have consent to touch the "
                       "corpse.\r\n",
                       ch);
          return ReturnValue::eFAILURE;
        }
      }
      if (ARE_CONTAINERS(sub_object))
      {
        if (isSet(sub_object->obj_flags.value[1], CONT_CLOSED))
        {
          dc_sprintf(buffer, "The %s is closed.\r\n", qPrintable(fname(sub_object->name())));
          ch->send(buffer);
          return ReturnValue::eFAILURE;
        }
        obj_object = get_obj_in_list_vis(ch, arg1, sub_object->contains);
        if (!obj_object && IS_AFFECTED(ch, AFF_BLIND) && ch->has_skill(SKILL_BLINDFIGHTING))
        {
          obj_object = get_obj_in_list_vis(ch, arg1, sub_object->contains, true);
          blindlag = true;
        }
        if (obj_object)
        {
          if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
              obj_object->obj_flags.value[0] > 10000 &&
              ch->getLevel() < 5)
          {
            ch->sendln("You cannot pick up that much money!");
            fail = true;
          }

          else if (sub_object->carried_by != ch && obj_object->obj_flags.eq_level > 9 && ch->getLevel() < 5)
          {
            ch->send(u"%1 is too powerful for you to possess.\r\n"_s).arg(obj_object->short_description()));
            fail = true;
          }

          else if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
                   !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && !ch->isImmortalPlayer()))
          {
            dc_sprintf(buffer, "%s : You can't carry that many items.\r\n", qPrintable(fname(obj_object->name())));
            ch->send(buffer);
            fail = true;
          }
          else if (inventorycontainer ||
                   (IS_CARRYING_W(ch) + obj_object->obj_flags.weight) < CAN_CARRY_W(ch) || GET_ITEM_TYPE(obj_object) == ITEM_MONEY)
          {
            if (has_consent && (isSet(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) ||
                                contains_no_trade_item(obj_object)))
            {
              // if I have consent and i'm touching the corpse, then I shouldn't be able
              // to pick up no_trade items because it is someone else's corpse.  If I am
              // the other of the corpse, has_consent will be false.
              if (!ch->isImmortalPlayer())
              {
                if (isexact("thiefcorpse", sub_object->name()) || (cmd == cmd_t::LOOT && isexact("lootable", sub_object->name())))
                {
                  ch->send(u"Whoa!  The %1 poofed into thin air!\r\n"_s.arg(obj_object->short_description()));

                  QString log_buf = {};
                  dc_sprintf(log_buf, "%s poofed %s[%d] from %s[%d]",
                             qPrintable(ch->name()),
                             obj_object->short_description,
                             DC::getInstance()->obj_index[obj_object->item_number].vnum(),
                             qPrintable(sub_object->name()),
                             DC::getInstance()->obj_index[sub_object->item_number].vnum());
                  DC::getInstance()->logentry(log_buf, ANGEL, DC::LogChannel::LOG_MORTAL);

                  extract_obj(obj_object);
                  fail = true;
                  dc_sprintf(buffer, "%s_consent", qPrintable(ch->name()));

                  if ((cmd == cmd_t::LOOT && isexact("lootable", sub_object->name())) && !isexact(buffer, sub_object->name()))
                  {
                    SET_BIT(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED);
                    affected_type pthiefaf;

                    pthiefaf.type = Character::PLAYER_OBJECT_THIEF;
                    pthiefaf.duration = 10;
                    pthiefaf.modifier = {};
                    pthiefaf.location = APPLY_NONE;
                    pthiefaf.bitvector = -1;

                    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
                    ch->sendln("You suddenly feel very guilty...shame on you stealing from the dead!");
                    if (ch->isPlayerObjectThief())
                    {
                      affect_from_char(ch, Character::PLAYER_OBJECT_THIEF);
                      affect_to_char(ch, &pthiefaf);
                    }
                    else
                      affect_to_char(ch, &pthiefaf);
                  }
                }
                else
                {
                  ch->send(u"%s : It seems magically attached to the corpse.\r\n"_s.arg(qPrintable(fname(obj_object->name()))));
                  fail = true;
                }
              }
            }
            else if (CAN_WEAR(obj_object, TAKE))
            {
              if (cmd == cmd_t::PALM)
                palm(ch, obj_object, sub_object, has_consent);
              else
                get(ch, obj_object, sub_object, has_consent, cmd);
              found = true;
              if (blindlag)
                WAIT_STATE(ch, DC::PULSE_VIOLENCE);
            }
            else
            {
              ch->sendln("You can't take that.");
              fail = true;
            }
          }
          else
          {
            dc_sprintf(buffer, "%s : You can't carry that much weight.\r\n", qPrintable(fname(obj_object->name())));
            ch->send(buffer);
            fail = true;
          }
        }
        else
        {
          dc_sprintf(buffer, "The %s does not contain the %s.\r\n", qPrintable(fname(sub_object->name())), arg1);
          ch->send(buffer);
          fail = true;
        }
      }
      else
      {
        dc_sprintf(buffer, "The %s is not a container.\r\n", qPrintable(fname(sub_object->name())));
        ch->send(buffer);
        fail = true;
      }
    }
    else
    {
      dc_sprintf(buffer, "You do not see or have the %s.\r\n", arg2);
      ch->send(buffer);
      fail = true;
    }
  }
  break;
  }
  if (fail)
    return ReturnValue::eFAILURE;
  ch->save(cmd_t::SAVE_SILENTLY);
  return ReturnValue::eSUCCESS;
}

command_return_t do_consent(CharacterPtr ch, QString arg, cmd_t cmd)
{
  ObjectPtr obj = {};
  CharacterPtr vict = {};

  auto arguments = QString(arg).trimmed().split(' ');
  auto arg1 = arguments.value(0);

  if (arg1.isEmpty())
  {
    ch->sendln("Give WHO consent to touch your rotting carcass?");
    return ReturnValue::eFAILURE;
  }

  if (!(vict = get_char_vis(ch, arg1)))
  {
    ch->send(u"Consent whom?  You can't see any %1.\r\n"_s.arg(arg1));
    return ReturnValue::eFAILURE;
  }

  if (vict == ch)
  {
    ch->sendln("Silly, you don't need to consent yourself!");
    return ReturnValue::eFAILURE;
  }
  if (vict->getLevel() < 10)
  {
    ch->sendln("That person is too low level to be consented.");
    return ReturnValue::eFAILURE;
  }

  // prevent consenting of NPCs
  if (vict->isNonPlayer())
  {
    ch->sendln("Now what business would THAT thing have with your mortal remains?");
    return ReturnValue::eFAILURE;
  }

  for (obj = DC::getInstance()->object_list; obj; obj = obj->next)
  {
    if (obj->obj_flags.type_flag != ITEM_CONTAINER || obj->obj_flags.value[3] != 1 || obj->name().isEmpty())
      continue;

    if (!isexact(qPrintable(ch->name()), obj->name()))
      // corpse isn't owned by the consenting player
      continue;

    // check to see if this player is already consented for the corpse
    if (isexact(u"%1_consent"_s.arg(arg1), obj->name()))
      // keep looking; there might be other corpses not yet consented
      continue;

    // check for buffer overflow before adding the new name to the list
    if (obj->name().size() + arg1.size() + u" _consent"_s.size() > (MAX_STRING_LENGTH / 2))
    {
      ch->sendln("Don't you think there are enough perverts molesting your");
      ch->sendln("maggot-ridden corpse already?");
      return ReturnValue::eFAILURE;
    }
    auto buf2 = u"%1 %1_consent"_s.arg(obj->name()).arg(arg1);
    obj->name(buf2);
  }

  ch->sendln(u"All corpses in the game which belong to you can now be molested by anyone named %1."_s).arg(arg1));
  return ReturnValue::eSUCCESS;
}

qint32 contents_cause_unique_problem(ObjectPtr obj, CharacterPtr vict)
{
  qint32 lastnum = -1;

  for (ObjectPtr inside = obj->contains; inside; inside = inside->next_content)
  {
    if (inside->item_number < 0) // skip -1 items
      continue;
    if (lastnum == inside->item_number) // items are in order.  If we've already checked
      continue;                         // this item, don't do it again.

    if (isSet(inside->obj_flags.more_flags, ITEM_UNIQUE) &&
        search_char_for_item(vict, inside->item_number, false))
      return true;
    lastnum = inside->item_number;
  }
  return false;
}

qint32 contains_no_trade_item(ObjectPtr obj)
{
  ObjectPtr inside = obj->contains;

  while (inside)
  {
    if (isSet(inside->obj_flags.more_flags, ITEM_NO_TRADE))
      return true;
    inside = inside->next_content;
  }

  return false;
}

command_return_t do_drop(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  qint32 amount;
  QString buffer;
  ObjectPtr tmp_object;
  ObjectPtr next_obj;
  bool test = false, blindlag = false;
  QString alldot;

  alldot[0] = '\0';

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, arg);

  if (is_number(arg))
  {
    if (!ch->isNonPlayer() && ch->isPlayerGoldThief())
    {
      ch->sendln("Your criminal acts prohibit it.");
      return ReturnValue::eFAILURE;
    }

    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (str_cmp("coins", arg) && str_cmp("coin", arg) && str_cmp("gold", arg))
    {
      ch->sendln("Sorry, you can't do that (yet)...");
      return ReturnValue::eFAILURE;
    }
    if (amount < 0)
    {
      ch->sendln("Sorry, you can't do that!");
      return ReturnValue::eFAILURE;
    }
    if (ch->getGold() < (quint32)amount)
    {
      ch->sendln("You haven't got that many coins!");
      return ReturnValue::eFAILURE;
    }
    ch->sendln("OK.");
    if (amount == 0)
      return ReturnValue::eSUCCESS;

    act_to_room("$n drops some gold.", ch, 0, 0, 0);
    tmp_object = create_money(amount);
    obj_to_room(tmp_object, ch->in_room);
    ch->removeGold(amount);
    if (ch->isImmortalPlayer())
    {
      special_log(QString(u"%1 dropped %2 coins in room %3!"_s).arg(ch->name()).arg(amount).arg(ch->in_room));
    }

    ch->save(cmd_t::SAVE_SILENTLY);
    return ReturnValue::eSUCCESS;
  }

  if (*arg)
  {
    if (!str_cmp(arg, "all") || sscanf(arg, "all.%s", alldot) != 0)
    {
      for (tmp_object = ch->carrying; tmp_object; tmp_object = next_obj)
      {
        next_obj = tmp_object->next_content;

        if (alldot[0] != '\0' && !isexact(alldot, tmp_object->name()))
          continue;

        if (isSet(tmp_object->obj_flags.extra_flags, ITEM_SPECIAL))
          continue;

        if (!ch->isNonPlayer() && ch->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
        {
          ch->sendln("Your criminal acts prohibit it.");
          return ReturnValue::eFAILURE;
        }
        if (isSet(tmp_object->obj_flags.more_flags, ITEM_NO_TRADE))
          continue;
        if (contains_no_trade_item(tmp_object))
          continue;
        if (!isSet(tmp_object->obj_flags.extra_flags, ITEM_NODROP) ||
            ch->isImmortalPlayer())
        {
          if (isSet(tmp_object->obj_flags.extra_flags, ITEM_NODROP))
            ch->sendln("(This item is cursed, BTW.)");
          if (CAN_SEE_OBJ(ch, tmp_object))
          {
            ch->sendln(u"You drop the %1."_s).arg(tmp_object->short_description));
          }
          else if (CAN_SEE_OBJ(ch, tmp_object, true))
          {
            ch->sendln(u"You drop the %1."_s).arg(tmp_object->short_description));
            blindlag = true;
          }
          else
            ch->sendln("You drop something.");

          if (tmp_object->obj_flags.type_flag != ITEM_MONEY)
          {
            QString log_buf = {};
            dc_sprintf(log_buf, "%s drops %s[%d] in room %d", qPrintable(ch->name()), tmp_object->short_description, DC::getInstance()->obj_index[tmp_object->item_number].vnum(), ch->in_room);
            DC::getInstance()->logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
            for (ObjectPtr loop_obj = tmp_object->contains; loop_obj; loop_obj = loop_obj->next_content)
              DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]",
                                      tmp_object->short_description,
                                      loop_obj->short_description,
                                      DC::getInstance()->obj_index[loop_obj->item_number].vnum());
          }

          act_to_room("$n drops $p.", ch, tmp_object, 0, INVIS_NULL);
          move_obj(tmp_object, ch->in_room);
          test = true;
          if (blindlag)
            WAIT_STATE(ch, DC::PULSE_VIOLENCE);
        }
        else
        {
          if (CAN_SEE_OBJ(ch, tmp_object, true))
          {
            dc_sprintf(buffer, "You can't drop %s, it must be CURSED!\r\n", tmp_object->short_description);
            ch->send(buffer);
            test = true;
          }
        }
      } /* for */

      if (!test)
        ch->sendln("You do not seem to have anything.");

    } /* if dc_strcmp "all" */

    else
    {
      tmp_object = get_obj_in_list_vis(ch, arg, ch->carrying);
      if (tmp_object)
      {

        if (!ch->isNonPlayer() && ch->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
        {
          ch->sendln("Your criminal acts prohibit it.");
          return ReturnValue::eFAILURE;
        }
        if (isSet(tmp_object->obj_flags.more_flags, ITEM_NO_TRADE) && !ch->isImmortalPlayer())
        {
          ch->sendln("It seems magically attached to you.");
          return ReturnValue::eFAILURE;
        }
        if (contains_no_trade_item(tmp_object))
        {
          ch->sendln("Something inside it seems magically attached to you.");
          return ReturnValue::eFAILURE;
        }

        if (isSet(tmp_object->obj_flags.extra_flags, ITEM_SPECIAL))
        {
          ch->sendln("You can't drop godload items.");
          return ReturnValue::eFAILURE;
        }
        else if (!isSet(tmp_object->obj_flags.extra_flags, ITEM_NODROP) ||
                 ch->isImmortalPlayer())
        {
          if (isSet(tmp_object->obj_flags.extra_flags, ITEM_NODROP))
            ch->sendln("(This item is cursed, BTW.)");
          ch->sendln(u"You drop the %1."_s).arg(tmp_object->short_description));
          act_to_room("$n drops $p.", ch, tmp_object, 0, INVIS_NULL);
          if (tmp_object->obj_flags.type_flag != ITEM_MONEY)
          {
            QString log_buf = {};
            dc_sprintf(log_buf, "%s drops %s[%d] in room %d", qPrintable(ch->name()), tmp_object->short_description, DC::getInstance()->obj_index[tmp_object->item_number].vnum(), ch->in_room);
            DC::getInstance()->logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
            for (ObjectPtr loop_obj = tmp_object->contains; loop_obj; loop_obj = loop_obj->next_content)
              DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]",
                                      tmp_object->short_description,
                                      loop_obj->short_description,
                                      DC::getInstance()->obj_index[loop_obj->item_number].vnum());
          }

          move_obj(tmp_object, ch->in_room);
          return ReturnValue::eSUCCESS;
        }
        else
          ch->sendln("You can't drop it, it must be CURSED!");
      }
      else
        ch->sendln("You do not have that item.");
    }
    ch->save(cmd_t::SAVE_SILENTLY);
  }
  else
    ch->sendln("Drop what?");
  return ReturnValue::eFAILURE;
}

void do_putalldot(CharacterPtr ch, QString name, QString target, cmd_t cmd)
{
  ObjectPtr tmp_object;
  ObjectPtr next_object;
  QString buf;
  bool found = false;

  /* If "put all.object bag", get all carried items
   * named "object", and put each into the bag.
   */

  for (tmp_object = ch->carrying; tmp_object; tmp_object = next_object)
  {
    next_object = tmp_object->next_content;
    if (!name && CAN_SEE_OBJ(ch, tmp_object))
    {
      dc_sprintf(buf, "%s %s", qPrintable(fname(tmp_object->name())), target);
      buf[99] = {};
      found = true;
      do_put(ch, buf, cmd);
    }
    else if (isexact(name, tmp_object->name()) && CAN_SEE_OBJ(ch, tmp_object))
    {
      dc_sprintf(buf, "%s %s", name, target);
      buf[99] = {};
      found = true;
      do_put(ch, buf, cmd);
    }
  }

  if (!found)
    ch->sendln("You don't have one.");
}

qint32 weight_in(ObjectPtr obj)
{ // Sheldon backpack. Damn procs.
  qint32 w = {};
  ObjectPtr obj2;
  for (obj2 = obj->contains; obj2; obj2 = obj2->next_content)
    w += obj2->obj_flags.weight;
  return w;
}

command_return_t do_put(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buffer;
  QString arg1;
  QString arg2;
  ObjectPtr obj_object;
  ObjectPtr sub_object;
  CharacterPtr tmp_char;
  qint32 bits;
  QString allbuf;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  argument_interpreter(argument, arg1, arg2);

  if (*arg1)
  {
    if (*arg2)
    {
      if (!(get_obj_in_list_vis(ch, arg2, ch->carrying)) && !(get_obj_in_list_vis(ch, arg2, DC::getInstance()->world[ch->in_room].contents)))
      {
        dc_sprintf(buffer, "You don't have a %s.\r\n", arg2);
        ch->send(buffer);
        return 1;
      }
      allbuf[0] = '\0';
      if (!str_cmp(arg1, "all"))
      {
        do_putalldot(ch, 0, arg2, cmd);
        return ReturnValue::eSUCCESS;
      }
      else if (sscanf(arg1, "all.%s", allbuf) != 0)
      {
        do_putalldot(ch, allbuf, arg2, cmd);
        return ReturnValue::eSUCCESS;
      }
      obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);

      if (obj_object)
      {
        if (isSet(obj_object->obj_flags.extra_flags, ITEM_NODROP))
        {
          if (!ch->isImmortalPlayer())
          {
            ch->sendln("You are unable to! That item must be CURSED!");
            return ReturnValue::eFAILURE;
          }
          else
            ch->sendln("(This item is cursed, BTW.)");
        }
        if (DC::getInstance()->obj_index[obj_object->item_number].vnum() == CHAMPION_ITEM)
        {
          ch->sendln("You must display this flag for all to see!");
          return ReturnValue::eFAILURE;
        }
        if (isSet(obj_object->obj_flags.extra_flags, ITEM_NEWBIE))
        {
          ch->sendln("The protective enchantment this item holds cannot be held within this container.");
          return ReturnValue::eFAILURE;
        }
        if (ARE_CONTAINERS(obj_object))
        {
          ch->sendln("You can't put that in there.");
          return ReturnValue::eFAILURE;
        }

        bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM,
                            ch, &tmp_char, &sub_object);
        if (sub_object)
        {
          if (ARE_CONTAINERS(sub_object))
          {
            // Keyrings can only hold keys
            if (GET_ITEM_TYPE(sub_object) == ITEM_KEYRING && GET_ITEM_TYPE(obj_object) != ITEM_KEY)
            {
              ch->send(u"You can't put %s on a keyring.\r\n"_s.arg(GET_OBJ_SHORT(obj_object)));
              return ReturnValue::eFAILURE;
            }

            // Altars can only hold totems
            if (GET_ITEM_TYPE(sub_object) == ITEM_ALTAR && GET_ITEM_TYPE(obj_object) != ITEM_TOTEM)
            {
              ch->sendln("You cannot put that in an altar.");
              return ReturnValue::eFAILURE;
            }

            if (!isSet(sub_object->obj_flags.value[1], CONT_CLOSED))
            {
              // Can't put an item in itself
              if (obj_object == sub_object)
              {
                ch->sendln("You attempt to fold it into itself, but fail.");
                return ReturnValue::eFAILURE;
              }

              // Can't put godload in non-godload
              if (IS_SPECIAL(obj_object) && NOT_SPECIAL(sub_object))
              {
                ch->sendln("Are you crazy?!  Someone could steal it!");
                return ReturnValue::eFAILURE;
              }

              // Can't put NO_TRADE item in someone else's container/altar/totem
              if (isSet(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) &&
                  sub_object->carried_by != ch)
              {
                ch->sendln("You can't trade that item.");
                return ReturnValue::eFAILURE;
              }

              if (isSet(obj_object->obj_flags.more_flags, ITEM_UNIQUE) && search_container_for_item(sub_object, obj_object->item_number))
              {
                ch->sendln("The object's uniqueness prevents it!");
                return ReturnValue::eFAILURE;
              }

              bool duplicate_key = search_container_for_item(sub_object, obj_object->item_number);
              if (GET_ITEM_TYPE(sub_object) == ITEM_KEYRING)
              {
                if (duplicate_key == true)
                {
                  if (ch && ch->player && isSet(ch->player->toggles, Player::PLR_NODUPEKEYS))
                  {
                    ch->send(u"A duplicate of %s is already on your keyring so you will not attach another one.\r\n"_s.arg(GET_OBJ_SHORT(obj_object)));
                    return ReturnValue::eFAILURE;
                  }
                  else
                  {
                    ch->send(u"A duplicate of %s is already on your keyring but you don't care.\r\n"_s.arg(GET_OBJ_SHORT(obj_object)));
                  }
                }
              }

              if (((sub_object->obj_flags.weight) +
                   (obj_object->obj_flags.weight)) <=
                      (sub_object->obj_flags.value[0]) &&
                  (DC::getInstance()->obj_index[sub_object->item_number].vnum() != 536 ||
                   weight_in(sub_object) + obj_object->obj_flags.weight <= 200))
              {
                if (bits == FIND_OBJ_INV)
                {
                  obj_from_char(obj_object);
                  /* make up for above line */
                  if (DC::getInstance()->obj_index[sub_object->item_number].vnum() != 536)
                    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj_object);
                  obj_to_obj(obj_object, sub_object);
                }
                else
                {
                  move_obj(obj_object, sub_object);
                }

                if (GET_ITEM_TYPE(sub_object) == ITEM_KEYRING)
                {
                  act_to_room("$n attaches $p to $P.", ch, obj_object, sub_object, INVIS_NULL);
                  act_to_character("You attach $p to $P.", ch, obj_object, sub_object, 0);
                  DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "%s attaches %s[%d] to %s[%d]",
                                          qPrintable(ch->name()),
                                          obj_object->short_description,
                                          DC::getInstance()->obj_index[obj_object->item_number].vnum(),
                                          sub_object->short_description,
                                          DC::getInstance()->obj_index[sub_object->item_number].vnum());
                }
                else
                {
                  act_to_room("$n puts $p in $P.", ch, obj_object, sub_object, INVIS_NULL);
                  act_to_character("You put $p in $P.", ch, obj_object, sub_object, 0);
                  DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "%s puts %s[%d] in %s[%d]",
                                          qPrintable(ch->name()),
                                          obj_object->short_description,
                                          DC::getInstance()->obj_index[obj_object->item_number].vnum(),
                                          sub_object->short_description,
                                          DC::getInstance()->obj_index[sub_object->item_number].vnum());
                }

                return ReturnValue::eSUCCESS;
              }
              else
              {
                ch->sendln("It won't fit.");
              }
            }
            else
              ch->sendln("It seems to be closed.");
          }
          else
          {
            dc_sprintf(buffer, "The %s is not a container.\r\n", qPrintable(fname(sub_object->name())));
            ch->send(buffer);
          }
        }
        else
        {
          dc_sprintf(buffer, "You dont have the %s.\r\n", arg2);
          ch->send(buffer);
        }
      }
      else
      {
        dc_sprintf(buffer, "You dont have the %s.\r\n", arg1);
        ch->send(buffer);
      }
    } /* if arg2 */
    else
    {
      dc_sprintf(buffer, "Put %s in what?\r\n", arg1);
      ch->send(buffer);
    }
  } /* if arg1 */
  else
  {
    ch->sendln("Put what in what?");
  }
  return ReturnValue::eFAILURE;
}

command_return_t Character::do_givealldot(QString name, QString target, cmd_t cmd)
{
  bool found = false;

  for (auto tmp_object = carrying, next_object = carrying; tmp_object; tmp_object = next_object)
  {
    next_object = tmp_object->next_content;
    if (name.isEmpty() && CAN_SEE_OBJ(this, tmp_object))
    {
      found = true;
      do_give({fname(tmp_object->name()), target});
    }
    else if (isexact(name, tmp_object->name()) && CAN_SEE_OBJ(this, tmp_object))
    {
      found = true;
      do_give({name, target});
    }
  }

  if (!found)
  {
    sendln("You don't have one.");
    return ReturnValue::eFAILURE;
  }

  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_give(QStringList arguments, cmd_t cmd)
{
  auto &arena = DC::getInstance()->arena_;
  command_return_t retval = {};
  CharacterPtr vict = {};
  ObjectPtr obj = {};

  if (isSet(DC::getInstance()->world[in_room].room_flags, QUIET))
  {
    sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  if (isPlayerObjectThief())
  {
    sendln("Your criminal actions prohibit it.");
    return ReturnValue::eFAILURE;
  }
  auto obj_name = arguments.value(0);
  if (!arguments.isEmpty())
    arguments.removeFirst();

  if (is_number(obj_name))
  {
    if (!this->isNonPlayer() && isPlayerGoldThief())
    {
      sendln("Your criminal acts prohibit it.");
      return ReturnValue::eFAILURE;
    }
    bool ok = false;
    auto amount = obj_name.toULongLong(&ok);
    auto arg = arguments.value(0);
    if (!arguments.isEmpty())
      arguments.removeFirst();

    if (arg != u"gold"_s && arg != u"coins"_s && arg != u"coin"_s)
    {
      sendln("Sorry, you can't do that (yet)...");
      return ReturnValue::eFAILURE;
    }
    if (!amount || !ok)
    {
      sendln("Sorry, you can't do that!");
      return ReturnValue::eFAILURE;
    }
    if (getGold() < amount && getLevel() < DEITY)
    {
      sendln("You haven't got that many coins!");
      return ReturnValue::eFAILURE;
    }
    auto vict_name = arguments.value(0);
    if (vict_name.isEmpty())
    {
      sendln("To whom?");
      return ReturnValue::eFAILURE;
    }

    if (!(vict = get_char_room_vis(vict_name)))
    {
      sendln("To whom?");
      return ReturnValue::eFAILURE;
    }

    if (this == vict)
    {
      sendln("Umm okay, you give it to yourself.");
      return ReturnValue::eFAILURE;
    }
    /*
          if(vict->getGold() > 2000000000) {
             sendln("They can't hold that much gold!");
             return ReturnValue::eFAILURE;
          }
    */
    sendln(u"You give %1 coin%2 to %3."_s).arg(amount).arg(amount == 1 ? "" : "s").arg(qPrintable(vict->shortdesc_or_name())));
    logobjects(u"%1 gives %2 coin%3 to %4"_s.arg(qPrintable(this->name())).arg(amount).arg(pluralize(amount)).arg(qPrintable(vict->name())));
    act_to_victim(u"%1 gives you %2 $B$5gold$R coin%3."_s.arg(PERS(this, vict)).arg(amount).arg(amount == 1 ? "" : "s"), this, 0, vict, INVIS_NULL);
    act_to_room("$n gives some gold to $N.", this, 0, vict, INVIS_NULL | NOTVICT);

    removeGold(amount);

    if (this->isNonPlayer() && (!IS_AFFECTED(this, AFF_CHARM) || getLevel() > 50))
    {
      special_log(QString(u"%1 (mob) giving %2 gold to %3 in room %4."_s).arg(name()).arg(amount).arg(vict->name()).arg(in_room));
    }

    if (getGold() < 0)
    {
      setGold(0);
      sendln("Warning:  You are giving out more $B$5gold$R than you had.");
      if (getLevel() < IMPLEMENTER)
      {
        special_log(QString(u"%1 gives %2 coins to %3 (negative!) in room %4."_s).arg(name()).arg(amount).arg(vict->name()).arg(in_room));
      }
    }
    vict->addGold(amount);

    // If a mob is given gold, we disable its ability to receive a gold bonus. This keeps
    // the mob from turning into an interest bearing savings account. :)
    if (vict->isNonPlayer())
    {
      SETBIT(vict->mobdata->actflags, ACT_NO_GOLD_BONUS);
    }

    save();
    vict->save();
    // bribe trigger automatically removes any gold given to mob
    mprog_bribe_trigger(vict, this, amount);

    return ReturnValue::eSUCCESS;
  }

  auto vict_name = arguments.value(0);

  if (obj_name.isEmpty() || vict_name.isEmpty())
  {
    sendln("Give what to whom?");
    return ReturnValue::eFAILURE;
  }

  if (vict_name == u"follower"_s)
  {
    bool found = false;
    follow_type *k;
    qint32 org_room = in_room;
    if (followers)
      for (k = followers; k && k != (follow_type *)0x95959595; k = k->next)
      {
        if (org_room == k->follower->in_room)
          if (IS_AFFECTED(k->follower, AFF_CHARM))
          {
            vict = k->follower;
            found = true;
          }
      }

    if (!found)
    {
      sendln("Nobody here are loyal subjects of yours!");
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    if (!(vict = get_char_room_vis(vict_name)))
    {
      sendln("No one by that name around here.");
      return ReturnValue::eFAILURE;
    }
  }

  if (this == vict)
  {
    sendln("Why give yourself stuff?");
    return ReturnValue::eFAILURE;
  }

  if (obj_name == u"all"_s)
  {
    do_givealldot({}, vict_name, cmd);
    return ReturnValue::eSUCCESS;
  }
  else if (obj_name.startsWith(u"all."_s))
  {
    auto allbuf = obj_name.split(".").value(1);
    do_givealldot(allbuf, vict_name, cmd);
    return ReturnValue::eSUCCESS;
  }

  if (!(obj = get_obj_in_list_vis(this, obj_name, carrying)))
  {
    sendln("You do not seem to have anything like that.");
    return ReturnValue::eFAILURE;
  }
  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL) && getLevel() < OVERSEER)
  {
    sendln("That sure would be a fucking stupid thing to do.");
    return ReturnValue::eFAILURE;
  }

  if (!this->isNonPlayer() && affected_by_spell(Character::PLAYER_OBJECT_THIEF))
  {
    sendln("Your criminal acts prohibit it.");
    return ReturnValue::eFAILURE;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_NODROP))
  {
    if (getLevel() < DEITY)
    {
      sendln("You can't let go of it! Yeech!!");
      return ReturnValue::eFAILURE;
    }
    else
      sendln("This item is NODROP btw.");
  }

  // Handle no-trade items
  if (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE))
  {
    // Mortal this can give immortal vict no-trade items
    if (!isImmortalPlayer() && vict->isImmortalPlayer())
    {
      sendln("It seems to no longer be magically attached to you.");
    }
    else
    {
      // You can give no_trade items to mobs for quest purposes.  It's taken care of later
      if (vict->isPlayer() && (this->isPlayer() || IS_AFFECTED(this, AFF_CHARM)))
      {
        if (getLevel() > IMMORTAL)
          sendln("That was a NO_TRADE item btw....");
        else
        {
          sendln("It seems magically attached to you.");
          return ReturnValue::eFAILURE;
        }
      }
      if (contains_no_trade_item(obj))
      {
        if (getLevel() > IMMORTAL)
          sendln("That was a NO_TRADE item btw....");
        else
        {
          sendln("Something inside it seems magically attached to you.");
          return ReturnValue::eFAILURE;
        }
      }
    }
  }

  if (vict->isNonPlayer() && (DC::getInstance()->mob_index[vict->mobdata->nr].non_combat_func == shop_keeper || DC::getInstance()->mob_index[vict->mobdata->nr].vnum() == QUEST_MASTER))
  {
    act_to_character("$N graciously refuses your gift.", this, 0, vict, 0);
    return ReturnValue::eFAILURE;
  }
  if (vict->isNonPlayer() && IS_AFFECTED(vict, AFF_CHARM) && (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) || contains_no_trade_item(obj)))
  {
    sendln("The creature doesn't understand what you're trying to do.");
    return ReturnValue::eFAILURE;
  }

  if (!this->isNonPlayer() && isPlayerObjectThief() && !vict->desc)
  {
    sendln("Now WHY would a thief give something to a linkdead character..?");
    return ReturnValue::eFAILURE;
  }

  // Check if mortal this is trying to give more items to vict than they can carry
  if ((1 + IS_CARRYING_N(vict)) > CAN_CARRY_N(vict))
  {
    if (isImmortalPlayer())
    {
      act_to_character("$N seems to have $S hands full but you give it to $M anyways.", this, 0, vict, 0);
    }
    else
    {
      auto &arena = DC::getInstance()->arena_;
      if (in_room == -1 ||
          in_room > DC::getInstance()->top_of_world ||
          obj_name != u"potato"_s ||
          !room().isArena() ||
          !vict->room().isArena() ||
          !arena.isPotato())
      {
        act_to_character("$N seems to have $S hands full.", this, 0, vict, 0);
        return ReturnValue::eFAILURE;
      }
    }
  }

  // Check if mortal this is trying to give more weight to vict than they can carry
  if (obj->obj_flags.weight + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
  {
    if (isImmortalPlayer())
    {
      act_to_character("$E can't carry that much weight but you give it to $M anyways.", this, 0, vict, 0);
    }
    else
    {
      auto &arena = DC::getInstance()->arena_;
      if (in_room == -1 ||
          in_room > DC::getInstance()->top_of_world ||
          obj_name != u"potato"_s ||
          !room().isArena() ||
          !vict->room().isArena() ||
          !arena.isPotato())
      {
        act_to_character("$E can't carry that much weight.", this, 0, vict, 0);
        return ReturnValue::eFAILURE;
      }
    }
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE))
  {
    if (search_char_for_item(vict, obj->item_number, false))
    {
      sendln("The item's uniqueness prevents it.");
      vict->send(u"%s tried to give you an item but was unable.\r\n"_s.arg(qPrintable(this->shortdesc_or_name())));
      return ReturnValue::eFAILURE;
    }
  }
  if (contents_cause_unique_problem(obj, vict))
  {
    sendln("The uniqueness of something inside it prevents it.");
    vict->send(u"%s tried to give you an item but was unable.\r\n"_s.arg(qPrintable(this->shortdesc_or_name())));
    return ReturnValue::eFAILURE;
  }

  move_obj(obj, vict);
  act_to_room("$n gives $p to $N.", this, obj, vict, INVIS_NULL | NOTVICT);
  act_to_victim("$n gives you $p.", this, obj, vict, 0);
  act_to_character("You give $p to $N.", this, obj, vict, 0);

  logobjects(u"%1 gives %2 to %3"_s.arg(qPrintable(this->name())).arg(obj->name()).arg(qPrintable(vict->name())));
  for (ObjectPtr loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content)
    DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s[%d] contained %s[%d]",
                            qPrintable(obj->short_description()),
                            DC::getInstance()->obj_index[obj->item_number].vnum(),
                            loop_obj->short_description,
                            DC::getInstance()->obj_index[loop_obj->item_number].vnum());

  if ((vict->in_room >= 0 && vict->in_room <= DC::getInstance()->top_of_world) && vict->isMortalPlayer() &&
      vict->room().isArena() && arena.isPotato() && DC::getInstance()->obj_index[obj->item_number].vnum() == 393)
  {
    vict->sendln("Here, have some for some potato lag!!");
    WAIT_STATE(vict, DC::PULSE_VIOLENCE * 2);
  }

  //    sendln("Ok.");
  save();
  vict->save();
  // if I gave a no_trade item to a mob, the mob needs to destroy it
  // otherwise it defeats the purpose of no_trade:)

  retval = mprog_give_trigger(vict, this, obj);
  bool objExists(ObjectPtr obj);
  if (!isSet(retval, ReturnValue::eEXTRA_VALUE) && isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) && vict->isNonPlayer() &&
      objExists(obj))
    extract_obj(obj);

  if (SOMEONE_DIED(retval))
  {
    retval = SWAP_CH_VICT(retval);
    return ReturnValue::eSUCCESS | retval;
  }

  return ReturnValue::eSUCCESS;
}

// Find an item on a character (in inv, or containers in inv (NOT WORN!))
// and try to put it in his inv.  If sucessful, return pointer to the item.
ObjectPtr bring_type_to_front(CharacterPtr ch, qint32 item_type)
{
  ObjectPtr item_carried = {};
  ObjectPtr container_item = {};

  QQueue<ObjectPtr> container_queue;

  for (item_carried = ch->carrying; item_carried; item_carried = item_carried->next_content)
  {
    if (GET_ITEM_TYPE(item_carried) == item_type)
      return item_carried;
    if (GET_ITEM_TYPE(item_carried) == ITEM_CONTAINER && !isSet(item_carried->obj_flags.value[1], CONT_CLOSED))
    { // search inside if open
      container_queue.push(item_carried);
    }
  }

  //  if(GET_ITEM_TYPE(i) == ITEM_CONTAINER) { // search inside if open
  while (container_queue.size() > 0)
  {
    item_carried = container_queue.front();
    container_queue.pop();
    for (container_item = item_carried->contains; container_item; container_item = container_item->next_content)
    {
      if (GET_ITEM_TYPE(container_item) == item_type)
      {
        get(ch, container_item, item_carried, 0, cmd_t::DEFAULT);
        return container_item;
      }
    }
  }
  return {};
}

// Find an item on a character
ObjectPtr search_char_for_item(CharacterPtr ch, qint16 item_number, bool wearonly)
{
  ObjectPtr i = {};
  ObjectPtr j = {};
  qint32 k;

  for (k = {}; k < MAX_WEAR; k++)
  {
    if (ch->equipment[k])
    {
      if (ch->equipment[k]->item_number == item_number)
        return ch->equipment[k];
      if (GET_ITEM_TYPE(ch->equipment[k]) == ITEM_CONTAINER)
      { // search inside
        for (j = ch->equipment[k]->contains; j; j = j->next_content)
        {
          if (j->item_number == item_number)
            return j;
        }
      }
    }
  }
  if (!wearonly)
    for (i = ch->carrying; i; i = i->next_content)
    {
      if (i->item_number == item_number)
        return i;

      // does not support containers inside containers
      if (GET_ITEM_TYPE(i) == ITEM_CONTAINER)
      { // search inside
        for (j = i->contains; j; j = j->next_content)
        {
          if (j->item_number == item_number)
            return j;
        }
      }
    }
  return {};
}

// Find out how many of an item exists on character
qint32 search_char_for_item_count(CharacterPtr ch, qint16 item_number, bool wearonly)
{
  ObjectPtr i = {};
  ObjectPtr j = {};
  qint32 k;
  qint32 count = {};

  for (k = {}; k < MAX_WEAR; k++)
  {
    if (ch->equipment[k])
    {
      if (ch->equipment[k]->item_number == item_number)
        count++;
      if (GET_ITEM_TYPE(ch->equipment[k]) == ITEM_CONTAINER)
      { // search inside
        for (j = ch->equipment[k]->contains; j; j = j->next_content)
        {
          if (j->item_number == item_number)
            count++;
        }
      }
    }
  }

  if (!wearonly)
    for (i = ch->carrying; i; i = i->next_content)
    {
      if (i->item_number == item_number)
        count++;

      // does not support containers inside containers
      if (GET_ITEM_TYPE(i) == ITEM_CONTAINER)
      { // search inside
        for (j = i->contains; j; j = j->next_content)
        {
          if (j->item_number == item_number)
            count++;
        }
      }
    }

  return count;
}

bool search_container_for_item(ObjectPtr obj, qint32 item_number)
{
  if (obj == nullptr)
  {
    return false;
  }

  if (NOT_CONTAINERS(obj))
  {
    return false;
  }

  for (ObjectPtr i = obj->contains; i; i = i->next_content)
  {
    if (i->item_number == item_number)
    {
      return true;
    }
  }

  return false;
}

bool search_container_for_vnum(ObjectPtr obj, qint32 vnum)
{
  if (obj == nullptr)
  {
    return false;
  }

  if (NOT_CONTAINERS(obj))
  {
    return false;
  }

  for (ObjectPtr i = obj->contains; i; i = i->next_content)
  {
    if (DC::getInstance()->obj_index[i->item_number].vnum() == vnum)
    {
      return true;
    }
  }

  return false;
}

qint32 find_door(CharacterPtr ch, QString type, QString dir)
{
  qint32 door;
  const QStringList dirs =
      {
          "north",
          "east",
          "south",
          "west",
          "up",
          "down",
          "\n"};

  if (*dir) /* a direction was specified */
  {
    if ((door = search_block(dir, dirs, false)) == -1) /* Partial Match */
    {
      ch->sendln("That's not a direction.");
      return (-1);
    }

    if (EXIT(ch, door))
      if (EXIT(ch, door)->keyword)
        if (isexact(type, EXIT(ch, door)->keyword))
          return (door);
        else
        {
          return (-1);
        }
      else
        return (door);
    else
    {
      return (-1);
    }
  }
  else // try to locate the keyword
  {
    for (door = {}; door <= 5; door++)
      if (EXIT(ch, door))
        if (EXIT(ch, door)->keyword)
          if (isexact(type, EXIT(ch, door)->keyword))
            return (door);

    return (-1);
  }
}

/*
in_room == exit->in_room

*/
bool is_bracing(CharacterPtr bracee, room_direction_data *exit)
{
  // this could happen on a repop of the zone
  if (!isSet(exit->exit_info, EX_CLOSED))
    return false;

  // this could happen from some sort of bug
  if (isSet(exit->exit_info, EX_BROKEN))
    return false;

  // has to be standing
  if (GET_POS(bracee) < position_t::STANDING)
    return false;

  // if neither the spot bracee is at, nor remote spot, is equal to teh exit
  if (bracee->brace_at != exit && bracee->brace_exit != exit)
    return false;

  for (qint32 i = {}; i < 6; i++)
    if (DC::getInstance()->world[bracee->in_room].dir_option[i] == exit)
      return true;

  if (bracee->in_room == exit->to_room)
  {
    if (exit == bracee->brace_at || exit == bracee->brace_exit)
      return true;
  }

  return false;
}

command_return_t Character::do_open(QStringList arguments, cmd_t cmd)
{
  bool found{};
  qint32 door{}, other_room{}, retval{};
  room_direction_data *back{};
  ObjectPtr obj{};
  CharacterPtr victim{};
  CharacterPtr next_vict{};

  command_return_t do_fall(CharacterPtr ch, short dir);

  auto type = arguments.value(0);
  auto dir = arguments.value(1);

  if (type.isEmpty())
  {
    ch->sendln("Open what?");
    return ReturnValue::eFAILURE;
  }
  else if ((door = find_door(ch, type, dir)) >= 0)
  {
    found = true;
    if (!isSet(EXIT(ch, door)->exit_info, EX_ISDOOR))
      ch->sendln("That's impossible, I'm afraid.");
    else if (!isSet(EXIT(ch, door)->exit_info, EX_CLOSED))
      ch->sendln("It's already open!");
    else if (isSet(EXIT(ch, door)->exit_info, EX_LOCKED))
      ch->sendln("It seems to be locked.");
    else if (isSet(EXIT(ch, door)->exit_info, EX_BROKEN))
      ch->sendln("It's already been broken open!");
    else if (EXIT(ch, door)->bracee != nullptr)
    {
      if (is_bracing(EXIT(ch, door)->bracee, EXIT(ch, door)))
      {
        if (EXIT(ch, door)->bracee->in_room == ch->in_room)
        {
          ch->sendln(u"%1 is holding the %2 shut."_s).arg(EXIT(ch, door)->bracee->name()).arg(fname(EXIT(ch, door)->keyword)));
          EXIT(ch, door)->bracee->sendln(u"The %1 quivers slightly but holds as %2 attempts to force their way through."_s).arg(fname(EXIT(ch, door)->keyword).arg(ch->name()));
        }
        else
        {
          ch->sendln(u"The %1 seems to be barred from the other side."_s).arg(fname(EXIT(ch, door)->keyword)));
          EXIT(ch, door)->bracee->sendln(u"The %1 quivers slightly but holds as someone attempts to force their way through."_s).arg(fname(EXIT(ch, door)->keyword)));
        }
      }
      else
      {
        do_brace(EXIT(ch, door)->bracee, "");
        return ch->do_open(argument.split(' '), cmd);
      }
    }
    else if (isSet(EXIT(ch, door)->exit_info, EX_IMM_ONLY) && !ch->isImmortalPlayer())
      ch->sendln("It seems to slither and resist your attempt to touch it.");
    else
    {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);

      if (isSet(EXIT(ch, door)->exit_info, EX_HIDDEN))
      {
        if (EXIT(ch, door)->keyword)
        {
          act_to_room("$n reveals a hidden $F!", ch, 0, EXIT(ch, door)->keyword, 0);
          ch->send(u"You reveal a hidden %s!\r\n"_s.arg(qPrintable(fname(EXIT(ch).arg(door)->keyword))));
        }
        else
        {
          act_to_room("$n reveals a hidden door!", ch, 0, EXIT(ch, door)->keyword, 0);
          ch->sendln("You reveal a hidden door!");
        }
      }
      else
      {
        if (EXIT(ch, door)->keyword)
          act_to_room("$n opens the $F.", ch, 0, EXIT(ch, door)->keyword, 0);
        else
        {
          act_to_room("$n opens the door.", ch, 0, 0, 0);
        }
        ch->sendln("Ok.");
      }

      /* now for opening the OTHER side of the door! */
      if ((other_room = EXIT(ch, door)->to_room) != DC::NOWHERE)
        if ((back = DC::getInstance()->world[other_room].dir_option[rev_dir[door]]) != 0)
          if (back->to_room == ch->in_room)
          {
            REMOVE_BIT(back->exit_info, EX_CLOSED);
            if ((back->keyword) && !isSet(DC::getInstance()->world[EXIT(ch, door)->to_room].room_flags, QUIET))
            {
              dc_sprintf(buf, "The %s is opened from the other side.\r\n", qPrintable(fname(back->keyword)));
              send_to_room(buf, EXIT(ch, door)->to_room, true);
            }
            else
              send_to_room("The door is opened from the other side.\r\n",
                           EXIT(ch, door)->to_room, true);
          }

      if ((isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_DOWN) && (door = 5)) ||
          (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_UP) && (door = 4)) ||
          (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_EAST) && (door = 1)) ||
          (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_WEST) && (door = 3)) ||
          (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_SOUTH) && (door = 2)) ||
          (isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_NORTH) && (door = 0)))
      {
        qint32 success = {};

        // opened the door that kept them from falling out
        for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_vict)
        {
          next_vict = victim->next_in_room;
          if (victim->isNonPlayer() || IS_AFFECTED(victim, AFF_FLYING))
            continue;
          if (!success)
          {
            send_to_room("With the door no longer closed for support, this area's strange gravity takes over!\r\n", victim->in_room, true);
            success = 1;
          }
          if (victim == ch)
            retval = do_fall(victim, door);
          else
            do_fall(victim, door);
        }
      }
    }
  }
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &victim, &obj, true))
  {
    found = true;
    // this is an object
    if (obj->obj_flags.type_flag != ITEM_CONTAINER)
      ch->sendln("That's not a container.");
    else if (!isSet(obj->obj_flags.value[1], CONT_CLOSED))
      ch->sendln("But it's already open!");
    else if (!isSet(obj->obj_flags.value[1], CONT_CLOSEABLE))
      ch->sendln("You can't do that.");
    else if (isSet(obj->obj_flags.value[1], CONT_LOCKED))
      ch->sendln("It seems to be locked.");
    else
    {
      REMOVE_BIT(obj->obj_flags.value[1], CONT_CLOSED);
      ch->sendln("Ok.");
      act_to_room("$n opens $p.", ch, obj, 0, 0);
    }
  }

  if (found == false)
  {
    ch->send(u"I see no %1 here.\r\n"_s).arg(type));
  }

  // in case ch died or anything
  if (retval)
    return retval;
  return ReturnValue::eSUCCESS;
}

command_return_t do_close(CharacterPtr ch, QString argument, cmd_t cmd)
{
  bool found = false;
  qint32 door, other_room;
  QString type, dir, buf;
  room_direction_data *back;
  ObjectPtr obj;
  CharacterPtr victim;

  argument_interpreter(argument, type, dir);

  if (type.isEmpty())
  {
    ch->sendln("Close what?");
    return ReturnValue::eFAILURE;
  }
  else if ((door = find_door(ch, type, dir)) >= 0)
  {
    found = true;
    if (!isSet(EXIT(ch, door)->exit_info, EX_ISDOOR))
      ch->sendln("That's absurd.");
    else if (isSet(EXIT(ch, door)->exit_info, EX_CLOSED))
      ch->sendln("It's already closed!");
    else if (isSet(EXIT(ch, door)->exit_info, EX_BROKEN))
      ch->sendln("It appears to be broken!");
    else
    {
      SET_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
      if (EXIT(ch, door)->keyword)
        act_to_room("$n closes the $F.", ch, 0, EXIT(ch, door)->keyword, 0);
      else
        act_to_room("$n closes the door.", ch, 0, 0, 0);
      ch->sendln("Ok.");
      /* now for closing the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != DC::NOWHERE)
        if ((back = DC::getInstance()->world[other_room].dir_option[rev_dir[door]]) != 0)
          if (back->to_room == ch->in_room)
          {
            SET_BIT(back->exit_info, EX_CLOSED);
            if ((back->keyword) &&
                !isSet(DC::getInstance()->world[EXIT(ch, door)->to_room].room_flags, QUIET))
            {
              dc_sprintf(buf, "The %s closes quietly.\r\n", qPrintable(fname(back->keyword)));
              send_to_room(buf, EXIT(ch, door)->to_room, true);
            }
            else
              send_to_room("The door closes quietly.\r\n",
                           EXIT(ch, door)->to_room, true);
          }
    }
  }
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &victim, &obj, true))
  {
    found = true;
    if (obj->obj_flags.type_flag != ITEM_CONTAINER)
      ch->sendln("That's not a container.");
    else if (isSet(obj->obj_flags.value[1], CONT_CLOSED))
      ch->sendln("But it's already closed!");
    else if (!isSet(obj->obj_flags.value[1], CONT_CLOSEABLE))
      ch->sendln("That's impossible.");
    else
    {
      SET_BIT(obj->obj_flags.value[1], CONT_CLOSED);
      ch->sendln("Ok.");
      act_to_room("$n closes $p.", ch, obj, 0, 0);
    }
  }

  if (found == false)
  {
    ch->send(u"I see no %1 here.\r\n"_s).arg(type));
  }

  return ReturnValue::eSUCCESS;
}

bool has_key(CharacterPtr ch, qint32 key)
{
  // if key vnum is 0, there is no key
  if (key == 0)
  {
    return false;
  }

  ObjectPtr obj = ch->equipment[WEAR_HOLD];
  if (obj && IS_KEY(obj))
  {
    if (DC::getInstance()->obj_index[obj->item_number].vnum() == key)
    {
      return true;
    }
  }

  for (obj = ch->carrying; obj; obj = obj->next_content)
  {
    if (IS_KEY(obj) && DC::getInstance()->obj_index[obj->item_number].vnum() == key)
    {
      return true;
    }

    if (IS_KEYRING(obj))
    {
      return search_container_for_vnum(obj, key);
    }
  }

  return false;
}

command_return_t do_lock(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 door, other_room;
  QString type, dir;
  room_direction_data *back;
  ObjectPtr obj;
  CharacterPtr victim;

  argument_interpreter(argument, type, dir);

  if (type.isEmpty())
  {
    ch->sendln("Lock what?");
    return ReturnValue::eFAILURE;
  }
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &victim, &obj, true))
  {
    /* ths is an object */

    if (obj->obj_flags.type_flag != ITEM_CONTAINER)
      ch->sendln("That's not a container.");
    else if (!isSet(obj->obj_flags.value[1], CONT_CLOSED))
      ch->sendln("Maybe you should close it first...");
    else if (obj->obj_flags.value[2] < 0)
      ch->sendln("That thing can't be locked.");
    else if (!has_key(ch, obj->obj_flags.value[2]))
      ch->sendln("You don't seem to have the proper key.");
    else if (isSet(obj->obj_flags.value[1], CONT_LOCKED))
      ch->sendln("It is locked already.");
    else
    {
      SET_BIT(obj->obj_flags.value[1], CONT_LOCKED);
      ch->sendln("*Cluck*");
      act_to_room("$n locks $p - 'cluck', it says.", ch, obj, 0, 0);
    }
  }
  else if ((door = find_door(ch, type, dir)) >= 0)
  {
    /* a door, perhaps */

    if (!isSet(EXIT(ch, door)->exit_info, EX_ISDOOR))
      ch->sendln("That's absurd.");
    else if (!isSet(EXIT(ch, door)->exit_info, EX_CLOSED))
      ch->sendln("You have to close it first, I'm afraid.");
    else if (isSet(EXIT(ch, door)->exit_info, EX_BROKEN))
      ch->sendln("You cannot lock it, it is broken.");
    else if (EXIT(ch, door)->key < 0)
      ch->sendln("There does not seem to be any keyholes.");
    else if (!has_key(ch, EXIT(ch, door)->key))
      ch->sendln("You don't have the proper key.");
    else if (isSet(EXIT(ch, door)->exit_info, EX_LOCKED))
      ch->sendln("It's already locked!");
    else
    {
      SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword)
        act("$n locks the $F.", ch, 0, EXIT(ch, door)->keyword,
            TO_ROOM, 0);
      else
        act_to_room("$n locks the door.", ch, 0, 0, 0);
      ch->sendln("*Click*");
      /* now for locking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != DC::NOWHERE)
        if ((back = DC::getInstance()->world[other_room].dir_option[rev_dir[door]]) != 0)
          if (back->to_room == ch->in_room)
            SET_BIT(back->exit_info, EX_LOCKED);
    }
  }
  else
    ch->sendln("You don't see anything like that.");
  return ReturnValue::eSUCCESS;
}

command_return_t do_unlock(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 door, other_room;
  QString type, dir;
  room_direction_data *back;
  ObjectPtr obj;
  CharacterPtr victim;

  argument_interpreter(argument, type, dir);

  if (type.isEmpty())
  {
    ch->sendln("Unlock what?");
    return ReturnValue::eFAILURE;
  }
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM,
                        ch, &victim, &obj, true))
  {
    /* ths is an object */

    if (obj->obj_flags.type_flag != ITEM_CONTAINER)
      ch->sendln("That's not a container.");
    else if (!isSet(obj->obj_flags.value[1], CONT_CLOSED))
      ch->sendln("Silly - it ain't even closed!");
    else if (obj->obj_flags.value[2] < 0)
      ch->sendln("Odd - you can't seem to find a keyhole.");
    else if (!has_key(ch, obj->obj_flags.value[2]))
      ch->sendln("You don't seem to have the proper key.");
    else if (!isSet(obj->obj_flags.value[1], CONT_LOCKED))
      ch->sendln("Oh.. it wasn't locked, after all.");
    else
    {
      REMOVE_BIT(obj->obj_flags.value[1], CONT_LOCKED);
      ch->sendln("*Click*");
      act_to_room("$n unlocks $p.", ch, obj, 0, 0);
    }
  }
  else if ((door = find_door(ch, type, dir)) >= 0)
  {
    /* it is a door */

    if (!isSet(EXIT(ch, door)->exit_info, EX_ISDOOR))
      ch->sendln("That's absurd.");
    else if (!isSet(EXIT(ch, door)->exit_info, EX_CLOSED))
      ch->sendln("Heck ... it ain't even closed!");
    else if (EXIT(ch, door)->key < 0)
      ch->sendln("You can't seem to spot any keyholes.");
    else if (!has_key(ch, EXIT(ch, door)->key))
      ch->sendln("You do not have the proper key for that.");
    else if (isSet(EXIT(ch, door)->exit_info, EX_BROKEN))
      ch->sendln("You cannot unlock it, it is broken!");
    else if (!isSet(EXIT(ch, door)->exit_info, EX_LOCKED))
      ch->sendln("It's already unlocked, it seems.");
    else
    {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword)
        act_to_room("$n unlocks the $F.", ch, 0, EXIT(ch, door)->keyword, 0);
      else
        act_to_room("$n unlocks the door.", ch, 0, 0, 0);
      ch->sendln("*click*");
      /* now for unlocking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != DC::NOWHERE)
        if ((back = DC::getInstance()->world[other_room].dir_option[rev_dir[door]]) != 0)
          if (back->to_room == ch->in_room)
            REMOVE_BIT(back->exit_info, EX_LOCKED);

      QString door_keyword = u"door"_s;
      if (EXIT(ch, door)->keyword)
      {
        door_keyword = fname(EXIT(ch, door)->keyword);
      }

      ch->sendln(u"You open the %1."_s).arg(door_keyword));
      auto rc = ch->do_open({door_keyword, dir});
      free(copy_of_door_keyword);
      return rc;
    }
  }
  else
    ch->sendln("You don't see anything like that.");
  return ReturnValue::eSUCCESS;
}

qint32 palm(CharacterPtr ch, ObjectPtr obj_object, ObjectPtr sub_object, bool has_consent)
{
  QString buffer;

  if (!ch->has_skill(SKILL_PALM) && ch->isPlayer())
  {
    ch->sendln("You aren't THAT slick there, pal.");
    return ReturnValue::eFAILURE;
  }

  if (!sub_object || sub_object->carried_by != ch)
  {
    if (isSet(obj_object->obj_flags.more_flags, ITEM_UNIQUE))
      if (search_char_for_item(ch, obj_object->item_number, false))
      {
        ch->sendln("The item's uniqueness prevents it!");
        return ReturnValue::eFAILURE;
      }
    if (contents_cause_unique_problem(obj_object, ch))
    {
      ch->sendln("Something inside the item is unique and prevents it!");
      return ReturnValue::eFAILURE;
    }
  }

  if (!charge_moves(ch, SKILL_PALM))
    return ReturnValue::eSUCCESS;

  if (DC::getInstance()->obj_index[obj_object->item_number].vnum() == CHAMPION_ITEM)
  {
    if (ch->isNonPlayer() || ch->getLevel() <= 5)
      return ReturnValue::eFAILURE;
    SETBIT(ch->affected_by, AFF_CHAMPION);

    ObjectPtr o = static_cast<ObjectPtr>(DC::getInstance()->obj_index[obj_object->item_number].item);

    if (o && o->short_description)
      send_info(u"\r\n##%1 has just picked up %2!\r\n"_s.arg(qPrintable(ch->name())).arg(o->short_description()));
    else
      send_info(u"\r\n##%1 has just picked up the Champion Flag!\r\n"_s.arg(qPrintable(ch->name())));
  }

  if (sub_object)
  {
    dc_sprintf(buffer, "%s_consent", qPrintable(ch->name()));
    if (has_consent && obj_object->obj_flags.type_flag != ITEM_MONEY)
    {
      if (isexact("lootable", sub_object->name()) && !isexact(buffer, sub_object->name()))
      {
        SET_BIT(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED);
        ;
        affected_type pthiefaf;
        WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
        ch->sendln("You suddenly feel very guilty...shame on you stealing from the dead!");

        pthiefaf.type = Character::PLAYER_OBJECT_THIEF;
        pthiefaf.duration = 10;
        pthiefaf.modifier = {};
        pthiefaf.location = APPLY_NONE;
        pthiefaf.bitvector = -1;

        if (ch->isPlayerObjectThief())
        {
          affect_from_char(ch, Character::PLAYER_OBJECT_THIEF);
          affect_to_char(ch, &pthiefaf);
        }
        else
          affect_to_char(ch, &pthiefaf);
      }
    }
    else if (has_consent && obj_object->obj_flags.type_flag == ITEM_MONEY && !isexact(buffer, sub_object->name()))
    {
      if (isexact("lootable", sub_object->name()))
      {
        affected_type pthiefaf;

        pthiefaf.type = Character::PLAYER_GOLD_THIEF;
        pthiefaf.duration = 10;
        pthiefaf.modifier = {};
        pthiefaf.location = APPLY_NONE;
        pthiefaf.bitvector = -1;
        WAIT_STATE(ch, DC::PULSE_VIOLENCE);
        ch->sendln("You suddenly feel very guilty...shame on you stealing from the dead!");

        if (ch->isPlayerGoldThief())
        {
          affect_from_char(ch, Character::PLAYER_GOLD_THIEF);
          affect_to_char(ch, &pthiefaf);
        }
        else
          affect_to_char(ch, &pthiefaf);
      }
    }
  }
  move_obj(obj_object, ch);
  QString log_buf = {};
  if (sub_object && sub_object->in_room && obj_object->obj_flags.type_flag != ITEM_MONEY)
  { // Logging gold gets from corpses would just be too much.
    //"%s palms %s[%d] from %s", qPrintable(ch->name()), obj_object->name(), DC::getInstance()->obj_index[obj_object->item_number].vnum(), qPrintable(sub_object->name()));

    DC::getInstance()->logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
    for (ObjectPtr loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
      DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]", obj_object->short_description, loop_obj->short_description,
                              DC::getInstance()->obj_index[loop_obj->item_number].vnum());
  }
  else if (!sub_object && obj_object->obj_flags.type_flag != ITEM_MONEY)
  {
    dc_sprintf(log_buf, "%s palms %s[%d] from room %d", qPrintable(ch->name()), obj_object->name(), DC::getInstance()->obj_index[obj_object->item_number].vnum(),
               ch->in_room);
    DC::getInstance()->logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
    for (ObjectPtr loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
      DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]", obj_object->short_description, loop_obj->short_description,
                              DC::getInstance()->obj_index[loop_obj->item_number].vnum());
  }

  if (skill_success(ch, nullptr, SKILL_PALM))
  {
    act("You successfully snag $p, no one saw you do it!", ch,
        obj_object, 0, TO_CHAR, 0);
    act("$n palms $p trying to hide it from your all knowing gaze.",
        ch, obj_object, 0, TO_ROOM, GODS);
  }
  else
  {
    act_to_character("You clumsily take $p...", ch, obj_object, 0, 0);
    if (sub_object)
      act("$n gets $p from $P.", ch, obj_object, sub_object,
          TO_ROOM, INVIS_NULL);
    else
      act_to_room("$n gets $p.", ch, obj_object, 0, INVIS_NULL);
  }
  if ((obj_object->obj_flags.type_flag == ITEM_MONEY) &&
      (obj_object->obj_flags.value[0] >= 1))
  {
    obj_from_char(obj_object);
    dc_sprintf(buffer, "There was %d coins.\r\n",
               obj_object->obj_flags.value[0]);
    ch->send(buffer);
    if (DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner > 0 && ch->clan !=
                                                                                                        DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner)
    {
      qint32 cgold = (qint32)((qreal)(obj_object->obj_flags.value[0]) * 0.1);
      obj_object->obj_flags.value[0] -= cgold;
      ch->send(u"Clan %s collects %d bounty, leaving %d for you.\r\n"_s).arg(get_clan(DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner)->name).arg(cgold).arg(obj_object->obj_flags.value[0]));
      DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).addGold(cgold);
    }

    ch->addGold(obj_object->obj_flags.value[0]);
    extract_obj(obj_object);
  }
  return ReturnValue::eSUCCESS;
}