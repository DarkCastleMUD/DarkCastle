/************************************************************************
| $Id: cl_thief.cpp,v 1.207 2015/05/27 05:09:35 jhhudso Exp $
| cl_thief.C
| Functions declared primarily for the thief class; some may be used in
|   other classes, but they are mainly thief-oriented.
*/
#include "DC/DC.h"

extern qint32 rev_dir[];

qint32 find_door(CharacterPtr ch, QString type, QString dir);
qint32 get_weapon_damage_type(ObjectPtr wielded);
qint32 check_autojoiners(CharacterPtr ch, qint32 skill = 0);
qint32 check_joincharmie(CharacterPtr ch, qint32 skill = 0);

command_return_t do_eyegouge(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name;
  qint32 level = ch->has_skill(SKILL_EYEGOUGE);

  if (ch->isNonPlayer())
    level = 50 + ch->getLevel() / 2;

  argument = one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)) && !(victim = ch->fighting))
  {
    ch->sendln("There is no one like that here to gouge.");
    return ReturnValue::eFAILURE;
  }
  if (IS_AFFECTED(victim, AFF_BLIND))
  {
    ch->sendln("They are already blinded!");
    return ReturnValue::eFAILURE;
  }
  if (victim == ch)
  {
    ch->sendln("That sounds...stupid.");
    return ReturnValue::eFAILURE;
  }

  if (!level)
  {
    ch->sendln("You would...if you knew how.");
    return ReturnValue::eFAILURE;
  }

  if (!can_be_attacked(ch, victim) || !can_attack(ch))
    return ReturnValue::eFAILURE;

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Trying to eyegouge a bladeshielded opponent would be suicide!");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_EYEGOUGE))
    return ReturnValue::eSUCCESS;

  qint32 retval = {};
  if (!skill_success(ch, victim, SKILL_EYEGOUGE))
  {
    retval = damage(ch, victim, 0, TYPE_PIERCE, SKILL_EYEGOUGE);
  }
  else
  {
    if (victim->affected_by_spell(SKILL_BATTLESENSE) && ch->dc_->number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act_to_character("$N's heightened battlesense sees your eyegouge coming from a mile away.", ch, 0, victim, 0);
      act_to_victim("Your heightened battlesense sees $n's eyegouge coming from a mile away.", ch, 0, victim, 0);
      act_to_room("$N's heightened battlesense sees $n's eyegouge coming from a mile away.", ch, 0, victim, NOTVICT);
      level = {};
    }
    else if (!isSet(victim->immune, TYPE_PIERCE))
    {
      SETBIT(victim->affected_by, AFF_BLIND);
      SET_BIT(victim->combat, COMBAT_THI_EYEGOUGE);
      affected_type af;
      af.type = SKILL_EYEGOUGE;
      af.location = APPLY_AC;
      af.modifier = level / 2;
      af.duration = 1;
      af.bitvector = -1;
      affect_to_char(victim, &af, DC::PULSE_VIOLENCE);
    }

    retval = damage(ch, victim, level * 2, TYPE_PIERCE, SKILL_EYEGOUGE);
  }

  if (!SOMEONE_DIED(retval) || (ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_WIMPY)))
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  return retval | ReturnValue::eSUCCESS;
}

command_return_t Character::do_backstab(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim;

  qint32 was_in = {};
  qint32 retval;

  QString name = arguments.value(0);

  if (!has_skill(SKILL_BACKSTAB) && !isNonPlayer())
  {
    sendln("You don't know how to backstab people!");
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_char_room_vis(name)))
  {
    sendln("Backstab whom?");
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    sendln("How can you sneak up on yourself?");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_HUGE))
  {
    sendln("You cannot backstab someone that HUGE!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_SWARM))
  {
    sendln("You cannot target just one to backstab!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_TINY))
  {
    sendln("You cannot target someone that tiny to backstab!");
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_ALERT))
  {
    act_to_character("$E is too alert and nervous looking; you are unable to sneak behind!", this, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(SKILL_BACKSTAB))
    return ReturnValue::eSUCCESS;

  qint32 min_hp = (qint32)(GET_MAX_HIT(this) / 5);
  min_hp = MIN(min_hp, 25);

  if (getHP() < min_hp)
  {
    sendln("You are feeling too weak right now to attempt such a bold maneuver.");
    return ReturnValue::eFAILURE;
  }

  if (!equipment[WEAR_WIELD])
  {
    sendln("You need to wield a weapon to make it a success.");
    return ReturnValue::eFAILURE;
  }

  if (equipment[WEAR_WIELD]->obj_flags.value[3] != 11 && equipment[WEAR_WIELD]->obj_flags.value[3] != 9)
  {
    sendln("You can't stab without a stabbing weapon...");
    return ReturnValue::eFAILURE;
  }

  if (victim->fighting)
  {
    sendln("You can't backstab a fighting person, they are too alert!");
    return ReturnValue::eFAILURE;
  }

  // Check the killer/victim
  if ((getLevel() < G_POWER) || isNonPlayer())
  {
    if (!can_attack(this) || !can_be_attacked(this, victim))
      return ReturnValue::eFAILURE;
  }

  qint32 itemp = dc_->number(1, 100);
  if (isPlayer() && victim->isPlayer())
  {
    if (victim->getLevel() > getLevel())
      itemp = {}; // not gonna happen
    else if (GET_MAX_HIT(victim) > GET_MAX_HIT(this))
    {
      if (GET_MAX_HIT(victim) * 0.85 > GET_MAX_HIT(this))
        itemp--;
      if (GET_MAX_HIT(victim) * 0.70 > GET_MAX_HIT(this))
        itemp--;
      if (GET_MAX_HIT(victim) * 0.55 > GET_MAX_HIT(this))
        itemp--;
      if (GET_MAX_HIT(victim) * 0.40 > GET_MAX_HIT(this))
        itemp--;
    }
  }

  // record the room I'm in.  Used to make sure a dual can go off.
  was_in = in_room;

  // Will this be a single or dual backstab this round?
  bool perform_dual_backstab = false;
  if ((((isPlayer() && GET_CLASS(this) == CLASS_THIEF && has_skill(SKILL_DUAL_BACKSTAB)) || getLevel() >= ARCHANGEL) || (isNonPlayer() && getLevel() > 70)) && (equipment[WEAR_SECOND_WIELD]) && ((equipment[WEAR_SECOND_WIELD]->obj_flags.value[3] == 11) || (equipment[WEAR_SECOND_WIELD]->obj_flags.value[3] == 9)) && (cmd != cmd_t::SBS))
  {
    if (skill_success(victim, SKILL_DUAL_BACKSTAB) || isNonPlayer())
    {
      perform_dual_backstab = true;
    }
  }

  WAIT_STATE(this, DC::PULSE_VIOLENCE * 1);

  // failure
  if (AWAKE(victim) && !skill_success(victim, SKILL_BACKSTAB))
  {
    // If this is stab 1 of 2 for a dual backstab, we dont want people autojoining on the first stab
    if (perform_dual_backstab && isPlayer())
    {
      player->unjoinable = true;
      retval = damage(this, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB);
      player->unjoinable = false;
    }
    else
    {
      retval = damage(this, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB);
    }
  }
  // success
  else if (!victim->isImmortalPlayer() &&
           victim->getLevel() <= (getLevel() + 19) &&
           (isImmortalPlayer() || itemp > 95 || (victim->isPlayer() && isSet(victim->player->punish, PUNISH_UNLUCKY))) &&
           ((equipment[WEAR_WIELD]->obj_flags.value[3] == 11 && !isSet(victim->immune, ISR_PIERCE)) || (equipment[WEAR_WIELD]->obj_flags.value[3] == 9 && !isSet(victim->immune, ISR_STING))))
  {
    act("$N crumples to the ground, $S body still quivering from "
        "$n's brutal assassination.",
        this, 0, victim, TO_ROOM, NOTVICT);
    act("You feel $n's blade slip into your heart, and all goes black.",
        this, 0, victim, TO_VICT, 0);
    act("BINGO! You brutally assassinate $N, and $S body crumples "
        "before you.",
        this, 0, victim, TO_CHAR, 0);
    return damage(this, victim, 9999999, TYPE_UNDEFINED, SKILL_BACKSTAB);
  }
  else
  {
    // If this is stab 1 of 2 for a dual backstab, we dont want people autojoining on the first stab
    if (perform_dual_backstab && isPlayer())
    {
      player->unjoinable = true;
      retval = attack(this, victim, SKILL_BACKSTAB, WEAR_WIELD);
      player->unjoinable = false;
    }
    else
    {
      retval = attack(this, victim, SKILL_BACKSTAB, WEAR_WIELD);
    }
  }

  if ((retval & ReturnValue::eVICT_DIED) && !(retval & ReturnValue::eCH_DIED))
  {
    return retval;
  }

  if (retval & ReturnValue::eCH_DIED)
    return retval;

  if (retval & ReturnValue::eVICT_DIED)
  {
    return retval;
  }

  if (!charExists(victim)) // heh
  {
    return ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED;
  }

  // If we're intended to have a dual backstab AND we still can
  if (perform_dual_backstab == true && charge_moves(SKILL_BACKSTAB) && GET_POS(victim) != position_t::DEAD && victim->in_room != DC::NOWHERE)
  {
    if (was_in == in_room)
    {
      if (AWAKE(victim) && !skill_success(victim, SKILL_BACKSTAB))
      {
        retval = damage(this, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB, WEAR_SECOND_WIELD);
      }
      else
      {
        retval = attack(this, victim, SKILL_BACKSTAB, WEAR_SECOND_WIELD);
      }

      //     if (!SOMEONE_DIED(retval)) {
      // check_autojoiners(this, 0);
      //     }
    }
  }

  if (!SOMEONE_DIED(retval))
  {
    // SET_BIT(retval, check_autojoiners(this,1));
    // if (!SOMEONE_DIED(retval))

    // if (IS_AFFECTED(this, AFF_CHARM)) SET_BIT(retval, check_joincharmie(this,1));
    // if (SOMEONE_DIED(retval)) return retval;
    if (c_class == CLASS_THIEF && victim->isPlayer())
    {
      WAIT_STATE(this, DC::PULSE_VIOLENCE * 2);
    }
  }

  return retval;
}

command_return_t do_circle(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  qint32 retval;

  if (!ch->canPerform(SKILL_CIRCLE))
  {
    ch->sendln("You do not know how to circle!");
    return ReturnValue::eFAILURE;
  }

  qint32 min_hp = (qint32)(GET_MAX_HIT(ch) / 5);
  min_hp = MIN(min_hp, 25);

  if (ch->getHP() < min_hp)
  {
    ch->send("You are feeling too weak right now to attempt such a bold maneuver.");
    return ReturnValue::eFAILURE;
  }

  if (ch->fighting)
    victim = ch->fighting;
  else
  {
    ch->sendln("You have be in combat to perform this action.  Try using 'backstab' instead.");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_HUGE) &&
      ch->has_skill(SKILL_CIRCLE) <= 80)
  {
    ch->sendln("You cannot circle behind someone that HUGE!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_SWARM) &&
      ch->has_skill(SKILL_CIRCLE) <= 80)
  {
    ch->sendln("You cannot pick just one to circle behind!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_TINY) &&
      ch->has_skill(SKILL_CIRCLE) <= 80)
  {
    ch->sendln("You cannot target something that tiny to circle behind!");
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_NO_CIRCLE))
  {
    act_to_character("$N notices your attempt and turns $S back away from you!", ch, 0, victim, 0);
    act_to_room("$N notices $n's attempt to circle behind $M and backs away quickly!", ch, 0, victim, NOTVICT);
    act_to_victim("You see $n try to circle around you and move quickly to block $s access!", ch, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("How can you sneak up on yourself?");
    return ReturnValue::eFAILURE;
  }

  if (!ch->equipment[WEAR_WIELD])
  {
    ch->sendln("You need to wield a weapon to make it a success.");
    return ReturnValue::eFAILURE;
  }

  // Check the killer/victim
  if ((ch->getLevel() < G_POWER) || ch->isNonPlayer())
  {
    if (!can_attack(ch) || !can_be_attacked(ch, victim))
      return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_CIRCLE))
    return ReturnValue::eSUCCESS;

  bool stabbingCircle = false;

  switch (get_weapon_damage_type(ch->equipment[WEAR_WIELD]))
  {
  case TYPE_PIERCE:
  case TYPE_STING:
    stabbingCircle = true;
    break;
  default:
    //     ch->sendln("Only certain weapons can be used for backstabbing, this is not one of them.");
    //     return ReturnValue::eFAILURE;
    break;
  }

  if (ch == victim->fighting && !IS_AFFECTED(victim, AFF_BLACKJACK))
  {
    ch->sendln("You can't break away while that person is hitting you!");
    return ReturnValue::eFAILURE;
  }

  act_to_character("You circle around your target...", ch, 0, 0, 0);
  act_to_room("$n circles around $s target...", ch, 0, 0, INVIS_NULL);
  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  QString buffer;
  dc_sprintf(buffer, "%s", qPrintable(victim->name()));

  if (AWAKE(victim) && !skill_success(ch, victim, SKILL_CIRCLE))
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB, WEAR_WIELD);
  else if (victim->affected_by_spell(SKILL_BATTLESENSE) &&
           dc_->number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
  {
    act_to_character("$N's heightened battlesense sees your circle coming from a mile away.", ch, 0, victim, 0);
    act_to_victim("Your heightened battlesense sees $n's circle coming from a mile away.", ch, 0, victim, 0);
    act_to_room("$N's heightened battlesense sees $n's circle coming from a mile away.", ch, 0, victim, NOTVICT);
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB, WEAR_WIELD);
  }
  else
  {
    SET_BIT(ch->combat, COMBAT_CIRCLE);
    if (stabbingCircle)
      retval = one_hit(ch, victim, SKILL_BACKSTAB, WEAR_WIELD);
    else
      retval = one_hit(ch, victim, SKILL_CIRCLE, WEAR_WIELD);

    if (SOMEONE_DIED(retval))
      return retval;

    // Now go for dual backstab
    if (ch->equipment[WEAR_SECOND_WIELD] && (ch->has_skill(SKILL_DUAL_BACKSTAB) || (ch->getLevel() >= ARCHANGEL)))
    {
      WAIT_STATE(ch, DC::PULSE_VIOLENCE);
      if (AWAKE(victim) && !skill_success(ch, victim, SKILL_DUAL_BACKSTAB))
        retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB, WEAR_SECOND_WIELD);
      else
      {
        SET_BIT(ch->combat, COMBAT_CIRCLE);

        switch (get_weapon_damage_type(ch->equipment[WEAR_SECOND_WIELD]))
        {
        case TYPE_PIERCE:
        case TYPE_STING:
          retval = one_hit(ch, victim, SKILL_BACKSTAB, WEAR_SECOND_WIELD);
          break;
        default:
          retval = one_hit(ch, victim, SKILL_CIRCLE, WEAR_SECOND_WIELD);
          break;
        }
      }
    }
  }

  if (!SOMEONE_DIED(retval))
  {
    SET_BIT(retval, check_autojoiners(ch, 1));
    if (!SOMEONE_DIED(retval))
      if (IS_AFFECTED(ch, AFF_CHARM))
        SET_BIT(retval, check_joincharmie(ch, 1));
    if (SOMEONE_DIED(retval))
      return retval;
  }

  return retval;
}

command_return_t do_trip(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim = {};
  QString name;
  qint32 retval;

  if (!ch->canPerform(SKILL_TRIP))
  {
    ch->sendln("You should learn how to trip first!");
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);

  /*  if (GET_CLASS(ch) == CLASS_BARD && IS_SINGING(ch))
    {
      QList<songInfo>::iterator i;

      for(i = ch->songs.begin(); i != ch->songs.end(); ++i) {
       if((*i).song_number == SKILL_SONG_CRUSHING_CRESCENDO - SKILL_SONG_BASE) {
         ch->sendln("You are too distracted by your song to do this.");
    return ReturnValue::eFAILURE;
       }
      }
    }
    */
  if (name.isEmpty() && ch->fighting)
  {
    victim = ch->fighting;
  }
  else
    victim = ch->get_char_room_vis(name);

  if (!victim)
  {
    ch->sendln("Trip whom?");
    return ReturnValue::eFAILURE;
  }

  if (ch->in_room != victim->in_room)
  {
    ch->sendln("That person seems to have left.");
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("(You would look pretty silly trying to trip yourself.)");
    return ReturnValue::eFAILURE;
  }

  if (!can_be_attacked(ch, victim) || !can_attack(ch))
    return ReturnValue::eFAILURE;

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("Tripping a bladeshielded opponent would be impossible!");
    return ReturnValue::eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_IRON_ROOTS))
  {
    act_to_character("You try to trip $N but tree roots around $S legs keep $M upright.", ch, 0, victim, 0);
    act_to_victim("$n trips you but the roots around your legs keep you from falling.", ch, 0, victim, 0);
    act_to_room("The tree roots support $N keeping $M from falling after $n's trip.", ch, 0, victim, NOTVICT);
    WAIT_STATE(ch, 2 * DC::PULSE_VIOLENCE);
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_TRIP))
    return ReturnValue::eSUCCESS;

  qint32 modifier = ch->get_stat(attribute_t::DEXTERITY) - victim->get_stat(attribute_t::DEXTERITY);
  if (modifier > 10)
    modifier = 10;
  if (modifier < -10)
    modifier = -10;

  if (!skill_success(ch, victim, SKILL_TRIP, modifier))
  {
    act_to_victim("$n fumbles clumsily as $e attempts to trip you!", ch, nullptr, victim, 0);
    act_to_character("You fumble the trip!", ch, nullptr, victim, 0);
    act_to_room("$n fumbles as $e tries to trip $N!", ch, nullptr, victim, NOTVICT);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_TRIP);
  }
  else
  {
    if (victim->affected_by_spell(SKILL_BATTLESENSE) &&
        dc_->number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act_to_character("$N's heightened battlesense sees your trip coming from a mile away.", ch, 0, victim, 0);
      act_to_victim("Your heightened battlesense sees $n's trip coming from a mile away.", ch, 0, victim, 0);
      act_to_room("$N's heightened battlesense sees $n's trip coming from a mile away.", ch, 0, victim, NOTVICT);
    }
    else
    {
      act_to_victim("$n trips you and you go down!", ch, nullptr, victim, 0);
      act_to_character("You trip $N and $N goes down!", ch, nullptr, victim, 0);
      act_to_room("$n trips $N and $N goes down!", ch, nullptr, victim, NOTVICT);
      if (GET_POS(victim) > position_t::SITTING)
        victim->setSitting();
      SET_BIT(victim->combat, COMBAT_BASH2);
      WAIT_STATE(victim, DC::PULSE_VIOLENCE * 1);
    }
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_TRIP);
  }
  return retval;
}

command_return_t do_sneak(CharacterPtr ch, QString argument, cmd_t cmd)
{

  auto &arena = dc_->arena_;
  affected_type af;

  if ((ch->in_room >= 0 && ch->in_room <= dc_->top_of_world) && ch->room().isArena() && arena.isPotato())
  {
    ch->sendln("You can't do that in a potato arena ya sneaky bastard!");
    return ReturnValue::eFAILURE;
  }

  if (!ch->canPerform(SKILL_SNEAK))
  {
    ch->sendln("You just don't seem like the sneaky type.");
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_SNEAK))
  {
    affect_from_char(ch, SKILL_SNEAK);
    if (cmd != cmd_t::PALM)
    {
      ch->sendln("You won't be so sneaky anymore.");
      return ReturnValue::eFAILURE;
    }
  }

  if (!charge_moves(ch, SKILL_SNEAK))
    return ReturnValue::eSUCCESS;

  do_hide(ch, u""_s, cmd_t::LOOK);

  ch->sendln("You try to move silently for a while.");

  //   ch->skill_increase_check(SKILL_SNEAK, ch->has_skill(SKILL_SNEAK),
  // SKILL_INCREASE_HARD);

  af.type = SKILL_SNEAK;
  af.duration = MAX<level_t>(5, ch->getLevel() / 2);
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  affect_to_char(ch, &af);
  return ReturnValue::eSUCCESS;
}

command_return_t do_stalk(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  CharacterPtr leader;

  if (!ch->canPerform(SKILL_STALK))
  {
    ch->sendln("I bet you think you're a thief. ;)");
    return ReturnValue::eFAILURE;
  }

  if (!(*argument))
  {
    if (ch->master)
      ch->send(u"You are currently stalking %s.\r\n"_s.arg(qPrintable(ch->master->shortdesc_or_name())));
    else
      ch->sendln("Pick a name, any name.");
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);

  if (!(leader = ch->get_char_room_vis(name)))
  {
    ch->sendln("I see no person by that name here!");
    return ReturnValue::eFAILURE;
  }

  if (leader == ch)
  {
    if (!ch->master)
      ch->sendln("You are already following yourself.");
    else if (IS_AFFECTED(ch, AFF_GROUP))
      ch->sendln("You must first abandon your group.");
    else
      stop_follower(ch, follower_reasons_t::END_STALK);
    return ReturnValue::eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You must first abandon your group.");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_STALK))
    return ReturnValue::eSUCCESS;

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1);

  if (!skill_success(ch, leader, SKILL_STALK))
    do_follow(ch, argument);

  else
  {
    do_follow(ch, argument, cmd_t::TRACK);
    do_sneak(ch, argument, cmd_t::TRACK);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_hide(CharacterPtr ch, const QString argument, cmd_t cmd)
{
  auto &arena = dc_->arena_;
  if (!ch->canPerform(SKILL_HIDE))
  {
    if (cmd != cmd_t::LOOK)
      ch->sendln("You don't know how to hide. What do you think you are, a thief?");
    return ReturnValue::eFAILURE;
  }

  if ((ch->in_room >= 0 && ch->in_room <= dc_->top_of_world) &&
      ch->room().isArena() && arena.isPotato())
  {
    ch->sendln("You can't do that in a potato arena ya sneaky bastard!");
    return ReturnValue::eFAILURE;
  }

  for (auto curr = dc_->world[ch->in_room].people_; curr; curr = curr->next_in_room)
  {
    if (curr->fighting == ch)
    {
      ch->sendln("In the middle of combat?!  Impossible!");
      return ReturnValue::eFAILURE;
    }
  }

  if (!IS_AFFECTED(ch, AFF_HIDE))
    if (!charge_moves(ch, SKILL_HIDE))
      return ReturnValue::eSUCCESS;

  ch->sendln("You attempt to hide yourself.");

  if (!IS_AFFECTED(ch, AFF_HIDE))
    SETBIT(ch->affected_by, AFF_HIDE);
  /* See how well it worked on those currently in the room. */
  qint32 a, i;
  CharacterPtr temp;
  if (ch->isPlayer() && (a = ch->has_skill(SKILL_HIDE)))
  {
    for (i = {}; i < MAX_HIDE; i++)
      ch->player->hiding_from[i] = {};
    i = {};
    for (temp = dc_->world[ch->in_room].people_; temp; temp = temp->next_in_room)
    {
      if (ch == temp)
        continue;
      if (i >= MAX_HIDE)
        break;
      if (ch->dc_->number(1, 101) > a) // Failed.
      {
        ch->player->hiding_from[i] = temp;
        ch->player->hide[i++] = false;
      }
      else
      {
        ch->player->hiding_from[i] = temp;
        ch->player->hide[i++] = true;
      }
    }
  }
  return ReturnValue::eSUCCESS;
}

qint32 max_level(CharacterPtr ch)
{
  qint32 i = 0, lvl = {};
  for (; i < MAX_WEAR; i++)
    if (ch->equipment[i] && (GET_ITEM_TYPE(ch->equipment[i]) == ITEM_ARMOR || GET_ITEM_TYPE(ch->equipment[i]) == ITEM_WEAPON || GET_ITEM_TYPE(ch->equipment[i]) == ITEM_INSTRUMENT || GET_ITEM_TYPE(ch->equipment[i]) == ITEM_FIREWEAPON || GET_ITEM_TYPE(ch->equipment[i]) == ITEM_LIGHT || GET_ITEM_TYPE(ch->equipment[i]) == ITEM_CONTAINER) &&
        !isSet(ch->equipment[i]->obj_flags.extra_flags, ITEM_SPECIAL))
      lvl = MAX<level_t>(lvl, ch->equipment[i]->obj_flags.eq_level);
  if (lvl < 20)
    lvl = 20;
  return lvl;
}

// steal an ITEM... not gold
command_return_t do_steal(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  ObjectPtr obj, *loop_obj, next_obj;
  affected_type pthiefaf, *paf;
  QString victim_name;
  QString obj_name;
  QString buf;
  qint32 eq_pos;
  qint32 _exp;
  qint32 retval;
  ObjectPtr has_item = {};
  bool ohoh = false;
  qint32 chance = GET_HITROLL(ch) + ch->has_skill(SKILL_STEAL) / 4;

  argument = one_argument(argument, obj_name);
  one_argument(argument, victim_name);
  if (ch->c_class != CLASS_THIEF || !ch->has_skill(SKILL_STEAL))
  {
    ch->sendln("You are not experienced within that field.");
    return ReturnValue::eFAILURE;
  }
  //  if (ch->isPlayerObjectThief()){
  //     ch->sendln("You're too busy watching your back to steal anything right now!");
  //     return ReturnValue::eFAILURE;
  //  }
  pthiefaf.type = Character::PLAYER_OBJECT_THIEF;
  pthiefaf.duration = 10;
  pthiefaf.modifier = {};
  pthiefaf.location = APPLY_NONE;
  pthiefaf.bitvector = -1;

  if (!(victim = ch->get_char_room_vis(victim_name)))
  {
    ch->sendln("Steal what from who?");
    return ReturnValue::eFAILURE;
  }
  else if (victim == ch)
  {
    ch->sendln("Got it!\r\nYou receive 30000000000 experience.");
    return ReturnValue::eFAILURE;
  }

  if (GET_POS(victim) == position_t::DEAD)
  {
    ch->sendln("Don't steal from dead people!");
    return ReturnValue::eFAILURE;
  }

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if ((ch->getLevel() < (victim->getLevel() - 19)))
  {
    ch->sendln("That person is far too experienced for you to steal from.");
    return ReturnValue::eFAILURE;
  }

  if (isSet(dc_->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("No stealing permitted in safe areas!");
    return ReturnValue::eFAILURE;
  }

  if (check_make_camp(ch->in_room))
  {
    ch->sendln("You can't steal inside of a camp!");
    return ReturnValue::eFAILURE;
  }

  if (ch->room().isArena())
  {
    ch->sendln("Do what!? This is an Arena, go kill someone!");
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_CHARM))
  {
    return do_say(ch, "Nice try, silly thief.");
  }

  if (victim->fighting)
  {
    ch->sendln("You can't get close enough because of the fight.");
    return ReturnValue::eFAILURE;
  }

  /*  if(victim->isPlayer() &&
      !(victim->conn_) && !victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF) ){
      ch->sendln("That person is not really there.");
      return ReturnValue::eFAILURE;
    }*/

  if (!charge_moves(ch, SKILL_STEAL))
    return ReturnValue::eSUCCESS;

  WAIT_STATE(ch, 12); /* It takes TIME to steal */

  //  if(GET_POS(victim) <= position_t::SLEEPING &&
  //   GET_POS(victim) != position_t::STUNNED)
  // percent = -1; /* ALWAYS SUCCESS */

  if ((obj = get_obj_in_list_vis(ch, obj_name, victim->carrying)))
  {
    chance -= GET_OBJ_WEIGHT(obj);
    if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
      ch->sendln("That item is protected by the gods.");
      return ReturnValue::eFAILURE;
    }
    if (isSet(obj->obj_flags.extra_flags, ITEM_NEWBIE))
    {
      ch->sendln("That piece of equipment is protected by the powerful magics of the MUD-school elders.");
      return ReturnValue::eFAILURE;
    }
    if (dc_->obj_index[obj->item_number].vnum() == CHAMPION_ITEM)
    {
      ch->send("You must earn that flag, no stealing allowed!");
      return ReturnValue::eFAILURE;
    }
    if (victim->isNonPlayer() && isexact("prize", obj->name()))
    {
      ch->sendln("You have to HUNT the targets...its not a Treasture Steal!");
      return ReturnValue::eFAILURE;
    }
    if (GET_OBJ_WEIGHT(obj) > 50)
    {
      ch->sendln("That item is too heavy to steal.");
      return ReturnValue::eFAILURE;
    }

    qint32 mod = ch->has_skill(SKILL_STEAL) - chance;
    if (!skill_success(ch, victim, SKILL_STEAL, 0 - mod))
    {
      set_cantquit(ch, victim);
      ch->sendln("Oops, that was clumsy...");
      ohoh = true;
      if (!number(0, 4))
      {
        act_to_victim("$n tried to steal something from you!", ch, 0, victim, 0);
        act_to_room("$n tries to steal something from $N.", ch, 0, victim, INVIS_NULL | NOTVICT);
        REMBIT(ch->affected_by, AFF_HIDE);
      }
      else
      {
        act_to_character("You managed to keep $N unaware of your failed attempt.", ch, 0, victim, 0);
        return ReturnValue::eFAILURE;
      }
    }
    else
    { /* Steal the item */
      if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)))
      {
        if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch))
        {
          move_obj(obj, ch);

          if (victim->isPlayer() || (ISSET(victim->mobdata->actflags, ACT_NICE_THIEF)))
            _exp = GET_OBJ_WEIGHT(obj) * 1000;
          else
            _exp = (GET_OBJ_WEIGHT(obj) * 1000);

          if (GET_POS(victim) <= position_t::SLEEPING || IS_AFFECTED(victim, AFF_PARALYSIS))
            _exp = {};

          ch->sendln("Got it!");
          if (_exp)
          {
            ch->exp += _exp; /* exp for stealing :) */
            dc_sprintf(buf, "You receive %d experience.\r\n", _exp);
            ch->send(buf);
          }

          if (victim->isPlayer())
          {
            victim->save(cmd_t::SAVE_SILENTLY);
            ch->save(cmd_t::SAVE_SILENTLY);
            if (!AWAKE(victim))
            {
              //              if(ch->dc_->number(1, 3) == 1)
              victim->sendln("You dream of someone stealing your equipment...");

              // if i'm not a thief, or if I fail dex-roll wake up victim
              //              if(GET_CLASS(ch) != CLASS_THIEF || dc_->number(1, 100) > GET_DEX(ch))
              //              {
              //                ch->sendln("Oops...");
              if ((paf = victim->affected_by_spell(SPELL_SLEEP)) && paf->modifier == 1)
              {
                paf->modifier = {}; // make sleep no longer work
              }
              ch->wake(victim);
              //              }
            }

            // if victim isn't a pthief
            //            if(!victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
            {
              // set_cantquit( ch, victim );
              if (ch->isPlayerObjectThief())
              {
                affect_from_char(ch, Character::PLAYER_OBJECT_THIEF);
                affect_to_char(ch, &pthiefaf);
              }
              else
                affect_to_char(ch, &pthiefaf);
            }
          }
          if (victim->isPlayer())
          {
            QString log_buf = {};
            dc_sprintf(log_buf, "%s stole %s[%lu] from %s", qPrintable(ch->name()), qPrintable(obj->short_description()), dc_->obj_index[obj->item_number].vnum(), qPrintable(victim->name()));
            dc_->logentry(log_buf, ANGEL, DC::LogChannel::LOG_MORTAL);
            for (loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content)
              dc_->logf(ANGEL, DC::LogChannel::LOG_MORTAL, "The %s contained %s[%d]", qPrintable(obj->short_description()), qPrintable(loop_obj->short_description()), dc_->obj_index[loop_obj->item_number].vnum());
          }
          if (dc_->obj_index[obj->item_number].vnum() != 76)
          {
            obj_from_char(obj);
            has_item = search_char_for_item(ch, obj->item_number, false);
            obj_to_char(obj, ch);
          }
          if (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
              (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) && has_item))
          {
            ch->send(u"Whoa!  The %1 poofed into thin air!\r\n"_s.arg(obj->short_description()));
            extract_obj(obj);
          }
          // check for no_trade inside containers
          else
            for (loop_obj = obj->contains; loop_obj; loop_obj = next_obj)
            {
              // this is 'else' since if the container was no_trade everything in it
              // has already been extracted
              next_obj = loop_obj->next_content;

              if (isSet(loop_obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
                  (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) && has_item))
              {
                ch->send(u"Whoa!  The %s inside the %s poofed into thin air!\r\n"_s.arg(qPrintable(loop_obj->short_description())).arg(qPrintable(obj->short_description())));
                extract_obj(loop_obj);
              }
            }
        }
        else
          ch->sendln("You cannot carry that much weight.");
      }
      else
        ch->sendln("You cannot carry that many items.");
    }
  }
  else // not in inventory
  {
    for (eq_pos = {}; (eq_pos < MAX_WEAR); eq_pos++)
    {
      if (victim->equipment[eq_pos] &&
          (isexact(obj_name, victim->equipment[eq_pos]->name())) && CAN_SEE_OBJ(ch, victim->equipment[eq_pos]))
      {
        obj = victim->equipment[eq_pos];
        break;
      }
    }

    if (obj)
    { // They're wearing it!
      /*    if (max_level(ch) < obj->obj_flags.eq_level)
          {
        ch->sendln("You find yourself unable to steal that.");
        return ReturnValue::eFAILURE;
          }*/

      chance -= GET_OBJ_WEIGHT(obj);
      if (GET_OBJ_WEIGHT(obj) > 50)
      {
        ch->sendln("That item is too heavy to steal.");
        return ReturnValue::eFAILURE;
      }

      qint32 wakey = 100;
      switch (eq_pos)
      {
      case WEAR_FINGER_R:
      case WEAR_FINGER_L:
      case WEAR_NECK_1:
      case WEAR_NECK_2:
      case WEAR_EAR_L:
      case WEAR_EAR_R:
      case WEAR_WRIST_R:
      case WEAR_WRIST_L:
        wakey = 30;
        break;
      case WEAR_HANDS:
      case WEAR_FEET:
      case WEAR_WAISTE:
      case WEAR_HEAD:
      case WEAR_WIELD:
      case WEAR_SECOND_WIELD:
      case WEAR_LIGHT:
      case WEAR_HOLD:
      case WEAR_HOLD2:
      case WEAR_SHIELD:
        wakey = 60;
        break;
      case WEAR_BODY:
      case WEAR_LEGS:
      case WEAR_ABOUT:
      case WEAR_FACE:
      case WEAR_ARMS:
        wakey = 90;
        break;
      default:
        ch->sendln("Something just screwed up. Tell an imm what you did.");
        return ReturnValue::eFAILURE;
      };
      wakey -= GET_DEX(ch) / 2;
      if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
      {
        ch->sendln("That item is protected by the gods.");
        return ReturnValue::eFAILURE;
      }
      if (!ch->has_skill(SKILL_STEAL))
      {
        ch->sendln("You don't know how to steal.");
        return ReturnValue::eFAILURE;
      }
      qint32 mod = ch->has_skill(SKILL_STEAL) - chance;

      if (GET_POS(victim) > position_t::SLEEPING ||
          GET_POS(victim) == position_t::STUNNED)
      {
        ch->sendln("Steal the equipment now? Impossible!");
        return ReturnValue::eFAILURE;
      }
      else if (!skill_success(ch, victim, SKILL_STEAL, 0 - mod))
      {
        set_cantquit(ch, victim);
        ohoh = true;
        ch->sendln("Oops, that was clumsy...");
        if ((paf = victim->affected_by_spell(SPELL_SLEEP)) && paf->modifier == 1)
        {
          paf->modifier = {}; // make sleep no longer work
        }

        ch->wake(victim);
        act_to_victim("$n tried to steal something from you, waking you up in the process.!", ch, 0, victim, 0);
        act_to_room("$n fails stealing something from $N, waking $N up in the process.", ch, 0, victim, INVIS_NULL | NOTVICT);
      }
      else if (!number(1, 4))
      {
        act_to_character("You remove $p and attempt to steal it.", ch, obj, 0, 0);
        ch->sendln("Your victim wakes up before you can complete the theft!");
        act_to_room("$n tries to steal $p from $N, but fails.", ch, obj, victim, NOTVICT);
        obj_to_char(victim->unequip_char(eq_pos), victim);
        act_to_victim("You awake to find $n removing some of your equipment.", ch, obj, victim, 0);
        victim->save(cmd_t::SAVE_SILENTLY);
        set_cantquit(ch, victim);
        if ((paf = victim->affected_by_spell(SPELL_SLEEP)) && paf->modifier == 1)
        {
          paf->modifier = {}; // make sleep no longer work
        }
        ch->wake(victim);
      }
      else
      {
        act_to_character("You remove $p and steal it.", ch, obj, 0, 0);
        act_to_room("$n steals $p from $N.", ch, obj, victim, NOTVICT);
        obj_to_char(victim->unequip_char(eq_pos), ch);
        if (victim->isPlayer() || (ISSET(victim->mobdata->actflags, ACT_NICE_THIEF)))
          _exp = GET_OBJ_WEIGHT(obj);
        else
          _exp = (GET_OBJ_WEIGHT(obj) * victim->getLevel());
        if (GET_POS(victim) <= position_t::SLEEPING)
          _exp = 1;
        ch->exp += _exp; /* exp for stealing :) */
        dc_sprintf(buf, "You receive %d exps.\r\n", _exp);
        ch->send(buf);
        dc_sprintf(buf, "%s stole %s from %s while victim was asleep",
                   qPrintable(ch->name()), qPrintable(obj->short_description()), qPrintable(victim->name()));
        dc_->logentry(buf, ANGEL, DC::LogChannel::LOG_MORTAL);
        if (!victim->isNonPlayer())
        {
          victim->save(cmd_t::SAVE_SILENTLY);
          ch->save(cmd_t::SAVE_SILENTLY);
          if (!AWAKE(victim))
          {
            //            if(ch->dc_->number(1, 3) == 1)
            victim->sendln("You dream of someone stealing your equipment...");

            // if i'm not a thief, or if I fail dex-roll wake up victim
            //            if(ch->dc_->number(1,101) > wakey)
            //            {
            //              ch->sendln("Oops, that was clumsy...");
            if ((paf = victim->affected_by_spell(SPELL_SLEEP)) && paf->modifier == 1)
            {
              paf->modifier = {}; // make sleep no longer work
            }
            ch->wake(victim);
            //            }
          }

          // You don't get a thief flag from stealing from a pthief
          //          if(!victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
          {
            if (ch->isPlayerObjectThief())
            {
              affect_from_char(ch, Character::PLAYER_OBJECT_THIEF);
              affect_to_char(ch, &pthiefaf);
            }
            else
              affect_to_char(ch, &pthiefaf);
          }
        } // !is_npc
        obj_from_char(obj);
        has_item = search_char_for_item(ch, obj->item_number, false);
        obj_to_char(obj, ch);

        if (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
            (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) && has_item))
        {
          ch->sendln("Whoa! It poofed into thin air!");
          extract_obj(obj);
        }
        else
          for (loop_obj = obj->contains; loop_obj; loop_obj = next_obj)
          {
            // this is 'else' since if the container was no_trade everything in it
            // has already been extracted
            next_obj = loop_obj->next_content;

            if (isSet(loop_obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
                (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) && has_item))
            {
              ch->send(u"Whoa! The %s inside the %s poofed into thin air!\r\n"_s.arg(qPrintable(loop_obj->short_description())).arg(qPrintable(obj->short_description())));
              extract_obj(loop_obj);
            }
          }
      } // else
    } // if(obj)
    else
    { // they don't got it
      act_to_character("$N does not seem to possess that item.", ch, 0, victim, 0);
      return ReturnValue::eFAILURE;
    }
  } // of else, not in inventory

  if (ohoh && victim->isNonPlayer() && AWAKE(victim) && ch->getLevel() < ANGEL)
  {
    if (ISSET(victim->mobdata->actflags, ACT_NICE_THIEF))
    {
      dc_sprintf(buf, "%s is a bloody thief.", qPrintable(ch->shortdesc_or_name()));
      do_shout(victim, buf);
    }
    else
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }
  }
  return ReturnValue::eSUCCESS;
}

// Steal gold
command_return_t do_pocket(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  affected_type pthiefaf;
  QString victim_name;
  QString buf;
  qint32 gold;
  qint32 _exp;
  qint32 retval;
  bool ohoh = false;

  one_argument(argument, victim_name);

  pthiefaf.type = Character::PLAYER_GOLD_THIEF;
  pthiefaf.duration = 6;
  pthiefaf.modifier = {};
  pthiefaf.location = APPLY_NONE;
  pthiefaf.bitvector = -1;

  if (!(victim = ch->get_char_room_vis(victim_name)))
  {
    ch->sendln("Steal what from who?");
    return ReturnValue::eFAILURE;
  }
  else if (victim == ch)
  {
    ch->sendln("Got it!\r\nYou receive 30000000000 experience.");
    return ReturnValue::eFAILURE;
  }

  if (GET_POS(victim) == position_t::DEAD)
  {
    ch->sendln("Don't steal from dead people.");
    return ReturnValue::eFAILURE;
  }

  if ((ch->getLevel() < (victim->getLevel() - 19)))
  {
    ch->sendln("That person is far too experienced to steal from.");
    return ReturnValue::eFAILURE;
  }

  if (victim->isPlayer() && ((victim->getLevel() + 20) < ch->getLevel()))
  {
    ch->sendln("That person is too low level, you don't want to tarnish your reputation!");
    return ReturnValue::eFAILURE;
  }

  if (isSet(dc_->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("No stealing permitted in safe areas!");
    return ReturnValue::eFAILURE;
  }

  if (check_make_camp(ch->in_room))
  {
    ch->sendln("You can't pocket $B$5gold$R while inside of a camp!");
    return ReturnValue::eFAILURE;
  }

  if (ch->room().isArena())
  {
    ch->sendln("Do what!? This is an Arena, go kill someone!");
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_CHARM))
  {
    return do_say(ch, "Nice try.");
  }

  if (victim->fighting)
  {
    ch->sendln("You can't get close enough because of the fight.");
    return ReturnValue::eFAILURE;
  }

  /*if(victim->isPlayer() &&
    !(victim->conn_) && !victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF) ){
    ch->sendln("That person is not really there.");
    return ReturnValue::eFAILURE;
  }
*/
  if (!ch->has_skill(SKILL_POCKET) && ch->isPlayer())
  {
    ch->sendln("Well, you would, if you knew how.");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_POCKET))
    return ReturnValue::eSUCCESS;

  WAIT_STATE(ch, 20); /* It takes TIME to steal */

  //    ch->skill_increase_check(SKILL_POCKET, ch->has_skill(SKILL_POCKET),SKILL_INCREASE_MEDIUM);

  if (!skill_success(ch, victim, SKILL_POCKET))
  {
    set_cantquit(ch, victim);
    ch->sendln("Oops, that was clumsy...");
    ohoh = true;
    if (ch->dc_->number(0, 6))
    {
      act_to_victim("You discover that $n has $s hands in your wallet.", ch, 0, victim, 0);
      act_to_room("$n tries to steal $B$5gold$R from $N.", ch, 0, victim, NOTVICT | INVIS_NULL);
      REMBIT(ch->affected_by, AFF_HIDE);
    }
    else
    {
      act_to_character("You manage to keep $N unaware of your botched attempt.", ch, 0, victim, 0);
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    qint32 learned = ch->has_skill(SKILL_POCKET);
    qint32 percent = 7 + (learned > 40) + (learned > 60) + (learned > 80);

    // Steal some gold coins
    gold = (qint32)((qreal)(victim->getGold()) * (qreal)((qreal)percent / 100.0));
    gold = MIN(10000000, gold);
    if (gold > 0)
    {
      ch->addGold(gold);
      victim->removeGold(gold);
      _exp = gold / 100 * victim->getLevel() / 5;
      if (victim->isPlayer())
        _exp = {};
      if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_NICE_THIEF))
        _exp = 1;
      if (GET_POS(victim) <= position_t::SLEEPING || IS_AFFECTED(victim, AFF_PARALYSIS))
        _exp = {};

      dc_sprintf(buf, "Nice work! You pilfered %d $B$5gold$R coins.\r\n", gold);
      ch->send(buf);
      if (_exp && _exp > 1)
      {
        ch->exp += _exp; /* exp for stealing :) */
        dc_sprintf(buf, "You receive %d experience.\r\n", _exp);
        ch->send(buf);
      }

      if (victim->isPlayer())
      {
        victim->save(cmd_t::SAVE_SILENTLY);
        ch->save(cmd_t::SAVE_SILENTLY);
        if (!victim->isPlayerGoldThief())
        {
          // set_cantquit( ch, victim );
          if (ch->isPlayerGoldThief())
          {
            affect_from_char(ch, Character::PLAYER_GOLD_THIEF);
            affect_to_char(ch, &pthiefaf);
          }
          else
            affect_to_char(ch, &pthiefaf);
        }
      }
      dc_->logf(0, DC::LogChannel::LOG_OBJECTS, "%s stole %d gold from %s in room %d", qPrintable(ch->name()), gold, qPrintable(victim->name()), GET_ROOM_VNUM(victim->in_room));
    }
    else
    {
      ch->sendln("You couldn't get any gold...");
    }
  }

  if (ohoh && victim->isNonPlayer() && AWAKE(victim) && ch->getLevel() < ANGEL)
  {
    if (ISSET(victim->mobdata->actflags, ACT_NICE_THIEF))
    {
      dc_sprintf(buf, "%s is a bloody thief.", qPrintable(ch->shortdesc_or_name()));
      do_shout(victim, buf);
    }
    else
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_pick(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 door, other_room, j;
  QString type, dir;
  RoomDirectionPtr back;
  ObjectPtr obj;
  CharacterPtr victim;
  bool has_lockpicks = false;

  argument_interpreter(argument, type, dir);

  if (!ch->has_skill(SKILL_PICK_LOCK))
  {
    ch->sendln("You don't know how to pick locks!");
    return ReturnValue::eFAILURE;
  }

  // for (obj = ch->carrying; obj; obj = obj->next_content)
  //    if (obj->obj_flags.type_flag == ITEM_LOCKPICK)
  //      has_lockpicks = true;

  for (j = {}; j < MAX_WEAR; j++)
    if (ch->equipment[j] && (ch->equipment[j]->obj_flags.type_flag == ITEM_LOCKPICK || dc_->obj_index[ch->equipment[j]->item_number].vnum() == 504))
      has_lockpicks = true;

  if (!has_lockpicks)
  {
    ch->sendln("But...you don't have a lockpick!");
    return ReturnValue::eFAILURE;
  }

  if (type.isEmpty())
  {
    ch->sendln("Pick what?");
  }
  else if (generic_find(argument, (FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM), ch, &victim, &obj, true))
  {
    // this is an object

    if (obj->obj_flags.type_flag != ITEM_CONTAINER)
      ch->sendln("That's not a container.");
    else if (!isSet(obj->obj_flags.value[1], CONT_CLOSED))
      ch->sendln("Silly, it's not even closed!");
    else if (obj->obj_flags.value[2] < 0)
      ch->sendln("Odd, you can't seem to find a keyhole.");
    else if (!isSet(obj->obj_flags.value[1], CONT_LOCKED))
      ch->sendln("Oh-ho! This thing is not even locked!");
    else if (isSet(obj->obj_flags.value[1], CONT_PICKPROOF))
      ch->sendln("The lock resists even your best attempts to pick it.");
    else
    {
      if (!charge_moves(ch, SKILL_PICK_LOCK))
        return ReturnValue::eSUCCESS;

      if (!skill_success(ch, nullptr, SKILL_PICK_LOCK))
      {
        ch->sendln("You failed to pick the lock.");
        WAIT_STATE(ch, DC::PULSE_VIOLENCE);
        return ReturnValue::eFAILURE;
      }

      REMOVE_BIT(obj->obj_flags.value[1], CONT_LOCKED);
      ch->sendln("*Click*");
      act_to_room("$n fiddles with $p.", ch, obj, 0, 0);
    }
  }
  else if ((door = find_door(ch, type, dir)) >= 0)
  {
    // this is a door

    if (!isSet(EXIT(ch, door)->exit_info, EX_ISDOOR))
    {
      ch->sendln("That's absurd.");
    }
    else if (!isSet(EXIT(ch, door)->exit_info, EX_CLOSED))
    {
      ch->sendln("You realize that the door is already open!");
    }
    else if (EXIT(ch, door)->key < 0)
    {
      ch->sendln("You can't seem to spot any lock to pick.");
    }
    else if (!isSet(EXIT(ch, door)->exit_info, EX_LOCKED))
    {
      ch->sendln("Oh...it wasn't locked at all.");
    }
    else if (isSet(EXIT(ch, door)->exit_info, EX_PICKPROOF))
    {
      ch->sendln("You seem to be unable to pick this lock.");
    }
    else
    {
      if (!charge_moves(ch, SKILL_PICK_LOCK))
      {
        return ReturnValue::eSUCCESS;
      }

      // ch->skill_increase_check(SKILL_PICK_LOCK, ch->has_skill(SKILL_PICK_LOCK), SKILL_INCREASE_MEDIUM);
      if (!skill_success(ch, nullptr, SKILL_PICK_LOCK))
      {
        ch->sendln("You failed to pick the lock.");
        WAIT_STATE(ch, DC::PULSE_VIOLENCE);

        return ReturnValue::eFAILURE;
      }

      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword)
      {
        act_to_room("$n skillfully picks the lock of the $F.", ch, 0, EXIT(ch, door)->keyword, 0);
      }
      else
      {
        act_to_room("$n picks the lock of the.", ch, 0, 0, INVIS_NULL);
      }

      ch->sendln("The lock quickly yields to your skills.");

      /* now for unlocking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != DC::NOWHERE)
      {
        if ((back = dc_->world[other_room].dir_option[rev_dir[door]]) != 0)
        {
          if (back->to_room == ch->in_room)
          {
            REMOVE_BIT(back->exit_info, EX_LOCKED);
          }
        }
      }

      QString door_keyword = u"door"_s;
      if (EXIT(ch, door)->keyword)
      {
        door_keyword = fname(EXIT(ch, door)->keyword);
      }

      ch->sendln(u"You open the %1."_s.arg(door_keyword));
      auto rc = ch->do_open({door_keyword, dir});
    }
  }
  else
  {
    ch->sendln("Pick what?");
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_slip(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString obj_name, vict_name[200], buf[200];
  QString arg;
  qint32 amount;
  CharacterPtr vict;
  ObjectPtr obj, *tmp_object, container;

  extern qint32 weight_in(ObjectPtr);

  if (!ch->isNonPlayer() && ch->isPlayerObjectThief())
  {
    ch->sendln("Your criminal acts prohibit this action.");
    return ReturnValue::eFAILURE;
  }
  if (!ch->has_skill(SKILL_SLIP))
  {
    ch->sendln("You don't know how to slip.");
    return ReturnValue::eFAILURE;
  }
  argument = one_argument(argument, obj_name);

  if (is_number(obj_name))
  {
    if (!ch->isNonPlayer() && ch->isPlayerGoldThief())
    {
      ch->sendln("Your criminal acts prohibit this action.");
      return ReturnValue::eFAILURE;
    }
    if (dc_strlen(obj_name) > 7)
    {
      ch->sendln("Number field too large.  Try something smaller.");
      return ReturnValue::eFAILURE;
    }

    amount = dc_atoi(obj_name);
    argument = one_argument(argument, arg);

    if (str_cmp("coins", arg) && str_cmp("coin", arg))
    {
      ch->sendln("Sorry, you can't do that (yet)...");
      return ReturnValue::eFAILURE;
    }
    if (amount <= 0)
    {
      ch->sendln("Sorry, you can't do that!");
      return ReturnValue::eFAILURE;
    }
    if ((ch->getGold() < (quint32)amount) && (ch->getLevel() < DEITY))
    {
      ch->sendln("You haven't got that many coins!");
      return ReturnValue::eFAILURE;
    }

    argument = one_argument(argument, vict_name);

    if (vict.isEmpty() _name)
    {
      ch->sendln("To who?");
      return ReturnValue::eFAILURE;
    }
    if (!(vict = ch->get_char_room_vis(vict_name)))
    {
      ch->sendln("To who?");
      return ReturnValue::eFAILURE;
    }
    if (ch == vict)
    {
      ch->sendln("To yourself?!  Very cute...");
      return ReturnValue::eFAILURE;
    }

    if (!charge_moves(ch, SKILL_SLIP))
      return ReturnValue::eSUCCESS;
    // Failure
    if (!skill_success(ch, vict, SKILL_SLIP))
    {
      ch->sendln("Whoops!  You dropped the coins!");
      if (ch->isImmortalPlayer())
      {
        special_log(QString(u"%1 tries to slip %2 coins to %3 and drops them in room %4!"_s.arg(ch->name()).arg(amount).arg(vict->name()).arg(ch->in_room));
      }

      act("$n tries to slip you some coins, but $e accidentally drops "
          "them.\r\n",
          ch, 0, vict, TO_VICT, 0);
      act("$n tries to slip $N some coins, but $e accidentally drops "
          "them.\r\n",
          ch, 0, vict, TO_ROOM, NOTVICT);

      if (ch->isNonPlayer() || (ch->getLevel() < DEITY))
        ch->removeGold(amount);

      tmp_object = create_money(amount);
      obj_to_room(tmp_object, ch->in_room);
      ch->save();
    } // failure

    // Success
    else
    {
      ch->send(u"You slip %d coins to %s.\r\n"_s.arg(amount).arg(qPrintable(vict->name())));

      if (ch->isImmortalPlayer())
      {
        special_log(QString(u"%1 slips %2 coins to %3 in room %4!"_s.arg(ch->name()).arg(amount).arg(vict->name()).arg(ch->in_room));
      }

      dc_sprintf(buf, "%s slips you %d $B$5gold$R coins.\r\n", PERS(ch, vict),
                 amount);
      act_to_victim(buf, ch, 0, vict, GODS);
      act_to_room("$n slips some $B$5gold$R to $N.", ch, 0, vict, GODS | NOTVICT);

      dc_sprintf(buf, "%s slips %d coins to %s", qPrintable(ch->name()), amount,
                 qPrintable(vict->name()));
      dc_->logentry(buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);

      if (ch->isNonPlayer() || (ch->getLevel() < DEITY))
        ch->removeGold(amount);

      vict->addGold(amount);

      // If a mob is given gold, we disable its ability to receive a gold bonus. This keeps
      // the mob from turning into an interest bearing savings account. :)
      if (vict->isNonPlayer())
      {
        SETBIT(vict->mobdata->actflags, ACT_NO_GOLD_BONUS);
      }

      ch->save();
      vict->save_char_obj();
    }

    return ReturnValue::eFAILURE;
  } // if (is_number)

  argument = one_argument(argument, vict_name);

  if (obj.isEmpty() _name || vict.isEmpty() _name)
  {
    ch->sendln("Slip what to who?");
    return ReturnValue::eFAILURE;
  }

  if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying)))
  {
    ch->sendln("You do not seem to have anything like that.");
    return ReturnValue::eFAILURE;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
  {
    ch->sendln("That sure would be a stupid thing to do.");
    return ReturnValue::eFAILURE;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE))
  {
    ch->sendln("You can't seem to get the item to leave you.");
    return ReturnValue::eFAILURE;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_NODROP))
  {
    if (ch->getLevel() < DEITY)
    {
      ch->sendln("You can't let go of it! Yeech!!");
      return ReturnValue::eFAILURE;
    }
    else
    {
      ch->sendln("This item is NODROP btw.");
    }
  }

  if (GET_ITEM_TYPE(obj) == ITEM_CONTAINER)
  {
    ch->sendln("That would ruin it!");
    return ReturnValue::eFAILURE;
  }

  // We're going to slip the item into our container instead
  if ((container = get_obj_in_list_vis(ch, vict_name, ch->carrying)))
  {
    if (GET_ITEM_TYPE(container) != ITEM_CONTAINER)
    {
      ch->sendln("That's not a container.");
      return ReturnValue::eFAILURE;
    }
    if (isSet(container->obj_flags.value[1], CONT_CLOSED))
    {
      ch->sendln("It seems to be closed.");
      return ReturnValue::eFAILURE;
    }
    if (((container->obj_flags.weight + obj->obj_flags.weight) >=
         container->obj_flags.value[0]) &&
        (dc_->obj_index[container->item_number].vnum() != 536 ||
         weight_in(container) + obj->obj_flags.weight >= 200))
    {
      ch->sendln("It won't fit...cheater.");
      return ReturnValue::eFAILURE;
    }
    if (!skill_success(ch, nullptr, SKILL_SLIP))
    { // fail
      act("$n tries to stealthily slip $p in $P, but you notice $s motions.", ch, obj,
          container, TO_ROOM, 0);
    }
    else
      act_to_room("$n slips $p in $P.", ch, obj, container, GODS);
    move_obj(obj, container);
    // fix weight (move_obj doesn't re-add it, but it removes it)
    if (dc_->obj_index[container->item_number].vnum() != 536)
      IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);

    act_to_character("You slip $p in $P.", ch, obj, container, 0);
    return ReturnValue::eSUCCESS;
  }
  if (!(vict = ch->get_char_room_vis(vict_name)))
  {
    ch->sendln("No one by that name around here.");
    return ReturnValue::eFAILURE;
  }

  if (vict->isNonPlayer() && dc_->mob_index[vict->mobdata->nr].non_combat_func == shop_keeper)
  {
    act_to_character("$N graciously refuses your gift.", ch, 0, vict, 0);
    return ReturnValue::eFAILURE;
  }

  if (ch == vict)
  {
    ch->sendln("To yourself?!  Very cute...");
    return ReturnValue::eFAILURE;
  }

  if (ch->isPlayerObjectThief() && !vict->conn_)
  {
    send_to_char("Now WHY would a thief slip something to a "
                 "linkdead character?\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  if ((1 + IS_CARRYING_N(vict)) > CAN_CARRY_N(vict))
  {
    act_to_character("$N seems to have $S hands full.", ch, 0, vict, 0);
    return ReturnValue::eFAILURE;
  }

  if (obj->obj_flags.weight + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
  {
    act_to_character("$E can't carry that much weight.", ch, 0, vict, 0);
    return ReturnValue::eFAILURE;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE))
  {
    if (search_char_for_item(vict, obj->item_number, false))
    {
      ch->sendln("The item's uniqueness prevents it!");
      return ReturnValue::eFAILURE;
    }
  }

  // ch->skill_increase_check(SKILL_SLIP, ch->has_skill(SKILL_SLIP), SKILL_INCREASE_EASY);

  if (!skill_success(ch, vict, SKILL_SLIP))
  {
    if (dc_->obj_index[obj->item_number].vnum() == 393)
    {
      ch->sendln("Whoa, you almost dropped your hot potato!");
      return ReturnValue::eFAILURE;
    }

    if (ch->isImmortalPlayer())
    {
      special_log(QString(u"%1 slips %2 to %3 and fumbles it in room %4!"_s.arg(ch->name()).arg(obj->short_description()).arg(vict->name()).arg(ch->in_room));
    }

    move_obj(obj, ch->in_room);

    act("$n tries to slip you something, but $e accidentally drops "
        "it.\r\n",
        ch, 0, vict, TO_VICT, 0);
    act("$n tries to slip $N something, but $e accidentally drops "
        "it.\r\n",
        ch, 0, vict, TO_ROOM, NOTVICT);
    ch->sendln("Whoops!  You dropped it.");
    ch->save();
  }

  // Success
  else
  {
    if (ch->isImmortalPlayer())
    {
      special_log(QString(u"%1 slips %2 to %3 in room %4."_s)).arg(ch->name()).arg(obj->short_description()).arg(vict->name()).arg(ch->in_room));
    }

    logobjects(u"%1 slips %2 to %3"_s.arg(qPrintable(ch->name())).arg(obj->name()).arg(qPrintable(vict->name())));

    move_obj(obj, vict);
    act_to_character("You slip $p to $N.", ch, obj, vict, 0);
    act_to_room("$n slips $p to $N.", ch, obj, vict, GODS | NOTVICT);
    act_to_victim("$n slips you $p.", ch, obj, vict, GODS);
    ch->save();
    vict->save_char_obj();
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_vitalstrike(CharacterPtr ch, QString argument, cmd_t cmd)
{
  affected_type af;

  if (ch->affected_by_spell(SKILL_VITAL_STRIKE) && !ch->isImmortalPlayer())
  {
    ch->sendln("Your body is still recovering from your last vitalstrike technique.");
    return ReturnValue::eFAILURE;
  }

  if (!ch->canPerform(SKILL_VITAL_STRIKE))
  {
    ch->sendln("You'd cut yourself to ribbons just trying!");
    return ReturnValue::eFAILURE;
  }

  if (!(ch->fighting))
  {
    ch->sendln("But you aren't fighting anyone!");
    return ReturnValue::eFAILURE;
  }

  // ch->skill_increase_check(SKILL_VITAL_STRIKE, ch->has_skill(SKILL_VITAL_STRIKE), SKILL_INCREASE_EASY);
  if (!charge_moves(ch, SKILL_VITAL_STRIKE))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_VITAL_STRIKE))
  {
    act_to_room("$n starts jabbing $s weapons around $mself and almost chops off $s pinkie finger.", ch, 0, 0, NOTVICT);
    ch->sendln("You try to begin the vital strike technique and nearly slice off your own pinkie finger!");
  }
  else
  {
    act_to_room("$n begins jabbing $s weapons with lethal accuracy and strength.", ch, 0, 0, NOTVICT);
    send_to_char("Your body begins to coil, the strength building inside of you, your mind\r\n"
                 "pinpointing vital and vulnerable areas....\r\n",
                 ch);
    SET_BIT(ch->combat, COMBAT_VITAL_STRIKE);
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  // learned should have max of 80 for mortal thieves
  // this means you can use it once per tick

  qint32 length = 9 - ch->has_skill(SKILL_VITAL_STRIKE) / 10;
  if (!isSet(ch->combat, COMBAT_VITAL_STRIKE))
    length /= 2;
  if (length < 1)
    length = 1;
  af.type = SKILL_VITAL_STRIKE;
  af.duration = length;
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  return ReturnValue::eSUCCESS;
}

command_return_t do_deceit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  affected_type af;

  if (!ch->canPerform(SKILL_DECEIT))
  {
    ch->sendln("You do not yet understand enough of the workings of your marks.");
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_DECEIT_TIMER))
  {
    ch->sendln("You have to wait to be more deceitful!");
    return ReturnValue::eFAILURE;
  }
  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You have no group to instruct!");
    return ReturnValue::eFAILURE;
  }

  qint32 grpsize = {};
  for (CharacterPtr tmp_char = dc_->world[ch->in_room].people_; tmp_char; tmp_char = tmp_char->next_in_room)
  {
    if (tmp_char == ch)
      continue;
    if (!ARE_GROUPED(ch, tmp_char))
      continue;
    grpsize++;
  }

  if (!charge_moves(ch, SKILL_DECEIT, grpsize))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, nullptr, SKILL_DECEIT))
  {
    ch->sendln("Your class just isn't up to the task.");
    act_to_room("$n tries to explain to you the weaknesses of others, but you do not understand.", ch, 0, 0, 0);
  }
  else
  {
    act_to_room("$n instructs $s group on the virtues of deceit.", ch, 0, 0, 0);
    ch->sendln("Your instruction is well received and your pupils are more able to exploit weaknesses.");

    af.type = SKILL_DECEIT_TIMER;
    af.duration = 1 + ch->has_skill(SKILL_DECEIT) / 10;
    af.modifier = {};
    af.location = {};
    af.bitvector = -1;
    affect_to_char(ch, &af);

    for (CharacterPtr tmp_char = dc_->world[ch->in_room].people_; tmp_char; tmp_char = tmp_char->next_in_room)
    {
      if (tmp_char == ch)
        continue;
      if (!ARE_GROUPED(ch, tmp_char))
        continue;

      affect_from_char(tmp_char, SKILL_DECEIT, SUPPRESS_MESSAGES);
      affect_from_char(tmp_char, SKILL_DECEIT, SUPPRESS_MESSAGES);
      act_to_victim("$n lures your mind into the thought patterns of the morally corrupt.", ch, 0, tmp_char, 0);

      af.type = SKILL_DECEIT;
      af.duration = 1 + ch->has_skill(SKILL_DECEIT) / 10;
      af.modifier = 1 + (ch->has_skill(SKILL_DECEIT) / 20);
      af.location = APPLY_MANA_REGEN;
      af.bitvector = -1;
      affect_to_char(tmp_char, &af);
      af.location = APPLY_DAMROLL;
      affect_to_char(tmp_char, &af);
      af.location = APPLY_HITROLL;
      affect_to_char(tmp_char, &af);
    }
  }

  // ch->skill_increase_check(SKILL_DECEIT, ch->has_skill(SKILL_DECEIT), SKILL_INCREASE_EASY);
  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  return ReturnValue::eSUCCESS;
}

command_return_t do_jab(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 retval = ReturnValue::eFAILURE, learned;

  if (ch->affected_by_spell(SKILL_JAB) && !ch->isImmortalPlayer())
  {
    ch->sendln("Your arm is still sore from your last attempt.");
    return ReturnValue::eFAILURE;
  }

  if (!(learned = ch->has_skill(SKILL_JAB)))
  {
    ch->sendln("You don't know how to jab.");
    return ReturnValue::eFAILURE;
  }

  if (!ch->equipment[WEAR_WIELD])
  {
    ch->sendln("Your must be wielding a weapon to make it a success.");
    return ReturnValue::eFAILURE;
  }

  QString arg;
  one_argument(argument, arg);
  CharacterPtr victim;

  if (arg.isEmpty() && ch->fighting)
    victim = ch->fighting;
  else
    victim = ch->get_char_room_vis(arg);

  if (!victim)
  {
    ch->sendln("Jab whom?");
    return ReturnValue::eFAILURE;
  }

  if (ch->in_room != victim->in_room)
  {
    ch->sendln("That person seems to have left.");
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Why are you trying to hit yourself on the head???.");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && (ISSET(victim->mobdata->actflags, ACT_HUGE) && learned < 81))
  {
    ch->sendln("You cannot jab someone that HUGE!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_SWARM) && learned < 81)
  {
    ch->sendln("You cannot target just one to jab!");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_TINY) && learned < 81)
  {
    ch->sendln("You cannot target someone that tiny to jab!");
    return ReturnValue::eFAILURE;
  }

  if ((ch->getLevel() < G_POWER) || ch->isNonPlayer())
  {
    if (!can_attack(ch) || !can_be_attacked(ch, victim))
      return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_JAB))
    return ReturnValue::eSUCCESS;

  set_cantquit(ch, victim);
  WAIT_STATE(ch, DC::PULSE_VIOLENCE);

  affected_type af;
  af.type = SKILL_JAB;
  af.duration = 2;
  af.duration_type = DC::PULSE_VIOLENCE;
  af.location = APPLY_NONE;
  af.bitvector = AFF_BLACKJACK;

  if (!skill_success(ch, victim, SKILL_JAB))
  {
    retval = damage(ch, victim, 0, TYPE_BLUDGEON, SKILL_JAB);
    return ReturnValue::eSUCCESS;
  }

  retval = damage(ch, victim, 100, TYPE_BLUDGEON, SKILL_JAB);

  // if there wasn't a failure and not immune to attack
  if (!(retval & ReturnValue::eFAILURE) && !(retval & ReturnValue::eIMMUNE_VICTIM))
  {
    // the victim didn't die then affect victim with jab effect
    if (!(retval & ReturnValue::eVICT_DIED))
    {
      if (ch->dc_->number(0, 1))
      {
        // victim's next target will be random
        af.modifier = 1;
        affect_to_char(victim, &af, DC::PULSE_VIOLENCE);
      }
      else
      {
        af.modifier = 2;
        victim->setSitting();
        SET_BIT(victim->combat, COMBAT_BASH1);
        affect_to_char(victim, &af, DC::PULSE_VIOLENCE);
        // victim's next attack will fail
      }

      af.location = APPLY_AC;
      af.modifier = learned * 1.5;
      affect_to_char(victim, &af, DC::PULSE_VIOLENCE);
    }

    // the character didn't die so affect it with jab wait effect
    if (!(retval & ReturnValue::eCH_DIED))
    {
      af.type = SKILL_JAB;
      af.duration = 1;
      af.modifier = {};
      af.location = APPLY_NONE;
      af.bitvector = -1;
      affect_to_char(ch, &af);
    }
  }

  // if the victim died and the character did not die
  if ((retval & ReturnValue::eVICT_DIED) && !(retval & ReturnValue::eCH_DIED))
  {
    if (ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_WIMPY))
      WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    return retval;
  }

  if ((retval & ReturnValue::eCH_DIED) || (retval & ReturnValue::eVICT_DIED))
  {
    return retval;
  }
  else
  {
    return ReturnValue::eSUCCESS;
  }
}

command_return_t do_appraise(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim = {};
  ObjectPtr obj = {};
  QString name = {}, buf = {};
  QString item = {};
  qint32 appraised = {}, bits = {}, learned = {};
  bool found = false, weight = false;

  argument = one_argument(argument, name);
  one_argument(argument, item);

  if (!(learned = ch->has_skill(SKILL_APPRAISE)))
  {
    ch->sendln("Your estimate would be baseless.");
    return ReturnValue::eFAILURE;
  }

  if (name[0] == '\0')
  {
    ch->sendln("Appraise whom?");
    return ReturnValue::eFAILURE;
  }

  bits = generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM,
                      ch, &victim, &obj, true);

  if (!bits)
  {
    ch->sendln("Appraise whom?");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_APPRAISE))
    return ReturnValue::eSUCCESS;

  if (obj)
  {
    appraised = obj->obj_flags.cost;
    found = true;
  }

  if (victim)
  {
    if (victim == ch && item.isEmpty())
    {
      ch->sendln("You're worth a million bucks, baby.");
      return ReturnValue::eFAILURE;
    }

    if (!item.isEmpty())
    {
      if (victim == ch)
      {
        obj = get_obj_in_list_vis(ch, item, ch->carrying);
        if (obj)
        {
          appraised = GET_OBJ_WEIGHT(obj);
          found = true;
          weight = true;
        }
        else
          ch->sendln("You don't seem to be carrying anything like that.");
      }
      else
      {
        obj = get_obj_in_list_vis(ch, item, victim->carrying);
        if (ch->dc_->number(0, 2) || !obj)
        {
          act_to_character("$N doesn't seem to be carrying anything like that.", ch, 0, victim, 0);
          return ReturnValue::eSUCCESS;
        }
        else
        {
          appraised = GET_OBJ_WEIGHT(obj);
          found = true;
          weight = true;
        }
      }
      if (ch->dc_->number(0, 1))
        appraised += 10 - learned / 10;
      appraised -= 10 - learned / 10;
    }

    if (!found)
      appraised = victim->getGold();
  }

  if (!weight)
  {
    if (!number(0, 1))
      appraised *= 1 + (100 - learned) / 100;
    else
      appraised *= 1 - (100 - learned) / 100;

    if (appraised < 1000)
    {
      appraised /= 100;
      appraised *= 100;
    }
    else if (appraised < 10000)
    {
      appraised /= 1000;
      appraised *= 1000;
    }
    else
    {
      appraised /= 10000;
      appraised *= 10000;
    }
  }

  if (appraised < 0)
    appraised = 1;

  if (!skill_success(ch, victim, SKILL_APPRAISE))
  {
    ch->sendln("You estimate a worth of 30000000000?!?");
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  }
  else
  {
    if (obj)
    {
      if (weight)
      {
        ch->sendln(QString(u"After some consideration, you estimate the weight of %1 to be %2."_s)).arg(GET_OBJ_SHORT(obj)).arg(appraised));
      }
      else if (found)
      {
        ch->sendln(QString(u"After some consideration, you estimate the value of %1 to be %2."_s)).arg(GET_OBJ_SHORT(obj)).arg(appraised));
      }
    }
    else if (victim)
    {
      ch->sendln(QString(u"After some consideration, you estimate the amount of $B$5gold$R %1 is carrying to be %2."_s.arg(victim->name()).arg(appraised));
    }
    WAIT_STATE(ch, (qint32)(DC::PULSE_VIOLENCE * 1.5));
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_cripple(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr vict;
  QString name;
  qint32 skill;

  one_argument(argument, name);

  if (!(skill = ch->has_skill(SKILL_CRIPPLE)))
  {
    ch->sendln("You don't know how to cripple anybody.");
    return ReturnValue::eFAILURE;
  }

  if (!(vict = ch->get_char_room_vis(name)) && !(vict = ch->fighting))
  {
    ch->sendln("Cripple whom?");
    return ReturnValue::eFAILURE;
  }

  if (vict == ch)
  {
    ch->sendln("You turn your head and grimace as you break your ankles with a sledgehammer.");
    return ReturnValue::eFAILURE;
  }

  if (vict->isNonPlayer() && ISSET(vict->mobdata->actflags, ACT_HUGE) && skill < 81)
  {
    ch->sendln("You cannot cripple someone that HUGE!");
    return ReturnValue::eFAILURE;
  }

  if (vict->isNonPlayer() && ISSET(vict->mobdata->actflags, ACT_SWARM) && skill < 81)
  {
    ch->sendln("You cannot pick just one to cripple!");
    return ReturnValue::eFAILURE;
  }

  if (vict->isNonPlayer() && ISSET(vict->mobdata->actflags, ACT_TINY) && skill < 81)
  {
    ch->sendln("You cannot target someone that tiny to cripple!");
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(vict, AFF_CRIPPLE))
  {
    act_to_character("$N has already been crippled!", ch, 0, vict, 0);
    return ReturnValue::eFAILURE;
  }

  if (!ch->isImmortalPlayer())
    if (!can_attack(ch) || !can_be_attacked(ch, vict))
      return ReturnValue::eFAILURE;

  if (!charge_moves(ch, SKILL_CRIPPLE))
    return ReturnValue::eSUCCESS;

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  // Make 'em fight eachother
  if (!vict->fighting)
    set_fighting(vict, ch);
  if (!ch->fighting)
    set_fighting(ch, vict);

  if (!skill_success(ch, vict, SKILL_CRIPPLE))
  {
    act_to_character("You quickly lash out but fail to cripple $N.", ch, 0, vict, 0);
    act_to_victim("$n quickly lashes out, narrowly missing an attempt to cripple you!", ch, 0, vict, 0);
    act_to_room("$n quickly lashes out but fails to cripple $N.", ch, 0, vict, NOTVICT);
  }
  else
  {
    if (vict->affected_by_spell(SKILL_BATTLESENSE) &&
        dc_->number(1, 100) < vict->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act_to_character("$N's heightened battlesense sees your strike coming from a mile away.", ch, 0, vict, 0);
      act_to_victim("Your heightened battlesense sees $n's strike coming from a mile away.", ch, 0, vict, 0);
      act_to_room("$N's heightened battlesense sees $n's strike coming from a mile away.", ch, 0, vict, NOTVICT);
    }
    else
    {
      act_to_character("You quickly lash out and strike a crippling blow to $N!", ch, 0, vict, 0);
      act_to_victim("$n lashes out quickly and cripples you with a painful blow!", ch, 0, vict, 0);
      act_to_room("$n quickly lashes out and strikes a crippling blow to $N!", ch, 0, vict, NOTVICT);
      affected_type af;
      af.type = SKILL_CRIPPLE;
      af.duration = skill / 20;
      af.duration_type = DC::PULSE_VIOLENCE;
      af.modifier = skill;
      af.location = APPLY_NONE;
      af.bitvector = AFF_CRIPPLE;
      affect_to_char(vict, &af, DC::PULSE_VIOLENCE);
    }
  }

  return ReturnValue::eSUCCESS;
}
