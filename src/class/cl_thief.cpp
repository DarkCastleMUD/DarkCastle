/************************************************************************
| $Id: cl_thief.cpp,v 1.170 2007/11/20 21:35:33 pirahna Exp $
| cl_thief.C
| Functions declared primarily for the thief class; some may be used in
|   other classes, but they are mainly thief-oriented.
*/
#include <character.h>
#include <structs.h>
#include <utility.h>
#include <spells.h>
#include <levels.h>
#include <player.h>
#include <obj.h>
#include <room.h>
#include <handler.h>
#include <mobile.h>
#include <fight.h>
#include <connect.h>
#include <interp.h>
#include <act.h>
#include <db.h>
#include <string.h>
#include <returnvals.h>
#include <clan.h>
#include <arena.h>

extern int rev_dir[];
extern CWorld world;
 
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern int top_of_world;
extern struct zone_data *zone_table;

int find_door(CHAR_DATA *ch, char *type, char *dir);
struct obj_data * search_char_for_item(char_data * ch, int16 item_number, bool wearonly = FALSE);

int get_weapon_damage_type(struct obj_data * wielded);



extern int check_autojoiners(CHAR_DATA *ch, int skill = 0);
extern int check_joincharmie(CHAR_DATA *ch, int skill = 0);


int palm(CHAR_DATA *ch, struct obj_data *obj_object,
          struct obj_data *sub_object, bool has_consent)
{
  char buffer[MAX_STRING_LENGTH];
    
  if(!has_skill(ch, SKILL_PALM) && !IS_NPC(ch)) {
    send_to_char("You aren't THAT slick there, pal.\r\n", ch);
    return eFAILURE;
  }

  if(!sub_object || sub_object->carried_by != ch) {
     if(IS_SET(obj_object->obj_flags.more_flags, ITEM_UNIQUE))
        if(search_char_for_item(ch, obj_object->item_number)) {
           send_to_char("The item's uniqueness prevents it!\r\n", ch);
           return eFAILURE;
        }
     if(contents_cause_unique_problem(obj_object, ch)) {
        send_to_char("Something inside the item is unique and prevents it!\n\r", ch);
        return eFAILURE;
     }
  }

  
  if(obj_index[obj_object->item_number].virt == CHAMPION_ITEM) {
     if (IS_NPC(ch) || GET_LEVEL(ch) <= 5) return eFAILURE;
           SETBIT(ch->affected_by, AFF_CHAMPION);
           sprintf(buffer, "\n\r##%s has just picked up the Champion flag!\n\r", GET_NAME(ch));
           send_info(buffer);
  }

  if (sub_object)
  {
                        sprintf(buffer,"%s_consent",GET_NAME(ch));
                      if (has_consent && obj_object->obj_flags.type_flag != ITEM_MONEY) {
                                if (isname("lootable",sub_object->name) && !isname(buffer,sub_object->name))
                                {
                                  SET_BIT(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED);;
                                  struct affected_type pthiefaf;
                                  WAIT_STATE(ch, PULSE_VIOLENCE*2);
                                  send_to_char("You suddenly feel very guilty...shame on you stealing from the dead!\r\n",ch);

                                  pthiefaf.type = FUCK_PTHIEF;
                                  pthiefaf.duration = 10;
                                  pthiefaf.modifier = 0;
                                  pthiefaf.location = APPLY_NONE;
                                  pthiefaf.bitvector = -1;

                                  if(affected_by_spell(ch, FUCK_PTHIEF))
                                  {
                                        affect_from_char(ch, FUCK_PTHIEF);
                                        affect_to_char(ch, &pthiefaf);
                                  }
                                  else
                                        affect_to_char(ch, &pthiefaf);

                                }
                      } else if (has_consent && obj_object->obj_flags.type_flag == ITEM_MONEY && !isname(buffer,sub_object->name)) {
                                if (isname("lootable",sub_object->name))
                                {
                                  struct affected_type pthiefaf;

                                  pthiefaf.type = FUCK_GTHIEF;
                                  pthiefaf.duration = 10;
                                  pthiefaf.modifier = 0;
                                  pthiefaf.location = APPLY_NONE;
                                  pthiefaf.bitvector = -1;
                                  WAIT_STATE(ch, PULSE_VIOLENCE);
                                  send_to_char("You suddenly feel very guilty...shame on you stealing from the dead!\r\n",ch);

                                  if(affected_by_spell(ch, FUCK_GTHIEF))
                                  {
                                        affect_from_char(ch, FUCK_GTHIEF);
                                        affect_to_char(ch, &pthiefaf);
                                  }
                                  else
                                        affect_to_char(ch, &pthiefaf);

                                }
                      }

  }
  move_obj(obj_object, ch);
  char log_buf[MAX_STRING_LENGTH];
  if (sub_object && sub_object->in_room && obj_object->obj_flags.type_flag != ITEM_MONEY)
  { // Logging gold gets from corpses would just be too much.
    sprintf(log_buf, "%s palms %s[%d] from %s", GET_NAME(ch), obj_object->name, obj_index[obj_object->item_number].virt,
                sub_object->name);
    log(log_buf, IMP, LOG_OBJECTS);
    for(OBJ_DATA *loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
      logf(IMP, LOG_OBJECTS, "The %s contained %s[%d]", obj_object->short_description, loop_obj->short_description,
                          obj_index[loop_obj->item_number].virt);


  } else if (!sub_object && obj_object->obj_flags.type_flag != ITEM_MONEY) {
    sprintf(log_buf, "%s palms %s[%d] from room %d", GET_NAME(ch), obj_object->name, obj_index[obj_object->item_number].virt,
                ch->in_room);
    log(log_buf, IMP, LOG_OBJECTS);
    for(OBJ_DATA *loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
      logf(IMP, LOG_OBJECTS, "The %s contained %s[%d]", obj_object->short_description, loop_obj->short_description,
                        obj_index[loop_obj->item_number].virt);
  }

  
  if(skill_success(ch,NULL,SKILL_PALM)) {
    act("You successfully snag $p, no one saw you do it!",  ch, 
          obj_object, 0, TO_CHAR, 0);
    act("$n palms $p trying to hide it from your all knowing gaze.",
          ch, obj_object, 0, TO_ROOM, GODS);
  }
  else {
    act("You clumsily take $p...", ch, obj_object, 0, TO_CHAR, 0);
    if(sub_object) act("$n gets $p from $P.", ch, obj_object, sub_object,
           TO_ROOM, INVIS_NULL);
    else
      act("$n gets $p.", ch, obj_object, 0, TO_ROOM, INVIS_NULL); 
  }
  if((obj_object->obj_flags.type_flag == ITEM_MONEY) &&
     (obj_object->obj_flags.value[0] >= 1)) {
    obj_from_char(obj_object);
    sprintf(buffer, "There was %d coins.\n\r",
      obj_object->obj_flags.value[0]);
    send_to_char(buffer, ch);
        if (zone_table[world[ch->in_room].zone].clanowner > 0 && ch->clan !=
                zone_table[world[ch->in_room].zone].clanowner)
        {
                 int cgold = (int)((float)(obj_object->obj_flags.value[0]) * 0.1);
                 obj_object->obj_flags.value[0] -= cgold;
                 csendf(ch, "Clan %s collects %d bounty, leaving %d for you.\r\n",get_clan(zone_table[world[ch->in_room].zone].clanowner)->name,cgold,
                       obj_object->obj_flags.value[0]);
                zone_table[world[ch->in_room].zone].gold += cgold;
        }

    GET_GOLD(ch) += obj_object->obj_flags.value[0];
    extract_obj(obj_object);
  }
  return eSUCCESS;
}

int do_eyegouge(CHAR_DATA *ch, char *argument, int cmd)
{
  CHAR_DATA *victim;
  char name[256];
  int level = has_skill(ch, SKILL_EYEGOUGE);

  argument = one_argument(argument,name);

  if(!(victim = get_char_room_vis(ch, name)) && !(victim = ch->fighting)) {
    send_to_char("There is no one like that here to gouge.\n\r", ch);
	    return eFAILURE;
  }
  if (IS_AFFECTED(victim, AFF_BLIND))
  {
    send_to_char("They are already blinded!\r\n",ch);
    return eFAILURE;
  }
  if(victim == ch) {
    send_to_char("That sounds...stupid.\n\r", ch);
    return eFAILURE;
  }

  if (!level) {
    send_to_char("You would...if you knew how.\r\n",ch);
    return eFAILURE;
  }

  if(!can_be_attacked(ch, victim) || !can_attack(ch))
    return eFAILURE;

  if (IS_SET(victim->combat, COMBAT_BLADESHIELD1) || IS_SET(victim->combat, COMBAT_BLADESHIELD2)) {
        send_to_char("Trying to eyegouge a bladeshielded opponent would be suicide!\n\r", ch);
        return eFAILURE;
  }

  int retval = 0;
  if (!skill_success(ch,victim, SKILL_EYEGOUGE))
  {
     retval = damage(ch,victim, 0, TYPE_PIERCE, SKILL_EYEGOUGE, 0);
  } else {
    if (!IS_SET(victim->immune, TYPE_PIERCE)) {
      SETBIT(victim->affected_by, AFF_BLIND);
      SET_BIT(victim->combat, COMBAT_THI_EYEGOUGE);
      struct affected_type af;
      af.type = SKILL_EYEGOUGE;
      af.location = APPLY_AC;
      af.modifier = level / 2;
      af.duration = 1;
      af.bitvector = -1;
      affect_to_char(victim, &af, PULSE_VIOLENCE);
    }

    retval = damage(ch, victim, level*2, TYPE_PIERCE, SKILL_EYEGOUGE, 0);
  }

  if(!SOMEONE_DIED(retval) || (!IS_NPC(ch) && IS_SET(ch->pcdata->toggles, PLR_WIMPY)))
     WAIT_STATE(ch, PULSE_VIOLENCE*3);
  return retval | eSUCCESS;
}

int do_backstab(CHAR_DATA *ch, char *argument, int cmd)
{
  CHAR_DATA *victim;
  char name[256];
  int was_in = 0;
  int retval;

  one_argument(argument, name);

  if(!has_skill(ch,SKILL_BACKSTAB) && !IS_MOB(ch) ) 
  {
    send_to_char("You don't know how to backstab people!\r\n", ch);
    return eFAILURE;
  }

  if(!(victim = get_char_room_vis(ch, name))) {
    send_to_char("Backstab whom?\n\r", ch);
    return eFAILURE;
  }

  if(victim == ch) {
    send_to_char("How can you sneak up on yourself?\n\r", ch);
    return eFAILURE;
  }

  if(IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_HUGE)) {
    send_to_char("You cannot backstab someone that HUGE!\r\n", ch);
    return eFAILURE;
  }

  if(IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_SWARM)) {
    send_to_char("You cannot target just one to backstab!\r\n", ch);
    return eFAILURE;
  }

  if(IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_TINY)) {
    send_to_char("You cannot target someone that tiny to backstab!\r\n", ch);
    return eFAILURE;
  }

  if(IS_AFFECTED(victim, AFF_ALERT)) {
    act("$E is too alert and nervous looking; you are unable to sneak behind!", ch,0,victim, TO_CHAR, 0);
    return eFAILURE;
  }

  int min_hp = (int) (GET_MAX_HIT(ch) / 5);
  min_hp = MIN( min_hp, 25 );

  if( GET_HIT(ch) < min_hp ) {
    send_to_char("You are feeling too weak right now to attempt such a bold maneuver.\r\n", ch);
    return eFAILURE;
  }

  if(!ch->equipment[WIELD]) {
    send_to_char("You need to wield a weapon to make it a success.\n\r", ch);
    return eFAILURE;
  }

  if(ch->equipment[WIELD]->obj_flags.value[3] != 11 && ch->equipment[WIELD]->obj_flags.value[3] != 9) {
    send_to_char("You can't stab without a stabbing weapon...\n\r", ch);
    return eFAILURE;
  }

  if(victim->fighting) {
    send_to_char("You can't backstab a fighting person, they are too alert!\n\r", ch);
    return eFAILURE;
  }
  
  // Check the killer/victim
  if((GET_LEVEL(ch) < G_POWER) || IS_NPC(ch)) {
      if(!can_attack(ch) || !can_be_attacked(ch, victim))
      return eFAILURE;
  }
 
  int itemp = number(1, 100);
  if (!IS_NPC(ch) && !IS_NPC(victim))
  {
    if (GET_LEVEL(victim) > GET_LEVEL(ch))
	itemp = 0; // not gonna happen
    else if (GET_MAX_HIT(victim) > GET_MAX_HIT(ch)) 
    {
        if (GET_MAX_HIT(victim) * 0.85 > GET_MAX_HIT(ch)) itemp--;
        if (GET_MAX_HIT(victim) * 0.70 > GET_MAX_HIT(ch)) itemp--;
        if (GET_MAX_HIT(victim) * 0.55 > GET_MAX_HIT(ch)) itemp--;
        if (GET_MAX_HIT(victim) * 0.40 > GET_MAX_HIT(ch)) itemp--;
        if (GET_MAX_HIT(victim) * 0.25 > GET_MAX_HIT(ch)) itemp--;
    }
  }
  

  // record the room I'm in.  Used to make sure a dual can go off.
  was_in = ch->in_room;

  // Will this be a single or dual backstab this round?
  bool perform_dual_backstab = false;
  if(((GET_CLASS(ch) == CLASS_THIEF) || (GET_LEVEL(ch) >= ARCHANGEL)) &&
     (ch->equipment[SECOND_WIELD])                                    &&
     ((ch->equipment[SECOND_WIELD]->obj_flags.value[3] == 11) ||
      (ch->equipment[SECOND_WIELD]->obj_flags.value[3] == 9))        &&
     has_skill(ch, SKILL_DUAL_BACKSTAB)
     )
    {
      if(skill_success(ch,victim,SKILL_DUAL_BACKSTAB)) {
	perform_dual_backstab = true;
      }
    }
  WAIT_STATE(ch, PULSE_VIOLENCE*2);
  // failure
  if(AWAKE(victim) && !skill_success(ch,victim,SKILL_BACKSTAB)) {
    // If this is stab 1 of 2 for a dual backstab, we dont want people autojoining on the first stab
    if (perform_dual_backstab && IS_PC(ch)) {
      ch->pcdata->unjoinable = true;
      retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB, 0);
      ch->pcdata->unjoinable = false;
    } else {
      retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB, 0);
    }
  }
  // success
  else if(
          ( ( GET_LEVEL(victim) < IMMORTAL && !IS_NPC(victim) ) 
            || IS_NPC(victim)
          ) 
          && (GET_LEVEL(victim) <= GET_LEVEL(ch) + 19) 
          && ((!IS_NPC(ch) && GET_LEVEL(ch) >= IMMORTAL) || itemp > 94 || 
               ( !IS_NPC(victim) && IS_SET(victim->pcdata->punish, PUNISH_UNLUCKY) )
             )
          && (ch->equipment[WIELD]->obj_flags.value[3] == 11 && !IS_SET(victim->immune, ISR_PIERCE) ||
              ch->equipment[WIELD]->obj_flags.value[3] == 9 && !IS_SET(victim->immune, ISR_STING)
             )
         ) 
  { 
    act("$N crumples to the ground, $S body still quivering from "
        "$n's brutal assassination.", ch, 0, victim, TO_ROOM, NOTVICT);
    act("You feel $n's blade slip into your heart, and all goes black.",
        ch, 0, victim, TO_VICT, 0);
    act("BINGO! You brutally assassinate $N, and $S body crumples "
        "before you.", ch, 0, victim, TO_CHAR, 0);
    return damage(ch, victim, 9999999, TYPE_UNDEFINED, SKILL_BACKSTAB, 0); 
  }
  else {
    // If this is stab 1 of 2 for a dual backstab, we dont want people autojoining on the first stab
    if (perform_dual_backstab && IS_PC(ch)) {
      ch->pcdata->unjoinable = true;
      retval = attack(ch, victim, SKILL_BACKSTAB, FIRST);
      ch->pcdata->unjoinable = false;
    } else {
      retval = attack(ch, victim, SKILL_BACKSTAB, FIRST);
    }
  }

  if (retval & eVICT_DIED && !retval & eCH_DIED)
  {
    if(!IS_NPC(ch) && IS_SET(ch->pcdata->toggles, PLR_WIMPY))
      WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    else
      add_command_lag(ch, cmd, PULSE_VIOLENCE*1.5); 
    return retval;
  }

  if (retval & eCH_DIED) return retval;

  if (retval & eVICT_DIED)
  {
    add_command_lag(ch, cmd, PULSE_VIOLENCE *1.5); 
    return retval;
  }
  extern bool charExists(char_data *ch);
  if (!charExists(victim))// heh
  {
    add_command_lag(ch, cmd, PULSE_VIOLENCE *1.5); 
      return eSUCCESS|eVICT_DIED;
  }
  WAIT_STATE(ch, PULSE_VIOLENCE*2);

  // If we're intended to have a dual backstab AND we still can
  if (perform_dual_backstab == true) {
    if (was_in == ch->in_room) {
      if (AWAKE(victim) && !skill_success(ch,victim, SKILL_BACKSTAB)) {
	retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB, SECOND);
      } else {
	retval = attack(ch, victim, SKILL_BACKSTAB, SECOND);
      }
      
      if (retval & eVICT_DIED && !retval & eCH_DIED) {
	if(!IS_NPC(ch) && IS_SET(ch->pcdata->toggles, PLR_WIMPY)) {
	  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
	} else {
	  WAIT_STATE(ch, PULSE_VIOLENCE);
	}
      }
    } else {
      // We were intended to have a dual backstab so we were unjoinable
      // for the first stab, but apparently we moved, so there will be no
      // second stab. We will kick off check_autojoiner now since it didnt
      // run last time.
      check_autojoiners(ch, 0);
    }
  }

  if (!SOMEONE_DIED(retval)) {
    SET_BIT(retval, check_autojoiners(ch,1));
    if (!SOMEONE_DIED(retval))

    if (IS_AFFECTED(ch, AFF_CHARM)) SET_BIT(retval, check_joincharmie(ch,1));
    if (SOMEONE_DIED(retval)) return retval;
  } else
    add_command_lag(ch, cmd, (int)((double)PULSE_VIOLENCE )); 

  return retval;
}

int do_circle(CHAR_DATA *ch, char *argument, int cmd)
{
   CHAR_DATA * victim;
   int retval;

   if(IS_MOB(ch))
     ;
   else if(!has_skill(ch, SKILL_CIRCLE)) {
     send_to_char("You do not know how to circle!\r\n", ch);
     return eFAILURE;
   }

   int min_hp = (int) (GET_MAX_HIT(ch) / 5);
   min_hp = MIN( min_hp, 25 );

   if( GET_HIT(ch) < min_hp ) {
      send_to_char("You are feeling too weak right now to attempt such a bold maneuver.", ch);
      return eFAILURE;
   }

   if (ch->fighting)
      victim = ch->fighting;
   else {
      send_to_char("You have be in combat to perform this action.  Try using 'backstab' instead.\r\n", ch);
      return eFAILURE;
   }

   if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_HUGE) &&
	has_skill(ch, SKILL_CIRCLE) <= 80) {
      send_to_char("You cannot circle behind someone that HUGE!\n\r", ch);
      return eFAILURE;
   }

   if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_SWARM) &&
	has_skill(ch, SKILL_CIRCLE) <= 80) {
      send_to_char("You cannot pick just one to circle behind!\n\r", ch);
      return eFAILURE;
   }

   if (IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_TINY) &&
	has_skill(ch, SKILL_CIRCLE) <= 80) {
      send_to_char("You cannot target something that tiny to circle behind!\n\r", ch);
      return eFAILURE;
   }

   if(IS_AFFECTED(victim, AFF_NO_CIRCLE)) {
      act("$N notices your attempt and turns $S back away from you!", ch, 0, victim, TO_CHAR, 0);
      act("$N notices $n's attempt to circle behind $M and backs away quickly!", ch, 0, victim, TO_ROOM, NOTVICT);
      act("You see $n try to circle around you and move quickly to block $s access!", ch, 0, victim, TO_VICT, 0);
      return eFAILURE;
   }
       
   if (victim == ch) {
      send_to_char("How can you sneak up on yourself?\n\r", ch);
      return eFAILURE;
   }
    
   if (!ch->equipment[WIELD]) {
      send_to_char("You need to wield a weapon to make it a success.\n\r", ch);
      return eFAILURE;
   }

   // Check the killer/victim
   if ((GET_LEVEL(ch) < G_POWER) || IS_NPC(ch)) {
     if (!can_attack(ch) || !can_be_attacked(ch, victim))
       return eFAILURE;
   }

   bool blackjack = FALSE;
   switch (get_weapon_damage_type(ch->equipment[WIELD])) {
   case TYPE_PIERCE:
   case TYPE_STING:
     break;
   case TYPE_BLUDGEON:
     if (has_skill(ch, SKILL_BLACKJACK))
       blackjack = TRUE;
     break;
   default:
     send_to_char("Only certain weapons can be used for backstabbing or blackjacking, this isn't one of them.\n\r", ch);
     return eFAILURE;
     break;
   }
   
   if (ch == victim->fighting) {
      send_to_char("You can't break away while that person is hitting you!\n\r", ch);
      return eFAILURE;
   }
      
   act ("You circle around your target...",  ch, 0, 0, TO_CHAR, 0);
   act ("$n circles around $s target...", ch, 0, 0, TO_ROOM, INVIS_NULL);
   WAIT_STATE(ch, PULSE_VIOLENCE * 2);

   char buffer[255];
   sprintf(buffer, "%s", GET_NAME(victim));

   if (AWAKE(victim) && !skill_success(ch,victim,SKILL_CIRCLE))
     if (blackjack)
       retval = damage(ch, victim, 0,TYPE_UNDEFINED, SKILL_BLACKJACK, FIRST);
     else
       retval = damage(ch, victim, 0,TYPE_UNDEFINED, SKILL_BACKSTAB, FIRST);
   else 
   {
      SET_BIT(ch->combat, COMBAT_CIRCLE);

      if (blackjack)
	return do_blackjack(ch, buffer, cmd);
      else
	retval = one_hit(ch, victim, SKILL_BACKSTAB, FIRST);

      if(SOMEONE_DIED(retval))
        return retval;

      // Now go for dual backstab
      if (has_skill(ch, SKILL_DUAL_BACKSTAB) &&
          ((GET_CLASS(ch) == CLASS_THIEF) || (GET_LEVEL(ch) >= ARCHANGEL)))
         if (ch->equipment[SECOND_WIELD])
            if ((ch->equipment[SECOND_WIELD]->obj_flags.value[3] == 11) ||
                (ch->equipment[SECOND_WIELD]->obj_flags.value[3] == 9)) {
	               WAIT_STATE(ch, PULSE_VIOLENCE);

               if (AWAKE(victim) && !skill_success(ch,victim,SKILL_DUAL_BACKSTAB))
                  retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB,
                         SECOND);
               else {
                  SET_BIT(ch->combat, COMBAT_CIRCLE);
                  retval = one_hit(ch, victim, SKILL_BACKSTAB, SECOND);
                  }
               } // end of if that checks weapon's validity
   //} // end of else
   }

  if (!SOMEONE_DIED(retval)) {
    SET_BIT(retval, check_autojoiners(ch,1));
    if (!SOMEONE_DIED(retval))
    if (IS_AFFECTED(ch, AFF_CHARM)) SET_BIT(retval, check_joincharmie(ch,1));
    if (SOMEONE_DIED(retval)) return retval;
  }

   return retval;
}

int do_trip(CHAR_DATA *ch, char *argument, int cmd)
{
  CHAR_DATA *victim = 0;
  char name[256];
  int retval;

  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
    ;
  else if(!has_skill(ch, SKILL_TRIP)) {
    send_to_char("You should learn how to trip first!\r\n", ch);
    return eFAILURE;
  }

  one_argument(argument, name);
  if (GET_CLASS(ch) == CLASS_BARD)
  {
    if (ch->song_timer && ch->song_number == 26) /* Crushing crescendo */
    {
       send_to_char("You are too distracted by your song to do this.\r\n",ch);
	return eFAILURE;
    }
  }
  if(!(victim = get_char_room_vis(ch, name))) {
    if(ch->fighting)
      victim = ch->fighting;
    else {
      send_to_char( "Trip whom?\n\r", ch );
      return eFAILURE;
    }
  }

  if(ch->in_room != victim->in_room) {
    send_to_char("That person seems to have left.\n\r", ch);
    return eFAILURE;
  }
  
  if(victim == ch) {
    send_to_char("(You would look pretty silly trying to trip yourself.)\n\r", ch);
    return eFAILURE;
  }

  if(!can_be_attacked(ch, victim) || !can_attack(ch))
    return eFAILURE;

  if (IS_SET(victim->combat, COMBAT_BLADESHIELD1) || IS_SET(victim->combat, COMBAT_BLADESHIELD2)) {
        send_to_char("Tripping a bladeshielded opponent would be impossible!\n\r", ch);
        return eFAILURE;
  }

  if(affected_by_spell(victim, SPELL_IRON_ROOTS)) {
    act("You try to trip $N but tree roots around $S legs keep $M upright.", ch, 0, victim, TO_CHAR, 0);
    act("$n trips you but the roots around your legs keep you from falling.", ch, 0, victim, TO_VICT, 0);
    act("The tree roots support $N keeping $M from falling after $n's trip.", ch, 0, victim, TO_ROOM, NOTVICT);
    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
    return eFAILURE;
  }

  int modifier = get_stat(ch, DEX) - get_stat(victim, DEX);
  if (modifier > 10)
    modifier = 10;
  if (modifier < -10)
    modifier = -10;

  if(!skill_success(ch,victim,SKILL_TRIP, modifier)) {
    act("$n fumbles clumsily as $e attempts to trip you!", ch, NULL, victim, TO_VICT, 0 );
    act("You fumble the trip!", ch, NULL, victim, TO_CHAR , 0);
    act("$n fumbles as $e tries to trip $N!", ch, NULL, victim, TO_ROOM, NOTVICT );
    WAIT_STATE(ch, PULSE_VIOLENCE*2);
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_TRIP, 0);
  }
  else {
    act("$n trips you and you go down!", ch, NULL, victim, TO_VICT , 0);
    act("You trip $N and $N goes down!", ch, NULL, victim, TO_CHAR , 0);
    act("$n trips $N and $N goes down!", ch, NULL, victim, TO_ROOM, NOTVICT );
    if(GET_POS(victim) > POSITION_SITTING)
       GET_POS(victim) = POSITION_SITTING;
    SET_BIT(victim->combat, COMBAT_BASH2);
    WAIT_STATE(ch, PULSE_VIOLENCE*2);
    WAIT_STATE(victim, PULSE_VIOLENCE*1);
    retval = damage(ch, victim, 0, TYPE_UNDEFINED, SKILL_TRIP, 0);
  }
  return retval;
}

int do_sneak(CHAR_DATA *ch, char *argument, int cmd)
{
   affected_type af;

   if((ch->in_room >= 0 && ch->in_room <= top_of_world) &&
     IS_SET(world[ch->in_room].room_flags, ARENA) && arena.type == POTATO) {
       send_to_char("You can't do that in a potato arena ya sneaky bastard!\n\r", ch);
       return eFAILURE;
  }

   if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
      ;
   else if(!has_skill(ch, SKILL_SNEAK)) {
      send_to_char("You just don't seem like the sneaky type.\r\n", ch);
      return eFAILURE;
   }

   if (IS_AFFECTED(ch, AFF_SNEAK))  
   {
      affect_from_char(ch, SKILL_SNEAK);
      if (cmd != 10) {
         send_to_char("You won't be so sneaky anymore.\n\r", ch);
         return eFAILURE;
      }
   }

   do_hide(ch, "", 12);

   send_to_char("You try to move silently for a while.\n\r", ch);

//   skill_increase_check(ch, SKILL_SNEAK, has_skill(ch,SKILL_SNEAK), 
//SKILL_INCREASE_HARD);

   af.type = SKILL_SNEAK;
   af.duration = MAX(5, GET_LEVEL(ch) / 2);
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bitvector = AFF_SNEAK;
   affect_to_char(ch, &af);
   return eSUCCESS;
}

int do_stalk(CHAR_DATA *ch, char *argument, int cmd)
{
  char name[MAX_STRING_LENGTH];
  CHAR_DATA *leader;


  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
    ;
  else if(!has_skill(ch, SKILL_STALK)) {
    send_to_char("I bet you think you're a thief. ;)\n\r", ch);
    return eFAILURE;
  } 

  if(!(*argument)) {
    if(ch->master) csendf(ch, "You are currently stalking %s.\n\r", GET_SHORT(ch->master));
    else send_to_char("Pick a name, any name.\n\r", ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  if(!(leader = get_char_room_vis(ch, name))) {
    send_to_char("I see no person by that name here!\n\r", ch);
    return eFAILURE;
  }

  if(leader == ch) {
    if(!ch->master) 
      send_to_char("You are already following yourself.\n\r", ch);
    else if(IS_AFFECTED(ch, AFF_GROUP)) 
      send_to_char("You must first abandon your group.\n\r",ch);
    else 
      stop_follower(ch, 1);
    return eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_GROUP))
  {
    send_to_char("You must first abandon your group.\r\n",ch);
    return eFAILURE;
  }
  if(GET_MOVE(ch) < 10) {
    send_to_char("You are too tired to stealthily follow somebody.\n\r", ch);
    return eFAILURE;
  }
  GET_MOVE(ch) -= 10;

  WAIT_STATE(ch, PULSE_VIOLENCE*1);

  if(!skill_success(ch,leader,SKILL_STALK))
    do_follow(ch, argument, 9);

  else { 
    do_follow(ch, argument, 10);
    do_sneak(ch, argument, 10);
  }
  return eSUCCESS;
}

int do_hide(CHAR_DATA *ch, char *argument, int cmd)
{

  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
    ;
  else if(!has_skill(ch, SKILL_HIDE)) {
    if (cmd != 12)
      send_to_char("I bet you think you're a thief. ;)\n\r", ch);
    return eFAILURE;
  } 

  if((ch->in_room >= 0 && ch->in_room <= top_of_world) &&
     IS_SET(world[ch->in_room].room_flags, ARENA) && arena.type == POTATO) {
       send_to_char("You can't do that in a potato arena ya sneaky bastard!\n\r", ch);
       return eFAILURE;
  }

   for(char_data * curr = world[ch->in_room].people;
       curr;
       curr = curr->next_in_room)
   {
      if(curr->fighting == ch) {
         send_to_char("In the middle of combat?!  Impossible!\r\n", ch);
         return eFAILURE;
      }
   }

   send_to_char("You attempt to hide yourself.\n\r", ch);

   if ( ! IS_AFFECTED(ch, AFF_HIDE) )
      SETBIT(ch->affected_by, AFF_HIDE);
  /* See how well it worked on those currently in the room. */
    int a,i;
    CHAR_DATA *temp;
    if (!IS_NPC(ch) && (a = has_skill(ch, SKILL_HIDE)))
    {
        for (i = 0; i < MAX_HIDE;i++)
          ch->pcdata->hiding_from[i] = NULL;
        i = 0;
        for (temp = world[ch->in_room].people; temp; temp = temp->next_in_room)
        {
	  if (ch == temp) continue;
	  if (i >= MAX_HIDE) break;
          if (number(1,101) > a) // Failed.
          {
            ch->pcdata->hiding_from[i] = temp;
            ch->pcdata->hide[i++] = FALSE;
          } else {
            ch->pcdata->hiding_from[i] = temp;
            ch->pcdata->hide[i++] = TRUE;
          }
        }
   }
   return eSUCCESS;
}

int max_level(CHAR_DATA *ch)
{
  int i = 0,lvl = 0;
  for (; i < MAX_WEAR; i++)
    if (ch->equipment[i] && (GET_ITEM_TYPE(ch->equipment[i]) == ITEM_ARMOR ||
GET_ITEM_TYPE(ch->equipment[i]) == ITEM_WEAPON || GET_ITEM_TYPE(ch->equipment[i]) == 
ITEM_INSTRUMENT ||GET_ITEM_TYPE(ch->equipment[i]) == ITEM_FIREWEAPON ||
GET_ITEM_TYPE(ch->equipment[i]) == ITEM_LIGHT || GET_ITEM_TYPE(ch->equipment[i]) == ITEM_CONTAINER) && 
!IS_SET(ch->equipment[i]->obj_flags.extra_flags, ITEM_SPECIAL))
        lvl = MAX(lvl, ch->equipment[i]->obj_flags.eq_level);
  if (lvl < 20) lvl = 20;
  return lvl;
}

// steal an ITEM... not gold
int do_steal(CHAR_DATA *ch, char *argument, int cmd)
{
  CHAR_DATA *victim;
  struct obj_data *obj, *loop_obj, *next_obj;
  struct affected_type pthiefaf, *paf;
  char victim_name[240];
  char obj_name[240];
  char buf[240];
  int eq_pos;
  int _exp;
  int retval;
  obj_data * has_item = NULL;
  bool ohoh = FALSE;
  int chance = GET_HITROLL(ch) + has_skill(ch, SKILL_STEAL) / 4;
  extern struct index_data *obj_index;

  argument = one_argument(argument, obj_name);
  one_argument(argument, victim_name);
  if (ch->c_class != CLASS_THIEF || !has_skill(ch, SKILL_STEAL))
  {
	send_to_char("You are not experienced within that field.\r\n",ch);
	return eFAILURE;
  }
//  if (affected_by_spell(ch, FUCK_PTHIEF)) {
//     send_to_char("You're too busy watching your back to steal anything right now!\r\n",ch);
//     return eFAILURE;
//  }
  pthiefaf.type = FUCK_PTHIEF;
  pthiefaf.duration = 10;
  pthiefaf.modifier = 0;
  pthiefaf.location = APPLY_NONE;
  pthiefaf.bitvector = -1;

  if(!(victim = get_char_room_vis(ch, victim_name))) {
    send_to_char("Steal what from who?\n\r", ch);
    return eFAILURE;
  }
  else if (victim == ch) {
    send_to_char("Got it!\n\rYou receive 30000000000 experience.\n\r", ch);
    return eFAILURE;
  }

  if(GET_POS(victim) == POSITION_DEAD) {
     send_to_char("Don't steal from dead people!\r\n", ch);
     return eFAILURE;
  }

  if(IS_MOB(ch))
     return eFAILURE;

  if((GET_LEVEL(ch) < (GET_LEVEL(victim) - 10))) {
    send_to_char("That person is far too experienced for you to steal from.\r\n", ch);
    return eFAILURE;
  }

  if(IS_SET(world[ch->in_room].room_flags, SAFE)) {
    send_to_char("No stealing permitted in safe areas!\n\r", ch);
    return eFAILURE;
  }
    
  if(IS_SET(world[ch->in_room].room_flags, ARENA)) {
     send_to_char("Do what!? This is an Arena, go kill someone!\n\r", ch);
     return eFAILURE;
  }

  if(IS_AFFECTED(ch, AFF_CHARM)) {
     return do_say(ch, "Nice try, silly thief.", 9);
  }

  if(victim->fighting) {
     send_to_char("You can't get close enough because of the fight.\n\r", ch);
     return eFAILURE;
  }

/*  if(!IS_NPC(victim) &&
    !(victim->desc) && !affected_by_spell(victim, FUCK_PTHIEF) ) {
    send_to_char("That person is not really there.\n\r", ch);
    return eFAILURE;
  }*/

  if(GET_MOVE(ch) < 6) {
    send_to_char("You are too tired to sneak up on anybody.\n\r", ch);
    return eFAILURE;
  }
  GET_MOVE(ch) -= 6;

  WAIT_STATE(ch, 12); /* It takes TIME to steal */

//  if(GET_POS(victim) <= POSITION_SLEEPING &&
  //   GET_POS(victim) != POSITION_STUNNED)
    //percent = -1; /* ALWAYS SUCCESS */

  if((obj = get_obj_in_list_vis(ch, obj_name, victim->carrying))) 
  {
   chance -= GET_OBJ_WEIGHT(obj);
    if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) 
    {
      send_to_char("That item is protected by the gods.\n\r", ch);
      return eFAILURE;
    }
    if (IS_SET(obj->obj_flags.extra_flags, ITEM_NEWBIE))
    {
	send_to_char("That piece of equipment is protected by the powerful magics of the MUD-school elders.\r\n",ch);
	return eFAILURE;
    }
    if(obj_index[obj->item_number].virt == CHAMPION_ITEM) {
       send_to_char("You must earn that flag, no stealing allowed!", ch);
       return eFAILURE;
    }
    if (IS_NPC(victim) && isname("prize", obj->name))
    {
      send_to_char("You have to HUNT the targets...its not a Treasture Steal!\r\n",ch);
      return eFAILURE;
    }
    if (GET_OBJ_WEIGHT(obj) > 50)
    {
	send_to_char("That item is too heavy to steal.\r\n",ch);
	return eFAILURE;
    }

    int mod = has_skill(ch, SKILL_STEAL) - chance; 
    if (!skill_success(ch,victim,SKILL_STEAL, 0-mod)) 
    {
      set_cantquit( ch, victim );
      send_to_char("Oops, that was clumsy...\n\r", ch);
      ohoh = TRUE;
      if(!number(0, 4)) {
        act("$n tried to steal something from you!", ch, 0, victim, TO_VICT, 0);
        act("$n tries to steal something from $N.", ch, 0, victim, TO_ROOM, INVIS_NULL|NOTVICT);
	REMBIT(ch->affected_by, AFF_HIDE);
      } else {
	act("You managed to keep $N unaware of your failed attempt.", ch, 0, victim, TO_CHAR, 0);
	return eFAILURE;
      }
    } 
    else 
    { /* Steal the item */
      if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) 
      {
        if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) 
        {
          move_obj(obj, ch);

          if(!IS_NPC(victim) || (ISSET(victim->mobdata->actflags,ACT_NICE_THIEF)))
            _exp = GET_OBJ_WEIGHT(obj) * 1000;
          else _exp = (GET_OBJ_WEIGHT(obj) * 1000);

          if(GET_POS(victim) <= POSITION_SLEEPING || IS_AFFECTED(victim, AFF_PARALYSIS))  
            _exp = 0;

          send_to_char("Got it!\n\r", ch);
          if(_exp) {
             GET_EXP(ch) += _exp; /* exp for stealing :) */
             sprintf(buf,"You receive %d experience.\n\r", _exp);
             send_to_char(buf, ch);
          }

          if(!IS_NPC(victim)) 
          {
            do_save(victim, "", 666);
            do_save(ch, "", 666);
            if(!AWAKE(victim))
            {
//              if(number(1, 3) == 1)
                send_to_char("You dream of someone stealing your equipment...\r\n", victim);

              // if i'm not a thief, or if I fail dex-roll wake up victim
//              if(GET_CLASS(ch) != CLASS_THIEF || number(1, 100) > GET_DEX(ch))
//              {
//                send_to_char("Oops...\r\n", ch);
              if((paf = affected_by_spell(victim, SPELL_SLEEP))&& paf->modifier == 1)
              {
                paf->modifier = 0; // make sleep no longer work
              }
                do_wake(ch, GET_NAME(victim), 9);
//              }
            }

            // if victim isn't a pthief
//            if(!affected_by_spell(victim, FUCK_PTHIEF) ) 
            {
              //set_cantquit( ch, victim );
              if(affected_by_spell(ch, FUCK_PTHIEF))
              {
                affect_from_char(ch, FUCK_PTHIEF);
                affect_to_char(ch, &pthiefaf);
              }
              else
                affect_to_char(ch, &pthiefaf);
            }
          }
          if(!IS_NPC(victim))
          {
            sprintf(log_buf,"%s stole %s[%d] from %s",
                 GET_NAME(ch), obj->short_description,  
                 obj_index[obj->item_number].virt, GET_NAME(victim));
            log(log_buf, ANGEL, LOG_MORTAL);
            for(loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content)
              logf(ANGEL, LOG_MORTAL, "The %s contained %s[%d]", 
                          obj->short_description,
                          loop_obj->short_description,
                          obj_index[loop_obj->item_number].virt);
          }
          obj_from_char(obj);
          has_item = search_char_for_item(ch, obj->item_number);
          obj_to_char(obj, ch);
          if(IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
                ( IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE) && has_item )
            )
          {
            csendf(ch, "Whoa!  The %s poofed into thin air!\r\n", obj->short_description);
            extract_obj(obj);
          }
          // check for no_trade inside containers
          else for(loop_obj = obj->contains; loop_obj; loop_obj = next_obj)
          {
             // this is 'else' since if the container was no_trade everything in it
             // has already been extracted
             next_obj = loop_obj->next_content;

             if(IS_SET(loop_obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
                ( IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE) && has_item )
               ) 
             {
                csendf(ch, "Whoa!  The %s inside the %s poofed into thin air!\r\n",
                           loop_obj->short_description, obj->short_description);
                extract_obj(loop_obj);
             }
          }
        }
        else
          send_to_char("You cannot carry that much weight.\n\r", ch);
      } else
        send_to_char("You cannot carry that many items.\n\r", ch);
    }
  }
  else // not in inventory
  {
    for(eq_pos = 0; (eq_pos < MAX_WEAR); eq_pos++)
    {
      if(victim->equipment[eq_pos] &&
        (isname(obj_name, victim->equipment[eq_pos]->name)) &&
        CAN_SEE_OBJ(ch,victim->equipment[eq_pos])) 
      {
        obj = victim->equipment[eq_pos];
        break;
      }
    }

    if(obj) 
    { // They're wearing it!
/*    if (max_level(ch) < obj->obj_flags.eq_level)
    {
	send_to_char("You find yourself unable to steal that.\r\n",ch);
	return eFAILURE;
    }*/

    chance -= GET_OBJ_WEIGHT(obj);
    if (GET_OBJ_WEIGHT(obj) > 50)
    {
	send_to_char("That item is too heavy to steal.\r\n",ch);
	return eFAILURE;
    }

     int wakey = 100;
       switch (eq_pos)
	{
	  case WEAR_FINGER_R:
	  case WEAR_FINGER_L:
	  case WEAR_NECK_1:
	  case WEAR_NECK_2:
	  case WEAR_EAR_L:
	  case WEAR_EAR_R:
	  case WEAR_WRIST_R:
	  case WEAR_WRIST_L:
	     wakey = 30;
	     break;
	  case WEAR_HANDS:
	  case WEAR_FEET:
	  case WEAR_WAISTE:
	  case WEAR_HEAD:
	  case WIELD:
	  case SECOND_WIELD:
	  case WEAR_LIGHT:
	  case HOLD:
	  case HOLD2:
	  case WEAR_SHIELD:
	     wakey = 60;
	     break;
	  case WEAR_BODY:
	  case WEAR_LEGS:
	  case WEAR_ABOUT:
	  case WEAR_FACE:
	  case WEAR_ARMS:
	    wakey = 90;
	    break;
	  default:
	    send_to_char("Something just screwed up. Tell an imm what you did.\r\n",ch);
	    return eFAILURE;	  
	};
	wakey -= GET_DEX(ch) /2;
      if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) 
      {
        send_to_char("That item is protected by the gods.\n\r", ch);
        return eFAILURE;
      }
      if (!has_skill(ch,SKILL_STEAL))
      {
	send_to_char("You don't know how to steal.\r\n",ch);
	return eFAILURE;
      }
    int mod = has_skill(ch, SKILL_STEAL) - chance; 

      if(GET_POS(victim) > POSITION_SLEEPING ||
         GET_POS(victim) == POSITION_STUNNED) 
      {
        send_to_char("Steal the equipment now? Impossible!\n\r", ch);
        return eFAILURE;
      }
    else if (!skill_success(ch,victim,SKILL_STEAL, 0-mod)) 
    {
      set_cantquit( ch, victim );
      ohoh = TRUE;
      send_to_char("Oops, that was clumsy...\r\n", ch);
      if((paf = affected_by_spell(victim, SPELL_SLEEP))&& paf->modifier == 1)
      {
        paf->modifier = 0; // make sleep no longer work
      }

      do_wake(ch, GET_NAME(victim), 9);
      act("$n tried to steal something from you, waking you up in the process.!", ch, 0, victim, TO_VICT, 0);
      act("$n fails stealing something from $N, waking $N up in the process.", ch, 0, victim, TO_ROOM, INVIS_NULL|NOTVICT);
    } 
    else if (!number(1,4))
    {
        act("You remove $p and attempt to steal it.", ch, obj ,0, TO_CHAR, 0);
	send_to_char("Your victim wakes up before you can complete the theft!\r\n",ch);
        act("$n tries to steal $p from $N, but fails.",ch,obj,victim,TO_ROOM, NOTVICT);
        obj_to_char(unequip_char(victim, eq_pos), victim);
        act("You awake to find $n removing some of your equipment.",ch,obj,victim,TO_VICT, 0);
	do_save(victim,"",666);
	set_cantquit(ch,victim);
        if((paf = affected_by_spell(victim, SPELL_SLEEP))&& paf->modifier == 1)
        {
           paf->modifier = 0; // make sleep no longer work
        }
        do_wake(ch, GET_NAME(victim), 9);
    }
    else 
    {
        act("You remove $p and steal it.", ch, obj ,0, TO_CHAR, 0);
        act("$n steals $p from $N.",ch,obj,victim,TO_ROOM, NOTVICT);
        obj_to_char(unequip_char(victim, eq_pos), ch);
        if(!IS_NPC(victim) || (ISSET(victim->mobdata->actflags,ACT_NICE_THIEF)))
          _exp = GET_OBJ_WEIGHT(obj);
        else
          _exp = (GET_OBJ_WEIGHT(obj) * GET_LEVEL(victim));
        if(GET_POS(victim) <= POSITION_SLEEPING)    _exp = 1; 
        GET_EXP(ch) += _exp;                   /* exp for stealing :) */ 
        sprintf(buf,"You receive %d exps.\n\r", _exp);
        send_to_char(buf, ch);
        sprintf(buf,"%s stole %s from %s while victim was asleep",
                GET_NAME(ch), obj->short_description, GET_NAME(victim));
        log(buf, ANGEL, LOG_MORTAL);
        if(!IS_MOB(victim)) 
        {
          do_save(victim, "", 666);
          do_save(ch, "", 666);
          if(!AWAKE(victim))
          {
//            if(number(1, 3) == 1)
             send_to_char("You dream of someone stealing your equipment...\r\n", victim);

            // if i'm not a thief, or if I fail dex-roll wake up victim
//            if(number(1,101) > wakey)
//            {
//              send_to_char("Oops, that was clumsy...\r\n", ch);
             if((paf = affected_by_spell(victim, SPELL_SLEEP))&& paf->modifier == 1)
             {
                paf->modifier = 0; // make sleep no longer work
             }
             do_wake(ch, GET_NAME(victim), 9);
//            }

          }

          // You don't get a thief flag from stealing from a pthief
//          if(!affected_by_spell(victim, FUCK_PTHIEF)) 
          {
            if(affected_by_spell(ch, FUCK_PTHIEF))
            {
              affect_from_char(ch, FUCK_PTHIEF);
              affect_to_char(ch, &pthiefaf);
            }
            else
              affect_to_char(ch, &pthiefaf);
          }  
        } // !is_npc
        obj_from_char(obj);
        has_item = search_char_for_item(ch, obj->item_number);
        obj_to_char(obj, ch);
        if(IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
            ( IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE) && has_item )
          )
        {
          send_to_char("Whoa! It poofed into thin air!\r\n", ch);
          extract_obj(obj);
        }
        else for(loop_obj = obj->contains; loop_obj; loop_obj = next_obj)
        {
           // this is 'else' since if the container was no_trade everything in it
           // has already been extracted
           next_obj = loop_obj->next_content;

           if(IS_SET(loop_obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
               ( IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE) && has_item )
             ) 
           {
              csendf(ch, "Whoa! The %s inside the %s poofed into thin air!\r\n",
                         loop_obj->short_description, obj->short_description);
              extract_obj(loop_obj);
           }
        }
      } // else
    } // if(obj)
    else
    { // they don't got it
      act("$N does not seem to possess that item.",ch,0,victim,TO_CHAR, 0);
      return eFAILURE;
    }
  } // of else, not in inventory

  if (ohoh && IS_NPC(victim) && AWAKE(victim) && GET_LEVEL(ch)<ANGEL)
  {
    if (ISSET(victim->mobdata->actflags, ACT_NICE_THIEF)) 
    {
      sprintf(buf, "%s is a bloody thief.", GET_SHORT(ch));
      do_shout(victim, buf, 0);
    } else 
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }
  }
  return eSUCCESS;
}

// Steal gold 
int do_pocket(CHAR_DATA *ch, char *argument, int cmd)
{
  CHAR_DATA *victim;
  struct affected_type pthiefaf;
  char victim_name[240];
  char buf[240];
  int gold;
  int _exp;
  int retval;
  bool ohoh = FALSE;

  one_argument(argument, victim_name);

  pthiefaf.type = FUCK_GTHIEF;
  pthiefaf.duration = 6;
  pthiefaf.modifier = 0;
  pthiefaf.location = APPLY_NONE;
  pthiefaf.bitvector = -1;

  if(!(victim = get_char_room_vis(ch, victim_name))) {
    send_to_char("Steal what from who?\n\r", ch);
    return eFAILURE;
  }
  else if (victim == ch) {
    send_to_char("Got it!\n\rYou receive 30000000000 experience.\n\r", ch);
    return eFAILURE;
  }

  if(GET_POS(victim) == POSITION_DEAD) {
     send_to_char("Don't steal from dead people.\r\n", ch);
     return eFAILURE;
  }

  if(IS_MOB(ch))
     return eFAILURE;

  if((GET_LEVEL(ch) < (GET_LEVEL(victim) - 10))) {
    send_to_char("That person is far too experienced to steal from.\r\n", ch);
    return eFAILURE;
  }

  if(IS_SET(world[ch->in_room].room_flags, SAFE)) {
    send_to_char("No stealing permitted in safe areas!\n\r", ch);
    return eFAILURE;
  }
    
  if(IS_SET(world[ch->in_room].room_flags, ARENA)) {
     send_to_char("Do what!? This is an Arena, go kill someone!\n\r", ch);
     return eFAILURE;
  }

  if(IS_AFFECTED(ch, AFF_CHARM)) {
     return do_say(ch, "Nice try.", 9);
  }

  if(victim->fighting) {
     send_to_char("You can't get close enough because of the fight.\n\r", ch);
     return eFAILURE;
  }


  /*if(!IS_NPC(victim) &&
    !(victim->desc) && !affected_by_spell(victim, FUCK_PTHIEF) ) {
    send_to_char("That person is not really there.\n\r", ch);
    return eFAILURE;
  }
*/
  if(!has_skill(ch,SKILL_POCKET))
  {
   send_to_char("Well, you would, if you knew how.\r\n",ch);
    return eFAILURE;
  }
 
  if(GET_MOVE(ch) < 6) {
    send_to_char("You are too tired to rob gold right now.\n\r", ch);
    return eFAILURE;
  }
  GET_MOVE(ch) -= 6;

  WAIT_STATE(ch, 20); /* It takes TIME to steal */


//    skill_increase_check(ch, SKILL_POCKET, has_skill(ch,SKILL_POCKET),SKILL_INCREASE_MEDIUM);

  if (!skill_success(ch, victim, SKILL_POCKET)) 
  {
    set_cantquit( ch, victim );
    send_to_char("Oops, that was clumsy...\r\n", ch);
    ohoh = TRUE;
	    if(number(0, 6)) {
      act("You discover that $n has $s hands in your wallet.", ch,0,victim,TO_VICT, 0);
      act("$n tries to steal gold from $N.", ch, 0, victim, TO_ROOM, NOTVICT|INVIS_NULL);
	REMBIT(ch->affected_by, AFF_HIDE);
    } else {
      act("You manage to keep $N unaware of your botched attempt.", ch, 0, victim, TO_CHAR, 0);
      return eFAILURE;
    }
  } else 
  {
    int learned = has_skill(ch, SKILL_POCKET);
    int percent = 7 + (learned > 40) + (learned > 60) + (learned > 80);
    
    // Steal some gold coins
    gold = (int) ((float)(GET_GOLD(victim))*(float)((float)percent/100.0));
    gold = MIN(10000000, gold);
    if (gold > 0) {
      GET_GOLD(ch)     += gold;
      GET_GOLD(victim) -= gold;
      _exp = gold / 100 * GET_LEVEL(victim) / 5;
	if (!IS_NPC(victim)) _exp = 0;
      if(IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_NICE_THIEF)) _exp = 1; 
      if(GET_POS(victim) <= POSITION_SLEEPING || IS_AFFECTED(victim, AFF_PARALYSIS)) _exp = 0;

      sprintf(buf, "Nice work! You pilfered %d gold coins.\n\r", gold);
      send_to_char(buf, ch);
      if(_exp && _exp > 1) {
         GET_EXP(ch) += _exp; /* exp for stealing :) */
         sprintf(buf,"You receive %d experience.\n\r", _exp);
         send_to_char(buf, ch);
      }

      if(!IS_NPC(victim)) 
      {
        do_save(victim, "", 666);
        do_save(ch, "", 666);
        if(!affected_by_spell(victim, FUCK_GTHIEF) ) 
        {
          //set_cantquit( ch, victim );
          if(affected_by_spell(ch, FUCK_GTHIEF))
          {
            affect_from_char(ch, FUCK_GTHIEF);
            affect_to_char(ch, &pthiefaf);
          }
          else
            affect_to_char(ch, &pthiefaf);
        }
      }
      if ((GET_LEVEL(ch)<ANGEL) && (!IS_NPC(victim))) 
      {
        sprintf(log_buf,"%s stole %d gold from %s", GET_NAME(ch), gold, GET_NAME(victim));
        log(log_buf, ANGEL, LOG_MORTAL);
      }
    } else 
    {
      send_to_char("You couldn't get any gold...\n\r", ch);
    }
  }

  if (ohoh && IS_NPC(victim) && AWAKE(victim) && GET_LEVEL(ch)<ANGEL)
  {
    if (ISSET(victim->mobdata->actflags, ACT_NICE_THIEF)) 
    {
      sprintf(buf, "%s is a bloody thief.", GET_SHORT(ch));
      do_shout(victim, buf, 0);
    } else 
    {
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
    }
  }
  return eSUCCESS;
}

int do_pick(CHAR_DATA *ch, char *argument, int cmd)
{
   int door, other_room, j;
   char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
   struct room_direction_data *back;
   struct obj_data *obj;
   CHAR_DATA *victim;
   bool has_lockpicks = FALSE;

   argument_interpreter(argument, type, dir);

   if(!has_skill(ch,SKILL_PICK_LOCK)) {
      send_to_char("You don't know how to pick locks!\r\n", ch);
      return eFAILURE;
   }

  // for (obj = ch->carrying; obj; obj = obj->next_content)
  //    if (obj->obj_flags.type_flag == ITEM_LOCKPICK)
  //      has_lockpicks = TRUE;

   for (j=0; j<MAX_WEAR; j++)
     if (ch->equipment[j] && (ch->equipment[j]->obj_flags.type_flag == ITEM_LOCKPICK
	|| obj_index[ch->equipment[j]->item_number].virt == 504))
       has_lockpicks = TRUE;

   if (!has_lockpicks) {
      send_to_char("But...you don't have a lockpick!\r\n", ch);
      return eFAILURE;
   }

   if (!*type)
      send_to_char("Pick what?\n\r", ch);
   else if (generic_find(argument, (FIND_OBJ_INV | FIND_OBJ_ROOM), ch, &victim, &obj))

  // this is an object

  if (obj->obj_flags.type_flag != ITEM_CONTAINER)
      send_to_char("That's not a container.\n\r", ch);
  else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
      send_to_char("Silly, it's not even closed!\n\r", ch);
  else if (obj->obj_flags.value[2] < 0)
      send_to_char("Odd, you can't seem to find a keyhole.\n\r", ch);
  else if (!IS_SET(obj->obj_flags.value[1], CONT_LOCKED))
      send_to_char("Oh-ho! This thing is not even locked!\n\r", ch);
  else if (IS_SET(obj->obj_flags.value[1], CONT_PICKPROOF))
      send_to_char("The lock resists even your best attempts to pick it.\n\r", ch);
  else
  {
   if (!skill_success(ch,NULL,SKILL_PICK_LOCK)) {
      send_to_char("You failed to pick the lock.\n\r", ch);
      WAIT_STATE(ch, PULSE_VIOLENCE);
      return eFAILURE;
    }

      REMOVE_BIT(obj->obj_flags.value[1], CONT_LOCKED);
      send_to_char("*Click*\n\r", ch);
      act("$n fiddles with $p.", ch, obj, 0, TO_ROOM, 0);
  }
    else if ((door = find_door(ch, type, dir)) >= 0)
  if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\n\r", ch);
  else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("You realize that the door is already open!\n\r", ch);
  else if (EXIT(ch, door)->key < 0)
      send_to_char("You can't seem to spot any lock to pick.\n\r", ch);
  else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("Oh...it wasn't locked at all.\n\r", ch);
  else if (IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF))
      send_to_char("You seem to be unable to pick this lock.\n\r", ch);
  else
  {
      //skill_increase_check(ch, SKILL_PICK_LOCK, has_skill(ch,SKILL_PICK_LOCK), SKILL_INCREASE_MEDIUM);
   if (!skill_success(ch,NULL,SKILL_PICK_LOCK)) {
      send_to_char("You failed to pick the lock.\n\r", ch);
      WAIT_STATE(ch, PULSE_VIOLENCE);
      return eFAILURE;
    }

      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword)
    act("$n skillfully picks the lock of the $F.", ch, 0,
        EXIT(ch, door)->keyword, TO_ROOM, 0);
      else
    act("$n picks the lock of the.", ch, 0, 0, TO_ROOM,
      INVIS_NULL);
      send_to_char("The lock quickly yields to your skills.\n\r", ch);
      /* now for unlocking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
      if ( ( back = world[other_room].dir_option[rev_dir[door]] ) != 0 )
        if (back->to_room == ch->in_room)
      REMOVE_BIT(back->exit_info, EX_LOCKED);
  }
  return eSUCCESS;
}


int do_slip(struct char_data *ch, char *argument, int cmd)
{
   char obj_name[200], vict_name[200], buf[200];
   char arg[MAX_INPUT_LENGTH];
   int amount;
   struct char_data *vict;
   struct obj_data *obj, *tmp_object, *container;

   extern int weight_in(OBJ_DATA *);

    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF) ) { 
      send_to_char("Your criminal acts prohibit this action.\n\r", ch);
      return eFAILURE;
    }
   if (!has_skill(ch, SKILL_SLIP))
   {
      send_to_char("You don't know how to slip.\r\n",ch);
      return eFAILURE;
   }
   argument = one_argument(argument, obj_name);

   if (is_number(obj_name)) { 
      if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) {
         send_to_char("Your criminal acts prohibit this action.\n\r", ch);
         return eFAILURE;
      }
      if (strlen(obj_name) > 7) {
         send_to_char("Number field too large.  Try something smaller.\n\r", ch);
         return eFAILURE;
         }
   
      amount   = atoi(obj_name);
      argument = one_argument(argument, arg);
      
      if (str_cmp("coins", arg) && str_cmp("coin", arg)) { 
         send_to_char("Sorry, you can't do that (yet)...\n\r",ch);
         return eFAILURE;
         }
      if (amount <= 0) { 
         send_to_char("Sorry, you can't do that!\n\r",ch);
         return eFAILURE;
         }
      if ((GET_GOLD(ch) < (uint32)amount) && (GET_LEVEL(ch) < DEITY)) { 
         send_to_char("You haven't got that many coins!\n\r",ch);
         return eFAILURE;
         }
    
      argument = one_argument(argument, vict_name);
    
      if (!*vict_name) { 
         send_to_char("To who?\n\r", ch);
         return eFAILURE;
         }
      if (!(vict = get_char_room_vis(ch, vict_name))) {
         send_to_char("To who?\n\r", ch);
         return eFAILURE;
         }
      if (ch == vict) {
         send_to_char("To yourself?!  Very cute...\n\r", ch);
         return eFAILURE;
      }    
      // Failure
      if (!skill_success(ch,vict,SKILL_SLIP)) {
         send_to_char("Whoops!  You dropped the coins!\n\r", ch);
         if (GET_LEVEL(ch) >= IMMORTAL) { 
            sprintf(buf, "%s tries to slip %d coins to %s and drops them!", GET_NAME(ch),
              amount, GET_NAME(vict));
            special_log(buf);
            }
      
         act("$n tries to slip you some coins, but $e accidentally drops "
             "them.\n\r", ch, 0, vict, TO_VICT, 0);
         act("$n tries to slip $N some coins, but $e accidentally drops "
             "them.\n\r", ch, 0, vict, TO_ROOM, NOTVICT);
    
         if (IS_NPC(ch) || (GET_LEVEL(ch) < DEITY))
            GET_GOLD(ch) -= amount;
      
         tmp_object = create_money(amount);
         obj_to_room(tmp_object, ch->in_room);
         do_save(ch, "", 9);
         } // failure
    
      // Success
      else {
         send_to_char("Ok.\n\r", ch);
         if (GET_LEVEL(ch) >= IMMORTAL) { 
            sprintf(buf, "%s slips %d coins to %s", GET_NAME(ch), amount,
                    GET_NAME(vict));
            special_log(buf);
            } 
      
         sprintf(buf, "%s slips you %d gold coins.\n\r", PERS(ch, vict),
           amount);
         act(buf, ch, 0, vict, TO_VICT, GODS);
         act("$n slips some gold to $N.", ch, 0, vict, TO_ROOM, GODS|NOTVICT);

        sprintf(buf, "%s slips %d coins to %s", GET_NAME(ch), amount,
                GET_NAME(vict));
        log(buf, IMP, LOG_OBJECTS);

    
         if (IS_NPC(ch) || (GET_LEVEL(ch) < DEITY))
            GET_GOLD(ch) -= amount;
      
         GET_GOLD(vict) += amount;
         do_save(ch, "", 9);
         save_char_obj(vict);
         }
    
      return eFAILURE;
      } // if (is_number)

   argument = one_argument(argument, vict_name);

   if (!*obj_name || !*vict_name) {
      send_to_char("Slip what to who?\n\r", ch);
      return eFAILURE;
      }
      
   if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying))) {
      send_to_char("You do not seem to have anything like that.\n\r", ch);
      return eFAILURE;
      }
      
   if (IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) {
      send_to_char("That sure would be a stupid thing to do.\n\r", ch);
      return eFAILURE;
      }
     
   if(IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE)) {
      send_to_char("You can't seem to get the item to leave you.\n\r", ch);
      return eFAILURE;
   }
 
   if (IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP))
      if (GET_LEVEL(ch) < DEITY) {
         send_to_char("You can't let go of it! Yeech!!\n\r", ch);
         return eFAILURE;
         }
      else
         send_to_char("This item is NODROP btw.\n\r", ch);

   if(GET_ITEM_TYPE(obj) == ITEM_CONTAINER)
   {
     send_to_char("That would ruin it!\n\r", ch);
     return eFAILURE;
   }

   // We're going to slip the item into our container instead
   if((container = get_obj_in_list_vis(ch, vict_name, ch->carrying)))
   {
      if(GET_ITEM_TYPE(container) != ITEM_CONTAINER) {
         send_to_char("That's not a container.\r\n", ch);
         return eFAILURE;
      }
      if(IS_SET(container->obj_flags.value[1], CONT_CLOSED)) {
         send_to_char("It seems to be closed.\r\n", ch);
         return eFAILURE;
      }
      if(((container->obj_flags.weight + obj->obj_flags.weight) >=
            container->obj_flags.value[0]) && (obj_index[container->item_number].virt != 536 || 
            weight_in(container) + obj->obj_flags.weight >= 200)) {
         send_to_char("It won't fit...cheater.\r\n", ch);
         return eFAILURE;
      }
      if (!skill_success(ch,NULL,SKILL_SLIP)) { // fail
         act("$n tries to stealthily slip $p in $P, but you notice $s motions.", ch, obj,
             container, TO_ROOM, 0);
      }
      else act("$n slips $p in $P.", ch, obj, container, TO_ROOM, GODS);
      move_obj(obj, container);
      // fix weight (move_obj doesn't re-add it, but it removes it)
      if (obj_index[container->item_number].virt != 536)
         IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);
      send_to_char("Ok.\r\n", ch);
      return eSUCCESS;      
   }
   if (!(vict = get_char_room_vis(ch, vict_name))) {
      send_to_char("No one by that name around here.\n\r", ch);
      return eFAILURE;
      }

   if (IS_NPC(vict) && mob_index[vict->mobdata->nr].non_combat_func == shop_keeper) {
      act("$N graciously refuses your gift.", ch, 0, vict, TO_CHAR, 0);
      return eFAILURE;
      }

   if (ch == vict) {
      send_to_char("To yourself?!  Very cute...\n\r", ch);
      return eFAILURE;
   }    

   if(affected_by_spell(ch, FUCK_PTHIEF) && !vict->desc) {
      send_to_char("Now WHY would a thief slip something to a "
             "linkdead character?\n\r", ch);
      return eFAILURE;
      }

   if ((1 + IS_CARRYING_N(vict)) > CAN_CARRY_N(vict)) {
      act("$N seems to have $S hands full.", ch, 0, vict, TO_CHAR, 0);
      return eFAILURE;
      }
      
   if (obj->obj_flags.weight + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
      act("$E can't carry that much weight.", ch, 0, vict, TO_CHAR, 0);
      return eFAILURE;
      }
    
   if(IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE)) {
     if(search_char_for_item(vict, obj->item_number)) {
        send_to_char("The item's uniqueness prevents it!\r\n", ch);
        return eFAILURE;
     }
   }

   //skill_increase_check(ch, SKILL_SLIP, has_skill(ch,SKILL_SLIP), SKILL_INCREASE_EASY);

   if (!skill_success(ch,vict,SKILL_SLIP)) {
      if(obj_index[obj->item_number].virt == 393) {
         send_to_char("Whoa, you almost dropped your hot potato!\n\r", ch);
         return eFAILURE;
      }

      if (GET_LEVEL(ch) >= IMMORTAL  && GET_LEVEL(ch) <= DEITY ) {
         sprintf(buf, "%s slips %s to %s and fumbles it.", GET_NAME(ch),
                 obj->short_description, GET_NAME(vict));
         special_log(buf);
         }
      
      move_obj(obj, ch->in_room);

      act("$n tries to slip you something, but $e accidentally drops "
          "it.\n\r", ch, 0, vict, TO_VICT, 0);
      act("$n tries to slip $N something, but $e accidentally drops "
          "it.\n\r", ch, 0, vict, TO_ROOM, NOTVICT);
      send_to_char("Whoops!  You dropped it.\n\r", ch);
      do_save(ch, "", 9);
      }
    
   // Success
   else {
/*      if (GET_LEVEL(ch) >= IMMORTAL  && GET_LEVEL(ch) <= DEITY ) {
         sprintf(buf, "%s slips %s to %s.", GET_NAME(ch),
                 obj->short_description, GET_NAME(vict));
         special_log(buf);
         }
  */ 
        sprintf(buf, "%s slips %s to %s", GET_NAME(ch), obj->name,
                GET_NAME(vict));
        log(buf, IMP, LOG_OBJECTS);

      move_obj(obj, vict);
      act("$n slips $p to $N.", ch, obj, vict, TO_ROOM, GODS|NOTVICT);
      act("$n slips you $p.", ch, obj, vict, TO_VICT, GODS);
      send_to_char("Ok.\n\r", ch);
      do_save(ch, "", 9);
      save_char_obj(vict);
      }
  return eSUCCESS;
}

int do_vitalstrike(struct char_data *ch, char *argument, int cmd)
{
  struct affected_type af;
    
  if(affected_by_spell(ch, SKILL_VITAL_STRIKE) && GET_LEVEL(ch) < IMMORTAL) {
    send_to_char("Your body is still recovering from your last vitalstrike technique.\r\n", ch);
    return eFAILURE;
  }
    
  if(IS_MOB(ch))
    ;
  else if(!has_skill(ch, SKILL_VITAL_STRIKE)) {
    send_to_char("You'd cut yourself to ribbons just trying!\r\n", ch);
    return eFAILURE;
  }

  if(!(ch->fighting)) {
    send_to_char("But you aren't fighting anyone!\r\n", ch);
    return eFAILURE;
  }

  //skill_increase_check(ch, SKILL_VITAL_STRIKE, has_skill(ch,SKILL_VITAL_STRIKE), SKILL_INCREASE_EASY);
  
  if(!skill_success(ch,NULL,SKILL_VITAL_STRIKE)) {
    act("$n starts jabbing $s weapons around $mself and almost chops off $s pinkie finger."
         , ch, 0, 0, TO_ROOM, NOTVICT);
    send_to_char("You try to begin the vital strike technique and nearly slice off your own pinkie finger!\r\n", ch);
  } 
  else {
    act("$n begins jabbing $s weapons with lethal accuracy and strength.", ch, 0, 0, TO_ROOM, NOTVICT);
    send_to_char("Your body begins to coil, the strength building inside of you, your mind\r\n"
                 "pinpointing vital and vulnerable areas....\r\n", ch);
    SET_BIT(ch->combat, COMBAT_VITAL_STRIKE);
  }   
   
  WAIT_STATE(ch, PULSE_VIOLENCE);

  // learned should have max of 80 for mortal thieves
  // this means you can use it once per tick
  
  int length = 9 - has_skill(ch,SKILL_VITAL_STRIKE) / 10;
  if (!IS_SET(ch->combat, COMBAT_VITAL_STRIKE)) length /= 2;
  if(length < 1)
    length = 1;
  af.type = SKILL_VITAL_STRIKE;
  af.duration  = length;
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  return eSUCCESS;
}


int do_deceit(struct char_data *ch, char *argument, int cmd)
{
  struct affected_type af;
  
  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
    ;
  else if(!has_skill(ch, SKILL_DECEIT)) {
    send_to_char("You do not yet understand enough of the workings of your marks.\r\n", ch);
    return eFAILURE;
  }   
      
  if(!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("You have no group to instruct!\r\n", ch);
    return eFAILURE;
  }   
      
  if (!skill_success(ch,NULL,SKILL_DECEIT)) {
     send_to_char("Your class just isn't up to the task.\r\n", ch);
     act ("$n tries to explain to you the weaknesses of others, but you do not understand.", ch, 0, 0, TO_ROOM, 0);
  }
  else {
    act ("$n instructs $s group on the virtues of deceit.", ch, 0, 0, TO_ROOM, 0);
    send_to_char("Your instruction is well received and your pupils are more able to exploit weaknesses.\r\n", ch);
    
    for(char_data * tmp_char = world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
    { 
      if(tmp_char == ch)
        continue;
      if(!ARE_GROUPED(ch, tmp_char))
        continue;
      affect_from_char(tmp_char, SKILL_DECEIT, SUPPRESS_MESSAGES);
      affect_from_char(tmp_char, SKILL_DECEIT, SUPPRESS_MESSAGES);
      act ("$n lures your mind into the thought patterns of the morally corrupt.", ch, 0, tmp_char, TO_VICT, 0);
  
      af.type      = SKILL_DECEIT;
      af.duration  = 1 + has_skill(ch,SKILL_DECEIT) / 10;
      af.modifier  = 1 + (has_skill(ch, SKILL_DECEIT) /20);
      af.location  = APPLY_MANA_REGEN;
      af.bitvector = -1;
      affect_to_char(tmp_char, &af);
      af.location  = APPLY_DAMROLL;
      affect_to_char(tmp_char, &af);
      af.location  = APPLY_HITROLL;
      affect_to_char(tmp_char, &af);
    }   
  }
    
  //skill_increase_check(ch, SKILL_DECEIT, has_skill(ch,SKILL_DECEIT), SKILL_INCREASE_EASY);
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
  GET_MOVE(ch) /= 2;
  return eSUCCESS;
}

int do_blackjack(struct char_data *ch, char *argument, int cmd)
{
  int retval, learned;

  if (!(learned = has_skill(ch, SKILL_BLACKJACK))) {
    send_to_char("You don't know how to blackjack.\r\n",ch);
    return eFAILURE;
  }

  if (!ch->equipment[WIELD] ||
      get_weapon_damage_type(ch->equipment[WIELD]) != TYPE_BLUDGEON)
    {
      send_to_char("Your primary weapon must bludgeon to make it a success.\r\n",ch);
      return eFAILURE;
    }

  char arg[MAX_INPUT_LENGTH];
  one_argument(argument,arg);
  struct char_data *victim;

  if (!(victim = get_char_room_vis(ch, arg)))
  {
    send_to_char("Blackjack whom?\r\n",ch);
    return eFAILURE;
  }
  if (victim == ch)
  {
    send_to_char("Why are you trying to hit yourself on the head???.\r\n",ch);
    return eFAILURE;
  }
  if(IS_AFFECTED(victim, AFF_ALERT)) {
    act("$E is too alert and nervous looking and you are unable to sneak behind!", ch,0,victim, TO_CHAR, 0);
    return eFAILURE;
  }  

  if(IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_HUGE)) {
    send_to_char("You cannot blackjack someone that HUGE!\r\n", ch);
    return eFAILURE;
  }

  if(IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_SWARM)) {
    send_to_char("You cannot target just one to blackjack!\r\n", ch);
    return eFAILURE;
  }

  if(IS_MOB(victim) && ISSET(victim->mobdata->actflags, ACT_TINY)) {
    send_to_char("You cannot target someone that tiny to blackjack!\r\n", ch);
    return eFAILURE;
  }

  if((GET_LEVEL(ch) < G_POWER) || IS_NPC(ch)) {
      if(!can_attack(ch) || !can_be_attacked(ch, victim))
      return eFAILURE;
  }

  set_cantquit(ch, victim);
  WAIT_STATE(ch, PULSE_VIOLENCE*3);
  if ( AWAKE(victim) ) { 
    int fail_percentage = (int)((((33.0-10.0) / 100.0) * learned) + 10.0);
    int work_percentage = (int)((((33.0-80.0) / 100.0) * learned) + 80.0);
    int rand_percentage = (int)((((33.0-10.0) / 100.0) * learned) + 10.0);

    struct affected_type af;
    af.type      = SKILL_BLACKJACK;
    af.duration  = 2;
    af.duration_type  = PULSE_VIOLENCE;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_BLACKJACK;
    
    int value = number(1, 100);

    //    fprintf(stderr, "l:%d f:%d w:%d r:%d v:%d\n\r", learned, fail_percentage, work_percentage, rand_percentage, value);
    
    skill_increase_check (ch, SKILL_BLACKJACK, has_skill(ch, SKILL_BLACKJACK),
			  SKILL_INCREASE_MEDIUM);

    if ((fail_percentage+work_percentage) < value &&
	value <= (fail_percentage+work_percentage+rand_percentage)) {
      // victim's next target will be random
      af.modifier = 1;     
      affect_to_char(victim, &af, PULSE_VIOLENCE);
      af.location = APPLY_AC;
      af.modifier = learned*1.5;
      affect_to_char(victim, &af, PULSE_VIOLENCE);
      retval = damage(ch, victim, 25, TYPE_BLUDGEON, SKILL_BLACKJACK, 0);
    } else if ( fail_percentage < value &&
		value <= (fail_percentage+work_percentage)) {
      // ch failed to blackjack victim
      retval = damage(ch, victim, 0, TYPE_BLUDGEON, SKILL_BLACKJACK, 0);
    } else if (0 < value && value <= fail_percentage) {
      af.modifier = 2;
      GET_POS(victim) = POSITION_SITTING;
      SET_BIT(victim->combat, COMBAT_BASH1);
      affect_to_char(victim, &af, PULSE_VIOLENCE);
      af.location = APPLY_AC;
      af.modifier = learned*1.5;
      affect_to_char(victim, &af, PULSE_VIOLENCE);
      retval = damage(ch, victim, 25, TYPE_BLUDGEON, SKILL_BLACKJACK, 0);
      // victim's next attack will fail
    }
  } else {
    retval = attack(ch, victim, SKILL_BLACKJACK, FIRST);
  }

  if (retval & eVICT_DIED && !retval & eCH_DIED)
  {
    if(!IS_NPC(ch) && IS_SET(ch->pcdata->toggles, PLR_WIMPY))
       WAIT_STATE(ch, PULSE_VIOLENCE *2);
    else
       WAIT_STATE(ch, PULSE_VIOLENCE);
    return retval;
  }

  if (retval & eCH_DIED) return retval;
  WAIT_STATE(ch, PULSE_VIOLENCE*2);

  if (retval & eVICT_DIED)
     return retval;

  return eSUCCESS;

}

int do_appraise(CHAR_DATA *ch, char *argument, int cmd)
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   char name[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
   char item[MAX_STRING_LENGTH];
   int appraised, bits, learned;
   bool found = FALSE, weight = FALSE;

   argument = one_argument(argument,name);
   one_argument(argument, item);

   if(!(learned = has_skill(ch, SKILL_APPRAISE))) {
      send_to_char("Your estimate would be baseless.\n\r", ch);
      return eFAILURE;
   }

   bits = generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM |
                  FIND_OBJ_EQUIP | FIND_CHAR_ROOM,
                  ch, &victim, &obj);

   if(!bits) {
     send_to_char("Appraise whom?\n\r", ch);
     return eFAILURE;
   }

   if(GET_MOVE(ch) < 25) {
      send_to_char("You're too tired to make a valid assessment.\n\r", ch);
      return eFAILURE;
   }
   GET_MOVE(ch) -=25;

   if(obj) {
      appraised = obj->obj_flags.cost;
      found = TRUE;
   }

   if(victim) {
      if(victim == ch && !*item) {
         send_to_char("You're worth a million bucks, baby.\n\r", ch);
         return eFAILURE;
      }

      if(*item) {
         if(victim == ch) {
            obj = get_obj_in_list_vis(ch, item, ch->carrying);
            if(obj) {
               appraised = GET_OBJ_WEIGHT(obj);
               found = TRUE;
               weight = TRUE;
            } else send_to_char("You don't seem to be carrying anything like that.\n\r", ch);
         } else {
            obj = get_obj_in_list_vis(ch, item, victim->carrying);
            if(number(0,2) || !obj) {
               act("$N doesn't seem to be carrying anything like that.", ch, 0, victim, TO_CHAR, 0);
               return eSUCCESS;
            } else {
               appraised = GET_OBJ_WEIGHT(obj);
               found = TRUE;
               weight = TRUE;
            }
         }
         if(number(0,1))
            appraised += 10 - learned / 10;
         appraised -= 10 - learned / 10;
      }

      if(!found)
         appraised = GET_GOLD(victim);
   }

   if(!weight) {
      if(!number(0,1))
         appraised *= 1 + (100 - learned) / 100;
      else 
         appraised *= 1 - (100 - learned) / 100;

      if(appraised < 1000) {
         appraised /= 100;
         appraised *= 100;
      } else if(appraised < 10000) {
         appraised /= 1000;
         appraised *= 1000;
      } else {
         appraised /= 10000;
         appraised *= 10000;
      }
   }

   if(appraised < 0)
      appraised = 1;

   if(!skill_success(ch, victim, SKILL_APPRAISE)) {
      send_to_char("You estimate a worth of 30000000000?!?\n\r", ch);
      WAIT_STATE(ch, PULSE_VIOLENCE);
   } else {
      if(weight)
         sprintf(buf, "After some consideration, you estimate the weight of %s to be %d.\n\r", GET_OBJ_SHORT(obj), appraised);
      else if(found) sprintf(buf, "After some consideration, you estimate the value of %s to be %d.\n\r", GET_OBJ_SHORT(obj), appraised);
      else sprintf(buf, "After some consideration, you estimate the amount of gold %s is carrying to be %d.\n\r", GET_NAME(victim), appraised);
      send_to_char(buf, ch);
      WAIT_STATE(ch, (int)(PULSE_VIOLENCE * 1.5));
   }

   return eSUCCESS;
}

int do_cripple(CHAR_DATA *ch, char *argument, int cmd)
{
   CHAR_DATA *vict;
   char name[MAX_STRING_LENGTH];
   int skill;

   one_argument(argument, name);

   if(!(skill = has_skill(ch, SKILL_CRIPPLE))) {
      send_to_char("You don't know how to cripple anybody.\n\r", ch);
      return eFAILURE;
   }

   if(!(vict = get_char_room_vis(ch, name)) && !(vict = ch->fighting)) {
      send_to_char("Cripple whom?\n\r", ch);
      return eFAILURE;
   }

   if(vict == ch) {
      send_to_char("You turn your head and grimace as you break your ankles with a sledgehammer.\n\r", ch);
      return eFAILURE;
   }

   if(IS_MOB(vict) && ISSET(vict->mobdata->actflags, ACT_HUGE) && skill < 81) {
      send_to_char("You cannot cripple someone that HUGE!\r\n", ch);
      return eFAILURE;
   }

   if(IS_MOB(vict) && ISSET(vict->mobdata->actflags, ACT_SWARM) && skill < 81) {
      send_to_char("You cannot pick just one to cripple!\r\n", ch);
      return eFAILURE;
   }

   if(IS_MOB(vict) && ISSET(vict->mobdata->actflags, ACT_TINY) && skill < 81) {
      send_to_char("You cannot target someone that tiny to cripple!\r\n", ch);
      return eFAILURE;
   }

   if(IS_AFFECTED(vict, AFF_CRIPPLE)) {
      act("$N has already been crippled!", ch, 0, vict, TO_CHAR, 0);
      return eFAILURE;
   }

   if((GET_LEVEL(ch) < IMMORTAL) || IS_NPC(ch))
      if(!can_attack(ch) || !can_be_attacked(ch, vict))
         return eFAILURE;

   WAIT_STATE(ch, PULSE_VIOLENCE * 2);
  // Make 'em fight eachother
   if (!vict->fighting)
     set_fighting(vict,ch);
   if (!ch->fighting)
    set_fighting(ch, vict);

   if(!skill_success(ch, vict, SKILL_CRIPPLE)) {
      act("You quickly lash out but fail to cripple $N.", ch, 0, vict, TO_CHAR, 0);
      act("$n quickly lashes out, narrowly missing an attempt to cripple you!", ch, 0, vict, TO_VICT, 0);
      act("$n quickly lashes out but fails to cripple $N.", ch, 0, vict, TO_ROOM, NOTVICT);
   } else {
      act("You quickly lash out and strike a crippling blow to $N!", ch, 0, vict, TO_CHAR, 0);
      act("$n lashes out quickly and cripples you with a painful blow!", ch, 0, vict, TO_VICT, 0);
      act("$n quickly lashes out and strikes a crippling blow to $N!", ch, 0, vict, TO_ROOM, NOTVICT);
      struct affected_type af;
      af.type      = SKILL_CRIPPLE;
      af.duration  = skill / 20;
      af.duration_type  = PULSE_VIOLENCE;
      af.modifier  = skill;
      af.location  = APPLY_NONE;
      af.bitvector = AFF_CRIPPLE;
      affect_to_char(vict, &af, PULSE_VIOLENCE);
   }

   return eSUCCESS;
}
