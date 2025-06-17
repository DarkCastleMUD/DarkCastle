/************************************************************************
| $Id: cl_warrior.cpp,v 1.86 2014/08/21 01:59:39 jhhudso Exp $
| cl_warrior.C
| Description:  This file declares implementation for warrior-specific
|   skills.
*/

#include <algorithm>

#include "DC/obj.h"
#include "DC/structs.h"
#include "DC/character.h"
#include "DC/player.h"
#include "DC/fight.h"
#include "DC/utility.h"
#include "DC/spells.h"
#include "DC/handler.h"
#include "DC/connect.h"
#include "DC/mobile.h"
#include "DC/room.h"
#include "DC/act.h"
#include "DC/db.h"
#include "DC/DC.h"
#include "DC/returnvals.h"
#include "DC/race.h"
#include <iostream>
#include "DC/interp.h"
#include "DC/spells.h"
#include "DC/const.h"
#include "DC/move.h"

/************************************************************************
| OFFENSIVE commands.  These are commands that should require the
|   victim to retaliate.
*/
int do_kick(Character *ch, char *argument, int cmd)
{
  Character *victim;
  Character *next_victim;
  char name[256];
  int dam;
  int retval;

  if (!ch->canPerform(SKILL_KICK))
  {
    ch->sendln("You will have to study from a master before you can use this.");
    return eFAILURE;
  }

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    if (ch->fighting)
    {
      victim = ch->fighting;
    }
    else
    {
      ch->sendln("Your foot comes up, but there's nobody there...");
      return eFAILURE;
    }
  }

  if (victim == ch)
  {
    ch->sendln("You kick yourself, metaphorically speaking.");
    return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Kicking a bladeshielded opponent would be a good way to lose a leg!");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_KICK))
    return eSUCCESS;

  WAIT_STATE(ch, (int)(DC::PULSE_VIOLENCE * 1.5));

  if (!skill_success(ch, victim, SKILL_KICK))
  {
    dam = 0;
    retval = damage(ch, victim, 0, TYPE_BLUDGEON, SKILL_KICK, 0);
    if (SOMEONE_DIED(retval))
      return retval;
  }
  else
  {
    if (victim->affected_by_spell(SKILL_BATTLESENSE) &&
        number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act("$N's heightened battlesense sees your kick coming from a mile away.", ch, 0, victim, TO_CHAR, 0);
      act("Your heightened battlesense sees $n's kick coming from a mile away.", ch, 0, victim, TO_VICT, 0);
      act("$N's heightened battlesense sees $n's kick coming from a mile away.", ch, 0, victim, TO_ROOM, NOTVICT);
      dam = 0;
    }
    else
      dam = (GET_DEX(ch) * 3) + (GET_STR(ch) * 2) + (ch->has_skill(SKILL_KICK));
    retval = damage(ch, victim, dam, TYPE_BLUDGEON, SKILL_KICK, 0);
    if (SOMEONE_DIED(retval))
      return retval;
  }

  // if our boots have a combat proc, and we did damage, let'um have it!
  if (dam && ch->equipment[WEAR_FEET])
  {
    retval = weapon_spells(ch, victim, WEAR_FEET);
    if (SOMEONE_DIED(retval))
      return retval;
    // leaving this built in proc here incase some new stuff is added, like kick_their_head_off
    if (DC::getInstance()->obj_index[ch->equipment[WEAR_FEET]->vnum].combat_func)
    {
      retval = ((*DC::getInstance()->obj_index[ch->equipment[WEAR_FEET]->vnum].combat_func)(ch, ch->equipment[WEAR_FEET], 0, "", ch));
    }
    if (SOMEONE_DIED(retval))
      return retval;
  }

  // Extra kick targeting main opponent for monks
  if ((GET_CLASS(ch) == CLASS_MONK) && ch->fighting && ch->in_room == ch->fighting->in_room)
  {
    next_victim = ch->fighting;
    if (!skill_success(ch, next_victim, SKILL_KICK))
    {
      dam = 0;
      retval = damage(ch, next_victim, 0, TYPE_UNDEFINED, SKILL_KICK, 0);
    }
    else
    {
      dam = (GET_DEX(ch) * 2) + (GET_STR(ch)) + (ch->has_skill(SKILL_KICK) / 2);
      retval = damage(ch, next_victim, dam, TYPE_UNDEFINED, SKILL_KICK, 0);
    }
    if (SOMEONE_DIED(retval))
      return retval;
    // if our boots have a combat proc, and we did damage, let'um have it!
    if (dam && ch->equipment[WEAR_FEET])
    {
      retval = weapon_spells(ch, next_victim, WEAR_FEET);
      if (SOMEONE_DIED(retval))
        return retval;
      // leaving this built in proc here incase some new stuff is added, like kick_their_head_off
      if (DC::getInstance()->obj_index[ch->equipment[WEAR_FEET]->vnum].combat_func)
      {
        retval = ((*DC::getInstance()->obj_index[ch->equipment[WEAR_FEET]->vnum].combat_func)(ch, ch->equipment[WEAR_FEET], 0, "", ch));
      }
    }
  }

  return retval;
}

int do_deathstroke(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char name[256];
  int dam, attacktype;
  int retval;
  int failchance = 25;

  if (!ch->canPerform(SKILL_DEATHSTROKE))
  {
    ch->sendln("You have no idea how to deathstroke.");
    return eFAILURE;
  }

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    if (ch->fighting)
    {
      victim = ch->fighting;
    }
    else
    {
      ch->sendln("Deathstroke whom?");
      return eFAILURE;
    }
  }

  if (victim == ch)
  {
    ch->sendln("The world would be a better place...");
    return eFAILURE;
  }

  if (!ch->equipment[WIELD])
  {
    ch->sendln("You must be wielding a weapon to deathstrike someone.");
    return eFAILURE;
  }

  if (victim->getPosition() > position_t::SITTING)
  {
    ch->sendln("Your opponent isn't in a vulnerable enough position!");
    return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Stroking a bladeshielded opponent would be suicide!");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_DEATHSTROKE))
    return eSUCCESS;

  int i = ch->has_skill(SKILL_DEATHSTROKE);
  if (i > 40)
    failchance -= 5;
  if (i > 60)
    failchance -= 5;
  if (i > 80)
    failchance -= 5;
  if (i > 90)
    failchance -= 5;

  int to_dam = GET_DAMROLL(ch);
  if (IS_NPC(victim))
    to_dam = (int)((float)to_dam * .8);

  dam = dice(ch->equipment[WIELD]->obj_flags.value[1],
             ch->equipment[WIELD]->obj_flags.value[2]);
  dam += to_dam;
  dam *= (ch->getLevel() / 6); // 10 at level 50

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
  attacktype = ch->equipment[WIELD]->obj_flags.value[3] + TYPE_HIT;

  if (!skill_success(ch, victim, SKILL_DEATHSTROKE, -25))
  {
    retval = damage(ch, victim, 0, attacktype, SKILL_DEATHSTROKE, 0);
    if (number(1, 100) > failchance)
    {
      ch->sendln("You manage to retain your balance!");
      return eFAILURE;
    }
    dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
    ch->removeHP(dam);
    update_pos(ch);
    if (ch->getPosition() == position_t::DEAD)
    {
      fight_kill(ch, ch, TYPE_CHOOSE, 0);
      return eSUCCESS | eCH_DIED;
    }
  }
  else
  {
    if (victim->affected_by_spell(SKILL_BATTLESENSE) &&
        number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act("$N's heightened battlesense somehow notices your deathstroke coming from a mile away.", ch, 0, victim, TO_CHAR, 0);
      act("Your heightened battlesense somehow notices $n's deathstroke coming from a mile away.", ch, 0, victim, TO_VICT, 0);
      act("$N's heightened battlesense somehow notices $n's deathstroke coming from a mile away.", ch, 0, victim, TO_ROOM, NOTVICT);
      dam = 0;
    }
    retval = damage(ch, victim, dam, attacktype, SKILL_DEATHSTROKE, 0);
  }

  return retval;
}

int do_retreat(Character *ch, char *argument, int cmd)
{
  int attempt;
  char buf[MAX_INPUT_LENGTH];
  // Azrack -- retval should be initialized to something
  int retval = 0;
  Character *chTemp, *loop_ch;

  int is_stunned(Character * ch);

  if (is_stunned(ch))
    return eFAILURE;

  if (!ch->canPerform(SKILL_RETREAT))
  {
    ch->sendln("You dunno how...better flee instead.");
    return eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_NO_FLEE))
  {
    if (ch->affected_by_spell(SPELL_IRON_ROOTS))
      ch->sendln("The roots bracing your legs make it impossible to run!");
    else
      ch->sendln("Your legs are too tired for running away!");
    return eFAILURE;
  }

  // after the below line, buf should contain the argument, hopefully
  // a direction to retreat.
  one_argument(argument, buf);

  if ((attempt = consttype(buf, dirs)) == -1)
  {
    send_to_char("You are supposed to retreat towards a specific "
                 "direction...\r\n",
                 ch);
    return eFAILURE;
  }

  if (!CAN_GO(ch, attempt))
  {
    ch->sendln("You cannot retreat in that direction.");
    return eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  if (!charge_moves(ch, SKILL_RETREAT))
    return eSUCCESS;

  // failure
  if (!skill_success(ch, nullptr, SKILL_RETREAT))
  {
    act("$n tries to retreat, but pays a heavy price for $s hesitancy.\r\n"
        "$n falls to the ground!",
        ch, 0, 0, TO_ROOM, INVIS_NULL);
    act("You try to retreat, but pay a heavy price for your hesitancy.\r\n"
        "You fall to the ground!",
        ch, 0, 0, TO_CHAR, INVIS_NULL);
    ch->setSitting();
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    return eFAILURE;
  }

  //   if (CAN_GO(ch, attempt))
  {
    act("$n tries to beat a hasty retreat.", ch, 0, 0, TO_ROOM,
        INVIS_NULL);
    ch->sendln("You try to beat a hasty retreat....");

    // check for any spec procs
    retval = ch->special("", attempt + 1);
    if (isSet(retval, eSUCCESS) || isSet(retval, eCH_DIED))
      return retval;

    retval = attempt_move(ch, attempt + 1, 1);
    if (isSet(retval, eSUCCESS))
    {
      // They got away.  Stop fighting for everyone not in the new room from fighting
      for (chTemp = combat_list; chTemp; chTemp = loop_ch)
      {
        loop_ch = chTemp->next_fighting;
        if (chTemp->fighting == ch && chTemp->in_room != ch->in_room)
          stop_fighting(chTemp);
      } // for
      stop_fighting(ch);
      // The escape has succeded
      return retval;
    }
    else
    {
      if (!isSet(retval, eCH_DIED))
        act("$n tries to retreat, but is too exhausted!", ch, 0, 0, TO_ROOM, INVIS_NULL);
      return retval;
    }
  }

  // No exits were found
  ch->sendln("You cannot retreat in that direction.!");
  return eFAILURE;
}

int do_hitall(Character *ch, char *argument, int cmd)
{
  if (IS_PC(ch) && ch->getLevel() < ARCHANGEL && !ch->has_skill(SKILL_HITALL))
  {
    ch->sendln("You better learn how to first...");
    return eFAILURE;
  }

  if (ch->getHP() == 1)
  {
    ch->sendln("You are too weak to do this right now.");
    return eFAILURE;
  }

  if (!can_attack(ch))
    return eFAILURE;

  if (!charge_moves(ch, SKILL_HITALL))
    return eSUCCESS;

  // TODO - I'm pretty sure we can remove this check....don't feel like checking right now though
  if (isSet(ch->combat, COMBAT_HITALL))
  {
    ch->sendln("You can't disengage!");
    return eFAILURE;
  }

  if (!skill_success(ch, nullptr, SKILL_HITALL))
  {

    act("You start swinging like a madman, but trip over your own feet!",
        ch, 0, 0, TO_CHAR, 0);
    act("$n starts swinging like a madman, but trips over $s own feet!", ch,
        0, 0, TO_ROOM, 0);

    room_mobs_only_hate(ch);

    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1);
  }
  else
  {
    act("You start swinging like a MADMAN!", ch, 0, 0, TO_CHAR, 0);
    act("$n starts swinging like a MADMAN!", ch, 0, 0, TO_ROOM, 0);
    SET_BIT(ch->combat, COMBAT_HITALL);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);

    const auto &character_list = DC::getInstance()->character_list;
    for_each(character_list.begin(), character_list.end(), [&ch](Character *vict)
             {
			if (vict && vict != (Character *) 0x95959595 && ch->in_room == vict->in_room && !ARE_GROUPED(ch, vict) && vict != ch && can_be_attacked(ch, vict)) {
				int retval = one_hit(ch, vict, TYPE_UNDEFINED, FIRST);
				if (isSet(retval, eCH_DIED)) {
					REMOVE_BIT(ch->combat, COMBAT_HITALL);
					return false;
				}
			}
			return true; });
    REMOVE_BIT(ch->combat, COMBAT_HITALL);
  }
  return eSUCCESS;
}

int do_bash(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char name[256];
  int retval;
  int hit = 0;

  one_argument(argument, name);

  if (IS_AFFECTED(ch, AFF_CHARM))
    return eFAILURE;

  if (!ch->canPerform(SKILL_BASH, "You don't know how to bash!\r\n"))
  {
    return eFAILURE;
  }

  //    if (IS_PC(ch))
  if (!ch->equipment[WIELD])
  {
    ch->sendln("You need to wield a weapon, to make it a success.");
    return eFAILURE;
  }

  if (*name)
    victim = ch->get_char_room_vis(name);
  else
    victim = ch->fighting;

  if (!victim)
  {
    ch->sendln("Bash whom?");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BASH))
    return eSUCCESS;

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_HUGE))
  {
    ch->sendln("You cannot bash something that HUGE!");
    return eFAILURE;
  }

  if (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_SWARM))
  {
    ch->sendln("You cannot pick just one to bash!");
    return eFAILURE;
  }

  if (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_TINY))
  {
    ch->sendln("You cannot target something that tiny!");
    return eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Aren't we funny today...");
    return eFAILURE;
  }

  set_cantquit(ch, victim);

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Bashing a bladeshielded opponent would be suicide!");
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_IRON_ROOTS))
  {
    act("You try to bash $N but tree roots around $S legs keep him upright.", ch, 0, victim, TO_CHAR, 0);
    act("$n bashes you but the roots around your legs keep you from falling.", ch, 0, victim, TO_VICT, 0);
    act("The tree roots support $N keeping $M from sprawling after $n's bash.", ch, 0, victim, TO_ROOM, NOTVICT);
    WAIT_STATE(ch, 2 * DC::PULSE_VIOLENCE);
    return eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_STABILITY) && number(0, 3) == 0)
  {
    act("You bounce off of $N and crash into the ground.", ch, 0, victim, TO_CHAR, 0);
    act("$n bounces off of you and crashes into the ground.", ch, 0, victim, TO_VICT, 0);
    act("$n bounces off of $N and crashes into the ground.", ch, 0, victim, TO_ROOM, NOTVICT);
    WAIT_STATE(ch, 2 * DC::PULSE_VIOLENCE);
    return eFAILURE;
  }

  int modifier = 0;
  // half as accurate without a shield
  if (!ch->equipment[WEAR_SHIELD])
  {
    modifier = -20;
    // but 3/4 as accurate with a 2hander
    if (ch->equipment[WIELD] && isSet(ch->equipment[WIELD]->obj_flags.extra_flags, ITEM_TWO_HANDED))
    {
      modifier += 10;
      // if the basher is a barb though, give them the full effect
      if (GET_CLASS(ch) == CLASS_BARBARIAN)
        modifier += 10;
    }
  }

  switch (GET_CLASS(victim))
  {
  case CLASS_MAGIC_USER:
  case CLASS_CLERIC:
  case CLASS_DRUID:
  case CLASS_BARD:
    modifier += 8;
    break;
  case CLASS_THIEF:
  case CLASS_RANGER:
  case CLASS_PALADIN:
  case CLASS_ANTI_PAL:
  case CLASS_PSIONIC:
  case CLASS_NECROMANCER:
    modifier += 4;
    break;
  default:
    break;
  }

  int stat_mod = ch->get_stat(attribute_t::STRENGTH) - victim->get_stat(attribute_t::STRENGTH);
  if (stat_mod > 10)
    stat_mod = 10;
  if (stat_mod < -10)
    stat_mod = -10;

  modifier += stat_mod;

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);

  if (!skill_success(ch, victim, SKILL_BASH, modifier))
  {
    ch->setSitting();
    SET_BIT(ch->combat, COMBAT_BASH1);
    retval = damage(ch, victim, 0, TYPE_BLUDGEON, SKILL_BASH, 0);
  }
  else
  {
    if (victim->affected_by_spell(SKILL_BATTLESENSE) &&
        number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act("$N's heightened battlesense sees your bash coming from a mile away.", ch, 0, victim, TO_CHAR, 0);
      act("Your heightened battlesense sees $n's bash coming from a mile away.", ch, 0, victim, TO_VICT, 0);
      act("$N's heightened battlesense sees $n's bash coming from a mile away.", ch, 0, victim, TO_ROOM, NOTVICT);
      retval = damage(ch, victim, 0, TYPE_BLUDGEON, SKILL_BASH, 0);
    }
    else
    {
      victim->setSitting();
      SET_BIT(victim->combat, COMBAT_BASH1);
      retval = damage(ch, victim, 25, TYPE_BLUDGEON, SKILL_BASH, 0);
    }
    if (!(retval & eEXTRA_VALUE))
    {
      hit = 1;
      // if they already have 2 rounds of wait, only tack on 1 instead of 2

      if (!SOMEONE_DIED(retval))
        if (victim->desc)
          WAIT_STATE(victim, DC::PULSE_VIOLENCE * 2);
    }
  }

  if (SOMEONE_DIED(retval))
    return retval;

  // if our shield has a combat proc and we hit them, let'um have it!
  if (hit && ch->equipment[WEAR_SHIELD])
  {
    if (DC::getInstance()->obj_index[ch->equipment[WEAR_SHIELD]->vnum].combat_func)
    {
      retval = ((*DC::getInstance()->obj_index[ch->equipment[WEAR_SHIELD]->vnum].combat_func)(ch, ch->equipment[WEAR_SHIELD], 0, "", ch));
    }
  }

  return retval;
}

int do_redirect(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char name[256];

  if (!ch->canPerform(SKILL_REDIRECT, "You aren't skilled enough to change opponents midfight!\r\n"))
    return eFAILURE;

  one_argument(argument, name);

  victim = ch->get_char_room_vis(name);

  if (victim == nullptr)
  {
    ch->sendln("Redirect your attacks to whom?");
    return eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Aren't we funny today...");
    return eFAILURE;
  }

  if (!ch->fighting)
  {
    ch->sendln("You're not fighting anyone to begin with!");
    return eFAILURE;
  }

  if (!victim->fighting)
  {
    ch->sendln("He isn't bothering anyone, you have enough problems as it is anyways!");
    return eFAILURE;
  }
  if (ch->fighting == victim)
  {
    act("You are already fighting $N.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  if (!can_be_attacked(ch, victim))
    return eFAILURE;

  if (!charge_moves(ch, SKILL_REDIRECT))
    return eSUCCESS;

  if (!skill_success(ch, victim, SKILL_REDIRECT))
  {
    act("$n tries to redirect his attacks but $N won't allow it.", ch, nullptr, ch->fighting, TO_VICT, 0);
    act("You try to redirect your attacks to $N but are blocked.", ch, nullptr, victim, TO_CHAR, 0);
    act("$n tries to redirect his attacks elsewhere, but $N wont allow it.", ch, nullptr, victim, TO_ROOM, NOTVICT);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  }
  else
  {
    act("$n redirects his attacks at YOU!", ch, nullptr, victim, TO_VICT, 0);
    act("You redirect your at attacks at $N!", ch, nullptr, victim, TO_CHAR, 0);
    act("$n redirects his attacks at $N!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    stop_fighting(ch);
    set_fighting(ch, victim);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  }
  return eSUCCESS;
}

int do_disarm(Character *ch, char *argument, int cmd)
{
  Character *victim;
  class Object *wielded;

  char name[256];
  class Object *obj;
  int retval = 0;

  int is_fighting_mob(Character * ch);

  if (!ch->canPerform(SKILL_DISARM))
  {
    ch->sendln("You dunno how.");
    return eFAILURE;
  }

  if (ch->equipment[WIELD] == nullptr)
  {
    ch->sendln("You must wield a weapon to disarm.");
    return eFAILURE;
  }

  one_argument(argument, name);
  victim = ch->get_char_room_vis(name);
  if (victim == nullptr)
    victim = ch->fighting;

  if (victim == nullptr)
  {
    ch->sendln("Disarm whom?");
    return eFAILURE;
  }

  if (victim->equipment[WIELD] == nullptr)
  {
    ch->sendln("Your opponent is not wielding a weapon!");
    return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Attempting to disarm a bladeshielded opponent would be suicide!");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_DISARM))
    return eSUCCESS;

  set_cantquit(ch, victim);

  if (victim == ch)
  {
    if (victim->getLevel() >= IMMORTAL)
    {
      ch->sendln("You can't seem to work it loose.");
      return eFAILURE;
    }
    if (ch->equipment[WIELD]->vnum == 27997)
    {
      send_to_room("$B$7Ghaerad, Sword of Legends says, 'Sneaky! Sneaky! But you can't catch me!'$R\n\r", ch->in_room);
      return eSUCCESS;
    }
    act("$n disarms $mself!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    ch->sendln("You disarm yourself!  Congratulations!  Try using 'remove' next-time.");
    obj = unequip_char(ch, WIELD);
    obj_to_char(obj, ch);
    if (ch->equipment[SECOND_WIELD])
    {
      obj = unequip_char(ch, SECOND_WIELD);
      equip_char(ch, obj, WIELD);
    }

    return eSUCCESS;
  }

  wielded = victim->equipment[WIELD];

  int modifier = 0;
  level_diff_t level_difference = victim->getLevel() - ch->getLevel();

  if (ch->getLevel() < 50 && ch->getLevel() + 10 < victim->getLevel()) // keep lowbies from disarming big mobs
    modifier = -(level_difference * 2);

  // Two handed weapons are twice as hard to disarm
  if (isSet(wielded->obj_flags.extra_flags, ITEM_TWO_HANDED))
    modifier -= 20;

  if (skill_success(ch, victim, SKILL_DISARM, modifier))
  {

    if (((isSet(wielded->obj_flags.extra_flags, ITEM_NODROP) || isSet(wielded->obj_flags.more_flags, ITEM_NO_DISARM)) ||
         (victim->getLevel() >= IMMORTAL)) &&
        (IS_PC(victim) || DC::getInstance()->mob_index[victim->mobdata->vnum].vnum > 2400 ||
         DC::getInstance()->mob_index[victim->mobdata->vnum].vnum < 2300))
      ch->sendln("You can't seem to work it loose.");
    else
      disarm(ch, victim);
    WAIT_STATE(ch, 2 * DC::PULSE_VIOLENCE);
    if (IS_NPC(victim) && !victim->fighting)
    {
      retval = one_hit(victim, ch, TYPE_UNDEFINED, 0);
      retval = SWAP_CH_VICT(retval);
    }
  }
  else
  {
    act("$B$n attempts to disarm you!$R", ch, nullptr, victim, TO_VICT, 0);
    act("You try to disarm $N and fail!", ch, nullptr, victim, TO_CHAR, 0);
    act("$n attempts to disarm $N, but fails!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
    if (IS_NPC(victim) && !victim->fighting)
    {
      retval = one_hit(victim, ch, TYPE_UNDEFINED, 0);
      retval = SWAP_CH_VICT(retval);
    }
  }
  return retval;
}

/************************************************************************
| NON-OFFENSIVE commands.  Below here are commands that should -not-
|   require the victim to retaliate.
*/
int Character::do_rescue(QStringList arguments, int cmd)
{
  Character *victim{}, *tmp_ch{};
  QString victim_name = arguments.value(0);

  if (!canPerform(SKILL_RESCUE))
  {
    sendln("You've got alot to learn before you try to be a bodyguard.");
    return eFAILURE;
  }

  if (!(victim = get_char_room_vis(victim_name)))
  {
    sendln("Whom do you want to rescue?");
    return eFAILURE;
  }

  if (victim == this)
  {
    sendln("What about fleeing instead?");
    return eFAILURE;
  }

  if (IS_PC(this) && (IS_NPC(victim) && !IS_AFFECTED(victim, AFF_CHARM)))
  {
    sendln("Doesn't need your help!");
    return eFAILURE;
  }

  if (fighting == victim)
  {
    sendln("How can you rescue someone you are trying to kill?");
    return eFAILURE;
  }

  if (!can_be_attacked(this, victim->fighting))
  {
    sendln("You cannot complete the rescue!");
    return eFAILURE;
  }

  for (tmp_ch = DC::getInstance()->world[in_room].people; tmp_ch &&
                                                          (tmp_ch->fighting != victim);
       tmp_ch = tmp_ch->next_in_room)
    ;

  if (!tmp_ch)
  {
    act("But nobody is fighting $M?", this, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if (!charge_moves(SKILL_RESCUE))
    return eSUCCESS;

  if (!skill_success(victim, SKILL_RESCUE))
  {
    sendln("You fail the rescue.");
    return eFAILURE;
  }

  sendln("Banzai! To the rescue...");
  act("You are rescued by $N, you are confused!", victim, 0, this, TO_CHAR, 0);
  act("$n heroically rescues $N.", this, 0, victim, TO_ROOM, NOTVICT);

  int tempwait = GET_WAIT(this);
  int tempvictwait = GET_WAIT(victim);

  if (victim->fighting == tmp_ch)
    stop_fighting(victim);

  if (tmp_ch->fighting)
    stop_fighting(tmp_ch, 0);
  //    if (ch->fighting)
  //     stop_fighting(ch);

  /*
   * so rescuing an NPC who is fighting a PC does not result in
   * the other guy getting killer flag
   */
  if (!fighting)
    set_fighting(this, tmp_ch);
  set_fighting(tmp_ch, this);

  WAIT_STATE(this, MAX(DC::PULSE_VIOLENCE * 2, tempwait));
  WAIT_STATE(victim, MAX(DC::PULSE_VIOLENCE * 2, tempvictwait));
  return eSUCCESS;
}

int do_bladeshield(Character *ch, char *argument, int cmd)
{
  struct affected_type af;
  int duration = 12;

  if (!ch->canPerform(SKILL_BLADESHIELD))
  {
    ch->sendln("You'd cut yourself to ribbons just trying!");
    return eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_BLADESHIELD) && !ch->isImmortalPlayer())
  {
    ch->sendln("Your body is still recovering from your last use of the blade shield technique.");
    return eFAILURE;
  }

  if (!(ch->fighting))
  {
    ch->sendln("But you aren't fighting anyone!");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BLADESHIELD))
    return eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_BLADESHIELD))
  {
    act("$n starts swinging $s weapons around but stops before narrowly avoiding dismembering $mself.", ch, 0, 0, TO_ROOM, NOTVICT);
    ch->sendln("You try to begin the bladeshield technique and almost chop off your own arm!");
    duration /= 2;
  }
  else
  {
    act("$n forms a defensive wall of swinging weapons around $mself.", ch, 0, 0, TO_ROOM, NOTVICT);
    send_to_char("The world around you slows to a crawl, the weapons around you swing as if through water.  "
                 "Your mind clears of all but thrust angles as your body and mind enter completely into the "
                 "blade shield technique.\r\n",
                 ch);
    SET_BIT(ch->combat, COMBAT_BLADESHIELD1);
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  af.type = SKILL_BLADESHIELD;
  af.duration = duration;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  return eSUCCESS;
}

/* BEGIN UTILITY FUNCTIONS FOR "Guard" */

// return true on guard doing anything
// otherwise false
int handle_any_guard(Character *ch)
{
  if (!ch->guarded_by)
    return false;

  Character *guard = nullptr;

  // search the room for my guard
  for (follow_type *curr = ch->guarded_by; curr;)
  {
    for (Character *vict = DC::getInstance()->world[ch->in_room].people; vict; vict = vict->next_in_room)
      if (vict == curr->follower)
      {
        curr = nullptr;
        guard = vict;
        break;
      }
    if (curr)
      curr = curr->next;
  }

  if (!guard) // my guard isn't here
    return false;

  if (ch->fighting && can_be_attacked(guard, ch->fighting) && skill_success(guard, ch, SKILL_GUARD))
  {
    guard->do_rescue(ch->getName().split(' '));
    if (ch->fighting)
      return true;
    else
      return false;
  }
  return false;
}

Character *is_guarding_me(Character *ch, Character *guard)
{
  follow_type *curr = ch->guarded_by;

  while (curr)
  {
    if (curr->follower == guard)
      return guard;
    curr = curr->next;
  }

  return nullptr;
}

void stop_guarding(Character *guard)
{
  if (!guard->guarding) // i'm not guarding anyone:)  get out
    return;

  Character *victim = guard->guarding;
  follow_type *curr = victim->guarded_by;
  follow_type *last = nullptr;

  while (curr)
  {
    if (curr->follower == guard)
      break;
    last = curr;
    curr = curr->next;
  }
  if (curr)
  {
    // found the guard, remove him from list
    if (last)
      last->next = curr->next;
    else
      victim->guarded_by = curr->next;
  }
  // if we didn't find guard, return, since we wanted to remove um anyway:)
  guard->guarding = nullptr;
}

void start_guarding(Character *guard, Character *victim)
{
  follow_type *curr = (struct follow_type *)dc_alloc(1, sizeof(struct follow_type));

  curr->follower = guard;
  curr->next = victim->guarded_by;
  victim->guarded_by = curr;

  guard->guarding = victim;
}

void stop_guarding_me(Character *victim)
{
  char buf[200];
  follow_type *curr = victim->guarded_by;
  follow_type *last;

  while (curr)
  {
    sprintf(buf, "You stop trying to guard %s.\r\n", GET_SHORT(victim));
    curr->follower->send(buf);
    curr->follower->guarding = nullptr;
    last = curr;
    curr = curr->next;
    dc_free(last);
  }

  victim->guarded_by = nullptr;
}

/* END UTILITY FUNCTIONS FOR "Guard" */

int do_guard(Character *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH];
  Character *victim = nullptr;

  if (!IS_NPC(ch) && (!ch->has_skill(SKILL_GUARD) || !ch->has_skill(SKILL_RESCUE)))
  {
    ch->sendln("You have no idea how to be a full time bodyguard.");
    return eFAILURE;
  }
  if (IS_NPC(ch))
    return eFAILURE;

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    ch->sendln("Some bodyguard you are.  That person isn't even here!");
    return eFAILURE;
  }

  if (ch == victim)
  {
    ch->sendln("You stop guarding anyone.");
    stop_guarding(ch);
    return eSUCCESS;
  }

  if (victim == ch->guarding)
  {
    ch->sendln("You are already guarding that person.");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_GUARD))
    return eSUCCESS;

  if (ch->guarding)
  {
    stop_guarding(ch);
    ch->sendln("You stop guarding anyone.");
    //      return eFAILURE;
  }

  start_guarding(ch, victim);
  sprintf(name, "You begin trying to guard %s.\r\n", GET_SHORT(victim));
  ch->send(name);
  return eSUCCESS;
}

int do_tactics(Character *ch, char *argument, int cmd)
{
  struct affected_type af;

  if (!ch->canPerform(SKILL_TACTICS))
  {
    ch->sendln("You just don't have the mind for strategic battle.");
    return eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_TACTICS_TIMER))
  {
    ch->sendln("You will need more time to work out your tactics.");
    return eFAILURE;
  }
  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You have no group to command.");
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

  if (!charge_moves(ch, SKILL_TACTICS, grpsize))
    return eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_TACTICS))
  {
    ch->sendln("Guess you just weren't the Patton you thought you were.");
    act("$n goes on about team not being spelled with an 'I' or something.", ch, 0, 0, TO_ROOM, 0);
  }
  else
  {
    act("$n takes command coordinating $s group's efforts.", ch, 0, 0, TO_ROOM, 0);
    ch->sendln("You take command coordinating the group's attacks.");

    af.type = SKILL_TACTICS_TIMER;
    af.duration = 1 + ch->has_skill(SKILL_TACTICS) / 10;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = -1;
    affect_to_char(ch, &af);

    for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
    {
      if (tmp_char == ch)
        continue;
      if (!ARE_GROUPED(ch, tmp_char))
        continue;

      affect_from_char(tmp_char, SKILL_TACTICS, SUPPRESS_MESSAGES);
      affect_from_char(tmp_char, SKILL_TACTICS, SUPPRESS_MESSAGES);

      act("$n's leadership makes you feel more comfortable with battle.", ch, 0, tmp_char, TO_VICT, 0);
      af.type = SKILL_TACTICS;
      af.duration = 1 + ch->has_skill(SKILL_TACTICS) / 10;
      af.modifier = 1 + ch->has_skill(SKILL_TACTICS) / 20;
      af.location = APPLY_HITROLL;
      af.bitvector = -1;
      affect_to_char(tmp_char, &af);
      af.location = APPLY_DAMROLL;
      affect_to_char(tmp_char, &af);
    }
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  return eSUCCESS;
}

int do_make_camp(Character *ch, char *argument, int cmd)
{
  Character *i, *next_i;
  int learned = ch->has_skill(SKILL_MAKE_CAMP);
  struct affected_type af;

  if (!IS_NPC(ch) && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("You do not know how to set up a safe camp.");
    return eFAILURE;
  }

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) || isSet(DC::getInstance()->world[ch->in_room].room_flags, UNSTABLE) ||
      isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_NORTH) || isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_SOUTH) ||
      isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_EAST) || isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_WEST) ||
      isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_UP) || isSet(DC::getInstance()->world[ch->in_room].room_flags, FALL_DOWN) ||
      DC::getInstance()->world[ch->in_room].sector_type == SECT_CITY || DC::getInstance()->world[ch->in_room].sector_type == SECT_PAVED_ROAD)
  {
    ch->sendln("Something about this area inherently prohibits a rugged camp.");
    return eFAILURE;
  }

  for (i = DC::getInstance()->world[ch->in_room].people; i; i = next_i)
  {
    next_i = i->next_in_room;

    if ((IS_NPC(i) && !IS_AFFECTED(i, AFF_CHARM) && !IS_AFFECTED(i, AFF_FAMILIAR)) || (i->fighting))
    {
      ch->sendln("This area does not yet feel secure enough to rest.");
      return eFAILURE;
    }
  }

  if (ch->affected_by_spell(SKILL_MAKE_CAMP_TIMER))
  {
    ch->sendln("You cannot make another camp so soon.");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_MAKE_CAMP))
  {
    ch->sendln("You are unable to muster the strength to make a camp.");
    return eFAILURE;
  }

  for (i = DC::getInstance()->world[ch->in_room].people; i; i = next_i)
  {
    next_i = i->next_in_room;

    if (i->affected_by_spell(SKILL_MAKE_CAMP))
    {
      ch->sendln("There is already a camp setup here.");
      return eFAILURE;
    }
  }

  WAIT_STATE(ch, (int)(DC::PULSE_VIOLENCE * 2.5));

  ch->sendln("You scan about for signs of danger as you clear an area to make camp...");
  act("$n scans about for signs of danger and clears an area to make camp...", ch, 0, 0, TO_ROOM, 0);

  if (!skill_success(ch, 0, SKILL_MAKE_CAMP))
  {
    send_to_room("The area does not yet feel secure enough to rest.\r\n", ch->in_room);
  }
  else
  {
    send_to_room("The area feels secure enough to get some rest.\r\n", ch->in_room);

    af.type = SKILL_MAKE_CAMP_TIMER;
    af.duration = 2 + learned / 9;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = -1;

    affect_to_char(ch, &af);

    af.type = SKILL_MAKE_CAMP;
    af.duration = 1 + learned / 9;
    af.modifier = ch->in_room;
    af.location = 0;
    af.bitvector = -1;

    affect_to_char(ch, &af);

    for (i = DC::getInstance()->world[ch->in_room].people; i; i = next_i)
    {
      next_i = i->next_in_room;

      if (!i->affected_by_spell(SPELL_FARSIGHT) && !IS_AFFECTED(i, AFF_FARSIGHT))
      {
        af.type = SPELL_FARSIGHT;
        af.duration = -1;
        af.modifier = 111;
        af.location = 0;
        af.bitvector = AFF_FARSIGHT;

        affect_to_char(i, &af);
      }
    }
  }

  return eSUCCESS;
}

int do_triage(Character *ch, char *argument, int cmd)
{
  int learned = ch->has_skill(SKILL_TRIAGE);
  struct affected_type af;

  if (ch->isMortalPlayer() && !learned)
  {
    ch->sendln("You do not know how to aid your regeneration in that way.");
    return eFAILURE;
  }

  if (ch->fighting)
  {
    ch->sendln("You are a little too busy for that right now.");
    return eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_TRIAGE_TIMER))
  {
    ch->sendln("You cannot take care of your battle wounds again so soon.");
    return eFAILURE;
  }

  if (!ch->decrementMove(skill_cost.find(SKILL_TRIAGE)->second, "You're too tired to use this skill."))
  {
    return eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  af.type = SKILL_TRIAGE_TIMER;
  af.modifier = 0;
  af.duration = 6;
  af.location = 0;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  if (!skill_success(ch, 0, SKILL_TRIAGE))
  {
    ch->sendln("You pause to clean and bandage some of your more painful injuries but feel little improvement in your health.");
    act("$n pauses to try and bandage some of $s more painful injuries.", ch, 0, 0, TO_ROOM, 0);
    return eSUCCESS;
  }

  af.type = SKILL_TRIAGE;
  af.modifier = learned / 3;
  af.duration = 2;
  af.location = APPLY_HP_REGEN;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  ch->sendln("You pause to clean and bandage some of your more painful injuries and speed the healing process.");
  act("$n pauses to try and bandage some of $s more painful injuries.", ch, 0, 0, TO_ROOM, 0);

  return eSUCCESS;
}

int do_battlesense(Character *ch, char *argument, int cmd)
{
  int learned = ch->has_skill(SKILL_BATTLESENSE);
  struct affected_type af;

  if (!IS_NPC(ch) && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("You do not know how to heighten your battle awareness.");
    return eFAILURE;
  }

  if (!ch->fighting)
  {
    ch->sendln("You must be in battle to heighten your combat awareness!");
    return eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  if (!skill_success(ch, 0, SKILL_BATTLESENSE))
  {
    ch->sendln("Your body and mind, tired from continued battle, are unable to reach full combat readiness.");
  }
  else
  {
    ch->sendln("Your awareness heightens dramatically as the rush of battle courses through your body.");
    act("$n's movements become quick and calculated as $s senses heighten with the rush of battle.", ch, 0, 0, TO_ROOM, 0);

    af.type = SKILL_BATTLESENSE;
    af.location = 0;
    af.modifier = 10 + learned / 4;
    af.duration = 1 + learned / 11;
    af.bitvector = -1;

    affect_to_char(ch, &af, DC::PULSE_VIOLENCE);
  }

  return eSUCCESS;
}

int do_smite(Character *ch, char *argument, int cmd)
{
  Character *vict = nullptr;
  char name[MAX_STRING_LENGTH];
  int learned = ch->has_skill(SKILL_SMITE);
  struct affected_type af;

  if (!IS_NPC(ch) && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("You do not know how to smite your enemies effectively.");
    return eFAILURE;
  }

  if (!*argument)
  {
    if (!ch->fighting)
    {
      ch->sendln("Smite whom?");
      return eFAILURE;
    }
    else
      vict = ch->fighting;
  }

  one_argument(argument, name);

  if (*argument)
    if (!(vict = ch->get_char_room_vis(name)))
    {
      ch->sendln("Smite whom?");
      return eFAILURE;
    }

  if (ch == vict)
  {
    ch->sendln("There's a suicide command for that...");
    return eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, vict))
  {
    act("You cannot attack $M", ch, 0, vict, TO_CHAR, 0);
    return eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_SMITE_TIMER))
  {
    ch->sendln("You cannot smite your enemies again so soon.");
    return eFAILURE;
  }

  af.type = SKILL_SMITE_TIMER;
  af.location = 0;
  af.modifier = 0;
  af.duration = 22 - learned / 10;
  af.bitvector = -1;

  affect_to_char(ch, &af);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 4);

  if (!skill_success(ch, vict, SKILL_SMITE))
  {
    act("Your less-than-mighty challenge fails to improve your attack of $N.", ch, 0, vict, TO_CHAR, 0);
    act("$n makes a pathetic attempt at shouting a challenge as $e attempts to strike you.", ch, 0, vict, TO_VICT, 0);
    act("$n makes a pathetic attempt at shouting a challenge as $e attempts to strike $N.", ch, 0, vict, TO_ROOM, NOTVICT);
  }
  else
  {
    act("You shout a mighty challenge and begin to assault $N with lethal efficiency.", ch, 0, vict, TO_CHAR, 0);
    act("$n shouts a mighty challenge and begins to assault you with lethal efficiency.", ch, 0, vict, TO_VICT, 0);
    act("$n shouts a mighty challenge and begins to assault $N with lethal efficiency.", ch, 0, vict, TO_ROOM, NOTVICT);

    af.type = SKILL_SMITE;
    af.location = 0;
    af.modifier = 0;
    af.victim = vict;
    af.duration = 1 + learned / 16;
    af.bitvector = -1;

    affect_to_char(ch, &af, DC::PULSE_VIOLENCE);
  }

  return ch->fighting ? eSUCCESS : attack(ch, vict, TYPE_UNDEFINED);
}

int do_leadership(Character *ch, char *argument, int cmd)
{
  int learned = ch->has_skill(SKILL_LEADERSHIP);
  struct affected_type af;

  if (!IS_NPC(ch) && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("You do not know that ability.");
    return eFAILURE;
  }

  if (ch->master || !ch->followers)
  {
    ch->sendln("You must be leading a group to call upon your leadership skills.");
    return eFAILURE;
  }

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) || isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("Stop trying to show off.");
    return eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_LEADERSHIP))
  {
    ch->sendln("You and your followers are already inspired.");
    return eFAILURE;
  }

  ch->sendln("You loudly call, 'Once more unto the breach, dear friends!'");
  act("$n loudly calls, 'Once more unto the breach, dear friends!'", ch, 0, 0, TO_ROOM, 0);

  WAIT_STATE(ch, (int)(DC::PULSE_VIOLENCE * 1.5));

  if (!skill_success(ch, 0, SKILL_LEADERSHIP))
  {
    ch->sendln("Your bravery miserably fails to inspire.");
    act("$n's bravery miserably fails to inspire.", ch, 0, 0, TO_ROOM, 0);
  }
  else
  {
    ch->sendln("Your bravery lends you additional might and inspires the group!");
    act("$n's bravery lends $m additional might and inspires the group!", ch, 0, 0, TO_ROOM, 0);

    af.type = SKILL_LEADERSHIP;
    af.duration = 1 + learned / 20;
    af.modifier = learned / 19; // cap on bonus
    af.location = 0;
    af.bitvector = -1;

    affect_to_char(ch, &af);

    ch->changeLeadBonus = true;
    ch->curLeadBonus = get_leadership_bonus(ch);
  }

  return eSUCCESS;
}

int do_perseverance(Character *ch, char *argument, int cmd)
{
  int learned = ch->has_skill(SKILL_PERSEVERANCE);
  struct affected_type af;

  if (!IS_NPC(ch) && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("Your lack of fortitude is stunning.");
    return eFAILURE;
  }

  if (!ch->fighting)
  {
    ch->sendln("What, exactly, are you trying to persevere through?");
    return eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  if (!skill_success(ch, 0, SKILL_PERSEVERANCE))
  {
    ch->sendln("Your movements seem unchanged, though you strive to gain some momentum in your actions.");
  }
  else
  {
    ch->sendln("Your movements seem to build energy and gain momentum as you fight with renewed vigor!");
    act("$n seems to build energy and $s movements gain momentum as the battle drags on...", ch, 0, 0, TO_ROOM, 0);

    af.type = SKILL_PERSEVERANCE;
    af.location = 0;
    af.duration = 1 + learned / 11;
    af.modifier = learned;
    af.bitvector = -1;

    affect_to_char(ch, &af, DC::PULSE_VIOLENCE);
  }

  return eSUCCESS;
}

int do_defenders_stance(Character *ch, char *argument, int cmd)
{
  Character *vict = nullptr;
  int learned = ch->has_skill(SKILL_DEFENDERS_STANCE);
  struct affected_type af;

  if (!IS_NPC(ch) && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("You do not know how to use this to your advantage.");
    return eFAILURE;
  }

  if (!ch->fighting)
  {
    ch->sendln("From whom are you trying to defend yourself?");
    return eFAILURE;
  }

  vict = ch->fighting;
  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);

  if (!skill_success(ch, 0, SKILL_DEFENDERS_STANCE))
  {
    act("You attempt to brace yourself to defend against $N's onslaught but stumble and fall!", ch, 0, vict, TO_CHAR, 0);
    act("$n attempts to brace $mself to defend against your onslaught but stumbles and falls!", ch, 0, vict, TO_VICT, 0);
    act("$n attempts to brace $mself to defend against $N's onslaught but stumbles and falls!", ch, 0, vict, TO_ROOM, NOTVICT);
    ch->setSitting();
  }
  else
  {
    act("You brace yourself to defend against $N's onslaught.", ch, 0, vict, TO_CHAR, 0);
    act("$n braces $mself to defend against your onslaught.", ch, 0, vict, TO_VICT, 0);
    act("$n braces $mself to defend against $N's onslaught.", ch, 0, vict, TO_ROOM, NOTVICT);

    af.type = SKILL_DEFENDERS_STANCE;
    af.location = 0;
    af.modifier = 5 + learned / 2;
    af.duration = 1 + learned / 12;
    af.bitvector = -1;

    affect_to_char(ch, &af, DC::PULSE_VIOLENCE);
  }
  return eSUCCESS;
}

int do_onslaught(Character *ch, char *argument, int cmd)
{
  int learned = ch->has_skill(SKILL_ONSLAUGHT);
  struct affected_type af;

  if (ch->isMortalPlayer() && !learned)
  {
    ch->sendln("You do not know how to use this to your advantage.");
    return eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_ONSLAUGHT_TIMER))
  {
    ch->sendln("You have not yet recovered from your previous onslaught attempt.");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_ONSLAUGHT))
  {
    ch->sendln("You do not have enough energy to attempt this onslaught.");
    return eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  if (!skill_success(ch, 0, SKILL_ONSLAUGHT))
  {
    act("Obviously you have a bit more to learn about battle...slowass.", ch, 0, 0, TO_CHAR, 0);
    act("$n waves $s weapon in the air in a futile attempt to look skillful.", ch, 0, 0, TO_ROOM, 0);

    af.type = SKILL_ONSLAUGHT_TIMER;
    af.location = 0;
    af.modifier = 0;
    af.duration = 7;
    af.bitvector = -1;
    affect_to_char(ch, &af);
  }
  else
  {
    act("Your attacks come fast and furious as you harness your battle expertise.", ch, 0, 0, TO_CHAR, 0);
    act("$n's attacks come fast and furious as $e harnesses $s battle expertise.", ch, 0, 0, TO_ROOM, 0);

    af.type = SKILL_ONSLAUGHT;
    af.location = 0;
    af.modifier = 0;
    af.duration = 1 + learned / 15;
    af.bitvector = -1;

    affect_to_char(ch, &af);

    af.type = SKILL_ONSLAUGHT_TIMER;
    af.location = 0;
    af.modifier = 0;
    af.duration = 7 + learned / 15;
    af.bitvector = -1;

    affect_to_char(ch, &af);
  }
  return eSUCCESS;
}
