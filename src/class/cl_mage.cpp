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


int do_focused_repelance(struct char_data *ch, char *argument, int cmd)
{
  byte percent;
  int learned, chance, specialization;
  struct affected_type af;

  if(IS_MOB(ch))
    learned = 75;
  else if(!(learned = has_skill(ch, SKILL_FOCUSED_REPELANCE))) {
    send_to_char("You wish really really hard that magic couldn't hurt you....\r\n", ch);
    return eFAILURE;
  }

  if(affected_by_spell(ch, SKILL_FOCUSED_REPELANCE)) {
    send_to_char("Your mind can not yet take the strain of another repelance.\r\n", ch);
    return eFAILURE;
  }

  specialization = learned / 100;
  learned %= 100;

  chance = 58;
  chance += learned / 10;
  chance += GET_INT(ch)/2;

  // 101% is a complete failure
  percent = number(1, 101);

  if (GET_LEVEL(ch) >= IMMORTAL)
    percent = 1;

  if (percent > learned) 
  {
    act("$n closes $s eyes and chants quietly, $s head shakes suddenly in confusion.",
         ch, NULL, NULL, TO_ROOM, NOTVICT);
    send_to_char("Your mind cannot handle the strain!\r\n", ch);
    WAIT_STATE(ch, PULSE_VIOLENCE*2);
  }
  else 
  {
    act( "$n closes $s eyes and chants quietly, $s eyes flick open with a devilish smile.",
	  ch, NULL, NULL, TO_ROOM, NOTVICT );
    send_to_char("Your mystical vision is clear, your senses of the arcane sharpened.  " 
                 "No mortal can break through _your_ magical barrier.\r\n", ch);
    SET_BIT(ch->combat, COMBAT_REPELANCE);
  }

  af.type      = SKILL_FOCUSED_REPELANCE;
  af.duration  = 40 - (learned/10);
  af.modifier  = 0;
  af.location  = 0;
  af.bitvector = 0;

  affect_to_char(ch, &af);

  skill_increase_check(ch, SKILL_FOCUSED_REPELANCE, learned, SKILL_INCREASE_EASY);

  return eSUCCESS;
}
