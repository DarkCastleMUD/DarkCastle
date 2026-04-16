/************************************************************************
 | $Id: move.cpp,v 1.99 2015/06/14 02:38:12 pirahna Exp $
 | move.C
 | Movement commands and stuff.
 *************************************************************************
 *  Revision History                                                     *
 *  11/09/2003   Onager   Changed do_unstable() and do_fall() to call    *
 *                        noncombat_damage() for damage                  *
 *************************************************************************
 */
#include "DC/DC.h"

qint32 check_ethereal_focus(CharacterPtr ch, qint32 trigger_type); // class/cl_mage.cpp

qint32 move_player(CharacterPtr ch, qint32 room)
{
  qint32 retval;

  retval = move_char(ch, room);

  if (!isSet(retval, ReturnValue::eSUCCESS))
  {
    retval = move_char(ch, real_room(START_ROOM));
    if (!isSet(retval, ReturnValue::eSUCCESS))
      dc_->logentry(u"Error in move_player(), Failure moving ch to start room. move_player_home_nofail"_s,
                    IMMORTAL, DC::LogChannel::LOG_BUG);
  }

  return retval;
}

void move_player_home(CharacterPtr victim)
{
  qint32 was_in = victim->in_room;
  qint32 found = {};
  ClanPtr clan = {};
  // check for homes that don't exist
  if (real_room(victim->hometown < 1))
    victim->hometown = START_ROOM;

  // next four lines ping-pong people from meta to tavern to help lessen spam
  if (dc_->world[was_in].number == victim->hometown && victim->hometown == START_ROOM)
    move_player(victim, real_room(SECOND_START_ROOM));
  else if (dc_->world[was_in].number == victim->hometown || IS_AFFECTED(victim, AFF_CHAMPION))
    move_player(victim, real_room(START_ROOM));
  // recalling into a clan room
  else if (!isSet(dc_->world[real_room(victim->hometown)].room_flags, CLAN_ROOM))
    move_player(victim, real_room(victim->hometown));
  // Clanroom else
  else
  {
    if (!victim->clan || !(clan = get_clan(victim)))
    {
      victim->sendln("The gods frown on you, and reset your home.");
      victim->hometown = START_ROOM;
      move_player(victim, real_room(victim->hometown));
    }
    else
    {
      found = false;
      for (const auto &room : clan->rooms_)
        if (room.room_number == victim->hometown)
        {
          found = true;
          break;
        }

      if (!found)
      {
        victim->sendln("The gods frown on you, and reset your home.");
        victim->hometown = START_ROOM;
      }
      move_player(victim, real_room(victim->hometown));
    }
  }
}

// Rewritten 9/1/96
// -Sadus
void record_track_data(CharacterPtr ch, cmd_t cmd)
{
  // Rangers outdoors leave no tracks
  if (dc_->world[ch->in_room].sector_type != SECT_INSIDE &&
      GET_CLASS(ch) == CLASS_RANGER)
    return;

  // If we just used a scent killer like catstink, don't leave tracks
  // on our next move
  if (ISSET(ch->affected_by, AFF_UTILITY))
  {
    REMBIT(ch->affected_by, AFF_UTILITY);
    return;
  }

  if (dc_->world[ch->in_room].sector_type == SECT_WATER_SWIM || dc_->world[ch->in_room].sector_type == SECT_WATER_NOSWIM)
    return;

  auto newScent = TracksPtr::create();
  auto valid_dir = getDirectionFromCommand(cmd);
  if (valid_dir)
    newScent->direction = *valid_dir;
  else
    newScent->direction = {};
  newScent->weight = (qint32)ch->weight;
  newScent->race = (qint32)ch->race;
  newScent->sex = (qint32)ch->sex;
  newScent->condition = ((ch->getHP() * 100) / (GET_MAX_HIT(ch) == 0 ? 100 : GET_MAX_HIT(ch)));
  newScent->next = {};     // just in case
  newScent->previous = {}; // just in case

  newScent->trackee = ch->name();

  dc_->world[ch->in_room].AddTrackItem(newScent);
}

// Removed this due to it being a funky cold medina. - Nocturnal 09/28/05
// void do_muddy(CharacterPtr ch)
//{
//   short chance = dc_->number(0,30);
//
//   if(ch->isNonPlayer() || IS_AFFECTED(ch, AFF_FLYING) || ch->isImmortalPlayer() || IS_AFFECTED(ch, AFF_FREEFLOAT)) {
//     ; //poop on a stick!
//   } else if(GET_DEX(ch) > chance) {
//      act_to_character("You barely avoid slipping in the mud.", ch, 0, 0,  0);
//   } else {
//      act_to_room("$n slips on the muddy terrain and goes down.", ch, 0, 0,  0);
//      act_to_character("Your feet slide out from underneath you in the mud.", ch, 0, 0,  0);
//      ch->setSitting();
//   }
//}

command_return_t do_unstable(CharacterPtr ch)
{
  QString death_log;
  qint32 retval;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  short chance = dc_->number(0, 30);

  if (IS_AFFECTED(ch, AFF_FLYING) || IS_AFFECTED(ch, AFF_FREEFLOAT) ||
      ch->isImmortalPlayer())
    return ReturnValue::eFAILURE;

  if (GET_DEX(ch) > chance)
  {
    act_to_character("You dextrously maintain your footing.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  act_to_room("$n looses his footing on the unstable ground.", ch, 0, 0, 0);
  act_to_character("You fall flat on your ass.", ch, 0, 0, 0);
  ch->setSitting();

  dc_sprintf(death_log, "%s slipped to death in room %d.", qPrintable(ch->name()), dc_->world[ch->in_room].number);
  retval = noncombat_damage(ch, GET_MAX_HIT(ch) / 12, "You feel your back snap painfully and all goes dark...",
                            "$n's frail body snaps in half as $e is buffeted about the room.", death_log, KILL_FALL);

  if (SOMEONE_DIED(retval))
    return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
  else
    return ReturnValue::eSUCCESS;
}

command_return_t do_fall(CharacterPtr ch, short dir)
{
  qint32 target;
  QString damage;
  qint32 retval;
  qint32 dam = dc_->number(50, 100);

  if (IS_AFFECTED(ch, AFF_FLYING))
    return ReturnValue::eFAILURE;

  if (ch->getLevel() == 50)
    dam = dc_->number(100, 200);

  if (GET_MAX_HIT(ch) > 2000)
    dam = dc_->number(400, 500);
  else if (GET_MAX_HIT(ch) > 1000)
    dam = dc_->number(200, 400);

  if (ch->isImmortalPlayer())
  {
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_FREEFLOAT))
  {
    dam = {};
    ch->sendln("Your freefloating magics reduce your fall to a safer speed.");
  }

  // Don't effect mobs
  if (ch->isNonPlayer())
  {
    act_to_room("$n clings to the terrain around $m and avoids falling.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (!CAN_GO(ch, dir))
    return ReturnValue::eFAILURE;

  target = dc_->world[ch->in_room].dir_option[dir]->to_room;

  if (isSet(dc_->world[target].room_flags, IMP_ONLY) && ch->getLevel() < IMPLEMENTER)
    return ReturnValue::eFAILURE;

  if (isSet(dc_->world[target].room_flags, TUNNEL))
  {
    qint32 ppl = {};
    CharacterPtr k;
    for (k = dc_->world[target].people_; k; k = k->next_in_room)
      if (k->isPlayer())
        ppl++;
    if (ppl > 2)
    {
      ch->sendln("There isn't enough space for you to follow.");
      return ReturnValue::eFAILURE;
    }
  }

  if (isSet(dc_->world[target].room_flags, PRIVATE))
  {
    qint32 ppl = {};
    CharacterPtr k;
    for (k = dc_->world[target].people_; k; k = k->next_in_room)
      if (k->isPlayer())
        ppl++;
    if (ppl > 4)
    {
      ch->sendln("There isn't enough space for you to follow.");
      return ReturnValue::eFAILURE;
    }
  }

  switch (dir)
  {
  case 0:
    act_to_room("$n rolls out to the north.", ch, 0, 0, 0);
    ch->sendln("You tumble to the north...");
    break;
  case 1:
    act_to_room("$n rolls out to the east.", ch, 0, 0, 0);
    ch->sendln("You tumble to the east...");
    break;
  case 2:
    act_to_room("$n rolls out to the south.", ch, 0, 0, 0);
    ch->sendln("You tumble to the south...");
    break;
  case 3:
    act_to_room("$n rolls out to the west.", ch, 0, 0, 0);
    ch->sendln("You tumble to the west...");
    break;
  case 4:
    act_to_room("$n is launched upwards.", ch, 0, 0, 0);
    ch->sendln("You are launched into the air...");
    break;
  case 5:
    act_to_room("$n falls through the room.", ch, 0, 0, 0);
    ch->sendln("You fall...");
    break;
  default:
    dc_->logentry(u"Default hit in do_fall"_s, IMMORTAL, DC::LogChannel::LOG_MORTAL);
    break;
  }

  retval = move_char(ch, target);

  if (!isSet(retval, ReturnValue::eSUCCESS))
  {
    ch->sendln("You are miraculously upheld by divine powers!");
    return retval;
  }

  do_look(ch, "\0");

  dc_sprintf(damage, "%s's fall from %d was lethal and it killed them.", qPrintable(ch->name()), dc_->world[ch->in_room].number);
  retval = noncombat_damage(ch, dam, "Luckily the ground breaks your fall.\r\n", "$n plummets into the room and hits the ground with a wet-sounding splat!",
                            damage, KILL_FALL);

  if (SOMEONE_DIED(retval))
    return ReturnValue::eSUCCESS;

  if (!SOMEONE_DIED(retval))
  {
    act_to_room("$n plummets into the room and hits the floor HARD.", ch, 0, 0, 0);
  }

  if ((isSet(dc_->world[ch->in_room].room_flags, FALL_DOWN) && (dir = 5)) || (isSet(dc_->world[ch->in_room].room_flags, FALL_UP) && (dir = 4)) || (isSet(dc_->world[ch->in_room].room_flags, FALL_EAST) && (dir = 1)) || (isSet(dc_->world[ch->in_room].room_flags, FALL_WEST) && (dir = 3)) || (isSet(dc_->world[ch->in_room].room_flags, FALL_SOUTH) && (dir = 2)) || (isSet(dc_->world[ch->in_room].room_flags, FALL_NORTH) && (dir = 0)))
  {
    return do_fall(ch, dir);
  }

  // We lived
  return ReturnValue::eSUCCESS;
}

// Assumes
// 1. No master and no followers.
// 2. That the direction exists.
command_return_t do_simple_move(CharacterPtr ch, cmd_t cmd, qint32 following)
{
  QString tmp;
  qint32 was_in;
  qint32 need_movement, learned, mvinroom = 0, mvtoroom = {};
  qint32 retval;
  ObjectPtr obj;
  bool has_boat;

  /*
   I think this is taken care of in the command interpreter.  Disabling it for now.
   -pir 7/25/01
   // Check for special routines (North is 1)
   retval = special(ch, cmd+1, "");
   if(isSet(retval, ReturnValue::eSUCCESS) || isSet(retval, ReturnValue::eCH_DIED))
   return retval;
   */

  auto valid_dir = getDirectionFromCommand(cmd);
  qint32 dir = {};
  if (valid_dir)
    dir = *valid_dir;

  if (ch == nullptr || ch->in_room < 1 || !valid_dir || dc_->world[ch->in_room].dir_option[dir] == nullptr || dc_->world[ch->in_room].dir_option[dir]->to_room < 1)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Error in room %d.", ch->in_room);
    ch->send("There was an error performing that movement.\r\n");
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_FLYING))
    need_movement = 1;
  else
  {
    if ((learned = ch->has_skill(SKILL_NATURES_LORE)))
    {

      if (learned > 90)
      {
        if (dc_->world[ch->in_room].sector_type == SECT_UNDERWATER)
        {
          mvinroom = movement_loss[dc_->world[ch->in_room].sector_type] / 2;
          ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
        }
        if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_UNDERWATER)
          mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type] / 2;
      }

      if (learned > 80)
      {
        if (dc_->world[ch->in_room].sector_type == SECT_MOUNTAIN)
        {
          mvinroom = movement_loss[dc_->world[ch->in_room].sector_type] / 2;
          if (learned < 91)
            ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
        }
        if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_MOUNTAIN)
          mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type] / 2;
      }

      if (learned > 70)
      {
        if (dc_->world[ch->in_room].sector_type == SECT_ARCTIC)
        {
          mvinroom = movement_loss[dc_->world[ch->in_room].sector_type] / 2;
          if (learned < 81)
            ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
        }
        if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_ARCTIC)
          mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type] / 2;
      }

      if (learned > 60)
      {
        if (dc_->world[ch->in_room].sector_type == SECT_HILLS)
        {
          mvinroom = movement_loss[dc_->world[ch->in_room].sector_type] / 2;
          if (learned < 71)
            ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
        }
        if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_HILLS)
          mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type] / 2;
      }

      if (learned > 50)
      {
        if (dc_->world[ch->in_room].sector_type == SECT_WATER_SWIM)
        {
          mvinroom = movement_loss[dc_->world[ch->in_room].sector_type] / 2;
          if (learned < 61)
            ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
        }
        if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_WATER_SWIM)
          mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type] / 2;
      }

      if (learned > 40)
      {
        if (dc_->world[ch->in_room].sector_type == SECT_FROZEN_TUNDRA)
        {
          mvinroom = movement_loss[dc_->world[ch->in_room].sector_type] / 2;
          if (learned < 51)
            ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
        }
        if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_FROZEN_TUNDRA)
          mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type] / 2;
      }

      if (learned > 30)
      {
        if (dc_->world[ch->in_room].sector_type == SECT_DESERT)
        {
          mvinroom = movement_loss[dc_->world[ch->in_room].sector_type] / 2;
          if (learned < 41)
            ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
        }
        if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_DESERT)
          mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type] / 2;
      }

      if (learned > 20)
      {
        if (dc_->world[ch->in_room].sector_type == SECT_BEACH)
        {
          mvinroom = movement_loss[dc_->world[ch->in_room].sector_type] / 2;
          if (learned < 31)
            ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
        }
        if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_BEACH)
          mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type] / 2;
      }

      if (learned > 10)
      {
        if (dc_->world[ch->in_room].sector_type == SECT_FOREST)
        {
          mvinroom = movement_loss[dc_->world[ch->in_room].sector_type] / 2;
          if (learned < 21)
            ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
        }
        if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_FIELD)
          mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type] / 2;
      }

      if (learned > 0)
      {
        if (dc_->world[ch->in_room].sector_type == SECT_FIELD)
        {
          mvinroom = movement_loss[dc_->world[ch->in_room].sector_type] / 2;
          if (learned < 11)
            ch->skill_increase_check(SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
        }
        if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_FOREST)
          mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type] / 2;
      }

      if (!mvinroom)
        mvinroom = movement_loss[dc_->world[ch->in_room].sector_type];
      if (!mvtoroom)
        mvtoroom = movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type];
      need_movement = (mvinroom + mvtoroom) / 2;
    }
    else
    {
      need_movement = (movement_loss[dc_->world[ch->in_room].sector_type] + movement_loss[dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type]) / 2;
    }

    // if I'm trying to go "up" into a "fall down" room, etc.
    // it's OK to go east into a "fall north" room though
    // not ok, if room we're going to is AIR though
    auto cmd_to_dir = getDirectionFromCommand(cmd);
    if (!IS_AFFECTED(ch, AFF_FLYING) && ((cmd == cmd_t::NORTH && isSet(dc_->world[dc_->world[ch->in_room].dir_option[NORTH]->to_room].room_flags, FALL_SOUTH)) ||
                                         (cmd == cmd_t::EAST && isSet(dc_->world[dc_->world[ch->in_room].dir_option[EAST]->to_room].room_flags, FALL_WEST)) ||
                                         (cmd == cmd_t::SOUTH && isSet(dc_->world[dc_->world[ch->in_room].dir_option[SOUTH]->to_room].room_flags, FALL_NORTH)) ||
                                         (cmd == cmd_t::WEST && isSet(dc_->world[dc_->world[ch->in_room].dir_option[WEST]->to_room].room_flags, FALL_EAST)) ||
                                         (cmd == cmd_t::UP && isSet(dc_->world[dc_->world[ch->in_room].dir_option[UP]->to_room].room_flags, FALL_DOWN)) ||
                                         (cmd == cmd_t::DOWN && isSet(dc_->world[dc_->world[ch->in_room].dir_option[DOWN]->to_room].room_flags, FALL_UP)) ||
                                         cmd_to_dir && dc_->world[dc_->world[ch->in_room].dir_option[*cmd_to_dir]->to_room].sector_type == SECT_AIR))
    {
      ch->sendln("You would need to fly to go there!");
      return ReturnValue::eFAILURE;
    }

    // fly doesn't work over water
    if ((dc_->world[ch->in_room].sector_type == SECT_WATER_NOSWIM) || (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_WATER_NOSWIM))
    {
      has_boat = false;
      // See if character is carrying a boat
      for (obj = ch->carrying; obj; obj = obj->next_content)
        if (obj->obj_flags.type_flag == ITEM_BOAT)
          has_boat = true;
      // See if character is wearing a boat (boat ring, etc)
      if (!has_boat)
        for (qint32 x = {}; x < MAX_WEAR; x++)
          if (ch->equipment[x])
            if (ch->equipment[x]->obj_flags.type_flag == ITEM_BOAT)
              has_boat = true;
      if (!has_boat && !IS_AFFECTED(ch, AFF_FLYING) && !ch->isImmortalPlayer() &&
          ch->race != RACE_FISH && ch->race != RACE_SLIME && !IS_AFFECTED(ch, AFF_FREEFLOAT))
      {
        ch->sendln("You need a boat to go there.");
        return ReturnValue::eFAILURE;
      }
    }

    if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type != SECT_WATER_NOSWIM && dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type != SECT_WATER_SWIM && dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type != SECT_UNDERWATER)
    {
      // It's NOT a water room and we don't have fly
      if (ch->race == RACE_FISH)
      {
        ch->sendln("You can't swim around outside water without being able to fly!");
        act_to_room("$n flops around in a futile attempt to move out of water.", ch, 0, 0, INVIS_NULL);
        return ReturnValue::eFAILURE;
      }
    }
  } // else if !FLYING

  if (isSet(dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].room_flags, IMP_ONLY) &&
      ch->getLevel() < IMPLEMENTER)
  {
    ch->sendln("No.");
    return ReturnValue::eFAILURE;
  }

  Room *rm = &(dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room]);

  if (rm->sector_type != dc_->world[ch->in_room].sector_type && ch->conn_ && ch->conn_->original && ch->conn_->original->getLevel() <= DC::MAX_MORTAL_LEVEL)
  {
    qint32 s2 = rm->sector_type, s1 = dc_->world[ch->in_room].sector_type;
    if ((s1 == SECT_CITY && (s2 != SECT_INSIDE && s2 != SECT_PAVED_ROAD)) || (s1 == SECT_INSIDE && (s2 != SECT_CITY && s2 != SECT_PAVED_ROAD)) || (s1 == SECT_PAVED_ROAD && (s2 != SECT_INSIDE && s2 != SECT_CITY)) || (s1 == SECT_FIELD && (s2 != SECT_HILLS && s2 != SECT_MOUNTAIN)) || (s1 == SECT_HILLS && (s2 != SECT_MOUNTAIN && s2 != SECT_FIELD)) || (s1 == SECT_MOUNTAIN && (s2 != SECT_HILLS && s2 != SECT_FIELD)) || (s1 == SECT_WATER_NOSWIM && (s2 != SECT_UNDERWATER && s2 != SECT_WATER_SWIM)) || (s1 == SECT_WATER_SWIM && (s2 != SECT_UNDERWATER && s2 != SECT_WATER_NOSWIM)) || (s1 == SECT_UNDERWATER && (s2 != SECT_WATER_NOSWIM && s2 != SECT_WATER_SWIM)) || (s1 == SECT_BEACH && (s2 != SECT_DESERT)) || (s1 == SECT_DESERT && (s2 != SECT_BEACH)) || (s1 == SECT_FROZEN_TUNDRA && (s2 != SECT_ARCTIC)) || (s1 == SECT_ARCTIC && (s2 != SECT_FROZEN_TUNDRA)) || (s1 == SECT_AIR) || (s1 == SECT_SWAMP))
    {
      ch->sendln("The ghost evaporates as you leave its habitat.");
      do_return(ch, "");
      // extract_char(ch,true);
      return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
    }
  }

  if (isSet(rm->room_flags, TUNNEL))
  {
    qint32 ppl = {};
    CharacterPtr k;
    for (k = rm->people; k; k = k->next_in_room)
      if (k->isPlayer())
        ppl++;
    if (ppl > 2)
    {
      ch->sendln("There's no room.");
      return ReturnValue::eFAILURE;
    }
  }

  if (!ch->isImmortalPlayer())
  {
    bool classRestrictions = false;
    // Determine if any class restrictions are in place
    for (qint32 c_class = 1; c_class < CLASS_MAX; c_class++)
    {
      if (rm->allow_class[c_class] == true)
      {
        classRestrictions = true;
      }
    }

    if (classRestrictions)
    {
      if (rm->allow_class[GET_CLASS(ch)] != true)
      {
        ch->sendln("Your class is not allowed there.");
        return ReturnValue::eFAILURE;
      }
    }
  }

  if (isSet(rm->room_flags, PRIVATE))
  {
    qint32 ppl = {};
    CharacterPtr k;
    for (k = rm->people; k; k = k->next_in_room)
      if (k->isPlayer())
        ppl++;
    if (ppl > 4)
    {
      ch->sendln("There's no room.");
      return ReturnValue::eFAILURE;
    }
  }

  if (ch->isPlayer() && dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type == SECT_UNDERWATER && !(ch->affected_by_spell(SPELL_WATER_BREATHING) || IS_AFFECTED(ch, AFF_WATER_BREATHING)))
  {
    ch->sendln("You feel air trying to explode from your lungs as you swim around.");
    // ch->sendln("Underwater?!");
    // return ReturnValue::eFAILURE;
  }

  // if I'm STAY_NO_TOWN, don't enter a Zone::Flag::IS_TOWN zone no matter what
  if (ch->isNonPlayer() && ISSET(ch->mobdata->actflags, ACT_STAY_NO_TOWN) && dc_->zones.value(dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].zone).isTown())
    return ReturnValue::eFAILURE;

  qint32 a = {};
  if ((a = ch->has_skill(SKILL_VIGOR)) && dc_->number(1, 101) < a / 10)
    need_movement /= 2; // No skill improvement here. Too easy.

  if (GET_MOVE(ch) < need_movement && ch->isPlayer())
  {
    if (!following)
      ch->sendln("You are too exhausted.");
    else
      ch->sendln("You are too exhausted to follow.");
    return ReturnValue::eFAILURE;
  }

  // If they were hit with a lullaby, go ahead and clear it out
  if (ISSET(ch->affected_by, AFF_NO_FLEE))
  {
    if (ch->affected_by_spell(SPELL_IRON_ROOTS))
    {
      ch->sendln("The roots bracing your legs keep you from moving!");
      return ReturnValue::eFAILURE;
    }
    else
    {
      REMBIT(ch->affected_by, AFF_NO_FLEE);
      ch->sendln("The travel wakes you up some, and clears the drowsiness from your legs.");
    }
  }

  if (!ch->isImmortalPlayer())
    ch->decrementMove(need_movement);

  // Everyone
  if (!IS_AFFECTED(ch, AFF_SNEAK) && !IS_AFFECTED(ch, AFF_FOREST_MELD))
  {
    dc_sprintf(tmp, "$n leaves %s.", dirs[dir]);
    act_to_room(tmp, ch, 0, 0, INVIS_NULL);
  }
  // Sneaking
  else if (IS_AFFECTED(ch, AFF_SNEAK))
  {
    QString tmp;
    if (!skill_success(ch, nullptr, SKILL_SNEAK))
    {
      dc_sprintf(tmp, "$n leaves %s.", dirs[dir]);
      act_to_room(tmp, ch, 0, 0, INVIS_NULL | STAYHIDE);
    }
    else
    {
      dc_sprintf(tmp, "$n sneaks %s.", dirs[dir]);
      act_to_group(tmp, ch, 0, 0, INVIS_NULL);
      act_to_room(tmp, ch, 0, 0, GODS);
    }
  }
  // Forest melded
  else
  {
    if (dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type != SECT_FOREST && dc_->world[dc_->world[ch->in_room].dir_option[dir]->to_room].sector_type != SECT_SWAMP)
    {
      dc_sprintf(tmp, "$n leaves %s.", dirs[dir]);
      REMBIT(ch->affected_by, AFF_FOREST_MELD);
      ch->sendln("You detach yourself from the forest.");
      act_to_room(tmp, ch, 0, 0, INVIS_NULL);
    }
    else
    {
      dc_sprintf(tmp, "$n sneaks %s.", dirs[dir]);
      act_to_room(tmp, ch, 0, 0, GODS);
    }
  }

  // at this point we messaged that we are moving, but we haven't actually moved yet.  Check if ethereal focus keeps us from moving.
  retval = check_ethereal_focus(ch, ETHEREAL_FOCUS_TRIGGER_MOVE);
  if (isSet(retval, ReturnValue::eFAILURE))
    return retval;

  was_in = ch->in_room;
  record_track_data(ch, cmd);

  retval = move_char(ch, dc_->world[was_in].dir_option[dir]->to_room);

  if (isSet(retval, ReturnValue::eSUCCESS) && IS_AFFECTED(ch, AFF_CRIPPLE))
  {
    ch->sendln("Your crippled body responds slowly.");
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  }

  ObjectPtr tmp_obj;
  for (tmp_obj = dc_->world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (dc_->obj_index[tmp_obj->item_number].vnum() == SILENCE_OBJ_NUMBER)
      ch->sendln("The sounds around you fade to nothing as the silence takes hold...");

  for (tmp_obj = dc_->world[was_in].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (dc_->obj_index[tmp_obj->item_number].vnum() == SILENCE_OBJ_NUMBER)
      ch->sendln("The noise around you returns as you leave the silenced area!");

  if (!isSet(retval, ReturnValue::eSUCCESS))
  {
    ch->sendln("You fail to move.");
    return retval;
  }

  // Fighting
  if (ch->fighting)
  {
    CharacterPtr chaser = ch->fighting;
    if (ch->isNonPlayer())
    {
      ch->add_memory(qPrintable(chaser->name()), 'f');
      remove_memory(ch, 'h');
    }
    if (chaser->isNonPlayer() && chaser->hunting.isEmpty())
    {
      level_diff_t level_difference = ch->getLevel() - chaser->getLevel() / 2;
      if (level_difference >= 0 || ch->getLevel() >= 50)
      {
        chaser->add_memory(qPrintable(ch->name()), 't');
        TimerPtr timer = TimerPtr(new Timer);
        timer->var_arg1 = chaser->hunting;
        timer->arg2 = (void *)chaser;
        timer->function = clear_hunt;
        timer->next = timer_list;
        timer_list = timer;
        timer->timeleft = (ch->getLevel() / 4.0) * 60.0;
      }
    }
    if (chaser->fighting == ch)
      stop_fighting(chaser);
    stop_fighting(ch);
    // This might be a bad idea...cause track calls move, which calls track, which...
    if (chaser->isNonPlayer())
    {
      retval = chaser->do_track(chaser->mobdata->hated.split(' '));
      if (SOMEONE_DIED(retval))
        return retval;
    }
  }

  if (IS_AFFECTED(ch, AFF_SNEAK) || (IS_AFFECTED(ch, AFF_FOREST_MELD) && (dc_->world[ch->in_room].sector_type == SECT_FOREST || dc_->world[ch->in_room].sector_type == SECT_SWAMP)))
  {
    act_to_room("$n sneaks into the room.", ch, 0, 0, GODS);
    act_to_group("$n sneaks into the room.", ch, 0, 0, INVIS_NULL);
  }
  else
    act_to_room("$n has arrived.", ch, 0, 0, INVIS_NULL);

  do_look(ch, "\0");

  // Elemental stuff goes HERE
  if (ch->isNonPlayer())
  {
    qint32 a = dc_->mob_index[ch->mobdata->nr].vnum();
    // code a bit repeaty, but whatever ;)
    if (a == 88 && dc_->world[ch->in_room].sector_type == SECT_UNDERWATER)
    {
      act_to_room("Unable to survive underwater, $n returns to the elemental plane of fire.", ch, 0, 0, 0);
      extract_char(ch, true);
      return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
    }
    if (a == 89 && dc_->world[ch->in_room].sector_type == SECT_DESERT)
    {
      act_to_room("Unable to survive in the desert, $n returns to the elemental plane of water.", ch, 0, 0, 0);
      extract_char(ch, true);
      return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
    }
    if (a == 90 && dc_->world[ch->in_room].sector_type == SECT_SWAMP)
    {
      act_to_room("Unable to survive in the swamp, $n returns to the elemental plane of air.", ch, 0, 0, 0);
      extract_char(ch, true);
      return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
    }
    if (a == 91 && dc_->world[ch->in_room].sector_type == SECT_AIR)
    {
      act_to_room("Unable to survive in the air, $n returns to the elemental plane of earth.", ch, 0, 0, 0);
      extract_char(ch, true);
      return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
    }
  }
  // Elemental stuff ends HERE

  if ((isSet(dc_->world[ch->in_room].room_flags, FALL_NORTH) && (dir = 0)) || (isSet(dc_->world[ch->in_room].room_flags, FALL_DOWN) && (dir = 5)) || (isSet(dc_->world[ch->in_room].room_flags, FALL_UP) && (dir = 4)) || (isSet(dc_->world[ch->in_room].room_flags, FALL_EAST) && (dir = 1)) || (isSet(dc_->world[ch->in_room].room_flags, FALL_WEST) && (dir = 3)) || (isSet(dc_->world[ch->in_room].room_flags, FALL_SOUTH) && (dir = 2)))
  {
    retval = do_fall(ch, dir);
    if (SOMEONE_DIED(retval))
      return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
  }

  if (isSet(dc_->world[ch->in_room].room_flags, UNSTABLE))
  {
    retval = do_unstable(ch);
    if (SOMEONE_DIED(retval))
      return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
  }

  /*
   if(isSet(dc_->world[ch->in_room].sector_type, SECT_FIELD) &&
   weather_info.sky == SKY_HEAVY_RAIN && !number(0,19)) {
   do_muddy(ch);
   }
   */
  if ((GET_CLASS(ch) == CLASS_BARD && ch->has_skill(SKILL_SONG_HYPNOTIC_HARMONY)) || GET_CLASS(ch) == CLASS_RANGER)
    for (CharacterPtr tmp_ch = dc_->world[ch->in_room].people_; tmp_ch; tmp_ch = tmp_ch->next_in_room)
    {
      if (tmp_ch->isPlayer())
        continue;
      if (IS_AFFECTED(tmp_ch, AFF_CHARM))
        continue;
      if (tmp_ch->fighting)
        continue;
      if (isSet(tmp_ch->immune, ISR_CHARM))
        continue;
      if (!number(0, 1))
        continue;

      if (GET_CLASS(ch) == CLASS_BARD && ISSET(tmp_ch->mobdata->actflags, ACT_BARDCHARM))
      {
        act_to_character("$N looks at you expectantly, perhaps hoping for a song?", ch, nullptr, tmp_ch, 0);
        act_to_room("$N looks at $n expectantly, perhaps hoping for a song?", ch, nullptr, tmp_ch, INVIS_NULL);
      }
      else if (GET_CLASS(ch) == CLASS_RANGER && ISSET(tmp_ch->mobdata->actflags, ACT_CHARM) && ch->getLevel() >= tmp_ch->getLevel() && CAN_SEE(tmp_ch, ch))
      {
        act_to_character("$N moves submissively out of your way.", ch, nullptr, tmp_ch, 0);
        act_to_room("$N moves submissively out of $n's way.", ch, nullptr, tmp_ch, INVIS_NULL);
      }
    }

  // let our mobs know they're here
  retval = mprog_entry_trigger(ch);
  if (SOMEONE_DIED(retval))
    return retval | ReturnValue::eSUCCESS;
  retval = ch->mprog_greet_trigger();
  if (SOMEONE_DIED(retval))
    return retval | ReturnValue::eSUCCESS;
  retval = ch->oprog_greet_trigger();
  if (SOMEONE_DIED(retval))
    return retval | ReturnValue::eSUCCESS;

  return ReturnValue::eSUCCESS;
}

qint32 attempt_move(CharacterPtr ch, cmd_t cmd, bool is_retreat)
{
  QString tmp;
  qint32 return_val;
  qint32 was_in = ch->in_room;
  follow_type *k, *next_dude;

  if (ch->brace_at)
  {
    ch->send(u"You can't move and brace the %s at the same time!\r\n"_s.arg(qPrintable(fname(ch->brace_at->keyword))));
    return ReturnValue::eFAILURE;
  }

  auto opt_dir = getDirectionFromCommand(cmd);
  if (!opt_dir)
  {
    ch->sendln("Error. Tell an immortal.");
    return ReturnValue::eFAILURE;
  }
  auto dir = *opt_dir;

  if (!dc_->world[ch->in_room].dir_option[dir])
  {
    ch->sendln("You can't go that way.");
    return ReturnValue::eFAILURE;
  }

  if (isSet(EXIT(ch, dir)->exit_info, EX_CLOSED))
  {
    if (isSet(EXIT(ch, dir)->exit_info, EX_HIDDEN))
      ch->sendln("You can't go that way.");
    else if (EXIT(ch, dir)->keyword)
      ch->send(u"The %s seems to be closed.\r\n"_s.arg(qPrintable(fname(EXIT(ch).arg(dir)->keyword))));
    else
      ch->sendln("It seems to be closed.");
    return ReturnValue::eFAILURE;
  }

  if (EXIT(ch, dir)->to_room == DC::NOWHERE)
  {
    ch->sendln("Alas, you can't go that way.");
    return ReturnValue::eFAILURE;
  }

  if (!ch->followers && !ch->master)
  {
    try
    {
      return_val = do_simple_move(ch, cmd, false);
    }
    catch (...)
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Error performing movement in room %d.", ch->in_room);
      return_val = ReturnValue::eFAILURE;
    }

    if (SOMEONE_DIED(return_val) || !isSet(return_val, ReturnValue::eSUCCESS))
      return return_val;
    if (!IS_AFFECTED(ch, AFF_SNEAK))
      return_val = ambush(ch);
    if (SOMEONE_DIED(return_val) || !isSet(return_val, ReturnValue::eSUCCESS))
      return return_val;
    return check_ethereal_focus(ch, ETHEREAL_FOCUS_TRIGGER_MOVE);
  }

  if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master) && (ch->in_room == ch->master->in_room))
  {
    ch->sendln("You are unable to abandon your master.");
    act_to_room("$n trembles as $s mind attempts to free itself from its magical bondage.", ch, 0, 0, 0);
    if (ch->master->isPlayer() && GET_CLASS(ch->master) == CLASS_BARD)
    {
      ch->master->sendln("You struggle to maintain control.");
      /*
       if (GET_KI(ch->master) < 5) {
       ch->add_memory(qPrintable(ch->master->name()), 'h');
       stop_follower(ch, follower_reasons_t::BROKE_CHARM);
       //ch->add_memory(qPrintable(ch->master->name()), 'h');
       do_say(ch, "Hey! You tricked me!");
       ch->master->sendln("You lose control.");
       }
       else
       GET_KI(ch->master) -= 5;
       */
    }
    return ReturnValue::eFAILURE;
  }

  try
  {
    return_val = do_simple_move(ch, cmd, true);
  }
  catch (...)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Error performing movement in room %d.", ch->in_room);
    return_val = ReturnValue::eFAILURE;
  }

  // this may cause problems with leader being ambushed, dying, and group not moving
  // but we have to be careful in case leader was a mob (no longer valid memory)
  if (SOMEONE_DIED(return_val) || !isSet(return_val, ReturnValue::eSUCCESS))
  {
    /*
     dc_sprintf(tmp, "%s group failed to follow. (died: %d ret: %d)",
     qPrintable(ch->name()), SOMEONE_DIED(return_val), return_val);
     dc_->logentry(tmp, OVERSEER, DC::LogChannel::LOG_BUG);
     */
    return return_val;
  }

  if (ch->followers && !isSet(ch->combat, COMBAT_FLEEING))
  {
    for (k = ch->followers; k; k = next_dude)
    { // no following a fleer
      next_dude = k->next;
      if ((was_in == k->follower->in_room) && ((is_retreat && GET_POS(k->follower) > position_t::RESTING) || (GET_POS(k->follower) >= position_t::STANDING)))
      {
        if (IS_AFFECTED(k->follower, AFF_NO_FLEE))
        {
          if (k->follower->affected_by_spell(SPELL_IRON_ROOTS))
            k->follower->sendln("The roots bracing your legs make it impossible to run!");
          else
            k->follower->sendln("Your legs are too tired for running away!");
          continue; // keep going through the groupies
        }
        if (is_retreat && k->follower->fighting && (ch->dc_->number(1, 100) < 4) && k->follower->fighting->isNonPlayer())
        {

          act_to_victim("$n notices your intent and moves quickly to block your retreat!", k->follower->fighting, nullptr, k->follower, 0);
          act_to_room("$n notices $N's intent and moves quickly to block $S retreat!", k->follower->fighting, nullptr, k->follower, NOTVICT);
          WAIT_STATE(k->follower, 8);
          continue;
        }
        if (CAN_SEE(k->follower, ch))
          dc_sprintf(tmp, "You follow %s.\r\n\r\n", qPrintable(ch->shortdesc_or_name()));
        else
          dc_strcpy(tmp, "You follow someone.\r\n\r\n");
        k->follower->send(tmp);
        // do_move(k->follower, "", cmd + 1);
        QString tempcommand;
        dc_strcpy(tempcommand, dirs[dir]);
        if (k->follower->fighting)
          stop_fighting(k->follower);
        k->follower->command_interpreter(tempcommand);
      }
      else
      {
        /*
         dc_sprintf(tmp, "%s attempted to follow %s but failed. (was_in:%d fol->in_room:%d pos: %d ret: %d",
         qPrintable(k->follower->name()), qPrintable(ch->name()), was_in, k->follower->in_room,
         GET_POS(k->follower), is_retreat);
         dc_->logentry(tmp, OVERSEER, DC::LogChannel::LOG_BUG);
         */
      }
    }
  }

  if (was_in != ch->in_room && !IS_AFFECTED(ch, AFF_SNEAK))
    return_val = ambush(ch);

  if (isSet(return_val, ReturnValue::eCH_DIED))
    return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;

  return ReturnValue::eSUCCESS;
}

//   Returns :
//   1 : If success.
//   0 : If fail
//  -1 : If dead.
command_return_t do_move(CharacterPtr ch, const QString argument, cmd_t cmd)
{
  return attempt_move(ch, cmd);
}

command_return_t do_leave(CharacterPtr ch, QString arguement, cmd_t cmd)
{
  ObjectPtr k;
  QString buf;
  qint32 retval;

  for (k = dc_->object_list; k; k = k->next)
  {
    if (k->isPortal())
    {
      if (k->getPortalType() == Object::portal_types_t::Permanent || k->getPortalType() == Object::portal_types_t::Temp)
      {
        if (k->in_room != DC::NOWHERE && !k->hasPortalFlagNoLeave())
        {
          if (k->getPortalDestinationRoom() == dc_->world[ch->in_room].number || k->obj_flags.value[2] == dc_->world[ch->in_room].zone)
          {
            ch->sendln("You exit the area.");
            act_to_room("$n has left the area.", ch, 0, 0, INVIS_NULL | STAYHIDE);
            retval = move_char(ch, real_room(dc_->world[k->in_room].number));
            if (!isSet(retval, ReturnValue::eSUCCESS))
            {
              ch->sendln("You attempt to leave, but the door slams in your face!");
              act_to_room("$n attempts to leave, but can't!", ch, 0, 0, INVIS_NULL | STAYHIDE);
              return ReturnValue::eFAILURE;
            }
            do_look(ch, "");
            dc_sprintf(buf, "%s walks out of %s.", qPrintable(ch->name()), k->short_description);
            act_to_room(buf, ch, 0, 0, INVIS_NULL | STAYHIDE);
            return ambush(ch);
          }
        }
      }
    }
  }

  ch->sendln("There are no exits around!");

  return ReturnValue::eFAILURE;
}

command_return_t do_enter(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  qint32 retval;

  CharacterPtr sesame;
  ObjectPtr portal = {};

  if ((ch->in_room != DC::NOWHERE) || (ch->in_room))
  {
    one_argument(argument, buf);
  }

  if (buf.isEmpty())
  {
    ch->sendln("Enter what?");
    return ReturnValue::eFAILURE;
  }

  if ((portal = get_obj_in_list_vis(ch, buf, dc_->world[ch->in_room].contents)) == nullptr)
  {
    ch->sendln("Nothing here by that name.");
    return ReturnValue::eFAILURE;
  }

  if (!portal->isPortal())
  {
    ch->sendln("You can't enter that.");
    return ReturnValue::eFAILURE;
  }

  if (real_room(portal->getPortalDestinationRoom()) == DC::NOWHERE)
  {
    dc_sprintf(buf, "Error in do_enter(), value 0 on object %d < 0", portal->item_number);
    dc_->logentry(buf, OVERSEER, DC::LogChannel::LOG_BUG);
    ch->sendln("You can't enter that.");
    return ReturnValue::eFAILURE;
  }

  if (dc_->world[real_room(portal->getPortalDestinationRoom())].sector_type == SECT_UNDERWATER && !(ch->affected_by_spell(SPELL_WATER_BREATHING) || IS_AFFECTED(ch, AFF_WATER_BREATHING)))
  {
    ch->sendln("You bravely attempt to plunge through the portal - let's hope you have gills!");
    return ReturnValue::eSUCCESS;
  }

  if (ch->getLevel() > IMMORTAL && ch->getLevel() < DEITY && isSet(dc_->world[real_room(portal->getPortalDestinationRoom())].room_flags, CLAN_ROOM))
  {
    ch->sendln("You may not enter a clanhall at your level.");
    return ReturnValue::eFAILURE;
  }

  if (ch->isNonPlayer() && ch->master && dc_->mob_index[ch->mobdata->nr].vnum() == 8)
  {
    sesame = ch->master;
    if (isSet(dc_->world[real_room(portal->obj_flags.value[0])].room_flags, CLAN_ROOM))
    {
      // Is golem's master not a member of this clan
      if (others_clan_room(sesame, &dc_->world[real_room(portal->obj_flags.value[0])]) == true)
      {
        ch->sendln("Your master is not from that clan.");
        act_to_room("$n finds $mself unable to enter!", ch, 0, 0, 0);
        do_say(ch, "I may not enter.");
        return ReturnValue::eFAILURE;
      }
    }
  }
  else
  {
    sesame = ch;
  }

  // should probably just combine this with 'if' below it, but i'm lazy
  if (isSet(portal->obj_flags.value[3], Object::portal_flags_t::No_Enter))
  {
    ch->sendln("The portal's destination rebels against you.");
    act_to_room("$n finds $mself unable to enter!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  // Thieves arent allowed through cleric portals
  if (ch->isPlayerObjectThief() && portal->isPortalTypePlayer())
  {
    ch->sendln("Your attempt to transport stolen goods through planes of magic fails!");
    return ReturnValue::eFAILURE;
  }

  if (!ch->isNonPlayer() && (ch->isPlayerObjectThief() || ch->isPlayerGoldThief() || IS_AFFECTED(ch, AFF_CHAMPION)) && (isSet(dc_->world[real_room(portal->obj_flags.value[0])].room_flags, CLAN_ROOM) || (portal->obj_flags.value[0] >= 1900 && portal->obj_flags.value[0] <= 1999 && !portal->obj_flags.value[1])))
  {
    ch->sendln("The portal's destination rebels against you.");
    act_to_room("$n finds $mself unable to enter!", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (isexact("only", portal->name()) && !isexact(qPrintable(sesame->name()), portal->name()))
  {
    ch->sendln("The portal fades when you draw near, then shimmers as you withdraw.");
    return ReturnValue::eFAILURE;
  }

  switch (portal->obj_flags.value[1])
  {
  case 0:
    ch->sendln("You reach out tentatively and touch the portal...");
    act_to_room("$n reaches out to the glowing dimensional portal....", ch, 0, 0, 0);
    break;
  case 1:
  case 2:
    dc_sprintf(buf, "You take a bold step towards %s.\r\n", portal->short_description);
    ch->send(buf);
    dc_sprintf(buf, "%s boldly walks toward %s and disappears.", qPrintable(ch->name()), portal->short_description);
    act_to_room(buf, ch, 0, 0, INVIS_NULL | STAYHIDE);
    break;
  case 3:
    ch->sendln("You cannot enter that.");
    return ReturnValue::eFAILURE;
  default:
    dc_sprintf(buf, "Error in do_enter(), value 1 on object %d returned default case", portal->item_number);
    dc_->logentry(buf, OVERSEER, DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  }

  retval = move_char(ch, real_room(portal->obj_flags.value[0]));

  if (SOMEONE_DIED(retval))
    return retval;

  if (!isSet(retval, ReturnValue::eSUCCESS))
  {
    ch->sendln("You recoil in pain as the portal slams shut!");
    act_to_room("$n recoils in pain as the portal slams shut!", ch, 0, 0, 0);
  }

  switch (portal->obj_flags.value[1])
  {
  case 0:
    do_look(ch, "");
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    ch->sendln("\r\nYou are momentarily dazed from the dimensional shift.");
    act_to_room("The portal glows brighter for a second as $n appears beside you.", ch, 0, 0, 0);
    break;
  case 1:
  case 2:
    dc_sprintf(buf, "%s has entered %s.", qPrintable(ch->name()), portal->short_description);
    act_to_room(buf, ch, 0, 0, STAYHIDE);
    do_look(ch, "");
    break;
  case 3:
    break;
  default:
    break;
  }

  return ambush(ch);
}

qint32 move_char(CharacterPtr ch, qint32 dest, bool stop_all_fighting)
{
  if (!ch)
  {
    dc_->logentry(u"Error in move_char(), nullptr character"_s, OVERSEER, DC::LogChannel::LOG_BUG);
    return ReturnValue::eINTERNAL_ERROR;
  }

  qint32 origination = ch->in_room;

  if (ch->in_room != DC::NOWHERE)
  {
    // Couldn't move character from the room
    if (char_from_room(ch, stop_all_fighting) == 0)
    {
      dc_->logentry(u"Error in move_char(), character not DC::NOWHERE, but couldn't be moved."_s,
                    OVERSEER, DC::LogChannel::LOG_BUG);
      return ReturnValue::eINTERNAL_ERROR;
    }
  }

  // Couldn't move character to new room
  if (char_to_room(ch, dest, stop_all_fighting) == 0)
  {
    // Now we have real problems
    if (char_to_room(ch, origination) == 0)
    {
      qFatal("%s", qUtf8Printable(u"Error in move_char(), character stuck in DC::NOWHERE: %1.\n"_s.arg(qPrintable(ch->name()))));
    }
    dc_->logf(OVERSEER, DC::LogChannel::LOG_BUG, "Error in move_char(), could not move %s to %d.", qPrintable(ch->name()), dc_->world[dest].number);
    return ReturnValue::eINTERNAL_ERROR;
  }

  // At this point, the character is happily in the new room
  return ReturnValue::eSUCCESS;
}

command_return_t do_climb(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  ObjectPtr obj = {};

  one_argument(argument, buf);

  if (buf.isEmpty())
  {
    ch->sendln("Climb what?");
    return ReturnValue::eSUCCESS;
  }

  if (!(obj = get_obj_in_list_vis(ch, buf, dc_->world[ch->in_room].contents)))
  {
    ch->sendln("Climb what?");
    return ReturnValue::eSUCCESS;
  }

  if (obj->obj_flags.type_flag != ITEM_CLIMBABLE)
  {
    ch->sendln("You can't climb that.");
    return ReturnValue::eSUCCESS;
  }

  qint32 dest = obj->obj_flags.value[0];

  if (real_room(dest) < 0)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Error in do_climb(), illegal destination in object %d.", dc_->obj_index[obj->item_number].vnum());
    ch->sendln("You can't climb that.");
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  act_to_room("$n carefully climbs $p.", ch, obj, 0, INVIS_NULL);
  dc_sprintf(buf, "You carefully climb %s.\r\n", qPrintable(obj->short_description()));
  ch->send(buf);
  qint32 retval = move_char(ch, real_room(dest));

  if (SOMEONE_DIED(retval))
    return retval;

  act_to_room("$n carefully climbs $p.", ch, obj, 0, INVIS_NULL);
  do_look(ch, "");

  return ReturnValue::eSUCCESS;
}

// The End

qint32 ambush(CharacterPtr ch)
{
  CharacterPtr i, next_i;
  qint32 retval;

  for (i = dc_->world[ch->in_room].people_; i; i = next_i)
  {
    next_i = i->next_in_room;

    if (i == ch || i->ambush.isEmpty() || !CAN_SEE(i, ch) || i->fighting)
      continue;

    if (GET_POS(i) <= position_t::RESTING ||
        GET_POS(i) == position_t::FIGHTING ||
        IS_AFFECTED(i, AFF_PARALYSIS) ||
        (isSet(dc_->world[i->in_room].room_flags, SAFE) &&
         !IS_AFFECTED(ch, AFF_CANTQUIT)))
      continue;
    if (!i->isNonPlayer() && !i->conn_) // don't work if I'm linkdead
      continue;
    if (isexact(i->ambush, qPrintable(ch->name())))
    {

      if (!i->canPerform(SKILL_AMBUSH))
      {
        continue;
      }

      if (IS_AFFECTED(ch, AFF_ALERT))
      {
        i->sendln("Your target is far too alert to accomplish an ambush!");
        continue;
      }

      if (!charge_moves(ch, SKILL_AMBUSH))
        return ReturnValue::eSUCCESS;

      if (skill_success(i, ch, SKILL_AMBUSH))
      {
        //         act_to_room("$n ambushes $N in a brilliant surprise attack!", i, 0, ch,  NOTVICT);
        //         act_to_victim("$n ambushes you as you enter the room!", i, 0, ch,  0);
        //         act_to_character("You ambush $N with a brilliant surprise attack!", i, 0, ch,  0);
        retval = damage(i, ch, i->getLevel() * 10, TYPE_HIT, SKILL_AMBUSH);
        if (isSet(retval, ReturnValue::eVICT_DIED))
          return (ReturnValue::eSUCCESS | ReturnValue::eCH_DIED); // ch = damage vict
        if (isSet(retval, ReturnValue::eCH_DIED))
          return (ReturnValue::eSUCCESS); // doesn't matter, but don't lag vict
        if (!i->isNonPlayer() && isSet(i->player->toggles, Player::PLR_WIMPY))
          WAIT_STATE(i, DC::PULSE_VIOLENCE * 3);
        else
          WAIT_STATE(i, DC::PULSE_VIOLENCE * 2);
        WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1);
      }
      // we continue instead of breaking in case there are any OTHER rangers in the room
    }
  }
  return ReturnValue::eSUCCESS;
}