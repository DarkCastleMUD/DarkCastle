/***************************************************************************
*  file: mob_act.c , Mobile action module.                Part of DIKUMUD *
*  Usage: Procedures generating 'intelligent' behavior in the mobiles.    *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*                                                                         *
*  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
*  Performance optimization and bug fixes by MERC Industries.             *
*  You can use our stuff in any way you like whatsoever so long as ths   *
*  copyright notice remains intact.  If you like it please drop a line    *
*  to mec@garnet.berkeley.edu.                                            *
*                                                                         *
*  This is free software and you are benefitting.  We hope that you       *
*  share your changes too.  What goes around, comes around.               *
***************************************************************************/
/**************************************************************************/
/* Revision History                                                       */
/* 12/05/2003   Onager   Created is_protected() to break out PFE/PFG code */
/* 12/05/2003   Onager   Created scavenge() to simplify mobile_activity() */
/* 12/06/2003   Onager   Modified mobile_activity() to prevent charmie    */
/*                       scavenging                                       */
/**************************************************************************/
/* $Id: mob_act.cpp,v 1.52 2014/07/04 22:00:04 jhhudso Exp $ */

extern "C"
{
#include <stdio.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <character.h>
#include <room.h>
#include <mobile.h>
#include <utility.h>
#include <fight.h>
#include <db.h> // index_data
#include <player.h>
#include <levels.h>
#include <act.h>
#include <handler.h>
#include <interp.h>
#include <returnvals.h>
#include <string.h>
#include <spells.h>
#include <race.h> // Race defines used in align-aggro messages.
#include <comm.h>
#include <connect.h>
#include <inventory.h>

extern CHAR_DATA *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern CWorld world;
extern struct zone_data *zone_table;

extern struct race_shit race_info[33];

int keywordfind(struct obj_data *obj_object);
int hands_are_free(CHAR_DATA *ch, int number);
void perform_wear(CHAR_DATA *ch, struct obj_data *obj_object,
                  int keyword);
bool is_protected(struct char_data *vict, struct char_data *ch);
void scavenge(struct char_data *ch);
bool is_r_denied(CHAR_DATA *ch, int room)
{
  struct deny_data *d;
  if (!IS_NPC(ch)) return FALSE;
  for (d = world[room].denied;d;d=d->next)
    if (mob_index[ch->mobdata->nr].virt == d->vnum)
	return TRUE;
  return FALSE;
}
void mobile_activity(void)
{
  CHAR_DATA *ch;
  CHAR_DATA *tmp_ch, *pch, *next_dude;
  struct obj_data *obj, *best_obj;
  char buf[1000];
  int door, max;
  int done;
  int tmp_race, tmp_bitv;
  int retval;

  int attempt_move(CHAR_DATA *ch, int cmd, int is_retreat = 0);
  extern int mprog_cur_result;
  
  /* Examine all mobs. */
  for(ch = character_list; ch; ch = next_dude) 
  {
    if (!ch || ch == (CHAR_DATA*)0x95959595) break;
    next_dude = ch->next;
    
    if(!IS_MOB(ch))
      continue;
    
    if (MOB_WAIT_STATE(ch) > 0) MOB_WAIT_STATE(ch) -= PULSE_MOBILE;
    if(IS_AFFECTED(ch, AFF_PARALYSIS))
      continue;
    
    if(IS_SET(ch->combat, COMBAT_SHOCKED) || IS_SET(ch->combat, COMBAT_SHOCKED2))
      continue;

    if((IS_SET(ch->combat, COMBAT_STUNNED)) || 
      (IS_SET(ch->combat, COMBAT_STUNNED2)))
      continue;
    
    if((IS_SET(ch->combat, COMBAT_BASH1)) || 
      (IS_SET(ch->combat, COMBAT_BASH2)))
      continue;

    retval = eSUCCESS;
    
    // Examine call for special procedure 
    // These are done BEFORE checks for awake and stuff, so the proc needs
    // to check that stuff on it's own to make sure it doesn't do something 
    // silly.  This is to allow for mob procs to "wake" the mob and stuff
    // It also means the mob has to check to make sure he's not already in
    // combat for stuff he shouldn't be able to do while fighting:)
    // And paralyze...
    if (ch->in_room == -1) {
      log("ch->in_room set to -1 but on character_list. Averting crash.", -1, LOG_BUG);
      produce_coredump();
      continue;
    }

    if(mob_index[ch->mobdata->nr].non_combat_func) {
      retval = ((*mob_index[ch->mobdata->nr].non_combat_func) (ch, 0, 0, "", ch));
      if(!IS_SET(retval, eFAILURE) || SOMEONE_DIED(retval))
        continue;
    }

    if(ch->fighting) // that's it for monsters busy fighting
      continue;


    if(!AWAKE(ch))
      continue;
    if(IS_AFFECTED(ch, AFF_PARALYSIS))
      continue;
    
    done = 0;

// TODO - Try to make the 'average' mob IQ higher
    
    // Only activate mprog random triggers if someone is in the zone
    if(zone_table[world[ch->in_room].zone].players)
      retval = mprog_random_trigger( ch );
    
    if(IS_SET(retval, eCH_DIED))
      continue;
   extern bool selfpurge;
	selfpurge = FALSE;
     retval = mprog_arandom_trigger (ch);
     if (IS_SET(retval, eCH_DIED) || selfpurge)
       continue;
    // activate mprog act triggers
    if ( ch->mobdata->mpactnum > 0 )  // we check to make sure ch is mob in very beginning, so safe
    {
        MPROG_ACT_LIST * tmp_act, *tmp2_act;
        for ( tmp_act = ch->mobdata->mpact; tmp_act != NULL; tmp_act = tmp_act->next )
        {
             mprog_wordlist_check( tmp_act->buf, ch, tmp_act->ch,
                       tmp_act->obj, tmp_act->vo, ACT_PROG, FALSE );
             retval = mprog_cur_result;
             if(IS_SET(retval, eCH_DIED))
               break; // break so we can continue with the next mob
        }
        if(IS_SET(retval, eCH_DIED) || selfpurge)
          continue; // move on to next mob, this one is dead

        for ( tmp_act = ch->mobdata->mpact; tmp_act != NULL; tmp_act = tmp2_act )
        {
             tmp2_act = tmp_act->next;
             dc_free( tmp_act->buf );
             dc_free( tmp_act );
        }
        ch->mobdata->mpactnum = 0;
        ch->mobdata->mpact    = NULL;
    }

// TODO - this really should be cleaned up and put into functions look at it and you'll
//    see what I mean.

    if (world[ch->in_room].contents &&
      ISSET(ch->mobdata->actflags, ACT_SCAVENGER) &&
      !IS_AFFECTED(ch, AFF_CHARM) &&
      number(0, 2) == 0) 
    {
       scavenge(ch);
    }
  
    // TODO - I believe this is second, so that we go through and pick up armor/weapons first
    // and then we pick up the best item and work our way down.  This makes sense but really 
    // is NOT that big a deal.  If an item is on the ground long enough for a mob to pick it
    // up, it's probably going to have time to get the next item too.  We need to move this
    // into the above SCAVENGER if statement, and streamline them both to be more effecient

    // Scavenge 
    if(ISSET(ch->mobdata->actflags, ACT_SCAVENGER)
      && !IS_AFFECTED(ch, AFF_CHARM)
      && world[ch->in_room].contents && number(0,4) == 0) 
    {
      max         = 1;
      best_obj    = 0;
      for(obj = world[ch->in_room].contents; obj; obj = obj->next_content) 
      {
        if(CAN_GET_OBJ(ch, obj) && obj->obj_flags.cost > max) 
        {
          best_obj    = obj;
          max         = obj->obj_flags.cost;
        }
      }
    
      if(best_obj) 
      {
          // This should get rid of all the "gold coins" in mobs inventories.
          // -Pirahna 12/11/00
          get(ch, best_obj, 0,0, 9);
//        move_obj( best_obj, ch );
//        act( "$n gets $p.",  ch, best_obj, 0, TO_ROOM, 0);
      }
    }
  
    /* Wander */
    if(!ISSET(ch->mobdata->actflags, ACT_SENTINEL)
      && GET_POS(ch) == POSITION_STANDING
      && (door = number(0,30)) <= 5
      && CAN_GO(ch, door)
      && !IS_SET(world[EXIT(ch,door)->to_room].room_flags, NO_MOB)
      && !IS_SET(world[EXIT(ch,door)->to_room].room_flags, CLAN_ROOM)
      && ( IS_AFFECTED(ch, AFF_FLYING) ||
           !IS_SET(world[EXIT(ch,door)->to_room].room_flags, 
                   (FALL_UP | FALL_SOUTH | FALL_NORTH | FALL_EAST | FALL_WEST | FALL_DOWN))
         )
      && ( !ISSET(ch->mobdata->actflags, ACT_STAY_ZONE) ||
           world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone
         )
      ) 
    {
      if (is_r_denied(ch, EXIT(ch,door)->to_room))
	; // DENIED
      else if(ch->mobdata->last_direction == door)
        ch->mobdata->last_direction = -1;
      else if(!ISSET(ch->mobdata->actflags, ACT_STAY_NO_TOWN) ||
              !IS_SET(zone_table[world[EXIT(ch, door)->to_room].zone].zone_flags, ZONE_IS_TOWN))
      {
        ch->mobdata->last_direction = door;
        retval = attempt_move( ch, ++door );
        if(IS_SET(retval, eCH_DIED))
          continue;
      }
    }
  
    // check hatred 
    if((ch->mobdata->hatred != NULL))    //  && (!ch->fighting)) (we check fighting earlier)
    {
      send_to_char("You're hating.\r\n",ch);
      CHAR_DATA *next_blah;
//      CHAR_DATA *temp = get_char(get_random_hate(ch));    
      done = 0;
    
      for(tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = next_blah) 
      {
        next_blah = tmp_ch->next_in_room;
      
        if(!CAN_SEE(ch, tmp_ch))
          continue;
        if(!IS_MOB(tmp_ch) && IS_SET(tmp_ch->pcdata->toggles, PLR_NOHASSLE))
          continue;
        act("Checking $N", ch, 0, tmp_ch, TO_CHAR, 0);
        if(isname(GET_NAME(tmp_ch), ch->mobdata->hatred)) // use isname since hatred is a list
        {
          if(IS_SET(world[ch->in_room].room_flags, SAFE))  
          {
            act("You growl at $N.", ch,0,tmp_ch,TO_CHAR, 0);
            act("$n growls at YOU!.",ch,0,tmp_ch,TO_VICT, 0);
            act("$n growls at $N.", ch,0,tmp_ch,TO_ROOM, NOTVICT);
            continue;
          } 
          else if(!IS_NPC(tmp_ch))  
          {
            act("$n screams, 'I am going to KILL YOU!'", ch, 0, 0, TO_ROOM, 0);
            attack(ch, tmp_ch, 0);
            done = 1;
            break;
          }
        }
      } // for 
    
      if(done)
        continue;

      if(!ISSET(ch->mobdata->actflags, ACT_STUPID))
      {
	if (GET_POS(ch) > POSITION_SITTING) {
	  if(!IS_AFFECTED(ch, AFF_BLIND) && ch->hunting) {
	    retval = do_track(ch, ch->hunting, 9);
	    if(SOMEONE_DIED(retval))
	      continue;
	  }
	} else if (GET_POS(ch) < POSITION_FIGHTING) {
	  do_stand(ch, "", CMD_DEFAULT);
	  continue;
	}
      }
    }  //  end FIRST hatred IF statement 
  
    /* Aggress */
    if (!ch->fighting) // don't aggro more than one person
    if(ISSET(ch->mobdata->actflags, ACT_AGGRESSIVE) &&
      !IS_SET(world[ch->in_room].room_flags, SAFE)) 
    {
      CHAR_DATA * next_aggro; 
      int targets = 1;
      done = 0;
 
      // While not very effective what this does, is go through the
      // list of people in room.  If it finds one, it sets targets to true
      // and has a 50% chance of going for that person.  If it does, we're
      // done and we leave.  If we don't, we keep going through the list
      // and get a 50% chance at each person.  If we hit the end of the list,
      // we know there _is_ still a person in the room we want to aggro, so
      // loop back through again. - pir 5/3/00
      while(!done && targets)
      {
        targets = 0;
        for(tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = next_aggro) 
        { 
          if(!tmp_ch || !ch) {
            log("Null ch or tmp_ch in mobile_action()", IMMORTAL, LOG_BUG);
            break;
          } 
          next_aggro = tmp_ch->next_in_room; 

          if(ch == tmp_ch)
            continue;      
          if(!CAN_SEE(ch, tmp_ch))
            continue;
          if(IS_NPC(tmp_ch) && !IS_AFFECTED(tmp_ch, AFF_CHARM) && !tmp_ch->desc)
            continue;
          if(ISSET(ch->mobdata->actflags, ACT_WIMPY) && AWAKE(tmp_ch) )
            continue;
          if ((!IS_MOB(tmp_ch) && IS_SET(tmp_ch->pcdata->toggles, PLR_NOHASSLE) )
		|| (tmp_ch->desc && tmp_ch->desc->original && 
		IS_SET(tmp_ch->desc->original->pcdata->toggles, PLR_NOHASSLE)))
            continue;
      
          /* check for PFG/PFE, (anti)pal perma-protections, etc. */
//          if (is_protected(tmp_ch, ch))
  //          continue;

          if(number(0, 1)) {
            done = 1;
            retval = mprog_attack_trigger( ch, tmp_ch );
            if(SOMEONE_DIED(retval))
              break;
            attack(ch, tmp_ch, TYPE_UNDEFINED);
            break;
          }
          else 
            targets = 1;
        }
      }
    
      if(done)
        continue;
    } // if aggressive
  
    if (!ch->fighting)
    if(ch->mobdata->fears)
      if(get_char_room_vis(ch, ch->mobdata->fears)) 
      {
        if(ch->mobdata->hatred != NULL)
          remove_memory(ch, 'h');
        act("$n screams 'Oh SHIT!'", ch, 0, 0, TO_ROOM, 0);
        do_flee(ch, "", 0); 
        continue;
      }
    
    if (!ch->fighting)
    if(ISSET(ch->mobdata->actflags, ACT_RACIST) || 
     ISSET(ch->mobdata->actflags, ACT_FRIENDLY) ||
     ISSET(ch->mobdata->actflags, ACT_AGGR_EVIL) ||
     ISSET(ch->mobdata->actflags, ACT_AGGR_NEUT) ||
     ISSET(ch->mobdata->actflags, ACT_AGGR_GOOD))
      for(tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = pch) 
      {
        pch = tmp_ch->next_in_room;

        if(ch == tmp_ch)
           continue;
      

        tmp_bitv = GET_BITV(tmp_ch);

        if(ISSET(ch->mobdata->actflags, ACT_FRIENDLY) &&
	   (!ch->mobdata->hatred || !isname(GET_NAME(tmp_ch), ch->mobdata->hatred)) &&
           tmp_ch->fighting &&
	   CAN_SEE(ch, tmp_ch) &&
           (IS_SET(race_info[(int)GET_RACE(ch)].friendly, tmp_bitv) ||
	     (int)GET_RACE(ch) == (int)GET_RACE(tmp_ch)) &&

           !(IS_NPC(tmp_ch->fighting) && !IS_AFFECTED(tmp_ch->fighting, AFF_CHARM))
		&& !IS_SET(race_info[(int)GET_RACE(ch)].friendly, GET_BITV(tmp_ch->fighting)) &&
	!affected_by_spell(tmp_ch, FUCK_PTHIEF) && !affected_by_spell(tmp_ch, FUCK_GTHIEF)
)
        {
          tmp_race = GET_RACE(tmp_ch);
          if(GET_RACE(ch) == tmp_race)
            sprintf(buf, "$n screams 'Take heart, fellow %s!'", race_info[tmp_race].singular_name);
          else
            sprintf(buf, "$n screams 'HEY! Don't be picking on %s!'", race_info[tmp_race].plural_name);
          act(buf,  ch, 0, 0, TO_ROOM, 0);

          retval = mprog_attack_trigger( ch, tmp_ch );
          if(SOMEONE_DIED(retval))
            break;
          attack(ch, tmp_ch->fighting, 0);
          break;
        }
      
        /* check for PFE/PFG, (anti)pal perma-protections, etc. */
// AFTER Friendly check, 'cause I don't wanna be protected against friendly...
        if (is_protected(tmp_ch, ch))
	continue;

//           continue;

        if((!IS_NPC(tmp_ch) && !tmp_ch->fighting && CAN_SEE(ch, tmp_ch) &&
           !IS_SET(world[ch->in_room].room_flags, SAFE) &&
           !IS_SET(tmp_ch->pcdata->toggles, PLR_NOHASSLE) 
           ) ||
	   (IS_NPC(tmp_ch) && tmp_ch->desc && tmp_ch->desc->original && CAN_SEE(ch, tmp_ch)
		&& !IS_SET(tmp_ch->desc->original->pcdata->toggles, PLR_NOHASSLE)  // this is safe, cause we checked !IS_NPC first
           )
          )
        { 
	int i=0;
          switch(ch->race)
           { // Messages for attackys
                case RACE_HUMAN:
		case RACE_ELVEN:
		case RACE_DWARVEN:
		case RACE_HOBBIT:
		case RACE_PIXIE:
		case RACE_GIANT:
		case RACE_GNOME:
		case RACE_ORC:
		case RACE_TROLL:
		case RACE_GOBLIN:
		case RACE_DRAGON:
		case RACE_ENFAN:
		case RACE_DEMON:
		case RACE_YRNALI:
		  i = 1;
		  break;
		default:
		  i = 0;
		  break;
           }

          if(ISSET(ch->mobdata->actflags, ACT_AGGR_EVIL) &&
            GET_ALIGNMENT(tmp_ch) <= -350)
          {
	   if (i==1) {
	    if (GET_ALIGNMENT(ch) <= -350)
           act("$n screams 'Get outta here, freak!'", ch, 0, 0, TO_ROOM, 0);
		else
            act("$n screams 'May truth and justice prevail!'", ch, 0, 0, TO_ROOM, 0);
	   }
	   else {
	    act("$n senses your evil intentions and attacks!", ch, 0,tmp_ch, TO_VICT,0);
	    act("$n senses $N's evil intentions and attacks!", ch, 0,tmp_ch,TO_ROOM,NOTVICT);
 	   }
            retval = mprog_attack_trigger( ch, tmp_ch );
            if(SOMEONE_DIED(retval))
              break;
            attack(ch, tmp_ch, 0);
            break;
          }
        
          if(ISSET(ch->mobdata->actflags, ACT_AGGR_GOOD) &&
            GET_ALIGNMENT(tmp_ch) >= 350)
          {
	   if (i==1) {
	    if (GET_ALIGNMENT(ch) >= 350)
	      act("$n screams 'I'm afraid I cannot let you trespass onto these grounds!'",ch,0,0,TO_ROOM,0);
	    else
              act("$n screams 'The forces of evil shall crush your goodness!'", ch, 0, 0, TO_ROOM, 0);
	   }
	   else {
	    act("$n is offended by your good nature and attacks!",ch,0, tmp_ch, TO_VICT,0);
	    act("$n is offended by $N's good nature and attacks!", ch, 0,tmp_ch,  TO_ROOM,NOTVICT);
	   }
            retval = mprog_attack_trigger( ch, tmp_ch );
            if(SOMEONE_DIED(retval))
              break;
            attack(ch, tmp_ch, 0);
            break;
          }
        
          if(ISSET(ch->mobdata->actflags, ACT_AGGR_NEUT) &&
            GET_ALIGNMENT(tmp_ch) > -350 &&
            GET_ALIGNMENT(tmp_ch) < 350)
          {
	   if (i==0)
            act("$n screams 'Pick a side, neutral dog!'", ch, 0, 0, TO_ROOM, 0);
	   else {
	    act("$n detects $N's neutrality and attacks!", ch, 0,tmp_ch, TO_ROOM,NOTVICT);
	    act("$n detects your neutrality and attacks!", ch, 0,tmp_ch, TO_VICT,0);
  	  }

            retval = mprog_attack_trigger( ch, tmp_ch );
            if(SOMEONE_DIED(retval))
              break;
            attack(ch, tmp_ch, 0);
            break;
          }
      
          if(ISSET(ch->mobdata->actflags, ACT_RACIST) &&
            IS_SET(race_info[(int)GET_RACE(ch)].hate_fear, tmp_bitv))
          {
            tmp_race = GET_RACE(tmp_ch);
            bool wimpy = ISSET(ch->mobdata->actflags, ACT_WIMPY);


            //if mob isn't wimpy, always attack
            //if mob is wimpy, but is equal or greater, attack
            //if mob is wimpy, and lower level.. flee
            if(!wimpy || (wimpy && GET_LEVEL(ch) >= GET_LEVEL(tmp_ch)))
            {
              sprintf(buf, "$n screams 'Oooo, I HATE %s!'", race_info[tmp_race].plural_name);
              act(buf, ch, 0, 0, TO_ROOM, 0);
              retval = mprog_attack_trigger( ch, tmp_ch );
              if(SOMEONE_DIED(retval))
                break;
              attack(ch, tmp_ch, 0);
            } 
            else 
            {
              sprintf(buf, "$n screams 'Eeeeek, I HATE %s!'", race_info[tmp_race].plural_name);
              act(buf,  ch, 0, 0, TO_ROOM, 0);
              do_flee(ch, "", 9);
            }
            break;
          }
        } // If !IS_NPC(tmp_ch) 
      } // for() for the RACIST, AGG_XXX and FRIENDLY flags

      // Note, if you add anything to this point, you need to put if(done) continue
      // check after the RACIST stuff.  They aren't checking if ch died or not since
      // it just ends here.
    
  } // for() all mobs
}

// Just a function to have mobs say random stuff when they are "suprised"
// about finding a player doing something and decide to attack them.
// For example, when a mob finds a player casting "spell" on them.
void mob_suprised_sayings(char_data * ch, char_data * aggressor)
{
   switch(number(0, 6))
   {
      case 0:  do_say(ch, "What do you think you are doing?!", 9);
               break;
      case 1:  do_say(ch, "Mess with the best?  Die like the rest!", 9);
               break;
      case 2:  do_emote(ch, " looks around for a moment, confused.", 9);
               do_say(ch, "YOU!!", 9);
               break;
      case 3:  do_say(ch, "Foolish.", 9);
               break;
      case 4:  do_say(ch, "I'm going to treat you like a baby treats a diaper.", 9);
               break;
      case 5:  do_say(ch, "Here comes the pain baby!", 9);
               break;
      case 6:  do_emote(ch, " wiggles its bottom.", 9);
               break;
   }
}

/* check to see if the player is protected from the mob */
// PROTECTION_FROM_EVIL and GOOD modifier contains the level
// protected from.  PAL's ANTI's take spell/level whichever higher
bool is_protected(struct char_data *vict, struct char_data *ch)
{
   struct affected_type *aff = affected_by_spell(vict, SPELL_PROTECT_FROM_EVIL);
   int level_protected = aff?aff->modifier:0;
   if(GET_CLASS(vict) == CLASS_ANTI_PAL && IS_EVIL(vict) && GET_LEVEL(vict) > level_protected)
      level_protected = GET_LEVEL(vict);

   if(IS_EVIL(ch) && GET_LEVEL(ch) < level_protected)
      return(true);

   if(IS_EVIL(ch) && GET_LEVEL(ch) <= GET_LEVEL(vict) && IS_AFFECTED(vict, AFF_PROTECT_EVIL)) return TRUE;
      
   aff = affected_by_spell(vict, SPELL_PROTECT_FROM_GOOD);
   level_protected = aff?aff->modifier:0;
   if(GET_CLASS(vict) == CLASS_PALADIN && IS_GOOD(vict) && GET_LEVEL(vict) > level_protected)
      level_protected = GET_LEVEL(vict);

   if(IS_GOOD(ch) && GET_LEVEL(ch) < level_protected)
      return(true);

   if(IS_GOOD(ch) && GET_LEVEL(ch) <= GET_LEVEL(vict) && IS_AFFECTED(vict, AFF_PROTECT_GOOD)) return TRUE;

/* old version
   if(IS_EVIL(ch) && GET_LEVEL(ch) <= (GET_LEVEL(vict))) {
      if((IS_AFFECTED(vict, AFF_PROTECT_EVIL)) ||
        (GET_CLASS(vict) == CLASS_ANTI_PAL && IS_EVIL(vict)))
           return(true);
   }

   if(IS_GOOD(ch) && GET_LEVEL(ch) <= (GET_LEVEL(vict))) {
      if((affected_by_spell(vict, SPELL_PROTECT_FROM_GOOD)) ||
        (GET_CLASS(vict) == CLASS_PALADIN && IS_GOOD(vict)))
           return(true);
   }
*/

   return(false);
}

void scavenge(struct char_data *ch)
{
  struct obj_data *obj;
  int done;
  int keyword;

  done     = 0;
  if (IS_AFFECTED(ch, AFF_CHARM)) return;
  for(obj = world[ch->in_room].contents; obj; obj = obj->next_content) 
  {
    if(!CAN_GET_OBJ(ch, obj))
      continue;

    if(obj_index[obj->item_number].virt == CHAMPION_ITEM) continue;
    
    keyword = keywordfind(obj);
    
    if(keyword != -2) {
      if (hands_are_free(ch, 1)) {
       if (CAN_WEAR(obj, ITEM_WIELD))
       {
        if(GET_OBJ_WEIGHT(obj) < GET_STR(ch)) 
        {
          if(!ch->equipment[WIELD]) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.", ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch, obj, WIELD);
            break;
          }
          /* damage check */
          if((ch->equipment[WIELD]) && (!ch->equipment[SECOND_WIELD])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.", ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, SECOND_WIELD);
            break;
          }
        } // GET_OBJ_WEIGHT()
       continue;
      } /* if can wear */            
      else 
      {
        if(((keyword == 13) || (keyword == 14)) && !hands_are_free(ch, 1)) 
          continue;
        
        switch (keyword) 
        {
          
        case 0: 
          if ((CAN_WEAR(obj, ITEM_WEAR_FINGER)) && 
            ( (!ch->equipment[WEAR_FINGER_L]) || (!ch->equipment[WEAR_FINGER_R])) ) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            if (!ch->equipment[WEAR_FINGER_L])
              equip_char(ch,obj, WEAR_FINGER_L);
            else
              equip_char(ch,obj, WEAR_FINGER_R);
            done = 1;
          } 
          break;
          
        case 1:
          if ((CAN_WEAR(obj, ITEM_WEAR_NECK)) && 
            ( (!ch->equipment[WEAR_NECK_1]) || (!ch->equipment[WEAR_NECK_2])) ) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            if (!ch->equipment[WEAR_NECK_1])
              equip_char(ch,obj, WEAR_NECK_1);
            else
              equip_char(ch,obj, WEAR_NECK_2);
            done = 1;
          }  
          break;
          
        case 2:
          if ((CAN_WEAR(obj, ITEM_WEAR_BODY)) && (!ch->equipment[WEAR_BODY])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, WEAR_BODY);
            done =1;
          } 
          break;
          
        case 3:
          if ((CAN_WEAR(obj, ITEM_WEAR_HEAD)) && (!ch->equipment[WEAR_HEAD])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, WEAR_HEAD);
            done = 1;
          } 
          break;
          
        case 4:
          if ((CAN_WEAR(obj, ITEM_WEAR_LEGS)) && (!ch->equipment[WEAR_LEGS])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.", ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, WEAR_LEGS);
            done = 1;
          } 
          break;
          
        case 5:
          if ((CAN_WEAR(obj, ITEM_WEAR_FEET)) && (!ch->equipment[WEAR_FEET])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.", ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, WEAR_FEET);
            done = 1;
          }  
          break;
                          
        case 6:
          if ((CAN_WEAR(obj, ITEM_WEAR_HANDS)) && (!ch->equipment[WEAR_HANDS])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, WEAR_HANDS);
            done = 1;
          } 
          break;
          
        case 7:
          if ((CAN_WEAR(obj, ITEM_WEAR_ARMS)) && (!ch->equipment[WEAR_ARMS])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, WEAR_ARMS);
            done = 1;
          } 
          break;
          
        case 8:
          if ((CAN_WEAR(obj, ITEM_WEAR_ABOUT)) && (!ch->equipment[WEAR_ABOUT])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, WEAR_ABOUT);
            done = 1;
          } 
          break;
          
        case 9:
          if ((CAN_WEAR(obj, ITEM_WEAR_WAISTE)) && (!ch->equipment[WEAR_WAISTE])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, WEAR_WAISTE);
            done = 1;
          } 
          break;

        case 10: 
          if ((CAN_WEAR(obj, ITEM_WEAR_WRIST)) && 
            ( (!ch->equipment[WEAR_WRIST_L]) || (!ch->equipment[WEAR_WRIST_R])) ) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            if (!ch->equipment[WEAR_WRIST_L])
              equip_char(ch,obj, WEAR_WRIST_L);
            else
              equip_char(ch,obj, WEAR_WRIST_R);
            done = 1;
          }
          break;
          
        case 11:
          if ((CAN_WEAR(obj, ITEM_WEAR_FACE)) && (!ch->equipment[WEAR_FACE])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, WEAR_FACE);
            done =1;
          } 
          break;
          
        case 12:  
            done = 1;
          break;
          
        case 13:
          if ((CAN_WEAR(obj, ITEM_WEAR_SHIELD)) && (!ch->equipment[WEAR_SHIELD])) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            equip_char(ch,obj, WEAR_SHIELD);
            done = 1;
          } 
          break;
          
        case 14:
          if ((CAN_WEAR(obj, ITEM_HOLD)) && (!ch->equipment[HOLD])) 
          {  
            if ( (obj->obj_flags.type_flag == ITEM_LIGHT) &&
              (!ch->equipment[WEAR_LIGHT]) ) 
            {
              move_obj( obj, ch );
              act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
              perform_wear(ch, obj, 16);
              obj_from_char(obj);
              equip_char(ch,obj, WEAR_LIGHT);
              done = 1;
            } 
            else if (obj->obj_flags.type_flag != ITEM_LIGHT) 
            {
              move_obj( obj, ch );
              act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
              perform_wear(ch, obj, keyword);
              obj_from_char(obj);
              equip_char(ch,obj, HOLD);
              done = 1;
            }
          } 
          break;
          
        case 15: 
          if ((CAN_WEAR(obj, ITEM_WEAR_EAR)) && 
            ( (!ch->equipment[WEAR_EAR_L]) || (!ch->equipment[WEAR_EAR_R])) ) 
          {
            move_obj( obj, ch );
            act( "$n gets $p.",  ch, obj, 0, TO_ROOM , 0);
            perform_wear(ch, obj, keyword);
            obj_from_char(obj);
            if (!ch->equipment[WEAR_EAR_L])
              equip_char(ch,obj, WEAR_EAR_L);
            else
              equip_char(ch,obj, WEAR_EAR_R);
            done = 1;
          } 
          break;
          
        default:
            log("Bad switch in mob_act.C", 0, LOG_BUG);
            break;

        } /* end switch */
 
        if(done == 1)
          break;
        else
            continue;
      } // else can wear
     } // if hands are free 
    } /* if keyword != -2 */ 
  } /* for obj */
}

void clear_hunt(void *arg1, void *arg2, void *arg3)
{
  clear_hunt((char*)arg1, (CHAR_DATA*)arg2,NULL);
}

void clear_hunt(char *arg1, CHAR_DATA *arg2, void *arg3)
{
  CHAR_DATA *curr;
  for (curr = character_list;curr;curr = curr->next)
  {
    if (curr == arg2)
    {
	dc_free(arg1);
	arg2->hunting = NULL;
    }
  }
}
