/************************************************************************
| cl_mage.C
| Description:  Commands for the mage class.
*/
#include <structs.h>
#include <player.h>
#include <levels.h>
#include <character.h>
#include <spells.h>
#include <utility.h>
#include <fight.h>
#include <mobile.h>
#include <connect.h>
#include <handler.h>
#include <act.h>
#include <interp.h>
#include <returnvals.h>

int spellcraft(struct char_data *ch, int spell)
{
  int a = has_skill(ch, SKILL_SPELLCRAFT);
  if (!a) return FALSE;
  if (has_skill(ch, spell) < 71) return FALSE;
  if (spell == SPELL_MAGIC_MISSILE)
  {
    if (a < 11) skill_increase_check(ch, SKILL_SPELLCRAFT, a, SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_BURNING_HANDS && a > 10)
  {
    if (a < 21) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_LIGHTNING_BOLT && a > 20)
  {
    if (a < 31) skill_increase_check(ch, SKILL_SPELLCRAFT, a,
SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_CHILL_TOUCH && a > 30)
  {
    if (a < 41) skill_increase_check(ch, SKILL_SPELLCRAFT, a,
SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_FIREBALL && a> 40)
  {
    if (a < 51) skill_increase_check(ch, SKILL_SPELLCRAFT, a,
SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_METEOR_SWARM && a > 50)
  {
    if (a < 61) skill_increase_check(ch, SKILL_SPELLCRAFT, a,
SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_PARALYZE && a > 60)
  {
    if (a < 71) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_CREATE_GOLEM && a > 70)
  {
    if (a < 91) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_SOLAR_GATE && a > 90)
  {
    if (a < 98) skill_increase_check(ch, SKILL_SPELLCRAFT, a,
SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_HELLSTREAM && a > 80)
  {
    skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  return FALSE;
}

int do_focused_repelance(struct char_data *ch, char *argument, int cmd)
{
  //ubyte percent;
  struct affected_type af;
  int duration = 40;
  if(IS_MOB(ch))
    ;
  else if(!has_skill(ch, SKILL_FOCUSED_REPELANCE)) {
    send_to_char("You wish really really hard that magic couldn't hurt you....\r\n", ch);
    return eFAILURE;
  }

  if(affected_by_spell(ch, SKILL_FOCUSED_REPELANCE)) {
    send_to_char("Your mind can not yet take the strain of another repelance.\r\n", ch);
    return eFAILURE;
  }

  if (!skill_success(ch,NULL,SKILL_FOCUSED_REPELANCE)) 
  {
    act("$n closes $s eyes and chants quietly, $s head shakes suddenly in confusion.",
         ch, NULL, NULL, TO_ROOM, NOTVICT);
    send_to_char("Your mind cannot handle the strain!\r\n", ch);
    WAIT_STATE(ch, PULSE_VIOLENCE*2);
    duration = 20 - (has_skill(ch, SKILL_FOCUSED_REPELANCE) / 10);
  }
  else 
  {
    act( "$n closes $s eyes and chants quietly, $s eyes flick open with a devilish smile.",
	  ch, NULL, NULL, TO_ROOM, NOTVICT );
    send_to_char("Your mystical vision is clear, your senses of the arcane sharpened.  " 
                 "No mortal can break through _your_ magical barrier.\r\n", ch);
    SET_BIT(ch->combat, COMBAT_REPELANCE);
    duration = 40 - (has_skill(ch, SKILL_FOCUSED_REPELANCE)/10);
  }

  af.type      = SKILL_FOCUSED_REPELANCE;
  af.duration  = duration;
  af.modifier  = 0;
  af.location  = 0;
  af.bitvector = -1;

  affect_to_char(ch, &af);

  return eSUCCESS;
}
