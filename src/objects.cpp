/************************************************************************
| $Id: objects.cpp,v 1.116 2015/06/16 04:10:54 pirahna Exp $
| objects.C
| Description:  Implementation of the things you can do with objects:
|   wear them, wield them, grab them, drink them, eat them, etc..
*/

#include "DC/DC.h"

extern const QStringList drinks;
extern const QStringList dirs;
extern qint32 drink_aff[][3];

void add_obj_affect(ObjectPtr obj, qint32 loc, qint32 mod)
{
  obj->num_affects++;
  obj->affected.push_back({.location = loc, .modifier = mod});
}

void remove_obj_affect_by_index(ObjectPtr obj, qint32 index)
{
  // shift everyone to right of the one we're deleting to the left
  // TODO - redo this with memmove
  for (qint32 i = index; i < obj->num_affects - 1; i++)
  {
    obj->affected[i].location = obj->affected[i + 1].location;
    obj->affected[i].modifier = obj->affected[i + 1].modifier;
  }

  if (!obj->affected.isEmpty())
  {
    obj->affected.removeLast();
    obj->num_affects--;
  }
}

void remove_obj_affect_by_type(ObjectPtr obj, qint32 loc)
{
  for (qint32 i = {}; i < obj->num_affects; i++)
    if (obj->affected[i].location == loc)
      remove_obj_affect_by_index(obj, i);
}

// given an object, return the maximum points of damage the item
// can take before being scrapped
qint32 eq_max_damage(ObjectPtr obj)
{
  qint32 amount = {};

  switch (GET_ITEM_TYPE(obj))
  {
  case ITEM_WEAPON:
    amount = 7;
    break;
  case ITEM_ARMOR:
    amount = 5;
    amount += ((obj->flags_.value[0]) / 2); // + 1 hit per 2ac
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
  case ITEM_INSTRUMENT:
  case ITEM_LIGHT:
    amount = 5;
    break;
  case ITEM_FIREWEAPON:
  case ITEM_CONTAINER:
  case ITEM_KEYRING:
  case ITEM_KEY:
    amount = 4;
    break;
  default:
    amount = 3;
    break;
  }

  return amount;
}

qint32 eq_current_damage(ObjectPtr obj)
{
  for (qint32 i = {}; i < obj->num_affects; i++)
    if (obj->affected[i].location == APPLY_DAMAGED)
      return (obj->affected[i].modifier);

  return 0;
}

// when repairing eq, we just leave the affect of 0 in there.  That way when
// it gets damaged again, we don't have to realloc the affect list again
void eq_remove_damage(ObjectPtr obj)
{
  for (qint32 i = {}; i < obj->num_affects; i++)
    if (obj->affected[i].location == APPLY_DAMAGED)
    {
      obj->affected[i].modifier = {};
      break;
    }
}

// Damage a piece of eq once and return the amount of damage currently on it
qint32 damage_eq_once(ObjectPtr obj)
{
  if (dc_->obj_index[obj->item_number].vnum() == SPIRIT_SHIELD_OBJ_NUMBER && obj->carried_by && obj->carried_by->in_room)
  {
    send_to_room("The spirit shield shimmers brightly then fades away.\r\n", obj->carried_by->in_room);
    extract_obj(obj);
    return 0;
  }
  // look for existing damage
  for (qint32 i = {}; i < obj->num_affects; i++)
    if (obj->affected[i].location == APPLY_DAMAGED)
    {
      obj->affected[i].modifier++;
      if (obj->affected[i].modifier > 1000)
      {
        obj->affected[i].modifier = 1000;
      }
      return (obj->affected[i].modifier);
    }

  // no existing damage.  Damage it once
  add_obj_affect(obj, APPLY_DAMAGED, 1);
  return 1;
}

void DC::object_activity(quint64 pulse_type)
{
  for (const auto &obj : active_obj_list)
  {
    qint32 item_number = obj->item_number;

    if (obj_index[item_number].non_combat_func)
    {
      obj_index[item_number].non_combat_func(nullptr, obj, cmd_t::UNDEFINED, "", nullptr);
    }
    else if (obj->flags_.type_flag == ITEM_MEGAPHONE && obj->ex_description && obj->flags_.value[0]-- == 0)
    {
      obj->flags_.value[0] = (obj_index[item_number].item)->flags_.value[1];
      send_to_room(obj->ex_description->description_, obj->in_room, true);
    }
    else
    {
      qint32 retval = {};

      if (obj->in_room != DC::NOWHERE)
      {
        if (zones.value(world[obj->in_room].zone).players > 0)
          retval = oprog_rand_trigger(obj);
      }
      else
        retval = oprog_rand_trigger(obj);
      if (!SOMEONE_DIED(retval) && objExists(obj))
        oprog_arand_trigger(obj);
    }
  }

  removeDead();
}

command_return_t do_switch(CharacterPtr ch, QString arg, cmd_t cmd)
{
  ObjectPtr between;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  if (!ch->equipment[WEAR_WIELD] || !ch->equipment[WEAR_SECOND_WIELD])
  {
    send_to_char("You must be wielding two weapons to switch their "
                 "positions.\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  if (GET_MOVE(ch) < 4)
  {
    ch->send("You are too tired to switch your weapons!");
    return ReturnValue::eFAILURE;
  }
  ch->decrementMove(4);

  if (!ch->has_skill(SKILL_SWITCH) || !skill_success(ch, nullptr, SKILL_SWITCH))
  {
    act_to_room("$n fails to switch $s weapons.", ch, 0, 0, 0);
    act_to_character("You fail to switch your weapons.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }
  if (GET_OBJ_WEIGHT(ch->equipment[WEAR_WIELD]) > MIN(GET_STR(ch) / 2, get_max_stat(ch, attribute_t::STRENGTH) / 2) && !IS_AFFECTED(ch, AFF_POWERWIELD))
  {
    ch->sendln("Your primary wield is too heavy to wield as secondary.");
    return ReturnValue::eFAILURE;
  }
  between = ch->equipment[WEAR_WIELD];
  ch->equipment[WEAR_WIELD] = ch->equipment[WEAR_SECOND_WIELD];
  ch->equipment[WEAR_SECOND_WIELD] = between;
  ch->sendln("You switch your weapon positions.");
  return ReturnValue::eSUCCESS;
}

command_return_t do_quaff(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  ObjectPtr temp;
  qint32 i /*,j*/;
  bool equipped;
  qint32 retval = ReturnValue::eSUCCESS;
  bool is_mob = ch->isNonPlayer();
  qint32 lvl;

  equipped = false;
  qint32 pos = -1;
  one_argument(argument, buf);

  if (!(temp = get_obj_in_list_vis(ch, buf, ch->carrying)))
  {
    temp = ch->equipment[WEAR_HOLD];
    equipped = true;
    pos = WEAR_HOLD;
    if ((temp == 0) || !isexact(buf, temp->name()))
    {
      temp = ch->equipment[WEAR_HOLD2];
      pos = WEAR_HOLD2;
      if ((temp == 0) || !isexact(buf, temp->name()))
      {
        equipped = false;
        pos = -2;
        if (!(temp = get_obj_in_list_vis(ch, buf, ch->carrying, true)))
        {
          act_to_character("You do not have that item.", ch, 0, 0, 0);
          return ReturnValue::eFAILURE;
        }
      }
    }
  }

  if (temp->flags_.type_flag != ITEM_POTION)
  {
    act_to_character("You can only quaff potions.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if ((GET_COND(ch, FULL) >= 24) && (GET_COND(ch, THIRST) >= 24) && ch->isPlayer())
  {
    act_to_character("Your stomach is too full to quaff that!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE / 2);
  if (ch->fighting && dc_->number(0, 1) != 0)
  {
    act_to_room("During combat, $n drops $p and it SMASHES!", ch, temp, 0, 0);
    act_to_character("During combat, you drop $p which SMASHES!", ch, temp, 0, 0);
    if (equipped)
      ch->unequip_char(pos);
    extract_obj(temp);
    return ReturnValue::eSUCCESS;
  }

  if (!ch->fighting && isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  if (pos == -2)
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  act_to_room("$n quaffs $p.", ch, temp, 0, 0);
  act_to_character("You quaff $p which dissolves.", ch, temp, 0, 0);

  gain_condition(ch, FULL, 2);
  gain_condition(ch, THIRST, 2);

  for (i = 1; i < 4; i++)
  {
    if (temp->flags_.value[i] >= 1)
    {
      if (spell_info[temp->flags_.value[i]].spell_pointer())
      {
        lvl = (qint32)(1.5 * temp->flags_.value[0]);
        retval = ((*spell_info[temp->flags_.value[i]].spell_pointer())((quint8)temp->flags_.value[0], ch, u""_s, SPELL_TYPE_POTION, ch, 0, lvl));
      }
      else if (spell_info[temp->flags_.value[i]].spell_pointer2())
      {
        lvl = (qint32)(1.5 * temp->flags_.value[0]);
        retval = ((*spell_info[temp->flags_.value[i]].spell_pointer2())((quint8)temp->flags_.value[0], ch, u""_s, SPELL_TYPE_POTION, ch, 0, lvl, 0));
      }
    }
    if (isSet(retval, ReturnValue::eCH_DIED))
      break;
  }
  if (!is_mob || !isSet(retval, ReturnValue::eCH_DIED)) // it's already been free'd when mob died
  {
    if (equipped)
      ch->unequip_char(pos, 1);
    extract_obj(temp);
  }
  return retval;
}

command_return_t do_recite(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  ObjectPtr scroll, obj;
  CharacterPtr victim;
  qint32 i, bits;
  bool equipped;
  qint32 retval = ReturnValue::eSUCCESS;
  bool is_mob = ch->isNonPlayer();
  qint32 lvl;

  if (isSet(dc_->world[ch->in_room].room_flags, NO_MAGIC))
  {
    ch->sendln("Your magic is muffled by greater beings.");
    return ReturnValue::eFAILURE;
  }
  equipped = false;
  obj = {};
  victim = {};
  qint32 pos = -1;
  argument = one_argument(argument, buf);

  if (!(scroll = get_obj_in_list_vis(ch, buf, ch->carrying)))
  {
    scroll = ch->equipment[WEAR_HOLD];
    equipped = true;
    pos = WEAR_HOLD;
    if ((scroll == 0) || !isexact(buf, scroll->name()))
    {
      scroll = ch->equipment[WEAR_HOLD2];
      pos = WEAR_HOLD2;
      if ((scroll == 0) || !isexact(buf, scroll->name()))
      {
        act_to_character("You do not have that item.", ch, 0, 0, 0);
        return ReturnValue::eFAILURE;
      }
    }
  }
  if (isSet(dc_->world[ch->in_room].room_flags, NO_MAGIC))
  {
    act_to_character("Your magic is muffled by greater beings.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }
  if (scroll->flags_.type_flag != ITEM_SCROLL)
  {
    act_to_character("Recite is normally used for scrolls.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (!argument.isEmpty())
  {
    bits = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &victim, &obj, true);
    if (bits == 0)
    {
      ch->sendln("No such thing around to recite the scroll on.");
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    victim = ch;
  }

  act_to_room("$n recites $p.", ch, scroll, 0, INVIS_NULL);
  act_to_character("You recite $p which dissolves.", ch, scroll, 0, 0);

  qint32 failmark = 35 - GET_INT(ch);
  if (ch->isNonPlayer())
    failmark -= 15;

  if (GET_CLASS(ch) == CLASS_MAGIC_USER ||
      GET_CLASS(ch) == CLASS_CLERIC ||
      GET_CLASS(ch) == CLASS_DRUID)
    failmark -= 5;
  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  if (ch->fighting && dc_->number(0, 100) < failmark)
  {
    // failed to read scroll
    act_to_room("$n mumbles the words on the scroll and it goes up in flame!", ch, 0, 0, 0);
    ch->sendln("You mumble the words and the scroll goes up in flame!");
  }
  else
  {
    if (victim && !AWAKE(victim) && dc_->number(1, 5) < 3)
      victim->sendln("Your sleep is restless.");

    // success
    for (i = 1; i < 4; i++)
    {
      if (scroll->flags_.value[i] >= 1)
      {
        lvl = (qint32)(1.5 * scroll->flags_.value[0]);

        if (spell_info[scroll->flags_.value[i]].spell_pointer())
        {
          retval = ((*spell_info[scroll->flags_.value[i]].spell_pointer())((quint8)scroll->flags_.value[0], ch, u""_s, SPELL_TYPE_SCROLL, victim, obj, lvl));
          if (SOMEONE_DIED(retval))
          {
            break;
          }
          if (victim && ch->in_room != victim->in_room)
          {
            break;
          }
        }
        else if (spell_info[scroll->flags_.value[i]].spell_pointer2())
        {
          retval = ((*spell_info[scroll->flags_.value[i]].spell_pointer2())((quint8)scroll->flags_.value[0], ch, u""_s, SPELL_TYPE_SCROLL, victim, obj, lvl, 0));
          if (SOMEONE_DIED(retval))
          {
            break;
          }
          if (victim && ch->in_room != victim->in_room)
          {
            break;
          }
        }
        else
        {
          dc_->logf(100, DC::LogChannel::LOG_BUG, "do_recite ran for scroll %d with spell %d but spell_info[%d].spell_pointer1&2() == nullptr", dc_->obj_index[scroll->item_number].vnum(), i, i);
          continue;
        }
      }
    }
  }
  if (!is_mob || !isSet(retval, ReturnValue::eCH_DIED)) // it's already been free'd when mob died
  {
    if (equipped)
      ch->unequip_char(pos, 1);
    extract_obj(scroll);
  }
  return ReturnValue::eSUCCESS;
}

constexpr auto GOD_TRAP_ITEM = 193;

void set_movement_trap(CharacterPtr ch, ObjectPtr obj)
{
  QString buf;
  ObjectPtr trap_obj = {};

  dc_sprintf(buf, "You set up the %s to catch people moving around in the area.\r\n", qPrintable(obj->short_description()));
  ch->send(buf);
  act_to_room("$n sets something on the ground all around $m.", ch, 0, 0, 0);

  // make a new item
  trap_obj = clone_object(GOD_TRAP_ITEM);

  // copy the data for that trap item over
  for (qint32 i = {}; i < 4; i++)
    trap_obj->flags_.value[i] = obj->flags_.value[i];

  // set it up in the room
  SETBIT(ch->affected_by, AFF_UTILITY);
  obj_to_room(trap_obj, ch->in_room);
}

void set_exit_trap(CharacterPtr ch, ObjectPtr obj, QString arg)
{
  QString buf;
  ObjectPtr trap_obj = {};

  dc_sprintf(buf, "You set up the %s to catch people trying to leave the area.\r\n", qPrintable(obj->short_description()));
  ch->send(buf);
  act_to_room("$n sets something on the ground all around $m.", ch, 0, 0, 0);

  // make a new item
  trap_obj = clone_object(GOD_TRAP_ITEM);

  // copy the data for that trap item over
  for (qint32 i = {}; i < 4; i++)
    trap_obj->flags_.value[i] = obj->flags_.value[i];

  // set it up in the room
  SETBIT(ch->affected_by, AFF_UTILITY);
  obj_to_room(trap_obj, ch->in_room);
}

constexpr auto MORTAR_ROUND_OBJECT_ID = 113;

// Return false if there was a command problem
// Return true if it went off
bool set_utility_mortar(CharacterPtr ch, ObjectPtr obj, QString arg)
{
  QString direct;
  QString buf;
  ObjectPtr trap_obj = {};
  qint32 dir;

  one_argument(arg, direct);
  if (!arg)
  {
    ch->sendln("Set it off in which direction?");
    return false;
  }

  if (direct[0] == 'n')
    dir = {};
  else if (direct[0] == 'e')
    dir = 1;
  else if (direct[0] == 's')
    dir = 2;
  else if (direct[0] == 'w')
    dir = 3;
  else if (direct[0] == 'u')
    dir = 4;
  else if (direct[0] == 'd')
    dir = 5;
  else
  {
    ch->sendln("Set it off in which direction?");
    return false;
  }

  if (isSet(dc_->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("In the rear with the gear huh?  Maybe use this somewhere in the field.");
    return false;
  }
  if (CAN_GO(ch, dir) && isSet(dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].room_flags, SAFE))
  {
    ch->sendln("Firing it into a safe room seems wasteful.");
    return false;
  }

  // make a new item
  trap_obj = clone_object(real_object(MORTAR_ROUND_OBJECT_ID));

  // copy the data for that trap item over
  for (qint32 i = {}; i < 4; i++)
    trap_obj->flags_.value[i] = obj->flags_.value[i];

  do_say(ch, "Fire in the hole!");
  act_to_room("$n sets off $o with a flash and bang!.", ch, obj, 0, 0);
  ch->sendln("You set off the device with a loud bang.");

  if (!CAN_GO(ch, dir))
  {
    send_to_room("It smacks against the wall, zings aorund, and lands in the middle of the room.\r\n", ch->in_room);
    // set it up in the room
    obj_to_room(trap_obj, ch->in_room);
  }
  else
  {
    dc_sprintf(buf, "It flies with great speed %sward.\r\n", dirs[dir]);
    send_to_room(buf, ch->in_room);
    dc_sprintf(buf, "Something flies into the area with great speed landing at your feet.\r\n");
    send_to_room(buf, dc_->world[ch->in_room].dir_option[dir]->to_room);
    // set it up in the target room
    obj_to_room(trap_obj, dc_->world[ch->in_room].dir_option[dir]->to_room);
  }

  return true;
}

// With catstink, the value[1] is the sector type it was designed for
void set_catstink(CharacterPtr ch, ObjectPtr obj)
{
  QString buf;
  extern const QStringList sector_types;

  dc_sprintf(buf, "You sprinkle the %s all around you.\r\n", qPrintable(obj->short_description()));
  ch->send(buf);
  act_to_room("$n sprinkles something on the ground around $m.", ch, 0, 0, 0);

  // make sure it's useable in the place we're at
  if (dc_->world[ch->in_room].sector_type != obj->flags_.value[1])
  {
    if (SECT_MAX_SECT < obj->flags_.value[1] ||
        0 > obj->flags_.value[1])
    {
      ch->sendln("This item has an illegal value1.  Tell a god.");
      return;
    }

    dc_sprintf(buf, "It probably won't work, since %s was designed for the smells of a %s",
               qPrintable(obj->short_description()), sector_types[obj->flags_.value[1]]);
    ch->send(buf);

    // small chance of success
    if (ch->dc_->number(0, 9))
      return;
  }

  SETBIT(ch->affected_by, AFF_UTILITY);
  dc_->world[ch->in_room].tracks_.clear();
}

void set_utility_item(CharacterPtr ch, ObjectPtr obj, QString argument)
{
  qint32 class_restricted(CharacterPtr ch, ObjectPtr obj);

  if (class_restricted(ch, obj))
  {
    ch->sendln("You are forbidden.");
    return;
  }

  switch (obj->flags_.value[0])
  {
  case UTILITY_CATSTINK:
    set_catstink(ch, obj);
    break;
  case UTILITY_EXIT_TRAP:
    set_exit_trap(ch, obj, argument);
    break;
  case UTILITY_MOVEMENT_TRAP:
    set_movement_trap(ch, obj);
    break;
  case UTILITY_MORTAR:
    if (!set_utility_mortar(ch, obj, argument))
      return; // it failed
    break;
  default:
    ch->sendln("Unknown utility item value.  Tell a god.");
    return;
    break;
  }

  WAIT_STATE(ch, (DC::PULSE_VIOLENCE * obj->flags_.value[3]));
  extract_obj(obj);
}

command_return_t do_mortal_set(CharacterPtr ch, QString argument, cmd_t cmd)
{
  ObjectPtr obj = {};
  QString arg;
  QString buf;

  argument = one_argument(argument, arg);

  if (arg.isEmpty())
  {
    ch->sendln("Set what?");
    return ReturnValue::eFAILURE;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
  {
    dc_sprintf(buf, "You do not seem to have a '%s'.\r\n", arg);
    ch->send(buf);
    return ReturnValue::eFAILURE;
  }

  switch (obj->flags_.type_flag)
  {
  case ITEM_UTILITY:
    set_utility_item(ch, obj, argument);
    break;
  case ITEM_TRAP:
    //      set_trap_item(ch, obj, argument);
    //      break;
  default:
    ch->sendln("You can't set that.");
    break;
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_use(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  QString targ;
  QString xtra_arg;
  CharacterPtr tmp_char;
  ObjectPtr tmp_object, stick;
  qint32 lvl;
  qint32 bits;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  if (isSet(dc_->world[ch->in_room].room_flags, NO_MAGIC))
  {
    ch->sendln("Your magic is muffled by greater beings.");
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, buf);

  if ((ch->equipment[WEAR_HOLD] == 0 || !isexact(buf, ch->equipment[WEAR_HOLD]->name())) &&
      (ch->equipment[WEAR_HOLD2] == 0 || !isexact(buf, ch->equipment[WEAR_HOLD2]->name())))
  {
    act_to_character("You must be holding an item in order to to use it.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }
  if (ch->equipment[WEAR_HOLD] && isexact(buf, ch->equipment[WEAR_HOLD]->name()))
    stick = ch->equipment[WEAR_HOLD];
  else
    stick = ch->equipment[WEAR_HOLD2];

  argument = one_argument(argument, targ);
  argument = one_argument(argument, xtra_arg);
  if (stick->flags_.type_flag == ITEM_STAFF)
  {
    act_to_room("$n taps $p three times on the ground.", ch, stick, 0, 0);
    act_to_character("You tap $p three times on the ground.", ch, stick, 0, 0);
    if (stick->flags_.value[2] > 0)
    { /* Charges left? */
      stick->flags_.value[2]--;
      lvl = (qint32)(1.5 * stick->flags_.value[0]);
      WAIT_STATE(ch, DC::PULSE_VIOLENCE);
      qint32 retval = {};
      if (spell_info[stick->flags_.value[3]].spell_pointer())
        retval = ((*spell_info[stick->flags_.value[3]].spell_pointer())((quint8)stick->flags_.value[0], ch, xtra_arg, SPELL_TYPE_STAFF, 0, 0, lvl));
      else if (spell_info[stick->flags_.value[3]].spell_pointer2())
        retval = ((*spell_info[stick->flags_.value[3]].spell_pointer2())((quint8)stick->flags_.value[0], ch, xtra_arg, SPELL_TYPE_STAFF, 0, 0, lvl, 0));
      else
        retval = ReturnValue::eFAILURE;
      return retval;
    }
    else
    {
      ch->sendln("The staff seems powerless.");
    }
  }
  else if (stick->flags_.type_flag == ITEM_WAND)
  {

    bits = generic_find(targ, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object, true);
    if (bits)
    {
      if (bits == FIND_CHAR_ROOM)
      {
        act_to_victim("$n points $p at you.", ch, stick, tmp_char, INVIS_NULL);
        act_to_room("$n points $p at $N.", ch, stick, tmp_char, NOTVICT | INVIS_NULL);
        act_to_character("You point $p at $N.", ch, stick, tmp_char, 0);
      }
      else
      {
        act_to_room("$n points $p at $P.", ch, stick, tmp_object, INVIS_NULL);
        act_to_character("You point $p at $P.", ch, stick, tmp_object, 0);
      }

      if (stick->flags_.value[2] > 0)
      { // are there any charges left?
        stick->flags_.value[2]--;
        lvl = (qint32)(1.5 * stick->flags_.value[0]);
        WAIT_STATE(ch, DC::PULSE_VIOLENCE);
        qint32 retval;
        if (spell_info[stick->flags_.value[3]].spell_pointer())
          retval = ((*spell_info[stick->flags_.value[3]].spell_pointer())((quint8)stick->flags_.value[0], ch, xtra_arg, SPELL_TYPE_WAND, tmp_char, tmp_object, lvl));
        else if (spell_info[stick->flags_.value[3]].spell_pointer2())
          retval = ((*spell_info[stick->flags_.value[3]].spell_pointer2())((quint8)stick->flags_.value[0], ch, xtra_arg, SPELL_TYPE_WAND, tmp_char, tmp_object, lvl, 0));
        else
          retval = ReturnValue::eFAILURE;
        return retval;
      }
      else
      {
        ch->sendln("The wand seems powerless.");
      }
    }
    else
    {
      ch->sendln("What should the wand be pointed at?");
    }
  }
  else
  {
    ch->sendln("Use is normally only for wands and staves.");
  }
  return ReturnValue::eFAILURE;
}

// Allows a player to change his "name" (short_desc) (Sadus)
command_return_t do_name(CharacterPtr ch, QString arg, cmd_t cmd)
{
  auto arguments = QString(arg).trimmed().split(' ');
  if (arguments.isEmpty())
  {
    ch->sendln("Set your name to what?");
    return ReturnValue::eFAILURE;
  }

  if (!ch->isNonPlayer() && isSet(ch->player->punish, PUNISH_NONAME))
  {
    ch->sendln("You can't do that.  You must have been naughty.");
    return ReturnValue::eFAILURE;
  }
  if (ch->getLevel() < 5)
  {
    ch->sendln("You cannot use the \"name\" command until you have reached level 5.");
    return ReturnValue::eFAILURE;
  }

  if (dc_strlen(arg) > 30)
  {
    ch->sendln("Name too long, must be under 30 characters long.");
    return ReturnValue::eFAILURE;
  }

  auto arg1 = arguments.value(0);
  if (!arg1.contains('%'))
  {
    ch->sendln("You MUST include your real name. Use % to indicate where you want it.");
    return ReturnValue::eFAILURE;
  }

  arg1 = arg1.replace('$', ' ');
  arg1 = arg1.replace("??", " ");
  arg1 = arg1.replace("%", fname(ch->name()));
  ch->short_description(arg1);

  ch->sendln("Ok.");
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_drink(QStringList arguments, cmd_t cmd)
{
  ObjectPtr temp = {};
  affected_type af = {};
  qint32 amount = {};

  if (isSet(dc_->world[in_room].room_flags, QUIET))
  {
    sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  if ((GET_COND(this, DRUNK) > 10) && (GET_COND(this, THIRST) > 0)) /* The pig is drunk */
  {
    act_to_character("You simply fail to reach your mouth!", this, 0, 0, 0);
    act_to_room("$n tried to drink but missed $s mouth!", this, 0, 0, INVIS_NULL);
    return ReturnValue::eFAILURE;
  }

  if (GET_COND(this, FULL) > 20 && GET_COND(this, THIRST) > 20) /* Stomach full */
  {
    act_to_character("Your stomach cannot contain anymore!", this, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  auto arg1 = arguments.value(0);
  if ((temp = get_obj_in_list_vis(this, arg1, dc_->world[in_room].contents)) && temp->flags_.type_flag == ITEM_FOUNTAIN && CAN_SEE_OBJ(this, temp))
  {
    act_to_character("You drink from $p.", this, temp, 0, 0);
    act_to_room("$n drinks from $p.", this, temp, 0, INVIS_NULL);
    act_to_character("You are full.", this, 0, 0, 0);
    act_to_character("You are not thirsty anymore.", this, 0, 0, 0);

    if (isImmortalPlayer())
      return ReturnValue::eSUCCESS;

    if (GET_COND(this, FULL) != -1)
      GET_COND(this, FULL) = 22 + dc_->number(0, 5);
    if (GET_COND(this, THIRST) != -1)
      GET_COND(this, THIRST) = 22 + dc_->number(0, 5);

    return ReturnValue::eSUCCESS;
  }

  if (GET_COND(this, THIRST) > 20)
  {
    sendln("Your stomach cannot contain anymore liquid!");
    return ReturnValue::eFAILURE;
  }

  if (!(temp = get_obj_in_list_vis(this, arg1, carrying)))
  {
    act_to_character("You cannot find it!", this, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (temp->flags_.type_flag != ITEM_DRINKCON)
  {
    act_to_character("You can't drink from that!", this, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (temp->flags_.type_flag == ITEM_DRINKCON)
  {
    if (temp->flags_.value[1] > 0) /* Not empty */
    {
      act_to_room(u"$n drinks %1 from $p."_s.arg(drinks[temp->flags_.value[2]]), this, temp, 0, INVIS_NULL);
      sendln(u"You drink the %1."_s.arg(drinks[temp->flags_.value[2]]));

      if (isImmortalPlayer())
        return ReturnValue::eSUCCESS;

      // TODO what is this for?  the statement immediatly afterwards wipes out value
      if (drink_aff[temp->flags_.value[2]][DRUNK] > 0)
        amount = (25 - GET_COND(this, THIRST)) / drink_aff[temp->flags_.value[2]][DRUNK];
      else
        amount = dc_->number(3, 10);

      amount = MIN(amount, temp->flags_.value[1]);

      /* You can't subtract more than the object weighs */

      gain_condition(this, DRUNK, (qint32)((qint32)drink_aff[temp->flags_.value[2]][DRUNK] * amount) / 4);

      gain_condition(this, FULL, (qint32)((qint32)drink_aff[temp->flags_.value[2]][FULL] * amount) / 4);

      gain_condition(this, THIRST, (qint32)((qint32)drink_aff[temp->flags_.value[2]][THIRST] * amount) / 4);

      if (GET_COND(this, DRUNK) > 10)
        act_to_character("You feel drunk.", this, 0, 0, 0);

      if (GET_COND(this, THIRST) > 20)
        act_to_character("You do not feel thirsty.", this, 0, 0, 0);

      if (GET_COND(this, FULL) > 20)
        act_to_character("You are full.", this, 0, 0, 0);

      if (temp->flags_.value[2] == LIQ_HOLYWATER &&
          getHP() < GET_MAX_HIT(this) &&
          dc_->number(0, 1))
      {
        sendln("You feel refreshed!");
        addHP(10);
      }

      if (temp->flags_.value[3] && (!isImmortalPlayer()))
      {
        /* The shit was poisoned ! */
        act_to_character("Ooups, it tasted rather strange ?!!?", this, 0, 0, 0);
        act_to_room("$n chokes and utters some strange sounds.", this, 0, 0, 0);
        if (ch->dc_->number(1, 100) < get_saves(this, SAVE_TYPE_POISON) - 15)
        {
          sendln("Luckily, your body rejects the poison almost immediately.");
        }
        else
        {
          af.type = SPELL_POISON;
          af.duration = amount * 3;
          af.modifier = {};
          af.location = APPLY_NONE;
          af.bitvector = AFF_POISON;
          affect_join(this, &af, false, false);
        }
      }

      /* empty the container, and no longer poison. */
      temp->flags_.value[1] -= amount;
      if (!temp->flags_.value[1])
      { /* The last bit */
        temp->flags_.value[2] = {};
        temp->flags_.value[3] = {};
      }
      /*
              if(temp->flags_.value[1]<=0) {
                act("It is now empty, and it magically disappears in a puff of smoke!",
                  this,0,0,TO_CHAR, 0);
                extract_obj(temp);
              }
      */
      return ReturnValue::eSUCCESS;
    }
  }

  act_to_character("It's empty already.", this, 0, 0, 0);
  return ReturnValue::eFAILURE;
}

command_return_t Character::do_eat(QStringList arguments, cmd_t cmd)
{
  ObjectPtr temp = {};
  affected_type af = {};

  if (isSet(dc_->world[in_room].room_flags, QUIET))
  {
    sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  auto arg1 = arguments.value(0);

  if (!(temp = get_obj_in_list_vis(this, arg1, carrying)))
  {
    act_to_character("You can't find it!", this, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if ((temp->flags_.type_flag != ITEM_FOOD) && (!isImmortalPlayer()))
  {
    act_to_character("Your stomach refuses to eat that!?!", this, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (GET_COND(this, FULL) > 20) /* Stomach full */
  {
    act_to_character("You are too full to eat more!", this, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  act_to_room("$n eats $p.", this, temp, 0, INVIS_NULL);
  act_to_character("You eat the $o.", this, temp, 0, 0);

  gain_condition(this, FULL, temp->flags_.value[0]);

  if (GET_COND(this, FULL) > 20)
    act_to_character("You are full.", this, 0, 0, 0);

  if (temp->flags_.value[3] && (!isImmortalPlayer()))
  {
    /* The shit was poisoned ! */
    act_to_character("Ooups, it tasted rather strange ?!!?", this, 0, 0, 0);
    act_to_room("$n coughs and utters some strange sounds.", this, 0, 0, 0);

    if (ch->dc_->number(1, 100) < get_saves(this, SAVE_TYPE_POISON) - 15)
    {
      sendln("Luckily, your body rejects the poison almost immediately.");
    }
    else
    {
      af.type = SPELL_POISON;
      af.duration = temp->flags_.value[0] * 2;
      af.modifier = {};
      af.location = APPLY_NONE;
      af.bitvector = AFF_POISON;
      affect_join(this, &af, false, false);
    }
  }

  extract_obj(temp);
  return ReturnValue::eSUCCESS;
}

command_return_t do_pour(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  QString arg2;
  QString buf;
  ObjectPtr from_obj;
  ObjectPtr to_obj;
  qint32 amount;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  argument_interpreter(argument, arg1, arg2);

  if (arg.isEmpty() 1) /* No arguments */
  {
    act_to_character("What do you want to pour from?", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
  {
    act_to_character("You can't find it!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (from_obj->flags_.type_flag != ITEM_DRINKCON)
  {
    act_to_character("You can't pour from that!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (from_obj->flags_.value[1] == 0)
  {
    act_to_character("The $p is empty.", ch, from_obj, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (arg.isEmpty() 2)
  {
    act_to_character("Where do you want it? Out or in what?", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (!str_cmp(arg2, "out"))
  {
    act_to_room("$n empties $p", ch, from_obj, 0, INVIS_NULL);
    act_to_character("You empty the $p.", ch, from_obj, 0, 0);

    from_obj->flags_.value[1] = {};
    from_obj->flags_.value[2] = {};
    from_obj->flags_.value[3] = {};
    return ReturnValue::eSUCCESS;
  }

  if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying)))
  {
    act_to_character("You can't find it!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (to_obj->flags_.type_flag != ITEM_DRINKCON)
  {
    act_to_character("You can't pour anything into that.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (to_obj == from_obj)
  {
    act_to_character("A most unproductive effort.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if ((to_obj->flags_.value[1] != 0) &&
      (to_obj->flags_.value[2] != from_obj->flags_.value[2]))
  {
    act_to_character("There is already a different liquid in it!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (!(to_obj->flags_.value[1] < to_obj->flags_.value[0]))
  {
    act_to_character("There is no room for more.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  dc_sprintf(buf, "You pour the %s into the %s.\r\n",
             drinks[from_obj->flags_.value[2]], arg2);
  ch->send(buf);

  /* First same type liq. */
  to_obj->flags_.value[2] = from_obj->flags_.value[2];

  /* Then how much to pour */
  from_obj->flags_.value[1] -= (amount =
                                    (to_obj->flags_.value[0] - to_obj->flags_.value[1]));

  to_obj->flags_.value[1] = to_obj->flags_.value[0];

  if (from_obj->flags_.value[1] < 0) /* There was to little */
  {
    to_obj->flags_.value[1] += from_obj->flags_.value[1];
    amount += from_obj->flags_.value[1];
    from_obj->flags_.value[1] = {};
    from_obj->flags_.value[2] = {};
    from_obj->flags_.value[3] = {};
  }

  /* Then the poison boogie */
  to_obj->flags_.value[3] =
      (to_obj->flags_.value[3] || from_obj->flags_.value[3]);

  return ReturnValue::eSUCCESS;
}

command_return_t do_sip(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  QString buf;
  ObjectPtr temp;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, arg);

  if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying)))
  {
    act_to_character("You can't find it!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (temp->flags_.type_flag != ITEM_DRINKCON)
  {
    act_to_character("You can't sip from that!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (GET_COND(ch, DRUNK) > 10) /* The pig is drunk ! */
  {
    act_to_character("You simply fail to reach your mouth!", ch, 0, 0, 0);
    act_to_room("$n tries to sip, but fails!", ch, 0, 0, INVIS_NULL);
    return ReturnValue::eFAILURE;
  }

  if (!temp->flags_.value[1]) /* Empty */
  {
    act_to_character("But there is nothing in it?", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  act_to_room("$n sips from the $o", ch, temp, 0, INVIS_NULL);
  dc_sprintf(buf, "It tastes like %s.\r\n", drinks[temp->flags_.value[2]]);
  ch->send(buf);

  gain_condition(ch, DRUNK,
                 (qint32)(drink_aff[temp->flags_.value[2]][DRUNK] / 4));

  if (GET_COND(ch, DRUNK) > 10)
    act_to_character("You feel drunk.", ch, 0, 0, 0);

  if (temp->flags_.value[3] == 1 && !IS_AFFECTED(ch, AFF_POISON))
    act_to_character("But it also had a strange taste!", ch, 0, 0, 0);

  temp->flags_.value[1]--;

  if (!temp->flags_.value[1]) /* The last bit */
  {
    temp->flags_.value[2] = {};
    temp->flags_.value[3] = {};
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_taste(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  ObjectPtr temp;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, arg);

  if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying)))
  {
    act_to_character("You can't find it!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (temp->flags_.type_flag == ITEM_DRINKCON)
  {
    return do_sip(ch, argument);
  }

  if (!(temp->flags_.type_flag == ITEM_FOOD))
  {
    act_to_character("Taste that?!? Your stomach refuses!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  act_to_room("$n tastes the $o.", ch, temp, 0, INVIS_NULL);
  act_to_character("You taste the $o.", ch, temp, 0, 0);

  if (temp->flags_.value[3] == 1 && !IS_AFFECTED(ch, AFF_POISON))
  {
    act_to_character("Oops, it did not taste good at all!", ch, 0, 0, 0);
  }

  temp->flags_.value[0]--;

  if (!temp->flags_.value[0]) /* Nothing left */
  {
    act_to_character("There is nothing left now.", ch, 0, 0, 0);
    extract_obj(temp);
  }

  return ReturnValue::eSUCCESS;
}

/* functions related to wear */

void perform_wear(CharacterPtr ch, ObjectPtr obj_object,
                  qint32 keyword)
{
  switch (keyword)
  {
  case 0:
    act_to_room("$n wears $p on $s finger.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 1:
    act_to_character("You wear $p around your neck.", ch, obj_object, 0, 0);
    act_to_room("$n wears $p around $s neck.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 2:
    act_to_character("You wear $p on your body.", ch, obj_object, 0, 0);
    act_to_room("$n wears $p on $s body.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 3:
    act_to_character("You wear $p on your head.", ch, obj_object, 0, 0);
    act_to_room("$n wears $p on $s head.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 4:
    act_to_character("You wear $p on your legs.", ch, obj_object, 0, 0);
    act_to_room("$n wears $p on $s legs.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 5:
    act_to_character("You wear $p on your feet.", ch, obj_object, 0, 0);
    act_to_room("$n wears $p on $s feet.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 6:
    act_to_character("You wear $p on your hands.", ch, obj_object, 0, 0);
    act_to_room("$n wears $p on $s hands.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 7:
    act_to_character("You wear $p on your arms.", ch, obj_object, 0, 0);
    act_to_room("$n wears $p on $s arms.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 8:
    act_to_character("You wear $p on your body.", ch, obj_object, 0, 0);
    act_to_room("$n wears $p about $s body.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 9:
    act_to_character("You wear $p on your waist.", ch, obj_object, 0, 0);
    act_to_room("$n wears $p about $s waist.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 10:
    act_to_room("$n wears $p around $s wrist.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 11:
    act_to_character("You wear $p on your face.", ch, obj_object, 0, 0);
    act_to_room("$n wears $p on $s face.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 12:
    act_to_character("You wield $p.", ch, obj_object, 0, 0);
    act_to_room("$n wields $p.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 13:
    act_to_character("You start using $p as a shield.", ch, obj_object, 0, 0);
    act_to_room("$n starts using $p as a shield.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 14:
    act_to_character("You grab $p.", ch, obj_object, 0, 0);
    act_to_room("$n grabs $p.", ch, obj_object, 0, INVIS_NULL);
    break;
  case 15:
    act_to_room("$n wears $p in $s ear.", ch, obj_object, 0, 0);
    break;
  case 16:
    act_to_character("You light $p and hold it.", ch, obj_object, 0, 0);
    act_to_room("$n lights $p and holds it.", ch, obj_object, 0, 0);
    break;
  }
}

qint32 class_restricted(CharacterPtr ch, ObjectPtr obj)
{
  if (ch->isNonPlayer())
    return false;
  if (IS_OBJ_STAT(obj, ITEM_ANY_CLASS))
    return false;

  if ((IS_OBJ_STAT(obj, ITEM_WARRIOR) && (GET_CLASS(ch) == CLASS_WARRIOR)) ||
      (IS_OBJ_STAT(obj, ITEM_MAGE) && (GET_CLASS(ch) == CLASS_MAGIC_USER)) ||
      (IS_OBJ_STAT(obj, ITEM_THIEF) && (GET_CLASS(ch) == CLASS_THIEF)) ||
      (IS_OBJ_STAT(obj, ITEM_CLERIC) && (GET_CLASS(ch) == CLASS_CLERIC)) ||
      (IS_OBJ_STAT(obj, ITEM_PAL) && (GET_CLASS(ch) == CLASS_PALADIN)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI) && (GET_CLASS(ch) == CLASS_ANTI_PAL)) ||
      (IS_OBJ_STAT(obj, ITEM_BARB) && (GET_CLASS(ch) == CLASS_BARBARIAN)) ||
      (IS_OBJ_STAT(obj, ITEM_RANGER) && (GET_CLASS(ch) == CLASS_RANGER)) ||
      (IS_OBJ_STAT(obj, ITEM_BARD) && (GET_CLASS(ch) == CLASS_BARD)) ||
      (IS_OBJ_STAT(obj, ITEM_DRUID) && (GET_CLASS(ch) == CLASS_DRUID)) ||
      (IS_OBJ_STAT(obj, ITEM_MONK) && (GET_CLASS(ch) == CLASS_MONK)))
    return false;
  return true;
}

qint32 charmie_restricted(CharacterPtr ch, ObjectPtr obj, qint32 wear_loc)
{
  return false; // sigh, work for nohin'
  if (ch->isNonPlayer() && ISSET(ch->affected_by, AFF_CHARM) && ch->master && ch->mobdata)
  {
    qint32 vnum = dc_->mob_index[ch->mobdata->nr].vnum();
    if (vnum == 8 || (vnum > 22388 && vnum < 22399))
      return false; // golems and corpses wear all
    switch (ch->race)
    {
    case RACE_GHOST:
    case RACE_ELEMENT:
    case RACE_SLIME:
      return true; // screw 'em!
    case RACE_REPTILE:
    case RACE_DRAGON:
    case RACE_SNAKE:
    case RACE_HORSE:
    case RACE_BIRD:
    case RACE_RODENT:
    case RACE_FISH:
    case RACE_ARACHNID:
    case RACE_INSECT:
    case RACE_ANIMAL:
    case RACE_TREE:
    case RACE_FELINE:
      switch (wear_loc)
      {
      case WEAR_ABOUT:
      case WEAR_FINGER_L:
      case WEAR_FINGER_R:
      case WEAR_EAR_L:
      case WEAR_EAR_R:
      case WEAR_WRIST_L:
      case WEAR_WRIST_R:
      case WEAR_NECK_1:
      case WEAR_FACE:
        return false;
      default:
        return true;
      }
      break;
    default:
      return false;
    }
  }
  return false;
}

qint32 size_restricted(CharacterPtr ch, ObjectPtr obj)
{
  if (isSet(obj->flags_.size, SIZE_ANY))
    return false;

  if (ch->race == RACE_HUMAN) // human can wear all sizes
    return false;

  if (ch->isNonPlayer()) // mobs (ie charmies) can wear all sizes
    return false;

  if (GET_HEIGHT(ch) < 42)
  {
    if (isSet(obj->flags_.size, SIZE_SMALL))
      return false;
    else
      return true;
  }

  if (GET_HEIGHT(ch) < 66)
  {
    if (isSet(obj->flags_.size, SIZE_SMALL) || isSet(obj->flags_.size, SIZE_MEDIUM))
      return false;
    else
      return true;
  }

  if (GET_HEIGHT(ch) < 79)
  {
    if (isSet(obj->flags_.size, SIZE_MEDIUM))
      return false;
    else
      return true;
  }

  if (GET_HEIGHT(ch) < 103)
  {
    if (isSet(obj->flags_.size, SIZE_LARGE) || isSet(obj->flags_.size, SIZE_MEDIUM))
      return false;
    else
      return true;
  }

  if (isSet(obj->flags_.size, SIZE_LARGE))
    return false;

  return true;
}

// find out if wearing/removing an item will screw up the items a player
// it wearing in terms of sizes vs. height
// ch = player obj = obj to remove/wear add = 1(wear) or 0(remove)
// function WILL tell the character if anything is wrong
qint32 will_screwup_worn_sizes(CharacterPtr ch, ObjectPtr obj, qint32 add)
{
  qint32 j;
  qint32 mod = {};

  // find out if the item affects the person's height
  for (j = {}; j < obj->num_affects; j++)
    if (obj->affected[j].location == APPLY_CHAR_HEIGHT)
      mod += obj->affected[j].modifier;

  if (!mod)
    return false;

  // temporarily affect the person's height
  if (add)
  {
    //	  dc_->logf(ANGEL, DC::LogChannel::LOG_BUG, "will_screwup_worn_sizes: %s height %d by %d = %d", qPrintable(ch->name()), GET_HEIGHT(ch), mod, GET_HEIGHT(ch)+mod);
    GET_HEIGHT(ch) += mod;
  }
  else
  {
    //	  dc_->logf(ANGEL, DC::LogChannel::LOG_BUG, "will_screwup_worn_sizes: %s height %d by -%d = %d", qPrintable(ch->name()), GET_HEIGHT(ch), mod, GET_HEIGHT(ch)-mod);
    GET_HEIGHT(ch) -= mod;
  }

  if (add == 1 && size_restricted(ch, obj))
  {
    // Only have to check the item itself if we're wearing it, not removing
    //	  dc_->logf(ANGEL, DC::LogChannel::LOG_BUG, "will_screwup_worn_sizes: %s height %d by -%d = %d", qPrintable(ch->name()), GET_HEIGHT(ch), mod, GET_HEIGHT(ch)-mod);
    GET_HEIGHT(ch) -= mod;
    ch->sendln("After modifying your height that item would not fit!");
    return true;
  }

  qint32 problem = {};
  for (j = {}; j < MAX_WEAR; j++)
  {
    if (ch->equipment[j] == obj || !ch->equipment[j])
      continue;

    if (size_restricted(ch, ch->equipment[j]))
    {
      problem = 1;
      break;
    }
  }

  // fix height back to normal
  if (add)
  {
    //	  dc_->logf(ANGEL, DC::LogChannel::LOG_BUG, "will_screwup_worn_sizes: %s height %d by -%d = %d", qPrintable(ch->name()), GET_HEIGHT(ch), mod, GET_HEIGHT(ch)-mod);
    GET_HEIGHT(ch) -= mod;
  }
  else
  {
    //	  dc_->logf(ANGEL, DC::LogChannel::LOG_BUG, "will_screwup_worn_sizes: %s height %d by %d = %d", qPrintable(ch->name()), GET_HEIGHT(ch), mod, GET_HEIGHT(ch)+mod);
    GET_HEIGHT(ch) += mod;
  }

  if (problem)
  {
    if (add)
      ch->send(u"Wearing that would cause your %s to no longer fit!\r\n"_s.arg(qPrintable(ch->equipment[j]->short_description())));
    else
      ch->send(u"Removing that would cause your %s to no longer fit!\r\n"_s.arg(qPrintable(ch->equipment[j]->short_description())));

    return true;
  }

  return false;
}

void wear(CharacterPtr ch, ObjectPtr obj_object, qint32 keyword)
{
  ObjectPtr obj;
  QString buffer;
  if (!obj_object)
    return;

  obj = obj_object;
  if (class_restricted(ch, obj_object))
  {
    act_to_character("You are forbidden from wearing $p due to a class restriction.", ch, obj_object, 0, 0);
    return;
  }

  if (size_restricted(ch, obj_object))
  {
    act_to_character("$p is the wrong size for you and does not fit.", ch, obj_object, 0, 0);
    return;
  }

  if (will_screwup_worn_sizes(ch, obj_object, 1))
  {
    // will_screwup_worn_sizes() takes care of the messages
    return;
  }

  if (ch->isPlayer())
  {
    if (ch->getLevel() < obj_object->flags_.eq_level)
    {
      dc_sprintf(buffer, "You must be level %llu to use $p.", obj_object->flags_.eq_level);
      act_to_character(buffer, ch, obj_object, 0, 0);
      return;
    }
  }
  else
  {
    if (dc_->mob_index[ch->mobdata->nr].vnum() != 8)
      if (ch->getLevel() < obj_object->flags_.eq_level)
      {
        dc_sprintf(buffer, "You must be level %llu to use $p.", obj_object->flags_.eq_level);
        act_to_character(buffer, ch, obj_object, 0, 0);
        return;
      }
  }
  /*  if (ch->isNonPlayer() && (dc_->mob_index[ch->mobdata->nr].vnum() < 22394 &&
    dc_->mob_index[ch->mobdata->nr].vnum() > 22388))
    {
       return;
    }*/

  if (isSet(obj->flags_.extra_flags, ITEM_SPECIAL) &&
      !isexact(qPrintable(ch->name()), obj->name()) && ch->getLevel() < IMPLEMENTER)
  {
    act_to_character("$p can only be worn by its rightful owner.", ch, obj_object, 0, 0);
    return;
  }

  if (IS_FAMILIAR(ch))
  {
    ch->sendln("Familiar's cannot wear eq!");
    return;
  }
  switch (keyword)
  {
  case 0:
  {
    if (CAN_WEAR(obj_object, FINGER))
    {
      if (charmie_restricted(ch, obj_object, WEAR_FINGER_L))
        ch->sendln("You cannot wear this.");
      else if ((ch->equipment[WEAR_FINGER_L]) && (ch->equipment[WEAR_FINGER_R]))
      {
        act_to_character("You are already wearing $p on your left ring-finger.", ch, ch->equipment[WEAR_FINGER_L], 0, 0);
        act_to_character("You are already wearing $p on your right ring-finger.", ch, ch->equipment[WEAR_FINGER_R], 0, 0);
      }
      else
      {
        perform_wear(ch, obj_object, keyword);
        if (ch->equipment[WEAR_FINGER_L])
        {
          dc_sprintf(buffer, "You put the %s on your right ring-finger.\r\n", qPrintable(fname(obj_object->name())));
          ch->send(buffer);
          obj_from_char(obj_object);
          ch->equip_char(obj_object, WEAR_FINGER_R);
        }
        else
        {
          dc_sprintf(buffer, "You put the %s on your left ring-finger.\r\n", qPrintable(fname(obj_object->name())));
          ch->send(buffer);
          obj_from_char(obj_object);
          ch->equip_char(obj_object, WEAR_FINGER_L);
        }
      }
    }
    else
      ch->sendln("You can't wear that on your finger.");
  }
  break;
  case 1:
  {
    if (CAN_WEAR(obj_object, NECK))
    {
      if (charmie_restricted(ch, obj_object, WEAR_NECK_1))
        ch->sendln("You cannot wear this.");
      else if ((ch->equipment[WEAR_NECK_1]) && (ch->equipment[WEAR_NECK_2]))
      {
        act_to_character("You are already wearing $p on your neck.", ch, ch->equipment[WEAR_NECK_1], 0, 0);
        act_to_character("You are already wearing $p on your neck.", ch, ch->equipment[WEAR_NECK_2], 0, 0);
      }
      else
      {
        perform_wear(ch, obj_object, keyword);
        if (ch->equipment[WEAR_NECK_1])
        {
          obj_from_char(obj_object);
          ch->equip_char(obj_object, WEAR_NECK_2);
        }
        else
        {
          obj_from_char(obj_object);
          ch->equip_char(obj_object, WEAR_NECK_1);
        }
      }
    }
    else
      ch->sendln("You can't wear that around your neck.");
  }
  break;
  case 2:
  {
    if (CAN_WEAR(obj_object, BODY))
    {
      if (charmie_restricted(ch, obj_object, WEAR_BODY))
        ch->sendln("You cannot wear this.");
      else if (ch->equipment[WEAR_BODY])
      {
        act_to_character("You already wear $p on your body.", ch, ch->equipment[WEAR_BODY], 0, 0);
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        ch->equip_char(obj_object, WEAR_BODY);
      }
    }
    else
      ch->sendln("You can't wear that on your body.");
  }
  break;
  case 3:
  {
    if (CAN_WEAR(obj_object, HEAD))
    {
      if (charmie_restricted(ch, obj_object, WEAR_HEAD))
        ch->sendln("You cannot wear this.");
      else if (ch->equipment[WEAR_HEAD])
      {
        act_to_character("You already wear $p on your head.", ch, ch->equipment[WEAR_HEAD], 0, 0);
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        ch->equip_char(obj_object, WEAR_HEAD);
      }
    }
    else
      ch->sendln("You can't wear that on your head.");
  }
  break;
  case 4:
  {
    if (CAN_WEAR(obj_object, LEGS))
    {
      if (charmie_restricted(ch, obj_object, WEAR_LEGS))
        ch->sendln("You cannot wear this.");
      else if (ch->equipment[WEAR_LEGS])
      {
        act_to_character("You already wear $p on your legs.", ch, ch->equipment[WEAR_LEGS], 0, 0);
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        ch->equip_char(obj_object, WEAR_LEGS);
      }
    }
    else
      ch->sendln("You can't wear that on your legs.");
  }
  break;
  case 5:
  {
    if (CAN_WEAR(obj_object, FEET))
    {
      if (charmie_restricted(ch, obj_object, WEAR_FEET))
        ch->sendln("You cannot wear this.");
      else if (ch->equipment[WEAR_FEET])
      {
        act_to_character("You already wear $p on your feet.", ch, ch->equipment[WEAR_FEET], 0, 0);
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        ch->equip_char(obj_object, WEAR_FEET);
      }
    }
    else
      ch->sendln("You can't wear that on your feet.");
  }
  break;
  case 6:
  {
    if (CAN_WEAR(obj_object, HANDS))
    {
      if (charmie_restricted(ch, obj_object, WEAR_HANDS))
        ch->sendln("You cannot wear this.");
      else if (ch->equipment[WEAR_HANDS])
      {
        act_to_character("You already wear $p on your hands.", ch, ch->equipment[WEAR_HANDS], 0, 0);
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        ch->equip_char(obj_object, WEAR_HANDS);
      }
    }
    else
      ch->sendln("You can't wear that on your hands.");
  }
  break;
  case 7:
  {
    if (CAN_WEAR(obj_object, ARMS))
    {
      if (charmie_restricted(ch, obj_object, WEAR_ARMS))
        ch->sendln("You cannot wear this.");
      else if (ch->equipment[WEAR_ARMS])
      {
        act_to_character("You already wear $p on your arms.", ch, ch->equipment[WEAR_ARMS], 0, 0);
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        ch->equip_char(obj_object, WEAR_ARMS);
      }
    }
    else
      ch->sendln("You can't wear that on your arms.");
  }
  break;
  case 8:
  {
    if (CAN_WEAR(obj_object, ABOUT))
    {
      if (charmie_restricted(ch, obj_object, WEAR_ABOUT))
        ch->sendln("You cannot wear this.");
      else if (ch->equipment[WEAR_ABOUT])
      {
        act_to_character("You already wear $p about your body.", ch, ch->equipment[WEAR_ABOUT], 0, 0);
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        ch->equip_char(obj_object, WEAR_ABOUT);
      }
    }
    else
      ch->sendln("You can't wear that about your body.");
  }
  break;
  case 9:
  {
    if (CAN_WEAR(obj_object, WAISTE))
    {
      if (charmie_restricted(ch, obj_object, WEAR_WAISTE))
        ch->sendln("You cannot wear this.");
      else if (ch->equipment[WEAR_WAISTE])
      {
        act_to_character("You already wear $p about your waist.", ch, ch->equipment[WEAR_WAISTE], 0, 0);
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        ch->equip_char(obj_object, WEAR_WAISTE);
      }
    }
    else
      ch->sendln("You can't wear that about your waist.");
  }
  break;
  case 10:
  {
    if (CAN_WEAR(obj_object, WRIST))
    {
      if (charmie_restricted(ch, obj_object, WEAR_WRIST_L))
        ch->sendln("You cannot wear this.");
      else if ((ch->equipment[WEAR_WRIST_L]) && (ch->equipment[WEAR_WRIST_R]))
      {
        act_to_character("You already wear $p around your left wrist.", ch, ch->equipment[WEAR_WRIST_L], 0, 0);
        act_to_character("You already wear $p around your right wrist.", ch, ch->equipment[WEAR_WRIST_R], 0, 0);
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        if (ch->equipment[WEAR_WRIST_L])
        {
          dc_sprintf(buffer, "You wear the %s around your right wrist.\r\n", qPrintable(fname(obj_object->name())));
          ch->send(buffer);
          ch->equip_char(obj_object, WEAR_WRIST_R);
        }
        else
        {
          dc_sprintf(buffer, "You wear the %s around your left wrist.\r\n", qPrintable(fname(obj_object->name())));
          ch->send(buffer);
          ch->equip_char(obj_object, WEAR_WRIST_L);
        }
      }
    }
    else
      ch->sendln("You can't wear that around your wrist.");
  }
  break;

  case 11:
  {
    if (CAN_WEAR(obj_object, FACE))
    {
      if (charmie_restricted(ch, obj_object, WEAR_FACE))
        ch->sendln("You cannot wear this.");
      else if ((ch->equipment[WEAR_FACE]))
      {
        ch->sendln("You only have one face.");
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        ch->equip_char(obj_object, WEAR_FACE);
      }
    }
    else
      ch->sendln("You can't wear that on your face!");
  }
  break;

  case 12:
    if (CAN_WEAR(obj_object, WIELD))
    {
      if (!ch->equipment[WEAR_WIELD] && GET_OBJ_WEIGHT(obj_object) > MIN<qint32>(GET_STR(ch), get_max_stat(ch, attribute_t::STRENGTH)) &&
          !ISSET(ch->affected_by, AFF_POWERWIELD))
        ch->sendln("It is too heavy for you to use.");
      else if (ch->equipment[WEAR_WIELD] && GET_OBJ_WEIGHT(obj_object) > MIN(GET_STR(ch) / 2, get_max_stat(ch, attribute_t::STRENGTH) / 2) &&
               !ISSET(ch->affected_by, AFF_POWERWIELD))

        act_to_character("$p is too heavy for you to use as a secondary weapon.", ch, obj_object, 0, 0);

      else if ((!ch->hands_are_free(2)) &&
               (isSet(obj_object->flags_.extra_flags, ITEM_TWO_HANDED) && !ISSET(ch->affected_by, AFF_POWERWIELD)))
        ch->sendln("You need both hands for this weapon.");
      else if (!ch->hands_are_free(1))
        ch->sendln("Your hands are already full.");

      else if (IS_AFFECTED(ch, AFF_CHARM))
      {
        ch->sendln("Sorry, charmies can't wield stuff anymore:(");
        do_say(ch, "I'm sorry my master, I lack the dexterity.");
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        if (ch->equipment[WEAR_WIELD])
          ch->equip_char(obj_object, WEAR_SECOND_WIELD);
        else
          ch->equip_char(obj_object, WEAR_WIELD);
      }
    }
    else
      ch->sendln("You can't wield that.");
    break;

  case 13:
  {
    if (CAN_WEAR(obj_object, SHIELD))
    {
      if (charmie_restricted(ch, obj_object, WEAR_SHIELD))
        ch->sendln("You cannot wear this.");
      else if (ch->equipment[WEAR_SHIELD])
      {
        act_to_character("You already using $p as a shield.", ch, ch->equipment[WEAR_SHIELD], 0, 0);
      }
      else if ((!ch->hands_are_free(2)) &&
               (isSet(obj_object->flags_.extra_flags, ITEM_TWO_HANDED) && !ISSET(ch->affected_by, AFF_POWERWIELD)))
      {
        ch->sendln("You need both hands for this shield.");
      }
      else if (!ch->hands_are_free(1))
        ch->sendln("Your hands are already full.");

      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        ch->equip_char(obj_object, WEAR_SHIELD);
      }
    }

    else
      ch->sendln("You can't use that as a shield.");
  }
  break;

  case 14:
    if (CAN_WEAR(obj_object, HOLD))
    {

      if (charmie_restricted(ch, obj_object, WEAR_HOLD))
        ch->sendln("You cannot wear this.");
      else if (!ch->hands_are_free(1))
        ch->sendln("Your hands are already full.");
      else if ((!ch->hands_are_free(2)) &&
               (isSet(obj_object->flags_.extra_flags, ITEM_TWO_HANDED) && !ISSET(ch->affected_by, AFF_POWERWIELD)))
      {
        ch->sendln("You need both hands for this item.");
      }
      else if (obj_object->flags_.extra_flags == ITEM_INSTRUMENT && ((ch->equipment[WEAR_HOLD] && ch->equipment[WEAR_HOLD]->flags_.type_flag == ITEM_INSTRUMENT) || (ch->equipment[WEAR_HOLD2] && ch->equipment[WEAR_HOLD2]->flags_.type_flag == ITEM_INSTRUMENT)))
      {
        ch->sendln("You're busy enough playing the instrument you're already using.");
      }
      else
      {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        if (ch->equipment[WEAR_HOLD])
          ch->equip_char(obj_object, WEAR_HOLD2);
        else
          ch->equip_char(obj_object, WEAR_HOLD);
      }
    }
    else
      ch->sendln("You can't hold this.");
    break;

  case 15:
  {
    if (CAN_WEAR(obj_object, EAR))
    {
      if (charmie_restricted(ch, obj_object, WEAR_EAR_L))
        ch->sendln("You cannot wear this.");
      else if ((ch->equipment[WEAR_EAR_L]) && (ch->equipment[WEAR_EAR_R]))
      {
        act_to_character("You already wearing $p on your left ear.", ch, ch->equipment[WEAR_EAR_L], 0, 0);
        act_to_character("You already wearing $p on your right ear.", ch, ch->equipment[WEAR_EAR_R], 0, 0);
      }
      else
      {
        perform_wear(ch, obj_object, keyword);
        if (ch->equipment[WEAR_EAR_L])
        {
          act_to_character("You wear $p in your right ear.", ch, obj_object, 0, 0);
          obj_from_char(obj_object);
          ch->equip_char(obj_object, WEAR_EAR_R);
        }
        else
        {
          act_to_character("You wear $p in your left ear.", ch, obj_object, 0, 0);
          obj_from_char(obj_object);
          ch->equip_char(obj_object, WEAR_EAR_L);
        }
      }
    }
    else
      ch->sendln("You can't wear that in your ear.");
  }
  break;

  case 16:
  { /* LIGHT SOURCE */
    if (charmie_restricted(ch, obj_object, WEAR_LIGHT))
      ch->sendln("You cannot wear this.");
    else if (ch->equipment[WEAR_LIGHT])
    {
      act_to_character("You are already holding $p as a light.", ch, ch->equipment[WEAR_LIGHT], 0, 0);
    }
    else if ((!ch->hands_are_free(2)) &&
             (isSet(obj_object->flags_.extra_flags, ITEM_TWO_HANDED) && !ISSET(ch->affected_by, AFF_POWERWIELD)))
    {
      ch->sendln("You need both hands for this light.");
    }
    else if (!ch->hands_are_free(1))
      ch->sendln("Your hands are already full.");
    else if (obj_object->flags_.type_flag != ITEM_LIGHT)
      ch->sendln("That isn't a light you cheating fuck!");
    else
    {
      obj_from_char(obj_object);
      perform_wear(ch, obj_object, keyword);
      ch->equip_char(obj_object, WEAR_LIGHT);
    }
  }
  break;

  case 17: // primary
    if (CAN_WEAR(obj_object, WIELD))
    {
      // if not wielding anything, just call regular wield
      if (!ch->equipment[WEAR_WIELD])
      {
        wear(ch, obj_object, 12);
        return;
      }

      if (!ch->hands_are_free(1))
      {
        ch->sendln("Your hands are already full.");
        break;
      }

      if (GET_MOVE(ch) < 4)
      {
        ch->send("You are too tired to switch your weapons!");
        return;
      }
      ch->decrementMove(4);

      if (!ch->has_skill(SKILL_SWITCH) || !skill_success(ch, nullptr, SKILL_SWITCH))
      {
        act_to_room("$n fails to switch $s weapons.", ch, 0, 0, 0);
        act_to_character("You fail to switch your weapons.", ch, 0, 0, 0);
        return;
      }

      ObjectPtr obj_temp = ch->equipment[WEAR_WIELD];
      obj_to_char(ch->unequip_char(WEAR_WIELD), ch);
      wear(ch, obj_object, 12);
      wear(ch, obj_temp, 12);
      return;
    }
    else
      ch->sendln("You can't wield that.");
    break;

  case -1:
  {
    dc_sprintf(buffer, "Wear %s where?.\r\n", qPrintable(fname(obj_object->name())));
    ch->send(buffer);
  }
  break;
  case -2:
  {
    dc_sprintf(buffer, "You can't wear the %s.\r\n", qPrintable(fname(obj_object->name())));
    ch->send(buffer);
  }
  break;
  default:
  {
    dc_->logentry(u"Unknown type called in wear."_s, ANGEL, DC::LogChannel::LOG_BUG);
  }
  break;
  }
  redo_hitpoints(ch);
  redo_mana(ch);
  redo_ki(ch);
}

qint32 Object::keywordfind(void)
{
  ObjectPtr obj_object = this;
  qint32 keyword;

  keyword = -2;
  if (CAN_WEAR(obj_object, FINGER))
    keyword = {};
  else if (CAN_WEAR(obj_object, NECK))
    keyword = 1;
  else if (CAN_WEAR(obj_object, BODY))
    keyword = 2;
  else if (CAN_WEAR(obj_object, HEAD))
    keyword = 3;
  else if (CAN_WEAR(obj_object, LEGS))
    keyword = 4;
  else if (CAN_WEAR(obj_object, FEET))
    keyword = 5;
  else if (CAN_WEAR(obj_object, HANDS))
    keyword = 6;
  else if (CAN_WEAR(obj_object, ARMS))
    keyword = 7;
  else if (CAN_WEAR(obj_object, ABOUT))
    keyword = 8;
  else if (CAN_WEAR(obj_object, WAISTE))
    keyword = 9;
  else if (CAN_WEAR(obj_object, WRIST))
    keyword = 10;
  else if (CAN_WEAR(obj_object, FACE))
    keyword = 11;
  else if (CAN_WEAR(obj_object, WIELD))
    keyword = 12;
  else if (CAN_WEAR(obj_object, SHIELD))
    keyword = 13;
  else if (CAN_WEAR(obj_object, HOLD))
    keyword = 14;
  else if (CAN_WEAR(obj_object, EAR))
    keyword = 15;
  return keyword;
}

QString Object::TypeString(void)
{
  return item_types.value(Type());
}

bool Object::TypeString(QString type)
{
  if (Type(item_types.indexOf(type.toUpper())))
    return true;
  else
    return false;
}

command_return_t do_wear(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  QString arg2;
  QString buf;
  QString buffer;
  ObjectPtr obj_object, *tmp_object, next_obj;
  qint32 keyword;
  bool blindlag = false;
  static const QStringList keywords = {
      "finger",
      "neck",
      "body",
      "head",
      "legs",
      "feet",
      "hands",
      "arms",
      "about",
      "waist",
      "wrist",
      "face",
      "wield",
      "shield",
      "hold",
      "ear",
      "light",
      "primary"};

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  argument_interpreter(argument, arg1, arg2);

  if (!(*arg1))
  {
    ch->sendln("Wear what?");
    return ReturnValue::eFAILURE;
  }

  if (!str_cmp(arg1, "all"))
  {
    for (tmp_object = ch->carrying; tmp_object; tmp_object = next_obj)
    {
      qint32 keyword;
      next_obj = tmp_object->next_content;
      if (!CAN_SEE_OBJ(ch, tmp_object))
        continue;
      keyword = tmp_object->keywordfind();
      if (keyword != -2)
        wear(ch, tmp_object, keyword);
    }

    return ReturnValue::eSUCCESS;
  }

  obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);
  if (!obj_object && IS_AFFECTED(ch, AFF_BLIND) && ch->has_skill(SKILL_BLINDFIGHTING))
  {
    obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying, true);
    blindlag = true;
  }
  if (obj_object)
  {
    if (*arg2)
    {
      keyword = search_list(arg2, keywords);
      if (keyword == -1)
      {
        dc_sprintf(buf, "%s is an unknown body location.\r\n", arg2);
        ch->send(buf);
      }
      else
      {
        wear(ch, obj_object, keyword);
      }
    }
    else
    {
      wear(ch, obj_object, obj_object->keywordfind());
    }
    if (blindlag)
      WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  }
  else
  {
    dc_sprintf(buffer, "You do not seem to have the '%s'.\r\n", arg1);
    ch->send(buffer);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_wield(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  QString arg2;
  QString buffer;
  ObjectPtr obj_object;
  bool blindlag = false;
  qint32 keyword = 12;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  argument_interpreter(argument, arg1, arg2);
  if (*arg1)
  {
    obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if (!obj_object && IS_AFFECTED(ch, AFF_BLIND) && ch->has_skill(SKILL_BLINDFIGHTING))
    {
      obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying, true);
      blindlag = true;
    }
    if (obj_object)
    {
      if (*arg2)
      {
        if (arg2[0] == 'p')
          keyword = 17;
      }

      wear(ch, obj_object, keyword);

      if (blindlag)
        WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    }
    else
    {
      dc_sprintf(buffer, "You do not seem to have the '%s'.\r\n", arg1);
      ch->send(buffer);
    }
  }
  else
  {
    ch->sendln("Wield what?");
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_grab(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  QString arg2;
  QString buffer;
  ObjectPtr obj_object;
  bool blindlag = false;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  argument_interpreter(argument, arg1, arg2);

  if (*arg1)
  {
    obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if (!obj_object && IS_AFFECTED(ch, AFF_BLIND) && ch->has_skill(SKILL_BLINDFIGHTING))
    {
      obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying, true);
      blindlag = true;
    }
    if (obj_object)
    {
      if (obj_object->flags_.type_flag == ITEM_LIGHT)
        wear(ch, obj_object, 16);
      else
        wear(ch, obj_object, 14);
      if (blindlag)
        WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    }
    else
    {
      dc_sprintf(buffer, "You do not seem to have the '%s'.\r\n", arg1);
      ch->send(buffer);
    }
  }
  else
  {
    ch->sendln("Hold what?");
  }
  return ReturnValue::eSUCCESS;
}

qint32 Character::hands_are_free(qint32 number)
{
  ObjectPtr wielded;
  qint32 hands = {};

  wielded = equipment[WEAR_WIELD];

  if (wielded)
    if (isSet(wielded->flags_.extra_flags, ITEM_TWO_HANDED) && !ISSET(affected_by, AFF_POWERWIELD))
      hands = 2;

  if (equipment[WEAR_WIELD])
    hands++;
  if (equipment[WEAR_SECOND_WIELD])
    hands++;

  if (equipment[WEAR_SHIELD])
  {
    if (isSet(equipment[WEAR_SHIELD]->flags_.extra_flags, ITEM_TWO_HANDED) &&
        !ISSET(affected_by, AFF_POWERWIELD))
      hands++;
    hands++;
  }
  if (equipment[WEAR_HOLD])
  {
    if (isSet(equipment[WEAR_HOLD]->flags_.extra_flags, ITEM_TWO_HANDED) &&
        !ISSET(affected_by, AFF_POWERWIELD))
      hands++;
    hands++;
  }
  if (equipment[WEAR_LIGHT])
  {
    if (isSet(equipment[WEAR_LIGHT]->flags_.extra_flags, ITEM_TWO_HANDED) &&
        !ISSET(affected_by, AFF_POWERWIELD))
      hands++;
    hands++;
  }
  if (equipment[WEAR_HOLD2])
    hands++;

  if (number == 1 && hands < 2)
    return (1);
  else if (number == 2 && hands < 1)
    return (1);
  else
    return {};
}

command_return_t do_remove(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  ObjectPtr obj_object;
  bool blindlag = false;
  qint32 j;

  if (isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, arg1);

  if (*arg1)
  {
    if (!dc_strcmp(arg1, "all"))
    {
      for (j = {}; j < MAX_WEAR; j++)
      {
        if (CAN_CARRY_N(ch) != IS_CARRYING_N(ch))
        {
          if (ch->equipment[j] && CAN_SEE_OBJ(ch, ch->equipment[j]))
          {
            obj_object = ch->equipment[j];
            if (isSet(obj_object->flags_.extra_flags, ITEM_NODROP) && ch->getLevel() <= MORTAL)
            {
              dc_sprintf(arg1, "You can't remove %s, it must be CURSED!\r\n", qPrintable(obj_object->short_description()));
              send_to_char(arg1, ch);
              continue;
            }
            if (dc_->obj_index[obj_object->item_number].vnum() == 30010 && obj_object->flags_.timer < 40)
            {
              ch->sendln("The ruby brooch is bound to your flesh. You cannot remove it!");
              continue;
            }

            if (dc_->obj_index[obj_object->item_number].vnum() == SPIRIT_SHIELD_OBJ_NUMBER)
            {
              send_to_room("The spirit shield shimmers brightly then fades away.\r\n", ch->in_room);
              extract_obj(obj_object);
              continue;
            }
            else
              obj_to_char(ch->unequip_char(j), ch);
            act_to_character("You stop using $p.", ch, obj_object, 0, 0);
            act_to_room("$n stops using $p.", ch, obj_object, 0, INVIS_NULL);
          }
        }
        else
        {
          ch->sendln("You can't carry that many items.");
          j = MAX_WEAR;
        }
      }
    }
    else
    {
      obj_object = ch->get_object_in_equip_vis(arg1, ch->equipment, &j, false);
      if (!obj_object && IS_AFFECTED(ch, AFF_BLIND) && ch->has_skill(SKILL_BLINDFIGHTING))
      {
        obj_object = ch->get_object_in_equip_vis(arg1, ch->equipment, &j, true);
        blindlag = true;
      }
      if (obj_object)
      {
        if (CAN_CARRY_N(ch) != IS_CARRYING_N(ch))
        {
          if (isSet(obj_object->flags_.extra_flags, ITEM_NODROP) && ch->getLevel() <= MORTAL)
          {
            dc_sprintf(arg1, "You can't remove %s, it must be CURSED!\r\n", qPrintable(obj_object->short_description()));
            send_to_char(arg1, ch);
            return ReturnValue::eFAILURE;
          }
          if (dc_->obj_index[obj_object->item_number].vnum() == 30010 && obj_object->flags_.timer < 40)
          {
            ch->sendln("The ruby brooch is bound to your flesh. You cannot remove it!");
            return ReturnValue::eFAILURE;
          }

          if (will_screwup_worn_sizes(ch, obj_object, 0))
          {
            // will_screwup_worn_sizes() takes care of the messages
            return ReturnValue::eFAILURE;
          }
          if (j == WEAR_WIELD)
          {
            obj_to_char(ch->unequip_char(j), ch);
            ch->equipment[WEAR_WIELD] = ch->equipment[WEAR_SECOND_WIELD];
            ch->equipment[WEAR_SECOND_WIELD] = {};
          }
          else if (dc_->obj_index[obj_object->item_number].vnum() == SPIRIT_SHIELD_OBJ_NUMBER)
          {
            send_to_room("The spirit shield shimmers brightly then fades away.\r\n", ch->in_room);
            extract_obj(obj_object);
            return ReturnValue::eSUCCESS;
          }
          else
            obj_to_char(ch->unequip_char(j), ch);

          act_to_character("You stop using $p.", ch, obj_object, 0, 0);
          act_to_room("$n stops using $p.", ch, obj_object, 0, INVIS_NULL);
          if (blindlag)
            WAIT_STATE(ch, DC::PULSE_VIOLENCE);
        }
        else
        {
          ch->sendln("You can't carry that many items.");
          j = MAX_WEAR;
        }
      }
      else
      {
        ch->sendln("You are not using it.");
      }
    }
  }
  else
  {
    ch->sendln("Remove what?");
  }
  return ReturnValue::eSUCCESS;
}

// Urizen, hack of will_screwup_worn_sizes
// Checks for, and removes items that are no longer
// wear-able, because of disarm, scrap etc.
qint32 Character::recheck_height_wears(void)
{
  qint32 j;
  ObjectPtr obj = {};
  if (isNonPlayer())
    return ReturnValue::eFAILURE; // NPCs get to wear the stuff.

  for (j = {}; j < MAX_WEAR; j++)
  {
    if (!equipment[j])
      continue;

    if (size_restricted(this, equipment[j]))
    {
      obj = unequip_char(j);
      obj_to_char(obj, this);
      act_to_room("$n looks uncomfortable, and shifts $p into $s inventory.", this, obj, nullptr, 0);
      act_to_character("$p feels uncomfortable and you shift it into your inventory.", this, obj, nullptr, 0);
    }
  }
  return ReturnValue::eSUCCESS;
}

bool fullSave(ObjectPtr obj)
{
  if (!obj)
    return 0;

  if (eq_current_damage(obj))
    return 1;

  ObjectPtr tmp_obj = get_obj(GET_OBJ_VNUM(obj));
  if (!tmp_obj)
  {
    QString buf;
    dc_sprintf(buf, "crash bug! objects.cpp, tmp_obj was null! %s is obj", qPrintable(obj->name()));
    dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    return 0;
  }

  if (isSet(obj->flags_.more_flags, ITEM_CUSTOM))
  {
    return 1;
  }

  if (isSet(obj->flags_.more_flags, ITEM_24H_SAVE))
  {
    return 1;
  }

  if (isSet(obj->flags_.more_flags, ITEM_24H_NO_SELL))
  {
    return 1;
  }

  if (dc_strcmp(GET_OBJ_SHORT(obj), GET_OBJ_SHORT(tmp_obj)))
    return 1;

  if (obj->name() != tmp_obj->name()) // GL. and stuff.
    return 1;

  if (obj->flags_.extra_flags != tmp_obj->flags_.extra_flags)
    return 1;

  if (obj->flags_.more_flags != tmp_obj->flags_.more_flags)
    return 1;

  if (obj->flags_.type_flag == ITEM_STAFF && obj->flags_.value[1] != obj->flags_.value[2])
    return 1;

  if (obj->flags_.type_flag == ITEM_WAND && obj->flags_.value[1] != obj->flags_.value[2])
    return 1;

  if (obj->flags_.type_flag == ITEM_DRINKCON && obj->flags_.value[0] != obj->flags_.value[1])
    return 1;

  return 0;
}

void Character::heightweight(bool add)
{
  qint32 i, j;
  for (i = {}; i < MAX_WEAR; i++)
  {
    if (equipment[i])
      for (j = {}; j < equipment[i]->num_affects; j++)
      {
        if (equipment[i]->affected[j].location == APPLY_CHAR_HEIGHT)
          affect_modify(this, equipment[i]->affected[j].location, equipment[i]->affected[j].modifier, -1, add);
        else if (equipment[i]->affected[j].location == APPLY_CHAR_WEIGHT)
          affect_modify(this, equipment[i]->affected[j].location, equipment[i]->affected[j].modifier, -1, add);
      }
  }
}

bool Character::allowColor(void)
{
  if (player)
  {
    if (isSet(player->toggles, Player::PLR_ANSI))
    {
      return true;
    }
  }
  else if (desc && desc->allowColor)
  {
    return true;
  }
  return false;
}

qint32 obj_from(ObjectPtr obj)
{
  if (obj == nullptr)
  {
    return false;
  }

  if (obj->in_obj)
  {
    return obj_from_obj(obj);
  }

  if (obj->in_room)
  {
    return obj_from_room(obj);
  }

  if (obj->carried_by)
  {
    return obj_from_char(obj);
  }

  return false;
}

bool Object::isDark(void)
{
  return isSet(flags_.extra_flags, ITEM_DARK);
}

quint64 Object::getLevel(void)
{
  return flags_.eq_level;
}

bool Object::isQuest(void)
{
  return isexact("quest", name()) ||
         dc_->obj_index[item_number].vnum() == 3124 ||
         dc_->obj_index[item_number].vnum() == 3125 ||
         dc_->obj_index[item_number].vnum() == 3126 ||
         dc_->obj_index[item_number].vnum() == 3127 ||
         dc_->obj_index[item_number].vnum() == 3128 ||
         dc_->obj_index[item_number].vnum() == 27997 ||
         dc_->obj_index[item_number].vnum() == 27998 ||
         dc_->obj_index[item_number].vnum() == 27999;
}

bool Object::isTest(void)
{
  return isexact(u"test"_s, name());
}

bool Object::isGodload(void)
{
  return isexact(u"gl"_s, name()) || isexact(u"godload"_s, name()) || isSet(flags_.extra_flags, ITEM_SPECIAL);
}

bool Object::hasPortalFlagNoLeave(void)
{
  return isSet(getPortalFlags(), Object::portal_flags_t::No_Leave);
}

bool Object::hasPortalFlagNoEnter(void)
{
  return isSet(getPortalFlags(), Object::portal_flags_t::No_Enter);
}

Object::~Object()
{
  ExtraDescriptionPtr next_one;
  for (auto ths = ex_description; ths; ths = next_one)
  {
    next_one = ths->next;
    ths = {};
  }
}