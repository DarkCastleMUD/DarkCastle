/******************************************************************************
 * $Id: cl_ranger.cpp,v 1.73 2006/07/04 08:42:16 shane Exp $ | cl_ranger.C  *
 * Description: Ranger skills/spells                                          *
 *                                                                            *
 * Revision History                                                           *
 * 10/28/2003  Onager   Modified do_tame() to remove hate flag for tamer      *
 * 12/08/2003  Onager   Added eq check to do_tame() to remove !charmie eq     *
 *                      from charmies                                         *
 ******************************************************************************/

extern "C"  {
  #include <string.h>
}
//#include <iostream.h>
#include <stdio.h>
#include <character.h>
#include <affect.h>
#include <mobile.h>
#include <utility.h>
#include <spells.h>
#include <isr.h>
#include <handler.h>
#include <room.h>
#include <terminal.h>
#include <player.h>
#include <levels.h>
#include <connect.h>
#include <fight.h>
#include <interp.h>
#include <db.h>
#include <act.h>
#include <fileinfo.h> // SAVE_DIR
#include <returnvals.h>

extern CWorld world;
extern struct zone_data *zone_table;
extern struct obj_data *object_list; 
extern struct race_shit race_info[];
extern int rev_dir[];
void save_corpses(void);

int saves_spell(CHAR_DATA *ch, CHAR_DATA *vict, int spell_base, int16 save_type);
void check_eq(CHAR_DATA *ch);
extern struct index_data *mob_index;
int get_difficulty(int);

int charm_space(int level)
{
  if (level >= 50) return 10;
  if (level >= 41) return 7;
  if (level >= 31) return 5;
  if (level >= 21) return 3;
  if (level >= 11) return 2;
  return 1;
}

int charm_levels(CHAR_DATA *ch)
{
  int i = GET_LEVEL(ch)/5;
  int z = 3;
  struct follow_type *f;
  for (f = ch->followers;f;f = f->next)
     if (IS_AFFECTED(f->follower, AFF_CHARM))
     {
	z--;
        i -= charm_space(GET_LEVEL(f->follower));
	}
  if (z <= 0) return -1;
  return i;
}

int do_tame(CHAR_DATA *ch, char *arg, int cmd)
{
  struct affected_type af;
  CHAR_DATA *victim;
  char buf[MAX_INPUT_LENGTH];

  void add_follower(CHAR_DATA *ch, CHAR_DATA *leader, int cmd);
  void stop_follower(CHAR_DATA *ch, int cmd);
  void remove_memory(CHAR_DATA *ch, char type, CHAR_DATA *vict);


  while(*arg == ' ')
    arg++;

  if(!*arg) {
    send_to_char("Who do you want to tame?\n\r", ch);
    return eFAILURE;
  }

  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
     ;
  else if(!has_skill(ch, SKILL_TAME)) {
    send_to_char("Try learning HOW to tame first.\r\n", ch);
    return eFAILURE;
  }

  one_argument(arg, buf);

  if(!(victim = get_char_room_vis(ch, buf))) {
    send_to_char("No one here by that name!\n\r", ch);
    return eFAILURE;
  }

  if(victim == ch) {
    send_to_char("Tame the wild beast!\n\r", ch);
    return eFAILURE;
  }

  if(!IS_NPC(victim)) {
    send_to_char("You find yourself unable to tame this player.\n\r", ch);
    return eFAILURE;
  }

  if(IS_AFFECTED(victim, AFF_CHARM) || IS_AFFECTED(ch, AFF_CHARM) ||
     (GET_LEVEL(ch) < GET_LEVEL(victim))) {
    send_to_char("You find yourself unable to tame this creature.\n\r", ch);
    return eFAILURE;
  }

  if(circle_follow(victim, ch)) {
    send_to_char("Sorry, following in circles can not be allowed.\n\r", ch);
    return eFAILURE;
  }

  
   if(charm_levels(ch) - charm_space(GET_LEVEL(victim)) < 0)  {
     send_to_char("How you plan on controlling so many followers?\n\r", ch);
     return eFAILURE;
/*   CHAR_DATA * vict = NULL;
   for(struct follow_type *k = ch->followers; k; k = k->next)
     if(IS_MOB(k->follower) && affected_by_spell(k->follower, SPELL_CHARM_PERSON))
     {
        vict = k->follower;
        break;
     }
     if (vict) {
	if (vict->in_room == ch->in_room && vict->position > POSITION_SLEEPING)
	  do_say(vict, "Hey... but what about ME!?", 9);
         remove_memory(vict, 'h');
	if (vict->master) { 
         stop_follower(vict, BROKE_CHARM);	
         add_memory(vict, GET_NAME(ch), 'h');
 	}
     }*/
   }

  act("$n holds out $s hand to $N and beckons softly.", ch, NULL, victim, TO_ROOM, INVIS_NULL); 

  WAIT_STATE(ch, PULSE_VIOLENCE * 1);

  if((IS_SET(victim->immune, ISR_CHARM)) ||
      !ISSET(victim->mobdata->actflags, ACT_CHARM)) {
    act("$N is wilder than you thought.", ch, NULL, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if(!skill_success(ch,victim,SKILL_TAME) || saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) >= 0) {
    act("$N is unreceptive to your attempts to tame $M.", ch, NULL, victim, TO_CHAR, 0);
    return eFAILURE;
  }

  if(victim->master)
    stop_follower(victim, 0);

  /* make charmie stop hating tamer */
  remove_memory(victim, 'h', ch);

  add_follower(victim, ch, 0);

  af.type      = SPELL_CHARM_PERSON;

  if(GET_INT(victim))
    af.duration  = 24*18/GET_INT(victim);
  else
    af.duration  = 24*18;

  af.modifier  = 0;
  af.location  = 0;
  af.bitvector = AFF_CHARM;
  affect_to_char(victim, &af);

  /* remove any !charmie eq the charmie is wearing */
  check_eq(victim);

  act("Isn't $n just such a nice person?", ch , 0, victim, TO_VICT, 0);
  return eSUCCESS;
}

int do_track(CHAR_DATA *ch, char *argument, int cmd)
{
  int x,y;
  int retval, how_deep, learned;
  char buf[300];
  char race[40];
  char sex[20];
  char condition[60];
  char weight[40];
  char victim[MAX_INPUT_LENGTH];
  CHAR_DATA *quarry;
  CHAR_DATA *tmp_ch;  // For checking room stuff
  room_track_data * pScent = 0;
  void swap_hate_memory(char_data * ch);
  extern char *dirs[];

  one_argument(argument, victim);

  learned = how_deep = ((has_skill(ch, SKILL_TRACK) / 10) + 1);

  if(GET_LEVEL(ch) >= IMMORTAL)
    how_deep = 50;
 
  quarry = get_char_room_vis(ch, victim);
 
  if(ch->hunting) {
    if(get_char_room_vis(ch, ch->hunting) ) {
      ansi_color( RED, ch);
      ansi_color( BOLD, ch);
      send_to_char("You have found your target!\n\r", ch);
      ansi_color( NTEXT, ch);

//      remove_memory(ch, 't');
      return eSUCCESS;
    } 
  }
  
  else if(quarry) {
    send_to_char("There's one right here ;)\n\r", ch);
  //  remove_memory(ch, 't');
    return eSUCCESS;
  }

  if(*victim && !IS_NPC(ch) && GET_CLASS(ch) != CLASS_RANGER
     && GET_CLASS(ch) != CLASS_DRUID && GET_LEVEL(ch) < ANGEL) {
    send_to_char("Only a ranger or a druid could track someone by name.\n\r", ch);
    return eFAILURE;
  }

  act("$n walks about slowly, searching for signs of $s quarry", ch, 0, 0, TO_ROOM, INVIS_NULL);
  send_to_char("You search for signs of your quarry...\n\r\n\r", ch);

  if(learned)
    skill_increase_check(ch, SKILL_TRACK, learned, SKILL_INCREASE_MEDIUM);

  // TODO - once we're sure that act_mob is properly checking for this,
  // and that it isn't call from anywhere else, we can probably remove it.
  // That way possessing imms can track.
  if(IS_MOB(ch) && ISSET(ch->mobdata->actflags, ACT_STUPID)) {
    send_to_char("Being stupid, you cannot find any..\r\n", ch);
    return eFAILURE;
  }

  if(world[ch->in_room].nTracks < 1) {
    if(ch->hunting) {
      ansi_color( RED, ch);
      ansi_color( BOLD, ch);
      send_to_char("You have lost the trail.\n\r", ch);
      ansi_color( NTEXT, ch);
      //remove_memory(ch, 't');
    }
    else
      send_to_char("There are no distinct scents here.\n\r",ch);
    return eFAILURE;
  }

  if(IS_NPC(ch))
    how_deep = 10;

  if (*victim) 
  { 
    for(x = 1; x <= how_deep; x++) {
        
      if ( (x > world[ch->in_room].nTracks) ||
          !(pScent = world[ch->in_room].TrackItem(x))) 
      {
         if (ch->hunting) 
         {
            ansi_color( RED, ch);
            ansi_color( BOLD, ch);
            send_to_char("You have lost the trail.\n\r", ch);
            ansi_color( NTEXT, ch);
         }
         else
            send_to_char("You can't find any traces of such a scent.\n\r",ch);
//         remove_memory(ch, 't');
         if(IS_NPC(ch))
           swap_hate_memory(ch);
         return eFAILURE;
      }

      if (isname(victim, pScent->trackee)) {
          y = pScent->direction;
          add_memory(ch, pScent->trackee, 't'); 
          ansi_color( RED, ch );
          ansi_color( BOLD, ch);
          csendf(ch, "You sense traces of your quarry to the %s.\n\r",
                  dirs[y]);
          ansi_color( NTEXT, ch); 

          if (IS_NPC(ch)) {
                   // temp disable tracking mobs into town
             if ( (/*!ISSET(ch->mobdata->actflags, ACT_STAY_NO_TOWN) ||*/
	           !IS_SET(zone_table[world[EXIT(ch, y)->to_room].zone].zone_flags, ZONE_IS_TOWN)
                  ) 
                 && !IS_SET(world[EXIT(ch, y)->to_room].room_flags,NO_TRACK)) 
             {
                 ch->mobdata->last_direction = y;
	         retval = do_move(ch, "", (y+1));
                 if(IS_SET(retval, eCH_DIED))
                   return retval;
             }

              if (!ch->hunting)
                 return eFAILURE; 

	      // Here's the deal: if the mob can't see the character in
	      // the room, but the character IS in the room, then the
	      // mob can't see the character and we need to stop tracking.
	      // It does, however, leave the mob open to be taken apart
	      // by, say, a thief.  I'll let he who wrote it fix that.
	      // Morc 28 July 96

              if (!(get_char_room_vis(ch, ch->hunting))) {
	         if ((tmp_ch = get_char(ch->hunting)) == 0)
		    return eFAILURE;
                 if (tmp_ch->in_room == ch->in_room) {
		    // The mob can't see him
                    act("$n says 'Damn, must have lost $M!'", ch, 0, tmp_ch,
		        TO_ROOM, 0);
//                    remove_memory(ch, 't');
	            }
                 return eFAILURE; 
	         }

              if (!IS_SET(world[ch->in_room].room_flags, SAFE))  {
                 act("$n screams 'YOU CAN RUN, BUT YOU CAN'T HIDE!'",
                     ch, 0, 0, TO_ROOM, 0);
                 return do_hit(ch, ch->hunting, 0);
                 }
              else 
                 act("$n says 'You can't stay here forever.'",
                     ch, 0, 0, TO_ROOM, 0);
           } // if IS_NPC
	 
           return eSUCCESS;
        } // if isname
     } // for
	
     if (ch->hunting) {
        ansi_color( RED, ch);
        ansi_color( BOLD, ch);
        send_to_char("You have lost the trail.\n\r", ch);
        ansi_color( NTEXT, ch);
     }
    else 
       send_to_char("You can't find any traces of such a scent.\n\r", ch);
    
//    remove_memory(ch, 't');
    return eFAILURE; 
  } // if *victim


  for(x = 1; x <= how_deep; x++) 
  {
     if ( (x > world[ch->in_room].nTracks) || !(pScent = world[ch->in_room].TrackItem(x))) 
     {
        if (x == 1)
           send_to_char("There are no distinct smells here.\n\r", ch);
        break;
     }
        
     y = pScent->direction;

     if (pScent->weight < 50)
        strcpy(weight," small,");
     else if(pScent->weight <= 201)
        strcpy(weight," medium-sized,");
     else if(pScent->weight < 350)
        strcpy(weight," big,");
     else if(pScent->weight < 500)
        strcpy(weight," large,");
     else if(pScent->weight < 800)
        strcpy(weight," huge,");
     else if(pScent->weight < 1500)
        strcpy(weight," very large,");
     else
        strcpy(weight," gigantic,");

     if (pScent->condition < 10)
        strcpy(condition," severely injured,");
     else if(pScent->condition < 25)
        strcpy(condition," badly wounded,");
     else if(pScent->condition < 40)
        strcpy(condition," injured,");
     else if(pScent->condition < 60)
        strcpy(condition," wounded,");
     else if(pScent->condition < 80)
        strcpy(condition," slightly injured,");
     else
        strcpy(condition," healthy,");

     if (pScent->sex == 1)
        strcpy(sex," male");
     else if (pScent->sex == 2)
        strcpy (sex," female");
     else
        strcpy(sex,""); 

     if (pScent->race >=1 && pScent->race <= 30) 
        sprintf(race, " %s", race_info[pScent->race].singular_name);
     else
        strcpy(race," non-descript race");

    if (number(1, 101) >= (how_deep * 10))
       strcpy(weight,"");
    if (number(1, 101) >= (how_deep * 10))
       strcpy(race," non-descript race");
    if (number(1, 101) >= (how_deep * 10))
       strcpy(condition,"");
    if (number(1, 101) >= (how_deep * 10))
       strcpy(sex,"");

    if (x == 1)
       send_to_char("Freshest scents first...\n\r", ch);
 
    sprintf(buf,"The scent of a%s%s%s%s leads %s.\n\r",
            weight,
            condition,
            sex,
            race,
            dirs[y]);
    send_to_char(buf, ch);
  }
  return eSUCCESS;
}

int ambush(CHAR_DATA *ch)
{
  CHAR_DATA *i, *next_i;
  int retval;

  for(i = world[ch->in_room].people; i; i = next_i) 
  {
     next_i = i->next_in_room;
  
     if(i == ch || !i->ambush || !CAN_SEE(i, ch) || i->fighting)
       continue;

     if(  GET_POS(i) <= POSITION_RESTING || 
          GET_POS(i) == POSITION_FIGHTING ||
          IS_AFFECTED(i, AFF_PARALYSIS) ||
          ( IS_SET(world[i->in_room].room_flags, SAFE) &&
	    !IS_AFFECTED(ch, AFF_CANTQUIT)
          ))
       continue;
     if(!IS_MOB(i) && !i->desc) // don't work if I'm linkdead
       continue;
     if(isname(i->ambush, GET_NAME(ch)))
     {

       if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
        ;
       else if(!has_skill(i, SKILL_AMBUSH))
         continue;

       if(IS_AFFECTED(ch, AFF_ALERT)) {
          send_to_char("Your target is far too alert to accomplish an ambush!\r\n", i);
          continue;
       }

       if(skill_success(i,ch, SKILL_AMBUSH)) 
       { 
//         act("$n ambushes $N in a brilliant surprise attack!", i, 0, ch, TO_ROOM, NOTVICT);
//         act("$n ambushes you as you enter the room!", i, 0, ch, TO_VICT, 0);
//         act("You ambush $N with a brilliant surprise attack!", i, 0, ch, TO_CHAR, 0);
         retval = damage(i, ch, GET_LEVEL(i) * 10, TYPE_HIT, SKILL_AMBUSH, 0); 
         if(IS_SET(retval, eVICT_DIED))
           return (eSUCCESS|eCH_DIED);  // ch = damage vict
         if(IS_SET(retval, eCH_DIED))
           return (eSUCCESS); // doesn't matter, but don't lag vict
         if(!IS_MOB(i) && IS_SET(i->pcdata->toggles, PLR_WIMPY))
            WAIT_STATE(i, PULSE_VIOLENCE * 3);
         else WAIT_STATE(i, PULSE_VIOLENCE * 2);
         WAIT_STATE(ch, PULSE_VIOLENCE * 1);
       }
       // we continue instead of breaking in case there are any OTHER rangers in the room
     }
  }
  return eSUCCESS;
}

int do_ambush(CHAR_DATA *ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];

  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
     ; // do nothing!
  else if(!has_skill(ch, SKILL_AMBUSH)) {
     send_to_char("You don't know how to ambush people!\r\n", ch);
     return eFAILURE;
  }

  one_argument(arg, arg);

  if(!*arg) { 
    sprintf(buf, "You will ambush %s on sight.\n\r", ch->ambush ? ch->ambush : "no one");
    send_to_char(buf, ch);
    return eSUCCESS;
  }

  if(!(ch->ambush)) {
    sprintf(buf, "You will now ambush %s on sight.\n\r", arg);
    send_to_char(buf, ch);
    ch->ambush = str_dup(arg);
    return eSUCCESS;
  } 

  if(!str_cmp(arg, ch->ambush)) {
    sprintf(buf, "You will no longer ambush %s on sight.\n\r", arg);
    send_to_char(buf, ch);
    dc_free(ch->ambush);
    ch->ambush = NULL;
    return eSUCCESS;
  }

  sprintf(buf, "You will now ambush %s on sight.\n\r", arg);

  // TODO - remove this later after I've watched for Bushmaster to do it a few times
  if(strlen(buf) > MAX_INPUT_LENGTH)
    logf(OVERSEER, LOG_BUG, "%s just tried to crash the mud with a huge ambush string (%s)",
          GET_NAME(ch), arg);

  send_to_char(buf, ch);
  dc_free(ch->ambush);
  ch->ambush = str_dup(arg);
  return eSUCCESS;
}

/*
SECT_INSIDE           0
SECT_CITY             1
SECT_FIELD            2
SECT_FOREST           3
SECT_HILLS            4
SECT_MOUNTAIN         5
SECT_WATER_SWIM       6
SECT_WATER_NOSWIM     7
SECT_NO_LOW           8
SECT_NO_HIGH          9
SECT_DESERT          10
underwater
swamp
air
*/

int pick_one(int a, int b)
{
  return number(1, 100) <= 50 ? a : b;
}

int pick_one(int a, int b, int c)
{
  int x = number(1, 100);
  
  if(x > 66)
    return a;
  else if(x > 33)
    return b;
  else
    return c;
}

int pick_one(int a, int b, int c, int d)
{
  int x = number(1, 100);
  
  if(x > 75)
    return a;
  else if(x > 50)
    return b;
  else if(x > 25)
    return c;
  else
    return d;
}

struct forage_lookup {
  int ovnum;
  int rate[4];
};


struct forage_lookup forage_lookup_table[SECT_MAX_SECT+1][6] = {
  // SECT_INSIDE
  {{0,{0,0,0,0}}, 
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}}},

  // SECT_CITY
  {{0,{0,0,0,0}}, 
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}}},

  // SECT_FIELD
  {{6306,{8,9,10,11}}, 
   {6310,{3,4,5,6}},
   {3171,{36,34,32,30}},
   {3173,{7,8,9,10}},
   {3174,{10,11,12,13}},
   {3176,{36,34,32,30}}},

  // SECT_FOREST
  {{6308, {8,9,10,11}}, 
   {6311, {3,4,5,6}},
   {3167, {36,34,32,30}},
   {3175, {36,34,32,30}},
   {3185, {10,11,12,13}},
   {27800,{7,8,9,10}}},

// SECT_HILLS
  {{6306,{8,9,10,11}}, 
   {6310,{3,4,5,6}},
   {3171,{36,34,32,30}},
   {3173,{7,8,9,10}},
   {3174,{10,11,12,13}},
   {3176,{36,34,32,30}}},

  // SECT_MOUNTAIN
  {{6305,{3,4,5,6}}, 
   {3177,{36,34,32,30}},
   {3178,{8,9,10,11}},
   {3179,{7,8,9,10}},
   {3184,{36,34,32,30}},
   {4,   {10,11,12,13}}},

  // SECT_WATER_SWIM
  {{6303,{8,9,10,11}}, 
   {3160,{36,34,32,30}},
   {3162,{7,8,9,10}},
   {3163,{10,11,12,13}},
   {3164,{3,4,5,6}},
   {3189,{36,34,32,30}}},

  // SECT_WATER_NOSWIM
  {{6303,{8,9,10,11}}, 
   {3160,{36,34,32,30}},
   {3162,{7,8,9,10}},
   {3163,{10,11,12,13}},
   {3164,{3,4,5,6}},
   {3189,{36,34,32,30}}},

  // SECT_BEACH
  {{6302,{8,9,10,11}}, 
   {6309,{3,4,5,6}},
   {6304,{7,8,9,10}},
   {3161,{36,34,32,30}},
   {3169,{36,34,32,30}},
   {3187,{10,11,12,13}}},

// SECT_PAVED_ROAD
  {{0,{0,0,0,0}}, 
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}}},

  // SECT_DESERT
  {{6307,{3,4,5,6}}, 
   {3165,{36,34,32,30}},
   {3166,{36,34,32,30}},
   {3172,{8,9,10,11}},
   {3186,{10,11,12,13}},
   {1570,{7,8,9,10}}},

  // SECT_UNDERWATER
  {{0,{0,0,0,0}}, 
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}}},

  // SECT_SWAMP
  {{6301,{8,9,10,11}}, 
   {6312,{3,4,5,6}}, 
   {3170,{36,34,32,30}}, 
   {3180,{7,8,9,10}}, 
   {3181,{10,11,12,13}}, 
   {7717,{36,34,32,30}}},

// SECT_AIR
  {{0,{0,0,0,0}}, 
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}},
   {0,{0,0,0,0}}},

// SECT_FROZEN_TUNDRA
  {{28314,{7,8,9,10}}, 
   {44,{10,11,12,13}},
   {5219,{36,34,32,30}},
   {3168,{8,9,10,11}},
   {28313,{3,4,5,6}},
   {28301,{36,34,32,30}}},

// SECT_ARCTIC
  {{28314,{7,8,9,10}}, 
   {44,{10,11,12,13}},
   {5219,{36,34,32,30}},
   {3168,{8,9,10,11}},
   {28313,{3,4,5,6}},
   {28301,{36,34,32,30}}},
};


int do_forage(CHAR_DATA *ch, char *arg, int cmd)
{
  int learned;
  struct obj_data * new_obj = 0;
  struct affected_type af;

  if(affected_by_spell(ch, SKILL_FORAGE)) {
    send_to_char("You already foraged recently.  Give mother nature a break!\n\r", ch);
    return eFAILURE;
  }
  
  if(GET_MOVE(ch) < 5) {
    send_to_char("You are too tired to be foraging around right now.\n\r", ch);
    return eFAILURE;
  }

  GET_MOVE(ch) -= 5;
 
  learned = has_skill(ch, SKILL_FORAGE);
  if(!learned) {
    send_to_char("Not knowing how to forage, you poke at the dirt with a stick, finding nothing.\r\n", ch);
    return eFAILURE;
  }

  skill_increase_check(ch, SKILL_FORAGE, learned, SKILL_INCREASE_HARD);

  int pick = number(1, 100);
  int ovnum;
  int lgroup;

  if (learned >= 1 && learned <= 30) {
    lgroup = 0;
  } else if (learned >= 31 && learned <= 60) {
    lgroup = 1;
  } else if (learned >= 61 && learned <= 90) {
    lgroup = 2;
  } else if (learned >= 91) {
    lgroup = 3;
  }
  int cur_sector = world[ch->in_room].sector_type;

  // If in a clan or safe room, set sector to inside so we fail forage
  if (IS_SET(world[ch->in_room].room_flags, CLAN_ROOM) ||
      IS_SET(world[ch->in_room].room_flags, SAFE) ||
      IS_SET(world[ch->in_room].room_flags, INDOORS)) {
    cur_sector = SECT_INSIDE;
  }

  int last = 0;

  for (int i=0; i < 6; i++) {
    if (pick > last && pick <= last+forage_lookup_table[cur_sector][i].rate[lgroup]) {
      ovnum = forage_lookup_table[cur_sector][i].ovnum;

      if ((1+IS_CARRYING_N(ch)) > CAN_CARRY_N(ch)) {
	send_to_char("You can't carry that many items!\r\n", ch);
	return eFAILURE;
      }

      new_obj = clone_object(real_object(ovnum));
      break;
    }
    last = last + forage_lookup_table[cur_sector][i].rate[lgroup];
  }

  int recharge;
  if(new_obj)
     recharge = 3 - (learned/40);
  else recharge = 1;

  af.type = SKILL_FORAGE;
  af.duration = recharge;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af, PULSE_REGEN);
  
  if(!new_obj) {
    act("$n forages around for some food, but turns up nothing.", ch, 0, 0, TO_ROOM, 0);
    act("You forage around for some food, but turn up nothing.", ch, 0, 0, TO_CHAR, 0);
    return eFAILURE;
  }
  
  act("$n forages around, turning up $p.", ch, new_obj, 0, TO_ROOM, 0);
  act("You forage around, turning up $p.", ch, new_obj, 0, TO_CHAR, 0);
  obj_to_char(new_obj, ch);
  new_obj->obj_flags.timer = 4;
  return eSUCCESS;
}


/* this is sent the arrow char string, and return the arrow type.
   also, checks level to make sure char is high enough
   return 0 for failure */

int parse_arrow(struct char_data * ch, char * arrow)
{

  if(GET_CLASS(ch) != CLASS_RANGER && GET_LEVEL(ch) < 100)
     return 0;

  while(*arrow == ' ')
    arrow++;

  if((arrow[0] == 'f') && has_skill(ch, SKILL_FIRE_ARROW))
    return 1;
  else if((arrow[0] == 'i') && has_skill(ch, SKILL_ICE_ARROW))
    return 2;
  else if((arrow[0] == 't') && has_skill(ch, SKILL_TEMPEST_ARROW))
    return 3;
  else if((arrow[0] == 'g') && has_skill(ch, SKILL_GRANITE_ARROW))
    return 4;

return 0;
}

/* go through and find an arrow */
/* return NULL if failure
*  return pointer if success */
struct obj_data * find_arrow(struct obj_data *quiver)
{
  struct obj_data *target;

  struct obj_data *get_obj_in_list(char *, struct obj_data *);

  target = get_obj_in_list("arrow", quiver->contains);
  
  if(!target)
    return NULL;
  
  if(!(target->obj_flags.type_flag == ITEM_MISSILE))
    target=NULL;

  return target;
}

void do_arrow_miss(struct char_data *ch, struct char_data *victim, int
                   dir, struct obj_data *found)
{
  char buf[200];

  extern char * dirs[];

  switch(number(1,6))
  {
     case 1: send_to_char("You miss your shot.\r\n", ch);
             break;
     case 2: send_to_char("Your arrow wizzes by the target harmlessely.\r\n", ch);
             break;
     case 3: send_to_char("Your pitiful aim spears a poor woodland creature instead..\r\n", ch);
             break;
     case 4: send_to_char("Your shot misses your target, and goes skittering across the ground.\r\n", ch);
             break;
     case 5: send_to_char("A slight breeze forces your arrow off the mark.\r\n", ch);
             break;
     case 6: send_to_char("Your shot narrowly misses the mark.\r\n", ch);
             break;
  }

  switch(number(1,3))
  {
    case 1:
      if(dir < 0) {
         sprintf(buf, "%s wizzes by.\n\r", found->short_description);
         send_to_char(buf, victim);
         sprintf(buf, "%s wizzes by.", found->short_description);
         act(buf, victim, NULL, ch, TO_ROOM, NOTVICT);
      } else {
         sprintf(buf, "%s wizzes by from the %s.\r\n", found->short_description, dirs[rev_dir[dir]]);
         send_to_char(buf, victim);
         sprintf(buf, "%s wizzes by from the %s.",
               found->short_description, dirs[rev_dir[dir]]);
         act(buf, victim, NULL, 0, TO_ROOM, 0);
      }
      break;
    case 2:
      sprintf(buf, "A quiet whistle sounds as %s flies over your head.", 
               found->short_description);
      act(buf, victim,0,0,TO_CHAR,0);
      sprintf(buf, "A quiet whistle sounds as %s flies over your head.", 
               found->short_description);
      act(buf, victim, 0, ch, TO_ROOM, NOTVICT);
      break;
    case 3:
      if(dir < 0) {
         sprintf(buf, "%s narrowly misses your head.\r\n", found->short_description);
         send_to_char(buf, victim);
         sprintf(buf, "%s narrowly misses $n.", found->short_description);
         act(buf, victim, NULL, ch, TO_ROOM, NOTVICT);
      } else {
         sprintf(buf, "%s from the %s narrowly misses your head.\r\n",
               found->short_description, dirs[rev_dir[dir]]);
         send_to_char(buf, victim);
         sprintf(buf, "%s from the %s narrowly misses $n.",
                found->short_description, dirs[rev_dir[dir]]);
         act(buf, victim, NULL, 0, TO_ROOM, 0);
      }
      break;
  }
}

int mob_arrow_response(struct char_data *ch, struct char_data *victim, 
                        int dir)
{
  int dir2 = 0;
  int retval;

  int attempt_move(CHAR_DATA *, int, int);

  /* this will make IS_STUPID mobs alot easier to kill with arrows,
     but then again, they _are_ 'stupid'.  Keeps people from tracking
     the waterwheel around shire though. 
  */

  if(ISSET(victim->mobdata->actflags, ACT_STUPID))
  {
    if(!number(0,20))
       do_shout(victim, "Duh George, someone keeps shooting me!", 9);    
    return eSUCCESS;
  }

  /* make mob hate the person, but _not_ track them, this should 
   * help make it harder for people to abuse this to RK people.
   * Not impossible, but harder
  */

  add_memory(victim, GET_NAME(ch), 'h');

  /* don't want the mob leaving a fight its already in */
  if(victim->fighting)
     return eSUCCESS;

  if(dir < 0) // in the same room
     return eSUCCESS;

  if(number(0,1))
  {
    /* Send the mob in a random dir */
    if(number(1,2) == 1) {
      do_say(victim, "Where are these fricken arrows coming from?!", 0);
      dir2 = number(0,5);
      if(CAN_GO(victim, dir2))
       if(EXIT(victim, dir2)) {
          if(!( ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN) &&
                IS_SET(zone_table[world[EXIT(victim, dir2)->to_room].zone].zone_flags, ZONE_IS_TOWN)
              )
             && !IS_SET(world[EXIT(victim, dir2)->to_room].room_flags,NO_TRACK))
             /* send 1-6 since attempt move --cmd's it */
             return attempt_move(victim, dir2+1, 0);
       }
    }
  }
  else 
  { 
    /* Send the mob after the fucker! */
    if(number(1,2) == 1) {
      do_say(victim, "There he is!", 0);
    }
    dir2 = rev_dir[dir];
    for(int i = 0; i < 4; i++)
      if(!CAN_GO(ch, dir2))
       dir2 = number(0, 5);
    if(EXIT(victim, dir2)) {
        if(CAN_GO(ch, dir2))
        if(!(ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN) &&
             IS_SET(zone_table[world[EXIT(victim, dir2)->to_room].zone].zone_flags, ZONE_IS_TOWN)))
        if(!IS_SET(world[EXIT(victim, dir2)->to_room].room_flags,NO_TRACK))       
        {
          /* dir+1 it since attempt_move will --cmd it */    
          retval = attempt_move(victim, (dir2+1), 0);
          if(SOMEONE_DIED(retval))
             return retval;

          if(IS_SET(retval, eFAILURE))   // can't go after the archer
            return do_flee(victim, "", 0); 
        }
    }    
      if(number(1, 5) == 1)
       if(number(0,1))
        do_shout(victim, "Where the fuck are these arrows coming from?!", 9);
        else do_shout(victim, "Quit shooting me dammit!", 9);
  }
  return eSUCCESS;
}

/* no need anymore 
int do_arrow_damage(struct char_data *ch, struct char_data *victim, 
                     int dir, int dam, int artype,
                     struct obj_data *found)
{
  char buf[200];
  int retval;

  extern char * dirs[];
  void inform_victim(CHAR_DATA *, CHAR_DATA *, int);

  buf[199] = '\0'; // just cause I'm paranoid

  set_cantquit(ch, victim);
 
  if(0 > (GET_HIT(victim) - dam))
  { // they aren't going to survive..life sucks
   switch(number(1,2)) 
   {
    case 1:
    sprintf(buf, "Your shot impales %s through the heart!\r\n", GET_SHORT(victim));
    send_to_char(buf, ch);
    sprintf(buf, "%s from the %s drives full force into your chest!\r\n",
       found->short_description, dirs[rev_dir[dir]]);
    send_to_char(buf, victim);
    sprintf(buf, "%s from the %s impales $n through the chest!",
       found->short_description, dirs[rev_dir[dir]]);
    act(buf, victim, NULL, 0, TO_ROOM, 0);
    break;

    case 2:
    sprintf(buf, "Your %s drives through the eye of %s ending their life.\r\n", 
       found->short_description, GET_SHORT(victim));
    send_to_char(buf, ch);
    sprintf(buf, "%s drives right through your left eye!\r\nThe last thing through your mind is.........an arrowhead.\r\n", 
       found->short_description);
    send_to_char(buf, victim);
    sprintf(buf, "%s from the %s lands with a solid 'thunk.'\r\n$n falls to the ground, an arrow sticking from $s left eye.", 
       found->short_description, dirs[rev_dir[dir]]);
    act(buf, victim, NULL, 0, TO_ROOM, 0);
    break;
   } // of switch
  }
  else  // they have enough to survive the arrow..lucky bastard
  {
    sprintf(buf, "You hit %s with %s!\r\n", GET_SHORT(victim),
       found->short_description);
    send_to_char(buf, ch);
    sprintf(buf, "You get shot with %s from the %s.  Ouch.",
          found->short_description, dirs[rev_dir[dir]]);
    act(buf, victim, 0, 0, TO_CHAR, 0);
    sprintf(buf, "%s from the %s hits $n!",
          found->short_description, dirs[rev_dir[dir]]);
    act(buf, victim, 0, 0, TO_ROOM, 0);

    switch(artype)
    {
      case 1: send_to_room("It was flaming hot!\r\n", victim->in_room);
              break;
      case 2: send_to_room("It was icy cold!\r\n", victim->in_room);
              break;
      default: break;
    }
    if(IS_NPC(victim)) 
      retval = mob_arrow_response(ch, victim, dir);
      if(SOMEONE_DIED(retval)) // mob died somehow while moving
         return retval;
  } 

  GET_HIT(victim) -= dam;
  update_pos(victim);
  inform_victim(ch, victim, dam);                   

  if(GET_HIT(victim) > 0)
     GET_POS(victim) = POSITION_STANDING;

// This is a cut & paste from fight.C cause I can't think of a better way to do it

  // Payoff for killing things.
  if(GET_HIT(victim) < 0)
  {
    group_gain(ch, victim);
    fight_kill(ch, victim, TYPE_CHOOSE, 0);
    return (eSUCCESS | eVICT_DIED);
  } // End of Hit < 0

  return eSUCCESS;
}
*/

int do_fire(struct char_data *ch, char *arg, int cmd)
{
  struct char_data *victim;
  int dam, dir = -1, artype, cost, retval, victroom;
  struct obj_data *found;
  unsigned cur_room, new_room;
  char direct[MAX_STRING_LENGTH], arrow[MAX_STRING_LENGTH], 
       target[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH], 
       buf2[MAX_STRING_LENGTH], victname[MAX_STRING_LENGTH],
       victhshr[MAX_STRING_LENGTH]; 
  bool enchantmentused = FALSE;
  extern char * dirs[];

  void get(struct char_data *, struct obj_data *, struct obj_data *);
  
  victim = NULL;
  *direct = '\0';
  *arrow = '\0';

  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
    ;
  else if(!has_skill(ch, SKILL_ARCHERY)) {
    send_to_char("You've no idea how those pointy things with strings and feathers work.\r\n", ch);
    return eFAILURE;
  }

  if (!ch->equipment[HOLD])
  {
    send_to_char("You need to be holding a bow moron.\r\n", ch);
    return eFAILURE;
  }

  if(!(ch->equipment[HOLD]->obj_flags.type_flag == ITEM_FIREWEAPON))
  {
    send_to_char("You need to be holding a bow moron.\r\n", ch);
    return eFAILURE;
  }
/*
  if(GET_POS(ch) == POSITION_FIGHTING)
  {
    send_to_char("Aren't you a bit busy with hand to hand combat?\r\n", ch);
    return eFAILURE;
  }*/

  if(ch->shotsthisround > 0)
  {
    send_to_char("Slow down there tiger, you can't fire them that fast!\r\n", ch);
    return eFAILURE;
  }


/*  Command syntax is: fire [target] <dir> [arrowtype] 
    if there is !dir, then check skill level
    if there is !arrow, then choose standard arrowtype */

  while(*arg == ' ')
    arg++;

  if(!*arg)
  {
    send_to_char("Shoot at whom?\r\n", ch);
    return eFAILURE;
  }
  half_chop(arg, target, buf2);
  half_chop(buf2, direct, arrow);

  direct[MAX_STRING_LENGTH-1] = '\0';
  arrow[MAX_STRING_LENGTH-1] = '\0';
  target[MAX_STRING_LENGTH-1] = '\0';

/* make safe rooms checks */
  if(IS_SET(world[ch->in_room].room_flags, SAFE))
  {
    send_to_char("You can't shoot arrows if yer in a safe room silly.\r\n", ch);
    return eFAILURE;
  }

  cost = 0;
  dir = -1;
  artype = 0;

  if(*arrow) {
     artype = parse_arrow(ch, arrow);
     if(!artype)
     {
        send_to_char("You don't know of that type of arrow.\r\n", ch);
        return eFAILURE;
     }
     switch(artype)
     {
        case 1:  cost = 30; break;
        case 2:  cost = 20; break;
        case 3:  cost = 10; break;
        case 4:  cost = 40; break;
     }
  } 
  if(*direct) {
     if(direct[0] == 'n') dir = 0;
     else if(direct[0] == 'e') dir = 1;
     else if(direct[0] == 's') dir = 2;
     else if(direct[0] == 'w') dir = 3;
     else if(direct[0] == 'u') dir = 4;
     else if(direct[0] == 'd') dir = 5;
     else if(direct[0] == 'f' || direct[0] == 't' || direct[0] == 'g' || direct[0] == 'i') artype = parse_arrow(ch, direct);
     else {
        send_to_char("Shoot in which direction?\n\r", ch);
        return eFAILURE;
     }

     if(dir >= 0 && !CAN_GO(ch, dir)) {
        send_to_char("There is nothing to shoot in that direction.\r\n", ch);
        return eFAILURE;
     } else if(artype) {
        switch(artype) {
           case 1:  cost = 30; break;
           case 2:  cost = 20; break;
           case 3:  cost = 10; break;
           case 4:  cost = 40; break;
        }
     }
  }     

  if((GET_MANA(ch) < cost) && (GET_LEVEL(ch) < ANGEL))
  {
    send_to_char("You don't have enough mana for that arrow.\r\n", ch);
    return eFAILURE;
  }

/* check if you can see your target */
/* put ch in the targets room to check if they are visible */
  cur_room = ch->in_room;

  if(target) {
     if(dir >= 0) {
        if(world[cur_room].dir_option[dir] && 
         !(world[cur_room].dir_option[dir]->to_room == NOWHERE) && 
         !IS_SET(world[cur_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
           new_room = world[cur_room].dir_option[dir]->to_room;
           if(IS_SET(world[new_room].room_flags, SAFE)) {
              send_to_char("Don't shoot into a safe room!  You might hit someone!\r\n", ch);
              return eFAILURE;
           }
           char_from_room(ch);
           if(!char_to_room(ch, new_room)) {
              /* put ch into a room before we exit */
              char_to_room(ch, cur_room);
              send_to_char("Error moving you to room in do_fire\r\n", ch);
              return eFAILURE;
           }
           victim = get_char_room_vis(ch, target);
        }
     }
     if(!victim && artype == 3 && dir >= 0) {
        if(world[new_room].dir_option[dir] && 
         !(world[new_room].dir_option[dir]->to_room == NOWHERE) && 
         !IS_SET(world[new_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
           new_room = world[new_room].dir_option[dir]->to_room;
           if(IS_SET(world[new_room].room_flags, SAFE)) {
              send_to_char("Don't shoot into a safe room!  You might hit someone!\r\n", ch);
              return eFAILURE;
           }
           char_from_room(ch);
           if(!char_to_room(ch, new_room)) {
              /* put ch into a room before we exit */
              char_to_room(ch, cur_room);
              send_to_char("Error moving you to room in do_fire\r\n", ch);
              return eFAILURE;
           }
           victim = get_char_room_vis(ch, target);
        }
     }
     if(!victim && artype == 3 && affected_by_spell(ch, SPELL_FARSIGHT) && dir >= 0) {
        if(world[new_room].dir_option[dir] && 
         !(world[new_room].dir_option[dir]->to_room == NOWHERE) && 
         !IS_SET(world[new_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
           new_room = world[new_room].dir_option[dir]->to_room;
           if(IS_SET(world[new_room].room_flags, SAFE)) {
              send_to_char("Don't shoot into a safe room!  You might hit someone!\r\n", ch);
              return eFAILURE;
           }
           char_from_room(ch);
           if(!char_to_room(ch, new_room)) {
              /* put ch into a room before we exit */
              char_to_room(ch, cur_room);
              send_to_char("Error moving you to room in do_fire\r\n", ch);
              return eFAILURE;
           }
           victim = get_char_room_vis(ch, target);
        }        
     }
     if(!victim && has_skill(ch, SKILL_ARCHERY) > 80) {
        char_from_room(ch);
        char_to_room(ch, cur_room);
        victim = get_char_room_vis(ch, target);
        dir = -1;
     }
     else if(dir < 0) {
        send_to_char("You aren't skilled enough to fire at a target this close.\n\r", ch);
        char_from_room(ch);
        char_to_room(ch, cur_room);
        return eFAILURE;
     }
     if(!victim) {
        send_to_char("You can't seem to locate your target.\r\n", ch);
        /* put char back */
        char_from_room(ch);
        char_to_room(ch, cur_room);
        return eFAILURE;
     }
  }
  else {
    send_to_char("Sorry, you must specify a target.\r\n", ch);
    return eFAILURE;
  }
  /* put ch back in original room after successful targeting */
  char_from_room(ch);
  char_to_room(ch, cur_room);

/* check for accidental targeting of self */
  if(victim == ch)
  {
    send_to_char("You need to type more of the target's name.\r\n", ch);
    return eFAILURE;
  }

/* Protect the newbies! */
  if(!IS_NPC(victim) && GET_LEVEL(victim) < 2)
  {
    send_to_char("Don't shoot at a poor level 1! :(\r\n", ch);
    return eFAILURE;
  }
/* check if target is fighting */
/*  if(victim->fighting)
  {
      send_to_char("You can't seem to get a clear line of sight.\r\n", ch);
      return eFAILURE;
  }*/

/* check for arrows here */

  found = NULL;
  if(ch->equipment[WEAR_ABOUT])
    if(ch->equipment[WEAR_ABOUT]->obj_flags.type_flag == ITEM_CONTAINER)
      if((ch->equipment[WEAR_ABOUT]->obj_flags.type_flag == ITEM_CONTAINER) 
         && isname("quiver", ch->equipment[WEAR_ABOUT]->name))
        {found = find_arrow(ch->equipment[WEAR_ABOUT]);
        if(found)
           get(ch,found,ch->equipment[WEAR_ABOUT]); }
   if(!found && ch->equipment[WEAR_WAISTE])
    if((ch->equipment[WEAR_WAISTE]->obj_flags.type_flag == ITEM_CONTAINER) 
        && isname("quiver", ch->equipment[WEAR_WAISTE]->name))
        {found = find_arrow(ch->equipment[WEAR_WAISTE]);
        if(found)
           get(ch,found,ch->equipment[WEAR_WAISTE]); }

   if(!found)
   {
     send_to_char("You aren't wearing any quivers with arrows!\r\n", ch);
     return eFAILURE;
   }

  if (IS_NPC(victim) && mob_index[victim->mobdata->nr].virt >= 2300 &&
        mob_index[victim->mobdata->nr].virt <= 2399)
  {
      send_to_char("Your arrow is disintegrated by the fortress' enchantments.\r\n", ch);
      extract_obj(found);
      return eFAILURE;
  }

/* go ahead and shoot */

  switch(number(1,2))
  {
    case 1: if(dir >= 0)
               sprintf(buf,"$n fires an arrow %sward.", dirs[dir]);
            else
               sprintf(buf, "$n fires an arrow.");
            act(buf, ch, 0, 0, TO_ROOM, 0);
            break;
    case 2: if(dir >= 0)
               sprintf(buf,"$n lets off an arrow to the %s.", dirs[dir]);
            else
               sprintf(buf, "$n lets off an arrow.");
            act(buf, ch, 0, 0, TO_ROOM, 0);
            break;
  }

  GET_MANA(ch) -= cost;

  if(!skill_success(ch,victim, SKILL_ARCHERY))
  {
     retval = eSUCCESS;
     do_arrow_miss(ch, victim, dir, found);
  }
  else {
     dam = dice(found->obj_flags.value[1], found->obj_flags.value[2]);
     dam += dice(ch->equipment[HOLD]->obj_flags.value[1], ch->equipment[HOLD]->obj_flags.value[2]);
     for(int i=0;i < found->num_affects;i++)
        if(found->affected[i].location == APPLY_DAMROLL && found->affected[i].modifier != 0)
           dam += found->affected[i].modifier;
        else if(found->affected[i].location == APPLY_HIT_N_DAM && found->affected[i].modifier != 0)
           dam += found->affected[i].modifier;

     set_cantquit(ch, victim);
     sprintf(victname, "%s", GET_SHORT(victim));
     victroom = victim->in_room;
     strcpy(victhshr, HSHR(victim));

     if(dir >= 0)
        send_to_room("An arrow flies into the room with incredible speed!\n\r", victroom);

     retval = damage(ch, victim, dam, TYPE_PIERCE, SKILL_ARCHERY, 0);

     if(IS_SET(retval, eVICT_DIED))  {
        switch(number(1,2)) {
           case 1:
              if(dir < 0) {
                 sprintf(buf, "The %s impales %s through the heart!", found->short_description, victname);
                 send_to_room(buf, victroom);
              } else {
                 sprintf(buf, "Your shot impales %s through the heart!\r\n", victname);
                 send_to_char(buf, ch);
                 sprintf(buf, "%s from the %s impales %s through the chest!", found->short_description, dirs[rev_dir[dir]], victname);
                 send_to_room(buf, victroom);
              }
              break;
           case 2:
              if(dir < 0) {
                 sprintf(buf, "%s drives through the eye of %s, ending %s life.", found->short_description, victname, victhshr);
                 send_to_room(buf, victroom);
              } else {
                 sprintf(buf, "Your %s drives through the eye of %s ending %s life.\r\n", found->short_description, victname, victhshr);
                 send_to_char(buf, ch);
                 sprintf(buf, "%s from the %s lands with a solid 'thunk.'\r\n%s falls to the ground, an arrow sticking from %s left eye.", found->short_description, dirs[rev_dir[dir]], victname, victhshr);
                 send_to_room(buf, victroom);
              }
              break;
        }
     } else {
        if(dir < 0) {
           sprintf(buf, "You get shot with %s.  Ouch.", found->short_description);
           act(buf, victim, 0, 0, TO_CHAR, 0);
           sprintf(buf, "%s hits $n!", found->short_description);
           act(buf, victim, 0, ch, TO_ROOM, NOTVICT);           
           sprintf(buf, "You hit %s with %s!\r\n", GET_SHORT(victim), found->short_description);
           send_to_char(buf, ch);
        } else {
           sprintf(buf, "You hit %s with %s!\r\n", GET_SHORT(victim), found->short_description);
           send_to_char(buf, ch);
           sprintf(buf, "You get shot with %s from the %s.  Ouch.", found->short_description, dirs[rev_dir[dir]]);
           act(buf, victim, 0, 0, TO_CHAR, 0);
           sprintf(buf, "%s from the %s hits $n!", found->short_description, dirs[rev_dir[dir]]);
           act(buf, victim, 0, 0, TO_ROOM, 0);
         }
        GET_POS(victim) = POSITION_STANDING;

        if(IS_NPC(victim)) 
           retval = mob_arrow_response(ch, victim, dir);
           if(SOMEONE_DIED(retval)) { // mob died somehow while moving
              extract_obj(found);
              return retval;
           }
     }

     if(!SOMEONE_DIED(retval)) {
        cur_room = ch->in_room;
        char_from_room(ch);
        if(!char_to_room(ch, victim->in_room)) {
           char_to_room(ch, cur_room);
           send_to_char("Error moving you to room in do_fire.\n\r", ch);
           return eFAILURE;
        }
        retval = weapon_spells(ch, victim, ITEM_MISSILE);
        //just in case
        if(IS_SET(retval, eCH_DIED)) {
           OBJ_DATA *corpse, *next;
           for(corpse = object_list; corpse; corpse = next) {
              next = corpse->next;
              if(IS_OBJ_STAT(corpse, ITEM_PC_CORPSE) && isname(GET_NAME(ch), GET_OBJ_NAME(corpse))) {
                 obj_from_room(corpse);
                 obj_to_room(corpse, cur_room);
                 save_corpses();
                 break;
              }
           }
        } else {
           char_from_room(ch);
           char_to_room(ch, cur_room);
        }
     }

     if(!SOMEONE_DIED(retval)) {
        switch(artype) {
           case 1:
              dam = 90;
              act("The flames surrounding the arrow burns your wound!", ch, 0, victim, TO_VICT, 0);
              act("The flames surrounding the arrow burns $n's wound!", victim, 0, 0, TO_ROOM, 0);
              retval = damage(ch, victim, dam, TYPE_FIRE, SKILL_FIRE_ARROW, 0);
              skill_increase_check(ch, SKILL_FIRE_ARROW, has_skill(ch, SKILL_FIRE_ARROW), get_difficulty(SKILL_FIRE_ARROW));
              enchantmentused = TRUE;
              break;
           case 2:
              dam = 50;
              act("The stray ice shards impale you!", ch, 0, victim, TO_VICT, 0);
              act("The stray ice shards impale $n!", victim, 0, 0, TO_ROOM, 0);
              if(number(1, 100) < has_skill(ch, SKILL_ICE_ARROW) / 4 ) {
                 act("Your body slows down for a second!", ch, 0, victim, TO_VICT, 0);
                 act("$n's body seems a bit slower!", victim, 0, 0, TO_ROOM, 0);
                 WAIT_STATE(victim, PULSE_VIOLENCE);
              }
              retval = damage(ch, victim, dam, TYPE_COLD, SKILL_ICE_ARROW, 0);
              skill_increase_check(ch, SKILL_ICE_ARROW, has_skill(ch, SKILL_ICE_ARROW), get_difficulty(SKILL_ICE_ARROW));
              enchantmentused = TRUE;
              break;
           case 3:
              dam = 30;
              act("The storm cloud enveloping the arrow shocks you!", ch, 0, victim, TO_VICT, 0);
              act("The storm cloud enveloping the arrow shocks $n!", victim, 0, ch, TO_ROOM, 0);
              retval = damage(ch, victim, dam, TYPE_ENERGY, SKILL_TEMPEST_ARROW, 0);
              skill_increase_check(ch, SKILL_TEMPEST_ARROW, has_skill(ch, SKILL_TEMPEST_ARROW), get_difficulty(SKILL_TEMPEST_ARROW));
              enchantmentused = TRUE;
              break;
           case 4:
              dam = 70;
              act("The magical stones surrounding the arrow smack into you, hard.", ch, 0, victim, TO_VICT, 0);
              act("The magical stones surrounding the arrow smack hard into $n.", victim, 0, 0, TO_ROOM, 0);
              if(number(1, 100) < has_skill(ch, SKILL_GRANITE_ARROW) / 4) {
                 act("The stones knock you flat on your ass!", ch, 0, victim, TO_VICT, 0);
                 act("The stones knock $n flat on $s ass!", victim, 0, 0, TO_ROOM, 0);
                 GET_POS(victim) = POSITION_SITTING;
                 WAIT_STATE(victim, PULSE_VIOLENCE);
              }
              retval = damage(ch, victim, dam, TYPE_HIT, SKILL_GRANITE_ARROW, 0);
              skill_increase_check(ch, SKILL_GRANITE_ARROW, has_skill(ch, SKILL_GRANITE_ARROW), get_difficulty(SKILL_GRANITE_ARROW));
              enchantmentused = TRUE;
              break;
           default: break;
        }
     }
  }

  extract_obj(found);

  if (has_skill(ch, SKILL_ARCHERY) < 51 || enchantmentused) ch->shotsthisround += PULSE_VIOLENCE;
  else if (has_skill(ch, SKILL_ARCHERY) < 86)               ch->shotsthisround += PULSE_VIOLENCE / 2;
  else                                                      ch->shotsthisround += 3;

  return retval;
}

int do_mind_delve(struct char_data *ch, char *arg, int cmd)
{
  char buf[1000];
  char_data * target = NULL;
//  int learned, specialization;

  if(!*arg) {
    send_to_char("Delve into whom?\r\n", ch);
    return eFAILURE;
  }

  one_argument(arg, arg);

/*
  TODO - make this into a skill and put it in

  if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
     learned = 75;
  else if(!(learned = has_skill(ch, SKILL_MIND_DELVE))) {
     send_to_char("You try to think like a chipmunk and go nuts.\r\n", ch);
     return eFAILURE;
  }
  specialization = learned / 100;
  learned = learned % 100;

*/

  target = get_char_room_vis(ch, arg);

  if(GET_LEVEL(ch) < GET_LEVEL(target)) {
    send_to_char("You can't seem to understand your target's mental processes.\r\n", ch);
    return eFAILURE;
  }

  if(!IS_MOB(target)) {
    sprintf(buf, "Ewwwww gross!!!  %s is imagining you naked on all fours!\r\n", GET_SHORT(target));
    send_to_char(buf, ch);
    return eFAILURE;
  }

  act("You enter $S mind...", ch, 0, target, TO_CHAR, INVIS_NULL);
  sprintf(buf, "%s seems to hate... %s.\r\n", GET_SHORT(target), 
            ch->mobdata->hatred ? ch->mobdata->hatred : "Noone!");
  send_to_char(buf, ch);
  if(ch->master)
    sprintf(buf, "%s seems to really like... %s.\r\n", GET_SHORT(target), 
            GET_SHORT(ch->master));
  else sprintf(buf, "%s seems to really like... %s.\r\n", GET_SHORT(target), 
            "Noone!");
  send_to_char(buf, ch);
  return eSUCCESS;
}

void check_eq(CHAR_DATA *ch)
{
   int pos;

   for (pos = 0; pos < MAX_WEAR; pos++) {
      if (ch->equipment[pos])
         equip_char(ch, unequip_char(ch, pos), pos);
   }
}
