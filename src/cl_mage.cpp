/************************************************************************
| cl_mage.C
| Description:  Commands for the mage class.
*/
#include <cstring>

#include "DC/obj.h"
#include "DC/structs.h"
#include "DC/player.h"
#include "DC/character.h"
#include "DC/spells.h"
#include "DC/utility.h"
#include "DC/fight.h"
#include "DC/mobile.h"
#include "DC/connect.h"
#include "DC/handler.h"
#include "DC/act.h"
#include "DC/interp.h"
#include "DC/returnvals.h"
#include "DC/room.h"
#include "DC/db.h"

extern struct spell_info_type spell_info[MAX_SPL_LIST];

int spellcraft(Character *ch, int spell)
{
  int a = ch->has_skill(SKILL_SPELLCRAFT);
  if (!a)
    return false;
  if (ch->has_skill(spell) < 71)
    return false;
  if (spell == SPELL_MAGIC_MISSILE)
  {
    if (a < 11)
      ch->skill_increase_check(SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_BURNING_HANDS && a > 10)
  {
    if (a < 21)
      ch->skill_increase_check(SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_LIGHTNING_BOLT && a > 20)
  {
    if (a < 31)
      ch->skill_increase_check(SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_CHILL_TOUCH && a > 30)
  {
    if (a < 41)
      ch->skill_increase_check(SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_FIREBALL && a > 40)
  {
    if (a < 51)
      ch->skill_increase_check(SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_METEOR_SWARM && a > 50)
  {
    if (a < 61)
      ch->skill_increase_check(SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_PARALYZE && a > 60)
  {
    if (a < 71)
      ch->skill_increase_check(SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_CREATE_GOLEM && a > 70)
  {
    if (a < 81)
      ch->skill_increase_check(SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_HELLSTREAM && a > 80)
  {
    if (a < 91)
      ch->skill_increase_check(SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_SOLAR_GATE && a > 90)
  {
    if (a < 100)
      ch->skill_increase_check(SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }

  return false;
}

int do_focused_repelance(Character *ch, char *argument, int cmd)
{
  // uint8_t percent;
  struct affected_type af;
  int duration = 40;

  if (!ch->canPerform(SKILL_FOCUSED_REPELANCE, "You wish really really hard that magic couldn't hurt you....\r\n"))
  {
    return eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_FOCUSED_REPELANCE))
  {
    ch->sendln("Your mind can not yet take the strain of another repelance.");
    return eFAILURE;
  }

  if (!skill_success(ch, nullptr, SKILL_FOCUSED_REPELANCE))
  {
    act("$n closes $s eyes and chants quietly, $s head shakes suddenly in confusion.",
        ch, nullptr, nullptr, TO_ROOM, NOTVICT);
    ch->sendln("Your mind cannot handle the strain!");
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2);
    duration = 20 - (ch->has_skill(SKILL_FOCUSED_REPELANCE) / 10);
  }
  else
  {
    act("$n closes $s eyes and chants quietly, $s eyes flick open with a devilish smile.",
        ch, nullptr, nullptr, TO_ROOM, NOTVICT);
    send_to_char("Your mystical vision is clear, your senses of the arcane sharpened.  "
                 "No mortal can break through _your_ magical barrier.\r\n",
                 ch);
    SET_BIT(ch->combat, COMBAT_REPELANCE);
    duration = 40 - (ch->has_skill(SKILL_FOCUSED_REPELANCE) / 10);
  }

  af.type = SKILL_FOCUSED_REPELANCE;
  af.duration = duration;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;

  affect_to_char(ch, &af);

  return eSUCCESS;
}

int do_imbue(Character *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  int lvl = ch->has_skill(SKILL_IMBUE);
  int charges = 0, manacost = 0;
  Object *wand;
  struct affected_type af;

  *buf = '\0';

  argument = one_argument(argument, buf);

  if (!lvl)
  {
    ch->sendln("The best you can do to a wand is polish it.");
    return eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_IMBUE))
  {
    ch->sendln("Your mind has not yet recovered from the previous imbuement.");
    return eFAILURE;
  }

  if (*buf == '\0')
  {
    ch->sendln("Imbue what?");
    return eFAILURE;
  }

  if (ch->in_room && (isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_MAGIC) || isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE)))
  {
    ch->sendln("Something about this room prohibits your magical imbuement.");
    return eFAILURE;
  }

  if (!(wand = get_obj_in_list_vis(ch, buf, ch->carrying)))
  {
    wand = ch->equipment[HOLD];
    if ((wand == 0) || !isexact(buf, wand->name))
    {
      wand = ch->equipment[HOLD2];
      if ((wand == 0) || !isexact(buf, wand->name))
      {
        ch->sendln("You do not have that wand.");
        return eFAILURE;
      }
    }
  }

  if (GET_ITEM_TYPE(wand) != ITEM_WAND)
  {
    ch->sendln("That \"wand\" doesn't seem very wand-like.");
    return eFAILURE;
  }

  if (ch->getLevel() < wand->obj_flags.value[0])
  {
    ch->sendln("This wand is too powerful for you to imbue.");
    return eFAILURE;
  }

  manacost = 4 + (11 - lvl / 10) * spell_info[wand->obj_flags.value[3]].min_usesmana;

  if (GET_MANA(ch) < manacost)
  {
    ch->sendln("You do not have enough magical energy to imbue this wand.");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_IMBUE))
    return eFAILURE;

  GET_MANA(ch) -= manacost;

  charges = number(1, 1 + lvl / 20);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2.5);

  af.type = SKILL_IMBUE;
  af.duration = 2;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;

  affect_to_char(ch, &af);

  // value[2] == current charges
  // value[1] == total charges

  if (skill_success(ch, 0, SKILL_IMBUE))
  {                                // success
    wand->obj_flags.value[1] -= 1; // reduce total by 1

    if (wand->obj_flags.value[1] == 0) // no total charges left
    {
      act("Unable to bear the strain, $p splits asunder with a sharp crack!", ch, wand, 0, TO_CHAR, 0);
      act("Unable to bear the strain, $n's $p splits asunder with a sharp crack!", ch, wand, 0, TO_ROOM, 0);
      make_scraps(ch, wand);
      extract_obj(wand);
      return eSUCCESS;
    }

    wand->obj_flags.value[2] += charges; // refill charges
    if (wand->obj_flags.value[2] >= wand->obj_flags.value[1])
    {
      wand->obj_flags.value[2] = wand->obj_flags.value[1];
      act("You focus your arcane powers and imbue them into $p restoring its full charge!", ch, wand, 0, TO_CHAR, 0);
    }
    else
    {
      sprintf(buf, "You focus your arcane powers and imbue them into $p restoring %d charges!", charges);
      act(buf, ch, wand, 0, TO_CHAR, 0);
    }

    ch->sendln("As you finish, the tip of the freshly charged wand $Bglows$R briefly and returns to normal.");
    act("$n focuses $s arcane powers and imbues them into $p!", ch, wand, 0, TO_ROOM, 0);
    act("As $e finishes, the tip of the freshly charged wand $Bglows$R briefly and returns to normal.", ch, wand, 0, TO_ROOM, 0);
  }
  else
  { // failure
    act("You focus your arcane powers on $p but fail to restore its powers.", ch, wand, 0, TO_CHAR, 0);
    act("$n focuses $s arcane powers on $p to no effect.", ch, wand, 0, TO_ROOM, 0);

    if (wand->obj_flags.value[2] == 0) // no current charges left
    {
      act("Unable to bear the strain, $p splits asunder with a sharp crack!", ch, wand, 0, TO_CHAR, 0);
      act("Unable to bear the strain, $n's $p splits asunder with a sharp crack!", ch, wand, 0, TO_ROOM, 0);
      make_scraps(ch, wand);
      extract_obj(wand);
      return eSUCCESS;
    }

    wand->obj_flags.value[2] -= charges;
    if (wand->obj_flags.value[2] <= 0)
    {
      wand->obj_flags.value[2] = 0;
      act("The energy in $p has been completely lost!", ch, wand, 0, TO_CHAR, 0);
    }
    else
      act("Some of the energy in $p has been lost!", ch, wand, 0, TO_CHAR, 0);
  }
  return eSUCCESS;
}

// Okay, if we enter this function, we came here either from do_move because I entered a room
// or because of an act() call, meaning a made a 'noise'.
// This is to check if ethereal focus should fire and handle the ramifications
// Remember that ch is the person triggering the call, meaning they are actually the victim
// eSUCCESS means the character is unaffected and can keep doing whatever.
// eFAILURE means the character was interrupted
int check_ethereal_focus(Character *ch, int trigger_type)
{
  Character *i, *next_i, *ally, *next_ally;
  char buf[MAX_STRING_LENGTH];
  int retval;

  // Moving, and act() calls both happen a lot, so we want to get out of here as fast as possible if
  // we can.  We do this by checking if the room has a flag or not
  // NOTICE:  This is a TEMP_room_flag
  if (!isSet(DC::getInstance()->world[ch->in_room].temp_room_flags, ROOM_ETHEREAL_FOCUS))
    return eSUCCESS;

  // loop through the room to find the caster. It should only be possible for a single
  // caster in a room to have this running (as long as no imms are being stupid)
  for (i = DC::getInstance()->world[ch->in_room].people; i; i = next_i)
  {
    next_i = i->next_in_room;

    // Only the caster should have the spell
    if (!i->affected_by_spell(SPELL_ETHEREAL_FOCUS))
      continue;

    // If I can't see my target, then I can't react, better keep true-sight up
    if (!CAN_SEE(i, ch))
      continue;

    // Okay, something is going down no matter what, so let's remove the flags first to avoid any cascading effects
    // NOTICE:  This is a TEMP_room_flag
    REMOVE_BIT(DC::getInstance()->world[ch->in_room].temp_room_flags, ROOM_ETHEREAL_FOCUS);
    affect_from_char(i, SPELL_ETHEREAL_FOCUS);

    if (IS_PC(i) && !i->desc) // don't work if I'm linkdead
      break;

    // If for some reason the caster is busy, the spell fails.
    if (i->getPosition() <= position_t::RESTING ||
        i->getPosition() == position_t::FIGHTING || i->fighting ||
        IS_AFFECTED(i, AFF_PARALYSIS) ||
        (isSet(DC::getInstance()->world[i->in_room].room_flags, SAFE) && !ch->isPlayerCantQuit()))
    {
      sprintf(buf, "I see you %s but I can't do anything about it!", GET_SHORT(ch));
      do_say(i, buf, CMD_DEFAULT);
      break;
    }

    if (i == ch)
    {
      do_say(i, "Wait, I didn't mean to move .... crap.", CMD_DEFAULT);
    }
    else
    {
      sprintf(buf, "I see movement!!!  It's %s!", IS_NPC(ch) ? GET_SHORT(ch) : GET_NAME(ch));
      do_say(i, buf, CMD_DEFAULT);
      set_fighting(i, ch);
      set_fighting(ch, i);
      retval = attack(i, ch, TYPE_UNDEFINED);
      if (isSet(retval, eVICT_DIED))
        return (eFAILURE | eCH_DIED); // dead target, so spell ends
      if (isSet(retval, eCH_DIED))
        return (eSUCCESS); // caster died, so spell ends, target gets no lag and can keep going
      retval = eFAILURE;
    }
    WAIT_STATE(i, DC::PULSE_VIOLENCE * 2);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1);

    // Loop through allies and attack
    for (ally = DC::getInstance()->world[ch->in_room].people; ally; ally = next_ally)
    {
      next_ally = ally->next_in_room;

      // Skip anyone unable to fight
      // Note that since they are joining the mage here, we don't check CAN_SEE.  Magical join!
      if (ally == ch || ally == i || ally->fighting || ally->getPosition() != position_t::STANDING ||
          (IS_PC(ally) && !ally->desc) // linkdead groupies won't help
      )
        continue;
      // TODO - skip anyone with this toggle turned off

      // Skip anyone that isn't a group member, or following me
      if (ally == i->master ||                        // if they are the caster's group leader
          ally->master == i ||                        // or the caster is their group leader
          (ally->master && ally->master == i->master) // or we share the same group leader
      )
      {
        act("$n's magically sharpened reflexes direct attacks at you as you enter the room!", i, 0, ch, TO_VICT, 0);
        act("You attack $N with supernaturally focused reflexes!", ally, 0, ch, TO_CHAR, 0);
        WAIT_STATE(ally, DC::PULSE_VIOLENCE * 2);

        if (trigger_type == ETHEREAL_FOCUS_TRIGGER_MOVE || trigger_type == ETHEREAL_FOCUS_TRIGGER_SOCIAL)
        {
          // Get um!
          sprintf(buf, "I see movement!!!  It's %s!", IS_NPC(ch) ? GET_SHORT(ch) : GET_NAME(ch));
          do_say(ally, buf, CMD_DEFAULT);
          set_fighting(ally, ch);
          set_fighting(ch, ally);
          retval = attack(ally, ch, TYPE_UNDEFINED);
          if (isSet(retval, eVICT_DIED))
            return (eFAILURE | eCH_DIED); // ch = damage vict, return since they are dead
        }
        else
        { // trigger_type == ETHEREAL_FOCUS_TRIGGER_ACT
          // TODO - i don't currently do this anywhere because of the concerns below
          // In the case of an act(), we can't return properly if someone dies, which would probably be fine, but could
          // also cause crashes all over the mud from people no longer being where they were (or existing in memory).
          // Because of this, we will instead just set them fighting, and still lag them
          set_fighting(ally, ch);
          set_fighting(ch, ally);
        }
      }
    } // loop allies
  } // loop looking for caster

  if (ch->fighting)
    return eFAILURE;

  // If I made it through and no one was able to attack me for whatever reason, I'm good
  return eSUCCESS;
}
