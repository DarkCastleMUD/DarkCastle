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
/* $Id: mob_act.cpp,v 1.3 2002/07/12 00:12:32 pirahna Exp $ */

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

extern CHAR_DATA *character_list;
extern struct index_data *mob_index;
extern CWorld world;
extern struct zone_data *zone_table;

extern struct str_app_type str_app[];
extern struct race_shit race_info[30];

extern char doing[500];  /* write to log if we crash */

int keywordfind(struct obj_data *obj_object);
int hands_are_free(CHAR_DATA *ch, int number);
void perform_wear(CHAR_DATA *ch, struct obj_data *obj_object,
                  int keyword);
char * get_random_hate(CHAR_DATA *ch);
void get(struct char_data *ch, struct obj_data *obj_object,
    struct obj_data *sub_object);

int fighter_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner);
int passive_magic_user(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner);
int passive_necro(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner);
int ranger_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner);
int paladin_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner);
int antipaladin_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
          struct char_data *owner);
int thief_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner);
int barbarian_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
          struct char_data *owner);
int monk_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner);
int passive_cleric(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner);

void mobile_activity(void)
{
  CHAR_DATA *ch;
  CHAR_DATA *tmp_ch, *pch, *next_dude;
  struct obj_data *obj, *best_obj;
  extern struct str_app_type str_app[];
  char buf[1000];
  int door, max;
  int keyword;
  int done;
  int tmp_race, tmp_bitv;
  int retval;

  int attempt_move(CHAR_DATA *ch, int cmd, int is_retreat = 0);
  extern int mprog_cur_result;
  
  /* Examine all mobs. */
  for(ch = character_list; ch; ch = next_dude) 
  {
    next_dude = ch->next;
    
    if(!IS_MOB(ch))
      continue;
    
    if(IS_AFFECTED(ch, AFF_PARALYSIS))
      continue;
    
    if(IS_SET(ch->combat, COMBAT_SHOCKED))
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
    if(mob_index[ch->mobdata->nr].non_combat_func) {
      retval = ((*mob_index[ch->mobdata->nr].non_combat_func) (ch, 0, 0, "", ch));
      if(!IS_SET(retval, eFAILURE))
        continue;
    }

    if(ch->fighting) // that's it for monsters busy fighting
      continue;

    // if the non_combat_proc returns eFAILURE, go ahead and try to use class non_combat stuff
    switch(GET_CLASS(ch)) 
    {
            case CLASS_WARRIOR:
              retval = fighter_non_combat(ch, NULL, 0, "", ch);
              if(!IS_SET(retval, eFAILURE))
                continue;
              break;
            case CLASS_THIEF:
              retval = thief_non_combat(ch, NULL, 0, "", ch);
              if(!IS_SET(retval, eFAILURE))
                continue;
              break;
            case CLASS_MONK:
              retval = monk_non_combat(ch, NULL, 0, "", ch);
              if(!IS_SET(retval, eFAILURE))
                continue;
              break;
            case CLASS_RANGER:
              retval = ranger_non_combat(ch, NULL, 0, "", ch);
              if(!IS_SET(retval, eFAILURE))
                continue;
              break;
            case CLASS_MAGIC_USER:
              retval = passive_magic_user(ch, NULL, 0, "", ch);
              if(!IS_SET(retval, eFAILURE))
                continue;
              break;
            case CLASS_CLERIC:
              retval = passive_cleric(ch, NULL, 0, "", ch);
              if(!IS_SET(retval, eFAILURE))
                continue;
              break;
            case CLASS_PALADIN:
              retval = paladin_non_combat(ch, NULL, 0, "", ch);
              if(!IS_SET(retval, eFAILURE))
                continue;
              break;
            case CLASS_ANTI_PAL:
              retval = antipaladin_non_combat(ch, NULL, 0, "", ch);
              if(!IS_SET(retval, eFAILURE))
                continue;
              break;
            case CLASS_BARBARIAN:
              retval = barbarian_non_combat(ch, NULL, 0, "", ch);
              if(!IS_SET(retval, eFAILURE))
                continue;
              break;
            case CLASS_NECROMANCER:
              retval = passive_necro(ch, NULL, 0, "", ch);
              if(!IS_SET(retval, eFAILURE))
                continue;
              break;
            default:
              break;
    }      
    if(SOMEONE_DIED(retval))  // paranoia check in case someone screwed up
       continue;              // and returned CH_DIED along with FAILURE

// TODO - we might want to think about doing some spec procs that are called
//     for certain races.  Ie, "snakes" all try to "bite" you, or horses kick, etc.    

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

    // activate mprog act triggers
    if ( ch->mobdata->mpactnum > 0 )  // we check to make sure ch is mob in very beginning, so safe
    {
        MPROG_ACT_LIST * tmp_act, *tmp2_act;
        for ( tmp_act = ch->mobdata->mpact; tmp_act != NULL; tmp_act = tmp_act->next )
        {
             mprog_wordlist_check( tmp_act->buf, ch, tmp_act->ch,
                       tmp_act->obj, tmp_act->vo, ACT_PROG );
             retval = mprog_cur_result;
             if(IS_SET(retval, eCH_DIED))
               break; // break so we can continue with the next mob
        }
        if(IS_SET(retval, eCH_DIED))
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
      IS_SET(ch->mobdata->actflags, ACT_SCAVENGER) &&
      number(0, 2) == 0) 
    {
      max      = 1;
      done     = 0;
      best_obj = 0;
      for(obj = world[ch->in_room].contents; obj; obj = obj->next_content) 
      {
        if(!CAN_GET_OBJ(ch, obj))
          continue;
        
        keyword = keywordfind(obj);
        
        if(keyword != -2) {
          if((hands_are_free(ch, 1)) && (CAN_WEAR(obj, WIELD))) 
          {
            if(GET_OBJ_WEIGHT(obj) < 
              str_app[STRENGTH_APPLY_INDEX(ch)].wield_w) 
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
            else
              continue;
          } /* if hands are free and can wear */            
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
          } 
        } /* if keyword != -2 */ 
      } /* for obj */
    }
  
    // TODO - I believe this is second, so that we go through and pick up armor/weapons first
    // and then we pick up the best item and work our way down.  This makes sense but really 
    // is NOT that big a deal.  If an item is on the ground long enough for a mob to pick it
    // up, it's probably going to have time to get the next item too.  We need to move this
    // into the above SCAVENGER if statement, and streamline them both to be more effecient

    // Scavenge 
    if(IS_SET(ch->mobdata->actflags, ACT_SCAVENGER)
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
          get(ch, best_obj, 0);
//        move_obj( best_obj, ch );
//        act( "$n gets $p.",  ch, best_obj, 0, TO_ROOM, 0);
      }
    }
  
    /* Wander */
    if(!IS_SET(ch->mobdata->actflags, ACT_SENTINEL)
      && GET_POS(ch) == POSITION_STANDING
      && (door = number(0,30)) <= 5
      && CAN_GO(ch, door)
      && !IS_SET(world[EXIT(ch,door)->to_room].room_flags, NO_MOB)
      && ( IS_AFFECTED(ch, AFF_FLYING) ||
           !IS_SET(world[EXIT(ch,door)->to_room].room_flags, 
                   (FALL_UP | FALL_SOUTH | FALL_NORTH | FALL_EAST | FALL_WEST | FALL_DOWN))
         )
      && ( !IS_SET(ch->mobdata->actflags, ACT_STAY_ZONE) ||
           world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone
         )
      ) 
    {
      if(ch->mobdata->last_direction == door)
        ch->mobdata->last_direction = -1;
      else if(!IS_SET(ch->mobdata->actflags, ACT_STAY_NO_TOWN) ||
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
      CHAR_DATA *next_blah;
    
      done = 0;
    
      for(tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = next_blah) 
      {
        next_blah = tmp_ch->next_in_room;
      
        if(!CAN_SEE(ch, tmp_ch))
          continue;
        if(!IS_MOB(tmp_ch) && IS_SET(tmp_ch->pcdata->toggles, PLR_NOHASSLE))
          continue;
      
        if(isname(GET_NAME(tmp_ch), ch->mobdata->hatred)) // use isname since hatred is a list
        {
          if( ( IS_AFFECTED(tmp_ch, AFF_PROTECT_EVIL) && IS_EVIL(ch) &&
                ( GET_LEVEL(ch) <= ( GET_LEVEL(tmp_ch)+2) )
              ) ||
              IS_SET(world[ch->in_room].room_flags, SAFE))  
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

      if(!ch->hunting && !IS_SET(ch->mobdata->actflags, ACT_STUPID)) 
      {
        add_memory(ch, get_random_hate(ch), 't');
        if(!IS_AFFECTED(ch, AFF_BLIND)) {
          do_track(ch, get_random_hate(ch), 9);
          continue;
        }
      }
    }  //  end FIRST hatred IF statement 
  
    /* Aggress */
    if(IS_SET(ch->mobdata->actflags, ACT_AGGRESSIVE) &&
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
      
          if(!CAN_SEE(ch, tmp_ch))
            continue;
          if(IS_NPC(tmp_ch) && !IS_AFFECTED(tmp_ch, AFF_CHARM))
            continue;
          if(IS_SET(ch->mobdata->actflags, ACT_WIMPY) && AWAKE(tmp_ch) )
            continue;
          if(!IS_MOB(tmp_ch) && IS_SET(tmp_ch->pcdata->toggles, PLR_NOHASSLE) )
            continue;
      
          if((IS_AFFECTED(tmp_ch, AFF_PROTECT_EVIL)) ||
            (GET_CLASS(tmp_ch) == CLASS_ANTI_PAL)) 
          {
            if(IS_EVIL(ch)) {
              if(GET_LEVEL(ch) <= (GET_LEVEL(tmp_ch)))
                continue;
            }
          }
      
          if(number(0, 1)) {
            done = 1;
            retval = mprog_attack_trigger( ch, tmp_ch );
            if(SOMEONE_DIED(retval))
              break;
            attack(ch, tmp_ch, 0);
            break;
          }
          else 
            targets = 1;
        }
      }
    
      if(done)
        continue;
    } // if aggressive
  
    if(ch->mobdata->fears)
      if(get_char_room_vis(ch, ch->mobdata->fears)) 
      {
        if(ch->mobdata->hatred != NULL)
          remove_memory(ch, 'h');
        act("$n screams 'Oh SHIT!'", ch, 0, 0, TO_ROOM, 0);
        do_flee(ch, "", 0); 
        continue;
      }
    
    if(IS_SET(ch->mobdata->actflags,
      ACT_RACIST|ACT_FRIENDLY|ACT_AGGR_EVIL|ACT_AGGR_NEUT|ACT_AGGR_GOOD))
      for(tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = pch) 
      {
        pch = tmp_ch->next_in_room;

        tmp_bitv = GET_BITV(tmp_ch);
      
        if(IS_SET(ch->mobdata->actflags, ACT_FRIENDLY) &&
           tmp_ch->fighting &&
           IS_SET(race_info[(int)GET_RACE(ch)].friendly, tmp_bitv) &&
           !IS_NPC(tmp_ch->fighting))
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
      
        if(!IS_NPC(tmp_ch) && !tmp_ch->fighting && CAN_SEE(ch, tmp_ch) &&
           !IS_SET(world[ch->in_room].room_flags, SAFE) &&
           !IS_SET(tmp_ch->pcdata->toggles, PLR_NOHASSLE) // this is safe, cause we checked !IS_NPC first
          )
        { 
          if(IS_SET(ch->mobdata->actflags, ACT_AGGR_EVIL) &&
            GET_ALIGNMENT(tmp_ch) <= -350)
          {
            act("$n screams 'May truth and justice prevail!'", ch, 0, 0, TO_ROOM, 0);

            retval = mprog_attack_trigger( ch, tmp_ch );
            if(SOMEONE_DIED(retval))
              break;
            attack(ch, tmp_ch, 0);
            break;
          }
        
          if(IS_SET(ch->mobdata->actflags, ACT_AGGR_GOOD) &&
            GET_ALIGNMENT(tmp_ch) >= 350)
          {
            act("$n screams 'The forces of evil shall crush your goodness!'", ch, 0, 0, TO_ROOM, 0);

            retval = mprog_attack_trigger( ch, tmp_ch );
            if(SOMEONE_DIED(retval))
              break;
            attack(ch, tmp_ch, 0);
            break;
          }
        
          if(IS_SET(ch->mobdata->actflags, ACT_AGGR_NEUT) &&
            GET_ALIGNMENT(tmp_ch) > -350 &&
            GET_ALIGNMENT(tmp_ch) < 350)
          {
            act("$n screams 'Pick a side, neutral dog!'", ch, 0, 0, TO_ROOM, 0);

            retval = mprog_attack_trigger( ch, tmp_ch );
            if(SOMEONE_DIED(retval))
              break;
            attack(ch, tmp_ch, 0);
            break;
          }
      
          if(IS_SET(ch->mobdata->actflags, ACT_RACIST) &&
            IS_SET(race_info[(int)GET_RACE(ch)].hate_fear, tmp_bitv))
          {
            tmp_race = GET_RACE(tmp_ch);
            if(GET_LEVEL(ch) >= GET_LEVEL(tmp_ch)) {
              sprintf(buf, "$n screams 'Oooo, I HATE %s!'", race_info[tmp_race].plural_name);
              act(buf, ch, 0, 0, TO_ROOM, 0);
              retval = mprog_attack_trigger( ch, tmp_ch );
              if(SOMEONE_DIED(retval))
                break;
              attack(ch, tmp_ch, 0);
            } else {
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







