/************************************************************************
| $Id: cl_warrior.cpp,v 1.10 2003/03/04 06:45:35 pirahna Exp $
| cl_warrior.C
| Description:  This file declares implementation for warrior-specific
|   skills.
*/
#include <structs.h>
#include <character.h>
#include <player.h>
#include <fight.h>
#include <utility.h>
#include <spells.h>
#include <handler.h>
#include <levels.h>
#include <connect.h>
#include <mobile.h>
#include <room.h>
#include <act.h>
#include <db.h>
#include <returnvals.h>

extern CWorld world;
extern struct index_data *obj_index;
 
extern char *dirs[];

bool ARE_GROUPED( CHAR_DATA *sub, CHAR_DATA *obj);
int attempt_move(CHAR_DATA *ch, int cmd, int is_retreat = 0);

/************************************************************************
| OFFENSIVE commands.  These are commands that should require the
|   victim to retaliate.
*/
int do_kick(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim;
    char name[256];
    int percent, learned, specialized, chance;
    int dam;
    int retval;

   if(IS_MOB(ch) || GET_LEVEL(ch) > ARCHANGEL)   
     learned = 75;
   else if(!(learned = has_skill(ch, SKILL_KICK))) {
     send_to_char("Go learn it from a master before you make a fool out of yourself.\r\n", ch);
     return eFAILURE;
   }

    specialized = learned / 100;
    learned = learned % 100;

    one_argument(argument, name);

    if (!(victim = get_char_room_vis(ch, name))) {
	if (ch->fighting) {
	    victim = ch->fighting;
	} else {
	    send_to_char("Kick whom?\n\r", ch);
	    return eFAILURE;
	}
    }

    if (victim == ch) {
	send_to_char("That would be pretty funny to watch...\n\r", ch);
	return eFAILURE;
    }

    if(!can_attack(ch) || !can_be_attacked(ch, victim))
          return eFAILURE;

    chance = 60 + learned/10;
	   
     /* 101% is a complete failure */
    percent = 10 - GET_AC(victim)/10 + number(1,101);

    WAIT_STATE(ch, PULSE_VIOLENCE*2);
    if (percent > chance) {
        dam = 0;
	retval = damage(ch, victim, 0,TYPE_UNDEFINED, SKILL_KICK, 0);
    } else {
        dam = GET_LEVEL(ch) * 4;
        if(specialized)
          dam = (int)(dam*1.5);
	retval = damage(ch, victim, dam, TYPE_UNDEFINED, SKILL_KICK, 0);
    }

    if(SOMEONE_DIED(retval))
      return retval;

    skill_increase_check(ch, SKILL_KICK, learned, SKILL_INCREASE_MEDIUM);

    // if our boots have a combat proc, and we did damage, let'um have it!
    if(dam && ch->equipment[WEAR_FEET]) {
      if(obj_index[ch->equipment[WEAR_FEET]->item_number].combat_func) {
        retval = ((*obj_index[ch->equipment[WEAR_FEET]->item_number].combat_func)
                       (ch, ch->equipment[WEAR_FEET], 0, "", ch));
      }
    }

    return retval;
}



int do_deathstroke(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim;
    char name[256];
    byte percent;
    int learned, specialization, chance;
    int dam;
    int retval;

    extern struct str_app_type str_app[];

    if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)   
      learned = 75;
    else if(!(learned = has_skill(ch, SKILL_DEATHSTROKE))) {
      send_to_char("You have no idea how to deathstroke.\r\n", ch);
      return eFAILURE;
    }

    one_argument(argument, name);

    if (!(victim = get_char_room_vis(ch, name))) {
	if (ch->fighting) {
	    victim = ch->fighting;
	} else {
	    send_to_char("Deathstroke whom?\n\r", ch);
	    return eFAILURE;
	}
    }

    if (victim == ch) {
	send_to_char("The world would be a better place...\n\r", ch);
	return eFAILURE;
    }

    if(!can_attack(ch) || !can_be_attacked(ch, victim))
          return eFAILURE;

    if(!ch->equipment[WIELD])
    {
       send_to_char("You must be wielding a weapon to deathstrike someone.\r\n", ch);
       return eFAILURE;
    }

    if(GET_POS(victim) > POSITION_RESTING)
    {
       send_to_char("Your opponent isn't in a vulnerable enough position!\r\n", ch);
       return eFAILURE;
    }

    specialization = learned / 100;
    learned = learned % 100;

    chance = 60;

    int to_dam = ( str_app[STRENGTH_APPLY_INDEX(ch)].todam ) + ( GET_DAMROLL(ch));
    if(IS_MOB(victim))
       to_dam = (int) ((float)to_dam * .8);
 
    dam = dice(ch->equipment[WIELD]->obj_flags.value[1],
               ch->equipment[WIELD]->obj_flags.value[2]);
    dam += to_dam;
    dam *= (GET_LEVEL(ch) / 5); // 10 at level 50

     /* 101% is a complete failure */
    percent= number(1,101);

    WAIT_STATE(ch, PULSE_VIOLENCE*3);

    skill_increase_check(ch, SKILL_DEATHSTROKE, learned, SKILL_INCREASE_EASY);

    if (percent > chance) {
	retval = damage(ch, victim, 0,TYPE_UNDEFINED, SKILL_DEATHSTROKE, 0);
        dam /= 4;
        if (IS_AFFECTED(ch, AFF_SANCTUARY))
          dam /= 2;
        GET_HIT(ch) -= dam;
        update_pos(ch);
        if (GET_POS(ch) == POSITION_DEAD) {
          fight_kill(ch, ch, TYPE_CHOOSE);
          return eSUCCESS|eCH_DIED;
        }
    } else {
	retval = damage(ch, victim, dam,TYPE_UNDEFINED, SKILL_DEATHSTROKE, 0);
    }

    return retval;
}


int do_retreat(struct char_data *ch, char *argument, int cmd)
{
   int attempt, die, percent;
   int learned, specialization, chance;
   char buf[MAX_INPUT_LENGTH];
   // Azrack -- retval should be initialized to something
   int retval = 0;

   int is_stunned(CHAR_DATA *ch);

   if (is_stunned(ch))
      return eFAILURE;

   if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)   
     learned = 75;
   else if(!(learned = has_skill(ch, SKILL_RETREAT))) {
     send_to_char("You dunno how...better flee instead.\r\n", ch);
     return eFAILURE;
   }

   // after the below line, buf should contain the argument, hopefully
   // a direction to retreat.
   one_argument(argument, buf); 

   if((attempt = consttype(buf, dirs)) == -1) {
     send_to_char("You are supposed to retreat towards a specific "
                  "direction...\n\r", ch);
     return eFAILURE;
   }

   if (IS_AFFECTED(ch, AFF_SNEAK))
      affect_from_char(ch, SKILL_SNEAK);

   specialization = learned / 100;
   learned = learned % 100;

   chance = 60;

   percent = number(1, 101);

   skill_increase_check(ch, SKILL_RETREAT, learned, SKILL_INCREASE_EASY);

   // failure
   if (percent > chance)
   {
      act("$n tries to retreat, but pays a heavy price for $s hesitancy.\n\r"
          "$n falls to the ground!",
          ch, 0, 0, TO_ROOM, INVIS_NULL);
      act("You try to retreat, but pay a heavy price for your hesitancy.\n\r"
          "You fall to the ground!",
          ch, 0, 0, TO_CHAR, INVIS_NULL);
      GET_POS(ch) = POSITION_SITTING;
      WAIT_STATE(ch, PULSE_VIOLENCE);
      return eFAILURE;
   }

   if (CAN_GO(ch, attempt))
   {
      act("$n tries to beat a hasty retreat.", ch, 0, 0, TO_ROOM,
         INVIS_NULL);
      send_to_char("You try to beat a hasty retreat....\n\r", ch);

      die = attempt_move(ch, attempt + 1, 1);
      if(IS_SET(die, eSUCCESS))
         // The escape has succeded
         return retval;
	  
      else {
         if (!die)
	    act("$n tries to retreat, but is too exhausted!",
                ch, 0, 0, TO_ROOM, INVIS_NULL);
         return retval;
      }
   }

   // No exits were found
   send_to_char("PANIC! You couldn't escape!\n\r", ch);
   return eFAILURE;
}


int do_hitall(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *vict, *temp;
    byte percent;
    int learned, specialization, chance;
    int retval;

    extern struct char_data *character_list;

   if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
     learned = 75;
   else if(!(learned = has_skill(ch, SKILL_HITALL))) {
     send_to_char("You better learn how to first...\r\n", ch);
     return eFAILURE;
   }

   if(GET_HIT(ch) == 1) {
      send_to_char("You are too weak to do this right now.\r\n", ch);
      return eFAILURE;
   }

   if(!can_attack(ch))
     return eFAILURE; 

   // TODO - I'm pretty sure we can remove this check....don't feel like checking right now though
   if(IS_SET(ch->combat, COMBAT_HITALL))
   {
     send_to_char("You can't disengage!\n\r",ch);
     return eFAILURE;
   }

   specialization = learned / 100;
   learned = learned % 100;

   chance = 60;

   /* 101% is a complete failure */
   percent= number(1,101);

   skill_increase_check(ch, SKILL_HITALL, learned, SKILL_INCREASE_MEDIUM);

   if (percent > chance) {

      act ("You start swinging like a madman, but trip over your own feet!", ch, 0, 0, TO_CHAR, 0);
      act ("$n starts swinging like a madman, but trips over $s own feet!", ch, 0, 0, TO_ROOM, 0);
     
      GET_POS(ch) = POSITION_SITTING;

      for (vict = character_list; vict; vict = temp) {
         temp = vict->next;
         if ((!ARE_GROUPED(ch, vict)) && (ch->in_room == vict->in_room) &&
           (vict != ch)  && !IS_SET(world[ch->in_room].room_flags, SAFE)) {
             remove_memory(vict, 'h');
             add_memory(vict, GET_NAME(ch), 'h');
         }
      }

   } else 
   {
      act ("You start swinging like a MADMAN!", ch, 0, 0, TO_CHAR, 0);
      act ("$n starts swinging like a MADMAN!", ch, 0, 0, TO_ROOM, 0);
      SET_BIT(ch->combat, COMBAT_HITALL);

      for (vict = character_list; vict; vict = temp) 
      {
         temp = vict->next;
         if ((!ARE_GROUPED(ch, vict)) && (ch->in_room == vict->in_room) &&
            (vict != ch)) 
         {
	    if(can_be_attacked(ch, vict))
              retval = one_hit(ch, vict, TYPE_UNDEFINED, FIRST);
            if(IS_SET(retval, eCH_DIED))
              return retval;
         }
       }
       REMOVE_BIT(ch->combat, COMBAT_HITALL);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE*3);
    return eSUCCESS;
}

int do_bash(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim;
    char name[256];
    byte percent;
    int learned, specialization, chance;
    int retval;
    int hit = 0;

    one_argument(argument, name);

  if(IS_AFFECTED(ch, AFF_CHARM))
    return eFAILURE;

  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)   
      learned = 75;
  else if(!(learned = has_skill(ch, SKILL_BASH))) {
      send_to_char("You don't know how to bash!\r\n", ch);
      return eFAILURE;
  }

  if (!IS_NPC(ch))
    if (!ch->equipment[WIELD]) 
    {
        send_to_char("You need to wield a weapon, to make it a success.\n\r",ch);
        return eFAILURE;
    }

  if (!(victim = get_char_room_vis(ch, name))) {
	if (ch->fighting) {
	    victim = ch->fighting;
	} else {
	    send_to_char("Bash whom?\n\r", ch);
	    return eFAILURE;
	}
    }

    if(!can_attack(ch) || !can_be_attacked(ch, victim))
      return eFAILURE;

    if (IS_MOB(victim) && IS_SET(victim->mobdata->actflags, ACT_HUGE)) {
      send_to_char("You can't bash something that HUGE!", ch);
         return eFAILURE;
    }

    if (victim == ch) {
	send_to_char("Aren't we funny today...\n\r", ch);
	return eFAILURE;
    }

    if(affected_by_spell(victim, SPELL_IRON_ROOTS)) {
        act("You try to bash $N but tree roots around $S legs keep him upright.", ch, 0, victim, TO_CHAR, 0);
        act("$n bashes you but the roots around your legs keep you from falling.", ch, 0, victim, TO_VICT, 0);
        act("The tree roots support $N keeping $M from sprawling after $n's bash.", ch, 0, victim, TO_ROOM, NOTVICT);
        WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
        return eFAILURE;
    }

    if(IS_AFFECTED2(victim, AFF_STABILITY) && number(0,3) == 0) {
        act("You bounce off of $N and crash into the ground.", ch, 0, victim, TO_CHAR, 0);
        act("$n bounces off of you and crashes into the ground.", ch, 0, victim, TO_VICT, 0);
        act("$n bounces off of $N and crashes into the ground.", ch, 0, victim, TO_ROOM, NOTVICT);
        WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
        return eFAILURE;
    }

    specialization = learned / 100;
    learned = learned % 100;

    skill_increase_check(ch, SKILL_BASH, learned, SKILL_INCREASE_HARD);

    chance = 60;

    // half as accurate without a shield
    if(!ch->equipment[WEAR_SHIELD]) {
      chance /= 2;
      // but 3/4 as accurate with a 2hander
      if(ch->equipment[WIELD] && IS_SET(ch->equipment[WIELD]->obj_flags.extra_flags, ITEM_TWO_HANDED))
        chance += (learned/2);
    }
 

    // 101% is a complete failure
    percent = number(1, 101) + ((GET_DEX(victim) - GET_DEX(ch))*3);

    WAIT_STATE(ch, PULSE_VIOLENCE*3);

    if (!IS_NPC(ch) && percent > chance) {
	GET_POS(ch) = POSITION_SITTING;
        SET_BIT(ch->combat, COMBAT_BASH1);
        act("As $N avoids your bash, you topple over and fall to the ground.", ch, NULL, victim, TO_CHAR , 0);
        act("You dodge a bash from $n who loses $s balance and falls.", ch, NULL, victim, TO_VICT , 0);
        act("$N avoids being bashed by $n who loses $s balance and falls.", ch, NULL, victim, TO_ROOM, NOTVICT);
	retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_BASH, 0);
    }
    else {
        hit = 1;
	GET_POS(victim) = POSITION_SITTING;
        SET_BIT(victim->combat, COMBAT_BASH1);
        // if they already have 2 rounds of wait, only tack on 1 instead of 2
        if(ch->desc && (ch->desc->wait > 1))
          WAIT_STATE(victim, PULSE_VIOLENCE);
	else WAIT_STATE(victim, PULSE_VIOLENCE*2);
        act("Your bash at $N sends $M sprawling.", ch, NULL, victim, TO_CHAR , 0);
        act("$n sends you sprawling.", ch, NULL, victim, TO_VICT , 0);
        act("$n sends $N sprawling with a powerful bash.", ch, NULL, victim, TO_ROOM, NOTVICT);
	retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_BASH, 0);
    }

    if(SOMEONE_DIED(retval))
      return retval;

    // if our shield has a combat proc and we hit them, let'um have it!
    if(hit && ch->equipment[WEAR_SHIELD]) {
      if(obj_index[ch->equipment[WEAR_SHIELD]->item_number].combat_func) {
        retval = ((*obj_index[ch->equipment[WEAR_SHIELD]->item_number].combat_func)
                       (ch, ch->equipment[WEAR_SHIELD], 0, "", ch));
      }
    }

    return retval;
}

int do_redirect(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim;
    char name[256];
    byte percent;
    int learned, specialization, chance;

    if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)   
      learned = 75;
    else if(!(learned = has_skill(ch, SKILL_REDIRECT))) {
      send_to_char("You aren't skilled enough to change opponents midfight!\r\n", ch);
      return eFAILURE;
    }

    one_argument(argument, name);

    victim = get_char_room_vis( ch, name );

    if ( victim == NULL )
    {
        send_to_char( "Redirect your attacks to whom?\n\r", ch );
        return eFAILURE;
    }

    if (victim == ch) {
        send_to_char("Aren't we funny today...\n\r", ch);
        return eFAILURE;
    }

    if (!ch->fighting) {
      send_to_char("You're not fighting anyone to begin with!\n\r", ch);
      return eFAILURE;
    }

    if (!victim->fighting) {
      send_to_char("He isn't bothering anyone, you have enough problems as it is anyways!\n\r", ch);
      return eFAILURE;
    }

    if(!can_be_attacked(ch, victim))
       return eFAILURE;

    specialization = learned / 100;
    learned = learned % 100;

    chance = 60;

    skill_increase_check(ch, SKILL_REDIRECT, learned, SKILL_INCREASE_EASY);

     /* 101% is a complete failure */
    percent=number(1,101) + (GET_LEVEL(victim) - GET_LEVEL(ch));

    if (percent > chance ) {
       act( "$n tries to redirect his attacks but $N won't allow it.", ch, NULL, ch->fighting, TO_VICT, 0 );
       act( "You try to redirect your attacks to $N but are blocked.", ch, NULL, victim, TO_CHAR, 0 );
       act( "$n tries to redirect his attacks elsewhere, but $N wont allow it.", ch, NULL, victim, TO_ROOM, NOTVICT );
       WAIT_STATE(ch, PULSE_VIOLENCE*3);
    } else 
    {
       act( "$n redirects his attacks at YOU!", ch, NULL, victim, TO_VICT , 0);
       act( "You redirect your at attacks at $N!", ch, NULL, victim, TO_CHAR , 0);
       act( "$n redirects his attacks at $N!", ch, NULL, victim, TO_ROOM, NOTVICT );
       stop_fighting(ch);
       set_fighting(ch, victim);
       WAIT_STATE(ch, PULSE_VIOLENCE*3);
    }
  return eSUCCESS;
}

int do_disarm( struct char_data *ch, char *argument, int cmd )
{
    struct char_data *victim;
    struct obj_data *wielded;

    char name[256];
    int percent;
    int learned, specialization, chance;
    struct obj_data *obj;
    int retval;

    int is_fighting_mob(struct char_data *ch);

    if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)   
      learned = 75;
    else if(!(learned = has_skill(ch, SKILL_DISARM))) {
      send_to_char("You dunno how.\r\n", ch);
      return eFAILURE;
    }

    if ( ch->equipment[WIELD] == NULL )
    {
	send_to_char( "You must wield a weapon to disarm.\n\r", ch );
	return eFAILURE;
    }

    one_argument( argument, name );
    victim = get_char_room_vis( ch, name );
    if ( victim == NULL )
	victim = ch->fighting;

    if ( victim == NULL )
    {
	send_to_char( "Disarm whom?\n\r", ch );
	return eFAILURE;
    }

    if(!can_attack(ch) || !can_be_attacked(ch, victim))
          return eFAILURE;

    set_cantquit(ch, victim);

    if ( victim->equipment[WIELD] == NULL )
    {
	send_to_char( "Your opponent is not wielding a weapon!\n\r", ch );
	return eFAILURE;
    }

    if ( victim == ch ) {
      if (GET_LEVEL(victim) >= IMMORTAL) {
          send_to_char("You can't seem to work it loose.\n\r", ch);
          return eFAILURE;
      }

      act( "$n disarms $mself!",  ch, NULL, victim, TO_ROOM, NOTVICT );
      send_to_char( "You disarm yourself!  Congradulations!  Try using 'remove' next-time.\n\r", ch );
      obj = unequip_char( ch, WIELD );
      obj_to_char( obj, ch );
      if (ch->equipment[SECOND_WIELD]) {
        obj = unequip_char( ch, SECOND_WIELD);
        equip_char(ch, obj, WIELD);
      }

      return eSUCCESS;
    }

    wielded = victim->equipment[WIELD];

    specialization = learned / 100;
    learned = learned % 100;

    chance = 60;

    percent = number( 1, 100 ) + GET_LEVEL(victim) - GET_LEVEL(ch);

    skill_increase_check(ch, SKILL_DISARM, learned, SKILL_INCREASE_MEDIUM);

    if(GET_LEVEL(ch) < 50 && GET_LEVEL(ch) + 10 < GET_LEVEL(victim))  // keep lowbies from disarming big mobs
       percent = 0;

    // Two handed weapons are twice as hard to disarm
    if (IS_SET(wielded->obj_flags.extra_flags, ITEM_TWO_HANDED))
       percent *= 2;

    if ( percent < chance )
    {
        if ((IS_SET(wielded->obj_flags.extra_flags,ITEM_NODROP)) || 
            (GET_LEVEL(victim) >= IMMORTAL)) {
          send_to_char("You can't seem to work it loose.\n\r", ch);
	WAIT_STATE( ch, 3 * PULSE_VIOLENCE );
        if (IS_NPC(victim) && !victim->fighting)
	  retval = one_hit(ch, victim, TYPE_UNDEFINED, 0);
          retval = SWAP_CH_VICT(retval);
          return retval;
        }

	disarm( ch, victim );
        if (IS_NPC(victim) && !victim->fighting)
	WAIT_STATE( ch, 3 * PULSE_VIOLENCE );
	retval = one_hit( victim, ch, TYPE_UNDEFINED, 0 );
        retval = SWAP_CH_VICT(retval);
    }
    else
    {
       act("$B$n attempts to disarm you!$R", ch, NULL, victim, TO_VICT, 0);
       act("You try to disarm $N and fail!", ch, NULL, victim, TO_CHAR, 0);
       act("$n attempts to disarm $N, but fails!", ch, NULL, victim, TO_ROOM, NOTVICT);
       WAIT_STATE( ch, PULSE_VIOLENCE*2 );
       if (IS_NPC(victim) && !victim->fighting)
          retval = one_hit( victim, ch, TYPE_UNDEFINED, 0 );
        retval = SWAP_CH_VICT(retval);
    }
    return retval;
}

/************************************************************************
| NON-OFFENSIVE commands.  Below here are commands that should -not-
|   require the victim to retaliate.
*/
int do_rescue(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim, *tmp_ch;
    int percent;
    int learned, specialization, chance;
    char victim_name[240];

    one_argument(argument, victim_name);

    if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)   
      learned = 75;
    else if(!(learned = has_skill(ch, SKILL_RESCUE))) {
      send_to_char("You've got alot to learn before you try to be a bodyguard.\r\n", ch);
      return eFAILURE;
    }

    if (!(victim = get_char_room_vis(ch, victim_name))) {
	send_to_char("Whom do you want to rescue?\n\r", ch);
	return eFAILURE;
    }

    if (victim == ch) {
	send_to_char("What about fleeing instead?\n\r", ch);
	return eFAILURE;
    }

    if(!can_be_attacked(ch, victim->fighting))
    {
       send_to_char("You cannot complete the rescue!\n\r", ch);
       return eFAILURE;
    }

    if ( !IS_NPC(ch) && IS_NPC(victim) )
    {
	send_to_char( "Doesn't need your help!\n\r", ch );
	return eFAILURE;
    }

    if (ch->fighting == victim) {
	send_to_char("How can you rescue someone you are trying to kill?\n\r",ch);
	return eFAILURE;
    }

    for (tmp_ch=world[ch->in_room].people; tmp_ch &&
	(tmp_ch->fighting != victim); tmp_ch=tmp_ch->next_in_room)
	;

    if (!tmp_ch) {
	act("But nobody is fighting $M?",  ch, 0, victim, TO_CHAR, 0);
	return eFAILURE;
    }

    specialization = learned / 100;
    learned = learned % 100;
    chance = 60;

    skill_increase_check(ch, SKILL_RESCUE, learned, SKILL_INCREASE_EASY);

    percent=number(1,101); /* 101% is a complete failure */

    if (percent > chance) {
        send_to_char("You fail the rescue.\n\r", ch);
        return eFAILURE;
    }

    send_to_char("Banzai! To the rescue...\n\r", ch);
    act("You are rescued by $N, you are confused!", victim, 0, ch, TO_CHAR, 0);
    act("$n heroically rescues $N.", ch, 0, victim, TO_ROOM, NOTVICT);

    if (victim->fighting == tmp_ch)
       stop_fighting(victim);
    if (tmp_ch->fighting)
       stop_fighting(tmp_ch);
    if (ch->fighting)
       stop_fighting(ch);

    /*
     * so rescuing an NPC who is fighting a PC does not result in
     * the other guy getting killer flag
     */
    set_fighting(ch, tmp_ch);
    set_fighting(tmp_ch, ch);

    WAIT_STATE(victim, 2*PULSE_VIOLENCE);
    return eSUCCESS;
}


int do_bladeshield(struct char_data *ch, char *argument, int cmd)
{
  struct affected_type af;
  int learned, percent, specialization, chance;

  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)   
    learned = 75;
  else if(!(learned = has_skill(ch, SKILL_BLADESHIELD))) {
    send_to_char("You'd cut yourself to ribbons just trying!\r\n", ch);
    return eFAILURE;
  }

  if(affected_by_spell(ch, SKILL_BLADESHIELD) && GET_LEVEL(ch) < IMMORTAL) {
    send_to_char("Your body is still recovering from your last use of the blade shield technique.\r\n", ch);
    return eFAILURE;
  }

  if(!(ch->fighting)) {
    send_to_char("But you aren't fighting anyone!\r\n", ch);
    return eFAILURE;
  }

  specialization = learned / 100;
  learned = learned % 100;
  chance = 60;
  chance += learned / 10;

  percent = number(1, 101);

  skill_increase_check(ch, SKILL_BLADESHIELD, learned, SKILL_INCREASE_EASY);

  if(chance < percent) {
    act("$n starts swinging $s weapons around but stops before narrowly avoiding dismembering $mself."
         , ch, 0, 0, TO_ROOM, NOTVICT);
    send_to_char("You try to begin the bladeshield technique and almost chop off your own arm!\r\n", ch);
  }
  else {
    act("$n forms a defensive wall of swinging weapons around $mself.", ch, 0, 0, TO_ROOM, NOTVICT);
    send_to_char("The world around you slows to a crawl, the weapons around you swing as if through water.  "
                 "Your mind clears of all but thrust angles as your body and mind enter completely into the "
                 "blade shield technique.\r\n", ch);
    SET_BIT(ch->combat, COMBAT_BLADESHIELD1);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE);

  af.type = SKILL_BLADESHIELD;
  af.duration  = 25;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = 0;
  affect_to_char(ch, &af);
  return eSUCCESS;
}

/* BEGIN UTILITY FUNCTIONS FOR "Guard" */

// return TRUE on guard doing anything
// otherwise FALSE
int handle_any_guard(char_data * ch)
{
   if(!ch->guarded_by)
      return FALSE;

   int specialized, chance;

   char_data * guard = NULL;

   // search the room for my guard
   for(follow_type * curr = ch->guarded_by; curr;) 
   {
      for(char_data * vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
         if(vict == curr->follower) 
         {
            curr = NULL;
            guard = vict;
            break;
         }
      if(curr)
         curr = curr->next;
   }

   if(!guard) // my guard isn't here
      return FALSE;

   int learned = has_skill(guard, SKILL_GUARD);
   specialized = learned / 100;
   learned = learned % 100;
   chance = 30;

   skill_increase_check(guard, SKILL_GUARD, learned, SKILL_INCREASE_MEDIUM);

   if(number(1, 101) < chance) {
      do_rescue(guard, GET_NAME(ch), 9);
      return TRUE;
   }
   return FALSE;
}

char_data * is_guarding_me(char_data * ch, char_data * guard)
{
   follow_type * curr = ch->guarded_by;

   while(curr) {
      if(curr->follower == guard)
        return guard;
      curr = curr->next;
   }

   return NULL;
}

void stop_guarding(char_data * guard)
{
   if(!guard->guarding) // i'm not guarding anyone:)  get out
      return;

   char_data   * victim = guard->guarding;
   follow_type * curr = victim->guarded_by;
   follow_type * last = NULL;

   while(curr) {
      if(curr->follower == guard)
        break;
      last = curr;
      curr = curr->next;
   }
   if(curr) {
      // found the guard, remove him from list
      if(last)
        last = curr->next;
      else victim->guarded_by = curr->next;
   }
   // if we didn't find guard, return, since we wanted to remove um anyway:)
   guard->guarding = NULL;
}

void start_guarding(char_data * guard, char_data * victim)
{
   follow_type * curr = (struct follow_type *) dc_alloc(1, sizeof(struct follow_type));

   curr->follower = guard;
   curr->next = victim->guarded_by;
   victim->guarded_by = curr;

   guard->guarding = victim;
}

void stop_guarding_me(char_data * victim)
{
   char buf[200];
   follow_type * curr = victim->guarded_by;
   follow_type * last;

   while(curr) {
      sprintf(buf, "You stop trying to guard %s.\n\r", GET_SHORT(victim));
      send_to_char(buf, curr->follower);
      curr->follower->guarding = NULL;
      last = curr;
      curr = curr->next;
      dc_free(last);      
   }

   victim->guarded_by = NULL;
}

/* END UTILITY FUNCTIONS FOR "Guard" */

int do_guard(struct char_data *ch, char *argument, int cmd)
{
   char name[MAX_INPUT_LENGTH];
   char_data * victim = NULL;

   if(!IS_MOB(ch) && (!has_skill(ch, SKILL_GUARD) || !has_skill(ch, SKILL_RESCUE))) {
     send_to_char("You have no idea how to be a full time bodyguard.\r\n", ch);
     return eFAILURE;
   }

   one_argument(argument, name);

   if (!(victim = get_char_room_vis(ch, name))) {
      send_to_char("Some bodyguard you are.  That person isn't even here!\n\r", ch);
      return eFAILURE;
   }

   if(ch == victim) {
      send_to_char("You stop guarding anyone.\n\r", ch);
      stop_guarding(ch);
      return eSUCCESS;
   }

   if(victim == ch->guarding) {
      send_to_char("You are already guarding that person.\n\r", ch);
      return eFAILURE;
   }

   if(ch->guarding) {
      send_to_char("You are already guarding someone.\n\r", ch);
      return eFAILURE;
   }

   start_guarding(ch, victim);
   sprintf(name, "You begin trying to guard %s.", GET_SHORT(victim));
   send_to_char(name, ch);
   return eSUCCESS;   
}
