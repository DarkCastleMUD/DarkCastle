/************************************************************************
| cl_warrior.C
| Description:  This file declares implementation for (anti)paladin-specific
|   skills.
|
| File create with do_layhands -Pirahna 7/6/1999
*/
#include "structs.h"
#include "character.h"
#include "player.h"
#include "fight.h"
#include "utility.h"
#include "spells.h"
#include "handler.h"
#include "levels.h"
#include "connect.h"
#include "mobile.h"
#include "room.h"
#include "act.h"
#include "db.h"
#include "returnvals.h"
#include "interp.h"



/************************************************************************
| OFFENSIVE commands.  These are commands that should require the
|   victim to retaliate.
*/

// Note that most of the (anti)paladin skills are already in "cl_warrior.C"

int do_harmtouch(Character *ch, char *argument, int cmd)
{
  Character *victim;
  // Character *tmp_ch;
  char victim_name[MAX_INPUT_LENGTH];
  struct affected_type af;
  int retval = eSUCCESS, dam;

  one_argument(argument, victim_name);

  if (!ch->canPerform(SKILL_HARM_TOUCH, "You dunno even HOW to harm touch.\r\n"))
  {
    return eFAILURE;
  }

  if (!(victim = ch->get_char_room_vis( victim_name)))
  {
    victim = ch->fighting;
    if (!victim)
    {
      ch->sendln("Whom do you want to harmtouch?");
      return eFAILURE;
    }
  }

  if (victim == ch)
  {
    if (GET_SEX(ch) == SEX_MALE)
      ch->sendln("You'd wither it!");
    else if (GET_SEX(ch) == SEX_FEMALE)
      ch->sendln("You naughty naughty girl...at least wait until someone's filming.");
    else
      ch->sendln("Looks like you've already harm touched yourself...");
    return eFAILURE;
  }

  if (ch->affected_by_spell(SKILL_HARM_TOUCH) && ch->getLevel()<= IMMORTAL)
  {
    ch->sendln("You have not spend enough time in devotion to your god to warrant such a favor yet.");
    return eFAILURE;
  }

  if (ch->getHP() < GET_MAX_HIT(ch) / 4)
  {
    ch->sendln("You don't posess the energy to do it!");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_HARM_TOUCH))
    return eSUCCESS;

  int duration = 24;
  if (!skill_success(ch, victim, SKILL_HARM_TOUCH))
  {
    ch->sendln("Your god refuses you.");
    duration = 1;
    WAIT_STATE(ch, DC::PULSE_VIOLENCE / 2 + number((quint64)1, (quint64)DC::PULSE_VIOLENCE / 2));
  }
  else
  {
    dam = 750;
    retval = damage(ch, victim, dam, TYPE_ACID, SKILL_HARM_TOUCH, 0);
    WAIT_STATE(ch, DC::PULSE_VIOLENCE);
    if (isSet(retval, eVICT_DIED) && !isSet(retval, eCH_DIED))
    {
      if (ch->has_skill( SKILL_HARM_TOUCH) > 30 && number(1, 3) == 1)
      {
        char dammsg[MAX_STRING_LENGTH];
        int amount = ch->getLevel() * 10;
        if (amount + ch->getHP() > GET_MAX_HIT(ch))
          amount = GET_MAX_HIT(ch) - ch->getHP();
        sprintf(dammsg, "$B%d$R", amount);
        send_damage("Your god basks in your worship of pain and infuses you with | life.", ch, 0, victim, dammsg, "You god basks in your worship of pain and infuses you with life.", TO_CHAR);
        ch->addHP(amount);
      }
    }
  }
  af.type = SKILL_HARM_TOUCH;
  af.duration = duration;
  af.modifier = 0;
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

int do_layhands(Character *ch, char *argument, int cmd)
{
  Character *victim;
  // Character *tmp_ch;
  char victim_name[240];
  struct affected_type af;
  int duration = 24;
  one_argument(argument, victim_name);

  if (!ch->canPerform(SKILL_LAY_HANDS, "You aren't skilled enough to lay a two-dollar whore with three bucks.\r\n"))
  {
    return eFAILURE;
  }

  if (!(victim = ch->get_char_room_vis( victim_name)))
  {
    ch->sendln("Whom do you want to layhands on?");
    return eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln("Oh yeah...that's really holy....pervert...");
    return eFAILURE;
  }

  //   if (ch->fighting == victim) {
  //     ch->sendln("Aren't you a little busy trying to KILL them right now?");
  //     return eFAILURE;
  //   }

  if (ch->affected_by_spell(SKILL_LAY_HANDS))
  {
    ch->sendln("You have not spent enough time in devotion to your god to warrant such a favor yet.");
    return eFAILURE;
  }

  if (ch->getHP() < GET_MAX_HIT(ch) / 4)
  {
    ch->sendln("You don't posess the energy to do it!");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_LAY_HANDS))
    return eSUCCESS;

  if (!skill_success(ch, victim, SKILL_LAY_HANDS))
  {
    ch->sendln("Your god refuses you.");
    duration = 1;
  }
  else
  {
    char dammsg[MAX_STRING_LENGTH];
    int amount = 500 + (ch->has_skill( SKILL_LAY_HANDS) * 10);
    if (amount + victim->getHP() > GET_MAX_HIT(victim))
      amount = GET_MAX_HIT(victim) - victim->getHP();
    victim->addHP(amount);
    sprintf(dammsg, "$B%d$R", amount);
    send_damage("Praying fervently, you lay hands as life force granted by your god streams from your body healing $N for | health.",
                ch, 0, victim, dammsg, "Praying fervently, you lay hands as life force granted by your god streams from your body into $N.", TO_CHAR);
    send_damage("Your body surges with holy energies as | points of life force granted by $n's god pours into you!",
                ch, 0, victim, dammsg, "Your body surges with holy energies as life force granted by $n's god pours into you!", TO_VICT);
    send_damage("A blinding flash fills the area as | points of life force granted from $n's god pours into $N!", ch, 0,
                victim, dammsg, "A blinding flash fills the area as life force granted from $n's god pours into $N!", TO_ROOM);
  }

  af.type = SKILL_LAY_HANDS;
  af.duration = duration;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  return eSUCCESS;
}

int do_behead(Character *ch, char *argument, int cmd)
{
  double modifier = 0.0;
  double enemy_hp = 0.0;
  int chance = 0;
  int retval = eSUCCESS;
  char buf[MAX_STRING_LENGTH];
  Character *vict;
  extern struct index_data *obj_index;

  one_argument(argument, buf);

  if (!ch->canPerform(SKILL_BEHEAD, "The closest you'll ever get to 'beheading' is at a brit milah. Mazal tov!\r\n"))
  {
    return eFAILURE;
  }

  if (!ch->equipment[WIELD] || !isSet(ch->equipment[WIELD]->obj_flags.extra_flags, ITEM_TWO_HANDED) || (ch->equipment[WIELD]->obj_flags.value[3] != 3)) // TYPE_SLASH
  {
    ch->sendln("You need to be wielding a two handed sword to behead!");
    return eFAILURE;
  }

  if (!(vict = ch->get_char_room_vis( buf)))
  {
    if (ch->fighting)
      vict = ch->fighting;
    else
    {
      ch->sendln("Whom do you want behead?");
      return eFAILURE;
    }
  }

  if (!can_attack(ch) || !can_be_attacked(ch, vict))
    return eFAILURE;

  if (isSet(vict->combat, COMBAT_BLADESHIELD1) || isSet(vict->combat, COMBAT_BLADESHIELD2))
  {
    ch->sendln("You can't behead a bladeshielded opponent!");
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BEHEAD))
    return eSUCCESS;

  WAIT_STATE(ch, (int)(DC::PULSE_VIOLENCE));

  if (!skill_success(ch, vict, SKILL_BEHEAD))
  {
    ch->sendln("Your mighty swing goes wild!");
    act("$n takes a mighty swing at your head, but it goes wild!", ch, 0, vict, TO_VICT, 0);
    act("$n takes a mighty swing at $n's head, but it goes wild!", ch, 0, vict, TO_ROOM, NOTVICT);
    retval = one_hit(ch, vict, SKILL_BEHEAD, FIRST);
    return retval;
  }

  int skill_level = ch->has_skill( SKILL_BEHEAD);
  modifier = 50.0 + skill_level / 2.0 + GET_ALIGNMENT(ch) / 100.0;
  modifier /= 100.0; // range .15-1.0

  enemy_hp = (vict->getHP() * 100.0) / GET_MAX_HIT(vict);
  enemy_hp /= 100.0; // range 0-1;

  if (enemy_hp <= 0)
    enemy_hp = 0.01;

  chance = (int)(modifier / (enemy_hp * enemy_hp));

  if (enemy_hp < 0.3) // covered is 0.3
  {
    chance += (ch->has_skill( SKILL_TWO_HANDED_WEAPONS) / 6);
    // csendf(ch, "BEHEAD chance increased by %d\r\n", ch->has_skill( SKILL_TWO_HANDED_WEAPONS) / 6);
  }
  else
    chance >>= 1; // halving the chance if less than covered (nerf)

  if (chance > 85)
    chance = 85;

  if (chance < 0)
    chance = 0;

  // csendf(ch, "behead chance: %d, enemy hp%: %f\r\n", chance, enemy_hp);

  if ((number(0, 99) < chance) && !isSet(vict->immune, ISR_SLASH) && !isSet(vict->immune, ISR_PHYSICAL))
  {
    if ((
            (vict->equipment[WEAR_NECK_1] && obj_index[vict->equipment[WEAR_NECK_1]->item_number].virt == 518) || (vict->equipment[WEAR_NECK_2] && obj_index[vict->equipment[WEAR_NECK_2]->item_number].virt == 518)) &&
        !number(0, 1))
    { // tarrasque's leash..
      act("You attempt to behead $N, but your sword bounces of $S neckwear.", ch, 0, vict, TO_CHAR, 0);
      act("$n attempts to behead $N, but fails.", ch, 0, vict, TO_ROOM, NOTVICT);
      act("$n attempts to behead you, but cannot cut through your neckwear.", ch, 0, vict, TO_VICT, 0);
      retval = damage(ch, vict, 0, TYPE_SLASH, SKILL_BEHEAD, 0);
      return eSUCCESS | retval;
    }

    if (IS_AFFECTED(vict, AFF_NO_BEHEAD))
    {
      act("$N deftly dodges your beheading attempt!", ch, 0, vict, TO_CHAR, 0);
      act("$N deftly dodges $n's attempt to behead $M!", ch, 0, vict, TO_ROOM, NOTVICT);
      act("You deftly avoid $n's attempt to lop your head off!", ch, 0, vict, TO_VICT, 0);
      retval = damage(ch, vict, 0, TYPE_SLASH, SKILL_BEHEAD, 0);
      return eSUCCESS | retval;
    }

    act("You feel your life end as $n's sword SLICES YOUR HEAD OFF!", ch, 0, vict, TO_VICT, 0);
    act("You SLICE $N's head CLEAN OFF $S body!", ch, 0, vict, TO_CHAR, 0);
    act("$n cleanly slices $N's head off $S body!", ch, 0, vict, TO_ROOM, NOTVICT);

    vict->setHP(-20);
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
    retval = damage(ch, vict, 0, TYPE_SLASH, SKILL_BEHEAD, 0);
  }

  return retval;
}
