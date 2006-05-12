/*
 * sing.c - implementation of bard songs
 * Pirahna
 *
 */
/**************************************************************************
 *  Revision History                                                      *
 *  10/23/2003  Onager  Changed do_sing() to allow non-bard imms to sing  *
 **************************************************************************/

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <sing.h>
#include <room.h>
#include <character.h>
#include <spells.h> // tar_char..
#include <levels.h>
#include <race.h>
#include <utility.h>
#include <player.h>
#include <interp.h>
#include <mobile.h>
#include <fight.h>
#include <handler.h>
#include <connect.h>
#include <act.h>
#include <db.h>
#include <magic.h> // dispel_magic
#include <innate.h> // SKILL_INNATE_EVASION
#include <returnvals.h>
extern CWorld world;
extern index_data *mob_index;
char_data *origsing = NULL;

 
extern pulse_data *bard_list;
extern CHAR_DATA *character_list;
void remove_memory(CHAR_DATA *ch, char type);
void add_memory(CHAR_DATA *ch, char *victim, char type);
extern bool check_social( CHAR_DATA *ch, char *pcomm, int length, char *arg );
void check_eq(CHAR_DATA *ch);

//        ubyte beats;     /* Waiting time after ki */
//        ubyte minimum_position; /* min position for use */
//        ubyte min_useski;       /* minimum ki used */
//        int16 skill_num;       /* skill number of the song */
//        int16 targets;         /* Legal targets */
//        SING_FUN *song_pointer; /* function to call */
//        SING_FUN *exec_pointer; /* other function to call */
//        SING_FUN *song_pulse;    /* other other function to call */
//        SING_FUN *intrp_pointer; /* other other function to call */
//	  int difficulty
struct song_info_type song_info [ ] = {

{ /* 0 */
        1, POSITION_RESTING, 0, SKILL_SONG_LIST_SONGS,
        TAR_IGNORE, song_listsongs, NULL, NULL, NULL,
	SKILL_INCREASE_EASY
},

{ /* 1 */
	1, POSITION_FIGHTING, 1, SKILL_SONG_WHISTLE_SHARP, 
	TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, 
	song_whistle_sharp, NULL, NULL, NULL,
	SKILL_INCREASE_MEDIUM
},

{ /* 2 */
	0, POSITION_RESTING, 0, SKILL_SONG_STOP, 
        TAR_IGNORE, 
        song_stop, NULL, NULL, NULL, SKILL_INCREASE_EASY
},

{ /* 3 */
	10, POSITION_RESTING, 2, SKILL_SONG_TRAVELING_MARCH, 
        TAR_IGNORE, 
        song_traveling_march, execute_song_traveling_march, NULL, NULL,
        SKILL_INCREASE_EASY
},

{ /* 4 */
	10, POSITION_RESTING, 9, SKILL_SONG_BOUNT_SONNET, 
        TAR_IGNORE, 
        song_bountiful_sonnet, execute_song_bountiful_sonnet,
	NULL, NULL, SKILL_INCREASE_EASY
},

{ /* 5 */
	5, POSITION_FIGHTING, 9, SKILL_SONG_INSANE_CHANT, 
        TAR_IGNORE, 
        song_insane_chant, execute_song_insane_chant,
	NULL, NULL, SKILL_INCREASE_MEDIUM
},

{ /* 7 */
        4, POSITION_RESTING, 5, SKILL_SONG_GLITTER_DUST, 
        TAR_IGNORE,
        song_glitter_dust, execute_song_glitter_dust,
        NULL, NULL, SKILL_INCREASE_HARD
},

{ /* 8 */
	6, POSITION_RESTING, 2, SKILL_SONG_SYNC_CHORD, 
	TAR_CHAR_ROOM|TAR_FIGHT_VICT, 
	song_synchronous_chord, execute_song_synchronous_chord, NULL, 
NULL,   SKILL_INCREASE_MEDIUM
},

{ /* 9 */
	10, POSITION_RESTING, 2, SKILL_SONG_HEALING_MELODY, 
	TAR_IGNORE, 
	song_healing_melody, execute_song_healing_melody, NULL, NULL,
        SKILL_INCREASE_MEDIUM
},

{ /* 10 */
        3, POSITION_SITTING, 7, SKILL_SONG_STICKY_LULL, 
        TAR_CHAR_ROOM|TAR_FIGHT_VICT,
        song_sticky_lullaby, execute_song_sticky_lullaby, NULL, NULL,
        SKILL_INCREASE_HARD
}, 

{ /* 11 */
	1, POSITION_RESTING, 1, SKILL_SONG_REVEAL_STACATO, 
	TAR_IGNORE, 
	song_revealing_stacato, execute_song_revealing_stacato, NULL, 
NULL,   SKILL_INCREASE_HARD
},

{ /* 12 */
	4, POSITION_RESTING, 3, SKILL_SONG_FLIGHT_OF_BEE, 
        TAR_IGNORE, 
        song_flight_of_bee, execute_song_flight_of_bee,
        pulse_flight_of_bee, intrp_flight_of_bee,
        SKILL_INCREASE_MEDIUM
},

{ /* 13 */
	5, POSITION_FIGHTING, 4, SKILL_SONG_JIG_OF_ALACRITY, 
        TAR_IGNORE, 
        song_jig_of_alacrity, execute_song_jig_of_alacrity,
        pulse_jig_of_alacrity, intrp_jig_of_alacrity,
        SKILL_INCREASE_HARD
},

{ /* 14 */
	7, POSITION_RESTING, 3, SKILL_SONG_NOTE_OF_KNOWLEDGE, 
	TAR_OBJ_INV, song_note_of_knowledge,
	execute_song_note_of_knowledge, NULL, NULL,
        SKILL_INCREASE_MEDIUM
},

{ /* 15 */
        2, POSITION_FIGHTING, 3, SKILL_SONG_TERRIBLE_CLEF, 
        TAR_IGNORE, song_terrible_clef, execute_song_terrible_clef, NULL,
        NULL,
        SKILL_INCREASE_MEDIUM
},

{ /* 16 */
	10, POSITION_RESTING, 4, SKILL_SONG_SOOTHING_REMEM, 
        TAR_IGNORE, 
        song_soothing_remembrance, execute_song_soothing_remembrance,
        NULL, NULL, SKILL_INCREASE_MEDIUM
},

{ /* 17 */
	10, POSITION_RESTING, 2, SKILL_SONG_FORGETFUL_RHYTHM, 
        TAR_CHAR_ROOM, 
        song_forgetful_rhythm, execute_song_forgetful_rhythm,
	NULL, NULL, SKILL_INCREASE_HARD
},

{ /* 18 */
	7, POSITION_RESTING, 4, SKILL_SONG_SEARCHING_SONG,
	TAR_CHAR_WORLD, song_searching_song,
	execute_song_searching_song, NULL, NULL, SKILL_INCREASE_HARD
},

{ /* 19 */
        4, POSITION_RESTING, 6, SKILL_SONG_VIGILANT_SIREN, 
        TAR_IGNORE, song_vigilant_siren, execute_song_vigilant_siren,
        pulse_vigilant_siren, intrp_vigilant_siren,
        SKILL_INCREASE_HARD
}, 

{ /* 20 */
	20, POSITION_RESTING, 10, SKILL_SONG_ASTRAL_CHANTY, 
        TAR_CHAR_WORLD, 
        song_astral_chanty, execute_song_astral_chanty,
	pulse_song_astral_chanty, NULL, SKILL_INCREASE_HARD
},

{ /* 21 */
	1, POSITION_FIGHTING, 8, SKILL_SONG_DISARMING_LIMERICK, 
        TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_SELF_NONO, 
        song_disrupt, NULL,
	NULL, NULL, SKILL_INCREASE_HARD
},

{ /* 22 */
	2, POSITION_FIGHTING, 6, SKILL_SONG_SHATTERING_RESO, 
        TAR_OBJ_ROOM, 
        song_shattering_resonance, execute_song_shattering_resonance,
	NULL, NULL, SKILL_INCREASE_HARD
},

{ /* 23 */
	8, POSITION_RESTING, 4, SKILL_SONG_UNRESIST_DITTY, 
        TAR_IGNORE, 
        song_unresistable_ditty, execute_song_unresistable_ditty,
	NULL, NULL, SKILL_INCREASE_MEDIUM
},
{ /* 24 */
        8, POSITION_RESTING, 4, SKILL_SONG_FANATICAL_FANFARE,
       TAR_IGNORE, song_fanatical_fanfare, execute_song_fanatical_fanfare,pulse_song_fanatical_fanfare,
       intrp_song_fanatical_fanfare, SKILL_INCREASE_MEDIUM

},
{ /* 25 */
        12, POSITION_FIGHTING, 7, SKILL_SONG_DISCHORDANT_DIRGE,
	        TAR_CHAR_ROOM|TAR_FIGHT_VICT,
        song_dischordant_dirge, execute_song_dischordant_dirge, NULL, 
NULL,    SKILL_INCREASE_HARD
},
{ /* 26 */
        2, POSITION_SITTING, 6, SKILL_SONG_CRUSHING_CRESCENDO, 
        TAR_IGNORE, song_crushing_crescendo, execute_song_crushing_crescendo,
        NULL, NULL, SKILL_INCREASE_HARD
}, 
{ /* 27 */
         15, POSITION_STANDING, 20, SKILL_SONG_HYPNOTIC_HARMONY,
	 TAR_CHAR_ROOM, song_hypnotic_harmony, 
         execute_song_hypnotic_harmony, NULL, NULL, SKILL_INCREASE_HARD
},
{ /* 28 */
         12, POSITION_SITTING, 6, SKILL_SONG_MKING_CHARGE,
         TAR_IGNORE, song_mking_charge, execute_mking_charge, pulse_mking_charge,
	intrp_mking_charge, SKILL_INCREASE_MEDIUM

},
{ /* 29 */
	7, POSITION_RESTING, 8, SKILL_SONG_SUBMARINERS_CHORUS, 
        TAR_IGNORE, 
        song_submariners_chorus, execute_song_submariners_chorus,
        pulse_submariners_chorus, intrp_submariners_chorus,
        SKILL_INCREASE_MEDIUM
}
};

char *songs[] = {
        "listsongs",
	"whistle sharp",
        "stop", /* If you move stop, update do_sing */
        "travelling march",
        "bountiful sonnet",
        "insane chant",
        "glitter dust",
        "synchronous chord",
	"healing melody",
        "sticky lullaby",
	"revealing staccato",
        "flight of the bumblebee",
        "jig of alacrity",
	"note of knowledge",
        "terrible clef",
        "soothing rememberance",
        "forgetful rhythm",
        "searching song",
        "vigilant siren",
        "astral chanty",
        "disarming limerick",
        "shattering resonance",
        "irresistable ditty",
	"fanatical fanfare",
        "dischordant dirge",
        "crushing crescendo",
        "hypnotic harmony",
	"mountain king's charge",
        "submariner's chorus",
	"\n"
};

void update_pos(CHAR_DATA *victim);
int16 use_song(CHAR_DATA *ch, int kn);
bool ARE_GROUPED(CHAR_DATA *sub, CHAR_DATA *obj);

int16 use_song(CHAR_DATA *ch, int kn)
{
	return(song_info[kn].min_useski);
}


void stop_grouped_bards(CHAR_DATA *ch, int action)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;
   if (action == 1)
     origsing = ch;

   if(!(master = ch->master))
      master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      // end any performances
      if(IS_SINGING(fvictim->follower))
         do_sing(fvictim->follower, "stop", 9);

   }
   if(IS_SINGING(master))
   {
      do_sing(master, "stop", 9);
   }
   origsing = NULL;
}

void get_instrument_bonus(char_data * ch, int & comb, int & non_comb)
{
   comb = 0;
   non_comb = 0;

   if(!ch->equipment[HOLD])                                     return;
   if(GET_ITEM_TYPE(ch->equipment[HOLD]) != ITEM_INSTRUMENT)    return;

   comb = ch->equipment[HOLD]->obj_flags.value[1];
   non_comb = ch->equipment[HOLD]->obj_flags.value[0];
}

int do_sing(CHAR_DATA *ch, char *arg, int cmd)
{
  CHAR_DATA *tar_char = 0;
  obj_data *tar_obj = 0;
  char name[MAX_STRING_LENGTH];
  char spellarg[MAX_STRING_LENGTH];
  char * argument = NULL;
  int qend, spl = -1;
  bool target_ok;
  int learned;
//  int specialization;

   if (!IS_NPC(ch) && GET_CLASS(ch) != CLASS_BARD && GET_LEVEL(ch) < IMMORTAL) {
      check_social(ch, "sing", 0, arg); // do the social:)
      return eSUCCESS;
   }

//   if ((IS_SET(world[ch->in_room].room_flags, SAFE)) && (GET_LEVEL(ch) < IMP)) {
//      send_to_char("For now, no songs can be sung in a safe room.\n\r", ch);
//      return eFAILURE;
//      }

  // we do this so we can pass constants to "do_sing" and no crash
  strcpy(spellarg, arg);
  argument = spellarg;

  argument = skip_spaces(argument);

  if(!(*argument)) {
    send_to_char("Yes, but WHAT would you like to sing?\n\r", ch);
    return eFAILURE;
  }

  if(*argument == '\'') // song is in 's
  {
    argument++;
    for(qend = 1; *(argument + qend) && (*(argument + qend) != '\'') ; qend++)
      *(argument+qend) = LOWER(*(argument + qend));
    if(*(argument+qend) != '\'')
    {
      send_to_char("If you start with a ' you have to end with a ' too.\r\n", ch);
      return eFAILURE;
    }
  }
  else
  {
    for(qend = 1; *(argument + qend) && (*(argument + qend) != ' ') ; qend++)
      *(argument+qend) = LOWER(*(argument + qend));
  }
  spl = old_search_block(argument, 0, qend, songs, 0);
  spl--;	 /* songs goes from 0+ not 1+ like spells */
  
  if(spl < 0) {
    send_to_char("You know not of that song.\n\r", ch);
    return eFAILURE;
  }
  if(spl != 2 && ch->song_timer > 0) {
    send_to_char("You are already in the middle of another song!\n\r", ch);
    return eFAILURE;
  }

  if ((IS_SET(world[ch->in_room].room_flags, SAFE)) && (GET_LEVEL(ch) < IMP) && (
        spl == SKILL_SONG_WHISTLE_SHARP - SKILL_SONG_BASE ||
        spl == SKILL_SONG_UNRESIST_DITTY - SKILL_SONG_BASE ||
        spl == SKILL_SONG_GLITTER_DUST - SKILL_SONG_BASE ||
        spl == SKILL_SONG_STICKY_LULL - SKILL_SONG_BASE ||
        spl == SKILL_SONG_REVEAL_STACATO - SKILL_SONG_BASE ||
        spl == SKILL_SONG_TERRIBLE_CLEF - SKILL_SONG_BASE ||
        spl == SKILL_SONG_DISCHORDANT_DIRGE - SKILL_SONG_BASE ||
        spl == SKILL_SONG_INSANE_CHANT - SKILL_SONG_BASE ||
        spl == SKILL_SONG_JIG_OF_ALACRITY - SKILL_SONG_BASE ||
        spl == SKILL_SONG_DISARMING_LIMERICK - SKILL_SONG_BASE ||
        spl == SKILL_SONG_CRUSHING_CRESCENDO - SKILL_SONG_BASE ||
        spl == SKILL_SONG_SHATTERING_RESO - SKILL_SONG_BASE ||
        spl == SKILL_SONG_MKING_CHARGE - SKILL_SONG_BASE ||
        spl == SKILL_SONG_HYPNOTIC_HARMONY - SKILL_SONG_BASE))        
  {
     send_to_char("This room feels too safe to sing an offensive song such as this.\n\r", ch);
     return eFAILURE;
  }  
  
  if(song_info[spl].song_pointer) {
    if(GET_POS(ch) < song_info[spl].minimum_position && 
	!IS_NPC(ch)) {
      switch(GET_POS(ch)) {
        case POSITION_SLEEPING:
          send_to_char("You dream of beautiful music.\n\r", ch);
          break;
        case POSITION_RESTING:
          send_to_char("You can't sing this resting!!\n\r", ch);
          break;
        case POSITION_SITTING:
          send_to_char("You can't do this sitting.  You must stand up.\n\r", ch);
          break;
        case POSITION_FIGHTING:
          send_to_char("This is a peaceful song.  Not for battle.\n\r", ch);
          break;
        default:
          send_to_char("It seems like you're in a pretty bad shape!\n\r", ch);
          break;
      }
      return eFAILURE;
    }
    else {
      if(GET_LEVEL(ch) < ARCHANGEL && spl != 0 && spl != 2)
        if(!(learned = has_skill(ch, song_info[spl].skill_num))) {
          if(IS_MOB(ch) && !ch->master)
            learned = 50;
          else 
          {
            send_to_char("You haven't learned that song.\n\r", ch);
            return eFAILURE;
          }
        }
    }
    argument += qend; /* Point to the space after the last ' */
    if(*argument == '\'') // they sang 'song with space'
       argument++;
    for(; *argument == ' '; argument++); /* skip spaces */

    /* Locate targets */
    target_ok = FALSE;
	
    if(!IS_SET(song_info[spl].targets, TAR_IGNORE)) {
      one_argument(argument, name);
      if(*name) {
        if(IS_SET(song_info[spl].targets, TAR_CHAR_ROOM))
          if((tar_char = get_char_room_vis(ch, name)) != NULL)
            target_ok = TRUE;

        if (!target_ok && IS_SET(song_info[spl].targets, TAR_CHAR_WORLD))
          if ( ( tar_char = get_char_vis(ch, name) ) != NULL )
            target_ok = TRUE;

        if (!target_ok && IS_SET(song_info[spl].targets, TAR_OBJ_INV))
          if ( ( tar_obj = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL )
            target_ok = TRUE;
  
        if (!target_ok && IS_SET(song_info[spl].targets, TAR_OBJ_ROOM))
        {
          tar_obj = get_obj_in_list_vis( ch, name, world[ch->in_room].contents );
          if ( tar_obj != NULL )
            target_ok = TRUE;
        }

        if (!target_ok && IS_SET(song_info[spl].targets, TAR_OBJ_EQUIP))
        {
          for(int i=0; i<MAX_WEAR && !target_ok; i++)
            if (ch->equipment[i] && str_cmp(name, ch->equipment[i]->name) == 0) {
              tar_obj = ch->equipment[i];
              target_ok = TRUE;
            }
        }

        if (!target_ok && IS_SET(song_info[spl].targets, TAR_OBJ_WORLD))
          if ( ( tar_obj = get_obj_vis(ch, name) ) != NULL )
            target_ok = TRUE;

	if(!target_ok && IS_SET(song_info[spl].targets, TAR_SELF_ONLY))
          if(str_cmp(GET_NAME(ch), name) == 0) {
            tar_char = ch;
            target_ok = TRUE;
          } // of !target_ok
      } // of *name
      
      /* No argument was typed */
      else if(!*name) {	
        if(IS_SET(song_info[spl].targets, TAR_FIGHT_VICT))
          if(ch->fighting)
            if((ch->fighting)->in_room == ch->in_room) {
              tar_char = ch->fighting;
              target_ok = TRUE;
            } 
            if(!target_ok && IS_SET(song_info[spl].targets, TAR_SELF_ONLY)) {
              tar_char = ch;
              target_ok = TRUE;
            }
      } // of !*name
      
      else
        target_ok = FALSE;
    }
    
    if(IS_SET(song_info[spl].targets, TAR_IGNORE))
      target_ok = TRUE;
	
    if(target_ok != TRUE) {
      if(*name)
      {
        if (IS_SET(song_info[spl].targets, TAR_CHAR_ROOM))
          send_to_char("Nobody here by that name.\n\r", ch);
        else if (IS_SET(song_info[spl].targets, TAR_CHAR_WORLD))
          send_to_char("Nobody playing by that name.\n\r", ch);
        else if (IS_SET(song_info[spl].targets, TAR_OBJ_INV))
          send_to_char("You are not carrying anything like that.\n\r", ch);  
        else if (IS_SET(song_info[spl].targets, TAR_OBJ_ROOM))
          send_to_char("Nothing here by that name.\n\r", ch);
        else if (IS_SET(song_info[spl].targets, TAR_OBJ_WORLD))
          send_to_char("Nothing at all by that name.\n\r", ch);
        else if (IS_SET(song_info[spl].targets, TAR_OBJ_EQUIP))
          send_to_char("You are not wearing anything like that.\n\r", ch);
        else if (IS_SET(song_info[spl].targets, TAR_OBJ_WORLD))
          send_to_char("Nothing at all by that name.\n\r", ch);
      }
      else /* No arguments were given */
        send_to_char("Whom should you sing to?\n\r", ch);
      return eFAILURE;
    }
    
    
    else if(target_ok) {
      if((tar_char == ch) && IS_SET(song_info[spl].targets, TAR_SELF_NONO)) {
        send_to_char("You cannot sing this to yourself!\n\r", ch);
        return eFAILURE;
      }
      else if((tar_char != ch) &&
              IS_SET(song_info[spl].targets, TAR_SELF_ONLY)) {
        send_to_char("You can only sing this song to yourself.\n\r", ch);
        return eFAILURE;
      }
      else if(IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tar_char)) {
        send_to_char("You are afraid that it might harm your master.\n\r", ch);
        return eFAILURE;
      }
    }

    if(!IS_SET(song_info[spl].targets, TAR_IGNORE)) 
     if(!tar_char && !tar_obj) {
       log("Dammit Pir, fix that null tar_char thing in do_song", IMP, LOG_BUG);
       send_to_char("If you triggered this message, you almost crashed the\n\r"
                    "game.  Tell a god what you did immediately.\n\r", ch);
       return eFAILURE|eINTERNAL_ERROR;
     }

    if (IS_SET(world[ch->in_room].room_flags, NO_KI)) {
      send_to_char("You find yourself unable to use energy based chants here.\n\r", ch);
      return eFAILURE;
    }

    if(GET_LEVEL(ch) < ARCHANGEL        &&
       !IS_MOB(ch)                      && 
       GET_KI(ch) < use_song(ch, spl)) 
    {
      send_to_char("You do not have enough ki!\n\r", ch);
      return eFAILURE;
    }

    // WAIT_STATE(ch, song_info[spl].beats);
    // Bards don't get a wait state for singing.  The songs take time
    // to go off, and 'beats' is how long it takes them.  Certain songs
    // DO give a wait state, but those songs apply the wait state internal
    // to the "do_song" code

    if((song_info[spl].song_pointer == NULL) && spl > 0) {
      send_to_char("Sorry, this power has not yet been implemented.\n\r", ch);
      return eFAILURE;
    }
    else {

      learned = has_skill(ch, song_info[spl].skill_num);
//      specialization = learned / 100;
//      learned %= 100;

      if(spl == SKILL_SONG_HYPNOTIC_HARMONY - SKILL_SONG_BASE ||
         spl == SKILL_SONG_DISARMING_LIMERICK - SKILL_SONG_BASE ||
         spl == SKILL_SONG_SHATTERING_RESO - SKILL_SONG_BASE ||
         spl == SKILL_SONG_SEARCHING_SONG - SKILL_SONG_BASE ||
         spl == SKILL_SONG_FANATICAL_FANFARE - SKILL_SONG_BASE ||
         spl == SKILL_SONG_MKING_CHARGE - SKILL_SONG_BASE ||
         spl == SKILL_SONG_VIGILANT_SIREN - SKILL_SONG_BASE)
      {
         if((!ch->equipment[HOLD] || GET_ITEM_TYPE(ch->equipment[HOLD]) != ITEM_INSTRUMENT) && (!ch->equipment[HOLD2] || GET_ITEM_TYPE(ch->equipment[HOLD2]) != ITEM_INSTRUMENT)) {
            send_to_char("You can't even begin this song without an instrument.\r\n", ch);
            return eFAILURE;
         }
      }

      if(spl != 2 && !skill_success(ch,tar_char, spl+SKILL_SONG_BASE)) {
        send_to_char("You forgot the words!\n\r", ch);
        GET_KI(ch) -= use_song(ch, spl)/2;
        return eSUCCESS;
      }

      /* Stop abusing your betters  */
     if(!IS_SET(song_info[spl].targets, TAR_IGNORE) && !tar_obj) 
     if (!IS_NPC(tar_char) && (GET_LEVEL(ch) > ARCHANGEL) 
          && (GET_LEVEL(tar_char) > GET_LEVEL(ch)))
      {
        send_to_char("That just might annoy them!\n\r", ch);
        return eFAILURE;
      }

      /* Imps ignore safe flags  */
     if(!IS_SET(song_info[spl].targets, TAR_IGNORE) && !tar_obj) 
     if (IS_SET(world[ch->in_room].room_flags, SAFE) && !IS_NPC(ch) 
          && (GET_LEVEL(ch) == IMP)) {
     send_to_char("There is no safe haven from an angry IMP!\n\r", tar_char);
     }

      if(ch->song_timer > 0) // I'm singing
      {
	if (!origsing) {
         send_to_char("You stop singing ", ch);
         send_to_char(songs[ch->song_number], ch);
         send_to_char(".\r\n", ch);
	}
        // If the song is a steady one, (like flight) than it needs to be
        // interrupted so we stop and remove the affects
        if((song_info[ch->song_number].intrp_pointer))
           ((*song_info[ch->song_number].intrp_pointer)(GET_LEVEL(ch),ch, NULL, NULL, learned));
        if(spl != 2 && !origsing) // song 'stop'
           ch->song_timer = 0;
      }
      if (origsing) return eSUCCESS;
//     Song messages are done in the actual song.  There is no 'standard'
//     entrance message since the songs aren't really alike anyway:)
//      send_to_char("You begin to sing...\n\r", ch);
//      act("$n raises $s voice in song...", ch, 0, 0, TO_ROOM, 0);

      ch->song_number = spl;
      GET_KI(ch) -= use_song(ch, spl);
      return ((*song_info[spl].song_pointer) (GET_LEVEL(ch), ch, argument, tar_char, learned));
    }
  }
  return eFAILURE;
}

// Go down the list of chars, and update song timers.  If the timer runs
// out, then activate the effect
void update_bard_singing()
{
  CHAR_DATA *i;
  //struct pulse_data * loop;

  for(i = character_list; i; i = i->next) 
  {
//    i = loop->thechar;
    if (!IS_NPC(i) && GET_CLASS(i) != CLASS_BARD && GET_LEVEL(i) < 100) continue;
    if(i->song_timer > 0) 
    {
      if(ISSET(i->affected_by, AFF_HIDE))
      {
        REMBIT(i->affected_by, AFF_HIDE);
        send_to_char("Your singing ruins your hiding place.\r\n", i);
      }
      if ((IS_SET(world[i->in_room].room_flags, SAFE)) && (GET_LEVEL(i) < IMP) && (
            i->song_number == SKILL_SONG_WHISTLE_SHARP - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_UNRESIST_DITTY - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_GLITTER_DUST - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_STICKY_LULL - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_REVEAL_STACATO - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_TERRIBLE_CLEF - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_DISCHORDANT_DIRGE - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_INSANE_CHANT - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_JIG_OF_ALACRITY - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_DISARMING_LIMERICK - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_CRUSHING_CRESCENDO - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_SHATTERING_RESO - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_MKING_CHARGE - SKILL_SONG_BASE ||
            i->song_number == SKILL_SONG_HYPNOTIC_HARMONY - SKILL_SONG_BASE))        
      {
         send_to_char("In this room, your song quiety fades away.\r\n", i);
         if((song_info[i->song_number].intrp_pointer))
            ((*song_info[i->song_number].intrp_pointer) (GET_LEVEL(i), i, NULL, NULL, -1));
         if(i->song_data) {
	    if ((int)i->song_data > 10) // Otherwise it's a temp variable.
              dc_free(i->song_data);
            i->song_data = 0;
         }
         return;
      }

      if(( (GET_POS(i) < song_info[i->song_number].minimum_position) && !IS_NPC(i))
         || IS_SET(i->combat, COMBAT_STUNNED)
         || IS_SET(i->combat, COMBAT_STUNNED2)
         || IS_SET(i->combat, COMBAT_SHOCKED) 
         || (IS_SET(i->combat, COMBAT_BASH1) || IS_SET(i->combat, COMBAT_BASH2)) &&
            (i->song_number == SKILL_SONG_TRAVELING_MARCH - SKILL_SONG_BASE ||
             i->song_number == SKILL_SONG_BOUNT_SONNET - SKILL_SONG_BASE ||
             i->song_number == SKILL_SONG_HEALING_MELODY - SKILL_SONG_BASE ||
             i->song_number == SKILL_SONG_SYNC_CHORD - SKILL_SONG_BASE ||
             i->song_number == SKILL_SONG_NOTE_OF_KNOWLEDGE - SKILL_SONG_BASE ||
             i->song_number == SKILL_SONG_SOOTHING_REMEM - SKILL_SONG_BASE ||
             i->song_number == SKILL_SONG_SEARCHING_SONG - SKILL_SONG_BASE ||
             i->song_number == SKILL_SONG_FORGETFUL_RHYTHM - SKILL_SONG_BASE) )
      {
           send_to_char("You can't keep singing in this position!\r\n", i); 
           i->song_timer = 0;
           if((song_info[i->song_number].intrp_pointer))
             ((*song_info[i->song_number].intrp_pointer) (GET_LEVEL(i), i, NULL, NULL, -1));
           if(i->song_data) {
              if ((int)i->song_data > 10) // Otherwise it's a temp variable.
                 dc_free(i->song_data);
              i->song_data = 0;
           }
           return;
      }

      if(i->song_number == SKILL_SONG_HYPNOTIC_HARMONY - SKILL_SONG_BASE ||
         i->song_number == SKILL_SONG_DISARMING_LIMERICK - SKILL_SONG_BASE ||
         i->song_number == SKILL_SONG_SHATTERING_RESO - SKILL_SONG_BASE ||
         i->song_number == SKILL_SONG_SEARCHING_SONG - SKILL_SONG_BASE ||
         i->song_number == SKILL_SONG_FANATICAL_FANFARE - SKILL_SONG_BASE ||
         i->song_number == SKILL_SONG_MKING_CHARGE - SKILL_SONG_BASE ||
         i->song_number == SKILL_SONG_VIGILANT_SIREN - SKILL_SONG_BASE)
      {
         if((!i->equipment[HOLD] || GET_ITEM_TYPE(i->equipment[HOLD]) != ITEM_INSTRUMENT) && (!i->equipment[HOLD2] || GET_ITEM_TYPE(i->equipment[HOLD2]) != ITEM_INSTRUMENT)) {
            send_to_char("Without an instrument, your song dies away.\r\n", i);
            if((song_info[i->song_number].intrp_pointer))
               ((*song_info[i->song_number].intrp_pointer) (GET_LEVEL(i), i, NULL, NULL, -1));
            i->song_timer = 0;
            if(i->song_data) {
	       if ((int)i->song_data > 10) // Otherwise it's a temp variable.
                  dc_free(i->song_data);
               i->song_data = 0;
            }
            return;
         }
      }
    }

    if(i->song_timer > 1)
    {
      i->song_timer--;

      if(IS_MOB(i) || !IS_SET(i->pcdata->toggles, PLR_BARD_SONG))
      {
         send_to_char("Singing [", i);
         send_to_char(songs[i->song_number], i);
         send_to_char("]: ", i);
         for(int j = 0; j < i->song_timer; j++)
           send_to_char("* ", i);
         send_to_char("\r\n", i);
      }
    }
    else if(i->song_timer == 1)
    {
      i->song_timer = 0;

//      if(GET_LEVEL(i) < IMMORTAL)
//        if(IS_SET(world[i->in_room].room_flags, SAFE)) {
//          send_to_char("No singing in safe rooms yet.\r\n", i);
//          if((song_info[i->song_number].intrp_pointer))
//            ((*song_info[i->song_number].intrp_pointer) (GET_LEVEL(i), i, NULL, NULL, -1));
//          if(i->song_data) {
//	    if ((int)i->song_data > 10) // Otherwise it's a temp variable.
//              dc_free(i->song_data);
//            i->song_data = 0;
//          }
//          return;
//        }

      int learned = has_skill(i, ( i->song_number + SKILL_SONG_BASE ) );

      if((song_info[i->song_number].exec_pointer))
        ((*song_info[i->song_number].exec_pointer) (GET_LEVEL(i), i, NULL, NULL, learned));
      else send_to_char("Bad exec pointer on the song you sang.  Tell a god.\r\n", i);
    }      
  }
}

int song_hypnotic_harmony(ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   if (!victim || !ch) {
      log("Serious problem in song_hypnotic_harmony!", ANGEL, LOG_BUG);
      return eFAILURE|eINTERNAL_ERROR;
      }
   act("$n sings an incredibly beautiful hymn, making you want to just give up your dayjob and follow $m around!",
       ch, 0, victim, TO_VICT, 0);
   act("$n sings an entrancing hymn to $N!",
       ch, 0, victim, TO_ROOM, NOTVICT);
   send_to_char("You sing your most enchanting hymn, hoping to attract some fans.\r\n", ch);
   ch->song_data = str_dup(arg);

   ch->song_timer = (song_info[ch->song_number].beats -
                              ( skill / 15 ) );

   return eSUCCESS;
}

int execute_song_hypnotic_harmony(ubyte level, CHAR_DATA *ch, char *Arg,
	CHAR_DATA *victim, int skill)
{
  struct affected_type af;
 
 
  if (!ch) {
      log("Serious problem in execute_song_hypnotic_harmony!", ANGEL, LOG_BUG);
      return eFAILURE|eINTERNAL_ERROR;
      }
   if(!(victim = get_char_room_vis(ch, ch->song_data)))
   {
   dc_free(ch->song_data);
   ch->song_data = 0;
 send_to_char("They seem to have left.\r\nIn the middle of your performance too!\r\n",ch); 
return eFAILURE;
  }
   dc_free(ch->song_data);
   ch->song_data = 0;

   WAIT_STATE(ch, PULSE_VIOLENCE);
   if (!IS_NPC(victim) || !ISSET(victim->mobdata->actflags, ACT_BARDCHARM))
   {
      send_to_char("They don't seem particularily interested.\r\n",ch);
      send_to_char("You manage to resist the entrancing lyrics.\r\n",victim);
      return eFAILURE;
   }

  if(circle_follow(victim, ch)) {
    send_to_char("Sorry, following in circles can not be allowed.\n\r", ch);
    return eFAILURE;
  }

int charm_levels(CHAR_DATA *ch);
int charm_space(int level);


 if(charm_levels(ch) - charm_space(GET_LEVEL(victim)) < 0)  {
     send_to_char("How you plan on controlling so many followers?\n\r", ch);
     return eFAILURE;
 }
/*
bool any_charms(CHAR_DATA *ch);

   if(any_charms(ch))  {
   CHAR_DATA * vict = NULL;
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
     }
   }   */
 if(victim->master)
    stop_follower(victim, 0);

  remove_memory(victim, 'h');

  add_follower(victim, ch, 0);

  af.type      = SPELL_CHARM_PERSON;
  af.duration  = 24 + ((level > 40) * 6) + ((level > 60) * 6) + ((level > 80) * 12) ;

  af.modifier  = 0;
  af.location  = 0;
  af.bitvector = AFF_CHARM;
  affect_to_char(victim, &af);

  /* remove any !charmie eq the charmie is wearing */
  check_eq(victim);

  act("You decide to follow $n's musical genius to the end.", ch , 0, victim, TO_VICT, 0);
  send_to_char("You succeed, and you find yourself having a new fan.\r\n",ch);
return eSUCCESS;

}

int song_disrupt( ubyte level, CHAR_DATA *ch, char 
*arg, CHAR_DATA *victim, int skill)
{
   if (!victim || !ch) {
      log("Serious problem in song_disrupt!", ANGEL, LOG_BUG);
      return eFAILURE|eINTERNAL_ERROR;
      }

   int learned = has_skill(ch, song_info[SKILL_SONG_DISARMING_LIMERICK].skill_num);

   act("$n sings a witty little limerick to you!\r\nYour laughing makes it hard to concentrate on keeping your spells up!", 
       ch, 0, victim, TO_VICT, 0);
   act("$n sings a hilarious limerick about a man from Nantucket to $N!",
       ch, 0, victim, TO_ROOM, NOTVICT);
   send_to_char("You sing your funniest limerick!\r\n", ch);
   
   WAIT_STATE(ch, PULSE_VIOLENCE);
   if (number(1,101) < get_saves(victim, SAVE_TYPE_MAGIC))
   {
act("$N resists your disarming limerick!", ch, NULL, victim, TO_CHAR,0);
act("$N resists $n's disarming limerick!", ch, NULL, victim, TO_ROOM,NOTVICT);
act("You resist $n's disarming limerick!",ch,NULL,victim,TO_VICT,0);
     return eFAILURE;
   }

   if(learned > 90) {
      if(IS_SET(victim->combat, COMBAT_REPELANCE)) {
         act("Your limerick disrupts $S magical barrier!", ch, 0, victim, TO_CHAR, 0);
         act("$n's limerick broke your concentration of your magical barrier!", ch, 0, victim, TO_VICT, 0);
         act("$N's concentration faultered from $n's gut-busting limerick!", ch, 0, victim, TO_ROOM, NOTVICT);
         REMOVE_BIT(victim->combat, COMBAT_REPELANCE);
         return eSUCCESS;
      }
   }
   if(learned > 85) {
      if(affected_by_spell(victim, KI_STANCE+KI_OFFSET)) {
         act("Your limerick breaks $S stance!", ch, 0, victim, TO_CHAR, 0);
         act("$n's limerick causes you to break your stance!", ch, 0, victim, TO_VICT, 0);
         act("$N's stance breaks down from $n's hilarious limerick!", ch, 0, victim, TO_ROOM, NOTVICT);
         affect_from_char(victim, KI_STANCE+KI_OFFSET);
         return eSUCCESS;
      }
   }

   return spell_dispel_magic(GET_LEVEL(ch)-1, ch, victim, 0, 0);
}

int song_whistle_sharp( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   int dam = 0;
   int retval;

   if (!victim) {
      log("No vict send to song whistle sharp!", ANGEL, LOG_BUG);
      return eFAILURE|eINTERNAL_ERROR;
      }

   if(!can_attack(ch) || !can_be_attacked(ch, victim))
     return eFAILURE;

   set_cantquit( ch, victim );

   int combat, non_combat;
   get_instrument_bonus(ch, combat, non_combat);

//   dam = GET_LEVEL(ch) + GET_INT(ch) + combat;
   dam = 80;
   act("You send a sharp piercing whistle at $N.", ch, 0, victim, TO_CHAR, 0);
   act("$n whistles a sharp tune that ravages your ear drums and pierces you to the bone!", 
       ch, 0, victim, TO_VICT, 0);
   act("$n whistles a super-high note at $N and blood drips from $S ears!",
       ch, 0, victim, TO_ROOM, NOTVICT);

   if (number(1,101) < get_saves(victim, SAVE_TYPE_MAGIC))
   {
act("$N resists your whistle sharp!", ch, NULL, victim, 
TO_CHAR,0);
act("$N resists $n's whistle sharp!", ch, NULL, victim, TO_ROOM,
NOTVICT);
act("You resist $n's whistle sharp!",ch,NULL,victim,TO_VICT,0);
	   dam/=2;
   }

  char buf[MAX_STRING_LENGTH];
  strcpy(buf, victim->short_desc);
    bool ispc = !IS_NPC(victim);

   retval = damage(ch, victim, dam, TYPE_SONG,SKILL_SONG_WHISTLE_SHARP, 0);
   if(IS_SET(retval, eCH_DIED))
      return retval;

   if(IS_SET(retval, eVICT_DIED))
   {
	char buf2[MAX_STRING_LENGTH];
      sprintf(buf2, "$n's whistle goes off the scale and %s's head explodes!", buf);

      act(buf2, ch, NULL, victim, TO_ROOM, NOTVICT);
      if (ispc)
        act("$n's whistle goes off the scale and your head explodes!", ch, NULL, victim, TO_VICT, 0);
      sprintf(buf2, "%s's head explodes as your whistle goes off the scale!", buf);
      act(buf2, ch, NULL, victim, TO_CHAR, 0);

      send_to_char("You dance a small jig on the corpse.\r\n", ch);
      act("$n dances a little jig on the fallen corpse.",
          ch, 0, victim, TO_ROOM, 0);
      return retval;
   }

   int wait = song_info[ch->song_number].beats - (skill / 10);
   wait = MAX(wait, 2);

   if (level != 51)
   WAIT_STATE(ch, wait);
   return eSUCCESS;
}

int song_healing_melody( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing a song of healing...\n\r", ch);
   act("$n raises $s voice in a soothing melody...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = (song_info[ch->song_number].beats -
                              ( skill / 15 ) );

   if(GET_LEVEL(ch) > MORTAL)
    ch->song_timer = 1;
   return eSUCCESS;
}

int execute_song_healing_melody( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   int heal;
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   int specialization = skill / 100;
   skill %= 100;

   heal = 3*(GET_LEVEL(ch)/5);

   int combat, non_combat;
   get_instrument_bonus(ch, combat, non_combat);

   heal += non_combat;

   if(specialization > 0)
     heal = (int)(heal * 1.5);

   if(ch->master && ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if(!ISSET(fvictim->follower->affected_by, AFF_GROUP) || 
         fvictim->follower->in_room != ch->in_room ||
         IS_UNDEAD(fvictim->follower))
         continue;

      send_to_char("You feel a little better.\r\n", fvictim->follower);
      GET_HIT(fvictim->follower) += number(1, heal);
      if(GET_HIT(fvictim->follower) > GET_MAX_HIT(fvictim->follower))
         GET_HIT(fvictim->follower) = GET_MAX_HIT(fvictim->follower);
   }
   if(ch->in_room == master->in_room && !IS_UNDEAD(master))
   {
      send_to_char("You feel a little better.\r\n", master);
      GET_HIT(master) += number(1, heal);
      if(GET_HIT(master) > GET_MAX_HIT(master))
         GET_HIT(master) = GET_MAX_HIT(master);
   }

   if(//number(1, 101) > ( 50 + skill/2 )
	!skill_success(ch, NULL, SKILL_SONG_HEALING_MELODY)  ) {
      send_to_char("You run out of lyrics and end the song.\r\n", ch);
      return eSUCCESS;
   }

   ch->song_timer = (song_info[ch->song_number].beats -
                              ( skill / 15 ) );

   if(GET_LEVEL(ch) > MORTAL)
    ch->song_timer = 1;
   return eSUCCESS;
}

int song_revealing_stacato( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing a song of revealing...\n\r", ch);
   act("$n begins to chant in rhythm...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int execute_song_revealing_stacato( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * i;
   struct room_data *room;
   char buf[MAX_STRING_LENGTH];
   char *direction[] =
   {
      "to the North",
      "to the East",
      "to the South",
      "to the West",
      "above you",
      "below you",
      "\n",
   };

   for (i = world[ch->in_room].people; i; i = i->next_in_room)
   {
      if(!ISSET(i->affected_by, AFF_HIDE) && !ISSET(i->affected_by, AFF_FOREST_MELD))
         continue;
      REMBIT(i->affected_by, AFF_HIDE);
	affect_from_char(i,AFF_FOREST_MELD);
//      REMBIT(i->affected_by, AFF_FOREST_MELD);
      if(i == ch)
      {
         act("$n continues $s singing...", ch, 0, 0, TO_ROOM, 0);
         send_to_char("Your singing ruins your hiding place.\r\n", ch);
      }
      else
      {
         act("$n's song makes you notice $N hiding over in the corner.",
              ch, 0, i, TO_ROOM, NOTVICT);
         act("Your song makes you notice $N hiding over in the corner.",
              ch, 0, i, TO_CHAR, 0);
      }
   }
 
   if(skill > 80) {
      for(int j=0;j<6;j++) {
         if(CAN_GO(ch, j)) {
            room = &world[world[ch->in_room].dir_option[j]->to_room];
            if(room == &world[ch->in_room])
               continue;
            for (i = room->people; i; i = i->next_in_room) {
               if(!ISSET(i->affected_by, AFF_HIDE) && !ISSET(i->affected_by, AFF_FOREST_MELD))
                  continue;
               REMBIT(i->affected_by, AFF_HIDE);
               affect_from_char(i,AFF_FOREST_MELD);
               sprintf(buf, "$n's song makes you notice $N hiding a little bit %s", direction[j]);
               act(buf, ch, 0, i, TO_ROOM, NOTVICT);
               sprintf(buf, "Your song makes you notice $N hiding a little bit %s", direction[j]);
               act(buf, ch, 0, i, TO_CHAR, 0);
            }
         }
      }
   }
   send_to_char("You tap your foot along to the revealing staccato.\r\n", ch);

   if( !skill_success(ch,NULL,  SKILL_SONG_REVEAL_STACATO)  ) {
      send_to_char("You run out of lyrics and end the song.\r\n", ch);
      return eSUCCESS;
   }

   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int song_note_of_knowledge( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   // store the obj name here, cause A, we don't pass tar_obj
   // and B, there's no place to save it before we execute
   ch->song_data = str_dup(arg);

   send_to_char("You begin to sing a long single note...\n\r", ch);
   act("$n sings a long solitary note.", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;   
}

int execute_song_note_of_knowledge( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   obj_data * obj = NULL;
   char_data * vict = NULL;
   obj_data * corpse = NULL;
   char buf[MAX_STRING_LENGTH];

   obj = get_obj_in_list(ch->song_data, ch->carrying);
   vict = get_char_room_vis(ch, ch->song_data);
   corpse = get_obj_in_list_vis(ch, ch->song_data, world[ch->in_room].contents);
   if(corpse && (GET_ITEM_TYPE(corpse) != ITEM_CONTAINER || corpse->obj_flags.value[3] != 1))
      corpse = NULL;
   int learned = has_skill(ch, song_info[SKILL_SONG_NOTE_OF_KNOWLEDGE].skill_num);

   dc_free(ch->song_data);
   ch->song_data = 0;

   if(obj) {
      spell_identify(GET_LEVEL(ch), ch, 0, obj, 0);
   }
   else if(learned > 80 && corpse) {
      sprintf(buf, "Corpse '%s'\n\r", corpse->name);
      send_to_char(buf, ch);
      spell_identify(GET_LEVEL(ch), ch, 0, corpse, 0);
   }
   else if(learned > 85 && vict) {
      spell_identify(GET_LEVEL(ch), ch, vict, 0, 0);
   }
   else send_to_char("You can't seem to find that item.\r\n", ch);
   return eSUCCESS;
}

int song_terrible_clef( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin a song of battle!\n\r", ch);
   act("$n sings a horrible battle hymn!", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int execute_song_terrible_clef( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   int dam = 0;
   int retval;

   //int specialization = skill / 100;
   skill %= 100;

   victim = ch->fighting;

   if(!victim)
   {
      send_to_char("Your song fades outside of battle.\r\n", ch);
      return eSUCCESS;
   }

   int combat, non_combat;
   get_instrument_bonus(ch, combat, non_combat);

//   dam = GET_LEVEL(ch) * 4 + GET_WIS(ch) * 2 + combat*2;
   dam = 300;
   send_to_char("Your singing hurts your opponent!\r\n", ch);
   act("$n's singing causes pain in $N's ears!\r\n", ch, 0, victim, TO_ROOM, NOTVICT);
   send_to_char("The music!  It hurts!  It hurts!\r\n", victim);
   if (number(1,101) < get_saves(victim, SAVE_TYPE_MAGIC))
   {
act("$N resists your terrible clef!", ch, NULL, victim, 
TO_CHAR,0);
act("$N resists $n's terrible clef!", ch, NULL, victim, TO_ROOM,
NOTVICT);
act("You resist $n's terrible clef!",ch,NULL,victim,TO_VICT,0);
    dam /= 2;
   }
   bool ispc = !IS_NPC(victim);
   char buf[MAX_STRING_LENGTH];
   strcpy(buf, victim->short_desc);
   retval = damage(ch, victim, dam, TYPE_SONG,SKILL_SONG_TERRIBLE_CLEF, 0);
   if(IS_SET(retval, eCH_DIED))
     return retval;
   if(IS_SET(retval, eVICT_DIED))
   {
      char buf2[MAX_STRING_LENGTH];
	sprintf(buf2, "$n's terrible clef bangs against %s until they are no more!", buf);
      act(buf2, ch, NULL, NULL, TO_ROOM, NOTVICT);
      if (ispc)
        act("$n's terrible clef has beaten you to a pulp!", ch, NULL, victim, TO_VICT, 0);
      sprintf(buf2, "The terrible clef has left %s broken, bloody, and dead!", buf);
      act(buf2, ch, NULL, victim, TO_CHAR, 0);

      send_to_char("You dance a small jig on the corpse.\r\n", ch);
      act("$n dances a little jig on the fallen corpse.",
          ch, 0, victim, TO_ROOM, 0);
	return retval;
   }

  if(!skill_success(ch, victim, SKILL_SONG_TERRIBLE_CLEF)  ) {
      send_to_char("You run out of lyrics and end the song.\r\n", ch);
      return eSUCCESS;
   }

   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int song_listsongs( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char buf[200];

   send_to_char("Available Songs\n\r---------------\r\n", ch);
   for(int i = 0; *songs[i] != '\n'; i++)
   {
      if(GET_LEVEL(ch) < IMMORTAL && !has_skill(ch, song_info[i].skill_num))
        continue;

      sprintf(buf, " %-50s    %d ki\r\n", songs[i], song_info[i].min_useski);
      send_to_char(buf, ch);
   }
   return eSUCCESS;
}

int song_soothing_remembrance( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing a song of rememberance...\n\r", ch);
   act("$n raises $s voice in a soothing ballad...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats - skill/18;
   return eSUCCESS;
}

int execute_song_soothing_remembrance( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   int heal;
   char_data * master = NULL;
   follow_type * fvictim = NULL;
   int specialization = skill / 100;
   skill %= 100;

   heal = GET_LEVEL(ch)/5;

   int combat, non_combat;
   get_instrument_bonus(ch, combat, non_combat);

   heal += non_combat;

   if(specialization > 0)
      heal = (int) (heal * 1.5);

   if(ch->master && ch->master->in_room == ch->in_room && 
                    ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if(!ISSET(fvictim->follower->affected_by, AFF_GROUP) || 
         fvictim->follower->in_room != ch->in_room)
         continue;

      send_to_char("You feel soothed.\r\n", fvictim->follower);
      GET_MANA(fvictim->follower) += number(1, heal);
      if(GET_MANA(fvictim->follower) > GET_MAX_MANA(fvictim->follower))
         GET_MANA(fvictim->follower) = GET_MAX_MANA(fvictim->follower);
   }
   if(ch->in_room == master->in_room)
   {
      send_to_char("You feel soothed.\r\n", master);
      GET_MANA(master) += number(1, heal);
      if(GET_MANA(master) > GET_MAX_MANA(master))
         GET_MANA(master) = GET_MAX_MANA(master);
   }

  if(//number(1, 101) > ( 50 + skill/2 )
        !skill_success(ch, NULL, SKILL_SONG_SOOTHING_REMEM)  ) {
      send_to_char("You run out of lyrics and end the song.\r\n", ch);
      return eSUCCESS;
   }

   ch->song_timer = song_info[ch->song_number].beats - skill/18;
   return eSUCCESS;
}

int song_traveling_march( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing a song of travel...\n\r", ch);
   act("$n raises $s voice in an uplifting march...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats -
                             (skill / 15);

   if(GET_LEVEL(ch) > MORTAL)
    ch->song_timer = 1;
   return eSUCCESS;
}

int execute_song_traveling_march( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   int heal;
   char_data * master = NULL;
   follow_type * fvictim = NULL;
   int specialization = skill / 100;
   skill %= 100;

   heal = ((GET_LEVEL(ch)/3)+1)*2;

   int combat, non_combat;
   get_instrument_bonus(ch, combat, non_combat);

   heal += non_combat;

   if(specialization > 0)
      heal = (int) (heal * 1.5);

   if(ch->master && ch->master->in_room == ch->in_room && 
                    ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if(!ISSET(fvictim->follower->affected_by, AFF_GROUP) || 
         fvictim->follower->in_room != ch->in_room)
         continue;

      send_to_char("Your feet feel lighter.\r\n", fvictim->follower);
      GET_MOVE(fvictim->follower) += number(1, heal);
      if(GET_MOVE(fvictim->follower) > GET_MAX_MOVE(fvictim->follower))
         GET_MOVE(fvictim->follower) = GET_MAX_MOVE(fvictim->follower);
   }
   if(ch->in_room == master->in_room)
   {
      send_to_char("Your feet feel lighter.\r\n", master);
      GET_MOVE(master) += number(1, heal);
      if(GET_MOVE(master) > GET_MAX_MOVE(master))
         GET_MOVE(master) = GET_MAX_MOVE(master);
   }

  if(//number(1, 101) > ( 50 + skill/2 )
        !skill_success(ch, NULL, SKILL_SONG_TRAVELING_MARCH)  ) {
      send_to_char("You run out of lyrics and end the song.\r\n", ch);
      return eSUCCESS;
   }

   ch->song_timer = song_info[ch->song_number].beats -
                             (skill / 15);

   if(GET_LEVEL(ch) > MORTAL)
    ch->song_timer = 1;
   return eSUCCESS;
}

int song_stop( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   if (origsing) return eFAILURE;
   if(!ch->song_timer)
   {
      send_to_char("Might wanna start the performance first...Hope this isn't indicative of your love life...\r\n", ch);
      return eFAILURE;
   }
   if(ch->song_data) {
	if ((int)ch->song_data > 10)
	      dc_free(ch->song_data);
      ch->song_data = 0;
   }
 
   send_to_char("You finish off your song with a flourish...\n\r", ch);
   act("$n finishes $s song in a flourish and a bow.", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = 0;
   return eSUCCESS;
}


int song_astral_chanty( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
  // Store it for later, since we can't store the vict pointer
  ch->song_data = str_dup(arg);

  send_to_char("You begin to sing an astral chanty...\n\r", ch);
  act("$n starts quietly in a sea chanty...", ch, 0, 0, TO_ROOM, 0);

  ch->song_timer = song_info[ch->song_number].beats;

  return eSUCCESS;
}


void do_astral_chanty_movement(CHAR_DATA *victim, CHAR_DATA *target)
{
  int retval;

  retval = move_char(victim, target->in_room);

  if(!IS_SET(retval, eSUCCESS)) {
    send_to_char("Mystic winds shock you back into your old reality.\r\n", victim);
    act("$n shudders as magical reality refuses to set in.", victim, 0, 0, TO_ROOM, 0);
    WAIT_STATE(victim, PULSE_VIOLENCE * 3);
    return;
    }

  do_look(victim, "", 9);
  WAIT_STATE(victim, PULSE_VIOLENCE*2);
  act("$n appears out of nowhere in a chorus of light and song.", victim, 0, 0, TO_ROOM, 0);
}


int execute_song_astral_chanty( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
  char_data * master = NULL;
  follow_type * fvictim = NULL;

  if(ch->master && ch->master->in_room == ch->in_room &&
    ISSET(ch->affected_by, AFF_GROUP))
    master = ch->master;
  else
    master = ch;

  victim = get_char(ch->song_data);

  if (!victim) {
    if(ch->song_data) {
      dc_free(ch->song_data);
      ch->song_data = 0;
      }
    send_to_char("You can't recall the right words.\r\n", ch);
    return eFAILURE;
    }

  if (GET_LEVEL(victim) > GET_LEVEL(ch)) {
    send_to_char("Your target resists the song's draw.\r\n", ch);
    if(ch->song_data) {
      dc_free(ch->song_data);
      ch->song_data = 0;
      }
    return eFAILURE;
    }

  if(IS_SET(world[victim->in_room].room_flags, NO_PORTAL) ||
    (IS_SET(world[victim->in_room].room_flags, ARENA) && !IS_SET(world[ch->in_room].room_flags, ARENA)) ||
    (IS_SET(world[ch->in_room].room_flags, ARENA) && !IS_SET(world[victim->in_room].room_flags, ARENA))
//  ||    (IS_AFFECTED(victim, AFF_SHADOWSLIP))
    )
    send_to_char("A mystical force seems to be keeping you out.\r\n", ch);
  else {
    // Move the group, NOT the leader, NOT the bard
    for (fvictim = master->followers; fvictim; fvictim = fvictim->next) {
      if (!ISSET(fvictim->follower->affected_by, AFF_GROUP) ||
        fvictim->follower == ch ||
        fvictim->follower->in_room != ch->in_room)
        continue;
      do_astral_chanty_movement(fvictim->follower, victim);
      }

    // Move just the group leader, but NEVER the bard
    if (ch != master && (ISSET(master->affected_by, AFF_GROUP) &&
      master->in_room == ch->in_room)) {
      do_astral_chanty_movement(master, victim);
      }

    // Move the bard's charm
    for(struct follow_type *k = ch->followers; k; k = k->next)
      if(IS_MOB(k->follower) && affected_by_spell(k->follower, SPELL_CHARM_PERSON) &&
        k->follower->in_room == ch->in_room)
        do_astral_chanty_movement(k->follower, victim);

    send_to_char("Your song completes, and your vision fades.\r\n", ch);
    act("$n's voice fades off into the ether.", ch, 0, 0, TO_ROOM, 0);

    // Move the bard after the charm because of the same_room check
    do_astral_chanty_movement(ch, victim);
    }

  // free our stored char name
  if(ch->song_data) {
    dc_free(ch->song_data);
    ch->song_data = 0;
    }

  return eSUCCESS;
}


int pulse_song_astral_chanty( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
  if (number(1, 3) == 3)
    act("$n sings a rousing chanty!", ch, 0, 0, TO_ROOM, 0);

  return eSUCCESS;
}


int song_forgetful_rhythm( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   // store the arg here, cause there's no place to save it before we execute
   ch->song_data = str_dup(arg);

   send_to_char("You begin to sing a song of forgetfulness...\n\r", ch);
   act("$n begins an entrancing rhythm...", ch, 0, 0, TO_ROOM, 0);   
   ch->song_timer = song_info[ch->song_number].beats -  (int) (has_skill(ch, song_info[ch->song_number].skill_num) / 15);
   return eSUCCESS;
}

int execute_song_forgetful_rhythm( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   int retval;

   if(!(victim = get_char_room_vis(ch, ch->song_data)))
   {
      send_to_char("You don't see that person here.\r\n", ch);
      dc_free(ch->song_data);
      ch->song_data = 0;
      return eFAILURE;
   }
   dc_free(ch->song_data);
   ch->song_data = 0;

   act("$n sings to $N about beautiful rainbows.", ch, 0, victim, TO_ROOM, NOTVICT);

   if(!IS_NPC(victim))
   {
      send_to_char("They seem to be looking at you really strangly.\r\n", ch);
      send_to_char("You are sung to about butterflies and bullfrogs.\r\n", victim);
      return eSUCCESS;
   }

   if(number(0, 1))
   {
      // monster forgets hate/fear/track list
      send_to_char("Hrm.....who were you mad at again??\r\n", victim);
      send_to_char("You have soothed the savage beast.\r\n", ch);
      remove_memory(victim, 'h');
      remove_memory(victim, 'f');
      remove_memory(victim, 't');
   }
   else
   {
      // Die bard!
      send_to_char("Uh oh.\r\n", ch);
      do_say(victim, "Die you spoony bard!", 9);
      retval = attack(victim, ch, TYPE_UNDEFINED);
      retval = SWAP_CH_VICT(retval);
      return retval;
   }
   return eSUCCESS;
}

int song_shattering_resonance( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   // store the arg here, cause there's no place to save it before we execute
   ch->song_data = str_dup(arg);

   send_to_char("You begin to sing a song of shattering...\n\r", ch);
   act("$n begins a fading resonance...", ch, 0, 0, TO_ROOM, 0);
   if(GET_LEVEL(ch) > 49)
     ch->song_timer = (song_info[ch->song_number].beats - 1);
   else ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int execute_song_shattering_resonance( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   obj_data * obj = NULL;
   obj_data * tobj = NULL;

   if(!(obj = get_obj_in_list(ch->song_data,
                                 world[ch->in_room].contents)))
   {
      send_to_char("You don't see that object here.\r\n", ch);
      dc_free(ch->song_data);
      ch->song_data = 0;
      return eFAILURE;
   }
   dc_free(ch->song_data);
   ch->song_data = 0;

   // code to shatter a beacon
   if(GET_ITEM_TYPE(obj) == ITEM_BEACON) {
      act("$n's song fades to an end.", ch, 0, 0, TO_ROOM, 0);
      if(!obj->equipped_by) {
         // Someone load it or something?
         send_to_char("The magic fades away back to the ether.\n\r", ch);
         act("$p fades away gently.", ch, obj, 0, TO_ROOM, INVIS_NULL);
      }
      else {
         send_to_char("The magic is shattered by your will!\n\r", ch);
         act("$p blinks out of existance with a bang!", ch, obj, 0, TO_ROOM, INVIS_NULL);
         send_to_char("Your magic beacon is shattered!\n\r", obj->equipped_by);
         obj->equipped_by->beacon = NULL;
         obj->equipped_by = NULL;
      }
      extract_obj(obj);
      return eSUCCESS;
   }

   // make sure the obj is a player portal
   if(obj->obj_flags.type_flag != ITEM_PORTAL) {
      send_to_char("You can't shatter that!\r\n", ch);
      return eFAILURE;
   }
   if(!isname("pcportal", obj->name)) {
      send_to_char("The portal resists your song.\r\n", ch);
      return eFAILURE;
   }

   act("$n's song fades to an end.", ch, 0, 0, TO_ROOM, 0);

   // determine chance of destroying it
   if(number(0, 1)) // 50/50 for now
   {
      send_to_char("The portal resists your song.\r\n", ch);
      return eFAILURE;
   }

   send_to_room("You hear a loud shattering sound of magic discharging and the portal fades away.\r\n", obj->in_room);
   // we remove it from the room, in case the other portal is also in the same room
   // we extract both portals at the end
   obj_from_room(obj);

   // find it's match
   if(!(tobj = get_obj_in_list("pcportal",
               world[real_room(obj->obj_flags.value[0])].contents))) 
   {
      send_to_char("Could not find matching exit portal? Tell Pirahna.\r\n", ch);
      return eFAILURE;
   }

   // destroy it
   send_to_room("You hear a loud shattering sound of magic discharging and the portal fades away.\r\n", tobj->in_room);
   extract_obj(obj);
   extract_obj(tobj);
   return eSUCCESS;
}

int song_insane_chant( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin chanting insanely...\n\r", ch);
   act("$n begins chanting wildly...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int execute_song_insane_chant( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   struct affected_type af;

   af.type      = SKILL_INSANE_CHANT;
   af.duration  = 1;
   af.modifier  = 0;
   af.location  = APPLY_INSANE_CHANT;
   af.bitvector = -1;

   act("$n's singing starts to drive you INSANE!!!", ch, 0, 0, TO_ROOM, 0);
   send_to_char("Your singing drives you INSANE!!!\r\n", ch);

   for(victim = world[ch->in_room].people; victim; victim = victim->next_in_room)
   {
     // don't effect gods unless it was a higher level god singing
     if(GET_LEVEL(victim) >= IMMORTAL && GET_LEVEL(ch) <= GET_LEVEL(victim))
       continue;
   if (number(1,101) < get_saves(victim, SAVE_TYPE_POISON))
   {
act("$N resists your insane chant!", ch, NULL, victim, 
TO_CHAR,0);
act("$N resists $n's insane chant!", ch, NULL, victim, TO_ROOM,
NOTVICT);
act("You resist $n's insane chant!",ch,NULL,victim,TO_VICT,0);
     continue;
   }

     affect_to_char(victim, &af);
   }
   return eSUCCESS;
}

int song_flight_of_bee( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing a lofty song...\n\r", ch);
   act("$n raises $s voice in an flighty quick march...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int execute_song_flight_of_bee( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if(ch->master && ch->master->in_room == ch->in_room && 
                    ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if(!ISSET(fvictim->follower->affected_by, AFF_GROUP))
         continue;

      if(ch->in_room != fvictim->follower->in_room)
      {
         if(ISSET(fvictim->follower->affected_by, AFF_FLYING) &&
            !affected_by_spell(fvictim->follower, SPELL_FLY))
         {
            REMBIT(fvictim->follower->affected_by, AFF_FLYING);
            send_to_char("Your musical flight ends.\n\r", fvictim->follower);
         }
         continue;
      }
      if(affected_by_spell(fvictim->follower, SPELL_FLY))
      {
         affect_from_char(fvictim->follower, SPELL_FLY);
         send_to_char("Your fly spells dissapates.", fvictim->follower);
      }
      SETBIT(fvictim->follower->affected_by, AFF_FLYING);
      send_to_char("Your feet feel like air.\r\n", fvictim->follower);
   }
   if(ch->in_room == master->in_room)
   {
      SETBIT(master->affected_by, AFF_FLYING);
      send_to_char("Your feet feel like air.\r\n", master);
   }
   else
   {
      if(ISSET(master->affected_by, AFF_FLYING) &&
         !affected_by_spell(master, SPELL_FLY))
      {
         REMBIT(master->affected_by, AFF_FLYING);
         send_to_char("Your musical flight ends.\n\r", master);
      }
   }

   if (!skill_success(ch, 0, SKILL_SONG_FLIGHT_OF_BEE)) {
      send_to_char("You run out of lyrics and end the song.\r\n", ch);
      return eSUCCESS;
   }

   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int pulse_flight_of_bee( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   if(number(1, 5) == 3)
      act("$n prances around like a fairy.", ch, 0, 0, TO_ROOM, 0);   
   return eSUCCESS;
}

int intrp_flight_of_bee( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if(ch->master && ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if (origsing && origsing != fvictim->follower) continue;
      if(ISSET(fvictim->follower->affected_by, AFF_FLYING) &&
         !affected_by_spell(fvictim->follower, SPELL_FLY))
      {
         REMBIT(fvictim->follower->affected_by, AFF_FLYING);
         send_to_char("Your musical flight ends.\n\r", fvictim->follower);
      }
   }

  if (!origsing || origsing == master)
   if(ISSET(master->affected_by, AFF_FLYING) &&
      !affected_by_spell(master, SPELL_FLY))
   {
      REMBIT(master->affected_by, AFF_FLYING);
      send_to_char("Your musical flight ends.\r\n", master);
   }
   return eSUCCESS;
}

int song_searching_song( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   // store the char name here, cause A, we don't pass tar_char
   // and B, there's no place to save it before we execute
   ch->song_data = str_dup(arg);

   send_to_char("Your voice raises sending out a song to search the lands...\n\r", ch);
   act("$n raises $s voice sending out a song to search the lands....", ch, 0, 0, TO_ROOM, 0);

   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}


int execute_song_searching_song( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * target = NULL;
   char buf[200];

   target = get_char(ch->song_data);

   dc_free(ch->song_data);
   ch->song_data = 0;

   act("$n's song ends and quietly fades away.", ch, 0, 0, TO_ROOM, 0);

   if(!target || GET_LEVEL(ch) < GET_LEVEL(target))
   {
      send_to_char("Your song fades away, its search unfinished.\r\n", ch);
      return eFAILURE;
   }
   if (affected_by_spell(target, SKILL_INNATE_EVASION) 
		|| IS_SET(world[target->in_room].room_flags, NO_KI))
   {
	send_to_char("Something blocks your vision.\r\n",ch);
	return eFAILURE;
   }

   sprintf(buf, "Your song finds %s ", GET_SHORT(target));

   switch(GET_POS(target)) {
      case POSITION_STUNNED  :
            sprintf(buf, "%s%s at ", buf, "on the ground, stunned"); break;
      case POSITION_DEAD     :
            sprintf(buf, "%s%s at ", buf, "lying dead"); break;
      case POSITION_STANDING :
            sprintf(buf, "%s%s at ", buf, "standing around"); break;
      case POSITION_SITTING  :
            sprintf(buf, "%s%s at ", buf, "sitting"); break;
      case POSITION_RESTING  :
            sprintf(buf, "%s%s at ", buf, "resting"); break;
      case POSITION_SLEEPING :
            sprintf(buf, "%s%s at ", buf, "sleeping"); break;
      case POSITION_FIGHTING :
            sprintf(buf, "%s%s at ", buf, "fighting"); break;
      default:
            sprintf(buf, "%s%s at ", buf, "masturbating"); break;
   }

 if (affected_by_spell(target, SPELL_DETECT_MAGIC) && affected_by_spell(target, SPELL_DETECT_MAGIC)->modifier > 80)
    send_to_char("You sense you are the target of scrying.\r\n", target);

   sprintf(buf, "%s%s.\r\n", buf, world[target->in_room].name);
   send_to_char(buf, ch);
   return eSUCCESS;
}

int song_jig_of_alacrity( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing a quick little jig of alacrity...\n\r", ch);
   act("$n starts humming a quick little ditty...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int song_fanatical_fanfare(ubyte level, CHAR_DATA *ch, char *Aag, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing loudly, and poke everyone in your surroundings with a stick..\r\n",ch);
   act("$n starts singing loudly, and begins to poke everyone around $m with a stick. Hey!", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
  return eSUCCESS;
}
int song_mking_charge(ubyte level, CHAR_DATA *ch, char *Aag, CHAR_DATA *victim, int skill)
{
   send_to_char("You inspire your allies with your rousing songs about rising against oppression!\r\n",ch);
   act("$n starts singing songs about former glory and past victories, rousing $s allies!", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
  return eSUCCESS;
}
int execute_song_jig_of_alacrity( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   // Note, the jig effects everyone in the group BUT the bard.

   if (IS_SET(world[ch->in_room].room_flags, NO_KI)) {
     send_to_char("You find yourself unable to use energy based chants here.\n\r", ch);
     return intrp_jig_of_alacrity(level, ch, arg, victim, -1);
   }

   if(GET_KI(ch) < 2) // we don't have the ki to keep the song going
   {
     return intrp_jig_of_alacrity(level, ch, arg, victim, -1);
   }

   if(ch->master && ch->master->in_room == ch->in_room && 
                    ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
//      if(ch == fvictim->follower) This affects singer.
  //       continue;

      if(!ISSET(fvictim->follower->affected_by, AFF_GROUP))
         continue;

      if(ch->in_room != fvictim->follower->in_room)
      {
         if(ISSET(fvictim->follower->affected_by, AFF_HASTE) &&
            !affected_by_spell(fvictim->follower, SPELL_HASTE))
         {
            REMBIT(fvictim->follower->affected_by, AFF_HASTE);
            send_to_char("Your limbs slow back to normal.\n\r", fvictim->follower);
         }
         continue;
      }
      if(affected_by_spell(fvictim->follower, SPELL_HASTE))
      {
         affect_from_char(fvictim->follower, SPELL_HASTE);
         send_to_char("Your limbs slow back to normal.\n\r", fvictim->follower);
      }
      SETBIT(fvictim->follower->affected_by, AFF_HASTE);
      send_to_char("Your dance quickens your pulse!\r\n", fvictim->follower);
   }

//  if(ch != master)
   if(ch->in_room == master->in_room)
   {
      SETBIT(master->affected_by, AFF_HASTE);
      send_to_char("Your dance quickens your pulse!\r\n", master);
   }
   else
   {
      if(ISSET(master->affected_by, AFF_HASTE) &&
         !affected_by_spell(master, SPELL_HASTE))
      {
         REMBIT(master->affected_by, AFF_HASTE);
         send_to_char("Your limbs slow back to normal.\n\r", fvictim->follower);
      }
   }

   if(!skill_success(ch, NULL, SKILL_SONG_JIG_OF_ALACRITY)) {
      send_to_char("You run out of lyrics and end the song.\r\n", ch);
      return eSUCCESS;
   }

   GET_KI(ch) -= 2;

   ch->song_timer = song_info[ch->song_number].beats + 
                             (GET_LEVEL(ch) > 33) +
                             (GET_LEVEL(ch) > 43);
   return eSUCCESS;
}

int execute_song_fanatical_fanfare(ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   // Note, the jig effects everyone in the group BUT the bard.

   if (IS_SET(world[ch->in_room].room_flags, NO_KI)) {
     send_to_char("You find yourself unable to use energy based chants here.\n\r", ch);
     return intrp_song_fanatical_fanfare(level, ch, arg, victim, -1);
   }

   bool ended = FALSE;
   if (!skill_success(ch, NULL, SKILL_SONG_FANATICAL_FANFARE))
	ended = TRUE;
   if(!ended && GET_KI(ch) < (has_skill(ch,SKILL_SONG_FANATICAL_FANFARE) > 60?1:2)) // we don't have the ki to keep the song going
   {
     return intrp_song_fanatical_fanfare(level, ch, arg, victim, -1);
   }

   if(ch->master && ch->master->in_room == ch->in_room && 
                    ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
//      if(ch == fvictim->follower)
//         continue;

      if(!ISSET(fvictim->follower->affected_by, AFF_GROUP))
         continue;

      if(ch->in_room != fvictim->follower->in_room || ended)
      {
         if(ISSET(fvictim->follower->affected_by, AFF_INSOMNIA) &&
            !affected_by_spell(fvictim->follower, SPELL_INSOMNIA))         {
            REMBIT(fvictim->follower->affected_by, AFF_INSOMNIA);
            send_to_char("Your mind returns to its normal state.\n\r", fvictim->follower);
         }
         continue;
      }
      if(affected_by_spell(fvictim->follower, SPELL_INSOMNIA))
      {
         affect_from_char(fvictim->follower, SPELL_INSOMNIA);
         send_to_char("Your mind returns to its normal state.\n\r", fvictim->follower);
      }
      SETBIT(fvictim->follower->affected_by, AFF_INSOMNIA);
      send_to_char("Your mind races at a thousand miles an hour, following the beat of the song!\r\n", fvictim->follower);
   }

  //if(ch != master)
   if(ch->in_room == master->in_room &&!ended) 
   {
      SETBIT(master->affected_by, AFF_INSOMNIA);
      send_to_char("Your song causes your mind to race at a thousand miles an hour!\r\n", master);
   }
   else
   {
      if(ISSET(master->affected_by, AFF_INSOMNIA) &&
         !affected_by_spell(master, SPELL_INSOMNIA))
      {
         REMBIT(master->affected_by, AFF_INSOMNIA);
         send_to_char("Your mind returns to its normal state.\n\r", master);
      }
   }


   if (ended)
   {
	send_to_char("You run out of lyrics and end the song.\r\n",ch);
	return eSUCCESS;
   }

   GET_KI(ch) -= (has_skill(ch,SKILL_SONG_FANATICAL_FANFARE) > 60?1:2);

   ch->song_timer = song_info[ch->song_number].beats + 
                             (GET_LEVEL(ch) > 33) +
                             (GET_LEVEL(ch) > 43);
   return eSUCCESS;
}

int execute_mking_charge(ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if (IS_SET(world[ch->in_room].room_flags, NO_KI)) {
     send_to_char("You find yourself unable to sing here.\n\r", ch);
     return intrp_mking_charge(level, ch, arg, victim, -1);
   }

   bool ended = FALSE;
   if (!skill_success(ch, NULL, SKILL_SONG_MKING_CHARGE))
	ended = TRUE;
   if(!ended && GET_KI(ch) < 5) // we don't have the ki to keep the song going
   {
	ended = TRUE;
     return intrp_mking_charge(level, ch, arg, victim, -1);
   }
    struct affected_type af;
  af.type      = SKILL_SONG_MKING_CHARGE;
  af.duration  = 1;
  af.modifier  = 0;
  af.location  = 0;
 if (skill > 80)
  af.bitvector = AFF_HASTE;
else af.bitvector = -1;

   if(ch->master && ch->master->in_room == ch->in_room && 
                    ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
//      if(ch == fvictim->follower)
//         continue;

      if(!ISSET(fvictim->follower->affected_by, AFF_GROUP))
         continue;

      if(ch->in_room != fvictim->follower->in_room || ended)
      {
	if (affected_by_spell(fvictim->follower, SKILL_SONG_MKING_CHARGE))
	{
         affect_from_char(fvictim->follower, SKILL_SONG_MKING_CHARGE);
	 send_to_char("You lose the inspiration.\r\n",fvictim->follower);
        }
         continue;
      }
	
	if (!affected_by_spell(fvictim->follower, SKILL_SONG_MKING_CHARGE))
	affect_to_char(fvictim->follower, &af);
     act("$n's songs and tales of your ancestors' struggles fuel you with rage, sending you into a temporary frenzy!  To arms!", ch, 0, fvictim->follower, TO_VICT, 0); 
   }

  //if(ch != master)
   if(ch->in_room == master->in_room &&!ended) 
   {
	if (!affected_by_spell(master, SKILL_SONG_MKING_CHARGE))
	affect_to_char(master, &af);
     if (ch != master)
     act("$n's songs and tales of your ancestors' struggles fuel you with rage, sending you into a temporary frenzy!  To arms!", ch, 0, master, TO_VICT, 0); 
    else
      send_to_char("Your songs and tales fuel you with rage, sending you into a temporary frenzy!  To arms!\r\n", master);
   }
   else
   {
      if (affected_by_spell(master, SKILL_SONG_MKING_CHARGE))
      {
	affect_from_char(master, SKILL_SONG_MKING_CHARGE);
	 send_to_char("You lose the inspiration.\r\n",master);
      }
   }
   if (af.bitvector == AFF_HASTE)
   {
     send_to_char("Weaving the jig of alacrity into your song of battle, your allies move faster.\r\n",ch);
   }

   if (ended)
   {
	send_to_char("You run out of lyrics and end the song.\r\n",ch);
	return eSUCCESS;
   }

   GET_KI(ch) -= 5;

   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int pulse_song_fanatical_fanfare(ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
  if (number(1,5) == 2)
      act("$n combines singing and poking the people with a stick, getting people worked up.", ch, 0, 0, TO_ROOM,0);
  return eSUCCESS;
}

int pulse_mking_charge(ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
  return eSUCCESS;
}

int pulse_jig_of_alacrity( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   if(number(1, 5) == 3)
      act("$n prances around like a fairy.", ch, 0, 0, TO_ROOM, 0);   
   return eSUCCESS;
}

int intrp_jig_of_alacrity( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if(ch->master && ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if (origsing && origsing != fvictim->follower) continue;
      if(ISSET(fvictim->follower->affected_by, AFF_HASTE) &&
         !affected_by_spell(fvictim->follower, SPELL_HASTE))
      {
         REMBIT(fvictim->follower->affected_by, AFF_HASTE);
         send_to_char("Your limbs slow back to normal.\n\r", fvictim->follower);
      }
   }

   if (!origsing || origsing == master)
   if(ISSET(master->affected_by, AFF_HASTE) &&
      !affected_by_spell(master, SPELL_HASTE))
   {
      REMBIT(master->affected_by, AFF_HASTE);
      send_to_char("Your limbs slow back to normal.\r\n", master);
   }
   return eSUCCESS;
}

int intrp_song_fanatical_fanfare( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if(ch->master && ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;
   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if (origsing && origsing != fvictim->follower) continue;
      if(ISSET(fvictim->follower->affected_by, AFF_INSOMNIA) &&
         !affected_by_spell(fvictim->follower, SPELL_INSOMNIA))
      {
         REMBIT(fvictim->follower->affected_by, AFF_INSOMNIA);
         send_to_char("Your mind returns to its normal state.\n\r", fvictim->follower);
      }
   }

   if (!origsing || origsing == master)
   if(ISSET(master->affected_by, AFF_INSOMNIA) &&
      !affected_by_spell(master, SPELL_INSOMNIA))
   {
      REMBIT(master->affected_by, AFF_INSOMNIA);
      send_to_char("Your mind returns to its normal state.\r\n", master);
   }
   return eSUCCESS;

}
int intrp_mking_charge( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if(ch->master && ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if (origsing && origsing != fvictim->follower) continue;
	if (affected_by_spell(fvictim->follower, SKILL_SONG_MKING_CHARGE))
      {
	 affect_from_char(fvictim->follower, SKILL_SONG_MKING_CHARGE);
	 send_to_char("You lose the inspiration.\r\n",fvictim->follower);
      }
   }

   if (!origsing || origsing == master)
   if (affected_by_spell(master, SKILL_SONG_MKING_CHARGE))
   {
	 affect_from_char(master, SKILL_SONG_MKING_CHARGE);
      send_to_char("You lose the inspiration.\r\n",master);

   }
   return eSUCCESS;

}

int song_glitter_dust( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You throw dust in the air and sing a wily ditty...\n\r", ch);
   act("$n throws some dust in the air and sings a wily ditty...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int execute_song_glitter_dust( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   struct affected_type af;

   af.type      = SKILL_GLITTER_DUST;
   af.duration  = (GET_LEVEL(ch) > 25) ? 2 : 1;
   af.modifier  = 0;
   af.location  = APPLY_GLITTER_DUST;
   af.bitvector = -1;

   act("The dust in the air clings to you, and begins to shine!", ch, 0, 0, TO_ROOM, 0);
   send_to_char("Your dust clings to everyone, showing where they are!\r\n", ch);

   for(victim = world[ch->in_room].people; victim; victim = victim->next_in_room)
   {
     // don't effect gods unless it was a higher level god singing
     if(GET_LEVEL(victim) >= IMMORTAL)
       continue;
     affect_to_char(victim, &af);
   }
      
   return eSUCCESS;
}

int song_bountiful_sonnet( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin long restoring sonnet...\n\r", ch);
   act("$n begins a long restorous sonnet...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int execute_song_bountiful_sonnet( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if(ch->master && ch->master->in_room == ch->in_room && 
                    ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if(!ISSET(fvictim->follower->affected_by, AFF_GROUP) || 
         fvictim->follower->in_room != ch->in_room)
         continue;

      send_to_char("You feel like you've just eaten a huge meal!\r\n", fvictim->follower);
      if(GET_COND(fvictim->follower, FULL) != -1)
         GET_COND(fvictim->follower, FULL) = 20;
      if(GET_COND(fvictim->follower, THIRST) != -1)
         GET_COND(fvictim->follower, THIRST) = 20;
   }
   if(ch->in_room == master->in_room)
   {
      send_to_char("You feel like you've just eaten a huge meal!\r\n", master);
      if(GET_COND(master, FULL) != -1)
         GET_COND(master, FULL) = 20;
      if(GET_COND(master, THIRST) != -1)
         GET_COND(master, THIRST) = 20;
   }
   return eSUCCESS;
}

int execute_song_dischordant_dirge( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * target = NULL;
   //char buf[400];

   target = get_char_room_vis(ch, ch->song_data);

   dc_free(ch->song_data);
   ch->song_data = 0;
   act("$n's dirge ends in a shriek.", ch, 0, 0, TO_ROOM, 0);

   if(!target || GET_LEVEL(ch) < GET_LEVEL(target))
   {
      send_to_char("Your dirge fades, its effect neutralized.\r\n", ch);
      return eFAILURE;
   }

   if(ch==target ) {
      send_to_char("Your loyalties has been broken. And stuff. What did you think?\r\n", ch);
      return eFAILURE;
   }

   if(!IS_NPC(target)) {
      send_to_char("You cannot break their bonds of loyalty.\r\n", ch);
      return eFAILURE;
   }
   if (!affected_by_spell(target, SPELL_CHARM_PERSON) && !IS_AFFECTED(target, AFF_FAMILIAR))
   {
	send_to_char("As far as you can tell, they are not loyal to anyone.\r\n",ch);
	return eFAILURE;
   }
   int type = 0;
   if (mob_index[target->mobdata->nr].virt == 8) type = 4;
   else if (IS_AFFECTED(target, AFF_FAMILIAR)) type = 3;
   else if (mob_index[target->mobdata->nr].virt >= 22394 &&
		mob_index[target->mobdata->nr].virt <= 22398) type = 2;
   else type = 1;
   if ((type == 4 && !number(0,9)) || (type == 2 && !number(0,4)) ||
       (type == 1 && !number(0,1)))
   {
        send_to_char("Ooops, that didn't work out like you hoped.\r\n",ch);
        act("$N charges at $n, for trying to break its bond with its master.\r\n",ch, 0, target, TO_ROOM,NOTVICT);
        act("$N charges at you!",ch,0,target,TO_CHAR,0);
        return attack(target, ch, TYPE_UNDEFINED);
   }


   //int i;
/*   for (i = 22394; i < 22399; i++)
     if (real_mobile(i) == target->mobdata->nr)
     {
	send_to_char("The undead being is unaffected by your song.\r\n",ch);
	return eFAILURE;
     }*/
   if (IS_AFFECTED(target, AFF_FAMILIAR))
   {
     act("$n shatters $N's bond with this realm, and the creature vanishes.",ch, 0, target, TO_ROOM, NOTVICT);
     act("At your dirge's completion, $N vanishes.", ch, 0, target, TO_CHAR,0);
     extract_char(target, TRUE);
     return eSUCCESS;
   }
   if (type == 4)
   {
      act("$n shatters!", target, 0, 0, TO_ROOM, 0);
      extract_char(target, FALSE);
      return eSUCCESS;
   } else if (type == 2) {
        act("$n's mind is set free, and the body falls onto the ground", target, 0, 0, TO_ROOM, 0);
	extract_char(target, TRUE);
	return eSUCCESS;
   }
   affect_from_char(target, SPELL_CHARM_PERSON);
   send_to_char("You shatter their magical chains.\r\n",ch);
   send_to_char("Boogie! Your mind has been set free!\r\n",target);
   
   act("$N blinks and shakes its head, clearing its thoughts.",
        ch, 0, target, TO_CHAR, 0);
   act("$N blinks and shakes its head, clearing its thoughts.",
          ch, 0, target, TO_ROOM, NOTVICT);
   if (target->fighting)
   {
	do_say(target, "Hey, this sucks. I'm goin' home!",9);
        if (target->fighting->fighting == target)
	  stop_fighting(target->fighting);
	stop_fighting(target);
   }
   return eSUCCESS;
}

int song_dischordant_dirge( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   // store the char name here, cause A, we don't pass tar_char
   // and B, there's no place to save it before we execute
   ch->song_data = str_dup(arg);

   send_to_char("You begin a wailing dirge...\n\r", ch);
   act("$n begins to sing a wailing dirge...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats - (GET_LEVEL(ch)/10);
   if(ch->song_timer < 1)
      ch->song_timer = 1;
   return eSUCCESS;
}


int song_synchronous_chord( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   // store the char name here, cause A, we don't pass tar_char
   // and B, there's no place to save it before we execute
   ch->song_data = str_dup(arg);

   send_to_char("You begin a strong chord...\n\r", ch);
   act("$n begins to sound a chord...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats - (GET_LEVEL(ch)/10);
   if(ch->song_timer < 1)
      ch->song_timer = 1;
   return eSUCCESS;
}


int execute_song_synchronous_chord( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * target = NULL;
   char buf[400];
   char * get_random_hate(CHAR_DATA *ch);
   int learned = has_skill(ch, song_info[SKILL_SONG_SYNC_CHORD].skill_num);

   extern char *isr_bits[];

   target = get_char_room_vis(ch, ch->song_data);

   dc_free(ch->song_data);
   ch->song_data = 0;

   act("$n's song ends with an abrupt stop.", ch, 0, 0, TO_ROOM, 0);

   if(!target || GET_LEVEL(ch) < GET_LEVEL(target))
   {
      send_to_char("Your song fades away, its target unknown.\r\n", ch);
      return eFAILURE;
   }

   if(ch == target) {
      send_to_char("You hate yourself, you self-loathing bastard.\r\n", ch);
      return eFAILURE;
   }

   if(!IS_NPC(target)) {
      send_to_char("They don't hate anyone, but they are looking at you kinda funny...\r\n", ch);
      return eFAILURE;
   }

   act("You enter $S mind...", ch, 0, target, TO_CHAR, INVIS_NULL);
   sprintf(buf, "%s seems to hate... %s.\r\n", GET_SHORT(target),
            get_random_hate(target) ? get_random_hate(target) : "no one!");
   send_to_char(buf, ch);

   if(learned > 80) {
      sprintbit(target->resist, isr_bits, buf);
      csendf(ch, "%s is resistant to: %s\n\r", GET_SHORT(target), buf);
   }
   if (learned > 85) {
      sprintbit(target->immune, isr_bits, buf);
      csendf(ch, "%s is immune to: %s\n\r", GET_SHORT(target), buf);
   }
   if (learned > 90) {  
      sprintbit(target->suscept, isr_bits, buf);
      csendf(ch, "%s is susceptable to: %s\n\r", GET_SHORT(target), buf);
   }

   return eSUCCESS;
}


int song_sticky_lullaby( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   // store the char name here, cause A, we don't pass tar_char
   // and B, there's no place to save it before we execute
   ch->song_data = str_dup(arg);

   send_to_char("You begin a slow numbing lullaby...\n\r", ch);
   act("$n starts a eye-drooping lullaby.", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   if(GET_LEVEL(ch) < 40)
      ch->song_timer += 2;
   return eSUCCESS;
}

int execute_song_sticky_lullaby( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   if(!(victim = get_char_room_vis(ch, ch->song_data)))
   {
      send_to_char("You don't see that person here.\r\n", ch);
      dc_free(ch->song_data);
      ch->song_data = 0;
      return eFAILURE;
   }
   dc_free(ch->song_data);
   ch->song_data = 0;
   if (number(1,101) < get_saves(victim, SAVE_TYPE_POISON))
   {
      act("$N resists your sticky lullaby!", ch, NULL, victim, TO_CHAR,0);
      act("$N resists $n's sticky lullaby!", ch, NULL, victim, TO_ROOM,NOTVICT);
      act("You resist $n's sticky lullaby!",ch,NULL,victim,TO_VICT,0);
      return eFAILURE;
   }

   act("$n lulls $N's feet into a numbing sleep.", ch, 0, victim, TO_ROOM, NOTVICT);
   act("$N's feet fall into a numbing sleep.", ch, 0, victim, TO_CHAR, 0);
   send_to_char("Your eyes begin to droop, and your feet fall asleep!\r\n", victim);
   SETBIT(victim->affected_by, AFF_NO_FLEE);
   return eSUCCESS;
}

int song_vigilant_siren( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing a fast nervous tune...\n\r", ch);
   act("$n starts mumbling out a quick, nervous tune...", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int execute_song_vigilant_siren( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if (IS_SET(world[ch->in_room].room_flags, NO_KI)) {
     send_to_char("You find yourself unable to use energy based chants here.\n\r", ch);
     return intrp_vigilant_siren(level, ch, arg, victim, -1);
   }

   if(GET_KI(ch) < 2) // we don't have the ki to keep the song going
   {
     return intrp_vigilant_siren(level, ch, arg, victim, -1);
   }

   if(ch->master && ch->master->in_room == ch->in_room && 
                    ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if(!ISSET(fvictim->follower->affected_by, AFF_GROUP))
         continue;

      if(ch->in_room != fvictim->follower->in_room &&
         ISSET(fvictim->follower->affected_by, AFF_ALERT))
      {
         REMBIT(fvictim->follower->affected_by, AFF_ALERT);
         send_to_char("You stop watching your back so closely.\r\n", fvictim->follower);
         continue;
      }
      if (ch->in_room != fvictim->follower->in_room) continue;
      SETBIT(fvictim->follower->affected_by, AFF_ALERT);
      send_to_char("You nervously watch your surroundings with magical speed!\r\n", fvictim->follower);
   }

   if(ch->in_room == master->in_room)
   {
      SETBIT(master->affected_by, AFF_ALERT);
      send_to_char("You nervously watch your surroundings with magical speed!\r\n", master);
   }
   else if(ISSET(master->affected_by, AFF_ALERT))
   {
      REMBIT(master->affected_by, AFF_ALERT);
      send_to_char("You stop watching your back so closely.\r\n", fvictim->follower);
   }

   GET_KI(ch) -= 1;

   ch->song_timer = song_info[ch->song_number].beats + 
                             (GET_LEVEL(ch) > 48);
   return eSUCCESS;
}

int pulse_vigilant_siren( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   if(number(1, 5) == 3)
      act("$n chatters a ditty about being alert and ever watchful.", ch, 0, 0, TO_ROOM, 0);   
   return eSUCCESS;
}

int intrp_vigilant_siren( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if(ch->master && ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if (origsing && origsing != fvictim->follower) continue;
      if(ISSET(fvictim->follower->affected_by, AFF_ALERT)) {
         REMBIT(fvictim->follower->affected_by, AFF_ALERT);
         send_to_char("You stop watching your back so closely.\r\n", fvictim->follower);
      }
   }

   if (!origsing || origsing == ch)
   if(ISSET(master->affected_by, AFF_ALERT)) {
      REMBIT(master->affected_by, AFF_ALERT);
      send_to_char("You stop watching your back so closely.\r\n", master);
   }
   return eSUCCESS;
}

void make_person_dance(char_data * ch)
{
   char * dances[] =
   {
      "dance",         // 0
      "tango",
      "boogie",
      "jig",
      "waltz",
      "bellydance",    // 5
      "\n"
   };

   int numdances = 5;
   char dothis[50];

   strcpy(dothis, dances[number(0, numdances)]);

   command_interpreter(ch, dothis);
}

int song_unresistable_ditty( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing an irresistable little ditty...\n\r", ch);
   act("$n begins to sing, 'du du dudu du du dudu du du dudu!'", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int execute_song_unresistable_ditty( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * i;

   act("$n finishs $s song, 'Ahhhhh!  Macarena!'", ch, 0, 0, TO_ROOM, 0);
   send_to_char("Ahhh....such beautiful music.\r\n", ch);

//   int specialization = skill / 100;
   skill %= 100;

   for (i = world[ch->in_room].people; i; i = i->next_in_room)
   {
   if (number(1,101) < get_saves(i, SAVE_TYPE_MAGIC))
   {
act("$N resists your irresistible ditty!", ch, NULL, i, 
TO_CHAR,0);
act("$N resists $n's irresitble ditty! Not so irresistible, eh!", ch, NULL, i, TO_ROOM,NOTVICT);
act("You resist $n's \"irresistible\" ditty!!",ch,NULL,i,TO_VICT,0);
     continue;
   }

      if(GET_LEVEL(i) <= GET_LEVEL(ch))
      {
         make_person_dance(i);
         if(skill > 80)
            WAIT_STATE(i, PULSE_VIOLENCE*2);
      }
   }


   return eSUCCESS;
}

int song_crushing_crescendo( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing, approaching crescendo!\n\r", ch);
   act("$n begins to sing, raising the volume to deafening levels!", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   ch->song_data = 0; // first round.
   return eSUCCESS;
}

int execute_song_crushing_crescendo( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   int dam = 0;
   int retval;

   //int specialization = skill / 100;
   skill %= 100;

   victim = ch->fighting;

   if (IS_SET(world[ch->in_room].room_flags, NO_KI)) {
     send_to_char("You find yourself unable to use energy based chants here.\n\r", ch);
     return eSUCCESS;
   } 

   if(!victim)
   {
      send_to_char("With the battle broken you end your crescendo.\r\n", ch);
      return eSUCCESS;
   }

   int combat, non_combat;
   get_instrument_bonus(ch, combat, non_combat);

   int i;
   dam = 75;
   for (i = 0; i < (int)ch->song_data; i++)
      dam = dam * 2;
   dam += combat * 5; // Make it hurt some more.
//   if ((int)ch->song_data < 3) // Doesn't help beyond that.
     ch->song_data = (char*)((int)ch->song_data + 1); // Add one round.
		// Bleh, C allows easier pointer manipulation
   if (number(1,101) < get_saves(victim, SAVE_TYPE_MAGIC))
   {
     act("$N resists your crushing crescendo!", ch, NULL, victim, TO_CHAR,0);
     act("$N resists $n's crushing crescendo!", ch, NULL, victim, TO_ROOM,NOTVICT);
     act("You resist $n's crushing crescendo!",ch,NULL,victim,TO_VICT,0);
     dam /= 2;
   }

   switch ((int)ch->song_data)
   {
     case 1:
       act("$N is injured by the strength of your music!", ch, NULL, victim, TO_CHAR,0);
       act("The strength of $n's music injures $N!", ch, NULL, victim, TO_ROOM,NOTVICT);
       act("The strength of $n's crushing crescendo injures you!",ch,NULL,victim,TO_VICT,0);
       break;     
     case 2:
       act("$N is injured further by the intensity of your music!", ch, NULL, victim, TO_CHAR,0);
       act("The strength of $n's music increases, and causes further injury to $N!", ch, NULL, victim, TO_ROOM,NOTVICT);
       act("The strength of $n's crushing crescendo increases, and hurts even more!",ch,NULL,victim,TO_VICT,0);
       break;     
     case 3:
       act("The force of your song powerfully crushes the life out of $N!", ch, NULL, victim, TO_CHAR,0);
       act("The force of $n's crushes the life out of $N!", ch, NULL, victim, TO_ROOM,NOTVICT);
       act("The force of $n's crushes the life out of you!",ch,NULL,victim,TO_VICT,0);
       break;     
     default:
       act("$N is injured by the strength of your music!", ch, NULL, victim, TO_CHAR,0);
       act("The strength of $n's music injures $N!", ch, NULL, victim, TO_ROOM,NOTVICT);
       act("The strength of $n's crushing crescendo injures you!",ch,NULL,victim,TO_VICT,0);
       break;     
   }

   bool ispc = !IS_NPC(victim);
   char buf[MAX_STRING_LENGTH];
   strcpy(buf, victim->short_desc);

   retval = damage(ch, victim, dam, TYPE_SONG,SKILL_SONG_CRUSHING_CRESCENDO, 0);
   if(IS_SET(retval, eCH_DIED))
     return retval;

   if(IS_SET(retval, eVICT_DIED))
   {
     char buf2[MAX_STRING_LENGTH];
	sprintf(buf2, "$n's crushing crescendo has completely crushed %s and they are no more.", buf);
      act(buf2, ch, NULL, victim, TO_ROOM, NOTVICT);
	if (ispc)
      act("$n's song has completely crushed you!", ch, NULL, victim, TO_VICT, 0);
      sprintf(buf2, "The power of your song has completely crushed %s!", buf);
      act(buf2, ch, NULL, victim, TO_CHAR, 0);

      send_to_char("You dance a small jig on the corpse.\r\n", ch);
      act("$n dances a little jig on the fallen corpse.",
          ch, 0, victim, TO_ROOM, 0);
	return retval;
  }

  if((int)ch->song_data > has_skill(ch,SKILL_SONG_CRUSHING_CRESCENDO) /20 || (int)ch->song_data > 3) 
  {
      send_to_char("You run out of lyrics and end the song.\r\n", ch);
      ch->song_data = 0;
      return eSUCCESS;
   }

  if (((int)ch->song_data) != 3) {
  if (GET_KI(ch) < song_info[ch->song_number].min_useski)
  {
	send_to_char("Having run out of ki, your song ends abruptly.\r\n",ch);
	ch->song_data = 0; // Reset, just in case.
	return eSUCCESS;
  }
  GET_KI(ch) -= song_info[ch->song_number].min_useski;
  }
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int song_submariners_chorus( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   send_to_char("You begin to sing about the shining sea and her terrible ways...\n\r", ch);
   act("$n sings a surly number about $s fickle mistress, the briny deep.", ch, 0, 0, TO_ROOM, 0);
   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int execute_song_submariners_chorus( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if(ch->master && ch->master->in_room == ch->in_room && 
                    ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if(!ISSET(fvictim->follower->affected_by, AFF_GROUP))
         continue;

      if(ch->in_room != fvictim->follower->in_room)
      {
         if(ISSET(fvictim->follower->affected_by, AFF_WATER_BREATHING) &&
            !affected_by_spell(fvictim->follower, SPELL_WATER_BREATHING))
         {
            REMBIT(fvictim->follower->affected_by, AFF_WATER_BREATHING);
            send_to_char("Your musical ability to breath water ends.\n\r", fvictim->follower);
         }
         continue;
      }
      if(affected_by_spell(fvictim->follower, SPELL_WATER_BREATHING))
      {
         affect_from_char(fvictim->follower, SPELL_WATER_BREATHING);
         send_to_char("Your magical gills disappear.", fvictim->follower);
      }
      SETBIT(fvictim->follower->affected_by, AFF_WATER_BREATHING);
      send_to_char("Your lungs absorb oxygen from any fluid!\r\n", fvictim->follower);
   }
   if(ch->in_room == master->in_room)
   {
      SETBIT(master->affected_by, AFF_WATER_BREATHING);
      send_to_char("Your lungs absorb oxygen from any fluid!\r\n", master);
   }
   else
   {
      if(ISSET(master->affected_by, AFF_WATER_BREATHING) &&
         !affected_by_spell(master, SPELL_WATER_BREATHING))
      {
         REMBIT(master->affected_by, AFF_WATER_BREATHING);
         send_to_char("Your musical ability to breath water ends.\n\r", master);
      }
   }

   ch->song_timer = song_info[ch->song_number].beats;
   return eSUCCESS;
}

int pulse_submariners_chorus( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   if(number(1, 5) == 3)
      act("$n breathes in with surprising ease.", ch, 0, 0, TO_ROOM, 0);
   return eSUCCESS;
}

int intrp_submariners_chorus( ubyte level, CHAR_DATA *ch, char *arg, CHAR_DATA *victim, int skill)
{
   char_data * master = NULL;
   follow_type * fvictim = NULL;

   if(ch->master && ISSET(ch->affected_by, AFF_GROUP))
      master = ch->master;
   else master = ch;

   for(fvictim = master->followers; fvictim; fvictim = fvictim->next)
   {
      if (origsing && origsing != fvictim->follower) continue;
      if(ISSET(fvictim->follower->affected_by, AFF_WATER_BREATHING) &&
         !affected_by_spell(fvictim->follower, SPELL_WATER_BREATHING))
      {
         REMBIT(fvictim->follower->affected_by, AFF_WATER_BREATHING);
         send_to_char("Your musical ability to breath water ends.\n\r", fvictim->follower);
      }
   }

  if (!origsing || origsing == master)
   if(ISSET(master->affected_by, AFF_WATER_BREATHING) &&
      !affected_by_spell(master, SPELL_WATER_BREATHING))
   {
      REMBIT(master->affected_by, AFF_WATER_BREATHING);
      send_to_char("Your musical ability to breath water ends.\r\n", master);
   }
   return eSUCCESS;
}

