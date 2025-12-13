/***************************************************************************
 *  file: mob_act.c , Mobile action module.                Part of DIKUMUD *
 *  Usage: Procedures generating 'intelligent' behavior in the mobiles.    *
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
/**************************************************************************/
/* Revision History                                                       */
/* 12/05/2003   Onager   Created is_protected() to break out PFE/PFG code */
/* 12/05/2003   Onager   Created scavenge() to simplify mobile_activity() */
/* 12/06/2003   Onager   Modified mobile_activity() to prevent charmie    */
/*                       scavenging                                       */
/**************************************************************************/
/* $Id: mob_act.cpp,v 1.52 2014/07/04 22:00:04 jhhudso Exp $ */

#include <cstring>

#include <cstdio>

#include "DC/character.h"
#include "DC/room.h"
#include "DC/mobile.h"
#include "DC/utility.h"
#include "DC/fight.h"
#include "DC/db.h" // index_data
#include "DC/player.h"
#include "DC/act.h"
#include "DC/handler.h"
#include "DC/interp.h"
#include "DC/returnvals.h"
#include "DC/spells.h"
#include "DC/race.h" // Race defines used in align-aggro messages.
#include "DC/comm.h"
#include "DC/connect.h"
#include "DC/inventory.h"
#include "DC/const.h"
#include "DC/Timer.h"
#include "DC/move.h"

void perform_wear(Character *ch, class Object *obj_object,
                  int keyword);
bool is_protected(Character *vict, Character *ch);
void scavenge(Character *ch);
bool is_r_denied(Character *ch, int room)
{
  struct deny_data *d;
  if (IS_PC(ch))
    return false;
  for (d = DC::getInstance()->world[room].denied; d; d = d->next)
    if (DC::getInstance()->mob_index[ch->mobdata->nr].virt == d->vnum)
      return true;
  return false;
}
void mobile_activity(void)
{
  Character *tmp_ch, *pch;
  class Object *obj, *best_obj;
  char buf[1000];
  int door, max;
  int done;
  int tmp_race, tmp_bitv;
  int retval;
  extern int mprog_cur_result;

  /* Examine all mobs. */
  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &ch : character_list)
  {
    if (ch->isDead() || ch->isNowhere())
    {
      continue;
    }

    if (!IS_NPC(ch))
      continue;

    if (MOB_WAIT_STATE(ch) > 0)
      MOB_WAIT_STATE(ch) -= DC::PULSE_MOBILE;
    if (IS_AFFECTED(ch, AFF_PARALYSIS))
      continue;

    if (isSet(ch->combat, COMBAT_SHOCKED) || isSet(ch->combat, COMBAT_SHOCKED2))
      continue;

    if ((isSet(ch->combat, COMBAT_STUNNED)) ||
        (isSet(ch->combat, COMBAT_STUNNED2)))
      continue;

    if ((isSet(ch->combat, COMBAT_BASH1)) ||
        (isSet(ch->combat, COMBAT_BASH2)))
      continue;

    retval = eSUCCESS;

    // Examine call for special procedure
    // These are done BEFORE checks for awake and stuff, so the proc needs
    // to check that stuff on it's own to make sure it doesn't do something
    // silly.  This is to allow for mob procs to "wake" the mob and stuff
    // It also means the mob has to check to make sure he's not already in
    // combat for stuff he shouldn't be able to do while fighting:)
    // And paralyze...

    if (ch->mobdata == nullptr)
    {
      continue;
    }

    if (DC::getInstance()->mob_index[ch->mobdata->nr].non_combat_func)
    {

      PerfTimers["mprog"].start();
      retval = ((*DC::getInstance()->mob_index[ch->mobdata->nr].non_combat_func)(ch, 0, cmd_t::UNDEFINED, "", ch));
      PerfTimers["mprog"].stop();

      if (!isSet(retval, eFAILURE) || SOMEONE_DIED(retval) || ch->isDead() || ch->isNowhere())
        continue;
    }

    if (ch->fighting) // that's it for monsters busy fighting
      continue;

    if (!AWAKE(ch))
      continue;
    if (IS_AFFECTED(ch, AFF_PARALYSIS))
      continue;

    done = 0;

    // TODO - Try to make the 'average' mob IQ higher
    selfpurge = false;

    // Only activate mprog random triggers if someone is in the zone
    try
    {
      if (DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).players)
      {
        retval = mprog_random_trigger(ch);
        if (isSet(retval, eCH_DIED) || ch->isDead() || ch->isNowhere())
        {
          continue;
        }
      }

      retval = mprog_arandom_trigger(ch);
      if (isSet(retval, eCH_DIED) || selfpurge || ch->isDead() || ch->isNowhere())
      {
        continue;
      }
    }
    catch (...)
    {
      logentry(QStringLiteral("error in mobile_activity. dumping core."), IMMORTAL, DC::LogChannel::LOG_BUG);
      produce_coredump(ch);
    }

    // activate mprog act triggers
    if (ch->mobdata->mpactnum > 0) // we check to make sure ch is mob in very beginning, so safe
    {
      mob_prog_act_list *tmp_act, *tmp2_act;
      for (tmp_act = ch->mobdata->mpact; tmp_act != nullptr; tmp_act = tmp_act->next)
      {
        PerfTimers["mprog_wordlist"].start();
        mprog_wordlist_check(tmp_act->buf, ch, tmp_act->ch,
                             tmp_act->obj, tmp_act->vo, ACT_PROG, false);
        PerfTimers["mprog_wordlist"].stop();

        retval = mprog_cur_result;
        if (isSet(retval, eCH_DIED) || ch->isDead() || ch->isNowhere())
          break; // break so we can continue with the next mob
      }
      if (isSet(retval, eCH_DIED) || selfpurge || ch->isDead() || ch->isNowhere())
        continue; // move on to next mob, this one is dead

      for (tmp_act = ch->mobdata->mpact; tmp_act != nullptr; tmp_act = tmp2_act)
      {
        tmp2_act = tmp_act->next;
        dc_free(tmp_act->buf);
        dc_free(tmp_act);
      }
      ch->mobdata->mpactnum = 0;
      ch->mobdata->mpact = nullptr;
    }

    PerfTimers["scavenge"].start();
    // TODO - this really should be cleaned up and put into functions look at it and you'll
    //    see what I mean.

    if (DC::getInstance()->world[ch->in_room].contents &&
        ISSET(ch->mobdata->actflags, ACT_SCAVENGER) &&
        !IS_AFFECTED(ch, AFF_CHARM) &&
        number(0, 2) == 0)
    {
      scavenge(ch);
    }

    // TODO - I believe this is second, so that we go through and pick up armor/weapons first
    // and then we pick up the best item and work our way down.  This makes sense but really
    // is NOT that big a deal.  If an item is on the ground long enough for a mob to pick it
    // up, it's probably going to have time to get the next item too.  We need to move this
    // into the above SCAVENGER if statement, and streamline them both to be more effecient

    // Scavenge
    if (ISSET(ch->mobdata->actflags, ACT_SCAVENGER) && !IS_AFFECTED(ch, AFF_CHARM) && DC::getInstance()->world[ch->in_room].contents && number(0, 4) == 0)
    {
      max = 1;
      best_obj = 0;
      for (obj = DC::getInstance()->world[ch->in_room].contents; obj; obj = obj->next_content)
      {
        if (CAN_GET_OBJ(ch, obj) && obj->obj_flags.cost > max)
        {
          best_obj = obj;
          max = obj->obj_flags.cost;
        }
      }

      if (best_obj)
      {
        // This should get rid of all the "gold coins" in mobs inventories.
        // -Pirahna 12/11/00
        get(ch, best_obj, 0, 0, cmd_t::DEFAULT);
        //        move_obj( best_obj, ch );
        //        act( "$n gets $p.",  ch, best_obj, 0, TO_ROOM, 0);
      }
    }

    PerfTimers["scavenge"].stop();

    /* Wander */
    if (!ISSET(ch->mobdata->actflags, ACT_SENTINEL) && GET_POS(ch) == position_t::STANDING)
    {
      door = number(0, 30);
      if (door <= 5 && CAN_GO(ch, door))
      {
        int room_nr_past_door = EXIT(ch, door)->to_room;
        if (room_nr_past_door < 0)
        {
          logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Error: Room %d has exit %d to room %d", ch->in_room, door, room_nr_past_door);
          continue;
        }
        Room room_past_door = DC::getInstance()->world[room_nr_past_door];
        if (!isSet(room_past_door.room_flags, NO_MOB) && !isSet(room_past_door.room_flags, CLAN_ROOM) && (IS_AFFECTED(ch, AFF_FLYING) || !isSet(room_past_door.room_flags, (FALL_UP | FALL_SOUTH | FALL_NORTH | FALL_EAST | FALL_WEST | FALL_DOWN))) && (!ISSET(ch->mobdata->actflags, ACT_STAY_ZONE) || room_past_door.zone == DC::getInstance()->world[ch->in_room].zone))
        {
          if (!is_r_denied(ch, EXIT(ch, door)->to_room) && ch->mobdata->last_direction == door)
            ch->mobdata->last_direction = -1;
          else if (!is_r_denied(ch, EXIT(ch, door)->to_room) && (!ISSET(ch->mobdata->actflags, ACT_STAY_NO_TOWN) ||
                                                                 !DC::getInstance()->zones.value(DC::getInstance()->world[EXIT(ch, door)->to_room].zone).isTown()))
          {
            ch->mobdata->last_direction = door;
            auto cmd_dir = getCommandFromDirection(door);
            if (cmd_dir)
            {
              retval = attempt_move(ch, *cmd_dir);
              if (isSet(retval, eCH_DIED))
                continue;
            }
          }
        }
      }
    }

    // check hated
    if ((ch->mobdata->hated != nullptr)) //  && (!ch->fighting)) (we check fighting earlier)
    {
      ch->sendln("You're hating.");
      Character *next_blah;
      //      Character *temp = get_char(get_random_hate(ch));
      done = 0;

      for (tmp_ch = DC::getInstance()->world[ch->in_room].people; tmp_ch; tmp_ch = next_blah)
      {
        next_blah = tmp_ch->next_in_room;

        if (!CAN_SEE(ch, tmp_ch))
          continue;
        if (!IS_NPC(tmp_ch) && isSet(tmp_ch->player->toggles, Player::PLR_NOHASSLE))
          continue;
        act("Checking $N", ch, 0, tmp_ch, TO_CHAR, 0);
        if (isexact(GET_NAME(tmp_ch), ch->mobdata->hated)) // use isname since hated is a list
        {
          if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
          {
            act("You growl at $N.", ch, 0, tmp_ch, TO_CHAR, 0);
            act("$n growls at YOU!.", ch, 0, tmp_ch, TO_VICT, 0);
            act("$n growls at $N.", ch, 0, tmp_ch, TO_ROOM, NOTVICT);
            continue;
          }
          else if (IS_PC(tmp_ch))
          {
            act("$n screams, 'I am going to KILL YOU!'", ch, 0, 0, TO_ROOM, 0);
            PerfTimers["mprog_attack"].start();
            retval = mprog_attack_trigger(ch, tmp_ch);
            PerfTimers["mprog_attack"].stop();

            if (SOMEONE_DIED(retval))
              break;
            attack(ch, tmp_ch, 0);
            done = 1;
            break;
          }
        }
      } // for

      if (done)
        continue;

      if (!ISSET(ch->mobdata->actflags, ACT_STUPID))
      {
        if (GET_POS(ch) > position_t::SITTING)
        {
          if (!IS_AFFECTED(ch, AFF_BLIND) && !ch->hunting.isEmpty())
          {
            retval = ch->do_track(QString(ch->hunting).split(' '));
            if (SOMEONE_DIED(retval))
              continue;
          }
        }
        else if (GET_POS(ch) < position_t::FIGHTING)
        {
          do_stand(ch, "");
          continue;
        }
      }
    } //  end FIRST hated IF statement

    /* Aggress */
    if (!ch->fighting) // don't aggro more than one person
      if (ISSET(ch->mobdata->actflags, ACT_AGGRESSIVE) &&
          !isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
      {
        Character *next_aggro;
        int targets = 1;
        done = 0;

        // While not very effective what this does, is go through the
        // list of people in room.  If it finds one, it sets targets to true
        // and has a 50% chance of going for that person.  If it does, we're
        // done and we leave.  If we don't, we keep going through the list
        // and get a 50% chance at each person.  If we hit the end of the list,
        // we know there _is_ still a person in the room we want to aggro, so
        // loop back through again. - pir 5/3/00
        while (!done && targets)
        {
          targets = 0;
          for (tmp_ch = DC::getInstance()->world[ch->in_room].people; tmp_ch; tmp_ch = next_aggro)
          {
            if (!tmp_ch || !ch)
            {
              logentry(QStringLiteral("Null ch or tmp_ch in mobile_action()"), IMMORTAL, DC::LogChannel::LOG_BUG);
              break;
            }
            next_aggro = tmp_ch->next_in_room;

            if (ch == tmp_ch)
              continue;
            if (!CAN_SEE(ch, tmp_ch))
              continue;
            if (IS_NPC(tmp_ch) && !IS_AFFECTED(tmp_ch, AFF_CHARM) && !tmp_ch->desc)
              continue;
            if (ISSET(ch->mobdata->actflags, ACT_WIMPY) && AWAKE(tmp_ch))
              continue;
            if ((!IS_NPC(tmp_ch) && isSet(tmp_ch->player->toggles, Player::PLR_NOHASSLE)) || (tmp_ch->desc && tmp_ch->desc->original &&
                                                                                              isSet(tmp_ch->desc->original->player->toggles, Player::PLR_NOHASSLE)))
              continue;

            /* check for PFG/PFE, (anti)pal perma-protections, etc. */
            //          if (is_protected(tmp_ch, ch))
            //          continue;

            if (number(0, 1))
            {
              done = 1;

              PerfTimers["attack_trigger2"].start();
              retval = mprog_attack_trigger(ch, tmp_ch);
              PerfTimers["attack_trigger2"].stop();

              if (SOMEONE_DIED(retval))
                break;
              attack(ch, tmp_ch, TYPE_UNDEFINED);
              break;
            }
            else
              targets = 1;
          }
        }

        if (done)
          continue;
      } // if aggressive

    if (!ch->fighting)
      if (ch->mobdata->fears)
        if (ch->get_char_room_vis(ch->mobdata->fears))
        {
          if (ch->mobdata->hated != nullptr)
            remove_memory(ch, 'h');
          act("$n screams 'Oh SHIT!'", ch, 0, 0, TO_ROOM, 0);
          do_flee(ch, "");
          continue;
        }

    if (!ch->fighting)
      if (ISSET(ch->mobdata->actflags, ACT_RACIST) ||
          ISSET(ch->mobdata->actflags, ACT_FRIENDLY) ||
          ISSET(ch->mobdata->actflags, ACT_AGGR_EVIL) ||
          ISSET(ch->mobdata->actflags, ACT_AGGR_NEUT) ||
          ISSET(ch->mobdata->actflags, ACT_AGGR_GOOD))
        for (tmp_ch = DC::getInstance()->world[ch->in_room].people; tmp_ch; tmp_ch = pch)
        {
          pch = tmp_ch->next_in_room;

          if (ch == tmp_ch)
            continue;

          tmp_bitv = getBitvector(tmp_ch->race);

          if (ISSET(ch->mobdata->actflags, ACT_FRIENDLY) &&
              (ch->mobdata->hated.isEmpty() || !isexact(GET_NAME(tmp_ch), ch->mobdata->hated)) &&
              tmp_ch->fighting &&
              CAN_SEE(ch, tmp_ch) &&
              (isSet(races[(int)GET_RACE(ch)].friendly, tmp_bitv) ||
               (int)GET_RACE(ch) == (int)GET_RACE(tmp_ch)) &&

              !(IS_NPC(tmp_ch->fighting) && !IS_AFFECTED(tmp_ch->fighting, AFF_CHARM)) && !isSet(races[(int)GET_RACE(ch)].friendly, getBitvector(tmp_ch->fighting->race)) &&
              !tmp_ch->affected_by_spell(Character::PLAYER_OBJECT_THIEF) && !tmp_ch->isPlayerGoldThief())
          {
            tmp_race = GET_RACE(tmp_ch);
            if (GET_RACE(ch) == tmp_race)
              sprintf(buf, "$n screams 'Take heart, fellow %s!'", races[tmp_race].singular_name);
            else
              sprintf(buf, "$n screams 'HEY! Don't be picking on %s!'", races[tmp_race].plural_name);
            act(buf, ch, 0, 0, TO_ROOM, 0);

            PerfTimers["attack_trigger3"].start();
            retval = mprog_attack_trigger(ch, tmp_ch);
            PerfTimers["attack_trigger3"].stop();

            if (SOMEONE_DIED(retval))
              break;
            attack(ch, tmp_ch->fighting, 0);
            break;
          }

          /* check for PFE/PFG, (anti)pal perma-protections, etc. */
          // AFTER Friendly check, 'cause I don't wanna be protected against friendly...
          if (is_protected(tmp_ch, ch))
            continue;

          //           continue;

          if ((IS_PC(tmp_ch) && !tmp_ch->fighting && CAN_SEE(ch, tmp_ch) &&
               !isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) &&
               !isSet(tmp_ch->player->toggles, Player::PLR_NOHASSLE)) ||
              (IS_NPC(tmp_ch) && tmp_ch->desc && tmp_ch->desc->original && CAN_SEE(ch, tmp_ch) && !isSet(tmp_ch->desc->original->player->toggles, Player::PLR_NOHASSLE) // this is safe, cause we checked IS_PC first
               ))
          {
            int i = 0;
            switch (ch->race)
            { // Messages for attackys
            case RACE_HUMAN:
            case RACE_ELVEN:
            case RACE_DWARVEN:
            case RACE_HOBBIT:
            case RACE_PIXIE:
            case RACE_GIANT:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_TROLL:
            case RACE_GOBLIN:
            case RACE_DRAGON:
            case RACE_ENFAN:
            case RACE_DEMON:
            case RACE_YRNALI:
              i = 1;
              break;
            default:
              i = 0;
              break;
            }

            if (ISSET(ch->mobdata->actflags, ACT_AGGR_EVIL) &&
                GET_ALIGNMENT(tmp_ch) <= -350)
            {
              if (i == 1)
              {
                if (GET_ALIGNMENT(ch) <= -350)
                  act("$n screams 'Get outta here, freak!'", ch, 0, 0, TO_ROOM, 0);
                else
                  act("$n screams 'May truth and justice prevail!'", ch, 0, 0, TO_ROOM, 0);
              }
              else
              {
                act("$n senses your evil intentions and attacks!", ch, 0, tmp_ch, TO_VICT, 0);
                act("$n senses $N's evil intentions and attacks!", ch, 0, tmp_ch, TO_ROOM, NOTVICT);
              }

              PerfTimers["attack_trigger4"].start();
              retval = mprog_attack_trigger(ch, tmp_ch);
              PerfTimers["attack_trigger4"].stop();

              if (SOMEONE_DIED(retval))
                break;
              attack(ch, tmp_ch, 0);
              break;
            }

            if (ISSET(ch->mobdata->actflags, ACT_AGGR_GOOD) &&
                GET_ALIGNMENT(tmp_ch) >= 350)
            {
              if (i == 1)
              {
                if (GET_ALIGNMENT(ch) >= 350)
                  act("$n screams 'I'm afraid I cannot let you trespass onto these grounds!'", ch, 0, 0, TO_ROOM, 0);
                else
                  act("$n screams 'The forces of evil shall crush your goodness!'", ch, 0, 0, TO_ROOM, 0);
              }
              else
              {
                act("$n is offended by your good nature and attacks!", ch, 0, tmp_ch, TO_VICT, 0);
                act("$n is offended by $N's good nature and attacks!", ch, 0, tmp_ch, TO_ROOM, NOTVICT);
              }
              PerfTimers["attack_trigger5"].start();
              retval = mprog_attack_trigger(ch, tmp_ch);
              PerfTimers["attack_trigger5"].stop();

              if (SOMEONE_DIED(retval))
                break;
              attack(ch, tmp_ch, 0);
              break;
            }

            if (ISSET(ch->mobdata->actflags, ACT_AGGR_NEUT) &&
                GET_ALIGNMENT(tmp_ch) > -350 &&
                GET_ALIGNMENT(tmp_ch) < 350)
            {
              if (i == 0)
                act("$n screams 'Pick a side, neutral dog!'", ch, 0, 0, TO_ROOM, 0);
              else
              {
                act("$n detects $N's neutrality and attacks!", ch, 0, tmp_ch, TO_ROOM, NOTVICT);
                act("$n detects your neutrality and attacks!", ch, 0, tmp_ch, TO_VICT, 0);
              }

              retval = mprog_attack_trigger(ch, tmp_ch);
              if (SOMEONE_DIED(retval))
                break;
              attack(ch, tmp_ch, 0);
              break;
            }

            if (ISSET(ch->mobdata->actflags, ACT_RACIST) &&
                isSet(races[(int)GET_RACE(ch)].hate_fear, tmp_bitv))
            {
              tmp_race = GET_RACE(tmp_ch);
              bool wimpy = ISSET(ch->mobdata->actflags, ACT_WIMPY);

              // if mob isn't wimpy, always attack
              // if mob is wimpy, but is equal or greater, attack
              // if mob is wimpy, and lower level.. flee
              if (!wimpy || (wimpy && ch->getLevel() >= tmp_ch->getLevel()))
              {
                sprintf(buf, "$n screams 'Oooo, I HATE %s!'", races[tmp_race].plural_name);
                act(buf, ch, 0, 0, TO_ROOM, 0);
                retval = mprog_attack_trigger(ch, tmp_ch);
                if (SOMEONE_DIED(retval))
                  break;
                attack(ch, tmp_ch, 0);
              }
              else
              {
                sprintf(buf, "$n screams 'Eeeeek, I HATE %s!'", races[tmp_race].plural_name);
                act(buf, ch, 0, 0, TO_ROOM, 0);
                do_flee(ch, "");
              }
              break;
            }
          } // If IS_PC(tmp_ch)
        } // for() for the RACIST, AGG_XXX and FRIENDLY flags

    // Note, if you add anything to this point, you need to put if(done) continue
    // check after the RACIST stuff.  They aren't checking if ch died or not since
    // it just ends here.

  } // for() all mobs
  DC::getInstance()->removeDead();
}

// Just a function to have mobs say random stuff when they are "suprised"
// about finding a player doing something and decide to attack them.
// For example, when a mob finds a player casting "spell" on them.
void mob_suprised_sayings(Character *ch, Character *aggressor)
{
  switch (number(0, 6))
  {
  case 0:
    do_say(ch, "What do you think you are doing?!");
    break;
  case 1:
    do_say(ch, "Mess with the best?  Die like the rest!");
    break;
  case 2:
    do_emote(ch, " looks around for a moment, confused.");
    do_say(ch, "YOU!!");
    break;
  case 3:
    do_say(ch, "Foolish.");
    break;
  case 4:
    do_say(ch, "I'm going to treat you like a baby treats a diaper.");
    break;
  case 5:
    do_say(ch, "Here comes the pain baby!");
    break;
  case 6:
    do_emote(ch, " wiggles its bottom.");
    break;
  }
}

/* check to see if the player is protected from the mob */
// PROTECTION_FROM_EVIL and GOOD modifier contains the level
// protected from.  PAL's ANTI's take spell/level whichever higher
bool is_protected(Character *vict, Character *ch)
{
  struct affected_type *aff = vict->affected_by_spell(SPELL_PROTECT_FROM_EVIL);
  int level_protected = aff ? aff->modifier : 0;
  if (GET_CLASS(vict) == CLASS_ANTI_PAL && IS_EVIL(vict) && vict->getLevel() > level_protected)
    level_protected = vict->getLevel();

  if (IS_EVIL(ch) && ch->getLevel() < level_protected)
    return (true);

  if (IS_EVIL(ch) && ch->getLevel() <= vict->getLevel() && IS_AFFECTED(vict, AFF_PROTECT_EVIL))
    return true;

  aff = vict->affected_by_spell(SPELL_PROTECT_FROM_GOOD);
  level_protected = aff ? aff->modifier : 0;
  if (GET_CLASS(vict) == CLASS_PALADIN && IS_GOOD(vict) && vict->getLevel() > level_protected)
    level_protected = vict->getLevel();

  if (IS_GOOD(ch) && ch->getLevel() < level_protected)
    return (true);

  if (IS_GOOD(ch) && ch->getLevel() <= vict->getLevel() && IS_AFFECTED(vict, AFF_PROTECT_GOOD))
    return true;

  /* old version
     if(IS_EVIL(ch) && ch->getLevel() <= (vict->getLevel())) {
        if((IS_AFFECTED(vict, AFF_PROTECT_EVIL)) ||
          (GET_CLASS(vict) == CLASS_ANTI_PAL && IS_EVIL(vict)))
             return(true);
     }

     if(IS_GOOD(ch) && ch->getLevel() <= (vict->getLevel())) {
        if((vict->affected_by_spell(SPELL_PROTECT_FROM_GOOD))||
          (GET_CLASS(vict) == CLASS_PALADIN && IS_GOOD(vict)))
             return(true);
     }
  */

  return (false);
}

void scavenge(Character *ch)
{
  class Object *obj;
  int done;
  int keyword;

  done = 0;
  if (IS_AFFECTED(ch, AFF_CHARM))
    return;
  for (obj = DC::getInstance()->world[ch->in_room].contents; obj; obj = obj->next_content)
  {
    if (!CAN_GET_OBJ(ch, obj))
      continue;

    if (DC::getInstance()->obj_index[obj->item_number].virt == CHAMPION_ITEM)
      continue;

    keyword = obj->keywordfind();

    if (keyword != -2)
    {
      if (ch->hands_are_free(1))
      {
        if (CAN_WEAR(obj, WIELD))
        {
          if (GET_OBJ_WEIGHT(obj) < GET_STR(ch))
          {
            if (!ch->equipment[WEAR_WIELD])
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_WIELD);
              break;
            }
            /* damage check */
            if ((ch->equipment[WEAR_WIELD]) && (!ch->equipment[WEAR_SECOND_WIELD]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_SECOND_WIELD);
              break;
            }
          } // GET_OBJ_WEIGHT()
          continue;
        } /* if can wear */
        else
        {
          if (((keyword == 13) || (keyword == 14)) && !ch->hands_are_free(1))
            continue;

          switch (keyword)
          {

          case 0:
            if ((CAN_WEAR(obj, FINGER)) &&
                ((!ch->equipment[WEAR_FINGER_L]) || (!ch->equipment[WEAR_FINGER_R])))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              if (!ch->equipment[WEAR_FINGER_L])
                ch->equip_char(obj, WEAR_FINGER_L);
              else
                ch->equip_char(obj, WEAR_FINGER_R);
              done = 1;
            }
            break;

          case 1:
            if ((CAN_WEAR(obj, NECK)) &&
                ((!ch->equipment[WEAR_NECK_1]) || (!ch->equipment[WEAR_NECK_2])))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              if (!ch->equipment[WEAR_NECK_1])
                ch->equip_char(obj, WEAR_NECK_1);
              else
                ch->equip_char(obj, WEAR_NECK_2);
              done = 1;
            }
            break;

          case 2:
            if ((CAN_WEAR(obj, BODY)) && (!ch->equipment[WEAR_BODY]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_BODY);
              done = 1;
            }
            break;

          case 3:
            if ((CAN_WEAR(obj, HEAD)) && (!ch->equipment[WEAR_HEAD]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_HEAD);
              done = 1;
            }
            break;

          case 4:
            if ((CAN_WEAR(obj, LEGS)) && (!ch->equipment[WEAR_LEGS]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_LEGS);
              done = 1;
            }
            break;

          case 5:
            if ((CAN_WEAR(obj, FEET)) && (!ch->equipment[WEAR_FEET]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_FEET);
              done = 1;
            }
            break;

          case 6:
            if ((CAN_WEAR(obj, HANDS)) && (!ch->equipment[WEAR_HANDS]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_HANDS);
              done = 1;
            }
            break;

          case 7:
            if ((CAN_WEAR(obj, ARMS)) && (!ch->equipment[WEAR_ARMS]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_ARMS);
              done = 1;
            }
            break;

          case 8:
            if ((CAN_WEAR(obj, ABOUT)) && (!ch->equipment[WEAR_ABOUT]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_ABOUT);
              done = 1;
            }
            break;

          case 9:
            if ((CAN_WEAR(obj, WAISTE)) && (!ch->equipment[WEAR_WAISTE]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_WAISTE);
              done = 1;
            }
            break;

          case 10:
            if ((CAN_WEAR(obj, WRIST)) &&
                ((!ch->equipment[WEAR_WRIST_L]) || (!ch->equipment[WEAR_WRIST_R])))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              if (!ch->equipment[WEAR_WRIST_L])
                ch->equip_char(obj, WEAR_WRIST_L);
              else
                ch->equip_char(obj, WEAR_WRIST_R);
              done = 1;
            }
            break;

          case 11:
            if ((CAN_WEAR(obj, FACE)) && (!ch->equipment[WEAR_FACE]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_FACE);
              done = 1;
            }
            break;

          case 12:
            done = 1;
            break;

          case 13:
            if ((CAN_WEAR(obj, SHIELD)) && (!ch->equipment[WEAR_SHIELD]))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              ch->equip_char(obj, WEAR_SHIELD);
              done = 1;
            }
            break;

          case 14:
            if ((CAN_WEAR(obj, HOLD)) && (!ch->equipment[WEAR_HOLD]))
            {
              if ((obj->obj_flags.type_flag == ITEM_LIGHT) &&
                  (!ch->equipment[WEAR_LIGHT]))
              {
                move_obj(obj, ch);
                act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
                perform_wear(ch, obj, 16);
                obj_from_char(obj);
                ch->equip_char(obj, WEAR_LIGHT);
                done = 1;
              }
              else if (obj->obj_flags.type_flag != ITEM_LIGHT)
              {
                move_obj(obj, ch);
                act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
                perform_wear(ch, obj, keyword);
                obj_from_char(obj);
                ch->equip_char(obj, WEAR_HOLD);
                done = 1;
              }
            }
            break;

          case 15:
            if ((CAN_WEAR(obj, EAR)) &&
                ((!ch->equipment[WEAR_EAR_L]) || (!ch->equipment[WEAR_EAR_R])))
            {
              move_obj(obj, ch);
              act("$n gets $p.", ch, obj, 0, TO_ROOM, 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              if (!ch->equipment[WEAR_EAR_L])
                ch->equip_char(obj, WEAR_EAR_L);
              else
                ch->equip_char(obj, WEAR_EAR_R);
              done = 1;
            }
            break;

          default:
            logentry(QStringLiteral("Bad switch in mob_act.C"), 0, DC::LogChannel::LOG_BUG);
            break;

          } /* end switch */

          if (done == 1)
            break;
          else
            continue;
        } // else can wear
      } // if hands are free
    } /* if keyword != -2 */
  } /* for obj */
}

void clear_hunt(varg_t arg1, void *arg2, void *arg3)
{
  clear_hunt(arg1, (Character *)arg2, nullptr);
}

void clear_hunt(varg_t arg1, Character *arg2, void *arg3)
{
  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &curr : character_list)
  {
    if (curr == arg2)
    {
      arg2->hunting.clear();
      break;
    }
  }
}
