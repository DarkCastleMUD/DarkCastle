/************************************************************************
| $Id: cl_barbarian.cpp,v 1.2 2002/06/13 04:41:13 dcastle Exp $
| cl_barbarian.C
| Description:  Commands for the barbarian class.
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

int do_rage(struct char_data *ch, char *argument, int cmd)
{
  char_data *victim;
  char name[256];
  byte percent;
  byte learned;

  if ((GET_CLASS(ch) != CLASS_BARBARIAN) && GET_LEVEL(ch) < ARCHANGEL) {
    send_to_char("You better leave all the craziness to TRUE Barbarians!\n\r",
                 ch);
    return eFAILURE;
  }

  if (GET_LEVEL(ch) <= 14){
    send_to_char("You are not crazy enough for this!\n\r", ch);
    return eFAILURE;
  }

  if(GET_HIT(ch) == 1) {
    send_to_char("You are feeling too weak right now to work yourself up into "
                 "a rage.", ch);
    return eFAILURE;
  }

  if(IS_MOB(ch))
    learned = 75;
  else if(!(learned = has_skill(ch, SKILL_RAGE))) {
    send_to_char("You should learn the skill before you try doing any raging in this machine...\r\n", ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  if(!(victim = get_char_room_vis(ch, name))) {
    if(ch->fighting)
      victim = ch->fighting;
    else {
      send_to_char( "Who do you want to rage on?\n\r", ch );
      return eFAILURE;
    }
  }

  if(ch->in_room != victim->in_room) {
    send_to_char("That person seems to have left.\n\r", ch);
    return eFAILURE;
  }
 
  if (victim == ch) {
    send_to_char("Aren't we funny today...\n\r", ch);
    return eFAILURE;
  }

  if(!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  // 101% is a complete failure
  percent = number(1, 101);

  if (percent > learned) {
    act ("You start advancing towards $N, but trip over your own feet!", 
         ch, 0, victim, TO_CHAR, 0);
    act ("$n starts advancing toward you, but trips over $s own feet!",
         ch, 0, victim, TO_VICT, 0);
    act ("$n starts advancing towards $N, but trips over $s own feet!",
         ch, 0, victim, TO_ROOM, NOTVICT);
     
    GET_POS(ch) = POSITION_SITTING;
    SET_BIT(ch->combat, COMBAT_BASH1);
  }
  else {
    act ("You advance confidently towards $N, and fly into a rage!",
        ch, 0, victim, TO_CHAR, 0);
    act ("$n advances confidently towards you, and flies into a rage!",
        ch, 0, victim, TO_VICT, 0);
    act ("$n advances confidently towards $N, and flies into a rage!",
            ch, 0, victim, TO_ROOM, NOTVICT);
	    
    SET_BIT(ch->combat, COMBAT_RAGE1);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE);
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
  if(!ch->fighting)
     return attack(ch, victim, TYPE_UNDEFINED);
  return eSUCCESS;
}

int do_battlecry(struct char_data *ch, char *argument, int cmd)
{
  byte percent;
  byte learned;
  follow_type * f;

  if ((GET_CLASS(ch) != CLASS_BARBARIAN) && GET_LEVEL(ch) < ARCHANGEL) {
     send_to_char("You better leave all the craziness to TRUE Barbarians!\n\r",
                  ch);
     return eFAILURE;
     }

  if (GET_LEVEL(ch) <= 14){
     send_to_char("You are not crazy enough for this!\n\r", ch);
     return eFAILURE;
     }

  if(IS_MOB(ch))
    learned = 75;
  else if(!(learned = has_skill(ch, SKILL_BATTLECRY))) {
    send_to_char("Have to learn how to battlecry before you can run with the big boys...\r\n", ch);
    return eFAILURE;
  }

  if (!ch->fighting) {
     send_to_char("You must be fighting already in order to use "
                  "this skill.\n\r", ch);
     return eFAILURE;
     }

  if (GET_HIT(ch) == 1) {
     send_to_char("You are feeling too weak right now to work yourself up "
                  "into a rage.", ch);
     return eFAILURE;
     }

  // 101% is a complete failure
  percent = number(1, 101);

  if (percent > learned) {
     act ("You give a cry of defiance, but trip over your own feet!", 
          ch, 0, 0, TO_CHAR, 0);
     act ("$n gives a cry of defiance, but trips over $s own feet!",
          ch, 0, 0, TO_ROOM, 0);
     
     GET_POS(ch) = POSITION_SITTING;
     SET_BIT(ch->combat, COMBAT_BASH1);
     }
  else {
     act ("You give a battlecry, sounding your defiance!", ch, 0, 0,
          TO_CHAR, 0);
     act ("$n yells 'They can take our lives, but they'll never take "
          "OUR FREEDOM!'", ch, 0, 0, TO_ROOM, 0);

     SET_BIT(ch->combat, COMBAT_RAGE1);

     for(f = ch->followers; f; f = f->next) {
        if (!IS_AFFECTED(f->follower, AFF_GROUP) ||
	    (IS_SET(f->follower->combat, COMBAT_RAGE1)) ||
	    (IS_SET(f->follower->combat, COMBAT_RAGE2)) ||
	    (IS_SET(f->follower->combat, COMBAT_BERSERK)) ||
	    (f->follower->in_room != ch->in_room) ||
            (!f->follower->fighting) ||
	    (GET_INT(f->follower) > 18))
           continue;

        act ("You give a battlecry, sounding your defiance!", f->follower,
	     0, 0, TO_CHAR, 0);
        act ("$n gives a loud cry of agreement!", f->follower,
	     0, 0, TO_ROOM, 0);
        SET_BIT(f->follower->combat, COMBAT_RAGE1);
	}
     }

   WAIT_STATE(ch, PULSE_VIOLENCE * 3);
   return eSUCCESS;
}


int do_berserk(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[256];
  byte percent;
  byte learned;
  int bSuccess = 0;
  int retval;
  
  if ((GET_CLASS(ch) != CLASS_BARBARIAN) && GET_LEVEL(ch) < ARCHANGEL) {
    send_to_char("You better leave all the craziness to TRUE Barbarians!\n\r",
                 ch);
    return eFAILURE;
  }

  if (GET_LEVEL(ch) < 35){
    send_to_char("You are not crazy enough for this!\n\r", ch);
    return eFAILURE;
  }

  if(IS_MOB(ch))
    learned = 75;
  else if(!(learned = has_skill(ch, SKILL_BERSERK))) {
    send_to_char("Have to learn how to battlecry before you can run with the big boys...\r\n", ch);
    return eFAILURE;
  }


  if(GET_HIT(ch) == 1) {
    send_to_char("You are feeling too weak right now to work yourself up into "
                 "a frenzy.", ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  if(!(victim = get_char_room_vis(ch, name))) {
    if(ch->fighting)
      victim = ch->fighting;
    else {
      send_to_char( "Who do you want to go berserk on?\n\r", ch );
      return eFAILURE;
    }
  }

  if(ch->in_room != victim->in_room) {
    send_to_char("That person seems to have left.\n\r", ch);
    return eFAILURE;
  }
 
  if (victim == ch) {
    send_to_char("Aren't we funny today...\n\r", ch);
    return eFAILURE;
  }

  if(!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  // 101% is a complete failure
  percent = number(1, 101);

  if (percent > learned) {
    act ("You start freaking out on $N, but trip over your own feet!", 
         ch, 0, victim, TO_CHAR, 0);
    act ("$n starts freaking out on you, but trips over $s own feet!",
         ch, 0, victim, TO_VICT, 0);
    act ("$n starts freaking out on $N, but trips over $s own feet!",
         ch, 0, victim, TO_ROOM, NOTVICT);
    GET_POS(ch) = POSITION_SITTING;
    SET_BIT(ch->combat, COMBAT_BASH1);     
  }
  else {
    act ("You start FOAMING at the mouth, and you go BERSERK on $N!",
        ch, 0, victim, TO_CHAR, 0);
    act ("$n starts FOAMING at the mouth, and goes BERSERK on you!",
        ch, 0, victim, TO_VICT, 0);
    act ("$n starts FOAMING at the mouth, as $e goes BERSERK on $N!",
            ch, 0, victim, TO_ROOM, NOTVICT);
    bSuccess = 1;
    GET_AC(ch) += 80;
  }

  if (bSuccess && !IS_SET(ch->combat, COMBAT_RAGE1))
     SET_BIT(ch->combat, COMBAT_RAGE1);

  if(!ch->fighting)
     retval = attack(ch, victim, TYPE_UNDEFINED);

  if(!IS_SET(retval, eCH_DIED)) {
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
  
    if (IS_SET(ch->combat, COMBAT_RAGE1))
       REMOVE_BIT(ch->combat, COMBAT_RAGE1);
    if (IS_SET(ch->combat, COMBAT_RAGE2))
       REMOVE_BIT(ch->combat, COMBAT_RAGE2);
     
    // I set the berserk bit AFTER the attack, to reduce the advantage
    // a bit.
    if (bSuccess)
       SET_BIT(ch->combat, COMBAT_BERSERK);
  }
  return retval;
}


int do_headbutt(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[256];
  byte percent;
  byte learned;
  int retval;

  if (((GET_CLASS(ch) != CLASS_BARBARIAN) && (GET_CLASS(ch) != CLASS_WARRIOR))
      && GET_LEVEL(ch) < ARCHANGEL) {
        send_to_char("You don't fight like that.\n\r", ch);
        return eFAILURE;
  }

  if ((GET_CLASS(ch) == CLASS_WARRIOR) && (GET_LEVEL(ch) < 15)) {
    send_to_char("You aren't quite bold enough, yet.\n\r", ch);
    return eFAILURE;
  }

  if(IS_MOB(ch))
    learned = 75;
  else if(!(learned = has_skill(ch, SKILL_SHOCK))) {
    send_to_char("You'd bonk yourself silly without proper training.\r\n", ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  victim = get_char_room_vis( ch, name );
  if ( victim == NULL )
    victim = ch->fighting;

  if ( victim == NULL ) {
    send_to_char( "Headbutt whom?\n\r", ch );
    return eFAILURE;
  }

  if(ch->in_room != victim->in_room) {
    send_to_char("That person seems to have left.\n\r", ch);
    return eFAILURE;
  }
  if (victim == ch) {
    send_to_char("Aren't we funny today...\n\r", ch);
    return eFAILURE;
  }

  if(!can_attack(ch) || !can_be_attacked(ch, victim))
    return eFAILURE;

  if(IS_MOB(victim) && IS_SET(victim->mobdata->actflags, ACT_HUGE)) {
    send_to_char("You're to puny too headbutt someone that HUGE!\n\r",ch);
    return eFAILURE;
  }

  // 101% is a complete failure
  percent = number(1, 101) + (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (GET_LEVEL(victim) >= IMMORTAL)
    percent = 101;

  if (percent > learned * 4 / 5 ) 
  {
    act( "$n tries to headbutt you but fails miserably.",
      ch, NULL, victim, TO_VICT , 0);
    act( "You try to headbutt $N, but fail miserably.",
      ch, NULL, victim, TO_CHAR , 0);
    act( "$n tries to headbutt $N, but fails miserably.",
      ch, NULL, victim, TO_ROOM, NOTVICT );
    WAIT_STATE(ch, PULSE_VIOLENCE*3);
    retval = damage (ch, victim, 0, TYPE_UNDEFINED, SKILL_SHOCK, 0);
  }
  else {
    act( "$n slams $s forehead into your face! You are SHOCKED!",
	  ch, NULL, victim, TO_VICT , 0);
    act( "You slam your forehead into $N's face! $N looks SHOCKED!",
	  ch, NULL, victim, TO_CHAR , 0);
    act( "$n slams $s forehead into $N's face! $N looks SHOCKED!",
	  ch, NULL, victim, TO_ROOM, NOTVICT );

    if (IS_SET(victim->combat, COMBAT_BERSERK))
       REMOVE_BIT(victim->combat, COMBAT_BERSERK);

    WAIT_STATE(ch, PULSE_VIOLENCE*4);
    WAIT_STATE(victim, PULSE_VIOLENCE*2);
    SET_BIT(victim->combat, COMBAT_SHOCKED);
    retval = damage (ch, victim, 0, TYPE_UNDEFINED, SKILL_SHOCK, 0);
  }
  return retval;
}

int do_bloodfury(struct char_data *ch, char *argument, int cmd)
{
  byte percent;
  byte learned;
  struct affected_type af;

  if(IS_MOB(ch))
    learned = 75;
  else if(!(learned = has_skill(ch, SKILL_BLOOD_FURY))) {
    send_to_char("You've no idea how to raise such bloodlust.\r\n", ch);
    return eFAILURE;
  }

  if(affected_by_spell(ch, SKILL_BLOOD_FURY)) {
    send_to_char("Your body can not yet take the strain of another blood fury yet.\r\n", ch);
    return eFAILURE;
  }

  // 101% is a complete failure
  percent = number(1, 101);

  if (GET_LEVEL(ch) >= IMMORTAL)
    percent = 1;

  if (percent > learned) 
  {
    act("$n starts breathing heavily, then chokes and tries to clear $s head.",
         ch, NULL, NULL, TO_ROOM, NOTVICT);
    send_to_char("You try to pysch yourself up and choke on the taste of blood.\r\n", ch);
  }
  else 
  {
    act( "Panting heavily $n's eyes glaze red $e begins to move with renewed fury!",
	  ch, NULL, NULL, TO_ROOM, NOTVICT );
    send_to_char("Your sight tinges red with the blood of battle and your " 
                 "limbs feel strong with death and destruction deep within " 
                 "your bones.\r\n", ch);
    GET_HIT(ch) += GET_MAX_HIT(ch)/2;
    if(GET_HIT(ch) > GET_MAX_HIT(ch))
      GET_HIT(ch) = GET_MAX_HIT(ch);
  }

  af.type      = SKILL_BLOOD_FURY;
  af.duration  = 42 - (GET_LEVEL(ch)/4);
  af.modifier  = 0;
  af.location  = 0;
  af.bitvector = 0;

  affect_to_char(ch, &af);

  return eSUCCESS;
}
