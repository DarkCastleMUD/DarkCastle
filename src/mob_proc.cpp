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
/* $Id: mob_proc.cpp,v 1.29 2003/01/30 05:48:10 pirahna Exp $ */
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


/* extern procedures */

int saves_spell(CHAR_DATA *ch, CHAR_DATA *vict, int spell_base, sh_int save_type);
bool many_charms(struct char_data *ch);
struct char_data *get_pc_vis_exact(struct char_data *ch, char *name);
void gain_exp(struct char_data *ch, int gain);

char * get_random_hate(CHAR_DATA *ch);

/* Data declarations */

struct social_type
{
  char *cmd;
  int next_line;
};

/* ********************************************************************
*  General special procedures for mobiles                                      *
******************************************************************** */

/* SOCIAL GENERAL PROCEDURES

If first letter of the command is '!' ths will mean that the following
command will be executed immediately.

"G",n      : Sets next line to n
"g",n      : Sets next line relative to n, fx. line+=n
"m<dir>",n : move to <dir>, <dir> is 0,1,2,3,4 or 5
"w",n      : Wake up and set standing (if possible)
"c<txt>",n : Look for a person named <txt> in the room
"o<txt>",n : Look for an object named <txt> in the room
"r<int>",n : Test if the npc in room number <int>?
"s",n      : Go to sleep, return false if can't go sleep
"e<txt>",n : echo <txt> to the room, can use $o/$p/$N depending on
	     contents of the **thing
"E<txt>",n : Send <txt> to person pointed to by thing
"B<txt>",n : Send <txt> to room, except to thing
"?<num>",n : <num> in [1..99]. A random chance of <num>% success rate.
	     Will as usual advance one line upon sucess, and change
	     relative n lines upon failure.
"O<txt>",n : Open <txt> if in sight.
"C<txt>",n : Close <txt> if in sight.
"L<txt>",n : Lock <txt> if in sight.
"U<txt>",n : Unlock <txt> if in sight.    */

// TODO - get this "general social" script thing to work, or some variant of it
//    so we can allow builders to easily make mobs do stuff
//    Also have to add a place for it in memory, and in the .mob files

/* Execute a social command.                                        */
void exec_social(struct char_data *npc, char *cmd, int next_line,
		 int *cur_line, void **thing)
{
  bool ok;

  int do_move(struct char_data *ch, char *argument, int cmd);
  int do_open(struct char_data *ch, char *argument, int cmd);
  int do_lock(struct char_data *ch, char *argument, int cmd);
  int do_unlock(struct char_data *ch, char *argument, int cmd);
  int do_close(struct char_data *ch, char *argument, int cmd);

  if (GET_POS(npc) == POSITION_FIGHTING)
    return;

  ok = TRUE;

  switch (*cmd) {

    case 'G' :
      *cur_line = next_line;
      return;

    case 'g' :
      *cur_line += next_line;
      return;

    case 'e' :
      act(cmd+1, npc, (OBJ_DATA *)*thing, *thing, TO_ROOM, 0);
      break;

    case 'E' :
      act(cmd+1, npc, 0, *thing, TO_VICT, 0);
      break;

    case 'B' :
      act(cmd+1, npc, 0, *thing, TO_ROOM, NOTVICT);
      break;

    case 'm' :
      do_move(npc, "", *(cmd+1)-'0'+1);
      break;

    case 'w' :
      if (GET_POS(npc) != POSITION_SLEEPING)
	ok = FALSE;
      else
	GET_POS(npc) = POSITION_STANDING;
      break;

    case 's' :
      if (GET_POS(npc) <= POSITION_SLEEPING)
	ok = FALSE;
      else
	GET_POS(npc) = POSITION_SLEEPING;
      break;

    case 'c' :  /* Find char in room */
      *thing = get_char_room_vis(npc, cmd+1);
      ok = (*thing != 0);
      break;

    case 'o' : /* Find object in room */
      *thing = get_obj_in_list_vis(npc, cmd+1, world[npc->in_room].contents);
      ok = (*thing != 0);
      break;

    case 'r' : /* Test if in a certain room */
      ok = (npc->in_room == atoi(cmd+1));
      break;

    case 'O' : /* Open something */
      do_open(npc, cmd+1, 0);
      break;

    case 'C' : /* Close something */
      do_close(npc, cmd+1, 0);
      break;

    case 'L' : /* Lock something  */
      do_lock(npc, cmd+1, 0);
      break;

    case 'U' : /* UnLock something  */
      do_unlock(npc, cmd+1, 0);
      break;

    case '?' : /* Test a random number */
      if (atoi(cmd+1) <= number(1,100))
	ok = FALSE;
      break;

    default:
      break;
  }  /* End Switch */

  if (ok)
    (*cur_line)++;
  else
    (*cur_line) += next_line;
}

// This is purely a utility function for use inside of other mob_procs.
// You send it the mob (ch) and the Vnum of the people you want to join it
// (iFriendId) and it will search for them, call for help, and they will
// join in combat.  (If they are in the room)
// Returns TRUE if we got help, FALSE if not
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

      GET_HIT(vict) -= damage; // Note -damage will HEAL the player
      update_pos(vict);
      inform_victim(ch, vict, damage);
      if(GET_HIT(vict) < 1)
        fight_kill(ch, vict, TYPE_CHOOSE);
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

// Patrol is another purely utilitarian function to include inside a proc.
// You call "patrol()" and the mob will check for players in all the
// rooms near it.  If there are any players fighting, the mob will walk
// into that room.  (it returns eSUCCESS if it went off, and eFAILURE if it didn't)
// Making the mob join/fight/run away/masturbate while watching is up to
// you in the calling proc. 

// standard return values
int patrol(struct char_data *ch)
{
    for(int i = 0; i < 6; i++)
    {
        if(!(CAN_GO(ch, i)))
            continue;
        for(char_data * target = world[EXIT(ch, i)->to_room].people; 
            target; 
            target = target->next_in_room)
        if(target->fighting)
            return do_simple_move(ch, i, FALSE);        
    }

    return eFAILURE;
}


void npc_steal(struct char_data *ch,struct char_data *victim)
{
    long gold;

    if(IS_NPC(victim)) return;
    if(GET_LEVEL(victim)>20) return;

    if (AWAKE(victim) && (number(0,GET_LEVEL(ch)) == 0)) {
	act("You discover that $n has $s hands in your wallet.",
	    ch,0,victim,TO_VICT, 0);
	act("$n tries to steal gold from $N.", ch, 0, victim, 
	  TO_ROOM, INVIS_NULL|NOTVICT);
    } else {
	/* Steal some gold coins */
	gold = ((GET_GOLD(victim)*number(1,10))/100);
	if (gold > 0) {
	    GET_GOLD(ch) += gold;
	    GET_GOLD(victim) -= gold;
	}
    }
}


int snake(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
          struct char_data *owner)
{
   int retval;

   if(cmd) return eFAILURE;

   if(GET_POS(ch)!=POSITION_FIGHTING) return eFAILURE;
    
   if ( ch->fighting && 
	(ch->fighting->in_room == ch->in_room) &&
	number(0 , 120) < 2 * GET_LEVEL(ch) )
	{
	    act("You bite $N!",  ch, 0, ch->fighting, TO_CHAR, INVIS_NULL);
	    act("$n bites $N!", ch, 0, ch->fighting, TO_ROOM, INVIS_NULL|NOTVICT);
	    act("$n bites you!", ch, 0, ch->fighting, TO_VICT, 0);
	    retval = cast_poison( GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
            if(SOMEONE_DIED(retval))
              return retval;
            return eSUCCESS;
	}
    return eFAILURE;
}

int robber(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   char_data *cons;

   if (cmd) {
      return eFAILURE;
      }
      
   if (GET_POS(ch) != POSITION_STANDING ||
       IS_SET(world[ch->in_room].room_flags, SAFE)) {
      return eFAILURE;
      }
   
   for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
      if ((!IS_NPC(cons)) && (GET_LEVEL(cons) < ANGEL) && (number(1, 5) == 1))
         npc_steal(ch, cons); 

    return eSUCCESS;
}

// non_combat proc for all mob "mages"
int passive_magic_user(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    // struct char_data *vict;
    // int percent;
    if(cmd) return eFAILURE;
    if (GET_POS(ch) <= POSITION_FIGHTING) return FALSE;

    if(IS_AFFECTED(ch, AFF_BLIND)) {
      act("$n utters the words 'Let there be light!'.", ch, 0, 0, TO_ROOM, 
	INVIS_NULL);
       cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
          return eSUCCESS;
      }

    if(!affected_by_spell(ch, SPELL_SHIELD) && GET_LEVEL(ch) > 12) {
      act("$n utters the words 'pongun'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_shield(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

    if(!affected_by_spell(ch, SPELL_STONE_SKIN) && GET_LEVEL(ch) > 31) {
      act("$n utters the words 'teri hatcher'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_stone_skin(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

    if(!IS_AFFECTED(ch, AFF_FIRESHIELD) && GET_LEVEL(ch) > 47) {
      act("$n utters the words 'puew mai'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_fireshield(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

    return eFAILURE;
}

// combat_proc for all mob "mages"
int active_magic_user(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner) 
{
   CHAR_DATA *vict;
   int retval = eSUCCESS;

   if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;
   
   if((GET_POS(ch) != POSITION_FIGHTING)) {
       return eFAILURE;
   }
    /* Find a dude to do evil things upon ! */

    vict = ch->fighting;

    if (!vict)
	return eFAILURE;

    if(IS_AFFECTED(ch, AFF_BLIND)) {
      act("$n utters the words 'Let there be light!'.", ch, 0, 0, 
       TO_ROOM, INVIS_NULL);
       cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
          return eSUCCESS;
      }
   

    if( vict==ch->fighting && GET_LEVEL(ch)>29 && number(0,3)==0 )
       {
         if ((IS_AFFECTED(vict, AFF_SANCTUARY))  ||
             (IS_AFFECTED(vict, AFF_FIRESHIELD))  ||
             (IS_AFFECTED(vict, AFF_HASTE))) {
   
  act("$n utters the words 'Instant Magic Remover(tm)'.", ch, 0, 0, TO_ROOM,
    INVIS_NULL);
         cast_dispel_magic(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
           return eSUCCESS;
        }
      }

   if( vict==ch->fighting && (GET_LEVEL(ch)>18) &&
       (!IS_AFFECTED(ch, AFF_HASTE)) && number(0, 2)==0 )
     {
     act("$n utters the words 'I wanna be FAST!'.", ch, 0, 0, TO_ROOM,
       INVIS_NULL);
      cast_haste(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
           return eSUCCESS;
      }


    if( vict==ch->fighting && GET_LEVEL(ch)>7 && number(0,4)==0 )
    {
	act("$n utters the words 'koholian dia'.", ch, 0, 0, TO_ROOM,
	  INVIS_NULL);
	retval = cast_blindness(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	return retval;
    }

    if( (GET_LEVEL(ch)>12) && (number(0,6)==0) && IS_EVIL(ch))
    {
	act("$n utters the words 'Slurp, Slurp!'.", ch, 0, 0, TO_ROOM,
	  INVIS_NULL);
	return cast_energy_drain(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }

    switch (GET_LEVEL(ch)) {
	case 1:
	case 2:
	case 3:
	case 4:
	    act("$n utters the words 'hahili duvini'.", ch, 0, 0, TO_ROOM,
	      INVIS_NULL);
	    retval = cast_magic_missile(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	    break;
	case 5:
	case 6:
	case 7:
	case 8:
	    act("$n utters the words 'grynt oef'.", ch, 0, 0, TO_ROOM, 
	      INVIS_NULL);
	    retval = cast_burning_hands(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	    break;
	case 9:
	case 10:
	    act("$n utters the words 'ZZZZZZTTTTT!'.", ch, 0, 0, TO_ROOM,
	      INVIS_NULL);
	    retval = cast_lightning_bolt(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	    break;
	case 11:
	case 12:
	case 13:
	case 14:
	    act("$n utters the words 'Pretty colours!'.", ch, 0, 0, TO_ROOM, 
	      INVIS_NULL);
	    retval = cast_colour_spray(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	    break;
        case 15:
        case 16:
        case 17:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
	  act("$n utters the words 'Burn Baby, Burn!'.", ch, 0, 0, TO_ROOM,
	    INVIS_NULL);
	    retval = cast_fireball(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
           break;
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
	  act("$n utters the words 'Duck for cover sucker'.", ch, 0, 0,
	    TO_ROOM, INVIS_NULL);
	    retval = cast_meteor_swarm(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	    break;
        default:
            act("$n utters the words 'Burn in hell!'.", ch, 0, 0,
	    TO_ROOM, INVIS_NULL);
              retval = cast_hellstream(
                   GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
              break;

         
    }
    return retval;
}

int passive_magic_user2(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    /*struct descriptor_data *d;*/

    struct char_data *vict;
    int percent;

    if(cmd) return eFAILURE;
    if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;
    if (GET_POS(ch) <= POSITION_FIGHTING) return eFAILURE;

    if(IS_AFFECTED(ch, AFF_BLIND)) 
    {
      act("$n utters the words 'Let there be light!'.", ch, 0, 0,
        TO_ROOM, INVIS_NULL);
      cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    if(!ch->fighting)
    {
      if(IS_AFFECTED(ch, AFF_BLIND)) 
      {
        act("$n utters the words 'Let there be light!'.", ch, 0, 0,
          TO_ROOM, INVIS_NULL);
        cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
        return eSUCCESS;
      }

      percent = (100*GET_HIT(ch)) / GET_MAX_HIT(ch);
      if(percent < 80)
        return eFAILURE;

      if(ch->mobdata->hatred) {
        vict = get_pc_vis_exact(ch, get_random_hate(ch));
        if (vict && !ch->fighting) {
           act("$n utters the words 'Your ass is MINE'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
           cast_summon(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
           return eSUCCESS;
        }
      }
    }
  return eFAILURE;
}

int active_magic_user2(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner) 
{
    CHAR_DATA *vict;
    int percent;
    int retval; 
    /* Find a dude to do evil things upon ! */

    if((GET_POS(ch) != POSITION_FIGHTING)) {
        return eFAILURE;
    }

    vict = ch->fighting;

    if (!vict)
	return eFAILURE;

    if(IS_AFFECTED(ch, AFF_BLIND)) {
      act("$n utters the words 'Let there be light!'.", ch, 0, 0, TO_ROOM,
	INVIS_NULL);
       cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
          return eSUCCESS;
      }

   if (IS_SET(ch->mobdata->actflags, ACT_WIMPY)) {

     percent = (100*GET_HIT(ch)) / GET_MAX_HIT(ch);

     if (percent < 40) {
    act("$n utters the words 'Hasta la vista, Baby!'.", ch, 0, 0, TO_ROOM, 0);
      cast_teleport(GET_LEVEL(ch), ch, "" ,SPELL_TELEPORT, ch, 0, GET_LEVEL(ch));
          return eSUCCESS;
    }
  }

   

    if( vict==ch->fighting && GET_LEVEL(ch)>29 && number(0,3)==0 )
       {
         if ((IS_AFFECTED(vict, AFF_SANCTUARY))  ||
             (IS_AFFECTED(vict, AFF_FIRESHIELD))  ||
             (IS_AFFECTED(vict, AFF_HASTE))) {
   
  act("$n utters the words 'Instant Magic Remover(tm)'.", ch, 0, 0, TO_ROOM, 
    INVIS_NULL);
         cast_dispel_magic(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
           return eSUCCESS;
        }
      }

   if( vict==ch->fighting && (GET_LEVEL(ch)>18) &&
       (!IS_AFFECTED(ch, AFF_HASTE)) && number(0, 2)==0 )
     {
     act("$n utters the words 'I wanna be FAST!'.", ch, 0, 0, TO_ROOM,
       INVIS_NULL);
      cast_haste(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
           return eSUCCESS;
      }


    if( vict==ch->fighting && GET_LEVEL(ch)>49 && number(0,4)==0 )
    {
	act("$n utters the words 'Burn in hell'.", ch, 0, 0, 
	  TO_ROOM, INVIS_NULL);
	return cast_hellstream(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }


    if( vict==ch->fighting && GET_LEVEL(ch)>7 && number(0,4)==0 )
    {
	act("$n utters the words 'koholian dia'.", ch, 0, 0,
	  TO_ROOM, INVIS_NULL);
	return cast_blindness(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }

    if( (GET_LEVEL(ch)>12) && (number(0,6)==0) && IS_EVIL(ch))
    {
	act("$n utters the words 'Slurp, Slurp!'.", ch, 0, 0,
	 TO_ROOM, INVIS_NULL);
	return cast_energy_drain(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }

    switch (GET_LEVEL(ch)) {
	case 1:
	case 2:
	case 3:
	case 4:
	    act("$n utters the words 'hahili duvini'.", ch, 0, 0,
	      TO_ROOM, INVIS_NULL);
	    retval = cast_magic_missile(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	    break;
	case 5:
	case 6:
	case 7:
	case 8:
	    act("$n utters the words 'grynt oef'.", ch, 0, 0, TO_ROOM,
	      INVIS_NULL);
	    retval = cast_burning_hands(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	    break;
	case 9:
	case 10:
	    act("$n utters the words 'ZZZZZZTTTTT!'.", ch, 0, 0, TO_ROOM,
	      INVIS_NULL);
	    retval = cast_lightning_bolt(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	    break;
	case 11:
	case 12:
	case 13:
	case 14:
	    act("$n utters the words 'Pretty colours!'.", ch, 0, 0,
	      TO_ROOM, INVIS_NULL);
	    retval = cast_colour_spray(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	    break;
        case 15:
        case 16:
        case 17:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
	  act("$n utters the words 'Burn Baby, Burn!'.", ch, 0, 0, TO_ROOM,
	    INVIS_NULL);
	    retval = cast_fireball(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
           break;
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
	  act("$n utters the words 'Duck for cover sucker'.", ch, 0, 0,
	    TO_ROOM, INVIS_NULL);
	    retval = cast_meteor_swarm(
		GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
	    break;
        default:
            act("$n utters the words 'I love Acid!'.", ch, 0, 0,
	      TO_ROOM, INVIS_NULL);
              retval = cast_acid_blast(
                   GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
              break;

         
    }
    return retval;
}

void cleric_healing(char_data * ch, char_data * vict)
{
      if(GET_LEVEL(ch) > 25) {
         act("$n utters the words 'Royal Bigmac!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
         cast_full_heal(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
         return;
       }

      if(GET_LEVEL(ch) > 13) {
         act("$n utters the words 'Cheeseburger!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
         cast_heal(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
         return;
       }

      if(GET_LEVEL(ch) > 8) {
         act("$n utters the words 'Chicken Nuggets!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
         cast_cure_critic(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
         return;
       }

      if(GET_LEVEL(ch) > 1) {
         act("$n utters the words 'Small Fries!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
         cast_cure_light(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
         return;
       }
}

// non_combat proc for all mob "clerics"
int passive_cleric(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *vict;

    if(cmd) return eFAILURE;
    if (GET_POS(ch) <= POSITION_FIGHTING) return eFAILURE;

    if(IS_AFFECTED(ch, AFF_BLIND)) {
      act("$n utters the words 'Let there be light!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    if((!IS_AFFECTED(ch, AFF_SANCTUARY) && GET_LEVEL(ch) > 17)) {
      act("$n utters the words 'Divine Protection!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_sanctuary(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    if((GET_HIT(ch) + 5) < GET_MAX_HIT(ch)) {
      cleric_healing(ch, ch);
      return eSUCCESS;
    }

   // TODO - Rest of self spells, like truesight
   // TODO - earthquaking if hidden and i'm angry

   // spelling up/healing the other mobs in the room
   for(vict = world[ch->in_room].people;
       vict;
       vict = vict->next_in_room)
   {
     // I might want to remove the ch == vict part and then remove all the self
     // buffing first from the top.  That way this loop will catch the cleric.
     // However, we can't be guaranteed the cleric will help themself first in
     // that case though.

     if(vict == ch || !IS_NPC(vict) || IS_AFFECTED(vict, AFF_CHARM))
       continue;

     if(!IS_AFFECTED(ch, AFF_SANCTUARY) && GET_LEVEL(ch) > 17) {
       act("$n utters the words 'Divine Protection!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
       cast_sanctuary(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
       return eSUCCESS;
     }

     if((GET_HIT(vict) + 5) < GET_MAX_HIT(vict)) {
       cleric_healing(ch, vict);
       return eSUCCESS;
     }
   }
   
  return eFAILURE;
}

// combat proc for all cleric "mobs"
int active_cleric(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   CHAR_DATA *vict;

   if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;
   if((GET_POS(ch) != POSITION_FIGHTING) || (!ch->fighting)) {
      return eFAILURE;
   }

   vict = ch->fighting;

   if (!vict)
      return FALSE;

   if(IS_AFFECTED(ch, AFF_BLIND)) {
      act("$n utters the words 'Let there be light!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
   }

   // TODO - earthquake   

   if( ( ( (double) GET_HIT(ch) / (double) GET_MAX_HIT(ch) ) < .75 )
       && number(0, 1))
   {
      cleric_healing(ch, ch);
      return eSUCCESS;
   }

   if( vict==ch->fighting && GET_LEVEL(ch)>45 && number(0,4)==0 )
   {
      if ((IS_AFFECTED(vict, AFF_SANCTUARY))  ||
          (IS_AFFECTED(vict, AFF_FIRESHIELD))  ||
          (IS_AFFECTED(vict, AFF_HASTE))) 
      {
         act("$n utters the words 'Instant Magic Remover(tm)'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
         return cast_dispel_magic(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
      }
   }

   if( vict==ch->fighting && GET_LEVEL(ch)>24 && number(0,3)==0 )
   {
      if (IS_GOOD(vict) && IS_EVIL(ch)) 
      {
         act("$n utters the words 'Suffer Sucker'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
         return cast_dispel_good(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
      }
      if (IS_EVIL(vict) && IS_GOOD(ch)) 
      {
         act("$n utters the words 'Suffer Sucker'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
         return cast_dispel_evil(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
      }
   }

   if ( GET_LEVEL(ch) >= 15 && GET_LEVEL(ch) <= MAX_MORTAL )
   {
      act("$n utters the words 'Tongue of fire'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      return cast_flamestrike ( GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch) );
   }
   return eFAILURE;
}

// bard combat proc
int bard_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  struct char_data *vict;

  if(cmd) return eFAILURE;

  if(GET_POS(ch) != POSITION_FIGHTING)
    return eFAILURE;
  
  if(!ch->fighting)
    return eFAILURE;
  
  if(IS_AFFECTED(ch, AFF_PARALYSIS))
    return eFAILURE;
  
  if(MOB_WAIT_STATE(ch))
    return eFAILURE;

  vict = ch->fighting;
  
  if(!vict)
    return eFAILURE;

  // whistle sharp every round
  return song_whistle_sharp(GET_LEVEL(ch), ch, "", vict, GET_LEVEL(ch));
}

// combat proc for mob monks
int monk(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  struct char_data *vict;
  int dam;

  if(cmd) return eFAILURE;

  if(GET_POS(ch) != POSITION_FIGHTING)
    return eFAILURE;
  
  if(!ch->fighting)
    return eFAILURE;
  
  if(IS_AFFECTED(ch, AFF_PARALYSIS))
    return eFAILURE;
  
  if(MOB_WAIT_STATE(ch))
    return eFAILURE;

  vict = ch->fighting;
  
  if(!vict)
    return eFAILURE;

  // Always want to dispel first.  FS and Sanct hurt
  if(GET_LEVEL(ch) > 40)
  {
    if(number(0, 1) && 
       ( IS_AFFECTED(vict, AFF_FIRESHIELD) ||
         IS_AFFECTED(vict, AFF_SANCTUARY)
       )
      )
    {
      return ki_disrupt(GET_LEVEL(ch), ch, 0, vict);
    }
  }

  // only 1 in 100 chance of this happening - _very_ deadly
  if(GET_LEVEL(ch) > 39 && number(1, 100) == 1)
  {
    MOB_WAIT_STATE(ch) = 2;
    if(number(0, 1))
    {
      dam = dice((GET_LEVEL(ch)), MORTAL) + (25 * (GET_LEVEL(ch)));
      return damage(ch, vict, dam, TYPE_UNDEFINED, SKILL_QUIVERING_PALM, 0);
    }
    else
      return damage(ch, vict, 0, TYPE_UNDEFINED, SKILL_QUIVERING_PALM, 0);
  }

  // 25% chance of stun
  if(GET_LEVEL(ch) > 30 && number(1, 4) == 1 )
  {
    MOB_WAIT_STATE(ch) = 4;
   if(number(0,1)) 
   {
     damage (ch, vict, 0,TYPE_UNDEFINED, SKILL_STUN, 0);
     act("$n delivers a HARD BLOW into your solar plexus!  You are STUNNED!",
           ch, NULL, vict, TO_VICT , 0);
     act("You deliver a HARD BLOW into $N's solar plexus!  $N is STUNNED!",
           ch, NULL, vict, TO_CHAR , 0);
     act("$n delivers a HARD BLOW into $N's solar plexus!  $N is STUNNED!",
           ch, NULL, vict, TO_ROOM, NOTVICT );

     WAIT_STATE(vict, PULSE_VIOLENCE*2);
     MOB_WAIT_STATE(ch) = 4;
     if(GET_POS(vict) > POSITION_STUNNED)
       GET_POS(vict) = POSITION_STUNNED;
     SET_BIT(vict->combat, COMBAT_STUNNED);
     return eSUCCESS;
   }
   else 
   {
     damage (ch, vict, 0,TYPE_UNDEFINED, SKILL_STUN, 0);

     act("$n attempts to hit you in your solar plexus!  You block $s attempt.",
        ch, NULL, vict, TO_VICT , 0);
     act("You attempt to hit $N in $s solar plexus...   YOU MISS!",
        ch, NULL, vict, TO_CHAR , 0);
     act("$n attempts to hit $N in $S solar plexus...   $e MISSES!",
        ch, NULL, vict, TO_ROOM, NOTVICT );
     MOB_WAIT_STATE(ch) = 4;
     return eSUCCESS;
   }
  }

  // If all else fails, kick
  if (GET_LEVEL(ch) > 2) {
    MOB_WAIT_STATE(ch) = 2;
    do_kick(ch, "", 9);
  }

  return eFAILURE;
}

int monk_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   if (cmd) return FALSE;
   if (GET_POS(ch) <= POSITION_FIGHTING) return FALSE;
   if (IS_AFFECTED(ch, AFF_PARALYSIS)) return FALSE;

   if( (  affected_by_spell(ch, SPELL_BLINDNESS)
       || affected_by_spell(ch,SPELL_POISON)   
       || affected_by_spell(ch,SPELL_WEAKEN)
       ) 
      && GET_LEVEL(ch) > 4)
   {
      ki_purify(GET_LEVEL(ch), ch, 0, ch);
      return eSUCCESS;
   }

   // TODO - put up ki sense
   // TODO - Later on, if I'm "smart" and have ki sense up, I should see if there
   // are any hiddens in the room.  If there are, and I'm aggro/mad, ki storm

   return eFAILURE;
}

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

int fighter_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   if (cmd)                             return eFAILURE;
   if (GET_POS(ch) <= POSITION_FIGHTING) return eFAILURE;
   if (IS_AFFECTED(ch, AFF_PARALYSIS))  return eFAILURE;

   // TODO - If I have sense life, and i see a hidden, and i'm aggro/hate someone
   //        I should do a hitall (like monk proc)

   return eFAILURE;
}

// non_combat proc for all mob "mages"
int passive_necro(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    // struct char_data *vict;
    // int percent;
    if(cmd) return eFAILURE;
    if (GET_POS(ch) <= POSITION_FIGHTING) return FALSE;

    if(IS_AFFECTED(ch, AFF_BLIND)) {
      act("$n utters the words 'dead eye'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    if(!affected_by_spell(ch, SPELL_SHIELD) && GET_LEVEL(ch) > 12) {
      act("$n utters the words 'pongun'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_shield(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

    if(!affected_by_spell(ch, SPELL_STONE_SKIN) && GET_LEVEL(ch) > 31) {
      act("$n utters the words 'beetle bailey'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_stone_skin(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

    if(!affected_by_spell(ch, SPELL_ACID_SHIELD) && GET_LEVEL(ch) > 47) {
      act("$n utters the words 'deadtly aura'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_acid_shield(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

    return eFAILURE;
}

// combat_proc for all mob "mages"
int active_necro(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner) 
{
   CHAR_DATA *vict;
//   int retval = eSUCCESS;

   if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;
   
   if((GET_POS(ch) != POSITION_FIGHTING)) {
       return eFAILURE;
   }
    /* Find a dude to do evil things upon ! */

    vict = ch->fighting;

    if (!vict)
	return eFAILURE;

    if( GET_LEVEL(ch) > 29 && number(0,2)==0 )
    {
       if ((IS_AFFECTED(vict, AFF_SANCTUARY))  ||
           (IS_AFFECTED(vict, AFF_FIRESHIELD))  ||
           (IS_AFFECTED(vict, AFF_HASTE))) 
       {
          act("$n utters the words 'death stops all'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
          cast_dispel_magic(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
          return eSUCCESS;
       }
    }

    if(GET_LEVEL(ch) > 18 && !IS_AFFECTED(ch, AFF_HASTE) && number(0, 2)==0 )
    {
       act("$n utters the words 'I wanna be FAST!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
       cast_haste(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
       return eSUCCESS;
    }

    if( (GET_LEVEL(ch)>12) && number(0,1))
    {
	act("$n utters the words 'Slurp, Slurp!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
	return cast_vampiric_touch(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }

    if( !IS_AFFECTED(vict, AFF_BLIND) ) {
       act("$n utters the words 'koholian dia'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
       return cast_blindness(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }

    return eFAILURE;
}


int passive_tarrasque(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    extern struct str_app_type str_app[];

    /*struct descriptor_data *d;*/
    struct char_data *vict;
    struct obj_data *best_obj;
    int max;
    int percent;

    if(cmd) return eFAILURE;

    if(!ch->fighting) 
    {
        if(IS_AFFECTED(ch, AFF_BLIND)) {
            act("$n utters the words 'Let there be light!'.", ch, 0, 0, TO_ROOM,
         	INVIS_NULL);
            cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
            return eSUCCESS;
        }
    }
  
    if (world[ch->in_room].contents && number(0,2) == 0) 
    {
        max = 1;
        best_obj = 0;

        for (obj = world[ch->in_room].contents; obj; obj = obj->next_content) {
             if(CAN_GET_OBJ(ch, obj) && (!strcmp("corpse", obj->name)) ) {
                 best_obj = obj;
                 break;
             }
 
             if (CAN_GET_OBJ(ch, obj) && obj->obj_flags.cost > max) {
                 best_obj = obj;
                 max = obj->obj_flags.cost;
             }
        } // for

        if (best_obj)  {
            obj_from_room(best_obj);
            act("$n gets $p.", ch, best_obj, 0, TO_ROOM, 0);
            act("$n eats $p.", ch, best_obj, 0, TO_ROOM, 0);
            act("$n burps.",   ch, 0,0, TO_ROOM, 0);
            extract_obj(best_obj);
            return eSUCCESS;
        } // best_obj
    } // contents

    if(ch->mobdata->hatred)
    {
      percent = (100*GET_HIT(ch)) / GET_MAX_HIT(ch);
      if(percent < 80)
           return eFAILURE;

//      vict = get_pc_vis_exact(ch, ch->mobdata->hatred);
      vict = get_pc_vis_exact(ch, get_random_hate(ch));

      if ( vict && !ch->fighting) {
            act("$n utters the words 'Your ass is MINE'.",  ch, 0, 0, TO_ROOM, 
	         INVIS_NULL);
	    return cast_summon(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
      }
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

int summonbash(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner) 
{
  /*struct descriptor_data *d;*/
  struct char_data *vict;
  int percent;

  if(cmd) return eFAILURE;

   
  if(IS_AFFECTED(ch, AFF_BLIND)) {
     act("$n utters the words 'Let there be light!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
     cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
     return eSUCCESS;
  }

  if(ch->fighting && number(1, 5) == 5)
  {
    return do_bash(ch, "", 9);
  }

  if(ch->mobdata->hatred && !ch->fighting) 
  {
    percent = (100*GET_HIT(ch)) / GET_MAX_HIT(ch);
    if(percent < 80)
      return eFAILURE;

//          vict = get_pc_vis_exact(ch, ch->mobdata->hatred);
          vict = get_pc_vis_exact(ch, get_random_hate(ch));
          if (vict && !ch->fighting) {
           act("$n utters the words 'Your ass is MINE'.", ch, 0, 0, TO_ROOM,
            INVIS_NULL);
            cast_summon(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
            return do_bash(ch, GET_NAME(vict), 9); 
          }
    }
     return eFAILURE;
}

int passive_grandmaster(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  /*struct descriptor_data *d;*/
  struct char_data *vict;
  int percent;

  if(cmd) return eFAILURE;

  if(!ch->fighting)
    if(IS_AFFECTED(ch, AFF_BLIND)) {
      act("$n utters the words 'Let there be light!'.", ch, 0, 0, TO_ROOM,
	INVIS_NULL);
       cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
          return eSUCCESS;
      }

    if(ch->mobdata->hatred) {
      percent = (100*GET_HIT(ch)) / GET_MAX_HIT(ch);
      if(percent < 80)
        return eFAILURE;

//      vict = get_pc_vis_exact(ch, ch->mobdata->hatred);
      vict = get_pc_vis_exact(ch, get_random_hate(ch));
      if (vict && !ch->fighting) {
           act("$n utters the words 'Your ass is MINE'.", ch, 0, 0, TO_ROOM,
	    INVIS_NULL);
	    return cast_summon(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
          }
    }
    return eFAILURE;
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
    if(GET_LEVEL(ch)>30 && number(0,2)==0 )
       {
         if ((IS_AFFECTED(vict, AFF_SANCTUARY))  ||
             (IS_AFFECTED(vict, AFF_FIRESHIELD))  ||
             (IS_AFFECTED(vict, AFF_HASTE))) {
   
  act("$n utters the words 'Instant Magic Remover(tm)'.", ch, 0, 0,
   TO_ROOM, INVIS_NULL);
         return cast_dispel_magic(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
        }
      }


    if(GET_LEVEL(ch)>40 && number(0,3)==0 )
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

int baby_troll(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  struct char_data *vict;

  if (cmd) return eFAILURE;
  if (!AWAKE(ch))
    return eFAILURE;

  if (!ch->fighting) {
    
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      if (number(0,1)==0)
    break;

    if (!vict)
      return eFAILURE;

    switch (number(0,15))
    {
      case 1 :
	act( "The baby troll says, 'Wana play?'",vict, 0, 0, TO_ROOM, 
	  0);
	break;
      case 2 :
	act("The baby troll says, 'Come sit down and play with us.'",
		vict, 0, 0, TO_ROOM, 0);
	break;
      case 3 :
	act("The baby troll throws something greasy and grimey at $n!",\
	    vict, 0, 0, TO_ROOM, 0);
	act("The baby troll throws something greasy and grimey at you!",
	    vict,0,0,TO_CHAR, 0);
	break;
      default:
	return eFAILURE;
    }
  }
  
  return eFAILURE;
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
   "Backyard green tree cemetary dawns, wine drunkeness over the rooftops,",
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

int ring_keeper(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *tch;
    /*struct char_data *mob;
    char buf[MAX_INPUT_LENGTH];

    char *strName;*/

    if (cmd || !AWAKE(ch))
	return eFAILURE;

    if(ch->fighting) {
      if(number(0,10)==0)
        do_flee(ch, "", 0);
      return eSUCCESS;
    }


    for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
       if(ch->mobdata->hatred)
         if((CAN_SEE(ch, tch)) &&
           (isname(tch->name, ch->mobdata->hatred))) {
	       if(!can_attack(ch) || !can_be_attacked(ch, tch))
	             return eSUCCESS;
		      
  
           if(ch->equipment[WIELD])  {
             if(tch->fighting) {
               act ("$n circles around $s target....\n\r", ch, 0, 0, TO_ROOM, INVIS_NULL);
               SET_BIT(ch->combat, COMBAT_CIRCLE);
             }
             return attack (ch, tch, SKILL_BACKSTAB);
           }
           else
             return attack (ch, tch, TYPE_UNDEFINED);
           return eSUCCESS;
        }
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


int secret_agent(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    /*struct char_data *tch;
    struct char_data *mob;
    char buf[MAX_INPUT_LENGTH];

    char *strName;*/

    if (cmd || !AWAKE(ch))
	return eFAILURE;

     if(ch->fighting) {
       if(number(0,20)==0)
         do_flee(ch, "", 0);
       return eSUCCESS;
     }

    /* for ( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
    {
	if ( IS_SET(tch->affected_by, AFF_KILLER) )
	    strName = "KILLER";
	else
	    continue;

	sprintf( buf,
	    "%s is a %s!  PROTECT THE INNOCENT!  MORE BLOOOOD!!!",
	    GET_SHORT(tch),
	    strName );
	do_shout( ch, buf, 0 );
    if (ch->equipment[WIELD])
	return attack( ch, tch, SKILL_BACKSTAB );
   else  return attack(ch, tch, TYPE_UNDEFINED);

	break;
    } */

    return eFAILURE;
}



int Executioner(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    /*struct char_data *tch;
    struct char_data *mob;
    char buf[MAX_INPUT_LENGTH];
    char *strName;*/

    if (cmd || !AWAKE(ch))
	return eFAILURE;

    /* for ( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
    {
	if ( IS_SET(tch->affected_by, AFF_KILLER) )
	    strName = "KILLER";
	else
	    continue;

	sprintf( buf,
	    "%s is a %s!  PROTECT THE INNOCENT!  MORE BLOOOOD!!!",
	    GET_SHORT(tch),
	    strName );
	do_shout( ch, buf, 0 );
	mob = clone_mobile( real_mobile(3060));
	char_to_room( mob, ch->in_room );
	mob = rclone_mobile( real_mobile(3060));
	char_to_room( mob, ch->in_room );
	return attack( ch, tch, TYPE_UNDEFINED );
	break;
    } */

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

int blue_dragon(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if(cmd) 
      return eFAILURE;

    if(GET_POS(ch)!=POSITION_FIGHTING) 
       return eFAILURE;
    
    if(!ch->fighting) 
       return eFAILURE;

    /* Find a dude to do evil things upon ! */

    if(number(0,6) > 1)
      return eFAILURE;

    act("$n breathes lightning.", ch, 0, 0, TO_ROOM, 0);
    return cast_lightning_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
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
  

/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

int guild_guard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if (cmd>6 || cmd<1)
	return eFAILURE;

    // TODO - go through these and remove all of the ones that are in
    // room that no longer exist on the mud

    if ( ( GET_CLASS(ch) != CLASS_MAGIC_USER
	&& ch->in_room == real_room(3017) && cmd == 1 )
    ||   ( GET_CLASS(ch) != CLASS_CLERIC
	&& ch->in_room == real_room(3004) && cmd == 1 )
    ||   ( GET_CLASS(ch) != CLASS_THIEF
	&& ch->in_room == real_room(3027) && cmd == 2 )
    ||   ( GET_CLASS(ch) != CLASS_WARRIOR
	&& ch->in_room == real_room(3021) && cmd == 2 )
    ||   ( GET_CLASS(ch) != CLASS_DRUID
	&& ch->in_room == real_room(3216) && cmd == 2 )
    ||   ( GET_CLASS(ch) != CLASS_BARD
	&& ch->in_room == real_room(3213) && cmd == 4 )
    ||   ( GET_CLASS(ch) != CLASS_ANTI_PAL
        && ch->in_room == real_room(9910) && cmd == 4 )
    ||   ( GET_CLASS(ch) != CLASS_PALADIN
        && ch->in_room == real_room(9900) && cmd == 1 )
    ||   ( GET_CLASS(ch) != CLASS_BARBARIAN 
        && ch->in_room == real_room(9905) && cmd == 1 )
    ||   ( GET_CLASS(ch) != CLASS_MONK
        && ch->in_room == real_room(9921) && cmd == 1 )
    ||   ( GET_CLASS(ch) != CLASS_MONK
        && ch->in_room == real_room(9921) && cmd == 4 )
    ||   ( GET_CLASS(ch) != CLASS_RANGER
        && ch->in_room == real_room(9924) && cmd == 3 ) 
    ||   ( !IS_EVIL(ch) 
        && ch->in_room == real_room(9910) && cmd == 4 )
    ||   ( !IS_GOOD(ch)
        && ch->in_room == real_room(9900) && cmd == 1 )
    || (GET_CLASS(ch) != CLASS_BARBARIAN
        && ch->in_room == real_room(10053) && cmd == 2)
    || (GET_CLASS(ch) != CLASS_THIEF
        && ch->in_room == real_room(10057) && cmd == 2)
    || (GET_CLASS(ch) != CLASS_PALADIN
        && ch->in_room == real_room(10083) && cmd == 2)
    || (GET_CLASS(ch) != CLASS_MAGIC_USER
        && ch->in_room == real_room(10097) && cmd == 2)
    || (GET_CLASS(ch) != CLASS_CLERIC
        && ch->in_room == real_room(10095) && cmd == 2)
    || (GET_CLASS(ch) != CLASS_RANGER
        && ch->in_room == real_room(10085) && cmd == 3)
    || (GET_CLASS(ch) != CLASS_ANTI_PAL
        && ch->in_room == real_room(10088) && cmd == 1)
    || (GET_CLASS(ch) != CLASS_WARRIOR
        && ch->in_room == real_room(10091) && cmd == 3)
    || (GET_CLASS(ch) != CLASS_MONK
        && ch->in_room == real_room(10094) && cmd == 3)
    ||   (IS_AFFECTED(ch, AFF_CANTQUIT) 
        && (!IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF)))


	)
    {
	act( "The guard humiliates $n, and blocks $s way.",
	    ch, 0, 0, TO_ROOM , 0);
	send_to_char(
	    "The guard humiliates you, and blocks your way.\n\r", ch );
	return eSUCCESS;
    }

    return eFAILURE;
}

int halfling_people(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if (cmd>6 || cmd<1)
        return eFAILURE;

    if (  (ch->in_room == real_room(10105) && cmd == 1 )
       || (ch->in_room == real_room(10107) && cmd == 1 )
       || (ch->in_room == real_room(10166) && cmd == 3 ) ) {
       act( "The militiaman shuffles in front of the door and blocks $n.",
          ch, 0, 0, TO_ROOM , 0);
       send_to_char(
          "The militiaman shuffles in front of the door to stop you.\n\r", ch );
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

    if ( (in_room == real_room(161) && cmd != 2 )
    ||   (in_room == real_room(158) && cmd != 4 )
    ||   (in_room == real_room(2426) && cmd != 2 ) // anaphrodesia, east
    ||   (in_room == real_room(127) && cmd != 4 )  // west
    ||   (in_room == real_room(124) && cmd != 1 )
    ||   (in_room == real_room(138) && cmd != 1 )
    ||   (in_room == real_room(141) && cmd != 3 )  
    ||   (in_room == real_room(144) && cmd != 2 )
    ||   (in_room == real_room(2339) && cmd != 1 )  // askani, north
    ||   (in_room == real_room(6010) && cmd != 6 )
    ||   (in_room == real_room(167) && cmd != 1 )
    ||   (in_room == real_room(2350) && cmd != 3 )   // elcipse, south
    ||	 (in_room == real_room(2344) && cmd != 1 )   // north
    ||	 (in_room == real_room(187) && cmd != 1 )   // sng, north
    ||	 (in_room == real_room(2315) && cmd != 1 )  // ferach, north
    ||	 (in_room == real_room(2411) && cmd != 1 )  // suindicate north
    ||	 (in_room == real_room(2305) && cmd != 2 )  // arcana, east
    ||   (in_room == real_room(181) && cmd != 2 )   // dark_tide, east
    ||	 (in_room == real_room(2300) && cmd != 1 )  // bandaleros, north
    ||	 (in_room == real_room(2321) && cmd != 3 )  // anarchist, south
    ||	 (in_room == real_room(2329) && cmd != 3 )  // darkened2, south
    ||	 (in_room == real_room(2325) && cmd != 1 )  // blackaxe, north
    ||	 (in_room == real_room(2336) && cmd != 1 )  // timewarp, north
    ||	 (in_room == real_room(2370) && cmd != 1 )  // tayledreas, north
    ||	 (in_room == real_room(2374) && cmd != 5 )  // solaris, up
    ||   (in_room == real_room(2380) && cmd != 3 )  // overlords, south
    ||   (in_room == real_room(9344) && cmd != 1 )  // overlords, north 
	)
      return eFAILURE;
	
	
    if ( (ch->clan != 1 && in_room == real_room(167))   // ulnhyrr
    ||   (ch->clan != 2 && in_room == real_room(127))   // tengu
    ||   (ch->clan != 3 && in_room == real_room(2305))  // arcana
    ||   (ch->clan != 5 && in_room == real_room(144))   // darkened
    ||   (ch->clan != 7 && in_room == real_room(158))   // studs
    ||   (ch->clan != 8 && in_room == real_room(124))   // vampyre
    ||   (ch->clan != 9 && in_room == real_room(187))   // sng
    ||   (ch->clan != 10 && in_room == real_room(2370)) // Tayledras
    ||   (ch->clan != 12 && in_room == real_room(6010)) // slackers
    ||   (ch->clan != 13 && in_room == real_room(2315)) // ferach
    ||   (ch->clan != 34 && in_room == real_room(138))  // nazgul
    ||   (ch->clan != 16 && in_room == real_room(2350))  // eclipse
    ||   (ch->clan != 17 && in_room == real_room(2339))  // epoch
    ||   (ch->clan != 18 && in_room == real_room(2321)) // anarchist
    ||	 (ch->clan != 19 && in_room == real_room(2344))  // ViG
    ||   (ch->clan != 21 && in_room == real_room(2426))  // dc_guard
    ||   (ch->clan != 22 && in_room == real_room(141))  // co.rpse
    ||   (ch->clan != 24 && in_room == real_room(161))  // smoke_jaguars
    ||   (ch->clan != 26 && in_room == real_room(2411)) // sindicate
    ||   (ch->clan != 28 && in_room == real_room(181))  // dark_tide 
    ||   (ch->clan != 29 && in_room == real_room(2300)) // bandaleros
    ||   (ch->clan != 5 && in_room == real_room(2329)) // darkened2
    ||   (ch->clan != 4 && in_room == real_room(2325)) // blackaxe
    ||   (ch->clan != 6 && in_room == real_room(2336)) // timewarp
    ||   (ch->clan != 11 && in_room == real_room(2374)) // solaris
    ||   (ch->clan != 15 && in_room == real_room(2380)) // overlords
    ||   (ch->clan != 15 && in_room == real_room(9344)) // overlords
	)
    {
	act( "$n is turned away from the clan hall.", ch, 0, 0, TO_ROOM , 0);
	send_to_char("The clan guard throws you out on your ass.\n\r", ch );
	return eSUCCESS;
    }
    else if(IS_AFFECTED(ch, AFF_CANTQUIT) && affected_by_spell(ch, FUCK_PTHIEF)) { 
	act( "$n is turned away from the clan hall.", ch, 0, 0, TO_ROOM , 0);
	send_to_char("The clan guard says 'Hey don't be bringing trouble around here!'\n\r", ch );
	return eSUCCESS;
    }
    return eFAILURE;
}


int anthor_mouth(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if(IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;

    if (cmd)
	return eFAILURE;

    switch (number(0, 12))
    {
	case 5:
	    do_say(ch, "Can you believe what that Godflesh character is doing?!", 0);
            break;
	case 2:
	    do_say(ch, "Hear any good gossip lately? ", 0);
            break;
	case 8:
	    do_say(ch, "I used to be quite the adventurer in my days!", 0);
            break;
	case 9:
	    do_say(ch, "You wanna form a group?", 0);
            break;
	default:
	    return eFAILURE;
    }
    return eSUCCESS;
}


int gossip(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    if(IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;

    if (cmd)
	return eFAILURE;

    switch (number(0, 12))
    {
	case 5:
	    do_say(ch, "Can you believe what that Godflesh character is doing?!", 0);
	    break;
	case 2:
	    do_say(ch, "Hear any good gossip lately? ", 0);
	    break;
	case 8:
	    do_say(ch, "I used to be quite the adventurer in my days!", 0);
	    break;
	case 9:
	    do_say(ch, "You wanna form a group?", 0);
	    break;
	default:
	    return eFAILURE;
    }
    return eSUCCESS;
}


/*--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--*/
static char *dethSayText [ ] =
{
    "Can i lick your stamps ?",
    "Want to buy a some cheap godload equipment ?",
    "Gravity.  It's not just a good idea, it's the law.",
    "I'm hooked on phonics !",
    "I'm through with Sergio, he treats me like a ragdoll",
    "How much vaseline should i use the first time ?",
    "Saaaweeet looking ass on that sanitation engineer !",
    "Want to see my hathead ?",
    "Can I borrow a few coins, at least until i find another sucker..",
    "I don't think the hydrostatic law is REALLY true.",
    "I'm an Innie, You ?",
    "I think I'll meta con next.",
    "See ya later, I need to go buy some more pipeweed.",
    "Good God, such soft hands you have..touch me more.",
    "Is it in yet ? I can't feel a thing.",
    "Why does my butt itch ?",
    "I like to lick testicles. Do you have any?",
    "My doctor recommends Monistat 7.", 
  "All that grunting and groaning for one measly fart?!",
  "I'll give you a brownie point if you snuggle me.",
  "Shirt and shoes required? Hell, that's all I need?",
  "Hey, Why is someone poking me in the ribs?",
  "Gods! Here I come, I can immort!",
  "You mean I can use a different piece of toilet paper?",
  "Mmmm smells great.  What'd you have for dinner?",
  "Can i clap the erasers on the bulletin board?",
  "I think this buffalo wing deal is a hoax, they taste like chicken.",
  "I've got the neatest alias, alias backtotown == rec recall.",
  "I've got the neatest alias, alias assholes == whoclan 1.",
  "Oooo a half-eaten nachos from Juan's just laying here on the ground.",
  "Damn this heat, it makes me shed.",
  "What the hell is that SMELL ?!?",
  "DOH !  I keep losing my link when i clap.",
  "Anyone trade me two twenties for a forty?",
  "I'd join clan [Studs] but they keep giving me wedgies.",
  "Ooo bastard enfan scrapped my lantern.",
  "Quit killing me, I'm saving up for a new pair of plate leggings.",
  "I think I'll stroll on over and hang out with Wesley Crusher for a while.",  
  "There is something inately sexual about my bald head.",
  "How does a moron think up a punchline?",
  "I've got the kleenex, I've got the vaseline.  Party!",
  "Michael Bolton ROCKS!!",
  "I particularly love Rush Limbaugh's pulpy eggy core.",
  "I can't wait to get that Van-Damme Butt-Clenching video!",
  "The arena has been opened! Type junk backpack to enter the bloodbath!",
  "There is a fine line between fishing and standing on the shore looking "
  "like an idiot.",
  "My palms keep shedding!",
  "Newt Gingrich makes me all wet and sticky.",
  "Why are you mudding, and not out getting laid?",
  "Gods!  Can I get a transport back to town please?",
  "My penis got caught in a milking machine, best experience of my life.",
  "Imagine if birds could be tickled by feathers.",
  "Senate Democrats today conceded defeat in an initiative to tax Gravity.",
  "You must think I'm pretty stupid for smoking all that oregano.",
  "Michael Huffington is my hero.",
  "Now I have to type 'kil'?? FUCK this!",
  "The Republicans are in power?  I might have to get a job!",
  "Anyone else here know how to breakdance?",
  "Hey Tro, how come you never return my calls anymore?",
  "Coke is IT!!",
  "Is it monday? I can't miss Melrose Place.",
  "Hey look, I have a pubic hair.",
  "Hey wait, this is a FINGER rubber!",
  "You have to admit, there are touches of genius in DOS.",
  "Gods, I need a reimb.  I got killed.",
  "Where's the monks guild?",
  "You'd be surprised how hard it is to put tab A into slot B.",
  "Anna Nicole Smith is one sick bitch.",
  "The Crawford/Gere split is final! Now's my chance!",
  "Is it 7 yet? I don't want to miss Judge Wapner.",
  "Waz, please don't zap me. I didn't think wiggle was a spammable action.",
  "Are there any imps on?  I have some stupid questions.",
  "Does anyone know the address for Hidden Worlds?",
  "Kurt Cobain is dead.",
  "You must think I'm pretty stupid for snorting all that aspirin.",
  "This whole godload thing really sucks. "
  "I can't even kill a poodle without a Wavy Bladed Kris these days.",
  "Uh-oh, I've been flagged a spammer!",
  "I think that recalls are quite reasonably priced.",
  "How do I set up tintin on my VAX account?",
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
  "I'm gonna be an Uruk'hai so the Nazgul stop killing me.",
  "Can someone help me get wintin to run on my Mac?",
  "I can never seem to rent a U-Haul in Oklahoma anymore.",
  "Quick!  Everyone buy stock in Netscape, I have a g00d feeling!",
  "Sadus, can I be a playtester for ++?",
  "Liriel wanted to sleep with me, but I told her only if she buys a strap-on..",
  "What state is Maryland in?",
  "Luna wants me.",
  "I heard Diana's driver was related to Ted Kennedy.",
  "Where's my silk stockings?  I got a date with Marv Albert tonight.",
  "My 7 year old sister can hold her liquor better than Anarchy...gross.",
  "This Godload SUCKS!",
  "Can I be an -=Umi_Helper=- too?",
  "FUCKING SHIT!",
  "Karma karma karma karma karma chameleon",
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
  "Would another name for pickled bread be Dill-dough?",
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
  "Since Americans throw rice at weddings, do Orientals throw hamburgers?",
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
  "When eskimos sit on the ice too long do they get polaroids?",
  "I heard when Bill Clinton was asked his opinion on foreign affairs, he replied, 'I don't know. I never had one.'",
  "Oral sex may make your day, but anal sex makes your hole weak.",
  "The flour was too messy!",
  "Pirahna er en ful apa!",
  "What do you call a prostitute with a runny nose?        Full",
  "Do soy beans and dildos both count as substitute meat?",
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
  "Raha!  When am I getting my 321 plats back?!?!?",
  "I fucked your mom...  No no, really, I fucked your mom.",
  "smoking.",
  "My aunt's a piece of ass.",
  "Fishing Rod.",
  "Would you rather be the ghoul, or the fucking fag?",
  "You're leaning on my lexus.",
  "Kid rock SUCKS",
  "Quit fouling me!  What's all this aboot?  If you wanna play fuutball we can.",
  "HEY!  You in the water!  Stop having fun!",
  "Don't dare me to do something man, cause I'll do it.",
  "Those damn Eclipse are always trying to get my lucky charms.",
  "Hey Orion!  Make sure your mom comes to the next party.  In Nausica's skirt!",
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
  "One good turn ..... Gets most of the blankets",
  "COLE'S LAW: Thinly sliced cabbage",
  "Editing is a rewording activity",
  "Rap is to music what Etch-a-Sketch is to art",
  "Never miss a good chance to shut up.",
  "How many of you believe in telekinesis? Please raise my hand",
  "I was thinking that women should put pictures of missing husbands on beer cans.",
  "It compiled? The first screen came up? Ship it!  --  Bill Gates",
  "1024x768x256... Sounds like one mean woman",
  "2B OR NOT 2B = FF",
  "9 out of 10 men, who tried Camels, prefer women",
  "A seminar on Time Travel will be held two weeks ago",
  "Apathy Error: Don't bother striking any key.",
  "Bad: Your children are sexually active. Worse: With each other.",
  "Ban the Bomb. Save the world for conventional warfare.",
  "Canadian DOS: Yer sure, eh? [Y,n]",
  "CAUTION! Do not look into laser with remaining eye.",
  "Coarse and violent nudity. Occasional language.",
  "Cosmetics: preventing men from reading between the lines",
  "Democracy: 3 wolves and a sheep voting on dinner",
  "Don't drink water, fish fuck in it",
  "Earth is shutting down in five minutes--please save all files and log out.",
  "Ever noticed how fast Windows run? Me neither.",
  "Evolution is God's way of issuing upgrades.",
  "Happiness is 9,10-Didehydo-N,N-diethyl-6-methylergoline-8B-carboxamide.",
  "I used to miss my girlfriend, but my aim improved.",
  "Insert disk 5 of 4 and press any key to continue.",
  "It is not enough to succeed. Others must fail.",
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
  "If a pig loses it's voice, is it disgruntled?",
  "Do Roman paramedics refer to IV's as 4's?",
  "Are people more violently opposed to fur rather than leather because it's much easier to harass rich women than motorcycle gangs?",
  "If you take an Oriental person and spin him around several times, does he become disoriented?",
  "Brake fluid mixed with Clorox makes smoke, and lots of it",
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
  "Avoid parking tickets by leaving your windshield wipers turn to fast wipe whenever you leave your car parked illegally.",
  "Old telephone books make ideal personal address books.  Simply cross out the names and addresses of people you don't know.",
  "As I said before, I never repeat myself",
  "Drink until she's cute, but stop before the wedding",
  "I'm not cheap, but I am on special this week",
  "Don't hit a man with glasses.....Use your fist",
  "I drive way too fast to worry about cholesterol",
  "The only substitute for good manners is fast reflexes",
  "Give a man a free hand and he'll run it all over you",
  "In Florida it is illegal to jog with your eyes closed.",
  "In Fairbanks, Alaska it is illegal to give beer to a moose.",
  "Impotence: Nature's way of saying no hard feelings.",
  "A good scapegoat is nearly as welcome as a solution to the problem.",
  "As I let go of my feelings of guilt, I can get in touch with my Inner Sociopath.",
  "I have the power to channel my imagination into ever-soaring levels of suspicion and paranoia.",
  "Joan of Arc heard voices too.",
  "Just one letter makes all the difference between here and there",
  "If time heals all wounds, how come the belly button stays the same?",
  "A conscience is what hurts when all your other parts feel so good.",
  "For every action, there is an equal and opposite criticism.",
  "If you must choose between two evils, pick the one you've never tried before.",
  "Half the people you know are below average.",
  "42.7 percent of all statistics are made up on the spot.",
  "'The escape pod is not an option.' 'Why not?' 'It escaped last Thursday.'",
  "Australian kissing: like French kissing, but in the land down under.",
  "Why did Pilgrim's pants always fall down?  Because they wore their belt buckles on their hats.",
  "Why are there 5 syllables in the word 'monosyllabic'?",
  "Why is there only one Monopolies commission?",
  "How come you press harder on a remote-control when you know the battery is dead?",
  "One nice thing about egotists:  They don't talk about other people.",
  "I doubt, therefore I might be.",
  "I didn't climb to the top of the food chain to be a vegetarian.",
  "College is just one big party, with a 25,000 dollar cover charge.",
  "My friend has kleptomania, but when it gets bad, he takes something for it.",
  "Never be afraid to try something new.  Remember amateurs built the ark - Professionals built the Titanic.",
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
  "Don't worry. It's only seems kinky the first time.",
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
  "Hey guys, what's this?  ...  I donno, but here it comes again.",
  "Here's a little song I wrote, might want to sing it note for note, don't worry, be happy.",
  "Take me to the river...Throw me in the water!",
  "Check me out!  I drank 4 bottles of wine!  *urp*  I guess that's a net of 2 now....",
  "Okay....what's this 'Pin the tail on the Phish' shit?",
  "Kill whitey!",
  "Hey Gamera, didn't you say you were tired?  Btw, where's your 'friend' and what're all these new claw marks on your back?",
  "Dammit Wasp, where's MY mug?!?",
  "You think that horse is impressive?  Let me show you something....",
  "Baby, I'm an American Express lover.... you shouldn't go home without me!",
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
  "I hope if dogs ever take over the world, and they chose a king, they don't just go by size, because I bet there are some Chihuahuas with some good ideas.",
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
  "No one ever says 'It's only a game,' when their team is winning.",
  "Ever wonder what the speed of lightning would be if it didn't zigzag?",
  "When cheese gets its picture taken, what does it say?",
  "Why are a wise man and a wise guy opposites?",
  "Why do overlook and oversee mean opposite things?",
  "Why do bankruptcy lawyers expect to be paid? ",
  "I'd kill for a Nobel Peace prize.",
  "Chastity is curable, if detected early.",
  "Love may be blind, but marriage is a real eye-opener.",
  "Borrow money from pessimists- they don't expect it back.",
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
  "A Unix user once said, 'rm -rf /*'  And since then everything has null and void.",
  "All your base are belong to us.",
  "You have no chance to survive make your time.",
  "Someone set up us the bomb.",
  "Why did the belt get arrested?  ...  For holding up the pants!",
  "Sometimes I like to rub icy hot over my entire body, and sit naked fondling myself as I watch excercise videos.",
  "Oh, it's just a RAM upgrade...I can back up those files later.",
  "Why do they call it common sense if so few people seem to have it?",
  "Oh whatever.  MUDing won't effect my GPA.  I can handle it.",
  "I've been throwing this lettuce in the air for an hour now...when does it start feeling good?",
  "Do not sully the great name of Mike Ditka.",
  "Did you ever notice when you put the 2 words 'The' and 'IRS' together it spells 'THEIRS'?",
  "The sole purpose of a child's middle name is so he can tell when he's really in trouble.",
  "The older you get, the tougher it is to lose weight, because by then your body and your fat are really good friends.",
  "The real art of conversation is not only to say the right thing at the right time, but also to leave unsaid the wrong thing at the tempting moment.",
  "Don't assume malice for what stupidity can explain.",
  "If you can't be kind, at least have the decency to be vague.",
  "When I'm feeling down, I like to whistle. It makes the neighbor's dog run to the end of his chain and gag himself.",
  "Birds of a feather flock together and crap on your car.",
  "A diplomat is one who can bring home the bacon without spilling the beans.",
  "A fine is a tax for doing wrong.  A tax is a fine for doing well.",
  "A first grade teacher is one who knows how to make little things count.",
  "A smart man is one who convinces his wife she looks fat in a fur coat.",
  "Bachelor: a man who wouldn't take \"Yes\" for an answer.",
  "Bowling tends to get kids off the streets and into the alleys.",
  "Confession may be good for the soul, but it's often bad for the reputation.",
  "Despite inflation, a penny is still a fair price for too many people's thoughts.",
  "Don't marry for money--you can borrow it more cheaply.",
  "Egotism is not one of the better virtues, but is does lubricate the wheels of life.",
  "Everybody should learn to drive, especially those who sit behind the wheel.",
  "Everything in the modern home is controlled by switches except the children.",
  "Fun is like life insurance; the older you get the more it costs.",
  "He who turns the other cheek too far gets it in the neck.",
  "Horsepower was a lot safer when only horses had it.",
  "If all cars in the U.S. were placed end to end, some fool would try to pass them.",
  "To get away with criticizing, leave the person with the idea he has been helped.",
  "If you want to forget all your other troubles, wear tight shoes.",
  "Intuition enables a woman to contradict her husband before he says anything.",
  "Many men are self-made, but only the successful ones will admit it.",
  "Need a helping hand?  Try the end of your arm.",
  "Opportunity merely knocks--temptation kicks the door in.",
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
  "When people run around and around in circles we say they are crazy. When planets do it we say they are orbiting.",
  "When they broke open molecules, they found they were only stuffed with atoms.  But when they broke open atoms, they found them stuffed with explosions.",
  "A vibration is a motion that cannot make up its mind which way it wants to go.",
  "Water freezes at 32 degrees and boils at 212 degrees. There are 180 degrees between freezing and boiling because there are 180 degrees between north and south.",
  "Lime is a green-tasting rock.",
  "Many dead animals of the past turned into fossils while others preferred to be oil.",
  "Genetics explain why you look like your father and if you don't why you should.",
  "To most people solutions mean finding the answers. But to chemists solutions are things that are still all mixed up.",
  "Clouds just keep circling the Earth around and around. And around. There is not much else to do.",
  "We keep track of the humidity in the air so we won't drown when we breathe.",
  "Rain is often spoken of as soft water, oppositely known as hail.",
  "In some rocks you can find the fossil footprints of fishes.",
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
  "You can't spell 'Crap' without 'Rap'."
};

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
            GET_OBJ_WEIGHT(i) < 20)
        {
            act("$n picks up some trash.", ch, 0, 0, TO_ROOM, 0);
            move_obj(i, ch);
            return eSUCCESS;
	}
    }
    return eFAILURE;
}

int cityguard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   struct char_data *vict;
   char buf[100];

   if (cmd || !AWAKE(ch) || (GET_POS(ch) == POSITION_FIGHTING))
      return eFAILURE;

   vict = ch->fighting;

      if(!vict)
        return eFAILURE;
       
      if(number(0,500) > 8)
        return eFAILURE;
 
      if(IS_NPC(vict))
        return eFAILURE;

      if(GET_POS(vict) == POSITION_FIGHTING)
        return eFAILURE;
        
      if(GET_ALIGNMENT(vict) <= -300)
         {
         sprintf(buf, "%s grips his sword tighter as he sees you approach\n\r",
                 GET_SHORT(ch));
         send_to_char(buf, vict);
         sprintf(buf, "%s grips his sword tighter as he sees %s approach.",
                 GET_SHORT(ch), GET_SHORT(vict));
         act(buf, ch, 0, vict, TO_ROOM, NOTVICT);
         return eSUCCESS;
         }
      if((GET_ALIGNMENT(vict) > -300) && (GET_ALIGNMENT(vict) < 300))
         {
         sprintf(buf, "%s eyes you suspiciously.\n\r", GET_SHORT(ch));
         send_to_char(buf, vict);
         sprintf(buf, "%s eyes %s suspiciously", GET_SHORT(ch), GET_SHORT(vict));
         act(buf, ch, 0, vict, TO_ROOM, NOTVICT);
         return eSUCCESS;
         }  
       if(GET_ALIGNMENT(vict) >= 300)
         {
         sprintf(buf, "%s bows before you and says, 'Good Day'\n\r", 
                 GET_SHORT(ch));
         send_to_char(buf, vict);
         sprintf(buf, "%s bows before %s", GET_SHORT(ch), GET_SHORT(vict));
         act(buf,  ch, 0, vict, TO_ROOM, NOTVICT);
         return eSUCCESS;
         }
       return eFAILURE;
}

int cry_dragon(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
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
        retval = spell_damage(ch, tmp_victim, dam, TYPE_POISON, SPELL_GAS_BREATH, 0);
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

static char *ianSayText [] = {
 "MO' Ian!!!",
 "Fuck You Cake!",
 "NOOOooooo!  Clowwwwwwnnnns!",
 "Uh-moh-in missa aventuras.",
 "Tell me 'bout da bunnies!",
 "Don't hurt muh friend Ian!",
 "Ohhhh, I gotta make uh big poopie.", 
 "I'm just a little slow....",
 "Hello my name is Ian, and I like to do draw-rings!",
 "My name is Ian, Ian Gump.",
 "Are you my mommy????",
 "Boys have a penis, girls have a vagina."
};

#define IAN_SAY_TEXT_SIZE ( sizeof(ianSayText) / sizeof(char*))

int ian (struct char_data*ch, int cmd, char*arg,        
          struct char_data *owner)
{
   int x;
   if (IS_AFFECTED(ch,AFF_PARALYSIS))
     return eFAILURE;
   if (cmd)
     return eFAILURE;
   x = number (0,IAN_SAY_TEXT_SIZE * 60);
   if((unsigned) x < IAN_SAY_TEXT_SIZE) {
     do_say(ch, ianSayText [ x ], 9);
     return eSUCCESS;
   }
   return eFAILURE;
}

static char *headSayText [] = {
 "Damn, I wish I had a body!",
 "I know something you don't know!",
 "Read some books in the library, you may even learn something.",
 "Many times there is more than meets the eye.",
 "Many are with the fallen angels waiting to rise again.",
 "To find the fallen angels, follow your nose.",
 "Aia ovela igpa atinla!",
 "A book may help you find what you are looking for.",
 "Need help finding the entrance, listen closely.",
 "Ont'da uckfa ithwa heta ingka!",
 "Beware of the pentagrams.",
 "Labels don't lie.",
 "Pitfalls can come at any time, watch your step!"
};

#define HEAD_SAY_TEXT_SIZE ( sizeof(headSayText) / sizeof(char*))

int head (struct char_data*ch, int cmd, char*arg,        
          struct char_data *owner)
 {
   int x;

   if (IS_AFFECTED(ch,AFF_PARALYSIS))
     return eFAILURE;
   if (cmd)
     return eFAILURE;
   x = number (0,HEAD_SAY_TEXT_SIZE * 60);
   if((unsigned) x < HEAD_SAY_TEXT_SIZE) {
     do_say(ch,headSayText [ x ], 9);
     return eSUCCESS;
   }
   return eFAILURE;
}

static char *madmelSayText [] = {
 "I love my Metal Alchemical Catheter!",
 "And by the Grignard Reaction we get 2,4 dinitro *mumble* *mumble*",
 "Never leave a beaker of ether lying around... epoxide... explosion!",
 "And then the soaps would cause a glass of tap water to have a nice head.",
 "Hmm.. could be because of the field full of goats with 4 feet in the air.",
 "And then the soaps would cause a glass of tap water to have a nice head.",
 "Hmm.. could be because of the field full of goats with 4 feet in the air.",
 "MmmmmmmmmmmmM  Buckey Balls!",
 "Bubble bubble toil and trou..... AH fuck it!",
 "HI HO HO HA HA HA HE HE HO HO HO HA HI HO!!!!",
 "*mumble* pinch of pig scrotum *mumble* dash of this shit *mumble* *mumble*",
};

#define MADMEL_SAY_TEXT_SIZE ( sizeof(madmelSayText) / sizeof(char*))

int madmel (struct char_data*ch, int cmd, char*arg,        
          struct char_data *owner)
{
   int x;

   if (IS_AFFECTED(ch,AFF_PARALYSIS))
     return eFAILURE;
   if (cmd)
     return eFAILURE;
   x = number (0,MADMEL_SAY_TEXT_SIZE * 60);
   if((unsigned) x < MADMEL_SAY_TEXT_SIZE) {
     do_say(ch,madmelSayText [ x ], 9);
     return eSUCCESS;
   }
   return eFAILURE;
}

int annoyingbirthdayshout(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   if (cmd)
      return eFAILURE;

   switch (number(0, 12))
   {
   case 0:
      do_shout(ch, "Happy Birthday to Pirahna!  Happy Birthday to me!", 0);
      break;
   case 2:
      do_shout(ch, "Happy Birthday to Pirahna!  Happy Birthday to you!", 0);
      break;
   default:
      return eFAILURE;
   }
   return eSUCCESS;
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
	    sprintf(buf, "%8d - %s\n\r",
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
	SET_BIT(pet->affected_by, AFF_CHARM);

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

    if ( ( GET_LEVEL(ch) > 15 /* mud school */
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

static char *charonSayText [] = {
"There is a charge to cross upon my boat.",
"Many souls have I accompanied, but none as ugly as you.",
"Do you wish to cross the river?",
"Type 'list' to see how much I will charge you.",
"Type 'pay' if you wish to cross."
};

#define CHARON_SAY_TEXT_SIZE (sizeof(charonSayText) / sizeof(char *) )

int charon(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   int x;                                                                       
   char buf[MAX_INPUT_LENGTH];
   char_data *vict;
                                                                                
   if(!((cmd == 0) || (cmd == 59) || (cmd == 183)))
      return eFAILURE;
                                                                                
   if(IS_AFFECTED(ch,AFF_PARALYSIS))                                            
     return eFAILURE;                                                       

   if(!cmd) /* its not a cmd, so it must be a emote time */
   {   
      x = number (0, CHARON_SAY_TEXT_SIZE*5);                             
                                                                                
      if((unsigned) x < CHARON_SAY_TEXT_SIZE) {  
         do_say(ch, charonSayText [x],0);                          
         return eSUCCESS; 
       }                                                                           
         return eFAILURE;    
   }
   
   x = ch->weight;
   x -= 100;
   if(x < 1)
    x = 1;
   x *= 5000;

   /* ch typed list */
   if(cmd == 59) {
      sprintf(buf, "Charon motions that the cost for %s will be %i.\r\n",
           GET_SHORT(ch), x);
      for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room)   
         send_to_char(buf, vict);
      return eSUCCESS;
   }
   
   /* ch typed pay */
   if(GET_GOLD(ch) < (uint32)x) {
      send_to_char("Charon ignores those that cannot afford his services.", ch);
      return eSUCCESS;
   }

   GET_GOLD(ch) -= x;
   act("$n is taken across the river by Charon.", ch, 0,0,TO_ROOM, 0);
   send_to_char("Charon takes your coins and ferry's you across.\r\n", ch); 
   send_to_char("At this point, we ferry you across, but I donno what room yet.\r\n", ch);

   // if(real_room(blah) == -1) {
   //    send_to_char("Destination room invalid.  Tell a God.\r\n", ch);
   //    return TRUE;
   // }

   // char_from_room(ch);
   // char_to_room(ch, real_room(blah));
   
   return eSUCCESS;
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
       cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
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

int ranger_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  struct obj_data *wielded;
  struct char_data *vict;
  // int dam;

  if(cmd) return eFAILURE;


  if (GET_POS(ch) < POSITION_FIGHTING)  return eFAILURE;
  if(GET_POS(ch)!=POSITION_FIGHTING)    return eFAILURE;
  if(!ch->fighting)            return eFAILURE;
  if (IS_AFFECTED(ch, AFF_PARALYSIS))   return eFAILURE;
  if(MOB_WAIT_STATE(ch))                return eFAILURE;

  vict = ch->fighting;

  if (!vict)
    return eFAILURE;
   
   if(number(1, 5) == 1 && GET_LEVEL(ch) > 44) {
      act("$n utters the words 'Save this Dinas!'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      return cast_creeping_death(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
   }
   
   if(number(1, 5) == 1 && GET_LEVEL(ch) > 29) {
     MOB_WAIT_STATE(ch) = 4;
     return do_stun(ch, "", 9);
   }

   wielded = vict->equipment[WIELD];

   if (ch->equipment[WIELD] && vict->equipment[WIELD])
   if (!IS_NPC(ch) || !IS_NPC(vict))
   if ((!IS_SET(wielded->obj_flags.extra_flags ,ITEM_NODROP)) &&
          (GET_LEVEL(vict) <= MAX_MORTAL ))
   if( vict==ch->fighting && GET_LEVEL(ch)>9 && number(0,2)==0 )
   {
     MOB_WAIT_STATE(ch) = 2;
     disarm(ch,vict);
     return eSUCCESS;
   }

   if(number(1, 2) == 1) {
     act("$n utters the words 'Get the point?'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
     return cast_bee_sting(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
   }

   return eFAILURE;
}

int ranger_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    // struct char_data *vict;
    // int percent;
    if(cmd) return eFAILURE;
 
    if (GET_POS(ch) <= POSITION_FIGHTING) 
      return eFAILURE;

    if(ch->mobdata->hatred) 
    {
      if(ch->ambush)
        dc_free(ch->ambush);
      ch->ambush = str_dup(get_random_hate(ch));
    }
    else if(ch->ambush)
    {
      dc_free(ch->ambush);
      ch->ambush = NULL;
    }

    if(GET_HIT(ch) < GET_MAX_HIT(ch) && number(1,3) == 1) {
      act("$n utters the words 'Herb Lore'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_herb_lore(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    if(!IS_AFFECTED(ch,AFF_INFRARED)) {
      act("$n utters the words 'owl eyes'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_eyes_of_the_owl(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

    if(!affected_by_spell(ch, SPELL_BARKSKIN) && GET_LEVEL(ch) > 24) {
      act("$n utters the words 'iwannawoody'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_barkskin(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
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

int areanotopen (struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
     int x;

     if (cmd)
       return eFAILURE;

     x = number (0, 10);

     if(x < 5) {
       do_shout(ch, "This zone is NOT open.  Leave now or risk zap.", 9);
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
       cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
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
       cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
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

// I just like to acid blast or hellstream every round.
int acidhellstreamer(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
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
       cast_cure_blind(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
          return eSUCCESS;
      }

    if( vict==ch->fighting && number(0,1)==0 )
    {
	act("$n utters the words 'Haduken!'.", ch, 0, 0, 
	  TO_ROOM, INVIS_NULL);
	return cast_hellstream(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }
    else 
    {
        act("$n utters the words 'Let's go girls! Feel the burn!'.", ch, 0, 0,
          TO_ROOM, INVIS_NULL);
        return cast_acid_blast(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }
    return eFAILURE;
}

int alakarbodyguard(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   return protect(ch, 17603);
}

int paladin(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *vict;
    struct char_data *other_victs;

    int enemycount = 0;

    if(cmd) return eFAILURE;

    if (GET_POS(ch) < POSITION_FIGHTING) return eFAILURE;
    if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;
    if(MOB_WAIT_STATE(ch)) return eFAILURE;

    if (!(vict = ch->fighting))
       return eFAILURE;

    /* count bad guys (in case we wanna earthquake) */
    for(other_victs = world[ch->in_room].people; other_victs; other_victs = other_victs->next_in_room )
       if (!IS_NPC(other_victs))
          enemycount++;

    if(number(0,2)==0 )
    {
      MOB_WAIT_STATE(ch) = 3;
      return do_bash(ch, "", 9);
    }
    if(GET_LEVEL(ch) > 10 && enemycount > 1) {
       return cast_earthquake(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }
    if(GET_LEVEL(ch) > 47)
    {
       return cast_power_harm(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }
    else if(GET_LEVEL(ch) > 35)
    {
       return cast_harm(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }
    if(number(0,1)==0 )
    {
       MOB_WAIT_STATE(ch) = 2;
       return do_kick(ch, "", 9);
    }
   return eFAILURE;
}

int paladin_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    // struct char_data *vict;
    // int percent;
    if(cmd) return eFAILURE;
 
    if (GET_POS(ch) <= POSITION_FIGHTING) 
      return eFAILURE;

    if(GET_HIT(ch) < GET_MAX_HIT(ch) && number(1,3) == 1 && GET_LEVEL(ch) > 29) {
      act("$n utters the words 'power heal'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_power_heal(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }

    if(!IS_AFFECTED(ch,AFF_DETECT_INVISIBLE)) {
      act("$n utters the words 'ghost eye'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_detect_invisibility(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

    if(!affected_by_spell(ch, SPELL_STRENGTH) && GET_LEVEL(ch) > 24) {
      act("$n utters the words 'sampson's gift'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_strength(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

     return eFAILURE;
}

int antipaladin(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *vict;

    if(cmd) return eFAILURE;
    if (GET_POS(ch) < POSITION_FIGHTING) return eFAILURE;
    if(GET_POS(ch)!=POSITION_FIGHTING) return eFAILURE;
    if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;
    if(MOB_WAIT_STATE(ch)) return eFAILURE;

    vict = ch->fighting;
    if (!vict)
       return eFAILURE;

    if (vict==ch->fighting && GET_LEVEL(ch)>2 && number(0,1)==0 )
    {
       MOB_WAIT_STATE(ch) = 2;
       return do_kick(ch, "", 9);
    }
    if(GET_LEVEL(ch) > 47)
    {
       return cast_acid_blast(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }
    if(GET_LEVEL(ch) > 27)
    {
       return cast_fireball(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }
    if(GET_LEVEL(ch) > 14)
    {
       return cast_vampiric_touch(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, vict, 0, GET_LEVEL(ch));
    }
   return eFAILURE;
}

int antipaladin_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,      
            struct char_data *owner)
{
    // struct char_data *vict;
    // int percent;
    if(cmd) return eFAILURE;
 
    if (GET_POS(ch) <= POSITION_FIGHTING) 
      return eFAILURE;

    if(!IS_AFFECTED(ch,AFF_DETECT_INVISIBLE) && GET_LEVEL(ch) > 11) {
      act("$n utters the words 'ghost eye'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_detect_invisibility(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

    if(!affected_by_spell(ch, SPELL_SHIELD) && GET_LEVEL(ch) > 19) {
      act("$n utters the words 'pongun'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_shield(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

    if(!affected_by_spell(ch, SPELL_STONE_SKIN) && GET_LEVEL(ch) > 46) {
      act("$n utters the words 'teri hatcher'.", ch, 0, 0, TO_ROOM, INVIS_NULL);
      cast_stone_skin(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch, 0, GET_LEVEL(ch));
      return eSUCCESS;
    }      

     return eFAILURE;
}

int thief(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *vict;

    if(cmd) return eFAILURE;
    if (GET_POS(ch) < POSITION_FIGHTING) return eFAILURE;
    if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;
    if(MOB_WAIT_STATE(ch)) return eFAILURE;

    vict = ch->fighting;
    if (!vict)
       return eFAILURE;

   // TODO - circle if i'm not the tank
   // TODO - i'm i'm smart, flee if I'm low on hps
   // TODO - disarm
   // TODO - trip

   if(number(0, 1)) {
     MOB_WAIT_STATE(ch) = 2;
     return do_trip(ch, "", 9);
   }

   return eFAILURE;
}

int thief_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   char_data * vict = NULL;

   if (cmd) return eFAILURE;
   if (GET_POS(ch) <= POSITION_FIGHTING) return eFAILURE;
   if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;

   if(ch->mobdata->hatred && (vict = get_char_room_vis(ch, get_random_hate(ch)))
      && ch->equipment[WIELD]
      && ( ch->equipment[WIELD]->obj_flags.value[3] == 11 ||
           ch->equipment[WIELD]->obj_flags.value[3] == 8 
         )
      && (!ch->fighting) && (!vict->fighting)
     ) 
   {
      return do_backstab(ch, GET_NAME(vict), 9);
   }

   if(!IS_AFFECTED(ch, AFF_HIDE)) {
      do_hide(ch, "", 9);
      return eSUCCESS;
   }

   // TODO - steal items/gold from players?
   // TODO - palm items from ground?
   // TODO - stalk people i hate?

   return eFAILURE;
}

int barbarian(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
    struct char_data *vict;

    if(cmd) return eFAILURE;
    if (GET_POS(ch) < POSITION_FIGHTING) return eFAILURE; 
    if (IS_AFFECTED(ch, AFF_PARALYSIS)) return eFAILURE;
    if(MOB_WAIT_STATE(ch)) return eFAILURE;

    vict = ch->fighting;
    if (!vict)
       return eFAILURE;

    if(GET_LEVEL(ch) > 15)
    {
      MOB_WAIT_STATE(ch) = 4;
      return do_rage(ch, "", 0);
    }

   // TODO - berserk
   // TODO - headbutt

   return eSUCCESS;
}

int barbarian_non_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
          struct char_data *owner)
{
   // TODO - not much a barb can do...maybe try to sleep to regen faster?

   return eFAILURE;
}

static char *firstQuestText [ ] =
{
// First is what I say all the time NULL if none
   "Where the hell did I put my mithril axe?",
// What I say if they give me the item
   "Oh, hells yeah.  Here, take this.",
// What I say if they give me something else
   "This isn't my axe, but I'll keep it, thanks.",
// Keyword 
   "mithril axe",
// Response
   "It's my axe made of mithril.  If you can find it, i'll reward you.",
// Additional pairs would go here

// END with NULL
NULL

};

int mithaxequest(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  char item[200];
  char target[200];
  int iItemId = 10107; // mithril axe
  int iRewardId = 28313; // dead fish

  struct char_data * mob = NULL;
  extern struct index_data *mob_index;

  // find myself - this might not work if there are more than one mob
  // with the same proc in room, but c'est la vie.  Best I can do.
  for(mob = world[ch->in_room].people; mob; mob = mob->next_in_room)
     if(IS_NPC(mob) && mob_index[mob->mobdata->nr].non_combat_func == mithaxequest)
        break;

  if(!mob) // oh crap....
     return eFAILURE;

  if(cmd == 88) // give
  {
    // check the give to make sure it's for the mob
    half_chop(arg, item, target);
    if (!(mob == get_char_room_vis(ch, target))) {
         return eFAILURE;
    }

    // check if it's right
    if ((obj = get_obj_in_list_vis(ch, item, ch->carrying)))
      if(IS_SET(do_give(ch, arg, 9), eSUCCESS))
        if(obj->item_number == real_object(iItemId))
        {
          do_say(mob, firstQuestText[1], 9);
 
          // remove item from game
          extract_obj(obj);

          // reward PC
          obj = clone_object(real_object(iRewardId));
          one_argument(obj->name, item);
          sprintf(item, "%s %s", item, GET_NAME(ch));
          obj_to_char(obj, mob);
          do_give(mob, item, 9);
          return eSUCCESS;
        }
        else {
          // wrong item
          do_say(mob, firstQuestText[2], 9);
          return eSUCCESS;
        }
      else return eSUCCESS; // do_give
    else return eFAILURE; // obj = get_obj
  }

  if(cmd == 11) // do_say
  {
    // is one of the keyword pairs in it?
    for(int i = 3; firstQuestText[i] ; i += 2)
      if(strstr(arg, firstQuestText[i])) // found the key!
      {
        do_say(ch, arg, 9);
        do_say(mob, firstQuestText[i+1], 9);
        return eSUCCESS;
      } 
    return eFAILURE;
  }

  if(cmd)
     return eFAILURE;

  // not a command
  if(number(1, 4) == 1)
  {
    if(firstQuestText[0])
      do_say(ch, firstQuestText[0], 9);
    return eSUCCESS;
  }
  return eFAILURE;
}



static char *turtle_greenSayText [] = {
  "Blue lettuce?  Yum.",
  "Turtles are so cute.",
  "Blue hair?  It's SO sexy!",
  "Mew?",
  "Pika?"
};

#define TURTLE_GREEN_TEXT_SIZE ( sizeof(turtle_greenSayText) / sizeof(char*))

int turtle_green(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,        
          struct char_data *owner)
{
   int x;
   if (IS_AFFECTED(ch,AFF_PARALYSIS))
     return eFAILURE;
   if (cmd)
     return eFAILURE;
   x = number (0,TURTLE_GREEN_TEXT_SIZE * 6);
   if((unsigned) x < TURTLE_GREEN_TEXT_SIZE) {
     do_say(ch, turtle_greenSayText [ x ], 9);
     return eSUCCESS;
   }
   return eFAILURE;
}

int turtle_green_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
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
 
  
    do_say(ch, "Pokeball Fire Attack!", 9); 
    act("a small green turtle ears back its arm and throws a ball at $n!",
            ch->fighting, 0, 0, TO_ROOM, 0);
    act("a small green turtle ears back its arm and throws a ball at you!",
            ch->fighting, 0, 0, TO_CHAR, 0);
    return cast_fire_breath(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL,
            ch->fighting, 0, GET_LEVEL(ch));
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

// never actually used....basically was a guard that walked around the museum
// watching for trouble
int museum_guard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
   int retval;

   if(-1 == patrol(ch))
     return eSUCCESS;
 
   retval = protect(ch, 9700);
   if(SOMEONE_DIED(retval) || IS_SET(retval, eSUCCESS))
     return retval;
   retval = protect(ch, 9701);
   if(SOMEONE_DIED(retval) || IS_SET(retval, eSUCCESS))
     return retval;
   retval = protect(ch, 9704);
   if(SOMEONE_DIED(retval) || IS_SET(retval, eSUCCESS))
     return retval;
   retval = protect(ch, 9705);
   if(SOMEONE_DIED(retval) || IS_SET(retval, eSUCCESS))
     return retval;
   retval = protect(ch, 9710);
   if(SOMEONE_DIED(retval) || IS_SET(retval, eSUCCESS))
     return retval;
   retval = protect(ch, 9711);
   if(SOMEONE_DIED(retval) || IS_SET(retval, eSUCCESS))
     return retval;

   return eFAILURE;
}

int mage_familiar(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  if(number(0, 1))
    return cast_fireball(GET_LEVEL(ch), ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, GET_LEVEL(ch));
  
  return eFAILURE;  
}

int mage_familiar_non(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
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

  if(!ch->fighting) 
  {
    if(ch->master && ch->master->fighting) { // help him!
      do_join(ch, GET_NAME(ch->master), 9);
      return eFAILURE;
    }

    if(ch->in_room != ch->master->in_room) {
      do_emote(ch, "looks around for it's master than *eep*'s and dissolves into a shadow.\r\n", 9);
      move_char(ch, ch->master->in_room);
      do_emote(ch, "steps out of a nearby shadow relieved to be back in it's master's presence.\r\n", 9);
      return eFAILURE;
    }

    if(number(1, 500) == 1) {
      do_emote(ch, "chitters about for a bit then settles down.", 9);
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


int generic_area_guard(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
          struct char_data *owner)
{
   char_data * vict;
   char buf[80];

   int attempt_move(CHAR_DATA *ch, int cmd, int is_retreat = 0);

   if(cmd)
      return eFAILURE;

   // search for combat in room i'm in
   for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      if(IS_MOB(vict) && vict->fighting)
         break;

   if(vict)
   {
      switch(ch->mobdata->nr) {
         case 19304:
            do_say(ch, "HEY! Don't be picking on elementals!", 9);
            break;
         default:
            break;
      }

      sprintf(buf, "%d", vict->mobdata->nr);
      return do_join(ch, buf, 9);
   }

   // search for combat in surrounding rooms.  If we find some, go there
   for(int i = 0; i < 6; i++)
      if(CAN_GO(ch, i))
        for(vict = world[world[ch->in_room].dir_option[i]->to_room].people; vict; vict = vict->next_in_room)
            if(vict->fighting)
              return attempt_move( ch, ++i );

   return eFAILURE;
}
