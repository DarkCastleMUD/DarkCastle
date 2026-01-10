/***************************************************************************
 *  file: magic.c , Implementation of spells.              Part of DIKUMUD *
 *  Usage : The actual effect of magic.                                    *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as this   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/

// Notes:  (March 14th, 2005)
// - Cleaned old notations and removed code & effects from several places.
// - Saved any code that was either useful for the future or of quetionable origin.
// - Where is Spellcraft "dual target" affect for burning hands?
// - Where is Spellcraft "reduced fireball lag" affect listed?
//    - Apoc.

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <math.h> // pow(double,double)

#include <cstring>
#include <map>

#include <fmt/format.h>

#include "DC/spells.h"
#include "DC/obj.h"
#include "DC/room.h"
#include "DC/connect.h"
#include "DC/DC.h"
#include "DC/race.h"
#include "DC/character.h"
#include "DC/magic.h"
#include "DC/player.h"
#include "DC/fight.h"
#include "DC/utility.h"
#include "DC/structs.h"
#include "DC/handler.h"
#include "DC/mobile.h"
#include "DC/interp.h"
#include "DC/weather.h"
#include "DC/isr.h"
#include "DC/db.h"
#include "DC/act.h"
#include "DC/clan.h"
#include "DC/innate.h"
#include "DC/returnvals.h"
#include "DC/const.h"
#include "DC/inventory.h"

#define BEACON_OBJ_NUMBER 405

int saves_spell(Character *ch, Character *vict, int spell_base, int16_t save_type);
clan_data *get_clan(Character *);

void update_pos(Character *victim);
bool many_charms(Character *ch);
bool ARE_GROUPED(Character *sub, Character *obj);

bool player_resist_reallocation(Character *victim, int skill)
{
  int savebonus = 0;
  // only PC get a resist check for reallocation
  if (IS_NPC(victim))
    return false;

  if (skill < 41)
    savebonus = 5;
  else if (skill < 61)
    savebonus = 0;
  else if (skill < 81)
    savebonus = -5;
  else
    savebonus = -10;

  if (number(1, 101) < (get_saves(victim, SAVE_TYPE_MAGIC) + savebonus))
    return true;
  else
    return false;
}

bool malediction_res(Character *ch, Character *victim, int spell)
{
  // A lower level character cannot resist a Deity+ immortal
  if (IS_MINLEVEL_PC(ch, DEITY) && victim->getLevel() < ch->getLevel())
  {
    return false;
  }

  // An immortal+ victim always resists a lesser character's spell
  if (IS_MINLEVEL_PC(victim, IMMORTAL) && victim->getLevel() > ch->getLevel())
  {
    return true;
  }

  int type = 0;
  int mod = 0;
  switch (spell)
  {
  case SPELL_CURSE:
  case SPELL_WEAKEN:
    type = SAVE_TYPE_MAGIC;
    mod = 5;
    break;
  case SPELL_BLINDNESS:
    type = SAVE_TYPE_MAGIC;
    mod = 10;
    break;
  case SPELL_POISON:
  case SPELL_ATTRITION:
  case SPELL_DEBILITY:
    type = SAVE_TYPE_POISON;
    mod = 5;
    break;
  case SPELL_FEAR:
    type = SAVE_TYPE_COLD;
    mod = 5;
    break;
  case SPELL_PARALYZE:
    type = SAVE_TYPE_MAGIC;
    mod = 30; // Tweak this if paralyze needs adjusting
    break;
  default:
    DC::getInstance()->logf(OVERSEER, DC::LogChannel::LOG_BUG, "Error in malediction_res(), sent spell %d.",
                            spell);
    return true; // It's safer to have the victim resist an unknown spell
    break;
  }
  int chance = victim->saves[type] + mod + (100 - ch->has_skill(spell)) / 2;
  if (number(0, 99) < chance)
    return true; // victim resists spell
  else
    return false; // victim does not resist spell
}

bool can_heal(Character *ch, Character *victim, int spellnum)
{
  bool can_cast = true;

  // You cannot heal an elemental from "conjure elemental"
  if (IS_NPC(victim) &&
      (DC::getInstance()->mob_index[victim->mobdata->nr].virt == 88 ||
       DC::getInstance()->mob_index[victim->mobdata->nr].virt == 89 ||
       DC::getInstance()->mob_index[victim->mobdata->nr].virt == 90 ||
       DC::getInstance()->mob_index[victim->mobdata->nr].virt == 91))
  {
    ch->sendln("The heavy magics surrounding this being prevent healing.");
    return false;
  }

  if (victim->getHP() > GET_MAX_HIT(victim) - 10)
  {
    if (spellnum != SPELL_CURE_LIGHT)
      can_cast = false;
  }
  else if (victim->getHP() > GET_MAX_HIT(victim) - 25)
  {
    if (spellnum != SPELL_CURE_LIGHT && spellnum != SPELL_CURE_SERIOUS)
      can_cast = false;
  }
  else if (victim->getHP() > GET_MAX_HIT(victim) - 50)
  {
    if (spellnum != SPELL_CURE_LIGHT && spellnum != SPELL_CURE_SERIOUS &&
        spellnum != SPELL_CURE_CRITIC)
      can_cast = false;
  }

  //  if (victim->getHP() > GET_MAX_HIT(victim)-5)
  //    can_cast = false;

  if (!can_cast)
  {
    if (ch == victim)
      ch->sendln("You have not received enough damage to warrant this powerful incantation.");
    else
      act("$N has not received enough damage to warrant this powerful incantation.", ch, 0, victim, TO_CHAR, 0);
  }
  return can_cast;
}

bool resist_spell(int perc)
{
  if (number(1, 100) > perc)
    return true;
  return false;
}

bool resist_spell(Character *ch, int skill)
{
  int perc = ch->has_skill(skill);
  if (number(1, 100) > perc)
    return true;
  return false;
}

/* ------------------------------------------------------- */
/* Begin Spells Listings                                   */
/* ------------------------------------------------------- */

/* MAGIC MISSILE */

int spell_magic_missile(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int count = 1;
  int retval = eSUCCESS;
  int weap_spell = obj ? WEAR_WIELD : 0;

  set_cantquit(ch, victim);
  dam = 15 + getRealSpellDamage(ch);
  count += (skill > 15) + (skill > 35) + (skill > 55) + (skill > 75);

  /* Spellcraft Effect */
  if (spellcraft(ch, SPELL_MAGIC_MISSILE))
    count++;

  while (!SOMEONE_DIED(retval) && count--)
  {
    if (level > 200)
      retval = damage(ch, victim, dam, TYPE_HIT + level - 200, SPELL_MAGIC_MISSILE, weap_spell);
    else
      retval = damage(ch, victim, dam, TYPE_MAGIC, SPELL_MAGIC_MISSILE, weap_spell);
    if (dam > 50)
      dam = 50; // spelldamage only applies to 1st missile
  }
  return retval;
}

/* CHILL TOUCH */

int spell_chill_touch(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  int dam = 300;
  int save;
  int weap_spell = obj ? WEAR_WIELD : 0;
  int retval;

  if (level > 200)
    retval = damage(ch, victim, dam, TYPE_HIT + level - 200, SPELL_CHILL_TOUCH, weap_spell);
  else
    retval = damage(ch, victim, dam, TYPE_COLD, SPELL_CHILL_TOUCH, weap_spell);

  if (SOMEONE_DIED(retval))
    return retval;

  bool hasSpellcraft = spellcraft(ch, SPELL_CHILL_TOUCH);

  if (isSet(retval, eEXTRA_VAL2))
    victim = ch;
  if (isSet(retval, eEXTRA_VALUE))
    return retval;

  save = saves_spell(ch, victim, (level / 2), SAVE_TYPE_COLD);
  if (save < 0 && skill > 50) // if failed
  {
    if (victim->affected_by_spell(SPELL_CHILL_TOUCH))
      return retval;
    af.type = SPELL_CHILL_TOUCH;
    af.duration = skill / 18;
    af.modifier = -skill / 18;
    af.location = APPLY_STR;
    af.bitvector = -1;
    affect_to_char(victim, &af);

    /* Spellcraft Effect */
    if (hasSpellcraft)
    {
      af.modifier = -skill / 18;
      af.location = APPLY_DEX;
      affect_to_char(victim, &af);
    }
  }
  return retval;
}

/* BURNING HANDS */

int spell_burning_hands(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  set_cantquit(ch, victim);
  dam = 165;
  if (level > 200)
    return damage(ch, victim, dam, TYPE_HIT + level - 200, SPELL_BURNING_HANDS);
  else
    return damage(ch, victim, dam, TYPE_FIRE, SPELL_BURNING_HANDS);
}

/* SHOCKING GRASP */

int spell_shocking_grasp(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  dam = 130;
  return damage(ch, victim, dam, TYPE_ENERGY, SPELL_SHOCKING_GRASP);
}

/* LIGHTNING BOLT */

int spell_lightning_bolt(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;
  set_cantquit(ch, victim);
  dam = 240;
  if (level > 200)
    return damage(ch, victim, dam, TYPE_HIT + level - 200, SPELL_LIGHTNING_BOLT, weap_spell);
  else
    return damage(ch, victim, dam, TYPE_ENERGY, SPELL_LIGHTNING_BOLT, weap_spell);
}

/* COLOUR SPRAY */

int spell_colour_spray(uint8_t level, Character *ch, Character *victim,
                       class Object *obj, int skill)
{
  bool victim_dazzled = false;
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;
  set_cantquit(ch, victim);
  dam = 370;

  if (number(1, 100) > get_saves(victim, SAVE_TYPE_MAGIC) + 40 && (skill > 50 || IS_NPC(ch)))
  {
    SET_BIT(victim->combat, COMBAT_SHOCKED2);
    victim_dazzled = true;
  }

  int retval = damage(ch, victim, dam, TYPE_MAGIC, SPELL_COLOUR_SPRAY,
                      weap_spell);

  if (SOMEONE_DIED(retval))
    return retval;

  if (isSet(retval, eEXTRA_VAL2))
    victim = ch;
  if (isSet(retval, eEXTRA_VALUE))
    return retval;

  /*  Dazzle Effect */
  if (victim_dazzled && ch->in_room == victim->in_room)
  {
    act("$N blinks in confusion from the distraction of the colour spray.", ch,
        0, victim, TO_ROOM, NOTVICT);
    act("Brilliant streams of colour streak from $n's fingers!  WHOA!  Cool!",
        ch, 0, victim, TO_VICT, 0);
    act("Your colours of brilliance dazzle the simpleminded $N.", ch, 0, victim,
        TO_CHAR, 0);
  }

  return retval;
}

/* DROWN */

int spell_drown(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam, retval;
  int weap_spell = obj ? WEAR_WIELD : 0;

  /* Does not work in Desert or Underwater */
  if (DC::getInstance()->world[ch->in_room].sector_type == SECT_DESERT)
  {
    ch->sendln("You're trying to drown someone in the desert?  Get a clue!");
    return eFAILURE;
  }
  if (DC::getInstance()->world[ch->in_room].sector_type == SECT_UNDERWATER)
  {
    ch->sendln("Hello!  You're underwater!  *knock knock*  Anyone home?");
    return eFAILURE;
  }

  set_cantquit(ch, victim);

  /* Drown BINGO Effect */
  if (skill > 80 && number(1, 100) == 1 && !victim->isImmortalPlayer())
  {
    dam = victim->getHP() * 5 + 20;
    victim->sendln(QStringLiteral("You are torn apart by the force of %1's watery blast and are killed instantly!").arg(ch->getName()));
    act("$N is torn apart by the force of $n's watery blast and killed instantly!", ch, 0, victim, TO_ROOM, NOTVICT);
    act("$N is torn apart by the force of your watery blast and killed instantly!", ch, 0, victim, TO_CHAR, 0);
    return damage(ch, victim, dam, TYPE_WATER, SPELL_DROWN);
  }

  return damage(ch, victim, 265, TYPE_WATER, SPELL_DROWN, weap_spell);
}

/* ENERGY DRAIN */
// Drains XP, MANA, HP - caster gains HP and MANA -- Currently MOB ONLY

int spell_energy_drain(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int mult = GET_EXP(victim) / 20;
  mult = MIN(10000, mult);
  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
    return eFAILURE;

  set_cantquit(ch, victim);
  if (saves_spell(ch, victim, 1, SAVE_TYPE_MAGIC) > 0)
    mult /= 2;

  gain_exp(victim, 0 - mult);
  victim->removeHP(victim->getHP() / 20);
  victim->sendln("Your knees buckle as life force is drained from your body!\r\nYou have lost some experience!");
  act("You drain some of $N's experience!", ch, 0, victim, TO_CHAR, 0);
  return eSUCCESS;
}

/* SOULDRAIN */
// Drains MANA - caster gains MANA

int spell_souldrain(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int mana;
  set_cantquit(ch, victim);

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You cannot do this in a safe room!");
    return eFAILURE;
  }
  mana = dam_percent(skill, 125 + getRealSpellDamage(ch));
  if (mana > GET_MANA(victim))
    mana = GET_MANA(victim);
  if (mana <= 0)
  {
    ch->sendln("There isn't enough magical energy to be drained.");
    return eFAILURE;
  }
  if (number(1, 100) < get_saves(victim, SAVE_TYPE_MAGIC) + 40)

  {
    act("$N resists your attempt to souldrain $M!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to souldrain $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to souldrain you!", ch, nullptr, victim, TO_VICT, 0);
    int retval;
    if (IS_NPC(victim) && !victim->fighting)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED, FIRST);
      retval = SWAP_CH_VICT(retval);
    }
    else
      retval = eFAILURE;
    return retval;
  }

  GET_MANA(victim) -= mana;
  GET_MANA(ch) += mana;
  char buf[MAX_STRING_LENGTH];
  sprintf(buf, "$B%d$R", mana);
  send_damage("You drain $N's very soul for | mana!", ch, 0, victim, buf, "You drain $N's very soul!", TO_CHAR);
  send_damage("You feel your very soul being drained by $n for | mana!", ch, 0, victim, buf, "You feel your very soul being drained by $n!", TO_VICT);
  send_damage("$N's soul is drained away by $n for | mana!", ch, 0, victim, buf, "$N's soul is drained away by $n!", TO_ROOM);

  if (GET_MANA(ch) > GET_MAX_MANA(ch))
    GET_MANA(ch) = GET_MAX_MANA(ch);

  int retval;
  if (IS_NPC(victim) && !victim->fighting)
  {
    retval = attack(victim, ch, TYPE_UNDEFINED, FIRST);
    retval = SWAP_CH_VICT(retval);
  }
  else
    retval = eSUCCESS;
  return retval;
}

/* VAMPIRIC TOUCH */

int spell_vampiric_touch(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;
  set_cantquit(ch, victim);
  dam = 225;
  int adam = dam_percent(skill, 225); // Actual damage, for drainy purposes.

  int i = victim->getHP();
  int retval = damage(ch, victim, dam, TYPE_COLD, SPELL_VAMPIRIC_TOUCH, weap_spell);
  if (!SOMEONE_DIED(retval) && victim->getHP() >= i)
    return retval;

  if (!SOMEONE_DIED(retval))
  {
    ch->addHP(MIN(adam, i - victim->getHP()));
  }
  else
  {
    ch->addHP(MIN(adam, i));
  }

  return retval;
}

/* METEOR SWARM */

int spell_meteor_swarm(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;
  set_cantquit(ch, victim);
  dam = 600;
  int retval;

  if (level > 200)
    retval = damage(ch, victim, dam, TYPE_HIT + level - 200, SPELL_METEOR_SWARM, weap_spell);
  else
    retval = damage(ch, victim, dam, TYPE_MAGIC, SPELL_METEOR_SWARM, weap_spell);

  /* Spellcraft Effect */
  if (!SOMEONE_DIED(retval) && spellcraft(ch, SPELL_METEOR_SWARM) && !number(0, 9))
  {
    if (isSet(retval, eEXTRA_VAL2))
      victim = ch;
    if (isSet(retval, eEXTRA_VALUE))
      return retval;
    act("The force of the spell knocks $N over!", ch, 0, victim, TO_CHAR, 0);
    victim->sendln("The force of the spell knocks you over!");
    victim->setSitting();
  }
  return retval;
}

/* FIREBALL */

int spell_fireball(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;
  set_cantquit(ch, victim);
  dam = 340;
  int retval;

  if (level > 200)
    retval = damage(ch, victim, dam, TYPE_HIT + level - 200, SPELL_FIREBALL, weap_spell);
  else
    retval = damage(ch, victim, dam, TYPE_FIRE, SPELL_FIREBALL, weap_spell);

  if (SOMEONE_DIED(retval) || ch->in_room != victim->in_room)
    return retval;
  // Above: Fleeing now saves you from the second blast.

  if (isSet(retval, eEXTRA_VAL2))
    victim = ch;
  if (isSet(retval, eEXTRA_VALUE))
    return retval;

  /* Fireball Recombining Effect */
  if (skill > 80)
    if (number(0, 100) < (skill / 5))
    {
      act("The expanding $B$4flames$R suddenly recombine and fly at $N again!", ch, 0, victim, TO_ROOM, 0);
      act("The expanding $B$4flames$R suddenly recombine and fly at $N again!", ch, 0, victim, TO_CHAR, 0);
      retval = damage(ch, victim, dam, TYPE_FIRE, SPELL_FIREBALL);
    }
  return retval;
}

/* SPARKS */

int spell_sparks(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;
  set_cantquit(ch, victim);
  dam = dice(level, 6);
  return damage(ch, victim, dam, TYPE_FIRE, SPELL_SPARKS, weap_spell);
}

/* HOWL */

int spell_howl(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  Character *tmp_char;
  int retval;
  int weap_spell = obj ? WEAR_WIELD : 0;
  set_cantquit(ch, victim);

  if (saves_spell(ch, victim, 5, SAVE_TYPE_MAGIC) >= 0)
  {
    return damage(ch, victim, 0, TYPE_SONG, SPELL_HOWL, weap_spell);
  }
  retval = damage(ch, victim, 300, TYPE_SONG, SPELL_HOWL, weap_spell);

  if (SOMEONE_DIED(retval))
    return retval;

  if (isSet(retval, eEXTRA_VAL2))
    victim = ch;
  if (isSet(retval, eEXTRA_VALUE))
    return retval;

  for (tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char;
       tmp_char = tmp_char->next_in_room)
  {

    if (!tmp_char->master)
      continue;

    if (tmp_char->master == ch || ARE_GROUPED(tmp_char->master, ch))
      continue;

    if (tmp_char->affected_by_spell(SPELL_CHARM_PERSON))
    {
      affect_from_char(tmp_char, SPELL_CHARM_PERSON);
      tmp_char->sendln("You feel less enthused about your master.");
      act("$N blinks and shakes its head, clearing its thoughts.",
          ch, 0, tmp_char, TO_CHAR, 0);
      act("$N blinks and shakes its head, clearing its thoughts.",
          ch, 0, tmp_char, TO_ROOM, NOTVICT);

      if (tmp_char->fighting)
      {
        do_say(tmp_char, "Screw this! I'm going home!");
        if (tmp_char->fighting->fighting == tmp_char)
          stop_fighting(tmp_char->fighting);

        stop_fighting(tmp_char);
      }
    }
  } // for loop through people in the room
  return retval;
}

/* HOLY AEGIS/UNHOLY AEGIS*/

int spell_aegis(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  int spl = GET_CLASS(ch) == CLASS_ANTI_PAL ? SPELL_U_AEGIS : SPELL_AEGIS;
  if (ch->affected_by_spell(spl))
    affect_from_char(ch, spl);
  if (ch->affected_by_spell(SPELL_ARMOR))
  {
    act("$n is already protected by magical armour.", ch, 0, 0, TO_CHAR, 0);
    return eFAILURE;
  }

  af.type = spl;
  af.duration = 10 + skill / 4;
  af.modifier = -7 - skill / 3;
  af.location = APPLY_AC;
  af.bitvector = -1;

  affect_to_char(ch, &af);
  if (GET_CLASS(ch) == CLASS_PALADIN)
    ch->sendln("You invoke your protective aegis.");
  else
    ch->sendln("You invoke your unholy aegis.");
  return eSUCCESS;
}

/* ARMOUR */

int spell_armor(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  if (victim->affected_by_spell(SPELL_ARMOR))
    affect_from_char(victim, SPELL_ARMOR);
  if (victim->affected_by_spell(SPELL_AEGIS) || victim->affected_by_spell(SPELL_U_AEGIS))
  {
    act("$n is already protected by magical armour.", victim, 0, ch, TO_VICT, 0);
    return eFAILURE;
  }

  af.type = SPELL_ARMOR;
  af.duration = 10 + skill / 3;
  af.modifier = 0 - skill / 2;
  af.location = APPLY_AC;
  af.bitvector = -1;

  affect_to_char(victim, &af);
  victim->sendln("You feel a magical armour surround you.");
  return eSUCCESS;
}

/* STONE SHIELD */

int spell_stone_shield(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  char buf[160];
  int duration, modifier;
  if (victim->affected_by_spell(SPELL_GREATER_STONE_SHIELD))
  {
    sprintf(buf, "%s is already surrounded by a greater stoneshield.\r\n", GET_SHORT(victim));
    ch->send(buf);
    return eSUCCESS;
  }

  if (victim->affected_by_spell(SPELL_STONE_SHIELD))
    affect_from_char(victim, SPELL_STONE_SHIELD);

  duration = 4 + (skill / 10) + (GET_WIS(ch) > 20);
  modifier = 15 + skill / 5;

  af.type = SPELL_STONE_SHIELD;
  af.duration = duration * modifier;
  af.modifier = modifier;
  af.location = 0;
  af.bitvector = -1;

  affect_to_char(victim, &af);
  victim->sendln("A shield of ethereal stones begins to swirl around you.");
  act("Ethereal stones form out of nothing and begin to swirl around $n.",
      victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

int cast_stone_shield(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (tar_ch->fighting)
    {
      ch->sendln("The combat disrupts the ether too much to coalesce into stones.");
      return eFAILURE;
    }
    return spell_stone_shield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_stone_shield(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_stone_shield(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_stone_shield(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in stone_shield!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_iridescent_aura(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_IRIDESCENT_AURA - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_iridescent_aura(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_IRIDESCENT_AURA - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
          retval &= spell_iridescent_aura(level, ch, leader, 0, skill);
      }

      return retval;
    }
    return spell_iridescent_aura(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_iridescent_aura(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_iridescent_aura(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_iridescent_aura(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in iridesent_aura!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* GREATER STONE SHIELD */

int spell_greater_stone_shield(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  char buf[160];
  int duration, modifier;
  if (victim->affected_by_spell(SPELL_STONE_SHIELD))
  {
    sprintf(buf, "%s is already surrounded by a stone shield.\r\n", GET_SHORT(victim));
    ch->send(buf);
    return eSUCCESS;
  }

  if (victim->affected_by_spell(SPELL_GREATER_STONE_SHIELD))
    affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);

  modifier = 20 + skill / 4;
  duration = 5 + (skill / 6) + (GET_WIS(ch) > 20);

  af.type = SPELL_GREATER_STONE_SHIELD;
  af.duration = duration * modifier;
  af.modifier = modifier;
  af.location = 0;
  af.bitvector = -1;

  affect_to_char(victim, &af);
  victim->sendln("A shield of ethereal stones begins to swirl around you.");
  act("Ethereal stones form out of nothing and begin to swirl around $n.",
      victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

int cast_greater_stone_shield(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (tar_ch->fighting)
    {
      ch->sendln("The combat disrupts the ether too much to coalesce into stones.");
      return eFAILURE;
    }
    return spell_greater_stone_shield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_greater_stone_shield(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_greater_stone_shield(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_greater_stone_shield(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in stone_shield!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* EARTHQUAKE */

int spell_earthquake(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  bool capsize = false, underwater = false;
  int dam = 0, retval = eSUCCESS, weap_spell = obj ? WEAR_WIELD : 0, ch_zone = 0, tmp_vict_zone = 0;
  Object *tmp_obj = 0, *obj_next = 0;

  switch (DC::getInstance()->world[ch->in_room].sector_type)
  {
  case SECT_AIR:
    ch->sendln("You attempt to cause an earthquake in the air, but nothing happens.");
    act("$n attempted to cause an earthquake in the air, what an idiot.", ch, 0, 0, TO_ROOM, 0);
    return retval;
    break;

  case SECT_WATER_NOSWIM:
    ch->sendln("The water turns violent as the earth beneath it trembles.");
    act("$n's earthquake creates a tsunami that capsizes any ships in the area.", ch, 0, 0, TO_ROOM, 0);
    capsize = true;
    break;

  case SECT_UNDERWATER:
    underwater = true;
    ch->sendln("The earth trembles below you, the water pressure nearly makes you lose consciousness.");
    act("$n's earthquake nearly makes you implode from the water pressure.", ch, 0, 0, TO_ROOM, 0);
    break;

  default:
    ch->sendln("The earth trembles beneath your feet!");
    act("$n makes the earth tremble and shiver.\r\n", ch, 0, 0, TO_ROOM, 0);
    break;
  }

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {
    if (isSet(retval, eCH_DIED))
    {
      break;
    }

    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    try
    {
      ch_zone = DC::getInstance()->world[ch->in_room].zone;
      tmp_vict_zone = DC::getInstance()->world[tmp_victim->in_room].zone;
    }
    catch (...)
    {
      produce_coredump();
      return eFAILURE;
    }

    if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) && (!ARE_GROUPED(ch, tmp_victim)) && can_be_attacked(ch, tmp_victim))
    {

      if (IS_NPC(ch) && IS_NPC(tmp_victim)) // mobs don't earthquake each other
        continue;

      dam = 150;

      if (!underwater && (IS_AFFECTED(tmp_victim, AFF_FREEFLOAT) || IS_AFFECTED(tmp_victim, AFF_FLYING)))
      {
        tmp_victim->sendln("Debris from the earthquake flies in every direction and strikes you.");
        dam = 1;
      }
      else
      {
        if (tmp_victim && capsize && IS_PC(tmp_victim))
        {
          // capsize
          dam = 0;

          for (tmp_obj = tmp_victim->carrying; tmp_obj; tmp_obj = obj_next)
          {
            obj_next = tmp_obj->next_content;

            if (tmp_obj->obj_flags.type_flag == ITEM_BOAT)
            {
              act("$p carried by $n has capsized!", tmp_victim, tmp_obj, 0, TO_ROOM, 0);
              act("Your $p has capsized!", tmp_victim, tmp_obj, 0, TO_CHAR, 0);
              act("$p breaks apart into floating junk.", tmp_victim, tmp_obj, 0, TO_CHAR, 0);
              act("$p breaks apart into floating junk.", tmp_victim, tmp_obj, 0, TO_ROOM, 0);
              eq_destroyed(tmp_victim, tmp_obj, -1);
            } // if (tmp_obj
          } // for (tmp_obj
        } // if (tmp_victim
      } // else

      if (dam > 0)
      {
        int retval2 = 0;
        retval2 = damage(ch, tmp_victim, dam, TYPE_MAGIC, SPELL_EARTHQUAKE, weap_spell);

        if (isSet(retval2, eVICT_DIED))
        {
          SET_BIT(retval, eVICT_DIED);
        }
        else if (isSet(retval2, eCH_DIED))
        {
          SET_BIT(retval, eCH_DIED);
          break;
        }
      } // if (dam > 0

      // If not in room, yourself, groupie, or cant be attacked then do this...
    }
    else if (ch_zone == tmp_vict_zone)
    {
      tmp_victim->sendln("The earth trembles and shivers.");
    }
  } // main for loop of all characters

  return retval;
}

/* LIFE LEECH */

int spell_life_leech(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam, retval = eSUCCESS;
  int weap_spell = obj ? WEAR_WIELD : 0;
  Character *tmp_victim, *temp;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
    return eFAILURE;
  /*  double o = 0.0, m = 0.0, avglevel = 0.0;
   for (tmp_victim = DC::getInstance()->world[ch->in_room].people;tmp_victim;tmp_victim = tmp_victim->next_in_room)
   if (!ARE_GROUPED(ch, tmp_victim) && ch != tmp_victim)
   { o++; m++; avglevel *= o-1; avglevel += tmp_victim->getLevel(); avglevel /= o;}
   else m++;
   m--; // don't count player
   avglevel -= (double)ch->getLevel();
   double powmod = 0.2;
   powmod -= (avglevel*0.001);
   powmod -= (ch->has_skill( SPELL_LIFE_LEECH) * 0.001);
   int max = (int)(o * 50 * ( m / pow(m, powmod*m)));
   max += number(-10,10);
   */
  for (tmp_victim = DC::getInstance()->world[ch->in_room].people; tmp_victim && tmp_victim != (Character *)0x95959595; tmp_victim = temp)
  {
    temp = tmp_victim->next_in_room;
    if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) && (!ARE_GROUPED(ch, tmp_victim)) && can_be_attacked(ch, tmp_victim))
    {
      //		dam = max / o;
      dam = 150;
      int adam = dam_percent(skill, dam);
      if (isSet(tmp_victim->immune, ISR_POISON))
        adam = 0;

      if (GET_HIT(tmp_victim) < adam)
      {
        ch->addHP(GET_HIT(tmp_victim) * 0.3);
      }
      else
      {
        ch->addHP(adam * 0.3);
      }

      retval &= damage(ch, tmp_victim, dam, TYPE_POISON, SPELL_LIFE_LEECH, weap_spell);
    }
  }
  return retval;
}

/* SOLAR GATE BLIND EFFECT (Spellcraft Effect) */

void do_solar_blind(Character *ch, Character *tmp_victim, int skill)
{
  struct affected_type af;
  if (!ch || !tmp_victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null ch or vict in solar_blind"), IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }
  if (tmp_victim->in_room != ch->in_room)
    return;
  if (ch->has_skill(SPELL_SOLAR_GATE) < 81)
    return;
  if (number(0, 9))
    return;
  if (!IS_AFFECTED(tmp_victim, AFF_BLIND))
  {
    act("$n seems to be blinded!", tmp_victim, 0, 0, TO_ROOM, INVIS_NULL);
    tmp_victim->sendln("The world dims and goes $B$0black$R as you are blinded!");

    af.type = SPELL_BLINDNESS;
    af.location = APPLY_HITROLL;
    af.modifier = tmp_victim->has_skill(SKILL_BLINDFIGHTING) ? skill_success(tmp_victim, 0, SKILL_BLINDFIGHTING) ? -10 : -20 : -20;
    // Make hitroll worse
    af.duration = 2;
    af.bitvector = AFF_BLIND;
    affect_to_char(tmp_victim, &af);
    af.location = APPLY_AC;
    af.modifier = tmp_victim->has_skill(SKILL_BLINDFIGHTING) ? skill_success(tmp_victim, 0, SKILL_BLINDFIGHTING) ? skill / 4 : skill / 2 : skill / 2;
    affect_to_char(tmp_victim, &af);
  } // if affect by blind
}

/* SOLAR GATE */

int spell_solar_gate(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int i;
  int dam;
  int retval;
  Character *tmp_victim, *temp;
  int orig_room;

  char *desc_dirs[] = {
      "from the South.",
      "from the West.",
      "from the North.",
      "from the East.",
      "from Below.",
      "from Above.",
      "\n"};

  cmd_t to_charge[6] = {
      cmd_t::SOUTH,
      cmd_t::WEST,
      cmd_t::NORTH,
      cmd_t::EAST,
      cmd_t::DOWN,
      cmd_t::UP};

  // do room caster is in
  ch->sendln("A Bright light comes down from the heavens.");
  act("$n opens a Solar Gate.\r\n", ch, 0, 0, TO_ROOM, 0);

  // we use "orig_room" for this now, instead of ch->in_room.  The reason for
  // this, is so if we die from a reflect, we don't keep looping through and
  // solaring from our new location.  Since the solar is theoretically hitting
  // everyone at the same time, we can't just break out. -pir 12/26

  orig_room = ch->in_room;

  // we also now use .people instead of the character_list -pir 12/26

  for (tmp_victim = DC::getInstance()->world[orig_room].people; tmp_victim && tmp_victim != (Character *)0x95959595; tmp_victim = temp)
  {
    temp = tmp_victim->next_in_room;
    if ((orig_room == tmp_victim->in_room) && (tmp_victim != ch) &&
        (!ARE_GROUPED(ch, tmp_victim)) && (can_be_attacked(ch, tmp_victim)))
    {

      dam = 660;
      retval = damage(ch, tmp_victim, dam, TYPE_FIRE, SPELL_SOLAR_GATE);
      if (isSet(retval, eCH_DIED))
        return retval;
      if (isSet(retval, eEXTRA_VALUE))
        return retval;
      if (!isSet(retval, eVICT_DIED) && spellcraft(ch, SPELL_SOLAR_GATE) && !isSet(retval, eEXTRA_VAL2))
        do_solar_blind(ch, tmp_victim, skill);
    } // if are grouped, etc
  } // for

  // do surrounding rooms

  for (i = 0; i < 6; i++)
  {
    if (CAN_GO(ch, i))
    {
      for (tmp_victim = DC::getInstance()->world[DC::getInstance()->world[orig_room].dir_option[i]->to_room].people;
           tmp_victim; tmp_victim = temp)
      {
        temp = tmp_victim->next_in_room;
        if (IS_NPC(tmp_victim) && DC::getInstance()->mob_index[tmp_victim->mobdata->nr].virt >= 2300 &&
            DC::getInstance()->mob_index[tmp_victim->mobdata->nr].virt <= 2399)
        {
          ch->sendln("The clan hall's enchantments absorbs part of your spell.");
          continue;
        }
        if ((tmp_victim != ch) && (tmp_victim->in_room != orig_room) &&
            (!ARE_GROUPED(ch, tmp_victim)) &&
            (can_be_attacked(tmp_victim, tmp_victim)))
        {
          char buf[MAX_STRING_LENGTH];
          dam = 360; // dice(level, 10) + skill/2;
          sprintf(buf, "You are ENVELOPED in a PAINFUL BRIGHT LIGHT pouring in %s.", desc_dirs[i]);
          act(buf, tmp_victim, 0, ch, TO_CHAR, 0);

          retval = damage(ch, tmp_victim, dam, TYPE_FIRE, SPELL_SOLAR_GATE);
          if (isSet(retval, eCH_DIED))
            return retval;

          if (isSet(retval, eVICT_DIED))
            if (ch->desc && ch->player && !isSet(ch->player->toggles, Player::PLR_WIMPY))
              ch->desc->wait = 0;
          if (!isSet(retval, eVICT_DIED))
          {
            // don't blind surrounding rooms
            // do_solar_blind(ch, tmp_victim);
            if (tmp_victim->getLevel())
              if (IS_NPC(tmp_victim))
              {
                tmp_victim->add_memory(GET_NAME(ch), 'h');
                if (IS_PC(ch) && IS_NPC(tmp_victim))
                  if (!ISSET(tmp_victim->mobdata->actflags, ACT_STUPID) && tmp_victim->hunting.isEmpty())
                  {
                    level_diff_t level_difference = ch->getLevel() - tmp_victim->getLevel() / 2;
                    if (level_difference > 0)
                    {
                      tmp_victim->add_memory(GET_NAME(ch), 't');
                      struct timer_data *timer = new timer_data;
                      timer->var_arg1 = tmp_victim->hunting;
                      timer->arg2 = (void *)tmp_victim;
                      timer->function = clear_hunt;
                      timer->next = timer_list;
                      timer_list = timer;
                      timer->timeleft = (ch->getLevel() / 4) * 60;
                    }
                  }
                if (!tmp_victim->isStanding())
                {
                  tmp_victim->setStanding();
                }

                if (!IS_AFFECTED(tmp_victim, AFF_BLIND) &&
                    !IS_AFFECTED(tmp_victim, AFF_PARALYSIS) &&
                    !tmp_victim->affected_by_spell(SPELL_IRON_ROOTS) &&
                    !ISSET(tmp_victim->mobdata->actflags, ACT_STUPID) &&
                    !tmp_victim->fighting)
                  do_move(tmp_victim, "", to_charge[i]);
              }
          } // if ! eVICT_DIED
        } // if are grouped, etc
      } // for tmp victim
    } // if can go
  } // for i 0 < 6
  return eSUCCESS;
}

/* GROUP RECALL */

int spell_group_recall(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int chance = 0;

  if (skill < 40)
  {
    chance = 5;
  }
  else if (skill > 40 && skill <= 60)
  {
    chance = 4;
  }
  else if (skill > 60 && skill <= 80)
  {
    chance = 3;
  }
  else if (skill > 80 && skill <= 90)
  {
    chance = 2;
  }
  else if (skill > 90)
  {
    chance = 1;
  }

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {
    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    if (ch->in_room == tmp_victim->in_room &&
        tmp_victim != ch &&
        ARE_GROUPED(ch, tmp_victim))
    {
      if (!tmp_victim)
      {
        DC::getInstance()->logentry(QStringLiteral("Bad character in character_list in magic.c in group-recall!"), ANGEL, DC::LogChannel::LOG_BUG);
        return eFAILURE | eINTERNAL_ERROR;
      }
      if (number(1, 101) > chance)
        spell_word_of_recall(level, ch, tmp_victim, obj, 110);
      else
      {
        csendf(tmp_victim, "%s's group recall partially fails leaving you behind!\r\n", qPrintable(ch->getName()));
        csendf(ch, "Your group recall partially fails leaving %s behind!\r\n", qPrintable(tmp_victim->getName()));
      }
    }
  }
  return spell_word_of_recall(level, ch, ch, obj, skill);
}

/* GROUP FLY */

int spell_group_fly(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {

    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    if ((ch->in_room == tmp_victim->in_room) &&
        (ARE_GROUPED(ch, tmp_victim)))
    {
      if (!tmp_victim)
      {
        DC::getInstance()->logentry(QStringLiteral("Bad tmp_victim in character_list in group fly!"), ANGEL, DC::LogChannel::LOG_BUG);
        return eFAILURE;
      }
      spell_fly(level, ch, tmp_victim, obj, skill);
    }
  }
  return eSUCCESS;
}

/* HEROES FEAST */

int spell_heroes_feast(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int result = 15 + skill / 6;

  if (GET_COND(ch, FULL) > -1)
  {
    GET_COND(ch, FULL) = result;
    GET_COND(ch, THIRST) = result;
  }
  ch->sendln("You partake in a magnificent feast!");
  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {
    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
        (ARE_GROUPED(ch, tmp_victim)))
    {
      if (GET_COND(tmp_victim, FULL) > -1 && tmp_victim->getLevel() < 60)
      {
        GET_COND(tmp_victim, FULL) = result;
        GET_COND(tmp_victim, THIRST) = result;
      }
      tmp_victim->sendln("You partake in a magnificent feast!");
    }
  }
  return eSUCCESS;
}

/* GROUP SANCTUARY */

int spell_group_sanc(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {
    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    if ((ch->in_room == tmp_victim->in_room) && (ARE_GROUPED(ch, tmp_victim)))
    {
      if (!tmp_victim)
      {
        DC::getInstance()->logentry(QStringLiteral("Bad tmp_victim in character_list in group fly!"), ANGEL, DC::LogChannel::LOG_BUG);
        return eFAILURE | eINTERNAL_ERROR;
      }
      spell_sanctuary(level, ch, tmp_victim, obj, skill);
    }
  }
  return eSUCCESS;
}

/* HEAL SPRAY */

int spell_heal_spray(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {
    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    if ((ch->in_room == tmp_victim->in_room) &&
        (ARE_GROUPED(ch, tmp_victim)))
    {
      spell_heal(level, ch, tmp_victim, obj, skill);
    }
  }
  return eSUCCESS;
}

/* FIRESTORM */

int spell_firestorm(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam = 0;
  int retval = eSUCCESS;
  int retval2 = 0;
  Character *next_victim = 0;

  ch->sendln("$B$4Fire$R falls from the heavens!");
  act("$n makes $B$4fire$R fall from the heavens!", ch, 0, 0, TO_ROOM, 0);

  for (Character *victim = DC::getInstance()->world[ch->in_room].people; victim && victim != reinterpret_cast<Character *>(0x95959595); victim = next_victim)
  {
    next_victim = victim->next_in_room;

    // skip yourself, your groupies and those who may not be attacked
    if ((!charExists(victim)) ||
        (victim == ch) ||
        (ARE_GROUPED(ch, victim)) ||
        (!can_be_attacked(ch, victim)))
    {
      continue;
    }

    dam = 300;

    if (level > 200)
    {
      retval2 = damage(ch, victim, dam, TYPE_HIT + level - 200, SPELL_FIRESTORM);
    }
    else
    {
      retval2 = damage(ch, victim, dam, TYPE_FIRE, SPELL_FIRESTORM);
    }

    if (isSet(retval2, eVICT_DIED))
    {
      SET_BIT(retval, eVICT_DIED);
    }
    else if (isSet(retval2, eCH_DIED))
    {
      SET_BIT(retval, eCH_DIED);
      break;
    }
  }

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {
    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    if ((tmp_victim->in_room == ch->in_room) ||
        (tmp_victim == ch) ||
        (!ARE_GROUPED(ch, tmp_victim)) ||
        (can_be_attacked(ch, tmp_victim)))
    {
      continue;
    }

    try
    {
      if (DC::getInstance()->world[ch->in_room].zone == DC::getInstance()->world[tmp_victim->in_room].zone)
      {
        tmp_victim->sendln("You feel a HOT blast of air.");
      }
    }
    catch (...)
    {
      produce_coredump();
      return eFAILURE | retval;
    }
  }

  return retval;
}

/* DISPEL EVIL */

int spell_dispel_evil(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  if (obj && obj->in_room && obj->obj_flags.value[0] == SPELL_DESECRATE)
  { // desecrate object
    Character *pal = nullptr;
    if ((pal = obj->obj_flags.origin) && charExists(pal))
    {
      pal->cRooms--;
      if (pal->in_room == obj->in_room)
      {
        pal->sendln("The runes upon the ground shatter with a burst of magic!\r\nYour unholy desecration has been destroyed!");
        victim = pal;
      }
      else
        csendf(pal, "You sense your desecration of %s has been destroyed!", DC::getInstance()->world[obj->in_room].name);
    }
    ch->sendln("The runes upon the ground shatter with a burst of magic!\r\nThe unholy desecration has been destroyed!");
    act("The runes upon the ground shatter with a burst of magic!\r\n$n has destroyed the unholy desecration here!", ch, 0, victim, TO_ROOM, NOTVICT);
    extract_obj(obj);
    return eSUCCESS;
  }
  else if (obj && !victim)
  { // targetting a random object
    ch->sendln("Nothing happens.");
    return eSUCCESS;
  }
  else
  { // possible weapon spell
    int dam, align;
    int weap_spell = obj ? WEAR_WIELD : 0;
    set_cantquit(ch, victim);
    if (IS_EVIL(ch))
      victim = ch;
    if (IS_NEUTRAL(victim) || IS_GOOD(victim))
    {
      act("$N does not seem to be affected.", ch, 0, victim, TO_CHAR, 0);
      return eFAILURE;
    }
    align = GET_ALIGNMENT(victim);
    if (align < 0)
      align = 0 - align;
    dam = 370 + align / 10;

    return damage(ch, victim, dam, TYPE_COLD, SPELL_DISPEL_EVIL, weap_spell);
  }
}

/* DISPEL GOOD */

int spell_dispel_good(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  if (obj && obj->in_room && obj->obj_flags.value[0] == SPELL_CONSECRATE)
  { // consecrate object
    Character *pal = nullptr;
    if ((pal = obj->obj_flags.origin) && charExists(pal))
    {
      pal->cRooms--;
      if (pal->in_room == obj->in_room)
      {
        pal->sendln("The runes upon the ground glow brightly, then fade to nothing.\r\nYour holy consecration has been destroyed!");
        victim = pal;
      }
      else
        csendf(pal, "You sense your consecration of %s has been destroyed!", DC::getInstance()->world[obj->in_room].name);
    }
    ch->sendln("Runes upon the ground glow brightly, then fade to nothing.\r\nThe holy consecration has been destroyed!");
    act("Runes upon the ground glow brightly, then fade to nothing.\r\n$n has destroyed the holy consecration here!", ch, 0, victim, TO_ROOM, NOTVICT);
    extract_obj(obj);
    return eSUCCESS;
  }
  else if (obj && !victim)
  { // targetting a random object
    ch->sendln("Nothing happens.");
    return eSUCCESS;
  }
  else
  { // possible weapon spell
    int dam, align;
    int weap_spell = obj ? WEAR_WIELD : 0;
    set_cantquit(ch, victim);
    if (IS_GOOD(ch))
      victim = ch;
    if (IS_NEUTRAL(victim) || IS_EVIL(victim))
    {
      act("$N does not seem to be affected.", ch, 0, victim, TO_CHAR, 0);
      return eFAILURE;
    }
    align = GET_ALIGNMENT(victim);
    if (align < 0)
      align = 0 - align;
    dam = 370 + align / 10;

    return damage(ch, victim, dam, TYPE_COLD, SPELL_DISPEL_GOOD, weap_spell);
  }
}

/* CALL LIGHTNING */

int spell_call_lightning(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  extern struct weather_data weather_info;
  set_cantquit(ch, victim);

  if (OUTSIDE(ch) && (weather_info.sky >= SKY_RAINING))
  {
    dam = dice(MIN((int)GET_MANA(ch), 725), 1);
    return damage(ch, victim, dam, TYPE_ENERGY, SPELL_CALL_LIGHTNING);
  }
  return eFAILURE;
}

/* HARM */

int spell_harm(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;
  set_cantquit(ch, victim);
  dam = 165;

  return damage(ch, victim, dam, TYPE_MAGIC, SPELL_HARM, weap_spell);
}

/* POWER HARM */

int spell_power_harm(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;
  set_cantquit(ch, victim);

  if (IS_EVIL(ch))
    dam = 530;
  else if (IS_NEUTRAL(ch))
    dam = 430;
  else
    dam = 330;

  return damage(ch, victim, dam, TYPE_MAGIC, SPELL_POWER_HARM, weap_spell);
}

/* DIVINE FURY */

int spell_divine_fury(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  set_cantquit(ch, victim);

  dam = 100 + GET_ALIGNMENT(ch) / 4;
  if (dam < 0)
  {
    dam = 0;
  }

  return damage(ch, victim, dam, TYPE_MAGIC, SPELL_DIVINE_FURY);
}

/* TELEPORT */

// TODO - make this spell have an effect based on skill level
int spell_teleport(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  auto &arena = DC::getInstance()->arena_;
  room_t to_room;
  char buf[100];

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim sent to teleport!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if ((ch->in_room >= 0 && ch->in_room <= DC::getInstance()->top_of_world) &&
      ch->room().isArena() && arena.isPotato())
  {
    ch->sendln("You can't teleport in potato arenas!");
    return eFAILURE;
  }
  if (ch->isPlayerObjectThief())
  {
    ch->sendln("Your attempt to transport stolen goods through planes of magic fails!!");
    return eFAILURE;
  }

  if (isSet(DC::getInstance()->world[victim->in_room].room_flags, TELEPORT_BLOCK) ||
      IS_AFFECTED(victim, AFF_SOLIDITY))
  {
    ch->sendln("You find yourself unable to.");
    if (ch != victim)
    {
      sprintf(buf, "%s just tried to teleport you.\r\n", GET_SHORT(ch));
      victim->send(buf);
    }
    return eFAILURE;
  }

  if (ch->room().isArena())
  {
    // If the ch is in a general arena and self-teleporting, there's a 25% chance they will teleport to the deathtrap.
    if (ch == victim && ch->in_room >= Arena::ARENA_LOW && ch->in_room <= Arena::ARENA_HIGH && number(1, 4) == 1)
    {
      to_room = real_room(Arena::ARENA_DEATHTRAP);
    }
    else
    {
      // Find a valid room in whatever arena area the ch is in
      int cur_zone = DC::getInstance()->world[ch->in_room].zone;
      int cur_zone_bottom = DC::getInstance()->zones.value(cur_zone).getRealBottom();
      int cur_zone_top = DC::getInstance()->zones.value(cur_zone).getRealTop();

      do
      {
        to_room = number(cur_zone_bottom, cur_zone_top);
      } while (real_room(to_room) == DC::NOWHERE);
    }
  }
  else
  {
    do
    {
      to_room = number<room_t>(1, DC::getInstance()->top_of_world);
    } while (!DC::getInstance()->rooms.contains(to_room) ||
             isSet(DC::getInstance()->world[to_room].room_flags, PRIVATE) ||
             isSet(DC::getInstance()->world[to_room].room_flags, IMP_ONLY) ||
             isSet(DC::getInstance()->world[to_room].room_flags, NO_TELEPORT) ||
             isSet(DC::getInstance()->world[to_room].room_flags, ARENA) ||
             DC::getInstance()->world[to_room].sector_type == SECT_UNDERWATER ||
             DC::getInstance()->zones.value(DC::getInstance()->world[to_room].zone).isNoTeleport() ||
             ((IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN)) ? (DC::getInstance()->zones.value(DC::getInstance()->world[to_room].zone).isTown()) : false) ||
             (IS_AFFECTED(victim, AFF_CHAMPION) && (isSet(DC::getInstance()->world[to_room].room_flags, CLAN_ROOM) ||
                                                    (to_room >= 1900 && to_room <= 1999))) ||
             // NPCs can only teleport within the same continent
             (IS_NPC(victim) &&
              DC::getInstance()->zones.value(DC::getInstance()->world[victim->in_room].zone).continent != DC::getInstance()->zones.value(DC::getInstance()->world[to_room].zone).continent));
  }

  if ((IS_NPC(victim)) && (ch->isPlayer()))
    victim->add_memory(GET_NAME(ch), 'h');

  act("$n slowly fades out of existence.", victim, 0, 0, TO_ROOM, 0);
  move_char(victim, to_room);
  act("$n slowly fades into existence.", victim, 0, 0, TO_ROOM, 0);

  do_look(victim, "");
  return eSUCCESS;
}

/* BLESS */

int spell_bless(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  if (!ch && (!victim || !obj))
  {
    DC::getInstance()->logentry(QStringLiteral("Null ch or victim and obj in bless."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  // Bless on an object should do something (perhaps reduce weight by X?)
  if (obj)
  {
    if ((5 * level > GET_OBJ_WEIGHT(obj)) &&
        (GET_POS(ch) != position_t::FIGHTING))
    {
      SET_BIT(obj->obj_flags.extra_flags, ITEM_BLESS);
      act("$p briefly glows.", ch, obj, 0, TO_CHAR, 0);
    }
  }
  else
  {
    if (victim->affected_by_spell(SPELL_BLESS))
      affect_from_char(victim, SPELL_BLESS);
    victim->sendln("You feel blessed.");

    if (victim != ch)
      act("$N receives the blessing from your god.", ch, nullptr, victim, TO_CHAR, 0);

    af.type = SPELL_BLESS;
    af.duration = 6 + skill / 3;
    af.modifier = 1 + skill / 45;
    af.location = APPLY_HITROLL;
    af.bitvector = -1;
    affect_to_char(victim, &af);

    af.location = APPLY_SAVING_MAGIC;
    af.modifier = 1 + skill / 18;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* PARALYZE */

int spell_paralyze(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  char buf[180];
  int retval;

  set_cantquit(ch, victim);
  if (victim->affected_by_spell(SPELL_PARALYZE))
    return eFAILURE;
  if (victim->affected_by_spell(SPELL_VILLAINY) && victim->affected_by_spell(SPELL_VILLAINY)->modifier >= 70)
  {
    act("$N seems unaffected.", ch, 0, victim, TO_CHAR, 0);
    act("Your gods protect you from $n's spell.", ch, 0, victim, TO_VICT, 0);
    return eSUCCESS;
  }

  if (victim->affected_by_spell(SPELL_SLEEP))
  {
    if (number(1, 6) < 5)
    {
      int retval = 0;
      if (number(0, 1))
      {
        ch->sendln("The combined magics fizzle!");
        if (GET_POS(victim) == position_t::SLEEPING)
        {
          victim->sendln("You are awakened by a burst of $6energy$R!");
          act("$n is awakened in a burst of $6energy$R!", victim, nullptr, nullptr, TO_ROOM, 0);
          victim->setSitting();
        }
      }
      else
      {
        ch->sendln("The combined magics cause an explosion!");
        retval = damage(ch, ch, number(5, 10), 0, TYPE_MAGIC);
      }
      return retval;
    }
  }

  if (isSet(victim->immune, ISR_PARA) || IS_AFFECTED(victim, AFF_NO_PARA))
  {
    act("$N absorbs your puny spell and seems no different!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N absorbs $n's puny spell and seems no different!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    if (IS_PC(victim))
      act("You absorb $n's puny spell and are no different!", ch, nullptr, victim, TO_VICT, 0);
    return eSUCCESS;
  }

  /* save the newbies! */
  if (IS_PC(ch) && IS_PC(victim) && (victim->getLevel() < 10))
  {
    ch->sendln("Your cold-blooded act causes your magic to misfire!");
    victim = ch;
  }

  if (malediction_res(ch, victim, SPELL_PARALYZE))
  {
    act("$N resists your attempt to paralyze $M!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to paralyze $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to paralyze you!", ch, nullptr, victim, TO_VICT, 0);
    if ((!victim->fighting) && GET_POS(victim) > position_t::SLEEPING && victim != ch)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }
    return eSUCCESS;
  }

  if (IS_NPC(victim) && (victim->getLevel() == 0))
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim level in spell_paralyze."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  // paralyze vs sleep modifier
  int save = 0 - (int)((double)get_saves(victim, SAVE_TYPE_MAGIC) * 0.5);
  if (victim->affected_by_spell(SPELL_SLEEP))
    save = -15; // Above check takes care of sleep.
  int spellret = saves_spell(ch, victim, save, SAVE_TYPE_MAGIC);

  /* ideally, we would do a dice roll to see if spell hits or not */
  if (spellret >= 0 && (victim != ch))
  {
    act("$N seems to be unaffected!", ch, nullptr, victim, TO_CHAR, 0);
    if (IS_PC(victim))
    {
      act("$n tried to paralyze you!", ch, nullptr, victim, TO_VICT, 0);
    }
    if (IS_NPC(victim) && (!victim->fighting) && GET_POS(victim) > position_t::SLEEPING)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }
    return eSUCCESS;
  }

  /* if they are too big - do a dice roll to see if they backfire */
  if (IS_PC(ch) && IS_PC(victim) && ((level - victim->getLevel()) > 10) &&
      (!IS_MINLEVEL_PC(ch, DEITY) || victim->getLevel() >= ch->getLevel()))
  {
    act("$N seems to be unaffected!", ch, nullptr, victim, TO_CHAR, 0);
    victim = ch;
    if (saves_spell(ch, ch, -100, SAVE_TYPE_MAGIC) >= 0)
    {
      act("Your magic misfires but you are saved!", ch, nullptr, victim, TO_CHAR, 0);
      return eSUCCESS;
    }
    act("Your cruel heart causes your magic to misfire!", ch, nullptr, victim, TO_CHAR, 0);
  }

  // Finish off any singing performances (bard)
  if (IS_SINGING(victim))
    do_sing(victim, "stop");

  act("$n seems to be paralyzed!", victim, 0, 0, TO_ROOM, INVIS_NULL);
  victim->sendln("Your entire body rebels against you and you are paralyzed!");

  if (IS_PC(victim))
  {
    sprintf(buf, "%s was just paralyzed.", qPrintable(victim->getName()));
    DC::getInstance()->logentry(buf, OVERSEER, DC::LogChannel::LOG_MORTAL);
  }

  af.type = SPELL_PARALYZE;
  af.location = APPLY_NONE;
  af.modifier = 0;
  af.duration = 2;
  if (skill > 75)
    af.duration++;
  if (skill > 50)
    af.duration++;
  if (spellcraft(ch, SPELL_PARALYZE))
    af.duration++;
  af.bitvector = AFF_PARALYSIS;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* BLINDNESS */

int spell_blindness(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  int retval;
  set_cantquit(ch, victim);

  if (victim->affected_by_spell(SPELL_VILLAINY) && victim->affected_by_spell(SPELL_VILLAINY)->modifier >= 90)
  {
    act("$N seems unaffected.", ch, 0, victim, TO_CHAR, 0);
    act("Your gods protect you from $n's spell.", ch, 0, victim, TO_VICT, 0);
    return eSUCCESS;
  }

  if (malediction_res(ch, victim, SPELL_BLINDNESS))
  {
    act("$N resists your attempt to blind $M!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to blind $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to blind you!", ch, nullptr, victim, TO_VICT, 0);
    if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > position_t::SLEEPING)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_BLINDNESS) || IS_AFFECTED(victim, AFF_BLIND))
    return eFAILURE;

  int spellret = saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC);

  if (spellret >= 0 && (victim != ch))
  {
    act("$N seems to be unaffected!", ch, nullptr, victim, TO_CHAR, 0);
    if (IS_PC(victim))
    {
      act("$n tried to blind you!", ch, nullptr, victim, TO_VICT, 0);
    }
    if (IS_NPC(victim) && (!victim->fighting) && GET_POS(victim) > position_t::SLEEPING)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }
    return eSUCCESS;
  }

  act("$n seems to be blinded!", victim, 0, 0, TO_ROOM, INVIS_NULL);
  victim->sendln("You have been blinded!");

  af.type = SPELL_BLINDNESS;
  af.location = APPLY_HITROLL;
  af.modifier = victim->has_skill(SKILL_BLINDFIGHTING) ? skill_success(victim, 0, SKILL_BLINDFIGHTING) ? -10 : -20 : -20;
  af.duration = 1 + (skill > 33) + (skill > 60);
  af.bitvector = AFF_BLIND;
  affect_to_char(victim, &af);

  af.location = APPLY_AC;
  af.modifier = victim->has_skill(SKILL_BLINDFIGHTING) ? skill_success(victim, 0, SKILL_BLINDFIGHTING) ? skill / 4 : skill / 2 : skill / 2;
  affect_to_char(victim, &af);
  return eSUCCESS;
}

/* CREATE FOOD */

int spell_create_food(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  class Object *tmp_obj;

  if (GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_PALADIN)
    tmp_obj = clone_object(real_object(8));
  else
    tmp_obj = clone_object(real_object(7));

  tmp_obj->obj_flags.value[0] += skill / 2;

  obj_to_room(tmp_obj, ch->in_room);

  act("$p suddenly appears.", ch, tmp_obj, 0, TO_ROOM, INVIS_NULL);
  act("$p suddenly appears.", ch, tmp_obj, 0, TO_CHAR, INVIS_NULL);
  return eSUCCESS;
}

/* CREATE WATER */

int spell_create_water(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int water;
  if (!ch || !obj)
  {
    DC::getInstance()->logentry(QStringLiteral("Null ch or obj in create_water."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (GET_ITEM_TYPE(obj) == ITEM_DRINKCON)
  {
    if ((obj->obj_flags.value[2] != LIQ_WATER) && (obj->obj_flags.value[1] != 0))
    {
      obj->obj_flags.value[2] = LIQ_SLIME;
    }
    else
    {
      water = 20 + skill * 2;

      /* Calculate water it can contain, or water created */
      water = MIN(obj->obj_flags.value[0] - obj->obj_flags.value[1], water);

      if (water > 0)
      {
        obj->obj_flags.value[2] = LIQ_WATER;
        obj->obj_flags.value[1] += water;
        act("$p is filled.", ch, obj, 0, TO_CHAR, 0);
      }
    }
  }
  return eSUCCESS;
}

/* REMOVE PARALYSIS */

int spell_remove_paralysis(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim in remove_paralysis!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_PARALYZE) &&
      number(1, 100) < (80 + skill / 6))
  {
    affect_from_char(victim, SPELL_PARALYZE);
    ch->sendln("Your spell is successful!");
    victim->sendln("Your movement returns!");
  }
  else
    ch->sendln("Your spell fails to return the victim's movement.");

  return eSUCCESS;
}

/* REMOVE BLIND */

int spell_remove_blind(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim in remove_blind!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (number(1, 100) < (80 + skill / 6))
  {
    if (!victim->affected_by_spell(SPELL_BLINDNESS) && !IS_AFFECTED(victim, AFF_BLIND))
    {
      if (victim == ch)
        ch->sendln("Seems you weren't blind after all.");
      else
        act("Seems $N wasn't blind after all.", ch, 0, victim, TO_CHAR, 0);
    }
    if (victim->affected_by_spell(SPELL_BLINDNESS))
    {
      affect_from_char(victim, SPELL_BLINDNESS);
      victim->sendln("Your vision returns!");
      if (victim != ch)
        act("$N can see again!", ch, 0, victim, TO_CHAR, 0);
    }
    if (IS_AFFECTED(victim, AFF_BLIND))
    {
      REMBIT(victim->affected_by, AFF_BLIND);
      victim->sendln("Your vision returns!");
      if (victim != ch)
        act("$N can see again!", ch, 0, victim, TO_CHAR, 0);
    }
  }
  else
  {
    if (ch == victim)
    {
      ch->sendln("Your spell fails to return your vision!");
    }
    else
    {
      act("Your spell fails to return $N's vision!", ch, 0, victim, TO_CHAR, 0);
    }
  }

  return eSUCCESS;
}

/* CURE CRITIC */

int spell_cure_critic(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int healpoints;
  char buf[MAX_STRING_LENGTH * 2], dammsg[MAX_STRING_LENGTH];

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim in cure_critic."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (GET_RACE(victim) == RACE_UNDEAD)
  {
    ch->sendln("Healing spells are useless on the undead.");
    return eFAILURE;
  }

  if (GET_RACE(victim) == RACE_GOLEM)
  {
    ch->sendln("The heavy magics surrounding this being prevent healing.");
    return eFAILURE;
  }

  if (!can_heal(ch, victim, SPELL_CURE_CRITIC))
    return eFAILURE;
  healpoints = dam_percent(skill, 100 + getRealSpellDamage(ch));
  healpoints = number(healpoints - (healpoints / 10), healpoints + (healpoints / 10));

  if ((healpoints + victim->getHP()) > hit_limit(victim))
  {
    healpoints = hit_limit(victim) - victim->getHP();
    victim->fillHPLimit();
  }
  else
  {
    victim->addHP(healpoints);
  }

  update_pos(victim);

  sprintf(dammsg, "$B%d$R damage", healpoints);

  if (ch != victim)
  {
    sprintf(buf, "You heal %s of the more critical wounds on $N.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "several" : "several");
    act(buf, ch, 0, victim, TO_CHAR, 0);
    sprintf(buf, "$n heals %s of your more critical wounds.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "several" : "several");
    act(buf, ch, 0, victim, TO_VICT, 0);
    sprintf(buf, "$n heals | of the more critical wounds on $N.");
    send_damage(buf, ch, 0, victim, dammsg, "$n heals several of the more critical wounds on $N.", TO_ROOM);
  }
  else
  {
    sprintf(buf, "You heal %s of your more critical wounds.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "several" : "several");
    act(buf, ch, 0, 0, TO_CHAR, 0);
    sprintf(buf, "$n heals | of $s more critical wounds.");
    send_damage(buf, ch, 0, victim, dammsg, "$n heals several of $s more critical wounds.", TO_ROOM);
  }

  return eSUCCESS;
}

/* CURE LIGHT */

int spell_cure_light(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int healpoints;
  char buf[MAX_STRING_LENGTH * 2], dammsg[MAX_STRING_LENGTH];

  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null ch or victim in cure_light!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (GET_RACE(victim) == RACE_UNDEAD)
  {
    ch->sendln("Healing spells are useless on the undead.");
    return eFAILURE;
  }

  if (GET_RACE(victim) == RACE_GOLEM)
  {
    ch->sendln("The heavy magics surrounding this being prevent healing.");
    return eFAILURE;
  }

  if (!can_heal(ch, victim, SPELL_CURE_LIGHT))
    return eFAILURE;
  healpoints = dam_percent(skill, 25 + getRealSpellDamage(ch));
  healpoints = number(healpoints - (healpoints / 10), healpoints + (healpoints / 10));
  if ((healpoints + victim->getHP()) > hit_limit(victim))
  {
    healpoints = hit_limit(victim) - victim->getHP();
    victim->fillHPLimit();
  }
  else
    victim->addHP(healpoints);

  update_pos(victim);

  sprintf(dammsg, "$B%d$R damage", healpoints);

  if (ch != victim)
  {
    sprintf(buf, "You heal %s small cuts and scratches on $N.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "several" : "several");
    act(buf, ch, 0, victim, TO_CHAR, 0);
    sprintf(buf, "$n heals %s of your small cuts and scratches.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "several" : "several");
    act(buf, ch, 0, victim, TO_VICT, 0);
    sprintf(buf, "$n heals | of small cuts and scratches on $N.");
    send_damage(buf, ch, 0, victim, dammsg, "$n heals several small cuts and scratches on $N.", TO_ROOM);
  }
  else
  {
    sprintf(buf, "You heal %s of your small cuts and scratches.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "several" : "several");
    act(buf, ch, 0, 0, TO_CHAR, 0);
    sprintf(buf, "$n heals | of $s small cuts and scratches.");
    send_damage(buf, ch, 0, victim, dammsg, "$n heals several of $s small cuts and scratches.", TO_ROOM);
  }
  return eSUCCESS;
}

/* CURSE */

int spell_curse(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  auto &arena = DC::getInstance()->arena_;
  struct affected_type af;
  int retval;

  if (obj && obj != ch->equipment[WEAR_WIELD] && obj != ch->equipment[WEAR_SECOND_WIELD])
  { // hack for weapon spells
    SET_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
    act("$p glows $4red$R momentarily, before returning to its original color.", ch, obj, 0, TO_CHAR, 0);
    /* LOWER ATTACK DICE BY -1 */
    if (obj->obj_flags.type_flag == ITEM_WEAPON)
    {
      if (obj->obj_flags.value[2] > 1)
      {
        obj->obj_flags.value[2]--;
      }
      //		 else {
      //		  ch->sendln("Your curse has failed.");
      //		 }
    }
  }
  else
  {
    if (!ch || !victim)
      return eFAILURE;

    // Curse in a prize arena follows rules of offensive spells
    if (ch->room().isArena() && (arena.isPrize() || arena.isChaos()))
    {
      if (!can_be_attacked(ch, victim) || !can_attack(ch))
        return eFAILURE;
    }

    set_cantquit(ch, victim);

    if (victim->affected_by_spell(SPELL_HEROISM) && victim->affected_by_spell(SPELL_HEROISM)->modifier >= 90)
    {
      act("$N seems unaffected.", ch, 0, victim, TO_CHAR, 0);
      act("Your gods protect you from $n's spell.", ch, 0, victim, TO_VICT, 0);
      return eSUCCESS;
    }

    int duration = 0, save = 0;
    if (skill < 41)
    {
      duration = 1;
      save = -5;
    }
    else if (skill < 51)
    {
      duration = 2;
      save = -10;
    }
    else if (skill < 61)
    {
      duration = 3;
      save = -15;
    }
    else if (skill < 71)
    {
      duration = 4;
      save = -20;
    }
    else if (skill < 81)
    {
      duration = 5;
      save = -25;
    }
    else
    {
      duration = 6;
      save = -30;
    }

    if (IS_PC(victim) && victim->getLevel() < 11)
    {
      ch->sendln("The curse fizzles!");
      return eSUCCESS;
    }

    if (malediction_res(ch, victim, SPELL_CURSE) || (IS_PC(victim) && victim->getLevel() >= IMMORTAL))
    {
      act("$N resists your attempt to curse $M!", ch, nullptr, victim, TO_CHAR, 0);
      act("$N resists $n's attempt to curse $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
      act("You resist $n's attempt to curse you!", ch, nullptr, victim, TO_VICT, 0);
    }
    else
    {
      if (victim->affected_by_spell(SPELL_CURSE))
        return eFAILURE;

      af.type = SPELL_CURSE;
      af.duration = duration;
      af.modifier = save;
      af.location = APPLY_SAVES;
      if (skill > 70)
        af.bitvector = AFF_CURSE;
      else
        af.bitvector = -1;

      affect_to_char(victim, &af);
      act("$n briefly reveals a $4red$R aura!", victim, 0, 0, TO_ROOM, 0);
      act("You feel very uncomfortable as a curse takes hold of you.", victim, 0, 0, TO_CHAR, 0);
    }

    if (IS_NPC(victim) && !victim->fighting)
    {
      mob_suprised_sayings(victim, ch);
      retval = attack(victim, ch, 0);
      SWAP_CH_VICT(retval);
      return retval;
    }

    // Curse in a prize arena follows rules of offensive spells
    if (ch->room().isArena() && (arena.isPrize() || arena.isChaos()))
    {
      if (!can_be_attacked(ch, victim) || !can_attack(ch))
        return eFAILURE;

      retval = attack(ch, victim, 0);
      return retval;
    }
  }
  return eSUCCESS;
}

/* DETECT EVIL */

int spell_detect_evil(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim in detect evil!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (ch != victim && skill < 70)
  {
    ch->sendln("You aren't practiced enough to be able to cast on others.");
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_DETECT_EVIL))
    affect_from_char(victim, SPELL_DETECT_EVIL);

  af.type = SPELL_DETECT_EVIL;
  af.duration = 10 + skill / 2;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_DETECT_EVIL;

  affect_to_char(victim, &af);
  victim->sendln("You become more conscious of the evil around you.");
  act("$n looks to be more conscious of the evil around $m.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

/* DETECT GOOD */

int spell_detect_good(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim sent to detect_good."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (ch != victim && skill < 70)
  {
    ch->sendln("You aren't practiced enough to be able to cast on others.");
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_DETECT_GOOD))
    affect_from_char(victim, SPELL_DETECT_GOOD);

  af.type = SPELL_DETECT_GOOD;
  af.duration = 10 + skill / 2;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_DETECT_GOOD;

  affect_to_char(victim, &af);
  victim->sendln("You are now able to truly recognize the good in others.");
  act("$n looks to be more conscious of the evil around $m.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

/* true SIGHT */

int spell_true_sight(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim sent to detect_good."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_true_SIGHT))
    affect_from_char(victim, SPELL_true_SIGHT);

  if (IS_AFFECTED(victim, AFF_true_SIGHT))
    return eFAILURE;

  af.type = SPELL_true_SIGHT;
  af.duration = 6 + skill / 2;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_true_SIGHT;

  affect_to_char(victim, &af);
  victim->sendln("You feel your vision enhanced with an incredibly keen perception.");
  return eSUCCESS;
}

/* DETECT INVISIBILITY */

int spell_detect_invisibility(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim sent to detect_good."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_DETECT_INVISIBLE))
    affect_from_char(victim, SPELL_DETECT_INVISIBLE);

  if (IS_AFFECTED(victim, AFF_DETECT_INVISIBLE))
    return eFAILURE;

  af.type = SPELL_DETECT_INVISIBLE;
  af.duration = 12 + skill / 2;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_DETECT_INVISIBLE;

  affect_to_char(victim, &af);
  victim->sendln("Your eyes tingle, allowing you to see the invisible.");
  if (ch != victim)
    csendf(ch, "%s's eyes tingle briefly.\r\n", GET_SHORT(victim));
  return eSUCCESS;
}

/* INFRAVISION */

int spell_infravision(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim sent to detect_good."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_INFRAVISION))
    affect_from_char(victim, SPELL_INFRAVISION);

  if (IS_AFFECTED(victim, AFF_INFRARED))
    return eFAILURE;

  af.type = SPELL_INFRAVISION;
  af.duration = 12 + skill / 2;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_INFRARED;

  affect_to_char(victim, &af);
  victim->sendln("Your eyes glow $B$4red$R.");
  if (ch != victim)
    csendf(ch, "%s's eyes glow $B$4red$R.\r\n", GET_SHORT(victim));

  return eSUCCESS;
}

/* DETECT MAGIC */

int spell_detect_magic(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim sent to detect_magic."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_DETECT_MAGIC))
    affect_from_char(victim, SPELL_DETECT_MAGIC);

  af.type = SPELL_DETECT_MAGIC;
  af.duration = 6 + skill / 2;
  af.modifier = skill;
  af.location = APPLY_NONE;
  af.bitvector = AFF_DETECT_MAGIC;

  affect_to_char(victim, &af);
  victim->sendln("Your vision temporarily blurs, your focus shifting to the metaphysical realm.");
  if (ch != victim)
    csendf(ch, "%s's eyes appear to blur momentarily.\r\n", GET_SHORT(victim));
  return eSUCCESS;
}

/* HASTE */

int spell_haste(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim sent to haste"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_HASTE) || IS_AFFECTED(victim, AFF_HASTE))
  {
    act("$N is already moving fast enough.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  af.type = SPELL_HASTE;
  af.duration = skill / 10;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_HASTE;

  affect_to_char(victim, &af);
  victim->sendln("You feel fast!");
  act("$n begins to move faster.", victim, 0, 0, TO_ROOM, 0);
  return eSUCCESS;
}

/* DETECT POISON */

// TODO - make this use skill for addtional effects
int spell_detect_poison(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  if (!ch && (!victim || !obj))
  {
    DC::getInstance()->logentry(QStringLiteral("Null ch or victim and obj in bless."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (victim)
  {
    if (victim == ch)
      if (IS_AFFECTED(victim, AFF_POISON))
        ch->sendln("You can sense poison in your blood.");
      else
        ch->sendln("You feel healthy.");
    else if (IS_AFFECTED(victim, AFF_POISON))
    {
      act("You sense that $E is poisoned.", ch, 0, victim, TO_CHAR, 0);
    }
    else
    {
      act("You sense that $E is healthy.", ch, 0, victim, TO_CHAR, 0);
    }
  }
  else
  { /* It's an object */
    if ((obj->obj_flags.type_flag == ITEM_DRINKCON) ||
        (obj->obj_flags.type_flag == ITEM_FOOD))
    {
      if (obj->obj_flags.value[3])
        act("Poisonous fumes are revealed.", ch, 0, 0, TO_CHAR, 0);
      else
        ch->sendln("It looks very delicious.");
    }
    else
    {
      ch->sendln("There is nothing much that poison would do on this.");
    }
  }
  return eSUCCESS;
}

/* ENCHANT ARMOR - CURRENTLY INACTIVE */

int spell_enchant_armor(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  /*int i;*/

  ch->sendln("This spell being revamped.  Sorry.");
  return eFAILURE;

  if ((GET_ITEM_TYPE(obj) == ITEM_ARMOR) &&
      !isSet(obj->obj_flags.extra_flags, ITEM_ENCHANTED))
  {

    SET_BIT(obj->obj_flags.extra_flags, ITEM_ENCHANTED);

    obj->obj_flags.value[1] += 1 + (level >= DC::MAX_MORTAL_LEVEL);

    if (IS_GOOD(ch))
    {
      SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_EVIL);
      act("$p glows $B$3blue$R.", ch, obj, 0, TO_CHAR, 0);
    }
    else if (IS_EVIL(ch))
    {
      SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_GOOD);
      act("$p glows $B$4red$R.", ch, obj, 0, TO_CHAR, 0);
    }
    else
    {
      act("$p glows $5yellow$R.", ch, obj, 0, TO_CHAR, 0);
    }
  }
  return eSUCCESS;
}

/* ENCHANT WEAPON - CURRENTLY INACTIVE */

int spell_enchant_weapon(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  if (!ch || !obj)
  {
    DC::getInstance()->logentry(QStringLiteral("Null ch or obj in enchant_weapon!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  ch->sendln("This spell being revamped, sorry.");
  return eFAILURE;

  if ((GET_ITEM_TYPE(obj) == ITEM_WEAPON) && !isSet(obj->obj_flags.extra_flags, ITEM_MAGIC))
  {
    if (!obj->affected.isEmpty())
      return eFAILURE;

    obj->affected = QList<obj_affected_type>(2);
    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 4 + (level >= 18) + (level >= 38) + (level >= 48) + (level >= DEITY);
    obj->affected[1].location = APPLY_DAMROLL;
    obj->affected[1].modifier = 4 + (level >= 20) + (level >= 40) + (level >= DC::MAX_MORTAL_LEVEL) + (level >= DEITY);
    SET_BIT(obj->obj_flags.extra_flags, ITEM_MAGIC);

    if (IS_GOOD(ch))
    {
      SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_EVIL);
      act("$p glows $B$1blue$R.", ch, obj, 0, TO_CHAR, 0);
    }
    else if (IS_EVIL(ch))
    {
      SET_BIT(obj->obj_flags.extra_flags, ITEM_ANTI_GOOD);
      act("$p glows $B$4red$R.", ch, obj, 0, TO_CHAR, 0);
    }
    else
    {
      act("$p glows $B$5yellow$R.", ch, obj, 0, TO_CHAR, 0);
    }
  }
  if (GET_ITEM_TYPE(obj) == ITEM_MISSILE)
  {
    if (!obj->obj_flags.value[2] && !obj->obj_flags.value[3])
    {
      obj->obj_flags.value[2] = (level > 20) + (level > 30) + (level > 40) + (level > 45) + (level > 49);
      obj->obj_flags.value[3] = obj->obj_flags.value[2];
    }
  }
  return eSUCCESS;
}

/* MANA - Potion & Immortal Only */

int spell_mana(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int mana;

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim sent to mana!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }
  mana = victim->getLevel() * 4;
  GET_MANA(victim) += mana;

  if (GET_MANA(victim) > GET_MAX_MANA(victim))
    GET_MANA(victim) = GET_MAX_MANA(victim);

  update_pos(victim);
  victim->sendln("You feel magical energy fill your mind!");
  return eSUCCESS;
}

/* HEAL */

int spell_heal(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int healy;
  char buf[MAX_STRING_LENGTH * 2], dammsg[MAX_STRING_LENGTH];

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim sent to heal!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }
  /* Adding paladin ability to heal others back in.
   * if (GET_CLASS(ch) == CLASS_PALADIN && victim != ch)
   * {
   *   ch->sendln("You cannot target others with this spell.");
   *   return eFAILURE;
   * }
   */

  if (GET_RACE(victim) == RACE_UNDEAD)
  {
    ch->sendln("Healing spells are useless on the undead.");
    return eFAILURE;
  }
  if (GET_RACE(victim) == RACE_GOLEM)
  {
    ch->sendln("The heavy magics surrounding this being prevent healing.");
    return eFAILURE;
  }

  if (!can_heal(ch, victim, SPELL_HEAL))
    return eFAILURE;
  healy = dam_percent(skill, 250 + getRealSpellDamage(ch));
  healy = number(healy - (healy / 10), healy + (healy / 10));
  victim->addHP(healy);

  if (victim->getHP() >= hit_limit(victim))
  {
    healy += hit_limit(victim) - victim->getHP();
    victim->fillHPLimit();
  }

  update_pos(victim);

  sprintf(dammsg, " of $B%d$R damage", healy);

  if (ch != victim)
  {
    sprintf(buf, "Your incantation heals $N%s.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
    act(buf, ch, 0, victim, TO_CHAR, 0);
    sprintf(buf, "$n calls forth an incantation that heals you%s.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
    act(buf, ch, 0, victim, TO_VICT, 0);
    sprintf(buf, "$n calls forth an incantation that heals $N|.");
    send_damage(buf, ch, 0, victim, dammsg, "", TO_ROOM);
  }
  else
  {
    sprintf(buf, "Your incantation heals you%s.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
    act(buf, ch, 0, 0, TO_CHAR, 0);
    sprintf(buf, "$n calls forth an incantation that heals $m|.");
    send_damage(buf, ch, 0, victim, dammsg, "", TO_ROOM);
  }
  return eSUCCESS;
}

/* POWER HEAL */

int spell_power_heal(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int healy;
  char buf[MAX_STRING_LENGTH * 2], dammsg[MAX_STRING_LENGTH];

  if (!victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null victim sent to power heal!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (GET_RACE(victim) == RACE_UNDEAD)
  {
    ch->sendln("Healing spells are useless on the undead.");
    return eFAILURE;
  }
  if (GET_RACE(victim) == RACE_GOLEM)
  {
    ch->sendln("The heavy magics surrounding this being prevent healing.");
    return eFAILURE;
  }

  if (!can_heal(ch, victim, SPELL_POWER_HEAL))
    return eFAILURE;
  healy = dam_percent(skill, 300 + getRealSpellDamage(ch));
  healy = number(healy - (healy / 10), healy + (healy / 10));
  victim->addHP(healy);

  if (victim->getHP() >= hit_limit(victim))
  {
    healy += hit_limit(victim) - victim->getHP();
    victim->fillHPLimit();
  }

  update_pos(victim);

  sprintf(dammsg, " of $B%d$R damage", healy);

  if (ch != victim)
  {
    sprintf(buf, "Your powerful incantation heals $N%s.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
    act(buf, ch, 0, victim, TO_CHAR, 0);
    sprintf(buf, "$n calls forth a powerful incantation that heals you%s.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
    act(buf, ch, 0, victim, TO_VICT, 0);
    sprintf(buf, "$n calls forth a powerful incantation that heals $N|.");
    send_damage(buf, ch, 0, victim, dammsg, "", TO_ROOM);
  }
  else
  {
    sprintf(buf, "Your powerful incantation heals you%s.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
    act(buf, ch, 0, 0, TO_CHAR, 0);
    sprintf(buf, "$n calls forth a powerful incantation that heals $m|.");
    send_damage(buf, ch, 0, victim, dammsg, "", TO_ROOM);
  }

  return eSUCCESS;
}

/* FULL HEAL */

int spell_full_heal(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  assert(victim);
  int healamount = 0;
  char buf[MAX_STRING_LENGTH * 2], dammsg[MAX_STRING_LENGTH];

  if (GET_RACE(victim) == RACE_UNDEAD)
  {
    ch->sendln("Healing spells are useless on the undead.");
    return eFAILURE;
  }
  if (GET_RACE(victim) == RACE_GOLEM)
  {
    ch->sendln("The heavy magics surrounding this being prevent healing.");
    return eFAILURE;
  }

  if (!can_heal(ch, victim, SPELL_FULL_HEAL))
    return eFAILURE;

  healamount = 10 * (skill / 2 + 5) + getRealSpellDamage(ch);
  if (GET_ALIGNMENT(ch) < -349)
    healamount -= 2 * (skill / 2 + 5);
  else if (GET_ALIGNMENT(ch) > 349)
    healamount += (skill / 2 + 5);

  healamount = number(healamount - (healamount / 10), healamount + (healamount / 10));
  victim->addHP(healamount);

  if (victim->getHP() >= hit_limit(victim))
  {
    healamount += hit_limit(victim) - victim->getHP();
    victim->fillHPLimit();
  }

  update_pos(victim);

  sprintf(dammsg, " of $B%d$R damage", healamount);

  if (ch != victim)
  {
    sprintf(buf, "You call forth the magic of the gods to restore $N%s.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
    act(buf, ch, 0, victim, TO_CHAR, 0);
    sprintf(buf, "$n calls forth the magic of the gods to restore you%s.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
    act(buf, ch, 0, victim, TO_VICT, 0);
    sprintf(buf, "$n calls forth the magic of the gods to restore $N|.");
    send_damage(buf, ch, 0, victim, dammsg, "", TO_ROOM);
  }
  else
  {
    sprintf(buf, "You call forth the magic of the gods to restore you%s.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
    act(buf, ch, 0, 0, TO_CHAR, 0);
    sprintf(buf, "$n calls forth the magic of the gods to restore $m|.");
    send_damage(buf, ch, 0, victim, dammsg, "", TO_ROOM);
  }

  return eSUCCESS;
}

/* INVISIBILITY */

int spell_invisibility(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  assert((ch && obj) || victim);

  if (obj)
  {
    if (CAN_WEAR(obj, TAKE))
    {
      if (!isSet(obj->obj_flags.extra_flags, ITEM_INVISIBLE))
      {
        act("$p turns invisible.", ch, obj, 0, TO_CHAR, 0);
        act("$p turns invisible.", ch, obj, 0, TO_ROOM, INVIS_NULL);
        SET_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
      }
    }
    else
    {
      act("You fail to make $p invisible.", ch, obj, 0, TO_CHAR, 0);
    }
  }
  else
  { /* Then it is a PC | NPC */

    if (victim->affected_by_spell(SPELL_INVISIBLE))
      affect_from_char(victim, SPELL_INVISIBLE);

    if (IS_AFFECTED(victim, AFF_INVISIBLE))
      return eFAILURE;

    act("$n slowly fades out of existence.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    victim->sendln("You slowly fade out of existence.");

    af.type = SPELL_INVISIBLE;
    af.duration = (int)(skill / 3.75);
    af.modifier = -10 + skill / 6;
    af.location = APPLY_AC;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* LOCATE OBJECT */

int spell_locate_object(uint8_t level, Character *ch, char *arg, Character *victim, class Object *obj, int skill)
{
  class Object *i;
  char name[256];
  char buf[MAX_STRING_LENGTH];
  char tmpname[256];
  char *tmp;
  int j, n;
  int total;
  int number;

  assert(ch);
  if (!arg)
    return eFAILURE;

  one_argument(arg, name);

  strncpy(tmpname, name, 256);
  tmp = tmpname;
  if ((number = get_number(&tmp)) < 0)
    number = 0;

  total = j = (int)(skill / 1.5);

  uint64_t skipped_nosee = 0, skipped_nolocate = 0, skipped_other = 0, skipped_god = 0, skipped_nowhere = 0;
  for (i = DC::getInstance()->object_list, n = 0; i && (j > 0) && (number > 0); i = i->next)
  {
    // TODO
    // Removed for now because it's keep locate spell from seeing portals or corpses
    //	  if (i->item_number == -1) {
    //		  continue;
    //	  }
    //
    if (IS_OBJ_STAT(i, ITEM_NOSEE))
    {
      if (isexact(tmp, i->Name()))
      {
        skipped_nosee++;
      }
      continue;
    }

    if (isSet(i->obj_flags.more_flags, ITEM_NOLOCATE))
    {
      if (isexact(tmp, i->Name()))
      {
        skipped_nolocate++;
      }
      continue;
    }

    Character *owner = 0;
    int room = 0;
    if (i->equipped_by)
    {
      owner = i->equipped_by;
      room = owner->in_room;
    }

    else if (i->carried_by)
      owner = i->carried_by;

    else if (i->in_room)
      room = i->in_room;

    else if (i->in_obj && i->in_obj->equipped_by)
      owner = i->in_obj->equipped_by;

    else if (i->in_obj && i->in_obj->carried_by)
      owner = i->in_obj->carried_by;

    else if (i->in_obj && i->in_obj->in_room)
      room = i->in_obj->in_room;

    // If owner, PC, with desc and not con_playing or wizinvis,
    if (owner && owner->player && is_in_game(owner) &&
        (owner->player->wizinvis > ch->getLevel()))
    {
      if (isexact(tmp, i->Name()))
      {
        skipped_other++;
      }
      continue;
    }

    // Skip objs in god rooms
    if (room >= 1 && room <= 47)
    {
      if (isexact(tmp, i->Name()))
      {
        skipped_god++;
      }
      continue;
    }

    buf[0] = 0;
    if (isexact(tmp, i->Name()))
    {
      if (i->carried_by)
      {
        sprintf(buf, "%s carried by %s.\r\n", i->short_description,
                PERS(i->carried_by, ch));
      }
      else if (i->in_obj)
      {
        sprintf(buf, "%s is in %s.\r\n", i->short_description,
                i->in_obj->short_description);
      }
      else if (i->in_room != DC::NOWHERE)
      {
        sprintf(buf, "%s is in %s.\r\n", i->short_description,
                DC::getInstance()->world[i->in_room].name);
      }
      else if (i->equipped_by != nullptr)
      {
        sprintf(buf, "%s is equipped by someone.\r\n",
                i->short_description);
      }
      else
      {
        skipped_nowhere++;
        continue;
      }

      if (buf[0] != 0)
      {
        n++;
        if (n >= number)
        {
          ch->send(buf);
          j--;
        }
      }
    }
  }

  if (j == total)
    ch->sendln("There appears to be no such object.");

  if (j == 0)
    ch->sendln("The tremendous amount of information leaves you very confused.");

  if (ch->isImmortalPlayer())
  {
    ch->send(fmt::format("Skipped god:{} other:{} nolocate:{} nosee:{} DC::NOWHERE:{}\r\n", skipped_god, skipped_other, skipped_nolocate, skipped_nosee, skipped_nowhere));
  }

  return eSUCCESS;
}

/* POISON */

int spell_poison(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  int retval = eSUCCESS;
  bool endy = false;

  if (victim)
  {
    if (IS_AFFECTED(victim, AFF_POISON))
    {
      act("$N's insides are already being eaten away by poison!", ch, nullptr, victim, TO_CHAR, 0);
      endy = true;
    }
    else if (isSet(victim->immune, ISR_POISON) ||
             malediction_res(ch, victim, SPELL_POISON) ||
             (IS_PC(victim) && victim->getLevel() >= IMMORTAL))
    {
      act("$N resists your attempt to poison $M!", ch, nullptr, victim, TO_CHAR, 0);
      act("$N resists $n's attempt to poison $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
      act("You resist $n's attempt to poison you!", ch, nullptr, victim, TO_VICT, 0);
      endy = true;
    }
    set_cantquit(ch, victim);
    if (!endy)
    {
      af.type = SPELL_POISON;
      af.duration = skill / 10;
      if (IS_NPC(ch))
      {
        af.modifier = -123;
        af.origin = {};
      }
      else
      {
        af.modifier = {};
        af.origin = ch;
      }
      af.location = APPLY_NONE;
      af.bitvector = AFF_POISON;
      affect_join(victim, &af, false, false);
      victim->sendln("You feel very sick.");
      act("$N looks very sick.", ch, 0, victim, TO_CHAR, 0);
    }
    if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > position_t::SLEEPING)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }
  }
  else
  { /* Object poison */
    if ((obj->obj_flags.type_flag == ITEM_DRINKCON) ||
        (obj->obj_flags.type_flag == ITEM_FOOD))
    {
      act("$p glows $2green$R for a moment, before returning to its original colour.", ch, obj, 0, TO_CHAR, 0);
      obj->obj_flags.value[3] = 1;
    }
    else
    {
      ch->sendln("Nothing special seems to happen.");
    }
  }
  return retval;
}

/* PROTECTION FROM EVIL */

int spell_protection_from_evil(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  assert(victim);
  int duration = skill ? skill / 3 : level / 3;
  int modifier = level + 10;

  /* keep spells from stacking */
  if (IS_AFFECTED(victim, AFF_PROTECT_EVIL) ||
      IS_AFFECTED(victim, AFF_PROTECT_GOOD) ||
      victim->affected_by_spell(SPELL_PROTECT_FROM_GOOD))
    return eFAILURE;

  if (victim->affected_by_spell(SPELL_PROTECT_FROM_EVIL))
  {
    act("$N is already protected from evil.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  // Used to identify PFE from godload_defender(), obj vnum 556
  if (skill == 150)
  {
    duration = 4;
    modifier = 60;
  }

  af.type = SPELL_PROTECT_FROM_EVIL;
  af.duration = duration;
  af.modifier = modifier;
  af.location = APPLY_NONE;
  af.bitvector = AFF_PROTECT_EVIL;
  affect_to_char(victim, &af);
  victim->sendln("You have a righteous, protected feeling!");
  act("A dark, $6pulsing$R aura surrounds $n.", victim, 0, 0, TO_ROOM, INVIS_NULL);

  return eSUCCESS;
}

/* PROTECTION FROM GOOD */

int spell_protection_from_good(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  assert(victim);

  int duration = skill ? skill / 3 : level / 3;
  int modifier = level + 10;

  /* keep spells from stacking */
  if (IS_AFFECTED(victim, AFF_PROTECT_GOOD) ||
      IS_AFFECTED(victim, AFF_PROTECT_EVIL) ||
      victim->affected_by_spell(SPELL_PROTECT_FROM_EVIL))
    return eFAILURE;

  if (victim->affected_by_spell(SPELL_PROTECT_FROM_GOOD))
  {
    act("$N is already protected from good.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  af.type = SPELL_PROTECT_FROM_GOOD;
  af.duration = duration;
  af.modifier = modifier;
  af.location = APPLY_NONE;
  af.bitvector = AFF_PROTECT_GOOD;
  affect_to_char(victim, &af);
  victim->sendln("You feel yourself wrapped in a protective mantle of evil.");
  act("A light, $B$6pulsing$R aura surrounds $n.", victim, 0, 0, TO_ROOM, INVIS_NULL);

  return eSUCCESS;
}

/* REMOVE CURSE */

int spell_remove_curse(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill, quint64 mana_cost)
{
  int j;
  assert(ch && (victim || obj));

  if (obj)
  {
    if (isSet(obj->obj_flags.extra_flags, ITEM_NODROP))
    {
      act("$p briefly glows $3blue$R.", ch, obj, 0, TO_CHAR, 0);
      REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
      if (DC::getInstance()->obj_index[obj->item_number].virt == 514)
      {
        int i = 0;
        for (i = 0; i < obj->affected.size(); i++)
          if (obj->affected[i].location == APPLY_MANA_REGEN)
            return eSUCCESS; // only do it once
        SET_BIT(obj->obj_flags.extra_flags, ITEM_HUM);
        Character *t = obj->equipped_by;
        int z = -1;

        if (t->equipment[WEAR_FINGER_L] == obj)
          z = WEAR_FINGER_L;
        else
          z = WEAR_FINGER_R;
        if (t)
          obj_to_char(t->unequip_char(z), t);
        add_obj_affect(obj, APPLY_MANA_REGEN, 2);
        if (t)
          wear(t, obj, 0);

        act(
            "With the restrictive curse lifted, $p begins to hum with renewed power!",
            ch, obj, 0, TO_ROOM, 0);
        act(
            "With the restrictive curse lifted, $p begins to hum with renewed power!",
            ch, obj, 0, TO_CHAR, 0);
      }
    }
    return eSUCCESS;
  }

  quint64 curses_removed = 0;
  /* Then it is a PC | NPC */
  if (victim->affected_by_spell(SPELL_CURSE))
  {
    act("$n briefly glows $4red$R, then $3blue$R.", victim, 0, 0, TO_ROOM, 0);
    act("You feel better.", victim, 0, 0, TO_CHAR, 0);
    affect_from_char(victim, SPELL_CURSE);
    if (!mana_cost)
      return eSUCCESS;
    curses_removed++;
  }

  for (j = 0; j < MAX_WEAR; j++)
  {
    if ((obj = victim->equipment[j]) && isSet(obj->obj_flags.extra_flags, ITEM_NODROP))
    {
      if (!curses_removed || GET_MANA(victim) > mana_cost)
      {
        if (curses_removed++)
          GET_MANA(victim) -= mana_cost;
        if (skill > 70 && DC::getInstance()->obj_index[obj->item_number].virt == 514)
        {
          int i = 0;
          for (i = 0; i < obj->affected.size(); i++)
            if (obj->affected[i].location == APPLY_MANA_REGEN)
              break; // only do it once
          SET_BIT(obj->obj_flags.extra_flags, ITEM_HUM);
          add_obj_affect(obj, APPLY_MANA_REGEN, 2);
          victim->mana_regen += 2;
          act("With the restrictive curse lifted, $p begins to hum with renewed power!", victim, obj, 0, TO_ROOM, 0);
          act("With the restrictive curse lifted, $p begins to hum with renewed power!", victim, obj, 0, TO_CHAR, 0);
        }
        act("$p briefly glows $3blue$R.", victim, obj, 0, TO_CHAR, 0);
        act("$p carried by $n briefly glows $3blue$R.", victim, obj, 0, TO_ROOM, 0);
        REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
        if (!mana_cost)
          return eSUCCESS;
      }
    }
  }

  for (obj = victim->carrying; obj; obj = obj->next_content)
  {
    if (isSet(obj->obj_flags.extra_flags, ITEM_NODROP))
    {
      if (!curses_removed || GET_MANA(victim) > mana_cost)
      {
        if (curses_removed++)
          GET_MANA(victim) -= mana_cost;
        act("$p carried by $n briefly glows $3blue$R.", victim, obj, 0, TO_ROOM, 0);
        act("$p briefly glows $3blue$R.", victim, obj, 0, TO_CHAR, 0);
        if (skill > 70 && DC::getInstance()->obj_index[obj->item_number].virt == 514)
        {
          int i = 0;
          for (i = 0; i < obj->affected.size(); i++)
            if (obj->affected[i].location == APPLY_MANA_REGEN)
              break; // only do it once
          SET_BIT(obj->obj_flags.extra_flags, ITEM_HUM);
          add_obj_affect(obj, APPLY_MANA_REGEN, 2);
          act("With the restrictive curse lifted, $p begins to hum with renewed power!", victim, obj, 0, TO_ROOM, 0);
          act("With the restrictive curse lifted, $p begins to hum with renewed power!", victim, obj, 0, TO_CHAR, 0);
        }
        REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
        if (!mana_cost)
          return eSUCCESS;
      }
    }
  }

  if (victim->affected_by_spell(SPELL_ATTRITION))
  {
    if (!curses_removed || GET_MANA(victim) > mana_cost)
    {
      if (curses_removed++)
        GET_MANA(victim) -= mana_cost;
      act("$n briefly glows $4red$R, then $3blue$R.", victim, 0, 0, TO_ROOM, 0);
      act("The curse of attrition afflicting you has been lifted!", victim, 0, 0, TO_CHAR, 0);
      affect_from_char(victim, SPELL_ATTRITION);
      if (!mana_cost)
        return eSUCCESS;
    }
  }

  return eSUCCESS;
}

/* REMOVE POISON */

int spell_remove_poison(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  assert(ch && (victim || obj));

  if (victim)
  {
    if (victim->affected_by_spell(SPELL_DEBILITY))
    {
      act("$n looks better.", victim, 0, 0, TO_ROOM, 0);
      act("You feel less debilitated.", victim, 0, 0, TO_CHAR, 0);
      affect_from_char(victim, SPELL_DEBILITY);
      return eSUCCESS;
    }
    if (victim->affected_by_spell(SPELL_POISON))
    {
      affect_from_char(victim, SPELL_POISON);
      act("A warm feeling runs through your body.", victim,
          0, 0, TO_CHAR, 0);
      act("$n looks better.", victim, 0, 0, TO_ROOM, 0);
    }
  }
  else
  {
    if ((obj->obj_flags.type_flag == ITEM_DRINKCON) ||
        (obj->obj_flags.type_flag == ITEM_FOOD))
    {
      obj->obj_flags.value[3] = 0;
      act("The $p steams briefly.", ch, obj, 0, TO_CHAR, 0);
    }
  }
  return eSUCCESS;
}

bool find_spell_shield(Character *ch, Character *victim)
{
  if (IS_AFFECTED(victim, AFF_FIRESHIELD))
  {
    if (ch == victim)
      ch->sendln("You are already protected by a shield of fire.");
    else
      act("$N is already protected by a shield of fire.", ch, 0, victim, TO_CHAR, INVIS_NULL);

    return true;
  }

  if (IS_AFFECTED(victim, AFF_LIGHTNINGSHIELD))
  {
    if (ch == victim)
      ch->sendln("You are already protected by a shield of lightning.");
    else
      act("$N is already protected by a shield of lightning.", ch, 0, victim, TO_CHAR, INVIS_NULL);

    return true;
  }

  if (IS_AFFECTED(victim, AFF_FROSTSHIELD))
  {
    if (ch == victim)
      ch->sendln("You are already protected by a $1frost shield$R.");
    else
      act("$N is already protected by a $1frost shield$R.", ch, 0, victim, TO_CHAR, INVIS_NULL);

    return true;
  }

  if (IS_AFFECTED(victim, AFF_ACID_SHIELD))
  {
    if (ch == victim)
      ch->sendln("You are already protected by a shield of acid.");
    else
      act("$N is already protected by a shield of acid.", ch, 0, victim, TO_CHAR, INVIS_NULL);

    return true;
  }

  return false;
}

/* FIRESHIELD */

int spell_fireshield(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (find_spell_shield(ch, victim) && IS_PC(victim))
    return eFAILURE;

  if (!victim->affected_by_spell(SPELL_FIRESHIELD))
  {
    act("$n is surrounded by $B$4flames$R.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    act("You are surrounded by $B$4flames$R.", victim, 0, 0, TO_CHAR, 0);

    af.type = SPELL_FIRESHIELD;
    af.duration = 1 + skill / 23;
    af.modifier = skill;
    af.location = APPLY_NONE;
    af.bitvector = AFF_FIRESHIELD;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* MEND GOLEM */

int spell_mend_golem(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int heal;
  char dammsg[30];
  struct follow_type *fol;
  for (fol = ch->followers; fol; fol = fol->next)
    if (IS_NPC(fol->follower) && DC::getInstance()->mob_index[fol->follower->mobdata->nr].virt == 8)
    {
      heal = (int)(GET_MAX_HIT(fol->follower) * (0.12 + level / 1000.0));
      heal = number(heal - (heal / 10), heal + (heal / 10));

      fol->follower->addHP(heal);

      if (GET_HIT(fol->follower) > GET_MAX_HIT(fol->follower))
      {
        heal += GET_MAX_HIT(fol->follower) - GET_HIT(fol->follower);
        fol->follower->fillHP();
      }
      sprintf(dammsg, "$B%d$R", heal);

      send_damage("$n focuses $s magical energy and | of the scratches on $s golem are fixed.", ch, 0, 0,
                  dammsg, "$n focuses $s magical energy and many of the scratches on $s golem are fixed.", TO_ROOM);
      send_damage("You focus your magical energy and | of the scratches on your golem are fixed.", ch, 0, 0,
                  dammsg, "You focus your magical enery and many of the scratches on your golem are fixed.", TO_CHAR);
      return eSUCCESS;
    }
  ch->sendln("You don't have a golem.");
  return eSUCCESS;
}

/* CAMOUFLAGE (for items) */

int cast_camouflague(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_camouflague(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_camouflague(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
    {
      return eFAILURE;
    }
    if (!tar_ch)
    {
      tar_ch = ch;
    }
    return spell_camouflague(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch;
         tar_ch = tar_ch->next_in_room)
      spell_camouflague(level, ch, tar_ch, 0, skill);
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_camouflague!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* FARSIGHT (for items) */

int cast_farsight(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_farsight(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_farsight(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
    {
      return eFAILURE;
    }
    if (!tar_ch)
    {
      tar_ch = ch;
    }
    return spell_farsight(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch;
         tar_ch = tar_ch->next_in_room)
      spell_farsight(level, ch, tar_ch, 0, skill);
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_farsight!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eSUCCESS;
}

/* FREEFLOAT (for items) */

int cast_freefloat(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_freefloat(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_freefloat(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
    {
      return eFAILURE;
    }
    if (!tar_ch)
    {
      tar_ch = ch;
    }
    return spell_freefloat(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch;
         tar_ch = tar_ch->next_in_room)
      ;
    spell_freefloat(level, ch, tar_ch, 0, skill);
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logmisc(QStringLiteral("Serious screw-up in cast_freefloat!"));
    break;
  }
  return eFAILURE;
}

/* INSOMNIA (for items) */

int cast_insomnia(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_insomnia(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_insomnia(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
    {
      return eFAILURE;
    }
    if (!tar_ch)
    {
      tar_ch = ch;
    }
    return spell_insomnia(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch;
         tar_ch = tar_ch->next_in_room)
      spell_insomnia(level, ch, tar_ch, 0, skill);
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_insomnia!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* SHADOWSLIP (for items) */

int cast_shadowslip(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_shadowslip(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_shadowslip(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
    {
      return eFAILURE;
    }
    if (!tar_ch)
    {
      tar_ch = ch;
    }
    return spell_shadowslip(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch;
         tar_ch = tar_ch->next_in_room)
      spell_shadowslip(level, ch, tar_ch, 0, skill);
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serios screw-up in cast_shadowslip!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* SANCTUARY (for items) */

int cast_sanctuary(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (GET_CLASS(ch) == CLASS_PALADIN && GET_ALIGNMENT(ch) < 351)
    {
      ch->sendln("You are not noble enough to cast it.");
      return eFAILURE;
    }
    if (GET_CLASS(ch) == CLASS_PALADIN && tar_ch != ch)
    {
      ch->sendln("You can only cast this spell on yourself.");
      return eFAILURE;
    }
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_SANCTUARY - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_sanctuary(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_SANCTUARY - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
          retval &= spell_sanctuary(level, ch, leader, 0, skill);
      }

      return retval;
    }
    return spell_sanctuary(level, ch, tar_ch, 0, skill);
    break;

    // Paladin casting restrictions above

  case SPELL_TYPE_POTION:
    return spell_sanctuary(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_sanctuary(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch;
         tar_ch = tar_ch->next_in_room)
      spell_sanctuary(level, ch, tar_ch, 0, skill);
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in sanctuary!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* CAMOUFLAGE */

// TODO - make this have effects based on skill
int spell_camouflague(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  if (victim->affected_by_spell(SPELL_CAMOUFLAGE))
  {
    act("$N is already hidden within the plant life.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("$n fades into the plant life.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("You fade into the plant life.", victim, 0, 0, TO_CHAR, 0);

  af.type = SPELL_CAMOUFLAGE;
  af.duration = 1 + skill / 10;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_CAMOUFLAGUE;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* FARSIGHT */

// TODO - make this gain effects based on skill
int spell_farsight(uint8_t level, Character *ch, Character *victim, class Object *tar_obj, int skill)
{
  struct affected_type af;
  if (victim->affected_by_spell(SPELL_FARSIGHT))
  {
    act("$N can already see far enough.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("Your eyesight improves.", victim, 0, 0, TO_CHAR, 0);

  af.type = SPELL_FARSIGHT;
  af.duration = 2 + level / 5;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_FARSIGHT;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* FREEFLOAT */

// TODO - make this gain effects based on skill
int spell_freefloat(uint8_t level, Character *ch, Character *victim, class Object *tar_obj, int skill)
{
  struct affected_type af;
  if (victim->affected_by_spell(SPELL_FREEFLOAT))
  {
    act("$N is already stable enough.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("You gain added stability.", victim, 0, 0, TO_CHAR, 0);

  af.type = SPELL_FREEFLOAT;
  af.duration = 12 + level / 4;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_FREEFLOAT;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* INSOMNIA */

// TODO - make this use skill
int spell_insomnia(uint8_t level, Character *ch, Character *victim, class Object *tar_obj, int skill)
{
  struct affected_type af;

  if (victim->affected_by_spell(SPELL_INSOMNIA))
  {
    act("$N is already wide awake.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("You suddenly feel wide awake.", victim, 0, 0, TO_CHAR, 0);

  af.type = SPELL_INSOMNIA;
  af.duration = 2 + level / 5;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_INSOMNIA;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* SHADOWSLIP */

// TODO - make this use skill
int spell_shadowslip(uint8_t level, Character *ch, Character *victim, class Object *tar_obj, int skill)
{
  struct affected_type af;
  if (victim->affected_by_spell(SPELL_SHADOWSLIP))
  {
    act("$N is already hidden amongst the shadows.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("Portals will no longer find you as your presence becomes obscured by shadows.", victim, 0, 0, TO_CHAR, 0);

  af.type = SPELL_SHADOWSLIP;
  af.duration = level / 4;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SHADOWSLIP;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* SANCTUARY */

int spell_sanctuary(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (IS_AFFECTED(victim, AFF_SANCTUARY))
  {
    act("$N is already sanctified.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  if (victim->affected_by_spell(SPELL_HOLY_AURA))
  {
    act("$N cannot be bestowed with that much power.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  if (!victim->affected_by_spell(SPELL_SANCTUARY))
  {
    act("$n is surrounded by a $Bwhite aura$R.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    act("You start $Bglowing$R.", victim, 0, 0, TO_CHAR, 0);
    af.type = SPELL_SANCTUARY;
    af.duration = 3 + skill / 18;
    af.modifier = 35;

    af.location = APPLY_NONE;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* SLEEP */

int spell_sleep(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  char buf[100];
  int retval;

  set_cantquit(ch, victim);

  if (victim->isPlayer() && victim->getLevel() <= 15)
  {
    ch->sendln("Oh come on....at least wait till $e's high enough level to have decent gear.");
    return eFAILURE;
  }

  /* You can't sleep someone higher level than you*/
  if (victim->affected_by_spell(SPELL_INSOMNIA) || IS_AFFECTED(victim, AFF_INSOMNIA))
  {
    act("$N does not look sleepy!", ch, nullptr, victim, TO_CHAR, 0);
    retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
    retval = SWAP_CH_VICT(retval);
    return retval;
  }
  if (victim->affected_by_spell(SPELL_SLEEP))
  {
    act("$N's mind still has the lingering effects of a past sleep spell active.",
        ch, nullptr, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_PARALYZE))
  {
    if (number(1, 20) < 19)
    {
      switch (number(1, 3))
      {
      case 1:
        act("$N does not look sleepy!", ch, nullptr, victim, TO_CHAR, 0);
        break;
      case 2:
      case 3:
        ch->sendln("The combined magics fizzle, and cause an explosion!");
        {
          act("$n wakes up in a burst of magical energies!", victim, nullptr, nullptr, TO_ROOM, 0);
          affect_from_char(victim, SPELL_PARALYZE);
        }
        break;
      }
      return eFAILURE;
    }
  }
  set_cantquit(ch, victim);

  if (number(1, 100) <= MIN(MAX((get_saves(ch, SAVE_TYPE_MAGIC) - get_saves(victim, SAVE_TYPE_MAGIC)) / 2, 1), 7))
  {
    act("$N resists your attempt to sleep $M!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to sleep $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to sleep you!", ch, nullptr, victim, TO_VICT, 0);
    if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > position_t::SLEEPING)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }

    return eFAILURE;
  }

  if (level < victim->getLevel())
  {
    snprintf(buf, 100, "%s laughs in your face at your feeble attempt.\r\n", GET_SHORT(victim));
    ch->send(buf);
    snprintf(buf, 100, "%s tries to make you sleep, but fails miserably.\r\n", GET_SHORT(ch));
    victim->send(buf);
    retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
    retval = SWAP_CH_VICT(retval);
    return retval;
  }

  if (IS_NPC(victim) || number(1, 2) == 1)
  {
    if (saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) < 0)
    {
      af.type = SPELL_SLEEP;
      af.duration = ch->getLevel() / 20;
      ;
      af.modifier = 1;
      af.location = APPLY_NONE;
      af.bitvector = AFF_SLEEP;
      affect_join(victim, &af, false, false);

      if (GET_POS(victim) > position_t::SLEEPING)
      {
        act("You feel very sleepy ..... zzzzzz", victim, 0, 0, TO_CHAR, 0);
        act("$n goes to sleep.", victim, 0, 0, TO_ROOM, INVIS_NULL);
        stop_fighting(victim);
        victim->setSleeping();
      }
      return eSUCCESS;
    }
    else
      act("$N does not look sleepy!", ch, nullptr, victim, TO_CHAR, 0);
  }
  else
    act("$N does not look sleepy!", ch, nullptr, victim, TO_CHAR, 0);

  retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
  retval = SWAP_CH_VICT(retval);
  return retval;
}

/* STRENGTH */

int spell_strength(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  struct affected_type *cur_af;

  assert(victim);

  int mod = 4 + (skill > 20) + (skill > 40) + (skill > 60) + (skill > 80);

  if ((cur_af = victim->affected_by_spell(SPELL_WEAKEN)))
  {
    af.type = cur_af->type;
    af.duration = cur_af->duration;
    af.modifier = cur_af->modifier + mod;
    af.location = cur_af->location;
    af.bitvector = cur_af->bitvector;
    affect_from_char(victim, SPELL_WEAKEN); // this makes cur_af invalid

    if (af.modifier < 0)
    { // it's not out yet, so put some back
      victim->sendln("You feel most of the magical weakness leave your body.");
      affect_to_char(victim, &af);
    }
    else
    {
      victim->sendln("You feel the magical weakness leave your body.");
    }

    return eSUCCESS;
  }

  if ((cur_af = victim->affected_by_spell(SPELL_STRENGTH)))
  {
    if (cur_af->modifier <= mod)
      affect_from_char(victim, SPELL_STRENGTH);
    else
    {
      ch->sendln("That person already has a stronger strength spell!");
      return eFAILURE;
    }
  }

  victim->sendln("You feel stronger.");
  act("$n's muscles bulge a bit and $e looks stronger.", victim, 0, 0, TO_ROOM, INVIS_NULL);

  af.type = SPELL_STRENGTH;
  af.duration = level / 2 + skill / 3;
  af.modifier = mod;
  af.location = APPLY_STR;
  af.bitvector = -1;
  affect_to_char(victim, &af);
  return eSUCCESS;
}

/* VENTRILOQUATE */

int spell_ventriloquate(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  /* Actual spell resides in cast_ventriloquate */
  return eSUCCESS;
}

/* WORD OF RECALL */

int spell_word_of_recall(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int location;
  char buf[200];
  clan_data *clan;
  struct clan_room_data *room;
  int found = 0;

  if (IS_AFFECTED(victim, AFF_SOLIDITY))
  {
    ch->sendln("You find yourself unable to.");
    if (ch != victim)
    {
      sprintf(buf, "%s just tried to recall you.\r\n", GET_SHORT(ch));
      victim->send(buf);
    }
    return eFAILURE;
  }
  assert(victim);
  if (victim->room().isArena())
  {
    ch->sendln("To the DEATH you wimp!");
    return eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_CHARM))
    return eFAILURE;
  if (IS_AFFECTED(victim, AFF_CURSE))
  {
    if (ch == victim)
      ch->sendln("Something blocks your attempt to recall.");
    else
    {
      act("Something blocks your attempt to recall $N.", ch, 0, victim, TO_CHAR, 0);
      act("Something blocks $n's attempt to recall you.", ch, 0, victim, TO_VICT, 0);
    }
    return eFAILURE;
  }

  if (victim->fighting && IS_PC(victim->fighting)) // PvP fight?
  {
    victim->sendln("The fight distracts you from casting word of recall!");
    return eFAILURE;
  }

  if (victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
  {
    victim->sendln("Your attempt to transport stolen goods through planes of magic fails!");
    return eFAILURE;
  }

  if (IS_NPC(victim))
    location = real_room(GET_HOME(victim));
  else
  {
    if (victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF) || victim->isPlayerGoldThief())
      location = real_room(START_ROOM);
    else
      location = real_room(GET_HOME(victim));

    if (IS_AFFECTED(victim, AFF_CANTQUIT))
      location = real_room(START_ROOM);
    else
      location = real_room(GET_HOME(victim));

    // make sure they aren't recalling into someone's chall
    if (isSet(DC::getInstance()->world[location].room_flags, CLAN_ROOM))
    {
      if (!victim->clan || !(clan = get_clan(victim)))
      {
        victim->sendln("The gods frown on you, and reset your home.");
        location = real_room(START_ROOM);
        GET_HOME(victim) = START_ROOM;
      }
      else
      {
        for (room = clan->rooms; room; room = room->next)
          if (room->room_number == GET_HOME(victim))
            found = 1;

        if (!found)
        {
          victim->sendln("The gods frown on you, and reset your home.");
          location = real_room(START_ROOM);
          GET_HOME(victim) = START_ROOM;
        }
      }
    }
  }

  if (location == -1)
  {
    victim->sendln("You are completely lost.");
    return eFAILURE;
  }

  if (isSet(DC::getInstance()->world[location].room_flags, CLAN_ROOM) && IS_AFFECTED(victim, AFF_CHAMPION))
  {
    victim->sendln("No recalling into a clan hall whilst Champion, go to the Tavern!");
    location = real_room(START_ROOM);
  }
  if (location >= 1900 && location <= 1999 && IS_AFFECTED(victim, AFF_CHAMPION))
  {
    victim->sendln("No recalling into a guild hall whilst Champion, go to the Tavern!");
    location = real_room(START_ROOM);
  }

  if (DC::getInstance()->zones.value(DC::getInstance()->world[victim->in_room].zone).continent != DC::getInstance()->zones.value(DC::getInstance()->world[location].zone).continent)
  {
    if (GET_MANA(victim) < use_mana(victim, skill))
    {
      victim->sendln("You don't posses the energy to travel that far.");
      GET_MANA(victim) += use_mana(victim, skill);
      return eFAILURE;
    }
    else
    {
      ch->sendln("The long distance drains additional mana from you.");
      GET_MANA(victim) -= use_mana(victim, skill);
    }
  }

  if (IS_PC(victim) && victim->player->golem && victim->player->golem->in_room == victim->in_room)
  {
    if (victim->mana < 50)
    {
      victim->sendln("You don't possses the energy to bring your golem along.");
    }
    else
    {
      if (victim->player->golem->fighting)
      {
        victim->sendln("Your golem is too distracted by something to follow.");
      }
      else
      {
        act("$n disappears.", victim->player->golem, 0, 0, TO_ROOM, INVIS_NULL);
        move_char(victim->player->golem, location);
        act("$n appears out of nowhere.", victim->player->golem, 0, 0, TO_ROOM, INVIS_NULL);
        GET_MANA(victim) -= 50;
      }
    }
  }
  /* a location has been found. */
  act("$n disappears.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  move_char(victim, location);
  act("$n appears out of nowhere.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  do_look(victim, "");
  return eSUCCESS;
}

/* WIZARD EYE */

int spell_wizard_eye(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int target;
  int original_loc;
  assert(ch && victim);

  if (isSet(DC::getInstance()->world[victim->in_room].room_flags, NO_MAGIC) ||
      (victim->isImmortalPlayer() && !ch->isImmortalPlayer()))
  {
    ch->sendln("Your vision is too clouded to make out anything.");
    return eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_FOREST_MELD))
  {
    ch->sendln("Your target's location is hidden by the forests.");
    return eFAILURE;
  }

  if (victim->affected_by_spell(SKILL_INNATE_EVASION))
  {
    ch->sendln("Your target evades your magical scrying!");
    return eFAILURE;
  }

  if (number(0, 100) > skill)
  {
    ch->sendln("Your spell fails to locate its target.");
    return eFAILURE;
  }

  original_loc = ch->in_room;
  target = victim->in_room;

  /* Detect Magic wiz-eye detection */
  if (victim->affected_by_spell(SPELL_DETECT_MAGIC) && victim->affected_by_spell(SPELL_DETECT_MAGIC)->modifier > 80)
    victim->sendln("You sense you are the target of magical scrying.");

  move_char(ch, target, false);
  ch->sendln("A vision forms in your mind... ");
  do_look(ch, "");
  move_char(ch, original_loc);
  return eSUCCESS;
}

/* EAGLE EYE */

int spell_eagle_eye(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int target;
  int original_loc;
  assert(ch && victim);

  if (!OUTSIDE(ch))
  {
    ch->sendln("Your eagle cannot find its way outside!");
    return eFAILURE;
  }

  if (!OUTSIDE(victim) ||
      (victim->isImmortalPlayer() && !ch->isImmortalPlayer()))
  {
    ch->sendln("Your eagle cannot scan the area.");
    return eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_INVISIBLE))
  {
    ch->sendln("Your eagle can't find the target.");
    return eFAILURE;
  }

  if (victim->affected_by_spell(SKILL_INNATE_EVASION))
  {
    ch->sendln("Your target evades the eagle's eyes!");
    return eFAILURE;
  }

  if (number(0, 100) > skill)
  {
    ch->sendln("Your eagle fails to locate its target.");
    return eFAILURE;
  }

  original_loc = ch->in_room;
  target = victim->in_room;

  move_char(ch, target, false);
  ch->sendln("You summon a large eagle to scan the area.\r\nThrough the eagle's eyes you see...");
  do_look(ch, "");
  move_char(ch, original_loc);
  return eSUCCESS;
}

/* SUMMON */

int spell_summon(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  unsigned target;
  int retval;
  assert(ch && victim);

  if (isSet(DC::getInstance()->world[victim->in_room].room_flags, SAFE))
  {
    ch->sendln("That person is in a safe area!");
    return eFAILURE;
  }

  if ((victim->getLevel() > MIN(MORTAL, level + 3)) && ch->getLevel() < IMPLEMENTER)
  {
    ch->sendln("You failed.");
    return eFAILURE;
  }

  if (IS_PC(ch))
    if (IS_NPC(victim) || !isSet(victim->player->toggles, Player::PLR_SUMMONABLE))
    {
      victim->sendln("Someone has tried to summon you!");
      ch->sendln("Something strange about that person prevents your summoning.");
      return eFAILURE;
    }
  if (IS_NPC(ch) && IS_NPC(victim))
    return eFAILURE;

  if ((IS_NPC(victim) && ch->getLevel() < IMPLEMENTER) ||
      isSet(DC::getInstance()->world[victim->in_room].room_flags, PRIVATE) ||
      isSet(DC::getInstance()->world[victim->in_room].room_flags, NO_SUMMON))
  {
    ch->sendln("You have failed to summon your target!");
    return eFAILURE;
  }

  if (IS_ARENA(ch->in_room))
    if (!IS_ARENA(victim->in_room))
    {
      ch->sendln("You can't summon someone INTO an arena!");
      return eFAILURE;
    }

  if (IS_ARENA(victim->in_room) && !IS_ARENA(ch->in_room))
  {
    ch->sendln("You can't summon someone OUT of an areana!");
    return eFAILURE;
  }

  if (number(1, 100) > 50 + skill / 2 ||
      victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF) || victim->isPlayerGoldThief())
  {
    ch->sendln("Your attempted summoning fails.");
    return eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_CHAMPION) && (isSet(DC::getInstance()->world[ch->in_room].room_flags, CLAN_ROOM) || (ch->in_room >= 1900 && ch->in_room <= 1999)))
  {
    ch->sendln("You cannot summon a Champion here.");
    return eFAILURE;
  }

  if (DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).continent != DC::getInstance()->zones.value(DC::getInstance()->world[victim->in_room].zone).continent)
  {
    if (GET_MANA(ch) < use_mana(ch, skill))
    {
      ch->sendln("You don't posses the energy to travel that far.");
      GET_MANA(ch) += use_mana(ch, skill);
      return eFAILURE;
    }
    else
    {
      ch->sendln("The long distance drains additional mana from you.");
      GET_MANA(ch) -= use_mana(ch, skill);
    }
  }

  act("$n disappears suddenly as a magical summoning draws their being.", victim, 0, 0, TO_ROOM, INVIS_NULL);

  target = ch->in_room;
  move_char(victim, target);
  act("$n arrives suddenly.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("$n has summoned you!", ch, 0, victim, TO_VICT, 0);
  do_look(victim, "");

  if (IS_NPC(victim) && victim->getLevel() >= ch->getLevel())
  {
    act("$n growls.", victim, 0, 0, TO_ROOM, 0);
    retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
    retval = SWAP_CH_VICT(retval);
    return retval;
  }
  else if (IS_NPC(victim))
  {
    act("$n freaks shit.", victim, 0, 0, TO_ROOM, 0);
    victim->add_memory(GET_NAME(ch), 'f');
    do_flee(victim, "");
  }
  return eSUCCESS;
}

/* CHARM PERSON - no longer operational */

int spell_charm_person(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  class Object *tempobj;

  ch->sendln("Disabled currently.");
  return eFAILURE;

  if (victim == ch)
  {
    ch->sendln("You like yourself even better!");
    return eFAILURE;
  }

  if (IS_PC(victim))
  {
    ch->sendln("You find yourself unable to charm this player.");
    return eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_CHARM) || IS_AFFECTED(ch, AFF_CHARM) || level <= victim->getLevel())
    return eFAILURE;

  if (circle_follow(victim, ch))
  {
    ch->sendln("Sorry, following in circles can not be allowed.");
    return eFAILURE;
  }

  if (many_charms(ch))
  {
    ch->sendln("How do you plan on controlling so many followers?");
    return eFAILURE;
  }

  if (isSet(victim->immune, ISR_CHARM) ||
      (IS_NPC(victim) && !ISSET(victim->mobdata->actflags, ACT_CHARM)))
  {
    act("$N laughs at your feeble charm attempt.", ch, nullptr, victim,
        TO_CHAR, 0);
    return eFAILURE;
  }

  if (saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) >= 0)
  {
    act("$N doesnt seem to be affected.", ch, nullptr, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if (victim->master)
    stop_follower(victim);

  add_follower(victim, ch);

  af.type = SPELL_CHARM_PERSON;

  if (GET_INT(victim))
    af.duration = 24 * 18 / GET_INT(victim);
  else
    af.duration = 24 * 18;

  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_CHARM;
  affect_to_char(victim, &af);

  act("Isn't $n just such a nice fellow?", ch, 0, victim, TO_VICT, 0);
  if (victim->equipment[WEAR_WIELD])
  {
    if (victim->equipment[WEAR_SECOND_WIELD])
    {
      tempobj = victim->unequip_char(WEAR_SECOND_WIELD);
      obj_to_room(tempobj, victim->in_room);
    }
    tempobj = victim->unequip_char(WEAR_WIELD);
    obj_to_room(tempobj, victim->in_room);
    act("$n's eyes dull and $s hands slacken dropping $s weapons.", victim, 0, 0, TO_ROOM, 0);
  }
  return eSUCCESS;
}

/* SENSE LIFE */

int spell_sense_life(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  assert(victim);

  if (victim->affected_by_spell(SPELL_SENSE_LIFE))
    affect_from_char(victim, SPELL_SENSE_LIFE);

  victim->sendln("You feel your awareness improve.");

  af.type = SPELL_SENSE_LIFE;
  af.duration = 18 + skill / 2;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SENSE_LIFE;
  affect_to_char(victim, &af);
  return eSUCCESS;
}

void show_obj_class_size_mini(Object *obj, Character *ch)
{
  for (int i = 12; i < 23; i++)
    if (isSet(obj->obj_flags.extra_flags, 1 << i))
      csendf(ch, " %s", Object::extra_bits.value(i).toStdString().c_str());
}

/* IDENFITY */

// TODO - make this use skill to affect amount of information provided
int spell_identify(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  char buf[MAX_STRING_LENGTH], buf2[256];
  int i;
  bool found;
  int value;

  assert(obj || victim);

  if (obj)
  {
    if (obj->in_room > DC::NOWHERE)
    {
      // it's an obj in a room.  If it's not a corpse, don't id it
      if (GET_ITEM_TYPE(obj) != ITEM_CONTAINER || obj->obj_flags.value[3] != 1)
      {
        ch->sendln("Your magical probing reveals nothing of interest.");
        return eSUCCESS;
      }
      ch->sendln("You probe the contents of the corpse magically....");
      // it's a corpse
      class Object *iobj;
      for (iobj = obj->contains; iobj; iobj = iobj->next_content)
      {
        if (!CAN_SEE_OBJ(ch, iobj))
          continue;
        ch->send(iobj->short_description);
        if (isSet(iobj->obj_flags.more_flags, ITEM_NO_TRADE))
        {
          ch->send(" $BNO_TRADE$R");
          show_obj_class_size_mini(iobj, ch);
        }
        ch->sendln("");
      }
      return eSUCCESS;
    }
    if (isSet(obj->obj_flags.extra_flags, ITEM_DARK) && ch->getLevel() < POWER)
    {
      ch->sendln("A magical aura around the item attempts to conceal its secrets.");
      return eFAILURE;
    }

    ch->sendln("You feel informed:");

    sprintf(buf, "Object '%s', Item type: ", qPrintable(obj->Name()));
    sprinttype(GET_ITEM_TYPE(obj), item_types, buf2);
    strcat(buf, buf2);
    strcat(buf, "\r\n");
    ch->send(buf);

    ch->send("Item is: ");
    sprintbit(obj->obj_flags.extra_flags, Object::extra_bits, buf);
    sprintbit(obj->obj_flags.more_flags, Object::more_obj_bits, buf2);
    strcat(buf, " ");
    strcat(buf, buf2);
    strcat(buf, "\r\n");
    ch->send(buf);

    ch->send("Worn by: ");
    sprintbit(obj->obj_flags.size, Object::size_bits, buf);
    strcat(buf, "\r\n");
    ch->send(buf);

    sprintf(buf, "Weight: %d, Value: %d, Level: %d\r\n", obj->obj_flags.weight, obj->obj_flags.cost, obj->obj_flags.eq_level);
    ch->send(buf);

    switch (GET_ITEM_TYPE(obj))
    {

    case ITEM_SCROLL:
    case ITEM_POTION:
      sprintf(buf, "Level %d spells of:\r\n", obj->obj_flags.value[0]);
      ch->send(buf);
      if (obj->obj_flags.value[1] >= 1)
      {
        sprinttype(obj->obj_flags.value[1] - 1, spells, buf);
        strcat(buf, "\r\n");
        ch->send(buf);
      }
      if (obj->obj_flags.value[2] >= 1)
      {
        sprinttype(obj->obj_flags.value[2] - 1, spells, buf);
        strcat(buf, "\r\n");
        ch->send(buf);
      }
      if (obj->obj_flags.value[3] >= 1)
      {
        sprinttype(obj->obj_flags.value[3] - 1, spells, buf);
        strcat(buf, "\r\n");
        ch->send(buf);
      }
      break;

    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(buf, "Has %d charges, with %d charges left.\r\n",
              obj->obj_flags.value[1],
              obj->obj_flags.value[2]);
      ch->send(buf);

      sprintf(buf, "Level %d spell of:\r\n", obj->obj_flags.value[0]);
      ch->send(buf);

      if (obj->obj_flags.value[3] >= 1)
      {
        sprinttype(obj->obj_flags.value[3] - 1, spells, buf);
        strcat(buf, "\r\n");
        ch->send(buf);
      }
      break;

    case ITEM_WEAPON:
      sprintf(buf, "Damage Dice are '%dD%d'\r\n",
              obj->obj_flags.value[1],
              obj->obj_flags.value[2]);
      ch->send(buf);
      break;

    case ITEM_INSTRUMENT:
      sprintf(buf, "Affects non-combat singing by '%d'\r\nAffects combat singing by '%d'\r\n",
              obj->obj_flags.value[0],
              obj->obj_flags.value[1]);
      ch->send(buf);
      break;

    case ITEM_MISSILE:
      sprintf(buf, "Damage Dice are '%dD%d'\r\nIt is +%d to arrow hit and +%d to arrow damage\r\n",
              obj->obj_flags.value[0],
              obj->obj_flags.value[1],
              obj->obj_flags.value[2],
              obj->obj_flags.value[3]);
      ch->send(buf);
      break;

    case ITEM_FIREWEAPON:
      sprintf(buf, "Bow is +%d to arrow hit and +%d to arrow damage.\r\n",
              obj->obj_flags.value[0],
              obj->obj_flags.value[1]);
      ch->send(buf);
      break;

    case ITEM_ARMOR:

      if (isSet(obj->obj_flags.extra_flags, ITEM_ENCHANTED))
      {
        value = (obj->obj_flags.value[0]) - (obj->obj_flags.value[1]);
      }
      else
      {
        value = obj->obj_flags.value[0];
      }

      sprintf(buf, "AC-apply is %d     Resistance to damage is %d\r\n",
              value, obj->obj_flags.value[2]);
      ch->send(buf);
      break;
    }

    found = false;

    for (i = 0; i < obj->affected.size(); i++)
    {
      if ((obj->affected[i].location != APPLY_NONE) &&
          (obj->affected[i].modifier != 0))
      {
        if (!found)
        {
          ch->sendln("Can affect you as:");
          found = true;
        }

        if (obj->affected[i].location < 1000)
          sprinttype(obj->affected[i].location, apply_types, buf2);
        else if (!get_skill_name(obj->affected[i].location / 1000).isEmpty())
          strcpy(buf2, get_skill_name(obj->affected[i].location / 1000).toStdString().c_str());
        else
          strcpy(buf2, "Invalid");
        sprintf(buf, "    Affects : %s By %d\r\n", buf2, obj->affected[i].modifier);
        ch->send(buf);
      }
    }
  }
  else
  { /* victim */

    if (IS_PC(victim))
    {
      sprintf(buf, "%d Years,  %d Months,  %d Days,  %d Hours old.\r\n",
              victim->age().year, victim->age().month,
              victim->age().day, victim->age().hours);
      ch->send(buf);

      sprintf(buf, "Race: ");
      sprinttype(victim->race, race_types, buf2);
      strcat(buf, buf2);
      ch->send(buf);

      sprintf(buf, "   Height %dcm  Weight %dpounds \r\n",
              GET_HEIGHT(victim), GET_WEIGHT(victim));
      ch->send(buf);

      if (victim->getLevel() > 9)
      {

        sprintf(buf, "Str%d,  Int %d,  Wis %d,  Dex %d,  Con %d\r\n",
                GET_STR(victim),
                GET_INT(victim),
                GET_WIS(victim),
                GET_DEX(victim),
                GET_CON(victim));
        ch->send(buf);
      }
    }
    else
    {
      ch->sendln("You learn nothing new.");
    }
  }
  return eSUCCESS;
}

/* ************************************************************************* *
 *                      NPC Spells (Breath Weapons)                          *
 * ************************************************************************* */

int spell_frost_breath(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int hpch;
  int retval;
  /*class Object *frozen;*/

  set_cantquit(ch, victim);

  hpch = ch->getHP();
  if (hpch < 10)
    hpch = 10;

  dam = number((hpch / 8) + 1, (hpch / 4));

  //	 if(saves_spell(ch, victim, 0, SAVE_TYPE_COLD) >= 0)
  //	   dam >>= 1;

  retval = damage(ch, victim, dam, TYPE_COLD, SPELL_FROST_BREATH);
  if (SOMEONE_DIED(retval))
  {
    /* The character's DEAD, don't mess with him */
    return retval;
  }

  /* And now for the damage on inventory */
  /*
  // TODO - make frost breath do something cool *pun!*

     if(number(0,100) < ch->getLevel())
     {
      if(!saves_spell(ch, victim, SAVING_BREATH) )
    {
      for(frozen=victim->carrying ;
      frozen && !(((frozen->obj_flags.type_flag==ITEM_DRINKCON) ||
        (frozen->obj_flags.type_flag==ITEM_FOOD) ||
        (frozen->obj_flags.type_flag==ITEM_POTION)) &&
        (number(0,2)==0)) ;
      frozen=frozen->next_content);
      if(frozen)
    {
      act("$o breaks.",victim,frozen,0,TO_CHAR, 0);
      extract_obj(frozen);
    }
    }
     }
  */
  return eSUCCESS;
}

int spell_acid_breath(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int hpch;
  int retval;

  int apply_ac(Character * ch, int eq_pos);

  set_cantquit(ch, victim);

  hpch = ch->getHP();
  if (hpch < 10)
    hpch = 10;

  dam = number((hpch / 8) + 1, (hpch / 4));

  //	 if(saves_spell(ch, victim, 0, SAVE_TYPE_ACID) >= 0)
  //	   dam >>= 1;

  retval = damage(ch, victim, dam, TYPE_ACID, SPELL_ACID_BREATH);
  if (SOMEONE_DIED(retval))
  {
    return retval;
  }

  /* And now for the damage on equipment */
  /*
  // TODO - make this do something cool
     if(number(0,100)<ch->getLevel())
     {
      if(!saves_spell(ch, victim, SAVING_BREATH))
    {
      for(damaged = 0; damaged<MAX_WEAR &&
      !((victim->equipment[damaged]) &&
       (victim->equipment[damaged]->obj_flags.type_flag!=ITEM_ARMOR) &&
       (victim->equipment[damaged]->obj_flags.value[0]>0) &&
       (number(0,2)==0)) ; damaged++);
      if(damaged<MAX_WEAR)
    {
      act("$o is damaged.",victim,victim->equipment[damaged],0,TO_CHAR,0);
      GET_AC(victim)-=apply_ac(victim,damaged);
      victim->equipment[damaged]->obj_flags.value[0]-=number(1,7);
      GET_AC(victim)+=apply_ac(victim,damaged);
      victim->equipment[damaged]->obj_flags.cost = 0;
    }
    }
     }
  */
  return eSUCCESS;
}

int spell_fire_breath(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int retval;

  act("$B$4You are $IENVELOPED$I$B$4 in scorching $B$4flames$R!$R", ch, 0, 0, TO_ROOM, 0);

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {
    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
        (IS_NPC(ch) ? IS_PC(tmp_victim) : true)) // if i'm a mob, don't hurt other mobs
    {
      if (GET_DEX(tmp_victim) > number(1, 100)) // roll vs dex dodged
      {
        ch->sendln("You dive out of the way of the main blast avoiding the inferno!");
        act("$n barely dives to the side avoiding the heart of the flame.", ch, 0, 0, TO_ROOM, 0);
        continue;
      }

      dam = dice(level, 8);

      //      if(saves_spell(ch,  tmp_victim, 0, SAVE_TYPE_FIRE) >= 0)
      //      dam >>= 1;

      retval = damage(ch, tmp_victim, dam, TYPE_FIRE, SPELL_FIRE_BREATH);
      if (SOMEONE_DIED(retval))
        return retval;
    }
    else if (DC::getInstance()->world[ch->in_room].zone == DC::getInstance()->world[tmp_victim->in_room].zone)
      tmp_victim->sendln("You feel a HOT blast of air.");
  }
  return eSUCCESS;
}

int spell_gas_breath(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int retval;

  act("You CHOKE on the gas fumes!",
      ch, 0, 0, TO_ROOM, 0);

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {
    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
        (IS_NPC(tmp_victim) || IS_NPC(ch)))
    {

      dam = dice(level, 6);

      // if(saves_spell(ch,  tmp_victim, 0, SAVE_TYPE_POISON) >= 0)
      //	dam >>= 1;

      retval = damage(ch, tmp_victim, dam, TYPE_POISON, SPELL_GAS_BREATH);
      if (isSet(retval, eCH_DIED))
        return retval;
    }
    else if (DC::getInstance()->world[ch->in_room].zone == DC::getInstance()->world[tmp_victim->in_room].zone)
      tmp_victim->sendln("You wanna choke on the smell in the air.");
  }
  return eSUCCESS;
}

int spell_lightning_breath(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int hpch;

  set_cantquit(ch, victim);

  hpch = ch->getHP();
  if (hpch < 10)
    hpch = 10;

  dam = number((hpch / 8) + 1, (hpch / 4));

  //	 if(saves_spell(ch, victim, 0, SAVE_TYPE_ENERGY) >= 0)
  //	   dam >>= 1;

  return damage(ch, victim, dam, TYPE_ENERGY, SPELL_LIGHTNING_BREATH);
}

/* **************************************************************** */

/* FEAR */

// TODO - make this use skill
int spell_fear(uint8_t level, Character *ch, Character *victim,
               class Object *obj, int skill)
{
  if (!victim || !ch)
    return eFAILURE;

  if (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_STUPID))
  {
    csendf(ch, "%s doesn't understand your psychological tactics.\r\n",
           GET_SHORT(victim));
    return eFAILURE;
  }

  if (GET_POS(victim) == position_t::SLEEPING)
  {
    ch->sendln("How do you expect a sleeping person to be scared?");
    return eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_FEARLESS))
  {
    act("$N seems to be quite unafraid of anything.", ch, 0, victim,
        TO_CHAR, 0);
    act("You laugh at $n's feeble attempt to frighten you.", ch, 0, victim,
        TO_VICT, 0);
    act("$N laughs at $n's futile attempt at scaring $M.", ch, 0, victim,
        TO_ROOM, NOTVICT);
    return eFAILURE;
  }

  if (isSet(victim->combat, COMBAT_BERSERK))
  {
    act(
        "$N looks at you with glazed over eyes, drools, and continues to fight!",
        ch, nullptr, victim, TO_CHAR, 0);
    act(
        "$N smiles madly, drool running down his chin as he ignores $n's magic!",
        ch, nullptr, victim, TO_ROOM, 0);
    act("You grin as $n realizes you have no target for his mental attack!",
        ch, nullptr, victim, TO_VICT, 0);
    return eFAILURE;
  }

  set_cantquit(ch, victim);

  if (victim->affected_by_spell(SPELL_HEROISM) && victim->affected_by_spell(SPELL_HEROISM)->modifier >= 70)
  {
    act("$N seems unaffected.", ch, 0, victim, TO_CHAR, 0);
    act("Your gods protect you from $n's spell.", ch, 0, victim, TO_VICT,
        0);
    return eFAILURE;
  }

  int retval = 0;

  if (malediction_res(ch, victim, SPELL_FEAR))
  {
    act("$N resists your attempt to scare $M!", ch, nullptr, victim, TO_CHAR,
        0);
    act("$N resists $n's attempt to scare $M!", ch, nullptr, victim, TO_ROOM,
        NOTVICT);
    act("You resist $n's attempt to scare you!", ch, nullptr, victim, TO_VICT,
        0);
    if (IS_NPC(
            victim) &&
        (!victim->fighting) && GET_POS(ch) > position_t::SLEEPING)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }

    return eFAILURE;
  }

  if (saves_spell(ch, victim, 0, SAVE_TYPE_COLD) >= 0)
  {
    victim->sendln("For a moment you feel compelled to run away, but you fight back the urge.");
    act("$N doesnt seem to be the yellow-bellied slug you thought!", ch,
        nullptr, victim, TO_CHAR, 0);

    if (IS_NPC(victim) && !victim->fighting)
    {
      mob_suprised_sayings(victim, ch);
      retval = attack(victim, ch, 0);
      SWAP_CH_VICT(retval);
    }
    else
      retval = eFAILURE;

    return retval;
  }

  victim->sendln("You suddenly feel very frightened, and you attempt to flee!");
  do_flee(victim, "", cmd_t::FEAR);

  return eSUCCESS;
}

/* REFRESH */

int spell_refresh(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch or victim sent to spell_refresh!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  dam = dice(skill / 2, 4) + skill / 2;
  dam = MAX(dam, 20);

  if ((dam + GET_MOVE(victim)) > victim->move_limit())
    dam = victim->move_limit() - GET_MOVE(victim);

  victim->incrementMove(dam);

  char buf[MAX_STRING_LENGTH];
  sprintf(buf, "$B%d$R", dam);

  if (ch != victim)
    send_damage("Your magic flows through $N and $E looks less tired by | fatigue points.", ch, 0, victim, buf, "Your magic flows through $N and $E looks less tired.", TO_CHAR);

  send_damage("You feel less tired by | fatigue points.", ch, 0, victim, buf, "You feel less tired.", TO_VICT);
  send_damage("$N feels less tired by | fatigue points.", ch, 0, victim, buf, "$N feels less tired.", TO_ROOM);
  return eSUCCESS;
}

/* FLY */

int spell_fly(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  struct affected_type *cur_af;

  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch or victim sent to spell_fly!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if ((cur_af = victim->affected_by_spell(SPELL_FLY)))
    affect_remove(victim, cur_af, SUPPRESS_ALL);

  if (IS_AFFECTED(victim, AFF_FLYING))
    return eFAILURE;

  victim->sendln("You start flapping and rise off the ground!");
  if (ch != victim)
    act("$N starts flapping and rises off the ground!", ch, nullptr, victim, TO_CHAR, 0);
  act("$N's feet rise off the ground.", ch, 0, victim, TO_ROOM, INVIS_NULL | NOTVICT);

  af.type = SPELL_FLY;
  af.duration = 6 + skill / 2;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_FLYING;
  affect_to_char(victim, &af);
  return eSUCCESS;
}

/* CONTINUAL LIGHT */
// TODO - make this use skill for multiple effects

int spell_cont_light(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  class Object *tmp_obj;

  if (!ch)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch sent to cont_light!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (obj)
  {
    if (isSet(obj->obj_flags.extra_flags, ITEM_GLOW))
    {
      ch->sendln("That item is already glowing with magical light.");
      return eFAILURE;
    }
    if (GET_ITEM_TYPE(obj) != ITEM_ARMOR)
    {
      ch->sendln("Only pieces of equipment may be magically lit in such a way.");
      return eFAILURE;
    }
    if (!CAN_WEAR(obj, EAR) && !CAN_WEAR(obj, SHIELD) && !CAN_WEAR(obj, FINGER))
    {
      ch->sendln("Only earrings, rings, and shields can be magically lit in such a way.");
      return eFAILURE;
    }
    SET_BIT(obj->obj_flags.extra_flags, ITEM_GLOW);
    act("$n twiddles $s thumbs and the $p $e is carrying begins to glow.", ch, obj, 0, TO_ROOM, INVIS_NULL);
    act("You twiddle your thumbs and the $p begins to glow.", ch, obj, 0, TO_CHAR, 0);
    return eSUCCESS;
  }

  tmp_obj = clone_object(real_object(6));

  obj_to_char(tmp_obj, ch);

  act("$n twiddles $s thumbs and $p suddenly appears.", ch, tmp_obj, 0, TO_ROOM, INVIS_NULL);
  act("You twiddle your thumbs and $p suddenly appears.", ch, tmp_obj, 0, TO_CHAR, 0);
  return eSUCCESS;
}

/* ANIMATE DEAD */
// TODO - make skill level have an affect on this

int spell_animate_dead(uint8_t level, Character *ch, Character *victim, class Object *corpse, int skill)
{
  Character *mob;
  class Object *obj_object, *next_obj;
  struct affected_type af;
  int number, r_num;

  if (!IS_EVIL(ch) && ch->getLevel() < ARCHANGEL && GET_CLASS(ch) == CLASS_ANTI_PAL)
  {
    ch->sendln("You aren't evil enough to cast such a repugnant spell.");
    return eFAILURE;
  }

  if (many_charms(ch))
  {
    ch->sendln("How do you plan on controlling so many followers?");
    return eFAILURE;
  }

  // check to see if its an eligible corpse
  if ((GET_ITEM_TYPE(corpse) != ITEM_CONTAINER) || !corpse->obj_flags.value[3] || isexact("pc", corpse->Name()))
  {
    act("$p shudders for a second, then lies still.", ch, corpse, 0,
        TO_CHAR, 0);
    act("$p shudders for a second, then lies still.", ch, corpse, 0,
        TO_ROOM, 0);
    return eFAILURE;
  }

  if (GET_ALIGNMENT(ch) < 0)
  {
    if (level < 20)
      number = 22394;
    else if (level < 30)
      number = 22395;
    else if (level < 40)
      number = 22396;
    else if (level < 50)
      number = 22397;
    else
      number = 22398;
  }
  else
  {
    if (level < 20)
      number = 22389;
    else if (level < 30)
      number = 22390;
    else if (level < 40)
      number = 22391;
    else if (level < 50)
      number = 22392;
    else
      number = 22393;
  }

  if ((r_num = real_mobile(number)) < 0)
  {
    ch->sendln("Mobile: Zombie not found.");
    return eFAILURE;
  }

  mob = ch->getDC()->clone_mobile(r_num);
  char_to_room(mob, ch->in_room);

  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  // take all from corpse, and give to zombie

  for (obj_object = corpse->contains; obj_object; obj_object = next_obj)
  {
    next_obj = obj_object->next_content;
    move_obj(obj_object, mob);
  }

  // set up descriptions and such
  // all in the mob now, no need
  //   sprintf(buf, "%s %s", corpse->name, mob->name);
  //   mob->name = str_hsh(buf);

  //  mob->short_desc = str_hsh(corpse->short_description);

  //  if (GET_ALIGNMENT(ch) < 0)
  //{
  // sprintf(buf, "%s slowly staggers around.\r\n", corpse->short_description);
  //    mob->long_desc = str_hsh(buf);
  //}
  //  else
  // sprintf(buf, "%s hovers above the ground here.\r\n",corpse->short_description);

  if (GET_ALIGNMENT(ch) < 0)
  {
    act("Calling upon your foul magic, you animate $p.\r\n$N slowly lifts "
        "itself to its feet.",
        ch, corpse, mob, TO_CHAR, INVIS_NULL);
    act("Calling upon $s foul magic, $n animates $p.\r\n$N slowly lifts "
        "itself to its feet.",
        ch, corpse, mob, TO_ROOM, INVIS_NULL);
  }
  else
  {
    act("Invoking your divine magic, you free $p's spirit.\r\n$N slowly rises "
        "out of the corpse and hovers a few feet above the ground.",
        ch, corpse, mob, TO_CHAR, INVIS_NULL);
    act("Invoking $s divine magic, $n releases $p's spirit.\r\n$N slowly rises "
        "out of the corpse and hovers a few feet above the ground.",
        ch, corpse, mob, TO_ROOM, INVIS_NULL);
  }

  // zombie should be charmed and follower ch
  // TODO duration needs skill affects too

  af.type = SPELL_CHARM_PERSON;
  af.duration = 5 + level / 2;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_CHARM;
  affect_to_char(mob, &af);
  if (isSet(mob->immune, ISR_PIERCE))
    REMOVE_BIT(mob->immune, ISR_PIERCE);
  add_follower(mob, ch);

  extract_obj(corpse);

  return eSUCCESS;
}

/* KNOW ALIGNMENT */

int spell_know_alignment(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int duration = 0;
  struct affected_type af, *cur_af;

  if (!ch)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch sent to know_alignment!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if ((cur_af = victim->affected_by_spell(SPELL_KNOW_ALIGNMENT)))
    affect_remove(victim, cur_af, SUPPRESS_ALL);

  if (skill <= 40)
    duration = level / 5;
  else if (skill > 40 && skill <= 60)
    duration = level / 4;
  else if (skill > 60 && skill <= 80)
    duration = level / 3;
  else if (skill > 80)
    duration = level / 2;

  if (skill == 150)
    duration = 2;
  ch->sendln("Your eyes tingle, allowing you to see the auras of other creatures.");
  act("$n's eyes flash with knowledge and insight!", ch, 0, 0, TO_ROOM, INVIS_NULL);

  af.type = SPELL_KNOW_ALIGNMENT;
  af.duration = duration;
  af.modifier = skill;
  af.location = APPLY_NONE;
  af.bitvector = AFF_KNOW_ALIGN;
  affect_to_char(ch, &af);

  return eSUCCESS;
}

/* DISPEL MINOR */

int spell_dispel_minor(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int rots = 0;
  int done = false;
  int retval;

  if (obj && (uint64_t)obj > 100) /* Trying to dispel_minor an obj */
  {                               // Heh, it passes spell cast through obj now too. Less than 100 = not
                                  // an actual obj.
    if (GET_ITEM_TYPE(obj) != ITEM_BEACON)
    {
      if (!obj->equipped_by && !obj->carried_by)
      {
        ch->sendln("You can't dispel that!");
        return eFAILURE;
      }
      if (isSet(obj->obj_flags.extra_flags, ITEM_INVISIBLE))
      {
        REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
        ch->sendln("You remove the item's invisibility.");
        return eSUCCESS;
      }
      else if (isSet(obj->obj_flags.extra_flags, ITEM_GLOW))
      {
        REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_GLOW);
        ch->sendln("You remove the item's $Bglowing$R aura.");
        return eSUCCESS;
      }
      else
      {
        ch->sendln("That item is not imbued with dispellable magic.");
        return eFAILURE;
      }
    }
    if (!obj->equipped_by)
    {
      // Someone load it or something?
      ch->sendln("The magic fades away back to the ether.");
      act("$p fades away gently.", ch, obj, 0, TO_ROOM, INVIS_NULL);
    }
    else
    {
      ch->sendln("The magic is shattered by your will!");
      act("$p blinks out of existence with a bang!", ch, obj, 0, TO_ROOM, INVIS_NULL);
      obj->equipped_by->sendln("Your magic beacon is shattered!");
      obj->equipped_by->beacon = nullptr;
      obj->equipped_by = nullptr;
    }
    extract_obj(obj);
    return eSUCCESS;
  }
  int spell = (int64_t)obj;
  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null ch or victim sent to dispel_minor!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (IS_PC(ch) && IS_PC(victim) && victim->fighting &&
      IS_NPC(victim->fighting))
  {
    ch->sendln("You misfire!");
    victim = ch;
  }

  int savebonus = 0;
  if (skill < 41)
    savebonus = -5;
  else if (skill < 61)
    savebonus = -10;
  else if (skill < 81)
    savebonus = -15;
  else
    savebonus = -20;

  if (spell && savebonus != -20 && IS_PC(ch))
  {
    ch->sendln("You do not know this spell well enough to target it.");
    return eFAILURE;
  }
  if (spell)
    savebonus = -5;

  set_cantquit(ch, victim);

  if (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_NODISPEL))
  {
    act("$N seems to ignore $n's spell!", ch, 0, victim, TO_ROOM, 0);
    act("$N seems to ignore your spell!", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  // If victim higher level, they get a save vs magic for no effect
  if (number(1, 100) < get_saves(victim, SAVE_TYPE_MAGIC) + savebonus)
  {
    act("$N resists your attempt to dispel minor!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to dispel minor!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to dispel minor!", ch, nullptr, victim, TO_VICT, 0);
    if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > position_t::SLEEPING)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }

    return eFAILURE;
  }

  // Input max number of spells in switch statement here
  while (!done && ((rots += 1) < 10))
  {
    int x = spell != 0 ? spell : number(1, 15);
    switch (x)
    {
    case 1:
      if (victim->affected_by_spell(SPELL_INVISIBLE))
      {
        affect_from_char(victim, SPELL_INVISIBLE);
        victim->sendln("You feel your invisibility dissipate.");
        act("$n fades into existence.", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      if (IS_AFFECTED(victim, AFF_INVISIBLE))
      {
        REMBIT(victim->affected_by, AFF_INVISIBLE);
        victim->sendln("You feel your invisibility dissipate.");
        act("$n fades into existence.", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;

    case 2:
      if (victim->affected_by_spell(SPELL_DETECT_INVISIBLE))
      {
        affect_from_char(victim, SPELL_DETECT_INVISIBLE);
        victim->sendln("Your ability to detect invisible has been dispelled!");
        act("$N's ability to detect invisible is removed.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;

    case 3:
      if (victim->affected_by_spell(SPELL_CAMOUFLAGE))
      {
        affect_from_char(victim, SPELL_CAMOUFLAGE);
        victim->sendln("Your camouflage has been dispelled!.");
        act("$N's form is less obscured as you dispel $S camouflage.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;

    case 4:
      if (victim->affected_by_spell(SPELL_RESIST_ACID))
      {
        affect_from_char(victim, SPELL_RESIST_ACID);
        victim->sendln("The $2green$R in your skin is dispelled!");
        act("$N's skin loses its $2green$R hue.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;

    case 5:
      if (victim->affected_by_spell(SPELL_RESIST_COLD))
      {
        affect_from_char(victim, SPELL_RESIST_COLD);
        victim->sendln("The $3blue$R in your skin is dispelled!");
        act("$N's skin loses its $3blue$R hue.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;

    case 6:
      if (victim->affected_by_spell(SPELL_RESIST_FIRE))
      {
        affect_from_char(victim, SPELL_RESIST_FIRE);
        victim->sendln("The $4red$R in your skin is dispelled!");
        act("$N's skin loses its $4red$R hue.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;

    case 7:
      if (victim->affected_by_spell(SPELL_RESIST_ENERGY))
      {
        affect_from_char(victim, SPELL_RESIST_ENERGY);
        victim->sendln("The $5yellow$R in your skin is dispelled!");
        act("$N's skin loses its $5yellow$R hue.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;

    case 8:
      if (victim->affected_by_spell(SPELL_BARKSKIN))
      {
        affect_from_char(victim, SPELL_BARKSKIN);
        victim->sendln("Your woody has been dispelled!");
        act("$N loses $S woody.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;

    case 9:
      if (victim->affected_by_spell(SPELL_STONE_SKIN))
      {
        affect_from_char(victim, SPELL_STONE_SKIN);
        victim->sendln("Your skin loses stone-like consistency.");
        act("$N's looks like less of a stoner as $S skin returns to normal.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;

    case 10:
      if (victim->affected_by_spell(SPELL_FLY))
      {
        affect_from_char(victim, SPELL_FLY);
        victim->sendln("You do not feel lighter than air anymore.");
        act("$N is no longer lighter than air.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      if (IS_AFFECTED(victim, AFF_FLYING))
      {
        REMBIT(victim->affected_by, AFF_FLYING);
        victim->sendln("You do not feel lighter than air anymore.");
        act("$n drops to the ground, no longer lighter than air.", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;

    case 11:
      if (victim->affected_by_spell(SPELL_true_SIGHT))
      {
        affect_from_char(victim, SPELL_true_SIGHT);
        victim->sendln("You no longer see what is hidden.");
        act("$N no longer sees what is hidden.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;

    case 12:
      if (victim->affected_by_spell(SPELL_WATER_BREATHING))
      {
        affect_from_char(victim, SPELL_WATER_BREATHING);
        victim->sendln("You can no longer breathe underwater!");
        act("$N can no longer breathe underwater!", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;
    case 13:
      if (victim->affected_by_spell(SPELL_ARMOR))
      {
        affect_from_char(victim, SPELL_ARMOR);
        done = true;
        victim->sendln("Your magical armour is dispelled!");
        act("$N's magical armour is dispelled!", ch, 0, victim, TO_CHAR, 0);
      }
      break;
    case 14:
      if (victim->affected_by_spell(SPELL_SHIELD))
      {
        affect_from_char(victim, SPELL_SHIELD);
        done = true;
        victim->sendln("Your force shield shimmers and fades away.");
        act("$N's force shield shimmers and fades away.", ch, 0, victim, TO_CHAR, 0);
      }
      break;

    case 15:
      if (victim->affected_by_spell(SPELL_RESIST_MAGIC))
      {
        affect_from_char(victim, SPELL_RESIST_MAGIC);
        victim->sendln("The $B$7white$R in your skin is dispelled!");
        act("$N's skin loses its $B$7white$R hue.", ch, 0, victim, TO_CHAR, 0);
        done = true;
      }
      break;

    default:
      ch->sendln("Illegal Value send to switch in dispel_minor, tell a god.");
      done = true;
      break;
    } // of switch
  } // of while

  if (IS_NPC(victim) && !victim->fighting)
  {
    mob_suprised_sayings(victim, ch);
    retval = attack(victim, ch, 0);
    SWAP_CH_VICT(retval);
    return retval;
  }
  return eSUCCESS;
}

/* DISPEL MAGIC */

int spell_dispel_magic(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill, int spell)
{
  int rots = 0;
  int done = false;
  int retval;

  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null ch or victim sent to dispel_magic!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  set_cantquit(ch, victim);

  if (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_NODISPEL))
  {
    act("$N seems to ignore $n's spell!", ch, 0, victim, TO_ROOM, 0);
    act("$N seems to ignore your spell!", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if (ISSET(victim->affected_by, AFF_GOLEM))
  {
    ch->sendln("The golem seems to shrug off your dispel attempt!");
    act("The golem seems to ignore $n's dispelling magic!", ch, 0, 0, TO_ROOM, 0);
    return eFAILURE;
  }
  /*
  if(IS_PC(ch) && IS_PC(victim) && victim->fighting &&
      IS_NPC(victim->fighting) &&
     !IS_AFFECTED(victim->fighting, AFF_CHARM))
  {
     ch->sendln("Your dispelling magic misfires!");
     victim = ch;
  }*/
  int savebonus = 0;
  if (IS_NPC(victim))
  {
    if (skill < 41)
      savebonus = 15;
    else if (skill < 61)
      savebonus = 10;
    else if (skill < 81)
      savebonus = 5;
  }
  else
  {
    if (skill < 41)
      savebonus = 5;
    else if (skill < 61)
      savebonus = 0;
    else if (skill < 81)
      savebonus = -5;
    else
      savebonus = -10;
  }

  if (spell < 11 && spell > 0 && skill < 81) // spell-targetted cast
  {
    ch->sendln("You do not yet know this spell well enough to target it.");
    return eFAILURE;
  }

  if (spell < 11 && spell > 0)
    savebonus = 0;

  // If victim higher level, they get a save vs magic for no effect
  //      if((victim->getLevel() > ch->getLevel()) && 0 > saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC))
  //          return eFAILURE;
  if (number(1, 100) < get_saves(victim, SAVE_TYPE_MAGIC) + savebonus && level != ch->getLevel() - 1)
  {
    act("$N resists your attempt to dispel magic!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to dispel magic!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to dispel magic!", ch, nullptr, victim, TO_VICT, 0);
    if (IS_NPC(victim) && (!victim->fighting) && GET_POS(ch) > position_t::SLEEPING)
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }

    return eFAILURE;
  }

  // Number of spells in the switch statement goes here
  while (!done && ((rots += 1) < 15))
  {
    int x;
    if (spell < 1 || spell > 10)
      x = number(1, 10); // weapon spell or non-spell-targetted cast, probably ;)
    else
      x = spell;
    switch (x)
    {
    case 1:
      if (victim->affected_by_spell(SPELL_SANCTUARY))
      {
        affect_from_char(victim, SPELL_SANCTUARY);
        act("You don't feel so invulnerable anymore.", ch, 0, victim, TO_VICT, 0);
        act("The $B$7white glow$R around $n's body fades.", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      if (IS_AFFECTED(victim, AFF_SANCTUARY))
      {
        REMBIT(victim->affected_by, AFF_SANCTUARY);
        act("You don't feel so invulnerable anymore.", ch, 0, victim, TO_VICT, 0);
        act("The $B$7white glow$R around $n's body fades.", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;

    case 2:
      if (victim->affected_by_spell(SPELL_PROTECT_FROM_EVIL))
      {
        affect_from_char(victim, SPELL_PROTECT_FROM_EVIL);
        act("Your protection from evil has been dispelled!", ch, 0, victim, TO_VICT, 0);
        act("The dark, $6pulsing$R aura surrounding $n has been dispelled!", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;

    case 3:
      if (victim->affected_by_spell(SPELL_HASTE))
      {
        affect_from_char(victim, SPELL_HASTE);
        act("Your magically enhanced speed has been dispelled!", ch, 0, victim, TO_VICT, 0);
        act("$n's actions slow to their normal speed.", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;

    case 4:
      if (victim->affected_by_spell(SPELL_STONE_SHIELD))
      {
        affect_from_char(victim, SPELL_STONE_SHIELD);
        act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0, victim, TO_VICT, 0);
        act("The shield of stones swirling about $n's body fall to the ground!", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;

    case 5:
      if (victim->affected_by_spell(SPELL_GREATER_STONE_SHIELD))
      {
        affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);
        act("Your shield of swirling stones falls harmlessly to the ground!", ch, 0, victim, TO_VICT, 0);
        act("The shield of stones swirling about $n's body falls to the ground!", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;

    case 6:
      if (IS_AFFECTED(victim, AFF_FROSTSHIELD))
      {
        REMBIT(victim->affected_by, AFF_FROSTSHIELD);
        act("Your shield of $B$3frost$R melts into nothing!.", ch, 0, victim, TO_VICT, 0);
        act("The $B$3frost$R encompassing $n's body melts away.", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;

    case 7:
      if (victim->affected_by_spell(SPELL_LIGHTNING_SHIELD))
      {
        affect_from_char(victim, SPELL_LIGHTNING_SHIELD);
        act("Your crackling shield of $B$5electricity$R vanishes!", ch, 0, victim, TO_VICT, 0);
        act("The $B$5electricity$R crackling around $n's body fades away.", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      else if (IS_AFFECTED(victim, AFF_LIGHTNINGSHIELD))
      {
        REMBIT(victim->affected_by, AFF_LIGHTNINGSHIELD);
        act("Your crackling shield of $B$5electricity$R vanishes!", ch, 0, victim, TO_VICT, 0);
        act("The $B$5electricity$R crackling around $n's body fades away.", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;
    case 8:
      if (victim->affected_by_spell(SPELL_FIRESHIELD))
      {
        affect_from_char(victim, SPELL_FIRESHIELD);
        act("Your $B$4flames$R have been extinguished!", ch, 0, victim, TO_VICT, 0);
        act("The $B$4flames$R encompassing $n's body are extinguished!", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      if (IS_AFFECTED(victim, AFF_FIRESHIELD))
      {
        REMBIT(victim->affected_by, AFF_FIRESHIELD);
        act("Your $B$4flames$R have been extinguished!", ch, 0, victim, TO_VICT, 0);
        act("The $B$4flames$R encompassing $n's body are extinguished!", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;
    case 9:
      if (victim->affected_by_spell(SPELL_ACID_SHIELD))
      {
        affect_from_char(victim, SPELL_ACID_SHIELD);
        act("Your shield of $B$2acid$R dissolves to nothing!", ch, 0, victim, TO_VICT, 0);
        act("The $B$2acid$R swirling about $n's body dissolves to nothing!", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      if (IS_AFFECTED(victim, AFF_ACID_SHIELD))
      {
        REMBIT(victim->affected_by, AFF_ACID_SHIELD);
        act("Your shield of $B$2acid$R dissolves to nothing!", ch, 0, victim, TO_VICT, 0);
        act("The $B$2acid$R swirling about $n's body dissolves to nothing!", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;

    case 10:
      if (victim->affected_by_spell(SPELL_PROTECT_FROM_GOOD))
      {
        affect_from_char(victim, SPELL_PROTECT_FROM_GOOD);
        act("Your protection from good has been dispelled!", ch, 0, victim, TO_VICT, 0);
        act("The light, $B$6pulsing$R aura surrounding $n has been dispelled!", victim, 0, 0, TO_ROOM, 0);
        done = true;
      }
      break;

    default:
      ch->send("Illegal Value sent to dispel_magic switch statement.  Tell a god.");
      done = true;
      break;
    } // end of switch
  } // end of while

  if (IS_NPC(victim) && !victim->fighting)
  {
    mob_suprised_sayings(victim, ch);
    retval = attack(victim, ch, 0);
    SWAP_CH_VICT(retval);
    return retval;
  }
  return eSUCCESS;
}

/* CURE SERIOUS */

int spell_cure_serious(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int healpoints;
  char buf[MAX_STRING_LENGTH * 2], dammsg[MAX_STRING_LENGTH];

  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null ch or victim sent to cure_serious!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (GET_RACE(victim) == RACE_UNDEAD)
  {
    ch->sendln("Healing spells are useless on the undead.");
    return eFAILURE;
  }
  if (GET_RACE(victim) == RACE_GOLEM)
  {
    ch->sendln("The heavy magics surrounding this being prevent healing.");
    return eFAILURE;
  }

  if (!can_heal(ch, victim, SPELL_CURE_SERIOUS))
    return eFAILURE;
  healpoints = dam_percent(skill, 50 + getRealSpellDamage(ch));
  healpoints = number(healpoints - (healpoints / 10), healpoints + (healpoints / 10));

  if ((healpoints + victim->getHP()) > hit_limit(victim))
  {
    healpoints = hit_limit(victim) - victim->getHP();
    victim->fillHPLimit();
  }
  else
  {
    victim->addHP(healpoints);
  }

  update_pos(victim);

  sprintf(dammsg, "$B%d$R damage", healpoints);

  if (ch != victim)
  {
    sprintf(buf, "You heal %s of the more serious wounds on $N.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "several" : "several");
    act(buf, ch, 0, victim, TO_CHAR, 0);
    sprintf(buf, "$n heals %s of your more serious wounds.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "several" : "several");
    act(buf, ch, 0, victim, TO_VICT, 0);
    sprintf(buf, "$n heals | of the more serious wounds on $N.");
    send_damage(buf, ch, 0, victim, dammsg, "$n heals several of the more serious wounds on $N.", TO_ROOM);
  }
  else
  {
    sprintf(buf, "You heal %s of your more serious wounds.", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "several" : "several");
    act(buf, ch, 0, 0, TO_CHAR, 0);
    sprintf(buf, "$n heals | of $s more serious wounds.");
    send_damage(buf, ch, 0, victim, dammsg, "$n heals several of $s more serious wounds.", TO_ROOM);
  }

  return eSUCCESS;
}

/* CAUSE LIGHT */

int spell_cause_light(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;

  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("Null ch or victim sent to cause_light"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  set_cantquit(ch, victim);
  dam = dice(1, 9) + (skill / 3);
  return damage(ch, victim, dam, TYPE_MAGIC, SPELL_CAUSE_LIGHT, weap_spell);
}

/* CAUSE CRITICAL */

int spell_cause_critical(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;

  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch or victim sent to cause_critical!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  set_cantquit(ch, victim);

  dam = dice(3, 9) + skill / 1.5;

  return damage(ch, victim, dam, TYPE_MAGIC, SPELL_CAUSE_CRITICAL, weap_spell);
}

/* CAUSE SERIOUS */

int spell_cause_serious(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch or victim sent to cause_serious!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  set_cantquit(ch, victim);

  dam = dice(2, 9) + (skill / 1.75);

  return damage(ch, victim, dam, TYPE_MAGIC, SPELL_CAUSE_SERIOUS);
}

/* FLAMESTRIKE */

int spell_flamestrike(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam, retval;
  int weap_spell = obj ? WEAR_WIELD : 0;

  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch or victim sent to flamestrike!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  set_cantquit(ch, victim);
  dam = 400;

  retval = damage(ch, victim, dam, TYPE_FIRE, SPELL_FLAMESTRIKE, weap_spell);

  if (SOMEONE_DIED(retval))
    return retval;
  if (isSet(retval, eEXTRA_VAL2))
    victim = ch;
  if (isSet(retval, eEXTRA_VALUE))
    return retval;
  // Burns up ki and mana on victim if learned over skill level 70

  if (skill > 70)
  {
    char buf[MAX_STRING_LENGTH];
    int mamount = GET_MANA(victim) / 20, kamount = GET_KI(victim) / 10;
    if (GET_MANA(victim) - mamount < 0)
      mamount = GET_MANA(victim);
    if (GET_KI(victim) - kamount < 0)
      kamount = GET_KI(victim);
    GET_MANA(victim) -= mamount;
    GET_KI(victim) -= kamount;
    sprintf(buf, "$B%d$R mana and $B%d$R ki", mamount, kamount);
    send_damage("$n's fires $4burn$R into your soul tearing away at your being for |.", ch, 0, victim, buf, "The fires $4burn$R into your soul tearing away at your being.", TO_VICT);
    send_damage("Your fires $4burn$R into $N's soul, tearing away at $S being for |.", ch, 0, victim, buf, "Your fires $4burn$R $N's soul, tearing away at $S being.", TO_CHAR);
  }
  return retval;
}

/* IRIDESCENT AURA */
int spell_iridescent_aura(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (victim->affected_by_spell(SPELL_IRIDESCENT_AURA))
  {
    act("$N is already protected by iridescent aura.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("The air around $n shimmers and an $B$3i$7r$3i$7d$3e$7s$3c$7e$3n$7t$R aura appears around $m.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("The air around you shimmers and an $B$3i$7r$3i$7d$3e$7s$3c$7e$3n$7t$R aura appears around you.", victim, 0, 0, TO_CHAR, 0);

  af.type = SPELL_IRIDESCENT_AURA;
  af.duration = 1 + skill / 10;
  af.modifier = skill / 15;
  af.bitvector = -1;
  af.location = APPLY_SAVES;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* RESIST COLD */

int spell_resist_cold(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  if (GET_CLASS(ch) == CLASS_PALADIN && ch != victim)
  {
    ch->sendln("You can only cast this on yourself.");
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_RESIST_COLD))
  {
    act("$N is already resistant to cold.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("$n's skin turns $3blue$R momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("Your skin turns $3blue$R momentarily.", victim, 0, 0, TO_CHAR, 0);

  af.type = SPELL_RESIST_COLD;
  af.duration = 1 + skill / 10;
  af.modifier = 10 + skill / 6;
  af.location = APPLY_SAVING_COLD;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* RESIST FIRE */

int spell_resist_fire(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (GET_CLASS(ch) == CLASS_MAGIC_USER && ch != victim)
  {
    ch->sendln("You can only cast this on yourself.");
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_RESIST_FIRE))
  {
    act("$N is already resistant to fire.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("$n's skin turns $4red$R momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("Your skin turns $4red$R momentarily.", victim, 0, 0, TO_CHAR, 0);
  af.type = SPELL_RESIST_FIRE;
  af.duration = 1 + skill / 10;
  af.modifier = 10 + skill / 6;
  af.location = APPLY_SAVING_FIRE;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* RESIST MAGIC */

int spell_resist_magic(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (GET_CLASS(ch) == CLASS_MAGIC_USER && ch != victim)
  {
    ch->sendln("You can only cast this on yourself.");
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_RESIST_MAGIC))
  {
    act("$N is already resistant to magic.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("$n's skin turns $B$7white$R momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("Your skin turns $B$7white$R momentarily.", victim, 0, 0, TO_CHAR, 0);
  af.type = SPELL_RESIST_MAGIC;
  af.duration = 1 + skill / 10;
  af.modifier = 10 + skill / 6;
  af.location = APPLY_SAVING_MAGIC;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* STAUNCHBLOOD */

int spell_staunchblood(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (GET_CLASS(ch) == CLASS_RANGER && ch != victim)
  {
    ch->sendln("You can only cast this on yourself.");
    return eFAILURE;
  }
  if (victim->affected_by_spell(SPELL_STAUNCHBLOOD))
  {
    act("$N is already resistant to poison.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("You feel supremely healthy and resistant to $2poison$R!", victim, 0, 0, TO_CHAR, 0);
  act("$n looks supremely healthy and begins looking for snakes and spiders to fight.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  af.type = SPELL_STAUNCHBLOOD;
  af.duration = 1 + skill / 10;
  af.modifier = 10 + skill / 6;
  af.location = APPLY_SAVING_POISON;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* RESIST ENERGY */

int spell_resist_energy(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (GET_CLASS(ch) != CLASS_DRUID && ch != victim)
  {
    ch->sendln("You can only cast this on yourself.");
    return eFAILURE;
  }
  if (victim->affected_by_spell(SPELL_RESIST_ENERGY))
  {
    act("$N is already resistant to energy.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("$n's skin turns $5yellow$R momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("Your skin turns $5yellow$R momentarily.", victim, 0, 0, TO_CHAR, 0);
  af.type = SPELL_RESIST_ENERGY;
  af.duration = 1 + skill / 10;
  af.modifier = 10 + skill / 6;
  af.location = APPLY_SAVING_ENERGY;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* STONE SKIN */

int spell_stone_skin(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!ch)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch sent to cause_serious!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (ch->affected_by_spell(SPELL_STONE_SKIN))
  {
    act("Your skin already rock hard.", ch, 0, 0, TO_CHAR, 0);
    return eFAILURE;
  }
  act("$n's skin turns grey and stone-like.", ch, 0, 0, TO_ROOM, INVIS_NULL);
  act("Your skin turns to a stone-like substance.", ch, 0, 0, TO_CHAR, 0);

  af.type = SPELL_STONE_SKIN;
  af.duration = 3 + skill / 6;
  af.modifier = -(10 + skill / 4);
  af.location = APPLY_AC;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  SET_BIT(ch->resist, ISR_PIERCE);

  return eSUCCESS;
}

/* SHIELD */

int spell_shield(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch or victim sent to shield!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_SHIELD))
    affect_from_char(victim, SPELL_SHIELD);

  act("$N is surrounded by a strong force shield.", ch, 0, victim, TO_ROOM, INVIS_NULL | NOTVICT);
  if (ch != victim)
  {
    act("$N is surrounded by a strong force shield.", ch, 0, victim, TO_CHAR, 0);
    act("You are surrounded by a strong force shield.", ch, 0, victim, TO_VICT, 0);
  }
  else
  {
    act("You are surrounded by a strong force shield.", ch, 0, victim, TO_VICT, 0);
  }

  af.type = SPELL_SHIELD;
  af.duration = 6 + skill / 3;
  af.modifier = -(10 + skill / 10);
  af.location = APPLY_AC;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* WEAKEN */

int spell_weaken(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  struct affected_type *cur_af;
  int retval;
  int duration = 0, str = 0, con = 0;
  void check_weapon_weights(Character * ch);

  if (!ch || !victim)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch or victim sent to weaken!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  set_cantquit(ch, victim);

  if (victim->affected_by_spell(SPELL_PARALYZE))
  {
    act("$N's paralyzed muscles are too rigid to be affected by this enchantment!", ch, 0, victim, TO_CHAR, 0);
    act("Your gods protect you from $n's spell.", ch, 0, victim, TO_VICT, 0);
    return eSUCCESS;
  }

  if (victim->affected_by_spell(SPELL_HEROISM) && victim->affected_by_spell(SPELL_HEROISM)->modifier >= 50)
  {
    act("$N seems unaffected.", ch, 0, victim, TO_CHAR, 0);
    act("Your gods protect you from $n's spell.", ch, 0, victim, TO_VICT, 0);
    return eSUCCESS;
  }

  if (skill < 40)
  {
    duration = 2;
    str = -4;
    con = -1;
  }
  else if (skill > 40 && skill <= 60)
  {
    duration = 3;
    str = -8;
    con = -2;
  }
  else if (skill > 60 && skill <= 80)
  {
    duration = 4;
    str = -10;
    con = -3;
  }
  else if (skill > 80)
  {
    duration = 5;
    str = -12;
    con = -4;
  }

  if (malediction_res(ch, victim, SPELL_WEAKEN))
  {
    act("$N resists your attempt to weaken $M!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to weaken $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to weaken you!", ch, nullptr, victim, TO_VICT, 0);
  }
  else
  {
    if (!victim->affected_by_spell(SPELL_WEAKEN))
    {
      act("You feel weaker.", ch, 0, victim, TO_VICT, 0);
      act("$n seems weaker.", victim, 0, 0, TO_ROOM, INVIS_NULL);

      if ((cur_af = victim->affected_by_spell(SPELL_STRENGTH)))
      {
        cur_af->modifier += str;
        af.type = cur_af->type;
        af.duration = cur_af->duration;
        af.modifier = cur_af->modifier;
        af.location = cur_af->location;
        af.bitvector = cur_af->bitvector;
        affect_from_char(victim, SPELL_STRENGTH); // this makes cur_af invalid
        if (af.modifier > 0)                      // it's not out yet
          affect_to_char(victim, &af);
        return eSUCCESS;
      }

      af.type = SPELL_WEAKEN;
      af.duration = duration;
      af.modifier = str;
      af.location = APPLY_STR;
      af.bitvector = -1;
      // Modify the affect's strength modifier if it would cause the victim to go negative
      int possible_str = GET_STR_BONUS(victim) + af.modifier + GET_RAW_STR(victim);
      if (possible_str < 0)
      {
        af.modifier -= possible_str;
      }
      affect_to_char(victim, &af);

      af.modifier = con;
      af.location = APPLY_CON;
      // Modify the affect's constitution modifier if it would cause the victim to go negative
      int possible_con = GET_CON_BONUS(victim) + af.modifier + GET_RAW_CON(victim);
      if (possible_con < 0)
      {
        af.modifier -= possible_con;
      }
      affect_to_char(victim, &af);

      check_weapon_weights(victim);
    }
  }

  if (IS_NPC(victim))
  {

    if (!victim->fighting)
    {
      mob_suprised_sayings(victim, ch);
      retval = attack(victim, ch, 0);
      return retval;
    }
  }
  return eSUCCESS;
}

/* MASS INVISIBILITY */

// TODO - make this use skill
int spell_mass_invis(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  Character *tmp_victim;
  struct affected_type af;

  if (!ch)
  {
    DC::getInstance()->logentry(QStringLiteral("nullptr ch sent to mass_invis!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }

  for (tmp_victim = DC::getInstance()->world[ch->in_room].people; tmp_victim;
       tmp_victim = tmp_victim->next_in_room)
  {
    if ((ch->in_room == tmp_victim->in_room))
      if (!tmp_victim->affected_by_spell(SPELL_INVISIBLE))
      {

        act("$n slowly fades out of existence.", tmp_victim, 0, 0,
            TO_ROOM, INVIS_NULL);
        tmp_victim->sendln("You vanish.");

        af.type = SPELL_INVISIBLE;
        af.duration = 24;
        af.modifier = -10 - skill / 6;
        af.location = APPLY_AC;
        af.bitvector = AFF_INVISIBLE;
        affect_to_char(tmp_victim, &af);
      }
  }
  return eSUCCESS;
}

/* ACID BLAST */

int spell_acid_blast(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int weap_spell = obj ? WEAR_WIELD : 0;
  set_cantquit(ch, victim);
  dam = 375;
  return damage(ch, victim, dam, TYPE_ACID, SPELL_ACID_BLAST, weap_spell);
}

/* HELLSTREAM */

int spell_hellstream(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;

  set_cantquit(ch, victim);
  dam = 950;
  if (level > 200)
    return damage(ch, victim, dam, TYPE_HIT + level - 200, SPELL_HELLSTREAM);
  else
    return damage(ch, victim, dam, TYPE_FIRE, SPELL_HELLSTREAM);
}

/* PORTAL (Creates the portal "item") */

void make_portal(Character *ch, Character *vict)
{
  class Object *ch_portal, *vict_portal;

  char buf[250];
  qint64 chance{};
  room_t destination{};
  bool good_destination = false;

  ch_portal = new Object;
  vict_portal = new Object;

  ch_portal->item_number = -1;
  vict_portal->item_number = -1;
  ch_portal->in_room = DC::NOWHERE;
  vict_portal->in_room = DC::NOWHERE;

  if (GET_CLASS(ch) == CLASS_CLERIC)
    sprintf(buf, "pcportal portal cleric %s", GET_NAME(ch));
  else
    sprintf(buf, "pcportal portal only %s %s", GET_NAME(ch), GET_NAME(vict));

  ch_portal->Name(buf);
  vict_portal->Name(buf);

  ch_portal->short_description = str_hsh("an extradimensional portal");
  vict_portal->short_description = str_hsh("an extradimensional portal");

  ch_portal->long_description = str_hsh("An extradimensional portal shimmers in "
                                        "the air before you.");
  vict_portal->long_description = str_hsh("An extradimensional portal shimmers in "
                                          "the air before you.");

  ch_portal->obj_flags.type_flag = ITEM_PORTAL;
  vict_portal->obj_flags.type_flag = ITEM_PORTAL;

  ch_portal->obj_flags.timer = 2;
  vict_portal->obj_flags.timer = 2;

  chance = ((vict->getLevel() * 2) - ch->getLevel());

  if (vict->getLevel() > ch->getLevel() && chance > number(0, 100))
  {
    while (!good_destination)
    {
      destination = number<room_t>(1, DC::getInstance()->top_of_world);
      if (!DC::getInstance()->rooms.contains(destination) ||
          isSet(DC::getInstance()->world[destination].room_flags, ARENA) ||
          isSet(DC::getInstance()->world[destination].room_flags, IMP_ONLY) ||
          isSet(DC::getInstance()->world[destination].room_flags, PRIVATE) ||
          isSet(DC::getInstance()->world[destination].room_flags, CLAN_ROOM) ||
          isSet(DC::getInstance()->world[destination].room_flags, NO_PORTAL) ||
          isSet(DC::getInstance()->world[destination].room_flags, NO_TELEPORT) ||
          DC::getInstance()->zones.value(DC::getInstance()->world[destination].zone).isNoTeleport())
      {
        good_destination = false;
      }
      else
      {
        good_destination = true;
      }
    }
    ch_portal->setPortalDestinationRoom(destination);
  }
  else
  {
    destination = vict->in_room;
    ch_portal->setPortalDestinationRoom(destination);
  }

  vict_portal->setPortalDestinationRoom(ch->in_room);

  ch_portal->next = vict_portal;
  vict_portal->next = DC::getInstance()->object_list;
  DC::getInstance()->object_list = ch_portal;

  obj_to_room(ch_portal, ch->in_room);
  obj_to_room(vict_portal, destination);

  send_to_room("There is a violent flash of light as a portal shimmers "
               "into existence.\r\n",
               ch->in_room);

  sprintf(buf, "The space in front of you warps itself to %s's iron will.\r\n",
          GET_SHORT(ch));
  send_to_room(buf, vict->in_room);
  send_to_room("There is a violent flash of light as a portal shimmers "
               "into existence.\r\n",
               vict->in_room);

  return;
}

/* PORTAL (Actual spell) */
// TODO - make this use skill

int spell_portal(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  if (isSet(DC::getInstance()->world[victim->in_room].room_flags, PRIVATE) ||
      isSet(DC::getInstance()->world[victim->in_room].room_flags, IMP_ONLY) ||
      isSet(DC::getInstance()->world[victim->in_room].room_flags, NO_PORTAL))
  {
    ch->sendln("You can't seem to find a path.");
    return eFAILURE;
  }

  if ((IS_PC(victim)) && (victim->getLevel() >= IMMORTAL))
  {
    ch->sendln("Just who do you think you are?");
    return eFAILURE;
  }
  if (IS_AFFECTED(victim, AFF_SHADOWSLIP))
  {
    ch->sendln("You can't seem to find a definite path.");
    return eFAILURE;
  }
  if (DC::getInstance()->zones.value(DC::getInstance()->world[victim->in_room].zone).isNoTeleport())
  {
    ch->sendln("A portal shimmers into view but is unstable and immediately fades to nothing.");
    act("A portal shimmers into view but is unstable and immediately fades to nothing.", ch, 0, 0, TO_ROOM, 0);
    return eFAILURE;
  }

  if (DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).continent != DC::getInstance()->zones.value(DC::getInstance()->world[victim->in_room].zone).continent)
  {
    if (GET_MANA(ch) < use_mana(ch, skill))
    {
      ch->sendln("You don't posses the energy to portal that far.");
      GET_MANA(ch) += use_mana(ch, skill);
      return eFAILURE;
    }
    else
    {
      ch->sendln("The long distance drains additional mana from you.");
      GET_MANA(ch) -= use_mana(ch, skill);
    }
  }

  bool portal_found = false;
  for (auto portal = DC::getInstance()->world[ch->in_room].contents; portal; portal = portal->next_content)
  {
    if (portal->isPortal())
    {
      portal_found = true;
      break;
    }
  }

  if (!portal_found)
  {
    for (auto portal = DC::getInstance()->world[victim->in_room].contents; portal; portal = portal->next_content)
    {
      if (portal->isPortal())
      {
        portal_found = true;
        break;
      }
    }
  }

  Character *tmpch;

  bool found_hunt_or_quest_item = false;
  for (tmpch = DC::getInstance()->world[victim->in_room].people; tmpch; tmpch = tmpch->next_in_room)
  {
    if (search_char_for_item(tmpch, real_object(76), false) || search_char_for_item(tmpch, real_object(51), false))
    {
      found_hunt_or_quest_item = true;
    }
  }

  if (portal_found || found_hunt_or_quest_item || IS_ARENA(victim->in_room) || IS_ARENA(ch->in_room))
  {
    ch->sendln("A portal shimmers into view, then fades away again.");
    act("A portal shimmers into view, then fades away again.",
        ch, 0, 0, TO_ROOM, 0);
    return eFAILURE;
  }

  make_portal(ch, victim);
  return eSUCCESS;
}

/* BURNING HANDS (scroll, wand) */

int cast_burning_hands(uint8_t level, Character *ch, char *arg, int type,
                       Character *victim, class Object *tar_obj, int skill)
{
  int retval;
  char arg1[MAX_STRING_LENGTH];
  arg1[0] = '\0';
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    retval = spell_burning_hands(level, ch, victim, 0, skill);
    //	 if (SOMEONE_DIED(retval)) return retval;
    one_argument(arg, arg1);
    Character *vict;
    if (arg1[0] && spellcraft(ch, SPELL_BURNING_HANDS))
      vict = ch->get_char_room_vis(arg1);
    else
      vict = nullptr;
    if (!vict || vict == victim)
      return retval;
    return spell_burning_hands(level, ch, vict, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_burning_hands(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_burning_hands(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_burning_hands(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in burning hands!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* CALL LIGHTNING (potion, scroll, staff) */

int cast_call_lightning(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill)
{
  extern struct weather_data weather_info;
  int retval;
  Character *next_v;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (OUTSIDE(ch) && (weather_info.sky >= SKY_RAINING))
      return spell_call_lightning(level, ch, victim, 0, skill);
    else
      ch->sendln("You fail to call upon the lightning from the sky!");
    break;
  case SPELL_TYPE_POTION:
    if (OUTSIDE(ch) && (weather_info.sky >= SKY_RAINING))
      return spell_call_lightning(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (OUTSIDE(ch) && (weather_info.sky >= SKY_RAINING))
    {
      if (victim)
        return spell_call_lightning(level, ch, victim, 0, skill);
      else if (!tar_obj)
        spell_call_lightning(level, ch, ch, 0, skill);
    }
    break;
  case SPELL_TYPE_STAFF:
    if (OUTSIDE(ch) && (weather_info.sky >= SKY_RAINING))
    {
      for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_v)
      {
        next_v = victim->next_in_room;

        if (!ARE_GROUPED(ch, victim))
        {
          retval = spell_call_lightning(level, ch, victim, 0, skill);
          if (isSet(retval, eCH_DIED))
            return retval;
        }
      }
      return eSUCCESS;
    }
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in call lightning!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* CHILL TOUCH (scroll, wand) */

int cast_chill_touch(uint8_t level, Character *ch, char *arg, int type,
                     Character *victim, class Object *tar_obj, int skill)
{

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_chill_touch(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_chill_touch(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_chill_touch(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_chill_touch(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in chill touch!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* SHOCKING GRASP (scroll, wand) */

int cast_shocking_grasp(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_shocking_grasp(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_shocking_grasp(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_shocking_grasp(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_shocking_grasp(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in shocking grasp!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* COLOUR SPRAY (scroll, wand) */

int cast_colour_spray(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_colour_spray(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_colour_spray(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_colour_spray(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_colour_spray(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in colour spray!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* DROWN (scroll, potion, wand) */

int cast_drown(uint8_t level, Character *ch, char *arg, int type,
               Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_drown(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_drown(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_drown(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_drown(level, ch, ch, 0, skill);
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_drown(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in drown!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* EARTHQUAKE (scroll, staff) */

int cast_earthquake(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_earthquake(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in earthquake!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* LIFE LEECH (scroll, staff) */

int cast_life_leech(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_life_leech(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_life_leach!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* HEROES FEAST (staff, scroll, potion, wand) */

int cast_heroes_feast(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_STAFF:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_POTION:
  case SPELL_TYPE_WAND:
    return spell_heroes_feast(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_heroes_feast!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* HEAL SPRAY (scroll, staff) */

int cast_heal_spray(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_heal_spray(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_heal_spray!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* GROUP SANCTUARY (scroll, staff) */

int cast_group_sanc(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_group_sanc(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_group_sanc!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* GROUP RECALL (scroll, staff) */

int cast_group_recall(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_group_recall(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_group_recall!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* GROUP FLY (scroll, staff) */

int cast_group_fly(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_group_fly(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_group_fly!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* FIRESTORM (scroll, staff) */

int cast_firestorm(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_firestorm(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_firestorm!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* SOLAR GATE (scroll, staff) */

int cast_solar_gate(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_solar_gate(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_firestorm!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* ENERGY DRAIN (potion, scroll, wand, staff) */

int cast_energy_drain(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_energy_drain(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_energy_drain(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_energy_drain(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_energy_drain(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_energy_drain(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_v)
    {
      next_v = victim->next_in_room;

      if (!ARE_GROUPED(ch, victim))
      {
        retval = spell_energy_drain(level, ch, victim, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in energy drain!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* SOULDRAIN (potion, scroll, wand, staff) */

int cast_souldrain(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_souldrain(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_souldrain(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_souldrain(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_souldrain(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_souldrain(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_v)
    {
      next_v = victim->next_in_room;

      if (!ARE_GROUPED(ch, victim))
      {
        retval = spell_souldrain(level, ch, victim, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in souldrain!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* VAMPIRIC TOUCH (scroll, wand) */

int cast_vampiric_touch(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_vampiric_touch(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_vampiric_touch(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_vampiric_touch(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_vampiric_touch(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in vampiric touch!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* METEOR SWARM (scroll, wand) */

int cast_meteor_swarm(uint8_t level, Character *ch, char *arg, int type,
                      Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_meteor_swarm(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_meteor_swarm(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_meteor_swarm(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_meteor_swarm(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in meteor swarm!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* FIREBALL (scroll, wand) */

int cast_fireball(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_fireball(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_fireball(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_fireball(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_fireball(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in fireball!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* SPARKS (scroll, wand) */

int cast_sparks(uint8_t level, Character *ch, char *arg, int type,
                Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_sparks(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_sparks(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_sparks(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_sparks(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in sparks!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* HOWL (scroll, wand) */

int cast_howl(uint8_t level, Character *ch, char *arg, int type,
              Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_howl(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_howl(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_howl(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_howl(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in howl!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* HARM (potion, staff, scroll) */

int cast_harm(uint8_t level, Character *ch, char *arg, int type,
              Character *victim, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_harm(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_harm(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_harm(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_harm(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_v)
    {
      next_v = victim->next_in_room;

      if (!ARE_GROUPED(ch, victim))
      {
        retval = spell_harm(level, ch, victim, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in harm!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* POWER HARM (potion, wand, staff) */

int cast_power_harm(uint8_t level, Character *ch, char *arg, int type,
                    Character *victim, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (victim)
      return spell_power_harm(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_power_harm(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_power_harm(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_power_harm(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_power_harm(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_v)
    {
      next_v = victim->next_in_room;

      if (!ARE_GROUPED(ch, victim))
      {
        retval = spell_power_harm(level, ch, victim, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in power_harm!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* DIVINE FURY (potion, staff) */

int cast_divine_fury(uint8_t level, Character *ch, char *arg, int type,
                     Character *victim, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_divine_fury(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_divine_fury(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_v)
    {
      next_v = victim->next_in_room;

      if (!ARE_GROUPED(ch, victim))
      {
        retval = spell_divine_fury(level, ch, victim, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in divine fury!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* LIGHTNING BOLT (scroll, wand) */

int cast_lightning_bolt(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_lightning_bolt(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_lightning_bolt(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_lightning_bolt(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_lightning_bolt(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in lightning bolt!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* MAGIC MISSILE (scroll, wand) */

int cast_magic_missile(uint8_t level, Character *ch, char *arg, int type,
                       Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_magic_missile(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_magic_missile(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_magic_missile(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (victim)
      return spell_magic_missile(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in magic missile!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* ARMOR (potion, scroll, wand) */

int cast_armor(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_ARMOR - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
          {
            if (ch != tar_ch)
              act("$N is protected by mystical armour.", ch, 0, tar_ch, TO_CHAR, 0);
            retval &= spell_armor(level, ch, tar_ch, 0, skill);
          }
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_ARMOR - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
        {
          if (ch != leader)
            act("$N is protected by mystical armour.", ch, 0, tar_ch, TO_CHAR, 0);
          retval &= spell_armor(level, ch, leader, 0, skill);
        }
      }
      return retval;
    }
    if (ch != tar_ch)
      act("$N is protected by mystical armour.", ch, 0, tar_ch, TO_CHAR, 0);
    return spell_armor(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_armor(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_armor(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    if (tar_ch->affected_by_spell(SPELL_ARMOR))
      return eFAILURE;
    return spell_armor(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in armor!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* HOLY AEGIS/UNHOLY AEGIS (potion, scroll, wand) */

int cast_aegis(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_aegis(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_aegis(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_aegis(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    if (tar_ch->affected_by_spell(SPELL_AEGIS))
      return eFAILURE;
    return spell_aegis(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in aegis!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int targetted_teleport(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  if (player_resist_reallocation(victim, skill))
  {
    act("$N resists your attempt to teleport $M!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to teleport $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to teleport you!", ch, nullptr, victim, TO_VICT, 0);
    return eFAILURE;
  }
  else
    return spell_teleport(level, ch, victim, 0, skill);
}

/* TELEPORT (potion, scroll, wand, staff) */

int cast_teleport(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill)
{
  Character *next_v;

  switch (type)
  {
  case SPELL_TYPE_POTION:
  case SPELL_TYPE_SPELL:
    if (!tar_ch)
      tar_ch = ch;
    return spell_teleport(level, ch, tar_ch, 0, skill);
    break;

  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_WAND:
    if (!tar_ch)
      tar_ch = ch;
    return targetted_teleport(level, ch, tar_ch, 0, skill);
    break;

  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      // must do it this way to insure staff continues in THIS room
      next_v = tar_ch->next_in_room;
      targetted_teleport(level, ch, tar_ch, 0, skill);
    }
    return eSUCCESS;
    break;

  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in teleport!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* BLESS (potion, scroll, wand) */

int cast_bless(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill)
{

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (tar_obj)
    { /* It's an object */
      if (isSet(tar_obj->obj_flags.extra_flags, ITEM_BLESS))
      {
        ch->sendln("Nothing seems to happen.");
        return eFAILURE;
      }
      return spell_bless(level, ch, 0, tar_obj, skill);
    }
    else
    { /* Then it is a PC | NPC */

      if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
      {
        int retval = eFAILURE;
        Character *leader;
        if (ch->master)
          leader = ch->master;
        else
          leader = ch;

        struct follow_type *k;
        for (k = leader->followers; k; k = k->next)
        {
          tar_ch = k->follower;
          if (ch->in_room == tar_ch->in_room)
          {
            if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_BLESS - 1)
            {
              act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
              act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
              act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
            }
            else if (GET_POS(tar_ch) == position_t::FIGHTING)
              ch->sendln("Nothing seems to happen.");
            else
              retval &= spell_bless(level, ch, tar_ch, 0, skill);
          }
        }
        if (ch->in_room == leader->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_BLESS - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else if (GET_POS(leader) == position_t::FIGHTING)
            ch->sendln("Nothing seems to happen.");
          else
            retval &= spell_bless(level, ch, leader, 0, skill);
        }

        return retval;
      }
      if (GET_POS(tar_ch) == position_t::FIGHTING)
      {
        ch->sendln("Nothing seems to happen.");
        return eFAILURE;
      }
      return spell_bless(level, ch, tar_ch, 0, skill);
    }
    break;
  case SPELL_TYPE_POTION:
    if (GET_POS(ch) == position_t::FIGHTING)
    {
      ch->sendln("Nothing seems to happen.");
      return eFAILURE;
    }
    return spell_bless(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
    { /* It's an object */
      if (isSet(tar_obj->obj_flags.extra_flags, ITEM_BLESS))
      {
        ch->sendln("Nothing seems to happen.");
        return eFAILURE;
      }
      return spell_bless(level, ch, 0, tar_obj, skill);
    }
    else
    { /* Then it is a PC | NPC */

      if (!tar_ch)
        tar_ch = ch;

      if (GET_POS(tar_ch) == position_t::FIGHTING)
      {
        ch->sendln("Nothing seems to happen.");
        return eFAILURE;
      }
      return spell_bless(level, ch, tar_ch, 0, skill);
    }
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
    { /* It's an object */
      if (isSet(tar_obj->obj_flags.extra_flags, ITEM_BLESS))
      {
        ch->sendln("Nothing seems to happen.");
        return eFAILURE;
      }
      return spell_bless(level, ch, 0, tar_obj, skill);
    }
    else
    { /* Then it is a PC | NPC */

      if (GET_POS(tar_ch) == position_t::FIGHTING)
      {
        ch->sendln("Nothing seems to happen.");
        return eFAILURE;
      }
      return spell_bless(level, ch, tar_ch, 0, skill);
    }
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in bless!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* PARALYZE (potion, scroll, wand, staff) */

int cast_paralyze(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill)
{
  int retval;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You can not paralyze anyone in a safe area!");
    return eFAILURE;
  }
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (IS_AFFECTED(tar_ch, AFF_PARALYSIS))
    {
      ch->sendln("Nothing seems to happen.");
      return eFAILURE;
    }
    return spell_paralyze(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    if (IS_AFFECTED(ch, AFF_PARALYSIS))
      return eFAILURE;
    return spell_paralyze(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    if (IS_AFFECTED(tar_ch, AFF_PARALYSIS))
      return eFAILURE;
    return spell_paralyze(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    if (IS_AFFECTED(tar_ch, AFF_PARALYSIS))
      return eFAILURE;
    return spell_paralyze(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      if (IS_NPC(tar_ch))
        if (!(IS_AFFECTED(tar_ch, AFF_PARALYSIS)))
        {
          retval = spell_paralyze(level, ch, tar_ch, 0, skill);
          if (isSet(retval, eCH_DIED))
            return retval;
        }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in paralyze!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* BLINDNESS (potion, scroll, wand, staff) */

int cast_blindness(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You can not blind anyone in a safe area!");
    return eFAILURE;
  }

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (IS_AFFECTED(tar_ch, AFF_BLIND))
    {
      ch->sendln("Nothing seems to happen.");
      return eFAILURE;
    }
    return spell_blindness(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    if (IS_AFFECTED(ch, AFF_BLIND))
      return eFAILURE;
    return spell_blindness(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    if (IS_AFFECTED(tar_ch, AFF_BLIND))
      return eFAILURE;
    return spell_blindness(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    if (IS_AFFECTED(tar_ch, AFF_BLIND))
      return eFAILURE;
    return spell_blindness(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!IS_AFFECTED(tar_ch, AFF_BLIND))
      {
        retval = spell_blindness(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in blindness!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* CONTROL WEATHER */

int cast_control_weather(uint8_t level, Character *ch, char *arg, int type,
                         Character *tar_ch, class Object *tar_obj, int skill)
{
  char buffer[MAX_STRING_LENGTH];
  extern struct weather_data weather_info;

  switch (type)
  {
  case SPELL_TYPE_SPELL:

    one_argument(arg, buffer);

    if (str_cmp("better", buffer) && str_cmp("worse", buffer))
    {
      ch->sendln("Do you want it to get better or worse?");
      return eFAILURE;
    }

    if (!str_cmp("better", buffer))
    {
      weather_info.change += (dice(((level) / 3), 9));
      ch->sendln("The skies around you grow a bit clearer.");
      act("$n's magic causes the skies around you to grow a bit clearer.", ch, 0, 0, TO_ROOM, 0);
    }
    else
    {
      weather_info.change -= (dice(((level) / 3), 9));
      ch->sendln("The skies around you grow a bit darker.");
      act("$n's magic causes the skies around you to grow a bit darker.", ch, 0, 0, TO_ROOM, 0);
    }
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in control weather!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* CREATE FOOD (wand, staff, scroll) */

int cast_create_food(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    act("$n magically creates a mushroom.", ch, 0, 0, TO_ROOM, 0);
    return spell_create_food(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    return spell_create_food(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_create_food(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    return spell_create_food(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    return spell_create_food(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in create food!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* CREATE WATER (wand, scroll) */

int cast_create_water(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (tar_obj->obj_flags.type_flag != ITEM_DRINKCON)
    {
      ch->sendln("It is unable to hold water.");
      return eFAILURE;
    }
    return spell_create_water(level, ch, 0, tar_obj, skill);
    break;
  case SPELL_TYPE_WAND:
    if (!tar_obj)
      return eFAILURE;
    //         if(tar_ch) return eFAILURE;
    return spell_create_food(level, ch, 0, tar_obj, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (!tar_obj)
      return eFAILURE;
    //         if(tar_ch) return eFAILURE;
    return spell_create_water(level, ch, 0, tar_obj, skill);
    break;

  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in create water!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* REMOVE PARALYSIS (potion, wand, scroll, staff) */

int cast_remove_paralysis(uint8_t level, Character *ch, char *arg, int type,
                          Character *tar_ch, class Object *tar_obj, int skill)
{
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_remove_paralysis(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_remove_paralysis(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    return spell_remove_paralysis(level, ch, tar_ch, 0, skill);

  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_remove_paralysis(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
    {
      retval = spell_remove_paralysis(level, ch, tar_ch, 0, skill);
      if (isSet(retval, eCH_DIED))
        return retval;
    }
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in remove paralysis!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* REMOVE BLIND (potion, scroll, staff) */

int cast_remove_blind(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_remove_blind(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_remove_blind(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_remove_blind(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_remove_blind(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw up in remove blind!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* EDITING ENDS HERE */

int cast_cure_critic(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_CURE_CRITIC - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_cure_critic(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_CURE_CRITIC - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
          retval &= spell_cure_critic(level, ch, leader, 0, skill);
      }

      return retval;
    }
    return spell_cure_critic(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_cure_critic(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_cure_critic(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_cure_critic(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cure critic!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_cure_light(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_CURE_LIGHT - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_cure_light(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_CURE_LIGHT - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
          retval &= spell_cure_light(level, ch, leader, 0, skill);
      }

      return retval;
    }
    return spell_cure_light(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (!tar_ch)
      return eFAILURE;
    return spell_cure_light(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_cure_light(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_cure_light(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_cure_light(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cure light!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_curse(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill)
{
  int retval;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) && tar_ch)
  {
    ch->sendln("You cannot curse someone in a safe area!");
    return eFAILURE;
  }

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (tar_obj) /* It is an object */
      return spell_curse(level, ch, 0, tar_obj, skill);
    else
    { /* Then it is a PC | NPC */
      return spell_curse(level, ch, tar_ch, 0, skill);
    }
    break;
  case SPELL_TYPE_POTION:
    return spell_curse(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj) /* It is an object */
      return spell_curse(level, ch, 0, tar_obj, skill);
    else
    { /* Then it is a PC | NPC */
      if (!tar_ch)
        tar_ch = ch;
      return spell_curse(level, ch, tar_ch, 0, skill);
    }
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return spell_curse(level, ch, 0, tar_obj, skill);
    else
    {
      if (!tar_ch)
        tar_ch = ch;
      return spell_curse(level, ch, tar_ch, 0, skill);
    }
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      if (IS_NPC(tar_ch))
      {
        retval = spell_curse(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in curse!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_detect_evil(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_detect_evil(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_detect_evil(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_detect_evil(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_detect_evil(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in detect evil!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_true_sight(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_true_sight(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_true_sight(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (!tar_ch)
      tar_ch = ch;
    return spell_true_sight(level, ch, tar_ch, 0, skill);
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_true_sight(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_true_sight(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in true sight!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_detect_good(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_detect_good(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_detect_good(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_detect_good(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_detect_good(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in detect good!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_detect_invisibility(uint8_t level, Character *ch, char *arg, int type,
                             Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_DETECT_INVISIBLE - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_detect_invisibility(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_DETECT_INVISIBLE - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
          retval &= spell_detect_invisibility(level, ch, leader, 0, skill);
      }

      return retval;
    }
    return spell_detect_invisibility(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_detect_invisibility(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (!tar_ch)
      return eFAILURE;
    return spell_detect_invisibility(level, ch, tar_ch, 0, skill);
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_detect_invisibility(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_detect_invisibility(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in detect invisibility!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_detect_magic(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_DETECT_MAGIC - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_detect_magic(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_DETECT_MAGIC - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
          retval &= spell_detect_magic(level, ch, leader, 0, skill);
      }

      return retval;
    }
    return spell_detect_magic(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_detect_magic(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_detect_magic(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_detect_magic(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in detect magic!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_haste(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_haste(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_haste(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_haste(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_haste(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      if (!(IS_AFFECTED(tar_ch, SPELL_HASTE)))
        spell_haste(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in haste!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_detect_poison(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_DETECT_POISON - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_detect_poison(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_DETECT_POISON - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
          retval &= spell_detect_poison(level, ch, leader, 0, skill);
      }

      return retval;
    }
    return spell_detect_poison(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_detect_poison(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
    {
      return spell_detect_poison(level, ch, 0, tar_obj, skill);
    }
    if (!tar_ch)
      tar_ch = ch;
    return spell_detect_poison(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in detect poison!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_dispel_evil(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;
  Object *next_o;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_dispel_evil(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_dispel_evil(level, ch, ch, tar_obj, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_dispel_evil(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    return spell_dispel_evil(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_dispel_evil(level, ch, tar_ch, tar_obj, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    for (tar_obj = DC::getInstance()->world[ch->in_room].contents; tar_obj; tar_obj = next_o)
    {
      next_o = tar_obj->next;

      retval = spell_dispel_evil(level, ch, 0, tar_obj, skill);
      if (isSet(retval, eCH_DIED))
        return retval;
    }
    return eSUCCESS;
    break;

  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in dispel evil!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_dispel_good(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;
  Object *next_o;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_dispel_good(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_dispel_good(level, ch, ch, tar_obj, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_dispel_good(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    return spell_dispel_good(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_dispel_good(level, ch, tar_ch, tar_obj, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    for (tar_obj = DC::getInstance()->world[ch->in_room].contents; tar_obj; tar_obj = next_o)
    {
      next_o = tar_obj->next;

      retval = spell_dispel_good(level, ch, 0, tar_obj, skill);
      if (isSet(retval, eCH_DIED))
        return retval;
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in dispel good!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_enchant_armor(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_enchant_armor(level, ch, 0, tar_obj, skill);
    break;

  case SPELL_TYPE_SCROLL:
    if (!tar_obj)
      return eFAILURE;
    return spell_enchant_armor(level, ch, 0, tar_obj, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in enchant weapon!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_enchant_weapon(uint8_t level, Character *ch, char *arg, int type,
                        Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_enchant_weapon(level, ch, 0, tar_obj, skill);
    break;

  case SPELL_TYPE_SCROLL:
    if (!tar_obj)
      return eFAILURE;
    return spell_enchant_weapon(level, ch, 0, tar_obj, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in enchant weapon!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_mana(uint8_t level, Character *ch, char *arg, int type,
              Character *tar_ch, class Object *tar_obj, int skill)
{

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_mana(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (!tar_ch)
      tar_ch = ch;
    return spell_mana(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_mana(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_mana(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in mana!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_heal(uint8_t level, Character *ch, char *arg, int type,
              Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_heal(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (!tar_ch)
      return eFAILURE;
    return spell_heal(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_heal(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_heal(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_heal(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in heal!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_power_heal(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_power_heal(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_power_heal(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    spell_power_heal(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (!tar_ch)
      return eFAILURE;
    return spell_power_heal(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_power_heal(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in power_heal!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_full_heal(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (!tar_ch)
      tar_ch = ch;
    return spell_full_heal(level, ch, tar_ch, 0, skill);
  case SPELL_TYPE_SPELL:
    return spell_full_heal(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_full_heal(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_full_heal(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in full_heal!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_invisibility(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (tar_obj)
    {
      if (isSet(tar_obj->obj_flags.extra_flags, ITEM_INVISIBLE))
        ch->sendln("Nothing new seems to happen.");
      else
        return spell_invisibility(level, ch, 0, tar_obj, skill);
    }
    else
    { /* tar_ch */
      return spell_invisibility(level, ch, tar_ch, 0, skill);
    }
    break;
  case SPELL_TYPE_POTION:
    return spell_invisibility(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
    {
      if (!(isSet(tar_obj->obj_flags.extra_flags, ITEM_INVISIBLE)))
        return spell_invisibility(level, ch, 0, tar_obj, skill);
    }
    else
    { /* tar_ch */
      if (!tar_ch)
        tar_ch = ch;

      return spell_invisibility(level, ch, tar_ch, 0, skill);
    }
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
    {
      if (!(isSet(tar_obj->obj_flags.extra_flags, ITEM_INVISIBLE)))
        return spell_invisibility(level, ch, 0, tar_obj, skill);
    }
    else
    { /* tar_ch */
      return spell_invisibility(level, ch, tar_ch, 0, skill);
    }
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_invisibility(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in invisibility!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_locate_object(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_locate_object(level, ch, arg, 0, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_ch)
      return eFAILURE;
    return spell_locate_object(level, ch, arg, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in locate object!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_poison(uint8_t level, Character *ch, char *arg, int type,
                Character *tar_ch, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;

  switch (type)
  {
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    return spell_poison(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SPELL:
    if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
    {
      ch->sendln("You can not poison someone in a safe area!");
      return eFAILURE;
    }
    return spell_poison(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_poison(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_poison(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;

  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in poison!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_protection_from_evil(uint8_t level, Character *ch, char *arg, int type,
                              Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (IS_EVIL(ch) && ch->getLevel() < ARCHANGEL)
    {
      ch->sendln("You are too evil to invoke the protection of your god.");
      return eFAILURE;
    }
    else if (ch != tar_ch && GET_CLASS(ch) != CLASS_CLERIC)
    {
      /* only let clerics cast on others */
      ch->sendln("You can only cast this spell on yourself.");
      return eFAILURE;
    }
    else
    {

      if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
      {
        int retval = eFAILURE;
        Character *leader;
        if (ch->master)
          leader = ch->master;
        else
          leader = ch;

        struct follow_type *k;
        for (k = leader->followers; k; k = k->next)
        {
          tar_ch = k->follower;
          if (ch->in_room == tar_ch->in_room)
          {
            if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_PROTECT_FROM_EVIL - 1)
            {
              act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
              act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
              act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
            }
            else
              retval &= spell_protection_from_evil(level, ch, tar_ch, 0, skill);
          }
        }
        if (ch->in_room == leader->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_PROTECT_FROM_EVIL - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_protection_from_evil(level, ch, leader, 0, skill);
        }

        return retval;
      }

      return spell_protection_from_evil(level, ch, tar_ch, 0, skill);
    }
    break;

  case SPELL_TYPE_POTION:
    return spell_protection_from_evil(level, ch, ch, 0, 0);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    return spell_protection_from_evil(level, ch, tar_ch, 0, 0);
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_protection_from_evil(level, ch, tar_ch, 0, 0);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
      spell_protection_from_evil(level, ch, tar_ch, 0, 0);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in protection from evil!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_protection_from_good(uint8_t level, Character *ch, char *arg, int type,
                              Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (IS_GOOD(ch) && ch->getLevel() < ARCHANGEL)
    {
      ch->sendln("Your goodness finds disfavor amongst the forces of darkness.");
      return eFAILURE;
    }
    else if (ch != tar_ch && GET_CLASS(ch) != CLASS_CLERIC)
    {
      /* only let clerics cast on others */
      ch->sendln("You can only cast this spell on yourself.");
      return eFAILURE;
    }
    else
    {
      if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
      {
        int retval = eFAILURE;
        Character *leader;
        if (ch->master)
          leader = ch->master;
        else
          leader = ch;

        struct follow_type *k;
        for (k = leader->followers; k; k = k->next)
        {
          tar_ch = k->follower;
          if (ch->in_room == tar_ch->in_room)
          {
            if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_PROTECT_FROM_GOOD - 1)
            {
              act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
              act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
              act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
            }
            else
              retval &= spell_protection_from_good(level, ch, tar_ch, 0, skill);
          }
        }
        if (ch->in_room == leader->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_PROTECT_FROM_EVIL - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_protection_from_good(level, ch, leader, 0, skill);
        }

        return retval;
      }

      return spell_protection_from_good(level, ch, tar_ch, 0, skill);
    }
    break;

  case SPELL_TYPE_POTION:
    return spell_protection_from_good(level, ch, ch, 0, 0);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    return spell_protection_from_good(level, ch, tar_ch, 0, 0);
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_protection_from_good(level, ch, tar_ch, 0, 0);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
      spell_protection_from_good(level, ch, tar_ch, 0, 0);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in protection from good!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_remove_curse(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill, quint64 mana_cost)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_remove_curse(level, ch, tar_ch, tar_obj, skill, mana_cost);
    break;
  case SPELL_TYPE_POTION:
    return spell_remove_curse(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
    {
      return spell_remove_curse(level, ch, 0, tar_obj, skill);
    }
    if (!tar_ch)
      tar_ch = ch;
    return spell_remove_curse(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
    {
      return spell_remove_curse(level, ch, 0, tar_obj, skill);
    }
    if (!tar_ch)
      tar_ch = ch;
    return spell_remove_curse(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_remove_curse(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in remove curse!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_remove_poison(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char;
           tmp_char = tmp_char->next_in_room)
      {
        if (!ARE_GROUPED(ch, tmp_char))
          continue;

        retval &= spell_remove_poison(level, ch, tmp_char, tar_obj, skill);
        if (isSet(retval, eCH_DIED))
        {
          return retval;
        }
      }

      return retval;
    }

    return spell_remove_poison(level, ch, tar_ch, tar_obj, skill);
    break;

  case SPELL_TYPE_WAND:
    if (!tar_ch)
      return eFAILURE;
    return spell_remove_poison(level, ch, tar_ch, 0, skill);
    break;

  case SPELL_TYPE_POTION:
    return spell_remove_poison(level, ch, ch, 0, skill);
    break;

  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_remove_poison(level, ch, tar_ch, 0, skill);
    break;

  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_remove_poison(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in remove poison!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }

  return eFAILURE;
}

int cast_fireshield(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_fireshield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_fireshield(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_fireshield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_fireshield(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in fireshield!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_sleep(uint8_t level, Character *ch, char *arg, int type,
               Character *tar_ch, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You can not sleep someone in a safe area!");
    return eFAILURE;
  }
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_sleep(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_sleep(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_sleep(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    return spell_sleep(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_sleep(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in sleep!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_strength(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_strength(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_strength(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_strength(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_strength(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in strength!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_ventriloquate(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill)
{
  Character *tmp_ch;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];

  if (type != SPELL_TYPE_SPELL)
  {
    DC::getInstance()->logentry(QStringLiteral("Attempt to ventriloquate by non-cast-spell."), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }
  for (; *arg && (*arg == ' '); arg++)
    ;
  if (tar_obj)
  {
    sprintf(buf1, "The %s says '%s'\r\n", qPrintable(fname(tar_obj->Name())), arg);
    sprintf(buf2, "Someone makes it sound like the %s says '%s'.\r\n", qPrintable(fname(tar_obj->Name())), arg);
  }
  else
  {
    sprintf(buf1, "%s says '%s'\r\n", GET_SHORT(tar_ch), arg);
    sprintf(buf2, "Someone makes it sound like %s says '%s'\r\n",
            GET_SHORT(tar_ch), arg);
  }

  sprintf(buf3, "Someone says, '%s'\r\n", arg);

  for (tmp_ch = DC::getInstance()->world[ch->in_room].people; tmp_ch;
       tmp_ch = tmp_ch->next_in_room)
  {

    if ((tmp_ch != ch) && (tmp_ch != tar_ch))
    {
      if (saves_spell(ch, tmp_ch, 0, SAVE_TYPE_MAGIC) >= 0)
        send_to_char(buf2, tmp_ch);
      else
        send_to_char(buf1, tmp_ch);
    }
    else
    {
      if (tmp_ch == tar_ch)
        send_to_char(buf3, tar_ch);
    }
    send_to_char(buf1, ch);
  }
  return eSUCCESS;
}

int targetted_word_of_recall(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  if (player_resist_reallocation(victim, skill))
  {
    act("$N resists your attempt to recall $M!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to recall $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to recall you!", ch, nullptr, victim, TO_VICT, 0);
    return eFAILURE;
  }
  else
    return spell_word_of_recall(level, ch, victim, 0, skill);
}

/* WORD OF RECALL (spell, potion, wand, staff, scroll)*/
int cast_word_of_recall(uint8_t level, Character *ch, char *arg, int type,
                        Character *tar_ch, class Object *tar_obj, int skill)
{
  Character *tar_ch_next;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_POTION:
    return spell_word_of_recall(level, ch, ch, 0, skill);
    break;

  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return targetted_word_of_recall(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch_next)
    {
      tar_ch_next = tar_ch->next_in_room;
      if (IS_PC(tar_ch))
        targetted_word_of_recall(level, ch, tar_ch, 0, skill);
    }
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in word of recall!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_wizard_eye(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_wizard_eye(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in wizard eye!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_eagle_eye(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_eagle_eye(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in eagle eye!"),
                                ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_summon(uint8_t level, Character *ch, char *arg, int type,
                Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_summon(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in summon!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_charm_person(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_charm_person(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (!tar_ch)
      return eFAILURE;
    return spell_charm_person(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_charm_person(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;

  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in charm person!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_sense_life(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_sense_life(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_sense_life(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    return spell_sense_life(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_sense_life(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in sense life!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_identify(uint8_t level, Character *ch, char *arg, int type,
                  Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_identify(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_SCROLL:
    return spell_identify(level, ch, tar_ch, tar_obj, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in identify!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_frost_breath(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_frost_breath(level, ch, tar_ch, 0, skill);
    break; /* It's a spell.. But people can't cast it! */
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in frostbreath!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_acid_breath(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_acid_breath(level, ch, tar_ch, 0, skill);
    break; /* It's a spell.. But people can't cast it! */
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in acidbreath!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_fire_breath(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_fire_breath(level, ch, ch, 0, skill);
    break;
    /* THIS ONE HURTS!! */
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in firebreath!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_gas_breath(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_gas_breath(level, ch, ch, 0, skill);
    break;
    /* THIS ONE HURTS!! */
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in gasbreath!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_lightning_breath(uint8_t level, Character *ch, char *arg, int type,
                          Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_lightning_breath(level, ch, tar_ch, 0, skill);
    break; /* It's a spell.. But people can't cast it! */
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in lightningbreath!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_fear(uint8_t level, Character *ch, char *arg, int type,
              Character *tar_ch, class Object *tar_obj, int skill)
{
  int retval;
  Character *next_v;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You can not fear someone in a safe area!");
    return eFAILURE;
  }

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_fear(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_fear(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    return spell_fear(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_fear(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in fear!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_refresh(uint8_t level, Character *ch, char *arg, int type,
                 Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_REFRESH - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_refresh(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_REFRESH - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
          retval &= spell_refresh(level, ch, leader, 0, skill);
      }
      return retval;
    }
    return spell_refresh(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    tar_ch = ch;
    return spell_refresh(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_refresh(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_refresh(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_refresh(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in refresh!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_fly(uint8_t level, Character *ch, char *arg, int type,
             Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_FLY - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_fly(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_FLY - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        retval &= spell_fly(level, ch, leader, 0, skill);
      }

      return retval;
    }
    return spell_fly(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_fly(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_fly(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    tar_ch = ch;
    return spell_fly(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_fly(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in fly!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_cont_light(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_cont_light(level, ch, 0, tar_obj, skill);
    break;
  case SPELL_TYPE_SCROLL:
    return spell_cont_light(level, ch, 0, tar_obj, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cont_light"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_know_alignment(uint8_t level, Character *ch, char *arg, int type,
                        Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_know_alignment(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_know_alignment(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_know_alignment(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_know_alignment(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_know_alignment(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in know alignment!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

char *dispel_magic_spells[] =
    {
        "", "sanctuary", "protection_from_evil", "haste", "stone_shield", "greater_stone_shield",
        "frost_shield", "lightning_shield", "fire_shield", "acid_shield", "protection_from_good", "\n"};

int cast_dispel_magic(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  int spell = 0;
  char buffer[MAX_INPUT_LENGTH];
  one_argument(arg, buffer);
  if (type == SPELL_TYPE_SPELL && arg && *arg)
  {
    int i;
    for (i = 1; *dispel_magic_spells[i] != '\n'; i++)
    {
      if (!str_prefix(buffer, dispel_magic_spells[i]))
        spell = i;
    }
    if (!spell)
    {
      ch->sendln("You cannot target that spell.");
      return eFAILURE;
    }
  }
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_dispel_magic(level, ch, tar_ch, 0, skill, spell);
    break;
  case SPELL_TYPE_POTION:
    tar_ch = ch;
    return spell_dispel_magic(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_dispel_magic(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_dispel_magic(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_dispel_magic(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in dispel magic!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

char *dispel_minor_spells[] =
    {
        "", "invisibility", "detect_invisibility", "camouflage", "resist_acid",
        "resist_cold", "resist_fire", "resist_energy", "barkskin",
        "stoneskin", "fly", "true_sight", "water_breath", "armor",
        "shield", "resist_magic", "\n"

};

int cast_dispel_minor(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  int spell = 0;
  char buffer[MAX_INPUT_LENGTH];
  one_argument(arg, buffer);
  if (!tar_obj && type == SPELL_TYPE_SPELL && arg && *arg)
  {
    int i = {};
    for (i = 1; *dispel_minor_spells[i] != '\n'; i++)
    {
      if (!str_prefix(buffer, dispel_minor_spells[i]))
      {
        spell = i;
      }
    }

    if (spell == 0)
    {
      ch->sendln("You cannot target that spell.");
      return eFAILURE;
    }
  }
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_dispel_minor(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (!tar_ch)
      tar_ch = ch;
    return spell_dispel_minor(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_WAND:
    if (!tar_ch)
      tar_ch = ch;
    return spell_dispel_minor(level, ch, tar_ch, tar_obj, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_dispel_minor(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in dispel minor!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int elemental_damage_bonus(int spell, Character *ch)
{
  Character *mst = ch->master ? ch->master : ch;
  struct follow_type *f, *t;
  bool fire, ice, earth, energy;
  fire = ice = earth = energy = false;
  for (f = mst->followers; f; f = f->next)
  {
    // if (IS_NPC(f->follower) && f->follower->height == 77)
    if (IS_NPC(f->follower) && f->follower->mobdata->mob_flags.value[3] == 77)
    {
      switch (DC::getInstance()->mob_index[f->follower->mobdata->nr].virt)
      {
      case 88:
        fire = true;
        break;
      case 89:
        ice = true;
        break;
      case 90:
        energy = true;
        break;
      case 91:
        earth = true;
        break;
      default:
        break;
      }
    }
    else
    {
      for (t = f->follower->followers; t; t = t->next)
        // if (IS_NPC(t->follower) && t->follower->height == 77)
        if (IS_NPC(t->follower) && t->follower->mobdata->mob_flags.value[3] == 77)
        {
          switch (DC::getInstance()->mob_index[t->follower->mobdata->nr].virt)
          {
          case 88:
            fire = true;
            break;
          case 89:
            ice = true;
            break;
          case 90:
            energy = true;
            break;
          case 91:
            earth = true;
            break;
          default:
            break;
          }
        }
    }
  }
  switch (spell)
  {
    // done this way to ensure no unintended spells get the bonus
    // (shields, for instance, prolly other things with TYPE_FIRE etc damage that's unintended)
  case SPELL_FIREBALL:
  case SPELL_FIRESTORM:
  case SPELL_FLAMESTRIKE:
  case SPELL_HELLSTREAM:
  case SPELL_BURNING_HANDS:
  case SPELL_SOLAR_GATE:
  case SPELL_SPARKS:
    if (fire)
      return dice(9, 11);
    else
      return 0;
  case SPELL_METEOR_SWARM:
  case SPELL_BEE_STING:
  case SPELL_CREEPING_DEATH:
  case SPELL_BLUE_BIRD:
  case SPELL_COLOUR_SPRAY:
  case SPELL_MAGIC_MISSILE:
  case SPELL_BEE_SWARM:
    if (earth)
      return dice(13, 7);
    else
      return 0;
  case SPELL_CALL_LIGHTNING:
  case SPELL_SUN_RAY:
  case SPELL_SHOCKING_GRASP:
  case SPELL_LIGHTNING_BOLT:
    if (energy)
      return dice(2, 60);
    else
      return 0;
  case SPELL_DROWN:
  case SPELL_VAMPIRIC_TOUCH:
  case SPELL_DISPEL_EVIL:
  case SPELL_DISPEL_GOOD:
  case SPELL_CHILL_TOUCH:
  case SPELL_ICESTORM:
    if (ice)
      return dice(11, 9);
    else
      return 0;
  default:
    return 0;
  }
}

bool elemental_score(Character *ch, int level)
{
  Character *mst = ch->master ? ch->master : ch;
  struct follow_type *f, *t;
  bool fire, ice, earth, energy;
  fire = ice = earth = energy = false;
  // reuse of elemental damage function
  for (f = mst->followers; f; f = f->next)
  {
    if (IS_NPC(f->follower))
    {
      // if (f->follower->height == 77) // improved
      if (f->follower->mobdata->mob_flags.value[3] == 77)
        switch (DC::getInstance()->mob_index[f->follower->mobdata->nr].virt)
        {
        case 88:
          fire = true;
          break;
        case 89:
          ice = true;
          break;
        case 90:
          energy = true;
          break;
        case 91:
          earth = true;
          break;
        default:
          break;
        }
    }
    else
    {
      for (t = f->follower->followers; t; t = t->next)
      {
        if (IS_NPC(t->follower))
        {
          if (t->follower->mobdata->mob_flags.value[3] == 77)
          {
            switch (DC::getInstance()->mob_index[t->follower->mobdata->nr].virt)
            {
            case 88:
              fire = true;
              break;
            case 89:
              ice = true;
              break;
            case 90:
              energy = true;
              break;
            case 91:
              earth = true;
              break;
            default:
              break;
            }
          }
        }
      }
    }
  }
  char buf[MAX_STRING_LENGTH];
  extern char frills[];
  if (fire)
  {
    sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
            frills[level], "Enhanced Fire Aura", "NONE", frills[level]);
    ch->send(buf);
    if (++level == 4)
      level = 0;
  }
  if (ice)
  {
    sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
            frills[level], "Enhanced Cold Aura", "NONE", frills[level]);
    ch->send(buf);
    if (++level == 4)
      level = 0;
  }
  if (energy)
  {
    sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
            frills[level], "Enhanced Energy Aura", "NONE", frills[level]);
    ch->send(buf);
    if (++level == 4)
      level = 0;
  }
  if (earth)
  {
    sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
            frills[level], "Enhanced Physical Aura", "NONE", frills[level]);
    ch->send(buf);
    if (++level == 4)
      level = 0;
  }
  return (fire || earth || energy || ice);
}

int cast_conjure_elemental(uint8_t level, Character *ch,
                           char *arg, int type,
                           Character *tar_ch, class Object *tar_obj, int skill)
{
  return spell_conjure_elemental(level, ch, arg, 0, tar_obj, skill);
}

int cast_cure_serious(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_CURE_SERIOUS - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_cure_serious(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_CURE_SERIOUS - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
          retval &= spell_cure_serious(level, ch, leader, 0, skill);
      }

      return retval;
    }
    return spell_cure_serious(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_cure_serious(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_cure_serious(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_cure_serious(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_cure_serious(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cure serious!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_cause_light(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_cause_light(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_cause_light(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_cause_light(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_cause_light(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;

  case SPELL_TYPE_POTION:
    return spell_cause_light(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cause light!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_cause_critical(uint8_t level, Character *ch, char *arg,
                        int type, Character *tar_ch,
                        class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_cause_critical(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_cause_critical(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_cause_critical(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_cause_critical(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  case SPELL_TYPE_POTION:
    return spell_cause_critical(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cause critical!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_cause_serious(uint8_t level, Character *ch, char *arg,
                       int type, Character *tar_ch,
                       class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_cause_serious(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_cause_serious(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_cause_serious(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_cause_serious(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  case SPELL_TYPE_POTION:
    return spell_cause_serious(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cause serious!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_flamestrike(uint8_t level, Character *ch, char *arg,
                     int type, Character *tar_ch,
                     class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_flamestrike(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_flamestrike(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_flamestrike(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_flamestrike(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in flamestrike!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_resist_cold(uint8_t level, Character *ch, char *arg,
                     int type, Character *tar_ch,
                     class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_resist_cold(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_resist_cold(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_resist_cold(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in resist_cold!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}
int cast_staunchblood(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!tar_ch)
      tar_ch = ch;
    return spell_staunchblood(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_staunchblood(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_staunchblood(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_staunchblood!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_resist_energy(uint8_t level, Character *ch, char *arg, int type,
                       Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_resist_energy(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_resist_energy(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_resist_energy(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in resist energy!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_resist_fire(uint8_t level, Character *ch, char *arg,
                     int type, Character *tar_ch,
                     class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_resist_fire(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_resist_fire(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_resist_fire(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in resist_fire!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_resist_magic(uint8_t level, Character *ch, char *arg,
                      int type, Character *tar_ch,
                      class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!strcmp(arg, "communegroupspell") && ch->has_skill(SKILL_COMMUNE))
    {
      int retval = eFAILURE;
      Character *leader;
      if (ch->master)
        leader = ch->master;
      else
        leader = ch;

      struct follow_type *k;
      for (k = leader->followers; k; k = k->next)
      {
        tar_ch = k->follower;
        if (ch->in_room == tar_ch->in_room)
        {
          if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_RESIST_MAGIC - 1)
          {
            act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
            act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
          }
          else
            retval &= spell_resist_magic(level, ch, tar_ch, 0, skill);
        }
      }
      if (ch->in_room == leader->in_room)
      {
        if (tar_ch->affected_by_spell(SPELL_IMMUNITY) && tar_ch->affected_by_spell(SPELL_IMMUNITY)->modifier == SPELL_RESIST_MAGIC - 1)
        {
          act("Your shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_CHAR, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses your magic.", ch, 0, tar_ch, TO_VICT, 0);
          act("$N's shield of holy immunity $Bs$3h$5i$7m$3m$5e$7r$3s$R briefly and disperses $n's magic.", ch, 0, tar_ch, TO_ROOM, NOTVICT);
        }
        else
          retval &= spell_resist_magic(level, ch, leader, 0, skill);
      }

      return retval;
    }
    return spell_resist_magic(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_resist_magic(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_resist_magic(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in resist_magic!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_stone_skin(uint8_t level, Character *ch, char *arg,
                    int type, Character *tar_ch,
                    class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_stone_skin(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_stone_skin(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_stone_skin(level, tar_ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in stone skin!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_shield(uint8_t level, Character *ch, char *arg,
                int type, Character *tar_ch,
                class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_shield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_shield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_shield(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_shield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)
      spell_shield(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in shield!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_weaken(uint8_t level, Character *ch, char *arg,
                int type, Character *tar_ch,
                class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You can not weaken anyone in a safe area!");
    return eFAILURE;
  }

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_weaken(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_weaken(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_weaken(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_weaken(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_weaken(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in weaken!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_mass_invis(uint8_t level, Character *ch, char *arg,
                    int type, Character *tar_ch,
                    class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_mass_invis(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    return spell_mass_invis(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    return spell_mass_invis(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in mass invis!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_acid_blast(uint8_t level, Character *ch, char *arg,
                    int type, Character *tar_ch,
                    class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_acid_blast(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_acid_blast(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_acid_blast(level, ch, tar_ch, 0, skill);
    break;

  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_acid_blast(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_acid_blast(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in acid blast!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_hellstream(uint8_t level, Character *ch, char *arg,
                    int type, Character *tar_ch,
                    class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_hellstream(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_hellstream(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_hellstream(level, ch, ch, 0, skill);
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_hellstream(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_hellstream(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in hell stream!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_portal(uint8_t level, Character *ch, char *arg,
                int type, Character *tar_ch,
                class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (ch->isPlayer() && GET_CLASS(ch) == CLASS_CLERIC)
    {
      if ((GET_MANA(ch) - 90) < 0)
      {
        ch->sendln("You just don't have the energy!");
        GET_MANA(ch) += 50;
        return eFAILURE;
      }
      else
      {
        GET_MANA(ch) -= 90;
      }
    }

    return spell_portal(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in portal!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_infravision(uint8_t level, Character *ch, char *arg,
                     int type, Character *tar_ch,
                     class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_infravision(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_infravision(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_infravision(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_infravision(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people;
         tar_ch; tar_ch = tar_ch->next_in_room)

      spell_infravision(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in infravision!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int cast_animate_dead(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_animate_dead(level, ch, 0, tar_obj, skill);
    break;

  case SPELL_TYPE_SCROLL:
    if (!tar_obj)
      return eFAILURE;
    return spell_animate_dead(level, ch, 0, tar_obj, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in Animate Dead!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* BEE STING */

int spell_bee_sting(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int retval;
  int bees = 1 + (ch->getLevel() / 15) + (ch->getLevel() == 60);
  affected_type af;
  int i;
  set_cantquit(ch, victim);
  dam = dice(4, 3) + skill / 3 + getRealSpellDamage(ch);
  int weap_spell = obj ? WEAR_WIELD : 0;

  for (i = 0; i < bees; i++)
  {

    retval = damage(ch, victim, dam, TYPE_PHYSICAL_MAGIC, SPELL_BEE_STING, weap_spell);
    dam = dice(4, 3) + skill / 3;
    if (SOMEONE_DIED(retval))
      return retval;
  }
  // Extra added bonus 1% of the time
  if (dice(1, 100) == 3)
    if (!isSet(victim->immune, ISR_POISON))
      if (saves_spell(ch, victim, 0, SAVE_TYPE_POISON) < 0)
      {
        af.type = SPELL_POISON;
        af.duration = level * 2;
        af.modifier = -2;
        af.location = APPLY_STR;
        af.bitvector = AFF_POISON;
        affect_join(victim, &af, false, false);
        victim->sendln("You seem to have an allergic reaction to these bees!");
        act("$N seems to be allergic to your bees!", ch, 0, victim,
            TO_CHAR, 0);
      }
  return eSUCCESS;
}

int cast_bee_sting(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    return spell_bee_sting(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SPELL:
    if (!OUTSIDE(ch))
    {
      ch->sendln("Your spell is more draining because you are indoors!");
      GET_MANA(ch) -= level / 2;
      if (GET_MANA(ch) < 0)
        GET_MANA(ch) = 0;
    }
    return spell_bee_sting(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_bee_sting(level, ch, ch, 0, skill);
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in bee sting!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* BEE SWARM */
int cast_bee_swarm(uint8_t level, Character *ch, char *arg, int type,
                   Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!OUTSIDE(ch))
    {
      ch->sendln("Your spell is more draining because you are indoors!");
      GET_MANA(ch) -= level / 2;
      if (GET_MANA(ch) < 0)
        GET_MANA(ch) = 0;
    }
    return spell_bee_swarm(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_bee_swarm(level, ch, ch, 0, skill);
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_bee_swarm(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_bee_swarm(level, ch, ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in bee swarm!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int spell_bee_swarm(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int retval;

  dam = 175;

  act("$n calls upon the insect world!\r\n", ch, 0, 0, TO_ROOM, INVIS_NULL);

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {
    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) &&
        (!ARE_GROUPED(ch, tmp_victim)) && can_be_attacked(ch, tmp_victim))
    {

      set_cantquit(ch, tmp_victim);

      retval = damage(ch, tmp_victim, dam, TYPE_MAGIC, SPELL_BEE_SWARM);
      if (isSet(retval, eCH_DIED))
        return retval;
    }
    else if (DC::getInstance()->world[ch->in_room].zone == DC::getInstance()->world[tmp_victim->in_room].zone)
    {
      tmp_victim->sendln("You hear the buzzing of hundreds of bees.");
    }
  }
  return eSUCCESS;
}

/* CREEPING DEATH */

int cast_creeping_death(uint8_t level, Character *ch, char *arg, int type, Character *victim, class Object *tar_obj, int skill)
{
  int dam;
  int retval;
  affected_type af;
  int bingo = 0, poison = 0;

  set_cantquit(ch, victim);
  dam = 300;

  if (DC::getInstance()->world[ch->in_room].sector_type == SECT_SWAMP)
    dam += 200;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_FOREST)
    dam += 185;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_FIELD)
    dam += 170;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_BEACH)
    dam += 150;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_HILLS)
    dam += 130;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_DESERT)
    dam += 110;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_MOUNTAIN)
    dam += 90;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_PAVED_ROAD)
    dam += 90;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_WATER_NOSWIM)
    dam -= 25;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_AIR)
    dam += 25;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_FROZEN_TUNDRA)
    dam -= 30;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_UNDERWATER)
    dam -= 100;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_ARCTIC)
    dam -= 60;

  if (!OUTSIDE(ch))
  {
    ch->sendln("Your spell is more draning because you are indoors!");
    // If they are NOT outside it costs extra mana
    GET_MANA(ch) -= level / 2;
    if (GET_MANA(ch) < 0)
      GET_MANA(ch) = 0;
  }

  retval = damage(ch, victim, dam, TYPE_PHYSICAL_MAGIC, SPELL_CREEPING_DEATH);
  if (SOMEONE_DIED(retval))
    return retval;

  if (isSet(retval, eEXTRA_VAL2))
    victim = ch;
  if (isSet(retval, eEXTRA_VALUE))
    return retval;

  if (skill > 40 && skill <= 60)
  {
    poison = 2;
  }
  else if (skill > 60 && skill <= 80)
  {
    poison = 3;
  }
  else if (skill > 80)
  {
    bingo = 1;
    poison = 4;
  }

  if (poison > 0)
  {
    if (dice(1, 100) <= poison && !isSet(victim->immune, ISR_POISON))
    {
      af.type = SPELL_POISON;
      af.duration = skill / 27;
      if (IS_NPC(ch))
      {
        af.modifier = -123;
      }
      else
      {
        af.modifier = 0;
        af.origin = ch;
      }

      af.location = APPLY_NONE;
      af.bitvector = AFF_POISON;
      affect_join(victim, &af, false, false);
      victim->sendln("The insect $2poison$R has gotten into your blood!");
      act("$N has been $2poisoned$R by your insect swarm!", ch, 0, victim, TO_CHAR, 0);
    }
  }

  if (bingo > 0)
  {
    if (number(1, 100) <= bingo && !victim->isImmortalPlayer())
    {
      dam = 9999999;
      send_to_char("The insects are crawling in your mouth, out of your eyes, "
                   "through your stomach!\r\n",
                   victim);
      act("$N is completely consumed by insects!", ch, 0, victim, TO_ROOM, NOTVICT);
      act("$N is completely consumed by your insects!", ch, 0, victim, TO_CHAR, 0);
      return damage(ch, victim, dam, TYPE_UNDEFINED, SPELL_CREEPING_DEATH);
    }
  }
  return eSUCCESS;
}

/* BARKSKIN */
int cast_barkskin(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (!victim)
      victim = ch;
    return spell_barkskin(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SPELL:
    if (!OUTSIDE(ch))
    {
      ch->sendln("Your spell is more draining because you are indoors!");
      GET_MANA(ch) -= level / 2;
      if (GET_MANA(ch) < 0)
        GET_MANA(ch) = 0;
    }
    return spell_barkskin(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_barkskin(level, ch, ch, 0, skill);
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in barkskin!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int spell_barkskin(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (victim->affected_by_spell(SPELL_BARKSKIN))
  {
    ch->sendln("You cannot make your skin any stronger!");
    return eFAILURE;
  }

  af.type = SPELL_BARKSKIN;
  af.duration = 5 + level / 7;
  af.modifier = -10 - skill / 5;
  af.location = APPLY_AC;
  af.bitvector = -1;

  affect_join(victim, &af, false, false);

  SET_BIT(victim->resist, ISR_SLASH);

  victim->sendln("Your skin turns stiff and bark-like.");
  act("$N begins to look rather woody.", ch, 0, victim, TO_ROOM, INVIS_NULL | NOTVICT);
  return eSUCCESS;
}

/* HERB LORE */

int cast_herb_lore(uint8_t level, Character *ch, char *arg, int type, Character *victim, class Object *tar_obj, int skill)
{
  int healamount;
  char buf[MAX_STRING_LENGTH * 2], dammsg[MAX_STRING_LENGTH];

  if (GET_RACE(victim) == RACE_UNDEAD)
  {
    ch->sendln("Healing spells are useless on the undead.");
    return eFAILURE;
  }
  if (GET_RACE(victim) == RACE_GOLEM)
  {
    ch->sendln("The heavy magics surrounding this being prevent healing.");
    return eFAILURE;
  }

  if (can_heal(ch, victim, SPELL_HERB_LORE))
  {
    healamount = dam_percent(skill, 250);
    healamount = number(healamount - (healamount / 10), healamount + (healamount / 10));
    if (OUTSIDE(ch))
    {
      victim->addHP(healamount);
    }
    else
    { /* if not outside */
      healamount = dam_percent(skill, 150);
      healamount = number(healamount - (healamount / 10), healamount + (healamount / 10));
      victim->addHP(healamount);
      ch->sendln("Your spell is less effective because you are indoors!");
    }
    if (victim->getHP() >= hit_limit(victim))
    {
      healamount += hit_limit(victim) - victim->getHP();
      victim->fillHPLimit();
    }

    update_pos(victim);

    sprintf(dammsg, " of $B%d$R damage", healamount);

    if (ch != victim)
    {
      sprintf(buf, "Your herbs heal $N%s and makes $m...hungry?", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
      act(buf, ch, 0, victim, TO_CHAR, 0);
      sprintf(buf, "$n's magic herbs heal $N| and make $M look...hungry?");
      send_damage(buf, ch, 0, victim, dammsg, "", TO_ROOM);
      sprintf(dammsg, ", healing you of $B%d$R damage", healamount);
      sprintf(buf, "$n magic herbs make you feel much better%s!", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
      act(buf, ch, 0, victim, TO_VICT, 0);
    }
    else
    {
      sprintf(buf, "Your magic herbs make you feel much better%s!", ch->player ? isSet(ch->player->toggles, Player::PLR_DAMAGE) ? dammsg : "" : "");
      act(buf, ch, 0, 0, TO_CHAR, 0);
      sprintf(buf, "$n magic herbs heal $m| and make $m look...hungry?");
      send_damage(buf, ch, 0, victim, dammsg, "", TO_ROOM);
    }
  }

  char arg1[MAX_INPUT_LENGTH];
  one_argument(arg, arg1);
  if (arg1[0])
  {
    Object *obj = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if (!obj)
    {
      ch->sendln("You don't seem to be carrying any such root.");
      return eFAILURE;
    }
    int virt = DC::getInstance()->obj_index[obj->item_number].virt;
    int aff = 0, spl = 0;
    switch (virt)
    {
    case INVIS_VNUM:
      aff = AFF_INVISIBLE;
      spl = SPELL_INVISIBLE;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n slowly fades out of existence.", victim, 0, 0, TO_ROOM, 0);
      act("You slowly fade out of existence.", victim, 0, 0, TO_CHAR, 0);
      break;
    case HASTE_VNUM:
      aff = AFF_HASTE;
      spl = SPELL_HASTE;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n begins moving faster!", victim, 0, 0, TO_ROOM, 0);
      act("You begin moving faster!", victim, 0, 0, TO_CHAR, 0);
      break;
    case true_VNUM:
      aff = AFF_true_SIGHT;
      spl = SPELL_true_SIGHT;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n's eyes starts to gently glow $Bwhite$R.", victim, 0, 0, TO_ROOM, 0);
      act("Your eyes start to glow $Bwhite$R.", victim, 0, 0, TO_CHAR, 0);
      break;
    case INFRA_VNUM:
      aff = AFF_INFRARED;
      spl = SPELL_INFRAVISION;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n's eyes starts to glow $B$4red$R.", victim, 0, 0, TO_ROOM, 0);
      act("Your eyes start to glow $B$4red$R.", victim, 0, 0, TO_CHAR, 0);
      break;
    case FARSIGHT_VNUM:
      aff = AFF_FARSIGHT;
      spl = SPELL_FARSIGHT;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n's eyes blur and seem to $B$0darken$R.", victim, 0, 0, TO_ROOM, 0);
      act("Your eyes blur and the world around you seems to come closer.", victim, 0, 0, TO_CHAR, 0);
      break;
    case LIGHTNING_SHIELD_VNUM:
      if (find_spell_shield(ch, victim))
      {
        ch->sendln("You cannot do that.");
        return eFAILURE;
      }
      aff = AFF_LIGHTNINGSHIELD;
      spl = SPELL_LIGHTNING_SHIELD;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n is surrounded by $B$5electricity$R.", victim, 0, 0, TO_ROOM, 0);
      act("You become surrounded by $B$5electricity$R.", victim, 0, 0, TO_CHAR, 0);
      break;
    case INSOMNIA_VNUM:
      aff = AFF_INSOMNIA;
      spl = SPELL_INSOMNIA;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n blinks and looks a little twitchy.", victim, 0, 0, TO_ROOM, 0);
      act("You suddenly feel very energetic and not at all sleepy.", victim, 0, 0, TO_CHAR, 0);
      break;
    case DETECT_GOOD_VNUM:
      aff = AFF_DETECT_GOOD;
      spl = SPELL_DETECT_GOOD;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n's eyes starts to gently glow $B$1blue$R.", victim, 0, 0, TO_ROOM, 0);
      act("Your eyes start to glow $B$1blue$R.", victim, 0, 0, TO_CHAR, 0);
      break;
    case DETECT_EVIL_VNUM:
      aff = AFF_DETECT_EVIL;
      spl = SPELL_DETECT_EVIL;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n's eyes starts to gently glow $4deep red$R.", victim, 0, 0, TO_ROOM, 0);
      act("Your eyes start to glow $4deep red$R.", victim, 0, 0, TO_CHAR, 0);
      break;
    case DETECT_INVISIBLE_VNUM:
      aff = AFF_DETECT_INVISIBLE;
      spl = SPELL_DETECT_INVISIBLE;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n's eyes starts to gently glow $B$5yellow$R.", victim, 0, 0, TO_ROOM, 0);
      act("Your eyes start to glow $B$5yellow$R.", victim, 0, 0, TO_CHAR, 0);
      break;
    case SENSE_LIFE_VNUM:
      aff = AFF_SENSE_LIFE;
      spl = SPELL_SENSE_LIFE;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      //		act("$n's eyes starts to gently glow a $2deep green$R.", victim, 0, 0, TO_ROOM, 0);
      act("Your become intensely aware of your surroundings.", victim, 0, 0, TO_CHAR, 0);
      break;
    case SOLIDITY_VNUM:
      aff = AFF_SOLIDITY;
      spl = SPELL_SOLIDITY;
      if (victim->affected_by_spell(spl))
      {
        ch->sendln("They are already affected by that spell.");
        return eFAILURE;
      }
      act("$n is surrounded by a pulsing, $6violet$R aura.", victim, 0, 0, TO_ROOM, 0);
      act("You become surrounded by a pulsing, $6violet$R aura.", victim, 0, 0, TO_CHAR, 0);
      break;
    case 2256:
      aff = 0;
      spl = 0;
      ch->sendln("Adding the herbs improve the healing effect of the spell.");
      victim->addHP(40);
      break;
    default:
      ch->sendln("That's not a herb!");
      return eFAILURE;
    }
    extract_obj(obj);
    struct affected_type af;
    af.type = spl;
    af.duration = 3;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = aff;
    if (aff)
      affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* CALL FOLLOWER */

int cast_call_follower(uint8_t level, Character *ch, char *arg, int type, Character *victim, class Object *tar_obj, int skill)
{
  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, CLAN_ROOM))
  {
    ch->sendln("I don't think your fellow clan members would appreciate the wildlife.");
    GET_MANA(ch) += 75;
    REM_WAIT_STATE(ch, skill / 10);
    return eFAILURE;
  }

  victim = nullptr;

  for (struct follow_type *k = ch->followers; k; k = k->next)
    if (IS_NPC(k->follower) && k->follower->affected_by_spell(SPELL_CHARM_PERSON) &&
        k->follower->in_room != ch->in_room)
    {
      victim = k->follower;
      break;
    }

  if (nullptr == victim)
  {
    ch->sendln("You don't have any tamed friends in need of a summon!");
    REM_WAIT_STATE(ch, skill / 10);
    return eFAILURE;
  }

  act("$n disappears suddenly.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  move_char(victim, ch->in_room);
  act("$n arrives suddenly.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("$n has summoned you!", ch, 0, victim, TO_VICT, 0);
  do_look(victim, "");

  if (!OUTSIDE(ch))
  {
    ch->sendln("Your spell is more draining because you are indoors!");
    // If they are NOT outside it costs extra mana
    GET_MANA(ch) -= level / 2;
    if (GET_MANA(ch) < 0)
      GET_MANA(ch) = 0;
  }

  REM_WAIT_STATE(ch, skill / 10);
  return eSUCCESS;
}

/* ENTANGLE */
int cast_entangle(uint8_t level, Character *ch, char *arg, int type,
                  Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_entangle(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_entangle(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_entangle(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_entangle(level, ch, ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in entangle!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int spell_entangle(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{

  if (!OUTSIDE(ch))
  {
    ch->sendln("You must be outside to cast this spell!");
    return eFAILURE;
  }
  set_cantquit(ch, victim);
  act("Your plants grab $N, attempting to force $M down.", ch, 0,
      victim, TO_CHAR, 0);
  act("$n's plants make an attempt to drag you to the ground!",
      ch, 0, victim, TO_VICT, 0);
  act("$n raises the plants which attack $N!", ch, 0, victim,
      TO_ROOM, NOTVICT);

  if (skill)
  {
    skill = 800 / skill - 1; // get the number for percentage
    if (!number(0, skill))
      spell_blindness(level, ch, victim, 0, 0); /* The plants blind the victim . . */
  }
  if (GET_POS(victim) > position_t::SITTING)
  {
    victim->setSitting(); /* And pull the victim down to the ground */
    if (victim->fighting)
      SET_BIT(victim->combat, COMBAT_BASH2);
  }
  update_pos(victim);
  return eSUCCESS;
}

/* EYES OF THE OWL */
int cast_eyes_of_the_owl(uint8_t level, Character *ch, char *arg, int type,
                         Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!OUTSIDE(ch))
    {
      ch->sendln("Your spell is more draining because you are indoors!");
      GET_MANA(ch) -= level / 2;
      if (GET_MANA(ch) < 0)
        GET_MANA(ch) = 0;
    }
    return spell_eyes_of_the_owl(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_eyes_of_the_owl(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (victim)
      return spell_eyes_of_the_owl(level, ch, victim, 0, skill);
    else if (!tar_obj)
      return spell_eyes_of_the_owl(level, ch, ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in eyes of the owl!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int spell_eyes_of_the_owl(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (victim->affected_by_spell(SPELL_INFRAVISION))
    affect_from_char(victim, SPELL_INFRAVISION);

  if (IS_AFFECTED(victim, AFF_INFRARED))
  {
    GET_MANA(ch) += 5;
    return eFAILURE;
  }

  af.type = SPELL_INFRAVISION;
  af.duration = skill * 2;
  af.modifier = 1 + (skill / 20);
  af.location = APPLY_WIS;
  af.bitvector = AFF_INFRARED;
  affect_join(victim, &af, false, false);
  redo_mana(victim);
  victim->sendln("You feel your vision become much more acute.");
  return eSUCCESS;
}

/* FELINE AGILITY */
int cast_feline_agility(uint8_t level, Character *ch, char *arg, int type,
                        Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (!victim)
      victim = ch;
    return spell_feline_agility(level, ch, victim, 0, skill);
  case SPELL_TYPE_SPELL:
    if (!OUTSIDE(ch))
    {
      ch->sendln("Your spell is more draining because you are indoors!");
      GET_MANA(ch) -= level / 2;
      if (GET_MANA(ch) < 0)
        GET_MANA(ch) = 0;
    }
    return spell_feline_agility(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_feline_agility(level, ch, ch, 0, skill);
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in feline agility!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int spell_feline_agility(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (ch != victim)
  {
    ch->sendln("You can only cast this spell on yourself!");
    return eFAILURE;
  }
  if (victim->affected_by_spell(SPELL_FELINE_AGILITY))
  {
    ch->sendln("You cannot be as agile as TWO cats!");
    return eFAILURE;
  }

  af.type = SPELL_FELINE_AGILITY;
  af.duration = 1 + (level / 4);
  af.modifier = -10 - skill / 6; /* AC bonus */
  af.location = APPLY_AC;
  af.bitvector = -1;
  affect_to_char(victim, &af);
  ch->sendln("Your step lightens as you gain the agility of a cat!");
  af.modifier = 1 + (skill / 20); /* + dex */
  af.location = APPLY_DEX;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* OAKEN FORTITUDE */
int cast_oaken_fortitude(uint8_t level, Character *ch, char *arg, int type,
                         Character *victim, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (!victim)
      victim = ch;
    return spell_oaken_fortitude(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_SPELL:
    if (!OUTSIDE(ch))
    {
      ch->sendln("Your spell is more draining because you are indoors!");
      GET_MANA(ch) -= level / 2;
      if (GET_MANA(ch) < 0)
        GET_MANA(ch) = 0;
    }
    return spell_oaken_fortitude(level, ch, victim, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_oaken_fortitude(level, ch, ch, 0, skill);
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in oaken fortitude!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int spell_oaken_fortitude(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{ // Feline agility rip
  struct affected_type af;

  if (ch != victim)
  {
    ch->sendln("You can only cast this spell on yourself!");
    return eFAILURE;
  }
  if (victim->affected_by_spell(SPELL_OAKEN_FORTITUDE))
  {
    ch->sendln("You cannot enhance your fortitude further.");
    return eFAILURE;
  }

  af.type = SPELL_OAKEN_FORTITUDE;
  af.duration = 1 + (level / 4);
  af.modifier = -9 - skill / 6; /* AC apply */
  af.location = APPLY_AC;
  af.bitvector = -1;
  affect_to_char(victim, &af);
  victim->sendln("Your fortitude increases to that of an oak.");
  af.modifier = 1 + (skill / 20);
  af.location = APPLY_CON;
  affect_to_char(victim, &af);
  redo_hitpoints(victim);
  return eSUCCESS;
}

/* CLARITY */

int cast_clarity(uint8_t level, Character *ch, char *arg, int type, Character *victim, class Object *tar_obj, int skill)
{ // Feline agility rip
  struct affected_type af;

  if (victim->affected_by_spell(SPELL_CLARITY))
  {
    ch->sendln("You cannot clear your mind any further without going stupid.");
    return eFAILURE;
  }

  af.type = SPELL_CLARITY;
  af.duration = 3 + skill / 6;
  af.modifier = 3 + skill / 20;
  af.location = APPLY_INT;
  af.bitvector = -1;
  affect_to_char(victim, &af);
  redo_mana(victim);
  victim->sendln("You suddenly feel smarter than everyone else.");
  act("$n's eyes shine with powerful intellect!", victim, 0, 0, TO_ROOM, 0);
  af.modifier = 3 + (skill / 20);
  af.location = APPLY_MANA_REGEN;
  affect_to_char(victim, &af);
  return eSUCCESS;
}

/* FOREST MELD */

int cast_forest_meld(uint8_t level, Character *ch, char *arg, int type, Character *victim, class Object *tar_obj, int skill)
{
  if (!(DC::getInstance()->world[ch->in_room].sector_type == SECT_FOREST || DC::getInstance()->world[ch->in_room].sector_type == SECT_SWAMP))
  {
    ch->sendln("You are not in a forest!!");
    return eFAILURE;
  }
  //	if(victim != ch)
  //	{
  //		send_to_char("Why would the forest like anyone but
  // you?\r\n", ch);
  //		return eFAILURE;
  //	}
  if (ch->isPlayerObjectThief() || ch->isPlayerGoldThief())
  {
    ch->sendln("The forests reject your naughty, thieving self.");
    return eFAILURE;
  }
  act("$n melts into the forest and is gone.", ch, 0, 0,
      TO_ROOM, INVIS_NULL);
  ch->sendln("You feel yourself slowly become a temporary part of the living forest.");
  struct affected_type af;
  int skil = ch->has_skill(SPELL_FOREST_MELD);
  af.type = SPELL_FOREST_MELD;
  af.duration = 2 + (skil > 40) + (skil > 60) + (skil > 80);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_FOREST_MELD;
  affect_to_char(ch, &af);

  // 	SETBIT(ch->affected_by, AFF_FOREST_MELD);
  SETBIT(ch->affected_by, AFF_HIDE);
  return eSUCCESS;
}

/* COMPANION (disabled) */

int cast_companion(uint8_t level, Character *ch, char *arg, int type, Character *victim, class Object *tar_obj, int skill)
{
  Character *mob;
  struct affected_type af;
  char name[MAX_STRING_LENGTH];
  char desc[MAX_STRING_LENGTH];
  int number = 19309; // Mob number

  /* remove this whenever someone actually fixes this spell -pir */
  ch->sendln("Spell not finished.");
  return eFAILURE;

  if (!OUTSIDE(ch))
  {
    ch->sendln("You cannot use such powerful magic indoors!");
    return eFAILURE;
  }

  // Load up the standard fire ruler from elemental canyon (mob #19309) */
  mob = ch->getDC()->clone_mobile(number);
  char_to_room(mob, ch->in_room);

  // I am not sure of a better way to do this, so I just set the duration
  // to roughly 24 hours.  Odds are the mud will crash or be rebooted
  // before the 24 hours elapse anyway.

  af.type = SPELL_CHARM_PERSON;
  af.duration = 960;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_CHARM;
  add_follower(mob, ch);
  affect_join(mob, &af, false, false);

  // The mob should have zero xp
  GET_EXP(mob) = 0;
  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  switch (dice(1, 4))
  {
  case 1:
    sprintf(desc, "%s's very powerful fire elemental", GET_NAME(ch));
    sprintf(name, "fire ");
    mob->max_hit += 300;
    if (dice(1, 100) == 1) // they got the .25% chance - very lucky
      SETBIT(mob->affected_by, AFF_FIRESHIELD);
    SET_BIT(mob->resist, ISR_FIRE);
    break;
  case 2:
    sprintf(desc, "%s's very powerful air elemental", GET_NAME(ch));
    sprintf(name, "air ");
    mob->max_hit += 200;
    SET_BIT(mob->resist, ISR_SLASH);
    break;
  case 3:
    sprintf(desc, "%s's very powerful water elemental", GET_NAME(ch));
    sprintf(name, "water ");
    mob->max_hit += 200;
    SET_BIT(mob->resist, ISR_HIT);
    break;
  case 4:
    sprintf(desc, "%s's very powerful earth elemental", GET_NAME(ch));
    sprintf(name, "earth ");
    mob->max_hit += 500;
    SET_BIT(mob->resist, ISR_PIERCE);
    break;
  default:
    sprintf(desc, "%s's very powerful GAME MESSUP", GET_NAME(ch));
    mob->max_hit += 1000;
    break;
  } // of switch

  mob->hit = mob->max_hit; // Set elem to full hps
  strcat(name, "elemental companion");
  mob->setName(name);
  mob->short_desc = str_hsh(desc);
  mob->long_desc = str_hsh(desc);

  // Set mob level - within 5 levels of the character
  if (dice(1, 2) - 1)
  {
    mob->decrementLevel(dice(1, 5));
  }
  else
  {
    mob->incrementLevel(dice(1, 5));
  }

  // Now set the AFF_CREATOR flag on the character for two hours
  af.type = 0;
  af.duration = 80; // Roughly 2 hours
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;
  affect_join(ch, &af, false, false);
  return eSUCCESS;
}

// Procedure for checking for/destroying spell components
// Expects SPELL_xxx, the caster, and boolean as to destroy the components
// Returns true for success and false for failure

int check_components(Character *ch, int destroy, int item_one = 0,
                     int item_two = 0, int item_three = 0, int item_four = 0, bool silent = false)
{
  // We're going to assume you never have more than 4 items
  // for a spell, though you can easily change to take more
  int all_ok = 0;
  Object *ptr_one, *ptr_two, *ptr_three, *ptr_four;

  ptr_one = ptr_two = ptr_three = ptr_four = nullptr;

  if (!ch)
  {
    DC::getInstance()->logentry(QStringLiteral("No ch sent to check spell components"), ANGEL, DC::LogChannel::LOG_BUG);
    return false;
  }

  if (!ch->carrying)
    return false;

  ptr_one = get_obj_in_list_num(real_object(item_one), ch->carrying);
  if (item_two)
    ptr_two = get_obj_in_list_num(real_object(item_two), ch->carrying);
  if (item_three)
    ptr_three = get_obj_in_list_num(real_object(item_three), ch->carrying);
  if (item_four)
    ptr_four = get_obj_in_list_num(real_object(item_four), ch->carrying);

  // Destroy the components if needed

  if (destroy)
  {
    int gone = false;
    if (ptr_one)
    {
      obj_from_char(ptr_one);
      extract_obj(ptr_one);
      gone = true;
    }
    if (ptr_two)
    {
      obj_from_char(ptr_two);
      extract_obj(ptr_two);
      gone = true;
    }
    if (ptr_three)
    {
      obj_from_char(ptr_three);
      extract_obj(ptr_three);
      gone = true;
    }
    if (ptr_four)
    {
      obj_from_char(ptr_four);
      extract_obj(ptr_four);
      gone = true;
    }
    if (gone && !silent)
      ch->sendln("The spell components poof into smoke.");
  }

  // Make sure we found everything before saying its OK

  all_ok = ((item_one != 0) && (ptr_one != 0));

  if (all_ok && item_one)
    all_ok = (int64_t)ptr_one;
  if (all_ok && item_two)
    all_ok = (int64_t)ptr_two;
  if (all_ok && item_three)
    all_ok = (int64_t)ptr_three;
  if (all_ok && item_four)
    all_ok = (int64_t)ptr_four;

  if (ch->getLevel() > ARCHANGEL && !all_ok && !silent)
  {
    ch->sendln("You didn't have the right components, but yer a god:)");
    return true;
  }

  return all_ok;
}

/* CREATE GOLEM */

int spell_create_golem(int level, Character *ch, Character *victim, class Object *obj, int skill)
{
  char buf[200];
  Character *mob;
  follow_type *k;

  struct affected_type af;

  ch->sendln("Disabled currently,");

  // make sure its a charmie
  if ((IS_PC(victim)) || (victim->master != ch))
  {
    ch->sendln("You can only do this to someone under your mental control.");
    GET_MANA(ch) += 50;
    return eFAILURE;
  }

  // make sure it isn't already a golem

  if (isexact("golem", qPrintable(victim->getName())))
  {
    ch->sendln("Isn't that already a golem?");
    GET_MANA(ch) += 50;
    return eFAILURE;
  }

  // check_special_room !
  if (!check_components(ch, true, 14905, 2256, 3181))
  {
    ch->sendln("Without the proper spell components, your spell fizzles out and dies.");
    act("$n's hands pop and sizzle with misused spell components.", ch, 0, 0, TO_ROOM, 0);
    return eFAILURE;
  }

  // take away 500 mana max (min is 50)
  GET_MANA(ch) -= 450;
  if (GET_MANA(ch) < 0)
    GET_MANA(ch) = 0;

  // give spiffy messages

  if (IS_EVIL(ch))
    act("$n seizes $N, strapping its unresisting form to the\r\n"
        "center of a ceremonial pentagram.  A few quiet words are uttered\r\n"
        "and $n thrusts $s finger into the heart of the captive\r\n"
        "drawing out its magical energies.  Its organs and limbs and flesh\r\n"
        "are divided into piles then reformed into the shape of a humanoid\r\n"
        "golem, obediant to its master's will.",
        ch, 0, victim, TO_ROOM, 0);
  else if (IS_GOOD(ch))
    act("$n lulls $N into a magical sleep, stopping\r\n"
        "its heart with a gentle tap of a finger to its chest.  A magical\r\n"
        "aura hides it from sight as its body reshapes and reforms,\r\n"
        "crafted by dweomers into the powerful shape of a golem,\r\n"
        "obediant only to $n.",
        ch, 0, victim, TO_ROOM, 0);
  else
    act("$n begins an incantation paralyzing $N\r\n"
        "in place.  Large mana bubbles form in the air swirling around\r\n"
        "$N in a parody of dance then cling onto it coating\r\n"
        "the area in a extra-planar light.  Minutes pass till the light\r\n"
        "fades revealing $n's new golem servant.",
        ch, 0, victim, TO_ROOM, 0);

  ch->sendln("Delving deep within yourself, you stretch your abilities to the very limit in an effort to create magical life!");

  // create golem

  mob = ch->getDC()->clone_mobile(real_mobile(201));
  if (!mob)
  {
    ch->sendln("Warning: Load mob not found in create_corpse");
    return eFAILURE;
  }
  char_to_room(mob, ch->in_room);
  af.type = SPELL_CHARM_PERSON;
  af.duration = -1;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_CHARM;
  affect_to_char(mob, &af);

  GET_EXP(mob) = 0;
  mob->setName("golem");
  sprintf(buf, "The golem seems to be a mishmash of other creatures binded by magic.\r\nIt appears to have pieces of %s in it.\r\n",
          GET_SHORT(victim));
  mob->description = str_hsh(buf);
  GET_SHORT_ONLY(mob) = str_hsh("A gruesome golem");
  mob->long_desc = str_hsh("A golem created of twisted magic, stands here motionless.\r\n");
  REMBIT(mob->mobdata->actflags, ACT_SCAVENGER);
  REMOVE_BIT(mob->immune, ISR_PIERCE);
  REMOVE_BIT(mob->immune, ISR_SLASH);

  // kill charmie(s) and give stats to golem

  if (GET_MAX_HIT(victim) * 2 < 32000)
  {
    mob->max_hit = GET_MAX_HIT(victim) * 2;
  }
  else
  {
    mob->max_hit = 32000;
  }
  mob->fillHP(); /* 50% of max */
  GET_AC(mob) = GET_AC(victim) - 30;
  mob->mobdata->damnodice = victim->mobdata->damnodice + 5;
  mob->mobdata->damsizedice = victim->mobdata->damsizedice;
  mob->setLevel(50);
  GET_HITROLL(mob) = GET_HITROLL(victim) + number<decltype(ch->intel)>(1, GET_INT(ch));
  GET_DAMROLL(mob) = GET_DAMROLL(victim) + number<decltype(ch->wis)>(1, GET_WIS(ch));
  mob->setStanding();
  GET_ALIGNMENT(mob) = GET_ALIGNMENT(ch);
  SETBIT(mob->affected_by, AFF_GOLEM);
  // kill um all!
  k = ch->followers;
  while (k)
    if (IS_AFFECTED(k->follower, AFF_CHARM))
    {
      fight_kill(ch, k->follower, TYPE_RAW_KILL, 0);
      k = ch->followers;
    }
    else
      k = k->next;

  add_follower(mob, ch);

  // add random abilities
  if (number(1, 3) > 1)
  {
    SETBIT(mob->mobdata->actflags, ACT_2ND_ATTACK);
    if (number(1, 2) == 1)
      SETBIT(mob->mobdata->actflags, ACT_3RD_ATTACK);
  }

  if (number(1, 15) == 15)
    SETBIT(mob->affected_by, AFF_SANCTUARY);

  if (number(1, 20) == 20)
    SETBIT(mob->affected_by, AFF_FIRESHIELD);

  if (number(1, 2) == 2)
    SETBIT(mob->affected_by, AFF_DETECT_INVISIBLE);

  if (number(1, 7) == 7)
    SETBIT(mob->affected_by, AFF_FLYING);

  if (number(1, 10) == 10)
    SETBIT(mob->affected_by, AFF_SNEAK);

  if (number(1, 2) == 1)
    SETBIT(mob->affected_by, AFF_INFRARED);

  if (number(1, 50) == 1)
    SETBIT(mob->affected_by, AFF_EAS);

  if (number(1, 3) == 1)
    SETBIT(mob->affected_by, AFF_true_SIGHT);

  // lag mage
  if (number(1, 3) == 3 && ch->getLevel() < ARCHANGEL)
  {
    act("$n falls to the ground, unable to move while $s body recovers from such an incredible and draining magical feat.",
        ch, 0, 0, TO_ROOM, 0);
    ch->sendln("You drop, drained by the release of such power.");
    ch->setResting();

    // why won't this line work?
    //  WAIT_STATE(ch, (DC::PULSE_VIOLENCE * number(10, 15)));
  }
  return eSUCCESS;
}

/* OLD CREATE/RELEASE GOLEM (unused) */

/*
int cast_create_golem( uint8_t level, Character *ch, char *arg, int type,
  Character *tar_ch, class Object *tar_obj, int skill )
{
  switch (type) {
    case SPELL_TYPE_SPELL:
      return spell_create_golem(level, ch, tar_ch, 0, skill);
      break;
    default :
      DC::getInstance()->logentry(QStringLiteral("Serious screw-up in create golem!"), ANGEL, DC::LogChannel::LOG_BUG);
      break;
  }
  return eFAILURE;
}*/

/*
int spell_release_golem(uint8_t level, Character *ch, char *arg, int type, Character *victim, class Object * tar_obj, int skill)
{
   // Character *tmp_vict;
   struct follow_type * temp;
   int done = 0;

   temp = ch->followers;

   if(!temp) {
      ch->sendln("You have no golem!");
      return eFAILURE;
   }

   while(temp && !done) {
      if(ISSET(temp->follower->affected_by, AFF_GOLEM))
         done = 1;
      else temp = temp->next;
   }

   if(!temp) {
      ch->sendln("You have no golem!");
      return eFAILURE;
   }

   fight_kill(ch, temp->follower, TYPE_RAW_KILL, 0);
   return eSUCCESS;
}
*/

/* BEACON */

int spell_beacon(uint8_t level, Character *ch, char *arg, int type, Character *victim, class Object *tar_obj, int skill)
{
  //   int to_room = 0;

  if (!ch->beacon)
  {
    ch->sendln("You have no beacon set!");
    return eFAILURE;
  }

  if (ch->isPlayerObjectThief())
  {
    ch->sendln("Your attempt to transport stolen goods through the astral planes fails!!");
    return eFAILURE;
  }

  if (ch->beacon->in_room < 1)
  {
    ch->sendln("Your magical beacon has been lost!");
    return eFAILURE;
  }

  if (ch->beacon->in_room == ch->in_room)
  {
    ch->sendln("Hey genius.  Poof.  You're already there.");
    return eFAILURE;
  }

  if ((!ch->room().isArena() &&
       ch->beacon->room().isArena()) ||
      (ch->room().isArena() &&
       !ch->beacon->room().isArena()))
  {
    ch->sendln("Your beacon cannot take you into or out of the arena!");
    return eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_CHAMPION))
  {
    if (ch->beacon->in_room >= 1900 && ch->beacon->in_room <= 1999)
    {
      ch->sendln("You cannot beacon into a guild whilst Champion.");
      return eFAILURE;
    }

    if (isSet(DC::getInstance()->world[ch->beacon->in_room].room_flags, CLAN_ROOM))
    {
      ch->sendln("You cannot beacon into a clan hall whilst Champion.");
      return eFAILURE;
    }
  }

  if (DC::getInstance()->zones.value(DC::getInstance()->world[ch->in_room].zone).continent != DC::getInstance()->zones.value(DC::getInstance()->world[ch->beacon->in_room].zone).continent)
  {
    if (GET_MANA(ch) < use_mana(ch, skill))
    {
      ch->sendln("You don't posses the energy to travel that far.");
      GET_MANA(ch) += use_mana(ch, skill);
      return eFAILURE;
    }
    else
    {
      ch->sendln("The long distance drains additional mana from you.");
      GET_MANA(ch) -= use_mana(ch, skill);
    }
  }

  if (others_clan_room(ch, &DC::getInstance()->world[ch->beacon->in_room]) == true)
  {
    ch->sendln("You cannot beacon into another clan's hall.");
    ch->beacon->equipped_by = nullptr;
    extract_obj(ch->beacon);
    ch->beacon = nullptr;
    return eFAILURE;
  }

  if (ch->fighting && (0 == number(0, 20)))
  {
    ch->sendln("In the heat of combat, you forget your beacon's location!");
    act("$n's eyes widen for a moment, $s concentration broken.", ch, 0, 0, TO_ROOM, 0);
    ch->beacon->equipped_by = nullptr;
    extract_obj(ch->beacon);
    ch->beacon = nullptr;
    return eFAILURE;
  }

  act("Poof! $e's gone!", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("You rip a dimensional hole through space and step out at your beacon.");

  if (!move_char(ch, ch->beacon->in_room))
  {
    ch->sendln("Failure in move_char.  Major fuckup.  Contact a god.");
    return eFAILURE;
  }
  do_look(ch, "");

  act("$n steps out from a dimensional rip.", ch, 0, 0, TO_ROOM, 0);
  return eSUCCESS;
}

int do_beacon(Character *ch, char *argument, cmd_t cmd)
{
  class Object *new_obj = nullptr;
  if (IS_NPC(ch))
    return eFAILURE;
  if (GET_CLASS(ch) != CLASS_ANTI_PAL && ch->getLevel() < ARCHANGEL)
  {
    ch->sendln("Sorry, but you cannot do that here!");
    return eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_CHAMPION) && ch->in_room >= 1900 && ch->in_room <= 1999)
  {
    ch->sendln("You cannot set a beacon in a guild whilst Champion.");
    return eFAILURE;
  }

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) || isSet(DC::getInstance()->world[ch->in_room].room_flags, NOLEARN))
  {
    ch->sendln("You may not place your beacon in an area protected by the gods.");
    return eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_CHAMPION) && isSet(DC::getInstance()->world[ch->in_room].room_flags, CLAN_ROOM))
  {
    ch->sendln("You cannot set a beacon in a clan hall whilst Champion.");
    return eFAILURE;
  }

  if (others_clan_room(ch, &DC::getInstance()->world[ch->in_room]) == true)
  {
    ch->sendln("You cannot set a beacon in another clan's hall.");
    return eFAILURE;
  }

  ch->sendln("You set a magical beacon in the air.");
  if (!ch->beacon)
  {
    if (!(new_obj = clone_object(real_object(BEACON_OBJ_NUMBER))))
    {
      ch->sendln("Error setting beacon.  Contact a god.");
      return eFAILURE;
    }
  }
  else
  {
    obj_from_room(ch->beacon);
    new_obj = ch->beacon;
  }

  new_obj->equipped_by = ch;
  obj_to_room(new_obj, ch->in_room);
  ch->beacon = new_obj;

  //   ch->beacon = DC::getInstance()->world[ch->in_room].number;
  return eSUCCESS;
}

/* REFLECT (non-castable atm) */

int spell_reflect(uint8_t level, Character *ch, char *arg, int type, Character *victim, class Object *tar_obj, int skill)
{
  ch->sendln("Tell an immortal what you just did.");
  return eFAILURE;
}

/* CALL FAMILIAR (SUMMON FAMILIAR) */

#define FAMILIAR_MOB_IMP 5
#define FAMILIAR_MOB_CHIPMUNK 6
#define FAMILIAR_MOB_GREMLIN 4
#define FAMILIAR_MOB_OWL 7

int choose_druid_familiar(Character *ch, char *arg)
{
  char buf[MAX_INPUT_LENGTH];

  one_argument(arg, buf);

  if (!strcmp(buf, "chipmunk"))
  {
    if (!check_components(ch, true, 27800))
    {
      ch->sendln("You remember at the last second that you don't have an acorn ready!");
      act("$n's hands pop with unused mystical energy and $e seems confused.", ch, 0, 0, TO_ROOM, 0);
      return -1;
    }
    return FAMILIAR_MOB_CHIPMUNK;
  }
  if (!strcmp(buf, "owl"))
  {
    if (!check_components(ch, true, 44))
    {
      ch->sendln("You remember at the last second that you don't have a dead mouse ready!");
      act("$n's hands pop with unused mystical energy and $e seems confused.", ch, 0, 0, TO_ROOM, 0);
      return -1;
    }
    return FAMILIAR_MOB_OWL;
  }
  send_to_char("You must specify the type of familiar you wish to summon.\r\n"
               "You currently may summon:  chipmunk or owl\r\n",
               ch);
  return -1;
}

int choose_mage_familiar(Character *ch, char *arg)
{
  char buf[MAX_INPUT_LENGTH];

  one_argument(arg, buf);

  if (!strcmp(buf, "imp"))
  {
    if (!check_components(ch, true, 4))
    {
      ch->sendln("You remember at the last second that you don't have a batwing ready!");
      act("$n's hands pop with unused mystical energy and $e seems confused.", ch, 0, 0, TO_ROOM, 0);
      return -1;
    }
    return FAMILIAR_MOB_IMP;
  }
  if (!strcmp(buf, "gremlin"))
  {
    if (!check_components(ch, true, 43))
    {
      ch->sendln("You remember at the last second that you don't have a silver piece ready!");
      act("$n's hands pop with unused mystical energy and $e seems confused.", ch, 0, 0, TO_ROOM, 0);
      return -1;
    }
    return FAMILIAR_MOB_GREMLIN;
  }
  send_to_char("You must specify the type of familiar you wish to summon.\r\n"
               "You currently may summon:  imp or gremlin\r\n",
               ch);
  return -1;
}

int choose_familiar(Character *ch, char *arg)
{
  if (GET_CLASS(ch) == CLASS_DRUID)
    return choose_druid_familiar(ch, arg);
  if (GET_CLASS(ch) == CLASS_MAGIC_USER)
    return choose_mage_familiar(ch, arg);
  return -1;
}

void familiar_creation_message(Character *ch, int fam_type)
{
  switch (fam_type)
  {
  case FAMILIAR_MOB_IMP:
    act("$n throws a batwing into the air which explodes into flame.\r\n"
        "A small imp appears from the smoke and perches on $n's shoulder.",
        ch, 0, 0, TO_ROOM, 0);
    send_to_char("You channel a miniature $B$4fireball$R into the wing and throw it into the air.\r\n"
                 "A small imp appears from the $B$4flames$R and perches upon your shoulder.\r\n",
                 ch);
    break;
  case FAMILIAR_MOB_CHIPMUNK:
    act("$n coaxs a chipmunk from nowhere and gives it an acorn to eat.\r\n"
        "A small chipmunk eats the acorn and looks at $n lovingly.",
        ch, 0, 0, TO_ROOM, 0);
    send_to_char("You whistle a little tune summoning a chipmunk to you and give it an acorn.\r\n"
                 "The small chipmunk eats the acorn and looks at you adoringly.\r\n",
                 ch);
    break;
  case FAMILIAR_MOB_OWL:
    act("$n throws a dead mouse skyward and a large owl swoops down and grabs it in midair.\r\n", ch, 0, 0, TO_ROOM, 0);
    ch->sendln("You toss the mouse into the air and grin as an owl swoops down from above.\r\nA voice echoes in your mind...'I can help you to $4watch$7 for nearby enemies.'");
    break;
  case FAMILIAR_MOB_GREMLIN:
    act("$n opens a small portal and throws in a piece of silver.\r\nA confused gremlin is launched out of the portal, hitting its head as it lands on the floor.", ch, 0, 0, TO_ROOM, 0);
    ch->sendln("You open a portal to your target and throw in your silver piece.\r\nThe gremlin is expunged from the dimension beyond through your portal, hitting its head in the process.");
    break;
  default:
    ch->sendln("Illegal message in familar_creation_message.  Tell pir.");
    break;
  }
}

int spell_summon_familiar(uint8_t level, Character *ch, char *arg, int type, Character *victim, class Object *tar_obj, int skill)
{
  Character *mob = nullptr;
  int r_num;
  struct affected_type af;
  follow_type *k = nullptr;
  int fam_type;

  fam_type = choose_familiar(ch, arg);
  if (-1 == fam_type)
    return eFAILURE;

  if ((r_num = real_mobile(fam_type)) < 0)
  {
    ch->sendln("Summon familiar mob not found.  Tell a god.");
    return eFAILURE;
  }

  k = ch->followers;
  while (k)
    if (IS_AFFECTED(k->follower, AFF_FAMILIAR))
    {
      ch->sendln("But you already have a devoted pet!");
      return eFAILURE;
    }
    else
      k = k->next;

  mob = ch->getDC()->clone_mobile(r_num);
  char_to_room(mob, ch->in_room);

  SETBIT(mob->affected_by, AFF_FAMILIAR);

  af.type = SPELL_SUMMON_FAMILIAR;
  af.duration = -1;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;
  affect_to_char(mob, &af);

  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  familiar_creation_message(ch, fam_type);
  add_follower(mob, ch);

  return eSUCCESS;
}

int cast_summon_familiar(uint8_t level, Character *ch, char *arg, int type,
                         Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_summon_familiar(level, ch, arg, SPELL_TYPE_SPELL, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in summon_familiar(potion)!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
    break;
  case SPELL_TYPE_SCROLL:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in summon_familiar(potion)!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
    break;
  case SPELL_TYPE_WAND:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in summon_familiar(potion)!"), ANGEL, DC::LogChannel::LOG_BUG);
    return eFAILURE;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in summon_familiar!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* LIGHTED PATH */

int spell_lighted_path(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  struct room_track_data *ptrack;
  char buf[180];

  ptrack = DC::getInstance()->world[ch->in_room].tracks;

  if (!ptrack)
  {
    ch->sendln("You detect no scents in this room.");
    return eSUCCESS;
  }

  send_to_char("Your magic pulls the essence of scent from around you straining\r\n"
               "knowledge from it to be displayed briefly in your mind.\r\n"
               "$B$3You sense...$R\r\n",
               ch);

  while (ptrack)
  {
    sprintf(buf, "A %s called %s headed %s...\r\n",
            races[ptrack->race].singular_name,
            qPrintable(ptrack->trackee),
            dirs[ptrack->direction]);
    ch->send(buf);
    ptrack = ptrack->next;
  }

  return eSUCCESS;
}

int cast_lighted_path(uint8_t level, Character *ch, char *arg, int type,
                      Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!OUTSIDE(ch))
    {
      ch->sendln("Your spell is more draining because you are indoors!");
      GET_MANA(ch) -= level / 2;
      if (GET_MANA(ch) < 0)
        GET_MANA(ch) = 0;
    }
    return spell_lighted_path(level, ch, "", SPELL_TYPE_SPELL, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_lighted_path(level, ch, "", SPELL_TYPE_SPELL, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    return spell_lighted_path(level, ch, "", SPELL_TYPE_SPELL, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    return spell_lighted_path(level, ch, "", SPELL_TYPE_SPELL, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    return spell_lighted_path(level, ch, "", SPELL_TYPE_SPELL, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in lighted_path! (unknown)"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* RESIST ACID */

int spell_resist_acid(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  if (GET_CLASS(ch) == CLASS_ANTI_PAL && ch != victim)
  {
    ch->sendln("You can only cast this on yourself.");
    return eFAILURE;
  }

  if (victim->affected_by_spell(SPELL_RESIST_ACID))
  {
    act("$N is already resistant to acid.", ch, 0, victim, TO_CHAR, 0);
    return eFAILURE;
  }
  act("$n's skin turns $2green$R momentarily.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("Your skin turns $2green$R momentarily.", victim, 0, 0, TO_CHAR, 0);

  af.type = SPELL_RESIST_ACID;
  af.duration = 1 + skill / 10;
  af.modifier = 10 + skill / 6;
  af.location = APPLY_SAVING_ACID;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

int cast_resist_acid(uint8_t level, Character *ch, char *arg, int type,
                     Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_resist_acid(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_resist_acid(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    return spell_resist_acid(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_resist_acid(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in resist acid!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* SUN RAY */

int spell_sun_ray(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  extern struct weather_data weather_info;

  set_cantquit(ch, victim);

  dam = MIN((int)GET_MANA(ch), 725);

  if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY) && (weather_info.sunlight > SUN_DARK))
  {

    //	 if(saves_spell(ch, victim, 0, SAVE_TYPE_ENERGY) >= 0)
    //		dam >>= 1;

    return damage(ch, victim, dam, TYPE_ENERGY, SPELL_SUN_RAY);
  }
  else
    act("The sun ray cannot reach $N!", ch, 0, victim, TO_CHAR, 0);

  return eFAILURE;
}

int cast_sun_ray(uint8_t level, Character *ch, char *arg, int type,
                 Character *victim, class Object *tar_obj, int skill)
{
  extern struct weather_data weather_info;
  int retval;
  Character *next_v;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY))
      return spell_sun_ray(level, ch, victim, 0, skill);
    else
      ch->sendln("You must be outdoors on a day that isn't raining!");
    break;
  case SPELL_TYPE_POTION:
    if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY))
      return spell_sun_ray(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY))
    {
      if (victim)
        return spell_sun_ray(level, ch, victim, 0, skill);
      else if (!tar_obj)
        spell_sun_ray(level, ch, ch, 0, skill);
    }
    break;
  case SPELL_TYPE_STAFF:
    if (OUTSIDE(ch) && (weather_info.sky <= SKY_CLOUDY))
    {
      for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_v)
      {
        next_v = victim->next_in_room;

        if (!ARE_GROUPED(ch, victim))
        {
          retval = spell_sun_ray(level, ch, victim, 0, skill);
          if (isSet(retval, eCH_DIED))
            return retval;
        }
      }
      return eSUCCESS;
      break;
    }
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in sun ray!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* RAPID MEND */

int spell_rapid_mend(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  int regen = 0, duration = 0;

  if (!victim->affected_by_spell(SPELL_RAPID_MEND))
  {
    act("$n starts to heal more quickly.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    victim->sendln("You feel your body begin to heal more quickly.");

    if (skill < 40)
    {
      duration = 3;
      regen = 4;
    }
    else if (skill > 40 && skill <= 60)
    {
      duration = 3;
      regen = 6;
    }
    else if (skill > 60 && skill <= 80)
    {
      duration = 4;
      regen = 8;
    }
    else if (skill > 80 && skill <= 90)
    {
      duration = 5;
      regen = 10;
    }
    else if (skill > 90)
    {
      duration = 6;
      regen = 12;
    }

    af.type = SPELL_RAPID_MEND;
    af.duration = duration;
    af.modifier = regen;
    af.location = APPLY_HP_REGEN;
    af.bitvector = -1;
    affect_to_char(victim, &af);
  }
  else
    act("$n is already mending quickly.", victim, 0, 0, TO_CHAR, 0);

  return eSUCCESS;
}

int cast_rapid_mend(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_rapid_mend(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_rapid_mend(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_rapid_mend(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in rapid_mend!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* IRON ROOTS */

int spell_iron_roots(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (ch->affected_by_spell(SPELL_IRON_ROOTS))
  {
    affect_from_char(ch, SPELL_IRON_ROOTS);
    act("The tree roots encasing $n's legs sink away under the surface of the ground.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    act("The roots release their hold upon you and melt away beneath the surface of the ground.", ch, 0, 0, TO_CHAR, 0);
    REMBIT(ch->affected_by, AFF_NO_FLEE);
  }
  else
  {
    act("Tree roots spring from the ground bracing $n's legs, holding $m down firmly to the ground.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    act("Tree roots spring from the ground firmly holding you to the ground.", ch, 0, 0, TO_CHAR, 0);

    SETBIT(ch->affected_by, AFF_NO_FLEE);

    af.type = SPELL_IRON_ROOTS;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(ch, &af);
  }
  return eSUCCESS;
}

/* IRON ROOTS (potion, scroll, staff, wand) */

int cast_iron_roots(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (!OUTSIDE(ch))
    {
      ch->sendln("Your spell is more draining because you are indoors!");
      GET_MANA(ch) -= level / 2;
      if (GET_MANA(ch) < 0)
        GET_MANA(ch) = 0;
    }
    return spell_iron_roots(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_iron_roots(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_iron_roots(level, tar_ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in iron_roots!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* ACID SHIELD */

int spell_acid_shield(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (find_spell_shield(ch, victim) && IS_PC(victim))
    return eFAILURE;

  if (!victim->affected_by_spell(SPELL_ACID_SHIELD))
  {
    act("$n is surrounded by a gaseous shield of $B$2acid$R.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    act("You are surrounded by a gaseous shield of $B$2acid$R.", victim, 0, 0, TO_CHAR, 0);

    af.type = SPELL_ACID_SHIELD;
    af.duration = 2 + (skill / 23);
    af.modifier = skill;
    af.location = APPLY_NONE;
    af.bitvector = AFF_ACID_SHIELD;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* ACID SHIELD (potion, scroll, wand, staves) */

int cast_acid_shield(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (GET_CLASS(ch) == CLASS_ANTI_PAL)
    {
      if (GET_ALIGNMENT(ch) > -351)
      {
        ch->sendln("You are not evil enough to cast this spell!");
        return eFAILURE;
      }
    }
    return spell_acid_shield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_acid_shield(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_acid_shield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
      spell_acid_shield(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in acid shield!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* WATER BREATHING */

int spell_water_breathing(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  struct affected_type *cur_af;

  if ((cur_af = victim->affected_by_spell(SPELL_WATER_BREATHING)))
    affect_remove(victim, cur_af, SUPPRESS_ALL);

  act("Small gills spring forth from $n's neck and begin fanning as $e breathes.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("Your neck springs gills and the air around you suddenly seems very dry.", victim, 0, 0, TO_CHAR, 0);

  af.type = SPELL_WATER_BREATHING;
  af.duration = 6 + (skill / 5);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* WATERBREATHING (potions, scrolls, staves, wands) */

int cast_water_breathing(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_water_breathing(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_water_breathing(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_water_breathing(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
      spell_water_breathing(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in water breathing!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* GLOBE OF DARKNESS */

int spell_globe_of_darkness(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  Object *globe;
  int dur = 0, mod = 0;

  if (skill <= 20)
  {
    dur = 2;
    mod = 10;
  }
  else if (skill > 20 && skill <= 40)
  {
    dur = 3;
    mod = 15;
  }
  else if (skill > 40 && skill <= 60)
  {
    dur = 3;
    mod = 15;
  }
  else if (skill > 60 && skill <= 80)
  {
    dur = 4;
    mod = 20;
  }
  else if (skill > 80 && skill <= 90)
  {
    dur = 4;
    mod = 20;
  }
  else if (skill > 90)
  {
    dur = 5;
    mod = 25;
  }
  if (skill == 150)
  {
    dur = 2;
    mod = 15;
  }
  globe = clone_object(real_object(GLOBE_OF_DARKNESS_OBJECT));

  if (!globe)
  {
    ch->sendln("Major screwup in globe of darkness.  Missing util obj.  Tell coder.");
    return eFAILURE;
  }

  globe->obj_flags.value[0] = dur;
  globe->obj_flags.value[1] = mod;

  act("$n's hand blurs, causing $B$0darkness$R to quickly expand and expunge light from the entire area.",
      ch, 0, 0, TO_ROOM, 0);
  ch->sendln("Your summoned $B$0darkness$R turns the area pitch black.");

  obj_to_room(globe, ch->in_room);
  DC::getInstance()->world[ch->in_room].light -= globe->obj_flags.value[1];

  return eSUCCESS;
}

/* GLOBE OF DARKNESS (potions, scrolls, staves, wands) */

int cast_globe_of_darkness(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_globe_of_darkness(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_globe_of_darkness(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_globe_of_darkness(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
      spell_globe_of_darkness(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in globe_of_darkness!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* EYES OF THE EAGLE (disabled) */

int spell_eyes_of_the_eagle(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  ch->sendln("This spell doesn't do anything right now.");
  return eSUCCESS;
}

int cast_eyes_of_the_eagle(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_eyes_of_the_eagle(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_eyes_of_the_eagle(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in eyes_of_the_eagle!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* ICESTORM */

int spell_icestorm(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int retval = eSUCCESS;
  int retval2;
  struct affected_type af;
  char buf[MAX_STRING_LENGTH];

  int learned = ch->has_skill(SPELL_ICESTORM);
  dam = 25 + learned * 4.25;

  if (DC::getInstance()->world[ch->in_room].sector_type == SECT_FROZEN_TUNDRA)
    dam = dam * 5 / 4;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_ARCTIC)
    dam = dam * 3 / 2;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_UNDERWATER)
    dam = dam * 3 / 4;
  else if (DC::getInstance()->world[ch->in_room].sector_type == SECT_DESERT)
    dam = dam * 1 / 2;

  ch->sendln("$B$3Ice$R erupts from the earth!");
  act("$n makes $B$3ice$R fall erupt from the earth!", ch, 0, 0, TO_ROOM, 0);

  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp_victim : character_list)
  {
    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }

    if ((ch->in_room == tmp_victim->in_room) && (ch != tmp_victim) && (!ARE_GROUPED(ch, tmp_victim)) && can_be_attacked(ch, tmp_victim))
    {

      if (number(1, 100) < level / 2 && saves_spell(ch, tmp_victim, 0, SAVE_TYPE_COLD) < 0)
      {
        af.type = SPELL_ICESTORM;
        af.duration = 5;
        af.modifier = -5;
        af.location = APPLY_STR;
        af.bitvector = -1;
        affect_to_char(tmp_victim, &af);

        act("$N shivers and looks very $B$3chilled$R as $E suffers the affects of your storm.", ch, 0, tmp_victim, TO_CHAR, 0);
        act("You shiver and feel very $B$3chilled$R as you suffer the affects of $n's storm.", ch, 0, tmp_victim, TO_VICT, 0);
        act("$N shivers and looks very $B$3chilled$R as $E suffers the affects of $n's storm.", ch, 0, tmp_victim, TO_ROOM, NOTVICT);
      }

      retval2 = damage(ch, tmp_victim, dam, TYPE_COLD, SPELL_ICESTORM);

      if (isSet(retval2, eVICT_DIED))
        SET_BIT(retval, eVICT_DIED);
      else if (isSet(retval2, eCH_DIED))
      {
        SET_BIT(retval, eCH_DIED);
        break;
      }
    }
    else
    {
      if (DC::getInstance()->world[ch->in_room].zone == DC::getInstance()->world[tmp_victim->in_room].zone)
        tmp_victim->sendln("You feel a BLAST of $B$3cold$R air.");
    }
  }

  return retval;
}

/* ICESTORM (potions, scrolls, wands, staves) */

int cast_icestorm(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_icestorm(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_icestorm(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_icestorm(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_icestorm(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in icestorm!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* LIGHTNING SHIELD */

int spell_lightning_shield(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (find_spell_shield(ch, victim) && IS_PC(victim))
    return eFAILURE;

  if (!victim->affected_by_spell(SPELL_LIGHTNING_SHIELD))
  {
    act("$n is surrounded by a crackling shield of $B$5electrical$R energy.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    act("You are surrounded by a crackling shield of $B$5electrical$R energy.", victim, 0, 0, TO_CHAR, 0);

    af.type = SPELL_LIGHTNING_SHIELD;
    af.duration = 2 + skill / 23;
    af.modifier = skill;
    af.location = APPLY_NONE;
    af.bitvector = AFF_LIGHTNINGSHIELD;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

/* LIGHTNING SHIELD (potion, scroll, wand, staves) */

int cast_lightning_shield(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_lightning_shield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_lightning_shield(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_lightning_shield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
      spell_lightning_shield(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in lightning shield!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* BLUE BIRD */

int spell_blue_bird(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  int dam;
  int count;
  int retval = eSUCCESS;
  int ch_level = ch->getLevel();
  if (ch_level < 5)
    ch_level = 5;
  set_cantquit(ch, victim);
  switch (DC::getInstance()->world[ch->in_room].sector_type)
  {

  case SECT_SWAMP:
    count = 3;
    break;
  case SECT_FOREST:
    count = 5;
    break;
  case SECT_AIR:
    count = 4;
    break;
  case SECT_HILLS:
    count = 4;
    break;
  case SECT_MOUNTAIN:
    count = 3;
    break;
  case SECT_FIELD:
    count = 3;
    break;
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_BEACH:
    count = 3;
    break;

  case SECT_UNDERWATER:
    ch->sendln("But you're underwater!  Your poor birdie would drown:(");
    return eFAILURE;

  default:
    count = 2;
    break;
  }

  dam = number(10, ch_level + 5) + getRealSpellDamage(ch);
  while (!SOMEONE_DIED(retval) && count--)
  {
    retval = damage(ch, victim, dam, TYPE_PHYSICAL_MAGIC, SPELL_BLUE_BIRD);
    dam = number(10, ch_level + 5);
  }

  return retval;
}

/* BLUE BIRD (scrolls, wands, staves) */

int cast_blue_bird(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_blue_bird(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_blue_bird(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_blue_bird(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in blue_bird!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* DEBILITY */

int spell_debility(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  int retval = eSUCCESS, duration = 0;
  double percent = 0;

  if (victim->affected_by_spell(SPELL_DEBILITY))
  {
    ch->sendln("Your victim has already been debilitized.");
    return eSUCCESS;
  }

  if (skill < 40)
  {
    percent = 30;
    duration = 2;
  }
  else if (skill > 40 && skill <= 60)
  {
    percent = 45;
    duration = 3;
  }
  else if (skill > 60 && skill <= 80)
  {
    percent = 60;
    duration = 4;
  }
  else if (skill > 80 && skill <= 90)
  {
    percent = 75;
    duration = 5;
  }
  else if (skill > 90)
  {
    percent = 90;
    duration = 6;
  }

  set_cantquit(ch, victim);

  if (malediction_res(ch, victim, SPELL_DEBILITY))
  {
    act("$N resists your attempt to $6debilitize$R $M!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to $6debilitize$R $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to $6debilitize$R you!", ch, nullptr, victim, TO_VICT, 0);
  }
  else
  {
    af.type = SPELL_DEBILITY;
    af.duration = duration;
    af.modifier = 0 - (int)((double)victim->hit_gain_lookup() * (percent / 100));
    af.location = APPLY_HP_REGEN;
    af.bitvector = -1;
    affect_to_char(victim, &af);
    af.location = APPLY_MOVE_REGEN;
    af.modifier = 0 - (int)((double)victim->move_gain_lookup() * (percent / 100));
    affect_to_char(victim, &af);
    af.location = APPLY_KI_REGEN;
    af.modifier = 0 - (int)((double)victim->ki_gain_lookup() * (percent / 100));
    affect_to_char(victim, &af);
    af.location = APPLY_MANA_REGEN;
    af.modifier = 0 - (int)((double)victim->mana_gain_lookup() * (percent / 100));
    affect_to_char(victim, &af);
    victim->sendln("Your body becomes $6debilitized$R, reducing your regenerative abilities!");
    act("$N takes on an unhealthy pallor as $n's magic takes hold.", ch, 0, victim, TO_ROOM, NOTVICT);
    act("Your magic $6debilitizes$R $N!", ch, 0, victim, TO_CHAR, 0);
  }
  if (IS_NPC(victim) && !victim->fighting)
  {
    mob_suprised_sayings(victim, ch);
    retval = attack(victim, ch, 0);
    SWAP_CH_VICT(retval);
  }

  return retval;
}

/* DEBILITY (scrolls, wands, staves) */

int cast_debility(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_debility(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_debility(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_debility(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_debility(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in debility!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* ATTRITION */

int spell_attrition(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  int retval = eSUCCESS;
  int acmod = 0, tohit = 0, duration = 0;

  if (victim->affected_by_spell(SPELL_ATTRITION))
  {
    ch->sendln("Your victim is already suffering from the affects of that spell.");
    return eSUCCESS;
  }
  acmod = skill;
  if (skill < 40)
  {
    tohit = -3;
    duration = 2;
  }
  else if (skill > 40 && skill <= 60)
  {
    tohit = -6;
    duration = 3;
  }
  else if (skill > 60 && skill <= 80)
  {
    tohit = -9;
    duration = 4;
  }
  else if (skill > 80 && skill <= 90)
  {
    tohit = -12;
    duration = 5;
  }
  else if (skill > 90)
  {
    tohit = -15;
    duration = 6;
  }

  set_cantquit(ch, victim);

  if (malediction_res(ch, victim, SPELL_ATTRITION))
  {
    act("$N resists your attempt to cast attrition on $M!", ch, nullptr, victim, TO_CHAR, 0);
    act("$N resists $n's attempt to cast attrition on $M!", ch, nullptr, victim, TO_ROOM, NOTVICT);
    act("You resist $n's attempt to cast attrition on you!", ch, nullptr, victim, TO_VICT, 0);
  }
  else
  {

    af.type = SPELL_ATTRITION;
    af.duration = duration;
    af.modifier = acmod;
    af.location = APPLY_AC;
    af.bitvector = -1;
    affect_to_char(victim, &af);
    af.modifier = tohit;
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);

    victim->sendln("You feel less energetic as painful magic penetrates your body!");
    act("$N's body takes on an unhealthy colouring as $n's magic enters $M.", ch, 0, victim, TO_ROOM, NOTVICT);
    act("$N pales as your devitalizing magic courses through $S veins!", ch, 0, victim, TO_CHAR, 0);
  }

  if (IS_NPC(victim))
  {
    if (!victim->fighting)
    {
      mob_suprised_sayings(victim, ch);
      retval = attack(victim, ch, 0);
    }
  }

  return retval;
}

/* ATTRITION (scrolls, wands, staves) */

int cast_attrition(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  Character *next_v;
  int retval;

  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_attrition(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_debility(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_attrition(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = next_v)
    {
      next_v = tar_ch->next_in_room;

      if (!ARE_GROUPED(ch, tar_ch))
      {
        retval = spell_attrition(level, ch, tar_ch, 0, skill);
        if (isSet(retval, eCH_DIED))
          return retval;
      }
    }
    return eSUCCESS;
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in attrition!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* VAMPIRIC AURA */

int spell_vampiric_aura(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  /*
  if (victim->affected_by_spell(SPELL_ACID_SHIELD)){
     act("A film of $B$0shadow$R begins to rise around $n but fades around $s ankles.", victim, 0, 0, TO_ROOM, INVIS_NULL);
     ch->sendln("A film of $B$0shadow$R tries to rise around you but dissolves in your acid shield.");
     return eFAILURE;
  }
*/
  if (ch->affected_by_spell(SPELL_VAMPIRIC_AURA_TIMER))
  {
    ch->sendln("Your dark power is not available to you so soon.");
    return eFAILURE;
  }
  act("A film of $B$0shadow$R encompasses $n then fades from view.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("A film of $B$0shadow$R encompasses you then fades from view.", victim, 0, 0, TO_CHAR, 0);

  af.type = SPELL_VAMPIRIC_AURA;
  af.duration = skill / 25;
  af.modifier = skill;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  af.type = SPELL_VAMPIRIC_AURA_TIMER;
  af.duration = 20;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

/* VAMPIRIC AURA */

int cast_vampiric_aura(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    if (GET_ALIGNMENT(ch) > -1000)
    {
      ch->sendln("You must be utterly vile to cast this spell.");
      return eFAILURE;
    }
    return spell_vampiric_aura(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in vampiric aura!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* HOLY AURA */

int spell_holy_aura(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (ch->affected_by_spell(SPELL_HOLY_AURA_TIMER))
  {
    ch->sendln("Your god is not so foolish as to grant that power to you so soon again.");
    return eFAILURE;
  }

  act("A serene calm comes over $n.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  act("A serene calm encompasses you.", victim, 0, 0, TO_CHAR, 0);
  GET_ALIGNMENT(ch) -= 200;
  GET_ALIGNMENT(ch) = MIN(1000, MAX((-1000), GET_ALIGNMENT(ch)));
  extern void zap_eq_check(Character * ch);
  zap_eq_check(ch);
  af.type = SPELL_HOLY_AURA;
  af.duration = 4;
  af.modifier = 50;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(victim, &af);

  af.type = SPELL_HOLY_AURA_TIMER;
  af.duration = 20;
  affect_to_char(victim, &af);

  return eSUCCESS;
}

int cast_holy_aura(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{

  struct affected_type af;
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    char buf[MAX_INPUT_LENGTH];
    if (ch->affected_by_spell(SPELL_SANCTUARY))
    {
      act("Your god does not allow that much power to be bestowed upon you.", ch, 0, 0, TO_CHAR, 0);
      return eFAILURE;
    }
    if (GET_ALIGNMENT(ch) < 1000)
    {
      ch->sendln("You are not holy enough to cast this spell.");
      return eFAILURE;
    }
    one_argument(arg, buf);
    int mod;
    if (!str_cmp(buf, "magic"))
      mod = 25;
    else if (!str_cmp(buf, "physical"))
      mod = 50;
    else
    {
      ch->sendln("You need to specify whether you want protection against magic or physical.");
      ch->mana += spell_info[SPELL_HOLY_AURA].min_usesmana();
      return eFAILURE;
    }

    if (ch->affected_by_spell(SPELL_HOLY_AURA_TIMER))
    {
      ch->sendln("Your god is not so foolish as to grant that power to you so soon again.");
      return eFAILURE;
    }

    act("A serene calm comes over $n.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    act("A serene calm encompasses you.", ch, 0, 0, TO_CHAR, 0);
    GET_ALIGNMENT(ch) -= 250;
    GET_ALIGNMENT(ch) = MIN(1000, MAX((-1000), GET_ALIGNMENT(ch)));
    extern void zap_eq_check(Character * ch);
    zap_eq_check(ch);
    af.type = SPELL_HOLY_AURA;
    af.duration = 4;
    af.modifier = mod;
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(ch, &af);

    af.type = SPELL_HOLY_AURA_TIMER;
    af.duration = 20;
    affect_to_char(ch, &af);

    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in holy aura!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* DISMISS FAMILIAR */

int spell_dismiss_familiar(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  victim = nullptr;

  for (struct follow_type *k = ch->followers; k; k = k->next)
    if (IS_NPC(k->follower) && IS_AFFECTED(k->follower, AFF_FAMILIAR))
    {
      victim = k->follower;
      break;
    }

  if (nullptr == victim)
  {
    ch->sendln("You don't have a familiar!");
    return eFAILURE;
  }

  act("$n disappears in a flash of flame and shadow.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  extract_char(victim, true);

  GET_MANA(ch) += 51;
  if (GET_MANA(ch) > GET_MAX_MANA(ch))
    GET_MANA(ch) = GET_MAX_MANA(ch);

  return eSUCCESS;
}

/* DISMISS FAMILIAR (potions, wands, scrolls, staves) */

int cast_dismiss_familiar(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_dismiss_familiar(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_dismiss_familiar(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_dismiss_familiar(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
      spell_dismiss_familiar(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in dismiss_familiar!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* DISMISS CORPSE */

int spell_dismiss_corpse(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  victim = nullptr;
  // ch->sendln("Disabled.");
  // return eFAILURE;

  for (struct follow_type *k = ch->followers; k; k = k->next)
    if (IS_NPC(k->follower) && k->follower->affected_by_spell(SPELL_CHARM_PERSON))
    {
      victim = k->follower;
      break;
    }

  if (nullptr == victim)
  {
    ch->sendln("You don't have a corpse!");
    return eFAILURE;
  }

  act("$n begins to melt and dissolves into the ground... dust to dust.", victim, 0, 0, TO_ROOM, INVIS_NULL);
  extract_char(victim, true);

  return eSUCCESS;
}

/* DISMISS CORPSE (wands, scrolls, potions, staves) */

int cast_dismiss_corpse(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_dismiss_corpse(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_dismiss_corpse(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_dismiss_corpse(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
      spell_dismiss_corpse(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in dismiss_corpse!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* RELEASE ELEMENTAL */

int spell_release_elemental(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  victim = nullptr;
  // ch->sendln("Disabled.");
  // return eFAILURE;

  for (struct follow_type *k = ch->followers; k; k = k->next)
    if (IS_NPC(k->follower) && ISSET(k->follower->affected_by, AFF_CHARM))
    {
      victim = k->follower;
      break;
    }

  if (nullptr == victim)
  {
    ch->sendln("You don't have an elemental!");
    return eFAILURE;
  }

  switch (DC::getInstance()->mob_index[victim->mobdata->nr].virt)
  {
  case FIRE_ELEMENTAL:
    act("The room begins to cool as $n returns to it's own plane of existance.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    break;
  case WATER_ELEMENTAL:
    act("$n melts into a puddle and vanishes as it is released from its servitude.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    break;
  case AIR_ELEMENTAL:
    act("$n suddenly vanishes, leaving only the strong scent of ozone.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    break;
  case EARTH_ELEMENTAL:
    act("Rocks fly everywhere as $n returns to its own plane of existance.", victim, 0, 0, TO_ROOM, INVIS_NULL);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in spell_release_elemental!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }

  extract_char(victim, true);

  return eSUCCESS;
}

/* RELEASE ELEMENTAL (wands, scrolls, potions, staves) */

int cast_release_elemental(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_release_elemental(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_release_elemental(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_release_elemental(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
      spell_release_elemental(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in cast_release_elemental!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* VISAGE OF HATE */

int spell_visage_of_hate(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You have no group to cast this upon.");
    return eFAILURE;
  }

  for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
  {
    if (tmp_char == ch)
      continue;
    if (!ARE_GROUPED(ch, tmp_char))
      continue;
    affect_from_char(tmp_char, SPELL_VISAGE_OF_HATE);
    affect_from_char(tmp_char, SPELL_VISAGE_OF_HATE);
    tmp_char->sendln("The violence hate brings shows upon your face.");

    af.type = SPELL_VISAGE_OF_HATE;
    af.duration = 1 + skill / 10;
    af.modifier = -skill / 25;
    af.location = APPLY_HITROLL;
    af.bitvector = -1;
    affect_to_char(tmp_char, &af);
    af.modifier = 1 + skill / 25;
    af.location = APPLY_DAMROLL;
    affect_to_char(tmp_char, &af);
  }

  ch->sendln("Your disdain and hate for all settles upon your peers.");
  return eSUCCESS;
}

int cast_visage_of_hate(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_visage_of_hate(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in visage_of_hate!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* BLESSED HALO */

int spell_blessed_halo(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (!IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You have no group to cast this upon.");
    return eFAILURE;
  }

  for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
  {
    if (tmp_char == ch)
      continue;
    if (!ARE_GROUPED(ch, tmp_char))
      continue;
    affect_from_char(tmp_char, SPELL_BLESSED_HALO);
    affect_from_char(tmp_char, SPELL_BLESSED_HALO);
    tmp_char->sendln("You feel blessed.");

    af.type = SPELL_BLESSED_HALO;
    af.duration = 1 + skill / 10;
    af.modifier = skill / 18;
    af.location = APPLY_HITROLL;
    af.bitvector = -1;
    affect_to_char(tmp_char, &af);
    af.modifier = skill / 18;
    af.location = APPLY_HP_REGEN;
    affect_to_char(tmp_char, &af);
  }

  ch->sendln("Your group members benefit from your blessing.");
  return eSUCCESS;
}

int cast_blessed_halo(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_blessed_halo(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in blessed_halo!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* SPIRIT WALK (GHOST WALK) */

int spell_ghost_walk(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  if (!ch)
    return eFAILURE;

  if (ch->fighting)
  {
    ch->sendln("You're a bit too distracted by the battle.");
    return eFAILURE;
  }
  if (!ch->desc || ch->desc->snooping || IS_NPC(ch) || !ch->in_room)
  {
    ch->sendln("You can't do that at the moment.");
    return eFAILURE;
  }

  if (ch->desc->snoop_by)
  {
    ch->desc->snoop_by->character->send("Whoa! Almost got caught snooping!\n");
    ch->desc->snoop_by->character->sendln("Your victim is casting spiritwalk spell.");
    ch->desc->snoop_by->character->do_snoop(ch->desc->snoop_by->character->getName().split(' '));
  }
  int vnum;
  switch (DC::getInstance()->world[ch->in_room].sector_type)
  {
  case SECT_INSIDE:
  case SECT_CITY:
  case SECT_PAVED_ROAD:
    vnum = 99;
    break;
  case SECT_FIELD:
  case SECT_HILLS:
  case SECT_MOUNTAIN:
    vnum = 98;
    break;
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_UNDERWATER:
    vnum = 97;
    break;
  case SECT_AIR:
    vnum = 96;
    break;
  case SECT_FROZEN_TUNDRA:
  case SECT_ARCTIC:
    vnum = 95;
    break;
  case SECT_DESERT:
  case SECT_BEACH:
    vnum = 94;
    break;
  case SECT_SWAMP:
    vnum = 93;
    break;
  case SECT_FOREST:
    //  case SECT_JUNGLE:
    vnum = 92;
    break;
  default:
    ch->sendln("Invalid sectortype. Please report location to an imm.");
    return eFAILURE;
  }
  int mobile;
  if ((mobile = real_mobile(vnum)) < 0)
  {
    DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Ghostwalk - Bad mob vnum: vnum %d.", vnum);
    ch->sendln("\"Spirit\" for this sector not yet implented.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  Character *mob;
  mob = ch->getDC()->clone_mobile(mobile);
  mob->hometown = ch->getDC()->world[ch->in_room].number;
  char_to_room(mob, ch->in_room);

  ch->sendln("You call upon the spirits of this area, shifting into a trance-state.");
  ch->sendln("(Use the 'return' command to return to your body).");
  ch->player->possesing = 1;
  ch->desc->character = mob;
  ch->desc->original = ch;
  mob->desc = ch->desc;
  ch->desc = 0;
  return eSUCCESS;
}

int cast_ghost_walk(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_ghost_walk(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in ghost_walk!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* CONJURE ELEMENTAL */

int spell_conjure_elemental(uint8_t level, Character *ch, char *arg, Character *victim, class Object *component, int skill)
{
  if (!ch)
    return eFAILURE;
  Character *mob;
  int r_num, liquid, virt;
  // 88 = fire fire
  // 89 = water cold
  // 90 = air energy
  // 91 = earth magic

  if (many_charms(ch))
  {
    ch->sendln("How do you plan on controlling so many followers?");
    return eFAILURE;
  }
  //  Object *container  = nullptr;
  if (!str_cmp(arg, "fire"))
  {
    virt = FIRE_ELEMENTAL;
    liquid = 13;
  }
  else if (!str_cmp(arg, "water"))
  {
    virt = WATER_ELEMENTAL;
    liquid = 17;
  }
  else if (!str_cmp(arg, "air"))
  {
    virt = AIR_ELEMENTAL;
    liquid = 2;
  }
  else if (!str_cmp(arg, "earth"))
  {
    virt = EARTH_ELEMENTAL;
    liquid = 9;
  }
  else
  {
    ch->sendln("Unknown elemental type. Applicable types are: fire, water, air and earth.");
    return eFAILURE;
  }
  Object *obj;
  for (obj = ch->carrying; obj; obj = obj->next_content)
  {
    if (obj->obj_flags.type_flag == ITEM_DRINKCON &&
        obj->obj_flags.value[2] == liquid &&
        obj->obj_flags.value[1] >= 5)
      break;
  }
  if (obj == nullptr)
  {
    ch->send(fmt::format("You do not have a container filled with 5 units of {}.\r\n", drinks[liquid]));

    if (IS_IMMORTAL(ch))
    {
      ch->send(fmt::format("Using your immortal powers you materialize the missing {}.\r\n", drinks[liquid]));
    }
    else
    {
      return eFAILURE;
    }
  }
  else
  {
    obj->obj_flags.value[1] -= 5;
  }

  switch (virt)
  {
  case FIRE_ELEMENTAL:
    act("Your container of blood burns hotly as a denizen of the elemental plane of fire arrives in a blast of flame!", ch, nullptr, nullptr, TO_CHAR, 0);
    act("$n's container of blood burns hotly as a denizen of the elemental plane of fire arrives in a blast of flame!", ch, nullptr, nullptr, TO_ROOM,
        0);
    break;
  case WATER_ELEMENTAL:
    act("Your holy water bubbles briefly as an icy denizen of the elemental plane of water crystalizes into existence.", ch, nullptr, nullptr,
        TO_CHAR, 0);
    act("$n's holy water bubbles briefly as an icy denizen of the elemental plane of water crystalizes intoto existence.", ch, nullptr, nullptr,
        TO_ROOM, 0);
    break;
  case EARTH_ELEMENTAL:
    act("Your dirty water churns violently as a denizen of the elemental plane of earth rises from the ground.", ch, nullptr, nullptr, TO_CHAR, 0);
    act("$n's dirty water churns violently as a denizen of the elemental plane of earth rises from the ground.", ch, nullptr, nullptr, TO_ROOM, 0);
    break;
  case AIR_ELEMENTAL:
    act("Your wine boils with energy as a denizen of the elemental plane of air crackles into existance.", ch, nullptr, nullptr, TO_CHAR, 0);
    act("$n's wine boils with energy as a denizen of the elemental plane of air crackles into existance.", ch, nullptr, nullptr, TO_ROOM, 0);
    break;
  default:
    ch->sendln("That item is not used for elemental summoning.");
    return eFAILURE;
  }
  if ((r_num = real_mobile(virt)) < 0)
  {
    ch->sendln("Mobile: Elemental not found.");
    return eFAILURE;
  }

  mob = ch->getDC()->clone_mobile(r_num);
  char_to_room(mob, ch->in_room);
  mob->max_hit += skill * 5;
  mob->hit = mob->max_hit;
  if (skill > 80)
    mob->mobdata->mob_flags.value[3] = 77;
  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  SETBIT(mob->affected_by, AFF_ELEMENTAL);
  SETBIT(mob->affected_by, AFF_CHARM);

  add_follower(mob, ch);

  return eSUCCESS;
}

/* MEND GOLEM */

int cast_mend_golem(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_mend_golem(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in ghost_walk!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* DIVINE INTERVENTION */

int spell_divine_intervention(uint8_t level, Character *ch, Character *victim, Object *obj, int skill)
{
  struct affected_type af;

  act("Light pours down from above bathing you in a shell of divine protection.", ch, 0, 0, TO_CHAR, 0);
  act("Light pours down from above and surrounds $n in a shell of divine protection.", ch, 0, 0, TO_ROOM, 0);

  af.type = SPELL_DIV_INT_TIMER;
  af.duration = 24;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  af.type = SPELL_NO_CAST_TIMER;
  af.duration = 5 + skill / 15;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;
  affect_to_char(ch, &af, DC::PULSE_VIOLENCE);

  af.type = SPELL_DIVINE_INTER;
  af.duration = 4 + skill / 15;
  af.modifier = 10 - skill / 10;
  affect_to_char(ch, &af, DC::PULSE_VIOLENCE);

  return eSUCCESS;
}

int cast_divine_intervention(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_divine_intervention(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in divine intervention!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* WRATH OF GOD */

int spell_wrath_of_god(uint8_t level, Character *ch, Character *victim, Object *obj, int skill)
{
  int castcost = 0, dam = 0;
  int retval = eSUCCESS;
  char buf[MAX_STRING_LENGTH];
  Character *next_vict;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You cannot cast this here.");
    return eFAILURE;
  }

  for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = victim->next_in_room)
    castcost += 75;

  castcost -= 75; // the initial 75 to cast the spell

  if (GET_MANA(ch) < castcost)
  {
    ch->sendln("You do not have enough mana to cast this spell.");
    return eFAILURE;
  }
  else
    GET_MANA(ch) -= castcost;

  ch->sendln("You call forth the fury of the gods to consume the area in a holy tempest!");
  act("$n calls forth the fury of the gods to consume the area in a holy tempest!", ch, 0, 0, TO_ROOM, 0);

  for (victim = DC::getInstance()->world[ch->in_room].people; victim && victim != (Character *)0x95959595; victim = next_vict)
  {
    next_vict = victim->next_in_room;

    if (IS_PC(victim) && victim->getLevel() >= IMMORTAL)
      continue;
    if (victim == ch)
      continue; // save for last

    dam = victim->getLevel() * 10 + skill * 2;
    sprintf(buf, "$B%d$R", dam_percent(skill, dam));
    send_damage("The holy tempest damages you for | hitpoints!", victim, 0, 0, buf, "The holy tempest hurts you significantly!", TO_CHAR);
    retval &= damage(ch, victim, dam, TYPE_MAGIC, SPELL_WRATH_OF_GOD);
  }

  // damage the caster!!
  dam = ch->getLevel() * 10 + skill * 2;
  sprintf(buf, "$B%d$R", dam_percent(skill, dam));
  send_damage("The holy tempest damages you for | hitpoints!", ch, 0, 0, buf, "The holy tempest hurts you significantly!", TO_CHAR);
  retval &= damage(ch, ch, dam, TYPE_MAGIC, SPELL_WRATH_OF_GOD);

  return retval;
}

int cast_wrath_of_god(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_wrath_of_god(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in wrath of god!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* ATONEMENT */

int spell_atonement(uint8_t level, Character *ch, Character *victim, Object *obj, int skill)
{
  int heal = GET_MAX_MANA(ch) - GET_MANA(ch);
  heal = heal / 1000 + 1;

  if (heal < 0)
  {
    ch->sendln("Yeah, right.");
    return eFAILURE;
  }
  if (GET_MAX_MANA(ch) - heal < 1)
  {
    ch->sendln("You do not have enough mana to sacrifice.");
    return eFAILURE;
  }
  if (ch->fighting)
    ch->max_mana -= 2 * heal;
  else
    ch->max_mana -= heal;
  GET_MANA(ch) = GET_MAX_MANA(ch);

  ch->sendln("You pray fervently for the support of the gods and are rewarded restoration...at a price.");
  act("$n prays fervently for the support of the gods and is rewarded restoration...at a price.", ch, 0, 0, TO_ROOM, 0);

  return eSUCCESS;
}

int cast_atonement(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_atonement(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in atonement!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* SILENCE */

int spell_silence(uint8_t level, Character *ch, Character *victim, Object *obj, int skill)
{
  Object *silence_obj = nullptr;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You cannot silence this room.");
    return eFAILURE;
  }

  ch->sendln("Your chants fade softy to an eerie quiet as the silence takes hold...");
  act("$n's chants fade to an eerie quiet as the silence takes hold...", ch, 0, 0, TO_ROOM, 0);

  if (!(silence_obj = clone_object(real_object(SILENCE_OBJ_NUMBER))))
  {
    ch->sendln("Error setting silence object.  Tell an immortal.");
    return eFAILURE;
  }

  silence_obj->obj_flags.value[0] = skill / 20 + 1;

  obj_to_room(silence_obj, ch->in_room);

  return eSUCCESS;
}

int cast_silence(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_silence(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in silence!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* IMMUNITY */

int spell_immunity(uint8_t level, Character *ch, Character *victim, Object *obj, int skill, int spl = 0)
{
  struct affected_type af;

  if ((spell_info[spl].targets() & TAR_IGNORE))
  {
    ch->sendln("You find it impossible to immunize yourself against this type of spell.");
    return eSUCCESS;
  }

  if (ch->fighting)
  {
    ch->sendln("You cannot concentrate enough to immunize yourself.");
    return eSUCCESS;
  }

  if (ch->affected_by_spell(SPELL_IMMUNITY))
  {
    csendf(ch, "You are already immune to %s.\r\n", spells[ch->affected_by_spell(SPELL_IMMUNITY)->modifier]);
    return eSUCCESS;
  }

  ch->send("You reach forth and etch a protective sigil in the air that briefly surrounds you in a soft $Bs$3h$5i$7m$3m$5e$7r$3i$5n$7g$3 l$5i$7g$3h$5t$R.");
  act("$n reaches forth and etches a protective sigil in the air that briefly surrounds $m in a soft $Bs$3h$5i$7m$3m$5e$7r$3i$5n$7g$3 l$5i$7g$3h$5t$R.", ch, 0, 0, TO_ROOM, 0);

  af.type = SPELL_IMMUNITY;
  af.duration = 2 + skill / 10;
  af.modifier = spl;
  af.location = 0;
  af.bitvector = -1;

  affect_to_char(ch, &af, DC::PULSE_REGEN);

  return eSUCCESS;
}

int cast_immunity(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_immunity(level, ch, (Character *)arg, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in immunity!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* BONESHIELD */

int spell_boneshield(uint8_t level, Character *ch, Character *victim, Object *obj, int skill)
{
  struct affected_type af;

  if (IS_PC(victim) || victim->master != ch)
  {
    ch->sendln("You cannot cast this spell on that.");
    return eFAILURE;
  }

  send_to_room("Deadly spikes of bone burst forth from the limbs and torso of the revenant forming a deadly shield!\r\n", ch->in_room);

  af.type = SPELL_BONESHIELD;
  af.location = 0;
  af.duration = -1;
  af.modifier = skill / 2;
  af.bitvector = -1;

  affect_to_char(victim, &af);

  return eSUCCESS;
}

int cast_boneshield(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_boneshield(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in boneshield!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* CHANNEL */

int spell_channel(uint8_t level, Character *ch, Character *victim, Object *obj, int skill, int heal = 0)
{
  char buf[MAX_STRING_LENGTH];

  if (!can_heal(ch, victim, SPELL_CHANNEL))
    return eSUCCESS; // to still use the mana of the loser botting channel

  if (heal <= 0)
    heal = GET_MAX_HIT(victim) - victim->getHP();
  if (GET_MANA(ch) < 2 * heal - 100)
    heal = GET_MANA(ch) / 2 + 50;
  GET_MANA(ch) -= heal * 2 - 100;
  victim->addHP(heal);

  sprintf(buf, "$B%d$R", heal);
  send_damage("You channel the power of the gods to heal $N of | damage.", ch, 0, victim, buf, "You channel the power of the gods to heal $N of $S injuries.", TO_CHAR);
  send_damage("$n channels the power of the gods to heal you of | damage.", ch, 0, victim, buf, "$n channels the power of the gods to heal you of your injuries.", TO_VICT);
  send_damage("$n channels the power of the gods to heal $N of | damage.", ch, 0, victim, buf, "$n channels the power of the gods to heal $N of $S injuries.", TO_ROOM);

  return eSUCCESS;
}

int cast_channel(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_channel(level, ch, tar_ch, 0, skill, atoi(arg));
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in channel!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

SPELL_POINTER get_wild_magic_offensive(uint8_t level, Character *ch, Character *victim, Object *obj, int skill)
{
  const int MAX_OFFENSIVE = 25;
  SPELL_POINTER spell_to_cast = nullptr;
  switch (number(1, MAX_OFFENSIVE + 1)) //+1 causes a chance of defensive
  {
  case 1:
    spell_to_cast = cast_blindness;
    break;
  case 2:
    spell_to_cast = cast_fear;
    break;
  case 3:
    spell_to_cast = cast_chill_touch;
    break;
  case 4:
    spell_to_cast = cast_dispel_magic;
    break;
  case 5:
    spell_to_cast = cast_colour_spray;
    break;
  case 6:
    spell_to_cast = cast_drown;
    break;
  case 7:
    spell_to_cast = cast_souldrain;
    break;
  case 8:
    spell_to_cast = cast_sparks;
    break;
  case 9:
    spell_to_cast = cast_flamestrike;
    break;
  case 10:
    spell_to_cast = cast_curse;
    break;
  case 11:
    spell_to_cast = cast_weaken;
    break;
  case 12:
    spell_to_cast = cast_acid_blast;
    break;
  case 13:
    spell_to_cast = cast_energy_drain;
    break;
  case 14:
    spell_to_cast = cast_fireball;
    break;
  case 15:
    spell_to_cast = cast_hellstream;
    break;
  case 16:
    spell_to_cast = cast_lightning_bolt;
    break;
  case 17:
    spell_to_cast = cast_power_harm;
    break;
  case 18:
    spell_to_cast = cast_magic_missile;
    break;
  case 19:
    spell_to_cast = cast_poison;
    break;
  case 20:
    spell_to_cast = cast_bee_sting;
    break;
  case 21:
    spell_to_cast = cast_paralyze;
    break;
  case 22:
    spell_to_cast = cast_debility;
    break;
  case 23:
    spell_to_cast = cast_attrition;
    break;
  case 24:
    spell_to_cast = cast_meteor_swarm;
    break;
  case 25:
    spell_to_cast = cast_sleep;
    break;

  // default case calls spell wild magic with opposite effect
  default:
    ch->sendln("Your magic goes wild and has the opposite effect!");
    spell_to_cast = get_wild_magic_defensive(level, ch, victim, obj, skill);
    break;
  }
  return spell_to_cast;
}

int cast_solidity(uint8_t level, Character *ch, char *arg,
                  int type, Character *tar_ch,
                  class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_solidity(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_solidity(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_solidity(level, ch, tar_ch, 0, skill);
    break;
    return spell_solidity(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_STAFF:
    for (tar_ch = DC::getInstance()->world[ch->in_room].people; tar_ch; tar_ch = tar_ch->next_in_room)
      spell_solidity(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in solidity!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int spell_solidity(uint8_t level, Character *ch, Character *victim, Object *obj, int skill)
{
  struct affected_type af;
  if (IS_AFFECTED(victim, AFF_SOLIDITY))
  {
    act("You are already $6violet$R.", victim, 0, 0, TO_CHAR, 0);
    act("$n is already $6violet$R.", ch, 0, 0, TO_CHAR, 0);
    return eSUCCESS;
  }

  if (!victim->affected_by_spell(AFF_SOLIDITY))
  {
    act("$n is surrounded by a pulsing, $6violet$R aura.", victim, 0, 0, TO_ROOM, 0);
    act("You become surrounded by a pulsing, $6violet$R aura.", victim, 0, 0, TO_CHAR, 0);

    af.type = SPELL_SOLIDITY;
    af.duration = level / 10;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_SOLIDITY;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

int cast_stability(uint8_t level, Character *ch, char *arg,
                   int type, Character *tar_ch,
                   class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_stability(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_stability(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_stability(level, ch, tar_ch, 0, skill);
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in stability!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int spell_stability(uint8_t level, Character *ch, Character *victim, Object *obj, int skill)
{
  struct affected_type af;
  if (IS_AFFECTED(victim, AFF_STABILITY))
  {
    act("You already have good balance.", victim, 0, 0, TO_CHAR, 0);
    act("$n already has good balance.", ch, 0, 0, TO_CHAR, 0);
    return eSUCCESS;
  }
  if (!victim->affected_by_spell(SPELL_STABILITY))
  {
    act("$n suddenly seems very hard to push over.", victim, 0, 0, TO_ROOM, 0);
    act("You feel very balanced.", victim, 0, 0, TO_CHAR, 0);

    af.type = SPELL_STABILITY;
    af.duration = level / 10;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_STABILITY;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

int cast_frostshield(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_frostshield(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_frostshield(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_frostshield(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in frostshield!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

int spell_frostshield(uint8_t level, Character *ch, Character *victim, Object *obj, int skill)
{
  struct affected_type af;

  if (find_spell_shield(ch, victim) && IS_PC(victim))
    return eFAILURE;

  if (!victim->affected_by_spell(SPELL_FROSTSHIELD))
  {
    act("$n is surrounded by a shield of $1ice$R.", victim, 0, 0, TO_ROOM, 0);
    act("You become surrounded by a shield of $1ice$R.", victim, 0, 0, TO_CHAR, 0);
    af.type = SPELL_FROSTSHIELD;
    af.duration = level / 10;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_FROSTSHIELD;
    affect_to_char(victim, &af);
  }
  return eSUCCESS;
}

SPELL_POINTER get_wild_magic_defensive(uint8_t level, Character *ch, Character *victim, Object *obj, int skill)
{
  const int MAX_DEFENSIVE = 50;
  SPELL_POINTER spell_to_cast = nullptr;

  switch (number(1, MAX_DEFENSIVE + 1)) //+1 to allow for a random chance
  {
  case 1:
    spell_to_cast = cast_armor;
    break;
  case 2:
    spell_to_cast = cast_water_breathing;
    break;
  case 3:
    spell_to_cast = cast_teleport;
    break;
  case 4:
    spell_to_cast = cast_bless;
    break;
  case 5:
    spell_to_cast = cast_barkskin;
    break;
  case 6:
    spell_to_cast = cast_iridescent_aura;
    break;
  case 7:
    spell_to_cast = cast_fly;
    break;
  case 8:
    spell_to_cast = cast_feline_agility;
    break;
  case 9:
    spell_to_cast = cast_cure_serious;
    break;
  case 10:
    spell_to_cast = cast_cure_critic;
    break;
  case 11:
    spell_to_cast = cast_camouflague;
    break;
  case 12:
    spell_to_cast = cast_stone_skin;
    break;
  case 13:
    spell_to_cast = cast_farsight;
    break;
  case 14:
    spell_to_cast = cast_shield;
    break;
  case 15:
    spell_to_cast = cast_freefloat;
    break;
  case 16:
    spell_to_cast = cast_detect_invisibility;
    break;
  case 17:
    spell_to_cast = cast_shadowslip;
    break;
  case 18:
    spell_to_cast = cast_detect_magic;
    break;
  case 19:
    spell_to_cast = cast_resist_energy;
    break;
  case 20:
    spell_to_cast = cast_staunchblood;
    break;
  case 21:
    spell_to_cast = cast_heal;
    break;
  case 22:
    spell_to_cast = cast_full_heal;
    break;
  case 23:
    spell_to_cast = cast_greater_stone_shield;
    break;
  case 24:
    spell_to_cast = cast_invisibility;
    break;
  case 25:
    spell_to_cast = cast_lightning_shield;
    break;
  case 26:
    spell_to_cast = cast_protection_from_evil;
    break;
  case 27:
    spell_to_cast = cast_sanctuary;
    break;
  case 28:
    spell_to_cast = cast_fireshield;
    break;
  case 29:
    spell_to_cast = cast_strength;
    break;
  case 30:
    spell_to_cast = cast_true_sight;
    break;
  case 31:
    spell_to_cast = cast_mana;
    break;
  case 32:
    spell_to_cast = cast_word_of_recall;
    break;
  case 33:
    spell_to_cast = cast_protection_from_good;
    break;
  case 34:
    spell_to_cast = cast_sense_life;
    break;
  case 35:
    spell_to_cast = cast_oaken_fortitude;
    break;
  case 36:
    spell_to_cast = cast_frostshield;
    break;
  case 37:
    spell_to_cast = cast_stability;
    break;
  case 38:
    spell_to_cast = cast_resist_acid;
    break;
  case 39:
    spell_to_cast = cast_resist_fire;
    break;
  case 40:
    spell_to_cast = cast_rapid_mend;
    break;
  case 41:
    spell_to_cast = cast_resist_cold;
    break;
  case 42:
    spell_to_cast = cast_solidity;
    break;
  case 43:
    spell_to_cast = cast_acid_shield;
    break;
  case 44:
    spell_to_cast = cast_resist_magic;
    break;
  case 45:
    spell_to_cast = cast_clarity;
    break;
  case 46:
    spell_to_cast = cast_stone_shield;
    break;
  case 47:
    spell_to_cast = cast_haste;
    break;
  case 48:
    spell_to_cast = cast_refresh;
    break;
  case 49:
    spell_to_cast = cast_infravision;
    break;
  case 50:
    spell_to_cast = cast_power_heal;
    break;

    // default case calls wild magic with opposite effect
  default:
    ch->sendln("Your magic goes wild and has the opposite effect!");
    spell_to_cast = get_wild_magic_offensive(level, ch, victim, obj, skill);
    break;
  }
  return spell_to_cast;
}

int cast_wild_magic(uint8_t level, Character *ch, char *arg,
                    int type, Character *tar_ch,
                    class Object *tar_obj, int skill)
{
  char off_def[MAX_INPUT_LENGTH + 1];
  SPELL_POINTER spell_to_cast = nullptr;

  arg = one_argument(arg, off_def);

  if (off_def[0] == 'o')
    spell_to_cast = get_wild_magic_offensive(level, ch, tar_ch, 0, skill);
  else if (off_def[0] == 'd')
    spell_to_cast = get_wild_magic_defensive(level, ch, tar_ch, 0, skill);
  else
  {
    ch->sendln("You need to specify offensive or defensive.");
    return eFAILURE;
  }

  if (spell_to_cast)
    return (*spell_to_cast)(level, ch, arg, type, tar_ch, tar_obj, skill);

  DC::getInstance()->logentry(QStringLiteral("Null spell passed to cast_wild_magic. Needs fixed asap."), ANGEL, DC::LogChannel::LOG_BUG);
  return eFAILURE;
}

/* SPIRIT SHIELD */

int spell_spirit_shield(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  Object *ssobj = nullptr;

  if (ch->equipment[WEAR_SHIELD])
  {
    ch->sendln("Your prayers are ignored by the gods as your hands are not free to receive thief gifts.");
    return eFAILURE;
  }

  if (!(ssobj = clone_object(real_object(SPIRIT_SHIELD_OBJ_NUMBER))))
  {
    ch->sendln("Error setting spirit shield object.  Tell an immortal.");
    return eFAILURE;
  }

  add_obj_affect(ssobj, APPLY_AC, -10 - skill / 4);
  add_obj_affect(ssobj, APPLY_HITROLL, number(1, 3) + skill / 20);
  add_obj_affect(ssobj, APPLY_DAMROLL, number(1, 3) + skill / 25);
  add_obj_affect(ssobj, APPLY_SAVES, number(1, 5) + skill / 20);
  add_obj_affect(ssobj, APPLY_REFLECT, number(1, 5) + 1);
  if (!number(0, 49))
    add_obj_affect(ssobj, APPLY_SANCTUARY, 100);

  ssobj->obj_flags.timer = 4 + skill / 5;

  ch->equip_char(ssobj, WEAR_SHIELD);

  ch->sendln("Your prayers to the gods for protection are answered as a glowing shield suddenly appears in your hand!");
  act("$n's prays to the gods for protection and a glowing shield appears in $s hand!", ch, 0, 0, TO_ROOM, 0);

  WAIT_STATE(ch, (int)(DC::PULSE_VIOLENCE * 2.5));

  return eSUCCESS;
}

int cast_spirit_shield(uint8_t level, Character *ch, char *arg, int type, Character *victim, Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_STAFF:
    return spell_spirit_shield(level, ch, victim, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in spirit shield!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* VILLAINY */

int spell_villainy(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (victim->affected_by_spell(SPELL_VILLAINY))
    affect_from_char(victim, SPELL_VILLAINY);

  af.type = SPELL_VILLAINY;
  af.duration = 9;
  af.modifier = skill;
  af.location = 0;
  af.bitvector = -1;

  affect_to_char(victim, &af);

  victim->sendln("You call upon the gods to grant you the magic and skill to defeat all that is good!");
  act("$n calls upon the gods to grant $m magic and skill in $s fight for evil!", victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

int cast_villainy(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_villainy(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_villainy(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_villainy(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_villainy(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in villainy!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* HEROISM */

int spell_heroism(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;

  if (victim->affected_by_spell(SPELL_HEROISM))
    affect_from_char(victim, SPELL_HEROISM);

  af.type = SPELL_HEROISM;
  af.duration = 9;
  af.modifier = skill;
  af.location = 0;
  af.bitvector = -1;

  affect_to_char(victim, &af);

  victim->sendln("You call upon the gods to grant you courage and skill in your fight for justice!");
  act("$n calls upon the gods to grant $m courage and skill in $s fight for justice!", victim, 0, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

int cast_heroism(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_heroism(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_heroism(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_heroism(level, ch, ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    if (!tar_ch)
      tar_ch = ch;
    return spell_heroism(level, ch, tar_ch, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in heroism!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* CONSERATE */

int spell_consecrate(uint8_t level, Character *ch, Character *victim,
                     class Object *obj, int skill)
{
  int spl = SPELL_CONSECRATE;
  int compNum = CONSECRATE_COMP_OBJ_NUMBER;
  Object *component = nullptr;

  if (IS_MORTAL(ch))
  {
    if (!(component = get_obj_in_list_vis(ch, compNum, ch->carrying)))
    {
      component = ch->equipment[WEAR_HOLD];
      if ((component == 0) || (compNum != DC::getInstance()->obj_index[component->item_number].virt))
      {
        component = ch->equipment[WEAR_HOLD2];
        if ((component == 0) || (compNum != DC::getInstance()->obj_index[component->item_number].virt))
        {
          ch->sendln("You do not have the required components.");
          return eFAILURE;
        }
      }
    }
    if (component->obj_flags.value[1] <= 0)
    {
      ch->sendln("There is nothing left in the container.");
      return eFAILURE;
    }
    if (component->obj_flags.value[2] != 13 && component->obj_flags.value[2] != 17)
    {
      ch->sendln("You do not have the required components,");
      return eFAILURE;
    }

    if (ch->getMove() < 100)
    {
      ch->sendln("You do not have enough energy to complete the incantation.");
      return eFAILURE;
    }
  }
  else if (IS_IMMORTAL(ch))
  {
    ch->send("You manifest the missing components.\r\n");
  }

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, CLAN_ROOM) ||
      isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) ||
      isSet(DC::getInstance()->world[ch->in_room].room_flags, NOLEARN))
  {
    if (IS_MORTAL(ch))
    {
      ch->sendln("Something about this room prohibits your incantation from being completed.");
      return eSUCCESS;
    }
    else if (IS_IMMORTAL(ch))
    {
      ch->send("Bypassing room restrictions.\r\n");
    }
  }

  Object *cItem = nullptr;

  if ((cItem = get_obj_in_list("consecrateitem", DC::getInstance()->world[ch->in_room].contents)))
  {
    if (ch == ((Character *)(cItem->obj_flags.origin)) && spl == SPELL_CONSECRATE)
    {
      ch->sendln("You have already consecrated the ground here!");
      return eSUCCESS;
    }

    if (cItem->obj_flags.value[0] == SPELL_DESECRATE)
    {
      ch->send("A foul taint prevents you from consecrating the ground here!");
      return eSUCCESS;
    }
    if (cItem->obj_flags.value[0] == SPELL_CONSECRATE)
    {
      ch->send("The ground here has already been consecrated!");
      return eSUCCESS;
    }
  }

  if (IS_MORTAL(ch))
  {
    component->obj_flags.value[1]--;
    ch->decrementMove(100);
  }

  act(
      "You chant softy, etching runes upon the ground in a large circle and sprinkle them with holy water.",
      ch, 0, 0, TO_CHAR, 0);
  act(
      "$n chants softy, etching runes upon the ground in a large circle and sprinkling them with holy water.",
      ch, 0, 0, TO_ROOM, 0);

  if (ch->cRooms >= 1 + skill / 25)
  {
    ch->sendln("You cannot keep up this many consecrated areas.");
    return eSUCCESS;
  }

  send_to_room(
      "The runes begin to glow brightly upon the ground, then softly fade from view.\r\n",
      ch->in_room);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  ch->cRooms++;

  cItem = clone_object(real_object(CONSECRATE_OBJ_NUMBER));
  if (cItem == nullptr)
  {
    ch->sendln("Consecrate item doesn't exist. Tell an imm.");
    return eFAILURE;
  }
  cItem->obj_flags.value[0] = spl;
  cItem->obj_flags.value[1] = 2 + skill / 50;
  cItem->obj_flags.value[2] = skill;
  cItem->obj_flags.value[3] = {};
  cItem->obj_flags.origin = ch;

  obj_to_room(cItem, ch->in_room);

  return eSUCCESS;
}

int cast_consecrate(uint8_t level, Character *ch, char *arg, int type,
                    Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_consecrate(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_consecrate(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    return spell_consecrate(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    return spell_consecrate(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in consecrate!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }

  return eFAILURE;
}

/* DESECRATE */

int spell_desecrate(uint8_t level, Character *ch, Character *victim,
                    class Object *obj, int skill)
{
  int spl = SPELL_DESECRATE;
  int compNum = DESECRATE_COMP_OBJ_NUMBER;
  char buf[MAX_STRING_LENGTH];
  Object *component = nullptr;

  if (IS_MORTAL(ch))
  {
    if (!(component = get_obj_in_list_vis(ch, compNum, ch->carrying)))
    {
      component = ch->equipment[WEAR_HOLD];
      if ((component == 0) || (compNum != DC::getInstance()->obj_index[component->item_number].virt))
      {
        component = ch->equipment[WEAR_HOLD2];
        if ((component == 0) || (compNum != DC::getInstance()->obj_index[component->item_number].virt))
        {
          ch->sendln("You do not have the required components.");
          return eFAILURE;
        }
      }
    }

    if (component->obj_flags.value[1] <= 0)
    {
      ch->sendln("There is nothing left in the container.");
      return eFAILURE;
    }

    if (component->obj_flags.value[2] != 13 && component->obj_flags.value[2] != 17)
    {
      ch->sendln("You do not have the required components,");
      return eFAILURE;
    }

    if (ch->getMove() < 100)
    {
      ch->sendln("You do not have enough energy to complete the incantation.");
      return eFAILURE;
    }
  }
  else if (IS_IMMORTAL(ch))
  {
    ch->send("You manifest the missing components.\r\n");
  }

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, CLAN_ROOM) ||
      isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) ||
      isSet(DC::getInstance()->world[ch->in_room].room_flags, NOLEARN))
  {
    if (IS_MORTAL(ch))
    {
      ch->sendln("Something about this room prohibits your incantation from being completed.");
      return eSUCCESS;
    }
    else if (IS_IMMORTAL(ch))
    {
      ch->send("You bypass the room restrictions.\r\n");
    }
  }

  if (ch->cRooms >= 1 + skill / 25)
  {
    ch->sendln("You cannot keep up this many desecrated areas.");
    return eSUCCESS;
  }

  Object *cItem = nullptr;
  if ((cItem = get_obj_in_list("consecrateitem", DC::getInstance()->world[ch->in_room].contents)))
  {
    if (ch == ((Character *)(cItem->obj_flags.origin)))
    {
      ch->sendln("You have already desecrated the ground here!");
      return eSUCCESS;
    }

    if (cItem->obj_flags.value[0] == SPELL_CONSECRATE)
    {
      ch->send("A powerful aura of goodness prevents you from desecrating the ground here!");
      return eSUCCESS;
    }
    if (cItem->obj_flags.value[0] == SPELL_DESECRATE)
    {
      ch->send("The ground here has already been desecrated!");
      return eSUCCESS;
    }
  }

  if (IS_MORTAL(ch))
  {
    component->obj_flags.value[1]--;
    ch->decrementMove(100);
  }

  act(
      "You chant darkly, etching a large circle of runes upon the ground in blood.",
      ch, 0, 0, TO_CHAR, 0);
  act(
      "$n chants darkly, etching a large circle of runes upon the ground in blood.",
      ch, 0, 0, TO_ROOM, 0);

  send_to_room(
      "The runes begin to hum ominously, then softly fade from view.\r\n",
      ch->in_room);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);

  ch->cRooms++;

  cItem = clone_object(real_object(CONSECRATE_OBJ_NUMBER));
  if (!cItem)
  {
    ch->sendln("Consecrate item doesn't exist. Tell an imm.");
    return eFAILURE;
  }
  sprintf(buf, "%s",
          "A circle of ominously humming blood runes are etched upon the ground here.");
  cItem->long_description = str_hsh(buf);
  cItem->obj_flags.value[0] = spl;
  cItem->obj_flags.value[1] = 2 + skill / 50;
  cItem->obj_flags.value[2] = skill;
  cItem->obj_flags.value[3] = {};
  cItem->obj_flags.origin = ch;

  obj_to_room(cItem, ch->in_room);

  return eSUCCESS;
}

int cast_desecrate(uint8_t level, Character *ch, char *arg, int type,
                   Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_desecrate(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_POTION:
    return spell_desecrate(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_SCROLL:
    if (tar_obj)
      return eFAILURE;
    return spell_desecrate(level, ch, 0, 0, skill);
    break;
  case SPELL_TYPE_WAND:
    if (tar_obj)
      return eFAILURE;
    return spell_desecrate(level, ch, 0, 0, skill);
    break;
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in desecrate!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }

  return eFAILURE;
}

/* ELEMENTAL_WALL */

int spell_elemental_wall(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  ch->sendln("Pirahna is still working on this.");
  return eFAILURE;
}

int cast_elemental_wall(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_elemental_wall(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_WAND:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_POTION:
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in elemental_wall!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}

/* ETHEREAL FOCUS */

int spell_ethereal_focus(uint8_t level, Character *ch, Character *victim, class Object *obj, int skill)
{
  struct affected_type af;
  Character *ally, *next_ally;

  // Set the spell on the caster to mark that they have the spell running
  if (ch->affected_by_spell(SPELL_ETHEREAL_FOCUS))
    affect_from_char(ch, SPELL_ETHEREAL_FOCUS);

  af.type = SPELL_ETHEREAL_FOCUS;
  af.duration = 1;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;

  affect_to_char(ch, &af);

  ch->sendln("You focus the minds of your allies to react to the slightest movement...");
  act("$n's magic attempts to focus $s allies minds into a unified supernatural focus...", ch, 0, 0, TO_ROOM, INVIS_NULL);
  // loop through group members in room
  for (ally = DC::getInstance()->world[ch->in_room].people; ally; ally = next_ally)
  {
    next_ally = ally->next_in_room;

    if (IS_PC(ally) && GET_POS(ally) > position_t::SLEEPING &&
        (ally->master == ch || ch->master == ally || (ch->master && ch->master == ally->master)))
    {
      ally->sendln("Your mind's eye focuses, you still your body, and poise yourself to react to anything!");
    }
    // TODO if not toggle set
    //    send_to_char("You refuse to be affected by %s's magic but you probably shouldn't move or do anything right now.\r\n"
  }

  // Lastly, set the current room to flag that someone in it is using ethereal_focus
  // We do this last because this is the trigger for the spell.  If we did this before the act() call we would trigger it
  // on ourself immediately which would probably make the spell not very useful.
  // NOTICE:  This is a TEMP_room_flag
  SET_BIT(DC::getInstance()->world[ch->in_room].temp_room_flags, ROOM_ETHEREAL_FOCUS);

  return eSUCCESS;
}

int cast_ethereal_focus(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  switch (type)
  {
  case SPELL_TYPE_SPELL:
    return spell_ethereal_focus(level, ch, tar_ch, 0, skill);
    break;
  case SPELL_TYPE_POTION:
  case SPELL_TYPE_SCROLL:
  case SPELL_TYPE_WAND:
  default:
    DC::getInstance()->logentry(QStringLiteral("Serious screw-up in ethereal_focus!"), ANGEL, DC::LogChannel::LOG_BUG);
    break;
  }
  return eFAILURE;
}
