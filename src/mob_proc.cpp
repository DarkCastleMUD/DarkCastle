/***************************************************************************
 *  file: spec_pro.c , Special module.                     Part of DIKUMUD *
 *  Usage: Procedures handling special procedures for object/room/mobile   *
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
/* $Id: mob_proc.cpp,v 1.180 2008/11/10 03:38:43 kkoons Exp $ */
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <assert.h>
#include <character.h>
#include <structs.h>
#include <utility.h>
#include <mobile.h>
#include <spells.h>
#include <room.h>
#include <handler.h>
#include <magic.h>
#include <levels.h>
#include <fight.h>
#include <obj.h>
#include <player.h>
#include <connect.h>
#include <interp.h>
#include <isr.h>
#include <race.h>
#include <db.h> // real_room
#include <sing.h> // bard skills
#include <act.h>
#include <ki.h> // monk skills
#include <string.h>
#include <returnvals.h>

/*   external vars  */

extern CWorld world;
 
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct index_data *mob_index;
extern struct time_info_data time_info;

int check_components(CHAR_DATA *ch, int destroy, int item_one = 0,
	int item_two = 0, int item_three = 0, int item_four = 0,
	bool silent = FALSE);
/* extern procedures */

int saves_spell(CHAR_DATA *ch, CHAR_DATA *vict, int spell_base, int16 save_type);
bool many_charms(struct char_data *ch);
struct char_data *get_pc_vis_exact(struct char_data *ch, char *name);
void gain_exp(struct char_data *ch, int64 gain);

char * get_random_hate(CHAR_DATA *ch);


// This is purely a utility function for use inside of other mob_procs.
// You send it the mob (ch) and the Vnum of the people you want to join it
// (iFriendId) and it will search for them, call for help, and they will
// join in combat.  (If they are in the room)
// Returns TRUE if we got help, FALSE if not

//Marauder proc uses this
int call_for_help_in_room(struct char_data *ch, int iFriendId)
{
  struct char_data * ally = NULL;
  int friends = 0;

  if(!ch)
    return FALSE;

  // Any friends in the room?  Call for help!   int friends = 0;
  for(ally = world[ch->in_room].people; ally; ally = ally->next_in_room )
  {
    if(!IS_MOB(ally))
      continue;
    if(ally == ch)
      continue;
    if(ally == ch->fighting)
      continue;
 
    if(real_mobile(iFriendId) == ally->mobdata->nr)
    {
      if(!can_be_attacked(ally, ch->fighting))
        continue;
      if(ally->fighting)
        continue;

      if(!friends)
      {
        do_say(ch, "This guy is beating the hell out of me!  HELP!!", 9);
        friends = 1;
      }
      do_say(ally, "I shall come to your aid!", 9);
      attack(ally, ch->fighting, TYPE_UNDEFINED);
    }
  }
  return friends;
}

// Protect is another purely utilitarian function to include inside a proc.
// You simply call protect() with the vnum of the mob they want to protect.
// They will check if that person is in the room and fighting.  If they are,
// the protector will join them and attempt to rescue.
// standard return values

//Several procs use this
int protect(struct char_data *ch, int iFriendId)
{
  struct char_data * ally = NULL;
  struct char_data * tmp_ch = NULL;
  int retval;

  if(!ch)
    return eFAILURE;

  // Any one I need to protect in the room?
  for(ally = world[ch->in_room].people; ally; ally = ally->next_in_room )
  {
    if(!IS_MOB(ally))
      continue;
    if(!ally->fighting) // if they arne't fighting, they're safe
      continue;
    if(ally == ch)
      continue;
    if(ally == ch->fighting)
      continue;

    if(real_mobile(iFriendId) == ally->mobdata->nr)
    {
      // obscure whitney houston joke
      do_say(ch, "and IiiiIIiiii will always, looove yooooou!", 9);
      // do join
      retval = attack(ch, ally->fighting, TYPE_UNDEFINED);
      if(SOMEONE_DIED(retval))
        return retval;
      // pertant rescue code (easier than calling it)
      send_to_char("Banzai! To the rescue...\n\r", ch);
      act("You are rescued by $N, you are confused!",
                 ally, 0, ch, TO_CHAR, 0);
      act("$n heroically rescues $N.", ch, 0, ally, TO_ROOM, NOTVICT);

      tmp_ch = ally->fighting;
      stop_fighting(ally);
      if (tmp_ch->fighting)
          stop_fighting(tmp_ch);
      if (ch->fighting)
          stop_fighting(ch);

      set_fighting(ch, tmp_ch);
      set_fighting(tmp_ch, ch);
      return eSUCCESS;
    }
  }
  return eFAILURE;
}

// find_random_player_in_room is another purely utilitarian function to
// include inside a proc.  You call it with the mob that you are using,
// and it returns either the pointer to a random player in the room,
// or NULL.

//Pagoda place uses this
char_data * find_random_player_in_room(char_data * ch)
{
    char_data * vict = NULL;
    int count = 0;

    // Count the number of players in room
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
       if(!IS_NPC(vict))
          count++;

    if(!count) // no players
       return NULL;

    // Pick a random one
    count = number(1, count);

    // Find the "count" player and return them
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
       if(!IS_NPC(vict))
          if(count > 1)
             count--;
          else return vict;

    // we should never get here
    return NULL;     
}

// Call this for any "area effect" damage you want to do to all the
// players in the room.  Useful for doing an "earthquake" without
// hurting the mob standing next to you.  Just do the messages yourself
// and the call this function to deal the damage you want to do
void damage_all_players_in_room(struct char_data *ch, int damage)
{
    char_data * vict = NULL;
    char_data * next_vict = NULL;
    void inform_victim(CHAR_DATA *ch, CHAR_DATA *vict, int dam);

    for (vict = world[ch->in_room].people; vict; vict = next_vict)
    {    
      // we need this here in case fight_kill moves our victim
      next_vict = vict->next_in_room;

      if(IS_NPC(vict))
        continue;
      if(ch == vict)
        continue;
      if(GET_LEVEL(vict) >= IMMORTAL)
        continue;

      if(affected_by_spell(vict, SPELL_DIVINE_INTER) && damage > affected_by_spell(vict, SPELL_DIVINE_INTER)->modifier)
         GET_HIT(vict) -= affected_by_spell(vict, SPELL_DIVINE_INTER)->modifier;
      else
         GET_HIT(vict) -= damage; // Note -damage will HEAL the player
      update_pos(vict);
      inform_victim(ch, vict, damage);
      if(GET_HIT(vict) < 1)
        fight_kill(ch, vict, TYPE_CHOOSE, 0);
    }
}

// Call this function with the summoner, and the vnum of the mobs
// you want summoned into the room with you.  It will pull them from
// anywhere in the world to you.
void summon_all_of_mob_to_room(struct char_data * ch, int iFriendId)
{
  struct char_data * victim = NULL;
  extern char_data * character_list;

  if(!ch)
    return;

  for(victim = character_list; victim; victim = victim->next)
  {
    if(!IS_MOB(victim))
      continue;
    if(real_mobile(iFriendId) == victim->mobdata->nr)
    {
      move_char(victim, ch->in_room);
    }
  }  
}

// Call this function with the finder, and the vnum of the mob you
// want to find, and it will return his pointer if he's in the room.
// If we find him, return his pointer
// If it doesn't, return NULL
char_data * find_mob_in_room(struct char_data *ch, int iFriendId)
{
  struct char_data * ally = NULL;

  if(!ch)
    return NULL;

  // Is my friend in the room?
  for(ally = world[ch->in_room].people; ally; ally = ally->next_in_room )
  {
    if(!IS_MOB(ally))
      continue;
    if(real_mobile(iFriendId) == ally->mobdata->nr)
      return ally;
  }
  return NULL;
}

//Spellcraft golem stuff.
int sc_golem(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   if (cmd) return eFAILURE;
   if (!ch->fighting) return eFAILURE;
   bool iron = FALSE;
   if (IS_AFFECTED(ch, AFF_GOLEM))  iron = TRUE;
   SPELL_FUN *iron_list[] = {
      cast_shocking_grasp,
      cast_lightning_bolt,
      cast_sparks,
      cast_chill_touch,
      cast_cure_critic
   };
   SPELL_FUN *stone_list[] = {
      cast_fireball,
      cast_cause_critical,
      cast_meteor_swarm,
      cast_bee_sting,
      cast_cure_critic
   };
   if (!(owner = ch->master) || !(has_skill(ch->master,SKILL_SPELLCRAFT) > 80))
      return eFAILURE;
   int i = number(0,4);
   SPELL_FUN *func = iron?iron_list[i]:stone_list[i];
   
   if (i == 4)
  return (*func)(50, ch, "",  SPELL_TYPE_SPELL, ch, 0, 0);
   else
  return (*func)(50, ch, "",  SPELL_TYPE_SPELL, ch->fighting, 0, 0);
}


// Couple mobs actually still use this.
int fighter(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    /*struct obj_data *obj;*/
    struct obj_data *wielded;
    struct char_data *vict;

    if(cmd) return eFAILURE;
    if (GET_POS(ch) < POSITION_FIGHTING) return eFAILURE;
    if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;
    if(MOB_WAIT_STATE(ch)) {
      return eFAILURE;
    }

    vict = ch->fighting;

    if (!vict)
       return eFAILURE;

    // Deathstroke my opponent whenever possible
    if(GET_LEVEL(ch)>39 && GET_POS(vict) < POSITION_FIGHTING)
    {
      MOB_WAIT_STATE(ch) = 2;
      return do_deathstroke(ch, "", 9);
    }

    if (ch->equipment[WIELD] && vict->equipment[WIELD])
      if (!IS_NPC(ch) || !IS_NPC(vict))
      {
        wielded = vict->equipment[WIELD];
        if ((!IS_SET(wielded->obj_flags.extra_flags ,ITEM_NODROP)) &&
            (GET_LEVEL(vict) <= MAX_MORTAL ))
          if( vict==ch->fighting && GET_LEVEL(ch)>9 && number(0,2)==0 )
          {
             MOB_WAIT_STATE(ch) = 2;
             disarm(ch,vict);
             return eSUCCESS;
          }
      }

    if( vict==ch->fighting && GET_LEVEL(ch)>3 && number(0,2)==0 )
    {
       MOB_WAIT_STATE(ch) = 3;      
       return do_bash(ch, "", 9);
    }
    if (vict==ch->fighting && GET_LEVEL(ch)>2 && number(0,1)==0 )
    {
       MOB_WAIT_STATE(ch) = 2;
       return do_kick(ch, "", 9);
    }

   return eFAILURE;
}


int active_tarrasque(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner) 
{
    CHAR_DATA *vict;

    if((GET_POS(ch) != POSITION_FIGHTING) || (!ch->fighting)) {
        return eFAILURE;
    }

    vict = ch->fighting;

    if (!vict)
	return eFAILURE;

   
 if (!IS_AFFECTED(vict, AFF_PARALYSIS))
    if(GET_LEVEL(ch)>30 && number(0,2)==0 )
       {
         if ((IS_AFFECTED(vict, AFF_SANCTUARY))  ||
             (IS_AFFECTED(vict, AFF_FIRESHIELD))  ||
             (IS_AFFECTED(vict, AFF_HASTE))) {
   
  act("$n utters the words 'Instant Magic Remover(tm)'.", ch, 0, 0, TO_ROOM,
    INVIS_NULL);
         return cast_dispel_magic(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
        }
      }



    if(GET_LEVEL(ch)>40 && number(0,2)==0 )
    {
	act("$n utters the words 'Burn in hell'.", ch, 0, 0, TO_ROOM,
	  INVIS_NULL);
	return cast_hellstream(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }

    if(GET_LEVEL(ch)>40 && number(0,1)==0 )
    {
	act("$n utters the words 'Go away pest'.", ch, 0, 0, TO_ROOM,
	  INVIS_NULL);
	return cast_teleport(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }
     
  

    act("$n utters the words 'I love Acid!'.", ch, 0, 0, TO_ROOM,
	      INVIS_NULL);
    return cast_acid_blast(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
}


int active_grandmaster(CHAR_DATA *ch, struct obj_data *obj, int command, char *arg,        
          struct char_data *owner) 
{
    CHAR_DATA *vict;
    /* Find a dude to do evil things upon ! */
    if((GET_POS(ch) != POSITION_FIGHTING)) {
        return eFAILURE;
    }

    vict = ch->fighting;

    if (!vict)
	return eFAILURE;

   
 if (!IS_AFFECTED(vict, AFF_PARALYSIS))
    if(GET_LEVEL(ch)>30 && number(0,4)==0 )
       {
         if ((IS_AFFECTED(vict, AFF_SANCTUARY))  ||
             (IS_AFFECTED(vict, AFF_FIRESHIELD))  ||
             (IS_AFFECTED(vict, AFF_HASTE))) {
   
  act("$n utters the words 'Instant Magic Remover(tm)'.", ch, 0, 0,
   TO_ROOM, INVIS_NULL);
         return cast_dispel_magic(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
        }
      }

    if(GET_LEVEL(ch)>40 && number(0,2)==0 )
    {
	act("$n utters the words 'Burn them suckers'.", ch, 0, 0, TO_ROOM,
	  INVIS_NULL);
	return cast_firestorm(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }

    if(GET_LEVEL(ch)>40 && number(0,2)==0 )
    {
	act("$n utters the words 'Burn in hell'.", ch, 0, 0, TO_ROOM, 
	  INVIS_NULL);
	return cast_hellstream(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }

  if ( !IS_AFFECTED(vict, AFF_PARALYSIS) )
    if(GET_LEVEL(ch)>40 && number(0,1)==0 )
    {
	act("$n utters the words 'Go away pest'.", ch, 0, 0, TO_ROOM,
	  INVIS_NULL);
	return cast_teleport(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }

            act("$n utters the words 'I love Acid!'.", ch, 0, 0, TO_ROOM,
	      INVIS_NULL);
              return cast_acid_blast(
                   GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
}

static char *frostyYellText [ ] = {
  "I >WAS< female but my breasts slid down in the heat.",
  "WOW, it sure is hot, who wants a frosty? *winkwink*",
  "Hey, who made me out of yellow snow?",
  "Deck the halls with boughs of CARNAGE!",
  "Where's that creepy little red-nosed reindeer?",
  "To hell with eggnog, I want Jose's worm!",
  "Who wants to lick my candycane?",
  "Whoa, my hand's frozen to it!",
  "Happy Mudding you little perverts !!",
  "You can't touch this.",
  "Hey Malibu Barbie, this bottom snowball is actually a huge sperm sack.",
  "What the hell good is this fucking broomstick ?!? Give me some GODLOAD",
  "99 bottles of Molson Ice on the wall, 99 bottles of Molson....",
  "I got frozen for cheating...hahah get it..frozen ? I thought it was funny",
  "Global Warming ?? Uh-Oh",
  "Cottonmouth ! Aggh, I need water! Oh, hey I'll just lick myself.",
  "Hey, you want to see some *real* snowballs ?!?",
  "Corncob pipe?  Give me a big fat cuban!",
  "I've got some mistletoe right here in my pants for you baby.",
  "See this carrot nose ? It'll make you squeal."
};

#define FROSTY_YELL_TEXT_SIZE ( sizeof(frostyYellText) / sizeof(char *) )

int frosty (struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
     int x;

     if(IS_AFFECTED(ch,AFF_PARALYSIS))
       return eFAILURE;

     if (cmd)
       return eFAILURE;

     x = number (0,FROSTY_YELL_TEXT_SIZE * 60);

     if((unsigned) x < FROSTY_YELL_TEXT_SIZE) {
       do_shout(ch, frostyYellText [ x ], 9);
       return eSUCCESS;
     }
     return eFAILURE;
}

static char *poetSayText [] = {
   "I saw the best minds of my generation destroyed by mistletoe,",
   "Dragging themselves through the snowy streets looking for a fix,",
   "Angelheaded hipsters burning for the ancient heavenly connection,",
   "Hollow-eyed and high sat up smoking in the supernatural darkness,",
   "Floating across the tops of cities contemplating jazz,",
   "Who saw Greater Powers staggering on tenement roofs illuminated,",
   "Who passed with radiant cool eyes hallucinating Thalos,",
   "Expelled from guildhouses for crazy and publishing obscene odes,",
   "Who cowered in unshaven rooms in underwear, burning money,",
   "Who ate fire in paint hotels or drank turpentine in Paradise,",
   "With dreams, with drugs, with waking nightmares,",
   "Incomparable blind streets of shuddering cloud and mind-lightning,",
   "Illuminating all the motionless world of Time between,",
   "Backyard green tree cemetery dawns, wine drunkeness over the rooftops,",
   "Sun and moon and tree vibrations in the winter dusks of Hyperborea,",
   "A lost battalion of platonic conversationalists jumping from the moon,",
   "Leaving a trail of ambiguous picture postcards of Midgaard,",
   "Who wondered where to go, and went, leaving no broken hearts,",
   "Who disappeared in the volcanoes of EC, leaving the lava of poetry,",
   "Who wandered on the snowbank docks waiting for a door to open,"
};

static char *poetEmoteText [] = {
   "bursts into tears at the beauty of his own writing.",
   "sips his cup of coffee and licks his lips.",
   "clears his throat and coughs significantly.",
   "glares at a couple necking in the corner.",
   "looks to see if there are any agents in the audience.",
   "stubs out his cigarette and lights another.",
   "flicks some ashes on the floor.",
   "stops to autograph a chapbook for his grandmother.",
   "clutches his head and sobs in existential angst.",
   "shoves a rival poet off the stage and glares about himself fiercely.",
   "drums his fingers on the podium while trying to make out his handwriting.",
   "blows a series of smoke-rings into the air and pauses to admire them."
};

#define POET_SAY_TEXT_SIZE (sizeof(poetSayText) / sizeof(char *) )
#define POET_EMOTE_TEXT_SIZE (sizeof(poetEmoteText) / sizeof(char *) )

int poet (struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   int x;

   if(IS_AFFECTED(ch,AFF_PARALYSIS))
     return eFAILURE;

   if(cmd)
      return eFAILURE;

   x = number (0,POET_SAY_TEXT_SIZE*10);

   if((unsigned) x < POET_SAY_TEXT_SIZE) {
      do_say(ch,poetSayText [x],0);
      return eSUCCESS;
    }

   x = number (0,POET_EMOTE_TEXT_SIZE*30);

   if((unsigned)  x < POET_EMOTE_TEXT_SIZE) {
      do_emote(ch,poetEmoteText [x],0);
      return eSUCCESS;
    }
      return eFAILURE;
    }

static char *stcrewEmoteText [] = {
  "dives to the ground, firing several disruptor shots into the ceiling.",
  "cackles madly, spraying disruptor fire into the corridor."
};

#define STCREW_EMOTE_TEXT_SIZE (sizeof(stcrewEmoteText) / sizeof (char *) )

int stcrew (struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    int x;

    if(IS_AFFECTED(ch,AFF_PARALYSIS))
     return eFAILURE;

    if(cmd)
     return eFAILURE;

    x = number (0,STCREW_EMOTE_TEXT_SIZE * 20);

    if((unsigned) x < STCREW_EMOTE_TEXT_SIZE) {
     do_emote(ch, stcrewEmoteText [x], 0);
     return eSUCCESS;
    }
    return eFAILURE;
}

static char *stofficerEmoteText [] = {
  "flinches as sparks shower the hallway behind him.",
  "fiddles with his phaser setting and fires into the archway."
};

#define OFFICER_EMOTE_TEXT_SIZE (sizeof(stofficerEmoteText)/sizeof(char *))

int stofficer(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    int x;

    if(IS_AFFECTED(ch,AFF_PARALYSIS))
     return eFAILURE;

    if(cmd)
     return eFAILURE;

    x = number (0,OFFICER_EMOTE_TEXT_SIZE * 20);

    if((unsigned) x < OFFICER_EMOTE_TEXT_SIZE) {
     do_emote(ch, stofficerEmoteText [x], 0);
     return eSUCCESS;
    }
    return eFAILURE;
}


int backstabber(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *tch;
    /*struct char_data *mob;
    char buf[MAX_INPUT_LENGTH];

    char *strName;*/

    if (cmd || !AWAKE(ch))
	return eFAILURE;

     if (ch->fighting) {

       if (number(0,6)==0)
          do_flee(ch, "", 0);

         return eSUCCESS;
            }


    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
        {

        if (!IS_NPC(tch))
           if (CAN_SEE(ch, tch)) {

    if(!can_attack(ch) || !can_be_attacked(ch, tch))
          return eFAILURE;
	   
          if (!IS_MOB(tch) && IS_SET(tch->pcdata->toggles, PLR_NOHASSLE))
                  continue;

    if (IS_AFFECTED(tch, AFF_PROTECT_EVIL)) {
         if (IS_EVIL(ch) && (GET_LEVEL(ch) <= GET_LEVEL(tch)))
             continue;
               }
    if (affected_by_spell(tch, SPELL_PROTECT_FROM_GOOD)) {
         if (IS_GOOD(ch) && (GET_LEVEL(ch) <= GET_LEVEL(tch)))
             continue;
               }
  
         if (MOB_WAIT_STATE(ch)) continue;
         if (IS_AFFECTED(tch,AFF_ALERT)) continue;
         if (ch->equipment[WIELD])  {

        if (tch->fighting) {

        act ("$n circles around $s target....\n\r", ch, 0, 0, TO_ROOM, 
	  INVIS_NULL);
         SET_BIT(ch->combat, COMBAT_CIRCLE);
           }

               return attack (ch, tch, SKILL_BACKSTAB);
      }  else return attack (ch, tch, TYPE_UNDEFINED);

              return eSUCCESS;
          }
    }
          return eFAILURE;
}





int white_dragon(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *vict = ch->fighting;

    if(cmd) 
      return eFAILURE;

    if(GET_POS(ch)!=POSITION_FIGHTING) 
      return eFAILURE;
    
    if(!ch->fighting) 
      return eFAILURE;

    if(number(0,6) > 1)
      return eFAILURE;

    act("$n breathes frost.",ch, 0, 0, TO_ROOM, 0);
    cast_frost_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));

    return eSUCCESS;
}

int black_dragon(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *vict = ch->fighting;


    if(cmd) 
      return eFAILURE;

    if(GET_POS(ch)!=POSITION_FIGHTING) 
      return eFAILURE;
    
    if(!ch->fighting) 
      return eFAILURE;

    /* Find a dude to do evil things upon ! */

    if(number(0,6) > 1)
      return eFAILURE;

    act("$n breathes acid.", ch, 0, 0, TO_ROOM, 0);
    return cast_acid_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
}

int red_dragon(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{

    if(cmd) 
      return eFAILURE;

    if(GET_POS(ch)!=POSITION_FIGHTING) 
       return eFAILURE;
    
    if(!ch->fighting) 
       return eFAILURE;

    if(number(0,6) > 1) 
      return eFAILURE;

    act("$n breathes fire.", ch, 0, 0, TO_ROOM, 0);
    return cast_fire_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
}

int green_dragon(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{

    if (cmd) 
       return eFAILURE;

    if(GET_POS(ch)!=POSITION_FIGHTING) 
      return eFAILURE;
    
    if(!ch->fighting) 
      return eFAILURE;

    if(number(0,6) > 1) 
      return eFAILURE;

    act("$n breathes gas.", ch, 0, 0, TO_ROOM, 0);
    return cast_gas_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
}

int brass_dragon(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *vict;

    if ( cmd == 4 && ch->in_room == real_room(5065) )
    {
	act( "The brass dragon says '$n isn't invited'",
	    ch, 0, 0, TO_ROOM, 0 );
	send_to_char( "The brass dragon says 'you're not invited'\n\r", ch );
	return eSUCCESS;
    }

    if ( cmd )
	return eFAILURE;

    if(GET_POS(ch)!=POSITION_FIGHTING) return eFAILURE;

    if (!ch->fighting) return eFAILURE;

    if (number(0,4)==0)
    {
	act("$n breathes gas.",ch, 0, 0, TO_ROOM, 0);
	return cast_gas_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
    }

    vict = ch->fighting;

    if (!vict)
	if (number(0,2) == 0)
	{
	    vict = ch->fighting;
	    if (vict == NULL)
		return eFAILURE;
	}
	else
	    return eFAILURE;

    act("$n breathes lightning.", ch, 0, 0, TO_ROOM, 0);
    return cast_lightning_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
}
  
int francis_guard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  if (cmd != 1) return eFAILURE;
  if (world[ch->in_room].number == 7077)
  {
    do_say(owner, "Oh no you don't!", 9);
    attack(owner, ch, 0);
    return eSUCCESS;
  }
  return eFAILURE;
}

/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

int guild_guard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if (cmd>6 || cmd<1)
	return eFAILURE;
    int dir = 0,clas=0, align = 0, dir2 = 0;
    // TODO - go through these and remove all of the ones that are in
    // room that no longer exist on the mud
    // 3 = south, 2 = east, 5 = up
    // 1 = north, 4  = west, 6 = down

    switch (world[ch->in_room].number)
    {
	case 1978:
	  dir = 4; clas = CLASS_MAGIC_USER; break;
	case 1910:
	  dir = 1; clas = CLASS_THIEF; break;
	case 3004:
	  dir = 0; clas = CLASS_CLERIC; break;
	case 1960:
	  dir = 0; clas = CLASS_MONK; break;
	case 1926:
	  dir = 0; clas = CLASS_BARBARIAN; break;
	case 1968:
	  dir = 0; clas = CLASS_PALADIN; align = 3; break;
	case 1942:
	  dir = 2; clas = CLASS_WARRIOR; break;
	case 1988:
	  dir = 2; clas = CLASS_DRUID; break;
 	case 1900:
	  dir = 3; clas = CLASS_BARD; break;
	case 1920:
	  dir = 3; align = 1; clas = CLASS_ANTI_PAL; break;
	case 1935:
	  dir = 3; clas = CLASS_RANGER; break;
        default: return eFAILURE;
    }
    dir++;
//    cmd++;
    if ((cmd == dir || cmd == dir2) && (
        (!IS_MOB(ch) && (affected_by_spell(ch, FUCK_PTHIEF) || 
affected_by_spell(ch, FUCK_GTHIEF) || IS_AFFECTED(ch, AFF_CHAMPION))) ||
GET_CLASS(ch) != clas || (align == 1 && !IS_EVIL(ch)) || (align == 3 && 
!IS_GOOD(ch)))) {
	act( "The guard humiliates $n, and blocks $s way.",
	    ch, 0, 0, TO_ROOM , 0);
	send_to_char(
	    "The guard humiliates you, and blocks your way.\n\r", ch );
	return eSUCCESS;
    }

    return eFAILURE;
}

int clan_guard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    int in_room = ch->in_room;

    if (cmd>6 || cmd<1)
	return eFAILURE;
    // 3 = south, 2 = east, 5 = up
    // 1 = north, 4  = west, 6 = down

    if ((in_room == real_room(2300) && cmd != CMD_UP) || // up
        (in_room == real_room(2310) && cmd != CMD_EAST) || // east
        (in_room == real_room(2320) && cmd != CMD_SOUTH) || // south
        (in_room == real_room(2390) && cmd != CMD_SOUTH) || // south
        (in_room == real_room(2330) && cmd != CMD_SOUTH) || // south
        (in_room == real_room(2340) && cmd != CMD_SOUTH) || // south
        (in_room == real_room(2350) && cmd != CMD_SOUTH) || // south
        (in_room == real_room(2360) && cmd != CMD_SOUTH) || // south
        (in_room == real_room(2370) && cmd != CMD_DOWN) || // down
        (in_room == real_room(2410) && cmd != CMD_UP) || // up
        (in_room == real_room(2420) && cmd != CMD_SOUTH) || // south
        (in_room == real_room(2380) && cmd != CMD_NORTH) || // north
        (in_room == real_room(2400) && cmd != CMD_WEST) || // west
        (in_room == real_room(2430) && cmd != CMD_SOUTH) || // south
        (in_room == real_room(2440) && cmd != CMD_NORTH) || // north
	(in_room == real_room(2460) && cmd != CMD_NORTH) || // north
        (in_room == real_room(2450) && cmd != CMD_WEST) || // west
	(in_room == real_room(2500) && cmd != CMD_NORTH)) //north
        return eFAILURE;

    int clan_num = ch->clan;
     if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))
     {
	int b = mob_index[ch->mobdata->nr].virt;
	switch (b)
	{
		case 8:
		case 88:
		case 89:
		case 90:
		case 91:
		case 22389:
		case 22391:
		case 22392:
		case 22393:
		case 22394:
		case 22395:
		case 22396:
		case 22397:
		case 22398:
			if (ch->master) clan_num = ch->master->clan;
		default:
		break;
	}
     }

    if ( (clan_num != 14 && in_room == real_room(2300))  // black_axe
    ||   (clan_num !=  4 && in_room == real_room(2310))  // dc_guard
    ||   (clan_num != 18 && in_room == real_room(2320))  // anarchist
    ||   (clan_num != 20 && in_room == real_room(2390))  // sindicate
    ||   (clan_num !=  1 && in_room == real_room(2330))  // uln'hyrr
    ||   (clan_num != 10 && in_room == real_room(2340))  // moor
    ||   (clan_num !=  9 && in_room == real_room(2350))  // eclipse
    ||   (clan_num !=  3 && in_room == real_room(2360))  // arcana
    ||   (clan_num != 17 && in_room == real_room(2370))  // voodoo
    ||   (clan_num != 13 && in_room == real_room(2380))  // slackers
    ||   (clan_num !=  6 && in_room == real_room(2410))  // timewarp
    ||   (clan_num != 19 && in_room == real_room(2420))  // solaris
    ||   (clan_num !=  8 && in_room == real_room(2400))  // merc
    ||   (clan_num != 15 && in_room == real_room(2430))  // askan'i
    ||   (clan_num !=  2 && in_room == real_room(2440))  // tengu
    ||   (clan_num != 16 && in_room == real_room(2450))  // houseless_rogues
    ||   (clan_num != 12 && in_room == real_room(2460))  // ko'bal
    ||   (clan_num != 11 && in_room == real_room(2500))  // triad
  
	)
    {
	act( "$n is turned away from the clan hall.", ch, 0, 0, TO_ROOM , 0);
	send_to_char("The clan guard throws you out on your ass.\n\r", ch );
	return eSUCCESS;
    }
    else if(affected_by_spell(ch, FUCK_PTHIEF) || affected_by_spell(ch, FUCK_GTHIEF)) { 
	act( "$n is turned away from the clan hall.", ch, 0, 0, TO_ROOM , 0);
	send_to_char("The clan guard says 'Hey don't be bringing trouble around here!'\n\r", ch );
	return eSUCCESS;
    }
    else if(IS_AFFECTED(ch, AFF_CHAMPION)) {
       act( "$n is turned away from the clan hall.", ch, 0, 0, TO_ROOM , 0);
       send_to_char("The clan guard says, 'Hey, don't be a wuss, get outta here.'\n\r", ch);
       return eSUCCESS;
    }
    return eFAILURE;
}

/*--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--*/
static char *dethSayText [ ] =
{
  "Can i lick your stamps?",
  "Want to buy a some cheap godload equipment?",
  "Gravity.  It's not just a good idea, it's the law.",
  "I'm hooked on phonics!",
  "I'm through with Sergio, he treats me like a ragdoll.",
  "How much Vaseline should i use the first time?",
  "Saaaweeet looking ass on that sanitation engineer!",
  "Want to see my hathead?",
  "Can I borrow a few coins, at least until i find another sucker...",
  "I don't think the hydrostatic law is REALLY true.",
  "I'm an Innie, you?",
  "I think I'll meta CON next.",
  "See ya later, I need to go buy some more pipeweed.",
  "Good God, such soft hands you have...touch me more.",
  "Is it in yet? I can't feel a thing.",
  "Why does my butt itch?",
  "I like to lick testicles. Do you have any?",
  "My doctor recommends Monistat 7.", 
  "All that grunting and groaning for one measly fart?!",
  "I'll give you a brownie point if you snuggle me.",
  "Shirt and shoes required? Hell, that's all I need?",
  "Hey, Why is someone poking me in the ribs?",
  "Gods! Here I come, I can Immort!",
  "You mean I can use a different piece of toilet paper?",
  "Mmmm smells great.  What'd you have for dinner?",
  "Can I clap the erasers on the bulletin board?",
  "I think this buffalo wing deal is a hoax, they taste like chicken.",
  "I've got the neatest alias, alias backtotown == rec recall.",
  "I've got the neatest alias, alias assholes == whoclan 1.",
  "Oooo a half-eaten nachos from Juan's just laying here on the ground.",
  "Damn this heat, it makes me shed.",
  "What the hell is that SMELL?!?",
  "DOH!  I keep losing my link when I clap.",
  "Anyone trade me two twenties for a forty?",
  "I'd join clan [Studs] but they keep giving me wedgies.",
  "Ooo bastard enfan scrapped my lantern.",
  "Quit killing me, I'm saving up for a new pair of plate leggings.",
  "I think I'll stroll on over and hang out with Wesley Crusher for a while.",  
  "There is something innately sexual about my bald head.",
  "How does a moron think up a punchline?",
  "I've got the Kleenex, I've got the Vaseline.  Party time!",
  "Michael Bolton ROCKS!!",
  "I particularly love Rush Limbaugh's pulpy eggy core.",
  "I can't wait to get that Van-Damme butt-clenching video!",
  "The arena has been opened! Type 'junk backpack' to enter the bloodbath!",
  "There is a fine line between fishing and standing on the shore looking like an idiot.",
  "My palms keep shedding!",
  "Newt Gingrich makes me all wet and sticky.",
  "Why are you mudding, and not out getting laid?",
  "Gods!  Can I get a transport back to town please?",
  "One time my penis got caught in a milking machine, best experience of my life.",
  "Imagine if birds could be tickled by feathers.",
  "Senate Democrats today conceded defeat in an initiative to tax Gravity.",
  "You must think I'm pretty stupid for smoking all that oregano.",
  "Michael Huffington is my hero.",
  "Now I have to type 'kil'?? FUCK this!",
  "The Republicans are in power?  I might have to get a job!...I hear they're hiring in Iraq.",
  "Anyone else here know how to breakdance?",
  "Hey Tro, how come you never return my calls anymore?",
  "Coke is IT!!",
  "Is it Monday? I can't miss Melrose Place.",
  "Hey look, I have a pubic hair.",
  "Hey wait, this is a FINGER rubber!",
  "You have to admit, there are touches of genius in DOS.",
  "Gods, I need a reimb.  I got killed.",
  "Where's the monks' guild?",
  "You'd be surprised how hard it is to put tab A into slot B.",
  "Anna Nicole Smith was one sick bitch.",
  "The Crawford/Gere split is final! Now's my chance!",
  "Is it 7 yet? I don't want to miss Judge Wapner.",
  "Waz, please don't zap me. I didn't think wiggle was a spammable action.",
  "Are there any Imps on?  I have some stupid questions.",
  "Does anyone know the address for Hidden Worlds?",
  "Kurt Cobain is dead.",
  "You must think I'm pretty stupid for snorting all that Aspirin.",
  "This whole godload thing really sucks.  I can't even kill a poodle without a Wavy Bladed Kris these days.",
  "Uh-oh, I've been flagged a spammer!",
  "I think that recalls are quite reasonably priced.",
  "How do I set up Tintin on my VAX account?",
  "My dog is my best friend.",
  "Where's the key to the graveyard?",
  "Gods, I'm transferring equipment. Please don't zap me!",
  "---------->>>>>***** L E V E L *****<<<<<----------",
  "Does anyone know the address for the Badlands?",
  "Is Sadus on?  Sabburo keeps harassing me. :(",
  "Please don't freeze me. I didn't know multiplaying is illegal here.",
  "I did it.  I ate the big white mint.",
  "I just don't know what to do now that Hidden Worlds is gone. :(",
  "Send me some cash and I'll set you up with an original Dagger of Death.",
  "Don't mess with me or I'll sick some copyright lawyers on you!",
  "Oh Dando, I love it when you lick my ear like that.  Do it again!",
  "I love being Morcallen's love slave.  He's so good to me.",
  "Somewhere in the world there is a fat woman having an IRC orgasm.", 
  "I may be annoying, but at least I don't say the same things over and over.",
  "Pirahna must be a girl, she smells like fish.",
  "I didn't inhale either.",
  "Eleyre is a fucking power hungry bitch!",
  "Gods help!  Someone keeps pingponging me!",
  "Boy, mages and barbs have just been fucked over...",
  "Where the hell did Phire go?",
  "I'm gonna become Uruk'hai so the Nazgul stop killing me.",
  "Can someone help me get Wintin to run on my Mac?",
  "I can never seem to rent a U-Haul in Oklahoma anymore.",
  "Quick!  Everyone buy stock in Netscape, I have a g00d feeling!",
  "Sadus, can I be a playtester for ++?",
  "Liriel wanted to sleep with me, but I told her only if she buys a strap-on...",
  "What state is Maryland in?",
  "Luna wants me.",
  "I heard Diana's driver was related to Ted Kennedy.",
  "Where's my silk stockings?  I got a date with Marv Albert tonight.",
  "My 7 year old sister can hold her liquor better than Anarchy...gross.",
  "This Godload SUCKS!",
  "Can I be an -=Umi_Helper=- too?",
  "FUCKING SHIT!",
  "Karma karma karma karma karma chameleon!",
  "If at first you don't succeed, skydiving is not for you.",
  "If you wake up in the morning, and the cat is licking your penis, and you don't knock it off, is that wrong?",
  "Is Apshai on?!  I wanted to know if I could quest!",
  "If your parents never had children, chances are you won't either.",
  "What do people mean when they say the computer went down on me?",
  "Did you hear that Captain Hook died from jock itch?",
  "Do infants enjoy infancy as much as adults enjoy adultery?",
  "If God dropped acid, would he see people?",
  "If love is blind, why is lingerie so popular?",
  "If the #2 pencil is the most popular, why is it still #2?",
  "If you try to fail, and succeed, which have you done?",
  "Why are hemorrhoids called 'hemorrhoids' instead of 'asteroids'?",
  "Why is the alphabet in that order? Is it because of that song?",
  "If most car accidents occur within five miles of home, why doesn't everyone just move 10 miles away?",
  "If man evolved from monkeys and apes, why do we still have monkeys and apes?",
  "The main reason Santa is so jolly is because he knows where all the bad girls live.",
  "If you're in a vehicle going the speed of light, what happens when you turn on the headlights?",
  "Should crematoriums give discounts for burn victims?",
  "This is my ass.  This is my ass on weeeeeeed dude!  Any questions?",
  "Hey, I tried that 7e10nee13nd place but I didn't find gnome village!  I died!",
  "Sex is alot like air...it's no big deal unless you aren't getting any.",
  "Would another name for pickled bread be dill-dough?",
  "Why do they report power outages on TV?",
  "Can you be a closet claustrophobic?",
  "When it rains, why don't sheep shrink?",
  "If a stealth bomber crashes in a forest, will it make a sound?",
  "Should vegetarians eat animal crackers?",
  "If a mute swears, does his mother wash his hands with soap?",
  "Isn't it a bit unnerving that doctors call what they do 'practice'?",
  "When you open a bag of cotton balls, is the top one meant to be thrown away?",
  "What WAS the best thing before sliced bread?",
  "What hair color do they put on the driver's licenses of bald men?",
  "If it's zero degrees outside today, and it's supposed to be twice as cold tomorrow, how cold is it going to be?",
  "Since Americans throw rice at weddings, do Asians throw hamburgers?",
  "Why is a carrot more orange than an orange?",
  "That was Zen, this is Tao.",
  "I am on a thirty day diet.  So far, I have lost 15 days.",
  "The problem with patting yourself on the back is that your hands aren't free to break your fall.",
  "Ladies, learn to work the toilet seat.  If it's up, put it down.",
  "Foreign films are best left to foreigners.",
  "If you smoke after sex, you're doing it too fast.",
  "So you're a feminist...Isn't that cute!",
  "Beauty is in the eye of the beer holder.",
  "Prevent inbreeding: ban country music.",
  "I killed a 6-pack just to watch it die.",
  "Virgin wool comes from ugly sheep.",
  "When Eskimos sit on the ice too long do they get Polaroids?",
  "I heard when Bill Clinton was asked his opinion on foreign affairs, he replied, 'I don't know. I never had one.'",
  "Oral sex may make your day, but anal sex makes your hole weak.",
  "The flour was too messy!",
  "Pirahna er en ful apa!",
  "What do you call a prostitute with a runny nose?     ...Full",
  "Do soya-beans and dildos both count as substitute meat?",
  "Never accept a drink from a urologist.",
  "The more you run over a dead cat, the flatter it gets.",
  "Creativity is great, but plagiarism is faster.",
  "Reality is a crutch for those who can't cope with fantasy.",
  "When you starve with a tiger, the tiger starves last.",
  "There are days when no matter which way you spit, it's upwind.",
  "Whatever it is that hits the fan, it will not be evenly distributed.",
  "Today's mighty oak is just yesterday's nut that held its ground.",
  "Clinton announced today that the new national bird is the spread eagle.",
  "Women in Washington DC were asked if they would have sex with the President.  86% said 'Not again.'",
  "The only reason Clinton is interested in the Gaza Strip is cause he thinks it's a topless bar.",
  "Luke, I am your father.",
  "The technical name for Viagra is Mycoxafailin",
  "Men are from earth.  Women are from earth.  Deal with it.",
  "Women like silent men, they think they're listening.",
  "Give a man a fish and he will eat for a day.  Teach him how to fish and he will sit in a boat and drink beer all day.",
  "Don't sweat the petty things, and don't pet the sweaty things.",
  "Always remember to pillage BEFORE you burn.",
  "To steal ideas from one person is plagiarism; to steal from many is research.",
  "The laws in this city are clearly racist.  All laws are racist.  The law of gravity is racist. - M. Barry, Mayor of Washington, DC",
  "Alcohol and calculus don't mix.  Never drink and derive.",
  "I'm so big and sexy!  GET IN MAH BELLY!!!",
  "Why is it called tourist season if we aren't allowed to shoot them?",
  "This mud sucks!  I'm going to go play on Nodeka!",
  "Raha!  When am I getting my 321 plats back?!?!?",  // - He stole them from Antijag
  "I fucked your mom...  No no, really, I fucked your mom.",
  "smoking.",
  "My aunt's a piece of ass.",
  "Fishing Rod.",
  "Would you rather be The Ghoul, or The Fucking-Fag?",
  "You're leaning on my Lexus.",
  "Kid rock SUCKS",
  "Quit fouling me!  What's all this aboot?  If you wanna play fuutball we can.",
  "HEY!  You in the water!  Stop having fun!",
  "Don't dare me to do something man, cause I'll do it.",
  "Those damn Eclipse are always trying to get me lucky charms.",
  "Hey Orion!  Make sure your mom comes to the next party...in Nausica's skirt!",
  "Everyone watch me jump off the balcony and shove this knee into my jaw!",
  "Oh, I'll get the volleyball.  I'll just vault the fence!",
  "Hey Cerotin, you're so good on thumbs, I have something else you could try.",
  "Budduda Budduda Budduda Clear!",
  "Seige is down!  Everyone tag base!",
  "If ignorance is bliss, why aren't more people happy?",
  "Hard work may pay off later, but laziness pays off now!",
  "If only women came with pull-down menus and on-line help.",
  "When blondes have more fun, do they know it?",
  "What happens if you get scared half to death twice?",
  "The original point and click interface was a Smith & Wesson.",
  "Four out of five people think the fifth is an idiot.",
  "Is reading in the bathroom considered Multi-Tasking?",
  "A hangover is the wrath of grapes.",
  "ENDLESS LOVE: Stevie Wonder and Ray Charles playing tennis",
  "A thing not worth doing isn't worth doing well.",
  "Sometimes too much to drink isn't enough.",
  "If you think there is good in everybody, you haven't met everybody.",
  "The faulty interface lies between the chair and the keyboard.",
  "Schizophrenia beats being alone.",
  "I'm a psychic amnesiac. I know in advance what I'll forget.",
  "Why do they lock gas station bathrooms?  Are they afraid someone will clean them?",
  "Why do people who know the least know it the loudest?",
  "A mouse is just an elephant built by the Japanese ",
  "Ground Beef: A Cow With No Legs",
  "Clones are people, two",
  "One good turn... Gets most of the blankets",
  "COLE'S LAW: Thinly sliced cabbage",
  "Editing is a rewording activity",
  "Rap is to music what Etch-a-Sketch is to art",
  "Never miss a good chance to shut up.",
  "How many of you believe in telekinesis?  Please raise my hand",
  "I was thinking that women should put pictures of missing husbands on beer cans.",
  "It compiled? The first screen came up? Ship it!  --  Bill Gates",
  "1024x768x256... Sounds like one mean woman!",
  "2B OR NOT 2B = FF",
  "9 out of 10 men, who tried camels, prefer women.",
  "A seminar on time travel will be held two weeks ago.",
  "Apathy Error: Don't bother striking any key.",
  "Bad: Your children are sexually active.  Worse: With each other.",
  "Ban the Bomb. Save the world for conventional warfare.",
  "Canadian DOS: Yer sure, eh? [Y,n]",
  "CAUTION! Do not look into laser with remaining eye.",
  "Coarse and violent nudity.  Occasional language.",
  "Cosmetics: preventing men from reading between the lines.",
  "Democracy: 3 wolves and a sheep voting on dinner.",
  "Don't drink water, fish fuck in it.",
  "Earth is shutting down in five minutes--please save all files and log out.",
  "Ever noticed how fast Windows runs? Me neither.",
  "Evolution is God's way of issuing upgrades.",
  "Happiness is 9,10-Didehydo-N,N-diethyl-6-methylergoline-8B-carboxamide.",
  "I used to miss my girlfriend, but my aim improved.",
  "Insert disk 5 of 4 and press any key to continue.",
  "It is not enough to succeed.  Others must fail.",
  "Let's grab some beer and dynamite and go fishing.",
  "Life: a sexually transmitted condition with 100% fatality.",
  "Never stand between a dog and a lamp post.",
  "Please return stewardess to original upright position.",
  "She has beautiful eyes... too bad she has three of them.",
  "The Earth is 98% full. Please delete anyone you can.",
  "The Ultimate Virus: A self installing copy of 'Win95'.",
  "2 + 2 = 5 for extremely large values of 2.",
  "Two Beatles down........two to go.  Muahahahahahahaha!",
  "Stress is when you wake up screaming and you realize you haven't fallen asleep yet.",
  "I refuse to star in your psychodrama.",
  "I once had a dyslexic girl friend in Idaho until she wrote me a John Deere letter.",
  "Some nights I stay up wondering if illiterate people get the full effect of Alphabet soup.",
  "Support bacteria - they're the only culture some people have.",
  "Depression is merely anger without enthusiasm.",
  "GHB - The Quicker Pick Her Upper",
  "Ambition is a poor excuse for not having enough sense to be lazy.",
  "Dancing is a perpendicular expression of a horizontal desire.",
  "If at first you don't succeed, destroy all evidence that you tried.",
  "The severity of the itch is proportional to the reach.",
  "You never really learn to swear until you learn to drive.",
  "The sooner you fall behind, the more time you'll have to catch up.",
  "Change is inevitable....except from vending machines.",
  "Drugs may lead to nowhere, but at least it's the scenic route.",
  "A big mountain of sugar is too much for one man. I can see now why God portions it out in those little packets.",
  "If a pig loses its voice, is it disgruntled?",
  "Do Roman paramedics refer to IV's as 4's?",
  "Are people more violently opposed to fur rather than leather because it's much easier to harass rich women than motorcycle gangs?",
  "If you take an Oriental person and spin them around several times, do they become disoriented?",
  "Brake fluid mixed with Clorox makes smoke, and lots of it!",
  "A rose by any other name would stick you just as bad and draw just as much blood when you grab a thorn.",
  "Strangers are friends you haven't bled for an easy twenty yet.",
  "If I wanted to hear the pitter-patter of little feet, I'd put shoes on my cat.",
  "If you don't like my driving, don't call anyone.  Just take another road.  That's why the highway department made so many of them.",
  "If genius is one percent inspiration and 99 percent perspiration, I wind up sharing elevators with a lot of bright people.",
  "Men are like buses. They have spare tires and smell funny.",
  "What do you do when you see an endangered animal eating an endangered plant?",
  "Would a fly without wings be called a walk?",
  "If a turtle doesn't have a shell, is she homeless or naked?",
  "How do they get the deer to cross at that yellow road sign?",
  "Before they invented drawing boards, what did they go back to?",
  "If you and I were squirrels, could I bust a nut in your hole?",
  "I'd like to wrap your legs around my head and wear you like a feed bag.",
  "Is that a keg in your pants?  'Cause I would love to tap that ass!",
  "How about we play lion and lion tamer?  You hold your mouth open, and I'll give you the meat.",
  "Wanna play Pearl Harbor?....Its a game where I lay back while you blow the hell out of me.",
  "When a man steals your wife, there is no better revenge than to let him keep her.",
  "Early bird gets the worm, but the second mouse gets the cheese.",
  "If Barbie is so popular, why do you have to buy her friends?",
  "When everything's coming your way, you're in the wrong lane.",
  "Many people quit looking for work when they find a job.",
  "When I'm not in my right mind, my left mind gets pretty crowded.",
  "Why do psychics have to ask you for your name?",
  "Ahhh... I see the screw-up fairy has visited us again.",
  "I don't know what your problem is, but I'll bet it's hard to pronounce.",
  "How about never? Is never good for you?",
  "I see you've set aside this special time to humiliate yourself in public.",
  "I'll try being nicer if you'll try being smarter.",
  "I don't work here. I'm a consultant.",
  "I like you. You remind me of when I was young and stupid.",
  "You are validating my inherent mistrust of strangers.",
  "What am I? Flypaper for freaks!?",
  "I'm not being rude. You're just insignificant.",
  "Avoid parking tickets by leaving your windshield wipers turned to fast wipe whenever you leave your car parked illegally.",
  "Old telephone books make ideal personal address books.  Simply cross out the names and addresses of people you don't know.",
  "As I said before, I never repeat myself.",
  "Drink until she's cute, but stop before the wedding.",
  "I'm not cheap, but I am on special this week.",
  "Don't hit a man with glasses.....use your fist.",
  "I drive way too fast to worry about cholesterol.",
  "The only substitute for good manners is fast reflexes.",
  "Give a man a free hand and he'll run it all over you.",
  "In Florida it is illegal to jog with your eyes closed.",
  "In Fairbanks, Alaska it is illegal to give beer to a moose.",
  "Impotence: Nature's way of saying no hard feelings.",
  "A good scapegoat is nearly as welcome as a solution to the problem.",
  "As I let go of my feelings of guilt, I can get in touch with my Inner Sociopath.",
  "I have the power to channel my imagination into ever-soaring levels of suspicion and paranoia.",
  "Joan of Arc heard voices too.",
  "Just one letter makes all the difference between here and there.",
  "If time heals all wounds, how come the belly button stays the same?",
  "A conscience is what hurts when all your other parts feel so good.",
  "For every action, there is an equal and opposite criticism.",
  "If you must choose between two evils, pick the one you've never tried before.",
  "Half the people you know are below average.",
  "42.7 percent of all statistics are made up on the spot.",
  "'The escape pod is not an option.' 'Why not?' 'It escaped last Thursday.'",
  "Australian kissing: like French kissing, but in the land down under.",
  "Why are there 5 syllables in the word 'monosyllabic'?",
  "Why is there only one Monopolies commission?",
  "How come you press harder on a remote-control when you know the battery is dead?",
  "One nice thing about egotists:  They don't talk about other people.",
  "I doubt, therefore I might be.",
  "I didn't climb to the top of the food chain to be a vegetarian.",
  "College is just one big party, with a 25,000 dollar cover charge.",
  "My friend has kleptomania, but when it gets bad, he takes something for it.",
  "Never be afraid to try something new.  Remember amateurs built the ark - professionals built the Titanic.",
  "Love is grand - divorce is a hundred grand.",
  "Politicians and diapers have one thing in common, they should both be changed regularly and for the same reason.",
  "Time may be a great healer, but it's also a lousy beautician.",
  "Age doesn't always bring wisdom, sometimes age comes alone.",
  "Life not only begins at forty, it begins to show.",
  "Bigamy is having one wife too many. Monogamy is the same.  -Oscar Wilde",
  "There's so much comedy on television. Does that cause comedy in the streets?",
  "Writing about music is like dancing about architecture.",
  "The dumber people think you are, the more surprised they're going to be when you kill them. -William Clayton",
  "I'm no glass of milk but i can still do your body good.",
  "If you drink, don't park. Accidents cause people.",
  "Don't worry. It only seems kinky the first time.",
  "Don't squat with spurs on.",
  "Anything worth taking seriously is worth making fun of.",
  "Before you criticize someone, you should walk a mile in their shoes. That way, when you criticize them, you're a mile away and you have their shoes.",
  "We are born naked, wet, and hungry. Then things get worse.",
  "I almost had a psychic girlfriend but she left me before we met.",
  "Would a vacuum that really sucked be a good deal?",
  "If you lose your right nut, would your left nut still be your left nut?  Joker?",
  "Hi, will you help me find my lost puppy? I think he went into this cheap motel across the street.",
  "Hi guys.  *slurp slurp*  This is my *slurp slurp slurp* 'friend.'",
  "Hey Bauglir, if you don't put so much cock in your mouth, you won't have a problem with choking your chicken.",
  "Just keep the asses away from me!  What's this all aboot?!",
  "Hey guys, what's this?  ...  I dunno, but here it comes again! ...",
  "Here's a little song I wrote, might want to sing it note for note, don't worry, be happy.",
  "Take me to the river...Throw me in the water!",
  "Check me out!  I drank 4 bottles of wine!  *urp*  I guess that's a net of 2 now....",
  "Okay....what's this 'Pin the tail on the Phish' shit?",
  "Kill whitey!",
  "Hey Gamera, didn't you say you were tired?  Btw, where's your 'friend' and what're all these new claw marks on your back?",
  "Dammit Wasp, where's MY mug?!?",
  "You think that horse is impressive?  Let me show you something...",
  "Baby, I'm an American Express lover...you shouldn't go home without me!",
  "The two most common elements in the universe are hydrogen and stupidity.",
  "Psychiatrists say that 1 of 4 people is mentally ill.  Check three friends.  If they're OK, you're it.",
  "It has recently been discovered that research causes cancer in rats.",
  "The average woman would rather have beauty than brains because the average man can see better than he can think.",
  "Clothes make the man.  Naked people have little or no influence on society.",
  "In the pinball game of life, some people's flippers are a little further apart than others.",
  "After eating, do turtles have to wait one hour before getting out of the water?",
  "If white wine goes with fish, do white grapes go with sushi?",
  "If someone has a mid-life crisis while playing hide and seek, does he automatically lose because he can't find himself?", 
  "Instead of talking to your plants, if you yelled at them would they still grow, but only to be troubled and insecure?",
  "When your pet bird sees you reading the newspaper, does he wonder why you're just sitting there, staring at carpeting?",
  "There are three kinds of people in this world; those who can count and those who can't.",
  "For people who like peace and quiet: a phoneless cord.",
  "Always proofread carefully to see if you any words out.",
  "I wish I had a Kryptonite cross, because then you could keep both Dracula AND Superman away.",
  "I hope if dogs ever take over the world and choose a king that they don't just go by size because I bet there are some Chihuahuas with some good ideas.",
  "When you go in for a job interview, I think a good thing to ask is if they press charges.",
  "Do people in Australia call the rest of the world 'up over'?",
  "Why doesn't Tarzan have a beard?",
  "Why is it that when you're driving and looking for an address, you turn down the volume on the radio?",
  "How do you write zero in Roman numerals?",
  "If a jogger runs at the speed of sound, can he still hear his Walkman?",
  "If blind people wear dark glasses, why don't deaf people wear earmuffs?",
  "Why do we sing 'Take Me Out To the Ballgame' when we are already there?",
  "I've learned that you cannot make someone love you. All you can do is stalk them and hope they panic and give in.",
  "Hey implementors, do you need any help coding?  I can code!",
  "Captain America could SO beat Captain Canada's ass.",
  "How much deeper would oceans be if sponges didn't live there?",
  "If it's true that we are here to help others, then what exactly are the others here for?",
  "No one ever says 'It's only a game' when their team is winning.",
  "Ever wonder what the speed of lightning would be if it didn't zigzag?",
  "When cheese gets its picture taken, what does it say?",
  "Why are a wise man and a wise guy opposites?",
  "Why do overlook and oversee mean opposite things?",
  "Why do bankruptcy lawyers expect to be paid? ",
  "I'd kill for a Nobel Peace prize.",
  "Chastity is curable if detected early.",
  "Love may be blind, but marriage is a real eye-opener.",
  "Borrow money from pessimists -- they don't expect it back.",
  "Everyone has a photographic memory, some just don't have film.",
  "A good pun is its own reword.",
  "A bartender is just a pharmacist with a limited inventory.",
  "Show me the thunder-stick!",
  "If we aren't supposed to eat animals, why are they made of meat?",
  "Daddy, why doesn't this magnet pick up this floppy disk?",
  "I'd give my right arm to be ambidextrous.",
  "Karaoke is Japanese for 'tone deaf'.",
  "An unemployed court jester is no one's fool.",
  "Any closet is a walk-in closet if you try hard enough.",
  "As long as I can remember, I've had amnesia.", 
  "Clairvoyants meeting canceled due to unforeseen events.",
  "Don't be a sexist, broads hate that.",
  "Honk if you love peace and quiet.",
  "Energizer Bunny Arrested! Charged with battery.",
  "I bet you I could stop gambling.",
  "A Unix user once said, 'rm -rf /'  And since then everything has been null and void.",
  "All your base are belong to us.",
  "You have no chance to survive make your time.",
  "Someone set up us the bomb.",
  "Sometimes I like to rub icy hot over my entire body, and sit naked fondling myself as I watch exercise videos.",
  "Oh, it's just a RAM upgrade...I can back up those files later.",
  "Why do they call it common sense if so few people seem to have it?",
  "Oh whatever, MUDing won't affect my GPA.  I can handle it.",
  "I've been throwing this lettuce in the air for an hour now...when does it start feeling good?",
  "Do not sully the great name of Mike Ditka.",
  "Did you ever notice when you put the 2 words 'The' and 'IRS' together it spells 'THEIRS'?",
  "The sole purpose of a child's middle name is so he can tell when he's really in trouble.",
  "The older you get, the tougher it is to lose weight, because by then your body and your fat are really good friends.",
  "The real art of conversation is not only to say the right thing at the right time, but also to leave unsaid the wrong thing at the tempting moment.",
  "Don't assume malice for what stupidity can explain.",
  "If you can't be kind, at least have the decency to be vague.",
  "When I'm feeling down I like to whistle. It makes the neighbor's dog run to the end of his chain and gag himself.",
  "Birds of a feather flock together and crap on your car.",
  "A diplomat is one who can bring home the bacon without spilling the beans.",
  "A fine is a tax for doing wrong.  A tax is a fine for doing well.",
  "A first grade teacher is one who knows how to make little things count.",
  "A smart man is one who convinces his wife she looks fat in a fur coat.",
  "Bachelor: a man who wouldn't take \"Yes\" for an answer.",
  "Bowling tends to get kids off the streets and into the alleys.",
  "Confession may be good for the soul, but it's often bad for the reputation.",
  "Despite inflation, a penny is still a fair price for too many people's thoughts.",
  "Don't marry for money -- you can borrow it more cheaply.",
  "Egotism is not one of the better virtues, but is does lubricate the wheels of life.",
  "Everybody should learn to drive, especially those who sit behind the wheel.",
  "Everything in the modern home is controlled by switches except the children.",
  "Fun is like life insurance; the older you get the more it costs.",
  "He who turns the other cheek too far gets it in the neck.",
  "Horsepower was a lot safer when only horses had it.",
  "If all cars in the U.S. were placed end to end, some fool would try to pass them.",
  "To get away with criticising, leave the person with the idea he has been helped.",
  "If you want to forget all your other troubles, wear tight shoes.",
  "Intuition enables a woman to contradict her husband before he says anything.",
  "Many men are self-made, but only the successful ones will admit it.",
  "Need a helping hand?  Try the end of your arm.",
  "Opportunity merely knocks -- temptation kicks the door in.",
  "People who get discovered and those who get found out are very different.",
  "Regardless of what happens, somebody always claims they knew it would.",
  "People go into debt trying to keep up with people who are already there.",
  "Tact is the ability to describe others as they see themselves.",
  "The best safety device in a car is a rear view mirror with a policeman in it.",
  "The clothes that keep a man looking his best are often worn by girls.",
  "Up to sixteen, a lad is a Boy Scout.  After that he is a girl scout.",
  "You are only young once, but you can stay immature indefinitely.",
  "You'll get along with the boss if he goes his way and you go his.",
  "A friend of mine confused her valium with her birth control pills. She had 14 kids, but she doesn't care.",
  "A polar bear is a rectangular bear after a coordinate transformation.",
  "Acting is all about honesty. If you can fake that, you've got it made. - George Burns",
  "After four decimal places, nobody gives a damn.",
  "How come light beer weighs the same as regular beer?",
  "When people run around and around in circles we say they are crazy.  When planets do it we say they are orbiting.",
  "When they broke open molecules, they found they were only stuffed with atoms.  But when they broke open atoms, they found them stuffed with explosions.",
  "Lime is a green-tasting rock.",
  "Many dead animals of the past turned into fossils while others preferred to be oil.",
  "Genetics explain why you look like your father and if you don't why you should.",
  "We keep track of the humidity in the air so we won't drown when we breathe.",
  "Rain is often spoken of as soft water, oppositely known as hail.",
  "A blizzard is when it snows sideways.",
  "It is so hot in some parts of the world that the people there have to live other places.",
  "The wind is like the air, only pushier.",
  "Nothing says 'I love you' like oral sex in the morning.",
  "The White Man?    No good.",
  "There's 10 kinds of people in this world, those who understand binary and those who don't.",
  "I think other prisoners' penises are a big pain in the ass.",
  "Vote Quimby.",
  "It's time to trade in my 26 year old for two 13s.",
  "Nothing says luvin like a tasty porkchop.",
  "Hey guys, ya know...it tastes just like salty tapioca.  No no, really...it does.",
  "Pirahna is such a loser with women.  Skanky hoes flee in terror from him.",
  "Outside a dog, a book is a man's best friend.  Inside a dog, it's too dark to read.",
  "Give a person a fish and you feed them for a day. Teach that person to use the internet and they won't bother you for weeks.",
  "Some people are like Slinkies...not really good for anything, but you can't help but smile when you see one tumble down the stairs.",
  "I read recipes the same way I read science fiction. I get to the end and I think, 'Well, that's never going to happen!'",
  "Doctors can be frustrating. You wait a month-and-a-half for an appointment, and he says, 'I wish you would've come to me sooner.'",
  "Have you ever noticed that a slight tax increase costs you two hundred dollars, and a substantial tax cut saves you thirty cents?",
  "In the 60's people took acid to make the world weird. Now the world is weird and people take Prozac to make it normal.",
  "Politics is supposed to be the second oldest profession. I have come to realize it bears a very close resemblance to the first.",
  "How is it one careless match can start a forest fire, but it takes a whole box to start a campfire?",
  "Marriage is like taking a hot bath.  After you've been in for a while...it isn't so hot.",
  "If you're playing a poker game and you look around the table and can't tell who the sucker is, guess what....it's you!",
  "Whenever I feel blue, I start breathing again.",
  "Is it because light travels faster than sound that some people appear bright until you hear them speak?",
  "Wtf...ever since I downloaded the 23k winXPfulleditioncrack.exe file my computer's been acting weird.",
  "Random number generation is too important to be left to chance.",
  "Why did Apoc get fired from the sperm bank?  He got caught drinking on the job.",
  "Sex is alot like Tang.  Just not powdery.  Or orange.",
  "MentalNote:  www.manpages.com is NOT an online reference for UNIX help files.",
  "Tip:  If you're in another country just bite the bullet and pretend you're Canadian.  It'll be alot easier on you.",
  "I swear.  Next time my mom approaches me about underage drinking I'm going to F'ing come out of the closet.",
  "60% of girls first kiss is with another girl.",
  "I think Dildo Daggins would be a great hobbit porno name.",
  "mmMmmMmmm synthetic horse rectum.",
  "I can't figure out how to login to the MUD.",
  "I'm going to become rich and famous after I invent a device that allows you to stab people in the face over the internet.",
  "I'm not saying there should be a capital punishment for stupidity, but why don't we just take the safety labels off everything and let the problem solve itself?",
  "Whoa cool!  When you gossip your password, it auto-replaces it with ******.  ****** see?  Try it.",
  "Girls are like Internet domain names.  The ones I like are already taken.  But you can still get one like it from a strange country.",
  "I beat the Internet.  That last boss is hard.",
  "What the fuck is wtf?",
  "Guy:  67% of girls are stupid.   Blonde Girl:  I belong with the other 13%.",
  "I used to hate weddings.  All the grandmas would poke me and go 'You're next.'  They stopped when I started doing it at funerals.",
  "IRC is just multiplayer notepad.",
  "Our f@th3r, wh0 0wnz heaven, j00 r0ck! May @11 0ur base som3day b310ng 2 u! 4m3n.",
  "Trojan Condoms?  Wasn't the Trojan horse just a trick to get men inside so they could burst out and go everywhere?",
  "Roses are red, violets are blue.  All of my base, are belong to you.",
  "The 'bishop' came to my church Sunday.  What am impostor!  He never once moved diagonally.",
  "A turtle's weakness is being on its back.  So if you tape 2 together they are unstoppable.",
  "I tried to change my Hotmail password to 'penis' but it said my password was too small.",
  "Real life needs a freaking search function.  I can never find my damn carkeys.",
  "Tetris is so unrealistic.",
  "Moo spelled backwards is moo.  No wait.",
  "My granddad died in a concentration camp.  Fell out of a guard tower.  Broke his neck.",
  "Make sure cat is not sleeping in bass drum before you begin playing them.",
  "Warning:  Do not let Dr. Mario touch your genitals.  He is NOT a real doctor.",
  "General rule about female mudders seems to be:  Attractive, Single, Mentally Stable.  Choose 2.",
  "The most secure computer in the world is one not connected to the Internet.  That's why I recommend AOL Broadband.",
  "In a perfect world spammers would be locked up with men that have enlarged their penis, taken Viagra, and are looking for new relationships.",
  "I'm going to name my kids Control, Alt, and Delete so I can just hit them all at once to solve any acting up.",
  "Does anyone here have a computer?",
  "Girls always act all old and mature until you fuck them in the ass and they're like 'STOP IM ONLY THIRTEEN, IM ONLY THIRTEEN!'",
  "Sorry Mario but the Princess is on another MUD.",
  "I'm annoyed that they can get 11KBps from mars but can't get me a stable 5KBps over 17 miles.",
  "If my calculations are correct SLINKY + ESCALATOR = EVERLASTING FUN!",
  "Mark Duval of Belgium is an idiot.",
  "Muhahaha!  I infected this guy at 127.0.0.1 with Sub7 and I'm messing with him now.",
  "How many Vietnam Vets does it take to screw in a lightbulb?  You don't KNOW MAN, cause you WEREN'T THERE.",
  "This girl I know said she doesn't give head and I'm like...they still make those?",
  "I'm not on 'AOL'. I'm on America Online.  Retard.",
  "You know something's wrong when you have dreams about eating pudding and you wake up with a spoon in your ass.",
  "Guy1: So you're in MENSA huh?  That's only 2% of the population.  Guy2: Actually it's more like 1 in 50.",
  "Hey Medicinae, do you know any girls whose names don't contain .jpeg, .mpg, or .avi?",
  "Wouldn't it be great if they renamed Scotland - Scatland.",
  "All these years I played sports before I realized you can just buy trophies at the store.",
  "Isn't American cheese appropriately named? It's fake and processed, just like America.",
  "Women are like hurricanes. When they come, they're wet and wild, but when they go, they take your house and car.",
  "Even 'The Magic 8 Ball' is smart and knows Microsoft Sucks.  It says 'Outlook not good.'",
  "Life really is like a box of chocolate.  A cheap meaningless gift that no one ever asks for.",
  "My anus is itchy and I can't scratch it because then my hand would smell like ass.",
  "There's a difference between being grumpy and hating every little bastard in existence.",
  "Hello Kitty is one cat I'd like to violate.",
  "Damn needy players. They should be happy with 2600 baud and dual wield.",
  "Philosophy SUCKS!",
  "Dyslexics of the world, UNTIE!",
  "I love Swedes and Canadians...my puny American ways are insignificant when compared to the power of Meatballs and Moose!",
  "I got fired from work trying to find this amulet of Yendor, and I finally have it. Now what?",
  "People who gives blowjobs are fucked in the head.",
  "What's the difference between Michael Jackson and acne? Acne waits till you're 13 to come on your face.",
  "I put racing strips on my computer.  Makes me feel like it goes faster.",
  "What happened to sex, drugs, and rock'n'roll.  Now we have AIDS, crack, and techno.",
  "It's a good thing Microsoft isn't in the condom business.",
  "Whenever I clean my glasses, my monitor becomes dirty as hell.",
  "Okay so this baby seal walks into a club....",
  "There's two types of women on the Internet.  .avi and .mpg",
  "It's sad when Perl is the closest thing you have to a relationship with a woman.",
  "Lucifer is sooo 20th century.  The Devil should rename himself something like 1337Ki11@r.",
  "What does God do when a kitten masturbates?",
  "This is great...some guy in Nigeria wants to transfer me a huge sum of money if I help him out!",
  "The truth, like my penis, sometimes slips out at inopportune times.",
  "I wonder if anyone ever calls Steve Jobs 'Mac Daddy'.",
  "Ever get the urge to call the operator and say 'I need an exit fast!'",
  "The thing that attracts me to fat chicks the most is probably the gravitational pull.",
    // Sneaky repeating of messages to make the fuckers vote.
  "I've lost 3 girlfriends to voting for Dark Castle on Mudconnect(through www.dcastle.info), but it was worth it.",
  "Good: Your wife meets you naked at the door. Bad: She's coming home.",
  "Good: Your boyfriend's exercising. Bad: So he'll fit in your clothes.",
  "What do you have when 100 lawyers are buried up to their neck in sand?  Not enough sand",
  "When a girl says \"No\" she actually means \"Yes\", but not with you.",
  "Quidquid latine dictum sit, altum viditur.",
    // whatever is said in Latin sounds profound
  "To be sure of hitting the target, shoot first and, whatever you hit, call it the target.",
  "Your lucky number is 7399928377275452622483. Look for it everywhere!",
  "Love is like a snowmobile racing across the tundra and then suddenly it flips over, pinning you underneath. At night, the ice weasels come.",
  "Don't worry about people stealing your ideas. If your ideas are any good, you'll have to ram them down people's throats.",
  "Avoid hangovers. Stay drunk!",
  "If money doesn't grow on trees then why do banks have branches?",
  "How important does a person have to be before they are considered assassinated instead of murdered?",
  "If electricity comes from electrons, does morality come from morons?",
  "How is it that we put a man on the moon before we figured out it would be a good idea to put wheels on luggage?",
  "Why is it that people say they 'slept like a baby' when babies wake up every two hours?",
  "Why do you have to \"put your two cents in\" but it's only a \"penny for your thoughts\"?  Where's that extra penny going to?",
  "Why do people pay to go up tall buildings and then put money in binoculars to look at things on the ground?",
  "Why do toasters always have a setting that burns the toast to a horrible crisp, which no decent human being would eat?",
  "Can a Hearse carrying a corpse drive in the carpool lane?",
  "Why do people point to their wrist when asking for the time, but don't point to their crotch when they ask where the bathroom is?",
  "If Wyle E. Coyote had enough money to buy all that ACME crap, why didn't he just buy dinner?",
  "Did you ever notice that when you blow in a dog's face, he gets mad at you, but when you take him for a car ride, he sticks his face out of the window?",
  "Apocalypse is the kind of man who understands, that when you put another man's cock in your mouth, you make a pact.",
  "Chuck Norris' genes aren't DNA. They're barbed wire.",
  "Since 1940, the year Chuck Norris was born, roundhouse kick related deaths have increased 13,000 percent.",
  "This mud sucks. I'm going to steal the code and make my own!",
  "Hmm, I'm not so sure that was meatball sauce I saw Urizen licking off his fingers.",
  "Of all the Daliesque tourist traps in the world, we had to walk into this one.",
  "Ahh yes, the good old days of the Internet...when men were men, women were men, and children were FBI agents.",
  "I think Bush's foreign policy is quite reasonable.",
  "Remember, arguing on the Internet is like entering the Special Olympics...even if you win, you're still a retard.",
  "Sorry guys, I have to step down as Implementor so I can pass my PhD.",
  "Chuck Norris doesn't teabag the ladies, he potato sacks them!",
  "Chuck Norris' tears could cure cancer except he's never cried.",
  "Sheep are great and all, but goats have handlebars and rabbits are handheld.",
  "Don't worry guys, I'll Imm more and shave when I'm done \"finding myself\"...until then I'm going to roam the land like in Kung Fu.",
  "The only hellstream I have ever seen is when I pee after sex with your mom.",
  "Wow, I really should have checked that this Dixie-cup wasn't Tainted before I drank from it!",
  "I know! I'm going to steal a MUD call it my own...People will trust me then for sure!",
  "Hey Zizou!  Yo Momma!",
  "Hey gods, tell Parry to stop trying to shoot his pointy thing into me!",
  "Sorry guys, I can't build right now...I have to work on my Psychology degree, it's very serious.",
  "Dark Castle MUD is like a box of chocolates.....thrown into a room full of starving psychotic teenagers with weapons.",
  "Wow guys, you shoulda seen this goat's balls, they were HUGE!  I couldn't stop droo...err stari...err yeah they were huge!",
  "So what's the speed of dark?",
  "A conclusion is the place where you got tired of thinking.",
  "If you think nobody cares about you, try missing a couple of payments.",
  "Someone sent me a postcard picture of the earth.  On the back it said, \"Wish you were here.\"",
  "I spilled spot remover on my dog.  Now he's gone. :(",
  "Cross country skiing is great if you live in a small country.",
  "What's another word for Thesaurus?",
  "You can't have everything...where would you put it?",
  "A lot of people are afraid of heights.  Not me, I'm afraid of widths.",
  "If you can wave a fan, and you can wave a club, can you wave a fan club?",
  "Smoking cures weight problems...eventually.",
  "Is \"tired old cliche\" one?",
  "I Xeroxed a mirror. Now I have an extra Xerox machine.",
  "You know the guy who writes all those bumper stickers?  He hates New York.",
  "If a word in the dictionary were misspelled, how would we know?",
  "When I was a little kid we had a sand box. It was a quicksand box. I was an only child....eventually.",
  "For my birthday I got a humidifier and a de-humidifier so I put them in the same room and let them fight it out.",
  "She criticized my apartment, so I knocked her flat.",
  "If you jumped off the bridge in Paris and swam in the river that goes through the city people would say you were insane.",
  "Two fonts walk into a bar.  The barman says to them, \"Get out. We don't serve your type here.\"",
  "Hey guys, do you mind if I add a whole bunch of dumb jokes to my material?",
  "Time is a great healer. Unless you've got a rash, then you're better off with ointment.",
  "The brain is soft and gelatinous - its consistency is something between jelly and cooked pasta (and goes well with a nice Chianti).",
  "The word clitoris is from the ancient Greek word kleitoris, meaning \"little hill\"...now if only I could find it. :(",
  "Hexakosioihexekontahexaphobiacs is the term for people who fear the number 666.  I'm scared of pi.",
  "A mind is a terrible thing to waste; I'm glad they didn't waste one on you.",
  "If idiots could fly this place would be an airport.",
  "How do I set a laser printer to stun?",
  "I will always cherish the initial misconceptions I had about your being competent.",
  "Too many freaks, not enough circuses.",
  "I have plenty of talent and vision. I just don't give a fuck.",
  "I like cats too. Let's exchange recipes.",
  "If I throw a stick, will you go away?", 
  "Those of you who think you know everything are very annoying to those of us who do.",
  "It's not that I wish any harm to the Dingo, I'm just saying I could happily sit by while someone beheads him.",
  "It is just you.",
  "Pack the bags, we're going on a guilt trip.",
  "For an enhanced playing experience, take red pill now.",
  "Lottery: A tax on people who are bad at math.",
  "Reality? That's where the pizza delivery guy comes from!",
  "Shit! Jesus is coming, everyone look busy.",
  "Erotic is using a feather. Kinky is using the whole chicken.",
  "Your ridiculous little opinion has been noted.",
  "We have enough youth, how about a fountain of SMART?",
  "Wait a second, did you rip off your own arm as a joke?", // Futurama > you
  "42",
  "I'm so embarrassed. I wish everybody else was dead.",
  "Hey Taint, I have lots of hit points too! Lets measure penises!",
  "Hey you!  Yeah, you!  Go donate!",
  "These aren't the stats you're looking for...",
  "dr00dz r 1337 h4x0r5!",
  "Schni Schna Schnappi.  Schnappi Schnappi Schnapp!",
  "Goooooooooooooooooooooaaaaaaaaaaaaaaaaaalllllllllllllllllllll!!!!!!!!!",
  "cArEbEaR sTaRe!",
  "That's hot.",
  "Wanna fuck? *puts on robe and wizard hat*",
  "Note to self: Salmon and Marguaritas just don't mix.",
  "If this is Canada, where the hell are all the Moose?!? - Feb. 4, 2007.",
  "God shouldn't be the only one allowed to play God.", // Steve Jobs said that, interestingly enough
  "A good icebreaker is to ask people what gender they are.",
  "Have you been beating off?  There's no Honolulu.",  // 2007 Tahoe ski trip
  "It looks like English, but I can't understand a damn word you're saying.",
  "I thought I wanted a career; turns out I just wanted a salary.",
  "Thank you. We're all refreshed and challenged by your unique point of view.",
  "She offered her honor. He honored her offer.  And all night long, honor, offer, honor, offer.",
  "I find pink poodles highly offensive. Please remove them from the game.",
  "You have insulted my honour and beliefs...I challenge you to a DUEL!",
  "Stromboli keeps telling offensive jokes. :( Can't you gods do something?",
  "I remember my first blowjob...I did so well the pastor told me I could skip church the next week!",
  "The Trinity? Or Jesus multiplaying?",
  "If I ever had twins, I'd use one for parts.",
  "Unattended newbies will be sold to Madonna.",
  "God? I have an imaginary friend too. His name is Bob.",
  "Fuck it. I'm moving to 581 c.",
  "Let Moose and Mountie unite!",
  "WARNING: the Imm staff is not responsible for Stromboli's \"activities\" should you make personal data available to other players.",
  "Build a man a fire and you warm him for a day.  Set a man on fire and you warm him for the rest of his life.",
  "Urizen LOVES the runka.",
  "I'm an agnostic dyslexic insomniac.  I can't sleep at night because I'm wondering if there's a Dog.",
  "Real coders don't document...if it was hard to write, it should be hard to understand.",
  "Math illiteracy affects 8 out of every 5 people.",
  "Despite the rising cost of living, have you noticed how it still remains so popular?",
  "I'm busy now. Can I ignore you some other time?",
  "I love deadlines...especially the sound they make as they go whooshing by!",
  "Urizen told me that even though hard work never killed anyone, he doesn't want to chance it.",
  "Tell me what you need and I'll tell you how to get along with out it.",
  "Don't worry. I'm from the Internet.",
  "God, root, what is difference?", // Written like that on purpose
  "You will never steal anything again while I'm not AFK.",
  "People call me Hadoken because I'm down-right-fierce.",
  "I wish my lawn was Emo so it would cut itself.",
  "DON'T TAZE ME BRO!",
  "Online Rage: That's Entertaintment!",
  "I just ate a Dr. Scholl's gel insert and I don't feel any better. :(",
  "Mark my words, wireless is a flash in the pan...wires rock!",
  "You can't take it with you.  Unless its less than 100ml and in a transparent ziplocked bag.",
  "Surrounding yourself with geekier people will not make you any less of a geek.",
  "I have delusions of mediocrity.",
  "I never faked it with you.  Okay, one time.",
  "The first step is not admitting you have a problem -- the first step is acquiring a problem.",
  "The time is now!  Anything you start right now will succeed! ...too late.",
  "My God has a bigger dick than your God."
 };

// ENDOFCHAIN

#define DETH_SAY_TEXT_SIZE    ( sizeof ( dethSayText )    / sizeof ( char * ) )

int deth (struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    int x;

    if(IS_AFFECTED(ch, AFF_PARALYSIS))
      return eFAILURE;

    if(cmd)
      return eFAILURE;

    x = number ( 0, DETH_SAY_TEXT_SIZE * 120 );

    if ((unsigned) x < DETH_SAY_TEXT_SIZE) {
	do_gossip ( ch, dethSayText [ x ], 0 );
	return eSUCCESS;
    }
    return eFAILURE;
}
/*--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--*/

int fido(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct obj_data *i, *temp, *next_obj, *deep, *next_deep;

    if (cmd || !AWAKE(ch))
	return eFAILURE;

    if(IS_AFFECTED(ch, AFF_CHARM))
        return eFAILURE;

    for (i = world[ch->in_room].contents; i; i = i->next_content) 
    {
	if (GET_ITEM_TYPE(i)==ITEM_CONTAINER && i->obj_flags.value[3]) 
        {
	    act("$n savagely devours a corpse.", ch, 0, 0, TO_ROOM, 0);
	    for(temp = i->contains; temp; temp=next_obj)
	    {
		next_obj = temp->next_content;
                // don't trade no_trade items
                if(IS_SET(temp->obj_flags.more_flags, ITEM_NO_TRADE))
                {
                  extract_obj(temp);
                  continue;
                }
                // take care of any no-trades inside the item
                for(deep = temp->contains; deep; deep = next_deep)
                {
                  next_deep = deep->next_content;
                  if(IS_SET(deep->obj_flags.more_flags, ITEM_NO_TRADE))
                    extract_obj(deep);                   
                }
		move_obj(temp, ch->in_room);
	    }
	    extract_obj(i);
	    return eSUCCESS;
	}
    }
    return eFAILURE;
}

int janitor(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct obj_data *i;

    if (cmd || !AWAKE(ch))
	return eFAILURE;

    if(IS_AFFECTED(ch, AFF_CHARM))
        return eFAILURE;

    for (i = world[ch->in_room].contents; i; i = i->next_content) 
    {
        if (IS_SET(i->obj_flags.wear_flags, ITEM_TAKE) &&
            GET_OBJ_WEIGHT(i) < 20 &&
	    !IS_SET(i->obj_flags.extra_flags, ITEM_SPECIAL) &&
	    obj_index[i->item_number].virt != CHAMPION_ITEM)
        {
            act("$n picks up some trash.", ch, 0, 0, TO_ROOM, 0);
            move_obj(i, ch);
            return eSUCCESS;
	}
    }
    return eFAILURE;
}


int mother_moat_and_moad(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  struct char_data *temp, *tmp_victim;
  struct affected_type af;
  int dam;
  int retval;

  if(cmd)
    return eFAILURE;
    
  if(GET_POS(ch)==POSITION_FIGHTING)
    return eFAILURE;
  
  if(!ch->mobdata->hatred)
    return eFAILURE;
  
  if(ch->fighting)
    return eFAILURE;

// TODO - make this Crydragon proc into something better    
  for(tmp_victim = world[ch->in_room].people; tmp_victim; tmp_victim = temp)
  {
     temp = tmp_victim->next_in_room;
     if((IS_NPC(tmp_victim) || IS_NPC(ch)) && (tmp_victim != ch)) { 
        send_to_char("You choke and gag as noxious gases fill the air.\n\r", 
                      tmp_victim);
        dam = dice(GET_LEVEL(ch), 8);
        act("$n floods the surroundings with poisonous gas.", ch, 0, 0,
             TO_ROOM, 0);
        retval = damage(ch, tmp_victim, dam, TYPE_POISON, SPELL_GAS_BREATH, 0);
        if(IS_SET(retval, eCH_DIED))
          return retval;
        if(!affected_by_spell(tmp_victim, SPELL_POISON))
         if(!IS_SET(tmp_victim->immune, ISR_POISON))
         {
           af.type = SPELL_POISON;
           af.duration = GET_LEVEL(ch)*2;
           af.modifier = -5;
           af.location = APPLY_STR;
           af.bitvector = AFF_POISON;

         affect_join(tmp_victim, &af, FALSE, FALSE);
         send_to_char("You feel very sick.\n\r", tmp_victim);
         return eSUCCESS;
         }
      }
    }
    return eFAILURE;
}        
       
int adept(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  struct char_data *tch;

  if (cmd || !AWAKE(ch))
    return eFAILURE;
  
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    if(!IS_NPC(tch) && number (0,2) == 1 && CAN_SEE(ch,tch))
      break;
  
  if (!tch)
    return eFAILURE;
  
  switch (number (0,10))
    {
    case 3 :
      act("$n utters the words 'garf'.", ch, 0, 0, TO_ROOM, 0);
      cast_cure_light(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, tch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    case 7 :
      act("$n utters the words 'nahk'.",  ch, 0, 0, TO_ROOM, 0);
      cast_bless(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, tch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    case 6 :
      act("$n utters the words 'tehctah'.",  ch, 0, 0, TO_ROOM, 0);
      cast_armor(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, tch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    case 4 :
      do_say(ch,"Finish school.  Don't drop out.", 0);
      return eSUCCESS;
    case 5 :
      do_say(ch,"Move it.  Others want to go to this school.", 0);
      return eSUCCESS;
    default:
      return eFAILURE;
    }
}

/*--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--*/

int mud_school_adept(struct char_data *ch,struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  struct char_data *tch;

  if (cmd)
    return eFAILURE;

  if (!AWAKE(ch))
    return eFAILURE;
  
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    if(!IS_NPC(tch) && number (0,2) == 1 && CAN_SEE(ch,tch))
      break;

  if (!tch)
    return eFAILURE;
  
/* .... to be finished ...
"What are you staring at?",
"Hi, I'm your friend.",
"Isn't this a fine school?",
"Finish school.  Dont drop out.",
"Move it.  Others want to go to this school.",
"Be careful, don't get killed by those monsters!",
"Don't forget to wear your clothes, young students!",
"What are you doing just STANDING there?!?!!?",
"GET going!",
"Hello....Hello, are you listening to me?",
*/

  switch (number(0,20))
    {
    case 15 :
      do_say(ch,"What are you staring at?", 0);
      break;
    case 18 :
      do_say(ch,"Hi, I'm your friend.", 0);
      break;
    case 3 :
      do_say(ch,"Isn't this a fine school?", 0);
      break;
    case 12 :
      do_say(ch,"Finish school.  Dont drop out.", 0);
      break;
    case 5 :
      do_say(ch,"Move it.  Others want to go to this school.", 0);
      break;
    case 6 :
      do_say(ch,"Be careful, don't get killed by those monsters!", 0);
      break;
    case 7 :
      do_say(ch,"Don't forget to wear your clothes, young students!", 0);
      break;
    case 8 :
      do_say(ch,"What are you doing just STANDING there?!?!!?", 0);
      do_say(ch,"GET going!", 0);
      break;
    case 9 :
      do_say(ch,"Hello....Hello, are you listening to me?", 0);
      break;
    default:
      return eFAILURE;
    }
  return eSUCCESS;  
}


int bee(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   if(cmd) return eFAILURE;
   if(GET_POS(ch)!=POSITION_FIGHTING) return eFAILURE;
    
    if ( ch->fighting && 
	(ch->fighting->in_room == ch->in_room) &&
	number(0 , 120) < 2 * GET_LEVEL(ch) )
	{
	    act("You sting $N!",  ch, 0, ch->fighting, TO_CHAR, 
	      INVIS_NULL);
	    act("$n stings at $N with its barbed stinger!", ch, 0,
              ch->fighting, TO_ROOM, INVIS_NULL|NOTVICT);
	    act("$n sinks a barbed stinger into you!", ch, 0,
              ch->fighting, TO_VICT, 0);
	    return cast_poison( GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL,
		 ch->fighting, 0, GET_LEVEL(ch));
	}
    return eFAILURE;
}

static char *apiary_workerEmoteText [] = {
"screams in sheer terror as he looks out the window.",
"screams, SCREAMS, SCREEEEEEEEAAAAAAAMS!!!!",
"shrieks in utter terror!",
"flails his arms wildly and SCREAMS aloud in desperation.",
"begins to cry for his mommy and wets himself.",
"wimpers quietly in the corner.",
"quivers, shakes, shudders, and finally SCCCCRRRREEEEAAAMMMMMSSSS!!!",
"says, 'That son-of-a-bitch stung me on the ASS! Look at this!'",
"screams, 'We .. are .. all .. going .. to .. DDDDIIIIIEEE!!!!!'",
"screams, 'Oh God, Oh God I think I went my pants... MOMMY!!!!'",
"shakes you as he screams, 'Well!  You're a hero!  Do something!'",
"says, 'Ok.  Just kill me quick, I cant take this anymore'",
"screams, 'That Bee has a FATT BUTT!'"
};

#define APIARY_WORKER_EMOTE_TEXT_SIZE (sizeof(apiary_workerEmoteText) / sizeof(char *) )

int apiary_worker (struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   int x;

   if(IS_AFFECTED(ch,AFF_PARALYSIS))
     return eFAILURE;

   if(cmd)
      return eFAILURE;

   x = number (0,APIARY_WORKER_EMOTE_TEXT_SIZE*25);

   if((unsigned) x < APIARY_WORKER_EMOTE_TEXT_SIZE) {
      do_emote(ch,apiary_workerEmoteText [x],0);
      return eSUCCESS;
    }
      return eFAILURE;
}



/*********************************************************************
*  Special procedures for shops                                      *
*********************************************************************/

int pet_shops(struct char_data *ch, int cmd, char *arg)
{
    char buf[MAX_STRING_LENGTH], pet_name[256];
    int pet_room;
    struct char_data *pet;

    pet_room = ch->in_room+1;

    if (cmd==59) { /* List */
	send_to_char("Available pets are:\n\r", ch);
	for(pet = world[pet_room].people; pet; pet = pet->next_in_room) {
	    sprintf(buf, "%8lld - %s\n\r",
		3*GET_EXP(pet), pet->short_desc);
	    send_to_char(buf, ch);
	}
	return eSUCCESS;
    } else if (cmd==56) { /* Buy */

	arg = one_argument(arg, buf);
	arg = one_argument(arg, pet_name);

	if (!(pet = get_char_room(buf, pet_room))) {
	    send_to_char("There is no such pet!\n\r", ch);
	    return eSUCCESS;
	}

	if (GET_GOLD(ch) < (uint32)(GET_EXP(pet)*3)) {
	    send_to_char("You don't have enough gold!\n\r", ch);
	    return eSUCCESS;
	}
         if (many_charms(ch)) {
         send_to_char("How you plan on feeding all your pets?", ch);
           return eSUCCESS;
           }


	GET_GOLD(ch) -= GET_EXP(pet)*3;

	/*
	 * Should be some code here to defend against weird monsters
	 * getting loaded into the pet shop back room.  -- Furey
	 */
	pet = clone_mobile(pet->mobdata->nr);
	GET_EXP(pet) = 0;
	SETBIT(pet->affected_by, AFF_CHARM);

      /* people were using this to steal plats from people transing in meta */
	if ( /* *pet_name */  0) {
	    sprintf(buf,"%s %s", pet->name, pet_name);
	    pet->name = str_hsh(buf);     

	    sprintf(buf, "%sA small sign on a chain around the neck "
                    "says 'My Name is %s'\n\r",
	      pet->description, pet_name);
	    pet->description = str_hsh(buf);
	}

	char_to_room(pet, ch->in_room);
	add_follower(pet, ch, 0);

	/* Be certain that pet's can't get/carry/use/weild/wear items */
	IS_CARRYING_W(pet) = 1000;
	IS_CARRYING_N(pet) = 100;

	send_to_char("May you enjoy your pet.\n\r", ch);
	act("$n bought $N as a pet.",ch,0,pet,TO_ROOM, 0);

	return eSUCCESS;
    }

    /* All commands except list and buy */
    return eFAILURE;
}

int newbie_zone_guard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if (cmd>6 || cmd<1)
	return eFAILURE;

    if(IS_NPC(ch))
        return eFAILURE;

    if ( ( GET_LEVEL(ch) > 10 /* mud school */
	&& ch->in_room == real_room(257) && cmd == 1 ) /* north */
        || 
         ( GET_LEVEL(ch) > 20 /* newbie caves */
        && ch->in_room == real_room(6400) && cmd == 6 ) /* down */
	)

    {
      if(ch->in_room == real_room(257)) /* mud school */
      {
	act( "The guard refuses $n entrance to this sacred school.",
	    ch, 0, 0, TO_ROOM , 0);
	send_to_char(
	    "The guard refuses you entrance to the school.\n\r", ch );
	return eSUCCESS;
      }
      else if(ch->in_room == real_room(6400)) /* newbie caves */
      {
	act( "The guard stops $n from entering the caves.",
	    ch, 0, 0, TO_ROOM , 0);
	send_to_char(
	    "The guard refuses you entrance to the caves.\n\r", ch );
	return eSUCCESS;
      }
      else /* default */
      {
	act( "The guard humiliates $n, and blocks $s way.",
	    ch, 0, 0, TO_ROOM , 0);
	send_to_char(
	    "The guard humiliates you, and blocks your way.\n\r", ch );
	return eSUCCESS;
      } 
    } 


    return eFAILURE;

}


// I just like to hellstream every other round.
int hellstreamer(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    CHAR_DATA *vict;
    // int percent; 
    /* Find a dude to do evil things upon ! */

    if((GET_POS(ch) != POSITION_FIGHTING)) {
        return eFAILURE;
    }

    vict = ch->fighting;

    if (!vict)
	return eFAILURE;

    if(IS_AFFECTED(ch, AFF_BLIND)) {
      act("$n utters the words 'I see said the blind!'.", ch, 0, 0, TO_ROOM,
	INVIS_NULL);
       cast_remove_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
          return eSUCCESS;
      }


    // removed && GET_LEVEL(ch) > 49
    if(number(0,2)==0 )
    {
	act("$n utters the words 'Burn motherfucker!'.", ch, 0, 0, 
	  TO_ROOM, INVIS_NULL);
	return cast_hellstream(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }

    return eFAILURE;
}

// I just firestorm every round...stupid groupies!
int firestormer(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    CHAR_DATA *vict = NULL;
    // int percent; 

    act("$n utters the words 'Fry bitch!'.", ch, 0, 0, 
	  TO_ROOM, INVIS_NULL);
    return cast_firestorm(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
}

int humaneater(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *tch;

    if (cmd || !AWAKE(ch))
	return eFAILURE;

     if (ch->fighting) {
         return eSUCCESS;
            }

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
        {

        if (!IS_NPC(tch))
           if (CAN_SEE(ch, tch)) {

              if(!can_attack(ch) || !can_be_attacked(ch, tch))
                continue;
	   
              if (!IS_MOB(tch) && IS_SET(tch->pcdata->toggles, PLR_NOHASSLE))
                continue;

              if (IS_AFFECTED(tch, AFF_PROTECT_EVIL))
                if (IS_EVIL(ch) && (GET_LEVEL(ch) <= GET_LEVEL(tch)))
                  continue;
              if (affected_by_spell(tch, SPELL_PROTECT_FROM_GOOD))
                if (IS_GOOD(ch) && (GET_LEVEL(ch) <= GET_LEVEL(tch)))
                  continue;

              if (GET_RACE(tch) != RACE_HUMAN)
                  continue;

              do_say(ch, "Ahh...more humans have come to feed our hunger!!\r\n", 0);
              return attack(ch, tch, TYPE_UNDEFINED);
          }
    }
          return eFAILURE;
}

static char *pir_slutSayText [] = {
  "Oh baby yes, do it to me!",
  "It's so BIG!",
  "Yes HARDER!",
  "Shoot it all over my face!",
  "Spread me like butter baby!",
  "*slurp* *slurp*",
  "Do my E cups excite you?",
  "Where's Sati?  I want some REAL muff diving!",
  "YES!  POWERTOOLS!",
  "Whoa!  THAT'S not the right hole!",
  "Don't worry, that rash isn't contagious.",
  "Pirahna comes like the Titanic.",
  "Put it between my tits!",
  "Don't worry, I like having that stuff in my eyes.",
  "What's that old condom doing up there?",
  "Eat me like a giant bearded clam!",
  "Let me suck the filling out of that twinkie.",
  "Sometimes when I masturbate while watching my dog lick itself, I think of big hairy men.",
  "Ram me like a piledriver you big stud!",
  "You want to watch me stick WHAT up there?",
  "Take me from behind!",
  "Bite my ass!",
  "Go ahead, put your face between them.",
  "Let me call a few of my girlfriends over.  I can't handle you by myself.",
  "Suck on my toes!",
  "Rub it on my nipples!",
  "That stuff is so hard to get out of your hair:(",
  "Squeeze my nipples harder!",
  "I brought my strap-on if you're into that stuff *wink*",
  "Have you ever heard the phrase 'Fisting'?",
  "Spank me daddy, I've been a bad girl!",
  "You're my daddy!  You're my daddy!",
  "That thing's bigger than a horse's!",
  "Don't worry, I never bite *wink* *meow*",
  "Don't worry, we can try again in a few minutes.  That happens to lots of men.",
  "Let me go slip into something more comfortable...like leather.",
  "Would you like to help me shave it?",
  "Wanna play 'hide the pingpong ball?'",
  "Stick that italian hogie right in my salad!",
  "Don't mind the scraping feeling...that's just some scabbing.",
  "I haven't been fucked like that since grade school.",
  "You know....there IS a minimum size requirement to ride on this ride....",
  "I'm not into farm animals... Donkey's don't count as farm animals.",
  "...and one time, at warrior camp, I stuck a halberd up my armor.",
  "My ass is like butter.",
  "You can eat at my hairy Taco Bell.",
  "Nothing like the delicious man chowder of a hot beef injection."
};

#define PIR_SAY_TEXT_SIZE ( sizeof(pir_slutSayText) / sizeof(char*))

int pir_slut (struct char_data*ch, struct obj_data *obj, int cmd, char*arg,        
          struct char_data *owner)
{
   int x;
   if (cmd)
     return eFAILURE;
   if (IS_AFFECTED(ch,AFF_PARALYSIS))
     return eFAILURE;
   x = number (0,PIR_SAY_TEXT_SIZE * 6);
   if((unsigned) x < PIR_SAY_TEXT_SIZE) {
     do_say(ch, pir_slutSayText [ x ], 9);
     return eSUCCESS;
   }
   return eFAILURE;
}


int clutchdrone_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    /*struct obj_data *obj;*/
    // struct obj_data *wielded;
    struct char_data *vict;
  // int dam;
    int retval;

    if(cmd)                                return eFAILURE;
    if (GET_POS(ch) < POSITION_FIGHTING)   return eFAILURE;
    if(GET_POS(ch)!=POSITION_FIGHTING)     return eFAILURE;
    if(!ch->fighting)             return eFAILURE;
    if (IS_AFFECTED(ch, AFF_PARALYSIS))    return eFAILURE;

    vict = ch->fighting;

    if (!vict)
	return eFAILURE;

    if(GET_LEVEL(ch)>3 && number(0,3)==0 
        && GET_POS(vict) > POSITION_SITTING)
    {
       retval = damage(ch, vict, 1,TYPE_HIT, SKILL_BASH, 0);
       if(IS_SET(retval, eCH_DIED))
	  return retval;

       act("Your bash at $N sends $M sprawling.", ch, NULL, vict, TO_CHAR , 0);
        act("$n sends you sprawling.",  ch, NULL, vict, TO_VICT , 0);
        act("$n sends $N sprawling with a powerful bash.", ch, NULL, vict, TO_ROOM, NOTVICT); 

       GET_POS(vict) = POSITION_SITTING;
      SET_BIT(vict->combat, COMBAT_BASH1);
           WAIT_STATE(vict, PULSE_VIOLENCE *2);

	return eSUCCESS;
    }
   if (vict==ch->fighting && GET_LEVEL(ch)>2 && number(0,1)==0 )
    {
       return damage (ch, vict, GET_LEVEL(ch)>>1, TYPE_HIT, SKILL_KICK, 0);
      }

   return eFAILURE;
}


int generic_guard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if (cmd>6 || cmd<1)
	return eFAILURE;

      /* lathander's zone */
      if((ch->in_room == real_room(20700) && cmd == 2))
      {
	act( "A statue magically holds $n back.",
	    ch, 0, 0, TO_ROOM , 0);
	send_to_char(
	    "A statue magically holds you from going east.\n\r", ch );
	return eSUCCESS;
      }

    return eFAILURE;
}

int portal_guard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    char buf[200];

    one_argument(arg, buf);

    if (cmd != 60)
	return eFAILURE;

      /* lathander's zone */
      if((ch->in_room == real_room(20720) && !str_cmp(buf, "door"))
        )
      {
	act( "Dense vegetation blocks $n's path through the door.",
	    ch, 0, 0, TO_ROOM , 0);
	send_to_char(
	    "There is too much vegetation in the way to get through.\n\r", ch );
	return eSUCCESS;
      }

    return eFAILURE;
}


int blindingparrot(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   if(cmd) return eFAILURE;
    if(GET_POS(ch)!=POSITION_FIGHTING) return eFAILURE;
    
    if ( ch->fighting && 
	(ch->fighting->in_room == ch->in_room) &&
	number(0 , 120) < 2 * GET_LEVEL(ch) )
	{
	    act("You peck $N!",  ch, 0, ch->fighting, TO_CHAR, 
	      INVIS_NULL);
	    act("$n pecks at $N with its beak!", ch, 0,
              ch->fighting, TO_ROOM, INVIS_NULL|NOTVICT);
	    act("$n pecks at you with it's beak!", ch, 0,
              ch->fighting, TO_VICT, 0);
	    return cast_blindness( GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL,
		 ch->fighting, 0, GET_LEVEL(ch));
	}
    return eFAILURE;
}

int doorcloser (struct char_data*ch, struct obj_data *obj, int cmd, char*arg,        
          struct char_data *owner)
{
   // int x;

   if (IS_AFFECTED(ch,AFF_PARALYSIS))  return eFAILURE;
   if (cmd)  return eFAILURE;

   /* but not every time */
   if(number(0, 1))
     return eFAILURE;

   if((EXIT(ch, 1) && !IS_SET(EXIT(ch, 1)->exit_info, EX_CLOSED)) ||
      (EXIT(ch, 3) && !IS_SET(EXIT(ch, 3)->exit_info, EX_CLOSED))
     )
   {
     if(number(0,1))
       do_say(ch, "How the hell do these doors keep opening?", 9);
     else do_say(ch, "I coulda sworn I just closed this....", 9);

     do_close(ch, "cell e", 9);
     do_close(ch, "cell w", 9);

     return eSUCCESS;
   }
   return eFAILURE;
}


int panicprisoner (struct char_data*ch, struct obj_data *obj, int cmd, char*arg,        
          struct char_data *owner)
{
   // int x;
   struct char_data *vict = NULL;

   if (cmd)  return eFAILURE;
   if (IS_AFFECTED(ch,AFF_PARALYSIS))  return eFAILURE;

   /* check for a guard */
   if((vict = get_char_room_vis(ch, "guard")))
   {
     if(number(0, 1))
       do_say(ch, "Run!  It's the fuzz!", 9);
     else do_say(ch, "Uh oh, guard.  I'm off like a prom dress!", 9);
     do_flee(ch, "", 0);
     return eSUCCESS;
   }   

   /* open any closed cells */
   /* but not every time */
   if(number(0, 2))
     return eFAILURE;

   if((EXIT(ch, 1) && IS_SET(EXIT(ch, 1)->exit_info, EX_CLOSED)) ||
      (EXIT(ch, 3) && IS_SET(EXIT(ch, 3)->exit_info, EX_CLOSED))
     )
   {
     if(number(0,1))
       do_say(ch, "I must free my fellow prisoners!", 9);
     else do_say(ch, "Viva la resistance!", 9);

     do_open(ch, "cell e", 9);
     do_open(ch, "cell w", 9);

     return eSUCCESS;
   }
   return eFAILURE;
}


// let's teleport people around the mud:)
int bounder(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    CHAR_DATA *vict;
    // int percent; 

    /* Find a dude to do evil things upon ! */

    if((GET_POS(ch) != POSITION_FIGHTING)) {
        return eFAILURE;
    }
    vict = ch->fighting;

    if (!vict)
	return eFAILURE;

    if(IS_AFFECTED(ch, AFF_BLIND)) {
      act("$n utters the words 'I see said the blind!'.", ch, 0, 0, TO_ROOM,
	INVIS_NULL);
       cast_remove_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
          return eSUCCESS;
      }

   do_say(ch, "I hope you land in enfan hell!", 9);
   act("$n recites a bound scroll.", ch, 0, vict, TO_ROOM,
	  INVIS_NULL);
   return cast_teleport(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
}

// I love to dispel stuff!
int dispelguy(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    CHAR_DATA *vict;
    // int percent; 

    if((GET_POS(ch) != POSITION_FIGHTING)) {
        return eFAILURE;
    }

    vict = ch->fighting;
    if (!vict)
	return eFAILURE;

    if(IS_AFFECTED(ch, AFF_BLIND)) {
      act("$n utters the words 'I see said the blind!'.", ch, 0, 0, TO_ROOM,
	INVIS_NULL);
       cast_remove_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
          return eSUCCESS;
      }

    if ((IS_AFFECTED(vict, AFF_SANCTUARY))  ||
        (IS_AFFECTED(vict, AFF_FIRESHIELD))  ||
        (IS_AFFECTED(vict, AFF_HASTE))) 
      {
         act("$n utters the words 'fjern magi'.", ch, 0, 0,
            TO_ROOM, INVIS_NULL);
         return cast_dispel_magic(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
      }
      else
      {
         act("$n utters the words 'frys din nisse'.", ch, 0, 0,
            TO_ROOM, INVIS_NULL);
         return cast_chill_touch(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
      }
    return eFAILURE;
}

int marauder(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    /*struct obj_data *obj;*/
    struct obj_data *wielded;
    struct char_data *vict;

    if(cmd) return eFAILURE;

    if (GET_POS(ch) < POSITION_FIGHTING) return eFAILURE;
    if(GET_POS(ch)!=POSITION_FIGHTING) return eFAILURE;
    if(!ch->fighting) return eFAILURE;
    if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;

    vict = ch->fighting;
    if (!vict)
	return eFAILURE;

    // Reinforcements!
    call_for_help_in_room(ch, 22701);

    wielded = vict->equipment[WIELD];

    if (ch->equipment[WIELD] && vict->equipment[WIELD])
    if (!IS_NPC(ch) || !IS_NPC(vict))
    if ((!IS_SET(wielded->obj_flags.extra_flags ,ITEM_NODROP)) &&
          (GET_LEVEL(vict) <= MAX_MORTAL ))
    if( vict==ch->fighting && GET_LEVEL(ch)>9 && number(0,2)==0 )
    {
        disarm(ch,vict);
	return eSUCCESS;
    }

    if( vict==ch->fighting && GET_LEVEL(ch)>3 && number(0,2)==0 )
    {
        return do_bash(ch, "", 9);
    }
    if (vict==ch->fighting && GET_LEVEL(ch)>2 && number(0,1)==0 )
    {
       return do_kick(ch, "", 9);
    }

   return eFAILURE;
}


int foggy_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data * mob = NULL;

    if(cmd)
      return eFAILURE;

    if(GET_POS(ch)!=POSITION_FIGHTING)
       return eFAILURE;

    if(!ch->fighting)
       return eFAILURE;

    // do this 50% of the time
    if(number(0, 1))
       return eFAILURE;

    act("$n glows in power and summons a horde of spirits to $s aid!",
             ch, 0, 0, TO_ROOM, INVIS_NULL );

    // create the mob
    mob = clone_mobile(real_mobile( 22026 ));
    if(!mob)
    {
       log("Foggy combat mobile missing", ANGEL, LOG_BUG);
       return eFAILURE;
    }
    // put it in the room ch is in
    char_to_room(mob, ch->in_room);

    // make it attack the person ch is
    act("$n issues the order 'destroy'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    attack( mob, ch->fighting, TYPE_UNDEFINED );
    // ignore if it died or not
    return eSUCCESS;
}

int foggy_non(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    // n s e s u d are commands 1-6.  For anything but that, ignore
    // the command and return
    if (cmd>6 || cmd<1)
	return eFAILURE;

    // message the player
    act( "The foggy guardian flows in front of $n, and blocks $s way.",
	    ch, 0, 0, TO_ROOM , 0);
    send_to_char("The foggy guardian flows in front of you, and blocks your way.\n\r", ch );
   
    // return true.  This lets the mud know that you already took care of
    // the command, and to ignore whatever it was.  (ie, don't move)
    return eSUCCESS;
}

int iasenko_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data * vict = NULL;
    int dam = 0;
    int retval;
    //char buf[200];

    if(cmd)
      return eFAILURE;

    retval = protect(ch, 8543); // rescue Koban if he's fighting
    if(IS_SET(retval, eSUCCESS) || IS_SET(retval, eCH_DIED))
      return retval;

    switch(number(1, 3)) {

    case 1:
    // hitall
     act("$n begins to twitch as the fury of his ancestors takes form!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     act("$n starts swinging like a MADMAN!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     dam = number(6, 800);
     damage_all_players_in_room(ch, dam);
     act("$n starts swinging like a MADMAN!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     dam = number(6, 800);
     damage_all_players_in_room(ch, dam);
    break;

    case 2:
    // earthquake
     act("$n utters the words, 'kao naga chi'", ch, 0, 0, TO_ROOM, INVIS_NULL);
     act("$n makes the earth tremble and shiver.\r\nYou fall, and hit yourself!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     dam = number(6, 400);
     damage_all_players_in_room(ch, dam);
     act("$n makes the earth tremble and shiver.\r\nYou fall, and hit yourself!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     dam = number(6, 400);
     damage_all_players_in_room(ch, dam);
    break;

    case 3:
    // redirect
    act("$n's eyes glaze over as he swings into a great fury!", ch, 0, 0, TO_ROOM, INVIS_NULL);
    if((vict = find_random_player_in_room(ch)))
    {
      stop_fighting(ch);
      return attack(ch, vict, TYPE_UNDEFINED);
    }
    break;

    } // end of switch

    return eSUCCESS;
}


int iasenko_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if(cmd)
      return eFAILURE;

    return protect(ch, 8543); // rescue Koban if he's fighting
}

int koban_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data * iasenko = NULL;
    struct char_data * temp_chr = NULL;

    if(cmd)
      return eFAILURE;

    iasenko = find_mob_in_room(ch, 8542);

    // re-sanct Iasenko if his sanct is down
    if(iasenko && !IS_AFFECTED(iasenko, AFF_SANCTUARY))
    {
      // stop koban from fighting so the sanct goes off, then start again
      temp_chr = ch->fighting;
      stop_fighting(ch);
      act("$n utters the words, 'gao kimo nachi'", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_sanctuary(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, iasenko, 0, GET_LEVEL(ch));
      set_fighting(ch, temp_chr);
      return eSUCCESS;
    }

    // full-heal Iasenko if he's hurt
    if(iasenko && ((GET_HIT(iasenko)+5) < GET_MAX_HIT(iasenko)) && number(0, 1))
    {
      act("$n calls on the souls of his fallen ancestors!", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_full_heal(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, iasenko, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    // call lightning
    act("$n utters the words, 'kao naga chi'", ch, 0, 0, TO_ROOM, INVIS_NULL);
    return cast_call_lightning(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
}

int koban_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    char_data * iasenko = NULL;

    if(cmd)
      return eFAILURE;

    if(ch->fighting)
      return eFAILURE;

    iasenko = find_mob_in_room(ch, 8542);

    // re-sanct Iasenko if his sanct is down
    if(iasenko && !IS_AFFECTED(iasenko, AFF_SANCTUARY))
    {
      act("$n utters the words, 'gao kimo nachi'", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_sanctuary(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, iasenko, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    // full heal Iasenko if he's hurt
    if(iasenko && ((GET_HIT(iasenko)+5) < GET_MAX_HIT(iasenko)))
    {
      act("$n calls on the souls of his fallen ancestors!", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_full_heal(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, iasenko, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    // re-sanct myself if my sanct is down
    if(!IS_AFFECTED(ch, AFF_SANCTUARY))
    {
      act("$n utters the words, 'gao kimo nachi'", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_sanctuary(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    // full-heal myself if i'm hurt
    if((GET_HIT(ch) + 5) < GET_MAX_HIT(ch))
    {
      act("$n calls on the souls of his fallen ancestors!", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_full_heal(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    return eFAILURE;
}

int kogiro_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data * vict = NULL;
    int dam = 0;
    int retval;

    if(cmd)
      return eFAILURE;

    retval = protect(ch, 8605); // rescue Takahashi if he's fighting
    if(SOMEONE_DIED(retval) || IS_SET(retval, eSUCCESS))
      return retval;

    switch(number(1, 3)) {

      case 1:
      // backstab random pc in room
      act("$n dives into the waters, buying him precious time to select a target...", ch, 0, 0, TO_ROOM, INVIS_NULL);
      vict = find_random_player_in_room(ch);
      stop_fighting(vict);
      stop_fighting(ch);
      return do_backstab(ch, GET_NAME(vict), 9);
      break;

      case 2:
      // room punch
      act("$n starts swinging like a MADMAN!", ch, 0, 0, TO_ROOM, INVIS_NULL);
      dam = number(6, 800);
      damage_all_players_in_room(ch, dam);
      break;

      case 3:
      // summon all mobs 8606
      act("$n utters the words 'gaisi ni gochi!'", ch, 0, 0, TO_ROOM, INVIS_NULL);
      if(! find_mob_in_room(ch, 8606) )
         summon_all_of_mob_to_room(ch, 8606);
      break;
    }

    return eSUCCESS;
}

int takahashi_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    int retval;

    if(cmd)
      return eFAILURE;
    
    switch(number(1, 2)) {
    
    case 1:
    // firestorm
     act("$n summons the power of the shadows to envelop you in fire!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     retval = cast_firestorm(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
     if(SOMEONE_DIED(retval))
       return retval;
     return cast_firestorm(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
     break;
    
    case 2:
    // vampiric touch
     act("$n calls upon the arcane knowledge of his ancestors!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     retval = cast_vampiric_touch(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
     if(SOMEONE_DIED(retval))
       return retval;
     return cast_vampiric_touch(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
     break;
    
    } // end of switch
    
    return eSUCCESS;
}

int askari_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    int dam;
    int retval;

    if(cmd)
      return eFAILURE;

    retval = protect(ch, 8646); // rescue Surimoto if he's fighting
    if(SOMEONE_DIED(retval) || IS_SET(retval, eSUCCESS))
      return retval;

    switch(number(1, 2)) {

    case 1:
    // hitall
     act("$n sizes you up, then swings with deadly acuracy!!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     act("$n starts swinging like a MADMAN!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     dam = number(6, 800);
     damage_all_players_in_room(ch, dam);
     act("$n starts swinging like a MADMAN!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     dam = number(6, 800);
     damage_all_players_in_room(ch, dam);
    break;

    case 2:
    // wanna be arrow damage
     act("$n takes aim with his mighty longbow...at YOU!!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     dam = number(300, 800);
     damage_all_players_in_room(ch, dam);
    break;

    } // end of switch

    return eSUCCESS;
}

int surimoto_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    int dam;

    if(cmd)
      return eFAILURE;

    switch(number(1, 8)) {

    case 1:
     act("$n utters the words, 'moshi-moshi'", ch, 0, 0, TO_ROOM, INVIS_NULL);
     cast_teleport(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
    break;

    default:
    // wanna be arrow damage
     act("$n takes aim with his mighty longbow...at YOU!!", ch, 0, 0, TO_ROOM, INVIS_NULL);
     dam = number(300, 800);
     damage_all_players_in_room(ch, dam);
    break;

    } // end of switch

    return eSUCCESS;
}     

int hiryushi_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    char_data * victim = NULL;

    if(cmd)
      return eFAILURE;

    switch(number(1, 3)) {

    case 1:
     act("$n utters the words, 'solar flare'", ch, 0, 0, TO_ROOM, INVIS_NULL);
     return cast_solar_gate(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
    break;

    case 2:
     act("$n utters the words, 'gasa ni umi'", ch, 0, 0, TO_ROOM, INVIS_NULL);
     return cast_hellstream(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
    break;

    case 3:
     act("$n leaps back and readies a wand...", ch, 0, 0, TO_ROOM, INVIS_NULL);
     for(victim = world[ch->in_room].people; victim; victim = victim->next_in_room)
     {
       if(IS_NPC(victim))
         continue;
       act("$n points a wand at $N.", ch, 0, victim, TO_ROOM, NOTVICT);
       return cast_drown(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, victim, 0, GET_LEVEL(ch));
     }
    break;

    } // end of switch

    return eSUCCESS;
}     

int izumi_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    int retval;
    if(cmd)
      return eFAILURE;

    switch(number(1, 7)) {

    case 1:
     act("$n utters the words, 'gasa ni umi'", ch, 0, 0, TO_ROOM, INVIS_NULL);
     retval = cast_poison(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
     if(SOMEONE_DIED(retval))
       return retval;
     cast_teleport(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
    break;

    default:
     act("$n utters the words, 'ga!'", ch, 0, 0, TO_ROOM, INVIS_NULL);
     return cast_colour_spray(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
    break;

    } // end of switch

    return eSUCCESS;
}     

int shogura_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if(cmd)
      return eFAILURE;

    if(GET_HIT(ch->fighting) < 5000)
    {
      do_say(ch, "It's time to finish this, little one.", 9);
      return ki_punch(GET_LEVEL(ch), ch, "", ch->fighting);
    }

    if(IS_AFFECTED(ch->fighting, AFF_FIRESHIELD) ||
       IS_AFFECTED(ch->fighting, AFF_FIRESHIELD) )
    {
      return ki_disrupt(GET_LEVEL(ch), ch, "", ch->fighting);
    }

    switch(number(1, 2)) {

      case 1:
      if(!number(0, 10))
      {
        act("$n summons up his iron will!", ch, 0, 0, TO_ROOM, INVIS_NULL);
        return do_quivering_palm(ch, "", 9); 
      }
      break;

      case 2:
      // summon all mobs 8668
      do_say(ch, "Multi-form technique!", 9);
      if(! find_mob_in_room(ch, 8668) )
         summon_all_of_mob_to_room(ch, 8668);
      break;
    }

    return eSUCCESS;
}     

// Proc for the arena mobs to make DAMN sure they stay in the arena.
int arena_only(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if(cmd || ch->fighting)
      return eFAILURE;

    if(!IS_SET(world[ch->in_room].room_flags, ARENA))
    {
      do_gossip(ch, "My life has no meaning outside of glorious arena combat!", 0);
      act("$n goes out in a blaze of glory!", ch, 0, 0, TO_ROOM, NOTVICT);
      // remove my eq
      for(int i = 0; i < MAX_WEAR; i++)  
        if(ch->equipment[i])
          obj_to_char(unequip_char(ch, i), ch);

      while(ch->carrying)
        extract_obj( ch->carrying );

      // get rid of me
      stop_fighting(ch);
      extract_char(ch, TRUE);
      // It's important we return true
      return eSUCCESS|eCH_DIED;
    }
    return eFAILURE;
}

int druid_elemental(struct char_data *ch, struct obj_data *obj,
 int cmd, char *arg, struct char_data *owner)
{
  if (cmd) return eFAILURE;
  if(!ch->master) {
    log("Elemental without a master.", IMMORTAL, LOG_BUG);
    extract_char(ch, TRUE);
    return (eCH_DIED | eSUCCESS);
  }
  if( GET_POS(ch) < POSITION_STANDING )
     return eFAILURE;
  if(!ch->fighting) 
  {
    if(ch->in_room != ch->master->in_room) {
      do_emote(ch, "creates an elemental gateway and steps through.\r\n", 9);
      move_char(ch, ch->master->in_room);
      act("An elemental gateway shimmers into existance and $n emerges.",ch, 0, 0, TO_ROOM, 0);
      return eSUCCESS;
    }
  }

  return eSUCCESS;
}


int mage_golem(struct char_data *ch, struct obj_data *obj, int cmd,
	char *arg, struct char_data *owner)
{
  if (cmd || !ch->master || ch->fighting || ch->in_room != ch->master->in_room) return eFAILURE;

   if(ch->master->fighting && IS_NPC(ch->master->fighting))
      do_join(ch, GET_NAME(ch->master), 9);

   return eFAILURE;
}


int gremlinthing(struct char_data *ch)
{
  if( GET_POS(ch) < POSITION_STANDING )
     return eFAILURE;

  if (ch->master && !IS_NPC(ch->master) && ch->master->pcdata->golem && 
ch->master->pcdata->golem->in_room
	== ch->in_room)
  {
     struct char_data *gol = ch->master->pcdata->golem;
    if (gol->hit < gol->max_hit)
    {
	  do_emote(ch, "climbs up its master's golem, hammering, tweaking and repairing.\r\n", 9);
	  gol->hit += number(40,60);
	  if (gol->hit > gol->max_hit) gol->hit = gol->max_hit;
    }
  }
  return eFAILURE;
}

int mage_familiar_gremlin(struct char_data *ch, struct obj_data *obj, int
  cmd, char *arg, struct char_data *owner)
{
 if (number(0,1)) return eFAILURE;
 return gremlinthing(ch);
}

int mage_familiar_gremlin_non(struct char_data *ch, struct obj_data *obj,
 int cmd, char *arg, struct char_data *owner)
{
  if (cmd) return eFAILURE;
  if(!ch->master) {
    log("Familiar without a master.", IMMORTAL, LOG_BUG);
    extract_char(ch, TRUE);
    return (eCH_DIED | eSUCCESS);
  }
  if( GET_POS(ch) < POSITION_STANDING )
     return eFAILURE;
  if(!ch->fighting) 
  {
    if(ch->in_room != ch->master->in_room) {
      do_emote(ch, "looks around, glances at its watch then skitters out of the room.\r\n", 9);
      move_char(ch, ch->master->in_room);
      do_emote(ch, "skitters into the room, anxiously looking for its master.\r\n", 9);
      return eFAILURE;
    }

    if(ch->master->fighting) { // help him!
       do_join(ch, GET_NAME(ch->master), 9);
      return eFAILURE;
    }

    if(number(1, 500) == 1) {
	act("$n chatters incessantly while magically fusing two small rocks together.",     ch, 0, 0, TO_ROOM, 0);
      return eFAILURE;
    }
  }

  if (number(0,4)) return eFAILURE;

  return gremlinthing(ch);
}



int mage_familiar_imp(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  if(number(0, 1))
    return cast_fireball(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));

  
  return eFAILURE;  
}

int mage_familiar_imp_non(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  if(cmd)
    return eFAILURE;

  if(ch->fighting)
    return eFAILURE;

  if(!ch->master) {
    log("Familiar without a master.", IMMORTAL, LOG_BUG);
    extract_char(ch, TRUE);
    return (eCH_DIED | eSUCCESS);
  }

  // do nothing unless doing nothing :)
  if( GET_POS(ch) < POSITION_STANDING )
     return eFAILURE;

  if(!ch->fighting) 
  {
    if(ch->in_room != ch->master->in_room) {
      do_emote(ch, "looks around for its master, then *eep*'s quietly and dissolves into shadow.\r\n", 9);
      move_char(ch, ch->master->in_room);
      do_emote(ch, "steps out of a nearby shadow relieved to be back in its master's presence.\r\n", 9);
      return eFAILURE;
    }

    if(ch->master->fighting) { // help him!
      do_join(ch, GET_NAME(ch->master), 9);
      return eFAILURE;
    }

    if(number(1, 500) == 1) {
      do_emote(ch, "chitters about for a bit then settles down.", 9);
      return eFAILURE;
    }
  }

  return eFAILURE;
}

int druid_familiar_owl_non(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *owner)
{
   if(ch->fighting)
    return eFAILURE;

   if (cmd == 155 && !owner->fighting && !ch->fighting && owner->master == ch)
   {
     char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
     arg = one_argument(arg, arg1);
     arg = one_argument(arg, arg2);
     if (str_cmp(arg1, "far") && str_cmp(arg1,"near"))
     {
       send_to_char("$BDo you want to spy $3far$7 or $3near$7?$R\r\n",ch);
	return eSUCCESS;
     }
     int dir;
     extern char *dirs[];
     for (dir = 0; *dirs[dir] != '\n'; dir++)
       if (!str_cmp(dirs[dir], arg2))
         break;
     if (*dirs[dir] == '\n' || !world[ch->in_room].dir_option[dir])
     {
       send_to_char("In what direction did you say?\r\n",ch);
       return eSUCCESS;
     }
     int to_room = 0;
     bool ts = IS_AFFECTED(ch, AFF_TRUE_SIGHT);
     if (!str_cmp(arg1,"far") && world[world[ch->in_room].dir_option[dir]->to_room].dir_option[dir])
       to_room = world[world[ch->in_room].dir_option[dir]->to_room].dir_option[dir]->to_room;
     if (!to_room) to_room = world[ch->in_room].dir_option[dir]->to_room;
     if (!check_components(ch, 1, 44, 0, 0, 0, TRUE))
     {
	send_to_char("The owl requires a feeding to do this for you.\r\n",ch);
        return eSUCCESS;
     }
     send_to_char("The owl accepts your mouse greedily.\r\n",ch);
     send_to_char("You see through the eyes of your familiar, looking into the distant room...\r\n",ch);
     int oldroom = ch->in_room;
     char_from_room(ch);
     char_to_room(ch, to_room);
     SETBIT(ch->affected_by, AFF_TRUE_SIGHT);
     do_look(ch, "", 9);
     if (!ts) REMBIT(ch->affected_by, AFF_TRUE_SIGHT);
     char_from_room(ch);
     char_to_room(ch, oldroom);
     return eSUCCESS;
   } else if (!cmd) {
  if(!ch->master) {
    log("Familiar without a master.", IMMORTAL, LOG_BUG);
    extract_char(ch, TRUE);
    return (eCH_DIED | eSUCCESS);
  }

  // do nothing unless doing nothing :)
  if( GET_POS(ch) < POSITION_STANDING )
     return eFAILURE;

  if(!ch->fighting) 
  {
    if(ch->in_room != ch->master->in_room) {
      do_emote(ch, "lifts from its perch and flies out of the room.\r\n", 9);
      move_char(ch, ch->master->in_room);
      do_emote(ch, "swoops into the room perching itself high up, watching its master.\r\n", 9);
      return eFAILURE;
    }

    if(ch->master->fighting) { // help him!
      do_join(ch, GET_NAME(ch->master), 9);
      return eFAILURE;
    }

    if(number(1, 500) == 1) {
      do_emote(ch, "circles above, looking for mice.", 9);
      return eFAILURE;
    }
  }
  }
 return eFAILURE;
}

int druid_familiar_chipmunk_non(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
          struct char_data *owner)
{
  if(cmd)
    return eFAILURE;

  if(ch->fighting)
    return eFAILURE;

  if(!ch->master) {
    log("Familiar without a master.", IMMORTAL, LOG_BUG);
    extract_char(ch, TRUE);
    return (eCH_DIED | eSUCCESS);
  }

  // do nothing unless doing nothing :)
  if( GET_POS(ch) < POSITION_STANDING )
     return eFAILURE;

  if(!ch->fighting) 
  {
    if(ch->in_room != ch->master->in_room) {
      do_emote(ch, "looks around for its master than runs off.\r\n", 9);
      move_char(ch, ch->master->in_room);
      do_emote(ch, "runs in and drops by its masters feet, obviously tired.\r\n", 9);
      return eFAILURE;
    }

    if(ch->master->fighting) { // help him!
      do_join(ch, GET_NAME(ch->master), 9);
      return eFAILURE;
    }

    if(number(1, 500) == 1) {
      do_emote(ch, "sqeaks with delight at a found nut.", 9);
      return eFAILURE;
    }

    if(number(1, 100) == 1) {
      send_to_char("The presence of your chipmunk is soothing to your mind.\r\n", ch->master);
      GET_MANA(ch->master) += 10;
      if(GET_MANA(ch->master) > GET_MAX_MANA(ch->master))
         GET_MANA(ch->master) = GET_MAX_MANA(ch->master);
      return eFAILURE;
    }
  }


  return eFAILURE;
}

// This is here so we don't need 2398429 procs that are just for a bodyguard
// Just add a case for your mob, and the mob he's supposed to protect
int bodyguard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  if(cmd)
    return eFAILURE;

  switch(mob_index[ch->mobdata->nr].virt) {
    case 9511: // sura mutant
      return protect(ch, 9510); // laiger

    case 9531: // andar
      return protect(ch, 9530); // andara

    case 9532: // Adua
      return protect(ch, 9529); // adele
     
    default:
      return eFAILURE;
  }
  return eFAILURE;
}


int generic_blocker(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if (cmd>6 || cmd<1)
        return eFAILURE;

    switch(ch->in_room) {
      case 6420:
        if(cmd != 1) // north
          break;
        act("$n is prevented from going north by $N.", ch, 0, owner, TO_ROOM, 0);
        act("You are prevented from going north by $N.", ch, 0, owner, TO_CHAR, 0);
        return eSUCCESS;
    }

    return eFAILURE;
}

int generic_doorpick_blocker(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
          struct char_data *owner)
{
    if(cmd != 35) // pick
       return eFAILURE;

    switch(ch->in_room) {
       case 3725:  // bishop room in nefarious
         act("$n stands protectively before the door.", owner, 0, 0, TO_ROOM, 0);
         return eSUCCESS;
         break;
       case 1382:  // Weapon enchanter room in TOHS
         act("$n stands protectively in front of the chest.", owner, 0, 0, TO_ROOM, 0);
         return eSUCCESS;
         break;
    }

    return eFAILURE;
}

int startrek_miles(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
          struct char_data *owner)
{
  if(cmd != 185)
    return eFAILURE;

  do_say(owner, "Don't push anything.  This is highly sophisticated equipment.\r\n", 9);
  return eSUCCESS;
}
