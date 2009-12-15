/************************************************************************
| $Id: move.cpp,v 1.94 2009/12/15 10:52:38 kkoons Exp $
| move.C
| Movement commands and stuff.
*************************************************************************
*  Revision History                                                     *
*  11/09/2003   Onager   Changed do_unstable() and do_fall() to call    *
*                        noncombat_damage() for damage                  *
*************************************************************************
*/
#include <character.h>
#include <affect.h>
#include <room.h>
#include <levels.h>
#include <utility.h>
#include <player.h>
#include <fight.h>
#include <mobile.h>
#include <interp.h>
#include <spells.h>
#include <handler.h>
#include <db.h>
#include <obj.h>
#include <connect.h>
#include <act.h>
#include <race.h> // RACE_FISH
#include <clan.h> // clan_room_data
#include <string.h>
#include <returnvals.h>
#include <game_portal.h>
#include <innate.h>
#include <weather.h>

#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

extern struct obj_data *object_list;
extern struct index_data *obj_index;
extern struct index_data *mob_index;
extern struct zone_data *zone_table;
extern CWorld world;
 
extern char *dirs[];
extern int movement_loss[];

int ambush(CHAR_DATA *ch); // class/cl_ranger.C


int move_player(char_data * ch, int room)
{
  int retval;
  
  retval = move_char(ch, room);

  if(!IS_SET(retval, eSUCCESS)) {
    retval = move_char(ch, real_room(START_ROOM));
    if(!IS_SET(retval, eSUCCESS))
    log("Error in move_player(), Failure moving ch to start room. move_player_home_nofail",
    IMMORTAL, LOG_BUG);
    }

  return retval;
}


void move_player_home(CHAR_DATA *victim)
{
  int was_in = victim->in_room;
  int found = 0;
  struct clan_data * clan = NULL;
  struct clan_room_data * room = NULL;
  struct clan_data * get_clan(CHAR_DATA *ch);
  
  // check for homes that don't exist
  if(real_room(GET_HOME(victim) < 1))
    GET_HOME(victim) = START_ROOM;

  // next four lines ping-pong people from meta to tavern to help lessen spam
  if (world[was_in].number == GET_HOME(victim) && GET_HOME(victim) == START_ROOM)
    move_player(victim, real_room(SECOND_START_ROOM));
  else if (world[was_in].number == GET_HOME(victim) || IS_AFFECTED(victim, AFF_CHAMPION))
    move_player(victim, real_room(START_ROOM));
  // recalling into a clan room
  else if (!IS_SET(world[real_room(GET_HOME(victim))].room_flags, CLAN_ROOM))
    move_player(victim, real_room(GET_HOME(victim)));
  // Clanroom else
  else {
    if(!victim->clan || !(clan = get_clan(victim))) {
      send_to_char("The gods frown on you, and reset your home.\r\n", victim);
      GET_HOME(victim) = START_ROOM;
      move_player(victim, real_room(GET_HOME(victim)));
      }
    else {
      for(room = clan->rooms; room; room = room->next)
        if(room->room_number == GET_HOME(victim))
          found = 1;
        if(!found) {
          send_to_char("The gods frown on you, and reset your home.\r\n", victim);
          GET_HOME(victim) = START_ROOM;
          }
        move_player(victim, real_room(GET_HOME(victim)));
      }
    }
}


// Rewritten 9/1/96
// -Sadus
void record_track_data(CHAR_DATA *ch, int cmd)
{
  room_track_data * newScent;
  
  // Rangers outdoors leave no tracks
  if(world[ch->in_room].sector_type != SECT_INSIDE &&
     GET_CLASS(ch) == CLASS_RANGER)
    return;
    
  // If we just used a scent killer like catstink, don't leave tracks
  // on our next move
  if(ISSET(ch->affected_by, AFF_UTILITY)) {
    REMBIT(ch->affected_by, AFF_UTILITY);
    return;
  }

  if(world[ch->in_room].sector_type == SECT_WATER_SWIM ||
     world[ch->in_room].sector_type == SECT_WATER_NOSWIM )
    return;

#ifdef LEAK_CHECK
  newScent = (room_track_data *)calloc(1, sizeof(struct room_track_data));
#else
  newScent = (room_track_data *)dc_alloc(1, sizeof(struct room_track_data));
#endif
  newScent->direction = cmd;
  newScent->weight    = (int)ch->weight;
  newScent->race      = (int)ch->race;
  newScent->sex       = (int)ch->sex;
  newScent->condition = ((GET_HIT(ch) * 100) / (GET_MAX_HIT(ch) == 0 ? 100 : GET_MAX_HIT(ch)));
  newScent->next      = NULL; // just in case
  newScent->previous  = NULL; // just in case

  if(IS_NPC(ch))
    newScent->trackee = GET_NAME(ch);
  else
    newScent->trackee = str_hsh(GET_NAME(ch));

  world[ch->in_room].AddTrackItem(newScent);
  
  return;
}


// Removed this due to it being a funky cold medina. - Nocturnal 09/28/05
//void do_muddy(CHAR_DATA *ch)
//{
//   short chance = number(0,30);
//
//   if(IS_NPC(ch) || IS_AFFECTED(ch, AFF_FLYING) || GET_LEVEL(ch) >= IMMORTAL || IS_AFFECTED(ch, AFF_FREEFLOAT)) {
//     ; //poop on a stick!
//   } else if(GET_DEX(ch) > chance) {
//      act("You barely avoid slipping in the mud.", ch, 0, 0, TO_CHAR, 0);
//   } else {
//      act("$n slips on the muddy terrain and goes down.", ch, 0, 0, TO_ROOM, 0);
//      act("Your feet slide out from underneath you in the mud.", ch, 0, 0, TO_CHAR, 0);
//      GET_POS(ch) = POSITION_SITTING;
//   }
//}


int do_unstable(CHAR_DATA *ch) 
{
  char death_log[MAX_STRING_LENGTH];
  int retval;

  if(IS_NPC(ch))
    return eFAILURE;

  short chance = number(0, 30);

  if(IS_AFFECTED(ch, AFF_FLYING) || IS_AFFECTED(ch, AFF_FREEFLOAT) ||
    GET_LEVEL(ch) >= IMMORTAL)
    return eFAILURE;

  if(GET_DEX(ch) > chance) {
    act("You dextrously maintain your footing.", ch, 0, 0, TO_CHAR, 0);
    return eFAILURE;
    }

  act("$n looses his footing on the unstable ground.", ch, 0, 0, TO_ROOM, 0);
  act("You fall flat on your ass.", ch, 0, 0, TO_CHAR, 0);
  GET_POS(ch) = POSITION_SITTING;
  
  sprintf(death_log, "%s slipped to death in room %d.",
    GET_NAME(ch), world[ch->in_room].number);
  retval = noncombat_damage(ch, GET_MAX_HIT(ch) / 12,
    "You feel your back snap painfully and all goes dark...",
    "$n's frail body snaps in half as $e is buffeted about the room.",
    death_log, KILL_FALL);

  if (SOMEONE_DIED(retval))
    return eSUCCESS|eCH_DIED;
  else
    return eSUCCESS;
}


int do_fall(CHAR_DATA *ch, short dir)
{
  int target;
  char damage[MAX_STRING_LENGTH];
  int retval;
  int dam = number(50,100);

  if(IS_AFFECTED(ch, AFF_FLYING))
    return eFAILURE;

  if (GET_LEVEL(ch) == 50)
    dam = number(100,200);
  
  if (GET_MAX_HIT(ch) > 2000)
    dam = number(400,500);
  else if (GET_MAX_HIT(ch) > 1000)
    dam = number(200,400);

  if(GET_LEVEL(ch) >= IMMORTAL) {
    return eFAILURE;
    }

  if(IS_AFFECTED(ch, AFF_FREEFLOAT)) {
    dam = 0;
    send_to_char("Your freefloating magics reduce your fall to a safer speed.\n\r",ch);
  }

  // Don't effect mobs
  if(IS_NPC(ch)) {
    act("$n clings to the terrain around $m and avoids falling.", ch, 0, 0, TO_ROOM, 0);
    return eFAILURE;
    }

  if(!CAN_GO(ch, dir))
    return eFAILURE;

  target = world[ch->in_room].dir_option[dir]->to_room;

  if(IS_SET(world[target].room_flags, IMP_ONLY) && GET_LEVEL(ch) < IMP)
    return eFAILURE;

  if (IS_SET(world[target].room_flags, TUNNEL)) {
    int ppl = 0;
    CHAR_DATA *k;
    for (k = world[target].people; k; k = k->next_in_room)
      if (!IS_NPC(k))
        ppl++;
    if (ppl > 2) {
      send_to_char("There isn't enough space for you to follow.\r\n",ch);
      return eFAILURE;
      }
    }

  if (IS_SET(world[target].room_flags, PRIVATE)) {
    int ppl = 0;
    CHAR_DATA *k;
    for (k = world[target].people; k; k = k->next_in_room)
      if (!IS_NPC(k))
        ppl++;
    if (ppl > 4) {
      send_to_char("There isn't enough space for you to follow.\r\n",ch);
      return eFAILURE;
      }
    }

  sprintf(damage,"%s falls from %d and sustains %d damage.",
    GET_NAME(ch), world[ch->in_room].number, dam);
  log(damage, IMMORTAL, LOG_MORTAL);

  switch(dir) {
    case 0:
      act("$n rolls out to the north.", ch, 0, 0, TO_ROOM, 0);
      send_to_char("You tumble to the north...\n\r", ch);
      break;
    case 1:
      act("$n rolls out to the east.", ch, 0, 0, TO_ROOM, 0);
      send_to_char("You tumble to the east...\n\r", ch);
      break;
    case 2:
      act("$n rolls out to the south.", ch, 0, 0, TO_ROOM, 0);
      send_to_char("You tumble to the south...\n\r", ch);
      break;
    case 3:
      act("$n rolls out to the west.", ch, 0, 0, TO_ROOM, 0);
      send_to_char("You tumble to the west...\n\r", ch);
      break;
    case 4:
      act("$n is launched upwards.", ch, 0, 0, TO_ROOM, 0);
      send_to_char("You are launched into the air...\n\r", ch);
      break;
    case 5:
      act("$n falls through the room.", ch, 0, 0, TO_ROOM, 0);
      send_to_char("You fall...\n\r", ch);
      break;
    default:
      log("Default hit in do_fall", IMMORTAL, LOG_MORTAL);
      break;
    }

  retval = move_char(ch, target);

  if(!IS_SET(retval, eSUCCESS)) {
    send_to_char("You are miraculously upheld by divine powers!\n\r", ch);
    return retval;
    }

  do_look(ch, "\0", 15);

  sprintf(damage,"%s's fall from %d was lethal and it killed them.",
    GET_NAME(ch), world[ch->in_room].number);
  retval = noncombat_damage(ch, dam,
    "Luckily the ground breaks your fall.\n\r",
    "$n plummets into the room and hits the ground with a wet-sounding splat!",
    damage, KILL_FALL);

  if (SOMEONE_DIED(retval))
    return eSUCCESS;

  if (!SOMEONE_DIED(retval)) {
    act("$n plummets into the room and hits the floor HARD.", ch, 0, 0, TO_ROOM, 0);
    }

  if((IS_SET(world[ch->in_room].room_flags, FALL_DOWN) && (dir = 5)) ||
    (IS_SET(world[ch->in_room].room_flags, FALL_UP) && (dir = 4)) ||
    (IS_SET(world[ch->in_room].room_flags, FALL_EAST) && (dir = 1)) ||
    (IS_SET(world[ch->in_room].room_flags, FALL_WEST) && (dir = 3)) ||
    (IS_SET(world[ch->in_room].room_flags, FALL_SOUTH) && (dir = 2)) ||
    (IS_SET(world[ch->in_room].room_flags, FALL_NORTH) && (dir = 0))) {
    return do_fall(ch, dir);
    }

  // We lived
  return eSUCCESS;
}


// Assumes
// 1. No master and no followers.
// 2. That the direction exists.
int do_simple_move(CHAR_DATA *ch, int cmd, int following)
{
  char tmp[80];
  int dir;
  int was_in;
  int need_movement, learned, mvinroom = 0, mvtoroom = 0;
  int retval;
  struct obj_data *obj;
  bool has_boat;

/*
  I think this is taken care of in the command interpreter.  Disabling it for now.
  -pir 7/25/01
  // Check for special routines (North is 1)
  retval = special(ch, cmd+1, "");
  if(IS_SET(retval, eSUCCESS) || IS_SET(retval, eCH_DIED))
    return retval;
*/

  if (IS_AFFECTED(ch, AFF_FLYING))
    need_movement = 1;
  else {
    if((learned = has_skill(ch, SKILL_NATURES_LORE))) {
    
      if(learned > 90) {
        if(world[ch->in_room].sector_type == SECT_UNDERWATER) {
          mvinroom = movement_loss[world[ch->in_room].sector_type] / 2;
          skill_increase_check(ch, SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
          }
        if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_UNDERWATER)
          mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
        }
        
      if(learned > 80) {
        if(world[ch->in_room].sector_type == SECT_MOUNTAIN) {
          mvinroom = movement_loss[world[ch->in_room].sector_type] / 2;
          if(learned < 91)
            skill_increase_check(ch, SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
          }
        if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_MOUNTAIN)
          mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
        }
        
      if(learned > 70) {
        if(world[ch->in_room].sector_type == SECT_ARCTIC) {
          mvinroom = movement_loss[world[ch->in_room].sector_type] / 2;
          if(learned < 81)
            skill_increase_check(ch, SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
          }
        if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_ARCTIC)
          mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
        }
        
      if(learned > 60) {
        if(world[ch->in_room].sector_type == SECT_HILLS) {
          mvinroom = movement_loss[world[ch->in_room].sector_type] / 2;
          if(learned < 71)
            skill_increase_check(ch, SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
          }
        if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_HILLS)
          mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
        }
        
      if(learned > 50) {
        if(world[ch->in_room].sector_type == SECT_WATER_SWIM) {
          mvinroom = movement_loss[world[ch->in_room].sector_type] / 2;
          if(learned < 61)
            skill_increase_check(ch, SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
          }
        if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_WATER_SWIM)
          mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
        }
        
      if(learned > 40) {
        if(world[ch->in_room].sector_type == SECT_FROZEN_TUNDRA) {
          mvinroom = movement_loss[world[ch->in_room].sector_type] / 2;
          if(learned < 51)
            skill_increase_check(ch, SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
          }
        if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_FROZEN_TUNDRA)
          mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
        }
        
      if(learned > 30) {
        if(world[ch->in_room].sector_type == SECT_DESERT) {
          mvinroom = movement_loss[world[ch->in_room].sector_type] / 2;
          if(learned < 41)
            skill_increase_check(ch, SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
          }
        if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_DESERT)
          mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
        }
        
      if(learned > 20) {
        if(world[ch->in_room].sector_type == SECT_BEACH) {
          mvinroom = movement_loss[world[ch->in_room].sector_type] / 2;
          if(learned < 31)
            skill_increase_check(ch, SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
          }
        if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_BEACH)
          mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
        }
        
      if(learned > 10) {
        if(world[ch->in_room].sector_type == SECT_FOREST) {
          mvinroom = movement_loss[world[ch->in_room].sector_type] / 2;
          if(learned < 21)
            skill_increase_check(ch, SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
          }
        if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_FIELD)
          mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
        }
        
      if(learned > 0) {
        if(world[ch->in_room].sector_type == SECT_FIELD) {
          mvinroom = movement_loss[world[ch->in_room].sector_type] / 2;
          if(learned < 11)
            skill_increase_check(ch, SKILL_NATURES_LORE, learned, SKILL_INCREASE_MEDIUM);
          }
        if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_FOREST)
          mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type] / 2;
        }
        
      if(!mvinroom)
        mvinroom = movement_loss[world[ch->in_room].sector_type];
      if(!mvtoroom)
        mvtoroom = movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type];
      need_movement = (mvinroom + mvtoroom) / 2;
      }
    else {
      need_movement = (movement_loss[world[ch->in_room].sector_type] +
        movement_loss[world[world[ch->in_room].dir_option[cmd]->to_room].sector_type]) / 2;
      }

    // if I'm trying to go "up" into a "fall down" room, etc.
    // it's OK to go east into a "fall north" room though
    // not ok, if room we're going to is AIR though
    if( !IS_AFFECTED(ch, AFF_FLYING) &&
      (( cmd == 0 && IS_SET( world[world[ch->in_room].dir_option[0]->to_room].room_flags, FALL_SOUTH ) ) ||
      ( cmd == 1 && IS_SET( world[world[ch->in_room].dir_option[1]->to_room].room_flags, FALL_WEST ) ) ||
      ( cmd == 2 && IS_SET( world[world[ch->in_room].dir_option[2]->to_room].room_flags, FALL_NORTH ) ) ||
      ( cmd == 3 && IS_SET( world[world[ch->in_room].dir_option[3]->to_room].room_flags, FALL_EAST ) ) ||
      ( cmd == 4 && IS_SET( world[world[ch->in_room].dir_option[4]->to_room].room_flags, FALL_DOWN ) ) ||
      ( cmd == 5 && IS_SET( world[world[ch->in_room].dir_option[5]->to_room].room_flags, FALL_UP ) ) ||
      world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_AIR)) {
      send_to_char("You would need to fly to go there!\n\r", ch);
      return eFAILURE;
      }

    // fly doesn't work over water
    if ((world[ch->in_room].sector_type == SECT_WATER_NOSWIM) ||
      (world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_WATER_NOSWIM)) {
      has_boat = FALSE;
      // See if char is carrying a boat
      for (obj = ch->carrying; obj; obj = obj->next_content)
        if (obj->obj_flags.type_flag == ITEM_BOAT)
          has_boat = TRUE;
      // See if char is wearing a boat (boat ring, etc)
      if(!has_boat)
        for(int x = 0; x <= MAX_WEAR; x++)
          if(ch->equipment[x])
            if(ch->equipment[x]->obj_flags.type_flag == ITEM_BOAT)
              has_boat = TRUE;
      if (!has_boat && !IS_AFFECTED(ch, AFF_FLYING) && GET_LEVEL(ch) < IMMORTAL && 
        GET_RACE(ch) != RACE_FISH && GET_RACE(ch) != RACE_SLIME && !IS_AFFECTED(ch, AFF_FREEFLOAT)) {
        send_to_char("You need a boat to go there.\n\r", ch);
        return eFAILURE;
        }
      }

    if (world[world[ch->in_room].dir_option[cmd]->to_room].sector_type != SECT_WATER_NOSWIM &&
      world[world[ch->in_room].dir_option[cmd]->to_room].sector_type != SECT_WATER_SWIM &&
      world[world[ch->in_room].dir_option[cmd]->to_room].sector_type != SECT_UNDERWATER) {
      // It's NOT a water room and we don't have fly
      if(GET_RACE(ch) == RACE_FISH) {
        send_to_char("You can't swim around outside water without being able to fly!\r\n", ch);
        act("$n flops around in a futile attempt to move out of water.", ch, 0, 0, TO_ROOM, INVIS_NULL);
        return eFAILURE;
        }
      }
    } // else if !FLYING

  if (IS_SET(world[world[ch->in_room].dir_option[cmd]->to_room].room_flags, IMP_ONLY) &&
    GET_LEVEL(ch) < IMP) {
    send_to_char("No.\r\n", ch);
    return eFAILURE;
    }

  struct room_data *rm = &(world[world[ch->in_room].dir_option[cmd]->to_room]);

  if (rm->sector_type != world[ch->in_room].sector_type &&
    ch->desc && ch->desc->original &&
    ch->desc->original->level <= MAX_MORTAL) {
    int s2 = rm->sector_type, s1 = world[ch->in_room].sector_type;
    if ((s1 == SECT_CITY && (s2 != SECT_INSIDE && s2 != SECT_PAVED_ROAD)) ||
      (s1 == SECT_INSIDE && (s2 != SECT_CITY && s2 != SECT_PAVED_ROAD)) ||
      (s1 == SECT_PAVED_ROAD && (s2 != SECT_INSIDE && s2 != SECT_CITY)) ||
      (s1 == SECT_FIELD && (s2 != SECT_HILLS && s2 != SECT_MOUNTAIN)) ||
      (s1 == SECT_HILLS && (s2 != SECT_MOUNTAIN && s2 != SECT_FIELD)) ||
      (s1 == SECT_MOUNTAIN && (s2 != SECT_HILLS && s2 != SECT_FIELD)) ||
      (s1 == SECT_WATER_NOSWIM && (s2 != SECT_UNDERWATER && s2 != SECT_WATER_SWIM)) ||
      (s1 == SECT_WATER_SWIM && (s2 != SECT_UNDERWATER && s2 != SECT_WATER_NOSWIM)) ||
      (s1 == SECT_UNDERWATER && (s2 != SECT_WATER_NOSWIM && s2 != SECT_WATER_SWIM)) ||
      (s1 == SECT_BEACH && (s2 != SECT_DESERT)) ||
      (s1 == SECT_DESERT && (s2 != SECT_BEACH)) ||
      (s1 == SECT_FROZEN_TUNDRA && (s2 != SECT_ARCTIC)) ||
      (s1 == SECT_ARCTIC && (s2 != SECT_FROZEN_TUNDRA)) ||
      (s1 == SECT_AIR) ||
      (s1 == SECT_SWAMP)) {
      send_to_char("The ghost evaporates as you leave its habitat.\r\n",ch);
      do_return(ch, "", 0);
      //extract_char(ch,TRUE);
      return eSUCCESS|eCH_DIED;
      }
    }

  if (IS_SET(rm->room_flags, TUNNEL)) {
    int ppl = 0;
    CHAR_DATA *k;
    for (k = rm->people; k; k = k->next_in_room)
      if (!IS_NPC(k))
        ppl++;
    if (ppl > 2) {
      send_to_char("There's no room.\r\n",ch);
      return eFAILURE;
      }
    }

  if (GET_LEVEL(ch) < IMMORTAL) {
    bool classRestrictions = FALSE;
    // Determine if any class restrictions are in place
    for (int c_class=1; c_class < CLASS_MAX; c_class++) {
      if (rm->allow_class[c_class] == TRUE) {
	classRestrictions = TRUE;
      }
    }

    if (classRestrictions) {
      if (rm->allow_class[GET_CLASS(ch)] != TRUE) {
	send_to_char("Your class is not allowed there.\r\n", ch);
	return eFAILURE;
      }
    }
  }

  if (IS_SET(rm->room_flags, PRIVATE)) {
    int ppl = 0;
    CHAR_DATA *k;
    for (k = rm->people; k; k = k->next_in_room)
      if (!IS_NPC(k))
        ppl++;
    if (ppl > 4) {
      send_to_char("There's no room.\r\n",ch);
      return eFAILURE;
      }
    }

  if(!IS_NPC(ch) &&
    world[world[ch->in_room].dir_option[cmd]->to_room].sector_type == SECT_UNDERWATER &&
    !(affected_by_spell(ch, SPELL_WATER_BREATHING) || IS_AFFECTED(ch, AFF_WATER_BREATHING))) {
    send_to_char("You feel air trying to explode from your lungs as you swim around.\r\n", ch);
    //send_to_char("Underwater?!\r\n", ch);
    //return eFAILURE;
    }

  // if I'm STAY_NO_TOWN, don't enter a ZONE_IS_TOWN zone no matter what
  if(IS_NPC(ch) &&
    ISSET(ch->mobdata->actflags, ACT_STAY_NO_TOWN) &&
    IS_SET(zone_table[world[world[ch->in_room].dir_option[cmd]->to_room].zone].zone_flags, ZONE_IS_TOWN))
    return eFAILURE;

  int a = 0;
  if ((a = has_skill(ch, SKILL_VIGOR)) && number(1,101) < a / 10)
    need_movement /= 2; // No skill improvement here. Too easy.
   
  if(GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
    if(!following)
      send_to_char("You are too exhausted.\n\r",ch);
    else
      send_to_char("You are too exhausted to follow.\n\r",ch);
    return eFAILURE;
    }

  // If they were hit with a lullaby, go ahead and clear it out
  if(ISSET(ch->affected_by, AFF_NO_FLEE)) {
    if(affected_by_spell(ch, SPELL_IRON_ROOTS)) {
      send_to_char("The roots bracing your legs keep you from moving!\r\n", ch);
      return eFAILURE;
      }
    else {
      REMBIT(ch->affected_by, AFF_NO_FLEE);
      send_to_char("The travel wakes you up some, and clears the drowsiness from your legs.\r\n", ch);
      }
    }

  if(GET_LEVEL(ch) < IMMORTAL && !IS_NPC(ch))
    GET_MOVE(ch) -= need_movement;

  // Everyone
  if (!IS_AFFECTED(ch, AFF_SNEAK) && !IS_AFFECTED(ch, AFF_FOREST_MELD)) {
    sprintf(tmp, "$n leaves %s.", dirs[cmd]);
    act(tmp, ch, 0, 0, TO_ROOM, INVIS_NULL);
    }
  // Sneaking
  else if (IS_AFFECTED(ch, AFF_SNEAK)) {
    char tmp[100];
    if (!skill_success(ch, NULL, SKILL_SNEAK)) {
      sprintf(tmp, "$n leaves %s.", dirs[cmd]);
      act(tmp, ch, 0, 0, TO_ROOM, INVIS_NULL|STAYHIDE);
      }
    else {
      sprintf(tmp, "$n sneaks %s.", dirs[cmd]);
      act(tmp, ch, 0, 0, TO_GROUP, INVIS_NULL);
      act(tmp, ch, 0, 0, TO_ROOM, GODS);
      }
    }
  // Forest melded
  else 
  {
    if(world[world[ch->in_room].dir_option[cmd]->to_room].sector_type != SECT_FOREST 
       && world[world[ch->in_room].dir_option[cmd]->to_room].sector_type != SECT_SWAMP) 
    {
      sprintf(tmp, "$n leaves %s.", dirs[cmd]);
      REMBIT(ch->affected_by, AFF_FOREST_MELD);
      send_to_char("You detach yourself from the forest.\r\n", ch);
      act(tmp, ch, 0, 0, TO_ROOM, INVIS_NULL);
    }
    else 
    {
      sprintf(tmp, "$n sneaks %s.", dirs[cmd]); 
      act(tmp, ch, 0, 0, TO_ROOM, GODS);
    }
  }

  was_in = ch->in_room;
  record_track_data(ch, cmd);

  retval = move_char(ch, world[was_in].dir_option[cmd]->to_room);

  if(IS_SET(retval, eSUCCESS) && IS_AFFECTED(ch, AFF_CRIPPLE)) {
    send_to_char("Your crippled body responds slowly.\n\r", ch);
    WAIT_STATE(ch, PULSE_VIOLENCE);
  }

  OBJ_DATA *tmp_obj;
  for(tmp_obj = world[ch->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
      send_to_char("The sounds around you fade to nothing as the silence takes hold...\n\r", ch);

  for(tmp_obj = world[was_in].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
    if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER)
      send_to_char("The noise around you returns as you leave the silenced area!\n\r", ch);

  if (!IS_SET(retval, eSUCCESS)) {
    send_to_char("You fail to move.\n\r", ch);
    return retval;
    }

  // Fighting
  if (ch->fighting) {
    char_data * chaser = ch->fighting;
    if (IS_NPC(ch)) {
      add_memory(ch, GET_NAME(chaser), 'f');
      remove_memory(ch, 'h');
      }
    if (IS_NPC(chaser) && chaser->hunting == 0) {
      if (GET_LEVEL(ch) - GET_LEVEL(chaser)/2 >= 0 || GET_LEVEL(ch) >= 50) {
        add_memory(chaser, GET_NAME(ch), 't');
        struct timer_data *timer;
        #ifdef LEAK_CHECK
          timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
        #else
          timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
        #endif
        timer->arg1 = (void*)chaser->hunting;
        timer->arg2 = (void*)chaser;
        timer->function = clear_hunt;
        timer->next = timer_list;
        timer_list = timer;
        timer->timeleft = (ch->level/4)*60;
        }
      }
    if (chaser->fighting == ch)
      stop_fighting(chaser);
    stop_fighting(ch);
    // This might be a bad idea...cause track calls move, which calls track, which...
    if (IS_NPC(chaser)) {
      retval = do_track(chaser, chaser->mobdata->hatred, 9);
      if (SOMEONE_DIED(retval))
        return retval;
      }
    }

  if (IS_AFFECTED(ch, AFF_SNEAK) ||
    (IS_AFFECTED(ch, AFF_FOREST_MELD) 
     && (world[ch->in_room].sector_type == SECT_FOREST
         || world[ch->in_room].sector_type == SECT_SWAMP)
    ))
  {
    act("$n sneaks into the room.", ch, 0, 0, TO_ROOM, GODS);
    act("$n sneaks into the room.", ch, 0, 0, TO_GROUP, INVIS_NULL);
  }
  else
    act("$n has arrived.", ch, 0,0, TO_ROOM, INVIS_NULL);

  do_look(ch, "\0", 15);

  // Elemental stuff goes HERE
  if (IS_NPC(ch))
  {
     int a = mob_index[ch->mobdata->nr].virt;
       // code a bit repeaty, but whatever ;)
     if (a == 88 && world[ch->in_room].sector_type == SECT_UNDERWATER)
     {
       act("Unable to survive underwater, $n returns to the elemental plane of fire.",ch, 0, 0, TO_ROOM, 0);
       extract_char(ch, TRUE);
       return eSUCCESS|eCH_DIED;
     }
     if (a == 89 && world[ch->in_room].sector_type == SECT_DESERT)
     {
       act("Unable to survive in the desert, $n returns to the elemental plane of water.",ch, 0, 0, TO_ROOM, 0);
       extract_char(ch, TRUE);
       return eSUCCESS|eCH_DIED;
     }
     if (a == 90 && world[ch->in_room].sector_type == SECT_SWAMP)
     {
       act("Unable to survive in the swamp, $n returns to the elemental plane of air.",ch, 0, 0, TO_ROOM, 0);
       extract_char(ch, TRUE);
       return eSUCCESS|eCH_DIED;
     }
     if (a == 91 && world[ch->in_room].sector_type == SECT_AIR)
     {
       act("Unable to survive in the air, $n returns to the elemental plane of earth.",ch, 0, 0, TO_ROOM, 0);
       extract_char(ch, TRUE);
       return eSUCCESS|eCH_DIED;
     }
  }
  // Elemental stuff ends HERE

  if((IS_SET(world[ch->in_room].room_flags, FALL_NORTH) && (dir = 0)) ||
    (IS_SET(world[ch->in_room].room_flags, FALL_DOWN) && (dir = 5)) ||
    (IS_SET(world[ch->in_room].room_flags, FALL_UP) && (dir = 4)) ||
    (IS_SET(world[ch->in_room].room_flags, FALL_EAST) && (dir = 1)) ||
    (IS_SET(world[ch->in_room].room_flags, FALL_WEST) && (dir = 3)) ||
    (IS_SET(world[ch->in_room].room_flags, FALL_SOUTH) && (dir = 2))) {
    retval = do_fall(ch, dir);
    if(SOMEONE_DIED(retval))
      return eSUCCESS|eCH_DIED;
    }

  if(IS_SET(world[ch->in_room].room_flags, UNSTABLE)) {
    retval = do_unstable(ch);
    if(SOMEONE_DIED(retval))
      return eSUCCESS|eCH_DIED;
    }

/*
  if(IS_SET(world[ch->in_room].sector_type, SECT_FIELD) &&
    weather_info.sky == SKY_HEAVY_RAIN && !number(0,19)) {
    do_muddy(ch);
    }
*/
  if ((GET_CLASS(ch) == CLASS_BARD && has_skill(ch, SKILL_SONG_HYPNOTIC_HARMONY)) || GET_CLASS(ch) == CLASS_RANGER)
  for (CHAR_DATA *tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
  {
    if (!IS_NPC(tmp_ch)) continue;
    if (IS_AFFECTED(tmp_ch, AFF_CHARM)) continue;
    if (tmp_ch->fighting) continue;
    if (IS_SET(tmp_ch->immune, ISR_CHARM)) continue;
    if (!number(0,1)) continue;

    if (GET_CLASS(ch) == CLASS_BARD && ISSET(tmp_ch->mobdata->actflags, ACT_BARDCHARM))
    {
	    act("$N looks at you expectantly, perhaps hoping for a song?", ch, NULL, tmp_ch, TO_CHAR, 0);
	    act("$N looks at $n expectantly, perhaps hoping for a song?", ch, NULL, tmp_ch, TO_ROOM, INVIS_NULL);
    } else if (GET_CLASS(ch) == CLASS_RANGER && ISSET(tmp_ch->mobdata->actflags, ACT_CHARM) && GET_LEVEL(ch) >= GET_LEVEL(tmp_ch) && CAN_SEE(tmp_ch, ch))
    {
	    act("$N moves submissively out of your way.", ch, NULL, tmp_ch, TO_CHAR, 0);
	    act("$N moves submissively out of $n's way.", ch, NULL, tmp_ch, TO_ROOM, INVIS_NULL);
    }
  }

  // let our mobs know they're here
  retval = mprog_entry_trigger( ch );
  if(SOMEONE_DIED(retval))
    return retval|eSUCCESS;
  retval = mprog_greet_trigger( ch );
  if(SOMEONE_DIED(retval))
    return retval|eSUCCESS;
  retval = oprog_greet_trigger( ch );
  if(SOMEONE_DIED(retval))
    return retval|eSUCCESS;



  return eSUCCESS;
}


int attempt_move(CHAR_DATA *ch, int cmd, int is_retreat = 0)
{
  char tmp[80];
  int  return_val;
  int  was_in = ch->in_room;
  struct follow_type *k, *next_dude;

  if(ch->brace_at)
  {
    csendf(ch, "You can't move and brace the %s at the same time!\r\n", fname(ch->brace_at->keyword));
    return eFAILURE;
  }

  --cmd;

  if(!world[ch->in_room].dir_option[cmd]) {
    send_to_char("You can't go that way.\n\r", ch);
    return eFAILURE;
    }

  if(IS_SET(EXIT(ch, cmd)->exit_info, EX_CLOSED)) {
    if(IS_SET(EXIT(ch, cmd)->exit_info, EX_HIDDEN))
      send_to_char("You can't go that way.\n\r", ch);
    else if (EXIT(ch, cmd)->keyword)
      csendf(ch, "The %s seems to be closed.\n\r", fname(EXIT(ch, cmd)->keyword));
    else 
      send_to_char("It seems to be closed.\n\r", ch);
    return eFAILURE;
    }

  if(EXIT(ch, cmd)->to_room == NOWHERE) {
    send_to_char("Alas, you can't go that way.\n\r", ch);
    return eFAILURE;
    }

  if(!ch->followers && !ch->master) {
    return_val = do_simple_move(ch, cmd, FALSE);
    if(SOMEONE_DIED(return_val) || !IS_SET(return_val, eSUCCESS))
      return return_val;
    if(!IS_AFFECTED(ch, AFF_SNEAK))
      return ambush(ch);
    return return_val;
    }

  if(IS_AFFECTED(ch, AFF_CHARM) && (ch->master) &&
    (ch->in_room == ch->master->in_room)) {
    send_to_char("You are unable to abandon your master.\n\r", ch);
    act("$n trembles as $s mind attempts to free itself from its magical bondage.",
      ch, 0, 0, TO_ROOM, 0);
    if (!IS_NPC(ch->master) && GET_CLASS(ch->master) == CLASS_BARD) {
      send_to_char("You struggle to maintain control.\r\n", ch->master);
/*
      if (GET_KI(ch->master) < 5) {
        add_memory(ch, GET_NAME(ch->master), 'h');
        stop_follower(ch, BROKE_CHARM);
        //add_memory(ch, GET_NAME(ch->master), 'h');
        do_say(ch, "Hey! You tricked me!", 9);
        send_to_char("You lose control.\r\n",ch->master);
        }
      else
        GET_KI(ch->master) -= 5;
*/
      }
    return eFAILURE;
    }

  return_val = do_simple_move(ch, cmd, TRUE);

  // this may cause problems with leader being ambushed, dying, and group not moving
  // but we have to be careful in case leader was a mob (no longer valid memory)
  if(SOMEONE_DIED(return_val) || !IS_SET(return_val, eSUCCESS)) {
/*
    sprintf(tmp, "%s group failed to follow. (died: %d ret: %d)",
      GET_NAME(ch), SOMEONE_DIED(return_val), return_val);
    log(tmp, OVERSEER, LOG_BUG);
*/
    return return_val;
    }

  if(ch->followers && !IS_SET(ch->combat, COMBAT_FLEEING)) {
    for(k = ch->followers; k; k = next_dude) { // no following a fleer
      next_dude = k->next;
      if((was_in == k->follower->in_room) &&
        ((is_retreat && GET_POS(k->follower) > POSITION_RESTING) ||
        (GET_POS(k->follower) >= POSITION_STANDING))) {
        if(IS_AFFECTED(k->follower, AFF_NO_FLEE)) {
          if(affected_by_spell(k->follower, SPELL_IRON_ROOTS))
            send_to_char("The roots bracing your legs make it impossible to run!\r\n", k->follower);
          else
            send_to_char("Your legs are too tired for running away!\r\n", k->follower);
          continue; //keep going through the groupies
          }
        if (is_retreat && k->follower->fighting && GET_LEVEL(k->follower->fighting) > 90 && 
		IS_NPC(k->follower->fighting))
        {
		int chance = 0;
	 	if(ISSET(k->follower->fighting->mobdata->actflags, ACT_BOSS))
			chance = GET_LEVEL(k->follower->fighting) / 2;
		else chance = GET_LEVEL(k->follower->fighting) / 8;
		act("$n notices your intent and moves quickly to block your retreat!", k->follower->fighting, NULL, k->follower,  TO_VICT, 0);
		act("$n notices $N's intent and moves quickly to block $S retreat!", k->follower->fighting, NULL, k->follower, TO_ROOM, NOTVICT);
		WAIT_STATE(k->follower, 8);
		continue;
        }
        if(CAN_SEE(k->follower, ch))
          sprintf(tmp, "You follow %s.\n\r\n\r", GET_SHORT(ch));
        else
          strcpy(tmp, "You follow someone.\n\r\n\r");
        send_to_char(tmp, k->follower); 
        //do_move(k->follower, "", cmd + 1);
        char tempcommand[32];
        strcpy(tempcommand, dirs[cmd]);
        if (k->follower->fighting)
          stop_fighting(k->follower);
        command_interpreter(k->follower, tempcommand);
        }
      else {
/*
        sprintf(tmp, "%s attempted to follow %s but failed. (was_in:%d fol->in_room:%d pos: %d ret: %d",
          GET_NAME(k->follower), GET_NAME(ch), was_in, k->follower->in_room,
          GET_POS(k->follower), is_retreat);
        log(tmp, OVERSEER, LOG_BUG);
*/
        }
      }
    }

  if(was_in != ch->in_room && !IS_AFFECTED(ch, AFF_SNEAK))
    return_val = ambush(ch);

  if(IS_SET(return_val, eCH_DIED))
    return eSUCCESS|eCH_DIED;

  return eSUCCESS;
}


//   Returns :
//   1 : If success.
//   0 : If fail
//  -1 : If dead.
int do_move(CHAR_DATA *ch, char *argument, int cmd)
{
  return attempt_move(ch, cmd);
}


int do_leave(struct char_data *ch, char *arguement, int cmd)
{
  struct obj_data *k;
  int origination;
  char buf[200];
  int retval;
  
  for(k = object_list; k; k = k->next) {
    if((k->obj_flags.type_flag == ITEM_PORTAL) &&
      (k->obj_flags.value[1] == 1 || (k->obj_flags.value[1] == 2)) &&
      (k->in_room > -1) &&
      !IS_SET(k->obj_flags.value[3], PORTAL_NO_LEAVE)) {
      if((k->obj_flags.value[0] == world[ch->in_room].number) ||
        (k->obj_flags.value[2] == world[ch->in_room].zone)) {
        send_to_char("You exit the area.\r\n", ch);
        act("$n has left the area.",  ch, 0, 0, TO_ROOM, INVIS_NULL|STAYHIDE);
        origination = ch->in_room;
        retval = move_char(ch, real_room(world[k->in_room].number));
        if(!IS_SET(retval, eSUCCESS)) {
          send_to_char("You attempt to leave, but the door slams in your face!\r\n", ch);
          act("$n attempts to leave, but can't!", ch, 0, 0, TO_ROOM, INVIS_NULL|STAYHIDE);
          return eFAILURE;
          }
        do_look(ch, "", 9);
        sprintf(buf, "%s walks out of %s.", GET_NAME(ch), k->short_description);
        act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL|STAYHIDE);
        return ambush(ch);
        }
      }
    }

  send_to_char("There are no exits around!\r\n", ch);

  return eFAILURE;
}


int do_enter(CHAR_DATA *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  int retval;
  
  CHAR_DATA *sesame;
  OBJ_DATA *portal = NULL;

  if ((ch->in_room != -1) || (ch->in_room)) {
    one_argument(argument, buf);
    }

  if (!*buf) {
    send_to_char("Enter what?\n\r", ch);
    return eFAILURE;
    }

  if ((portal = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents)) == NULL) {
    send_to_char("Nothing here by that name.\n\r", ch);
    return eFAILURE;
    }

  if (portal->obj_flags.type_flag != ITEM_PORTAL) {
    send_to_char("You can't enter that.\r\n", ch);
    return eFAILURE;
    }

  if (real_room(portal->obj_flags.value[0]) < 0) {
    sprintf(buf, "Error in do_enter(), value 0 on object %d < 0", portal->item_number);
    log(buf, OVERSEER, LOG_BUG);
    send_to_char("You can't enter that.\r\n", ch);
    return eFAILURE;
    }

  if (world[real_room(portal->obj_flags.value[0])].sector_type == SECT_UNDERWATER &&
    !(affected_by_spell(ch, SPELL_WATER_BREATHING) || IS_AFFECTED(ch, AFF_WATER_BREATHING))) {
    send_to_char("You bravely attempt to plunge through the portal - let's hope you have gills!\r\n", ch);
    return eSUCCESS;
    }

  if (GET_LEVEL(ch) > IMMORTAL && GET_LEVEL(ch) < DEITY &&
    IS_SET(world[real_room(portal->obj_flags.value[0])].room_flags, CLAN_ROOM)) {
    send_to_char("You may not enter a clanhall at your level.\r\n", ch);
    return eFAILURE;
    }

  if (IS_NPC(ch) && ch->master && mob_index[ch->mobdata->nr].virt == 8) {
    sesame = ch->master;
    if (IS_SET(world[real_room(portal->obj_flags.value[0])].room_flags, CLAN_ROOM)) {
      // Is golem's master not a member of this clan
      if (others_clan_room(sesame, &world[real_room(portal->obj_flags.value[0])]) == true) {
	send_to_char("Your master is not from that clan.\r\n", ch);
	act("$n finds $mself unable to enter!", ch, 0, 0, TO_ROOM, 0);
	do_say(ch, "I may not enter.", 9);
	return eFAILURE;
      }
    }
  } else {
    sesame = ch;
  }

  // should probably just combine this with 'if' below it, but i'm lazy
  if(IS_SET(portal->obj_flags.value[3], PORTAL_NO_ENTER)) {
    send_to_char("The portal's destination rebels against you.\r\n", ch);
    act("$n finds $mself unable to enter!", ch, 0, 0, TO_ROOM, 0);
    return eFAILURE;
    }
  
  // Thieves arent allowed through cleric portals
  if (affected_by_spell(ch, FUCK_PTHIEF) && portal->obj_flags.value[1] == 0)
  {
    send_to_char("Your attempt to transport stolen goods through planes of magic fails!\r\n",ch);
    return eFAILURE;
  }

  if (!IS_MOB(ch) 
      && (affected_by_spell(ch, FUCK_PTHIEF) 
          || affected_by_spell(ch, FUCK_GTHIEF) 
          || IS_AFFECTED(ch, AFF_CHAMPION)
         ) 
      && (IS_SET(world[real_room(portal->obj_flags.value[0])].room_flags, CLAN_ROOM) 
          || (portal->obj_flags.value[0] >= 1900 && portal->obj_flags.value[0] <= 1999 && !portal->obj_flags.value[1])
         )
     ) 
    {
    send_to_char("The portal's destination rebels against you.\r\n", ch);
    act("$n finds $mself unable to enter!", ch, 0, 0, TO_ROOM, 0);
    return eFAILURE;
    }

  if(isname("only", portal->name) && !isname(GET_NAME(sesame), portal->name)) {
    send_to_char("The portal fades when you draw near, then shimmers as you withdraw.\n\r", ch);
    return eFAILURE;
    }

  switch(portal->obj_flags.value[1]) {
    case 0:
      send_to_char("You reach out tentatively and touch the portal...\n\r", ch);
      act("$n reaches out to the glowing dimensional portal....", ch, 0, 0, TO_ROOM, 0);
      break;
    case 1:
    case 2:
      sprintf(buf, "You take a bold step towards %s.\n\r", portal->short_description);
      send_to_char(buf, ch);
      sprintf(buf, "%s boldly walks toward %s and disappears.",
      GET_NAME(ch), portal->short_description);
      act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL|STAYHIDE);
      break;
    case 3:
      send_to_char("You cannot enter that.\r\n", ch);
      return eFAILURE;
    default:
      sprintf(buf, "Error in do_enter(), value 1 on object %d returned default case",
      portal->item_number);
      log(buf, OVERSEER, LOG_BUG);
      return eFAILURE;
    }

  retval = move_char(ch, real_room(portal->obj_flags.value[0]));

  if(SOMEONE_DIED(retval))
    return retval;

  if(!IS_SET(retval, eSUCCESS)) {
    send_to_char("You recoil in pain as the portal slams shut!\n\r", ch);
    act("$n recoils in pain as the portal slams shut!", ch, 0, 0, TO_ROOM, 0);
    }

  switch(portal->obj_flags.value[1]) {
    case 0:
      do_look(ch, "", 9);
      WAIT_STATE(ch, PULSE_VIOLENCE);
      send_to_char("\n\rYou are momentarily dazed from the dimensional shift.\n\r", ch);
      act("The portal glows brighter for a second as $n appears beside you.", ch, 0, 0, TO_ROOM, 0);
      break;
    case 1:
    case 2:
      sprintf(buf, "%s has entered %s.", GET_NAME(ch), portal->short_description);
      act(buf, ch, 0, 0, TO_ROOM, STAYHIDE);
      do_look(ch, "", 9);
      break;
    case 3:
      break;
    default:
      break;
    }

  return ambush(ch);
}


int move_char (CHAR_DATA *ch, int dest)
{
  return move_char(ch, dest, true);  
}

int move_char (CHAR_DATA *ch, int dest, bool stop_all_fighting)
{
  if(!ch) {
    log("Error in move_char(), NULL character", OVERSEER, LOG_BUG);
    return eINTERNAL_ERROR;
    }

  int origination = ch->in_room;

  if(ch->in_room != NOWHERE) {
    // Couldn't move char from the room
    if(char_from_room(ch, stop_all_fighting) == 0) {
      log("Error in move_char(), character not NOWHERE, but couldn't be moved.",
      OVERSEER, LOG_BUG);
      return eINTERNAL_ERROR;
      }
    }

  // Couldn't move char to new room
  if (char_to_room(ch, dest) == 0) {
    // Now we have real problems
    if(char_to_room(ch, origination) == 0) {
      fprintf(stderr, "Error in move_char(), character stuck in NOWHERE: %s.\n",
      GET_NAME(ch));
      abort();
      }
    logf(OVERSEER, LOG_BUG, "Error in move_char(), could not move %s to %d.",
    GET_NAME(ch), world[dest].number);
    return eINTERNAL_ERROR;
    }

  // At this point, the character is happily in the new room
  return eSUCCESS;
}


int do_climb(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH];
  obj_data * obj = NULL;
  
  one_argument(argument, buf);
  
  if(!*buf) {
    send_to_char("Climb what?\r\n", ch);
    return eSUCCESS;
    }

  if(!(obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
    send_to_char("Climb what?\r\n", ch);
    return eSUCCESS;
    }

  if(obj->obj_flags.type_flag != ITEM_CLIMBABLE) {
    send_to_char("You can't climb that.\r\n", ch);
    return eSUCCESS;
    }

  int dest = obj->obj_flags.value[0];

  if(real_room(dest) < 0) {
    logf(IMMORTAL, LOG_WORLD, "Error in do_climb(), illegal destination in object %d.",
    obj_index[obj->item_number].virt);
    send_to_char("You can't climb that.\r\n", ch);
    return eFAILURE|eINTERNAL_ERROR;
    }

  act("$n carefully climbs $p.", ch, obj, 0, TO_ROOM, INVIS_NULL);
  sprintf(buf, "You carefully climb %s.\r\n", obj->short_description);
  send_to_char(buf, ch);
  int retval = move_char(ch, real_room(dest));

  if(SOMEONE_DIED(retval))
    return retval;

  act("$n carefully climbs $p.", ch, obj, 0, TO_ROOM, INVIS_NULL);
  do_look(ch, "", 9);
  
  return eSUCCESS;
}


// The End

