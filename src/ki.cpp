/*
 * ki.c - implementation of ki usage
 * Morcallen 12/18
 *
 */
/* $Id: ki.cpp,v 1.94 2014/07/04 22:00:04 jhhudso Exp $ */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fmt/format.h>

#include "DC/ki.h"
#include "DC/room.h"
#include "DC/character.h"
#include "DC/spells.h" // tar_char..
#include "DC/utility.h"
#include "DC/player.h"
#include "DC/interp.h"
#include "DC/mobile.h"
#include "DC/fight.h"
#include "DC/handler.h"
#include "DC/connect.h"
#include "DC/act.h"
#include "DC/db.h"
#include "DC/returnvals.h"
#include <vector>
#include "DC/handler.h"

struct ki_info_type ki_info[] = {
    {/* 0 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 12,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, ki_blast,
     SKILL_INCREASE_HARD},

    {/* 1 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 12,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, ki_punch,
     SKILL_INCREASE_HARD},

    {/* 2 */
     3 * DC::PULSE_TIMER, position_t::STANDING, 5,
     TAR_IGNORE | TAR_CHAR_ROOM | TAR_SELF_ONLY, ki_sense,
     SKILL_INCREASE_MEDIUM},

    {/* 3 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 8,
     TAR_IGNORE, ki_storm, SKILL_INCREASE_HARD},

    {/* 4 */
     3 * DC::PULSE_TIMER, position_t::STANDING, 25,
     TAR_IGNORE | TAR_CHAR_ROOM | TAR_SELF_ONLY, ki_speed,
     SKILL_INCREASE_HARD},

    {/* 5 */
     3 * DC::PULSE_TIMER, position_t::RESTING, 8,
     TAR_IGNORE | TAR_CHAR_ROOM | TAR_SELF_ONLY, ki_purify,
     SKILL_INCREASE_MEDIUM},

    {/* 6 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 10,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT, ki_disrupt,
     SKILL_INCREASE_HARD},

    {/* 7 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 12,
     TAR_IGNORE, ki_stance, SKILL_INCREASE_EASY},

    {/* 8 */
     3 * DC::PULSE_TIMER, position_t::FIGHTING, 20,
     TAR_IGNORE, ki_agility, SKILL_INCREASE_MEDIUM},

    {/* 9 */
     3 * DC::PULSE_TIMER, position_t::RESTING, 15,
     TAR_IGNORE, ki_meditation, SKILL_INCREASE_HARD},

    {/* 10 */
     3 * DC::PULSE_TIMER, position_t::STANDING, 1,
     TAR_CHAR_ROOM | TAR_SELF_NONO, ki_transfer, SKILL_INCREASE_HARD}

};

const char *ki[] = {
    "blast",
    "punch",
    "sense",
    "storm",
    "speed",
    "purify",
    "disrupt",
    "stance",
    "agility",
    "meditation",
    "transfer",
    "\n"};

int16_t use_ki(Character *ch, int kn);
bool ARE_GROUPED(Character *sub, Character *obj);

int16_t use_ki(Character *ch, int kn)
{
  return (ki_info[kn].min_useski);
}

int do_ki(Character *ch, char *argument, int cmd)
{
  Character *tar_char = ch;
  char name[MAX_STRING_LENGTH];
  int qend, spl = -1;
  bool target_ok;
  int learned;

  if (ch->getLevel() < ARCHANGEL && GET_CLASS(ch) != CLASS_MONK)
  {
    ch->sendln("You are unable to control your ki in this way!");
    return eFAILURE;
  }
  /*
   if ((isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE)) && (ch->getLevel() < IMPLEMENTER)) {
   send_to_char("You feel at peace, calm, relaxed, one with yourself and "
   "the universe.\r\n", ch);
   return eFAILURE;
   }*/

  argument = skip_spaces(argument);

  if (!(*argument))
  {
    ch->sendln("Yes, but WHAT would you like to do?");
    return eFAILURE;
  }

  for (qend = 1; *(argument + qend) && (*(argument + qend) != ' '); qend++)
    *(argument + qend) = LOWER(*(argument + qend));

  spl = old_search_block(argument, 0, qend, ki, 0);
  spl--; /* ki goes from 0+ not 1+ like spells */

  if (spl < 0)
  {
    ch->sendln("You cannot harness that energy!");
    return eFAILURE;
  }

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) && (ch->getLevel() < IMPLEMENTER) && spl != KI_SENSE && spl != KI_SPEED && spl != KI_PURIFY && spl != KI_STANCE && spl != KI_AGILITY && spl != KI_MEDITATION)
  {
    send_to_char("You feel at peace, calm, relaxed, one with yourself and "
                 "the universe.\r\n",
                 ch);
    return eFAILURE;
  }

  learned = ch->has_skill((spl + KI_OFFSET));
  if (!learned)
  {
    ch->sendln("You do not know that ki power!");
    return eFAILURE;
  }

  if (ki_info[spl].ki_pointer)
  {
    if (ch->getPosition() < ki_info[spl].minimum_position || (spl == KI_MEDITATION && (ch->getPosition() == position_t::FIGHTING || ch->getPosition() <= position_t::SLEEPING)))
    {
      switch (ch->getPosition())
      {
      case position_t::SLEEPING:
        ch->sendln("You dream of wonderful ki powers.");
        break;
      case position_t::RESTING:
        send_to_char("You cannot harness that much energy while "
                     "resting!\n\r",
                     ch);
        break;
      case position_t::SITTING:
        ch->sendln("You can't do this sitting!");
        break;
      case position_t::FIGHTING:
        ch->sendln("This is a peaceful ki power.");
        break;
      default:
        ch->sendln("It seems like you're in a pretty bad shape!");
        break;
      }
      return eFAILURE;
    }
    argument += qend; /* Point to the space after the last ' */
    for (; *argument == ' '; argument++)
      ; /* skip spaces */

    /* Locate targets */
    target_ok = false;

    if (!isSet(ki_info[spl].targets, TAR_IGNORE))
    {
      argument = one_argument(argument, name);
      if (*name)
      {
        if (isSet(ki_info[spl].targets, TAR_CHAR_ROOM))
          if ((tar_char = ch->get_char_room_vis(name)) != nullptr)
            target_ok = true;

        if (!target_ok && isSet(ki_info[spl].targets, TAR_SELF_ONLY))
          if (str_cmp(GET_NAME(ch), name) == 0)
          {
            tar_char = ch;
            target_ok = true;
          } // of !target_ok
      } // of *name

      /* No argument was typed */
      else if (!*name)
      {
        if (isSet(ki_info[spl].targets, TAR_FIGHT_VICT))
          if (ch->fighting)
            if ((ch->fighting)->in_room == ch->in_room)
            {
              tar_char = ch->fighting;
              target_ok = true;
            }
        if (!target_ok && isSet(ki_info[spl].targets, TAR_SELF_ONLY))
        {
          tar_char = ch;
          target_ok = true;
        }
      } // of !*name

      else
        target_ok = false;
    }

    if (isSet(ki_info[spl].targets, TAR_IGNORE))
      target_ok = true;

    if (target_ok != true)
    {
      if (*name)
        ch->sendln("Nobody here by that name.");
      else
        /* No arguments were given */
        ch->sendln("Whom should the power be used upon?");
      return eFAILURE;
    }

    else if (target_ok)
    {
      if ((tar_char == ch) && isSet(ki_info[spl].targets, TAR_SELF_NONO))
      {
        ch->sendln("You cannot use this power on yourself.");
        return eFAILURE;
      }
      else if ((tar_char != ch) && isSet(ki_info[spl].targets, TAR_SELF_ONLY))
      {
        ch->sendln("You can only use this power upon yourself.");
        return eFAILURE;
      }
      else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tar_char))
      {
        ch->sendln("You are afraid that it might harm your master.");
        return eFAILURE;
      }
    }

    /* I put ths in to stop those crashes.  Morc: find your own bug ;)
     * -Sadus
     * This has hence been fixed. - Pir
     */
    if (!isSet(ki_info[spl].targets, TAR_IGNORE))
      if (!tar_char)
      {
        logentry(QStringLiteral("Dammit Morc, fix that null tar_char thing in ki"), IMPLEMENTER,
                 DC::LogChannel::LOG_BUG);
        send_to_char(
            "If you triggered this message, you almost crashed the\n\r"
            "game.  Tell a god what you did immediately.\r\n",
            ch);
        return eFAILURE;
      }

    /* crasher right here */
    if (isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_KI))
    {
      ch->sendln("You find yourself unable to focus your energy here.");
      return eFAILURE;
    }

    if (!isSet(ki_info[spl].targets, TAR_IGNORE))
      if (!can_attack(ch) || !can_be_attacked(ch, tar_char))
        return eFAILURE;

    if (ch->getLevel() < ARCHANGEL && GET_KI(ch) < use_ki(ch, spl))
    {
      ch->sendln("You do not have enough ki!");
      return eFAILURE;
    }

    WAIT_STATE(ch, ki_info[spl].beats);

    if ((ki_info[spl].ki_pointer == nullptr) && spl > 0)
      ch->sendln("Sorry, this power has not yet been implemented.");
    else
    {
      if (!skill_success(ch, tar_char,
                         spl + KI_OFFSET) &&
          !isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
      {
        ch->sendln("You lost your concentration!");
        GET_KI(ch) -= use_ki(ch, spl) / 2;
        WAIT_STATE(ch, ki_info[spl].beats / 2);

        return eSUCCESS;
      }

      if (!isSet(ki_info[spl].targets, TAR_IGNORE))
        if (!tar_char || (ch->in_room != tar_char->in_room))
        {
          ch->sendln("Whom should the power be used upon?");
          return eFAILURE;
        }

      /* Stop abusing your betters  */
      if (!isSet(ki_info[spl].targets, TAR_IGNORE))
        if (IS_PC(tar_char) && (ch->getLevel() > ARCHANGEL) && (tar_char->getLevel() > ch->getLevel()))
        {
          ch->sendln("That just might annoy them!");
          return eFAILURE;
        }

      /* Imps ignore safe flags  */
      if (!isSet(ki_info[spl].targets, TAR_IGNORE))
        if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) && IS_PC(ch) && (ch->getLevel() == IMPLEMENTER))
        {
          tar_char->sendln("There is no safe haven from an angry IMPLEMENTER!");
        }

      ch->sendln("Ok.");
      GET_KI(ch) -= use_ki(ch, spl);

      return ((*ki_info[spl].ki_pointer)(ch->getLevel(), ch, argument,
                                         tar_char));
    }
    return eFAILURE;
  }
  return eFAILURE;
}

void reduce_ki(Character *ch, int type)
{
  int amount = 0;

  amount += ch->getLevel() / type; /* the higher the response
                                    * the lower the divisor */

  amount -= dice(1, 10);
  if (amount < 0)
    amount = 0;
  GET_KI(ch) -= amount;
}

int Character::ki_gain_lookup(void)
{
  int gain;

  /* gain 1 - 7 depedant on level */
  gain = GET_CLASS(this) == CLASS_MONK ? (int)(this->max_ki * 0.04) : (int)(this->max_ki * 0.05); /*(this->getLevel() / 8) + 1;*/
  gain += this->ki_regen;

  // Normalize these so we dont underun the array below
  int norm_wis = MAX(0, GET_WIS(this));
  int norm_int = MAX(0, GET_INT(this));

  if (GET_CLASS(this) == CLASS_MONK)
  {
    gain += wis_app[norm_wis].ki_regen;
  }
  else if (GET_CLASS(this) == CLASS_BARD)
  {
    gain += int_app[norm_int].ki_regen;
  }

  gain += age().year / 25;

  if (isSet(DC::getInstance()->world[this->in_room].room_flags, SAFE) || check_make_camp(this->in_room))
    gain = (int)(gain * 1.25);

  int multiplyer = 1;
  switch (this->getPosition())
  {
  case position_t::SLEEPING:
    multiplyer = 3;
    break;
  case position_t::RESTING:
    multiplyer = 2;
    break;
  case position_t::SITTING:
    multiplyer = 2;
    break;
  default:
    multiplyer = 1;
    break;
  }
  gain *= multiplyer;

  return MAX(gain, 1);
}

int ki_blast(uint8_t level, Character *ch, char *arg, Character *vict)
{
  int success = 0;
  int exit = number(0, 5); /* Chooses an exit */

  extern char *dirswards[];
  char buf[200];

  if (!vict)
  {
    logentry(QStringLiteral("Serious problem in ki blast!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eINTERNAL_ERROR;
  }

  success += ch->getLevel();

  if (vict->weight < 50)
    success += 50;
  else if (vict->weight >= 50 && vict->weight < 120)
    success += 20;
  else if (vict->weight >= 200 && vict->weight < 255)
    success -= 10;
  else
    success -= 20; /* more than 300 pounds?! */

  if (number(1, 101) > success || vict->affected_by_spell(SPELL_IRON_ROOTS)) /* 101 is complete failure */
  {
    act("$n fails to blast $N!", ch, 0, vict, TO_ROOM, NOTVICT);
    act("You fail to blast $N!", ch, 0, vict, TO_CHAR, 0);
    act("$n finds that you are hard to blast!", ch, 0, vict, TO_VICT, 0);
    if (!vict->fighting && IS_NPC(vict))
      return attack(vict, ch, TYPE_UNDEFINED);
    return eSUCCESS;
  }

  if (CAN_GO(vict, exit) &&
      !isSet(DC::getInstance()->world[EXIT(vict, exit)->to_room].room_flags, IMP_ONLY) &&
      !isSet(DC::getInstance()->world[EXIT(vict, exit)->to_room].room_flags, NO_TRACK) &&
      (!IS_AFFECTED(vict, AFF_CHAMPION) || champion_can_go(EXIT(vict, exit)->to_room)) &&
      class_can_go(GET_CLASS(vict), EXIT(vict, exit)->to_room))
  {
    sprintf(buf, "$N is blasted out of the room %s by $n!", dirswards[exit]);
    act(buf, ch, 0, vict, TO_ROOM, NOTVICT);
    sprintf(buf, "You watch as $N goes flailing out of the room %s!", dirswards[exit]);
    act(buf, ch, 0, vict, TO_CHAR, 0);
    act("$n's vicious blast throws you out of the room!", ch, 0,
        vict, TO_VICT, 0);

    if (vict->fighting)
    {
      if (IS_NPC(vict))
      {
        vict->add_memory(GET_NAME(ch), 'h');
        remove_memory(vict, 'f');
      }
      Character *tmp;
      for (tmp = DC::getInstance()->world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
        if (tmp->fighting == vict)
          stop_fighting(tmp);
      stop_fighting(vict);
    }

    move_char(vict, (DC::getInstance()->world[(ch)->in_room].dir_option[exit])->to_room);
    vict->setSitting();
    SET_BIT(vict->combat, COMBAT_BASH2);
    return eSUCCESS;
  }
  else /* There is no exit there */
  {
    char buf[MAX_STRING_LENGTH], name[100];
    int prev = vict->getHP();

    strcpy(name, GET_SHORT(vict));
    int retval = damage(ch, vict, 100, TYPE_KI, KI_OFFSET + KI_BLAST, 0);
    vict->setSitting();
    SET_BIT(vict->combat, COMBAT_BASH2);
    if (!SOMEONE_DIED(retval) && !vict->fighting && IS_NPC(vict))
      return attack(vict, ch, TYPE_UNDEFINED);
    return retval;
  }
  /* still here?  It was unsuccessful */
  return eSUCCESS;
}

int ki_punch(uint8_t level, Character *ch, char *arg, Character *vict)
{
  if (!vict)
  {
    logf(ANGEL, DC::LogChannel::LOG_BUG, "Serious problem in ki punch!", ANGEL, DC::LogChannel::LOG_BUG);
    return eINTERNAL_ERROR;
  }

  set_cantquit(ch, vict);
  auto dam = number(500, 700);
  auto manadam = GET_MANA(vict) / 4;
  int retval = eFAILURE;

  manadam = MAX(150, manadam);
  manadam = MIN(750, manadam);
  if (vict->getHP() < 500000)
  {
    auto success_chance = (ch->getLevel() / 5) + (ch->has_skill(KI_OFFSET + KI_PUNCH) * 0.75) - (vict->getLevel() / 5);
    if (number(1, 101) < success_chance)

    {
      GET_MANA(vict) -= manadam;
      retval = damage(ch, vict, dam, TYPE_UNDEFINED, KI_OFFSET + KI_PUNCH, 0);
      return retval;
    }
    else
    {
      retval = damage(ch, vict, 0, TYPE_UNDEFINED, KI_OFFSET + KI_PUNCH, 0);

      ch->removeHP((1 / 8) * (GET_MAX_HIT(ch)));
      WAIT_STATE(ch, DC::PULSE_VIOLENCE);
      if (!vict->fighting)
        return attack(vict, ch, TYPE_UNDEFINED);
    }
  } // end of < 5000

  else
  {
    ch->sendln("Your opponent has too many hit points!");
    if (!vict->fighting)
      return attack(vict, ch, TYPE_UNDEFINED);
  }

  return eSUCCESS; // shouldn't get here
}

int ki_sense(uint8_t level, Character *ch, char *arg, Character *vict)
{
  struct affected_type af;
  if (IS_AFFECTED(ch, AFF_INFRARED))
    return eSUCCESS;
  if (ch->affected_by_spell(SPELL_INFRAVISION))
    return eSUCCESS;

  af.type = SPELL_INFRAVISION;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.duration = level;
  af.bitvector = AFF_INFRARED;
  affect_to_char(vict, &af);
  vict->sendln("You feel your sense become more acute.");

  return eSUCCESS;
}

int ki_storm(uint8_t level, Character *ch, char *arg, Character *vict)
{
  int dam;
  int retval;
  Character *tmp_victim, *temp;

  dam = number(135, 165);
  //  ch->sendln("Your wholeness of spirit purges the souls of those around you!");
  //  act("$n's eyes flash as $e pools the energy within $m!\n\rA burst of energy slams into you!\r\n",
  int32_t room = ch->in_room;
  for (tmp_victim = DC::getInstance()->world[ch->in_room].people; tmp_victim && tmp_victim != (Character *)0x95959595; tmp_victim = temp)
  {
    temp = tmp_victim->next_in_room;
    if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
        (!ARE_GROUPED(ch, tmp_victim)) && can_be_attacked(ch, tmp_victim))
    {
      retval = damage(ch, tmp_victim, dam, TYPE_KI,
                      KI_OFFSET + KI_STORM, 0);
      if (isSet(retval, eCH_DIED))
        return retval;
      act("A burst of energy slams into you!", ch, 0, 0, TO_ROOM, 0);
    } // else
    //		if (DC::getInstance()->world[ch->in_room].zone == DC::getInstance()->world[tmp_victim->in_room].zone)
    //	tmp_victim->sendln("A crackle of energy echos past you.");
  }
  int dir = number(0, 5), distance = number(1, 3), i;
  if (room > 0)
    for (i = 0; i < distance; i++)
    {
      if (!IS_EXIT(room, dir) || !IS_OPEN(room, dir))
        break;
      room = EXIT_TO(room, dir);
      if (room == DC::NOWHERE)
        break;
      for (tmp_victim = DC::getInstance()->world[room].people; tmp_victim; tmp_victim = tmp_victim->next_in_room)
        tmp_victim->sendln("A crackle of energy echoes past you.");
    }
  if (number(1, 4) == 4 && !ch->fighting)
  {
    char dammsg[MAX_STRING_LENGTH];
    sprintf(dammsg, "$B%d$R", dam);
    if (dam + ch->getHP() > GET_MAX_HIT(ch))
      dam = GET_MAX_HIT(ch) - ch->getHP();
    ch->addHP(dam);
    send_damage("The flash of energy surges within you for | life!", ch, 0, 0, dammsg, "The flash of energy surges within you!", TO_CHAR);
  }
  WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  return eSUCCESS;
}

int ki_speed(uint8_t level, Character *ch, char *arg, Character *vict)
{
  struct affected_type af;

  if (!vict)
  {
    logentry(QStringLiteral("Null victim sent to ki speed"), ANGEL, DC::LogChannel::LOG_BUG);
    return eINTERNAL_ERROR;
  }

  if (vict->affected_by_spell(SPELL_HASTE))
    return eSUCCESS;

  af.type = SPELL_HASTE;
  af.duration = ch->has_skill(KI_OFFSET + KI_SPEED) / 15;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_HASTE;

  affect_to_char(vict, &af);

  af.type = SPELL_HASTE;
  af.duration = ch->has_skill(KI_OFFSET + KI_SPEED) / 15;
  af.modifier = -ch->has_skill(KI_OFFSET + KI_SPEED) / 4;
  af.location = APPLY_AC;
  af.bitvector = -1;

  affect_to_char(vict, &af);

  vict->sendln("You feel a quickening in your limbs!");
  return eSUCCESS;
}

int ki_purify(uint8_t level, Character *ch, char *arg, Character *vict)
{
  if (!vict)
  {
    logentry(QStringLiteral("Null victim sent to ki purify"), ANGEL, DC::LogChannel::LOG_BUG);
    return eINTERNAL_ERROR;
  }
  if (!arg)
  {
    ch->send("You can only purify poison, blindness, alcohol or weaken.");
    return eFAILURE;
  }
  if (!str_cmp(arg, "poison"))
  {
    if (vict->affected_by_spell(SPELL_POISON))
      affect_from_char(vict, SPELL_POISON);
    else
    {
      ch->sendln("That taint is not present.");
      return eFAILURE;
    }
    ch->sendln("You purge the poison.");
  }
  else if (!str_cmp(arg, "blindness"))
  {
    if (vict->affected_by_spell(SPELL_BLINDNESS))
      affect_from_char(vict, SPELL_BLINDNESS);
    else
    {
      ch->sendln("That taint is not present.");
      return eFAILURE;
    }
    ch->sendln("You purge the blindness.");
  }
  else if (!str_cmp(arg, "weaken"))
  {
    if (vict->affected_by_spell(SPELL_WEAKEN))
      affect_from_char(vict, SPELL_WEAKEN);
    else
    {
      ch->sendln("That taint is not present.");
      return eFAILURE;
    }
    ch->sendln("You purge the poison.");
  }
  else if (!str_cmp(arg, "alcohol"))
  {
    if (GET_COND(ch, DRUNK) > 0)
      gain_condition(vict, DRUNK, -GET_COND(ch, DRUNK));
    else
    {
      ch->sendln("That taint is not present.");
      return eFAILURE;
    }
    ch->sendln("You purge the alcohol.");
  }
  else
  {
    ch->sendln("You cannot purge that.");
  }
  return eSUCCESS;
}

int ki_disrupt(uint8_t level, Character *ch, char *arg, Character *victim)
{
  if (!victim)
  {
    logentry(QStringLiteral("Serious problem in ki disrupt!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eINTERNAL_ERROR;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE);
  set_cantquit(ch, victim);

  bool disrupt_bingo = false;
  int success_chance = 0;
  int learned = ch->has_skill(KI_OFFSET + KI_DISRUPT);

  if (learned > 85)
  {
    level_diff_t level_difference = ch->getLevel() - victim->getLevel();

    if (level_difference >= 0)
    {
      success_chance = 6;
    }
    else if (level_difference >= -20)
    {
      success_chance = 5;
    }
    else if (level_difference >= -100)
    {
      success_chance = 4;
    }
    else
    {
      success_chance = 1;
    }

    if (success_chance >= number(1, 100))
    {
      disrupt_bingo = true;
    }
  }

  if (disrupt_bingo == true)
  {
    act("$n slams a bolt of focused ki energy into the flow of magic all around you!", ch, 0, victim, TO_VICT, 0);
    act("$n focuses a blast of ki to disrupt the flow of magic all around $N!", ch, 0, victim, TO_ROOM, 0);
    ch->sendln("You focus your ki to disrupt the flow of magic all around your opponent!");
  }
  else
  {
    act("$n slams a bolt of focused ki energy into the flow of magic around you!", ch, 0, victim, TO_VICT, 0);
    act("$n focuses a blast of ki to disrupt the flow of magic around $N!", ch, 0, victim, TO_ROOM, 0);
    ch->sendln("You focus your ki to disrupt the flow of magic around your opponent!");
  }

  if (ISSET(victim->affected_by, AFF_GOLEM))
  {
    ch->sendln("The golem seems to shrug off your ki disrupt attempt!");
    act("The golem seems to ignore $n's disrupting energy!", ch, 0, 0, TO_ROOM, 0);
    return eFAILURE;
  }
  if (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_NODISPEL))
  {
    act("$N seems to ignore $n's disrupting energy!", ch, 0, victim, TO_ROOM, 0);
    act("$N seems to ignore your disrupting energy!", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  int savebonus = 0;
  if (learned < 41)
  {
    savebonus = 35;
  }
  else if (learned < 61)
  {
    savebonus = 30;
  }
  else if (learned < 81)
  {
    savebonus = 25;
  }
  else
  {
    savebonus = 20;
  }

  // Players are easier to disrupt
  if (IS_PC(victim))
  {
    savebonus -= 10;
  }

  // Check if caster gets a bonus against this victim
  affected_type *af = victim->affected_by_spell(KI_DISRUPT + KI_OFFSET);
  if (af)
  {
    // We've KI_DISRUPTED the victim and failed before so we get a bonus
    if (af->caster == std::string(GET_NAME(ch)))
    {
      savebonus -= af->modifier;
    }
    else
    {
      // Some other caster's KI_DISRUPT was on the victim, removing it
      affect_from_char(victim, KI_DISRUPT + KI_OFFSET);
      af = 0;
    }
  }

  int retval = 0;

  if (number(1, 100) <= get_saves(victim, SAVE_TYPE_MAGIC) + savebonus && level != ch->getLevel() - 1)
  {
    // We've failed this time, so we'll make it easier for next time
    if (af)
    {
      // We've failed before
      af->modifier += 1 + (learned / 20);
    }
    else
    {
      // This is the first time we've failed
      affected_type newaf;
      newaf.type = KI_DISRUPT + KI_OFFSET;
      newaf.duration = -1;
      newaf.modifier = 1 + (learned / 20);
      newaf.location = APPLY_NONE;
      newaf.bitvector = -1;
      newaf.caster = std::string(GET_NAME(ch));

      affect_to_char(victim, &newaf);
    }

    act("$N resists your attempt to disrupt magic!", ch, nullptr, victim,
        TO_CHAR, 0);
    act("$N resists $n's attempt to disrupt magic!", ch, nullptr, victim, TO_ROOM,
        NOTVICT);
    act("You resist $n's attempt to disrupt magic!", ch, nullptr, victim, TO_VICT,
        0);

    if (IS_NPC(victim) && (!victim->fighting) &&
        ch->getPosition() > position_t::SLEEPING)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }

    return eFAILURE;
  }

  // We have success so if af is set then the victim had a ki_disupt
  // bonus set. We will remove it.
  if (af)
  {
    affect_from_char(victim, KI_DISRUPT + KI_OFFSET);
  }

  // Disrupt bingo chance
  if (disrupt_bingo)
  {
    if (victim->affected_by_spell(SPELL_SANCTUARY) ||
        IS_AFFECTED(victim, AFF_SANCTUARY))
    {
      affect_from_char(victim, SPELL_SANCTUARY);
      REMBIT(victim->affected_by, AFF_SANCTUARY);
      act("You don't feel so invulnerable anymore.", ch, 0, victim, TO_VICT, 0);
      act("The $B$7white glow$R around $n's body fades.", victim, 0, 0, TO_ROOM, 0);
    }
    if (victim->affected_by_spell(SPELL_PROTECT_FROM_EVIL))
    {
      affect_from_char(victim, SPELL_PROTECT_FROM_EVIL);
      act("Your protection from evil has been disrupted!", ch, 0, victim, TO_VICT, 0);
      act("The dark, $6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, TO_ROOM, 0);
    }

    if (victim->affected_by_spell(SPELL_HASTE))
    {
      affect_from_char(victim, SPELL_HASTE);
      act("Your magically enhanced speed has been disrupted!", ch, 0, victim, TO_VICT, 0);
      act("$n's actions slow to their normal speed.", victim, 0, 0, TO_ROOM, 0);
    }

    if (victim->affected_by_spell(SPELL_STONE_SHIELD))
    {
      affect_from_char(victim, SPELL_STONE_SHIELD);
      act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0, victim, TO_VICT, 0);
      act("The shield of stones swirling about $n's body fall to the ground!", victim, 0, 0, TO_ROOM, 0);
    }

    if (victim->affected_by_spell(SPELL_GREATER_STONE_SHIELD))
    {
      affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);
      act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0, victim, TO_VICT, 0);
      act("The shield of stones swirling about $n's body falls to the ground!", victim, 0, 0, TO_ROOM, 0);
    }

    if (IS_AFFECTED(victim, AFF_FROSTSHIELD))
    {
      REMBIT(victim->affected_by, AFF_FROSTSHIELD);
      act("Your shield of $B$3frost$R melts into nothing!.", ch, 0, victim, TO_VICT, 0);
      act("The $B$3frost$R encompassing $n's body melts away.", victim, 0, 0, TO_ROOM, 0);
    }

    if (victim->affected_by_spell(SPELL_LIGHTNING_SHIELD))
    {
      affect_from_char(victim, SPELL_LIGHTNING_SHIELD);
      act("Your crackling shield of $B$5electricity$R vanishes!", ch, 0, victim, TO_VICT, 0);
      act("The $B$5electricity$R crackling around $n's body fades away.", victim, 0, 0, TO_ROOM, 0);
    }

    if (victim->affected_by_spell(SPELL_FIRESHIELD) || IS_AFFECTED(victim, AFF_FIRESHIELD))
    {
      REMBIT(victim->affected_by, AFF_FIRESHIELD);
      affect_from_char(victim, SPELL_FIRESHIELD);
      act("Your $B$4flames$R have been extinguished!", ch, 0, victim, TO_VICT, 0);
      act("The $B$4flames$R encompassing $n's body are extinguished!", victim, 0, 0, TO_ROOM, 0);
    }
    if (victim->affected_by_spell(SPELL_ACID_SHIELD))
    {
      affect_from_char(victim, SPELL_ACID_SHIELD);
      act("Your shield of $B$2acid$R dissolves to nothing!", ch, 0, victim, TO_VICT, 0);
      act("The $B$2acid$R swirling about $n's body dissolves to nothing!", victim, 0, 0, TO_ROOM, 0);
    }
    if (victim->affected_by_spell(SPELL_PROTECT_FROM_GOOD))
    {
      affect_from_char(victim, SPELL_PROTECT_FROM_GOOD);
      act("Your protection from good has been disrupted!", ch, 0, victim, TO_VICT, 0);
      act("The light, $B$6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, TO_ROOM, 0);
    }

    if (IS_NPC(victim) && !victim->fighting)
    {
      retval = attack(victim, ch, 0);
      SWAP_CH_VICT(retval);
      return retval;
    }
  }

  // This section of code looks for specific spells or affects and
  // adds them to a list called aff_list. Then a random element of
  // the list will be chosen for removal. This ensures we pick a random
  // affect only out of those that the player is using.
  std::vector<affected_type> aff_list;

  // Since we're looking for either these 3 affects OR the spells that cause them
  // we're keeping a track of which is found so we don't mark them twice
  bool frostshieldFound = false, fireshieldFound = false, sanctuaryFound = false;

  for (affected_type *curr = victim->affected; curr; curr = curr->next)
  {
    switch (curr->type)
    {
    case SPELL_FROSTSHIELD:
      frostshieldFound = true;
      aff_list.push_back(*curr);
      break;
    case SPELL_FIRESHIELD:
      fireshieldFound = true;
      aff_list.push_back(*curr);
      break;
    case SPELL_SANCTUARY:
      sanctuaryFound = true;
      aff_list.push_back(*curr);
      break;
    case SPELL_PROTECT_FROM_EVIL:
    case SPELL_HASTE:
    case SPELL_STONE_SHIELD:
    case SPELL_GREATER_STONE_SHIELD:
    case SPELL_LIGHTNING_SHIELD:
    case SPELL_ACID_SHIELD:
    case SPELL_PROTECT_FROM_GOOD:
      aff_list.push_back(*curr);
      break;
    }
  }

  // For these 3 affects, if they weren't caused by a spell we'll
  // add them to our list as if they were a spell to be removed
  affected_type localaff;

  if (IS_AFFECTED(victim, AFF_FROSTSHIELD) && !frostshieldFound)
  {
    localaff.type = SPELL_FROSTSHIELD;
    aff_list.push_back(localaff);
  }

  if (IS_AFFECTED(victim, AFF_FIRESHIELD) && !fireshieldFound)
  {
    localaff.type = SPELL_FIRESHIELD;
    aff_list.push_back(localaff);
  }

  if (IS_AFFECTED(victim, AFF_SANCTUARY) && !sanctuaryFound)
  {
    localaff.type = SPELL_SANCTUARY;
    aff_list.push_back(localaff);
  }

  // Nothing applicable found to be removed
  if (aff_list.size() < 1)
  {
    return eFAILURE;
  }

  // Pick the lucky spell/affect to be removed
  uint64_t i = number((quint64)0, (quint64)aff_list.size() - 1);

  try
  {
    af = &aff_list.at(i);
  }
  catch (...)
  {
    return eFAILURE;
  }

  if (af->type == SPELL_SANCTUARY)
  {
    affect_from_char(victim, SPELL_SANCTUARY);
    REMBIT(victim->affected_by, AFF_SANCTUARY);
    act("You don't feel so invulnerable anymore.", ch, 0, victim, TO_VICT, 0);
    act("The $B$7white glow$R around $n's body fades.", victim, 0, 0, TO_ROOM, 0);
  }

  if (af->type == SPELL_PROTECT_FROM_EVIL)
  {
    affect_from_char(victim, SPELL_PROTECT_FROM_EVIL);
    act("Your protection from evil has been disrupted!", ch, 0, victim, TO_VICT, 0);
    act("The dark, $6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, TO_ROOM, 0);
  }

  if (af->type == SPELL_HASTE)
  {
    affect_from_char(victim, SPELL_HASTE);
    act("Your magically enhanced speed has been disrupted!", ch, 0, victim, TO_VICT, 0);
    act("$n's actions slow to their normal speed.", victim, 0, 0, TO_ROOM, 0);
  }

  if (af->type == SPELL_STONE_SHIELD)
  {
    affect_from_char(victim, SPELL_STONE_SHIELD);
    act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0, victim, TO_VICT, 0);
    act("The shield of stones swirling about $n's body fall to the ground!", victim, 0, 0, TO_ROOM, 0);
  }

  if (af->type == SPELL_GREATER_STONE_SHIELD)
  {
    affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);
    act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0, victim, TO_VICT, 0);
    act("The shield of stones swirling about $n's body falls to the ground!", victim, 0, 0, TO_ROOM, 0);
  }

  if (af->type == SPELL_FROSTSHIELD)
  {
    affect_from_char(victim, SPELL_FROSTSHIELD);
    REMBIT(victim->affected_by, AFF_FROSTSHIELD);
    act("Your shield of $B$3frost$R melts into nothing!.", ch, 0, victim, TO_VICT, 0);
    act("The $B$3frost$R encompassing $n's body melts away.", victim, 0, 0, TO_ROOM, 0);
  }

  if (af->type == SPELL_LIGHTNING_SHIELD)
  {
    affect_from_char(victim, SPELL_LIGHTNING_SHIELD);
    act("Your crackling shield of $B$5electricity$R vanishes!", ch, 0, victim, TO_VICT, 0);
    act("The $B$5electricity$R crackling around $n's body fades away.", victim, 0, 0, TO_ROOM, 0);
  }

  if (af->type == SPELL_FIRESHIELD)
  {
    REMBIT(victim->affected_by, AFF_FIRESHIELD);
    affect_from_char(victim, SPELL_FIRESHIELD);
    act("Your $B$4flames$R have been extinguished!", ch, 0, victim, TO_VICT, 0);
    act("The $B$4flames$R encompassing $n's body are extinguished!", victim, 0, 0, TO_ROOM, 0);
  }

  if (af->type == SPELL_ACID_SHIELD)
  {
    affect_from_char(victim, SPELL_ACID_SHIELD);
    act("Your shield of $B$2acid$R dissolves to nothing!", ch, 0, victim, TO_VICT, 0);
    act("The $B$2acid$R swirling about $n's body dissolves to nothing!", victim, 0, 0, TO_ROOM, 0);
  }

  if (af->type == SPELL_PROTECT_FROM_GOOD)
  {
    affect_from_char(victim, SPELL_PROTECT_FROM_GOOD);
    act("Your protection from good has been disrupted!", ch, 0, victim, TO_VICT, 0);
    act("The light, $B$6pulsing$R aura surrounding $n has been disrupted!", victim, 0, 0, TO_ROOM, 0);
  }

  if (IS_NPC(victim) && !victim->fighting)
  {
    retval = attack(victim, ch, 0);
    SWAP_CH_VICT(retval);
    return retval;
  }
  return eSUCCESS;
}

int ki_stance(uint8_t level, Character *ch, char *arg, Character *vict)
{
  struct affected_type af;

  if (ch->affected_by_spell(KI_STANCE + KI_OFFSET))
  {
    ch->sendln("You focus your ki to harden your stance, but your body is still recovering from last time...");
    return eFAILURE;
  }

  act("$n assumes a defensive stance and attempts to absorb the energies that surround $m.",
      ch, 0, vict, TO_ROOM, 0);
  ch->sendln("You take a defensive stance and try to aborb the energies seeking to harm you.");

  // chance of failure - can be meta'd past that point though
  if (number(1, 100) > (GET_DEX(ch) * 4))
  {
    ch->sendln("You accidently stub your toe and fall out of the defenseive stance.");
    return eSUCCESS;
  }

  SET_BIT(ch->combat, COMBAT_MONK_STANCE);

  af.type = KI_STANCE + KI_OFFSET;
  af.duration = 50 - ((ch->getLevel() / 5) * 2);
  af.modifier = 1;
  af.location = APPLY_NONE;
  af.bitvector = -1;

  affect_to_char(ch, &af);
  return eSUCCESS;
}

int ki_agility(uint8_t level, Character *ch, char *arg, Character *vict)
{
  int learned, chance, percent;
  struct affected_type af;

  if (IS_NPC(ch) || ch->getLevel() >= ARCHANGEL)
    learned = 75;
  else if (!(learned = ch->has_skill(KI_AGILITY + KI_OFFSET)))
  {
    ch->sendln("You aren't experienced enough to teach others graceful movement.");
    return eFAILURE;
  }

  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You have no group to instruct.");
    return eFAILURE;
  }

  learned = learned % 100;

  chance = 75;

  // 101% is a complete failure
  percent = number(1, 101);
  if (percent > chance)
  {
    ch->sendln("Hopefully none of them noticed you trip on that rock.");
    act("$n tries to show everyone how to be graceful and trips over a rock.", ch, 0, 0, TO_ROOM, 0);
  }
  else
  {
    ch->sendln("You instruct your party on more graceful movement.");
    act("$n holds a quick tai chi class.", ch, 0, 0, TO_ROOM, 0);

    for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
    {
      if (tmp_char == ch)
        continue;
      if (!ARE_GROUPED(ch, tmp_char))
        continue;
      affect_from_char(tmp_char, KI_AGILITY + KI_OFFSET);
      affect_from_char(tmp_char, KI_AGILITY + KI_OFFSET);
      act("$n's graceful movement inspires you to better form.", ch, 0, tmp_char, TO_VICT, 0);

      af.type = KI_AGILITY + KI_OFFSET;
      af.duration = 1 + learned / 10;
      af.modifier = 1;
      af.location = APPLY_MOVE_REGEN;
      af.bitvector = -1;
      affect_to_char(tmp_char, &af);
      af.modifier = -10 - learned / 4;
      af.location = APPLY_ARMOR;
      affect_to_char(tmp_char, &af);
    }
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
  return eSUCCESS;
}

int ki_meditation(uint8_t level, Character *ch, char *arg, Character *vict)
{
  int gain;

  if (IS_NPC(ch))
    return eFAILURE;

  act("You enter a brief meditative state and focus your ki to heal your injuries.", ch, 0, vict, TO_CHAR, 0);
  act("$n enters a brief meditative state and focuses $s ki to heal several wounds.", ch, 0, vict, TO_ROOM, 0);

  gain = ch->hit_gain(position_t::SLEEPING);

  ch->setHP(MIN(ch->getHP() + gain, hit_limit(ch)));

  return eSUCCESS;
}

int ki_transfer(uint8_t level, Character *ch, char *arg, Character *victim)
{
  char amt[MAX_STRING_LENGTH], type[MAX_STRING_LENGTH];
  int amount, temp = 0;
  struct affected_type af;

  argument_interpreter(arg, amt, type);
  // arg = one_argument(arg, amt);
  // arg = one_argument(arg, type);

  amount = atoi(amt);

  if (amount < 0)
  {
    ch->sendln("Trying to be a funny guy?");
    return eFAILURE;
  }

  if (amount > GET_KI(ch))
  {
    ch->sendln("You do not have that much energy to transfer.");
    return eFAILURE;
  }

  int learned = ch->has_skill(KI_TRANSFER + KI_OFFSET);

  if (victim->affected_by_spell(SPELL_KI_TRANS_TIMER))
  {
    act("$N cannot receive a transfer right now due to the stress $S mind has been recently been through.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if (ch->affected_by_spell(SPELL_KI_TRANS_TIMER))
    affect_from_char(ch, SPELL_KI_TRANS_TIMER);

  af.type = SPELL_KI_TRANS_TIMER;
  af.duration = 1;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;

  affect_to_char(ch, &af);

  if (type[0] == 'k')
  {
    GET_KI(ch) -= amount;
    temp = number(amount - amount / 10, amount + amount / 10); //+-10%
    temp = (temp * learned) / 100;
    GET_KI(victim) += temp;
    if (GET_KI(victim) > GET_MAX_KI(victim))
      GET_KI(victim) = GET_MAX_KI(victim);

    sprintf(amt, "%d", amount);
    send_damage("You focus intently, bonding briefly with $N's spirit, transferring | ki of your essence to $M.",
                ch, 0, victim, amt,
                "You focus intently, bonding briefly with $N's spirit,  transferring a portion of your essence to $M.", TO_CHAR);
    sprintf(amt, "%d", temp);
    send_damage("$n focuses intently, bonding briefly with your spirit, replenishing | ki of your essence with $s own.",
                ch, 0, victim, amt,
                "$n focuses intently, bonding briefly with your spirit, replenishing your essence with $s own.", TO_VICT);
    act("$n focuses intently upon $N as though briefly bonding with $S spirit.", ch, 0, victim, TO_ROOM, NOTVICT);
  }
  else if (type[0] == 'm')
  {
    GET_KI(ch) -= amount;

    int mana_per_ki = learned / 5;

    temp = mana_per_ki * amount;
    GET_MANA(victim) += temp;
    if (GET_MANA(victim) > GET_MAX_MANA(victim))
      GET_MANA(victim) = GET_MAX_MANA(victim);

    std::string buffer;
    buffer = fmt::format("You focus intently, bonding briefly with $N's spirit, transferring {} ki of your essence into {} mana for $M.", amount, temp);
    act(buffer.c_str(), ch, 0, victim, TO_CHAR, 0);

    buffer = fmt::format("$n focuses intently, bonding briefly with your spirit, replenishing {} mana with a portion of $s essence.", temp);
    act(buffer.c_str(), ch, 0, victim, TO_VICT, 0);

    act("$n focuses intently upon $N as though briefly bonding with $S spirit.", ch, 0, victim, TO_ROOM, NOTVICT);
  }
  else
  {
    ch->sendln("You do not know of that essense.");
    return eFAILURE;
  }

  return eSUCCESS;
}
