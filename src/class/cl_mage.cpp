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
  byte learned;
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
                 "No mortal can break through _your_ magical barriers.\r\n", ch);
    GET_HIT(ch) += GET_MAX_HIT(ch)/2;
    if(GET_HIT(ch) > GET_MAX_HIT(ch))
      GET_HIT(ch) = GET_MAX_HIT(ch);
  }

  af.type      = SKILL_FOCUSED_REPELANCE;
  af.duration  = 2;
  af.modifier  = 0;
  af.location  = 0;
  af.bitvector = 0;

  affect_to_char(ch, &af);

  SET_BIT(ch->combat, COMBAT_REPELANCE);

  return eSUCCESS;
}
