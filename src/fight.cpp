/**************************************************************************
 * Fight1.c written from the original by Morcallen 1/17/95                *
 * This contains all the fight starting mechanisms as well                *
 * as damage.                                                             *
 *                                                                        *
 **************************************************************************
 * Revision History                                                       *
 * 10/23/2003 Onager Added checks for ch == nullptr in raw_kill() to prevent *
 * crashes from non-(N)PC deaths                                          *
 * Changed raw_kill() to make imms immune to stat loss                    *
 * 11/09/2003 Onager Added noncombat_damage() to do noncombat-related     *
 * damage (such as falls, drowning) that may kill                         *
 * 11/24/2003 Onager Totally revamped group_gain(); subbed out a lot of   *
 * the code and revised exp calculations for soloers                      *
 * and groups.                                                            *
 * 12/01/2003 Onager Re-revised group_gain() to divide up mob exp among   *
 * groupies                                                               *
 * 12/08/2003 Onager Changed change_alignment() to a simpler algorithm    *
 * with smaller changes in alignment                                      *
 * 12/28/2003 Pirahna Changed do_fireshield() to check ch->immune instead *
 * of just race stuff                                                     *
 **************************************************************************
 * $Id: fight.cpp,v 1.571 2015/06/16 04:10:54 pirahna Exp $               *
 **************************************************************************/

#include "DC/DC.h"
// log
#include <cassert>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include "DC/comm.h"
#include "DC/fight.h"
#include "DC/race.h"
#include "DC/player.h" // log
#include "DC/spells.h" // weapon_spells
#include "DC/handler.h"
#include "DC/interp.h" // do_flee()
#include "DC/db.h"     // fread_string()
#include "DC/magic.h"  // weapon spells
#include "DC/act.h"
#include "DC/clan.h"
#include "DC/innate.h"
#include "DC/token.h"
#include "DC/DC.h"
#include "DC/const.h"
#include "DC/corpse.h"
#include "DC/stat.h"
#include "DC/obj.h"

#include "DC/punish.h"
#include "DC/utility.h"

constexpr auto MAX_CHAMP_DEATH_MESSAGE = 14;

CharacterPtr combat_list = {}, combat_next_dude = {};

const QStringList champ_death_messages =
    {
        "\r\n##Somewhere a village has been deprived of their idiot.\r\n",
        "\r\n##Don't feel bad %s. A lot of people have no talent!\r\n",
        "\r\n##If at first you don't succeed, failure may be your style.\r\n",
        "\r\n##%s just found the cure for stupidity. Death.\r\n",
        "\r\n##%s just succumbed to a fatal case of stupidity.\r\n",
        "\r\n##%s: About as useful as a windshield wiper on a goat's ass.\r\n",
        "\r\n##Proof of reincarnation. Nobody could be as stupid as %s in one lifetime.\r\n",
        "\r\n##%s: The poster child for birth control.\r\n",
        "\r\n##Someone... we're not saying who... but someone is an unmitigated moron.\r\n",
        "\r\n##%s wasn't ready.\r\n",
        "\r\n##EPIC FAIL\r\n",
        "\r\n##%s wants a do-over.\r\n",
        "\r\n##%s prays, 'THIS GAME SUCKS'\r\n",
        "\r\n##DarkCastle: Helping people like %s to die since 1991.\r\n"};

qint32 debug_retval(CharacterPtr ch, CharacterPtr victim, qint32 retval);

void do_champ_flag_death(CharacterPtr victim)
{
  QString buf;
  ObjectPtr obj;
  obj = get_obj_in_list_num(real_object(CHAMPION_ITEM), victim->carrying);
  if (obj)
  {
    obj_from_char(obj);
    obj_to_room(obj, CFLAG_HOME);
    dc_snprintf(buf, 200, champ_death_messages[number(0, MAX_CHAMP_DEATH_MESSAGE - 1)], qPrintable(victim->name()));
    send_info(buf);

    if (obj && qPrintable(obj->short_description()))
      send_info(u"##%1 has just died with %2, watch for it to reappear!\r\n"_s.arg(victim->name()).arg(obj->short_description()));
    else
      send_info(u"##%1 has just died with the Champion Flag, watch for it to reappear!\r\n"_s.arg(victim->name()));
  }
  else
  {
    dc_->logentry(u"Champion without the flag, no bueno amigo!"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
  }
}

bool someone_fighting(CharacterPtr ch)
{
  CharacterPtr vict;
  if (ch->fighting && ch->fighting->fighting == ch)
    return true;
  for (vict = dc_->world[ch->in_room].people; vict; vict = vict->next_in_room)
  {
    if (vict->fighting == ch)
      return true;
  }
  return false;
}

qint32 check_autojoiners(CharacterPtr ch, qint32 skill = 0)
{
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE; // irrelevant
  if (!ch->fighting)
    return ReturnValue::eFAILURE;
  if (ch->player && ch->player->unjoinable == true)
    return ReturnValue::eFAILURE;
  if (isSet(dc_->world[ch->in_room].room_flags, SAFE))
    return ReturnValue::eFAILURE;

  CharacterPtr tmp;
  for (tmp = dc_->world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if (tmp == ch || tmp == ch->fighting)
      continue;
    if (tmp->isNonPlayer())
      continue;
    if (tmp->fighting)
      continue;
    if (!tmp->desc)
      continue;
    if (GET_POS(tmp) != position_t::STANDING)
      continue;
    if (tmp->player == nullptr || tmp->player->joining.empty())
      continue;
    if (!isexact(qPrintable(ch->name()), tmp->player->joining))
      continue;
    if (IS_AFFECTED(tmp, AFF_INSANE))
      continue;
    if (skill && !skill_success(tmp, ch, SKILL_FASTJOIN))
      continue;
    qint32 retval = tmp->do_join(u"0.%1"_s.arg(ch->name()).split(' '));
    if (SOMEONE_DIED(retval))
      return retval;
  }
  return ReturnValue::eSUCCESS;
}

qint32 check_joincharmie(CharacterPtr ch, qint32 skill = 0)
{
  if (ch->isPlayer())
    return ReturnValue::eFAILURE; // irrelevant
  if (!ch->fighting)
    return ReturnValue::eFAILURE;

  CharacterPtr tmp = ch->master;
  if (!tmp)
    return ReturnValue::eFAILURE;
  if (tmp == ch || tmp == ch->fighting)
    return ReturnValue::eFAILURE;
  if (!tmp->desc)
    return ReturnValue::eFAILURE;
  if (tmp->isNonPlayer())
    return ReturnValue::eFAILURE;
  if (tmp->fighting)
    return ReturnValue::eFAILURE;
  if (GET_POS(tmp) != position_t::STANDING)
    return ReturnValue::eFAILURE;
  if (!tmp->player || tmp->player->joining.empty())
    return ReturnValue::eFAILURE;
  if (!isexact("follower", tmp->player->joining) &&
      !isexact("followers", tmp->player->joining))
    return ReturnValue::eFAILURE;
  if (skill && !skill_success(tmp, ch, SKILL_FASTJOIN))
    return ReturnValue::eFAILURE;
  return tmp->do_join({"follower"});
}

qint32 Character::check_charmiejoin(void)
{
  if (fighting || !master || master == this || !isStanding())
  {
    return ReturnValue::eFAILURE;
  }

  return do_join(u"0.%1"_s.arg(master->name()).split(' '));
}

qint32 check_charmiejoin(CharacterPtr ch)
{
  if (ch)
  {
    return ch->check_charmiejoin();
  }
  else
  {
    return ReturnValue::eFAILURE;
  }
}

void DC::perform_violence(void)
{
  // QString debug;
  CharacterPtr ch;
  bool is_mob = {};
  qint32 retval;
  static affected_type *af, *next_af_dude;
  follow_type *fol, *folnext;
  extern QStringList spell_wear_off_msg;

  if (!combat_list)
    return;

  for (ch = combat_list; ch; ch = combat_next_dude)
  {
    // if combat_next_dude gets killed, it get's updated in "stop_fighting"
    // pretty kludgy way to do it, but it work
    combat_next_dude = ch->next_fighting;

    if (!ch->fighting)
    {
      dc_->logentry(u"Error in perform_violence()!  Null ch->fighting!"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
      return;
    }

    // DEBUG CODE
    qint32 last_virt = -1;
    qint32 last_class = GET_CLASS(ch);
    if (ch->isNonPlayer())
      last_virt = dc_->mob_index[ch->mobdata->nr].vnum();
    // DEBUG CODE
    if (!ch->fighting)
      continue;

    if (ch->in_room != ch->fighting->in_room)
    { // Fix for the whacky fighting someone who's not here thing.
      stop_fighting(ch);
      continue;
    }
    qint32 retval = check_autojoiners(ch);
    if (SOMEONE_DIED(retval))
      continue;
    if (IS_AFFECTED(ch, AFF_CHARM))
      retval = check_joincharmie(ch);
    if (SOMEONE_DIED(retval))
      continue;
    if (ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_CHARMIEJOIN))
    {
      if (ch->followers)
      {
        for (fol = ch->followers; fol; fol = folnext)
        {
          folnext = fol->next;
          if (IS_AFFECTED(fol->follower, AFF_CHARM) && ch->in_room == fol->follower->in_room)
            retval = check_charmiejoin(fol->follower);
          if (isSet(retval, ReturnValue::eVICT_DIED))
            break;
        }
      }
    }
    if (SOMEONE_DIED(retval))
      continue;

    if (!ch->fighting || ch->fighting->in_room != ch->in_room)
    {
      continue;
    }

    if (can_attack(ch))
    {
      is_mob = ch->isNonPlayer();
      if (is_mob)
      {
        if ((dc_->mob_index[ch->mobdata->nr].combat_func) && MOB_WAIT_STATE(ch) <= 0)
        {
          retval = ((*dc_->mob_index[ch->mobdata->nr].combat_func)(ch, nullptr, cmd_t::UNDEFINED, "", ch));
          if (SOMEONE_DIED(retval))
            continue;
          // Check if we're still fighting someone
          if (ch->fighting == 0)
            continue;
        }
        else if (!IS_AFFECTED(ch, AFF_CHARM))
        {
          if (MOB_WAIT_STATE(ch) > 0)
          {
            // dc_sprintf(debug, "DEBUG: Mob: %s (Lag: %d)", qPrintable(ch->shortdesc_or_name()), MOB_WAIT_STATE(ch));
            // MOB_WAIT_STATE(ch) -= DC::PULSE_VIOLENCE; // MIKE
            // dc_->logentry(debug, OVERSEER, DC::LogChannel::LOG_BUG);
          }
          else
          {
            ;
          }
        }
      } // if is_mob

      // DEBUG CODE
      if (last_class != GET_CLASS(ch))
      {
        // if this happened, most likely the mob died somehow during the proc and didn't return ReturnValue::eCH_DIED and is
        // now invalid memory.  report what class we were and return
        dc_->logf(IMPLEMENTER, DC::LogChannel::LOG_BUG, "Crash bug!!!!  fight.cpp last_class changed (%d) Mob=%d", last_class, last_virt);
        break;
      }
      // DEBUG CODE

      retval = attack(ch, ch->fighting, TYPE_UNDEFINED);

      if (SOMEONE_DIED(retval)) // no point in going anymore
        continue;

      // MOB Progs
      retval = mprog_hitprcnt_trigger(ch, ch->fighting);
      if (SOMEONE_DIED(retval) || !ch || !(ch->fighting) || ch->isDead() || ch->isNowhere() || ch->fighting->isDead() || ch->fighting->isNowhere())
      {
        continue;
      }

      retval = mprog_fight_trigger(ch, ch->fighting);
      if (SOMEONE_DIED(retval) || !ch || !(ch->fighting) || ch->isDead() || ch->isNowhere() || ch->fighting->isDead() || ch->fighting->isNowhere())
      {
        continue;
      }

    } // can_attack
    else if (!is_stunned(ch) && !IS_AFFECTED(ch, AFF_PARALYSIS))
      stop_fighting(ch);

    // This takes care of flee and stuff
    if (ch && ch->fighting)
      if (ch->fighting != (CharacterPtr)0x95959595 && ch->in_room != (ch->fighting)->in_room)
        stop_fighting(ch);
  } // for
  //

  // new round thing, so that how long things lasts doesn't depend
  // on who attacked who first
  for (ch = combat_list; ch; ch = combat_next_dude)
  {
    combat_next_dude = ch->next_fighting;

    if (!ch->fighting)
    {
      dc_->logentry(u"Error in perform_violence(), part2!  Null ch->fighting!"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
      return;
    }
    bool over = false;
    for (af = ch->affected; af; af = next_af_dude)
    {
      if (af == (affected_type *)0x95959595)
      {
        over = true;
        break;
      }
      next_af_dude = af->next;
      if (af->type == SPELL_POISON && af->location == APPLY_NONE)
      {
        qint32 dam = af->duration * 10 + dc_->number(30, 50);
        if (get_saves(ch, SAVE_TYPE_POISON) > dc_->number(1, 100))
        {
          dam = dam * get_saves(ch, SAVE_TYPE_POISON) / 100;
          ch->sendln("You feel very sick, but resist the $2poison's$R damage.");
        }
        if (dam)
        {
          QString dammsg;
          dc_sprintf(dammsg, "$B%d$R", dam);
          send_damage("You feel burning $2poison$R in your blood and suffer painful convulsions for | damage.", ch,
                      0, 0, dammsg, "You feel burning $2poison$R in your blood and suffer painful convulsions.", TO_CHAR);
          send_damage("$n looks extremely sick and shivers uncomfortably from the $2poison$R in $s veins that did | damage.", ch,
                      0, 0, dammsg, "$n looks extremely sick and shivers uncomfortably from the $2poison$R in $s veins.", TO_ROOM);
          retval = noncombat_damage(ch, dam,
                                    "You quiver from the effects of the poison and have no enegry left...",
                                    "$n stops struggling as $e is consumed by poison.",
                                    0, KILL_POISON);
          if (SOMEONE_DIED(retval))
          {
            over = true;
            break;
          }
        }
      }
      else if (af->type == SPELL_ATTRITION)
      {
        ch->sendln("Your body aches at the effort of combat.");
        if (ch->affected_by_spell(SPELL_DIVINE_INTER))
        {
          ch->removeHP(ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier);
        }
        else
        {
          ch->removeHP(25);
        }
        ch->setHP(MAX(1, ch->getHP())); // doesn't kill only hurts
      }
      // primal checks bitvecotr instead of type because the timer has type SKILL_PRIMAL_FURY..
      else if ((af->type != SPELL_PARALYZE && af->bitvector != AFF_PRIMAL_FURY) || !someone_fighting(ch))
      {
        continue;
      }

      if (af->duration >= 1)
      {
        af->duration--;
      }

      if (af->duration <= 0)
      {
        if (af->type < MAX_SPL_LIST && *spell_wear_off_msg[af->type])
        {
          send_to_char(spell_wear_off_msg[af->type], ch);
          ch->sendln("");
        }

        while (next_af_dude && af->type == next_af_dude->type)
        {
          next_af_dude = next_af_dude->next;
        }

        affect_remove(ch, af, 0);
      }
    }
    if (over)
      continue;
    update_flags(ch);
    if (is_stunned(ch) || IS_AFFECTED(ch, AFF_PARALYSIS))
      update_stuns(ch);
  }
}

void add_threat(CharacterPtr mob, CharacterPtr ch, qint32 amt)
{
  if (!mob || !ch || !amt || mob->isPlayer() || !qPrintable(ch->name()))
    return;
  for (auto thr = mob->mobdata->threat; thr; thr = thr->next)
  {
    if (ch->name() == thr->name_)
    { // Already angry with player.
      thr->threat += amt;
      return;
    }
  }
  auto thr = new threat_data;
  thr->next = mob->mobdata->threat;
  thr->threat = amt;
  thr->name_ = ch->name();
  mob->mobdata->threat = thr;
  // Threat added.
}

// could use bits for all that, but I dun' wanna.
constexpr auto DAMAGE = 1;
constexpr auto HEALING = 2;
constexpr auto AREA_DAM = 3;
constexpr auto AREA_HEAL = 4;

void generate_skillthreat(CharacterPtr mob, qint32 skill, qint32 damage, CharacterPtr actor, CharacterPtr target)
{
  if (!actor || !mob || mob->isPlayer())
    return;
  threat_data *thr;
  qreal v = (qreal)actor->has_skill(skill) / 100.0;
  if (!v)
    v = 0.4; // like weapons
  qint32 type = {};
  qint32 threat = {};
  switch (skill)
  {
  case SPELL_HELLSTREAM:
    threat = 100;
    type = DAMAGE;
    break;
  };
  if (!threat)
  { // Nothing set. Bugger.
    dc_->logf(110, DC::LogChannel::LOG_BUG, "Skill/spell %s(%d) missing threatsetting.", qPrintable(get_skill_name(skill)), skill);
    return;
  }
  threat = (qint32)(threat * v); // vary depending on skill

  if (type == DAMAGE)
  {
    if (!target)
      return; // damage spell without a target? right.

    if (target != mob && target != actor)
    {
      // damaging yerself gets you no coochie coochie
      for (thr = mob->mobdata->threat; thr; thr = thr->next)
        if (thr->name_ == target->name()))
          threat = 0 - threat; // this guy deserves mucho love, let's provide.
      // it damaged something else, get pissed off if friendly flagged
      if (threat > 0 && !ISSET(mob->mobdata->actflags, ACT_FRIENDLY))
        return;
    }
    add_threat(mob, actor, threat);
  }
}

bool gets_dual_wield_attack(CharacterPtr ch)
{
  if (!ch->equipment[WEAR_SECOND_WIELD]) // only if we have a second wield:)
    return false;

  if (!ch->has_skill(SKILL_DUAL_WIELD))
    return false;

  if (!skill_success(ch, nullptr, SKILL_DUAL_WIELD, 15))
    return false;

  return true;
}

// qint32 attack(...) FUNCTION SHOULD BE CALLED *INSTEAD* OF HIT IN ALL CASES!
// standard retvals
qint32 attack(CharacterPtr ch, CharacterPtr vict, qint32 type, qint32 weapon)
{
  qint32 result = {};
  qint32 chance;
  ObjectPtr wielded = {};
  qint32 handle_any_guard(CharacterPtr ch);

  if (!ch || !vict)
  {
    dc_->logentry(u"nullptr Victim or Ch sent to attack!  This crashes us!"_s, -1, DC::LogChannel::LOG_BUG);
    produce_coredump();

    return ReturnValue::eINTERNAL_ERROR;
  }

  if (GET_POS(ch) == position_t::DEAD)
  {
    dc_->logentry(u"Dead ch sent to attack. Wtf ;)"_s, -1, DC::LogChannel::LOG_BUG);
    produce_coredump();
    stop_fighting(ch);

    return ReturnValue::eINTERNAL_ERROR;
  }

  // victim could be dead if a skill like do_ki causes folowers to autojoin and kill
  // before attack gets called
  if (GET_POS(vict) == position_t::DEAD)
  {
    stop_fighting(ch);
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(ch))
    return ReturnValue::eFAILURE;
  if (!can_be_attacked(ch, vict))
    return ReturnValue::eFAILURE;

  // TODO - until I can make sure that area effects don't attack other mobs
  // when cast by mobs, I need to make sure mobs aren't killing each other
  if (ch->isNonPlayer() && vict->isNonPlayer() &&
      !IS_AFFECTED(ch, AFF_CHARM) && !IS_AFFECTED(ch, AFF_FAMILIAR) &&
      !IS_AFFECTED(vict, AFF_CHARM) && !IS_AFFECTED(vict, AFF_FAMILIAR) && !ch->desc && !vict->desc)
  {
    if (ch->fighting == vict)
    {
      stop_fighting(ch);
      remove_memory(ch, 'h');
      remove_memory(ch, 't');
    }
    if (vict->fighting == ch)
    {
      remove_memory(vict, 'h');
      remove_memory(vict, 't');
      stop_fighting(vict);
    }
    do_say(ch, QString("I'm sorry my fellow mob, I have seen the error of my ways."));
    do_say(vict, QString("It is okay my friend, let's go have a beer."));
    return ReturnValue::eFAILURE;
  }

  set_cantquit(ch, vict); // This sets the flag if necessary

  if (!vict->fighting && vict->in_room == ch->in_room)
    set_fighting(vict, ch); // So attacker starts round #2.
  else if (vict->in_room == ch->in_room)
    set_fighting(ch, vict);
  wielded = ch->equipment[WEAR_WIELD];

  if (type != SKILL_BACKSTAB)
    if (handle_any_guard(vict))
    {
      if ((vict = ch->fighting) == nullptr)
        return ReturnValue::eFAILURE;
    }
  assert(vict);

  if (ch->has_skill(SKILL_NAT_SELECT) &&
      ch->affected_by_spell(SKILL_NAT_SELECT) &&
      ch->affected_by_spell(SKILL_NAT_SELECT)->modifier == vict->race &&
      dc_->number(0, 3) == 0)
  {
    ch->skill_increase_check(SKILL_NAT_SELECT, ch->has_skill(SKILL_NAT_SELECT),
                             SKILL_INCREASE_HARD);
  }
  /* if it's backstab send it to one_hit so it can be handled */
  if (type == SKILL_BACKSTAB)
  {
    return one_hit(ch, vict, SKILL_BACKSTAB, weapon);
  }
  else if (type == SKILL_JAB)
  {
    return one_hit(ch, vict, SKILL_JAB, weapon);
  }
  else if (GET_CLASS(ch) == CLASS_MONK && wielded == nullptr)
  {
    if (ch->getLevel() >= MORTAL)
    {
      result = one_hit(ch, vict, type, WEAR_WIELD);
      if (SOMEONE_DIED(result))
        return result;
    }

    if (ch->getLevel() > 47)
      if (ch->dc_->number(0, 1))
      {
        result = one_hit(ch, vict, type, WEAR_WIELD);
        if (SOMEONE_DIED(result))
          return result;
      }

    if (ch->getLevel() > 39)
    {
      result = one_hit(ch, vict, type, WEAR_WIELD);
      if (SOMEONE_DIED(result))
        return result;
    }
    else if (ch->getLevel() > 29)
      if (ch->dc_->number(0, 1))
      {
        result = one_hit(ch, vict, type, WEAR_WIELD);
        if (SOMEONE_DIED(result))
          return result;
      }

    if (ch->getLevel() > 19)
    {
      result = one_hit(ch, vict, type, WEAR_WIELD);
      if (SOMEONE_DIED(result))
        return result;
    }

    if (IS_AFFECTED(ch, AFF_HASTE))
    {
      if (ch->affected_by_spell(SPELL_HASTE)) // spell is 33%
        chance = 33;
      else
        chance = 66; // eq/bard is 66%
      if ((ch->equipment[WEAR_WIELD] && dc_->obj_index[ch->equipment[WEAR_WIELD]->item_number].vnum() == 586) ||
          (ch->equipment[WEAR_SECOND_WIELD] && dc_->obj_index[ch->equipment[WEAR_SECOND_WIELD]->item_number].vnum() == 586))
        chance = 101;
      if (chance > dc_->number(1, 100))
      {
        result = one_hit(ch, vict, type, WEAR_WIELD);
        if (SOMEONE_DIED(result))
          return result;
      }
    }

    // lose an attack if using a shield
    //    if(ch->getLevel() > 9 && !ch->equipment[WEAR_SHIELD]) {
    //    result = one_hit(ch, vict, type, WEAR_WIELD);
    //  if(SOMEONE_DIED(result))       return result;
    // }

    if (isSet(ch->combat, COMBAT_MISS_AN_ATTACK) || IS_AFFECTED(ch, AFF_CRIPPLE))
    {
      ch->sendln("Your body refuses to work properly and you miss an attack.");
      REMOVE_BIT(ch->combat, COMBAT_MISS_AN_ATTACK);
    }
    else if (ch->isPlayer() || !ISSET(ch->mobdata->actflags, ACT_NOATTACK))
    {
      result = one_hit(ch, vict, type, WEAR_WIELD);
      if (SOMEONE_DIED(result))
        return result;
    }
  } // End of the monk attacks
  else // It's a normal attack
  {
    if (isSet(ch->combat, COMBAT_MISS_AN_ATTACK) || IS_AFFECTED(ch, AFF_CRIPPLE))
    {
      ch->sendln("Your body refuses to work properly and you miss an attack.");
      REMOVE_BIT(ch->combat, COMBAT_MISS_AN_ATTACK);
    }
    else if (ch->isPlayer() || !ISSET(ch->mobdata->actflags, ACT_NOATTACK))
    {
      result = one_hit(ch, vict, type, WEAR_WIELD); // everyone get's one hit (normally)
      if (SOMEONE_DIED(result))
        return result;
    }

    // This is here so we only show this after the PC's first
    // attack rather than after every hit.
    if (GET_POS(vict) == position_t::STUNNED)
      act_to_room("$n is $B$0stunned$R, but will probably recover.", vict, 0, 0, INVIS_NULL);

    if (second_attack(ch))
    {
      result = one_hit(ch, vict, type, WEAR_WIELD);
      if (SOMEONE_DIED(result))
        return result;
    }
    if (third_attack(ch))
    {
      result = one_hit(ch, vict, type, WEAR_WIELD);
      if (SOMEONE_DIED(result))
        return result;
    }
    if (fourth_attack(ch))
    {
      result = one_hit(ch, vict, type, WEAR_WIELD);
      if (SOMEONE_DIED(result))
        return result;
    }
    if (IS_AFFECTED(ch, AFF_HASTE))
    {
      if (ch->affected_by_spell(SPELL_HASTE)) // spell is 33%
        chance = 33;
      else
        chance = 66; // eq/bard is 66%
      if ((ch->equipment[WEAR_WIELD] && dc_->obj_index[ch->equipment[WEAR_WIELD]->item_number].vnum() == 586) ||
          (ch->equipment[WEAR_SECOND_WIELD] && dc_->obj_index[ch->equipment[WEAR_SECOND_WIELD]->item_number].vnum() == 586))
        chance = 101;

      if (chance > dc_->number(1, 100))
      {
        result = one_hit(ch, vict, type, WEAR_WIELD);
        if (SOMEONE_DIED(result))
          return result;
      }
    }
    if (ch->affected_by_spell(SKILL_ONSLAUGHT))
    {
      if (ch->dc_->number(1, 100) < ch->has_skill(SKILL_ONSLAUGHT) / 2)
      {
        result = one_hit(ch, vict, type, WEAR_WIELD);
        if (SOMEONE_DIED(result))
          return result;
      }
    }
    // if(second_wield(ch) && gets_dual_wield_attack(ch)) {
    if (gets_dual_wield_attack(ch))
    {
      result = one_hit(ch, vict, type, WEAR_SECOND_WIELD);
      if (SOMEONE_DIED(result))
        return result;
    }

    qint32 lrn = ch->has_skill(SKILL_OFFHAND_DOUBLE);
    if (gets_dual_wield_attack(ch) && lrn)
    {
      ch->skill_increase_check(SKILL_OFFHAND_DOUBLE, lrn, SKILL_INCREASE_HARD);
      qint32 p = lrn / 2 + ch->getHP() * 10 / GET_MAX_HIT(ch) - (10 - ch->has_skill(SKILL_SECOND_ATTACK) / 10);
      if (ch->dc_->number(1, 100) <= p)
      {
        result = one_hit(ch, vict, type, WEAR_SECOND_WIELD);
        if (SOMEONE_DIED(result))
          return result;
      }
    }
  }

  return ReturnValue::eSUCCESS;
} // of attack

void update_flags(CharacterPtr vict)
{
  if (isSet(vict->combat, COMBAT_BASH1))
  {
    REMOVE_BIT(vict->combat, COMBAT_BASH1);
    SET_BIT(vict->combat, COMBAT_BASH2);
  }
  else if (isSet(vict->combat, COMBAT_BASH2))
    REMOVE_BIT(vict->combat, COMBAT_BASH2);

  if (isSet(vict->combat, COMBAT_RAGE1))
  {
    REMOVE_BIT(vict->combat, COMBAT_RAGE1);
    SET_BIT(vict->combat, COMBAT_RAGE2);
  }
  else if (isSet(vict->combat, COMBAT_RAGE2))
  {
    REMOVE_BIT(vict->combat, COMBAT_RAGE2);
    act_to_room("$n calms down a bit.", vict, 0, 0, 0);
    act_to_character("Your mind seems a bit clearer now.", vict, 0, 0, 0);
  }

  if (isSet(vict->combat, COMBAT_CRUSH_BLOW))
  {
    REMOVE_BIT(vict->combat, COMBAT_CRUSH_BLOW);
    SET_BIT(vict->combat, COMBAT_CRUSH_BLOW2);
  }
  else if (isSet(vict->combat, COMBAT_CRUSH_BLOW2))
  {
    REMOVE_BIT(vict->combat, COMBAT_CRUSH_BLOW2);
    act_to_room("$n shrugs off $s weakness!", vict, 0, 0, 0);
    act_to_character("You shrug off your weakness.!", vict, 0, 0, 0);
  }

  if (isSet(vict->combat, COMBAT_BLADESHIELD1))
  {
    REMOVE_BIT(vict->combat, COMBAT_BLADESHIELD1);
    SET_BIT(vict->combat, COMBAT_BLADESHIELD2);
  }
  else if (isSet(vict->combat, COMBAT_BLADESHIELD2))
    REMOVE_BIT(vict->combat, COMBAT_BLADESHIELD2);

  if (isSet(vict->combat, COMBAT_VITAL_STRIKE))
    REMOVE_BIT(vict->combat, COMBAT_VITAL_STRIKE);

  if (isSet(vict->combat, COMBAT_ORC_BLOODLUST2))
  {
    REMOVE_BIT(vict->combat, COMBAT_ORC_BLOODLUST2);
    vict->sendln("Your bloodlust fades.");
    act_to_room("$n's bloodlust fades.", vict, 0, 0, 0);
  }

  if (isSet(vict->combat, COMBAT_ORC_BLOODLUST1))
  {
    REMOVE_BIT(vict->combat, COMBAT_ORC_BLOODLUST1);
    SET_BIT(vict->combat, COMBAT_ORC_BLOODLUST2);
  }

  if (isSet(vict->combat, COMBAT_THI_EYEGOUGE2))
  {
    REMOVE_BIT(vict->combat, COMBAT_THI_EYEGOUGE2);
    REMBIT(vict->affected_by, AFF_BLIND);
    act_to_room("$n clears the blood from $s eyes.\r\n", vict, nullptr, nullptr, 0);
    vict->sendln("You clear the blood out of your eyes.");
  }

  if (isSet(vict->combat, COMBAT_THI_EYEGOUGE))
  {
    REMOVE_BIT(vict->combat, COMBAT_THI_EYEGOUGE);
    SET_BIT(vict->combat, COMBAT_THI_EYEGOUGE2);
  }

  if (isSet(vict->combat, COMBAT_MONK_STANCE))
  {
    // stance lasts 'modifier' rounds.  Remove bit once used up
    affected_type *pspell;
    pspell = vict->affected_by_spell(KI_STANCE + KI_OFFSET);
    if (!pspell)
    {
      REMOVE_BIT(vict->combat, COMBAT_MONK_STANCE);
      return;
    }
    pspell->modifier--;
    if (pspell->modifier < 0)
    {
      REMOVE_BIT(vict->combat, COMBAT_MONK_STANCE);
      vict->sendln("Your stance ends.  You can absorb no more.");
    }
    else
      vict->sendln("Your stance weakens...");
  }
}

void update_stuns(CharacterPtr ch)
{
  if (isSet(ch->combat, COMBAT_SHOCKED))
  {
    REMOVE_BIT(ch->combat, COMBAT_SHOCKED);
    SET_BIT(ch->combat, COMBAT_SHOCKED2);
  }
  else if (isSet(ch->combat, COMBAT_SHOCKED2))
    REMOVE_BIT(ch->combat, COMBAT_SHOCKED2);

  if (isSet(ch->combat, COMBAT_STUNNED))
  {
    REMOVE_BIT(ch->combat, COMBAT_STUNNED);
    SET_BIT(ch->combat, COMBAT_STUNNED2);
  }
  else if (isSet(ch->combat, COMBAT_STUNNED2))
  {
    REMOVE_BIT(ch->combat, COMBAT_STUNNED2);
    if (ch->getHP() > 0)
      if (GET_POS(ch) != position_t::FIGHTING)
      {
        act_to_room("$n regains consciousness...", ch, 0, 0, 0);
        act_to_character("You regain consciousness...", ch, 0, 0, 0);
        if (ch->fighting)
          ch->setPOSFighting();
        else
          ch->setStanding();
        ;
        if (isSet(ch->combat, COMBAT_BERSERK))
        {
          ch->sendln("After that period of unconsciousness, you've forgotten what you were mad about.");
          REMOVE_BIT(ch->combat, COMBAT_BERSERK);
        }
      }
  }
}

bool do_frostshield(CharacterPtr ch, CharacterPtr vict)
{
  if (!ch || !vict)
  {
    dc_->logentry(u"Null ch or vict sent to do_frostshield"_s, IMPLEMENTER, DC::LogChannel::LOG_BUG);
    return (false);
  }
  if (!IS_AFFECTED(vict, AFF_FROSTSHIELD))
  {
    return (false);
  }
  if (ch->dc_->number(0, 99) < 5)
  {
    act_to_victim("Bits of $Bfrost$R fly as $n's blow bounces off your $B$1icy$R shield.", ch, 0, vict, 0);
    act_to_room("Bits of $Bfrost$R fly as $n's blow bounces off $N's $B$1icy$R shield.", ch, 0, vict, NOTVICT);
    act_to_character("Bits of $Bfrost$R fly as your blow bounces off $N's $B$1icy$R shield.", ch, 0, vict, 0);
    return (true);
  }
  else
  {
    return (false);
  }
}

command_return_t do_lightning_shield(CharacterPtr ch, CharacterPtr vict, qint32 dam)
{
  affected_type *cur_af;
  qint32 learned = {};

  if (!ch || !vict)
  {
    dc_->logentry(u"Null ch or vict sent to do_lightning_shield"_s, IMPLEMENTER, DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (GET_POS(vict) == position_t::DEAD)
    return ReturnValue::eFAILURE;
  if (ch->isImmortalPlayer())
    return ReturnValue::eFAILURE;
  if (!IS_AFFECTED(vict, AFF_LIGHTNINGSHIELD))
    return ReturnValue::eFAILURE;
  if (isSet(races[(qint32)ch->race].immune, ISR_ENERGY) || isSet(ch->immune, ISR_ENERGY))
  {
    dam = {};
  }
  else
  {
    if ((cur_af = vict->affected_by_spell(SPELL_LIGHTNING_SHIELD)))
      learned = (qint32)cur_af->modifier;

    if (learned == 0) // mob
      learned = 81;

    // Close to 50% damage returned for a fully learned lightning shield
    dam *= ((learned / 100.0) * 53.0) / 100.0;

    // Make it fluctuate between 475-525
    if (dam > 500)
    {
      dam = 475;
      dam += dc_->number(0, 50);
    }

    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
    if (ch->affected_by_spell(SPELL_HOLY_AURA) && ch->affected_by_spell(SPELL_HOLY_AURA)->modifier == 25)
      dam /= 2;
    if (ch->affected_by_spell(SPELL_DIVINE_INTER) && dam > ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier)
      dam = ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier;
    qint32 save = get_saves(ch, SAVE_TYPE_ENERGY);
    if (ch->dc_->number(1, 100) < save || save < 0)
    {
      if (save > 50)
        save = 50;
      dam -= (qint32)(dam * (double)save / 100);
    }
  }

  /*  if ((learned = ch->has_skill( SKILL_SHIELDBLOCK)) && ch->equipment[WEAR_SHIELD] && GET_CLASS(ch) != CLASS_ANTI_PAL && GET_CLASS(ch) != CLASS_THIEF)
  {
      if (learned/3 > dc_->number(0,99)) {
        act_to_victim("$n deftly blocks your burst of $B$5lightning$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict,  0);
        act_to_character("You defly block $N's burst of $B$5lightning$R with your $p!", ch, ch->equipment[WEAR_SHIELD], vict,  0);
        act_to_room("$n deftly blocks $N's burst of $B$5lightning$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict,  NOTVICT);
        return ReturnValue::eFAILURE;
      }
    }
    */
  ch->removeHP(dam);
  do_dam_msgs(vict, ch, dam, SPELL_LIGHTNING_SHIELD, WEAR_WIELD);
  update_pos(ch);

  if (GET_POS(ch) == position_t::DEAD)
  {
    act_to_room("$n is DEAD!!", ch, 0, 0, INVIS_NULL);
    group_gain(vict, ch);
    if (ch->isPlayer())
      ch->sendln("You have been KILLED!!\r\n");

    fight_kill(vict, ch, TYPE_CHOOSE, 0);
    return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
  }

  return ReturnValue::eSUCCESS;
}

// standard retvals
command_return_t do_vampiric_aura(CharacterPtr ch, CharacterPtr vict)
{
  if (!ch || !vict || ch == vict)
  {
    dc_->logentry(u"Null ch or vict, or ch==vict sent to do_vampiric_aura!"_s, IMPLEMENTER, DC::LogChannel::LOG_BUG);
    abort();
  }

  if (GET_POS(vict) == position_t::DEAD)
    return ReturnValue::eFAILURE;

  affected_type *af;

  if (nullptr == (af = vict->affected_by_spell(SPELL_VAMPIRIC_AURA)))
    return ReturnValue::eFAILURE;

  // ch just hit vict...vict has Vampiric aura up
  if (ch->dc_->number(0, 101) < ((af->modifier) / 1.7))
  {
    qint32 level = MAX(1, af->modifier / 6);
    qint32 retval = spell_vampiric_touch(level, vict, ch, 0, af->modifier);
    retval = SWAP_CH_VICT(retval);
    return debug_retval(ch, vict, retval);
  }
  return ReturnValue::eFAILURE;
}

// standard retvals
command_return_t do_fireshield(CharacterPtr ch, CharacterPtr vict, qint32 dam)
{
  // ch is the person who just hit the victim
  // so ch takes the damage from this spell
  affected_type *cur_af;
  qint32 learned = {};

  if (!ch || !vict || ch == vict)
  {
    dc_->logentry(u"Null ch or vict, or ch==vict sent to do_fireshield!"_s, IMPLEMENTER, DC::LogChannel::LOG_BUG);
    abort();
  }

  if (GET_POS(vict) == position_t::DEAD)
    return ReturnValue::eFAILURE;
  if (ch->isImmortalPlayer())
    return ReturnValue::eFAILURE;
  if (!IS_AFFECTED(vict, AFF_FIRESHIELD))
    return ReturnValue::eFAILURE;

  if (isSet(races[(qint32)ch->race].immune, ISR_FIRE) ||
      isSet(ch->immune, ISR_FIRE))
  {
    dam = {};
  }
  else
  {
    if ((cur_af = vict->affected_by_spell(SPELL_FIRESHIELD)))
      learned = (qint32)cur_af->modifier;

    if (learned == 0) // mob
      learned = 81;

    // Close to 75% damage returned for a fully learned fireshield
    dam *= ((learned / 100.0) * 79.0) / 100.0;

    // Make it fluctuate between 475-525
    if (dam > 500)
    {
      dam = 475;
      dam += dc_->number(0, 50);
    }

    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
    if (ch->affected_by_spell(SPELL_HOLY_AURA) && ch->affected_by_spell(SPELL_HOLY_AURA)->modifier == 25)
      dam /= 2;
    if (ch->affected_by_spell(SPELL_DIVINE_INTER) && dam > ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier)
      dam = ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier;
    qint32 save = get_saves(ch, SAVE_TYPE_FIRE);
    if (ch->dc_->number(1, 100) < save || save < 0)
    {
      if (save > 50)
        save = 50;
      dam -= (qint32)(dam * (double)save / 100);
    }
  }

  /*
    if ((learned = ch->has_skill( SKILL_SHIELDBLOCK)) && ch->equipment[WEAR_SHIELD] && GET_CLASS(ch) != CLASS_ANTI_PAL && GET_CLASS(ch) != CLASS_THIEF) {
      if (learned/3 > dc_->number(0,99)) {
        act_to_victim("$n deftly blocks your burst of $B$4flame$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict,  0);
        act_to_character("You defly block $N's burst of $B$4flame$R with your $p!", ch, ch->equipment[WEAR_SHIELD], vict,  0);
        act_to_room("$n deftly blocks $N's burst of $B$4flame$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict,  NOTVICT);
        return ReturnValue::eFAILURE;
      }
    }
  */
  ch->removeHP(dam);
  do_dam_msgs(vict, ch, dam, SPELL_FIRESHIELD, WEAR_WIELD);
  update_pos(ch);

  if (GET_POS(ch) == position_t::DEAD)
  {
    act_to_room("$n is DEAD!!", ch, 0, 0, INVIS_NULL);
    group_gain(vict, ch);
    if (ch->isPlayer())
      ch->sendln("You have been KILLED!!\r\n");

    fight_kill(vict, ch, TYPE_CHOOSE, 0);
    return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
  }

  return ReturnValue::eSUCCESS;
}

// standard retvals
command_return_t do_acidshield(CharacterPtr ch, CharacterPtr vict, qint32 dam)
{
  // ch is the person who just hit the victim
  // so ch takes the damage from this spell
  affected_type *cur_af;
  qint32 learned = {};

  if (!ch || !vict || ch == vict)
  {
    dc_->logentry(u"Null ch or vict, or ch==vict sent to do_acidshield!"_s, IMPLEMENTER, DC::LogChannel::LOG_BUG);
    abort();
  }

  if (GET_POS(vict) == position_t::DEAD)
    return ReturnValue::eFAILURE;
  if (ch->isImmortalPlayer())
    return ReturnValue::eFAILURE;
  if (!IS_AFFECTED(vict, AFF_ACID_SHIELD))
    return ReturnValue::eFAILURE;

  if (isSet(races[(qint32)ch->race].immune, ISR_ACID) || isSet(ch->immune, ISR_ACID))
    dam = {};
  else
  {
    if ((cur_af = vict->affected_by_spell(SPELL_ACID_SHIELD)))
      learned = (qint32)cur_af->modifier;

    if (learned == 0) // mob
      learned = 81;

    // Close to 30% damage returned for a fully learned acidshield
    dam *= ((learned / 100.0) * 32.0) / 100.0;

    // Make it fluctuate between 475-525
    if (dam > 500)
    {
      dam = 475;
      dam += dc_->number(0, 50);
    }

    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
    if (ch->affected_by_spell(SPELL_HOLY_AURA) && ch->affected_by_spell(SPELL_HOLY_AURA)->modifier == 25)
      dam /= 2;
    if (ch->affected_by_spell(SPELL_DIVINE_INTER) && dam > ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier)
      dam = ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier;
    qint32 save = get_saves(ch, SAVE_TYPE_ACID);
    if (ch->dc_->number(1, 100) < save || save < 0)
    {
      if (save > 50)
        save = 50;
      dam -= (qint32)(dam * (double)save / 100);
    }
  }

  /*
    if ((learned = ch->has_skill( SKILL_SHIELDBLOCK)) && ch->equipment[WEAR_SHIELD] && GET_CLASS(ch) != CLASS_ANTI_PAL && GET_CLASS(ch) != CLASS_THIEF) {
      if (learned/3 > dc_->number(0,99)) {
        act_to_victim("$n deftly blocks your burst of $B$2acid$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict,  0);
        act_to_character("You defly block $N's burst of $B$2acid$R with your $p!", ch, ch->equipment[WEAR_SHIELD], vict,  0);
        act_to_room("$n deftly blocks $N's burst of $B$2acid$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict,  NOTVICT);
        return ReturnValue::eFAILURE;
      }
    }
    */
  ch->removeHP(dam);
  do_dam_msgs(vict, ch, dam, SPELL_ACID_SHIELD, WEAR_WIELD);
  update_pos(ch);

  if (GET_POS(ch) == position_t::DEAD)
  {
    act_to_room("$n is DEAD!!", ch, 0, 0, INVIS_NULL);
    group_gain(vict, ch);
    if (ch->isPlayer())
      ch->sendln("You have been KILLED!!\r\n");

    fight_kill(vict, ch, TYPE_CHOOSE, 0);
    return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_boneshield(CharacterPtr ch, CharacterPtr vict, qint32 dam)
{
  // ch is the person who just hit the victim
  // so ch takes the damage from this spell
  affected_type *cur_af;
  qint32 learned = {};

  if (!ch || !vict || ch == vict)
  {
    dc_->logentry(u"Null ch or vict, or ch==vict sent to do_boneshield!"_s, IMPLEMENTER, DC::LogChannel::LOG_BUG);
    abort();
  }

  if (GET_POS(vict) == position_t::DEAD)
    return ReturnValue::eFAILURE;
  if (ch->isImmortalPlayer())
    return ReturnValue::eFAILURE;
  if (!vict->affected_by_spell(SPELL_BONESHIELD))
    return ReturnValue::eFAILURE;

  if (isSet(races[(qint32)ch->race].immune, ISR_PHYSICAL) ||
      isSet(ch->immune, ISR_PHYSICAL))
    dam = {};
  else
  {
    if ((cur_af = vict->affected_by_spell(SPELL_BONESHIELD)))
      learned = (qint32)cur_af->modifier;

    dam = learned;

    dam -= ch->melee_mitigation;

    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
    if (ch->affected_by_spell(SPELL_HOLY_AURA) && ch->affected_by_spell(SPELL_HOLY_AURA)->modifier == 25)
      dam /= 2;
    if (ch->affected_by_spell(SPELL_DIVINE_INTER) && dam > ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier)
      dam = ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier;
  }

  ch->removeHP(dam);
  do_dam_msgs(vict, ch, dam, SPELL_BONESHIELD, WEAR_WIELD);
  update_pos(ch);

  if (GET_POS(ch) == position_t::DEAD)
  {
    act_to_room("$n is DEAD!!", ch, 0, 0, INVIS_NULL);
    group_gain(vict, ch);
    if (ch->isPlayer())
      ch->sendln("You have been KILLED!!\r\n");

    fight_kill(vict, ch, TYPE_CHOOSE, 0);
    return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
  }

  return ReturnValue::eSUCCESS;
}

void check_weapon_skill_bonus(CharacterPtr ch, qint32 type, ObjectPtr wielded,
                              qint32 &weapon_skill_hit_bonus, qint32 &weapon_skill_dam_bonus)
{
  qint32 specialization;
  qint32 skill, learned;
  switch (type)
  {
  case TYPE_BLUDGEON:

    skill = SKILL_BLUDGEON_WEAPONS;
    break;
  case TYPE_WHIP:
    skill = SKILL_WHIPPING_WEAPONS;
    break;
  case TYPE_CRUSH:
    skill = SKILL_CRUSHING_WEAPONS;
    break;
  case TYPE_SLASH:
    skill = SKILL_SLASHING_WEAPONS;
    break;
  case TYPE_PIERCE:
    skill = SKILL_PIERCEING_WEAPONS;
    break;
  case TYPE_STING:
    skill = SKILL_STINGING_WEAPONS;
    break;
  case TYPE_HIT:
    skill = SKILL_HAND_TO_HAND;
    break;
  default:
    weapon_skill_hit_bonus = {};
    weapon_skill_dam_bonus = {};
    return;
  }
  learned = ch->has_skill(skill);
  if (skill_success(ch, nullptr, skill))
  {
    // rare skill increases
    specialization = learned / 100;
    learned = learned % 100;

    weapon_skill_hit_bonus = learned / 5;
    weapon_skill_dam_bonus = learned / 10 ? dc_->number(1, learned / 10) : 1;

    if (specialization)
    {
      weapon_skill_hit_bonus += 4;
      weapon_skill_dam_bonus += dc_->number(1, 5);
    }
  }

  // now check for two-handed weapons
  if (wielded && isSet(wielded->obj_flags.extra_flags, ITEM_TWO_HANDED) &&
      (learned = ch->has_skill(SKILL_TWO_HANDED_WEAPONS)))
  {
    // rare skill increases
    if (0 == dc_->number(0, 5))
      ch->skill_increase_check(SKILL_TWO_HANDED_WEAPONS, learned, SKILL_INCREASE_HARD);

    specialization = learned / 100;
    learned = learned % 100;

    weapon_skill_hit_bonus += learned / 6;
    weapon_skill_dam_bonus += learned / 10 ? dc_->number(1, learned / 10) : 1;

    if (specialization)
    {
      weapon_skill_hit_bonus += 5;
      weapon_skill_dam_bonus += dc_->number(1, 5);
    }
  }
}

qint32 get_weapon_damage_type(ObjectPtr wielded)
{
  QString log_buf;

  switch (wielded->obj_flags.value[3])
  {
  case 0:
  case 1:
    return TYPE_WHIP;
    break;
  case 2:
  case 3:
    return TYPE_SLASH;
    break;
  case 4:
  case 5:
    return TYPE_CRUSH;
    break;
  case 6:
  case 7:
    return TYPE_BLUDGEON;
    break;
  case 8:
  case 9:
    return TYPE_STING;
    break;
  case 10:
  case 11:
    return TYPE_PIERCE;
    break;
  default:
    logbug(u"WORLD: Unknown w_type for object #%1 name: %2, fourth value flag is: %3."_s.arg(wielded->item_number).arg(wielded->name()).arg(wielded->obj_flags.value[3]));
    break;
  }
  return TYPE_HIT; // should never get here
}

qint32 get_monk_bare_damage(CharacterPtr ch)
{
  qint32 dam = {};

  if (ch->getLevel() < 11)
    dam = dice(4, 1);
  else if (ch->getLevel() < 21)
    dam = dice(4, 2);
  else if (ch->getLevel() < 31)
    dam = dice(4, 3);
  else if (ch->getLevel() < 41)
    dam = dice(4, 4);
  else if (ch->getLevel() < 50)
    dam = dice(4, 5);
  else if (ch->getLevel() < 60)
    dam = dice(5, 5);
  else if (ch->getLevel() < 61)
    dam = dice(6, 5);
  else if (ch->getLevel() < 100)
    dam = dice(10, 6);
  else if (ch->getLevel() < 110)
    dam = dice(10, 10);
  else
    dam = dice(50, 5);

  return dam;
}

qint32 calculate_paladin_damage_bonus(CharacterPtr ch, CharacterPtr victim)
{
  if (GET_CLASS(ch) != CLASS_PALADIN)
    return 0;

  if (GET_ALIGNMENT(victim) > 350)
    return (-(ch->getLevel() / 10));

  if (GET_ALIGNMENT(victim) < -350)
    return (ch->getLevel() / 10);

  return 0;
}

// standard "returnvals.h" returns
qint32 one_hit(CharacterPtr ch, CharacterPtr vict, qint32 type, qint32 weapon)
{
  ObjectPtr wielded; /* convenience */
  qint32 w_type;     /* Holds type info for damage() */
  qint32 weapon_type;
  qint32 dam = {}; /* Self explan. */
  qint32 retval = {};
  qint32 weapon_skill_hit_bonus = {};
  qint32 weapon_skill_dam_bonus = {};

  extern quint8 backstab_mult[];

  if (!vict || !ch)
  {
    dc_->logentry(u"Null victim or character in one_hit!  This Crashes us!"_s, -1, DC::LogChannel::LOG_BUG);
    return ReturnValue::eINTERNAL_ERROR;
  }

  if (ch == vict)
  {
    do_say(ch, "What the hell am I DOING?!?!");
    stop_fighting(ch);
    return ReturnValue::eFAILURE;
  }

  if (GET_POS(vict) == position_t::DEAD)
    return (ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED);

  // TODO - I'd like to remove these 3 cause they are checked in attack()
  /* This happens with multi-attacks */
  if (ch->in_room != vict->in_room)
    return ReturnValue::eFAILURE;
  if (!can_be_attacked(ch, vict))
    return ReturnValue::eFAILURE;
  if (!can_attack(ch))
    return ReturnValue::eFAILURE;

  // Figure out the correct weapon
  wielded = ch->equipment[weapon];

  // Second got called without a secondary wield
  if (weapon == WEAR_SECOND_WIELD && wielded == nullptr)
    return ReturnValue::eFAILURE;

  set_cantquit(ch, vict); /* This sets the flag if necessary */

  w_type = TYPE_HIT;
  if (wielded && wielded->obj_flags.type_flag == ITEM_WEAPON)
    w_type = get_weapon_damage_type(wielded);

  if (wielded && dc_->obj_index[wielded->item_number].vnum() == 30019 && isSet(wielded->obj_flags.more_flags, ITEM_TOGGLE))
  {                     // Durendal - changes damage type and other stuff
    w_type = TYPE_FIRE; // no skill bonus
  }

  check_weapon_skill_bonus(ch, w_type, wielded, weapon_skill_hit_bonus, weapon_skill_dam_bonus);

  weapon_type = w_type;
  if (type == SKILL_BACKSTAB)
    w_type = SKILL_BACKSTAB;
  if (type == SKILL_JAB)
    w_type = SKILL_JAB;

  if (wielded)
  {
    if (ch->affected_by_spell(SKILL_SMITE) && ch->affected_by_spell(SKILL_SMITE)->victim == vict)
      for (qint32 i = {}; i < wielded->obj_flags.value[1]; i++)
        dam += wielded->obj_flags.value[2] - dc_->number(0, 1);
    else
      dam = dice(wielded->obj_flags.value[1], wielded->obj_flags.value[2]);
    if (ch->isNonPlayer())
    {
      dam = dice(ch->mobdata->damnodice, ch->mobdata->damsizedice);
      dam += (dice(wielded->obj_flags.value[1], wielded->obj_flags.value[2]) / 2);
    }
  }
  else if (ch->isNonPlayer())
    dam = dice(ch->mobdata->damnodice, ch->mobdata->damsizedice);
  else if (GET_CLASS(ch) == CLASS_MONK && wielded == nullptr)
    dam = get_monk_bare_damage(ch);
  else
    dam = number<quint64>(0, ch->getLevel() / 2);

  /* Damage bonuses */
  //  dam += str_app[STRENGTH_APPLY_INDEX(ch)].todam;
  dam += GET_REAL_DAMROLL(ch);
  dam += weapon_skill_dam_bonus;
  dam += calculate_paladin_damage_bonus(ch, vict);

  if (wielded && dc_->obj_index[wielded->item_number].vnum() == 30019 && isSet(wielded->obj_flags.more_flags, ITEM_TOGGLE))
  {
    dam = dam * 85 / 100;
    dam = dam + (getRealSpellDamage(ch) / 2);
    w_type = SKILL_FLAMESLASH;
  }
  // BACKSTAB GOES IN HERE!
  if ((type == SKILL_BACKSTAB || type == SKILL_CIRCLE) && dam < 10000)
  { // Bingo not affected.
    if (isSet(ch->combat, COMBAT_CIRCLE))
    {
      if (ch->getLevel() <= MORTAL)
        if (type == SKILL_CIRCLE)
          dam = dam * 3 / 2; // non stabbing weapons
        else
          dam *= ((backstab_mult[(qint32)ch->getLevel()]) / 2);
      else
        dam *= 25;
      REMOVE_BIT(ch->combat, COMBAT_CIRCLE);
    }
    else if ((GET_CLASS(ch) == CLASS_THIEF) ||
             (GET_CLASS(ch) == CLASS_ANTI_PAL) || ch->isNonPlayer())
    {
      if (ch->getLevel() <= MORTAL)
      {
        dam *= backstab_mult[(qint32)ch->getLevel()];
      }
      else
        dam *= 25;
    }
  }

  if (wielded && isSet(wielded->obj_flags.extra_flags, ITEM_TWO_HANDED) && ch->has_skill(SKILL_EXECUTE))
    if (vict->getHP() < 3500 && vict->getHP() * 100 / GET_MAX_HIT(vict) < 15)
    {
      retval = do_execute_skill(ch, vict, w_type);
      if (SOMEONE_DIED(retval))
        return debug_retval(ch, vict, retval);
    }

  if (dam < 1)
    dam = 1;

  // Now check for special code that occurs each hit
  retval = do_skewer(ch, vict, dam, weapon_type, w_type, weapon);
  if (SOMEONE_DIED(retval))
    return debug_retval(ch, vict, retval);

  if (ch->has_skill(SKILL_NAT_SELECT) && ch->affected_by_spell(SKILL_NAT_SELECT))
    if (ch->affected_by_spell(SKILL_NAT_SELECT)->modifier == vict->race)
      dam += 15 + ch->has_skill(SKILL_NAT_SELECT) / 10;

  do_combatmastery(ch, vict, weapon);
  if (isSet(ch->combat, COMBAT_CRUSH_BLOW2))
    dam >>= 1; // dam = dam / 2;

  if (w_type == TYPE_HIT && ch->isNonPlayer())
  {
    qint32 a = dc_->mob_index[ch->mobdata->nr].vnum();
    switch (a)
    {
    case 88:
      w_type = TYPE_FIRE;
      break;
    case 89:
      w_type = TYPE_WATER;
      break;
    case 90:
      w_type = TYPE_ENERGY;
      break;
    case 91:
      w_type = TYPE_CRUSH;
      break;
    default:
      break;
    }
  }

  retval = damage(ch, vict, dam, weapon_type, w_type, weapon);
  if (SOMEONE_DIED(retval) || !ch->fighting)
  {
    return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
  }

  // Was last hit a success?
  if (isSet(retval, ReturnValue::eSUCCESS))
  {
    // If they're wielding a weapon check for weapon spells, otherwise check for hand spells
    if (wielded)
    {
      retval = weapon_spells(ch, vict, weapon);
      if (SOMEONE_DIED(retval) || !ch->fighting)
      {
        return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
      }

      if (ch->equipment[weapon] && dc_->obj_index[ch->equipment[weapon]->item_number].combat_func)
      {
        retval = ((*dc_->obj_index[ch->equipment[weapon]->item_number].combat_func)(ch, ch->equipment[weapon], cmd_t::UNDEFINED, "", ch));
        if (SOMEONE_DIED(retval) || !ch->fighting)
        {
          return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
        }
      }
    }
    else
    { // not wielding a weapon
      if (ch->equipment[WEAR_HANDS])
      {

        retval = weapon_spells(ch, vict, WEAR_HANDS);
        if (SOMEONE_DIED(retval) || !ch->fighting)
        {
          return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
        }
        if (dc_->obj_index[ch->equipment[WEAR_HANDS]->item_number].combat_func)
          retval = ((*dc_->obj_index[ch->equipment[WEAR_HANDS]->item_number].combat_func)(ch, ch->equipment[WEAR_HANDS], cmd_t::UNDEFINED, "", ch));
        if (SOMEONE_DIED(retval) || !ch->fighting)
        {
          return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
        }
      }

      if (ch->equipment[WEAR_HOLD])
      {
        retval = weapon_spells(ch, vict, WEAR_HOLD);
        if (SOMEONE_DIED(retval) || !ch->fighting)
        {
          return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
        }

        if (ch->equipment[WEAR_HOLD]->item_number >= 0)
        {
          if (dc_->obj_index[ch->equipment[WEAR_HOLD]->item_number].combat_func != nullptr)
            retval = ((*dc_->obj_index[ch->equipment[WEAR_HOLD]->item_number].combat_func)(ch, ch->equipment[WEAR_HOLD], cmd_t::UNDEFINED, "", ch));
          if (SOMEONE_DIED(retval) || !ch->fighting)
          {
            return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
          }
        }
      }

      if (ch->equipment[WEAR_HOLD2])
      {

        retval = weapon_spells(ch, vict, WEAR_HOLD2);
        if (SOMEONE_DIED(retval) || !ch->fighting)
        {
          return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
        }

        if (ch->equipment[WEAR_HOLD2]->item_number >= 0)
        {
          if (dc_->obj_index[ch->equipment[WEAR_HOLD2]->item_number].combat_func != nullptr)
            retval = ((*dc_->obj_index[ch->equipment[WEAR_HOLD2]->item_number].combat_func)(ch, ch->equipment[WEAR_HOLD2], cmd_t::UNDEFINED, "", ch));
          if (SOMEONE_DIED(retval) || !ch->fighting)
          {
            return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
          }
        }
      }
    }
  }

  if (act_poisonous(ch))
  {
    retval = cast_poison(ch->getLevel(), ch, "", SPELL_TYPE_SPELL, vict, nullptr, ch->getLevel());
    if (SOMEONE_DIED(retval))
      return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
  }

  if (ch->isNonPlayer() && ISSET(ch->mobdata->actflags, ACT_DRAINY))
  {
    if (ch->dc_->number(1, 100) <= 10)
    {
      SET_BIT(retval, spell_energy_drain(1, ch, vict, 0, 0));
    }
  }

  return debug_retval(ch, vict, retval) | ReturnValue::eSUCCESS;
} // one_hit

// pos of -1 means inventory
void eq_destroyed(CharacterPtr ch, ObjectPtr obj, qint32 pos)
{
  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    return;
  if (isSet(obj->obj_flags.more_flags, ITEM_NO_SCRAP))
    return;

  if (pos != -1) // if its not an inventory item, do this
  {
    ch->unequip_char(pos);

    if (pos == WEAR_LIGHT)
    {
      dc_->world[ch->in_room].light--;
      ch->glow_factor--;
    }
    else if ((pos == WEAR_WIELD) && (ch->equipment[WEAR_SECOND_WIELD]))
    {
      ObjectPtr temp;
      temp = ch->unequip_char(WEAR_SECOND_WIELD);
      ch->equip_char(temp, WEAR_WIELD);
    }
    act_to_room("$p carried by $n is destroyed.", ch, obj, 0, 0);
  }
  else
  { // if its an inventory item, do this
    act_to_room("$p worn by $n is destroyed.", ch, obj, 0, 0);
    ch->recheck_height_wears(); // Make sure $n can still wear the rest of
                                // the eq
  }

  act_to_character("Your $p has been destroyed.", ch, obj, 0, 0);

  while (obj->contains) // drop contents to floor
  {
    if (isSet(obj->contains->obj_flags.more_flags, ITEM_NO_TRADE))
    {
      act_to_room("A $p falls to $n's inventory.", ch, obj->contains, 0, 0);
      act_to_character("A $p falls to your inventory from your destroyed container.", ch, obj->contains, 0, 0);
      move_obj(obj->contains, ch);
    }
    else
    {
      act_to_room("A $p falls to the ground from $n.", ch, obj->contains, 0, 0);
      act_to_character("A $p falls to the ground from your destroyed container.", ch, obj->contains, 0, 0);
      move_obj(obj->contains, ch->in_room); // this updates obj->contains
    }
  }

  act_to_character("$p falls to the ground in scraps.", ch, obj, 0, 0);
  act_to_room("$p falls to the ground in scraps.", ch, obj, 0, 0);
  make_scraps(ch, obj);
  extract_obj(obj);
}

void eq_damage(CharacterPtr ch, CharacterPtr victim,
               qint32 dam, qint32 weapon_type, qint32 attacktype)
{
  qint32 count, eqdam, chance, value;
  ObjectPtr obj;
  qint32 pos;

  if (ch->room().isArena()) // Don't damage eq in arena
    return;
  if (victim->isPlayer() && ch->isPlayer()) // Don't damage eq on pc->pc fights
    return;

  chance = 10 + GET_DEX(ch);
  if (dam < 40)         // Under 40 damage decreases chance of damage
    chance += 40 - dam; // Helps out the lower level chars

  if (ch->dc_->number(0, chance)) // 1 in chance of eq damage
    return;

  //  let's scrap some worn eq
  if (ch->dc_->number(0, 3) == 0)
  {
    // TODO - eventually we need to determine where someone gets hit and just pass the location

    // determine which location takes the damage giving a higher chance to certain locations
    pos = dc_->number(0, MAX_WEAR + 6);
    switch (pos)
    {
    case MAX_WEAR:
    case MAX_WEAR + 1:
    case MAX_WEAR + 2:
      pos = WEAR_BODY;
      break;
    case MAX_WEAR + 3:
      pos = WEAR_LEGS;
      break;
    case MAX_WEAR + 4:
      pos = WEAR_ARMS;
      break;
    case MAX_WEAR + 5:
      pos = WEAR_HEAD;
      break;
    case MAX_WEAR + 6:
      pos = WEAR_SHIELD;
      break;
    default:
      break; // pos = itself
    }

    if (!victim->equipment[pos]) // they lucked out and didn't get anything hit
      return;

    obj = victim->equipment[pos];

    // add a point
    eqdam = damage_eq_once(obj);

    if (!obj)
      return;

    if (dc_->obj_index[obj->item_number].progtypes & ARMOUR_PROG)
      victim->oprog_armour_trigger(obj);
    // determine if time to scrap it
    if (eqdam >= eq_max_damage(obj))
      eq_destroyed(victim, obj, pos);
    else
    {
      act_to_character("$p is damaged.", victim, obj, 0, 0);
      act_to_room("$p worn by $n is damaged.", victim, obj, 0, 0);
    }
  } // dc_->number(0, 3) == 0
  else if (victim->carrying && (ch->dc_->number(0, 19) == 0))
  {
    // let's scrap something in the inventory
    for (count = 0, obj = victim->carrying; obj; obj = obj->next_content)
      count++;
    value = dc_->number(1, count); // choose a random inventory item
    // loop up to that item
    for (count = 1, obj = victim->carrying; obj && count < value; obj = obj->next_content)
      count++;

    assert(obj);

    // add a point
    eqdam = damage_eq_once(obj);

    if (!obj)
      return;

    if (dc_->obj_index[obj->item_number].progtypes & ARMOUR_PROG)
      victim->oprog_armour_trigger(obj);

    // determine if time to scrap it
    if (eqdam >= eq_max_damage(obj))
      eq_destroyed(victim, obj, -1);
    else
    {
      act_to_character("$p is damaged.", victim, obj, 0, 0);
      act_to_room("$p carried by $n is damaged.", victim, obj, 0, 0);
    }
  } // end of inventory damage...

  victim->save_char_obj();
}

void pir_stat_loss(CharacterPtr victim, qint32 chance, bool heh, bool zz)
{
  QString log_buf = {};
  if (victim->getLevel() < 50)
    return;
  chance /= 2;
  /* Pir's extra stat loss.  Bwahahah */
  if ((heh || (ch->dc_->number(0, 99) < chance)) && victim->isPlayer())
  {
    switch (ch->dc_->number(1, 5))
    {
    case 1:
      if (GET_STR(victim) > 4)
      {
        GET_STR(victim) -= 1;
        victim->raw_str -= 1;
        victim->sendln("*** You lose one strength point ***");
        dc_sprintf(log_buf, "%s lost a str too. ouch.", qPrintable(victim->name()));
      }
      break;
    case 2:
      if (GET_WIS(victim) > 4)
      {
        GET_WIS(victim) -= 1;
        victim->raw_wis -= 1;
        victim->sendln("*** You lose one wisdom point ***");
        dc_sprintf(log_buf, "%s lost a wis too. ouch.", qPrintable(victim->name()));
      }
      break;
    case 3:
      GET_CON(victim) -= 1;
      victim->raw_con -= 1;
      victim->sendln("*** You lose one constitution point ***");
      dc_sprintf(log_buf, "%s lost a con too. ouch.", qPrintable(victim->name()));
      break;
    case 4:
      if (GET_INT(victim) > 4)
      {
        GET_INT(victim) -= 1;
        victim->raw_intel -= 1;
        victim->sendln("*** You lose one intelligence point ***");
        dc_sprintf(log_buf, "%s lost a qint32 too. ouch.", qPrintable(victim->name()));
      }
      break;
    case 5:
      if (GET_DEX(victim) > 4)
      {
        GET_DEX(victim) -= 1;
        victim->raw_dex -= 1;
        victim->sendln("*** You lose one dexterity point ***");
        dc_sprintf(log_buf, "%s lost a dex too. ouch.", qPrintable(victim->name()));
      }
      break;
    } // of switch
    dc_->logentry(log_buf, SERAPH, DC::LogChannel::LOG_MORTAL);
    victim->player->statmetas -= 1; // we lost a stat, so don't charge extra meta
  } // of pir's extra stat loss
}

qint32 damage_retval(CharacterPtr ch, CharacterPtr vict, qint32 value)
{
  // we need to make sure in the case of a reflect or something
  // that we are returning the death of the CH if he died

  if (ch == vict && isSet(value, ReturnValue::eVICT_DIED))
    return (value | ReturnValue::eCH_DIED);

  return value;
}

bool is_bingo(qint32 dam, qint32 weapon_type, qint32 attacktype)
{
  if (weapon_type != TYPE_UNDEFINED)
    return false;

  switch (attacktype)
  {
  case SKILL_SKEWER:
  case SKILL_EAGLE_CLAW:
  case SKILL_BACKSTAB:
  case SKILL_SONG_WHISTLE_SHARP:
  case SPELL_CREEPING_DEATH:
    if (dam == 9999999)
      return true;
    break;
  }

  return false;
}

qint32 getRealSpellDamage(CharacterPtr ch)
{
  qint32 spell_dam;
  switch (GET_CLASS(ch))
  {
  case CLASS_MAGE:
    spell_dam = (GET_SPELLDAMAGE(ch) + 30);
    break;
  case CLASS_ANTI_PAL:
  case CLASS_BARD:
  case CLASS_RANGER:
    spell_dam = (GET_SPELLDAMAGE(ch) + int_app[GET_INT(ch)].spell_dam_bonus);
    break;
  case CLASS_CLERIC:
    spell_dam = (GET_SPELLDAMAGE(ch) + 25);
    break;
  case CLASS_PALADIN:
  case CLASS_MONK:
  case CLASS_DRUID:
    spell_dam = (GET_SPELLDAMAGE(ch) + wis_app[GET_WIS(ch)].spell_dam_bonus + 20);
    break;
  default:
    spell_dam = GET_SPELLDAMAGE(ch);
    break;
  }
  return spell_dam;
}

qint32 Character::getHP(void)
{
  return hit;
}

void Character::setHP(qint32 dam, CharacterPtr causer)
{
  qint32 old_hp = hit;
  hit = dam;

  if (causer)
  {
    causer->damages++;
    causer->damage_done += old_hp - dam;
    causer->damage_per_second = causer->damage_done / causer->damages;
  }
}

void Character::removeHP(qint32 dam, CharacterPtr causer)
{
  hit -= dam;

  if (causer)
  {
    causer->damages++;
    causer->damage_done += dam;
    causer->damage_per_second = causer->damage_done / causer->damages;
  }
}

void Character::fillHP(void)
{
  setHP(max_hit);
}

void Character::fillHPLimit(void)
{
  setHP(hit_limit(this));
}

void Character::addHP(qint32 newhp, CharacterPtr causer)
{
  hit += newhp;
  if (hit > max_hit)
  {
    hit = max_hit;
  }
}

// returns standard returnvals.h return codes
qint32 damage(CharacterPtr ch, CharacterPtr victim, qint32 dam, qint32 weapon_type, qint32 attacktype, qint32 weapon, bool is_death_prog, ObjectPtr obj)
{
  qint32 can_miss = 1;
  qint32 weapon_bit;
  ObjectPtr wielded;
  qint32 typeofdamage;
  qint32 damage_type(qint32 weapon_type);
  qint32 get_weapon_bit(qint32 weapon_type);
  qint32 hit_limit(CharacterPtr ch);
  qint32 retval = {};
  qint32 modifier = {};
  qint32 percent;
  qint32 learned;
  qint32 ethereal = {};
  bool reflected = false;
  QString buf, buf2, buf3;

  bool bingo;
  if (is_bingo(dam, weapon_type, attacktype))
  {
    bingo = true;
  }
  else
  {
    bingo = false;
  }

  SET_BIT(retval, ReturnValue::eSUCCESS);
  weapon_bit = get_weapon_bit(weapon_type);
  typeofdamage = damage_type(weapon_type);
  follow_type *fol;
  if (attacktype == SKILL_FLAMESLASH)
    weapon_bit = TYPE_FIRE;

  if (GET_POS(victim) == position_t::DEAD)
    return (ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED);
  if (ch->in_room != victim->in_room && !(attacktype == SPELL_SOLAR_GATE ||
                                          attacktype == SKILL_ARCHERY || attacktype == SPELL_LIGHTNING_BOLT ||
                                          attacktype == SKILL_FIRE_ARROW || attacktype == SKILL_TEMPEST_ARROW ||
                                          attacktype == SKILL_GRANITE_ARROW || attacktype == SKILL_ICE_ARROW ||
                                          attacktype == SPELL_POISON))
    return debug_retval(ch, victim, retval);
  qint32 l = {};
  if (dam != 0 && attacktype && attacktype < TYPE_HIT && attacktype != TYPE_UNDEFINED && attacktype != SKILL_FLAMESLASH)
  { // Skill damages based on learned %
    l = ch->has_skill(attacktype);
    if (ch->isNonPlayer())
      l = 50;
    if (ch->isNonPlayer() && ch->master)
      l *= (ch->master->getLevel() / 50);
    //   if (l || ch->isPlayer())
    if (weapon && attacktype <= MAX_SPL_LIST)
    {
      l = 70;
      dam /= 2;
    } // weapon spell
    dam = dam_percent(l, dam);
    dam = dc_->number(dam - (dam / 10), dam + (dam / 10)); // +- 10%
    if (ch->isNonPlayer())
      dam = (qint32)(dam * 0.6);
  }

  if (!weapon)
    weapon = WEAR_WIELD;

  // here goes le elemental stuff

  extern qint32 elemental_damage_bonus(qint32 spell, CharacterPtr ch);

  if (attacktype && attacktype < MAX_SPL_LIST && attacktype != TYPE_UNDEFINED)
    dam += elemental_damage_bonus(attacktype, ch);
  //

  if (isSet(victim->combat, COMBAT_REPELANCE) && !bingo &&
      attacktype <= MAX_SPL_LIST)
  {
    if (ch->getLevel() > 70)
      victim->sendln("The power of the spell bursts through your mental barriers as if they weren't there!");
    else if (!(ch->dc_->number(0, 9)))
      victim->sendln("Your mental shields cannot hold back the force of the spell!");
    else
    {
      if (reflected)
      {
        act_to_victim("You dissolve the reflected spell into nothingness by your will.", ch, 0, victim, 0);
        act_to_room("$n's reacts quickly and dissolves the reflected spell into formless mana.", ch, 0, victim, NOTVICT);
      }
      else
      {
        act_to_victim("$n's spell is dissolved into nothingness by your will.", ch, 0, victim, 0);
        act_to_character("$N's supreme will dissolves your spell into formless mana.", ch, 0, victim, 0);
        act_to_room("$n's spell streaks at $N and suddenly ceases to be.", ch, 0, victim, NOTVICT);
      }
      REMOVE_BIT(victim->combat, COMBAT_REPELANCE);
      return debug_retval(ch, victim, retval);
    }
    REMOVE_BIT(victim->combat, COMBAT_REPELANCE);
    //  }
  }
  bool imm = false;
  if (isSet(victim->immune, weapon_bit))
    imm = true;

  if (attacktype < MAX_SPL_LIST && ch && dam > 1)
  {
    if (attacktype != SPELL_MAGIC_MISSILE &&
        attacktype != SPELL_BEE_STING &&
        attacktype != SPELL_BLUE_BIRD && // Handled separately in magic.cpp
        attacktype != SPELL_LIGHTNING_SHIELD &&
        attacktype != SPELL_FIRESHIELD &&
        attacktype != SPELL_ACID_SHIELD

    )
      dam += getRealSpellDamage(ch);
  }

  learned = victim->has_skill(SKILL_MAGIC_RESIST);
  qint32 save = {};
  if (!imm)
    switch (weapon_type)
    {
    case TYPE_FIRE:
      save = get_saves(victim, SAVE_TYPE_FIRE);
      dc_sprintf(buf, "$B$4fire$R and sustain");
      if (learned)
        victim->skill_increase_check(SKILL_MAGIC_RESIST, learned, SKILL_INCREASE_MEDIUM);
      break;
    case TYPE_COLD:
      save = get_saves(victim, SAVE_TYPE_COLD);
      dc_sprintf(buf, "$B$3cold$R and sustain");
      if (learned)
        victim->skill_increase_check(SKILL_MAGIC_RESIST, learned, SKILL_INCREASE_MEDIUM);
      break;
    case TYPE_ENERGY:
      save = get_saves(victim, SAVE_TYPE_ENERGY);
      dc_sprintf(buf, "$B$5energy$R and sustain");
      if (learned)
        victim->skill_increase_check(SKILL_MAGIC_RESIST, learned, SKILL_INCREASE_MEDIUM);
      break;
    case TYPE_ACID:
      save = get_saves(victim, SAVE_TYPE_ACID);
      dc_sprintf(buf, "$B$2acid$R and sustain");
      if (learned)
        victim->skill_increase_check(SKILL_MAGIC_RESIST, learned, SKILL_INCREASE_MEDIUM);
      break;
    case TYPE_MAGIC:
      save = get_saves(victim, SAVE_TYPE_MAGIC);
      dc_sprintf(buf, "$B$7magic$R and sustain");
      break;
    case TYPE_POISON:
      save = get_saves(victim, SAVE_TYPE_POISON);
      dc_sprintf(buf, "$2poison$R and sustain");
      if (learned)
        victim->skill_increase_check(SKILL_MAGIC_RESIST, learned, SKILL_INCREASE_HARD);
      break;
    default:
      break;
    }
  /* qint32 v = {};
  if (GET_CLASS(ch) == CLASS_MAGIC_USER) {
  //spellcraft
    v = ch->has_skill( SKILL_SPELLCRAFT);
     if (v) {
        if (save && dam!=0 && attacktype && attacktype < TYPE_HIT)
          save -= GET_INT(ch) / 3;
     }
   }*/

  if (save < 0 && !imm)
  {
    double mult = 0 - save; // Turns positive.
    mult = 1.0 + (double)mult / 100;
    dam = (qint32)(dam * mult);
    if (reflected)
    {
      dc_strcpy(buf3, buf);
      dc_sprintf(buf2, "s additional damage.");
      dc_strcat(buf, buf2);
      dc_sprintf(buf2, "%s is susceptible to the reflected ", qPrintable(ch->shortdesc_or_name()));
      dc_strcat(buf2, buf);
      act_to_room(buf2, ch, 0, victim, NOTVICT);
      dc_sprintf(buf2, " additional damage.");
      dc_strcat(buf3, buf2);
      dc_sprintf(buf2, "You are susceptible to the reflected ");
      dc_strcat(buf2, buf3);
      act_to_character(buf2, ch, 0, victim, 0);
    }
    else
    {
      dc_strcpy(buf3, buf);
      dc_sprintf(buf2, "s additional damage.");
      dc_strcat(buf, buf2);
      dc_sprintf(buf2, " additional damage.");
      dc_strcat(buf3, buf2);
      dc_sprintf(buf2, "%s is susceptible to %s's ", qPrintable(victim->shortdesc_or_name()), qPrintable(ch->shortdesc_or_name()));
      dc_strcat(buf2, buf);
      act_to_room(buf2, victim, 0, ch, NOTVICT);
      dc_sprintf(buf2, "%s is susceptible to your ", qPrintable(victim->shortdesc_or_name()));
      dc_strcat(buf2, buf);
      act_to_victim(buf2, victim, 0, ch, 0);
      dc_sprintf(buf2, "You are susceptible to %s's ", qPrintable(ch->shortdesc_or_name()));
      dc_strcat(buf2, buf3);
      act_to_character(buf2, victim, 0, ch, 0);
    }
  }
  else if (ch->dc_->number(1, 101) < save && !imm)
  {
    if (save > 50)
      save = 50;
    dam -= (qint32)(dam * (double)save / 100); // Save chance.
    if (reflected)
    {
      dc_strcpy(buf3, buf);
      dc_sprintf(buf2, "s reduced damage.");
      dc_strcat(buf, buf2);
      dc_sprintf(buf2, "%s resists the reflected ", qPrintable(ch->shortdesc_or_name()));
      dc_strcat(buf2, buf);
      act_to_room(buf2, ch, 0, victim, NOTVICT);
      dc_sprintf(buf2, " reduced damage.");
      dc_strcat(buf3, buf2);
      dc_sprintf(buf2, "You resist the reflected ");
      dc_strcat(buf2, buf3);
      act_to_character(buf2, ch, 0, victim, 0);
    }
    else
    {
      dc_strcpy(buf3, buf);
      dc_sprintf(buf2, "s reduced damage.");
      dc_strcat(buf, buf2);
      dc_sprintf(buf2, " reduced damage.");
      dc_strcat(buf3, buf2);
      dc_sprintf(buf2, "%s resists %s's ", qPrintable(victim->shortdesc_or_name()), qPrintable(ch->shortdesc_or_name()));
      dc_strcat(buf2, buf);
      act_to_room(buf2, victim, 0, ch, NOTVICT);
      dc_sprintf(buf2, "%s resists your ", qPrintable(victim->shortdesc_or_name()));
      dc_strcat(buf2, buf);
      act_to_victim(buf2, victim, 0, ch, 0);
      dc_sprintf(buf2, "You resist %s's ", qPrintable(ch->shortdesc_or_name()));
      dc_strcat(buf2, buf3);
      act_to_character(buf2, victim, 0, ch, 0);
    }
    //        act_to_room("$n resists $N's assault and sustains reduced damage.", victim, 0, ch,  NOTVICT);
    //        act("$n resists your assault and sustains reduced damage.", victim,0,ch, TO_VICT, 0);
    //        act_to_character("You resist $N's assault and sustain reduced damage.", victim, 0, ch,  0);
  }
  /*
  if (v) { // spellcraft damage bonus
  qint32 o = {};
     switch (attacktype)
     {
  case SPELL_BURNING_HANDS: o = 10; break;
  case SPELL_LIGHTNING_BOLT: o = 20; break;
  case SPELL_CHILL_TOUCH: o = 30; break;
  case SPELL_FIREBALL: o = 40; break;
  case SPELL_METEOR_SWARM: o = 50; break;
  case SPELL_HELLSTREAM: o = 90; break;
  default: break;
     }
     if (v > o && ch->has_skill( attacktype) > o) dam += v;
  }
*/

  if (ch->affected_by_spell(SKILL_SONG_MKING_CHARGE))
  {
    dam = (qint32)(dam * 1.2); // scary!
    SET_BIT(modifier, COMBAT_MOD_FRENZY);
  }

  // Can't hurt god, but he likes to see the messages.
  if (victim->getLevel() >= IMMORTAL && victim->isPlayer())
    dam = {};

  if (victim != ch)
  {
    if (!can_attack(ch) || !can_be_attacked(ch, victim))
      return ReturnValue::eFAILURE;
    set_cantquit(ch, victim);
  }

  /* An eye for an eye, a tooth for a tooth, a life for a life. */
  if (GET_POS(victim) > position_t::STUNNED && ch != victim)
  {
    if (!victim->fighting && ch->in_room == victim->in_room)
      set_fighting(victim, ch);

    if ((!isSet(victim->combat, COMBAT_STUNNED)) &&
        (!isSet(victim->combat, COMBAT_STUNNED2)) &&
        (!isSet(victim->combat, COMBAT_BASH1)) &&
        (!isSet(victim->combat, COMBAT_BASH2)))
    {
      if (GET_POS(victim) > position_t::STUNNED)
      {
        if (GET_POS(victim) < position_t::FIGHTING)
        {
          act_to_room("$n scrambles to $s feet!", victim, 0, 0, 0);
          act_to_character("You scramble to your feet!", victim, 0, 0, 0);
          victim->setPOSFighting();
        }
      }
    }
  }
  else if (GET_POS(victim) == position_t::SLEEPING)
  {
    affect_from_char(victim, INTERNAL_SLEEPING);
    act_to_room("$n is shocked to a wakened state and scrambles to $s feet!", victim, 0, 0, 0);
    ch->send("You are awakened from combat adrenaline springing you to your feet!");
    victim->setPOSFighting();
  }

  if (GET_POS(ch) == position_t::FIGHTING &&
      !ch->fighting)
  { // fix for fighting thin air thing related to poison
    ch->setStanding();
    ;
  }
  if (GET_POS(ch) > position_t::STUNNED && ch != victim)
  {
    if (!ch->fighting && ch->in_room == victim->in_room)
      set_fighting(ch, victim);
  }

  if (IS_AFFECTED(ch, AFF_INVISIBLE) && (!IS_AFFECTED(ch, AFF_ILLUSION) || !(ch->affected_by_spell(BASE_TIMERS + SPELL_INVISIBLE) &&
                                                                             ch->affected_by_spell(SPELL_INVISIBLE) && ch->affected_by_spell(SPELL_INVISIBLE)->modifier == 987)))
  {
    act_to_room("$n slowly fades into existence.", ch, 0, 0, 0);
    // if (ch->affected_by_spell(SPELL_INVISIBLE))
    //  no point it looping through the list twice...
    affect_from_char(ch, SPELL_INVISIBLE);
    REMBIT(ch->affected_by, AFF_INVISIBLE);
  }

  // Frost Shield won't effect a backstab
  if (attacktype != SKILL_BACKSTAB && victim->getHP() > 0 &&
      (typeofdamage == DAMAGE_TYPE_PHYSICAL || attacktype == TYPE_PHYSICAL_MAGIC))
    if (do_frostshield(ch, victim))
    {
      return debug_retval(ch, victim, retval) | ReturnValue::eEXTRA_VALUE;
      ;
    }

  if (typeofdamage == DAMAGE_TYPE_PHYSICAL)
  {
    if (isSet(ch->combat, COMBAT_BERSERK))
      dam = (qint32)(dam * 1.75);
    if (IS_AFFECTED(ch, AFF_PRIMAL_FURY))
      dam = dam * 5;
    if (isSet(ch->combat, COMBAT_RAGE1) || (isSet(ch->combat, COMBAT_RAGE2) && attacktype != SKILL_BACKSTAB))
      dam = (qint32)(dam * 1.4);
    if (isSet(ch->combat, COMBAT_HITALL))
      dam = (qint32)(dam * 2);
    if (isSet(ch->combat, COMBAT_ORC_BLOODLUST1))
    {
      dam = (qint32)(dam * 1.7);
      //      REMOVE_BIT(ch->combat, COMBAT_ORC_BLOODLUST1);
      //      SET_BIT(ch->combat, COMBAT_ORC_BLOODLUST2);
    }
    if (isSet(ch->combat, COMBAT_ORC_BLOODLUST2))
    {
      dam = (qint32)(dam * 1.7);
      //      REMOVE_BIT(ch->combat, COMBAT_ORC_BLOODLUST2);
    }
    percent = (qint32)((((qreal)ch->getHP()) / ((qreal)GET_MAX_HIT(ch))) * 100);
    if (percent < 40 && (learned = ch->has_skill(SKILL_FRENZY)))
    {
      if (skill_success(ch, victim, SKILL_FRENZY))
      {
        dam = (qint32)(dam * 1.2);
        SET_BIT(modifier, COMBAT_MOD_FRENZY);
      }
    }
    if (isSet(ch->combat, COMBAT_VITAL_STRIKE))
      dam = (qint32)(dam * 2.5);

    // we do this AFTER all the multipliers but BEFORE all the reducers
    // to make it cause the smallest impact
    if (dam) // misses turned to tickles
      dam = (dam * (100 - victim->melee_mitigation)) / 100;

    if (victim->affected_by_spell(SPELL_HOLY_AURA) && victim->affected_by_spell(SPELL_HOLY_AURA)->modifier == 50)
      dam /= 2; // half damage against physical attacks
  }
  else
  {
    if (victim->affected_by_spell(SPELL_HOLY_AURA) && victim->affected_by_spell(SPELL_HOLY_AURA)->modifier == 25)
      dam /= 2;
  }
  if (typeofdamage == DAMAGE_TYPE_MAGIC && dam)
    dam = (dam * (100 - victim->spell_mitigation)) / 100;
  else if (typeofdamage == DAMAGE_TYPE_SONG && dam)
    dam = (dam * (100 - victim->song_mitigation)) / 100;

  if (IS_AFFECTED(victim, AFF_EAS))
    dam /= 4;

  // sanct damage now 35% for all caster aligns
  if (IS_AFFECTED(victim, AFF_SANCTUARY) && dam > 0)
  {
    qint32 mod = victim->affected_by_spell(SPELL_SANCTUARY) ? victim->affected_by_spell(SPELL_SANCTUARY)->modifier : 35;
    dam -= (qint32)(qreal)((qreal)dam * ((qreal)mod / 100.0));
  }
  if (isSet(victim->combat, COMBAT_MONK_STANCE) && dam > 0) // half damage
    dam /= 2;
  qint32 reduce = 0, type = {};
  if (can_miss == 1)
  {
    if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) || attacktype == SKILL_FLAMESLASH)
    {
      qint32 retval2 = {};

      retval2 = isHit(ch, victim, attacktype, type, reduce);
      if (SOMEONE_DIED(retval2)) // Riposte
        return damage_retval(ch, victim, retval2);

      if (isSet(retval2, ReturnValue::eSUCCESS))
      {
        switch (type)
        {
        case 0:
          return ReturnValue::eFAILURE; // Dodge or parry

        case 4:
        case 1:
        case 2:
          SET_BIT(modifier, COMBAT_MOD_REDUCED);
          dam -= (qint32)((qreal)dam * ((qreal)reduce / 100));
          break;
          // Shield block or Mdefense or tumbling partial avoid

        case 3:
          dam = {}; // Miss!
          break;
        default:
          return ReturnValue::eFAILURE; // Shouldn't happen
        }
      }

      /* Old Hitcode!
    if(check_parry(ch, victim, attacktype)) {
      if(typeofdamage == DAMAGE_TYPE_PHYSICAL)
      {
        return damage_retval(ch, victim, check_riposte(ch, victim, attacktype));
      }
    }
    if (check_dodge(ch, victim, attacktype))
      return ReturnValue::eFAILURE;

    if((reduce = check_shieldblock(ch, victim, attacktype)))
     {
  SET_BIT(modifier, COMBAT_MOD_REDUCED);
  dam -= (qint32)((qreal) dam * ((qreal)reduce/100));
     }
   */

      if ((learned = ch->has_skill(SKILL_CRIT_HIT)) && dam)
        if (ch->dc_->number(1, 101) <= learned / 10 + GET_DEX(ch) - GET_DEX(victim))
        {
          dam += (qint32)(dam * (qreal)(2 + learned / 5) / 100);
          act_to_character("Your strike at $N lands with lethal accuracy and inflicts additional damage!", ch, 0, victim, 0);
          act_to_victim("$n's strike lands with lethal accuracy and inflicts additional damage!", ch, 0, victim, 0);
          act_to_room("$n's strike at $N lands with lethal accuracy and inflicts additional damage!", ch, 0, victim, NOTVICT);
          ch->skill_increase_check(SKILL_CRIT_HIT, learned, SKILL_INCREASE_HARD);
        }
    }
    /* Never heard of it.
  if (attacktype == TYPE_PHYSICAL_MAGIC)
  {
    // Physical Magic can be dodged or blocked with a shield, but not parried
    if(check_shieldblock(ch, victim,attacktype))
      return ReturnValue::eFAILURE;
    if (check_dodge(ch, victim,attacktype))
      return ReturnValue::eFAILURE;
  }*/
    if (attacktype <= MAX_SPL_LIST && attacktype != TYPE_UNDEFINED)
    {
      qint32 reduce = {};
      if ((reduce = check_magic_block(ch, victim, attacktype)))
      {
        if (GET_CLASS(victim) != CLASS_MONK && victim->isPlayer())
        {
          if (ch->dc_->number(1, 100) <= MAX(1, dam / 150))
          {
            qint32 eqdam = damage_eq_once(victim->equipment[WEAR_SHIELD]);
            if (victim->equipment[WEAR_SHIELD])
            {
              if (dc_->obj_index[victim->equipment[WEAR_SHIELD]->item_number].progtypes & ARMOUR_PROG)
                victim->oprog_armour_trigger(victim->equipment[WEAR_SHIELD]);
              if (eqdam >= eq_max_damage(victim->equipment[WEAR_SHIELD]))
                eq_destroyed(victim, victim->equipment[WEAR_SHIELD], WEAR_SHIELD);
              else
              {
                act_to_character("$p is damaged by the force of the spell!", victim, victim->equipment[WEAR_SHIELD], 0, 0);
                act_to_room("$p worn by $n is damaged by the force of the spell!", victim, victim->equipment[WEAR_SHIELD], 0, 0);
              }
            }
          }
        }
        dam -= (qint32)((qreal)dam * ((qreal)reduce / 100));
      }
    } // spellblock

  } // can_miss

  qint32 pre_stoneshield_dam = {};
  std::stringstream string1;
  affected_type *pspell = {};
  if (!victim->isImmortalPlayer() && dam > 0 && typeofdamage == DAMAGE_TYPE_PHYSICAL &&
      ((pspell = victim->affected_by_spell(SPELL_STONE_SHIELD)) ||
       (pspell = victim->affected_by_spell(SPELL_GREATER_STONE_SHIELD))))
  {
    pre_stoneshield_dam = dam;
    if (dam > pspell->modifier)
    {
      dam -= pspell->modifier;
      pspell->duration -= pspell->modifier;
      victim->send(u"Your stones absorb %d damage allowing %d through.\r\n"_s.arg(pspell->modifier).arg(dam));
      string1 << "Your attack hits $N's stones for " << pspell->modifier << " damage allowing " << dam << " through.";
      act_to_character(string1.str().c_str(), ch, 0, victim, 0);
      string1.clear();
      string1.str("");
      string1 << "$n's attack hits $N's stones for " << pspell->modifier << " damage allowing " << dam << " through.";
      act_to_room(string1.str().c_str(), ch, 0, victim, NOTVICT);
    }
    else
    {
      pspell->duration -= dam;
      victim->send(u"Your stones absorb %1 damage from the attack and change its direction slightly.\r\n"_s.arg(dam));
      string1 << "$N's stones absorb " << dam << " damage of your attack and cause your blow to change direction slightly.";
      act_to_character(string1.str().c_str(), ch, 0, victim, 0);
      string1.clear();
      string1.str("");
      string1 << "$N's stones completely absorbed $n's attack of " << dam << " damage changing its direction slightly.";
      act_to_room(string1.str().c_str(), ch, 0, victim, NOTVICT);
      dam = 1;
    }

    if (0 >= pspell->duration)
    {
      ethereal = 1;
      affect_from_char(victim, SPELL_STONE_SHIELD);
      affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);
    }
  }

  if (dam < 0)
    dam = {};

  // Immune / Susceptibilities / Resistances
  // Some code is in saves_spell  (for obvious reasons)
  weapon_bit = get_weapon_bit(weapon_type);

  if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) ||
      (attacktype == SKILL_BACKSTAB))
  {
    if (isSet(victim->immune, ISR_PHYSICAL))
      weapon_bit += ISR_PHYSICAL;
    else if (isSet(victim->resist, ISR_PHYSICAL))
      weapon_bit += ISR_PHYSICAL;
    else if (isSet(victim->suscept, ISR_PHYSICAL))
      weapon_bit += ISR_PHYSICAL;

    wielded = ch->equipment[weapon];

    if (wielded)
    {
      //      if ((isSet(victim->immune, ISR_MAGIC)) &&
      //      (isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
      //    weapon_bit += ISR_MAGIC;
      if ((isSet(victim->immune, ISR_NON_MAGIC)) &&
          (!isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)))
        weapon_bit += ISR_NON_MAGIC;
      //      if ((isSet(victim->suscept, ISR_MAGIC)) &&
      //        (isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
      //        weapon_bit += ISR_MAGIC;
      if ((isSet(victim->suscept, ISR_NON_MAGIC)) &&
          (!isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)))
        weapon_bit += ISR_NON_MAGIC;
      //      if ((isSet(victim->resist, ISR_MAGIC)) &&
      //       (isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
      //       weapon_bit += ISR_MAGIC;
      if ((isSet(victim->resist, ISR_NON_MAGIC)) &&
          (!isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)))
        weapon_bit += ISR_NON_MAGIC;
    }
  }

  if (isSet(victim->immune, weapon_bit))
  {
    dam = {};
    SET_BIT(retval, ReturnValue::eIMMUNE_VICTIM);
    if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) || attacktype == SKILL_FLAMESLASH)
    {
      SET_BIT(modifier, COMBAT_MOD_IGNORE);
      SET_BIT(retval, ReturnValue::eEXTRA_VALUE);
    }
  }
  else if (isSet(victim->suscept, weapon_bit))
  {
    //    magic stuff is handled elsewhere
    if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) || attacktype == SKILL_FLAMESLASH)
    {
      dam = (qint32)(dam * 1.3);
      SET_BIT(modifier, COMBAT_MOD_SUSCEPT);
    }
  }
  else if (isSet(victim->resist, weapon_bit))
  {
    //    magic stuff is handled elsewhere
    if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) || attacktype == SKILL_FLAMESLASH)
    {
      dam = (qint32)(dam * 0.7);
      SET_BIT(modifier, COMBAT_MOD_RESIST);
    }
  }
  if (dam < 0)
    dam = {};

  percent = (qint32)((((qreal)victim->getHP()) /
                      ((qreal)GET_MAX_HIT(victim))) *
                     100);
  if (percent < 40 && (learned = victim->has_skill(SKILL_FRENZY)))
  {
    switch (attacktype)
    {
    case SPELL_BURNING_HANDS:
    case SPELL_DROWN:
    case SPELL_CAUSE_SERIOUS:
    case SPELL_ACID_BLAST:
    case SPELL_FIREBALL:
    case SPELL_LIGHTNING_BOLT:
    case SPELL_VAMPIRIC_TOUCH:
    case SPELL_METEOR_SWARM:
    case SPELL_CALL_LIGHTNING:
    case SPELL_CHILL_TOUCH:
    case SPELL_COLOUR_SPRAY:
    case SPELL_CAUSE_LIGHT:
    case SPELL_CAUSE_CRITICAL:
    case SPELL_SPARKS:
    case SPELL_FLAMESTRIKE:
    case SPELL_DISPEL_EVIL:
    case SPELL_DISPEL_GOOD:
    case SPELL_HELLSTREAM:
    case SPELL_HARM:
    case SPELL_POWER_HARM:
    case SPELL_MAGIC_MISSILE:
    case SPELL_BLUE_BIRD:
    case SPELL_SHOCKING_GRASP:
    case SPELL_SUN_RAY:
    case SPELL_BEE_STING:
    case SPELL_DIVINE_FURY:
      if (ch->dc_->number(1, 100) <= (learned / 2))
      {
        act("In your frenzied state you shake off some of the affects of "
            "$n's magical attack!",
            ch, 0, victim, TO_VICT, 0);
        act("In $k frenzied state, $N shakes off some of the damage of "
            "your spell!",
            ch, 0, victim, TO_CHAR, 0);
        act("In $k frenzied state, $N shakes off some of the damage of "
            "$n's spell!",
            ch, 0, victim, TO_ROOM, NOTVICT);

        dam = (qint32)(dam * (double)(ch->dc_->number(45, 55) / 100.0));
      }
      break;
    }
  }

  if (dam > 0 && victim->affected_by_spell(SPELL_DIVINE_INTER) && dam > victim->affected_by_spell(SPELL_DIVINE_INTER)->modifier)
    dam = victim->affected_by_spell(SPELL_DIVINE_INTER)->modifier;

  // Check for parry, mob disarm, and trip. Print a suitable damage message.
  if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) || (ch->isNonPlayer() && dc_->mob_index[ch->mobdata->nr].vnum() > 87 && dc_->mob_index[ch->mobdata->nr].vnum() < 92) || attacktype == SKILL_FLAMESLASH)
  {
    if (ch->equipment[weapon] == nullptr)
    {
      dam_message(dam, ch, victim, TYPE_HIT, modifier);
    }
    else
    {
      dam_message(dam, ch, victim, attacktype, modifier);
    }

    victim->removeHP(dam, ch);
    update_pos(victim);
  }
  else
  {
    affected_type *af;
    if (dam >= 350 && (af = victim->affected_by_spell(SPELL_PARALYZE)) && victim->isPlayer())
    {
      act_to_victim("The overpowering magic from $n's spell disrupts the paralysis surrounding you!", ch, 0, victim, 0);
      act_to_character("The powerful magic from your spell has disrupted the paralysis surrounding $N!", ch, 0, victim, 0);
      act_to_room("The powerful magic of $n's spell has disrupted the paralysis surrounding $N!", ch, 0, victim, NOTVICT);
      affect_remove(victim, af, 0);
    }

    victim->removeHP(dam, ch);
    update_pos(victim);
    do_dam_msgs(ch, victim, dam, attacktype, weapon, weapon_type);
  }

  mprog_damage_trigger(victim, ch, dam);

  if (ethereal)
  {
    victim->sendln("The ethereal stones protecting you shatter and fade into nothing.");
    act_to_room("The ethereal stones surrounding $n shatter into nothingness.\r\n", victim, 0, 0, 0);
  }

  /*  Now for eq damage...   */
  if (dam > 25 && typeofdamage == DAMAGE_TYPE_PHYSICAL)
    eq_damage(ch, victim, dam, weapon_type, attacktype);

  inform_victim(ch, victim, dam);

  if (GET_POS(victim) != position_t::DEAD && ch->in_room != victim->in_room &&
      !(attacktype == SPELL_SOLAR_GATE || attacktype == SKILL_ARCHERY ||
        attacktype == SPELL_LIGHTNING_BOLT || attacktype == SKILL_FIRE_ARROW ||
        attacktype == SKILL_ICE_ARROW || attacktype == SKILL_TEMPEST_ARROW ||
        attacktype == SKILL_GRANITE_ARROW || attacktype == SPELL_POISON)) // Wimpy
    return debug_retval(ch, victim, retval);

  if (typeofdamage == DAMAGE_TYPE_PHYSICAL && type == 1 && reduce > 0 && dam > 0 && ch != victim)
  { // Shieldblock riposte..
    qint32 retval2 = check_riposte(ch, victim, attacktype);
    if (SOMEONE_DIED(retval2))
      return damage_retval(ch, victim, retval2);
  }

  if (typeofdamage == DAMAGE_TYPE_PHYSICAL && type == 2 && reduce > 0 && dam > 0 && ch != victim)
  { // Martial Defense Counter Strike..
    qint32 retval2 = checkCounterStrike(ch, victim);
    if (SOMEONE_DIED(retval2))
      return damage_retval(ch, victim, retval2);
  }

  if (typeofdamage == DAMAGE_TYPE_PHYSICAL && dam > 0 && ch != victim && attacktype != SKILL_ARCHERY && !is_death_prog)
  {
    qint32 retval2;
    retval2 = do_fireshield(ch, victim, MAX(pre_stoneshield_dam, dam));
    if (SOMEONE_DIED(retval2))
      return damage_retval(ch, victim, retval2);
    retval2 = do_acidshield(ch, victim, MAX(pre_stoneshield_dam, dam));
    if (SOMEONE_DIED(retval2))
      return damage_retval(ch, victim, retval2);
    retval2 = do_lightning_shield(ch, victim, MAX(pre_stoneshield_dam, dam));
    if (SOMEONE_DIED(retval2))
      return damage_retval(ch, victim, retval2);
    retval2 = do_vampiric_aura(ch, victim);
    if (SOMEONE_DIED(retval2))
      return damage_retval(ch, victim, retval2);
    retval2 = do_boneshield(ch, victim, dam);
    if (SOMEONE_DIED(retval2))
      return damage_retval(ch, victim, retval2);
  }

  // Sleep spells.
  if (!isSet(victim->combat, COMBAT_STUNNED) &&
      !isSet(victim->combat, COMBAT_STUNNED2))
    if (!AWAKE(victim))
    {
      if (victim->fighting)
        stop_fighting(victim);

      if (isSet(victim->combat, COMBAT_BERSERK))
        REMOVE_BIT(victim->combat, COMBAT_BERSERK);
      if (isSet(victim->combat, COMBAT_RAGE1))
        REMOVE_BIT(victim->combat, COMBAT_RAGE1);
      if (isSet(victim->combat, COMBAT_RAGE2))
        REMOVE_BIT(victim->combat, COMBAT_RAGE2);
    }

  // Payoff for killing things.
  if (GET_POS(victim) == position_t::DEAD)
  {
    if (attacktype == SKILL_EAGLE_CLAW)
      make_heart(ch, victim);
    group_gain(ch, victim);
    if (attacktype == SPELL_POISON)
      fight_kill(ch, victim, TYPE_CHOOSE, KILL_POISON);
    else
    {
      if (bingo)
        fight_kill(ch, victim, TYPE_CHOOSE, KILL_BINGO);
      else
        fight_kill(ch, victim, TYPE_CHOOSE, 0);
    }
    return damage_retval(ch, victim, (ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED));
  }
  else
  {

    if (ch->in_room == victim->in_room && attacktype != SKILL_CIRCLE)
    {
      SET_BIT(retval, check_autojoiners(ch, 1));
      if (!SOMEONE_DIED(retval))
        if (IS_AFFECTED(ch, AFF_CHARM))
          SET_BIT(retval, check_joincharmie(ch));
      if (SOMEONE_DIED(retval))
        return debug_retval(ch, victim, retval);
      if (ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_CHARMIEJOIN) && attacktype != SKILL_AMBUSH)
      {
        if (ch->followers)
        {
          follow_type *folnext;
          for (fol = ch->followers; fol; fol = folnext)
          {
            folnext = fol->next;
            if (IS_AFFECTED(fol->follower, AFF_CHARM) && ch->in_room == fol->follower->in_room)
              SET_BIT(retval, check_charmiejoin(fol->follower));
            if (isSet(retval, ReturnValue::eVICT_DIED))
              break;
          }
        }
      }
      if (SOMEONE_DIED(retval))
        return debug_retval(ch, victim, retval);
    }
  }
  return debug_retval(ch, victim, retval);
}

// this function deals damage in noncombat situations (falls, drowning, etc.)
// returns standard returnvals.h return codes
qint32 noncombat_damage(CharacterPtr ch, qint32 dam, const QString char_death_msg, const QString room_death_msg, const QString death_log_msg, qint32 type)
{
  qint32 kill_type = TYPE_CHOOSE;

  if (IS_AFFECTED(ch, AFF_EAS))
    dam /= 4;
  if (IS_AFFECTED(ch, AFF_SANCTUARY))
  {
    qint32 mod = ch->affected_by_spell(SPELL_SANCTUARY) ? ch->affected_by_spell(SPELL_SANCTUARY)->modifier : 35;
    dam -= (qint32)(qreal)((qreal)dam * ((qreal)mod / 100.0));
  }
  if (ch->affected_by_spell(SPELL_HOLY_AURA) && ch->affected_by_spell(SPELL_HOLY_AURA)->modifier == 50 && (type == KILL_DROWN || type == KILL_FALL))
    dam /= 2;
  if (ch->affected_by_spell(SPELL_HOLY_AURA) && ch->affected_by_spell(SPELL_HOLY_AURA)->modifier == 25 && type == KILL_POISON)
    dam /= 2;
  if (ch->affected_by_spell(SPELL_DIVINE_INTER) && dam > ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier)
    dam = ch->affected_by_spell(SPELL_DIVINE_INTER)->modifier;

  ch->removeHP(dam);
  update_pos(ch);
  if (GET_POS(ch) == position_t::DEAD)
  {
    if (char_death_msg)
    {
      ch->send(char_death_msg);
      ch->sendln("\r\nYou have been KILLED!");
    }
    if (room_death_msg)
      act_to_room(room_death_msg, ch, 0, 0, 0);
    if (death_log_msg)
      dc_->logentry(death_log_msg, IMMORTAL, DC::LogChannel::LOG_MORTAL);
    if (type == KILL_BATTER)
      kill_type = TYPE_PKILL;
    fight_kill(nullptr, ch, kill_type, type);
    return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED;
  }
  else
  {
    return ReturnValue::eSUCCESS;
  }
}

bool is_pkill(CharacterPtr ch, CharacterPtr vict)
{
  CharacterPtr tmp_ch;

  // TODO - change this so a mob following another mob isn't a pkill

  if (!ch)
    return true;

  for (tmp_ch = ch; tmp_ch; tmp_ch = tmp_ch->master)
  {
    if (tmp_ch->isPlayer())
    {
      if (vict->isNonPlayer())
      {
        if (vict->master)
        { /* Attacking someone's charmie */
          if (vict->master->isPlayer())
          {
            if (vict->master != ch)
            { /* Can't pkill your own charmie */
              return true;
            }
            return false; /* Standard mob kill */
          }
        }
      }
      else
      { /* vict is a pc */
        return true;
      }
    }
  }

  /* Still here?  The killer is an uncharmed mob, definitely not a pkill! */
  return false;
}

void send_damage(QString buf, CharacterPtr ch, ObjectPtr obj, CharacterPtr victim, QString dmg, QString buf2, qint32 to)
{
  send_damage(qPrintable(buf), ch, obj, victim, qPrintable(dmg), qPrintable(buf2), to);
}

void send_damage(QString buf, CharacterPtr ch, ObjectPtr obj, CharacterPtr victim, QString dmg, QString buf2, qint32 to)
{
  CharacterPtr tmpch;
  QString string1, string2;

  qint32 i, z = 0, y = {};
  for (i = {}; i <= (qint32)strlen(buf); i++)
  {
    if (*(buf + i) == '|')
    {
      string1[z] = '\0';
      dc_strcat(string1, dmg);
      z += strlen(dmg);
    }
    else
    {
      string1[z++] = *(buf + i);
      string2[y++] = *(buf + i);
    }
  }
  if (buf2) // lazy, should've done it earlier, some extra processing wasted, but I don't care!
    dc_strcpy(string2, buf2);

  TokenList *tokens, *tokens2;
  tokens = new TokenList(string1);
  tokens2 = new TokenList(string2);

  if ((IS_AFFECTED(ch, AFF_HIDE) || ISSET(ch->affected_by, AFF_FOREST_MELD)) && to != TO_CHAR)
  {
    REMBIT(ch->affected_by, AFF_HIDE);
    affect_from_char(ch, SPELL_FOREST_MELD);
  }

  if (to == TO_ROOM)
  {
    for (tmpch = dc_->world[ch->in_room].people; tmpch; tmpch = tmpch->next_in_room)
    {
      if (tmpch == ch || tmpch == victim)
        continue;
      if (tmpch->isPlayer() && isSet(tmpch->player->toggles, Player::PLR_DAMAGE))
        send_tokens(tokens, ch, obj, victim, 0, tmpch);
      else
        send_tokens(tokens2, ch, obj, victim, 0, tmpch);
    }
  }
  else if (to == TO_CHAR)
  {
    if (ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_DAMAGE))
      send_tokens(tokens, ch, obj, victim, 0, ch);
    else
      send_tokens(tokens2, ch, obj, victim, 0, ch);
  }
  else if (to == TO_VICT)
  {
    if (victim->isPlayer() && isSet(victim->player->toggles, Player::PLR_DAMAGE))
      send_tokens(tokens, ch, obj, victim, 0, victim);
    else
      send_tokens(tokens2, ch, obj, victim, 0, victim);
  }
  tokens = {};
  tokens2 = {};
}

void do_dam_msgs(CharacterPtr ch, CharacterPtr victim, qint32 dam, qint32 attacktype, qint32 weapon, qint32 filter)
{
  extern message_list fight_messages[MAX_MESSAGES];
  message_type *messages, *messages2;
  qint32 i, j, nr;
  QString find, replace;

  if (is_bingo(dam, weapon, attacktype))
    return;

  if (filter > TYPE_HIT && (attacktype == SPELL_BURNING_HANDS || attacktype == SPELL_FIREBALL || attacktype == SPELL_FIRESTORM || attacktype == SPELL_HELLSTREAM || attacktype == SPELL_MAGIC_MISSILE || attacktype == SPELL_METEOR_SWARM || attacktype == SPELL_LIGHTNING_BOLT || attacktype == SPELL_CHILL_TOUCH))
  {
    if (attacktype == SPELL_CHILL_TOUCH)
      find = u"$B$3"_s;
    else if (attacktype == SPELL_LIGHTNING_BOLT)
      find = u"$B$5"_s;
    else if (attacktype == SPELL_MAGIC_MISSILE || attacktype == SPELL_METEOR_SWARM)
      find = u"$B$7"_s;
    else
      find = u"$B$4"_s;

    switch (filter)
    {
    case TYPE_FIRE:
      replace = u"$B$4"_s;
      break;
    case TYPE_COLD:
      replace = u"$B$3"_s;
      break;
    case TYPE_ENERGY:
      replace = u"$B$5"_s;
      break;
    case TYPE_ACID:
      replace = u"$B$2"_s;
      break;
    case TYPE_POISON:
      replace = u"$2"_s;
      break;
    case TYPE_MAGIC:
      replace = u"$B$7"_s;
      break;
    default:
      replace = find;
      break;
    }
  }

  for (i = {}; i < MAX_MESSAGES; i++)
  {
    if (fight_messages[i].a_type != attacktype)
      continue;

    nr = dice(1, fight_messages[i].number_of_attacks);
    j = 1;
    for (messages = fight_messages[i].msg, messages2 = fight_messages[i].msg2; j < nr && messages; j++)
    {
      messages = messages->next;
      if (messages2)
        messages2 = messages2->next;
    }
    QString dmgmsg;
    dmgmsg[0] = '\0';
    if (dam > 0)
      dc_sprintf(dmgmsg, "$B%d$R", dam);
    if (!messages)
      return;
    if (victim->isPlayer() && victim->getLevel() >= IMMORTAL)
    {
      act(replaceString(messages->god_msg.attacker_msg, find, replace),
          ch, ch->equipment[weapon], victim, TO_CHAR, 0);
      act(replaceString(messages->god_msg.victim_msg, find, replace),
          ch, ch->equipment[weapon], victim, TO_VICT, 0);
      act(replaceString(messages->god_msg.room_msg, find, replace),
          ch, ch->equipment[weapon], victim, TO_ROOM, NOTVICT);
    }
    else if (dam == 0)
    {
      act(replaceString(messages->miss_msg.attacker_msg, find, replace),
          ch, ch->equipment[weapon], victim, TO_CHAR, 0);
      act(replaceString(messages->miss_msg.victim_msg, find, replace),
          ch, ch->equipment[weapon], victim, TO_VICT, 0);
      act(replaceString(messages->miss_msg.room_msg, find, replace),
          ch, ch->equipment[weapon], victim, TO_ROOM, NOTVICT);
    }
    else if (GET_POS(victim) == position_t::DEAD)
    {
      QString victim_msg1 = replaceString(messages->die_msg.victim_msg, find, replace);
      QString victim_msg2 = replaceString(messages2->die_msg.victim_msg, find, replace);
      QString attacker_msg1 = replaceString(messages->die_msg.attacker_msg, find, replace);
      QString attacker_msg2 = replaceString(messages2->die_msg.attacker_msg, find, replace);
      QString room_msg1 = replaceString(messages->die_msg.room_msg, find, replace);
      QString room_msg2 = replaceString(messages2->die_msg.room_msg, find, replace);

      send_damage(victim_msg2, ch, ch->equipment[weapon], victim, dmgmsg, victim_msg1, TO_VICT);
      send_damage(attacker_msg2, ch, ch->equipment[weapon], victim, dmgmsg, attacker_msg1, TO_CHAR);
      send_damage(room_msg2, ch, ch->equipment[weapon], victim, dmgmsg, room_msg1, TO_ROOM);
    }
    else
    {
      QString victim_msg1 = replaceString(messages->hit_msg.victim_msg, find, replace);
      QString victim_msg2 = replaceString(messages2->hit_msg.victim_msg, find, replace);
      QString attacker_msg1 = replaceString(messages->hit_msg.attacker_msg, find, replace);
      QString attacker_msg2 = replaceString(messages2->hit_msg.attacker_msg, find, replace);
      QString room_msg1 = replaceString(messages->hit_msg.room_msg, find, replace);
      QString room_msg2 = replaceString(messages2->hit_msg.room_msg, find, replace);

      send_damage(victim_msg2, ch, ch->equipment[weapon], victim, dmgmsg, victim_msg1, TO_VICT);
      send_damage(attacker_msg2, ch, ch->equipment[weapon], victim, dmgmsg, attacker_msg1, TO_CHAR);
      send_damage(room_msg2, ch, ch->equipment[weapon], victim, dmgmsg, room_msg1, TO_ROOM);
    }
  }
}

void set_cantquit(CharacterPtr ch, CharacterPtr vict, bool forced)
{
  affected_type af, *paf;
  CharacterPtr realch;
  CharacterPtr realvict;
  qint32 ch_vnum = -1;
  qint32 vict_vnum = -1;

  if (!ch || !vict)
    return; /* This will happen if the character was in a fall room */

  if (ch == vict)
    return;

  /* can't get pkill flag in the arena! */
  if (ch->dc_->IS_ARENA(ch->in_room))
    return;

  if (ch->isNonPlayer())
    ch_vnum = dc_->mob_index[ch->mobdata->nr].vnum();

  if (vict->isNonPlayer())
    vict_vnum = dc_->mob_index[vict->mobdata->nr].vnum();

  if (ch->isNonPlayer() && (IS_AFFECTED(ch, AFF_CHARM) || IS_AFFECTED(ch, AFF_FAMILIAR) || ch_vnum == 8) && ch->master && ch->master->in_room == ch->in_room)
    realch = ch->master;
  else
    realch = ch;

  if (vict->isNonPlayer() && (IS_AFFECTED(vict, AFF_CHARM) || IS_AFFECTED(vict, AFF_FAMILIAR) || vict_vnum == 8) && vict->master)
    realvict = vict->master;
  else
    realvict = vict;

  if (realvict == realch) // killing your own pet was giving you a CQ
    return;

  if (!realch->fighting)
    SET_BIT(realch->combat, COMBAT_ATTACKER);

  if (is_pkill(realch, realvict) && !ISSET(realvict->affected_by, AFF_CANTQUIT) &&
      !realvict->isPlayerGoldThief() && !IS_AFFECTED(realvict, AFF_CHAMPION) && !IS_AFFECTED(realch, AFF_CHAMPION) &&
      !realvict->affected_by_spell(Character::PLAYER_OBJECT_THIEF) && !forced)
  {
    af.type = Character::PLAYER_CANTQUIT;
    af.duration = 5;
    af.modifier = {};
    af.location = APPLY_NONE;
    af.bitvector = AFF_CANTQUIT;
    if (realvict && qPrintable(realvict->name()))
    {
      af.caster = qPrintable(realvict->name());
    }

    if (!ISSET(realch->affected_by, AFF_CANTQUIT))
      affect_to_char(realch, &af);
    else
    {
      for (paf = realch->affected; paf; paf = paf->next)
      {
        if (paf->type == Character::PLAYER_CANTQUIT)
          paf->duration = 5;
      }
    }
  }
}

void fight_kill(CharacterPtr ch, CharacterPtr vict, qint32 type, qint32 spec_type)
{
  if (!vict)
  {
    dc_->logentry(u"Null vict sent to fight_kill()!"_s, -1, DC::LogChannel::LOG_BUG);
    return;
  }
  bool vict_is_attacker = false;
  if (isSet(vict->combat, COMBAT_ATTACKER))
    vict_is_attacker = true;

  if (vict->fighting)
    stop_fighting(vict);
  if (ch && ch->fighting && (ch->isPlayer() || ch->fighting == vict))
    stop_fighting(ch);

  // loop through world and stop anyone else that was fighting vict from fighting
  CharacterPtr ich, next_ich;
  for (ich = combat_list; ich; ich = next_ich)
  {
    next_ich = ich->next_fighting;
    if (ich->fighting == vict)
      stop_fighting(ich);
  }
  if (vict->master || vict->followers)
    stop_grouped_bards(vict, !IS_SINGING(vict));

  switch (type)
  {
  case TYPE_CHOOSE:
    /* if it's a mob then it can't be pkilled */
    if (vict->isNonPlayer() || (spec_type == KILL_POISON && vict->affected_by_spell(SPELL_POISON)->modifier == -123))
      raw_kill(ch, vict);
    else if (ch->dc_->IS_ARENA(vict->in_room))
      arena_kill(ch, vict, spec_type);
    else if (is_pkill(ch, vict))
      do_pkill(ch, vict, spec_type, vict_is_attacker);
    else
      raw_kill(ch, vict);
    break;
  case TYPE_PKILL:
    do_pkill(ch, vict, spec_type);
    break;
  case TYPE_RAW_KILL:
    raw_kill(ch, vict);
    break;
  case TYPE_ARENA_KILL:
    arena_kill(ch, vict, spec_type);
    break;
  }
}

QString translate_name(const CharacterPtr ch)
{
  if (ch->isNonPlayer())
  {
    return u"%1(v%2)"_s.arg(qPrintable(ch->name())).arg(dc_->mob_index[ch->mobdata->nr].vnum());
  }
  return qPrintable(ch->name());
}

void debug_isHit(const CharacterPtr ch, const CharacterPtr victim, const qint32 &attacktype, const qint32 &type, const qint32 &reduce, const qint32 &tohit, const QString message = QString())
{
  if (ch->getDebug() || victim->getDebug())
  {
    dc_->logentry(u"isHit: %1 vs %2 attacktype=%3 type=%4 reduce=%5 tohit=%6 %7"_s.arg(translate_name(ch)).arg(translate_name(victim)).arg(attacktype).arg(type).arg(reduce).arg(tohit).arg(message), DIVINITY, DC::LogChannel::LOG_BUG);
  }
}

void debug_isHit_toHit(const CharacterPtr ch, const CharacterPtr victim, const qint32 &toHit)
{
  if (ch->getDebug() || victim->getDebug())
  {
    dc_->logentry(u"isHit: toHit=%1"_s.arg(toHit), DIVINITY, DC::LogChannel::LOG_BUG);
  }
}

void debug_isHit_generic(const CharacterPtr ch, const CharacterPtr victim, const auto &parry, const auto &dodge, const auto &block, const auto &martial, const auto &tumbling)
{
  if (ch->getDebug() || victim->getDebug())
  {
    dc_->logentry(u"parry=%1 dodge=%2 block=%3 martial=%4 tumbling=%5"_s.arg(parry).arg(dodge).arg(block).arg(martial).arg(tumbling), DIVINITY, DC::LogChannel::LOG_BUG);
  }
}

// New toHit code
bool isHit(CharacterPtr ch, CharacterPtr victim, qint32 attacktype, qint32 &type, qint32 &reduce)
{
  if ((isSet(victim->combat, COMBAT_STUNNED)) ||
      (isSet(victim->combat, COMBAT_STUNNED2)) ||
      (isSet(victim->combat, COMBAT_BASH1)) ||
      (isSet(victim->combat, COMBAT_BASH2)) ||
      (isSet(victim->combat, COMBAT_SHOCKED)) ||
      (isSet(victim->combat, COMBAT_SHOCKED2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)) ||
      (!AWAKE(victim)))
  {
    debug_isHit(ch, victim, attacktype, type, reduce, 0, "stunned,bash,shocked return ReturnValue::eFAILURE");
    return ReturnValue::eFAILURE; // always hit
  }

  level_diff_t level_difference = ch->getLevel() - victim->getLevel();
  qint32 skill = {};

  // Figure out toHit value.
  qint32 toHit = GET_REAL_HITROLL(ch);
  debug_isHit_toHit(ch, victim, toHit);
  //  toHit += speciality_bonus(ch, attacktype, GET_LEVEL(victim));

  switch (attacktype)
  {
  case TYPE_BLUDGEON:
    skill = SKILL_BLUDGEON_WEAPONS;
    break;
  case TYPE_WHIP:
    skill = SKILL_WHIPPING_WEAPONS;
    break;
  case TYPE_CRUSH:
    skill = SKILL_CRUSHING_WEAPONS;
    break;
  case TYPE_SLASH:
    skill = SKILL_SLASHING_WEAPONS;
    break;
  case TYPE_PIERCE:
    skill = SKILL_PIERCEING_WEAPONS;
    break;
  case TYPE_STING:
    skill = SKILL_STINGING_WEAPONS;
    break;
  case TYPE_HIT:
    skill = SKILL_HAND_TO_HAND;
    break;
  default:
    break;
  }
  if (skill)
  {
    toHit += ch->has_skill(skill) / 8;
    debug_isHit_toHit(ch, victim, toHit);
  }

  if (isSet(ch->combat, COMBAT_BERSERK) || IS_AFFECTED(ch, AFF_PRIMAL_FURY))
  {
    toHit = (qint32)((qreal)toHit * 0.90) - 5;
    debug_isHit_toHit(ch, victim, toHit);
  }
  else if (isSet(ch->combat, COMBAT_RAGE1) || isSet(ch->combat, COMBAT_RAGE2))
  {
    toHit = (qint32)((qreal)toHit * 0.95) - 2;
    debug_isHit_toHit(ch, victim, toHit);
  }

  if (toHit < 1)
  {
    toHit = 1;
    debug_isHit_toHit(ch, victim, toHit);
  }

  // Hitting stuff close to your level gives you a bonus,
  if (level_difference > 15 && level_difference < 25)
  {
    toHit += 5;
    debug_isHit_toHit(ch, victim, toHit);
  }
  else if (level_difference > 5 && level_difference <= 15)
  {
    toHit += 7;
    debug_isHit_toHit(ch, victim, toHit);
  }
  else if (level_difference >= 0 && level_difference <= 5)
  {
    toHit += 10;
    debug_isHit_toHit(ch, victim, toHit);
  }
  else if (level_difference >= -5 && level_difference < 0)
  {
    toHit += 5;
    debug_isHit_toHit(ch, victim, toHit);
  }

  // Give a tohit bonus to low level players.

  qreal lowlvlmod = (50.0 - ch->getLevel() - (victim->getLevel() / 2.0)) / 10.0;
  if (lowlvlmod > 1.0)
  {
    toHit = (qint32)((qreal)toHit * lowlvlmod);
    debug_isHit_toHit(ch, victim, toHit);
  }

  // The stuff.
  float num1 = 1.0 - (-300.0 - (qreal)GET_AC(victim)) * 4.761904762 * 0.0001;
  float num2 = 20.0 + (-300.0 - (qreal)GET_AC(victim)) * 0.0095238095;
  qreal percent = 30 + num1 * (qreal)(toHit)-num2;

  // "percent" now contains the maximum avoidance rate. If they do not have two maxed defensive skills, it will actually be less.

  // Determine defensive skills.
  qint32 parry = victim->isNonPlayer() ? ISSET(victim->mobdata->actflags, ACT_PARRY) ? victim->getLevel() / 2 : 0 : victim->has_skill(SKILL_PARRY);
  qint32 dodge = victim->isNonPlayer() ? ISSET(victim->mobdata->actflags, ACT_DODGE) ? victim->getLevel() / 2 : 0 : victim->has_skill(SKILL_DODGE);
  qint32 block = victim->has_skill(SKILL_SHIELDBLOCK);
  qint32 martial = victim->has_skill(SKILL_DEFENSE);
  qint32 tumbling = victim->has_skill(SKILL_TUMBLING);

  debug_isHit_generic(ch, victim, parry, dodge, block, martial, tumbling);

  if (victim->equipment[WEAR_WIELD] == nullptr)
    parry = {};

  if (!victim->equipment[WEAR_SHIELD])
    block = {};
  else if (victim->isNonPlayer())
    block = victim->getLevel() / 2;

  // Modify defense rate accordingly
  qint32 amt = parry + dodge + block + martial + tumbling;

  qreal scale = (qreal)amt / (196.0); // Mobs can get a bonus if they can perform 3+.

  percent *= (1.0 - scale / 5.0);

  if (parry)
    victim->skill_increase_check(SKILL_PARRY, parry, SKILL_INCREASE_HARD + 500);
  if (dodge)
    victim->skill_increase_check(SKILL_DODGE, dodge, SKILL_INCREASE_HARD + 500);
  if (block)
    victim->skill_increase_check(SKILL_SHIELDBLOCK, block, SKILL_INCREASE_HARD + 500);
  if (martial)
    victim->skill_increase_check(SKILL_DEFENSE, martial, SKILL_INCREASE_HARD + 500);
  if (tumbling)
    victim->skill_increase_check(SKILL_TUMBLING, tumbling, SKILL_INCREASE_HARD + 500);

  // Ze random stuff.
  if (ch->dc_->number(1, 100) < (qint32)percent && !isSet(victim->combat, COMBAT_BLADESHIELD1) && !isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    debug_isHit(ch, victim, attacktype, type, reduce, toHit, u"Ze random stuff percent=%1"_s.arg(percent));
    return ReturnValue::eFAILURE;
  }

  // Miss, determine a message

  amt += 8; // Chance for a pure miss.

  qint32 what = dc_->number(0, amt);
  qint32 retval = {};

  if (what < parry || isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  { // Parry. Riposte-check goes here.
    act_to_room("$n parries $N's attack.", victim, nullptr, ch, NOTVICT);
    act_to_victim("$n parries your attack.", victim, nullptr, ch, 0);
    act_to_character("You parry $N's attack.", victim, nullptr, ch, 0);
    retval = check_riposte(ch, victim, attacktype);
    if (SOMEONE_DIED(retval))
    {
      debug_isHit(ch, victim, attacktype, type, reduce, toHit, "SOMEONE_DIED");
      return debug_retval(ch, victim, retval);
    }
  }
  else if (what < (parry + tumbling))
  { // Tumbling
    switch (ch->dc_->number(0, 2))
    {
    case 0: // full avoid
      if (ch->dc_->number(0, 1))
      {
        act_to_character("You spin adroitly to the side, watching in amusement as $N's swing passes by harmlessly.", victim, 0, ch, 0);
        act_to_victim("$n spins adroitly to the side, watching in amusement as your swing passes by harmlessly.", victim, 0, ch, 0);
        act_to_room("$n spins adroitly to the side, watching in amusement as $N's swing passes by harmlessly.", victim, 0, ch, NOTVICT);
      }
      else
      {
        act_to_character("You jump quickly and execute a full backflip, landing nimbly on your feet as $N's blow misses completely.", victim, 0, ch, 0);
        act_to_victim("$n jumps quickly and executes a full backflip, landing nimbly on $s feet as your blow misses completely.", victim, 0, ch, 0);
        act_to_room("$n jumps quickly and executes a full backflip, landing nimbly on $s feet as $N's blow misses completely.", victim, 0, ch, NOTVICT);
      }
      break;
    case 1: // shieldblock style damage
      type = 4;
      reduce = victim->has_skill(SKILL_TUMBLING);
      break;
    case 2: // riposte style damage
      retval = doTumblingCounterStrike(ch, victim);
      if (SOMEONE_DIED(retval))
      {
        debug_isHit(ch, victim, attacktype, type, reduce, toHit, "SOMEONE_DIED");
        return debug_retval(ch, victim, retval);
      }
      break;
    default:
      ch->sendln("Messed up tumbling. tell somebody, whore!");
      break;
    }
  }
  else if (what < (parry + tumbling + dodge))
  { // Dodge
    act_to_room("$n dodges $N's attack.", victim, nullptr, ch, NOTVICT);
    act_to_victim("$n dodges your attack.", victim, nullptr, ch, 0);
    act_to_character("You dodge $N's attack.", victim, nullptr, ch, 0);
  }
  else if (what < (parry + tumbling + dodge + block))
  { // Shieldblock
    type = 1;
    reduce = victim->has_skill(SKILL_SHIELDBLOCK);
  }
  else if (what < (parry + tumbling + dodge + block + martial))
  { // Mdefense
    type = 2;
    reduce = 100 * victim->has_skill(SKILL_DEFENSE) / 125;
  }
  else
  { // Miss
    type = 3;
  }
  debug_isHit(ch, victim, attacktype, type, reduce, toHit, "return ReturnValue::eSUCCESS");
  return ReturnValue::eSUCCESS;
}

// check counter strike never returns ReturnValue::eSUCCESS because that would
// get returned from damage as a successful damage, which it's
// not.
qint32 checkCounterStrike(CharacterPtr ch, CharacterPtr victim)
{
  qint32 retval, lvl = victim->has_skill(SKILL_COUNTER_STRIKE);

  if ((isSet(victim->combat, COMBAT_STUNNED)) ||
      (victim->equipment[WEAR_WIELD] != nullptr) ||
      (isSet(victim->combat, COMBAT_STUNNED2)) ||
      (isSet(victim->combat, COMBAT_BASH1)) ||
      (isSet(victim->combat, COMBAT_BASH2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)) ||
      (isSet(victim->combat, COMBAT_BLADESHIELD1)) ||
      (isSet(victim->combat, COMBAT_BLADESHIELD2)) ||
      (isSet(ch->combat, COMBAT_BLADESHIELD1)) ||
      (isSet(ch->combat, COMBAT_BLADESHIELD2)))
    return ReturnValue::eFAILURE;

  qint32 p = lvl / 2 - (100 - victim->has_skill(SKILL_DEFENSE)) - GET_DEX(ch) + victim->getHP() * 10 / GET_MAX_HIT(victim);

  victim->skill_increase_check(SKILL_COUNTER_STRIKE, lvl, SKILL_INCREASE_HARD);

  if (ch->dc_->number(1, 100) > p)
    return ReturnValue::eFAILURE;

  switch (ch->dc_->number(1, 4))
  {
  case 1:
    act_to_character("Upon blocking $N's blow, you counter with a hard strike of your palm!", victim, nullptr, ch, 0);
    act_to_victim("Upon blocking your blow, $n counters with a hard strike of $s palm!", victim, nullptr, ch, 0);
    act_to_room("Upon blocking $N's blow, $n counters with a hard strike of $s palm!", victim, nullptr, ch, NOTVICT);
    break;
  case 2:
    act_to_character("Upon blocking $N's blow, you counter with a sharp kick of your heel!", victim, nullptr, ch, 0);
    act_to_victim("Upon blocking your blow, $n counters with a sharp kick of $s heel!", victim, nullptr, ch, 0);
    act_to_room("Upon blocking $N's blow, $n counters with a sharp kick of $s heel!", victim, nullptr, ch, NOTVICT);
    break;
  case 3:
    act_to_character("Upon blocking $N's blow, you spin and land a short, hard strike with your elbow!", victim, nullptr, ch, 0);
    act_to_victim("Upon blocking your blow, $n spins and lands a short, hard strike with $s elbow!", victim, nullptr, ch, 0);
    act_to_room("Upon blocking $N's blow, $n spins and lands a short, hard strike with $s elbow!", victim, nullptr, ch, NOTVICT);
    break;
  case 4:
    act_to_character("Upon blocking $N's blow, you spin and land a solid strike with your knee!", victim, nullptr, ch, 0);
    act_to_victim("Upon blocking your blow, $n spins and lands a solid strike with $s knee!", victim, nullptr, ch, 0);
    act_to_room("Upon blocking $N's blow, $n spins and lands a solid strike with $s knee!", victim, nullptr, ch, NOTVICT);
    break;
  default:
    dc_->logentry(u"Serious screw-up in counter strike!"_s, ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }

  retval = one_hit(victim, ch, TYPE_HIT, WEAR_WIELD);
  retval = SWAP_CH_VICT(retval);

  REMOVE_BIT(retval, ReturnValue::eSUCCESS);
  SET_BIT(retval, ReturnValue::eFAILURE);

  return debug_retval(ch, victim, retval);
}

// check counter strike never returns ReturnValue::eSUCCESS because that would
// get returned from damage as a successful damage, which it's
// not.
qint32 doTumblingCounterStrike(CharacterPtr ch, CharacterPtr victim)
{
  qint32 retval;

  if ((isSet(victim->combat, COMBAT_STUNNED)) ||
      (isSet(victim->combat, COMBAT_STUNNED2)) ||
      (isSet(victim->combat, COMBAT_BASH1)) ||
      (isSet(victim->combat, COMBAT_BASH2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)) ||
      (isSet(victim->combat, COMBAT_BLADESHIELD1)) ||
      (isSet(victim->combat, COMBAT_BLADESHIELD2)) ||
      (isSet(ch->combat, COMBAT_BLADESHIELD1)) ||
      (isSet(ch->combat, COMBAT_BLADESHIELD2)))
    return ReturnValue::eFAILURE;

  switch (ch->dc_->number(1, 2))
  {
  case 1:
    act_to_character("$N overextends $Mself as $E strikes you, leaving $M open to your counterattack!", victim, nullptr, ch, 0);
    act_to_victim("You overextend yourself as you strike $n, leaving yourself open to $s counterattack!", victim, nullptr, ch, 0);
    act_to_room("$N overextends $Mself as $E strikes $n, leaving $M open to a counterattack!", victim, nullptr, ch, NOTVICT);
    break;
  case 2:
    act_to_character("You find an opening in $N's defenses as $E swings, and land a quick counterattack!", victim, nullptr, ch, 0);
    act_to_victim("$n finds an opening in your defenses as you swing, and lands a quick counterattack!", victim, nullptr, ch, 0);
    act_to_room("$n finds an opening in $N's defenses as $E swings, and lands a quick counterattack!", victim, nullptr, ch, NOTVICT);
    break;
  default:
    dc_->logentry(u"Serious screw-up in counter strike!"_s, ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }

  retval = one_hit(victim, ch, TYPE_HIT, WEAR_WIELD);
  retval = SWAP_CH_VICT(retval);

  REMOVE_BIT(retval, ReturnValue::eSUCCESS);
  SET_BIT(retval, ReturnValue::eFAILURE);

  return debug_retval(ch, victim, retval);
}

// check riposte never returns ReturnValue::eSUCCESS because that would
// get returned from damage as a successful damage, which it's
// not.
qint32 check_riposte(CharacterPtr ch, CharacterPtr victim, qint32 attacktype)
{
  qint32 retval;

  if ((isSet(victim->combat, COMBAT_STUNNED)) ||
      (ch->equipment[WEAR_WIELD] == nullptr && dc_->number(1, 101) >= 50) ||
      (isSet(victim->combat, COMBAT_STUNNED2)) ||
      (isSet(victim->combat, COMBAT_BASH1)) ||
      (isSet(victim->combat, COMBAT_BASH2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)) ||
      (isSet(victim->combat, COMBAT_BLADESHIELD1)) ||
      (isSet(victim->combat, COMBAT_BLADESHIELD2)) ||
      (isSet(ch->combat, COMBAT_BLADESHIELD1)) ||
      (isSet(ch->combat, COMBAT_BLADESHIELD2)))
    return ReturnValue::eFAILURE;

  // 25% chance of success for mobs
  if (victim->isNonPlayer())
  {
    if (ch->dc_->number(0, 3) > 0)
    {
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    if (!victim->has_skill(SKILL_RIPOSTE))
    {
      return ReturnValue::eFAILURE;
    }
    else
    {
      qint32 modifier = {};

      modifier += speciality_bonus(ch, attacktype, victim->getLevel());
      modifier -= GET_DEX(ch) / 2;
      modifier -= 10;

      if (!skill_success(victim, ch, SKILL_RIPOSTE, modifier))
      {
        return ReturnValue::eFAILURE;
      }
    }
  }

  act_to_room("$n turns $N's attack into one of $s own!", victim, nullptr, ch, NOTVICT);
  act_to_victim("$n turns your attack against you!", victim, nullptr, ch, 0);
  act_to_character("You turn $N's attack against $M.", victim, nullptr, ch, 0);

  retval = one_hit(victim, ch, TYPE_UNDEFINED, WEAR_WIELD);
  retval = SWAP_CH_VICT(retval);

  REMOVE_BIT(retval, ReturnValue::eSUCCESS);
  SET_BIT(retval, ReturnValue::eFAILURE);

  return debug_retval(ch, victim, retval);
}

qint32 check_magic_block(CharacterPtr ch, CharacterPtr victim, qint32 attacktype)
{
  qint32 reduce = {};
  if (victim->equipment[WEAR_SHIELD] == nullptr && GET_CLASS(victim) != CLASS_MONK)
    return 0;
  if ((isSet(victim->combat, COMBAT_STUNNED)) ||
      //    (victim->equipment[WEAR_SHIELD] == nullptr) ||
      (victim->isNonPlayer() && (!ISSET(victim->mobdata->actflags, ACT_PARRY))) ||
      (isSet(victim->combat, COMBAT_STUNNED2)) ||
      (isSet(victim->combat, COMBAT_BASH1)) ||
      (isSet(victim->combat, COMBAT_BASH2)) ||
      (isSet(victim->combat, COMBAT_SHOCKED)) ||
      (isSet(victim->combat, COMBAT_SHOCKED2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return 0;
  if (victim->isNonPlayer())
    reduce = victim->getLevel() / 2; // shrug
  if (!(reduce = victim->has_skill(SKILL_SHIELDBLOCK)) && !(GET_CLASS(victim) == CLASS_MONK))
    return 0;
  else if (!((reduce = victim->has_skill(SKILL_DEFENSE)) && GET_CLASS(victim) == CLASS_MONK))
    return 0;

  qint32 skill = reduce / 10;
  reduce = (qint32)((qreal)reduce * 0.75);

  if (ch->dc_->number(1, 101) > skill)
    return 0;
  if (GET_CLASS(victim) != CLASS_MONK)
  {
    act_to_room("$n blocks part of $N's spell with $p.", victim, victim->equipment[WEAR_SHIELD], ch, NOTVICT);
    act_to_victim("$n blocks part of your spell with $p.", victim, victim->equipment[WEAR_SHIELD], ch, 0);
    act_to_character("You dodge down behind $p and deflect part of $N's spell.", victim, victim->equipment[WEAR_SHIELD], ch, 0);
  }
  else
  {
    act_to_room("$n manages to avoid taking a direct hit from $N's spell!", victim, nullptr, ch, NOTVICT);
    act_to_victim("$n avoids part of your spell!", victim, nullptr, ch, 0);
    act_to_character("Your martial defense allows you to avoid a direct hit from $N's spell!", victim, nullptr, ch, 0);
    reduce = (qint32)((qreal)reduce / 1.25);
  }
  return reduce;
}

qint32 check_shieldblock(CharacterPtr ch, CharacterPtr victim, qint32 attacktype)
{
  qint32 modifier = {};
  qint32 reduce = {};
  if (victim->equipment[WEAR_SHIELD] == nullptr && GET_CLASS(victim) != CLASS_MONK)
    return 0;
  if ((isSet(victim->combat, COMBAT_STUNNED)) ||
      //    (victim->equipment[WEAR_SHIELD] == nullptr) ||
      (victim->isNonPlayer() && (!ISSET(victim->mobdata->actflags, ACT_PARRY))) ||
      (isSet(victim->combat, COMBAT_STUNNED2)) ||
      (isSet(victim->combat, COMBAT_BASH1)) ||
      (isSet(victim->combat, COMBAT_BASH2)) ||
      (isSet(victim->combat, COMBAT_SHOCKED)) ||
      (isSet(victim->combat, COMBAT_SHOCKED2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return 0;

  // TODO - remove this when mobs have "skills"
  if (victim->isNonPlayer())
  {
    reduce = victim->getLevel() / 2;
    switch (GET_CLASS(victim))
    {
    case CLASS_MONK:
    case CLASS_ANTI_PAL:
    case CLASS_PALADIN:
    case CLASS_WARRIOR:
      modifier = 5;
      break;
    case CLASS_RANGER:
    case CLASS_BARBARIAN:
    case CLASS_THIEF:
      modifier = {};
      break;
    default:
      modifier = -5;
      break;
    }
  }
  else if (!(reduce = victim->has_skill(SKILL_SHIELDBLOCK)) && !(GET_CLASS(victim) == CLASS_MONK))
    return 0;
  else if (GET_CLASS(victim) == CLASS_MONK && !((reduce = victim->has_skill(SKILL_DEFENSE))))
    return 0;
  modifier += speciality_bonus(ch, attacktype, victim->getLevel());

  //  extern qint32 stat_mod[];
  // modifier -= stat_mod[GET_DEX(ch)];

  //  if (victim->isNonPlayer()) modifier -= 50;
  //  modifier += GET_DEX(victim)/2;
  if (GET_CLASS(victim) == CLASS_MONK)
  {
    if (!skill_success(victim, ch, SKILL_DEFENSE, modifier))
      return 0;
  }
  else if (!skill_success(victim, ch, SKILL_SHIELDBLOCK, modifier))
    return 0;

  // act_to_room("$n blocks $N's attack with $s shield.", victim, nullptr, ch,  NOTVICT);
  // act_to_victim("$n blocks your attack with $s shield.", victim, nullptr, ch,  0);
  // act_to_character("You block $N's attack with your shield.", victim, nullptr, ch,  0);
  /*  if (!GET_CLASS(victim) == CLASS_MONK) {
      act_to_room("$n blocks $N's attack with $p.", victim, victim->equipment[WEAR_SHIELD], ch,  NOTVICT);
      act_to_victim("$n blocks your attack with $p.", victim, victim->equipment[WEAR_SHIELD], ch,  0);
      act_to_character("You block $N's attack with $p.", victim, victim->equipment[WEAR_SHIELD], ch,  0);
    } else {
      act_to_room("$n swiftly deflects $N's attack.", victim, nullptr, ch,  NOTVICT);
      act("$n swiftly deflects your attack.",victim, nullptr, ch, TO_VICT, 0);
      act_to_character("You swiftly deflect $N's attack.",victim, nullptr, ch,  0);
    }*/
  if (GET_CLASS(victim) == CLASS_MONK)
    reduce = (qint32)((qreal)reduce / 1.25);

  return reduce;
}

bool check_parry(CharacterPtr ch, CharacterPtr victim, qint32 attacktype, bool display_results)
{
  qint32 modifier = {};
  if ((isSet(victim->combat, COMBAT_STUNNED)) ||
      (victim->equipment[WEAR_WIELD] == nullptr) ||
      (victim->isNonPlayer() && (!ISSET(victim->mobdata->actflags, ACT_PARRY))) ||
      (isSet(victim->combat, COMBAT_STUNNED2)) ||
      ((isSet(victim->combat, COMBAT_BASH1) ||
        isSet(victim->combat, COMBAT_BASH2)) &&
       !isSet(victim->combat, COMBAT_BLADESHIELD1) &&
       !isSet(victim->combat, COMBAT_BLADESHIELD2)) ||
      (isSet(victim->combat, COMBAT_SHOCKED)) ||
      (isSet(victim->combat, COMBAT_SHOCKED2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return false;

  if (victim->isNonPlayer())
  {
    switch (GET_CLASS(victim))
    {
    case CLASS_WARRIOR:
      modifier = 15;
      break;
    case CLASS_THIEF:
      modifier = 1;
      break;
    case CLASS_MONK:
      modifier = -5;
      break;
    case CLASS_BARD:
      modifier = 1;
      break;
    case CLASS_RANGER:
      modifier = 5;
      break;
    case CLASS_PALADIN:
      modifier = 10;
      break;
    case CLASS_ANTI_PAL:
      modifier = 5;
      break;
    case CLASS_BARBARIAN:
      modifier = 5;
      break;
    case CLASS_MAGIC_USER:
      modifier = -5;
      break;
    case CLASS_CLERIC:
      modifier = -5;
      break;
    case CLASS_NECROMANCER:
      modifier = -5;
      break;
    default:
      modifier = {};
      break;
    }
  }
  else if (!victim->has_skill(SKILL_PARRY))
    return false;
  if (!modifier && victim->isNonPlayer() && (ISSET(victim->mobdata->actflags, ACT_PARRY)))
    modifier = 10;
  else if (victim->isNonPlayer() && !ISSET(victim->mobdata->actflags, ACT_PARRY))
    return false; // damned mobs

  modifier += speciality_bonus(ch, attacktype, victim->getLevel());
  //  if (victim->isNonPlayer()) modifier -= 50;
  //  if (attacktype==TYPE_HIT) modifier += 30; // Harder to parry unarmed attacks
  //  else modifier += 22;
  modifier -= GET_DEX(ch) / 2;
  modifier -= 10;
  if (!skill_success(victim, ch, SKILL_PARRY, modifier) &&
      !isSet(victim->combat, COMBAT_BLADESHIELD1) &&
      !isSet(victim->combat, COMBAT_BLADESHIELD2))
    return false;

  if (display_results == true)
  {
    act_to_room("$n parries $N's attack.", victim, nullptr, ch, NOTVICT);
    act_to_victim("$n parries your attack.", victim, nullptr, ch, 0);
    act_to_character("You parry $N's attack.", victim, nullptr, ch, 0);
  }
  return true;
}

qint32 speciality_bonus(CharacterPtr ch, qint32 attacktype, qint32 level)
{
  qint32 skill = {};
  /*  qint32 w_type = TYPE_HIT;
    if(wielded && wielded->obj_flags.type_flag == ITEM_WEAPON)
       w_type = get_weapon_damage_type(wielded);*/
  switch (attacktype)
  {
  case TYPE_BLUDGEON:
    skill = SKILL_BLUDGEON_WEAPONS;
    break;
  case TYPE_WHIP:
    skill = SKILL_WHIPPING_WEAPONS;
    break;
  case TYPE_CRUSH:
    skill = SKILL_CRUSHING_WEAPONS;
    break;
  case TYPE_SLASH:
    skill = SKILL_SLASHING_WEAPONS;
    break;
  case TYPE_PIERCE:
    skill = SKILL_PIERCEING_WEAPONS;
    break;
  case TYPE_STING:
    skill = SKILL_STINGING_WEAPONS;
    break;
  case TYPE_HIT:
    skill = SKILL_HAND_TO_HAND;
    break;
  default:
    break;
  }
  level -= ch->getLevel();
  if (!skill)
    return 0;
  else
    return ch->has_skill(skill) / 10;

  if (level < -20 && ch->isNonPlayer())
    return 0 - (qint32)(ch->getLevel() / 2.4);
  else if (level < -10 && ch->isNonPlayer())
    return 0 - (qint32)(ch->getLevel() / 2.6);
  else if (level < 0 && ch->isNonPlayer())
    return 0 - (qint32)(ch->getLevel() / 2.8);
  else if (ch->isNonPlayer())
    return 0 - (qint32)(ch->getLevel() / 3.5);

  qint32 l = ch->has_skill(skill) / 2;
  return 0 - l;
}

/*
 * Check for dodge.
 */
bool check_dodge(CharacterPtr ch, CharacterPtr victim, qint32 attacktype, bool display_results)
{
  //  qint32 chance;
  qint32 modifier = {};
  if ((isSet(victim->combat, COMBAT_STUNNED)) ||
      (isSet(victim->combat, COMBAT_STUNNED2)) ||
      (isSet(victim->combat, COMBAT_BASH1)) ||
      (isSet(victim->combat, COMBAT_BASH2)) ||
      (isSet(victim->combat, COMBAT_SHOCKED)) ||
      (isSet(victim->combat, COMBAT_SHOCKED2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return false;

  if (victim->isNonPlayer())
  {
    switch (GET_CLASS(victim))
    {
    case CLASS_WARRIOR:
      modifier = 10;
      break;
    case CLASS_THIEF:
      modifier = 25;
      break;
    case CLASS_MONK:
      modifier = 5;
      break;
    case CLASS_BARD:
      modifier = 5;
      break;
    case CLASS_RANGER:
      modifier = 3;
      break;
    case CLASS_PALADIN:
      modifier = 1;
      break;
    case CLASS_ANTI_PAL:
      modifier = 5;
      break;
    case CLASS_BARBARIAN:
      modifier = 1;
      break;
    case CLASS_MAGIC_USER:
      modifier = -5;
      break;
    case CLASS_CLERIC:
      modifier = -5;
      ;
      break;
    case CLASS_NECROMANCER:
      modifier = -5;
      break;
    default:
      modifier = {};
      break;
    }
    if (!ISSET(victim->mobdata->actflags, ACT_DODGE))
      modifier = {}; // damned mobs
    else if (modifier == 0)
      modifier = 5;
  }
  else if (!victim->has_skill(SKILL_DODGE))
    return false;

  if (modifier == 0 && victim->isNonPlayer())
    return false;

  modifier += speciality_bonus(ch, attacktype, victim->getLevel());
  //  if (victim->isNonPlayer()) modifier = 50; // 75 is base, and it's calculated
  // around here
  modifier -= GET_DEX(ch) / 2;
  if (!skill_success(victim, ch, SKILL_DODGE, modifier))
    return false;

  if (display_results == true)
  {
    act_to_room("$n dodges $N's attack.", victim, nullptr, ch, NOTVICT);
    act_to_victim("$n dodges your attack.", victim, nullptr, ch, 0);
    act_to_character("You dodge $N's attack.", victim, nullptr, ch, 0);
  }

  return true;
}

/*
 * Load fighting messages into memory.
 */
void DC::load_messages(const QString file, qint32 base)
{
  FILE *fl;
  qint32 i, type;
  extern message_list fight_messages[MAX_MESSAGES];
  message_type *messages;
  QString chk;

  if (!(fl = fopen(file, "r")))
  {
    perror("read messages");
    exit(0);
  }
  if (base == 0)
    for (i = {}; i < MAX_MESSAGES; i++)
    {
      fight_messages[i].a_type = {};
      fight_messages[i].number_of_attacks = {};
      fight_messages[i].msg = {};
      fight_messages[i].msg2 = {};
    }

  fscanf(fl, "%s\n", chk);

  while (*chk == 'M')
  {
    fscanf(fl, " %d\n", &type);
    //     type += base;
    for (i = {}; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
                 (fight_messages[i].a_type);
         i++)
      ;
    //     if (type == 80)
    //	 produce_coredump();
    if (i >= MAX_MESSAGES)
    {
      dc_->logentry(u"Too many combat messages."_s, ANGEL, DC::LogChannel::LOG_BUG);
      exit(0);
    }

    messages = new message_type;
    if (!base)
      fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    if (!base)
    {
      messages->next = fight_messages[i].msg;
      fight_messages[i].msg = messages;
    }
    else
    {
      messages->next = fight_messages[i].msg2;
      fight_messages[i].msg2 = messages;
    }
    messages->die_msg.attacker_msg = fread_string(fl, 0);
    messages->die_msg.victim_msg = fread_string(fl, 0);
    messages->die_msg.room_msg = fread_string(fl, 0);
    messages->miss_msg.attacker_msg = fread_string(fl, 0);
    messages->miss_msg.victim_msg = fread_string(fl, 0);
    messages->miss_msg.room_msg = fread_string(fl, 0);
    messages->hit_msg.attacker_msg = fread_string(fl, 0);
    messages->hit_msg.victim_msg = fread_string(fl, 0);
    messages->hit_msg.room_msg = fread_string(fl, 0);
    messages->god_msg.attacker_msg = fread_string(fl, 0);
    messages->god_msg.victim_msg = fread_string(fl, 0);
    messages->god_msg.room_msg = fread_string(fl, 0);
    fscanf(fl, " %s \n", chk);
  }

  fclose(fl);
}

void DC::free_messages_from_memory(void)
{
  extern message_list fight_messages[MAX_MESSAGES];
  message_type *next_message = {};
  qint32 i;

  for (i = {}; (i < MAX_MESSAGES) && (fight_messages[i].a_type); i++)
    while (fight_messages[i].msg)
    {
      next_message = fight_messages[i].msg->next;
      fight_messages[i].msg = {};
      fight_messages[i].msg = next_message;
    }
}

/*
 * Set position of a victim.
 */
void update_pos(CharacterPtr victim)
{
  if (victim->getHP() > 0)
  {
    if ((!isSet(victim->combat, COMBAT_STUNNED)) && (!isSet(victim->combat, COMBAT_STUNNED2)))
      if (GET_POS(victim) <= position_t::STUNNED)
        victim->setStanding();
    return;
  }
  else
    victim->setDead();
}

/*
 * Start fights.
 */
void set_fighting(CharacterPtr ch, CharacterPtr vict)
{
  CharacterPtr k, next_char;
  qint32 count = {};

  if (ch->fighting) /* If he's already fighting */
    return;
  if (IS_AFFECTED(ch, AFF_HIDE))
    REMBIT(ch->affected_by, AFF_HIDE);
  if (IS_AFFECTED(vict, AFF_HIDE))
    REMBIT(vict->affected_by, AFF_HIDE);

  if (ch->isPlayer() && vict->isNonPlayer())
    if (!ISSET(vict->mobdata->actflags, ACT_STUPID))
      vict->add_memory(qPrintable(ch->name()), 'h');

  if (ch->isNonPlayer() && vict->isNonPlayer() && IS_AFFECTED(ch, AFF_CHARM) &&
      ch->master && ch->master->isPlayer())
    if (!ISSET(vict->mobdata->actflags, ACT_STUPID))
      vict->add_memory(qPrintable(ch->master->name()), 'h');

  if (ch->isPlayer() && vict->isNonPlayer())
    if (!ISSET(vict->mobdata->actflags, ACT_STUPID) && vict->hunting.isEmpty())
    {
      level_diff_t level_difference = ch->getLevel() - (vict->getLevel() / 2.0);
      if (level_difference > 0 || ch->getLevel() == 60)
      {
        vict->add_memory(qPrintable(ch->name()), 't');
        timer_data *timer = new timer_data;
        timer->var_arg1 = vict->hunting;
        timer->arg2 = (void *)vict;
        timer->function = clear_hunt;
        timer->next = timer_list;
        timer_list = timer;
        timer->timeleft = (ch->getLevel() / 4) * 60;
      }
      if (vict->isPlayer() && ch->isNonPlayer())
        if (!ISSET(ch->mobdata->actflags, ACT_STUPID) && ch->hunting.isEmpty())
        {
          level_diff_t level_difference = vict->getLevel() - (ch->getLevel() / 2);
          if (level_difference > 0 || vict->getLevel() == 60)
          {
            ch->add_memory(vict->name(), 't');
            timer_data *timer = new timer_data;
            timer->var_arg1 = ch->hunting;
            timer->arg2 = (void *)ch;
            timer->function = clear_hunt;
            timer->next = timer_list;
            timer_list = timer;
            timer->timeleft = (vict->getLevel() / 4) * 60;
          }
        }
    }

  for (k = combat_list; k; k = next_char)
  {
    next_char = k->next_fighting;
    if (k->fighting == vict)
      count++;
  }

  /*(  if( ( ch->isPlayer() || IS_AFFECTED(ch, AFF_CHARM) )
        && count >= 6 )
    {
      ch->sendln("You can't get close enough to fight.");
      return;
    }*/

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (IS_AFFECTED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  ch->fighting = vict;

  if ((!isSet(ch->combat, COMBAT_STUNNED)) &&
      (!isSet(ch->combat, COMBAT_STUNNED2)) &&
      (!isSet(ch->combat, COMBAT_BASH1)) &&
      (!isSet(ch->combat, COMBAT_BASH2)))
    ch->setPOSFighting();
}

// Stop fights.
void stop_fighting(CharacterPtr ch, qint32 clearlag)
{
  CharacterPtr tmp;

  if (!ch)
  {
    dc_->logentry(u"Null ch in stop_fighting.  This would have crashed us."_s, IMPLEMENTER, DC::LogChannel::LOG_BUG);
    return;
  }

  if (!ch->fighting)
    return;

  // This is in the command interpreter now, so berserk lasts
  // until you are totally done fighting.
  // -Sadus
  if (isSet(ch->combat, COMBAT_BERSERK))
  {
    bool keepZerk = false;
    for (tmp = dc_->world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
      if (tmp->fighting == ch)
        keepZerk = true;
    if (!keepZerk)
    {
      REMOVE_BIT(ch->combat, COMBAT_BERSERK);
      act_to_room("$n settles down.", ch, 0, 0, 0);
      act_to_character("You settle down.", ch, 0, 0, 0);
      GET_AC(ch) -= 30;
    }
  }

  REMOVE_BIT(ch->combat, COMBAT_ATTACKER);

  if (IS_AFFECTED(ch, AFF_PRIMAL_FURY))
  {
    affected_type *af;

    for (af = ch->affected; af; af = af->next)
    {
      if (af->bitvector && af->type == SKILL_PRIMAL_FURY)
      {
        affect_remove(ch, af, 0);
        break;
      }
    }
  }

  if (isSet(ch->combat, COMBAT_RAGE1))
  {
    REMOVE_BIT(ch->combat, COMBAT_RAGE1);
    act_to_room("$n calms down.", ch, 0, 0, 0);
    act_to_character("Your mind seems a bit clearer now.", ch, 0, 0, 0);
  }
  if (isSet(ch->combat, COMBAT_RAGE2))
  {
    REMOVE_BIT(ch->combat, COMBAT_RAGE2);
    act_to_room("$n calms down.", ch, 0, 0, 0);
    act_to_character("Your mind seems a bit clearer now.", ch, 0, 0, 0);
  }

  // make sure people aren't stuck unable to do anything
  if (isSet(ch->combat, COMBAT_SHOCKED2))
    REMOVE_BIT(ch->combat, COMBAT_SHOCKED2);
  if (isSet(ch->combat, COMBAT_SHOCKED))
    REMOVE_BIT(ch->combat, COMBAT_SHOCKED);

  if (isSet(ch->combat, COMBAT_ORC_BLOODLUST1))
  {
    REMOVE_BIT(ch->combat, COMBAT_ORC_BLOODLUST1);
  }
  if (isSet(ch->combat, COMBAT_ORC_BLOODLUST2))
  {
    REMOVE_BIT(ch->combat, COMBAT_ORC_BLOODLUST2);
  }
  if (isSet(ch->combat, COMBAT_VITAL_STRIKE))
    REMOVE_BIT(ch->combat, COMBAT_VITAL_STRIKE);
  if (isSet(ch->combat, COMBAT_BLADESHIELD1))
    REMOVE_BIT(ch->combat, COMBAT_BLADESHIELD1);
  if (isSet(ch->combat, COMBAT_BLADESHIELD2))
    REMOVE_BIT(ch->combat, COMBAT_BLADESHIELD2);
  if (isSet(ch->combat, COMBAT_THI_EYEGOUGE))
  {
    REMOVE_BIT(ch->combat, COMBAT_THI_EYEGOUGE);
    REMBIT(ch->affected_by, AFF_BLIND);
  }
  if (isSet(ch->combat, COMBAT_THI_EYEGOUGE2))
  {
    REMOVE_BIT(ch->combat, COMBAT_THI_EYEGOUGE2);
    REMBIT(ch->affected_by, AFF_BLIND);
  }

  affect_from_char(ch, KI_DISRUPT + KI_OFFSET);

  ch->setStanding();
  ;
  update_pos(ch);

  // Remove ch's lag if he wasn't using wimpy.
  if (ch->isPlayer() && ch->desc && !isSet(ch->player->toggles, Player::PLR_WIMPY) && clearlag)
    ch->desc->wait = {};

  if (ch == combat_next_dude)
    combat_next_dude = ch->next_fighting;

  if (combat_list == ch)
    combat_list = ch->next_fighting;
  else
  {
    for (tmp = combat_list; tmp && (tmp->next_fighting != ch);
         tmp = tmp->next_fighting)
      ;
    if (!tmp)
    {
      dc_->logentry(u"Stop_fighting: character not found"_s, ANGEL, DC::LogChannel::LOG_BUG);
      // abort();
      return;
    }
    tmp->next_fighting = ch->next_fighting;
  }

  ch->next_fighting = {};
  ch->fighting = {};

  if (isSet(ch->combat, COMBAT_BASH1))
    REMOVE_BIT(ch->combat, COMBAT_BASH1);
  if (isSet(ch->combat, COMBAT_BASH2))
    REMOVE_BIT(ch->combat, COMBAT_BASH2);
  if (isSet(ch->combat, COMBAT_STUNNED))
    REMOVE_BIT(ch->combat, COMBAT_STUNNED);
  if (isSet(ch->combat, COMBAT_STUNNED2))
    REMOVE_BIT(ch->combat, COMBAT_STUNNED2);
  if (isSet(ch->combat, COMBAT_CRUSH_BLOW))
    REMOVE_BIT(ch->combat, COMBAT_CRUSH_BLOW);
  if (isSet(ch->combat, COMBAT_CRUSH_BLOW2))
    REMOVE_BIT(ch->combat, COMBAT_CRUSH_BLOW2);

  ch->last_damage = time(nullptr);

  quint64 fight_length = ch->last_damage - ch->first_damage;
  if (fight_length < 1)
  {
    fight_length = 1;
  }

  ch->damage_per_second = ch->damage_done / fight_length;
  auto showdps = ch->getSetting("fighting.showdps", "0");
  if (showdps == "1" || showdps.startsWith('t', Qt::CaseInsensitive))
  {
    ch->send(u"You caused %lu damage over %lu seconds with DPS of %lu.\r\n"_s.arg(ch->damage_done).arg(fight_length).arg(ch->damage_per_second));
  }
}

void make_scraps(CharacterPtr ch, ObjectPtr obj)
{
  ObjectPtr corpse /*, o*/;
  QString buf;
  /*qint32 i;*/

  corpse = new Object;
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name(u"scraps"_s);

  dc_sprintf(buf, "A pile of scraps from %s is lying here.", qPrintable(obj->short_description()));
  corpse->long_description(buf);

  dc_sprintf(buf, "a pile of scraps.");
  corpse->short_description(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = {TAKE};
  corpse->obj_flags.value[0] = {};
  corpse->obj_flags.value[3] = {};
  corpse->obj_flags.weight = obj->obj_flags.weight;
  corpse->obj_flags.eq_level = {};

  corpse->obj_flags.more_flags = {};

  corpse->obj_flags.timer = {};

  object_list_new_new_owner(corpse, 0);
  obj_to_room(corpse, ch->in_room);
}

static constexpr quint64 MAX_NPC_CORPSE_TIME = 10;
static constexpr quint64 MAX_PC_CORPSE_TIME = 30;

void make_corpse(CharacterPtr ch)
{
  ObjectPtr corpse{}, *o{}, *o_in_container{}, next_o_in_container = {};
  ObjectPtr money{}, next_obj = {};

  QString buf = {};
  qint32 i = {};

  corpse = new Object;
  clear_object(corpse);
  if (ch->isPlayer())
  {
    corpse->setOwner(qPrintable(ch->name()));
  }

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;

  // If pc is in the name, the consent system works
  // Thieves don't deserve consent! Loot time!
  // Morc

  if (ch->isNonPlayer())
  {
    corpse->obj_flags.wear_flags = {};
    dc_sprintf(buf, "corpse %s", qPrintable(ch->name()));
  }
  else if (ch->isPlayerObjectThief())
  {
    corpse->obj_flags.wear_flags = {};
    dc_sprintf(buf, "corpse %s thiefcorpse", qPrintable(ch->name()));
  }
  else
  {
    corpse->obj_flags.wear_flags = {TAKE};

    if (ch->getLevel() >= 50)
      dc_sprintf(buf, "corpse %s pc lootable", qPrintable(ch->name()));
    else
      dc_sprintf(buf, "corpse %s pc", qPrintable(ch->name()));
  }
  corpse->name(buf);

  dc_sprintf(buf, "the corpse of %s is lying here.", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->long_description(buf);

  dc_sprintf(buf, "the corpse of %s", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->short_description(buf);

  corpse->obj_flags.type_flag = ITEM_CONTAINER;
  corpse->obj_flags.value[0] = {}; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1;  /* corpse identifier */
  corpse->obj_flags.weight = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  corpse->obj_flags.eq_level = {};

  SET_BIT(GET_OBJ_EXTRA(corpse), ITEM_UNIQUE_SAVE);
  if (ch->isNonPlayer())
  {
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
    corpse->obj_flags.more_flags = {};
    SET_BIT(corpse->obj_flags.more_flags, ITEM_NPC_CORPSE);
    GET_OBJ_VROOM(corpse) = DC::NOWHERE;
  }
  else
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
    SET_BIT(GET_OBJ_EXTRA(corpse), ITEM_PC_CORPSE);
    GET_OBJ_VROOM(corpse) = GET_ROOM_VNUM(ch->in_room);
  }

  if (ch->mobdata)
  {
    // Charmie/golem corpses get flag ITEM_LIMIT_SACRIFICE which limits them from being sacrificed
    // if they contain items
    if (ISSET(ch->mobdata->actflags, ACT_CHARM))
    {
      SET_BIT(corpse->obj_flags.more_flags, ITEM_LIMIT_SACRIFICE);
    }
    else
    {
      // Mob corpses with level 50+ equipment also get flag ITEM_LIMIT_SACRIFICE
      for (i = {}; i < MAX_WEAR; i++)
      {
        if (ch->equipment[i] && ch->equipment[i]->obj_flags.eq_level >= 50)
        {
          SET_BIT(corpse->obj_flags.more_flags, ITEM_LIMIT_SACRIFICE);
          break;
        }
      }

      // If the above didn't already set the corpse object with flag ITEM_LIMIT_SACRIFICE
      // then we search its inventory including container contents
      if (!isSet(corpse->obj_flags.more_flags, ITEM_LIMIT_SACRIFICE))
      {
        for (o = ch->carrying; o; o = next_obj)
        {
          next_obj = o->next_content;

          if (o->obj_flags.eq_level >= 50)
          {
            SET_BIT(corpse->obj_flags.more_flags, ITEM_LIMIT_SACRIFICE);
            break;
          }

          if (GET_ITEM_TYPE(o) == ITEM_CONTAINER)
          {
            for (o_in_container = o->contains; o_in_container; o_in_container = next_o_in_container)
            {
              next_o_in_container = o_in_container->next_content;
              if (o_in_container->obj_flags.eq_level >= 50)
              {
                SET_BIT(corpse->obj_flags.more_flags, ITEM_LIMIT_SACRIFICE);
                break;
              }
            } // if and for
          }
        } // for
      }
    }
  }

  // level 1-19 PC's can keep their eq
  if (ch->isNonPlayer() || ch->getLevel() > 19)
  {
    for (i = {}; i < MAX_WEAR; i++)
      if (ch->equipment[i])
        obj_to_char(ch->unequip_char(i), ch);

    if (ch->getGold() > 0)
    {
      money = create_money(ch->getGold());
      ch->setGold(0);
      obj_to_obj(money, corpse);
    }

    if (ch->isNonPlayer() && ch->getLevel() > 60 && dc_->number(1, 100) > 90) // 10%
    {
      ObjectPtr recipeitem = {};
      qint32 rarity = dc_->number(1, 100);
      bool itemtype = dc_->number(0, 1);
      if (rarity > 95) // 96-100 5%
      {
        if (itemtype == 0)
          recipeitem = clone_object(real_object(6324));
        else if (itemtype == 1)
          recipeitem = clone_object(real_object(6338));
      }
      else if (rarity > 85) // 85-95 10%
      {
        if (itemtype == 0)
          recipeitem = clone_object(real_object(6323));
        else if (itemtype == 1)
          recipeitem = clone_object(real_object(6339));
      }
      else if (rarity > 65) // 65-85 20%
      {
        if (itemtype == 0)
          recipeitem = clone_object(real_object(6322));
        else if (itemtype == 1)
          recipeitem = clone_object(real_object(6340));
      }
      else if (rarity > 40) // 41-65 25%
      {
        if (itemtype == 0)
          recipeitem = clone_object(real_object(6321));
        else if (itemtype == 1)
          recipeitem = clone_object(real_object(6341));
      }
      else // 1-40 40%
      {
        if (itemtype == 1)
          recipeitem = clone_object(real_object(6342));
        else if (itemtype == 0)
          recipeitem = clone_object(real_object(6320));
      }
      if (recipeitem != nullptr)
      {
        obj_to_obj(recipeitem, corpse);
      }
      else
      {
        QString bugmsg;
        dc_sprintf(bugmsg, "%s was supposed to drop a paper/bottle but was unable to create the item", qPrintable(ch->name()));
        dc_->logentry(bugmsg, IMMORTAL, DC::LogChannel::LOG_BUG);
      }
    }

    for (o = ch->carrying; o; o = next_obj)
    {
      next_obj = o->next_content;

      if (isSet(o->obj_flags.extra_flags, ITEM_SPECIAL) &&
          (GET_ITEM_TYPE(o) == ITEM_CONTAINER))
        for (o_in_container = o->contains; o_in_container; o_in_container = next_o_in_container)
        {
          next_o_in_container = o_in_container->next_content;
          if (!isSet(o_in_container->obj_flags.extra_flags, ITEM_SPECIAL))
          {
            move_obj(o_in_container, corpse);
          }
        } // if and for

      if (!isSet(o->obj_flags.extra_flags, ITEM_SPECIAL))
        move_obj(o, corpse);

    } // for
  }

  corpse->next = dc_->object_list;
  dc_->object_list = corpse;

  // TODO - i think this is taken care of in "obj_to_obj"...check it, and if so
  // remove this line.  (It's updating in_obj pointer for everything in corpse)
  for (o = corpse->contains; o; o->in_obj = corpse, o = o->next_content)
    ;

  object_list_new_new_owner(corpse, 0);
  obj_to_room(corpse, ch->in_room);

  if (corpse->contains)
    save_corpses();
}

void make_dust(CharacterPtr ch)
{
  ObjectPtr o, *tmp_o, blah;
  ObjectPtr money, next_obj;
  qint32 i;

  for (i = {}; i < MAX_WEAR; i++)
    if (ch->equipment[i])
      obj_to_char(ch->unequip_char(i), ch);

  if (ch->getGold() > 0)
  {
    money = create_money(ch->getGold());
    ch->setGold(0);
    obj_to_room(money, ch->in_room);
  }

  for (o = ch->carrying; o; o = next_obj)
  {
    next_obj = o->next_content;

    if (isSet(o->obj_flags.extra_flags, ITEM_SPECIAL) && (GET_ITEM_TYPE(o) == ITEM_CONTAINER))
      for (tmp_o = o->contains; tmp_o; tmp_o = blah)
      {
        blah = tmp_o->next_content;
        if (!isSet(tmp_o->obj_flags.extra_flags, ITEM_SPECIAL))
          move_obj(tmp_o, ch->in_room);

      } // if and for

    if (!isSet(o->obj_flags.extra_flags, ITEM_SPECIAL))
      move_obj(o, ch->in_room);

  } // for
}

qint32 alignment_value(qint32 val)
{
  if (val >= 350)
    return 1;
  if (val <= -350)
    return -1;
  return 0;
}

// run through eq removing and rewearing is
void zap_eq_check(CharacterPtr ch)
{
  SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT);
  for (qint32 i = {}; i < MAX_WEAR; i++)
    if (ch->equipment[i])
      ch->equip_char(ch->unequip_char(i, 1), i, 1);
  REMBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT);
}

// ch kills victim
void change_alignment(CharacterPtr ch, CharacterPtr victim)
{
  qint32 change;

  change = (GET_ALIGNMENT(victim) * 2) / 100;
  if (IS_NEUTRAL(ch))
    change /= 4;
  else
    change /= 2;
  GET_ALIGNMENT(ch) -= change;
  GET_ALIGNMENT(ch) = MIN(1000, MAX((-1000), GET_ALIGNMENT(ch)));

  if (change != alignment_value(GET_ALIGNMENT(ch)))
    zap_eq_check(ch);
}

/* head of a corpse or withered husk  */

void make_husk(CharacterPtr ch)
{
  ObjectPtr corpse;
  QString buf;

  corpse = new Object;
  clear_object(corpse);
  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name(u"husk"_s);
  dc_sprintf(buf, "The withered husk of %s, its soul drained, flutters here.", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->long_description(buf);
  dc_sprintf(buf, "Husk of %s", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->short_description(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = {};
  corpse->obj_flags.value[0] = {};
  corpse->obj_flags.value[3] = 1;
  corpse->obj_flags.weight = 1000;
  corpse->obj_flags.eq_level = {};
  if (ch->isNonPlayer())
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }
  obj_to_room(corpse, ch->in_room);
}

void make_head(CharacterPtr ch)
{
  ObjectPtr corpse;
  QString buf;

  corpse = new Object;
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name(u"head"_s);

  dc_sprintf(buf, "The head of %s is lying here.", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->long_description(buf);

  dc_sprintf(buf, "Head of %s", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->short_description(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = {TAKE, HOLD};
  corpse->obj_flags.value[0] = {}; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1;  /* corpse identifyer */
  corpse->obj_flags.weight = 5;
  corpse->obj_flags.eq_level = {};
  if (ch->isNonPlayer())
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  obj_to_room(corpse, ch->in_room);
}

void make_arm(CharacterPtr ch)
{
  ObjectPtr corpse;
  QString buf;

  corpse = new Object;
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name(u"arm"_s);

  dc_sprintf(buf, "The arm of %s is lying here.", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->long_description(buf);

  dc_sprintf(buf, "Arm of %s", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->short_description(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = {TAKE, HOLD};
  corpse->obj_flags.value[0] = {}; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1;  /* corpse identifyer */
  corpse->obj_flags.weight = 5;
  corpse->obj_flags.eq_level = {};
  if (ch->isNonPlayer())
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  obj_to_room(corpse, ch->in_room);
}

void make_leg(CharacterPtr ch)
{
  ObjectPtr corpse;
  QString buf;

  corpse = new Object;
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name(u"leg"_s);

  dc_sprintf(buf, "The leg of %s is lying here.", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->long_description(buf);

  dc_sprintf(buf, "Leg of %s", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->short_description(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = {TAKE, HOLD};
  corpse->obj_flags.value[0] = {}; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1;  /* corpse identifyer */
  corpse->obj_flags.weight = 5;
  corpse->obj_flags.eq_level = {};
  if (ch->isNonPlayer())
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  obj_to_room(corpse, ch->in_room);
}

void make_bowels(CharacterPtr ch)
{
  ObjectPtr corpse;
  QString buf;

  corpse = new Object;
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name(u"bowels"_s);

  dc_sprintf(buf, "The steaming bowels of %s is lying here.", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->long_description(buf);

  dc_sprintf(buf, "Bowels of %s", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->short_description(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = {TAKE, HOLD};
  corpse->obj_flags.value[0] = {}; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1;  /* corpse identifyer */
  corpse->obj_flags.weight = 5;
  corpse->obj_flags.eq_level = {};
  if (ch->isNonPlayer())
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  obj_to_room(corpse, ch->in_room);
}

void make_blood(CharacterPtr ch)
{
  ObjectPtr corpse;
  QString buf;

  corpse = new Object;
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name(u"blood"_s);

  dc_sprintf(buf, "A pool of %s's blood is here.", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->long_description(buf);

  dc_sprintf(buf, "Pooled blood of %s", (ch->isNonPlayer() ? qPrintable(ch->short_description()) : qPrintable(ch->name())));
  corpse->short_description(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = {TAKE, HOLD};
  corpse->obj_flags.value[0] = {}; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1;  /* corpse identifyer */
  corpse->obj_flags.weight = 5;
  corpse->obj_flags.eq_level = {};
  if (ch->isNonPlayer())
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  obj_to_room(corpse, ch->in_room);
}

void make_heart(CharacterPtr ch, CharacterPtr vict)
{
  ObjectPtr corpse;
  QString buf;

  if (!ch->hands_are_free(1))
    return;
  corpse = new Object;

  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name(u"heart"_s);

  dc_sprintf(buf, "%s's heart is laying here.", (vict->isNonPlayer() ? qPrintable(vict->short_description()) : qPrintable(vict->name())));
  corpse->long_description(buf);

  dc_sprintf(buf, "the heart of %s", (vict->isNonPlayer() ? qPrintable(vict->short_description()) : qPrintable(vict->name())));
  corpse->short_description(buf);

  corpse->obj_flags.type_flag = ITEM_FOOD;
  corpse->obj_flags.wear_flags = {TAKE, HOLD};
  corpse->obj_flags.value[0] = {}; /* You can't store stuff in a heart */
  corpse->obj_flags.value[3] = 1;  /* corpse identifier */
  corpse->obj_flags.weight = 2;
  corpse->obj_flags.eq_level = {};

  if (ch->isNonPlayer())
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = {};
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  if (!ch->equipment[WEAR_HOLD] && !ch->equipment[WEAR_WIELD] && !ch->equipment[WEAR_LIGHT])
    ch->equip_char(corpse, WEAR_HOLD);
  else if (!ch->equipment[WEAR_HOLD2] && !ch->equipment[WEAR_SECOND_WIELD])
    ch->equip_char(corpse, WEAR_HOLD2);
  else
    extract_obj(corpse);
}

void death_cry(CharacterPtr ch)
{
  qint32 door, was_in;
  const QString message;

  act("Your blood freezes as you hear $n's death cry.",
      ch, 0, 0, TO_ROOM, 0);

  if (ch->isNonPlayer())
    message = "You hear something's death cry.";
  else
    message = "You hear someone's death cry.";

  was_in = ch->in_room;
  for (door = {}; door <= 5; door++)
  {
    if (CAN_GO(ch, door))
    {
      ch->in_room = dc_->world[was_in].dir_option[door]->to_room;
      if (ch->in_room == was_in)
        continue;
      act_to_room(message, ch, 0, 0, 0);
      ch->in_room = was_in;
    }
  }
  ch->in_room = was_in;
}

// Return true if killed vict.  False otherwise
command_return_t do_skewer(CharacterPtr ch, CharacterPtr vict, qint32 dam, qint32 wt, qint32 wt2, qint32 weapon)
{
  qint32 damadd = {};

  if ((GET_CLASS(ch) != CLASS_WARRIOR) && ch->getLevel() < ARCHANGEL)
    return 0;
  if (vict->isPlayer() && vict->getLevel() >= IMMORTAL)
    return 0;
  if (!ch->equipment[weapon])
    return 0;
  if (!ch->has_skill(SKILL_SKEWER))
    return 0;
  // TODO - need to make this take specialization into consideration
  if (ch->in_room != vict->in_room)
    return 0;

  if (ch->affected_by_spell(SKILL_DEFENDERS_STANCE))
    return 0;

  qint32 type = get_weapon_damage_type(ch->equipment[weapon]);
  if (!(type == TYPE_PIERCE || type == TYPE_SLASH || type == TYPE_STING))
    return 0;
  if (!skill_success(ch, vict, SKILL_SKEWER))
    return 0;

  if (ch->dc_->number(0, 100) < 25)
  {
    if (check_dodge(ch, vict, type, false))
    {
      act_to_room("$n dodges $N's skewer!!", vict, nullptr, ch, NOTVICT);
      act_to_victim("$n dodges your skewer!!", vict, nullptr, ch, 0);
      act_to_character("You dodge $N's skewer!!", vict, nullptr, ch, 0);
      return 0;
    }
    if (check_parry(ch, vict, type, false))
    {
      act_to_room("$n parries $N's skewer!!", vict, nullptr, ch, NOTVICT);
      act_to_victim("$n parries your skewer!!", vict, nullptr, ch, 0);
      act_to_character("You parry $N's skewer!!", vict, nullptr, ch, 0);
      return 0;
    }
    if (check_shieldblock(ch, vict, type))
    {
      // act_to_room("$2$n shield blocks $N's skewer!!$R", ch, 0, vict,  NOTVICT);
      return 0;
    }

    //  act_to_room("$n jams $s weapon into $N!!", ch, 0, vict,  NOTVICT);
    //  act_to_character("You jam your weapon in $N's heart!", ch, 0, vict,  0);
    //  act_to_victim("$n's weapon is speared into you! Ouch!", ch, 0, vict,  0);
    damadd = (qint32)(dam * 1.5);
    qint32 retval = damage(ch, vict, damadd, wt, SKILL_SKEWER, weapon);
    if (SOMEONE_DIED(retval))
      return debug_retval(ch, vict, retval);
    // vict->removeHP(damadd);
    update_pos(vict);
    inform_victim(ch, vict, damadd);

    if (GET_POS(vict) != position_t::DEAD && dc_->number(0, 4999) == 1)
    { /* tiny chance of instakill */
      vict->setHP(-1, ch);
      ch->sendln("You impale your weapon through your opponent's chest!");
      act_to_victim("$n's weapon blows through your chest sending your entrails flying for yards behind you.  Everything goes black...", ch, 0, vict, 0);
      act("$n's weapon rips through $N's chest sending gore and entrails flying for yards!\r\n", ch, 0, vict, NOTVICT, 0);
      // duplicate message   act_to_room("$n is DEAD!!", vict, 0, 0,  INVIS_NULL);
      vict->sendln("You have been SKEWERED!!\r\n");
      damage(ch, vict, 9999999, TYPE_UNDEFINED, SKILL_SKEWER, weapon);
      //      update_pos(vict);
      return ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED;
    }
    return debug_retval(ch, vict, retval);
  }
  // if they're still here the skewer missed
  return ReturnValue::eSUCCESS;
}

command_return_t do_behead_skill(CharacterPtr ch, CharacterPtr vict)
{
  qint32 chance, percent;

  percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
  chance = dc_->number(0, 101);
  if (chance > (1.3 * percent))
  {
    percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
    chance = dc_->number(0, 101);
    if (chance > (2 * percent))
    {
      chance = dc_->number(0, 101);
      if (chance > (2 * percent))
      {
        chance = dc_->number(0, 101);
        if (chance > (2 * percent) && !isSet(vict->immune, ISR_SLASH) && skill_success(ch, vict, SKILL_BEHEAD))
        {
          if (((vict->equipment[WEAR_NECK_1] && dc_->obj_index[vict->equipment[WEAR_NECK_1]->item_number].vnum() == 518) ||
               (vict->equipment[WEAR_NECK_2] && dc_->obj_index[vict->equipment[WEAR_NECK_2]->item_number].vnum() == 518)) &&
              !number(0, 1))
          { // tarrasque's leash..
            act_to_character("You attempt to behead $N, but your sword bounces of $S neckwear.", ch, 0, vict, 0);
            act_to_room("$n attempts to behead $N, but fails.", ch, 0, vict, NOTVICT);
            act_to_victim("$n attempts to behead you, but cannot cut through your neckwear.", ch, 0, vict, 0);
            return ReturnValue::eSUCCESS;
          }
          if (IS_AFFECTED(vict, AFF_NO_BEHEAD))
          {
            act_to_character("$N deftly dodges your beheading attempt!", ch, 0, vict, 0);
            act_to_room("$N deftly dodges $n's attempt to behead $M!", ch, 0, vict, NOTVICT);
            act_to_victim("You deftly avoid $n's attempt to lop your head off!", ch, 0, vict, 0);
            return ReturnValue::eSUCCESS;
          }
          act_to_victim("You feel your life end as $n's sword SLICES YOUR HEAD OFF!", ch, 0, vict, 0);
          act_to_character("You SLICE $N's head CLEAN OFF $S body!", ch, 0, vict, 0);
          act_to_room("$n cleanly slices $N's head off $S body!", ch, 0, vict, NOTVICT);
          vict->setHP(-20, ch);
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
          return ReturnValue::eSUCCESS;
        }
      }
    }
  }
  return ReturnValue::eFAILURE;
}

command_return_t do_execute_skill(CharacterPtr ch, CharacterPtr vict, qint32 w_type)
{
  qint32 chance, percent;

  percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
  chance = dc_->number(0, 101);
  if (chance > (1.3 * percent))
  {
    percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
    chance = dc_->number(0, 101);
    if (chance > (2 * percent))
    {
      chance = dc_->number(0, 101);
      if (chance > (2 * percent))
      {
        if (IS_AFFECTED(vict, AFF_NO_BEHEAD) ||
            (w_type == TYPE_SLASH && isSet(vict->immune, ISR_SLASH)) ||
            (w_type == TYPE_PIERCE && isSet(vict->immune, ISR_PIERCE)) ||
            (w_type == TYPE_CRUSH && isSet(vict->immune, ISR_CRUSH)) ||
            (w_type == TYPE_BLUDGEON && isSet(vict->immune, ISR_BLUDGEON)) ||
            (w_type == TYPE_WHIP && isSet(vict->immune, ISR_WHIP)) ||
            (w_type == TYPE_STING && isSet(vict->immune, ISR_STING)))
        {
          act_to_character("$N deftly dodges your mortal strike!", ch, 0, vict, 0);
          act_to_room("$N deftly dodges $n's mortal strike!", ch, 0, vict, NOTVICT);
          act_to_victim("You deftly avoid $n's mortal strike!", ch, 0, vict, 0);
          return ReturnValue::eSUCCESS;
        }
        else if (!skill_success(ch, vict, SKILL_EXECUTE))
        {
          act_to_character("$N narrowly avoids your lethal blow as you attempt to thrust aside $S defenses!", ch, 0, vict, 0);
          act_to_victim("You narrowly avoid a lethal blow as $n attempts to thrust aside your defenses!", ch, 0, vict, 0);
          act_to_room("$N narrowly avoids $n's lethal blow as $e attempts to thrust aside $S defenses! ", ch, 0, vict, NOTVICT);
          return ReturnValue::eSUCCESS;
        }
        else
        {
          act_to_victim("$n quickly thrusts aside your defenses and strikes a fatal blow!", ch, 0, vict, 0);
          act_to_victim("You feel a flash of pain and your vision dims from $4red$R to $B$0black$R...", ch, 0, vict, 0);
          vict->setHP(-20, ch);
          act_to_character("You quickly thrust aside $N's defenses and strike a fatal blow!", ch, 0, vict, 0);
          act_to_room("$n quickly thrusts aside $N's defenses and strikes a lethal blow!", ch, 0, vict, NOTVICT);
          if (w_type == TYPE_SLASH || w_type == TYPE_PIERCE)
          {
            if (ch->dc_->number(0, 1))
            {
              act_to_character("$N's arm is neatly severed from $S battered body as it crumples to the ground.", ch, 0, vict, 0);
              act_to_room("$N's arm is neatly severed from $S battered body as it crumples to the ground.", ch, 0, vict, NOTVICT);
              make_arm(vict);
            }
            else
            {
              act_to_character("$N's leg is neatly severed from $S battered body as it crumples to the ground.", ch, 0, vict, 0);
              act_to_room("$N's leg is neatly severed from $S battered body as it crumples to the ground.", ch, 0, vict, NOTVICT);
              make_leg(vict);
            }
          }
          if (w_type == TYPE_CRUSH || w_type == TYPE_BLUDGEON)
          {
            act_to_character("$N's guts spill to the ground as $S body is split open like an overripe melon.", ch, 0, vict, 0);
            act_to_room("$N's guts spill to the ground as $S body is split open like an overripe melon.", ch, 0, vict, NOTVICT);
            make_bowels(vict);
          }
          if (w_type == TYPE_WHIP || w_type == TYPE_STING)
          {
            act_to_character("$N's blood pools on the ground as the remaining life seeps from $S body.", ch, 0, vict, 0);
            act_to_room("$N's blood pools on the ground as the remaining life seeps from $S body.", ch, 0, vict, NOTVICT);
            make_blood(vict);
          }
          group_gain(ch, vict);
          fight_kill(ch, vict, TYPE_CHOOSE, 0);
          return ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED; /* Zero means kill it! */
        }
      }
    }
  }
  return ReturnValue::eFAILURE;
}

void do_combatmastery(CharacterPtr ch, CharacterPtr vict, qint32 weapon)
{
  if ((GET_CLASS(ch) != CLASS_WARRIOR) && ch->getLevel() < ARCHANGEL)
    return;
  if (vict->isPlayer() && vict->getLevel() >= IMMORTAL)
    return;
  if (!ch->equipment[weapon])
    return;
  if (!ch->has_skill(SKILL_COMBAT_MASTERY))
    return;
  if (ch->in_room != vict->in_room)
    return;

  qint32 type = get_weapon_damage_type(ch->equipment[weapon]);
  if (type != TYPE_STING && type != TYPE_WHIP && type != TYPE_CRUSH && type != TYPE_BLUDGEON)
    return;

  if (!skill_success(ch, vict, SKILL_COMBAT_MASTERY))
    return;

  if (ch->dc_->number(0, 8))
    return; // Chance lowered

  if (type == TYPE_STING)
  {
    if (!IS_AFFECTED(vict, AFF_BLIND))
    {
      affected_type af;
      af.type = SKILL_COMBAT_MASTERY;
      af.location = APPLY_HITROLL;
      af.modifier = vict->has_skill(SKILL_BLINDFIGHTING) ? skill_success(vict, 0, SKILL_BLINDFIGHTING) ? -10 : -20 : -20;
      af.duration = 1;
      af.bitvector = AFF_BLIND;
      affect_to_char(vict, &af);

      af.location = APPLY_AC;
      af.modifier = vict->has_skill(SKILL_BLINDFIGHTING) ? skill_success(vict, 0, SKILL_BLINDFIGHTING) ? +25 : 50 : 50;
      affect_to_char(vict, &af);

      act_to_character("Your attack stings $N's eyes, causing momentary blindness!", ch, 0, vict, 0);
      act_to_victim("$n's attack stings your eyes, causing momentary blindness!", ch, 0, vict, 0);
      act_to_room("$n's attack stings $N's eyes, causing momentary blindness!", ch, 0, vict, NOTVICT);
    }
  }
  if (type == TYPE_BLUDGEON || type == TYPE_CRUSH)
  {
    if (vict->getLevel() >= 90 || (vict->isNonPlayer() && ISSET(vict->mobdata->actflags, ACT_HUGE)))
    {
      act_to_character("$N shakes off your crushing blow!", ch, 0, vict, 0);
      act_to_room("$N shakes off $n's crushing blow!", ch, 0, vict, NOTVICT);
      act_to_victim("You shake off $n's crushing blow!", ch, 0, vict, 0);
      return;
    }
    if (vict->getLevel() >= 90 || (vict->isNonPlayer() && ISSET(vict->mobdata->actflags, ACT_SWARM)))
    {
      act_to_character("$N swarms around your crushing blow!", ch, 0, vict, 0);
      act_to_room("$N swarms around $n's crushing blow!", ch, 0, vict, NOTVICT);
      act_to_victim("You swarm around $n's crushing blow!", ch, 0, vict, 0);
      return;
    }
    if (vict->getLevel() >= 90 || (vict->isNonPlayer() && ISSET(vict->mobdata->actflags, ACT_TINY)))
    {
      act_to_character("$N is so small, $E easily avoids your crushing blow!", ch, 0, vict, 0);
      act_to_room("$N easily avoids $n's slow, crushing blow!", ch, 0, vict, NOTVICT);
      act_to_victim("You easily avoid $n's slow, crushing blow!", ch, 0, vict, 0);
      return;
    }
    if (!isSet(vict->combat, COMBAT_CRUSH_BLOW))
    {
      SET_BIT(vict->combat, COMBAT_CRUSH_BLOW);
      act_to_character("Your crushing blow causes $N's attacks to momentarily weaken!", ch, 0, vict, 0);
      act_to_victim("$n's crushing blow causes your attacks to momentarily weaken!", ch, 0, vict, 0);
      act_to_room("$n's crushing blow causes $N's attacks to momentarily weaken!", ch, 0, vict, NOTVICT);
    }
  }
  if (type == TYPE_WHIP && !ch->affected_by_spell(SKILL_CM_TIMER))
  {
    if (GET_POS(vict) > position_t::SITTING && !vict->affected_by_spell(SPELL_IRON_ROOTS))
    {
      vict->setSitting();
      SET_BIT(vict->combat, COMBAT_BASH2);
      WAIT_STATE(vict, DC::PULSE_VIOLENCE);
      act_to_character("Your whipping attack trips up $N and $E goes down!", ch, 0, vict, 0);
      act_to_victim("$n's whipping attack trips you up, causing you to stumble and fall!", ch, 0, vict, 0);
      act_to_room("$n's whipping attack trips up $N causing $M to stumble and fall!", ch, 0, vict, NOTVICT);

      affected_type af;
      af.type = SKILL_CM_TIMER;
      af.location = {};
      af.modifier = {};
      af.duration = dc_->number(3, 4);
      af.bitvector = -1;
      affect_to_char(ch, &af, DC::PULSE_VIOLENCE);
    }
  }
}

void raw_kill(CharacterPtr ch, CharacterPtr victim)
{
  QString buf;
  QString buf2;
  bool is_thief = {};
  qint32 death_room = {};

  if (!victim)
  {
    dc_->logentry(u"Error in raw_kill()!  Null victim!"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (victim->room().isArena())
  {
    fight_kill(ch, victim, TYPE_ARENA_KILL, 0);
    return;
  }

  if (IS_AFFECTED(victim, AFF_CHAMPION))
  {
    REMBIT(victim->affected_by, AFF_CHAMPION);
    do_champ_flag_death(victim);
  }
  if (ch && ch->isImmortalPlayer() && victim->isNonPlayer())
  {
    special_log(QString(u"%1 killed %2 in room %3!"_s.arg(ch->name()).arg(victim->name()).arg(ch->in_room));
  }

  // register my death with this zone's counter
  DC::incrementZoneDiedTick(dc_->world[victim->in_room].zone);

  victim->setStanding();
  qint32 retval = mprog_death_trigger(victim, ch);
  if (SOMEONE_DIED(retval))
    return;
  victim->setDead();

  if (victim->race == RACE_UNDEAD ||
      victim->race == RACE_GHOST ||
      victim->race == RACE_ELEMENT ||
      victim->race == RACE_PLANAR ||
      victim->race == RACE_SLIME ||
      (victim->isNonPlayer() && dc_->mob_index[victim->mobdata->nr].vnum() == 8))
    make_dust(victim);
  else
    make_corpse(victim);

  if (ch != nullptr && IS_AFFECTED(ch, AFF_GROUP))
  {
    CharacterPtr master = ch->master ? ch->master : ch;
    if (master->isPlayer())
    {
      if (victim->isPlayer())
      {
        master->player->grpplvl += victim->getLevel();
        master->player->group_pkills += 1;
      }
      master->player->group_kills += 1;
    }
  }

  if (victim->isNonPlayer())
  {
    if (ch == victim)
    {
      if (ch == nullptr)
      {
        dc_->logf(0, DC::LogChannel::LOG_BUG, "selfpurge on nullptr to nullptr");
      }
      else
      {
        // dc_->logf(0, DC::LogChannel::LOG_BUG, "selfpurge on %s to %s", qPrintable(ch->name()), qPrintable(victim->name()));
      }
      selfpurge = true;
      selfpurge.setOwner(ch, "raw_kill");
    }
    extract_char(victim, true);
    return;
  }

  if (victim->followers || victim->master)
  {
    stop_grouped_bards(victim, !IS_SINGING(victim));
  }

  if (victim->player->golem)
  {
    void release_message(CharacterPtr ch);
    void shatter_message(CharacterPtr ch);

    if (ch->dc_->number(0, 99) < (victim->getLevel() / 10 + victim->player->golem->getLevel() / 5))
    { /* rk */
      QString buf;
      dc_sprintf(buf, "%s's golem lost a level!", qPrintable(victim->name()));
      dc_->logentry(buf, ANGEL, DC::LogChannel::LOG_MORTAL);
      shatter_message(victim->player->golem);
      extract_char(victim->player->golem, true);
    }
    else
    { /* release */
      release_message(victim->player->golem);
      extract_char(victim->player->golem, false);
    }
  }
  victim->player->group_pkills = {};
  victim->player->grpplvl = {};
  victim->player->group_kills = {};
  if (isSet(victim->player->punish, PUNISH_SPAMMER))
    REMOVE_BIT(victim->player->punish, PUNISH_SPAMMER);
  if (victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
  {
    is_thief = 1;
    affect_from_char(victim, Character::PLAYER_OBJECT_THIEF);
  }
  if (victim->isPlayerGoldThief())
  {
    if (victim->getGold() > 0)
    {
      act_to_room("$n drops $s stolen booty!", victim, 0, 0, 0);
      obj_to_room(create_money(victim->getGold()), victim->in_room);
      victim->setGold(0);
      victim->save_char_obj();
    }
  }

  victim->setResting();
  death_room = victim->in_room;
  extract_char(victim, false);

  victim->setHP(1);
  if (victim->getMove() == 0)
  {
    victim->setMove(1);
  }

  if (GET_MANA(victim) <= 0)
    GET_MANA(victim) = 1;
  add_totem_stats(victim);
  if (GET_CLASS(victim) == CLASS_MONK)
    GET_AC(victim) -= (victim->getLevel() * 2);
  GET_AC(victim) -= victim->has_skill(SKILL_COMBAT_MASTERY) / 2;
  GET_COND(victim, FULL) = {};
  GET_COND(victim, THIRST) = {};

  if (victim && isSet(victim->combat, COMBAT_BERSERK))
  {
    GET_AC(victim) -= 30;
    victim->combat = {};
  }

  victim->save_char_obj();

  const auto &character_list = dc_->character_list;
  for (const auto &i : character_list)
  {

    remove_memory(i, 'h', victim);
    if (i->isNonPlayer() && (i->mobdata->fears))
      if (!dc_strcmp(i->mobdata->fears, qPrintable(victim->name())))
        remove_memory(i, 'f');
    if (i->isNonPlayer() && !i->hunting.isEmpty())
      if (i->hunting == victim->name())
        remove_memory(i, 't');
  }

  /* If we're still here we can thrash the victim */
  if (victim->isPlayer())
  { /* Only log player deaths */
    if (ch)
    {
      if (ch->mobdata)
      {
        dc_sprintf(buf, "%s killed by %lu (%s)", qPrintable(victim->name()), dc_->mob_index[ch->mobdata->nr].vnum(), qPrintable(ch->name()));
      }
      else
      {
        dc_sprintf(buf, "%s killed by %s", qPrintable(victim->name()), qPrintable(ch->name()));
      }
    }
    else
    {
      dc_sprintf(buf, "%s killed by [null killer]", qPrintable(victim->name()));
    }

    // notify the clan members - clan_death checks for null ch/vict
    clan_death(victim, ch);
    QString log_buf = {};
    dc_sprintf(log_buf, "%s at %d", buf, dc_->world[death_room].number);
    dc_->logentry(log_buf, ANGEL, DC::LogChannel::LOG_MORTAL);

    // update stats
    GET_RDEATHS(victim) += 1;

    /* gods don't suffer from stat loss */
    if (victim->isMortalPlayer() && victim->getLevel() > 19)
    {
      /* New death system... dying is a BITCH!  */
      // thief + mob kill = stat loss
      // or got a bad roll

      if (is_thief)
        pir_stat_loss(victim, 100, true, is_thief);
      else if (victim->getLevel() > 20)
      {
        qint32 chance = ch ? ch->getLevel() / 10 : 50 / 10;
        chance += victim->getLevel() / 2;
        if (victim->getLevel() >= 50)
        {
          chance += (qint32)(25.0 * (qreal)((qreal)(ch ? ch->getLevel() : 50) / 100.0) * (qreal)((qreal)(ch ? ch->getLevel() : 50) / 100.0));
          // An extra 1% for each level over 50.
          chance += victim->getLevel() - 50;
        }

        if (ch->dc_->number(0, 99) < chance)
        {
          if (victim->race != RACE_TROLL)
          {
            GET_CON(victim) -= 1;
            victim->raw_con -= 1;
            victim->sendln("*** You lose one constitution point ***");
            if (victim->isPlayer())
            {
              dc_sprintf(log_buf, "%s lost a con. ouch.", qPrintable(victim->name()));
              dc_->logentry(log_buf, SERAPH, DC::LogChannel::LOG_MORTAL);
              victim->player->statmetas--;
            }
          }
          else
          {
            GET_DEX(victim) -= 1;
            victim->raw_dex -= 1;
            victim->sendln("*** You lose one dexterity point ***");
            if (victim->isPlayer())
            {
              dc_sprintf(log_buf, "%s lost a dex. ouch.", qPrintable(victim->name()));
              dc_->logentry(log_buf, SERAPH, DC::LogChannel::LOG_MORTAL);
              victim->player->statmetas--;
            }
          }
          pir_stat_loss(victim, chance, false, is_thief);
        }
      }
      victim->check_maxes(); // Check if any skills got lowered because of
                             // stat loss. guild.cpp.
      // hmm
      if (GET_CON(victim) <= 4)
      {
        victim->sendln("Your Constitution has reached 4...you are permanently dead!");
        send_to_char("\r\n"
                     "         (buh bye, - pirahna)\r\n"
                     "        O  ,-----------,\r\n"
                     "       o  /             \\  /|\r\n"
                     "       . /  0            \\/ |\r\n"
                     "        |                   |\r\n"
                     "         \\               /\\ |\r\n"
                     "          \\             /  \\|\r\n"
                     "           `-----------`\r\n",
                     victim);
        auto name = victim->name();
        do_quit(victim, "", cmd_t::SAVE_SILENTLY);
        remove_familiars(name, CONDEATH);
        dc_->vaults_.remove_vault(name, CONDEATH);
        if (victim->clan)
        {
          remove_clan_member(victim->clan, victim);
        }
        remove_character(name, CONDEATH);

        dc_sprintf(buf2, "%s permanently dies.", qPrintable(name));
        dc_->logentry(buf2, ANGEL, DC::LogChannel::LOG_MORTAL);
        return;
      }
      else if (GET_INT(victim) <= 4)
      {
        victim->sendln("Your Intelligence has reached 4...you are permanently dead!");
        send_to_char("\r\n"
                     "                     At least you have something nice to look at before your character is erased! - Wendy\r\n"
                     "   888   M:::::::::::::M8888888888888M:::::mM888888888888888    8888\r\n"
                     "    888  M::::::::::::M8888:888888888888::::m::Mm88888 888888   8888\r\n"
                     "     88  M::::::::::::8888:88888888888888888::::::Mm8   88888   888\r\n"
                     "     88  M::::::::::8888M::88888::888888888888:::::::Mm88888    88\r\n"
                     "     8   MM::::::::8888M:::8888:::::888888888888::::::::Mm8     4\r\n"
                     "         8M:::::::8888M:::::888:::::::88:::8888888::::::::Mm    2\r\n"
                     "        88MM:::::8888M:::::::88::::::::8:::::888888:::M:::::M\r\n"
                     "       8888M:::::888MM::::::::8:::::::::::M::::8888::::M::::M\r\n"
                     "      88888M:::::88:M::::::::::8:::::::::::M:::8888::::::M::M\r\n"
                     "     88 888MM:::888:M:::::::::::::::::::::::M:8888:::::::::M:\r\n"
                     "     8 88888M:::88::M:::::::::::::::::::::::MM:88::::::::::::M\r\n"
                     "       88888M:::88::M::::::::::*88*::::::::::M:88::::::::::::::M\r\n"
                     "      888888M:::88::M:::::::::88@@88:::::::::M::88::::::::::::::M\r\n"
                     "      888888MM::88::MM::::::::88@@88:::::::::M:::8::::::::::::::*8\r\n"
                     "      88888  M:::8::MM:::::::::*88*::::::::::M:::::::::::::::::88@@\r\n"
                     "      8888   MM::::::MM:::::::::::::::::::::MM:::::::::::::::::88@@\r\n"
                     "       888    M:::::::MM:::::::::::::::::::MM::M::::::::::::::::*8\r\n"
                     "       888    MM:::::::MMM::::::::::::::::MM:::MM:::::::::::::::M\r\n"
                     "        88     M::::::::MMMM:::::::::::MMMM:::::MM::::::::::::MM\r\n"
                     "         88    MM:::::::::MMMMMMMMMMMMMMM::::::::MMM::::::::MMM\r\n"
                     "          88    MM::::::::::::MMMMMMM::::::::::::::MMMMMMMMMM\r\n"
                     "           88   8MM::::::::::::::::::::::::::::::::::MMMMM\r\n"
                     "           88   8MM::::::::::::::::::::::::::::::::::MMMMMM\r\n"
                     "           8   88MM::::::::::::::::::::::M:::M::::::::MM\r\n",
                     victim);

        auto name = victim->name();
        do_quit(victim, "", cmd_t::SAVE_SILENTLY);

        remove_familiars(name, CONDEATH);
        dc_->vaults_.remove_vault(name, CONDEATH);
        if (victim->clan)
        {
          remove_clan_member(victim->clan, victim);
        }
        remove_character(name, CONDEATH);

        dc_sprintf(buf2, "%s sees tits.", qPrintable(name));
        dc_->logentry(buf2, ANGEL, DC::LogChannel::LOG_MORTAL);
        return;
      }
      else if (GET_WIS(victim) <= 4)
      {
        victim->sendln("Your Wisdom has reached 4...you are permanently dead!");
        send_to_char("\r\n"
                     "    	The other stat deaths have alot fancier ASCII pics.\r\n"
                     "          =,    (\\_/)    ,=\r\n"
                     "           /`-'--(\")--'-'\\\r\n"
                     "          /     (___)     \\\r\n"
                     "         /.-.-./ \" \" \\.-.-.\\\r\n",
                     victim);

        auto name = victim->name();
        do_quit(victim, "", cmd_t::SAVE_SILENTLY);

        remove_familiars(name, CONDEATH);
        dc_->vaults_.remove_vault(name, CONDEATH);
        if (victim->clan)
        {
          remove_clan_member(victim->clan, victim);
        }
        remove_character(name, CONDEATH);

        dc_sprintf(buf2, "%s gets batted to death.", qPrintable(name));
        dc_->logentry(buf2, ANGEL, DC::LogChannel::LOG_MORTAL);
        return;
      }
      else if (GET_STR(victim) <= 4)
      {
        victim->sendln("Your Strength has reached 4...you are permanently dead!");
        send_to_char("\r\n"
                     "           To moose heaven with you! - Apoc\r\n"
                     "    _/\\_       __/\\__\r\n"
                     "   ) . (_    _) .' (\r\n"
                     "   `) '.(   ) .'  (`\r\n"
                     "    `-._\\(_ )/__(~`\r\n"
                     "        (ovo)-.__.--._\r\n"
                     "        )             `-.______\r\n"
                     "       /                       `---._\r\n"
                     "      ( ,// )                        \\\\r\n"
                     "       `''\\/-.                        |\r\n"
                     "              \\                       | \r\n"
                     "              |                       |\r\n",
                     victim);

        auto name = victim->name();
        do_quit(victim, "", cmd_t::SAVE_SILENTLY);

        remove_familiars(name, CONDEATH);
        dc_->vaults_.remove_vault(name, CONDEATH);
        if (victim->clan)
        {
          remove_clan_member(victim->clan, victim);
        }
        remove_character(name, CONDEATH);

        dc_sprintf(buf2, "%s goes to moose heaven.", qPrintable(name));
        dc_->logentry(buf2, ANGEL, DC::LogChannel::LOG_MORTAL);
        return;
      }
      else if (GET_DEX(victim) <= 4 && victim->race == RACE_TROLL)
      {
        victim->sendln("Your Dexterity has reached 4...you are permanently dead!");
        send_to_char("\r\n"
                     " Dear Mudder, you suck.\r\nSincerely - Urizen\r\n"
                     "$4              /                   \\\r\n"
                     "             /|      ,             |\\\r\n"
                     "           /' |     /(     )\\      | `\\\r\n"
                     "         /'   \\    | `~~~~~' |    /    `\\\r\n"
                     "       /'      \\   \\  $1\\$4 , $1/$4  /   /      `\\\r\n"
                     "     /C         \\   |  ___  |   /        C\\\r\n"
                     "    OC       |   `\\  \\ ` ' /  /'   |      CO\r\n"
                     "   OC   \\    |   __\\_/~\\ /~\\_/__   |   \\   CO\r\n"
                     "  Oo   |      \\/'  `    '    '  `\\/     |   oO\r\n"
                     " OC    |      |         :         |     |    CO\r\n"
                     "oOC    /~~~\\_ |    ;,__,',__,;    | _/~~~\\   COo\r\n"
                     "oOC   |      \\\\   '\\   _|_   /`   //      |  COo\r\n"
                     "oOC   |       ~\\    |  _|_   |   /~       |  COo\r\n"
                     "oOC    \\     ,  \\   \\_______/   /  ,     /   COo\r\n"
                     " OC     `\\    \\  \\   \\_____/   /  /    /'    CO\r\n"
                     "  O       `\\   \\, \\   \\   /   / ,/   /'      O\r\n"
                     "   O    /~~\\\\   \\\\_\\  |___|  /_//   /~~\\    O\r\n"
                     "    \\  |    `\\,  \\ |  |   |  | /  ,/    |  /\r\n"
                     "     \\ |     /   /  '''    ``` \\   \\    | /\r\n"
                     "      \\|    /   /               \\   \\   |/\r\n"
                     "       `\\   VVV~                 ~VVV  /'$R\r\n",
                     victim);

        auto name = victim->name();
        do_quit(victim, "", cmd_t::SAVE_SILENTLY);

        remove_familiars(name, CONDEATH);
        dc_->vaults_.remove_vault(name, CONDEATH);
        if (victim->clan)
        {
          remove_clan_member(victim->clan, victim);
        }
        remove_character(name, CONDEATH);

        dc_sprintf(buf2, "%s permanently dies the horrible dex-death.", qPrintable(name));
        dc_->logentry(buf2, ANGEL, DC::LogChannel::LOG_MORTAL);
        return;
      }
    }

    qreal penalty = 1;
    if (ch)
      penalty += ch->getLevel() * .05;
    penalty = MIN(penalty, 2);
    victim->exp = (qint64)(victim->exp / penalty);
  } // isPlayer()
}

void group_gain(CharacterPtr ch, CharacterPtr victim)
{
  QString buf;
  qint32 no_members = 0, total_levels = {};
  qint64 share, total_share = {};
  qint64 base_xp = 0, bonus_xp = {};
  CharacterPtr leader, highest, tmp_ch;
  follow_type *f;

  if (is_pkill(ch, victim))
    return;
  if (ch == victim)
    return;
  if (victim->isPlayer())
    return;

  if (ch->isNonPlayer() && !(IS_AFFECTED(ch, AFF_CHARM) || IS_AFFECTED(ch, AFF_FAMILIAR)))
    return; // non charmies/familiars get out

  // if i'm charmie/familiar and not grouped, give my master the credit if he's in room
  if (ch->isNonPlayer() && ch->master && ch->in_room == ch->master->in_room)
    ch = ch->master;

  // Set group leader
  if (!(leader = ch->master) || !IS_AFFECTED(ch, AFF_GROUP))
    leader = ch;

  highest = get_highest_level_killer(leader, ch);
  no_members = count_xp_eligibles(leader, ch, highest->getLevel(), &total_levels);

  // loop with leader first, then all the followers
  tmp_ch = leader;
  f = leader->followers;
  do
  {
    if ((tmp_ch->in_room != ch->in_room) ||
        ((!IS_AFFECTED(tmp_ch, AFF_GROUP)) && (no_members > 1)) ||
        (!IS_AFFECTED(tmp_ch, AFF_GROUP) && tmp_ch != ch) ||
        ((tmp_ch != ch) && (!IS_AFFECTED(ch, AFF_GROUP))))
    {
      tmp_ch = loop_followers(&f);
      continue;
    }

    level_diff_t level_difference = tmp_ch->getLevel() - highest->getLevel();
    if (level_difference <= -51 && tmp_ch->isPlayer())
    {
      act_to_character("You are too low for this group.  You gain no experience.", tmp_ch, 0, 0, 0);

      tmp_ch = loop_followers(&f);
      continue;
    }

    // Charmies dont steal xp whether they're in a group or not
    if (tmp_ch->isNonPlayer() && (IS_AFFECTED(tmp_ch, AFF_CHARM) || IS_AFFECTED(tmp_ch, AFF_FAMILIAR)))
    {
      tmp_ch = loop_followers(&f);
      continue;
    }

    /* calculate base XP value */
    base_xp = victim->exp;
    if (IS_AFFECTED(victim, AFF_CHARM))
    {
      share = {};
      base_xp = {};
      bonus_xp = {};
    }
    /* calculate this character's share of the XP */
    else
    {
      share = scale_char_xp(tmp_ch, ch, victim, no_members, total_levels, highest->getLevel(), base_xp, &bonus_xp);
    }

    if (IS_AFFECTED(tmp_ch, AFF_CHAMPION))
      share = (qint32)((double)share * 1.10);
    dc_sprintf(buf, "You receive %ld exps of %ld total.\r\n", share, base_xp + bonus_xp);
    tmp_ch->send(buf);
    gain_exp(tmp_ch, share);
    total_share += share;
    change_alignment(tmp_ch, victim);

    // this loops the followers (cut and pasted above)
    tmp_ch = loop_followers(&f);
  } while (tmp_ch);
  getAreaData(dc_->world[victim->in_room].zone, dc_->mob_index[victim->mobdata->nr].vnum(), total_share, victim->getGold());
}

/* find the highest level present at the kill */
CharacterPtr get_highest_level_killer(CharacterPtr leader, CharacterPtr killer)
{
  follow_type *f;
  CharacterPtr highest = killer;

  /* check to see if the group leader was involved and outranks the killer */
  if (leader->in_room == killer->in_room && leader->getLevel() > killer->getLevel())
    highest = leader;

  /* loop through all groupies */
  for (f = leader->followers; f; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) &&     // if grouped
        f->follower->in_room == killer->in_room && // and in the room
        !f->follower->isNonPlayer())
    {
      if (f->follower->getLevel() > highest->getLevel())
        highest = f->follower;
    }
  }
  return (highest);
}

/* count the number of group members eligible for XP from a kill */
qint32 count_xp_eligibles(CharacterPtr leader, CharacterPtr killer,
                          qint32 highest_level, qint32 *total_levels)
{
  follow_type *f;
  qint32 num_eligibles = {};

  *total_levels = {};

  /* check to see if the group leader was involved and eligible for XP */
  if (leader->in_room == killer->in_room && highest_level - leader->getLevel() < 20)
  {
    num_eligibles += 1;
    *total_levels += leader->getLevel();
  }

  /* loop through all the groupies */
  for (f = leader->followers; f; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) &&     // if grouped
        f->follower->in_room == killer->in_room && // and in the room
        !f->follower->isNonPlayer() &&
        (highest_level - f->follower->getLevel()) < 25)
    {
      num_eligibles += 1;
      *total_levels += f->follower->getLevel();
    }
  }
  return (num_eligibles);
}

/* scale character XP based on various factors */
qint64 scale_char_xp(CharacterPtr ch, CharacterPtr killer, CharacterPtr victim,
                     qint32 no_killers, qint32 total_levels, qint32 highest_level,
                     qint64 base_xp, qint64 *bonus_xp)
{
  qint32 scaled_share;
  *bonus_xp = {};

  scaled_share = ((base_xp + *bonus_xp) * MAX(ch->getLevel(), 1)) / total_levels;

  if (scaled_share > (ch->getLevel() * 8000))
    scaled_share = ch->getLevel() * 8000;

  return (scaled_share);
}

/* advance to the next follower in the list */
CharacterPtr loop_followers(follow_type **f)
{
  CharacterPtr tmp_ch;

  // this loops the followers
  if (!f.isEmpty())
  {
    tmp_ch = (*f)->follower;
    *f = (*f)->next;
  }
  else
    tmp_ch = {};

  return (tmp_ch);
}

const QStringList elem_type =
    {
        "$B$4stream of flame$R",
        "$B$3shards of ice$R",
        "$B$5bolt of energy$R",
        "$B$0stone fist$R"};

void dam_message(qint32 dam, CharacterPtr ch, CharacterPtr victim,
                 qint32 w_type, qint32 modifier)
{
  static const QStringList attack_table =
      {
          "hit", "pound", "pierce", "slash", "whip", "claw",
          "bite", "sting", "crush"};

  QString buf1, buf2, buf3;
  const QString vs, *vp, *vx;
  const QString attack = {};
  QChar punct;
  QString modstring;
  QString endstring;

  if (0 == dam && isSet(modifier, COMBAT_MOD_IGNORE))
  {
    dc_sprintf(buf1, "$n's pitiful attack is ignored by $N!");
    dc_sprintf(buf2, "Your pitiful attack is completely ignored by $N!");
    dc_sprintf(buf3, "You ignore $n's pitiful attack.");
    act_to_room(buf1, ch, nullptr, victim, NOTVICT);
    act_to_character(buf2, ch, nullptr, victim, 0);
    act_to_victim(buf3, ch, nullptr, victim, 0);
    return;
  }

  vx = "";

  if (dam == 0)
  {
    vs = "miss";
    vp = "misses";
  }
  else if (dam <= 2)
  {
    vs = "tickle";
    vp = "tickles";
  }
  else if (dam <= 5)
  {
    vs = "scratch";
    vp = "scratches";
  }
  else if (dam <= 9)
  {
    vs = "graze";
    vp = "grazes";
  }
  else if (dam <= 14)
  {
    vs = "hit";
    vp = "hits";
  }
  else if (dam <= 19)
  {
    vs = "hit";
    vp = "hits";
    vx = " hard";
  }
  else if (dam <= 25)
  {
    vs = "hit";
    vp = "hits";
    vx = " very hard";
  }
  else if (dam <= 31)
  {
    vs = "hit";
    vp = "hits";
    vx = " damn hard";
  }
  else if (dam <= 39)
  {
    vs = "pummel";
    vp = "pummels";
  }
  else if (dam <= 49)
  {
    vs = "massacre";
    vp = "massacres";
  }
  else if (dam <= 59)
  {
    vs = "annihilate";
    vp = "annihilates";
  }
  else if (dam <= 69)
  {
    vs = "obliterate";
    vp = "obliterates";
  }
  else if (dam <= 79)
  {
    vs = "cremate";
    vp = "cremates";
  }
  else if (dam <= 94)
  {
    vs = "decimate";
    vp = "decimates";
  }
  else if (dam <= 109)
  {
    vs = "mangle";
    vp = "mangles";
  }
  else if (dam <= 129)
  {
    vs = "eviscerate";
    vp = "eviscerates";
  }
  else if (dam <= 149)
  {
    vs = "beat the shit out of";
    vp = "beats the shit out of";
  }
  else if (dam <= 174)
  {
    vs = "BEAT THE LIVING SHIT out of";
    vp = "BEATS THE LIVING SHIT out of";
  }
  else if (dam <= 204)
  {
    vs = "POUND THE FUCK out of";
    vp = "POUNDS THE FUCK out of";
  }
  else if (dam <= 249)
  {
    vs = "FUCKING DEMOLISH";
    vp = "FUCKING DEMOLISHES";
  }
  else if (dam <= 299)
  {
    vs = "TOTALLY FUCKING DISINTEGRATE";
    vp = "TOTALLY FUCKING DISINTEGRATES";
  }
  else if (dam <= 999)
  {
    vs = "ABSOLUTELY FUCKING ERADICATE";
    vp = "ABSOLUTELY FUCKING ERADICATES";
  }
  else
  {
    vs = "nick";
    vp = "nicks";
  }

  if (w_type != SKILL_FLAMESLASH)
  {
    w_type -= TYPE_HIT;
    if (((quint32)w_type) >= sizeof(attack_table))
    {
      dc_->logentry(u"Dam_message: bad w_type"_s, ANGEL, DC::LogChannel::LOG_BUG);
      w_type = {};
    }
  }
  else
  {
    attack = "$B$4flaming slash$R";
  }

  // Custom damage messages.
  if (ch->isNonPlayer())
    switch (dc_->mob_index[ch->mobdata->nr].vnum())
    {
    case 13434:
      attack = "$2poison$R";
      break;
    case 13435:
      attack = "$B$4fire$R";
      break;
    case 13436:
      attack = "$B$2acid$R";
      break;
    case 13437:
      attack = "$B$5lightning$R";
      break;
    case 13438:
      attack = "$B$3frost$R";
      break;
    default:
      break;
    };

  punct = (dam <= 29) ? '.' : '!';

  if (isSet(modifier, COMBAT_MOD_FRENZY))
  {
    dc_strcpy(modstring, "frenzied ");
  }
  else
    *modstring = '\0';

  if (dam > 0)
  {
    if (isSet(modifier, COMBAT_MOD_SUSCEPT))
    {
      dc_strcpy(endstring, " doing extra damage");
    }

    if (isSet(modifier, COMBAT_MOD_RESIST))
    {
      dc_strcpy(endstring, " but is resisted");
    }
    else
      *endstring = '\0';
  }
  else
    *endstring = '\0';

  QString shield;
  QString dammsg;
  dammsg[0] = '\0';

  if (dam > 0)
  {
    switch (ch->dc_->number(0, 3))
    {
    case 0:
      dc_sprintf(dammsg, " causing $B%d $Rdamage", dam);
      break;
    case 1:
      dc_sprintf(dammsg, " delivering $B%d$R damage", dam);
      break;
    case 2:
      dc_sprintf(dammsg, " inflicting $B%d$R damage", dam);
      break;
    case 3:
      dc_sprintf(dammsg, " dealing $B%d$R damage", dam);
      break;
    }
  }

  if (isSet(modifier, COMBAT_MOD_REDUCED))
  {
    if (GET_CLASS(victim) == CLASS_MONK)
    {
      switch (ch->dc_->number(0, 3))
      {
      case 0:
        dc_sprintf(shield, "shin");
        break;
      case 1:
        dc_sprintf(shield, "hand");
        break;
      case 2:
        dc_sprintf(shield, "foot");
        break;
      case 3:
        dc_sprintf(shield, "forearm");
        break;
      default:
        dc_sprintf(shield, "error");
        break;
      }
    }
    else if (victim->equipment[WEAR_SHIELD])
      dc_sprintf(shield, "%s", qPrintable(victim->equipment[WEAR_SHIELD]->short_description()));

    if (GET_CLASS(victim) == CLASS_MONK)
    {
      if (w_type == 0)
      {
        if (!attack)
          attack = races[ch->race].unarmed;
        dc_sprintf(buf1, "$n's %s %s $N%s| as it deflects off $S %s%c", attack, vp, vx, shield, punct);
        dc_sprintf(buf2, "You %s $N%s%s as $E raises $S %s to deflect your %s%c", vs, vx, ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", shield, attack, punct);
        dc_sprintf(buf3, "$n %s you%s%s as you deflect $s %s with your %s%c", vp, vx, victim->isPlayer() && isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", attack, shield, punct);
      }
      else
      {
        if (!attack)
          attack = attack_table[w_type];
        dc_sprintf(buf1, "$n's %s %s $N%s| as it deflects off $S %s%c", attack, vp, vx, shield, punct);
        dc_sprintf(buf2, "You %s $N%s%s as $E raises $S %s to deflect your %s%c", vs, vx, ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", shield, attack, punct);
        dc_sprintf(buf3, "$n %s you%s%s as you deflect $s %s with your %s%c", vp, vx, victim->isPlayer() && isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", attack, shield, punct);
      }
    }
    else if (victim->has_skill(SKILL_TUMBLING))
    {
      if (ch->dc_->number(0, 1))
      {
        dc_sprintf(buf1, "$N leaps away from $n's strike, managing to avoid all but a scratch|.");
        dc_sprintf(dammsg, " for $B%d$R damage", dam);
        dc_sprintf(buf2, "$N leaps away from your strike, managing to avoid all but a scratch%s.", ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "");
        dc_sprintf(buf3, "You leap away from $n's strike, managing to avoid all but a scratch%s.", victim->isPlayer() && isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "");
      }
      else
      {
        dc_sprintf(buf1, "$N's roll to the side comes a moment too late as $n still manages to land a glancing blow|.");
        dc_sprintf(dammsg, ", dealing $B%d$R damage", dam);
        dc_sprintf(buf2, "$N's roll to the side comes a moment too late as you still manages to land a glancing blow%s.", ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "");
        dc_sprintf(buf3, "Your roll to the side comes a moment too late as $n still manages to land a glancing blow%s.", victim->isPlayer() && isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "");
      }
    }
    else
    {
      if (w_type == 0)
      {
        if (!attack)
          attack = races[ch->race].unarmed;
        dc_sprintf(buf1, "$n's %s %s $N%s| as it strikes $S %s%c", attack, vp, vx, shield, punct);
        dc_sprintf(buf2, "You %s $N%s%s as $E raises $S %s to deflect your %s%c", vs, vx, ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", shield, attack, punct);
        dc_sprintf(buf3, "$n %s you%s%s as you deflect $s %s with %s%c", vp, vx, victim->isPlayer() && isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", attack, shield, punct);
      }
      else
      {
        if (!attack)
          attack = attack_table[w_type];
        dc_sprintf(buf1, "$n's %s %s $N%s| as it strikes $S %s%c", attack, vp, vx, shield, punct);
        dc_sprintf(buf2, "You %s $N%s%s as $E raises $S %s to deflect your %s%c", vs, vx, ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", shield, attack, punct);
        dc_sprintf(buf3, "$n %s you%s%s as you deflect $s %s with %s%c", vp, vx, victim->isPlayer() && isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", attack, shield, punct);
      }
    }
  }
  else
  {
    if (w_type == 0)
    {
      if (!attack)
        attack = races[ch->race].unarmed;
      qint32 a;
      if (ch->isNonPlayer() && (a = dc_->mob_index[ch->mobdata->nr].vnum()) < 92 && a > 87)
        attack = elem_type[a - 88];
      dc_sprintf(buf1, "$n's %s%s %s $N%s|%c", modstring, attack, vp, vx, punct);
      dc_sprintf(buf2, "Your %s%s %s $N%s%s%c", modstring, attack, vp, vx, ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", punct);
      dc_sprintf(buf3, "$n's %s%s %s you%s%s%c", modstring, attack, vp, vx, victim->isPlayer() && isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", punct);
    }
    else
    {
      if (!attack)
        attack = attack_table[w_type];
      dc_sprintf(buf1, "$n's %s%s %s $N%s|%c", modstring, attack, vp, vx, punct);
      dc_sprintf(buf2, "Your %s%s %s $N%s%s%c", modstring, attack, vp, vx, ch->isPlayer() && isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", punct);
      dc_sprintf(buf3, "$n's %s%s %s you%s%s%c", modstring, attack, vp, vx, victim->isPlayer() && isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", punct);
    }
  }
  //   act_to_room(buf1, ch, nullptr, victim,  NOTVICT);
  send_damage(buf1, ch, 0, victim, dammsg, 0, TO_ROOM);
  act_to_character(buf2, ch, nullptr, victim, 0);
  act_to_victim(buf3, ch, nullptr, victim, 0);
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm(CharacterPtr ch, CharacterPtr victim)
{
  ObjectPtr obj;

  if (victim->equipment[WEAR_WIELD] == nullptr)
    return;
  if (ch->equipment[WEAR_WIELD] == nullptr)
    return;

  if (victim->affected_by_spell(SPELL_PARALYZE))
  {
    ch->sendln("Their paralyzed fingers are gripping the weapon too tightly.");
    return;
  }
  if (isSet(victim->combat, COMBAT_BERSERK))
  {
    ch->sendln("In their enraged state, there's no chance they'd let go of their weapon!");
    return;
  }
  act_to_victim("$B$n disarms you and sends your weapon flying!$R", ch, nullptr, victim, 0);
  act_to_character("You disarm $N and send $S weapon flying!", ch, nullptr, victim, 0);
  act_to_room("$n disarms $N and sends $S weapon flying!", ch, nullptr, victim, NOTVICT);

  // all disarms go to inventory right now -pir
  //  if (ch->isPlayer()) {
  obj = victim->unequip_char(WEAR_WIELD);
  obj_to_char(obj, victim);
  if (victim->equipment[WEAR_SECOND_WIELD])
  {
    obj = victim->unequip_char(WEAR_SECOND_WIELD);
    victim->equip_char(obj, WEAR_WIELD);
  }
  victim->recheck_height_wears();
  return;
  //  }

  // we never get here cause of above code

  obj = victim->unequip_char(WEAR_WIELD);
  /* If it's gl make it go to inventory. Morc. */
  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    obj_to_char(obj, victim);
  else
    obj_to_room(obj, victim->in_room);

  if (victim->equipment[WEAR_SECOND_WIELD])
  {
    obj = victim->unequip_char(WEAR_SECOND_WIELD);
    victim->equip_char(obj, WEAR_WIELD);
  }
}

/*
 * Trip a creature.
 * Caller must check for successful attack.
 */
void trip(CharacterPtr ch, CharacterPtr victim)
{
  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return;

  act("$n trips you and you go down!",
      ch, nullptr, victim, TO_VICT, 0);
  act("You trip $N and $N goes down!",
      ch, nullptr, victim, TO_CHAR, 0);
  act("$n trips $N and $N goes down!",
      ch, nullptr, victim, TO_ROOM, NOTVICT);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 3);
  if (damage(ch, victim, 1, TYPE_HIT, SKILL_TRIP, 0) == (-1))
    return;

  WAIT_STATE(victim, DC::PULSE_VIOLENCE * 2);
  victim->setSitting();
}

constexpr auto PKILL_COUNT_LIMIT = 20;

// 'ch' can be null
// do_pkill should never be called directly, only through "fight_kill"
void do_pkill(CharacterPtr ch, CharacterPtr victim, qint32 type, bool vict_is_attacker)
{
  qint32 num;
  QString killer_message;
  //  CharacterPtr i = {};
  affected_type *af, *afpk;

  void move_player_home(CharacterPtr victim);
  num = dc_->number(1, 1000);

  if (!victim)
  {
    dc_->logentry(u"Null victim sent to do_pkill."_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (ch)
  {
    set_cantquit(ch, victim);
  }
  extern void pk_check(CharacterPtr ch, CharacterPtr victim);
  pk_check(ch, victim);
  // Kill charmed mobs outright
  if (victim->isNonPlayer())
  {
    fight_kill(ch, victim, TYPE_RAW_KILL, 0);
    return;
  }

  if (victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
  {
    victim->setMove(2);
    fight_kill(ch, victim, TYPE_RAW_KILL, 0);
    return;
  }

  if (victim->isPlayerGoldThief())
  {
    victim->setMove(2);
    if (victim->getGold() > 0)
    {
      act_to_room("$N drops $S stolen booty!", ch, 0, victim, NOTVICT);
      obj_to_room(create_money(victim->getGold()), victim->in_room);
      victim->setGold(0);
      victim->save_char_obj();
    }
  }
  // Make sure barbs get their ac back
  if (isSet(victim->combat, COMBAT_BERSERK))
  {
    REMOVE_BIT(victim->combat, COMBAT_BERSERK);
    GET_AC(victim) -= 30;
  }

  if (type == KILL_POISON && victim->affected_by_spell(SPELL_POISON)->modifier > 0)
  {
    const auto &character_list = dc_->character_list;
    for (const auto &findchar : character_list)
    {
      if (findchar == victim->affected_by_spell(SPELL_POISON)->origin)
        ch = findchar;
    }
  }

  for (af = victim->affected; af; af = afpk)
  {
    afpk = af->next;
    if (af->type != Character::PLAYER_CANTQUIT &&
        af->type != SKILL_LAY_HANDS &&
        af->type != SKILL_HARM_TOUCH &&
        af->type != SKILL_BLOOD_FURY &&
        af->type != SKILL_QUIVERING_PALM &&
        af->type != SKILL_INNATE_TIMER &&
        af->type != SPELL_HOLY_AURA_TIMER &&
        af->type != SPELL_NAT_SELECT_TIMER &&
        af->type != SKILL_NAT_SELECT &&
        af->type != SPELL_DIV_INT_TIMER &&
        af->type != SPELL_NO_CAST_TIMER &&
        af->type != SKILL_CRAZED_ASSAULT &&
        af->type != SKILL_FOCUSED_REPELANCE && !(af->type >= 1100 && af->type <= 1300))
      affect_remove(victim, af, SUPPRESS_ALL);
  }

  victim->setHP(1);
  GET_KI(victim) = 1;
  GET_MANA(victim) = 1;
  if (IS_AFFECTED(victim, AFF_CANTQUIT))
    victim->setMove(4);

  move_player_home(victim);

  victim->setResting();
  GET_COND(victim, DRUNK) = {};
  GET_COND(victim, FULL) = {};
  GET_COND(victim, THIRST) = {};

  victim->save_char_obj();

  // have to be level 20 and linkalive to count as a pkill and not yourself
  if (ch != nullptr)
  {
    if (type == KILL_POTATO)
      dc_sprintf(killer_message, "\r\n##%s just got POTATOED!!\r\n", qPrintable(victim->name()));
    else if (type == KILL_MORTAR)
      dc_sprintf(killer_message, "\r\n##%s just got a FIRE IN THE HOLE!!\r\n", qPrintable(victim->name()));
    else if (type == KILL_POISON)
      dc_sprintf(killer_message, "\r\n##%s has perished from %s's POISON!\r\n", qPrintable(victim->name()), qPrintable(ch->name()));
    else if (!str_cmp(qPrintable(ch->name()), qPrintable(victim->name())))
      dc_sprintf(killer_message, "");
    //    dc_sprintf(killer_message,"\r\n##%s just commited SUICIDE!\r\n", qPrintable(victim->name()));
    else if (victim->getLevel() < PKILL_COUNT_LIMIT || ch == victim)
      // dc_sprintf(killer_message,"\r\n##%s just DIED!\r\n", qPrintable(victim->name()));
      // dc_sprintf(killer_message,"\r\n##%s was just introduced to the warm hospitality of Dark Castle!!\r\n", qPrintable(victim->name()));
      dc_sprintf(killer_message, "");
    else if (num == 1000)
      dc_sprintf(killer_message, "\r\n##%s was just ANALLY PROBED by %s!\r\n", qPrintable(victim->name()), qPrintable(ch->name()));
    else if (IS_AFFECTED(ch, AFF_FAMILIAR) && ch->master)
      dc_sprintf(killer_message, "\r\n##%s was just DEFEATED in battle by %s's familiar!\r\n",
                 qPrintable(victim->name()), qPrintable(ch->master->name()));
    else if (IS_AFFECTED(ch, AFF_CHARM) && ch->master)
      dc_sprintf(killer_message, "\r\n##%s was just DEFEATED in battle by %s's charmie!\r\n",
                 qPrintable(victim->name()), qPrintable(ch->master->name()));
    else if (ch->in_room == real_room(START_ROOM))
      dc_sprintf(killer_message, "\r\n##%s was just PINGED by %s!\r\n",
                 qPrintable(victim->name()), qPrintable(ch->name()));
    else if (ch->in_room == real_room(SECOND_START_ROOM))
      dc_sprintf(killer_message, "\r\n##%s was just PONGED by %s!\r\n",
                 qPrintable(victim->name()), qPrintable(ch->name()));
    else if (IS_ANONYMOUS(ch))
      dc_sprintf(killer_message, "\r\n##%s was just DEFEATED in battle by %s!\r\n",
                 qPrintable(victim->name()), qPrintable(ch->name()));
    else if (ch->getLevel() > MORTAL)
      dc_sprintf(killer_message, "\r\n##%s was just SMITED...er..SMOTED..err PKILLED by %s!\r\n", qPrintable(victim->name()), qPrintable(ch->name()));
    else if (type == KILL_BINGO)
      dc_sprintf(killer_message, "\r\n##%s was just BINGOED by %s!\r\n",
                 qPrintable(victim->name()), qPrintable(ch->name()));
    else if (type == SPELL_CONSECRATE)
      dc_sprintf(killer_message, "\r\n##%s was just slain by %s's CONSECRATION!\r\n", qPrintable(victim->name()), qPrintable(ch->name()));
    else if (type == SPELL_DESECRATE)
      dc_sprintf(killer_message, "\r\n##%s was just slain by %s's DESECRATION!\r\n", qPrintable(victim->name()), qPrintable(ch->name()));
    else
      switch (GET_CLASS(ch))
      {
      case CLASS_MAGIC_USER:
        dc_sprintf(killer_message, "\r\n##%s was just FRIED by %s's magic!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      case CLASS_CLERIC:
        dc_sprintf(killer_message, "\r\n##%s was just BANISHED by %s's holiness!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      case CLASS_THIEF:
        dc_sprintf(killer_message, "\r\n##%s was just ASSASSINATED by %s!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      case CLASS_WARRIOR:
        dc_sprintf(killer_message, "\r\n##%s was just SLAIN by %s's might!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      case CLASS_ANTI_PAL:
        dc_sprintf(killer_message, "\r\n##%s was just CONSUMED by %s's darkness!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      case CLASS_PALADIN:
        dc_sprintf(killer_message, "\r\n##%s was just VANQUISHED by %s's goodness!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      case CLASS_BARBARIAN:
        dc_sprintf(killer_message, "\r\n##%s was just SHREDDED by %s's crazed fury!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      case CLASS_MONK:
        dc_sprintf(killer_message, "\r\n##%s was just SHATTERED by %s's karma!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      case CLASS_RANGER:
        dc_sprintf(killer_message, "\r\n##%s was just PENETRATED by %s's wood!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      case CLASS_BARD:
        dc_sprintf(killer_message, "\r\n##%s was just MUTED by %s's snazzy rhythm!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      case CLASS_DRUID:
        dc_sprintf(killer_message, "\r\n##%s was just VIOLATED by %s's woodland friends!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      default:
        dc_sprintf(killer_message, "\r\n##%s was just DEFEATED in battle by %s!\r\n",
                   qPrintable(victim->name()), qPrintable(ch->name()));
        break;
      }
    level_diff_t level_spread;
    // have to be level 20 and linkalive to count as a pkill and not yourself
    // (we check earlier to make sure victim isn't a mob)
    // now with tav/meta pkilling not adding to your score
    if (!ch->isNonPlayer()
        // && victim->getLevel() > PKILL_COUNT_LIMIT
        && victim->desc && ch != victim && ch->in_room != real_room(START_ROOM) && ch->in_room != real_room(SECOND_START_ROOM))
    {
      level_spread = ch->getLevel() - victim->getLevel();
      if (level_spread > 20 && !(IS_AFFECTED(victim, AFF_CANTQUIT) || IS_AFFECTED(victim, AFF_CHAMPION)) && !vict_is_attacker)
      {
        if (GET_PKILLS(ch) > 0)
          GET_PKILLS(ch) -= 1;
      }
      else if (victim->getLevel() > PKILL_COUNT_LIMIT)
      {
        GET_PDEATHS(victim) += 1;
        GET_PDEATHS_LOGIN(victim) += 1;

        GET_PKILLS(ch) += 1;
        GET_PKILLS_LOGIN(ch) += 1;
        GET_PKILLS_TOTAL(ch) += victim->getLevel();
        GET_PKILLS_TOTAL_LOGIN(ch) += victim->getLevel();

        if (IS_AFFECTED(ch, AFF_GROUP))
        {
          CharacterPtr master = ch->master ? ch->master : ch;
          if (master->isPlayer())
          {
            if (victim->isPlayer())
            {
              master->player->grpplvl += victim->getLevel();
              master->player->group_pkills += 1;
            }
            master->player->group_kills += 1;
          }
        }
      }
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master
        //  && victim->getLevel() > PKILL_COUNT_LIMIT
        && victim->desc && ch->master != victim && ch->in_room != real_room(START_ROOM) && ch->in_room != real_room(SECOND_START_ROOM))
    {
      level_spread = ch->master->getLevel() - victim->getLevel();
      if (level_spread > 20 && !(IS_AFFECTED(victim, AFF_CANTQUIT) || IS_AFFECTED(victim, AFF_CHAMPION)) && !vict_is_attacker)
      {
        if (GET_PKILLS(ch->master) > 0)
          GET_PKILLS(ch->master) -= 1;
      }
      else if (victim->getLevel() > PKILL_COUNT_LIMIT)
      {
        GET_PDEATHS(victim) += 1;
        GET_PDEATHS_LOGIN(victim) += 1;

        GET_PKILLS(ch->master) += 1;
        GET_PKILLS_LOGIN(ch->master) += 1;
        GET_PKILLS_TOTAL(ch->master) += victim->getLevel();
        GET_PKILLS_TOTAL_LOGIN(ch->master) += victim->getLevel();

        if (ch->master->master)
        {
          if (IS_AFFECTED(ch->master->master, AFF_GROUP))
          {
            CharacterPtr master = ch->master->master ? ch->master->master : ch->master;
            if (master->isPlayer())
            {
              if (victim->isPlayer())
              {
                master->player->grpplvl += victim->getLevel();
                master->player->group_pkills += 1;
              }
              master->player->group_kills += 1;
            }
          }
        }
      }
    }
    if (IS_AFFECTED(ch, AFF_FAMILIAR) && ch->master
        // && victim->getLevel() > PKILL_COUNT_LIMIT
        && victim->desc && ch->master != victim && ch->in_room != real_room(START_ROOM) && ch->in_room != real_room(SECOND_START_ROOM))
    {
      level_spread = ch->master->getLevel() - victim->getLevel();
      if (level_spread > 20 && !(IS_AFFECTED(victim, AFF_CANTQUIT) || IS_AFFECTED(victim, AFF_CHAMPION)) && !vict_is_attacker)
      {
        if (GET_PKILLS(ch->master) > 0)
          GET_PKILLS(ch->master) -= 1;
      }
      else if (victim->getLevel() > PKILL_COUNT_LIMIT)
      {

        GET_PDEATHS(victim) += 1;
        GET_PDEATHS_LOGIN(victim) += 1;

        GET_PKILLS(ch->master) += 1;
        GET_PKILLS_LOGIN(ch->master) += 1;
        GET_PKILLS_TOTAL(ch->master) += victim->getLevel();
        GET_PKILLS_TOTAL_LOGIN(ch->master) += victim->getLevel();

        if (ch->master->master)
        {
          if (IS_AFFECTED(ch->master->master, AFF_GROUP))
          {
            CharacterPtr master = ch->master->master ? ch->master->master : ch->master;
            if (master->isPlayer())
            {
              if (victim->isPlayer())
              {
                master->player->grpplvl += victim->getLevel();
                master->player->group_pkills += 1;
              }
              master->player->group_kills += 1;
            }
          }
        }
      }
    }
    // if (ch && ch != victim)
  }
  else
  {
    // ch == nullptr
    if (type == KILL_DROWN)
      dc_sprintf(killer_message, "\r\n##%s just DROWNED!\r\n", qPrintable(victim->name()));
    else if (type == KILL_POTATO)
      dc_sprintf(killer_message, "\r\n##%s just got POTATOED!!\r\n", qPrintable(victim->name()));
    else if (type == KILL_POISON)
      dc_sprintf(killer_message, "\r\n##%s has perished from POISON!\r\n", qPrintable(victim->name()));
    else if (type == KILL_FALL)
      dc_sprintf(killer_message, "\r\n##%s has FALLEN to death!\r\n", qPrintable(victim->name()));
    else if (type == KILL_BATTER)
      dc_sprintf(killer_message, "\r\n##That's using your head! %s just died attempting to batter a door!\r\n", qPrintable(victim->name()));
    else
      dc_sprintf(killer_message, "\r\n##%s just DIED!\r\n", qPrintable(victim->name()));
  }

  send_info(killer_message);

  if (IS_AFFECTED(victim, AFF_CHAMPION) && ch && ch != victim)
  {
    REMBIT(victim->affected_by, AFF_CHAMPION);
    ObjectPtr obj = {};
    if (!(obj = get_obj_in_list_num(real_object(CHAMPION_ITEM), victim->carrying)))
    {
      dc_->logentry(u"Champion without the flag, no bueno amigo!"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
      return;
    }
    if (ch->isNonPlayer() && ch->master)
    {
      if (ch->master->in_room >= 1900 || ch->master->in_room <= 1999 || isSet(dc_->world[ch->master->in_room].room_flags, CLAN_ROOM))
      {
        SETBIT(victim->affected_by, AFF_CHAMPION);
        dc_sprintf(killer_message, "##%s didn't deserve to become the new Champion, it remains %s!\r\n", qPrintable(ch->master->name()), qPrintable(victim->name()));
      }
      else
      {
        move_obj(obj, ch->master);
        SETBIT(ch->master->affected_by, AFF_CHAMPION);
        dc_sprintf(killer_message, "##%s has become the new Champion!\r\n", qPrintable(ch->master->name()));
      }
    }
    else
    {
      move_obj(obj, ch);
      SETBIT(ch->affected_by, AFF_CHAMPION);
      dc_sprintf(killer_message, "##%s has become the new Champion!\r\n", qPrintable(ch->name()));
    }
    send_info(killer_message);
  }
}

// 'ch' can be null
void arena_kill(CharacterPtr ch, CharacterPtr victim, qint32 type)
{
  void remove_nosave(CharacterPtr vict);

  QString killer_message;
  //  CharacterPtr i;
  ClanPtr ch_clan = {};
  ClanPtr victim_clan = {};
  qint32 eliminated = 1;
  void move_player_home(CharacterPtr victim);

  if (!victim)
  {
    dc_->logentry(u"Null victim sent to arena_kill."_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  // Kill mobs outright
  if (victim->isNonPlayer())
  {
    mprog_death_trigger(victim, ch);
    remove_nosave(victim);
    extract_char(victim, true);
    return;
  }

  // We keep objects with the ITEM_NOSAVE flag
  // Needs to be called BEFORE move_char so that the items
  // end up in the right room
  // why did we do this? -pir
  // remove_nosave(victim);

  move_player_home(victim);

  while (victim->affected)
    affect_remove(victim, victim->affected, SUPPRESS_ALL);

  auto &arena = dc_->arena_;
  if (ch && arena.isChaos())
  {
    if (ch && ch->clan && ch->isMortalPlayer())
      ch_clan = get_clan(ch);
    if (victim->clan && victim->isMortalPlayer())
      victim_clan = get_clan(victim);

    if (type == KILL_BINGO)
    {
      dc_sprintf(killer_message, "\r\n## %s [%s] just BINGOED %s [%s] in the arena!\r\n", ((ch->isNonPlayer() && ch->master) ? qPrintable(ch->master->name()) : qPrintable(ch->name())), qPrintable(get_clan_name(ch_clan)), qPrintable(victim->name()), qPrintable(get_clan_name(victim_clan)));
    }
    else
    {
      dc_sprintf(killer_message, "\r\n## %s [%s] just SLAUGHTERED %s [%s] in the arena!\r\n", ((ch->isNonPlayer() && ch->master) ? qPrintable(ch->master->name()) : qPrintable(ch->name())), qPrintable(get_clan_name(ch_clan)), qPrintable(victim->name()), qPrintable(get_clan_name(victim_clan)));
    }

    dc_->logf(IMMORTAL, DC::LogChannel::LOG_ARENA, "%s [%s] killed %s [%s]",
              ((ch->isNonPlayer() && ch->master) ? qPrintable(ch->master->name()) : qPrintable(ch->name())), qPrintable(get_clan_name(ch_clan)), qPrintable(victim->name()), qPrintable(get_clan_name(victim_clan)));
  }
  else if (ch)
  {
    if (type == KILL_POTATO)
      dc_sprintf(killer_message, "\r\n## %s just got POTATOED in the arena!\r\n", qPrintable(victim->shortdesc_or_name()));
    else if (type == KILL_MASHED)
      dc_sprintf(killer_message, "\r\n## %s just got MASHED in the potato arena!\r\n", qPrintable(victim->shortdesc_or_name()));
    else
    {
      if (type == KILL_BINGO)
      {
        dc_sprintf(killer_message, "\r\n## %s just BINGOED %s in the arena!\r\n",
                   (ch->isNonPlayer() && (ch->master) ? qPrintable(ch->master->shortdesc_or_name()) : qPrintable(ch->shortdesc_or_name())), qPrintable(victim->shortdesc_or_name()));
      }
      else
      {
        dc_sprintf(killer_message, "\r\n## %s just SLAUGHTERED %s in the arena!\r\n",
                   (ch->isNonPlayer() && (ch->master) ? qPrintable(ch->master->shortdesc_or_name()) : qPrintable(ch->shortdesc_or_name())), qPrintable(victim->shortdesc_or_name()));
      }
    }
  }
  else
  {
    if (type == KILL_POTATO)
      dc_sprintf(killer_message, "\r\n## %s just got POTATOED in the arena!\r\n", qPrintable(victim->shortdesc_or_name()));
    else if (type == KILL_MASHED)
      dc_sprintf(killer_message, "\r\n## %s just got MASHED in the potato arena!\r\n", qPrintable(victim->shortdesc_or_name()));
    else
      dc_sprintf(killer_message, "\r\n## %s just DIED in the arena!\r\n", qPrintable(victim->shortdesc_or_name()));
  }
  send_info(killer_message);

  if (ch && victim && (arena.isPrize() || arena.isChaos()))
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_ARENA, "%s killed %s", qPrintable(ch->name()), qPrintable(victim->name()));
  }

  // if it's a chaos, see if the clan was eliminated
  if (victim && arena.isChaos() && victim_clan)
  {
    const auto &character_list = dc_->character_list;
    for (const auto &tmp : character_list)
    {

      if (tmp->room().isArena())
        if (victim->clan == tmp->clan && victim != tmp && tmp->isMortalPlayer())
          eliminated = {};
    }
    if (eliminated)
    {
      dc_sprintf(killer_message, "## [%s] was just eliminated from the chaos!\r\n", qPrintable(get_clan_name(victim_clan)));
      send_info(killer_message);
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_ARENA, "## [%s] was just eliminated from the chaos!", qPrintable(get_clan_name(victim_clan)));
    }
  }

  victim->sendln("You have been completely healed.");
  victim->setResting();
  victim->fillHP();
  GET_MANA(victim) = GET_MAX_MANA(victim);
  victim->setMove(GET_MAX_MOVE(victim));
  GET_KI(victim) = GET_MAX_KI(victim);

  if (ch)
    ch->combat = {}; // remove all combat effects

  remove_active_potato(victim);
  victim->save_char_obj();
}

bool is_stunned(CharacterPtr ch)
{
  if (isSet(ch->combat, COMBAT_STUNNED))
    return true;
  if (isSet(ch->combat, COMBAT_STUNNED2))
    return true;
  if (isSet(ch->combat, COMBAT_SHOCKED))
    return true;
  if (isSet(ch->combat, COMBAT_SHOCKED2))
    return true;
  return false;
}

qint32 can_attack(CharacterPtr ch)
{
  if (ch->room().isArena() && ch->room().arena().isOpened())
  {
    ch->sendln("Wait until it closes!");
    return false;
  }

  auto &arena = dc_->arena_;
  if ((ch->in_room >= 0 && ch->in_room <= dc_->top_of_world) &&
      ch->room().isArena() && arena.isPotato())
  {
    ch->sendln("You can't attack in a potato arena, go find a potato would ya?!");
    return false;
  }

  if (IS_AFFECTED(ch, AFF_PARALYSIS))
    return false;
  if (is_stunned(ch))
    return false;

  return true;
}

qint32 can_be_attacked(CharacterPtr ch, CharacterPtr vict)
{
  /* this will happen sometimes, no need to log it */
  if (!ch || !vict)
    return false;

  if (vict->in_room == DC::NOWHERE || vict->in_room >= dc_->top_of_world ||
      ch->in_room == DC::NOWHERE || ch->in_room >= dc_->top_of_world)
    return false;

  // Ch should not be able to attack a wizinvis immortal player
  if (vict->isPlayer() && ch->getLevel() < vict->player->wizinvis)
    return false;

  if (vict->isNonPlayer())
    if (ISSET(vict->mobdata->actflags, ACT_NOATTACK))
    {
      ch->sendln("Due to heavy magics, they cannot be attacked.");
      return false;
    }

  // Prize Arena
  auto &arena = dc_->arena_;
  if (ch->room().isArena() && arena.isPrize() && ch->isPlayer() && vict->isPlayer())
  {

    if (ch->fighting && ch->fighting != vict)
    {
      ch->sendln("You are already fighting someone.");
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_ARENA, "%s, whom was fighting %s was prevented from attacking %s.",
                qPrintable(ch->name()), qPrintable(ch->fighting->name()), qPrintable(vict->name()));
      return false;
    }
    else if (vict->fighting && vict->fighting != ch)
    {
      ch->sendln("They are already fighting someone.");
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_ARENA, "%s was prevented from attacking %s who was fighting %s.",
                qPrintable(ch->name()), qPrintable(vict->name()), qPrintable(vict->fighting->name()));
      return false;
    }
  }

  // Clan Chaos Arena
  if (ch->room().isArena() && arena.isChaos() && ch->isPlayer() && vict->isPlayer())
  {
    if (ch->fighting && ch->fighting != vict && !ARE_CLANNED(ch->fighting, vict))
    {
      ch->sendln("You are already fighting someone from another clan.");
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_ARENA, "%s [%s], whom was fighting %s [%s] was prevented from attacking %s [%s].",
                qPrintable(ch->name()), qPrintable(get_clan_name(ch)), qPrintable(ch->fighting->name()), qPrintable(get_clan_name(ch->fighting)), qPrintable(vict->name()), qPrintable(get_clan_name(vict)));
      return false;
    }
    else if (vict->fighting && vict->fighting != ch && !ARE_CLANNED(vict->fighting, ch))
    {
      ch->sendln("They are already fighting someone.");
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_ARENA, "%s [%s] was prevented from attacking %s [%s] who was fighting %s [%s].",
                qPrintable(ch->name()), qPrintable(get_clan_name(ch)), qPrintable(vict->name()), qPrintable(get_clan_name(vict)), qPrintable(vict->fighting->name()), qPrintable(get_clan_name(vict->fighting)));
      return false;
    }
  }

  // Golem cannot attack players
  if (ch->isNonPlayer() && dc_->mob_index[ch->mobdata->nr].vnum() == 8 && vict->isPlayer())
    return false;

  if (vict->isNonPlayer())
  {
    if ((IS_AFFECTED(vict, AFF_FAMILIAR) || dc_->mob_index[vict->mobdata->nr].vnum() == 8 || vict->affected_by_spell(SPELL_CHARM_PERSON) || ISSET(vict->affected_by, AFF_CHARM)) &&
        vict->master &&
        vict->fighting != ch &&
        !(IS_AFFECTED(vict->master, AFF_CANTQUIT) || IS_AFFECTED(vict->master, AFF_CHAMPION)) &&
        isSet(dc_->world[vict->in_room].room_flags, SAFE))
    {
      ch->sendln("No fighting permitted in a safe room.");
      return false;
    }
    // Any other mob can be attacked at any time
    return true;
  }

  if (ch->isPlayer() && vict->isPlayer() && ch->getLevel() < 5)
  {
    ch->sendln("You are too new in this realm to make enemies!");
    return false;
  }

  if (IS_AFFECTED(vict, AFF_CANTQUIT) || vict->affected_by_spell(Character::PLAYER_OBJECT_THIEF) || vict->isPlayerGoldThief() || IS_AFFECTED(vict, AFF_CHAMPION))
    return true;

  if (ch->isPlayer() && vict->getLevel() < 5)
  {
    act_to_character("The magic of the MUD school is protecting $M from harm.", ch, 0, vict, 0);
    return false;
  }

  if (ch->isNonPlayer() && vict->isPlayer() && IS_AFFECTED(ch, AFF_CHARM))
  { // New charmie stuff. No attacking pcs unless yer master's a ranger/cleric.
    // Those guys are soo convincing.
    if (!ch->master)
      return false; // What the hell?
    if (GET_CLASS(ch->master) != CLASS_ANTI_PAL && GET_CLASS(ch->master) != CLASS_RANGER && GET_CLASS(ch->master) != CLASS_CLERIC)
      return false;
    if (vict == ch->master)
      return false;
    if (vict->getLevel() < 5)
    {
      do_say(ch, "I'm sorry master, I cannot do that.");
      return ReturnValue::eFAILURE;
    }
  }

  if (isSet(dc_->world[ch->in_room].room_flags, SAFE))
  {
    /* Allow the NPCs to continue fighting */
    if (ch->isNonPlayer())
    {
      if (ch->fighting == vict)
        return true;
    }
    /* Allow players to continue fighting if they have a cantquit */
    if (ch->isPlayerCantQuit() || IS_AFFECTED(ch, AFF_CHAMPION))
    {
      if (ch->fighting == vict)
        return true;
    }
    /* Imps ignore safe flags  */
    if (ch->isPlayer() && (ch->getLevel() == IMPLEMENTER))
    {
      vict->sendln("There is no safe haven from an angry IMPLEMENTER!");
      return true;
    }

    if (vict->fighting == ch)
    {
      /*
      This happens when someone with CQ is defending himself from people without CQ,
      if they are already in combat, a riposte or similar will cause this.
      */
      return true;
    }

    ch->sendln("No fighting permitted in a safe room.");

    if (ch->fighting == vict)
    {
      /*
      This check is to only stop fighting if the person the ch is trying to fight
      is not the person they're currently fighting.
      Otherwise they can trigger a stop_fighting just by trying to attack someone
      that they can't attack while attacking someone else.
      */
      stop_fighting(ch);
    }

    return false;
  }
  return true;
}

qint32 weapon_spells(CharacterPtr ch, CharacterPtr vict, qint32 weapon)
{
  qint32 i, current_affect, chance, percent, retval;
  ObjectPtr weap;

  if (!ch || !vict)
  {
    dc_->logentry(u"Null ch or vict sent to weapon spells!"_s, -1, DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }
  if (!weapon)
    return ReturnValue::eFAILURE;

  if (!can_attack(ch) || !can_be_attacked(ch, vict))
    return ReturnValue::eFAILURE;

  if ((ch->in_room != vict->in_room && weapon != ITEM_MISSILE) || GET_POS(vict) == position_t::DEAD)
    return ReturnValue::eFAILURE;

  if (!ch->equipment[weapon] && weapon != ITEM_MISSILE)
    return ReturnValue::eFAILURE;

  qint32 wep_skill = 40;

  if (weapon == ITEM_MISSILE)
    weap = get_obj_in_list_vis(ch, "arrow", ch->carrying);
  else
    weap = ch->equipment[weapon];

  if (!weap)
    return ReturnValue::eFAILURE;

  for (i = {}; i < weap->num_affects; i++)
  {
    /* It's possible the victim has fled or died */
    if (ch->in_room != vict->in_room)
      return ReturnValue::eFAILURE;
    if (GET_POS(vict) == position_t::DEAD)
      return ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED;
    chance = dc_->number(1, 100);
    percent = weap->affected[i].modifier;
    current_affect = weap->affected[i].location;

    /* If they don't roll under chance, it doesn't work! */
    if (chance > percent)
      continue;
    switch (current_affect)
    {
    case WEP_MAGIC_MISSILE:
      retval = spell_magic_missile(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_BLIND:
      retval = spell_blindness(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_EARTHQUAKE:
      retval = spell_earthquake(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_CURSE:
      retval = spell_curse(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_COLOUR_SPRAY:
      retval = spell_colour_spray(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_DISPEL_EVIL:
      retval = spell_dispel_evil(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_ENERGY_DRAIN:
      retval = spell_energy_drain(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_FIREBALL:
      retval = spell_fireball(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_LIGHTNING_BOLT:
      retval = spell_lightning_bolt(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_HARM:
      retval = spell_harm(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_POISON:
      retval = spell_poison(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_SLEEP:
      retval = spell_sleep(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_FEAR:
      retval = spell_fear(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_DISPEL_MAGIC:
      retval = spell_dispel_magic(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_WEAKEN:
      retval = spell_weaken(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_CAUSE_LIGHT:
      retval = spell_cause_light(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_CAUSE_CRITICAL:
      retval = spell_cause_critical(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_PARALYZE:
      retval = spell_paralyze(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_ACID_BLAST:
      retval = spell_acid_blast(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_BEE_STING:
      retval = spell_bee_sting(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_CURE_LIGHT:
      retval = spell_cure_light(ch->getLevel(), ch, ch, weap, wep_skill);
      break;
    case WEP_FLAMESTRIKE:
      retval = spell_flamestrike(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_HEAL_SPRAY:
      retval = spell_heal_spray(ch->getLevel(), ch, ch, weap, wep_skill);
      break;
    case WEP_DROWN:
      retval = spell_drown(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_HOWL:
      retval = spell_howl(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_SOULDRAIN:
      retval = spell_souldrain(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_SPARKS:
      retval = spell_sparks(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_DISPEL_GOOD:
      retval = spell_dispel_good(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_TELEPORT:
      retval = spell_teleport(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_CHILL_TOUCH:
      retval = spell_chill_touch(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_POWER_HARM:
      retval = spell_power_harm(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_VAMPIRIC_TOUCH:
      retval = spell_vampiric_touch(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_LIFE_LEECH:
      retval = spell_life_leech(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_METEOR_SWARM:
      retval = spell_meteor_swarm(ch->getLevel(), ch, vict, weap, wep_skill);
      break;
    case WEP_ENTANGLE:
      /* This is a hack since Morc did the spell wrong  - pir */
      retval = cast_entangle(ch->getLevel(), ch, "", 0, vict, weap, wep_skill);
      break;
    case WEP_CREATE_FOOD:
      retval = cast_create_food(ch->getLevel(), ch, "", 0, vict, weap, wep_skill);
      break;
    case WEP_WILD_MAGIC:
      retval = cast_wild_magic(ch->getLevel(), ch, "offense", 0, vict, weap, wep_skill);
      break;
      /*
          case WEP_THIEF_POISON:
            retval = handle_poisoned_weapon_attack(ch, vict, percent);
            break;
      */
    default:
      retval = ReturnValue::eSUCCESS;
      // Don't want to log this since a non-spell affect is going to happen all
      // the time (like SAVE_VS_FIRE or HIT-N-DAM for example) -pir
      // dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Illegal affect %d in weapons spells item '%d'.",
      //     current_affect, dc_->obj_index[ch->equipment[weapon]->item_number].vnum());
      break;
    } /* switch statement */
    if (SOMEONE_DIED(retval))
      return debug_retval(ch, vict, retval);

  } /* for loop */

  if (dc_->obj_index[weap->item_number].progtypes & WEAPON_PROG)
    ch->oprog_weapon_trigger(weap);

  return ReturnValue::eSUCCESS;
} /* spell effects */

qint32 act_poisonous(CharacterPtr ch)
{
  if (ch->isNonPlayer() && ISSET(ch->mobdata->actflags, ACT_POISONOUS))
    if (!number<quint64>(0, ch->getLevel() / 10))
      return true; // poisoned

  return false;
}

qint32 second_attack(CharacterPtr ch)
{
  qint32 learned;

  if ((ch->isNonPlayer()) && (ISSET(ch->mobdata->actflags, ACT_2ND_ATTACK)))
    return true;
  if (ch->affected_by_spell(SKILL_DEFENDERS_STANCE))
    return false;
  learned = ch->has_skill(SKILL_SECOND_ATTACK);
  if (learned && skill_success(ch, nullptr, SKILL_SECOND_ATTACK, 15))
  {
    return true;
  }
  return false;
}

qint32 third_attack(CharacterPtr ch)
{
  qint32 learned;

  if ((ch->isNonPlayer()) && (ISSET(ch->mobdata->actflags, ACT_3RD_ATTACK)))
    return true;
  if (ch->affected_by_spell(SKILL_DEFENDERS_STANCE))
    return false;
  learned = ch->has_skill(SKILL_THIRD_ATTACK);
  if (learned && skill_success(ch, nullptr, SKILL_THIRD_ATTACK, 15))
  {
    return true;
  }
  return false;
}

qint32 fourth_attack(CharacterPtr ch)
{
  if ((ch->isNonPlayer()) && (ISSET(ch->mobdata->actflags, ACT_4TH_ATTACK)))
    return true;
  return false;
}

/*  No longer used.  Any class can try to use their second wield if they have
    the skill.
qint32 second_wield(CharacterPtr ch)
{
  // If the ch is capable of using the WEAR_SECOND_WIELD
  if((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_MONK))
    return false;
  return true;
}
*/

void inform_victim(CharacterPtr ch, CharacterPtr victim, qint32 dam)
{
  qint32 max_hit;

  switch (GET_POS(victim))
  {
  case position_t::STUNNED:
    // This was moved into "attack" so that the message only goes off
    // once on the players first attack.
    //        act("$n is stunned, but will probably recover.",
    //        victim, 0, 0, TO_ROOM, INVIS_NULL);
    victim->sendln("You are stunned, but will probably recover.");
    break;
  case position_t::DEAD:
    act_to_room("$n is DEAD!!", victim, 0, 0, INVIS_NULL);
    victim->sendln("You have been KILLED!!\r\n");

    break;
  default:
    max_hit = hit_limit(victim);
    if (dam > max_hit / 5)
      victim->sendln("That really did HURT!");
    // Wimp out?
    if (victim->getHP() < (max_hit / 5))
    {
      if (isSet(victim->combat, COMBAT_BERSERK) ||
          isSet(victim->combat, COMBAT_RAGE1) ||
          isSet(victim->combat, COMBAT_RAGE2))
      {
        send_to_char("You are too OUTRAGED to care about your "
                     "wounds!\r\n",
                     victim);
        return;
      }
      if (dam > 0)
        victim->sendln("You wish you would stop BLEEDING so much!");

      if (victim->isNonPlayer())
      {
        if (ISSET(victim->mobdata->actflags, ACT_WIMPY))
        {
          remove_memory(victim, 't');
          remove_memory(victim, 'h', victim);
          if (victim->fighting)
            victim->add_memory(qPrintable(victim->fighting->name()), 'f');
          if ((!IS_AFFECTED(victim, AFF_PARALYSIS)) &&
              (!isSet(victim->combat, COMBAT_STUNNED)) &&
              (!isSet(victim->combat, COMBAT_STUNNED2)) &&
              (!isSet(victim->combat, COMBAT_BASH1)) &&
              (!isSet(victim->combat, COMBAT_BASH2)) &&
              (dam > 0))
            do_flee(victim, "");
          return;
        } // end of if ACT_WIMPY
      } // end of if npc
      else
      {
        if (isSet(victim->player->toggles, Player::PLR_WIMPY))
        {
          if ((!IS_AFFECTED(victim, AFF_PARALYSIS)) &&
              (!isSet(victim->combat, COMBAT_STUNNED)) &&
              (!isSet(victim->combat, COMBAT_STUNNED2)) &&
              (!isSet(victim->combat, COMBAT_BASH1)) &&
              (!isSet(victim->combat, COMBAT_BASH2)) &&
              (dam > 0))
            do_flee(victim, "");
          return;
        }
      } // end else
    } // end max_hit / 5
    break;
  } // switch
} // inform_victim

/*****
| This function will return non-zero if the
| ch is fighting a NON-pkill fight; that is,
| if ch loses s/he will die a real death.
| If the loser will be considered pkilled then
| the function returns 0.
| Morc 8/6/95
*/
bool is_fighting_mob(CharacterPtr ch)
{
  CharacterPtr fighting = ch->fighting;
  if (!fighting)
    return 0;
  if (is_pkill(ch, fighting))
    return 0;
  if (fighting->isNonPlayer() && ch->isPlayer())
    return 1;
  return 0;
}

command_return_t do_flee(CharacterPtr ch, const QString argument, cmd_t cmd)
{
  qint32 i, attempt, retval, escape = {};
  CharacterPtr chTemp, loop_ch, vict = {};

  if (is_stunned(ch))
    return ReturnValue::eFAILURE;

  if (cmd == cmd_t::ESCAPE)
  {
    if (ch->isPlayer() && !(escape = ch->has_skill(SKILL_ESCAPE)))
    {
      ch->sendln("Huh?");
      return ReturnValue::eFAILURE;
    }
    if (ch->isNonPlayer())
      escape = 50 + ch->getLevel() / 3;

    if (!ch->fighting)
    {
      ch->sendln("But there is nobody from whom to escape!");
      return ReturnValue::eFAILURE;
    }
    else
    {
      vict = ch->fighting;
    }

    if (!charge_moves(ch, SKILL_ESCAPE))
      return ReturnValue::eFAILURE;

    ch->skill_increase_check(SKILL_ESCAPE, escape, SKILL_INCREASE_HARD);
    if (ch->dc_->number(1, 101) > MIN((GET_INT(ch) + GET_DEX(ch) + (qreal)escape / 1.5 - GET_INT(vict) / 2 - GET_WIS(vict) / 2), 100))
      escape = {};
  }

  if (IS_AFFECTED(ch, AFF_SNEAK) && !escape)
  {
    affect_from_char(ch, SKILL_SNEAK);
    REMBIT(ch->affected_by, AFF_SNEAK); // Mobs don't always have the affect
  }

  if (GET_CLASS(ch) == CLASS_BARD && IS_SINGING(ch))
    do_sing(ch, "stop");

  if (IS_AFFECTED(ch, AFF_NO_FLEE))
  {
    if (ch->affected_by_spell(SPELL_IRON_ROOTS))
      ch->sendln("The roots bracing your legs make it impossible to run!");
    else
      ch->sendln("Your legs are too tired for running away!");
    return ReturnValue::eFAILURE;
  }

  for (i = {}; i < 3; i++)
  {
    attempt = dc_->number(0, 5); // Select a random direction

    // keep mobs from fleeing into a no_track room
    if (CAN_GO(ch, attempt))
      if (ch->isPlayer() || !isSet(dc_->world[EXIT(ch, attempt)->to_room].room_flags, NO_TRACK))
      {
        if (!escape)
        {
          act_to_room("$n panics, and attempts to flee.", ch, 0, 0, INVIS_NULL);
          act_to_character("You panic, and attempt to flee.", ch, 0, 0, 0);
        }
        else
        {
          act_to_character("You quickly duck $N's attack and attempt to make good your escape!", ch, 0, vict, 0);
          act_to_victim("$n quickly ducks your attack and attempts to make good a stealthy escape!", ch, 0, vict, 0);
          act_to_room("$n quickly ducks $N's attack and attempts to make good a stealthy escape!", ch, 0, vict, INVIS_NULL | NOTVICT);
        }
        // The flee has succeeded
        CharacterPtr last_fighting = ch->fighting;
        ch->setStanding();
        ;

        QString tempcommand = dirs[attempt];

        // we do this so that any spec procs overriding movement can take effect
        SET_BIT(ch->combat, COMBAT_FLEEING);
        retval = ch->command_interpreter(tempcommand);
        // so that a player doesn't keep the flag afte rdying
        if (ch->isPlayer())
          REMOVE_BIT(ch->combat, COMBAT_FLEEING);

        if (isSet(retval, ReturnValue::eCH_DIED))
          return retval;
        REMOVE_BIT(ch->combat, COMBAT_FLEEING);
        if (isSet(retval, ReturnValue::eSUCCESS))
        {
          stop_fighting(ch);

          // Since the move stops the fight between ch and ch->fighting we have to check_pursuit
          // against it separate than the combat_list loop
          if (cmd == cmd_t::FLEE && last_fighting)
          {
            last_fighting->check_pursuit(ch, tempcommand);
          }

          // They got away.  Stop fighting for everyone not in the new room from fighting
          for (chTemp = combat_list; chTemp; chTemp = loop_ch)
          {
            loop_ch = chTemp->next_fighting;
            if (chTemp->fighting == ch && chTemp->in_room != ch->in_room)
            {
              stop_fighting(chTemp);
              if (cmd == cmd_t::FLEE)
                chTemp->check_pursuit(ch, tempcommand);
            }
          } // for

          return ReturnValue::eSUCCESS;
        }
        else
        {
          if (!isSet(retval, ReturnValue::eCH_DIED))
            act_to_room("$n tries to flee, but is too exhausted!", ch, 0, 0, INVIS_NULL);

          if (last_fighting)
            ch->setPOSFighting();

          return retval;
        }
      } // if CAN_GO
  } // for

  // No exits were found
  ch->sendln("PANIC! You couldn't escape!");
  return ReturnValue::eFAILURE;
}

command_return_t Character::check_pursuit(CharacterPtr victim, QString dircommand)
{
  // Handle pursuit skill
  if (victim == 0 || this->isNonPlayer() || !affected_by_spell(SKILL_PURSUIT))
    return ReturnValue::eFAILURE;

  qint32 pursuit = this->has_skill(SKILL_PURSUIT);
  if (ch->dc_->number(1, 100) > pursuit)
  {
    // failure
    act_to_character("$N fled quickly before you were able to pursue $m!", this, 0, victim, 0);
    WAIT_STATE(this, DC::PULSE_VIOLENCE);
  }
  else
  {
    // succeeded
    stop_fighting(victim);
    if (!charge_moves(SKILL_PURSUIT))
      return ReturnValue::eFAILURE;

    act_to_character("Upon seeing $N flee, you bellow in rage and charge blindly after $m!", this, 0, victim, 0);
    act_to_room("Upon seeing $N flee, $n bellows in rage and charges blindly after $m!", this, 0, victim, NOTVICT);

    qint32 retval = this->command_interpreter(dircommand);
    if (isSet(retval, ReturnValue::eCH_DIED))
      return ReturnValue::eSUCCESS;

    act_to_character("Spotting $N nearby, you rush in towards $m and furiously attack!", this, 0, victim, 0);
    act_to_victim("$n charges in with a bellow of rage, cutting of your escape and attacks you furiously!", this, 0, victim, 0);
    act_to_room("$n charges in with a bellow of rage, cutting of $N's escape and attacks $m furiously!", this, 0, victim, NOTVICT);
    attack(this, victim, TYPE_UNDEFINED);
    WAIT_STATE(this, DC::PULSE_VIOLENCE);
  }

  return ReturnValue::eSUCCESS;
}

qint32 get_weapon_bit(qint32 weapon_type)
{
  switch (weapon_type)
  {
  case TYPE_HIT:
    return (ISR_HIT);
  case TYPE_BLUDGEON:
    return (ISR_BLUDGEON);
  case TYPE_PIERCE:
    return (ISR_PIERCE);
  case TYPE_SLASH:
    return (ISR_SLASH);
  case TYPE_WHIP:
    return (ISR_WHIP);
  case TYPE_CLAW:
    return (ISR_CLAW);
  case TYPE_BITE:
    return (ISR_BITE);
  case TYPE_STING:
    return (ISR_STING);
  case TYPE_CRUSH:
    return (ISR_CRUSH);

  case TYPE_PHYSICAL_MAGIC:
  case TYPE_MAGIC:
    return (ISR_MAGIC);

  case TYPE_CHARM:
    return (ISR_CHARM);
  case TYPE_FIRE:
    return (ISR_FIRE);
  case TYPE_ENERGY:
    return (ISR_ENERGY);
  case TYPE_ACID:
    return (ISR_ACID);
  case TYPE_POISON:
    return (ISR_POISON);
  case TYPE_SLEEP:
    return (ISR_SLEEP);
  case TYPE_COLD:
    return (ISR_COLD);
  case TYPE_PARA:
    return (ISR_PARA);
  case TYPE_SONG:
    return (ISR_SONG);
  case TYPE_KI:
    return (ISR_KI);
  case TYPE_WATER:
    return (ISR_WATER);
  default:
    return {};
  } /* end switch */
  return {};
}

void remove_nosave(CharacterPtr vict)
{
  ObjectPtr o, *next_obj, *blah, tmp_o;

  if (!vict)
  {
    dc_->logentry(u"Null victim sent to remove_nosave!"_s, OVERSEER, DC::LogChannel::LOG_BUG);
    return;
  }

  for (o = vict->carrying; o; o = next_obj)
  {
    next_obj = o->next_content;

    // I suppose it would be possible to have a NOSAVE container --
    // in this case you lose everything inside the container.

    if (GET_ITEM_TYPE(o) == ITEM_CONTAINER &&
        !isSet(o->obj_flags.extra_flags, ITEM_NOSAVE))
    {
      for (tmp_o = o->contains; tmp_o; tmp_o = blah)
      {
        blah = tmp_o->next_content;
        if (isSet(tmp_o->obj_flags.extra_flags, ITEM_NOSAVE))
          move_obj(tmp_o, vict->in_room);
      }
    }

    if (isSet(o->obj_flags.extra_flags, ITEM_NOSAVE))
      move_obj(o, vict->in_room);

  } // for
}

void remove_active_potato(CharacterPtr vict)
{
  ObjectPtr obj, next_obj;
  // QString buf;

  if (!vict)
  {
    dc_->logentry(u"Null victim sent to remove_active_potato!"_s, OVERSEER, DC::LogChannel::LOG_BUG);
    return;
  }

  for (obj = vict->carrying; obj; obj = next_obj)
  {
    next_obj = obj->next_content;
    if (dc_->obj_index[obj->item_number].vnum() == 393 && obj->obj_flags.value[3] > 0)
    {
      extract_obj(obj);
    }
  }
}

qint32 damage_type(qint32 weapon_type)
{
  switch (weapon_type)
  {
  case TYPE_HIT:
  case TYPE_BLUDGEON:
  case TYPE_PIERCE:
  case TYPE_SLASH:
  case TYPE_WHIP:
  case TYPE_CLAW:
  case TYPE_BITE:
  case TYPE_STING:
  case TYPE_CRUSH:
    return (DAMAGE_TYPE_PHYSICAL);

  case TYPE_MAGIC:
  case TYPE_CHARM:
  case TYPE_FIRE:
  case TYPE_ENERGY:
  case TYPE_ACID:
  case TYPE_POISON:
  case TYPE_SLEEP:
  case TYPE_COLD:
  case TYPE_PARA:
  case TYPE_KI:
  case TYPE_WATER:
  case TYPE_PHYSICAL_MAGIC:
    return (DAMAGE_TYPE_MAGIC);

  case TYPE_SONG:
    return (DAMAGE_TYPE_SONG);

  default:
    return {};
  } /* end switch */
  return {};
}

qint32 debug_retval(CharacterPtr ch, CharacterPtr victim, qint32 retval)
{
  static qint32 dumped = {};
  bool bugged = false;

  // Only coredump up to 10 times
  if (bugged && dumped < 10)
  {
    produce_coredump();
    dumped++;
  }

  return retval;
}

void Character::send(const QString buffer)
{
  send_to_char(buffer, this);
}

void Character::send(QString buffer)
{
  send_to_char(buffer.c_str(), this);
}

void Character::send(QString buffer)
{
  send_to_char(buffer, this);
}

void Character::sendRaw(QString buffer)
{
  if (this->desc != nullptr)
  {
    this->desc->allowColor = {};
  }

  this->send(buffer);

  if (this->desc != nullptr)
  {
    this->desc->allowColor = 1;
  }
}

command_return_t Character::tell(CharacterPtr victim, QString message)
{
  if (victim && !message.isEmpty())
  {
    return do_tell((victim->name() + " " + message).split(' '), cmd_t::TELL);
  }
  return ReturnValue::eFAILURE;
}
