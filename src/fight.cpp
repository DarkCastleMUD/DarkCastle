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

#include <assert.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>

#include "fight.h"
#include "levels.h"
#include "race.h"
#include "player.h" // log
#include "character.h"
#include "utility.h" // log
#include "connect.h"
#include "spells.h" // weapon_spells
#include "isr.h"
#include "mobile.h"
#include "room.h"
#include "handler.h"
#include "interp.h"   // do_flee()
#include "fileinfo.h" // SAVE_DIR
#include "db.h"       // fread_string()
#include "connect.h"  // Connection
#include "magic.h"    // weapon spells
#include "terminal.h" // YELLOW, etc..
#include "act.h"
#include "clan.h"
#include "returnvals.h"
#include "sing.h" // stop_grouped_bards
#include "innate.h"
#include "token.h"
#include "vault.h"
#include "arena.h"
#include "const.h"
#include "corpse.h"
#include "stat.h"

#define MAX_CHAMP_DEATH_MESSAGE 14

extern room_t top_of_world;

extern struct index_data *mob_index;
extern struct index_data *obj_index;

Character *combat_list = nullptr, *combat_next_dude = nullptr;

char *champ_death_messages[] =
    {
        /*1*/ "\n\r##Somewhere a village has been deprived of their idiot.\r\n",
        "\n\r##Don't feel bad %s. A lot of people have no talent!\n\r",
        "\n\r##If at first you don't succeed, failure may be your style.\r\n",
        "\n\r##%s just found the cure for stupidity. Death.\r\n",
        /*5*/ "\n\r##%s just succumbed to a fatal case of stupidity.\r\n",
        "\n\r##%s: About as useful as a windshield wiper on a goat's ass.\r\n",
        "\n\r##Proof of reincarnation. Nobody could be as stupid as %s in one lifetime.\r\n",
        "\n\r##%s: The poster child for birth control.\r\n",
        "\n\r##Someone... we're not saying who... but someone is an unmitigated moron.\r\n",
        /*10*/ "\n\r##%s wasn't ready.\r\n",
        "\n\r##EPIC FAIL\n\r",
        "\n\r##%s wants a do-over.\r\n",
        "\n\r##%s prays, 'THIS GAME SUCKS'\n\r",
        "\n\r##DarkCastle: Helping people like %s to die since 1991.\r\n"};

int debug_retval(Character *ch, Character *victim, int retval);

void do_champ_flag_death(Character *victim)
{

  char buf[MAX_STRING_LENGTH];
  Object *obj;
  obj = get_obj_in_list_num(real_object(CHAMPION_ITEM), victim->carrying);
  if (obj)
  {
    obj_from_char(obj);
    obj_to_room(obj, CFLAG_HOME);
    snprintf(buf, 200, champ_death_messages[number(0, MAX_CHAMP_DEATH_MESSAGE - 1)], victim->getNameC());
    send_info(buf);
    snprintf(buf, 200, "##%s has just died with the Champion flag, watch for it to reappear!\n\r", victim->getNameC());
    send_info(buf);
  }
  else
  {
    logentry("Champion without the flag, no bueno amigo!", IMMORTAL, LogChannels::LOG_BUG);
  }
}

bool someone_fighting(Character *ch)
{
  Character *vict;
  if (ch->fighting && ch->fighting->fighting == ch)
    return true;
  for (vict = DC::getInstance()->world[ch->in_room].people; vict; vict = vict->next_in_room)
  {
    if (vict->fighting == ch)
      return true;
  }
  return false;
}

int check_autojoiners(Character *ch, int skill = 0)
{
  if (IS_NPC(ch))
    return eFAILURE; // irrelevant
  if (!ch->fighting)
    return eFAILURE;
  if (ch->player && ch->player->unjoinable == true)
    return eFAILURE;
  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
    return eFAILURE;

  Character *tmp;
  for (tmp = DC::getInstance()->world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if (tmp == ch || tmp == ch->fighting)
      continue;
    if (IS_NPC(tmp))
      continue;
    if (tmp->fighting)
      continue;
    if (!tmp->desc)
      continue;
    if (GET_POS(tmp) != position_t::STANDING)
      continue;
    if (tmp->player == nullptr || tmp->player->joining.empty())
      continue;
    if (!isname(GET_NAME(ch), tmp->player->joining))
      continue;
    if (IS_AFFECTED(tmp, AFF_INSANE))
      continue;
    if (skill && !skill_success(tmp, ch, SKILL_FASTJOIN))
      continue;
    int retval = tmp->do_join(QString("0.%1").arg(ch->getName()).split(' '));
    if (SOMEONE_DIED(retval))
      return retval;
  }
  return eSUCCESS;
}

int check_joincharmie(Character *ch, int skill = 0)
{
  if (IS_PC(ch))
    return eFAILURE; // irrelevant
  if (!ch->fighting)
    return eFAILURE;

  Character *tmp = ch->master;
  if (!tmp)
    return eFAILURE;
  if (tmp == ch || tmp == ch->fighting)
    return eFAILURE;
  if (!tmp->desc)
    return eFAILURE;
  if (IS_NPC(tmp))
    return eFAILURE;
  if (tmp->fighting)
    return eFAILURE;
  if (GET_POS(tmp) != position_t::STANDING)
    return eFAILURE;
  if (!tmp->player || tmp->player->joining.empty())
    return eFAILURE;
  if (!isname("follower", tmp->player->joining) &&
      !isname("followers", tmp->player->joining))
    return eFAILURE;
  if (skill && !skill_success(tmp, ch, SKILL_FASTJOIN))
    return eFAILURE;
  return tmp->do_join({"follower"});
}

int Character::check_charmiejoin(void)
{
  if (fighting || !master || master == this || !isStanding())
  {
    return eFAILURE;
  }

  return do_join(QString("0.%1").arg(master->getName()).split(' '));
}

int check_charmiejoin(Character *ch)
{
  if (ch)
  {
    return ch->check_charmiejoin();
  }
  else
  {
    return eFAILURE;
  }
}

void perform_violence(void)
{
  // char debug[256];
  Character *ch;
  int is_mob = 0;
  int retval;
  static struct affected_type *af, *next_af_dude;
  struct follow_type *fol, *folnext;
  extern char *spell_wear_off_msg[];

  if (!combat_list)
    return;

  for (ch = combat_list; ch; ch = combat_next_dude)
  {
    // if combat_next_dude gets killed, it get's updated in "stop_fighting"
    // pretty kludgy way to do it, but it work
    combat_next_dude = ch->next_fighting;

    if (!ch->fighting)
    {
      logentry("Error in perform_violence()!  Null ch->fighting!", IMMORTAL, LogChannels::LOG_BUG);
      return;
    }

    // DEBUG CODE
    int last_virt = -1;
    int last_class = GET_CLASS(ch);
    if (IS_MOB(ch))
      last_virt = mob_index[ch->mobdata->nr].virt;
    // DEBUG CODE
    if (!ch->fighting)
      continue;

    if (ch->in_room != ch->fighting->in_room)
    { // Fix for the whacky fighting someone who's not here thing.
      stop_fighting(ch);
      continue;
    }
    int retval = check_autojoiners(ch);
    if (SOMEONE_DIED(retval))
      continue;
    if (IS_AFFECTED(ch, AFF_CHARM))
      retval = check_joincharmie(ch);
    if (SOMEONE_DIED(retval))
      continue;
    if (IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_CHARMIEJOIN))
    {
      if (ch->followers)
      {
        for (fol = ch->followers; fol; fol = folnext)
        {
          folnext = fol->next;
          if (IS_AFFECTED(fol->follower, AFF_CHARM) && ch->in_room == fol->follower->in_room)
            retval = check_charmiejoin(fol->follower);
          if (DC::isSet(retval, eVICT_DIED))
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
      is_mob = IS_MOB(ch);
      if (is_mob)
      {
        if ((mob_index[ch->mobdata->nr].combat_func) && MOB_WAIT_STATE(ch) <= 0)
        {
          retval = ((*mob_index[ch->mobdata->nr].combat_func)(ch, nullptr, 0, "", ch));
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
            // sprintf(debug, "DEBUG: Mob: %s (Lag: %d)", GET_SHORT(ch), MOB_WAIT_STATE(ch));
            // MOB_WAIT_STATE(ch) -= DC::PULSE_VIOLENCE; // MIKE
            // logentry(debug, OVERSEER, LogChannels::LOG_BUG);
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
        // if this happened, most likely the mob died somehow during the proc and didn't return eCH_DIED and is
        // now invalid memory.  report what class we were and return
        logf(IMPLEMENTER, LogChannels::LOG_BUG, "Crash bug!!!!  fight.cpp last_class changed (%d) Mob=%d", last_class, last_virt);
        break;
      }
      // DEBUG CODE

      retval = attack(ch, ch->fighting, TYPE_UNDEFINED);

      if (SOMEONE_DIED(retval)) // no point in going anymore
        continue;

      // MOB Progs
      retval = mprog_hitprcnt_trigger(ch, ch->fighting);
      if (SOMEONE_DIED(retval) || !ch || !(ch->fighting) || ch->isDead() || isNowhere(ch) || ch->fighting->isDead() || isNowhere(ch->fighting))
      {
        continue;
      }

      retval = mprog_fight_trigger(ch, ch->fighting);
      if (SOMEONE_DIED(retval) || !ch || !(ch->fighting) || ch->isDead() || isNowhere(ch) || ch->fighting->isDead() || isNowhere(ch->fighting))
      {
        continue;
      }

    } // can_attack
    else if (!is_stunned(ch) && !IS_AFFECTED(ch, AFF_PARALYSIS))
      stop_fighting(ch);

    // This takes care of flee and stuff
    if (ch && ch->fighting)
      if (ch->fighting != (Character *)0x95959595 && ch->in_room != (ch->fighting)->in_room)
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
      logentry("Error in perform_violence(), part2!  Null ch->fighting!", IMMORTAL, LogChannels::LOG_BUG);
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
        int dam = af->duration * 10 + number(30, 50);
        if (get_saves(ch, SAVE_TYPE_POISON) > number(1, 100))
        {
          dam = dam * get_saves(ch, SAVE_TYPE_POISON) / 100;
          ch->sendln("You feel very sick, but resist the $2poison's$R damage.");
        }
        if (dam)
        {
          char dammsg[MAX_STRING_LENGTH];
          sprintf(dammsg, "$B%d$R", dam);
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
        if (affected_by_spell(ch, SPELL_DIVINE_INTER))
        {
          ch->removeHP(affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier);
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
          send_to_char("\n\r", ch);
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

void add_threat(Character *mob, Character *ch, int amt)
{
  struct threat_struct *thr;
  if (!mob || !ch || !amt || IS_PC(mob) || !ch->getNameC())
    return;
  for (thr = mob->mobdata->threat; thr; thr = thr->next)
  {
    if (!str_cmp(ch->getNameC(), thr->name))
    { // Already angry with player.
      thr->threat += amt;
      return;
    }
  }
#ifdef LEAK_CHECK
  thr = (struct threat_struct *)calloc(1, sizeof(struct threat_struct));
#else
  thr = (struct threat_struct *)dc_alloc(1, sizeof(struct threat_struct));
#endif
  thr->next = mob->mobdata->threat;
  thr->threat = amt;
  thr->name = str_dup(GET_NAME(ch));
  mob->mobdata->threat = thr;
  // Threat added.
  return;
}

// could use bits for all that, but I dun' wanna.
#define DAMAGE 1
#define HEALING 2
#define AREA_DAM 3
#define AREA_HEAL 4

void generate_skillthreat(Character *mob, int skill, int damage, Character *actor, Character *target)
{
  if (!actor || !mob || IS_PC(mob))
    return;
  struct threat_struct *thr;
  float v = (float)actor->has_skill(skill) / 100.0;
  if (!v)
    v = 0.4; // like weapons
  int type = 0;
  int threat = 0;
  switch (skill)
  {
  case SPELL_HELLSTREAM:
    threat = 100;
    type = DAMAGE;
    break;
  };
  if (!threat)
  { // Nothing set. Bugger.
    logf(110, LogChannels::LOG_BUG, "Skill/spell %s(%d) missing threatsetting.", get_skill_name(skill), skill);
    return;
  }
  threat = (int)(threat * v); // vary depending on skill

  if (type == DAMAGE)
  {
    if (!target)
      return; // damage spell without a target? right.

    if (target != mob && target != actor)
    {
      // damaging yerself gets you no coochie coochie
      for (thr = mob->mobdata->threat; thr; thr = thr->next)
        if (!str_cmp(thr->name, GET_NAME(target)))
          threat = 0 - threat; // this guy deserves mucho love, let's provide.
      // it damaged something else, get pissed off if friendly flagged
      if (threat > 0 && !ISSET(mob->mobdata->actflags, ACT_FRIENDLY))
        return;
    }
    add_threat(mob, actor, threat);
  }
}

bool gets_dual_wield_attack(Character *ch)
{
  if (!ch->equipment[SECOND_WIELD]) // only if we have a second wield:)
    return false;

  if (!ch->has_skill(SKILL_DUAL_WIELD))
    return false;

  if (!skill_success(ch, nullptr, SKILL_DUAL_WIELD, 15))
    return false;

  return true;
}

// int attack(...) FUNCTION SHOULD BE CALLED *INSTEAD* OF HIT IN ALL CASES!
// standard retvals
int attack(Character *ch, Character *vict, int type, int weapon)
{
  int result = 0;
  int chance;
  Object *wielded = 0;
  int handle_any_guard(Character * ch);

  if (!ch || !vict)
  {
    logentry("nullptr Victim or Ch sent to attack!  This crashes us!", -1, LogChannels::LOG_BUG);
    produce_coredump();

    return eINTERNAL_ERROR;
  }

  if (GET_POS(ch) == position_t::DEAD)
  {
    logentry("Dead ch sent to attack. Wtf ;)", -1, LogChannels::LOG_BUG);
    produce_coredump();
    stop_fighting(ch);

    return eINTERNAL_ERROR;
  }

  // victim could be dead if a skill like do_ki causes folowers to autojoin and kill
  // before attack gets called
  if (GET_POS(vict) == position_t::DEAD)
  {
    stop_fighting(ch);
    return eFAILURE;
  }

  if (!can_attack(ch))
    return eFAILURE;
  if (!can_be_attacked(ch, vict))
    return eFAILURE;

  // TODO - until I can make sure that area effects don't attack other mobs
  // when cast by mobs, I need to make sure mobs aren't killing each other
  if (IS_NPC(ch) && IS_NPC(vict) &&
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
    do_say(ch, std::string("I'm sorry my fellow mob, I have seen the error of my ways."), 0);
    do_say(vict, std::string("It is okay my friend, let's go have a beer."), 0);
    return eFAILURE;
  }

  set_cantquit(ch, vict); // This sets the flag if necessary

  if (!vict->fighting && vict->in_room == ch->in_room)
    set_fighting(vict, ch); // So attacker starts round #2.
  else if (vict->in_room == ch->in_room)
    set_fighting(ch, vict);
  wielded = ch->equipment[WIELD];

  if (type != SKILL_BACKSTAB)
    if (handle_any_guard(vict))
    {
      if ((vict = ch->fighting) == nullptr)
        return eFAILURE;
    }
  assert(vict);

  if (ch->has_skill(SKILL_NAT_SELECT) &&
      affected_by_spell(ch, SKILL_NAT_SELECT) &&
      affected_by_spell(ch, SKILL_NAT_SELECT)->modifier == GET_RACE(vict) &&
      number(0, 3) == 0)
  {
    skill_increase_check(ch, SKILL_NAT_SELECT, ch->has_skill(SKILL_NAT_SELECT),
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
      result = one_hit(ch, vict, type, FIRST);
      if (SOMEONE_DIED(result))
        return result;
    }

    if (ch->getLevel() > 47)
      if (number(0, 1))
      {
        result = one_hit(ch, vict, type, FIRST);
        if (SOMEONE_DIED(result))
          return result;
      }

    if (ch->getLevel() > 39)
    {
      result = one_hit(ch, vict, type, FIRST);
      if (SOMEONE_DIED(result))
        return result;
    }
    else if (ch->getLevel() > 29)
      if (number(0, 1))
      {
        result = one_hit(ch, vict, type, FIRST);
        if (SOMEONE_DIED(result))
          return result;
      }

    if (ch->getLevel() > 19)
    {
      result = one_hit(ch, vict, type, FIRST);
      if (SOMEONE_DIED(result))
        return result;
    }

    if (IS_AFFECTED(ch, AFF_HASTE))
    {
      if (affected_by_spell(ch, SPELL_HASTE)) // spell is 33%
        chance = 33;
      else
        chance = 66; // eq/bard is 66%
      if ((ch->equipment[WIELD] && obj_index[ch->equipment[WIELD]->item_number].virt == 586) ||
          (ch->equipment[SECOND_WIELD] && obj_index[ch->equipment[SECOND_WIELD]->item_number].virt == 586))
        chance = 101;
      if (chance > number(1, 100))
      {
        result = one_hit(ch, vict, type, FIRST);
        if (SOMEONE_DIED(result))
          return result;
      }
    }

    // lose an attack if using a shield
    //    if(ch->getLevel() > 9 && !ch->equipment[WEAR_SHIELD]) {
    //    result = one_hit(ch, vict, type, FIRST);
    //  if(SOMEONE_DIED(result))       return result;
    // }

    if (DC::isSet(ch->combat, COMBAT_MISS_AN_ATTACK) || IS_AFFECTED(ch, AFF_CRIPPLE))
    {
      ch->sendln("Your body refuses to work properly and you miss an attack.");
      REMOVE_BIT(ch->combat, COMBAT_MISS_AN_ATTACK);
    }
    else if (IS_PC(ch) || !ISSET(ch->mobdata->actflags, ACT_NOATTACK))
    {
      result = one_hit(ch, vict, type, FIRST);
      if (SOMEONE_DIED(result))
        return result;
    }
  }    // End of the monk attacks
  else // It's a normal attack
  {
    if (DC::isSet(ch->combat, COMBAT_MISS_AN_ATTACK) || IS_AFFECTED(ch, AFF_CRIPPLE))
    {
      ch->sendln("Your body refuses to work properly and you miss an attack.");
      REMOVE_BIT(ch->combat, COMBAT_MISS_AN_ATTACK);
    }
    else if (IS_PC(ch) || !ISSET(ch->mobdata->actflags, ACT_NOATTACK))
    {
      result = one_hit(ch, vict, type, FIRST); // everyone get's one hit (normally)
      if (SOMEONE_DIED(result))
        return result;
    }

    // This is here so we only show this after the PC's first
    // attack rather than after every hit.
    if (GET_POS(vict) == position_t::STUNNED)
      act("$n is $B$0stunned$R, but will probably recover.", vict, 0, 0, TO_ROOM, INVIS_NULL);

    if (second_attack(ch))
    {
      result = one_hit(ch, vict, type, FIRST);
      if (SOMEONE_DIED(result))
        return result;
    }
    if (third_attack(ch))
    {
      result = one_hit(ch, vict, type, FIRST);
      if (SOMEONE_DIED(result))
        return result;
    }
    if (fourth_attack(ch))
    {
      result = one_hit(ch, vict, type, FIRST);
      if (SOMEONE_DIED(result))
        return result;
    }
    if (IS_AFFECTED(ch, AFF_HASTE))
    {
      if (affected_by_spell(ch, SPELL_HASTE)) // spell is 33%
        chance = 33;
      else
        chance = 66; // eq/bard is 66%
      if ((ch->equipment[WIELD] && obj_index[ch->equipment[WIELD]->item_number].virt == 586) ||
          (ch->equipment[SECOND_WIELD] && obj_index[ch->equipment[SECOND_WIELD]->item_number].virt == 586))
        chance = 101;

      if (chance > number(1, 100))
      {
        result = one_hit(ch, vict, type, FIRST);
        if (SOMEONE_DIED(result))
          return result;
      }
    }
    if (affected_by_spell(ch, SKILL_ONSLAUGHT))
    {
      if (number(1, 100) < ch->has_skill(SKILL_ONSLAUGHT) / 2)
      {
        result = one_hit(ch, vict, type, FIRST);
        if (SOMEONE_DIED(result))
          return result;
      }
    }
    // if(second_wield(ch) && gets_dual_wield_attack(ch)) {
    if (gets_dual_wield_attack(ch))
    {
      result = one_hit(ch, vict, type, SECOND);
      if (SOMEONE_DIED(result))
        return result;
    }

    int lrn = ch->has_skill(SKILL_OFFHAND_DOUBLE);
    if (gets_dual_wield_attack(ch) && lrn)
    {
      skill_increase_check(ch, SKILL_OFFHAND_DOUBLE, lrn, SKILL_INCREASE_HARD);
      int p = lrn / 2 + ch->getHP() * 10 / GET_MAX_HIT(ch) - (10 - ch->has_skill(SKILL_SECOND_ATTACK) / 10);
      if (number(1, 100) <= p)
      {
        result = one_hit(ch, vict, type, SECOND);
        if (SOMEONE_DIED(result))
          return result;
      }
    }
  }

  return eSUCCESS;
} // of attack

void update_flags(Character *vict)
{
  if (DC::isSet(vict->combat, COMBAT_BASH1))
  {
    REMOVE_BIT(vict->combat, COMBAT_BASH1);
    SET_BIT(vict->combat, COMBAT_BASH2);
  }
  else if (DC::isSet(vict->combat, COMBAT_BASH2))
    REMOVE_BIT(vict->combat, COMBAT_BASH2);

  if (DC::isSet(vict->combat, COMBAT_RAGE1))
  {
    REMOVE_BIT(vict->combat, COMBAT_RAGE1);
    SET_BIT(vict->combat, COMBAT_RAGE2);
  }
  else if (DC::isSet(vict->combat, COMBAT_RAGE2))
  {
    REMOVE_BIT(vict->combat, COMBAT_RAGE2);
    act("$n calms down a bit.", vict, 0, 0, TO_ROOM, 0);
    act("Your mind seems a bit clearer now.", vict, 0, 0, TO_CHAR, 0);
  }

  if (DC::isSet(vict->combat, COMBAT_CRUSH_BLOW))
  {
    REMOVE_BIT(vict->combat, COMBAT_CRUSH_BLOW);
    SET_BIT(vict->combat, COMBAT_CRUSH_BLOW2);
  }
  else if (DC::isSet(vict->combat, COMBAT_CRUSH_BLOW2))
  {
    REMOVE_BIT(vict->combat, COMBAT_CRUSH_BLOW2);
    act("$n shrugs off $s weakness!", vict, 0, 0, TO_ROOM, 0);
    act("You shrug off your weakness.!", vict, 0, 0, TO_CHAR, 0);
  }

  if (DC::isSet(vict->combat, COMBAT_BLADESHIELD1))
  {
    REMOVE_BIT(vict->combat, COMBAT_BLADESHIELD1);
    SET_BIT(vict->combat, COMBAT_BLADESHIELD2);
  }
  else if (DC::isSet(vict->combat, COMBAT_BLADESHIELD2))
    REMOVE_BIT(vict->combat, COMBAT_BLADESHIELD2);

  if (DC::isSet(vict->combat, COMBAT_VITAL_STRIKE))
    REMOVE_BIT(vict->combat, COMBAT_VITAL_STRIKE);

  if (DC::isSet(vict->combat, COMBAT_ORC_BLOODLUST2))
  {
    REMOVE_BIT(vict->combat, COMBAT_ORC_BLOODLUST2);
    vict->sendln("Your bloodlust fades.");
    act("$n's bloodlust fades.", vict, 0, 0, TO_ROOM, 0);
  }

  if (DC::isSet(vict->combat, COMBAT_ORC_BLOODLUST1))
  {
    REMOVE_BIT(vict->combat, COMBAT_ORC_BLOODLUST1);
    SET_BIT(vict->combat, COMBAT_ORC_BLOODLUST2);
  }

  if (DC::isSet(vict->combat, COMBAT_THI_EYEGOUGE2))
  {
    REMOVE_BIT(vict->combat, COMBAT_THI_EYEGOUGE2);
    REMBIT(vict->affected_by, AFF_BLIND);
    act("$n clears the blood from $s eyes.\r\n", vict, nullptr, nullptr, TO_ROOM, 0);
    vict->sendln("You clear the blood out of your eyes.");
  }

  if (DC::isSet(vict->combat, COMBAT_THI_EYEGOUGE))
  {
    REMOVE_BIT(vict->combat, COMBAT_THI_EYEGOUGE);
    SET_BIT(vict->combat, COMBAT_THI_EYEGOUGE2);
  }

  if (DC::isSet(vict->combat, COMBAT_MONK_STANCE))
  {
    // stance lasts 'modifier' rounds.  Remove bit once used up
    struct affected_type *pspell;
    pspell = affected_by_spell(vict, KI_STANCE + KI_OFFSET);
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

void update_stuns(Character *ch)
{
  if (DC::isSet(ch->combat, COMBAT_SHOCKED))
  {
    REMOVE_BIT(ch->combat, COMBAT_SHOCKED);
    SET_BIT(ch->combat, COMBAT_SHOCKED2);
  }
  else if (DC::isSet(ch->combat, COMBAT_SHOCKED2))
    REMOVE_BIT(ch->combat, COMBAT_SHOCKED2);

  if (DC::isSet(ch->combat, COMBAT_STUNNED))
  {
    REMOVE_BIT(ch->combat, COMBAT_STUNNED);
    SET_BIT(ch->combat, COMBAT_STUNNED2);
  }
  else if (DC::isSet(ch->combat, COMBAT_STUNNED2))
  {
    REMOVE_BIT(ch->combat, COMBAT_STUNNED2);
    if (ch->getHP() > 0)
      if (GET_POS(ch) != position_t::FIGHTING)
      {
        act("$n regains consciousness...", ch, 0, 0, TO_ROOM, 0);
        act("You regain consciousness...", ch, 0, 0, TO_CHAR, 0);
        if (ch->fighting)
          ch->setPOSFighting();
        else
          ch->setStanding();
        ;
        if (DC::isSet(ch->combat, COMBAT_BERSERK))
        {
          ch->sendln("After that period of unconsciousness, you've forgotten what you were mad about.");
          REMOVE_BIT(ch->combat, COMBAT_BERSERK);
        }
      }
  }
}

bool do_frostshield(Character *ch, Character *vict)
{
  if (!ch || !vict)
  {
    logentry("Null ch or vict sent to do_frostshield", IMPLEMENTER, LogChannels::LOG_BUG);
    return (false);
  }
  if (!IS_AFFECTED(vict, AFF_FROSTSHIELD))
  {
    return (false);
  }
  if (number(0, 99) < 5)
  {
    act("Bits of $Bfrost$R fly as $n's blow bounces off your $B$1icy$R shield.", ch, 0, vict, TO_VICT, 0);
    act("Bits of $Bfrost$R fly as $n's blow bounces off $N's $B$1icy$R shield.", ch, 0, vict, TO_ROOM, NOTVICT);
    act("Bits of $Bfrost$R fly as your blow bounces off $N's $B$1icy$R shield.", ch, 0, vict, TO_CHAR, 0);
    return (true);
  }
  else
  {
    return (false);
  }
}

int do_lightning_shield(Character *ch, Character *vict, int dam)
{
  struct affected_type *cur_af;
  int learned = 0;

  if (!ch || !vict)
  {
    logentry("Null ch or vict sent to do_lightning_shield", IMPLEMENTER, LogChannels::LOG_BUG);
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (GET_POS(vict) == position_t::DEAD)
    return eFAILURE;
  if (ch->getLevel() >= IMMORTAL)
    return eFAILURE;
  if (!IS_AFFECTED(vict, AFF_LIGHTNINGSHIELD))
    return eFAILURE;
  if (DC::isSet(races[(int)GET_RACE(ch)].immune, ISR_ENERGY) || DC::isSet(ch->immune, ISR_ENERGY))
  {
    dam = 0;
  }
  else
  {
    if ((cur_af = affected_by_spell(vict, SPELL_LIGHTNING_SHIELD)))
      learned = (int)cur_af->modifier;

    if (learned == 0) // mob
      learned = 81;

    // Close to 50% damage returned for a fully learned lightning shield
    dam *= ((learned / 100.0) * 53.0) / 100.0;

    // Make it fluctuate between 475-525
    if (dam > 500)
    {
      dam = 475;
      dam += number(0, 50);
    }

    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
    if (affected_by_spell(ch, SPELL_HOLY_AURA) && affected_by_spell(ch, SPELL_HOLY_AURA)->modifier == 25)
      dam /= 2;
    if (affected_by_spell(ch, SPELL_DIVINE_INTER) && dam > affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier)
      dam = affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier;
    int save = get_saves(ch, SAVE_TYPE_ENERGY);
    if (number(1, 100) < save || save < 0)
    {
      if (save > 50)
        save = 50;
      dam -= (int)(dam * (double)save / 100);
    }
  }

  /*  if ((learned = ch->has_skill( SKILL_SHIELDBLOCK)) && ch->equipment[WEAR_SHIELD] && GET_CLASS(ch) != CLASS_ANTI_PAL && GET_CLASS(ch) != CLASS_THIEF)
  {
      if (learned/3 > number(0,99)) {
        act("$n deftly blocks your burst of $B$5lightning$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict, TO_VICT, 0);
        act("You defly block $N's burst of $B$5lightning$R with your $p!", ch, ch->equipment[WEAR_SHIELD], vict, TO_CHAR, 0);
        act("$n deftly blocks $N's burst of $B$5lightning$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict, TO_ROOM, NOTVICT);
        return eFAILURE;
      }
    }
    */
  ch->removeHP(dam);
  do_dam_msgs(vict, ch, dam, SPELL_LIGHTNING_SHIELD, WIELD);
  update_pos(ch);

  if (GET_POS(ch) == position_t::DEAD)
  {
    act("$n is DEAD!!", ch, 0, 0, TO_ROOM, INVIS_NULL);
    group_gain(vict, ch);
    if (IS_PC(ch))
      send_to_char("You have been KILLED!!\n\r\n\r", ch);

    fight_kill(vict, ch, TYPE_CHOOSE, 0);
    return eSUCCESS | eCH_DIED;
  }

  return eSUCCESS;
}

// standard retvals
int do_vampiric_aura(Character *ch, Character *vict)
{
  if (!ch || !vict || ch == vict)
  {
    logentry("Null ch or vict, or ch==vict sent to do_vampiric_aura!", IMPLEMENTER, LogChannels::LOG_BUG);
    abort();
  }

  if (GET_POS(vict) == position_t::DEAD)
    return eFAILURE;

  struct affected_type *af;

  if (nullptr == (af = affected_by_spell(vict, SPELL_VAMPIRIC_AURA)))
    return eFAILURE;

  // ch just hit vict...vict has Vampiric aura up
  if (number(0, 101) < ((af->modifier) / 10))
  {
    int level = MAX(1, af->modifier / 6);
    int retval = spell_vampiric_touch(level, vict, ch, 0, af->modifier);
    retval = SWAP_CH_VICT(retval);
    return debug_retval(ch, vict, retval);
  }
  return eFAILURE;
}

// standard retvals
int do_fireshield(Character *ch, Character *vict, int dam)
{
  // ch is the person who just hit the victim
  // so ch takes the damage from this spell
  struct affected_type *cur_af;
  int learned = 0;

  if (!ch || !vict || ch == vict)
  {
    logentry("Null ch or vict, or ch==vict sent to do_fireshield!", IMPLEMENTER, LogChannels::LOG_BUG);
    abort();
  }

  if (GET_POS(vict) == position_t::DEAD)
    return eFAILURE;
  if (IS_PC(ch) && ch->getLevel() >= IMMORTAL)
    return eFAILURE;
  if (!IS_AFFECTED(vict, AFF_FIRESHIELD))
    return eFAILURE;

  if (DC::isSet(races[(int)GET_RACE(ch)].immune, ISR_FIRE) ||
      DC::isSet(ch->immune, ISR_FIRE))
  {
    dam = 0;
  }
  else
  {
    if ((cur_af = affected_by_spell(vict, SPELL_FIRESHIELD)))
      learned = (int)cur_af->modifier;

    if (learned == 0) // mob
      learned = 81;

    // Close to 75% damage returned for a fully learned fireshield
    dam *= ((learned / 100.0) * 79.0) / 100.0;

    // Make it fluctuate between 475-525
    if (dam > 500)
    {
      dam = 475;
      dam += number(0, 50);
    }

    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
    if (affected_by_spell(ch, SPELL_HOLY_AURA) && affected_by_spell(ch, SPELL_HOLY_AURA)->modifier == 25)
      dam /= 2;
    if (affected_by_spell(ch, SPELL_DIVINE_INTER) && dam > affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier)
      dam = affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier;
    int save = get_saves(ch, SAVE_TYPE_FIRE);
    if (number(1, 100) < save || save < 0)
    {
      if (save > 50)
        save = 50;
      dam -= (int)(dam * (double)save / 100);
    }
  }

  /*
    if ((learned = ch->has_skill( SKILL_SHIELDBLOCK)) && ch->equipment[WEAR_SHIELD] && GET_CLASS(ch) != CLASS_ANTI_PAL && GET_CLASS(ch) != CLASS_THIEF) {
      if (learned/3 > number(0,99)) {
        act("$n deftly blocks your burst of $B$4flame$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict, TO_VICT, 0);
        act("You defly block $N's burst of $B$4flame$R with your $p!", ch, ch->equipment[WEAR_SHIELD], vict, TO_CHAR, 0);
        act("$n deftly blocks $N's burst of $B$4flame$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict, TO_ROOM, NOTVICT);
        return eFAILURE;
      }
    }
  */
  ch->removeHP(dam);
  do_dam_msgs(vict, ch, dam, SPELL_FIRESHIELD, WIELD);
  update_pos(ch);

  if (GET_POS(ch) == position_t::DEAD)
  {
    act("$n is DEAD!!", ch, 0, 0, TO_ROOM, INVIS_NULL);
    group_gain(vict, ch);
    if (IS_PC(ch))
      send_to_char("You have been KILLED!!\n\r\n\r", ch);

    fight_kill(vict, ch, TYPE_CHOOSE, 0);
    return eSUCCESS | eCH_DIED;
  }

  return eSUCCESS;
}

// standard retvals
int do_acidshield(Character *ch, Character *vict, int dam)
{
  // ch is the person who just hit the victim
  // so ch takes the damage from this spell
  struct affected_type *cur_af;
  int learned = 0;

  if (!ch || !vict || ch == vict)
  {
    logentry("Null ch or vict, or ch==vict sent to do_acidshield!", IMPLEMENTER, LogChannels::LOG_BUG);
    abort();
  }

  if (GET_POS(vict) == position_t::DEAD)
    return eFAILURE;
  if (IS_PC(ch) && ch->getLevel() >= IMMORTAL)
    return eFAILURE;
  if (!IS_AFFECTED(vict, AFF_ACID_SHIELD))
    return eFAILURE;

  if (DC::isSet(races[(int)GET_RACE(ch)].immune, ISR_ACID) || DC::isSet(ch->immune, ISR_ACID))
    dam = 0;
  else
  {
    if ((cur_af = affected_by_spell(vict, SPELL_ACID_SHIELD)))
      learned = (int)cur_af->modifier;

    if (learned == 0) // mob
      learned = 81;

    // Close to 30% damage returned for a fully learned acidshield
    dam *= ((learned / 100.0) * 32.0) / 100.0;

    // Make it fluctuate between 475-525
    if (dam > 500)
    {
      dam = 475;
      dam += number(0, 50);
    }

    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
    if (affected_by_spell(ch, SPELL_HOLY_AURA) && affected_by_spell(ch, SPELL_HOLY_AURA)->modifier == 25)
      dam /= 2;
    if (affected_by_spell(ch, SPELL_DIVINE_INTER) && dam > affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier)
      dam = affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier;
    int save = get_saves(ch, SAVE_TYPE_ACID);
    if (number(1, 100) < save || save < 0)
    {
      if (save > 50)
        save = 50;
      dam -= (int)(dam * (double)save / 100);
    }
  }

  /*
    if ((learned = ch->has_skill( SKILL_SHIELDBLOCK)) && ch->equipment[WEAR_SHIELD] && GET_CLASS(ch) != CLASS_ANTI_PAL && GET_CLASS(ch) != CLASS_THIEF) {
      if (learned/3 > number(0,99)) {
        act("$n deftly blocks your burst of $B$2acid$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict, TO_VICT, 0);
        act("You defly block $N's burst of $B$2acid$R with your $p!", ch, ch->equipment[WEAR_SHIELD], vict, TO_CHAR, 0);
        act("$n deftly blocks $N's burst of $B$2acid$R with $s $p!", ch, ch->equipment[WEAR_SHIELD], vict, TO_ROOM, NOTVICT);
        return eFAILURE;
      }
    }
    */
  ch->removeHP(dam);
  do_dam_msgs(vict, ch, dam, SPELL_ACID_SHIELD, WIELD);
  update_pos(ch);

  if (GET_POS(ch) == position_t::DEAD)
  {
    act("$n is DEAD!!", ch, 0, 0, TO_ROOM, INVIS_NULL);
    group_gain(vict, ch);
    if (IS_PC(ch))
      send_to_char("You have been KILLED!!\n\r\n\r", ch);

    fight_kill(vict, ch, TYPE_CHOOSE, 0);
    return eSUCCESS | eCH_DIED;
  }

  return eSUCCESS;
}

int do_boneshield(Character *ch, Character *vict, int dam)
{
  // ch is the person who just hit the victim
  // so ch takes the damage from this spell
  struct affected_type *cur_af;
  int learned = 0;

  if (!ch || !vict || ch == vict)
  {
    logentry("Null ch or vict, or ch==vict sent to do_boneshield!", IMPLEMENTER, LogChannels::LOG_BUG);
    abort();
  }

  if (GET_POS(vict) == position_t::DEAD)
    return eFAILURE;
  if (IS_PC(ch) && ch->getLevel() >= IMMORTAL)
    return eFAILURE;
  if (!affected_by_spell(vict, SPELL_BONESHIELD))
    return eFAILURE;

  if (DC::isSet(races[(int)GET_RACE(ch)].immune, ISR_PHYSICAL) ||
      DC::isSet(ch->immune, ISR_PHYSICAL))
    dam = 0;
  else
  {
    if ((cur_af = affected_by_spell(vict, SPELL_BONESHIELD)))
      learned = (int)cur_af->modifier;

    dam = learned;

    dam -= ch->melee_mitigation;

    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
    if (affected_by_spell(ch, SPELL_HOLY_AURA) && affected_by_spell(ch, SPELL_HOLY_AURA)->modifier == 25)
      dam /= 2;
    if (affected_by_spell(ch, SPELL_DIVINE_INTER) && dam > affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier)
      dam = affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier;
  }

  ch->removeHP(dam);
  do_dam_msgs(vict, ch, dam, SPELL_BONESHIELD, WIELD);
  update_pos(ch);

  if (GET_POS(ch) == position_t::DEAD)
  {
    act("$n is DEAD!!", ch, 0, 0, TO_ROOM, INVIS_NULL);
    group_gain(vict, ch);
    if (IS_PC(ch))
      send_to_char("You have been KILLED!!\n\r\n\r", ch);

    fight_kill(vict, ch, TYPE_CHOOSE, 0);
    return eSUCCESS | eCH_DIED;
  }

  return eSUCCESS;
}

void check_weapon_skill_bonus(Character *ch, int type, Object *wielded,
                              int &weapon_skill_hit_bonus, int &weapon_skill_dam_bonus)
{
  int specialization;
  int skill, learned;
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
    weapon_skill_hit_bonus = 0;
    weapon_skill_dam_bonus = 0;
    return;
  }
  learned = ch->has_skill(skill);
  if (skill_success(ch, nullptr, skill))
  {
    // rare skill increases
    specialization = learned / 100;
    learned = learned % 100;

    weapon_skill_hit_bonus = learned / 5;
    weapon_skill_dam_bonus = learned / 10 ? number(1, learned / 10) : 1;

    if (specialization)
    {
      weapon_skill_hit_bonus += 4;
      weapon_skill_dam_bonus += number(1, 5);
    }
  }

  // now check for two-handed weapons
  if (wielded && DC::isSet(wielded->obj_flags.extra_flags, ITEM_TWO_HANDED) &&
      (learned = ch->has_skill(SKILL_TWO_HANDED_WEAPONS)))
  {
    // rare skill increases
    if (0 == number(0, 5))
      skill_increase_check(ch, SKILL_TWO_HANDED_WEAPONS, learned, SKILL_INCREASE_HARD);

    specialization = learned / 100;
    learned = learned % 100;

    weapon_skill_hit_bonus += learned / 6;
    weapon_skill_dam_bonus += learned / 10 ? number(1, learned / 10) : 1;

    if (specialization)
    {
      weapon_skill_hit_bonus += 5;
      weapon_skill_dam_bonus += number(1, 5);
    }
  }
}

int get_weapon_damage_type(class Object *wielded)
{
  char log_buf[256];

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
    sprintf(log_buf, "WORLD: Unknown w_type for object #%d name: %s, fourth value flag is: %d.",
            wielded->item_number, wielded->name, wielded->obj_flags.value[3]);
    logentry(log_buf, OVERSEER, LogChannels::LOG_BUG);
    break;
  }
  return TYPE_HIT; // should never get here
}

int get_monk_bare_damage(Character *ch)
{
  int dam = 0;

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
  else if (ch->getLevel() < IMMORTAL)
    dam = dice(10, 6);
  else if (ch->getLevel() < IMPLEMENTER)
    dam = dice(10, 10);
  else
    dam = dice(50, 5);

  return dam;
}

int calculate_paladin_damage_bonus(Character *ch, Character *victim)
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
int one_hit(Character *ch, Character *vict, int type, int weapon)
{
  class Object *wielded; /* convenience */
  int w_type;            /* Holds type info for damage() */
  int weapon_type;
  int dam = 0; /* Self explan. */
  int retval = 0;
  int weapon_skill_hit_bonus = 0;
  int weapon_skill_dam_bonus = 0;

  extern uint8_t backstab_mult[];

  if (!vict || !ch)
  {
    logentry("Null victim or char in one_hit!  This Crashes us!", -1, LogChannels::LOG_BUG);
    return eINTERNAL_ERROR;
  }

  if (ch == vict)
  {
    do_say(ch, "What the hell am I DOING?!?!", CMD_DEFAULT);
    stop_fighting(ch);
    return eFAILURE;
  }

  if (GET_POS(vict) == position_t::DEAD)
    return (eSUCCESS | eVICT_DIED);

  // TODO - I'd like to remove these 3 cause they are checked in attack()
  /* This happens with multi-attacks */
  if (ch->in_room != vict->in_room)
    return eFAILURE;
  if (!can_be_attacked(ch, vict))
    return eFAILURE;
  if (!can_attack(ch))
    return eFAILURE;

  // Figure out the correct weapon
  wielded = ch->equipment[weapon];

  // Second got called without a secondary wield
  if (weapon == SECOND && wielded == nullptr)
    return eFAILURE;

  set_cantquit(ch, vict); /* This sets the flag if necessary */

  w_type = TYPE_HIT;
  if (wielded && wielded->obj_flags.type_flag == ITEM_WEAPON)
    w_type = get_weapon_damage_type(wielded);

  if (wielded && obj_index[wielded->item_number].virt == 30019 && DC::isSet(wielded->obj_flags.more_flags, ITEM_TOGGLE))
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
    if (affected_by_spell(ch, SKILL_SMITE) && affected_by_spell(ch, SKILL_SMITE)->victim == vict)
      for (int i = 0; i < wielded->obj_flags.value[1]; i++)
        dam += wielded->obj_flags.value[2] - number(0, 1);
    else
      dam = dice(wielded->obj_flags.value[1], wielded->obj_flags.value[2]);
    if (IS_NPC(ch))
    {
      dam = dice(ch->mobdata->damnodice, ch->mobdata->damsizedice);
      dam += (dice(wielded->obj_flags.value[1], wielded->obj_flags.value[2]) / 2);
    }
  }
  else if (IS_NPC(ch))
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

  if (wielded && obj_index[wielded->item_number].virt == 30019 && DC::isSet(wielded->obj_flags.more_flags, ITEM_TOGGLE))
  {
    dam = dam * 85 / 100;
    dam = dam + (getRealSpellDamage(ch) / 2);
    w_type = SKILL_FLAMESLASH;
  }
  // BACKSTAB GOES IN HERE!
  if ((type == SKILL_BACKSTAB || type == SKILL_CIRCLE) && dam < 10000)
  { // Bingo not affected.
    if (DC::isSet(ch->combat, COMBAT_CIRCLE))
    {
      if (ch->getLevel() <= MORTAL)
        if (type == SKILL_CIRCLE)
          dam = dam * 3 / 2; // non stabbing weapons
        else
          dam *= ((backstab_mult[(int)ch->getLevel()]) / 2);
      else
        dam *= 25;
      REMOVE_BIT(ch->combat, COMBAT_CIRCLE);
    }
    else if ((GET_CLASS(ch) == CLASS_THIEF) ||
             (GET_CLASS(ch) == CLASS_ANTI_PAL) || IS_NPC(ch))
    {
      if (ch->getLevel() <= MORTAL)
      {
        dam *= backstab_mult[(int)ch->getLevel()];
      }
      else
        dam *= 25;
    }
  }

  if (wielded && DC::isSet(wielded->obj_flags.extra_flags, ITEM_TWO_HANDED) && ch->has_skill(SKILL_EXECUTE))
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

  if (ch->has_skill(SKILL_NAT_SELECT) && affected_by_spell(ch, SKILL_NAT_SELECT))
    if (affected_by_spell(ch, SKILL_NAT_SELECT)->modifier == GET_RACE(vict))
      dam += 15 + ch->has_skill(SKILL_NAT_SELECT) / 10;

  do_combatmastery(ch, vict, weapon);
  if (DC::isSet(ch->combat, COMBAT_CRUSH_BLOW2))
    dam >>= 1; // dam = dam / 2;

  if (w_type == TYPE_HIT && IS_NPC(ch))
  {
    int a = mob_index[ch->mobdata->nr].virt;
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
    return debug_retval(ch, vict, retval) | eSUCCESS;
  }

  // Was last hit a success?
  if (DC::isSet(retval, eSUCCESS))
  {
    // If they're wielding a weapon check for weapon spells, otherwise check for hand spells
    if (wielded)
    {
      retval = weapon_spells(ch, vict, weapon);
      if (SOMEONE_DIED(retval) || !ch->fighting)
      {
        return debug_retval(ch, vict, retval) | eSUCCESS;
      }

      if (ch->equipment[weapon] && obj_index[ch->equipment[weapon]->item_number].combat_func)
      {
        retval = ((*obj_index[ch->equipment[weapon]->item_number].combat_func)(ch, ch->equipment[weapon], 0, "", ch));
        if (SOMEONE_DIED(retval) || !ch->fighting)
        {
          return debug_retval(ch, vict, retval) | eSUCCESS;
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
          return debug_retval(ch, vict, retval) | eSUCCESS;
        }
        if (obj_index[ch->equipment[WEAR_HANDS]->item_number].combat_func)
          retval = ((*obj_index[ch->equipment[WEAR_HANDS]->item_number].combat_func)(ch, ch->equipment[WEAR_HANDS], 0, "", ch));
        if (SOMEONE_DIED(retval) || !ch->fighting)
        {
          return debug_retval(ch, vict, retval) | eSUCCESS;
        }
      }

      if (ch->equipment[HOLD])
      {
        retval = weapon_spells(ch, vict, HOLD);
        if (SOMEONE_DIED(retval) || !ch->fighting)
        {
          return debug_retval(ch, vict, retval) | eSUCCESS;
        }

        if (ch->equipment[HOLD]->item_number >= 0)
        {
          if (obj_index[ch->equipment[HOLD]->item_number].combat_func != nullptr)
            retval = ((*obj_index[ch->equipment[HOLD]->item_number].combat_func)(ch, ch->equipment[HOLD], 0, "", ch));
          if (SOMEONE_DIED(retval) || !ch->fighting)
          {
            return debug_retval(ch, vict, retval) | eSUCCESS;
          }
        }
      }

      if (ch->equipment[HOLD2])
      {

        retval = weapon_spells(ch, vict, HOLD2);
        if (SOMEONE_DIED(retval) || !ch->fighting)
        {
          return debug_retval(ch, vict, retval) | eSUCCESS;
        }

        if (ch->equipment[HOLD2]->item_number >= 0)
        {
          if (obj_index[ch->equipment[HOLD2]->item_number].combat_func != nullptr)
            retval = ((*obj_index[ch->equipment[HOLD2]->item_number].combat_func)(ch, ch->equipment[HOLD2], 0, "", ch));
          if (SOMEONE_DIED(retval) || !ch->fighting)
          {
            return debug_retval(ch, vict, retval) | eSUCCESS;
          }
        }
      }
    }
  }

  if (act_poisonous(ch))
  {
    retval = cast_poison(ch->getLevel(), ch, "", SPELL_TYPE_SPELL, vict, nullptr, ch->getLevel());
    if (SOMEONE_DIED(retval))
      return debug_retval(ch, vict, retval) | eSUCCESS;
  }

  if (IS_MOB(ch) && ISSET(ch->mobdata->actflags, ACT_DRAINY))
  {
    if (number(1, 100) <= 10)
    {
      SET_BIT(retval, spell_energy_drain(1, ch, vict, 0, 0));
    }
  }

  return debug_retval(ch, vict, retval) | eSUCCESS;
} // one_hit

// pos of -1 means inventory
void eq_destroyed(Character *ch, Object *obj, int pos)
{
  if (DC::isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    return;
  if (DC::isSet(obj->obj_flags.more_flags, ITEM_NO_SCRAP))
    return;

  if (pos != -1) // if its not an inventory item, do this
  {
    unequip_char(ch, pos);

    if (pos == WEAR_LIGHT)
    {
      DC::getInstance()->world[ch->in_room].light--;
      ch->glow_factor--;
    }
    else if ((pos == WIELD) && (ch->equipment[SECOND_WIELD]))
    {
      Object *temp;
      temp = unequip_char(ch, SECOND_WIELD);
      equip_char(ch, temp, WIELD);
    }
    act("$p carried by $n is destroyed.", ch, obj, 0, TO_ROOM, 0);
  }
  else
  { // if its an inventory item, do this
    act("$p worn by $n is destroyed.", ch, obj, 0, TO_ROOM, 0);
    recheck_height_wears(ch); // Make sure $n can still wear the rest of
                              // the eq
  }

  act("Your $p has been destroyed.", ch, obj, 0, TO_CHAR, 0);

  while (obj->contains) // drop contents to floor
  {
    if (DC::isSet(obj->contains->obj_flags.more_flags, ITEM_NO_TRADE))
    {
      act("A $p falls to $n's inventory.", ch, obj->contains, 0, TO_ROOM, 0);
      act("A $p falls to your inventory from your destroyed container.", ch, obj->contains, 0, TO_CHAR, 0);
      move_obj(obj->contains, ch);
    }
    else
    {
      act("A $p falls to the ground from $n.", ch, obj->contains, 0, TO_ROOM, 0);
      act("A $p falls to the ground from your destroyed container.", ch, obj->contains, 0, TO_CHAR, 0);
      move_obj(obj->contains, ch->in_room); // this updates obj->contains
    }
  }

  act("$p falls to the ground in scraps.", ch, obj, 0, TO_CHAR, 0);
  act("$p falls to the ground in scraps.", ch, obj, 0, TO_ROOM, 0);
  make_scraps(ch, obj);
  extract_obj(obj);
}

void eq_damage(Character *ch, Character *victim,
               int dam, int weapon_type, int attacktype)
{
  int count, eqdam, chance, value;
  class Object *obj;
  int pos;

  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, ARENA)) // Don't damage eq in arena
    return;
  if (IS_PC(victim) && IS_PC(ch)) // Don't damage eq on pc->pc fights
    return;

  chance = 10 + GET_DEX(ch);
  if (dam < 40)         // Under 40 damage decreases chance of damage
    chance += 40 - dam; // Helps out the lower level chars

  if (number(0, chance)) // 1 in chance of eq damage
    return;

  //  let's scrap some worn eq
  if (number(0, 3) == 0)
  {
    // TODO - eventually we need to determine where someone gets hit and just pass the location

    // determine which location takes the damage giving a higher chance to certain locations
    pos = number(0, MAX_WEAR + 6);
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

    if (obj_index[obj->item_number].progtypes & ARMOUR_PROG)
      oprog_armour_trigger(victim, obj);
    // determine if time to scrap it
    if (eqdam >= eq_max_damage(obj))
      eq_destroyed(victim, obj, pos);
    else
    {
      act("$p is damaged.", victim, obj, 0, TO_CHAR, 0);
      act("$p worn by $n is damaged.", victim, obj, 0, TO_ROOM, 0);
    }
  } // number(0, 3) == 0
  else if (victim->carrying && (number(0, 7) == 0))
  {
    // let's scrap something in the inventory
    for (count = 0, obj = victim->carrying; obj; obj = obj->next_content)
      count++;
    value = number(1, count); // choose a random inventory item
    // loop up to that item
    for (count = 1, obj = victim->carrying; obj && count < value; obj = obj->next_content)
      count++;

    assert(obj);

    // add a point
    eqdam = damage_eq_once(obj);

    if (!obj)
      return;

    if (obj_index[obj->item_number].progtypes & ARMOUR_PROG)
      oprog_armour_trigger(victim, obj);

    // determine if time to scrap it
    if (eqdam >= eq_max_damage(obj))
      eq_destroyed(victim, obj, -1);
    else
    {
      act("$p is damaged.", victim, obj, 0, TO_CHAR, 0);
      act("$p carried by $n is damaged.", victim, obj, 0, TO_ROOM, 0);
    }
  } // end of inventory damage...

  save_char_obj(victim);
}

void pir_stat_loss(Character *victim, int chance, bool heh, bool zz)
{
  char log_buf[MAX_STRING_LENGTH] = {};
  if (victim->getLevel() < 50)
    return;
  chance /= 2;
  /* Pir's extra stat loss.  Bwahahah */
  if ((heh || (number(0, 99) < chance)) && IS_PC(victim))
  {
    switch (number(1, 5))
    {
    case 1:
      if (GET_STR(victim) > 4)
      {
        GET_STR(victim) -= 1;
        victim->raw_str -= 1;
        victim->sendln("*** You lose one strength point ***");
        sprintf(log_buf, "%s lost a str too. ouch.", victim->getNameC());
      }
      break;
    case 2:
      if (GET_WIS(victim) > 4)
      {
        GET_WIS(victim) -= 1;
        victim->raw_wis -= 1;
        victim->sendln("*** You lose one wisdom point ***");
        sprintf(log_buf, "%s lost a wis too. ouch.", victim->getNameC());
      }
      break;
    case 3:
      GET_CON(victim) -= 1;
      victim->raw_con -= 1;
      victim->sendln("*** You lose one constitution point ***");
      sprintf(log_buf, "%s lost a con too. ouch.", victim->getNameC());
      break;
    case 4:
      if (GET_INT(victim) > 4)
      {
        GET_INT(victim) -= 1;
        victim->raw_intel -= 1;
        victim->sendln("*** You lose one intelligence point ***");
        sprintf(log_buf, "%s lost a int too. ouch.", victim->getNameC());
      }
      break;
    case 5:
      if (GET_DEX(victim) > 4)
      {
        GET_DEX(victim) -= 1;
        victim->raw_dex -= 1;
        victim->sendln("*** You lose one dexterity point ***");
        sprintf(log_buf, "%s lost a dex too. ouch.", victim->getNameC());
      }
      break;
    } // of switch
    logentry(log_buf, SERAPH, LogChannels::LOG_MORTAL);
    victim->player->statmetas -= 1; // we lost a stat, so don't charge extra meta
  }                                 // of pir's extra stat loss
}

int damage_retval(Character *ch, Character *vict, int value)
{
  // we need to make sure in the case of a reflect or something
  // that we are returning the death of the CH if he died

  if (ch == vict && DC::isSet(value, eVICT_DIED))
    return (value | eCH_DIED);

  return value;
}

bool is_bingo(int dam, int weapon_type, int attacktype)
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

int getRealSpellDamage(Character *ch)
{
  int spell_dam;
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

int32_t Character::getHP(void)
{
  return hit;
}

void Character::setHP(int dam, Character *causer)
{
  int old_hp = hit;
  hit = dam;

  if (causer)
  {
    causer->damages++;
    causer->damage_done += old_hp - dam;
    causer->damage_per_second = causer->damage_done / causer->damages;
  }
}

void Character::removeHP(int dam, Character *causer)
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

void Character::addHP(int newhp, Character *causer)
{
  hit += newhp;
  if (hit > max_hit)
  {
    hit = max_hit;
  }
}

// returns standard returnvals.h return codes
int damage(Character *ch, Character *victim,
           int dam, int weapon_type, int attacktype, int weapon, bool is_death_prog)
{
  int can_miss = 1;
  int32_t weapon_bit;
  class Object *wielded;
  int typeofdamage;
  int damage_type(int weapon_type);
  int32_t get_weapon_bit(int weapon_type);
  int32_t hit_limit(Character * ch);
  int retval = 0;
  int modifier = 0;
  int percent;
  int learned;
  int ethereal = 0;
  bool reflected = false;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];

  bool bingo;
  if (is_bingo(dam, weapon_type, attacktype))
  {
    bingo = true;
  }
  else
  {
    bingo = false;
  }

  SET_BIT(retval, eSUCCESS);
  weapon_bit = get_weapon_bit(weapon_type);
  typeofdamage = damage_type(weapon_type);
  struct follow_type *fol;
  if (attacktype == SKILL_FLAMESLASH)
    weapon_bit = TYPE_FIRE;

  if (GET_POS(victim) == position_t::DEAD)
    return (eSUCCESS | eVICT_DIED);
  if (ch->in_room != victim->in_room && !(attacktype == SPELL_SOLAR_GATE ||
                                          attacktype == SKILL_ARCHERY || attacktype == SPELL_LIGHTNING_BOLT ||
                                          attacktype == SKILL_FIRE_ARROW || attacktype == SKILL_TEMPEST_ARROW ||
                                          attacktype == SKILL_GRANITE_ARROW || attacktype == SKILL_ICE_ARROW ||
                                          attacktype == SPELL_POISON))
    return debug_retval(ch, victim, retval);
  int l = 0;
  if (dam != 0 && attacktype && attacktype < TYPE_HIT && attacktype != TYPE_UNDEFINED && attacktype != SKILL_FLAMESLASH)
  { // Skill damages based on learned %
    l = ch->has_skill(attacktype);
    if (IS_NPC(ch))
      l = 50;
    if (IS_NPC(ch) && ch->master)
      l *= (ch->master->getLevel() / 50);
    //   if (l || IS_PC(ch))
    if (weapon && attacktype <= MAX_SPL_LIST)
    {
      l = 70;
      dam /= 2;
    } // weapon spell
    dam = dam_percent(l, dam);
    dam = number(dam - (dam / 10), dam + (dam / 10)); // +- 10%
    if (IS_NPC(ch))
      dam = (int)(dam * 0.6);
  }

  if (!weapon)
    weapon = WIELD;

  // here goes le elemental stuff

  extern int elemental_damage_bonus(int spell, Character *ch);

  if (attacktype && attacktype < MAX_SPL_LIST && attacktype != TYPE_UNDEFINED)
    dam += elemental_damage_bonus(attacktype, ch);
  //

  if (DC::isSet(victim->combat, COMBAT_REPELANCE) && !bingo &&
      attacktype <= MAX_SPL_LIST)
  {
    if (ch->getLevel() > 70)
      victim->sendln("The power of the spell bursts through your mental barriers as if they weren't there!");
    else if (!(number(0, 9)))
      victim->sendln("Your mental shields cannot hold back the force of the spell!");
    else
    {
      if (reflected)
      {
        act("You dissolve the reflected spell into nothingness by your will.", ch, 0, victim, TO_VICT, 0);
        act("$n's reacts quickly and dissolves the reflected spell into formless mana.", ch, 0, victim, TO_ROOM, NOTVICT);
      }
      else
      {
        act("$n's spell is dissolved into nothingness by your will.", ch, 0, victim, TO_VICT, 0);
        act("$N's supreme will dissolves your spell into formless mana.", ch, 0, victim, TO_CHAR, 0);
        act("$n's spell streaks at $N and suddenly ceases to be.", ch, 0, victim, TO_ROOM, NOTVICT);
      }
      REMOVE_BIT(victim->combat, COMBAT_REPELANCE);
      return debug_retval(ch, victim, retval);
    }
    REMOVE_BIT(victim->combat, COMBAT_REPELANCE);
    //  }
  }
  bool imm = false;
  if (DC::isSet(victim->immune, weapon_bit))
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
  int save = 0;
  if (!imm)
    switch (weapon_type)
    {
    case TYPE_FIRE:
      save = get_saves(victim, SAVE_TYPE_FIRE);
      sprintf(buf, "$B$4fire$R and sustain");
      if (learned)
        skill_increase_check(victim, SKILL_MAGIC_RESIST, learned, SKILL_INCREASE_MEDIUM);
      break;
    case TYPE_COLD:
      save = get_saves(victim, SAVE_TYPE_COLD);
      sprintf(buf, "$B$3cold$R and sustain");
      if (learned)
        skill_increase_check(victim, SKILL_MAGIC_RESIST, learned, SKILL_INCREASE_MEDIUM);
      break;
    case TYPE_ENERGY:
      save = get_saves(victim, SAVE_TYPE_ENERGY);
      sprintf(buf, "$B$5energy$R and sustain");
      if (learned)
        skill_increase_check(victim, SKILL_MAGIC_RESIST, learned, SKILL_INCREASE_MEDIUM);
      break;
    case TYPE_ACID:
      save = get_saves(victim, SAVE_TYPE_ACID);
      sprintf(buf, "$B$2acid$R and sustain");
      if (learned)
        skill_increase_check(victim, SKILL_MAGIC_RESIST, learned, SKILL_INCREASE_MEDIUM);
      break;
    case TYPE_MAGIC:
      save = get_saves(victim, SAVE_TYPE_MAGIC);
      sprintf(buf, "$B$7magic$R and sustain");
      break;
    case TYPE_POISON:
      save = get_saves(victim, SAVE_TYPE_POISON);
      sprintf(buf, "$2poison$R and sustain");
      if (learned)
        skill_increase_check(victim, SKILL_MAGIC_RESIST, learned, SKILL_INCREASE_HARD);
      break;
    default:
      break;
    }
  /* int v = 0;
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
    dam = (int)(dam * mult);
    if (reflected)
    {
      strcpy(buf3, buf);
      sprintf(buf2, "s additional damage.");
      strcat(buf, buf2);
      sprintf(buf2, "%s is susceptible to the reflected ", GET_SHORT(ch));
      strcat(buf2, buf);
      act(buf2, ch, 0, victim, TO_ROOM, NOTVICT);
      sprintf(buf2, " additional damage.");
      strcat(buf3, buf2);
      sprintf(buf2, "You are susceptible to the reflected ");
      strcat(buf2, buf3);
      act(buf2, ch, 0, victim, TO_CHAR, 0);
    }
    else
    {
      strcpy(buf3, buf);
      sprintf(buf2, "s additional damage.");
      strcat(buf, buf2);
      sprintf(buf2, " additional damage.");
      strcat(buf3, buf2);
      sprintf(buf2, "%s is susceptible to %s's ", GET_SHORT(victim), GET_SHORT(ch));
      strcat(buf2, buf);
      act(buf2, victim, 0, ch, TO_ROOM, NOTVICT);
      sprintf(buf2, "%s is susceptible to your ", GET_SHORT(victim));
      strcat(buf2, buf);
      act(buf2, victim, 0, ch, TO_VICT, 0);
      sprintf(buf2, "You are susceptible to %s's ", GET_SHORT(ch));
      strcat(buf2, buf3);
      act(buf2, victim, 0, ch, TO_CHAR, 0);
    }
  }
  else if (number(1, 101) < save && !imm)
  {
    if (save > 50)
      save = 50;
    dam -= (int)(dam * (double)save / 100); // Save chance.
    if (reflected)
    {
      strcpy(buf3, buf);
      sprintf(buf2, "s reduced damage.");
      strcat(buf, buf2);
      sprintf(buf2, "%s resists the reflected ", GET_SHORT(ch));
      strcat(buf2, buf);
      act(buf2, ch, 0, victim, TO_ROOM, NOTVICT);
      sprintf(buf2, " reduced damage.");
      strcat(buf3, buf2);
      sprintf(buf2, "You resist the reflected ");
      strcat(buf2, buf3);
      act(buf2, ch, 0, victim, TO_CHAR, 0);
    }
    else
    {
      strcpy(buf3, buf);
      sprintf(buf2, "s reduced damage.");
      strcat(buf, buf2);
      sprintf(buf2, " reduced damage.");
      strcat(buf3, buf2);
      sprintf(buf2, "%s resists %s's ", GET_SHORT(victim), GET_SHORT(ch));
      strcat(buf2, buf);
      act(buf2, victim, 0, ch, TO_ROOM, NOTVICT);
      sprintf(buf2, "%s resists your ", GET_SHORT(victim));
      strcat(buf2, buf);
      act(buf2, victim, 0, ch, TO_VICT, 0);
      sprintf(buf2, "You resist %s's ", GET_SHORT(ch));
      strcat(buf2, buf3);
      act(buf2, victim, 0, ch, TO_CHAR, 0);
    }
    //        act("$n resists $N's assault and sustains reduced damage.", victim, 0, ch, TO_ROOM, NOTVICT);
    //        act("$n resists your assault and sustains reduced damage.", victim,0,ch, TO_VICT,0);
    //        act("You resist $N's assault and sustain reduced damage.", victim, 0, ch, TO_CHAR, 0);
  }
  /*
  if (v) { // spellcraft damage bonus
  int o = 0;
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

  if (affected_by_spell(ch, SKILL_SONG_MKING_CHARGE))
  {
    dam = (int)(dam * 1.2); // scary!
    SET_BIT(modifier, COMBAT_MOD_FRENZY);
  }

  // Can't hurt god, but he likes to see the messages.
  if (victim->getLevel() >= IMMORTAL && IS_PC(victim))
    dam = 0;

  if (victim != ch)
  {
    if (!can_attack(ch) || !can_be_attacked(ch, victim))
      return eFAILURE;
    set_cantquit(ch, victim);
  }

  /* An eye for an eye, a tooth for a tooth, a life for a life. */
  if (GET_POS(victim) > position_t::STUNNED && ch != victim)
  {
    if (!victim->fighting && ch->in_room == victim->in_room)
      set_fighting(victim, ch);

    if ((!DC::isSet(victim->combat, COMBAT_STUNNED)) &&
        (!DC::isSet(victim->combat, COMBAT_STUNNED2)) &&
        (!DC::isSet(victim->combat, COMBAT_BASH1)) &&
        (!DC::isSet(victim->combat, COMBAT_BASH2)))
    {
      if (GET_POS(victim) > position_t::STUNNED)
      {
        if (GET_POS(victim) < position_t::FIGHTING)
        {
          act("$n scrambles to $s feet!", victim, 0, 0, TO_ROOM, 0);
          act("You scramble to your feet!", victim, 0, 0, TO_CHAR, 0);
          victim->setPOSFighting();
        }
      }
    }
  }
  else if (GET_POS(victim) == position_t::SLEEPING)
  {
    affect_from_char(victim, INTERNAL_SLEEPING);
    act("$n is shocked to a wakened state and scrambles to $s feet!", victim, 0, 0, TO_ROOM, 0);
    send_to_char("You are awakened from combat adrenaline springing you to your feet!", ch);
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

  if (IS_AFFECTED(ch, AFF_INVISIBLE) && (!IS_AFFECTED(ch, AFF_ILLUSION) || !(affected_by_spell(ch,
                                                                                               BASE_TIMERS + SPELL_INVISIBLE) &&
                                                                             affected_by_spell(ch, SPELL_INVISIBLE) && affected_by_spell(ch, SPELL_INVISIBLE)->modifier == 987)))
  {
    act("$n slowly fades into existence.", ch, 0, 0, TO_ROOM, 0);
    // if (affected_by_spell(ch, SPELL_INVISIBLE))
    //  no point it looping through the list twice...
    affect_from_char(ch, SPELL_INVISIBLE);
    REMBIT(ch->affected_by, AFF_INVISIBLE);
  }

  // Frost Shield won't effect a backstab
  if (attacktype != SKILL_BACKSTAB && victim->getHP() > 0 &&
      (typeofdamage == DAMAGE_TYPE_PHYSICAL || attacktype == TYPE_PHYSICAL_MAGIC))
    if (do_frostshield(ch, victim))
    {
      return debug_retval(ch, victim, retval) | eEXTRA_VALUE;
      ;
    }

  if (typeofdamage == DAMAGE_TYPE_PHYSICAL)
  {
    if (DC::isSet(ch->combat, COMBAT_BERSERK))
      dam = (int)(dam * 1.75);
    if (IS_AFFECTED(ch, AFF_PRIMAL_FURY))
      dam = dam * 5;
    if (DC::isSet(ch->combat, COMBAT_RAGE1) || (DC::isSet(ch->combat, COMBAT_RAGE2) && attacktype != SKILL_BACKSTAB))
      dam = (int)(dam * 1.4);
    if (DC::isSet(ch->combat, COMBAT_HITALL))
      dam = (int)(dam * 2);
    if (DC::isSet(ch->combat, COMBAT_ORC_BLOODLUST1))
    {
      dam = (int)(dam * 1.7);
      //      REMOVE_BIT(ch->combat, COMBAT_ORC_BLOODLUST1);
      //      SET_BIT(ch->combat, COMBAT_ORC_BLOODLUST2);
    }
    if (DC::isSet(ch->combat, COMBAT_ORC_BLOODLUST2))
    {
      dam = (int)(dam * 1.7);
      //      REMOVE_BIT(ch->combat, COMBAT_ORC_BLOODLUST2);
    }
    percent = (int)((((float)ch->getHP()) / ((float)GET_MAX_HIT(ch))) * 100);
    if (percent < 40 && (learned = ch->has_skill(SKILL_FRENZY)))
    {
      if (skill_success(ch, victim, SKILL_FRENZY))
      {
        dam = (int)(dam * 1.2);
        SET_BIT(modifier, COMBAT_MOD_FRENZY);
      }
    }
    if (DC::isSet(ch->combat, COMBAT_VITAL_STRIKE))
      dam = (int)(dam * 2.5);

    // we do this AFTER all the multipliers but BEFORE all the reducers
    // to make it cause the smallest impact
    if (dam) // misses turned to tickles
      dam = (dam * (100 - victim->melee_mitigation)) / 100;

    if (affected_by_spell(victim, SPELL_HOLY_AURA) && affected_by_spell(victim, SPELL_HOLY_AURA)->modifier == 50)
      dam /= 2; // half damage against physical attacks
  }
  else
  {
    if (affected_by_spell(victim, SPELL_HOLY_AURA) && affected_by_spell(victim, SPELL_HOLY_AURA)->modifier == 25)
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
    int mod = affected_by_spell(victim, SPELL_SANCTUARY) ? affected_by_spell(victim, SPELL_SANCTUARY)->modifier : 35;
    dam -= (int)(float)((float)dam * ((float)mod / 100.0));
  }
  if (DC::isSet(victim->combat, COMBAT_MONK_STANCE) && dam > 0) // half damage
    dam /= 2;
  int reduce = 0, type = 0;
  if (can_miss == 1)
  {
    if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) || attacktype == SKILL_FLAMESLASH)
    {
      int retval2 = 0;

      retval2 = isHit(ch, victim, attacktype, type, reduce);
      if (SOMEONE_DIED(retval2)) // Riposte
        return damage_retval(ch, victim, retval2);

      if (DC::isSet(retval2, eSUCCESS))
      {
        switch (type)
        {
        case 0:
          return eFAILURE; // Dodge or parry

        case 4:
        case 1:
        case 2:
          SET_BIT(modifier, COMBAT_MOD_REDUCED);
          dam -= (int)((float)dam * ((float)reduce / 100));
          break;
          // Shield block or Mdefense or tumbling partial avoid

        case 3:
          dam = 0; // Miss!
          break;
        default:
          return eFAILURE; // Shouldn't happen
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
      return eFAILURE;

    if((reduce = check_shieldblock(ch, victim, attacktype)))
     {
  SET_BIT(modifier, COMBAT_MOD_REDUCED);
  dam -= (int)((float) dam * ((float)reduce/100));
     }
   */

      if ((learned = ch->has_skill(SKILL_CRIT_HIT)) && dam)
        if (number(1, 101) <= learned / 10 + GET_DEX(ch) - GET_DEX(victim))
        {
          dam += (int)(dam * (float)(2 + learned / 5) / 100);
          act("Your strike at $N lands with lethal accuracy and inflicts additional damage!", ch, 0, victim, TO_CHAR, 0);
          act("$n's strike lands with lethal accuracy and inflicts additional damage!", ch, 0, victim, TO_VICT, 0);
          act("$n's strike at $N lands with lethal accuracy and inflicts additional damage!", ch, 0, victim, TO_ROOM, NOTVICT);
          skill_increase_check(ch, SKILL_CRIT_HIT, learned, SKILL_INCREASE_HARD);
        }
    }
    /* Never heard of it.
  if (attacktype == TYPE_PHYSICAL_MAGIC)
  {
    // Physical Magic can be dodged or blocked with a shield, but not parried
    if(check_shieldblock(ch, victim,attacktype))
      return eFAILURE;
    if (check_dodge(ch, victim,attacktype))
      return eFAILURE;
  }*/
    if (attacktype <= MAX_SPL_LIST && attacktype != TYPE_UNDEFINED)
    {
      int reduce = 0;
      if ((reduce = check_magic_block(ch, victim, attacktype)))
      {
        if (GET_CLASS(victim) != CLASS_MONK && IS_PC(victim))
        {
          if (number(1, 100) <= MAX(1, dam / 150))
          {
            int eqdam = damage_eq_once(victim->equipment[WEAR_SHIELD]);
            if (victim->equipment[WEAR_SHIELD])
            {
              if (obj_index[victim->equipment[WEAR_SHIELD]->item_number].progtypes & ARMOUR_PROG)
                oprog_armour_trigger(victim, victim->equipment[WEAR_SHIELD]);
              if (eqdam >= eq_max_damage(victim->equipment[WEAR_SHIELD]))
                eq_destroyed(victim, victim->equipment[WEAR_SHIELD], WEAR_SHIELD);
              else
              {
                act("$p is damaged by the force of the spell!", victim, victim->equipment[WEAR_SHIELD], 0, TO_CHAR, 0);
                act("$p worn by $n is damaged by the force of the spell!", victim, victim->equipment[WEAR_SHIELD], 0, TO_ROOM, 0);
              }
            }
          }
        }
        dam -= (int)((float)dam * ((float)reduce / 100));
      }
    } // spellblock

  } // can_miss

  int pre_stoneshield_dam = 0;
  std::stringstream string1;
  struct affected_type *pspell = nullptr;
  if (victim->getLevel() < IMMORTAL && dam > 0 && typeofdamage == DAMAGE_TYPE_PHYSICAL &&
      ((pspell = affected_by_spell(victim, SPELL_STONE_SHIELD)) ||
       (pspell = affected_by_spell(victim, SPELL_GREATER_STONE_SHIELD))))
  {
    pre_stoneshield_dam = dam;
    if (dam > pspell->modifier)
    {
      dam -= pspell->modifier;
      pspell->duration -= pspell->modifier;
      csendf(victim, "Your stones absorb %d damage allowing %d through.\r\n", pspell->modifier, dam);
      string1 << "Your attack hits $N's stones for " << pspell->modifier << " damage allowing " << dam << " through.";
      act(string1.str().c_str(), ch, 0, victim, TO_CHAR, 0);
      string1.clear();
      string1.str("");
      string1 << "$n's attack hits $N's stones for " << pspell->modifier << " damage allowing " << dam << " through.";
      act(string1.str().c_str(), ch, 0, victim, TO_ROOM, NOTVICT);
    }
    else
    {
      pspell->duration -= dam;
      victim->send(QString("Your stones absorb %1 damage from the attack and change its direction slightly.\r\n").arg(dam));
      string1 << "$N's stones absorb " << dam << " damage of your attack and cause your blow to change direction slightly.";
      act(string1.str().c_str(), ch, 0, victim, TO_CHAR, 0);
      string1.clear();
      string1.str("");
      string1 << "$N's stones completely absorbed $n's attack of " << dam << " damage changing its direction slightly.";
      act(string1.str().c_str(), ch, 0, victim, TO_ROOM, NOTVICT);
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
    dam = 0;

  // Immune / Susceptibilities / Resistances
  // Some code is in saves_spell  (for obvious reasons)
  weapon_bit = get_weapon_bit(weapon_type);

  if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) ||
      (attacktype == SKILL_BACKSTAB))
  {
    if (DC::isSet(victim->immune, ISR_PHYSICAL))
      weapon_bit += ISR_PHYSICAL;
    else if (DC::isSet(victim->resist, ISR_PHYSICAL))
      weapon_bit += ISR_PHYSICAL;
    else if (DC::isSet(victim->suscept, ISR_PHYSICAL))
      weapon_bit += ISR_PHYSICAL;

    wielded = ch->equipment[weapon];

    if (wielded)
    {
      //      if ((DC::isSet(victim->immune, ISR_MAGIC)) &&
      //      (DC::isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
      //    weapon_bit += ISR_MAGIC;
      if ((DC::isSet(victim->immune, ISR_NON_MAGIC)) &&
          (!DC::isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)))
        weapon_bit += ISR_NON_MAGIC;
      //      if ((DC::isSet(victim->suscept, ISR_MAGIC)) &&
      //        (DC::isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
      //        weapon_bit += ISR_MAGIC;
      if ((DC::isSet(victim->suscept, ISR_NON_MAGIC)) &&
          (!DC::isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)))
        weapon_bit += ISR_NON_MAGIC;
      //      if ((DC::isSet(victim->resist, ISR_MAGIC)) &&
      //       (DC::isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
      //       weapon_bit += ISR_MAGIC;
      if ((DC::isSet(victim->resist, ISR_NON_MAGIC)) &&
          (!DC::isSet(wielded->obj_flags.extra_flags, ITEM_MAGIC)))
        weapon_bit += ISR_NON_MAGIC;
    }
  }

  if (DC::isSet(victim->immune, weapon_bit))
  {
    dam = 0;
    SET_BIT(retval, eIMMUNE_VICTIM);
    if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) || attacktype == SKILL_FLAMESLASH)
    {
      SET_BIT(modifier, COMBAT_MOD_IGNORE);
      SET_BIT(retval, eEXTRA_VALUE);
    }
  }
  else if (DC::isSet(victim->suscept, weapon_bit))
  {
    //    magic stuff is handled elsewhere
    if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) || attacktype == SKILL_FLAMESLASH)
    {
      dam = (int)(dam * 1.3);
      SET_BIT(modifier, COMBAT_MOD_SUSCEPT);
    }
  }
  else if (DC::isSet(victim->resist, weapon_bit))
  {
    //    magic stuff is handled elsewhere
    if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) || attacktype == SKILL_FLAMESLASH)
    {
      dam = (int)(dam * 0.7);
      SET_BIT(modifier, COMBAT_MOD_RESIST);
    }
  }
  if (dam < 0)
    dam = 0;

  percent = (int)((((float)victim->getHP()) /
                   ((float)GET_MAX_HIT(victim))) *
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
      if (number(1, 100) <= (learned / 2))
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

        dam = (int)(dam * (double)(number(45, 55) / 100.0));
      }
      break;
    }
  }

  if (dam > 0 && affected_by_spell(victim, SPELL_DIVINE_INTER) && dam > affected_by_spell(victim, SPELL_DIVINE_INTER)->modifier)
    dam = affected_by_spell(victim, SPELL_DIVINE_INTER)->modifier;

  // Check for parry, mob disarm, and trip. Print a suitable damage message.
  if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) || (IS_NPC(ch) && mob_index[ch->mobdata->nr].virt > 87 && mob_index[ch->mobdata->nr].virt < 92) || attacktype == SKILL_FLAMESLASH)
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
    if (dam >= 350 && (af = affected_by_spell(victim, SPELL_PARALYZE)) && IS_PC(victim))
    {
      act("The overpowering magic from $n's spell disrupts the paralysis surrounding you!", ch, 0, victim, TO_VICT, 0);
      act("The powerful magic from your spell has disrupted the paralysis surrounding $N!", ch, 0, victim, TO_CHAR, 0);
      act("The powerful magic of $n's spell has disrupted the paralysis surrounding $N!", ch, 0, victim, TO_ROOM, NOTVICT);
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
    act("The ethereal stones surrounding $n shatter into nothingness.\r\n", victim, 0, 0, TO_ROOM, 0);
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
    int retval2 = check_riposte(ch, victim, attacktype);
    if (SOMEONE_DIED(retval2))
      return damage_retval(ch, victim, retval2);
  }

  if (typeofdamage == DAMAGE_TYPE_PHYSICAL && type == 2 && reduce > 0 && dam > 0 && ch != victim)
  { // Martial Defense Counter Strike..
    int retval2 = checkCounterStrike(ch, victim);
    if (SOMEONE_DIED(retval2))
      return damage_retval(ch, victim, retval2);
  }

  if (typeofdamage == DAMAGE_TYPE_PHYSICAL && dam > 0 && ch != victim && attacktype != SKILL_ARCHERY && !is_death_prog)
  {
    int retval2;
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
  if (!DC::isSet(victim->combat, COMBAT_STUNNED) &&
      !DC::isSet(victim->combat, COMBAT_STUNNED2))
    if (!AWAKE(victim))
    {
      if (victim->fighting)
        stop_fighting(victim);

      if (DC::isSet(victim->combat, COMBAT_BERSERK))
        REMOVE_BIT(victim->combat, COMBAT_BERSERK);
      if (DC::isSet(victim->combat, COMBAT_RAGE1))
        REMOVE_BIT(victim->combat, COMBAT_RAGE1);
      if (DC::isSet(victim->combat, COMBAT_RAGE2))
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
    return damage_retval(ch, victim, (eSUCCESS | eVICT_DIED));
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
      if (IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_CHARMIEJOIN) && attacktype != SKILL_AMBUSH)
      {
        if (ch->followers)
        {
          struct follow_type *folnext;
          for (fol = ch->followers; fol; fol = folnext)
          {
            folnext = fol->next;
            if (IS_AFFECTED(fol->follower, AFF_CHARM) && ch->in_room == fol->follower->in_room)
              SET_BIT(retval, check_charmiejoin(fol->follower));
            if (DC::isSet(retval, eVICT_DIED))
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
int noncombat_damage(Character *ch, int dam, char *char_death_msg,
                     char *room_death_msg, char *death_log_msg, int type)
{
  int kill_type = TYPE_CHOOSE;

  if (IS_AFFECTED(ch, AFF_EAS))
    dam /= 4;
  if (IS_AFFECTED(ch, AFF_SANCTUARY))
  {
    int mod = affected_by_spell(ch, SPELL_SANCTUARY) ? affected_by_spell(ch, SPELL_SANCTUARY)->modifier : 35;
    dam -= (int)(float)((float)dam * ((float)mod / 100.0));
  }
  if (affected_by_spell(ch, SPELL_HOLY_AURA) && affected_by_spell(ch, SPELL_HOLY_AURA)->modifier == 50 && (type == KILL_DROWN || type == KILL_FALL))
    dam /= 2;
  if (affected_by_spell(ch, SPELL_HOLY_AURA) && affected_by_spell(ch, SPELL_HOLY_AURA)->modifier == 25 && type == KILL_POISON)
    dam /= 2;
  if (affected_by_spell(ch, SPELL_DIVINE_INTER) && dam > affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier)
    dam = affected_by_spell(ch, SPELL_DIVINE_INTER)->modifier;

  ch->removeHP(dam);
  update_pos(ch);
  if (GET_POS(ch) == position_t::DEAD)
  {
    if (char_death_msg)
    {
      ch->send(char_death_msg);
      send_to_char("\n\rYou have been KILLED!\n\r", ch);
    }
    if (room_death_msg)
      act(room_death_msg, ch, 0, 0, TO_ROOM, 0);
    if (death_log_msg)
      logentry(death_log_msg, IMMORTAL, LogChannels::LOG_MORTAL);
    if (type == KILL_BATTER)
      kill_type = TYPE_PKILL;
    fight_kill(nullptr, ch, kill_type, type);
    return eSUCCESS | eCH_DIED;
  }
  else
  {
    return eSUCCESS;
  }
}

int is_pkill(Character *ch, Character *vict)
{
  Character *tmp_ch;

  // TODO - change this so a mob following another mob isn't a pkill

  if (!ch)
    return true;

  for (tmp_ch = ch; tmp_ch; tmp_ch = tmp_ch->master)
  {
    if (IS_PC(tmp_ch))
    {
      if (IS_NPC(vict))
      {
        if (vict->master)
        { /* Attacking someone's charmie */
          if (IS_PC(vict->master))
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

void send_damage(char const *buf, Character *ch, Object *obj, Character *victim, char const *dmg, char const *buf2, int to)
{
  Character *tmpch;
  char string1[MAX_INPUT_LENGTH], string2[MAX_INPUT_LENGTH];

  int i, z = 0, y = 0;
  for (i = 0; i <= (int)strlen(buf); i++)
  {
    if (*(buf + i) == '|')
    {
      string1[z] = '\0';
      strcat(string1, dmg);
      z += strlen(dmg);
    }
    else
    {
      string1[z++] = *(buf + i);
      string2[y++] = *(buf + i);
    }
  }
  if (buf2) // lazy, should've done it earlier, some extra processing wasted, but I don't care!
    strcpy(string2, buf2);

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
    for (tmpch = DC::getInstance()->world[ch->in_room].people; tmpch; tmpch = tmpch->next_in_room)
    {
      if (tmpch == ch || tmpch == victim)
        continue;
      if (IS_PC(tmpch) && DC::isSet(tmpch->player->toggles, Player::PLR_DAMAGE))
        send_tokens(tokens, ch, obj, victim, 0, tmpch);
      else
        send_tokens(tokens2, ch, obj, victim, 0, tmpch);
    }
  }
  else if (to == TO_CHAR)
  {
    if (IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_DAMAGE))
      send_tokens(tokens, ch, obj, victim, 0, ch);
    else
      send_tokens(tokens2, ch, obj, victim, 0, ch);
  }
  else if (to == TO_VICT)
  {
    if (IS_PC(victim) && DC::isSet(victim->player->toggles, Player::PLR_DAMAGE))
      send_tokens(tokens, ch, obj, victim, 0, victim);
    else
      send_tokens(tokens2, ch, obj, victim, 0, victim);
  }
  delete tokens;
  delete tokens2;
}

void do_dam_msgs(Character *ch, Character *victim, int dam, int attacktype, int weapon, int filter)
{
  extern struct message_list fight_messages[MAX_MESSAGES];
  struct message_type *messages, *messages2;
  int i, j, nr;
  std::string find, replace;

  if (is_bingo(dam, weapon, attacktype))
    return;

  if (filter > TYPE_HIT && (attacktype == SPELL_BURNING_HANDS || attacktype == SPELL_FIREBALL || attacktype == SPELL_FIRESTORM || attacktype == SPELL_HELLSTREAM || attacktype == SPELL_MAGIC_MISSILE || attacktype == SPELL_METEOR_SWARM || attacktype == SPELL_LIGHTNING_BOLT || attacktype == SPELL_CHILL_TOUCH))
  {
    if (attacktype == SPELL_CHILL_TOUCH)
      find = "$B$3";
    else if (attacktype == SPELL_LIGHTNING_BOLT)
      find = "$B$5";
    else if (attacktype == SPELL_MAGIC_MISSILE || attacktype == SPELL_METEOR_SWARM)
      find = "$B$7";
    else
      find = "$B$4";

    switch (filter)
    {
    case TYPE_FIRE:
      replace = "$B$4";
      break;
    case TYPE_COLD:
      replace = "$B$3";
      break;
    case TYPE_ENERGY:
      replace = "$B$5";
      break;
    case TYPE_ACID:
      replace = "$B$2";
      break;
    case TYPE_POISON:
      replace = "$2";
      break;
    case TYPE_MAGIC:
      replace = "$B$7";
      break;
    default:
      replace = find;
      break;
    }
  }

  for (i = 0; i < MAX_MESSAGES; i++)
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
    char dmgmsg[MAX_INPUT_LENGTH];
    dmgmsg[0] = '\0';
    if (dam > 0)
      sprintf(dmgmsg, "$B%d$R", dam);
    if (!messages)
      return;
    if (IS_PC(victim) && victim->getLevel() >= IMMORTAL)
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
      send_damage(replaceString(messages2->die_msg.victim_msg, find, replace).c_str(), ch, ch->equipment[weapon],
                  victim, dmgmsg, replaceString(messages->die_msg.victim_msg, find, replace).c_str(), TO_VICT);
      send_damage(replaceString(messages2->die_msg.attacker_msg, find, replace).c_str(), ch, ch->equipment[weapon],
                  victim, dmgmsg, replaceString(messages->die_msg.attacker_msg, find, replace).c_str(), TO_CHAR);
      send_damage(replaceString(messages2->die_msg.room_msg, find, replace).c_str(), ch, ch->equipment[weapon],
                  victim, dmgmsg, replaceString(messages->die_msg.room_msg, find, replace).c_str(), TO_ROOM);
    }
    else
    {
      send_damage(replaceString(messages2->hit_msg.victim_msg, find, replace).c_str(), ch, ch->equipment[weapon],
                  victim, dmgmsg, replaceString(messages->hit_msg.victim_msg, find, replace).c_str(), TO_VICT);
      send_damage(replaceString(messages2->hit_msg.attacker_msg, find, replace).c_str(), ch, ch->equipment[weapon],
                  victim, dmgmsg, replaceString(messages->hit_msg.attacker_msg, find, replace).c_str(), TO_CHAR);
      send_damage(replaceString(messages2->hit_msg.room_msg, find, replace).c_str(), ch, ch->equipment[weapon],
                  victim, dmgmsg, replaceString(messages->hit_msg.room_msg, find, replace).c_str(), TO_ROOM);
    }
  }
}

void set_cantquit(Character *ch, Character *vict, bool forced)
{
  struct affected_type af, *paf;
  Character *realch;
  Character *realvict;
  int ch_vnum = -1;
  int vict_vnum = -1;

  if (!ch || !vict)
    return; /* This will happen if the char was in a fall room */

  if (ch == vict)
    return;

  /* can't get pkill flag in the arena! */
  if (IS_ARENA(ch->in_room))
    return;

  if (IS_NPC(ch))
    ch_vnum = mob_index[ch->mobdata->nr].virt;

  if (IS_NPC(vict))
    vict_vnum = mob_index[vict->mobdata->nr].virt;

  if (IS_NPC(ch) && (IS_AFFECTED(ch, AFF_CHARM) || IS_AFFECTED(ch, AFF_FAMILIAR) || ch_vnum == 8) && ch->master && ch->master->in_room == ch->in_room)
    realch = ch->master;
  else
    realch = ch;

  if (IS_NPC(vict) && (IS_AFFECTED(vict, AFF_CHARM) || IS_AFFECTED(vict, AFF_FAMILIAR) || vict_vnum == 8) && vict->master)
    realvict = vict->master;
  else
    realvict = vict;

  if (realvict == realch) // killing your own pet was giving you a CQ
    return;

  if (!realch->fighting)
    SET_BIT(realch->combat, COMBAT_ATTACKER);

  if (is_pkill(realch, realvict) && !ISSET(realvict->affected_by, AFF_CANTQUIT) &&
      !affected_by_spell(realvict, FUCK_GTHIEF) && !IS_AFFECTED(realvict, AFF_CHAMPION) && !IS_AFFECTED(realch, AFF_CHAMPION) &&
      !affected_by_spell(realvict, FUCK_PTHIEF) && !forced)
  {
    af.type = FUCK_CANTQUIT;
    af.duration = 5;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_CANTQUIT;
    if (realvict && GET_NAME(realvict))
    {
      af.caster = GET_NAME(realvict);
    }

    if (!ISSET(realch->affected_by, AFF_CANTQUIT))
      affect_to_char(realch, &af);
    else
    {
      for (paf = realch->affected; paf; paf = paf->next)
      {
        if (paf->type == FUCK_CANTQUIT)
          paf->duration = 5;
      }
    }
  }
}

void fight_kill(Character *ch, Character *vict, int type, int spec_type)
{
  if (!vict)
  {
    logentry("Null vict sent to fight_kill()!", -1, LogChannels::LOG_BUG);
    return;
  }
  bool vict_is_attacker = false;
  if (DC::isSet(vict->combat, COMBAT_ATTACKER))
    vict_is_attacker = true;

  if (vict->fighting)
    stop_fighting(vict);
  if (ch && ch->fighting && (IS_PC(ch) || ch->fighting == vict))
    stop_fighting(ch);

  // loop through world and stop anyone else that was fighting vict from fighting
  Character *ich, *next_ich;
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
    if (IS_NPC(vict) || (spec_type == KILL_POISON && affected_by_spell(vict, SPELL_POISON)->modifier == -123))
      raw_kill(ch, vict);
    else if (IS_ARENA(vict->in_room))
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

QString translate_name(const Character *ch)
{
  if (IS_NPC(ch))
  {
    return QString("%1(v%2)").arg(GET_NAME(ch)).arg(mob_index[ch->mobdata->nr].virt);
  }
  return GET_NAME(ch);
}

void debug_isHit(const Character *ch, const Character *victim, const int &attacktype, const int &type, const int &reduce, const int &tohit, const QString message = QString())
{
  if (ch->getDebug() || victim->getDebug())
  {
    logentry(QString("isHit: %1 vs %2 attacktype=%3 type=%4 reduce=%5 tohit=%6 %7").arg(translate_name(ch)).arg(translate_name(victim)).arg(attacktype).arg(type).arg(reduce).arg(tohit).arg(message), DIVINITY, LOG_BUG);
  }
}

void debug_isHit_toHit(const Character *ch, const Character *victim, const int &toHit)
{
  if (ch->getDebug() || victim->getDebug())
  {
    logentry(QString("isHit: toHit=%1").arg(toHit), DIVINITY, LOG_BUG);
  }
}

void debug_isHit_generic(const Character *ch, const Character *victim, const auto &parry, const auto &dodge, const auto &block, const auto &martial, const auto &tumbling)
{
  if (ch->getDebug() || victim->getDebug())
  {
    logentry(QString("parry=%1 dodge=%2 block=%3 martial=%4 tumbling=%5").arg(parry).arg(dodge).arg(block).arg(martial).arg(tumbling), DIVINITY, LOG_BUG);
  }
}

// New toHit code
int isHit(Character *ch, Character *victim, int attacktype, int &type, int &reduce)
{
  if ((DC::isSet(victim->combat, COMBAT_STUNNED)) ||
      (DC::isSet(victim->combat, COMBAT_STUNNED2)) ||
      (DC::isSet(victim->combat, COMBAT_BASH1)) ||
      (DC::isSet(victim->combat, COMBAT_BASH2)) ||
      (DC::isSet(victim->combat, COMBAT_SHOCKED)) ||
      (DC::isSet(victim->combat, COMBAT_SHOCKED2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)) ||
      (!AWAKE(victim)))
  {
    debug_isHit(ch, victim, attacktype, type, reduce, 0, "stunned,bash,shocked return eFAILURE");
    return eFAILURE; // always hit
  }

  level_diff_t level_difference = ch->getLevel() - victim->getLevel();
  int skill = 0;

  // Figure out toHit value.
  int toHit = GET_REAL_HITROLL(ch);
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

  if (DC::isSet(ch->combat, COMBAT_BERSERK) || IS_AFFECTED(ch, AFF_PRIMAL_FURY))
  {
    toHit = (int)((float)toHit * 0.90) - 5;
    debug_isHit_toHit(ch, victim, toHit);
  }
  else if (DC::isSet(ch->combat, COMBAT_RAGE1) || DC::isSet(ch->combat, COMBAT_RAGE2))
  {
    toHit = (int)((float)toHit * 0.95) - 2;
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

  float lowlvlmod = (50.0 - ch->getLevel() - (victim->getLevel() / 2.0)) / 10.0;
  if (lowlvlmod > 1.0)
  {
    toHit = (int)((float)toHit * lowlvlmod);
    debug_isHit_toHit(ch, victim, toHit);
  }

  // The stuff.
  float num1 = 1.0 - (-300.0 - (float)GET_AC(victim)) * 4.761904762 * 0.0001;
  float num2 = 20.0 + (-300.0 - (float)GET_AC(victim)) * 0.0095238095;
  float percent = 30 + num1 * (float)(toHit)-num2;

  // "percent" now contains the maximum avoidance rate. If they do not have two maxed defensive skills, it will actually be less.

  // Determine defensive skills.
  int parry = IS_NPC(victim) ? ISSET(victim->mobdata->actflags, ACT_PARRY) ? victim->getLevel() / 2 : 0 : victim->has_skill(SKILL_PARRY);
  int dodge = IS_NPC(victim) ? ISSET(victim->mobdata->actflags, ACT_DODGE) ? victim->getLevel() / 2 : 0 : victim->has_skill(SKILL_DODGE);
  int block = victim->has_skill(SKILL_SHIELDBLOCK);
  int martial = victim->has_skill(SKILL_DEFENSE);
  int tumbling = victim->has_skill(SKILL_TUMBLING);

  debug_isHit_generic(ch, victim, parry, dodge, block, martial, tumbling);

  if (victim->equipment[WIELD] == nullptr)
    parry = 0;

  if (!victim->equipment[WEAR_SHIELD])
    block = 0;
  else if (IS_NPC(victim))
    block = victim->getLevel() / 2;

  // Modify defense rate accordingly
  int amt = parry + dodge + block + martial + tumbling;

  float scale = (float)amt / (196.0); // Mobs can get a bonus if they can perform 3+.

  percent *= (1.0 - scale / 5.0);

  if (parry)
    skill_increase_check(victim, SKILL_PARRY, parry, SKILL_INCREASE_HARD + 500);
  if (dodge)
    skill_increase_check(victim, SKILL_DODGE, dodge, SKILL_INCREASE_HARD + 500);
  if (block)
    skill_increase_check(victim, SKILL_SHIELDBLOCK, block, SKILL_INCREASE_HARD + 500);
  if (martial)
    skill_increase_check(victim, SKILL_DEFENSE, martial, SKILL_INCREASE_HARD + 500);
  if (tumbling)
    skill_increase_check(victim, SKILL_TUMBLING, tumbling, SKILL_INCREASE_HARD + 500);

  // Ze random stuff.
  if (number(1, 100) < (int)percent && !DC::isSet(victim->combat, COMBAT_BLADESHIELD1) && !DC::isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    debug_isHit(ch, victim, attacktype, type, reduce, toHit, QString("Ze random stuff percent=%1").arg(percent));
    return eFAILURE;
  }

  // Miss, determine a message

  amt += 8; // Chance for a pure miss.

  int what = number(0, amt);
  int retval = 0;

  if (what < parry || DC::isSet(victim->combat, COMBAT_BLADESHIELD1) || DC::isSet(victim->combat, COMBAT_BLADESHIELD2))
  { // Parry. Riposte-check goes here.
    act("$n parries $N's attack.", victim, nullptr, ch, TO_ROOM, NOTVICT);
    act("$n parries your attack.", victim, nullptr, ch, TO_VICT, 0);
    act("You parry $N's attack.", victim, nullptr, ch, TO_CHAR, 0);
    retval = check_riposte(ch, victim, attacktype);
    if (SOMEONE_DIED(retval))
    {
      debug_isHit(ch, victim, attacktype, type, reduce, toHit, "SOMEONE_DIED");
      return debug_retval(ch, victim, retval);
    }
  }
  else if (what < (parry + tumbling))
  { // Tumbling
    switch (number(0, 2))
    {
    case 0: // full avoid
      if (number(0, 1))
      {
        act("You spin adroitly to the side, watching in amusement as $N's swing passes by harmlessly.", victim, 0, ch, TO_CHAR, 0);
        act("$n spins adroitly to the side, watching in amusement as your swing passes by harmlessly.", victim, 0, ch, TO_VICT, 0);
        act("$n spins adroitly to the side, watching in amusement as $N's swing passes by harmlessly.", victim, 0, ch, TO_ROOM, NOTVICT);
      }
      else
      {
        act("You jump quickly and execute a full backflip, landing nimbly on your feet as $N's blow misses completely.", victim, 0, ch, TO_CHAR, 0);
        act("$n jumps quickly and executes a full backflip, landing nimbly on $s feet as your blow misses completely.", victim, 0, ch, TO_VICT, 0);
        act("$n jumps quickly and executes a full backflip, landing nimbly on $s feet as $N's blow misses completely.", victim, 0, ch, TO_ROOM, NOTVICT);
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
    act("$n dodges $N's attack.", victim, nullptr, ch, TO_ROOM, NOTVICT);
    act("$n dodges your attack.", victim, nullptr, ch, TO_VICT, 0);
    act("You dodge $N's attack.", victim, nullptr, ch, TO_CHAR, 0);
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
  debug_isHit(ch, victim, attacktype, type, reduce, toHit, "return eSUCCESS");
  return eSUCCESS;
}

// check counter strike never returns eSUCCESS because that would
// get returned from damage as a successful damage, which it's
// not.
int checkCounterStrike(Character *ch, Character *victim)
{
  int retval, lvl = victim->has_skill(SKILL_COUNTER_STRIKE);

  if ((DC::isSet(victim->combat, COMBAT_STUNNED)) ||
      (victim->equipment[WIELD] != nullptr) ||
      (DC::isSet(victim->combat, COMBAT_STUNNED2)) ||
      (DC::isSet(victim->combat, COMBAT_BASH1)) ||
      (DC::isSet(victim->combat, COMBAT_BASH2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)) ||
      (DC::isSet(victim->combat, COMBAT_BLADESHIELD1)) ||
      (DC::isSet(victim->combat, COMBAT_BLADESHIELD2)) ||
      (DC::isSet(ch->combat, COMBAT_BLADESHIELD1)) ||
      (DC::isSet(ch->combat, COMBAT_BLADESHIELD2)))
    return eFAILURE;

  int p = lvl / 2 - (100 - victim->has_skill(SKILL_DEFENSE)) - GET_DEX(ch) + victim->getHP() * 10 / GET_MAX_HIT(victim);

  skill_increase_check(victim, SKILL_COUNTER_STRIKE, lvl, SKILL_INCREASE_HARD);

  if (number(1, 100) > p)
    return eFAILURE;

  switch (number(1, 4))
  {
  case 1:
    act("Upon blocking $N's blow, you counter with a hard strike of your palm!", victim, nullptr, ch, TO_CHAR, 0);
    act("Upon blocking your blow, $n counters with a hard strike of $s palm!", victim, nullptr, ch, TO_VICT, 0);
    act("Upon blocking $N's blow, $n counters with a hard strike of $s palm!", victim, nullptr, ch, TO_ROOM, NOTVICT);
    break;
  case 2:
    act("Upon blocking $N's blow, you counter with a sharp kick of your heel!", victim, nullptr, ch, TO_CHAR, 0);
    act("Upon blocking your blow, $n counters with a sharp kick of $s heel!", victim, nullptr, ch, TO_VICT, 0);
    act("Upon blocking $N's blow, $n counters with a sharp kick of $s heel!", victim, nullptr, ch, TO_ROOM, NOTVICT);
    break;
  case 3:
    act("Upon blocking $N's blow, you spin and land a short, hard strike with your elbow!", victim, nullptr, ch, TO_CHAR, 0);
    act("Upon blocking your blow, $n spins and lands a short, hard strike with $s elbow!", victim, nullptr, ch, TO_VICT, 0);
    act("Upon blocking $N's blow, $n spins and lands a short, hard strike with $s elbow!", victim, nullptr, ch, TO_ROOM, NOTVICT);
    break;
  case 4:
    act("Upon blocking $N's blow, you spin and land a solid strike with your knee!", victim, nullptr, ch, TO_CHAR, 0);
    act("Upon blocking your blow, $n spins and lands a solid strike with $s knee!", victim, nullptr, ch, TO_VICT, 0);
    act("Upon blocking $N's blow, $n spins and lands a solid strike with $s knee!", victim, nullptr, ch, TO_ROOM, NOTVICT);
    break;
  default:
    logentry("Serious screw-up in counter strike!", ANGEL, LogChannels::LOG_BUG);
    break;
  }

  retval = one_hit(victim, ch, TYPE_HIT, FIRST);
  retval = SWAP_CH_VICT(retval);

  REMOVE_BIT(retval, eSUCCESS);
  SET_BIT(retval, eFAILURE);

  return debug_retval(ch, victim, retval);
}

// check counter strike never returns eSUCCESS because that would
// get returned from damage as a successful damage, which it's
// not.
int doTumblingCounterStrike(Character *ch, Character *victim)
{
  int retval;

  if ((DC::isSet(victim->combat, COMBAT_STUNNED)) ||
      (DC::isSet(victim->combat, COMBAT_STUNNED2)) ||
      (DC::isSet(victim->combat, COMBAT_BASH1)) ||
      (DC::isSet(victim->combat, COMBAT_BASH2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)) ||
      (DC::isSet(victim->combat, COMBAT_BLADESHIELD1)) ||
      (DC::isSet(victim->combat, COMBAT_BLADESHIELD2)) ||
      (DC::isSet(ch->combat, COMBAT_BLADESHIELD1)) ||
      (DC::isSet(ch->combat, COMBAT_BLADESHIELD2)))
    return eFAILURE;

  switch (number(1, 2))
  {
  case 1:
    act("$N overextends $Mself as $E strikes you, leaving $M open to your counterattack!", victim, nullptr, ch, TO_CHAR, 0);
    act("You overextend yourself as you strike $n, leaving yourself open to $s counterattack!", victim, nullptr, ch, TO_VICT, 0);
    act("$N overextends $Mself as $E strikes $n, leaving $M open to a counterattack!", victim, nullptr, ch, TO_ROOM, NOTVICT);
    break;
  case 2:
    act("You find an opening in $N's defenses as $E swings, and land a quick counterattack!", victim, nullptr, ch, TO_CHAR, 0);
    act("$n finds an opening in your defenses as you swing, and lands a quick counterattack!", victim, nullptr, ch, TO_VICT, 0);
    act("$n finds an opening in $N's defenses as $E swings, and lands a quick counterattack!", victim, nullptr, ch, TO_ROOM, NOTVICT);
    break;
  default:
    logentry("Serious screw-up in counter strike!", ANGEL, LogChannels::LOG_BUG);
    break;
  }

  retval = one_hit(victim, ch, TYPE_HIT, FIRST);
  retval = SWAP_CH_VICT(retval);

  REMOVE_BIT(retval, eSUCCESS);
  SET_BIT(retval, eFAILURE);

  return debug_retval(ch, victim, retval);
}

// check riposte never returns eSUCCESS because that would
// get returned from damage as a successful damage, which it's
// not.
int check_riposte(Character *ch, Character *victim, int attacktype)
{
  int retval;

  if ((DC::isSet(victim->combat, COMBAT_STUNNED)) ||
      (ch->equipment[WIELD] == nullptr && number(1, 101) >= 50) ||
      (DC::isSet(victim->combat, COMBAT_STUNNED2)) ||
      (DC::isSet(victim->combat, COMBAT_BASH1)) ||
      (DC::isSet(victim->combat, COMBAT_BASH2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)) ||
      (DC::isSet(victim->combat, COMBAT_BLADESHIELD1)) ||
      (DC::isSet(victim->combat, COMBAT_BLADESHIELD2)) ||
      (DC::isSet(ch->combat, COMBAT_BLADESHIELD1)) ||
      (DC::isSet(ch->combat, COMBAT_BLADESHIELD2)))
    return eFAILURE;

  // 25% chance of success for mobs
  if (IS_NPC(victim))
  {
    if (number(0, 3) > 0)
    {
      return eFAILURE;
    }
  }
  else
  {
    if (!victim->has_skill(SKILL_RIPOSTE))
    {
      return eFAILURE;
    }
    else
    {
      int modifier = 0;

      modifier += speciality_bonus(ch, attacktype, victim->getLevel());
      modifier -= GET_DEX(ch) / 2;
      modifier -= 10;

      if (!skill_success(victim, ch, SKILL_RIPOSTE, modifier))
      {
        return eFAILURE;
      }
    }
  }

  act("$n turns $N's attack into one of $s own!", victim, nullptr, ch, TO_ROOM, NOTVICT);
  act("$n turns your attack against you!", victim, nullptr, ch, TO_VICT, 0);
  act("You turn $N's attack against $M.", victim, nullptr, ch, TO_CHAR, 0);

  retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
  retval = SWAP_CH_VICT(retval);

  REMOVE_BIT(retval, eSUCCESS);
  SET_BIT(retval, eFAILURE);

  return debug_retval(ch, victim, retval);
}

int check_magic_block(Character *ch, Character *victim, int attacktype)
{
  int reduce = 0;
  if (victim->equipment[WEAR_SHIELD] == nullptr && GET_CLASS(victim) != CLASS_MONK)
    return 0;
  if ((DC::isSet(victim->combat, COMBAT_STUNNED)) ||
      //    (victim->equipment[WEAR_SHIELD] == nullptr) ||
      (IS_NPC(victim) && (!ISSET(victim->mobdata->actflags, ACT_PARRY))) ||
      (DC::isSet(victim->combat, COMBAT_STUNNED2)) ||
      (DC::isSet(victim->combat, COMBAT_BASH1)) ||
      (DC::isSet(victim->combat, COMBAT_BASH2)) ||
      (DC::isSet(victim->combat, COMBAT_SHOCKED)) ||
      (DC::isSet(victim->combat, COMBAT_SHOCKED2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return 0;
  if (IS_NPC(victim))
    reduce = victim->getLevel() / 2; // shrug
  if (!(reduce = victim->has_skill(SKILL_SHIELDBLOCK)) && !(GET_CLASS(victim) == CLASS_MONK))
    return 0;
  else if (!((reduce = victim->has_skill(SKILL_DEFENSE)) && GET_CLASS(victim) == CLASS_MONK))
    return 0;

  int skill = reduce / 10;
  reduce = (int)((float)reduce * 0.75);

  if (number(1, 101) > skill)
    return 0;
  if (GET_CLASS(victim) != CLASS_MONK)
  {
    act("$n blocks part of $N's spell with $p.", victim, victim->equipment[WEAR_SHIELD], ch, TO_ROOM, NOTVICT);
    act("$n blocks part of your spell with $p.", victim, victim->equipment[WEAR_SHIELD], ch, TO_VICT, 0);
    act("You dodge down behind $p and deflect part of $N's spell.", victim, victim->equipment[WEAR_SHIELD], ch, TO_CHAR, 0);
  }
  else
  {
    act("$n manages to avoid taking a direct hit from $N's spell!", victim, nullptr, ch, TO_ROOM, NOTVICT);
    act("$n avoids part of your spell!", victim, nullptr, ch, TO_VICT, 0);
    act("Your martial defense allows you to avoid a direct hit from $N's spell!", victim, nullptr, ch, TO_CHAR, 0);
    reduce = (int)((float)reduce / 1.25);
  }
  return reduce;
}

int check_shieldblock(Character *ch, Character *victim, int attacktype)
{
  int modifier = 0;
  int reduce = 0;
  if (victim->equipment[WEAR_SHIELD] == nullptr && GET_CLASS(victim) != CLASS_MONK)
    return 0;
  if ((DC::isSet(victim->combat, COMBAT_STUNNED)) ||
      //    (victim->equipment[WEAR_SHIELD] == nullptr) ||
      (IS_NPC(victim) && (!ISSET(victim->mobdata->actflags, ACT_PARRY))) ||
      (DC::isSet(victim->combat, COMBAT_STUNNED2)) ||
      (DC::isSet(victim->combat, COMBAT_BASH1)) ||
      (DC::isSet(victim->combat, COMBAT_BASH2)) ||
      (DC::isSet(victim->combat, COMBAT_SHOCKED)) ||
      (DC::isSet(victim->combat, COMBAT_SHOCKED2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return 0;

  // TODO - remove this when mobs have "skills"
  if (IS_NPC(victim))
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
      modifier = 0;
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

  //  extern int stat_mod[];
  // modifier -= stat_mod[GET_DEX(ch)];

  //  if (IS_NPC(victim)) modifier -= 50;
  //  modifier += GET_DEX(victim)/2;
  if (GET_CLASS(victim) == CLASS_MONK)
  {
    if (!skill_success(victim, ch, SKILL_DEFENSE, modifier))
      return 0;
  }
  else if (!skill_success(victim, ch, SKILL_SHIELDBLOCK, modifier))
    return 0;

  // act("$n blocks $N's attack with $s shield.", victim, nullptr, ch, TO_ROOM, NOTVICT);
  // act("$n blocks your attack with $s shield.", victim, nullptr, ch, TO_VICT, 0);
  // act("You block $N's attack with your shield.", victim, nullptr, ch, TO_CHAR, 0);
  /*  if (!GET_CLASS(victim) == CLASS_MONK) {
      act("$n blocks $N's attack with $p.", victim, victim->equipment[WEAR_SHIELD], ch, TO_ROOM, NOTVICT);
      act("$n blocks your attack with $p.", victim, victim->equipment[WEAR_SHIELD], ch, TO_VICT, 0);
      act("You block $N's attack with $p.", victim, victim->equipment[WEAR_SHIELD], ch, TO_CHAR, 0);
    } else {
      act("$n swiftly deflects $N's attack.", victim, nullptr, ch, TO_ROOM, NOTVICT);
      act("$n swiftly deflects your attack.",victim, nullptr, ch, TO_VICT,0);
      act("You swiftly deflect $N's attack.",victim, nullptr, ch, TO_CHAR, 0);
    }*/
  if (GET_CLASS(victim) == CLASS_MONK)
    reduce = (int)((float)reduce / 1.25);

  return reduce;
}

bool check_parry(Character *ch, Character *victim, int attacktype, bool display_results)
{
  int modifier = 0;
  if ((DC::isSet(victim->combat, COMBAT_STUNNED)) ||
      (victim->equipment[WIELD] == nullptr) ||
      (IS_NPC(victim) && (!ISSET(victim->mobdata->actflags, ACT_PARRY))) ||
      (DC::isSet(victim->combat, COMBAT_STUNNED2)) ||
      ((DC::isSet(victim->combat, COMBAT_BASH1) ||
        DC::isSet(victim->combat, COMBAT_BASH2)) &&
       !DC::isSet(victim->combat, COMBAT_BLADESHIELD1) &&
       !DC::isSet(victim->combat, COMBAT_BLADESHIELD2)) ||
      (DC::isSet(victim->combat, COMBAT_SHOCKED)) ||
      (DC::isSet(victim->combat, COMBAT_SHOCKED2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return false;

  if (IS_NPC(victim))
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
      modifier = 0;
      break;
    }
  }
  else if (!victim->has_skill(SKILL_PARRY))
    return false;
  if (!modifier && IS_NPC(victim) && (ISSET(victim->mobdata->actflags, ACT_PARRY)))
    modifier = 10;
  else if (IS_NPC(victim) && !ISSET(victim->mobdata->actflags, ACT_PARRY))
    return false; // damned mobs

  modifier += speciality_bonus(ch, attacktype, victim->getLevel());
  //  if (IS_NPC(victim)) modifier -= 50;
  //  if (attacktype==TYPE_HIT) modifier += 30; // Harder to parry unarmed attacks
  //  else modifier += 22;
  modifier -= GET_DEX(ch) / 2;
  modifier -= 10;
  if (!skill_success(victim, ch, SKILL_PARRY, modifier) &&
      !DC::isSet(victim->combat, COMBAT_BLADESHIELD1) &&
      !DC::isSet(victim->combat, COMBAT_BLADESHIELD2))
    return false;

  if (display_results == true)
  {
    act("$n parries $N's attack.", victim, nullptr, ch, TO_ROOM, NOTVICT);
    act("$n parries your attack.", victim, nullptr, ch, TO_VICT, 0);
    act("You parry $N's attack.", victim, nullptr, ch, TO_CHAR, 0);
  }
  return true;
}

int speciality_bonus(Character *ch, int attacktype, int level)
{
  int skill = 0;
  /*  int w_type = TYPE_HIT;
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

  if (level < -20 && IS_NPC(ch))
    return 0 - (int)(ch->getLevel() / 2.4);
  else if (level < -10 && IS_NPC(ch))
    return 0 - (int)(ch->getLevel() / 2.6);
  else if (level < 0 && IS_NPC(ch))
    return 0 - (int)(ch->getLevel() / 2.8);
  else if (IS_NPC(ch))
    return 0 - (int)(ch->getLevel() / 3.5);

  int l = ch->has_skill(skill) / 2;
  return 0 - l;
}

/*
 * Check for dodge.
 */
bool check_dodge(Character *ch, Character *victim, int attacktype, bool display_results)
{
  //  int chance;
  int modifier = 0;
  if ((DC::isSet(victim->combat, COMBAT_STUNNED)) ||
      (DC::isSet(victim->combat, COMBAT_STUNNED2)) ||
      (DC::isSet(victim->combat, COMBAT_BASH1)) ||
      (DC::isSet(victim->combat, COMBAT_BASH2)) ||
      (DC::isSet(victim->combat, COMBAT_SHOCKED)) ||
      (DC::isSet(victim->combat, COMBAT_SHOCKED2)) ||
      (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return false;

  if (IS_NPC(victim))
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
      modifier = 0;
      break;
    }
    if (!ISSET(victim->mobdata->actflags, ACT_DODGE))
      modifier = 0; // damned mobs
    else if (modifier == 0)
      modifier = 5;
  }
  else if (!victim->has_skill(SKILL_DODGE))
    return false;

  if (modifier == 0 && IS_NPC(victim))
    return false;

  modifier += speciality_bonus(ch, attacktype, victim->getLevel());
  //  if (IS_NPC(victim)) modifier = 50; // 75 is base, and it's calculated
  // around here
  modifier -= GET_DEX(ch) / 2;
  if (!skill_success(victim, ch, SKILL_DODGE, modifier))
    return false;

  if (display_results == true)
  {
    act("$n dodges $N's attack.", victim, nullptr, ch, TO_ROOM, NOTVICT);
    act("$n dodges your attack.", victim, nullptr, ch, TO_VICT, 0);
    act("You dodge $N's attack.", victim, nullptr, ch, TO_CHAR, 0);
  }

  return true;
}

/*
 * Load fighting messages into memory.
 */
void load_messages(char *file, int base)
{
  FILE *fl;
  int i, type;
  extern struct message_list fight_messages[MAX_MESSAGES];
  struct message_type *messages;
  char chk[100];

  if (!(fl = fopen(file, "r")))
  {
    perror("read messages");
    exit(0);
  }
  if (base == 0)
    for (i = 0; i < MAX_MESSAGES; i++)
    {
      fight_messages[i].a_type = 0;
      fight_messages[i].number_of_attacks = 0;
      fight_messages[i].msg = 0;
      fight_messages[i].msg2 = 0;
    }

  fscanf(fl, "%s\n", chk);

  while (*chk == 'M')
  {
    fscanf(fl, " %d\n", &type);
    //     type += base;
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
                (fight_messages[i].a_type);
         i++)
      ;
    //     if (type == 80)
    //	 produce_coredump();
    if (i >= MAX_MESSAGES)
    {
      logentry("Too many combat messages.", ANGEL, LogChannels::LOG_BUG);
      exit(0);
    }

#ifdef LEAK_CHECK
    messages = (struct message_type *)
        calloc(1, sizeof(struct message_type));
#else
    messages = (struct message_type *)
        dc_alloc(1, sizeof(struct message_type));
#endif
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

void free_messages_from_memory()
{
  extern struct message_list fight_messages[MAX_MESSAGES];
  struct message_type *next_message = nullptr;
  int i;

  for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type); i++)
    while (fight_messages[i].msg)
    {
      next_message = fight_messages[i].msg->next;
      dc_free(fight_messages[i].msg->die_msg.attacker_msg);
      dc_free(fight_messages[i].msg->die_msg.victim_msg);
      dc_free(fight_messages[i].msg->die_msg.room_msg);
      dc_free(fight_messages[i].msg->miss_msg.attacker_msg);
      dc_free(fight_messages[i].msg->miss_msg.victim_msg);
      dc_free(fight_messages[i].msg->miss_msg.room_msg);
      dc_free(fight_messages[i].msg->hit_msg.attacker_msg);
      dc_free(fight_messages[i].msg->hit_msg.victim_msg);
      dc_free(fight_messages[i].msg->hit_msg.room_msg);
      dc_free(fight_messages[i].msg->god_msg.attacker_msg);
      dc_free(fight_messages[i].msg->god_msg.victim_msg);
      dc_free(fight_messages[i].msg->god_msg.room_msg);
      dc_free(fight_messages[i].msg);
      fight_messages[i].msg = next_message;
    }
}

/*
 * Set position of a victim.
 */
void update_pos(Character *victim)
{
  if (victim->getHP() > 0)
  {
    if ((!DC::isSet(victim->combat, COMBAT_STUNNED)) && (!DC::isSet(victim->combat, COMBAT_STUNNED2)))
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
void set_fighting(Character *ch, Character *vict)
{
  Character *k, *next_char;
  int count = 0;

  if (ch->fighting) /* If he's already fighting */
    return;
  if (IS_AFFECTED(ch, AFF_HIDE))
    REMBIT(ch->affected_by, AFF_HIDE);
  if (IS_AFFECTED(vict, AFF_HIDE))
    REMBIT(vict->affected_by, AFF_HIDE);

  if (IS_PC(ch) && IS_NPC(vict))
    if (!ISSET(vict->mobdata->actflags, ACT_STUPID))
      vict->add_memory(GET_NAME(ch), 'h');

  if (IS_NPC(ch) && IS_NPC(vict) && IS_AFFECTED(ch, AFF_CHARM) &&
      ch->master && IS_PC(ch->master))
    if (!ISSET(vict->mobdata->actflags, ACT_STUPID))
      vict->add_memory(GET_NAME(ch->master), 'h');

  if (IS_PC(ch) && IS_NPC(vict))
    if (!ISSET(vict->mobdata->actflags, ACT_STUPID) && vict->hunting.isEmpty())
    {
      level_diff_t level_difference = ch->getLevel() - (vict->getLevel() / 2.0);
      if (level_difference > 0 || ch->getLevel() == 60)
      {
        vict->add_memory(GET_NAME(ch), 't');
        struct timer_data *timer;
#ifdef LEAK_CHECK
        timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
#else
        timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
#endif
        timer->var_arg1 = vict->hunting;
        timer->arg2 = (void *)vict;
        timer->function = clear_hunt;
        timer->next = timer_list;
        timer_list = timer;
        timer->timeleft = (ch->getLevel() / 4) * 60;
      }
      if (IS_PC(vict) && IS_NPC(ch))
        if (!ISSET(ch->mobdata->actflags, ACT_STUPID) && ch->hunting.isEmpty())
        {
          level_diff_t level_difference = vict->getLevel() - (ch->getLevel() / 2);
          if (level_difference > 0 || vict->getLevel() == 60)
          {
            ch->add_memory(vict->getName(), 't');
            struct timer_data *timer;
#ifdef LEAK_CHECK
            timer = (struct timer_data *)calloc(1, sizeof(struct
                                                          timer_data));
#else
            timer = (struct timer_data *)dc_alloc(1, sizeof(struct
                                                            timer_data));
#endif
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

  /*(  if( ( IS_PC(ch) || IS_AFFECTED(ch, AFF_CHARM) )
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

  if ((!DC::isSet(ch->combat, COMBAT_STUNNED)) &&
      (!DC::isSet(ch->combat, COMBAT_STUNNED2)) &&
      (!DC::isSet(ch->combat, COMBAT_BASH1)) &&
      (!DC::isSet(ch->combat, COMBAT_BASH2)))
    ch->setPOSFighting();

  return;
}

// Stop fights.
void stop_fighting(Character *ch, int clearlag)
{
  Character *tmp;

  if (!ch)
  {
    logentry("Null ch in stop_fighting.  This would have crashed us.", IMPLEMENTER, LogChannels::LOG_BUG);
    return;
  }

  if (!ch->fighting)
    return;

  // This is in the command interpreter now, so berserk lasts
  // until you are totally done fighting.
  // -Sadus
  if (DC::isSet(ch->combat, COMBAT_BERSERK))
  {
    bool keepZerk = false;
    for (tmp = DC::getInstance()->world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
      if (tmp->fighting == ch)
        keepZerk = true;
    if (!keepZerk)
    {
      REMOVE_BIT(ch->combat, COMBAT_BERSERK);
      act("$n settles down.", ch, 0, 0, TO_ROOM, 0);
      act("You settle down.", ch, 0, 0, TO_CHAR, 0);
      GET_AC(ch) -= 30;
    }
  }

  REMOVE_BIT(ch->combat, COMBAT_ATTACKER);

  if (IS_AFFECTED(ch, AFF_PRIMAL_FURY))
  {
    struct affected_type *af;

    for (af = ch->affected; af; af = af->next)
    {
      if (af->bitvector && af->type == SKILL_PRIMAL_FURY)
      {
        affect_remove(ch, af, 0);
        break;
      }
    }
  }

  if (DC::isSet(ch->combat, COMBAT_RAGE1))
  {
    REMOVE_BIT(ch->combat, COMBAT_RAGE1);
    act("$n calms down.", ch, 0, 0, TO_ROOM, 0);
    act("Your mind seems a bit clearer now.", ch, 0, 0, TO_CHAR, 0);
  }
  if (DC::isSet(ch->combat, COMBAT_RAGE2))
  {
    REMOVE_BIT(ch->combat, COMBAT_RAGE2);
    act("$n calms down.", ch, 0, 0, TO_ROOM, 0);
    act("Your mind seems a bit clearer now.", ch, 0, 0, TO_CHAR, 0);
  }

  // make sure people aren't stuck unable to do anything
  if (DC::isSet(ch->combat, COMBAT_SHOCKED2))
    REMOVE_BIT(ch->combat, COMBAT_SHOCKED2);
  if (DC::isSet(ch->combat, COMBAT_SHOCKED))
    REMOVE_BIT(ch->combat, COMBAT_SHOCKED);

  if (DC::isSet(ch->combat, COMBAT_ORC_BLOODLUST1))
  {
    REMOVE_BIT(ch->combat, COMBAT_ORC_BLOODLUST1);
  }
  if (DC::isSet(ch->combat, COMBAT_ORC_BLOODLUST2))
  {
    REMOVE_BIT(ch->combat, COMBAT_ORC_BLOODLUST2);
  }
  if (DC::isSet(ch->combat, COMBAT_VITAL_STRIKE))
    REMOVE_BIT(ch->combat, COMBAT_VITAL_STRIKE);
  if (DC::isSet(ch->combat, COMBAT_BLADESHIELD1))
    REMOVE_BIT(ch->combat, COMBAT_BLADESHIELD1);
  if (DC::isSet(ch->combat, COMBAT_BLADESHIELD2))
    REMOVE_BIT(ch->combat, COMBAT_BLADESHIELD2);
  if (DC::isSet(ch->combat, COMBAT_THI_EYEGOUGE))
  {
    REMOVE_BIT(ch->combat, COMBAT_THI_EYEGOUGE);
    REMBIT(ch->affected_by, AFF_BLIND);
  }
  if (DC::isSet(ch->combat, COMBAT_THI_EYEGOUGE2))
  {
    REMOVE_BIT(ch->combat, COMBAT_THI_EYEGOUGE2);
    REMBIT(ch->affected_by, AFF_BLIND);
  }

  affect_from_char(ch, KI_DISRUPT + KI_OFFSET);

  ch->setStanding();
  ;
  update_pos(ch);

  // Remove ch's lag if he wasn't using wimpy.
  if (IS_PC(ch) && ch->desc && !DC::isSet(ch->player->toggles, Player::PLR_WIMPY) && clearlag)
    ch->desc->wait = 0;

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
      logentry("Stop_fighting: char not found", ANGEL, LogChannels::LOG_BUG);
      // abort();
      return;
    }
    tmp->next_fighting = ch->next_fighting;
  }

  ch->next_fighting = 0;
  ch->fighting = 0;

  if (DC::isSet(ch->combat, COMBAT_BASH1))
    REMOVE_BIT(ch->combat, COMBAT_BASH1);
  if (DC::isSet(ch->combat, COMBAT_BASH2))
    REMOVE_BIT(ch->combat, COMBAT_BASH2);
  if (DC::isSet(ch->combat, COMBAT_STUNNED))
    REMOVE_BIT(ch->combat, COMBAT_STUNNED);
  if (DC::isSet(ch->combat, COMBAT_STUNNED2))
    REMOVE_BIT(ch->combat, COMBAT_STUNNED2);
  if (DC::isSet(ch->combat, COMBAT_CRUSH_BLOW))
    REMOVE_BIT(ch->combat, COMBAT_CRUSH_BLOW);
  if (DC::isSet(ch->combat, COMBAT_CRUSH_BLOW2))
    REMOVE_BIT(ch->combat, COMBAT_CRUSH_BLOW2);

  ch->last_damage = time(nullptr);

  uint64_t fight_length = ch->last_damage - ch->first_damage;
  if (fight_length < 1)
  {
    fight_length = 1;
  }

  ch->damage_per_second = ch->damage_done / fight_length;
  if (ch->getSetting("fighting.showdps", "0") == "1")
  {
    csendf(ch, "You caused %lu damage over %lu seconds with DPS of %lu.\r\n", ch->damage_done, fight_length, ch->damage_per_second);
  }
  return;
}

void make_scraps(Character *ch, class Object *obj)
{
  class Object *corpse /*, *o*/;
  char buf[MAX_STRING_LENGTH];
  /*int i;*/

  corpse = new Object;
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name = str_hsh("scraps");

  sprintf(buf, "A pile of scraps from %s is lying here.",
          obj->short_description);
  corpse->description = str_hsh(buf);

  sprintf(buf, "a pile of scraps.");
  corpse->short_description = str_hsh(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = ITEM_TAKE;
  corpse->obj_flags.value[0] = 0;
  corpse->obj_flags.value[3] = 0;
  corpse->obj_flags.weight = obj->obj_flags.weight;
  corpse->obj_flags.eq_level = 0;

  corpse->obj_flags.more_flags = 0;

  corpse->obj_flags.timer = 0;

  object_list_new_new_owner(corpse, 0);
  obj_to_room(corpse, ch->in_room);

  return;
}

static constexpr uint64_t MAX_NPC_CORPSE_TIME = 10;
static constexpr uint64_t MAX_PC_CORPSE_TIME = 30;

void make_corpse(Character *ch)
{
  class Object *corpse, *o, *o_in_container, *next_o_in_container;
  class Object *money, *next_obj;
  extern class Object *object_list;
  char buf[MAX_STRING_LENGTH];
  int i;

  corpse = new Object;
  clear_object(corpse);
  if (IS_PC(ch))
  {
    corpse->setOwner(GET_NAME(ch));
  }

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;

  // If pc is in the name, the consent system works
  // Thieves don't deserve consent! Loot time!
  // Morc

  if (IS_NPC(ch))
  {
    corpse->obj_flags.wear_flags = 0;
    sprintf(buf, "corpse %s", GET_NAME(ch));
  }
  else if (affected_by_spell(ch, FUCK_PTHIEF))
  {
    corpse->obj_flags.wear_flags = 0;
    sprintf(buf, "corpse %s thiefcorpse", GET_NAME(ch));
  }
  else
  {
    corpse->obj_flags.wear_flags = ITEM_TAKE;

    if (ch->getLevel() >= 50)
      sprintf(buf, "corpse %s pc lootable", GET_NAME(ch));
    else
      sprintf(buf, "corpse %s pc", GET_NAME(ch));
  }
  corpse->name = str_hsh(buf);

  sprintf(buf, "the corpse of %s is lying here.",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->description = str_hsh(buf);

  sprintf(buf, "the corpse of %s",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->short_description = str_hsh(buf);

  corpse->obj_flags.type_flag = ITEM_CONTAINER;
  corpse->obj_flags.value[0] = 0; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1; /* corpse identifier */
  corpse->obj_flags.weight = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  corpse->obj_flags.eq_level = 0;

  SET_BIT(GET_OBJ_EXTRA(corpse), ITEM_UNIQUE_SAVE);
  if (IS_NPC(ch))
  {
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
    corpse->obj_flags.more_flags = 0;
    SET_BIT(corpse->obj_flags.more_flags, ITEM_NPC_CORPSE);
    GET_OBJ_VROOM(corpse) = DC::NOWHERE;
  }
  else
  {
    corpse->obj_flags.more_flags = 0;
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
      for (i = 0; i < MAX_WEAR; i++)
      {
        if (ch->equipment[i] && ch->equipment[i]->obj_flags.eq_level >= 50)
        {
          SET_BIT(corpse->obj_flags.more_flags, ITEM_LIMIT_SACRIFICE);
          break;
        }
      }

      // If the above didn't already set the corpse object with flag ITEM_LIMIT_SACRIFICE
      // then we search its inventory including container contents
      if (!DC::isSet(corpse->obj_flags.more_flags, ITEM_LIMIT_SACRIFICE))
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
  if (IS_MOB(ch) || ch->getLevel() > 19)
  {
    for (i = 0; i < MAX_WEAR; i++)
      if (ch->equipment[i])
        obj_to_char(unequip_char(ch, i), ch);

    if (ch->getGold() > 0)
    {
      money = create_money(ch->getGold());
      ch->setGold(0);
      obj_to_obj(money, corpse);
    }

    if (IS_MOB(ch) && ch->getLevel() > 60 && number(1, 100) > 90) // 10%
    {
      class Object *recipeitem = nullptr;
      int rarity = number(1, 100);
      bool itemtype = number(0, 1);
      if (rarity > 95) // 96-100 5%
      {
        switch (itemtype)
        {
        case 0: // bottle
          recipeitem = clone_object(real_object(6324));
          break;
        case 1:
          recipeitem = clone_object(real_object(6338));
          break;
        }
      }
      else if (rarity > 85) // 85-95 10%
      {
        switch (itemtype)
        {
        case 0: // bottle
          recipeitem = clone_object(real_object(6323));
          break;
        case 1:
          recipeitem = clone_object(real_object(6339));
          break;
        }
      }
      else if (rarity > 65) // 65-85 20%
      {
        switch (itemtype)
        {
        case 0: // bottle
          recipeitem = clone_object(real_object(6322));
          break;
        case 1:
          recipeitem = clone_object(real_object(6340));
          break;
        }
      }
      else if (rarity > 40) // 41-65 25%
      {
        switch (itemtype)
        {
        case 0: // bottle
          recipeitem = clone_object(real_object(6321));
          break;
        case 1:
          recipeitem = clone_object(real_object(6341));
          break;
        }
      }
      else // 1-40 40%
      {
        switch (itemtype)
        {
        case 0: // bottle
          recipeitem = clone_object(real_object(6320));
          break;
        case 1:
          recipeitem = clone_object(real_object(6342));
          break;
        }
      }
      if (recipeitem > (Object *)0)
      {
        obj_to_obj(recipeitem, corpse);
      }
      else
      {
        char bugmsg[MAX_STRING_LENGTH];
        sprintf(bugmsg, "%s was supposed to drop a paper/bottle but was unable to create the item", GET_NAME(ch));
        logentry(bugmsg, IMMORTAL, LogChannels::LOG_BUG);
      }
    }

    for (o = ch->carrying; o; o = next_obj)
    {
      next_obj = o->next_content;

      if (DC::isSet(o->obj_flags.extra_flags, ITEM_SPECIAL) &&
          (GET_ITEM_TYPE(o) == ITEM_CONTAINER))
        for (o_in_container = o->contains; o_in_container; o_in_container = next_o_in_container)
        {
          next_o_in_container = o_in_container->next_content;
          if (!DC::isSet(o_in_container->obj_flags.extra_flags, ITEM_SPECIAL))
          {
            move_obj(o_in_container, corpse);
          }
        } // if and for

      if (!DC::isSet(o->obj_flags.extra_flags, ITEM_SPECIAL))
        move_obj(o, corpse);

    } // for
  }

  corpse->next = object_list;
  object_list = corpse;

  // TODO - i think this is taken care of in "obj_to_obj"...check it, and if so
  // remove this line.  (It's updating in_obj pointer for everything in corpse)
  for (o = corpse->contains; o; o->in_obj = corpse, o = o->next_content)
    ;

  object_list_new_new_owner(corpse, 0);
  obj_to_room(corpse, ch->in_room);

  save_corpses();
  return;
}

void make_dust(Character *ch)
{
  class Object *o, *tmp_o, *blah;
  class Object *money, *next_obj;
  int i;

  for (i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i])
      obj_to_char(unequip_char(ch, i), ch);

  if (ch->getGold() > 0)
  {
    money = create_money(ch->getGold());
    ch->setGold(0);
    obj_to_room(money, ch->in_room);
  }

  for (o = ch->carrying; o; o = next_obj)
  {
    next_obj = o->next_content;

    if (DC::isSet(o->obj_flags.extra_flags, ITEM_SPECIAL) && (GET_ITEM_TYPE(o) == ITEM_CONTAINER))
      for (tmp_o = o->contains; tmp_o; tmp_o = blah)
      {
        blah = tmp_o->next_content;
        if (!DC::isSet(tmp_o->obj_flags.extra_flags, ITEM_SPECIAL))
          move_obj(tmp_o, ch->in_room);

      } // if and for

    if (!DC::isSet(o->obj_flags.extra_flags, ITEM_SPECIAL))
      move_obj(o, ch->in_room);

  } // for

  return;
}

int alignment_value(int val)
{
  if (val >= 350)
    return 1;
  if (val <= -350)
    return -1;
  return 0;
}

// run through eq removing and rewearing is
void zap_eq_check(Character *ch)
{
  SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT);
  for (int i = 0; i < MAX_WEAR; i++)
    if (ch->equipment[i])
      equip_char(ch, unequip_char(ch, i, 1), i, 1);
  REMBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT);
}

// ch kills victim
void change_alignment(Character *ch, Character *victim)
{
  int change;

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

void make_husk(Character *ch)
{
  class Object *corpse;
  char buf[MAX_STRING_LENGTH];

#ifdef LEAK_CHECK
  corpse = (class Object *)calloc(1, sizeof(class Object));
#else
  corpse = (class Object *)dc_alloc(1, sizeof(class Object));
#endif
  clear_object(corpse);
  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name = str_hsh("husk");
  sprintf(buf, "The withered husk of %s, its soul drained, flutters here.",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->description = str_hsh(buf);
  sprintf(buf, "Husk of %s",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->short_description = str_hsh(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = 0;
  corpse->obj_flags.value[0] = 0;
  corpse->obj_flags.value[3] = 1;
  corpse->obj_flags.weight = 1000;
  corpse->obj_flags.eq_level = 0;
  if (IS_NPC(ch))
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }
  obj_to_room(corpse, ch->in_room);
  return;
}

void make_head(Character *ch)
{
  class Object *corpse;
  char buf[MAX_STRING_LENGTH];

#ifdef LEAK_CHECK
  corpse = (class Object *)calloc(1, sizeof(class Object));
#else
  corpse = (class Object *)dc_alloc(1, sizeof(class Object));
#endif
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name = str_hsh("head");

  sprintf(buf, "The head of %s is lying here.",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->description = str_hsh(buf);

  sprintf(buf, "Head of %s",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->short_description = str_hsh(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = ITEM_TAKE + ITEM_WEAR_HOLD;
  corpse->obj_flags.value[0] = 0; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1; /* corpse identifyer */
  corpse->obj_flags.weight = 5;
  corpse->obj_flags.eq_level = 0;
  if (IS_NPC(ch))
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  obj_to_room(corpse, ch->in_room);

  return;
}

void make_arm(Character *ch)
{
  class Object *corpse;
  char buf[MAX_STRING_LENGTH];

#ifdef LEAK_CHECK
  corpse = (class Object *)calloc(1, sizeof(class Object));
#else
  corpse = (class Object *)dc_alloc(1, sizeof(class Object));
#endif
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name = str_hsh("arm");

  sprintf(buf, "The arm of %s is lying here.",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->description = str_hsh(buf);

  sprintf(buf, "Arm of %s",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->short_description = str_hsh(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = ITEM_TAKE + ITEM_WEAR_HOLD;
  corpse->obj_flags.value[0] = 0; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1; /* corpse identifyer */
  corpse->obj_flags.weight = 5;
  corpse->obj_flags.eq_level = 0;
  if (IS_NPC(ch))
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  obj_to_room(corpse, ch->in_room);

  return;
}

void make_leg(Character *ch)
{
  class Object *corpse;
  char buf[MAX_STRING_LENGTH];

#ifdef LEAK_CHECK
  corpse = (class Object *)calloc(1, sizeof(class Object));
#else
  corpse = (class Object *)dc_alloc(1, sizeof(class Object));
#endif
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name = str_hsh("leg");

  sprintf(buf, "The leg of %s is lying here.",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->description = str_hsh(buf);

  sprintf(buf, "Leg of %s",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->short_description = str_hsh(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = ITEM_TAKE + ITEM_WEAR_HOLD;
  corpse->obj_flags.value[0] = 0; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1; /* corpse identifyer */
  corpse->obj_flags.weight = 5;
  corpse->obj_flags.eq_level = 0;
  if (IS_NPC(ch))
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  obj_to_room(corpse, ch->in_room);

  return;
}

void make_bowels(Character *ch)
{
  class Object *corpse;
  char buf[MAX_STRING_LENGTH];

#ifdef LEAK_CHECK
  corpse = (class Object *)calloc(1, sizeof(class Object));
#else
  corpse = (class Object *)dc_alloc(1, sizeof(class Object));
#endif
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name = str_hsh("bowels");

  sprintf(buf, "The steaming bowels of %s is lying here.",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->description = str_hsh(buf);

  sprintf(buf, "Bowels of %s",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->short_description = str_hsh(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = ITEM_TAKE + ITEM_WEAR_HOLD;
  corpse->obj_flags.value[0] = 0; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1; /* corpse identifyer */
  corpse->obj_flags.weight = 5;
  corpse->obj_flags.eq_level = 0;
  if (IS_NPC(ch))
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  obj_to_room(corpse, ch->in_room);

  return;
}

void make_blood(Character *ch)
{
  class Object *corpse;
  char buf[MAX_STRING_LENGTH];

#ifdef LEAK_CHECK
  corpse = (class Object *)calloc(1, sizeof(class Object));
#else
  corpse = (class Object *)dc_alloc(1, sizeof(class Object));
#endif
  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name = str_hsh("blood");

  sprintf(buf, "A pool of %s's blood is here.",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->description = str_hsh(buf);

  sprintf(buf, "Pooled blood of %s",
          (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->short_description = str_hsh(buf);

  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = ITEM_TAKE + ITEM_WEAR_HOLD;
  corpse->obj_flags.value[0] = 0; /* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1; /* corpse identifyer */
  corpse->obj_flags.weight = 5;
  corpse->obj_flags.eq_level = 0;
  if (IS_NPC(ch))
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  obj_to_room(corpse, ch->in_room);

  return;
}

void make_heart(Character *ch, Character *vict)
{
  class Object *corpse;
  char buf[MAX_STRING_LENGTH];
  int hands_are_free(Character * ch, int number);

  if (!hands_are_free(ch, 1))
    return;
#ifdef LEAK_CHECK
  corpse = (class Object *)calloc(1, sizeof(class Object));
#else
  corpse = (class Object *)dc_alloc(1, sizeof(class Object));
#endif

  clear_object(corpse);

  corpse->item_number = -1;
  corpse->in_room = DC::NOWHERE;
  corpse->name = str_hsh("heart");

  sprintf(buf, "%s's heart is laying here.",
          (IS_NPC(vict) ? vict->short_desc : GET_NAME(vict)));
  corpse->description = str_hsh(buf);

  sprintf(buf, "the heart of %s",
          (IS_NPC(vict) ? vict->short_desc : GET_NAME(vict)));
  corpse->short_description = str_hsh(buf);

  corpse->obj_flags.type_flag = ITEM_FOOD;
  corpse->obj_flags.wear_flags = ITEM_TAKE + ITEM_WEAR_HOLD;
  corpse->obj_flags.value[0] = 0; /* You can't store stuff in a heart */
  corpse->obj_flags.value[3] = 1; /* corpse identifier */
  corpse->obj_flags.weight = 2;
  corpse->obj_flags.eq_level = 0;

  if (IS_NPC(ch))
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }

  if (!ch->equipment[HOLD] && !ch->equipment[WIELD] && !ch->equipment[WEAR_LIGHT])
    equip_char(ch, corpse, HOLD);
  else if (!ch->equipment[HOLD2] && !ch->equipment[SECOND_WIELD])
    equip_char(ch, corpse, HOLD2);
  else
    extract_obj(corpse);

  return;
}

void death_cry(Character *ch)
{
  int door, was_in;
  char *message;

  act("Your blood freezes as you hear $n's death cry.",
      ch, 0, 0, TO_ROOM, 0);

  if (IS_NPC(ch))
    message = "You hear something's death cry.";
  else
    message = "You hear someone's death cry.";

  was_in = ch->in_room;
  for (door = 0; door <= 5; door++)
  {
    if (CAN_GO(ch, door))
    {
      ch->in_room = DC::getInstance()->world[was_in].dir_option[door]->to_room;
      if (ch->in_room == was_in)
        continue;
      act(message, ch, 0, 0, TO_ROOM, 0);
      ch->in_room = was_in;
    }
  }
  ch->in_room = was_in;
}

// Return true if killed vict.  False otherwise
int do_skewer(Character *ch, Character *vict, int dam, int wt, int wt2, int weapon)
{
  int damadd = 0;

  if ((GET_CLASS(ch) != CLASS_WARRIOR) && ch->getLevel() < ARCHANGEL)
    return 0;
  if (IS_PC(vict) && vict->getLevel() >= IMMORTAL)
    return 0;
  if (!ch->equipment[weapon])
    return 0;
  if (!ch->has_skill(SKILL_SKEWER))
    return 0;
  // TODO - need to make this take specialization into consideration
  if (ch->in_room != vict->in_room)
    return 0;

  if (affected_by_spell(ch, SKILL_DEFENDERS_STANCE))
    return 0;

  int type = get_weapon_damage_type(ch->equipment[weapon]);
  if (!(type == TYPE_PIERCE || type == TYPE_SLASH || type == TYPE_STING))
    return 0;
  if (!skill_success(ch, vict, SKILL_SKEWER))
    return 0;

  if (number(0, 100) < 25)
  {
    if (check_dodge(ch, vict, type, false))
    {
      act("$n dodges $N's skewer!!", vict, nullptr, ch, TO_ROOM, NOTVICT);
      act("$n dodges your skewer!!", vict, nullptr, ch, TO_VICT, 0);
      act("You dodge $N's skewer!!", vict, nullptr, ch, TO_CHAR, 0);
      return 0;
    }
    if (check_parry(ch, vict, type, false))
    {
      act("$n parries $N's skewer!!", vict, nullptr, ch, TO_ROOM, NOTVICT);
      act("$n parries your skewer!!", vict, nullptr, ch, TO_VICT, 0);
      act("You parry $N's skewer!!", vict, nullptr, ch, TO_CHAR, 0);
      return 0;
    }
    if (check_shieldblock(ch, vict, type))
    {
      // act("$2$n shield blocks $N's skewer!!$R", ch, 0, vict, TO_ROOM, NOTVICT);
      return 0;
    }

    //  act("$n jams $s weapon into $N!!", ch, 0, vict, TO_ROOM, NOTVICT);
    //  act("You jam your weapon in $N's heart!", ch, 0, vict, TO_CHAR, 0);
    //  act("$n's weapon is speared into you! Ouch!", ch, 0, vict, TO_VICT, 0);
    damadd = (int)(dam * 1.5);
    int retval = damage(ch, vict, damadd, wt, SKILL_SKEWER, weapon);
    if (SOMEONE_DIED(retval))
      return debug_retval(ch, vict, retval);
    // vict->removeHP(damadd);
    update_pos(vict);
    inform_victim(ch, vict, damadd);

    if (GET_POS(vict) != position_t::DEAD && number(0, 4999) == 1)
    { /* tiny chance of instakill */
      vict->setHP(-1, ch);
      ch->sendln("You impale your weapon through your opponent's chest!");
      act("$n's weapon blows through your chest sending your entrails flying for yards behind you.  Everything goes black...", ch, 0, vict, TO_VICT, 0);
      act("$n's weapon rips through $N's chest sending gore and entrails flying for yards!\r\n", ch, 0, vict, NOTVICT, 0);
      // duplicate message   act("$n is DEAD!!", vict, 0, 0, TO_ROOM, INVIS_NULL);
      send_to_char("You have been SKEWERED!!\n\r\n\r", vict);
      damage(ch, vict, 9999999, TYPE_UNDEFINED, SKILL_SKEWER, weapon);
      //      update_pos(vict);
      return eSUCCESS | eVICT_DIED;
    }
    return debug_retval(ch, vict, retval);
  }
  // if they're still here the skewer missed
  return eSUCCESS;
}

int do_behead_skill(Character *ch, Character *vict)
{
  int chance, percent;

  percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
  chance = number(0, 101);
  if (chance > (1.3 * percent))
  {
    percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
    chance = number(0, 101);
    if (chance > (2 * percent))
    {
      chance = number(0, 101);
      if (chance > (2 * percent))
      {
        chance = number(0, 101);
        if (chance > (2 * percent) && !DC::isSet(vict->immune, ISR_SLASH) && skill_success(ch, vict, SKILL_BEHEAD))
        {
          if (((vict->equipment[WEAR_NECK_1] && obj_index[vict->equipment[WEAR_NECK_1]->item_number].virt == 518) ||
               (vict->equipment[WEAR_NECK_2] && obj_index[vict->equipment[WEAR_NECK_2]->item_number].virt == 518)) &&
              !number(0, 1))
          { // tarrasque's leash..
            act("You attempt to behead $N, but your sword bounces of $S neckwear.", ch, 0, vict, TO_CHAR, 0);
            act("$n attempts to behead $N, but fails.", ch, 0, vict, TO_ROOM, NOTVICT);
            act("$n attempts to behead you, but cannot cut through your neckwear.", ch, 0, vict, TO_VICT, 0);
            return eSUCCESS;
          }
          if (IS_AFFECTED(vict, AFF_NO_BEHEAD))
          {
            act("$N deftly dodges your beheading attempt!", ch, 0, vict, TO_CHAR, 0);
            act("$N deftly dodges $n's attempt to behead $M!", ch, 0, vict, TO_ROOM, NOTVICT);
            act("You deftly avoid $n's attempt to lop your head off!", ch, 0, vict, TO_VICT, 0);
            return eSUCCESS;
          }
          act("You feel your life end as $n's sword SLICES YOUR HEAD OFF!", ch, 0, vict, TO_VICT, 0);
          act("You SLICE $N's head CLEAN OFF $S body!", ch, 0, vict, TO_CHAR, 0);
          act("$n cleanly slices $N's head off $S body!", ch, 0, vict, TO_ROOM, NOTVICT);
          vict->setHP(-20, ch);
          make_head(vict);
          group_gain(ch, vict);
          fight_kill(ch, vict, TYPE_CHOOSE, 0);
          return eSUCCESS | eVICT_DIED; /* Zero means kill it! */
          // it died..
        }
        else
        { /* You MISS the fucker! */
          act("You hear the SWOOSH sound of wind as $n's sword attempts to slice off your head!", ch, 0, vict, TO_VICT, 0);
          act("You miss your attempt to behead $N.", ch, 0, vict, TO_CHAR, 0);
          act("$N jumps back as $n makes an attempt to BEHEAD $M!", ch, 0, vict, TO_ROOM, NOTVICT);
          return eSUCCESS;
        }
      }
    }
  }
  return eFAILURE;
}

int do_execute_skill(Character *ch, Character *vict, int w_type)
{
  int chance, percent;

  percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
  chance = number(0, 101);
  if (chance > (1.3 * percent))
  {
    percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
    chance = number(0, 101);
    if (chance > (2 * percent))
    {
      chance = number(0, 101);
      if (chance > (2 * percent))
      {
        if (IS_AFFECTED(vict, AFF_NO_BEHEAD) ||
            (w_type == TYPE_SLASH && DC::isSet(vict->immune, ISR_SLASH)) ||
            (w_type == TYPE_PIERCE && DC::isSet(vict->immune, ISR_PIERCE)) ||
            (w_type == TYPE_CRUSH && DC::isSet(vict->immune, ISR_CRUSH)) ||
            (w_type == TYPE_BLUDGEON && DC::isSet(vict->immune, ISR_BLUDGEON)) ||
            (w_type == TYPE_WHIP && DC::isSet(vict->immune, ISR_WHIP)) ||
            (w_type == TYPE_STING && DC::isSet(vict->immune, ISR_STING)))
        {
          act("$N deftly dodges your mortal strike!", ch, 0, vict, TO_CHAR, 0);
          act("$N deftly dodges $n's mortal strike!", ch, 0, vict, TO_ROOM, NOTVICT);
          act("You deftly avoid $n's mortal strike!", ch, 0, vict, TO_VICT, 0);
          return eSUCCESS;
        }
        else if (!skill_success(ch, vict, SKILL_EXECUTE))
        {
          act("$N narrowly avoids your lethal blow as you attempt to thrust aside $S defenses!", ch, 0, vict, TO_CHAR, 0);
          act("You narrowly avoid a lethal blow as $n attempts to thrust aside your defenses!", ch, 0, vict, TO_VICT, 0);
          act("$N narrowly avoids $n's lethal blow as $e attempts to thrust aside $S defenses! ", ch, 0, vict, TO_ROOM, NOTVICT);
          return eSUCCESS;
        }
        else
        {
          act("$n quickly thrusts aside your defenses and strikes a fatal blow!", ch, 0, vict, TO_VICT, 0);
          act("You feel a flash of pain and your vision dims from $4red$R to $B$0black$R...", ch, 0, vict, TO_VICT, 0);
          vict->setHP(-20, ch);
          act("You quickly thrust aside $N's defenses and strike a fatal blow!", ch, 0, vict, TO_CHAR, 0);
          act("$n quickly thrusts aside $N's defenses and strikes a lethal blow!", ch, 0, vict, TO_ROOM, NOTVICT);
          if (w_type == TYPE_SLASH || w_type == TYPE_PIERCE)
          {
            if (number(0, 1))
            {
              act("$N's arm is neatly severed from $S battered body as it crumples to the ground.", ch, 0, vict, TO_CHAR, 0);
              act("$N's arm is neatly severed from $S battered body as it crumples to the ground.", ch, 0, vict, TO_ROOM, NOTVICT);
              make_arm(vict);
            }
            else
            {
              act("$N's leg is neatly severed from $S battered body as it crumples to the ground.", ch, 0, vict, TO_CHAR, 0);
              act("$N's leg is neatly severed from $S battered body as it crumples to the ground.", ch, 0, vict, TO_ROOM, NOTVICT);
              make_leg(vict);
            }
          }
          if (w_type == TYPE_CRUSH || w_type == TYPE_BLUDGEON)
          {
            act("$N's guts spill to the ground as $S body is split open like an overripe melon.", ch, 0, vict, TO_CHAR, 0);
            act("$N's guts spill to the ground as $S body is split open like an overripe melon.", ch, 0, vict, TO_ROOM, NOTVICT);
            make_bowels(vict);
          }
          if (w_type == TYPE_WHIP || w_type == TYPE_STING)
          {
            act("$N's blood pools on the ground as the remaining life seeps from $S body.", ch, 0, vict, TO_CHAR, 0);
            act("$N's blood pools on the ground as the remaining life seeps from $S body.", ch, 0, vict, TO_ROOM, NOTVICT);
            make_blood(vict);
          }
          group_gain(ch, vict);
          fight_kill(ch, vict, TYPE_CHOOSE, 0);
          return eSUCCESS | eVICT_DIED; /* Zero means kill it! */
        }
      }
    }
  }
  return eFAILURE;
}

void do_combatmastery(Character *ch, Character *vict, int weapon)
{
  if ((GET_CLASS(ch) != CLASS_WARRIOR) && ch->getLevel() < ARCHANGEL)
    return;
  if (IS_PC(vict) && vict->getLevel() >= IMMORTAL)
    return;
  if (!ch->equipment[weapon])
    return;
  if (!ch->has_skill(SKILL_COMBAT_MASTERY))
    return;
  if (ch->in_room != vict->in_room)
    return;

  int type = get_weapon_damage_type(ch->equipment[weapon]);
  if (type != TYPE_STING && type != TYPE_WHIP && type != TYPE_CRUSH && type != TYPE_BLUDGEON)
    return;

  if (!skill_success(ch, vict, SKILL_COMBAT_MASTERY))
    return;

  if (number(0, 8))
    return; // Chance lowered

  if (type == TYPE_STING)
  {
    if (!IS_AFFECTED(vict, AFF_BLIND))
    {
      struct affected_type af;
      af.type = SKILL_COMBAT_MASTERY;
      af.location = APPLY_HITROLL;
      af.modifier = vict->has_skill(SKILL_BLINDFIGHTING) ? skill_success(vict, 0, SKILL_BLINDFIGHTING) ? -10 : -20 : -20;
      af.duration = 1;
      af.bitvector = AFF_BLIND;
      affect_to_char(vict, &af);

      af.location = APPLY_AC;
      af.modifier = vict->has_skill(SKILL_BLINDFIGHTING) ? skill_success(vict, 0, SKILL_BLINDFIGHTING) ? +25 : 50 : 50;
      affect_to_char(vict, &af);

      act("Your attack stings $N's eyes, causing momentary blindness!", ch, 0, vict, TO_CHAR, 0);
      act("$n's attack stings your eyes, causing momentary blindness!", ch, 0, vict, TO_VICT, 0);
      act("$n's attack stings $N's eyes, causing momentary blindness!", ch, 0, vict, TO_ROOM, NOTVICT);
    }
  }
  if (type == TYPE_BLUDGEON || type == TYPE_CRUSH)
  {
    if (vict->getLevel() >= 90 || (IS_MOB(vict) && ISSET(vict->mobdata->actflags, ACT_HUGE)))
    {
      act("$N shakes off your crushing blow!", ch, 0, vict, TO_CHAR, 0);
      act("$N shakes off $n's crushing blow!", ch, 0, vict, TO_ROOM, NOTVICT);
      act("You shake off $n's crushing blow!", ch, 0, vict, TO_VICT, 0);
      return;
    }
    if (vict->getLevel() >= 90 || (IS_MOB(vict) && ISSET(vict->mobdata->actflags, ACT_SWARM)))
    {
      act("$N swarms around your crushing blow!", ch, 0, vict, TO_CHAR, 0);
      act("$N swarms around $n's crushing blow!", ch, 0, vict, TO_ROOM, NOTVICT);
      act("You swarm around $n's crushing blow!", ch, 0, vict, TO_VICT, 0);
      return;
    }
    if (vict->getLevel() >= 90 || (IS_MOB(vict) && ISSET(vict->mobdata->actflags, ACT_TINY)))
    {
      act("$N is so small, $E easily avoids your crushing blow!", ch, 0, vict, TO_CHAR, 0);
      act("$N easily avoids $n's slow, crushing blow!", ch, 0, vict, TO_ROOM, NOTVICT);
      act("You easily avoid $n's slow, crushing blow!", ch, 0, vict, TO_VICT, 0);
      return;
    }
    if (!DC::isSet(vict->combat, COMBAT_CRUSH_BLOW))
    {
      SET_BIT(vict->combat, COMBAT_CRUSH_BLOW);
      act("Your crushing blow causes $N's attacks to momentarily weaken!", ch, 0, vict, TO_CHAR, 0);
      act("$n's crushing blow causes your attacks to momentarily weaken!", ch, 0, vict, TO_VICT, 0);
      act("$n's crushing blow causes $N's attacks to momentarily weaken!", ch, 0, vict, TO_ROOM, NOTVICT);
    }
  }
  if (type == TYPE_WHIP && !affected_by_spell(ch, SKILL_CM_TIMER))
  {
    if (GET_POS(vict) > position_t::SITTING && !affected_by_spell(vict, SPELL_IRON_ROOTS))
    {
      vict->setSitting();
      SET_BIT(vict->combat, COMBAT_BASH2);
      WAIT_STATE(vict, DC::PULSE_VIOLENCE);
      act("Your whipping attack trips up $N and $E goes down!", ch, 0, vict, TO_CHAR, 0);
      act("$n's whipping attack trips you up, causing you to stumble and fall!", ch, 0, vict, TO_VICT, 0);
      act("$n's whipping attack trips up $N causing $M to stumble and fall!", ch, 0, vict, TO_ROOM, NOTVICT);

      struct affected_type af;
      af.type = SKILL_CM_TIMER;
      af.location = 0;
      af.modifier = 0;
      af.duration = number(3, 4);
      af.bitvector = -1;
      affect_to_char(ch, &af, DC::PULSE_VIOLENCE);
    }
  }

  return;
}

void raw_kill(Character *ch, Character *victim)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[100];
  int is_thief = 0;
  int death_room = 0;

  if (!victim)
  {
    logentry("Error in raw_kill()!  Null victim!", IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  if (DC::isSet(DC::getInstance()->world[victim->in_room].room_flags, ARENA))
  {
    fight_kill(ch, victim, TYPE_ARENA_KILL, 0);
    return;
  }

  if (IS_AFFECTED(victim, AFF_CHAMPION))
  {
    REMBIT(victim->affected_by, AFF_CHAMPION);
    do_champ_flag_death(victim);
  }
  if (ch && IS_NPC(victim) && IS_PC(ch) && ch->getLevel() >= IMMORTAL)
  {
    sprintf(buf, "%s killed %s.", GET_NAME(ch), victim->getNameC());
    special_log(buf);
  }

  // register my death with this zone's counter
  DC::incrementZoneDiedTick(DC::getInstance()->world[victim->in_room].zone);

  victim->setStanding();
  int retval = mprog_death_trigger(victim, ch);
  if (SOMEONE_DIED(retval))
    return;
  victim->setDead();

  if (GET_RACE(victim) == RACE_UNDEAD ||
      GET_RACE(victim) == RACE_GHOST ||
      GET_RACE(victim) == RACE_ELEMENT ||
      GET_RACE(victim) == RACE_ASTRAL ||
      GET_RACE(victim) == RACE_SLIME ||
      (IS_NPC(victim) && mob_index[victim->mobdata->nr].virt == 8))
    make_dust(victim);
  else
    make_corpse(victim);

  if (ch != nullptr && IS_AFFECTED(ch, AFF_GROUP))
  {
    Character *master = ch->master ? ch->master : ch;
    if (IS_PC(master))
    {
      if (IS_PC(victim))
      {
        master->player->grpplvl += victim->getLevel();
        master->player->group_pkills += 1;
      }
      master->player->group_kills += 1;
    }
  }

  if (IS_NPC(victim))
  {
    if (ch == victim)
    {
      if (ch == nullptr)
      {
        logf(0, LogChannels::LOG_BUG, "selfpurge on nullptr to nullptr");
      }
      else
      {
        // logf(0, LogChannels::LOG_BUG, "selfpurge on %s to %s", GET_NAME(ch), victim->getNameC());
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
    void release_message(Character * ch);
    void shatter_message(Character * ch);

    if (number(0, 99) < (victim->getLevel() / 10 + victim->player->golem->getLevel() / 5))
    { /* rk */
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "%s's golem lost a level!", victim->getNameC());
      logentry(buf, ANGEL, LogChannels::LOG_MORTAL);
      shatter_message(victim->player->golem);
      extract_char(victim->player->golem, true);
    }
    else
    { /* release */
      release_message(victim->player->golem);
      extract_char(victim->player->golem, false);
    }
  }
  victim->player->group_pkills = 0;
  victim->player->grpplvl = 0;
  victim->player->group_kills = 0;
  if (DC::isSet(victim->player->punish, PUNISH_SPAMMER))
    REMOVE_BIT(victim->player->punish, PUNISH_SPAMMER);
  if (affected_by_spell(victim, FUCK_PTHIEF))
  {
    is_thief = 1;
    affect_from_char(victim, FUCK_PTHIEF);
  }
  if (affected_by_spell(victim, FUCK_GTHIEF))
  {
    if (victim->getGold() > 0)
    {
      act("$n drops $s stolen booty!", victim, 0, 0, TO_ROOM, 0);
      obj_to_room(create_money(victim->getGold()), victim->in_room);
      victim->setGold(0);
      save_char_obj(victim);
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
  GET_COND(victim, FULL) = 0;
  GET_COND(victim, THIRST) = 0;

  if (victim && DC::isSet(victim->combat, COMBAT_BERSERK))
  {
    GET_AC(victim) -= 30;
    victim->combat = 0;
  }

  save_char_obj(victim);

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &i : character_list)
  {

    remove_memory(i, 'h', victim);
    if (IS_NPC(i) && (i->mobdata->fears))
      if (!strcmp(i->mobdata->fears, victim->getNameC()))
        remove_memory(i, 'f');
    if (IS_NPC(i) && !i->hunting.isEmpty())
      if (i->hunting == victim->getName())
        remove_memory(i, 't');
  }

  /* If we're still here we can thrash the victim */
  if (IS_PC(victim))
  { /* Only log player deaths */
    if (ch)
    {
      if (ch->mobdata)
      {
        sprintf(buf, "%s killed by %d (%s)", victim->getNameC(), mob_index[ch->mobdata->nr].virt,
                GET_NAME(ch));
      }
      else
      {
        sprintf(buf, "%s killed by %s", victim->getNameC(), GET_NAME(ch));
      }
    }
    else
    {
      sprintf(buf, "%s killed by [null killer]", victim->getNameC());
    }

    // notify the clan members - clan_death checks for null ch/vict
    clan_death(victim, ch);
    char log_buf[MAX_STRING_LENGTH] = {};
    sprintf(log_buf, "%s at %d", buf, DC::getInstance()->world[death_room].number);
    logentry(log_buf, ANGEL, LogChannels::LOG_MORTAL);

    // update stats
    GET_RDEATHS(victim) += 1;

    /* gods don't suffer from stat loss */
    if (victim->getLevel() < IMMORTAL && victim->getLevel() > 19)
    {
      /* New death system... dying is a BITCH!  */
      // thief + mob kill = stat loss
      // or got a bad roll

      if (is_thief)
        pir_stat_loss(victim, 100, true, is_thief);
      else if (victim->getLevel() > 20)
      {
        int chance = ch ? ch->getLevel() / 10 : 50 / 10;
        chance += victim->getLevel() / 2;
        if (victim->getLevel() >= 50)
        {
          chance += (int)(25.0 * (float)((float)(ch ? ch->getLevel() : 50) / 100.0) * (float)((float)(ch ? ch->getLevel() : 50) / 100.0));
          // An extra 1% for each level over 50.
          chance += victim->getLevel() - 50;
        }

        if (number(0, 99) < chance)
        {
          if (GET_RACE(victim) != RACE_TROLL)
          {
            GET_CON(victim) -= 1;
            victim->raw_con -= 1;
            send_to_char("*** You lose one constitution point ***\n\r", victim);
            if (IS_PC(victim))
            {
              sprintf(log_buf, "%s lost a con. ouch.", victim->getNameC());
              logentry(log_buf, SERAPH, LogChannels::LOG_MORTAL);
              victim->player->statmetas--;
            }
          }
          else
          {
            GET_DEX(victim) -= 1;
            victim->raw_dex -= 1;
            send_to_char("*** You lose one dexterity point ***\n\r", victim);
            if (IS_PC(victim))
            {
              sprintf(log_buf, "%s lost a dex. ouch.", victim->getNameC());
              logentry(log_buf, SERAPH, LogChannels::LOG_MORTAL);
              victim->player->statmetas--;
            }
          }
          pir_stat_loss(victim, chance, false, is_thief);
        }
      }
      check_maxes(victim); // Check if any skills got lowered because of
                           // stat loss. guild.cpp.
      // hmm
      if (GET_CON(victim) <= 4)
      {
        send_to_char("Your Constitution has reached 4...you are permanently dead!\n\r", victim);
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
        char name[100];
        strncpy(name, victim->getNameC(), 100);
        do_quit(victim, "", 666);

        remove_familiars(name, CONDEATH);
        remove_vault(name, CONDEATH);
        if (victim->clan)
        {
          remove_clan_member(victim->clan, victim);
        }
        remove_character(name, CONDEATH);

        sprintf(buf2, "%s permanently dies.", name);
        logentry(buf2, ANGEL, LogChannels::LOG_MORTAL);
        return;
      }
      else if (GET_INT(victim) <= 4)
      {
        send_to_char("Your Intelligence has reached 4...you are permanently dead!\n\r", victim);
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

        char name[100];
        strncpy(name, victim->getNameC(), 100);
        do_quit(victim, "", 666);

        remove_familiars(name, CONDEATH);
        remove_vault(name, CONDEATH);
        if (victim->clan)
        {
          remove_clan_member(victim->clan, victim);
        }
        remove_character(name, CONDEATH);

        sprintf(buf2, "%s sees tits.", name);
        logentry(buf2, ANGEL, LogChannels::LOG_MORTAL);
        return;
      }
      else if (GET_WIS(victim) <= 4)
      {
        send_to_char("Your Wisdom has reached 4...you are permanently dead!\n\r", victim);
        send_to_char("\r\n"
                     "    	The other stat deaths have alot fancier ASCII pics.\r\n"
                     "          =,    (\\_/)    ,=\r\n"
                     "           /`-'--(\")--'-'\\\r\n"
                     "          /     (___)     \\\r\n"
                     "         /.-.-./ \" \" \\.-.-.\\\r\n",
                     victim);

        char name[100];
        strncpy(name, victim->getNameC(), 100);
        do_quit(victim, "", 666);

        remove_familiars(name, CONDEATH);
        remove_vault(name, CONDEATH);
        if (victim->clan)
        {
          remove_clan_member(victim->clan, victim);
        }
        remove_character(name, CONDEATH);

        sprintf(buf2, "%s gets batted to death.", name);
        logentry(buf2, ANGEL, LogChannels::LOG_MORTAL);
        return;
      }
      else if (GET_STR(victim) <= 4)
      {
        send_to_char("Your Strength has reached 4...you are permanently dead!\n\r", victim);
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
        char name[100];
        strncpy(name, victim->getNameC(), 100);
        do_quit(victim, "", 666);

        remove_familiars(name, CONDEATH);
        remove_vault(name, CONDEATH);
        if (victim->clan)
        {
          remove_clan_member(victim->clan, victim);
        }
        remove_character(name, CONDEATH);

        sprintf(buf2, "%s goes to moose heaven.", name);
        logentry(buf2, ANGEL, LogChannels::LOG_MORTAL);
        return;
      }
      else if (GET_DEX(victim) <= 4 && GET_RACE(victim) == RACE_TROLL)
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

        char name[100];
        strncpy(name, victim->getNameC(), 100);
        do_quit(victim, "", 666);

        remove_familiars(name, CONDEATH);
        remove_vault(name, CONDEATH);
        if (victim->clan)
        {
          remove_clan_member(victim->clan, victim);
        }
        remove_character(name, CONDEATH);

        sprintf(buf2, "%s permanently dies the horrible dex-death.", name);
        logentry(buf2, ANGEL, LogChannels::LOG_MORTAL);
        return;
      }
    }

    float penalty = 1;
    if (ch)
      penalty += ch->getLevel() * .05;
    penalty = MIN(penalty, 2);
    GET_EXP(victim) = (int64_t)(GET_EXP(victim) / penalty);
  } // IS_PC
}

void group_gain(Character *ch, Character *victim)
{
  char buf[256];
  int32_t no_members = 0, total_levels = 0;
  int64_t share, total_share = 0;
  int64_t base_xp = 0, bonus_xp = 0;
  Character *leader, *highest, *tmp_ch;
  struct follow_type *f;

  if (is_pkill(ch, victim))
    return;
  if (ch == victim)
    return;
  if (IS_PC(victim))
    return;

  if (IS_NPC(ch) && !(IS_AFFECTED(ch, AFF_CHARM) || IS_AFFECTED(ch, AFF_FAMILIAR)))
    return; // non charmies/familiars get out

  // if i'm charmie/familiar and not grouped, give my master the credit if he's in room
  if (IS_NPC(ch) && ch->master && ch->in_room == ch->master->in_room)
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
    if (level_difference <= -51 && IS_PC(tmp_ch))
    {
      act("You are too low for this group.  You gain no experience.", tmp_ch, 0, 0, TO_CHAR, 0);

      tmp_ch = loop_followers(&f);
      continue;
    }

    // Charmies dont steal xp whether they're in a group or not
    if (IS_NPC(tmp_ch) && (IS_AFFECTED(tmp_ch, AFF_CHARM) || IS_AFFECTED(tmp_ch, AFF_FAMILIAR)))
    {
      tmp_ch = loop_followers(&f);
      continue;
    }

    /* calculate base XP value */
    base_xp = GET_EXP(victim);
    if (IS_AFFECTED(victim, AFF_CHARM))
    {
      share = 0;
      base_xp = 0;
      bonus_xp = 0;
    }
    /* calculate this character's share of the XP */
    else
    {
      share = scale_char_xp(tmp_ch, ch, victim, no_members, total_levels, highest->getLevel(), base_xp, &bonus_xp);
    }

    if (IS_AFFECTED(tmp_ch, AFF_CHAMPION))
      share = (int)((double)share * 1.10);
    sprintf(buf, "You receive %ld exps of %ld total.\r\n", share, base_xp + bonus_xp);
    tmp_ch->send(buf);
    gain_exp(tmp_ch, share);
    total_share += share;
    change_alignment(tmp_ch, victim);

    // this loops the followers (cut and pasted above)
    tmp_ch = loop_followers(&f);
  } while (tmp_ch);
  getAreaData(DC::getInstance()->world[victim->in_room].zone, mob_index[victim->mobdata->nr].virt, total_share, victim->getGold());
}

/* find the highest level present at the kill */
Character *get_highest_level_killer(Character *leader, Character *killer)
{
  struct follow_type *f;
  Character *highest = killer;

  /* check to see if the group leader was involved and outranks the killer */
  if (leader->in_room == killer->in_room && leader->getLevel() > killer->getLevel())
    highest = leader;

  /* loop through all groupies */
  for (f = leader->followers; f; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) &&     // if grouped
        f->follower->in_room == killer->in_room && // and in the room
        !IS_MOB(f->follower))
    {
      if (f->follower->getLevel() > highest->getLevel())
        highest = f->follower;
    }
  }
  return (highest);
}

/* count the number of group members eligible for XP from a kill */
int32_t count_xp_eligibles(Character *leader, Character *killer,
                           int32_t highest_level, int32_t *total_levels)
{
  struct follow_type *f;
  int32_t num_eligibles = 0;

  *total_levels = 0;

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
        !IS_MOB(f->follower) &&
        (highest_level - f->follower->getLevel()) < 25)
    {
      num_eligibles += 1;
      *total_levels += f->follower->getLevel();
    }
  }
  return (num_eligibles);
}

/* scale character XP based on various factors */
int64_t scale_char_xp(Character *ch, Character *killer, Character *victim,
                      int32_t no_killers, int32_t total_levels, int32_t highest_level,
                      int64_t base_xp, int64_t *bonus_xp)
{
  int32_t scaled_share;
  *bonus_xp = 0;

  scaled_share = ((base_xp + *bonus_xp) * ch->getLevel()) / total_levels;

  if (scaled_share > (ch->getLevel() * 8000))
    scaled_share = ch->getLevel() * 8000;

  return (scaled_share);
}

/* advance to the next follower in the list */
Character *loop_followers(struct follow_type **f)
{
  Character *tmp_ch;

  // this loops the followers
  if (*f)
  {
    tmp_ch = (*f)->follower;
    *f = (*f)->next;
  }
  else
    tmp_ch = nullptr;

  return (tmp_ch);
}

char *elem_type[] =
    {
        "$B$4stream of flame$R",
        "$B$3shards of ice$R",
        "$B$5bolt of energy$R",
        "$B$0stone fist$R"};

void dam_message(int dam, Character *ch, Character *victim,
                 int w_type, int32_t modifier)
{
  static const char *attack_table[] =
      {
          "hit", "pound", "pierce", "slash", "whip", "claw",
          "bite", "sting", "crush"};

  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
  char *vs, *vp, *vx;
  const char *attack = 0;
  char punct;
  char modstring[200];
  char endstring[200];

  if (0 == dam && DC::isSet(modifier, COMBAT_MOD_IGNORE))
  {
    sprintf(buf1, "$n's pitiful attack is ignored by $N!");
    sprintf(buf2, "Your pitiful attack is completely ignored by $N!");
    sprintf(buf3, "You ignore $n's pitiful attack.");
    act(buf1, ch, nullptr, victim, TO_ROOM, NOTVICT);
    act(buf2, ch, nullptr, victim, TO_CHAR, 0);
    act(buf3, ch, nullptr, victim, TO_VICT, 0);
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
    if (((unsigned)w_type) >= sizeof(attack_table))
    {
      logentry("Dam_message: bad w_type", ANGEL, LogChannels::LOG_BUG);
      w_type = 0;
    }
  }
  else
  {
    attack = "$B$4flaming slash$R";
  }

  // Custom damage messages.
  if (IS_NPC(ch))
    switch (mob_index[ch->mobdata->nr].virt)
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

  if (DC::isSet(modifier, COMBAT_MOD_FRENZY))
  {
    strcpy(modstring, "frenzied ");
  }
  else
    *modstring = '\0';

  if (dam > 0)
  {
    if (DC::isSet(modifier, COMBAT_MOD_SUSCEPT))
    {
      strcpy(endstring, " doing extra damage");
    }

    if (DC::isSet(modifier, COMBAT_MOD_RESIST))
    {
      strcpy(endstring, " but is resisted");
    }
    else
      *endstring = '\0';
  }
  else
    *endstring = '\0';

  char shield[MAX_INPUT_LENGTH];
  char dammsg[MAX_INPUT_LENGTH];
  dammsg[0] = '\0';

  if (dam > 0)
  {
    switch (number(0, 3))
    {
    case 0:
      sprintf(dammsg, " causing $B%d $Rdamage", dam);
      break;
    case 1:
      sprintf(dammsg, " delivering $B%d$R damage", dam);
      break;
    case 2:
      sprintf(dammsg, " inflicting $B%d$R damage", dam);
      break;
    case 3:
      sprintf(dammsg, " dealing $B%d$R damage", dam);
      break;
    }
  }

  if (DC::isSet(modifier, COMBAT_MOD_REDUCED))
  {
    if (GET_CLASS(victim) == CLASS_MONK)
    {
      switch (number(0, 3))
      {
      case 0:
        sprintf(shield, "shin");
        break;
      case 1:
        sprintf(shield, "hand");
        break;
      case 2:
        sprintf(shield, "foot");
        break;
      case 3:
        sprintf(shield, "forearm");
        break;
      default:
        sprintf(shield, "error");
        break;
      }
    }
    else if (victim->equipment[WEAR_SHIELD])
      sprintf(shield, "%s", victim->equipment[WEAR_SHIELD]->short_description);

    if (GET_CLASS(victim) == CLASS_MONK)
    {
      if (w_type == 0)
      {
        if (!attack)
          attack = races[GET_RACE(ch)].unarmed;
        sprintf(buf1, "$n's %s %s $N%s| as it deflects off $S %s%c", attack, vp, vx, shield, punct);
        sprintf(buf2, "You %s $N%s%s as $E raises $S %s to deflect your %s%c", vs, vx, IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", shield, attack, punct);
        sprintf(buf3, "$n %s you%s%s as you deflect $s %s with your %s%c", vp, vx, IS_PC(victim) && DC::isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", attack, shield, punct);
      }
      else
      {
        if (!attack)
          attack = attack_table[w_type];
        sprintf(buf1, "$n's %s %s $N%s| as it deflects off $S %s%c", attack, vp, vx, shield, punct);
        sprintf(buf2, "You %s $N%s%s as $E raises $S %s to deflect your %s%c", vs, vx, IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", shield, attack, punct);
        sprintf(buf3, "$n %s you%s%s as you deflect $s %s with your %s%c", vp, vx, IS_PC(victim) && DC::isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", attack, shield, punct);
      }
    }
    else if (victim->has_skill(SKILL_TUMBLING))
    {
      if (number(0, 1))
      {
        sprintf(buf1, "$N leaps away from $n's strike, managing to avoid all but a scratch|.");
        sprintf(dammsg, " for $B%d$R damage", dam);
        sprintf(buf2, "$N leaps away from your strike, managing to avoid all but a scratch%s.", IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "");
        sprintf(buf3, "You leap away from $n's strike, managing to avoid all but a scratch%s.", IS_PC(victim) && DC::isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "");
      }
      else
      {
        sprintf(buf1, "$N's roll to the side comes a moment too late as $n still manages to land a glancing blow|.");
        sprintf(dammsg, ", dealing $B%d$R damage", dam);
        sprintf(buf2, "$N's roll to the side comes a moment too late as you still manages to land a glancing blow%s.", IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "");
        sprintf(buf3, "Your roll to the side comes a moment too late as $n still manages to land a glancing blow%s.", IS_PC(victim) && DC::isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "");
      }
    }
    else
    {
      if (w_type == 0)
      {
        if (!attack)
          attack = races[GET_RACE(ch)].unarmed;
        sprintf(buf1, "$n's %s %s $N%s| as it strikes $S %s%c", attack, vp, vx, shield, punct);
        sprintf(buf2, "You %s $N%s%s as $E raises $S %s to deflect your %s%c", vs, vx, IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", shield, attack, punct);
        sprintf(buf3, "$n %s you%s%s as you deflect $s %s with %s%c", vp, vx, IS_PC(victim) && DC::isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", attack, shield, punct);
      }
      else
      {
        if (!attack)
          attack = attack_table[w_type];
        sprintf(buf1, "$n's %s %s $N%s| as it strikes $S %s%c", attack, vp, vx, shield, punct);
        sprintf(buf2, "You %s $N%s%s as $E raises $S %s to deflect your %s%c", vs, vx, IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", shield, attack, punct);
        sprintf(buf3, "$n %s you%s%s as you deflect $s %s with %s%c", vp, vx, IS_PC(victim) && DC::isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", attack, shield, punct);
      }
    }
  }
  else
  {
    if (w_type == 0)
    {
      if (!attack)
        attack = races[GET_RACE(ch)].unarmed;
      int a;
      if (IS_NPC(ch) && (a = mob_index[ch->mobdata->nr].virt) < 92 && a > 87)
        attack = elem_type[a - 88];
      sprintf(buf1, "$n's %s%s %s $N%s|%c", modstring, attack, vp, vx, punct);
      sprintf(buf2, "Your %s%s %s $N%s%s%c", modstring, attack, vp, vx, IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", punct);
      sprintf(buf3, "$n's %s%s %s you%s%s%c", modstring, attack, vp, vx, IS_PC(victim) && DC::isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", punct);
    }
    else
    {
      if (!attack)
        attack = attack_table[w_type];
      sprintf(buf1, "$n's %s%s %s $N%s|%c", modstring, attack, vp, vx, punct);
      sprintf(buf2, "Your %s%s %s $N%s%s%c", modstring, attack, vp, vx, IS_PC(ch) && DC::isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", punct);
      sprintf(buf3, "$n's %s%s %s you%s%s%c", modstring, attack, vp, vx, IS_PC(victim) && DC::isSet(victim->player->toggles, Player::PLR_DAMAGE) ? dammsg : "", punct);
    }
  }
  //   act(buf1, ch, nullptr, victim, TO_ROOM, NOTVICT);
  send_damage(buf1, ch, 0, victim, dammsg, 0, TO_ROOM);
  act(buf2, ch, nullptr, victim, TO_CHAR, 0);
  act(buf3, ch, nullptr, victim, TO_VICT, 0);
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm(Character *ch, Character *victim)
{
  class Object *obj;

  if (victim->equipment[WIELD] == nullptr)
    return;
  if (ch->equipment[WIELD] == nullptr)
    return;

  if (affected_by_spell(victim, SPELL_PARALYZE))
  {
    ch->sendln("Their paralyzed fingers are gripping the weapon too tightly.");
    return;
  }
  if (DC::isSet(victim->combat, COMBAT_BERSERK))
  {
    ch->sendln("In their enraged state, there's no chance they'd let go of their weapon!");
    return;
  }
  act("$B$n disarms you and sends your weapon flying!$R", ch, nullptr, victim, TO_VICT, 0);
  act("You disarm $N and send $S weapon flying!", ch, nullptr, victim, TO_CHAR, 0);
  act("$n disarms $N and sends $S weapon flying!", ch, nullptr, victim, TO_ROOM, NOTVICT);

  // all disarms go to inventory right now -pir
  //  if (IS_PC(ch)) {
  obj = unequip_char(victim, WIELD);
  obj_to_char(obj, victim);
  if (victim->equipment[SECOND_WIELD])
  {
    obj = unequip_char(victim, SECOND_WIELD);
    equip_char(victim, obj, WIELD);
  }
  recheck_height_wears(victim);
  return;
  //  }

  // we never get here cause of above code

  obj = unequip_char(victim, WIELD);
  /* If it's gl make it go to inventory. Morc. */
  if (DC::isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    obj_to_char(obj, victim);
  else
    obj_to_room(obj, victim->in_room);

  if (victim->equipment[SECOND_WIELD])
  {
    obj = unequip_char(victim, SECOND_WIELD);
    equip_char(victim, obj, WIELD);
  }
}

/*
 * Trip a creature.
 * Caller must check for successful attack.
 */
void trip(Character *ch, Character *victim)
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

  return;
}

#define PKILL_COUNT_LIMIT 20

// 'ch' can be null
// do_pkill should never be called directly, only through "fight_kill"
void do_pkill(Character *ch, Character *victim, int type, bool vict_is_attacker)
{
  int num;
  char killer_message[MAX_STRING_LENGTH];
  //  Character *i = 0;
  struct affected_type *af, *afpk;

  void move_player_home(Character * victim);
  num = number(1, 1000);

  if (!victim)
  {
    logentry("Null victim sent to do_pkill.", IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  if (ch)
  {
    set_cantquit(ch, victim);
  }
  extern void pk_check(Character * ch, Character * victim);
  pk_check(ch, victim);
  // Kill charmed mobs outright
  if (IS_NPC(victim))
  {
    fight_kill(ch, victim, TYPE_RAW_KILL, 0);
    return;
  }

  if (affected_by_spell(victim, FUCK_PTHIEF))
  {
    victim->setMove(2);
    fight_kill(ch, victim, TYPE_RAW_KILL, 0);
    return;
  }

  if (affected_by_spell(victim, FUCK_GTHIEF))
  {
    victim->setMove(2);
    if (victim->getGold() > 0)
    {
      act("$N drops $S stolen booty!", ch, 0, victim, TO_ROOM, NOTVICT);
      obj_to_room(create_money(victim->getGold()), victim->in_room);
      victim->setGold(0);
      save_char_obj(victim);
    }
  }
  // Make sure barbs get their ac back
  if (DC::isSet(victim->combat, COMBAT_BERSERK))
  {
    REMOVE_BIT(victim->combat, COMBAT_BERSERK);
    GET_AC(victim) -= 30;
  }

  if (type == KILL_POISON && affected_by_spell(victim, SPELL_POISON)->modifier > 0)
  {
    const auto &character_list = DC::getInstance()->character_list;
    for (const auto &findchar : character_list)
    {
      if (findchar == affected_by_spell(victim, SPELL_POISON)->origin)
        ch = findchar;
    }
  }

  for (af = victim->affected; af; af = afpk)
  {
    afpk = af->next;
    if (af->type != FUCK_CANTQUIT &&
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
  GET_COND(victim, DRUNK) = 0;
  GET_COND(victim, FULL) = 0;
  GET_COND(victim, THIRST) = 0;

  save_char_obj(victim);

  // have to be level 20 and linkalive to count as a pkill and not yourself
  if (ch != nullptr)
  {
    if (type == KILL_POTATO)
      sprintf(killer_message, "\n\r##%s just got POTATOED!!\n\r", victim->getNameC());
    else if (type == KILL_MORTAR)
      sprintf(killer_message, "\n\r##%s just got a FIRE IN THE HOLE!!\n\r", victim->getNameC());
    else if (type == KILL_POISON)
      sprintf(killer_message, "\n\r##%s has perished from %s's POISON!\n\r", victim->getNameC(), GET_NAME(ch));
    else if (!str_cmp(GET_NAME(ch), victim->getNameC()))
      sprintf(killer_message, "");
    //    sprintf(killer_message,"\n\r##%s just commited SUICIDE!\n\r", victim->getNameC());
    else if (victim->getLevel() < PKILL_COUNT_LIMIT || ch == victim)
      // sprintf(killer_message,"\n\r##%s just DIED!\n\r", victim->getNameC());
      // sprintf(killer_message,"\n\r##%s was just introduced to the warm hospitality of Dark Castle!!\n\r", victim->getNameC());
      sprintf(killer_message, "");
    else if (num == 1000)
      sprintf(killer_message, "\n\r##%s was just ANALLY PROBED by %s!\n\r", victim->getNameC(), GET_NAME(ch));
    else if (IS_AFFECTED(ch, AFF_FAMILIAR) && ch->master)
      sprintf(killer_message, "\n\r##%s was just DEFEATED in battle by %s's familiar!\n\r",
              victim->getNameC(), GET_NAME(ch->master));
    else if (IS_AFFECTED(ch, AFF_CHARM) && ch->master)
      sprintf(killer_message, "\n\r##%s was just DEFEATED in battle by %s's charmie!\n\r",
              victim->getNameC(), GET_NAME(ch->master));
    else if (ch->in_room == real_room(START_ROOM))
      sprintf(killer_message, "\n\r##%s was just PINGED by %s!\n\r",
              victim->getNameC(), GET_NAME(ch));
    else if (ch->in_room == real_room(SECOND_START_ROOM))
      sprintf(killer_message, "\n\r##%s was just PONGED by %s!\n\r",
              victim->getNameC(), GET_NAME(ch));
    else if (IS_ANONYMOUS(ch))
      sprintf(killer_message, "\n\r##%s was just DEFEATED in battle by %s!\n\r",
              victim->getNameC(), GET_NAME(ch));
    else if (ch->getLevel() > MORTAL)
      sprintf(killer_message, "\n\r##%s was just SMITED...er..SMOTED..err PKILLED by %s!\n\r", victim->getNameC(), GET_NAME(ch));
    else if (type == KILL_BINGO)
      sprintf(killer_message, "\n\r##%s was just BINGOED by %s!\n\r",
              victim->getNameC(), GET_NAME(ch));
    else if (type == SPELL_CONSECRATE)
      sprintf(killer_message, "\n\r##%s was just slain by %s's CONSECRATION!\n\r", victim->getNameC(), GET_NAME(ch));
    else if (type == SPELL_DESECRATE)
      sprintf(killer_message, "\n\r##%s was just slain by %s's DESECRATION!\n\r", victim->getNameC(), GET_NAME(ch));
    else
      switch (GET_CLASS(ch))
      {
      case CLASS_MAGIC_USER:
        sprintf(killer_message, "\n\r##%s was just FRIED by %s's magic!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      case CLASS_CLERIC:
        sprintf(killer_message, "\n\r##%s was just BANISHED by %s's holiness!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      case CLASS_THIEF:
        sprintf(killer_message, "\n\r##%s was just ASSASSINATED by %s!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      case CLASS_WARRIOR:
        sprintf(killer_message, "\n\r##%s was just SLAIN by %s's might!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      case CLASS_ANTI_PAL:
        sprintf(killer_message, "\n\r##%s was just CONSUMED by %s's darkness!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      case CLASS_PALADIN:
        sprintf(killer_message, "\n\r##%s was just VANQUISHED by %s's goodness!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      case CLASS_BARBARIAN:
        sprintf(killer_message, "\n\r##%s was just SHREDDED by %s's crazed fury!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      case CLASS_MONK:
        sprintf(killer_message, "\n\r##%s was just SHATTERED by %s's karma!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      case CLASS_RANGER:
        sprintf(killer_message, "\n\r##%s was just PENETRATED by %s's wood!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      case CLASS_BARD:
        sprintf(killer_message, "\n\r##%s was just MUTED by %s's snazzy rhythm!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      case CLASS_DRUID:
        sprintf(killer_message, "\n\r##%s was just VIOLATED by %s's woodland friends!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      default:
        sprintf(killer_message, "\n\r##%s was just DEFEATED in battle by %s!\n\r",
                victim->getNameC(), GET_NAME(ch));
        break;
      }
    level_diff_t level_spread;
    // have to be level 20 and linkalive to count as a pkill and not yourself
    // (we check earlier to make sure victim isn't a mob)
    // now with tav/meta pkilling not adding to your score
    if (!IS_MOB(ch)
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
          Character *master = ch->master ? ch->master : ch;
          if (IS_PC(master))
          {
            if (IS_PC(victim))
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
            Character *master = ch->master->master ? ch->master->master : ch->master;
            if (IS_PC(master))
            {
              if (IS_PC(victim))
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
            Character *master = ch->master->master ? ch->master->master : ch->master;
            if (IS_PC(master))
            {
              if (IS_PC(victim))
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
      sprintf(killer_message, "\n\r##%s just DROWNED!\n\r", victim->getNameC());
    else if (type == KILL_POTATO)
      sprintf(killer_message, "\n\r##%s just got POTATOED!!\n\r", victim->getNameC());
    else if (type == KILL_POISON)
      sprintf(killer_message, "\n\r##%s has perished from POISON!\n\r", victim->getNameC());
    else if (type == KILL_FALL)
      sprintf(killer_message, "\n\r##%s has FALLEN to death!\n\r", victim->getNameC());
    else if (type == KILL_BATTER)
      sprintf(killer_message, "\n\r##That's using your head! %s just died attempting to batter a door!\n\r", victim->getNameC());
    else
      sprintf(killer_message, "\n\r##%s just DIED!\n\r", victim->getNameC());
  }

  send_info(killer_message);

  if (IS_AFFECTED(victim, AFF_CHAMPION) && ch && ch != victim)
  {
    REMBIT(victim->affected_by, AFF_CHAMPION);
    Object *obj = nullptr;
    if (!(obj = get_obj_in_list_num(real_object(CHAMPION_ITEM), victim->carrying)))
    {
      logentry("Champion without the flag, no bueno amigo!", IMMORTAL, LogChannels::LOG_BUG);
      return;
    }
    if (IS_NPC(ch) && ch->master)
    {
      if (ch->master->in_room >= 1900 || ch->master->in_room <= 1999 || DC::isSet(DC::getInstance()->world[ch->master->in_room].room_flags, CLAN_ROOM))
      {
        SETBIT(victim->affected_by, AFF_CHAMPION);
        sprintf(killer_message, "##%s didn't deserve to become the new Champion, it remains %s!\n\r", GET_NAME(ch->master), victim->getNameC());
      }
      else
      {
        move_obj(obj, ch->master);
        SETBIT(ch->master->affected_by, AFF_CHAMPION);
        sprintf(killer_message, "##%s has become the new Champion!\n\r", GET_NAME(ch->master));
      }
    }
    else
    {
      move_obj(obj, ch);
      SETBIT(ch->affected_by, AFF_CHAMPION);
      sprintf(killer_message, "##%s has become the new Champion!\n\r", GET_NAME(ch));
    }
    send_info(killer_message);
  }
}

// 'ch' can be null
void arena_kill(Character *ch, Character *victim, int type)
{
  void remove_nosave(Character * vict);

  char killer_message[MAX_STRING_LENGTH];
  //  Character *i;
  clan_data *ch_clan = nullptr;
  clan_data *victim_clan = nullptr;
  int eliminated = 1;
  void move_player_home(Character * victim);

  if (!victim)
  {
    logentry("Null victim sent to do_pkill.", IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  // Kill mobs outright
  if (IS_NPC(victim))
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
  if (ch && arena.type == CHAOS)
  {
    if (ch && ch->clan && ch->getLevel() < IMMORTAL)
      ch_clan = get_clan(ch);
    if (victim->clan && victim->getLevel() < IMMORTAL)
      victim_clan = get_clan(victim);

    if (type == KILL_BINGO)
    {
      sprintf(killer_message, "\n\r## %s [%s] just BINGOED %s [%s] in the arena!\n\r",
              ((IS_NPC(ch) && ch->master) ? GET_NAME(ch->master) : GET_NAME(ch)), get_clan_name(ch_clan),
              victim->getNameC(), get_clan_name(victim_clan));
    }
    else
    {
      sprintf(killer_message, "\n\r## %s [%s] just SLAUGHTERED %s [%s] in the arena!\n\r",
              ((IS_NPC(ch) && ch->master) ? GET_NAME(ch->master) : GET_NAME(ch)), get_clan_name(ch_clan),
              victim->getNameC(), get_clan_name(victim_clan));
    }

    logf(IMMORTAL, LogChannels::LOG_ARENA, "%s [%s] killed %s [%s]",
         ((IS_NPC(ch) && ch->master) ? GET_NAME(ch->master) : GET_NAME(ch)), get_clan_name(ch_clan),
         victim->getNameC(), get_clan_name(victim_clan));
  }
  else if (ch)
  {
    if (type == KILL_POTATO)
      sprintf(killer_message, "\n\r## %s just got POTATOED in the arena!\n\r", GET_SHORT(victim));
    else if (type == KILL_MASHED)
      sprintf(killer_message, "\n\r## %s just got MASHED in the potato arena!\n\r", GET_SHORT(victim));
    else
    {
      if (type == KILL_BINGO)
      {
        sprintf(killer_message, "\n\r## %s just BINGOED %s in the arena!\n\r",
                (IS_NPC(ch) && (ch->master) ? GET_SHORT(ch->master) : GET_SHORT(ch)), GET_SHORT(victim));
      }
      else
      {
        sprintf(killer_message, "\n\r## %s just SLAUGHTERED %s in the arena!\n\r",
                (IS_NPC(ch) && (ch->master) ? GET_SHORT(ch->master) : GET_SHORT(ch)), GET_SHORT(victim));
      }
    }
  }
  else
  {
    if (type == KILL_POTATO)
      sprintf(killer_message, "\n\r## %s just got POTATOED in the arena!\n\r", GET_SHORT(victim));
    else if (type == KILL_MASHED)
      sprintf(killer_message, "\n\r## %s just got MASHED in the potato arena!\n\r", GET_SHORT(victim));
    else
      sprintf(killer_message, "\n\r## %s just DIED in the arena!\n\r", GET_SHORT(victim));
  }
  send_info(killer_message);

  if (ch && victim && (arena.type == PRIZE || arena.type == CHAOS))
  {
    logf(IMMORTAL, LogChannels::LOG_ARENA, "%s killed %s", GET_NAME(ch), victim->getNameC());
  }

  // if it's a chaos, see if the clan was eliminated
  if (victim && arena.type == CHAOS && victim_clan)
  {
    const auto &character_list = DC::getInstance()->character_list;
    for (const auto &tmp : character_list)
    {

      if (DC::isSet(DC::getInstance()->world[tmp->in_room].room_flags, ARENA))
        if (victim->clan == tmp->clan && victim != tmp && tmp->getLevel() < IMMORTAL)
          eliminated = 0;
    }
    if (eliminated)
    {
      sprintf(killer_message, "## [%s] was just eliminated from the chaos!\n\r", get_clan_name(victim_clan));
      send_info(killer_message);
      logf(IMMORTAL, LogChannels::LOG_ARENA, "## [%s] was just eliminated from the chaos!", get_clan_name(victim_clan));
    }
  }

  victim->sendln("You have been completely healed.");
  victim->setResting();
  victim->fillHP();
  GET_MANA(victim) = GET_MAX_MANA(victim);
  victim->setMove(GET_MAX_MOVE(victim));
  GET_KI(victim) = GET_MAX_KI(victim);

  if (ch)
    ch->combat = 0; // remove all combat effects

  remove_active_potato(victim);
  save_char_obj(victim);
}

int is_stunned(Character *ch)
{
  if (DC::isSet(ch->combat, COMBAT_STUNNED))
    return true;
  if (DC::isSet(ch->combat, COMBAT_STUNNED2))
    return true;
  if (DC::isSet(ch->combat, COMBAT_SHOCKED))
    return true;
  if (DC::isSet(ch->combat, COMBAT_SHOCKED2))
    return true;
  return false;
}

int can_attack(Character *ch)
{
  if ((ch->in_room >= 0 && ch->in_room <= top_of_world) &&
      DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, ARENA) && ArenaIsOpen())
  {
    send_to_char("Wait until it closes!\n\r", ch);
    return false;
  }

  if ((ch->in_room >= 0 && ch->in_room <= top_of_world) &&
      DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, ARENA) && arena.type == POTATO)
  {
    send_to_char("You can't attack in a potato arena, go find a potato would ya?!\n\r", ch);
    return false;
  }

  if (IS_AFFECTED(ch, AFF_PARALYSIS))
    return false;
  if (is_stunned(ch))
    return false;

  return true;
}

int can_be_attacked(Character *ch, Character *vict)
{
  /* this will happen sometimes, no need to log it */
  if (!ch || !vict)
    return false;

  if (vict->in_room == DC::NOWHERE || vict->in_room >= top_of_world ||
      ch->in_room == DC::NOWHERE || ch->in_room >= top_of_world)
    return false;

  // Ch should not be able to attack a wizinvis immortal player
  if (IS_PC(vict) && ch->getLevel() < vict->player->wizinvis)
    return false;

  if (IS_NPC(vict))
    if (ISSET(vict->mobdata->actflags, ACT_NOATTACK))
    {
      ch->sendln("Due to heavy magics, they cannot be attacked.");
      return false;
    }

  // Prize Arena
  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, ARENA) && arena.type == PRIZE && IS_PC(ch) && IS_PC(vict))
  {

    if (ch->fighting && ch->fighting != vict)
    {
      ch->sendln("You are already fighting someone.");
      logf(IMMORTAL, LogChannels::LOG_ARENA, "%s, whom was fighting %s was prevented from attacking %s.",
           GET_NAME(ch), GET_NAME(ch->fighting), GET_NAME(vict));
      return false;
    }
    else if (vict->fighting && vict->fighting != ch)
    {
      ch->sendln("They are already fighting someone.");
      logf(IMMORTAL, LogChannels::LOG_ARENA, "%s was prevented from attacking %s who was fighting %s.",
           GET_NAME(ch), GET_NAME(vict), GET_NAME(vict->fighting));
      return false;
    }
  }

  // Clan Chaos Arena
  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, ARENA) && arena.type == CHAOS && IS_PC(ch) && IS_PC(vict))
  {
    if (ch->fighting && ch->fighting != vict && !ARE_CLANNED(ch->fighting, vict))
    {
      ch->sendln("You are already fighting someone from another clan.");
      logf(IMMORTAL, LogChannels::LOG_ARENA, "%s [%s], whom was fighting %s [%s] was prevented from attacking %s [%s].",
           GET_NAME(ch), get_clan_name(ch), GET_NAME(ch->fighting), get_clan_name(ch->fighting), GET_NAME(vict), get_clan_name(vict));
      return false;
    }
    else if (vict->fighting && vict->fighting != ch && !ARE_CLANNED(vict->fighting, ch))
    {
      ch->sendln("They are already fighting someone.");
      logf(IMMORTAL, LogChannels::LOG_ARENA, "%s [%s] was prevented from attacking %s [%s] who was fighting %s [%s].",
           GET_NAME(ch), get_clan_name(ch), GET_NAME(vict), get_clan_name(vict), GET_NAME(vict->fighting), get_clan_name(vict->fighting));
      return false;
    }
  }

  // Golem cannot attack players
  if (IS_NPC(ch) && mob_index[ch->mobdata->nr].virt == 8 && IS_PC(vict))
    return false;

  if (IS_NPC(vict))
  {
    if ((IS_AFFECTED(vict, AFF_FAMILIAR) || mob_index[vict->mobdata->nr].virt == 8 || affected_by_spell(vict, SPELL_CHARM_PERSON) || ISSET(vict->affected_by, AFF_CHARM)) &&
        vict->master &&
        vict->fighting != ch &&
        !(IS_AFFECTED(vict->master, AFF_CANTQUIT) || IS_AFFECTED(vict->master, AFF_CHAMPION)) &&
        DC::isSet(DC::getInstance()->world[vict->in_room].room_flags, SAFE))
    {
      ch->sendln("No fighting permitted in a safe room.");
      return false;
    }
    // Any other mob can be attacked at any time
    return true;
  }

  if (IS_PC(ch) && IS_PC(vict) && ch->getLevel() < 5)
  {
    send_to_char("You are too new in this realm to make enemies!\n\r", ch);
    return false;
  }

  if (IS_AFFECTED(vict, AFF_CANTQUIT) || affected_by_spell(vict, FUCK_PTHIEF) || affected_by_spell(vict, FUCK_GTHIEF) || IS_AFFECTED(vict, AFF_CHAMPION))
    return true;

  if (IS_PC(ch) && vict->getLevel() < 5)
  {
    act("The magic of the MUD school is protecting $M from harm.", ch, 0, vict, TO_CHAR, 0);
    return false;
  }

  if (IS_NPC(ch) && IS_PC(vict) && IS_AFFECTED(ch, AFF_CHARM))
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
      do_say(ch, "I'm sorry master, I cannot do that.", CMD_DEFAULT);
      return eFAILURE;
    }
  }

  if (DC::isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    /* Allow the NPCs to continue fighting */
    if (IS_NPC(ch))
    {
      if (ch->fighting == vict)
        return true;
    }
    /* Allow players to continue fighting if they have a cantquit */
    if (IS_AFFECTED(ch, AFF_CANTQUIT) || IS_AFFECTED(ch, AFF_CHAMPION))
    {
      if (ch->fighting == vict)
        return true;
    }
    /* Imps ignore safe flags  */
    if (IS_PC(ch) && (ch->getLevel() == IMPLEMENTER))
    {
      send_to_char("There is no safe haven from an angry IMPLEMENTER!\n\r", vict);
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

int weapon_spells(Character *ch, Character *vict, int weapon)
{
  int i, current_affect, chance, percent, retval;
  Object *weap;

  if (!ch || !vict)
  {
    logentry("Null ch or vict sent to weapon spells!", -1, LogChannels::LOG_BUG);
    return eFAILURE | eINTERNAL_ERROR;
  }
  if (!weapon)
    return eFAILURE;

  if (!can_attack(ch) || !can_be_attacked(ch, vict))
    return eFAILURE;

  if ((ch->in_room != vict->in_room && weapon != ITEM_MISSILE) || GET_POS(vict) == position_t::DEAD)
    return eFAILURE;

  if (!ch->equipment[weapon] && weapon != ITEM_MISSILE)
    return eFAILURE;

  int wep_skill = 40;

  if (weapon == ITEM_MISSILE)
    weap = get_obj_in_list_vis(ch, "arrow", ch->carrying);
  else
    weap = ch->equipment[weapon];

  if (!weap)
    return eFAILURE;

  for (i = 0; i < weap->num_affects; i++)
  {
    /* It's possible the victim has fled or died */
    if (ch->in_room != vict->in_room)
      return eFAILURE;
    if (GET_POS(vict) == position_t::DEAD)
      return eSUCCESS | eVICT_DIED;
    chance = number(0, 101);
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
      retval = eSUCCESS;
      // Don't want to log this since a non-spell affect is going to happen all
      // the time (like SAVE_VS_FIRE or HIT-N-DAM for example) -pir
      // logf(IMMORTAL, LogChannels::LOG_BUG, "Illegal affect %d in weapons spells item '%d'.",
      //     current_affect, obj_index[ch->equipment[weapon]->item_number].virt);
      break;
    } /* switch statement */
    if (SOMEONE_DIED(retval))
      return debug_retval(ch, vict, retval);

  } /* for loop */

  if (obj_index[weap->item_number].progtypes & WEAPON_PROG)
    oprog_weapon_trigger(ch, weap);

  return eSUCCESS;
} /* spell effects */

int act_poisonous(Character *ch)
{
  if (IS_NPC(ch) && ISSET(ch->mobdata->actflags, ACT_POISONOUS))
    if (!number<quint64>(0, ch->getLevel() / 10))
      return true; // poisoned

  return false;
}

int second_attack(Character *ch)
{
  int learned;

  if ((IS_NPC(ch)) && (ISSET(ch->mobdata->actflags, ACT_2ND_ATTACK)))
    return true;
  if (affected_by_spell(ch, SKILL_DEFENDERS_STANCE))
    return false;
  learned = ch->has_skill(SKILL_SECOND_ATTACK);
  if (learned && skill_success(ch, nullptr, SKILL_SECOND_ATTACK, 15))
  {
    return true;
  }
  return false;
}

int third_attack(Character *ch)
{
  int learned;

  if ((IS_NPC(ch)) && (ISSET(ch->mobdata->actflags, ACT_3RD_ATTACK)))
    return true;
  if (affected_by_spell(ch, SKILL_DEFENDERS_STANCE))
    return false;
  learned = ch->has_skill(SKILL_THIRD_ATTACK);
  if (learned && skill_success(ch, nullptr, SKILL_THIRD_ATTACK, 15))
  {
    return true;
  }
  return false;
}

int fourth_attack(Character *ch)
{
  if ((IS_NPC(ch)) && (ISSET(ch->mobdata->actflags, ACT_4TH_ATTACK)))
    return true;
  return false;
}

/*  No longer used.  Any class can try to use their second wield if they have
    the skill.
int second_wield(Character *ch)
{
  // If the ch is capable of using the SECOND_WIELD
  if((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_MONK))
    return false;
  return true;
}
*/

void inform_victim(Character *ch, Character *victim, int dam)
{
  int max_hit;

  switch (GET_POS(victim))
  {
  case position_t::STUNNED:
    // This was moved into "attack" so that the message only goes off
    // once on the players first attack.
    //        act("$n is stunned, but will probably recover.",
    //        victim, 0, 0, TO_ROOM, INVIS_NULL);
    send_to_char("You are stunned, but will probably recover.\r\n",
                 victim);
    break;
  case position_t::DEAD:
    act("$n is DEAD!!", victim, 0, 0, TO_ROOM, INVIS_NULL);
    send_to_char("You have been KILLED!!\n\r\n\r", victim);

    break;
  default:
    max_hit = hit_limit(victim);
    if (dam > max_hit / 5)
      send_to_char("That really did HURT!\n\r", victim);
    // Wimp out?
    if (victim->getHP() < (max_hit / 5))
    {
      if (DC::isSet(victim->combat, COMBAT_BERSERK) ||
          DC::isSet(victim->combat, COMBAT_RAGE1) ||
          DC::isSet(victim->combat, COMBAT_RAGE2))
      {
        send_to_char("You are too OUTRAGED to care about your "
                     "wounds!\n\r",
                     victim);
        return;
      }
      if (dam > 0)
        send_to_char("You wish you would stop BLEEDING so much!\n\r",
                     victim);

      if (IS_NPC(victim))
      {
        if (ISSET(victim->mobdata->actflags, ACT_WIMPY))
        {
          remove_memory(victim, 't');
          remove_memory(victim, 'h', victim);
          if (victim->fighting)
            victim->add_memory(GET_NAME(victim->fighting), 'f');
          if ((!IS_AFFECTED(victim, AFF_PARALYSIS)) &&
              (!DC::isSet(victim->combat, COMBAT_STUNNED)) &&
              (!DC::isSet(victim->combat, COMBAT_STUNNED2)) &&
              (!DC::isSet(victim->combat, COMBAT_BASH1)) &&
              (!DC::isSet(victim->combat, COMBAT_BASH2)) &&
              (dam > 0))
            do_flee(victim, "", 0);
          return;
        } // end of if ACT_WIMPY
      }   // end of if npc
      else
      {
        if (DC::isSet(victim->player->toggles, Player::PLR_WIMPY))
        {
          if ((!IS_AFFECTED(victim, AFF_PARALYSIS)) &&
              (!DC::isSet(victim->combat, COMBAT_STUNNED)) &&
              (!DC::isSet(victim->combat, COMBAT_STUNNED2)) &&
              (!DC::isSet(victim->combat, COMBAT_BASH1)) &&
              (!DC::isSet(victim->combat, COMBAT_BASH2)) &&
              (dam > 0))
            do_flee(victim, "", 0);
          return;
        }
      } // end else
    }   // end max_hit / 5
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
int is_fighting_mob(Character *ch)
{
  Character *fighting = ch->fighting;
  if (!fighting)
    return 0;
  if (is_pkill(ch, fighting))
    return 0;
  if (IS_NPC(fighting) && IS_PC(ch))
    return 1;
  return 0;
}

int do_flee(Character *ch, char *argument, int cmd)
{
  int i, attempt, retval, escape = 0;
  Character *chTemp, *loop_ch, *vict = nullptr;

  if (is_stunned(ch))
    return eFAILURE;

  if (cmd == CMD_ESCAPE)
  {
    if (IS_PC(ch) && !(escape = ch->has_skill(SKILL_ESCAPE)))
    {
      send_to_char("Huh?\n\r", ch);
      return eFAILURE;
    }
    if (IS_NPC(ch))
      escape = 50 + ch->getLevel() / 3;

    if (!ch->fighting)
    {
      send_to_char("But there is nobody from whom to escape!\n\r", ch);
      return eFAILURE;
    }
    else
    {
      vict = ch->fighting;
    }

    if (!charge_moves(ch, SKILL_ESCAPE))
      return eFAILURE;

    skill_increase_check(ch, SKILL_ESCAPE, escape, SKILL_INCREASE_HARD);
    if (number(1, 101) > MIN((GET_INT(ch) + GET_DEX(ch) + (float)escape / 1.5 - GET_INT(vict) / 2 - GET_WIS(vict) / 2), 100))
      escape = 0;
  }

  if (IS_AFFECTED(ch, AFF_SNEAK) && !escape)
  {
    affect_from_char(ch, SKILL_SNEAK);
    REMBIT(ch->affected_by, AFF_SNEAK); // Mobs don't always have the affect
  }

  if (GET_CLASS(ch) == CLASS_BARD && IS_SINGING(ch))
    do_sing(ch, "stop", CMD_DEFAULT);

  if (IS_AFFECTED(ch, AFF_NO_FLEE))
  {
    if (affected_by_spell(ch, SPELL_IRON_ROOTS))
      ch->sendln("The roots bracing your legs make it impossible to run!");
    else
      ch->sendln("Your legs are too tired for running away!");
    return eFAILURE;
  }

  for (i = 0; i < 3; i++)
  {
    attempt = number(0, 5); // Select a random direction

    // keep mobs from fleeing into a no_track room
    if (CAN_GO(ch, attempt))
      if (IS_PC(ch) || !DC::isSet(DC::getInstance()->world[EXIT(ch, attempt)->to_room].room_flags, NO_TRACK))
      {
        if (!escape)
        {
          act("$n panics, and attempts to flee.", ch, 0, 0, TO_ROOM, INVIS_NULL);
          act("You panic, and attempt to flee.", ch, 0, 0, TO_CHAR, 0);
        }
        else
        {
          act("You quickly duck $N's attack and attempt to make good your escape!", ch, 0, vict, TO_CHAR, 0);
          act("$n quickly ducks your attack and attempts to make good a stealthy escape!", ch, 0, vict, TO_VICT, 0);
          act("$n quickly ducks $N's attack and attempts to make good a stealthy escape!", ch, 0, vict, TO_ROOM, INVIS_NULL | NOTVICT);
        }
        // The flee has succeeded
        Character *last_fighting = ch->fighting;
        ch->setStanding();
        ;

        QString tempcommand = dirs[attempt];

        // we do this so that any spec procs overriding movement can take effect
        SET_BIT(ch->combat, COMBAT_FLEEING);
        retval = ch->command_interpreter(tempcommand);
        // so that a player doesn't keep the flag afte rdying
        if (IS_PC(ch))
          REMOVE_BIT(ch->combat, COMBAT_FLEEING);

        if (DC::isSet(retval, eCH_DIED))
          return retval;
        REMOVE_BIT(ch->combat, COMBAT_FLEEING);
        if (DC::isSet(retval, eSUCCESS))
        {
          stop_fighting(ch);

          // Since the move stops the fight between ch and ch->fighting we have to check_pursuit
          // against it separate than the combat_list loop
          if (cmd == CMD_FLEE)
            last_fighting->check_pursuit(ch, tempcommand);

          // They got away.  Stop fighting for everyone not in the new room from fighting
          for (chTemp = combat_list; chTemp; chTemp = loop_ch)
          {
            loop_ch = chTemp->next_fighting;
            if (chTemp->fighting == ch && chTemp->in_room != ch->in_room)
            {
              stop_fighting(chTemp);
              if (cmd == CMD_FLEE)
                chTemp->check_pursuit(ch, tempcommand);
            }
          } // for

          return eSUCCESS;
        }
        else
        {
          if (!DC::isSet(retval, eCH_DIED))
            act("$n tries to flee, but is too exhausted!", ch, 0, 0, TO_ROOM, INVIS_NULL);

          if (last_fighting)
            ch->setPOSFighting();

          return retval;
        }
      } // if CAN_GO
  }     // for

  // No exits were found
  send_to_char("PANIC! You couldn't escape!\n\r", ch);
  return eFAILURE;
}

command_return_t Character::check_pursuit(Character *victim, QString dircommand)
{
  // Handle pursuit skill
  if (victim == 0 || IS_NPC(this) || !affected_by_spell(SKILL_PURSUIT))
    return eFAILURE;

  int pursuit = this->has_skill(SKILL_PURSUIT);
  if (number(1, 100) > pursuit)
  {
    // failure
    act("$N fled quickly before you were able to pursue $m!", this, 0, victim, TO_CHAR, 0);
    WAIT_STATE(this, DC::PULSE_VIOLENCE);
  }
  else
  {
    // succeeded
    stop_fighting(victim);
    if (!charge_moves(SKILL_PURSUIT))
      return eFAILURE;

    act("Upon seeing $N flee, you bellow in rage and charge blindly after $m!", this, 0, victim, TO_CHAR, 0);
    act("Upon seeing $N flee, $n bellows in rage and charges blindly after $m!", this, 0, victim, TO_ROOM, NOTVICT);

    int retval = this->command_interpreter(dircommand);
    if (DC::isSet(retval, eCH_DIED))
      return eSUCCESS;

    act("Spotting $N nearby, you rush in towards $m and furiously attack!", this, 0, victim, TO_CHAR, 0);
    act("$n charges in with a bellow of rage, cutting of your escape and attacks you furiously!", this, 0, victim, TO_VICT, 0);
    act("$n charges in with a bellow of rage, cutting of $N's escape and attacks $m furiously!", this, 0, victim, TO_ROOM, NOTVICT);
    attack(this, victim, TYPE_UNDEFINED);
    WAIT_STATE(this, DC::PULSE_VIOLENCE);
  }

  return eSUCCESS;
}

int32_t get_weapon_bit(int weapon_type)
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
    return (0);
  } /* end switch */
  return (0);
}

void remove_nosave(Character *vict)
{
  class Object *o, *next_obj, *blah, *tmp_o;

  if (!vict)
  {
    logentry("Null victim sent to remove_nosave!", OVERSEER, LogChannels::LOG_BUG);
    return;
  }

  for (o = vict->carrying; o; o = next_obj)
  {
    next_obj = o->next_content;

    // I suppose it would be possible to have a NOSAVE container --
    // in this case you lose everything inside the container.

    if (GET_ITEM_TYPE(o) == ITEM_CONTAINER &&
        !DC::isSet(o->obj_flags.extra_flags, ITEM_NOSAVE))
    {
      for (tmp_o = o->contains; tmp_o; tmp_o = blah)
      {
        blah = tmp_o->next_content;
        if (DC::isSet(tmp_o->obj_flags.extra_flags, ITEM_NOSAVE))
          move_obj(tmp_o, vict->in_room);
      }
    }

    if (DC::isSet(o->obj_flags.extra_flags, ITEM_NOSAVE))
      move_obj(o, vict->in_room);

  } // for
}

void remove_active_potato(Character *vict)
{
  class Object *obj, *next_obj;
  // char buf[256];

  if (!vict)
  {
    logentry("Null victim sent to remove_active_potato!", OVERSEER, LogChannels::LOG_BUG);
    return;
  }

  for (obj = vict->carrying; obj; obj = next_obj)
  {
    next_obj = obj->next_content;
    if (obj_index[obj->item_number].virt == 393 && obj->obj_flags.value[3] > 0)
    {
      extract_obj(obj);
    }
  }
}

int damage_type(int weapon_type)
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
    return (0);
  } /* end switch */
  return (0);
}

int debug_retval(Character *ch, Character *victim, int retval)
{
  static int dumped = 0;
  bool bugged = false;

  // Only coredump up to 10 times
  if (bugged && dumped < 10)
  {
    produce_coredump();
    dumped++;
  }

  return retval;
}

void Character::send(const char *buffer)
{
  send_to_char(std::string(buffer), this);
}

void Character::send(std::string buffer)
{
  send_to_char(buffer, this);
}

void Character::send(QString buffer)
{
  send_to_char(buffer.toStdString(), this);
}

void Character::sendRaw(std::string buffer)
{
  if (this->desc != nullptr)
  {
    this->desc->allowColor = 0;
  }

  this->send(buffer);

  if (this->desc != nullptr)
  {
    this->desc->allowColor = 1;
  }
}

command_return_t Character::tell(Character *victim, QString message)
{
  if (victim && !message.isEmpty())
  {
    return do_tell((victim->getName() + " " + message).split(' '), CMD_TELL);
  }
  return eFAILURE;
}
