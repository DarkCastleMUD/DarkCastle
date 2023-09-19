/******************************************************************************
 * $Id: cl_ranger.cpp,v 1.95 2015/05/30 04:48:42 pirahna Exp $ | cl_ranger.C  *
 * Description: Ranger skills/spells                                          *
 *                                                                            *
 * Revision History                                                           *
 * 10/28/2003  Onager   Modified do_tame() to remove hate flag for tamer      *
 * 12/08/2003  Onager   Added eq check to do_tame() to remove !charmie eq     *
 *                      from charmies                                         *
 ******************************************************************************/

#include <string.h>
#include <stdio.h>

#include "character.h"
#include "affect.h"
#include "mobile.h"
#include "utility.h"
#include "spells.h"
#include "isr.h"
#include "handler.h"
#include "room.h"
#include "terminal.h"
#include "player.h"
#include "levels.h"
#include "connect.h"
#include "fight.h"
#include "interp.h"
#include "db.h"
#include "act.h"
#include "fileinfo.h" // SAVE_DIR
#include "returnvals.h"
#include "inventory.h"
#include "const.h"
#include "move.h"
#include "corpse.h"


extern class Object *object_list;
extern int rev_dir[];

int saves_spell(Character *ch, Character *vict, int spell_base, int16_t save_type);
void check_eq(Character *ch);
extern struct index_data *mob_index;
int get_difficulty(int);

int charm_space(int level)
{
  if (level >= 50)
    return 10;
  if (level >= 41)
    return 7;
  if (level >= 31)
    return 5;
  if (level >= 21)
    return 3;
  if (level >= 11)
    return 2;
  return 1;
}

int charm_levels(Character *ch)
{
  int i = GET_LEVEL(ch) / 5;
  int z = 3;
  struct follow_type *f;
  for (f = ch->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_CHARM))
    {
      z--;
      i -= charm_space(GET_LEVEL(f->follower));
    }
  if (z <= 0)
    return -1;
  return i;
}

int do_free_animal(Character *ch, char *arg, int cmd)
{
  Character *victim = nullptr;
  char buf[MAX_INPUT_LENGTH];
  void stop_follower(Character * ch, int cmd);

  if (!has_skill(ch, SKILL_FREE_ANIMAL))
  {
    send_to_char("Try learning HOW to free an animal first.\r\n", ch);
    return eFAILURE;
  }

  arg = one_argument(arg, buf);

  for (struct follow_type *k = ch->followers; k; k = k->next)
    if (IS_MOB(k->follower) && ISSET(k->follower->affected_by, AFF_CHARM) && isname(buf, GET_NAME(k->follower)))
    {
      victim = k->follower;
      break;
    }

  if (!victim)
  {
    send_to_char("They aren't even here!\r\n", ch);
    return eSUCCESS;
  }

  if (!charge_moves(ch, SKILL_FREE_ANIMAL))
    return eSUCCESS;

  if (!skill_success(ch, victim, SKILL_FREE_ANIMAL))
  {
    act("You give $N a gentle pat to the head, but $E seems unwilling to leave your side.",
        ch, nullptr, victim, TO_CHAR, 0);
    act("$n gives $N a gentle pat to the head, but $E seems unwilling to leave $s side.",
        ch, nullptr, victim, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  act("With a gentle pat to the head, you set $N free to roam the wilds again.",
      ch, nullptr, victim, TO_CHAR, 0);
  act("With a gentle pat to the head, $n sets $N free to roam the wilds again.",
      ch, nullptr, victim, TO_ROOM, INVIS_NULL);

  stop_follower(victim, 0);

  return eSUCCESS;
}

int do_tame(Character *ch, char *arg, int cmd)
{
  struct affected_type af;
  Character *victim;
  char buf[MAX_INPUT_LENGTH];

  while (*arg == ' ')
    arg++;

  if (!*arg)
  {
    send_to_char("Who do you want to tame?\n\r", ch);
    return eFAILURE;
  }

  if (!canPerform(ch, SKILL_TAME, "Try learning HOW to tame first.\r\n"))
  {
    return eFAILURE;
  }

  one_argument(arg, buf);

  if (!(victim = get_char_room_vis(ch, buf)))
  {
    send_to_char("No one here by that name!\n\r", ch);
    return eFAILURE;
  }

  if (victim == ch)
  {
    send_to_char("Tame the wild beast!\n\r", ch);
    return eFAILURE;
  }

  if (IS_PC(victim))
  {
    send_to_char("You find yourself unable to tame this player.\r\n", ch);
    return eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_CHARM) || IS_AFFECTED(ch, AFF_CHARM) ||
      (GET_LEVEL(ch) < GET_LEVEL(victim)))
  {
    send_to_char("You find yourself unable to tame this creature.\r\n", ch);
    return eFAILURE;
  }

  if (circle_follow(victim, ch))
  {
    send_to_char("Sorry, following in circles can not be allowed.\r\n", ch);
    return eFAILURE;
  }

  if (charm_levels(ch) - charm_space(GET_LEVEL(victim)) < 0)
  {
    send_to_char("How you plan on controlling so many followers?\n\r", ch);
    return eFAILURE;
    /*   Character * vict = nullptr;
       for(struct follow_type *k = ch->followers; k; k = k->next)
         if(IS_MOB(k->follower) && affected_by_spell(k->follower, SPELL_CHARM_PERSON))
         {
            vict = k->follower;
            break;
         }
         if (vict) {
      if (vict->in_room == ch->in_room && vict->position > POSITION_SLEEPING)
        do_say(vict, "Hey... but what about ME!?", CMD_DEFAULT);
             remove_memory(vict, 'h');
      if (vict->master) {
             stop_follower(vict, BROKE_CHARM);
             add_memory(vict, GET_NAME(ch), 'h');
      }
         }*/
  }

  if (!charge_moves(ch, SKILL_TAME))
    return eSUCCESS;

  act("$n holds out $s hand to $N and beckons softly.", ch, nullptr, victim, TO_ROOM, INVIS_NULL);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1);

  if ((IS_SET(victim->immune, ISR_CHARM)) ||
      !ISSET(victim->mobdata->actflags, ACT_CHARM))
  {
    act("$N is wilder than you thought.", ch, nullptr, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if (!skill_success(ch, victim, SKILL_TAME) || saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) >= 0)
  {
    act("$N is unreceptive to your attempts to tame $M.", ch, nullptr, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if (victim->master)
    stop_follower(victim, 0);

  /* make charmie stop hating tamer */
  remove_memory(victim, 'h', ch);

  add_follower(victim, ch, 0);

  af.type = SPELL_CHARM_PERSON;

  if (GET_INT(victim))
    af.duration = 24 * 18 / GET_INT(victim);
  else
    af.duration = 24 * 18;

  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_CHARM;
  affect_to_char(victim, &af);

  /* remove any !charmie eq the charmie is wearing */
  check_eq(victim);

  act("Isn't $n just such a nice person?", ch, 0, victim, TO_VICT, 0);
  return eSUCCESS;
}

int do_track(Character *ch, char *argument, int cmd)
{
  int x, y;
  int retval, how_deep, learned;
  char buf[300];
  char race[40];
  char sex[20];
  char condition[60];
  char weight[40];
  char victim[MAX_INPUT_LENGTH];
  Character *quarry;
  Character *tmp_ch = nullptr; // For checking room stuff
  room_track_data *pScent = 0;
  void swap_hate_memory(Character * ch);

  one_argument(argument, victim);

  learned = how_deep = ((has_skill(ch, SKILL_TRACK) / 10) + 1);

  if (GET_LEVEL(ch) >= IMMORTAL)
    how_deep = 50;

  quarry = get_char_room_vis(ch, victim);

  if (ch->hunting)
  {
    if (get_char_room_vis(ch, ch->hunting))
    {
      ansi_color(RED, ch);
      ansi_color(BOLD, ch);
      send_to_char("You have found your target!\n\r", ch);
      ansi_color(NTEXT, ch);

      //      remove_memory(ch, 't');
      return eSUCCESS;
    }
  }

  else if (quarry)
  {
    send_to_char("There's one right here ;)\n\r", ch);
    //  remove_memory(ch, 't');
    return eSUCCESS;
  }

  if (*victim && IS_PC(ch) && GET_CLASS(ch) != CLASS_RANGER && GET_CLASS(ch) != CLASS_DRUID && GET_LEVEL(ch) < ANGEL)
  {
    send_to_char("Only a ranger could track someone by name.\r\n", ch);
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_TRACK))
    return eSUCCESS;

  act("$n walks about slowly, searching for signs of $s quarry", ch, 0, 0, TO_ROOM, INVIS_NULL);
  send_to_char("You search for signs of your quarry...\n\r\n\r", ch);

  if (learned)
    skill_increase_check(ch, SKILL_TRACK, learned, SKILL_INCREASE_MEDIUM);

  // TODO - once we're sure that act_mob is properly checking for this,
  // and that it isn't call from anywhere else, we can probably remove it.
  // That way possessing imms can track.
  if (IS_MOB(ch) && ISSET(ch->mobdata->actflags, ACT_STUPID))
  {
    send_to_char("Being stupid, you cannot find any..\r\n", ch);
    return eFAILURE;
  }

  if (DC::getInstance()->world[ch->in_room].nTracks < 1)
  {
    if (ch->hunting)
    {
      ansi_color(RED, ch);
      ansi_color(BOLD, ch);
      send_to_char("You have lost the trail.\r\n", ch);
      ansi_color(NTEXT, ch);
      // remove_memory(ch, 't');
    }
    else
      send_to_char("There are no distinct scents here.\r\n", ch);
    return eFAILURE;
  }

  if (IS_NPC(ch))
    how_deep = 10;

  if (*victim)
  {
    for (x = 1; x <= how_deep; x++)
    {

      if ((x > DC::getInstance()->world[ch->in_room].nTracks) ||
          !(pScent = DC::getInstance()->world[ch->in_room].TrackItem(x)))
      {
        if (ch->hunting)
        {
          ansi_color(RED, ch);
          ansi_color(BOLD, ch);
          send_to_char("You have lost the trail.\r\n", ch);
          ansi_color(NTEXT, ch);
        }
        else
          send_to_char("You can't find any traces of such a scent.\r\n", ch);
        //         remove_memory(ch, 't');
        if (IS_NPC(ch))
          swap_hate_memory(ch);
        return eFAILURE;
      }

      if (isname(victim, pScent->trackee))
      {
        y = pScent->direction;
        add_memory(ch, pScent->trackee, 't');
        ansi_color(RED, ch);
        ansi_color(BOLD, ch);
        csendf(ch, "You sense traces of your quarry to the %s.\r\n",
               dirs[y]);
        ansi_color(NTEXT, ch);

        if (IS_NPC(ch))
        {
          // temp disable tracking mobs into town
          if (DC::getInstance()->zones.value(DC::getInstance()->world[EXIT(ch, y)->to_room].zone).isTown() == false && !IS_SET(DC::getInstance()->world[EXIT(ch, y)->to_room].room_flags, NO_TRACK))
          {
            ch->mobdata->last_direction = y;
            retval = do_move(ch, "", (y + 1));
            if (IS_SET(retval, eCH_DIED))
              return retval;
          }

          if (!ch->hunting)
            return eFAILURE;

          // Here's the deal: if the mob can't see the character in
          // the room, but the character IS in the room, then the
          // mob can't see the character and we need to stop tracking.
          // It does, however, leave the mob open to be taken apart
          // by, say, a thief.  I'll let he who wrote it fix that.
          // Morc 28 July 96

          if ((tmp_ch = get_char(ch->hunting)) == 0)
            return eFAILURE;
          if (!(get_char_room_vis(ch, ch->hunting)))
          {
            if (tmp_ch->in_room == ch->in_room)
            {
              // The mob can't see him
              act("$n says 'Damn, must have lost $M!'", ch, 0, tmp_ch,
                  TO_ROOM, 0);
              //                    remove_memory(ch, 't');
            }
            return eFAILURE;
          }

          if (!IS_SET(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
          {
            act("$n screams 'YOU CAN RUN, BUT YOU CAN'T HIDE!'",
                ch, 0, 0, TO_ROOM, 0);
            retval = eSUCCESS;
            if (tmp_ch)
            {
              retval = mprog_attack_trigger(ch, tmp_ch);
            }
            if (SOMEONE_DIED(retval) || (ch && ch->fighting) || !ch->hunting)
              return retval;
            else
              return do_hit(ch, ch->hunting, 0);
          }
          else
            act("$n says 'You can't stay here forever.'",
                ch, 0, 0, TO_ROOM, 0);
        } // if IS_NPC

        return eSUCCESS;
      } // if isname
    }   // for

    if (ch->hunting)
    {
      ansi_color(RED, ch);
      ansi_color(BOLD, ch);
      send_to_char("You have lost the trail.\r\n", ch);
      ansi_color(NTEXT, ch);
    }
    else
      send_to_char("You can't find any traces of such a scent.\r\n", ch);

    //    remove_memory(ch, 't');
    return eFAILURE;
  } // if *victim

  for (x = 1; x <= how_deep; x++)
  {
    if ((x > DC::getInstance()->world[ch->in_room].nTracks) || !(pScent = DC::getInstance()->world[ch->in_room].TrackItem(x)))
    {
      if (x == 1)
        send_to_char("There are no distinct smells here.\r\n", ch);
      break;
    }

    y = pScent->direction;

    if (pScent->weight < 50)
      strcpy(weight, " small,");
    else if (pScent->weight <= 201)
      strcpy(weight, " medium-sized,");
    else if (pScent->weight < 350)
      strcpy(weight, " big,");
    else if (pScent->weight < 500)
      strcpy(weight, " large,");
    else if (pScent->weight < 800)
      strcpy(weight, " huge,");
    else if (pScent->weight < 1500)
      strcpy(weight, " very large,");
    else
      strcpy(weight, " gigantic,");

    if (pScent->condition < 10)
      strcpy(condition, " severely injured,");
    else if (pScent->condition < 25)
      strcpy(condition, " badly wounded,");
    else if (pScent->condition < 40)
      strcpy(condition, " injured,");
    else if (pScent->condition < 60)
      strcpy(condition, " wounded,");
    else if (pScent->condition < 80)
      strcpy(condition, " slightly injured,");
    else
      strcpy(condition, " healthy,");

    if (pScent->sex == 1)
      strcpy(sex, " male");
    else if (pScent->sex == 2)
      strcpy(sex, " female");
    else
      strcpy(sex, "");

    if (pScent->race >= 1 && pScent->race <= 30)
      sprintf(race, " %s", races[pScent->race].singular_name);
    else
      strcpy(race, " non-descript race");

    if (number(1, 101) >= (how_deep * 10))
      strcpy(weight, "");
    if (number(1, 101) >= (how_deep * 10))
      strcpy(race, " non-descript race");
    if (number(1, 101) >= (how_deep * 10))
      strcpy(condition, "");
    if (number(1, 101) >= (how_deep * 10))
      strcpy(sex, "");

    if (x == 1)
      send_to_char("Freshest scents first...\r\n", ch);

    sprintf(buf, "The scent of a%s%s%s%s leads %s.\r\n",
            weight,
            condition,
            sex,
            race,
            dirs[y]);
    send_to_char(buf, ch);
  }
  return eSUCCESS;
}

int do_ambush(Character *ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];

  if (!canPerform(ch, SKILL_AMBUSH))
  {
    send_to_char("You don't know how to ambush people!\r\n", ch);
    return eFAILURE;
  }

  one_argument(arg, arg);

  if (!*arg)
  {
    sprintf(buf, "You will ambush %s on sight.\r\n", ch->ambush ? ch->ambush : "no one");
    send_to_char(buf, ch);
    return eSUCCESS;
  }

  if (!(ch->ambush))
  {
    sprintf(buf, "You will now ambush %s on sight.\r\n", arg);
    send_to_char(buf, ch);
    ch->ambush = str_dup(arg);
    return eSUCCESS;
  }

  if (!str_cmp(arg, ch->ambush))
  {
    sprintf(buf, "You will no longer ambush %s on sight.\r\n", arg);
    send_to_char(buf, ch);
    dc_free(ch->ambush);
    ch->ambush = nullptr;
    return eSUCCESS;
  }

  sprintf(buf, "You will now ambush %s on sight.\r\n", arg);

  // TODO - remove this later after I've watched for Bushmaster to do it a few times
  if (strlen(buf) > MAX_INPUT_LENGTH)
    logf(OVERSEER, LogChannels::LOG_BUG, "%s just tried to crash the mud with a huge ambush string (%s)",
         GET_NAME(ch), arg);

  send_to_char(buf, ch);
  dc_free(ch->ambush);
  ch->ambush = str_dup(arg);
  return eSUCCESS;
}

/*
SECT_INSIDE           0
SECT_CITY             1
SECT_FIELD            2
SECT_FOREST           3
SECT_HILLS            4
SECT_MOUNTAIN         5
SECT_WATER_SWIM       6
SECT_WATER_NOSWIM     7
SECT_NO_LOW           8
SECT_NO_HIGH          9
SECT_DESERT          10
underwater
swamp
air
*/

int pick_one(int a, int b)
{
  return number(1, 100) <= 50 ? a : b;
}

int pick_one(int a, int b, int c)
{
  int x = number(1, 100);

  if (x > 66)
    return a;
  else if (x > 33)
    return b;
  else
    return c;
}

int pick_one(int a, int b, int c, int d)
{
  int x = number(1, 100);

  if (x > 75)
    return a;
  else if (x > 50)
    return b;
  else if (x > 25)
    return c;
  else
    return d;
}

struct forage_lookup
{
  int ovnum;
  int rate[4];
};

struct forage_lookup forage_lookup_table[SECT_MAX_SECT + 1][6] = {
    // SECT_INSIDE
    {{0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}}},

    // SECT_CITY
    {{0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}}},

    // SECT_FIELD
    {{6306, {8, 9, 10, 11}},
     {6310, {3, 4, 5, 6}},
     {3171, {36, 34, 32, 30}},
     {3173, {7, 8, 9, 10}},
     {3174, {10, 11, 12, 13}},
     {3176, {36, 34, 32, 30}}},

    // SECT_FOREST
    {{6308, {8, 9, 10, 11}},
     {6311, {3, 4, 5, 6}},
     {3167, {36, 34, 32, 30}},
     {3175, {36, 34, 32, 30}},
     {3185, {10, 11, 12, 13}},
     {27800, {7, 8, 9, 10}}},

    // SECT_HILLS
    {{6306, {8, 9, 10, 11}},
     {6310, {3, 4, 5, 6}},
     {3171, {36, 34, 32, 30}},
     {3173, {7, 8, 9, 10}},
     {3174, {10, 11, 12, 13}},
     {3176, {36, 34, 32, 30}}},

    // SECT_MOUNTAIN
    {{6305, {3, 4, 5, 6}},
     {3177, {36, 34, 32, 30}},
     {3178, {8, 9, 10, 11}},
     {3179, {7, 8, 9, 10}},
     {3184, {36, 34, 32, 30}},
     {4, {10, 11, 12, 13}}},

    // SECT_WATER_SWIM
    {{6303, {8, 9, 10, 11}},
     {3160, {36, 34, 32, 30}},
     {3162, {7, 8, 9, 10}},
     {3163, {10, 11, 12, 13}},
     {3164, {3, 4, 5, 6}},
     {3189, {36, 34, 32, 30}}},

    // SECT_WATER_NOSWIM
    {{6303, {8, 9, 10, 11}},
     {3160, {36, 34, 32, 30}},
     {3162, {7, 8, 9, 10}},
     {3163, {10, 11, 12, 13}},
     {3164, {3, 4, 5, 6}},
     {3189, {36, 34, 32, 30}}},

    // SECT_BEACH
    {{6302, {8, 9, 10, 11}},
     {6309, {3, 4, 5, 6}},
     {6304, {7, 8, 9, 10}},
     {3161, {36, 34, 32, 30}},
     {3169, {36, 34, 32, 30}},
     {3187, {10, 11, 12, 13}}},

    // SECT_PAVED_ROAD
    {{0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}}},

    // SECT_DESERT
    {{6307, {3, 4, 5, 6}},
     {3165, {36, 34, 32, 30}},
     {3166, {36, 34, 32, 30}},
     {3172, {8, 9, 10, 11}},
     {3186, {10, 11, 12, 13}},
     {1570, {7, 8, 9, 10}}},

    // SECT_UNDERWATER
    {{0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}}},

    // SECT_SWAMP
    {{6301, {8, 9, 10, 11}},
     {6312, {3, 4, 5, 6}},
     {3170, {36, 34, 32, 30}},
     {3180, {7, 8, 9, 10}},
     {3181, {10, 11, 12, 13}},
     {7717, {36, 34, 32, 30}}},

    // SECT_AIR
    {{0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}}},

    // SECT_FROZEN_TUNDRA
    {{28314, {7, 8, 9, 10}},
     {44, {10, 11, 12, 13}},
     {5219, {36, 34, 32, 30}},
     {3168, {8, 9, 10, 11}},
     {28313, {3, 4, 5, 6}},
     {28301, {36, 34, 32, 30}}},

    // SECT_ARCTIC
    {{28314, {7, 8, 9, 10}},
     {44, {10, 11, 12, 13}},
     {5219, {36, 34, 32, 30}},
     {3168, {8, 9, 10, 11}},
     {28313, {3, 4, 5, 6}},
     {28301, {36, 34, 32, 30}}},
};

int do_forage(Character *ch, char *arg, int cmd)
{
  int learned;
  class Object *new_obj = 0;
  struct affected_type af;

  if (affected_by_spell(ch, SKILL_FORAGE))
  {
    if (number(0, 1) == 0)
      send_to_char("You already foraged recently.  Give mother nature a break!\n\r", ch);
    else
      send_to_char("There's a limit to how often you can play with your nuts.  Give it some time.\r\n", ch);
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_FORAGE))
    return eSUCCESS;

  learned = has_skill(ch, SKILL_FORAGE);
  if (!learned)
  {
    send_to_char("Not knowing how to forage, you poke at the dirt with a stick, finding nothing.\r\n", ch);
    return eFAILURE;
  }

  int pick = number(1, 100);
  int ovnum;
  int lgroup = 0;

  if (learned >= 1 && learned <= 30)
  {
    lgroup = 0;
  }
  else if (learned >= 31 && learned <= 60)
  {
    lgroup = 1;
  }
  else if (learned >= 61 && learned <= 90)
  {
    lgroup = 2;
  }
  else if (learned >= 91)
  {
    lgroup = 3;
  }
  int cur_sector = DC::getInstance()->world[ch->in_room].sector_type;

  // If in a clan or safe room, set sector to inside so we fail forage
  if (IS_SET(DC::getInstance()->world[ch->in_room].room_flags, CLAN_ROOM) ||
      IS_SET(DC::getInstance()->world[ch->in_room].room_flags, SAFE) ||
      IS_SET(DC::getInstance()->world[ch->in_room].room_flags, INDOORS))
  {
    cur_sector = SECT_INSIDE;
  }

  int last = 0;

  for (int i = 0; i < 6; i++)
  {
    if (pick > last && pick <= last + forage_lookup_table[cur_sector][i].rate[lgroup])
    {
      ovnum = forage_lookup_table[cur_sector][i].ovnum;

      if ((1 + IS_CARRYING_N(ch)) > CAN_CARRY_N(ch))
      {
        send_to_char("You can't carry that many items!\r\n", ch);
        return eFAILURE;
      }

      new_obj = clone_object(real_object(ovnum));
      break;
    }
    last = last + forage_lookup_table[cur_sector][i].rate[lgroup];
  }

  int recharge;
  if (new_obj)
    recharge = 10 - (learned / 20);
  else
    recharge = 6;

  af.type = SKILL_FORAGE;
  af.duration = recharge;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af, DC::PULSE_VIOLENCE);

  skill_increase_check(ch, SKILL_FORAGE, learned, SKILL_INCREASE_HARD);
  if (!new_obj)
  {
    act("$n forages around for some food, but turns up nothing.", ch, 0, 0, TO_ROOM, 0);
    act("You forage around for some food, but turn up nothing.", ch, 0, 0, TO_CHAR, 0);
    return eFAILURE;
  }

  act("$n forages around, turning up $p.", ch, new_obj, 0, TO_ROOM, 0);
  act("You forage around, turning up $p.", ch, new_obj, 0, TO_CHAR, 0);
  obj_to_char(new_obj, ch);
  new_obj->obj_flags.timer = 4;
  return eSUCCESS;
}

/* this is sent the arrow char string, and return the arrow type.
   also, checks level to make sure char is high enough
   return 0 for failure */

int parse_arrow(Character *ch, char *arrow)
{

  if (GET_CLASS(ch) != CLASS_RANGER && GET_LEVEL(ch) < 100)
    return 0;

  while (*arrow == ' ')
    arrow++;

  if ((arrow[0] == 'f') && has_skill(ch, SKILL_FIRE_ARROW))
    return 1;
  else if ((arrow[0] == 'i') && has_skill(ch, SKILL_ICE_ARROW))
    return 2;
  else if ((arrow[0] == 't') && has_skill(ch, SKILL_TEMPEST_ARROW))
    return 3;
  else if ((arrow[0] == 'g') && has_skill(ch, SKILL_GRANITE_ARROW))
    return 4;

  return 0;
}

/* go through and find an arrow */
/* return nullptr if failure
 *  return pointer if success */
class Object *find_arrow(class Object *quiver)
{
  class Object *target;

  class Object *get_obj_in_list(char *, class Object *);

  target = get_obj_in_list("arrow", quiver->contains);

  if (!target)
    return nullptr;

  if (!(target->obj_flags.type_flag == ITEM_MISSILE))
    target = nullptr;

  return target;
}

void do_arrow_miss(Character *ch, Character *victim, int dir, class Object *found)
{
  char buf[200];

  switch (number(1, 6))
  {
  case 1:
    send_to_char("You miss your shot.\r\n", ch);
    break;
  case 2:
    send_to_char("Your arrow wizzes by the target harmlessely.\r\n", ch);
    break;
  case 3:
    send_to_char("Your pitiful aim spears a poor woodland creature instead..\r\n", ch);
    break;
  case 4:
    send_to_char("Your shot misses your target, and goes skittering across the ground.\r\n", ch);
    break;
  case 5:
    send_to_char("A slight breeze forces your arrow off the mark.\r\n", ch);
    break;
  case 6:
    send_to_char("Your shot narrowly misses the mark.\r\n", ch);
    break;
  }

  switch (number(1, 3))
  {
  case 1:
    if (dir < 0)
    {
      sprintf(buf, "%s wizzes by.\r\n", found->short_description);
      send_to_char(buf, victim);
      sprintf(buf, "%s wizzes by.", found->short_description);
      act(buf, victim, nullptr, ch, TO_ROOM, NOTVICT);
    }
    else
    {
      sprintf(buf, "%s wizzes by from the %s.\r\n", found->short_description, dirs[rev_dir[dir]]);
      send_to_char(buf, victim);
      sprintf(buf, "%s wizzes by from the %s.",
              found->short_description, dirs[rev_dir[dir]]);
      act(buf, victim, nullptr, 0, TO_ROOM, 0);
    }
    break;
  case 2:
    sprintf(buf, "A quiet whistle sounds as %s flies over your head.",
            found->short_description);
    act(buf, victim, 0, 0, TO_CHAR, 0);
    sprintf(buf, "A quiet whistle sounds as %s flies over your head.",
            found->short_description);
    act(buf, victim, 0, ch, TO_ROOM, NOTVICT);
    break;
  case 3:
    if (dir < 0)
    {
      sprintf(buf, "%s narrowly misses your head.\r\n", found->short_description);
      send_to_char(buf, victim);
      sprintf(buf, "%s narrowly misses $n.", found->short_description);
      act(buf, victim, nullptr, ch, TO_ROOM, NOTVICT);
    }
    else
    {
      sprintf(buf, "%s from the %s narrowly misses your head.\r\n",
              found->short_description, dirs[rev_dir[dir]]);
      send_to_char(buf, victim);
      sprintf(buf, "%s from the %s narrowly misses $n.",
              found->short_description, dirs[rev_dir[dir]]);
      act(buf, victim, nullptr, 0, TO_ROOM, 0);
    }
    break;
  }
}

int mob_arrow_response(Character *ch, Character *victim,
                       int dir)
{
  int dir2 = 0;
  int retval;

  /* this will make IS_STUPID mobs alot easier to kill with arrows,
     but then again, they _are_ 'stupid'.  Keeps people from tracking
     the waterwheel around shire though.
  */

  if (ISSET(victim->mobdata->actflags, ACT_STUPID))
  {
    if (!number(0, 20))
      do_shout(victim, "Duh George, someone keeps shooting me!", CMD_DEFAULT);
    return eSUCCESS;
  }

  /* make mob hate the person, but _not_ track them, this should
   * help make it harder for people to abuse this to RK people.
   * Not impossible, but harder
   */

  add_memory(victim, GET_NAME(ch), 'h');

  /* don't want the mob leaving a fight its already in */
  if (victim->fighting)
    return eSUCCESS;

  if (dir < 0) // in the same room
    return eSUCCESS;

  if (number(0, 1))
  {
    /* Send the mob in a random dir */
    if (number(1, 2) == 1)
    {
      do_say(victim, "Where are these fricken arrows coming from?!", 0);
      dir2 = number(0, 5);
      if (CAN_GO(victim, dir2))
        if (EXIT(victim, dir2))
        {
          if (!(ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN) &&
                DC::getInstance()->zones.value(DC::getInstance()->world[EXIT(victim, dir2)->to_room].zone).isTown()) &&
              !IS_SET(DC::getInstance()->world[EXIT(victim, dir2)->to_room].room_flags, NO_TRACK))
            /* send 1-6 since attempt move --cmd's it */
            return attempt_move(victim, dir2 + 1, 0);
        }
    }
  }
  else
  {
    /* Send the mob after the fucker! */
    if (number(1, 2) == 1)
    {
      do_say(victim, "There he is!", 0);
    }
    dir2 = rev_dir[dir];
    for (int i = 0; i < 4; i++)
      if (!CAN_GO(ch, dir2))
        dir2 = number(0, 5);
    if (EXIT(victim, dir2))
    {
      if (CAN_GO(ch, dir2))
        if (!(ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN) &&
              DC::getInstance()->zones.value(DC::getInstance()->world[EXIT(victim, dir2)->to_room].zone).isTown()))
          if (!IS_SET(DC::getInstance()->world[EXIT(victim, dir2)->to_room].room_flags, NO_TRACK))
          {
            /* dir+1 it since attempt_move will --cmd it */
            retval = attempt_move(victim, (dir2 + 1), 0);
            if (SOMEONE_DIED(retval))
              return retval;

            if (IS_SET(retval, eFAILURE)) // can't go after the archer
              return do_flee(victim, "", 0);
          }
    }
    if (number(1, 5) == 1)
    {
      if (number(0, 1))
      {
        do_shout(victim, "Where the fuck are these arrows coming from?!", CMD_DEFAULT);
      }
      else
      {
        do_shout(victim, "Quit shooting me dammit!", CMD_DEFAULT);
      }
    }
  }
  return eSUCCESS;
}

/* no need anymore
int do_arrow_damage(Character *ch, Character *victim,
                     int dir, int dam, int artype,
                     class Object *found)
{
  char buf[200];
  int retval;

  void inform_victim(Character *, Character *, int);

  buf[199] = '\0'; // just cause I'm paranoid

  set_cantquit(ch, victim);

  if(0 > (victim->getHP() - dam))
  { // they aren't going to survive..life sucks
   switch(number(1,2))
   {
    case 1:
    sprintf(buf, "Your shot impales %s through the heart!\r\n", GET_SHORT(victim));
    send_to_char(buf, ch);
    sprintf(buf, "%s from the %s drives full force into your chest!\r\n",
       found->short_description, dirs[rev_dir[dir]]);
    send_to_char(buf, victim);
    sprintf(buf, "%s from the %s impales $n through the chest!",
       found->short_description, dirs[rev_dir[dir]]);
    act(buf, victim, nullptr, 0, TO_ROOM, 0);
    break;

    case 2:
    sprintf(buf, "Your %s drives through the eye of %s ending their life.\r\n",
       found->short_description, GET_SHORT(victim));
    send_to_char(buf, ch);
    sprintf(buf, "%s drives right through your left eye!\r\nThe last thing through your mind is.........an arrowhead.\r\n",
       found->short_description);
    send_to_char(buf, victim);
    sprintf(buf, "%s from the %s lands with a solid 'thunk.'\r\n$n falls to the ground, an arrow sticking from $s left eye.",
       found->short_description, dirs[rev_dir[dir]]);
    act(buf, victim, nullptr, 0, TO_ROOM, 0);
    break;
   } // of switch
  }
  else  // they have enough to survive the arrow..lucky bastard
  {
    sprintf(buf, "You hit %s with %s!\r\n", GET_SHORT(victim),
       found->short_description);
    send_to_char(buf, ch);
    sprintf(buf, "You get shot with %s from the %s.  Ouch.",
          found->short_description, dirs[rev_dir[dir]]);
    act(buf, victim, 0, 0, TO_CHAR, 0);
    sprintf(buf, "%s from the %s hits $n!",
          found->short_description, dirs[rev_dir[dir]]);
    act(buf, victim, 0, 0, TO_ROOM, 0);

    switch(artype)
    {
      case 1: send_to_room("It was flaming hot!\r\n", victim->in_room);
              break;
      case 2: send_to_room("It was icy cold!\r\n", victim->in_room);
              break;
      default: break;
    }
    if(IS_NPC(victim))
      retval = mob_arrow_response(ch, victim, dir);
      if(SOMEONE_DIED(retval)) // mob died somehow while moving
         return retval;
  }

  victim->removeHP(dam);
  update_pos(victim);
  inform_victim(ch, victim, dam);

  if(victim->getHP() > 0)
     GET_POS(victim) = POSITION_STANDING;

// This is a cut & paste from fight.C cause I can't think of a better way to do it

  // Payoff for killing things.
  if(victim->getHP() < 0)
  {
    group_gain(ch, victim);
    fight_kill(ch, victim, TYPE_CHOOSE, 0);
    return (eSUCCESS | eVICT_DIED);
  } // End of Hit < 0

  return eSUCCESS;
}
*/

int do_fire(Character *ch, char *arg, int cmd)
{
  Character *victim;
  int dam, dir = -1, artype, cost, retval, victroom;
  class Object *found;
  unsigned cur_room, new_room = 0;
  char direct[MAX_STRING_LENGTH], arrow[MAX_STRING_LENGTH],
      target[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH],
      buf2[MAX_STRING_LENGTH], victname[MAX_STRING_LENGTH],
      victhshr[MAX_STRING_LENGTH];
  bool enchantmentused = false;

  victim = nullptr;
  *direct = '\0';
  *arrow = '\0';

  if (!canPerform(ch, SKILL_ARCHERY))
  {
    send_to_char(
        "You've no idea how those pointy things with strings and feathers work.\r\n",
        ch);
    return eFAILURE;
  }

  if (!ch->equipment[HOLD])
  {
    send_to_char("You need to be holding a bow, moron.\r\n", ch);
    return eFAILURE;
  }

  if (!(ch->equipment[HOLD]->obj_flags.type_flag == ITEM_FIREWEAPON))
  {
    send_to_char("You need to be holding a bow, moron.\r\n", ch);
    return eFAILURE;
  }
  /*
   if(GET_POS(ch) == POSITION_FIGHTING)
   {
   send_to_char("Aren't you a bit busy with hand to hand combat?\r\n", ch);
   return eFAILURE;
   }*/

  if (ch->shotsthisround > 0)
  {
    send_to_char(
        "Slow down there tiger, you can't fire them that fast!\r\n",
        ch);
    return eFAILURE;
  }

  /*  Command syntax is: fire [target] <dir> [arrowtype]
   if there is !dir, then check skill level
   if there is !arrow, then choose standard arrowtype */

  while (*arg == ' ')
    arg++;

  if (!*arg)
  {
    send_to_char("Shoot at whom?\r\n", ch);
    return eFAILURE;
  }
  half_chop(arg, target, buf2);
  half_chop(buf2, direct, arrow);

  direct[MAX_STRING_LENGTH - 1] = '\0';
  arrow[MAX_STRING_LENGTH - 1] = '\0';
  target[MAX_STRING_LENGTH - 1] = '\0';

  /* make safe rooms checks */
  if (IS_SET(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    send_to_char("You can't shoot arrows if yer in a safe room silly.\r\n",
                 ch);
    return eFAILURE;
  }

  cost = 0;
  dir = -1;
  artype = 0;

  if (*arrow)
  {
    artype = parse_arrow(ch, arrow);
    if (!artype)
    {
      send_to_char("You don't know of that type of arrow.\r\n", ch);
      return eFAILURE;
    }
    switch (artype)
    {
    case 1:
      cost = 30;
      break;
    case 2:
      cost = 20;
      break;
    case 3:
      cost = 10;
      break;
    case 4:
      cost = 40;
      break;
    }
  }
  if (*direct)
  {
    if (direct[0] == 'n')
      dir = 0;
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
    else if (direct[0] == 'f' || direct[0] == 't' || direct[0] == 'g' || direct[0] == 'i')
      artype = parse_arrow(ch, direct);
    else
    {
      send_to_char("Shoot in which direction?\n\r", ch);
      return eFAILURE;
    }

    if (dir >= 0 && !CAN_GO(ch, dir))
    {
      send_to_char("There is nothing to shoot in that direction.\r\n",
                   ch);
      return eFAILURE;
    }
    else if (artype)
    {
      switch (artype)
      {
      case 1:
        cost = 30;
        break;
      case 2:
        cost = 20;
        break;
      case 3:
        cost = 10;
        break;
      case 4:
        cost = 40;
        break;
      }
    }
  }

  if ((GET_MANA(ch) < cost) && (GET_LEVEL(ch) < ANGEL))
  {
    send_to_char("You don't have enough mana for that arrow.\r\n", ch);
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_ARCHERY))
    return eSUCCESS;

  /* check if you can see your target */
  /* put ch in the targets room to check if they are visible */
  cur_room = ch->in_room;

  if (target[0])
  {
    if (!ch->fighting)
    {
      if (dir >= 0)
      {
        if (DC::getInstance()->world[cur_room].dir_option[dir] && !(DC::getInstance()->world[cur_room].dir_option[dir]->to_room == DC::NOWHERE) && !IS_SET(DC::getInstance()->world[cur_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
          new_room = DC::getInstance()->world[cur_room].dir_option[dir]->to_room;
          if (IS_SET(DC::getInstance()->world[new_room].room_flags, SAFE))
          {
            send_to_char(
                "Don't shoot into a safe room!  You might hit someone!\r\n",
                ch);
            return eFAILURE;
          }
          char_from_room(ch, false);
          if (!char_to_room(ch, new_room))
          {
            /* put ch into a room before we exit */
            char_to_room(ch, cur_room);
            send_to_char("Error moving you to room in do_fire\r\n",
                         ch);
            return eFAILURE;
          }
          victim = get_char_room_vis(ch, target);
        }
      }

      if (!victim && new_room && artype == 3 && dir >= 0)
      {
        if (DC::getInstance()->world[new_room].dir_option[dir] && !(DC::getInstance()->world[new_room].dir_option[dir]->to_room == DC::NOWHERE) && !IS_SET(DC::getInstance()->world[new_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
          new_room = DC::getInstance()->world[new_room].dir_option[dir]->to_room;
          char_from_room(ch, false);
          if (IS_SET(DC::getInstance()->world[new_room].room_flags, SAFE))
          {
            send_to_char(
                "Don't shoot into a safe room!  You might hit someone!\r\n",
                ch);
            char_to_room(ch, cur_room);
            return eFAILURE;
          }
          if (!char_to_room(ch, new_room))
          {
            /* put ch into a room before we exit */
            char_to_room(ch, cur_room);
            send_to_char("Error moving you to room in do_fire\r\n",
                         ch);
            return eFAILURE;
          }
          victim = get_char_room_vis(ch, target);
        }
      }

      if (!victim && new_room && artype == 3 && affected_by_spell(ch, SPELL_FARSIGHT) && dir >= 0)
      {
        if (DC::getInstance()->world[new_room].dir_option[dir] && !(DC::getInstance()->world[new_room].dir_option[dir]->to_room == DC::NOWHERE) && !IS_SET(DC::getInstance()->world[new_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
          new_room = DC::getInstance()->world[new_room].dir_option[dir]->to_room;
          char_from_room(ch, false);
          if (IS_SET(DC::getInstance()->world[new_room].room_flags, SAFE))
          {
            send_to_char(
                "Don't shoot into a safe room!  You might hit someone!\r\n",
                ch);
            char_to_room(ch, cur_room);
            return eFAILURE;
          }
          if (!char_to_room(ch, new_room))
          {
            /* put ch into a room before we exit */
            char_to_room(ch, cur_room);
            send_to_char("Error moving you to room in do_fire\r\n",
                         ch);
            return eFAILURE;
          }
          victim = get_char_room_vis(ch, target);
        }
      }
      /* put char back */
      char_from_room(ch, false);
      char_to_room(ch, cur_room);
    }
    if (!victim && has_skill(ch, SKILL_ARCHERY) > 80)
    {
      if ((victim = get_char_room_vis(ch, target)))
        dir = -1;
    }
    else if (dir < 0)
    {
      send_to_char(
          "You aren't skilled enough to fire at a target this close.\r\n",
          ch);
      return eFAILURE;
    }
    if (!victim)
    {
      if (dir >= 0)
        send_to_char(
            "You cannot concentrate enough to fire into an adjacent room while fighting.\r\n",
            ch);
      else
        send_to_char("You cannot seem to locate your target.\r\n", ch);
      return eFAILURE;
    }
  }
  else
  {
    send_to_char("Sorry, you must specify a target.\r\n", ch);
    return eFAILURE;
  }

  /* check for accidental targeting of self */
  if (victim == ch)
  {
    send_to_char("You need to type more of the target's name.\r\n", ch);
    return eFAILURE;
  }

  /* Protect the newbies! */
  if (IS_PC(victim) && GET_LEVEL(victim) < 6)
  {
    send_to_char("Don't shoot at a poor defenseless n00b! :(\r\n", ch);
    return eFAILURE;
  }
  /* check if target is fighting */
  /*  if(victim->fighting)
   {
   send_to_char("You can't seem to get a clear line of sight.\r\n", ch);
   return eFAILURE;
   }*/

  /* check for arrows here */

  found = nullptr;
  int where = 0;
  for (; where < MAX_WEAR; where++)
  {
    if (ch->equipment[where])
      if (IS_CONTAINER(ch->equipment[where]) && isname("quiver", ch->equipment[where]->name))
      {
        found = find_arrow(ch->equipment[where]);
        if (found)
        {
          get(ch, found, ch->equipment[where], 0, CMD_DEFAULT);
        }
        break;
      }
  }

  if (!found)
  {
    send_to_char("You aren't wearing any quivers with arrows!\r\n", ch);
    return eFAILURE;
  }

  if (IS_NPC(victim) && mob_index[victim->mobdata->nr].virt >= 2300 && mob_index[victim->mobdata->nr].virt <= 2399)
  {
    send_to_char(
        "Your arrow is disintegrated by the fortress' enchantments.\r\n",
        ch);
    extract_obj(found);
    return eFAILURE;
  }

  /* go ahead and shoot */

  switch (number(1, 2))
  {
  case 1:
    if (dir >= 0)
      sprintf(buf, "$n fires an arrow %sward.", dirs[dir]);
    else
      sprintf(buf, "$n fires an arrow.");
    act(buf, ch, 0, 0, TO_ROOM, 0);
    break;
  case 2:
    if (dir >= 0)
      sprintf(buf, "$n lets off an arrow to the %s.", dirs[dir]);
    else
      sprintf(buf, "$n lets off an arrow.");
    act(buf, ch, 0, 0, TO_ROOM, 0);
    break;
  }

  GET_MANA(ch) -= cost;

  if (!skill_success(ch, victim, SKILL_ARCHERY))
  {
    retval = eSUCCESS;
    do_arrow_miss(ch, victim, dir, found);
  }
  else
  {
    dam = dice(found->obj_flags.value[1], found->obj_flags.value[2]);
    dam += dice(ch->equipment[HOLD]->obj_flags.value[1],
                ch->equipment[HOLD]->obj_flags.value[2]);
    for (int i = 0; i < found->num_affects; i++)
      if (found->affected[i].location == APPLY_DAMROLL && found->affected[i].modifier != 0)
        dam += found->affected[i].modifier;
      else if (found->affected[i].location == APPLY_HIT_N_DAM && found->affected[i].modifier != 0)
        dam += found->affected[i].modifier;

    set_cantquit(ch, victim);
    sprintf(victname, "%s", GET_SHORT(victim));
    victroom = victim->in_room;
    strcpy(victhshr, HSHR(victim));

    if (dir >= 0)
      send_to_room(
          "An arrow flies into the room with incredible speed!\n\r",
          victroom);

    retval = damage(ch, victim, dam, TYPE_PIERCE, SKILL_ARCHERY, 0);

    if (IS_SET(retval, eVICT_DIED))
    {
      switch (number(1, 3))
      {
      case 1:
        if (dir < 0)
        {
          sprintf(buf, "The %s impales %s through the heart!\r\n",
                  found->short_description, victname);
          send_to_room(buf, victroom);
        }
        else
        {
          sprintf(buf, "Your shot impales %s through the heart!\r\n",
                  victname);
          send_to_char(buf, ch);
          sprintf(buf,
                  "%s from the %s impales %s through the chest!\r\n",
                  found->short_description, dirs[rev_dir[dir]],
                  victname);
          send_to_room(buf, victroom);
        }
        break;
      case 2:
        if (dir < 0)
        {
          sprintf(buf,
                  "%s drives through the eye of %s, ending %s life.\r\n",
                  found->short_description, victname, victhshr);
          send_to_room(buf, victroom);
        }
        else
        {
          sprintf(buf,
                  "Your %s drives through the eye of %s ending %s life.\r\n",
                  found->short_description, victname, victhshr);
          send_to_char(buf, ch);
          sprintf(buf,
                  "%s from the %s lands with a solid 'thunk.'\r\n%s falls to the ground, an arrow sticking from %s left eye.\r\n",
                  found->short_description, dirs[rev_dir[dir]],
                  victname, victhshr);
          send_to_room(buf, victroom);
        }
        break;
      case 3:
        if (dir < 0)
        {
          sprintf(buf,
                  "The %s rips through %s's throat.  Blood spouts as %s expires with a final gurgle.\r\n",
                  found->short_description, victname, HSSH(victim));
          send_to_room(buf, victroom);
        }
        else
        {
          sprintf(buf,
                  "Your shot rips through the throat of %s ending their life with a gurgle.\r\n",
                  victname);
          send_to_char(buf, ch);
          sprintf(buf,
                  "%s from the %s ripes through the throat of %s!  Blood spouts as %s expires with a final gurgle.\r\n",
                  found->short_description, dirs[rev_dir[dir]],
                  victname, HSSH(victim));
          send_to_room(buf, victroom);
        }
        break;
      }
    }
    else
    {
      if (dir < 0)
      {
        sprintf(buf, "You get shot with %s.  Ouch.",
                found->short_description);
        act(buf, victim, 0, 0, TO_CHAR, 0);
        sprintf(buf, "%s hits $n!", found->short_description);
        act(buf, victim, 0, ch, TO_ROOM, NOTVICT);
        sprintf(buf, "You hit %s with %s!\r\n", GET_SHORT(victim),
                found->short_description);
        send_to_char(buf, ch);
      }
      else
      {
        sprintf(buf, "You hit %s with %s!\r\n", GET_SHORT(victim),
                found->short_description);
        send_to_char(buf, ch);
        sprintf(buf, "You get shot with %s from the %s.  Ouch.",
                found->short_description, dirs[rev_dir[dir]]);
        act(buf, victim, 0, 0, TO_CHAR, 0);
        sprintf(buf, "%s from the %s hits $n!",
                found->short_description, dirs[rev_dir[dir]]);
        act(buf, victim, 0, 0, TO_ROOM, 0);
      }
      GET_POS(victim) = POSITION_STANDING;

      if (IS_NPC(victim))
        retval = mob_arrow_response(ch, victim, dir);
      if (SOMEONE_DIED(retval))
      { // mob died somehow while moving
        extract_obj(found);
        return retval;
      }
    }

    if (!SOMEONE_DIED(retval))
    {
      cur_room = ch->in_room;
      char_from_room(ch, false);
      if (!char_to_room(ch, victim->in_room))
      {
        char_to_room(ch, cur_room);
        send_to_char("Error moving you to room in do_fire.\r\n", ch);
        return eFAILURE;
      }
      retval = weapon_spells(ch, victim, ITEM_MISSILE);
      // just in case
      if (IS_SET(retval, eCH_DIED))
      {
        Object *corpse, *next;
        for (corpse = object_list; corpse; corpse = next)
        {
          next = corpse->next;
          if (IS_OBJ_STAT(corpse, ITEM_PC_CORPSE) && isname(GET_NAME(ch), GET_OBJ_NAME(corpse)))
          {
            obj_from_room(corpse);
            obj_to_room(corpse, cur_room);
            save_corpses();
            break;
          }
        }
      }
      else
      {
        char_from_room(ch, false);
        char_to_room(ch, cur_room);
      }
    }

    char buffer[100];
    if (!SOMEONE_DIED(retval))
    {
      switch (artype)
      {
      case 1:
        dam = 90;
        snprintf(buffer, 100, "%d", dam);
        send_damage(
            "The flames surrounding the arrow burns your wound for | damage!",
            ch, 0, victim, buffer,
            "The flames surrounding the arrow burns your wound!",
            TO_VICT);
        send_damage(
            "The flames surrounding the arrow burns $n's wound for | damage!",
            victim, 0, 0, buffer,
            "The flames surrounding the arrow burns $n's wound!",
            TO_ROOM);
        retval = damage(ch, victim, dam, TYPE_FIRE, SKILL_FIRE_ARROW,
                        0);
        skill_increase_check(ch, SKILL_FIRE_ARROW,
                             has_skill(ch, SKILL_FIRE_ARROW),
                             get_difficulty(SKILL_FIRE_ARROW));
        enchantmentused = true;
        break;
      case 2:
        dam = 50;
        snprintf(buffer, 100, "%d", dam);
        send_damage("The stray ice shards impale you for | damage!", ch,
                    0, victim, buffer, "The stray ice shards impale you!",
                    TO_VICT);
        send_damage("The stray ice shards impale $n for | damage!",
                    victim, 0, 0, buffer, "The stray ice shards impale $n!",
                    TO_ROOM);
        if (number(1, 100) < has_skill(ch, SKILL_ICE_ARROW) / 4)
        {
          act("Your body slows down for a second!", ch, 0, victim,
              TO_VICT, 0);
          act("$n's body seems a bit slower!", victim, 0, 0, TO_ROOM,
              0);
          WAIT_STATE(victim, DC::PULSE_VIOLENCE);
        }
        retval = damage(ch, victim, dam, TYPE_COLD, SKILL_ICE_ARROW, 0);
        skill_increase_check(ch, SKILL_ICE_ARROW,
                             has_skill(ch, SKILL_ICE_ARROW),
                             get_difficulty(SKILL_ICE_ARROW));
        enchantmentused = true;
        break;
      case 3:
        dam = 30;
        snprintf(buffer, 100, "%d", dam);
        send_damage(
            "The storm cloud enveloping the arrow shocks you for | damage!",
            ch, 0, victim, buffer,
            "The storm cloud enveloping the arrow shocks you!",
            TO_VICT);
        send_damage(
            "The storm cloud enveloping the arrow shocks $n for | damage!",
            victim, 0, ch, buffer,
            "The storm cloud enveloping the arrow shocks $n!",
            TO_ROOM);
        retval = damage(ch, victim, dam, TYPE_ENERGY,
                        SKILL_TEMPEST_ARROW, 0);
        skill_increase_check(ch, SKILL_TEMPEST_ARROW,
                             has_skill(ch, SKILL_TEMPEST_ARROW),
                             get_difficulty(SKILL_TEMPEST_ARROW));
        enchantmentused = true;
        break;
      case 4:
        dam = 70;
        snprintf(buffer, 100, "%d", dam);
        send_damage(
            "The magical stones surrounding the arrow smack into you, hard for | damage.",
            ch, 0, victim, buffer,
            "The magical stones surrounding the arrow smack into you, hard.",
            TO_VICT);
        send_damage(
            "The magical stones surrounding the arrow smack hard into $n for | damage.",
            victim, 0, 0, buffer,
            "The magical stones surrounding the arrow smack hard into $n.",
            TO_ROOM);
        if (number(1, 100) < has_skill(ch, SKILL_GRANITE_ARROW) / 4)
        {
          act("The stones knock you flat on your ass!", ch, 0, victim,
              TO_VICT, 0);
          act("The stones knock $n flat on $s ass!", victim, 0, 0,
              TO_ROOM, 0);
          GET_POS(victim) = POSITION_SITTING;
          WAIT_STATE(victim, DC::PULSE_VIOLENCE);
        }
        retval = damage(ch, victim, dam, TYPE_HIT, SKILL_GRANITE_ARROW,
                        0);
        skill_increase_check(ch, SKILL_GRANITE_ARROW,
                             has_skill(ch, SKILL_GRANITE_ARROW),
                             get_difficulty(SKILL_GRANITE_ARROW));
        enchantmentused = true;
        break;
      default:
        break;
      }
    }
  }

  extract_obj(found);

  DC::getInstance()->shooting_list.insert(ch);

  if (has_skill(ch, SKILL_ARCHERY) < 51 || enchantmentused)
    ch->shotsthisround += DC::PULSE_VIOLENCE;
  else if (has_skill(ch, SKILL_ARCHERY) < 86)
    ch->shotsthisround += DC::PULSE_VIOLENCE / 2;
  else
    ch->shotsthisround += 3;

  return retval;
}

int do_mind_delve(Character *ch, char *arg, int cmd)
{
  char buf[1000];
  Character *target = nullptr;
  //  int learned, specialization;

  if (!*arg)
  {
    send_to_char("Delve into whom?\r\n", ch);
    return eFAILURE;
  }

  one_argument(arg, arg);

  /*
    TODO - make this into a skill and put it in

    if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
       learned = 75;
    else if(!(learned = has_skill(ch, SKILL_MIND_DELVE))) {
       send_to_char("You try to think like a chipmunk and go nuts.\r\n", ch);
       return eFAILURE;
    }
    specialization = learned / 100;
    learned = learned % 100;

  */

  target = get_char_room_vis(ch, arg);

  if (GET_LEVEL(ch) < GET_LEVEL(target))
  {
    send_to_char("You can't seem to understand your target's mental processes.\r\n", ch);
    return eFAILURE;
  }

  if (!IS_MOB(target))
  {
    sprintf(buf, "Ewwwww gross!!!  %s is imagining you naked on all fours!\r\n", GET_SHORT(target));
    send_to_char(buf, ch);
    return eFAILURE;
  }

  act("You enter $S mind...", ch, 0, target, TO_CHAR, INVIS_NULL);
  sprintf(buf, "%s seems to hate... %s.\r\n", GET_SHORT(target),
          ch->mobdata->hatred ? ch->mobdata->hatred : "Noone!");
  send_to_char(buf, ch);
  if (ch->master)
    sprintf(buf, "%s seems to really like... %s.\r\n", GET_SHORT(target),
            GET_SHORT(ch->master));
  else
    sprintf(buf, "%s seems to really like... %s.\r\n", GET_SHORT(target),
            "Noone!");
  send_to_char(buf, ch);
  return eSUCCESS;
}

void check_eq(Character *ch)
{
  int pos;

  for (pos = 0; pos < MAX_WEAR; pos++)
  {
    if (ch->equipment[pos])
      equip_char(ch, unequip_char(ch, pos), pos);
  }
}

int do_natural_selection(Character *ch, char *arg, int cmd)
{
  int i;
  char buf[MAX_STRING_LENGTH];
  struct affected_type af, *cur;

  one_argument(arg, buf);

  int learned = has_skill(ch, SKILL_NAT_SELECT);
  if (IS_NPC(ch) || !learned)
  {
    send_to_char("You don't know how to use this to your advantage.\r\n", ch);
    return eFAILURE;
  }

  if (affected_by_spell(ch, SPELL_NAT_SELECT_TIMER))
  {
    send_to_char("You cannot yet select a new enemy of choice.\r\n", ch);
    return eFAILURE;
  }

  cur = affected_by_spell(ch, SKILL_NAT_SELECT);

  for (i = 1; i < 33; i++)
  {
    if (is_abbrev(buf, races[i].singular_name))
    {
      if (cur && cur->modifier == i)
      {
        send_to_char("You are already studying this race.\r\n", ch);
        return eFAILURE;
      }
      break;
    }
    if (i == 32)
    {
      send_to_char("Please select a valid race.\r\n", ch);
      return eFAILURE;
    }
  }

  if (cur)
    affect_remove(ch, cur, SUPPRESS_ALL);

  af.type = SKILL_NAT_SELECT;
  af.duration = -1;
  af.modifier = i;
  af.location = 0;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  af.type = SPELL_NAT_SELECT_TIMER;
  af.duration = 60 - learned / 5;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  csendf(ch, "You study the habits of the %s race and select them as your enemy of choice.\r\n", races[i].singular_name);

  return eSUCCESS;
}
