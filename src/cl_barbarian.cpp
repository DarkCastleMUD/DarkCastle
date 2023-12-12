/************************************************************************
 * $Id: cl_barbarian.cpp,v 1.113 2011/01/27 05:55:44 jhhudso Exp $
 * cl_barbarian.cpp
 * Description: Commands for the barbarian class.
 *************************************************************************/
#include "structs.h"
#include "player.h"
#include "levels.h"
#include "character.h"
#include "spells.h"
#include "utility.h"
#include "fight.h"
#include "mobile.h"
#include "magic.h"
#include "connect.h"
#include "handler.h"
#include "act.h"
#include "interp.h"
#include "returnvals.h"
#include "room.h"
#include "db.h"
#include "clan.h"
#include "utility.h"
#include "const.h"
#include "inventory.h"
#include "move.h"

extern struct index_data *obj_index;
extern int rev_dir[];

int do_batter(Character *ch, char *argument, int cmd)
{
  bool battervbrace = false;
  bool batterwins = false;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], dammsg[20];
  struct room_direction_data *exit, *back;
  int other_room, door, dam, skill, retval;

  if (!(skill = ch->has_skill(SKILL_BATTERBRACE)))
  {
    ch->sendln("You could accidentally hurt someone if you try this untrained...");
    return eFAILURE;
  }

  argument_interpreter(argument, type, dir);

  if (!*type)
  {
    ch->sendln("Batter what??");
    ch->sendln("batter <door> <direction>");
    return eFAILURE;
  }

  if (!*dir)
  {
    ch->sendln("What direction?");
    return eFAILURE;
  }

  if ((door = find_door(ch, type, dir)) >= 0)
  {
    exit = EXIT(ch, door);

    // check to make sure door is in right state
    if (!DC::isSet(exit->exit_info, EX_ISDOOR))
    {
      ch->sendln("You can't figure out how to break it down.");
      return eFAILURE;
    }

    if (!DC::isSet(exit->exit_info, EX_CLOSED))
    {
      ch->sendln("You can't break down an open door!");
      return eFAILURE;
    }

    if (DC::isSet(exit->exit_info, EX_PICKPROOF))
    {
      ch->sendln("It seems far too sturdy for you to break down.");
      return eFAILURE;
    }

    if (exit->bracee == ch)
    {
      ch->sendln("You can't batter a door you're bracing shut!");
      return eFAILURE;
    }

    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1.5);
    if (!charge_moves(ch, SKILL_BATTERBRACE))
      return eSUCCESS;

    dam = number(100, 200) + 3 * (100 - skill);

    csendf(ch, "You take a deep breath, let loose a mighty bellow, and charge blindly at the %s in your path...\r\n",
           fname(exit->keyword));
    act("$n takes a deep breath, lets loose a mighty bellow, and charges blindly at the $F in $s path...",
        ch, 0, exit->keyword, TO_ROOM, 0);

    if (!skill_success(ch, nullptr, SKILL_BATTERBRACE))
    {

      sprintf(dammsg, "$B%d$R", dam);
      sprintf(buf2, "With a resounding crash, you bounce off the %s and fall to the ground, receiving | damage!", fname(exit->keyword));
      sprintf(buf, "With a resounding crash, you bounce off the %s and fall to the ground!", fname(exit->keyword));
      send_damage(buf2, ch, 0, 0, dammsg, buf, TO_CHAR);
      sprintf(buf, "With a resounding crash, $e bounces off the %s and falls to the ground!", fname(exit->keyword));
      send_damage(buf, ch, 0, 0, dammsg, buf, TO_ROOM);

      sprintf(buf, "The %s survived, but you didn't...\r\n", fname(exit->keyword));
      sprintf(buf2, "The %s survived, but $n didn't...", fname(exit->keyword));
      retval = noncombat_damage(ch, dam, buf, buf2, 0, KILL_BATTER);

      if (SOMEONE_DIED(retval))
      {
        return retval;
      }

      ch->setSitting();
      update_pos(ch);
      return eFAILURE;
    }
    else
    {
      // check for brace
      if (exit->bracee != nullptr)
      {
        battervbrace = true;
        int batterer = GET_STR(ch) + GET_CON(ch) + GET_DEX(ch) + number(1, 10);
        int bracee = GET_STR(exit->bracee) + GET_CON(exit->bracee) + GET_DEX(exit->bracee) + number(1, 10);
        if (batterer < bracee) // batterer fails (ch fails)
        {
          ch->decrementMove(100);
        }
        else
        {
          batterwins = true;
          exit->bracee->decrementMove(100);
        }
      }

      REMOVE_BIT(exit->exit_info, EX_CLOSED); // break this side of door
      SET_BIT(exit->exit_info, EX_BROKEN);

      if ((other_room = exit->to_room) != DC::NOWHERE)
        if ((back = DC::getInstance()->world[other_room].dir_option[rev_dir[door]]) != 0)
          if (back->to_room == ch->in_room)
          {
            REMOVE_BIT(back->exit_info, EX_CLOSED); // break other side of door
            SET_BIT(back->exit_info, EX_BROKEN);
          }

      if (battervbrace)
      {
        if (batterwins)
        {
          csendf(exit->bracee, "The %s bursts open with a resounding crash and you are hurld to the ground!\r\n", fname(exit->keyword));
          act("The $F bursts open with a resounding crash and $n is hurled to the ground!", exit->bracee, 0, exit->keyword, TO_ROOM, 0);
          exit->bracee->setSitting();
          update_pos(exit->bracee);
          do_brace(exit->bracee, "", 0); // unbrace
        }
        else
        { // brace wins
          csendf(exit->bracee, "The %s shakes dangerously as a powerful blow strikes it from the other side!\r\n", fname(exit->keyword));
        }
      }
      else
      {
        sprintf(dammsg, "$B%d$R", dam);
        sprintf(buf2, "With a resounding crash, the %s gives way and bursts open, receiving | damage!.", fname(exit->keyword));
        sprintf(buf, "With a resounding crash, the %s gives way and bursts open!\r\n", fname(exit->keyword));
        send_damage(buf2, ch, 0, 0, dammsg, buf, TO_CHAR);
        sprintf(buf, "With a resounding crash, the %s gives way and bursts open!", fname(exit->keyword));
        send_damage(buf, ch, 0, 0, dammsg, buf, TO_ROOM);
      }

      sprintf(buf, "Your heroic efforts managed to slay both the %s... and yourself. Nice going.\r\n", fname(exit->keyword));
      sprintf(buf2, "$n's heroic efforts manage to slay both the %s... and $mself. Oops.", fname(exit->keyword));

      retval = noncombat_damage(ch, dam, buf, buf2, 0, KILL_BATTER);

      if (SOMEONE_DIED(retval))
      {
        return retval;
      }

      if (number(1, 100) > (40 - GET_DEX(ch)) + (100 - skill))
      {
        ch->sendln("You manage to maintain your balance and admire your handywork.");
        act("$n manages to maintain $h balance and admires $s handywork.", ch, 0, exit->keyword, TO_ROOM, 0);
      }
      else
      {
        if (CAN_GO(ch, door) &&
            !DC::isSet(DC::getInstance()->world[EXIT(ch, door)->to_room].room_flags, IMP_ONLY) &&
            (!IS_AFFECTED(ch, AFF_CHAMPION) || champion_can_go(EXIT(ch, door)->to_room)) &&
            class_can_go(GET_CLASS(ch), EXIT(ch, door)->to_room) &&
            !others_clan_room(ch, &DC::getInstance()->world[EXIT(ch, door)->to_room]))
        {
          ch->sendln("You are unable to maintain your balance and sail into the adjacent room! Ouch!\r\n");
          act("$n is unable to maintain $h balance and sails into the adjacent room!", ch, 0, exit->keyword, TO_ROOM, 0);
          move_char(ch, exit->to_room);
          act("The $F suddenly bursts apart and $n tumbles headlong through!", ch, 0, exit->keyword, TO_ROOM, 0);
          do_look(ch, "", CMD_LOOK);
        }
        else
        {
          ch->sendln("You are unable to maintain your balance and sail towards the adjacent room, but some force keeps you out!");
          act("$n is unable to maintain $h balance and sails towards the adjacent room, but some force keeps $h out!",
              ch, 0, exit->keyword, TO_ROOM, 0);
        }

        ch->setSitting();
        update_pos(ch);
      }
      return eSUCCESS;
    }
  }
  ch->sendln("You don't see anything like that to batter.");
  return eFAILURE;
}

int do_brace(Character *ch, char *argument, int cmd)
{
  int door, other_room;
  struct room_direction_data *back, *exit;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];

  argument_interpreter(argument, type, dir);

  if (!ch->has_skill(SKILL_BATTERBRACE))
  {
    ch->sendln("You could accidentally hurt someone if you try this untrained...");
    return eFAILURE;
  }

  if (!*type)
  {
    if (ch->brace_at != nullptr)
    {
      if (cmd == 0)
      {
        csendf(ch, "You are no longer able to brace the %s.\r\n", fname(ch->brace_at->keyword));
      }
      else
      {
        csendf(ch, "You stop holding the %s shut.\r\n", fname(ch->brace_at->keyword));
        act("$n stops holding the $F shut.", ch, 0, ch->brace_at->keyword, TO_ROOM, 0);
      }
      ch->brace_at->bracee = nullptr;
      ch->brace_at = nullptr;
      if (ch->brace_exit != nullptr) // incase it's a weird exit area
        ch->brace_exit->bracee = nullptr;
      ch->brace_exit = nullptr;
      return eSUCCESS;
    }
    ch->sendln("Brace what??");
    ch->sendln("brace <door> <direction>");
    return eFAILURE;
  }

  if (!*dir)
  {
    ch->sendln("What direction?");
    return eFAILURE;
  }

  if ((door = find_door(ch, type, dir)) >= 0)
  {

    exit = EXIT(ch, door);

    if (!DC::isSet(exit->exit_info, EX_ISDOOR))
    {
      ch->sendln("You can't figure out how to hold it shut.");
      return eFAILURE;
    }

    if (!DC::isSet(exit->exit_info, EX_CLOSED))
    {
      ch->sendln("You have to close it first!");
      return eFAILURE;
    }
    if (exit->bracee != nullptr)
    {
      if (exit->bracee->in_room == ch->in_room)
      {
        if (exit->bracee == ch)
        {
          csendf(ch, "You are already bracing the %s shut!\n\r", fname(exit->keyword));
        }
        else
        {
          csendf(ch, "%s is already holding the %s shut!\r\n", GET_NAME(exit->bracee), fname(exit->keyword));
        }
      }
      else
        csendf(ch, "The %s is already being braced from the other side!\r\n", fname(exit->keyword));

      return eFAILURE;
    }

    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1.5);
    if (!charge_moves(ch, SKILL_BATTERBRACE, 0.5))
      return eSUCCESS;

    csendf(ch, "You lean heavily on the %s, bracing your shoulder solidly against it...\r\n",
           fname(exit->keyword));
    act("$n leans heavily on the $F, bracing $s shoulder solidly against it...",
        ch, 0, exit->keyword, TO_ROOM, 0);

    if (!skill_success(ch, nullptr, SKILL_BATTERBRACE))
    {
      ch->sendln("Your attempt to block the passage fails.");
      act("$s attempt to block the passage fails!", ch, 0, exit->keyword, TO_ROOM, 0);
      return eFAILURE;
    }
    else
    {
      ch->sendln("The passage now appears firmly blocked.");
      act("The passage now appears firmly blocked.", ch, 0, exit->keyword, TO_ROOM, 0);
      exit->bracee = ch;
      ch->brace_at = exit;
      if ((other_room = exit->to_room) != DC::NOWHERE)
        if ((back = DC::getInstance()->world[other_room].dir_option[rev_dir[door]]) != 0)
          if (back->to_room == ch->in_room)
          {
            ch->brace_exit = back;
            back->bracee = ch;
          }

      return eSUCCESS;
    }
  }

  ch->sendln("You don't see anything like that to block.");
  return eFAILURE;
}

command_return_t Character::do_rage(QStringList arguments, int cmd)
{
  if (getHP() == 1)
  {
    sendln("You are feeling too weak right now to work yourself up into a rage.");
    return eFAILURE;
  }

  if (!canPerform(SKILL_RAGE, "You should learn the skill before you try doing any raging in this machine...\r\n"))
  {
    return eFAILURE;
  }

  QString name = arguments.value(0);

  Character *victim = get_char_room_vis(name);
  if (!victim)
  {
    if (fighting)
    {
      victim = fighting;
    }
    else
    {
      sendln("Who do you want to rage on?");
      return eFAILURE;
    }
  }

  if (in_room != victim->in_room)
  {
    this->sendln("That person seems to have left.");
    return eFAILURE;
  }

  if (victim == this)
  {
    this->sendln("Aren't we funny today...");
    return eFAILURE;
  }

  if (!can_attack(this) || !can_be_attacked(this, victim))
    return eFAILURE;

  if (!charge_moves(SKILL_RAGE))
    return eSUCCESS;

  int retval = 0;
  if (!skill_success(victim, SKILL_RAGE))
  {
    act("You start advancing towards $N, but trip over your own feet!",
        this, 0, victim, TO_CHAR, 0);
    act("$n starts advancing towards $N, but trips over $s own feet!",
        this, 0, victim, TO_ROOM, NOTVICT);
    act_return ar = act("$n starts advancing toward you, but trips over $s own feet!", this, 0, victim, TO_VICT, 0);
    retval = ar.retval;
    if (DC::isSet(retval, eVICT_DIED))
    {
      return retval;
    }

    this->setSitting();
    SET_BIT(this->combat, COMBAT_BASH1);
  }
  else
  {
    act("You advance confidently towards $N, and fly into a rage!",
        this, 0, victim, TO_CHAR, 0);
    act("$n advances confidently towards $N, and flies into a rage!",
        this, 0, victim, TO_ROOM, NOTVICT);
    act_return ar = act("$n advances confidently towards you, and flies into a rage!", this, 0, victim, TO_VICT, 0);
    retval = ar.retval;
    if (DC::isSet(retval, eVICT_DIED))
    {
      return retval;
    }

    SET_BIT(this->combat, COMBAT_RAGE1);
  }

  WAIT_STATE(this, DC::PULSE_VIOLENCE * 3);

  if (!this->fighting)
    return attack(this, victim, TYPE_UNDEFINED);

  // chance of bonus round at high level of skill
  if (has_skill(SKILL_RAGE) > 75 && !number(0, 9))
    return attack(this, victim, TYPE_UNDEFINED);

  return eSUCCESS;
}

int do_battlecry(Character *ch, char *argument, int cmd)
{
  struct follow_type *f = 0;

  if (!ch->canPerform(SKILL_BATTLECRY, "Have to learn how to battlecry before you can run with the big boys...\r\n"))
  {
    return eFAILURE;
  }

  if (!ch->fighting)
  {
    ch->sendln("You must be fighting already in order to battlecry.");
    return eFAILURE;
  }

  if (ch->master || !ch->followers)
  {
    ch->sendln("You must be leading a group in order to battlecry.");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BATTLECRY))
    return eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_BATTLECRY))
  {
    act("You give a cry of defiance, but trip over your own feet!", ch, 0, 0, TO_CHAR, 0);
    act("$n gives a cry of defiance, but trips over $s own feet!", ch, 0, 0, TO_ROOM, 0);

    ch->setSitting();
    SET_BIT(ch->combat, COMBAT_BASH1);
  }
  else
  {
    act("You give a battlecry, sounding your defiance!", ch, 0, 0, TO_CHAR, 0);
    act("$n yells 'They can take our lives, but they'll never take OUR FREEDOM!'", ch, 0, 0, TO_ROOM, 0);

    if (ch->followers)
      f = ch->followers;

    for (; f; f = f->next)
    {
      if (!IS_AFFECTED(f->follower, AFF_GROUP) ||
          (DC::isSet(f->follower->combat, COMBAT_RAGE1)) ||
          (DC::isSet(f->follower->combat, COMBAT_RAGE2)) ||
          (DC::isSet(f->follower->combat, COMBAT_BERSERK)) ||
          (f->follower->in_room != ch->in_room) ||
          (!f->follower->fighting))
        continue;

      if (number(1, 101) > ch->has_skill(SKILL_BATTLECRY))
      {
        act("You look away sheepishly, unaffected by the rage.", f->follower, 0, 0, TO_CHAR, 0);
        act("$n looks away sheepishly, unaffected by the rage.", f->follower, 0, 0, TO_ROOM, 0);
        continue;
      }
      else
      {
        act("You give a battlecry, sounding your defiance!", f->follower, 0, 0, TO_CHAR, 0);
        act("$n gives a loud cry of agreement!", f->follower, 0, 0, TO_ROOM, 0);
        SET_BIT(f->follower->combat, COMBAT_RAGE1);
      }
    }
  }

  if (ch->has_skill(SKILL_BATTLECRY) > 40 && !number(0, 4))
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  else
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
  return eSUCCESS;
}

int do_berserk(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char name[256];
  int bSuccess = 0;
  int retval = 0;

  if (!ch->canPerform(SKILL_BERSERK, "You aren't crazy enough for that yet... try rage maybe...\r\n"))
  {
    return eFAILURE;
  }

  if (ch->getHP() == 1)
  {
    send_to_char("You are feeling too weak right now to work yourself up into "
                 "a frenzy.",
                 ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    if (ch->fighting)
      victim = ch->fighting;
    else
    {
      ch->sendln("Who do you want to go berserk on?");
      return eFAILURE;
    }
  }
  else if (ch->fighting)
  {
    ch->sendln("You're already in combat, and focus your energies on your current target instead.");
    victim = ch->fighting;
  }

  if (ch->in_room != victim->in_room)
  {
    ch->sendln("That person seems to have left.");
    return eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Aren't we funny today...");
    return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (!charge_moves(ch, SKILL_BERSERK))
    return eSUCCESS;

  if (!skill_success(ch, victim, SKILL_BERSERK))
  {
    act("You start freaking out on $N, but trip over your own feet!", ch, 0, victim, TO_CHAR, 0);
    act("$n starts freaking out on $N, but trips over $s own feet!", ch, 0, victim, TO_ROOM, NOTVICT);
    act_return ar = act("$n starts freaking out on you, but trips over $s own feet!", ch, 0, victim, TO_VICT, 0);
    retval = ar.retval;
    if (DC::isSet(retval, eVICT_DIED))
    {
      return retval;
    }

    ch->setSitting();
    if (ch->has_skill(SKILL_BERSERK) > 50 && !number(0, 5))
    {
      SET_BIT(ch->combat, COMBAT_BASH2);
      ch->sendln("Your advanced training in berserk allows you to roll with your fall and get up faster.");
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
    }
    else
    {
      SET_BIT(ch->combat, COMBAT_BASH1);
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
    }
  }
  else
  {
    act("You start FOAMING at the mouth, and you go BERSERK on $N!", ch, 0, victim, TO_CHAR, 0);
    act("$n starts FOAMING at the mouth, as $e goes BERSERK on $N!", ch, 0, victim, TO_ROOM, NOTVICT);
    act_return ar = act("$n starts FOAMING at the mouth, and goes BERSERK on you!", ch, 0, victim, TO_VICT, 0);
    retval = ar.retval;
    if (DC::isSet(retval, eVICT_DIED))
    {
      return retval;
    }

    bSuccess = 1;
  }

  if (bSuccess && !DC::isSet(ch->combat, COMBAT_RAGE1))
    SET_BIT(ch->combat, COMBAT_RAGE1);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  if (!ch->fighting)
    retval = attack(ch, victim, TYPE_UNDEFINED);

  if (!DC::isSet(retval, eCH_DIED))
  {
    if (!DC::isSet(retval, eVICT_DIED) && IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_WIMPY))
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);

    REMOVE_BIT(ch->combat, COMBAT_RAGE1);
    REMOVE_BIT(ch->combat, COMBAT_RAGE2);

    // I set the berserk bit AFTER the attack, to reduce the advantage
    // a bit.
    if (bSuccess)
    {
      SET_BIT(ch->combat, COMBAT_BERSERK);
      if (IS_PC(ch))
        GET_AC(ch) += 30; // we do this here, so we know if someone dies with COMBAT_BERSERK to
                          // give them their AC back
    }
  }
  return retval;
}

int do_headbutt(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char name[256];
  int retval = 0;

  if (!ch->canPerform(SKILL_HEADBUTT,
                      "You'd bonk yourself silly without proper training.\r\n"))
  {
    return eFAILURE;
  }

  one_argument(argument, name);

  victim = ch->get_char_room_vis(name);
  if (victim == nullptr)
    victim = ch->fighting;

  if (victim == nullptr)
  {
    ch->sendln("Headbutt whom?");
    return eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Aren't we funny today...");
    return eFAILURE;
  }

  if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_HUGE) && ch->has_skill(SKILL_HEADBUTT) < 86)
  {
    ch->sendln("You are too puny to headbutt someone that HUGE!");
    return eFAILURE;
  }

  if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_SWARM))
  {
    ch->sendln("You cannot pick just one to headbutt!");
    return eFAILURE;
  }

  if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_TINY))
  {
    act("$N's small size makes it impossible to target just $S head!", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_NOHEADBUTT))
  {
    ch->sendln("That would be like smashing your head into a wall!");
    return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (DC::isSet(victim->combat, COMBAT_BLADESHIELD1) || DC::isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Headbutting a bladeshielded opponent would be asking for decapitation!");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_HEADBUTT))
    return eSUCCESS;

  if (DC::isSet(victim->combat, COMBAT_BERSERK) && (IS_NPC(victim) || victim->has_skill(SKILL_BERSERK) > 80))
  {
    act("$N shakes off $n's attempt to immobilize them.", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("$N shakes off your attempt to immobilize them.", ch, nullptr, victim, TO_CHAR, NOTVICT);
    act("In your enraged state, you shake off $n's attempt to immobilize you.", ch, nullptr, victim, TO_VICT, 0);

    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
    return eSUCCESS;
  }

  int mod = 0;
  if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_HUGE))
    mod = -25;

  if (victim->equipment[WEAR_HEAD])
    mod -= victim->equipment[WEAR_HEAD]->obj_flags.value[0];

  mod -= (GET_DEX(victim) / 2);
  mod += (GET_STR(ch) / 2);

  if (mod > 0)
    mod = 0;

  int32_t get_weapon_bit(int weapon_type);
  int32_t weapon_bit;
  weapon_bit = get_weapon_bit(TYPE_CRUSH);

  if (!skill_success(ch, victim, SKILL_HEADBUTT, mod) || DC::isSet(victim->immune, weapon_bit) || do_frostshield(ch, victim))
  {
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
    retval = damage(ch, victim, 0, TYPE_CRUSH, SKILL_HEADBUTT, 0);
    // the damage call here takes care of starting combat and such
    retval = eSUCCESS;
  }
  else
  {
    if (affected_by_spell(victim, SKILL_BATTLESENSE) &&
        number(1, 100) < affected_by_spell(victim, SKILL_BATTLESENSE)->modifier)
    {
      act("$N's heightened battlesense sees your headbutt coming from a mile away.", ch, 0, victim, TO_CHAR, 0);
      act("$N's heightened battlesense sees $n's headbutt coming from a mile away.", ch, 0, victim, TO_ROOM, NOTVICT);
      act_return ar = act("Your heightened battlesense sees $n's headbutt coming from a mile away.", ch, 0, victim, TO_VICT, 0);
      retval = ar.retval;
      if (DC::isSet(retval, eVICT_DIED))
      {
        return retval;
      }

      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
      retval = damage(ch, victim, 0, TYPE_CRUSH, SKILL_HEADBUTT, 0);
      // the damage call here takes care of starting combat and such
      retval = eSUCCESS;
    }
    else
    {
      if (DC::isSet(victim->combat, COMBAT_BERSERK))
        REMOVE_BIT(victim->combat, COMBAT_BERSERK);

      set_fighting(victim, ch);
      WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);

      WAIT_STATE(victim, DC::PULSE_VIOLENCE * 2);
      SET_BIT(victim->combat, COMBAT_SHOCKED2);
      retval = damage(ch, victim, 50, TYPE_CRUSH, SKILL_HEADBUTT, 0);
      if (!SOMEONE_DIED(retval) && !number(0, 9) &&
          ch->equipment[WEAR_HEAD] && obj_index[ch->equipment[WEAR_HEAD]->item_number].virt == 508)
      {
        act("$n's spiked helmet crackles as it strikes $N's face!", ch, nullptr, victim, TO_ROOM, NOTVICT);
        act("$n's spiked helmet crackles as it strikes your face!", ch, nullptr, victim, TO_VICT, 0);
        act("Your spiked helmet crackles as it strikes $N's face!", ch, nullptr, victim, TO_CHAR, 0);
        //	retval = damage(ch, victim, 50, TYPE_PIERCE, TYPE_UNDEFINED, 0);
        retval = spell_shocking_grasp(50, ch, victim, 0, 60);
        // TWEAKME
      }
    }
  }

  return retval;
}

int do_bloodfury(Character *ch, char *argument, int cmd)
{
  struct affected_type af;
  float modifier;
  int duration = 42;

  if (!ch->canPerform(SKILL_BLOOD_FURY,
                      "You've no idea how to raise such bloodlust.\r\n"))
  {
    return eFAILURE;
  }

  if (affected_by_spell(ch, SKILL_BLOOD_FURY))
  {
    ch->sendln("Your body can not yet take the strain of another blood fury yet.");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BLOOD_FURY))
    return eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_BLOOD_FURY))
  {
    act("$n starts breathing heavily, then chokes and tries to clear $s head.", ch, nullptr, nullptr, TO_ROOM, NOTVICT);
    ch->sendln("You try to pysch yourself up and choke on the taste of blood.");
    duration = 1;
  }
  else
  {
    act("Panting heavily $n's eyes glaze red $e begins to move with renewed fury!",
        ch, nullptr, nullptr, TO_ROOM, NOTVICT);
    send_to_char("Your sight tinges red with the blood of battle and your "
                 "limbs feel strong with death and destruction deep within "
                 "your bones.\r\n",
                 ch);

    modifier = .2 + (.00375 * ch->has_skill(SKILL_BLOOD_FURY)); // mod = .2 - .5

    ch->addHP((float)GET_MAX_HIT(ch) * modifier);

    duration = 36 - (ch->getLevel() / 6);
  }

  af.type = SKILL_BLOOD_FURY;
  af.duration = duration;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;

  affect_to_char(ch, &af);

  return eSUCCESS;
}

int do_crazedassault(Character *ch, char *argument, int cmd)
{
  struct affected_type af;
  int duration = 20;
  if (affected_by_spell(ch, SKILL_CRAZED_ASSAULT) && ch->getLevel() < IMMORTAL)
  {
    ch->sendln("Your body is still recovering from your last crazed assault technique.");
    return eFAILURE;
  }

  if (!ch->canPerform(SKILL_CRAZED_ASSAULT, "You just aren't crazy enough...try assaulting old ladies.\r\n"))
    return eFAILURE;

  if (!charge_moves(ch, SKILL_CRAZED_ASSAULT))
    return eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_CRAZED_ASSAULT))
  {
    ch->sendln("You try to psyche yourself up for it but just can't muster the concentration.");
    duration = 1;
  }
  else
  {
    ch->sendln("Your mind focuses completely on hitting your opponent.");
    af.type = SKILL_CRAZED_ASSAULT;
    af.duration = 2;
    af.modifier = (ch->has_skill(SKILL_CRAZED_ASSAULT) / 5) + 5;
    af.location = APPLY_HITROLL;
    af.bitvector = -1;
    affect_to_char(ch, &af);
    duration = 16 - ch->has_skill(SKILL_CRAZED_ASSAULT) / 10;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  af.type = SKILL_CRAZED_ASSAULT;
  af.duration = duration;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  return eSUCCESS;
}

void rush_reset(varg_t arg1, void *arg2, void *arg3)
{
  Character *ch = arg1.ch;
  extern bool charExists(Character * ch);
  if (!charExists(ch))
    return;
  REMBIT(ch->affected_by, AFF_RUSH_CD);
}

int do_bullrush(Character *ch, char *argument, int cmd)
{
  char direction[MAX_INPUT_LENGTH];
  char who[MAX_INPUT_LENGTH];
  int dir = 0;
  int retval;
  Character *victim;

  if (ch->getHP() == 1)
  {
    ch->sendln("You are feeling too weak right now for rushing to and fro.");
    return eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_RUSH_CD))
  {
    ch->sendln("You must take a moment to gather your strength before another rush!");
    return eFAILURE;
  }
  if (!ch->canPerform(SKILL_BULLRUSH, "Closest yer gonna get to a bull right now is a Red one..and you have to drink it...\r\n"))
  {
    return eFAILURE;
  }

  argument = one_argument(argument, who);
  one_argument(argument, direction);
  if (!*direction)
  {
    ch->sendln("Bullrush which direction?");
    return eFAILURE;
  }
  if (!*who)
  {
    ch->sendln("Bullrush on.. who?");
    return eFAILURE;
  }

  for (int i = 0; i < 6; i++)
  {
    if (!str_prefix(direction, dirs[i]))
    {
      dir = i + 1;
      break;
    }
  }

  if (!dir)
  {
    ch->sendln("Bullrush a valid direction dumb barb...like north maybe?");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BULLRUSH))
    return eSUCCESS;

  // before we move anyone, we need to check for any spec procs in the
  // room like guild guards
  retval = ch->special("", dir);
  if (DC::isSet(retval, eSUCCESS) || DC::isSet(retval, eCH_DIED))
    return retval;
  SETBIT(ch->affected_by, AFF_RUSH_CD);
  extern void addtimer(struct timer_data * add);

  // Reset bullrush AFF in 5 seconds
  struct timer_data *timer;
#ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
#else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
#endif
  timer->arg1.ch = ch;
  timer->function = rush_reset;
  timer->timeleft = 5;
  addtimer(timer);

  retval = attempt_move(ch, dir);
  if (SOMEONE_DIED(retval))
    return retval;

  if (!(victim = ch->get_char_room_vis(who)))
  {
    ch->sendln("You charge in, but are left confused by the complete lack of such a target!");
    //     WAIT_STATE(ch,DC::PULSE_VIOLENCE/2);
    return eFAILURE;
  }
  //  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  if (!skill_success(ch, victim, SKILL_BULLRUSH))
  {
    ch->sendln("You rush in madly and fail to find your target!");
    act("$n rushes into the room with nostrils flaring then looks around sheepishly.",
        ch, nullptr, nullptr, TO_ROOM, NOTVICT);
    return eFAILURE;
  }

  if (!victim || victim == ch)
  {
    ch->sendln("You successfully rush in and bushwack... the air.");
    return eFAILURE;
  }

  act("$n rushes into the room with an amazingly violent speed!",
      ch, nullptr, nullptr, TO_ROOM, NOTVICT);
  return attack(ch, victim, TYPE_UNDEFINED);
}

int do_ferocity(Character *ch, char *argument, int cmd)
{
  struct affected_type af;

  if (!ch->canPerform(SKILL_FEROCITY, "You're just not angry enough!\r\n"))
  {
    return eFAILURE;
  }
  if (affected_by_spell(ch, SKILL_FEROCITY_TIMER))
  {
    ch->sendln("It is too soon to try and rile up the masses!");
    return eFAILURE;
  }
  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You have no group to inspire.");
    return eFAILURE;
  }

  int grpsize = 0;
  for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
  {
    if (tmp_char == ch)
      continue;
    if (!ARE_GROUPED(ch, tmp_char))
      continue;
    grpsize++;
  }

  if (!charge_moves(ch, SKILL_FEROCITY, grpsize))
    return eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_FEROCITY))
  {
    ch->sendln("Guess you just weren't that angry.");
    act("$n tries to rile you up but just seems to be pouty.", ch, 0, 0, TO_ROOM, 0);
  }
  else
  {
    act("$n lets out a deafening roar!", ch, 0, 0, TO_ROOM, 0);
    ch->sendln("Your heart beats adrenaline through your body and you roar with ferocity!");

    af.type = SKILL_FEROCITY_TIMER;
    af.duration = 1 + ch->has_skill(SKILL_FEROCITY) / 10;
    af.location = 0;
    af.bitvector = -1;
    af.modifier = 0;
    affect_to_char(ch, &af);

    for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
    {
      if (tmp_char == ch)
        continue;
      if (!ARE_GROUPED(ch, tmp_char))
        continue;

      affect_from_char(tmp_char, SKILL_FEROCITY, SUPPRESS_MESSAGES);
      affect_from_char(tmp_char, SKILL_FEROCITY, SUPPRESS_MESSAGES);
      act("$n's fierce roar gets your adrenaline pumping!", ch, 0, tmp_char, TO_VICT, 0);

      af.type = SKILL_FEROCITY;
      af.duration = 1 + ch->has_skill(SKILL_FEROCITY) / 10;
      af.modifier = 31 + ch->has_skill(SKILL_FEROCITY) / 2;
      af.location = APPLY_HIT;
      af.bitvector = -1;
      affect_to_char(tmp_char, &af);
      af.modifier = 1 + ch->has_skill(SKILL_FEROCITY) / 15;
      af.location = APPLY_HP_REGEN;
      affect_to_char(tmp_char, &af);
    }
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  return eSUCCESS;
}

void barb_magic_resist(Character *ch, int old, int nw)
{
  int bonus = 0, i;
  int oldbonus = (old / 10) + 1;
  bonus = (nw / 10 + 1) - oldbonus;
  if (bonus)
    for (i = 0; i <= SAVE_TYPE_MAX; i++)
      ch->saves[i] += bonus;
}

int do_knockback(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char buf[MAX_STRING_LENGTH], where[MAX_STRING_LENGTH], who[MAX_STRING_LENGTH];
  int dir = 0;
  int retval, dam, dampercent, learned;

  if (ch->getHP() == 1)
  {
    ch->sendln("You are feeling too weak right now to smash into anybody.");
    return eFAILURE;
  }

  if (!ch->canPerform(SKILL_KNOCKBACK, "You'd bounce off of your opponent before you caused any damage.\r\n"))
  {
    return eFAILURE;
  }

  argument = one_argument(argument, who);
  one_argument(argument, where);

  if (!*who)
  {
    ch->sendln("Knockback whom?");
    return eFAILURE;
  }

  victim = ch->get_char_room_vis(who);

  if (victim == nullptr)
    victim = ch->fighting;

  if (victim == nullptr)
  {
    ch->sendln("Knockback whom?");
    return eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Where do you want to go today?");
    return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (!charge_moves(ch, SKILL_KNOCKBACK))
    return eSUCCESS;

  bool victim_paralyzed = false;
  affected_type *af;
  if ((af = affected_by_spell(victim, SPELL_PARALYZE)))
  {
    victim_paralyzed = true;
    if (af->duration >= 1)
      af->duration--;
  }
  else
  {
    if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_HUGE))
    {
      send_to_char("You are too tiny to knock someone that HUGE anywhere!\n\r",
                   ch);
      return eFAILURE;
    }

    if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_SWARM))
    {
      ch->sendln("You cannot pick just one to knockback!");
      return eFAILURE;
    }

    if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_TINY))
    {
      act("$N would evade your knockback attempt with ease!", ch, 0, victim, TO_CHAR, 0);
      return eFAILURE;
    }

    if (IS_AFFECTED(victim, AFF_STABILITY) && number(0, 3) == 0)
    {
      act("You bounce off of $N and crash into the ground.", ch, 0, victim, TO_CHAR, 0);
      act("$n bounces off of $N and crashes into the ground.", ch, 0, victim, TO_ROOM, NOTVICT);
      act("$n bounces off of you and crashes into the ground.", ch, 0, victim, TO_VICT, 0);
      WAIT_STATE(ch, DC::PULSE_VIOLENCE);
      return eFAILURE;
    }
  }

  learned = ch->has_skill(SKILL_KNOCKBACK);
  dam = 100;

  if (*where)
  {
    if (learned < 80)
      ch->sendln("You're not good enough to direct your smashes, so you just let it fly!");
    else
    {
      for (int i = 0; i < 6; i++)
      {
        if (!str_prefix(where, dirs[i]))
        {
          dir = i + 1;
          break;
        }
      }
      learned -= 20;
    }
  }

  if (!dir)
    dir = number(1, 6);
  dir--;
  dampercent = 0;
  if (ch->height > 102)
    dampercent += 15;
  else if (ch->height > 42)
    dampercent += 7;
  if (victim->height < 42)
    dampercent += 7;
  else if (victim->height < 102)
    dampercent += 15;

  dam = (int)(dam * (1.0 + dampercent / 100.0));

  char buf2[MAX_STRING_LENGTH], dammsg[20];
  int prevhps = victim->getHP();

  if (!victim_paralyzed && !skill_success(ch, victim, SKILL_KNOCKBACK, 0 - (learned / 4 * 3)))
  {
    act("You lunge forward in an attempt to smash $N but fall, missing $M completely.", ch, 0, victim, TO_CHAR, 0);
    act("$n lunges forward in an attempt to smash into $N but falls flat on $s face.", ch, 0, victim, TO_ROOM, NOTVICT);
    act_return ar = act("$n lunges forward in an attempt to smash into you but falls flat on $s face, missing completely.", ch, 0, victim, TO_VICT, 0);
    retval = ar.retval;
    if (DC::isSet(retval, eVICT_DIED))
    {
      return retval;
    }

    ch->setSitting();
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    retval = damage(ch, victim, 0, TYPE_CRUSH, SKILL_KNOCKBACK, 0);
    return eFAILURE;
  }
  else if (!victim_paralyzed && affected_by_spell(victim, SKILL_BATTLESENSE) &&
           number(1, 100) < affected_by_spell(victim, SKILL_BATTLESENSE)->modifier)
  {
    act("$N's heightened battlesense sees your smash coming from a mile away and $E easily sidesteps it.", ch, 0, victim, TO_CHAR, 0);
    act("$N's heightened battlesense sees $n's smash coming from a mile away and $N easily sidesteps it.", ch, 0, victim, TO_ROOM, NOTVICT);
    act_return ar = act("Your heightened battlesense sees $n's smash coming from a mile away and you easily sidestep it.", ch, 0, victim, TO_VICT, 0);
    retval = ar.retval;
    if (DC::isSet(retval, eVICT_DIED))
    {
      return retval;
    }

    ch->setSitting();
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    return eFAILURE;
  }
  else if (CAN_GO(victim, dir) &&
           !affected_by_spell(victim, SPELL_IRON_ROOTS) &&
           !DC::isSet(DC::getInstance()->world[EXIT(victim, dir)->to_room].room_flags, IMP_ONLY) &&
           !DC::isSet(DC::getInstance()->world[EXIT(victim, dir)->to_room].room_flags, NO_TRACK) &&
           (!IS_AFFECTED(victim, AFF_CHAMPION) || champion_can_go(EXIT(victim, dir)->to_room)) &&
           class_can_go(GET_CLASS(victim), EXIT(victim, dir)->to_room))
  {
    // need to do more checks on if the victim can actually be knocked into
    // the room?
    char temp[256]; // what did my innocent bugfix ever do to you?
    sprintf(temp, "%s", GET_SHORT(victim));
    retval = damage(ch, victim, dam, TYPE_CRUSH, SKILL_KNOCKBACK, 0);
    if (SOMEONE_DIED(retval))
    {
      sprintf(buf, "You smash %s apart!", temp);
      act(buf, ch, 0, 0, TO_CHAR, 0);
      sprintf(buf, "$n smashes %s to pieces!", temp);
      act(buf, ch, 0, 0, TO_ROOM, 0);
      return retval; // this too, just in case it gets called from  a
                     // proc later on, it returns correct stuff
    }
    else
    {
      sprintf(dammsg, "$B%d$R", prevhps - victim->getHP());
      sprintf(buf2, "Your smash for | damage sends %s reeling %s.", GET_SHORT(victim), dirs[dir]);
      sprintf(buf, "Your smash sends %s reeling %s.", GET_SHORT(victim), dirs[dir]);
      send_damage(buf2, ch, 0, victim, dammsg, buf, TO_CHAR);
      sprintf(buf2, "%s smashes into you for | damage, sending you reeling %s.", GET_NAME(ch), dirs[dir]);
      sprintf(buf, "%s smashes into you, sending you reeling %s.", GET_NAME(ch), dirs[dir]);
      send_damage(buf2, ch, 0, victim, dammsg, buf, TO_VICT);

      if (selfpurge)
        return eSUCCESS | eVICT_DIED;
      sprintf(buf2, "%s smashes into %s for | damage and sends $S ass reeling to the %s.", GET_NAME(ch), GET_SHORT(victim), dirs[dir]);
      sprintf(buf, "%s smashes into %s and sends $S ass reeling to the %s.", GET_NAME(ch), GET_SHORT(victim), dirs[dir]);
      send_damage(buf2, ch, 0, victim, dammsg, buf, TO_ROOM);

      if (victim->fighting)
      {
        if (IS_NPC(victim))
        {
          victim->add_memory(GET_NAME(ch), 'h');
          remove_memory(victim, 'f');
        }

        Character *tmp;
        for (tmp = DC::getInstance()->world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
          if (tmp->fighting == victim)
            stop_fighting(tmp);
        stop_fighting(victim);
      }
      move_char(victim, (DC::getInstance()->world[(ch)->in_room].dir_option[dir])->to_room);
    }
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    return eSUCCESS;
  }
  else
  {
    char temp[256];
    sprintf(temp, "%s", GET_SHORT(victim));
    retval = damage(ch, victim, dam, TYPE_CRUSH, SKILL_KNOCKBACK, 0);
    if (SOMEONE_DIED(retval))
    {
      sprintf(buf, "You smash %s apart!", temp);
      act(buf, ch, 0, 0, TO_CHAR, 0);
      sprintf(buf, "$n smashes %s to pieces!", temp);
      act(buf, ch, 0, 0, TO_ROOM, 0);
      return retval;
    }
    else
    {
      sprintf(dammsg, "$B%d$R", prevhps - victim->getHP());
      send_damage("$N backpeddles across the room due to $n's smash for | damage.", ch, 0, victim, dammsg,
                  "$N backpessles across the room due to $n's smash.", TO_ROOM);
      send_damage("$N barely keeps $S footing, stumbling backwards after your smash and taking | damage.", ch, 0, victim, dammsg,
                  "$N barely keeps $S footing, stumbling backwards after your smash.", TO_CHAR);
      send_damage("$n knocks you back across the room for | damage.", ch, 0, victim, dammsg,
                  "$n knocks you back across the room.", TO_VICT);
      WAIT_STATE(ch, DC::PULSE_VIOLENCE);
      return attack(victim, ch, TYPE_UNDEFINED);
    }
  }
  return eSUCCESS;
}

int do_primalfury(Character *ch, char *argument, int cmd)
{
  struct affected_type af;

  if (!ch->has_skill(SKILL_PRIMAL_FURY))
  {
    ch->sendln("You don't know how to.");
    return eSUCCESS;
  }
  if (affected_by_spell(ch, SKILL_PRIMAL_FURY))
  {
    ch->sendln("You must wait before using this ability again.");
    return eSUCCESS;
  }
  if (!ch->fighting || GET_POS(ch) != position_t::FIGHTING)
  {
    ch->sendln("You must be in combat in order to use this ability.");
    return eSUCCESS;
  }
  if (!charge_moves(ch, SKILL_PRIMAL_FURY))
    return eSUCCESS;

  if (GET_RAW_STR(ch) < 16)
  {
    ch->sendln("You do not possess sufficient strength to attempt this feat.");
    return eSUCCESS;
  }

  // Timer applies to both success and failure, so it goes here.
  af.type = SKILL_PRIMAL_FURY;
  af.duration = 40;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  if (!skill_success(ch, nullptr, SKILL_PRIMAL_FURY))
  {
    affect_to_char(ch, &af);
    ch->sendln("You attempt to let forth a primal scream, but manage only a squeak...how embarassing!");
    act("$n attempts to let forth a primal scream, but manages only a squeak...how embarassing!", ch, nullptr, nullptr, TO_ROOM, 0);
    ch->setMove(ch->getMove() / 2.0);
    return eSUCCESS;
  }

  // Success below.

  ch->decrementMove(40);
  if (number(1, 101) > (5 + ch->has_skill(SKILL_PRIMAL_FURY) / 5))
  { // Str loss.
    GET_RAW_STR(ch) -= 1;
    affect_modify(ch, APPLY_STR, 0, -1, true);
    send_to_char("You lose one point of strength.", ch);
    logf(OVERSEER, LogChannels::LOG_MORTAL, "Statloss: %s lost one point of strength through primal fury.", GET_NAME(ch));
  }

  // rest already set
  af.duration = 2 + ch->has_skill(SKILL_PRIMAL_FURY) / 30;
  af.bitvector = AFF_PRIMAL_FURY;
  affect_to_char(ch, &af);

  act("$n lets forth a primal scream of anger and begins to fight with terrible fury!", ch, nullptr, nullptr, TO_ROOM, 0);
  ch->sendln("You let forth a primal scream of anger and fall upon your enemies with a terrible fury!");

  return eSUCCESS;
}

int do_pursue(Character *ch, char *argument, int cmd)
{
  if (!ch->has_skill(SKILL_PURSUIT))
  {
    ch->sendln("You don't know how to.");
    return eFAILURE;
  }

  if (affected_by_spell(ch, SKILL_PURSUIT))
  {
    ch->sendln("You will no longer pursue your victims.");
    affect_from_char(ch, SKILL_PURSUIT);
  }
  else
  {
    ch->sendln("You will pursue your victims.");
    affected_type af;
    af.type = SKILL_PURSUIT;
    af.duration = -1;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = -1;
    affect_to_char(ch, &af);
  }

  return eSUCCESS;
}
