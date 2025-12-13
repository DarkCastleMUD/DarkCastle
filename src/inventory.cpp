/************************************************************************
| Description:  This file contains implementation of inventory-management
|   commands: get, give, put, etc..
|
| Authors: DikuMUD, Pirahna, Staylor, Urizen, Rahz, Zaphod, Shane, Jhhudso, Heaven1 and others
*/
#include <cctype>
#include <cstring>

#include <queue>

#include <fmt/format.h>

#include "DC/obj.h"
#include "DC/connect.h"
#include "DC/character.h"
#include "DC/DC.h"
#include "DC/mobile.h"
#include "DC/room.h"
#include "DC/structs.h"
#include "DC/utility.h"
#include "DC/player.h"
#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/act.h"
#include "DC/returnvals.h"
#include "DC/spells.h"
#include "DC/clan.h"
#include "DC/inventory.h"
#include "DC/corpse.h"

/* extern variables */

extern int rev_dir[];

/* procedures related to get */
void get(Character *ch, class Object *obj_object, class Object *sub_object, bool has_consent, cmd_t cmd)
{
  std::string buffer;

  if (!sub_object || sub_object->carried_by != ch)
  {
    if (isSet(obj_object->obj_flags.more_flags, ITEM_NO_TRADE))
    {
      if (IS_NPC(ch))
      {
        ch->sendln("You cannot get that item.");
        return;
      }
      else if (!obj_object->getOwner().isEmpty() && obj_object->getOwner() != GET_NAME(ch))
      {
        ch->send(QStringLiteral("You cannot get that item because it's marked NO_TRADE and owned by %1\r\n").arg(obj_object->getOwner()));
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

  if ((IS_NPC(ch) || ch->affected_by_spell(OBJ_CHAMPFLAG_TIMER)) && DC::getInstance()->obj_index[obj_object->item_number].virt == CHAMPION_ITEM)
  {
    ch->sendln("No champion flag for you, two years!");
    return;
  }

  if (sub_object)
  {
    buffer = fmt::format("{}_consent", GET_NAME(ch));
    if (has_consent && obj_object->obj_flags.type_flag != ITEM_MONEY)
    {
      if ((cmd == cmd_t::LOOT && isexact("lootable", sub_object->name)) && !isexact(buffer, sub_object->name))
      {
        SET_BIT(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED);
        ;
        struct affected_type pthiefaf;
        WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

        char log_buf[MAX_STRING_LENGTH] = {};
        sprintf(log_buf, "%s looted %s[%d] from %s", GET_NAME(ch), obj_object->short_description, DC::getInstance()->obj_index[obj_object->item_number].virt, sub_object->name);
        logentry(log_buf, ANGEL, DC::LogChannel::LOG_MORTAL);

        ch->sendln("You suddenly feel very guilty...shame on you stealing from the dead!");

        pthiefaf.type = Character::PLAYER_OBJECT_THIEF;
        pthiefaf.duration = 10;
        pthiefaf.modifier = 0;
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
    else if (has_consent && obj_object->obj_flags.type_flag == ITEM_MONEY && !isexact(buffer, sub_object->name))
    {
      if (cmd == cmd_t::LOOT && isexact("lootable", sub_object->name))
      {
        struct affected_type pthiefaf;

        pthiefaf.type = Character::PLAYER_GOLD_THIEF;
        pthiefaf.duration = 10;
        pthiefaf.modifier = 0;
        pthiefaf.location = APPLY_NONE;
        pthiefaf.bitvector = -1;
        WAIT_STATE(ch, DC::PULSE_VIOLENCE);
        ch->sendln("You suddenly feel very guilty...shame on you stealing from the dead!");

        char log_buf[MAX_STRING_LENGTH] = {};
        sprintf(log_buf, "%s looted %d coins from %s", GET_NAME(ch), obj_object->obj_flags.value[0], sub_object->name);
        logentry(log_buf, ANGEL, DC::LogChannel::LOG_MORTAL);

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
      char log_buf[MAX_STRING_LENGTH] = {};
      sprintf(log_buf, "%s gets %s[%d] from %s[%d]",
              GET_NAME(ch),
              obj_object->name,
              DC::getInstance()->obj_index[obj_object->item_number].virt,
              sub_object->name,
              DC::getInstance()->obj_index[sub_object->item_number].virt);
      logentry(log_buf, 110, DC::LogChannel::LOG_OBJECTS);
      for (Object *loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
        logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s[%d] contained %s[%d]",
             obj_object->short_description,
             DC::getInstance()->obj_index[obj_object->item_number].virt,
             loop_obj->short_description,
             DC::getInstance()->obj_index[loop_obj->item_number].virt);
    }
    move_obj(obj_object, ch);
    if (sub_object->carried_by == ch)
    {
      act("You get $p from $P.", ch, obj_object, sub_object, TO_CHAR, 0);
      act("$n gets $p from $s $P.", ch, obj_object, sub_object, TO_ROOM, INVIS_NULL);
    }
    else
    {
      act("You get $p from $P.", ch, obj_object, sub_object, TO_CHAR, INVIS_NULL);
      act("$n gets $p from $P.", ch, obj_object, sub_object, TO_ROOM,
          INVIS_NULL);
    }
  }
  else
  {
    move_obj(obj_object, ch);
    act("You get $p.", ch, obj_object, 0, TO_CHAR, 0);
    act("$n gets $p.", ch, obj_object, 0, TO_ROOM, INVIS_NULL);
    if (obj_object->obj_flags.type_flag != ITEM_MONEY)
    {
      char log_buf[MAX_STRING_LENGTH] = {};
      sprintf(log_buf, "%s gets %s[%d] from room %d", GET_NAME(ch), obj_object->name, DC::getInstance()->obj_index[obj_object->item_number].virt,
              ch->in_room);
      logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
      for (Object *loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
        logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]",
             obj_object->short_description,
             loop_obj->short_description,
             DC::getInstance()->obj_index[loop_obj->item_number].virt);
    }

    if (DC::getInstance()->obj_index[obj_object->item_number].virt == CHAMPION_ITEM)
    {
      SETBIT(ch->affected_by, AFF_CHAMPION);
      buffer = fmt::format("\r\n##{} has just picked up {}!\r\n", GET_NAME(ch), static_cast<Object *>(DC::getInstance()->obj_index[obj_object->item_number].item)->short_description);
      send_info(buffer);
    }
  }
  if (sub_object && sub_object->obj_flags.value[3] == 1 &&
      isexact("pc", sub_object->name))
    ch->save(cmd_t::SAVE_SILENTLY);

  if ((obj_object->obj_flags.type_flag == ITEM_MONEY) &&
      (obj_object->obj_flags.value[0] >= 1))
  {
    obj_from_char(obj_object);

    buffer = fmt::format("There was {} coins.",
                         obj_object->obj_flags.value[0]);
    if (IS_NPC(ch) || !isSet(ch->player->toggles, Player::PLR_BRIEF))
    {
      ch->send(buffer);
      ch->sendln("");
    }
    bool tax = false;

    if (DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner > 0 && ch->clan !=
                                                                                                        DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner)
    {
      int cgold = (int)((float)(obj_object->obj_flags.value[0]) * 0.1);
      obj_object->obj_flags.value[0] -= cgold;
      DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).addGold(cgold);
      if (!IS_NPC(ch) && isSet(ch->player->toggles, Player::PLR_BRIEF))
      {
        tax = true;
        buffer = fmt::format("{} Bounty: {}", buffer, cgold);
        DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).addGold(cgold);
      }
      else
        csendf(ch, "Clan %s collects %d bounty, leaving %d for you.\r\n", get_clan(DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner)->name, cgold,
               obj_object->obj_flags.value[0]);
    }
    //	if (sub_object && sub_object->obj_flags.value[3] == 1 &&
    //           !isexact("pc",sub_object->name) && ch->clan
    //            && get_clan(ch)->tax && !isSet(GET_TOGGLES(ch), Player::PLR_NOTAX))
    if (((sub_object && sub_object->obj_flags.value[3] == 1 &&
          !isexact("pc", sub_object->name)) ||
         !sub_object) &&
        ch->clan && get_clan(ch)->tax && !isSet(GET_TOGGLES(ch), Player::PLR_NOTAX))
    {
      int cgold = (int)((float)(obj_object->obj_flags.value[0]) * (float)((float)(get_clan(ch)->tax) / 100.0));
      obj_object->obj_flags.value[0] -= cgold;
      ch->addGold(obj_object->obj_flags.value[0]);
      get_clan(ch)->cdeposit(cgold);
      if (!IS_NPC(ch) && isSet(ch->player->toggles, Player::PLR_BRIEF))
      {
        tax = true;
        buffer = fmt::format("{} ClanTax: {}", buffer, cgold);
      }
      else
      {
        csendf(ch, "Your clan taxes you %d $B$5gold$R, leaving %d $B$5gold$R for you.\r\n", cgold, obj_object->obj_flags.value[0]);
      }
      save_clans();
    }
    else
      ch->addGold(obj_object->obj_flags.value[0]);

    // If a mob gets gold, we disable its ability to receive a gold bonus. This keeps
    // the mob from turning into an interest bearing savings account. :)
    if (IS_NPC(ch))
    {
      SETBIT(ch->mobdata->actflags, ACT_NO_GOLD_BONUS);
    }

    if (tax)
    {
      buffer = fmt::format("{}. {} $B$5gold$R remaining.\r\n", buffer, obj_object->obj_flags.value[0]);
    }
    else
    {
      buffer = fmt::format("{}\r\n", buffer);
    }

    if (!IS_NPC(ch) && isSet(ch->player->toggles, Player::PLR_BRIEF))
      ch->send(buffer);
    extract_obj(obj_object);
  }

  save_corpses();
}

// return eSUCCESS if item was picked up
// return eFAILURE if not
// TODO - currently this is designed with icky if-logic.  While it allows you to put
// code at the end that would affect any attempted 'get' it looks really nasty and
// is never utilized.  Restructure it so it is clear.  Pay proper attention to 'saving'
// however so as not to introduce a potential dupe-bug.
int do_get(Character *ch, char *argument, cmd_t cmd)
{
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  class Object *sub_object;
  class Object *obj_object;
  class Object *next_obj;
  bool found = false;
  bool fail = false;
  bool has_consent = false;
  int type = 3;
  bool alldot = false;
  bool inventorycontainer = false, blindlag = false;
  char allbuf[MAX_STRING_LENGTH];

  argument_interpreter(argument, arg1, arg2);

  /* get type */
  if (!*arg1)
  {
    type = 0;
  }
  if (*arg1 && !*arg2)
  {
    alldot = false;
    allbuf[0] = '\0';
    if ((str_cmp(arg1, "all") != 0) &&
        (sscanf(arg1, "all.%s", allbuf) != 0))
    {
      strcpy(arg1, "all");
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
      strcpy(arg1, "all");
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
    return eFAILURE;
  }

  if (cmd == cmd_t::PALM && type != 2 && type != 6 && type != 0)
  {
    send_to_char("You can only palm objects that are in the same room, "
                 "one at a time.\r\n",
                 ch);
    return eFAILURE;
  }

  if (cmd == cmd_t::LOOT && type != 0 && type != 6)
  {
    ch->sendln("You can only loot 1 item from a non-consented corpse.");
    return eFAILURE;
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
      return eFAILURE;
    }
    sub_object = 0;
    found = false;
    fail = false;
    for (obj_object = DC::getInstance()->world[ch->in_room].contents;
         obj_object;
         obj_object = next_obj)
    {
      next_obj = obj_object->next_content;

      /* IF all.obj, only get those named "obj" */
      if (alldot && !isexact(allbuf, obj_object->name))
        continue;

      // Can't pick up NO_NOTICE items with 'get all'  only 'all.X' or 'X'
      if (!alldot && isSet(obj_object->obj_flags.more_flags, ITEM_NONOTICE) && !ch->isImmortalPlayer())
        continue;

      // Ignore NO_TRADE items on a 'get all'
      if (isSet(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) && !ch->isImmortalPlayer())
      {
        ch->send(QStringLiteral("The %1 appears to be NO_TRADE so you don't pick it up.\r\n").arg(obj_object->short_description));
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
        ch->send(QStringLiteral("%1 is too powerful for you to possess.\r\n").arg(obj_object->short_description));
        continue;
      }

      if (isSet(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
          !isexact(GET_NAME(ch), obj_object->name) && ch->getLevel() < IMPLEMENTER)
      {
        ch->send(QStringLiteral("The %1 appears to be SPECIAL. Only its rightful owner can take it.\r\n").arg(obj_object->short_description));
        continue;
      }

      // PC corpse
      if ((obj_object->obj_flags.value[3] == 1 && isexact("pc", obj_object->name)) || isexact("thiefcorpse", obj_object->name))
      {
        sprintf(buffer, "%s_consent", GET_NAME(ch));
        if ((isexact("thiefcorpse", obj_object->name) &&
             !isexact(GET_NAME(ch), obj_object->name)) ||
            isexact(GET_NAME(ch), obj_object->name) || ch->getLevel() >= OVERSEER)
          has_consent = true;
        if (!has_consent && !isexact(GET_NAME(ch), obj_object->name))
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
          sprintf(buffer, "%s : You can't carry that many items.\r\n", fname(obj_object->name).toStdString().c_str());
          ch->send(buffer);
          fail = true;
        }
        else if ((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) > CAN_CARRY_W(ch) && !ch->isImmortalPlayer() && GET_ITEM_TYPE(obj_object) != ITEM_MONEY)
        {
          sprintf(buffer, "%s : You can't carry that much weight.\r\n", fname(obj_object->name).toStdString().c_str());
          ch->send(buffer);
          fail = true;
        }
        else if (CAN_WEAR(obj_object, ITEM_TAKE))
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
      else if (CAN_SEE_OBJ(ch, obj_object) && ch->isImmortalPlayer() && CAN_WEAR(obj_object, ITEM_TAKE))
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
    sub_object = 0;
    found = false;
    fail = false;
    obj_object = get_obj_in_list_vis(ch, arg1,
                                     DC::getInstance()->world[ch->in_room].contents);
    if (obj_object)
    {
      if (obj_object->obj_flags.type_flag == ITEM_CONTAINER &&
          obj_object->obj_flags.value[3] == 1 &&
          isexact("pc", obj_object->name))
      {
        sprintf(buffer, "%s_consent", GET_NAME(ch));
        if (isexact(GET_NAME(ch), obj_object->name))
          has_consent = true;
        if (!has_consent && !isexact(GET_NAME(ch), obj_object->name))
        {
          send_to_char("You don't have consent to take the "
                       "corpse.\r\n",
                       ch);
          return eFAILURE;
        }
        has_consent = false; // reset it
      }

      if (isSet(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
          !isexact(GET_NAME(ch), obj_object->name) && ch->getLevel() < IMPLEMENTER)
      {
        ch->send(QStringLiteral("The %1 appears to be SPECIAL. Only its rightful owner can take it.\r\n").arg(obj_object->short_description));
      }
      else if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
               !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && !ch->isImmortalPlayer()))
      {
        sprintf(buffer, "%s : You can't carry that many items.\r\n", fname(obj_object->name).toStdString().c_str());
        ch->send(buffer);
        fail = true;
      }
      else if ((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) > CAN_CARRY_W(ch) &&
               !ch->isImmortalPlayer() && GET_ITEM_TYPE(obj_object) != ITEM_MONEY)
      {
        sprintf(buffer, "%s : You can't carry that much weight.\r\n", fname(obj_object->name).toStdString().c_str());
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
          ch->send(QStringLiteral("%1 is too powerful for you to possess.\r\n").arg(obj_object->short_description));
          fail = true;
        }
        else
        {
          ch->send(QStringLiteral("The aura of the donation room allows you to pick up %1.\r\n").arg(obj_object->short_description));
          get(ch, obj_object, sub_object, 0, cmd);
          ch->save(cmd_t::SAVE_SILENTLY);
          found = true;
        }
      }
      else if (CAN_WEAR(obj_object, ITEM_TAKE))
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
      sprintf(buffer, "You do not see a %s here.\r\n", arg1);
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
            isexact("pc", sub_object->name)) ||
           isexact("thiefcorpse", sub_object->name)))
      {
        sprintf(buffer, "%s_consent", GET_NAME(ch));
        if ((isexact("thiefcorpse", sub_object->name) && !isexact(GET_NAME(ch), sub_object->name)) || isexact(buffer, sub_object->name) || ch->getLevel() > 105)
          has_consent = true;
        if (!has_consent && !isexact(GET_NAME(ch), sub_object->name))
        {
          ch->sendln("You don't have consent to touch the corpse.");
          return eFAILURE;
        }
      }
      if (ARE_CONTAINERS(sub_object))
      {
        if (isSet(sub_object->obj_flags.value[1], CONT_CLOSED))
        {
          sprintf(buffer, "The %s is closed.\r\n", fname(sub_object->name).toStdString().c_str());
          ch->send(buffer);
          return eFAILURE;
        }
        for (obj_object = sub_object->contains;
             obj_object;
             obj_object = next_obj)
        {
          next_obj = obj_object->next_content;
          if (GET_ITEM_TYPE(obj_object) == ITEM_CONTAINER && contains_no_trade_item(obj_object))
          {
            csendf(ch, "%s : It seems magically attached to the corpse.\r\n", fname(obj_object->name).toStdString().c_str());
            continue;
          } /*
       class Object *temp,*next_contentthing;
       for (temp = obj_object->contains;temp;temp = next_contentthing)
       {
      next_contentthing = temp->next_content;
      if(isSet(temp->obj_flags.more_flags, ITEM_NO_TRADE))
      {
      csendf(ch, "Whoa!  The %s inside the %s poofed into thin air!\r\n", temp->short_description,obj_object->short_description);
      extract_obj(temp);
      }
       }*/
          //		}
          /* IF all.obj, only get those named "obj" */
          if (alldot && !isexact(allbuf, obj_object->name))
          {
            continue;
          }

          // Ignore NO_TRADE items on a 'get all'
          if (isSet(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) && ch->getLevel() < 100)
          {
            ch->send(QStringLiteral("The %1 appears to be NO_TRADE so you don't pick it up.\r\n").arg(obj_object->short_description));
            continue;
          }

          if (isSet(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
              !isexact(GET_NAME(ch), obj_object->name) && ch->getLevel() < IMPLEMENTER)
          {
            ch->send(QStringLiteral("The %1 appears to be SPECIAL. Only its rightful owner can take it.\r\n").arg(obj_object->short_description));
            continue;
          }

          if (CAN_SEE_OBJ(ch, obj_object))
          {
            if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
                !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && !ch->isImmortalPlayer()))
            {
              sprintf(buffer, "%s : You can't carry that many items.\r\n", fname(obj_object->name).toStdString().c_str());
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
                    if (isexact(obj_object->name, "thiefcorpse"))
                    {
                      ch->send(QStringLiteral("Whoa!  The %1 poofed into thin air!\r\n").arg(obj_object->short_description));
                      extract_obj(obj_object);
                      continue;
                    }
                    csendf(ch, "%s : It seems magically attached to the corpse.\r\n", fname(obj_object->name).toStdString().c_str());
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
                  ch->send(QStringLiteral("%1 is too powerful for you to possess.\r\n").arg(obj_object->short_description));
                  continue;
                }

                if (CAN_WEAR(obj_object, ITEM_TAKE))
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
                sprintf(buffer,
                        "%s : You can't carry that much weight.\r\n", fname(obj_object->name).toStdString().c_str());
                ch->send(buffer);
                fail = true;
              }
            }
          }
        }
        if (!found && !fail)
        {
          sprintf(buffer, "You do not see anything in the %s.\r\n", fname(sub_object->name).toStdString().c_str());
          ch->send(buffer);
          fail = true;
        }
      }
      else
      {
        sprintf(buffer, "The %s is not a container.\r\n", fname(sub_object->name).toStdString().c_str());
        ch->send(buffer);
        fail = true;
      }
    }
    else
    {
      sprintf(buffer, "You do not see or have the %s.\r\n", arg2);
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
        return eFAILURE;
      }

      sub_object = get_obj_in_list_vis(ch, arg2, ch->carrying, true);
      inventorycontainer = true;
    }
    if (sub_object)
    {
      if (sub_object->obj_flags.type_flag == ITEM_CONTAINER &&
          ((sub_object->obj_flags.value[3] == 1 &&
            isexact("pc", sub_object->name)) ||
           isexact("thiefcorpse", sub_object->name)))
      {
        sprintf(buffer, "%s_consent", GET_NAME(ch));

        if ((cmd != cmd_t::LOOT && (isexact("thiefcorpse", sub_object->name) && !isexact(GET_NAME(ch), sub_object->name))) || isexact(buffer, sub_object->name))
          has_consent = true;
        if (!isexact(GET_NAME(ch), sub_object->name) && (cmd == cmd_t::LOOT && isexact("lootable", sub_object->name)) && !isSet(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED) && !isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) && ch->getLevel() >= 50)
          has_consent = true;
        if (!has_consent && !isexact(GET_NAME(ch), sub_object->name))
        {
          send_to_char("You don't have consent to touch the "
                       "corpse.\r\n",
                       ch);
          return eFAILURE;
        }
      }
      if (ARE_CONTAINERS(sub_object))
      {
        if (isSet(sub_object->obj_flags.value[1], CONT_CLOSED))
        {
          sprintf(buffer, "The %s is closed.\r\n", fname(sub_object->name).toStdString().c_str());
          ch->send(buffer);
          return eFAILURE;
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
            ch->send(QStringLiteral("%1 is too powerful for you to possess.\r\n").arg(obj_object->short_description));
            fail = true;
          }

          else if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
                   !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && !ch->isImmortalPlayer()))
          {
            sprintf(buffer, "%s : You can't carry that many items.\r\n", fname(obj_object->name).toStdString().c_str());
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
                if (isexact("thiefcorpse", sub_object->name) || (cmd == cmd_t::LOOT && isexact("lootable", sub_object->name)))
                {
                  ch->send(QStringLiteral("Whoa!  The %1 poofed into thin air!\r\n").arg(obj_object->short_description));

                  char log_buf[MAX_STRING_LENGTH] = {};
                  sprintf(log_buf, "%s poofed %s[%d] from %s[%d]",
                          GET_NAME(ch),
                          obj_object->short_description,
                          DC::getInstance()->obj_index[obj_object->item_number].virt,
                          sub_object->name,
                          DC::getInstance()->obj_index[sub_object->item_number].virt);
                  logentry(log_buf, ANGEL, DC::LogChannel::LOG_MORTAL);

                  extract_obj(obj_object);
                  fail = true;
                  sprintf(buffer, "%s_consent", GET_NAME(ch));

                  if ((cmd == cmd_t::LOOT && isexact("lootable", sub_object->name)) && !isexact(buffer, sub_object->name))
                  {
                    SET_BIT(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED);
                    struct affected_type pthiefaf;

                    pthiefaf.type = Character::PLAYER_OBJECT_THIEF;
                    pthiefaf.duration = 10;
                    pthiefaf.modifier = 0;
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
                  csendf(ch, "%s : It seems magically attached to the corpse.\r\n", fname(obj_object->name).toStdString().c_str());
                  fail = true;
                }
              }
            }
            else if (CAN_WEAR(obj_object, ITEM_TAKE))
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
            sprintf(buffer, "%s : You can't carry that much weight.\r\n", fname(obj_object->name).toStdString().c_str());
            ch->send(buffer);
            fail = true;
          }
        }
        else
        {
          sprintf(buffer, "The %s does not contain the %s.\r\n", fname(sub_object->name).toStdString().c_str(), arg1);
          ch->send(buffer);
          fail = true;
        }
      }
      else
      {
        sprintf(buffer,
                "The %s is not a container.\r\n", fname(sub_object->name).toStdString().c_str());
        ch->send(buffer);
        fail = true;
      }
    }
    else
    {
      sprintf(buffer, "You do not see or have the %s.\r\n", arg2);
      ch->send(buffer);
      fail = true;
    }
  }
  break;
  }
  if (fail)
    return eFAILURE;
  ch->save(cmd_t::SAVE_SILENTLY);
  return eSUCCESS;
}

int do_consent(Character *ch, char *arg, cmd_t cmd)
{
  char buf[MAX_INPUT_LENGTH + 1], buf2[MAX_STRING_LENGTH + 1];
  class Object *obj;
  Character *vict;

  while (isspace(*arg))
    ++arg;

  one_argument(arg, buf);

  if (!*buf)
  {
    ch->sendln("Give WHO consent to touch your rotting carcass?");
    return eFAILURE;
  }

  if (!(vict = get_char_vis(ch, buf)))
  {
    ch->send(QStringLiteral("Consent whom?  You can't see any %1.\r\n").arg(buf));
    return eFAILURE;
  }

  if (vict == ch)
  {
    ch->sendln("Silly, you don't need to consent yourself!");
    return eFAILURE;
  }
  if (vict->getLevel() < 10)
  {
    ch->sendln("That person is too low level to be consented.");
    return eFAILURE;
  }

  // prevent consenting of NPCs
  if (IS_NPC(vict))
  {
    ch->sendln("Now what business would THAT thing have with your mortal remains?");
    return eFAILURE;
  }

  for (obj = DC::getInstance()->object_list; obj; obj = obj->next)
  {
    if (obj->obj_flags.type_flag != ITEM_CONTAINER || obj->obj_flags.value[3] != 1 || !obj->name)
      continue;

    if (!isexact(GET_NAME(ch), obj->name))
      // corpse isn't owned by the consenting player
      continue;

    // check to see if this player is already consented for the corpse
    sprintf(buf2, "%s_consent", buf);
    if (isexact(buf2, obj->name))
      // keep looking; there might be other corpses not yet consented
      continue;

    // check for buffer overflow before adding the new name to the list
    if ((strlen(obj->name) + strlen(buf) + strlen(" _consent")) > (MAX_STRING_LENGTH / 2))
    {
      send_to_char("Don't you think there are enough perverts molesting "
                   "your\r\nmaggot-ridden corpse already?\r\n",
                   ch);
      return eFAILURE;
    }
    sprintf(buf2, "%s %s_consent", obj->name, buf);
    obj->name = str_hsh(buf2);
  }

  sprintf(buf2, "All corpses in the game which belong to you can now be "
                "molested by\r\nanyone named %s.\r\n",
          buf);
  send_to_char(buf2, ch);
  return eSUCCESS;
}

int contents_cause_unique_problem(Object *obj, Character *vict)
{
  int lastnum = -1;

  for (Object *inside = obj->contains; inside; inside = inside->next_content)
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

int contains_no_trade_item(Object *obj)
{
  Object *inside = obj->contains;

  while (inside)
  {
    if (isSet(inside->obj_flags.more_flags, ITEM_NO_TRADE))
      return true;
    inside = inside->next_content;
  }

  return false;
}

int do_drop(Character *ch, char *argument, cmd_t cmd)
{
  char arg[MAX_STRING_LENGTH];
  int amount;
  char buffer[MAX_STRING_LENGTH];
  class Object *tmp_object;
  class Object *next_obj;
  bool test = false, blindlag = false;
  char alldot[MAX_STRING_LENGTH];

  alldot[0] = '\0';

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eFAILURE;
  }

  argument = one_argument(argument, arg);

  if (is_number(arg))
  {
    if (!IS_NPC(ch) && ch->isPlayerGoldThief())
    {
      ch->sendln("Your criminal acts prohibit it.");
      return eFAILURE;
    }

    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (str_cmp("coins", arg) && str_cmp("coin", arg) && str_cmp("gold", arg))
    {
      ch->sendln("Sorry, you can't do that (yet)...");
      return eFAILURE;
    }
    if (amount < 0)
    {
      ch->sendln("Sorry, you can't do that!");
      return eFAILURE;
    }
    if (ch->getGold() < (uint32_t)amount)
    {
      ch->sendln("You haven't got that many coins!");
      return eFAILURE;
    }
    ch->sendln("OK.");
    if (amount == 0)
      return eSUCCESS;

    act("$n drops some gold.", ch, 0, 0, TO_ROOM, 0);
    tmp_object = create_money(amount);
    obj_to_room(tmp_object, ch->in_room);
    ch->removeGold(amount);
    if (ch->isImmortalPlayer())
    {
      special_log(QString(QStringLiteral("%1 dropped %2 coins in room %3!")).arg(ch->getName()).arg(amount).arg(ch->in_room));
    }

    ch->save(cmd_t::SAVE_SILENTLY);
    return eSUCCESS;
  }

  if (*arg)
  {
    if (!str_cmp(arg, "all") || sscanf(arg, "all.%s", alldot) != 0)
    {
      for (tmp_object = ch->carrying; tmp_object; tmp_object = next_obj)
      {
        next_obj = tmp_object->next_content;

        if (alldot[0] != '\0' && !isexact(alldot, tmp_object->name))
          continue;

        if (isSet(tmp_object->obj_flags.extra_flags, ITEM_SPECIAL))
          continue;

        if (!IS_NPC(ch) && ch->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
        {
          ch->sendln("Your criminal acts prohibit it.");
          return eFAILURE;
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
            ch->sendln(QStringLiteral("You drop the %1.").arg(tmp_object->short_description));
          }
          else if (CAN_SEE_OBJ(ch, tmp_object, true))
          {
            ch->sendln(QStringLiteral("You drop the %1.").arg(tmp_object->short_description));
            blindlag = true;
          }
          else
            ch->sendln("You drop something.");

          if (tmp_object->obj_flags.type_flag != ITEM_MONEY)
          {
            char log_buf[MAX_STRING_LENGTH] = {};
            sprintf(log_buf, "%s drops %s[%d] in room %d", GET_NAME(ch), tmp_object->short_description, DC::getInstance()->obj_index[tmp_object->item_number].virt, ch->in_room);
            logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
            for (Object *loop_obj = tmp_object->contains; loop_obj; loop_obj = loop_obj->next_content)
              logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]",
                   tmp_object->short_description,
                   loop_obj->short_description,
                   DC::getInstance()->obj_index[loop_obj->item_number].virt);
          }

          act("$n drops $p.", ch, tmp_object, 0, TO_ROOM, INVIS_NULL);
          move_obj(tmp_object, ch->in_room);
          test = true;
          if (blindlag)
            WAIT_STATE(ch, DC::PULSE_VIOLENCE);
        }
        else
        {
          if (CAN_SEE_OBJ(ch, tmp_object, true))
          {
            sprintf(buffer, "You can't drop %s, it must be CURSED!\r\n", tmp_object->short_description);
            ch->send(buffer);
            test = true;
          }
        }
      } /* for */

      if (!test)
        ch->sendln("You do not seem to have anything.");

    } /* if strcmp "all" */

    else
    {
      tmp_object = get_obj_in_list_vis(ch, arg, ch->carrying);
      if (tmp_object)
      {

        if (!IS_NPC(ch) && ch->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
        {
          ch->sendln("Your criminal acts prohibit it.");
          return eFAILURE;
        }
        if (isSet(tmp_object->obj_flags.more_flags, ITEM_NO_TRADE) && !ch->isImmortalPlayer())
        {
          ch->sendln("It seems magically attached to you.");
          return eFAILURE;
        }
        if (contains_no_trade_item(tmp_object))
        {
          ch->sendln("Something inside it seems magically attached to you.");
          return eFAILURE;
        }

        if (isSet(tmp_object->obj_flags.extra_flags, ITEM_SPECIAL))
        {
          ch->sendln("You can't drop godload items.");
          return eFAILURE;
        }
        else if (!isSet(tmp_object->obj_flags.extra_flags, ITEM_NODROP) ||
                 ch->isImmortalPlayer())
        {
          if (isSet(tmp_object->obj_flags.extra_flags, ITEM_NODROP))
            ch->sendln("(This item is cursed, BTW.)");
          ch->sendln(QStringLiteral("You drop the %1.").arg(tmp_object->short_description));
          act("$n drops $p.", ch, tmp_object, 0, TO_ROOM, INVIS_NULL);
          if (tmp_object->obj_flags.type_flag != ITEM_MONEY)
          {
            char log_buf[MAX_STRING_LENGTH] = {};
            sprintf(log_buf, "%s drops %s[%d] in room %d", GET_NAME(ch), tmp_object->short_description, DC::getInstance()->obj_index[tmp_object->item_number].virt, ch->in_room);
            logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
            for (Object *loop_obj = tmp_object->contains; loop_obj; loop_obj = loop_obj->next_content)
              logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]",
                   tmp_object->short_description,
                   loop_obj->short_description,
                   DC::getInstance()->obj_index[loop_obj->item_number].virt);
          }

          move_obj(tmp_object, ch->in_room);
          return eSUCCESS;
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
  return eFAILURE;
}

void do_putalldot(Character *ch, char *name, char *target, cmd_t cmd)
{
  class Object *tmp_object;
  class Object *next_object;
  char buf[200];
  bool found = false;

  /* If "put all.object bag", get all carried items
   * named "object", and put each into the bag.
   */

  for (tmp_object = ch->carrying; tmp_object; tmp_object = next_object)
  {
    next_object = tmp_object->next_content;
    if (!name && CAN_SEE_OBJ(ch, tmp_object))
    {
      sprintf(buf, "%s %s", fname(tmp_object->name).toStdString().c_str(), target);
      buf[99] = 0;
      found = true;
      do_put(ch, buf, cmd);
    }
    else if (isexact(name, tmp_object->name) && CAN_SEE_OBJ(ch, tmp_object))
    {
      sprintf(buf, "%s %s", name, target);
      buf[99] = 0;
      found = true;
      do_put(ch, buf, cmd);
    }
  }

  if (!found)
    ch->sendln("You don't have one.");
}

int weight_in(class Object *obj)
{ // Sheldon backpack. Damn procs.
  int w = 0;
  class Object *obj2;
  for (obj2 = obj->contains; obj2; obj2 = obj2->next_content)
    w += obj2->obj_flags.weight;
  return w;
}

int do_put(Character *ch, char *argument, cmd_t cmd)
{
  char buffer[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  class Object *obj_object;
  class Object *sub_object;
  Character *tmp_char;
  int bits;
  char allbuf[MAX_STRING_LENGTH];

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eFAILURE;
  }

  argument_interpreter(argument, arg1, arg2);

  if (*arg1)
  {
    if (*arg2)
    {
      if (!(get_obj_in_list_vis(ch, arg2, ch->carrying)) && !(get_obj_in_list_vis(ch, arg2, DC::getInstance()->world[ch->in_room].contents)))
      {
        sprintf(buffer, "You don't have a %s.\r\n", arg2);
        ch->send(buffer);
        return 1;
      }
      allbuf[0] = '\0';
      if (!str_cmp(arg1, "all"))
      {
        do_putalldot(ch, 0, arg2, cmd);
        return eSUCCESS;
      }
      else if (sscanf(arg1, "all.%s", allbuf) != 0)
      {
        do_putalldot(ch, allbuf, arg2, cmd);
        return eSUCCESS;
      }
      obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);

      if (obj_object)
      {
        if (isSet(obj_object->obj_flags.extra_flags, ITEM_NODROP))
        {
          if (!ch->isImmortalPlayer())
          {
            ch->sendln("You are unable to! That item must be CURSED!");
            return eFAILURE;
          }
          else
            ch->sendln("(This item is cursed, BTW.)");
        }
        if (DC::getInstance()->obj_index[obj_object->item_number].virt == CHAMPION_ITEM)
        {
          ch->sendln("You must display this flag for all to see!");
          return eFAILURE;
        }
        if (isSet(obj_object->obj_flags.extra_flags, ITEM_NEWBIE))
        {
          ch->sendln("The protective enchantment this item holds cannot be held within this container.");
          return eFAILURE;
        }
        if (ARE_CONTAINERS(obj_object))
        {
          ch->sendln("You can't put that in there.");
          return eFAILURE;
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
              csendf(ch, "You can't put %s on a keyring.\r\n", GET_OBJ_SHORT(obj_object));
              return eFAILURE;
            }

            // Altars can only hold totems
            if (GET_ITEM_TYPE(sub_object) == ITEM_ALTAR && GET_ITEM_TYPE(obj_object) != ITEM_TOTEM)
            {
              ch->sendln("You cannot put that in an altar.");
              return eFAILURE;
            }

            if (!isSet(sub_object->obj_flags.value[1], CONT_CLOSED))
            {
              // Can't put an item in itself
              if (obj_object == sub_object)
              {
                ch->sendln("You attempt to fold it into itself, but fail.");
                return eFAILURE;
              }

              // Can't put godload in non-godload
              if (IS_SPECIAL(obj_object) && NOT_SPECIAL(sub_object))
              {
                ch->sendln("Are you crazy?!  Someone could steal it!");
                return eFAILURE;
              }

              // Can't put NO_TRADE item in someone else's container/altar/totem
              if (isSet(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) &&
                  sub_object->carried_by != ch)
              {
                ch->sendln("You can't trade that item.");
                return eFAILURE;
              }

              if (isSet(obj_object->obj_flags.more_flags, ITEM_UNIQUE) && search_container_for_item(sub_object, obj_object->item_number))
              {
                ch->sendln("The object's uniqueness prevents it!");
                return eFAILURE;
              }

              bool duplicate_key = search_container_for_item(sub_object, obj_object->item_number);
              if (GET_ITEM_TYPE(sub_object) == ITEM_KEYRING)
              {
                if (duplicate_key == true)
                {
                  if (ch && ch->player && isSet(ch->player->toggles, Player::PLR_NODUPEKEYS))
                  {
                    csendf(ch, "A duplicate of %s is already on your keyring so you will not attach another one.\r\n", GET_OBJ_SHORT(obj_object));
                    return eFAILURE;
                  }
                  else
                  {
                    csendf(ch, "A duplicate of %s is already on your keyring but you don't care.\r\n", GET_OBJ_SHORT(obj_object));
                  }
                }
              }

              if (((sub_object->obj_flags.weight) +
                   (obj_object->obj_flags.weight)) <=
                      (sub_object->obj_flags.value[0]) &&
                  (DC::getInstance()->obj_index[sub_object->item_number].virt != 536 ||
                   weight_in(sub_object) + obj_object->obj_flags.weight <= 200))
              {
                if (bits == FIND_OBJ_INV)
                {
                  obj_from_char(obj_object);
                  /* make up for above line */
                  if (DC::getInstance()->obj_index[sub_object->item_number].virt != 536)
                    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj_object);
                  obj_to_obj(obj_object, sub_object);
                }
                else
                {
                  move_obj(obj_object, sub_object);
                }

                if (GET_ITEM_TYPE(sub_object) == ITEM_KEYRING)
                {
                  act("$n attaches $p to $P.", ch, obj_object, sub_object, TO_ROOM, INVIS_NULL);
                  act("You attach $p to $P.", ch, obj_object, sub_object, TO_CHAR, 0);
                  logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "%s attaches %s[%d] to %s[%d]",
                       ch->getNameC(),
                       obj_object->short_description,
                       DC::getInstance()->obj_index[obj_object->item_number].virt,
                       sub_object->short_description,
                       DC::getInstance()->obj_index[sub_object->item_number].virt);
                }
                else
                {
                  act("$n puts $p in $P.", ch, obj_object, sub_object, TO_ROOM, INVIS_NULL);
                  act("You put $p in $P.", ch, obj_object, sub_object, TO_CHAR, 0);
                  logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "%s puts %s[%d] in %s[%d]",
                       ch->getNameC(),
                       obj_object->short_description,
                       DC::getInstance()->obj_index[obj_object->item_number].virt,
                       sub_object->short_description,
                       DC::getInstance()->obj_index[sub_object->item_number].virt);
                }

                return eSUCCESS;
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
            sprintf(buffer, "The %s is not a container.\r\n", fname(sub_object->name).toStdString().c_str());
            ch->send(buffer);
          }
        }
        else
        {
          sprintf(buffer, "You dont have the %s.\r\n", arg2);
          ch->send(buffer);
        }
      }
      else
      {
        sprintf(buffer, "You dont have the %s.\r\n", arg1);
        ch->send(buffer);
      }
    } /* if arg2 */
    else
    {
      sprintf(buffer, "Put %s in what?\r\n", arg1);
      ch->send(buffer);
    }
  } /* if arg1 */
  else
  {
    ch->sendln("Put what in what?");
  }
  return eFAILURE;
}

void do_givealldot(Character *ch, char *name, char *target, cmd_t cmd)
{
  class Object *tmp_object;
  class Object *next_object;
  char buf[200];
  bool found = false;

  for (tmp_object = ch->carrying; tmp_object; tmp_object = next_object)
  {
    next_object = tmp_object->next_content;
    if (!name && CAN_SEE_OBJ(ch, tmp_object))
    {
      sprintf(buf, "%s %s", fname(tmp_object->name).toStdString().c_str(), target);
      buf[99] = 0;
      found = true;
      do_give(ch, buf, cmd);
    }
    else if (isexact(name, tmp_object->name) && CAN_SEE_OBJ(ch, tmp_object))
    {
      sprintf(buf, "%s %s", name, target);
      buf[99] = 0;
      found = true;
      do_give(ch, buf, cmd);
    }
  }

  if (!found)
    ch->sendln("You don't have one.");
}

int do_give(Character *ch, char *argument, cmd_t cmd)
{
  auto &arena = DC::getInstance()->arena_;
  char obj_name[MAX_INPUT_LENGTH + 1], vict_name[MAX_INPUT_LENGTH + 1], buf[200];
  char arg[80], allbuf[80];
  int64_t amount;
  int retval;
  Character *vict;
  class Object *obj;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eFAILURE;
  }

  if (ch->isPlayerObjectThief())
  {
    ch->sendln("Your criminal actions prohibit it.");
    return eFAILURE;
  }
  argument = one_argument(argument, obj_name);

  if (is_number(obj_name))
  {
    if (!IS_NPC(ch) && ch->isPlayerGoldThief())
    {
      ch->sendln("Your criminal acts prohibit it.");
      return eFAILURE;
    }
    /*
        if(strlen(obj_name) > 7) {
          ch->sendln("Number field too large.");
          return eFAILURE;
        }*/
    amount = atoll(obj_name);
    argument = one_argument(argument, arg);
    if (str_cmp("gold", arg) && str_cmp("coins", arg) && str_cmp("coin", arg))
    {
      ch->sendln("Sorry, you can't do that (yet)...");
      return eFAILURE;
    }
    if (amount < 0)
    {
      ch->sendln("Sorry, you can't do that!");
      return eFAILURE;
    }
    if (ch->getGold() < amount && ch->getLevel() < DEITY)
    {
      ch->sendln("You haven't got that many coins!");
      return eFAILURE;
    }
    argument = one_argument(argument, vict_name);
    if (!*vict_name)
    {
      ch->sendln("To whom?");
      return eFAILURE;
    }

    if (!(vict = ch->get_char_room_vis(vict_name)))
    {
      ch->sendln("To whom?");
      return eFAILURE;
    }

    if (ch == vict)
    {
      ch->sendln("Umm okay, you give it to yourself.");
      return eFAILURE;
    }
    /*
          if(vict->getGold() > 2000000000) {
             ch->sendln("They can't hold that much gold!");
             return eFAILURE;
          }
    */
    csendf(ch, "You give %ld coin%s to %s.\r\n", amount,
           amount == 1 ? "" : "s", GET_SHORT(vict));

    sprintf(buf, "%s gives %ld coin%s to %s", GET_NAME(ch), amount,
            pluralize(amount), GET_NAME(vict));
    logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);

    sprintf(buf, "%s gives you %ld $B$5gold$R coin%s.", PERS(ch, vict), amount,
            amount == 1 ? "" : "s");
    act(buf, ch, 0, vict, TO_VICT, INVIS_NULL);
    act("$n gives some gold to $N.", ch, 0, vict, TO_ROOM, INVIS_NULL | NOTVICT);

    ch->removeGold(amount);

    if (IS_NPC(ch) && (!IS_AFFECTED(ch, AFF_CHARM) || ch->getLevel() > 50))
    {
      special_log(QString(QStringLiteral("%1 (mob) giving %2 gold to %3 in room %4.")).arg(ch->getName()).arg(amount).arg(vict->getName()).arg(ch->in_room));
    }

    if (ch->getGold() < 0)
    {
      ch->setGold(0);
      ch->sendln("Warning:  You are giving out more $B$5gold$R than you had.");
      if (ch->getLevel() < IMPLEMENTER)
      {
        special_log(QString(QStringLiteral("%1 gives %2 coins to %3 (negative!) in room %4.")).arg(ch->getName()).arg(amount).arg(vict->getName()).arg(ch->in_room));
      }
    }
    vict->addGold(amount);

    // If a mob is given gold, we disable its ability to receive a gold bonus. This keeps
    // the mob from turning into an interest bearing savings account. :)
    if (IS_NPC(vict))
    {
      SETBIT(vict->mobdata->actflags, ACT_NO_GOLD_BONUS);
    }

    ch->save();
    vict->save();
    // bribe trigger automatically removes any gold given to mob
    mprog_bribe_trigger(vict, ch, amount);

    return eSUCCESS;
  }

  argument = one_argument(argument, vict_name);

  if (!*obj_name || !*vict_name)
  {
    ch->sendln("Give what to whom?");
    return eFAILURE;
  }

  if (!strcmp(vict_name, "follower"))
  {
    bool found = false;
    struct follow_type *k;
    int org_room = ch->in_room;
    if (ch->followers)
      for (k = ch->followers; k && k != (follow_type *)0x95959595; k = k->next)
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
      ch->sendln("Nobody here are loyal subjects of yours!");
      return eFAILURE;
    }
  }
  else
  {
    if (!(vict = ch->get_char_room_vis(vict_name)))
    {
      ch->sendln("No one by that name around here.");
      return eFAILURE;
    }
  }

  if (ch == vict)
  {
    ch->sendln("Why give yourself stuff?");
    return eFAILURE;
  }

  if (!str_cmp(obj_name, "all"))
  {
    do_givealldot(ch, 0, vict_name, cmd);
    return eSUCCESS;
  }
  else if (sscanf(obj_name, "all.%s", allbuf) != 0)
  {
    do_givealldot(ch, allbuf, vict_name, cmd);
    return eSUCCESS;
  }

  if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying)))
  {
    ch->sendln("You do not seem to have anything like that.");
    return eFAILURE;
  }
  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL) && ch->getLevel() < OVERSEER)
  {
    ch->sendln("That sure would be a fucking stupid thing to do.");
    return eFAILURE;
  }

  if (!IS_NPC(ch) && ch->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
  {
    ch->sendln("Your criminal acts prohibit it.");
    return eFAILURE;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_NODROP))
  {
    if (ch->getLevel() < DEITY)
    {
      ch->sendln("You can't let go of it! Yeech!!");
      return eFAILURE;
    }
    else
      ch->sendln("This item is NODROP btw.");
  }

  // Handle no-trade items
  if (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE))
  {
    // Mortal ch can give immortal vict no-trade items
    if (!ch->isImmortalPlayer() && vict->isImmortalPlayer())
    {
      ch->sendln("It seems to no longer be magically attached to you.");
    }
    else
    {
      // You can give no_trade items to mobs for quest purposes.  It's taken care of later
      if (IS_PC(vict) && (IS_PC(ch) || IS_AFFECTED(ch, AFF_CHARM)))
      {
        if (ch->getLevel() > IMMORTAL)
          ch->sendln("That was a NO_TRADE item btw....");
        else
        {
          ch->sendln("It seems magically attached to you.");
          return eFAILURE;
        }
      }
      if (contains_no_trade_item(obj))
      {
        if (ch->getLevel() > IMMORTAL)
          ch->sendln("That was a NO_TRADE item btw....");
        else
        {
          ch->sendln("Something inside it seems magically attached to you.");
          return eFAILURE;
        }
      }
    }
  }

  if (IS_NPC(vict) && (DC::getInstance()->mob_index[vict->mobdata->nr].non_combat_func == shop_keeper || DC::getInstance()->mob_index[vict->mobdata->nr].virt == QUEST_MASTER))
  {
    act("$N graciously refuses your gift.", ch, 0, vict, TO_CHAR, 0);
    return eFAILURE;
  }
  if (IS_NPC(vict) && IS_AFFECTED(vict, AFF_CHARM) && (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) || contains_no_trade_item(obj)))
  {
    ch->sendln("The creature doesn't understand what you're trying to do.");
    return eFAILURE;
  }

  if (!IS_NPC(ch) && ch->isPlayerObjectThief() && !vict->desc)
  {
    ch->sendln("Now WHY would a thief give something to a linkdead char..?");
    return eFAILURE;
  }

  // Check if mortal ch is trying to give more items to vict than they can carry
  if ((1 + IS_CARRYING_N(vict)) > CAN_CARRY_N(vict))
  {
    if (ch->isImmortalPlayer())
    {
      act("$N seems to have $S hands full but you give it to $M anyways.", ch, 0, vict, TO_CHAR, 0);
    }
    else
    {
      auto &arena = DC::getInstance()->arena_;
      if ((ch->in_room >= 0 && ch->in_room <= DC::getInstance()->top_of_world) && !strcmp(obj_name, "potato") &&
          ch->room().isArena() && vict->room().isArena() &&
          arena.isPotato())
      {
        ;
      }
      else
      {
        act("$N seems to have $S hands full.", ch, 0, vict, TO_CHAR, 0);
        return eFAILURE;
      }
    }
  }

  // Check if mortal ch is trying to give more weight to vict than they can carry
  if (obj->obj_flags.weight + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
  {
    if (ch->isImmortalPlayer())
    {
      act("$E can't carry that much weight but you give it to $M anyways.", ch, 0, vict, TO_CHAR, 0);
    }
    else
    {
      auto &arena = DC::getInstance()->arena_;
      if ((ch->in_room >= 0 && ch->in_room <= DC::getInstance()->top_of_world) && !strcmp(obj_name, "potato") &&
          ch->room().isArena() && vict->room().isArena() &&
          arena.isPotato())
      {
        ;
      }
      else
      {
        act("$E can't carry that much weight.", ch, 0, vict, TO_CHAR, 0);
        return eFAILURE;
      }
    }
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE))
  {
    if (search_char_for_item(vict, obj->item_number, false))
    {
      ch->sendln("The item's uniqueness prevents it.");
      csendf(vict, "%s tried to give you an item but was unable.\r\n", GET_SHORT(ch));
      return eFAILURE;
    }
  }
  if (contents_cause_unique_problem(obj, vict))
  {
    ch->sendln("The uniqueness of something inside it prevents it.");
    csendf(vict, "%s tried to give you an item but was unable.\r\n", GET_SHORT(ch));
    return eFAILURE;
  }

  move_obj(obj, vict);
  act("$n gives $p to $N.", ch, obj, vict, TO_ROOM, INVIS_NULL | NOTVICT);
  act("$n gives you $p.", ch, obj, vict, TO_VICT, 0);
  act("You give $p to $N.", ch, obj, vict, TO_CHAR, 0);

  sprintf(buf, "%s gives %s to %s", GET_NAME(ch), obj->name,
          GET_NAME(vict));
  logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
  for (Object *loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content)
    logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s[%d] contained %s[%d]",
         obj->short_description,
         DC::getInstance()->obj_index[obj->item_number].virt,
         loop_obj->short_description,
         DC::getInstance()->obj_index[loop_obj->item_number].virt);

  if ((vict->in_room >= 0 && vict->in_room <= DC::getInstance()->top_of_world) && vict->isMortalPlayer() &&
      vict->room().isArena() && arena.isPotato() && DC::getInstance()->obj_index[obj->item_number].virt == 393)
  {
    vict->sendln("Here, have some for some potato lag!!");
    WAIT_STATE(vict, DC::PULSE_VIOLENCE * 2);
  }

  //    ch->sendln("Ok.");
  ch->save();
  vict->save();
  // if I gave a no_trade item to a mob, the mob needs to destroy it
  // otherwise it defeats the purpose of no_trade:)

  retval = mprog_give_trigger(vict, ch, obj);
  bool objExists(Object * obj);
  if (!isSet(retval, eEXTRA_VALUE) && isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) && IS_NPC(vict) &&
      objExists(obj))
    extract_obj(obj);

  if (SOMEONE_DIED(retval))
  {
    retval = SWAP_CH_VICT(retval);
    return eSUCCESS | retval;
  }

  return eSUCCESS;
}

// Find an item on a character (in inv, or containers in inv (NOT WORN!))
// and try to put it in his inv.  If sucessful, return pointer to the item.
class Object *bring_type_to_front(Character *ch, int item_type)
{
  class Object *item_carried = nullptr;
  class Object *container_item = nullptr;

  std::queue<Object *> container_queue;

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
  return nullptr;
}

// Find an item on a character
class Object *search_char_for_item(Character *ch, int16_t item_number, bool wearonly)
{
  class Object *i = nullptr;
  class Object *j = nullptr;
  int k;

  for (k = 0; k < MAX_WEAR; k++)
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
  return nullptr;
}

// Find out how many of an item exists on character
int search_char_for_item_count(Character *ch, int16_t item_number, bool wearonly)
{
  class Object *i = nullptr;
  class Object *j = nullptr;
  int k;
  int count = 0;

  for (k = 0; k < MAX_WEAR; k++)
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

bool search_container_for_item(Object *obj, int item_number)
{
  if (obj == nullptr)
  {
    return false;
  }

  if (NOT_CONTAINERS(obj))
  {
    return false;
  }

  for (Object *i = obj->contains; i; i = i->next_content)
  {
    if (i->item_number == item_number)
    {
      return true;
    }
  }

  return false;
}

bool search_container_for_vnum(Object *obj, int vnum)
{
  if (obj == nullptr)
  {
    return false;
  }

  if (NOT_CONTAINERS(obj))
  {
    return false;
  }

  for (Object *i = obj->contains; i; i = i->next_content)
  {
    if (DC::getInstance()->obj_index[i->item_number].virt == vnum)
    {
      return true;
    }
  }

  return false;
}

int find_door(Character *ch, char *type, char *dir)
{
  int door;
  const char *dirs[] =
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
    for (door = 0; door <= 5; door++)
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
bool is_bracing(Character *bracee, struct room_direction_data *exit)
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

  for (int i = 0; i < 6; i++)
    if (DC::getInstance()->world[bracee->in_room].dir_option[i] == exit)
      return true;

  if (bracee->in_room == exit->to_room)
  {
    if (exit == bracee->brace_at || exit == bracee->brace_exit)
      return true;
  }

  return false;
}

int do_open(Character *ch, char *argument, cmd_t cmd)
{
  bool found = false;
  int door, other_room, retval;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct room_direction_data *back;
  class Object *obj;
  Character *victim;
  Character *next_vict;

  int do_fall(Character * ch, short dir);

  retval = 0;

  argument_interpreter(argument, type, dir);

  if (!*type)
  {
    ch->sendln("Open what?");
    return eFAILURE;
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
          csendf(ch, "%s is holding the %s shut.\r\n", EXIT(ch, door)->bracee->getNameC(), fname(EXIT(ch, door)->keyword).toStdString().c_str());
          csendf(EXIT(ch, door)->bracee, "The %s quivers slightly but holds as %s attempts to force their way through.\r\n", fname(EXIT(ch, door)->keyword).toStdString().c_str(), ch);
        }
        else
        {
          csendf(ch, "The %s seems to be barred from the other side.\r\n", fname(EXIT(ch, door)->keyword).toStdString().c_str());
          csendf(EXIT(ch, door)->bracee, "The %s quivers slightly but holds as someone attempts to force their way through.\r\n", fname(EXIT(ch, door)->keyword).toStdString().c_str());
        }
      }
      else
      {
        do_brace(EXIT(ch, door)->bracee, "");
        return do_open(ch, argument, cmd);
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
          act("$n reveals a hidden $F!", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
          csendf(ch, "You reveal a hidden %s!\r\n", fname((char *)EXIT(ch, door)->keyword).toStdString().c_str());
        }
        else
        {
          act("$n reveals a hidden door!", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
          ch->sendln("You reveal a hidden door!");
        }
      }
      else
      {
        if (EXIT(ch, door)->keyword)
          act("$n opens the $F.", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
        else
        {
          act("$n opens the door.", ch, 0, 0, TO_ROOM, 0);
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
              sprintf(buf, "The %s is opened from the other side.\r\n", fname(back->keyword).toStdString().c_str());
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
        int success = 0;

        // opened the door that kept them from falling out
        for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_vict)
        {
          next_vict = victim->next_in_room;
          if (IS_NPC(victim) || IS_AFFECTED(victim, AFF_FLYING))
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
      act("$n opens $p.", ch, obj, 0, TO_ROOM, 0);
    }
  }

  if (found == false)
  {
    ch->send(QStringLiteral("I see no %1 here.\r\n").arg(type));
  }

  // in case ch died or anything
  if (retval)
    return retval;
  return eSUCCESS;
}

int do_close(Character *ch, char *argument, cmd_t cmd)
{
  bool found = false;
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct room_direction_data *back;
  class Object *obj;
  Character *victim;

  argument_interpreter(argument, type, dir);

  if (!*type)
  {
    ch->sendln("Close what?");
    return eFAILURE;
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
        act("$n closes the $F.", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
      else
        act("$n closes the door.", ch, 0, 0, TO_ROOM, 0);
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
              sprintf(buf, "The %s closes quietly.\r\n", fname(back->keyword).toStdString().c_str());
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
      act("$n closes $p.", ch, obj, 0, TO_ROOM, 0);
    }
  }

  if (found == false)
  {
    ch->send(QStringLiteral("I see no %1 here.\r\n").arg(type));
  }

  return eSUCCESS;
}

bool has_key(Character *ch, int key)
{
  // if key vnum is 0, there is no key
  if (key == 0)
  {
    return false;
  }

  Object *obj = ch->equipment[WEAR_HOLD];
  if (obj && IS_KEY(obj))
  {
    if (DC::getInstance()->obj_index[obj->item_number].virt == key)
    {
      return true;
    }
  }

  for (obj = ch->carrying; obj; obj = obj->next_content)
  {
    if (IS_KEY(obj) && DC::getInstance()->obj_index[obj->item_number].virt == key)
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

int do_lock(Character *ch, char *argument, cmd_t cmd)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  class Object *obj;
  Character *victim;

  argument_interpreter(argument, type, dir);

  if (!*type)
  {
    ch->sendln("Lock what?");
    return eFAILURE;
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
      act("$n locks $p - 'cluck', it says.", ch, obj, 0, TO_ROOM, 0);
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
        act("$n locks the door.", ch, 0, 0, TO_ROOM, 0);
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
  return eSUCCESS;
}

int do_unlock(Character *ch, char *argument, cmd_t cmd)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  class Object *obj;
  Character *victim;

  argument_interpreter(argument, type, dir);

  if (!*type)
  {
    ch->sendln("Unlock what?");
    return eFAILURE;
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
      act("$n unlocks $p.", ch, obj, 0, TO_ROOM, 0);
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
        act("$n unlocks the $F.", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
      else
        act("$n unlocks the door.", ch, 0, 0, TO_ROOM, 0);
      ch->sendln("*click*");
      /* now for unlocking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != DC::NOWHERE)
        if ((back = DC::getInstance()->world[other_room].dir_option[rev_dir[door]]) != 0)
          if (back->to_room == ch->in_room)
            REMOVE_BIT(back->exit_info, EX_LOCKED);

      QString door_keyword = QStringLiteral("door");
      if (EXIT(ch, door)->keyword)
      {
        door_keyword = fname(EXIT(ch, door)->keyword);
      }

      ch->sendln(QStringLiteral("You open the %1.").arg(door_keyword));
      auto copy_of_door_keyword = strdup(qPrintable(QStringLiteral("%1 %2").arg(door_keyword).arg(dir)));
      auto rc = do_open(ch, copy_of_door_keyword);
      free(copy_of_door_keyword);
      return rc;
    }
  }
  else
    ch->sendln("You don't see anything like that.");
  return eSUCCESS;
}

int palm(Character *ch, class Object *obj_object, class Object *sub_object, bool has_consent)
{
  char buffer[MAX_STRING_LENGTH];

  if (!ch->has_skill(SKILL_PALM) && IS_PC(ch))
  {
    ch->sendln("You aren't THAT slick there, pal.");
    return eFAILURE;
  }

  if (!sub_object || sub_object->carried_by != ch)
  {
    if (isSet(obj_object->obj_flags.more_flags, ITEM_UNIQUE))
      if (search_char_for_item(ch, obj_object->item_number, false))
      {
        ch->sendln("The item's uniqueness prevents it!");
        return eFAILURE;
      }
    if (contents_cause_unique_problem(obj_object, ch))
    {
      ch->sendln("Something inside the item is unique and prevents it!");
      return eFAILURE;
    }
  }

  if (!charge_moves(ch, SKILL_PALM))
    return eSUCCESS;

  if (DC::getInstance()->obj_index[obj_object->item_number].virt == CHAMPION_ITEM)
  {
    if (IS_NPC(ch) || ch->getLevel() <= 5)
      return eFAILURE;
    SETBIT(ch->affected_by, AFF_CHAMPION);

    Object *o = static_cast<Object *>(DC::getInstance()->obj_index[obj_object->item_number].item);

    if (o && o->short_description)
      send_info(QStringLiteral("\n\r##%1 has just picked up %2!\n\r").arg(GET_NAME(ch)).arg(o->short_description));
    else
      send_info(QStringLiteral("\n\r##%1 has just picked up the Champion Flag!\n\r").arg(GET_NAME(ch)));
  }

  if (sub_object)
  {
    sprintf(buffer, "%s_consent", GET_NAME(ch));
    if (has_consent && obj_object->obj_flags.type_flag != ITEM_MONEY)
    {
      if (isexact("lootable", sub_object->name) && !isexact(buffer, sub_object->name))
      {
        SET_BIT(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED);
        ;
        struct affected_type pthiefaf;
        WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
        ch->sendln("You suddenly feel very guilty...shame on you stealing from the dead!");

        pthiefaf.type = Character::PLAYER_OBJECT_THIEF;
        pthiefaf.duration = 10;
        pthiefaf.modifier = 0;
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
    else if (has_consent && obj_object->obj_flags.type_flag == ITEM_MONEY && !isexact(buffer, sub_object->name))
    {
      if (isexact("lootable", sub_object->name))
      {
        struct affected_type pthiefaf;

        pthiefaf.type = Character::PLAYER_GOLD_THIEF;
        pthiefaf.duration = 10;
        pthiefaf.modifier = 0;
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
  char log_buf[MAX_STRING_LENGTH] = {};
  if (sub_object && sub_object->in_room && obj_object->obj_flags.type_flag != ITEM_MONEY)
  { // Logging gold gets from corpses would just be too much.
    sprintf(log_buf, "%s palms %s[%d] from %s", GET_NAME(ch), obj_object->name, DC::getInstance()->obj_index[obj_object->item_number].virt,
            sub_object->name);
    logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
    for (Object *loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
      logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]", obj_object->short_description, loop_obj->short_description,
           DC::getInstance()->obj_index[loop_obj->item_number].virt);
  }
  else if (!sub_object && obj_object->obj_flags.type_flag != ITEM_MONEY)
  {
    sprintf(log_buf, "%s palms %s[%d] from room %d", GET_NAME(ch), obj_object->name, DC::getInstance()->obj_index[obj_object->item_number].virt,
            ch->in_room);
    logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
    for (Object *loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
      logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]", obj_object->short_description, loop_obj->short_description,
           DC::getInstance()->obj_index[loop_obj->item_number].virt);
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
    act("You clumsily take $p...", ch, obj_object, 0, TO_CHAR, 0);
    if (sub_object)
      act("$n gets $p from $P.", ch, obj_object, sub_object,
          TO_ROOM, INVIS_NULL);
    else
      act("$n gets $p.", ch, obj_object, 0, TO_ROOM, INVIS_NULL);
  }
  if ((obj_object->obj_flags.type_flag == ITEM_MONEY) &&
      (obj_object->obj_flags.value[0] >= 1))
  {
    obj_from_char(obj_object);
    sprintf(buffer, "There was %d coins.\r\n",
            obj_object->obj_flags.value[0]);
    ch->send(buffer);
    if (DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner > 0 && ch->clan !=
                                                                                                        DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner)
    {
      int cgold = (int)((float)(obj_object->obj_flags.value[0]) * 0.1);
      obj_object->obj_flags.value[0] -= cgold;
      csendf(ch, "Clan %s collects %d bounty, leaving %d for you.\r\n", get_clan(DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).clanowner)->name, cgold,
             obj_object->obj_flags.value[0]);
      DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).addGold(cgold);
    }

    ch->addGold(obj_object->obj_flags.value[0]);
    extract_obj(obj_object);
  }
  return eSUCCESS;
}