
/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/

extern "C"
{
  #include <ctype.h>
  #include <string.h>
}

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <fileinfo.h> 
#include <act.h>
#include <player.h>
#include <levels.h>
#include <room.h>
#include <structs.h>
#include <fight.h>
#include <spells.h>
#include <utility.h>
#include <connect.h>
#include <interp.h>
#include <handler.h>
#include <db.h>
#include <comm.h>
#include <returnvals.h>

// external vars

extern struct index_data *mob_index;
extern CWorld world;
extern struct descriptor_data *descriptor_list;
extern bool MOBtrigger;
extern struct mprog_throw_type *g_mprog_throw_list;

struct obj_data *get_object_in_equip_vis(struct char_data *ch, char *arg, struct obj_data *equipment[], int *j);

/*
 * Local functions.
 */

char *			mprog_type_to_name	( int type );

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. It allows the words to show up in mpstat to
 *  make it just a hair bit easier to see what a mob should be doing.
 */

char *mprog_type_to_name( int type )
{
    switch ( type )
    {
    case IN_FILE_PROG:          return "in_file_prog";
    case ACT_PROG:              return "act_prog";
    case SPEECH_PROG:           return "speech_prog";
    case RAND_PROG:             return "rand_prog";
    case ARAND_PROG:		return "arand_prog";
    case FIGHT_PROG:            return "fight_prog";
    case HITPRCNT_PROG:         return "hitprcnt_prog";
    case DEATH_PROG:            return "death_prog";
    case ENTRY_PROG:            return "entry_prog";
    case GREET_PROG:            return "greet_prog";
    case ALL_GREET_PROG:        return "all_greet_prog";
    case GIVE_PROG:             return "give_prog";
    case BRIBE_PROG:            return "bribe_prog";
    case CATCH_PROG:		return "catch_prog";
    case ATTACK_PROG:		return "attack_prog";
    default:                    return "ERROR_PROG";
    }
}

/* A trivial rehack of do_mstat.  This doesnt show all the data, but just
 * enough to identify the mob and give its basic condition.  It does however,
 * show the MOBprograms which are set.
 */

void mpstat( CHAR_DATA *ch, CHAR_DATA *victim)
{
    char        buf[ MAX_STRING_LENGTH ];
    char        buf2[ MAX_STRING_LENGTH ];
    MPROG_DATA *mprg;
    int i;

    sprintf( buf, "$3Name$R: %s  $3Vnum$R: %d.\n\r",
	victim->name, mob_index[victim->mobdata->nr].virt );
    send_to_char( buf, ch );

    sprintf( buf, "$3Short description$R: %s\n\r$3Long  description$R: %s\r\n",
	    victim->short_desc,
	    victim->long_desc ? victim->long_desc : "(NULL)" );
    send_to_char( buf, ch );

    if ( !( mob_index[victim->mobdata->nr].progtypes ) )
    {
	send_to_char( "That mob has no programs set.\n\r", ch);
	return;
    }

    for ( mprg = mob_index[victim->mobdata->nr].mobprogs, i = 1; mprg != NULL;
	 i++, mprg = mprg->next )
    {
      sprintf( buf, "$3%d$R>$3$B", i);
      send_to_char( buf, ch );
      send_to_char(mprog_type_to_name( mprg->type ), ch);
      send_to_char("$R ", ch);
      sprintf( buf, "$B$5%s$R\n\r", mprg->arglist);
      send_to_char(buf, ch);
      sprintf( buf, "%s\n\r", mprg->comlist );
      double_dollars(buf2, buf);
      send_to_char( buf2, ch );
    }
}

/* prints the argument to all the rooms aroud the mobile */

int do_mpasound( CHAR_DATA *ch, char *argument, int cmd )
{

  long was_in_room;
  int              door;

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
        return eSUCCESS;
    }

    if ( argument[0] == '\0' )
    {
        logf( IMMORTAL, LOG_WORLD, "Mpasound - No argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    was_in_room = ch->in_room;
    for ( door = 0; door <= 5; door++ )
    {
      if(CAN_GO(ch, door))
      {
	ch->in_room = world[was_in_room].dir_option[door]->to_room;
        if(ch->in_room == was_in_room)
          continue;
	MOBtrigger  = FALSE;
        // argument +1 so we skip the leading ' '
	act( argument+1, ch, NULL, NULL, TO_ROOM, 0 );
        ch->in_room = was_in_room;
      }
    }

  ch->in_room = was_in_room;
  return eSUCCESS;

}

/* lets the mobile kill any player or mobile without murder*/

int do_mpkill( CHAR_DATA *ch, char *argument, int cmd )
{
    char      arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	logf( IMMORTAL, LOG_WORLD, "MpKill - no argument: vnum %d.",
		mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( ( victim = get_char_room_vis( ch, arg ) ) == NULL )
    {
	logf( IMMORTAL, LOG_WORLD, "MpKill - Victim not in room: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( victim == ch )
    {
	logf( IMMORTAL, LOG_WORLD, "MpKill - Bad victim to attack: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
    {
	logf( IMMORTAL, LOG_WORLD, "MpKill - Charmed mob attacking master: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( ch->position == POSITION_FIGHTING )
    {	
	logf( IMMORTAL, LOG_WORLD, "MpKill - Already fighting: vnum %d",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    return attack( ch, victim, TYPE_UNDEFINED );
}


/* lets the mobile destroy an object in its inventory
   it can also destroy a worn object and it can destroy 
   just plain everything  */

int do_mpjunk( CHAR_DATA *ch, char *argument, int cmd )
{
    char      arg[ MAX_INPUT_LENGTH ];
    OBJ_DATA *obj;
    int location;

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0')
    {
        logf( IMMORTAL, LOG_WORLD, "Mpjunk - No argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( str_cmp( arg, "all" ) )
    {
      if ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &location )))
      {
	extract_obj( unequip_char( ch, location ) );
	return eSUCCESS;
      }
      if ((obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
      {
        extract_obj( obj );
        return eSUCCESS;
      }
    }
    else
    {
        for(int l = 0; l < MAX_WEAR; l++ )
          if ( ch->equipment[l] )
            extract_obj(unequip_char(ch,l));

        while(ch->carrying)
          extract_obj(ch->carrying);

        return eSUCCESS;
    }
    return eFAILURE;

}

/* prints the message to everyone in the room other than the mob and victim */

int do_mpechoaround( CHAR_DATA *ch, char *argument, int cmd )
{
  char       arg[ MAX_INPUT_LENGTH ];
  CHAR_DATA *victim;

    if ( !IS_NPC( ch ) )
    {
       send_to_char( "Huh?\n\r", ch );
       return eSUCCESS;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
       logf( IMMORTAL, LOG_WORLD, "Mpechoaround - No argument:  vnum %d.", mob_index[ch->mobdata->nr].virt );
       return eFAILURE|eINTERNAL_ERROR;
    }

    if ( !( victim=get_char_room( arg,ch->in_room ) ) )
    {
        logf( IMMORTAL, LOG_WORLD, "Mpechoaround - victim does not exist: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }
     if (CAN_SEE(ch,victim))
    act( argument+1, ch, NULL, victim, TO_ROOM, NOTVICT );
    return eSUCCESS;
}

/* prints the message to only the victim */

int do_mpechoat( CHAR_DATA *ch, char *argument, int cmd )
{
  char       arg[ MAX_INPUT_LENGTH ];
  CHAR_DATA *victim;

    if ( !IS_NPC( ch ) )
    {
       send_to_char( "Huh?\n\r", ch );
       return eSUCCESS;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
       logf( IMMORTAL, LOG_WORLD, "Mpechoat - No argument:  vnum %d.",
	   mob_index[ch->mobdata->nr].virt );
       return eFAILURE|eINTERNAL_ERROR;
    }

    if ( !( victim = get_char_room( arg, ch->in_room ) ) )
    {
        logf( IMMORTAL, LOG_WORLD, "Mpechoat - victim does not exist: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    act( argument+1, ch, NULL, victim, TO_VICT, 0 );
    return eSUCCESS;
}

/* prints the message to the room at large */

int do_mpecho( CHAR_DATA *ch, char *argument, int cmd )
{
    if ( !IS_NPC(ch) )
    {
        send_to_char( "Huh?\n\r", ch );
        return eSUCCESS;
    }

    if ( argument[0] == '\0' )
    {
        logf( IMMORTAL, LOG_WORLD, "Mpecho - called w/o argument: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
        return eFAILURE|eINTERNAL_ERROR;
    }

    act( argument+1, ch, NULL, NULL, TO_ROOM, 0 );
    return eSUCCESS;
}

/* lets the mobile load an item or mobile.  All items
are loaded into inventory.  you can specify a level with
the load object portion as well. */

int do_mpmload( CHAR_DATA *ch, char *argument, int cmd )
{
    char            arg[ MAX_INPUT_LENGTH ];
    int realnum;
    CHAR_DATA      *victim;

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' || !is_number(arg) )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpmload - Bad vnum as arg: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( ( realnum = real_mobile( atoi( arg ) ) ) < 0 )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpmload - Bad mob vnum: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    victim = clone_mobile( realnum );
    char_to_room( victim, ch->in_room );
    return eSUCCESS;
}

int do_mpoload( CHAR_DATA *ch, char *argument, int cmd )
{
    char arg1[ MAX_INPUT_LENGTH ];
    OBJ_DATA       *obj;
    int             realnum;

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    argument = one_argument( argument, arg1 );
 
    if ( arg1[0] == '\0' || !is_number( arg1 ) )
    {
        logf( IMMORTAL, LOG_WORLD, "Mpoload - Bad syntax: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
        return eFAILURE|eINTERNAL_ERROR;
    }
 
    if ( ( realnum = real_object( atoi( arg1 ) ) ) < 0 )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpoload - Bad vnum arg: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    obj = clone_object( realnum );
    if ( CAN_WEAR(obj, ITEM_TAKE) )
    {
	obj_to_char( obj, ch );
    }
    else
    {
	obj_to_room( obj, ch->in_room );
    }

    return eSUCCESS;
}

/* lets the mobile purge all objects and other npcs in the room,
   or purge a specified object or mob in the room.  It can purge
   itself, but this had best be the last command in the MOBprogram
   otherwise ugly stuff will happen */

int do_mppurge( CHAR_DATA *ch, char *argument, int cmd )
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        /* 'purge' */
        CHAR_DATA *vnext;
        OBJ_DATA  *obj_next;

	for ( victim = world[ch->in_room].people; victim != NULL; victim = vnext )
	{
	  vnext = victim->next_in_room;
	  if ( IS_NPC( victim ) && victim != ch )
	    extract_char( victim, TRUE );
	}

	for ( obj = world[ch->in_room].contents; obj != NULL; obj = obj_next )
	{
	  obj_next = obj->next_content;
	  extract_obj( obj );
	}

	return eSUCCESS;
    }

    if ( !( victim = get_char_room_vis( ch, arg )))
    {
	if ( ( obj = get_obj_in_list_vis( ch, arg, world[ch->in_room].contents ) ) ||
             ( obj = get_obj_in_list_vis( ch, arg, ch->carrying ) ) )
	{
	    extract_obj( obj );
	}

//      - We don't want to log this since it is quite possible that the item we want to purge
//      isn't there.  For example, if we tried to give a unique quest item and want to purge it
//      from our inventory if the player already had one.  It may or may not be there.
//	else
//	{
//	    logf( IMMORTAL, LOG_WORLD, "Mppurge - Bad argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
//            return eFAILURE|eINTERNAL_ERROR;
//	}
	return eSUCCESS;
    }

    if ( !IS_NPC( victim ) )
    {
	logf( IMMORTAL, LOG_WORLD, "Mppurge - Purging a PC: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

//    issame = (ch == victim);

    extract_char( victim, TRUE );

//    if(issame)

// we have to return both ch, and vict, since this could be in a greet or entry prog
// or a number of other possibilities and we dunno who died.  Otherwise we continue 
// trying to move someone that is dead.

      return eSUCCESS|eCH_DIED|eVICT_DIED;

//    return eSUCCESS;
}


/* lets the mobile goto any location it wishes that is not private */

int do_mpgoto( CHAR_DATA *ch, char *argument, int cmd )
{
    char             arg[ MAX_INPUT_LENGTH ];
    long location = -1;

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpgoto - No argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

// TODO - make this work with strings (goto chode) too

    location = atoi(arg);

    if ( location < 0 )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpgoto - No such location: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( ch->fighting != NULL )
	stop_fighting( ch );

    char_from_room( ch );
    char_to_room( ch, location );

    return eSUCCESS;
}

/* lets the mobile do a command at another location. Very useful */

int do_mpat( CHAR_DATA *ch, char *argument, int cmd )
{
    char             arg[ MAX_INPUT_LENGTH ];
    long             location;
    long             original;
    int              result;

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }
 
    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpat - Bad argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

// TODO - make location take args
    location = atoi(arg);

    if ( location < 0 )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpat - No such location: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    original = ch->in_room;
    char_from_room( ch );
    char_to_room( ch, location );
    result = command_interpreter( ch, argument );

    if(IS_SET(result, eCH_DIED))
      return result;

    char_from_room( ch );
    char_to_room( ch, original );

    return result;
}

// Reward the player with some XP
// Also works with -xp to penalize
int do_mpxpreward( CHAR_DATA *ch, char *argument, int cmd )
{
    char             arg[ MAX_INPUT_LENGTH ];
    char             buf[ MAX_INPUT_LENGTH ];
    int              reward;
    char_data *      vict;
    
    CHAR_DATA *get_pc_room_vis_exact(CHAR_DATA *ch, char *name);

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }
 
    half_chop(argument, arg, buf);

    if ( arg[0] == '\0' || !(vict = get_pc_room_vis_exact(ch, arg)))
    {
	logf( IMMORTAL, LOG_WORLD, "Mpxpreward - Bad argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if(!check_valid_and_convert(reward, buf)) 
    {
	logf( IMMORTAL, LOG_WORLD, "Mpxpreward - Bad argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if(reward < 0)
      if((GET_EXP(vict) + reward) < 0) {
        csendf(vict, "You lose %d exps.\r\n", (-1 * reward));
        GET_EXP(vict) = 0;
        return eSUCCESS;
      }
      
    csendf(vict, "You recieve %d exps.\r\n", reward);
    gain_exp(vict, reward);
    return eSUCCESS;
}
 
/* lets the mobile transfer people.  the all argument transfers
   everyone in the current room to the specified location */

int do_mptransfer( CHAR_DATA *ch, char *argument, int cmd )
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    long             location;
    descriptor_data *d;
    CHAR_DATA       *victim;

    if ( !IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
	logf( IMMORTAL, LOG_WORLD, "Mptransfer - Bad syntax: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( !str_cmp( arg1, "all" ) )
    {
	for ( d = descriptor_list; d != NULL; d = d->next )
	{
	    if ( d->connected == CON_PLAYING
	    &&   d->character != ch
	    &&   d->character->in_room
	    &&   CAN_SEE( ch, d->character ) )
	    {
		char buf[MAX_STRING_LENGTH];
		sprintf( buf, "%s %s", d->character->name, arg2 );
		do_trans( ch, buf, 9 );
	    }
	}
	return eSUCCESS;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' )
    {
	location = ch->in_room;
    }
    else
    {
// TODO - make location take args
        location = atoi(arg2);

	if ( location < 0 )
	{
	    logf( IMMORTAL, LOG_WORLD, "Mptransfer - No such location: vnum %d.",
	        mob_index[ch->mobdata->nr].virt );
	    return eFAILURE|eINTERNAL_ERROR;
	}

	if ( IS_SET(world[location].room_flags, PRIVATE))
	{
	    logf( IMMORTAL, LOG_WORLD, "Mptransfer - Private room: vnum %d.",
		mob_index[ch->mobdata->nr].virt );
	    return eFAILURE|eINTERNAL_ERROR;
	}
    }

    if ( ( victim = get_char_vis( ch, arg1 ) ) == NULL )
    {
	logf( IMMORTAL, LOG_WORLD, "Mptransfer - No such person: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( victim->in_room < 0)
    {
	logf( IMMORTAL, LOG_WORLD, "Mptransfer - Victim in Limbo: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( victim->fighting != NULL )
	stop_fighting( victim );

    char_from_room( victim );
    char_to_room( victim, location );

    return eSUCCESS;
}

/* lets the mobile force someone to do something.  must be mortal level
   and the all argument only affects those in the room with the mobile */

int do_mpforce( CHAR_DATA *ch, char *argument, int cmd )
{
    char arg[ MAX_INPUT_LENGTH ];

    if ( !IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpforce - Bad syntax: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

// TODO - this is dangerous...need to rework it.  If the person we force kills the
// next person we force, who was a mob, we can crash cause vch_next is no longer valid

    if ( !str_cmp( arg, "all" ) )
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

	for ( vch = world[ch->in_room].people; vch != NULL; vch = vch_next )
	{
	    vch_next = vch->next_in_room;

	    if (GET_LEVEL( vch ) < IMMORTAL 
		&& CAN_SEE( ch, vch ) )
	    {
		command_interpreter( vch, argument );
	    }
	}
    }
    else
    {
	CHAR_DATA *victim;

	if ( ( victim = get_char_room_vis( ch, arg ) ) == NULL )
	{
	    logf( IMMORTAL, LOG_WORLD, "Mpforce - No such victim: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
	    return eFAILURE|eINTERNAL_ERROR;
	}

	if ( victim == ch )
    	{
	    logf( IMMORTAL, LOG_WORLD, "Mpforce - Forcing oneself: vnum %d.",
	    	mob_index[ch->mobdata->nr].virt );
	    return eFAILURE|eINTERNAL_ERROR;
	}

	if (CAN_SEE( ch, victim ) )
	{
            if(GET_LEVEL( victim ) < IMMORTAL)
	        command_interpreter( victim, argument );
            else csendf(ch, "Mob %d just tried to MOBProg force you to '%s'.\r\n", 
                            mob_index[ch->mobdata->nr].virt, argument);
        }
    }

    return eSUCCESS;
}

// "Throw" a message to another mob.  Right now, it's only an int specifying which
// 'catch' should handle it
// argument should be <mob> <catchnum> <delay>
int do_mpthrow( CHAR_DATA *ch, char *argument, int cmd )
{
  struct mprog_throw_type * throwitem = NULL;
  int mob_num;
  int catch_num;
  int delay;

  char first[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  char second[MAX_INPUT_LENGTH];
  char third[MAX_INPUT_LENGTH];

  // locate and validate argument to find target
  half_chop(argument, first, buf);
  half_chop(buf, second, third);

  if(isdigit(*first)) {
    if(!check_valid_and_convert(mob_num, first) || (real_mobile(mob_num) < 0)) {
      logf( IMMORTAL, LOG_WORLD, "Mpthrow - Invalid mobnum: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
      return eFAILURE;
    }
    *first = '\0';
  }
  else {
    if(strlen(first) >= MAX_THROW_NAME) {
      logf( IMMORTAL, LOG_WORLD, "Mpthrow - Name too long: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
      return eFAILURE;
    }
  }

  if(!check_range_valid_and_convert(catch_num, second, MPROG_CATCH_MIN, MPROG_CATCH_MAX)) {
    logf( IMMORTAL, LOG_WORLD, "Mpthrow - Invalid catch_num: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
    return eFAILURE;
  }

  if(!check_range_valid_and_convert(delay, third, 0, 500)) {
    logf( IMMORTAL, LOG_WORLD, "Mpthrow - Invalid delay: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
    return eFAILURE;
  }

  // create struct
  throwitem = (struct mprog_throw_type *)dc_alloc(1, sizeof(struct mprog_throw_type));
  throwitem->target_mob_num = mob_num;
  strcpy(throwitem->target_mob_name, first);
  throwitem->data_num = catch_num;
  throwitem->delay = delay;

  // add to delay list
  throwitem->next = g_mprog_throw_list;
  g_mprog_throw_list = throwitem;

  return eSUCCESS;
}


int do_mpteachskill( CHAR_DATA *ch, char *argument, int cmd )
{
    char arg[ MAX_INPUT_LENGTH ];
    char skill[ MAX_INPUT_LENGTH ];

    int learn_skill(char_data * ch, int skill, int amount, int maximum);
    class_skill_defines * get_skill_list(char_data * ch);
    int search_skills(char * arg, class_skill_defines * list_skills);
    int search_skills2(int arg, class_skill_defines * list_skills);
    if ( !IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    half_chop(argument, arg, skill);

    if ( arg[0] == '\0' || skill[0] == '\0' )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpteachskill - Bad syntax: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    CHAR_DATA *victim;

    if ( ( victim = get_char_room( arg, ch->in_room ) ) == NULL )
    {
        logf( IMMORTAL, LOG_WORLD, "Mpteachskill - No such victim: vnum %d.", mob_index[ch->mobdata->nr].virt );
        return eFAILURE|eINTERNAL_ERROR;
    }

    int skillnum;
    if(!(check_range_valid_and_convert(skillnum, skill, 0, SKILL_SONG_MAX)))
    {
        logf( IMMORTAL, LOG_WORLD, "Mpteachskill - No such skill: vnum %d", mob_index[ch->mobdata->nr].virt);
        return eFAILURE|eINTERNAL_ERROR;
    }

    char * skillname = get_skill_name(skillnum);

    if(has_skill(victim, skillnum)) {
       csendf(victim, "You already know the basics of %s!\r\n", skillname ? skillname : "Unknown");
       return eFAILURE;
    }

    class_skill_defines * skilllist = get_skill_list(victim);
    if(!skilllist) {
      logf( IMMORTAL, LOG_WORLD, "Mpteachskill - (%s) %s had no skill list?", GET_SHORT(ch), GET_NAME(victim));
      return eFAILURE;  // no skills to train
    }

    int index = search_skills2(skillnum, skilllist);
    if(GET_LEVEL(victim) < skilllist[index].levelavailable) {
       csendf(victim, "You try to learn the basics of %s, but it is too advanced for you right now.\r\n", skillname);
       return eFAILURE;
    }

    if(skillname)
       sprintf(skill, "$BYou have learned the basics of %s.$R\n\r", skillname);
    else {
       send_to_char("I just tried to teach you an invalid skill.  Tell a god.\r\n", victim);
       logf( IMMORTAL, LOG_WORLD, "Mpteachskill - invalid skill number");
       return eFAILURE|eINTERNAL_ERROR;
    }

    send_to_char(skill, victim);

    learn_skill(victim, skillnum, 1, 1);/*
    // TEMP until borodin is happy
        char_data * boro = get_char("Borodin");
        if(boro) {
          sprintf(skill, "$B$2%s tells you, 'I just taught %s the basics of %s.'$R\n\r", GET_SHORT(ch), GET_NAME(victim), skillname);
          send_to_char(skill, boro);
        }
    }*/
    sprintf(skill,"$N has learned the basics of %s.",skillname);
    act(skill, ch, 0, victim, TO_ROOM, NOTVICT);

    return eSUCCESS;
}

int determine_attack_type(char * attacktype)
{
    extern char * strs_damage_types[];

    for(int i = 10; *strs_damage_types[i] != '\n'; i++)
       if(!strcmp(strs_damage_types[i], attacktype))
          return (TYPE_HIT + i);

    if(!strcmp("undefined", attacktype))
        return TYPE_UNDEFINED;

    return 0;
}

int do_mpsettemp(CHAR_DATA *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  char temp[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  if (!IS_NPC(ch)) {
    send_to_char("Huh?\r\n",ch);
    return eFAILURE;
  }
  half_chop(argument, arg, temp);
  victim = get_char_room( arg, ch->in_room );
  if (!victim) return eFAILURE;
  if (victim->tempVariable) 
     dc_free(victim->tempVariable);
  victim->tempVariable = str_dup(temp);
  return eSUCCESS;
}

int do_mpdamage( CHAR_DATA *ch, char *argument, int cmd )
{
    char arg[ MAX_INPUT_LENGTH ];
    char temp[ MAX_INPUT_LENGTH ];
    char damroll[ MAX_INPUT_LENGTH ];
    char attacktype[ MAX_INPUT_LENGTH ];

    if ( !IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    half_chop(argument, arg, temp);
    // arg = victim or 'all'
    // attacktype contains rest of string

    if ( arg[0] == '\0' )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpdamage - Bad syntax: Missing victim - vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    CHAR_DATA *victim = NULL;

    if(strcmp(arg, "all")) // if it's 'all' leave victim NULL and skip
    {
       victim = get_char_room( arg, ch->in_room );
       if(victim && ( GET_LEVEL(victim) > MORTAL || IS_MOB(victim) ) ) // don't target immortals
           victim = NULL;

       if (!victim)
           return eFAILURE;  // not an error, just couldn't get valid vict
    }

    // at this point, victim is either NULL (all) or pointing to valid vict
    half_chop(temp, damroll, attacktype);

    int numdice, sizedice;
    numdice = sizedice = 0;
    sscanf(damroll, "%dd%d", &numdice, &sizedice);

    if(!numdice || !sizedice)
    {
        logf( IMMORTAL, LOG_WORLD, "Mpdamage - Invalid damroll: vnum %d", mob_index[ch->mobdata->nr].virt);
        return eFAILURE|eINTERNAL_ERROR;
    }

    // figure out attack type
    int damtype = determine_attack_type(attacktype);
    if(!damtype)
    {
        logf( IMMORTAL, LOG_WORLD, "Mpdamage - Invalid damtype: vnum %d", mob_index[ch->mobdata->nr].virt);
        return eFAILURE|eINTERNAL_ERROR;
    }

    // do the damage
    if(victim)
       return damage(ch, victim, dice(numdice, sizedice), damtype, TYPE_UNDEFINED, 0);

    char_data * next_vict;
    int retval;
    for(victim = world[ch->in_room].people; victim; victim = next_vict)
    {
        next_vict = victim->next_in_room;
        if(IS_MOB(victim) || GET_LEVEL(victim) > MORTAL)
           continue;
        retval = damage(ch, victim, dice(numdice, sizedice), damtype, TYPE_UNDEFINED, 0);
        if(IS_SET(retval, eCH_DIED))
           return retval;
    }

    return eSUCCESS;
}


