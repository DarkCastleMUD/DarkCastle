/******************************************************************************
 * $Id: cl_ranger.cpp,v 1.95 2015/05/30 04:48:42 pirahna Exp $ | cl_ranger.C  *
 * Description: Ranger skills/spells                                          *
 *                                                                            *
 * Revision History                                                           *
 * 10/28/2003  Onager   Modified do_tame() to remove hate flag for tamer      *
 * 12/08/2003  Onager   Added eq check to do_tame() to remove !charmie eq     *
 *                      from charmies                                         *
 ******************************************************************************/

#include <cstring>
#include <cstdio>

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
  int i = ch->getLevel() / 5;
  int z = 3;
  struct follow_type *f;
  for (f = ch->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_CHARM))
    {
      z--;
      i -= charm_space(f->follower->getLevel());
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

  if (!ch->has_skill(SKILL_FREE_ANIMAL))
  {
    ch->sendln("Try learning HOW to free an animal first.");
    return eFAILURE;
  }

  arg = one_argument(arg, buf);

  for (struct follow_type *k = ch->followers; k; k = k->next)
    if (IS_MOB(k->follower) && ISSET(k->follower->affected_by, AFF_CHARM) && isexact(buf, GET_NAME(k->follower)))
    {
      victim = k->follower;
      break;
    }

  if (!victim)
  {
    ch->sendln("They aren't even here!");
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
    ch->sendln("Who do you want to tame?");
    return eFAILURE;
  }

  if (!ch->canPerform(SKILL_TAME, "Try learning HOW to tame first.\r\n"))
  {
    return eFAILURE;
  }

  one_argument(arg, buf);

  if (!(victim = ch->get_char_room_vis(buf)))
  {
    ch->sendln("No one here by that name!");
    return eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Tame the wild beast!");
    return eFAILURE;
  }

  if (IS_PC(victim))
  {
    ch->sendln("You find yourself unable to tame this player.");
    return eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_CHARM) || IS_AFFECTED(ch, AFF_CHARM) ||
      (ch->getLevel() < victim->getLevel()))
  {
    ch->sendln("You find yourself unable to tame this creature.");
    return eFAILURE;
  }

  if (circle_follow(victim, ch))
  {
    ch->sendln("Sorry, following in circles can not be allowed.");
    return eFAILURE;
  }

  if (charm_levels(ch) - charm_space(victim->getLevel()) < 0)
  {
    ch->sendln("How you plan on controlling so many followers?");
    return eFAILURE;
    /*   Character * vict = nullptr;
       for(struct follow_type *k = ch->followers; k; k = k->next)
         if(IS_MOB(k->follower) &&k->follower->affected_by_spell( SPELL_CHARM_PERSON))
         {
            vict = k->follower;
            break;
         }
         if (vict) {
      if (vict->in_room == ch->in_room && vict->position > position_t::SLEEPING)
        do_say(vict, "Hey... but what about ME!?", CMD_DEFAULT);
             remove_memory(vict, 'h');
      if (vict->master) {
             stop_follower(vict, BROKE_CHARM);
             vict->add_memory( GET_NAME(ch), 'h');
      }
         }*/
  }

  if (!charge_moves(ch, SKILL_TAME))
    return eSUCCESS;

  act("$n holds out $s hand to $N and beckons softly.", ch, nullptr, victim, TO_ROOM, INVIS_NULL);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1);

  if ((isSet(victim->immune, ISR_CHARM)) ||
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

command_return_t Character::do_track(QStringList arguments, int cmd)
{
  int x, y;
  int retval, how_deep, learned;
  char buf[300];
  char race[40];
  char sex[20];
  char condition[60];
  char weight[40];

  Character *quarry;
  Character *tmp_ch = nullptr; // For checking room stuff
  room_track_data *pScent = 0;

  QString victim = arguments.value(0);

  learned = how_deep = ((has_skill(SKILL_TRACK) / 10) + 1);

  if (this->getLevel() >= IMMORTAL)
    how_deep = 50;

  quarry = get_char_room_vis(victim);

  if (!hunting.isEmpty())
  {
    if (get_char_room_vis(hunting))
    {
      ansi_color(RED, this);
      ansi_color(BOLD, this);
      this->sendln("You have found your target!");
      ansi_color(NTEXT, this);

      //      remove_memory(this, 't');
      return eSUCCESS;
    }
  }

  else if (quarry)
  {
    this->sendln("There's one right here ;)");
    //  remove_memory(this, 't');
    return eSUCCESS;
  }

  if (!victim.isEmpty() && IS_PC(this) && GET_CLASS(this) != CLASS_RANGER && GET_CLASS(this) != CLASS_DRUID && this->getLevel() < ANGEL)
  {
    this->sendln("Only a ranger could track someone by name.");
    return eFAILURE;
  }

  if (!charge_moves(SKILL_TRACK))
    return eSUCCESS;

  act("$n walks about slowly, searching for signs of $s quarry", this, 0, 0, TO_ROOM, INVIS_NULL);
  this->sendln("You search for signs of your quarry...\n\r");

  if (learned)
    skill_increase_check(SKILL_TRACK, learned, SKILL_INCREASE_MEDIUM);

  // TODO - once we're sure that act_mob is properly checking for this,
  // and that it isn't call from anywhere else, we can probably remove it.
  // That way possessing imms can track.
  if (IS_MOB(this) && ISSET(this->mobdata->actflags, ACT_STUPID))
  {
    this->sendln("Being stupid, you cannot find any..");
    return eFAILURE;
  }

  if (DC::getInstance()->world[this->in_room].nTracks < 1)
  {
    if (!hunting.isEmpty())
    {
      ansi_color(RED, this);
      ansi_color(BOLD, this);
      this->sendln("You have lost the trail.");
      ansi_color(NTEXT, this);
      // remove_memory(this, 't');
    }
    else
      this->sendln("There are no distinct scents here.");
    return eFAILURE;
  }

  if (IS_NPC(this))
    how_deep = 10;

  if (!victim.isEmpty())
  {
    for (x = 1; x <= how_deep; x++)
    {

      if ((x > DC::getInstance()->world[this->in_room].nTracks) ||
          !(pScent = DC::getInstance()->world[this->in_room].TrackItem(x)))
      {
        if (!hunting.isEmpty())
        {
          ansi_color(RED, this);
          ansi_color(BOLD, this);
          this->sendln("You have lost the trail.");
          ansi_color(NTEXT, this);
        }
        else
          this->sendln("You can't find any traces of such a scent.");
        //         remove_memory(this, 't');
        if (IS_NPC(this))
          swap_hate_memory();
        return eFAILURE;
      }

      if (isexact(victim, pScent->trackee))
      {
        y = pScent->direction;
        this->add_memory(pScent->trackee, 't');
        ansi_color(RED, this);
        ansi_color(BOLD, this);
        csendf(this, "You sense traces of your quarry to the %s.\r\n",
               dirs[y]);
        ansi_color(NTEXT, this);

        if (IS_NPC(this))
        {
          // temp disable tracking mobs into town
          if (DC::getInstance()->zones.value(DC::getInstance()->world[EXIT(this, y)->to_room].zone).isTown() == false && !isSet(DC::getInstance()->world[EXIT(this, y)->to_room].room_flags, NO_TRACK))
          {
            this->mobdata->last_direction = y;
            retval = do_move(this, "", (y + 1));
            if (isSet(retval, eCH_DIED))
              return retval;
          }

          if (hunting.isEmpty())
            return eFAILURE;

          // Here's the deal: if the mob can't see the character in
          // the room, but the character IS in the room, then the
          // mob can't see the character and we need to stop tracking.
          // It does, however, leave the mob open to be taken apart
          // by, say, a thief.  I'll let he who wrote it fix that.
          // Morc 28 July 96

          if ((tmp_ch = get_char(hunting)) == 0)
            return eFAILURE;
          if (!(get_char_room_vis(hunting)))
          {
            if (tmp_ch->in_room == this->in_room)
            {
              // The mob can't see him
              act("$n says 'Damn, must have lost $M!'", this, 0, tmp_ch,
                  TO_ROOM, 0);
              //                    remove_memory(this, 't');
            }
            return eFAILURE;
          }

          if (!isSet(DC::getInstance()->world[this->in_room].room_flags, SAFE))
          {
            act("$n screams 'YOU CAN RUN, BUT YOU CAN'T HIDE!'",
                this, 0, 0, TO_ROOM, 0);
            retval = eSUCCESS;
            if (tmp_ch)
            {
              retval = mprog_attack_trigger(this, tmp_ch);
            }
            if (SOMEONE_DIED(retval) || (this && this->fighting) || !!hunting.isEmpty())
              return retval;
            else
              return do_hit(hunting.split(' '));
          }
          else
            act("$n says 'You can't stay here forever.'",
                this, 0, 0, TO_ROOM, 0);
        } // if IS_NPC

        return eSUCCESS;
      } // if isname
    } // for

    if (!hunting.isEmpty())
    {
      ansi_color(RED, this);
      ansi_color(BOLD, this);
      this->sendln("You have lost the trail.");
      ansi_color(NTEXT, this);
    }
    else
      this->sendln("You can't find any traces of such a scent.");

    //    remove_memory(this, 't');
    return eFAILURE;
  } // if *victim

  for (x = 1; x <= how_deep; x++)
  {
    if ((x > DC::getInstance()->world[this->in_room].nTracks) || !(pScent = DC::getInstance()->world[this->in_room].TrackItem(x)))
    {
      if (x == 1)
        this->sendln("There are no distinct smells here.");
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
      this->sendln("Freshest scents first...");

    sprintf(buf, "The scent of a%s%s%s%s leads %s.\r\n",
            weight,
            condition,
            sex,
            race,
            dirs[y]);
    this->send(buf);
  }
  return eSUCCESS;
}

command_return_t Character::do_ambush(QStringList arguments, int cmd)
{
  if (!canPerform(SKILL_AMBUSH))
  {
    sendln("You don't know how to ambush people!");
    return eFAILURE;
  }

  QString arg1 = arguments.value(0);

  if (arg1.isEmpty())
  {
    sendln(QStringLiteral("You will ambush %1 on sight.").arg(ambush.isEmpty() ? "no one" : ambush));
    return eSUCCESS;
  }

  if (ambush == arg1)
  {
    ambush.clear();
    sendln(QStringLiteral("You will no longer ambush %1 on sight.").arg(arg1));
    return eSUCCESS;
  }

  ambush = arg1;
  sendln(QStringLiteral("You will now ambush %1 on sight.").arg(arg1));
  return eSUCCESS;
}

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

  if (ch->affected_by_spell(SKILL_FORAGE))
  {
    if (number(0, 1) == 0)
      ch->sendln("You already foraged recently.  Give mother nature a break!");
    else
      ch->sendln("There's a limit to how often you can play with your nuts.  Give it some time.");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_FORAGE))
    return eSUCCESS;

  learned = ch->has_skill(SKILL_FORAGE);
  if (!learned)
  {
    ch->sendln("Not knowing how to forage, you poke at the dirt with a stick, finding nothing.");
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
  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, CLAN_ROOM) ||
      isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) ||
      isSet(DC::getInstance()->world[ch->in_room].room_flags, INDOORS))
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
        ch->sendln("You can't carry that many items!");
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

  ch->skill_increase_check(SKILL_FORAGE, learned, SKILL_INCREASE_HARD);
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

/* this is sent the arrow char std::string, and return the arrow type.
   also, checks level to make sure char is high enough
   return 0 for failure */

int parse_arrow(Character *ch, char *arrow)
{

  if (GET_CLASS(ch) != CLASS_RANGER && ch->getLevel() < 100)
    return 0;

  while (*arrow == ' ')
    arrow++;

  if ((arrow[0] == 'f') && ch->has_skill(SKILL_FIRE_ARROW))
    return 1;
  else if ((arrow[0] == 'i') && ch->has_skill(SKILL_ICE_ARROW))
    return 2;
  else if ((arrow[0] == 't') && ch->has_skill(SKILL_TEMPEST_ARROW))
    return 3;
  else if ((arrow[0] == 'g') && ch->has_skill(SKILL_GRANITE_ARROW))
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
    ch->sendln("You miss your shot.");
    break;
  case 2:
    ch->sendln("Your arrow wizzes by the target harmlessely.");
    break;
  case 3:
    ch->sendln("Your pitiful aim spears a poor woodland creature instead..");
    break;
  case 4:
    ch->sendln("Your shot misses your target, and goes skittering across the ground.");
    break;
  case 5:
    ch->sendln("A slight breeze forces your arrow off the mark.");
    break;
  case 6:
    ch->sendln("Your shot narrowly misses the mark.");
    break;
  }

  switch (number(1, 3))
  {
  case 1:
    if (dir < 0)
    {
      sprintf(buf, "%s wizzes by.\r\n", found->short_description);
      victim->send(buf);
      sprintf(buf, "%s wizzes by.", found->short_description);
      act(buf, victim, nullptr, ch, TO_ROOM, NOTVICT);
    }
    else
    {
      sprintf(buf, "%s wizzes by from the %s.\r\n", found->short_description, dirs[rev_dir[dir]]);
      victim->send(buf);
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
      victim->send(buf);
      sprintf(buf, "%s narrowly misses $n.", found->short_description);
      act(buf, victim, nullptr, ch, TO_ROOM, NOTVICT);
    }
    else
    {
      sprintf(buf, "%s from the %s narrowly misses your head.\r\n",
              found->short_description, dirs[rev_dir[dir]]);
      victim->send(buf);
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

  victim->add_memory(GET_NAME(ch), 'h');

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
              !isSet(DC::getInstance()->world[EXIT(victim, dir2)->to_room].room_flags, NO_TRACK))
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
          if (!isSet(DC::getInstance()->world[EXIT(victim, dir2)->to_room].room_flags, NO_TRACK))
          {
            /* dir+1 it since attempt_move will --cmd it */
            retval = attempt_move(victim, (dir2 + 1), 0);
            if (SOMEONE_DIED(retval))
              return retval;

            if (isSet(retval, eFAILURE)) // can't go after the archer
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
    ch->send(buf);
    sprintf(buf, "%s from the %s drives full force into your chest!\r\n",
       found->short_description, dirs[rev_dir[dir]]);
    victim->send(buf);
    sprintf(buf, "%s from the %s impales $n through the chest!",
       found->short_description, dirs[rev_dir[dir]]);
    act(buf, victim, nullptr, 0, TO_ROOM, 0);
    break;

    case 2:
    sprintf(buf, "Your %s drives through the eye of %s ending their life.\r\n",
       found->short_description, GET_SHORT(victim));
    ch->send(buf);
    sprintf(buf, "%s drives right through your left eye!\r\nThe last thing through your mind is.........an arrowhead.\r\n",
       found->short_description);
    victim->send(buf);
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
    ch->send(buf);
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
     victim->setStanding();

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

  if (!ch->canPerform(SKILL_ARCHERY))
  {
    ch->sendln("You've no idea how those pointy things with strings and feathers work.");
    return eFAILURE;
  }

  if (!ch->equipment[HOLD])
  {
    ch->sendln("You need to be holding a bow, moron.");
    return eFAILURE;
  }

  if (!(ch->equipment[HOLD]->obj_flags.type_flag == ITEM_FIREWEAPON))
  {
    ch->sendln("You need to be holding a bow, moron.");
    return eFAILURE;
  }
  /*
   if(GET_POS(ch) == position_t::FIGHTING)
   {
   ch->sendln("Aren't you a bit busy with hand to hand combat?");
   return eFAILURE;
   }*/

  if (ch->shotsthisround > 0)
  {
    ch->sendln("Slow down there tiger, you can't fire them that fast!");
    return eFAILURE;
  }

  /*  Command syntax is: fire [target] <dir> [arrowtype]
   if there is !dir, then check skill level
   if there is !arrow, then choose standard arrowtype */

  while (*arg == ' ')
    arg++;

  if (!*arg)
  {
    ch->sendln("Shoot at whom?");
    return eFAILURE;
  }
  half_chop(arg, target, buf2);
  half_chop(buf2, direct, arrow);

  direct[MAX_STRING_LENGTH - 1] = '\0';
  arrow[MAX_STRING_LENGTH - 1] = '\0';
  target[MAX_STRING_LENGTH - 1] = '\0';

  /* make safe rooms checks */
  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You can't shoot arrows if yer in a safe room silly.");
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
      ch->sendln("You don't know of that type of arrow.");
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
      ch->sendln("Shoot in which direction?");
      return eFAILURE;
    }

    if (dir >= 0 && !CAN_GO(ch, dir))
    {
      ch->sendln("There is nothing to shoot in that direction.");
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

  if ((GET_MANA(ch) < cost) && (ch->getLevel() < ANGEL))
  {
    ch->sendln("You don't have enough mana for that arrow.");
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
        if (DC::getInstance()->world[cur_room].dir_option[dir] && !(DC::getInstance()->world[cur_room].dir_option[dir]->to_room == DC::NOWHERE) && !isSet(DC::getInstance()->world[cur_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
          new_room = DC::getInstance()->world[cur_room].dir_option[dir]->to_room;
          if (isSet(DC::getInstance()->world[new_room].room_flags, SAFE))
          {
            ch->sendln("Don't shoot into a safe room!  You might hit someone!");
            return eFAILURE;
          }
          char_from_room(ch, false);
          if (!char_to_room(ch, new_room))
          {
            /* put ch into a room before we exit */
            char_to_room(ch, cur_room);
            ch->sendln("Error moving you to room in do_fire");
            return eFAILURE;
          }
          victim = ch->get_char_room_vis(target);
        }
      }

      if (!victim && new_room && artype == 3 && dir >= 0)
      {
        if (DC::getInstance()->world[new_room].dir_option[dir] && !(DC::getInstance()->world[new_room].dir_option[dir]->to_room == DC::NOWHERE) && !isSet(DC::getInstance()->world[new_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
          new_room = DC::getInstance()->world[new_room].dir_option[dir]->to_room;
          char_from_room(ch, false);
          if (isSet(DC::getInstance()->world[new_room].room_flags, SAFE))
          {
            ch->sendln("Don't shoot into a safe room!  You might hit someone!");
            char_to_room(ch, cur_room);
            return eFAILURE;
          }
          if (!char_to_room(ch, new_room))
          {
            /* put ch into a room before we exit */
            char_to_room(ch, cur_room);
            ch->sendln("Error moving you to room in do_fire");
            return eFAILURE;
          }
          victim = ch->get_char_room_vis(target);
        }
      }

      if (!victim && new_room && artype == 3 && ch->affected_by_spell(SPELL_FARSIGHT) && dir >= 0)
      {
        if (DC::getInstance()->world[new_room].dir_option[dir] && !(DC::getInstance()->world[new_room].dir_option[dir]->to_room == DC::NOWHERE) && !isSet(DC::getInstance()->world[new_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
          new_room = DC::getInstance()->world[new_room].dir_option[dir]->to_room;
          char_from_room(ch, false);
          if (isSet(DC::getInstance()->world[new_room].room_flags, SAFE))
          {
            ch->sendln("Don't shoot into a safe room!  You might hit someone!");
            char_to_room(ch, cur_room);
            return eFAILURE;
          }
          if (!char_to_room(ch, new_room))
          {
            /* put ch into a room before we exit */
            char_to_room(ch, cur_room);
            ch->sendln("Error moving you to room in do_fire");
            return eFAILURE;
          }
          victim = ch->get_char_room_vis(target);
        }
      }
      /* put char back */
      char_from_room(ch, false);
      char_to_room(ch, cur_room);
    }
    if (!victim && ch->has_skill(SKILL_ARCHERY) > 80)
    {
      if ((victim = ch->get_char_room_vis(target)))
        dir = -1;
    }
    else if (dir < 0)
    {
      ch->sendln("You aren't skilled enough to fire at a target this close.");
      return eFAILURE;
    }
    if (!victim)
    {
      if (dir >= 0)
        ch->sendln("You cannot concentrate enough to fire into an adjacent room while fighting.");
      else
        ch->sendln("You cannot seem to locate your target.");
      return eFAILURE;
    }
  }
  else
  {
    ch->sendln("Sorry, you must specify a target.");
    return eFAILURE;
  }

  /* check for accidental targeting of self */
  if (victim == ch)
  {
    ch->sendln("You need to type more of the target's name.");
    return eFAILURE;
  }

  /* Protect the newbies! */
  if (IS_PC(victim) && victim->getLevel() < 6)
  {
    ch->sendln("Don't shoot at a poor defenseless n00b! :(");
    return eFAILURE;
  }
  /* check if target is fighting */
  /*  if(victim->fighting)
   {
   ch->sendln("You can't seem to get a clear line of sight.");
   return eFAILURE;
   }*/

  /* check for arrows here */

  found = nullptr;
  int where = 0;
  for (; where < MAX_WEAR; where++)
  {
    if (ch->equipment[where])
      if (IS_CONTAINER(ch->equipment[where]) && isexact("quiver", ch->equipment[where]->name))
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
    ch->sendln("You aren't wearing any quivers with arrows!");
    return eFAILURE;
  }

  if (IS_NPC(victim) && DC::getInstance()->mob_index[victim->mobdata->nr].virt >= 2300 && DC::getInstance()->mob_index[victim->mobdata->nr].virt <= 2399)
  {
    ch->sendln("Your arrow is disintegrated by the fortress' enchantments.");
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

    if (isSet(retval, eVICT_DIED))
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
          ch->send(buf);
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
          ch->send(buf);
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
          ch->send(buf);
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
        ch->send(buf);
      }
      else
      {
        sprintf(buf, "You hit %s with %s!\r\n", GET_SHORT(victim),
                found->short_description);
        ch->send(buf);
        sprintf(buf, "You get shot with %s from the %s.  Ouch.",
                found->short_description, dirs[rev_dir[dir]]);
        act(buf, victim, 0, 0, TO_CHAR, 0);
        sprintf(buf, "%s from the %s hits $n!",
                found->short_description, dirs[rev_dir[dir]]);
        act(buf, victim, 0, 0, TO_ROOM, 0);
      }
      victim->setStanding();

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
        ch->sendln("Error moving you to room in do_fire.");
        return eFAILURE;
      }
      retval = weapon_spells(ch, victim, ITEM_MISSILE);
      // just in case
      if (isSet(retval, eCH_DIED))
      {
        Object *corpse, *next;
        for (corpse = object_list; corpse; corpse = next)
        {
          next = corpse->next;
          if (IS_OBJ_STAT(corpse, ITEM_PC_CORPSE) && isexact(GET_NAME(ch), GET_OBJ_NAME(corpse)))
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
        ch->skill_increase_check(SKILL_FIRE_ARROW,
                                 ch->has_skill(SKILL_FIRE_ARROW),
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
        if (number(1, 100) < ch->has_skill(SKILL_ICE_ARROW) / 4)
        {
          act("Your body slows down for a second!", ch, 0, victim,
              TO_VICT, 0);
          act("$n's body seems a bit slower!", victim, 0, 0, TO_ROOM,
              0);
          WAIT_STATE(victim, DC::PULSE_VIOLENCE);
        }
        retval = damage(ch, victim, dam, TYPE_COLD, SKILL_ICE_ARROW, 0);
        ch->skill_increase_check(SKILL_ICE_ARROW,
                                 ch->has_skill(SKILL_ICE_ARROW),
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
        ch->skill_increase_check(SKILL_TEMPEST_ARROW,
                                 ch->has_skill(SKILL_TEMPEST_ARROW),
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
        if (number(1, 100) < ch->has_skill(SKILL_GRANITE_ARROW) / 4)
        {
          act("The stones knock you flat on your ass!", ch, 0, victim,
              TO_VICT, 0);
          act("The stones knock $n flat on $s ass!", victim, 0, 0,
              TO_ROOM, 0);
          victim->setSitting();
          WAIT_STATE(victim, DC::PULSE_VIOLENCE);
        }
        retval = damage(ch, victim, dam, TYPE_HIT, SKILL_GRANITE_ARROW,
                        0);
        ch->skill_increase_check(SKILL_GRANITE_ARROW,
                                 ch->has_skill(SKILL_GRANITE_ARROW),
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

  if (ch->has_skill(SKILL_ARCHERY) < 51 || enchantmentused)
    ch->shotsthisround += DC::PULSE_VIOLENCE;
  else if (ch->has_skill(SKILL_ARCHERY) < 86)
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
    ch->sendln("Delve into whom?");
    return eFAILURE;
  }

  one_argument(arg, arg);

  /*
    TODO - make this into a skill and put it in

    if(IS_MOB(ch) || ch->getLevel() >= ARCHANGEL)
       learned = 75;
    else if(!(learned = ch->has_skill( SKILL_MIND_DELVE))) {
       ch->sendln("You try to think like a chipmunk and go nuts.");
       return eFAILURE;
    }
    specialization = learned / 100;
    learned = learned % 100;

  */

  target = ch->get_char_room_vis(arg);

  if (ch->getLevel() < target->getLevel())
  {
    ch->sendln("You can't seem to understand your target's mental processes.");
    return eFAILURE;
  }

  if (!IS_MOB(target))
  {
    sprintf(buf, "Ewwwww gross!!!  %s is imagining you naked on all fours!\r\n", GET_SHORT(target));
    ch->send(buf);
    return eFAILURE;
  }

  act("You enter $S mind...", ch, 0, target, TO_CHAR, INVIS_NULL);
  ch->sendln(QStringLiteral("%1 seems to hate... %2.").arg(GET_SHORT(target)).arg(ch->mobdata->hated.isEmpty() ? "Noone!" : ch->mobdata->hated));

  if (ch->master)
    sprintf(buf, "%s seems to really like... %s.\r\n", GET_SHORT(target),
            GET_SHORT(ch->master));
  else
    sprintf(buf, "%s seems to really like... %s.\r\n", GET_SHORT(target),
            "Noone!");
  ch->send(buf);
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

  int learned = ch->has_skill(SKILL_NAT_SELECT);
  if (IS_NPC(ch) || !learned)
  {
    ch->sendln("You don't know how to use this to your advantage.");
    return eFAILURE;
  }

  if (ch->affected_by_spell(SPELL_NAT_SELECT_TIMER))
  {
    ch->sendln("You cannot yet select a new enemy of choice.");
    return eFAILURE;
  }

  cur = ch->affected_by_spell(SKILL_NAT_SELECT);

  for (i = 1; i < 33; i++)
  {
    if (is_abbrev(buf, races[i].singular_name))
    {
      if (cur && cur->modifier == i)
      {
        ch->sendln("You are already studying this race.");
        return eFAILURE;
      }
      break;
    }
    if (i == 32)
    {
      ch->sendln("Please select a valid race.");
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
