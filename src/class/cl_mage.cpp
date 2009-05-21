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
#include <room.h>
#include <db.h>

extern CWorld world;
extern struct spell_info_type spell_info[MAX_SPL_LIST];


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
    if (a < 31) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_CHILL_TOUCH && a > 30)
  {
    if (a < 41) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_FIREBALL && a > 40)
  {
    if (a < 51) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_METEOR_SWARM && a > 50)
  {
    if (a < 61) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_PARALYZE && a > 60)
  {
    if (a < 71) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_CREATE_GOLEM && a > 70)
  {
    if (a < 81) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_HELLSTREAM && a > 80)
  {
    if (a < 91) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
    return eSUCCESS;
  }
  if (spell == SPELL_SOLAR_GATE && a > 90)
  {
    if (a < 100) skill_increase_check(ch, SKILL_SPELLCRAFT, a,SKILL_INCREASE_HARD);
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


int do_imbue(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  int lvl = has_skill(ch, SKILL_IMBUE);
  int charges = 0, manacost = 0;
  OBJ_DATA *wand;
  struct affected_type af;

  *buf = '\0';

  argument = one_argument(argument, buf);

  if(!lvl) {
   send_to_char("Huh?\r\n", ch);
   return eFAILURE;
  }
  if(affected_by_spell(ch, SKILL_IMBUE)) {
   send_to_char("Your mind has not yet recovered from the previous imbuement.\r\n", ch);
   return eFAILURE;
  }
  if(*buf == '\0') {
   send_to_char("Imbue what?\r\n", ch);
   return eFAILURE;
  }
  if(ch->in_room && IS_SET(world[ch->in_room].room_flags, NO_MAGIC)) {
   send_to_char("Something about this room prohibits your magical imbuement.\r\n", ch);
   return eFAILURE;
  }
  if(!(wand = get_obj_in_list_vis(ch, buf, ch->carrying))) {
   wand = ch->equipment[HOLD];
   if((wand == 0) || !isname(buf, wand->name)) {
    wand = ch->equipment[HOLD2];
    if((wand == 0) || !isname(buf, wand->name)) {
     send_to_char("You do not have that wand.\r\n", ch);
     return eFAILURE;
    }
   }
  }
  if(GET_LEVEL(ch) < wand->obj_flags.value[0]) {
   send_to_char("This wand is too powerful for you to imbue.\r\n", ch);
   return eFAILURE;
  }

  manacost = 4 + (10 - lvl/10) * spell_info[wand->obj_flags.value[3]].min_usesmana;

  if(GET_MANA(ch) < manacost) {
   send_to_char("You do not have enough magical energy to imbue this wand.\r\n", ch);
   return eFAILURE;
  }
  if(!charge_moves(ch, SKILL_IMBUE)) return eFAILURE;
   
  GET_MANA(ch) -= manacost;

  charges = number(1, 1 + lvl / 20);

  if(skill_success(ch, 0, SKILL_IMBUE)) {
   wand->obj_flags.value[2] += charges;
   if(wand->obj_flags.value[2] >= wand->obj_flags.value[1]) {
    wand->obj_flags.value[2] = wand->obj_flags.value[1];
    act("You focus your arcane powers and imbue them into $p restoring its full charge!", ch, wand, 0, TO_CHAR, 0);
   } else {
    sprintf(buf, "You focus your arcane powers and imbue them into $p restoring %d charges!", charges);
    act(buf, ch, wand, 0, TO_CHAR, 0);
   }
   send_to_char("As you finish, the tip of the freshly charged wand $Bglows$R briefly and returns to normal.\r\n", ch);
   act("$n focuses $s arcane powers and imbues them into $p!", ch, wand, 0, TO_ROOM, 0);
   act("As $e finishes, the tip of the freshly charged wand $Bglows$R briefly and returns to normal.", ch, wand, 0, TO_ROOM, 0);
  }
  else {
   act("You focus your arcane powers on $p but fail to restore its powers.", ch, wand, 0, TO_CHAR, 0);
   act("$n focuses $s arcane powers on $p to no effect.", ch, wand, 0, TO_ROOM, 0);
   if(wand->obj_flags.value[2] == 0) {
    act("Unable to bear the strain, $p splits asunder with a sharp crack!", ch, wand, 0, TO_CHAR, 0);
    act("Unable to bear the strain, $n's $p splits asunder with a sharp crack!", ch, wand, 0, TO_ROOM, 0);
    make_scraps(ch, wand);
   }
   else {
    wand->obj_flags.value[2] -= charges;
    if(wand->obj_flags.value[2] <= 0) {
     wand->obj_flags.value[2] = 0;
     act("The energy in $p has been completely lost!", ch, wand, 0, TO_CHAR, 0);
    }
    act("Some of the energy in $p has been lost!", ch, wand, 0, TO_CHAR, 0);
   }
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2.5);

  af.type      = SKILL_IMBUE;
  af.duration  = 2;
  af.modifier  = 0;
  af.location  = 0;
  af.bitvector = -1;

  affect_to_char(ch, &af);
  
  return eSUCCESS;
}
