/************************************************************************
| cl_warrior.C
| Description:  This file declares implementation for (anti)paladin-specific
|   skills.
|
| File create with do_layhands -Pirahna 7/6/1999
*/
#include "DC/DC.h"

/************************************************************************
| OFFENSIVE commands.  These are commands that should require the
|   victim to retaliate.
*/

// Note that most of the (anti)paladin skills are already in "cl_warrior.C"

command_return_t do_harmtouch(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  // CharacterPtr tmp_ch;
  QString victim_name;
  affected_type af;
  qint32 retval = ReturnValue::eSUCCESS, dam;

  one_argument(argument, victim_name);

  if (!ch->canPerform(SKILL_HARM_TOUCH, "You dunno even HOW to harm touch.\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  if (!(victim = ch->get_char_room_vis(victim_name)))
  {
    victim = ch->fighting;
    if (!victim)
    {
      ch->sendln("Whom do you want to harmtouch?");
      return ReturnValue::eFAILURE;
    }
  }

  if (victim == ch)
  {
    if (GET_SEX(ch) == Character::sex_t::MALE)
      ch->sendln("You'd wither it!");
    else if (GET_SEX(ch) == Character::sex_t::FEMALE)
      ch->sendln("You naughty naughty girl...at least wait until someone's filming.");
    else
      ch->sendln("Looks like you've already harm touched yourself...");
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_HARM_TOUCH) && ch->getLevel() <= IMMORTAL)
  {
    ch->sendln("You have not spend enough time in devotion to your god to warrant such a favor yet.");
    return ReturnValue::eFAILURE;
  }

  if (ch->getHP() < GET_MAX_HIT(ch) / 4)
  {
    ch->sendln("You don't posess the energy to do it!");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_HARM_TOUCH))
    return ReturnValue::eSUCCESS;

  qint32 duration = 24;
  if (!skill_success(ch, victim, SKILL_HARM_TOUCH))
  {
    ch->sendln("Your god refuses you.");
    duration = 1;
    WAIT_STATE(ch, DC::PULSE_VIOLENCE / 2 + dc_->number((quint64)1, (quint64)DC::PULSE_VIOLENCE / 2));
  }
  else
  {
    dam = 750;
    retval = damage(ch, victim, dam, TYPE_ACID, SKILL_HARM_TOUCH);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    if (isSet(retval, ReturnValue::eVICT_DIED) && !isSet(retval, ReturnValue::eCH_DIED))
    {
      if (ch->has_skill(SKILL_HARM_TOUCH) > 30 && dc_->number(1, 3) == 1)
      {
        QString dammsg;
        qint32 amount = ch->getLevel() * 10;
        if (amount + ch->getHP() > GET_MAX_HIT(ch))
          amount = GET_MAX_HIT(ch) - ch->getHP();
        dc_sprintf(dammsg, "$B%d$R", amount);
        send_damage("Your god basks in your worship of pain and infuses you with | life.", ch, 0, victim, dammsg, "You god basks in your worship of pain and infuses you with life.", TO_CHAR);
        ch->addHP(amount);
      }
    }
  }
  af.type = SKILL_HARM_TOUCH;
  af.duration = duration;
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  return retval;
}

/************************************************************************
| NON-OFFENSIVE commands.  Below here are commands that should -not-
|   require the victim to retaliate.
*/

// Again note that alot of them are in cl_warrior.C

command_return_t do_layhands(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  // CharacterPtr tmp_ch;
  QString victim_name;
  affected_type af;
  qint32 duration = 24;
  one_argument(argument, victim_name);

  if (!ch->canPerform(SKILL_LAY_HANDS, "You aren't skilled enough to lay a two-dollar whore with three bucks.\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  if (!(victim = ch->get_char_room_vis(victim_name)))
  {
    ch->sendln("Whom do you want to layhands on?");
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Oh yeah...that's really holy....pervert...");
    return ReturnValue::eFAILURE;
  }

  //   if (ch->fighting == victim) {
  //     ch->sendln("Aren't you a little busy trying to KILL them right now?");
  //     return ReturnValue::eFAILURE;
  //   }

  if (ch->affected_by_spell(SKILL_LAY_HANDS))
  {
    ch->sendln("You have not spent enough time in devotion to your god to warrant such a favor yet.");
    return ReturnValue::eFAILURE;
  }

  if (ch->getHP() < GET_MAX_HIT(ch) / 4)
  {
    ch->sendln("You don't posess the energy to do it!");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_LAY_HANDS))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, victim, SKILL_LAY_HANDS))
  {
    ch->sendln("Your god refuses you.");
    duration = 1;
  }
  else
  {
    QString dammsg;
    qint32 amount = 500 + (ch->has_skill(SKILL_LAY_HANDS) * 10);
    if (amount + victim->getHP() > GET_MAX_HIT(victim))
      amount = GET_MAX_HIT(victim) - victim->getHP();
    victim->addHP(amount);
    dc_sprintf(dammsg, "$B%d$R", amount);
    send_damage("Praying fervently, you lay hands as life force granted by your god streams from your body healing $N for | health.",
                ch, 0, victim, dammsg, "Praying fervently, you lay hands as life force granted by your god streams from your body into $N.", TO_CHAR);
    send_damage("Your body surges with holy energies as | points of life force granted by $n's god pours into you!",
                ch, 0, victim, dammsg, "Your body surges with holy energies as life force granted by $n's god pours into you!", TO_VICT);
    send_damage("A blinding flash fills the area as | points of life force granted from $n's god pours into $N!", ch, 0,
                victim, dammsg, "A blinding flash fills the area as life force granted from $n's god pours into $N!", TO_ROOM);
  }

  af.type = SKILL_LAY_HANDS;
  af.duration = duration;
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  return ReturnValue::eSUCCESS;
}

command_return_t do_behead(CharacterPtr ch, QString argument, cmd_t cmd)
{
  double modifier = 0.0;
  double enemy_hp = 0.0;
  qint32 chance = {};
  qint32 retval = ReturnValue::eSUCCESS;
  QString buf;
  CharacterPtr vict;

  one_argument(argument, buf);

  if (!ch->canPerform(SKILL_BEHEAD, "The closest you'll ever get to 'beheading' is at a brit milah. Mazal tov!\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  if (!ch->equipment[WEAR_WIELD] || !isSet(ch->equipment[WEAR_WIELD]->flags_.extra_flags, ITEM_TWO_HANDED) || (ch->equipment[WEAR_WIELD]->flags_.value[3] != 3)) // TYPE_SLASH
  {
    ch->sendln("You need to be wielding a two handed sword to behead!");
    return ReturnValue::eFAILURE;
  }

  if (!(vict = ch->get_char_room_vis(buf)))
  {
    if (ch->fighting)
      vict = ch->fighting;
    else
    {
      ch->sendln("Whom do you want behead?");
      return ReturnValue::eFAILURE;
    }
  }

  if (!can_attack(ch) || !can_be_attacked(ch, vict))
    return ReturnValue::eFAILURE;

  if (isSet(vict->combat, COMBAT_BLADESHIELD1) || isSet(vict->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("You can't behead a bladeshielded opponent!");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BEHEAD))
    return ReturnValue::eSUCCESS;

  WAIT_STATE(ch, (qint32)(DC::PULSE_VIOLENCE));

  if (!skill_success(ch, vict, SKILL_BEHEAD))
  {
    ch->sendln("Your mighty swing goes wild!");
    act_to_victim("$n takes a mighty swing at your head, but it goes wild!", ch, 0, vict, 0);
    act_to_room("$n takes a mighty swing at $n's head, but it goes wild!", ch, 0, vict, NOTVICT);
    retval = one_hit(ch, vict, SKILL_BEHEAD, WEAR_WIELD);
    return retval;
  }

  qint32 skill_level = ch->has_skill(SKILL_BEHEAD);
  modifier = 50.0 + skill_level / 2.0 + GET_ALIGNMENT(ch) / 100.0;
  modifier /= 100.0; // range .15-1.0

  enemy_hp = (vict->getHP() * 100.0) / GET_MAX_HIT(vict);
  enemy_hp /= 100.0; // range 0-1;

  if (enemy_hp <= 0)
    enemy_hp = 0.01;

  chance = (qint32)(modifier / (enemy_hp * enemy_hp));

  if (enemy_hp < 0.3) // covered is 0.3
  {
    chance += (ch->has_skill(SKILL_TWO_HANDED_WEAPONS) / 6);
    // ch->send(u"BEHEAD chance increased by %d\r\n"_s.arg(ch->has_skill( SKILL_TWO_HANDED_WEAPONS) / 6));
  }
  else
    chance >>= 1; // halving the chance if less than covered (nerf)

  if (chance > 85)
    chance = 85;

  if (chance < 0)
    chance = {};

  // ch->send(u"behead chance: %d, enemy hp%: %f\r\n"_s.arg(chance).arg(enemy_hp));

  if ((ch->dc_->number(0, 99) < chance) && !isSet(vict->immune, ISR_SLASH) && !isSet(vict->immune, ISR_PHYSICAL))
  {
    if ((
            (vict->equipment[WEAR_NECK_1] && dc_->obj_index[vict->equipment[WEAR_NECK_1]->item_number].vnum() == 518) || (vict->equipment[WEAR_NECK_2] && dc_->obj_index[vict->equipment[WEAR_NECK_2]->item_number].vnum() == 518)) &&
        !number(0, 1))
    { // tarrasque's leash..
      act_to_character("You attempt to behead $N, but your sword bounces of $S neckwear.", ch, 0, vict, 0);
      act_to_room("$n attempts to behead $N, but fails.", ch, 0, vict, NOTVICT);
      act_to_victim("$n attempts to behead you, but cannot cut through your neckwear.", ch, 0, vict, 0);
      retval = damage(ch, vict, 0, TYPE_SLASH, SKILL_BEHEAD);
      return ReturnValue::eSUCCESS | retval;
    }

    if (IS_AFFECTED(vict, AFF_NO_BEHEAD))
    {
      act_to_character("$N deftly dodges your beheading attempt!", ch, 0, vict, 0);
      act_to_room("$N deftly dodges $n's attempt to behead $M!", ch, 0, vict, NOTVICT);
      act_to_victim("You deftly avoid $n's attempt to lop your head off!", ch, 0, vict, 0);
      retval = damage(ch, vict, 0, TYPE_SLASH, SKILL_BEHEAD);
      return ReturnValue::eSUCCESS | retval;
    }

    act_to_victim("You feel your life end as $n's sword SLICES YOUR HEAD OFF!", ch, 0, vict, 0);
    act_to_character("You SLICE $N's head CLEAN OFF $S body!", ch, 0, vict, 0);
    act_to_room("$n cleanly slices $N's head off $S body!", ch, 0, vict, NOTVICT);

    vict->setHP(-20);
    make_head(vict);
    group_gain(ch, vict);
    fight_kill(ch, vict, TYPE_CHOOSE, 0);
    return ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED; /* Zero means kill it! */
    // it died..
  }
  else
  { /* You MISS the fucker! */
    act_to_victim("You hear the SWOOSH sound of wind as $n's sword attempts to slice off your head!", ch, 0, vict, 0);
    act_to_character("You miss your attempt to behead $N.", ch, 0, vict, 0);
    act_to_room("$N jumps back as $n makes an attempt to BEHEAD $M!", ch, 0, vict, NOTVICT);
    retval = damage(ch, vict, 0, TYPE_SLASH, SKILL_BEHEAD);
  }

  return retval;
}
