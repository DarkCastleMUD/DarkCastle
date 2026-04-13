/************************************************************************
| $Id: cl_warrior.cpp,v 1.86 2014/08/21 01:59:39 jhhudso Exp $
| cl_warrior.C
| Description:  This file declares implementation for warrior-specific
|   skills.
*/

#include <algorithm>
#include <qtypes.h>

#include "DC/obj.h"
#include "DC/structs.h"
#include "DC/DC.h"
#include "DC/player.h"
#include "DC/fight.h"

#include "DC/spells.h"
#include "DC/handler.h"
#include "DC/act.h"
#include "DC/interp.h"
#include "DC/spells.h"
#include "DC/const.h"
#include "DC/move.h"
#include "DC/utility.h"

/************************************************************************
| OFFENSIVE commands.  These are commands that should require the
|   victim to retaliate.
*/
command_return_t Character::do_kick(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim{}, next_victim = {};
  QString name;
  qint32 dam = {};
  command_return_t retval = {};

  if (!canPerform(SKILL_KICK))
  {
    sendln("You will have to study from a master before you can use this.");
    return ReturnValue::eFAILURE;
  }

  name = arguments.value(0);

  if (!(victim = get_char_room_vis(name)))
  {
    if (fighting)
    {
      victim = fighting;
    }
    else
    {
      sendln("Your foot comes up, but there's nobody there...");
      return ReturnValue::eFAILURE;
    }
  }

  if (victim == this)
  {
    sendln("You kick yourself, metaphorically speaking.");
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(this) || !can_be_attacked(this, victim))
    return ReturnValue::eFAILURE;

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    sendln("Kicking a bladeshielded opponent would be a good way to lose a leg!");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(SKILL_KICK))
    return ReturnValue::eSUCCESS;

  WAIT_STATE(this, (qint32)(DC::PULSE_VIOLENCE * 1.5));

  if (!skill_success(victim, SKILL_KICK))
  {
    dam = {};
    retval = damage(this, victim, 0, TYPE_BLUDGEON, SKILL_KICK);
    if (SOMEONE_DIED(retval))
      return retval;
  }
  else
  {
    if (victim->affected_by_spell(SKILL_BATTLESENSE) && dc_->number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act_to_character("$N's heightened battlesense sees your kick coming from a mile away.", this, 0, victim, 0);
      act_to_victim("Your heightened battlesense sees $n's kick coming from a mile away.", this, 0, victim, 0);
      act_to_room("$N's heightened battlesense sees $n's kick coming from a mile away.", this, 0, victim, NOTVICT);
      dam = {};
    }
    else
      dam = (GET_DEX(this) * 3) + (GET_STR(this) * 2) + (has_skill(SKILL_KICK));
    retval = damage(this, victim, dam, TYPE_BLUDGEON, SKILL_KICK);
    if (SOMEONE_DIED(retval))
      return retval;
  }

  // if our boots have a combat proc, and we did damage, let'um have it!
  if (dam && equipment[WEAR_FEET])
  {
    retval = weapon_spells(this, victim, WEAR_FEET);
    if (SOMEONE_DIED(retval))
      return retval;
    // leaving this built in proc here incase some new stuff is added, like kick_their_head_off
    if (dc_->obj_index[equipment[WEAR_FEET]->item_number].combat_func)
    {
      retval = ((*dc_->obj_index[equipment[WEAR_FEET]->item_number].combat_func)(this, equipment[WEAR_FEET], cmd_t::UNDEFINED, "", this));
    }
    if (SOMEONE_DIED(retval))
      return retval;
  }

  // Extra kick targeting main opponent for monks
  if ((GET_CLASS(this) == CLASS_MONK) && fighting && in_room == fighting->in_room)
  {
    next_victim = fighting;
    if (!skill_success(next_victim, SKILL_KICK))
    {
      dam = {};
      retval = damage(this, next_victim, 0, TYPE_UNDEFINED, SKILL_KICK);
    }
    else
    {
      dam = (GET_DEX(this) * 2) + (GET_STR(this)) + (has_skill(SKILL_KICK) / 2);
      retval = damage(this, next_victim, dam, TYPE_UNDEFINED, SKILL_KICK);
    }
    if (SOMEONE_DIED(retval))
      return retval;
    // if our boots have a combat proc, and we did damage, let'um have it!
    if (dam && equipment[WEAR_FEET])
    {
      retval = weapon_spells(this, next_victim, WEAR_FEET);
      if (SOMEONE_DIED(retval))
        return retval;
      // leaving this built in proc here incase some new stuff is added, like kick_their_head_off
      if (dc_->obj_index[equipment[WEAR_FEET]->item_number].combat_func)
      {
        retval = ((*dc_->obj_index[equipment[WEAR_FEET]->item_number].combat_func)(this, equipment[WEAR_FEET], cmd_t::UNDEFINED, "", this));
      }
    }
  }

  return retval;
}

command_return_t do_deathstroke(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name;
  qint32 dam, attacktype;
  qint32 retval;
  qint32 failchance = 25;

  if (!ch->canPerform(SKILL_DEATHSTROKE))
  {
    ch->sendln("You have no idea how to deathstroke.");
    return ReturnValue::eFAILURE;
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
      return ReturnValue::eFAILURE;
    }
  }

  if (victim == ch)
  {
    ch->sendln("The world would be a better place...");
    return ReturnValue::eFAILURE;
  }

  if (!ch->equipment[WEAR_WIELD])
  {
    ch->sendln("You must be wielding a weapon to deathstrike someone.");
    return ReturnValue::eFAILURE;
  }

  if (GET_POS(victim) > position_t::SITTING)
  {
    ch->sendln("Your opponent isn't in a vulnerable enough position!");
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return ReturnValue::eFAILURE;

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Stroking a bladeshielded opponent would be suicide!");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_DEATHSTROKE))
    return ReturnValue::eSUCCESS;

  qint32 i = ch->has_skill(SKILL_DEATHSTROKE);
  if (i > 40)
    failchance -= 5;
  if (i > 60)
    failchance -= 5;
  if (i > 80)
    failchance -= 5;
  if (i > 90)
    failchance -= 5;

  qint32 to_dam = GET_DAMROLL(ch);
  if (victim->isNonPlayer())
    to_dam = (qint32)((qreal)to_dam * .8);

  dam = dice(ch->equipment[WEAR_WIELD]->obj_flags.value[1],
             ch->equipment[WEAR_WIELD]->obj_flags.value[2]);
  dam += to_dam;
  dam *= (ch->getLevel() / 6); // 10 at level 50

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
  attacktype = ch->equipment[WEAR_WIELD]->obj_flags.value[3] + TYPE_HIT;

  if (!skill_success(ch, victim, SKILL_DEATHSTROKE, -25))
  {
    retval = damage(ch, victim, 0, attacktype, SKILL_DEATHSTROKE);
    if (ch->dc_->number(1, 100) > failchance)
    {
      ch->sendln("You manage to retain your balance!");
      return ReturnValue::eFAILURE;
    }
    dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
    ch->removeHP(dam);
    update_pos(ch);
    if (GET_POS(ch) == position_t::DEAD)
    {
      fight_kill(ch, ch, TYPE_CHOOSE, 0);
      return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
    }
  }
  else
  {
    if (victim->affected_by_spell(SKILL_BATTLESENSE) &&
        dc_->number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act_to_character("$N's heightened battlesense somehow notices your deathstroke coming from a mile away.", ch, 0, victim, 0);
      act_to_victim("Your heightened battlesense somehow notices $n's deathstroke coming from a mile away.", ch, 0, victim, 0);
      act_to_room("$N's heightened battlesense somehow notices $n's deathstroke coming from a mile away.", ch, 0, victim, NOTVICT);
      dam = {};
    }
    retval = damage(ch, victim, dam, attacktype, SKILL_DEATHSTROKE);
  }

  return retval;
}

command_return_t do_retreat(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 attempt;
  QString buf;
  // Azrack -- retval should be initialized to something
  qint32 retval = {};
  CharacterPtr chTemp, loop_ch;

  bool is_stunned(CharacterPtr ch);

  if (is_stunned(ch))
    return ReturnValue::eFAILURE;

  if (!ch->canPerform(SKILL_RETREAT))
  {
    ch->sendln("You dunno how...better flee instead.");
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_NO_FLEE))
  {
    if (ch->affected_by_spell(SPELL_IRON_ROOTS))
      ch->sendln("The roots bracing your legs make it impossible to run!");
    else
      ch->sendln("Your legs are too tired for running away!");
    return ReturnValue::eFAILURE;
  }

  // after the below line, buf should contain the argument, hopefully
  // a direction to retreat.
  one_argument(argument, buf);

  if ((attempt = consttype(buf, dirs)) == -1)
  {
    send_to_char("You are supposed to retreat towards a specific "
                 "direction...\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  if (!CAN_GO(ch, attempt))
  {
    ch->sendln("You cannot retreat in that direction.");
    return ReturnValue::eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  if (!charge_moves(ch, SKILL_RETREAT))
    return ReturnValue::eSUCCESS;

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
    return ReturnValue::eFAILURE;
  }

  //   if (CAN_GO(ch, attempt))
  {
    act("$n tries to beat a hasty retreat.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    ch->sendln("You try to beat a hasty retreat....");

    // check for any spec procs
    retval = ch->special("", static_cast<cmd_t>(attempt + 1));
    if (isSet(retval, ReturnValue::eSUCCESS) || isSet(retval, ReturnValue::eCH_DIED))
      return retval;

    auto dir_cmd = static_cast<cmd_t>(attempt + 1);
    retval = attempt_move(ch, dir_cmd, 1);
    if (isSet(retval, ReturnValue::eSUCCESS))
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
      if (!isSet(retval, ReturnValue::eCH_DIED))
        act_to_room("$n tries to retreat, but is too exhausted!", ch, 0, 0, INVIS_NULL);
      return retval;
    }
  }

  // No exits were found
  ch->sendln("You cannot retreat in that direction.!");
  return ReturnValue::eFAILURE;
}

command_return_t do_hitall(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isPlayer() && ch->getLevel() < ARCHANGEL && !ch->has_skill(SKILL_HITALL))
  {
    ch->sendln("You better learn how to first...");
    return ReturnValue::eFAILURE;
  }

  if (ch->getHP() == 1)
  {
    ch->sendln("You are too weak to do this right now.");
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch))
    return ReturnValue::eFAILURE;

  if (!charge_moves(ch, SKILL_HITALL))
    return ReturnValue::eSUCCESS;

  // TODO - I'm pretty sure we can remove this check....don't feel like checking right now though
  if (isSet(ch->combat, COMBAT_HITALL))
  {
    ch->sendln("You can't disengage!");
    return ReturnValue::eFAILURE;
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
    act_to_character("You start swinging like a MADMAN!", ch, 0, 0, 0);
    act_to_room("$n starts swinging like a MADMAN!", ch, 0, 0, 0);
    SET_BIT(ch->combat, COMBAT_HITALL);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);

    const auto &character_list = dc_->character_list;
    std::for_each(character_list.begin(), character_list.end(), [&ch](CharacterPtr vict)
                  {
			if (vict && vict != (CharacterPtr ) 0x95959595 && ch->in_room == vict->in_room && !ARE_GROUPED(ch, vict) && vict != ch && can_be_attacked(ch, vict)) {
				qint32 retval = one_hit(ch, vict, TYPE_UNDEFINED, WEAR_WIELD);
				if (isSet(retval, ReturnValue::eCH_DIED)) {
					REMOVE_BIT(ch->combat, COMBAT_HITALL);
					return false;
				}
			}
			return true; });
    REMOVE_BIT(ch->combat, COMBAT_HITALL);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_bash(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name;
  qint32 retval;
  qint32 hit = {};

  one_argument(argument, name);

  if (IS_AFFECTED(ch, AFF_CHARM))
    return ReturnValue::eFAILURE;

  if (!ch->canPerform(SKILL_BASH, "You don't know how to bash!\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  //    if (ch->isPlayer())
  if (!ch->equipment[WEAR_WIELD])
  {
    ch->sendln("You need to wield a weapon, to make it a success.");
    return ReturnValue::eFAILURE;
  }

  if (!name.isEmpty())
    victim = ch->get_char_room_vis(name);
  else
    victim = ch->fighting;

  if (!victim)
  {
    ch->sendln("Bash whom?");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BASH))
    return ReturnValue::eSUCCESS;

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return ReturnValue::eFAILURE;

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_HUGE))
  {
    ch->sendln("You cannot bash something that HUGE!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_SWARM))
  {
    ch->sendln("You cannot pick just one to bash!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_TINY))
  {
    ch->sendln("You cannot target something that tiny!");
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Aren't we funny today...");
    return ReturnValue::eFAILURE;
  }

  set_cantquit(ch, victim);

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Bashing a bladeshielded opponent would be suicide!");
    return ReturnValue::eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_IRON_ROOTS))
  {
    act_to_character("You try to bash $N but tree roots around $S legs keep him upright.", ch, 0, victim, 0);
    act_to_victim("$n bashes you but the roots around your legs keep you from falling.", ch, 0, victim, 0);
    act_to_room("The tree roots support $N keeping $M from sprawling after $n's bash.", ch, 0, victim, NOTVICT);
    WAIT_STATE(ch, 2 * DC::PULSE_VIOLENCE);
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_STABILITY) && dc_->number(0, 3) == 0)
  {
    act_to_character("You bounce off of $N and crash into the ground.", ch, 0, victim, 0);
    act_to_victim("$n bounces off of you and crashes into the ground.", ch, 0, victim, 0);
    act_to_room("$n bounces off of $N and crashes into the ground.", ch, 0, victim, NOTVICT);
    WAIT_STATE(ch, 2 * DC::PULSE_VIOLENCE);
    return ReturnValue::eFAILURE;
  }

  qint32 modifier = {};
  // half as accurate without a shield
  if (!ch->equipment[WEAR_SHIELD])
  {
    modifier = -20;
    // but 3/4 as accurate with a 2hander
    if (ch->equipment[WEAR_WIELD] && isSet(ch->equipment[WEAR_WIELD]->obj_flags.extra_flags, ITEM_TWO_HANDED))
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

  qint32 stat_mod = ch->get_stat(attribute_t::STRENGTH) - victim->get_stat(attribute_t::STRENGTH);
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
    retval = damage(ch, victim, 0, TYPE_BLUDGEON, SKILL_BASH);
  }
  else
  {
    if (victim->affected_by_spell(SKILL_BATTLESENSE) &&
        dc_->number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act_to_character("$N's heightened battlesense sees your bash coming from a mile away.", ch, 0, victim, 0);
      act_to_victim("Your heightened battlesense sees $n's bash coming from a mile away.", ch, 0, victim, 0);
      act_to_room("$N's heightened battlesense sees $n's bash coming from a mile away.", ch, 0, victim, NOTVICT);
      retval = damage(ch, victim, 0, TYPE_BLUDGEON, SKILL_BASH);
    }
    else
    {
      victim->setSitting();
      SET_BIT(victim->combat, COMBAT_BASH1);
      retval = damage(ch, victim, 25, TYPE_BLUDGEON, SKILL_BASH);
    }
    if (!(retval & ReturnValue::eEXTRA_VALUE))
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
    if (ch->dc_->obj_index[ch->equipment[WEAR_SHIELD]->item_number].combat_func)
    {
      retval = ((*ch->dc_->obj_index[ch->equipment[WEAR_SHIELD]->item_number].combat_func)(ch, ch->equipment[WEAR_SHIELD], cmd_t::UNDEFINED, "", ch));
    }
  }

  return retval;
}

command_return_t do_redirect(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name;

  if (!ch->canPerform(SKILL_REDIRECT, "You aren't skilled enough to change opponents midfight!\r\n"))
    return ReturnValue::eFAILURE;

  one_argument(argument, name);

  victim = ch->get_char_room_vis(name);

  if (victim == nullptr)
  {
    ch->sendln("Redirect your attacks to whom?");
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Aren't we funny today...");
    return ReturnValue::eFAILURE;
  }

  if (!ch->fighting)
  {
    ch->sendln("You're not fighting anyone to begin with!");
    return ReturnValue::eFAILURE;
  }

  if (!victim->fighting)
  {
    ch->sendln("He isn't bothering anyone, you have enough problems as it is anyways!");
    return ReturnValue::eFAILURE;
  }
  if (ch->fighting == victim)
  {
    act_to_character("You are already fighting $N.", ch, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }
  if (!can_be_attacked(ch, victim))
    return ReturnValue::eFAILURE;

  if (!charge_moves(ch, SKILL_REDIRECT))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, victim, SKILL_REDIRECT))
  {
    act_to_victim("$n tries to redirect his attacks but $N won't allow it.", ch, nullptr, ch->fighting, 0);
    act_to_character("You try to redirect your attacks to $N but are blocked.", ch, nullptr, victim, 0);
    act_to_room("$n tries to redirect his attacks elsewhere, but $N wont allow it.", ch, nullptr, victim, NOTVICT);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  }
  else
  {
    act_to_victim("$n redirects his attacks at YOU!", ch, nullptr, victim, 0);
    act_to_character("You redirect your at attacks at $N!", ch, nullptr, victim, 0);
    act_to_room("$n redirects his attacks at $N!", ch, nullptr, victim, NOTVICT);
    stop_fighting(ch);
    set_fighting(ch, victim);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_disarm(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  ObjectPtr wielded;

  QString name;
  ObjectPtr obj;
  qint32 retval = {};

  bool is_fighting_mob(CharacterPtr ch);

  if (!ch->canPerform(SKILL_DISARM))
  {
    ch->sendln("You dunno how.");
    return ReturnValue::eFAILURE;
  }

  if (ch->equipment[WEAR_WIELD] == nullptr)
  {
    ch->sendln("You must wield a weapon to disarm.");
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);
  victim = ch->get_char_room_vis(name);
  if (victim == nullptr)
    victim = ch->fighting;

  if (victim == nullptr)
  {
    ch->sendln("Disarm whom?");
    return ReturnValue::eFAILURE;
  }

  if (victim->equipment[WEAR_WIELD] == nullptr)
  {
    ch->sendln("Your opponent is not wielding a weapon!");
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return ReturnValue::eFAILURE;

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Attempting to disarm a bladeshielded opponent would be suicide!");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_DISARM))
    return ReturnValue::eSUCCESS;

  set_cantquit(ch, victim);

  if (victim == ch)
  {
    if (victim->getLevel() >= IMMORTAL)
    {
      ch->sendln("You can't seem to work it loose.");
      return ReturnValue::eFAILURE;
    }
    if (dc_->obj_index[ch->equipment[WEAR_WIELD]->item_number].vnum() == 27997)
    {
      send_to_room("$B$7Ghaerad, Sword of Legends says, 'Sneaky! Sneaky! But you can't catch me!'$R\r\n", ch->in_room);
      return ReturnValue::eSUCCESS;
    }
    act_to_room("$n disarms $mself!", ch, nullptr, victim, NOTVICT);
    ch->sendln("You disarm yourself!  Congratulations!  Try using 'remove' next-time.");
    obj = ch->unequip_char(WEAR_WIELD);
    obj_to_char(obj, ch);
    if (ch->equipment[WEAR_SECOND_WIELD])
    {
      obj = ch->unequip_char(WEAR_SECOND_WIELD);
      ch->equip_char(obj, WEAR_WIELD);
    }

    return ReturnValue::eSUCCESS;
  }

  wielded = victim->equipment[WEAR_WIELD];

  qint32 modifier = {};
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
        (victim->isPlayer() || dc_->mob_index[victim->mobdata->nr].vnum() > 2400 ||
         dc_->mob_index[victim->mobdata->nr].vnum() < 2300))
      ch->sendln("You can't seem to work it loose.");
    else
      disarm(ch, victim);
    WAIT_STATE(ch, 2 * DC::PULSE_VIOLENCE);
    if (victim->isNonPlayer() && !victim->fighting)
    {
      retval = one_hit(victim, ch, TYPE_UNDEFINED, 0);
      retval = SWAP_CH_VICT(retval);
    }
  }
  else
  {
    act_to_victim("$B$n attempts to disarm you!$R", ch, nullptr, victim, 0);
    act_to_character("You try to disarm $N and fail!", ch, nullptr, victim, 0);
    act_to_room("$n attempts to disarm $N, but fails!", ch, nullptr, victim, NOTVICT);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
    if (victim->isNonPlayer() && !victim->fighting)
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
qint32 Character::do_rescue(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim{}, tmp_ch = {};
  QString victim_name = arguments.value(0);

  if (!canPerform(SKILL_RESCUE))
  {
    sendln("You've got alot to learn before you try to be a bodyguard.");
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_char_room_vis(victim_name)))
  {
    sendln("Whom do you want to rescue?");
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    sendln("What about fleeing instead?");
    return ReturnValue::eFAILURE;
  }

  if (this->isPlayer() && (victim->isNonPlayer() && !IS_AFFECTED(victim, AFF_CHARM)))
  {
    sendln("Doesn't need your help!");
    return ReturnValue::eFAILURE;
  }

  if (fighting == victim)
  {
    sendln("How can you rescue someone you are trying to kill?");
    return ReturnValue::eFAILURE;
  }

  if (!can_be_attacked(this, victim->fighting))
  {
    sendln("You cannot complete the rescue!");
    return ReturnValue::eFAILURE;
  }

  for (tmp_ch = dc_->world[in_room].people; tmp_ch &&
                                            (tmp_ch->fighting != victim);
       tmp_ch = tmp_ch->next_in_room)
    ;

  if (!tmp_ch)
  {
    act_to_character("But nobody is fighting $M?", this, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(SKILL_RESCUE))
    return ReturnValue::eSUCCESS;

  if (!skill_success(victim, SKILL_RESCUE))
  {
    sendln("You fail the rescue.");
    return ReturnValue::eFAILURE;
  }

  sendln("Banzai! To the rescue...");
  act_to_character("You are rescued by $N, you are confused!", victim, 0, this, 0);
  act_to_room("$n heroically rescues $N.", this, 0, victim, NOTVICT);

  qint32 tempwait = GET_WAIT(this);
  qint32 tempvictwait = GET_WAIT(victim);

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

  WAIT_STATE(this, MAX<quint64>(DC::PULSE_VIOLENCE * 2, tempwait));
  WAIT_STATE(victim, MAX<quint64>(DC::PULSE_VIOLENCE * 2, tempvictwait));
  return ReturnValue::eSUCCESS;
}

command_return_t do_bladeshield(CharacterPtr ch, QString argument, cmd_t cmd)
{
  affected_type af;
  qint32 duration = 12;

  if (!ch->canPerform(SKILL_BLADESHIELD))
  {
    ch->sendln("You'd cut yourself to ribbons just trying!");
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_BLADESHIELD) && !ch->isImmortalPlayer())
  {
    ch->sendln("Your body is still recovering from your last use of the blade shield technique.");
    return ReturnValue::eFAILURE;
  }

  if (!(ch->fighting))
  {
    ch->sendln("But you aren't fighting anyone!");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BLADESHIELD))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_BLADESHIELD))
  {
    act_to_room("$n starts swinging $s weapons around but stops before narrowly avoiding dismembering $mself.", ch, 0, 0, NOTVICT);
    ch->sendln("You try to begin the bladeshield technique and almost chop off your own arm!");
    duration /= 2;
  }
  else
  {
    act_to_room("$n forms a defensive wall of swinging weapons around $mself.", ch, 0, 0, NOTVICT);
    send_to_char("The world around you slows to a crawl, the weapons around you swing as if through water.  "
                 "Your mind clears of all but thrust angles as your body and mind enter completely into the "
                 "blade shield technique.\r\n",
                 ch);
    SET_BIT(ch->combat, COMBAT_BLADESHIELD1);
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  af.type = SKILL_BLADESHIELD;
  af.duration = duration;
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  return ReturnValue::eSUCCESS;
}

/* BEGIN UTILITY FUNCTIONS FOR "Guard" */

// return true on guard doing anything
// otherwise false
qint32 handle_any_guard(CharacterPtr ch)
{
  if (!ch->guarded_by)
    return false;

  CharacterPtr guard = {};

  // search the room for my guard
  for (follow_type *curr = ch->guarded_by; curr;)
  {
    for (auto vict = dc_->world[ch->in_room].people; vict; vict = vict->next_in_room)
      if (vict == curr->follower)
      {
        curr = {};
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
    guard->do_rescue(ch->name().split(' '));
    if (ch->fighting)
      return true;
    else
      return false;
  }
  return false;
}

CharacterPtr is_guarding_me(CharacterPtr ch, CharacterPtr guard)
{
  follow_type *curr = ch->guarded_by;

  while (curr)
  {
    if (curr->follower == guard)
      return guard;
    curr = curr->next;
  }

  return {};
}

void stop_guarding(CharacterPtr guard)
{
  if (!guard->guarding) // i'm not guarding anyone:)  get out
    return;

  CharacterPtr victim = guard->guarding;
  follow_type *curr = victim->guarded_by;
  follow_type *last = {};

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
  guard->guarding = {};
}

void start_guarding(CharacterPtr guard, CharacterPtr victim)
{
  auto curr = new follow_type;
  curr->follower = guard;
  curr->next = victim->guarded_by;
  victim->guarded_by = curr;
  guard->guarding = victim;
}

void stop_guarding_me(CharacterPtr victim)
{
  QString buf;
  follow_type *curr = victim->guarded_by;
  follow_type *last;

  while (curr)
  {
    dc_sprintf(buf, "You stop trying to guard %s.\r\n", qPrintable(victim->shortdesc_or_name()));
    curr->follower->send(buf);
    curr->follower->guarding = {};
    last = curr;
    curr = curr->next;
    last = {};
  }

  victim->guarded_by = {};
}

/* END UTILITY FUNCTIONS FOR "Guard" */

command_return_t do_guard(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  CharacterPtr victim = {};

  if (!ch->isNonPlayer() && (!ch->has_skill(SKILL_GUARD) || !ch->has_skill(SKILL_RESCUE)))
  {
    ch->sendln("You have no idea how to be a full time bodyguard.");
    return ReturnValue::eFAILURE;
  }
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    ch->sendln("Some bodyguard you are.  That person isn't even here!");
    return ReturnValue::eFAILURE;
  }

  if (ch == victim)
  {
    ch->sendln("You stop guarding anyone.");
    stop_guarding(ch);
    return ReturnValue::eSUCCESS;
  }

  if (victim == ch->guarding)
  {
    ch->sendln("You are already guarding that person.");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_GUARD))
    return ReturnValue::eSUCCESS;

  if (ch->guarding)
  {
    stop_guarding(ch);
    ch->sendln("You stop guarding anyone.");
    //      return ReturnValue::eFAILURE;
  }

  start_guarding(ch, victim);
  dc_sprintf(name, "You begin trying to guard %s.\r\n", qPrintable(victim->shortdesc_or_name()));
  ch->send(name);
  return ReturnValue::eSUCCESS;
}

command_return_t do_tactics(CharacterPtr ch, QString argument, cmd_t cmd)
{
  affected_type af;

  if (!ch->canPerform(SKILL_TACTICS))
  {
    ch->sendln("You just don't have the mind for strategic battle.");
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_TACTICS_TIMER))
  {
    ch->sendln("You will need more time to work out your tactics.");
    return ReturnValue::eFAILURE;
  }
  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You have no group to command.");
    return ReturnValue::eFAILURE;
  }

  qint32 grpsize = {};
  for (CharacterPtr tmp_char = dc_->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
  {
    if (tmp_char == ch)
      continue;
    if (!ARE_GROUPED(ch, tmp_char))
      continue;
    grpsize++;
  }

  if (!charge_moves(ch, SKILL_TACTICS, grpsize))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_TACTICS))
  {
    ch->sendln("Guess you just weren't the Patton you thought you were.");
    act_to_room("$n goes on about team not being spelled with an 'I' or something.", ch, 0, 0, 0);
  }
  else
  {
    act_to_room("$n takes command coordinating $s group's efforts.", ch, 0, 0, 0);
    ch->sendln("You take command coordinating the group's attacks.");

    af.type = SKILL_TACTICS_TIMER;
    af.duration = 1 + ch->has_skill(SKILL_TACTICS) / 10;
    af.modifier = {};
    af.location = {};
    af.bitvector = -1;
    affect_to_char(ch, &af);

    for (CharacterPtr tmp_char = dc_->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
    {
      if (tmp_char == ch)
        continue;
      if (!ARE_GROUPED(ch, tmp_char))
        continue;

      affect_from_char(tmp_char, SKILL_TACTICS, SUPPRESS_MESSAGES);
      affect_from_char(tmp_char, SKILL_TACTICS, SUPPRESS_MESSAGES);

      act_to_victim("$n's leadership makes you feel more comfortable with battle.", ch, 0, tmp_char, 0);
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
  return ReturnValue::eSUCCESS;
}

command_return_t do_make_camp(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr i, next_i;
  qint32 learned = ch->has_skill(SKILL_MAKE_CAMP);
  affected_type af;

  if (!ch->isNonPlayer() && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("You do not know how to set up a safe camp.");
    return ReturnValue::eFAILURE;
  }

  if (isSet(dc_->world[ch->in_room].room_flags, SAFE) || isSet(dc_->world[ch->in_room].room_flags, UNSTABLE) ||
      isSet(dc_->world[ch->in_room].room_flags, FALL_NORTH) || isSet(dc_->world[ch->in_room].room_flags, FALL_SOUTH) ||
      isSet(dc_->world[ch->in_room].room_flags, FALL_EAST) || isSet(dc_->world[ch->in_room].room_flags, FALL_WEST) ||
      isSet(dc_->world[ch->in_room].room_flags, FALL_UP) || isSet(dc_->world[ch->in_room].room_flags, FALL_DOWN) ||
      dc_->world[ch->in_room].sector_type == SECT_CITY || dc_->world[ch->in_room].sector_type == SECT_PAVED_ROAD)
  {
    ch->sendln("Something about this area inherently prohibits a rugged camp.");
    return ReturnValue::eFAILURE;
  }

  for (i = dc_->world[ch->in_room].people; i; i = next_i)
  {
    next_i = i->next_in_room;

    if ((i->isNonPlayer() && !IS_AFFECTED(i, AFF_CHARM) && !IS_AFFECTED(i, AFF_FAMILIAR)) || (i->fighting))
    {
      ch->sendln("This area does not yet feel secure enough to rest.");
      return ReturnValue::eFAILURE;
    }
  }

  if (ch->affected_by_spell(SKILL_MAKE_CAMP_TIMER))
  {
    ch->sendln("You cannot make another camp so soon.");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_MAKE_CAMP))
  {
    ch->sendln("You are unable to muster the strength to make a camp.");
    return ReturnValue::eFAILURE;
  }

  for (i = dc_->world[ch->in_room].people; i; i = next_i)
  {
    next_i = i->next_in_room;

    if (i->affected_by_spell(SKILL_MAKE_CAMP))
    {
      ch->sendln("There is already a camp setup here.");
      return ReturnValue::eFAILURE;
    }
  }

  WAIT_STATE(ch, (qint32)(DC::PULSE_VIOLENCE * 2.5));

  ch->sendln("You scan about for signs of danger as you clear an area to make camp...");
  act_to_room("$n scans about for signs of danger and clears an area to make camp...", ch, 0, 0, 0);

  if (!skill_success(ch, 0, SKILL_MAKE_CAMP))
  {
    send_to_room("The area does not yet feel secure enough to rest.\r\n", ch->in_room);
  }
  else
  {
    send_to_room("The area feels secure enough to get some rest.\r\n", ch->in_room);

    af.type = SKILL_MAKE_CAMP_TIMER;
    af.duration = 2 + learned / 9;
    af.modifier = {};
    af.location = {};
    af.bitvector = -1;

    affect_to_char(ch, &af);

    af.type = SKILL_MAKE_CAMP;
    af.duration = 1 + learned / 9;
    af.modifier = ch->in_room;
    af.location = {};
    af.bitvector = -1;

    affect_to_char(ch, &af);

    for (i = dc_->world[ch->in_room].people; i; i = next_i)
    {
      next_i = i->next_in_room;

      if (!i->affected_by_spell(SPELL_FARSIGHT) && !IS_AFFECTED(i, AFF_FARSIGHT))
      {
        af.type = SPELL_FARSIGHT;
        af.duration = -1;
        af.modifier = 111;
        af.location = {};
        af.bitvector = AFF_FARSIGHT;

        affect_to_char(i, &af);
      }
    }
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_triage(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 learned = ch->has_skill(SKILL_TRIAGE);
  affected_type af;

  if (ch->isMortalPlayer() && !learned)
  {
    ch->sendln("You do not know how to aid your regeneration in that way.");
    return ReturnValue::eFAILURE;
  }

  if (ch->fighting)
  {
    ch->sendln("You are a little too busy for that right now.");
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_TRIAGE_TIMER))
  {
    ch->sendln("You cannot take care of your battle wounds again so soon.");
    return ReturnValue::eFAILURE;
  }

  if (!ch->decrementMove(skill_cost.find(SKILL_TRIAGE)->second, "You're too tired to use this skill."))
  {
    return ReturnValue::eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  af.type = SKILL_TRIAGE_TIMER;
  af.modifier = {};
  af.duration = 6;
  af.location = {};
  af.bitvector = -1;
  affect_to_char(ch, &af);

  if (!skill_success(ch, 0, SKILL_TRIAGE))
  {
    ch->sendln("You pause to clean and bandage some of your more painful injuries but feel little improvement in your health.");
    act_to_room("$n pauses to try and bandage some of $s more painful injuries.", ch, 0, 0, 0);
    return ReturnValue::eSUCCESS;
  }

  af.type = SKILL_TRIAGE;
  af.modifier = learned / 3;
  af.duration = 2;
  af.location = APPLY_HP_REGEN;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  ch->sendln("You pause to clean and bandage some of your more painful injuries and speed the healing process.");
  act_to_room("$n pauses to try and bandage some of $s more painful injuries.", ch, 0, 0, 0);

  return ReturnValue::eSUCCESS;
}

command_return_t do_battlesense(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 learned = ch->has_skill(SKILL_BATTLESENSE);
  affected_type af;

  if (!ch->isNonPlayer() && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("You do not know how to heighten your battle awareness.");
    return ReturnValue::eFAILURE;
  }

  if (!ch->fighting)
  {
    ch->sendln("You must be in battle to heighten your combat awareness!");
    return ReturnValue::eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  if (!skill_success(ch, 0, SKILL_BATTLESENSE))
  {
    ch->sendln("Your body and mind, tired from continued battle, are unable to reach full combat readiness.");
  }
  else
  {
    ch->sendln("Your awareness heightens dramatically as the rush of battle courses through your body.");
    act_to_room("$n's movements become quick and calculated as $s senses heighten with the rush of battle.", ch, 0, 0, 0);

    af.type = SKILL_BATTLESENSE;
    af.location = {};
    af.modifier = 10 + learned / 4;
    af.duration = 1 + learned / 11;
    af.bitvector = -1;

    affect_to_char(ch, &af, DC::PULSE_VIOLENCE);
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_smite(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr vict = {};
  QString name;
  qint32 learned = ch->has_skill(SKILL_SMITE);
  affected_type af;

  if (!ch->isNonPlayer() && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("You do not know how to smite your enemies effectively.");
    return ReturnValue::eFAILURE;
  }

  if (argument.isEmpty())
  {
    if (!ch->fighting)
    {
      ch->sendln("Smite whom?");
      return ReturnValue::eFAILURE;
    }
    else
      vict = ch->fighting;
  }

  one_argument(argument, name);

  if (!argument.isEmpty())
    if (!(vict = ch->get_char_room_vis(name)))
    {
      ch->sendln("Smite whom?");
      return ReturnValue::eFAILURE;
    }

  if (ch == vict)
  {
    ch->sendln("There's a suicide command for that...");
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch) || !can_be_attacked(ch, vict))
  {
    act_to_character("You cannot attack $M", ch, 0, vict, 0);
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_SMITE_TIMER))
  {
    ch->sendln("You cannot smite your enemies again so soon.");
    return ReturnValue::eFAILURE;
  }

  af.type = SKILL_SMITE_TIMER;
  af.location = {};
  af.modifier = {};
  af.duration = 22 - learned / 10;
  af.bitvector = -1;

  affect_to_char(ch, &af);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 4);

  if (!skill_success(ch, vict, SKILL_SMITE))
  {
    act_to_character("Your less-than-mighty challenge fails to improve your attack of $N.", ch, 0, vict, 0);
    act_to_victim("$n makes a pathetic attempt at shouting a challenge as $e attempts to strike you.", ch, 0, vict, 0);
    act_to_room("$n makes a pathetic attempt at shouting a challenge as $e attempts to strike $N.", ch, 0, vict, NOTVICT);
  }
  else
  {
    act_to_character("You shout a mighty challenge and begin to assault $N with lethal efficiency.", ch, 0, vict, 0);
    act_to_victim("$n shouts a mighty challenge and begins to assault you with lethal efficiency.", ch, 0, vict, 0);
    act_to_room("$n shouts a mighty challenge and begins to assault $N with lethal efficiency.", ch, 0, vict, NOTVICT);

    af.type = SKILL_SMITE;
    af.location = {};
    af.modifier = {};
    af.victim = vict;
    af.duration = 1 + learned / 16;
    af.bitvector = -1;

    affect_to_char(ch, &af, DC::PULSE_VIOLENCE);
  }

  return ch->fighting ? ReturnValue::eSUCCESS : attack(ch, vict, TYPE_UNDEFINED);
}

command_return_t do_leadership(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 learned = ch->has_skill(SKILL_LEADERSHIP);
  affected_type af;

  if (!ch->isNonPlayer() && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("You do not know that ability.");
    return ReturnValue::eFAILURE;
  }

  if (ch->master || !ch->followers)
  {
    ch->sendln("You must be leading a group to call upon your leadership skills.");
    return ReturnValue::eFAILURE;
  }

  if (isSet(dc_->world[ch->in_room].room_flags, SAFE) || isSet(dc_->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("Stop trying to show off.");
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_LEADERSHIP))
  {
    ch->sendln("You and your followers are already inspired.");
    return ReturnValue::eFAILURE;
  }

  ch->sendln("You loudly call, 'Once more unto the breach, dear friends!'");
  act_to_room("$n loudly calls, 'Once more unto the breach, dear friends!'", ch, 0, 0, 0);

  WAIT_STATE(ch, (qint32)(DC::PULSE_VIOLENCE * 1.5));

  if (!skill_success(ch, 0, SKILL_LEADERSHIP))
  {
    ch->sendln("Your bravery miserably fails to inspire.");
    act_to_room("$n's bravery miserably fails to inspire.", ch, 0, 0, 0);
  }
  else
  {
    ch->sendln("Your bravery lends you additional might and inspires the group!");
    act_to_room("$n's bravery lends $m additional might and inspires the group!", ch, 0, 0, 0);

    af.type = SKILL_LEADERSHIP;
    af.duration = 1 + learned / 20;
    af.modifier = learned / 19; // cap on bonus
    af.location = {};
    af.bitvector = -1;

    affect_to_char(ch, &af);

    ch->changeLeadBonus = true;
    ch->curLeadBonus = get_leadership_bonus(ch);
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_perseverance(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 learned = ch->has_skill(SKILL_PERSEVERANCE);
  affected_type af;

  if (!ch->isNonPlayer() && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("Your lack of fortitude is stunning.");
    return ReturnValue::eFAILURE;
  }

  if (!ch->fighting)
  {
    ch->sendln("What, exactly, are you trying to persevere through?");
    return ReturnValue::eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  if (!skill_success(ch, 0, SKILL_PERSEVERANCE))
  {
    ch->sendln("Your movements seem unchanged, though you strive to gain some momentum in your actions.");
  }
  else
  {
    ch->sendln("Your movements seem to build energy and gain momentum as you fight with renewed vigor!");
    act_to_room("$n seems to build energy and $s movements gain momentum as the battle drags on...", ch, 0, 0, 0);

    af.type = SKILL_PERSEVERANCE;
    af.location = {};
    af.duration = 1 + learned / 11;
    af.modifier = learned;
    af.bitvector = -1;

    affect_to_char(ch, &af, DC::PULSE_VIOLENCE);
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_defenders_stance(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr vict = {};
  qint32 learned = ch->has_skill(SKILL_DEFENDERS_STANCE);
  affected_type af;

  if (!ch->isNonPlayer() && ch->getLevel() <= ARCHANGEL && !learned)
  {
    ch->sendln("You do not know how to use this to your advantage.");
    return ReturnValue::eFAILURE;
  }

  if (!ch->fighting)
  {
    ch->sendln("From whom are you trying to defend yourself?");
    return ReturnValue::eFAILURE;
  }

  vict = ch->fighting;
  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);

  if (!skill_success(ch, 0, SKILL_DEFENDERS_STANCE))
  {
    act_to_character("You attempt to brace yourself to defend against $N's onslaught but stumble and fall!", ch, 0, vict, 0);
    act_to_victim("$n attempts to brace $mself to defend against your onslaught but stumbles and falls!", ch, 0, vict, 0);
    act_to_room("$n attempts to brace $mself to defend against $N's onslaught but stumbles and falls!", ch, 0, vict, NOTVICT);
    ch->setSitting();
  }
  else
  {
    act_to_character("You brace yourself to defend against $N's onslaught.", ch, 0, vict, 0);
    act_to_victim("$n braces $mself to defend against your onslaught.", ch, 0, vict, 0);
    act_to_room("$n braces $mself to defend against $N's onslaught.", ch, 0, vict, NOTVICT);

    af.type = SKILL_DEFENDERS_STANCE;
    af.location = {};
    af.modifier = 5 + learned / 2;
    af.duration = 1 + learned / 12;
    af.bitvector = -1;

    affect_to_char(ch, &af, DC::PULSE_VIOLENCE);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_onslaught(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 learned = ch->has_skill(SKILL_ONSLAUGHT);
  affected_type af;

  if (ch->isMortalPlayer() && !learned)
  {
    ch->sendln("You do not know how to use this to your advantage.");
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_ONSLAUGHT_TIMER))
  {
    ch->sendln("You have not yet recovered from your previous onslaught attempt.");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_ONSLAUGHT))
  {
    ch->sendln("You do not have enough energy to attempt this onslaught.");
    return ReturnValue::eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  if (!skill_success(ch, 0, SKILL_ONSLAUGHT))
  {
    act_to_character("Obviously you have a bit more to learn about battle...slowass.", ch, 0, 0, 0);
    act_to_room("$n waves $s weapon in the air in a futile attempt to look skillful.", ch, 0, 0, 0);

    af.type = SKILL_ONSLAUGHT_TIMER;
    af.location = {};
    af.modifier = {};
    af.duration = 7;
    af.bitvector = -1;
    affect_to_char(ch, &af);
  }
  else
  {
    act_to_character("Your attacks come fast and furious as you harness your battle expertise.", ch, 0, 0, 0);
    act_to_room("$n's attacks come fast and furious as $e harnesses $s battle expertise.", ch, 0, 0, 0);

    af.type = SKILL_ONSLAUGHT;
    af.location = {};
    af.modifier = {};
    af.duration = 1 + learned / 15;
    af.bitvector = -1;

    affect_to_char(ch, &af);

    af.type = SKILL_ONSLAUGHT_TIMER;
    af.location = {};
    af.modifier = {};
    af.duration = 7 + learned / 15;
    af.bitvector = -1;

    affect_to_char(ch, &af);
  }
  return ReturnValue::eSUCCESS;
}
